/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"

#include "if.h"
#include "prefix.h"
#include "log.h"
#include "mpls.h"
#include "mpls_common.h"
#include "mpls_client.h"
#include "nsmd.h"
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */

#ifdef HAVE_DSTE
#include "nsm_dste.h"
#endif /* HAVE_DSTE */
#include "rib/nsm_table.h"
#include "nsm_debug.h"
#include "nsm_mpls.h"
#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */
#include "nsm_mpls_rib.h"
#include "nsm_mpls_fwd.h"
#include "nsm_server.h"
#ifdef HAVE_MPLS_OAM 
#include "nsm_mpls_oam.h"
#ifdef HAVE_VCCV
#include "mpls_util.h"
#endif /* HAVE_VCCV */
#endif /* HAVE_MPLS_OAM */
#ifdef HAVE_GMPLS
#include "gmpls.h"
#endif /* HAVE_GMPLS */

#if defined HAVE_GELS && defined HAVE_I_BEB && defined HAVE_B_BEB
extern int
nsm_pbb_te_create_dynamic_esp(struct interface *ifp, u_int32_t tesid,
                      u_char *bmac, u_int16_t bvid, bool_t egress);

extern int
nsm_pbb_te_delete_dynamic_esp(struct interface *ifp, u_int32_t tesid,
                      u_char *bmac, u_int16_t bvid, bool_t egress);
#endif /* HAVE_GELS && HAVE_I_BEB && HAVE_B_BEB */

#ifdef HAVE_IPV6
/* Add FTN entry to Forwarder. */
int
nsm_mpls_ftn6_add_to_fwd (struct nsm_master *nm, struct ftn_entry *ftn,
                          struct prefix *fec_prefix, u_int32_t vrf_id,
                          struct ftn_entry *tunnel_ftn, int active_head)
{
  struct interface *tunnel_ifp, *vpn_ifp;
  struct if_ident tunnel_if_ident, vpn_if_ident;
  struct mpls_nh_fwd tunnel_nhop, vpn_nhop;
  struct nhlfe_key *nkey, *tkey = NULL;
  u_int32_t tunnel_label, vpn_label;
  u_int32_t tunnel_oix = 0, vpn_oix = 0;
  u_char opcode;
  struct mpls_owner_fwd owner;
  int ret;

  if (ftn == NULL || fec_prefix == NULL
      || FTN_XC (ftn) == NULL || FTN_NHLFE (ftn) == NULL)
    return NSM_FAILURE;

  /* Dummy entries dont need to be added */
  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_DUMMY))
    return NSM_SUCCESS;

#ifdef HAVE_GMPLS
  /* Bidirectional check */
  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_BIDIR) &&
      !nsm_gmpls_bidir_is_rev_dir_up (ftn))
    return NSM_SUCCESS;
#endif /*HAVE_GMPLS*/

  /* Get valid outgoing interface pointer. */
  nkey = (struct nhlfe_key *) &FTN_NHLFE (ftn)->nkey;
  if (tunnel_ftn)
    {
      tkey = (struct nhlfe_key *) &FTN_NHLFE (tunnel_ftn)->nkey;
    }

  if (nkey->afi == AFI_IP)
    tunnel_label = tunnel_ftn ? 
                   tkey->u.ipv4.out_label : 0;
  else 
    {
      if (nkey->afi == AFI_IP6)
        tunnel_label = tunnel_ftn ? 
                       tkey->u.ipv6.out_label : 0;
      else 
        return NSM_ERR_INVALID_ARGS;
    }

  if (tunnel_ftn && tunnel_label != LABEL_IMPLICIT_NULL)
    {
      if (tkey->afi == AFI_IP)
        {
          tunnel_nhop.afi = AFI_IP;
          IPV4_ADDR_COPY (&tunnel_nhop.u.ipv4, 
                          &tkey->u.ipv4.nh_addr);
          tunnel_oix = tkey->u.ipv4.oif_ix;
        }
      else
        {
          tunnel_nhop.afi = AFI_IP6;
          IPV6_ADDR_COPY (&tunnel_nhop.u.ipv6, 
                          &tkey->u.ipv6.nh_addr);
          tunnel_oix = tkey->u.ipv6.oif_ix;
        }

      /* Lookup tunnel outgoing ifp. */
      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_GMPLS), 
                              tunnel_oix, tunnel_ifp);
      if (! tunnel_ifp)
        return NSM_FAILURE;

      /* Fill ident. */
      pal_mem_set (&tunnel_if_ident, 0, sizeof (struct if_ident));
      tunnel_if_ident.if_index = tunnel_ifp->ifindex;
      pal_mem_cpy (tunnel_if_ident.if_name, tunnel_ifp->name, INTERFACE_NAMSIZ);
      /* VPN labels to use */
      if (nkey->afi == AFI_IP)
        {
          vpn_label = nkey->u.ipv4.out_label;
          IPV4_ADDR_COPY (&vpn_nhop.u.ipv4,
                          &nkey->u.ipv4.nh_addr);

          vpn_oix = nkey->u.ipv4.oif_ix;
        }
      else
        {
          vpn_label = nkey->u.ipv6.out_label;
          IPV6_ADDR_COPY (&vpn_nhop.u.ipv6, 
                          &nkey->u.ipv6.nh_addr);
          vpn_oix = nkey->u.ipv6.oif_ix;
        }

      /* lookup vpn outgoing ifp */
      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ftn->flags, 
                              FTN_ENTRY_FLAG_GMPLS), vpn_oix, vpn_ifp);
      pal_mem_set (&vpn_if_ident, 0, sizeof (struct if_ident));
      if (vpn_ifp)
        {
          vpn_if_ident.if_index = vpn_ifp->ifindex;
          pal_mem_cpy (vpn_if_ident.if_name, vpn_ifp->name, INTERFACE_NAMSIZ);
        }
    }
  else
    {
      if (nkey->afi == AFI_IP)
        {
          tunnel_nhop.afi = AFI_IP;
          tunnel_label = nkey->u.ipv4.out_label;
          if (tunnel_label == LABEL_IMPLICIT_NULL)
            return NSM_SUCCESS;
          
          IPV4_ADDR_COPY (&tunnel_nhop.u.ipv4,
                          &nkey->u.ipv4.nh_addr);
          tunnel_oix = nkey->u.ipv4.oif_ix;
        }
      else if (nkey->afi == AFI_IP6)
        {
          tunnel_nhop.afi = AFI_IP6;
          tunnel_label = nkey->u.ipv6.out_label;
          if (tunnel_label == LABEL_IMPLICIT_NULL)
            return NSM_SUCCESS;

          IPV6_ADDR_COPY (&tunnel_nhop.u.ipv6,
                          &nkey->u.ipv6.nh_addr);
          tunnel_oix = nkey->u.ipv6.oif_ix;
        }

      /* Lookup tunnel outgoing ifp. */
      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ftn->flags, 
                              FTN_ENTRY_FLAG_GMPLS), tunnel_oix,tunnel_ifp);
      if (! tunnel_ifp)
        return NSM_FAILURE;

      /* Fill ident. */
      pal_mem_set (&tunnel_if_ident, 0, sizeof (struct if_ident));
      tunnel_if_ident.if_index = tunnel_ifp->ifindex;
      pal_mem_cpy (tunnel_if_ident.if_name, tunnel_ifp->name, INTERFACE_NAMSIZ);

      vpn_label = LABEL_VALUE_INVALID;
      pal_mem_set (&vpn_nhop, '\0', sizeof (vpn_nhop));
      vpn_oix = 0;
      vpn_ifp = NULL;
      pal_mem_set (&vpn_if_ident, 0, sizeof (struct if_ident));
    }

  opcode = FTN_NHLFE (ftn)->opcode;
  if (opcode == SWAP)
    opcode = PUSH;
  else if (opcode == POP_FOR_VPN)
    opcode = DLVR_TO_IP;
  
  pal_mem_set (&owner, 0, sizeof (struct mpls_owner_fwd));
  if (ftn->owner.owner == MPLS_RSVP) 
    {
      struct rsvp_key_fwd *key = &owner.u.r_key;
      if (ftn->owner.u.r_key.afi == AFI_IP)
        {
          owner.protocol = APN_PROTO_RSVP;
          key->afi = AFI_IP;
          key->len = sizeof (struct rsvp_key_ipv4_fwd);
          key->u.ipv4.trunk_id = ftn->owner.u.r_key.u.ipv4.trunk_id;
          key->u.ipv4.lsp_id = ftn->owner.u.r_key.u.ipv4.lsp_id;
          IPV4_ADDR_COPY (&key->u.ipv4.egr,
                          &ftn->owner.u.r_key.u.ipv4.egr);
          IPV4_ADDR_COPY (&key->u.ipv4.ingr,
                          &ftn->owner.u.r_key.u.ipv4.ingr);
        }
      else
        {
          owner.protocol = APN_PROTO_RSVP;
          key->afi = AFI_IP6;
          key->len = sizeof (struct rsvp_key_ipv6_fwd);
          key->u.ipv6.trunk_id = ftn->owner.u.r_key.u.ipv6.trunk_id;
          key->u.ipv6.lsp_id = ftn->owner.u.r_key.u.ipv6.lsp_id;
          IPV6_ADDR_COPY (&key->u.ipv6.egr,
                          &ftn->owner.u.r_key.u.ipv6.egr);
          IPV6_ADDR_COPY (&key->u.ipv6.ingr,
                          &ftn->owner.u.r_key.u.ipv6.ingr);
        }
    }

  ret = apn_mpls_ftn6_entry_add (vrf_id == GLOBAL_FTN_ID 
                                ? GLOBAL_TABLE : vrf_id,
                                APN_PROTO_NSM,
                                &fec_prefix->u.prefix6,
                                &fec_prefix->prefixlen,
                                &ftn->dscp_in,
                                &tunnel_label,
                                &tunnel_nhop,
                                &tunnel_if_ident,
                                vpn_label == LABEL_VALUE_INVALID 
                                ? NULL : &vpn_label,
                                &vpn_nhop,
                                vpn_oix == 0 ? NULL : &vpn_if_ident,
                                &ftn->tun_id,
                                &ftn->qos_resrc_id,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                (struct ds_info_fwd *)&ftn->ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                                opcode,
                                FTN_NHLFE (ftn)->nhlfe_ix,
                                ftn->ftn_ix,
                                (u_char)ftn->ftn_type,
                                &owner,
                                ftn->bypass_ftn_ix,
                                ftn->lsp_bits.bits.lsp_type,
                                active_head);

  if (ret < 0)
    {
      zlog_warn (NSM_ZG, "Could not add %s entry %r --> %d",
                 ((vrf_id == GLOBAL_FTN_ID) ? "FTN" : "VRF"),
                 &fec_prefix->u.prefix4,
                 vpn_label == LABEL_VALUE_INVALID ? tunnel_label : vpn_label);
      return NSM_ERR_FTN_INSTALL_FAILURE;
    }
  else if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Added/Modified %s entry %r --> %d",
               ((vrf_id == GLOBAL_FTN_ID) ? "FTN" : "VRF"),
               &fec_prefix->u.prefix4,
               vpn_label == LABEL_VALUE_INVALID ? tunnel_label : vpn_label);

