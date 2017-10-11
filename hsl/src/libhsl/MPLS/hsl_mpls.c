/* Copyright (C) 2005  All Rights Reserved. */

#include "config.h"

#include "hsl_os.h"
#include "hsl_types.h"
#include "hsl_oss.h"
#include "hsl_table.h"
#include "hal_msg.h"
#include "hsl.h"
#include "hsl_ptree.h"
#include "hsl_ifmgr.h"
#include "hsl_ether.h"
#include "hsl_fib.h"
#include "hal_mpls_types.h"

/* #include "hal_mpls_types.h"  */
#include "hsl_mpls.h"
#include "bcm_incl.h"
#include "hsl_bcm_if.h"

static struct hsl_mpls_table *p_hsl_mpls_table = NULL;

/* Forward declarations */
void hsl_mpls_hw_cb_set_and_register ();
int hsl_mpls_vpn_vc_add (struct hsl_mpls_vpn_entry *ve, 
                         struct hsl_mpls_vpn_vc *vc);


struct hsl_if *
hsl_mpls_if_create (u_int32_t l3_ifindex, void *data)
{
  struct hsl_if *tmp_ifp = NULL;
  struct hsl_if *ifp = NULL;
  struct hsl_if *ifp2 = NULL;
  int ret;

  /* get l2/l3 interface */
  tmp_ifp = hsl_ifmgr_lookup_by_index (l3_ifindex);
  if (! tmp_ifp)
    {
      return NULL;
    }
  
  if (tmp_ifp->type != HSL_IF_TYPE_IP)
    {
      HSL_IFMGR_IF_REF_DEC (tmp_ifp);
      return NULL;
    }
  
  /* create mpls interface */
  ret = hsl_ifmgr_mpls_create (tmp_ifp->u.ip.mac, HSL_ETHER_ALEN, 
                               data, &ifp);
  if (ret < 0 || ifp == NULL)
    {
      HSL_IFMGR_IF_REF_DEC (tmp_ifp);
      return NULL;
    }
  
  ifp2 = hsl_ifmgr_get_first_L2_port (tmp_ifp);
  if (! ifp2)
    {
      HSL_IFMGR_IF_REF_DEC (tmp_ifp);
      return NULL;
    }
  
  HSL_IFMGR_IF_REF_DEC (tmp_ifp);
  tmp_ifp = NULL;
  
  /* bind mpls interface and layer 2 port */
  hsl_ifmgr_bind2 (ifp, ifp2);
  
  HSL_IFMGR_IF_REF_DEC (ifp2);
  
  /* Activate interface */
  if (ifp->flags & IFF_UP)
    {
      ret = hsl_bcm_if_mpls_up (ifp);
      if (ret < 0)
        return NULL;
    }
  
  return ifp;
}


void
hsl_mpls_if_delete (struct hsl_if *ifp)
{
  hsl_ifmgr_mpls_delete (ifp);
}


int
hsl_mpls_ftn_add (struct hal_msg_mpls_ftn_add *fa)
{
  int ret;
  hsl_prefix_t fec_prefix, nh_addr;
  u_int32_t nexthop;
  struct hsl_ptree_node *pn = NULL;
  struct hsl_if *ifp = NULL;
  struct hsl_nh_entry *nh_entry = NULL;

  if (fa->family != AF_INET)
    return -1;

  nexthop = fa->tunnel_nhop;

  /* get mpls interface */
  pn = hsl_ptree_node_get (p_hsl_mpls_table->mpls_if_table, (u_char *)&fa->nhlfe_ix,
                           HSL_MPLS_IF_KEY_LEN);
  if (! pn)
    return -1;

  if (! pn->info)
    {
      ifp = hsl_mpls_if_create (fa->tunnel_oif_ix, (void *)fa);
      if (! ifp)
        {
          goto err_ret;
        }

      memset (&nh_addr, 0, sizeof (hsl_prefix_t));
      nh_addr.family = AF_INET;
      nh_addr.prefixlen = 32;
      nh_addr.u.prefix4 = htonl (fa->tunnel_nhop);

      nh_entry = hsl_fib_nh_get (HSL_DEFAULT_FIB, ifp->ifindex, &nh_addr, HSL_NH_TYPE_MPLS);
      if (! nh_entry)
        {
          goto err_ret;
        }
    }
  else
    {
      nh_entry = pn->info;
    }

  memset (&fec_prefix, 0, sizeof (hsl_prefix_t));
  fec_prefix.family = AF_INET;
  fec_prefix.prefixlen = fa->fec_prefixlen;
  fec_prefix.u.prefix4 = htonl (fa->fec_addr);
  
  ret = hsl_fib_add_mpls_ftn (&fec_prefix, nh_entry);
  if (ret < 0)
    {
      goto err_ret;
    }
  
  if (pn->info == NULL)
    pn->info = nh_entry;
  else
    hsl_ptree_unlock_node (pn);

  return 0;

 err_ret:
  if (pn->info == NULL && nh_entry != NULL)
    hsl_fib_nh_release (nh_entry);
  
  if (ifp)
    hsl_mpls_if_delete (ifp);

  if (pn)
    hsl_ptree_unlock_node (pn);

  return -1;
}


