
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
#include "streamgraph.h"
#include "stream.h"

static void lwp_streamgraph_print (lwp_streamgraph graph);

lwp_streamgraph lwp_streamgraph_new ()
{
   lwp_streamgraph graph = calloc (sizeof (*graph), 1);

   return graph;
}

void lwp_streamgraph_delete (lwp_streamgraph graph)
{
   lwp_streamgraph_clear_expanded (graph);

   if (graph->user_count == 0)
      free (graph);
   else
      graph->dead = lw_true;
}

static void swallow (lwp_streamgraph graph, lw_stream stream)
{
   struct lwp_stream_filterspec * spec;
   lwp_streamgraph_link link;

   assert (graph);
   assert (stream->graph);

   if (stream->graph == graph)
      return;

   stream->graph = graph;
   stream->last_expand = graph->last_expand;

   lwp_list_each (stream->filters_upstream, spec)
   {
      swallow (graph, spec->filter);
   }

   lwp_list_each (stream->filters_downstream, spec)
   {
      swallow (graph, spec->filter);
   }

   lwp_list_each (stream->filtering, spec)
   {
      swallow (graph, spec->stream);
   }

   lwp_list_each (stream->next, link)
   {
      swallow (graph, link->to);
   }
}

void lwp_streamgraph_swallow (lwp_streamgraph graph, lwp_streamgraph old_graph)
{
   lw_stream root;

   assert (old_graph != graph);

   lwp_streamgraph_clear_expanded (old_graph);

   lwp_list_each (old_graph->roots, root)
   {
      lwp_list_push (graph->roots, root);
      swallow (graph, root);
   }

   lwp_streamgraph_delete (old_graph);
}

static void expand_stream (lwp_streamgraph graph, lw_stream stream,
                           lw_stream * first, lw_stream * last)
{
   struct lwp_stream_filterspec * spec;
   struct lwp_stream_data_hook hook;

   *first = *last = 0;


   /* Upstream filters come first */

   lwp_list_each (stream->filters_upstream, spec)
   {
      lw_stream stream = spec->filter, expanded, next;

      stream->last_expand = graph->last_expand;

      expand_stream (graph, stream, &expanded, &next);

      if (!*first)
         *first = expanded;
      else
      {
         spec->link.from_exp = *last;
         spec->link.to_exp = expanded;

         lwp_list_push ((*last)->next_expanded, &spec->link);
         lwp_list_push (expanded->prev_expanded, &spec->link);
      }

      *last = next;
   }

   stream->head_upstream = *first;


   /* Now the stream itself */

   stream->last_expand = graph->last_expand;

   if (*last)
   {
      lwp_streamgraph_link link = calloc (sizeof (*link), 1);

      link->from_exp = *last;
      link->to_exp = stream;
      link->bytes_left = -1;

      lwp_list_push ((*last)->next_expanded, link);
      lwp_list_push (stream->prev_expanded, link);

      *last = stream;
   }
   else
   {
      *first = *last = stream;
   }


   /* And downstream filters afterwards */

   lwp_list_each (stream->filters_downstream, spec)
   {
      lw_stream stream = spec->filter, expanded, next;

      stream->last_expand = graph->last_expand;

      expand_stream (graph, stream, &expanded, &next);

      if (last)
      {
         spec->link.from_exp = *last;
         spec->link.to_exp = expanded;

         lwp_list_push ((*last)->next_expanded, &spec->link);
         lwp_list_push (expanded->prev_expanded, &spec->link);
      }

      *last = next;
   }


   /* The last downstream filter is in charge of calling any data hooks */

   lwp_list_each (stream->data_hooks, hook)
   {
      lwp_list_push ((*last)->exp_data_hooks, hook);
   }
}

