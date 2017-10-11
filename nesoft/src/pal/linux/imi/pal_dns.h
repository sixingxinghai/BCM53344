/* Copyright (C) 2002,   All Rights Reserved. */

/* pal_dns.h -- PacOS PAL DNS operations definitions
                for Linux */

#ifndef _PAL_DNS_H
#define _PAL_DNS_H

/* Include files. */
#include "pal.h"

/* Constants and enumerations */
#define         DNS_FILE                "/etc/resolv.conf"
#define         DNS_FILE_BKUP           "/etc/resolv.conf.sav"

#define         DNS_TOP_FILE_TEXT       "# /etc/resolv.conf - DNS Setup File.\n#"
#define         DNS_DISABLE_FILE_TEXT   "# The Domain Name Service (DNS) is currently disabled.\n#\n# Default domain(s).\n# Domain search list.\n# Name server(s).\n"

#define         DNS_DOMAIN_DFLT_TEXT    "\n# Default domain(s).\n"
#define         DNS_DOMAIN_LIST_TEXT    "\n# Domain search list.\n"
#define         DNS_NAME_SERVER_TEXT    "\n# Name Server(s).\n"

/* Functions */
#include "pal_dns.def"

#endif /* def _PAL_DNS_H */
