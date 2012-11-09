
/* vim: set et ts=4 sw=4 ft=cpp:
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

#ifndef LacewingIncluded
#define LacewingIncluded

#include <stdarg.h>
#include <stdio.h>

#ifndef _MSC_VER

    #ifndef __STDC_FORMAT_MACROS
        #define __STDC_FORMAT_MACROS
    #endif

    #include <inttypes.h>
    
    #define lw_i64   int64_t
    #define lw_iptr  intptr_t
    #define lw_i32   int32_t
    #define lw_i16   int16_t
    #define lw_i8    int8_t

    #define lw_PRId64 PRId64
    #define lw_PRIu64 PRIu64

#else

    #ifdef _WIN64
        #define lw_iptr __int64
    #else
        #define lw_iptr __int32
    #endif

    #define lw_i64  __int64
    #define lw_i32  __int32
    #define lw_i16  __int16
    #define lw_i8   __int8

    #define lw_PRId64 "I64d"
    #define lw_PRIu64 "I64u"
    
#endif

#ifndef _WIN32
    #ifndef lw_callback
        #define lw_callback
    #endif

    #ifndef lw_import
        #define lw_import
    #endif
#else

    /* For the definition of HANDLE and OVERLAPPED (used by lw_pump) */

    #ifndef _INC_WINDOWS
        #include <winsock2.h>
        #include <windows.h>
    #endif

    #define lw_callback __cdecl

#endif

#ifndef lw_import
    #define lw_import __declspec (dllimport)
#endif

typedef lw_i8 lw_bool;

#define lw_true   ((lw_bool) 1)
#define lw_false  ((lw_bool) 0)

#ifdef _WIN32
    typedef HANDLE lw_fd;
#else
    typedef int lw_fd;
#endif

#ifndef __cplusplus

  typedef struct lw_thread            * lw_thread;
  typedef struct lw_addr              * lw_addr;
  typedef struct lw_filter            * lw_filter;
  typedef struct lw_pump              * lw_pump;
  typedef struct lw_pump_watch        * lw_pump_watch;
  typedef struct lw_stream            * lw_stream;
  typedef struct lw_timer             * lw_timer;
  typedef struct lw_sync              * lw_sync;
  typedef struct lw_sync_lock         * lw_sync_lock;
  typedef struct lw_event             * lw_event;
  typedef struct lw_error             * lw_error;
  typedef struct lw_server            * lw_server;
  typedef struct lw_udp               * lw_udp;
  typedef struct lw_flashpolicy       * lw_flashpolicy;
  typedef struct lw_ws                * lw_ws;
  typedef struct lw_ws_req_hdr        * lw_ws_req_hdr;
  typedef struct lw_ws_req_param      * lw_ws_req_param;
  typedef struct lw_ws_req_cookie     * lw_ws_req_cookie;
  typedef struct lw_ws_req_ssnitem    * lw_ws_req_ssnitem;
  typedef struct lw_ws_upload         * lw_ws_upload;
  typedef struct lw_ws_upload_hdr     * lw_ws_upload_hdr;

#else

  /* We can't use the same typedef trick in C++, so here's some nasty macros
   * instead.  (Fortunately, those using a C++ compiler probably won't be
   * interested in the C API anyway.)
   */

  #define lw_thread           struct lw_thread *
  #define lw_addr             struct lw_addr *
  #define lw_filter           struct lw_filter *
  #define lw_pump             struct lw_pump *
  #define lw_pump_watch       struct lw_pump_watch *
  #define lw_stream           struct lw_stream *
  #define lw_timer            struct lw_timer *
  #define lw_sync             struct lw_sync *
  #define lw_sync_lock        struct lw_sync_lock *
  #define lw_event            struct lw_event *
  #define lw_error            struct lw_error *
  #define lw_server           struct lw_server *
  #define lw_udp              struct lw_udp *
  #define lw_flashpolicy      struct lw_flashpolicy *
  #define lw_ws               struct lw_ws *
  #define lw_ws_req_hdr       struct lw_ws_req_hdr *
  #define lw_ws_req_param     struct lw_ws_req_param *
  #define lw_ws_req_cookie    struct lw_ws_req_cookie *
  #define lw_ws_req_ssnitem   struct lw_ws_req_ssnitem *
  #define lw_ws_upload        struct lw_ws_upload *
  #define lw_ws_upload_hdr    struct lw_ws_upload_hdr *

