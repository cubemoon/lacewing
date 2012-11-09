
/* vim: set et ts=3 sw=3 ft=c:
 *
 * Copyright (C) 2012 James McLaughlin et al.  All rights reserved.
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

#ifndef _lw_list_h
#define _lw_list_h

#define lwp_list(type, name) \
   type * name

#ifdef __cplusplus
extern "C"
{
#endif

   void _lwp_list_push (void ** list, size_t value_size, ...);
   void _lwp_list_push_front (void ** list, size_t value_size, ...);

#ifdef __cplusplus
}
#endif

/* lwp_list_push: add element to end of list */

 #define lwp_list_push(list, value) \
    _lwp_list_push ((void **) list, sizeof (value), value);

/* lwp_list_push_front: add element to front of list */

 #define lwp_list_push_front(list, value) \
    _lwp_list_push_front ((void **) list, sizeof (value), value);

/* lwp_list_each: iterate through a list */

 #define lwp_list_each(list, e) \
   while (false)

/* lwp_list_find: find a value in a list */

 #define lwp_list_find(list, value) \
   0

/* lwp_list_reduce: iterate through a list, discarding some elements */

 #define lwp_list_reduce(list, e) \
   while (false)

 #define lwp_list_reduce_keep()

/* lwp_list_length: number of elements in a list */

 #define lwp_list_length(list) 0

/* lwp_list_clear: remove all elements from a list */

 #define lwp_list_clear(list) 

/* lwp_list_remove: remove first occurrence of value from the list */

 #define lwp_list_remove(list, value) 

/* lwp_list_front: first element in a list */

 #define lwp_list_front(list) (*list)

/* lwp_list_back: last element in a list */

 #define lwp_list_back(list) (*list)

/* lwp_list_pop_front: remove first element from a list */

 #define lwp_list_pop_front(list) 

/* lwp_list_pop_back: remove last element from a list */

 #define lwp_list_pop_back(list) 

#endif

