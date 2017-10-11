/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_OSPF_TE
#ifndef HAVE_OPAQUE_LSA
#error "Wrong ospfd config."
#endif /* HAVE_OSPF_TE */

#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "stream.h"
#include "linklist.h"
#include "tlv.h"
#include "lib/gmpls/gmpls.h"
#include "ospf_common/ospf_te_common.h"
#ifdef HAVE_OSPF_CSPF
#include "cspf_common.h"
#include "cspf_server.h"
#endif /* HAVE_OSPF_CSPF */

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_neighbor.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_debug.h"
#include "ospfd/ospf_te.h"
#ifdef HAVE_VRF_OSPF
#include "ospfd/ospf_vrf.h"
#endif /* HAVE_VRF_OSPF */
#ifdef HAVE_OSPF_CSPF
#include "ospfd/cspf/ospf_cspf.h"
#endif /* HAVE_OSPF_CSPF */


#ifdef HAVE_GMPLS
/* OSPF interface parameter related functions. */
struct ospf_tlink_params *
ospf_tlink_params_new ()
{
  struct ospf_tlink_params *otp;

  otp = XMALLOC (MTYPE_OSPF_TLINK_PARAMS, sizeof (struct ospf_tlink_params));
  pal_mem_set (otp, 0, sizeof (struct ospf_tlink_params));

  return otp;
}
#endif /* HAVE_GMPLS */

static struct ospf_interface *
ospf_te_find_interface_on_link (struct ospf_area *area, struct interface *ifp)
{
  struct ospf_interface *oi, *best_oi = NULL;
  struct listnode *node;
  
  for (node = LISTHEAD (area->iflist); node; NEXTNODE (node))
    {
      oi = GETDATA (node);
      
      if (oi->u.ifp != ifp || !ospf_if_is_up (oi))
        continue;

      best_oi = oi;
      
      if (oi->full_nbr_count)
        return oi;
    }

  return best_oi;
}

#define TLV_PAD(S, T) stream_put ((S), NULL, \
                                 TE_TLV_LENGTH ((struct te_tlv_header *)(T)) \
                                 - (pal_ntoh16 ((T)->header.tlv_len) + \
                                    sizeof (struct te_tlv_header)))

int
ospf_te_stlv_put_link_type (struct stream *s, struct ospf_interface *oi)
{
  struct te_tlv *tlv;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_LINK_TYPE_FIX_LEN)
    return 0;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_LINK_TYPE);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_LINK_TYPE_FIX_LEN);

  /* Set value. */
  if (oi->type == OSPF_IFTYPE_POINTOPOINT
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA)
    stream_putc (s, TE_LINK_TYPE_PTP);
  else
    stream_putc (s, TE_LINK_TYPE_MULTIACCESS);

  TLV_PAD (s, tlv);

  return 1;
}

int
ospf_te_stlv_put_link_id (struct stream *s, struct ospf_interface *oi)
{
  int set;
  struct ls_node *rn;
  struct ospf_neighbor *nbr;
  struct te_tlv *tlv;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_LINK_ID_FIX_LEN)
    return 0;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_LINK_ID);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_LINK_ID_FIX_LEN);

  /* Set value. */
  set = 0;
  if (oi->type == OSPF_IFTYPE_POINTOPOINT
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA)
    {
      for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
        if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
          if (stream_put_in_addr (s, &nbr->ident.router_id))
            {
              set = 1;
              ls_unlock_node (rn);
              break;
            }
    }
  else /* Multiaccess link. */
    if (stream_put_in_addr (s, &oi->ident.d_router))
      set = 1;

  if (! set)
    return 0;

  /* Make four-octet align. */
  TLV_PAD (s, tlv);

  return 1;
}

void
ospf_te_stlv_put_local_ip_addr (struct stream *s, struct ospf_interface *oi)
{
  int count;
  struct listnode *n;
  u_int32_t putp0, putp1;
  struct te_tlv *tlv;
  struct ospf_interface *oi_tmp;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_IP_ADDR_MIN_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  putp0 = stream_get_putp (s);
  stream_putw (s, TE_LINK_SUBTLV_LOCAL_ADDRESS);

  /* Set length. */
  putp1 = stream_get_putp (s);
  stream_putw (s, 0);

  /* Set value. */
  count = 0;
  for (n = LISTHEAD (oi->area->iflist); n; NEXTNODE (n))
    if ((oi_tmp = GETDATA (n)))
      if (oi_tmp->u.ifp == oi->u.ifp)
        if (stream_put_in_addr (s, &oi_tmp->ident.address->u.prefix4))
          count++;

  if (count)
    {
      /* Set length. */
      stream_putw_at (s, putp1, count * OSPF_TE_TLV_IP_ADDR_MIN_LEN);

      /* Make four-octet align. */
      TLV_PAD (s, tlv);
    }
  else
    stream_set_putp (s, putp0);
}

void
ospf_te_stlv_put_remote_ip_addr (struct stream *s, struct ospf_interface *oi)
{
  int count;
  struct listnode *n;
  u_int32_t putp0, putp1;
  struct te_tlv *tlv;
  struct ospf_interface *oi_tmp;
  struct ls_node *rn;
  struct ospf_neighbor *nbr;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_IP_ADDR_MIN_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  putp0 = stream_get_putp (s);
  stream_putw (s, TE_LINK_SUBTLV_REMOTE_ADDRESS);

  /* Set length. */
  putp1 = stream_get_putp (s);
  stream_putw (s, 0);

  /* Set value. */
  count = 0;
  for (n = LISTHEAD (oi->area->iflist); n; NEXTNODE (n))
    if ((oi_tmp = GETDATA (n)))
      if (oi_tmp->u.ifp == oi->u.ifp)
        for (rn = ls_table_top (oi_tmp->nbrs); rn; rn = ls_route_next (rn))
          if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
            if (nbr->state >= NFSM_Full)
              if (stream_put_in_addr (s, &nbr->ident.address->u.prefix4))
                count++;

  if (count)
    {
      /* Set length. */
      stream_putw_at (s, putp1, count * OSPF_TE_TLV_IP_ADDR_MIN_LEN);

      /* Make four-octet align. */
      TLV_PAD (s, tlv);
    }
  else
    /* Reset this TLV. */
    stream_set_putp (s, putp0);
}

void
ospf_te_stlv_put_te_metric (struct stream *s, struct ospf_interface *oi)
{
  struct te_tlv *tlv;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_METRIC_FIX_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_TE_METRIC);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_METRIC_FIX_LEN);

  /* Set value. */
  if (oi->te_metric > 0)
    stream_putl (s, oi->te_metric);
  else
    stream_putl (s, oi->output_cost);

  /* Make four-octet align. */
  TLV_PAD (s, tlv);
}

void
ospf_te_stlv_put_maximum_link_bw (struct stream *s, struct ospf_interface *oi)
{
  struct te_tlv *tlv;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_BW_FIX_LEN)
    return;
      
  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_MAX_BANDWIDTH);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_BW_FIX_LEN);

  /* Set value. */
  stream_putf (s, oi->u.ifp->bandwidth);

  /* Make four-octet align. */
  TLV_PAD (s, tlv);
}

void
ospf_te_stlv_put_reservable_link_bw (struct stream *s,
                                     struct ospf_interface *oi)
{
  struct te_tlv *tlv;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_BW_FIX_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_MAX_RES_BANDWIDTH);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_BW_FIX_LEN);

  /* Set value. */
  stream_putf (s, oi->u.ifp->max_resv_bw);

  /* Make four-octet align. */
  TLV_PAD (s, tlv);
}

void
ospf_te_stlv_put_unreserved_bw (struct stream *s, struct ospf_interface *oi)
{
  struct te_tlv *tlv;
  int i;

#ifdef HAVE_DSTE
  u_int32_t putp0;
  int count = 0;
#endif /* HAVE_DSTE */

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_UNRSV_BW_FIX_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
#ifdef HAVE_DSTE
  putp0 = stream_get_putp (s);
#endif /* HAVE_DSTE */
  stream_putw (s, TE_LINK_SUBTLV_UNRES_BANDWIDTH);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_UNRSV_BW_FIX_LEN);

  /* Set value. */
  for (i = 0; i < MAX_PRIORITIES; i++)
    {
#ifdef HAVE_DSTE
      if (oi->u.ifp->tecl_priority_bw[i] > 0)
        count++;
#endif /* HAVE_DSTE */

      stream_putf (s, oi->u.ifp->tecl_priority_bw[i]);
    }

#ifdef HAVE_DSTE
  if (count > 0)
    /* Make four-octet align. */
    TLV_PAD (s, tlv);
  else
    /* Reset this TLV */
    stream_set_putp (s, putp0);
#else
  /* Make four-octet align. */
  TLV_PAD (s, tlv);
#endif /* HAVE_DSTE */
}

void
ospf_te_stlv_put_admin_group (struct stream *s, struct ospf_interface *oi)
{
  struct te_tlv *tlv;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_RSRC_COLOR_FIX_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_RESOURCE_CLASS);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_RSRC_COLOR_FIX_LEN);

  /* Set value. */
  stream_putl (s, oi->u.ifp->admin_group);

  /* Make four-octet align. */
  TLV_PAD (s, tlv);
}

#ifdef HAVE_DSTE
void
ospf_te_stlv_put_bc (struct stream *s, struct ospf_interface *oi)
{
  struct te_tlv *tlv;
  u_int32_t putp0, putp1;
  int ctr, count;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_BC_MAX_LEN)
    return;
 
  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set Type */
  putp0 = stream_get_putp (s);
  stream_putw (s, TE_LINK_SUBTLV_BC);

  /* Set Length */
  putp1 = stream_get_putp (s);
  stream_putw (s, 0);

  /* Set BC Model */
  stream_putc (s, oi->u.ifp->bc_mode);

  for (ctr = MAX_BW_CONST; ctr > 0; ctr--)
    if (oi->u.ifp->bw_constraint[ctr - 1] > 0)
      break;

  /* Reset TLV if all bandwidth constraints are zero */
  if (ctr == 0)
    {
      stream_set_putp (s, putp0);
      return;
    }

  count = ctr;
  for (ctr = 0; ctr < count; ctr++)
    stream_putf (s, oi->u.ifp->bw_constraint[ctr]);

  /* Set Length */
  stream_putw_at (s, putp1, count * 4 + 1);

  /* Make 4-octet alignment */
  TLV_PAD (s, tlv);
}
#endif /* HAVE_DSTE */

int
ospf_te_tlv_put_router_addr (struct stream *s, struct ospf *top)
{
  struct te_tlv *tlv;

  tlv = (struct te_tlv *) STREAM_DATA (s);

  /* Set type. */
  stream_putw (s, TE_TLV_ROUTER_TYPE);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_RTADDR_FIX_LEN);

  /* Set value. */
  stream_put_in_addr (s, &top->router_id);

  /* Make four-octet align. */
  TLV_PAD (s, tlv);

  return 1;
}

#ifdef HAVE_GMPLS
/** @brief Function to encode te-link type sub TLV

    @param *s            - Stream buffer 
                            to be encoded
    @return   
       SUCCESS : 1
       FAILURE  : 0
*/
int
ospf_te_stlv_put_te_link_type (struct stream *s)
{
  struct te_tlv *tlv;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_LINK_TYPE_FIX_LEN)
    return 0;

  tlv = (struct te_tlv *)  STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_LINK_TYPE);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_LINK_TYPE_FIX_LEN);

  /* For TE-links link type is PTP */
  stream_putc (s, TE_LINK_TYPE_PTP);

  TLV_PAD (s, tlv);

  return 1;
}

/** @brief Function to encode te-link ID sub TLV

    @param *s            - Stream buffer 
                            to be encoded
    @param *ot           - OSFP telink structure
    @return   
       SUCCESS : 1
       FAILURE : 0
*/
int
ospf_te_stlv_put_te_link_id (struct stream *s, struct ospf_telink *ot)
{
  int set;
  struct te_tlv *tlv;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_LINK_ID_FIX_LEN)
    return 0;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_LINK_ID);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_LINK_ID_FIX_LEN);

  /* Set value. */
  set = 0;
  if (stream_put_in_addr (s, &ot->tlink->prop.linkid))
    set = 1;
      
  if (! set)
    return 0;

  /* Make four-octet align. */
  TLV_PAD (s, tlv);

  return 1;
}

/** @brief Function to encode te-link link local address sub TLV

    @param *s            - Stream buffer 
    @param *ot           - OSFP telink structure

    @return   void
*/
void
ospf_te_stlv_put_te_link_local_ip_addr (struct stream *s, 
                                  struct ospf_telink *ot)
{
  int count;
  u_int32_t putp0, putp1;
  struct te_tlv *tlv;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_IP_ADDR_MIN_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  putp0 = stream_get_putp (s);
  stream_putw (s, TE_LINK_SUBTLV_LOCAL_ADDRESS);

  /* Set length. */
  putp1 = stream_get_putp (s);
  stream_putw (s, 0);

