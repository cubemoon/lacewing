
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

#include "../lw_common.h"

/* FDStream makes the assumption that this will fail for anything but a regular
 * file (i.e. something that is always considered read ready)
 *
 * TODO: support more sendfile variants
 */

static lw_i64 lwp_sendfile (int source, int dest, lw_i64 size)
{
   #if defined (__linux__)

     ssize_t sent = 0;

     if ((sent = sendfile (dest, source, 0, size)) == -1)
         return errno == EAGAIN ? 0 : -1;
  
     return sent;

   #elif defined (__FreeBSD__)

     off_t sent = 0;

     if (sendfile (source, dest, lseek (source, 0, SEEK_CUR),
                     size, 0, &sent, 0) != 0)
     {
        if (errno != EAGAIN)
           return -1;
     }

     lseek (source, sent, SEEK_CUR);
     return sent;

   #elif defined (__APPLE__)

     off_t bytes = size;

     if (sendfile (source, dest, lseek (source, 0, SEEK_CUR),
                     &bytes, 0, 0) != 0)
     {
        if (errno != EAGAIN)
           return -1;
     }

     lseek (source, bytes, SEEK_CUR);
     return bytes;

   #endif

   errno = EINVAL;
   return -1;
}

#define lwp_fdstream_flag_nagle       1
#define lwp_fdstream_flag_is_socket   2
#define lwp_fdstream_flag_autoclose   4
#define lwp_fdstream_flag_reading     8

struct lw_fdstream
{
   struct lw_stream stream;

   int ref_count;

   lw_pump pump;
   lw_pump_watch watch;

   int fd;

   char flags;

   size_t size;
   size_t reading_size;
};

static void write_ready (void * tag)
{
   lw_fdstream * fdstream = tag;

   lw_stream_retry (fdstream, lw_stream_retry_now);
}

static void read_ready (void * tag)
{
   lw_fdstream ctx = tag;

   if (fd == -1)
      return;

   if (ctx->flags & lwp_fdstream_flag_reading)
      return;

   ctx->flags |= lwp_fdstream_flag_reading;

   ++ ctx->ref_count;

   /* TODO : Use a buffer on the heap instead? */

   char buffer [lwp_default_buffer_size];

   while (ctx->reading_size == -1 || ctx->reading_size > 0)
   {
      size_t to_read = sizeof (buffer);

      if (ctx->reading_size != -1 && to_read > ctx->reading_size)
         to_read = ctx->reading_size;

      int bytes = read (FD, buffer, to_read);

      if (bytes == 0)
      {
         lw_stream_close (ctx, lw_true);
         break;
      }

      if (bytes == -1)
      {
         if (errno == EAGAIN)
            break;

         lw_stream_close (ctx, lw_true);
         break;
      }

      if (ctx->reading_size != -1 && (ctx->reading_size -= bytes) < 0)
         ctx->reading_size = 0;

      lwp_stream_data (ctx->stream, buffer, bytes);

      /* Calling Data or Close may result in destruction of the Stream -
       * see FDStream destructor.
       */

      if (! (ctx->flags & lwp_fdstream_flag_reading))
         break;
   }

   ctx->flags &= ~ lwp_fdstream_flag_reading;

   if ((-- fdstream->ref_count) == 0 &&
         (ctx-flags & lwp_fdstream_flag_dead))
   {
      lw_fdstream_delete (ctx);
   }
}

void lw_fdstream_delete (lw_fdstream ctx)
{
   lw_fdstream_close (ctx, lw_true);

   if (ctx->ref_count > 0)
      ctx->flags |= lwp_fdstream_flag_dead;
   else
      lw_stream_delete (&ctx->stream);
}

void lw_fdstream_set_fd (lw_fdstream ctx, int fd, lw_pump_watch watch,
                         lw_bool auto_close)
{
   if (ctx->watch)
   {
      lw_pump_remove (ctx->pump, ctx->watch);
      ctx->watch = 0;
   }

   if ( (ctx->flags & lwp_fdstream_flag_autoclose) && ctx->fd != -1)
      lw_stream_close (ctx, lw_true);

   ctx->fd = fd;

   if (auto_close)
      ctx->flags |= lwp_fdstream_flag_autoclose;
   else
      ctx->flags &= ~ lwp_fdstream_flag_autoclose;

   if (ctx->fd == -1)
      return;

   fcntl (fd, F_SETFL, fcntl (fd, F_GETFL, 0) | O_NONBLOCK);

   #if HAVE_DECL_SO_NOSIGPIPE
   {  int b = 1;
      setsockopt (fd, SOL_SOCKET, SO_NOSIGPIPE, (char *) &b, sizeof (b));
   }
   #endif

   {  int b = (ctx->flags & lwp_fdstream_flag_nagle) ? 0 : 1;
      setsockopt (fd, SOL_SOCKET, TCP_NODELAY, (char *) &b, sizeof (b));
   }

   struct stat stat;
   fstat (fd, &stat);

   if (S_ISSOCK (stat.st_mode))
      ctx->flags |= lwp_fdstream_flag_is_socket;
   else
      ctx->flags &= ~ lwp_fdstream_flag_is_socket;

   if ((ctx->size = stat.st_size) > 0)
      return;

   /* Size is 0.  Is it really an empty file? */

   if (S_ISREG (stat.st_mode))
      return;

   ctx->size = -1;

   if (watch)
   {
      /* Given an existing Watch - change it to our callbacks */

      ctx->watch = watch;

      lw_pump_update_callbacks
         (ctx->pump, ctx->watch, ctx, read_ready, write_ready, lw_true);
   }
   else
   {
      ctx->watch = lw_pump_add (ctx->pump, fd, ctx, read_ready, write_ready);
   }
}

