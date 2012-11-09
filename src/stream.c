
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

#include "common.h"
#include "stream.h"

void lwp_stream_init (lw_stream ctx, const lw_stream_def * def, lw_pump pump)
{
   memset (ctx, 0, sizeof (*ctx));

   ctx->def = def;
   ctx->pump = pump;

   ctx->retry = lw_stream_retry_never;

   ctx->graph = lwp_streamgraph_new ();
   lwp_list_push (ctx->graph->roots, ctx);

   ctx->last_expand = ctx->graph->last_expand;

   lwp_streamgraph_expand (ctx->graph);
}

lw_stream lw_stream_new (const lw_stream_def * def, lw_pump pump)
{
   lw_stream stream = malloc (sizeof (*stream));
   lwp_stream_init (stream, def, pump);

   return stream;
}

void lw_stream_delete (lw_stream ctx)
{
   lwp_list_clear (ctx->data_handlers);

   lwp_streamgraph_clear_expanded (ctx->graph);

   lw_stream_close (ctx, lw_true);

   /* Prevent any entry to lw_stream_close now */

   ctx->flags |= lwp_stream_flag_closing;

   /* If this stream is a root in the graph, remove it */

   lwp_list_remove (ctx->graph->roots, ctx);

   /* If this stream is filtering any other streams, remove it from their
    * filter list.
    */

   while (lwp_list_length (ctx->filtering) > 0)
   {
      struct lwp_stream_filterspec * spec = lwp_list_front (ctx->filtering);
      lwp_list_pop_front (ctx->filtering);

      lwp_list_remove (spec->stream->filters_upstream, spec);
      lwp_list_remove (spec->stream->filters_downstream, spec);

      free (spec);
   }

   /* If this stream is being filtered upstream by any other streams, remove
    * it from their filtering list.
    */

   while (lwp_list_length (ctx->filters_upstream) > 0)
   {
      struct lwp_stream_filterspec * spec
         = lwp_list_front (ctx->filters_upstream);

      lwp_list_pop_front (ctx->filters_upstream);

      lwp_list_remove (spec->filter->filtering, spec);

      if (spec->delete_with_stream)
         lw_stream_delete (spec->filter);

      free (spec);
   }

   /* If this stream is being filtered downstream by any other streams, remove
    * it from their filtering list.
    */

   while (lwp_list_length (ctx->filters_downstream) > 0)
   {
      struct lwp_stream_filterspec * spec
         = lwp_list_front (ctx->filters_downstream);

      lwp_list_pop_front (ctx->filters_downstream);

      lwp_list_remove (spec->filter->filtering, spec);

      if (spec->delete_with_stream)
         lw_stream_delete (spec->filter);

      free (spec);
   }

   /* Is the graph empty now? */

   if (lwp_list_length (ctx->graph->roots) == 0)
      lwp_streamgraph_delete (ctx->graph);
   else
      lwp_streamgraph_expand (ctx->graph);


   /* TODO : clear queues */

   if (ctx->user_count == 0)
      free (ctx);
}

/* The public lw_stream_write just calls lwp_stream_write with flags = 0 */

void lw_stream_write (lw_stream ctx, const char * buffer, size_t size)
{
   lwp_stream_write (ctx, buffer, size, 0);
}

/* Convenience queue functions for lwp_stream_write */

static void queue_back (lw_stream ctx, const char * buffer, size_t size)
{
   if ( (!lwp_list_length (ctx->back_queue)) ||
         lwp_list_back (ctx->back_queue).type != lwp_stream_queued_data)
   {
      struct lwp_stream_queued queued;
      memset (&queued, 0, sizeof (queued));
      queued.type = lwp_stream_queued_data;

      lwp_list_push (ctx->back_queue, queued);
   }

   lwp_heapbuffer_add (lwp_list_back (ctx->back_queue).buffer, buffer, size);
}