int
hsl_mpls_ftn_del (struct hal_msg_mpls_ftn_del *fd)
{
  int ret;
  hsl_prefix_t fec_prefix;
  u_int32_t nexthop;
  struct hsl_ptree_node *pn = NULL;
  struct hsl_nh_entry *nh_entry;

  if (fd->family != AF_INET)
    return -1;

  nexthop = fd->tunnel_nhop;

  /* get mpls nexthop */
  pn = hsl_ptree_node_lookup (p_hsl_mpls_table->mpls_if_table, (u_char *)&fd->nhlfe_ix,
                              HSL_MPLS_IF_KEY_LEN);
  if (! pn)
    return -1;

  nh_entry = pn->info;
  hsl_ptree_unlock_node (pn);

  memset (&fec_prefix, 0, sizeof (hsl_prefix_t));
  fec_prefix.family = AF_INET;
  fec_prefix.prefixlen = fd->fec_prefixlen;
  fec_prefix.u.prefix4 = htonl (fd->fec_addr);

  ret = hsl_fib_delete_mpls_ftn (&fec_prefix, nh_entry);
  if (ret < 0)
    return ret;
  
  if (nh_entry->refcnt == 0 &&
      ! FLAG_ISSET (nh_entry->flags, HSL_NH_ENTRY_STATIC) &&
      FLAG_ISSET (nh_entry->flags, HSL_NH_ENTRY_DEPENDENT))
    {
      pn->info = NULL;
      hsl_ptree_unlock_node (pn);
      hsl_mpls_if_delete (nh_entry->ifp);
      nh_entry->ifp = NULL;
      hsl_fib_nh_release (nh_entry);
    }

  return 0;
}



/* Create an ILM key. */
void
hsl_mpls_ilm_key_create (u_int32_t in_ifindex, u_int32_t in_label, 
                         struct hsl_mpls_ilm_key *key)
{
  memset (key, 0, sizeof (struct hsl_mpls_ilm_key));
  key->in_ifindex = htonl (in_ifindex);
  HSL_PREP_FOR_NETWORK (key->in_label, in_label, 20);
}



void
hsl_mpls_link_ilm_and_nh (struct hsl_mpls_ilm_entry *ilm, 
                          struct hsl_nh_entry *nh)
{
  ilm->nh_entry = nh;
  ilm->nh_ilm_next = nh->ilm_list;
  nh->ilm_list = ilm;

  nh->refcnt++;
}

void
hsl_mpls_unlink_ilm_and_nh (struct hsl_mpls_ilm_entry *ilm,
                            struct hsl_nh_entry *nh)
{
  struct hsl_mpls_ilm_entry *ilm_tmp = NULL;
  struct hsl_ptree_node *pn = NULL;

  if (nh->ilm_list == ilm)
    {
      nh->ilm_list = ilm->nh_ilm_next;
      ilm_tmp = ilm;
    }
  else
    {
      for (ilm_tmp = nh->ilm_list; ilm_tmp; ilm_tmp = ilm_tmp->nh_ilm_next)
        {
          if (ilm_tmp->nh_ilm_next == ilm)
            {
              ilm_tmp->nh_ilm_next = ilm->nh_ilm_next;
              break;
            }
        }
    }

  if (ilm_tmp)
    {
      ilm->nh_ilm_next = NULL;
      ilm->nh_entry = NULL;
      nh->refcnt--;

      if (nh->refcnt == 0 &&
          ! FLAG_ISSET (nh->flags, HSL_NH_ENTRY_STATIC) &&
          FLAG_ISSET (nh->flags, HSL_NH_ENTRY_DEPENDENT))
        {
          if (nh->nh_type == HSL_NH_TYPE_MPLS)
            {
              pn = hsl_ptree_node_lookup (p_hsl_mpls_table->mpls_if_table, 
                              (u_char *)&nh->ifp->u.mpls.label_info->nhlfe_ix,
                              HSL_MPLS_IF_KEY_LEN);
            }

          if (pn)
            {
              if (nh->nh_type == HSL_NH_TYPE_MPLS)
                hsl_mpls_if_delete (nh->ifp);

              pn->info = NULL;
              hsl_ptree_unlock_node (pn);
              nh->ifp = NULL;
              hsl_fib_nh_release (nh);
            }
        }
    }
}

struct hsl_mpls_ilm_entry *
hsl_mpls_ilm_new (struct hal_msg_mpls_ilm_add *ia)
{
  struct hsl_mpls_ilm_entry *ilm;
  struct hsl_if *in_ifp = NULL;

  in_ifp = hsl_ifmgr_lookup_by_index (ia->in_ifindex);
  if (! in_ifp)
    return NULL;

  ilm = oss_malloc (sizeof (struct hsl_mpls_ilm_entry), OSS_MEM_HEAP);
  if (! ilm)
    {
      HSL_IFMGR_IF_REF_DEC (in_ifp);
      return NULL;
    }

  memset (ilm, 0, sizeof (struct hsl_mpls_ilm_entry));

  ilm->in_label = ia->in_label;
  ilm->in_ifp = in_ifp;
  ilm->opcode = ia->opcode;
  ilm->swap_label = ia->swap_label;
  ilm->tunnel_label = ia->tunnel_label;
  ilm->vpn_id = ia->vpn_id;
  
  HSL_IFMGR_IF_REF_DEC (in_ifp);
  
  return ilm;
}


void
hsl_mpls_ilm_free (struct hsl_mpls_ilm_entry *ilm)
{
  if (ilm->nh_entry)
    hsl_mpls_unlink_ilm_and_nh (ilm, ilm->nh_entry);

  oss_free (ilm, OSS_MEM_HEAP);
}



