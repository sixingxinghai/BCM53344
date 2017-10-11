/* Copyright (C) 2009  All Rights Reserved.  */

#include "pal.h"
#include "lib.h"
#include "nsmd.h"
#include "nsm_fea.h"
#include "nsm_interface.h"


/*------------------------------------------------------------
 * _nsm_ifma_install_primary - Local function to set MAC address
 *                             in the HAL adn PAL.
 *
 * NOTE: From PacOS we change only address in PAL when we toggle
 *       the physical address with virtual one.
 */
static ZRESULT
_nsm_ifma_install_primary(struct nsm_if *nif)
{
  NSM_IFMA_VEC *mav = &nif->nsm_ifma_vec;
  NSM_IFMA     *pma = &mav->mav_vec[NSM_IFMA_TYPE_PHYSICAL];
  NSM_IFMA     *lma = &mav->mav_vec[NSM_IFMA_TYPE_LOGICAL];
  NSM_IFMA     *vma = &mav->mav_vec[NSM_IFMA_TYPE_VIRTUAL];
  NSM_IFMA_TYPE ma_type = NSM_IFMA_TYPE_UNSET;
  u_int8_t     *ma_tab[2] = {NULL,NULL};
  ZRESULT       ret;

  if (lma->ma_is_set)
    {
      ma_type   = NSM_IFMA_TYPE_LOGICAL;
      ma_tab[0] = lma->ma_addr;
    }
  else if (pma->ma_is_set)
    {
      ma_type   = NSM_IFMA_TYPE_PHYSICAL;
      ma_tab[0] = pma->ma_addr;
    }
  else
    return ZRES_ERR;

  /* If VMAC is set, we will force it into PAL. */
  if (vma->ma_is_set)
    ma_tab[1] = vma->ma_addr;

  ret = nsm_fea_if_set_primary_hwaddr(nif->ifp, ma_tab, mav->mav_len);
  if (ret == ZRES_OK)
    {
      mav->mav_type = ma_type;
      /* Set the same in the */
    }
  return (ret);
}

/*------------------------------------------------------------
 * nsm_ifma_set_physical - Address set by the hardware or OS
 */

ZRESULT
nsm_ifma_set_physical(struct nsm_if *nif,
                      u_int8_t      *pma_addr,
                      s_int32_t      pma_len)
{
  NSM_IFMA_VEC *mav = &nif->nsm_ifma_vec;
  NSM_IFMA     *pma = &mav->mav_vec[NSM_IFMA_TYPE_PHYSICAL];

  if (pma_len > INTERFACE_HWADDR_MAX)
    return ZRES_ERR;

  /* Save the address in the container.
     We may save the physical address only once.
  */
  if (! pma->ma_is_set)
    {
      pal_mem_cpy(pma->ma_addr, pma_addr, pma_len);
      pma->ma_is_set = PAL_TRUE;
    }
  switch (mav->mav_type)
    {
    case NSM_IFMA_TYPE_UNSET:
    case NSM_IFMA_TYPE_PHYSICAL:
      /* Make it the primary MAC. */
      mav->mav_type = NSM_IFMA_TYPE_PHYSICAL;
      mav->mav_len  = pma_len;
      /* Shall we update the protocols? */
      break;

#if defined(HAVE_L3) && defined(HAVE_INTERVLAN_ROUTING)
    case NSM_IFMA_TYPE_LOGICAL:
      break;
#endif /* defined(HAVE_L3) && defined(HAVE_INTERVLAN_ROUTING) */
      break;
    default:
      return ZRES_ERR;
      break;
    }
  return ZRES_OK;
}

/* Returns a pointer to the buf (if present), ptr to the internal storage (if
   buf eq. NULL and address set), or NULL.
 */
u_int8_t *
nsm_ifma_get_physical(struct nsm_if *nif,
                      u_int8_t      *buf,
                      s_int32_t     *buf_len)
{
  NSM_IFMA_VEC *mav = &nif->nsm_ifma_vec;
  NSM_IFMA     *pma = &mav->mav_vec[NSM_IFMA_TYPE_PHYSICAL];
  u_int8_t     *ret_ptr;

  if (mav->mav_type == NSM_IFMA_TYPE_UNSET)
    return NULL;
  if (! pma->ma_is_set)
    return NULL;
  if (buf && *buf_len < mav->mav_len)
    return NULL;

  ret_ptr = buf;
  if (buf)
    pal_mem_cpy(buf, pma->ma_addr, mav->mav_len);
  else
    ret_ptr = pma->ma_addr;

  if (buf_len)
    *buf_len = mav->mav_len;

  return (ret_ptr);
}

bool_t
nsm_ifma_is_physical_set(struct nsm_if *nif)
{
  NSM_IFMA_VEC *mav = &nif->nsm_ifma_vec;
  NSM_IFMA     *pma = &mav->mav_vec[NSM_IFMA_TYPE_PHYSICAL];

  return (pma->ma_is_set);
}