  /* Set value. */
  count = 0;
  if (stream_put_in_addr (s, &ot->tlink->addr.u.prefix4))
    count++;

  if (count)
    {
      /* Set length. */
      stream_putw_at (s, putp1, count * OSPF_TE_TLV_IP_ADDR_MIN_LEN);

      /* Make four-octet align. */
      TLV_PAD (s, tlv);
    }
  else
    stream_set_putp (s, putp0);  
}

/** @brief Function to encode te-link remote ip address sub TLV

    @param *s            - Stream buffer
    @param *ot           - OSFP telink structure

    @return    void
*/
void
ospf_te_stlv_put_te_link_remote_ip_addr (struct stream *s,
                                   struct ospf_telink *ot)
{
  int count;
  u_int32_t putp0, putp1;
  struct te_tlv *tlv;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_IP_ADDR_MIN_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  putp0 = stream_get_putp (s);
  stream_putw (s, TE_LINK_SUBTLV_REMOTE_ADDRESS);

  /* Set length. */
  putp1 = stream_get_putp (s);
  stream_putw (s, 0);

  /* Set value. */
  count = 0;
  if (stream_put_in_addr (s, &ot->tlink->remote_addr.u.prefix4))
    count++;

  if (count)
    {
      /* Set length. */
      stream_putw_at (s, putp1, count * OSPF_TE_TLV_IP_ADDR_MIN_LEN);

      /* Make four-octet align. */
      TLV_PAD (s, tlv);
    }
  else
    /* Reset this TLV. */
    stream_set_putp (s, putp0);
}

/** @brief Function to encode te-link metric sub TLV

    @param *s            - Stream buffer
    @param *ot           - OSFP telink structure

    @return    void
*/
void
ospf_te_stlv_put_te_link_te_metric (struct stream *s,
                          struct ospf_telink *ot)
{
  struct te_tlv *tlv;
  u_int32_t metric;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_METRIC_FIX_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_TE_METRIC);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_METRIC_FIX_LEN);

  /* Set value. */
  if (CHECK_FLAG (ot->top->flags, OSPF_ROUTER_RESTART))
    {
      metric = 0xffffffff;
      stream_putl (s, metric);
    }
  else if ((ot->params) && 
           CHECK_FLAG (ot->params->config, OSPF_TLINK_PARAM_TE_METRIC))
    stream_putl (s, ot->params->te_metric);
  else 
    stream_putl (s, OSPF_OUTPUT_COST_DEFAULT);

  /* Make four-octet align. */
  TLV_PAD (s, tlv);
}

/** @brief Function to encode max te-link BW sub TLV

    @param *s            - Stream buffer
    @param *ot           - OSFP telink structure

    @return    void
*/
void
ospf_te_stlv_put_maximum_te_link_bw (struct stream *s, 
                                struct ospf_telink *ot)
{
  struct te_tlv *tlv;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_BW_FIX_LEN)
    return;
      
  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_MAX_BANDWIDTH);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_BW_FIX_LEN);

  /* Set value. */
  stream_putf (s, ot->tlink->prop.lpro.max_lsp_bw[0]);

  /* Make four-octet align. */
  TLV_PAD (s, tlv);
}

/** @brief Function to encode te-link reservable BW sub TLV

    @param *s            - Stream buffer
    @param *ot           - OSFP telink structure

    @return    void
*/
void
ospf_te_stlv_put_te_link_reservable_bw (struct stream *s,
                                   struct ospf_telink *ot)
{
  struct te_tlv *tlv;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_BW_FIX_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_MAX_RES_BANDWIDTH);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_BW_FIX_LEN);

  /* Set value. */
  stream_putf (s, ot->tlink->prop.lpro.max_resv_bw);

  /* Make four-octet align. */
  TLV_PAD (s, tlv);
}

/** @brief Function to encode te-link unreservable BW sub TLV

    @param *s            - Stream buffer
    @param *ot           - OSFP telink structure

    @return    void
*/
void
ospf_te_stlv_put_te_link_unreserved_bw (struct stream *s,
                                    struct ospf_telink *ot)
{
  struct te_tlv *tlv;
  int i;

#ifdef HAVE_DSTE
  u_int32_t putp0;
  int count = 0;
#endif /* HAVE_DSTE */

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_UNRSV_BW_FIX_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
#ifdef HAVE_DSTE
  putp0 = stream_get_putp (s);
#endif /* HAVE_DSTE */
  stream_putw (s, TE_LINK_SUBTLV_UNRES_BANDWIDTH);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_UNRSV_BW_FIX_LEN);

  /* Set value. */
  if (CHECK_FLAG (ot->top->flags, OSPF_ROUTER_RESTART))
    stream_putf (s, 0);
  else
    for (i = 0; i < MAX_PRIORITIES; i++)
      {
#ifdef HAVE_DSTE
        if (ot->tlink->prop.lpro.tecl_priority_bw[i] > 0)
          count++;
#endif /* HAVE_DSTE */

        stream_putf (s, ot->tlink->prop.lpro.tecl_priority_bw[i]);
      }

#ifdef HAVE_DSTE
  if (count > 0)
    /* Make four-octet align. */
    TLV_PAD (s, tlv);
  else
    /* Reset this TLV */
    stream_set_putp (s, putp0);
#else
  /* Make four-octet align. */
  TLV_PAD (s, tlv);
#endif /* HAVE_DSTE */
}

#if defined HAVE_DSTE && defined HAVE_PACKET
/** @brief Function to encode te-link Bandwidth constraint sub TLV

    @param *s            - Stream buffer
    @param *ot           - OSFP telink structure

    @return    void
*/
void
ospf_te_stlv_put_te_link_bc (struct stream *s, struct ospf_telink *ot)
{
  struct te_tlv *tlv;
  u_int32_t putp0, putp1;
  int ctr, count;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_BC_MAX_LEN)
    return;
 
  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set Type */
  putp0 = stream_get_putp (s);
  stream_putw (s, TE_LINK_SUBTLV_BC);

  /* Set Length */
  putp1 = stream_get_putp (s);
  stream_putw (s, 0);

  /* Set the bc_model */
  stream_putc (s, ot->tlink->prop.bc_mode);
  for (ctr = MAX_BW_CONST; ctr > 0; ctr--)
    if (ot->tlink->prop.lpro.bw_constraint[ctr - 1] > 0)
      break;

  /* Reset TLV if all bandwidth constraints are zero */
  if (ctr == 0)
    {
      stream_set_putp (s, putp0);
      return;
    }

  count = ctr;
  for (ctr = 0; ctr < count; ctr++)
    stream_putf (s, ot->tlink->prop.lpro.bw_constraint[ctr]);

  /* Set Length */
  stream_putw_at (s, putp1, count * 4 + 1);

  /* Make 4-octet alignment */
  TLV_PAD (s, tlv);
}
#endif /* HAVE_DSTE && HAVE_PACKET */

/** @brief Function to encode te-link admin group sub TLV

    @param *s            - Stream buffer
    @param *ot           - OSFP telink structure

    @return    void
*/
void
ospf_te_stlv_put_te_link_admin_group (struct stream *s,
                                struct ospf_telink *ot)
{
  struct te_tlv *tlv;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_RSRC_COLOR_FIX_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_RESOURCE_CLASS);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_RSRC_COLOR_FIX_LEN);

  /* Set value. */
  stream_putl (s, ot->tlink->prop.admin_group);

  /* Make four-octet align. */
  TLV_PAD (s, tlv);
}

/** @brief Function to encode te-link local remote ID sub TLV

    @param *s            - Stream buffer
    @param *ot           - OSFP telink structure

    @return    void
*/
void
ospf_te_stlv_put_te_link_local_remote_id (struct stream *s,
                                     struct ospf_telink *ot)
{
  struct te_tlv *tlv;
  u_int32_t local_if_id;
  u_int32_t remote_if_id;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_LOCAL_REMOTE_ID_FIX_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_LOCAL_REMOTE_ID);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_LOCAL_REMOTE_ID_FIX_LEN);

  /* Set local id  value*/
  if (ot->tlink->l_linkid.type == GMPLS_LINK_ID_TYPE_UNNUMBERED)
    local_if_id = pal_hton32(ot->tlink->l_linkid.linkid.unnumbered); 
  else 
    local_if_id = ot->tlink->l_linkid.linkid.ipv4.s_addr;

  stream_putl (s, local_if_id);

  /* Set remote id value */
  if (ot->tlink->r_linkid.type == GMPLS_LINK_ID_TYPE_UNNUMBERED)
    remote_if_id = pal_hton32(ot->tlink->r_linkid.linkid.unnumbered);
  else
    remote_if_id = ot->tlink->r_linkid.linkid.ipv4.s_addr;

  stream_putl (s, remote_if_id);
  /* Make four-octet align. */
  TLV_PAD (s, tlv);
}

/** @brief Function to encode te-link protection type sub TLV

    @param *s            - Stream buffer
    @param *ot           - OSFP telink structure

    @return    void
*/
void
ospf_te_stlv_put_te_link_protection_type (struct stream *s,
                                     struct ospf_telink *ot)
{
  struct te_tlv *tlv;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_PROTECTION_TYPE_FIX_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_PROTECTION_TYPE);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_PROTECTION_TYPE_FIX_LEN);

  /* Set value. */
  stream_putc (s, ot->tlink->prop.phy_prop.protection);

  stream_put (s, NULL, 3);
  /* Make four-octet align. */
  TLV_PAD (s, tlv);
}
void
ospf_te_stlv_put_te_link_capability_default (struct stream *s,
                                     struct ospf_telink *ot,
                                     u_int8_t capability)
{
  struct te_tlv *tlv;
  int i;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_IF_SW_CAPABILITY_MIN_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_IF_SW_CAPABILITY);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_IF_SW_CAPABILITY_MIN_LEN);

  /* Set value. */
  stream_putc (s, capability);
  stream_putc (s, ot->tlink->prop.phy_prop.encoding_type);
  stream_putw (s, 0);

  for (i = 0; i < MAX_PRIORITIES; i++)
    stream_putf (s, ot->tlink->prop.lpro.max_lsp_bw[i]);

  /* Make four octet align */
  TLV_PAD (s, tlv);
}

void
ospf_te_stlv_put_te_link_capability_psc (struct stream *s,
                                     struct ospf_telink *ot,
                                     u_int8_t capability)
{
  struct te_tlv *tlv;
  int i;

  if (STREAM_REMAIN (s) < 4 + OSPF_TE_TLV_IF_SW_CAPABILITY_MIN_LEN)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_IF_SW_CAPABILITY);

  /* Set length. */
  stream_putw (s, OSPF_TE_TLV_IF_SW_CAPABILITY_PSC_LEN);

  /* Set value. */
  stream_putc (s, capability);
  stream_putc (s, ot->tlink->prop.phy_prop.encoding_type);
  stream_putw (s, 0);

  for (i = 0; i < MAX_PRIORITIES; i++)
    stream_putf (s, ot->tlink->prop.lpro.max_lsp_bw[i]);

  stream_putf (s, ot->tlink->prop.phy_prop.min_lsp_bw);
  stream_putw (s, ot->tlink->prop.mtu);
  stream_putw (s, 0);
}

/** @brief Function to encode te-link Switching capability sub TLV

    @param *s            - Stream buffer
    @param *ot           - OSFP telink structure

    @return    void
*/

void
ospf_te_stlv_put_te_link_capability_type (struct stream *s,
                                     struct ospf_telink *ot)
{
  u_int32_t capability;

  capability  = ot->tlink->prop.phy_prop.switching_cap;

  if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_PSC1))
    ospf_te_stlv_put_te_link_capability_psc (s, ot,
                                  GMPLS_SWITCH_CAPABILITY_PSC1);
  if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_PSC2))
    ospf_te_stlv_put_te_link_capability_psc (s, ot,
                                  GMPLS_SWITCH_CAPABILITY_PSC2);
  
  if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_PSC3))
    ospf_te_stlv_put_te_link_capability_psc (s, ot,
                                  GMPLS_SWITCH_CAPABILITY_PSC3);
  if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_PSC4))
    ospf_te_stlv_put_te_link_capability_psc (s, ot,
                                  GMPLS_SWITCH_CAPABILITY_PSC4);
 
  if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_L2SC))
    ospf_te_stlv_put_te_link_capability_default (s, ot,
                                  GMPLS_SWITCH_CAPABILITY_L2SC);
  if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_TDM))
    ospf_te_stlv_put_te_link_capability_default (s, ot,
                                  GMPLS_SWITCH_CAPABILITY_TDM);
  if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_LSC))
    ospf_te_stlv_put_te_link_capability_default (s, ot,
                                  GMPLS_SWITCH_CAPABILITY_LSC);
  if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_FSC))
    ospf_te_stlv_put_te_link_capability_default (s, ot,
                                  GMPLS_SWITCH_CAPABILITY_FSC);
  if (CHECK_FLAG (capability, GMPLS_SWITCH_CAPABILITY_FLAG_PBB_TE))
    ospf_te_stlv_put_te_link_capability_default (s, ot,
                                  GMPLS_SWITCH_CAPABILITY_802_1);
}