static void queue_front (lw_stream ctx, const char * buffer, size_t size)
{
   if ( (!lwp_list_length (ctx->front_queue)) ||
         lwp_list_back (ctx->front_queue).type != lwp_stream_queued_data)
   {
      struct lwp_stream_queued queued;
      memset (&queued, 0, sizeof (queued));
      queued.type = lwp_stream_queued_data;

      lwp_list_push (ctx->front_queue, queued);
   }

   lwp_heapbuffer_add (lwp_list_back (ctx->front_queue).buffer, buffer, size);
}

size_t lwp_stream_write (lw_stream ctx, const char * buffer, size_t size, int flags)
{
   if (size == -1)
      size = strlen (buffer);

   lwp_trace ("Writing " lwp_fmt_size " bytes", size);

   if (size == 0)
      return size; /* nothing to do */

   if ((! (flags & lwp_stream_write_ignore_filters)) && ctx->head_upstream)
   {
      lwp_trace ("%p is filtered upstream by %p; writing " lwp_fmt_size " to that",
                     ctx, ctx->head_upstream, size);

      /* There's a filter to write the data to first.  At the end of the
       * chain of filters, the data will be written back to us again
       * with the write_ignore_filters flag.
       */

      if (flags & lwp_stream_write_partial)
      {
         return lwp_stream_write (ctx->head_upstream, buffer, size,
                                  lwp_stream_write_partial);
      }

      lwp_stream_write (ctx->head_upstream, buffer, size, 0);

      return size;
   }

   if (lwp_list_length (ctx->prev) > 0)
   {
      if (! (flags & lwp_stream_write_ignore_busy))
      {
         lwp_trace ("Busy: Adding to back queue");

         if (flags & lwp_stream_write_partial)
            return 0;

         /* Something is behind us, but this data doesn't come from it.
          * Queue the data to write when we're not busy.
          */

         queue_back (ctx, buffer, size);

         return size;
      }

      /* Something is behind us and gave us this data. */

      if ((! (ctx->flags & lwp_stream_write_ignore_queue))
                && lwp_list_length (ctx->front_queue) > 0)
      {
         lwp_trace ("%p : Adding to front queue (queueing = %d, front queue length = %d)",
               ctx, (int) ( (ctx->flags & lwp_stream_flag_queueing) != 0),
               lwp_list_length (ctx->front_queue));

         if (flags & lwp_stream_write_partial)
            return 0;

         queue_front (ctx, buffer, size);

         if (ctx->retry == lw_stream_retry_more_data)
            lw_stream_retry (ctx, lw_stream_retry_now);

         return size;
      }

      if (ctx->def->is_transparent &&
            ctx->def->is_transparent (ctx))
      {
         lw_stream_data (ctx, buffer, size);
         return size;
      }

      size_t written = ctx->def->sink_data (ctx, buffer, size);

      lwp_trace ("%p : Stream sank " lwp_fmt_size " of " lwp_fmt_size,
                     ctx, written, size);

      if (flags & lwp_stream_write_partial)
         return written;

      if (written < size)
         queue_front (ctx, buffer + written, size - written);

      return size;
   }

   if ( (! (flags & lwp_stream_write_ignore_queue)) &&
         ( (ctx->flags & lwp_stream_flag_queueing)
           || lwp_list_length (ctx->back_queue) > 0))
   {
      lwp_trace ("%p : Adding to back queue (queueing = %d, front queue length = %d)",
            ctx, (int) ( (ctx->flags & lwp_stream_flag_queueing) != 0),
                            lwp_list_length (ctx->front_queue));

      if (flags & lwp_stream_write_partial)
         return 0;

      queue_back (ctx, buffer, size);

      if (ctx->retry == lw_stream_retry_more_data)
         lw_stream_retry (ctx, lw_stream_retry_now);

      return size;
   }
   
   /* If the stream def says the stream should be considered transparent, we
    * can skip sinking the data and just act as if it's already passed though.
    */

   if (ctx->def->is_transparent && ctx->def->is_transparent (ctx))
   {
      lw_stream_data (ctx, buffer, size);
      return size;
   }

   size_t written = ctx->def->sink_data (ctx, buffer, size);

   if (flags & lwp_stream_write_partial)
      return written;

   if (written < size)
   {
      if (flags & lwp_stream_write_ignore_queue)
      {
         if (lwp_heapbuffer_size (lwp_list_front (ctx->back_queue).buffer) == 0)
         {
            lwp_heapbuffer_add (lwp_list_front (ctx->back_queue).buffer,
                                buffer + written, size - written);
         }
         else
         {
            /* TODO : rewind offset where possible instead of creating a new Queued? */

            struct lwp_stream_queued queued;
            memset (&queued, 0, sizeof (queued));
            queued.type = lwp_stream_queued_data;

            lwp_heapbuffer_add (queued.buffer, buffer + written, size - written);

            lwp_list_push_front (ctx->back_queue, queued);
         }
      }
      else
      {
         queue_back (ctx, buffer + written, size - written);
      }
   }

   return size;
}