#ifdef HAVE_MPLS_FWD
  /* mplsInterfacePerfOutLabelsUsed */
  nsm_mpls_if_stats_incr_out_labels_used (tunnel_ifp);
#endif /* HAVE_MPLS_FWD */

  return NSM_SUCCESS;
}
#endif

/* Add FTN entry to Forwarder. */
int
nsm_mpls_ftn4_add_to_fwd (struct nsm_master *nm, struct ftn_entry *ftn,
                         struct prefix *fec_prefix, u_int32_t vrf_id,
                         struct ftn_entry *tunnel_ftn, int active_head)
{
  struct interface *tunnel_ifp, *vpn_ifp;
  struct if_ident tunnel_if_ident, vpn_if_ident;
  struct pal_in4_addr *tunnel_nhop, *vpn_nhop;
#ifdef HAVE_GMPLS
  struct pal_in4_addr nhop;
#endif /* HAVE_GMPLS */
  struct nhlfe_key *nkey, *tkey;
  u_int32_t tunnel_label, vpn_label;
  u_int32_t tunnel_oix, vpn_oix;
  u_char opcode;
  struct mpls_owner_fwd owner;
  int ret = NSM_SUCCESS;

  if (ftn == NULL || fec_prefix == NULL
      || FTN_XC (ftn) == NULL || FTN_NHLFE (ftn) == NULL)
    return NSM_FAILURE;

#ifdef HAVE_GELS
    if((ftn->tgen_data) && 
       (((struct gmpls_tgen_data *)ftn->tgen_data)->gmpls_type == 
                                              gmpls_entry_type_pbb_te))
#if defined HAVE_I_BEB && defined HAVE_B_BEB
    {
      struct interface *ifp = NULL;
      struct nhlfe_key_gen *nkey;

      nkey = (struct nhlfe_key_gen *) ftn->xc->nhlfe->nkey;

      /* Get the interface structure from the ifindex */
      NSM_MPLS_GET_INDEX_PTR (PAL_TRUE, nkey->u.pbb.oif_ix, ifp);

      /* Call the PBB-TE API */
      ret = nsm_pbb_te_create_dynamic_esp(ifp,
                ((struct gmpls_tgen_data *)ftn->tgen_data)->u.pbb.tesid,
                nkey->u.pbb.lbl.bmac,
                nkey->u.pbb.lbl.bvid, PAL_TRUE);

      return ret;
    }
    else
#else
    {
      return ret;
    } 
#endif /* HAVE_I_BEB && HAVE_B_BEB */
#endif /* HAVE_GELS */

  /* Dummy entries dont need to be added */
  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_DUMMY))
    return NSM_SUCCESS;

  /* Get valid outgoing interface pointer. */
  nkey = (struct nhlfe_key *) FTN_NHLFE (ftn)->nkey;
  if (nkey->afi != AFI_IP)
    return NSM_ERR_INVALID_ARGS;

  if (tunnel_ftn)
    tkey = (struct nhlfe_key *) FTN_NHLFE (tunnel_ftn)->nkey;
  else
    tkey = (struct nhlfe_key *) FTN_NHLFE (ftn)->nkey;

  if (tunnel_ftn && 
      tkey->u.ipv4.out_label != LABEL_IMPLICIT_NULL)
    {
      tunnel_label = tkey->u.ipv4.out_label;
#ifdef HAVE_GMPLS
      if (tkey->u.ipv4.nh_addr.s_addr == 0xFFFFFFFF)
        {
          pal_mem_set (&nhop, '\0', sizeof (struct pal_in4_addr));
          tunnel_nhop = &nhop;
        }
      else
#endif /* HAVE_GMPLS */
        tunnel_nhop = &tkey->u.ipv4.nh_addr;

      tunnel_oix = tkey->u.ipv4.oif_ix;

      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_GMPLS),
                              tunnel_oix, tunnel_ifp);
    
      if (! tunnel_ifp)
        return NSM_FAILURE;

      /* Fill ident. */
      pal_mem_set (&tunnel_if_ident, 0, sizeof (struct if_ident));
      tunnel_if_ident.if_index = tunnel_ifp->ifindex;
      pal_mem_cpy (tunnel_if_ident.if_name, tunnel_ifp->name, INTERFACE_NAMSIZ);

      vpn_label = nkey->u.ipv4.out_label;
#ifdef HAVE_GMPLS
      if (tkey->u.ipv4.nh_addr.s_addr == 0xFFFFFFFF)
        {
          pal_mem_set (&nhop, '\0', sizeof (struct pal_in4_addr));
          vpn_nhop = &nhop;
        }
      else
#endif /* HAVE_GMPLS */
        vpn_nhop = &nkey->u.ipv4.nh_addr;

      vpn_oix = nkey->u.ipv4.oif_ix;

      /* lookup vpn outgoing ifp */
      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_GMPLS), 
                               vpn_oix, vpn_ifp);

      pal_mem_set (&vpn_if_ident, 0, sizeof (struct if_ident));
      if (vpn_ifp)
        {
          vpn_if_ident.if_index = vpn_ifp->ifindex;
          pal_mem_cpy (vpn_if_ident.if_name, vpn_ifp->name, INTERFACE_NAMSIZ);
        }
    }
  else
    {
      tunnel_label = tkey->u.ipv4.out_label;
      if (tunnel_label == LABEL_IMPLICIT_NULL)
        return NSM_SUCCESS;

#ifdef HAVE_GMPLS
     if (tkey->u.ipv4.nh_addr.s_addr == 0xFFFFFFFF)
       {
         pal_mem_set (&nhop, '\0', sizeof (struct pal_in4_addr));
         tunnel_nhop = &nhop;
       }
     else
#endif /* HAVE_GMPLS */
       tunnel_nhop = &tkey->u.ipv4.nh_addr;

      tunnel_oix = tkey->u.ipv4.oif_ix;

      /* Lookup tunnel outgoing ifp. */
      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_GMPLS), 
                              tunnel_oix, tunnel_ifp);

      if (! tunnel_ifp)
        return NSM_FAILURE;

      /* Fill ident. */
      pal_mem_set (&tunnel_if_ident, 0, sizeof (struct if_ident));
      tunnel_if_ident.if_index = tunnel_ifp->ifindex;
      pal_mem_cpy (tunnel_if_ident.if_name, tunnel_ifp->name, INTERFACE_NAMSIZ);

      vpn_label = LABEL_VALUE_INVALID;
      vpn_nhop = NULL;
      vpn_oix = 0;
      vpn_ifp = NULL;
      pal_mem_set (&vpn_if_ident, 0, sizeof (struct if_ident));
    }

  opcode = FTN_NHLFE (ftn)->opcode;
  if (opcode == SWAP)
    opcode = PUSH;
  else if (opcode == POP_FOR_VPN)
    opcode = DLVR_TO_IP;
  
  pal_mem_set (&owner, 0, sizeof (struct mpls_owner_fwd));
  if (ftn->owner.owner == MPLS_RSVP &&
      ftn->owner.u.r_key.afi == AFI_IP)
    {
      struct rsvp_key_fwd *key = &owner.u.r_key;
      
      owner.protocol = APN_PROTO_RSVP;
      key->afi = AFI_IP;
      key->len = sizeof (struct rsvp_key_ipv4_fwd);
      key->u.ipv4.trunk_id = ftn->owner.u.r_key.u.ipv4.trunk_id;
      key->u.ipv4.lsp_id = ftn->owner.u.r_key.u.ipv4.lsp_id;
      IPV4_ADDR_COPY (&key->u.ipv4.egr,
                      &ftn->owner.u.r_key.u.ipv4.egr);
      IPV4_ADDR_COPY (&key->u.ipv4.ingr,
                      &ftn->owner.u.r_key.u.ipv4.ingr);
    }

  ret = apn_mpls_ftn4_entry_add (vrf_id == GLOBAL_FTN_ID 
                                ? GLOBAL_TABLE : vrf_id,
                                APN_PROTO_NSM,
                                &fec_prefix->u.prefix4,
                                &fec_prefix->prefixlen,
                                &ftn->dscp_in,
                                &tunnel_label,
                                tunnel_nhop,
                                &tunnel_if_ident,
                                vpn_label == LABEL_VALUE_INVALID 
                                ? NULL : &vpn_label,
                                vpn_nhop,
                                vpn_oix == 0 ? NULL : &vpn_if_ident,
                                &ftn->tun_id,
                                &ftn->qos_resrc_id,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                (struct ds_info_fwd *)&ftn->ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* ! HAVE_GMPLS || HAVE_PACKET */
                                opcode,
                                FTN_NHLFE (ftn)->nhlfe_ix,
                                ftn->ftn_ix,
                                (u_char)ftn->ftn_type,
                                &owner,
                                ftn->bypass_ftn_ix,
                                ftn->lsp_bits.bits.lsp_type,
                                active_head);

  if (ret < 0)
    {
      zlog_warn (NSM_ZG, "Could not add %s entry %r --> %d",
                 ((vrf_id == GLOBAL_FTN_ID) ? "FTN" : "VRF"),
                 &fec_prefix->u.prefix4,
                 vpn_label == LABEL_VALUE_INVALID ? tunnel_label : vpn_label);
      return NSM_ERR_FTN_INSTALL_FAILURE;
    }
  else if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Added/Modified %s entry %r --> %d",
               ((vrf_id == GLOBAL_FTN_ID) ? "FTN" : "VRF"),
               &fec_prefix->u.prefix4,
               vpn_label == LABEL_VALUE_INVALID ? tunnel_label : vpn_label);

