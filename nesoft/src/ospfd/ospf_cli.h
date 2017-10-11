/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_OSPF_CLI_H
#define _PACOS_OSPF_CLI_H

#define OSPF_NONPRINTABLE_STR   0
#define OSPF_PRINTABLE_STR      1

#define OSPF_CLI_IF_STR                                                       \
     "IP Information",                                                        \
     "OSPF interface commands"
#define OSPF_CLI_IF_ADDR_STR                                                  \
     "Address of interface"
#define OSPF_CLI_AREA_STR                                                     \
    "OSPF area parameters",                                                   \
    "OSPF area ID in IP address format",                                      \
    "OSPF area ID as a decimal value"

#define OSPF_CLI_VLINK_STR                                                    \
    "Define a virtual link and its parameters",                               \
    "ID (IP addr) associated with virtual link neighbor"

#define OSPF_CLI_NSSA_STR                                                     \
    "Specify a NSSA area"

/* Prototypes. */
void ospf_cli_init ();
int ospf_printable_str (char *, int);
int ospf_cli_return (struct cli *, int);

#endif /* _PACOS_OSPF_CLI_H */
