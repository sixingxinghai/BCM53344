/* Copyright (C) 2003  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_VPLS 

#include "ptree.h"
#include "table.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#ifdef HAVE_DSTE
#include "dste.h"
#endif /* HAVE_DSTE */
#include "mpls.h"
#include "nsm_message.h"
#include "log.h"
#include "buffer.h"
#include "mpls_common.h"
#include "mpls_client.h"

#include "nsmd.h"
#include "nsm_interface.h"
#include "nsm_mpls.h"
#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */
#include "nsm_mpls_rib.h"
#include "nsm_mpls_dep.h"
#include "nsm_vpls.h"
#include "nsm_server.h"
#include "nsm_mpls_vc.h"
#include "nsm_debug.h"
#include "bitmap.h"

#ifdef HAVE_SNMP
#include "nsm_mpls_vc_snmp.h"
#endif /* HAVE_SNMP */

int nsm_vpls_add_send (struct nsm_vpls *, cindex_t);
int nsm_vpls_delete_send (struct nsm_vpls *);
int nsm_vpls_peer_add_send (struct nsm_vpls *, struct addr_in *);
int nsm_vpls_peer_delete_send (struct nsm_vpls *, struct addr_in *);
int nsm_vpls_spoke_vc_add_send (struct nsm_mpls_circuit *, u_int16_t);
int nsm_vpls_spoke_vc_delete_send (struct nsm_mpls_circuit *);
extern pal_time_t nsm_get_sys_up_time ();

static int
nsm_vpls_spoke_vc_test (struct nsm_mpls_circuit *vc, char *vc_name,
                        struct nsm_vpls *vpls, bool_t update)
{
    struct nsm_master *nm = vpls->nm;
    struct nsm_mpls_if *mif;
    struct listnode *ln, *node;
    struct nsm_vpls_spoke_vc *svc;
    struct ptree_node *pn;
    struct nsm_vpls *vpls_tmp;
    struct nsm_mpls_vc_info *vc_info;

    if (vc->vc_info)
      return NSM_ERR_VC_BINDING_EXISTS;

    if (vc->vpls && ! update)
      {
        NSM_ASSERT (vc->vpls != vpls);
        if (vc->vpls == vpls)
          return NSM_ERR_INTERNAL;

        return NSM_ERR_VC_BINDING_EXISTS;
      }

    /* check if vc already attached to an interface */
    LIST_LOOP (NSM_MPLS->iflist, mif, ln)
      {
        LIST_LOOP (mif->vpls_info_list, vc_info, node)
          if ((vc_info->vc_name) &&
              (pal_strcmp (vc_info->vc_name, vc_name) == 0))
            return NSM_ERR_VC_BINDING_EXISTS;
      }

    /* check if vc already bound to some vpls instance */
    for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
      {
        if (! pn->info)
          continue;

        vpls_tmp = pn->info;
        svc = nsm_vpls_spoke_vc_lookup_by_name (vpls_tmp, vc_name,
            NSM_FALSE);
        if (svc && ! update)
          {
            ptree_unlock_node (pn);
            return NSM_ERR_VC_BINDING_EXISTS;
          }
      }

    return NSM_SUCCESS;
}


struct nsm_vpls *
nsm_vpls_new (struct nsm_master *nm, char *name, u_int32_t vpls_id,
              u_int16_t vpls_type, int *err)
{
  struct nsm_vpls *vpls;

  if (vpls_id == 0 || ! name)
    {
      *err = NSM_ERR_INVALID_ARGS;
      return NULL;
    }

  vpls = XCALLOC (MTYPE_NSM_VPLS, sizeof (struct nsm_vpls));
  if (! vpls)
    {
      *err = NSM_ERR_MEM_ALLOC_FAILURE;
      return NULL;
    }
  vpls->nm = nm;

  vpls->vpls_name = XSTRDUP (MTYPE_TMP, name);
  if (! vpls->vpls_name)
    {
     *err = NSM_ERR_MEM_ALLOC_FAILURE;
     goto cleanup;
    }

  vpls->mp_table = route_table_init ();
  if (! vpls->mp_table)
    {
      *err = NSM_ERR_MEM_ALLOC_FAILURE;
      goto cleanup;
    }

  vpls->mp_table->id = vpls_id;

  vpls->svc_list = list_new ();
  if (! vpls->svc_list)
    {
      *err = NSM_ERR_MEM_ALLOC_FAILURE;
      goto cleanup;
    }

  vpls->svc_list->del = nsm_vpls_spoke_vc_free;

  vpls->vpls_info_list = list_new ();
  if (! vpls->vpls_info_list)
    {
      *err = NSM_ERR_MEM_ALLOC_FAILURE;
      goto cleanup;
    }
  vpls->vpls_info_list->del = nsm_vpls_if_unlink;

  vpls->vpls_id = vpls_id;
  vpls->vpls_type = vpls_type;
  if (vpls_type == VC_TYPE_ETH_VLAN || vpls_type == VC_TYPE_ETHERNET)
    vpls->ifmtu = IF_ETHER_DEFAULT_MTU;

  /* send vpls new message to fwd */
  nsm_vpls_fwd_add (vpls);
  
  return vpls;
  
 cleanup:
  if (vpls->vpls_info_list)
    list_delete (vpls->vpls_info_list);

  if (vpls->svc_list)
    list_delete (vpls->svc_list);
  
  if (vpls->mp_table)
    route_table_finish (vpls->mp_table);

  if (vpls->vpls_name)
    XFREE (MTYPE_TMP, vpls->vpls_name);
  
  if (vpls)
    XFREE (MTYPE_NSM_VPLS, vpls);

  return NULL;
}


void
nsm_vpls_free (struct nsm_master *nm, struct nsm_vpls *vpls)
{
  struct route_node *rn;
  struct nsm_vpls_peer *peer;

  if (! vpls)
    return;

  /* send vpls delete to fwd */
  nsm_vpls_fwd_delete (vpls);

  if (vpls->vpls_name)
    XFREE (MTYPE_TMP, vpls->vpls_name);

  if (vpls->svc_list)
    {
      struct nsm_vpls_spoke_vc *svc;
      struct listnode *ln; 
      struct listnode *ln_next = NULL;

      for (ln = LISTHEAD (vpls->svc_list); ln; ln = ln_next)
        {
          ln_next = ln->next;
          svc = GETDATA (ln);
          nsm_vpls_spoke_vc_free (svc);
          list_delete_node (vpls->svc_list, ln);
        }

      list_delete (vpls->svc_list);
    }
  
  if (vpls->mp_table)
    {
      for (rn = route_top (vpls->mp_table); rn; rn = route_next (rn))
        {
          peer = rn->info;
          if (! peer)
            continue;
          
          nsm_vpls_mesh_peer_free (nm, peer, vpls);
          vpls->mp_count--;
          rn->info = NULL;
          route_unlock_node (rn);
        }
      
      vpls->mp_count = 0;
      route_table_finish (vpls->mp_table);
    }

  if (vpls->vpls_info_list)
    list_delete (vpls->vpls_info_list);

  XFREE (MTYPE_NSM_VPLS, vpls);

  return;
}


int
nsm_vpls_set_values (struct nsm_vpls *vpls, u_int16_t vpls_type, 
                     u_int16_t ifmtu, u_char c_word)
{
  if (! vpls)
    return NSM_ERR_INVALID_ARGS;

  vpls->vpls_type = vpls_type;
  vpls->ifmtu = ifmtu;
  vpls->c_word = c_word;

  return NSM_SUCCESS;
}


int
nsm_vpls_add (struct nsm_vpls *vpls)
{
  struct nsm_master *nm = NULL;
  struct ptree_node *pn;
  u_char *key;
  u_int32_t tmp32;
  
  if (! vpls)
    return NSM_ERR_INVALID_ARGS;

  nm = vpls->nm;
  NSM_ASSERT (NSM_MPLS != NULL);
  NSM_ASSERT (NSM_MPLS->vpls_table != NULL);
  if (! NSM_MPLS || ! NSM_MPLS->vpls_table)
    return NSM_ERR_INTERNAL;

  tmp32 = pal_hton32 (vpls->vpls_id);
  key = (u_char *)&tmp32;

  pn = ptree_node_get (NSM_MPLS->vpls_table, key, VPLS_ID_LEN);
  if (! pn)
    return NSM_ERR_MEM_ALLOC_FAILURE;

  NSM_ASSERT (pn->info == NULL);
  if (pn->info)
    {
      ptree_unlock_node (pn);
      return NSM_ERR_INTERNAL;
    }

  pn->info = vpls;
  
  return NSM_SUCCESS;
}


struct nsm_vpls *
nsm_vpls_lookup (struct nsm_master *nm,
                 u_int32_t vpls_id, bool_t rem_flag)
{
  struct ptree_node *pn;
  struct nsm_vpls *vpls;
  u_char *key;
  u_int32_t tmp32;

  if (! NSM_MPLS || ! NSM_MPLS->vpls_table)
    return NULL;
  
  if (vpls_id == 0)
    return NULL;

  tmp32 = pal_hton32 (vpls_id);
  key = (u_char *)&tmp32;

  pn = ptree_node_lookup (NSM_MPLS->vpls_table, key, VPLS_ID_LEN);
  if (! pn)
    return NULL;

  vpls = pn->info;
  ptree_unlock_node (pn);
  
  if (rem_flag)
    {
      pn->info = NULL;
      ptree_unlock_node (pn);
    }

  return vpls;
}