#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
  /* Setting the FTN entry installed flag here itself 
   * to correctly update BFD sessions. 
   * This setting is anyways again done in the caller of this function.
   */
  SET_FLAG (ftn->flags, FTN_ENTRY_FLAG_INSTALLED);
  nsm_mpls_ftn_bfd_update (nm, ftn);
#endif /* HAVE_BFD && HAVE_MPLS_OAM */

#ifdef HAVE_MPLS_FWD
  /* mplsInterfacePerfOutLabelsUsed */
  nsm_mpls_if_stats_incr_out_labels_used (tunnel_ifp);
#endif /* HAVE_MPLS_FWD */

  return NSM_SUCCESS;
}

/* Add ILM entry to Forwarder. */
int
nsm_mpls_ilm_add_to_fwd (struct nsm_master *nm, struct ilm_entry *ilm,
                         struct ftn_entry *tunnel_ftn)
{
  u_int32_t iix, oix;
  struct interface *ifp_in, *ifp_out;
  struct if_ident ident_in, ident_out;
  struct mpls_nh_fwd nhop;
  struct prefix p;
  u_char opcode;
  u_int32_t in_label, swap_label, tunnel_label;
  u_char egress;
  struct nhlfe_key *nkey, *tkey = NULL;
  u_int32_t *o_lbl = NULL;
  struct mpls_owner_fwd owner;
  u_int32_t vpn_id = 0;
  struct pal_in4_addr *vc_peer = NULL;
  int ret;

  if (ilm == NULL 
      || ILM_XC (ilm) == NULL || ILM_NHLFE (ilm) == NULL)
    return NSM_FAILURE;

  nkey = (struct nhlfe_key *) &ILM_NHLFE (ilm)->nkey;
  if (nkey->afi != AFI_IP
#ifdef HAVE_IPV6
      && nkey->afi != AFI_IP6
#endif
    )
    return -1;

#ifdef HAVE_GMPLS
  if (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_DUMMY))
    return NSM_SUCCESS;

#endif /* HAVE_GMPLS */

  opcode = ILM_NHLFE (ilm)->opcode;
  if ((opcode == PUSH) && (ilm->n_pops == 1))
    opcode = SWAP;
  
  if ((opcode == POP) || (opcode == POP_FOR_VC) ||
      (opcode == POP_FOR_VPN))
    {
      o_lbl = NULL;
      egress = NSM_TRUE;
    }
  else if (opcode == DLVR_TO_IP)
    {
      o_lbl = NULL;
      egress = NSM_TRUE;
      opcode = POP_FOR_VPN;
    }
  else 
    {
      o_lbl = &swap_label;
      egress = NSM_FALSE;
    }

    if (tunnel_ftn)
      tkey = (struct nhlfe_key *) &FTN_NHLFE (tunnel_ftn)->nkey;

#ifdef HAVE_IPV6 
  if (nkey->afi == AFI_IP6)
    {
      swap_label = nkey->u.ipv6.out_label;
  
      if (tunnel_ftn && 
          tkey->u.ipv6.out_label != LABEL_IMPLICIT_NULL)
        {
          tunnel_label = tkey->u.ipv6.out_label;
          oix = tkey->u.ipv6.oif_ix;
          nhop.afi =  tkey->afi;

          if (tkey->afi == AFI_IP)
            {
#ifdef HAVE_GMPLS
              if (tkey->u.ipv4.nh_addr.s_addr == 0xFFFFFFFF)
                pal_mem_set (&nhop.u.ipv4, '\0', sizeof (struct pal_in4_addr));
              else
#endif /* HAVE_GMPLS */
                IPV4_ADDR_COPY (&nhop.u.ipv4, &tkey->u.ipv4.nh_addr);
            }
          else
            {
              IPV6_ADDR_COPY (&nhop.u.ipv6, &tkey->u.ipv6.nh_addr);
            }
      
          if (tunnel_label == LABEL_VALUE_INVALID)
            return -1;

          if (swap_label == LABEL_IMPLICIT_NULL)
            {
              swap_label = tunnel_label;
              tunnel_label = LABEL_VALUE_INVALID;
            }
        }
      else
        {
          tunnel_label = LABEL_VALUE_INVALID;
          nhop.afi =  nkey->afi;
          if (nkey->afi == AFI_IP)
            {
              oix = nkey->u.ipv4.oif_ix;
              IPV4_ADDR_COPY (&nhop.u.ipv4, 
                              &nkey->u.ipv6.nh_addr);
            }
          else
            {
              oix = nkey->u.ipv6.oif_ix;
              IPV6_ADDR_COPY (&nhop.u.ipv6,
                              &nkey->u.ipv6.nh_addr);
            }
        }
    }
  else
#endif
    {
      swap_label = nkey->u.ipv4.out_label;
      nhop.afi =  nkey->afi;
  
      if (tunnel_ftn && 
          tkey->u.ipv4.out_label != LABEL_IMPLICIT_NULL)
        {
          tunnel_label = tkey->u.ipv4.out_label;
          oix = tkey->u.ipv4.oif_ix;
#ifdef HAVE_GMPLS
          if (tkey->u.ipv4.nh_addr.s_addr == 0xFFFFFFFF)
            pal_mem_set (&nhop.u.ipv4, '\0', sizeof (struct pal_in4_addr));
          else
#endif /* HAVE_GMPLS */
            IPV4_ADDR_COPY (&nhop.u.ipv4, &tkey->u.ipv4.nh_addr);
      
          if (tunnel_label == LABEL_VALUE_INVALID)
            return -1;

          if (swap_label == LABEL_IMPLICIT_NULL)
            {
              swap_label = tunnel_label;
              tunnel_label = LABEL_VALUE_INVALID;
            }
        }
      else
        {
          tunnel_label = LABEL_VALUE_INVALID;
          oix = nkey->u.ipv4.oif_ix;

#ifdef HAVE_GMPLS
          if (nkey->u.ipv4.nh_addr.s_addr == 0xFFFFFFFF)
            pal_mem_set (&nhop.u.ipv4, '\0', sizeof (struct pal_in4_addr));
          else
#endif /* HAVE_GMPLS */
            IPV4_ADDR_COPY (&nhop.u.ipv4, &nkey->u.ipv4.nh_addr);
        }
    }

  iix = ilm->key.u.pkt.iif_ix;
  NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_GMPLS), 
                          iix, ifp_in);

#ifdef HAVE_GMPLS
  if (oix == NSM_MPLS->loop_gindex)
    {
      /* ifp_out is the loopback interface */
      ifp_out = if_lookup_by_name (&nm->vr->ifm, "lo");
    }
  else
