/* Copyright (C) 2004   All Rights Reserved. */

#ifndef _HSL_BCM_IFDB_H_
#define _HSL_BCM_IFDB_H_

/* Callback for port attachment message. */
int hsl_bcm_ifcb_port_attach (bcmx_lport_t lport, int unit, bcm_port_t port,
                                uint32 flags);

/* Callback for port detachment message. */
int hsl_bcm_ifcb_port_detach (bcmx_lport_t lport, bcmx_uport_t uport);

/* Callback for link scan message. */
int hsl_bcm_ifcb_link_scan (bcmx_lport_t lport, bcm_port_info_t *info);

#endif /* _HSL_BCM_IFDB_H_ */