void lw_stream_write_stream (lw_stream ctx, lw_stream source,
      size_t size, bool delete_when_finished)
{
   if (size == 0)
   {
      if (delete_when_finished)
         lw_stream_delete (source);

      return;
   }

   lwp_stream_write_stream (ctx, source, size, delete_when_finished ?
                              lwp_stream_write_delete_stream : 0);
}

void lwp_stream_write_stream (lw_stream ctx, lw_stream source,
                              size_t size, int flags)
{
   if ( ( (! (flags & lwp_stream_write_ignore_queue)) &&
            (ctx->back_queue.length || ctx->flags & lwp_stream_flag_queueing) )
         || ( (! (flags & lwp_stream_write_ignore_busy)) && ctx->prev.length) )
   {
      lwp_stream_queued queued;
      memset (&queued, 0, sizeof (queued));

      queued.type = lwp_stream_queued_stream;
      queued.stream = source;
      queued.stream_bytes_left = size;
      queued.delete_stream = (flags & lwp_stream_write_delete_stream);

      lwp_list_push (ctx->back_queue, queued);

      return;
   }

   /* Are we currently in a different graph from the source stream? */

   if (ctx->graph != source->graph)
      lwp_streamgraph_swallow (source->graph, ctx->graph);

   assert (ctx->graph == source->graph);

   StreamGraph::Link * link = new (std::nothrow) StreamGraph::Link
      (source, 0, ctx, 0, size, flags & Write_DeleteStream);

   lwp_list_push (source->next, link);
   lwp_list_push (ctx->prev, link);

   /* This stream is now linked to, so doesn't need to be a root */

   lwp_list_remove (ctx->graph->roots, stream);

   lwp_streamgraph_expand (ctx->graph);
   lwp_streamgraph_read (ctx->graph);
}

void lw_stream_write_file (lw_stream ctx, const char * filename)
{
   /* This method may only be used when the stream is associated with a pump */

   assert (ctx->pump);

   lw_file file = lw_file_new (ctx->pump, filename, "rb");

   if (!lw_stream_valid (file))
      return;

   lw_stream_write_stream (ctx, file, lw_stream_bytes_left (file), lw_true);
}

void lw_stream_add_filter_upstream (lw_stream ctx, lw_stream filter,
                                    lw_bool delete_with_stream,
                                    lw_bool close_together)
{
   lwp_stream_filter_spec * spec = malloc (sizeof (*spec));

   spec->stream = ctx;
   spec->filter = filter;
   spec->delete_with_stream = delete_with_stream;
   spec->close_together = close_together;

   /* Upstream data passes through the most recently added filter first */

   lwp_list_push (ctx->filters_upstream, spec);
   lwp_list_push (filter->filtering, spec);

   if (filter->graph != ctx->graph)
      lwp_streamgraph_swallow (ctx->graph, filter->graph);

   lwp_streamgraph_expand (ctx->graph);
   lwp_streamgraph_read (ctx->graph);
}

void lw_stream_add_filter_downstream (lw_stream ctx, lw_stream filter,
                                      lw_bool delete_with_stream,
                                      lw_bool close_together)
{
   lwp_stream_filter_spec * spec = malloc (sizeof (*spec));

   spec->stream = ctx;
   spec->filter = filter;
   spec->delete_with_stream = delete_with_stream;
   spec->close_together = close_together;

   /* Downstream data passes through the most recently added filter last */

   lwp_list_push (ctx->filters_downstream, spec);
   lwp_list_push (filter->filtering, spec);

   if (filter->graph != ctx->graph)
      lwp_streamgraph_swallow (ctx->graph, filter->graph);

   lwp_streamgraph_expand (ctx->graph);
   lwp_streamgraph_read (ctx->graph);
}

