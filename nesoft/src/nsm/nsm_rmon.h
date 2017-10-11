/* Copyright (C) 2004  All Rights Reserved.  */

#ifndef _NSM_RMON_H
#define _NSM_RMON_H

#include "nsm_server.h"
#include "nsm_message.h"
int
nsm_server_recv_rmon_req_statistics (struct nsm_msg_header *header,
                                     void *arg, void *message);

int
nsm_rmon_send_statistics (struct nsm_server_entry *nse, 
                          struct nsm_msg_rmon_stats *msg,
                          u_int32_t msg_id);


int
nsm_rmon_set_server_callback (struct nsm_server *ns);

#endif /* _NSM_RMON_H */