/** @brief Lookup VPLS table based on ID.
    @param nm - NSM master
    @param id - VPLS identifier
    @return struct nsm_vpls 
 */
struct nsm_vpls *
nsm_vpls_lookup_by_id (struct nsm_master *nm, u_int32_t id)
{
  struct nsm_vpls *vpls = NULL;
  struct ptree_node *pn = NULL;

  if ((! NSM_MPLS) || (! NSM_MPLS->vpls_table))
    return NULL;

  for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
    {
      vpls = pn->info;
      if (! vpls)
        continue;

      if (vpls->vpls_id == id)
        {
          ptree_unlock_node (pn);
          return vpls;
        }
    }

  return NULL;
}


struct nsm_vpls *
nsm_vpls_lookup_by_name (struct nsm_master *nm, char *sz_name)
{
  struct nsm_vpls *vpls;
  struct ptree_node *pn;

  if (! sz_name || pal_strlen (sz_name) == 0)
    return NULL;

  for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
    {
      vpls = pn->info;
      if (! vpls)
        continue;

      if (pal_strcmp (vpls->vpls_name, sz_name) == 0)
        {
          ptree_unlock_node (pn);
          return vpls;
        }
    }

  return NULL;
}


int
nsm_vpls_activate (struct nsm_vpls *vpls)
{
  NSM_ASSERT (vpls != NULL);
  if (! vpls)
    return NSM_ERR_INVALID_ARGS;

  if (vpls->vpls_type != VC_TYPE_ETH_VLAN
          && vpls->vpls_type != VC_TYPE_ETHERNET) 
    return NSM_SUCCESS;

  if ((vpls->state == NSM_VPLS_STATE_INACTIVE) &&
      ((vpls->mp_count > 0) || (! LIST_ISEMPTY(vpls->svc_list))))
    vpls->state = NSM_VPLS_STATE_ACTIVE;

  return NSM_SUCCESS;
}


int
nsm_vpls_mesh_peer_activate (struct nsm_vpls *vpls, struct addr_in *addr)
{
  NSM_ASSERT (vpls != NULL);
  if (! vpls)
    return NSM_ERR_INVALID_ARGS;

  if (vpls->state == NSM_VPLS_STATE_ACTIVE)
    {
            /* send peer add msg to ldp */
            nsm_vpls_peer_add_send (vpls, addr);
    }
  else
    {
      nsm_vpls_activate (vpls);

      if (vpls->state == NSM_VPLS_STATE_ACTIVE)
        {
                /* send vpls add msg to ldp */
                nsm_vpls_add_send (vpls, NSM_VPLS_ATTR_FLAG_ALL);
        }
    }

  return NSM_SUCCESS;
}

int
nsm_vpls_cleanup (struct nsm_master *nm, struct nsm_vpls *vpls)
{
  struct route_node *rn;
  struct nsm_vpls_peer *peer = NULL;
  struct nsm_mpls_vc_info *vc_info = NULL;
  struct listnode *ln, *ln_next;

  NSM_ASSERT (vpls != NULL);
  if (! vpls)
    return NSM_ERR_INVALID_ARGS;

  if (vpls->state == NSM_VPLS_STATE_INACTIVE)
    return NSM_SUCCESS;

  /* send vpls delete msg to ldp */
  nsm_vpls_delete_send (vpls);
  
  if (vpls->state == NSM_VPLS_STATE_ACTIVE)
    {
      struct listnode *ln;
      struct nsm_vpls_spoke_vc *svc;

      for (rn = route_top (vpls->mp_table); rn; rn = route_next (rn))
        {
          if (! rn->info)
            continue;

          peer = rn->info;
          nsm_vpls_mesh_peer_cleanup (nm, vpls, rn);
        }

      LIST_LOOP (vpls->svc_list, svc, ln)
        nsm_vpls_spoke_vc_cleanup (nm, svc, NSM_FALSE, NSM_TRUE);
    }

  for (ln = LISTHEAD (vpls->vpls_info_list); ln; ln = ln_next)
    {
      ln_next = ln->next;
      vc_info = GETDATA (ln);

      if (vc_info)
        nsm_vpls_interface_delete (vpls, vc_info->mif->ifp, vc_info); 
    }

  vpls->state = NSM_VPLS_STATE_INACTIVE;

  return NSM_SUCCESS;
}


struct nsm_vpls_peer *
nsm_vpls_peer_new (struct addr_in *addr, int *err)
{
  struct nsm_vpls_peer *peer;

  if (! addr || gmpls_addr_in_zero (addr))
    {
      *err = NSM_ERR_INVALID_ARGS;
      return NULL;
    }

  peer = XCALLOC (MTYPE_NSM_VPLS_PEER, sizeof (struct nsm_vpls_peer));
  if (! peer)
    {
      *err = NSM_ERR_MEM_ALLOC_FAILURE;
      return NULL;
    }
  
  pal_mem_set (peer, 0, sizeof (struct nsm_vpls_peer));
  pal_mem_cpy (&peer->peer_addr, addr, sizeof (struct addr_in));
  
#ifdef HAVE_SNMP  
  peer->create_time = nsm_get_sys_up_time();  
#endif /*HAVE_SNMP */  
  return peer;
}


void
nsm_vpls_mesh_peer_free (struct nsm_master *nm, struct nsm_vpls_peer *peer,
                         struct nsm_vpls *vpls)
{
  if (! peer || ! vpls)
    return;

  /* Trap notifying VPLS deletion */
#ifdef HAVE_SNMP
          if (peer)
            {
              nsm_mpls_vc_del_notify (vpls->vpls_id, vpls->vpls_type,
                                     ((peer->peer_addr.afi == AFI_IP)
                                     ? INET_AD_IPV4 : INET_AD_UNKNOWN),
                                     &peer->peer_addr.u.ipv4,
                                     peer->vc_snmp_index);
            }

  /*deleting the vc snmp index part*/
  bitmap_release_index (NSM_MPLS->vc_indx_mgr,peer->vc_snmp_index);
#endif /*HAVE_SNMP */

  if (peer->vc_fib)
    XFREE (MTYPE_NSM_VC_FIB, peer->vc_fib);

  XFREE (MTYPE_NSM_VPLS_PEER, peer);
  return;
}


int
nsm_vpls_mesh_peer_add (struct nsm_vpls *vpls, struct nsm_vpls_peer *peer)
{
  struct route_node *rn;
  struct prefix p;

  if (! vpls || ! peer)
    return NSM_ERR_INVALID_ARGS;
    
  NSM_ASSERT (vpls->mp_table != NULL);
  if (! vpls->mp_table)
    return NSM_ERR_INTERNAL;

  mpls_addr_in_to_prefix (&peer->peer_addr, &p);

  rn = route_node_get (vpls->mp_table, &p);
  if (! rn)
    return NSM_ERR_MEM_ALLOC_FAILURE;

  NSM_ASSERT (rn->info == NULL);
  if (rn->info)
    {
      route_unlock_node (rn);
      return NSM_ERR_INTERNAL;
    }

  rn->info = peer;
  vpls->mp_count++;

  return NSM_SUCCESS;
}

#ifdef HAVE_SNMP
/** @brief Function to loop through MEsh VC's
    for snmp get request based on the vc_snmp_index

    @param struct nsm_master *nm    - NSM master
    @param vc_index                 - vc_snmp_index

    return struct nsm_vpls_peer * - mesh vc sturcture
*/
struct nsm_vpls_peer *
nsm_vpls_snmp_lookup_by_index (struct nsm_master *nm,
                               u_int32_t vc_index)
{
  struct nsm_vpls *vpls = NULL;
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct route_node *rn;
  struct ptree_node *pn;
  
  if ((! NSM_MPLS) || (! NSM_MPLS->vpls_table) ||
     (vc_index == NSM_ERR_INVALID_VPLS))
    return NULL;

   for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
    {
       if (! pn->info)
            continue;
        
        vpls = pn->info;     
        for (rn = route_top (vpls->mp_table); rn; rn = route_next (rn))
          {
            if (!rn->info)
              continue;
            
            vpls_peer = rn->info;
            if (! vpls_peer)
              continue;

            if (vpls_peer->vc_snmp_index == vc_index)
              {
                route_unlock_node (rn);
                return vpls_peer;
              }
           } 
        }
  return NULL; 
}
#endif /*HAVE_SNMP */
struct route_node *
nsm_vpls_mesh_peer_node_lookup (struct nsm_vpls *vpls, struct addr_in *addr) 
                                
{
  struct route_node *rn;
  struct prefix p;
  
  if (! vpls || ! addr || gmpls_addr_in_zero (addr))
    return NULL;
    
  NSM_ASSERT (vpls->mp_table != NULL);
  if (! vpls->mp_table)
    return NULL;

  mpls_addr_in_to_prefix (addr, &p);
  
  rn = route_node_lookup (vpls->mp_table, &p);
  if (! rn)
    return NULL;

  route_unlock_node (rn);
  
  return rn;
}

