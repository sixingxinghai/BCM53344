/*   Copyright (C) 2003  All Rights Reserved.  */

#ifndef __PACOS_LACP_CLI_H__
#define __PACOS_LACP_CLI_H__

/*
   802.3 LACP
  
   This module declares the interface to LACP
  
 */

#include "vty.h"

/* Install 802.3 LACP related CLI.  */
void
lacp_cli_init (struct lib_globals * lacpm);

#endif