size_t lw_stream_bytes_left (lw_stream ctx)
{
   return ctx->def->bytes_left ();
}

void lw_stream_read (lw_stream ctx, size_t bytes)
{
   ctx->def->read (bytes);
}

void lw_stream_data (lw_stream ctx, const char * buffer, size_t size)
{
   int num_data_handlers = ctx->exp_data_handlers.length, i = 0;

   ++ ctx->user_count;

   /* TODO: The data handler list would be faster to make a copy of if it was
    * a real array.
    */

   lwp_stream_data_handler * data_handlers =
      alloca (num_data_handlers * sizeof (lwp_stream_data_handler));

   lw_list_each (ctx->exp_data_handlers, handler)
   {
      data_handlers [i ++] = handler;

      ++ handler.stream->user_count;
   }

   for (int i = 0; i < num_data_handlers; ++ i)
   {
      lwp_stream_data_handler handler = data_handlers [i];

      if (! (handler.stream->flags & lwp_stream_flag_dead))
         handler.proc (handler.stream, handler.tag, buffer, size);

      if ((-- handler.stream->user_count) == 0)
         free (handler.stream);
   }

   /* Write the data to any streams next in the (expanded) graph, if this
    * stream still exists.
    */

   if (! (ctx->flags & lwp_stream_flag_dead))
   {
      lwp_stream_push (ctx, buffer, size);

      /* Pushing data may result in stream destruction */

      if ((-- ctx->user_count) == 0
            && (ctx->flags & lwp_stream_flag_dead))
      {
         free (ctx);
      }
   }
}

void lwp_stream_push (lw_stream ctx, const char * buffer, size_t size)
{
   lwp_streamgraph_link link;

   int num_links = ctx->next_expanded.length;

   if (!num_links)
      return; /* nothing to do */

   ++ ctx->user_count;

   lwp_streamgraph_link * links = alloca
      (num_links * sizeof (lwp_streamgraph_link));

   int i = 0;

   /* Copy the link dest pointers into our local array.
    *
    * TODO: This would be faster if the links were a real array in the first
    * place.
    */

   lwp_list_each (ctx->next_expanded, link)
   {
      links [i ++] = link;
   }

   int last_expand = ctx->graph->last_expand;

   for (i = 0; i < num_links; ++ i)
   {
      lwp_streamgraph_link * link = links [i];

      if (!link)
      {
         lwp_trace ("Link %d in the local Push list is now dead; skipping", i);
         continue;
      }

      lwp_trace ("Pushing to link %d of %d (%p)", i, num_links, link);

      size_t to_write = link->bytes_left != -1 &&
         size > link->bytes_left ? link->bytes_left : size;

      if (!link->to_exp->is_transparent ())
      {
         /* If we have some actual data, write it to the target stream */

         lwp_trace ("Link target is not transparent; buffer = %p", buffer);

         if (buffer)
         {
            lwp_stream_write (link->to_exp, buffer, to_write,
                  lwp_stream_write_ignore_filters |
                  lwp_stream_write_ignore_busy);
         }
      }
      else
      {
         lwp_trace ("Link target is transparent; pushing data forward");

         /* Target stream is transparent - have it push the data forward */

         lwp_stream_push (link->to_exp, buffer, to_write);
      }

      /* Pushing data may have caused this stream to be deleted */

      if (stream->flags & lwp_stream_flag_dead)
         break;

      if (graph->last_expand != last_expand)
      {
         lwp_trace ("Graph has re-expanded - checking local Push list...");

         /* If any new links have been added to NextExpanded, this Push will
          * not write to them.  However, if any links in our local array
          * (yet to be written to) have disappeared, they must be removed.
          */

         for (int x = i; x < num_links; ++ x)
         {
            if (!lwp_list_find (stream->next_expanded, links [x]))
            {
               if (link == links [x])
                  link = 0;

               links [x] = 0;
            }
         }

         if (!link)
            continue;
      }

      if (!link->to)
         continue; /* filters cannot expire */

      if (link->bytes_left == -1)
      {
         if (lw_stream_bytes_left (link->from_exp) != 0)
            continue;
      }
      else
      {
         if ((link->bytes_left -= to_write) > 0)
            continue;
      }

      if (link->delete_stream)
      {
         lw_stream_delete (link->from); /* will re-expand the graph */
      }
      else
      {
         lwp_graph_clear_expanded (stream->graph);

         lwp_list_remove (stream->next, link);
         lwp_list_remove (link->to->prev, link);

         /* Since the target and anything after it are still part
          * of this graph, make it a root before deleting the link.
          */

         lwp_list_push (stream->graph->roots, link->to);

         lwp_streamgraph_expand (stream->graph);

         free (link);

         lwp_streamgraph_read (stream->graph);
      }

      /* Maybe deleting the source stream caused this stream to be
       * deleted, too?
       */

      if (stream->flags & lwp_stream_flag_dead)
         break;

      lwp_trace ("Graph re-expanded by Push - checking local list...");

      /* Since the graph has re-expanded, check the rest of the links we
       * intend to write to are still present in the list.
       */

      for (int x = i; x < num_links; ++ x)
         if (!lwp_list_find (stream->next_expanded, links [x]))
            links [x] = 0;
   }

   if ((-- stream->user_count) == 0 && stream->flags & lwp_stream_flag_dead)
      free (stream);

   if (stream->flags & lwp_stream_flag_closeASAP
         && lwp_stream_may_close (stream))
   {
      lw_close (stream, lw_true);
   }
}