extern "C"
{
#endif

lw_import    const char* lw_version                  ();
lw_import        lw_i64  lw_file_last_modified       (const char * filename);
lw_import       lw_bool  lw_file_exists              (const char * filename);
lw_import        size_t  lw_file_size                (const char * filename);
lw_import       lw_bool  lw_path_exists              (const char * filename);
lw_import          void  lw_temp_path                (char * buffer);
lw_import    const char* lw_guess_mime_type          (const char * filename);
lw_import          void  lw_md5                      (char * output, const char * input, size_t length);
lw_import          void  lw_md5_hex                  (char * output, const char * input, size_t length);
lw_import          void  lw_sha1                     (char * output, const char * input, size_t length);
lw_import          void  lw_sha1_hex                 (char * output, const char * input, size_t length);
lw_import          void  lw_dump                     (const char * buffer, size_t size);
lw_import       lw_bool  lw_random                   (char * buffer, size_t size);

/* Thread */

  lw_import      lw_thread  lw_thread_new      (const char * name, void * function);
  lw_import           void  lw_thread_delete   (lw_thread);
  lw_import           void  lw_thread_start    (lw_thread, void * parameter);
  lw_import        lw_bool  lw_thread_started  (lw_thread);
  lw_import           long  lw_thread_join     (lw_thread);

/* Address */

  lw_import        lw_addr  lw_addr_new          (const char * hostname, const char * service);
  lw_import        lw_addr  lw_addr_new_port     (const char * hostname, long port);
  lw_import        lw_addr  lw_addr_new_ex       (const char * hostname, const char * service, long hints);
  lw_import        lw_addr  lw_addr_new_port_ex  (const char * hostname, long port, long hints);
  lw_import        lw_addr  lw_addr_copy         (lw_addr);
  lw_import           void  lw_addr_delete       (lw_addr);
  lw_import           long  lw_addr_port         (lw_addr);
  lw_import           void  lw_addr_set_port     (lw_addr, long port);
  lw_import        lw_bool  lw_addr_is_ready     (lw_addr);
  lw_import        lw_bool  lw_addr_is_ipv6      (lw_addr);
  lw_import        lw_bool  lw_addr_is_equal     (lw_addr, lw_addr);
  lw_import     const char* lw_addr_tostring     (lw_addr);

/* Filter */

  lw_import      lw_filter  lw_filter_new                ();
  lw_import           void  lw_filter_delete             (lw_filter);
  lw_import      lw_filter  lw_filter_copy               (lw_filter);
  lw_import           void  lw_filter_set_remote         (lw_filter, lw_addr);
  lw_import        lw_addr  lw_filter_get_remote         (lw_filter);
  lw_import           void  lw_filter_set_local          (lw_filter, lw_addr);
  lw_import        lw_addr  lw_filter_get_local          (lw_filter);
  lw_import           void  lw_filter_set_local_port     (lw_filter, long port);
  lw_import           long  lw_filter_get_local_port     (lw_filter);
  lw_import           void  lw_filter_set_remote_port    (lw_filter, long port);
  lw_import           long  lw_filter_get_remote_port    (lw_filter);
  lw_import           void  lw_filter_set_reuse          (lw_filter);
  lw_import        lw_bool  lw_filter_is_reuse_set       (lw_filter);
  lw_import           void  lw_filter_set_ipv6           (lw_filter, lw_bool);
  lw_import        lw_bool  lw_filter_is_ipv6            (lw_filter);

/* Pump */

  lw_import           void  lw_pump_delete               (lw_pump);
  lw_import           void  lw_pump_add_user             (lw_pump);
  lw_import           void  lw_pump_remove_user          (lw_pump);
  lw_import        lw_bool  lw_pump_in_use               (lw_pump);
  lw_import           void  lw_pump_remove               (lw_pump, lw_pump_watch);
  lw_import           void  lw_pump_post                 (lw_pump, void * fn, void * param);
  lw_import        lw_bool  lw_pump_is_eventpump         (lw_pump);

  #ifdef _WIN32

    typedef void (lw_callback * lw_pump_callback)
        (void * tag, OVERLAPPED *, unsigned int bytes, int error);

    lw_import lw_pump_watch lw_pump_add
                          (lw_pump, HANDLE, void * tag, lw_pump_callback);

    lw_import void lw_pump_update_callbacks
                          (lw_pump, lw_pump_watch *, void * tag, lw_pump_callback);
  #else

    typedef void (lw_callback * lw_pump_callback) (void * tag);

    lw_import lw_pump_watch lw_pump_add (lw_pump, int FD, void * tag,
                                          lw_pump_callback onReadReady,
                                          lw_pump_callback onWriteReady,
                                          lw_bool edge_triggered);

    lw_import void lw_pump_update_callbacks (lw_pump, lw_pump_watch, void * tag,
                                              lw_pump_callback onReadReady,
                                              lw_pump_callback onWriteReady,
                                              lw_bool edge_triggered);
  #endif

/* EventPump */

  lw_import        lw_pump  lw_eventpump_new                  ();
  lw_import           void  lw_eventpump_tick                 (lw_pump);
  lw_import           void  lw_eventpump_start_event_loop     (lw_pump);
  lw_import           void  lw_eventpump_start_sleepy_ticking (lw_pump, void (lw_callback * on_tick_needed) (lw_pump));
  lw_import           void  lw_eventpump_post_eventloop_exit  (lw_pump);

/* Stream */

  lw_import      void  lw_stream_delete                (lw_stream);
  lw_import   lw_bool  lw_stream_valid                 (lw_stream);
  lw_import    size_t  lw_stream_bytes_left            (lw_stream);
  lw_import      void  lw_stream_read                  (lw_stream, size_t bytes);
  lw_import      void  lw_stream_begin_queue           (lw_stream);
  lw_import    size_t  lw_stream_queued                (lw_stream);
  lw_import      void  lw_stream_end_queue             (lw_stream);
  lw_import      void  lw_stream_end_queue_hb          (lw_stream, int head_buffers, const char ** buffers, size_t * lengths);
  lw_import      void  lw_stream_write                 (lw_stream, const char * buffer, size_t length);
  lw_import      void  lw_stream_write_text            (lw_stream, const char * buffer);
  lw_import      void  lw_stream_writef                (lw_stream, const char * format, ...);
  lw_import      void  lw_stream_write_stream          (lw_stream, lw_stream src, size_t size, lw_bool delete_when_finished);
  lw_import      void  lw_stream_write_file            (lw_stream, const char * filename);
  lw_import      void  lw_stream_retry                 (lw_stream, int when);
  lw_import      void  lw_stream_add_filter_upstream   (lw_stream, lw_stream filter, lw_bool delete_with_stream, lw_bool close_together);
  lw_import      void  lw_stream_add_filter_downstream (lw_stream, lw_stream filter, lw_bool delete_with_stream, lw_bool close_together);
  lw_import   lw_bool  lw_stream_is_transparent        (lw_stream);
  lw_import   lw_bool  lw_stream_close                 (lw_stream, lw_bool immediate);

  #define lw_stream_retry_now  1
  #define lw_stream_retry_never  2
  #define lw_stream_retry_more_data  3

  typedef void (lw_callback * lw_stream_handler_data)
      (lw_stream, void * tag, const char * buffer, size_t length);

  lw_import void lw_stream_add_handler_data (lw_stream, lw_stream_handler_data, void * tag);
  lw_import void lw_stream_remove_handler_data (lw_stream, lw_stream_handler_data, void * tag);

  typedef void (lw_callback * lw_stream_handler_close) (lw_stream, void * tag);

  lw_import void lw_stream_add_handler_close (lw_stream, lw_stream_handler_close, void * tag);
  lw_import void lw_stream_remove_handler_close (lw_stream, lw_stream_handler_close, void * tag);

  /* For stream implementors */

   typedef struct lw_stream_def
   {
       size_t (* sink_data) (lw_stream, const char * buffer, size_t size);
       size_t (* sink_stream) (lw_stream, lw_stream source, size_t size);

       void (* retry) (lw_stream, int when);

       lw_bool (* is_transparent) (lw_stream);
       lw_bool (* close) (lw_stream, lw_bool immediate);
 
       size_t (* bytes_left) (lw_stream);
       void (* read) (lw_stream, size_t bytes);

       lw_bool (* cleanup) (lw_stream);

   } lw_stream_def;

   lw_import lw_stream lw_stream_new (const lw_stream_def *, lw_pump);
   lw_import const lw_stream_def * lw_stream_get_def (lw_stream);

   lw_import void lw_stream_data (lw_stream, const char * buffer, size_t size);

/* FDStream */

  lw_import  lw_stream  lw_fdstream_new         (lw_pump);
  lw_import       void  lw_fdstream_set_fd      (lw_stream, lw_fd fd, lw_pump_watch watch, lw_bool auto_close);
  lw_import       void  lw_fdstream_cork        (lw_stream);
  lw_import       void  lw_fdstream_uncork      (lw_stream);
  lw_import       void  lw_fdstream_nagle       (lw_stream, lw_bool nagle);

/* File */

  lw_import lw_stream lw_file_new (lw_pump);

  lw_import lw_stream lw_file_new_open
      (lw_pump, const char * filename, const char * mode);

  lw_import lw_bool lw_file_open
      (lw_stream file, const char * filename, const char * mode);

/* Pipe */
  
  lw_import  lw_stream  lw_pipe_new  (lw_pump);

/* Timer */
  
  lw_import       lw_timer  lw_timer_new                  ();
  lw_import           void  lw_timer_delete               (lw_timer);
  lw_import           void  lw_timer_start                (lw_timer, long milliseconds);
  lw_import        lw_bool  lw_timer_started              (lw_timer);
  lw_import           void  lw_timer_stop                 (lw_timer);
  lw_import           void  lw_timer_force_tick           (lw_timer);

  typedef void (lw_callback * lw_timer_handler_tick) (lw_timer);
  lw_import void lw_timer_ontick (lw_timer, lw_timer_handler_tick);

/* Sync */

  lw_import        lw_sync  lw_sync_new                  ();
  lw_import           void  lw_sync_delete               (lw_sync);
  lw_import   lw_sync_lock  lw_sync_lock_new             (lw_sync);
  lw_import           void  lw_sync_lock_delete          (lw_sync_lock);
  lw_import           void  lw_sync_lock_release         (lw_sync_lock);

/* Event */

  lw_import       lw_event  lw_event_new                 ();
  lw_import           void  lw_event_delete              (lw_event);
  lw_import           void  lw_event_signal              (lw_event);
  lw_import           void  lw_event_unsignal            (lw_event);
  lw_import        lw_bool  lw_event_signalled           (lw_event);
  lw_import           void  lw_event_wait                (lw_event, long milliseconds);

/* Error */

  lw_import       lw_error  lw_error_new                 ();
  lw_import           void  lw_error_delete              (lw_error);
  lw_import           void  lw_error_add                 (lw_error, long);
  lw_import           void  lw_error_addf                (lw_error, const char * format, ...);
  lw_import     const char* lw_error_tostring            (lw_error);
  lw_import       lw_error  lw_error_clone               (lw_error);

/* Client */

  lw_import      lw_stream  lw_client_new                (lw_pump);
  lw_import           void  lw_client_connect            (lw_stream, const char * host, long port);
  lw_import           void  lw_client_connect_addr       (lw_stream, lw_addr);
  lw_import           void  lw_client_disconnect         (lw_stream);
  lw_import        lw_bool  lw_client_connected          (lw_stream);
  lw_import        lw_bool  lw_client_connecting         (lw_stream);
  lw_import        lw_addr  lw_client_server_addr        (lw_stream);
  
  typedef void (lw_callback * lw_client_handler_connect) (lw_stream);
  lw_import void lw_client_onconnect (lw_stream, lw_client_handler_connect);

  typedef void (lw_callback * lw_client_handler_disconnect) (lw_stream);
  lw_import void lw_client_ondisconnect (lw_stream, lw_client_handler_disconnect);

  typedef void (lw_callback * lw_client_handler_receive) (lw_stream, const char * buffer, long size);
  lw_import void lw_client_onreceive (lw_stream, lw_client_handler_receive);

  typedef void (lw_callback * lw_client_handler_error) (lw_stream, lw_error);
  lw_import void lw_client_onerror (lw_stream, lw_client_handler_error);

/* Server */

  lw_import        lw_server  lw_server_new                      (lw_pump);
  lw_import             void  lw_server_delete                   (lw_server);
  lw_import             void  lw_server_host                     (lw_server, long port);
  lw_import             void  lw_server_host_filter              (lw_server, lw_filter);
  lw_import             void  lw_server_unhost                   (lw_server);
  lw_import          lw_bool  lw_server_hosting                  (lw_server);
  lw_import             long  lw_server_port                     (lw_server);
  lw_import          lw_bool  lw_server_load_cert_file           (lw_server, const char * filename, const char * passphrase);
  lw_import          lw_bool  lw_server_load_sys_cert            (lw_server, const char * store_name, const char * common_name, const char * location);
  lw_import          lw_bool  lw_server_cert_loaded              (lw_server);
  lw_import          lw_addr  lw_server_client_address           (lw_stream client);
  lw_import        lw_stream  lw_server_client_next              (lw_stream client);

  typedef void (lw_callback * lw_server_handler_connect) (lw_server, lw_stream client);
  lw_import void lw_server_onconnect (lw_server, lw_server_handler_connect);

  typedef void (lw_callback * lw_server_handler_disconnect) (lw_server, lw_stream * client);
  lw_import void lw_server_ondisconnect (lw_server, lw_server_handler_disconnect);

  typedef void (lw_callback * lw_server_handler_receive) (lw_server, lw_stream * client, const char * buffer, size_t size);
  lw_import void lw_server_onreceive (lw_server, lw_server_handler_receive);
  
  typedef void (lw_callback * lw_server_handler_error) (lw_server, lw_error);
  lw_import void lw_server_onerror (lw_server, lw_server_handler_error);

/* UDP */

  lw_import         lw_udp* lw_udp_new                   (lw_pump);
  lw_import           void  lw_udp_delete                (lw_udp);
  lw_import           void  lw_udp_host                  (lw_udp, long port);
  lw_import           void  lw_udp_host_filter           (lw_udp, lw_filter);
  lw_import           void  lw_udp_host_addr             (lw_udp, lw_addr);
  lw_import        lw_bool  lw_udp_hosting               (lw_udp);
  lw_import           void  lw_udp_unhost                (lw_udp);
  lw_import           long  lw_udp_port                  (lw_udp);
  lw_import           void  lw_udp_write                 (lw_udp, lw_addr, const char * buffer, size_t size);

  typedef void (lw_callback * lw_udp_handler_receive)(lw_udp, lw_addr, const char * buffer, size_t size);
  lw_import void lw_udp_onreceive (lw_udp, lw_udp_handler_receive);

  typedef void (lw_callback * lw_udp_handler_error) (lw_udp, lw_error);
  lw_import void lw_udp_onerror (lw_udp, lw_udp_handler_error);

/* FlashPolicy */

  lw_import  lw_flashpolicy  lw_flashpolicy_new           (lw_pump);
  lw_import            void  lw_flashpolicy_delete        (lw_flashpolicy);
  lw_import            void  lw_flashpolicy_host          (lw_flashpolicy, const char * filename);
  lw_import            void  lw_flashpolicy_host_filter   (lw_flashpolicy, const char * filename, lw_filter);
  lw_import            void  lw_flashpolicy_unhost        (lw_flashpolicy);
  lw_import         lw_bool  lw_flashpolicy_hosting       (lw_flashpolicy);

  typedef void (lw_callback * lw_flashpolicy_handler_error) (lw_flashpolicy, lw_error);
  lw_import void lw_flashpolicy_onerror (lw_flashpolicy, lw_flashpolicy_handler_error);

/* Webserver */

  lw_import              lw_ws  lw_ws_new                    (lw_pump);
  lw_import               void  lw_ws_delete                 (lw_ws);
  lw_import               void  lw_ws_host                   (lw_ws, long port);
  lw_import               void  lw_ws_host_secure            (lw_ws, long port);
  lw_import               void  lw_ws_host_filter            (lw_ws, lw_filter);
  lw_import               void  lw_ws_host_secure_filter     (lw_ws, lw_filter);
  lw_import               void  lw_ws_unhost                 (lw_ws);
  lw_import               void  lw_ws_unhost_secure          (lw_ws);
  lw_import            lw_bool  lw_ws_hosting                (lw_ws);
  lw_import            lw_bool  lw_ws_hosting_secure         (lw_ws);
  lw_import               long  lw_ws_port                   (lw_ws);
  lw_import               long  lw_ws_port_secure            (lw_ws);
  lw_import            lw_bool  lw_ws_load_cert_file         (lw_ws, const char * filename, const char * passphrase);
  lw_import            lw_bool  lw_ws_load_sys_cert          (lw_ws, const char * store_name, const char * common_name, const char * location);
  lw_import            lw_bool  lw_ws_cert_loaded            (lw_ws);
  lw_import               void  lw_ws_close_session          (lw_ws, const char * id);
  lw_import               void  lw_ws_enable_manual_finish   (lw_ws);
  lw_import               long  lw_ws_idle_timeout           (lw_ws);
  lw_import               void  lw_ws_set_idle_timeout       (lw_ws, long seconds);  
  lw_import            lw_addr* lw_ws_req_addr               (lw_stream req);
  lw_import            lw_bool  lw_ws_req_secure             (lw_stream req);
  lw_import         const char* lw_ws_req_url                (lw_stream req);
  lw_import         const char* lw_ws_req_hostname           (lw_stream req);
  lw_import               void  lw_ws_req_disconnect         (lw_stream req); 
  lw_import               void  lw_ws_req_set_redirect       (lw_stream req, const char * url);
  lw_import               void  lw_ws_req_set_status         (lw_stream req, long code, const char * message);
  lw_import               void  lw_ws_req_set_mime_type      (lw_stream req, const char * mime_type);
  lw_import               void  lw_ws_req_set_mime_type_ex   (lw_stream req, const char * mime_type, const char * charset);
  lw_import               void  lw_ws_req_guess_mime_type    (lw_stream req, const char * filename);
  lw_import               void  lw_ws_req_finish             (lw_stream req);
  lw_import             lw_i64  lw_ws_req_last_modified      (lw_stream req);
  lw_import               void  lw_ws_req_set_last_modified  (lw_stream req, lw_i64);
  lw_import               void  lw_ws_req_set_unmodified     (lw_stream req);
  lw_import               void  lw_ws_req_set_header         (lw_stream req, const char * name, const char * value);
  lw_import         const char* lw_ws_req_header             (lw_stream req, const char * name);
  lw_import      lw_ws_req_hdr  lw_ws_req_first_header       (lw_stream req);
  lw_import         const char* lw_ws_req_hdr_name           (lw_ws_req_hdr *);
  lw_import         const char* lw_ws_req_hdr_value          (lw_ws_req_hdr *);
  lw_import      lw_ws_req_hdr  lw_ws_req_hdr_next           (lw_ws_req_hdr *);
  lw_import    lw_ws_req_param  lw_ws_req_first_GET          (lw_stream req);
  lw_import    lw_ws_req_param  lw_ws_req_first_POST         (lw_stream req);
  lw_import         const char* lw_ws_req_param_name         (lw_ws_req_param *);
  lw_import         const char* lw_ws_req_param_value        (lw_ws_req_param *);
  lw_import    lw_ws_req_param  lw_ws_req_param_next         (lw_ws_req_param *);
  lw_import   lw_ws_req_cookie  lw_ws_req_first_cookie       (lw_stream req);
  lw_import         const char* lw_ws_req_cookie_name        (lw_ws_req_cookie *);
  lw_import         const char* lw_ws_req_cookie_value       (lw_ws_req_cookie *);
  lw_import   lw_ws_req_cookie  lw_ws_req_cookie_next        (lw_ws_req_cookie *);
  lw_import               void  lw_ws_req_set_cookie         (lw_stream req, const char * name, const char * value);
  lw_import               void  lw_ws_req_set_cookie_ex      (lw_stream req, const char * name, const char * value, const char * attributes);
  lw_import         const char* lw_ws_req_get_cookie         (lw_stream req, const char * name);
  lw_import         const char* lw_ws_req_session_id         (lw_stream req);
  lw_import               void  lw_ws_req_session_write      (lw_stream req, const char * name, const char * value);
  lw_import         const char* lw_ws_req_session_read       (lw_stream req, const char * name);
  lw_import               void  lw_ws_req_session_close      (lw_stream req);
  lw_import  lw_ws_req_ssnitem  lw_ws_req_first_session_item (lw_stream req);
  lw_import         const char* lw_ws_req_ssnitem_name       (lw_ws_req_ssnitem);
  lw_import         const char* lw_ws_req_ssnitem_value      (lw_ws_req_ssnitem);
  lw_import  lw_ws_req_ssnitem  lw_ws_req_ssnitem_next       (lw_ws_req_ssnitem);
  lw_import         const char* lw_ws_req_GET                (lw_stream req, const char * name);
  lw_import         const char* lw_ws_req_POST               (lw_stream req, const char * name);
  lw_import         const char* lw_ws_req_body               (lw_stream req);
  lw_import               void  lw_ws_req_disable_cache      (lw_stream req);
  lw_import               long  lw_ws_req_idle_timeout       (lw_stream req);
  lw_import               void  lw_ws_req_set_idle_timeout   (lw_stream req, long seconds);  
/*lw_import               void  lw_ws_req_enable_dl_resuming (lw_stream req);
  lw_import             lw_i64  lw_ws_req_reqrange_begin     (lw_stream req);
  lw_import             lw_i64  lw_ws_req_reqrange_end       (lw_stream req);
  lw_import               void  lw_ws_req_set_outgoing_range (lw_stream req, lw_i64 begin, lw_i64 end);*/
  lw_import         const char* lw_ws_upload_form_el_name    (lw_ws_upload);
  lw_import         const char* lw_ws_upload_filename        (lw_ws_upload);
  lw_import         const char* lw_ws_upload_header          (lw_ws_upload, const char * name);
  lw_import               void  lw_ws_upload_set_autosave    (lw_ws_upload);
  lw_import         const char* lw_ws_upload_autosave_fname  (lw_ws_upload);
  lw_import   lw_ws_upload_hdr  lw_ws_upload_first_header    (lw_ws_upload);
  lw_import         const char* lw_ws_upload_hdr_name        (lw_ws_upload_hdr);
  lw_import         const char* lw_ws_upload_hdr_value       (lw_ws_upload_hdr);
  lw_import   lw_ws_upload_hdr  lw_ws_upload_hdr_next        (lw_ws_upload_hdr);

  typedef void (lw_callback * lw_ws_handler_get) (lw_ws, lw_stream req);
  lw_import void lw_ws_onget (lw_ws, lw_ws_handler_get);

  typedef void (lw_callback * lw_ws_handler_post) (lw_ws, lw_stream req);
  lw_import void lw_ws_onpost (lw_ws, lw_ws_handler_post);

  typedef void (lw_callback * lw_ws_handler_head) (lw_ws, lw_stream req);
  lw_import void lw_ws_onhead (lw_ws, lw_ws_handler_head);

  typedef void (lw_callback * lw_ws_handler_error) (lw_ws, lw_error *);
  lw_import void lw_ws_onerror (lw_ws, lw_ws_handler_error);

  typedef void (lw_callback * lw_ws_handler_disconnect) (lw_ws, lw_stream req);
  lw_import void lw_ws_ondisconnect (lw_ws, lw_ws_handler_disconnect);

  typedef void (lw_callback * lw_ws_handler_upload_start) (lw_ws, lw_stream req, lw_ws_upload);
  lw_import void lw_ws_onuploadstart (lw_ws, lw_ws_handler_upload_start);

  typedef void (lw_callback * lw_ws_handler_upload_chunk) (lw_ws, lw_stream req, lw_ws_upload, const char * buffer, size_t size);
  lw_import void lw_ws_onuploadchunk (lw_ws, lw_ws_handler_upload_chunk);

  typedef void (lw_callback * lw_ws_handler_upload_done) (lw_ws, lw_stream req, lw_ws_upload);
  lw_import void lw_ws_onuploaddone (lw_ws, lw_ws_handler_upload_done);

  typedef void (lw_callback * lw_ws_handler_upload_post) (lw_ws, lw_stream req, lw_ws_upload uploads [], long upload_count);
  lw_import void lw_ws_onuploadpost (lw_ws, lw_ws_handler_upload_post);

#ifdef __cplusplus
}