struct nsm_vpls_peer *
nsm_vpls_mesh_peer_lookup (struct nsm_vpls *vpls, struct addr_in *addr, 
                           bool_t rem_flag)
{
  struct nsm_vpls_peer *peer;
  struct route_node *rn;
  
  if (! vpls || ! addr || gmpls_addr_in_zero (addr))
    return NULL;
    
  NSM_ASSERT (vpls->mp_table != NULL);
  if (! vpls->mp_table)
    return NULL;

  rn = nsm_vpls_mesh_peer_node_lookup (vpls, addr);
  if (! rn)
    return NULL;

  peer = rn->info;

  if (rem_flag)
    {
      rn->info = NULL;
      route_unlock_node (rn);
      vpls->mp_count--;
    }

  return peer;
}

int
nsm_vpls_mesh_peer_cleanup (struct nsm_master *nm,
                            struct nsm_vpls *vpls, struct route_node *rn_peer)
{
  struct prefix pfx; 
  struct nsm_vpls_peer *peer;
  struct fec_gen_entry gen_entry;

  NSM_ASSERT (rn_peer != NULL && rn_peer->info != NULL);
  if (! rn_peer || ! rn_peer->info)
    return NSM_ERR_INVALID_ARGS;

  peer = rn_peer->info;

  mpls_addr_in_to_prefix (&peer->peer_addr, &pfx);
  NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &pfx);
  gmpls_lsp_dep_del (nm, &gen_entry, CONFIRM_VPLS_MESH_VC, rn_peer);

  if (peer->state != NSM_VPLS_PEER_DOWN)
    NSM_VPLS_MESH_PEER_FIB_RESET (peer);
#ifdef HAVE_SNMP
    peer->last_change = nsm_get_sys_up_time ();
#endif /*HAVE_SNMP */
                            
  
  return NSM_SUCCESS;
}


struct nsm_vpls_spoke_vc *
nsm_vpls_spoke_vc_new (char *vc_name, struct nsm_mpls_circuit *vc,
                       int *err)
{
  struct nsm_vpls_spoke_vc *svc;

  if (! vc_name || pal_strlen (vc_name) == 0)
    {
      *err = NSM_ERR_INVALID_ARGS;
      return NULL;
    }

  svc = XCALLOC (MTYPE_NSM_VPLS_SPOKE_VC, sizeof (struct nsm_vpls_spoke_vc));
  if (! svc)
    {
      *err = NSM_ERR_MEM_ALLOC_FAILURE;
      return NULL;
    }

  svc->vc_name = XSTRDUP (MTYPE_TMP, vc_name);
  svc->vc = vc;
  
  return svc;
}


void
nsm_vpls_spoke_vc_free (void *ptr)
{
  struct nsm_vpls_spoke_vc *svc;

  if (! ptr)
    return;

  svc = (struct nsm_vpls_spoke_vc *)ptr;

  if (svc->vc_name)
    XFREE (MTYPE_TMP, svc->vc_name);

  if (svc->vc_fib)
    XFREE (MTYPE_NSM_VC_FIB, svc->vc_fib);

  XFREE (MTYPE_NSM_VPLS_SPOKE_VC, svc);
}


int
nsm_vpls_spoke_vc_add (struct nsm_vpls *vpls, struct nsm_vpls_spoke_vc *svc)
{
  if (! vpls || ! svc)
    return NSM_ERR_INVALID_ARGS;

  listnode_add (vpls->svc_list, svc);

  return NSM_SUCCESS;
}


struct nsm_vpls_spoke_vc *
nsm_vpls_spoke_vc_lookup (struct nsm_vpls *vpls, u_int32_t vc_id, 
                          bool_t del_flag)
{
  struct nsm_vpls_spoke_vc *svc;
  struct listnode *ln;

  if (! vpls || vc_id == 0)
    return NULL;

  for (ln = LISTHEAD (vpls->svc_list); ln; NEXTNODE (ln))
    {
      svc = GETDATA (ln);
      if (svc->vc && svc->vc->id == vc_id)
        {
          if (del_flag && (! svc->secondary))
            list_delete_node (vpls->svc_list, ln);

          return svc;
        }
    }

  return NULL;
}


struct nsm_vpls_spoke_vc *
nsm_vpls_spoke_vc_lookup_by_name (struct nsm_vpls *vpls, char *vc_name,
                                  bool_t del_flag)
{
  struct nsm_vpls_spoke_vc *svc;
  struct listnode *ln;

  if (! vpls || ! vc_name || pal_strlen (vc_name) == 0)
    return NULL;

  for (ln = LISTHEAD (vpls->svc_list); ln; NEXTNODE (ln))
    {
      svc = GETDATA (ln);
      if (pal_strcmp (svc->vc_name, vc_name) == 0)
        {
          if (del_flag)
            list_delete_node (vpls->svc_list, ln);

          return svc;
        }
    }

  return NULL;
}


void
nsm_vpls_spoke_vc_cleanup (struct nsm_master *nm,
                           struct nsm_vpls_spoke_vc *svc, 
                           bool_t send_msg, bool_t delink_spoke_vc)
{
  struct fec_gen_entry gen_entry;

  if (! svc)
    return;

  if (svc->vc)
    {
      /* send delete message to ldp */
      if (send_msg && svc->vc->vpls->state == NSM_VPLS_STATE_ACTIVE)
        nsm_vpls_spoke_vc_delete_send (svc->vc);
      
      if (svc->vc->vpls->mp_count == 0)
        svc->vc->vpls->state = NSM_VPLS_STATE_INACTIVE;

      UNSET_FLAG (svc->vc->flags, NSM_MPLS_VC_FLAG_SELECTED);
      if (FLAG_ISSET (svc->vc->flags, NSM_MPLS_VC_FLAG_DEP))
        {
          NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &svc->vc->address);
          gmpls_lsp_dep_del (nm, &gen_entry, CONFIRM_VPLS_SPOKE_VC, svc);
          UNSET_FLAG (svc->vc->flags, NSM_MPLS_VC_FLAG_DEP);
        }

      if (svc->state >= NSM_VPLS_SPOKE_VC_ACTIVE)
        NSM_VPLS_SPOKE_VC_FIB_RESET (svc);

      if (delink_spoke_vc)
        {
          svc->vc->vpls = NULL;
          svc->vc = NULL;
        }
    }
}


/* Process zero or more MAC Address withdrawals for a specified VPLS. */
int
nsm_vpls_mac_withdraw_process (struct nsm_master *nm,
                               struct nsm_msg_vpls_mac_withdraw *vmw)
{
  struct nsm_vpls *vpls;
  int ret;

  if (! vmw)
    return NSM_ERR_INVALID_ARGS;

  /* Lookup VPLS by Id. */
  vpls = nsm_vpls_lookup (nm, vmw->vpls_id, NSM_FALSE);
  if (! vpls)
    return NSM_FAILURE;

  /* If num == 0, withdraw all MAC Addresses for this VPLS. */
  if (vmw->num == 0)
    {
      /* TBD.
         Need to send FLUSH_ALL MAC Addresses msg to the Forwarder. */
      ret = apn_mpls_vpls_mac_withdraw (vmw->vpls_id, vmw->num, NULL);      
      return ret;
    }

  ret = apn_mpls_vpls_mac_withdraw (vmw->vpls_id, vmw->num, vmw->mac_addrs);

  return ret;

#if 0
  /* Process MAC Addresses. */
  mac_addr = vmw->mac_addrs;
  for (i = 0; i < vmw->num; i++)
    {
      /* TBD.
         Need to send this MAC Address to the Forwarder. */
      mac_addr += VPLS_MAC_ADDR_LEN;
    }
#endif 
}


int
nsm_vpls_add_process (struct nsm_master *nm,
                      char *name, u_int32_t vpls_id, u_int16_t vpls_type, 
                      u_int16_t ifmtu, u_char c_word, 
                      struct nsm_vpls **ppvpls)
{
  struct nsm_vpls *vpls;
  struct nsm_mpls_vc_info *vc_info;
  struct nsm_mpls_if *mif;
  struct nsm_mpls_circuit *vc;
  struct listnode *ln;
  struct interface *ifp;
  struct route_node *rn;
  int err;
  int ret;

  if (ppvpls)
    *ppvpls = NULL;

  if (! name || vpls_id == 0
      || (vpls_type != VC_TYPE_ETH_VLAN && vpls_type != VC_TYPE_ETHERNET))
    return NSM_ERR_INVALID_ARGS;

  /* Check if any Virtual Circuit is using this ID. */
  vc = nsm_mpls_l2_circuit_lookup_by_id (nm, vpls_id);
  if (vc)
    return NSM_ERR_INVALID_VPLS_ID;

  vpls = nsm_vpls_lookup (nm, vpls_id, NSM_FALSE);
  if (vpls)
    {
      if (pal_strcmp (name, vpls->vpls_name) != 0)
        return NSM_ERR_INVALID_VPLS_NAME;

      if (ppvpls)
        *ppvpls = vpls;
      return NSM_SUCCESS;
    }