int
hsl_mpls_ilm_add (struct hal_msg_mpls_ilm_add *ia, u_int32_t vc_id,
                  u_int32_t vc_style)
{
  int ret;
  struct hsl_mpls_ilm_key key;
  struct hsl_nh_entry *nh = NULL;
  hsl_prefix_t p;
  struct hsl_ptree_node *pn = NULL;
  struct hsl_mpls_ilm_entry *ilm = NULL;

#ifdef HAVE_MPLS_VC
  struct hsl_mpls_vpn_entry *ve = NULL;
#endif /* HAVE_MPLS_VC */

  if (ia->family != AF_INET)
    return -1;

#ifdef HAVE_MPLS_VC
  if (vc_style != HAL_MPLS_VC_STYLE_NONE)
    {
      pn = hsl_ptree_node_lookup (p_hsl_mpls_table->mpls_vpn_table, 
                                  (u_char *)&ia->vpn_id, HSL_MPLS_VPN_KEY_LEN);
      if (! pn)
        return -1;
      
      ve = pn->info;
      hsl_ptree_unlock_node (pn);
    }
#endif /* HAVE_MPLS_VC */

  hsl_mpls_ilm_key_create (ia->in_ifindex, ia->in_label, &key);

  /* get ilm entry */
  pn = hsl_ptree_node_get (p_hsl_mpls_table->mpls_ilm_table, 
                           (u_char *)&key, HSL_MPLS_ILM_KEY_LEN);
  if (! pn)
    return -1;

  if (pn->info)
    {
      hsl_ptree_unlock_node (pn);
      return 0;
    }

  ilm = hsl_mpls_ilm_new (ia);
  if (! ilm)
    {
      goto err_ret;
    }
  
#ifdef HAVE_MPLS_VC
  if (ia->opcode != HAL_MPLS_POP_FOR_VC
#ifdef HAVE_VPLS
  && ia->opcode != HAL_MPLS_POP_FOR_VPLS
#endif /* HAVE_VPLS */
     )
#endif /* HAVE_MPLS_VC */
    {
      p.family = AF_INET;
      p.prefixlen = 32;
      p.u.prefix4 = htonl (ia->nexthop);
      
      nh = hsl_fib_nh_get (HSL_DEFAULT_FIB, ia->out_ifindex, &p, HSL_NH_TYPE_IP);
      if (! nh)
        {
          goto err_ret;
        }
      
      /* Link NH to ilm */
      hsl_mpls_link_ilm_and_nh (ilm, nh);
    }

#ifdef HAVE_MPLS_VC
  if (vc_style != HAL_MPLS_VC_STYLE_NONE)
    {
      struct hsl_mpls_vpn_vc *vc;

      if (ia->opcode == HAL_MPLS_POP_FOR_VC)
        {
          struct hsl_if *out_ifp;
          
          out_ifp = hsl_ifmgr_lookup_by_index (ia->out_ifindex);
          if (! out_ifp)
            {
              goto err_ret;
            }
          ilm->out_ifp = out_ifp;
          HSL_IFMGR_IF_REF_DEC (out_ifp);
        }
      vc = hsl_mpls_vpn_vc_lookup (ve, vc_id,
                                   ia->vc_peer, vc_style,
                                   HSL_FALSE);
      if (! vc)
        {
          vc = hsl_mpls_vpn_vc_create (ve, vc_id, ia->vc_peer,
                                       vc_style);
          if (! vc)
            goto err_ret;
          
          vc->vc_ilm = ilm;
          ilm->vc = vc;
          
          hsl_mpls_vpn_vc_add (ve, vc);
        }
  else
    {
            vc->vc_ilm = ilm;
            ilm->vc = vc;
    }
    }
#endif /* HAVE_MPLS_VC */

  /* Add it to hardware. */
  ret = hsl_mpls_ilm_add_to_hw (ilm, nh);
  if (ret < 0)
    {
      goto err_ret;
    }

  pn->info = ilm;
    
  return 0;

 err_ret:
  if (ilm)
    hsl_mpls_ilm_free (ilm);

  if (pn)
    hsl_ptree_unlock_node (pn);

  return -1;
}


static int
_hsl_mpls_ilm_del (u_int32_t in_ifindex, u_int32_t in_label)
{
  struct hsl_mpls_ilm_key key;
  struct hsl_ptree_node *pn = NULL;
  struct hsl_mpls_ilm_entry *ilm = NULL;

  hsl_mpls_ilm_key_create (in_ifindex, in_label, &key);

  /* get ilm entry */
  pn = hsl_ptree_node_lookup (p_hsl_mpls_table->mpls_ilm_table, 
                              (u_char *)&key, HSL_MPLS_ILM_KEY_LEN);
  if (! pn)
    return -1;

  ilm = pn->info;

  hsl_mpls_ilm_del_from_hw (ilm);

  if (ilm->nh_entry)
    {
      hsl_mpls_unlink_ilm_and_nh (ilm, ilm->nh_entry);
    }

  hsl_ptree_unlock_node (pn);
  pn->info = NULL;
  hsl_ptree_unlock_node (pn);
  
#ifdef HAVE_MPLS_VC 
  if (ilm->vc != NULL)
    {
          ilm->vc->vc_ilm = NULL;
          if (ilm->vc->vc_ftn_label == 0)
            {
              hsl_mpls_vpn_vc_lookup (ilm->vc->ve, ilm->vc->vc_id,
                                      ilm->vc->vc_peer_addr, ilm->vc->vc_type,
                                      HSL_TRUE);

              hsl_mpls_vpn_vc_free (ilm->vc);
            }

          ilm->vc = NULL;
    }
#endif /* HAVE_MPLS_VC */

  hsl_mpls_ilm_free (ilm);

  return 0;
}


