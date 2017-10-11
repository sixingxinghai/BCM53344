/* Copyright (C) 2009  All Rights Reserved.  */

#ifndef _NSM_IFMA_H_
#define _NSM_IFMA_H_


#include "pal.h"
#include "lib.h"
#include "nsm_interface.h"

struct nsm_if;

typedef enum nsm_ifma_type
{
  NSM_IFMA_TYPE_UNSET    = -1,
  NSM_IFMA_TYPE_PHYSICAL = 0,
  NSM_IFMA_TYPE_LOGICAL  = 1,
  NSM_IFMA_TYPE_VIRTUAL  = 2,
  NSM_IFMA_TYPE_TOTAL,

} NSM_IFMA_TYPE;

/*------------------------------------------------------------------
 * NSM_IFMA_ADDR - Maybe it is not necessary except it allows as to assign
 *               address to another address.
 */
typedef struct nsm_ifma
{
  bool_t   ma_is_set;
  u_int8_t ma_addr[INTERFACE_HWADDR_MAX];
} NSM_IFMA;

/*------------------------------------------------------------------
 * This is a container of all configured MAC addresses for an interface.
 * At any time only one address is active. Such address is always stored
 * in the ifp->hw_addr field.
 *
 * Generally, a precedence of addresses is as follows:
 *
 *  Virtual - the highest
 *  Logical
 *  Physical - the lowest
 *
 * Every time the higher prefence address becomes activated, it preempts
 * the current MAC address and it is installed in the interface staructure
 * as hw_addr, in the kernel and the HAL (as a primary - if the HAL is in
 * the build).
 *
 */

typedef struct nsm_ifma_vec
{
  NSM_IFMA_TYPE   mav_type;
  s_int32_t       mav_len;
  NSM_IFMA        mav_vec[NSM_IFMA_TYPE_TOTAL];
} NSM_IFMA_VEC;

/*------------------------------------------------------------
 * nsm_ifma_set_physical - Address set by the hardware or OS
 */

ZRESULT nsm_ifma_set_physical(struct nsm_if *nif,
                              u_int8_t      *pma_addr,
                              s_int32_t      pma_len);

/* Returns a pointer to the buf (if present), ptr to the internal storage (if
   buf eq. NULL and address set), or NULL.
 */
u_int8_t *nsm_ifma_get_physical(struct nsm_if *nif,
                                u_int8_t      *buf,
                                s_int32_t     *buf_len);
bool_t nsm_ifma_is_physical_set(struct nsm_if *nif);

#if defined(HAVE_L3) && defined(HAVE_INTERVLAN_ROUTING)
/*--------------------------------------------------------------------
 * nsm_ifma_set_logical - Setting logical MAC address.
 *
 * Purpose:
 *   This is a management API function.
 *   To set a logical MAC address on the interface.
 *   The address will be installed on a given interface if no Virtual
 *   MAC is already installed.
 */
ZRESULT nsm_ifma_set_logical(char      *if_name,
                             u_int8_t  *lmac_addr,
                             s_int32_t  lmac_len);
u_int8_t *nsm_ifma_get_logical(struct nsm_if *nif,
                               u_int8_t      *buf,
                               s_int32_t     *buf_len);
ZRESULT nsm_ifma_del_logical(char *if_name);
bool_t nsm_ifma_is_logical_set(struct nsm_if *nif);
#endif

#ifdef HAVE_VRRP
ZRESULT nsm_ifma_set_virtual(struct nsm_if *nif,
                             u_int8_t      *vmac_addr,
                             s_int32_t      vmac_len);
ZRESULT nsm_ifma_del_virtual(struct nsm_if *nif,
                             u_int8_t      *vmac_addr,
                             s_int32_t      vmac_len);
#endif
/*--------------------------------------------------------------------
 * nsm_ifma_init - Called by nsm_if at the time of creation.
 *                 The ifp may already store the hw_addr.
 *
 */
void nsm_ifma_init(struct nsm_if *nif);
void nsm_ifma_close(struct nsm_if *nif);

/*--------------------------------------------------------------------
 * nsm_ifma_update - Called by nsm_if at the time of update.
 *                   We will interpret the ifp hw_addr depending on the
 *                   current NSM_IFMA_VEC state.
 *
 */
void nsm_ifma_update(struct nsm_if *nif);

char *nsm_ifma_to_str(u_int8_t *ma, char *buf, int buf_len);

/* For HA to init specific MAC address.
 */
void nsm_ifma_make(struct nsm_if *nif,
                   NSM_IFMA_TYPE  type,
                   bool_t         is_set,
                   u_int8_t      *ma);

#endif /* _NSM_IFMA_H_ */

