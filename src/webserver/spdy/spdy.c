
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2012 James McLaughlin.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "../common.h"

static void spdy_emit (spdy_ctx * ctx, const char * buffer, size_t size)
{
    lwp_ws_spdyclient ctx = (lwp_ws_spdyclient) spdy_ctx_get_tag (ctx);
    
    lw_stream_write (ctx->socket, buffer, size);
}

static spdy_config config = { 0 };


static void on_stream_create (spdy_ctx * spdy, spdy_stream * stream,
                              size_t num_headers, spdy_nv_pair * headers)
{
    lwp_ws_spdyclient ctx = spdy_ctx_get_tag (spdy);

    lwp_ws_request request = lwp_ws_request_new (ctx);

    if (!request)
    {
        spdy_stream_close (stream, SPDY_STATUS_INTERNAL_ERROR);
        return;
    }

    request->tag = stream;
    spdy_stream_set_tag (stream, request);

    lw_stream_begin_queue ((lw_stream) request);

    on_stream_headers (spdy, stream, num_headers, headers);

    if (!spdy_stream_open_remote (stream))
    {
        /* Stream is already half-closed to the remote endpoint, so we
         * aren't expecting a request body.
         */

        on_request_complete (stream);
    }
}

static void on_request_complete (spdy_ctx * ctx, spdy_stream * stream)
{
    lwp_ws_request request = spdy_stream_get_tag (stream);

    lwp_trace ("SPDY request complete: %p", request);

    lwp_request_run_handler (request);
}

static void on_stream_headers (spdy_ctx * ctx, spdy_stream * stream,
                               size_t num_headers, spdy_nv_pair * headers)
{
    lwp_ws_request request = spdy_stream_get_tag (stream);

    lwp_trace ("SPDY: Got %d headers for stream @ %p", num_headers, stream);

    for (size_t i = 0; i < num_headers; ++ i)
    { 
        spdy_nv_pair * header = &headers [i];

        if (spdy_active_version (spdy) == 2)
        {
            if (!strcmp (header->name, "version"))
            {
                lwp_ws_req_in_version
                    (request, header->value_len, header->value);

                continue;
            }

            if (!strcmp (header->name, "method"))
            {
                lwp_ws_req_in_method
                    (request, header->value_len, header->value);

                continue;
            }

            if (!strcmp (header->name, "url"))
            {
                lwp_ws_req_in_url
                    (request, header->value_len, header->value);

                continue;
            }
        }
        else
        {
            if (!strcmp (header->name, ":version"))
            {
                lwp_ws_req_in_version
                    (request, header->value_len, header->value);

                continue;
            }

            if (!strcmp (header->name, ":method"))
            {
                lwp_ws_req_in_method
                    (request, header->value_len, header->value);

                continue;
            }

            if (!strcmp (header->name, ":scheme"))
               continue;

            if (!strcmp (header->name, ":path"))
            {
                lwp_ws_req_in_url
                    (request, header->value_len, header->value);

               continue;
            }
        }

        /* Each header may contain multiple values separated by 0 octets. */

        char * value_start = header->value;
        size_t value_len = 0;

        for (size_t i = 0 ;; ++ i)
        {
            if (i == header->value_len)
            {
                lwp_ws_req_in_header (request, header->name_len, header->name,
                                      value_len, value_start);

                break;
            }

            if (header->value [i] == '\0')
            {
                lwp_ws_req_in_header (request, header->name_len, header->name,
                                      value_len, value_start);

                value_len = 0;
                value_start = header->value + i + 1;

                continue;
            }

            ++ value_len;
        }
    }
}

static void on_stream_data (spdy_ctx * ctx, spdy_stream * stream,
                            const char * data, size_t size)
{
    lwp_ws_request request = spdy_stream_get_tag (stream);

    lwp_heapbuffer_add (request->buffer, data, size);

    if (!spdy_stream_open_remote (stream))
        on_request_complete (ctx, stream);
}

static void on_stream_close (spdy_stream * stream, int status_code)
{
    lwp_ws_request request = spdy_stream_get_tag (stream);

    if (!request)
        return;

    delete request;
}

static size_t spdy_request_sink_data (const char * buffer, size_t size)
{
    lwp_trace ("SPDY: Writing " lwp_fmt_size " bytes of data", size);

    spdy_stream_write_data (stream, buffer, size, 0);

    return size;
}