#define lacewing_class \
    struct Internal;  Internal * internal; 

#define lacewing_class_tag \
    struct Internal;  Internal * internal;  void * Tag;

namespace Lacewing
{

struct Error
{
    lacewing_class_tag;

    lw_import  Error ();
    lw_import ~Error ();
    
    lw_import void Add (const char * Format, ...);
    lw_import void Add (int);
    lw_import void Add (const char * Format, va_list);

    lw_import int Size ();

    lw_import const char * ToString ();
    lw_import operator const char * ();

    lw_import Lacewing::Error * Clone ();
};

struct Event
{
    lacewing_class_tag;

    lw_import Event ();
    lw_import ~Event ();

    lw_import void Signal ();
    lw_import void Unsignal ();
    
    lw_import bool Signalled ();

    lw_import bool Wait (int Timeout = -1);
};

struct Pump
{
private:

    lacewing_class_tag;

public:

    lw_import Pump ();
    lw_import virtual ~Pump ();

    lw_import void AddUser ();
    lw_import void RemoveUser ();

    lw_import bool InUse ();

    struct Watch;

    #ifdef _WIN32

        typedef void (* Callback)
            (void * Tag, OVERLAPPED *, unsigned int BytesTransferred, int Error);

        virtual Watch * Add (HANDLE, void * tag, Callback) = 0;

