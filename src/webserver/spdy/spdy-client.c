
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

const static lw_stream_def spdyclient_def;

static void client_respond (lwp_ws_client, lw_ws_req request);
static void client_tick (lwp_ws_client);
static void client_cleanup (lwp_ws_client);

lwp_ws_client lwp_ws_spdyclient_new (lw_ws ws, lw_srv_client socket,
                                     lw_bool secure, int version)
{
   lwp_ws_spdyclient ctx = malloc (sizeof (*ctx));

   if (!ctx)
      return 0;

   ctx->ws = ws;

   ctx->respond = client_respond;
   ctx->tick = client_tick;

   lwp_stream_init ((lw_stream) ctx, spdyclient_def);

   if (!config.emit)
   {
      config.is_server = true;

      config.emit = ::spdy_emit;

      config.on_stream_create = ::spdy_stream_create;
      config.on_stream_headers = ::spdy_stream_headers;
      config.on_stream_data = ::spdy_stream_data;
      config.on_stream_close = ::spdy_stream_close;
   }

   ctx->spdy = spdy_ctx_new (&config, version, 0, 0);
   spdy_ctx_set_tag (ctx->spdy, this);

   lwp_trace ("SPDY context created @ %p", spdy);

   /* When the retry mode is Retry_MoreData and Put can't consume everything,
    * Put will be called again as soon as more data arrives.
    */

   lw_stream_retry ((lw_stream) ctx, lw_stream_retry_more_data);

   return (lwp_ws_client) ctx;
}

void client_cleanup (lwp_ws_client client)
{
   lwp_ws_spdyclient ctx = (lwp_ws_spdyclient) client;

   lw_ws_req request;

   lwp_list_each (ctx->requests, request)
   {
      if (ctx->ws->on_disconnect)
         ctx->ws->on_disconnect (ctx->ws, request);

      lwp_ws_req_free (request);
   }

   spdy_ctx_delete (ctx->spdy);

   lwp_stream_cleanup ((lw_stream) ctx);
}

static size_t def_spdyclient_sink_data (lwp_ws_client client,
                                        const char * buffer, size_t size)
{
   lwp_ws_spdyclient ctx = (lwp_ws_spdyclient) client;

   int res = spdy_data (ctx->spdy, buffer, &size);

   lwp_trace ("SPDY processed " lwp_fmt_size " bytes", size);

   if (res != SPDY_E_OK)
   {
      lwp_trace ("SPDY error: %s", spdy_error_string (res));

      lw_stream_close (ctx->socket, lw_true);

      return size;
   }

   return size;
}

void client_respond (lwp_ws_client client, lw_ws_req request)
{
   lwp_ws_spdyclient ctx = (lwp_ws_spdyclient) client;

    spdy_stream * stream = (spdy_stream *) request.Tag;
    
    assert (!spdy_stream_open_remote (stream));

    Socket.Cork ();

    spdy_nv_pair * headers = (spdy_nv_pair *)
        alloca (sizeof (spdy_nv_pair) * (request.OutHeaders.Size + 3));

    int n = 0;

    /* version header */

    char version [16];

    sprintf (version, "HTTP/%d.%d",
                (int) request.Version_Major, (int) request.Version_Minor);

    {   spdy_nv_pair &pair = headers [n ++];

        pair.name_len = strlen (pair.name = (char *)
             (spdy_active_version (spdy) == 2 ? "version" : ":version"));

        pair.value_len = strlen (pair.value = version);
    }

    /* status header */

    {   spdy_nv_pair &pair = headers [n ++];

        pair.name_len = strlen (pair.name = (char *)
             (spdy_active_version (spdy) == 2 ? "status" : ":status"));

        pair.value_len = strlen (pair.value = request.Status);
    }

    /* content-length header */

    size_t length = request.Queued ();

    char length_str [24];
    sprintf (length_str, lwp_fmt_size, length);

    {   spdy_nv_pair &pair = headers [n ++];

        pair.name_len = strlen (pair.name = (char *) "content-length");
        pair.value_len = strlen (pair.value = length_str);
    }

    for(List <WebserverHeader>::Element * E
            = request.OutHeaders.First; E; E = E->Next)
    {
        WebserverHeader &header = (** E);
        spdy_nv_pair &pair = headers [n ++];

        pair.name = header.Name;
        pair.name_len = strlen (header.Name);

        pair.value = header.Value;
        pair.value_len = strlen (header.Value);
    }

    if (length > 0)
    {
        int res = spdy_stream_write_headers (stream, n, headers, 0);

        if (res != SPDY_E_OK)
        {
            lwp_trace ("Error writing headers: %s", spdy_error_string (res));
        }

    }
    else
    {
        /* Writing headers with SPDY_FLAG_FIN when the stream is already closed
         * to the remote will cause the stream to be deleted.
         */

        spdy_stream_write_headers (stream, n, headers, SPDY_FLAG_FIN);
        stream = 0;
    }

    lwp_trace ("SPDY: %d headers -> stream @ %p, content length " lwp_fmt_size,
                    n, stream, length);

    Socket.Write (request, length);

    request.EndQueue ();
    request.BeginQueue ();

    if (length > 0)
    {
        spdy_stream_write_data (stream, 0, 0, SPDY_FLAG_FIN);
        stream = 0;
    }

    Socket.Uncork ();

    delete &request;
}

void client_tick (lwp_ws_client client)
{
   lwp_ws_spdyclient ctx = (lwp_ws_spdyclient) client;


}