/** @brief Function to encode te-link Shared Risk List Group sub TLV

    @param *s            - Stream buffer
    @param *ot           - OSFP telink structure

    @return    void
*/
void
ospf_te_stlv_put_te_risk_group (struct stream *s, struct ospf_telink *ot)
{
  struct te_tlv *tlv;
  u_int32_t *group;
  int i;

  if (STREAM_REMAIN (s) < 4 + vector_count (ot->tlink->prop.srlg) * 4)
    return;

  tlv = (struct te_tlv *) STREAM_PUT_PNT (s);

  /* Set type. */
  stream_putw (s, TE_LINK_SUBTLV_SHARED_RISK_GROUP);

  /* Set length. */
  stream_putw (s, vector_count (ot->tlink->prop.srlg) * 4);

  /* Set value. */
  for (i = 0; i < vector_count (ot->tlink->prop.srlg); i++)
    if ((group = vector_slot (ot->tlink->prop.srlg, i)))
      stream_putl (s, *group);

  /* Make four-octet align. */
  TLV_PAD (s, tlv);
}

/** @brief Function to encode te-link TLV

    @param *s            - Stream buffer
    @param *ospf_tel     - ospf TE-link structure

    @return   
       SUCCESS : 1
       FAILURE  : 0
*/
int
ospf_te_tlv_put_te_link (struct stream *s, struct ospf_telink *ospf_tel)
{
  struct te_tlv *tlv;
  u_int32_t putp;

  tlv = (struct te_tlv *) STREAM_DATA (s);
  
  /* Set type. */
  stream_putw (s, TE_TLV_LINK_TYPE);

  /* Set length. */
  putp = stream_get_putp (s);
  stream_putw (s, 0);

  /* Set values. */

  /* Set link type sub TLV. */
  if (! ospf_te_stlv_put_te_link_type (s))
  /* The Link Type and Link ID sub-TLVs are mandatory. */
    return 0;

  /* Set link ID sub TLV. */
  if (! ospf_te_stlv_put_te_link_id (s, ospf_tel))
  /* The Link Type and Link ID sub-TLVs are mandatory. */
    return 0;
  
  if (CHECK_FLAG (ospf_tel->tlink->flags, GMPLS_TE_LINK_NUMBERED)) 
    {
      /* Set local interface IP addresses sub TLV. */
      ospf_te_stlv_put_te_link_local_ip_addr (s, ospf_tel);

      /* Set remote interface IP address sub TLV for point-to-point link. */
      ospf_te_stlv_put_te_link_remote_ip_addr (s, ospf_tel);
    }

  /* Set traffic engineering metric sub TLV. */
  ospf_te_stlv_put_te_link_te_metric (s, ospf_tel);

  /* Set maximum bandwidth sub TLV. */
  if (ospf_tel->tlink->prop.lpro.bandwidth > 0)
    ospf_te_stlv_put_maximum_te_link_bw (s, ospf_tel);

  /* Set maximum reservable bandwidth sub TLV. */
  if (ospf_tel->tlink->prop.lpro.max_resv_bw > 0)
    ospf_te_stlv_put_te_link_reservable_bw (s, ospf_tel);

#ifdef HAVE_DSTE 
#ifdef HAVE_PACKET
  /* Set bandwidth constraint sub TLV */
  ospf_te_stlv_put_te_link_bc (s, ospf_tel);
#endif /* HAVE_PACKET */
#endif /* HAVE_DSTE */

  /* Set unreserved bandwidth sub TLV. */
#ifdef HAVE_DSTE
  ospf_te_stlv_put_te_link_unreserved_bw (s, ospf_tel);
#else
  if (ospf_tel->tlink->prop.lpro.bandwidth > 0 
      && ospf_tel->tlink->prop.lpro.max_resv_bw > 0)
    ospf_te_stlv_put_te_link_unreserved_bw (s, ospf_tel);
#endif /* HAVE_DSTE */

  /* Set resource class sub TLV. */
  if (ospf_tel->tlink->prop.admin_group)
    ospf_te_stlv_put_te_link_admin_group (s, ospf_tel);

  /* Set link local/remote identifiers sub TLV. */
  if (!CHECK_FLAG (ospf_tel->tlink->flags, GMPLS_TE_LINK_NUMBERED)) 
    ospf_te_stlv_put_te_link_local_remote_id (s, ospf_tel);

  /* Set link protection type sub TLV. */
  if (ospf_tel->tlink->prop.phy_prop.protection)
    ospf_te_stlv_put_te_link_protection_type (s, ospf_tel);

  /* Set interface switching capability descriptor sub TLV. */
  if (ospf_tel->tlink->prop.phy_prop.switching_cap)
    ospf_te_stlv_put_te_link_capability_type (s, ospf_tel);

  /* Set shared risk link group sub TLV. */
  if (vector_count (ospf_tel->tlink->prop.srlg) > 0)
    ospf_te_stlv_put_te_risk_group (s, ospf_tel);

  /* Set length. */
  stream_putw_at (s, putp, stream_get_putp (s) - OSPF_TE_TLV_HEADER_FIX_LEN);

  /* Make four-octet align. */
  TLV_PAD (s, tlv);

  return 1;
}
#endif/* HAVE_GMPLS */

int
ospf_te_tlv_put_link (struct stream *s, struct ospf_interface *oi)
{
  struct te_tlv *tlv;
  u_int32_t putp;

  tlv = (struct te_tlv *) STREAM_DATA (s);

  /* Set type. */
  stream_putw (s, TE_TLV_LINK_TYPE);

  /* Set length. */
  putp = stream_get_putp (s);
  stream_putw (s, 0);

  /* Set values. */

  /* Set link type sub TLV. */
  if (! ospf_te_stlv_put_link_type (s, oi))
    /* The Link Type and Link ID sub-TLVs are mandatory. */
    return 0;

  /* Set link ID sub TLV. */
  if (! ospf_te_stlv_put_link_id (s, oi))
    /* The Link Type and Link ID sub-TLVs are mandatory. */
    return 0;

  /* Set local interface IP addresses sub TLV. */
  ospf_te_stlv_put_local_ip_addr (s, oi);

  /* Set remote interface IP address sub TLV for point-to-point link. */
  if (oi->type == OSPF_IFTYPE_POINTOPOINT
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT
      || oi->type == OSPF_IFTYPE_POINTOMULTIPOINT_NBMA)
    ospf_te_stlv_put_remote_ip_addr (s, oi);

  /* Set traffic engineering metric sub TLV. */
  ospf_te_stlv_put_te_metric (s, oi);

  /* Set maximum bandwidth sub TLV. */
  if (oi->u.ifp->bandwidth > 0)
    ospf_te_stlv_put_maximum_link_bw (s, oi);

  /* Set maximum reservable bandwidth sub TLV. */
  if (oi->u.ifp->max_resv_bw > 0)
    ospf_te_stlv_put_reservable_link_bw (s, oi);

#ifdef HAVE_DSTE
  /* Set bandwidth constraint sub TLV */
  ospf_te_stlv_put_bc (s, oi);
#endif /* HAVE_DSTE */

  /* Set unreserved bandwidth sub TLV. */
#ifdef HAVE_DSTE
  ospf_te_stlv_put_unreserved_bw (s, oi);
#else
  if (oi->u.ifp->bandwidth > 0 && oi->u.ifp->max_resv_bw > 0)
    ospf_te_stlv_put_unreserved_bw (s, oi);
#endif /* HAVE_DSTE */

  /* Set resource class sub TLV. */
  if (oi->u.ifp->admin_group)
    ospf_te_stlv_put_admin_group (s, oi);

  /* Set length. */
  stream_putw_at (s, putp, stream_get_putp (s) - OSPF_TE_TLV_HEADER_FIX_LEN);

  /* Make four-octet align. */
  TLV_PAD (s, tlv);

  return 1;
}

void
ospf_te_announce_router_to_area (struct ospf_area *area)
{
  struct pal_in4_addr id;
  struct stream *s;
  
  if (!IS_PROC_ACTIVE (area->top))
    return;
  
  pal_mem_set (&id, 0, sizeof (struct pal_in4_addr));
  
  /* Instance ID of router address tlv is 1.  */
  id.s_addr = pal_hton32 ((1<<24) | 0x00000001);
  
  s = stream_new (OSPF_MIN_LSA_BUFSIZ);

  if (ospf_te_tlv_put_router_addr (s, area->top))
    ospf_opaque_lsa_redistribute_area (area, id, STREAM_DATA (s),
                                       stream_get_putp (s));

  stream_free (s);
}

void
ospf_te_withdraw_router_from_area (struct ospf_area *area)
{
  struct pal_in4_addr id;

  pal_mem_set (&id, 0, sizeof (struct pal_in4_addr));

  /* Instance ID of router address tlv is 1.  */
  id.s_addr = pal_hton32 ((1<<24) | 0x00000001);

  ospf_opaque_lsa_redistribute_area (area, id, NULL, 0);
}

void
ospf_te_announce_router (struct ospf *top)
{
  struct ospf_area *area;
  struct ls_node *rn;
  
  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
        ospf_te_announce_router_to_area (area);
}

void
ospf_te_withdraw_router (struct ospf *top)
{
  struct ospf_area *area;
  struct ls_node *rn;

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
        ospf_te_withdraw_router_from_area (area);
}

void
ospf_te_announce_link_to_area (struct ospf *top, struct ospf_area *area,
                               struct interface *ifp)
{
  struct ospf_interface *oi;
  struct pal_in4_addr id;
  struct stream *s;

  if (!IS_PROC_ACTIVE (top))
    return;

  oi = ospf_te_find_interface_on_link (area, ifp);

  if (!oi)
    return;
  
  if (oi->full_nbr_count <= 0)
    return;

  if (oi->type != OSPF_IFTYPE_POINTOPOINT
      && oi->type != OSPF_IFTYPE_POINTOMULTIPOINT
      && oi->type != OSPF_IFTYPE_POINTOMULTIPOINT_NBMA
      && oi->ident.d_router.s_addr == 0)
    return;
  
  if (oi->state < IFSM_PointToPoint)
    return;

#ifdef HAVE_GMPLS
  /* SET oi type to MPLS, if it set to GMPLS type */
  UNSET_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_CONTROL);
  UNSET_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_DATA);

#endif /* HAVE_GMPLS */

  pal_mem_set (&id, 0, sizeof (struct pal_in4_addr));

#ifdef HAVE_GMPLS
  { /* Use Instance ID for link TLVs as interface gifindex incremented by 1 */
    id.s_addr = pal_hton32 (((ifp->gifindex + 1) & 0x0000ffff) | (1<<24));
  }
#else /* HAVE_GMPLS */
  {
    /* Use Instance ID for link TLVs as interface index incremented by 1 */
    id.s_addr = pal_hton32 (((ifp->ifindex + 1) & 0x0000ffff) | (1<<24));
  }
#endif /* HAVE_GMPLS */

  s = stream_new (OSPF_MIN_LSA_BUFSIZ);

  /* Set Link TLV. */
  if (ospf_te_tlv_put_link (s, oi))
    ospf_opaque_lsa_redistribute_area (area, id, STREAM_DATA (s),
                                       stream_get_putp (s));

  stream_free (s);
}

void
ospf_te_announce_link (struct ospf *top, struct interface *ifp)
{
  struct ospf_area *area;
  struct ls_node *rn;

#ifdef HAVE_GMPLS 
  /* If interface type is (control, data or control-data),
     then TE inforamtion is not announced to area */
  if (ifp->gmpls_type != PHY_PORT_TYPE_GMPLS_UNKNOWN)
    return;
#endif /* HAVE_GMPLS*/

  if (if_is_up (ifp))
    for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
      if ((area = RN_INFO (rn, RNI_DEFAULT)))
        if (IS_AREA_ACTIVE (area))
          ospf_te_announce_link_to_area (top, area, ifp);
}

void
ospf_te_withdraw_link (struct ospf *top, struct interface *ifp)
{
  struct pal_in4_addr id;
  struct ospf_area *area;
  struct ls_node *rn;

  pal_mem_set (&id, 0, sizeof (struct pal_in4_addr));

#ifdef HAVE_GMPLS
  { /* Use Instance ID for link TLVs as interface gifindex incremented by 1 */
    id.s_addr = pal_hton32 (((ifp->gifindex + 1) & 0x0000ffff) | (1<<24));
  }
#else /* HAVE_GMPLS */
  {
    /* Use Instance ID for link TLVs as interface index incremented by 1 */
    id.s_addr = pal_hton32 (((ifp->ifindex + 1) & 0x0000ffff) | (1<<24));
  }
#endif /* HAVE_GMPLS */

  for (rn = ls_table_top (top->area_table); rn; rn = ls_route_next (rn))
    if ((area = RN_INFO (rn, RNI_DEFAULT)))
      if (IS_AREA_ACTIVE (area))
        ospf_opaque_lsa_redistribute_area (area, id, NULL, 0);
}