static void expand (lwp_streamgraph graph, lw_stream last,
                    lwp_streamgraph_link last_link,
                    lwp_streamgraph_link link, lw_stream stream)
{
   lw_stream expanded_first, expanded_last;

   expand_stream (graph, stream, &expanded_first, &expanded_last);

   if (last)
   {
      link->to_exp = expanded_first;

      lwp_list_push (expanded_first->prev_expanded, link);

      link->from_exp = last;

      assert (!lwp_list_find (last->next_expanded, link));

      lwp_list_push (last->next_expanded, link);
   }
   else
   {
      lwp_list_push (graph->roots_expanded, expanded_first);
   }

   last = expanded_last;
   last_link = link;

   lwp_list_each (stream->next, link)
   {
      expand (graph, last, last_link, link, link->to);
   }
}

void lwp_streamgraph_expand (lwp_streamgraph graph)
{
   lw_stream stream;

   lwp_streamgraph_clear_expanded (graph);

   lwp_list_each (graph->roots, stream)
   {
      if (stream->last_expand == graph->last_expand)
         continue;

      expand (graph, 0, 0, 0, stream);
   }

   lwp_list_reduce (graph->roots_expanded, stream)
   {
      /* A root only needs to remain a root if it doesn't appear
       * elsewhere in the graph.
       */

      if (!lwp_list_length (stream->prev_expanded))
         lwp_list_reduce_keep ();
   }

   #ifdef LacewingDebug
      lwp_streamgraph_print (graph);
   #endif
}

static lw_bool find_next_direct (lw_stream stream, lw_stream * next_direct,
                                 size_t * bytes)
{
   lwp_streamgraph_link link;

   lwp_list_each (stream->next_expanded, link)
   {
      if (link->bytes_left != -1 && link->bytes_left < *bytes)
         *bytes = link->bytes_left;

      lw_stream next = link->to_exp;

      if (lw_stream_is_transparent (next))
      {
         if (!find_next_direct (next, next_direct, bytes))
            return lw_false;
      }

      if (next)
      {
         if (*next_direct)
            return lw_false;

         *next_direct = next;
      }
   }

   return lw_true;
}

static void graph_read (lwp_streamgraph graph, int this_expand,
                        lw_stream stream, size_t bytes)
{
   lwp_streamgraph_link link;

   /* TODO : Currently, the presence of a filter forces a read */

   if (!lwp_list_length (stream->next_expanded))
      return;

   if (bytes == -1)
   {
      /* Use the biggest bytes_left from all the links */

      lwp_list_each (stream->next, link)
      {
         if (link->bytes_left == -1)
         {
            bytes = -1;
            break;
         }

         if (link->bytes_left > bytes)
            bytes = link->bytes_left;
      }
   }

   lwp_trace ("Reading " lwp_fmt_size " from %p", bytes, stream);

   lw_bool wrote_direct = lw_false;

   do
   {
      if (lwp_list_length (stream->exp_data_hooks) > 0)
         break;

      lw_stream next = 0;

      size_t direct_bytes = bytes;

      /* TODO : Skip to next direct instead of trying to read direct from 
       * intermediate streams?
       */

      if (!find_next_direct (stream, &next, &direct_bytes))
         break;

      lwp_trace ("Next direct from %p -> %p", stream, next);

      if ( (!next) || next->prev_direct)
         break;

      /* Only one non-transparent stream follows.  It may be possible to
       * shift the data directly (without having to read it first).
       *
       * Note that we don't have to check next's queue is empty, because
       * we're already being written (the queue is behind us).
       */

      next->prev_direct = stream;
      next->direct_bytes_left = direct_bytes;

      lwp_trace ("Attempting to read direct for %p -> %p", stream, next);

      if (lwp_stream_write_direct (next))
      {
         lwp_trace ("write_direct succeeded for %p -> %p", stream, next);

         wrote_direct = lw_true;
         break;
      }
      else
      {
         lwp_trace ("write_direct failed for %p -> %p", stream, next);

         next->prev_direct = 0;
      }

   } while (0);

   if (!wrote_direct)
      lw_stream_read (stream, bytes);

   /* Calling Push or Read may well run user code, or expire a link.  If this
    * causes the graph to re-expand, Read() will be called again and this one
    * must abort.
    */

   if (this_expand != graph->last_expand || graph->dead)
   {
      lwp_trace ("last_expand %d changed to %d after read; aborting",
            this_expand, graph->last_expand);

      return;
   }

   lwp_list_each (stream->next_expanded, link)
   {
      graph_read (graph, this_expand, link->to_exp, link->bytes_left);

      if (this_expand != graph->last_expand || graph->dead)
         return;
   }
}