int
hsl_mpls_ilm_del (struct hal_msg_mpls_ilm_del *id)
{
  if (id->family != AF_INET)
    return -1;

  return _hsl_mpls_ilm_del (id->in_ifindex, id->in_label);
}


int
hsl_mpls_ilm_add_to_hw (struct hsl_mpls_ilm_entry *ilm,
                        struct hsl_nh_entry *nh)
{
    int ret = 0;
    int add_to_hw = HSL_FALSE;

  /* Send it to the hardware. */
#ifdef HAVE_MPLS_VC
  if (ilm->opcode == HAL_MPLS_POP_FOR_VC)
    {
      if (ilm->vpn_id > 0 && ilm->out_ifp != NULL)
        add_to_hw = HSL_TRUE;
    }    
#ifdef HAVE_VPLS
    else if (ilm->opcode == HAL_MPLS_POP_FOR_VPLS)
      {
        if (ilm->vpn_id > 0)
          add_to_hw = HSL_TRUE;
      }
    else
#endif /* HAVE_VPLS */
#endif /* HAVE_MPLS_VC */
      {
        if (nh && FLAG_ISSET (nh->flags, HSL_NH_ENTRY_VALID))
          add_to_hw = HSL_TRUE;
      }
    
  if (add_to_hw == HSL_TRUE && 
      p_hsl_mpls_table->hw_cb && p_hsl_mpls_table->hw_cb->hw_mpls_ilm_add)
    {
      ret = (*p_hsl_mpls_table->hw_cb->hw_mpls_ilm_add) (ilm, nh);
      if (ret < 0)
        return ret;

      SET_FLAG (ilm->flags, HSL_MPLS_ILM_INSTALLED);
    }

  return 0;
}


int
hsl_mpls_ilm_del_from_hw (struct hsl_mpls_ilm_entry *ilm)
{
  if (FLAG_ISSET (ilm->flags, HSL_MPLS_ILM_INSTALLED))
    {
      if (p_hsl_mpls_table->hw_cb && p_hsl_mpls_table->hw_cb->hw_mpls_ilm_del)
        (*p_hsl_mpls_table->hw_cb->hw_mpls_ilm_del) (ilm);
      
      UNSET_FLAG (ilm->flags, HSL_MPLS_ILM_INSTALLED);
    }

  return 0;
}


int
hsl_mpls_ftn_add_to_hw (struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  int ret = 0;

  /* Send it to the hardware. */
  if (FLAG_ISSET (nh->flags, HSL_NH_ENTRY_VALID) &&
      p_hsl_mpls_table->hw_cb && p_hsl_mpls_table->hw_cb->hw_mpls_ftn_add)
    ret = (*p_hsl_mpls_table->hw_cb->hw_mpls_ftn_add) (rnp, nh);
  
  return ret;
}

int
hsl_mpls_ftn_del_from_hw (struct hsl_route_node *rnp, struct hsl_nh_entry *nh)
{
  struct hsl_prefix_entry *pe;

  pe = rnp->info;

  /* Delete from hardware, only if the NH is valid. */
  if (FLAG_ISSET (nh->flags, HSL_NH_ENTRY_VALID) &&
      p_hsl_mpls_table->hw_cb && p_hsl_mpls_table->hw_cb->hw_mpls_ftn_del)
    (*p_hsl_mpls_table->hw_cb->hw_mpls_ftn_del) (rnp, nh);

  return 0;
}



/*
  Register HW callbacks 
*/
int
hsl_mpls_hw_cb_register (struct hsl_mpls_hw_callbacks *cb)
{
  p_hsl_mpls_table->hw_cb = cb;
  return 0;
}

/*
  Unregister HW callbacks 
*/
int
hsl_mpls_hw_cb_unregister (void)
{
  p_hsl_mpls_table->hw_cb = NULL;
  return 0;
}



struct hsl_mpls_vpn_entry *
hsl_mpls_vpn_create (u_int32_t vpn_id, u_char vpn_type)
{
  struct hsl_mpls_vpn_entry *ve;

  ve = oss_malloc (sizeof (struct hsl_mpls_vpn_entry), OSS_MEM_HEAP);
  if (! ve)
    return NULL;

  memset (ve, 0, sizeof (struct hsl_mpls_vpn_entry));
  ve->vpn_id = vpn_id;
  ve->vpn_type = vpn_type;

  return ve;
}


void
hsl_mpls_vpn_free (struct hsl_mpls_vpn_entry *ve)
{
  struct hsl_mpls_vpn_port *vp, *vp_next;
#ifdef HAVE_MPLS_VC
  struct hsl_mpls_vpn_vc *vc, *vc_next;
#endif /* HAVE_MPLS_VC */

  for (vp = ve->port_list; vp; vp = vp_next)
    {
      vp_next = vp->next;
      
      if (p_hsl_mpls_table->hw_cb && p_hsl_mpls_table->hw_cb->hw_mpls_vpn_if_unbind)
        (*p_hsl_mpls_table->hw_cb->hw_mpls_vpn_if_unbind) (ve, vp);

      oss_free (vp, OSS_MEM_HEAP);
      vp = NULL;
    }

  ve->port_list = NULL;
  ve->port_count = 0;

#ifdef HAVE_MPLS_VC
  for (vc = ve->vc_list; vc; vc = vc_next)
    {
      vc_next = vc->vc_next;
      hsl_mpls_vpn_vc_free (vc);
      vc = NULL;
    }
  
  ve->vc_list = NULL;
  ve->vc_count = 0;
#endif /* HAVE_MPLS_VC */
  
  oss_free (ve, OSS_MEM_HEAP);
}

