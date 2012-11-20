
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2011, 2012 James McLaughlin.  All rights reserved.
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
#include "address.h"

static void resolver (lw_addr);

void lwp_addr_init (lw_addr ctx, const char * hostname,
                    const char * service, long hints)
{
   memset (&ctx, 0, sizeof (*ctx));

   ctx->resolver_thread = lw_thread_new ("resolver", resolver);

   ctx->hints = hints;

   ctx->hostname_to_free = ctx->hostname = strdup (hostname);

   while (isspace (*ctx->hostname))
      ++ ctx->hostname;

   while (isspace (ctx->hostname [strlen (ctx->hostname) - 1]))
      ctx->hostname [strlen (ctx->hostname) - 1] = 0;

   for (char * it = ctx->hostname; *it; ++ it)
   {
      if (it [0] == ':' && it [1] == '/' && it [2] == '/')
      {
         *it = 0;

         service = ctx->hostname;
         ctx->hostname = it + 3;
      }

      if (*it == ':')
      {
         /* an explicit port overrides the service name */

         service = it + 1;
         *it = 0;
      }
   }

   lwp_copy_string (ctx->service, service, sizeof (ctx->service)); 

   lw_thread_join (ctx->resolver_thread); /* block if the thread is already running */
   lw_thread_start (ctx->resolver_thread, ctx);
}

lw_addr lw_addr_new (const char * hostname, const char * service)
{
   lw_addr ctx = malloc (sizeof (*ctx));
   lwp_addr_init (ctx, hostname, service, 0);

   return ctx;
}

lw_addr lw_addr_new_port (const char * hostname, long port)
{
   char service [64];
   lwp_snprintf (service, sizeof (service), "%d", port);

   lw_addr ctx = malloc (sizeof (*ctx));
   lwp_addr_init (ctx, hostname, service, 0);

   return ctx;
}

lw_addr lw_addr_new_hint (const char * hostname, const char * service, long hints)
{
   lw_addr ctx = malloc (sizeof (*ctx));
   lwp_addr_init (ctx, hostname, service, hints);

   return ctx;
}

lw_addr lw_addr_new_port_hint (const char * hostname, long port, long hints)
{
   char service [64];
   lwp_snprintf (service, sizeof (service), "%d", port);

   lw_addr ctx = malloc (sizeof (*ctx));
   lwp_addr_init (ctx, hostname, service, hints);

   return ctx;
}

lw_addr lw_addr_clone (lw_addr ctx)
{
   lw_addr addr = calloc (sizeof (*addr), 1);

   if (lw_addr_resolve (addr))
      return 0;

   if (!addr->info)
      return 0;

   addr->info = malloc (sizeof (*addr->info));
   memcpy (addr->info, ctx->info, sizeof (*addr->info));

   addr->info->ai_next = 0;
   addr->info->ai_addr = malloc (addr->info->ai_addrlen);

   memcpy (addr->info->ai_addr, ctx->info->ai_addr, addr->info->ai_addrlen);

   memcpy (addr->service, ctx->service, sizeof (ctx->service));

   addr->hostname = addr->hostname_to_free = ctx->hostname;

   return addr;
}

void lwp_addr_cleanup (lw_addr ctx)
{
   lw_thread_join (ctx->resolver_thread);
   lw_thread_delete (ctx->resolver_thread);

   free (ctx->hostname_to_free);

   lw_error_delete (ctx->error);

   if (ctx->info_list)
   {
      #ifdef _WIN32
         fn_freeaddrinfo freeaddrinfo = compat_freeaddrinfo ();
      #endif

      freeaddrinfo (ctx->info_list);
   }

   if (ctx->info_to_free)
   {
      free (ctx->info_to_free->ai_addr);
      free (ctx->info_to_free);
   }
}

const char * lw_addr_to_string (lw_addr ctx)
{
   if (!lw_addr_is_ready (ctx))
      return "";

   if (*ctx->buffer)
      return ctx->buffer;

   if ((!ctx->info) || (!ctx->info->ai_addr))
      return "";

   switch (ctx->info->ai_family)
   {
      case AF_INET:

         lwp_snprintf (ctx->buffer, 
                       sizeof (ctx->buffer),
                       "%s:%d",
                       inet_ntoa (((struct sockaddr_in *)
                                     ctx->info->ai_addr)->sin_addr),
                       ntohs (((struct sockaddr_in *)
                             ctx->info->ai_addr)->sin_port));

         break;

      case AF_INET6:
      {             
         int length = sizeof (ctx->buffer) - 1;

         #ifdef _WIN32

            WSAAddressToStringA ((LPSOCKADDR) ctx->info->ai_addr,
                                 (DWORD) ctx->info->ai_addrlen,
                                 0,
                                 ctx->buffer,
                                 (LPDWORD) &length);

         #else

            inet_ntop (AF_INET6,
                       &((struct sockaddr_in6 *)
                       ctx->info->ai_addr)->sin6_addr,
                       ctx->buffer,
                       length);

         #endif

            lwp_snprintf (ctx->buffer + strlen (ctx->buffer) - 1,
                          sizeof (ctx->buffer) - strlen (ctx->buffer),
                          ":%d",
                          ntohs (((struct sockaddr_in6 *)
                                ctx->info->ai_addr)->sin6_port));

            break;
       }
   };

   return ctx->buffer ? ctx->buffer: "";
}

