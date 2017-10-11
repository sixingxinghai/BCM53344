/* Copyright (C) 2002,   All Rights Reserved. */

/* pal_pppoe.h -- PacOS PAL PPPOE operations definitions
                 for Linux */

#ifndef _PAL_PPPOE_H
#define _PAL_PPPOE_H

/* Include files */
#include "pal.h"
#include "pal_pppoe.def"

/* Constants and enumerations */
#define PPPOE_CLIENT_APP        "/usr/sbin/adsl-start"
#define PPPOE_CLIENT_APP_STOP   "/usr/sbin/adsl-stop"
#define PPPOE_CLIENT_OPTIONS    ""
#define PPPOE_FILE              "/etc/ppp/pppoe.conf"
#define PPPOE_FILE_BKUP         "/etc/pppoe.conf.sav"
#define PPPOE_PID_FILE          "/var/run/pppoe.pid"

#define PPPOE_PAP_SECRETS_FILE                "/etc/ppp/pap-secrets"
#define PPPOE_PAP_SECRETS_FILE_BKUP           "/etc/ppp/pap-secrets.sav"
#define PPPOE_PAP_SECRETS_TOP_FILE_TEXT       "# Secrets for authentication using PAP\n\
                                               # client server  secret                  IP addresses\n"

#define PPPOE_CHAP_SECRETS_FILE                "/etc/ppp/chap-secrets"
#define PPPOE_CHAP_SECRETS_FILE_BKUP           "/etc/ppp/chap-secrets.sav"
#define PPPOE_CHAP_SECRETS_TOP_FILE_TEXT      "# Secrets for authentication using CHAP\n\
                                               # client server  secret                  IP addresses\n"

#define PPPOE_CONF_COPYRIGHT_TEXT "\n\
#***********************************************************************\n\
#\n\
# pppoe.conf\n\
#\n\
# Configuration file for rp-pppoe.  Edit as appropriate and install in\n\
# /etc/ppp/pppoe.conf\n\
#\n\
# NOTE: This file is used by the adsl-start, adsl-stop, adsl-connect and\n\
#       adsl-status shell scripts.  It is *not* used in any way by the\n\
#       \"pppoe\" executable.\n\
#\n\
# Copyright (C) 2000 Roaring Penguin Software Inc.\n\
#\n\
# This file may be distributed under the terms of the GNU General\n\
# Public License.\n\
#\n\
# LIC: GPL\n\
# pal_pppoe.h,v 1.2 2008/01/30 20:29:31 bob.macgill Exp\n\
#***********************************************************************\n\
\n\
# When you configure a variable, DO NOT leave spaces around the \"=\" sign."

#define PPPOE_CONF_ETH_TEXT \
"# Ethernet card connected to ADSL modem\n"

#define PPPOE_CONF_USERNAME_TEXT \
"# ADSL user name.  You may have to supply \"@provider.com\"  Sympatico\n\
# users in Canada do need to include \"@sympatico.ca\"\n\
# Sympatico uses PAP authentication.  Make sure /etc/ppp/pap-secrets\n\
# contains the right username/password combination.\n\
# For Magma, use xxyyzz@magma.ca\n"

#define PPPOE_CONF_DEMAND_TEXT \
"# Bring link up on demand?  Default is to leave link up all the time.\n\
# If you want the link to come up on demand, set DEMAND to a number indicating\n\
# the idle time after which the link is brought down.\n\
DEMAND=no\n\
#DEMAND=300\n"

#define PPPOE_CONF_DNS_TEXT \
"# DNS type: SERVER=obtain from server; SPECIFY=use DNS1 and DNS2;\n\
# NOCHANGE=do not adjust.\n\
DNSTYPE=NOCHANGE\n\
\n\
# Obtain DNS server addresses from the peer (recent versions of pppd only)\n\
# In old config files, this used to be called USEPEERDNS.  Changed to\n\
# PEERDNS for better Red Hat compatibility\n\
PEERDNS=no\n\
\n\
DNS1=\n\
DNS2=\n"

#define PPPOE_CONF_DEFAULTROUTE_TEXT \
"# Make the PPPoE connection your default route.  Set to\n\
# DEFAULTROUTE=no if you don't want this.\n\
DEFAULTROUTE=yes\n"

