/* Copyright (C) 2008  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_BFD

#include "lib.h"
#include "bfd_client_api.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_vrf.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#include "ospfd/ospf_neighbor.h"
#include "ospfd/ospf_nfsm.h"
#ifdef HAVE_RESTART
#include "ospfd/ospf_restart.h"
#endif /* HAVE_RESTART */
#include "ospfd/ospf_bfd.h"

static bool_t
ospf_bfd_add_session (struct ospf_neighbor *nbr, bool_t admin_flag)
{
  struct ospf_interface *oi = nbr->oi;
  struct apn_vrf *iv = oi->top->ov->iv;
  struct apn_vr *vr = oi->top->om->vr;
  struct bfd_client_api_session s;
  bool_t ret;

  /* Sanity check. */
  if ((!CHECK_FLAG (oi->flags, OSPF_IF_BFD) && !(admin_flag)) || 
       nbr->state <= NFSM_TwoWay)
    return PAL_FALSE;

  pal_mem_set (&s, 0, sizeof (struct bfd_client_api_session));
  s.zg = ZG;
  s.vr_id = vr->id;
  s.vrf_id = iv->id;
  s.afi = AFI_IP;
  if (admin_flag)
    SET_FLAG (s.flags, BFD_CLIENT_API_FLAG_AD);
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    {
      /* For Multihop session no ifindex binding should be there */
      s.ifindex = 0;
      SET_FLAG (s.flags, BFD_CLIENT_API_FLAG_MH);
    }
  else
    s.ifindex = oi->u.ifp->ifindex;
  s.addr.ipv4.src = oi->ident.address->u.prefix4;
  s.addr.ipv4.dst = nbr->ident.address->u.prefix4;
  ret = bfd_client_api_session_add (&s);
  if (ret == LIB_API_SUCCESS)
    SET_FLAG (nbr->flags, OSPF_NEIGHBOR_BFD);
  return ret;
}

static bool_t
ospf_bfd_delete_session (struct ospf_neighbor *nbr, bool_t admin_flag)
{
  struct ospf_interface *oi = nbr->oi;
  struct apn_vrf *iv = oi->top->ov->iv;
  struct apn_vr *vr = oi->top->om->vr;
  struct bfd_client_api_session s;
  bool_t ret;

#ifdef HAVE_RESTART
  if (IS_GRACEFUL_RESTART (oi->top))
    return PAL_FALSE;
  
  if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_RESTART))
    return PAL_FALSE;
#endif /* HAVE_RESTART */

  /* Sanity check. We should be able to delet BFD session eitherways
  if (CHECK_FLAG (oi->flags, OSPF_IF_BFD))
    return PAL_FALSE;
  
  */

  pal_mem_set (&s, 0, sizeof (struct bfd_client_api_session));
  s.zg = ZG;
  s.vr_id = vr->id;
  s.vrf_id = iv->id;
  s.afi = AFI_IP;
  if (admin_flag)
    SET_FLAG (s.flags, BFD_CLIENT_API_FLAG_AD);
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK)
    {
      /* Ifindex should be 0 for Multihop sessions delete too */
      s.ifindex = 0;
      if (oi->ident.address->u.prefix4.s_addr)
        s.addr.ipv4.src = oi->ident.address->u.prefix4;
      else
        s.addr.ipv4.src = oi->u.vlink->out_oi->ident.address->u.prefix4;
 
      SET_FLAG (s.flags, BFD_CLIENT_API_FLAG_MH);
    }
  else
    {
      s.ifindex = oi->u.ifp->ifindex;
      s.addr.ipv4.src = oi->ident.address->u.prefix4;
    }

  s.addr.ipv4.dst = nbr->ident.address->u.prefix4;
  ret = bfd_client_api_session_delete (&s);

  if (ret == LIB_API_SUCCESS)
    UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_BFD);
  return ret;
}