#endif /* HAVE_GMPLS */
    {
      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_GMPLS), 
                              oix, ifp_out);
    }
  
  if (ifp_in && ifp_out)
    {
      /* Fill ident in. */
      pal_mem_set (&ident_in, 0, sizeof (struct if_ident));
      ident_in.if_index = ifp_in->ifindex;
      pal_mem_cpy (ident_in.if_name, ifp_in->name, INTERFACE_NAMSIZ);
      
      /* Fill ident out. */
      pal_mem_set (&ident_out, 0, sizeof (struct if_ident));
      ident_out.if_index = ifp_out->ifindex;
      pal_mem_cpy (ident_out.if_name, ifp_out->name, INTERFACE_NAMSIZ);
      
      in_label = ilm->key.u.pkt.in_label;

      p.family = ilm->family;
      p.prefixlen= ilm->prefixlen;

#ifdef HAVE_IPV6
      if (p.family == AF_INET6)
        pal_mem_cpy (&p.u.prefix6, ilm->prfx, sizeof (struct pal_in6_addr));
      else
#endif
        pal_mem_cpy (&p.u.prefix4, ilm->prfx, sizeof (struct pal_in4_addr));

      if ((opcode == SWAP) || (opcode == SWAP_AND_LOOKUP) || 
         (opcode == POP) || (opcode == POP_FOR_VPN) || (opcode == POP_FOR_VC))
        {
          char *opcode_str = "";
          
          if (opcode == POP_FOR_VPN)
            opcode_str = "VRF ";
#ifdef HAVE_MPLS_VC
          else if (opcode == POP_FOR_VC)
            {
              opcode_str = "VC ";
              vpn_id = ilm->owner.u.vc_key.vc_id;
              vc_peer = &ilm->owner.u.vc_key.vc_peer;
            }
#endif /* HAVE_MPLS_VC */
          
          pal_mem_set (&owner, 0 , sizeof (struct mpls_owner_fwd));
#ifdef HAVE_IPV6
          if (ilm->owner.u.r_key.afi == AFI_IP6)
            {
              if (ilm->owner.owner == MPLS_RSVP)
                {
                  struct rsvp_key_fwd *key = &owner.u.r_key;
               
                  owner.protocol = APN_PROTO_RSVP;
                  key->afi = AFI_IP6;
                  key->len = sizeof (struct rsvp_key_ipv4_fwd);
                  key->u.ipv6.trunk_id = ilm->owner.u.r_key.u.ipv6.trunk_id;
                  key->u.ipv6.lsp_id = ilm->owner.u.r_key.u.ipv6.lsp_id;
                  IPV6_ADDR_COPY (&key->u.ipv6.egr,
                                  &ilm->owner.u.r_key.u.ipv6.egr);
                  IPV6_ADDR_COPY (&key->u.ipv6.ingr,
                                  &ilm->owner.u.r_key.u.ipv6.ingr);
                }

              ret = apn_mpls_ilm6_entry_add (APN_PROTO_NSM,
                                            &in_label,
                                            o_lbl,
                                            &ident_in,
                                            &ident_out,
                                            &p.u.prefix6,
                                            &p.prefixlen,
                                            &nhop,
                                            egress,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                           (struct ds_info_fwd *)&ilm->ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                                           opcode,
                                           tunnel_label == LABEL_VALUE_INVALID 
                                           ? NULL : &tunnel_label,
                                           &ilm->qos_resrc_id,
                                           ILM_NHLFE (ilm)->nhlfe_ix,
                                           &owner, vpn_id, vc_peer);

            }
          else
#endif
            {
              if (ilm->owner.owner == MPLS_RSVP)
                {
                  struct rsvp_key_fwd *key = &owner.u.r_key;
               
                  owner.protocol = APN_PROTO_RSVP;
                  key->afi = AFI_IP;
                  key->len = sizeof (struct rsvp_key_ipv4_fwd);
                  key->u.ipv4.trunk_id = ilm->owner.u.r_key.u.ipv4.trunk_id;
                  key->u.ipv4.lsp_id = ilm->owner.u.r_key.u.ipv4.lsp_id;
                  IPV4_ADDR_COPY (&key->u.ipv4.egr,
                                  &ilm->owner.u.r_key.u.ipv4.egr);
                  IPV4_ADDR_COPY (&key->u.ipv4.ingr,
                                  &ilm->owner.u.r_key.u.ipv4.ingr);
                }

              ret = apn_mpls_ilm4_entry_add (APN_PROTO_NSM,
                                            &in_label,
                                            o_lbl,
                                            &ident_in,
                                            &ident_out,
                                            &p.u.prefix4,
                                            &p.prefixlen,
                                            &nhop.u.ipv4,
                                            egress,
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
                                           (struct ds_info_fwd *)&ilm->ds_info,
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */
                                           opcode,
                                           tunnel_label == LABEL_VALUE_INVALID 
                                           ? NULL : &tunnel_label,
                                           &ilm->qos_resrc_id,
                                           ILM_NHLFE (ilm)->nhlfe_ix,
                                           &owner, vpn_id, vc_peer);
             }

          if (ret < 0)
            {
              zlog_warn (NSM_ZG, "Could not add %sILM Entry %d --> %d",
                         opcode_str, in_label, swap_label);
              return NSM_ERR_ILM_INSTALL_FAILURE;
            }
          else if (IS_NSM_DEBUG_EVENT)
            zlog_info (NSM_ZG, "Added/Modified %sILM Entry %d --> %d",
                       opcode_str, in_label, swap_label);

#ifdef HAVE_MPLS_FWD
          /* mplsInterfacePerfInLabelsUsed */
          nsm_mpls_if_stats_incr_in_labels_used (nm, ifp_in);
          /* mplsInterfacePerfOutLabelsUsed */
#ifdef HAVE_GMPLS
          if (oix != NSM_MPLS->loop_gindex)
#endif /* HAVE_GMPLS */
            nsm_mpls_if_stats_incr_out_labels_used (ifp_out);
#endif /* HAVE_MPLS_FWD */
        }
    }

  return NSM_SUCCESS;
}

/* Del FTNv6 entry from forwarder */
#ifdef HAVE_IPV6
int
nsm_mpls_ftn6_del_from_fwd (struct nsm_master *nm, struct ftn_entry *ftn, 
                           struct prefix *fec_prefix, u_int32_t vrf_id,
                           struct ftn_entry *tunnel_ftn)
{
  u_int32_t out_label;
  int ret;
  struct mpls_nh_fwd tunnel_nhop;
  struct nhlfe_key *nkey, *tkey;
  
  if (! ftn || ! fec_prefix)
    return NSM_FAILURE;

  nkey = (struct nhlfe_key *) &FTN_NHLFE (ftn)->nkey;

  if (nkey->afi != AFI_IP &&
#ifdef HAVE_IPV6
      nkey->afi != AFI_IP6
#endif
     )
    return NSM_SUCCESS;

  if (fec_prefix->family == AF_INET6)
    {
      if (nkey->afi == AFI_IP6)
        out_label = nkey->u.ipv6.out_label;
      else
        out_label = nkey->u.ipv4.out_label;

      if (out_label == LABEL_IMPLICIT_NULL)
        return NSM_SUCCESS;

      if (tunnel_ftn)
        tkey = (struct nhlfe_key *) &FTN_NHLFE (tunnel_ftn)->nkey;

      if (tunnel_ftn) 
        {
          if (tkey->afi == AFI_IP6)
            {
              if (tkey->u.ipv6.out_label != LABEL_IMPLICIT_NULL)
                {
                  tunnel_nhop.afi = AFI_IP6;
                  IPV6_ADDR_COPY (&tunnel_nhop.u.ipv6, 
                      &tkey->u.ipv6.nh_addr);
                }
            }
          else 
            {
              if (tkey->u.ipv4.out_label != LABEL_IMPLICIT_NULL)
                {
                  tunnel_nhop.afi = AFI_IP;
#ifdef HAVE_GMPLS
                  if (tkey->u.ipv4.nh_addr.s_addr == 0xFFFFFFFF)
                    pal_mem_set (&tunnel_nhop.u.ipv4, '\0', 
                        sizeof (struct pal_in4_addr));
                  else
#endif /* HAVE_GMPLS */ 
                    IPV4_ADDR_COPY (&tunnel_nhop.u.ipv4, &tkey->u.ipv4.nh_addr);
                }
            }
        }
      else
        {
          if (nkey->afi == AFI_IP6)
            {
              tunnel_nhop.afi = AFI_IP6;
              IPV6_ADDR_COPY (&tunnel_nhop.u.ipv6, &nkey->u.ipv6.nh_addr);
            }
          else
            {
              tunnel_nhop.afi = AFI_IP;
#ifdef HAVE_GMPLS
              if (nkey->u.ipv4.nh_addr.s_addr == 0xFFFFFFFF)
                pal_mem_set (&tunnel_nhop.u.ipv4, '\0', 
                             sizeof (struct pal_in4_addr));
              else
#endif /* HAVE_GMPLS */ 
               IPV4_ADDR_COPY (&tunnel_nhop.u.ipv4, &nkey->u.ipv4.nh_addr);
            }
        }

      ret = apn_mpls_ftn6_entry_del ((vrf_id == GLOBAL_FTN_ID
                                     ? GLOBAL_TABLE : vrf_id),
                                     APN_PROTO_NSM,
                                     &ftn->tun_id,
                                     &fec_prefix->u.prefix6,
                                     &fec_prefix->prefixlen,
                                     &ftn->dscp_in,
                                     FTN_NHLFE (ftn)->nhlfe_ix,
                                     &tunnel_nhop,
                                     ftn->ftn_ix);

      if (ret < 0)
        {
          zlog_warn (NSM_ZG, "Could not remove %s entry %r",
                     (vrf_id == GLOBAL_FTN_ID ? "FTN" : "VRF"),
                     &fec_prefix->u.prefix6);
          return ret;
        }
      else if (IS_NSM_DEBUG_EVENT)
        zlog_info (NSM_ZG, "Successfully removed %s entry %R --> %d",
                   (vrf_id == GLOBAL_FTN_ID ? "FTN" : "VRF"),
                   &fec_prefix->u.prefix6, out_label);

#ifdef HAVE_MPLS_FWD
        {
          struct interface *ifp;
          u_int32_t ifindex;
          /* mplsInterfacePerfOutLabelsUsed */
          if (nkey->afi == AFI_IP6)
            ifindex = nkey->u.ipv6.oif_ix;
          else
            ifindex = nkey->u.ipv4.oif_ix;
          NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_GMPLS),
                                  ifindex, ifp);