#if defined(HAVE_L3) && defined(HAVE_INTERVLAN_ROUTING)

/*----------------------------------------------------------------
 *                              LOGICAL
 *----------------------------------------------------------------
 * nsm_ifma_set_logical - Setting logical MAC address.
 *
 * Purpose:
 *   This is a management API function.
 *   To set a logical MAC address on the interface.
 *   The address will be installed on a given interface if no Virtual
 *   MAC is already installed.
 */
ZRESULT
nsm_ifma_set_logical(char      *if_name,
                     u_int8_t  *lmac_addr,
                     s_int32_t  lmac_len)
{
  struct nsm_if *nif = NULL;
  NSM_IFMA_VEC  *mav = NULL;
  NSM_IFMA      *lma = NULL;
  struct interface *ifp;

  if (lmac_len > INTERFACE_HWADDR_MAX)
    return ZRES_ERR;

  ifp = ifg_lookup_by_name (&nzg->ifg, if_name);
  if (! ifp)
    return ZRES_ERR;

  if (! (nif = ifp->info))
    return ZRES_ERR;

  mav = &nif->nsm_ifma_vec;

  if (! mav->mav_len)
    mav->mav_len = lmac_len;

  lma  = &mav->mav_vec[NSM_IFMA_TYPE_LOGICAL];

  lma->ma_is_set = PAL_TRUE;
  pal_mem_cpy(lma->ma_addr, lmac_addr, lmac_len);

  _nsm_ifma_install_primary(nif);
  return ZRES_OK;
}

u_int8_t *
nsm_ifma_get_logical(struct nsm_if *nif,
                     u_int8_t      *buf,
                     s_int32_t     *buf_len)
{
  NSM_IFMA_VEC *mav = &nif->nsm_ifma_vec;
  NSM_IFMA     *lma = &mav->mav_vec[NSM_IFMA_TYPE_LOGICAL];
  u_int8_t     *ret_ptr;

  if (! lma->ma_is_set)
    return NULL;
  if (buf && *buf_len < mav->mav_len)
    return NULL;

  ret_ptr = buf;
  if (buf)
    pal_mem_cpy(buf, lma->ma_addr, mav->mav_len);
  else
    ret_ptr = lma->ma_addr;

  if (buf_len)
    *buf_len = mav->mav_len;

  return (ret_ptr);
}

ZRESULT
nsm_ifma_del_logical(char *if_name)
{
  struct nsm_if *nif = NULL;
  NSM_IFMA_VEC  *mav = NULL;
  NSM_IFMA      *lma = NULL;
  struct interface *ifp;

  ifp = ifg_lookup_by_name (&nzg->ifg, if_name);
  if (! ifp)
    return ZRES_ERR;

  if (! (nif = ifp->info))
    return ZRES_ERR;

  mav = &nif->nsm_ifma_vec;
  lma = &mav->mav_vec[NSM_IFMA_TYPE_LOGICAL];
  lma->ma_is_set = PAL_FALSE;
  pal_mem_set(lma->ma_addr, 0, sizeof(lma->ma_addr));
  _nsm_ifma_install_primary(nif);
  return ZRES_OK;
}

bool_t
nsm_ifma_is_logical_set(struct nsm_if *nif)
{
  NSM_IFMA_VEC *mav = &nif->nsm_ifma_vec;
  NSM_IFMA     *lma = &mav->mav_vec[NSM_IFMA_TYPE_LOGICAL];

  return (lma->ma_is_set);
}
#endif

#ifdef HAVE_VRRP
/*----------------------------------------------------------------
 *                              VIRTUAL
 *----------------------------------------------------------------
 * Not suitable for installation of two virtual addresses at the same
 * time.
 */
ZRESULT
nsm_ifma_set_virtual(struct nsm_if *nif,
                     u_int8_t      *vmac_addr,
                     s_int32_t      vmac_len)
{
  NSM_IFMA_VEC *mav = &nif->nsm_ifma_vec;
  NSM_IFMA     *vma = &mav->mav_vec[NSM_IFMA_TYPE_VIRTUAL];

  if (vmac_len > INTERFACE_HWADDR_MAX)
    return ZRES_ERR;

  vma->ma_is_set = PAL_TRUE;
  pal_mem_cpy(vma->ma_addr, vmac_addr, vmac_len);

  nsm_fea_if_set_virtual_hwaddr(nif->ifp, vmac_addr, vmac_len);
  return ZRES_OK;
}

/* Delete virtual address. Replace with the most preferable if needed.
 */