void lw_stream_retry (int when)
{
   lwp_trace ("Stream::Retry for %p (PrevDirect %p)",
         stream, stream->prev_direct);

   if (when == lw_stream_retry_now)
   {
      if (!lwp_stream_write_direct (stream))
      {
         /* TODO: ??? */
      }

      lwp_stream_write_queued (stream);
      return;
   }

   stream->retry = when;
}

void lwp_stream_write_queued (lw_stream stream)
{
   if (stream->flags & lwp_stream_flag_queueing)
      return;

   lwp_trace ("%p : Writing FrontQueue (size = %d)",
         this, stream->front_queue.length);

   lwp_stream_write_queue (stream, stream->front_queue); 

   lwp_trace ("%p : FrontQueue size is now %d, Prev.Size is %d, %d in back queue",
         stream, stream->front_queue.length, stream->prev.length, stream->back_queue.length);

   if (stream->front_queue.length == 0 && !stream->prev.length)
      lwp_stream_write_queue (stream, stream->back_queue);

   if (stream->flags & lwp_stream_flag_closeASAP
         && lwp_stream_may_close (stream))
   {
      lw_stream_close (stream, lw_true);
   }
}

void lwp_stream_write_queue (lw_stream stream, 
                             lwp_list (lwp_stream_queued *, queue))
{
   lwp_trace ("%p : WriteQueued : %d to write", stream, queue.length);

   lwp_list_each (stream->queue, queued)
   {
      if (queued.type == lwp_queued_begin_marker)
      {
         lwp_list_pop_front (stream->queue);
         stream->flags |= lwp_stream_flag_queueing;

         return;
      }

      if (queued.type == lwp_queued_data)
      {
         if (lwp_heapbuffer_size (queued.buffer) > 0)
         {
            /* There's still something in the buffer that needs to be written */

            int written = Write (queued.Buffer.Buffer + queued.Buffer.Offset,
                  queued.Buffer.Size - queued.Buffer.Offset,
                  Write_IgnoreQueue | Write_Partial |
                  Write_IgnoreBusy);

            if ((queued.Buffer.Offset += written) < queued.Buffer.Size)
               break; /* couldn't write everything */

            queued.Buffer.Reset ();
         }

         queue.PopFront ();

         continue;
      }

      if (queued.Type == Queued::Type_Stream)
      {
         Stream::Internal * stream = queued.StreamPtr;
         size_t bytes = queued.StreamBytesLeft;

         int flags = Write_IgnoreQueue;

         if (queued.Flags & Queued::Flag_DeleteStream)
            flags |= Write_DeleteStream;

         queue.PopFront ();

         Write (stream, bytes, flags);

         continue;
      }
   }
}