void lwp_streamgraph_read (lwp_streamgraph graph)
{
   lw_stream root;

   ++ graph->user_count;

   int this_expand = graph->last_expand;

   lwp_trace ("streamgraph_read with last_expand %d", this_expand);

   lwp_list_each (graph->roots_expanded, root)
   {
      graph_read (graph, this_expand, root, -1);

      if (graph->last_expand != this_expand || graph->dead)
      {
         lwp_trace ("Abort streamgraph_read with last_expand %d", this_expand);

         if ((-- graph->user_count) == 0 && graph->dead)
            lwp_streamgraph_delete (graph);

         return;
      }
   }
}

static void clear_expanded (lw_stream stream)
{
   lwp_streamgraph_link link;

   lwp_list_each (stream->next_expanded, link)
   {
      if (link->to_exp)
         clear_expanded (link->to_exp);

      link->to_exp = link->from_exp = 0;
   }

   lwp_list_clear (stream->prev_expanded);
   lwp_list_clear (stream->next_expanded);

   stream->prev_direct = 0;

   lwp_list_clear (stream->exp_data_hooks);
}

void lwp_streamgraph_clear_expanded (lwp_streamgraph graph)
{
   lw_stream root;

   ++ graph->last_expand;

   lwp_list_each (graph->roots_expanded, root)
   {
      clear_expanded (root);
   }

   lwp_list_clear (graph->roots_expanded);
}


/* debugging crap */

static void print (lwp_streamgraph graph, lw_stream stream, int depth)
{
   lwp_streamgraph_link link;

   assert (stream->graph == graph);

   fprintf (stderr, "stream @ %p (" lwp_fmt_size " bytes, %d hooks)\n",
         stream, lw_stream_bytes_left (stream),
         lwp_list_length (stream->data_hooks));

   lwp_list_each (stream->next, link)
   {
      for (int i = 0; i < depth; ++ i)
         fprintf (stderr, "  ");

      fprintf (stderr, "link (" lwp_fmt_size ") %p ==> ",
            link->bytes_left, link);

      print (graph, link->to, depth + 1);
   }
}

static void print_expanded (lwp_streamgraph graph, lw_stream stream, int depth)
{
   lwp_streamgraph_link link;

   assert (stream->graph == graph);

   fprintf (stderr, "stream @ %p (" lwp_fmt_size " bytes, %d hooks)\n",
         stream, lw_stream_bytes_left (stream),
         lwp_list_length (stream->exp_data_hooks));

   lwp_list_each (stream->next_expanded, link)
   {
      for (int i = 0; i < depth; ++ i)
         fprintf (stderr, "  ");

      fprintf (stderr, "link (" lwp_fmt_size ") %p ==> ",
            link->bytes_left, link);

      assert (link->from_exp == stream);

      print (graph, link->to_exp, depth + 1);
   }
}

void lwp_streamgraph_print (lwp_streamgraph graph)
{
   lw_stream root;

   fprintf (stderr, "\n--- Graph %p (%d) ---\n\n",
         graph, lwp_list_length (graph->roots));

   lwp_list_each (graph->roots, root)
   {
      print (graph, root, 1);
   }

   fprintf (stderr, "\n--- Graph %p expanded (%d) ---\n\n",
         graph, lwp_list_length (graph->roots_expanded));

   lwp_list_each (graph->roots_expanded, root)
   {
      print_expanded (graph, root, 1);
   }

   fprintf (stderr, "\n");
}