          if (ifp)
            nsm_mpls_if_stats_decr_out_labels_used (ifp);
        }
#endif /* HAVE_MPLS_FWD */
    }
  
  return NSM_SUCCESS;
}

#endif

/* Del FTN entry from Forwarder. */
int
nsm_mpls_ftn4_del_from_fwd (struct nsm_master *nm, struct ftn_entry *ftn, 
                           struct prefix *fec_prefix, u_int32_t vrf_id,
                           struct ftn_entry *tunnel_ftn)
{
  u_int32_t out_label;
  int ret = NSM_SUCCESS;
  struct pal_in4_addr *tunnel_nhop;
  struct nhlfe_key *nkey, *tkey;
  
  if (! ftn || ! fec_prefix)
    return NSM_FAILURE;

#ifdef HAVE_GELS
    if((ftn->tgen_data) && 
       (((struct gmpls_tgen_data *)ftn->tgen_data)->gmpls_type == 
                                                   gmpls_entry_type_pbb_te))
#if defined HAVE_I_BEB && defined HAVE_B_BEB
    {
      struct interface *ifp = NULL;
      struct nhlfe_key_gen *nkey1;

      nkey1= (struct nhlfe_key_gen *) ftn->xc->nhlfe->nkey;

      /* Get the interface structure from the ifindex */
      NSM_MPLS_GET_INDEX_PTR (PAL_TRUE, nkey1->u.pbb.oif_ix, ifp);

      /* Call the PBB-TE API */
      ret = nsm_pbb_te_delete_dynamic_esp(ifp,
                ((struct gmpls_tgen_data *)ftn->tgen_data)->u.pbb.tesid,
                nkey1->u.pbb.lbl.bmac,
                nkey1->u.pbb.lbl.bvid, PAL_TRUE);
      return ret;
    }
    else
#else
    {
      return ret;
    }
#endif /* HAVE_I_BEB && HAVE_B_BEB */
#endif /* HAVE_GELS */

  /* Dummy entries needn't be deleted */
  if (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_DUMMY))
    return NSM_SUCCESS;

  nkey = (struct nhlfe_key *) &FTN_NHLFE (ftn)->nkey;

  if (nkey->afi != AFI_IP)
    return NSM_SUCCESS;

   if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Receive FTN entry delete request %r",
                &fec_prefix->u.prefix4);

  out_label = nkey->u.ipv4.out_label;

  if (out_label == LABEL_IMPLICIT_NULL)
    return NSM_SUCCESS;

  if (tunnel_ftn)
    tkey = (struct nhlfe_key *) &FTN_NHLFE (tunnel_ftn)->nkey;

  if (tunnel_ftn && 
      tkey->u.ipv4.out_label != LABEL_IMPLICIT_NULL)
    {
#ifdef HAVE_GMPLS
      if (tkey->u.ipv4.nh_addr.s_addr == 0xFFFFFFFF)
        tunnel_nhop = NULL;
      else
#endif /* HAVE_GMPLS */ 
        tunnel_nhop = &tkey->u.ipv4.nh_addr;
    }
  else
    {
#ifdef HAVE_GMPLS
      if (nkey->u.ipv4.nh_addr.s_addr == 0xFFFFFFFF)
        tunnel_nhop = NULL;
      else
#endif /* HAVE_GMPLS */ 
      tunnel_nhop = &nkey->u.ipv4.nh_addr; 
    }

  ret = apn_mpls_ftn4_entry_del ((vrf_id == GLOBAL_FTN_ID
                                ? GLOBAL_TABLE : vrf_id),
                                APN_PROTO_NSM,
                                &ftn->tun_id,
                                &fec_prefix->u.prefix4,
                                &fec_prefix->prefixlen,
                                &ftn->dscp_in,
                                FTN_NHLFE (ftn)->nhlfe_ix,
                                tunnel_nhop,
                                ftn->ftn_ix);

  if (ret < 0)
    {
      zlog_warn (NSM_ZG, "Could not remove %s entry %r",
                 (vrf_id == GLOBAL_FTN_ID ? "FTN" : "VRF"),
                 &fec_prefix->u.prefix4);
      return ret;
    }
  else if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Successfully removed %s entry %r --> %d",
               (vrf_id == GLOBAL_FTN_ID ? "FTN" : "VRF"),
               &fec_prefix->u.prefix4, out_label);

#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
  /* If BFD was enabled for this entry, send BFD session Delete. */
  if (CHECK_FLAG (ftn->bfd_flag, NSM_MPLS_BFD_ENABLED))
    nsm_mpls_ftn_bfd_delete (nm, ftn);
#endif /* HAVE_BFD && HAVE_MPLS_OAM */

/* Send FTN Update to OAMD. */
#ifdef HAVE_MPLS_OAM
  nsm_mpls_oam_send_ftn_update (nm, ftn);
  ftn->is_oam_enabled = PAL_FALSE;
#endif /* HAVE_MPLS_OAM */

#ifdef HAVE_MPLS_FWD
    {
      struct interface *ifp = NULL;
      u_int32_t ifindex;

      /* mplsInterfacePerfOutLabelsUsed */
      ifindex = nkey->u.ipv4.oif_ix;
      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ftn->flags, FTN_ENTRY_FLAG_GMPLS),
                              ifindex, ifp);
      if (ifp)
        nsm_mpls_if_stats_decr_out_labels_used (ifp);
    }
#endif /* HAVE_MPLS_FWD */

  return NSM_SUCCESS;
}

/* Del ILM entry from Forwarder. */
int
nsm_mpls_ilm_del_from_fwd (struct nsm_master *nm, struct ilm_entry *ilm)
{
  struct interface *ifp = NULL;
  struct if_ident ident;
  int ret;
  struct nhlfe_key *nkey;

  if (! ilm || ! (ilm->xc))
    return NSM_FAILURE;

  nkey = (struct nhlfe_key *) ILM_NHLFE (ilm)->nkey;
  if (nkey->afi != AFI_IP)
    return NSM_SUCCESS;

  /* Get interface data. */
  NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_GMPLS),
                          ilm->key.u.pkt.iif_ix, ifp);
  if (ifp)
    {
      /* Fill ident. */
      pal_mem_set (&ident, 0, sizeof (struct if_ident));
      ident.if_index = ifp->ifindex;
      pal_mem_cpy (ident.if_name, ifp->name, INTERFACE_NAMSIZ);

      ret = apn_mpls_ilm_entry_del (APN_PROTO_NSM,
                                    &ilm->key.u.pkt.in_label,
                                    &ident);
      if (ret < 0)
        {
          zlog_warn (NSM_ZG, "Could not remove ILM Entry %d",
                     ilm->key.u.pkt.in_label);
          return ret;
        }
      else if (IS_NSM_DEBUG_EVENT)
        zlog_info (NSM_ZG, "Successfully removed ILM Entry %d",
                   ilm->key.u.pkt.in_label);

#ifdef HAVE_MPLS_FWD
      {
        struct interface *ifp_out = NULL;
        u_int32_t ifindex;

        /* mplsInterfacePerfInLabelsUsed */
        if (ifp)
          nsm_mpls_if_stats_decr_in_labels_used (nm, ifp);
        /* mplsInterfacePerfOutLabelsUsed */
#ifdef HAVE_IPV6
        if (nkey->afi == AFI_IP6)
          ifindex = nkey->u.ipv6.oif_ix;
        else
#endif
          ifindex = nkey->u.ipv4.oif_ix;
        NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (ilm->flags, ILM_ENTRY_FLAG_GMPLS),
                                 ifindex, ifp_out);
        if (ifp_out)
          nsm_mpls_if_stats_decr_out_labels_used (ifp_out);
      }
#endif /* HAVE_MPLS_FWD */
    }

  return NSM_SUCCESS;
}


#ifdef HAVE_MPLS_VC

/* return NSM_TRUE means need install to fib 
   return NSM_FALSE means do not install to fib */
