/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_IMI_H
#define _PACOS_IMI_H

#include "line.h"
#include "modbmap.h"

#define IMI_ASSERT(expr)         pal_assert((expr))

/* Small macro to determine newline is newline only or linefeed needed.   *
 * NOTE: This is needed because VTY_NEWLINE uses 'vty' structure and this *
 * conflicts with global VTYSH 'vty' structure.                           */
#define IMI_NEWLINE  ((vty_user->type == VTY_TERM) ? "\r\n" : "\n")

/* Error types for IMI APIs. */
#define IMI_API_SUCCESS                         0
#define IMI_API_INVALID_ARG_ERROR               -1
#define IMI_API_DUPLICATE_ERROR                 -2
#define IMI_API_NOT_SET_ERROR                   -3
#define IMI_API_IPV4_ERROR                      -4
#define IMI_API_OUT_OF_RANGE_ERROR              -5
#define IMI_API_ORDER_ERROR                     -6
#define IMI_API_CANT_SET_ERROR                  -7
#define IMI_API_UNKNOWN_CMD_ERROR               -8
#define IMI_API_MEMORY_ERROR                    -9

/* Error types for PAL return. */
#define IMI_API_FILE_OPEN_ERROR                 -10
#define IMI_API_FILE_SEEK_ERROR                 -11
#define IMI_API_FILE_WRITE_ERROR                -12

/* Warnings. */
#define IMI_API_OUT_OF_RANGE_WARNING            -13
#define IMI_API_CONSOLIDATED_WARNING            -14

#define IMI_API_RANGE_NOT_FOUND_ERROR           -15
/* IMI Global pointer. */
#define IMI                                     imig

/* Data structures. */
#include "imi_cli.h"
#ifdef HAVE_DNS
#include "imi_dns.h"
#endif /* HAVE_DNS. */
#include "imi_interface.h"
#if defined(HAVE_DHCP_CLIENT) || defined(HAVE_DHCP_SERVER)
#include "imi_dhcp.h"
#include "imi/util/imi_fw.h"
#endif /* defined(HAVE_DHCP_CLIENT) || defined(HAVE_DHCP_SERVER) */
#ifdef HAVE_NAT
#include "imi_vs.h"
#endif /* HAVE_NAT. */
#ifdef HAVE_PPPOE
#include "imi_pppoe.h"
#endif /* HAVE_PPPOE */
#ifdef HAVE_IMI_DEFAULT_GW
#include "imi_util.h"
#endif /* HAVE_IMI_DEFAULT_GW. */
#ifdef HAVE_NTP
#include "imi_ntp.h"
#endif /* HAVE_NTP. */

enum imi_callback_type
{
  IMI_CALLBACK_CONNECT,
  IMI_CALLBACK_DISCONNECT,
  IMI_CALLBACK_MAX
};

/* IMI global structure.  */
struct imi_globals
{
  /* Library globals.  */
  struct lib_globals *zg;

  /* Active PMs.  */
  modbmap_t module;

#ifdef HAVE_DNS
  /* DNS. */
  struct imi_dns *dns;
#endif /* HAVE_DNS. */

#if defined(HAVE_DHCP_CLIENT) || defined(HAVE_DHCP_SERVER)
  /* DHCP. */
  struct imi_dhcp *dhcp;
#endif /* defined(HAVE_DHCP_CLIENT) || defined(HAVE_DHCP_SERVER) */

#ifdef HAVE_PPPOE
  /* PPPoE. */
  struct imi_pppoe *pppoe;
#endif /* HAVE_PPPOE */

#ifdef HAVE_IMI_DEFAULT_GW
  /* WAN Status. */
  enum wan_status wan_status;
#endif /* HAVE_IMI_DEFAULT_GW. */

  /* Callback functions.  */
  int (*imi_callback[IMI_CALLBACK_MAX]) (struct apn_vr *);
};

/* IMI master. */
struct imi_master
{
  /* Pointer to VR. */
  struct apn_vr *vr;

  /* Description */
  char *desc;

  /* Active PMs for this VR.  */
  modbmap_t module;

  /* Configured PMs for this VR.  */
  modbmap_t module_config;

  /* Filename for configuration. */
  char *filename;

#ifdef HAVE_VLOGD
  /* Local to VR log file name. */
  char *im_vlog_fname;
#endif

  /* Vector for IMI line connection.  */
  vector lines[LINE_TYPE_MAX];

  /* Set after NMS is caming with the VR add msg. */
  bool_t im_nsm_add_flag;
};

/* Global Variable. */
extern struct lib_globals *imim;
extern struct imi_globals *imig;
extern int boot_flag;

/* Prototypes. */
void imi_access_list_init (struct lib_globals *);
void imi_prefix_list_init (struct lib_globals *);
struct imi_master *imi_master_init (struct apn_vr *);
void imi_master_finish (struct apn_vr *);
void imi_nsm_init (struct lib_globals *);
void imi_add_callback (struct lib_globals *, enum imi_callback_type,
                       int (*func) (struct apn_vr *));
void imi_config_file_set (struct apn_vr *, char *);
int imi_config_read (struct apn_vr *);

#ifdef HAVE_VLOGD
ZRESULT imi_vlog_init(struct lib_globals *zg);
#endif

#endif /* _PACOS_IMI_H */
