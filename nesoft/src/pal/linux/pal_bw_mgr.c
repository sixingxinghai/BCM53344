/* Copyright (C) 2006 i All Rights Reserved. */
#include "pal.h"
#include "pal_bw_mgr.h"

#ifdef HAVE_GMPLS

bool_t
apn_bw_link_properties_same (struct link_properties *lp1, 
                             struct link_properties *lp2)
{
  s_int32_t ii = 0;

#ifdef HAVE_PACKET
  if (lp1->max_lsp_size != lp2->max_lsp_size)
    return PAL_FALSE;
#endif /* HAVE_PACKET */
  
  if (lp1->bandwidth != lp2->bandwidth)
    return PAL_FALSE;

  if (lp1->max_resv_bw != lp2->max_resv_bw)
    return PAL_FALSE;

  while (ii < MAX_PRIORITIES)
    {
      if (lp1->max_lsp_bw [ii] != lp2->max_lsp_bw [ii])
        return PAL_FALSE;

      if (lp1->tecl_priority_bw [ii] != lp2->tecl_priority_bw [ii])
        return PAL_FALSE;

      ii ++;
    }

#if defined HAVE_DSTE && defined HAVE_PACKET
  ii = 0;
  while (ii < MAX_BW_CONST)
    {
      if (lp1->bw_constraint [ii] != lp2->bw_constraint [ii])
        return PAL_FALSE;

      ii ++;
    }
#endif /* HAVE_DSTE && HAVE_PACKET */

  return PAL_TRUE;
}

s_int32_t
apn_bw_derive_te_link (struct telink *tel, bool_t *change)
{
  struct avl_node *node;
  struct datalink *dl;
  bool_t rcv_change = PAL_FALSE;
  struct link_properties *lprop;

  /* Before updating any information copy the information in the structure. 
    Only in case of change an update needs to be sent across */
  tel->bnd_dl = 0;

  lprop = XMALLOC (MTYPE_TMP, sizeof (struct link_properties));
  if (lprop == NULL)
    return -1;

  /* Copy information in lprop */
  *lprop = tel->prop.lpro;
  for (node = avl_top (&tel->dltree); node; node = avl_next (node))
    {
      dl = (struct datalink *) node->info;

      /* Do property validation before updating bandwidth */
      if (dl == NULL)
        continue;
      
      if (!CHECK_FLAG (dl->status, GMPLS_INTERFACE_UP))
        /* No property updates required because datalink is not up */
        continue;

      /* Property mismatched links cannot be considered for the bundle */
      if (tel->bnd_dl != 0 && 
          CHECK_FLAG (dl->flags, GMPLS_DATA_LINK_PROP_MISMATCH))
        continue;

      rcv_change = PAL_FALSE;     
      apn_bw_add_te_link_change (tel, dl, &rcv_change);
      tel->bnd_dl ++;
    } 
   
  *change = !(apn_bw_link_properties_same (lprop, &tel->prop.lpro));

  XFREE (MTYPE_TMP, lprop);
  return 0;
}

#ifdef HAVE_PACKET
/* Max LSP Size */
s_int32_t
apn_bw_te_link_get_max_lsp_size (struct telink *tel, bool_t *change)
{
  struct avl_node *node;
  struct datalink *dl;
  u_int16_t cnt = 0, sz = 0;

  if (tel->num_dl != 0) 
    sz = tel->prop.lpro.max_lsp_size;

  /* Find the Maximum LSP size for the TE link */
  for (node = avl_top (&tel->dltree); node; node = avl_next (node))
    {
      dl = (struct datalink *) node->info;

      /* Do property validation before updating bandwidth */
      if (dl == NULL)
        continue;

      if (!CHECK_FLAG (dl->status, GMPLS_INTERFACE_UP))
        /* No property updates required because datalink is not up */
        continue;

      /* Property mismatched links cannot be considered for the bundle */
      if (tel->bnd_dl != 0 &&
          CHECK_FLAG (dl->flags, GMPLS_DATA_LINK_PROP_MISMATCH))
        continue;

      /* Now get the Max LSP size */
      if ((cnt == 0) || 
          (tel->prop.lpro.max_lsp_size > dl->prop->max_lsp_size))
        {
          tel->prop.lpro.max_lsp_size = dl->prop->max_lsp_size;
        }
      cnt ++;
    }

  if (sz != tel->prop.lpro.max_lsp_size)
    *change = PAL_TRUE;

  return 0;
}
#endif /* HAVE_PACKET */