#define PPPOE_CONF_EXPERT_TEXT \
"### ONLY TOUCH THE FOLLOWING SETTINGS IF YOU'RE AN EXPERT\n\
\n\
# How long adsl-start waits for a new PPP interface to appear before\n\
# concluding something went wrong.  If you use 0, then adsl-start\n\
# exits immediately with a successful status and does not wait for the\n\
# link to come up.  Time is in seconds.\n\
#\n\
# WARNING WARNING WARNING:\n\
#\n\
# If you are using rp-pppoe on a physically-inaccessible host, set\n\
# CONNECT_TIMEOUT to 0.  This makes SURE that the machine keeps trying\n\
# to connect forever after adsl-start is called.  Otherwise, it will\n\
# give out after CONNECT_TIMEOUT seconds and will not attempt to\n\
# connect again, making it impossible to reach.\n\
CONNECT_TIMEOUT=30\n\
\n\
# How often in seconds adsl-start polls to check if link is up\n\
CONNECT_POLL=2\n\
\n\
# Specific desired AC Name\n\
ACNAME=\n\
\n\
# Specific desired service name\n\
SERVICENAME=\n\
\n\
# Character to echo at each poll.  Use PING=\"\" if you don't want\n\
# anything echoed\n\
PING=\".\"\n\
\n\
# File where the adsl-connect script writes its process-ID.\n\
# Three files are actually used:\n\
#   $PIDFILE       contains PID of adsl-connect script\n\
#   $PIDFILE.pppoe contains PID of pppoe process\n\
#   $PIDFILE.pppd  contains PID of pppd process\n\
CF_BASE=`basename $CONFIG`\n\
PIDFILE=\"/var/run/$CF_BASE-adsl.pid\"\n\
\n\
# Do you want to use synchronous PPP?  \"yes\" or \"no\".  \"yes\" is much\n\
# easier on CPU usage, but may not work for you.  It is safer to use\n\
# \"no\", but you may want to experiment with \"yes\".  \"yes\" is generally\n\
# safe on Linux machines with the n_hdlc line discipline; unsafe on others.\n\
SYNCHRONOUS=no\n\
\n\
# Do you want to clamp the MSS?  Here's how to decide:\n\
# - If you have only a SINGLE computer connected to the ADSL modem, choose\n\
#   \"no\".\n\
# - If you have a computer acting as a gateway for a LAN, choose \"1412\".\n\
#   The setting of 1412 is safe for either setup, but uses slightly more\n\
#   CPU power.\n\
CLAMPMSS=1412\n\
#CLAMPMSS=no\n\
\n\
# LCP echo interval and failure count.\n\
LCP_INTERVAL=20\n\
LCP_FAILURE=3\n\
\n\
# PPPOE_TIMEOUT should be about 4*LCP_INTERVAL\n\
PPPOE_TIMEOUT=80\n\
\n\
# Firewalling: One of NONE, STANDALONE or MASQUERADE\n\
FIREWALL=NONE\n\
\n\
# Linux kernel-mode plugin for pppd.  If you want to try the kernel-mode\n\
# plugin, use LINUX_PLUGIN=/etc/ppp/plugins/rp-pppoe.so\n\
LINUX_PLUGIN=\n\
\n\
# Any extra arguments to pass to pppoe.  Normally, use a blank string\n\
# like this:\n\
PPPOE_EXTRA=\"\"\n\
\n\
# Rumour has it that \"Citizen's Communications\" with a 3Com\n\
# HomeConnect ADSL Modem DualLink requires these extra options:\n\
# PPPOE_EXTRA=\"-f 3c12:3c13 -S ISP\"\n\
\n\
# Any extra arguments to pass to pppd.  Normally, use a blank string\n\
# like this:\n\
PPPD_EXTRA=\"\"\n\
\n\
\n\
########## DON'T CHANGE BELOW UNLESS YOU KNOW WHAT YOU ARE DOING\n\
# If you wish to COMPLETELY overrride the pppd invocation:\n\
# Example:\n\
# OVERRIDE_PPPD_COMMAND=\"pppd call dsl\"\n\
\n\
# If you want adsl-connect to exit when connection drops:\n\
# RETRY_ON_FAILURE=no\n"

/* Functions */

#endif /* def _PAL_PPPOE_H */