        virtual void UpdateCallbacks (Watch *, void * tag, Callback) = 0;

    #else

        typedef void (* Callback) (void * Tag);

        virtual Watch * Add
            (int FD, void * Tag, Callback onReadReady,
                Callback onWriteReady = 0, bool edge_triggered = true) = 0;

        virtual void UpdateCallbacks
            (Watch *, void * tag, Callback onReadReady,
                Callback onWriteReady = 0, bool edge_triggered = true) = 0;

    #endif
 
    virtual void Remove (Watch *) = 0;

    virtual void Post (void * Function, void * Parameter = 0) = 0;   

    lw_import virtual bool IsEventPump ();
};

struct EventPump : public Pump
{
    lacewing_class_tag;

    lw_import  EventPump (int MaxHint = 1024);
    lw_import ~EventPump ();

    lw_import Lacewing::Error * Tick ();
    lw_import Lacewing::Error * StartEventLoop ();

    lw_import Lacewing::Error * StartSleepyTicking
        (void (lw_callback * onTickNeeded) (Lacewing::EventPump &EventPump));

    lw_import void PostEventLoopExit ();

    #ifdef _WIN32

        lw_import Watch * Add (HANDLE, void * tag, Callback);

        lw_import void UpdateCallbacks (Watch *, void * tag, Callback);