int
hsl_mpls_vpn_add (u_int32_t vpn_id, u_char vpn_type)
{
  int ret = -1;
  struct hsl_ptree_node *pn;
  struct hsl_mpls_vpn_entry *ve = NULL;

  pn = hsl_ptree_node_get (p_hsl_mpls_table->mpls_vpn_table, 
                           (u_char *)&vpn_id, HSL_MPLS_VPN_KEY_LEN);
  if (! pn)
    return -1;

  if (pn->info != NULL)
    {
      hsl_ptree_unlock_node (pn);
      return 0;
    }

  ve = hsl_mpls_vpn_create (vpn_id, vpn_type);
  if (! ve)
    {
      hsl_ptree_unlock_node (pn);
      return -1;
    }
  
  pn->info = ve;
  
  if (p_hsl_mpls_table->hw_cb && p_hsl_mpls_table->hw_cb->hw_mpls_vpn_add)
    ret = (*p_hsl_mpls_table->hw_cb->hw_mpls_vpn_add) (ve);
      
  if (ret < 0)
    {
      pn->info = NULL;
      hsl_ptree_unlock_node (pn);
      hsl_mpls_vpn_free (ve);

      return ret;
    }

  return 0;
}

int
hsl_mpls_vpn_del (u_int32_t vpn_id, u_char vpn_type)
{
  struct hsl_ptree_node *pn;
  struct hsl_mpls_vpn_entry *ve = NULL;

  pn = hsl_ptree_node_lookup (p_hsl_mpls_table->mpls_vpn_table, 
                              (u_char *)&vpn_id, HSL_MPLS_VPN_KEY_LEN);
  if (! pn)
    return 0;

  ve = pn->info;
  hsl_ptree_unlock_node (pn);
  
  if (p_hsl_mpls_table->hw_cb && p_hsl_mpls_table->hw_cb->hw_mpls_vpn_del)
    (*p_hsl_mpls_table->hw_cb->hw_mpls_vpn_del) (ve);

  hsl_mpls_vpn_free (ve);
  pn->info = NULL;
  hsl_ptree_unlock_node (pn);

  return 0;
}


int
hsl_mpls_vpn_port_add (struct hsl_mpls_vpn_entry *ve,   
                       struct hsl_mpls_vpn_port *vp)
{
  vp->next = ve->port_list;
  ve->port_list = vp;
  ve->port_count++;

  return 0;
}


struct hsl_mpls_vpn_port *
hsl_mpls_vpn_port_lookup (struct hsl_mpls_vpn_entry *ve,
                          u_int32_t ifindex, u_int16_t vlan_id,
                          int remove_port)
{
  struct hsl_mpls_vpn_port *vp, *vp_tmp;

  if (ve->port_list == NULL)
    return NULL;

  if (ve->port_list->port_id == ifindex && 
      ve->port_list->port_vlan_id == vlan_id)
    {
      vp = ve->port_list;
      
      if (remove_port)
        {
          ve->port_list = vp->next;
          vp->next = NULL;
          ve->port_count--;
        }

      return vp;
    }
  
  for (vp_tmp = ve->port_list; vp_tmp; vp_tmp = vp_tmp->next)
    {
      if (vp_tmp->next == NULL)
        break;

      if (vp_tmp->next->port_id == ifindex && 
          vp_tmp->next->port_vlan_id == vlan_id)
        {
          vp = vp_tmp->next;
          if (remove_port)
            {
              vp_tmp->next = vp->next;
              vp->next = NULL;
              ve->port_count--;
            }
          return vp;
        }
    }
  
  return NULL;
}



int 
hsl_mpls_vpn_if_bind (u_int32_t vpn_id, u_int32_t ifindex, 
                      u_int16_t vlan_id, u_char vpn_type)
{
  int ret = -1;
  struct hsl_ptree_node *pn;
  struct hsl_mpls_vpn_entry *ve = NULL;
  struct hsl_mpls_vpn_port *vp;

  pn = hsl_ptree_node_lookup (p_hsl_mpls_table->mpls_vpn_table, 
                              (u_char *)&vpn_id, HSL_MPLS_VPN_KEY_LEN);
  if (! pn)
    return -1;

  ve = pn->info;
  
  vp = oss_malloc (sizeof (struct hsl_mpls_vpn_port), OSS_MEM_HEAP);
  if (! vp)
    {
      hsl_ptree_unlock_node (pn);
      return -1;
    }

  vp->port_id = ifindex;
  vp->port_vlan_id = vlan_id;
  
  hsl_ptree_unlock_node (pn);

  if (p_hsl_mpls_table->hw_cb && p_hsl_mpls_table->hw_cb->hw_mpls_vpn_if_bind)
    ret = (*p_hsl_mpls_table->hw_cb->hw_mpls_vpn_if_bind) (ve, vp);

  if (ret < 0)
    {
      oss_free (vp, OSS_MEM_HEAP);
      return ret;
    }

  hsl_mpls_vpn_port_add (ve, vp);

  return 0;
}