bool Stream::Internal::WriteDirect ()
{
   if (!PrevDirect)
      return true;

   size_t written = Public->Put (*PrevDirect->Public, DirectBytesLeft);

   if (written != -1)
   {
      PrevDirect->Push (0, written);

      if (DirectBytesLeft != -1)
         DirectBytesLeft -= written;

      return true;
   }

   return false;
}

lw_bool lwp_stream_may_close (lw_stream stream)
{
   return stream->prev.length == 0 &&
      stream->back_queue.length == 0 &&
      stream->front_queue.length == 0;
}

lw_bool lw_stream_close (lw_stream stream, lw_bool immediate)
{
   if (stream->flags & lwp_stream_flag_closing)
      return lw_false;

   if ( (!immediate) && !lwp_stream_may_close (stream))
   {
      stream->flags |= lwp_stream_flag_closeASAP;
      return lw_false;
   }

   stream->flags |= lwp_stream_flag_closing;

   ++ stream->user_count;

   /* If RootsExpanded is already empty, something else has already cleared
    * the expanded graph (e.g. another stream closing) and should re-expand
    * later (meaning we don't have to bother)
    */

   lw_bool already_cleared = (stream->graph->roots_expanded.length == 0);

   if (!already_cleared)
      lwp_streamgraph_clear_expanded (stream->graph);


   /* Anything that comes before us can no longer link here */

   lwp_list_each (stream->prev, link)
   {
      lwp_list_remove (link->from->next, link);

      free (link);
   }

   lwp_list_clear (stream->prev);


   /* Anything that comes after us will have to be a root */

   lwp_list_each (stream->next, link)
   {
      lwp_list_push (graph->roots, link->to);
      lwp_list_remove (link->to, link);

      free (link);
   }

   lwp_list_clear (stream->next);


   /* If we're set to close together with any filters, close those too (this
    * is the reason for the already_cleared check)
    */

   lw_stream ** to_close = alloca
      (sizeof (lw_stream *) * stream->filtering.length
       + stream->filters_upstream.length
       + stream->filters_downstream.length);

   int n = 0;

   lwp_list_each (stream->filtering, filter)
   {
      if (filter->close_together)
      {
         ++ filter->stream->user_count;
         to_close [n ++] = filter->stream;
      }
   }

   lwp_list_each (stream->filters_upstream, filter)
   {
      if (filter->close_together)
      {
         ++ filter->filter->user_count;
         to_close [n ++] = filter->filter;
      }
   }

   lwp_list_each (stream->filters_downstream, filter)
   {
      if (filter->close_together)
      {
         ++ filter->filter->user_count;
         to_close [n ++] = filter->filter;
      }
   }

   for (int i = 0; i < n; ++ i)
   {
      lw_stream * stream = to_close [i];

      -- stream->user_count;

      if (stream->flags & lwp_stream_flag_dead)
      {
         if (stream->user_count == 0)
            lw_stream_delete (stream);
      }
      else
      {
         lw_stream_close (stream);
      }
   }

   if (!already_cleared)
   {
      if (!lwp_list_find (graph->roots, this))
         lwp_list_push (graph->roots, this);

      lwp_streamgraph_expand (graph);
      lwp_streamgraph_read (graph);
   }


   lwp_list_each (stream->close_handlers, handler)
   {
      handler->proc (stream, handler->tag);

      if (stream->flags & lwp_stream_flag_dead)
         break;  /* close handler destroyed the stream */
   }

   stream->flags &= ~ lwp_stream_flag_closing;

   if ((-- stream->user_count) == 0
         && stream->flags & lwp_stream_flag_dead) /* see Stream destructor */
   {
      delete this;
   }

   return lw_true;
}

