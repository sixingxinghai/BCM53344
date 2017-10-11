/*=============================================================================
**
** Copyright (C) 2002,   All Rights Reserved.
**
** pal_dns.h -- PacOS PAL Firewall operations definitions
**              for Linux
*/
#ifndef _PAL_FW_H
#define _PAL_FW_H

/*-----------------------------------------------------------------------------
**
** Include files
*/
#include "pal.h"

/*-----------------------------------------------------------------------------
**
** Constants and enumerations
*/
#define PORT_MIN          0
#define PORT_MAX          65535

#define IPTABLES          "iptables"

/* Tables. */
#define NAT_TABLE         "-t nat"
#define FILTER_TABLE      "-t filter"

/* Chain rules. */
#define ADD_POSTROUTING   "-A POSTROUTING"
#define DEL_POSTROUTING   "-D POSTROUTING"
#define INSERT_POSTROUTING "-I POSTROUTING"
#define ADD_PREROUTING    "-A PREROUTING"
#define DEL_PREROUTING    "-D PREROUTING"
#define INSERT_PREROUTING "-I PREROUTING"
#define ADD_INPUT         "-A INPUT"
#define INSERT_INPUT      "-I INPUT"
#define DEL_INPUT         "-D INPUT"
#define ADD_OUTPUT        "-A OUTPUT"
#define INSERT_OUTPUT     "-I OUTPUT"
#define DEL_OUTPUT        "-D OUTPUT"
#define ADD_FORWARD        "-A FORWARD"
#define INSERT_FORWARD      "-I FORWARD"
#define DEL_FORWARD        "-D FORWARD"
#define FLUSH_PREROUTING  "-F PREROUTING"
#define FLUSH_POSTROUTING "-F POSTROUTING"
#define FLUSH_INPUT       "-F INPUT"
#define FLUSH_OUTPUT      "-F OUTPUT"
#define FLUSH_FORWARD     "-F FORWARD"

/* Targets. */
#define SNAT              "-j SNAT"
#define DNAT              "-j DNAT"

/* Timeouts */
#define SYSCTL_READ       "sysctl"
#define SYSCTL_WRITE      "sysctl -w"

#define UDP_TIMEOUT    "net.ipv4.netfilter.ip_conntrack_udp_timeout"
#define ICMP_TIMEOUT    "net.ipv4.netfilter.ip_conntrack_icmp_timeout"
#define GENERIC_TIMEOUT "net.ipv4.netfilter.ip_conntrack_generic_timeout"
#define TCP_TIMEOUT    "net.ipv4.netfilter.ip_conntrack_tcp_timeout_established"
#define TCP_FIN_TIMEOUT "net.ipv4.netfilter.ip_conntrack_tcp_timeout_fin_wait"

#define _NAT_TRANSLATION_TIMEOUT     "/tmp/apn_nat_timeout"
#define DEFAULT_GENERIC_TIMEOUT 600
#define DEFAULT_UDP_TIMEOUT     30
#define DEFAULT_TCP_TIMEOUT     432000
#define DEFAULT_TCP_FIN_TIMEOUT 120
#define DEFAULT_ICMP_TIMEOUT    30
#define DEFAULT_DNS_TIMEOUT     30

#define PAL_NAT_BUF_SIZE        100
/*-----------------------------------------------------------------------------
**
** Types
*/


/*-----------------------------------------------------------------------------
**
** Functions
*/

#include "pal_fw.def"

/*-----------------------------------------------------------------------------
**
** Done
*/
#endif /* def _PAL_FW_H */