void
ospf_bfd_update_session (struct ospf_neighbor *nbr)
{
  if (nbr->state > NFSM_TwoWay && nbr->ostate <= NFSM_TwoWay)
    ospf_bfd_add_session (nbr, PAL_FALSE);
  else if (nbr->ostate > NFSM_TwoWay && nbr->state <= NFSM_TwoWay)
    ospf_bfd_delete_session (nbr, PAL_FALSE);
}

static void
ospf_bfd_add_session_by_interface (struct ospf_interface *oi,
				   bool_t admin_flag)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK
      && (nbr = ospf_nbr_lookup_ptop (oi)))
    ospf_bfd_add_session (nbr, admin_flag);
  else
    for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
      if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
        if (nbr->state == NFSM_Full)
          ospf_bfd_add_session (nbr, admin_flag);
}

static void
ospf_bfd_delete_session_by_interface (struct ospf_interface *oi,
				      bool_t admin_flag)
{
  struct ospf_neighbor *nbr;
  struct ls_node *rn;
  if (oi->type == OSPF_IFTYPE_VIRTUALLINK
      && (nbr = ospf_nbr_lookup_ptop (oi)))
    ospf_bfd_delete_session (nbr, admin_flag);
  else
    for (rn = ls_table_top (oi->nbrs); rn; rn = ls_route_next (rn))
      if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
        ospf_bfd_delete_session (nbr, admin_flag);
}

static bool_t
ospf_bfd_if_flag_update (struct ospf_interface *oi)
{
  bool_t current_flag = CHECK_FLAG (oi->flags, OSPF_IF_BFD);
  if (OSPF_IF_PARAM_CHECK (oi->params_default, BFD_DISABLE))
    UNSET_FLAG (oi->flags, OSPF_IF_BFD);
  else if (OSPF_IF_PARAM_CHECK (oi->params_default, BFD))
    SET_FLAG (oi->flags, OSPF_IF_BFD);
  else if ((CHECK_FLAG (oi->top->config, OSPF_CONFIG_BFD)) &&
            (oi->type != OSPF_IFTYPE_VIRTUALLINK))
    SET_FLAG (oi->flags, OSPF_IF_BFD);
  else 
    UNSET_FLAG (oi->flags, OSPF_IF_BFD);

  return current_flag != (oi->flags & OSPF_IF_BFD);
}

void
ospf_bfd_if_update (struct ospf_interface *oi, bool_t admin_flag)
{
  if (ospf_bfd_if_flag_update (oi))
    {
      if (CHECK_FLAG (oi->flags, OSPF_IF_BFD))
        {
          ospf_bfd_add_session_by_interface (oi, PAL_FALSE);
        }
      else
        ospf_bfd_delete_session_by_interface (oi, admin_flag);
    }
}

void
ospf_bfd_update (struct ospf *top, bool_t admin_flag)
{
  struct ospf_interface *oi;
  struct ls_node *rn;
  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_OSPF_IF)))
      ospf_bfd_if_update (oi, admin_flag);
}

struct ospf_neighbor *
ospf_bfd_nbr_lookup (struct bfd_client_api_session *s)
{
  struct ospf_master *om;
  struct ospf_interface *oi;
  struct ospf_neighbor *nbr;

  om = ospf_master_lookup_by_id (ZG, s->vr_id);
  if (! om)
    return NULL;
  oi = ospf_global_if_entry_lookup (om, s->addr.ipv4.src, s->ifindex);
  if (oi)
    if ((nbr = ospf_nbr_lookup_by_addr (oi->nbrs, &s->addr.ipv4.dst)))
      if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_BFD))
        return nbr;
  return NULL;
}