int
nsm_mpls_vc_fib_add_check (struct nsm_master *nm, struct nsm_mpls_circuit *vc)
{
  struct nsm_mpls_circuit *s_vc = NULL;
  struct nsm_mpls_vc_info *s_info = vc->vc_info->sibling_info;
  int ret;

  /* Get sibling */
  if (s_info)
    s_vc = s_info->u.vc;

  /* 1. If it is qualified 
        1) if it is primary and if sibling is installed, 
           delete sibling from fib 
        2) install it to fib by return NSM_TRUE */
  if (PW_STATUS_INSTALL_CHECK (vc->pw_status, vc->remote_pw_status))
    {
      if (! CHECK_FLAG (vc->vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY) &&
          s_vc && s_vc->vc_fib && s_vc->vc_fib->install_flag == NSM_TRUE)
        ret = nsm_mpls_vc_fib_del (nm, s_vc, s_vc->id);

      return NSM_TRUE;
    }
 
  /* 2. If it is secondary and qualified by clear local standby bit:
        1) if primary installed, do not install this secondary 
        2) if primary not installed, 
           in non-revertive mode set primary local standby bit, 
           clear secondary standby bit and install secondary */
  if (CHECK_FLAG (vc->vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY) &&
      nsm_mpls_vc_secondary_qualified (vc))
    {
      if (s_vc && s_vc->vc_fib && s_vc->vc_fib->install_flag == NSM_TRUE)
        return NSM_FALSE;

      if (! CHECK_FLAG (vc->vc_info->conf_flag, NSM_MPLS_VC_CONF_REVERTIVE) && 
          s_info)
        {
          s_info->run_mode = NSM_MPLS_VC_STANDBY;
          if (s_vc)
            {
              SET_FLAG (s_vc->pw_status, NSM_CODE_PW_STANDBY); 
              /* send pw-status to signaling protocol */
              nsm_mpls_l2_circuit_send_pw_status (s_vc->vc_info->mif, 
                                                  s_vc);
            }
        }

      vc->vc_info->run_mode = NSM_MPLS_VC_ACTIVE;
      UNSET_FLAG (vc->pw_status, NSM_CODE_PW_STANDBY);
      return NSM_TRUE;
    }

  return NSM_FALSE;
}

void
nsm_mpls_vc_fib_del_check (struct nsm_mpls_circuit *vc)
{
  struct nsm_mpls_vc_info *s_info = NULL;
  struct nsm_mpls_circuit *s_vc = NULL;

  if (! vc || ! vc->vc_info)
    return;

  s_info = vc->vc_info->sibling_info;
  if (s_info)
    s_vc = s_info->u.vc;

  /* if secondary gets deleted from fib, set standby bit, 
     if primary standby is set, clear it. */
  if (CHECK_FLAG (vc->vc_info->conf_flag, NSM_MPLS_VC_CONF_SECONDARY))
    {
      vc->vc_info->run_mode = NSM_MPLS_VC_STANDBY;
      SET_FLAG (vc->pw_status, NSM_CODE_PW_STANDBY);
      if (s_info)
        {
          s_info->run_mode = NSM_MPLS_VC_ACTIVE;
          if (s_vc)
            {
              UNSET_FLAG (s_vc->pw_status, NSM_CODE_PW_STANDBY);
              /* send pw-status to signaling protocol */
              nsm_mpls_l2_circuit_send_pw_status (s_vc->vc_info->mif, 
                                                  s_vc);
            }
        }
    }
}

/* Send message to next level for the fib addition. The fib add call used is 
 * the same for VPLS and for the VC's. Hence a common API is used for the same
 */
int
nsm_mpls_vc_fib_add (struct nsm_master *nm, struct nsm_mpls_circuit *vc,
                     u_int32_t vc_id, u_char opcode,
                     struct ftn_entry *tunnel_ftn)
{
  int ret = NSM_SUCCESS;
  u_int32_t tunnel_label, tunnel_ftnix;
  struct mpls_nh_fwd tunnel_nhop, peer_nhop;
  struct nhlfe_key *tkey;
  u_int32_t tunnel_oix, tunnel_nhlfe_ix;
  struct interface *ifp_in, *ifp_out, *ifp_tunnel = NULL;
  struct if_ident ident_in, ident_out, ident_tunnel;
  struct prefix p;
  int is_ms_pw = NSM_FALSE;
  u_int32_t nw_if_ix, ac_if_ix;

  if (! vc)
    return NSM_FAILURE;

#ifdef HAVE_MS_PW
  if (vc->ms_pw)
    is_ms_pw = NSM_TRUE;
#endif /* HAVE_MS_PW */

  vc->vc_fib->opcode = opcode;

  tunnel_label = LABEL_VALUE_INVALID;
  tunnel_ftnix = 0;
  tunnel_oix = 0;
  tunnel_nhlfe_ix = 0;

  /* Prefix Address for refering the ILM entry */
  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.u.prefix4.s_addr = pal_hton32 (vc_id);

  if (tunnel_ftn)
    {
      tunnel_ftnix = tunnel_ftn->ftn_ix;
      tkey = (struct nhlfe_key *) &FTN_NHLFE (tunnel_ftn)->nkey;

#ifdef HAVE_IPV6
      if (tkey->afi == AFI_IP6)
        {
          peer_nhop.afi = AFI_IP6;
          IPV6_ADDR_COPY (&peer_nhop.u.ipv6, 
                          &tkey->u.ipv6.nh_addr);

          if (tkey->u.ipv6.out_label != 
              LABEL_IMPLICIT_NULL &&
              tkey->u.ipv6.out_label != 
              LABEL_IPV4_EXP_NULL)
            {
              tunnel_label = tkey->u.ipv6.out_label;
              tunnel_nhop.afi = AFI_IP6;
              IPV6_ADDR_COPY (&tunnel_nhop.u.ipv6, 
                              &tkey->u.ipv6.nh_addr);
              tunnel_oix = tkey->u.ipv6.oif_ix;
              tunnel_nhlfe_ix = FTN_NHLFE (tunnel_ftn)->nhlfe_ix;

              NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (tunnel_ftn->flags, 
                                      FTN_ENTRY_FLAG_GMPLS),
                                      tunnel_oix, ifp_tunnel);
              if (! ifp_tunnel)
                return NSM_FAILURE;

              pal_mem_set (&ident_tunnel, 0, sizeof (struct if_ident));
              ident_tunnel.if_index = ifp_tunnel->ifindex;
              pal_mem_cpy (ident_tunnel.if_name, ifp_tunnel->name, 
                           INTERFACE_NAMSIZ);
            }
        }
      else
#endif
        {
          peer_nhop.afi = AFI_IP;
          IPV4_ADDR_COPY (&peer_nhop.u.ipv4, 
                          &tkey->u.ipv4.nh_addr);
          
          tunnel_label = tkey->u.ipv4.out_label;

          if (tunnel_label != LABEL_IMPLICIT_NULL &&
              tunnel_label != LABEL_IPV4_EXP_NULL)
            {
              tunnel_nhop.afi = AFI_IP;
              IPV4_ADDR_COPY (&tunnel_nhop, 
                              &tkey->u.ipv4.nh_addr);
              tunnel_oix = tkey->u.ipv4.oif_ix;
              tunnel_nhlfe_ix = FTN_NHLFE (tunnel_ftn)->nhlfe_ix;

              NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (tunnel_ftn->flags,
                                      FTN_ENTRY_FLAG_GMPLS),
                                      tunnel_oix, ifp_tunnel);

              if (! ifp_tunnel)
                return NSM_FAILURE;

              pal_mem_set (&ident_tunnel, 0, sizeof (struct if_ident));
              ident_tunnel.if_index = ifp_tunnel->ifindex;
              pal_mem_cpy (ident_tunnel.if_name, ifp_tunnel->name, 
                           INTERFACE_NAMSIZ);
            }
        }
    }
  else
    {
      peer_nhop.afi = AFI_IP;
      IPV4_ADDR_COPY (&peer_nhop.u.ipv4, &vc->address.u.prefix4);
    }

  if ((vc->vc_fib->out_label == LABEL_IMPLICIT_NULL) ||
      (vc->vc_fib->out_label == LABEL_IPV4_EXP_NULL))
    return NSM_SUCCESS;
  
  /* Get incoming interface. */
  NSM_MPLS_GET_INDEX_PTR (PAL_FALSE, vc->vc_fib->ac_if_ix, ifp_in);

  if (! ifp_in)
    return NSM_FAILURE;

  /* Get outgoing interface. */
#ifdef HAVE_GMPLS
  if (tunnel_ftn)
    {
      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (tunnel_ftn->flags, FTN_ENTRY_FLAG_GMPLS), 
                              vc->vc_fib->nw_if_ix, ifp_out);
    }
  else
#endif /* HAVE_GMPLS */
    NSM_MPLS_GET_INDEX_PTR (PAL_FALSE, vc->vc_fib->nw_if_ix, ifp_out);

  if (! ifp_out)
    return NSM_FAILURE;

#ifdef HAVE_GMPLS
  nw_if_ix = nsm_gmpls_get_ifindex_from_gifindex(nm, vc->vc_fib->nw_if_ix);
  ac_if_ix = nsm_gmpls_get_ifindex_from_gifindex(nm, vc->vc_fib->ac_if_ix);