    #else

        lw_import Watch * Add
            (int FD, void * Tag, Callback onReadReady,
                Callback onWriteReady = 0, bool edge_triggered = true);

        lw_import void UpdateCallbacks
            (Watch *, void * tag, Callback onReadReady,
                Callback onWriteReady = 0, bool edge_triggered = true);

    #endif

    lw_import void Remove (Watch *);

    lw_import void Post (void * Function, void * Parameter = 0);

    lw_import bool IsEventPump ();
};

struct Thread
{
    lacewing_class_tag;

    lw_import   Thread (const char * Name, void * Function);
    lw_import ~ Thread ();

    lw_import void Start (void * Parameter = 0);
    lw_import bool Started ();

    lw_import int Join ();
};

struct Timer
{
    lacewing_class_tag;

    lw_import  Timer (Pump &);
    lw_import ~Timer ();

    lw_import void Start    (int Milliseconds);
    lw_import void Stop     ();
    lw_import bool Started  ();
    
    lw_import void ForceTick ();
    
    typedef void (lw_callback * HandlerTick) (Lacewing::Timer &Timer);
    lw_import void onTick (HandlerTick);
};

struct Sync
{
    lacewing_class_tag;

    lw_import  Sync ();
    lw_import ~Sync ();

    struct Lock
    {
        void * internal;