int 
hsl_mpls_vpn_if_unbind (u_int32_t vpn_id, u_int32_t ifindex, 
                        u_int16_t vlan_id, u_char vpn_type)
{
  int ret = -1;
  struct hsl_ptree_node *pn;
  struct hsl_mpls_vpn_entry *ve = NULL;
  struct hsl_mpls_vpn_port *vp;

  pn = hsl_ptree_node_lookup (p_hsl_mpls_table->mpls_vpn_table, 
                              (u_char *)&vpn_id, HSL_MPLS_VPN_KEY_LEN);
  if (! pn)
    return -1;

  ve = pn->info;
  
  vp = hsl_mpls_vpn_port_lookup (ve, ifindex, vlan_id, HSL_FALSE);
  if (! vp)
    {
      hsl_ptree_unlock_node (pn);
      return -1;
    }

  hsl_ptree_unlock_node (pn);

  if (p_hsl_mpls_table->hw_cb && p_hsl_mpls_table->hw_cb->hw_mpls_vpn_if_unbind)
    ret = (*p_hsl_mpls_table->hw_cb->hw_mpls_vpn_if_unbind) (ve, vp);

  if (ret < 0)
    return ret;

  hsl_mpls_vpn_port_lookup (ve, ifindex, vlan_id, HSL_TRUE);

  oss_free (vp, OSS_MEM_HEAP);

  return 0;
}



#ifdef HAVE_MPLS_VC
int
hsl_mpls_vpn_vc_ftn_add_to_hw (struct hsl_mpls_vpn_vc *vc,
                           struct hsl_nh_entry *nh)
{
  int ret = 0;

  if (FLAG_ISSET (nh->flags, HSL_NH_ENTRY_VALID) &&
      p_hsl_mpls_table->hw_cb && 
      p_hsl_mpls_table->hw_cb->hw_mpls_vpn_vc_ftn_add)
    ret = (*p_hsl_mpls_table->hw_cb->hw_mpls_vpn_vc_ftn_add) (vc, nh);

  return ret;
}

int
hsl_mpls_vpn_vc_ftn_del_from_hw (struct hsl_mpls_vpn_vc *vc)
{
  int ret = -1;

  if (FLAG_ISSET (vc->vc_nh_entry->flags, HSL_NH_ENTRY_VALID) &&
      p_hsl_mpls_table->hw_cb && 
      p_hsl_mpls_table->hw_cb->hw_mpls_vpn_vc_ftn_del)
    ret = (*p_hsl_mpls_table->hw_cb->hw_mpls_vpn_vc_ftn_del) (vc);

  return ret;
}


/* Link NH to vc ftn */
void
hsl_mpls_link_vpn_vc_and_nh (struct hsl_mpls_vpn_vc *vc,
                             struct hsl_nh_entry *nh)
{
  vc->vc_nh_entry = nh;
  vc->nh_vc_next = nh->vpn_vc_list;
  nh->vpn_vc_list = vc;

  nh->refcnt++;
}


void
hsl_mpls_unlink_vpn_vc_and_nh (struct hsl_mpls_vpn_vc *vc,
                               struct hsl_nh_entry *nh)
{
  struct hsl_mpls_vpn_vc *vc_tmp = NULL;

  if (nh->vpn_vc_list == vc)
    {
      nh->vpn_vc_list = vc->nh_vc_next;
      vc_tmp = vc;
    }
  else
    {
      for (vc_tmp = nh->vpn_vc_list; vc_tmp; vc_tmp = vc_tmp->nh_vc_next)
        {
          if (vc_tmp->nh_vc_next == vc)
            {
              vc_tmp->nh_vc_next = vc->nh_vc_next;
              vc_tmp = vc;
              break;
            }
        }
    }

  if (vc_tmp)
    {
      vc_tmp->nh_vc_next = NULL;
      vc_tmp->vc_nh_entry = NULL;
      nh->refcnt--;
    }
}

struct hsl_mpls_vpn_vc *
hsl_mpls_vpn_vc_create (struct hsl_mpls_vpn_entry *ve,
                        u_int32_t vc_id, u_int32_t vc_peer,
                        u_char vc_type)
{
  struct hsl_mpls_vpn_vc *vc;

  vc = oss_malloc (sizeof (struct hsl_mpls_vpn_vc), OSS_MEM_HEAP);
  if (! vc)
    return NULL;

  memset (vc, 0, sizeof (struct hsl_mpls_vpn_vc));
  vc->vc_type = vc_type;
  vc->vc_id = vc_id;
  vc->vc_peer_addr = vc_peer;
  vc->ve = ve;

  return vc;
}


void
hsl_mpls_vpn_vc_free (struct hsl_mpls_vpn_vc *vc)
{
  struct hsl_nh_entry *nh;
  struct hsl_ptree_node *pn = NULL;

  if (vc->vc_nh_entry)
   {
     nh = vc->vc_nh_entry;

     hsl_mpls_unlink_vpn_vc_and_nh (vc, nh);

     if (nh->refcnt == 0 &&
         ! FLAG_ISSET (nh->flags, HSL_NH_ENTRY_STATIC) &&
         FLAG_ISSET (nh->flags, HSL_NH_ENTRY_DEPENDENT))
       {
         if (nh->nh_type == HSL_NH_TYPE_MPLS) 
           {
             pn = hsl_ptree_node_lookup (p_hsl_mpls_table->mpls_if_table, 
                              (u_char *)&nh->ifp->u.mpls.label_info->nhlfe_ix,
                              HSL_MPLS_IF_KEY_LEN);
           }

          if (pn)
            {
              if (nh->nh_type == HSL_NH_TYPE_MPLS) 
                hsl_mpls_if_delete (nh->ifp);

              pn->info = NULL;
              hsl_ptree_unlock_node (pn);

              nh->ifp = NULL;
              hsl_fib_nh_release (nh);
            }
       }
     
     vc->vc_nh_entry = NULL;
   }
  
  if (vc->vc_ilm)
    {
      _hsl_mpls_ilm_del (vc->vc_ilm->in_ifp->ifindex, vc->vc_ilm->in_label);
      vc->vc_ilm = NULL;  
    }

  oss_free (vc, OSS_MEM_HEAP);
}