  /* make sure there is no pre-existing vpls with same name */
  vpls = nsm_vpls_lookup_by_name (nm, name);
  if (vpls)
    return NSM_ERR_INVALID_VPLS_ID;

  vpls = nsm_vpls_new (nm, name, vpls_id, vpls_type, &err);
  if (! vpls)
    return err;

  /* Set back pointer. */
  vpls->nm = nm;

  ret = nsm_vpls_set_values (vpls, vpls_type, ifmtu, c_word);
  if (ret != NSM_SUCCESS)
    {
      nsm_vpls_free (nm, vpls);
      return ret;
    }

  ret = nsm_vpls_add (vpls);
  if (ret != NSM_SUCCESS)
    {
      nsm_vpls_free (nm, vpls);
      return ret;
    }
  
  /* add interfaces bound to vpls in the list */
  for (rn = route_top (nm->vr->ifm.if_table); rn; rn = route_next (rn))
    if ((ifp = rn->info))
      {
        mif = nsm_mpls_if_lookup (ifp);    
        if (!mif)
          continue;

        LIST_LOOP (mif->vpls_info_list, vc_info, ln)    
          if ((CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS) ||
               CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_VPLS_VLAN)) &&
              (pal_strcmp (vc_info->vc_name, vpls->vpls_name) == 0))
            {
              vc_info->u.vpls = vpls;
              nsm_vpls_interface_add (vpls, ifp, vc_info);
            }
      }

  ret = nsm_vpls_activate (vpls);
  if (ret == NSM_SUCCESS &&
      vpls->state == NSM_VPLS_STATE_ACTIVE)
    {
      /* send vpls add msg to ldp */
      nsm_vpls_add_send (vpls, NSM_VPLS_ATTR_FLAG_ALL);

      /* send vpls vc add */
      nsm_vpls_spoke_vc_add_send_all (vpls);
    }
  
  if (ppvpls)
    *ppvpls = vpls;

  return NSM_SUCCESS;
}


int
nsm_vpls_spoke_vc_add_send_all (struct nsm_vpls *vpls)
{
  struct listnode *ln;
  struct nsm_vpls_spoke_vc *svc;

  if (! vpls)
    return NSM_ERR_INVALID_ARGS;

  LIST_LOOP (vpls->svc_list, svc, ln)
    if (svc->vc)
      nsm_vpls_spoke_vc_add_send (svc->vc, svc->vc_type);
  
  return NSM_SUCCESS;
}


int
nsm_vpls_delete_process (struct nsm_master *nm, struct nsm_vpls *vpls)
{
  if (! vpls)
    return NSM_ERR_INVALID_ARGS;

  nsm_vpls_group_unregister (nm, vpls);
  
  nsm_vpls_cleanup (nm, vpls);

  /* remove from table */
  nsm_vpls_lookup (nm, vpls->vpls_id, NSM_TRUE);

  nsm_vpls_free (nm, vpls);

  return NSM_SUCCESS;
}

/** @brief Function for adding VPLS mesh peer.

    @param nm            - NSM Master
    @param vpls          - VPLS structure
    @param sz_peer       - Peer address
    @param mapping_type  - Mapping type
    @param tunnel_info   - tunnel name/id
    @param fec_type_vc   - Identify VC type
    @param sz_mode       - mode
    @param sec_peer      - secondary peer address
*/
int
nsm_vpls_mesh_peer_add_process (struct nsm_master *nm,
                                struct nsm_vpls *vpls, struct addr_in *addr,
                                u_char mapping_type, char *tunnel_info,
                                u_char tunnel_dir, u_int8_t fec_type_vc, 
                                char *peer_mode, struct addr_in *sec_addr)
{
  struct nsm_vpls_peer *peer = NULL, *sec_peer = NULL;
  struct route_node *rn = NULL;
  struct fec_gen_entry fec;
  struct prefix pfx;
  u_int32_t temp_tunnel_id = -1;
  int err, ret;
  bool_t mapping_update = NSM_FALSE;

  if (! vpls || ! addr || gmpls_addr_in_zero (addr))
    return NSM_ERR_INVALID_ARGS;

  NSM_ASSERT (vpls->mp_table != NULL);
  if (! vpls->mp_table)
    return NSM_ERR_INTERNAL;

  if (mapping_type != MPLS_VC_MAPPING_NONE && tunnel_info == NULL)
    return NSM_ERR_INVALID_ARGS;

  if (mapping_type == MPLS_VC_MAPPING_TUNNEL_ID)
    temp_tunnel_id = pal_strtou32 (tunnel_info, NULL, 10);

  rn = nsm_vpls_mesh_peer_node_lookup (vpls, addr);

  if (rn && rn->info)
    peer = rn->info;

  if (! peer)
    {
      peer = nsm_vpls_peer_new (addr, &err);
      if (! peer)
        return err;
      peer->peer_mode = NSM_MPLS_VC_PRIMARY;
      ret = nsm_vpls_mesh_peer_add (vpls, peer);
      if (ret != NSM_SUCCESS)
        {
          nsm_vpls_mesh_peer_free (nm, peer, vpls);
          return ret;
        }
#ifdef HAVE_SNMP
   peer->vpls_id = vpls->vpls_id;   
   ret = bitmap_request_index (NSM_MPLS->vc_indx_mgr,&peer->vc_snmp_index);
   if (ret < 0)
     return NSM_FAILURE;
#endif /*HAVE_SNMP */
   
      if (fec_type_vc != PW_OWNER_MANUAL)
        nsm_vpls_mesh_peer_activate(vpls, addr);
    }
  else if (peer->state == NSM_VPLS_PEER_DOWN && fec_type_vc != PW_OWNER_MANUAL)
    {
      nsm_vpls_mesh_peer_activate(vpls, addr);
    }
  /* If mapping_type is changed and peer is either active or up.*/
  else if (peer->state > NSM_VPLS_PEER_DOWN) 
    {
      if (peer->fec_type_vc != fec_type_vc)
        {
          return NSM_ERR_VPLS_MESH_OWNER_MISMATCH;
        }

      /*
       *  If mapping type is changed or
       *  if the tunnel_id or tunnel name is changed
       */
      if ((peer->mapping_type != mapping_type) ||
          (peer->tunnel_dir != tunnel_dir) ||
          (mapping_type == MPLS_VC_MAPPING_TUNNEL_ID &&
           (peer->tunnel_id != temp_tunnel_id)) ||
          (mapping_type == MPLS_VC_MAPPING_TUNNEL_NAME &&
           (pal_strcmp (peer->tunnel_name, tunnel_info) != 0)))
        {
          mpls_addr_in_to_prefix (&peer->peer_addr, &pfx);
          NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY (&fec, &pfx);
          gmpls_lsp_dep_del (nm, &fec, CONFIRM_VPLS_MESH_VC, rn);

          peer->tunnel_id = 0;
          if (peer->tunnel_name)
            XFREE (MTYPE_TMP, peer->tunnel_name);
          mapping_update = NSM_TRUE;
          peer->tunnel_dir = TUNNEL_DIR_FWD;
        }
    }

  /* Identify Tunnel Info. */
  peer->mapping_type = mapping_type;
  peer->tunnel_dir = tunnel_dir;
  if (mapping_type == MPLS_VC_MAPPING_TUNNEL_NAME)
    {
      peer->tunnel_name = XSTRDUP (MTYPE_TMP, tunnel_info);
      peer->tunnel_id = 0;
    }
  else if (mapping_type == MPLS_VC_MAPPING_TUNNEL_ID)
    {
      peer->tunnel_id = temp_tunnel_id;
      peer->tunnel_name = NULL;
    }
  else
    {
      peer->tunnel_name = NULL;
      peer->tunnel_id = 0;
    }

  peer->fec_type_vc = fec_type_vc;
  if (mapping_update == NSM_TRUE)
    {
      mpls_addr_in_to_prefix (&peer->peer_addr, &pfx);
      NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY (&fec, &pfx);
      gmpls_lsp_dep_add (nm, &fec, CONFIRM_VPLS_MESH_VC, rn);
    }

  if (peer_mode)
    {
      sec_peer = nsm_vpls_mesh_peer_lookup (vpls, sec_addr, NSM_FALSE);
      if (sec_peer)
        {
          if (sec_peer->peer_mode == NSM_MPLS_VC_PRIMARY)
            return NSM_ERR_VPLS_PEER_EXISTS;
          if (sec_peer->primary && sec_peer->primary != peer)
            return NSM_ERR_VPLS_PEER_BOUND_TO_PRIM;
          return NSM_SUCCESS;
        }
      else
        {
          sec_peer = nsm_vpls_peer_new (sec_addr, &err);
          if (! sec_peer)
            return NSM_ERR_VPLS_SEC_PEER_FAILURE;
        }
      sec_peer->peer_mode = NSM_MPLS_VC_SECONDARY;
      sec_peer->primary = peer;
      peer->secondary = sec_peer;

      ret = nsm_vpls_mesh_peer_add (vpls, sec_peer);
      if (ret != NSM_SUCCESS)
        {
          nsm_vpls_mesh_peer_free (nm, sec_peer, vpls);
          return ret;
        }
     
      nsm_vpls_mesh_peer_activate(vpls, sec_addr);
    }
  else if (peer->secondary)
    {
      zlog_info (NSM_ZG, " Deleting the secondary peer for %r",
          &peer->peer_addr.u.ipv4);
      ret = nsm_vpls_mesh_peer_delete_process (nm, vpls,
          &peer->secondary->peer_addr);
      peer->secondary = NULL;
    }

  return NSM_SUCCESS;
}