#ifdef HAVE_GMPLS
/** @brief Function to announce TE-link to ospf area

    @param os_tel        - OSPF te-link structure

    @return void
*/
void
ospf_te_announce_te_link (struct ospf_telink *os_tel)
{
  struct pal_in4_addr id;
  struct stream *s;

  /* Set Instance ID to te link gifindex */
  id.s_addr = pal_hton32 ((((os_tel->tlink->gifindex + 1) & 0x0000ffff)
                             | (1<<24)));

  s = stream_new (OSPF_MIN_LSA_BUFSIZ);

  /* Set Link TLV. */
  if (ospf_te_tlv_put_te_link (s, os_tel))
    ospf_opaque_lsa_redistribute_area (os_tel->area, id, STREAM_DATA (s),
                                       stream_get_putp (s));
  
  stream_free (s);
}

/** @brief Function to withdraw TE-link from ospf area

    @param os_tel        - OSPF te-link structure

    @return void
*/
void
ospf_te_withdraw_te_link (struct ospf_telink *os_tel)
{
  struct pal_in4_addr id;

  /* Set Instance ID to te link gifindex */
  id.s_addr = pal_hton32 ((((os_tel->tlink->gifindex + 1) & 0x0000ffff)
                            |(1<<24)));

  ospf_opaque_lsa_redistribute_area (os_tel->area, id, NULL, 0);
}

void *
ospf_telink_add_dummy (void *arg1)
{
  /* This function is to act as a dummy for the hash to return
     the ospf_telink itself */
  return arg1;
}

/** @brief Function to create new ospf_telink structur 
        and add it to the te-link table

    @param vr_id         - VR identifier
    @param os_tel        - OSPF te-link structure

    @return   ospf_telink structure
*/
struct ospf_telink *
ospf_te_link_new (u_int32_t vr_id, struct telink *tel)
{
  struct ospf_telink *os_tel = NULL;
  struct ospf_master *om = NULL;

  om = tel->gmif->vr->proto;
  if (om == NULL)
    return NULL;

  os_tel = XCALLOC (MTYPE_OSPF_TELINK, sizeof (struct ospf_telink));
  if (!os_tel)
    return NULL;

  /* Set te-link information */
  os_tel->tlink = tel;

  return os_tel;
}

/** @brief Function to set te-link local LSA 

    @param os_tel        - OSPF te-link structure

    @return   void 
*/
void
ospf_te_link_local_lsa_set (struct ospf_telink *os_tel)
{
  struct stream *s;
  struct ospf_interface *oi = NULL;
  struct ls_node *rn = NULL, *rn1 = NULL;
  struct ospf_neighbor *nbr = NULL;
  struct pal_in4_addr id;
  bool_t oi_found = PAL_FALSE;

  if (!os_tel->params || 
      !CHECK_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_TE_LINK_LOCAL) ||
      !CHECK_FLAG (os_tel->tlink->status, GMPLS_INTERFACE_UP))
    return;

  /* Obtain ospf_interface from corresponding te_link*/
  if (!os_tel->top)
    return;

  for (rn = ls_table_top (os_tel->top->if_table); rn; rn = ls_route_next (rn))
    {
      if ((oi = RN_INFO (rn, RNI_OSPF_IF)))
        for (rn1 = ls_table_top (oi->nbrs); rn1; rn1 = ls_route_next (rn1))
          {
            if ((nbr = RN_INFO (rn1, RNI_DEFAULT)))
              if (!pal_mem_cmp (&nbr->ident.router_id, 
                               &os_tel->tlink->prop.linkid,
                               sizeof (struct pal_in4_addr)))
                {
                  /* ospf interface corresponding to te-link found */
                  oi_found = PAL_TRUE;
                  break;
                }
          }

      if (oi_found)
        break;
    }
  /* If no link exists return */
  if (!oi_found)
    return;

  s = stream_new (OSPF_MIN_LSA_BUFSIZ);

  /* Set TLV Type*/
  stream_putw (s, TE_TLV_LINK_TYPE);

  /* Set Length*/
  stream_putw (s, OSPF_TE_TLV_LINK_LOCAL_IDNTIFIER);

  /* Set Value */
  stream_putl (s, os_tel->tlink->l_linkid.linkid.ipv4.s_addr);

  id.s_addr = MAKE_OPAQUE_ID (1, 0);

  /* Add opaque LSA */
  ospf_opaque_lsa_redistribute_link (oi, id, STREAM_DATA (s),
                                     stream_get_endp (s));

  stream_free (s);
}

/** @brief Function to unset te-link local LSA 

    @param os_tel        - OSPF te-link structure

    @return   void 
*/
void
ospf_te_link_local_lsa_unset (struct ospf_telink *os_tel)
{
  struct ospf_interface *oi = NULL;
  struct ls_node *rn = NULL, *rn1 = NULL; 
  struct ospf_neighbor *nbr = NULL;
  struct pal_in4_addr id;
  bool_t oi_found = PAL_FALSE;

  if (!os_tel->params || 
      !CHECK_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_TE_LINK_LOCAL) ||
      !CHECK_FLAG (os_tel->tlink->status, GMPLS_INTERFACE_UP))
    return;

  if (!os_tel->top)
    return;

  /* Obtain ospf_interface from corresponding te_link*/
  for (rn = ls_table_top (os_tel->top->if_table); rn; rn = ls_route_next (rn))
    {
      if ((oi = RN_INFO (rn, RNI_OSPF_IF)))
        for (rn1 = ls_table_top (oi->nbrs); rn1; rn1 = ls_route_next (rn1))
          {
            if ((nbr = RN_INFO (rn1, RNI_DEFAULT)))
              if (!pal_mem_cmp (&nbr->ident.router_id, 
                               &os_tel->tlink->prop.linkid,
                               sizeof (struct pal_in4_addr)))
                {
                  /* ospf interface corresponding to te-link found */
                  oi_found = PAL_TRUE;
                  break;
                }
          }

      if (oi_found)
        break;
    }

  /* If no link exists return */
  if (!oi_found)
    return;

  id.s_addr = MAKE_OPAQUE_ID (1, 0);

  /* Delete opaque LSA */
  ospf_opaque_lsa_redistribute_link (oi, id, NULL, 0);
  return;
}

/** @brief Function to activate ospf_telink 

    @param os_tel        - OSPF te-link structure

    @return  void
*/
void 
ospf_activate_te_link (struct ospf_telink *os_tel)
{
  /* If te-link status is up and te-link flooding is enable 
     then announce the te-link  */
  if (CHECK_FLAG (os_tel->tlink->status, GMPLS_INTERFACE_UP)
      && (os_tel->top))
    ospf_te_announce_te_link (os_tel);

}

/** @brief Function to deactivate ospf_telink

    @param os_tel        - OSPF te-link structure

    @return  void
*/
void
ospf_deactivate_te_link (struct ospf_telink *os_tel)
{
  if (CHECK_FLAG (os_tel->tlink->status, GMPLS_INTERFACE_UP))
    {
      /* Unset te-link local LSA */
      if ((os_tel->params) && 
           CHECK_FLAG (os_tel->params->config, OSPF_TLINK_PARAM_TE_LINK_LOCAL))
        ospf_te_link_local_lsa_unset (os_tel);
      if (os_tel->top)
        ospf_te_withdraw_te_link (os_tel);
    }
}

/** @brief Function to update ospf_telink

    @param os_tel        - OSPF te-link structure

    @return  void
*/
void
ospf_update_te_link (struct ospf_telink *os_tel)
{
  if (CHECK_FLAG (os_tel->tlink->status, GMPLS_INTERFACE_UP)
      && (os_tel->top))
    ospf_te_announce_te_link (os_tel);
}

#endif /* HAVE_GMPLS */

void
ospf_te_announce_lsa (struct ospf *top)
{
  struct ospf_master *om = top->om;
  struct interface *ifp;
  struct listnode *node;
#ifdef HAVE_GMPLS
  struct telink *tl = NULL;
  struct avl_node *tl_node = NULL; 
  struct ospf_telink *os_tel = NULL;
#endif /* HAVE_GMPLS */

  
  if (!IS_PROC_ACTIVE (top))
    return;

  ospf_te_announce_router (top);

  LIST_LOOP (om->vr->ifm.if_list, ifp, node)
    if (if_is_up (ifp))
      if (!if_is_loopback (ifp))
        ospf_te_announce_link (top, ifp);

#ifdef HAVE_GMPLS
  /* Loop through the telink table and announce each telink to area*/
  AVL_TREE_LOOP (&om->zg->gmif->teltree, tl, tl_node)
    {
      if (tl->info == NULL)
       continue;

      os_tel = (struct ospf_telink *)tl->info;
      if (CHECK_FLAG (os_tel->tlink->status, GMPLS_INTERFACE_UP)
        && (os_tel->top))
        ospf_te_announce_te_link (os_tel);    
    }
#endif /* HAVE_GMPLS */
}

void
ospf_te_withdraw_lsa (struct ospf *top)
{
  struct ospf_master *om = top->om;
  struct interface *ifp;
  struct listnode *node;
#ifdef HAVE_GMPLS
  struct telink *tl = NULL;
  struct avl_node *tl_node = NULL; 
  struct ospf_telink *os_tel = NULL;
#endif /* HAVE_GMPLS */

  ospf_te_withdraw_router (top);

  LIST_LOOP (om->vr->ifm.if_list, ifp, node)
    if (if_is_up (ifp))
      if (!if_is_loopback (ifp))
        ospf_te_withdraw_link (top, ifp);

#ifdef HAVE_GMPLS
  /* Loop through the telink table and withdraw telink from area*/
  AVL_TREE_LOOP (&om->zg->gmif->teltree, tl, tl_node)
    {
      if (tl->info == NULL)
       continue;

      os_tel = (struct ospf_telink *)tl->info;
      if (CHECK_FLAG (os_tel->tlink->status, GMPLS_INTERFACE_UP)
        && (os_tel->top))
        ospf_te_withdraw_te_link (os_tel);
    }
#endif /* HAVE_GMPLS */

}


int
ospf_te_if_attr_update (struct interface *ifp, struct ospf *top)
{
  if (!IS_TE_CAPABLE (top))
    return 0;
  
  if (ifp == NULL
      || !if_is_up (ifp)
      || if_is_loopback (ifp))
    return 0;

  ospf_te_announce_link (top, ifp);

  return 0;
}

#ifdef HAVE_GMPLS
void 
ospf_te_if_gmpls_type_update (struct ospf_interface *oi)
{
  struct interface *ifp = oi->u.ifp;

  if (!CHECK_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_CONTROL)
      && !(CHECK_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_DATA)))
    {
      /* Withdraw the link LSAs generated for MPLS*/
      ospf_te_withdraw_link (oi->top, ifp);

      if (ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_DATA)
        {
          SET_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_DATA);
          if (CHECK_FLAG (oi->flags, OSPF_IF_UP))
            ospf_if_down (oi);
        }

      else if ((ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_CONTROL)
         || (ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_DATA_CONTROL))
        {
          SET_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_CONTROL);
          if (!CHECK_FLAG (oi->flags, OSPF_IF_UP))
            ospf_if_up (oi);
        }
    }

  else if (CHECK_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_CONTROL)
      && (ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_DATA))
    {
      /* Session should go down when the */
      SET_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_DATA);
      UNSET_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_CONTROL);
      ospf_if_down (oi);
    }
  
  else if (CHECK_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_DATA)
      && ((ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_CONTROL)
          ||(ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_DATA_CONTROL)))
    {
      SET_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_CONTROL);
      UNSET_FLAG (oi->flags, OSPF_IF_GMPLS_TYPE_DATA);
      ospf_if_up (oi);
    }
}
#endif /* HAVE_GMPLS*/
int
ospf_te_ifsm_state_change (u_char event, void *arg,
                          void *param1, void *param2)
{
  struct ospf_interface *oi = param1;
  int old_state = (int)param2;

  if (!IS_TE_CAPABLE (oi->top))
    return 0;

  if (oi->type == OSPF_IFTYPE_VIRTUALLINK
      || oi->type == OSPF_IFTYPE_LOOPBACK)
    return 0;

  if (oi->full_nbr_count > 0)
    {
      if (old_state < IFSM_PointToPoint)
        {
          if (oi->state >= IFSM_PointToPoint)
            ospf_te_announce_link (oi->top, oi->u.ifp);
        }
      else
        ospf_te_announce_link (oi->top, oi->u.ifp);
    }

  if (old_state >= IFSM_PointToPoint && oi->state < IFSM_PointToPoint)
    ospf_te_withdraw_link (oi->top, oi->u.ifp);

  return 0;
}

