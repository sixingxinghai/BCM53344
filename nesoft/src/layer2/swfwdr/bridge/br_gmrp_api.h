/* Copyright (C) 2003  All Rights Reserved.
 *
 *  GMRP functionalities
 *
 *  Author:  
 *
 */
                                                                                
#ifndef __PACOS_BR_GMRP_API_H__
#define __PACOS_BR_GMRP_API_H__
                                                                                
#include "if_ipifwd.h"
#include "br_types.h"

extern int
br_gmrp_set_service_requirement (struct apn_bridge *br, int vlanid, int ifindex, int serv_req, bool_t activate);

extern int
br_gmrp_set_ext_filter (struct apn_bridge *br, bool_t enable);

#endif /* __PACOS_BR_GARP_API_H__ */