        lw_import  Lock (Lacewing::Sync &);
        lw_import ~Lock ();
        
        lw_import void Release ();
    };
};

struct Stream
{
    lacewing_class;

    lw_import static Stream &New ();
    lw_import static Stream &New (Pump &);

    lw_import virtual ~ Stream ();

    typedef void (lw_callback * HandlerData)
        (Stream &, void * tag, const char * buffer, size_t size);

    lw_import void AddHandlerData (HandlerData, void * tag = 0);
    lw_import void RemoveHandlerData (HandlerData, void * tag = 0);

    typedef void (lw_callback * HandlerClose) (Stream &, void * Tag);

    lw_import void AddHandlerClose (HandlerClose, void * tag);
    lw_import void RemoveHandlerClose (HandlerClose, void * tag);

    lw_import virtual size_t BytesLeft (); /* if -1, Read() does nothing */
    lw_import virtual void Read (size_t bytes = -1); /* -1 = until EOF */

    lw_import void BeginQueue ();
    lw_import size_t Queued ();

    /* When EndQueue is called, one or more head buffers may optionally be
     * written _before_ the queued data.  This is used for e.g. including HTTP
     * headers before the (already buffered) response body.
     */

    lw_import void EndQueue ();    

    lw_import void EndQueue
        (int head_buffers, const char ** buffers, size_t * lengths);

    lw_import void Write
        (const char * buffer, size_t size = -1);

    lw_import size_t WritePartial
        (const char * buffer, size_t size = -1);

    inline Stream &operator << (char * s)                   
    {   Write (s);                                      
        return *this;                                   
    }                                                   

    inline Stream &operator << (const char * s)             
    {   Write (s);                                      
        return *this;                                   
    }                                                   

    inline Stream &operator << (lw_i64 v)                   
    {   char buffer [128];
        sprintf (buffer, "%" lw_PRId64, v);
        Write (buffer);
        return *this;                                   
    }                                                   

    lw_import void Write
        (Stream &, size_t size = -1, bool delete_when_finished = false);

    lw_import void WriteFile (const char * filename);

    lw_import void AddFilterUpstream
      (Stream &, bool delete_with_stream = false, bool close_together = false);

    lw_import void AddFilterDownstream
      (Stream &, bool delete_with_stream = false, bool close_together = false);

    lw_import virtual bool IsTransparent ();

    lw_import virtual bool Close (bool immediate = false);

    /* Since we don't compile with RTTI (and this is the only place it would be needed) */

    lw_import virtual void * Cast (void * type);

protected:
 
    lw_import void Data (const char * buffer, size_t size);

    lw_import virtual void Retry (int when = lw_stream_retry_now);

    lw_import virtual size_t Put (const char * buffer, size_t size) = 0;

    lw_import virtual size_t Put (Stream &, size_t size);
};
              
struct Pipe : public Stream
{
    lw_import Pipe ();
    lw_import Pipe (Lacewing::Pump &);

    lw_import ~ Pipe ();

    lw_import size_t Put (const char * buffer, size_t size);

    lw_import bool IsTransparent ();
};

/* TODO : Seek method? */

struct FDStream : public Stream
{
private:

    lacewing_class;

public:

    using Stream::Write;

    lw_import FDStream (Lacewing::Pump &);
    lw_import virtual ~ FDStream ();

    lw_import void SetFD
        (lw_fd, Pump::Watch * watch = 0, bool auto_close = false);

    lw_import bool Valid ();