/** @brief Function for deleting VPLS mesh peer.

    @param nm            - NSM Master
    @param vpls          - VPLS structure
    @param addr          - Peer address
*/
int
nsm_vpls_mesh_peer_delete_process (struct nsm_master *nm, struct nsm_vpls *vpls,
                                   struct addr_in *addr)
{
  struct nsm_vpls_peer *peer = NULL;
  struct route_node *rn = NULL;

  if (! vpls || ! addr || gmpls_addr_in_zero (addr))
    return NSM_ERR_INVALID_ARGS;

  NSM_ASSERT (vpls->mp_table != NULL);
  if (! vpls->mp_table)
    return NSM_ERR_INTERNAL;

  rn = nsm_vpls_mesh_peer_node_lookup (vpls, addr);
  if (! rn)
    return NSM_ERR_VPLS_PEER_NOT_FOUND;

  nsm_vpls_mesh_peer_cleanup (nm, vpls, rn);

  peer = rn->info;

  if (vpls->state == NSM_VPLS_STATE_ACTIVE)
    {
      if (vpls->mp_count == 1)
        {
          struct listnode *ln;
          struct nsm_vpls_spoke_vc *svc;

          LIST_LOOP (vpls->svc_list, svc, ln)
                   return NSM_SUCCESS;

           /* send vpls delete msg to ldp */
           nsm_vpls_delete_send (vpls);

          vpls->state = NSM_VPLS_STATE_INACTIVE;
        }
      else
        {
          /*
           * If peer VC is not statically configured
           * send peer delete msg to ldp
           */
          if (peer->fec_type_vc != PW_OWNER_MANUAL)
            nsm_vpls_peer_delete_send (vpls, addr);
        }
    }

  if (! peer->secondary)
    {
      nsm_vpls_mesh_peer_lookup (vpls, addr, NSM_TRUE);
      nsm_vpls_mesh_peer_free (nm, peer, vpls);
      peer = NULL;
    }

  return NSM_SUCCESS;
}


int
nsm_vpls_spoke_vc_add_process (struct nsm_vpls *vpls, char *vc_name, 
                               u_int16_t vc_type, char *sz_mode,
                               char *sz_name)
{
  struct nsm_mpls_circuit *vc = NULL, *vc1 = NULL;
  struct nsm_vpls_spoke_vc *svc = NULL, *svc_sec = NULL;
  bool_t update = NSM_FALSE;
  int ret = NSM_FAILURE;
  bool_t svc_created = NSM_FALSE, svc_sec_created = NSM_FALSE;

  if (! vpls || ! vc_name || pal_strlen (vc_name) == 0)
    return NSM_ERR_INVALID_ARGS;

  svc = nsm_vpls_spoke_vc_lookup_by_name (vpls, vc_name, NSM_FALSE);
  if (svc)
    {
      if (svc->vc_type == vc_type) 
        return NSM_SUCCESS;
      else
        return NSM_FAILURE;
    }
  if (sz_mode)
    {
      svc_sec = nsm_vpls_spoke_vc_lookup_by_name (vpls, sz_name, NSM_FALSE);
      vc1 = nsm_mpls_l2_circuit_lookup_by_name (vpls->nm, sz_name);
    }
  /* The below conditions would not be met. Commenting it for now */
#if 0
  if (svc && (svc->state > NSM_VPLS_SPOKE_VC_DOWN))
    {
      if (svc_sec && svc_sec->primary == svc)
        return NSM_SUCCESS;
    }
#endif 
  /* lookup vc */
  vc = nsm_mpls_l2_circuit_lookup_by_name (vpls->nm, vc_name);

  if (vc)
    {
      ret = nsm_vpls_spoke_vc_test (vc, vc_name, vpls, update);

      if (ret < 0)
        return ret;
    }

  if (vc1)
    {
      if (svc_sec)
        ret = nsm_vpls_spoke_vc_test (vc1, sz_name, vpls, NSM_TRUE);
      else
        ret = nsm_vpls_spoke_vc_test (vc1, sz_name, vpls, NSM_FALSE);
      if (ret < 0)
        return ret;
    }

  if (! svc)
    {
      svc = nsm_vpls_spoke_vc_new (vc_name, vc, &ret);
      svc_created = NSM_TRUE;
    }

  if (! svc)
    return ret;
 
  if (sz_mode && ! svc_sec)
    {
      svc_sec = nsm_vpls_spoke_vc_new (sz_name, vc1, &ret);
      svc_sec_created = NSM_TRUE;
    }

  if (sz_mode && ! svc_sec)
    {
      if (svc && svc_created)
        nsm_vpls_spoke_vc_free (svc);
      return ret;
    }

  svc->vc_type = vc_type;
  svc->svc_mode = NSM_MPLS_VC_PRIMARY;

  ret = nsm_vpls_spoke_vc_add (vpls, svc);
  if (ret < 0)
    {
      if (svc && svc_created)
        nsm_vpls_spoke_vc_free (svc);
      if (svc_sec && svc_sec_created)
        nsm_vpls_spoke_vc_free (svc_sec);
      return ret;
    }

  if (vc)
    {
      vc->vpls = vpls;
      nsm_vpls_activate (vpls);

      vc->state = NSM_MPLS_L2_CIRCUIT_ACTIVE;

      if (vpls->state == NSM_VPLS_STATE_ACTIVE)
        nsm_vpls_spoke_vc_add_send (vc, vc_type);
    }

  if (sz_mode)
    {
      svc->secondary = svc_sec;
      svc_sec->svc_mode = NSM_MPLS_VC_SECONDARY;
      svc_sec->vc_type = vc_type;
      svc_sec->primary = svc;
      ret = nsm_vpls_spoke_vc_add (vpls, svc_sec);
      if (ret < 0)
        {
          if (svc_sec && svc_sec_created)
            nsm_vpls_spoke_vc_free (svc_sec);
          return ret;
        }
      if (vc1)
        {
          vc1->vpls = vpls;
          vc1->state = NSM_MPLS_L2_CIRCUIT_ACTIVE;
          if (vpls->state == NSM_VPLS_STATE_ACTIVE)
            nsm_vpls_spoke_vc_add_send (vc1, vc_type);
        }
    }
  else if (svc->secondary)
    {
      zlog_info (NSM_ZG, "Deleting secondary spoke %s for VPLS %s",
          vpls->vpls_name, svc->secondary->vc_name);
      ret = nsm_vpls_spoke_vc_delete_process (vpls->nm, vpls, svc->secondary->vc_name);
      if (ret < 0)
        return ret;
      svc->secondary = NULL;
    }
  return NSM_SUCCESS;
}

int
nsm_vpls_spoke_vc_delete_process (struct nsm_master *nm,
                                  struct nsm_vpls *vpls, char *vc_name)
{
  struct nsm_vpls_spoke_vc *svc;

  if (! vpls || ! vc_name || pal_strlen (vc_name) == 0)
    return NSM_ERR_INVALID_ARGS;

  svc = nsm_vpls_spoke_vc_lookup_by_name (vpls, vc_name, NSM_TRUE);
  if (! svc)
    return NSM_ERR_SPOKE_VC_NOT_FOUND;

  if (! svc->secondary)
    {
      nsm_vpls_spoke_vc_cleanup (nm, svc, NSM_TRUE, NSM_TRUE);
      nsm_vpls_spoke_vc_free (svc);
    }
   else
     nsm_vpls_spoke_vc_cleanup (nm, svc, NSM_TRUE, NSM_FALSE);

#if 0
  if (vpls->mp_count == 0)
        {
           /* send vpls delete msg to ldp */
          nsm_vpls_delete_send (vpls);

          vpls->state = NSM_VPLS_STATE_INACTIVE;
        } 
#endif  
  return 0;
}

int
nsm_vpls_interface_add (struct nsm_vpls *vpls, struct interface *ifp, 
                        struct nsm_mpls_vc_info *vc_info)
{
  struct listnode *ln, *ln_p = NULL;
  struct nsm_mpls_vc_info *tmp_vci;
  bool_t added = NSM_FALSE;

  if (! vpls || ! ifp)
    return NSM_ERR_INVALID_ARGS;

  if (listcount (vpls->vpls_info_list) == 0)
    {
      listnode_add (vpls->vpls_info_list, vc_info);
      nsm_vpls_fwd_if_bind (vpls, ifp, vc_info->vlan_id);
      return NSM_SUCCESS;
    }
 
