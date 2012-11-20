
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

#include "common.h"

static void on_connect (lw_server server, lw_server_client client_socket)
{
   lw_ws ws = lw_server_tag (server);

   lw_bool secure = (server == ws->socket_secure);

   lwp_ws_client client;

   do
   {
      #ifndef LacewingNoSPDY

         if (!strcasecmp (lw_server_client_npn (client_socket), "spdy/3"))
         {
            client = lwp_ws_spdyclient_new (ws, client_socket, secure, 3);
            break;
         }

         if (!strcasecmp (lw_server_client_npn (client_socket), "spdy/2"))
         {
            client = lwp_ws_spdyclient_new (ws, client_socket, secure, 2);
            break;
         }

      #endif

      client = lwp_ws_httpclient_new (ws, client_socket, secure);

   } while (0);

   if (!client)
   {
      lw_stream_close (client_socket, lw_true);
      return;
   }

   lw_stream_set_tag (client_socket, client);
   lw_stream_write_stream ((lw_stream) client, client_socket, -1, lw_false);
}

static void on_disconnect (lw_server server, lw_server_client client_socket)
{
   lwp_ws_client client = lw_stream_tag (client_socket);

   client->cleanup ();
   free (client);
}

static void on_error (lw_server server, lw_error error)
{
    lwp_trace ("Webserver: Socket error: %s", lw_error_tostring (error));

    lw_error_addf (error, "Socket error");

    lw_ws ws = lw_server_tag (server);

    if (ws->on_error)
        ws->on_error (ws, error);
}

static void start_timer (lw_ws ctx)
{
   #ifdef LacewingTimeoutExperiment

      if (lw_timer_started (ctx->timer))
         return;

      lw_timer_start (ctx->timer, ctx->timeout * 1000);

   #endif
}

static void prepare_socket (lw_ws ctx)
{
   if (!ctx->socket)
   {
      ctx->socket = lw_server_new (ctx->pump);
      lw_server_set_tag (ctx->socket, ctx);

      lw_server_onconnect (ctx->socket, on_connect);
      lw_server_ondisconnect (ctx->socket, on_disconnect);
      lw_server_onerror (ctx->socket, on_error);
   }

   start_timer (ctx);
}

static void prepare_socket_secure (lw_ws ctx)
{
   if(!ctx->socket_secure)
   {
      ctx->socket_secure = lw_server_new (ctx->pump);
      lw_server_set_tag (ctx->socket_secure, ctx);

      lw_server_onconnect (ctx->socket_secure, on_connect);
      lw_server_ondisconnect (ctx->socket_secure, on_disconnect);
      lw_server_onerror (ctx->socket_secure, on_error);

      #ifndef LacewingNoSPDY
         lw_server_add_npn (ctx->socket_secure, "spdy/3");
         lw_server_add_npn (ctx->socket_secure, "spdy/2");
      #endif

      lw_server_add_npn (ctx->socket_secure, "http/1.1");
      lw_server_add_npn (ctx->socket_secure, "http/1.0");
   }

   start_timer (ctx);
}

static void on_timer_tick (lw_timer timer)
{
   lw_server_client client_socket;
   lwp_ws_client client;
   lw_ws ws = lw_timer_tag (timer);

   if (ws->socket)
   {
      for (client_socket = lw_server_client_first (ws->socket);
           client_socket;
           client_socket = lw_server_client_next (client_socket))
      {
         client = lw_stream_tag (client_socket);

         client->tick (client);
      }
   }

   if (ws->socket_secure)
   {
      for (client_socket = lw_server_client_first (ws->socket_secure);
           client_socket;
           client_socket = lw_server_client_next (client_socket))
      {
         client = lw_stream_tag (client_socket);

         client->tick (client);
      }
   }
}