    lw_import virtual size_t BytesLeft ();
    lw_import virtual void Read (size_t Bytes = -1);

    lw_import void Cork ();
    lw_import void Uncork ();    

    lw_import void Nagle (bool);

    lw_import bool Close (bool immediate = false);

    lw_import virtual void * Cast (void *);

protected:

    lw_import size_t Put (const char * buffer, size_t size);
    lw_import size_t Put (Stream &, size_t size);
};

struct File : public FDStream
{
    lacewing_class_tag;

    lw_import File
        (Lacewing::Pump &);

    lw_import File
        (Lacewing::Pump &, const char * filename, const char * mode = "rb");
    
    lw_import virtual ~ File ();

    lw_import bool Open (const char * filename, const char * mode = "rb");
    lw_import bool OpenTemp ();

    lw_import const char * Name ();
};

struct Address
{  
    lacewing_class_tag;

    const static int HINT_TCP   = 1;
    const static int HINT_UDP   = 2;
    const static int HINT_IPv4  = 4;

    lw_import Address (Address &);
    lw_import Address (const char * Hostname, int Port = 0, int Hints = 0);
    lw_import Address (const char * Hostname, const char * Service, int Hints = 0);

    lw_import ~ Address ();

    lw_import  int Port ();
    lw_import void Port (int);
    
    lw_import bool IPv6 ();

    lw_import bool Ready ();
    lw_import Lacewing::Error * Resolve ();

    lw_import const char * ToString  ();
    lw_import operator const char *  ();

    lw_import bool operator == (Address &);
    lw_import bool operator != (Address &);
};

struct Filter
{
    lacewing_class_tag;

    lw_import Filter ();
    lw_import Filter (Filter &);

    lw_import ~Filter ();

    lw_import void Local (Lacewing::Address *);    
    lw_import void Remote (Lacewing::Address *);

    lw_import Lacewing::Address * Local ();    
    lw_import Lacewing::Address * Remote (); 

    lw_import void LocalPort (int Port);   
    lw_import  int LocalPort ();    

    lw_import void RemotePort (int Port);   
    lw_import  int RemotePort ();    

    lw_import void Reuse (bool Enabled);
    lw_import bool Reuse ();

    lw_import void IPv6 (bool Enabled);
    lw_import bool IPv6 ();
};

struct Client : public FDStream
{
    lacewing_class_tag;

    lw_import  Client (Pump &);
    lw_import ~Client ();

    lw_import void Connect (const char * Host, int Port);
    lw_import void Connect (Address &);

    lw_import bool Connected ();
    lw_import bool Connecting ();
    
    lw_import Lacewing::Address &ServerAddress ();

    typedef void (lw_callback * HandlerConnect)
        (Lacewing::Client &client);

    typedef void (lw_callback * HandlerDisconnect)
        (Lacewing::Client &client);

    typedef void (lw_callback * HandlerReceive)
        (Lacewing::Client &client, const char * buffer, size_t size);

    typedef void (lw_callback * HandlerError)
        (Lacewing::Client &client, Lacewing::Error &error);

    lw_import void onConnect    (HandlerConnect);
    lw_import void onDisconnect (HandlerDisconnect);
    lw_import void onReceive    (HandlerReceive);
    lw_import void onError      (HandlerError);
};

struct Server
{
    lacewing_class_tag;

    lw_import  Server (Pump &);
    lw_import ~Server ();

    lw_import void Host    (int Port);
    lw_import void Host    (Lacewing::Filter &Filter);

    lw_import void Unhost  ();
    lw_import bool Hosting ();
    lw_import int  Port    ();

    lw_import bool LoadCertificateFile
        (const char * filename, const char * passphrase = "");

    lw_import bool LoadSystemCertificate
        (const char * storeName, const char * common_name,
         const char * location = "CurrentUser");

    lw_import bool CertificateLoaded ();

    lw_import bool CanNPN ();
    lw_import void AddNPN (const char *);

    struct Client : public FDStream
    {
        Client (Lacewing::Pump &, lw_fd);

        lacewing_class_tag;

        lw_import Lacewing::Address &GetAddress ();

        lw_import Client * Next ();

        lw_import const char * NPN ();
    };

    lw_import int ClientCount ();
    lw_import Client * FirstClient ();

    typedef void (lw_callback * HandlerConnect)
        (Lacewing::Server &server, Lacewing::Server::Client &client);

    typedef void (lw_callback * HandlerDisconnect)
        (Lacewing::Server &server, Lacewing::Server::Client &client);

    typedef void (lw_callback * HandlerReceive)
        (Lacewing::Server &server, Lacewing::Server::Client &client,
             const char * data, size_t size);

    typedef void (lw_callback * HandlerError)
        (Lacewing::Server &server, Lacewing::Error &error);
    
    lw_import void onConnect     (HandlerConnect);
    lw_import void onDisconnect  (HandlerDisconnect);
    lw_import void onReceive     (HandlerReceive);
    lw_import void onError       (HandlerError);
};

struct UDP
{
    lacewing_class_tag;

    lw_import  UDP (Pump &);
    lw_import ~UDP ();

    lw_import void Host (int Port);
    lw_import void Host (Lacewing::Filter &Filter);
    lw_import void Host (Address &); /* Use Port () afterwards to get the port number */

    lw_import bool Hosting ();
    lw_import void Unhost ();

    lw_import int Port ();

    lw_import void Write (Lacewing::Address &Address, const char * Data, size_t Size = -1);

    typedef void (lw_callback * HandlerReceive)
        (Lacewing::UDP &UDP, Lacewing::Address &from, char * data, size_t size);

    typedef void (lw_callback * HandlerError)
        (Lacewing::UDP &UDP, Lacewing::Error &error);
    
    lw_import void onReceive (HandlerReceive);
    lw_import void onError  (HandlerError);
};

struct Webserver
{
    lacewing_class_tag;

    lw_import  Webserver (Pump &);
    lw_import ~Webserver ();

    lw_import void Host         (int Port = 80);
    lw_import void HostSecure   (int Port = 443);
    
    lw_import void Host         (Lacewing::Filter &Filter);
    lw_import void HostSecure   (Lacewing::Filter &Filter);
    