  /* First sort by ifindex */ 
  for (ln = LISTHEAD (vpls->vpls_info_list); ln; NEXTNODE (ln))
    {
      tmp_vci = GETDATA (ln);
      if (!tmp_vci)
        continue;

      if (tmp_vci->mif->ifp->ifindex < ifp->ifindex)
        continue;

      if (tmp_vci->mif->ifp->ifindex > ifp->ifindex)
        {
          list_add_node_prev (vpls->vpls_info_list, ln, vc_info);
          added = NSM_TRUE;
          break;
        }
      /* Secondary sort by vlan_id */
      if (tmp_vci->mif->ifp->ifindex == ifp->ifindex)
        {
          if (tmp_vci->vlan_id == vc_info->vlan_id)
            return NSM_SUCCESS;
          
          if (tmp_vci->vlan_id > vc_info->vlan_id)
            {
              list_add_node_prev (vpls->vpls_info_list, ln, vc_info);
              added = NSM_TRUE;
              break;
            }
            
          if (tmp_vci->vlan_id < vc_info->vlan_id)
            ln_p = ln;
        }
    }

  if (! added)
    {
      if (ln_p)
        list_add_node_next (vpls->vpls_info_list, ln_p, vc_info);
      else
        listnode_add (vpls->vpls_info_list, vc_info);
    }

  nsm_vpls_fwd_if_bind (vpls, ifp, vc_info->vlan_id);

  return NSM_SUCCESS;
}

int
nsm_vpls_interface_delete (struct nsm_vpls *vpls, struct interface *ifp,
                           struct nsm_mpls_vc_info *vc_info)
{
  struct listnode *ln;
  struct nsm_mpls_if *mif;
  struct nsm_mpls_vc_info *tmp_vci;
  
  if (! vpls || ! ifp)
    return NSM_ERR_INVALID_ARGS;

  mif = vc_info->mif;
  if (!mif)
    return NSM_ERR_INVALID_ARGS;

  for (ln = LISTHEAD (vpls->vpls_info_list); ln; NEXTNODE (ln))
    {
      tmp_vci = GETDATA (ln);
      if (tmp_vci != vc_info)
        continue;

      nsm_vpls_fwd_if_unbind (vpls, ifp, vc_info->vlan_id);
      /* delete from vpls->vpls_info_list */
      list_delete_node (vpls->vpls_info_list, ln);
      nsm_vpls_if_unlink (vc_info);
      break;
    }

  return NSM_SUCCESS;
}

int
nsm_vpls_interface_up (struct nsm_vpls *vpls, struct interface *ifp)
{
  return NSM_SUCCESS;
}


int
nsm_vpls_interface_down (struct nsm_vpls *vpls, struct interface *ifp)
{
  return NSM_SUCCESS;
}

void
nsm_vpls_if_unlink (void *p)
{
  ((struct nsm_mpls_vc_info *)p)->u.vpls = NULL;
}


int
nsm_vpls_fib_add_msg_process (struct nsm_master *nm,
                              struct nsm_msg_vc_fib_add *msg)
{
  struct nsm_vpls *vpls = NULL;
  struct nsm_vpls_peer *vp = NULL;
  bool_t fib_created = NSM_FALSE;
  bool_t mesh_vc = NSM_FALSE;
  struct route_node *rn = NULL;
  struct prefix pfx;
  struct nsm_vpls_spoke_vc *svc = NULL;
  struct vc_fib **ppfib = NULL;
  struct fec_gen_entry gen_entry;

  if (! msg)
    return NSM_ERR_INVALID_ARGS;

  vpls = nsm_vpls_lookup (nm, msg->vpn_id, NSM_FALSE);
  if (! vpls)
    return NSM_ERR_VPLS_NOT_FOUND;

  
  if (msg->vc_style == MPLS_VC_STYLE_VPLS_MESH)
    {
      rn = nsm_vpls_mesh_peer_node_lookup (vpls, &msg->addr);
      if (! rn)
        return NSM_ERR_VPLS_PEER_NOT_FOUND;
      vp = rn->info;

      mesh_vc = NSM_TRUE;
      ppfib = &vp->vc_fib;
    }
  else
    {
      svc = nsm_vpls_spoke_vc_lookup (vpls, msg->vc_id, NSM_FALSE);
      if (! svc || ! svc->vc)
        return NSM_ERR_VC_ID_NOT_FOUND;

      ppfib = &svc->vc_fib;
    }

  if (*ppfib == NULL)
    {
      *ppfib = XCALLOC (MTYPE_NSM_VC_FIB, sizeof (struct vc_fib));
      if (*ppfib == NULL)
        return NSM_ERR_MEM_ALLOC_FAILURE;

      fib_created = NSM_TRUE;
    }

  (*ppfib)->in_label = msg->in_label;
  (*ppfib)->out_label = msg->out_label;
  (*ppfib)->ac_if_ix = msg->ac_if_ix;
  (*ppfib)->nw_if_ix = msg->nw_if_ix;

  if (mesh_vc)
    {
      if (vp->peer_mode == NSM_MPLS_VC_PRIMARY)
        {
          /* MKD : TODO : delete secondary vc dep data here */
          vp->state = NSM_VPLS_PEER_ACTIVE;
#ifdef HAVE_SNMP
          vp->last_change = nsm_get_sys_up_time ();
#endif /*HAVE_SNMP */
          mpls_addr_in_to_prefix (&vp->peer_addr, &pfx);
          NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &pfx);
          gmpls_lsp_dep_add (nm, &gen_entry, CONFIRM_VPLS_MESH_VC, rn);
        }
      else if (vp->primary->state < NSM_VPLS_PEER_UP)
        {
          vp->state = NSM_VPLS_PEER_ACTIVE;
#ifdef HAVE_SNMP
          vp->last_change = nsm_get_sys_up_time ();
#endif /*HAVE_SNMP */
          mpls_addr_in_to_prefix (&vp->peer_addr, &pfx);
          NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &pfx);
          gmpls_lsp_dep_add (nm, &gen_entry, CONFIRM_VPLS_MESH_VC, rn);
        }
    }
  else
    {
      if (svc->svc_mode == NSM_MPLS_VC_PRIMARY)
        {
          svc->state = NSM_VPLS_SPOKE_VC_ACTIVE;
          SET_FLAG (svc->vc->flags, NSM_MPLS_VC_FLAG_SELECTED);
          NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &svc->vc->address);
          gmpls_lsp_dep_add (nm, &gen_entry, CONFIRM_VPLS_SPOKE_VC, svc);
          SET_FLAG (svc->vc->flags, NSM_MPLS_VC_FLAG_DEP);
        }
      else if (! CHECK_FLAG (svc->primary->vc->flags, NSM_MPLS_VC_FLAG_SELECTED))
        {
          svc->state = NSM_VPLS_SPOKE_VC_ACTIVE;
          SET_FLAG (svc->vc->flags, NSM_MPLS_VC_FLAG_SELECTED);
          NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &svc->vc->address);
          gmpls_lsp_dep_add (nm, &gen_entry, CONFIRM_VPLS_SPOKE_VC, svc);
          SET_FLAG (svc->vc->flags, NSM_MPLS_VC_FLAG_DEP);
        }

    }

  return NSM_SUCCESS;
}

int
nsm_vpls_fib_delete_msg_process (struct nsm_master *nm,
                                 struct nsm_msg_vc_fib_delete *msg)
{
  struct nsm_vpls *vpls;
  struct nsm_vpls_peer *vp = NULL;
  struct nsm_vpls_spoke_vc *svc = NULL;
  struct prefix pfx;
  struct route_node *rn = NULL;
  struct fec_gen_entry gen_entry;
  
  if (! msg)
    return NSM_ERR_INVALID_ARGS;
  
  vpls = nsm_vpls_lookup (nm, msg->vpn_id, NSM_FALSE);
  if (! vpls)
    return NSM_ERR_VPLS_NOT_FOUND;

  if (msg->vc_style == MPLS_VC_STYLE_VPLS_SPOKE)
    {
      
      svc = nsm_vpls_spoke_vc_lookup (vpls, msg->vc_id, NSM_FALSE);
      if (! svc || ! svc->vc)
        return NSM_ERR_VC_ID_NOT_FOUND;

      if (svc->state == NSM_VPLS_SPOKE_VC_UP)
        {
          UNSET_FLAG (svc->vc->flags, NSM_MPLS_VC_FLAG_SELECTED);
          NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &svc->vc->address);
          gmpls_lsp_dep_del (nm, &gen_entry, CONFIRM_VPLS_SPOKE_VC, svc);
          UNSET_FLAG (svc->vc->flags, NSM_MPLS_VC_FLAG_DEP);
          NSM_VPLS_SPOKE_VC_FIB_RESET (svc);
        }
    }
  else
    {
      rn = nsm_vpls_mesh_peer_node_lookup (vpls, &msg->addr);
      if (! rn)
        return NSM_ERR_VPLS_PEER_NOT_FOUND;
      
      vp = rn->info;
      
      /* delete from forwarder */
      if (vp->state >= NSM_VPLS_PEER_ACTIVE)
        {
          mpls_addr_in_to_prefix (&vp->peer_addr, &pfx);
          NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &pfx);
          gmpls_lsp_dep_del (nm, &gen_entry, CONFIRM_VPLS_MESH_VC, rn);
          NSM_VPLS_MESH_PEER_FIB_RESET (vp);
#ifdef HAVE_SNMP
          vp->last_change = nsm_get_sys_up_time ();
#endif /*HAVE_SNMP */
                                      
        }
    }

  return NSM_SUCCESS;
}