#else /* HAVE_GMPLS */
  nw_if_ix = vc->vc_fib->nw_if_ix;
  ac_if_ix = vc->vc_fib->ac_if_ix;
#endif /* HAVE_GMPLS */

  /* Fill ident_in. */
  pal_mem_set (&ident_in, 0, sizeof (struct if_ident));
  ident_in.if_index = ifp_in->ifindex;
  pal_mem_cpy (ident_in.if_name, ifp_in->name, INTERFACE_NAMSIZ);

  /* Fill ident_out. */
  pal_mem_set (&ident_out, 0, sizeof (struct if_ident));
  ident_out.if_index = ifp_out->ifindex;
  pal_mem_cpy (ident_out.if_name, ifp_out->name, INTERFACE_NAMSIZ);

  /* Before install to fib, need check if it is qualified to install to fib */
  if (! nsm_mpls_vc_fib_add_check (nm, vc))
    {
      /* send pw-state to protocol */
      nsm_mpls_l2_circuit_send_pw_status (vc->vc_info->mif, vc);
      return NSM_SUCCESS;
    }

  /* vc binding in HW is done only for non S-PE nodes */
  if (!is_ms_pw)
    ret = nsm_mpls_l2_circuit_add_to_fwd (vc->id, vc->vc_info->mif->ifp, 
                                          vc->vlan_id);
  if (ret < 0)
    return NSM_FAILURE;

#ifdef HAVE_MS_PW
  /* Check if both the vcs of the ms-pw pair have all the data */
  if (is_ms_pw)
    {
      /* Copy temp data for this VC */
      pal_mem_cpy(&vc->vc_fib->vc_fib_data_temp.if_in, &ident_in,
                  sizeof(struct if_ident));
      pal_mem_cpy(&vc->vc_fib->vc_fib_data_temp.if_out, &ident_out,
                  sizeof(struct if_ident));
      IPV4_ADDR_COPY(&vc->vc_fib->vc_fib_data_temp.peer_nhop_addr,
                      &peer_nhop.u.ipv4);
      IPV4_ADDR_COPY(&vc->vc_fib->vc_fib_data_temp.fec_addr, &p.u.prefix4);
      vc->vc_fib->vc_fib_data_temp.fec_prefixlen = p.prefixlen;
      vc->vc_fib->vc_fib_data_temp.tunnel_label = tunnel_label;
      IPV4_ADDR_COPY(&vc->vc_fib->vc_fib_data_temp.tunnel_nhop,
                     &tunnel_nhop.u.ipv4);
      vc->vc_fib->vc_fib_data_temp.tunnel_oix = tunnel_oix;
      vc->vc_fib->vc_fib_data_temp.tunnel_nhlfe_ix = tunnel_nhlfe_ix;
      vc->vc_fib->vc_fib_data_temp.tunnel_ftnix = tunnel_ftnix;
      vc->vc_fib->vc_fib_data_temp.data_avail = NSM_TRUE;

      /* Check if all required data is present for OTHER vc.
       * Return if data missing.
       */
      if (vc->vc_other)
        {
          if (vc->vc_other->vc_fib)
            {
              if (vc->vc_other->vc_fib->vc_fib_data_temp.data_avail ==
                                                                   NSM_FALSE)
                return NSM_SUCCESS;
            }
          else /* The other vc fib is to be created */
            return NSM_SUCCESS;
        }
      else 
        return NSM_FAILURE;

      /* Since we are stitching the two VCs, the incoming params will be same
       * for the vc, but out going params will have to be obtained from the
       * VCFIB of the other VC.
       */
      ret = apn_mpls_vc4_fib_add (vc_id,
                                  MPLS_VC_STYLE_MARTINI,
                                  NULL,
                                  &vc->vc_fib->in_label,
                                  &vc->vc_other->vc_fib->out_label,
                                  &nw_if_ix,
                                  &ac_if_ix,
                                  &vc->vc_other->vc_fib->vc_fib_data_temp.if_in,                                  &vc->vc_other->vc_fib->vc_fib_data_temp.\
                                  if_out,
                                  opcode,
                                  &vc->vc_other->address.u.prefix4,
                                  &vc->vc_other->vc_fib->vc_fib_data_temp.\
                                   peer_nhop_addr,
                                  &vc->vc_other->vc_fib->vc_fib_data_temp.\
                                   fec_addr,
                                  &vc->vc_other->vc_fib->vc_fib_data_temp.\
                                   fec_prefixlen,
                                  &vc->vc_other->vc_fib->vc_fib_data_temp.\
                                   tunnel_label,
                                  &vc->vc_other->vc_fib->vc_fib_data_temp.\
                                   tunnel_nhop,
                                  &vc->vc_other->vc_fib->vc_fib_data_temp.\
                                   tunnel_oix,
                                  &vc->vc_other->vc_fib->vc_fib_data_temp.\
                                    tunnel_nhlfe_ix,
                                  &vc->vc_other->vc_fib->vc_fib_data_temp.\
                                    tunnel_ftnix,
                                  is_ms_pw);

      if (ret < 0)
        {
          zlog_warn (NSM_ZG, "ERROR: MPLS Virtual Circuit fib install failure");
          return NSM_ERR_FTN_INSTALL_FAILURE;
        }
      vc->pw_status = NSM_CODE_PW_FORWARDING;
      vc->vc_fib->install_flag = NSM_TRUE;

      /* Adding the FIB entry of the OTHER VC of a MSPW */
      ret = apn_mpls_vc4_fib_add (vc->vc_other->id,
                                  MPLS_VC_STYLE_MARTINI,
                                  NULL,
                                  &vc->vc_other->vc_fib->in_label,
                                  &vc->vc_fib->out_label,
                                  &ac_if_ix,
                                  &nw_if_ix,
                                  &vc->vc_fib->vc_fib_data_temp.if_in,
                                  &vc->vc_fib->vc_fib_data_temp.if_out,
                                  opcode,
                                  &vc->address.u.prefix4,
                                  &vc->vc_fib->vc_fib_data_temp.peer_nhop_addr,
                                  &vc->vc_fib->vc_fib_data_temp.fec_addr,
                                  &vc->vc_fib->vc_fib_data_temp.fec_prefixlen,
                                  &vc->vc_fib->vc_fib_data_temp.tunnel_label,
                                  &vc->vc_fib->vc_fib_data_temp.tunnel_nhop,
                                  &vc->vc_fib->vc_fib_data_temp.tunnel_oix,
                                  &vc->vc_fib->vc_fib_data_temp.tunnel_nhlfe_ix,
                                  &vc->vc_fib->vc_fib_data_temp.tunnel_ftnix,
                                  is_ms_pw);
      if (ret < 0)
        {
          zlog_warn (NSM_ZG, "ERROR: MPLS Virtual Circuit fib install failure");
          return NSM_ERR_FTN_INSTALL_FAILURE;
        }
      vc->vc_other->pw_status = NSM_CODE_PW_FORWARDING;
      vc->vc_other->vc_fib->install_flag = NSM_TRUE;

      /* send pw-state to protocol */
      if (vc->fec_type_vc != PW_OWNER_MANUAL)
        nsm_mpls_l2_circuit_send_pw_status (vc->vc_info->mif, vc);

      /* send pw-state of the other VC to protocol */
      if (vc->vc_other->fec_type_vc != PW_OWNER_MANUAL)
        nsm_mpls_l2_circuit_send_pw_status (vc->vc_other->vc_info->mif, 
                                            vc->vc_other);
    }
  else
#endif /* HAVE_MS_PW */
    {
#ifdef HAVE_IPV6
      if (p.family == AF_INET6)
        ret = apn_mpls_vc6_fib_add (vc_id, 
                                    MPLS_VC_STYLE_MARTINI, 
                                    NULL,
                                    &vc->vc_fib->in_label,
                                    &vc->vc_fib->out_label,
                                    &ac_if_ix,
                                    &nw_if_ix,
                                    &ident_in,
                                    &ident_out,
                                    opcode,
                                    &vc->address.u.prefix4,
                                    &peer_nhop,
                                    &p.u.prefix6,
                                    &p.prefixlen,
                                    &tunnel_label,
                                    &tunnel_nhop,
                                    &tunnel_oix,
                                    &tunnel_nhlfe_ix,
                                    &tunnel_ftnix);

      else
#endif
        ret = apn_mpls_vc4_fib_add (vc_id,
                                    MPLS_VC_STYLE_MARTINI, 
                                    NULL,
                                    &vc->vc_fib->in_label,
                                    &vc->vc_fib->out_label,
                                    &ac_if_ix, 
                                    &nw_if_ix,
                                    &ident_in,
                                    &ident_out,
                                    opcode,
                                    &vc->address.u.prefix4,
                                    &peer_nhop.u.ipv4,
                                    &p.u.prefix4,
                                    &p.prefixlen,
                                    &tunnel_label,
                                    &tunnel_nhop.u.ipv4,
                                    &tunnel_oix,
                                    &tunnel_nhlfe_ix,
                                    &tunnel_ftnix,
                                    is_ms_pw);

      if (ret < 0)
        {
          zlog_warn (NSM_ZG, "ERROR: MPLS Virtual Circuit fib install failure");
          return NSM_ERR_FTN_INSTALL_FAILURE;
        }
      vc->pw_status = NSM_CODE_PW_FORWARDING;
      vc->vc_fib->install_flag = NSM_TRUE;

      /* send pw-state to protocol */
      nsm_mpls_l2_circuit_send_pw_status (vc->vc_info->mif, vc);
    }

