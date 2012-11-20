
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2011, 2012 James McLaughlin et al.  All rights reserved.
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
#include "../../deps/multipart-parser/multipart_parser.h"
#include "../stream.h"

typedef struct lwp_ws_client * lwp_ws_client;

struct lw_ws_req_hdr
{
    char * name, * value;
    lw_ws_req_hdr * next;
};

struct lw_ws_upload
{
    lw_ws_req request;

    lwp_nvhash disposition;
    
    lw_file autosave_file;
    char * autosave_filename;

    lwp_list (struct lw_ws_req_hdr, headers);
};

lw_ws_upload lwp_ws_upload_new (lw_ws_req request);
void lwp_ws_upload_delete (lw_ws_upload);

#include "multipart.h"

struct lw_ws_session
{
   lwp_nvhash data;
   UT_hash_handle hh;
};

struct lw_ws
{
   lw_pump pump;

   lw_server socket, socket_secure;

   lw_timer timer;

   lw_ws_session sessions;

   lw_bool auto_finish;

   long timeout;

   lw_ws_hook_error          on_error;
   lw_ws_hook_get            on_get;
   lw_ws_hook_post           on_post;
   lw_ws_hook_head           on_head;
   lw_ws_hook_upload_start   on_upload_start;
   lw_ws_hook_upload_chunk   on_upload_chunk;
   lw_ws_hook_upload_done    on_upload_done;
   lw_ws_hook_upload_post    on_upload_post;
   lw_ws_hook_disconnect     on_disconnect;

   void * tag;
};

struct lw_ws_req_cookie
{
   char * name, * value, * attr;
   lw_bool changed;

   UT_hash_handle hh;
};

struct lw_ws_req
{
   void * tag;

   lw_ws ws;
   lwp_ws_client client;

   struct lw_ws_req_cookie * cookies;


   /* Input */

   char version_major, version_minor;

   char method     [16];
   char url        [4096];
   char hostname   [128];

   lwp_list (struct lw_ws_req_hdr, headers_in);
   lwp_nvhash get_items, post_items;

    
   /* The protocol implementation can use this for any intermediate
    * buffering, providing it contains the request body (if any) when
    * the handler is called.
    */

   lwp_heapbuffer buffer;

   lw_bool parsed_post_data;


   /* Output */

   char status [64];

   lwp_list (struct lw_ws_req_hdr, headers_out);

   lw_bool responded;
};

void lwp_ws_req_clean (lw_ws_req);

void lwp_ws_req_set_cookie (lw_ws_req, size_t name_len, const char * name,
                                       size_t value_len, const char * value,
                                       size_t attr_len, const char * attr,
                                       lw_bool changed);

/* Request */

lw_bool lwp_ws_req_in_version (lw_ws_req, size_t len, const char * version);
lw_bool lwp_ws_req_in_method (lw_ws_req, size_t len, const char * method);

lw_bool lwp_ws_req_in_header (lw_ws_req, size_t name_len, const char * name,
                                         size_t value_len, const char * value);

lw_bool lwp_ws_req_in_url (lw_ws_req, size_t len, const char * url);


/* Response */

void lwp_ws_req_before_handler (lw_ws_req);
void lwp_ws_req_after_handler (lw_ws_req);

void lwp_ws_req_run_handler (lw_ws_req);

void lwp_ws_req_respond (lw_ws_req);


struct lwp_ws_client
{
   struct lw_stream stream;

   void (* respond) (lwp_ws_client, lw_ws_req request);
   void (* tick) (lwp_ws_client);
   void (* cleanup) ();

   lw_bool secure;

   lw_ws ws;
   lw_server_client socket;

   long timeout;

   lwp_ws_multipart multipart;
};

#include "http/http.h"

#ifndef LacewingNoSPDY
    #include "spdy/spdy.h"
#endif

