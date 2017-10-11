/* Copyright (C) 2005  All Rights Reserved. */

#ifndef _HAL_GMRP_H_
#define _HAL_GMRP_H_

#define HAL_GMRP_LEGACY_FORWARD_ALL                     0x00
#define HAL_GMRP_LEGACY_FORWARD_UNREGISTERED            0x01
#define NOT_IMPLIMENTED                                 2

/*
   Name: hal_gmrp_set_service_requirement

   Description:
   This API sets the gmrp service requirement for an interface .

   Parameters:
   bridge name
   vlanid
   ifindex
   serv_req
   type  - FWDALL -0x00
         - FWDUNREGISTERED -01
   activate - True
          - False
   Returns: Not Implemented 
  
*/
/* This fuction notifies forwarder about gmrp service requirement on a port */

extern int
hal_l2_gmrp_set_service_requirement (char *bridge_name, int vlanid, int ifindex, int serv_req, bool_t activate);

/*
   Name: hal_gmrp_set_bridge_ext_filter

   Description:
   This API enables/disables the extended group filtering services on a bridge.

   Parameters:
   bridge name
   enable - True
          - False
   Returns: Not Implemented

*/

/* This fuction notifies forwarder about enabling extended filtering services on a bridge */

extern int
hal_gmrp_set_bridge_ext_filter (char *name, bool_t enable);

#endif /* _HAL_GMRP_H_ */

