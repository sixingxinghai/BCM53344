/*=============================================================================
**
** Copyright (C) 2008-2009   All Rights Reserved.
**
** vlog_api.c - Definitions of the VLOG server.
*/

#ifndef _PACOS_VLOG_SERVER_H
#define _PACOS_VLOG_SERVER_H

#include "pal.h"
#include "lib.h"
#include "linklist.h"
#include "message.h"


/*-------------------------------------------------------------------
 * vlog_server - VLOG server structure.
 *
 */
typedef struct vlog_server
{
  struct lib_globals     *vsv_zg;
  struct message_handler *vsv_ms;
  vector                  vsv_clt_vect;
} VLOG_SERVER;



/*-------------------------------------------------------------------
 * vlog_server_entry - VLOG client's server entry object.
 *
 */
typedef struct vlog_server_entry
{
  module_id_t           vse_mod_id;
  struct message_entry *vse_me;
  VLOG_SERVER          *vse_server;
  u_int32_t             vse_recv_msg_count;
  pal_time_t            vse_connect_time;
  pal_time_t            vse_read_time;
} VLOG_SRV_ENTRY;

VLOG_SERVER * vlog_server_init (struct lib_globals *);
int vlog_server_connect (struct message_handler *, struct message_entry *,
                         pal_sock_handle_t );
int vlog_server_disconnect (struct message_handler *, struct message_entry *,
                            pal_sock_handle_t );
int vlog_server_read_msg (struct message_handler *, struct message_entry *,
                          pal_sock_handle_t );
int vlog_server_finish (VLOG_SERVER *);

/* Support for "show vlog clients" command.
   In order to "walk" all the server clients the caller must implement the
   callback and invoke the vlog_server_walk_clients()
*/
typedef ZRESULT (* VLOG_SERVER_WALK_CB)(VLOG_SRV_ENTRY *vse, intptr_t user_ref);

ZRESULT vlog_server_walk_clients(VLOG_SERVER *vsv, VLOG_SERVER_WALK_CB user_cb,
                                 intptr_t user_ref);

#endif /* _PACOS_VLOG_SERVER_H */

