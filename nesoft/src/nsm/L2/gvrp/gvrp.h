/* Copyright 2003  All Rights Reserved. */

#ifndef _PACOS_GVRP_H
#define _PACOS_GVRP_H

#include "lib.h"

#define GVRP_MAX_VLANS                    4095

#define GVRP_NUMBER_OF_GID_MACHINES       GVRP_MAX_VLANS 
#define GVRP_UNUSED_INDEX                 GVRP_NUMBER_OF_GID_MACHINES

#define GVRP_MGMT_PORT_CONFIGURED         0x80

extern struct lib_globals *gvrpm;

enum gvrp_registration_type
{
  REG_TYPE_GVRP,
  REG_TYPE_MVRP
};

enum gvrp_applicant_state
{
  GVRP_APPLICANT_NORMAL  = 1,
  GVRP_APPLICANT_ACTIVE  = 2
};

enum gvrp_attribute_type
{
  GVRP_ATTR_ALL  = 0,
  GVRP_ATTR_VLAN = 1,
  GVRP_ATTR_MAX  = 2
};

struct gvrp
{
  struct nsm_bridge *bridge;
  struct garp garp;
  struct garp_instance garp_instance;
  struct gvrp_gvd *gvd;
  struct nsm_vlan_listener *gvrp_appln;
  struct nsm_bridge_listener *gvrp_br_appln;
  u_int16_t dynamic_vlan_enabled;
  u_int16_t vid;
  u_int32_t num_packets_dropped;
  u_int32_t failed_registrations;
  enum gvrp_registration_type reg_type;
  u_char p2p:1;
  u_int16_t encoded_vid;
  int gvrp_last_pdu_origin;
};

struct gvrp_port
{
  u_int16_t vid;
  void *port;
  struct gid *gid;
  struct gid_port *gid_port;
  u_char flags;
  int    registration_mode;
  int    applicant_state;
#ifdef HAVE_MVRP
  /* num_pkts, leave_all, join_empty, join_in, leave_empty, leave_in, empty */
  unsigned int receive_counters[MRP_ATTR_EVENT_MAX + 1];

  /* num_pkts, leave_all, join_empty, join_in, leave_empty, leave_in, empty */
  unsigned int transmit_counters[MRP_ATTR_EVENT_MAX + 1];
#else
  /* num_pkts, leave_all, join_empty, join_in, leave_empty, leave_in, empty */
  unsigned int receive_counters[GARP_ATTR_EVENT_MAX + 1];

  /* num_pkts, leave_all, join_empty, join_in, leave_empty, leave_in, empty */
  unsigned int transmit_counters[GARP_ATTR_EVENT_MAX + 1];
#endif /* HAVE_MVRP */
  u_int32_t gvrp_failed_registrations;
  u_int8_t gvrp_last_pdu_origin [ETHER_ADDR_LEN];
};

struct gvrp_port_config
{
  int registration_mode;
  int applicant_state;
  pal_time_t timer_details[GARP_MAX_TIMERS];
  bool_t enable_port;
  u_int8_t p2p;
};

extern void
gvrp_init (struct lib_globals *zg);


extern bool_t
gvrp_create_gvrp (struct nsm_bridge *bridge, u_int16_t vid, struct gvrp **gvrp);


extern int 
gvrp_destroy_gvrp (struct nsm_bridge_master *master,
    char * bridge_name);


extern bool_t
gvrp_create_port (struct gvrp *gvrp, struct interface *ifp, 
                  u_int16_t vid, struct gvrp_port **gvrp_port);


extern void
gvrp_destroy_port (struct gvrp_port *old_gvrp_port);


extern void
gvrp_join_indication (void *gvrp, struct gid *gid,
                      u_int32_t joining_gid_index);

extern void
gvrp_join_propagated (void *gvrp, struct gid *gid,
                      u_int32_t joining_gid_index);


extern void
gvrp_leave_indication (void *gvrp, struct gid *gid, 
                       u_int32_t leave_gid_index);


extern void
gvrp_leave_propagated (void *application, struct gid *gid, 
                       u_int32_t leave_gid_index);

extern void
gvrp_do_actions (struct gid *gid);

extern u_int16_t
gvrp_get_vid (void *application);

extern u_int16_t
gvrp_get_svid (void *application);

extern void*
gvrp_get_bridge_instance (void *gvrp);

extern struct gid*
gvrp_get_gid (struct nsm_bridge_port *br_port, u_int16_t vid);
#endif /* !_PACOS_GVRP_H */