s_int32_t
apn_bw_te_link_get_max_lsp_bw (struct telink *tel, u_char prio, 
                               bool_t *change)
{
  struct avl_node *node;
  struct datalink *dl;
  u_int16_t cnt = 0;
  float32_t bw = 0;

  if (tel->num_dl != 0)
    bw = tel->prop.lpro.max_lsp_size;

  /* Find the Maximum LSP size for the TE link */
  for (node = avl_top (&tel->dltree); node; node = avl_next (node))
    {
      dl = (struct datalink *) node->info;

      /* Do property validation before updating bandwidth */
      if (dl == NULL)
        continue;

      if (!CHECK_FLAG (dl->status, GMPLS_INTERFACE_UP))
        /* No property updates required because datalink is not up */
        continue;

      /* Property mismatched links cannot be considered for the bundle */
      if (tel->bnd_dl != 0 &&
          CHECK_FLAG (dl->flags, GMPLS_DATA_LINK_PROP_MISMATCH))
        continue;

      /* Now get the Max LSP size */
      if ((cnt == 0) ||
          (tel->prop.lpro.max_lsp_bw [prio] >  dl->prop->max_lsp_bw [prio]))
        {
          tel->prop.lpro.max_lsp_bw [prio] =  dl->prop->max_lsp_bw [prio];
        }
      cnt ++;
    }

  if (bw != tel->prop.lpro.max_lsp_size)
    *change = PAL_TRUE;

  return 0;
}

/* This is called when a new link is added */
s_int32_t
apn_bw_del_te_link_change (struct telink *tel, struct datalink *dl,
                           bool_t *change)
{
  u_char prio = 0;

  /* change is initialized to PAL_FALSE outside the function */
  if (!tel || !dl)
    return -1;

  /* If first active data link copy the properties */
  if (tel->bnd_dl == 1)
    {
      /* If the only link is being deleted - just change the count to 0.
         Nothing more needs to be done */
      *change = 0;
      tel->bnd_dl = 0;
      return 0;
    }

  /* Update values based on */
#ifdef HAVE_PACKET
  if ((TE_LINK_IS_PACKET_CAPABLE (tel)) &&
      (tel->prop.lpro.max_lsp_size > dl->prop->max_lsp_size))
    {
      /* Update */
      apn_bw_te_link_get_max_lsp_size (tel, change);
    }
#endif /* HAVE_PACKET */

  /* Nothing needs to be done with bandwidth. The maximum LSP Bandwidth
     replaces the Maximum Bandwidth for bundled links. */

  /* The Maximum Reservable Bandwidth of the bundled link
   is set to the sum of the Maximum Reservable Bandwidths of all
   component links associated with the bundled link. */
  tel->prop.lpro.max_resv_bw -= dl->prop->max_resv_bw;

  /* Update Unreserved Bandwidth */
  prio = 0;
  while (prio < MAX_PRIORITIES)
    {
      tel->prop.lpro.tecl_priority_bw [prio] -=  dl->prop->max_lsp_bw [prio];
      *change = PAL_TRUE;
      prio ++;
    }

  /* RFC4201: The Maximum LSP Bandwidth of a bundled link at
   priority p is defined to be the maximum of the Maximum LSP Bandwidth
   at priority p of all of its component links. */
  prio = 0;
  while (prio < MAX_PRIORITIES)
    {
      apn_bw_te_link_get_max_lsp_bw (tel, prio, change);
      prio ++;
    }
  return 0;
}

/* This is called when a new link is added */
s_int32_t
apn_bw_add_te_link_change (struct telink *tel, struct datalink *dl,
                           bool_t *change)
{
  u_char prio = 0;

  /* change is initialized to PAL_FALSE outside the function */
  if (!tel || !dl)
    return -1;

  /* If first active data link copy the properties */
  if (tel->bnd_dl == 0)
    {
      if (dl->prop)
        tel->prop.lpro = *dl->prop;
      return 0;
    }

#ifdef HAVE_PACKET 
  if ((TE_LINK_IS_PACKET_CAPABLE (tel)) &&
      tel->prop.lpro.max_lsp_size > dl->prop->max_lsp_size)
    {
      /* Update */
      *change = PAL_TRUE;
      tel->prop.lpro.max_lsp_size = dl->prop->max_lsp_size;
    }
#endif /* HAVE_PACKET */

  /* Nothing needs to be done with bandwidth. The maximum LSP Bandwidth 
     replaces the Maximum Bandwidth for bundled links. */
    
  /* The Maximum Reservable Bandwidth of the bundled link
   is set to the sum of the Maximum Reservable Bandwidths of all
   component links associated with the bundled link. */
  tel->prop.lpro.max_resv_bw += dl->prop->max_resv_bw;
 
  /* Update Unreserved Bandwidth */
  prio = 0;
  while (prio < MAX_PRIORITIES)
    {
      tel->prop.lpro.tecl_priority_bw [prio] +=  dl->prop->max_lsp_bw [prio];
      *change = PAL_TRUE;
      prio ++;
    }

  /* RFC4201: The Maximum LSP Bandwidth of a bundled link at
   priority p is defined to be the maximum of the Maximum LSP Bandwidth
   at priority p of all of its component links. */
  prio = 0;
  while (prio < MAX_PRIORITIES)
    {
      if (tel->prop.lpro.max_lsp_bw [prio] <  dl->prop->max_lsp_bw [prio])
        {
          tel->prop.lpro.max_lsp_bw [prio] =  dl->prop->max_lsp_bw [prio];
          *change = PAL_TRUE;
        }
      prio ++;
    }
  
  return 0;
}

