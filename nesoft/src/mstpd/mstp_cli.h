/* Copyright (C) 2003  All Rights Reserved.  
   LAYER 2 BRIDGE CLI
  
   This module declares the interface to the Layer 2 Bridge CLI
  
*/

#ifndef __PACOS_MSTP_CLI_H__
#define __PACOS_MSTP_CLI_H__

#define MSTP_PORTSTATE_ERR 4
#define MSTP_TIME_LENGTH   48

/* The interface name with vlan information for provider edge ports */
#define MSTP_CLI_IFNAME_LEN (L2_IF_NAME_LEN + 5 + 1)

#define MSTP_GET_BR_NAME(ARGC, POS)                                           \
     if (argc == ARGC)                                                        \
       {                                                                      \
         if (argv[0][0]!='b')                                                 \
           CLI_GET_INTEGER_RANGE ("Max bridge groups", brno, argv[0], 1, 32);   \
         bridge_name = argv [0];                                              \
       }                                                                      \
     else                                                                     \
       bridge_name = def_bridge_name;

/* Install bridge related CLI.  */
void
mstp_bridge_cli_init (struct lib_globals * mstpm);

/* Show bridge configuration.  */
int
mstp_bridge_config_write (struct cli * cli);

#ifdef HAVE_RPVST_PLUS
/* Show rpvst+ bridge configuration.  */
void
rpvst_plus_bridge_config_write (struct cli * cli,
                                struct mstp_bridge_instance *br_inst,
                                struct mstp_bridge *br);
#endif /* HAVE_RPVST_PLUS */
/* Install WMI debug commands. */
void
mstp_wmi_debug_init (struct lib_globals *mstpm);

#endif