int
nsm_vpls_fib_cleanup (struct nsm_master *nm, struct nsm_vpls *vpls)
{
  struct nsm_vpls_peer *vp;
  struct route_node *rn;
  struct listnode *ln;
  struct nsm_vpls_spoke_vc *svc;
  struct prefix pfx;
  struct fec_gen_entry gen_entry;

  for (rn = route_top (vpls->mp_table); rn; rn = route_next (rn))
    {
      if (! rn->info)
        continue;

      vp = rn->info;
      if (vp->fec_type_vc != PW_OWNER_MANUAL && 
          vp->state >= NSM_VPLS_PEER_ACTIVE)
      {
        mpls_addr_in_to_prefix (&vp->peer_addr, &pfx);
        NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &pfx);
        gmpls_lsp_dep_del (nm, &gen_entry, CONFIRM_VPLS_MESH_VC, rn);
        NSM_VPLS_MESH_PEER_FIB_RESET (vp);
#ifdef HAVE_SNMP
        vp->last_change = nsm_get_sys_up_time ();
#endif /*HAVE_SNMP */
                                    
      }
    }

  LIST_LOOP (vpls->svc_list, svc, ln)
    {
      if (svc->vc && svc->vc->fec_type_vc != PW_OWNER_MANUAL
          && svc->state >= NSM_VPLS_SPOKE_VC_ACTIVE)
        {
          UNSET_FLAG (svc->vc->flags, NSM_MPLS_VC_FLAG_SELECTED);
          NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &svc->vc->address);
          gmpls_lsp_dep_del (nm, &gen_entry, CONFIRM_VPLS_SPOKE_VC, svc);
          UNSET_FLAG (svc->vc->flags, NSM_MPLS_VC_FLAG_DEP);
          NSM_VPLS_SPOKE_VC_FIB_RESET (svc);
        }
    }

  return NSM_SUCCESS;
} 


void
nsm_vpls_fib_cleanup_all (struct nsm_master *nm)
{
  struct ptree_node *pn;
  struct nsm_vpls *vpls;

  for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
    {
      if (! pn->info)
        continue;

      vpls = pn->info;
      nsm_vpls_fib_cleanup (nm, vpls);
    }
}


int
_nsm_vpls_fib_add (u_int32_t vc_id, struct nsm_vpls *vpls, struct vc_fib *vc_fib, 
                   struct pal_in4_addr *peer_addr,
                   u_int32_t vc_style, struct ftn_entry *tunnel_ftn)
{
  u_int32_t tunnel_label, tunnel_ftnix;
  struct pal_in4_addr *tunnel_nhop, *vpls_peer_nhop = NULL;
  u_int32_t tunnel_oix, tunnel_nhlfe_ix;
  struct nhlfe_key *tkey = NULL;
  u_int32_t nw_if_ix, ac_if_ix;

  tunnel_label = LABEL_VALUE_INVALID;
  tunnel_ftnix = 0;
  tunnel_nhop = NULL;
  tunnel_oix = 0;
  tunnel_nhlfe_ix = 0;
  
  if (tunnel_ftn)
    {
      tunnel_ftnix = tunnel_ftn->ftn_ix;
      tkey = (struct nhlfe_key *) &FTN_NHLFE (tunnel_ftn)->nkey;
      vpls_peer_nhop = &tkey->u.ipv4.nh_addr;
      
      if (tkey->u.ipv4.out_label != LABEL_IMPLICIT_NULL &&
          tkey->u.ipv4.out_label != LABEL_IPV4_EXP_NULL)
        {
          tunnel_label = tkey->u.ipv4.out_label;
          tunnel_nhop = &tkey->u.ipv4.nh_addr;
#ifdef HAVE_GMPLS
          tunnel_oix = nsm_gmpls_get_ifindex_from_gifindex (vpls->nm,
                          tkey->u.ipv4.oif_ix);
#else
          tunnel_oix = tkey->u.ipv4.oif_ix;
#endif /* HAVE_GMPLS */
          tunnel_nhlfe_ix = FTN_NHLFE (tunnel_ftn)->nhlfe_ix;
        }
    }
  else
    {
      vpls_peer_nhop = peer_addr;
    }

#ifdef HAVE_GMPLS
  nw_if_ix = nsm_gmpls_get_ifindex_from_gifindex(vpls->nm, vc_fib->nw_if_ix);
  ac_if_ix = nsm_gmpls_get_ifindex_from_gifindex(vpls->nm, vc_fib->ac_if_ix);
#else
  nw_if_ix = vc_fib->nw_if_ix;
  ac_if_ix = vc_fib->ac_if_ix;
#endif /* HAVE_GMPLS */

  return apn_mpls_vc4_fib_add (vc_id,
                              vc_style,
                              &vpls->vpls_id,
                              &vc_fib->in_label,
                              &vc_fib->out_label,
                              &ac_if_ix,
                              &nw_if_ix,
                              NULL,
                              NULL,
                              vc_fib->opcode,
                              peer_addr,
                              vpls_peer_nhop,
                              NULL,
                              NULL,
                              &tunnel_label,
                              tunnel_nhop,
                              &tunnel_oix,
                              &tunnel_nhlfe_ix,
                              &tunnel_ftnix,
                              NSM_FALSE);
}


int
nsm_vpls_fib_add (struct nsm_vpls *vpls, struct nsm_vpls_peer *peer, 
                  u_char opcode, struct ftn_entry *tunnel_ftn)
{
  peer->vc_fib->opcode = opcode;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Adding fib entry for vpls %d, nbr %r", vpls->vpls_id,
                &peer->peer_addr.u.ipv4);

  return _nsm_vpls_fib_add (vpls->vpls_id, vpls, peer->vc_fib, 
                            &peer->peer_addr.u.ipv4,
                            MPLS_VC_STYLE_VPLS_MESH, tunnel_ftn);
}


int
nsm_vpls_fib_delete (struct nsm_vpls *vpls, struct nsm_vpls_peer *peer)
{
  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Deleting fib entry for vpls %d, nbr %r", vpls->vpls_id,
               &peer->peer_addr.u.ipv4);
  return apn_mpls_vc_fib_delete (vpls->vpls_id,
                                 MPLS_VC_STYLE_VPLS_MESH,
                                 &vpls->vpls_id,
                                 &peer->vc_fib->in_label,
                                 &peer->vc_fib->out_label,
                                 &peer->vc_fib->ac_if_ix,
                                 &peer->vc_fib->nw_if_ix,
                                 NULL,
                                 NULL,
                                 &peer->peer_addr.u.ipv4,
                                 NSM_FALSE);
}


int
nsm_vpls_spoke_vc_fib_add (struct nsm_vpls_spoke_vc *svc, u_char opcode, 
                           struct ftn_entry *tunnel_ftn)
{
  int ret;

  svc->vc_fib->opcode = opcode;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Adding fib entry for vpls spoke vc %d, nbr %r",
               svc->vc->id, &svc->vc->address.u.prefix4);

  ret = _nsm_vpls_fib_add (svc->vc->id, svc->vc->vpls, svc->vc_fib,
                           &svc->vc->address.u.prefix4,
                           MPLS_VC_STYLE_VPLS_SPOKE, tunnel_ftn);

  svc->vc->pw_status = NSM_CODE_PW_FORWARDING;

  /* send pw-state to protocol */
  if (svc->vc->fec_type_vc != PW_OWNER_MANUAL)
    nsm_vpls_spoke_vc_send_pw_status (svc->vc->vpls, svc->vc);

  return ret;

}


int
nsm_vpls_spoke_vc_fib_delete (struct nsm_vpls_spoke_vc *svc)
{
  int ret;

  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Deleting fib entry for vpls spoke vc  %d, nbr %r",
               svc->vc->id, &svc->vc->address.u.prefix4);

  ret = apn_mpls_vc_fib_delete (svc->vc->id,
                                MPLS_VC_STYLE_VPLS_SPOKE,
                                &svc->vc->vpls->vpls_id,
                                &svc->vc_fib->in_label,
                                &svc->vc_fib->out_label,
                                &svc->vc_fib->ac_if_ix,
                                &svc->vc_fib->nw_if_ix,
                                NULL,
                                NULL,
                                &svc->vc->address.u.prefix4,
                                NSM_FALSE);

  SET_FLAG (svc->vc->pw_status, NSM_CODE_PW_NOT_FORWARDING);

  /* send pw-state to protocol */
  if (svc->vc->fec_type_vc != PW_OWNER_MANUAL)
    nsm_vpls_spoke_vc_send_pw_status (svc->vc->vpls, svc->vc);

  return ret;
}

int
nsm_vpls_fwd_add (struct nsm_vpls *vpls)
{
  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Adding vpls instance %d to fwd", vpls->vpls_id);
  return apn_mpls_vpls_fwd_add (&vpls->vpls_id);
}