bool_t
apn_bw_allow_resv_on_te_link (struct telink *tel, float32_t bw, 
                              u_char setup_prio)
{
  bool_t allow = PAL_FALSE;

  if (tel->bnd_dl == 0 || setup_prio >= MAX_PRIORITIES)
    {
      /* If no Data link */
      return allow;
    }

  /* Check if the TE link satsfies the constraint */
  if (tel->prop.lpro.max_lsp_bw [setup_prio] >= bw &&
      bw >=  tel->prop.phy_prop.min_lsp_bw)
    {
      allow = PAL_TRUE;
    }
  return allow;
}

s_int32_t
apn_bw_resv_on_te_link (struct telink *tel, float32_t bw, u_char setup_prio,
                        struct datalink *dlink)
{
  struct avl_node *node;
  struct datalink *dl;

  dlink = NULL;
  if (tel->bnd_dl == 0 || setup_prio >= MAX_PRIORITIES)
    {
      /* If no Data link */
      return -1;
    }

  /* Check if the TE link satsfies the constraint */
  if (tel->prop.lpro.max_lsp_bw [setup_prio] < bw ||
      bw <  tel->prop.phy_prop.min_lsp_bw)
    {
      return -1;
    }

  /* Walk through the tree of datalinks and find the first data link 
     satisfing the property request */
  for (node = avl_top (&tel->dltree); node; node = avl_next (node))
    {
      dl = (struct datalink *) node->info;
      if (dl == NULL || dl->ifp == NULL)
        {
          continue;
        }

      if (!CHECK_FLAG (dl->status, GMPLS_INTERFACE_UP))
        /* No property updates required because datalink is not up */
        {
          continue;
        }

      /* Property mismatched links cannot be considered for the bundle */
      if (CHECK_FLAG (dl->flags, GMPLS_DATA_LINK_PROP_MISMATCH))
        {
          continue;
        }

      /* Link cannot satify the bandwidth properties */
      if ((dl->prop->max_lsp_bw [setup_prio] < bw) ||
          (bw < dl->ifp->phy_prop.min_lsp_bw))
        {
          continue;
        }

      /* Now time to update the properties. We have found the datalink
         we were looking for.*/
      dl->prop->tecl_priority_bw [setup_prio] -= bw;

#ifdef HAVE_PACKET
      if ((!TE_LINK_IS_PACKET_CAPABLE (tel)) ||
          (dl->prop->max_lsp_size > dl->prop->tecl_priority_bw [setup_prio]))
#endif /* HAVE_PACKET */
        {
          dl->prop->max_lsp_bw [setup_prio] = 
                               dl->prop->tecl_priority_bw [setup_prio];     
        }
      break;
    }
 
  dlink = dl;
  return 0;
}

s_int32_t
apn_bw_update_data_link_bw (struct interface *ifp, struct datalink *dl)
{
  u_char prio;

#if defined (HAVE_DSTE) && defined (HAVE_PACKET)
  u_char class;
#endif /* HAVE_DSTE && HAVE_PACKET */

  /* Datalink properties can be derived from interface properties. Number 
     of datalinks derived from a TE link needs to be set before calling
     this function  */
  if (ifp->num_dl == 0)
    return -1;
  
  switch (ifp->phy_prop.encoding_type)
    {
      case GMPLS_ENCODING_TYPE_PACKET:
        /* For packet encoding allow any */
        break;
      case GMPLS_ENCODING_TYPE_ETHERNET:
      default:
        return -1;
    }

  dl->prop->bandwidth = ifp->bandwidth / ifp->num_dl;

  prio = 0;
  while (prio < MAX_PRIORITIES)
    {
      dl->prop->tecl_priority_bw [prio] = 
                      ifp->tecl_priority_bw [prio] / ifp->num_dl;

      if (dl->prop->tecl_priority_bw [prio] < dl->prop->max_lsp_size)
        dl->prop->max_lsp_bw [prio] = dl->prop->tecl_priority_bw [prio];
      else
        dl->prop->max_lsp_bw [prio] = dl->prop->max_lsp_size;

      prio ++;
    }

#if defined (HAVE_DSTE) && defined (HAVE_PACKET)
  class = 0;
  while (class < MAX_BW_CONST)
    {
      if (ifp->num_dl)
        dl->prop->bw_constraint [class] = ifp->bw_constraint [class] / 
                                                    ifp->num_dl;
      class ++;
    }
#endif /* HAVE_DSTE && HAVE_PACKET */
  return 0;
}                  

#endif /* HAVE_GMPLS */

/* End of File */

