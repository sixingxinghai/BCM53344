/* Copyright (C) 2002-2003  All Rights Reserved. */
#include "pal.h"

#include "prefix.h"
#include "ptree.h"
#include "avl_tree.h"
#include "table.h"
#include "log.h"
#include "bitmap.h"
#include "thread.h"
#include "label_pool.h"
#include "mpls_common.h"
#include "mpls.h"
#include "mpls_client.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */

#include "nsmd.h"
#include "nsm_router.h"
#include "rib/rib.h"
#include "nsm_debug.h"
#ifdef HAVE_DSTE
#include "nsm_dste.h"
#endif /* HAVE_DSTE */
#include "nsm_mpls.h"
#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
int nsm_is_vlan_exist (struct interface *, u_int16_t);
#endif /* HAVE_MPLS_VC */
#include "nsm_mpls_dep.h"
#include "nsm_mpls_rib.h"
#include "nsm_mpls_api.h"
#include "nsm_mpls_fwd.h"
#include "nsm_message.h"
#include "nsm_server.h"
#ifdef HAVE_SNMP 
#include "nsm_mpls_snmp.h"
#include "nsm_mpls_vc_snmp.h"
#include "nsm_mpls_vc_api.h"
#endif /* HAVE_SNMP */
#include "nsm_interface.h"

#ifdef HAVE_VPLS
#include "nsm_vpls.h"
#endif /* HAVE_VPLS */

#ifdef HAVE_MS_PW
#include "nsm_mpls_ms_pw.h"
#endif /* HAVE_MS_PW */

/* Macro version of check_bit (). */
#define CHECK_BIT(X,P) ((((u_char *)(X))[(P) / 8]) >> (7 - ((P) % 8)) & 1)

extern void nsm_restart_stop (int proto_id);
extern pal_time_t nsm_get_sys_up_time ();

#if defined HAVE_GMPLS && defined HAVE_PBB_TE
#ifdef HAVE_GELS
#if defined HAVE_I_BEB && defined HAVE_B_BEB
extern int
nsm_pbb_te_create_dynamic_esp(struct interface *ifp, u_int32_t tesid,
                      u_char *bmac, u_int16_t bvid, bool_t egress);

extern int
nsm_pbb_te_delete_dynamic_esp(struct interface *ifp, u_int32_t tesid,
                      u_char *bmac, u_int16_t bvid, bool_t egress);

#endif /* HAVE_I_BEB && HAVE_B_BEB */

extern int
nsm_pbb_te_bcb_action_create(struct interface *ifp,
                             u_char *bmac, u_int16_t bvid);
extern int
nsm_pbb_te_bcb_action_delete(struct interface *ifp,
                             u_char *bmac, u_int16_t bvid);
#endif /* HAVE_GELS */
#endif /* HAVE_GMPLS && HAVE_PBB_TE */

void
nsm_gmpls_owner_afi_set (struct mpls_owner *owner, afi_t afi)
{
  switch (owner->owner)
    {
    case MPLS_RSVP:
    case MPLS_IGP_SHORTCUT:
      owner->u.r_key.afi = afi;
      break;
    case MPLS_LDP:
      owner->u.l_key.afi = afi;
      break;
    case MPLS_OTHER_BGP:
      owner->u.b_key.afi = afi;
      break;
    default:
      break;
    }
}

s_int32_t
gmpls_set_dep_ilm_nhprefix (struct prefix *p, struct ilm_entry *ilm)
{
  struct nhlfe_key *nkey;

  pal_mem_set (p, 0, sizeof (struct prefix));

  if (! ilm || ! ilm->xc || ! ilm->xc->nhlfe)
    return -1;

  nkey = (struct nhlfe_key *) ilm->xc->nhlfe->nkey;

#ifdef HAVE_IPV6
  if (nkey->afi == AF_INET6)
    {
      p->family = AF_INET6;
      p->u.prefix6 = nkey->u.ipv6.nh_addr;
      p->prefixlen = IPV6_MAX_PREFIXLEN;
    }
  else
#endif /* HAVE_IPV6 */
    {
#ifdef HAVE_GMPLS
      if (nkey->afi == AF_UNNUMBERED)
        {
          p->family = AF_UNNUMBERED;
          p->prefixlen = IPV4_MAX_PREFIXLEN;
          pal_mem_cpy (&p->u.prefix4, &nkey->u.unnum, 
                       sizeof (struct pal_in4_addr));
        }
      else
#endif /* HAVE_GMPLS */
        {
          p->family = AF_INET;
          p->prefixlen = IPV4_MAX_PREFIXLEN;
          p->u.prefix4 = nkey->u.ipv4.nh_addr;
        }
    }

  return 1;
}

void 
gmpls_ftn_del_from_fwd (struct nsm_master *nm,
                       struct ftn_entry *ftn, struct fec_gen_entry *fec,
                       u_int32_t vrf_id, struct ftn_entry *tunnel_ftn)
{
  s_int32_t ret;

  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED))
    {
      switch (fec->type)
        {
          case gmpls_entry_type_ip:
#ifdef HAVE_IPV6
            if (fec->u.prefix.family == AF_INET6)
              ret = nsm_mpls_ftn6_del_from_fwd (nm, ftn, &fec->u.prefix, 
                                                vrf_id, tunnel_ftn);
            else
#endif
              ret = nsm_mpls_ftn4_del_from_fwd (nm, ftn, &fec->u.prefix, 
                                                vrf_id, tunnel_ftn);
            break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
          case gmpls_entry_type_pbb_te:
            return;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
          case gmpls_entry_type_tdm:
            return;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
           default:
             break;
        }

      UNSET_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED);
      FTN_XC_OPR_STATUS_COUNT_DEC (ftn);
      FTN_XC_STATUS_UPDATE (ftn);
    }
}

/* Create new ftn entry */
static struct ftn_entry *
gmpls_ftn_new (struct ftn_add_gen_data *data, mpls_ftn_type_t f_type, 
               s_int32_t *ret_code)
{
  struct ftn_entry *ftn;
  
  *ret_code = NSM_SUCCESS;

  NSM_ASSERT (data != NULL);
  if (! data)
    {
      *ret_code = NSM_ERR_INVALID_ARGS; 
      return NULL;
    }

  /* allocate memory */
  ftn = XCALLOC (MTYPE_MPLS_FTN_ENTRY, sizeof (struct ftn_entry));
  if (! ftn)
    {
      *ret_code = NSM_ERR_MEM_ALLOC_FAILURE;
      return NULL;
    }

  pal_mem_set (ftn, 0, sizeof (struct ftn_entry));
  pal_mem_cpy (&ftn->owner, &data->owner, sizeof (struct mpls_owner));
  ftn->protocol = gmpls_owner_to_proto (data->owner.owner);
  ftn->lsp_bits.bits.lsp_type = data->lsp_bits.bits.lsp_type;
  ftn->lsp_bits.bits.act_type = data->lsp_bits.bits.act_type;
  ftn->lsp_bits.bits.lsp_metric = data->lsp_bits.bits.lsp_metric;
  ftn->lsp_bits.bits.igp_shortcut = data->lsp_bits.bits.igp_shortcut;
  ftn->exp_bits = data->exp_bits;
  ftn->dscp_in = data->dscp_in;
  ftn->tun_id = data->tunnel_id;
  ftn->protected_lsp_id = data->protected_lsp_id;
  ftn->qos_resrc_id = data->qos_resrc_id;
  ftn->bypass_ftn_ix = data->bypass_ftn_ix;
#ifdef HAVE_MPLS_OAM
  ftn->is_oam_enabled = PAL_FALSE;
#endif /* HAVE_MPLS_OAM */

#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
  ftn->bfd_flag = PAL_FALSE;
#endif /* HAVE_BFD && HAVE_MPLS_OAM */

  if (data->ftn.type == gmpls_entry_type_ip)
    {
      if (data->ftn.u.prefix.family == AF_INET)
        ftn->ent_type = FTN_ENTRY_TYPE_IPV4;
#ifdef HAVE_IPV6
      else
        ftn->ent_type = FTN_ENTRY_TYPE_IPV6;
#endif /* HAVE_IPV6 */
    }

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  else if (data->ftn.type == gmpls_entry_type_pbb_te)
    ftn->ent_type = FTN_ENTRY_TYPE_PBB_TE;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
  else if (data->ftn.type == gmpls_entry_type_tdm)
    ftn->ent_type = FTN_ENTRY_TYPE_TDM;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
  
  if (data->sz_desc)
    ftn->sz_desc = XSTRDUP (MTYPE_TMP, data->sz_desc);
  if (data->pri_fec_prefix)
    {
      ftn->pri_fec_prefix = prefix_new ();
      prefix_copy (ftn->pri_fec_prefix, data->pri_fec_prefix);
    }

  /* GELS : Add the Tech specific data to ftn entry */
  if (data->tgen_data)
  {
    switch (data->tgen_data->gmpls_type)
    {
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        ftn->tgen_data = (struct gmpls_tgen_data *) XCALLOC (MTYPE_TMP, 
                                     sizeof(struct gmpls_tgen_data));
        ((struct gmpls_tgen_data *)ftn->tgen_data)->gmpls_type = 
         gmpls_entry_type_pbb_te;
        ((struct gmpls_tgen_data *)ftn->tgen_data)->u.pbb.tesid = 
         data->tgen_data->u.pbb.tesid;
        break;
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
      default:
         break;
    }
  }

  ftn->row_status = RS_NOT_READY;
  ftn->ftn_type = f_type;
  if (data->lsp_bits.bits.lsp_type == LSP_TYPE_PRIMARY || 
      data->lsp_bits.bits.lsp_type == LSP_TYPE_BYPASS)
    SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_PRIMARY);

  if (CHECK_FLAG (data->flags, NSM_MSG_GEN_FTN_BIDIR))
    {
      SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_BIDIR);
      if (CHECK_FLAG (data->flags, NSM_MSG_GEN_FTN_FWD))
        {
          SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_FWD);
        }
    }

  if (!CHECK_FLAG (data->flags, NSM_MSG_GEN_FTN_MPLS))
    SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_GMPLS);
#ifdef HAVE_DIFFSERV
  pal_mem_cpy (&ftn->ds_info, &data->ds_info, sizeof (struct ds_info));
#endif /* HAVE_DIFFSERV */

#ifdef HAVE_MPLS_FWD
  pal_mem_set (&ftn->stats, 0, sizeof (pal_ftn_entry_stats_t));
#endif /* HAVE_MPLS_FWD */

  return ftn;
}


/* Free FTN Entry */
void
gmpls_ftn_free (struct ftn_entry *ftn)
{
  if (ftn)
    {
      if (ftn->sz_desc)
        XFREE (MTYPE_TMP, ftn->sz_desc);
      if (ftn->pri_fec_prefix)
        prefix_free (ftn->pri_fec_prefix);

      if(ftn->tgen_data)
        XFREE (MTYPE_TMP, ftn->tgen_data);

      XFREE (MTYPE_MPLS_FTN_ENTRY, ftn);
    }
}

#ifdef HAVE_RESTART
s_int32_t
gmpls_ilm_del_partial_info (struct ilm_entry * ilm)
{
  if (ilm && CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_OUT_LBL_NOT_REFRESHED)
      && ilm->partial)
    {
      UNSET_FLAG (ilm->flags, ILM_ENTRY_FLAG_OUT_LBL_NOT_REFRESHED);
      XFREE (MTYPE_MPLS_ILM_ENTRY, ilm->partial);
      ilm->partial = NULL;
    }

  return NSM_SUCCESS;

}
s_int32_t
gmpls_ilm_update_partial_info (struct ilm_add_gen_data *data, 
                               struct ilm_entry *ilm)
{
  NSM_ASSERT ((ilm != NULL) && (data != NULL));

  if ((! ilm) || (! data))
    return NSM_ERR_INVALID_ARGS;

  ilm->partial = XCALLOC (MTYPE_MPLS_ILM_ENTRY,
                          sizeof (struct ilm_add_gen_data));

  if (!ilm->partial)
    return -2;
   
  pal_mem_cpy (ilm->partial, data, sizeof(struct ilm_add_gen_data));

  return NSM_SUCCESS;
}
#endif /* HAVE_RESTART */

/* Create an ILM entry */
static struct ilm_entry *
gmpls_ilm_new (struct ilm_add_gen_data *data, bool_t mapped_ilm, 
              s_int32_t *ret_code, bool_t config)
{
  struct ilm_entry *ilm;

  *ret_code = NSM_SUCCESS;

  NSM_ASSERT (data != NULL);
  if (! data)
    {
      *ret_code = NSM_ERR_INVALID_ARGS;
      return NULL;
    }
 
#ifdef HAVE_IPV6
  if (data->nh_addr.afi == AFI_IP6) 
    ilm = XCALLOC (MTYPE_MPLS_ILM_ENTRY, sizeof (struct ilm_entry) + 
                   sizeof (struct pal_in6_addr));
  else
#endif
    ilm = XCALLOC (MTYPE_MPLS_ILM_ENTRY, sizeof (struct ilm_entry) +
                   sizeof (struct pal_in4_addr));

  if (! ilm)
    {
      *ret_code = NSM_ERR_MEM_ALLOC_FAILURE;
      return NULL;
     }

  pal_mem_cpy (&ilm->owner, &data->owner, sizeof (struct mpls_owner));
  
  switch (data->in_label.type)
    {
      case gmpls_entry_type_ip:
        ilm->key.u.pkt.iif_ix = data->iif_ix;
        ilm->key.u.pkt.in_label = data->in_label.u.pkt;
        ilm->ent_type = ILM_ENTRY_TYPE_PACKET;
        break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        ilm->key.u.pbb_key.iif_ix = data->iif_ix;
        ilm->key.u.pbb_key.in_label = data->in_label.u.pbb;
        ilm->ent_type = ILM_ENTRY_TYPE_PBB_TE;
        break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        ilm->key.u.tdm_key.iif_ix = data->iif_ix;
        ilm->key.u.tdm_key.in_label = data->in_label.u.tdm;
        ilm->ent_type = ILM_ENTRY_TYPE_TDM;
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
        break;
    }

  if (data->primary)
    SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_PRIMARY);
  
  if (CHECK_FLAG (data->flags, NSM_MSG_ILM_BYPASS))
    SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_DEPENDENT);

  if (CHECK_FLAG (data->flags, NSM_MSG_ILM_GEN_MPLS))
    SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_GMPLS);

  if (CHECK_FLAG (data->flags, NSM_MSG_ILM_BIDIR))
    SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_BIDIR);

  if (CHECK_FLAG (data->flags, NSM_MSG_GEN_ILM_FWD))
    SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_FWD);

  if (config == NSM_TRUE)
    SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_STATIC);

  ilm->n_pops = data->n_pops;

  ilm->family = data->fec_prefix.family;
  ilm->prefixlen = data->fec_prefix.prefixlen;

#ifdef HAVE_IPV6
  if (data->fec_prefix.family == AF_INET6) 
    pal_mem_cpy (ilm->prfx, &data->fec_prefix.u.prefix6, 
                 sizeof (struct pal_in6_addr));
  else
#endif
    pal_mem_cpy (ilm->prfx, &data->fec_prefix.u.prefix4, 
                 sizeof (struct pal_in4_addr));

  ilm->qos_resrc_id = data->qos_resrc_id;
  if (mapped_ilm)
    SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_MAPPED);
  ilm->row_status = RS_NOT_READY;

#ifdef HAVE_DIFFSERV
  pal_mem_cpy (&ilm->ds_info, &data->ds_info, sizeof (struct ds_info));
#endif /* HAVE_DIFFSERV */

#ifdef HAVE_MPLS_FWD
  pal_mem_set (&ilm->stats, 0, sizeof (pal_ilm_entry_stats_t));
#endif /* HAVE_MPLS_FWD */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  /* GELS : Add the Tech specific data to ilm entry */
  if (data->tgen_data)
  {
    switch (data->tgen_data->gmpls_type)
    {
      case gmpls_entry_type_pbb_te:
        ilm->tgen_data = (struct gmpls_tgen_data *) XCALLOC (MTYPE_TMP, 
                          sizeof(struct gmpls_tgen_data));
        ((struct gmpls_tgen_data *)ilm->tgen_data)->u.pbb.tesid = 
                          data->tgen_data->u.pbb.tesid;
        break;

      default:
         break;
    }
  }
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
 
  return ilm;
}

/* Free ILM entry */
void
gmpls_ilm_free (struct ilm_entry *ilm)
{
  NSM_ASSERT (ilm != NULL);
  if (! ilm)
    return;

  if(ilm->tgen_data)
    XFREE (MTYPE_TMP, ilm->tgen_data);

  XFREE (MTYPE_MPLS_ILM_ENTRY, ilm);
}

/* Create XC Entry */
static struct xc_entry *
gmpls_xc_new (struct xc_new_data *data, s_int32_t *ret_code)
{
  struct xc_entry *xc;

  *ret_code = NSM_SUCCESS;
  
  NSM_ASSERT (data != NULL);
  if (! data)
    {
      *ret_code = NSM_ERR_INVALID_ARGS;
      return NULL;
    }
  
  xc = XMALLOC (MTYPE_MPLS_XC_ENTRY, sizeof (struct xc_entry));
  if (! xc)
    {
      *ret_code = NSM_ERR_MEM_ALLOC_FAILURE;
      return NULL;
    }

  pal_mem_set (xc, 0, sizeof (struct xc_entry));
  xc->key.iif_ix = data->iif_ix;
  xc->key.in_label = data->in_label;
  pal_mem_cpy (&xc->lspid, &data->lspid, sizeof (struct mpls_lspid));
  xc->owner = data->owner;
  xc->persistent = NSM_FALSE;
  xc->row_status = RS_NOT_READY;
  xc->admn_status = ADMN_DOWN;
  xc->opr_status = OPR_NOT_PRESENT;
  xc->type = data->in_label.type;
 
  if (CHECK_FLAG(data->flags, XC_DATA_FLAG_GMPLS))  
    SET_FLAG(xc->flags, XC_ENTRY_FLAG_GMPLS);

  return xc;
}


/* Free XC entry */
void
gmpls_xc_free (struct xc_entry *xc)
{
  if (xc)
    XFREE (MTYPE_MPLS_XC_ENTRY, xc);
}

void
gmpls_packet_nhlfe_entry_set_key (struct nhlfe_entry *nhlfe, 
                                  struct nhlfe_new_data *data)
{
  struct nhlfe_key *key;

  key = (struct nhlfe_key *) &nhlfe->nkey;
  key->afi = data->nh_addr.afi;
  if ((key->afi == AFI_IP) 
#ifdef HAVE_GMPLS
      || (key->afi == AFI_UNNUMBERED)
#endif /* HAVE_GMPLS*/
     )
    {
      pal_mem_cpy (&key->u.ipv4.nh_addr, &data->nh_addr.u.ipv4, 
                   IPV4_MAX_BYTELEN);
      key->u.ipv4.oif_ix = data->oif_ix; 
      key->u.ipv4.out_label = data->out_label.u.pkt; 
    }
#ifdef HAVE_IPV6
  else if (key->afi == AFI_IP6)
    {
      pal_mem_cpy (&key->u.ipv6.nh_addr, &data->nh_addr.u.ipv6, 
                   sizeof (struct pal_in6_addr));
      key->u.ipv6.oif_ix = data->oif_ix;
      key->u.ipv6.out_label = data->out_label.u.pkt;
    }
#endif /* HAVE_IPV6 */  

  return;
}

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
void
gmpls_pbb_te_nhlfe_entry_set_key (struct nhlfe_entry *nhlfe,
                                  struct nhlfe_new_data *data)
{
  struct nhlfe_pbb_key *key;

  key = (struct nhlfe_pbb_key *) &nhlfe->nkey;
  key->oif_ix = data->oif_ix;
  key->lbl = data->out_label.u.pbb;
  return;
}

#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

#ifdef HAVE_TDM
void
gmpls_tdm_nhlfe_entry_set_key (struct nhlfe_entry *nhlfe,
                         struct nhlfe_new_data *data)
{
  struct nhlfe_tdm_key *key;

  key = (struct nhlfe_tdm_key *) &nhlfe->nkey;
  key->oif_ix = data->oif_ix;
  key->lbl = data->out_label.u.tdm;
  return;
}

#endif /* HAVE_TDM */

void
gmpls_nhlfe_entry_set_key (struct nhlfe_entry *nhlfe, 
                           struct nhlfe_new_data *data)
{
  switch (data->out_label.type)
    {
      case gmpls_entry_type_ip:     
        gmpls_packet_nhlfe_entry_set_key (nhlfe, data);
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:     
        gmpls_pbb_te_nhlfe_entry_set_key (nhlfe, data);
        break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:     
        gmpls_tdm_nhlfe_entry_set_key (nhlfe, data);
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
        break;
    }
}

s_int32_t
gmpls_nhlfe_entry_size (struct nhlfe_new_data *data)
{
  s_int32_t sz = 0;

#ifdef HAVE_PACKET
  if (data->out_label.type == gmpls_entry_type_ip)
    {
      /* The one byte AFI size is already taken care of */
      if (data->nh_addr.afi == AFI_IP)
         sz = sizeof (struct nhlfe_key_ipv4);
#ifdef HAVE_IPV6
       if (data->nh_addr.afi == AFI_IP6)
         sz = sizeof (struct nhlfe_key_ipv6);
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
       if (data->nh_addr.afi == AFI_UNNUMBERED)
         sz = sizeof (struct nhlfe_key_unnum);
#endif /* HAVE_GMPLS */
    }
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  if (data->out_label.type == gmpls_entry_type_pbb_te)
    {
      /* First byte to be skipped */
      sz = sizeof (struct nhlfe_pbb_key);
    }
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
  if (data->out_label.type == gmpls_entry_type_tdm)
    {
      /* First byte to be skipped */
      sz = sizeof (struct nhlfe_tdm_key);
    }
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

  return sz;
}

/* Create NHLFE Entry */
static struct nhlfe_entry *
gmpls_nhlfe_new (struct nhlfe_new_data *data, s_int32_t *ret_code)
{
  struct nhlfe_entry *nhlfe;

  *ret_code = NSM_SUCCESS;
  
  NSM_ASSERT (data != NULL);
  if (! data)
    {
      *ret_code = NSM_ERR_INVALID_ARGS;
      return NULL;
    }

  nhlfe = XMALLOC (MTYPE_MPLS_NHLFE_ENTRY, sizeof (struct nhlfe_entry) + 
                   gmpls_nhlfe_entry_size (data));
  if (! nhlfe)
    {
      *ret_code = NSM_ERR_MEM_ALLOC_FAILURE;
      return NULL;
    }
  
  pal_mem_set (nhlfe, 0, sizeof (struct nhlfe_entry) +
                         gmpls_nhlfe_entry_size (data));
  nhlfe->owner = data->owner;
  nhlfe->opcode = data->opcode;
  nhlfe->row_status = RS_NOT_READY;
  if (CHECK_FLAG(data->flags, NHD_DATA_FLAG_GMPLS))
    SET_FLAG(nhlfe->flags, NHLFE_ENTRY_FLAG_GMPLS);

  /* Set the type of entry i.e. Packet/ PBB/ TDM */
  nhlfe->type = data->out_label.type;
  gmpls_nhlfe_entry_set_key (nhlfe, data);

  return nhlfe;
}

/* Free NHLFE entry */
void 
gmpls_nhlfe_free (struct nhlfe_entry *nhlfe)
{
  NSM_ASSERT (nhlfe != NULL);
  if (! nhlfe)
    return;

  XFREE (MTYPE_MPLS_NHLFE_ENTRY, nhlfe);
}


void
gmpls_ftn_xc_link (struct ftn_entry *ftn, struct xc_entry *xc)
{
  NSM_ASSERT ((ftn != NULL) && (xc != NULL));
  if ((! ftn) || (! xc))
    return;
  
  ftn->xc = xc;
  xc->refcount++;
}

struct xc_entry *
gmpls_ftn_xc_unlink (struct ftn_entry *ftn)
{
  struct xc_entry *xc;

  NSM_ASSERT (ftn != NULL);
  if (! ftn)
    return NULL;

  xc = ftn->xc;
  if (! xc)
    return NULL;
  
  xc->refcount--;
  ftn->xc = NULL;

  return xc;
}

s_int32_t
gmpls_dep_ftn_row_update (struct ftn_entry *ftn_dep, struct ftn_entry *ftn_new)
{
  if (! ftn_new || ! ftn_new->xc || ! ftn_new->xc->nhlfe)
    return -1;

  gmpls_ftn_xc_unlink (ftn_dep);
  gmpls_ftn_xc_link (ftn_dep, ftn_new->xc);

  return 0;
}

void
gmpls_ilm_xc_link (struct ilm_entry *ilm, struct xc_entry *xc)
{
  NSM_ASSERT ((ilm != NULL) && (xc != NULL));
  if ((! ilm) || (! xc))
    return;

  ilm->xc = xc;
  xc->refcount++;
}

struct xc_entry *
gmpls_ilm_xc_unlink (struct ilm_entry *ilm)
{
  struct xc_entry *xc;

  NSM_ASSERT (ilm != NULL);
  if (! ilm)
    return NULL;

  xc = ilm->xc;
  if (! xc)
    return NULL;

  xc->refcount--;
  ilm->xc = NULL;

  return xc;
}

void
gmpls_xc_nhlfe_link (struct xc_entry *xc, struct nhlfe_entry *nhlfe)
{
  NSM_ASSERT ((xc != NULL) && (nhlfe != NULL));
  if ((! xc) || (! nhlfe))
    return;
  
  xc->nhlfe = nhlfe;
  xc->key.nhlfe_ix = nhlfe->nhlfe_ix;
  xc->key.xc_ix = nhlfe->xc_ix;
  nhlfe->refcount++;
}

struct nhlfe_entry *
gmpls_xc_nhlfe_unlink (struct xc_entry *xc)
{
  struct nhlfe_entry *nhlfe;

  NSM_ASSERT (xc != NULL);
  if (! xc)
    return NULL;

  nhlfe = xc->nhlfe;
  if (! nhlfe)
    return NULL;

  nhlfe->refcount--;
  xc->nhlfe = NULL;
  
  return nhlfe;
}

/* Create an ILM key. */
#ifdef HAVE_PACKET

/* All MPLS related functions to be removed. The GMPLS functions will take of 
   MPLS and other functions */
void
mpls_make_ilm_key (u_int32_t iif_ix, u_int32_t in_label, struct ilm_key *key,
                   u_int32_t *len)
{
  pal_mem_set (key, 0, sizeof (struct ilm_key));
  key->iif_ix = pal_hton32 (iif_ix);
  PREP_FOR_NETWORK (key->in_label, in_label, MAX_LABEL_BITLEN);
  if (len)
    *len = ILM_KEY_BITLEN;
}

static void
gmpls_packet_make_ilm_key (u_int32_t iif_ix, u_int32_t *in_label, 
                           struct ilm_key *key)
{
  pal_mem_set (key, 0, sizeof (struct ilm_key));
  key->iif_ix = iif_ix;
  key->in_label = *in_label;
}

#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
static void
gmpls_pbb_te_make_ilm_key (u_int32_t iif_ix, struct pbb_te_label *in_label,
                           struct ilm_key_pbb *key)
{
  pal_mem_set (key, 0, sizeof (struct ilm_key_pbb));

  key->iif_ix = iif_ix;
  pal_mem_cpy (&key->in_label, in_label, sizeof(struct pbb_te_label));
  return;
}

#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
static void
gmpls_tdm_make_ilm_key (u_int32_t iif_ix, struct tdm_label *in_label,
                        struct ilm_key_tdm *key)
{
  pal_mem_set (key, 0, sizeof (struct ilm_key_tdm));

  key->iif_ix = iif_ix;
  pal_mem_cpy (&key->in_label, in_label, sizeof(struct tdm_label));
  return;
}

#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

static void
gmpls_make_ilm_key (u_int32_t iif_ix, struct gmpls_gen_label *in_label,
                    struct ilm_key_gen *key)
{
  switch (in_label->type)
    {
      case gmpls_entry_type_ip:
        gmpls_packet_make_ilm_key (iif_ix, &in_label->u.pkt, &key->u.pkt);
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        gmpls_pbb_te_make_ilm_key (iif_ix, &in_label->u.pbb, &key->u.pbb_key);
        break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        gmpls_tdm_make_ilm_key (iif_ix, &in_label->u.tdm, &key->u.tdm_key);
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        break;
    }
}

static void
gmpls_make_xc_key (u_int32_t xc_ix, u_int32_t iif_ix, 
                   struct gmpls_gen_label *in_label,
                   u_int32_t nhlfe_ix, struct xc_key *key)
{
  key->xc_ix = xc_ix;
  key->in_label = *in_label;
  key->iif_ix = iif_ix;

#ifdef HAVE_PACKET
  /* Only for packet we need to change the label to network order */
  if (in_label->type == gmpls_entry_type_ip)
    key->in_label.u.pkt = pal_hton32 (in_label->u.pkt);
#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  if (in_label->type == gmpls_entry_type_pbb_te)
    key->in_label.u.pkt = in_label->u.pkt;
#endif /* HAVE_PBB_TE */ 
#ifdef HAVE_TDM
  if (in_label->type == gmpls_entry_type_tdm)
    key->in_label.u.pkt = pal_hton32 (in_label->u.tdm);
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

  key->nhlfe_ix = nhlfe_ix;
}


static void
gmpls_make_nhlfe_key (struct addr_in *nh_addr, u_int32_t oif_ix,
                      struct gmpls_gen_label *out_label, 
                      struct nhlfe_key_gen *key, 
                      u_int32_t *key_len)
{
  switch (out_label->type)
    {
      case gmpls_entry_type_ip:
        key->u.pkt.afi = nh_addr->afi;
        if ((nh_addr->afi== AFI_IP) ||
#ifdef HAVE_GMPLS
             (nh_addr->afi== AFI_UNNUMBERED) ||
#endif /* HAVE_GMPLS */
             0)
          { 
            /* assuming ifindex and IP address will never overlap */
            pal_mem_cpy (&key->u.pkt.u.ipv4.nh_addr, &nh_addr->u.ipv4, 
                         IPV4_MAX_BYTELEN); 
            key->u.pkt.u.ipv4.oif_ix = oif_ix; 
            key->u.pkt.u.ipv4.out_label = out_label->u.pkt; 
            if (key_len)
              *key_len = IPV4_NHLFE_KEY_BITLEN; 
          } 
#ifdef HAVE_IPV6 
        else if (nh_addr->afi == AFI_IP6) 
          { 
            pal_mem_cpy (&key->u.pkt.u.ipv6.nh_addr, &nh_addr->u.ipv6, 
                         IPV6_MAX_BYTELEN); 
            key->u.pkt.u.ipv6.oif_ix = pal_hton32 (oif_ix); 
            PREP_FOR_NETWORK (key->u.pkt.u.ipv6.out_label, out_label->u.pkt, 
                              MAX_LABEL_BITLEN); 
            if (key_len)
              *key_len = IPV6_NHLFE_KEY_BITLEN; 
          }
#endif /* HAVE_IPV6 */ 
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
    case gmpls_entry_type_pbb_te:
       key->u.pbb.oif_ix = pal_hton32 (oif_ix);
       key->u.pbb.lbl = out_label->u.pbb;
       if (key_len)
         *key_len = PBB_TE_NHLE_KEY_BITLEN;
       break;

#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
    case gmpls_entry_type_tdm:
       if ((nh_addr->afi== AFI_IP) || (nh_addr->afi== AFI_UNNUMBERED))
         {
           /* assuming ifindex and IP address will never overlap */
           pal_mem_cpy (&key->u.tdm.u.ipv4.nh_addr, &nh_addr->u.ipv4, 
                        IPV4_MAX_BYTELEN);
           key->u.tdm.u.ipv4.oif_ix = pal_hton32 (oif_ix);
           key->u.tdm.u.ipv4.out_label = out_label->u.tdm;
           if (key_len)
             *key_len = IPV4_NHLFE_KEY_BITLEN;
          }
#ifdef HAVE_IPV6
       else if (nh_addr->afi == AFI_IP6)
         {
           pal_mem_cpy (&key->u.tdm.u.ipv6.nh_addr, &nh_addr->u.ipv6, 
                        IPV6_MAX_BYTELEN);
           key->u.tdm.u.ipv6.oif_ix = pal_hton32 (oif_ix);
           key->u.tdm.u.ipv6.out_label = out_label->u.tdm;
           if (key_len)
             *key_len = IPV6_NHLFE_KEY_BITLEN;
          }
#endif /* HAVE_IPV6 */
      break;

#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
        break;
    }
}

u_int32_t
gmpls_nhlfe_outgoing_if (struct nhlfe_entry *nhlfe)
{
  struct nhlfe_key_gen *key;

  key = (struct nhlfe_key_gen *) nhlfe->nkey;

  switch (nhlfe->type)
    {
      case gmpls_entry_type_ip:
        if (key->u.pkt.afi == AFI_IP)
          return key->u.pkt.u.ipv4.oif_ix; 
#ifdef HAVE_IPV6 
        else if (key->u.pkt.afi == AFI_IP6)
          return key->u.pkt.u.ipv6.oif_ix; 
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
        else if (key->u.pkt.afi == AFI_UNNUMBERED)
          return key->u.pkt.u.unnum.oif_ix; 
#endif /* HAVE_GMPLS */
        break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        return key->u.pbb.oif_ix;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        return key->u.tdm.oif_ix;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        break;
     }

  return 0;
}

void
gmpls_nhlfe_outgoing_label (struct nhlfe_entry *nhlfe, 
                            struct gmpls_gen_label *lbl)
{
  struct nhlfe_key_gen *key;

  key = (struct nhlfe_key_gen *) nhlfe->nkey;

  switch (nhlfe->type)
    {
      case gmpls_entry_type_ip:
        lbl->type = gmpls_entry_type_ip;
        if (key->u.pkt.afi == AFI_IP) 
          {
            lbl->u.pkt = key->u.pkt.u.ipv4.out_label;
            break;
          }
#ifdef HAVE_IPV6 
        else if (key->u.pkt.afi == AFI_IP6) 
          {
            lbl->u.pkt = key->u.pkt.u.ipv6.out_label;
          }
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
        if (key->u.pkt.afi == AFI_UNNUMBERED)
          {
            lbl->u.pkt = key->u.pkt.u.unnum.out_label;
          }
#endif /* HAVE_GMPLS */
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        lbl->u.pbb = key->u.pbb.lbl;
        break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        lbl->u.tdm = key->u.tdm.out_label;
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      
      default:
        break;
     }

  return;
}

s_int32_t
gmpls_nhlfe_nh_addr (struct nhlfe_entry *nhlfe, 
                     struct addr_in *nh_addr)
{
  struct nhlfe_key_gen *key;

  key = (struct nhlfe_key_gen *) nhlfe->nkey;

  switch (nhlfe->type)
    {
      case gmpls_entry_type_ip:
        nh_addr->afi = key->u.pkt.afi;

        if ((key->u.pkt.afi == AFI_IP) 
#ifdef HAVE_GMPLS
             || (key->u.pkt.afi == AFI_UNNUMBERED)
#endif /* HAVE_GMPLS */
           )
          {
            pal_mem_cpy (&nh_addr->u.ipv4, &key->u.pkt.u.ipv4.nh_addr, 
                         IPV4_MAX_BYTELEN);
            return NSM_SUCCESS;
          }
#ifdef HAVE_IPV6 
        else if (key->u.pkt.afi == AFI_IP6) 
          {
            pal_mem_cpy (&nh_addr->u.ipv6, &key->u.pkt.u.ipv6.nh_addr, 
                         IPV6_MAX_BYTELEN);
            return NSM_SUCCESS;
          }
#endif /* HAVE_IPV6 */
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
     default:
       break;
    }

  return NSM_ERR_INVALID_ADDR_FAMILY;
}

/* Helper function to get priority rating for owner. */
mpls_lsp_priority_t
gmpls_owner_priority (mpls_owner_t owner)
{
  switch (owner)
    {
    case MPLS_OTHER_CLI:
      return MPLS_P_CLI;
    case MPLS_RSVP:
      return MPLS_P_RSVP;
    case MPLS_LDP:
      return MPLS_P_LDP;
    case MPLS_CRLDP:
      return MPLS_P_CRLDP;
    case MPLS_IGP_SHORTCUT:
      return MPLS_P_IGP_SHORTCUT;
    default:
      return MPLS_P_NONE;
    }
}

/* Check if new FTN entry is a candidate primary entry */
static bool_t
mpls_is_primary_candidate (struct ftn_entry *ftn_primary,
                           struct ftn_entry *ftn, 
                           bool_t is_primary)
{
  if (ftn->ftn_type == MPLS_FTN_REGULAR || 
      ftn->ftn_type == MPLS_FTN_RSVP_MAPPED ||
      ftn->ftn_type == MPLS_FTN_IGP_SHORTCUT)
    if (! ftn_primary || is_primary ||
         CHECK_FLAG (ftn_primary->flags, FTN_ENTRY_FLAG_INACTIVE))
    return NSM_TRUE;

  return NSM_FALSE;
}

/* Get first active ftn in ftn list */
struct ftn_entry *
gmpls_ftn_active_head (struct ftn_entry *ftn_list)
{
  struct ftn_entry *ftn_head = ftn_list;

  while (ftn_head)
    {
      if (! CHECK_FLAG (ftn_head->flags, FTN_ENTRY_FLAG_INACTIVE))
        break;

      ftn_head = ftn_head->next;
    }

  return ftn_head;
}

/* Get first active ftn in ftn list with the same tunnel-id */
struct ftn_entry *
gmpls_ftn_active_tunnel_head (struct ftn_entry *ftn_list, u_int32_t t_id, 
                              u_int16_t flags)
{
  struct ftn_entry *ftn_head = ftn_list;

  while (ftn_head)
    {
      if (! CHECK_FLAG (ftn_head->flags, FTN_ENTRY_FLAG_INACTIVE) &&
          ftn_head->tun_id == t_id 
#ifdef HAVE_GMPLS
          /* If the two have the same value for bidirectional as well as 
             direction of the bidirectional LSP */
          && (((flags & FTN_ENTRY_FLAG_BIDIR) == 
               (ftn_head->flags & FTN_ENTRY_FLAG_BIDIR))
              && ((flags & FTN_ENTRY_FLAG_FWD) == (flags & FTN_ENTRY_FLAG_FWD)))
#endif /* HAVE_GMPLS */
          )
        break;

      ftn_head = ftn_head->next;
    }

  return ftn_head;
}

/* Find first active ftn_entry with the same tunnel_id */
struct ftn_entry *
mpls_ftn_match_tunnel_id (struct ftn_entry *ftn_list, u_int32_t t_id) 
{
  struct ftn_entry *tmp_ftn;

  for (tmp_ftn = gmpls_ftn_active_head (ftn_list); tmp_ftn; 
       tmp_ftn = tmp_ftn->next)
    {
      if (tmp_ftn->tun_id == t_id)
        return tmp_ftn;
    }

  return NULL;
}

/* Match the passed-in prefix to an existing node in the table,
   and return it. */
struct ptree_node *
gmpls_ftn_node_match (struct nsm_master *nm,
                      u_int32_t vrf_id, struct fec_gen_entry *fec)
{
  struct ptree_ix_table *ft;
  struct ptree_node *pn;
  u_char key [40];
  u_int16_t key_len;
  
  /* Get correct FTN table. */
  ft = nsm_gmpls_ftn_table_lookup (nm, vrf_id, fec);
  if (! ft)
    return NULL;

  gmpls_ftn_node_generate_key (fec, key, &key_len);

  /* Match. */
  pn = ptree_node_match (ft->m_table, key, key_len);
  if (! pn)
    return NULL;

  ptree_unlock_node (pn);
  return pn;
}

struct ptree_node *
mpls_ftn_node_match (struct nsm_master *nm,
                     u_int32_t vrf_id, struct prefix *p)
{
  struct ptree_node *pn;
  struct fec_gen_entry fec;

  fec.type = gmpls_entry_type_ip;
  fec.u.prefix = *p;

  pn = gmpls_ftn_node_match (nm, vrf_id, &fec);
  return pn;
}

/* Get primary ftn entry for the match found for the specified
   prefix. */
struct ftn_entry *
gmpls_ftn_match_primary (struct nsm_master *nm, u_int32_t vrf_id,
                         struct fec_gen_entry *fec)
{
  struct ptree_node *pn;

  /* Find matching node. */
  pn = gmpls_ftn_node_match (nm, vrf_id, fec);
  if (! pn)
    return NULL;

  return pn->info;
}


struct ptree_node *
gmpls_ftn_node_match_exclude (struct nsm_master *nm, u_int32_t vrf_id,
                              struct prefix *pfx, struct prefix *excl_pfx)
{
  struct ptree_ix_table *ft;
  struct ptree_node *pn;
  struct fec_gen_entry gfec;

  gfec.type = gmpls_entry_type_ip;
  gfec.u.prefix = *pfx;

  /* Get correct FTN table. */
  ft = nsm_gmpls_ftn_table_lookup (nm, vrf_id, &gfec);
  if (! ft)
    return NULL;

  /* get matching node */
  pn = ptree_node_match_exclude (ft->m_table, &pfx->u.prefix, pfx->prefixlen, 
                                 &excl_pfx->u.prefix, excl_pfx->prefixlen);
  if (! pn)
    return NULL;

  ptree_unlock_node (pn);
  return pn; 
}

#ifdef HAVE_PACKET
struct ilm_entry *
mpls_ilm_node_lookup (struct nsm_master *nm, u_int32_t iif_ix, 
                       u_int32_t in_label)
{
  struct ilm_entry tmp_ilm;
  struct avl_node *an;
  struct gmpls_gen_label label;

  if ((iif_ix == 0) || (in_label == 0))
    return NULL;

  NSM_ASSERT ((iif_ix > 0) && (in_label > 0));

  label.type = gmpls_entry_type_ip;
  label.u.pkt = in_label;

  gmpls_make_ilm_key (iif_ix, &label, &tmp_ilm.key);
  an = avl_search (ILM_TABLE, (u_char *)&tmp_ilm);
  if (! an)
    return NULL;

  return an->info;
}

#endif /* HAVE_PACKET */

struct ilm_entry *
gmpls_ilm_node_lookup (struct nsm_master *nm, u_int32_t iif_ix, 
                       struct gmpls_gen_label *label)
{
  struct avl_node *an = NULL;
  struct ilm_key_gen *key;
  struct ilm_entry tmp_entry;

  if ((iif_ix == 0) 
#ifdef HAVE_PACKET
      || ((label->type == gmpls_entry_type_ip) && (label->u.pkt == 0))
#endif /* HAVE_PACKET */
     )
    return NULL;

  NSM_ASSERT (iif_ix > 0);

  pal_mem_set (&tmp_entry, 0, sizeof (struct ilm_entry));
  key = &tmp_entry.key;
  switch (label->type)
    {
      case gmpls_entry_type_ip:
        gmpls_packet_make_ilm_key (iif_ix, &label->u.pkt, &key->u.pkt);
        an = avl_search (ILM_TABLE, &tmp_entry);
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case  gmpls_entry_type_pbb_te:
        gmpls_pbb_te_make_ilm_key (iif_ix, &label->u.pbb, &key->u.pbb_key); 
        an = avl_search (ILM_PBB_TABLE, &tmp_entry);
        break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        gmpls_tdm_make_ilm_key (iif_ix, label->u.tdm, &key->u.tdm_key);
        an = avl_search (ILM_TDM_TABLE, &tmp_entry);
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
         break;
    }

  if (an == NULL)
    return NULL;

  return an->info;
}

void
gmpls_ilm_node_remove (struct nsm_master *nm, struct ilm_entry *ilm)
{
  switch (ilm->ent_type)
    {
      case ILM_ENTRY_TYPE_PACKET:
        if (ilm->key.u.pkt.iif_ix == 0)
          return;

        NSM_ASSERT (ilm->key.u.pkt.iif_ix > 0);

        avl_remove (ILM_TABLE, ilm);
        break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case ILM_ENTRY_TYPE_PBB_TE:
        if (ilm->key.u.pbb_key.iif_ix == 0)
          return;

        NSM_ASSERT (ilm->key.u.pbb_key.iif_ix > 0);

        avl_remove (ILM_PBB_TABLE, ilm);
        break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case ILM_ENTRY_TYPE_TDM:
        if (ilm->key.u.tdm_key.iif_ix == 0)
          return;

        NSM_ASSERT (ilm->key.u.tdm.iif_ix > 0);

        avl_remove (ILM_TDM_TABLE, ilm);
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
         break;
    }

  return;
}

/* Get primary/independent mapped entry for specified prefix. */
struct ftn_entry *
nsm_gmpls_get_mapped_ftn (struct nsm_master *nm, struct fec_gen_entry *fec)
{
  struct ftn_entry *ftn;

  ftn = gmpls_ftn_match_primary (nm, GLOBAL_FTN_ID, fec);
  if (ftn
      && CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED)
      && ! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_DEPENDENT))
    return ftn;

  return NULL;
}

struct ftn_entry *
nsm_mpls_get_ftn_by_proto (struct nsm_master *nm, struct prefix *p, 
                           u_char proto)
{
  struct ftn_entry *ftn;
  struct fec_gen_entry fec;

  fec.type = gmpls_entry_type_ip;
  fec.u.prefix = *p;

  ftn = gmpls_ftn_match_primary (nm, GLOBAL_FTN_ID, &fec);
  for (; ftn != NULL; ftn = ftn->next)
    {
      if (ftn->protocol == proto)
        return ftn;
    }

  return NULL;
}

s_int32_t
gmpls_ftn_get_lookup_flag (struct ftn_entry *ftn, bool_t *lookup)
{
  u_int32_t out_label;
  struct xc_key *key;
  struct nhlfe_key_gen *nkey;

  *lookup = NSM_FALSE;

  if (! ftn || ! ftn->xc || ! ftn->xc->nhlfe)
    return NSM_ERR_INVALID_ARGS;
  
  key = (struct xc_key *) &ftn->xc->key;
  nkey = (struct nhlfe_key_gen *) ftn->xc->nhlfe->nkey;

  switch (key->in_label.type)
    {
      case gmpls_entry_type_ip:
        if ((nkey->u.pkt.afi == AFI_IP)
#ifdef HAVE_GMPLS
           || (nkey->u.pkt.afi == AFI_UNNUMBERED)
#endif /* HAVE_GMPLS */
          )
         {
           out_label = nkey->u.pkt.u.ipv4.out_label;
         }
#ifdef HAVE_IPV6
       else if (nkey->u.pkt.afi == AFI_IP6)
         out_label = nkey->u.pkt.u.ipv6.out_label;
#endif /* HAVE_IPV6 */
        else
          {
            zlog_err (NSM_ZG, "Invalid address family %d ",
                      nkey->u.pkt.afi);
            return NSM_ERR_INVALID_ADDR_FAMILY;
          }

        if (out_label != LABEL_IMPLICIT_NULL &&
            out_label != LABEL_IPV4_EXP_NULL
#ifdef HAVE_IPV6
            && out_label != LABEL_IPV6_EXP_NULL
#endif /* HAVE_IPV6 */
            )
          *lookup = NSM_TRUE;
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
        *lookup = NSM_TRUE;
        return NSM_SUCCESS;
    }

      
  return NSM_SUCCESS;
}

struct ptree_node *
nsm_gmpls_ftn_rn_lookup (struct ptree *tbl, struct ftn_entry *ftn)
{
  struct ptree_node *tmp_pn, *pn = NULL;
  struct ftn_entry *ftn_list, *tmp_ftn;

  for (tmp_pn = ptree_top (tbl); tmp_pn; tmp_pn = ptree_next (tmp_pn))
    {
      ftn_list = tmp_pn->info;
      if (! ftn_list)
        continue;

      for (tmp_ftn = ftn_list; tmp_ftn; tmp_ftn = tmp_ftn->next)
        if (tmp_ftn == ftn)
          {
            pn = tmp_pn;
            break;
          }

      if (pn)
        break;
    }
  return pn;
}

s_int32_t
nsm_gmpls_map_route_ftn_add_gen_data_set (struct ftn_add_gen_data *fad,
                                          struct mapped_route *route,
                                          struct fec_gen_entry *fec,
                                          struct ftn_entry *t_ftn,
                                          u_int32_t t_id)
{
  struct nhlfe_key_gen *key;
  struct nhlfe_key *nkey;

  pal_mem_set (fad, 0, sizeof (struct ftn_add_gen_data));
  fad->owner.owner = route->owner;
  fad->vrf_id = GLOBAL_FTN_ID;

  /* Copy the FEC */
  fad->ftn = *fec; 
  fad->ftn_ix = route->ix;

  if (! CHECK_FLAG (t_ftn->flags, FTN_ENTRY_FLAG_GMPLS))
    SET_FLAG (fad->flags, NSM_MSG_GEN_FTN_MPLS);

  key = (struct nhlfe_key_gen *) &FTN_NHLFE (t_ftn)->nkey;
  nkey = (struct nhlfe_key *) &FTN_NHLFE (t_ftn)->nkey;

  switch (fec->type)
    {
      case gmpls_entry_type_ip:
        fad->nh_addr.afi = key->u.pkt.afi;
        nsm_gmpls_owner_afi_set (&fad->owner, fad->nh_addr.afi);
  
        if ((fad->nh_addr.afi == AFI_IP)
#ifdef HAVE_GMPLS
            || (fad->nh_addr.afi == AFI_UNNUMBERED)
#endif /* HAVE_GMPLS */
           )
          {
            fad->nh_addr.u.ipv4 = nkey->u.ipv4.nh_addr;
            fad->out_label.u.pkt = nkey->u.ipv4.out_label;
            fad->out_label.type = gmpls_entry_type_ip;
            fad->oif_ix = nkey->u.ipv4.oif_ix;
          }
#ifdef HAVE_IPV6
        else if (fad->nh_addr.afi == AFI_IP6)
          {
            fad->nh_addr.u.ipv6 = nkey->u.ipv6.nh_addr;
            fad->out_label.u.pkt = nkey->u.ipv6.out_label;
            fad->out_label.type = gmpls_entry_type_ip;
            fad->oif_ix = nkey->u.ipv6.oif_ix;
          }
#endif /* HAVE_IPV6 */
        else
          {
            return NSM_ERR_INVALID_ADDR_FAMILY;
            NSM_ASSERT (0);
          }
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
        break;
    }
 
  fad->lspid = FTN_XC (t_ftn)->lspid;
  fad->exp_bits = t_ftn->exp_bits;

#ifdef HAVE_PACKET
#ifdef HAVE_DIFFSERV
  fad->ds_info = t_ftn->ds_info;
#endif /* HAVE_DIFFSERV */
#endif /* HAVE_PACKET */


#ifdef HAVE_PACKET
  fad->dscp_in = DIFFSERV_INVALID_DSCP;
#endif /* HAVE_PACKET */

  fad->lsp_bits.bits.lsp_type = LSP_TYPE_PRIMARY;
  fad->lsp_bits.bits.act_type = t_ftn->lsp_bits.bits.act_type;
  fad->opcode = FTN_NHLFE (t_ftn)->opcode;
  fad->tunnel_id = t_id;
  fad->pri_fec_prefix = prefix_new ();
  prefix_copy (fad->pri_fec_prefix, &route->fec);

  return 0;
}

/* Add mapped route */
s_int32_t
nsm_gmpls_map_route_add (struct nsm_master *nm,
                         struct ftn_entry *ftn,
                         struct mapped_route *route,
                         struct fec_gen_entry *fec,
                         mpls_ftn_type_t ftn_type,
                         u_int32_t t_id)
{
  struct ftn_add_gen_data fad;
  struct ftn_ret_data rd;
  s_int32_t ret;
  
  if (! ftn || ! ftn->xc || ! ftn->xc->nhlfe || ! route) 
    return NSM_FAILURE;
 
  pal_mem_set (&rd, 0, sizeof (struct ftn_ret_data));

  /* Get mapped route. */
  
  /* Fill ftn add data. */
  ret = nsm_gmpls_map_route_ftn_add_gen_data_set (&fad, route, fec, ftn, t_id);
  if (ret < 0)
    return NSM_FAILURE; 

  ret = nsm_gmpls_ftn_add_msg_process (nm, &fad, &rd, ftn_type, 
                                      NSM_FALSE);
  if (ret < 0)
    {
      zlog_warn (NSM_ZG, "Mapped route %O --> %O addition failure",
                 fec, &route->fec);
      return NSM_FAILURE;
    }

  /* Set returned FTN IX in mapped route object. */
  route->ix = rd.ftn_ix;
  
  return NSM_SUCCESS;
}

/* Del mapped route */
s_int32_t
nsm_gmpls_map_route_del (struct nsm_master *nm,
                         struct ftn_entry *ftn,
                         struct mapped_route *route,
                         struct fec_gen_entry *fec,
                         mpls_ftn_type_t ftn_type,
                         u_int32_t t_id)
{
  s_int32_t ret;
  
  if (! ftn || ! route)
    return NSM_FAILURE;
  
  if (route->ix)
    {
      /* Fast delete. */
      ret = nsm_gmpls_ftn_del_msg_process (nm, GLOBAL_FTN_ID,
                                          fec, route->ix);
      if (ret < 0)
        {
          zlog_warn (NSM_ZG, "Delete failed for mapped route %O", fec);
          return NSM_FAILURE;
        }
      route->ix = 0;
    }
  else /* Slow delete. */
    {
      struct ftn_add_gen_data fad;
      
      /* Fill data. */
      ret = nsm_gmpls_map_route_ftn_add_gen_data_set (&fad, route, fec, 
                                                      ftn, t_id);
      if (ret < 0)
        return NSM_FAILURE;

      ret = nsm_gmpls_ftn_del_slow_msg_process (nm, &fad);
      if (ret < 0)
        {
          if (fec->type == gmpls_entry_type_ip)
            {
              zlog_warn (NSM_ZG, "Delete failed for mapped route %O", &fec->u.prefix);
            }
          return NSM_FAILURE;
        }
    }

  return NSM_SUCCESS;
}

void
nsm_gmpls_set_fec_from_mapped_route (struct ptree_node *mapped, 
                                     struct fec_gen_entry *fec)
{
  struct mapped_route *route;
  u_int32_t byte_len;

  if (!mapped || !fec)
    return;

  pal_mem_set (fec, '\0', sizeof (struct fec_gen_entry));
  route = mapped->info;
  switch (route->type)
    {
      case MAPPED_ENTRY_TYPE_IPv4:
        fec->type = gmpls_entry_type_ip;
        fec->u.prefix.family = AF_INET;
        fec->u.prefix.prefixlen = mapped->key_len;
        byte_len = mapped->key_len / 8;
        if (mapped->key_len % 8)
          byte_len ++;

        pal_mem_cpy (&fec->u.prefix.u.prefix, mapped->key, byte_len);
        break;

#ifdef HAVE_IPV6
      case MAPPED_ENTRY_TYPE_IPV6:
        fec->type = gmpls_entry_type_ip;
        fec->u.prefix.family = AF_INET6;
        fec->u.prefix.prefixlen = mapped->key_len;
        byte_len = mapped->key_len / 8;
        if (mapped->key_len % 8)
          byte_len ++;

        pal_mem_cpy (&fec->u.prefix.u.prefix, mapped->key, byte_len);
        break;
#endif /* HAVE_IPV6 */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case MAPPED_ENTRY_TYPE_PBB_TE:
        fec->type = gmpls_entry_type_pbb_te;
        pal_mem_cpy(&fec->u.pbb, 
                   (struct fec_entry_pbb_te *) mapped->key, 
                   sizeof(struct fec_entry_pbb_te));
        break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case MAPPED_ENTRY_TYPE_TDM:
        fec->type = gmpls_entry_type_tdm;
        fec->u.tdm = (struct fec_entry_tdm *) mapped->key;
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
        break;
    }
  return;
}

void
nsm_gmpls_set_fec_from_ftn (struct ftn_entry *ftn, struct fec_gen_entry *fec)
{
  u_char byte_len;

  pal_mem_set (fec, '\0', sizeof (struct fec_gen_entry));

  switch (ftn->ent_type)
    {
      case FTN_ENTRY_TYPE_IPV4:
        fec->type = gmpls_entry_type_ip;
        fec->u.prefix.family = AF_INET;
        fec->u.prefix.prefixlen = ftn->pn->key_len;
        byte_len = ftn->pn->key_len / 8;
        if (ftn->pn->key_len % 8)
          byte_len ++;
        pal_mem_cpy (&fec->u.prefix.u.prefix, ftn->pn->key, byte_len);
        break;

#ifdef HAVE_IPV6
      case FTN_ENTRY_TYPE_IPV6:
        fec->type = gmpls_entry_type_ip;
        fec->u.prefix.family = AF_INET6;
        fec->u.prefix.prefixlen = ftn->pn->key_len;
        byte_len = ftn->pn->key_len / 8;
        if (ftn->pn->key_len % 8)
          byte_len ++;
        pal_mem_cpy (&fec->u.prefix.u.prefix, ftn->pn->key, byte_len);

        break;
#endif /* HAVE_IPV6 */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case FTN_ENTRY_TYPE_PBB_TE:
        fec->type = gmpls_entry_type_pbb_te;
        fec->u.pbb = *(struct fec_entry_pbb_te *) ftn->pn->key;
        break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case FTN_ENTRY_TYPE_TDM:
        fec->type = gmpls_entry_type_tdm;
        fec->u.tdm = *(struct fec_entry_tdm *) ftn->pn->key;
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        break;
    }

  return; 
}

void
gmpls_confirm_data_add_to_fwd (struct nsm_master *nm,
                              struct lsp_dep_confirm *confirm,
                              struct ftn_entry *ftn, bool_t lookup)
{
#ifdef HAVE_VRF
#ifdef HAVE_SNMP
  struct nsm_mpls *nmpls = NULL;

  nmpls = nm->nmpls;
#endif  /* HAVE_SNMP */
#endif  /* HAVE_VRF */

  if (! confirm || ! ftn)
    return;

  if (confirm->type == CONFIRM_MAPPED_ROUTE)
    {
      struct ptree_node *mapped_pn = confirm->data;
      struct mapped_route *route = mapped_pn->info;
      struct fec_gen_entry fec;

      nsm_gmpls_set_fec_from_mapped_route (mapped_pn, &fec);
      nsm_gmpls_map_route_add (nm, ftn, route, &fec, MPLS_FTN_MAPPED, 0);
    }
  else if (confirm->type == CONFIRM_RSVP_MAPPED_ROUTE ||
           confirm->type == CONFIRM_IGP_SHORTCUT_ROUTE)
    {
      struct ptree_node *pn = NULL;
      struct ftn_entry *ftn_c = confirm->data;
      struct ftn_entry *ftnc_head;
      struct fec_gen_entry fec;
      s_int32_t ret;
      s_int32_t active_head = NSM_FALSE;

      if (! CHECK_FLAG (ftn_c->flags, FTN_ENTRY_FLAG_SELECTED))
        return;

      pn = ftn_c->pn;
      nsm_gmpls_set_fec_from_ftn (ftn, &fec);
      if (pn) 
        {
          /* igp-shortcut route only add for the same tunnel-id */
          if ((confirm->type == CONFIRM_IGP_SHORTCUT_ROUTE ||
               confirm->type ==  CONFIRM_RSVP_MAPPED_ROUTE) && 
              ((ftn_c->tun_id != ftn->tun_id)
#ifdef HAVE_GMPLS
              || ((ftn_c->flags & FTN_ENTRY_FLAG_BIDIR) != 
                  (ftn_c->flags & FTN_ENTRY_FLAG_BIDIR))
              || ((ftn_c->flags & FTN_ENTRY_FLAG_FWD) != 
                  (ftn_c->flags & FTN_ENTRY_FLAG_FWD))
#endif /* HAVE_GMPLS */
              ))
            return;

          if (ftn_c->xc != ftn->xc)
            {
              gmpls_ftn_del_from_fwd (nm, ftn_c, &fec, GLOBAL_FTN_ID, ftn); 
              ret = gmpls_dep_ftn_row_update (ftn_c, ftn);
              if (fec.type == gmpls_entry_type_ip)
                {
                  if (ret < 0)
                    zlog_err (NSM_ZG, "Cannot update dependant ftn %P xc to "
                                      "selected primary ftn", &fec.u.prefix);
                  else
                    zlog_info (NSM_ZG, "Update dependant ftn %P xc to "
                                       "selected primary ftn", &fec.u.prefix);
                }
            }

          ftnc_head = gmpls_ftn_active_head ((struct ftn_entry *)pn->info);
          if (ftn_c == ftnc_head)
            active_head = NSM_TRUE;

          nsm_gmpls_ftn_add_check (nm, ftn_c, ftnc_head, pn);

#ifdef HAVE_PACKET
          if (fec.type == gmpls_entry_type_ip)
            {
#ifdef HAVE_IPV6
              if (fec.u.prefix.family == AF_INET6)
                ret = nsm_mpls_ftn6_add_to_fwd (nm, ftn_c, &fec.u.prefix, 
                                                pn->tree->id, NULL, 
                                                active_head);
              else
#endif /* HAVE_IPV6 */
                ret = nsm_mpls_ftn4_add_to_fwd (nm, ftn_c, &fec.u.prefix, 
                                                pn->tree->id, NULL, 
                                                active_head);
              if (ret == NSM_SUCCESS)
                {
                  SET_FLAG (ftn_c->flags, FTN_ENTRY_FLAG_INSTALLED); 
                  FTN_XC_OPR_STATUS_COUNT_INC (ftn_c);
                  FTN_XC_STATUS_UPDATE (ftn_c);
                }
            }
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
          if (fec.type == gmpls_entry_type_pbb_te)
            {
              /* Nothing for now */
            }
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
          if (fec.type == gmpls_entry_type_tdm)
            {
              /* Nothing for now */
            }
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
        }
    }
  else if (confirm->type == CONFIRM_RSVP_BYPASS_ILM)
    {
      struct ilm_entry *ilm = confirm->data;
      gmpls_ilm_add_entry_to_fwd_tbl (nm, ilm, ftn);
    }
  else if (confirm->type == CONFIRM_MAPPED_LSP)
    {
      struct mapped_lsp_entry_head *entry = confirm->data;

      nsm_gmpls_mapped_ilm_add (nm, ftn, entry,
                                lookup ? SWAP_AND_LOOKUP : SWAP);
    }
#ifdef HAVE_VRF
  else if (confirm->type == CONFIRM_VRF)
    {
      s_int32_t ret;
      struct ptree_node *vrf_rn = confirm->data;
      struct ftn_entry *vrf = vrf_rn->info;
      struct fec_gen_entry fec;

      NSM_ASSERT (vrf != NULL);
      if (! vrf)
        return;

      nsm_gmpls_set_fec_from_ftn (vrf, &fec);
      FTN_NHLFE (vrf)->opcode = (lookup == NSM_TRUE ?
                                 PUSH_AND_LOOKUP :
                                 PUSH);
#ifdef HAVE_IPV6
      if (fec.u.prefix.family == AF_INET6)
        ret = nsm_mpls_ftn6_add_to_fwd (nm, vrf, &fec.u.prefix,
                                       vrf_rn->tree->id, 
                                       lookup == NSM_TRUE ? ftn : NULL,
                                       NSM_FALSE);
      else
#endif
        ret = nsm_mpls_ftn4_add_to_fwd (nm, vrf, &fec.u.prefix,
                                       vrf_rn->tree->id, 
                                       lookup == NSM_TRUE ? ftn : NULL,
                                       NSM_FALSE);
      if (ret == NSM_SUCCESS)
        {
          SET_FLAG (vrf->flags, FTN_ENTRY_FLAG_INSTALLED);
          FTN_XC_OPR_STATUS_COUNT_INC (vrf);
          FTN_XC_STATUS_UPDATE (vrf);
#ifdef HAVE_SNMP
          if (nmpls->notificationcntl == NSM_MPLS_NOTIFICATIONCNTL_ENA &&
              CHECK_FLAG (vrf->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG)) 
            nsm_mpls_opr_sts_trap (nmpls, vrf->xc->key.xc_ix, 
                                   vrf->xc->opr_status);
#endif  /* HAVE_SNMP */
        }
    }
#endif /* HAVE_VRF */
#ifdef HAVE_MPLS_VC
  else if (confirm->type == CONFIRM_VC)
    {
      s_int32_t ret;
      u_char opcode;
      struct nsm_mpls_circuit *vc = confirm->data;
      u_int32_t vc_id = 0;
      struct interface *ifp;
      struct nsm_mpls_if *mif = NULL;
      struct listnode *ln;
      struct ilm_entry *ilm = NULL;
      struct nhlfe_key_gen *nkey;
      s_int32_t iif_ix = 0;

      NSM_ASSERT (vc != NULL);
      if ((! vc) || (!vc->vc_fib))
        {
          zlog_info (NSM_ZG, "%s-%d: VC FIB not present\n", __FUNCTION__,
                     __LINE__);
          return;
        }

      vc_id = vc->id;

      LIST_LOOP (NSM_MPLS->iflist, mif, ln)
        {
          /* Get interface object. */
          ifp = mif->ifp;
#ifdef HAVE_GMPLS
          if ((ifp != NULL) && (ifp->gifindex == vc->vc_fib->ac_if_ix))
#else
          if ((ifp != NULL) && (ifp->ifindex == vc->vc_fib->ac_if_ix))
#endif /* HAVE_GMPLS */
            break;
        }

      if (! mif)
        return;

      if (lookup == NSM_TRUE)
        opcode = PUSH_AND_LOOKUP_FOR_VC;
      else
        opcode = PUSH_FOR_VC;
#ifdef HAVE_MS_PW
      if (vc->ms_pw)
        opcode = SWAP_AND_LOOKUP;
#endif /* HAVE_MS_PW */

      nkey = (struct nhlfe_key_gen *) FTN_NHLFE (ftn)->nkey;
      switch (FTN_NHLFE (ftn)->type)
        {
          case gmpls_entry_type_ip:
            if ((nkey->u.pkt.afi == AFI_IP)
#ifdef HAVE_GMPLS
                 || (nkey->u.pkt.afi == AFI_IP6)
#endif /* HAVE_GMPLS */
                )
              {
                iif_ix = nkey->u.pkt.u.ipv4.oif_ix;
              }
#ifdef HAVE_IPV6
            else
              iif_ix = nkey->u.pkt.u.ipv6.oif_ix;
#endif /* HAVE_IPV6 */
           break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
          case gmpls_entry_type_pbb_te:
            iif_ix = nkey->u.pbb.oif_ix;
            break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
          case gmpls_entry_type_tdm:
            iif_ix = nkey->u.tdm.oif_ix;
            break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

          default:
            break;
        }

      /* Update the VC-FIB and ILM if NHLFE is changed for new FTN */
      if (vc->vc_fib->nw_if_ix != iif_ix)
        {
          ilm = nsm_gmpls_ilm_ix_lookup (nm, vc->ilm_ix);
          
          if (ilm)
            {
              struct ilm_add_gen_data iad;
              struct ilm_gen_ret_data ird;
              struct gmpls_gen_label lbl;

              pal_mem_set (&iad, 0, sizeof (struct ilm_add_gen_data));
              pal_mem_set (&ird, 0, sizeof (struct ilm_gen_ret_data));

              switch (ilm->ent_type)
                {
                  case ILM_ENTRY_TYPE_PACKET:
                    lbl.type = gmpls_entry_type_ip;                   
                    lbl.u.pkt = ilm->key.u.pkt.in_label;
                    nsm_gmpls_ilm_fast_del_msg_process (nm, 
                                                        ilm->key.u.pkt.iif_ix, 
                                                        &lbl, ilm->ilm_ix);
                    iad.iif_ix = nkey->u.pkt.u.ipv4.oif_ix;
                    break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
                  case ILM_ENTRY_TYPE_PBB_TE:
                    lbl.type = gmpls_entry_type_pbb_te;
                    lbl.u.pbb = ilm->key.u.pbb_key.in_label;
                    nsm_gmpls_ilm_fast_del_msg_process (nm, 
                                                        ilm->key.u.pbb_key.iif_ix,
                                                        &lbl, ilm->ilm_ix);
                    iad.iif_ix = nkey->u.pbb.oif_ix;
                    break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
                  case ILM_ENTRY_TYPE_TDM:
                    lbl.type = gmpls_entry_type_tdm;
                    lbl.u.pbb = ilm->key.u.tdm.in_label;
                    nsm_gmpls_ilm_fast_del_msg_process (nm,
                                                        ilm->key.u.tdm.iif_ix,
                                                        &lbl, ilm->ilm_ix);
                    iad.iif_ix = nkey->u.tdm.u.ipv4.oif_ix;

                    break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
                  default:
                    break;
                }

              iad.owner.owner = ftn->owner.owner;
              iad.owner.u.vc_key.vc_id = vc->id;
              IPV4_ADDR_COPY (&iad.owner.u.vc_key.vc_peer, 
                               &vc->address.u.prefix4);
              iad.oif_ix = vc->vc_fib->ac_if_ix;
              iif_ix = iad.iif_ix;

              iad.in_label.type = gmpls_entry_type_ip;
              iad.in_label.u.pkt = vc->vc_fib->in_label;

              iad.out_label.type = gmpls_entry_type_ip;
              iad.out_label.u.pkt = LABEL_VALUE_INVALID;

              iad.n_pops = 1;
              iad.fec_prefix.family = AF_INET;
              iad.fec_prefix.prefixlen = IPV4_MAX_PREFIXLEN;
              iad.fec_prefix.u.prefix4.s_addr = pal_hton32 (vc->id);
              iad.opcode = POP_FOR_VC;
              iad.nh_addr.afi = AFI_IP;
              SET_FLAG (iad.flags, NSM_MSG_VC_ILM_ADD);
 
              ret = nsm_gmpls_ilm_add_msg_process (nm, &iad, NSM_FALSE, &ird, 
                                                   NSM_FALSE);
 
              if (ret == NSM_SUCCESS)
                {
                  vc->xc_ix = ird.xc_ix;
                  vc->ilm_ix = ird.ilm_ix;
                }
            }
          vc->vc_fib->nw_if_ix = iif_ix;
          vc->vc_fib->nw_if_ix_conf = 0;
        }

      UNSET_FLAG (vc->pw_status, NSM_CODE_PW_PSN_EGRESS_TX_FAULT);
      UNSET_FLAG (vc->pw_status, NSM_CODE_PW_PSN_INGRESS_RX_FAULT);
      ret = nsm_mpls_vc_fib_add (nm, vc, vc_id, opcode, ftn);

      if (ret == NSM_SUCCESS)
        {
          vc->state = NSM_MPLS_L2_CIRCUIT_UP;
          vc->last_change = nsm_get_sys_up_time();
          vc->uptime = nsm_get_sys_up_time();
          vc->ftn = ftn;
#ifdef HAVE_SNMP
          nsm_mpls_vc_up_down_notify (PW_L2_STATUS_UP,
                                      vc->vc_snmp_index);
#endif /* HAVE_SNMP */
        }
    }
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
  else if (confirm->type == CONFIRM_VPLS_MESH_VC)
    {
      struct route_node *rn = NULL, *rn_temp = NULL;
      struct nsm_vpls_peer *vp = NULL, *peer = NULL;
      struct nsm_vpls *vpls = NULL;
      struct fec_gen_entry fec;
      struct prefix pfx;
      struct nhlfe_key_gen *key;
      int ret;
      u_char opcode;
      u_int32_t oif_ix = 0;

      if (confirm->data == NULL)
        return;

      rn = confirm->data;
      vp = rn->info;

      if (!vp || !vp->vc_fib)
        return;

      vpls = nsm_vpls_lookup (nm, rn->table->id, NSM_FALSE);
      NSM_ASSERT (vpls != NULL);
      if (vpls)
        {
          /* A secondary peer will not have the fib entry made at this time */
          for (rn_temp = route_top (vpls->mp_table); rn_temp;
               rn_temp = route_next (rn_temp))
             {
               if (! rn_temp->info)
                  continue;
               peer = rn_temp->info;
               if (peer != vp)
                 {
                   if (peer->peer_mode == NSM_MPLS_VC_PRIMARY)
                     {
                       if (peer->state == NSM_VPLS_PEER_ACTIVE &&
                           vp->peer_mode == NSM_MPLS_VC_SECONDARY)
                         return;
                       else if (peer->state == NSM_VPLS_PEER_UP)
                         {
                           if (vp->peer_mode == NSM_MPLS_VC_SECONDARY &&
                              (vp->state < NSM_VPLS_PEER_UP))
                             return;
                         }
                        else
                          break;
                      }
                    else if (peer->peer_mode == NSM_MPLS_VC_SECONDARY)
                      {
                        if ((peer->state == NSM_VPLS_PEER_UP) &&
                            vp->peer_mode == NSM_MPLS_VC_PRIMARY)
                          {
                            mpls_addr_in_to_prefix (&peer->peer_addr, &pfx);
                            NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY (
                                           &fec, &pfx);
                            gmpls_lsp_dep_del (nm, &fec, CONFIRM_VPLS_MESH_VC,
                                               rn_temp);
                            NSM_VPLS_MESH_PEER_FIB_RESET (peer);
#ifdef HAVE_SNMP                            
                            peer->last_change = nsm_get_sys_up_time ();
#endif /*HAVE_SNMP */
                          }
                      }
                  }
                else
                  continue;
             }

          /* Update the Mesh VC-FIB if NHLFE is changed for new FTN */
          key = (struct nhlfe_key_gen *) FTN_NHLFE (ftn)->nkey;
          switch (FTN_NHLFE (ftn)->type)
            {
              case gmpls_entry_type_ip:
                if ((key->u.pkt.afi == AFI_IP)
#ifdef HAVE_GMPLS
                     || (key->u.pkt.afi == AFI_IP6)
#endif /* HAVE_GMPLS */
                    )
                  {
                    oif_ix = key->u.pkt.u.ipv4.oif_ix;
                  }
#ifdef HAVE_IPV6
                else
                  oif_ix = key->u.pkt.u.ipv6.oif_ix;
#endif /* HAVE_IPV6 */
               break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
              case gmpls_entry_type_pbb_te:
                oif_ix = key->u.pbb.oif_ix;
               break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
              case gmpls_entry_type_tdm:
                oif_ix = key->u.tdm.oif_ix;
                break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
              default:
                break;
            }

          if (vp->vc_fib->nw_if_ix != oif_ix)
            vp->vc_fib->nw_if_ix = oif_ix;

          if (lookup == NSM_TRUE)
            opcode = PUSH_AND_LOOKUP_FOR_VC;
          else
            opcode = PUSH_FOR_VC;

          ret = nsm_vpls_fib_add (vpls, vp, opcode, ftn);
          if (ret == 0)
            {
              vp->state = NSM_VPLS_PEER_UP;
#ifdef HAVE_SNMP
              vp->up_time = nsm_get_sys_up_time ();
              vp->last_change = vp->up_time;
              nsm_mpls_vc_up_down_notify (PW_L2_STATUS_UP,
                                          vp->vc_snmp_index);
#endif /*HAVE_SNMP */
              vp->ftn = ftn;             
            }
        }
    }
  else if (confirm->type == CONFIRM_VPLS_SPOKE_VC)
    {
      struct nsm_vpls_spoke_vc *svc = confirm->data, *svc_temp;
      struct nsm_vpls *vpls;
      struct fec_gen_entry fec;
      struct listnode *ln;
      struct nhlfe_key_gen *key;
      u_int32_t oif_ix = 0;
      u_char opcode;

      NSM_ASSERT (svc->vc != NULL);
      if (! svc->vc)
        return;

      vpls = nsm_vpls_lookup (nm, svc->vc->vpls->vpls_id, NSM_FALSE);

      if (lookup == NSM_TRUE)
        opcode = PUSH_AND_LOOKUP_FOR_VC;
      else
        opcode = PUSH_FOR_VC;

      if (! vpls)
        return;

      /* Fib entry for Secondary spoke is not created at this time. */
      LIST_LOOP(vpls->svc_list, svc_temp, ln)
        {
          if (svc_temp == svc)
            continue;
          else if (svc_temp->svc_mode == NSM_MPLS_VC_PRIMARY)
            {
              if ((svc_temp->state > NSM_VPLS_SPOKE_VC_DOWN) &&
                  svc->svc_mode == NSM_MPLS_VC_SECONDARY)
                return;
              else if (svc_temp->state == NSM_VPLS_SPOKE_VC_DOWN)
                break;
            }
          else if (svc_temp->svc_mode == NSM_MPLS_VC_SECONDARY)
            {
              if ((svc_temp->state == NSM_VPLS_SPOKE_VC_UP) &&
                  svc->svc_mode == NSM_MPLS_VC_PRIMARY)
                {
                  fec.type = gmpls_entry_type_ip;
                  fec.u.prefix = svc_temp->vc->address;
                  gmpls_lsp_dep_del (nm, &fec, CONFIRM_VPLS_SPOKE_VC, svc_temp);
                  NSM_VPLS_SPOKE_VC_FIB_RESET (svc);
                }
            }
        }

      key = (struct nhlfe_key_gen *) FTN_NHLFE (ftn)->nkey;
      switch (FTN_NHLFE (ftn)->type)
        {
          case gmpls_entry_type_ip:
            if ((key->u.pkt.afi == AFI_IP)
#ifdef HAVE_GMPLS
                || (key->u.pkt.afi == AFI_IP6)
#endif /* HAVE_GMPLS */
               )
              {
                oif_ix = key->u.pkt.u.ipv4.oif_ix;
              }
#ifdef HAVE_IPV6
            else
              oif_ix = key->u.pkt.u.ipv6.oif_ix;
#endif /* HAVE_IPV6 */
             break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
          case gmpls_entry_type_pbb_te:
            oif_ix = key->u.pbb.oif_ix;
           break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
          case gmpls_entry_type_tdm:
            oif_ix = key->u.tdm.oif_ix;
            break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
          default:
            break;
        }

      /* Update the VC-FIB if NHLFE is changed for new FTN */
      if (svc->vc_fib->nw_if_ix != oif_ix)
        svc->vc_fib->nw_if_ix = oif_ix;

      nsm_vpls_spoke_vc_fib_add (svc, opcode, ftn);
      svc->state = NSM_VPLS_SPOKE_VC_UP;
      svc->vc->ftn = ftn;
    }
#endif /* HAVE_VPLS  */
}

static void
mpls_confirm_data_delete_from_fwd (struct nsm_master *nm,
                                   struct lsp_dep_confirm *confirm,
                                   struct ftn_entry *ftn,
                                   bool_t lookup)
{
#ifdef HAVE_VRF
#ifdef HAVE_SNMP
  struct nsm_mpls *nmpls = NULL;

  nmpls = nm->nmpls;
#endif  /* HAVE_SNMP */
#endif  /* HAVE_VRF */

  if (! confirm)
    return;

  if (confirm->type == CONFIRM_MAPPED_ROUTE)
    {
      struct ptree_node *mapped_pn = confirm->data;
      struct mapped_route *route = mapped_pn->info;
      struct fec_gen_entry fec;

      if (! ftn)
        return;

      nsm_gmpls_set_fec_from_mapped_route (mapped_pn, &fec);
      nsm_gmpls_map_route_del (nm, ftn, route, &fec,
                               MPLS_FTN_MAPPED, 0);
    }

  if (confirm->type == CONFIRM_RSVP_MAPPED_ROUTE ||
      confirm->type == CONFIRM_IGP_SHORTCUT_ROUTE)
    {
      struct ptree_node *pn = NULL;
      struct ftn_entry *map_ftn = confirm->data;
      struct fec_gen_entry fec;
      s_int32_t ret; 

      pn = map_ftn->pn;
      if (pn)
        {
          nsm_gmpls_set_fec_from_ftn (map_ftn, &fec);
          switch (map_ftn->ent_type)
            {
#ifdef HAVE_IPV6
              case FTN_ENTRY_TYPE_IPV6:
                if (fec.u.prefix.family == AF_INET6)
                  {
                    ret = nsm_mpls_ftn6_del_from_fwd (nm, confirm->data, 
                                                      &fec.u.prefix, 
                                                      pn->tree->id, NULL); 
                   }
                 break;
#endif
              case FTN_ENTRY_TYPE_IPV4:
                if (fec.u.prefix.family == AF_INET)
                  {
                    ret = nsm_mpls_ftn4_del_from_fwd (nm, confirm->data, 
                                                      &fec.u.prefix, 
                                                      pn->tree->id, NULL);
                  }
                break; 

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
               case FTN_ENTRY_TYPE_PBB_TE:
                 break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
               case FTN_ENTRY_TYPE_TDM:
                 break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
               default:
                 break;
             }

          UNSET_FLAG (map_ftn->flags, FTN_ENTRY_FLAG_INSTALLED);
          FTN_XC_OPR_STATUS_COUNT_DEC (map_ftn);
          FTN_XC_STATUS_UPDATE (map_ftn);
        }
    }
  else if (confirm->type == CONFIRM_RSVP_BYPASS_ILM)
    {
      struct ilm_entry *ilm = confirm->data;

      gmpls_ilm_del_entry_from_fwd_tbl (nm, ilm);
    }
  else if (confirm->type == CONFIRM_MAPPED_LSP)
    {
      struct ptree_node *pn = confirm->data;
    
      if (! ftn)
        return;

      nsm_gmpls_mapped_ilm_del (nm, ftn, pn);
    }
#ifdef HAVE_VRF
  else if (confirm->type == CONFIRM_VRF)
    {
      struct ptree_node *vrf_rn = confirm->data;
      struct ftn_entry *vrf = vrf_rn->info;
      struct fec_gen_entry fec;
  
      if (! ftn)
        return;

      NSM_ASSERT (vrf != NULL);
      if (vrf)
        {
          nsm_gmpls_set_fec_from_ftn (vrf, &fec);
          gmpls_ftn_del_from_fwd (nm, vrf, &fec, vrf_rn->tree->id, 
                                 lookup == NSM_TRUE ? ftn : NULL);
          FTN_XC_STATUS_UPDATE (vrf);
#ifdef HAVE_SNMP
          if (nmpls->notificationcntl == NSM_MPLS_NOTIFICATIONCNTL_ENA &&
              CHECK_FLAG (vrf->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG)) 
            nsm_mpls_opr_sts_trap (nmpls, vrf->xc->key.xc_ix, 
                                   vrf->xc->opr_status);
#endif  /* HAVE_SNMP */
          if (FTN_NHLFE(vrf)->opcode == PUSH_AND_LOOKUP)
            FTN_NHLFE(vrf)->opcode = PUSH;
        }
    }
#endif /* HAVE_VRF */
#ifdef HAVE_MPLS_VC
  else if (confirm->type == CONFIRM_VC)
    {
      struct nsm_mpls_circuit *vc = confirm->data;
      struct nsm_mpls_circuit *s_vc = NULL;
      u_int32_t vc_id = vc->id;
      NSM_ASSERT (vc->vc_fib != NULL);

#ifdef HAVE_VLAN
      if(((vc->vc_info->vlan_id == 0) ||
         (nsm_is_vlan_exist(vc->vc_info->mif->ifp,
          vc->vc_info->vlan_id) == NSM_SUCCESS)))
#endif /* HAVE_VLAN */
      if (vc->vc_fib->install_flag == NSM_TRUE)
        {
          nsm_mpls_vc_fib_del (nm, vc, vc_id);
        }
      vc->state = NSM_MPLS_L2_CIRCUIT_COMPLETE;
      vc->vc_fib->nw_if_ix_conf = 0;
      vc->last_change = nsm_get_sys_up_time();
#ifdef HAVE_SNMP
      nsm_mpls_vc_up_down_notify (PW_L2_STATUS_DOWN,
                                  vc->vc_snmp_index);
#endif /* HAVE_SNMP */
      vc->ftn = NULL;
      if (vc->vc_info->sibling_info)
        {
          if ((s_vc = vc->vc_info->sibling_info->u.vc) &&
              (nsm_mpls_vc_revert (s_vc, NSM_FALSE)))
            (void) nsm_mpls_vc_fib_add (nm, s_vc, s_vc->id, 
                                        s_vc->vc_fib->opcode, s_vc->ftn);
        }
    }
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
  else if (confirm->type == CONFIRM_VPLS_MESH_VC)
    {
      struct route_node *rn = confirm->data, *rn_temp;
      struct nsm_vpls_peer *vp = rn->info, *peer;
      struct nsm_vpls *vpls;
      struct prefix pfx;
      struct fec_gen_entry fec;

      vpls = nsm_vpls_lookup (nm, rn->table->id, NSM_FALSE);
      NSM_ASSERT (vpls != NULL);
      if (vpls && vp->state == NSM_VPLS_PEER_UP)
        {
          if (vp->peer_mode == NSM_MPLS_VC_PRIMARY)
            {
              for (rn_temp = route_top (vpls->mp_table); rn_temp;
                  rn_temp = route_next (rn_temp))
                 {
                   if (! rn_temp->info)
                     continue;
                   peer = rn_temp->info;
                   if ((peer != vp) && peer->peer_mode == NSM_MPLS_VC_SECONDARY)
                     {
                       if (peer->state == NSM_VPLS_PEER_UP)
                         {
                           zlog_warn (NSM_ZG, "WARNING: SECONDARY FIB Entry"
                                      " is Already Active");
                           nsm_vpls_fib_delete (vpls, vp);
                           vp->state = NSM_VPLS_PEER_ACTIVE;
#ifdef HAVE_SNMP
                           vp->last_change = nsm_get_sys_up_time ();
#endif /*HAVE_SNMP */
                           vp->ftn = NULL;
                           return;
                         }
                       else if (peer->state < NSM_VPLS_PEER_UP)
                         {
                           nsm_vpls_fib_delete (vpls, vp);
                           vp->state = NSM_VPLS_PEER_ACTIVE;
#ifdef HAVE_SNMP
                           vp->last_change = nsm_get_sys_up_time ();
#endif /*HAVE_SNMP */
                           vp->ftn = NULL;
                           peer->state= NSM_VPLS_PEER_ACTIVE;
#ifdef HAVE_SNMP
                           peer->last_change = nsm_get_sys_up_time ();
#endif /*HAVE_SNMP */
                           mpls_addr_in_to_prefix (&peer->peer_addr, &pfx);
                           NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY (&fec, &pfx);
                           gmpls_lsp_dep_add (nm, &fec, CONFIRM_VPLS_MESH_VC,
                                              rn_temp);
                           return;
                         }
                     }
                 }
            }

          nsm_vpls_fib_delete (vpls, vp);
          vp->state = NSM_VPLS_PEER_ACTIVE;
#ifdef HAVE_SNMP
          vp->last_change = nsm_get_sys_up_time ();
          nsm_mpls_vc_up_down_notify (PW_L2_STATUS_DOWN,
                                      vp->vc_snmp_index);
#endif /*HAVE_SNMP */
          vp->ftn = NULL;
        }
    }
  else if (confirm->type == CONFIRM_VPLS_SPOKE_VC)
    {
      struct nsm_vpls_spoke_vc *svc = confirm->data;

      if (svc->state ==  NSM_VPLS_SPOKE_VC_UP) /*NSM_VPLS_PEER_UP */
        {
          nsm_vpls_spoke_vc_fib_delete (svc);
          svc->state = NSM_VPLS_SPOKE_VC_ACTIVE;
          svc->vc->ftn = NULL;
        }
    }
#endif /* HAVE_VPLS */
}

static void
gmpls_lsp_dep_up_update (struct nsm_master *nm,
                        struct route_table *table, struct ftn_entry *ftn,
                        struct prefix *p)
{
  struct route_node *rn, *rn_head;
  struct confirm_list *c_list;
  struct lsp_dep_confirm *confirm;
  bool_t lookup = NSM_FALSE;
  bool_t pfx_same = NSM_FALSE;
#ifdef HAVE_MPLS_VC
  struct nsm_mpls_circuit *vc = NULL;
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
  struct route_node *rn_temp_vpls = NULL;
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls_spoke_vc *svc = NULL;
#endif /* HAVE_VPLS */
  s_int32_t ret;

  ret = gmpls_ftn_get_lookup_flag (ftn, &lookup);
  if (ret < 0)
    return;

  rn_head = route_node_get (table, p);
  if (! rn_head)
    return;

  route_lock_node (rn_head);
  for (rn = rn_head; rn; rn = route_next_until (rn, rn_head)) 
    {
      if (! rn->info)
        continue;

      c_list = rn->info;

      if (prefix_same (&rn->p, p))
        pfx_same = NSM_TRUE;

      if (c_list->type == CONFIRM_NODE_FTN)
        {
          if (! pfx_same)
            continue;
        }
      else
        {
          if (pfx_same)
            c_list->type = CONFIRM_NODE_FTN;
          else if (c_list->p.prefixlen > p->prefixlen)
            continue;

          pal_mem_cpy (&c_list->p, p, sizeof (struct prefix));
        }

      /* Go through confirm list and modify fib entries */
      for (confirm = c_list->head; confirm; confirm = confirm->next)
        {
#ifdef HAVE_MPLS_VC
          if (confirm->type == CONFIRM_VC)
            {
              vc = confirm->data;
              if (NSM_MPLS_VC_NOT_MAP_TO_FTN (ftn, vc))
                continue;
            }
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
          /* Check if the ftn entry is binded to any VPLS peer. */
          else if (confirm->type == CONFIRM_VPLS_MESH_VC)
            {
              rn_temp_vpls = confirm->data;
              if (rn_temp_vpls == NULL)
                continue;

              vpls_peer = rn_temp_vpls->info;
              if ((vpls_peer == NULL) ||
                  (NSM_VPLS_ENTRY_NOT_MAP_TO_FTN (ftn, vpls_peer)))
                continue;
            }
          /* Check if the ftn entry is binded to any VPLS peer. */
          else if (confirm->type == CONFIRM_VPLS_SPOKE_VC)
            {
              svc = confirm->data;
              if (svc == NULL || svc->vc == NULL)
                continue;

              if (NSM_VPLS_ENTRY_NOT_MAP_TO_FTN (ftn, svc->vc))
                continue;
            }
#endif /* HAVE_VPLS*/
          gmpls_confirm_data_add_to_fwd (nm, confirm, ftn, lookup);
        }
    }
  route_unlock_node (rn_head);
}


static void
gmpls_lsp_dep_down_update (struct nsm_master *nm,
                          struct route_table *down_table, 
                          struct route_table *up_table,
                          struct ftn_entry *ftn,
                          struct prefix *fec_prefix)
{
  struct route_node *rn, *rn_head;
  struct confirm_list *c_list;
  struct lsp_dep_confirm *confirm, *confirm_next;
#ifdef HAVE_MPLS_VC
  struct nsm_mpls_circuit *vc = NULL;
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
  struct route_node *rn_temp_vpls = NULL;
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls_spoke_vc *svc = NULL;
#endif /* HAVE_VPLS */
  bool_t lookup;
  s_int32_t ret;

  ret = gmpls_ftn_get_lookup_flag (ftn, &lookup);
  if (ret < 0)
    return;

  /* Get correct FTN entry node. */
  rn_head = route_node_get (down_table, fec_prefix);
  if (! rn_head)
    return;

  route_lock_node (rn_head);
  for (rn = rn_head; rn; rn = route_next_until (rn, rn_head))
    {
      c_list = rn->info;
      if (! c_list) 
        continue;

      /* Go through confirm list and add */
      for (confirm = c_list->head; confirm; confirm = confirm_next)
        {
          confirm_next = confirm->next;
#ifdef HAVE_MPLS_VC
          if (confirm->type == CONFIRM_VC)
            {
              vc = confirm->data;
              if (NSM_MPLS_VC_NOT_MAP_TO_FTN (ftn, vc))
                continue;

              /* No outer tunnel configured */
              if ((! vc->owner) && (! vc->ftn))
                {
                  if (ftn->owner.owner == MPLS_LDP)
                    vc->dn = PSNBOUND;
                  vc->ftn = ftn;
                }     
              /* Protocol Specific Outer Tunnel */
              else if ((vc->owner) && (!vc->ftn) &&
                      (vc->owner == ftn->owner.owner))
                {
                  if (ftn->owner.owner == MPLS_LDP)
                    vc->dn = PSNBOUND;
                  vc->ftn = ftn;
                }
              else if (vc->fec_type_vc == PW_OWNER_MANUAL && (!vc->ftn))
                {
                  vc->ftn = ftn;
                }
              else
                continue;
            }
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
          /* Check if the ftn entry is required by any VPLS peer. */
          if (confirm->type == CONFIRM_VPLS_MESH_VC)
            {
              rn_temp_vpls = confirm->data;

              if (rn_temp_vpls == NULL)
                continue;

              vpls_peer = rn_temp_vpls->info;

              if ((vpls_peer == NULL) ||
                  (NSM_VPLS_ENTRY_NOT_MAP_TO_FTN (ftn, vpls_peer)))
                continue;

              if (!vpls_peer->ftn)
                vpls_peer->ftn = ftn;
              else
                continue;
            }
          else if (confirm->type == CONFIRM_VPLS_SPOKE_VC)
            {
              svc = confirm->data;

              if (svc == NULL || svc->vc == NULL)
                continue;

              if (NSM_VPLS_ENTRY_NOT_MAP_TO_FTN (ftn, svc->vc))
                continue;

              if (!svc->vc->ftn)
                svc->vc->ftn = ftn;
              else
                continue;
            }
#endif /* HAVE_VPLS */
          gmpls_confirm_data_add_to_fwd (nm, confirm, ftn, lookup);
          mpls_confirm_entry_move (confirm, c_list, up_table, 
                                   CONFIRM_NODE_FTN, fec_prefix);
        }

      if (c_list->count == 0)
        {
          mpls_confirm_list_free (c_list);
          rn->info = NULL;
          route_unlock_node (rn);
        }
    }
  route_unlock_node (rn_head);
}

struct ftn_entry *
gmpls_lsp_dep_ftn_down_select_next (struct nsm_master *nm, 
                                    struct ftn_entry *ftn, 
                                    struct ptree_node *pn)
{
  mpls_lsp_priority_t ftn_next_pri, ftn_pri;
  struct fec_gen_entry fec;
  struct ftn_entry *ftn_next, *ftn_head, *ret = NULL;
 
  ftn_head = gmpls_ftn_active_head ((struct ftn_entry *)pn->info);
  nsm_gmpls_set_fec_from_ftn (ftn, &fec);
  ftn_next = ftn->next;
  if (! ftn_next)
    {
      if (ftn->igp_status == FTN_ENTRY_IGP_STATUS_ADVERTISED)
        {
          nsm_mpls_send_igp_shortcut_lsp (ftn, &fec.u.prefix,
                                          NSM_MSG_IGP_SHORTCUT_DELETE);
          ftn->igp_status = FTN_ENTRY_IGP_STATUS_NONE;
        }

      return NULL;
    }

  if (! pn->info)
    return NULL;

  ftn_next_pri = gmpls_owner_priority (ftn_next->owner.owner);
  ftn_pri = gmpls_owner_priority (ftn->owner.owner);

  if (ftn_pri == ftn_next_pri)
    {
      /* if have secondary/backup with same tunnel-id, 
         move dep list to it */
      if ((ftn_next->tun_id == ftn->tun_id)
#ifdef HAVE_GMPLS
           && ((ftn_next->flags & FTN_ENTRY_FLAG_BIDIR) == 
               (ftn->flags & FTN_ENTRY_FLAG_BIDIR))
           && ((ftn_next->flags & FTN_ENTRY_FLAG_FWD) == 
               (ftn->flags & FTN_ENTRY_FLAG_FWD))
#endif /* HAVE_GMPLS */
         )
        {
          gmpls_ftn_entry_select (nm, ftn_next, pn);

          /* send igp-shortcut lsp update to OSPF if metric 
             is not same, else do nothing */
          if (fec.type == gmpls_entry_type_ip)
            {
              if (ftn->lsp_bits.bits.lsp_metric !=
                  ftn_next->lsp_bits.bits.lsp_metric)
                nsm_mpls_send_igp_shortcut_lsp (ftn_next, &fec.u.prefix,
                                                NSM_MSG_IGP_SHORTCUT_UPDATE);
            }

          ftn->igp_status = FTN_ENTRY_IGP_STATUS_NONE;
          ftn_next->igp_status = FTN_ENTRY_IGP_STATUS_ADVERTISED;
          ret = ftn_next;
          return ret;
        }

      /* if next ftn does not have the same trunk-id, delete rsvp-mapped-route
         and igp-shortcut route, leave static mapped route to map to it later */
      if ((ftn == ftn_head || ftn->tun_id != ftn_head->tun_id
#ifdef HAVE_GMPLS
          || ((ftn_head->flags & FTN_ENTRY_FLAG_BIDIR) != 
           (ftn->flags & FTN_ENTRY_FLAG_BIDIR))
          || ((ftn_head->flags & FTN_ENTRY_FLAG_FWD) != 
               (ftn->flags & FTN_ENTRY_FLAG_FWD))
#endif /* HAVE_GMPLS */
          ) && ! CHECK_FLAG (ftn_next->flags, FTN_ENTRY_FLAG_DEPENDENT) &&
          ftn_next->ftn_type == MPLS_FTN_REGULAR &&
          CHECK_FLAG (ftn_next->flags, FTN_ENTRY_FLAG_INSTALLED))
        ret = ftn_next;
    }

  if (ftn->igp_status == FTN_ENTRY_IGP_STATUS_ADVERTISED)
    {
      nsm_mpls_send_igp_shortcut_lsp (ftn, &fec.u.prefix,
                                      NSM_MSG_IGP_SHORTCUT_DELETE);
      ftn->igp_status = FTN_ENTRY_IGP_STATUS_NONE;
    }

  return ret;
}

void
gmpls_dep_ftn_update_process (struct nsm_master *nm,
                              struct ftn_entry *ftn_next,
                              struct ftn_entry *ftn_sel,
                              struct ftn_entry *ftn_old,
                              struct lsp_dep_confirm *confirm,
                              struct confirm_list *c_list,
                              struct route_node *rn_list,
                              bool_t del_lookup)
{
  struct ftn_entry *ftn_dep = NULL, *ftn_tmp = NULL;
  struct ptree_node *rn_dep;
  struct ptree_ix_table *pix_tbl;
  struct fec_gen_entry fec;
  struct prefix p;
  bool_t update_xc = NSM_FALSE;
#ifdef HAVE_MPLS_VC
  u_int8_t tunnel_dir = 0;
#endif /* HAVE_MPLS_VC */
  s_int32_t ret;
#ifdef HAVE_VRF
#ifdef HAVE_SNMP
  struct nsm_mpls *nmpls = NULL;
  nmpls = nm->nmpls;
#endif  /* HAVE_SNMP */
#endif  /* HAVE_VRF */

  if (! confirm || ! ftn_old)
    return;

  if (confirm->type == CONFIRM_IGP_SHORTCUT_ROUTE ||
      confirm->type == CONFIRM_RSVP_MAPPED_ROUTE)
    {
      ftn_dep = confirm->data;
      rn_dep = ftn_dep->pn;
      nsm_gmpls_set_fec_from_ftn (ftn_dep, &fec);

      pal_mem_cpy (&p, &fec.u.prefix, sizeof (struct prefix));

      /* If this ftn_dep is not map to deselected ftn */
      if (ftn_dep->xc != ftn_old->xc)
        return;

      /* If there is no new ftn selected, delete dep ftn entry */
      if (! ftn_next || ftn_next->tun_id != ftn_dep->tun_id
#ifdef HAVE_GMPLS
          || ((ftn_next->flags & FTN_ENTRY_FLAG_BIDIR) !=
           (ftn_dep->flags & FTN_ENTRY_FLAG_BIDIR))
          || ((ftn_next->flags & FTN_ENTRY_FLAG_FWD) !=
               (ftn_dep->flags & FTN_ENTRY_FLAG_FWD))
#endif /* HAVE_GMPLS */
          )
        {
          /* If it is not selected, need also delete from comfirm list */
          if (! CHECK_FLAG (ftn_dep->flags, FTN_ENTRY_FLAG_SELECTED))
            {
              mpls_confirm_list_remove (c_list, confirm);
              XFREE (MTYPE_NSM_LSP_DEP_CONFIRM, confirm);
              confirm = NULL;

              /* cleanup list and tree */
              if (c_list->count == 0)
                {
                  mpls_confirm_list_free (c_list);
                  rn_list->info = NULL;
                  route_unlock_node (rn_list);
                  c_list = NULL;
                }

              ftn_list_delete_node ((struct ftn_entry **)&rn_dep->info,
                                    ftn_dep);
            } 
          else
            gmpls_ftn_entry_remove_list (nm, 
                                         (struct ftn_entry **)&rn_dep->info,
                                         ftn_dep->ftn_ix, rn_dep, NSM_TRUE);

          if (rn_dep->info == NULL)
            ptree_unlock_node (rn_dep);
          
          pix_tbl = nsm_gmpls_ftn_table_lookup (nm, 0, &fec);

          if (pix_tbl)
            gmpls_ftn_entry_cleanup (nm, ftn_dep, &fec, pix_tbl);

          gmpls_ftn_free (ftn_dep);

          return;
        }
      /* map rsvp-map-route|igp-shortcut-route to new selected ftn, 
         update xc */
      else 
        update_xc = NSM_TRUE;
    }
  else if (confirm->type == CONFIRM_MAPPED_ROUTE)
    {
      struct ptree_node *mapped_rn = confirm->data;
      struct ptree_node *rn_dep;
      struct mapped_route *route = mapped_rn->info;
      struct ptree_ix_table *tbl;
      struct fec_gen_entry fec, rfec;

      nsm_gmpls_set_fec_from_mapped_route (mapped_rn, &fec);
      tbl = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, &fec);
      if (! tbl)
        return;

      ftn_dep = nsm_gmpls_ftn_lookup (nm, tbl, &fec, route->ix, NSM_FALSE);
      if (! ftn_dep)
        return;

      rn_dep = ftn_dep->pn;

      if (ftn_dep->xc != ftn_old->xc)
        return;

      if (! ftn_next && ! ftn_sel)
        {
          if (CHECK_FLAG (ftn_dep->flags, FTN_ENTRY_FLAG_SELECTED))
            {
              UNSET_FLAG (ftn_dep->flags, FTN_ENTRY_FLAG_SELECTED);
              FTN_XC_ADMN_STATUS_COUNT_DEC (ftn_dep);

              nsm_gmpls_set_fec_from_ftn (ftn_dep, &rfec);
              gmpls_ftn_entry_cleanup (nm, ftn_dep, &rfec, tbl);
              ftn_list_delete_node ((struct ftn_entry **)&rn_dep->info,
                                    ftn_dep);
              gmpls_ftn_delete_select_update (nm, 
                        gmpls_owner_priority (ftn_dep->owner.owner), rn_dep);

              gmpls_ftn_free (ftn_dep);

              if (rn_dep->info == NULL)
                ptree_unlock_node (rn_dep);

              route->ix = 0;
            }
        }
      else
        update_xc = NSM_TRUE;
    }
  else if (confirm->type == CONFIRM_MAPPED_LSP)
    {
      struct ptree_node *pn = confirm->data;
      struct ftn_entry *ftn_t = NULL;
      bool_t lookup = NSM_FALSE;

      nsm_gmpls_mapped_ilm_del (nm, ftn_old, pn);

      if (ftn_next)
        ftn_t = ftn_next;
      else if (ftn_sel)
        ftn_t = ftn_sel;

      if (ftn_t)
        {
          gmpls_ftn_get_lookup_flag (ftn_t, &lookup);

          nsm_gmpls_mapped_lsp_ilm_add (nm, ftn_t, pn, lookup ? 
                                        SWAP_AND_LOOKUP : SWAP);
        }
    }
#ifdef HAVE_VRF
  else if (confirm->type == CONFIRM_VRF)
    {
      struct ptree_node *vrf_rn = confirm->data;
      struct ftn_entry *vrf = vrf_rn->info;

      NSM_ASSERT (vrf != NULL);

      if (ftn_next || ftn_sel)
        update_xc = NSM_TRUE;   
      else if (vrf)
        {
          gmpls_ftn_del_from_fwd (nm, vrf, &fec, vrf_rn->tree->id,
                                  del_lookup == NSM_TRUE ? ftn_old : NULL);
          FTN_XC_STATUS_UPDATE (vrf);
#ifdef HAVE_SNMP
          if (nmpls->notificationcntl == NSM_MPLS_NOTIFICATIONCNTL_ENA &&
              CHECK_FLAG (vrf->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG))
            nsm_mpls_opr_sts_trap (nmpls, vrf->xc->key.xc_ix,
                                   vrf->xc->opr_status);
#endif  /* HAVE_SNMP */
          if (FTN_NHLFE(vrf)->opcode == PUSH_AND_LOOKUP)
            FTN_NHLFE(vrf)->opcode = PUSH;
        }
    }
#endif /* HAVE_VRF */
#ifdef HAVE_MPLS_VC
  else if (confirm->type == CONFIRM_VC)
    {
      struct nsm_mpls_circuit *vc = confirm->data;
      struct nsm_mpls_circuit *s_vc = NULL;
      struct ftn_entry *ftn_t = NULL;
      bool_t lookup = NSM_FALSE;
      u_int32_t vc_id = vc->id;
      u_char opcode;
      struct ilm_entry *ilm = NULL;
      struct route_table *down_table = NSM_MPLS->lsp_dep_down;
      struct nhlfe_key_gen *key;
      struct gmpls_gen_label lbl;
      u_int32_t iif_ix = 0, oif_ix = 0;

      SET_FLAG (vc->pw_status, NSM_CODE_PW_PSN_EGRESS_TX_FAULT);
#ifdef HAVE_MS_PW
      /* NSM_CODE_PW_PSN_EGRESS_TX_FAULT on a vc translates to
       * NSM_CODE_PW_PSN_INGRESS_RX_FAULT on the other
       */
      if ((vc->ms_pw) && (vc->vc_other))
        SET_FLAG (vc->vc_other->pw_status, NSM_CODE_PW_PSN_INGRESS_RX_FAULT);
#endif /* HAVE_MS_PW */

      if (vc->vc_fib->install_flag == NSM_TRUE)
        {
          nsm_mpls_vc_fib_del (nm, vc, vc_id);
          vc->state = NSM_MPLS_L2_CIRCUIT_COMPLETE;
          vc->vc_fib->nw_if_ix_conf = 0;
          vc->ftn = NULL;
#ifdef HAVE_SNMP
          nsm_mpls_vc_up_down_notify (PW_L2_STATUS_DOWN,
                                      vc->vc_snmp_index);
#endif /* HAVE_SNMP */
        }

      if (! ftn_next && !ftn_sel)
        {
          vc->ftn = NULL;
          if (vc->vc_info->sibling_info)
            {
              if ((s_vc = vc->vc_info->sibling_info->u.vc) &&
                  (nsm_mpls_vc_revert (s_vc, NSM_FALSE)))
                (void) nsm_mpls_vc_fib_add (nm, s_vc, s_vc->id, 
                                            s_vc->vc_fib->opcode, s_vc->ftn);
            }
        }
      else
        {
          if (ftn_next)
            ftn_t = ftn_next;
          else
            ftn_t = ftn_sel;

          if (CHECK_FLAG (ftn_t->flags, FTN_ENTRY_FLAG_FWD))
            tunnel_dir = TUNNEL_DIR_FWD;
          else if (CHECK_FLAG (ftn_t->flags, FTN_ENTRY_FLAG_BIDIR))
            tunnel_dir = TUNNEL_DIR_REV;

          if (ftn_t && 
              (vc->tunnel_id == 0 || 
              (ftn_t->tun_id == vc->tunnel_id && 
               vc->tunnel_dir == tunnel_dir)))
            {
	      gmpls_ftn_get_lookup_flag (ftn_t, &lookup);
	      if (lookup == NSM_TRUE)
		opcode = PUSH_AND_LOOKUP_FOR_VC;
	      else
		opcode = PUSH_FOR_VC;

              ilm = nsm_gmpls_ilm_ix_lookup (nm, vc->ilm_ix);
              key = (struct nhlfe_key_gen *) FTN_NHLFE (ftn_t)->nkey;
              switch (FTN_NHLFE (ftn_t)->type)
                {
                  case gmpls_entry_type_ip:
                    if ((key->u.pkt.afi == AFI_IP)
#ifdef HAVE_GMPLS
                        || (key->u.pkt.afi == AFI_UNNUMBERED)
#endif /* HAVE_GMPLS */
                       )
                      {
                        oif_ix = key->u.pkt.u.ipv4.oif_ix;
                      }
#ifdef HAVE_IPV6
                    else
                      {
                        oif_ix = key->u.pkt.u.ipv6.oif_ix;
                      }
#endif /* HAVE_IPV6 */
                    if (ilm)
                      {
                        iif_ix = ilm->key.u.pkt.iif_ix;
                        lbl.type = gmpls_entry_type_ip;
                        lbl.u.pkt = ilm->key.u.pkt.in_label;
                      }

                    break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
                  case gmpls_entry_type_pbb_te:
                    oif_ix = key->u.pbb.oif_ix;
                    if (ilm)
                      {
                        iif_ix = ilm->key.u.pbb_key.iif_ix;
                        lbl.type = gmpls_entry_type_pbb_te;
                        lbl.u.pbb = ilm->key.u.pbb_key.in_label;
                      }
                    break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
                  case gmpls_entry_type_tdm:
                    oif_ix = key->u.tdm.oif_ix;
                    if (ilm)
                      { 
                        iif_ix = ilm->key.u.tdm.iif_ix;
                        lbl.type = gmpls_entry_type_tdm;
                        lbl.u.pbb = ilm->key.u.tdm.in_label;
                      }
                    break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
                  default:
                    break;
                }

	      /* Update the VC-FIB and ILM if NHLFE is changed for new FTN */
	      if (vc->vc_fib->nw_if_ix != oif_ix)
		{
		  ilm = nsm_gmpls_ilm_ix_lookup (nm, vc->ilm_ix);
 
		  if (ilm)
		    {
		      struct ilm_add_gen_data iad;
		      struct ilm_gen_ret_data ird;
  
		      /* This will also delete the entry from xc table */
		      nsm_gmpls_ilm_fast_del_msg_process (nm, iif_ix, &lbl,
							  ilm->ilm_ix);
 
		      pal_mem_set (&iad, 0, sizeof (struct ilm_add_gen_data));
		      pal_mem_set (&ird, 0, sizeof (struct ilm_gen_ret_data));
 
		      iad.owner.owner = ftn_t->owner.owner;
		      iad.owner.u.vc_key.vc_id = vc->id;
		      IPV4_ADDR_COPY (&iad.owner.u.vc_key.vc_peer, 
                                      &vc->address.u.prefix4);
		      iad.iif_ix = oif_ix;
		      iad.oif_ix = vc->vc_fib->ac_if_ix;
                      iad.in_label.type = gmpls_entry_type_ip;
		      iad.in_label.u.pkt = vc->vc_fib->in_label;
                      iad.out_label.type = gmpls_entry_type_ip;
		      iad.out_label.u.pkt = LABEL_VALUE_INVALID;
		      iad.n_pops = 1;
                      iad.fec_prefix.family = AF_INET;
		      iad.fec_prefix.prefixlen = IPV4_MAX_PREFIXLEN;
		      iad.fec_prefix.u.prefix4.s_addr = pal_hton32 (vc->id);

		      iad.opcode = POP_FOR_VC;
		      iad.nh_addr.afi = AFI_IP;
		      SET_FLAG (iad.flags, NSM_MSG_VC_ILM_ADD);
		      /* dummy ilm entry creation for SNMP */
		      ret = nsm_gmpls_ilm_add_msg_process (nm, &iad, 
                                                           NSM_FALSE, 
                                                           &ird, NSM_FALSE);
 
		      if (ret == NSM_SUCCESS)
			{
			  vc->xc_ix = ird.xc_ix;
			  vc->ilm_ix = ird.ilm_ix;
			}
		    }
		  vc->vc_fib->nw_if_ix_conf = vc->vc_fib->nw_if_ix;
		  vc->vc_fib->nw_if_ix = oif_ix;
		}

              UNSET_FLAG (vc->pw_status, NSM_CODE_PW_PSN_EGRESS_TX_FAULT);
#ifdef HAVE_MS_PW
              /* NSM_CODE_PW_PSN_EGRESS_TX_FAULT on a vc translates to
               * NSM_CODE_PW_PSN_INGRESS_RX_FAULT on the other
               */
              if ((vc->ms_pw) && (vc->vc_other))
                UNSET_FLAG (vc->vc_other->pw_status, 
                            NSM_CODE_PW_PSN_INGRESS_RX_FAULT);
#endif /* HAVE_MS_PW */

	      ret = nsm_mpls_vc_fib_add (nm, vc, vc_id, opcode, ftn_t);
	      if (ret == NSM_SUCCESS)
		{
		  vc->state = NSM_MPLS_L2_CIRCUIT_UP;
#ifdef HAVE_SNMP
		  vc->uptime = nsm_get_sys_up_time();
#endif /*HAVE_SNMP */
		  vc->ftn = ftn_t;
#ifdef HAVE_SNMP
		  nsm_mpls_vc_up_down_notify (PW_L2_STATUS_UP,
                                              vc->vc_snmp_index);
#endif /* HAVE_SNMP */
		}
	      else
		{
		  vc->ftn = NULL;
#ifdef HAVE_SNMP
		  nsm_mpls_vc_up_down_notify (PW_L2_STATUS_DOWN,
                                              vc->vc_snmp_index);
#endif /* HAVE_SNMP */
		}
	    }
          /* Need move confirm list to down_table in case ftn tunnel_id is 
             not match vc's tunnel_id */
          else
            {
              mpls_confirm_entry_move (confirm, c_list, down_table, 
                                       CONFIRM_NODE_DUMMY, NULL); 
            }
        }
#ifdef HAVE_SNMP
      vc->last_change = nsm_get_sys_up_time();
#endif /*HAVE_SNMP */
    }
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
  else if (confirm->type == CONFIRM_VPLS_MESH_VC)
    {
      struct route_node *rn = NULL;
      struct nsm_vpls_peer *vpls_peer = NULL;
      struct route_table *down_table = NSM_MPLS->lsp_dep_down;
      struct nsm_vpls *vpls = NULL;
      bool_t lookup = NSM_FALSE;
      struct ftn_entry *ftn_t = NULL;
      struct nhlfe_key_gen *key;
      u_int32_t oif_ix = 0;
      u_char opcode;
      int ret = NSM_FAILURE;

      if (confirm->data == NULL)
        return;
      rn = confirm->data;

      if (rn->info == NULL)
        return;
      vpls_peer = rn->info;

      vpls = nsm_vpls_lookup (nm, rn->table->id, NSM_FALSE);
      NSM_ASSERT (vpls != NULL);
      if (vpls && vpls_peer->state == NSM_VPLS_PEER_UP)
        {
          nsm_vpls_fib_delete (vpls, vpls_peer);
          vpls_peer->vc_fib->nw_if_ix = 0;
          vpls_peer->ftn = NULL;

          if (ftn_next || ftn_sel)
            {  
              if ( ftn_next)
                ftn_t = ftn_next;
              else
                ftn_t = ftn_sel;

              /*
               * Check if the ftn entry tunnel id matches
               * with the VPLS tunnel-id.
               */
              if (ftn_t && (vpls_peer->tunnel_id == 0
                  || ftn_t->tun_id == vpls_peer->tunnel_id))
                {
                  gmpls_ftn_get_lookup_flag (ftn_t, &lookup);
                  if (lookup == NSM_TRUE)
                    opcode = PUSH_AND_LOOKUP_FOR_VC;
                  else
                    opcode = PUSH_FOR_VC;

               /* Update the Mesh VC-FIB if NHLFE is changed for new FTN */
               key = (struct nhlfe_key_gen *) FTN_NHLFE (ftn_t)->nkey;
               switch (FTN_NHLFE (ftn_t)->type)
                 {
                   case gmpls_entry_type_ip:
                     if ((key->u.pkt.afi == AFI_IP)
#ifdef HAVE_GMPLS
                         || (key->u.pkt.afi == AFI_IP6)
#endif /* HAVE_GMPLS */
                        )
                       {
                         oif_ix = key->u.pkt.u.ipv4.oif_ix;
                       }
#ifdef HAVE_IPV6
                     else
                       oif_ix = key->u.pkt.u.ipv6.oif_ix;
#endif /* HAVE_IPV6 */
                     break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
                   case gmpls_entry_type_pbb_te:
                     oif_ix = key->u.pbb.oif_ix;
                     break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
                   case gmpls_entry_type_tdm:
                     oif_ix = key->u.tdm.oif_ix;
                     break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
                   default:
                     break;
                 }

               if (vpls_peer->vc_fib->nw_if_ix != oif_ix)
                 vpls_peer->vc_fib->nw_if_ix = oif_ix;

                  ret = nsm_vpls_fib_add (vpls, vpls_peer, opcode, ftn_t);
                  if (ret == NSM_SUCCESS)
                    vpls_peer->ftn = ftn_t;
                }
              else
                {
                  mpls_confirm_entry_move (confirm, c_list, down_table,
                                           CONFIRM_NODE_DUMMY, NULL);
                }
            }

           if (ret == NSM_FAILURE)
             {
                vpls_peer->state = NSM_VPLS_PEER_ACTIVE;
#ifdef HAVE_SNMP
                vpls_peer->last_change = nsm_get_sys_up_time ();
                nsm_mpls_vc_up_down_notify (PW_L2_STATUS_DOWN,
                                            vpls_peer->vc_snmp_index);
#endif /*HAVE_SNMP */
                vpls_peer->ftn = NULL;
             }
        }
    }
  else if (confirm->type == CONFIRM_VPLS_SPOKE_VC)
    {
      struct nsm_vpls_spoke_vc *svc = confirm->data;
      struct ftn_entry *ftn_t = NULL;
      struct route_table *down_table = NSM_MPLS->lsp_dep_down;
      struct nhlfe_key_gen *key;
      u_int32_t oif_ix = 0;
      bool_t lookup = NSM_FALSE;
      u_char opcode;
      int ret = NSM_FAILURE;

      NSM_ASSERT (svc != NULL);
      if (svc && svc->state == NSM_VPLS_PEER_UP)
        {
          nsm_vpls_spoke_vc_fib_delete (svc);
          svc->vc_fib->nw_if_ix = 0;
          svc->vc->ftn = NULL;

          if (ftn_next || ftn_sel)
            {
              if (ftn_next)
                ftn_t = ftn_next;
              else
                ftn_t = ftn_sel;

              if (ftn_t && (svc->vc->tunnel_id == 0 ||
                  ftn_t->tun_id == svc->vc->tunnel_id))
                {
                  gmpls_ftn_get_lookup_flag (ftn_t, &lookup);
                  if (lookup == NSM_TRUE)
                    opcode = PUSH_AND_LOOKUP_FOR_VC;
                  else
                    opcode = PUSH_FOR_VC;

                  /* Update the VC-FIB if NHLFE is changed for new FTN */
                  key = (struct nhlfe_key_gen *) FTN_NHLFE (ftn_t)->nkey;
                  switch (FTN_NHLFE (ftn_t)->type)
                    {
                      case gmpls_entry_type_ip:
                        if ((key->u.pkt.afi == AFI_IP)
#ifdef HAVE_GMPLS
                            || (key->u.pkt.afi == AFI_IP6)
#endif /* HAVE_GMPLS */
                            )
                          {
                            oif_ix = key->u.pkt.u.ipv4.oif_ix;
                          }
#ifdef HAVE_IPV6
                        else
                          oif_ix = key->u.pkt.u.ipv6.oif_ix;
#endif /* HAVE_IPV6 */
                        break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
                     case gmpls_entry_type_pbb_te:
                       oif_ix = key->u.pbb.oif_ix;
                       break; 
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
                     case gmpls_entry_type_tdm:
                       oif_ix = key->u.tdm.oif_ix;
                       break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
                     default:
                       break;
                    }

                  if (svc->vc_fib->nw_if_ix != oif_ix)
                     svc->vc_fib->nw_if_ix = oif_ix;

                  ret = nsm_vpls_spoke_vc_fib_add (svc, opcode, ftn_t);

                  if (ret == NSM_SUCCESS)
                    svc->vc->ftn = ftn_t;
                }
              else
                mpls_confirm_entry_move (confirm, c_list, down_table,
                                         CONFIRM_NODE_DUMMY, NULL);
            }
 
          if (ret == NSM_FAILURE)
            {
              svc->state = NSM_VPLS_SPOKE_VC_ACTIVE;
              svc->vc->ftn = NULL;
            } 
        }
    }
#endif /* HAVE_VPLS */

  if (update_xc && ftn_dep)
    {
      struct fec_gen_entry fec, tfec;
      s_int32_t active_head = NSM_FALSE;

      nsm_gmpls_set_fec_from_ftn (ftn_dep, &fec);
      gmpls_ftn_del_from_fwd (nm, ftn_dep, &fec, GLOBAL_FTN_ID, 
                             ftn_old);

      ftn_tmp = NULL;

      if (ftn_next)
        ftn_tmp = ftn_next;
      else if (ftn_sel)
        ftn_tmp = ftn_sel;

      if (! ftn_tmp)
        return;

      nsm_gmpls_set_fec_from_ftn (ftn_tmp, &tfec);
      if (ftn_dep == gmpls_ftn_active_head (
                                        (struct ftn_entry *)ftn_dep->pn->info))
        active_head = NSM_TRUE;

      ret = gmpls_dep_ftn_row_update (ftn_dep, ftn_tmp);
      if (fec.type == gmpls_entry_type_ip)
        {
          if (ret < 0)
            zlog_err (NSM_ZG, "Cannot update dependant ftn %P xc to "
                              "next selected primary ftn %P",
                              &fec.u.prefix, &tfec);
          else
            zlog_info (NSM_ZG, "Update dependant ftn %P xc to "
                               "next selected primary ftn %P",
                               &fec.u.prefix, &tfec);
        }

      if ((fec.type == gmpls_entry_type_ip)
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
          || (fec.type == gmpls_entry_type_pbb_te)
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */
         )
        {
#ifdef HAVE_IPV6
          if (tfec.u.prefix.family == AF_INET6)
            ret = nsm_mpls_ftn6_add_to_fwd (nm, ftn_dep, &tfec.u.prefix, 
                                            GLOBAL_FTN_ID, ftn_tmp, 
                                            active_head);
          else
#endif /* HAVE_IPV6 */
            ret = nsm_mpls_ftn4_add_to_fwd (nm, ftn_dep, &tfec.u.prefix, 
                                            GLOBAL_FTN_ID, ftn_tmp, 
                                            active_head);
        }

      if (ret == NSM_SUCCESS)
        {
          SET_FLAG (ftn_dep->flags, FTN_ENTRY_FLAG_INSTALLED);
          FTN_XC_OPR_STATUS_COUNT_INC (ftn_dep);
          FTN_XC_STATUS_UPDATE (ftn_dep);
        }
    }

  return;
}

/* 
   Handle FTN entry add. If there are any VRF/VC
   entries dependent on this FTN entry, install them appropriately.
   If there is any entries should depends on this entry but right now
   depends on other entry, move dependent to this entry.
*/
static void
gmpls_lsp_dep_ftn_up_process (struct nsm_master *nm, struct ftn_entry *ftn, 
                              struct prefix *fec_prefix, s_int32_t uptable)
{
  struct route_table *up_table, *down_table;

  /* get up lsp dependency table */
  up_table = NSM_MPLS->lsp_dep_up;
  if (! up_table)
    return;

  /* get down lsp dependency table */
  down_table = NSM_MPLS->lsp_dep_down;
  if (! down_table)
    return;

  /* update dependencies in the up depencency table */
  if (uptable == NSM_TRUE)
    gmpls_lsp_dep_up_update (nm, up_table, ftn, fec_prefix);

  /* update dependencies in the down depencency table */
  gmpls_lsp_dep_down_update (nm, down_table, up_table, ftn, fec_prefix);
}


/* 
   Handle FTN entry delete  If there are any VRF/VC
   entries dependent on this FTN entry, uninstall them appropriately.
*/
static void
gmpls_lsp_dep_ftn_down_process (struct nsm_master *nm,
                                struct ftn_entry *ftn,
                                struct ptree_node *rn_ftn)
{
  struct route_table *up_table, *down_table;
  struct route_node *rn, *rn2, *rn_head;
  struct fec_gen_entry fec;
  struct lsp_dep_confirm *confirm, *confirm_tmp;
  struct confirm_list *c_list = NULL;
  struct confirm_list *c_list_new = NULL;
  bool_t lsp_dep_update, lookup, del_lookup_flag;
  struct ptree_node *rn_exc;
  struct ftn_entry *ftn_next, *ftn_tmp, *ftn_head, *ftn_sel = NULL;
#ifdef HAVE_MPLS_VC
  struct nsm_mpls_circuit *vc;
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
  struct route_node *rn_temp_vpls = NULL;
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls_spoke_vc *svc = NULL;
#endif /* HAVE_VPLS */
  s_int32_t ret;

  up_table = NSM_MPLS->lsp_dep_up;
  if (! up_table)
    return;

  down_table = NSM_MPLS->lsp_dep_down;
  if (! down_table)
    return;

  if (!rn_ftn || !rn_ftn->info)
    return;

  nsm_gmpls_set_fec_from_ftn ((struct ftn_entry *)rn_ftn->info, &fec);

  /* Try to select next valid ftn entry in the same node 
     If have secondary/backup with same tunnel-id, 
     move dep list to it, else set delete flag */
  ftn_next = gmpls_lsp_dep_ftn_down_select_next (nm, ftn, rn_ftn);
  ftn_head = gmpls_ftn_active_head ((struct ftn_entry *)rn_ftn->info);
  if (ftn != ftn_head)
    ftn_sel = ftn_head;

  rn_head = route_node_get (up_table, &fec.u.prefix);
  if (! rn_head)
    return;
 
  gmpls_ftn_get_lookup_flag (ftn, &del_lookup_flag);

  route_lock_node (rn_head);
  for (rn = rn_head; rn; rn = route_next_until (rn, rn_head))
    {
      c_list = rn->info;
      if (! c_list) 
        continue;

      if (c_list->type == CONFIRM_NODE_FTN &&
          ! prefix_same (&c_list->p, &fec.u.prefix))
        continue;
      
      lsp_dep_update = NSM_FALSE;

      for (confirm = c_list->head; confirm; confirm = confirm_tmp)
        {
          confirm_tmp = confirm->next;

#ifdef HAVE_MPLS_VC
          if (confirm->type == CONFIRM_VC)
            {
              vc = confirm->data;
              if (NSM_MPLS_VC_NOT_MAP_TO_FTN (ftn, vc))
                continue;
            }
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
          /* If the ftn entry matches the VPLS peer tunnel-id move the
             entry to down table. */
          if (confirm->type == CONFIRM_VPLS_MESH_VC)
            {
              rn_temp_vpls = confirm->data;
              if (rn_temp_vpls == NULL)
                continue;

              vpls_peer = rn_temp_vpls->info;
              if ((vpls_peer == NULL) ||
                  (NSM_VPLS_ENTRY_NOT_MAP_TO_FTN (ftn, vpls_peer)))
                continue;
            }
          else if (confirm->type == CONFIRM_VPLS_SPOKE_VC)
            {
              svc = confirm->data;

              if ((svc == NULL || svc->vc == NULL) ||
                  (NSM_VPLS_ENTRY_NOT_MAP_TO_FTN (ftn, svc->vc)))
                continue;
            }
#endif /* HAVE_VPLS */
          /* For rsvp mapped route or igp-shortcut-route, 
             do not need move to Down table if it need be deleted. */
          gmpls_dep_ftn_update_process (nm, ftn_next, ftn_sel, ftn, confirm, 
                                        c_list, rn, del_lookup_flag);
          if (rn->info)
            NSM_ASSERT (c_list->count != 0);
        }

      if (rn->info && c_list->count == 0)
        {
          mpls_confirm_list_free (c_list);
          rn->info = NULL;
          route_unlock_node (rn);
        }

      if (ftn_sel || ftn_next || rn->info == NULL)
        continue;

      /* lookup in the ftn table to get a match */
      rn_exc = gmpls_ftn_node_match_exclude (nm, GLOBAL_FTN_ID, 
                                             &rn->p, &fec.u.prefix);
      if (rn_exc)
        {
          ftn_tmp = rn_exc->info;
          if (CHECK_FLAG (ftn_tmp->flags, FTN_ENTRY_FLAG_SELECTED)
              && ! CHECK_FLAG (ftn_tmp->flags, FTN_ENTRY_FLAG_DEPENDENT))
            {
              nsm_gmpls_set_fec_from_ftn (ftn, &fec);

              prefix_copy (&c_list->p, &fec.u.prefix);

              ret = gmpls_ftn_get_lookup_flag (ftn_tmp, &lookup);
              if (ret == NSM_SUCCESS)
                {
                  for (confirm = c_list->head; confirm; confirm = confirm->next)
                    gmpls_confirm_data_add_to_fwd (nm, confirm,
                                                  ftn_tmp, lookup);

                  if (c_list->type != CONFIRM_NODE_DUMMY)
                    c_list->type = CONFIRM_NODE_DUMMY;
                 
                  lsp_dep_update = NSM_TRUE;
                }
            }
        }
     
      if (! lsp_dep_update && rn->info != NULL)
        {
          /* Remove node. */
          rn->info = NULL;
          route_unlock_node (rn);
         
          rn2 = route_node_get (down_table, &rn->p);

          if (! rn2)
            continue;
         
          if (rn2->info)
            {
              c_list_new = rn2->info;
              route_unlock_node (rn2);
             
              mpls_confirm_list_add_list (c_list_new, c_list);
              mpls_confirm_list_free (c_list);
              c_list = NULL;
            }
          else
            {
              rn2->info = c_list;
              c_list_new = c_list;
            }

          c_list_new->type = CONFIRM_NODE_DUMMY;
          pal_mem_set (&c_list_new->p, 0, sizeof (struct prefix));
        }
    }
  route_unlock_node (rn_head);
}

s_int32_t
gmpls_lsp_dep_del_data (struct nsm_master *nm, struct fec_gen_entry *fec,
                        u_char lsp_dep_type, s_int32_t up_table, 
                        void *data)
{
  struct route_table *tree;
  struct lsp_dep_confirm *confirm;
  struct route_node *rn;
  struct ftn_entry *ftn_list, *ftn_head = NULL;
  struct confirm_list *c_list;
  bool_t lookup = NSM_FALSE;
  s_int32_t found = NSM_FALSE;

  tree = up_table ? NSM_MPLS->lsp_dep_up : NSM_MPLS->lsp_dep_down;

  if (! tree)
    return NSM_FALSE;

  if (up_table)
    {
      /* get primary ftn entry for the specified fec prefix */
      ftn_list = gmpls_ftn_match_primary (nm, GLOBAL_FTN_ID, fec);

      if (ftn_list)
        ftn_head = gmpls_ftn_active_head (ftn_list);
    }

  if (fec->type != gmpls_entry_type_ip)
    {
      return NSM_FALSE;
    }

  /* lookup entry in the tree */
  rn = route_node_lookup (tree, &fec->u.prefix);
  if (! rn)
    return NSM_FALSE;

  route_unlock_node (rn);
  c_list = rn->info;

  /* lookup and remove confirm node from list */
  confirm = mpls_confirm_list_lookup (c_list, data, lsp_dep_type, NSM_TRUE);
  if (confirm)
    {
      gmpls_ftn_get_lookup_flag (ftn_head, &lookup);
      if (up_table)
        mpls_confirm_data_delete_from_fwd (nm, confirm, ftn_head, lookup);

      XFREE (MTYPE_NSM_LSP_DEP_CONFIRM, confirm);
      confirm = NULL;
      found = NSM_TRUE;
    }
 
  /* cleanup list and tree */
  if (c_list->count == 0)
    {
      mpls_confirm_list_free (c_list);
      rn->info = NULL;
      route_unlock_node (rn);
    }

  return found;
}

/* Handle a delete for a resource that is dependent on an LSP. */
void
gmpls_lsp_dep_del (struct nsm_master *nm,
                   struct fec_gen_entry *fec, u_char lsp_dep_type, void *data)
{
  s_int32_t ret;

  ret = gmpls_lsp_dep_del_data (nm, fec, lsp_dep_type, NSM_TRUE, data);

  /* If could not found in up-table, need check down-table */
  if (ret == NSM_FALSE)
    gmpls_lsp_dep_del_data (nm, fec, lsp_dep_type, NSM_FALSE, data);
}

void
gmpls_generate_key_from_node (afi_t family, struct prefix *pfx, 
                              struct ptree_node *node)
{
  u_char byte_len;

  /* set family */
  pfx->family = family;
  pfx->prefixlen = node->key_len;
  
  byte_len = node->key_len / 8;

  if (node->key_len % 8)
    byte_len ++;

  pal_mem_cpy (&pfx->u.prefix, &node->key, byte_len);
  return;
}

bool_t
gmpls_fec_same (struct fec_gen_entry *fec, struct ptree_node *node)
{
  struct prefix pfx;

  switch (fec->type)
    {
      case gmpls_entry_type_ip:
        /* key same */
        gmpls_generate_key_from_node (fec->u.prefix.family, &pfx,
                                      node);
        if (prefix_cmp (&fec->u.prefix, &pfx) == 0)
          return PAL_TRUE;
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        if (pal_mem_cmp (&fec->u.pbb, node->key, 
                         sizeof (struct fec_entry_pbb_te)) == 0)
          return PAL_TRUE;

        break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        if (pal_mem_cmp (fec->tdm, node->key, sizeof (struct tdm_label)) == 0)
          return PAL_TRUE;

        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
        break;
    }

  return PAL_FALSE;  
}

/* Handle an add for a resource that is dependent on an LSP. */
void
gmpls_lsp_dep_add (struct nsm_master *nm, struct fec_gen_entry *fec,
                   u_char lsp_dep_type, void *data)
{
  struct lsp_dep_confirm *confirm = NULL;
  struct route_table *tree;
  struct ptree_node *pn_ftn = NULL;
  struct route_node *rn;
  struct ftn_entry *ftn = NULL;
  struct confirm_list *c_list = NULL;
  bool_t up;
  bool_t lookup;
  bool_t clist_created = NSM_FALSE;
  u_char node_type = CONFIRM_NODE_DUMMY;
  s_int32_t ret;
  struct fec_gen_entry pfec;

  up = NSM_FALSE;
  lookup = NSM_FALSE;

  /* If this is a VC ADD Message check if the VC has a tunnel confiugred for it
   * and if this entry is for the configured tunnel proceed with the addtion.
   * Otherwise add the VC to the dep down table and proceed.
   */
#ifdef HAVE_MPLS_VC
  if (lsp_dep_type == CONFIRM_VC)
    {
      struct ptree_ix_table *ftn_ix_table;
      struct nsm_mpls_circuit *vc = data;

      /* Get correct FTN table. */
      ftn_ix_table = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, 
                                                 fec);
      if ((! ftn_ix_table) || (! ftn_ix_table->m_table))
        return;

      /* Loop into the FTN table to identify the required ftn entry for use 
         with this VC */
      pn_ftn = gmpls_ftn_node_match (nm, GLOBAL_FTN_ID, fec);
      if (pn_ftn)
        {
          /*Search ftn for default case*/
          if ((!(vc->owner)) && ((! vc->tunnel_name) && (vc->tunnel_id == 0)))
            {
              ftn = gmpls_ftn_active_head ((struct ftn_entry *)pn_ftn->info);
              if (ftn->owner.owner == MPLS_LDP)
                vc->dn = PSNBOUND;
              vc->ftn = ftn;
              up = NSM_TRUE;
            }
          else
            {
              /* Search ftn if tunnel id or name or vc->owner through SNMP are
               * specified */
              for (ftn = pn_ftn->info ; ftn; ftn = ftn->next)
                { 
                  if (! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED) || 
                      ! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED) 
#ifdef HAVE_GMPLS
                      || CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_DUMMY)
#endif /* HAVE_GMPLS */
                     )
                    continue;

                  /* If tunnel info is passed in from the CLI get the
                   * relevant tunnel  */
                  if ((vc->tunnel_name) || (vc->tunnel_id > 0))
                    {
                      /* Check owner not LDP */
                      if (ftn->owner.owner == MPLS_LDP)
                        continue;

                      /* Check if the Tunnel direction matches */
                      if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_FWD))
                        {
                          if (vc->tunnel_dir != TUNNEL_DIR_FWD)
                            continue;
                        }
                      else if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_BIDIR))
                        {
                          if (vc->tunnel_dir != TUNNEL_DIR_REV)
                            continue;
                        }

                      if (((vc->tunnel_name) &&
                           (pal_strcmp (ftn->sz_desc, vc->tunnel_name) == 0)) ||
                          (vc->tunnel_id > 0 &&
                           ftn->tun_id == vc->tunnel_id))
                        {
                          vc->ftn = ftn;
                          up = NSM_TRUE;
                          break;
                        }
                    }
                  else 
                    { /* VC owner Check */
                      if (ftn && (ftn->owner.owner == vc->owner))
                        {
                          up = NSM_TRUE;
                          if (ftn->owner.owner == MPLS_LDP)
                            vc->dn = PSNBOUND;
                          vc->ftn = ftn;
                          break;
                        }
                      else
                        continue;
                    }
                }/*end of for */
            }/*end of else */
        }/*end of if*/
      
      /* could not find ftn which is depend on */
      if (up == NSM_FALSE ||
          ! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED) ||
          CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_DEPENDENT))
        {
          SET_FLAG (vc->pw_status, NSM_CODE_PW_PSN_EGRESS_TX_FAULT);
          up = NSM_FALSE;
          nsm_mpls_l2_circuit_send_pw_status (vc->vc_info->mif, vc);
        }
    }/*end of if (lsp_dep_type == CONFIRM_VC) */
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
  else if (lsp_dep_type == CONFIRM_VPLS_MESH_VC ||
           lsp_dep_type == CONFIRM_VPLS_SPOKE_VC)
    {
      struct ptree_ix_table *ftn_ix_table = NULL;
      struct route_node *rn_vpls = NULL;
      struct nsm_vpls_peer *vpls_peer = NULL;
      struct nsm_vpls_spoke_vc *svc = NULL;
      u_int32_t vpls_entry_tunnel_id = -1;
      u_char vpls_entry_tunnel_dir = TUNNEL_DIR_FWD;
      char *vpls_entry_tunnel_name = NULL;

      if (lsp_dep_type == CONFIRM_VPLS_MESH_VC)
        {
          rn_vpls = data;
          vpls_peer = rn_vpls->info;
          if (!vpls_peer)
            return;

          vpls_entry_tunnel_name = vpls_peer->tunnel_name;
          vpls_entry_tunnel_id = vpls_peer->tunnel_id;
          vpls_entry_tunnel_dir = vpls_peer->tunnel_dir;
        }
      else if (lsp_dep_type == CONFIRM_VPLS_SPOKE_VC)
        {
          svc = data;
          if (svc->vc == NULL)
            return;

          vpls_entry_tunnel_name = svc->vc->tunnel_name;
          vpls_entry_tunnel_id = svc->vc->tunnel_id;
          vpls_entry_tunnel_dir = svc->vc->tunnel_dir;
        }

      /* Get correct FTN table. */
      ftn_ix_table = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, fec);
      if ((! ftn_ix_table) || (! ftn_ix_table->m_table))
        return;

      /* Loop into the FTN table to identify the required ftn entry. */
      pn_ftn = gmpls_ftn_node_match (nm, GLOBAL_FTN_ID, fec);
      if (pn_ftn)
        {
          /* Search ftn for default case */
          if ((! vpls_entry_tunnel_name) && (vpls_entry_tunnel_id == 0))
            {
              ftn = gmpls_ftn_active_head ((struct ftn_entry *)pn_ftn->info);

              if (lsp_dep_type == CONFIRM_VPLS_MESH_VC)
                vpls_peer->ftn = ftn;
              else if (lsp_dep_type == CONFIRM_VPLS_SPOKE_VC)
                svc->vc->ftn = ftn;

              if (IS_NSM_DEBUG_EVENT)
                zlog_info (NSM_ZG, "VPLS Entry ADD: ftn entry found with"
                           "Tunnel-id %d", ftn->tun_id);

              up = NSM_TRUE;
            }
          else
            {
              /* Search ftn if tunnel id is specified */
              for (ftn = pn_ftn->info ; ftn; ftn = ftn->next)
                {
                  if (! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED) ||
                      ! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED))
                    continue;

                  /* Check for the Tunnel Direction */
                  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_FWD))
                    {
                      if (vpls_entry_tunnel_dir != TUNNEL_DIR_FWD)
                        continue;
                    }
                  else if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_BIDIR))
                    {
                      if (vpls_entry_tunnel_dir != TUNNEL_DIR_REV)
                        continue;
                    }

                  if (((vpls_entry_tunnel_name) &&
                        (pal_strcmp (ftn->sz_desc,
                                     vpls_entry_tunnel_name) == 0)) ||
                      (vpls_entry_tunnel_id > 0 &&
                       ftn->tun_id == vpls_entry_tunnel_id))
                    {
                      if (lsp_dep_type == CONFIRM_VPLS_MESH_VC)
                        vpls_peer->ftn = ftn;
                      else if (lsp_dep_type == CONFIRM_VPLS_SPOKE_VC)
                        svc->vc->ftn = ftn;

                      if (IS_NSM_DEBUG_EVENT)
                        zlog_info (NSM_ZG, "VPLS Entry ADD: ftn entry found"
                            "with the specified Tunnel-id %d",
                            ftn->tun_id);

                      up = NSM_TRUE;
                      break;
                    }
                }/*end of for */
            }/*end of else */
        }/*end of if*/
    }
#endif /* HAVE_VPLS */
#ifdef HAVE_MPLS_VC
  else
    {
      /* get the selected ftn entry for the specified fec prefix */
      pn_ftn = gmpls_ftn_node_match (nm, GLOBAL_FTN_ID, fec);

      /* get corresponding ftn entry */
      if (pn_ftn)
        ftn = gmpls_ftn_active_head ((struct ftn_entry *)pn_ftn->info);
    }
#endif /* HAVE_MPLS_VC */

  if (ftn && 
      CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED) && 
      ! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_DEPENDENT))
    {
      up = NSM_TRUE;

      /* check for appropriate lsp tunnel for specified fec */ 
      ret = gmpls_ftn_get_lookup_flag (ftn, &lookup);
      if (ret < 0)
        return;

      /* check if ftn prefix is same as lsp dep prefix */
      if (gmpls_fec_same (fec, pn_ftn))
        node_type = CONFIRM_NODE_FTN;
    }

  /* Get correct table node. */
  tree = ! up ? NSM_MPLS->lsp_dep_down : NSM_MPLS->lsp_dep_up;
  if (! tree)
    return;

  /* get a node for fec */
  rn = route_node_get (tree, &fec->u.prefix);
  if (! rn)
    return;

  /* get confirm list */
  c_list = rn->info;
  if (! c_list)
    {
      if (ftn)
        {
          if (pn_ftn && pn_ftn->info != NULL)
            {
              nsm_gmpls_set_fec_from_ftn ((struct ftn_entry *) pn_ftn->info, 
                                          &pfec);
            }
          c_list = mpls_confirm_list_new (node_type, &pfec.u.prefix); 
        }
      else
        c_list = mpls_confirm_list_new (node_type, NULL);
      if (! c_list)
        {
          route_unlock_node (rn);
          return;
        }

      prefix_copy (&c_list->p, &fec->u.prefix);
      clist_created = NSM_TRUE;
      rn->info = c_list;
    }
  else
    route_unlock_node (rn);

  /* Check if confirm already in the list */
  confirm = mpls_confirm_list_lookup (c_list, data, lsp_dep_type, NSM_FALSE);
  
  if (! confirm)
    {
      /* Create object. */
      confirm = XCALLOC (MTYPE_NSM_LSP_DEP_CONFIRM,
                         sizeof (struct lsp_dep_confirm));
      if (! confirm)
        return;

      /* Copy data. */
      confirm->type = lsp_dep_type;
      confirm->data = data;

      /* add confirm node to confirm list */
      ret = mpls_confirm_list_add (c_list, confirm);
      if (ret < 0)
        {
          XFREE (MTYPE_NSM_LSP_DEP_CONFIRM, confirm);
          if (clist_created)
            {
              mpls_confirm_list_free (c_list);
              rn->info = NULL;
              route_unlock_node (rn);
            }
          return;
        }
    }

  if (up == NSM_TRUE)
    gmpls_confirm_data_add_to_fwd (nm, confirm, ftn, lookup);
}

static void
gmpls_nhlfe_nh_addr_to_prefix (struct nhlfe_key *key, struct prefix *p)
{
  pal_mem_set (p, 0, sizeof (struct prefix));
  p->family = key->afi;
  
  if (key->afi == AFI_IP)
    {
      IPV4_ADDR_COPY (&p->u.prefix4, &key->u.ipv4.nh_addr);
      p->prefixlen = IPV4_MAX_BITLEN;
    }
#ifdef HAVE_IPV6
  else if (key->afi == AFI_IP6)
    {
      IPV6_ADDR_COPY (&p->u.prefix6, &key->u.ipv6.nh_addr);
      p->prefixlen = IPV6_MAX_BITLEN;
    }
#endif /* HAVE_IPV6 */

#ifdef HAVE_GMPLS
  else if (key->afi == AFI_UNNUMBERED)
    {
      IPV4_ADDR_COPY (&p->u.prefix4, &key->u.ipv4.nh_addr);
      p->prefixlen = IPV4_MAX_BITLEN;
    }
#endif /* HAVE_GMPLS */

}

void
gmpls_ftn_entry_deselect (struct nsm_master *nm, struct ftn_entry *ftn, 
                          struct ptree_node *pn, bool_t del_fib)
{
  /* Set the route node correctly */
#ifdef HAVE_SNMP
  struct nsm_mpls *nmpls = nm->nmpls;
#endif  /* HAVE_SNMP */
  struct ftn_entry *ftn_head;
  struct fec_gen_entry fec;

  if (! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED))
    return;

  UNSET_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED);
  FTN_XC_ADMN_STATUS_COUNT_DEC (ftn);
  
  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_DEPENDENT))
    {
      struct nhlfe_key *key;
      struct prefix p;
     
      key = (struct nhlfe_key *) ftn->xc->nhlfe->nkey;
      gmpls_nhlfe_nh_addr_to_prefix (key, &p);

      if (ftn->ftn_type == MPLS_FTN_MAPPED)
        {
          nsm_gmpls_set_fec_from_ftn ((struct ftn_entry *) pn->info, &fec);
          gmpls_lsp_dep_del (nm, &fec, CONFIRM_MAPPED_ROUTE, pn);
        }
      else if (ftn->ftn_type == MPLS_FTN_RSVP_MAPPED)
        {
          fec.type = gmpls_entry_type_ip;
          fec.u.prefix = *ftn->pri_fec_prefix;

          gmpls_lsp_dep_del (nm, &fec, CONFIRM_RSVP_MAPPED_ROUTE, ftn);
        }
      else if (ftn->ftn_type == MPLS_FTN_IGP_SHORTCUT)
        {
          fec.type = gmpls_entry_type_ip;
          fec.u.prefix = *ftn->pri_fec_prefix;

          gmpls_lsp_dep_del (nm, &fec, CONFIRM_IGP_SHORTCUT_ROUTE, ftn);
        }
      else
        {
          fec.type = gmpls_entry_type_ip;
          fec.u.prefix = p;

          gmpls_lsp_dep_del (nm, &fec, CONFIRM_VRF, pn);
        }
    }

  ftn_head = gmpls_ftn_active_head ((struct ftn_entry *)pn->info);

  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED))
    {
      if (ftn->ftn_type == MPLS_FTN_REGULAR && 
          (ftn == ftn_head || ftn_head == NULL || 
           ftn->tun_id != ftn_head->tun_id
#ifdef HAVE_GMPLS
          || ((ftn_head->flags & FTN_ENTRY_FLAG_BIDIR) !=
              (ftn->flags & FTN_ENTRY_FLAG_BIDIR))
          || ((ftn_head->flags & FTN_ENTRY_FLAG_FWD) !=
              (ftn->flags & FTN_ENTRY_FLAG_FWD))

#endif /* HAVE_GMPLS */
          ))
        gmpls_lsp_dep_ftn_down_process (nm, ftn, pn);

      if (del_fib)
        {
         
          nsm_gmpls_set_fec_from_ftn (ftn, &fec);
          gmpls_ftn_del_from_fwd (nm, ftn, &fec, pn->tree->id, NULL);
        }
    }
  
  FTN_XC_STATUS_UPDATE (ftn);
#ifdef HAVE_SNMP
  if (nmpls->notificationcntl == NSM_MPLS_NOTIFICATIONCNTL_ENA &&
      CHECK_FLAG (ftn->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG)) 
    nsm_mpls_opr_sts_trap (nmpls, ftn->xc->key.xc_ix, ftn->xc->opr_status);
#endif  /* HAVE_SNMP */
}

void
nsm_gmpls_ftn_add_check (struct nsm_master *nm, struct ftn_entry *ftn,
                         struct ftn_entry *ftn_head, struct ptree_node *pn)
{
  struct ftn_entry *ftn_next, *ftn_tmp;
  mpls_lsp_priority_t ftn_pri, ftn_next_pri, ftn_head_pri, ftn_tmp_pri;
  struct fec_gen_entry fec;

  if ((! ftn->next) || (! ftn_head))
    return;

  if ((CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INACTIVE)) 
#ifdef HAVE_GMPLS
      || (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_DUMMY))
#endif /* HAVE_GMPLS */
     )
    return;
 
  ftn_next = ftn->next;
  ftn_pri = gmpls_owner_priority (ftn->owner.owner);
  ftn_next_pri = gmpls_owner_priority (ftn_next->owner.owner);
  ftn_head_pri = gmpls_owner_priority (ftn_head->owner.owner);

  /* in case NSM_MPLS_FLAG_INSTALL_BK_LSP is set */
  if (ftn != ftn_head && ftn->tun_id == ftn_head->tun_id
#ifdef HAVE_GMPLS
      && ((ftn_head->flags & FTN_ENTRY_FLAG_BIDIR) ==
          (ftn->flags & FTN_ENTRY_FLAG_BIDIR))
      && ((ftn_head->flags & FTN_ENTRY_FLAG_FWD) ==
          (ftn->flags & FTN_ENTRY_FLAG_FWD))
#endif /* HAVE_GMPLS */
      )
    return;

  if (ftn_pri == ftn_next_pri && (ftn->tun_id != ftn_next->tun_id
#ifdef HAVE_GMPLS
          || ((ftn_next->flags & FTN_ENTRY_FLAG_BIDIR) !=
              (ftn->flags & FTN_ENTRY_FLAG_BIDIR))
          || ((ftn_next->flags & FTN_ENTRY_FLAG_FWD) !=
              (ftn->flags & FTN_ENTRY_FLAG_FWD))
#endif /* HAVE_GMPLS */
      ))
    return;

  nsm_gmpls_set_fec_from_ftn (ftn, &fec);
  if (! CHECK_FLAG (NSM_MPLS->flags, NSM_MPLS_FLAG_INSTALL_BK_LSP))
    {
      for (ftn_tmp = ftn_next; ftn_tmp; ftn_tmp = ftn_tmp->next)
        {
          ftn_tmp_pri = gmpls_owner_priority (ftn_tmp->owner.owner);
          if (ftn_tmp_pri != ftn_next_pri)
            break;

          if (ftn_tmp->ftn_type == MPLS_FTN_REGULAR &&
             (ftn_tmp == ftn_head || ftn_tmp->tun_id != ftn_head->tun_id
#ifdef HAVE_GMPLS
             || ((ftn_tmp->flags & FTN_ENTRY_FLAG_BIDIR) !=
              (ftn_head->flags & FTN_ENTRY_FLAG_BIDIR))
             || ((ftn_tmp->flags & FTN_ENTRY_FLAG_FWD) !=
              (ftn_head->flags & FTN_ENTRY_FLAG_FWD))
#endif /* HAVE_GMPLS */
               ))
            gmpls_lsp_dep_ftn_down_process (nm, ftn_tmp, pn); 

          gmpls_ftn_del_from_fwd (nm, ftn_tmp, &fec, pn->tree->id, NULL);
        }
    }
}

s_int32_t
gmpls_ftn_entry_select_process (struct nsm_master *nm, struct ftn_entry *ftn, 
                                struct ptree_node *pn)
{
  struct ftn_entry *ftn_head, *ftn_t_head;
  s_int32_t ret = NSM_SUCCESS;
  s_int32_t active_head = NSM_FALSE;
  struct fec_gen_entry fec;

  if ((ftn->ent_type != FTN_ENTRY_TYPE_IPV4)
#ifdef HAVE_IPV6
      && (ftn->ent_type != FTN_ENTRY_TYPE_IPV6)
#endif /* HAVE_IPV6 */
      )
     return SUCCESS;

  if (pn == NULL)
     return SUCCESS;

  ftn_head = gmpls_ftn_active_head ((struct ftn_entry *)pn->info);
  ftn_t_head = gmpls_ftn_active_tunnel_head ((struct ftn_entry *)pn->info,
                                             ftn->tun_id, ftn->flags);

  nsm_gmpls_set_fec_from_ftn (ftn, &fec);

  if (!(CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_DEPENDENT))
      || (ftn->ftn_type == MPLS_FTN_MAPPED))
    {
      /* first need delete ftn entry already installed, if there is any */
      nsm_gmpls_ftn_add_check (nm, ftn, ftn_head, pn);

      /* install ftn entry to fib */
      if (ftn == ftn_head)
        active_head = NSM_TRUE;

      switch (ftn->ent_type)
        {
#ifdef HAVE_IPV6
          case FTN_ENTRY_TYPE_IPV6:
            ret = nsm_mpls_ftn6_add_to_fwd (nm, ftn, &fec.u.prefix, 
                                            pn->tree->id, NULL, active_head);
            break;
#endif /* HAVE_IPV6 */

          case FTN_ENTRY_TYPE_IPV4:
            ret = nsm_mpls_ftn4_add_to_fwd (nm, ftn, &fec.u.prefix, 
                                            pn->tree->id, 
                                            NULL, active_head);
            break;

          default:
            ret = NSM_SUCCESS;
            break;
        }

      if (ret != NSM_SUCCESS)
        return NSM_ERR_FTN_INSTALL_FAILURE;
      
      SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED);
      FTN_XC_OPR_STATUS_COUNT_INC (ftn);

      if (ftn->ftn_type == MPLS_FTN_REGULAR)
        {
          /* only move up-table dependent entry to new selected ftn which 
           * is active head 
           */
          if ((ftn->ent_type == FTN_ENTRY_TYPE_IPV4)
#ifdef HAVE_IPV6
              || (ftn->ent_type == FTN_ENTRY_TYPE_IPV6)
#endif /* HAVE_IPV6 */
             )
            {
              if (ftn == ftn_head)
                {
                  gmpls_lsp_dep_ftn_up_process (nm, ftn, &fec.u.prefix, 
                                                NSM_TRUE); 
                }
              else if ((ftn_head == NULL) ||
                       ((ftn->tun_id != ftn_head->tun_id
#ifdef HAVE_GMPLS
                       || ((ftn->flags & FTN_ENTRY_FLAG_BIDIR) !=
                          (ftn_head->flags & FTN_ENTRY_FLAG_BIDIR))
                       || ((ftn->flags & FTN_ENTRY_FLAG_FWD) !=
                          (ftn_head->flags & FTN_ENTRY_FLAG_FWD))
#endif /* HAVE_GMPLS */
                         ) && 
                       (ftn == ftn_t_head)))
                  {
                    gmpls_lsp_dep_ftn_up_process (nm, ftn, &fec.u.prefix, 
                                                  NSM_FALSE);
                  }
            }
        }
    }
  else
    {
      struct fec_gen_entry fec;
      u_char dep_type;
      struct nhlfe_key *key;
     
      if (ftn->ftn_type == MPLS_FTN_RSVP_MAPPED)
        dep_type = CONFIRM_RSVP_MAPPED_ROUTE;
      else if (ftn->ftn_type == MPLS_FTN_IGP_SHORTCUT)
        dep_type = CONFIRM_IGP_SHORTCUT_ROUTE; 
      else
        dep_type = CONFIRM_VRF;

      fec.type = gmpls_entry_type_ip;
      key = (struct nhlfe_key *) &ftn->xc->nhlfe->nkey;
      gmpls_nhlfe_nh_addr_to_prefix (key, &fec.u.prefix);

      if (ftn->ftn_type == MPLS_FTN_RSVP_MAPPED ||
          ftn->ftn_type == MPLS_FTN_IGP_SHORTCUT)
        {
          fec.u.prefix = *ftn->pri_fec_prefix;
          gmpls_lsp_dep_add (nm, &fec, dep_type, ftn);
        }
      else
        {
          gmpls_lsp_dep_add (nm, &fec, dep_type, pn);
        }
    }

  return NSM_SUCCESS;
}

s_int32_t
gmpls_ftn_entry_select (struct nsm_master *nm,
                       struct ftn_entry *ftn, struct ptree_node *pn)
{
#ifdef HAVE_SNMP
  struct nsm_mpls *nmpls = nm->nmpls;
#endif  /* HAVE_SNMP */
  s_int32_t ret;

  SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED);
  FTN_XC_ADMN_STATUS_COUNT_INC (ftn);
  
  ret = gmpls_ftn_entry_select_process (nm, ftn, pn);
  
  FTN_XC_STATUS_UPDATE (ftn);
#ifdef HAVE_SNMP
  if (nmpls->notificationcntl == NSM_MPLS_NOTIFICATIONCNTL_ENA &&
      CHECK_FLAG (ftn->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG)) 
    nsm_mpls_opr_sts_trap (nmpls, ftn->xc->key.xc_ix, ftn->xc->opr_status);
#endif  /* HAVE_SNMP */
  return ret;
}

void
gmpls_ftn_add_select_update (struct nsm_master *nm,
                             struct ftn_entry *ftn,
                             struct ptree_node *pn)
{
  struct ftn_entry *ftn_tmp, *ftn_next;
  mpls_lsp_priority_t ftn_pri, ftn_next_pri, ftn_tmp_pri;
  bool_t pri_next_deselect = NSM_FALSE;
  struct fec_gen_entry fec;

  if (! pn->info || ! ftn || ! ftn->next)
    return;

  ftn_next = ftn->next;
  ftn_next_pri = gmpls_owner_priority (ftn_next->owner.owner);
  ftn_pri = gmpls_owner_priority (ftn->owner.owner);

  /* should not happened */
  if (ftn_pri > ftn_next_pri)
    return;

  /* need deselect but keep in list for dep ftn */
  if (ftn_pri < ftn_next_pri)
    {
      if (CHECK_FLAG (ftn_next->flags, FTN_ENTRY_FLAG_DEPENDENT))
        {
          if (CHECK_FLAG (ftn_next->flags, FTN_ENTRY_FLAG_SELECTED))
            {
              UNSET_FLAG (ftn_next->flags, FTN_ENTRY_FLAG_SELECTED);
              FTN_XC_ADMN_STATUS_COUNT_DEC (ftn_next);
              
              if (CHECK_FLAG (ftn_next->flags, FTN_ENTRY_FLAG_INSTALLED))
                {
                   nsm_gmpls_set_fec_from_ftn (ftn_next, &fec);
#ifdef HAVE_IPV6
                  if (fec.u.prefix.family == AF_INET6)
                    nsm_mpls_ftn6_del_from_fwd (nm, ftn_next, 
                                                &fec.u.prefix,
                                                ftn_next->pn->tree->id, NULL);
                  else
#endif /* HAVE_IPV6 */
                    nsm_mpls_ftn4_del_from_fwd (nm, ftn_next, 
                                                &fec.u.prefix,
                                               ftn_next->pn->tree->id, NULL);
                  UNSET_FLAG (ftn_next->flags, FTN_ENTRY_FLAG_INSTALLED);
                  FTN_XC_OPR_STATUS_COUNT_DEC (ftn_next);
                  FTN_XC_STATUS_UPDATE (ftn_next);
                }
            }
        }
      else
        {
          gmpls_ftn_entry_deselect (nm, ftn_next, pn, NSM_FALSE);
          pri_next_deselect = NSM_TRUE;
        }
    }
  else if ((ftn_next->tun_id == ftn->tun_id)
#ifdef HAVE_GMPLS
            && ((ftn_next->flags & FTN_ENTRY_FLAG_BIDIR) ==
                (ftn->flags & FTN_ENTRY_FLAG_BIDIR))
            && ((ftn_next->flags & FTN_ENTRY_FLAG_FWD) ==
                (ftn->flags & FTN_ENTRY_FLAG_FWD))
#endif /* HAVE_GMPLS */
           )
    { 
      if (! CHECK_FLAG (NSM_MPLS->flags, NSM_MPLS_FLAG_INSTALL_BK_LSP))
        gmpls_ftn_entry_deselect (nm, ftn_next, pn, NSM_FALSE);

      if (ftn_next->igp_status == FTN_ENTRY_IGP_STATUS_ADVERTISED)
        {
          if (ftn_next->lsp_bits.bits.lsp_metric != 
              ftn->lsp_bits.bits.lsp_metric)
            {
              nsm_gmpls_set_fec_from_ftn (ftn, &fec);
              nsm_mpls_send_igp_shortcut_lsp (ftn, &fec.u.prefix,
                                              NSM_MSG_IGP_SHORTCUT_UPDATE);
            }

          ftn_next->igp_status = FTN_ENTRY_IGP_STATUS_NONE;
          ftn->igp_status = FTN_ENTRY_IGP_STATUS_ADVERTISED;
        }
    }

  for (ftn_tmp = ftn_next->next; ftn_tmp; ftn_tmp = ftn_tmp->next)
    {
      ftn_tmp_pri = gmpls_owner_priority (ftn_tmp->owner.owner);
      if (ftn_tmp_pri != ftn_next_pri)
        break;

      if (pri_next_deselect == NSM_TRUE)
        gmpls_ftn_entry_deselect (nm, ftn_tmp, pn, NSM_FALSE);
      else if (ftn_tmp->tun_id == ftn->tun_id && 
#ifdef HAVE_GMPLS
            ((ftn_tmp->flags & FTN_ENTRY_FLAG_BIDIR) ==
                (ftn->flags & FTN_ENTRY_FLAG_BIDIR))
            && ((ftn_tmp->flags & FTN_ENTRY_FLAG_FWD) ==
                (ftn->flags & FTN_ENTRY_FLAG_FWD)) &&
#endif /* HAVE_GMPLS */

               ! CHECK_FLAG (NSM_MPLS->flags, NSM_MPLS_FLAG_INSTALL_BK_LSP))
        gmpls_ftn_entry_deselect (nm, ftn_tmp, pn, NSM_TRUE);
    }
}

void
gmpls_ftn_delete_select_update (struct nsm_master *nm,
                                mpls_lsp_priority_t old_pri,
                                struct ptree_node *pn)
{
  struct ftn_entry *ftn_head, *ftn, *ftn_prev;
  mpls_lsp_priority_t pri_head = MPLS_P_NONE; 
  mpls_lsp_priority_t ftn_pri;
  struct fec_gen_entry fec;
 
  if (! pn->info)
    return;

  ftn_head = gmpls_ftn_active_head ((struct ftn_entry *)pn->info); 
  if (! ftn_head)
    return;

  pri_head = gmpls_owner_priority (ftn_head->owner.owner);

  if (pri_head <= old_pri)
    return;

  if (! CHECK_FLAG (ftn_head->flags, FTN_ENTRY_FLAG_SELECTED))
    gmpls_ftn_entry_select (nm, ftn_head, pn);

  if (ftn_head->lsp_bits.bits.igp_shortcut && 
      ftn_head->igp_status != FTN_ENTRY_IGP_STATUS_ADVERTISED)
    {
      nsm_gmpls_set_fec_from_ftn (ftn_head, &fec);
      if (fec.type == gmpls_entry_type_ip)
        {
          nsm_mpls_send_igp_shortcut_lsp (ftn_head, &fec.u.prefix,
                                          NSM_MSG_IGP_SHORTCUT_ADD);
          ftn_head->igp_status = FTN_ENTRY_IGP_STATUS_ADVERTISED;
        }
    }

  for (ftn = ftn_head->next, ftn_prev = ftn_head; ftn; ftn = ftn->next)
    {
      ftn_pri = gmpls_owner_priority (ftn->owner.owner);
      if (ftn_pri != pri_head)
        break;
     
      if ((CHECK_FLAG (NSM_MPLS->flags, NSM_MPLS_FLAG_INSTALL_BK_LSP) ||
           CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_PRIMARY) ||
           ftn_prev->tun_id != ftn->tun_id
#ifdef HAVE_GMPLS
          || ((ftn->flags & FTN_ENTRY_FLAG_BIDIR) !=
              (ftn_head->flags & FTN_ENTRY_FLAG_BIDIR))
          || ((ftn->flags & FTN_ENTRY_FLAG_FWD) !=
              (ftn_head->flags & FTN_ENTRY_FLAG_FWD))
#endif /* HAVE_GMPLS */

           ) && (! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED)))
        {
          if (! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED))
            gmpls_ftn_entry_select (nm, ftn, pn);

          if (ftn->lsp_bits.bits.igp_shortcut &&
              ftn_head->igp_status != FTN_ENTRY_IGP_STATUS_ADVERTISED)
            {
              nsm_gmpls_set_fec_from_ftn (ftn_head, &fec);
              if (fec.type == gmpls_entry_type_ip)
                {
                  nsm_mpls_send_igp_shortcut_lsp (ftn, &fec.u.prefix,
                                                  NSM_MSG_IGP_SHORTCUT_ADD);
                  ftn->igp_status = FTN_ENTRY_IGP_STATUS_ADVERTISED;
                }
            }
        }

      ftn_prev = ftn;
    }
}


/* Remove specified ftn entry from ftn entry list without deleting it. */
void
ftn_list_delete_node (struct ftn_entry **ftn_list, struct ftn_entry *ftn)
{
  struct ftn_entry *tmp;

  NSM_ASSERT (*ftn_list != NULL);
  if (*ftn_list == NULL)
    return;

  /* If first... */
  if (*ftn_list == ftn)
    {
      *ftn_list = ftn->next;
      return;
    }

  /* Else... */
  for (tmp = *ftn_list; tmp; tmp = tmp->next)
    {
      if (tmp->next == ftn)
        {
          tmp->next = ftn->next;
          return;
        }
    }
  
  ftn->pn = NULL;
}

/* ftn sorted by inactive, priority, tunnel-id.
   Within the same tunnel-id, ftn sorted by 
   pirmary, backup, secondary. */
static void
ftn_list_add (struct ftn_entry **ftn_list,
              struct ftn_entry *ftn, struct ftn_entry *prev_ftn)
{
  struct ftn_entry *tmp_ftn;

  if (prev_ftn)
    {
      for (tmp_ftn = prev_ftn; tmp_ftn; tmp_ftn = tmp_ftn->next)
        {
          if (! tmp_ftn->next ||
              tmp_ftn->next->tun_id != prev_ftn->tun_id ||
#ifdef HAVE_GMPLS
              ((tmp_ftn->flags & FTN_ENTRY_FLAG_BIDIR) !=
                  (prev_ftn->flags & FTN_ENTRY_FLAG_BIDIR))
              || ((tmp_ftn->flags & FTN_ENTRY_FLAG_FWD) !=
                  (prev_ftn->flags & FTN_ENTRY_FLAG_FWD)) ||
#endif /* HAVE_GMPLS */
              tmp_ftn->next->protected_lsp_id != prev_ftn->protected_lsp_id)
            break;
        }

      ftn->next = tmp_ftn->next;
      tmp_ftn->next = ftn;
    }
  else
    {
      ftn->next = *ftn_list;
      *ftn_list = ftn;
    }
}

struct ftn_entry *
mpls_ftn_lookup_inactive_tail (struct ftn_entry *ftn_head)
{
  struct ftn_entry *ftn;
  
  if (! ftn_head || ! CHECK_FLAG (ftn_head->flags, FTN_ENTRY_FLAG_INACTIVE))
    return NULL;

  for (ftn = ftn_head; ftn; ftn = ftn->next)
    {
      if (! ftn->next || 
          ! CHECK_FLAG (ftn->next->flags, FTN_ENTRY_FLAG_INACTIVE)) 
        return ftn;
    }

  return NULL;
}       

struct ftn_entry *
mpls_ftn_lookup_primary (struct ftn_entry *ftn_head, 
                         mpls_lsp_priority_t ftn_priority,
                         u_int32_t tunnel_id, u_int16_t flags,
                         struct ftn_entry **prev_ftn)
{
  struct ftn_entry *ftn;
  struct ftn_entry *ftn_tmp;
  mpls_lsp_priority_t priority;

  if (prev_ftn)
    *prev_ftn = NULL;

  for (ftn = ftn_head, ftn_tmp = NULL; ftn; ftn = ftn->next)
    {
      priority = gmpls_owner_priority (ftn->owner.owner);
      if (priority == ftn_priority)
        {
          if ((ftn->tun_id == tunnel_id)
#ifdef HAVE_GMPLS
              && ((flags & FTN_ENTRY_FLAG_BIDIR) ==
                  (ftn->flags & FTN_ENTRY_FLAG_BIDIR))
              && ((flags & FTN_ENTRY_FLAG_FWD) ==
                  (ftn->flags & FTN_ENTRY_FLAG_FWD))
#endif /* HAVE_GMPLS */
              )
            {
              if (prev_ftn)
                *prev_ftn = ftn_tmp;

              return ftn;
            }
        }
      else if (priority > ftn_priority)
        break;
      
      ftn_tmp = ftn;
    }


  if (prev_ftn)
    *prev_ftn = ftn_tmp;

  return NULL;
}

s_int32_t
gmpls_ilm_unselected_entry_process (struct nsm_master *nm, 
                                    struct ilm_entry *ilm)
{
  return 0;
}

s_int32_t
gmpls_ilm_selected_entry_process (struct nsm_master *nm, struct ilm_entry *ilm)
{
  struct prefix nh_p;
  struct fec_gen_entry fec;

  SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_SELECTED);
  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_DEPENDENT))
    {
      if (gmpls_set_dep_ilm_nhprefix (&nh_p, ilm))
        {
          fec.type = gmpls_entry_type_ip;
          fec.u.prefix = nh_p;
          gmpls_lsp_dep_add (nm, &fec, CONFIRM_RSVP_BYPASS_ILM, ilm);
        }
    }
  else
    {
      gmpls_ilm_add_entry_to_fwd_tbl (nm, ilm, NULL);
    }

  return 0;
}

static s_int32_t
gmpls_ftn_entry_add_list_primary (struct nsm_master *nm,
                                  struct ftn_entry **ftn_list, 
                                  struct ftn_entry *ftn, 
                                  struct ftn_entry *prev_ftn)
{
  mpls_lsp_priority_t ftn_priority;
  mpls_lsp_priority_t head_pri = MPLS_P_NONE;
  struct ftn_entry *ftn_head;

  if (! ftn)
    return NSM_ERR_INVALID_ARGS;

  ftn_head = gmpls_ftn_active_head (*ftn_list);
  if (ftn_head)
    head_pri = gmpls_owner_priority (ftn_head->owner.owner);

  ftn_priority = gmpls_owner_priority (ftn->owner.owner);
  
  if (ftn_priority <= head_pri)
    {
#ifdef HAVE_GMPLS
      if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_BIDIR))
        {
          if (nsm_gmpls_bidir_is_rev_dir_up (ftn))
            {
              /* Reverse ILM is UP */
              gmpls_ilm_selected_entry_process (nm, ftn->ilm_rev);
              SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED);
              FTN_XC_ADMN_STATUS_COUNT_INC (ftn);
            }
        }
      else
#endif /* HAVE_GMPLS */
        {
          SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED);
          FTN_XC_ADMN_STATUS_COUNT_INC (ftn);
        }
    }

  ftn_list_add (ftn_list, ftn, prev_ftn);

  return NSM_SUCCESS;
}

struct ftn_entry *
gmpls_ftn_entry_remove_list_primary (struct nsm_master *nm,
                                    struct ftn_entry **ftn_list,
                                    struct ftn_entry *ftn,
                                    struct ptree_node *pn,
                                    bool_t update_primary)
{
  bool_t update = update_primary;
  mpls_lsp_priority_t ftn_pri;
  struct ftn_entry *ftn_next;
    
  NSM_ASSERT (*ftn_list != NULL);
  if (! *ftn_list)
    return NULL;
  
  ftn_pri = gmpls_owner_priority (ftn->owner.owner);

  /* deselect ftn entry */
  gmpls_ftn_entry_deselect (nm, ftn, pn, update);

  /* get next ftn entry */
  ftn_next = ftn->next;

  /* remove ftn from list */
  ftn_list_delete_node (ftn_list, ftn);

  if (update)
    gmpls_ftn_delete_select_update (nm, ftn_pri, pn);

  return ftn;
}
 

static s_int32_t
gmpls_ftn_entry_add_list (struct nsm_master *nm,
                          struct ftn_entry **ftn_list, struct ftn_entry *ftn,
                          struct ptree_node *pn, bool_t primary_lsp)
{
  struct ftn_entry *ftn_head = NULL;
  struct ftn_entry *ftn_primary = NULL;
  bool_t pri_candidate;
  s_int32_t ret;
  struct ftn_entry *prev_ftn = NULL;
  mpls_lsp_priority_t ftn_priority;

  NSM_ASSERT (ftn != NULL);
  if (! ftn)
    return NSM_ERR_INVALID_ARGS;
  
  NSM_ASSERT (ftn->ftn_ix > 0);
  if (ftn->ftn_ix == 0)
    return NSM_ERR_INVALID_ARGS;

  /* if ftn is inactive, add it to head of the list */
  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INACTIVE))
    {
      ftn_list_add (ftn_list, ftn, NULL);
      return NSM_SUCCESS;
    }

  /* add as primary if empty list */
  if (! *ftn_list)
    {
      ret = gmpls_ftn_entry_add_list_primary (nm, ftn_list, ftn, NULL);
      if (ret != NSM_SUCCESS)
        return ret;
 
      ftn->igp_status = FTN_ENTRY_IGP_STATUS_CANDIDATE;

      return NSM_SUCCESS;
    }

  ftn_priority = gmpls_owner_priority (ftn->owner.owner);

  ftn_head = gmpls_ftn_active_head (*ftn_list);
  /* if there is no active head, set ftn_primary to NULL */ 
  if (! ftn_head)
      ftn_primary = NULL;
  else
    /* get primary entry with the same tunnel id and owner priority */
    ftn_primary = mpls_ftn_lookup_primary (ftn_head, ftn_priority, 
                                           ftn->tun_id, ftn->flags, 
                                           &prev_ftn);

  if (prev_ftn == NULL)
    prev_ftn = mpls_ftn_lookup_inactive_tail (*ftn_list);

  /*
    we need to apply fib selection policy to check 
    if we need to make this entry as primary 
  */
  pri_candidate = mpls_is_primary_candidate (ftn_primary, ftn, primary_lsp);
  if (pri_candidate == NSM_TRUE)
    {
      /* add as primary */
      ret = gmpls_ftn_entry_add_list_primary (nm, ftn_list, ftn, prev_ftn);
      if (ret != NSM_SUCCESS)
        {
          gmpls_ftn_delete_select_update (nm, ftn_priority, pn);
          return ret;
        }

      ftn->igp_status = FTN_ENTRY_IGP_STATUS_CANDIDATE;    
    }
  else
    {
      /* Backup/Detour second LSP all need install */
      if (CHECK_FLAG (NSM_MPLS->flags, NSM_MPLS_FLAG_INSTALL_BK_LSP))
        gmpls_ftn_entry_add_list_primary (nm, ftn_list, ftn, ftn_primary); 
      else
        ftn_list_add (ftn_list, ftn, ftn_primary);
    }
    
  return NSM_SUCCESS;
}

s_int32_t
gmpls_ftn_selected_entry_process (struct nsm_master *nm, struct ftn_entry *ftn)
{
  struct fec_gen_entry fec;
  s_int32_t ret;

  /* Get fec from FTN */
  nsm_gmpls_set_fec_from_ftn (ftn, &fec);

  gmpls_ftn_entry_remove_list (nm, (struct ftn_entry **)&ftn->pn->info,
                               ftn->ftn_ix, ftn->pn, NSM_TRUE);

  gmpls_ftn_entry_add_list (nm, (struct ftn_entry **)&ftn->pn->info,
                            ftn, ftn->pn, 
                            !!(ftn->flags & FTN_ENTRY_FLAG_PRIMARY));

  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED))
    {
      ret = gmpls_ftn_entry_select_process (nm, ftn, ftn->pn);
      if (ret != NSM_SUCCESS)
        return NSM_ERR_FTN_INSTALL_FAILURE;

      if (ftn->igp_status == FTN_ENTRY_IGP_STATUS_CANDIDATE)
        {
          if (ftn->next)
            gmpls_ftn_add_select_update (nm, ftn, ftn->pn);

          /* above gmpls_ftn_add_select_update might change igp_status */
          if (ftn->igp_status == FTN_ENTRY_IGP_STATUS_CANDIDATE &&
              ftn->lsp_bits.bits.igp_shortcut)
            {
              nsm_gmpls_set_fec_from_ftn (ftn, &fec);
              if (fec.type == gmpls_entry_type_ip)
                {
                  nsm_mpls_send_igp_shortcut_lsp (ftn, &fec.u.prefix,
                                                  NSM_MSG_IGP_SHORTCUT_ADD);

                  ftn->igp_status = FTN_ENTRY_IGP_STATUS_ADVERTISED;
                }
            }
          else if (ftn->igp_status != FTN_ENTRY_IGP_STATUS_ADVERTISED)
            {
              ftn->igp_status = FTN_ENTRY_IGP_STATUS_NONE;
            }
        }
    }

  return 0;
}

s_int32_t
gmpls_ftn_entry_remove_list_node (struct nsm_master *nm,
                                  struct ftn_entry **ftn_list,
                                  struct ftn_entry *ftn,
                                  struct ptree_node *pn,
                                  s_int32_t update_primary)
{
  NSM_ASSERT (ftn_list != NULL);
  if (! ftn_list)
    return NSM_ERR_INVALID_ARGS;

  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED))
    {
      if (! gmpls_ftn_entry_remove_list_primary (nm, ftn_list, ftn, 
                                                 pn, update_primary))
        return NSM_ERR_INTERNAL;
    }
  else
    ftn_list_delete_node (ftn_list, ftn);

  return NSM_SUCCESS;
}

struct ftn_entry *
gmpls_ftn_entry_remove_list (struct nsm_master *nm,
                             struct ftn_entry **ftn_list,
                             u_int32_t ftn_ix, 
                             struct ptree_node *pn,
                             bool_t update_primary)
{
  struct ftn_entry *ftn;

  NSM_ASSERT ((ftn_list != NULL) && (ftn_ix > 0));
  if ((! ftn_list) || (ftn_ix == 0))
    return NULL;

  for (ftn = *ftn_list; ftn; ftn = ftn->next)
    {
      if (ftn->ftn_ix == ftn_ix)
        {
          gmpls_ftn_entry_remove_list_node (nm, ftn_list, ftn,
                                            pn, update_primary);
          return ftn;
        }
    }
  
  return NULL;
}

static struct ftn_entry *
gmpls_ftn_entry_lookup_list (struct ftn_entry *ftn_list, u_int32_t ftn_ix)
{
  struct ftn_entry *ftn;

  NSM_ASSERT ((ftn_list != NULL) && (ftn_ix > 0));
  if ((! ftn_list) || (ftn_ix == 0))
    return NULL;

  for (ftn = ftn_list; ftn; ftn = ftn->next)
    {
      if (ftn->ftn_ix == ftn_ix)
        return ftn;
    }

  return NULL;
}


/* Add an entry to FTN table */
struct ptree_node *
gmpls_ftn_add (struct nsm_master *nm, struct ptree_ix_table *ft, 
               struct ftn_entry *ftn, struct fec_gen_entry *fec, 
               s_int32_t *status)
{
  struct ptree_node *pn;
  u_int32_t ftn_ix;
  s_int32_t ret;
  bool_t node_created = NSM_TRUE;
  u_char key[40];
  u_int16_t key_len;

  if ((! ft) || (! ftn))
    {
      *status = NSM_ERR_INVALID_ARGS;
      return NULL;
    }

  gmpls_ftn_node_generate_key (fec, key, &key_len);

  /* get tree node */
  pn = ptree_node_get (ft->m_table, key, key_len);
    
  if (! pn)
    {
      *status = NSM_ERR_MEM_ALLOC_FAILURE;
      return NULL;
    }

  if (pn->info)
    node_created = NSM_FALSE;

  /* get new ftn index */
  if (ftn->ftn_ix == 0)
    {
      ret = bitmap_request_index (ft->ix_mgr, &ftn_ix);
      if (ret < 0)
        {
          ptree_unlock_node (pn);
          *status = NSM_ERR_NO_TABLE_INDEX;
          return NULL;
        }

      ftn->ftn_ix = ftn_ix;
    }

  ret = gmpls_ftn_entry_add_list (nm, (struct ftn_entry **)&(pn->info), ftn,
                                  pn, CHECK_FLAG (ftn->flags,
                                  FTN_ENTRY_FLAG_PRIMARY));
  if (ret != NSM_SUCCESS)
    {
      /* release ftn index */
      if (ftn->ftn_ix > 0)
        {
          bitmap_release_index (ft->ix_mgr, ftn_ix);
          ftn->ftn_ix = 0;
        }

      ptree_unlock_node (pn);
      *status = ret;
      return NULL;
    }

  FTN_ROW_STATUS_ACTIVATE (ftn);
  
  if (! node_created)
    ptree_unlock_node (pn);

  *status = NSM_SUCCESS;
  return pn;
}

#ifdef HAVE_PACKET
struct ptree_node *
mpls_ftn_add (struct nsm_master *nm, struct ptree_ix_table *ft, 
              struct ftn_entry *ftn, struct prefix *pfx, s_int32_t *status)
{
  struct fec_gen_entry gfec;

  gfec.type = gmpls_entry_type_ip;
  gfec.u.prefix = *pfx;

  return gmpls_ftn_add (nm, ft, ftn, &gfec, status);
}
#endif /* HAVE_PACKET */

void
gmpls_ftn_add_reactivate (struct nsm_master *nm, 
                          struct ptree_ix_table *ftn_ix_tble, 
                          struct ftn_entry *ftn,
                          struct fec_gen_entry *fec)
{
#ifdef HAVE_SNMP
  struct nsm_mpls *nmpls = nm->nmpls;
#endif /* HAVE_SNMP */
  struct ptree_node *pn;
  s_int32_t ret;

  pn = gmpls_ftn_add (nm, ftn_ix_tble, ftn, fec, &ret); 
  if (pn == NULL)
    {
      gmpls_ftn_entry_cleanup (nm, ftn, fec, ftn_ix_tble);
      gmpls_ftn_free (ftn);
      return;
    }

  /* save pointer to ptree_node */
  ftn->pn = pn;

  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED))
    {
      ret = gmpls_ftn_entry_select_process (nm, ftn, pn);
      if (ret != NSM_SUCCESS)
        return;
    }

  FTN_XC_STATUS_UPDATE (ftn);
#ifdef HAVE_SNMP
  if (nmpls->notificationcntl == NSM_MPLS_NOTIFICATIONCNTL_ENA &&
      CHECK_FLAG (ftn->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG))
      nsm_mpls_opr_sts_trap (nmpls, ftn->xc->key.xc_ix, ftn->xc->opr_status);
#endif  /* HAVE_SNMP */
}

void
gmpls_ftn_node_generate_key (struct fec_gen_entry *fec,
                             u_char *key, u_int16_t *key_len)
{
#ifdef HAVE_PACKET
#endif /* HAVE_PACKET */
  switch (fec->type)
     {
#ifdef HAVE_PACKET
       case gmpls_entry_type_ip: 
         if (fec->u.prefix.family == AF_INET)
           {
             *key_len = fec->u.prefix.prefixlen;
             pal_mem_cpy (key, fec->u.prefix.u.val, IPV4_MAX_BYTELEN);
           }
#ifdef HAVE_IPV6
         else
           {
             *key_len = fec->u.prefix.prefixlen;
             pal_mem_cpy (key, fec->u.prefix.u.val, IPV6_MAX_BYTELEN);
           }
#endif /* HAVE_IPV6 */
         break; 

#endif /* HAVE_PACKET */
 
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
       case gmpls_entry_type_pbb_te:
         /* Keylength is bit length */
         *key_len = 8 * sizeof (struct fec_entry_pbb_te);
         pal_mem_cpy (key, &fec->u.pbb, sizeof (struct fec_entry_pbb_te));
         break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
       case gmpls_entry_type_tdm:
         /* Keylength is bit length */
         *key_len = 8 * sizeof (struct fec_entry_tdm);
         pal_mem_cpy (key, &fec->u.tdm, sizeof (struct fec_entry_tdm));
         break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

       default:
         break;
     }
  return;
}

struct ptree_node *
gmpls_ftn_node_lookup (struct ptree *pt, struct fec_gen_entry *ftn)
{
  struct ptree_node *pn;
  u_char key [40];
  u_int16_t key_len;

  if (! pt)
    return NULL;

  gmpls_ftn_node_generate_key (ftn, key, &key_len);

  /* lookup node */
  pn = ptree_node_lookup (pt, key, key_len);
  if (! pn)
    return NULL;
   
  ptree_unlock_node (pn);
 
  return pn;
}

bool_t
gmpls_labels_are_eq (struct gmpls_gen_label *label1, 
                     struct gmpls_gen_label *label2)
{
  s_int32_t ret = 0;

  if (label1->type != label2->type)
    return PAL_FALSE;

  switch (label1->type)
    {
      case gmpls_entry_type_ip:
        ret = (&label1->u.pkt == &label2->u.pkt) ? 1 : 0;
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        ret = pal_mem_cmp (&label1->u.pbb, &label2->u.pbb, 
                           sizeof (struct fec_entry_pbb_te));
        break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        ret = pal_mem_cmp (&label1->u.tdm, &label2->u.tdm, 
                           sizeof (struct fec_entry_tdm));
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        return PAL_FALSE;
    }

  if (ret != 0)
    {
      return PAL_FALSE;
    }

  return PAL_TRUE;
}

bool_t
gmpls_fec_are_eq (struct fec_gen_entry *fec1, struct fec_gen_entry *fec2)
{
  if (!fec1 || !fec2)
    return PAL_FALSE;

  if (fec1->type != fec2->type)
    return PAL_FALSE;

  switch (fec1->type)
    {
      case gmpls_entry_type_ip: 
        if (prefix_cmp (&fec1->u.prefix, &fec2->u.prefix) != 0)
          return PAL_FALSE;

        break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        if (pal_mem_cmp (&fec1->u.pbb, &fec2->u.pbb, 
                         sizeof (struct fec_entry_pbb_te)) != 0)
          return PAL_FALSE;
        break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        if (pal_mem_cmp (&fec1->u.tdm, &fec2->u.tdm, 
                         sizeof (struct fec_entry_tdm)) != 0)
          return PAL_FALSE;
        break;
#endif /* HAVE_TDM */
      
#endif /* HAVE_GMPLS */

      default:
        break;
    }

  return PAL_TRUE;
}

bool_t
gmpls_ftn_lookup_input (struct nsm_master *nm, struct ftn_entry *ftn, 
                        u_int32_t t_id, struct fec_gen_entry *fec,
                        struct gmpls_gen_label *label, 
                        struct pal_in4_addr nhop, char *ifname)
{
  struct interface *ifp = NULL;
  struct gmpls_gen_label tmp_lbl;
  int ret = 0, idx;
  struct addr_in nh_addr;
  struct fec_gen_entry fec_f;

  nsm_gmpls_set_fec_from_ftn (ftn, &fec_f);
  if (ftn->xc->nhlfe)
    {
      gmpls_nhlfe_outgoing_label (FTN_NHLFE (ftn), &tmp_lbl);
      ret = gmpls_nhlfe_nh_addr (ftn->xc->nhlfe, &nh_addr);
      idx = gmpls_nhlfe_outgoing_if (ftn->xc->nhlfe);
      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ftn->flags,
                              FTN_ENTRY_FLAG_GMPLS), idx, ifp);

      /* This function is only called from the CLI and we do not assume we 
         will allow clashes of tunnel Id from the CLI, that is why not checking
         the direction flag */
      if (ifp && (ftn->tun_id == t_id)
          && (gmpls_fec_are_eq (&fec_f, fec))
          && (gmpls_labels_are_eq (&tmp_lbl, label))
          && (IPV4_ADDR_SAME(&nh_addr.u.ipv4, &nhop))
          && (pal_strcmp (ifp->name, ifname) == 0 ))
        return PAL_TRUE;
    }

  return PAL_FALSE;
}


/* Lookup/Remove an entry from FTN table */
struct ftn_entry * 
nsm_gmpls_ftn_lookup (struct nsm_master *nm,
                      struct ptree_ix_table *pix_tbl, 
                      struct fec_gen_entry *fec,
                      u_int32_t ftn_ix, bool_t rem_ftn)
{
  struct ftn_entry *ftn;
  struct ptree_node *pn;
  
  NSM_ASSERT ((ftn_ix > 0) && (pix_tbl != NULL));
  if ((ftn_ix == 0) || (!pix_tbl))
    return NULL;
  
  /* lookup node */
  pn = gmpls_ftn_node_lookup (pix_tbl->m_table, fec);
  if (! pn)
    return NULL;
  
  if (rem_ftn)
    {
      ftn = gmpls_ftn_entry_remove_list (nm, (struct ftn_entry **)&pn->info,
                                         ftn_ix, pn, NSM_TRUE);

      if (pn->info == NULL)
        ptree_unlock_node (pn);
    }
  else
    ftn = gmpls_ftn_entry_lookup_list (pn->info, ftn_ix);
 
  return ftn;
}

#ifdef HAVE_RESTART 
struct ftn_entry *
nsm_gmpls_ftn_lookup_with_data (struct nsm_master *nm,
                                struct ptree_ix_table *pix_tbl,
                                struct fec_gen_entry *fec,
                                struct mpls_owner *owner)
{
  struct ftn_entry *ftn;
  struct ptree_node *pn;

  NSM_ASSERT (pix_tbl != NULL);
  if (! pix_tbl)
    return NULL;

  /* lookup node */
  pn = gmpls_ftn_node_lookup (pix_tbl->m_table, fec);
  if (! pn)
    return NULL;

  for (ftn = pn->info; ftn; ftn = ftn->next)
    {
      if (ftn->owner.owner != owner->owner)
        continue;

      switch (owner->owner)
        {
          case MPLS_LDP:
              {
                if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE))
                  {
                    UNSET_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE);
                    return ftn;
                  }
              }

          default:
            return NULL;
          }
      }
    return NULL;
}
#endif /* HAVE_RESTART */

struct ftn_entry *
nsm_gmpls_ftn_slow_lookup (struct nsm_master *nm,
                           struct ptree_ix_table *pix_tbl, 
                           struct ftn_add_gen_data *data, bool_t rem_flag)
{
  struct ftn_entry *ftn;
  struct ptree_node *pn;
  struct addr_in nh_addr;
  struct gmpls_gen_label tmp_lbl;
  s_int32_t ret;

  NSM_ASSERT ((data != NULL) && (pix_tbl != NULL));
  if ((! data) || (! pix_tbl))
    return NULL;

  pn = gmpls_ftn_node_lookup (pix_tbl->m_table, &data->ftn);
  if (! pn)
    return NULL;

  for (ftn = pn->info; ftn; ftn = ftn->next)
    {
      if (data->vrf_id != GLOBAL_FTN_ID)
        {
          if ((data->opcode == PUSH)  &&
              (ftn->xc->nhlfe->opcode != PUSH) &&
              (ftn->xc->nhlfe->opcode != PUSH_AND_LOOKUP))
            continue;
        }

      if (mpls_owner_same (&ftn->owner, &data->owner)
          && ftn->exp_bits == data->exp_bits
          && ftn->dscp_in == data->dscp_in
          && ftn->tun_id == data->tunnel_id
#ifdef HAVE_GMPLS
          && ((ftn->flags & FTN_ENTRY_FLAG_BIDIR) == 
              (data->flags & NSM_MSG_GEN_FTN_BIDIR))
          && ((ftn->flags & FTN_ENTRY_FLAG_FWD) == 
              (data->flags & NSM_MSG_GEN_FTN_FWD))
#endif /* HAVE_GMPLS */
          && ftn->protected_lsp_id == data->protected_lsp_id
          && (data->qos_resrc_id == 0 
              || ftn->qos_resrc_id == data->qos_resrc_id)
          && pal_mem_cmp (&ftn->xc->lspid, &data->lspid, 
                          sizeof (struct mpls_lspid)) == 0
          && ftn->lsp_bits.bits.act_type == data->lsp_bits.bits.act_type
          && gmpls_nhlfe_outgoing_if (ftn->xc->nhlfe) == data->oif_ix)
       {
          gmpls_nhlfe_outgoing_label (ftn->xc->nhlfe, &tmp_lbl);
          if (!gmpls_labels_are_eq (&tmp_lbl, &data->out_label))
            continue;

          if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_PRIMARY))
            {
              if (data->lsp_bits.bits.lsp_type != LSP_TYPE_PRIMARY &&
                  data->lsp_bits.bits.lsp_type != LSP_TYPE_BYPASS)
                continue;       
            }
          else
            {
              if (data->lsp_bits.bits.lsp_type == LSP_TYPE_PRIMARY || 
                  data->lsp_bits.bits.lsp_type == LSP_TYPE_BYPASS)
                continue;
            }

          ret = gmpls_nhlfe_nh_addr (ftn->xc->nhlfe, &nh_addr);
          if ((ret == NSM_SUCCESS) &&
              (gmpls_addr_in_same (&nh_addr, &data->nh_addr))) 
            {
              if (ftn->pri_fec_prefix && data->pri_fec_prefix)
                {
                  if (! prefix_same (ftn->pri_fec_prefix, data->pri_fec_prefix))
                    continue;
                }
              else if (ftn->pri_fec_prefix && ! data->pri_fec_prefix)
                continue;
              else if (! ftn->pri_fec_prefix && data->pri_fec_prefix)
                continue;

              if (rem_flag)
                {
                  gmpls_ftn_entry_remove_list_node (nm, 
                                               (struct ftn_entry **)&pn->info, 
                                               ftn, pn, NSM_TRUE);

                  if (pn->info == NULL)
                    ptree_unlock_node (pn);
                }

              return ftn;
            }
        }
    }
  return NULL;
}

/* Get first active ilm in ilm list */
struct ilm_entry *
mpls_ilm_active_head (struct ilm_entry *ilm_list)
{
  struct ilm_entry *ilm_head = ilm_list;

  while (ilm_head)
    {
      if (! CHECK_FLAG (ilm_head->flags, ILM_ENTRY_FLAG_INACTIVE))
        break;

      ilm_head = ilm_head->next;
    }

  return ilm_head;
}

struct ilm_entry *
mpls_ilm_lookup_inactive_tail (struct ilm_entry *ilm_head)
{
  struct ilm_entry *ilm;

  if (! ilm_head || ! CHECK_FLAG (ilm_head->flags, ILM_ENTRY_FLAG_INACTIVE))
    return NULL;

  for (ilm = ilm_head; ilm; ilm = ilm->next)
    {
      if (! ilm->next ||
          ! CHECK_FLAG (ilm->next->flags, ILM_ENTRY_FLAG_INACTIVE))
        return ilm;
    }

  return NULL;
}

struct ilm_entry *
gmpls_ilm_entry_lookup_list (struct nsm_master *nm,
                            struct ilm_entry **ilm_head, u_int32_t ilm_ix,
                            bool_t rem_flag, bool_t update_primary)
{
  struct ilm_entry *ilm, *ilm_prev;
  struct prefix nh_p;
  struct fec_gen_entry fec;
  s_int32_t ret;
#ifdef HAVE_GMPLS
  struct ilm_entry *rev_ilm;
  struct ftn_entry *rev_ftn;
#endif /* HAVE_GMPLS */

  if (! ilm_head || ! *ilm_head)
    return NULL;

  for (ilm = *ilm_head, ilm_prev = NULL; ilm; ilm = ilm->next)
    {
      if (ilm_ix > 0 && ilm->ilm_ix != ilm_ix)
        {
          ilm_prev = ilm;
          continue;
        }

      if (! rem_flag)
        return ilm;

      if (ilm_prev)
        {
          ilm_prev->next = ilm->next;
          ilm->next = NULL;
          
          return ilm;
        }

      if (update_primary)
        {
          if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_DEPENDENT))
            {
              if (gmpls_set_dep_ilm_nhprefix (&nh_p, ilm))
                {
                  fec.type = gmpls_entry_type_ip;
                  fec.u.prefix = nh_p;

                  gmpls_lsp_dep_del (nm, &fec, CONFIRM_RSVP_BYPASS_ILM, ilm);
                }
            }
          else
            {
#ifdef HAVE_GMPLS
              /* Unlink bidirectional entries */
              if ((CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_BIDIR)) &&
                  (ilm->rev_entry != NULL))
                {
                  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_REV_FTN))
                    {
                      rev_ftn = (struct ftn_entry *) ilm->rev_entry;
                      rev_ftn->ilm_rev = NULL;
                      gmpls_ftn_selected_entry_process (nm, rev_ftn);
                      UNSET_FLAG (rev_ftn->flags, FTN_ENTRY_FLAG_SELECTED);
                    }
                  else
                    {
                      rev_ilm = (struct ilm_entry *) ilm->rev_entry;
                      rev_ilm->rev_entry = NULL;
                      gmpls_ilm_unselected_entry_process (nm, rev_ilm);
                      UNSET_FLAG (rev_ilm->flags, ILM_ENTRY_FLAG_SELECTED);
                   }

                  ilm->rev_entry = NULL;
                }

#endif /* HAVE_GMPLS */

              /* deselect existing primary */
              gmpls_ilm_del_entry_from_fwd_tbl (nm, ilm);
            }

          if (ilm->next)
            {
              /* select new primary */
              SET_FLAG (ilm->next->flags, ILM_ENTRY_FLAG_SELECTED);

              /* add to fib */
              if (CHECK_FLAG (ilm->next->flags, ILM_ENTRY_FLAG_DEPENDENT))
                {
                  if (gmpls_set_dep_ilm_nhprefix (&nh_p, ilm->next))
                    {
                      fec.type = gmpls_entry_type_ip;
                      fec.u.prefix = nh_p;
                      gmpls_lsp_dep_add (nm, &fec, CONFIRM_RSVP_BYPASS_ILM, 
                                         ilm->next);
                    }
                }
              else
                {
                  ret = gmpls_ilm_add_entry_to_fwd_tbl (nm, ilm->next, NULL);
                }
            }
        }
      
      *ilm_head = ilm->next;
      ilm->next = NULL;
      
      return ilm;
    }
  
  return NULL;
}

s_int32_t
gmpls_ilm_entry_add_list (struct nsm_master *nm,
                          struct ilm_entry **ilm_head, struct ilm_entry *ilm)
{
  struct ilm_entry *ilm_ahead, *ilm_inactive;
  struct prefix nh_p;
  struct fec_gen_entry fec;
#ifdef HAVE_GMPLS
  bool_t select = PAL_FALSE;
#endif /* HAVE_GMPLS */

  if (! ilm_head || ! ilm)
    return NSM_ERR_INVALID_ARGS;

  /* if ilm entry is inactive, add it to head */
  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_INACTIVE))
    {
      if (*ilm_head == ilm)
        {
          ilm->next = NULL;
          *ilm_head = ilm;
          return NSM_SUCCESS;
        }
      else
        {
          ilm->next = *ilm_head;
          *ilm_head = ilm;
          return NSM_SUCCESS;
        }
    }

  ilm_ahead = mpls_ilm_active_head (*ilm_head);
  /* find last inactive entry */
  ilm_inactive = mpls_ilm_lookup_inactive_tail (*ilm_head);

  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_PRIMARY) || ! ilm_ahead)
    {
      /* deselect old primary entry if new entry is active */
      if (ilm_ahead)
        {
          /* delete existing ilm entry from fwd */
          if (CHECK_FLAG (ilm_ahead->flags, ILM_ENTRY_FLAG_DEPENDENT))
            {
              if (gmpls_set_dep_ilm_nhprefix (&nh_p, ilm_ahead))
                {
                  /* set fec from prefix */
                  fec.type = gmpls_entry_type_ip;
                  fec.u.prefix = nh_p;
                  gmpls_lsp_dep_del (nm, &fec, CONFIRM_RSVP_BYPASS_ILM, 
                                     ilm_ahead);
                }
            }
          else
            {
              gmpls_ilm_del_entry_from_fwd_tbl (nm, ilm_ahead);
            }
        }

      /* if there are some inactive entries, add it at the end of inactive 
         entry */
      if (ilm_inactive)
        {
          ilm->next = ilm_inactive->next;
          ilm_inactive->next = ilm;
        }
      else
        {
          if (*ilm_head == ilm)
            ilm->next = NULL;
          else
            ilm->next = *ilm_head;

          *ilm_head = ilm;
        }

      /* mark entry as selected */
#ifdef HAVE_GMPLS
      if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_BIDIR))
        {
          if (nsm_gmpls_bidir_is_rev_dir_up_ilm (ilm) == NSM_TRUE)
            {
              if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_REV_FTN))
                {
                  gmpls_ftn_selected_entry_process (nm, 
                                          (struct ftn_entry *) ilm->rev_entry);
                  if (CHECK_FLAG (((struct ftn_entry *) ilm->rev_entry)->flags,
                                  FTN_ENTRY_FLAG_SELECTED))
                    {
                      select = PAL_TRUE;
                    }
                }
              else
                {
                  gmpls_ilm_selected_entry_process (nm, 
                                          (struct ilm_entry *) ilm->rev_entry);
                  if (CHECK_FLAG (((struct ilm_entry *) ilm->rev_entry)->flags,
                                    ILM_ENTRY_FLAG_SELECTED))
                    {
                      select = PAL_TRUE;
                    }
                }

              if (select == PAL_TRUE)
                SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_SELECTED);
            }
        }
      else
#endif /* HAVE_GMPLS */
        {
          SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_SELECTED);
        }
    }
  /* add it to the back of active head */
  else
    {
      ilm->next = ilm_ahead->next;
      ilm_ahead->next = ilm;
    }
  
  return NSM_SUCCESS;
}

void
gmpls_ilm_add_reactivate(struct nsm_master *nm, struct avl_node *an,
                        struct ilm_entry **ilm_head, struct ilm_entry *ilm)
{
  s_int32_t ret;
  struct prefix nh_p;
  struct fec_gen_entry fec;

  ret = gmpls_ilm_entry_add_list (nm, ilm_head, ilm);
  if (ret < 0)
    {
      if (ILM_IX (ilm) > 0)
        {
          bitmap_release_index (ILM_IX_MGR, ilm->ilm_ix);
          ILM_IX (ilm) = 0;
        }

      return;
    }
  
  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_DEPENDENT))
    {
      if (gmpls_set_dep_ilm_nhprefix (&nh_p, ilm))
        {
          fec.type = gmpls_entry_type_ip;
          fec.u.prefix = nh_p;
          gmpls_lsp_dep_add (nm, &fec, CONFIRM_RSVP_BYPASS_ILM, ilm);
        }
    }
  else
    {
      gmpls_ilm_add_entry_to_fwd_tbl (nm, ilm, NULL);
    }

}

/* gmpls_ilm_add is split into 2 functions for SNMP support */
/* The 2 functions are: gmpls_ilm_create and gmpls_ilm_add_entry_to_fwd_tbl */

int 
gmpls_ilm_create (struct nsm_master *nm, struct ilm_entry *ilm)
{
  struct avl_node *an = NULL;
  u_int32_t ilm_ix;
  s_int32_t ret = 0;
  bool_t new_ent = PAL_FALSE;

  NSM_ASSERT (ilm != NULL);
  if (! ilm)
    return NSM_ERR_INVALID_ARGS;

  if (ilm->ent_type == ILM_ENTRY_TYPE_PACKET && ILM_TABLE)
    {
      an = avl_search (ILM_TABLE, ilm);
      if (an == NULL)
        {
          new_ent = PAL_TRUE;
          an = avl_insert_node (ILM_TABLE, ilm);
        }

      if (an == NULL)
        return NSM_ERR_MEM_ALLOC_FAILURE;
    }
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  else if (ilm->ent_type == ILM_ENTRY_TYPE_PBB_TE && ILM_PBB_TABLE)
    {
      an = avl_search (ILM_PBB_TABLE, ilm);
      if (an == NULL)
        {
          new_ent = PAL_TRUE;
          an = avl_insert_node (ILM_PBB_TABLE, ilm);
        }

      if (an == NULL)
        return NSM_ERR_MEM_ALLOC_FAILURE;
    }
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
  else if (ilm->ent_type == ILM_ENTRY_TYPE_TDM && ILM_TDM_TABLE)
    {
      an = avl_search (ILM_TDM_TABLE, ilm);
      if (an == NULL)
        {
          new_ent = PAL_TRUE;
          an = avl_insert_node (ILM_TDM_TABLE, ilm);
        }

      if (an == NULL)
        return NSM_ERR_MEM_ALLOC_FAILURE;
    }
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

  ilm_ix = ILM_IX (ilm);

  /* get ilm index */
  if (ilm_ix == 0)
    {
      ret = bitmap_request_index (ILM_IX_MGR, &ilm_ix);
      if (ret < 0 || ilm_ix == 0)
        {
          return NSM_ERR_NO_TABLE_INDEX;
        }
    }

  /* set ilm index */
  ILM_IX (ilm) = ilm_ix;
  
  if (ilm->xc)
    ilm->xc->ilm_ix = ilm_ix;

  /* add ilm entry to list */
  if (an)
    {
      if (new_ent == PAL_FALSE)
        {
          ret = gmpls_ilm_entry_add_list (nm, (struct ilm_entry **)&(an->info), 
                                          ilm);
        }
      else
        {
          an->info = NULL;
          ret = gmpls_ilm_entry_add_list (nm, (struct ilm_entry **)&(an->info), 
                                          ilm);
        }
    }

  if (ret < 0)
    {
      if (ILM_IX (ilm) > 0)
        {
          bitmap_release_index (ILM_IX_MGR, ilm_ix);
          ILM_IX (ilm) = 0;
        }
      
      return ret;
    }
  
  ilm->an = an;

  if (an)
    avl_lock_node (an);

  return NSM_SUCCESS;
}

int 
gmpls_ilm_add_entry_to_fwd_tbl (struct nsm_master *nm, struct ilm_entry *ilm,
                               struct ftn_entry *tunnel_ftn)
{
#ifdef HAVE_SNMP
  struct nsm_mpls *nmpls = nm->nmpls;
#endif  /* HAVE_SNMP */
  u_int32_t ilm_ix = 0;
  s_int32_t ret = 0;

  ilm_ix = ILM_IX (ilm);

  if ((CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_SELECTED)) &&
     !(CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_INSTALLED)))
  {
    /* GELS : Calling the PBB_TE API instead of creating entry in the
     *        MPLS forwarder
     */
#if defined HAVE_GMPLS && defined HAVE_PBB_TE
    if(ilm->ent_type == ILM_ENTRY_TYPE_PBB_TE)
    {
      struct interface *ifp = NULL;

      /* Get the interface structure from the ifindex */
      NSM_MPLS_GET_INDEX_PTR (PAL_TRUE, ilm->key.u.pbb_key.iif_ix, ifp);
      if (ifp == NULL)
        return NSM_ERR_ILM_INSTALL_FAILURE;

#ifdef HAVE_GELS
      /* Call the PBB-TE API */
      if(ilm->xc->nhlfe->opcode == POP)
      {
#if defined HAVE_I_BEB && defined HAVE_B_BEB
        ret = nsm_pbb_te_create_dynamic_esp(ifp,
                 ((struct gmpls_tgen_data *)ilm->tgen_data)->u.pbb.tesid,
                 ilm->key.u.pbb_key.in_label.bmac,
                 ilm->key.u.pbb_key.in_label.bvid, PAL_FALSE);
#endif /* HAVE_I_BEB && HAVE_B_BEB */
      }
      else if(ilm->xc->nhlfe->opcode == SWAP)
      {
        ret = nsm_pbb_te_bcb_action_create(ifp,
                               ilm->key.u.pbb_key.in_label.bmac,
                               ilm->key.u.pbb_key.in_label.bvid);
      }
#endif /* HAVE_GELS */

    }
    else
#endif  /* HAVE_GMPLS && HAVE PBB_TE */
    {
      /* add to fib */
      ret = nsm_mpls_ilm_add_to_fwd (nm, ilm, tunnel_ftn);
    }

    if (ret != NSM_SUCCESS)
      return NSM_ERR_ILM_INSTALL_FAILURE;

    SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_INSTALLED);
  }

  ILM_ROW_STATUS_ACTIVATE (ilm);

#ifdef HAVE_SNMP
  if (ilm->xc && 
      nmpls->notificationcntl == NSM_MPLS_NOTIFICATIONCNTL_ENA &&
      CHECK_FLAG (ilm->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG)) 
    nsm_mpls_opr_sts_trap (nmpls, ilm->xc->key.xc_ix, ilm->xc->opr_status);
#endif  /* HAVE_SNMP */
  return NSM_SUCCESS;
}

s_int32_t
gmpls_ilm_del_entry_from_fwd_tbl (struct nsm_master *nm, struct ilm_entry *ilm)
{
#ifdef HAVE_SNMP
    struct nsm_mpls *nmpls = nm->nmpls;
#endif  /* HAVE_SNMP */

  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_INSTALLED))
  {
    /* GELS : Calling the PBB_TE API instead of deleteing entry in the
     *        MPLS forwarder
     */
#if defined HAVE_GMPLS && defined HAVE_PBB_TE
    if(ilm->ent_type == ILM_ENTRY_TYPE_PBB_TE)
    {
      struct interface *ifp = NULL;

      /* Get the interface structure from the ifindex */
      NSM_MPLS_GET_INDEX_PTR (PAL_TRUE, ilm->key.u.pbb_key.iif_ix, ifp);

      if (!ifp)
        return NSM_FAILURE;

#ifdef HAVE_GELS
      /* Call the PBB-TE API */
       if(ilm->xc->nhlfe->opcode == POP)
       {
#if defined HAVE_I_BEB && defined HAVE_B_BEB
         nsm_pbb_te_delete_dynamic_esp(ifp,
                ((struct gmpls_tgen_data *)ilm->tgen_data)->u.pbb.tesid,
                ilm->key.u.pbb_key.in_label.bmac,
                ilm->key.u.pbb_key.in_label.bvid, PAL_FALSE);
#endif /* HAVE_I_BEB && HAVE_B_BEB */
        }
        else if(ilm->xc->nhlfe->opcode == SWAP)
        {
          nsm_pbb_te_bcb_action_delete(ifp,
                                 ilm->key.u.pbb_key.in_label.bmac,
                                 ilm->key.u.pbb_key.in_label.bvid);
        }
#endif /* HAVE_GELS */
        UNSET_FLAG (ilm->flags, ILM_ENTRY_FLAG_INSTALLED);
    }
    else
#endif  /* HAVE PBB_TE && HAVE_GMPLS */
    {
        MPLS_ILM_FWD_DEL (nm, ilm);
    }
  }

  UNSET_FLAG (ilm->flags, ILM_ENTRY_FLAG_SELECTED);
  ILM_ROW_STATUS_ACTIVATE(ilm);

#ifdef HAVE_GMPLS
  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_BIDIR))
    {
      if (ilm->rev_entry != NULL)
        {
          if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_REV_FTN))
            {
              /* FTN entry unselect */
              gmpls_ftn_selected_entry_process (nm, 
                                           (struct ftn_entry *) ilm->rev_entry);
            }
          else
            {
              /* ILM entry unselect */
              gmpls_ilm_unselected_entry_process (nm, 
                                           (struct ilm_entry *) ilm->rev_entry);
            }
        }
    }
#endif /* HAVE_GMPLS */

#ifdef HAVE_SNMP
    if (ilm->xc &&
        nmpls->notificationcntl == NSM_MPLS_NOTIFICATIONCNTL_ENA &&
        CHECK_FLAG (ilm->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG))
      nsm_mpls_opr_sts_trap (nmpls, ilm->xc->key.xc_ix, ilm->xc->opr_status);
#endif /* HAVE_SNMP */

    return NSM_SUCCESS;
}


struct ilm_entry *
nsm_gmpls_ilm_lookup_by_owner (struct nsm_master *nm,
                               u_int32_t iif_ix,
                               struct gmpls_gen_label *in_label,
                               mpls_owner_t owner)
{
  struct ilm_entry *ilm, *ilm_start;

  ilm_start = gmpls_ilm_node_lookup (nm, iif_ix, in_label);
  if (! ilm_start)
    return NULL;

  for (ilm = ilm_start; ilm; ilm = ilm->next)
    {
      if (ilm->owner.owner == owner)
      return ilm;
    }

  return NULL;
}

struct ilm_entry *
nsm_gmpls_ilm_lookup (struct nsm_master *nm,
                      u_int32_t iif_ix, struct gmpls_gen_label *in_label,
                      u_int32_t ilm_ix, bool_t rem_flag,
                      bool_t update_primary)
{
  struct ilm_entry *ilm, *ilm_start;
  struct avl_tree *avl_t = NULL;

  ilm_start = gmpls_ilm_node_lookup (nm, iif_ix, in_label);

  ilm = gmpls_ilm_entry_lookup_list (nm, (struct ilm_entry **)&ilm_start,
                                     ilm_ix, rem_flag, update_primary);
  if (ilm && rem_flag)
    {
      switch (in_label->type)
        {
        case gmpls_entry_type_ip:
          avl_t = ILM_TABLE;
          break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
        case gmpls_entry_type_pbb_te:
          avl_t = ILM_PBB_TABLE;
          break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
        case gmpls_entry_type_tdm:
          avl_t = ILM_TDM_TABLE;
          break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

        default:
          break;
        }

      if (avl_is_last_node (ilm->an))
        {
          if (avl_t)
            {
              avl_delete_node (avl_t, ilm->an);
              ilm->an = NULL;
            }
        }
      else
        {
          ilm->an->info = ilm_start;
          avl_unlock_node (ilm->an);
        }
    }

  return ilm;
}

#ifdef HAVE_PACKET
struct ilm_entry *
nsm_mpls_ilm_lookup (struct nsm_master *nm,
                     u_int32_t iif_ix, u_int32_t in_label, 
                     u_int32_t ilm_ix, bool_t rem_flag,
                     bool_t update_primary)
{
  struct ilm_entry *ilm;
  struct gmpls_gen_label label;

  label.type = gmpls_entry_type_ip;
  label.u.pkt = in_label;

  ilm = nsm_gmpls_ilm_lookup (nm, iif_ix, &label, ilm_ix, rem_flag, 
                              update_primary);
     
  return ilm;
}

#endif /*  HAVE_PACKET */

struct ilm_entry *
nsm_gmpls_ilm_lookup_exact (struct nsm_master *nm,
                            struct ilm_add_gen_data *data,
                            bool_t rem_flag,
                            bool_t update_primary)
{
  struct ilm_entry *ilm = NULL, *ilm_start;
  struct avl_node *an;
  u_int32_t ilm_ix = 0;
  struct addr_in nh_addr;
  struct gmpls_gen_label lbl;
  s_int32_t ret;

  NSM_ASSERT (data != NULL);
  if (! data)
    return NULL;

  ilm_start = gmpls_ilm_node_lookup (nm, data->iif_ix, &data->in_label);
  if (! ilm_start)
    return NULL;

  an = ilm_start->an;
  for (ilm = ilm_start; ilm; ilm = ilm->next)
    {
      gmpls_nhlfe_outgoing_label (ilm->xc->nhlfe, &lbl);
      if (ilm->xc && mpls_owner_same (&ilm->owner, &data->owner) &&
          pal_mem_cmp (&ilm->xc->lspid, &data->lspid,
                       sizeof (struct mpls_lspid)) == 0 &&
          gmpls_nhlfe_outgoing_if (ilm->xc->nhlfe) == data->oif_ix &&
          gmpls_are_label_equal (&lbl, &data->out_label) && 
          data->primary == FLAG_ISSET (ilm->flags, ILM_ENTRY_FLAG_PRIMARY) &&
          data->fec_prefix.family == ilm->family &&
          data->fec_prefix.prefixlen == ilm->prefixlen)
        {
#ifdef HAVE_IPV6
          if (data->fec_prefix.family == AF_INET6)
            {
              if (pal_mem_cmp (&data->fec_prefix.u.prefix6,
                  ilm->prfx, sizeof (struct pal_in6_addr)) != 0)
                continue;
            }
          else
#endif
            {
              if (pal_mem_cmp (&data->fec_prefix.u.prefix4, 
                  ilm->prfx, sizeof (struct pal_in4_addr)) != 0)
                continue;
            }

          ret = gmpls_nhlfe_nh_addr (ilm->xc->nhlfe, &nh_addr);
          if (ret == NSM_SUCCESS &&
              gmpls_addr_in_same (&nh_addr, &data->nh_addr))
            {
              ilm_ix = ilm->ilm_ix;
              break;
            }
        }
    }

  if (ilm_ix == 0)
    return NULL;

  if (rem_flag == NSM_FALSE)
    return ilm;
      
  /* remove ilm entry */
  ilm = gmpls_ilm_entry_lookup_list (nm, (struct ilm_entry **)&ilm_start, 
                                    ilm_ix, NSM_TRUE, update_primary);

  if (an)
    an->info = ilm_start;
  return ilm;
}

struct ilm_entry *
nsm_gmpls_ilm_lookup_exact_pfx (struct nsm_master *nm,
                                struct ilm_add_data *data,
                                bool_t rem_flag,
                                bool_t update_primary)
{
  struct ilm_add_gen_data gilm;
  struct interface *intf = NULL;

  nsm_gmpls_set_ilm_gen_data (&gilm, data);
  NSM_MPLS_GET_INDEX_PTR (PAL_FALSE, gilm.oif_ix, intf);
  NSM_MPLS_GET_INDEX_PTR (PAL_FALSE, gilm.iif_ix, intf);
#ifdef HAVE_GMPLS
  if (intf)
    {
      gilm.iif_ix = intf->gifindex;
    }
  else
#endif /* HAVE_GMPLS */
    {
      gilm.iif_ix = data->iif_ix;
    }


  return nsm_gmpls_ilm_lookup_exact (nm, &gilm, rem_flag, update_primary);
}

s_int32_t
gmpls_xc_add (struct nsm_master *nm, struct xc_entry *xc)
{
  struct avl_node *an;
  u_int32_t xc_ix;
  bool_t ix_created = NSM_FALSE;
  s_int32_t ret = 0;

  NSM_ASSERT (xc != NULL);
  if (! xc)
    return NSM_ERR_INVALID_ARGS;

  xc_ix = XC_IX (xc);
  
  if (xc_ix == 0)
    {
      /* get new xc index */
      ret = bitmap_request_index (XC_IX_MGR, &xc_ix);
      if ((ret < 0) || (xc_ix == 0))
        return NSM_ERR_NO_TABLE_INDEX;

      ix_created = NSM_TRUE;
    }
  
  XC_IX (xc) = xc_ix;
  
  an = avl_search (XC_TABLE, xc);
  if (an == NULL)
    {
      /* Insert the entry */ 
      an  = avl_insert_node (XC_TABLE, xc);

      if (an == NULL)
        {
          if (ix_created)
            bitmap_release_index (XC_IX_MGR, xc_ix);

          XC_IX (xc) = 0;
          return NSM_ERR_MEM_ALLOC_FAILURE;
        }

      /* "an" will never be NULL */
      avl_lock_node (an);
    }   

  if ((xc->nhlfe) && (xc->nhlfe->xc_ix != xc_ix))
    xc->nhlfe->xc_ix = xc_ix;
  
  return NSM_SUCCESS;
}


struct xc_entry *
gmpls_xc_node_lookup (struct nsm_master *nm,
                      u_int32_t xc_ix, u_int32_t iif_ix, 
                      struct gmpls_gen_label *in_label, u_int32_t nhlfe_ix)
{
  struct avl_node *node;
  struct xc_entry tmp_entry, *xc;

  NSM_ASSERT (xc_ix != 0);
  if (xc_ix == 0)
    return NULL;

  gmpls_make_xc_key (xc_ix, iif_ix, in_label, nhlfe_ix, &tmp_entry.key);

  node = avl_search (XC_TABLE, &tmp_entry);
  if (node == NULL)
    return NULL;
  
  xc = node->info;

  if (! xc)
    return NULL;
    
  return xc;
}

struct xc_entry *
nsm_gmpls_xc_lookup (struct nsm_master *nm,
                     u_int32_t xc_ix, u_int32_t iif_ix, 
                     struct gmpls_gen_label *in_label, 
                     u_int32_t nhlfe_ix, bool_t rem_flag)
{
  struct avl_node *an;
  struct xc_entry tmp_entry, *xc;

  NSM_ASSERT (xc_ix != 0);

  if (xc_ix == 0)
    return NULL;

/* fix provided is temporary as the avl_search is not returning */
/* the expected value. The avl_search needs to be re-looked     */

/*
    gmpls_make_xc_key (xc_ix, iif_ix, in_label, nhlfe_ix, &tmp_entry.key);
    an = avl_search (XC_TABLE, &tmp_entry);
     if (an == NULL)
       return NULL;

    xc = an->info;
*/

  for (an = avl_top (XC_TABLE); an; an = avl_next (an))
    {
      if (an->info != NULL)
        {
          xc = (struct xc_entry *) an->info;
          if (xc->key.xc_ix == xc_ix &&
              xc->key.iif_ix == iif_ix &&
              gmpls_are_label_equal (&xc->key.in_label, in_label) &&
              xc->key.nhlfe_ix == nhlfe_ix)
               break;
        }
    }

  if (an == NULL)
    return NULL;

  if (rem_flag)
    {
      if (avl_is_last_node (an))
        {
          avl_delete_node (XC_TABLE, an);
        }
      else
        {
          avl_unlock_node (an);
          an->info = NULL;
        }
    }

  return xc;
}

#ifdef HAVE_PACKET
struct xc_entry *
nsm_mpls_xc_lookup (struct nsm_master *nm, u_int32_t xc_ix, u_int32_t iif_ix, 
                    u_int32_t in_label, u_int32_t nhlfe_ix, bool_t rem_flag)
{
  struct gmpls_gen_label lbl;
  lbl.type = gmpls_entry_type_ip;
  lbl.u.pkt = in_label;

  return nsm_gmpls_xc_lookup (nm, xc_ix, iif_ix, &lbl, nhlfe_ix, rem_flag);
}
#endif /* HAVE_PACKET */
/* InSeg Interface and Label based XC Lookup */
struct xc_entry *
nsm_gmpls_ilm_key_xc_lookup (struct nsm_master *nm, u_int32_t iif_ix, 
                             struct gmpls_gen_label *in_label, u_int32_t ilm_ix)
{
  struct avl_node *an = NULL;
  struct xc_entry *xc = NULL;

  if (! XC_TABLE)
    return NULL;

  for (an = avl_top (XC_TABLE); an; an = avl_next (an))
    {
      if (an->info != NULL)
        {
          xc = (struct xc_entry *) an->info;
          if (ilm_ix)
            {
              if (xc->ilm_ix == ilm_ix)
                {
                  return xc;
                }
            }
          else
            {
              if (xc->key.iif_ix == iif_ix && 
                  gmpls_are_label_equal (&xc->key.in_label, in_label))
                {
                  return xc;
                }
            }
        }
    }

  return NULL;
}

#ifdef HAVE_PACKET
struct xc_entry *
nsm_mpls_ilm_key_xc_lookup (struct nsm_master *nm, u_int32_t iif_ix,
                            u_int32_t in_label, u_int32_t ilm_ix)
{
  struct gmpls_gen_label lbl;

  lbl.type = gmpls_entry_type_ip;
  lbl.u.pkt = in_label;

  return nsm_gmpls_ilm_key_xc_lookup (nm, iif_ix, &lbl, ilm_ix);
}
#endif /* HAVE_PACKET */

struct nhlfe_entry *
gmpls_nhlfe_get_entry (struct nsm_master *nm, struct nhlfe_entry *nhlfe)
{
  struct avl_node *an = NULL;
  struct nhlfe_key *key;

  switch (nhlfe->type)
    {
      case gmpls_entry_type_ip:
        key = (struct nhlfe_key *) nhlfe->nkey;

        if ((key->afi == AFI_IP)
#ifdef HAVE_GMPLS
            || (key->afi == AFI_UNNUMBERED)
#endif /* HAVE_GMPLS */
            )
          {
            an = avl_search (NHLFE_TABLE4, nhlfe);
            if (an == NULL)
              {
                /* Node not already present add the new node now */
                an = avl_insert_node (NHLFE_TABLE4, nhlfe);
                if (an == NULL)
                  return NULL;

                /* "an" shuld be Non-NULL*/
                avl_lock_node (an);
              }
           }

#ifdef HAVE_IPV6
        if (key->afi == AFI_IP6)
          {
            an = avl_search (NHLFE_TABLE6, nhlfe);
            if (an == NULL)
              {
                /* Node not already present add the new node now */
                an = avl_insert_node (NHLFE_TABLE6, nhlfe);
                if (an == NULL)
                  return NULL;

                /* "an" shuld be Non-NULL*/
                avl_lock_node (an);
              }
          }
#endif /* HAVE_IPV6*/
        break;


#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        an = avl_search (NHLFE_PBB_TABLE, nhlfe);
        if (an == NULL)
          {
            /* Node not already present add the new node now */
            an = avl_insert_node (NHLFE_PBB_TABLE, nhlfe);
            if (an == NULL)
              return NULL;

            /* "an" shuld be Non-NULL*/
            avl_lock_node (an);
          }
        break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        an = avl_search (NHLFE_TDM_TABLE, nhlfe);
        if (an == NULL)
          {
            /* Node not already present add the new node now */
            an = avl_insert_node (NHLFE_TDM_TABLE, nhlfe);
            if (an == NULL)
              return NULL;

            /* "an" shuld be Non-NULL*/
            avl_lock_node (an);
          }
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        return NULL;
    }

  return nhlfe;
}

int 
gmpls_nhlfe_add (struct nsm_master *nm, struct nhlfe_entry *nhlfe)
{
  struct nhlfe_entry *tmp_nh;
  u_int32_t nhlfe_ix;
  s_int32_t ret;

  NSM_ASSERT (nhlfe != NULL);
  if (! nhlfe)
    return NSM_ERR_INVALID_ARGS;

  tmp_nh = gmpls_nhlfe_get_entry (nm, nhlfe);

  if (! tmp_nh)
    return NSM_ERR_MEM_ALLOC_FAILURE;

  /* get a new nhlfe index */
  if (! tmp_nh->nhlfe_ix)
    {
      ret = bitmap_request_index (NHLFE_IX_MGR, &nhlfe_ix);
      if (ret < 0)
        {
          /* Clear and remove the NHLFE entry */
          return NSM_ERR_NO_TABLE_INDEX;
        }
  
      nhlfe->nhlfe_ix = nhlfe_ix;
    }

  return NSM_SUCCESS;
}

static struct avl_node *
gmpls_nhlfe_node_lookup (struct nsm_master *nm,
                         struct addr_in *nh_addr, u_int32_t oif_ix,
                         struct gmpls_gen_label *label)
{
  struct avl_node *an;
  struct nhlfe_entry_tmp entry;
  u_int32_t key_len;

  if ((oif_ix == 0)
#ifdef HAVE_PACKET
      || ((label->type == gmpls_entry_type_ip) && (label->u.pkt == 0))
#endif /* HAVE_PACKET */
     )
    return NULL;

  NSM_ASSERT (oif_ix > 0);

#ifdef HAVE_PACKET
  if ((label->type == gmpls_entry_type_ip) && 
       (label->u.pkt == LABEL_VALUE_INVALID))
    return NULL;
#endif /* HAVE_PACKET */

  pal_mem_set (&entry, 0, sizeof (struct nhlfe_entry_tmp));
  gmpls_make_nhlfe_key (nh_addr, oif_ix, label, &entry.key, &key_len);

  switch (label->type)
    {
      case gmpls_entry_type_ip:

#ifdef HAVE_IPV6
        if (nh_addr->afi == AFI_IP6)
          an = avl_search (NHLFE_TABLE6, (u_char *)&entry);
        else
#endif
          an = avl_search (NHLFE_TABLE4, (u_char *)&entry);

         if (! an)
           return NULL;
         break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
       case gmpls_entry_type_pbb_te:
         an = avl_search (NHLFE_PBB_TABLE, (u_char *)&entry);
         break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
       case gmpls_entry_type_tdm:
         an = avl_search (NHLFE_TDM_TABLE, (u_char *)&entry);
         break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
        default:
          an = NULL;
          break;
    }

  return an;
}

s_int32_t
gmpls_nhlfe_entry_remove (struct nsm_master *nm, struct nhlfe_entry *nhlfe,
                          bool_t preserve_index)
{
  struct nhlfe_key *key;
  struct avl_node *an;

  /* Do not need locks for this as the nhlfe entry already has counters */
  switch (nhlfe->type)
    {
      case gmpls_entry_type_ip:
        key = (struct nhlfe_key *) nhlfe->nkey;

#ifdef HAVE_IPV6
        if (key->afi == AFI_IP6)
          {
            an = avl_search (NHLFE_TABLE6, (u_char *)nhlfe);
            if (an == NULL)
              break;

            if (avl_is_last_node (an))
              {
                avl_delete_node (NHLFE_TABLE6, an);
              }
            else
              {
                avl_unlock_node (an);
                an->info = NULL;
              }
          }
        else
#endif
          {
            an = avl_search (NHLFE_TABLE4, (u_char *)nhlfe);
            if (an == NULL)
              break;
            if (avl_is_last_node (an))
              {
                avl_delete_node (NHLFE_TABLE4, an);
              }
            else
              {
                avl_unlock_node (an);
                an->info = NULL;
              }
          }
        break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        an = avl_search (NHLFE_PBB_TABLE, (u_char *)nhlfe);
        if (an == NULL)
          break;
        if (avl_is_last_node (an))
          {
            avl_delete_node (NHLFE_PBB_TABLE, an);
          }
        else
          {
            avl_unlock_node (an);
            an->info = NULL;
          }
        break; 
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        an = avl_search (NHLFE_TDM_TABLE, (u_char *)nhlfe);
        if (an == NULL)
          break;

        if (avl_is_last_node (an))
          {
            avl_delete_node (NHLFE_TDM_TABLE, an);
          }
        else
          {
            avl_unlock_node (an);
            an->info = NULL;
          }
        break; 
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        break;
   }

  if (nhlfe->nhlfe_ix > 0 && ! preserve_index)
    bitmap_release_index (NHLFE_IX_MGR, nhlfe->nhlfe_ix);

  return NSM_SUCCESS;
}

struct nhlfe_entry *
nsm_gmpls_nhlfe_lookup (struct nsm_master *nm,
                        struct addr_in *nh_addr, u_int32_t oif_ix,
                        struct gmpls_gen_label *out_label, bool_t rem_flag)
{
  struct avl_node *an;
  struct nhlfe_entry *nhlfe;
  struct nhlfe_key *key;

  an = gmpls_nhlfe_node_lookup (nm, nh_addr, oif_ix, out_label);
  if (! an)
    return NULL;

  nhlfe = an->info;

  if (rem_flag)
    {
      if (nhlfe->nhlfe_ix > 0)
        bitmap_release_index (NHLFE_IX_MGR, nhlfe->nhlfe_ix);

      an->info = NULL;

      switch (nhlfe->type)
        {
          case gmpls_entry_type_ip:
            key = (struct nhlfe_key *) nhlfe->nkey;

#ifdef HAVE_IPV6
            if (key->afi == AFI_IP6)
              {
                if (avl_is_last_node (an))
                  {
                    avl_delete_node (NHLFE_TABLE6, an);
                  }
                else
                  {
                    avl_unlock_node (an);
                    an->info = NULL;
                  }
              }
            else
#endif
              {
                if (avl_is_last_node (an))
                  {
                    avl_delete_node (NHLFE_TABLE4, an);
                  }
                else
                  {
                    avl_unlock_node (an);
                    an->info = NULL;
                  }
              }
            break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
          case gmpls_entry_type_pbb_te:
            if (avl_is_last_node (an))
              {
                avl_delete_node (NHLFE_PBB_TABLE, an);
              }
            else
              {
                avl_unlock_node (an);
                an->info = NULL;
              }
        break; 
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
          case gmpls_entry_type_tdm:
            if (avl_is_last_node (an))
              {
                avl_delete_node (NHLFE_TDM_TABLE, an);
              }
            else
              {
                avl_unlock_node (an);
                an->info = NULL;
              }
            break; 
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

          default:
            break;
        }
    }
  
  return nhlfe;
}

void
gmpls_xc_row_cleanup (struct nsm_master *nm, struct xc_entry *xc,
                     bool_t del_flag, bool_t preserve_index)
{
  struct nhlfe_entry *nhlfe;
  struct avl_node *an;
  bool_t rel_xc_ix = NSM_FALSE;
  
  if ((xc) && (xc->refcount == 0))
    {
      /* unlink xc and nhlfe */
      an = avl_search (XC_TABLE, xc);
      if (an == NULL)
        return;
    
      nhlfe = gmpls_xc_nhlfe_unlink (xc);
      if ((nhlfe) && (nhlfe->refcount == 0) && del_flag)
        {
          rel_xc_ix = NSM_TRUE;

          /* remove nhlfe entry */
          gmpls_nhlfe_entry_remove (nm, nhlfe, preserve_index);
          if (del_flag)
            {
              gmpls_nhlfe_free (nhlfe);
              nhlfe = NULL;
            }
        }

      if ((rel_xc_ix) && (XC_IX (xc) > 0) && ! preserve_index)
        bitmap_release_index (XC_IX_MGR, XC_IX (xc));

      /* remove xc entry */
      if (avl_is_last_node (an))
        avl_delete_node (XC_TABLE, an);
      else
        {
          an->info = NULL;
          avl_unlock_node (an);
        }
    }
}

void
gmpls_ftn_row_cleanup (struct nsm_master *nm,
                      struct ftn_entry *ftn, struct fec_gen_entry *fec,
                      u_int32_t vrf_id, bool_t del_flag)
{
  struct xc_entry *xc;
#ifdef HAVE_GMPLS
  struct ilm_entry *ilm; 
#endif /* HAVE_GMPLS */

  NSM_ASSERT (ftn != NULL);
  if (! ftn) 
    return;

  gmpls_ftn_del_from_fwd (nm, ftn, fec, vrf_id, NULL);

#ifdef HAVE_GMPLS
  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_BIDIR))
    {
      ilm = ftn->ilm_rev;
      if (ilm != NULL)
        {
          /* Unbind the entries */
          ilm->rev_entry = NULL;
          ftn->ilm_rev = NULL;
          UNSET_FLAG (ilm->flags, ILM_ENTRY_FLAG_SELECTED);
          gmpls_ilm_unselected_entry_process (nm, ilm);
        }
    }
#endif /* HAVE_GMPLS */

  xc = gmpls_ftn_xc_unlink (ftn);
  if ((xc) && (xc->refcount == 0) && del_flag)
    {
      gmpls_xc_row_cleanup (nm, xc, del_flag, NSM_FALSE);
      if (del_flag)
        {
          gmpls_xc_free (xc);
          xc = NULL;
        }
    }

}

void
gmpls_ilm_row_cleanup (struct nsm_master *nm, struct ilm_entry *ilm, 
                      bool_t del_flag)
{
  struct xc_entry *xc;
  struct prefix nh_p;
  struct fec_gen_entry fec;
  s_int32_t preserve_index = NSM_FALSE;
#ifdef HAVE_GMPLS
  struct ftn_entry *rev_ftn;
  struct ilm_entry *rev_ilm;
#endif /* HAVE_GMPLS */

  NSM_ASSERT (ilm != NULL);
  if (! ilm) 
    return;

  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_INSTALLED))
    {
      if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_DEPENDENT))
        {
          if (gmpls_set_dep_ilm_nhprefix (&nh_p, ilm))
            {
              fec.type = gmpls_entry_type_ip;
              fec.u.prefix = nh_p;
              gmpls_lsp_dep_del (nm, &fec, CONFIRM_RSVP_BYPASS_ILM, ilm);
            }
        }
      else
        gmpls_ilm_del_entry_from_fwd_tbl (nm, ilm);

    }

#ifdef HAVE_RESTART
  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_OUT_LBL_NOT_REFRESHED))
    preserve_index = NSM_TRUE;
#endif /* HAVE_RESTART */

#ifdef HAVE_GMPLS
  /* Unlink bidirectional entries */
  if ((CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_BIDIR)) &&
      (ilm->rev_entry != NULL))
    {
      if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_REV_FTN))
        {
          rev_ftn = (struct ftn_entry *) ilm->rev_entry;
          rev_ftn->ilm_rev = NULL;
          gmpls_ftn_selected_entry_process (nm, rev_ftn);
          UNSET_FLAG (rev_ftn->flags, FTN_ENTRY_FLAG_SELECTED);
        }
      else
        {
          rev_ilm = (struct ilm_entry *) ilm->rev_entry;
          rev_ilm->rev_entry = NULL;
          gmpls_ilm_unselected_entry_process (nm, rev_ilm);
          UNSET_FLAG (rev_ilm->flags, ILM_ENTRY_FLAG_SELECTED);
        }

      ilm->rev_entry = NULL;
    }

#endif /* HAVE_GMPLS */

  xc = gmpls_ilm_xc_unlink (ilm);
  if ((xc) && (xc->refcount == 0) && del_flag)
    {
      gmpls_xc_row_cleanup (nm, xc, del_flag, preserve_index);
      if (del_flag)
        {
          gmpls_xc_free (xc);
          xc = NULL;
        }
    }
}


#ifdef HAVE_SNMP 
struct nhlfe_temp_ptr *
nsm_gmpls_nhlfe_temp_lookup_next (u_int32_t ix)
{
  struct nhlfe_temp_ptr *nhlfe_temp_ptr = NULL;
  struct nhlfe_temp_ptr *ret_ptr = NULL;
  struct listnode *node = NULL;
  u_int32_t ix_next = 0;

  LIST_LOOP (nhlfe_entry_temp_list, nhlfe_temp_ptr, node)
    {
      if (nhlfe_temp_ptr->nhlfe_ix > ix)
        {
          if (! ix_next || (nhlfe_temp_ptr->nhlfe_ix < ix_next))
            {
              ix_next = nhlfe_temp_ptr->nhlfe_ix;
              ret_ptr = nhlfe_temp_ptr;
            }
        }

    }

  return ret_ptr;
}

/* Lookup Temporary NHLFE List*/
struct nhlfe_temp_ptr *
nsm_gmpls_nhlfe_temp_lookup (u_int32_t ix, u_int32_t outseg_if, 
                             struct gmpls_gen_label *out_label, 
                             struct prefix *addr, 
                             u_int32_t lookup_type)
{
  /* If lookup_type is not IDX, Interface-Label-NHAddr lookup is done */
  struct nhlfe_temp_ptr *nhlfe_temp = NULL;
  struct listnode *node = NULL;

  if (lookup_type == IDX_LOOKUP)  /* Lookup to be done based on Index */
    {
      LIST_LOOP (nhlfe_entry_temp_list, nhlfe_temp, node)
        if (nhlfe_temp->nhlfe_ix == ix)
          return nhlfe_temp;
    }
  else /* Lookup to be done based on Interface-Label-NHAddr */
    {
      LIST_LOOP (nhlfe_entry_temp_list, nhlfe_temp, node)
        if (nhlfe_temp->oif_ix == outseg_if && 
            gmpls_are_label_equal (&nhlfe_temp->out_label, out_label) &&
            ! (prefix_cmp (&nhlfe_temp->nh_addr, addr)) && /* If equal */
            nhlfe_temp->nhlfe_ix != ix)
          return nhlfe_temp;
    }

  return NULL;
}
#endif /* HAVE_SNMP */

/* Lookup NHLFE entry using the NHLFE index. */
struct nhlfe_entry *
nsm_mpls_nhlfe_ix_lookup (struct nsm_master *nm, u_int32_t ix)
{
  struct nhlfe_entry *nhlfe;
  struct avl_node *an;
  struct avl_tree *at, *nxt_at;
  bool_t done = NSM_FALSE;

  if (ix == 0)
    return NULL;

  GET_FIRST_NHLFE_TABLE (at);

  while (!done)
    {
      for (an = avl_top (at); an; an = avl_next (an))
        {
          if ((nhlfe = an->info) != NULL)
            {
              if (nhlfe->nhlfe_ix == ix)
                {
                  return nhlfe;
                }
            }
        }

      GET_NEXT_NHLFE_TABLE (at, nxt_at);
      if (at == NULL)
        {
          done = NSM_TRUE;
        }
      else
        {
          at = nxt_at;
        }
    }

  return NULL;
}


/* Lookup "next" NHLFE entry using the NHLFE index. */
struct nhlfe_entry *
nsm_mpls_nhlfe_ix_lookup_next (struct nsm_master *nm, u_int32_t ix)
{
  struct nhlfe_entry *nhlfe, *nhlfe_next;
  struct avl_node *an;
  u_int32_t ix_next;
  struct avl_tree *at, *nxt_at;
  bool_t done = NSM_FALSE;

  GET_FIRST_NHLFE_TABLE (at);
  if ((! at) || (ix == 0))
    return NULL;

  ix_next = 0;
  nhlfe_next = NULL;

  while (!done)
    {
      for (an = avl_top (at); an; an = avl_next (an))
        {
          if ((nhlfe = an->info) != NULL)
            {
              if (nhlfe->nhlfe_ix > ix)
                {
                  /* If found nhlfe entry's index is 1 more than passed-in
                     index, next nhlfe entry has been found. */
                  if (ix + 1 == nhlfe->nhlfe_ix)
                    {
                      return nhlfe;
                    }
              
                  if ((! ix_next) || (nhlfe->nhlfe_ix < ix_next))
                    {
                      ix_next = nhlfe->nhlfe_ix;
                      nhlfe_next = nhlfe;
                    }
                }
            }
        }

      GET_NEXT_NHLFE_TABLE (at, nxt_at);
      if (at == NULL)
        {
          done = NSM_TRUE;
        }
      else
        {
          at = nxt_at;
        }
    }

  return nhlfe_next;
}


/* Lookup "next" Cross Connect entry. To be used by SNMP queries. */
struct xc_entry *
nsm_gmpls_xc_lookup_next (struct nsm_master *nm,
                          u_int32_t xc_ix, u_int32_t iif_ix,
                          struct gmpls_gen_label *in_label, u_int32_t nhlfe_ix,
                          u_char l_len)
{
  struct xc_entry *xc = NULL, tmp_xc;
  struct avl_node *head, *next, *an;
  bool_t new = PAL_FALSE;
  
  if (! XC_TABLE)
    return NULL;
  
  pal_mem_set (&tmp_xc, '\0', sizeof (struct xc_entry));

  /* Fill key and lookup. */
  gmpls_make_xc_key (xc_ix, iif_ix, in_label, nhlfe_ix, &tmp_xc.key);

  head = avl_search (XC_TABLE, &tmp_xc);
  if (! head)
    {
      head = avl_insert_node (XC_TABLE, &tmp_xc);
      if (!head)
        return NULL;
      new = PAL_TRUE;
    }

  next = avl_next (head);

  for (an = next; an; an = avl_next (an))
    {
      if ((xc = an->info) != NULL)
        {
          break;
        }
    }
  if (new == PAL_TRUE)
    avl_delete_node (XC_TABLE, head);

  return xc;
}

#ifdef HAVE_SNMP
/* Lookup Temporary FTN List*/
struct ftn_temp_ptr *
nsm_gmpls_ftn_temp_lookup (u_int32_t ix, struct prefix *dst_addr, 
                            u_int32_t lookup_type)
{
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct listnode *node = NULL;

  if (lookup_type == IDX_LOOKUP)  /* Lookup to be done based on Index */
    {
      LIST_LOOP (ftn_entry_temp_list, ftn_temp, node)
        if (ftn_temp->ftn_ix == ix)
          return ftn_temp;
    }
  else  /* Lookup to be done based on Dest Addr */
    {
      LIST_LOOP (ftn_entry_temp_list, ftn_temp, node)
        if (!(prefix_cmp (&ftn_temp->fec.u.prefix, dst_addr)) &&
            ftn_temp->ftn_ix != ix)
          return ftn_temp;
    }

  return NULL;
}

struct ftn_temp_ptr *
nsm_gmpls_ftn_temp_lookup_next (u_int32_t ftn_ix)
{
  struct ftn_temp_ptr *ftn_temp = NULL;
  struct ftn_temp_ptr *ftn_ret = NULL;
  struct listnode *node = NULL;
  u_int32_t ix_next = 0;

    LIST_LOOP (ftn_entry_temp_list, ftn_temp, node)
      {
         if (ftn_temp->ftn_ix > ftn_ix)
           {
             if (! ix_next || (ftn_temp->ftn_ix < ix_next))
               {
                 ix_next = ftn_temp->ftn_ix;
                 ftn_ret = ftn_temp;
               }
           }
      }

  return ftn_ret;
}

/* Lookup Temporary FTN List for given XC Index */
s_int32_t
nsm_mpls_ftn_xc_ix_lookup_temp_list (u_int32_t xc_ix)
{
  struct ftn_temp_ptr *ftn_temp_ptr = NULL;
  struct listnode *node = NULL;

  LIST_LOOP (ftn_entry_temp_list, ftn_temp_ptr, node)
    {
      if (ftn_temp_ptr->xc->key.xc_ix == xc_ix)
        return NSM_FAILURE;
    }

  return NSM_SUCCESS;
}
#endif /* HAVE_SNMP */

struct ftn_entry *
nsm_mpls_xc_ix_ftn_lookup (struct nsm_master *nm, u_int32_t xc_ix)
{
  struct ptree_node *pn;
  struct ftn_entry *ftn, *list;
  struct ptree *pt, *nxt_pt;
  bool_t done = NSM_FALSE;

  GET_FIRST_FTN_TABLE (pt);
  if ((pt == NULL) || (xc_ix == 0))
    return NULL;

  while (!done)
    {
      for (pn = ptree_top (pt); pn; pn = ptree_next (pn))
        {
          if ((list = pn->info) != NULL)
            {
              for (ftn = list; ftn; ftn = ftn->next)
                {
                  if (ftn->xc && ftn->xc->key.xc_ix == xc_ix)
                    return ftn;
                }
            }
        }

      GET_NEXT_FTN_TABLE (pt, nxt_pt);
      if (nxt_pt == NULL)
        {
          done = NSM_TRUE;
        }
      else
        {
          pt = nxt_pt;
        }
    }

  return NULL;
}

/* Lookup an FTN entry by FTN Index. */
struct ftn_entry *
nsm_gmpls_ftn_ix_lookup (struct nsm_master *nm,
                        u_int32_t ftn_ix, struct fec_gen_entry *fec)
{
  struct ptree_node *pn;
  struct ftn_entry *ftn, *list;
  struct ptree *pt, *nxt_pt;
  bool_t done = NSM_FALSE;

  GET_FIRST_FTN_TABLE (pt);
  if ((pt == NULL) || (ftn_ix == 0))
    return NULL;

  while (!done)
    {
      for (pn = ptree_top (pt); pn; pn = ptree_next (pn))
        {
          if ((list = pn->info) != NULL)
            {
              for (ftn = list; ftn; ftn = ftn->next)
                {
                  if (ftn->ftn_ix == ftn_ix)
                    {
                      nsm_gmpls_set_fec_from_ftn (ftn, fec);
                      ptree_unlock_node (pn);
                      return ftn;
                    }
                }
            }
        }

      GET_NEXT_FTN_TABLE (pt, nxt_pt);
      if (nxt_pt == NULL)
        {
          done = NSM_TRUE;
        }
      else
        {
          pt = nxt_pt;
        }
    }

  return NULL;
}


/* Lookup "next" FTN entry by FTN Index. */
struct ftn_entry *
nsm_gmpls_ftn_ix_lookup_next (struct nsm_master *nm,
                              u_int32_t ftn_ix, struct fec_gen_entry *fec)
{
  struct ptree_node *pn;
  struct ftn_entry *list, *ftn, *ftn_next;
  u_int32_t ix_next;
  struct ptree *pt, *nxt_pt;
  bool_t done = NSM_FALSE;

  GET_FIRST_FTN_TABLE (pt);
  if (pt == NULL)
    return NULL;

  ix_next = 0;
  ftn_next = NULL;
  while (!done)
    {
      for (pn = ptree_top (pt); pn; pn = ptree_next (pn))
        {
          if ((list = pn->info) != NULL)
           {
              for (ftn = list; ftn; ftn = ftn->next)
                {
                  if (ftn && ftn->ftn_ix > ftn_ix)
                    {
                      /* If found ftn entry's index is 1 more than passed-in
                         index, next ftn entry has been found. */
                      if (ftn_ix + 1 == ftn->ftn_ix)
                        {
                          nsm_gmpls_set_fec_from_ftn (ftn, fec);
                          ptree_unlock_node (pn);
                          return ftn;
                        }
                  
                      if ((! ix_next) || (ftn->ftn_ix < ix_next))
                        {
                          nsm_gmpls_set_fec_from_ftn (ftn, fec);
                          ix_next = ftn->ftn_ix;
                          ftn_next = ftn;
                        }
                    }
                }
            }
        }

      GET_NEXT_FTN_TABLE (pt, nxt_pt);
      if (nxt_pt == NULL)
        {
          done = NSM_TRUE;
        }
      else
        {
          pt = nxt_pt;
        }
    }

  return ftn_next;
}

#ifdef HAVE_PACKET
struct ftn_entry *
nsm_mpls_ftn_ix_lookup_next (struct nsm_master *nm,
                             u_int32_t ftn_ix, struct prefix *pfx)
{
  struct fec_gen_entry fec;

  fec.type = gmpls_entry_type_ip;
  fec.u.prefix = *pfx;
  return nsm_gmpls_ftn_ix_lookup_next (nm, ftn_ix, &fec);
}
#endif /* HAVE_PACKET */

#ifdef HAVE_SNMP
struct ilm_temp_ptr *
nsm_gmpls_ilm_temp_lookup_next (u_int32_t ix)
{
  struct ilm_temp_ptr *ilm_temp = NULL;
  struct ilm_temp_ptr *ilm_ret = NULL;
  struct listnode *node = NULL;
  u_int32_t ix_next = 0;

  LIST_LOOP (ilm_entry_temp_list, ilm_temp, node)
    {
      if (ilm_temp->ilm_ix > ix)
        {
          if (! ix_next || (ilm_temp->ilm_ix < ix_next))
            {
              ix_next = ilm_temp->ilm_ix;
              ilm_ret = ilm_temp;
            }
        }
    }

  return ilm_ret;
}

struct ilm_temp_ptr *
nsm_gmpls_ilm_temp_lookup (u_int32_t ix, u_int32_t inseg_if,
                           struct gmpls_gen_label *in_label,
                           u_int32_t lookup_type)
{
  /* If lookup_type is not IDX, Interface-Label lookup is done */
  struct ilm_temp_ptr *ilm_temp = NULL;
  struct listnode *node = NULL;

  if (lookup_type == IDX_LOOKUP)  /* Lookup to be done based on Index */
    {
      LIST_LOOP (ilm_entry_temp_list, ilm_temp, node)
        if (ilm_temp->ilm_ix == ix)
          return ilm_temp;
    }
  else  /* Lookup to be done based on Interface-Label pair */
    {
      LIST_LOOP (ilm_entry_temp_list, ilm_temp, node)
        if (ilm_temp->iif_ix == inseg_if &&
            gmpls_are_label_equal (&ilm_temp->in_label, in_label) &&
            ilm_temp->ilm_ix != ix)  /* Different entry */
          return ilm_temp;
    }

  return NULL;
}

/* Lookup Temporary ILM List */
struct ilm_temp_ptr *
nsm_mpls_ilm_temp_lookup (u_int32_t ix, u_int32_t inseg_if, 
                           u_int32_t in_label, u_int32_t lookup_type)
{
  struct gmpls_gen_label lbl;

  lbl.type = gmpls_entry_type_ip;
  lbl.u.pkt = in_label;

  return nsm_gmpls_ilm_temp_lookup (ix, inseg_if, &lbl, lookup_type);
}
#endif /* HAVE_SNMP */

/* Lookup ILM entry using the ILM index. */
struct ilm_entry *
nsm_gmpls_ilm_ix_lookup (struct nsm_master *nm, u_int32_t ix)
{
  struct ilm_entry *ilm;
  struct avl_node *an;
  struct avl_tree *at, *nxt_at;
  bool_t done = PAL_FALSE;

  GET_FIRST_ILM_TABLE (at);

  if ((! at) || (ix == 0))
    return NULL;

  while (!done)
    {
      for (an = avl_top (at); an; an = avl_next (an))
        {
          if (an->info != NULL)
            {
              for (ilm = (struct ilm_entry *)an->info; ilm; ilm = ilm->next)
                {
                  if (ilm->ilm_ix == ix)
                    {
                      return ilm;
                    }
                }
            }
        }

      GET_NEXT_ILM_TABLE (at, nxt_at);
      if (nxt_at == NULL)
        {
          done = NSM_TRUE;
        }
      else
        {
          at = nxt_at;
        }
    }

  return NULL;
}

/* Lookup "next" ILM entry using the NHLFE index. */
struct ilm_entry *
nsm_gmpls_ilm_ix_lookup_next (struct nsm_master *nm, u_int32_t ix)
{
  struct ilm_entry *ilm, *ilm_next;
  struct avl_node *an;
  struct avl_tree *at, *nxt_at;
  u_int32_t ix_next;
  bool_t done = PAL_FALSE;
  
  GET_FIRST_ILM_TABLE (at);

  if (! at)
    return NULL;

  ix_next = 0;
  ilm_next = NULL;

  while (!done)
    {
      for (an = avl_top (at); an; an = avl_next (an))
        {
          if ((an->info) != NULL)
            {
              for (ilm = (struct ilm_entry *)an->info; ilm; ilm = ilm->next)
                {
                  if (ilm->ilm_ix > ix)
                    {
                      /* If found ilm entry's index is 1 more than passed-in
                         index, next ilm entry has been found. */
                      if (ix + 1 == ilm->ilm_ix)
                        {
                          return ilm;
                        }
              
                      if ((! ix_next) || (ilm->ilm_ix < ix_next))
                        {
                          ix_next = ilm->ilm_ix;
                          ilm_next = ilm;
                        }
                    }
                }
            }
        }

      GET_NEXT_ILM_TABLE (at, nxt_at);
      if (nxt_at == NULL)
        {
          done = NSM_TRUE;
        }
      else
        {
          at = nxt_at;
        }

    }

  return ilm_next;
}

static s_int32_t
gmpls_ftn_row_create (struct nsm_master *nm, struct ftn_entry *ftn, 
                      struct ftn_add_gen_data *data)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct xc_entry *xc = NULL;
  struct gmpls_gen_label in_label;
#ifdef HAVE_GMPLS
  struct ilm_entry *rev_ilm = NULL;
#endif /* HAVE_GMPLS */
  bool_t nhlfe_created = NSM_FALSE;
  bool_t xc_created = NSM_FALSE;
  s_int32_t ret;

  if (! data)
    {
      ret = NSM_ERR_INVALID_ARGS;
      /* Data is NULL*/
      goto cleanup;
    }
  
  /* lookup nhlfe entry */
  nhlfe = nsm_gmpls_nhlfe_lookup (nm, &data->nh_addr, data->oif_ix,
                                  &data->out_label, NSM_FALSE);
  if (! nhlfe)
    {
      struct nhlfe_new_data nhd;

      /* set up data for creating nhlfe */
      pal_mem_set (&nhd, 0, sizeof (struct nhlfe_new_data));
      nhd.owner = data->owner.owner;
      nhd.opcode = data->opcode;
      nhd.oif_ix = data->oif_ix;
      nhd.out_label = data->out_label;
      pal_mem_cpy (&nhd.nh_addr, &data->nh_addr, sizeof (struct addr_in));
      if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_GMPLS))      
        SET_FLAG (nhd.flags, NHD_DATA_FLAG_GMPLS);

      /* create a new nhlfe entry */
      nhlfe = gmpls_nhlfe_new (&nhd, &ret);
      if (! nhlfe)
        goto cleanup;
        
      nhlfe_created = NSM_TRUE;

      /* add nhlfe to table */
      ret = gmpls_nhlfe_add (nm, nhlfe);
      if (ret != NSM_SUCCESS)
        goto cleanup;
    }
  else
    {
      /* pre-existing nhlfe entry */
      nhlfe_created = NSM_FALSE;
      
      NSM_ASSERT (nhlfe->nhlfe_ix > 0);
      NSM_ASSERT (nhlfe->xc_ix > 0);
      if ((nhlfe->nhlfe_ix == 0) || (nhlfe->xc_ix == 0))
        {
          ret = NSM_ERR_INVALID_NHLFE;
          goto cleanup;
        }
      
      /* get xc entry */
      pal_mem_set (&in_label, 0, sizeof (struct gmpls_gen_label));
      in_label.type = gmpls_entry_type_ip;
      xc = nsm_gmpls_xc_lookup (nm, nhlfe->xc_ix, 0, &in_label,
                               nhlfe->nhlfe_ix, NSM_FALSE);
    }


  if (! xc)
    {
      struct xc_new_data xcd;

      /* set up data for creating xc */
      pal_mem_set (&xcd, 0, sizeof (struct xc_new_data));
      xcd.owner = data->owner.owner;
      xcd.iif_ix = 0;

      /* Setting the type of the cross-connect entry
       * from the out_label type. 
       */
      xcd.in_label.type = data->out_label.type;

      pal_mem_set (&xcd.lspid, 0, sizeof (struct mpls_lspid));
      if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_GMPLS))
        SET_FLAG (xcd.flags, XC_DATA_FLAG_GMPLS);

      if (data->owner.owner == MPLS_RSVP || 
          data->owner.owner == MPLS_IGP_SHORTCUT)
        pal_mem_cpy (&xcd.lspid, &data->lspid, sizeof (struct mpls_lspid));
      
      /* create new xc entry */
      xc = gmpls_xc_new (&xcd, &ret);
      if (! xc)
        {
          if (nhlfe_created)
            {
              /* remove nhlfe entry from table */
              gmpls_nhlfe_entry_remove (nm, nhlfe, NSM_FALSE); 
            }
          goto cleanup;
        }

      xc_created = NSM_TRUE;

      /* link xc to nhlfe */
      gmpls_xc_nhlfe_link (xc, nhlfe);
      
      /* add xc to table */
      ret = gmpls_xc_add (nm, xc);
      if (ret != NSM_SUCCESS)
        {
          gmpls_xc_nhlfe_unlink (xc);
          xc->key.nhlfe_ix = 0;

          if (nhlfe_created)
            {
              /* remove nhlfe entry from table */
              gmpls_nhlfe_entry_remove (nm, nhlfe, NSM_FALSE); 
            }
          goto cleanup;
        }
    }


  /* link ftn to xc */
  gmpls_ftn_xc_link (ftn, xc);
  
#ifdef HAVE_GMPLS
  /* Check if bidirectional LSP and xc_ix set */
  if (CHECK_FLAG (data->flags, NSM_MSG_GEN_FTN_BIDIR))
    {
      if (data->rev_ilm_ix != 0)
        {
          /* reverse entry find */
          rev_ilm = nsm_gmpls_ilm_ix_lookup (nm, data->rev_ilm_ix);
          
          if (rev_ilm == NULL)
            {
              ret = NSM_ERR_INVALID_ARGS;
              gmpls_ftn_xc_unlink (ftn);
              if (xc_created)
                {
                  gmpls_xc_nhlfe_unlink (xc);
                  avl_remove (XC_TABLE, xc);
                }
              if (nhlfe_created)
                gmpls_nhlfe_entry_remove (nm, nhlfe, NSM_FALSE); 
              goto cleanup;
            }

          ftn->ilm_rev = rev_ilm;
          rev_ilm->rev_entry = ftn;
          SET_FLAG (rev_ilm->flags, ILM_ENTRY_FLAG_REV_FTN);
        }
    }
#endif /* HAVE_GMPLS */
  return NSM_SUCCESS;
  
 cleanup:

  if (nhlfe && nhlfe_created)
    gmpls_nhlfe_free (nhlfe);
  
  if (xc && xc_created)
    gmpls_xc_free (xc);
  
  return ret;
}

void
gmpls_ftn_entry_cleanup (struct nsm_master *nm,
                         struct ftn_entry *ftn, struct fec_gen_entry *fec,
                         struct ptree_ix_table *tbl)
{
  gmpls_ftn_del_from_fwd (nm, ftn, fec, tbl->m_table->id, NULL);

  if (ftn->ftn_ix > 0 && tbl->ix_mgr)
    {
      bitmap_release_index (tbl->ix_mgr, ftn->ftn_ix);
      ftn->ftn_ix = 0;
    }

  gmpls_ftn_row_cleanup (nm, ftn, fec, tbl->m_table->id, NSM_TRUE);
  return;
}


/* Process an new FTN addition. */
static s_int32_t
gmpls_ftn_add_process (struct nsm_master *nm,
                       struct ptree_ix_table *pix_tbl, 
                       struct ftn_add_gen_data *data, 
                       struct ftn_entry **ppftn,
                       mpls_ftn_type_t f_type,
                       bool_t config)
{
  struct ftn_entry *ftn;
  struct ptree_node *rn;
  struct interface *ifp;
  struct fec_gen_entry fec;
#ifdef HAVE_SNMP
  struct nsm_mpls *nmpls = nm->nmpls;
#endif  /* HAVE_SNMP */
#ifdef HAVE_GMPLS
  struct gmpls_if *gmif;
  struct datalink *dl; 
#endif /* HAVE_GMPLS */
  s_int32_t ret;

  if ((! pix_tbl) || (! data))
    return NSM_ERR_INVALID_ARGS;
  
  *ppftn = NULL;

  /* If Client Restarted gracefully, create new entry for the same prefix */ 
  if (f_type == MPLS_FTN_BGP_VRF) 
    {
      rn = gmpls_ftn_node_lookup (pix_tbl->m_table, &data->ftn);
      /* Traverse the node to find the particular Prefix */
      if (rn)
        { 
          for (ftn = rn->info; ftn; ftn = ftn->next)
            {
              if (ftn->owner.owner == MPLS_OTHER_BGP
                  && CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE))
                break;
            }
        }   

      /* If the particular entry is present and the entry is not stale, return 
        error */ 
      if (rn && ftn && ! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE))
        return NSM_ERR_VRF_ENTRY_EXISTS;
    }

  /* create ftn entry */
  ftn = gmpls_ftn_new (data, f_type, &ret);
  if (! ftn)
    return ret;

  if (f_type == MPLS_FTN_BGP_VRF && data->opcode != DLVR_TO_IP)
    SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_DEPENDENT);

  if (f_type == MPLS_FTN_MAPPED || f_type == MPLS_FTN_RSVP_MAPPED
      || f_type == MPLS_FTN_IGP_SHORTCUT)
    SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_DEPENDENT);

  if (config == NSM_TRUE)
    SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_STATIC);

#ifdef HAVE_GMPLS
  gmif = gmpls_if_get (nm->vr->zg, nm->vr->id);

  if (!CHECK_FLAG (data->flags, NSM_MSG_GEN_FTN_MPLS))
    {
      dl = data_link_lookup_by_index (gmif, data->oif_ix);
      if ((dl == NULL) || (!IS_DATA_LINK_UP (dl) || (dl->ifp == NULL)))
        {
          SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_INACTIVE);
        }
    }
  else
    {
      ifp = if_lookup_by_gindex (gmif, data->oif_ix);
      if (ifp && ! NSM_MPLS_VALID_INTERFACE (ifp))
        SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_INACTIVE);
    }
#else /* HAVE_GMPLS */
  ifp = if_lookup_by_index (&nm->vr->ifm, data->oif_ix);
  if (! NSM_MPLS_VALID_INTERFACE (ifp))
    SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_INACTIVE);
#endif /* HAVE_GMPLS */

  ret = gmpls_ftn_row_create (nm, ftn, data);
  if (ret != NSM_SUCCESS)
    {
      gmpls_ftn_free (ftn);
      return ret;
    }

  /* add ftn entry to table */
  rn = gmpls_ftn_add (nm, pix_tbl, ftn, &data->ftn, &ret);
  if (rn == NULL)
    {
      gmpls_ftn_entry_cleanup (nm, ftn, &data->ftn, pix_tbl);
      gmpls_ftn_free (ftn);
      
      return ret;
    }

  /* save pointer to ptree_node in FTN_TABLE */
  ftn->pn = rn;
  
  /* process selected ftn entry */
  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED))
    {
      ret = gmpls_ftn_entry_select_process (nm, ftn, rn);
      if (ret != NSM_SUCCESS)
        return NSM_ERR_FTN_INSTALL_FAILURE;

      if (ftn->igp_status == FTN_ENTRY_IGP_STATUS_CANDIDATE)
        {
          if (ftn->next) 
            gmpls_ftn_add_select_update (nm, ftn, rn);

          /* above gmpls_ftn_add_select_update might change igp_status */
          if (ftn->igp_status == FTN_ENTRY_IGP_STATUS_CANDIDATE &&
              ftn->lsp_bits.bits.igp_shortcut)
            {
              nsm_gmpls_set_fec_from_ftn (ftn, &fec);
              if (fec.type == gmpls_entry_type_ip)
                {
                  nsm_mpls_send_igp_shortcut_lsp (ftn, &fec.u.prefix,
                                                  NSM_MSG_IGP_SHORTCUT_ADD);

                  ftn->igp_status = FTN_ENTRY_IGP_STATUS_ADVERTISED; 
                }
            }
          else if (ftn->igp_status != FTN_ENTRY_IGP_STATUS_ADVERTISED)
            {
              ftn->igp_status = FTN_ENTRY_IGP_STATUS_NONE;
            }
        }
    }
  else
    {
       fec.type = gmpls_entry_type_ip;

      /* For rsvp_mapped route and igp-shortcut route, even it does not get
         selected, still need add to confirm list */
       if (ftn->ftn_type == MPLS_FTN_RSVP_MAPPED)
         {
           gmpls_lsp_dep_add (nm, &fec, CONFIRM_RSVP_MAPPED_ROUTE, ftn);
           fec.u.prefix = *ftn->pri_fec_prefix;
         }
       else if (ftn->ftn_type == MPLS_FTN_IGP_SHORTCUT)
         {
           gmpls_lsp_dep_add (nm, &fec, CONFIRM_IGP_SHORTCUT_ROUTE, ftn);
           fec.u.prefix = *ftn->pri_fec_prefix;
         }
    }

  FTN_XC_STATUS_UPDATE (ftn);
#ifdef HAVE_SNMP
  if (nmpls->notificationcntl == NSM_MPLS_NOTIFICATIONCNTL_ENA &&
      CHECK_FLAG (ftn->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG)) 
    nsm_mpls_opr_sts_trap (nmpls, ftn->xc->key.xc_ix, ftn->xc->opr_status);
#endif  /* HAVE_SNMP */
  *ppftn = ftn;
  
  return ret;
}


static void
mpls_ftn_desc_string_update (struct ftn_entry *ftn, char *sz_desc)
{
  if (! ftn)
    return;

  if (! ftn->sz_desc)
    {
      if (sz_desc)
        ftn->sz_desc = XSTRDUP (MTYPE_TMP, sz_desc);
    }
  else if (! sz_desc)
    {
      XFREE (MTYPE_TMP, ftn->sz_desc);
      ftn->sz_desc = NULL;
    }
  else if ((pal_strlen (ftn->sz_desc) != pal_strlen (sz_desc)) ||
           (pal_strncmp (ftn->sz_desc, sz_desc, 
                         pal_strlen (ftn->sz_desc)) != 0))
    {
      XFREE (MTYPE_TMP, ftn->sz_desc);
      ftn->sz_desc = XSTRDUP (MTYPE_TMP, sz_desc);
    }
  
  return;
}



static s_int32_t
gmpls_ftn_update_process (struct nsm_master *nm,
                          struct ptree_ix_table *pix_tbl, 
                          struct ftn_add_gen_data *data, 
                          struct ftn_entry *ftn)
{
#ifdef HAVE_SNMP
  struct nsm_mpls *nmpls = nm->nmpls;
#endif  /* HAVE_SNMP */
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_key_gen *key;
  struct nhlfe_key *nkey;
  struct ptree_node *rn;
  struct gmpls_gen_label lbl;
  struct fec_gen_entry fec;
  s_int32_t proto_id;
  bool_t recreate_row = NSM_FALSE;
  bool_t update_primary = NSM_FALSE;
  bool_t primary_lsp = NSM_FALSE;
  s_int32_t igp_shortcut = -1;
  s_int32_t ret;

  NSM_ASSERT ((ftn != NULL) && (data != NULL) && (pix_tbl != NULL));
  if ((! pix_tbl) || (! pix_tbl->m_table) || (! data) || (! ftn))
    return NSM_ERR_INVALID_ARGS;
  
  if (ftn->owner.owner != data->owner.owner)
    return NSM_ERR_OWNER_MISMATCH;

  proto_id = gmpls_owner_to_proto (ftn->owner.owner);

  ftn->row_status = RS_NOT_READY;

  nhlfe = FTN_NHLFE (ftn);
  NSM_ASSERT (nhlfe != NULL);
  if (! nhlfe)
    return NSM_ERR_INTERNAL;

  nkey = (struct nhlfe_key *) nhlfe->nkey;
  key = (struct nhlfe_key_gen *) nhlfe->nkey;

  /* check for nhlfe data change */
  switch (nhlfe->type)
    {
      case gmpls_entry_type_ip:
        lbl.type = gmpls_entry_type_ip;

        if ((nkey->afi == AFI_IP)
#ifdef HAVE_GMPLS
            || (nkey->afi == AFI_UNNUMBERED)
#endif /* HAVE_GMPLS */
            )
          {
            lbl.u.pkt = nkey->u.ipv4.out_label;
            if ((data->nh_addr.afi != AFI_IP) || 
                (! IPV4_ADDR_SAME (&nkey->u.ipv4.nh_addr, 
                                   &data->nh_addr.u.ipv4)) ||
                (nkey->u.ipv4.oif_ix != data->oif_ix) ||
                (! gmpls_are_label_equal (&lbl, &data->out_label)))
              recreate_row = NSM_TRUE;
            else if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE))
              {
                THREAD_TIMER_OFF (ng->restart[proto_id].t_preserve); 
                UNSET_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE);
              }
          }
#ifdef HAVE_IPV6
        else 
          {
            lbl.u.pkt = nkey->u.ipv6.out_label;

            if ((data->nh_addr.afi != AFI_IP6) ||
                (! IPV6_ADDR_SAME (&nkey->u.ipv6.nh_addr, 
                                   &data->nh_addr.u.ipv6)) ||
                (nkey->u.ipv6.oif_ix != data->oif_ix) ||
                (gmpls_are_label_equal (&lbl, &data->out_label)))
              recreate_row = NSM_TRUE;
            else if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE))
              {
                THREAD_TIMER_OFF (ng->restart[proto_id].t_preserve); 
                UNSET_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE);
              }
        }
#endif /* HAVE_IPV6 */
        break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
         if(key)
           {
             pal_mem_cpy(&lbl.u.pbb,
                 &key->u.pbb.lbl, sizeof(struct pbb_te_label));

             if ((key->u.pbb.oif_ix != data->oif_ix) ||
                (gmpls_are_label_equal (&lbl, &data->out_label)))
               recreate_row = NSM_TRUE;
             else if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE))
               {
                 THREAD_TIMER_OFF (ng->restart[proto_id].t_preserve);
                 UNSET_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE);
               }
           }
        break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
       default:
         break;
    }

  /* check for change in opcode */
  if ((! recreate_row) &&
      ! ((nhlfe->opcode == data->opcode) ||
        (nhlfe->opcode == SWAP && data->opcode == PUSH)))
    recreate_row = NSM_TRUE;
  
  /* update ftn action type */
  if (data->lsp_bits.bits.act_type != ftn->lsp_bits.bits.act_type)
    {
      ftn->lsp_bits.bits.act_type = data->lsp_bits.bits.act_type;
      recreate_row = NSM_TRUE;
    }
  
#ifdef HAVE_DIFFSERV
  /* update ds info */
  if (pal_mem_cmp (&ftn->ds_info, &data->ds_info, 
                   sizeof (struct ds_info)) != 0)
    {
      pal_mem_cpy (&ftn->ds_info, &data->ds_info, sizeof (struct ds_info));
      recreate_row = NSM_TRUE;
    }
#endif /* HAVE_DIFFSERV */

  /* update ftn owner */
  if (mpls_owner_same (&data->owner, &ftn->owner) == NSM_FALSE)
    pal_mem_cpy (&ftn->owner, &data->owner, sizeof (struct mpls_owner));

  /* update ftn description string */
  mpls_ftn_desc_string_update (ftn, data->sz_desc);

  /* update ftn exp bits */
  if (data->exp_bits != ftn->exp_bits)
    {
      recreate_row = NSM_TRUE;
      ftn->exp_bits = data->exp_bits;
    }

  /* Update incoming DSCP. */
  if (data->dscp_in != ftn->dscp_in)
    {
      recreate_row = NSM_TRUE;
      ftn->dscp_in = data->dscp_in;
    }

  /* Update tunnel id. */
  if (data->tunnel_id != ftn->tun_id)
    {
      recreate_row = NSM_TRUE;
      ftn->tun_id = data->tunnel_id;
    }

  /* update tunnel lsp id */
  if (data->protected_lsp_id != ftn->protected_lsp_id)
    {
      recreate_row = NSM_TRUE;
      ftn->protected_lsp_id = data->protected_lsp_id;
    }

  /* Update QoS resource id. */
  if (data->qos_resrc_id != ftn->qos_resrc_id)
    {
      recreate_row = NSM_TRUE;
      ftn->qos_resrc_id = data->qos_resrc_id;
    }

  /* Update Bypass FTN index. */
  if (data->bypass_ftn_ix != ftn->bypass_ftn_ix)
    {
      recreate_row = NSM_TRUE;
      ftn->bypass_ftn_ix = data->bypass_ftn_ix;
    }

  if (data->lsp_bits.bits.lsp_type == LSP_TYPE_BYPASS &&
      ftn->lsp_bits.bits.lsp_type != LSP_TYPE_BYPASS)
    {
      recreate_row = NSM_TRUE;
      ftn->lsp_bits.bits.lsp_type = data->lsp_bits.bits.lsp_type;
    }

  if (data->lsp_bits.bits.lsp_type != LSP_TYPE_BYPASS &&
      ftn->lsp_bits.bits.lsp_type == LSP_TYPE_BYPASS)
    {
      recreate_row = NSM_TRUE;
      ftn->lsp_bits.bits.lsp_type = data->lsp_bits.bits.lsp_type;
    }

  if (data->lsp_bits.bits.lsp_metric != ftn->lsp_bits.bits.lsp_metric)
    {
      ftn->lsp_bits.bits.lsp_metric = data->lsp_bits.bits.lsp_metric;

      if (ftn->igp_status == FTN_ENTRY_IGP_STATUS_ADVERTISED)
        igp_shortcut = NSM_MSG_IGP_SHORTCUT_UPDATE;
    }

  if (data->lsp_bits.bits.igp_shortcut != ftn->lsp_bits.bits.igp_shortcut)
    {
      ftn->lsp_bits.bits.igp_shortcut = data->lsp_bits.bits.igp_shortcut;

      if (ftn->lsp_bits.bits.igp_shortcut)
        igp_shortcut = NSM_MSG_IGP_SHORTCUT_ADD;
      else if (ftn->igp_status == FTN_ENTRY_IGP_STATUS_ADVERTISED)
        igp_shortcut = NSM_MSG_IGP_SHORTCUT_DELETE;
    }

  /* update xc lspid */
  if ((! recreate_row) &&
      (pal_mem_cmp (&ftn->xc->lspid, &data->lspid,
                    sizeof (struct mpls_lspid)) != 0))
    pal_mem_cpy (&ftn->xc->lspid, &data->lspid, sizeof (struct mpls_lspid));

  rn = gmpls_ftn_node_lookup (pix_tbl->m_table, &data->ftn);
  if (! rn)
    return NSM_ERR_INTERNAL;
 
  if (data->lsp_bits.bits.lsp_type == LSP_TYPE_PRIMARY || 
      data->lsp_bits.bits.lsp_type == LSP_TYPE_BYPASS)
    { 
      primary_lsp = NSM_TRUE;

      if (! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_PRIMARY)) 
        {
          SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_PRIMARY);
          if (! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED))
            update_primary = NSM_TRUE;
        }
    }
  else if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_PRIMARY))
    {
      UNSET_FLAG (ftn->flags, FTN_ENTRY_FLAG_PRIMARY);
      if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED))
        update_primary = NSM_TRUE;
    }

  if (!CHECK_FLAG (data->flags, NSM_MSG_GEN_FTN_MPLS))
    {
      SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_GMPLS);
    }

  if (update_primary)
    {
      gmpls_ftn_entry_remove_list (nm, (struct ftn_entry **)&rn->info,
                                  ftn->ftn_ix, rn, NSM_TRUE);

      ret = gmpls_ftn_entry_add_list (nm, (struct ftn_entry **)&rn->info,
                                     ftn, rn, primary_lsp);
      if (ret != NSM_SUCCESS)
        goto cleanup;

      /* igp_shortcut will be taking care of by update primary */
      if (ftn->igp_status == FTN_ENTRY_IGP_STATUS_CANDIDATE &&
          ftn->lsp_bits.bits.igp_shortcut)
        igp_shortcut = NSM_MSG_IGP_SHORTCUT_ADD;
      else
        igp_shortcut = -1;
    }

  if (recreate_row)
    {
      struct ftn_entry *prev_ftn = NULL;
      mpls_lsp_priority_t ftn_pri;

      primary_lsp = NSM_FALSE;

      if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED))
        primary_lsp = NSM_TRUE;

      ftn_pri = gmpls_owner_priority (ftn->owner.owner);

      mpls_ftn_lookup_primary (rn->info, ftn_pri, ftn->tun_id, ftn->flags, 
                               &prev_ftn);

      gmpls_ftn_entry_remove_list (nm, (struct ftn_entry **)&rn->info,
                                  ftn->ftn_ix, rn, NSM_FALSE);

      if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED))
        {
          if (data->ftn.type == gmpls_entry_type_ip)
            {
#ifdef HAVE_IPV6
              if (data->ftn.u.prefix.family == AF_INET6)
                nsm_mpls_ftn6_del_from_fwd (nm, ftn, &data->ftn.u.prefix, 
                                            rn->tree->id, NULL);
          else
#endif
                nsm_mpls_ftn4_del_from_fwd (nm, ftn, &data->ftn.u.prefix, 
                                            rn->tree->id, NULL);
            }

          UNSET_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED);
        }
      
      gmpls_ftn_row_cleanup (nm, ftn, &data->ftn, rn->tree->id, NSM_TRUE);
      ret = gmpls_ftn_row_create (nm, ftn, data);
      if (ret != NSM_SUCCESS)
        {
          if (ftn->ftn_ix > 0)
            {
              bitmap_release_index (pix_tbl->ix_mgr, ftn->ftn_ix);
              ftn->ftn_ix = 0;
            }

          gmpls_ftn_free (ftn);

          if (rn->info == NULL)
            ptree_unlock_node (rn);

          return ret;
        }

      if (primary_lsp)
        {
          ret = gmpls_ftn_entry_add_list_primary (nm, (struct ftn_entry **)
                                                  &rn->info,
                                                  ftn, prev_ftn);
          ftn->igp_status = FTN_ENTRY_IGP_STATUS_CANDIDATE;
        }
      else
        ret = gmpls_ftn_entry_add_list (nm, (struct ftn_entry **)&rn->info, 
                                        ftn, rn, NSM_FALSE);
      if (ret != NSM_SUCCESS)
        goto cleanup;
    }

  if ((CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_SELECTED)) &&
      (! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED)))
    {
      ret = gmpls_ftn_entry_select_process (nm, ftn, rn);
      if (ret != NSM_SUCCESS)
        return NSM_ERR_FTN_INSTALL_FAILURE;

      if (ftn->igp_status == FTN_ENTRY_IGP_STATUS_CANDIDATE)
        {
          if (ftn->next) 
            gmpls_ftn_add_select_update (nm, ftn, rn);

          /* above gmpls_ftn_add_select_update might change igp_status */
          if (ftn->igp_status == FTN_ENTRY_IGP_STATUS_CANDIDATE &&
              ftn->lsp_bits.bits.igp_shortcut && 
              ftn->ent_type == ILM_ENTRY_TYPE_PACKET)
            {
              nsm_gmpls_set_fec_from_ftn (ftn, &fec);
              nsm_mpls_send_igp_shortcut_lsp (ftn, &fec.u.prefix,
                                              NSM_MSG_IGP_SHORTCUT_ADD);

              ftn->igp_status = FTN_ENTRY_IGP_STATUS_ADVERTISED; 
            }
          else if (ftn->igp_status != FTN_ENTRY_IGP_STATUS_ADVERTISED)
            ftn->igp_status = FTN_ENTRY_IGP_STATUS_NONE;
        }
      igp_shortcut = -1;
    }
  
  FTN_ROW_STATUS_ACTIVATE (ftn);
  FTN_XC_STATUS_UPDATE (ftn);

  if (igp_shortcut != -1 && ftn->ent_type == ILM_ENTRY_TYPE_PACKET)
    {
      nsm_gmpls_set_fec_from_ftn (ftn, &fec);
      nsm_mpls_send_igp_shortcut_lsp (ftn, &fec.u.prefix, igp_shortcut);
      if (igp_shortcut == NSM_MSG_IGP_SHORTCUT_DELETE)
        ftn->igp_status = FTN_ENTRY_IGP_STATUS_NONE; 
      else
        ftn->igp_status = FTN_ENTRY_IGP_STATUS_ADVERTISED;
    }

#ifdef HAVE_SNMP
  if (nmpls->notificationcntl == NSM_MPLS_NOTIFICATIONCNTL_ENA &&
      CHECK_FLAG (ftn->xc->flags, XC_ENTRY_FLAG_OPR_STS_CHG)) 
    nsm_mpls_opr_sts_trap (nmpls, ftn->xc->key.xc_ix, ftn->xc->opr_status);
#endif  /* HAVE_SNMP */
  return NSM_SUCCESS;

 cleanup:
  gmpls_ftn_entry_cleanup (nm, ftn, &data->ftn, pix_tbl);
  gmpls_ftn_free (ftn);
  
  if (rn->info == NULL)
    ptree_unlock_node (rn);

  return ret;
}

static s_int32_t
gmpls_ilm_row_create (struct nsm_master *nm,
                      struct ilm_entry *ilm, struct ilm_add_gen_data *data,
                      u_int32_t old_xc_ix, u_int32_t old_nhlfe_ix)
{
  struct nhlfe_entry *nhlfe;
  struct fec_gen_entry fec;
  struct xc_entry *xc = NULL;
#ifdef HAVE_GMPLS
  struct ilm_entry *rev_ilm = NULL;
  struct ftn_entry *rev_ftn = NULL;
#endif /* HAVE_GMPLS */
  bool_t nhlfe_created = NSM_FALSE;
  bool_t xc_created = NSM_FALSE;
  struct xc_new_data xcd;
  struct prefix nh_p;
  s_int32_t ret;

  /* lookup nhlfe entry */
  nhlfe = nsm_gmpls_nhlfe_lookup (nm, &data->nh_addr, data->oif_ix,
                                  &data->out_label, NSM_FALSE);
  if (! nhlfe)
    {
      struct nhlfe_new_data nhd;

      /* set up data for creating nhlfe */
      pal_mem_set (&nhd, 0, sizeof (struct nhlfe_new_data));
      nhd.owner = data->owner.owner;
      nhd.opcode = data->opcode;
      nhd.oif_ix = data->oif_ix;
      nhd.out_label = data->out_label;
      pal_mem_cpy (&nhd.nh_addr, &data->nh_addr, sizeof (struct addr_in));

      /* create a new nhlfe entry */
      nhlfe = gmpls_nhlfe_new (&nhd, &ret);
      if (! nhlfe)
        goto cleanup;
        
      nhlfe_created = NSM_TRUE;

      /* Set the nhlfe index with stored index */
      nhlfe->nhlfe_ix = old_nhlfe_ix;

      /* add nhlfe to table */
      ret = gmpls_nhlfe_add (nm, nhlfe);
      if (ret != NSM_SUCCESS)
        goto cleanup;
    }
  else
    {
      /* make sure that duplicate ilm row is not created */
      NSM_ASSERT (nhlfe->nhlfe_ix > 0);
      NSM_ASSERT (nhlfe->xc_ix > 0);
      if (nhlfe->nhlfe_ix == 0 || nhlfe->xc_ix == 0)
        {
          ret = NSM_ERR_INVALID_NHLFE;
          goto cleanup;
        }

      /* get xc entry */
      xc = nsm_gmpls_xc_lookup (nm, nhlfe->xc_ix, data->iif_ix, 
                                &data->in_label, nhlfe->nhlfe_ix,
                                NSM_FALSE);
      if (xc)
        {
          ret = NSM_ERR_INVALID_XC;
          goto cleanup;
        }
    }

  /* set up data for creating xc entry */
  pal_mem_set (&xcd, 0, sizeof (struct xc_new_data));
  xcd.owner = data->owner.owner;
  xcd.iif_ix = data->iif_ix;
  xcd.in_label = data->in_label;
  pal_mem_set (&xcd.lspid, 0, sizeof (struct mpls_lspid));

  if (data->owner.owner == MPLS_RSVP)
    pal_mem_cpy (&xcd.lspid, &data->lspid, sizeof (struct mpls_lspid));

  /* create new xc entry */
  xc = gmpls_xc_new (&xcd, &ret);
  if (! xc)
    {
      if (nhlfe_created)
        gmpls_nhlfe_entry_remove (nm, nhlfe, NSM_FALSE);

      goto cleanup;
    }

  xc_created = NSM_TRUE;

  /* link xc to nhlfe */
  gmpls_xc_nhlfe_link (xc, nhlfe);

  /* Set xc index with the saved index */
  XC_IX (xc) = old_xc_ix;

  /* add xc to table */
  ret = gmpls_xc_add (nm, xc);
  if (ret != NSM_SUCCESS)
    {
      gmpls_xc_nhlfe_unlink (xc);
      xc->key.nhlfe_ix = 0;

      if (nhlfe_created)
        {
          /* remove nhlfe entry from table */
          gmpls_nhlfe_entry_remove (nm, nhlfe, NSM_FALSE);
        }
      goto cleanup;
    }

  /* link ilm to xc */
  gmpls_ilm_xc_link (ilm, xc);
  
  /* add ilm entry to table */

  /*  ret = mpls_ilm_add (nm, ilm);     */
  /* mpls_ilm_add is split into the following 2 functions for SNMP support */

#ifdef HAVE_GMPLS
  /* Check if bidirectional LSP and xc_ix set */
  if (CHECK_FLAG (data->flags, NSM_MSG_ILM_BIDIR))
    {
      if ((data->rev_ftn_ix != 0) || (data->rev_ilm_ix != 0))
        {
          /* reverse entry find */
          if (data->rev_ilm_ix != 0)
            {
              /* Find and map reverse ILM entry */
              rev_ilm = nsm_gmpls_ilm_ix_lookup (nm, data->rev_ilm_ix);

              if (rev_ilm == NULL)
                {
                  ret = NSM_ERR_INVALID_ARGS;
                  gmpls_ilm_xc_unlink (ilm);
                  if (xc_created)
                    {
                      gmpls_xc_nhlfe_unlink (xc);
                      avl_remove (XC_TABLE, xc);
                    }
                  if (nhlfe_created)
                    gmpls_nhlfe_entry_remove (nm, nhlfe, NSM_FALSE);
                  goto cleanup;
                }

              ilm->rev_entry = rev_ilm;
              UNSET_FLAG (ilm->flags, ILM_ENTRY_FLAG_REV_FTN);

              rev_ilm->rev_entry = ilm;
              UNSET_FLAG (rev_ilm->flags, ILM_ENTRY_FLAG_REV_FTN);
            }
          else
            {
              /* For bidirectional LSP the entry type is always IP */
              rev_ftn = nsm_gmpls_ftn_ix_lookup (nm, data->rev_ftn_ix,
                                                 &fec);
              if ((rev_ftn == NULL) || (fec.type != gmpls_entry_type_ip) ||
                  (fec.u.prefix.family != AF_INET))
                {
                  ret = NSM_ERR_INVALID_ARGS;
                  gmpls_ilm_xc_unlink (ilm);
                  if (xc_created)
                    {
                      gmpls_xc_nhlfe_unlink (xc);
                      avl_remove (XC_TABLE, xc);
                    }
                  if (nhlfe_created)
                    gmpls_nhlfe_entry_remove (nm, nhlfe, NSM_FALSE);
                  goto cleanup;
                }
              ilm->rev_entry = rev_ftn;
              SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_REV_FTN);

              rev_ftn->ilm_rev = ilm;
            }
        }
    }
#endif /* HAVE_GMPLS */

  ret = gmpls_ilm_create (nm, ilm);

  if (ret == NSM_SUCCESS && ! CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_INACTIVE))
    {
      if (ilm->owner.owner != MPLS_OTHER_LDP_VC)
        {
          if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_DEPENDENT))
            {
              if (gmpls_set_dep_ilm_nhprefix (&nh_p, ilm))
                {
                  fec.type = gmpls_entry_type_ip;
                  fec.u.prefix = nh_p;
                  gmpls_lsp_dep_add (nm, &fec, CONFIRM_RSVP_BYPASS_ILM, ilm);
                }
            }
          else
            ret = gmpls_ilm_add_entry_to_fwd_tbl (nm, ilm, NULL);
        }

    }

  if (ret == NSM_ERR_ILM_INSTALL_FAILURE)
    return ret;

  if (ret != NSM_SUCCESS)
    {
      gmpls_ilm_row_cleanup (nm, ilm, NSM_FALSE);
      
      goto cleanup;
    }
  
  return NSM_SUCCESS;
  
 cleanup:

  if (nhlfe && nhlfe_created)
    gmpls_nhlfe_free (nhlfe);
  
  if (xc && xc_created)
    gmpls_xc_free (xc);
  
  return ret;
}


static s_int32_t
gmpls_ilm_add_process (struct nsm_master *nm,
                       struct ilm_add_gen_data *data, bool_t mapped_ilm, 
                       struct ilm_entry **ppilm, bool_t config)
{
  struct ilm_entry *ilm;
#ifdef HAVE_GMPLS
  struct gmpls_if *gmif;
  struct datalink *dlin, *dlout;
#endif /* HAVE_GMPLS */
  struct interface *ifp_in, *ifp_out;
  s_int32_t ret;

  *ppilm = NULL;

  /* create ilm entry */
  ilm = gmpls_ilm_new (data, mapped_ilm, &ret, config);
  if (! ilm)
    return ret;

#ifdef HAVE_GMPLS
  gmif = gmpls_if_get (nm->vr->zg, nm->vr->id);
  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_GMPLS))
    {
      dlin = data_link_lookup_by_index (gmif, data->iif_ix);
      dlout = data_link_lookup_by_index (gmif, data->oif_ix);
      if (dlin && (!IS_DATA_LINK_UP (dlin) || !dlin->ifp))
        {
          SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_INACTIVE);  
        }

      if (dlout && (!IS_DATA_LINK_UP (dlout) || !dlout->ifp))
        {
          SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_INACTIVE);  
        }

      if (dlout && ((dlout->ifp && !NSM_MPLS_VALID_INTERFACE (dlout->ifp)) ||
          (dlin->ifp && !NSM_MPLS_VALID_INTERFACE (dlin->ifp))))
        {
          SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_INACTIVE);  
        }
    }
  else
    {
      ifp_in = if_lookup_by_gindex (gmif, data->iif_ix);
      if (ifp_in && ! NSM_MPLS_VALID_INTERFACE (ifp_in))
        SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_INACTIVE);

      if (data->oif_ix != NSM_MPLS->loop_gindex)
        {
          ifp_out = if_lookup_by_gindex (gmif, data->oif_ix);
          if (ifp_out && (! NSM_MPLS_VALID_INTERFACE (ifp_out)) &&
              (! NSM_MPLS_PHP_ENTRY (ifp_out, data->opcode)))
            SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_INACTIVE);
        }
    }
#else /* HAVE_GMPLS */
    {
      ifp_in = if_lookup_by_index (&nm->vr->ifm, data->iif_ix);
      if (! NSM_MPLS_VALID_INTERFACE (ifp_in))
        SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_INACTIVE);

       ifp_out = if_lookup_by_index (&nm->vr->ifm, data->oif_ix);
       if ((! NSM_MPLS_VALID_INTERFACE (ifp_out)) &&
           (! NSM_MPLS_PHP_ENTRY (ifp_out, data->opcode)))
          SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_INACTIVE);
    }
#endif /* HAVE_GMPLS */

  ret = gmpls_ilm_row_create (nm, ilm, data, NSM_FALSE, NSM_FALSE);
  if (ret == NSM_ERR_ILM_INSTALL_FAILURE)
    return ret;

  if (ret == NSM_SUCCESS)
    *ppilm = ilm;
  else
    gmpls_ilm_free (ilm);

  return ret;
}

static s_int32_t
gmpls_ilm_update (struct nsm_master *nm,
                  struct ilm_entry *ilm, struct ilm_add_gen_data *data)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_key *nkey;
  struct nhlfe_key_gen *key = NULL;
  struct gmpls_gen_label lbl;
  bool_t recreate_row = NSM_FALSE;
  s_int32_t proto_id;
  s_int32_t ret;
  u_int32_t nhlfe_ix = 0;
  u_int32_t xc_ix = 0;

  NSM_ASSERT ((ilm != NULL) && (data != NULL));
  if ((! ilm) || (! data))
    return NSM_ERR_INVALID_ARGS;

  if (ilm->owner.owner != data->owner.owner)
    return NSM_ERR_OWNER_MISMATCH;

  proto_id = gmpls_owner_to_proto (ilm->owner.owner);

  ilm->row_status = RS_NOT_READY;

  if (! mpls_owner_same (&ilm->owner, &data->owner))
    pal_mem_cpy (&ilm->owner, &data->owner, sizeof (struct mpls_owner));

  if (ilm->n_pops != data->n_pops)
    ilm->n_pops = data->n_pops;

  if (ilm->family == data->fec_prefix.family)
    {
      ilm->prefixlen = data->fec_prefix.prefixlen;

#ifdef HAVE_IPV6
      if (ilm->family == AF_INET6)
        pal_mem_cpy (ilm->prfx, &data->fec_prefix.u.prefix6,
                     sizeof (struct pal_in6_addr));
      else
#endif
        pal_mem_cpy (ilm->prfx, &data->fec_prefix.u.prefix4,
                     sizeof (struct pal_in4_addr));
       
    }

  if (ilm->xc &&
      pal_mem_cmp (&ilm->xc->lspid, &data->lspid,
                   sizeof (struct mpls_lspid)) != 0)
    pal_mem_cpy (&ilm->xc->lspid, &data->lspid, sizeof (struct mpls_lspid));

#ifdef HAVE_PACKET
#ifdef HAVE_DIFFSERV
  /* update ds info */
  if (pal_mem_cmp (&ilm->ds_info, &data->ds_info, 
                   sizeof (struct ds_info)) != 0)
    {
      pal_mem_cpy (&ilm->ds_info, &data->ds_info, sizeof (struct ds_info));
      recreate_row = NSM_TRUE;
    }
#endif /* HAVE_DIFFSERV */
#endif /* HAVE_PACKET */

  if (ilm->xc)
    nhlfe = ILM_NHLFE (ilm);
  NSM_ASSERT (nhlfe != NULL);
  if (! nhlfe)
    return NSM_ERR_INTERNAL;

  if ((! recreate_row) &&
      ! ((nhlfe->opcode == data->opcode) ||
        (nhlfe->opcode == PUSH && data->opcode == SWAP && ilm->n_pops == 1)))
    recreate_row = NSM_TRUE;

  if  (! recreate_row)
    {
      nkey = (struct nhlfe_key *) nhlfe->nkey;
      key = (struct nhlfe_key_gen *) nhlfe->nkey;
      switch (nhlfe->type)
        {
          case gmpls_entry_type_ip:
            lbl.type = gmpls_entry_type_ip;

            if ((nkey->afi == AFI_IP)
#ifdef HAVE_GMPLS
                || (nkey->afi == AFI_UNNUMBERED)
#endif /* HAVE_GMPLS */
                )
              {
                lbl.u.pkt = nkey->u.ipv4.out_label;
                if ((data->nh_addr.afi != AFI_IP) ||
                    (! IPV4_ADDR_SAME (&nkey->u.ipv4.nh_addr,
                                       &data->nh_addr.u.ipv4)) ||
                    (nkey->u.ipv4.oif_ix != data->oif_ix) ||
                    (gmpls_are_label_equal (&lbl, &data->out_label)))
                  recreate_row = NSM_TRUE;
                else if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE))
                  {
                    THREAD_TIMER_OFF (ng->restart[proto_id].t_preserve);
                    UNSET_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE);
                  }
              }
#ifdef HAVE_IPV6
            else
              {
                lbl.u.pkt = nkey->u.ipv6.out_label;
 
                if ((data->nh_addr.afi != AFI_IP6) ||
                    (! IPV6_ADDR_SAME (&nkey->u.ipv6.nh_addr,
                                       &data->nh_addr.u.ipv6)) ||
                    (nkey->u.ipv6.oif_ix != data->oif_ix) ||
                    (gmpls_are_label_equal (&lbl, &data->out_label)))
                  recreate_row = NSM_TRUE;
                else if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE))
                  {
                    THREAD_TIMER_OFF (ng->restart[proto_id].t_preserve);
                    UNSET_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE);
                  }
              }
#endif /* HAVE_IPV6 */
            break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
          case gmpls_entry_type_pbb_te:
           lbl.type = gmpls_entry_type_pbb_te;

           if(key)
             {
               pal_mem_cpy(&lbl.u.pbb,
                   &key->u.pbb.lbl, sizeof(struct pbb_te_label));

               if ((key->u.pbb.oif_ix != data->oif_ix) ||
                  (gmpls_are_label_equal (&lbl, &data->out_label)))
                 recreate_row = NSM_TRUE;
               else if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE))
                 {
                   THREAD_TIMER_OFF (ng->restart[proto_id].t_preserve);
                   UNSET_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE);
                 }
             }
            break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
          case gmpls_entry_type_tdm:
            break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
           default:
             break;
        }
    }

#ifdef HAVE_GMPLS
  if (CHECK_FLAG (data->flags, NSM_MSG_ILM_GEN_MPLS))
    {
      SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_GMPLS);
    }
#endif /* HAVE_GMPLS */

  if (! recreate_row) 
    {
      if (ilm->xc && pal_mem_cmp (&ilm->xc->lspid, &data->lspid,
                                  sizeof (struct mpls_lspid)) != 0)
        pal_mem_cpy (&ilm->xc->lspid, &data->lspid, 
                     sizeof (struct mpls_lspid));
    }
  else
    {
#ifdef  HAVE_RESTART
      if (ilm->partial)
        {
          /* Save xc_ix and nhlfe_ix so that indexes can be reused */
          xc_ix    = ILM_XC_IX (ilm);
          nhlfe_ix = ILM_NHLFE_IX (ilm);
        }
#endif /* HAVE_RESTART */

      /* remove ilm entry from table */
      nsm_gmpls_ilm_lookup (nm, data->iif_ix, &data->in_label, 
                            data->ilm_ix, NSM_TRUE, NSM_FALSE);

      gmpls_ilm_row_cleanup (nm, ilm, NSM_TRUE);

      ret = gmpls_ilm_row_create (nm, ilm, data, xc_ix, nhlfe_ix); 
      if (ret == NSM_ERR_ILM_INSTALL_FAILURE)
        goto CLEANUP;

      if (ret != NSM_SUCCESS)
        {
#ifdef HAVE_RESTART
          if (ilm->partial)
            gmpls_ilm_del_partial_info (ilm);
#endif /* HAVE_RESTART */

          gmpls_ilm_free (ilm);
          return ret;
        }
    }
 
  ret =  NSM_SUCCESS;

CLEANUP :
#ifdef HAVE_RESTART
  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE))
    UNSET_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE);

  if (CHECK_FLAG (data->flags, NSM_MSG_ILM_STALE_DB_AVAILABLE))
    SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_RETAIN_STALE);

  if (ilm->partial)
     gmpls_ilm_del_partial_info (ilm);
#endif /* HAVE_RESTART */

   return ret;
}

void
nsm_gmpls_set_ftn_gen_data (struct ftn_add_gen_data *gen_data, 
                            struct ftn_add_data *data)
{
  if (CHECK_FLAG (data->flags, NSM_MSG_FTN_ADD))
    SET_FLAG (gen_data->flags, NSM_MSG_GEN_FTN_ADD);
  else
    SET_FLAG (gen_data->flags, NSM_MSG_GEN_FTN_DEL);

  if (CHECK_FLAG (data->flags, NSM_MSG_FTN_FAST_DELETE))
    SET_FLAG (gen_data->flags, NSM_MSG_GEN_FTN_FAST_DEL);

  if (CHECK_FLAG (data->flags, NSM_MSG_FTN_DSCP_IN))
    SET_FLAG (gen_data->flags, NSM_MSG_GEN_FTN_DSCP_IN);

  if (!CHECK_FLAG (data->flags, NSM_MSG_FTN_GMPLS))
    SET_FLAG (gen_data->flags, NSM_MSG_GEN_FTN_MPLS);

  if (CHECK_FLAG (data->flags, NSM_MSG_FTN_BIDIR))
    SET_FLAG (gen_data->flags, NSM_MSG_GEN_FTN_BIDIR);

  if (CHECK_FLAG (data->flags, NSM_MSG_FTN_INACT))
    SET_FLAG (gen_data->flags, NSM_MSG_GEN_FTN_INACT);

  gen_data->id = data->id;
  gen_data->owner = data->owner;
  gen_data->vrf_id = data->vrf_id;
  gen_data->ftn.type = gmpls_entry_type_ip;
  gen_data->ftn.u.prefix = data->fec_prefix;

  gen_data->ftn_ix = data->ftn_ix;
#ifdef HAVE_GMPLS
  gen_data->rev_ilm_ix = data->rev_idx;
#endif /* HAVE_GMPLS */
#ifdef HAVE_PACKET
#ifdef HAVE_DIFFSERV
  gen_data->ds_info = data->ds_info;
#endif /* HAVE_DIFFSERV */
#endif /* HAVE_PACKET */

  gen_data->nh_addr = data->nh_addr;
  gen_data->out_label.type = gmpls_entry_type_ip;
  gen_data->out_label.u.pkt = data->out_label;
  gen_data->oif_ix = data->oif_ix;

  gen_data->lspid = data->lspid;
  gen_data->exp_bits = data->exp_bits;

#ifdef HAVE_PACKET
  gen_data->dscp_in = data->dscp_in;
#endif /* HAVE_PACKET */

  gen_data->lsp_bits.data = data->lsp_bits.data;

  gen_data->opcode = data->opcode;;
  gen_data->tunnel_id = data->tunnel_id;
  gen_data->protected_lsp_id = data->protected_lsp_id;
  gen_data->qos_resrc_id = data->qos_resrc_id;
  gen_data->bypass_ftn_ix = data->bypass_ftn_ix;
  gen_data->sz_desc = data->sz_desc;
  gen_data->pri_fec_prefix = data->pri_fec_prefix;

  return;
}

/* 
   Handler for ftn add message. If dep is TRUE, 
   do not check for dependencies. 
*/
s_int32_t
nsm_mpls_ftn_add_msg_process (struct nsm_master *nm,
                              struct ftn_add_data *data, 
                              struct ftn_ret_data *ret_data,
                              mpls_ftn_type_t f_type,
                              bool_t config)
{
  struct ftn_add_gen_data gen_data;
  s_int32_t ret;

  pal_mem_set (&gen_data, '\0', sizeof (struct ftn_add_gen_data));
  nsm_gmpls_set_ftn_gen_data (&gen_data, data);

  ret = nsm_gmpls_ftn_add_msg_process (nm, &gen_data, ret_data, f_type, config);
  
  return ret;
}

bool_t
nsm_gmpls_validate_fec (struct fec_gen_entry *fec)
{
  switch (fec->type)
    {
#ifdef HAVE_PACKET
      case gmpls_entry_type_ip:
        if ((fec->u.prefix.family != AF_INET)
#ifdef HAVE_IPV6
          && (fec->u.prefix.family != AF_INET6)
#endif /* HAVE_IPV6 */
      )
          return PAL_FALSE;
              
        break;
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:

        break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:

        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        break;
    }

  return PAL_TRUE;
}

s_int32_t
nsm_gmpls_ftn_add_msg_process (struct nsm_master *nm,
                               struct ftn_add_gen_data *data,
                               struct ftn_ret_data *ret_data,
                               mpls_ftn_type_t f_type,
                               bool_t config)
{
  struct ftn_entry *ftn = NULL;
  struct ptree_ix_table *pix_tbl;
#ifdef HAVE_RESTART
  u_int8_t proto_id = 0;
#endif /* HAVE_RESTART */
  bool_t fv4, nv4;
  s_int32_t ret;
  bool_t valid;

  NSM_ASSERT (data != NULL);
  if (! data)  
    return NSM_ERR_INVALID_ARGS;

  /* validate fec prefix family */
   valid = nsm_gmpls_validate_fec (&data->ftn);
   if (valid == PAL_FALSE)
     {
       return NSM_ERR_INVALID_ADDR_FAMILY;
     }

#ifdef HAVE_PACKET
  if (data->ftn.type == gmpls_entry_type_ip)
    fv4 = data->ftn.u.prefix.family == AF_INET ?
          NSM_TRUE :
          NSM_FALSE;

  /* validate opcode */
  switch (data->opcode)
    {
      case PUSH :
        if (data->oif_ix == 0)
          return NSM_ERR_INVALID_INTERFACE;

       /* Fall through to the PUSH and lookup checks */
      case PUSH_AND_LOOKUP :
#ifdef HAVE_PACKET
        if (gmpls_addr_in_zero (&data->nh_addr))
          return NSM_ERR_INVALID_NEXTHOP;
#endif /* HAVE_PACKET */

        if ((data->out_label.type == gmpls_entry_type_ip) &&
            (data->out_label.u.pkt > LABEL_VALUE_MAX))
         return NSM_ERR_INVALID_LABEL;
      break;

    case DLVR_TO_IP :
        if (data->oif_ix == 0)
          return NSM_ERR_INVALID_INTERFACE;
      break;

    default :
      return NSM_ERR_INVALID_OPCODE;
    }

  /* nexthop address family validation */
  if ((data->nh_addr.afi != AFI_IP)
#ifdef HAVE_IPV6
      && (data->nh_addr.afi != AFI_IP6)
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
      && (data->nh_addr.afi != AFI_UNNUMBERED)
#endif /* HAVE_GMPLS */
      )
    return NSM_ERR_INVALID_ADDR_FAMILY;

  /* fec and nexthop should have same address family */
  nv4 = data->nh_addr.afi == AFI_IP ?
    NSM_TRUE :
    NSM_FALSE;
#endif /* HAVE_PACKET */

  /* get relevant ftn table */
  pix_tbl = nsm_gmpls_ftn_table_lookup (nm, data->vrf_id, &data->ftn);
  if (! pix_tbl)
    return NSM_ERR_INVALID_VRF_ID;

#ifdef HAVE_RESTART
  proto_id = gmpls_owner_to_proto (data->owner.owner);

  /* Lookup for stale entries is only done when the GR is in progess.
     By the time the GR moves to inactive state all the entries would
     be updated. */

  if (data->ftn_ix == 0 &&
      nsm_restart_state (proto_id))
    {
      ftn = nsm_gmpls_ftn_lookup_with_data (nm, pix_tbl, &data->ftn,
                                           &data->owner);

      if (ftn)
        data->ftn_ix = ftn->ftn_ix;
    }
#endif /* HAVE_RESTART */

  /* check for ftn update */
  if (data->ftn_ix > 0)
    {
      /* get existing ftn entry */
      ftn = nsm_gmpls_ftn_lookup (nm, pix_tbl, &data->ftn, data->ftn_ix, 
                                  NSM_FALSE);
      if (! ftn)
        return NSM_ERR_NO_FTN_ENTRY;

      /* update ftn entry */
      ret = gmpls_ftn_update_process (nm, pix_tbl, data, ftn);
    }
  else
    {
      if (f_type == MPLS_FTN_NONE)
        {
          switch (data->owner.owner)
            {
            case MPLS_LDP :
            case MPLS_RSVP :
              if (data->pri_fec_prefix)
                f_type = MPLS_FTN_RSVP_MAPPED;
              else
                f_type = MPLS_FTN_REGULAR;
                break;

            case MPLS_OTHER_CLI :
            case MPLS_SNMP :
            case MPLS_POLICYAGENT :
            case MPLS_CRLDP :
              f_type = MPLS_FTN_REGULAR;
              break;

            case MPLS_OTHER_BGP :
              if (data->vrf_id > 0)
                f_type = MPLS_FTN_BGP_VRF;
              else
                f_type = MPLS_FTN_BGP;
              break;

            default :
              return NSM_ERR_INVALID_FTN_TYPE;
            }
        }

      /* addition of new ftn entry in the table */
      ret = gmpls_ftn_add_process (nm, pix_tbl, data, &ftn, f_type, config);
    }

  if ((ret == NSM_SUCCESS) && (ret_data) && (ftn))
    {
      pal_mem_set (ret_data, 0, sizeof (struct ftn_ret_data));
      SET_FLAG (ret_data->flags, NSM_MSG_REPLY_TO_FTN_ADD);
      pal_mem_cpy (&ret_data->owner, &ftn->owner, sizeof (struct mpls_owner));
      ret_data->ftn_ix = ftn->ftn_ix;
      ret_data->xc_ix = FTN_XC_IX (ftn);
      ret_data->nhlfe_ix = FTN_NHLFE_IX (ftn);
    }

  return ret;
}

s_int32_t
nsm_gmpls_ftn_fast_add_msg_process (struct nsm_master *nm, 
                                    u_int32_t vrf_id,
                                    struct fec_gen_entry *fec,
                                    u_int32_t ftn_ix)
{
#ifdef HAVE_GMPLS
  struct ftn_entry *ftn;
  struct ilm_entry ilm;
  struct ptree_ix_table *tbl;
  s_int32_t ret;
 
  NSM_ASSERT (fec != NULL);

  if (ftn_ix == 0)
    return NSM_ERR_INVALID_FTN_INDEX;

  tbl = nsm_gmpls_ftn_table_lookup (nm, vrf_id, fec);
  if (! tbl)
    return NSM_ERR_INVALID_VRF_ID;

  /* Find ftn entry */
  ftn = nsm_gmpls_ftn_lookup (nm, tbl, fec, ftn_ix, NSM_TRUE);
  if (! ftn)
    return NSM_ERR_NO_FTN_ENTRY;

  /* check if the flag was not dummy and now is dummy */
  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_DUMMY))
    {
      UNSET_FLAG (ftn->flags, FTN_ENTRY_FLAG_DUMMY);

      /* Add to forwarder */
      gmpls_ftn_entry_select_process (nm, ftn, ftn->pn);

      /* Check for reverse entry */
      ret = nsm_gmpls_bidir_ftn_get_rev_entry (ftn, &ilm);
      if (ret != NSM_SUCCESS)
        {
          gmpls_ilm_add_entry_to_fwd_tbl (nm, &ilm, NULL);
        }

    }
#endif /* HAVE_GMPLS */

  return NSM_SUCCESS;
}


/* Handler for ftn delete message */
s_int32_t
nsm_gmpls_ftn_del_msg_process (struct nsm_master *nm,
                               u_int32_t vrf_id, 
                               struct fec_gen_entry *fec, 
                               u_int32_t ftn_ix)
{
  struct ftn_entry *ftn;
  struct ptree_ix_table *tbl;
  bool_t valid;
  
  NSM_ASSERT (fec != NULL);
  NSM_ASSERT (ftn_ix > 0);
  
  valid = nsm_gmpls_validate_fec (fec);
  if (valid == PAL_FALSE)
    {
      return NSM_ERR_INVALID_ADDR_FAMILY;
    }

  /* validata ftn index */
  if (ftn_ix == 0)
    return NSM_ERR_INVALID_FTN_INDEX;

  tbl = nsm_gmpls_ftn_table_lookup (nm, vrf_id, fec);
  if (! tbl)
    return NSM_ERR_INVALID_VRF_ID;
       
  /* remove ftn entry */
  ftn = nsm_gmpls_ftn_lookup (nm, tbl, fec, ftn_ix, NSM_TRUE);
  if (! ftn)
    return NSM_ERR_NO_FTN_ENTRY;

  gmpls_ftn_entry_cleanup (nm, ftn, fec, tbl);
  
  /* release memory */
  gmpls_ftn_free (ftn);
  
  return NSM_SUCCESS;
}


#ifdef HAVE_PACKET
s_int32_t
nsm_mpls_ftn_del_msg_process (struct nsm_master *nm,
                              u_int32_t vrf_id,
                              struct prefix *pfx,
                              u_int32_t ftn_ix)

{
  struct fec_gen_entry fec;

  pal_mem_set (&fec, '\0', sizeof (struct fec_gen_entry));
  fec.type = gmpls_entry_type_ip;
  fec.u.prefix = *pfx;

  nsm_gmpls_ftn_del_msg_process (nm, vrf_id, &fec, ftn_ix);
  return 0;
}
#endif /* HAVE_PACKET */

s_int32_t
nsm_mpls_ftn_del_slow_msg_process (struct nsm_master *nm,
                                   struct ftn_add_data *data)
{
  struct ftn_add_gen_data gen_data;

  pal_mem_set (&gen_data, '\0', sizeof (struct ftn_add_gen_data));
  nsm_gmpls_set_ftn_gen_data (&gen_data, data);

  return nsm_gmpls_ftn_del_slow_msg_process (nm, &gen_data);
}

/* Handler for ftn slow delete message */
s_int32_t
nsm_gmpls_ftn_del_slow_msg_process (struct nsm_master *nm,
                                    struct ftn_add_gen_data *data)
{
  struct ftn_entry *ftn;
  struct ptree_ix_table *pix_tbl;

  pix_tbl = nsm_gmpls_ftn_table_lookup (nm, data->vrf_id, &data->ftn);

  if ((! pix_tbl) || (! pix_tbl->m_table) || (! pix_tbl->ix_mgr))
    return NSM_ERR_NO_FTN_ENTRY;

  /* remove ftn entry */
  ftn = nsm_gmpls_ftn_slow_lookup (nm, pix_tbl, data, NSM_TRUE);
  if (! ftn)
    return NSM_ERR_NO_FTN_ENTRY;

  gmpls_ftn_entry_cleanup (nm, ftn, &data->ftn, pix_tbl); 

  /* release memory */
  gmpls_ftn_free (ftn);

  return NSM_SUCCESS;
}

#ifdef HAVE_PACKET
void
nsm_gmpls_set_ilm_gen_data (struct ilm_add_gen_data *gdata,
                            struct ilm_add_data *data)
{
  gdata->flags = data->flags;
  SET_FLAG (gdata->flags, NSM_MSG_ILM_TRANS_MPLS);
  gdata->id = data->id;
  gdata->owner = data->owner;
  gdata->iif_ix = data->iif_ix;

  gdata->in_label.type = gmpls_entry_type_ip;
  gdata->in_label.u.pkt = data->in_label;
  gdata->ilm_ix = data->ilm_ix;
#ifdef HAVE_PACKET
#ifdef HAVE_DIFFSERV
  gdata->ds_info = data->ds_info;
#endif /* HAVE_DIFFSERV */
#endif /* HAVE_PACKET */

  gdata->oif_ix = data->oif_ix;

  gdata->out_label.type = gmpls_entry_type_ip;
  gdata->out_label.u.pkt = data->out_label;

  gdata->nh_addr = data->nh_addr;
  gdata->lspid = data->lspid;
  gdata->n_pops = data->n_pops;

  gdata->fec_prefix = data->fec_prefix;
  gdata->qos_resrc_id = data->qos_resrc_id;
  gdata->opcode = data->opcode;
  gdata->primary = data->primary;

  return;
}

s_int32_t
nsm_gmpls_ilm_add_msg_process_pkt (struct nsm_master *nm,
                                   struct ilm_add_data *data, 
                                   bool_t mapped_ilm,
                                   struct ilm_ret_data *ret_data, 
                                   bool_t config)
{
  struct ilm_add_gen_data gdata;
  struct ilm_gen_ret_data gret;
#ifdef HAVE_GMPLS
  struct interface *intf;
#endif /* HAVE_GMPLS */
  s_int32_t ret = 0;

  pal_mem_set (&gdata, 0, sizeof (struct ilm_add_gen_data));
  pal_mem_set (&gret, 0, sizeof (struct ilm_gen_ret_data));

  /* convert to generalized structure - and gifindex and add call gmpls data */
  nsm_gmpls_set_ilm_gen_data (&gdata, data);

#ifdef HAVE_GMPLS
  intf = if_lookup_by_index (&nm->vr->ifm, data->oif_ix);
  if (! intf)
    return NSM_ERR_IF_NOT_FOUND;

  gdata.oif_ix = intf->gifindex;

  intf = if_lookup_by_index (&nm->vr->ifm, data->iif_ix);
  if (! intf)
    return NSM_ERR_IF_NOT_FOUND;

  gdata.iif_ix = intf->gifindex;
#endif /* HAVE_GMPLS */

  ret = nsm_gmpls_ilm_add_msg_process (nm, &gdata, mapped_ilm, &gret, config);
  
  /* Convert the return data to correct type */
  ret_data->flags = gret.flags;
  ret_data->id = gret.id;
  ret_data->owner = gret.owner;
#ifdef HAVE_GMPLS
  ret_data->iif_ix = data->iif_ix;
#else /* HAVE_GMPLS */
  ret_data->iif_ix = gret.iif_ix;
#endif /* HAVE_GMPLS */

  ret_data->in_label = gret.in_label.u.pkt;
  ret_data->ilm_ix = gret.ilm_ix;
  ret_data->xc_ix = gret.xc_ix;
  ret_data->nhlfe_ix = gret.nhlfe_ix;

  return ret;
}
#endif /* HAVE_PACKET */

bool_t
nsm_gmpls_verify_label (struct gmpls_gen_label *label)
{
  switch (label->type)
    {
#ifdef HAVE_PACKET
      case gmpls_entry_type_ip:
         if (label->u.pkt > LABEL_VALUE_MAX)
           return PAL_FALSE;
         break;
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
         break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
         break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
        return PAL_FALSE;
    }

  return PAL_TRUE;
}

/* Handler for ilm add message - will be called for both GMPLS and MPLS (a 
   subset of GMPLS)*/
int 
nsm_gmpls_ilm_add_msg_process (struct nsm_master *nm,
                               struct ilm_add_gen_data *data, 
                               bool_t mapped_ilm,
                               struct ilm_gen_ret_data *ret_data, 
                               bool_t config)
{
  s_int32_t ret;
  struct ilm_entry *ilm = NULL;

  NSM_ASSERT (data != NULL);
  if (! data)
    return NSM_ERR_INVALID_ARGS;

  if (nsm_gmpls_verify_label (&data->in_label) == PAL_FALSE)
    return NSM_ERR_INVALID_LABEL;

#ifdef HAVE_PACKET
  /* special labels (0-15) are not stored in the ilm table */
  if ((data->in_label.type ==  gmpls_entry_type_ip) &&
      (data->in_label.u.pkt < LABEL_VALUE_INITIAL))
    {
      if (ret_data)
        {
          pal_mem_set (ret_data, 0, sizeof (struct ilm_ret_data));
          pal_mem_cpy (&ret_data->owner, &data->owner, 
                       sizeof (struct mpls_owner));
        }

      return NSM_SUCCESS;
    }
#endif /* HAVE_PACKET */

  if (data->iif_ix == 0)
    return NSM_ERR_INVALID_INTERFACE;
  
  /* nexthop address family validation */
  if ((data->nh_addr.afi != AFI_IP)
#ifdef HAVE_IPV6
      && (data->nh_addr.afi != AFI_IP6)
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
      && (data->nh_addr.afi != AFI_UNNUMBERED)
#endif /* HAVE_GMPLS */
      )
    return NSM_ERR_INVALID_ADDR_FAMILY;
  
  switch (data->opcode)
    {
    case SWAP :
    case SWAP_AND_LOOKUP :
#ifdef HAVE_PACKET
      if (nsm_gmpls_verify_label (&data->out_label) == PAL_FALSE)
        return NSM_ERR_INVALID_LABEL;
#endif /* HAVE_PACKET */

      if (gmpls_addr_in_zero (&data->nh_addr) == NSM_TRUE) 
        return NSM_ERR_INVALID_NEXTHOP;

      /* fall through */

    case POP_FOR_VPN :
    case POP_FOR_VC :
    case POP :
      if (data->oif_ix == 0)
        return NSM_ERR_INVALID_INTERFACE;

      if (data->n_pops == 0)
        return NSM_ERR_INVALID_OPCODE;
      break;
      
    default :
      return NSM_ERR_INVALID_OPCODE;
    }

  /* check for update */
  if (data->ilm_ix > 0)
    {
      /* lookup ilm */
      ilm = nsm_gmpls_ilm_lookup (nm, data->iif_ix, &data->in_label, 
                                  data->ilm_ix, NSM_FALSE, NSM_FALSE);
      if (! ilm)
        {
#ifdef HAVE_RESTART
          if (CHECK_FLAG (data->flags, NSM_MSG_ILM_STALE_DB_AVAILABLE))
            {
              ilm = nsm_gmpls_ilm_lookup_by_owner (nm, data->iif_ix, 
                                                   &data->in_label, 
                                                   data->owner.owner);
              if (! ilm)
                ret = gmpls_ilm_add_process (nm, data, mapped_ilm, &ilm, 
                                             config);
              else
                ret = gmpls_ilm_update (nm, ilm, data);
             }
           else
#endif /* HAVE_RESTART */          
             return NSM_ERR_NO_ILM_ENTRY;
        }
      else 
        {
#ifdef HAVE_RESTART          
          /* ilm update */
          if (CHECK_FLAG (data->flags, NSM_MSG_ILM_OUT_LABEL_NOT_REFRESHED))
            {
              SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_OUT_LBL_NOT_REFRESHED);
              ret = gmpls_ilm_update_partial_info (data, ilm);
            }
          else
#endif /* HAVE_RESTART */            
            ret = gmpls_ilm_update (nm, ilm, data);
        }
    }
  else /* ilm add */
    {
      ret = gmpls_ilm_add_process (nm, data, mapped_ilm, &ilm, config);
    }

  if ((ret == NSM_SUCCESS) && (ret_data) && (ilm))
    {
      pal_mem_set (ret_data, 0, sizeof (struct ilm_ret_data));
      SET_FLAG (ret_data->flags, NSM_MSG_REPLY_TO_ILM_ADD);
      pal_mem_cpy (&ret_data->owner, &ilm->owner, sizeof (struct mpls_owner));

      switch (ilm->ent_type)
        {
          case ILM_ENTRY_TYPE_PACKET:
            ret_data->iif_ix = ilm->key.u.pkt.iif_ix;
            ret_data->in_label.type = gmpls_entry_type_ip;
            ret_data->in_label.u.pkt = ilm->key.u.pkt.in_label;
            break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
          case ILM_ENTRY_TYPE_PBB_TE:
            ret_data->iif_ix = ilm->key.u.pbb_key.iif_ix;
            ret_data->in_label.type = gmpls_entry_type_pbb_te;
            ret_data->in_label.u.pbb = ilm->key.u.pbb_key.in_label;
            break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
          case ILM_ENTRY_TYPE_TDM:
            ret_data->iif_ix = ilm->key.u.tdm_key.iif_ix;
            ret_data->in_label.type = gmpls_entry_type_tdm;
            ret_data->in_label.u.tdm = ilm->key.u.tdm_key.in_label;
            break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
          default:
            break;
        }

      ret_data->ilm_ix = ILM_IX (ilm);
      ret_data->xc_ix = ILM_XC_IX (ilm);
      ret_data->nhlfe_ix = ILM_NHLFE_IX (ilm);
    }
  
  return ret;
}

#ifdef HAVE_PACKET
s_int32_t
nsm_mpls_ilm_fast_del_msg_process (struct nsm_master *nm, u_int32_t iif_ix,
                                   u_int32_t lbl, u_int32_t ilm_ix)
{
  struct gmpls_gen_label in_label;

  pal_mem_set (&in_label, '\0', sizeof (struct gmpls_gen_label));
  in_label.type = gmpls_entry_type_ip;
  in_label.u.pkt = lbl;

  return nsm_gmpls_ilm_fast_del_msg_process (nm, iif_ix, &in_label, ilm_ix);
}
#endif /* HAVE_PACKET */

/* Handler for ilm fast delete message */
s_int32_t
nsm_gmpls_ilm_fast_del_msg_process (struct nsm_master *nm, u_int32_t iif_ix, 
                                    struct gmpls_gen_label *in_label,
                                    u_int32_t ilm_ix)
{
  struct ilm_entry *ilm;

  if (iif_ix == 0)
    return NSM_ERR_INVALID_ARGS;

#ifdef HAVE_RESTART
  ilm = nsm_gmpls_ilm_lookup (nm, iif_ix, in_label, ilm_ix, NSM_FALSE, 
                              NSM_FALSE);
  if (ilm && CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_OUT_LBL_NOT_REFRESHED)
      && ilm->partial)
    {
      gmpls_ilm_del_partial_info (ilm);
      return NSM_SUCCESS;
    }

  if (ilm && CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_RETAIN_STALE))
    {
      UNSET_FLAG (ilm->flags, ILM_ENTRY_FLAG_RETAIN_STALE);
      SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE);
      return NSM_SUCCESS;
    }
#endif /* HAVE_RESTART */  

  ilm = nsm_gmpls_ilm_lookup (nm, iif_ix, in_label, ilm_ix, NSM_TRUE,
                              NSM_TRUE);
  if (! ilm)
    return NSM_ERR_NO_ILM_ENTRY;
   
  /* cleanup references in other tables */
  gmpls_ilm_row_cleanup (nm, ilm, NSM_TRUE);
  
  /* release memory */
  gmpls_ilm_free (ilm);

  return NSM_SUCCESS;
}

#ifdef HAVE_PACKET
s_int32_t
nsm_mpls_ilm_del_msg_process (struct nsm_master *nm, struct ilm_add_data *data)
{
  struct ilm_add_gen_data gen_data;
#ifdef HAVE_GMPLS
  struct interface *intf;
#endif /* HAVE_GMPLS */

  nsm_gmpls_set_ilm_gen_data (&gen_data, data);
#ifdef HAVE_GMPLS
  intf = if_lookup_by_index (&nm->vr->ifm, data->oif_ix);

  if (intf)
    {
      gen_data.oif_ix = intf->gifindex;
    }
  else
#endif /* HAVE_GMPLS */
    {
      gen_data.oif_ix = data->oif_ix;
    }

#ifdef HAVE_GMPLS
  intf = if_lookup_by_index (&nm->vr->ifm, data->iif_ix);
  if (intf)
    {
      gen_data.iif_ix = intf->gifindex;
    }
  else
#endif /* HAVE_GMPLS */
    {
      gen_data.iif_ix = data->iif_ix;
    }

  return nsm_gmpls_ilm_del_msg_process (nm, &gen_data);
}
#endif /* HAVE_PACKET */

/* Handler for ilm delete message */
s_int32_t
nsm_gmpls_ilm_del_msg_process (struct nsm_master *nm, 
                               struct ilm_add_gen_data *data)
{
  struct ilm_entry *ilm;
  struct avl_tree *at = NULL;

#ifdef HAVE_RESTART 
  ilm = nsm_gmpls_ilm_lookup (nm, data->iif_ix, &data->in_label, data->ilm_ix,
                              NSM_FALSE, NSM_FALSE);

  if (ilm && CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_OUT_LBL_NOT_REFRESHED)
      && ilm->partial)
    {
      gmpls_ilm_del_partial_info (ilm);
      return NSM_SUCCESS;
    }

  if (ilm && CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_RETAIN_STALE))
    {
      UNSET_FLAG (ilm->flags, ILM_ENTRY_FLAG_RETAIN_STALE);
      SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE);
      return NSM_SUCCESS;
    }
#endif /* HAVE_RESTART */

  /* remove ilm entry */
  ilm = nsm_gmpls_ilm_lookup_exact (nm, data, NSM_TRUE, NSM_TRUE);
  if (! ilm)
    return NSM_ERR_NO_ILM_ENTRY;

  gmpls_ilm_row_cleanup (nm, ilm, NSM_TRUE);

  switch (data->in_label.type)
    {
      case gmpls_entry_type_ip:
        at = ILM_TABLE;
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case  gmpls_entry_type_pbb_te:
        at = ILM_PBB_TABLE;
        break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        at = ILM_TDM_TABLE;
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
         break;
    }

  /* nsm_gmpls_ilm_lookup_exact does not delete the node */
  if (avl_is_last_node (ilm->an))
    {
      if (at)
        {
          avl_delete_node (at, ilm->an);
          ilm->an = NULL;
        }
    }
  else
    {
      avl_unlock_node (ilm->an);
    }

  /* release memory */
  gmpls_ilm_free (ilm);

  return NSM_SUCCESS;
}

void
nsm_gmpls_set_mapped_lsp_label (struct mapped_lsp_entry_head *map,
                                struct gmpls_gen_label *label, bool_t out) 
{
  if (label->type != map->type)
    return;

  switch (label->type)
    {
#ifdef HAVE_PACKET
      case gmpls_entry_type_ip:
        {
          struct mapped_lsp_entry_pkt *pkt;

          pkt = (struct mapped_lsp_entry_pkt *) map;
          if (out == PAL_TRUE)
            {
              pkt->out_label = label->u.pkt;
            }
          else
            {
              pkt->in_label = label->u.pkt;
            }

          break;
        }
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        {
          struct mapped_lsp_entry_pbb *pbb;

          pbb = (struct mapped_lsp_entry_pbb *) map;
          if (out == PAL_TRUE)
            {
              pbb->out_label = label->u.pbb;
            }
          else
            {
              pbb->in_label = label->u.pbb;
            }

          break;
        }
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        {
          struct mapped_lsp_entry_tdm *tdm;

          tdm = (struct mapped_lsp_entry_tdm *) map;
          if (out == PAL_TRUE)
            {
              tdm->out_label = label->u.tdm;
            }
          else
            {
              tdm->in_label = label->u.tdm;
            }
    
          break;
        }
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        break;  
    }

  return;
}

void
nsm_gmpls_set_mapped_lsp_fec (struct mapped_lsp_entry_head *map,
                              struct fec_gen_entry *fec)
{
  switch (fec->type)
    {
#ifdef HAVE_PACKET
      case gmpls_entry_type_ip:
        {
          struct mapped_lsp_entry_pkt *pkt;

          pkt = (struct mapped_lsp_entry_pkt *) map;
          pkt->fec = fec->u.prefix;
          break;
        }
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        {
          struct mapped_lsp_entry_pbb *pbb;

          pbb = (struct mapped_lsp_entry_pbb *) map;
          pbb->fec = fec->u.pbb;
          break;
        }
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        {
          struct mapped_lsp_entry_tdm *tdm;

          tdm = (struct mapped_lsp_entry_tdm *) map;
          tdm->fec = fec->u.tdm;
          break;
        } 
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        break;
    }

  return;
}


u_int32_t
nsm_gmpls_get_mapped_lsp_size (struct gmpls_gen_label *label)
{
  switch (label->type)
    {
#ifdef HAVE_PACKET
      case gmpls_entry_type_ip:
       return sizeof (struct mapped_lsp_entry_pkt);
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
       return sizeof (struct mapped_lsp_entry_pbb);
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
       return sizeof (struct mapped_lsp_entry_tdm);
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        break;
    }

  return 0;
}

s_int32_t
nsm_gmpls_mapped_lsp_add (struct nsm_master *nm, u_int32_t iif_ix,
                          struct gmpls_gen_label *in_label, 
                          struct gmpls_gen_label *out_label,
                          struct fec_gen_entry *fec)
{
  struct mapped_lsp_entry_head *entry = NULL;
  struct avl_node *an;
  struct avl_tree *at;
  s_int32_t ret;
  
  nsm_gmpls_get_mapped_lsp_size (in_label);

  entry = XCALLOC (MTYPE_MPLS_MAPPED_LSP_ENTRY, 
                   nsm_gmpls_get_mapped_lsp_size (in_label));
  if (! entry)
    {
      ret = NSM_ERR_MEM_ALLOC_FAILURE;
      goto RET_ERR;
    }

  entry->iif_ix = iif_ix;
  entry->type = in_label->type;

  nsm_gmpls_set_mapped_lsp_label (entry, out_label, PAL_TRUE);
  nsm_gmpls_set_mapped_lsp_label (entry, in_label, PAL_FALSE);

  nsm_gmpls_set_mapped_lsp_fec (entry, fec);
  at = nsm_gmpls_get_mapped_lsp_tree (nm, in_label);
  an = avl_search (at, entry);

  if (an)
    {
      ret = NSM_ERR_EXISTS;
      goto RET_ERR;
    }

  an = avl_insert_node (at, entry);
  if (an == NULL)
    {
      ret = NSM_FAILURE;
      goto RET_ERR;
    }

  an->info = entry;
  avl_lock_node (an);

  gmpls_lsp_dep_add (nm, fec, CONFIRM_MAPPED_LSP, entry);
  return NSM_SUCCESS;

 RET_ERR:
  if (entry)
    XFREE (MTYPE_MPLS_MAPPED_LSP_ENTRY, entry);

  return ret;
}


s_int32_t
nsm_gmpls_mapped_lsp_del (struct  nsm_master *nm, u_int32_t iif_ix,
                          struct gmpls_gen_label *in_label)
{
  struct avl_node *an;
  struct ilm_entry ilm;
  struct fec_gen_entry fec;

  gmpls_make_ilm_key (iif_ix, in_label, &ilm.key);
  an = avl_search (NSM_MPLS->mapped_lsp, (u_char *)&ilm);
  if (! an)
    return NSM_ERR_NOT_FOUND;
  
  fec.type = gmpls_entry_type_ip;
  fec.u.prefix = ((struct mapped_lsp_entry *)(an->info))->u.pkt.fec;
  gmpls_lsp_dep_del (nm, &fec, CONFIRM_MAPPED_LSP, an);

  if (avl_is_last_node (an))
    {
      /* Remove the AVL node */
      avl_delete_node (NSM_MPLS->mapped_lsp, an);
    }
  else
    {
      avl_unlock_node (an);
      an->info = NULL;
    }

  XFREE (MTYPE_MPLS_MAPPED_LSP_ENTRY, an->info);

  return NSM_SUCCESS;
}

struct avl_tree*
nsm_gmpls_get_mapped_lsp_tree (struct nsm_master *nm, 
                               struct gmpls_gen_label *lbl)
{
  switch (lbl->type)
    {
      case gmpls_entry_type_ip: 
       return NSM_MPLS->mapped_lsp;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
       return NSM_MPLS->mapped_lsp_pbb;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
       return NSM_MPLS->mapped_lsp_tdm;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        break;
    }

  return NULL;
}

void
gmpls_set_mapped_lsp_key (s_int32_t iif_ix, struct gmpls_gen_label *label, 
                          struct mapped_lsp_entry *map)
{
 
  switch (label->type)
    {
#ifdef HAVE_PACKET
      case gmpls_entry_type_ip:
          {
            map->u.pkt.iif_ix = iif_ix;
            map->u.pkt.in_label = label->u.pkt;
            break;
          }
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
          {
            map->u.pbb.iif_ix = iif_ix;
            map->u.pbb.in_label = label->u.pbb;
            break;
          }

#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
          {
            map->u.tdm.iif_ix = iif_ix;
            map->u.tdm.in_label = label->u.tdm;
            break;
          }

#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
        break;
    }
 
  return;
}

struct mapped_lsp_entry_head *
nsm_gmpls_mapped_lsp_lookup (struct  nsm_master *nm, u_int32_t iif_ix,
                             struct gmpls_gen_label *in_label)
{
  struct avl_node *an;
  struct avl_tree *at;
  struct mapped_lsp_entry map;

  gmpls_set_mapped_lsp_key (iif_ix, in_label, &map);
  
  at = nsm_gmpls_get_mapped_lsp_tree (nm, in_label);
  if (!at)
    return NULL;

  an = avl_search (at, (struct mapped_lsp_entry_head *) &map);
  if (! an)
    return NULL;

  return an->info;
}

s_int32_t
nsm_gmpls_mapped_lsp_ilm_add (struct nsm_master *nm, struct ftn_entry *ftn,
                              struct ptree_node *pn, u_char opcode)
{
  /* Fill in later */
  return 0;
}

s_int32_t
nsm_gmpls_mapped_ilm_add (struct nsm_master *nm, struct ftn_entry *ftn, 
                          struct mapped_lsp_entry_head *map, u_char opcode)
{
  struct ilm_add_gen_data iad;
  struct ilm_gen_ret_data ird;
  struct nhlfe_key_gen *key;
  s_int32_t oif_ix = 0;
  s_int32_t ret;
  
  /* Fill add data. */
  pal_mem_set (&iad, 0, sizeof (struct ilm_add_gen_data));

  iad.owner.owner = MPLS_OTHER_CLI;
  iad.iif_ix = pal_ntoh32 (map->iif_ix);

  key = (struct nhlfe_key_gen *) FTN_NHLFE (ftn)->nkey;
  switch (map->type)
    {
#ifdef HAVE_PACKET
      case gmpls_entry_type_ip:
        {
          struct mapped_lsp_entry_pkt *pkt;
          
          pkt = (struct mapped_lsp_entry_pkt *) map;

          iad.in_label.type = gmpls_entry_type_ip;
          PREP_FOR_HOST (iad.in_label.u.pkt, pkt->in_label, MAX_LABEL_BITLEN);

          iad.out_label.type = gmpls_entry_type_ip;
          PREP_FOR_HOST (iad.out_label.u.pkt, pkt->out_label, 
                         MAX_LABEL_BITLEN);

          if (key->u.pkt.afi == AFI_IP)
            oif_ix = key->u.pkt.u.ipv4.oif_ix;
#ifdef HAVE_IPV6
          else
            oif_ix = key->u.pkt.u.ipv6.oif_ix;

          if (pkt->fec.family == AF_INET6)
            {
              iad.nh_addr.afi = AFI_IP6;
              iad.nh_addr.u.ipv6 = pkt->fec.u.prefix6;
            }
          else
#endif /* HAVE_IPV6 */
            {
              iad.nh_addr.afi = AFI_IP;
              iad.nh_addr.u.ipv4 = pkt->fec.u.prefix4;
            }

          break;
        }
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        {
          struct mapped_lsp_entry_pbb *pbb;
 
          pbb = (struct mapped_lsp_entry_pbb *) map;

          iad.in_label.type = gmpls_entry_type_pbb_te;
          iad.in_label.u.pbb = pbb->in_label;

          iad.out_label.type = gmpls_entry_type_pbb_te;
          iad.out_label.u.pbb = pbb->out_label;
          oif_ix = key->u.pbb.oif_ix;
          break;
        }
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        { 
          struct mapped_lsp_entry_tdm *tdm;

          tdm = (struct mapped_lsp_entry_tdm *) map;

          iad.in_label.type = gmpls_entry_type_tdm;
          iad.in_label.u.tdm = tdm->in_label;

          iad.out_label.type = gmpls_entry_type_pbb_te;
          iad.out_label.u.tdm = tdm->out_label;
          oif_ix = key->u.tdm.oif_ix;
          break;
        }
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        break;

    }
  
  iad.opcode = opcode;

  iad.oif_ix = oif_ix;
  iad.primary = 1;
  iad.n_pops = 1;
  
  ret = nsm_gmpls_ilm_add_msg_process (nm, &iad, NSM_TRUE, &ird, NSM_FALSE);
  if (ret < 0)
    return NSM_FAILURE;

  return NSM_SUCCESS;
}

void
nsm_gmpls_mapped_lsp_get_label (struct mapped_lsp_entry_head *map,
                                struct gmpls_gen_label *label, bool_t out)
{
  switch (map->type)
    {
#ifdef HAVE_PACKET
      case gmpls_entry_type_ip:
        {
          struct mapped_lsp_entry_pkt *pkt;

          pkt = (struct mapped_lsp_entry_pkt *) map;
          label->type = gmpls_entry_type_ip;
          if (out == PAL_TRUE)
            {
              label->u.pkt = pkt->out_label;
            }
          else
            {
              label->u.pkt = pkt->in_label;
            }

          break;
        }

#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        {
          struct mapped_lsp_entry_pbb *pbb;

          pbb = (struct mapped_lsp_entry_pbb *) map;
          label->type = gmpls_entry_type_pbb_te;
          if (out == PAL_TRUE)
            {
              pal_mem_cpy(&label->u.pbb, &pbb->out_label, sizeof(struct pbb_te_label));
            }
          else
            {
              pal_mem_cpy(&label->u.pbb, &pbb->in_label, sizeof(struct pbb_te_label));
            }

          break;
        }
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        {
          struct mapped_lsp_entry_tdm *tdm;

          tdm = (struct mapped_lsp_entry_tdm *) map;
          label->type = gmpls_entry_type_tdm;
          if (out == PAL_TRUE)
            {
              label->u.tdm = pkt->out_label;
            }
          else
            {
              label->u.tdm = pkt->in_label;
            }

          break;
        }
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        break;

    }

  return;
}

s_int32_t
nsm_gmpls_mapped_ilm_del (struct nsm_master *nm,
                          struct ftn_entry *ftn,
                          struct ptree_node *pn)
{
  struct ilm_entry *ilm;
  struct gmpls_gen_label in_label;
  struct ilm_entry *ilm_head;
  struct gmpls_gen_label lbl;
  struct mapped_lsp_entry_head *map = 
                              (struct mapped_lsp_entry_head *)pn->info;
  s_int32_t iif_ix = 0;

  nsm_gmpls_mapped_lsp_get_label (map, &in_label, PAL_FALSE);

  ilm_head = gmpls_ilm_node_lookup (nm, pal_ntoh32 (map->iif_ix), &in_label);
  if (! ilm_head)
    return NSM_ERR_NOT_FOUND;
  
  for (ilm = ilm_head; ilm; ilm = ilm->next)
    {
      if (FLAG_ISSET (ilm->flags, ILM_ENTRY_FLAG_MAPPED))
        {
          switch (ilm->ent_type)
            {
              case ILM_ENTRY_TYPE_PACKET:
                iif_ix = ilm->key.u.pkt.iif_ix;
                lbl.type = gmpls_entry_type_ip;
                lbl.u.pkt = ilm->key.u.pkt.in_label;
                break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
              case ILM_ENTRY_TYPE_PBB_TE:
                iif_ix = ilm->key.u.pbb_key.iif_ix;
                lbl.type = gmpls_entry_type_pbb_te;
                lbl.u.pbb = ilm->key.u.pbb_key.in_label;
                break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
              case ILM_ENTRY_TYPE_TDM:
                iif_ix = ilm->key.u.tdm_key.iif_ix;
                lbl.type = gmpls_entry_type_tdm;
                lbl.u.tdm = ilm->key.u.tdm_key.in_label;
                break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
              default:
                break;
            }

          nsm_gmpls_ilm_fast_del_msg_process (nm, iif_ix, &lbl, ilm->ilm_ix);
          return NSM_SUCCESS;
        }
    }

  return NSM_SUCCESS;
}


/* Look "next" ILM entry. To be used by SNMP queries. */
struct ilm_entry *
nsm_gmpls_ilm_lookup_next (struct nsm_master *nm, u_int32_t iif_ix, 
                           struct gmpls_gen_label *label, u_char l_len)
{
  struct ilm_entry  tilm, *ilm = NULL;
  struct avl_node *an, *tmp_an = NULL;
  struct avl_tree *at = NULL;

  pal_mem_set (&tilm, '\0', sizeof (struct ilm_entry));

  /* Fill key. */
  gmpls_make_ilm_key (iif_ix, label, &tilm.key);

  switch (label->type)
    {
      case gmpls_entry_type_ip:
        at = ILM_TABLE;
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case  gmpls_entry_type_pbb_te:
        at = ILM_PBB_TABLE;
        break;
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        at = ILM_TDM_TABLE;
        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
         break;
    }

  if (at == NULL)
    return NULL;

  an = avl_search (at, &tilm);
  if (an == NULL)
    {
      tmp_an = avl_insert_node (at, &tilm);
      if (tmp_an == NULL)
        {
          return NULL;
        }
    }
  else
    tmp_an = an;

  for (an = avl_next (tmp_an); an; an = avl_next (an))
    {
      if ((ilm = an->info) != NULL)
        {
          break;
        }
    }

  if (tmp_an)
    {
      avl_delete_node (at, tmp_an);
    }

  return ilm;
}

struct ilm_entry *
nsm_mpls_ilm_lookup_next (struct nsm_master *nm,
                          u_int32_t iif_ix, u_int32_t ilabel, u_char l_len)
{
  struct gmpls_gen_label in_label; 

  in_label.type = gmpls_entry_type_ip;
  in_label.u.pkt = ilabel;

  return nsm_gmpls_ilm_lookup_next (nm, iif_ix, &in_label, l_len);
}

/* Clean up all mapped routes. */
void
nsm_gmpls_mapped_route_delete_all (struct nsm_master *nm)
{
  struct ptree_node *pn;
  struct mapped_route *route;
  struct ptree *pt, *nxt_pt;
  struct fec_gen_entry fec;

  if (! NSM_MPLS)
    return;

  GET_FIRST_MAPPED_ROUTE_TABLE (pt);

  while (pt)
    {
      for (pn = ptree_top (pt); pn; pn = ptree_next (pn))
        {
          if ((route = pn->info) != NULL)
            {
              /* Delete entry. */
              fec.type = gmpls_entry_type_ip;
              fec.u.prefix = route->fec;
              gmpls_lsp_dep_del (nm, &fec, CONFIRM_MAPPED_ROUTE, pn);
          
              /* Free up memory. */
              XFREE (MTYPE_MPLS_MAPPED_ROUTE, route);
              pn->info = NULL;
              ptree_unlock_node (pn);
            }
       }

      GET_NEXT_MAPPED_ROUTE_TABLE (pt, nxt_pt);
      ptree_finish (pt);
      pt = nxt_pt;
    }

  NSM_MPLS->trees [NSM_MPLS_TREE_ARRAY_MAPROUTE_TREES].cnt = 0;
}


/* Clean up all mapped lsp entries */
void
nsm_gmpls_mapped_lsp_delete_all (struct nsm_master *nm)
{
  struct avl_node *an;
  struct mapped_lsp_entry *entry;
  struct fec_gen_entry fec;

  if (! NSM_MPLS || ! NSM_MPLS->mapped_lsp)
    return;

  for (an = avl_top (NSM_MPLS->mapped_lsp); an; an = avl_next (an))
    {
      if ((entry = an->info) != NULL)
        {
          /* Delete entry. */
          fec.type = gmpls_entry_type_ip;
          fec.u.prefix = entry->u.pkt.fec;
          gmpls_lsp_dep_del (nm, &fec, CONFIRM_MAPPED_LSP, an);

          /* Free up memory. */
          XFREE (MTYPE_MPLS_MAPPED_LSP_ENTRY, entry);
          an->info = NULL;
        }
    }

  avl_finish (NSM_MPLS->mapped_lsp);
  NSM_MPLS->mapped_lsp = NULL;
}

s_int32_t
nsm_mpls_igp_shortcut_route (struct nsm_master *nm,
                             struct nsm_msg_igp_shortcut_route *isroute,
                             u_char action)
{
  struct ptree_node *t_pn = NULL;
  struct ptree_ix_table *pix_tbl;
  struct ftn_entry *t_ftn = NULL;
  struct fec_gen_entry fec, fec_dst;
  struct prefix t_p;
  struct mapped_route route;
  u_char key [40];
  u_int16_t key_len;
  
  pal_mem_set (&t_p, 0, sizeof (struct prefix));
  t_p.family = AF_INET;
  t_p.prefixlen = IPV4_MAX_PREFIXLEN;
  t_p.u.prefix4 = isroute->t_endp;

  pal_mem_set (&fec, 0, sizeof (struct fec_gen_entry));
  fec.type = gmpls_entry_type_ip;
  fec.u.prefix = t_p;

  pal_mem_set (&fec_dst, 0, sizeof (struct fec_gen_entry));
  fec_dst.type = gmpls_entry_type_ip;
  prefix_copy (&fec_dst.u.prefix, &isroute->fec);

  /* Find tunnel end point FTN entry ptree_node */
  t_pn = gmpls_ftn_node_match (nm, GLOBAL_FTN_ID, &fec);
  if (! t_pn)
    return NSM_FAILURE;

  /* Find the tunnel end point FTN with the same tunnel-id */
  t_ftn = mpls_ftn_match_tunnel_id ((struct ftn_entry *)t_pn->info, 
                                    isroute->tunnel_id);
  if (! t_ftn)
    return NSM_FAILURE;

  pix_tbl = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, &fec_dst);
  if (! pix_tbl)
    return NSM_FAILURE;

  gmpls_ftn_node_generate_key (&fec_dst, key, &key_len);

  /* Use tmp mapped_route for create ftn_add_data */
  pal_mem_set (&route, 0, sizeof (struct mapped_route));
  route.owner = MPLS_IGP_SHORTCUT;

  prefix_copy (&route.fec, &fec.u.prefix);

  switch (action)
    {
    case NSM_MSG_IGP_SHORTCUT_ADD:
      return nsm_gmpls_map_route_add (nm, t_ftn, &route, &fec_dst,
                                     MPLS_FTN_IGP_SHORTCUT, 
                                     isroute->tunnel_id);
    case NSM_MSG_IGP_SHORTCUT_DELETE: 
      return nsm_gmpls_map_route_del (nm, t_ftn, &route, &fec_dst,
                                     MPLS_FTN_IGP_SHORTCUT,
                                     isroute->tunnel_id);
    case NSM_MSG_IGP_SHORTCUT_UPDATE: 
    default:
      break;
    }
  return NSM_FAILURE;
}

s_int32_t
nsm_mpls_igp_shortcut_route_add (struct nsm_master *nm, 
                                 struct nsm_msg_igp_shortcut_route *isroute)
{
  return nsm_mpls_igp_shortcut_route (nm, isroute, NSM_MSG_IGP_SHORTCUT_ADD);
}

s_int32_t
nsm_mpls_igp_shortcut_route_delete (struct nsm_master *nm, 
                                 struct nsm_msg_igp_shortcut_route *isroute)
{
  return nsm_mpls_igp_shortcut_route (nm, isroute, NSM_MSG_IGP_SHORTCUT_DELETE);
}

s_int32_t
nsm_mpls_igp_shortcut_route_update (struct nsm_master *nm, 
                                    struct nsm_msg_igp_shortcut_route *isroute)
{
  return nsm_mpls_igp_shortcut_route (nm, isroute, NSM_MSG_IGP_SHORTCUT_UPDATE);
}

#ifdef HAVE_MPLS_VC
#if 0
void
nsm_mpls_vc_cleanup_all (struct nsm_master *nm, bool_t del_flag)
{
#if 0 /* [TODO: MKD] */
  struct route_node *rn;
  struct nsm_mpls_circuit *vc;
  struct prefix p;

  if (! NSM_MPLS || ! VC_FTN_TABLE)
    return;

  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      vc = rn->info;
      if (! vc)
        continue;
      
      gmpls_lsp_dep_del (nm, &vc->address, CONFIRM_VC, vc);
      vc->vc_ilm = NULL;
      vc->vc_nhlfe = NULL;
#if 0
      mpls_vc_ftn_dep_remove (nm, pn, &vc->nh_addr);
      mpls_vc_ftn_free (vc);
#endif
    }
#if 0
  if (del_flag)
    {
      ptree_finish (VC_FTN_TABLE);
      VC_FTN_TABLE = NULL;
    }
#endif     
#endif
  return;
}
#endif
s_int32_t
nsm_mpls_vc_cleanup(struct nsm_master *nm, struct nsm_mpls_circuit *vc)
{
  struct fec_gen_entry fec;
  struct gmpls_gen_label lbl;

    if (vc->state >= NSM_MPLS_L2_CIRCUIT_COMPLETE &&
        FLAG_ISSET (vc->flags, NSM_MPLS_VC_FLAG_DEP))
      {
        UNSET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_SELECTED);
        fec.type = gmpls_entry_type_ip;
        fec.u.prefix = vc->address;
        gmpls_lsp_dep_del (nm, &fec, CONFIRM_VC, vc);
        UNSET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_DEP);
      }

    vc->state = NSM_MPLS_L2_CIRCUIT_ACTIVE;

    /* delete dummy ilm entry for SNMP */
    if (! vc->vc_fib)
      return NSM_SUCCESS;

    lbl.type = gmpls_entry_type_ip;
    lbl.u.pkt = vc->vc_fib->in_label;
    nsm_gmpls_ilm_fast_del_msg_process (nm, vc->vc_fib->nw_if_ix, &lbl, 
                                        vc->ilm_ix);

    vc->ilm_ix = 0;

    pal_mem_set (vc->vc_fib, 0, sizeof (struct vc_fib));

    return NSM_SUCCESS;
}

s_int32_t
nsm_mpls_vc_cleanup_all (struct nsm_master *nm, bool_t del_flag)
{
    struct route_node *rn;
    struct nsm_mpls_circuit *vc;

    if (! NSM_MPLS)
      return NSM_SUCCESS;

    for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
      {
        vc = rn->info;
        if (! vc)
          continue;

        nsm_mpls_vc_cleanup(nm, vc);
      }

    return NSM_SUCCESS;
}

void
nsm_mpls_vc_owner_cleanup (struct nsm_master *nm, mpls_owner_t owner)
{
#if 0 /* TODO : MKD */
  struct ptree_node *pn;
  struct vc_fib_add_data *vc;
  struct prefix p;

  if (! NSM_MPLS)
    return;

  NSM_ASSERT ((owner != MPLS_OTHER) && (owner != MPLS_UNKNOWN));
  if ((owner != MPLS_LDP) && (owner != MPLS_SNMP) &&
      (owner != MPLS_POLICYAGENT) && (owner != MPLS_OTHER_CLI))
    return;

  if (! VC_FTN_TABLE)
    return;

  for (pn = ptree_top (VC_FTN_TABLE); pn; pn = ptree_next (pn))
    {
      vc = pn->info;
      if (! vc)
        continue;

      if (vc->owner.owner == owner)
        {

          UNSET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_SELECTED);
          if (FLAG_ISSET (vc->flags, NSM_MPLS_VC_FLAG_DEP))
            {
              gmpls_lsp_dep_del (nm, &vc->nh_addr, CONFIRM_VC, pn);
              UNSET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_DEP);
            }
                              
          pn->info = NULL;
          ptree_unlock_node (pn);
        }
    }
#endif
     
  return;
}

void
nsm_mpls_vc_fib_cleanup (struct nsm_master *nm,
                         struct nsm_mpls_circuit *vc,
                         s_int32_t del_vc_fib)
{
  struct fec_gen_entry fec;
  struct gmpls_gen_label lbl;

  if (!vc)
    {
      if (IS_NSM_DEBUG_EVENT)
        zlog_err (NSM_ZG, "%s-%d: vc not found \n",
                  __FUNCTION__, __LINE__);
      return;
    }

  if (vc->vc_fib)
    {
      if (vc->state >= NSM_MPLS_L2_CIRCUIT_COMPLETE)
        {
          UNSET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_SELECTED);
          if (FLAG_ISSET (vc->flags, NSM_MPLS_VC_FLAG_DEP))
            {
              fec.type = gmpls_entry_type_ip;
              fec.u.prefix = vc->address;

              gmpls_lsp_dep_del (nm, &fec, CONFIRM_VC, vc);
              UNSET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_DEP);
            }

          lbl.type = gmpls_entry_type_ip;
          lbl.u.pkt= vc->vc_fib->in_label;

          nsm_gmpls_ilm_fast_del_msg_process (nm, vc->vc_fib->nw_if_ix,
                                              &lbl, vc->ilm_ix);
          vc->ilm_ix = 0;
          vc->xc_ix = 0;
          vc->dn_in = 0;
        }

      if (del_vc_fib)
        {
          if (IS_NSM_DEBUG_EVENT)
            zlog_info (NSM_ZG, "%s-%d: Freeing vc_fib for vcid %d\n",
                       __FUNCTION__, __LINE__, vc->id);
          XFREE (MTYPE_NSM_VC_FIB, vc->vc_fib);
          vc->vc_fib = NULL;
        }
    }
  return;
}

/* Update new vc-fib Params */
void nsm_mpls_vc_fib_update(struct nsm_mpls_circuit *vc,
                            struct nsm_msg_vc_fib_add *vcfa)
{
    vc->vc_fib->in_label = vcfa->in_label;
    vc->vc_fib->out_label = vcfa->out_label;
    vc->vc_fib->nw_if_ix = vcfa->nw_if_ix;
#ifdef HAVE_MS_PW
    if (vc->ms_pw) 
      {
        if (vc->vc_other->vc_fib)
          {
            vc->vc_other->vc_fib->ac_if_ix = vcfa->nw_if_ix;
            vc->vc_fib->ac_if_ix = vc->vc_other->vc_fib->nw_if_ix;
            SET_FLAG (vc->ms_pw->ms_pw_flag, NSM_MPLS_MSPW_FIB_COMPLETE);
          }
        else
          {
            UNSET_FLAG (vc->ms_pw->ms_pw_flag, NSM_MPLS_MSPW_FIB_COMPLETE);
          }
      }
    else
#endif /* HAVE_MS_PW */
      {
        vc->vc_fib->ac_if_ix = vcfa->ac_if_ix;
      }
    vc->vc_fib->c_word = vcfa->c_word;
    vc->vc_fib->lsr_id = vcfa->lsr_id;
    vc->vc_fib->index = vcfa->index;
    vc->remote_grp_id = vcfa->remote_grp_id;
    vc->remote_pw_status = vcfa->remote_pw_status;
#ifdef HAVE_VCCV
    vc->vc_fib->remote_cc_types = vcfa->remote_cc_types;
    vc->vc_fib->remote_cv_types = vcfa->remote_cv_types;
#endif /* HAVE_VCCV */
    vc->vc_fib->rem_pw_status_cap = vcfa->rem_pw_status_cap;
}

s_int32_t
nsm_mpls_vc_fib_add_dep (struct nsm_master *nm, struct nsm_mpls_circuit *vc)
{
  s_int32_t ret;
  struct ilm_add_data iad;
  struct ilm_add_gen_data giad;
  struct ilm_gen_ret_data ird;
  struct fec_gen_entry fec;

  pal_mem_set (&iad, 0, sizeof (struct ilm_add_data));
  pal_mem_set (&giad, 0, sizeof (struct ilm_add_gen_data));
  pal_mem_set (&ird, 0, sizeof (struct ilm_gen_ret_data));
  pal_mem_set (&fec, 0, sizeof (struct fec_gen_entry));

  iad.owner.owner = MPLS_OTHER_LDP_VC;
  iad.owner.u.vc_key.vc_id = vc->id;
  IPV4_ADDR_COPY (&iad.owner.u.vc_key.vc_peer, &vc->address.u.prefix4);
  iad.iif_ix = vc->vc_fib->nw_if_ix;
  iad.oif_ix = vc->vc_fib->ac_if_ix;
  iad.in_label = vc->vc_fib->in_label;
  iad.out_label = LABEL_VALUE_INVALID;
  iad.n_pops = 1;
  iad.fec_prefix.family = AF_INET;
  iad.fec_prefix.prefixlen = IPV4_MAX_PREFIXLEN;
  iad.fec_prefix.u.prefix4.s_addr = pal_hton32 (vc->id);
  iad.opcode = POP_FOR_VC;
  iad.nh_addr.afi = AFI_IP;
  SET_FLAG (iad.flags, NSM_MSG_VC_ILM_ADD);
  /* dummy ilm entry creation for SNMP */
  
  nsm_gmpls_set_ilm_gen_data (&giad, &iad);

  ret = nsm_gmpls_ilm_add_msg_process (nm, &giad, NSM_FALSE, &ird, NSM_FALSE);
  if (ret == NSM_SUCCESS)
    {
      vc->xc_ix = ird.xc_ix;
      vc->dn_in = FROMPSN;
      vc->ilm_ix = ird.ilm_ix;
    }

  SET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_SELECTED);
  if ((if_is_running (vc->vc_info->mif->ifp))  
#ifdef HAVE_SNMP
      && (vc->admin_status != PW_SNMP_ADMIN_DOWN)
#endif /* HAVE_SNMP */
#ifdef HAVE_VLAN      
      && ((vc->vc_info->vlan_id == 0) ||
       (nsm_is_vlan_exist(vc->vc_info->mif->ifp,
        vc->vc_info->vlan_id) >= 0))
#endif /* HAVE_VLAN */
    )
    {
      fec.type = gmpls_entry_type_ip;
      fec.u.prefix = vc->address;
      gmpls_lsp_dep_add (nm, &fec, CONFIRM_VC, vc);
      SET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_DEP);
    }

  return NSM_SUCCESS;
}

s_int32_t
nsm_mpls_vc_fib_check_and_add (struct nsm_master *nm, 
                               struct nsm_mpls_circuit *vc)
{
  s_int32_t ret;

  if (vc->vc_fib && (vc->state == NSM_MPLS_L2_CIRCUIT_ACTIVE))
    {
      vc->state = NSM_MPLS_L2_CIRCUIT_COMPLETE;

      ret = nsm_mpls_vc_fib_add_dep (nm, vc);
      return ret;
    }

  return NSM_SUCCESS;
}

s_int32_t
nsm_mpls_remote_pw_status_update (struct nsm_master *nm, 
                                  struct nsm_msg_pw_status *pws)
{
  struct nsm_mpls_circuit *vc = NULL, *s_vc = NULL;
  struct nsm_mpls_vc_info *vc_info, *s_info = NULL;
  s_int32_t install_sibling = NSM_FALSE;
  s_int32_t ret;

  if (! pws)
    return NSM_ERR_INVALID_ARGS;

  vc = nsm_mpls_l2_circuit_lookup_by_id (nm, pws->vc_id);
  if (! vc)
    return NSM_ERR_INVALID_ARGS;

  if (vc->remote_pw_status == pws->pw_status)
    return NSM_SUCCESS;

  vc->remote_pw_status = pws->pw_status;
 
  /* Update those vc which local is ready */ 
  if (! vc->vc_fib || vc->state < NSM_MPLS_L2_CIRCUIT_UP)
    return NSM_SUCCESS;

  vc_info = vc->vc_info;

  if ((s_info = vc->vc_info->sibling_info) != NULL)
    s_vc = s_info->u.vc;

  /* primary or primary/primary configured Standby */
  if (CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_STANDBY))
    return NSM_SUCCESS;

  /* vc installed but not qualified any more, remove it from fib */
  if (vc->vc_fib->install_flag == NSM_TRUE && 
      ! PW_STATUS_INSTALL_CHECK (vc->pw_status, vc->remote_pw_status))
    {
      /* in case of primary/secondary pair, if sibling is qualified,
         install it */
      if (CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY) ||
          (s_vc && CHECK_FLAG (s_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY)))
        {
          if (nsm_mpls_vc_revert (s_vc, NSM_FALSE))
            {
              if (CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY) ||
                  ! CHECK_FLAG (vc_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE))
                {
                  vc_info->run_mode = NSM_MPLS_VC_STANDBY;
                  SET_FLAG (vc->pw_status, NSM_CODE_PW_STANDBY);
                }
              install_sibling = NSM_TRUE; 
            }
        }

      ret = nsm_mpls_vc_fib_del (nm, vc, vc->id);
      if (install_sibling)
        ret = nsm_mpls_vc_fib_add (nm, s_vc, s_vc->id, s_vc->vc_fib->opcode,
                                   s_vc->ftn);
      return NSM_SUCCESS;
    }

  /* vc not installed but qualified now, install this entry. */
  if (vc->vc_fib->install_flag != NSM_TRUE && 
      nsm_mpls_vc_revert (vc, NSM_FALSE))
    {
      ret = nsm_mpls_vc_fib_add (nm, vc, vc->id, vc->vc_fib->opcode,
                                 vc->ftn);
    }

  return NSM_SUCCESS; 
}

/* MPLS Virtual Circuit Fib Addtion process. Will make separate calls
 *  for adding into the MPLS Forwarder tables
 */
s_int32_t
nsm_mpls_vc_fib_add_msg_process (struct nsm_master *nm,
                                 struct nsm_msg_vc_fib_add *vcfa)
{
  bool_t changed = NSM_FALSE;
  struct nsm_mpls_circuit *vc = NULL;
  struct gmpls_gen_label lbl;
  struct fec_gen_entry fec;
#ifdef HAVE_MS_PW
  int ret = NSM_SUCCESS;
#endif /* HAVE_MS_PW */

  if (! vcfa)
    return NSM_ERR_INVALID_ARGS;

  /* Check if a VC with this id already exists. In such a case only
   * an update type operation is carried out
   */
  vc = nsm_mpls_l2_circuit_lookup_by_id (nm, vcfa->vc_id);
  if (! vc)
    return NSM_ERR_INVALID_ARGS;

  if (vc->vc_fib)
    {
      if (vc->state >= NSM_MPLS_L2_CIRCUIT_COMPLETE)
        {
          changed = NSM_FALSE;

          if ((vc->vc_fib->in_label != vcfa->in_label) ||
              (vc->vc_fib->out_label != vcfa->out_label) ||
              (vc->vc_fib->c_word != vcfa->c_word) ||
              (vc->vc_fib->nw_if_ix != vcfa->nw_if_ix) ||
              (vc->vc_fib->ac_if_ix != vcfa->ac_if_ix) ||
              (vc->vc_fib->lsr_id != vcfa->lsr_id))
            {
              changed = NSM_TRUE;
            }

          if (changed == NSM_FALSE)
            return NSM_SUCCESS;

          lbl.type = gmpls_entry_type_ip;
          lbl.u.pkt = vc->vc_fib->in_label;
          if (vc->ilm_ix > 0)
            nsm_gmpls_ilm_fast_del_msg_process(nm, vc->vc_fib->nw_if_ix,
                                               &lbl, vc->ilm_ix);

          vc->ilm_ix = 0;

          fec.type = gmpls_entry_type_ip;
          fec.u.prefix = vc->address;
          gmpls_lsp_dep_del (nm, &fec, CONFIRM_VC, vc);
          UNSET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_DEP);
          vc->state = NSM_MPLS_L2_CIRCUIT_ACTIVE;
        }

      pal_mem_set (vc->vc_fib, 0, sizeof (struct vc_fib));
    }
  else
    {
      /* Memory alloc. */
      vc->vc_fib = XCALLOC (MTYPE_NSM_VC_FIB, sizeof (struct vc_fib));
      if (! vc->vc_fib)
        return NSM_ERR_MEM_ALLOC_FAILURE;
    }

  nsm_mpls_vc_fib_update (vc, vcfa);
#ifdef HAVE_MS_PW
  if (vc->ms_pw) 
    {
      if (vc->fec_type_vc == PW_OWNER_MANUAL)
        {
          SET_FLAG (vc->ms_pw->ms_pw_flag, NSM_MPLS_MSPW_MANUAL_VC_FIB_READY);
          ret = nsm_mpls_ms_pw_stitch_send (nm, vc->ms_pw, NSM_TRUE);
          if (ret < 0)
            zlog_err (NSM_ZG, "%s - %d: Error in sending message to LDP\n",
                      __FUNCTION__, __LINE__);
        }

      if (CHECK_FLAG (vc->ms_pw->ms_pw_flag, NSM_MPLS_MSPW_FIB_COMPLETE))
        {
          /* The VC INFO might have changed, delete and readd */
          if (vc->vc_info)
            {
              /* Clean up the vc_info of the VC */
              ret = nsm_mpls_ms_pw_vc_info_cleanup (nm, vc);
              if (ret < 0)
                if (NSM_DEBUG_EVENT)
                  zlog_err (NSM_ZG, 
                            "%s - %d: vc_info_cleanup failed for vc id: %d\n",
                            __FUNCTION__, __LINE__, vc->id);
            }
          nsm_mpls_ms_pw_vc_info_create (nm, vc);

          if (vc->vc_other->vc_info)
            {
              ret = nsm_mpls_ms_pw_vc_info_cleanup (nm, vc->vc_other);
              if (ret < 0)
                if (NSM_DEBUG_EVENT)
                  zlog_err (NSM_ZG, 
                            "%s - %d: vc_info_cleanup failed for vc id: %d\n",
                            __FUNCTION__, __LINE__, vc->vc_other->id);
            }
          nsm_mpls_ms_pw_vc_info_create (nm, vc->vc_other);

          ret = nsm_mpls_vc_fib_check_and_add (nm, vc);
          if (ret < 0)
            {
              if (IS_NSM_DEBUG_EVENT)
                zlog_err (NSM_ZG, "%s-%d: nsm_mpls_vc_fib_check_and_add"
                          " failed for vcid %d\n", __FUNCTION__,
                          __LINE__, vc->id);
              return ret;
            }

          ret = nsm_mpls_vc_fib_check_and_add (nm, vc->vc_other);
          if (ret < 0)
            {
              if (IS_NSM_DEBUG_EVENT)
                zlog_err (NSM_ZG, "%s-%d: nsm_mpls_vc_fib_check_and_add"
                          " failed for vcid %d\n", __FUNCTION__,
                          __LINE__, vc->id);
              return ret;
            }
          return ret;
        }
      else
        {
          return NSM_SUCCESS;
        }
    }
#endif /* HAVE_MS_PW */
    return nsm_mpls_vc_fib_check_and_add (nm, vc);
}

/** @brief Function to manually switch between VC's.

    @param nm            - NSM master
    @param vc            - Primary VC
    @param svc           - Secondary VC
 */
int
nsm_mpls_pw_manual_switchover (struct nsm_master *nm,
                               struct nsm_mpls_circuit *vc,
                               struct nsm_mpls_circuit *svc)
{
  int ret = -1;

  if (nm == NULL || vc == NULL || svc == NULL)
    return NSM_ERR_INVALID_ARGS;

  /* Switchover only if secondary VC status is qualified. */
  if (nsm_mpls_vc_secondary_qualified (svc))
    {
      SET_FLAG (vc->pw_status, NSM_CODE_PW_STANDBY);
      nsm_mpls_vc_fib_del (nm, vc, vc->id);

      /* Sibling VC status and run mode are modified in fib addition. */
      ret = nsm_mpls_vc_fib_add (nm, svc, svc->id, svc->vc_fib->opcode,
                                 svc->ftn);
    }

  return ret;
}

#if 0
s_int32_t
nsm_mpls_vc_fib_add_msg_process (struct nsm_master *nm,
                                 struct nsm_msg_vc_fib_add *vcfa)
{
  bool_t changed = NSM_FALSE;
  struct nsm_mpls_circuit *vc *sibling_vc = NULL;
  struct ilm_add_data iad;
  struct ilm_ret_data ird;
  s_int32_t ret; 

  if (! vcfa)
    return NSM_ERR_INVALID_ARGS;

  /* Check if a VC with this id already exists. In such a case only
   * an update type operation is carried out
   */
  vc = nsm_mpls_l2_circuit_lookup_by_id (nm, vcfa->vc_id);
  if (! vc)
    return NSM_ERR_INVALID_ARGS;
  
  if (vc->vc_fib)
    {
      changed = NSM_FALSE;

      if (vc->vc_fib->in_label != vcfa->in_label)
        {
          vc->vc_fib->in_label = vcfa->in_label;
          changed = NSM_TRUE;
        }

      if(vc->vc_fib->out_label != vcfa->out_label)
        {
          vc->vc_fib->out_label = vcfa->out_label;
          changed = NSM_TRUE;
        }

      if (vc->vc_fib->c_word != vcfa->c_word)
        {
          vc->vc_fib->c_word = vcfa->c_word;
          changed = NSM_TRUE;
        }


      if (vc->vc_fib->nw_if_ix != vcfa->nw_if_ix)
        {
          vc->vc_fib->nw_if_ix = vcfa->nw_if_ix;
          changed = NSM_TRUE;
        }

      if (vc->vc_fib->ac_if_ix != vcfa->ac_if_ix)
        {
          vc->vc_fib->ac_if_ix = vcfa->ac_if_ix;
          changed = NSM_TRUE;
        }
      
      if (vc->vc_fib->lsr_id != vcfa->lsr_id)
        { 
          vc->vc_fib->lsr_id = vcfa->lsr_id;
          changed = NSM_TRUE;
        }

      vc->remote_grp_id = vcfa->remote_grp_id;
      if (changed == NSM_FALSE)
        return NSM_SUCCESS;

      gmpls_lsp_dep_del (nm, &vc->address, CONFIRM_VC, vc);
    }
  else
    {
      
      /* Memory alloc. */
      vc->vc_fib = XCALLOC (MTYPE_NSM_VC_FIB, sizeof (struct vc_fib));
      if (! vc->vc_fib)
        return NSM_ERR_MEM_ALLOC_FAILURE;

      vc->vc_fib->in_label = vcfa->in_label;
      vc->vc_fib->out_label = vcfa->out_label;
      vc->vc_fib->nw_if_ix = vcfa->nw_if_ix;
      vc->vc_fib->ac_if_ix = vcfa->ac_if_ix;
      vc->vc_fib->c_word = vcfa->c_word;
      vc->vc_fib->lsr_id = vcfa->lsr_id;
      vc->vc_fib->index = vcfa->index;
      vc->remote_grp_id = vcfa->remote_grp_id;
    }
  pal_mem_set (&iad, 0, sizeof (struct ilm_add_data));
  iad.owner.owner = MPLS_OTHER_LDP_VC;
  iad.owner.u.vc_key.vc_id = vcfa->vc_id;
  IPV4_ADDR_COPY (&iad.owner.u.vc_key.vc_peer, &vcfa->addr.u.ipv4);
  iad.iif_ix = vcfa->nw_if_ix;
  iad.oif_ix = vcfa->ac_if_ix;
  iad.in_label = vcfa->in_label;
  iad.out_label = LABEL_VALUE_INVALID;
  iad.n_pops = 1;
  iad.fec_prefix.family = AF_INET;
  iad.fec_prefix.prefixlen = IPV4_MAX_PREFIXLEN;
  iad.fec_prefix.u.prefix4.s_addr = pal_hton32 (vcfa->vc_id);
  iad.opcode = POP_FOR_VC;
  iad.nh_addr.afi = AFI_IP;
  SET_FLAG (iad.flags, NSM_MSG_VC_ILM_ADD);
/* dummy ilm entry creation for SNMP */
  ret = nsm_gmpls_ilm_add_msg_process (nm, &iad, NSM_FALSE, &ird, NSM_FALSE);
  if (ret == NSM_SUCCESS)
    {
      vc->xc_ix = ird.xc_ix;
      vc->dn_in = FROMPSN;
      vc->ilm_ix = ird.ilm_ix;
    }
  
  gmpls_lsp_dep_add (nm, &vc->address, CONFIRM_VC, vc);

  return NSM_SUCCESS;
}
#endif

/* MPLS Virtual Circuit FIB Delete function */
s_int32_t
nsm_mpls_vc_fib_del_msg_process (struct nsm_master *nm,
                                 struct nsm_msg_vc_fib_delete *vcfd)
{
  struct nsm_mpls_circuit *vc;
#ifdef HAVE_MS_PW
  int ret = NSM_SUCCESS;
#endif /* HAVE_MS_PW */

  if (! vcfd)
    return NSM_ERR_INVALID_ARGS;

  /* Check if a VC with this id already exists. */
  vc = nsm_mpls_l2_circuit_lookup_by_id (nm, vcfd->vc_id);
  if (! vc)
    return NSM_ERR_INVALID_ARGS;

  nsm_mpls_vc_fib_cleanup(nm, vc, NSM_TRUE);
  if (vc->state >= NSM_MPLS_L2_CIRCUIT_COMPLETE)
    vc->state = NSM_MPLS_L2_CIRCUIT_ACTIVE;

#ifdef HAVE_MS_PW
  if (vc->ms_pw)
    {
      /* If the vc is manual, then
       *   - UNSET NSM_MPLS_MSPW_MANUAL_VC_FIB_READY flag
       *   - send information to NSM clients
       */
      if (vc->fec_type_vc == PW_OWNER_MANUAL)
        {
          UNSET_FLAG (vc->ms_pw->ms_pw_flag, NSM_MPLS_MSPW_MANUAL_VC_FIB_READY);
          ret = nsm_mpls_ms_pw_stitch_send (nm, vc->ms_pw, NSM_FALSE);
          if (ret < 0)
            zlog_err (NSM_ZG, "%s - %d: Error in sending message to LDP\n",
                      __FUNCTION__, __LINE__);
        }

      /* If vc_other is manual, then
       *   - Do not UNSET NSM_MPLS_MSPW_MANUAL_VC_FIB_READY flag 
       *   - Do not delete the FIB in NSM, remove from harware and
       *     make vc as Inactive.
       */
      if ((vc->vc_other) && (vc->vc_other->fec_type_vc == PW_OWNER_MANUAL))
        {
          nsm_mpls_vc_fib_cleanup (nm, vc->vc_other, NSM_FALSE);
          if (vc->vc_other->state >= NSM_MPLS_L2_CIRCUIT_COMPLETE)
            vc->vc_other->state = NSM_MPLS_L2_CIRCUIT_ACTIVE;
        }
    }
#endif /* HAVE_MS_PW */
  return NSM_SUCCESS;
}
#endif /* HAVE_MPLS_VC */

void
nsm_gmpls_ftn_cleanup_all (struct nsm_master *nm, u_int32_t vrf_id,
                           bool_t del_flag)
{
  struct ptree_node *pn;
  struct ptree *pt;
  struct ptree_ix_table *ftn_ix_table, *nxt_ftn_ix_table;
  struct ftn_entry *ftn, *ftn_next;
  bool_t done = NSM_FALSE;
  struct fec_gen_entry fec;
  u_char family = AF_UNSPEC;
  bool_t first = PAL_TRUE;

  if (! NSM_MPLS)
    return;

  if (vrf_id != 0)
    {
#ifdef HAVE_PACKET
      family = AF_INET;
#else /* HAVE_PACKET */
      return;
#endif /* HAVE_PACKET */
    }
  GET_FIRST_FTN_IX_TABLE (ftn_ix_table);

  while (!done)
    {
      if (vrf_id != 0)
        {
          fec.type = gmpls_entry_type_ip;
          fec.u.prefix.family = AF_INET;
          ftn_ix_table = nsm_gmpls_ftn_table_lookup (nm, vrf_id, &fec);

          if ((! ftn_ix_table) || (! ftn_ix_table->m_table))
            return;

#ifdef HAVE_IPV6
      /* Clean the ix_mgr pointer as the memory has already been cleaned 
         for IPv4 ftn_ix_table.
         XXX: Both IPv4 and IPv6 ftn_ix_table have pointer to the same
         ix_mgr_memory
       */
          if (family == AF_INET6)
            {
              if (del_flag)
	        ftn_ix_table->ix_mgr = NULL;
            }
#endif /* HAVE_IPV6 */
        }

      pt = ftn_ix_table->m_table;
      for (pn = ptree_top (pt); pn; pn = ptree_next (pn))
        {
          if (! pn->info)
            continue;

          for (ftn = pn->info; ftn; ftn = ftn_next)
            {
              ftn_next = ftn->next;

              gmpls_ftn_entry_remove_list_node (nm, 
                                               (struct ftn_entry **)&pn->info,
                                               ftn, pn, NSM_FALSE);

              nsm_gmpls_set_fec_from_ftn (ftn, &fec); 
              gmpls_ftn_entry_cleanup (nm, ftn, &fec, ftn_ix_table);
              gmpls_ftn_free (ftn);
            }

          pn->info = NULL;
        }

      if (del_flag)
        {
          if (first == PAL_TRUE)
            { 
              PTREE_IX_TABLE_CLEANUP (ftn_ix_table);
              first = PAL_FALSE;
            }
          else
            {
              if (ftn_ix_table->m_table)
                {
                  ptree_finish (ftn_ix_table->m_table);
                  ftn_ix_table->m_table = NULL;
                }
            }
        }
  
      if (vrf_id != 0)
        {
#ifdef HAVE_IPV6
          if (family != AF_INET6)
            {
              family = AF_INET6; 
            }
          else
#endif
            done = NSM_TRUE;
        }
      else
        {
          GET_NEXT_FTN_IX_TABLE (ftn_ix_table, nxt_ftn_ix_table);
          pal_mem_set (ftn_ix_table, '\0', sizeof (struct ptree_ix_table));
          if (nxt_ftn_ix_table == NULL)
            {
              done = NSM_TRUE;
            }
          else
            {
              ftn_ix_table = nxt_ftn_ix_table;
            }
        }
    }  

  NSM_MPLS->trees [NSM_MPLS_TREE_ARRAY_FTN_TREES].cnt = 0;
  NSM_MPLS->trees [NSM_MPLS_TREE_ARRAY_FTN_IX_TREES].cnt = 0;
  NSM_MPLS->trees [NSM_MPLS_TREE_ARRAY_FTN6_TREES].cnt = 0;
  NSM_MPLS->trees [NSM_MPLS_TREE_ARRAY_FTN6_IX_TREES].cnt = 0;

  return;
}

void
nsm_gmpls_ftn_owner_stale_update (struct nsm_master *nm, mpls_owner_t owner, 
                                  s_int8_t flags, u_int32_t oi_index)
{
  struct ptree_ix_table *ftn_ix_table = NULL, *nxt_ftn_ix_table = NULL;
  struct ptree_node *pn = NULL;
  struct ftn_entry *ftn = NULL;
  mpls_lsp_priority_t owner_pri, ftn_pri;
  struct interface *ifp = NULL;
  u_int32_t idx;

  if (! NSM_MPLS)
    return;

  if ((owner == MPLS_OTHER) || (owner == MPLS_UNKNOWN))
    return;

  GET_FIRST_FTN_IX_TABLE (ftn_ix_table);
  while (ftn_ix_table != NULL)
    {
      if ((! ftn_ix_table) || (! ftn_ix_table->m_table))
        return;

      owner_pri = gmpls_owner_priority (owner);

      /* traverse the table and mark relevant entries */
      for (pn = ptree_top (ftn_ix_table->m_table); pn; pn = ptree_next (pn))
        {
          if (! pn->info)
            continue;

          for (ftn = pn->info; ftn; ftn = ftn->next)
            {
              ftn_pri = gmpls_owner_priority (ftn->owner.owner);
              if (ftn->owner.owner == owner)
                {
                  idx = gmpls_nhlfe_outgoing_if (ftn->xc->nhlfe);
                  NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ftn->flags, 
                                          FTN_ENTRY_FLAG_GMPLS),
                                          idx, ifp);
#ifdef HAVE_RESTART
                  if (ifp && (ifp->ifindex == oi_index))                     
                    {
                      if (CHECK_FLAG(flags, NSM_MARK_STALE_ENTRIES))
                        SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE);
                      else if(CHECK_FLAG (flags, 
                              NSM_UNMARK_STALE_ENTRIES))
                        UNSET_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE);
                    }
#endif /* HAVE_RESTART */
                }
              else if (ftn_pri > owner_pri)
                break;
            }
        }
      GET_NEXT_FTN_IX_TABLE (ftn_ix_table, nxt_ftn_ix_table);
      ftn_ix_table = nxt_ftn_ix_table;
    }
  return;
}

void
nsm_gmpls_ilm_owner_stale_update (struct nsm_master *nm, mpls_owner_t owner,
                                  s_int8_t flags, u_int32_t oi_index)
{
  struct avl_node *an = NULL;
  struct avl_tree *at = NULL, *nxt_at = NULL;
  struct ilm_entry *ilm = NULL;
  struct nhlfe_key *key = NULL;
  struct xc_entry *xc = NULL;
  struct interface *ifp = NULL;

  if (! NSM_MPLS)
    return;

  if ((owner == MPLS_OTHER) || (owner == MPLS_UNKNOWN))
    return;

  GET_FIRST_ILM_TABLE (at);

  while (at != NULL)
    {
      for (an = avl_top (at); an; an = avl_next (an))
        {
          if (! an->info)
            continue;

          for (ilm = (struct ilm_entry *)an->info;
               ilm; ilm = ilm->next)
            {
              if (ilm->owner.owner == owner)
                {
                  xc = ilm->xc;
                  if (xc)
                    { 
                      key = (struct nhlfe_key *) ilm->xc->nhlfe->nkey;
                      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ilm->flags, 
                                              ILM_ENTRY_FLAG_GMPLS),
                                              key->u.ipv4.oif_ix, ifp);
#ifdef HAVE_RESTART
                      if (ifp && (ifp->ifindex == oi_index))
                        {
                          if (CHECK_FLAG(flags, 
                              NSM_MARK_STALE_ENTRIES))
                            SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE);
                          else if (CHECK_FLAG (flags, 
                                   NSM_UNMARK_STALE_ENTRIES))
                            UNSET_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE);
                        }
#endif /* HAVE_RESTART */
                    }
                }
            }
        }

      GET_NEXT_ILM_TABLE (at, nxt_at);
      at = nxt_at;
    }

  return;
}

void
nsm_gmpls_ftn_owner_cleanup (struct nsm_master *nm, u_int32_t vrf_id,
                             mpls_owner_t owner, s_int32_t preserve,
                             s_int32_t stale_cleanup)
{
  struct ptree_ix_table *ftn_ix_table = NULL, *nxt_ftn_ix_table = NULL;
  struct ptree_node *pn;
  struct ftn_entry *ftn;
  struct ftn_entry *ftn_next = NULL;
  mpls_lsp_priority_t owner_pri, ftn_pri;
  struct fec_gen_entry fec;

  if (! NSM_MPLS)
    return;

  NSM_ASSERT ((owner != MPLS_OTHER) && (owner != MPLS_UNKNOWN));
  if ((owner == MPLS_OTHER) || (owner == MPLS_UNKNOWN))
    return;
  
  GET_FIRST_FTN_IX_TABLE (ftn_ix_table);
  while (ftn_ix_table != NULL)
    {
      if ((! ftn_ix_table) || (! ftn_ix_table->m_table))
        return;

      owner_pri = gmpls_owner_priority (owner);

      /* traverse the table and remove relevant entries */
      for (pn = ptree_top (ftn_ix_table->m_table); pn; pn = ptree_next (pn))
        {
          if (! pn->info)
            continue;
     
          for (ftn = pn->info; ftn; ftn = ftn_next) 
            {
              ftn_next = ftn->next;

              ftn_pri = gmpls_owner_priority (ftn->owner.owner);
              if (ftn->owner.owner == owner)
                {
                  if (preserve)
                    SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE);
                  else
                    {
                      if (stale_cleanup)
                        if (! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE))
                          continue;

                      gmpls_ftn_entry_remove_list_node (nm,
                                                (struct ftn_entry **)&pn->info,
                                                 ftn, pn, NSM_FALSE);
                      nsm_gmpls_set_fec_from_ftn (ftn, &fec);
                      gmpls_ftn_entry_cleanup (nm, ftn, &fec, ftn_ix_table);
                      gmpls_ftn_free (ftn);
                    }
                }
              else if (ftn_pri > owner_pri)
                {
                  ftn_next = ftn;
                  break;
                }
            }

          if (pn->info == NULL)
            {
              ptree_unlock_node (pn);
            }
          else if (! preserve)
            {
              gmpls_ftn_delete_select_update (nm, owner_pri, pn);
            }
        }

      GET_NEXT_FTN_IX_TABLE (ftn_ix_table, nxt_ftn_ix_table);
      ftn_ix_table = nxt_ftn_ix_table;
    }
 
  return;
}

#ifdef HAVE_VRF
void
nsm_mpls_vrf_cleanup_all (struct nsm_master *nm, bool_t del_flag)
{
  struct vrf_table *vhe, *vhe_next;
  s_int32_t ctr;
  s_int32_t ret;

  if (! NSM_MPLS)
    return;

  for (ctr = 0; ctr < VRF_HASH_SIZE; ctr++)
    {
      for (vhe = NSM_MPLS->vrf_hash[ctr]; vhe; vhe = vhe_next)
        {
          vhe_next = vhe->next;

          ret = apn_mpls_vrf_end (vhe->vrf_id);
          if (ret < 0)
            zlog_warn (NSM_ZG, "Could not delete VRF table with identifier %d "
                       "from the MPLS Forwarder", vhe->vrf_id);
          else if (IS_NSM_DEBUG_EVENT)
            zlog_info (NSM_ZG, "Successfully deleted VRF table with "
                       "identifier %d from the MPLS Forwarder", vhe->vrf_id);
          
          nsm_gmpls_ftn_cleanup_all (nm, vhe->vrf_id, del_flag);
          
          if (del_flag)
            nsm_mpls_vrf_table_free (vhe);
        }

      if (del_flag)
        NSM_MPLS->vrf_hash[ctr] = NULL;
    }

  return;
}

void
nsm_mpls_vrf_owner_cleanup (struct nsm_master *nm, mpls_owner_t owner,
                            s_int32_t preserve, s_int32_t stale_cleanup)
{
  struct vrf_table *vhe;
  s_int32_t ctr;

  NSM_ASSERT ((owner != MPLS_OTHER) && (owner != MPLS_UNKNOWN));
  if ((owner != MPLS_OTHER_BGP) && (owner != MPLS_SNMP) &&
      (owner != MPLS_POLICYAGENT) && (owner != MPLS_OTHER_CLI))
    return;   

  for (ctr = 0; ctr < VRF_HASH_SIZE; ctr++)
    for (vhe = NSM_MPLS->vrf_hash[ctr]; vhe; NEXTNODE (vhe))
      nsm_gmpls_ftn_owner_cleanup (nm, vhe->vrf_id, owner,
                                  preserve, stale_cleanup);

  return;
}
#endif /* HAVE_VRF */
#ifdef HAVE_RESTART
void
nsm_gmpls_ftn_owner_stale_mark (struct nsm_master *nm, u_int32_t vrf_id,
                                mpls_owner_t owner)
{
  struct ptree_node *pn;
  struct ftn_entry *ftn;
  struct ftn_entry *ftn_next = NULL;
  struct nhlfe_key *key;
  bool_t done = NSM_FALSE;
  struct ptree *pt, *nxt_pt;
  s_int32_t proto_id;
  u_int32_t restart_time;
  struct prefix nexthop_self;

  pal_mem_set (&nexthop_self, 0, sizeof (struct prefix));

  proto_id = gmpls_owner_to_proto (owner);
  restart_time = nsm_restart_time (proto_id);

  if (! NSM_MPLS)
    return;

  NSM_ASSERT ((owner != MPLS_OTHER) && (owner != MPLS_UNKNOWN));
  if ((owner == MPLS_OTHER) || (owner == MPLS_UNKNOWN))
    return;
  
  GET_FIRST_FTN_TABLE (pt);
  if (pt == NULL)
    return;

  while (!done)
    {
      /* traverse the table and mark relevant entries */
      for (pn = ptree_top (pt); pn; pn = ptree_next (pn))
        {
          if (! pn->info)
            continue;
     
          for (ftn = pn->info; ftn; ftn = ftn_next) 
            {
              ftn_next = ftn->next;

               key = (struct nhlfe_key *) ftn->xc->nhlfe->nkey;

              /* Locally generated routes should not be marked as stale */ 
              if (ftn->xc && ftn->xc->nhlfe && 
                  IPV4_ADDR_SAME (&key->u.ipv4.nh_addr, &nexthop_self))
                continue;

              if (ftn->owner.owner == owner)
                {
                  if (restart_time)
                    SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_STALE);
                } 
            }
        }  

      GET_NEXT_FTN_TABLE (pt, nxt_pt);
      if (nxt_pt == NULL)
        {
          done = NSM_TRUE;
        }
      else
        {
          pt = nxt_pt;
        }
    }
}

/* ILM mark entries as stale */
void
nsm_gmpls_ilm_entry_mark_tree_stale (struct avl_tree *tree, mpls_owner_t owner)
{
  struct avl_node *an;
  struct ilm_entry *ilm, *ilm_next, *ilm_prev;
  struct nhlfe_key *key, nexthop_self;
  u_int32_t restart_time;
  s_int32_t proto_id;

  pal_mem_set (&nexthop_self, 0, sizeof (struct pal_in4_addr));

  if (! tree)
    return;

  proto_id = gmpls_owner_to_proto (owner);
  restart_time = nsm_restart_time (proto_id);

  for (an = avl_top (tree); an; an = avl_next (an))
    {
      if (! an->info)
        continue;

      for (ilm = (struct ilm_entry *)an->info, ilm_prev = NULL;
           ilm; ilm = ilm_next)
        {
          ilm_next = ilm->next;

          key = (struct nhlfe_key *) ilm->xc->nhlfe->nkey;
          /* Locally generated routes should not be marked as stale */
          if (ilm->xc && ilm->xc->nhlfe && 
              !pal_mem_cmp (&key, &nexthop_self, sizeof (struct nhlfe_key)))
            continue;

          if (ilm->owner.owner == owner)
            {
              if (restart_time)
                SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE);
            }
          else
            ilm_prev = ilm;
        }
    }

  return;
}

void
nsm_gmpls_ilm_owner_stale_mark (struct nsm_master *nm, mpls_owner_t owner)
{
  if (! NSM_MPLS)
    return;

  NSM_ASSERT ((owner != MPLS_OTHER) && (owner != MPLS_UNKNOWN));
  if ((owner == MPLS_OTHER) || (owner == MPLS_UNKNOWN))
    return;
  
#ifdef HAVE_PACKET
  if (! ILM_TABLE)
    nsm_gmpls_ilm_entry_mark_tree_stale (ILM_TABLE, owner);
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  if (! ILM_PBB_TABLE)
    nsm_gmpls_ilm_entry_mark_tree_stale (ILM_PBB_TABLE, owner);
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
  if (! ILM_TDM_TABLE)
    nsm_gmpls_ilm_entry_mark_tree_stale (ILM_TDM_TABLE, owner);
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

  return;
}

void
nsm_gmpls_ilm_owner_stale_sync (struct nsm_master *nm, s_int32_t proto_id)
{
  struct avl_node *an = NULL;
  struct ilm_entry *ilm = NULL;
  struct ilm_entry *ilm_prev = NULL;
  struct ilm_entry *ilm_next = NULL;
  struct nsm_gen_msg_stale_info info;
  mpls_owner_t owner;
  s_int32_t ret = 0;
  struct avl_tree *at, *nxt_at;

  owner = gmpls_proto_to_owner (proto_id);
  if (! NSM_MPLS)
    return;

  NSM_ASSERT ((owner != MPLS_OTHER) && (owner != MPLS_UNKNOWN));
  if ((owner == MPLS_OTHER) || (owner == MPLS_UNKNOWN))
    return;

   /* Reset */
  pal_mem_set (&info, 0x0, sizeof (struct nsm_gen_msg_stale_info));
  GET_FIRST_ILM_TABLE (at);

  while (at != NULL)
    {
      for (an = avl_top (at); an; an = avl_next (an))
        {
          if (! an->info)
            continue;

          for (ilm = (struct ilm_entry *)an->info, ilm_prev = NULL;
              ilm; ilm = ilm_next)
            {
              ilm_next = ilm->next;

              if (ilm->owner.owner == owner && 
                  CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE) )
                {
                  ret = nsm_gmpls_ilm_owner_get_stale_info (ilm, &info);
                  if (0 == ret)
                    {
                      if (!CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_GMPLS))
                        {
#ifdef HAVE_GMPLS
                          info.iif_ix = nsm_gmpls_get_ifindex_from_gifindex (nm, 
                                                                 info.iif_ix);
                          info.oif_ix = nsm_gmpls_get_ifindex_from_gifindex (nm
                                                                 ,info.oif_ix);
#endif /* HAVE_GMPLS */
                        }
                      nsm_gmpls_send_stale_bundle_msg (&info);
                    }
                }
              else
                ilm_prev = ilm;
            }
        }

      GET_NEXT_ILM_TABLE (at, nxt_at);
      at = nxt_at;
    }

  return;
}

int nsm_gmpls_ilm_owner_get_stale_info (struct ilm_entry *ilm ,  
                                        struct nsm_gen_msg_stale_info *info)
{
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_key_gen *key = NULL;
  struct xc_entry *xc = NULL;
  struct prefix fec_prefix;

  if (!ilm || !info)
    return -1;

  nhlfe = NULL;

  pal_mem_set (info, 0x0, sizeof (struct nsm_msg_stale_info));

  xc = ilm->xc;
  if (xc)
    nhlfe = xc->nhlfe;

  fec_prefix.family = ilm->family;
  fec_prefix.prefixlen = ilm->prefixlen;

#ifdef HAVE_IPV6
  if (ilm->family == AF_INET6)
    pal_mem_cpy (&fec_prefix.u.prefix6, ilm->prfx,
                 sizeof (struct pal_in6_addr));
  else
#endif
    pal_mem_cpy (&fec_prefix.u.prefix4, ilm->prfx,
                 sizeof (struct pal_in4_addr));

  prefix_copy (&info->fec_prefix, &fec_prefix);

  if (nhlfe)
    key = (struct nhlfe_key_gen *) nhlfe->nkey;

  switch (ilm->ent_type)
    {
      case ILM_ENTRY_TYPE_PACKET:
        info->in_label.type = gmpls_entry_type_ip;
        info->in_label.u.pkt = ilm->key.u.pkt.in_label;
        info->iif_ix  = ilm->key.u.pkt.iif_ix;
        if (key)
          {
            if ((key->u.pkt.afi == AFI_IP)
#ifdef HAVE_GMPLS
                || (key->u.pkt.afi == AFI_UNNUMBERED)
#endif /* HAVE_GMPLS */
               )
              {
                info->out_label.type = gmpls_entry_type_ip;
                info->out_label.u.pkt = key->u.pkt.u.ipv4.out_label;
                info->oif_ix = key->u.pkt.u.ipv4.oif_ix;
              }
#ifdef HAVE_IPV6
             else
              {
                info->out_label.type = gmpls_entry_type_ip;
                info->out_label.u.pkt = key->u.pkt.u.ipv6.out_label;
                info->oif_ix = key->u.pkt.u.ipv4.oif_ix;
              }
#endif /* HAVE_IPV6 */
          }
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case ILM_ENTRY_TYPE_PBB_TE:
        info->in_label.type = gmpls_entry_type_pbb_te;
        info->in_label.u.pbb = ilm->key.u.pbb_key.in_label;
        info->iif_ix  = ilm->key.u.pbb_key.iif_ix;
        if (key)
          {
            info->out_label.type = gmpls_entry_type_ip;
            info->out_label.u.pbb = key->u.pbb.lbl;
            info->oif_ix = key->u.pbb.oif_ix;
          }
        break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case ILM_ENTRY_TYPE_TDM:
        info->in_label.type = gmpls_entry_type_tdm;
        info->in_label.u.tdm = ilm->key.u.tdm_key.in_label;
        info->iif_ix  = ilm->key.u.tdm_key.iif_ix;
        if (key)
          {
            info->out_label.type = gmpls_entry_type_ip;
            info->out_label.u.tdm = key->u.tdm.out_label;
            info->oif_ix = key->u.tdm.oif_ix;
          }

        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
      default:
        break;
    }

  info->ilm_ix = ilm->ilm_ix;

  return NSM_SUCCESS;
}
#endif /* HAVE_RESTART */

void 
nsm_mpls_ilm_primary_update (struct nsm_master *nm, struct ilm_entry *ilm)
{
  s_int32_t ret;
  struct fec_gen_entry fec;

  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_SELECTED))
    {
      fec.type = gmpls_entry_type_ip;

      if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_DEPENDENT))
        {
          if (gmpls_set_dep_ilm_nhprefix (&fec.u.prefix, ilm))
            gmpls_lsp_dep_del (nm, &fec, CONFIRM_RSVP_BYPASS_ILM, ilm);
        }
      else
        gmpls_ilm_del_entry_from_fwd_tbl (nm, ilm);

      if (ilm->next)
        {
          /* select new primary */
          SET_FLAG (ilm->next->flags, ILM_ENTRY_FLAG_SELECTED);

          /* add to fib */
          if (CHECK_FLAG (ilm->next->flags, ILM_ENTRY_FLAG_DEPENDENT))
            {
              if (gmpls_set_dep_ilm_nhprefix (&fec.u.prefix, ilm->next))
                gmpls_lsp_dep_add (nm, &fec, CONFIRM_RSVP_BYPASS_ILM, 
                                   ilm->next);
            }
          else
            ret = gmpls_ilm_add_entry_to_fwd_tbl (nm, ilm->next, NULL);
        }
    }

}

void
nsm_gmpls_ilm_cleanup_all (struct nsm_master *nm, bool_t del_flag)
{
  struct avl_node *an;
  struct avl_ix_table *at, *nxt_at;
  struct ilm_entry *ilm, *ilm_next;
  bool_t first = PAL_TRUE;

  if (! NSM_MPLS)
    return;

  GET_FIRST_ILM_IX_TABLE (at);
  
  while (at != NULL)
    {
      for (an = avl_top (at->m_table); an; an = avl_next (an))
        {
          if (! an->info)
            continue;
      
          for (ilm = (struct ilm_entry *)an->info; ilm; ilm = ilm_next)
            {
              ilm_next = ilm->next;
              gmpls_ilm_row_cleanup (nm, ilm, NSM_TRUE);
              gmpls_ilm_free (ilm);  
            }

          an->info = NULL;
        }

      if (del_flag)
        {
          if (first == PAL_TRUE)
            {
              AVL_IX_TABLE_CLEANUP (at);
              first = PAL_FALSE;
            }
          else
            {
              avl_tree_free (&at->m_table, NULL);
              at->m_table = NULL;
            }
        }

      GET_NEXT_ILM_IX_TABLE (at, nxt_at);
      pal_mem_set (at, '\0', sizeof (struct avl_ix_table));
      at = nxt_at;
    }
 
  NSM_MPLS->trees [NSM_MPLS_TREE_ARRAY_ILM_TREES].cnt = 0;
  NSM_MPLS->trees [NSM_MPLS_TREE_ARRAY_ILM_IX_TREES].cnt = 0;

  return;
}


void
nsm_gmpls_ilm_owner_cleanup (struct nsm_master *nm, mpls_owner_t owner,
                            s_int32_t preserve, s_int32_t stale_cleanup, 
                            bool_t force_del)
{
  struct avl_node *an;
  struct avl_node *an_next;
  struct avl_tree *at, *nxt_at;
  struct ilm_entry *ilm, *ilm_prev, *ilm_next;

  if (! NSM_MPLS)
    return;

  NSM_ASSERT ((owner != MPLS_OTHER) && (owner != MPLS_UNKNOWN));
  if ((owner == MPLS_OTHER) || (owner == MPLS_UNKNOWN))
    return;
  
  GET_FIRST_ILM_TABLE (at);

  while (at != NULL)
    {
      AVL_LIST_LOOP_DEL (at, ilm, an, an_next)
        {
          for (ilm_prev = NULL; ilm; ilm = ilm_next)
            {
              ilm_next = ilm->next;
              if (ilm->owner.owner == owner)
                {
                  if (preserve)
                    SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE);
                  else
                    {
#ifdef HAVE_RESTART                      
                      if (CHECK_FLAG(ilm->flags, ILM_ENTRY_FLAG_RETAIN_STALE))
                        UNSET_FLAG (ilm->flags, ILM_ENTRY_FLAG_RETAIN_STALE);
#endif /* HAVE_RESTART */

                      if (stale_cleanup)
                        if (! (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_STALE)))
                          continue;
  
#ifdef HAVE_RESTART
                      if (ilm->partial && ! force_del)
                        {
                          UNSET_FLAG (ilm->partial->flags, 
                                      NSM_MSG_ILM_STALE_DB_AVAILABLE);
                          gmpls_ilm_update (nm, ilm, ilm->partial);
                          continue;
                        }
                      else
#endif /* HAVE_RESTART */                      
                        /* update primary */
                        nsm_mpls_ilm_primary_update (nm, ilm);

                      if (! ilm_prev)
                        an->info = ilm_next;
                      else
                        ilm_prev->next = ilm_next;
                  
#ifdef HAVE_RESTART
                      if (ilm->partial)
                        {
                          XFREE (MTYPE_MPLS_ILM_ENTRY, ilm->partial);
                          ilm->partial = NULL;
                        }
#endif /* HAVE_RESTART */                  

                      gmpls_ilm_row_cleanup (nm, ilm, NSM_TRUE);
                      gmpls_ilm_free (ilm);
                    } 
                }
              else
                ilm_prev = ilm;
            }

          if (! an->info)
            {
              avl_delete_node (at, an);
            }
        }    

      GET_NEXT_ILM_TABLE (at, nxt_at);
      at = nxt_at;
    }

  return;
}

/* cleanup MPLS RIB data */
void
nsm_gmpls_rib_owner_stale_cleanup (struct nsm_master *nm, mpls_owner_t owner,
                                   s_int32_t preserve, s_int32_t stale_cleanup,
                                   bool_t ilm_force_del)
{
  bool_t ftn_cleanup = NSM_TRUE;
  
  if (! NSM_MPLS)
    return;

  NSM_ASSERT ((owner != MPLS_OTHER) && (owner != MPLS_UNKNOWN));

  if ((owner == MPLS_OTHER_CLI) ||
      (owner == MPLS_POLICYAGENT) ||
      (owner == MPLS_SNMP))
    {
#ifdef HAVE_VRF
      nsm_mpls_vrf_owner_cleanup (nm, owner, preserve, stale_cleanup);
#endif /* HAVE_VRF */
#ifdef HAVE_MPLS_VC
      nsm_mpls_vc_owner_cleanup (nm, owner);
#endif /* HAVE_MPLS_VC */
    }
  else if (owner == MPLS_OTHER_BGP)
    {
#ifdef HAVE_VRF
      nsm_mpls_vrf_owner_cleanup (nm, owner, preserve, stale_cleanup);
#endif /* HAVE_VRF */
    }
  else if (owner == MPLS_LDP)
    {
#ifdef HAVE_MPLS_VC
      nsm_mpls_vc_owner_cleanup (nm, owner);
#endif /* HAVE_MPLS_VC */
    }
  else if ((owner != MPLS_RSVP) && 
           (owner != MPLS_CRLDP)) 
    return;

  if (ftn_cleanup)
    nsm_gmpls_ftn_owner_cleanup (nm, GLOBAL_FTN_ID, owner,
                                preserve, stale_cleanup);
  
  nsm_gmpls_ilm_owner_cleanup (nm, owner, preserve, stale_cleanup, 
                               ilm_force_del);

  return;
}

#ifdef HAVE_MPLS_FWD
void
nsm_gmpls_nhlfe_stats_cleanup_all (struct nsm_master *nm)
{
  struct nhlfe_entry *nhlfe;
  struct avl_node *an;
  struct avl_tree *nh = NULL, *nxt_nh = NULL;

  GET_FIRST_NHLFE_TABLE (nh);
  while (!nh)
    {
      for (an = avl_top (nh); an; an = avl_next (an))
        if ((nhlfe = an->info) != NULL)
          pal_mem_set (&nhlfe->stats, 0, sizeof (pal_nhlfe_entry_stats_t));

      GET_NEXT_NHLFE_TABLE (nh, nxt_nh);
      nh = nxt_nh;
    }

  return;
}
#endif /* HAVE_MPLS_FWD */

void
nsm_mpls_nhlfe_cleanup_all (struct nsm_master *nm, bool_t flag)
{
  struct nhlfe_entry *nhlfe;
  struct avl_node *an = NULL;
  struct avl_node *an_next = NULL;
  struct avl_tree *nh, *nxt_nh;
  bool_t first = PAL_TRUE;

  GET_FIRST_NHLFE_TABLE (nh);
  while (nh != NULL)
    {
      for (an = avl_top (nh); an; an = an_next)
        {
          an_next = avl_next (an);
          if ((nhlfe = an->info) != NULL)
            {
              if (flag == NSM_TRUE)
                {
                  gmpls_nhlfe_free (nhlfe);
                  nhlfe = NULL;
                }
            }
        }

      if (flag == NSM_TRUE)
        {
          if (first == PAL_FALSE)
            {
              avl_tree_free (&nh, NULL);
              NHLFE_TABLE4 = NULL;
            }
          else
            {
              AVL_IX_TABLE_CLEANUP (&NSM_MPLS->nhlfe_ix_table4);
              first = PAL_FALSE;
            }
        }

      GET_NEXT_NHLFE_TABLE (nh, nxt_nh);
      nh = nxt_nh;
    }

  
  NSM_MPLS->trees [NSM_MPLS_TREE_ARRAY_NHLFE_TREES].cnt = 0;
  NSM_MPLS->trees [NSM_MPLS_TREE_ARRAY_NHLFE6_TREES].cnt = 0;
  return;
}

void
nsm_mpls_xc_cleanup_all (struct nsm_master *nm, bool_t flag)
{
  struct xc_entry *xc;
  struct avl_node *an;
  struct avl_node *an_next;

  if (! XC_TABLE)
    return;

   AVL_LIST_LOOP_DEL (XC_TABLE, xc, an, an_next)
     {
       gmpls_xc_row_cleanup (nm, xc, flag, NSM_FALSE);
       if (flag)
         {
	   gmpls_xc_free (xc);
	   xc = NULL;
	 }
     }

  if (flag == NSM_TRUE)
    {
      AVL_IX_TABLE_CLEANUP (&NSM_MPLS->xc_ix_table);
      pal_mem_set (&NSM_MPLS->xc_ix_table, '\0', sizeof (struct avl_ix_table));
    }

  return;
}

/* Helper function to dump row status. */
char *
nsm_mpls_dump_row_status (mpls_row_status_t status)
{
  switch (status)
    {
    case RS_ACTIVE:             return "Active";
    case RS_NOT_IN_SERVICE:     return "Not in service";
    case RS_NOT_READY:          return "Not ready";
    case RS_CREATE_GO:          return "Create Go";
    case RS_CREATE_WAIT:        return "Create Wait";
    case RS_DESTROY:            return "Destroy";
    default:                    return "N/A";
    }
}

/* Helper function to dump administrative status. */
char  *
nsm_mpls_dump_admn_status (mpls_admn_status_t status)
{
  switch (status)
    {
    case ADMN_UP:               return "Up";
    case ADMN_DOWN:             return "Down";
    case ADMN_TESTING:          return "Testing";
    default:                    return "N/A";
    }
}

/* Helper function to dump operational status. */
char  *
nsm_mpls_dump_opr_status (mpls_opr_status_t status)
{
  switch (status)
    {
    case OPR_UP:                return "Up";
    case OPR_DOWN:              return "Down";
    case OPR_TESTING:           return "Testing";
    case OPR_UNKNOWN:           return "Unknown";
    case OPR_DORMANT:           return "Dormant";
    case OPR_NOT_PRESENT:       return "Not present";
    case OPR_LL_DOWN:           return "Link Layer Down";
    default:                    return "N/A";
    }
}

#ifdef HAVE_DIFFSERV
void
nsm_mpls_update_diffserv_info (struct nsm_master *nm)
{
  struct avl_node *an;
  struct ptree_ix_table *ftn_ix_table;
  struct ftn_entry *ftn, *ftn_next, *ftn_ahead;
  struct ptree_node *pn;
  struct ilm_entry *ilm;
  bool_t done = NSM_FALSE;
  u_int8_t family = AF_INET;
  s_int32_t active_head;
  s_int32_t ret = NSM_SUCCESS;
  struct fec_gen_entry fec, tmp_fec;

  if (! NSM_MPLS)
    return;

  fec.type = gmpls_entry_type_ip;
  /* Update the ftn table. */
  while (!done)
    {
      fec.u.prefix.family = family;
      ftn_ix_table = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, &fec);
      if ((! ftn_ix_table) || (! ftn_ix_table->m_table))
        return;

      for (pn = ptree_top (ftn_ix_table->m_table); pn; pn = ptree_next (pn))
        {
          if (! pn->info)
            continue;
   
          ftn_ahead = gmpls_ftn_active_head ((struct ftn_entry *)pn->info); 
          for (ftn = pn->info; ftn; ftn = ftn_next)
            {
              ftn_next = ftn->next;

              if (ftn == ftn_ahead)
                active_head = NSM_TRUE;
              else
                active_head = NSM_FALSE;

              if (ftn->ds_info.lsp_type == ELSP_CONFIG)
                {
                  /* Update the entry. */
                  pal_mem_cpy (ftn->ds_info.dscp_exp_map, NSM_MPLS->dscp_exp_map,
                               sizeof(u_char) * DIFFSERV_MAX_DSCP_EXP_MAPPINGS);
              
                  /* If the entry has been installed, update the forwarder. */
                  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED))
                    {
                      nsm_gmpls_set_fec_from_ftn (ftn, &tmp_fec);
#ifdef HAVE_IPV6
                      if (tmp_fec.u.prefix.family == AF_INET6)
                        {
                          ret = nsm_mpls_ftn6_add_to_fwd (nm, ftn, 
                                                          &tmp_fec.u.prefix, 
                                                          pn->tree->id, 
                                                          NULL, active_head);
                          if (ret != NSM_SUCCESS && IS_NSM_DEBUG_EVENT)
                            zlog_warn (NSM_ZG, "MPLS update diffserv: Fwd"
                                               " update failed for %R", 
                                       tmp_fec.u.prefix.u.prefix6);
                          
                        }
                      else
#endif
                        {
                          ret = nsm_mpls_ftn4_add_to_fwd (nm, ftn, 
                                                          &tmp_fec.u.prefix, 
                                                          pn->tree->id, 
                                                          NULL, active_head);
                          if (ret != NSM_SUCCESS && IS_NSM_DEBUG_EVENT)
                            zlog_warn (NSM_ZG, "MPLS update diffserv: Fwd "
                                              "update failed for %r", 
                                       tmp_fec.u.prefix.u.prefix4);

                        }
                    }
                }
            }
        }

#ifdef HAVE_IPV6
      if (family != AF_INET6)
        family = AF_INET6;
      else
#endif
        done = NSM_TRUE;
    }

  /* Update the ILM table. */

  if (! ILM_TABLE)
    return;

  for (an = avl_top (ILM_TABLE); an; an = avl_next (an))
    {
      ilm = an->info;
      if (! ilm)
        continue;

      if (ilm->ds_info.lsp_type == ELSP_CONFIG)
        {
          /* Update the entry. */
          pal_mem_cpy (ilm->ds_info.dscp_exp_map, NSM_MPLS->dscp_exp_map,
                       sizeof(u_char) * DIFFSERV_MAX_DSCP_EXP_MAPPINGS);
          
          /* Update the forwarder. */
          if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_DEPENDENT))
            {
              tmp_fec.type = gmpls_entry_type_ip;
              if (gmpls_set_dep_ilm_nhprefix (&tmp_fec.u.prefix, ilm))
                gmpls_lsp_dep_add (nm, &tmp_fec, CONFIRM_RSVP_BYPASS_ILM, ilm);
            }
          else
            gmpls_ilm_add_entry_to_fwd_tbl (nm, ilm, NULL);
        } 
    }
}
#endif /* HAVE_DIFFSERV */

void
nsm_mpls_ftn_if_up_process (struct nsm_master *nm, u_int32_t index)
{
  struct ptree_ix_table *pt, *nxt_pt;
  struct ptree_node *pn;
  struct ftn_entry *ftn, *ftn_next;
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_key_gen *key;
  u_int32_t oif_ix = 0;
  struct fec_gen_entry fec;

  GET_FIRST_FTN_IX_TABLE (pt);
  while (pt != NULL)
    {
      for (pn = ptree_top (pt->m_table); pn; pn = ptree_next (pn))
        {
          if (! pn->info)
            continue;

          for (ftn = pn->info; ftn; ftn = ftn_next)
            {
              ftn_next = ftn->next;
              if (! CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INACTIVE))
                continue;

              nhlfe = ftn->xc->nhlfe;
              if (! nhlfe)
                continue;

              oif_ix = 0;
              key = (struct nhlfe_key_gen *) nhlfe->nkey;
              switch (nhlfe->type)
                {
                  case gmpls_entry_type_ip:
                    if (key->u.pkt.afi == AFI_IP)
                      oif_ix = key->u.pkt.u.ipv4.oif_ix;
#ifdef HAVE_IPV6
                    else if (key->u.pkt.afi == AFI_IP6)
                      oif_ix = key->u.pkt.u.ipv6.oif_ix;
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
                    else if (key->u.pkt.afi == AFI_UNNUMBERED)
                      oif_ix = key->u.pkt.u.unnum.oif_ix;
#endif /* HAVE_GMPLS */
                    break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
                  case gmpls_entry_type_pbb_te:
                    oif_ix = key->u.pbb.oif_ix;
                    break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
                  case gmpls_entry_type_tdm:
                    oif_ix = key->u.tdm.oif_ix;
                    break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
                   default:
                     break;
                 }

              if (oif_ix == 0 || index != oif_ix)
                continue;

              /* Found the ftn entry which need to be actived */
              ftn_list_delete_node ((struct ftn_entry **)&pn->info, ftn);
              UNSET_FLAG (ftn->flags, FTN_ENTRY_FLAG_INACTIVE);

              nsm_gmpls_set_fec_from_ftn (ftn, &fec);
              gmpls_ftn_add_reactivate (nm, pt, ftn, &fec);
           }
        }

      GET_NEXT_FTN_IX_TABLE (pt, nxt_pt);
      pt = nxt_pt;
    } 
}

void
nsm_mpls_ftn_if_down_process (struct nsm_master *nm, s_int32_t index)
{
  struct ptree_ix_table *pt, *nxt_pt;
  struct ptree_node *pn, *pn_tmp;
  struct ftn_entry *ftn;
  struct ftn_entry *ftn_next = NULL;
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_key_gen *key;
  struct fec_gen_entry fec;
  u_int32_t oif_ix = 0;
  s_int32_t ret;
#ifdef HAVE_GMPLS
  struct gmpls_if *gmif;
  struct datalink *dl;
  struct interface *ifp;
#endif /* HAVE_GMPLS */
  bool_t done = PAL_FALSE;

  if (! NSM_MPLS)
    return;

  GET_FIRST_FTN_IX_TABLE (pt);
  while (pt != NULL)
    {
      /* traverse the ftn table and remove affected entries */
      for (pn = ptree_top (pt->m_table); pn; pn = ptree_next (pn))
        {
          if (! pn->info)
            continue;
      
          for (ftn = pn->info; ftn; ftn = ftn_next)
            {
              ftn_next = ftn->next;
          
              nhlfe = ftn->xc->nhlfe;
              if (! nhlfe)
                continue;
          
              oif_ix = 0;
              key = (struct nhlfe_key_gen *) nhlfe->nkey;
              switch (nhlfe->type)
                {
                  case gmpls_entry_type_ip:
                    if (key->u.pkt.afi == AFI_IP)
                      oif_ix = key->u.pkt.u.ipv4.oif_ix;
#ifdef HAVE_IPV6
                    else if (key->u.pkt.afi == AFI_IP6)
                      oif_ix = key->u.pkt.u.ipv6.oif_ix;
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
                    else if (key->u.pkt.afi == AFI_UNNUMBERED)
                      oif_ix = key->u.pkt.u.unnum.oif_ix;
#endif /* HAVE_GMPLS */
                    break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
                  case gmpls_entry_type_pbb_te:
                    oif_ix = key->u.pbb.oif_ix;
                    break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
                  case gmpls_entry_type_tdm:
                    oif_ix = key->u.tdm.oif_ix;
                    break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
                   default:
                     break;
                 }

              NSM_ASSERT (oif_ix > 0);
              if (oif_ix == 0 || index != oif_ix)
                continue;

              if (ftn->ftn_type == MPLS_FTN_MAPPED
                  || ftn->ftn_type == MPLS_FTN_RSVP_MAPPED 
                  || ftn->ftn_type == MPLS_FTN_IGP_SHORTCUT)
                continue;

              gmpls_ftn_entry_remove_list_node (nm, 
                                               (struct ftn_entry **)&pn->info,
                                               ftn, pn, NSM_TRUE);

              nsm_gmpls_set_fec_from_ftn (ftn, &fec);
              if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_STATIC))
                {
                  /* Check if this is the case because of interface type change
                     From Data to non-data (control or MPLS )*/
#ifdef HAVE_GMPLS
                  if (ftn)
                    {
                      gmif = gmpls_if_get (nzg, nm->vr->id);
                      if (gmif == NULL)
                        {
                          gmpls_ftn_entry_cleanup (nm, ftn, &fec, pt);
                          gmpls_ftn_free (ftn);
                          done = PAL_TRUE;
                        }
                      else
                        {
                          /* Find using the oif_ix */
                          if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_GMPLS))
                            {
                              /* If entry type is GMPLS but interface is not */
                              dl = data_link_lookup_by_index (gmif, oif_ix);
                              if (((dl == NULL) || (dl->ifp == NULL)) ||
                                  ((dl->ifp->gmpls_type == 
                                                 PHY_PORT_TYPE_GMPLS_UNKNOWN)
                                    || (dl->ifp->gmpls_type == 
                                                 PHY_PORT_TYPE_GMPLS_CONTROL)))
                                {
                                  gmpls_ftn_entry_cleanup (nm, ftn, &fec, pt);
                                  gmpls_ftn_free (ftn);
                                  done = PAL_TRUE;
                                }
                            }
                          else
                            {
                              /* If entry type is not GMPLS but interface is */
                              ifp = if_lookup_by_gindex (gmif, oif_ix);
                              if (!ifp)
                                continue;
                              if ((ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_DATA)
                                  || (ifp->gmpls_type == 
                                            PHY_PORT_TYPE_GMPLS_DATA_CONTROL))
                                 {
                                   gmpls_ftn_entry_cleanup (nm, ftn, &fec, pt);
                                   gmpls_ftn_free (ftn);
                                   done = PAL_TRUE;
                                 }

                              /* If entry type is GMPLS but interface is not */
                            }
                        }
                    }
#endif /* HAVE_GMPLS */

                  if (done == PAL_FALSE)
                    {
                      gmpls_ftn_del_from_fwd (nm, ftn, &fec, pt->id, NULL);
                      SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_INACTIVE);

                      pn_tmp = gmpls_ftn_add (nm, pt, ftn, &fec, &ret); 
                      if (pn_tmp == NULL)
                        {
                          gmpls_ftn_entry_cleanup (nm, ftn, &fec, pt);
                          gmpls_ftn_free (ftn);
                        }
                      else
                        ftn->pn = pn_tmp;
                    }
                 }
              else
                {
                  gmpls_ftn_entry_cleanup (nm, ftn, &fec, pt);
                  gmpls_ftn_free (ftn);
                }
            }
      
          if (pn->info == NULL)
            ptree_unlock_node (pn);
        }

      GET_NEXT_FTN_IX_TABLE (pt, nxt_pt);
      pt = nxt_pt;
    }
}

void
nsm_mpls_ilm_if_up_process (struct nsm_master *nm, s_int32_t index)
{
  struct interface *ifp_in, *ifp_out;
  struct avl_node *an;
  struct avl_tree *at, *nxt_at;
  struct ilm_entry *ilm, *ilm_prev, *ilm_next, *ilm_head;
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_key_gen *key;
  u_int32_t oif_ix = 0, iif_ix = 0;
  u_int32_t min_label, max_label;
  s_int32_t ret = 0;

  if (! NSM_MPLS)
    return;
  
  GET_FIRST_ILM_TABLE (at);

  /* traverse the ilm table and remove affected entries */
  while (at != NULL)
    {
      for (an = avl_top (at); an; an = avl_next (an))
        {
          if (! an->info)
            continue;
     
          ilm_head = (struct ilm_entry *)an->info; 
          for (ilm = (struct ilm_entry *)an->info, ilm_prev = NULL;
               ilm; ilm = ilm_next)
            {
              ilm_next = ilm->next;
              if (ilm->next == ilm)
                break;

              if (ilm->xc)
                nhlfe = ilm->xc->nhlfe;
              else
                nhlfe = NULL;

              if (! nhlfe)
                {
                  ilm_prev = ilm;
                  continue;
                }

             oif_ix = 0;
             key = (struct nhlfe_key_gen *) nhlfe->nkey;
             switch (nhlfe->type)
               {
                 case gmpls_entry_type_ip:
                   iif_ix = ilm->key.u.pkt.iif_ix;
                   if (key->u.pkt.afi == AFI_IP)
                     oif_ix = key->u.pkt.u.ipv4.oif_ix;
#ifdef HAVE_IPV6
                   else if (key->u.pkt.afi == AFI_IP6)
                     oif_ix = key->u.pkt.u.ipv6.oif_ix;
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
                   else if (key->u.pkt.afi == AFI_UNNUMBERED)
                     oif_ix = key->u.pkt.u.unnum.oif_ix;
#endif /* HAVE_GMPLS */
                   break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
                 case gmpls_entry_type_pbb_te:
                   iif_ix = ilm->key.u.pbb_key.iif_ix;
                   oif_ix = key->u.pbb.oif_ix;
                   break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
                 case gmpls_entry_type_tdm:
                   iif_ix = ilm->key.u.tdm.iif_ix;
                   oif_ix = key->u.tdm.oif_ix;
                   break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
                  default:
                    break;
                }
        
              /* In case of VC binded inteface, continue */ 
              NSM_ASSERT (oif_ix > 0);
              NSM_MPLS_GET_INDEX_PTR (
                                 CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_GMPLS), 
                                 iif_ix, ifp_in);
#ifdef HAVE_GMPLS
              if (oif_ix == NSM_MPLS->loop_gindex)
                {
                  ifp_out = if_lookup_by_name (&nm->vr->ifm, "lo");
                }
              else
#endif /* HAVE_GMPLS */
                {
                  NSM_MPLS_GET_INDEX_PTR (
                               CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_GMPLS), 
                               oif_ix, ifp_out);
                }

              if (!ifp_in || !ifp_out)
                {
                  ilm_prev = ilm;
                  continue;
                }

              if (oif_ix == 0 || (index != oif_ix && 
                  index != iif_ix) ||
                nhlfe->opcode == PUSH_FOR_VC || nhlfe->opcode == POP_FOR_VC)
                {
                  ilm_prev = ilm;
                  continue;
                } 

              /* In case one of in/out interface is down, continue */
              if ((! NSM_MPLS_VALID_INTERFACE (ifp_in)) ||
                  (! NSM_MPLS_VALID_INTERFACE (ifp_out) &&
                   ! NSM_MPLS_PHP_ENTRY (ifp_out, nhlfe->opcode)))
                {
                  ilm_prev = ilm;
                  continue;
                }

              /* In case configured in_label is not in the static ilm 
              ilm label range, leave it as inactive. */
              if (ilm->ent_type == ILM_ENTRY_TYPE_PACKET)
                {
                  ret = nsm_gmpls_static_label_range_check (
                                                      ilm->key.u.pkt.in_label, 
                                                      ifp_in,
                                                      &min_label, &max_label);
                }
              if (ret < 0)
                {
                  zlog_info (NSM_ZG, "%% Incoming label should within range of "
                                     "%d - %d \n", min_label, max_label); 
                  continue;
                }

              /* remove ilm entry from list */
              gmpls_ilm_entry_lookup_list (nm, &ilm_head, ilm->ilm_ix, 
                                           NSM_TRUE, NSM_FALSE);

              UNSET_FLAG (ilm->flags, ILM_ENTRY_FLAG_INACTIVE);

              gmpls_ilm_add_reactivate (nm, an, &ilm_head, ilm);
              if (an)
                an->info = ilm_head;
            }
        }

      GET_NEXT_ILM_TABLE (at, nxt_at);
      at = nxt_at;
    }
}

void
nsm_mpls_ilm_if_down_process (struct nsm_master *nm, s_int32_t index)
{
  struct avl_node *an, *an_next;
  struct avl_tree *at, *nxt_at;
  struct ilm_entry *ilm, *ilm_prev, *ilm_next, *ilm_head;
  struct nhlfe_entry *nhlfe = NULL;
  struct nhlfe_key_gen *key;
  u_int32_t oif_ix = 0, iif_ix = 0;
  bool_t done = PAL_FALSE;
#ifdef HAVE_GMPLS
  struct gmpls_if *gmif;
  struct interface *ifp;
  struct datalink *dl;
#endif /* HAVE_GMPLS */

  if (! NSM_MPLS)
    return;

  GET_FIRST_ILM_TABLE (at);

  /* traverse the ilm table and remove affected entries */
  while (at != NULL)
    {
      AVL_LIST_LOOP_DEL (at, ilm, an, an_next)
        {
          for (ilm_prev = NULL; ilm; ilm = ilm_next)
            {
              ilm_next = ilm->next;

              if (ilm->xc)
                nhlfe = ilm->xc->nhlfe;
              else
                nhlfe = NULL;

              if (! nhlfe)
                {
                  ilm_prev = ilm;
                  continue;
                }

              oif_ix = 0;
              key = (struct nhlfe_key_gen *) nhlfe->nkey;
              switch (nhlfe->type)
                {
                  case gmpls_entry_type_ip:
                    iif_ix = ilm->key.u.pkt.iif_ix;

                    if (key->u.pkt.afi == AFI_IP)
                      oif_ix = key->u.pkt.u.ipv4.oif_ix;
#ifdef HAVE_IPV6
                    else if (key->u.pkt.afi == AFI_IP6)
                      oif_ix = key->u.pkt.u.ipv6.oif_ix;
#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
                    else if (key->u.pkt.afi == AFI_UNNUMBERED)
                      oif_ix = key->u.pkt.u.unnum.oif_ix;
#endif /* HAVE_GMPLS */
                    break;
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
                  case gmpls_entry_type_pbb_te:
                    iif_ix = ilm->key.u.pbb_key.iif_ix;
                    oif_ix = key->u.pbb.oif_ix;
                    break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
                  case gmpls_entry_type_tdm:
                    iif_ix = ilm->key.u.tdm.iif_ix;
                    oif_ix = key->u.tdm.oif_ix;
                    break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
                   default:
                     break;
                 }

              /* In case of VC binded inteface, do not clean ilm entry */ 
              NSM_ASSERT (oif_ix > 0);
              if (oif_ix == 0 || 
                  (index != oif_ix && index != iif_ix) ||
                   nhlfe->opcode == PUSH_FOR_VC || nhlfe->opcode == POP_FOR_VC)
                {
                  ilm_prev = ilm;
                  continue;
                }

              /* remove ilm entry from list and update primary entry */
              gmpls_ilm_entry_lookup_list (nm, (struct ilm_entry **)&an->info,
                                           ilm->ilm_ix, NSM_TRUE, NSM_TRUE);

              if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_STATIC))
                {
#ifdef HAVE_GMPLS
                  gmif = gmpls_if_get (nzg, nm->vr->id);
                  if (gmif == NULL)
                    {
                      done = PAL_TRUE;
                    }
                  else
                    {
                      if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_GMPLS))
                        {
                          /* In this case both the in and out interfaces should
                             have the interface type of data */
                          if (oif_ix != NSM_MPLS->loop_gindex)
                            {
                              dl = data_link_lookup_by_index (gmif, oif_ix);
                              if (((dl == NULL) || (dl->ifp == NULL)) ||
                                  ((dl->ifp->gmpls_type ==
                                                 PHY_PORT_TYPE_GMPLS_UNKNOWN)
                                   || (dl->ifp->gmpls_type ==
                                                 PHY_PORT_TYPE_GMPLS_CONTROL)))
                                {
                                  done = PAL_TRUE;
                                }
                              else
                                {
                                  dl = data_link_lookup_by_index (gmif, iif_ix);
                                  if (((dl == NULL) || (dl->ifp == NULL)) ||
                                      ((dl->ifp->gmpls_type ==
                                                  PHY_PORT_TYPE_GMPLS_UNKNOWN)
                                       || (dl->ifp->gmpls_type ==
                                                 PHY_PORT_TYPE_GMPLS_CONTROL)))
                                    {
                                      done = PAL_TRUE;
                                    }
                                }

                            }
                        }
                      else
                        {
                          /* If entry type is not GMPLS but interface is */
                          ifp = if_lookup_by_gindex (gmif, oif_ix);
                          
                          if ((ifp == NULL) ||
                              (ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_DATA)
                              || (ifp->gmpls_type ==
                                     PHY_PORT_TYPE_GMPLS_DATA_CONTROL))
                            {
                              done = PAL_TRUE;
                            }
                          else
                            {
                              ifp = if_lookup_by_gindex (gmif, iif_ix);
                              if ((ifp == NULL) ||
                                  (ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_DATA)
                                  || (ifp->gmpls_type ==
                                             PHY_PORT_TYPE_GMPLS_DATA_CONTROL))
                                {
                                  done = PAL_TRUE;
                                }
                            }
                        }
                    }
#endif /* HAVE_GMPLS */
                 
                  if (done == PAL_TRUE)
                    {
#ifdef HAVE_RESTART
                      if (ilm->partial)
                        {
                          XFREE (MTYPE_MPLS_ILM_ENTRY, ilm->partial);
                          ilm->partial = NULL;
                        }
#endif /* HAVE_RESTART */

                      if (avl_is_last_node (an))
                        {
                          avl_delete_node (at, an);
                          ilm->an = NULL;
                        }
                      else
                        {
                          avl_unlock_node (an);
                        }

                      /* cleanup references in other tables */
                      gmpls_ilm_row_cleanup (nm, ilm, NSM_TRUE);

                      /* release memory */
                      gmpls_ilm_free (ilm);
                    }
                  else
                    {
                      SET_FLAG (ilm->flags, ILM_ENTRY_FLAG_INACTIVE);

                      /* add it to head of the list */
                      ilm_head = (struct ilm_entry *)an->info;
                      ilm->next = ilm_head;
                      ilm_head = ilm;
                      an->info = ilm_head;
                    }
                }
              else 
                {
#ifdef HAVE_RESTART
                  if (ilm->partial)
                    {
                      XFREE (MTYPE_MPLS_ILM_ENTRY, ilm->partial);
                      ilm->partial = NULL;
                    }
#endif /* HAVE_RESTART */

                  if (avl_is_last_node (an))
                    {
                      avl_delete_node (at, an);
                      ilm->an = NULL;
                    }
                  else
                    {
                      avl_unlock_node (an);
                    }

                  /* cleanup references in other tables */
                  gmpls_ilm_row_cleanup (nm, ilm, NSM_TRUE);
 
                  /* release memory */
                  gmpls_ilm_free (ilm);
                }
            }
        }
      GET_NEXT_ILM_TABLE (at, nxt_at);
      at = nxt_at;
    }
}

void
nsm_mpls_rib_if_up_process (struct interface *ifp)
{
  struct nsm_master *nm = ifp->vr->proto;

  if (! NSM_MPLS)
    return;
#ifdef HAVE_GMPLS
  if (ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_UNKNOWN)
    {
      nsm_mpls_ftn_if_up_process (nm, ifp->gifindex);
      nsm_mpls_ilm_if_up_process (nm, ifp->gifindex);
    }
  else
    {
      /* If there is a dta link associated with this link do an UP process 
         for it too */
      if ((ifp->num_dl == 1) && (ifp->dlink.datalink != NULL))
        {
            nsm_mpls_ftn_if_up_process (nm, ifp->dlink.datalink->gifindex);
            nsm_mpls_ilm_if_up_process (nm, ifp->dlink.datalink->gifindex);
        }
    }
#else  /* HAVE_GMPLS */
  nsm_mpls_ftn_if_up_process (nm, ifp->ifindex);
  nsm_mpls_ilm_if_up_process (nm, ifp->ifindex);
#endif /* HAVE_GMPLS */

  return;
}

void
nsm_mpls_rib_if_down_process (struct interface *ifp)
{
  struct nsm_master *nm = ifp->vr->proto;

  if (! NSM_MPLS)
    return;

#ifdef HAVE_GMPLS
  if (ifp->gmpls_type == PHY_PORT_TYPE_GMPLS_UNKNOWN)
    {
      nsm_mpls_ftn_if_down_process (nm, ifp->gifindex);
      nsm_mpls_ilm_if_down_process (nm, ifp->gifindex);
    }
  else
    {
      if ((ifp->num_dl == 1) && (ifp->dlink.datalink != NULL))
        {
          nsm_mpls_ftn_if_down_process (nm, ifp->dlink.datalink->gifindex);
          nsm_mpls_ilm_if_down_process (nm, ifp->dlink.datalink->gifindex);
        }
    }

#else /* HAVE_GMPLS */
  nsm_mpls_ftn_if_down_process (nm, ifp->ifindex);
  nsm_mpls_ilm_if_down_process (nm, ifp->ifindex);
#endif /* HAVE_GMPLS */

  return;
}

void 
nsm_mpls_ftn_qos_resrc_del_process (struct nsm_master *nm,
                                    u_int32_t qos_resrc_id)
{
  struct ptree_ix_table *pt, *nxt_pt;
  struct ptree_node *pn;
  struct ftn_entry *ftn;
  struct ftn_entry *ftn_next = NULL;
  struct fec_gen_entry fec;

  if (! NSM_MPLS)
    return;

  GET_FIRST_FTN_IX_TABLE (pt);
  while (pt != NULL)
    {
      /* traverse the ftn table and remove affected entries */
      for (pn = ptree_top (pt->m_table); pn; pn = ptree_next (pn))
        {
          if (! pn->info)
            continue;
      
          for (ftn = pn->info; ftn; ftn = ftn_next)
            {
              ftn_next = ftn->next;

              if (ftn->qos_resrc_id != qos_resrc_id)
                continue;

              gmpls_ftn_entry_remove_list_node (nm, 
                                                (struct ftn_entry **)&pn->info,
                                                ftn, pn, NSM_TRUE);
            
              nsm_gmpls_set_fec_from_ftn (ftn, &fec);
              gmpls_ftn_entry_cleanup (nm, ftn, &fec, pt);
              gmpls_ftn_free (ftn);
            }
      
          if (pn->info == NULL)
            ptree_unlock_node (pn);
        }

      GET_NEXT_FTN_IX_TABLE (pt, nxt_pt);
      pt = nxt_pt;
    }
}

void 
nsm_mpls_ilm_qos_resrc_del_process (struct nsm_master *nm,
                                    u_int32_t qos_resrc_id)
{
  struct avl_tree *at, *nxt_at;
  struct avl_node *an;
  struct avl_node *an_next;
  struct ilm_entry *ilm, *ilm_prev, *ilm_next;

  if (! NSM_MPLS)
    return;
  
  GET_FIRST_ILM_TABLE (at);

  /* traverse the ilm table and remove affected entries */
  while (!at)
    {
      AVL_LIST_LOOP_DEL (at, ilm, an, an_next)
        {
          for (ilm_prev = NULL; ilm; ilm = ilm_next)
            {
              ilm_next = ilm->next;

              if (ilm->qos_resrc_id != qos_resrc_id)
                {
                  ilm_prev = ilm;
                  continue;
                }

              /* update primary */
              nsm_mpls_ilm_primary_update (nm, ilm);
          
              if (! ilm_prev)
                an->info = ilm_next;
              else
                ilm_prev->next = ilm_next;

              gmpls_ilm_row_cleanup (nm, ilm, NSM_TRUE);
              gmpls_ilm_free (ilm);
            }

          if (! an->info)
            {
              avl_delete_node (at, an);
            }
        }

      GET_NEXT_ILM_TABLE (at, nxt_at);
      at = nxt_at;
    }
}

void
nsm_mpls_rib_qos_rsrc_del_process (struct nsm_master *nm,
                                   u_int32_t qos_resrc_id)
{
  if (! NSM_MPLS)
    return;

  nsm_mpls_ftn_qos_resrc_del_process (nm, qos_resrc_id);
  nsm_mpls_ilm_qos_resrc_del_process (nm, qos_resrc_id);
  
  return; 
}

/* Lookup first FTN entry installed in the forwarder. */
struct ftn_entry *
nsm_gmpls_ftn_lookup_installed (struct nsm_master *nm,
                                u_int16_t vrf_id, struct prefix *pfx)
{
  struct ftn_entry *ftn;
  struct ptree_node *pn;
  struct fec_gen_entry fec;
  struct ptree_ix_table *table;
  
  NSM_ASSERT (pfx != NULL);
  if (! pfx)
    return NULL;
  
  fec.type = gmpls_entry_type_ip;
  fec.u.prefix = *pfx;

  /* Lookup table. */
  table = nsm_gmpls_ftn_table_lookup (nm, vrf_id, &fec);
  if (! table)
    return NULL;
  
  /* Lookup ftn list. */
  pn = gmpls_ftn_node_lookup (table->m_table, &fec);
  if (! pn)
    return NULL;
  
  for (ftn = pn->info; ftn; ftn = ftn->next)
    if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED))
      return ftn;
  
  return NULL;
}

/* Lookup first ILM entry installed in the forwarder. */
struct ilm_entry *
nsm_gmpls_ilm_lookup_installed (struct nsm_master *nm, u_int32_t ifindex, 
                                struct gmpls_gen_label *label)
{
  struct ilm_entry *ilm, *ilm_start;
                               
  /* Lookup ILM list. */
  ilm_start = gmpls_ilm_node_lookup (nm, ifindex, label);
  if (! ilm_start)
    return NULL;
  
  for (ilm = ilm_start; ilm; ilm = ilm->next)
    if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_INSTALLED))
      return ilm;
  
  return NULL;
}     

#ifdef HAVE_PACKET
struct ilm_entry *
nsm_mpls_ilm_lookup_installed (struct nsm_master *nm,
                               u_int32_t ifindex, u_int32_t label)
{
  struct gmpls_gen_label lbl;

  lbl.type = gmpls_entry_type_ip;
  lbl.u.pkt = label;

  return nsm_gmpls_ilm_lookup_installed (nm, ifindex, &lbl);
}
#endif /* HAVE_PACKET */

/* For the same tunnel-id,
   return tunnel-id : if found ftn as same as passed in is_primary
   return 0         : if no same ftn could be found
   return -1        : for error case
*/ 
s_int32_t
nsm_gmpls_static_ftn_tunnel_id_check (struct nsm_master *nm, s_int32_t id,
                                      s_int32_t vrf_id, bool_t is_primary)
{
  struct ptree_node *pn;
  struct ptree *pt, *nxt_pt;
  struct ftn_entry *ftn;
  bool_t primary;

  if (id <= 0)
    return -1;

  GET_FIRST_FTN_TABLE (pt);
  while (pt)
    {
      for (pn = ptree_top (pt); pn; pn = ptree_next (pn))
        {
          if (! pn->info)
            continue;
    
          for (ftn = pn->info; ftn; ftn = ftn->next)
            if (ftn->tun_id == id)
              {
                if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_PRIMARY))
                  primary = NSM_TRUE;
                else
                  primary = NSM_FALSE;

                if (primary == is_primary) 
                  {
                    ptree_unlock_node (pn);
                    return id;
                  }
              }
         }

      GET_NEXT_FTN_TABLE (pt, nxt_pt);
      pt = nxt_pt;
    }

  return 0;
}

/* Check if there is another ftn entry has the same tunnel-id, 
   return 1 : fec is ok (not conflict with other ftn with the same tunnel-id
   return 0 : fec is conflict with another ftn with the same tunnel-id
   return -1: error case
*/

s_int32_t
nsm_gmpls_static_ftn_fec_check (struct nsm_master *nm, u_int32_t id, 
                                s_int32_t vrf_id, bool_t is_primary,
                                struct fec_gen_entry *fec)
{
  struct ptree_ix_table *pt;
  struct ptree_node *pn;
  struct ftn_entry *ftn;
  bool_t primary;
  s_int32_t ret = 1;

  if (id <= 0)
    return -1;

  pt = nsm_gmpls_ftn_table_lookup (nm, vrf_id, fec);

  if (!pt)
    return -1;

  for (pn = ptree_top (pt->m_table); pn; pn = ptree_next (pn))
    {
      if (! pn->info)
        continue;
    
      for (ftn = pn->info; ftn; ftn = ftn->next)
        if (ftn->tun_id == id)
          {
            if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_PRIMARY))
              primary = NSM_TRUE;
            else
              primary = NSM_FALSE; 

            /* There is another ftn entry with the same tunnel-id */
            if (primary != is_primary)
              {
                if (!gmpls_fec_same (fec, pn))
                  ret = 0;
                else
                  ret = 1;
              }

            ptree_unlock_node (pn);
            return ret;
          }
    }

  return ret;
}

#ifdef HAVE_PACKET
s_int32_t
nsm_mpls_static_ftn_fec_check (struct nsm_master *nm, u_int32_t id,
                               s_int32_t vrf_id, bool_t is_primary,
                               struct prefix *fec)
{
  struct fec_gen_entry gfec;

  gfec.type = gmpls_entry_type_ip;
  gfec.u.prefix = *fec;

  return nsm_gmpls_static_ftn_fec_check (nm, id, vrf_id, is_primary, &gfec);
}
#endif /* HAVE_PACKET */

/* Remove stale marked entries from MPLS RIB.  
   This function is called when restart time is expired or 
   client send message stale remove message.  */
void
nsm_gmpls_restart_stale_remove (struct nsm_master *nm, s_int32_t proto_id,
                                bool_t ilm_force_del)
{
  mpls_owner_t owner;

  owner = gmpls_proto_to_owner (proto_id);

  nsm_gmpls_rib_owner_stale_cleanup (nm, owner, NSM_FALSE, NSM_TRUE,
                                     ilm_force_del);

  /* Clear restart information.  */
  /* For BGP, stop route preserve only after NSM RIB cleanup */
  if (owner != MPLS_OTHER_BGP)
    {
      nsm_restart_stop (proto_id);
      nsm_restart_option_unset (proto_id);
    }
}

void
nsm_gmpls_stale_entries_update (struct nsm_master *nm, s_int32_t proto_id,
                                s_int8_t stale_update_flags, 
                                u_int32_t oi_index)
{
  mpls_owner_t owner;

  owner = gmpls_proto_to_owner (proto_id);

  nsm_gmpls_ftn_owner_stale_update (nm, owner, stale_update_flags,
                                    oi_index);
  nsm_gmpls_ilm_owner_stale_update (nm, owner, stale_update_flags,
                                    oi_index);
}

#ifdef HAVE_VRF
/*
 * Function Name  : nsm_gmpls_restart_stale_remove_vrf ()
 * Input          : nsm_master structure, protocol id
 * Output         : None
 * Purpose        : This function remove all mpls vrf state when clent send
                    stale remove message or when the restart timer expired
*/

void
nsm_gmpls_restart_stale_remove_vrf (struct nsm_master *nm, s_int32_t proto_id)
{
  mpls_owner_t owner;

  owner = gmpls_proto_to_owner (proto_id);

  nsm_gmpls_rib_owner_stale_cleanup (nm, owner, NSM_FALSE, NSM_TRUE, 
                                     NSM_FALSE);
}
#endif /* HAVE_VRF */


#ifdef HAVE_RESTART
/* MPLS Forwarding state Hold time is expired.  */
s_int32_t
nsm_mpls_fwd_hold_time_expire (struct thread *t)
{
  struct nsm_restart *restart;
  struct nsm_master *nm;
  s_int32_t proto_id;

  /* Get restart structure and protocol id.  */
  restart = THREAD_ARG (t);
  proto_id = restart->proto_id;

  /* Clear timer pointer.  */
  ng->restart[proto_id].t_preserve = NULL;
  ng->restart[proto_id].preserve_time = 0;

  /* Logging.  */
  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Timer (preserve timer expire for proto_id %d)",
               proto_id);

  nm = nsm_master_lookup_by_id (nzg, 0);
  if (nm != NULL)
    nsm_gmpls_restart_stale_remove (nm, proto_id, NSM_FALSE);

  nsm_restart_state_unset (proto_id);

  return 0;
}


#ifdef HAVE_VRF
/*
 * Function Name : nsm_mpls_vrf_restart_stale_mark ()
 * Input         : nsm_master struct, owner 
 * Output        : None
 * Purpose       : Mark all the mpls vrf entries as stale . This function will
                   be called when client send stale mark message.
*/
void
nsm_mpls_vrf_restart_stale_mark (struct nsm_master *nm, s_int32_t proto_id)
{
  struct vrf_table *vhe;
  s_int32_t ctr;
  u_int32_t restart_time;
  mpls_owner_t owner;

  restart_time = nsm_restart_time (proto_id);
  owner = gmpls_proto_to_owner (proto_id);

  NSM_ASSERT ((owner != MPLS_OTHER) && (owner != MPLS_UNKNOWN));
  if ((owner != MPLS_OTHER_BGP) && (owner != MPLS_SNMP) &&
      (owner != MPLS_POLICYAGENT) && (owner != MPLS_OTHER_CLI))
    return;

  if (owner == MPLS_OTHER_BGP)
    {
      for (ctr = 0; ctr < VRF_HASH_SIZE; ctr++)
         for (vhe = NSM_MPLS->vrf_hash[ctr]; vhe; NEXTNODE (vhe))
            nsm_gmpls_ftn_owner_stale_mark (nm, vhe->vrf_id, owner);
    }
  nsm_gmpls_ilm_owner_stale_mark (nm, owner);
  return;
}
#endif /* HAVE_VRF */
#endif /* HAVE_RESTART */

/* Clean up all RIB by client_id.  When restart is enabled, we mark
   the route by stale flag.  */
void
nsm_gmpls_rib_clean_client (struct nsm_master *nm, mpls_owner_t owner,
                           s_int32_t proto_id)
{
#ifdef HAVE_RESTART
  u_int32_t restart_time;

  restart_time = nsm_restart_time (proto_id);
  nsm_gmpls_rib_owner_stale_cleanup (nm, owner, restart_time, NSM_FALSE,
                                     NSM_FALSE);

#ifdef HAVE_VRF
  owner = gmpls_proto_to_owner (proto_id);
#endif /* HAVE_VRF */

  /* Turn on the preserve timer if restart_time is set. */
#ifdef HAVE_VRF
  /* Turn on the preserve timer after NSM RIB stale mark. Since only one timer,
     start the timer only after NSM RIB clean. Timer will be handled in 
     nsm_rib_clean_client  */
  if (owner != MPLS_OTHER_BGP)
    {
#endif /* HAVE_VRF */
      if (restart_time && ! ng->restart[proto_id].t_preserve)
        {
          ng->restart[proto_id].t_preserve
          = thread_add_timer (nzg, nsm_mpls_fwd_hold_time_expire,
                              &ng->restart[proto_id], restart_time);
          ng->restart[proto_id].preserve_time = restart_time;
        }
#ifdef HAVE_VRF
    }
#endif /* HAVE_VRF */

  /* Clear restart time.  */
  /* If Proto is BGP, NSM RIB also needs to preserve. clear it after NSM RIB clean */
#ifdef HAVE_VRF 
  if (owner != MPLS_OTHER_BGP)
    { 
#endif /* HAVE_VRF */
       ng->restart[proto_id].restart_time = 0;
       ng->restart[proto_id].disconnect_time = pal_time_current (NULL);
#ifdef HAVE_VRF
    }
#endif /* HAVE_VRF */
#else
  nsm_gmpls_rib_owner_stale_cleanup (nm, owner, 0, NSM_FALSE, NSM_FALSE);
#endif /* HAVE_RESTART */
}

/* FTN lookups for Specific cases */
struct ftn_entry *
nsm_gmpls_get_ldp_ftn (struct nsm_master *nm, struct prefix *pfx)
{
  struct ptree_ix_table *pt = NULL;
  struct ptree_node *pn_ftn = NULL;
  struct ftn_entry *ftn;
  struct fec_gen_entry fec;

  fec.type = gmpls_entry_type_ip;
  fec.u.prefix = *pfx; 
  
  pt = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, &fec);
 
  if (pt) 
    pn_ftn = gmpls_ftn_node_lookup (pt->m_table, &fec);
  
  if (! pn_ftn || ! pn_ftn->info)
    return NULL;
 
  for (ftn = pn_ftn->info; ftn; ftn = ftn->next)
     {
       if ((ftn->owner.owner == MPLS_LDP) &&
           (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED)))
         return ftn;
     }

  return NULL;
}

struct ftn_entry *
nsm_gmpls_get_rsvp_ftn (struct nsm_master *nm, struct fec_gen_entry *fec)
{
  struct ptree_ix_table *pt = NULL;
  struct ptree_node *pn_ftn = NULL;
  struct ftn_entry *ftn;

  pt = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, fec);

  if (pt)
    pn_ftn = gmpls_ftn_node_lookup (pt->m_table, fec);

  if (! pn_ftn || ! pn_ftn->info)
    return NULL;

  for (ftn = pn_ftn->info; ftn; ftn = ftn->next)
     {
       if ((ftn->owner.owner == MPLS_RSVP) &&
           (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED)))
         return ftn;
     }

  return NULL;
}

#ifdef HAVE_PACKET
struct ftn_entry *
nsm_gmpls_get_rsvp_ftn_pfx (struct nsm_master *nm, struct prefix *pfx)
{
  struct fec_gen_entry fec;

  fec.type = gmpls_entry_type_ip;
  fec.u.prefix = *pfx; 
  return nsm_gmpls_get_rsvp_ftn (nm, &fec);
}

#endif /* nsm_gmpls_get_rsvp_ftn_pfx */

struct ftn_entry *
nsm_gmpls_get_rsvp_ftn_by_tunnel (struct nsm_master *nm, u_char *lookup,
                                 u_char type)
{
  struct ftn_entry *ftn;
  struct ptree_node *pn_ftn;
  struct ptree *pt, *nxt_pt;
  u_int32_t *tunnel_id = NULL;
  u_char *tunnel_name = NULL;

  if (type == RSVP_TUNNEL_NAME)
    tunnel_name = lookup;
  else if (type == RSVP_TUNNEL_ID)
    tunnel_id = (u_int32_t *)lookup;
  
  GET_FIRST_FTN_TABLE (pt);
  while (pt)
    {
      /* Loop into the FTN table to identify the required ftn entry for use 
         with this VC*/
      if ((tunnel_name) || (tunnel_id))
        {
          for (pn_ftn = ptree_top (pt); pn_ftn ; pn_ftn = ptree_next (pn_ftn))
             {
               for (ftn = pn_ftn->info; ftn; ftn = ftn->next)
                  {
                    if ((ftn->owner.owner == MPLS_RSVP) &&
                       (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED)) &&
                        ftn->sz_desc)
                      {
                        /* Tunnel name should be good enough. Tunnel id check 
                           is added as a safety we do not also need to check 
                           direction here. */
                        if (((tunnel_name) && (pal_strcmp (ftn->sz_desc, 
                             tunnel_name) == 0 ))|| ((tunnel_id != NULL) && 
                             (ftn->tun_id == *tunnel_id)))
                          return ftn;                  
                      }
                  }
             }

          if (!pn_ftn)
            return NULL;
        }

      GET_NEXT_FTN_TABLE (pt, nxt_pt);
      pt = nxt_pt;
    }

  return NULL;
}

struct ftn_entry *
nsm_gmpls_get_static_ftn (struct nsm_master *nm, struct fec_gen_entry *fec)
{
  struct ptree_ix_table *pt = NULL;
  struct ptree_node *pn_ftn = NULL;
  struct ftn_entry *ftn;

  pt = nsm_gmpls_ftn_table_lookup (nm, GLOBAL_FTN_ID, fec);

  if (pt)
    pn_ftn = gmpls_ftn_node_lookup (pt->m_table, fec);

  if (! pn_ftn || ! pn_ftn->info)
    return NULL;

  for (ftn = pn_ftn->info; ftn; ftn = ftn->next)
     {
       if (((ftn->owner.owner == MPLS_OTHER_CLI)||
           (ftn->owner.owner == MPLS_SNMP)||
           (ftn->owner.owner == MPLS_POLICYAGENT)) &&
           (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED)))
         return ftn;
     }

  return NULL;
}

#ifdef HAVE_PACKET
struct ftn_entry *
nsm_gmpls_get_static_ftn_pfx (struct nsm_master *nm, struct prefix *pfx)
{
  struct fec_gen_entry fec;

  fec.type = gmpls_entry_type_ip;
  fec.u.prefix = *pfx;
  return nsm_gmpls_get_static_ftn (nm, &fec);
}
#endif /* HAVE_PACKET */


s_int32_t
nsm_gmpls_ilm_entry_add_sub (struct nsm_master *nm, u_int32_t vc_id,
                             struct gmpls_gen_label *in_label, char *if_in,
                             struct gmpls_gen_label *out_label, char *if_out,
                             struct pal_in4_addr *nhop, u_char opcode,
                             struct fec_gen_entry* fec, u_int32_t ilm_ix,
                             u_int32_t flags, u_int32_t rev_idx)
{
  u_int32_t min_label, max_label;
  struct interface *ifp_in, *ifp_out;
  s_int32_t in_idx = 0, out_idx = 0;
  struct pal_in4_addr loopback;
  struct ilm_add_gen_data iad;
  struct ilm_gen_ret_data rd;
  struct nsm_mpls_circuit *vc = NULL;
  s_int32_t ret = 0;
  bool_t b_loopback = NSM_FALSE;
#ifdef HAVE_GMPLS
  struct datalink *dl;
  bool_t b_gmpls_add = NSM_TRUE;
  bool_t is_gmpls = PAL_FALSE, is_bidir = PAL_FALSE;
#endif /* HAVE_GMPLS */

  /* Incoming interface. */
  ifp_in = if_lookup_by_name (&nm->vr->ifm, if_in);
  if (!ifp_in || if_is_loopback (ifp_in))
    {
      zlog_err (NSM_ZG, "%% Incoming interface %s is not found or is"
                        "loopback\n", if_in);
      return NSM_ERR_IF_NOT_FOUND;
    }

  /* Outgoing interface. */
  if (pal_strncmp (if_out, "loopback", pal_strlen (if_out)) == 0)
    {
      loopback.s_addr = pal_hton32 (INADDR_LOOPBACK);
      ifp_out = if_lookup_by_ipv4_address (&nm->vr->ifm, &loopback);
      b_loopback = NSM_TRUE;
    }
  else
    {
      ifp_out = if_lookup_by_name (&nm->vr->ifm, if_out);
    }

  if (!ifp_out)
    {
      zlog_err (NSM_ZG, "%% Outgoing interface %s is not found \n", if_out);
      return NSM_ERR_IF_NOT_FOUND;
    }

#ifdef HAVE_GMPLS
  if ((ifp_in->gmpls_type != PHY_PORT_TYPE_GMPLS_UNKNOWN) &&
      (ifp_in->gmpls_type != PHY_PORT_TYPE_GMPLS_CONTROL))
    {
      is_gmpls = PAL_TRUE;
    }

  else if (CHECK_FLAG (flags, NSM_MPLS_CLI_ILM_MPLS_CONFIG))
    {
      if (ifp_in->gmpls_type == PHY_PORT_TYPE_GMPLS_UNKNOWN)
        b_gmpls_add = NSM_FALSE;
    }

  if ((CHECK_FLAG (flags, NSM_MPLS_CLI_ILM_BIDIR_CONFIG)) &&
      (CHECK_FLAG (flags, NSM_MPLS_CLI_ILM_BIDIR_ENABLED)))
    {
      is_gmpls = PAL_TRUE;
      is_bidir = PAL_TRUE;
    }

  /* Check to see ILM entry is configured correctly for 
     MPLS on mpls enabled interfaces and
     GMPLS on gmpls enabled interfaces*/
  if (!is_bidir)
    {
      if ((ifp_out->gmpls_type != PHY_PORT_TYPE_GMPLS_UNKNOWN)
        &&  CHECK_FLAG (flags, NSM_MPLS_CLI_ILM_MPLS_CONFIG))
       {
         zlog_err (NSM_ZG, "%% ILM set for GMPLS enabled interfaces: %s\n", 
                   if_in);
         return NSM_ERR_IF_GMPLS_ENABLED;
       }
      else if ((ifp_out->gmpls_type == PHY_PORT_TYPE_GMPLS_UNKNOWN)
           && !CHECK_FLAG (flags, NSM_MPLS_CLI_ILM_MPLS_CONFIG)
           && opcode != POP)
        {
          zlog_err (NSM_ZG, "%% ILM set for MPLS enabled interfaces: %s\n", 
                    if_in);
          return NSM_ERR_IF_MPLS_ENABLED;
        }

     if ((ifp_in->gmpls_type != PHY_PORT_TYPE_GMPLS_UNKNOWN)
       && CHECK_FLAG (flags, NSM_MPLS_CLI_ILM_MPLS_CONFIG))
       {
         zlog_err (NSM_ZG, "%% ILM set for GMPLS enabled interfaces: %s\n", 
                   if_in);
         return NSM_ERR_IF_GMPLS_ENABLED;
       }
     else if ((ifp_in->gmpls_type == PHY_PORT_TYPE_GMPLS_UNKNOWN)
        && !CHECK_FLAG (flags, NSM_MPLS_CLI_ILM_MPLS_CONFIG))
       {
         zlog_err (NSM_ZG, "%% ILM set for MPLS enabled interfaces: %s\n", 
                   if_in);
         return NSM_ERR_IF_MPLS_ENABLED;
       }
    }
      
  if (ifp_in->gmpls_type == PHY_PORT_TYPE_GMPLS_UNKNOWN)
    {
      b_gmpls_add = NSM_FALSE;
    }
  else if (ifp_in->gmpls_type == PHY_PORT_TYPE_GMPLS_CONTROL)
    {
      zlog_err (NSM_ZG, "%% Invalid arguments \n");
      return NSM_ERR_INVALID_ARGS;
    }

  if (is_gmpls)
    {
      if ((ifp_in->gmpls_type == PHY_PORT_TYPE_GMPLS_UNKNOWN)
          || (ifp_in->gmpls_type == PHY_PORT_TYPE_GMPLS_CONTROL))
        return NSM_ERR_INVALID_ARGS;

      dl = (ifp_in->num_dl == 1)  ? ifp_in->dlink.datalink : NULL;
      if (! dl)
        {
          zlog_err (NSM_ZG, "%% Internal Error \n");
          return NSM_ERR_INTERNAL;
        }

      in_idx = dl->gifindex;
    }
  else
    {
      in_idx = ifp_in->gifindex;
    }

  if (! b_loopback)
    {
      if ((ifp_out->gmpls_type == PHY_PORT_TYPE_GMPLS_UNKNOWN && is_gmpls)
          || (ifp_out->gmpls_type == PHY_PORT_TYPE_GMPLS_CONTROL))
        return NSM_ERR_INVALID_ARGS;

      if (is_gmpls)
        {
          dl = (ifp_out->num_dl == 1) ? ifp_out->dlink.datalink : NULL;
          if (! dl)
            return NSM_ERR_INTERNAL;
          
          out_idx = dl->gifindex;
        }
      else
        {
          out_idx = ifp_out->gifindex;
        }
    }
  else
    {
      out_idx = NSM_MPLS->loop_gindex;
    }
#else
  in_idx = ifp_in->ifindex;
  out_idx = ifp_out->ifindex;
#endif /* HAVE_GMPLS */

#ifdef HAVE_MPLS_VC
  vc = nsm_mpls_l2_circuit_lookup_by_id (nm, vc_id);
#endif /* HAVE_MPLS_VC */

  if (vc_id > 0 && ! vc)
    {
      zlog_err (NSM_ZG, "%% Invalid Virtual Circuit Identifier\n");
      return NSM_ERR_VC_ID_NOT_FOUND;
    }

  if (in_label->type == gmpls_entry_type_ip)
    {
#if CHECK_LABEL
      if (! VALID_LABEL (in_label.u.pkt))
        {
          zlog_err (NSM_ZG, "%% Incoming label value %s is invalid\n", 
                             in_lbl_str);
          return NSM_ERR_INVALID_LABEL;
        }
#endif /* CHECK_LABEL */

  /* Check if incoming label is within static label range for incoming
   *  *      interface's label space */
      ret = nsm_gmpls_static_label_range_check (in_label->u.pkt, ifp_in,
                                                &min_label, &max_label);
    }

  if (ret < 0)
    {
      zlog_err (NSM_ZG, "%% Incoming label should within range of %d - %d \n",
                min_label, max_label);
      return NSM_ERR_INVALID_LABEL;
    }

  if ((opcode == SWAP) && (ifp_out && if_is_loopback (ifp_out)))
    {
      zlog_err (NSM_ZG, "%% Outgoing interface not allow loopback for swap \n");
      return NSM_ERR_INVALID_LABEL;
    }
  
  if ((ilm_ix == 0) &&
      (gmpls_ilm_node_lookup (nm, in_idx, in_label) != NULL))
    {
      zlog_warn (NSM_ZG, "Duplicate ILM entry \n");
      return NSM_ERR_ILM_DUP_ENTRY;
    }

  /* Fill ilm_add_data. */
  pal_mem_set (&iad, 0, sizeof (struct ilm_add_gen_data));
  iad.owner.owner = MPLS_OTHER_CLI;
  iad.owner.u.vc_key.vc_id = vc_id;
#ifdef HAVE_MPLS_VC
  if (vc)
    iad.owner.u.vc_key.vc_peer = vc->address.u.prefix4;
  else
    pal_mem_set (&iad.owner.u.vc_key.vc_peer, 0, sizeof (struct pal_in4_addr));
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_GMPLS
  if (CHECK_FLAG (flags, NSM_MPLS_CLI_ILM_BIDIR_REV_FTN))
    {
      iad.rev_ftn_ix = rev_idx;
    }
  else
    {
      iad.rev_ilm_ix = rev_idx;
    }
#endif /* HAVE_GMPLS */

  iad.iif_ix = in_idx;
  iad.in_label = *in_label;
  iad.oif_ix = out_idx;
  iad.out_label = *out_label;
  iad.ilm_ix = ilm_ix;
  iad.nh_addr.afi = AFI_IP;
  iad.fec_prefix.family = AF_INET;
  iad.fec_prefix.prefixlen = fec->u.prefix.prefixlen;
  IPV4_ADDR_COPY (&iad.fec_prefix.u.prefix4, &fec->u.prefix.u.prefix4);
  IPV4_ADDR_COPY (&iad.nh_addr.u.ipv4, nhop);
  iad.opcode = opcode;

#ifdef HAVE_GMPLS
  if (is_bidir == PAL_TRUE)
    SET_FLAG (iad.flags, NSM_MSG_ILM_BIDIR);
 
  if (is_gmpls == PAL_TRUE)
    SET_FLAG (iad.flags, NSM_MSG_ILM_GEN_MPLS);
#endif /* HAVE_GMPLS */

  if (opcode == POP_FOR_VC)
    {
      iad.fec_prefix.u.prefix4.s_addr = pal_hton32 (vc_id);
      iad.fec_prefix.prefixlen = IPV4_MAX_BITLEN;
    }

  if ((opcode == POP) ||
      (opcode == SWAP) ||
      (opcode == POP_FOR_VPN) ||
      (opcode == POP_FOR_VC))
    iad.n_pops = 1;

  ret = nsm_gmpls_ilm_add_msg_process (nm, &iad, NSM_FALSE, &rd, NSM_TRUE);

  if (ret < 0)
    {
      if (ret == NSM_ERR_ILM_INSTALL_FAILURE)
        {
          nsm_gmpls_ilm_del_msg_process (nm, &iad);
        }
      zlog_warn (NSM_ZG, "Could not add ILM Entry %d --> %d",
                 in_label, out_label);
      return NSM_FAILURE;
    }
  else if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Added/Modified ILM Entry %d --> %d",
        in_label, out_label);

  return NSM_SUCCESS;
}

#ifdef HAVE_PACKET
s_int32_t
nsm_mpls_ilm_entry_add_sub_pfx (struct nsm_master *nm, u_int32_t vc_id,
                                u_int32_t in_label, char *if_in,
                                u_int32_t out_label, char *if_out,
                                struct pal_in4_addr *nhop, u_char opcode,
                                struct prefix* fec, u_int32_t ilm_ix,
                                u_int32_t flags, u_int32_t rev_idx)
{
  struct gmpls_gen_label ilbl, olbl;
  struct fec_gen_entry gfec;

  ilbl.type = gmpls_entry_type_ip;
  ilbl.u.pkt = in_label;

  olbl.type = gmpls_entry_type_ip;
  olbl.u.pkt = out_label;

  gfec.type = gmpls_entry_type_ip;
  gfec.u.prefix = *fec;

  /* Also convert from physical link to datalink */
  return nsm_gmpls_ilm_entry_add_sub (nm, vc_id, &ilbl, if_in, &olbl, if_out,
                                      nhop, opcode, &gfec, ilm_ix, flags, 
                                      rev_idx);
}

#endif /* HAVE_PACKET */

struct ftn_entry *
nsm_mpls_get_rsvp_ftn_by_lsp (struct nsm_master *nm, u_int32_t *lookup)
{

  struct ftn_entry *ftn = NULL;
  struct ptree_node *pn_ftn = NULL;
  struct ptree_ix_table *ft, *nxt_ft;
  u_int16_t *lsp_id = NULL;

  lsp_id = (u_int16_t *)lookup;

  /* Loop into the FTN table to identify the required ftn entry for use with 
    this VC*/
  if (lsp_id)
    {
      GET_FIRST_FTN_IX_TABLE (ft);
      while (ft)
        {
          for (pn_ftn = ptree_top (ft->m_table); pn_ftn ;
               pn_ftn = ptree_next (pn_ftn))
             {
               for (ftn = pn_ftn->info; ftn; ftn = ftn->next)
                {
                  if ((ftn->owner.owner == MPLS_RSVP) &&
                       CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED)) 
                     {
                       if (ftn->owner.u.r_key.u.ipv4.lsp_id == *lsp_id)
                         return ftn;                  
                     }
                 }
             }

           GET_NEXT_FTN_IX_TABLE (ftn, nxt_ft);
           ft = nxt_ft;
         }

      if (!pn_ftn)
        return NULL;
    }

  return NULL;
}

#ifdef HAVE_GMPLS
s_int32_t 
nsm_gmpls_data_link_down_to_rib (u_int32_t vr_id, struct datalink *dl)
{
  if ((!dl) || (!dl->ifp) || (!dl->ifp->vr))
    {
      return NSM_FAILURE;
    }

  nsm_mpls_ftn_if_down_process (dl->ifp->vr->proto, dl->gifindex);
  nsm_mpls_ilm_if_down_process (dl->ifp->vr->proto, dl->gifindex);

  return NSM_SUCCESS;
}

s_int32_t 
nsm_gmpls_data_link_up_to_rib (u_int32_t vr_id, struct datalink *dl)
{
  if ((!dl) || (!dl->ifp) || (!dl->ifp->vr))
    {
      return NSM_FAILURE;
    }

  nsm_mpls_ftn_if_up_process (dl->ifp->vr->proto, dl->gifindex);
  nsm_mpls_ilm_if_up_process (dl->ifp->vr->proto, dl->gifindex);

  return NSM_SUCCESS;
}

bool_t
nsm_gmpls_bidir_is_rev_dir_up (struct ftn_entry *ftn)
{
  struct ilm_entry *ilm, *ilm_ahead;

  if (!CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_BIDIR))
    return NSM_FALSE;

  if (ftn->ilm_rev == NULL)
    return NSM_FALSE;

  /* Now check if the ftn->ilm_rev can be selected */
  ilm = ftn->ilm_rev;

  /* Reverse entry is an ILM entry */
  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_INACTIVE))
    {
      return PAL_FALSE;
    }

  ilm_ahead = mpls_ilm_active_head (ilm);
  if ((!CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_PRIMARY)) && ilm_ahead &&
      (ilm_ahead != ilm))
    {
      return PAL_FALSE;
    }

  return NSM_TRUE;
}

bool_t
nsm_gmpls_bidir_is_rev_dir_up_ilm (struct ilm_entry *ilm)
{
  mpls_lsp_priority_t ftn_priority;
  mpls_lsp_priority_t head_pri = MPLS_P_NONE;
  struct ftn_entry *ftn_head = NULL, *ftn;
  struct ilm_entry *rev_ilm, *ilm_ahead;

  if (!CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_BIDIR))
    return NSM_FALSE;

  if (ilm->rev_entry == NULL)
    return NSM_FALSE;

  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_REV_FTN))
    {
      ftn = (struct ftn_entry *) ilm->rev_entry;

      /* Reverse entry is an FTN entry */
      if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_INACTIVE))
        {
          return PAL_FALSE;
        }

      if ((ftn->pn != NULL) && (ftn->pn->info != NULL))
        {
          ftn_head = gmpls_ftn_active_head ((struct ftn_entry *)ftn->pn->info);
        }

      if (ftn_head)
        {
          head_pri = gmpls_owner_priority (ftn_head->owner.owner);
        }

      ftn_priority = gmpls_owner_priority (ftn->owner.owner);
      if (ftn_priority > head_pri)
        {
          return PAL_FALSE;
        }
    }
  else
    {
      rev_ilm = (struct ilm_entry *) ilm->rev_entry;

      /* Reverse entry is an ILM entry */
      if (CHECK_FLAG (rev_ilm->flags, ILM_ENTRY_FLAG_INACTIVE))
        {
          return PAL_FALSE;
        }

      ilm_ahead = mpls_ilm_active_head (ilm);
      if ((!CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_PRIMARY)) && ilm_ahead &&
           (ilm_ahead != ilm))
        {
          return PAL_FALSE;
        }
    }
  return NSM_TRUE;
}

s_int32_t
nsm_gmpls_bidir_ftn_get_rev_entry (struct ftn_entry *ftn, 
                                   struct ilm_entry *ilm)
{
  ilm = NULL;

  if (!CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_BIDIR))
    return NSM_FAILURE;

  ilm = ftn->ilm_rev;
  return NSM_SUCCESS;
}

s_int32_t
nsm_gmpls_bidir_ilm_get_rev_entry (struct ilm_entry *ilm, 
                                   struct ftn_entry *ftn, 
                                   struct ilm_entry *rev_ilm)
{
  rev_ilm = NULL;
  ftn = NULL;

  if (!CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_BIDIR))
    return NSM_FAILURE;

  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_REV_FTN))
    {
      ftn = (struct ftn_entry *) ilm->rev_entry;
    }
  else
    {
      ilm = (struct ilm_entry *) ilm->rev_entry;
    }

  return NSM_SUCCESS;
}

#endif /* HAVE_GMPLS */