int
hsl_mpls_vpn_vc_add (struct hsl_mpls_vpn_entry *ve, 
                     struct hsl_mpls_vpn_vc *vc)
{
  vc->vc_next = ve->vc_list;
  ve->vc_list = vc;
  ve->vc_count++;

  return 0;
  
}


struct hsl_mpls_vpn_vc *
hsl_mpls_vpn_vc_lookup (struct hsl_mpls_vpn_entry *ve, u_int32_t vc_id,
                        u_int32_t vc_peer, u_char vc_type, int remove_vc)
{
  struct hsl_mpls_vpn_vc *vc, *vc_tmp;

  if (ve->vc_list == NULL)
    return NULL;

  if (ve->vc_list->vc_id == vc_id && 
      ve->vc_list->vc_peer_addr == vc_peer &&
      ve->vc_list->vc_type == vc_type)
    {
      vc = ve->vc_list;
      
      if (remove_vc)
        {
          ve->vc_list = vc->vc_next;
          vc->vc_next = NULL;
          ve->vc_count--;
        }

      return vc;
    }
  
  for (vc_tmp = ve->vc_list; vc_tmp; vc_tmp = vc_tmp->vc_next)
    {
      if (vc_tmp->vc_next == NULL)
        break;

      if (vc_tmp->vc_next->vc_id == vc_id &&
          vc_tmp->vc_next->vc_peer_addr == vc_peer &&
          vc_tmp->vc_next->vc_type == vc_type)
        {
          vc = vc_tmp->vc_next;
          if (remove_vc)
            {
              vc_tmp->vc_next = vc->vc_next;
              vc->vc_next = NULL;
              ve->vc_count--;
            }
          return vc;
        }
    }
  
  return NULL;
}

int 
hsl_mpls_vc_ftn_add (struct hal_msg_mpls_vc_fib_add *vfa)
{
  struct hsl_ptree_node *pn;
  struct hsl_nh_entry *nh;
  hsl_prefix_t p;
  struct hsl_mpls_vpn_entry *ve = NULL;
  struct hsl_mpls_vpn_vc *vc;

  pn = hsl_ptree_node_lookup (p_hsl_mpls_table->mpls_vpn_table, 
                              (u_char *)&vfa->vpls_id, HSL_MPLS_VPN_KEY_LEN);
  if (! pn)
    return -1;

  ve = pn->info;

  hsl_ptree_unlock_node (pn);

  vc = hsl_mpls_vpn_vc_lookup (ve, vfa->vc_id, vfa->vc_peer, 
                               vfa->vc_style, HSL_FALSE);
  if (vc && vc->vc_ftn_label)
    return 0;

  if (! vc)
    {
      vc = hsl_mpls_vpn_vc_create (ve, vfa->vc_id, vfa->vc_peer,
                                   vfa->vc_style);
      if (! vc)
        return -1;
    }

  vc->vc_peer_nhop = vfa->vc_nhop;
  vc->vc_ftn_label = vfa->out_label;

  if (vfa->opcode == HAL_MPLS_PUSH_AND_LOOKUP_FOR_VC && 
      vfa->tunnel_label != HSL_MPLS_LABEL_VALUE_INVALID)
    {
      pn = hsl_ptree_node_lookup (p_hsl_mpls_table->mpls_if_table, 
                                  (u_char *)&vfa->tunnel_nhlfe_ix,
                                  HSL_MPLS_IF_KEY_LEN);
      if (! pn)
        {
          hsl_mpls_vpn_vc_free (vc);
          return -1;
        }
      
      nh = pn->info;
      hsl_ptree_unlock_node (pn);
    }
  else
    {
      p.family = AF_INET;
      p.prefixlen = 32;
      p.u.prefix4 = htonl (vfa->vc_nhop);
      
      nh = hsl_fib_nh_get (HSL_DEFAULT_FIB, vfa->nw_ifindex, &p, HSL_NH_TYPE_IP);
      if (! nh)
        return -1;
    }

  /* Link NH to vc ftn */
  hsl_mpls_link_vpn_vc_and_nh (vc, nh);

  /* Add it to hardware. */
  hsl_mpls_vpn_vc_ftn_add_to_hw (vc, nh);

  hsl_mpls_vpn_vc_add (ve, vc);

  return 0;
}