    lw_import void Unhost        ();
    lw_import void UnhostSecure  ();

    lw_import bool Hosting       ();
    lw_import bool HostingSecure ();

    lw_import int  Port          ();
    lw_import int  SecurePort    ();

    lw_import bool LoadCertificateFile
        (const char * filename, const char * passphrase = "");

    lw_import bool LoadSystemCertificate
        (const char * storeName, const char * common_name,
         const char * location = "CurrentUser");

    lw_import bool CertificateLoaded ();

    lw_import void EnableManualRequestFinish ();

    lw_import int  IdleTimeout ();
    lw_import void IdleTimeout (int Seconds);

    struct Request : public Stream
    {
        lacewing_class_tag;

        lw_import Request (Pump &);
        lw_import ~ Request ();

        lw_import Address &GetAddress ();

        lw_import bool Secure ();

        lw_import const char * URL ();
        lw_import const char * Hostname ();
        
        lw_import void Disconnect ();

        lw_import void SetRedirect (const char * URL);
        lw_import void Status (int Code, const char * Message);

        lw_import void SetMimeType (const char * MimeType, const char * Charset = "UTF-8");
        lw_import void GuessMimeType (const char * Filename);

        lw_import void Finish ();

        lw_import int  IdleTimeout ();
        lw_import void IdleTimeout (int Seconds);

        lw_import lw_i64 LastModified  ();
        lw_import void   LastModified  (lw_i64 Time);
        lw_import void   SetUnmodified ();
    
        lw_import void DisableCache ();

        lw_import void   EnableDownloadResuming ();
        lw_import lw_i64 RequestedRangeBegin ();
        lw_import lw_i64 RequestedRangeEnd ();
        lw_import void   SetOutgoingRange (lw_i64 Begin, lw_i64 End);
        

        /** Headers **/

        struct Header
        {
            lw_import const char * Name ();
            lw_import const char * Value ();

            lw_import Header * Next ();
        };

        lw_import struct Header * FirstHeader ();

        lw_import const char * Header (const char * Name);
        lw_import void Header (const char * Name, const char * Value);

        /* Does not overwrite an existing header with the same name */

        lw_import void AddHeader
            (const char * Name, const char * Value);

    
        /** Cookies **/

        struct Cookie
        {
            lw_import const char * Name ();
            lw_import const char * Value ();

            lw_import Cookie * Next ();
        };

        lw_import struct Cookie * FirstCookie ();

        lw_import const char * Cookie (const char * Name);
        lw_import void         Cookie (const char * Name, const char * Value);
        lw_import void         Cookie (const char * Name, const char * Value, const char * Attributes);

    
        /** Sessions **/

        struct SessionItem
        {
            lw_import const char * Name ();
            lw_import const char * Value ();

            lw_import SessionItem * Next ();
        };

        lw_import SessionItem * FirstSessionItem ();

        lw_import const char * Session ();
        lw_import const char * Session (const char * Key);
        lw_import void         Session (const char * Key, const char * Value);

        lw_import void  CloseSession ();

            
        /** GET/POST data **/

        struct Parameter
        {
            lw_import const char * Name ();
            lw_import const char * Value ();
            lw_import const char * ContentType ();

            lw_import Parameter * Next ();
        };

        lw_import Parameter * GET ();
        lw_import Parameter * POST ();

        lw_import const char * GET  (const char * Name);
        lw_import const char * POST (const char * Name);

        lw_import const char * Body ();
    };

    lw_import void CloseSession (const char * ID);

    struct Upload
    {
        lacewing_class_tag;

        lw_import const char * FormElementName ();
        lw_import const char * Filename ();
        lw_import void         SetAutoSave ();
        lw_import const char * GetAutoSaveFilename ();

        lw_import const char * Header (const char * Name);
        
        struct Header
        {
            lw_import const char * Name ();
            lw_import const char * Value ();

            lw_import Header * Next ();
        };

        lw_import struct Header * FirstHeader ();
    };

    typedef void (lw_callback * HandlerGet)                    (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request);
    typedef void (lw_callback * HandlerPost)                   (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request);
    typedef void (lw_callback * HandlerHead)                   (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request);  
    typedef void (lw_callback * HandlerDisconnect)             (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request);
    typedef void (lw_callback * HandlerError)                  (Lacewing::Webserver &Webserver, Lacewing::Error &);

    typedef void (lw_callback * HandlerUploadStart)            (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request,
                                                                    Lacewing::Webserver::Upload &Upload);

    typedef void (lw_callback * HandlerUploadChunk)            (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request,
                                                                    Lacewing::Webserver::Upload &Upload, const char * Data, size_t Size);
     
    typedef void (lw_callback * HandlerUploadDone)             (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request,
                                                                    Lacewing::Webserver::Upload &Upload);

    typedef void (lw_callback * HandlerUploadPost)             (Lacewing::Webserver &Webserver, Lacewing::Webserver::Request &Request,
                                                                    Lacewing::Webserver::Upload * Uploads [], int UploadCount);

    lw_import void onGet              (HandlerGet);
    lw_import void onUploadStart      (HandlerUploadStart);
    lw_import void onUploadChunk      (HandlerUploadChunk);
    lw_import void onUploadDone       (HandlerUploadDone);
    lw_import void onUploadPost       (HandlerUploadPost);
    lw_import void onPost             (HandlerPost);
    lw_import void onHead             (HandlerHead);
    lw_import void onDisconnect       (HandlerDisconnect);
    lw_import void onError            (HandlerError);
};
    
struct FlashPolicy
{
    lacewing_class_tag;

    lw_import  FlashPolicy (Pump &);
    lw_import ~FlashPolicy ();

    lw_import void Host (const char * Filename);
    lw_import void Host (const char * Filename, Lacewing::Filter &Filter);

    lw_import void Unhost ();

    lw_import bool Hosting ();

    typedef void (lw_callback * HandlerError)
        (Lacewing::FlashPolicy &FlashPolicy, Lacewing::Error &);

    lw_import void onError (HandlerError);
};

}

#endif /* __cplusplus */
#endif /* LacewingIncluded */

