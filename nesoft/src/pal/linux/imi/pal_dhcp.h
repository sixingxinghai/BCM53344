/* Copyright (C) 2002,   All Rights Reserved. */

/* pal_dhcp.h -- PacOS PAL DHCP operations definitions
                 for Linux */

#ifndef _PAL_DHCP_H
#define _PAL_DHCP_H

/* Include files */
#include "pal.h"

/* Constants and enumerations */

/* Client. */
#define         DHCP_CLIENT_APP         "/usr/sbin/dhcpcd"
#define         DHCP_OPT_CLIENTID       "-I "
#define         DHCP_OPT_HOSTNAME       "-h "
#define         DHCP_CLIENT_PID_FILE    "/var/run/dhcpcd-%s.pid"
#define         DHCP_OPT_CONFIGDIR      "-L"
#define         DHCP_CONFIGDIR_PATH     "/var/run"

/* Server. */
#define         DHCP_SERVER_APP         "/usr/sbin/dhcpd"
#define         DHCP_SERVER_OPTIONS     "-q"
#define         DHCP_FILE               "/etc/dhcpd.conf"
#define         DHCP_FILE_BKUP          "/etc/dhcpd.conf.sav"
#define         DHCP_PID_FILE           "/var/run/dhcpd.pid"

#define         DHCP_TOP_FILE_TEXT      "#\n# /etc/dhcpd.conf - DHCP Server Setup File.\n#\n"
#define         DHCP_DISABLE_FILE_TEXT  "#\n# The DHCP server is currently disabled.\n#\n"

/* Functions */
#include "pal_dhcp.def"

#endif /* def _PAL_DHCP_H */