ZRESULT
nsm_ifma_del_virtual(struct nsm_if *nif,
                     u_int8_t      *vmac_addr,
                     s_int32_t      vmac_len)
{
  NSM_IFMA_VEC *mav = &nif->nsm_ifma_vec;
  NSM_IFMA     *pma = &mav->mav_vec[NSM_IFMA_TYPE_PHYSICAL];
  NSM_IFMA     *lma = &mav->mav_vec[NSM_IFMA_TYPE_LOGICAL];

  NSM_IFMA     *vma = &mav->mav_vec[NSM_IFMA_TYPE_VIRTUAL];

  u_int8_t *ma_tab[2] = {NULL, NULL};

  if (! vma->ma_is_set)
    return ZRES_OK;

  ma_tab[1] = vma->ma_addr;
  vma->ma_is_set = PAL_FALSE;

  /* We will provide to fea function the primary and thr virtual address. */
  switch (mav->mav_type)
    {
    case NSM_IFMA_TYPE_PHYSICAL:
      ma_tab[0] = pma->ma_addr;
      break;
    case NSM_IFMA_TYPE_LOGICAL:
      ma_tab[0] = lma->ma_addr;
      break;
    default:
      break;
    }
  nsm_fea_if_del_virtual_hwaddr(nif->ifp, ma_tab, vmac_len);
  return ZRES_OK;
}

bool_t
nsm_ifma_is_virtual_set(struct nsm_if *nif)
{
  NSM_IFMA_VEC *mav = &nif->nsm_ifma_vec;
  NSM_IFMA     *vma = &mav->mav_vec[NSM_IFMA_TYPE_VIRTUAL];

  return (vma->ma_is_set);
}

#endif

/*--------------------------------------------------------------------
 * nsm_ifma_make - Called by nsm_if at the time of creation.
 *                 The ifp may already store the hw_addr.
 *
 */
void
nsm_ifma_init(struct nsm_if *nif)
{
  struct interface *ifp = nif->ifp;

  if (! ifp || ! ifp->hw_addr_len)
    return;

  /* If the interface has address set, it can be any type of address.
     In HA system we do not care - the checkpoint will arrive with the
     right values for the NSM_IFMA_VEC, and it may override the non-physical
     address set as physical.
     In not-HA system we assume this may only be a physical address.
     Otherwise, nothing makes sense here.
   */
  nsm_ifma_set_physical(nif, ifp->hw_addr, ifp->hw_addr_len);
}

/*--------------------------------------------------------------------
 * nsm_ifma_close - Called by nsm_if at the time of deletion of the nsm_if
 */
void
nsm_ifma_close(struct nsm_if *nif)
{
  struct interface *ifp = nif->ifp;

  if (! ifp)
    return;

#if defined(HAVE_L3) && defined(HAVE_INTERVLAN_ROUTING)
  /* Force physical address. */
  nsm_ifma_del_logical(ifp->name);
#endif
}

/*--------------------------------------------------------------------
 * nsm_ifma_update - Called by nsm_if at the time of update.
 *                   We will interpret the ifp hw_addr depending on the
 *                   current NSM_IFMA_VEC state.
 *
 */
void
nsm_ifma_update(struct nsm_if *nif)
{
  NSM_IFMA_VEC *mav = &nif->nsm_ifma_vec;
  struct interface *ifp = nif->ifp;

  if (! ifp)
    return;

  /* If the curret hw_addr is same as the most preferable one, its fine.
     Otherwise, we must override and set the most preferable address.

     In HA system we do not care - the checkpoint will arrive with the
     right values for the NSM_IFMA_VEC
     In a not HA system we assume this may only be a physical address. Otherwise,
     nothing makes sense here.
   */
  if (! ifp->hw_addr_len)
    {
      /* Interface does not know the address - maybe we know it.
      */
      switch (mav->mav_type)
        {
        case NSM_IFMA_TYPE_UNSET:
          return;

        case NSM_IFMA_TYPE_PHYSICAL:
        case NSM_IFMA_TYPE_LOGICAL:
          _nsm_ifma_install_primary(nif);
      default:
        return;
        break;
      }
    }
  else
    {
      if (mav->mav_type == NSM_IFMA_TYPE_UNSET)
        nsm_ifma_set_physical(nif, ifp->hw_addr, ifp->hw_addr_len);
    }
}

char *
nsm_ifma_to_str(u_int8_t *ma, char *buf, int buf_len)
{
  if (buf_len < 15)
    return ("(encode-err)");
  else
    pal_sprintf(buf, "%02x%02x.%02x%02x.%02x%02x",
                ma[0], ma[1], ma[2], ma[3], ma[4], ma[5]);
  return buf;
}

/* Just init the specified type of address. */
void
nsm_ifma_make(struct nsm_if *nif,
              NSM_IFMA_TYPE  type,
              bool_t         is_set,
              u_int8_t       *ma)
{
  NSM_IFMA_VEC *mav = &nif->nsm_ifma_vec;
  NSM_IFMA     *xma = NULL;

  if (type < NSM_IFMA_TYPE_PHYSICAL || type > NSM_IFMA_TYPE_VIRTUAL)
    return;

  xma = &mav->mav_vec[type];
  xma->ma_is_set = is_set;
  if (is_set)
    pal_mem_cpy(xma->ma_addr, ma, sizeof(xma->ma_addr));
  else
    pal_mem_set(xma->ma_addr, 0, sizeof(xma->ma_addr));
}