lw_ws lw_ws_new (lw_pump pump)
{
    lwp_init ();
    
    lw_ws ctx = calloc (sizeof (*ctx), 1);

    ctx->pump = pump;
    ctx->auto_finish = lw_true;
    ctx->timeout = 5;

    lw_timer_set_tag (ctx->timer, ctx);
    lw_timer_on_tick (on_timer_tick);

    return ctx;
}

void lw_ws_delete (lw_ws ctx)
{
   lw_ws_unhost (ctx);
   lw_ws_unhost_secure (ctx);

   free (ctx);
}

void lw_ws_host (lw_ws ctx, long port)
{
   lw_filter filter = lw_filter_new ();
   lw_filter_local_port (filter, port);

   lw_ws_host_filter (filter);
}

void lw_ws_host_filter (lw_ws ctx, lw_filter filter)
{
   prepare_socket (ctx);

   if (!lw_filter_local_port (filter))
      lw_filter_local_port (filter, 80);

   lw_server_host (ctx->socket, filter);
}

void lw_ws_host_secure (lw_ws ctx, long port)
{
   lw_filter filter = lw_filter_new ();
   lw_filter_local_port (filter, port);

   lw_ws_host_secure_filter (filter);
}

void lw_ws_host_secure_filter (lw_ws ctx, lw_filter filter)
{
   if(!lw_ws_certificate_loaded (ctx))
      return;

   prepare_socket_secure (ctx);

   if (!lw_filter_local_port (filter))
      lw_filter_local_port (filter, 443);

   lw_server_host (ctx->socket_secure, filter);
}

void lw_ws_unhost (lw_ws ctx)
{
   if (ctx->socket)
      lw_server_unhost (ctx->socket);
}

void lw_ws_unhost (lw_ws ctx)
{
   if (ctx->socket_secure)
      lw_server_unhost (ctx->socket_secure);
}

lw_bool lw_ws_hosting (lw_ws ctx)
{
    if (!ctx->socket)
        return lw_false;

    return lw_server_hosting (ctx->socket);
}

lw_bool lw_ws_hosting_secure (lw_ws ctx)
{
    if (!ctx->socket_secure)
        return lw_false;

    return lw_server_hosting (ctx->socket_secure);
}

long lw_ws_port (lw_ws ctx)
{
    if (!ctx->socket)
        return 0;

    return lw_server_port (ctx->socket);
}

int lw_ws_port_secure (lw_ws ctx)
{
    if (!ctx->socket_secure)
        return 0;

    return lw_server_port (ctx->socket_secure);
}

lw_bool lw_ws_load_cert_file (lw_ws ctx, const char * filename,
                              const char * passphrase)
{
   prepare_socket_secure (ctx);
   
   return lw_server_load_cert_file (ctx->socket_secure, filename, common_name);
}

lw_bool lw_ws_load_sys_cert (lw_ws ctx, const char * store_name,
                                        const char * common_name,
                                        const char * location)
{
   prepare_socket_secure (ctx);
   
   return lw_server_load_sys_cert (ctx->socket_secure, store_name,
                                   common_name, location);
}

lw_bool lw_ws_cert_loaded (lw_ws ctx)
{
   return ctx->socket_secure && lw_server_cert_loaded (ctx->socket_secure);
}

void lw_ws_enable_manual_finish (lw_ws ctx)
{
    ctx->auto_finish = lw_false;
}

void lw_ws_idle_timeout (long seconds)
{
    ctx->timeout = seconds;

    if (lw_timer_started (ctx->timer))
    {
       lw_timer_stop (ctx->timer);
       lw_timer_start (ctx->timer);
    }
}

long lw_ws_idle_timeout (lw_ws ctx)
{
    return ctx->timeout;
}

lw_def_hook (ws, get)
lw_def_hook (ws, post)
lw_def_hook (ws, head)
lw_def_hook (ws, error)
lw_def_hook (ws, upload_start)
lw_def_hook (ws, upload_chunk)
lw_def_hook (ws, upload_done)
lw_def_hook (ws, upload_post)
lw_def_hook (ws, disconnect)