int
ospf_te_nfsm_state_change (u_char event, void *arg,
                          void *param1, void *param2)
{
  struct ospf_neighbor *nbr = param1;
  int old_state = (int)param2;

  if (!IS_TE_CAPABLE (nbr->oi->top))
    return 0;

  if (nbr->oi->type == OSPF_IFTYPE_VIRTUALLINK
      || nbr->oi->type == OSPF_IFTYPE_LOOPBACK)
    return 0;

  if (nbr->oi->area->full_nbr_count == 1)
    ospf_te_announce_router_to_area (nbr->oi->area);
  else if (nbr->oi->area->full_nbr_count == 0)
    ospf_te_withdraw_router_from_area (nbr->oi->area);

  if (old_state < NFSM_Full && nbr->state >= NFSM_Full) 
    ospf_te_announce_link (nbr->oi->top, nbr->oi->u.ifp);

  if (old_state >= NFSM_Full
      && nbr->state < NFSM_Full
      && nbr->oi->full_nbr_count <= 0)
    ospf_te_withdraw_link (nbr->oi->top, nbr->oi->u.ifp);

  return 0;
}

int
ospf_te_area_new (u_char event, void *arg, void *param1, void *param2)
{
  struct ospf_area *area = param1;

  if (!IS_TE_CAPABLE (area->top))
    return 0;
  
  if (!IS_PROC_ACTIVE (area->top))
    return 0;

  ospf_te_announce_router_to_area (area);

  return 0;
}

int
ospf_te_router_id_changed (u_char event, void *arg,
                           void *param1, void *param2)
{
  struct ospf *top = param1;

  if (!IS_TE_CAPABLE (top))
    return 0;

  ospf_te_withdraw_lsa (top);
  ospf_te_announce_lsa (top);
  
  return 0;
}

int
ospf_te_network_del (u_char event, void *arg, void *param1, void *param2)
{
  struct ospf_interface *oi = param1;

  if (!IS_TE_CAPABLE (oi->top))
    return 0;

  if (oi->type == OSPF_IFTYPE_VIRTUALLINK
      || oi->type == OSPF_IFTYPE_LOOPBACK)
    return 0;

  ospf_te_withdraw_link (oi->top, oi->u.ifp);

  return 0;
}

int
ospf_te_process_del (u_char event, void *arg, void *param1, void *param2)
{
  struct ospf *top = param1;

  if (IS_TE_CAPABLE (top))
    ospf_te_withdraw_lsa (param1);

  return 0;
}

int
ospf_te_write_config (u_char event, void *arg, void *param1, void *param2)
{
  struct cli *cli = param1;
  struct ospf *top = param2;

  if (!IS_TE_CAPABLE (top))
    cli_out (cli, " no capability traffic-engineering\n");

  return 0;
}