int
hsl_mpls_vc_ftn_del (struct hal_msg_mpls_vc_fib_del *vfd)
{
  struct hsl_ptree_node *pn;
  struct hsl_mpls_vpn_entry *ve = NULL;
  struct hsl_mpls_vpn_vc *vc;

  pn = hsl_ptree_node_lookup (p_hsl_mpls_table->mpls_vpn_table, 
                              (u_char *)&vfd->vpls_id, HSL_MPLS_VPN_KEY_LEN);
  if (! pn)
    return 0;

  ve = pn->info;
  hsl_ptree_unlock_node (pn);

  vc = hsl_mpls_vpn_vc_lookup (ve, vfd->vc_id, vfd->vc_peer, 
                               vfd->vc_style, HSL_FALSE);
  if (! vc)
    return 0;

  hsl_mpls_vpn_vc_ftn_del_from_hw (vc);

  if (vc->vc_ilm == NULL)
    {
      hsl_mpls_vpn_vc_lookup (ve, vfd->vc_id, vfd->vc_peer, vfd->vc_style, HSL_TRUE);
      hsl_mpls_vpn_vc_free (vc);
    }

  return 0;
}





int
hsl_mpls_vc_ilm_add (struct hal_msg_mpls_vc_fib_add *vfib_msg)
{
  struct hal_msg_mpls_ilm_add ia;
  int ret;

  memset (&ia, 0, sizeof (struct hal_msg_mpls_ilm_add));
  ia.family = AF_INET;
  ia.in_label = vfib_msg->in_label;
  ia.in_ifindex = vfib_msg->nw_ifindex;
  strcpy (ia.in_ifname, "");
  if (vfib_msg->vc_style == HAL_MPLS_VC_STYLE_MARTINI)
    {
      ia.opcode = HAL_MPLS_POP_FOR_VC;
      ia.out_ifindex = vfib_msg->ac_ifindex;
    }
  else
    {
      ia.opcode = HAL_MPLS_POP_FOR_VPLS;
    }
  strcpy (ia.out_ifname, "");
  ia.swap_label = HSL_MPLS_LABEL_VALUE_INVALID;
  ia.tunnel_label = HSL_MPLS_LABEL_VALUE_INVALID;
  ia.vpn_id = vfib_msg->vpls_id;

  ret = hsl_mpls_ilm_add (&ia, ia.vpn_id, vfib_msg->vc_style);
  if (ret < 0)
    return ret;

  return 0;
}

int
hsl_mpls_vpls_ilm_del (u_int32_t in_ifindex, u_int32_t in_label)
{
  struct hal_msg_mpls_ilm_del id;
  int ret;

  memset (&id, 0, sizeof (struct hal_msg_mpls_ilm_del));
  id.family = AF_INET;
  id.in_label = in_label;
  id.in_ifindex = in_ifindex;

  ret = hsl_mpls_ilm_del (&id);
  if (ret < 0)
    return ret;

  return 0;
}





int 
hsl_mpls_vc_fib_add (struct hal_msg_mpls_vc_fib_add *vfib_msg)
{
  int ret;

  ret = hsl_mpls_vc_ilm_add (vfib_msg);
  if (ret < 0)
    return ret;

  ret = hsl_mpls_vc_ftn_add (vfib_msg);
  if (ret < 0)
    return ret;
  
  return 0;
}



int 
hsl_mpls_vc_fib_del (struct hal_msg_mpls_vc_fib_del *vfib_msg)
{
  hsl_mpls_vpls_ilm_del (vfib_msg->nw_ifindex, vfib_msg->in_label);
  hsl_mpls_vc_ftn_del (vfib_msg);
  return 0;
}
#endif /* HAVE_MPLS_VC */


int
hsl_mpls_init ()
{
  int ret;

  if (p_hsl_mpls_table)
    return 0;

  p_hsl_mpls_table = oss_malloc (sizeof (struct hsl_mpls_table), OSS_MEM_HEAP);
  if (! p_hsl_mpls_table)
    goto err_ret;

  p_hsl_mpls_table->mpls_if_table = hsl_ptree_init (HSL_MPLS_IF_KEY_LEN);
  if (! p_hsl_mpls_table->mpls_if_table)
    goto err_ret;

  p_hsl_mpls_table->mpls_ilm_table = hsl_ptree_init (HSL_MPLS_ILM_KEY_LEN);
  if (! p_hsl_mpls_table->mpls_ilm_table)
    goto err_ret;

  p_hsl_mpls_table->mpls_vpn_table = hsl_ptree_init (HSL_MPLS_VPN_KEY_LEN);
  if (! p_hsl_mpls_table->mpls_vpn_table)
    goto err_ret;

  hsl_mpls_hw_cb_set_and_register ();

  ret = hsl_hw_mpls_init ();  
  if (ret < 0)
    goto err_ret;

  return 0;
  
 err_ret:
  hsl_mpls_deinit ();
  return -1;
}


int 
hsl_mpls_deinit ()
{
  if (! p_hsl_mpls_table)
    return 0;
  
  if (p_hsl_mpls_table->mpls_ilm_table)
    {
      hsl_ptree_finish (p_hsl_mpls_table->mpls_ilm_table);
      p_hsl_mpls_table->mpls_ilm_table = NULL;
    }

  if (p_hsl_mpls_table->mpls_vpn_table)
    {
      hsl_ptree_finish (p_hsl_mpls_table->mpls_vpn_table);
      p_hsl_mpls_table->mpls_vpn_table = NULL;
    }

  if (p_hsl_mpls_table->mpls_if_table)
    {
      hsl_ptree_finish (p_hsl_mpls_table->mpls_if_table);
      p_hsl_mpls_table->mpls_if_table = NULL;
    }

  hsl_mpls_hw_cb_unregister ();

  if (p_hsl_mpls_table)
    oss_free (p_hsl_mpls_table, OSS_MEM_HEAP);

  hsl_hw_mpls_deinit ();

  return 0;
}
