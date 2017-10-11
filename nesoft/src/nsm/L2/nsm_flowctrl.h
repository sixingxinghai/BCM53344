/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _NSM_FLOWCTL_H
#define _NSM_FLOWCTL_H

#ifdef HAVE_SMI
#include "smi_message.h"
#endif /* HAVE_SMI */

struct flowctl_node {
  struct interface *port;
  unsigned char direction;
#ifdef HAVE_DCB
  bool_t status;
#define NSM_PFC_ENABLE   PAL_TRUE
#define NSM_PFC_DISABLE  PAL_FALSE
#endif /*HAVE_DCB*/
#ifdef HAVE_HA
  HA_CDR_REF nsm_flow_cntl_cdr_ref;
#endif /*HAVE_HA*/
};
struct flowctl_stats {
  int rxpause;
  int txpause;
  unsigned char direction;
};

int flow_control_write(struct cli *, int);
void flow_control_init(struct lib_globals *zg);
int flow_control_get_direction (struct interface *port);
int port_add_flow_control (struct interface *ifp, unsigned char direction);
int port_delete_flow_control (struct interface *ifp, unsigned char direction);
struct flowctl_node* flow_control_list_lookup(char *ifname);
#ifdef HAVE_SMI
int get_flow_control_statistics (struct interface *ifp, 
                                 enum smi_flow_control_dir *direction,
                                 int *rxpause, int *txpause);
int flow_control_get_interface (struct interface *ifp, 
                                enum smi_flow_control_dir *direction);
#endif /* HAVE_SMI */
int nsm_flow_control_enable_if (struct interface *ifp);
int nsm_flow_control_disable_if (struct interface *ifp);
#endif /* _NSM_FLOWCTL_H. */