void
ospf_te_show_router_tlv (struct cli *cli, struct te_tlv_header *tlv)
{
  struct pal_in4_addr *addr;

  if (pal_ntoh16 (tlv->tlv_len) != OSPF_TE_TLV_RTADDR_FIX_LEN)
    {
      cli_out (cli, "      Mismatched length SubTLV type %u length %u\n",
               pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
      return;
    }

  addr = (struct pal_in4_addr *) TE_TLV_DATA (tlv);
  cli_out (cli, "    MPLS TE router ID : %r\n\n", addr);
}

void
ospf_te_show_link_stlv_link_type (struct cli *cli, struct te_tlv_header *tlv)
{
  if (pal_ntoh16 (tlv->tlv_len) != OSPF_TE_TLV_LINK_TYPE_FIX_LEN)
    {
      cli_out (cli, "      Mismatched length SubTLV type %u length %u\n",
               pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
      return;
    }

  cli_out (cli, "    Link connected to ");

  switch (*((u_char *) TE_TLV_DATA (tlv)))
    {
    case TE_LINK_TYPE_PTP:
      cli_out (cli, "Point-to-Point");
      break;
    case TE_LINK_TYPE_MULTIACCESS:
      cli_out (cli, "Broadcast");
      break;
    default:
      cli_out (cli, "Unknown");
      break;
    }
  cli_out (cli, " network\n");
}

void
ospf_te_show_link_stlv_link_id (struct cli *cli, struct te_tlv_header *tlv)
{
  struct pal_in4_addr *addr;

  if (pal_ntoh16 (tlv->tlv_len) != OSPF_TE_TLV_LINK_ID_FIX_LEN)
    {
      cli_out (cli, "      Mismatched length SubTLV type %u length %u\n",
               pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
      return;
    }

  addr = (struct pal_in4_addr *) TE_TLV_DATA (tlv);
  cli_out (cli, "      Link ID : %r\n", addr);
}

void
ospf_te_show_link_stlv_local (struct cli *cli, struct te_tlv_header *tlv)
{
  struct pal_in4_addr *addr;
  int i;

  if (pal_ntoh16 (tlv->tlv_len) % 4)
    {
      cli_out (cli, "      Mismatched length SubTLV type %u length %u\n",
               pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
      return;
    }

  addr = (struct pal_in4_addr *) TE_TLV_DATA (tlv);
  for (i = 4; i <= pal_ntoh16 (tlv->tlv_len); i += 4, addr++)
    cli_out (cli, "      Interface Address : %r\n", addr);
}

void
ospf_te_show_link_stlv_remote (struct cli *cli, struct te_tlv_header *tlv)
{
  struct pal_in4_addr *addr;
  int i;

  if (pal_ntoh16 (tlv->tlv_len) % 4)
    {
      cli_out (cli, "      Mismatched length SubTLV type %u length %u\n",
               pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
      return;
    }

  addr = (struct pal_in4_addr *) TE_TLV_DATA (tlv);
  for (i = 4; i <= pal_ntoh16 (tlv->tlv_len); i += 4, addr++)
    cli_out (cli, "      Neighbor Address : %r\n", addr);
}

void
ospf_te_show_link_stlv_metric (struct cli *cli, struct te_tlv_header *tlv)
{
  u_int32_t *metric;

  if (pal_ntoh16 (tlv->tlv_len) != OSPF_TE_TLV_METRIC_FIX_LEN)
    {
      cli_out (cli, "      Mismatched length SubTLV type %u length %u\n",
               pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
      return;
    }

  metric = (u_int32_t *) TE_TLV_DATA (tlv);
  cli_out (cli, "      Admin Metric : %d\n", pal_ntoh32 (*metric));
}

void
ospf_te_show_link_stlv_max_bw (struct cli *cli, struct te_tlv_header *tlv)
{
  union
  {
    u_int32_t i;
    float32_t f;
  } bw;
  u_int32_t *ptr;

  if (pal_ntoh16 (tlv->tlv_len) != OSPF_TE_TLV_BW_FIX_LEN)
    {
      cli_out (cli, "      Mismatched length SubTLV type %u length %u\n",
               pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
      return;
    }

  ptr = (u_int32_t *) TE_TLV_DATA (tlv);
  bw.i = pal_ntoh32 (*ptr);
  cli_out (cli, "      Maximum bandwidth : %.2f Kbits/s\n", bw.f * 8 / 1000);
}

void
ospf_te_show_link_stlv_res_bw (struct cli *cli, struct te_tlv_header *tlv) 
{
  union
  {
    u_int32_t i;
    float32_t f;
  } bw;
  u_int32_t *ptr;

  if (pal_ntoh16 (tlv->tlv_len) != OSPF_TE_TLV_BW_FIX_LEN)
    {
      cli_out (cli, "      Mismatched length SubTLV type %u length %u\n",
               pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
      return;
    }

  ptr = (u_int32_t *) TE_TLV_DATA (tlv);
  bw.i = pal_ntoh32 (*ptr);
  cli_out (cli, "      Maximum reservable bandwidth : %.2f Kbits/s\n",
           bw.f * 8 / 1000);
}

void
ospf_te_show_link_stlv_unres_bw (struct cli *cli, struct te_tlv_header *tlv)
{
  int i;
  union
  {
    u_int32_t i;
    float32_t f;
  } bw;
  u_int32_t *ptr;

  if (pal_ntoh16 (tlv->tlv_len) != OSPF_TE_TLV_UNRSV_BW_FIX_LEN)
    {
      cli_out (cli, "      Mismatched length SubTLV type %u length %u\n",
               pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
      return;
    }

  cli_out (cli, "      Unreserved Bandwidth : \n");
  ptr = (u_int32_t *) TE_TLV_DATA (tlv);
  cli_out (cli, "      Number of Priority : %d\n", MAX_PRIORITIES);

  for (i = 0; i < MAX_PRIORITIES; i += 2)
    {
      bw.i = pal_ntoh32 (*ptr++);
      cli_out (cli, "      Priority %d : %.2f Kbits/s ", i,
               bw.f * 8 / 1000);
      bw.i = pal_ntoh32 (*ptr++);
      cli_out (cli, "      Priority %d : %.2f Kbits/s\n", i + 1,
               bw.f * 8 / 1000);
    }
}

void
ospf_te_show_link_stlv_admin_group (struct cli *cli, struct te_tlv_header *tlv)
{
  u_int32_t *admin;

  if (pal_ntoh16 (tlv->tlv_len) != OSPF_TE_TLV_RSRC_COLOR_FIX_LEN)
    {
      cli_out (cli, "      Mismatched length SubTLV type %u length %u\n",
               pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
      return;
    }

  admin = (u_int32_t *) TE_TLV_DATA (tlv); 
  cli_out (cli, "      Affinity Bit : 0x%x\n", pal_ntoh32 (*admin));
}

void
ospf_te_show_link_stlv_bc (struct cli *cli, struct te_tlv_header *tlv)
{
  u_char *bc_model = NULL;
  int i;
  u_int32_t *ptr = NULL;

  union
  {
    u_int32_t i;
    float32_t f;
  } bc;


  if (pal_ntoh16 (tlv->tlv_len) < OSPF_TE_TLV_BC_MIN_LEN)
    {
      cli_out (cli, "      Mismatched length SubTLV type %u length %u\n",
              pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
      return;
    }
  
  bc_model = (u_char *) TE_TLV_DATA (tlv);
  cli_out (cli, "      Bandwidth Constraint Model : %d\n", *bc_model);

  ptr = (u_int32_t *) (++bc_model);
  for (i = 0 ; i < MAX_PRIORITIES ; i++)
    {
      bc.i = pal_ntoh32 (*++ptr);
      cli_out (cli, "      Bandwidth Constraint at priority %d : %.3f Kbits/s\n",
               i, bc.f);
      
    }
}

#ifdef HAVE_GMPLS
void
ospf_te_show_link_stlv_local_remote_id (struct cli *cli,
                                        struct te_tlv_header *tlv)
{
  struct pal_in4_addr *id;
  struct pal_in4_addr local_id, remote_id;

  if (pal_ntoh16 (tlv->tlv_len) != OSPF_TE_TLV_LOCAL_REMOTE_ID_FIX_LEN)
    {
      cli_out (cli, "      Mismatched length SubTLV type %u length %u\n",
               pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
      return;
    }

  id = (struct pal_in4_addr *) TE_TLV_DATA (tlv);

  local_id = *(id++);
  local_id.s_addr = pal_ntoh32 (local_id.s_addr);
  remote_id = *(id++);
  remote_id.s_addr = pal_ntoh32 (remote_id.s_addr);

  cli_out (cli, "      Link Local Identifier  : %r\n",  (&local_id));
  cli_out (cli, "      Link Local Identifier  : %r\n",  (&remote_id));
}

void
ospf_te_show_link_stlv_protection_type (struct cli *cli,
                                        struct te_tlv_header *tlv)
{
  u_char *type;

  if (pal_ntoh16 (tlv->tlv_len) != OSPF_TE_TLV_PROTECTION_TYPE_FIX_LEN)
    {
      cli_out (cli, "      Mismatched length SubTLV type %u length %u\n",
               pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
      return;
    }

  type = (u_char *) TE_TLV_DATA (tlv);

  cli_out (cli, "      Link Protection Type : \n");
  if (CHECK_FLAG (*type, GMPLS_PROTECTION_TYPE_EXTRA_TRAFFIC))
    cli_out (cli, "        Extra Traffic\n");
  if (CHECK_FLAG (*type, GMPLS_PROTECTION_TYPE_UNPROTECTED))
    cli_out (cli, "        Unprotected\n");
  if (CHECK_FLAG (*type, GMPLS_PROTECTION_TYPE_SHARED))
    cli_out (cli, "        Shared\n");
  if (CHECK_FLAG (*type, GMPLS_PROTECTION_TYPE_DEDICATED_1TO1))
    cli_out (cli, "        Dedicated 1:1\n");
  if (CHECK_FLAG (*type, GMPLS_PROTECTION_TYPE_DEDICATED_1PLUS1))
    cli_out (cli, "        Dedicated 1+1\n");
  if (CHECK_FLAG (*type, GMPLS_PROTECTION_TYPE_ENHANCED))
    cli_out (cli, "        Enhanced\n");
}

void
ospf_te_show_link_stlv_capability (struct cli *cli,
                                   struct te_tlv_header *tlv)
{
  u_int8_t *capability;
  u_int8_t *encoding;
  u_int8_t *indication;
  u_int16_t *mtu;
  u_int32_t *ptr;
  union
  {
    u_int32_t i;
    float32_t f;
  } bw;
  int i;

  if (pal_ntoh16 (tlv->tlv_len) < OSPF_TE_TLV_IF_SW_CAPABILITY_MIN_LEN)
    {
      cli_out (cli, "      Mismatched length SubTLV type %u length %u\n",
               pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
      return;
    }

  cli_out (cli, "      Link Switching Capability Discriptor : \n");

  capability = (u_char *) TE_TLV_DATA (tlv);
  cli_out (cli, "        Capability Type : %s\n",
           gmpls_capability_type_str (*capability));
  encoding = capability + 1;
  cli_out (cli, "        Encoding Type : %s\n",
           gmpls_encoding_type_str (*encoding));

  ptr = (u_int32_t *) TE_TLV_DATA (tlv);
  for (i = 0; i < MAX_PRIORITIES; i++)
    {
      bw.i = pal_ntoh32 (*++ptr);
      cli_out (cli, "        Max LSP Bandwidth at priority %d : %.02f\n",
               i, bw.f);
    }

  /* Switching Capability-specific information. */
  switch (*capability)
    {
    case GMPLS_SWITCH_CAPABILITY_FLAG_PSC1:
    case GMPLS_SWITCH_CAPABILITY_FLAG_PSC2:
    case GMPLS_SWITCH_CAPABILITY_FLAG_PSC3:
    case GMPLS_SWITCH_CAPABILITY_FLAG_PSC4:
      if (pal_ntoh16 (tlv->tlv_len) < OSPF_TE_TLV_IF_SW_CAPABILITY_PSC_LEN)
        return;
      bw.i = pal_ntoh32 (*++ptr);
      mtu = (u_int16_t *) ++ptr;
      cli_out (cli, "          Minimum LSP Bandwidth : %.02f\n", bw.f);
      cli_out (cli, "          Interface MTU : %d\n", pal_ntoh16 (*mtu));
      break;
    case GMPLS_SWITCH_CAPABILITY_FLAG_TDM:
      if (pal_ntoh16 (tlv->tlv_len) < OSPF_TE_TLV_IF_SW_CAPABILITY_TDM_LEN)
        return;
      bw.i = pal_ntoh32 (*++ptr);
      indication = (u_int8_t *) ++ptr;
      cli_out (cli, "          Minimum LSP Bandwidth : %.02f\n", bw.f);
#ifdef TBD_GMPLS
      cli_out (cli, "          SONET/SDH Indication : %s\n",
               gmpls_sdh_indication_str (indication));
#endif
      break;
    default:
      break;
    }
}

void
ospf_te_show_link_stlv_shared_risk_group (struct cli *cli,
                                          struct te_tlv_header *tlv)
{
  u_int32_t *group = (u_int32_t *) TE_TLV_DATA (tlv);
  int i, len;

  len = pal_ntoh16 (tlv->tlv_len);
  if (len % 4)
    {
      cli_out (cli, "      Mismatched length SubTLV type %u length %u\n",
               pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
      return;
    }

  cli_out (cli, "      Shared Risk Link Group : ");
  for (i = 0;  i < len; i += 4)
    {
      if (i % 60 == 0)
        cli_out (cli, "\n        ");
      else
        cli_out (cli, ", ");
      cli_out (cli, "%lu", pal_ntoh32 (*group++));
    }
  cli_out (cli, "\n");
}
#endif /* HAVE_GMPLS */

void
ospf_te_show_stlv_unknown (struct cli *cli, struct te_tlv_header *tlv)
{
  cli_out (cli, "      Unknown SubTLV type %u length %u\n",
           pal_ntoh16 (tlv->tlv_type), pal_ntoh16 (tlv->tlv_len));
}

int
ospf_te_show_link_tlv (struct cli *cli, struct te_tlv_header *tlv)
{
  struct te_tlv_header *stlv = (struct te_tlv_header *)TE_TLV_DATA (tlv);
  u_int16_t type;
  int len;

  for (len = pal_ntoh16 (tlv->tlv_len); len > 0 && TE_TLV_LENGTH (stlv) <= len;
       len -= TE_TLV_LENGTH (stlv), stlv = TE_TLV_NEXT (stlv))
    {
      type = pal_ntoh16 (stlv->tlv_type);

      if (type == TE_LINK_SUBTLV_LINK_TYPE)
        ospf_te_show_link_stlv_link_type (cli, stlv);
      else if (type == TE_LINK_SUBTLV_LINK_ID)
        ospf_te_show_link_stlv_link_id (cli, stlv);
      else if (type == TE_LINK_SUBTLV_LOCAL_ADDRESS)
        ospf_te_show_link_stlv_local (cli, stlv);
      else if (type == TE_LINK_SUBTLV_REMOTE_ADDRESS)
        ospf_te_show_link_stlv_remote (cli, stlv);
      else if (type == TE_LINK_SUBTLV_TE_METRIC)
        ospf_te_show_link_stlv_metric (cli, stlv);
      else if (type == TE_LINK_SUBTLV_MAX_BANDWIDTH)
        ospf_te_show_link_stlv_max_bw (cli, stlv);
      else if (type == TE_LINK_SUBTLV_MAX_RES_BANDWIDTH)
        ospf_te_show_link_stlv_res_bw (cli, stlv);
      else if (type == TE_LINK_SUBTLV_UNRES_BANDWIDTH)
        ospf_te_show_link_stlv_unres_bw (cli, stlv);
      else if (type == TE_LINK_SUBTLV_RESOURCE_CLASS)
        ospf_te_show_link_stlv_admin_group (cli, stlv);
      else if (type == TE_LINK_SUBTLV_BC)
        ospf_te_show_link_stlv_bc (cli, stlv);
#ifdef HAVE_GMPLS
      else if (type == TE_LINK_SUBTLV_LOCAL_REMOTE_ID)
        ospf_te_show_link_stlv_local_remote_id (cli, stlv);
      else if (type == TE_LINK_SUBTLV_PROTECTION_TYPE)
        ospf_te_show_link_stlv_protection_type (cli, stlv);
      else if (type == TE_LINK_SUBTLV_IF_SW_CAPABILITY)
        ospf_te_show_link_stlv_capability (cli, stlv);
      else if (type == TE_LINK_SUBTLV_SHARED_RISK_GROUP)
        ospf_te_show_link_stlv_shared_risk_group (cli, stlv);
#endif /* HAVE_GMPLS */
      else
        ospf_te_show_stlv_unknown (cli, stlv);
    }
  cli_out (cli, "\n");

  return 1;
}

int
ospf_te_show_opaque_data (struct cli *cli, struct opaque_lsa *lsa)
{
  struct te_tlv_header *tlv = (struct te_tlv_header *)lsa->opaque_data;
  u_int16_t len = OPAQUE_DATA_LENGTH (lsa);
  int number = 0;

  cli_out (cli, "\n");

  while (TE_TLV_OK (tlv, len))
    {
      switch (pal_ntoh16 (tlv->tlv_type))
        {
        case TE_TLV_ROUTER_TYPE:
          ospf_te_show_router_tlv (cli, tlv);
          break;
        case TE_TLV_LINK_TYPE:
          if (ospf_te_show_link_tlv (cli, tlv))
            number++;
          break;
        }
      len -= TE_TLV_LENGTH (tlv);
      tlv = TE_TLV_NEXT (tlv);
    }

  cli_out (cli, "    Number of Links : %d\n\n", number);

  return 0;
}


int
ospf_te_init (struct ospf_master *om)
{
  ospf_register_notifier (om, OSPF_NOTIFY_IFSM_STATE_CHANGE,
                          ospf_te_ifsm_state_change, NULL, 0);
  ospf_register_notifier (om, OSPF_NOTIFY_NFSM_STATE_CHANGE,
                          ospf_te_nfsm_state_change, NULL, 0);
  ospf_register_notifier (om, OSPF_NOTIFY_AREA_NEW,
                          ospf_te_area_new, NULL, 0);
  ospf_register_notifier (om, OSPF_NOTIFY_PROCESS_DEL,
                          ospf_te_process_del, NULL, 0);
  ospf_register_notifier (om, OSPF_NOTIFY_NETWORK_DEL,
                          ospf_te_network_del, NULL, 0);
  ospf_register_notifier (om, OSPF_NOTIFY_ROUTER_ID_CHANGED,
                          ospf_te_router_id_changed, NULL, 0);
  ospf_register_notifier (om, OSPF_NOTIFY_WRITE_PROCESS_CONFIG,
                          ospf_te_write_config, NULL, 0);
  
  return 0;
}


#ifdef HAVE_OSPF_CSPF
struct ospf_lsa *
ospf_te_lsa_new (struct ospf_lsa *lsa, struct ospf_te_lsa_data **data)
{
  struct ospf_lsa *new_lsa;

  new_lsa = XMALLOC (MTYPE_OSPF_LSA, sizeof (struct ospf_lsa));
  pal_mem_set (new_lsa, 0, sizeof (struct ospf_lsa));
  new_lsa->data = (struct lsa_header *)(*data);

  new_lsa->flags = lsa->flags;
  new_lsa->tv_update = lsa->tv_update;

  return new_lsa;
}

void
ospf_te_lsa_free (struct ospf_lsa *lsa)
{
  if (lsa == NULL)
    return;

  if (lsa->data)
    {
      struct ospf_te_lsa_data *data;

      data = (struct ospf_te_lsa_data *)(lsa->data);

      ospf_te_lsa_data_free (data);
    }

  XFREE (MTYPE_OSPF_LSA, lsa);
}

void
ospf_te_lsa_data_free (struct ospf_te_lsa_data *data)
{
  if (data == NULL)
    return;

  if (data->type == TE_TLV_LINK_TYPE)
    {
      struct ospf_te_tlv_link_lsa *lsa;

      lsa = &data->ospf_te_tlv_lsa.link_lsa;

      if ((CHECK_FLAG (lsa->flag, TE_LINK_ATTR_LOCAL_IF_ADDR)) &&
         (lsa->local_addr.addr))
       XFREE (MTYPE_TMP, lsa->local_addr.addr);

      if ((CHECK_FLAG (lsa->flag, TE_LINK_ATTR_REMOTE_IF_ADDR)) &&
         (lsa->remote_addr.addr))
       XFREE (MTYPE_TMP, lsa->remote_addr.addr);
    }

  XFREE (MTYPE_OSPF_TE_LSA_DATA, data);
}

int
ospf_te_tlv_header_decode (u_char **pnt, u_int16_t *size,
                           struct te_tlv_header *tlv_h)
{
  if (*size < OSPF_TE_TLV_HEADER_FIX_LEN)
    return CSPF_ERR_PKT_TOO_SMALL;

  DECODE_GETW (&tlv_h->tlv_type);
  DECODE_GETW (&tlv_h->tlv_len);

  tlv_h->tlv_type = pal_ntoh16 (tlv_h->tlv_type);
  tlv_h->tlv_len = pal_ntoh16 (tlv_h->tlv_len);

  return OSPF_TE_TLV_HEADER_FIX_LEN;
}

void
ospf_te_lsa_header_decode (u_char **pnt, u_int16_t *size,
                           struct lsa_header *header)
{
  pal_mem_set (header, 0, sizeof (struct lsa_header));
  
  /* get lsa header */
  DECODE_GETW (&header->ls_age);
  DECODE_GETC (&header->options);
  DECODE_GETC (&header->type);
  DECODE_GETL (&header->id);
  DECODE_GETL (&header->adv_router);
  DECODE_GETL (&header->ls_seqnum);
  DECODE_GETW (&header->checksum);
  DECODE_GETW (&header->length);
}


int
ospf_te_lsa_decode (u_char **pnt, u_int16_t *size,
                    struct ospf_te_lsa_data *te_lsa)
{
  struct te_tlv_header tlv_h;
  int ret;

  while (*size > 0)
    {
      pal_mem_set (&tlv_h, 0, sizeof (struct te_tlv_header));
      
      /* decode tlv header */
      ret = ospf_te_tlv_header_decode (pnt, size, &tlv_h);
      if (ret < 0)
        return ret;

      /* skip zero length TLVs */
      if (tlv_h.tlv_len == 0)
        continue;
      
      if (tlv_h.tlv_type == TE_TLV_ROUTER_TYPE)
        {
          ret = ospf_te_tlv_rtaddr_lsa_decode (pnt, size, &tlv_h,
                                               &te_lsa->
                                               ospf_te_tlv_lsa.rtaddr_lsa);
          te_lsa->type = tlv_h.tlv_type;
          return ret;
        }
      else if (tlv_h.tlv_type == TE_TLV_LINK_TYPE)
        {
          ret = ospf_te_tlv_link_lsa_decode (pnt, size, &tlv_h,
                                             &te_lsa->
                                             ospf_te_tlv_lsa.link_lsa);
          te_lsa->type = tlv_h.tlv_type;
          return ret;
        }
      else
        {
          ret = ospf_te_tlv_unknown_decode (pnt, size, &tlv_h);
          if (ret < 0)
            return ret;
        }
    } 

  return CSPF_SUCCESS;
}

int
ospf_te_tlv_rtaddr_lsa_decode (u_char **pnt, u_int16_t *size,
                               struct te_tlv_header *tlv_h,
                               struct ospf_te_tlv_rtaddr_lsa *rtaddr)
{
  if (*size < OSPF_TE_TLV_RTADDR_FIX_LEN)
    return CSPF_ERR_PKT_TOO_SMALL;

  rtaddr->tlv_h = *tlv_h;

  DECODE_GETL (&rtaddr->router_id);

  return OSPF_TE_TLV_RTADDR_FIX_LEN;
}

int
ospf_te_tlv_link_lsa_decode (u_char **pnt, u_int16_t *size,
                             struct te_tlv_header *tlv_h,
                             struct ospf_te_tlv_link_lsa *link)
{
  u_char *spnt = *pnt;
  struct te_tlv_header header;
  int ret;
#ifdef HAVE_GMPLS
  int i; 
#endif /* HAVE_GMPLS */

  link->tlv_h = *tlv_h;

  link->number_sw_cap = 0;

  while ((*pnt - spnt) < tlv_h->tlv_len)
    {
      pal_mem_set (&header, 0, sizeof (struct te_tlv_header));

      ret = ospf_te_tlv_header_decode (pnt, size, &header);
      if (ret < 0)
       return ret;

      switch (header.tlv_type)
       {
       case TE_LINK_SUBTLV_LINK_TYPE :
         if (CHECK_FLAG (link->flag, TE_LINK_ATTR_LINK_TYPE))
           return CSPF_ERR_INVALID_TLV;

         ret = ospf_te_tlv_link_type_decode (pnt, size, &header,
                                             &link->link_type);
         if (ret < 0)
           return ret;

         SET_FLAG (link->flag, TE_LINK_ATTR_LINK_TYPE);
         break;

       case TE_LINK_SUBTLV_LINK_ID :
         if (CHECK_FLAG (link->flag, TE_LINK_ATTR_LINK_ID))
           return CSPF_ERR_INVALID_TLV;

         ret = ospf_te_tlv_link_id_decode (pnt, size, &header,
                                           &link->link_id);
         if (ret < 0)
           return ret;

         SET_FLAG (link->flag, TE_LINK_ATTR_LINK_ID);
         break;

        case TE_LINK_SUBTLV_LOCAL_ADDRESS :
          if (CHECK_FLAG (link->flag, TE_LINK_ATTR_LOCAL_IF_ADDR))
            return CSPF_ERR_INVALID_TLV;

          ret = ospf_te_tlv_if_addr_decode (pnt, size, &header,
                                            &link->local_addr);
          if (ret < 0)
            return ret;

          SET_FLAG (link->flag, TE_LINK_ATTR_LOCAL_IF_ADDR);
          break;

        case TE_LINK_SUBTLV_REMOTE_ADDRESS :
          if (CHECK_FLAG (link->flag, TE_LINK_ATTR_REMOTE_IF_ADDR))
            return CSPF_ERR_INVALID_TLV;

          ret = ospf_te_tlv_if_addr_decode (pnt, size, &header,
                                            &link->remote_addr);
          if (ret < 0)
            return ret;

          SET_FLAG (link->flag, TE_LINK_ATTR_REMOTE_IF_ADDR);
          break;

        case TE_LINK_SUBTLV_TE_METRIC :
          if (CHECK_FLAG (link->flag, TE_LINK_ATTR_TE_METRIC))
            return CSPF_ERR_INVALID_TLV;
          ret = ospf_te_tlv_metric_decode (pnt, size, &header,
                                           &link->te_metric);
          if (ret < 0)
            return ret;

          SET_FLAG (link->flag, TE_LINK_ATTR_TE_METRIC);
          break;

        case TE_LINK_SUBTLV_MAX_BANDWIDTH :
          if (CHECK_FLAG (link->flag, TE_LINK_ATTR_MAX_BANDWIDTH))
            return CSPF_ERR_INVALID_TLV;

          ret = ospf_te_tlv_bw_decode (pnt, size, &header,
                                       &link->max_bw);
          if (ret < 0)
            return ret;

          SET_FLAG (link->flag, TE_LINK_ATTR_MAX_BANDWIDTH);
          break;

        case TE_LINK_SUBTLV_MAX_RES_BANDWIDTH :
          if (CHECK_FLAG (link->flag, TE_LINK_ATTR_MAX_RES_BANDWIDTH))
            return CSPF_ERR_INVALID_TLV;

          ret = ospf_te_tlv_bw_decode (pnt, size, &header,
                                       &link->max_res_bw);
          if (ret < 0)
            return ret;

          SET_FLAG (link->flag, TE_LINK_ATTR_MAX_RES_BANDWIDTH);
          break;

        case TE_LINK_SUBTLV_UNRES_BANDWIDTH :
          if (CHECK_FLAG (link->flag, TE_LINK_ATTR_UNRSV_BANDWIDTH))
            return CSPF_ERR_INVALID_TLV;

          ret = ospf_te_tlv_unrsv_bw_decode (pnt, size, &header,
                                             &link->unrsv_bw);
          if (ret < 0)
            return ret;

          SET_FLAG (link->flag, TE_LINK_ATTR_UNRSV_BANDWIDTH);
          break;

        case TE_LINK_SUBTLV_RESOURCE_CLASS :
          if (CHECK_FLAG (link->flag, TE_LINK_ATTR_RSRC_COLOR))
            return CSPF_ERR_INVALID_TLV;

          ret = ospf_te_tlv_rsrc_color_decode (pnt, size, &header,
                                               &link->color);
          if (ret < 0)
            return ret;

          SET_FLAG (link->flag, TE_LINK_ATTR_RSRC_COLOR);
          break;

#ifdef HAVE_DSTE
        case TE_LINK_SUBTLV_BC :
          if (CHECK_FLAG (link->flag, TE_LINK_ATTR_BC))
            return CSPF_ERR_INVALID_TLV;

          ret = ospf_te_tlv_bc_decode (pnt, size, &header,
                                 &link->bc);
          if (ret < 0)
            return ret;

          SET_FLAG (link->flag, TE_LINK_ATTR_BC);
          break;
#endif /* HAVE_DSTE */

#ifdef HAVE_GMPLS
        case TE_LINK_SUBTLV_LOCAL_REMOTE_ID :
          if (CHECK_FLAG (link->flag, TE_LINK_ATTR_LINK_LOCAL_REMOTE_ID))
            return CSPF_ERR_INVALID_TLV;

          ret = ospf_te_tlv_local_remote_id_decode (pnt, size, &header,
                                           &link->local_remote_id);
          if (ret < 0)
            return ret;

          SET_FLAG (link->flag, TE_LINK_ATTR_LINK_LOCAL_REMOTE_ID);
          break;

        case TE_LINK_SUBTLV_IF_SW_CAPABILITY :
          if (CHECK_FLAG (link->flag, TE_LINK_ATTR_SWITCHING_CAP))
            return CSPF_ERR_INVALID_TLV;

          /* At present only one capability is supported */
          link->number_sw_cap = 1;
          link->cap = XCALLOC (MTYPE_TMP, 
                link->number_sw_cap*sizeof (struct ospf_te_tlv_capability));

          if (!link->cap)
             return 0;

          for (i = 0; i < link->number_sw_cap; i++)
            {
              ret = ospf_te_tlv_capability_decode (pnt, size, &header, 
                                                   &link->cap[i]);
              if (ret < 0)
                return ret;
            }

          SET_FLAG (link->flag, TE_LINK_ATTR_SWITCHING_CAP);
          break;

        case TE_LINK_SUBTLV_PROTECTION_TYPE :
          if (CHECK_FLAG (link->flag, TE_LINK_ATTR_PROTECTION))
            return CSPF_ERR_INVALID_TLV;

          ret = ospf_te_tlv_protection_decode (pnt, size, &header,
                                           &link->protection);
          if (ret < 0)
            return ret;

          SET_FLAG (link->flag, TE_LINK_ATTR_PROTECTION);
          break;

        case TE_LINK_SUBTLV_SHARED_RISK_GROUP :
          if (CHECK_FLAG (link->flag, TE_LINK_ATTR_SRLG))
            return CSPF_ERR_INVALID_TLV;

          ret = ospf_te_tlv_srlg_decode (pnt, size, &header, &link->srlg);
          if (ret < 0)
            return ret;

          SET_FLAG (link->flag, TE_LINK_ATTR_SRLG);
          break;
#endif /* HAVE_GMPLS */

       default :
         ret = ospf_te_tlv_unknown_decode (pnt, size, &header);
         if (ret < 0)
           return ret;
         
         break;
       }
    }
  
  if ((*pnt) - spnt != tlv_h->tlv_len)
    return CSPF_ERR_DECODE_LENGTH;
  
  if ((! CHECK_FLAG (link->flag, TE_LINK_ATTR_LINK_TYPE)) ||
      (! CHECK_FLAG (link->flag, TE_LINK_ATTR_LINK_ID)))
    {
      return CSPF_ERR_INVALID_TLV;
    }

  return tlv_h->tlv_len;
}

int
ospf_te_tlv_rsrc_color_decode (u_char **pnt, u_int16_t *size,
                               struct te_tlv_header *tlv_h,
                               struct ospf_te_tlv_rsrc_color *color)
{
  u_int16_t length;

  length = tlv_h->tlv_len;

  if (length != OSPF_TE_TLV_RSRC_COLOR_FIX_LEN)
    return CSPF_ERR_INVALID_TLV_LENGTH;

  if (*size < length)
    return CSPF_ERR_PKT_TOO_SMALL;

  color->tlv_h = *tlv_h;

  DECODE_GETL (&color->color);
  color->color = pal_ntoh32 (color->color);

  return length;
}


int
ospf_te_tlv_unrsv_bw_decode (u_char **pnt, u_int16_t *size,
                             struct te_tlv_header *tlv_h,
                             struct ospf_te_tlv_unrsv_bw *unrsv_bw)
{
  u_int16_t length;
  u_int32_t tmp[8];
  int i;

  length = tlv_h->tlv_len;

  if (length != OSPF_TE_TLV_UNRSV_BW_FIX_LEN)
    return CSPF_ERR_INVALID_TLV_LENGTH;

  if (*size < length)
    return CSPF_ERR_PKT_TOO_SMALL;

  unrsv_bw->tlv_h = *tlv_h;

  for (i = 0; i < MAX_PRIORITIES; i++)
    {
      DECODE_GETL (&tmp[i]);
      tmp[i] = pal_ntoh32 (tmp[i]);
      pal_mem_cpy (&unrsv_bw->bw[i], &tmp[i], sizeof (u_int32_t));
    }

  return length;
}

int
ospf_te_tlv_bw_decode (u_char **pnt, u_int16_t *size,
                       struct te_tlv_header *tlv_h,
                       struct ospf_te_tlv_bw *bw)
{
  u_int16_t length;
  u_int32_t tmp;

  length = tlv_h->tlv_len;

  if (length != OSPF_TE_TLV_BW_FIX_LEN)
    return CSPF_ERR_INVALID_TLV_LENGTH;

  if (*size < length)
    return CSPF_ERR_PKT_TOO_SMALL;

  bw->tlv_h = *tlv_h;

  DECODE_GETL (&tmp);
  tmp = pal_ntoh32 (tmp);
  pal_mem_cpy (&bw->bw, &tmp, sizeof (u_int32_t));

  return length;
}

int
ospf_te_tlv_metric_decode (u_char **pnt, u_int16_t *size,
                           struct te_tlv_header *tlv_h,
                           struct ospf_te_tlv_metric *metric)
{
  u_int16_t length;

  length = tlv_h->tlv_len;

  if (length != OSPF_TE_TLV_METRIC_FIX_LEN)
    return CSPF_ERR_INVALID_TLV_LENGTH;

  if (*size < length)
    return CSPF_ERR_PKT_TOO_SMALL;

  metric->tlv_h = *tlv_h;

  DECODE_GETL (&metric->te_metric);

  metric->te_metric = pal_ntoh32 (metric->te_metric);

  return length;

}

int
ospf_te_tlv_if_addr_decode (u_char **pnt, u_int16_t *size,
                            struct te_tlv_header *tlv_h,
                            struct ospf_te_tlv_if_addr *addr)
{
  u_int16_t length;
  int count, ctr;

  length = tlv_h->tlv_len;

  if (*size < length)
    return CSPF_ERR_PKT_TOO_SMALL;

  addr->tlv_h = *tlv_h;

  count = length / sizeof (struct pal_in4_addr);

  addr->addr = (struct pal_in4_addr *)XMALLOC (MTYPE_TMP, length);
  pal_mem_set (addr->addr, 0, length);

  for (ctr = 0; ctr < count; ctr++)
    DECODE_GETL (&addr->addr[ctr]);

  return length;
}

int
ospf_te_tlv_link_id_decode (u_char **pnt, u_int16_t *size,
                            struct te_tlv_header *tlv_h,
                            struct ospf_te_tlv_link_id *link_id)
{
  u_int16_t length;

  length = tlv_h->tlv_len;

  if (length != OSPF_TE_TLV_LINK_ID_FIX_LEN)
    return CSPF_ERR_INVALID_TLV_LENGTH;

  if (*size < length)
    return CSPF_ERR_PKT_TOO_SMALL;

  link_id->tlv_h = *tlv_h;

  DECODE_GETL (&link_id->link_id);

  return length;
}

int
ospf_te_tlv_link_type_decode (u_char **pnt, u_int16_t *size,
                              struct te_tlv_header *tlv_h,
                              struct ospf_te_tlv_link_type *link_type)
{
  u_int16_t length;

  length = tlv_h->tlv_len;

  if (length != OSPF_TE_TLV_LINK_TYPE_FIX_LEN)
    return CSPF_ERR_INVALID_TLV_LENGTH;

  if (*size < length)
    return CSPF_ERR_PKT_TOO_SMALL;

  link_type->tlv_h = *tlv_h;

  DECODE_GETC (&link_type->link_type);

  /* discard padding bytes */
  *pnt += (TE_TLV_ALIGN (length) - length);
  *size -= (TE_TLV_ALIGN (length) - length);

  return length;
}

#ifdef HAVE_DSTE
int
ospf_te_tlv_bc_decode (u_char **pnt, u_int16_t *size,
                       struct te_tlv_header *tlv_h,
                       struct ospf_te_tlv_bc *bc)
{
  u_int16_t length;
  u_int32_t tmp32;
  int count;
  int ctr;

  length = tlv_h->tlv_len;

  if (length < OSPF_TE_TLV_BC_MIN_LEN)
    return CSPF_ERR_INVALID_TLV_LENGTH;

  if (*size < length)
    return CSPF_ERR_PKT_TOO_SMALL;

  bc->tlv_h = *tlv_h;

  DECODE_GETC (&bc->bc_model);

  count = (length - 1)/4;
  for (ctr = 0; ctr < count; ctr++)
    {
      DECODE_GETL (&tmp32);
      tmp32 = pal_ntoh32 (tmp32);
      pal_mem_cpy (&bc->bc[ctr], &tmp32, sizeof (u_int32_t));
    }

  /* discard padding bytes */
  *pnt += (TE_TLV_ALIGN (length) - length);
  *size -= (TE_TLV_ALIGN (length) - length);

  return length;
}
#endif /* HAVE_DSTE */

#ifdef HAVE_GMPLS
/** @brief Function to decode local remote identifier

    @param pnt           - Buffer from which the data to be decoded
    @param size          - Buffer length
    @header              - TLV header that need to be filled
    @param sw_cap        - local-remote-id TLV that need to be filled

    @return   number of bytes decoded
*/
int
ospf_te_tlv_local_remote_id_decode (u_char **pnt, u_int16_t *size,
                                    struct te_tlv_header *tlv_h,
                                    struct ospf_te_tlv_local_remote_id *id)
{
  u_int16_t length;
  u_int32_t local_if_id;
  u_int32_t remote_if_id;
  
  length = tlv_h->tlv_len;

  if (length != OSPF_TE_TLV_LOCAL_REMOTE_ID_FIX_LEN)
    return CSPF_ERR_INVALID_TLV_LENGTH;

  if (*size < length)
    return CSPF_ERR_PKT_TOO_SMALL;

  id->tlv_h = *tlv_h;

  DECODE_GETL (&local_if_id);
  id->local_if_id.s_addr = pal_ntoh32 (local_if_id);

  DECODE_GETL (&remote_if_id);
  id->remote_if_id.s_addr = pal_ntoh32 (remote_if_id);

  return length;
}

/** @brief Function to switching capability  

    @param pnt           - Buffer from which the data to be decoded
    @param size          - Buffer length
    @header              - TLV header that need to be filled
    @param sw_cap        - capability TLV that need to be filled

    @return   number of bytes decoded
*/

int
ospf_te_tlv_capability_decode (u_char **pnt, u_int16_t *size,
                               struct te_tlv_header *tlv_h,
                               struct ospf_te_tlv_capability *cap)
{
  u_int16_t length;
  u_int32_t tmp;
  u_int32_t tmp_bw[8];
  int i;

  length = tlv_h->tlv_len;

  if (length < OSPF_TE_TLV_IF_SW_CAPABILITY_MIN_LEN)
    return CSPF_ERR_INVALID_TLV_LENGTH;

  if (*size < length)
    return CSPF_ERR_PKT_TOO_SMALL;

  cap->tlv_h = *tlv_h;

  DECODE_GETC (&cap->capability);
  DECODE_GETC (&cap->encoding);
  DECODE_GETW (&cap->reserve);

  for (i = 0; i < MAX_PRIORITIES; i++)
    {
      DECODE_GETL (&tmp_bw[i]);
      tmp_bw[i] = pal_ntoh32 (tmp_bw[i]);
      pal_mem_cpy (&cap->max_lsp_bw[i], &tmp_bw[i], sizeof (u_int32_t));
    }

#ifdef HAVE_PACKET
  if ((cap->capability >= GMPLS_SWITCH_CAPABILITY_PSC1)
     && (cap->capability <= GMPLS_SWITCH_CAPABILITY_PSC4))
    {
      u_int16_t mtu;
      DECODE_GETL (&tmp);
      tmp = pal_ntoh32 (tmp);
      pal_mem_cpy (&cap->min_lsp_bw, &tmp, sizeof (u_int32_t));

      DECODE_GETW (&mtu);
      cap->mtu = pal_ntoh16 (mtu);
      DECODE_GETW (&cap->pad);

    }
#endif /* HAVE_PACKET */

  return length; 
}

/** @brief Function to Decode protection type data

    @param pnt           - Buffer from which the data to be decoded
    @param size          - Buffer length
    @header              - TLV header that need to be filled
    @param sw_cap        - Protection type TLV that need to be filled

    @return   number of bytes decoded
*/
int
ospf_te_tlv_protection_decode (u_char **pnt, u_int16_t *size,
                               struct te_tlv_header *tlv_h,
                               struct ospf_te_tlv_protection *pro_type)
{
  u_int16_t length;
  int i;

  length = tlv_h->tlv_len;

  if (length < OSPF_TE_TLV_PROTECTION_TYPE_FIX_LEN)
    return CSPF_ERR_INVALID_TLV_LENGTH;

  if (*size < length)
    return CSPF_ERR_PKT_TOO_SMALL;

  pro_type->tlv_h = *tlv_h;

  DECODE_GETC (&pro_type->protection);

  for (i =0; i < 3; i++)
   DECODE_GETC (&pro_type->reserve[i]);

  return length;
}

/** @brief Function to Decode SRLG data

    @param pnt           - Buffer from which the data to be decoded
    @param size          - Buffer length
    @header              - TLV header that need to be filled
    @param sw_cap        - SRLG TLV that need to be filled

    @return   number of bytes decoded
*/
int
ospf_te_tlv_srlg_decode (u_char **pnt, u_int16_t *size,
                        struct te_tlv_header *tlv_h,
                        struct ospf_te_tlv_srlg *srlg)
{
  u_int16_t length;
  int i, count;
  u_int32_t tmp = 0;

  length = tlv_h->tlv_len;

  if (*size < length)
    return CSPF_ERR_PKT_TOO_SMALL;

  srlg->tlv_h = *tlv_h;

  count = ((srlg->tlv_h.tlv_len)/OSPF_SRLG_VALUE_SIZE);

  if (count <=0)
    return count;

  if (srlg->srlg)
    {
      XFREE (MTYPE_OSPF_SRLG, srlg->srlg);
      srlg->srlg = NULL;
    }

  if (count) 
    {
      srlg->srlg = XCALLOC (MTYPE_OSPF_SRLG, count*OSPF_SRLG_VALUE_SIZE);
      if (!srlg->srlg)
        return CSPF_ERR_MEM_ALLOC_FAILURE;
    }

  for (i = 0; i < count; i++)
   {
     DECODE_GETL (&tmp);
     tmp = pal_ntoh32 (tmp);
     srlg->srlg[i] = tmp;
   }

  return tlv_h->tlv_len;
}

#endif /* HAVE_GMPLS */

int
ospf_te_tlv_unknown_decode (u_char **pnt, u_int16_t *size,
                            struct te_tlv_header *tlv_h)
{
  u_int16_t len;

  if (*size < tlv_h->tlv_len)
    return CSPF_ERR_PKT_TOO_SMALL;
  
  len = TE_TLV_ALIGN (tlv_h->tlv_len);

  *pnt += len;
  *size -= len;

  return len;
}

void
ospf_te_process_lsa_add (struct ospf_master *om,
                         struct ptree *te_lsdb, struct ospf_lsa *lsa)
{
  struct ospf_te_lsa_data *te_lsa_data;
  struct ospf_lsa *new_lsa;
  u_char link_lsa_added = CSPF_FALSE;
  struct lsa_header lsa_hdr;
  u_char *pnt = NULL;
  u_int16_t size;
  int ret;

  if ((! te_lsdb) || (! lsa))
    return;

  if (! IS_OSPF_TE_LSA (lsa))
    return;

  size = pal_ntoh16 (lsa->data->length);
  pnt = (u_char *)(lsa->data);

  /* decode lsa header */
  ospf_te_lsa_header_decode (&pnt, &size, &lsa_hdr);

  while (size > 0)
    {
      te_lsa_data = XMALLOC (MTYPE_OSPF_TE_LSA_DATA,
                             sizeof (struct ospf_te_lsa_data));
      pal_mem_set (te_lsa_data, 0, sizeof (struct ospf_te_lsa_data));      
      
      /* copy over the common lsa header */
      pal_mem_cpy (&te_lsa_data->header, &lsa_hdr, 
                   sizeof (struct lsa_header));

      /* decode te lsa */
      ret = ospf_te_lsa_decode (&pnt, &size, te_lsa_data);
      if (ret < 0)
        {
          ospf_te_lsa_data_free (te_lsa_data);
          return;
        }
      
      if (te_lsa_data->type == TE_TLV_LINK_TYPE)
        {
          if (! link_lsa_added)
            link_lsa_added = CSPF_TRUE;
          else
            {
              zlog_err (ZG, "Invalid OSPF-TE LSA : Multiple TE link TLVs");
              ospf_te_lsa_data_free (te_lsa_data);
              return;
            }
        }
      
      /* create new ospf lsa */
      new_lsa = ospf_te_lsa_new (lsa, &te_lsa_data);
      
      /* add lsa to te database */
      ospf_te_lsdb_add (te_lsdb, new_lsa);  
    }

  return;
}

void
ospf_te_process_lsa_delete (struct ospf_master *om,
                            struct ptree *te_lsdb, struct ospf_lsa *lsa)
{
  u_int16_t size;
  u_char *pnt;
  struct ospf_te_lsa_data lsa_data;
  struct lsa_header lsa_hdr;
  struct prefix p; 
  int ret;

  if ((! te_lsdb) || (! lsa))
    return;

  if (! IS_OSPF_TE_LSA (lsa))
    return;

  size = pal_ntoh16 (lsa->data->length);
  pnt = (u_char *)(lsa->data);

  /* decode lsa header */
  ospf_te_lsa_header_decode (&pnt, &size, &lsa_hdr);
   
  while (size > 0)
    {
      pal_mem_set (&lsa_data, 0, sizeof (struct ospf_te_lsa_data));
      pal_mem_cpy (&lsa_data.header, &lsa_hdr, sizeof (struct lsa_header));
       
      /* decode te lsa */
      ret = ospf_te_lsa_decode (&pnt, &size, &lsa_data);
      if (ret > 0)
        {
          struct ospf_lsa *new_lsa;
           
          new_lsa = ospf_te_lsdb_lookup (te_lsdb,
                                         lsa->data->adv_router,
                                         lsa_data.type,
                                         &lsa->data->id, CSPF_TRUE,
                                         CSPF_TRUE);
          if (new_lsa)
            ospf_te_lsa_free (new_lsa);
        }
      if (lsa_data.type == TE_TLV_LINK_TYPE)
        {
          struct ospf_te_tlv_link_lsa *link_lsa;
           
          link_lsa = &lsa_data.ospf_te_tlv_lsa.link_lsa;
          if (CHECK_FLAG (link_lsa->flag, TE_LINK_ATTR_LOCAL_IF_ADDR))
            {
              cspf_ipv4_prefix_set (&p, link_lsa->local_addr.addr);
              cspf_te_addr_delete_process (om->cspf, &p);
              XFREE (MTYPE_TMP, link_lsa->local_addr.addr);
            }

          if (CHECK_FLAG (link_lsa->flag, TE_LINK_ATTR_REMOTE_IF_ADDR))
            XFREE (MTYPE_TMP, link_lsa->remote_addr.addr);
        }
    }
}

#endif /* HAVE_OSPF_CSPF */
#endif /* HAVE_OSPF_TE */
