/* Copyright (C) 2002  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "timeutil.h"

#include "ospf_const.h"
#include "ospf_util.h"


s_int32_t LSAMaxSeqNumber;
struct pal_timeval MinLSInterval;
struct pal_timeval MinLSArrival;
struct pal_in4_addr OSPFBackboneAreaID;
struct pal_in4_addr OSPFRouterIDUnspec;


void
ospf_const_init ()
{
  LSAMaxSeqNumber = pal_hton32 (OSPF_MAX_SEQUENCE_NUMBER);
  MinLSInterval = INT2TV (OSPF_MIN_LS_INTERVAL);
  MinLSArrival = INT2TV (OSPF_MIN_LS_ARRIVAL);
  OSPFBackboneAreaID.s_addr = pal_hton32 (OSPF_AREA_BACKBONE);
  OSPFRouterIDUnspec.s_addr = pal_hton32 (0);
}
