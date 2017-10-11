/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "ospfd/ospfd.h"

void
ospf_snmp_community_event (struct apn_vr *vr, char *name)
{
  vr->snmp_community.current_community = name; 
}

void
ospf_snmp_community_init (struct ospf_master *om)
{
  snmp_community_event_hook (om->vr, ospf_snmp_community_event);
}
