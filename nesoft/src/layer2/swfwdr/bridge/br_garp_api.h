/* Copyright (C) 2005  All Rights Reserved.
 *
 *  GARP functionalities
 *
 *  Author:  
 *
 */
                                                                                
#ifndef __PACOS_BR_GARP_API_H__
#define __PACOS_BR_GARP_API_H__
                                                                                
#include "if_ipifwd.h"
#include "br_types.h"

/* Function to set the port state as gmrp/gvrp enable or disable */
extern int
br_garp_set_bridge_type (struct apn_bridge *br, unsigned long garp_type, int enable);

#endif /* __PACOS_BR_GARP_API_H__ */