void resolver (lw_addr ctx)
{
   struct addrinfo hints;
   memset (&hints, 0, sizeof (hints));

   if (ctx->hints & lw_addr_hint_tcp)
   {
      if (!(ctx->hints & lw_addr_hint_udp))
         hints.ai_socktype = SOCK_STREAM;
   }
   else
   {
      if (ctx->hints & lw_addr_hint_udp)
         hints.ai_socktype = SOCK_DGRAM;
   }

   hints.ai_protocol  =  0;
   hints.ai_flags     =  AI_V4MAPPED | AI_ADDRCONFIG;

   if (ctx->hints & lw_addr_hint_ipv6)
      hints.ai_family = AF_INET6;
   else
      hints.ai_family = AF_INET;

   #ifdef _WIN32
      fn_getaddrinfo getaddrinfo = compat_getaddrinfo ();
   #endif

   int result = getaddrinfo
      (ctx->hostname, ctx->service, &hints, &ctx->info_list);

   if (result != 0)
   {
      lw_error_delete (ctx->error);
      ctx->error = lw_error_new ();

      lw_error_addf (ctx->error, "%s", gai_strerror (result));
      lw_error_addf (ctx->error, "getaddrinfo error");

      return;
   }

   for (struct addrinfo * info = ctx->info_list; info; info = info->ai_next)
   {
      if (info->ai_family == AF_INET6)
      {
         ctx->info = info;
         break;
      }

      if (info->ai_family == AF_INET)
      {
         ctx->info = info;
         break;
      }
   }
}

lw_bool lw_addr_ready (lw_addr ctx)
{
   return !lw_thread_started (ctx->resolver_thread);
}

long lw_addr_port (lw_addr ctx)
{
    if ((!lw_addr_ready (ctx)) || !ctx->info || !ctx->info->ai_addr)
        return 0;

    return ntohs (ctx->info->ai_family == AF_INET6 ?
        ((struct sockaddr_in6 *) ctx->info->ai_addr)->sin6_port :
            ((struct sockaddr_in *) ctx->info->ai_addr)->sin_port);
}

void lw_addr_set_port (lw_addr ctx, long port)
{
    if ((!lw_addr_ready (ctx)) || !ctx->info || !ctx->info->ai_addr)
        return;

    *ctx->buffer = 0;

    if (ctx->info->ai_family == AF_INET6)
        ((struct sockaddr_in6 *) ctx->info->ai_addr)->sin6_port = htons (port);
    else
        ((struct sockaddr_in *) ctx->info->ai_addr)->sin_port = htons (port);
}

lw_error lw_addr_resolve (lw_addr ctx)
{
   lw_thread_join (ctx->resolver_thread);

   return ctx->error;
}

static lw_bool sockaddr_equal (struct sockaddr * a, struct sockaddr * b)
{
   if ((!a) || (!b))
      return false;

   if (a->sa_family == AF_INET6)
   {
      if (b->sa_family != AF_INET6)
         return lw_false;

      return !memcmp (&((struct sockaddr_in6 *) a)->sin6_addr,
                      &((struct sockaddr_in6 *) b)->sin6_addr,
                      sizeof (struct in6_addr));
   }

   if (a->sa_family == AF_INET)
   {
      if (b->sa_family != AF_INET)
         return lw_false;

      return !memcmp (&((struct sockaddr_in *) a)->sin_addr,
                      &((struct sockaddr_in *) b)->sin_addr,
                      sizeof (struct in6_addr));
   }

   return lw_false;
}

lw_bool lw_addr_equal (lw_addr a, lw_addr b)
{
   if ((!a->info) || (!b->info))
      return true;

   return sockaddr_equal (a->info->ai_addr, b->info->ai_addr);
}

lw_bool lw_addr_ipv6 (lw_addr ctx)
{
   return ctx->info && ctx->info->ai_addr &&
      ((struct sockaddr_storage *) ctx->info->ai_addr)->ss_family == AF_INET6;
}

