/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_NSM_OAM_H_
#define _PACOS_NSM_OAM_H_
#include "nsm_interface.h"
#include "nsm_message.h"

int nsm_oam_set_server_callback (struct nsm_server *ns);

int nsm_efm_oam_if_msg_process (struct nsm_msg_header *header,
                                void *arg, void *message);

int nsm_lldp_msg_process (struct nsm_msg_header *header,
                          void *arg, void *message);

int
nsm_cfm_msg_process (struct nsm_msg_header *header,
                     void *arg, void *message);
int
nsm_cfm_status_msg_process (struct nsm_msg_header *header,
                     void *arg, void *message);

#ifdef HAVE_G8032
int
nsm_g8032_portstate_msg_process (struct nsm_msg_header *header,
                                 void *arg, void *message);
#endif /*HAVE_G8032*/

int
nsm_pg_init_msg_process (struct nsm_msg_header *header,
                     void *arg, void *message);

int
nsm_g8031_portstate_msg_process (struct nsm_msg_header *header,
                                 void *arg, void *message);


#define NSM_L2_OAM_ERR_NONE                  0

#define NSM_EFM_OAM_ERR_BASE                 -200
#define NSM_EFM_OAM_ERR_IF_NOT_FOUND         (NSM_EFM_OAM_ERR_BASE + 1)
#define NSM_EFM_OAM_ERR_NOT_ENABLED          (NSM_EFM_OAM_ERR_BASE + 2)
#define NSM_EFM_OAM_ERR_ENABLED              (NSM_EFM_OAM_ERR_BASE + 3)
#define NSM_EFM_OAM_ERR_HAL                  (NSM_EFM_OAM_ERR_BASE + 4)
#define NSM_EFM_OAM_ERR_MEMORY               (NSM_EFM_OAM_ERR_BASE + 5)
#define NSM_EFM_OAM_ERR_INTERNAL             (NSM_EFM_OAM_ERR_BASE + 6)
#define NSM_EFM_OAM_LINK_MONITOR_OFF         (NSM_EFM_OAM_ERR_BASE + 7)

#define NSM_OAM_LLDP_ERR_BASE                -300
#define NSM_OAM_LLDP_ERR_IF_NOT_FOUND       (NSM_OAM_LLDP_ERR_BASE + 1)
#define NSM_OAM_LLDP_ERR_INTERNAL           (NSM_OAM_LLDP_ERR_BASE + 2)

#define NSM_OAM_CFM_ERR_BASE                 -400
#define NSM_OAM_CFM_ERR_IF_NOT_FOUND        (NSM_OAM_CFM_ERR_BASE + 1)
#define NSM_OAM_CFM_ERR_INTERNAL            (NSM_OAM_CFM_ERR_BASE + 2)
#define NSM_OAM_CFM_ERR_HAL                 (NSM_OAM_CFM_ERR_BASE + 3)

#define NSM_OAM_G8031_ERR_BASE               -500
#define NSM_OAM_G8031_INTERNAL               (NSM_OAM_G8031_ERR_BASE + 1)

#define NSM_OAM_G8032_ERR_BASE               -600
#define NSM_OAM_G8032_FAIL_SET_PORT_STATE    (NSM_OAM_G8032_ERR_BASE + 1)
#define NSM_OAM_G8032_FAIL_TO_FLUSH          (NSM_OAM_G8032_ERR_BASE + 2)
 
#define NSM_EFM_OAM_FRAME_SEC_INTERVAL        1

struct nsm_efm_oam_if
{
  struct nsm_if *zif;
  u_int16_t      local_event;
  struct thread  *t_frame_secs;
  struct thread  *t_frame_window;
  struct thread  *t_frame_secs_window;
  u_int8_t       link_monitor_on;
  ut_int64_t     sym_period_window;
  u_int32_t      frame_event_window;
  u_int32_t      frame_period_window;
  u_int32_t      frame_sec_sum_window;
  u_int32_t      no_of_err_frame_sec;
  ut_int64_t     no_of_err_last_frame_window;
};

struct nsm_l2_oam_master
{
  u_int16_t syscap;
  u_int16_t cfm_ether_type;
  struct nsm_master *master;
  u_int8_t cfm_proto_add [ETHER_ADDR_LEN];
  u_int8_t lldp_proto_add [ETHER_ADDR_LEN];
};

struct nsm_lldp_oam_if
{
  struct nsm_if *zif;

  u_int16_t protocol;

  u_int32_t agg_port_id;
};

u_int16_t
nsm_efm_oam_str_to_event (u_int8_t *str);

enum nsm_efm_opcode
nsm_efm_oam_str_to_opcode (u_int8_t *str);

s_int32_t
nsm_efm_oam_process_period_error_event (struct interface *ifp,
                                        enum nsm_efm_opcode opcode,
                                        u_int16_t no_of_errors);

s_int32_t
nsm_efm_oam_process_return (struct interface *ifp,
                            struct cli *cli,
                            s_int32_t retval);

int
nsm_efm_oam_process_local_event (struct interface *ifp,
                                 u_int16_t event,
                                 u_int8_t enable);

s_int32_t
nsm_efm_oam_set_frame_seconds_errors (struct interface *ifp,
                                      u_int16_t no_of_errors);

s_int32_t
nsm_efm_oam_set_frame_errors (struct interface *ifp,
                              u_int16_t no_of_errors);

s_int32_t
nsm_oam_lldp_set_sys_cap (u_int32_t vr_id, u_int16_t syscap, u_int8_t enable);

void
nsm_l2_oam_master_init (struct nsm_master *nm);

void
nsm_l2_oam_master_deinit (struct nsm_master *nm);

void
nsm_oam_lldp_if_add (struct interface *ifp);

void
nsm_oam_lldp_if_delete (struct interface *ifp);

int
nsm_oam_lldp_if_set_protocol_list (struct interface *ifp,
                                   u_int32_t protocol,
                                   u_int8_t enable);

int
nsm_lldp_set_agg_port_id (struct interface *ifp,
                          u_int32_t ifindex);

#endif /* _PACOS_NSM_OAM_H_ */
