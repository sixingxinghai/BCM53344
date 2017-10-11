/* Copyright 2003  All Rights Reserved. */
#ifndef _PACOS_GARP_H
#define _PACOS_GARP_H

#include "pal.h"
#include "lib.h"

#define ATTR_INDX_MAX_BITLEN 32
/*
 * Comment Me 
 */
struct gid;
struct garp;

/* garp */
struct garp {

  /* pointer to the management control structure for the garp application
    (GVRP, GMRP) */
  struct lib_globals *garpm;

  enum reg_proto_type proto;

  /* Callback garp application function that gets upcalled when join
     indication is received for the garp application */
  void(*join_indication_func) (void *application, 
                               struct gid *gid_instance, 
                               u_int32_t joining_gid_index);

  /* Callback garp application function that gets upcalled when leave
     indication is received for the garp application */
  void(*leave_indication_func) (void *application,
                                struct gid *gid_instance, 
                                u_int32_t leaving_gid_index); 

  /* Callback garp application function that gets upcalled when join
     indication is received for the garp application */
  void(*join_propagated_func) (void *application,
                               struct gid *gid_instance, 
                               u_int32_t joining_gid_index);

  /* Callback garp application function that gets upcalled when leave
     indication is received for the garp application */
  void(*leave_propagated_func) (void *application,
                                struct gid *gid_instance, 
                                u_int32_t leaving_gid_index);

  /* Callback garp application function that gets upcalled when a gid_instance 
     associated with the port requires to transmit a garp pdu */
  bool_t (*transmit_func) (void *application, struct gid *gid_instance);

  void* (*get_bridge_instance_func) (void *application);

  u_int16_t (*get_vid_func) (void *application);

  u_int16_t (*get_svid_func) (void *application);

  struct gid* (*get_gid_func)(const void *port, u_int16_t vid, u_int16_t svid);
};

/* garp_instance */
struct garp_instance {

  /*Pointer to GARP */
  struct garp *garp;

  /* pointer to the garp application (GMRP, GVRP) */
  struct garp *application;

  /* pointer to garp information propagation instance */
  struct ptree *gip_table;

  /* num_pkts, leave_all, join_empty, join_in, leave_empty, leave_in, empty */
#if defined HAVE_MMRP || defined HAVE_MVRP
  unsigned int receive_counters[MRP_ATTR_EVENT_MAX + 1];
#else
  unsigned int receive_counters[GARP_ATTR_EVENT_MAX + 1];
#endif /* HAVE_MMRP || HAVE_MVRP */

  /* num_pkts, leave_all, join_empty, join_in, leave_empty, leave_in, empty */
#if defined HAVE_MMRP || defined HAVE_MVRP
  unsigned int transmit_counters[MRP_ATTR_EVENT_MAX + 1];
#else
  unsigned int transmit_counters[GARP_ATTR_EVENT_MAX + 1];
#endif /* HAVE_MMRP || HAVE_MVRP */

};

/* Function declarations. */
int garp_init (struct lib_globals *zg);

int
garp_deinit (struct lib_globals *zg);

#endif /* _PACOS_GARP_H */