int
nsm_vpls_fwd_delete (struct nsm_vpls *vpls)
{
  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Deleting vpls instance %d from fwd", vpls->vpls_id);
  return apn_mpls_vpls_fwd_delete (&vpls->vpls_id);
}

int 
nsm_vpls_fwd_if_bind (struct nsm_vpls *vpls, struct interface *ifp, 
                      u_int16_t vlan_id)
{
  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Adding interface %d to vpls instance %d in fwd", 
               ifp->ifindex, vpls->vpls_id);
  return apn_mpls_vpls_fwd_if_bind (&vpls->vpls_id,
                                    &ifp->ifindex, vlan_id);
}

int 
nsm_vpls_fwd_if_unbind (struct nsm_vpls *vpls, struct interface *ifp,
                        u_int16_t vlan_id)
{
  if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Deleting interface %d from vpls instance %d in fwd", 
               ifp->ifindex, vpls->vpls_id);
  return apn_mpls_vpls_fwd_if_unbind (&vpls->vpls_id,
                                      &ifp->ifindex, vlan_id);
}


/* Set MTU information for VPLS instance. */
void
nsm_vpls_mtu_set (struct nsm_vpls *vpls, u_int16_t mtu)
{
  cindex_t cindex = 0;
  if (! vpls ||
      (CHECK_FLAG (vpls->config, NSM_VPLS_CONFIG_MTU) && vpls->ifmtu == mtu))
    return;

  /* Set data. */
  vpls->ifmtu = mtu;
  SET_FLAG (vpls->config, NSM_VPLS_CONFIG_MTU);

  /* Send VPLS update. */
  if (vpls->state == NSM_VPLS_STATE_ACTIVE)
    {
      NSM_SET_CTYPE (cindex, NSM_VPLS_CTYPE_IF_MTU);
      nsm_vpls_add_send (vpls, cindex);
    }
}

void
nsm_vpls_group_set (struct nsm_vpls *vpls, u_int32_t grp_id)
{
  cindex_t cindex = 0;
  if (! vpls ||
      (CHECK_FLAG (vpls->config, NSM_VPLS_CONFIG_GROUP_ID) &&
        vpls->grp_id == grp_id))
    return;

  /* Set data. */
  vpls->grp_id = grp_id;
  SET_FLAG (vpls->config, NSM_VPLS_CONFIG_GROUP_ID);

  /* Send VPLS update. */
  if (vpls->state == NSM_VPLS_STATE_ACTIVE)
    {
      NSM_SET_CTYPE (cindex, NSM_VPLS_CTYPE_GROUP_ID);
      nsm_vpls_add_send (vpls, cindex);
    }
}

void
nsm_vpls_group_unset (struct nsm_vpls *vpls)
{
  cindex_t cindex = 0;
  if (! vpls || ! CHECK_FLAG (vpls->config, NSM_VPLS_CONFIG_GROUP_ID))
    return;

  vpls->grp_id = 0;

  UNSET_FLAG (vpls->config, NSM_VPLS_CONFIG_GROUP_ID);

  /* Send VPLS update. */
  if (vpls->state == NSM_VPLS_STATE_ACTIVE)
    {
      NSM_SET_CTYPE (cindex, NSM_VPLS_CTYPE_GROUP_ID);
      nsm_vpls_add_send (vpls, cindex);
    }
}

/* Unset MTU information for VPLS instance. */
void
nsm_vpls_mtu_unset (struct nsm_vpls *vpls)
{
  cindex_t cindex = 0;
  if (! vpls || ! CHECK_FLAG (vpls->config, NSM_VPLS_CONFIG_MTU))
    return;

  /* Set ifmtu to default. */
  if (vpls->vpls_type == VC_TYPE_ETH_VLAN || 
      vpls->vpls_type == VC_TYPE_ETHERNET)
    vpls->ifmtu = IF_ETHER_DEFAULT_MTU;
  else
    vpls->ifmtu = 0;

  UNSET_FLAG (vpls->config, NSM_VPLS_CONFIG_MTU);

  /* Send VPLS update. */
  if (vpls->state == NSM_VPLS_STATE_ACTIVE)
    {
      NSM_SET_CTYPE (cindex, NSM_VPLS_CTYPE_IF_MTU);
      nsm_vpls_add_send (vpls, cindex);
    }
}

/* Set description for VPLS instance. */
void
nsm_vpls_desc_set (struct nsm_vpls *vpls, char *argv)
{
  if (! vpls || ! argv)
    return;
  
  /* Remove old. */
  if (vpls->desc)
    {
      XFREE (MTYPE_TMP, vpls->desc);
      vpls->desc = NULL;
    }

  vpls->desc = XSTRDUP (MTYPE_TMP, argv);
}

/* Unset description for VPLS instance. */
void
nsm_vpls_desc_unset (struct nsm_vpls *vpls)
{
  if (vpls && vpls->desc)
    {
      XFREE (MTYPE_TMP, vpls->desc);
      vpls->desc = NULL;
    }
}

/* Set type. */
int
nsm_vpls_type_set (struct nsm_vpls *vpls, u_int16_t type)
{
  cindex_t cindex = 0;
  NSM_ASSERT (vpls != NULL && type != 0);
  if (! vpls || ! type)
    return NSM_FAILURE;

  /* If no change, return. */
  if (vpls->vpls_type == type)
    {
      if (! CHECK_FLAG (vpls->config, NSM_VPLS_CONFIG_TYPE))
        SET_FLAG (vpls->config, NSM_VPLS_CONFIG_TYPE);

      return NSM_SUCCESS;
    }

  /* If there are any mesh of spoke VCs already configured for this VPLS,
     type cannot be changed. */
  if (vpls->mp_count || LISTCOUNT (vpls->svc_list))
    return NSM_ERR_VPLS_PEER_EXISTS;

  /* Set data. */
  SET_FLAG (vpls->config, NSM_VPLS_CONFIG_TYPE);
  vpls->vpls_type = type;

  /* Send VPLS update. */
  if (vpls->state == NSM_VPLS_STATE_ACTIVE)
    {
      NSM_SET_CTYPE (cindex, NSM_VPLS_CTYPE_VPLS_TYPE);
      nsm_vpls_add_send (vpls, cindex);
    }

  return NSM_SUCCESS;
}

/* Unset type to VLAN. */
int
nsm_vpls_type_unset (struct nsm_vpls *vpls)
{
  cindex_t cindex = 0;
  NSM_ASSERT (vpls != NULL);
  if (! vpls)
    return NSM_FAILURE;

  /* If no change, return. */
  if (vpls->vpls_type == VC_TYPE_ETHERNET)
    {
      if (CHECK_FLAG (vpls->config, NSM_VPLS_CONFIG_TYPE))
        UNSET_FLAG (vpls->config, NSM_VPLS_CONFIG_TYPE);

      return NSM_SUCCESS;
    }

  /* If there are any mesh of spoke VCs already configured for this VPLS,
     type cannot be changed. */
  if (vpls->mp_count || LISTCOUNT (vpls->svc_list))
    return NSM_ERR_VPLS_PEER_EXISTS;

  UNSET_FLAG (vpls->config, NSM_VPLS_CONFIG_TYPE);
  vpls->vpls_type = VC_TYPE_ETHERNET;

  /* Send VPLS update. */
  if (vpls->state == NSM_VPLS_STATE_ACTIVE)
    {
      NSM_SET_CTYPE (cindex, NSM_VPLS_CTYPE_VPLS_TYPE);
      nsm_vpls_add_send (vpls, cindex);
    }

  return NSM_SUCCESS;
}

#ifdef HAVE_VLAN
void
nsm_vpls_vlan_bind (struct interface *ifp, u_int16_t vlan_id)
{
  struct nsm_mpls_if *mif;
  struct nsm_mpls_vc_info *vc_info;
  struct listnode *ln;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    return;

  LIST_LOOP (mif->vpls_info_list, vc_info, ln) 
    if ((vc_info->vlan_id == vlan_id || vlan_id == NSM_VLAN_ALL) && 
        vc_info->u.vpls)
      nsm_vpls_if_add_send (ifp, vlan_id);
}

void
nsm_vpls_vlan_unbind (struct interface *ifp, u_int16_t vlan_id)
{
  struct nsm_mpls_if *mif;
  struct nsm_mpls_vc_info *vc_info;
  struct listnode *ln;
  int vpls_bind = 0;
  int found = 0;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    return;

  /* If no vpls bind in this interface configuration other than 
     given vlan, unbind vpls from interface. */
  LIST_LOOP (mif->vpls_info_list, vc_info, ln) 
    {
      if (vc_info->vlan_id != vlan_id)
        if (NSM_MPLS_VALID_VLAN (ifp, vc_info->vlan_id) == NSM_SUCCESS)
          vpls_bind ++;

      if (vc_info->vlan_id == vlan_id && vc_info->u.vpls)
        found ++;
    }

  /* Send unbind vpls from interface to LDP but no not change ifp->bind flag,
     so it could send bind again when vlan is added back. */
  if (vpls_bind == 0 && found)
      nsm_vpls_vlan_if_delete_send (ifp);
}
#endif /* HAVE_VLAN */
#endif /* HAVE_VPLS */