#if defined (HAVE_BFD) && defined (HAVE_VCCV)
  /* If BFD is configured for this VC entry, send BFD session Add. */
  if (oam_util_get_bfd_cvtype_in_use(vc->cv_types, vc->vc_fib->remote_cv_types) 
                                                                != CV_TYPE_NONE)
    {
       nsm_mpls_vc_bfd_session_add (nm, vc);
    }
#endif /* HAVE_BFD && HAVE_VCCV */

/* Send VC Update to OAMD. */
#ifdef HAVE_MPLS_OAM
  nsm_mpls_oam_send_vc_update (nm, vc);
  vc->vc_fib->is_oam_enabled = PAL_FALSE;
#endif /* HAVE_MPLS_OAM */

  return NSM_SUCCESS;
}


int
nsm_mpls_vc_fib_del (struct nsm_master *nm, struct nsm_mpls_circuit *vc,
                     u_int32_t vc_id)
{
  struct interface *ifp_in, *ifp_out;
  struct pal_in4_addr *nh_addr;
  struct if_ident if_in, if_out;
  int ret;
  int is_ms_pw = NSM_FALSE;
  u_int32_t nw_if_ix, ac_if_ix;

  if (! vc || ! vc->vc_fib)
    return NSM_FAILURE;

  if (vc->vc_fib->install_flag != NSM_TRUE)
    return NSM_SUCCESS;

  vc->vc_fib->install_flag = NSM_FALSE;

#ifdef HAVE_MS_PW
  if (vc->ms_pw)
    is_ms_pw = NSM_TRUE;
#endif /* HAVE_MS_PW */

  nsm_mpls_vc_fib_del_check (vc);

  /* Get incoming interface of VC */
  NSM_MPLS_GET_INDEX_PTR (PAL_FALSE, vc->vc_fib->ac_if_ix, ifp_in);
  if (! ifp_in)
    return NSM_FAILURE;

  /* Get outgoing interface of VC */
#ifdef HAVE_GMPLS
  if (vc->ftn)
    {
      NSM_MPLS_GET_INDEX_PTR (CHECK_FLAG (vc->ftn->flags, FTN_ENTRY_FLAG_GMPLS), 
                              vc->vc_fib->nw_if_ix, ifp_out);
    }
  else
#endif /* HAVE_GMPLS */
    NSM_MPLS_GET_INDEX_PTR (PAL_FALSE, vc->vc_fib->nw_if_ix, ifp_out); 

  if (! ifp_out)
    return NSM_FAILURE;

#ifdef HAVE_GMPLS
  nw_if_ix = nsm_gmpls_get_ifindex_from_gifindex(nm, vc->vc_fib->nw_if_ix);
  ac_if_ix = nsm_gmpls_get_ifindex_from_gifindex(nm, vc->vc_fib->ac_if_ix);
#else /* HAVE_GMPLS */
  nw_if_ix = vc->vc_fib->nw_if_ix;
  ac_if_ix = vc->vc_fib->ac_if_ix;
#endif /* HAVE_GMPLS */


  /* Fill ident_in. */
  pal_mem_set (&if_in, 0, sizeof (struct if_ident));
  if_in.if_index = ifp_in->ifindex;
  pal_mem_cpy (if_in.if_name, ifp_in->name, INTERFACE_NAMSIZ);

  /* Fill ident_out. */
  pal_mem_set (&if_out, 0, sizeof (struct if_ident));
  if_out.if_index = ifp_out->ifindex;
  pal_mem_cpy (if_out.if_name, ifp_out->name, INTERFACE_NAMSIZ);

  if (!is_ms_pw)
    {
      nh_addr = &vc->address.u.prefix4;

      ret = apn_mpls_vc_fib_delete (vc_id,
                                    MPLS_VC_STYLE_MARTINI,
                                    NULL,
                                    &vc->vc_fib->in_label,
                                    &vc->vc_fib->out_label,
                                    &ac_if_ix,
                                    &nw_if_ix,
                                    &if_in,
                                    &if_out,
                                    nh_addr, 
                                    is_ms_pw);
      if (ret < 0)
        {
          zlog_warn (NSM_ZG, "Could not remove VC FIB entry for interface %s",
                     ifp_in->name);
          return ret;
        }
      else if (IS_NSM_DEBUG_EVENT)
        zlog_info (NSM_ZG, "Successfully removed FIB entry for interface %s",
                   ifp_in->name);

      nsm_mpls_l2_circuit_del_from_fwd (vc->id, vc->vc_info->mif->ifp, 
                                        vc->vlan_id);

      SET_FLAG (vc->pw_status, NSM_CODE_PW_NOT_FORWARDING);

      /* send pw-state to protocol */
      nsm_mpls_l2_circuit_send_pw_status (vc->vc_info->mif, vc);

      /* Send VC Update to OAMD. */
#ifdef HAVE_MPLS_OAM
      nsm_mpls_oam_send_vc_update (nm, vc);
      vc->vc_fib->is_oam_enabled = PAL_FALSE;
#endif /* HAVE_MPLS_OAM */

#if defined (HAVE_BFD) && defined (HAVE_VCCV)
      /* If BFD was enabled for this VC entry, send BFD session Delete. */
      if (vc->is_bfd_running)
        nsm_mpls_vc_bfd_session_delete (nm, vc, PAL_FALSE);
#endif /* HAVE_BFD && HAVE_VCCV */
    }
#ifdef HAVE_MS_PW
  else
    {
      if (!vc->vc_other)
        return NSM_FAILURE;

      ret = apn_mpls_vc_fib_delete (vc_id,
                                    MPLS_VC_STYLE_MARTINI,
                                    NULL,
                                    &vc->vc_fib->in_label,
                                    &vc->vc_other->vc_fib->out_label,
                                    &ac_if_ix,
                                    &nw_if_ix,
                                    &if_out,
                                    &if_in,
                                    &vc->vc_other->address.u.prefix4,
                                    is_ms_pw);
       if (ret < 0)        
         {          
           zlog_warn (NSM_ZG, "Could not remove VC FIB entry for interface %s",
                      ifp_out->name);
           return ret;        
         }      
       else if (IS_NSM_DEBUG_EVENT)        
         zlog_info (NSM_ZG, "Successfully removed FIB entry for interface %s",
                    ifp_out->name);
       
       ret = apn_mpls_vc_fib_delete (vc->vc_other->id,
                                     MPLS_VC_STYLE_MARTINI,
                                     NULL,
                                     &vc->vc_other->vc_fib->in_label,
                                     &vc->vc_fib->out_label,
                                     &ac_if_ix,
                                     &nw_if_ix,
                                     &if_in,
                                     &if_out,
                                     &vc->address.u.prefix4,
                                     is_ms_pw);
        if (ret < 0)
          { 
            zlog_warn (NSM_ZG, "Could not remove VC FIB entry for interface %s",
                       ifp_in->name);
            return ret;
          } 
        else if (IS_NSM_DEBUG_EVENT)
          zlog_info (NSM_ZG, "Successfully removed FIB entry for interface %s",
                     ifp_in->name);
          
        SET_FLAG (vc->pw_status, NSM_CODE_PW_NOT_FORWARDING);
        SET_FLAG (vc->vc_other->pw_status, NSM_CODE_PW_NOT_FORWARDING);
          
        /* send pw-state to protocol */
        if (vc->fec_type_vc != PW_OWNER_MANUAL)
          nsm_mpls_l2_circuit_send_pw_status (vc->vc_info->mif, vc);
        
        if (vc->vc_other->fec_type_vc != PW_OWNER_MANUAL)        
          nsm_mpls_l2_circuit_send_pw_status (vc->vc_other->vc_info->mif,                                                     
                                              vc->vc_other);      
        /* Send VC Update to OAMD. */
#ifdef HAVE_MPLS_OAM      
        nsm_mpls_oam_send_vc_update (nm, vc);
        vc->vc_fib->is_oam_enabled = PAL_FALSE;
        nsm_mpls_oam_send_vc_update (nm, vc->vc_other);
        vc->vc_other->vc_fib->is_oam_enabled = PAL_FALSE;
#endif /* HAVE_MPLS_OAM */

#if defined (HAVE_BFD) && defined (HAVE_VCCV)      
/* If BFD was enabled for this VC entry, send BFD session Delete. */      
        if (vc->is_bfd_running)
          nsm_mpls_vc_bfd_session_delete (nm, vc, PAL_FALSE);
        if (vc->vc_other->is_bfd_running)        
          nsm_mpls_vc_bfd_session_delete (nm, vc->vc_other, PAL_FALSE);
#endif /* HAVE_BFD && HAVE_VCCV */
    }
#endif /* HAVE_MS_PW */

  return NSM_SUCCESS;
}
#endif /* HAVE_MPLS_VC */