static void
ospf_bfd_connected (struct bfd_client_api_server *s)
{
  struct ospf_master *om;
  struct apn_vr *vr;
  struct ospf *top;
  struct listnode *node;
  struct ls_node *rn, *rn1;
  struct ospf_neighbor *nbr;
  struct ospf_vlink *vlink;
  struct ospf_interface *oi;
  int i;

  for (i = 0; i < vector_max (s->zg->vr_vec); i++)
    if ((vr = vector_slot (s->zg->vr_vec, i)))
      if ((om = vr->proto))
	LIST_LOOP (om->ospf, top, node)
          {
            for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
               {
                 if ((oi = RN_INFO (rn, RNI_OSPF_IF)))
                   {
                     if (CHECK_FLAG (oi->flags, OSPF_IF_BFD))
                       {
                         ospf_bfd_add_session_by_interface (oi, PAL_FALSE);
                       }
                   }
               }

            /* Now go through all virtual links and sent the virtual link 
               information */
            for (rn = ls_table_top (top->vlink_table); rn; 
                                    rn = ls_route_next (rn))
              {
                if ((vlink = RN_INFO (rn, RNI_DEFAULT)))
                  {
                    for (rn1 = ls_table_top (vlink->oi->nbrs); rn1; 
                         rn1 = ls_route_next (rn1))
                      {
                        if ((nbr = RN_INFO (rn1, RNI_DEFAULT)))
                          if (nbr->state > NFSM_TwoWay)
                            ospf_bfd_add_session (nbr, PAL_FALSE);
                      }
                  }
              }
          }
}

static void
ospf_bfd_disconnected (struct bfd_client_api_server *s)
{
  struct ospf_master *om;
  struct apn_vr *vr;
  struct ospf *top;
  struct listnode *node;
  struct ospf_neighbor *nbr;
  struct ls_node *rn;
  int i;

  /* Reset the BFD flag for all the neighbors.  */
  for (i = 0; i < vector_max (s->zg->vr_vec); i++)
    if ((vr = vector_slot (s->zg->vr_vec, i)))
      if ((om = vr->proto))
	LIST_LOOP (om->ospf, top, node)
	  for (rn = ls_table_top (top->nbr_table); rn; rn = ls_route_next (rn))
	    if ((nbr = RN_INFO (rn, RNI_DEFAULT)))
	      if (CHECK_FLAG (nbr->flags, OSPF_NEIGHBOR_BFD))
		UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_BFD);
}

static void
ospf_bfd_session_down (struct bfd_client_api_session *s)
{
  struct ospf_neighbor *nbr = ospf_bfd_nbr_lookup (s);
  if (nbr)
    OSPF_NFSM_EVENT_EXECUTE (nbr, NFSM_KillNbr);
}

static void
ospf_bfd_session_admin_down (struct bfd_client_api_session *s)
{
  /* Ignore the admin down. */
  return;
}

static void
ospf_bfd_session_error (struct bfd_client_api_session *s)
{
#ifdef ERROR_SUPPORT
  struct ospf_neighbor *nbr = ospf_bfd_nbr_lookup (s);
  if (nbr)
    {
      /* Session error is sent if the OSPF session does not come up in 5 secomds
         This should not unset the BFD neighbor flags 
      UNSET_FLAG (nbr->flags, OSPF_NEIGHBOR_BFD);  */
    }
#endif  /* ERROR_SUPPORT */
}

void
ospf_bfd_init (struct lib_globals *zg)
{
  struct bfd_client_api_callbacks cb;
  memset (&cb, 0, sizeof (struct bfd_client_api_callbacks));
  cb.connected = ospf_bfd_connected;
  cb.disconnected = ospf_bfd_disconnected;
  cb.reconnected = ospf_bfd_connected;
  cb.session_down = ospf_bfd_session_down;
  cb.session_admin_down = ospf_bfd_session_admin_down;
  cb.session_error = ospf_bfd_session_error;

  bfd_client_api_client_new (zg, &cb);
  bfd_client_api_client_start (zg);
}

void
ospf_bfd_term (struct lib_globals *zg)
{
  bfd_client_api_client_delete (zg);
}

/* Turn on debug output.  */
void
ospf_bfd_debug_set (struct ospf_master *om)
{
  bfd_client_api_debug_set (om->zg);
}

/* Turn off debug output.  */
void
ospf_bfd_debug_unset (struct ospf_master *om)
{
  bfd_client_api_debug_unset (om->zg);
}

#endif /* HAVE_BFD */