void lw_stream_begin_queue (lw_stream stream)
{
   if (stream->back_queue.length || stream->back_queue.length)
   {
      /* Although we're going to start queueing any new data, whatever is
       * currently in the queue still needs to be written.  A Queued with
       * the BeginMarker type indicates where the stream should stop writing
       * and set the Queueing flag
       */

      lwp_stream_queued queued;
      memset (&queued, 0, sizeof (*queued));
      queued.type = lwp_queued_begin_marker;

      lwp_list_push (stream->back_queue, queued);
   }
   else
   {
      stream->flags |= lwp_stream_flag_queueing;
   }
}

size_t lw_stream_queued (lw_stream stream)
{
   size_t size = 0;

   lwp_list_each (stream->back_queue, queued)
   {
      if (queued.type == lwp_queued_data)
      {
         size += queued.buffer->size - queued.buffer->offset;
         continue;
      }

      if (queued.type == lwp_queued_stream)
      {
         if (!queued.stream)
            continue;

         if (queued.stream_bytes_left != -1)
         {
            size += queued.stream_bytes_left;
            continue;
         }

         size_t bytes = lw_stream_bytes_left (queued.stream);

         if (bytes == -1)
            return -1;

         size += bytes;

         continue;
      }
   }

   return size;
}

void lw_stream_end_queue_hb (lw_stream stream, int head_buffers,
                             const char ** buffers, size_t * lengths)
{
   for (int i = 0; i < head_buffers; ++ i)
   {
      lwp_stream_write (stream, buffers [i], lengths [i],
            lwp_stream_write_ignore_queue | lwp_stream_write_ignore_busy);
   }

   lw_stream_end_queue (stream);
}

void lw_stream_end_queue (lw_stream stream)
{
   lwp_trace ("%p : EndQueue", stream);

   /* TODO : Look for a queued item w/ Flag_BeginQueue if Queueing is false? */

   assert (stream->flags & lwp_stream_flag_queueing);

   stream->flags &= ~ lwp_stream_flag_queueing;

   lwp_stream_write_queued (stream);
}

bool lwp_stream_is_transparent (lw_stream stream)
{
   assert (! (stream->flags & lwp_stream_flag_dead));

   return (!stream->exp_data_handlers.length) &&
      (stream->back_queue.length + stream->front_queue.length) == 0
      && (! (stream->flags & lwp_stream_flag_queueing))
      && stream->def->is_transparent ();
}

void lw_stream_add_handler_data (lw_stream stream,
                                 lw_stream_handler_data proc,
                                 void * tag)
{   
   /* TODO : Prevent the same handler being registered twice? */

   lwp_stream_data_handler * handler = calloc (sizeof (*handler), 1);

   handler->proc = proc;
   handler->stream = stream;
   handler->tag = tag;

   lwp_list_push (stream->data_handlers, handler);

   lwp_streamgraph_clear_expanded (stream->graph);
   lwp_streamgraph_expand (stream->graph);

   /* TODO: Do we need to call lwp_streamgraph_read here? */
} 

void lw_stream_remove_handler_data (lw_stream stream,
                                    lw_stream_handler_data proc,
                                    void * tag)
{   
   lwp_list_each (stream->data_handlers, handler)
   {
      if (handler->proc == proc && handler->tag == tag)
      {
         lwp_list_erase (stream->data_handlers, handler);
         break;
      }
   }

   lwp_streamgraph_clear_expanded (stream->graph);
   lwp_streamgraph_expand (stream->graph);
}

void lw_stream_add_handler_close (lw_stream stream,
                                  lw_stream_handler_close proc,
                                  void * tag)
{   
   lwp_stream_close_handler handler = { proc, tag };
   lwp_list_push (stream->close_handlers, handler);

   lwp_streamgraph_clear_expanded (stream->graph);
   lwp_streamgraph_expand (stream->graph);
} 

void lw_stream_remove_handler_close (lw_stream stream,
                                     lw_stream_handler_close proc,
                                     void * tag)
{   
   lwp_list_each (stream->close_handlers, handler)
   {
      if (handler->proc == proc && handler->tag == tag)
      {
         lwp_list_erase (stream->close_handlers, handler);
         break;
      }
   }

   lwp_streamgraph_clear_expanded (stream->graph);
   lwp_streamgraph_expand (stream->graph);
}



