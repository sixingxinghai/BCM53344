/*
 *  Generic parts
 *  Linux ethernet bridge
 *
 *  Authors:
 *  Lennert Buytenhek   <buytenh@gnu.org>
 *
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#if 0 /* XXX */
#include <linux/brlock.h>
#endif
#include <asm/uaccess.h>
#include <config.h>
#include "if_ipifwd.h"
#include "br_types.h"
#include "br_input.h"
#include "br_ioctl.h"
#include "br_fdb.h"
#include "br_notify.h"
#include "bdebug.h"
#ifdef HAVE_CFM
#include "af_cfm.h"
#endif /* HAVE_CFM */
#ifdef HAVE_ELMID
#include "af_elmi.h"
#endif /* HAVE_ELMID */

/* AF_STP socket family functions */
extern int stp_init(void);
extern void stp_exit(void);

/* AF_GARP socket family functions */
extern int garp_init(void);
extern void garp_exit(void);

/* AF_IGMP_SNOOP socket family functions */
extern int igmp_snoop_init(void);
extern void igmp_snoop_exit(void);

#ifdef HAVE_EFM
/* AF_EFMOAM socket family functions */
extern int efm_init(void);
extern void efm_exit(void);
#endif /* HAVE_EFM */

#ifdef HAVE_ELMID
/* AF_ELMI socket family functions */
extern int elmi_init(void);
extern void elmi_exit(void);
#endif /* HAVE_ELMID */

#ifdef HAVE_CFM
/* AF_CFM socket family functions */
extern int cfm_init(void);
extern void cfm_exit(void);
#endif /* HAVE_CFM */
/* This function registers the bridge with the Linux OS */

int 
br_init (void)
{
  BDEBUG("APN Ethernet Bridge\n");
  stp_init();
  garp_init();
  igmp_snoop_init();
#ifdef HAVE_EFM
  efm_init ();
#endif /* HAVE_EFM */
#ifdef HAVE_CFM
  cfm_init ();
#endif /* HAVE_CFM */

#ifdef HAVE_ELMID
  elmi_init ();
#endif /* HAVE_ELMID */

  register_apn_handle_frame_hook(br_handle_frame);
  register_netdevice_notifier (&br_device_notifier);

  return 0;
}

static void 
br_clear_frame_hook (void)
{
  unregister_apn_handle_frame_hook();
}

/* This function unregisters the bridge with the Linux OS */

void 
br_exit (void)
{
  stp_exit();
  garp_exit();
  igmp_snoop_exit();

#ifdef HAVE_CFM
  cfm_exit();
#endif /* HAVE_CFM */

#ifdef HAVE_EFM
  efm_exit ();
#endif /* HAVE_EFM */

#ifdef HAVE_ELMID
  elmi_exit ();
#endif /* HAVE_ELMID */

  unregister_netdevice_notifier (&br_device_notifier);
}