lw_bool lw_fdstream_is_valid (lw_fdstream ctx)
{
   return ctx->fd != -1;
}

void lw_fdstream_read (lw_fdstream ctx, size_t bytes)
{
   lwp_trace ("FDStream (FD %d) read " lwp_fmt_size, ctx->fd, bytes);

   lw_bool was_reading = ctx->reading_size != 0;

   if (bytes == -1)
      ctx->reading_size = lw_stream_bytes_left (ctx);
   else
      ctx->reading_size += bytes;

   if (!was_reading)
      ctx->try_read ();
}

size_t lw_fdstream_bytes_left (lw_fdstream ctx)
{
   if (ctx->fd == -1)
      return -1; /* not valid */

   if (ctx->size == -1)
      return -1;

   /* TODO : lseek64? */

   return ctx->size - lseek (ctx->fd, 0, SEEK_CUR);
}

lw_bool lw_fdstream_close (lw_fdstream ctx, lw_bool immediate)
{
   lwp_trace ("FDStream::Close for FD %d", ctx->fd);

   ++ ctx->ref_count;

   if (!lw_stream_close (ctx, immediate))
      return lw_false;

   /* NOTE: Public may be deleted at this point */

   /* Remove the watch to make sure we don't get any more
    * callbacks from the pump.
    */

   if (ctx->watch)
   {
      lw_pump_remove (ctx->pump, ctx->watch);
      ctx->watch = 0;
   }

   int fd = ctx->fd;

   ctx->fd = -1;

   if (fd != -1)
   {
      if (ctx->flags & lwp_fdstream_flag_autoclose)
      {
         shutdown (fd, SHUT_RDWR);
         close (fd);
      }
   }

   if ((-- ctx->ref_count) == 0 && (ctx->flags & lwp_fdstream_flag_dead))
      lw_fdstream_delete (ctx);

   return lw_true;
}

void lw_fdstream_cork (lw_fdstream ctx)
{
#ifdef LacewingCork
   int enabled = 1;
   setsockopt (ctx->fd, IPPROTO_TCP, LacewingCork, &enabled, sizeof (enabled));
#endif
}

void lw_fdstream_uncork (lw_fdstream ctx)
{
#ifdef LacewingCork
   int enabled = 0;
   setsockopt (ctx->fd, IPPROTO_TCP, LacewingCork, &enabled, sizeof (enabled));
#endif
}

void lw_fdstream_nagle (lw_fdstream ctx, lw_bool enabled)
{
   if (enabled)
      ctx->flags |= lwp_fdstream_flag_nagle;
   else
      ctx->flags &= ~ lwp_fdstream_flag_nagle;

   if (ctx->fd != -1)
   {
      int b = enabled ? 0 : 1;
      setsockopt (ctx->fd, SOL_SOCKET, TCP_NODELAY, (char *) &b, sizeof (b));
   }
}

static void sink_data (lw_stream stream, const char * buffer, size_t size)
{
   lw_fdstream ctx = (lw_fdstream) stream;

   size_t written;

   #if HAVE_DECL_SO_NOSIGPIPE
      written = write (ctx->fd, buffer, size);
   #else
      if (ctx->Flags & Internal::Flag_IsSocket)
         written = send (ctx->fd, buffer, size, MSG_NOSIGNAL);
   #else
      written = write (ctx->fd, buffer, size);
   #endif

   if (written == -1)
      return 0;

   return written;
}

static size_t sink_stream (lw_stream dest_stream,
                           lw_stream source_stream,
                           size_t size)
{
   if (lw_stream_type (source) != lwp_fdstream_type)
      return -1;

   lw_fdstream source = (lw_stream) source_source;
   lw_fdstream dest = (lw_stream) dest_stream;

   if (size == -1)
      size = lw_stream_bytes_left (source);

   lw_i64 sent = lwp_sendfile (source->fd, dest->fd, size);

   lwp_trace ("lwp_sendfile sent " lwp_fmt_size " of " lwp_fmt_size,
                     ((size_t) sent), (size_t) size);

   return sent;
}

const static lw_stream_def fdstream_def =
{
   sink_data,
   sink_stream,
   0, /* retry */
   is_transparent,
   0, /* close */
   0, /* cast */
   0, /* bytes_left */
   0, /* read */
};

void lw_fdstream_init (lw_fdstream ctx, lw_pump pump)
{
   memset (ctx, 0, sizeof (*ctx));

   lw_stream_init (&ctx->stream, pump);

   ctx->fd = -1;
   ctx->flags = lwp_fdstream_flag_nagle;
}

lw_fdstream lw_fdstream_new (lw_pump pump)
{
   lw_fdstream ctx = malloc (sizeof (*fdstream));
   lwp_fdstream_init (ctx, pump);

   return ctx;
}

