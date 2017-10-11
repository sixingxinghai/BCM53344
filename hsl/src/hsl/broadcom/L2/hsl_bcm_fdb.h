/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_BCM_FDB_H_
#define _HSL_BCM_FDB_H_

/* Proto */
void hsl_bcm_fdb_addr_register (bcmx_l2_addr_t *addr, int insert, void *cookie);
void hsl_fdb_hw_cb_register (void);
void hsl_fdb_hw_cb_unregister (void);

#endif /* _HSL_BCM_FDB_H_ */

