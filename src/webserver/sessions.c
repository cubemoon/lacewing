
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

const char * const session_cookie = "lw_session";

void lw_ws_req_session_set (lw_ws_req request, const char * key,
                            const char * value)
{
   const char * cookie = lw_ws_req_cookie (session_cookie);

   lw_ws_session session;

   if (*cookie)
   {   HASH_FIND_STR (ctx->ws->sessions, cookie, session);
   }
   else
   {   session = 0;
   }

   if (!session)
   {
      char session_id [32];
      char session_id_hex [sizeof (session_id) * 2 + 1];

      lw_random (session_id, sizeof (session_id));

      for (int i = 0; i < sizeof (session_id); ++ i)
         sprintf (session_id_hex + i * 2, "%02x", session_id [i]);

      session = malloc (sizeof (*session));

      HASH_ADD_KEYPTR (hh, ctx->ws->sessions, session_id_hex,
                          sizeof (session_id_hex), session);
   }

   lwp_nvhash_set (&session->data, key, value, lw_true);
}

const char * lw_ws_req_session (lw_ws_req request, const char * key)
{
   const char * cookie = lw_ws_req_cookie (request, session_cookie);

   if (!*cookie)
      return "";

   lw_ws_session session;
   HASH_FIND_STR (ctx->ws->sessions, cookie, session);

   if (!session)
      return "";

   return lwp_nvhash_get (&session->data, key, "");
}

void lw_ws_close_session (lw_ws ws, const char * id)
{
   lw_ws_session session;
   HASH_FIND_STR (ws->sessions, id, session);

   if (!session)
      return;

   lwp_nvhash_clear (&session->data);
   HASH_DEL (ws->sessions, session);
}

void lw_ws_req_close_session (lw_ws_req request)
{
   lw_ws_close_session (request->ws, lw_ws_req_session (request));
}

const char * lw_ws_req_session (lw_ws_req request)
{
   return lw_ws_req_cookie (request, session_cookie);
}

lwp_ws_sessionitem lw_ws_req_first_sessionitem (lw_ws_req request)
{
   const char * cookie = lw_ws_req_cookie (request, session_cookie);

   if (!*cookie)
      return 0;

   lw_ws_session session;
   HASH_FIND_STR (ctx->ws->sessions, cookie, session);

   if (!session)
      return 0;

   return session->data;
}

lw_ws_sessionitem lw_ws_sessionitem_next (lw_ws_sessionitem item)
{
   return lwp_list_next (item);
}

const char * lw_ws_sessionitem_name (lw_ws_sessionitem item)
{
   return ((lwp_nvhash) item)->key;
}

const char * lw_ws_sessionitem_value (lw_ws_sessionitem item)
{
   return ((lwp_nvhash) item)->key;
}

