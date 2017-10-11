/* Copyright (C) 2002  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "tlv.h"
#include "thread.h"
#include "checksum.h"

#include "vty.h"
#include "linklist.h"
#include "prefix.h"
#include "table.h"
#include "hash.h"

#include "if.h"
#include "log.h"
#include "label_pool.h"
#include "bitmap.h"
#ifdef HAVE_TE
#include "admin_grp.h"
#endif /* HAVE_TE */
#include "ptree.h"
#include "mpls.h"
#include "mpls_common.h"
#include "mpls_client.h"
#include "nsmd.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#ifdef HAVE_DSTE
#include "dste.h"
#include "nsm_dste.h"
#endif /* HAVE_DSTE */
#include "nsm_message.h"
#include "nsm_mpls.h"
#ifdef HAVE_GMPLS
#include "nsm_gmpls.h"
#endif /* HAVE_GMPLS */
#include "nsm_mpls_vc.h"
#include "nsm_interface.h"
#include "nsm_debug.h"
#include "nsm_redistribute.h"
#include "nsm_table.h"
#include "rib.h"
#include "nsm_server.h"
#include "nsm_mpls_rib.h"
#include "nsm_mpls_api.h"
#include "nsm_lp_serv.h"
#ifdef HAVE_NSM_MPLS_OAM
#include "nsm_mpls_oam.h"
#endif /* HAVE_NSM_MPLS_OAM */

#ifdef HAVE_SNMP
#include "asn1.h"
#include "nsm_mpls_snmp.h"
#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc_snmp.h"
#include "nsm_mpls_vc_api.h"
#ifdef HAVE_MS_PW
#include "nsm_mpls_ms_pw.h"
#endif /* HAVE_MS_PW */
#endif /* HAVE_MPLS_VC */
#endif /* HAVE_SNMP */

#ifdef HAVE_VPLS
#include "nsm_vpls.h"
#endif /* HAVE_VPLS */
#include "nsm_router.h"
#define IP_ADDR_INVALID  0xffffffff
/* Prototype from nsm_mpls_cli.c. */
void nsm_mpls_init_commands (void);
int apn_mpls_init_all_handles  (struct lib_globals *, u_char protocol);

#ifdef HAVE_MPLS_VC
void nsm_mpls_l2_circuit_add_send (struct nsm_mpls_if *,
                                   struct nsm_mpls_circuit *);
void nsm_mpls_l2_circuit_del_send (struct nsm_mpls_if *,
                                   struct nsm_mpls_circuit *);
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_NSM_MPLS_OAM
static int
nsm_mpls_oam_udp_create_socket (struct nsm_master *nm)
{
  const char *fn_name = "nsm_mpls_oam_udp_create_socket";
  int sock = -1;
  struct pal_sockaddr_in4 addr;
  int ret = -1;

  sock = pal_sock (NSM_ZG, AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0)
    {
      zlog_err (NSM_ZG, "%s: Socket creation error, errno = %d", fn_name, errno);
      return NSM_FAILURE;
    }
  pal_sock_set_reuseaddr (sock, 1);
  pal_sock_set_reuseport (sock, 1);

  ret = pal_sock_set_ip_recvif (sock, 1);
  if (ret < 0)
    {
      zlog_warn (NSM_ZG, "%s: Unable to set required socket option to "
                 "receive incoming interface for a packet, errno =  %d",
                 fn_name, errno);
    }

  /* bind socket to OAM UDP port */
  pal_mem_set (&addr, 0, sizeof (struct pal_sockaddr_in4));
  addr.sin_family = AF_INET;
  addr.sin_port = pal_hton16 (MPLS_OAM_DEFAULT_PORT_UDP);
  addr.sin_addr.s_addr = pal_hton32 (INADDR_ANY);

  ret = pal_sock_bind (sock, (struct pal_sockaddr *)&addr,
                       sizeof (struct pal_sockaddr_in4));
  if (ret < 0)
    {
      zlog_err (NSM_ZG, "%s: Failed to bind socket %d, errno = %d", fn_name,
                sock, errno);
      pal_sock_close (NSM_ZG, sock);
      return NSM_FAILURE;
    }

  nm->nmpls->oam_sock = sock;

  return NSM_SUCCESS;
}

static int
oam_raw_create_socket (struct nsm_master *nm)
{
  const char *fn_name = "oam_raw_create_socket";
  int sock = -1;
  struct pal_sockaddr_in4 addr;
  int ret = -1;

  /* create socket */
  sock = pal_sock (NSM_ZG, AF_INET, SOCK_RAW, IPPROTO_UDP);
  if (sock < 0)
    {
      zlog_err (NSM_ZG, "%s: Socket creation error, errno = %d", fn_name, errno);
      return NSM_FAILURE;
    }

  /* set IP_HDRINCL option */
  ret = pal_sock_set_ip_hdr_incl (sock, 1);
  if (ret < 0)
    {
      zlog_warn (NSM_ZG, "%s: Unable to set IP_HDRINCL socket option errno =  %d",
                 fn_name, errno);
      pal_sock_close (NSM_ZG, sock);
      return NSM_FAILURE;
    }

  /*
    bind socket to an invalid IP address to
    avoid receiving packets on this socket
  */
  pal_mem_set (&addr, 0, sizeof (struct pal_sockaddr_in4));
  addr.sin_family = AF_INET;
  addr.sin_port = 0;
  addr.sin_addr.s_addr = IP_ADDR_INVALID;

  ret = pal_sock_bind (sock, (struct pal_sockaddr *)&addr,
                       sizeof (struct pal_sockaddr_in4));
  if (ret < 0)
    zlog_err (NSM_ZG, "%s: Failed to bind socket %d, errno = %d", fn_name,
              sock, errno);

  /* disable multicast loopback */
  ret = pal_sock_set_ipv4_multicast_loop (sock, 0);
  if (ret < 0)
    {
      zlog_err (NSM_ZG, "Can't set IP_MULTICAST_LOOP");
      pal_sock_close (NSM_ZG, sock);
      return NSM_FAILURE;
    }

  /* set ldp socket for sending hello packets */
  nm->nmpls->oam_s_sock = sock;

  return NSM_SUCCESS;
}

#endif /* HAVE_NSM_MPLS_OAM */

void
nsm_avl_ix_table_init (struct avl_ix_table *tbl, s_int32_t max_nodes,
                       struct bitmap *ix_mgr,
                       int (*compare_function) (void *data1, void *data2))
{
  avl_create (&tbl->m_table, max_nodes, compare_function);

  if (ix_mgr)
    tbl->ix_mgr = ix_mgr;
  else
    tbl->ix_mgr = bitmap_default_new ();
}

/* Init MPLS Patricia Tree and bitmap. */
void
nsm_ptree_ix_table_init (struct ptree_ix_table *table, u_int16_t key_len,
                         struct bitmap *ix_mgr)
{
  table->m_table = ptree_init (key_len);
  if (ix_mgr)
    table->ix_mgr = ix_mgr;
  else
    table->ix_mgr = bitmap_default_new ();
}

/* Init MPLS Route Table and bitmap. */
void
nsm_route_ix_table_init (struct route_ix_table *table, u_int32_t id,
                         struct bitmap *map)
{
  table->m_table = route_table_init ();
  if (table->m_table)
    route_table_id_set (table->m_table, id);
  if (map)
    table->ix_mgr = map;
  else
    table->ix_mgr = bitmap_default_new ();
}

/* Initialize a new MPLS interface. */
struct nsm_mpls_if *
nsm_mpls_if_new (struct nsm_master *nm)
{
  struct nsm_mpls_if *mif;

  /* Initialize interface object. */
  mif = XCALLOC (MTYPE_NSM_MPLS_IF, sizeof (struct nsm_mpls_if));
  if (! mif)
    return NULL;

  /* Set NSM Master */
  mif->nm = nm;

  /* Initialize vrf id to 0. */
  mif->vrf_id = VRF_ID_UNSPEC;

#ifdef HAVE_MPLS_VC
  mif->vc_info_list = list_new ();
#endif /* HAVE_MPLS_VC */
#ifdef HAVE_VPLS
  mif->vpls_info_list = list_new ();
#endif /* HAVE_VPLS */

  return mif;
}

/* Lookup PacOS MPLS interface in specified master. */
struct nsm_mpls_if *
nsm_mpls_if_lookup_in (struct nsm_master *nm, struct interface *ifp)
{
  struct nsm_mpls_if *mif;
  struct interface *tmp_ifp;
  struct listnode *ln;

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return NULL;

  LIST_LOOP (NSM_MPLS->iflist, mif, ln)
    {
      /* Get interface object. */
      tmp_ifp = mif->ifp;
      if ((tmp_ifp != NULL) && (tmp_ifp->ifindex == ifp->ifindex) &&
          (pal_strcmp (tmp_ifp->name, ifp->name) == 0))
        return mif;
    }

  return NULL;
}

/* Lookup PacOS MPLS interface. */
struct nsm_mpls_if *
nsm_mpls_if_lookup (struct interface *ifp)
{
  if (ifp)
    return nsm_mpls_if_lookup_in (ifp->vr->proto, ifp);

  return NULL;
}

#ifdef HAVE_VRF
/* Create a VRF table in NSM. */
struct vrf_table *
nsm_mpls_vrf_table_init (struct nsm_master *nm, vrf_id_t id)
{
  struct vrf_table *table, *head;
  int arr_ix;

  if (! nm->nmpls || id == GLOBAL_FTN_ID)
    return NULL;

  /* Create. */
  table = XCALLOC (MTYPE_NSM_MPLS_VRF_TABLE, sizeof (struct vrf_table));
  if (! table)
    return NULL;

  /* Fill data. */
  table->vrf_id = id;
  nsm_ptree_ix_table_init (&table->vrf_ix_table4, IPV4_MAX_BITLEN,
                            NULL);
  table->vrf_ix_table4.id = id;
  table->vrf_ix_table4.m_table->id = id;

  if (! table->vrf_ix_table4.m_table || ! table->vrf_ix_table4.ix_mgr)
    {
      if (table->vrf_ix_table4.m_table)
        ptree_finish (table->vrf_ix_table4.m_table);
      else if (table->vrf_ix_table4.ix_mgr)
        bitmap_free (table->vrf_ix_table4.ix_mgr);

      XFREE (MTYPE_NSM_MPLS_VRF_TABLE, table);
      return NULL;
    }

#ifdef HAVE_IPV6
  nsm_ptree_ix_table_init (&table->vrf_ix_table6, IPV6_MAX_BITLEN,
                            table->vrf_ix_table6.ix_mgr);
  table->vrf_ix_table6.id = id;
  table->vrf_ix_table6.m_table->id = id;

  if (! table->vrf_ix_table6.m_table || ! table->vrf_ix_table6.ix_mgr)
    {
      if (table->vrf_ix_table4.m_table)
        ptree_finish (table->vrf_ix_table4.m_table);

      if (table->vrf_ix_table6.m_table)
        ptree_finish (table->vrf_ix_table6.m_table);

      if (table->vrf_ix_table4.ix_mgr)
        bitmap_free (table->vrf_ix_table4.ix_mgr);

      XFREE (MTYPE_NSM_MPLS_VRF_TABLE, table);
      return NULL;
    }
#endif

  /* Add to hash. */
  arr_ix = VRF_HASH_KEY (id);
  head = NSM_MPLS->vrf_hash[arr_ix];
  NSM_MPLS->vrf_hash[arr_ix] = table;
  table->next = head;

  return table;
}

/* Delete VRF table entry. */
void
nsm_mpls_vrf_table_free (struct vrf_table *table)
{
  if (! table)
    return;

  /* Clean up route_ix. */
  PTREE_IX_TABLE_CLEANUP (&table->vrf_ix_table4);

#ifdef HAVE_IPV6
  PTREE_IX_TABLE_CLEANUP (&table->vrf_ix_table6);

  table->vrf_ix_table6.ix_mgr = NULL;
#endif

  XFREE (MTYPE_NSM_MPLS_VRF_TABLE, table);
}

/* Remove table from NSM's scope. */
void
nsm_mpls_vrf_table_del (struct nsm_master *nm, int id)
{
  struct vrf_table *table, *head;
  int arr_ix;

  if (! NSM_MPLS || id == GLOBAL_FTN_ID)
    return;

  /* Lookup in hash. */
  arr_ix = VRF_HASH_KEY (id);
  head = NSM_MPLS->vrf_hash[arr_ix];
  for (table = head; table; table = table->next)
    {
      if (table->vrf_id == id)
        {
          /* Remove from list. */
          if (table->prev)
            table->prev->next = table->next;
          else
            NSM_MPLS->vrf_hash[arr_ix] = table->next;
          if (table->next)
            table->next->prev = table->prev;

          nsm_mpls_vrf_table_free (table);
          return;
        }
    }
}

/* Create a VRF in the Forwarder. */
int
nsm_mpls_vrf_add (struct nsm_master *nm, int vrf_id)
{
  struct vrf_table *table;
  int ret;

  /* Only create VRF tables for non GLOBAL FTNs. */
  if (vrf_id == GLOBAL_FTN_ID)
    return 0;

  /* Create new table. */
  table = nsm_mpls_vrf_table_init (nm, vrf_id);
  if (! table)
    ret = -1;
  else
    ret = apn_mpls_vrf_init (vrf_id);

  if (ret < 0)
    {
      zlog_warn (nzg, "Could not create VRF table with identifier %d "
                 "in the MPLS Forwarder", vrf_id);
      return ret;
    }
  else if (IS_NSM_DEBUG_EVENT)
    zlog_info (nzg, "Successfully created VRF table with identifier %d "
               "in the MPLS Forwarder", vrf_id);

  return 0;
}

/* Delete a VRF from the Forwarder. */
void
nsm_mpls_vrf_del (struct nsm_master *nm, int vrf_id)
{
  int ret;

  if (! NSM_MPLS)
    return;

  ret = apn_mpls_vrf_end (vrf_id);
  if (ret < 0)
    zlog_warn (NSM_ZG, "Could not delete VRF table with identifier %d "
               "from the MPLS Forwarder", vrf_id);
  else if (IS_NSM_DEBUG_EVENT)
    zlog_info (NSM_ZG, "Successfully deleted VRF table with identifier %d "
               "from the MPLS Forwarder", vrf_id);

  nsm_gmpls_ftn_cleanup_all (nm, vrf_id, NSM_TRUE);

  /* Remove table from NSM's scope. */
  nsm_mpls_vrf_table_del (nm, vrf_id);
}


/* Bind interface to VRF in Forwarder. */
int
nsm_mpls_if_vrf_bind (struct nsm_mpls_if *mif, int vrf_id)
{
  struct if_ident ident;
  int ret;

  if (vrf_id == mif->vrf_id)
    return 0;

  /* Update mif. */
  mif->vrf_id = vrf_id;

  /* Fill ident. */
  pal_mem_set (&ident, 0, sizeof (struct if_ident));
  ident.if_index = mif->ifp->ifindex;
  pal_mem_cpy (ident.if_name, mif->ifp->name, INTERFACE_NAMSIZ);

  ret = apn_mpls_if_update_vrf (&ident, (vrf_id > 0 ? vrf_id : -1));
  if (ret < 0)
    {
      zlog_warn (NSM_ZG, "Could not bind interface %s with index %d "
                 "to VRF with identifier %d", mif->ifp->name,
                 mif->ifp->ifindex, mif->vrf_id);
      return ret;
    }
  else
    {
      if (IS_NSM_DEBUG_EVENT)
        zlog_info (NSM_ZG, "Bound interface %s with index %d to VRF "
                   "with identifier %d", mif->ifp->name,
                   mif->ifp->ifindex, mif->vrf_id);
    }

  return 0;
}

/* Bind interface to VRF in Forwarder by ifp */
int
nsm_mpls_if_vrf_bind_by_ifp (struct interface *ifp, int vrf_id)
{
  struct nsm_mpls_if *mif;
  int ret;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    {
      zlog_warn (NSM_ZG, "Interface %s with index %d was not found in "
                 "the MPLS interface list.", ifp->name, ifp->ifindex);
      return -1;
    }

  /* Bind vrf. */
  ret = nsm_mpls_if_vrf_bind (mif, vrf_id);

  return ret;
}

/* Unbind interface from VRF in Forwarder by ifp */
int
nsm_mpls_if_vrf_unbind_by_ifp (struct interface *ifp)
{
  struct nsm_mpls_if *mif;
  int ret;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    {
      zlog_warn (NSM_ZG, "Interface %s with index %d was not found in "
                 "the MPLS interface list.", ifp->name, ifp->ifindex);
      return -1;
    }

  /* Unbind vrf. */
  ret = nsm_mpls_if_vrf_bind (mif, VRF_ID_UNSPEC);

  return ret;
}
#endif /* HAVE_VRF */

/* Move MPLS interface structure associated with 'ifp' to MPLS iflist in
   the passed-in nsm_master. If nsm_master is NULL, move interface to
   MPLS iflist for privileged VR. */
int
nsm_mpls_if_binding_update (struct interface *ifp,
                            struct nsm_master *nm_old,
                            struct nsm_master *nm)
{
  struct nsm_mpls_if *mif;
  struct listnode *ln;
#ifdef HAVE_VRF
  int ret;
#endif /* HAVE_VRF */

  NSM_ASSERT (ifp != NULL && nm_old != NULL && nm != NULL);
  if (! ifp || ! nm_old || ! nm)
    return NSM_FAILURE;

  /* Get MPLS Interface. */
  mif = nsm_mpls_if_lookup_in (nm_old, ifp);
  if (! mif)
    return NSM_FAILURE;

#ifdef HAVE_VRF
  /* If this interface is bound to a VRF, break the binding. */
  ret = nsm_mpls_if_vrf_bind (mif, VRF_ID_UNSPEC);
  if (ret < 0)
    return ret;
#endif /* HAVE_VRF */

  /* Add to new iflist and remove from old. */
  ln = listnode_add (nm->nmpls->iflist, (void *)mif);
  if (! ln)
    return NSM_ERR_MEM_ALLOC_FAILURE;
  list_delete_node (nm_old->nmpls->iflist, mif->ln);
  mif->ln = ln;
  mif->nm = nm;

  return NSM_SUCCESS;
}

/* Initialize label space object. */
struct nsm_label_space *
nsm_mpls_label_space_init (struct nsm_master *nm, u_int16_t label_space)
{
  struct nsm_label_space *nls;

  if ((! NSM_MPLS) || (! NSM_MPLS->ls_table))
    return NULL;

  /* Create and return. */
  nls = XCALLOC (MTYPE_NSM_LABEL_SPACE, sizeof (struct nsm_label_space));
  if (! nls)
    return NULL;

  nls->label_space = label_space;
  nls->min_label_val = NSM_MPLS->min_label_val;
  nls->max_label_val = NSM_MPLS->max_label_val;
  nls->static_block = NSM_FALSE;

  nls->nm = nm;

  return nls;
}

/* Lookup a label space object. */
struct nsm_label_space *
nsm_mpls_label_space_lookup (struct nsm_master *nm, u_int16_t label_space)
{
  struct route_node *rn;
  struct prefix p;

  if ((! NSM_MPLS) || (! NSM_MPLS->ls_table))
    return NULL;

  /* Fill prefix. */
  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET;
  p.prefixlen = MAX_LABEL_SPACE_BITLEN;
  PREP_FOR_NETWORK (p.u.prefix4.s_addr, label_space, MAX_LABEL_SPACE_BITLEN);

  /* Lookup label space object. Create if not present. */
  rn = route_node_lookup (NSM_MPLS->ls_table, &p);
  if (rn)
    {
      route_unlock_node (rn);
      return (struct nsm_label_space *)rn->info;
    }

  return NULL;
}

/* Get label space object. */
struct nsm_label_space *
nsm_mpls_label_space_get (struct nsm_master *nm, u_int16_t label_space)
{
  struct nsm_label_space *nls;
  struct route_node *rn;
  struct prefix p;

  if ((! NSM_MPLS) || (! NSM_MPLS->ls_table))
    return NULL;

  /* Fill prefix. */
  pal_mem_set (&p, 0, sizeof (struct prefix));
  p.family = AF_INET;
  p.prefixlen = MAX_LABEL_SPACE_BITLEN;
  PREP_FOR_NETWORK (p.u.prefix4.s_addr, label_space, MAX_LABEL_SPACE_BITLEN);

  /* Lookup label space object. Create if not present. */
  rn = route_node_lookup (NSM_MPLS->ls_table, &p);
  if (! rn)
    {
      /* Create label space object. */
      nls = nsm_mpls_label_space_init (nm, label_space);
      if (! nls)
        return NULL;

      /* Get new and set. */
      rn = route_node_get (NSM_MPLS->ls_table, &p);
      rn->info = nls;
      nls->rn = rn;
    }
  else
    {
      route_unlock_node (rn);
      nls = rn->info;
    }

  NSM_ASSERT (nls != NULL);
  return nls;
}

/* Special handling for platform-wide labelspace. Create a labelspace
   object for labelspace 0 at init time. */
void
nsm_mpls_platform_ls_init (struct nsm_master *nm)
{
  struct nsm_label_space *nls;

  /* Get label space. */
  nls = nsm_mpls_label_space_get (nm, 0);
  NSM_ASSERT (nls != NULL);
  if (! nls)
    {
      zlog_err (NSM_ZG, "Internal Error : Platform-wide label space enabling "
                "procedure failure");
      return;
    }

  /* Increment refcount. */
  nls->refcount++;
}

/* Destroy label space object. */
void
nsm_mpls_label_space_del (struct nsm_label_space *nls)
{
  struct route_node *rn;

  /* Get route node and delete it. */
  rn = nls->rn;
  rn->info = NULL;
  route_unlock_node (rn);

  /* Free memory. */
  XFREE (MTYPE_NSM_LABEL_SPACE, nls);
}

/* Special handling for platform-wide labelspace. */
void
nsm_mpls_platform_ls_deinit (struct nsm_master *nm)
{
  struct nsm_label_space *nls;

  /* Get label space. */
  nls = nsm_mpls_label_space_get (nm, 0);
  if (! nls)
    return;

  /* Decrement refcount. This will be done after all interfaces have
     been deleted, so refcount should be zero afterwards. */
  --nls->refcount;
  NSM_ASSERT (nls->refcount == 0);
  if (nls->refcount != 0)
    {
      zlog_err (NSM_ZG, "Internal Error: Platform-wide label space "
                "deinit failure");
      return;
    }

  if (NSM_LABEL_SPACE_UNUSED (nls))
    nsm_mpls_label_space_del (nls);
}

/* Register an interface for a label space. */
void
nsm_mpls_if_register_for_ls (struct nsm_mpls_if *mif, u_int16_t label_space)
{
  struct nsm_label_space *nls;

  /* Get label space. */
  nls = nsm_mpls_label_space_get (mif->nm, label_space);
  NSM_ASSERT (nls != NULL);
  if (! nls)
    {
      zlog_err (NSM_ZG, "Internal Error: Could not create object for "
                "label space %d", label_space);
      return;
    }

  /* Set data. */
  mif->nls = nls;
  nls->refcount++;
}
/* Register an interface from a label space. */
void
nsm_mpls_if_unregister_from_ls (struct nsm_mpls_if *mif)
{
  struct nsm_label_space *nls;

  /* Get label space. */
  nls = mif->nls;
  NSM_ASSERT (nls != NULL);
  if (! nls)
    {
      zlog_err (NSM_ZG, "Internal Error: Could not get object for label "
                "space while unregistering MPLS interface");
      return;
    }

  /* Update nls. */
  nls->refcount--;
  mif->nls = NULL;

  if (NSM_LABEL_SPACE_UNUSED (nls))
    nsm_mpls_label_space_del (nls);
}

/* Enable interface for MPLS. */
int
nsm_mpls_if_enable (struct nsm_mpls_if *mif, u_int16_t label_space)
{
  struct nsm_master *nm = mif->nm;
  cindex_t cindex = 0;
#ifdef HAVE_MPLS_VC
  struct nsm_mpls_vc_info *vc_info = NULL;
  struct listnode *ln;
#endif /* HAVE_MPLS_VC */

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return -1;

  /* Loopback interface check, return err if loopback */
  if (! mif->ifp || if_is_loopback (mif->ifp))
    {
      zlog_warn (NSM_ZG, "ERROR: Invalid Interface for MPLS: %s\n",
                 mif->ifp ? mif->ifp->name : "N/A");
      return -1;
    }
  /* If this is a change in label space, unregister from old. */
  if (mif->nls)
    {
      if (mif->nls->label_space != label_space && mif->ifp)
        {
          if (mif->nls->refcount == 1 && mif->nls->static_block == NSM_TRUE)
            {
              nsm_lp_static_block_release (nm, mif->nls, APN_PROTO_NSM);
              mif->nls->static_block = NSM_FALSE;
            }
          nsm_mpls_rib_if_down_process (mif->ifp);

          nsm_mpls_if_unregister_from_ls (mif);
          nsm_mpls_if_register_for_ls (mif, label_space);
        }
      else
        return 0;
    }
  else
    nsm_mpls_if_register_for_ls (mif, label_space);

  if (! mif->nls)
    return -1;

  NSM_ASSERT (mif->ifp != NULL);
  if (! mif->ifp)
    {
      zlog_err (NSM_ZG, "Internal Error: Interface object mismatch while "
                "enabling interface for label-switching");
      return -1;
    }

  nsm_mpls_ifp_set (mif, mif->ifp);
  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_LABEL_SPACE);
  if (if_is_running (mif->ifp))
    nsm_server_if_up_update (mif->ifp, cindex);
  else
    nsm_server_if_down_update (mif->ifp, cindex);

  {
    struct if_ident ident;
    int ret;

    /* Fill ident. */
    pal_mem_set (&ident, 0, sizeof (struct if_ident));
    ident.if_index = mif->ifp->ifindex;
    pal_mem_cpy (ident.if_name, mif->ifp->name, INTERFACE_NAMSIZ);

    ret = apn_mpls_if_init (&ident, mif->nls->label_space);
    if (ret < 0)
      {
        zlog_warn (NSM_ZG, "Could not enable interface %s with index %d "
                   "for MPLS", mif->ifp->name, mif->ifp->ifindex);
        return ret;
      }
    else
      {
        if (IS_NSM_DEBUG_EVENT)
          zlog_info (NSM_ZG, "Enabled interface %s with index %d for MPLS",
                     mif->ifp->name, mif->ifp->ifindex);

        nsm_mpls_rib_if_up_process (mif->ifp);
#ifdef HAVE_MPLS_VC
        LIST_LOOP (mif->vc_info_list, vc_info, ln)
          {
            if ((! CHECK_FLAG (vc_info->if_bind_type,  NSM_IF_BIND_MPLS_VC))
                && (! CHECK_FLAG (vc_info->if_bind_type,
                                  NSM_IF_BIND_MPLS_VC_VLAN)))
              continue;

            if (vc_info->u.vc)
              nsm_mpls_if_install_vc_data (mif, vc_info->u.vc,
                                           vc_info->vlan_id);
          }
#endif /* HAVE_MPLS_VC */
      }
  }

  if (mif->nls && mif->nls->static_block == NSM_FALSE)
    {
      nsm_lp_static_block_get (nm, mif->nls, APN_PROTO_NSM);
      mif->nls->static_block = NSM_TRUE;
    }
  return 0;
}

/* Disable interface for MPLS. */
void
nsm_mpls_if_disable (struct nsm_mpls_if *mif)
{
  struct nsm_master *nm = mif->nm;
  struct interface *ifp;
  cindex_t cindex = 0;
  bool_t last_ls = NSM_FALSE;
#ifdef HAVE_MPLS_VC
  struct nsm_mpls_circuit *vc;
  struct route_node *rn;
  int ret = 0;
#endif /* HAVE_MPLS_VC */

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return;

  /* NLS object MUST be there. If not, this interface is already disabled. */
  if (mif->nls == NULL)
    return;

  /* If label-space is undefined, return. */
  ifp = mif->ifp;
  if (! ifp)
    return;

  if ((mif->nls->label_space == 0 && mif->nls->refcount == 2) ||
      (mif->nls->refcount == 1))
    last_ls = NSM_TRUE;

  if (last_ls && mif->nls->static_block == NSM_TRUE)
    {
      nsm_lp_static_block_release (nm, mif->nls, APN_PROTO_NSM);
      mif->nls->static_block = NSM_FALSE;
    }

  if (ifp->ls_data.status == LABEL_SPACE_VALID)
    {
      struct if_ident ident;

      /* Fill ident. */
      pal_mem_set (&ident, 0, sizeof (struct if_ident));
      ident.if_index = ifp->ifindex;
      pal_mem_cpy (ident.if_name, ifp->name, INTERFACE_NAMSIZ);

#ifdef HAVE_MPLS_VC
      if (LISTCOUNT (mif->vc_info_list) == 0)
        {
          ret = apn_mpls_if_end (&ident);
          if (ret < 0)
            {
              zlog_warn (NSM_ZG, "Could not disable interface %s with index %d "
                         "for MPLS", ifp->name, ifp->ifindex);
            }
          else
            {
              if (IS_NSM_DEBUG_EVENT)
                zlog_info (NSM_ZG, "Disabled interface %s with index %d for MPLS",
                           ifp->name, ifp->ifindex);
            }
        }
      else /* Case of mif binded to VC */
        {
          if (mif->nls->label_space != PLATFORM_WIDE_LABEL_SPACE)
            {
              /* Loopback interface check, return err if loopback */
              if (if_is_loopback (mif->ifp))
                {
                  zlog_warn (NSM_ZG, "ERROR:Invalid Interface for MPLS: %s\n",
                             mif->ifp->name);
                  return;
                }
              ret = apn_mpls_if_init (&ident, PLATFORM_WIDE_LABEL_SPACE);
              if (ret < 0)
                {
                  zlog_warn (NSM_ZG, "Could not enable interface %s with index"
                             " %d for MPLS", mif->ifp->name, mif->ifp->ifindex);
                }
              else if (IS_NSM_DEBUG_EVENT)
                zlog_info (NSM_ZG, "Enabled interface %s with index %d for MPLS",
                           mif->ifp->name, mif->ifp->ifindex);
            }
        }
#endif /* HAVE_MPLS_VC */
    }

#ifdef HAVE_MPLS_VC
  if (NSM_MPLS->vc_table != NULL)
    for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
      {
        /* Get vc */
        vc = rn->info;

        /* If no data, skip. */
        if (! vc)
          continue;

         if((vc->fec_type_vc == PW_OWNER_MANUAL) && (vc->vc_fib))
           {
#ifdef HAVE_GMPLS
            if (ifp->gifindex == vc->vc_fib->nw_if_ix)
#else
            if (ifp->ifindex == vc->vc_fib->nw_if_ix)
#endif /* HAVE_GMPLS */
              {
                 nsm_mpls_vc_fib_cleanup(nm, vc, NSM_TRUE);
                 if (vc->state >= NSM_MPLS_L2_CIRCUIT_COMPLETE)
                    vc->state = NSM_MPLS_L2_CIRCUIT_ACTIVE;
              }
           }
      }
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_MPLS_FWD
  /* Clear mpls interface performance data. */
  nsm_mpls_if_stats_clear_in_labels_used (ifp);
  nsm_mpls_if_stats_clear_out_labels_used (ifp);
#endif /* HAVE_MPLS_FWD */

  /* Unset label space. */
  nsm_mpls_if_unregister_from_ls (mif);

  /* We unset before we send an UP here, because we want the ifp structure
     to have the latest labelspace status value etc. */
  nsm_mpls_ifp_unset (ifp);

  nsm_mpls_rib_if_down_process (ifp);

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_LABEL_SPACE);

  if (NSM_MPLS->shutdown == NSM_FALSE)
    {
      /* Announce change. */
      if (if_is_running (ifp))
        nsm_server_if_up_update (ifp, cindex);
      else
        nsm_server_if_down_update (ifp, cindex);
    }
}

/* Enable interface for MPLS using ifp. */
int
nsm_mpls_if_enable_by_ifp (struct interface *ifp, u_int16_t label_space)
{
  struct nsm_master *nm = ifp->vr->proto;
  struct nsm_mpls_if *mif;
  int ret;

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return NSM_SUCCESS;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    {
      zlog_warn (NSM_ZG, "Interface %s with index %d was not found in "
                 "the MPLS interface list.", ifp->name, ifp->ifindex);
      return NSM_FAILURE;
    }

  /* Enable interface. */
  if (if_is_loopback (ifp))
    return NSM_ERR_LS_ON_LOOPBACK;

  ret = nsm_mpls_if_enable (mif, label_space);
  if (ret < 0)
    return ret;

  return NSM_SUCCESS;
}

/* Disable interface for MPLS using ifp. */
int
nsm_mpls_if_disable_by_ifp (struct interface *ifp)
{
  struct nsm_master *nm = ifp->vr->proto;
  struct nsm_mpls_if *mif;

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return NSM_SUCCESS;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    {
      zlog_warn (NSM_ZG, "Interface %s with index %d was not found in "
                 "the MPLS interface list.", ifp->name, ifp->ifindex);
      return NSM_FAILURE;
    }

  /* Disable interface. */
  if (if_is_loopback (ifp))
    return NSM_ERR_LS_ON_LOOPBACK;

  nsm_mpls_if_disable (mif);
  return NSM_SUCCESS;
}

/* Enable all interface for MPLS. */
void
nsm_mpls_enable_all_interfaces (struct nsm_master *nm,
                                u_int16_t label_space)
{
  struct nsm_mpls_if *mif;
  struct listnode *ln;

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return;

  /* Set flag. */
  SET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_ENABLE_ALL_IFS);

  LIST_LOOP (NSM_MPLS->iflist, mif, ln)
    if ((mif->ifp != NULL) && (! if_is_loopback (mif->ifp)))
      nsm_mpls_if_enable (mif, label_space);
}

/* Disable all interface for MPLS. */
void
nsm_mpls_disable_all_interfaces (struct nsm_master *nm)
{
  struct nsm_mpls_if *mif;
  struct listnode *ln;

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return;

  /* Unset flag. */
  UNSET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_ENABLE_ALL_IFS);

  LIST_LOOP (NSM_MPLS->iflist, mif, ln)
    {
      if (! mif->ifp || if_is_loopback (mif->ifp))
        continue;

      if (mif->nls)
        nsm_mpls_if_disable (mif);
    }
}

/* Interface ADD handling. */
struct nsm_mpls_if *
nsm_mpls_if_add (struct interface *ifp)
{
  struct nsm_master *nm = ifp->vr->proto;
  struct nsm_mpls_if *mif;
  struct listnode *ln;

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return NULL;

  /* If already there, return. */
  mif = nsm_mpls_if_lookup (ifp);
  if (mif)
    return mif;

  /* New interface. */
  mif = nsm_mpls_if_new (ifp->vr->proto);
  if (! mif)
    return NULL;

  mif->ifp = ifp;

  /* Add to linklist. */
  ln = listnode_add (NSM_MPLS->iflist, (void *)mif);

  /* Set back pointer. */
  mif->ln = ln;

  /* If enable-all-interfaces flag is set, enable this interface
     for label-switching with label-space zero. */
  if (CHECK_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_ENABLE_ALL_IFS))
    nsm_mpls_if_enable (mif, 0);

  return mif;
}

/* Helper function to set pacos interface's label space data. */
void
nsm_mpls_ifp_set (struct nsm_mpls_if *mif, struct interface *ifp)
{
    enum label_pool_id ctr = LABEL_POOL_SERVICE_ERROR;

  if (mif->nls)
    {
      /* Enable labelspace. */
      ifp->ls_data.status          = LABEL_SPACE_VALID;

      /* Copy over label_space data to ifp. */
      ifp->ls_data.label_space     = mif->nls->label_space;
      ifp->ls_data.min_label_value = mif->nls->min_label_val;
      ifp->ls_data.max_label_value = mif->nls->max_label_val;

      for (ctr = LABEL_POOL_RANGE_INITIAL; ctr < LABEL_POOL_RANGE_MAX;
           ctr++)
        {
          ifp->ls_data.ls_module_ranges[ctr].from_label =
                                    mif->nls->service_ranges[ctr].from_label;
          ifp->ls_data.ls_module_ranges[ctr].to_label =
                                    mif->nls->service_ranges[ctr].to_label;
        }
    }
}

/* Helper function to unset pacos interface's label space data. */
void
nsm_mpls_ifp_unset (struct interface *ifp)
{
    enum label_pool_id ctr = LABEL_POOL_SERVICE_ERROR;

  /* Disable labelspace. */
  ifp->ls_data.status          = LABEL_SPACE_INVALID;

  ifp->ls_data.label_space     = PLATFORM_WIDE_LABEL_SPACE;
  ifp->ls_data.min_label_value = LABEL_VALUE_INITIAL;
  ifp->ls_data.max_label_value = LABEL_VALUE_MAX;

  for (ctr = LABEL_POOL_RANGE_INITIAL; ctr < LABEL_POOL_RANGE_MAX; ctr++)
    {
      ifp->ls_data.ls_module_ranges[ctr].from_label = 0;
      ifp->ls_data.ls_module_ranges[ctr].to_label = 0;
    }
}

/* Interface DELETE handling. */
void
nsm_mpls_if_del (struct interface *ifp, bool_t announce)
{
  struct nsm_master *nm = ifp->vr->proto;
  struct nsm_mpls_if *mif;
  struct listnode *ln;
  cindex_t cindex = 0;
#ifdef HAVE_MPLS_VC
  struct nsm_mpls_vc_info *vc_info = NULL;
#endif /* HAVE_MPLS_VC */

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    {
      zlog_warn (NSM_ZG, "Interface %s with index %d was not found in "
                 "the MPLS interface list.", ifp->name, ifp->ifindex);
      return;
    }

#ifdef HAVE_MPLS_VC
  /* Unbind MPLS VC.  */
    if (mif->vc_info_list)
    {
      for (ln = mif->vc_info_list->head; ln;)
      {
        if ((vc_info = ln->data) != NULL)
        {
          ln = ln->next;
          if ((! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC)) &&
            (! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC_VLAN)))
            continue;

          if (vc_info->u.vc)
            nsm_mpls_l2_circuit_unbind_by_ifp (ifp, vc_info->vc_name,
                                           vc_info->vlan_id);
        }
        else
          ln = ln->next;
      }

      list_delete (mif->vc_info_list);
      mif->vc_info_list = NULL;
    }
#endif /* HAVE_MPLS_VC */

  /* Get list node. */
  ln = mif->ln;

  /* Free structure. */
  XFREE (MTYPE_NSM_MPLS_IF, mif);

  /* Free list node. */
  if (NSM_MPLS && NSM_MPLS->iflist)
    list_delete_node (NSM_MPLS->iflist, ln);

  /* Unset label_space data from ifp. */
  nsm_mpls_ifp_unset (ifp);

  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_LABEL_SPACE);

  /* Announce to protocols if required. */
  if (announce == NSM_TRUE)
    {
      if (if_is_running (ifp))
        nsm_server_if_up_update (ifp, cindex);
      else
        nsm_server_if_down_update (ifp, cindex);
    }
}

/* Interface UP handling. */
void
nsm_mpls_if_up (struct nsm_mpls_if *mif)
{
#ifdef HAVE_MPLS_VC
  struct nsm_mpls_vc_info *vc_info = NULL;
  struct listnode *ln;
#endif /* HAEV_MPLS_VC */

  if (! mif->ifp)
    return;

  nsm_mpls_rib_if_up_process (mif->ifp);

#ifdef HAVE_MPLS_VC
  if (if_is_up (mif->ifp))
    LIST_LOOP (mif->vc_info_list, vc_info, ln)
    {
      if ((! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC)) &&
          (! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC_VLAN)))
        continue;

      if (vc_info->u.vc)
        {  
#ifdef HAVE_MS_PW
      /* Process ONLY for SS-PW.
       * In case of MS-PW the vc is stitched and not "bound"
       * Both stitching and binding use the same vc_info data struct.
       */
          if (vc_info->u.vc->ms_pw == NULL)
#endif /* HAVE_MS_PW */
            nsm_mpls_vc_if_up_process (vc_info->u.vc, mif);
        }
    }
#endif /* HAVE_MPLS_VC */

}

/* Interface DOWN handling. */
void
nsm_mpls_if_down (struct interface *ifp)
{
  struct nsm_master *nm = ifp->vr->proto;
  struct nsm_mpls_if *mif;

  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return;

  /* Lookup interface. */
  mif = nsm_mpls_if_lookup (ifp);
  if (! mif)
    {
      zlog_warn (NSM_ZG, "Interface %s with index %d was not found in "
                 "the MPLS interface list.", ifp->name, ifp->ifindex);
      return;
    }

#ifdef HAVE_MPLS_VC
  if (mif->ifp)
    {
      struct nsm_mpls_vc_info *vc_info = NULL;
      struct listnode *ln;

      LIST_LOOP (mif->vc_info_list, vc_info, ln)
        {
          if ((! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC)) &&
              (! CHECK_FLAG (vc_info->if_bind_type, NSM_IF_BIND_MPLS_VC_VLAN)))
            continue;

          if (vc_info->u.vc)
            {
#ifdef HAVE_MS_PW
              /* Process ONLY for SS-PW.
               * In case of MS-PW the vc is stitched and not "bound"
               * Both stitching and binding use the same vc_info data struct.
               */
              if (vc_info->u.vc->ms_pw == NULL)
#endif /* HAVE_MS_PW */
                {
                  nsm_mpls_vc_if_down_process (vc_info->u.vc, mif);
                }
            }
        }
    }
#endif /* HAVE_MPLS_VC */
}

/* Delete all interfaces. */
void
nsm_mpls_if_delete_all (struct nsm_master *nm, int announce)
{
  struct nsm_mpls_if *mif;
  struct listnode *ln, *ln_next;

  for (ln = LISTHEAD (NSM_MPLS->iflist); ln; ln = ln_next)
    {
      /* Get next. */
      ln_next = ln->next;

      /* Get interface object. */
      mif = ln->data;

      /* Delete MPLS interface. */
      nsm_mpls_if_del (mif->ifp, announce);
    }
}

#if defined (HAVE_MPLS_FWD)
/* mpls interface performance */
void
nsm_mpls_if_stats_incr_in_labels_used (struct nsm_master *nm,
                                       struct interface *ifp)
{
  struct nsm_mpls_if *mif;

  mif = nsm_mpls_if_lookup (ifp);
  if (mif)
    {
      if (ifp->ls_data.label_space == PLATFORM_WIDE_LABEL_SPACE)
        nm->nmpls->if_stats.in_labels_used++;
      else
        mif->stats.in_labels_used++;
    }
}

void
nsm_mpls_if_stats_decr_in_labels_used (struct nsm_master *nm,
                                       struct interface *ifp)
{
  struct nsm_mpls_if *mif;

  mif = nsm_mpls_if_lookup (ifp);
  if (mif)
    {
      if (ifp->ls_data.label_space == PLATFORM_WIDE_LABEL_SPACE)
        {
          if (nm->nmpls->if_stats.in_labels_used > 0)
            nm->nmpls->if_stats.in_labels_used--;
        }
      else
        {
          if (mif->stats.in_labels_used > 0)
            mif->stats.in_labels_used--;
        }
    }
}

void
nsm_mpls_if_stats_clear_in_labels_used (struct interface *ifp)
{
  struct nsm_mpls_if *mif;

  mif = nsm_mpls_if_lookup (ifp);
  if (mif)
    mif->stats.in_labels_used = 0;
}

void
nsm_mpls_if_stats_incr_out_labels_used (struct interface *ifp)
{
  struct nsm_mpls_if *mif;

  mif = nsm_mpls_if_lookup (ifp);
  if (mif)
    mif->stats.out_labels_used++;
}

void
nsm_mpls_if_stats_decr_out_labels_used (struct interface *ifp)
{
  struct nsm_mpls_if *mif;

  mif = nsm_mpls_if_lookup (ifp);
  if (mif)
    if (mif->stats.out_labels_used > 0)
      mif->stats.out_labels_used--;
}

void
nsm_mpls_if_stats_clear_out_labels_used (struct interface *ifp)
{
  struct nsm_mpls_if *mif;

  mif = nsm_mpls_if_lookup (ifp);
  if (mif)
    mif->stats.out_labels_used = 0;
}
#endif /* HAVE_MPLS_FWD */

/* Enable logging. */
void
nsm_mpls_log (struct nsm_master *nm, u_char val)
{
  /* If no change, return. */
  if ((val != NSM_MPLS_SHOW_MSG_ALL) &&
      (CHECK_FLAG (NSM_MPLS->kern_msgs, val)))
    return;

  /* If all log types specified. */
  if (val == NSM_MPLS_SHOW_MSG_ALL)
    {
      SET_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_ERROR);
      SET_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_WARNING);
      SET_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_DEBUG);
      SET_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_NOTICE);
    }
  else
    /* Set single type. */
    SET_FLAG (NSM_MPLS->kern_msgs, val);

  {
    u_char msg_type = 0;
    int ret;

    switch (val)
      {
      case NSM_MPLS_SHOW_MSG_ALL:
        msg_type = msg_type | KERN_MSG_ERROR;
        msg_type = msg_type | KERN_MSG_WARNING;
        msg_type = msg_type | KERN_MSG_DEBUG;
        msg_type = msg_type | KERN_MSG_NOTICE;
        break;
      case NSM_MPLS_SHOW_MSG_ERROR:
        msg_type = msg_type | KERN_MSG_ERROR;
        break;
      case NSM_MPLS_SHOW_MSG_WARNING:
        msg_type = msg_type | KERN_MSG_WARNING;
        break;
      case NSM_MPLS_SHOW_MSG_DEBUG:
        msg_type = msg_type | KERN_MSG_DEBUG;
        break;
      case NSM_MPLS_SHOW_MSG_NOTICE:
        msg_type = msg_type | KERN_MSG_NOTICE;
        break;
      }

    ret = apn_mpls_debugging_handle (APN_PROTO_NSM, msg_type, MPLS_INIT);
    if (ret < 0)
      zlog_warn (NSM_ZG, "Could not enable logging of messages in the "
                 "MPLS Forwarder");
    else if (IS_NSM_DEBUG_EVENT)
      zlog_info (NSM_ZG, "Successfully enabled logging of messages in the "
                 "MPLS Forwarder");
  }
}

/* Disable logging. */
void
nsm_mpls_no_log (struct nsm_master *nm, u_char val)
{
  /* If no change, return. */
  if ((val != NSM_MPLS_SHOW_MSG_ALL) &&
      (! CHECK_FLAG (NSM_MPLS->kern_msgs, val)))
    return;

  /* If all log types specified. */
  if (val == NSM_MPLS_SHOW_MSG_ALL)
    {
      UNSET_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_ERROR);
      UNSET_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_WARNING);
      UNSET_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_DEBUG);
      UNSET_FLAG (NSM_MPLS->kern_msgs, NSM_MPLS_SHOW_MSG_NOTICE);
    }
  else
    /* Unset single type. */
    UNSET_FLAG (NSM_MPLS->kern_msgs, val);

  {
    u_char msg_type = 0;
    int ret;

    switch (val)
      {
      case NSM_MPLS_SHOW_MSG_ALL:
        msg_type = msg_type & ~KERN_MSG_ERROR;
        msg_type = msg_type & ~KERN_MSG_WARNING;
        msg_type = msg_type & ~KERN_MSG_DEBUG;
        msg_type = msg_type & ~KERN_MSG_NOTICE;
        break;
      case NSM_MPLS_SHOW_MSG_ERROR:
        msg_type = msg_type & ~KERN_MSG_ERROR;
        break;
      case NSM_MPLS_SHOW_MSG_WARNING:
        msg_type = msg_type & ~KERN_MSG_WARNING;
        break;
      case NSM_MPLS_SHOW_MSG_DEBUG:
        msg_type = msg_type & ~KERN_MSG_DEBUG;
        break;
      case NSM_MPLS_SHOW_MSG_NOTICE:
        msg_type = msg_type & ~KERN_MSG_NOTICE;
        break;
      }

    ret = apn_mpls_debugging_handle (APN_PROTO_NSM, msg_type, MPLS_END);
    if (ret < 0)
      zlog_warn (NSM_ZG, "Could not disable logging of messages in the "
                 "MPLS Forwarder");
    else if (IS_NSM_DEBUG_EVENT)
      zlog_info (NSM_ZG, "Successfully disabled logging of messages in the "
                 "MPLS Forwarder");
  }
}

/* Get maximum label value configured */
u_int32_t
nsm_mpls_max_label_val_get (struct nsm_master *nm)
{
  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return 0;

  return NSM_MPLS->max_label_val;
}

/* Get maximum label value configured. */
u_int32_t
nsm_mpls_min_label_val_get (struct nsm_master *nm)
{
  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return 0;

  return NSM_MPLS->min_label_val;
}

#ifdef HAVE_VPLS
void
nsm_vpls_cleanup_all (struct nsm_master *nm)
{
  struct ptree_node *pn;
  struct nsm_vpls *vpls;

  if (! NSM_MPLS->vpls_table)
    return;

  for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
    {
      if (! pn->info)
        continue;
      vpls = pn->info;

      nsm_vpls_group_unregister (nm, vpls);
      nsm_vpls_cleanup (nm, vpls);
      nsm_vpls_free (nm, vpls);
      pn->info = NULL;
      ptree_unlock_node (pn);
    }
}
#endif /* HAVE_VPLS */

#ifdef HAVE_NSM_MPLS_OAM
int
mpls_netlink_recv_process (u_char *buf, int len, struct mpls_oam_ctrl_data *ctrl_data)
{
  struct nsm_master *nm;

  nm = nsm_master_lookup_by_id (NSM_ZG, 0);
  nsm_mpls_oam_packet_read (nm, buf, len, ctrl_data, NULL);
#ifdef HAVE_MPLS_OAM_ITUT
  nsm_mpls_oam_itut_packet_read(nm, buf, len, ctrl_data);
#endif /* HAVE_MPLS_OAM_ITUT */
  return 0;
}
#endif /* HAVE_NSM_MPLS_OAM */

/* Initialize top-level MPLS data. */
int
nsm_mpls_main_init ()
{
  int ret;

#ifdef HAVE_NSM_MPLS_OAM
  apn_mpls_register_oam_callback (mpls_netlink_recv_process);
#endif /* HAVE_NSM_MPLS_OAM */
  /* Initialize connection to the MPLS Forwarder. */
  ret = apn_mpls_init_all_handles (NSM_ZG, APN_PROTO_NSM);

  if (ret < 0)
    return ret;

  /* Initialize the NSM MPLS commands. */
  nsm_mpls_init_commands ();

  return 0;
}

static s_int32_t
nsm_mpls_cmp_pkt_ilm (void *data1, void* data2)
{
  struct ilm_entry *ilm1, *ilm2;

  if (data1 == NULL || data2 == NULL)
    return -1;

  ilm1 = (struct ilm_entry *) data1;
  ilm2 = (struct ilm_entry *) data2;

  if (ilm1->key.u.pkt.iif_ix > ilm2->key.u.pkt.iif_ix)
    return 1;
  else if (ilm1->key.u.pkt.iif_ix < ilm2->key.u.pkt.iif_ix)
    return -1;

  if (ilm1->key.u.pkt.in_label > ilm2->key.u.pkt.in_label)
    return 1;
  else if (ilm1->key.u.pkt.in_label < ilm2->key.u.pkt.in_label)
    return -1;

  return 0;
}

s_int32_t
nsm_mpls_cmp_gen_label (struct gmpls_gen_label *lbl1,
                        struct gmpls_gen_label *lbl2)
{
#ifdef HAVE_GMPLS
#if defined (HAVE_PBB_TE) || defined (HAVE_TDM)
  s_int32_t ret;
#endif /* HAVE_PBB_TE || HAVE_TDM */
#endif /* HAVE_GMPLS */

  if (lbl1 == NULL || lbl2 == NULL)
    return -1;

  if (lbl1->type > lbl1->type)
    return 1;
  else if (lbl1->type > lbl1->type)
    return -1;

  switch (lbl1->type)
    {
      case gmpls_entry_type_ip:
        if (lbl1->u.pkt > lbl2->u.pkt)
          return 1;
        else if (lbl1->u.pkt > lbl2->u.pkt)
          return -1;
        break;

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
      case gmpls_entry_type_pbb_te:
        ret  = pal_mem_cmp (&lbl1->u.pbb, &lbl2->u.pbb,
                            sizeof (struct pbb_te_label));
        if (ret != 0)
          return ret;

        break;
#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
      case gmpls_entry_type_tdm:
        ret  = pal_mem_cmp (&lbl1->u.tdm, &lbl2->u.tdm,
                            sizeof (struct tdm_label));
        if (ret != 0)
          return ret;

        break;
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

      default:
        break;
    }

  return 0;
}

static s_int32_t
nsm_mpls_cmp_xc_key (void *data1, void* data2)
{
  struct xc_entry *xc1, *xc2;
  s_int32_t ret;

  if (data1 == NULL || data2 == NULL)
    return -1;

  xc1 = (struct xc_entry *) data1;
  xc2 = (struct xc_entry *) data2;

  if (xc1->key.xc_ix > xc2->key.xc_ix)
    return 1;
  else if (xc1->key.xc_ix < xc2->key.xc_ix)
    return -1;

  if (xc1->key.iif_ix > xc2->key.iif_ix)
    return 1;
  else if (xc1->key.iif_ix < xc2->key.iif_ix)
    return -1;

  ret = nsm_mpls_cmp_gen_label (&xc1->key.in_label, &xc2->key.in_label);
  if (ret != 0)
    return ret;

  if (xc1->key.nhlfe_ix > xc2->key.nhlfe_ix)
    return 1;
  else if (xc1->key.nhlfe_ix < xc2->key.nhlfe_ix)
    return -1;

  return 0;
}

static s_int32_t
nsm_mpls_cmp_nhlfe_ipv4_key (void *data1, void* data2)
{
  struct nhlfe_entry *nh1, *nh2;
  struct nhlfe_key *key1, *key2;
  s_int32_t ret;

  if (data1 == NULL || data2 == NULL)
    return -1;

  nh1 = (struct nhlfe_entry *) data1;
  nh2 = (struct nhlfe_entry *) data2;

  key1 = (struct nhlfe_key *) nh1->nkey;
  key2 = (struct nhlfe_key *) nh2->nkey;

  ret = pal_mem_cmp (&key1->u.ipv4.nh_addr, &key2->u.ipv4.nh_addr,
                     sizeof (struct pal_in4_addr));

  if (ret != 0)
    return ret;

  if (key1->u.ipv4.oif_ix > key2->u.ipv4.oif_ix)
    return 1;
  else if (key1->u.ipv4.oif_ix < key2->u.ipv4.oif_ix)
    return -1;

  if (key1->u.ipv4.out_label > key2->u.ipv4.out_label)
    return 1;
  else if (key1->u.ipv4.out_label < key2->u.ipv4.out_label)
    return -1;

  return 0;
}

static s_int32_t
nsm_mpls_cmp_mapped_lsp_pkt_key (void *data1, void *data2)
{
  struct mapped_lsp_entry_pkt *map1, *map2;

  map1 = (struct mapped_lsp_entry_pkt *)data1;
  map2 = (struct mapped_lsp_entry_pkt *)data2;

  if (map1->type > map2->type)
    return 1;
  else if (map1->type < map2->type)
    return -1;

  if (map1->iif_ix > map2->iif_ix)
    return 1;
  else if (map1->iif_ix < map2->iif_ix)
    return -1;

  if (map1->in_label > map2->in_label)
    return 1;
  else   if (map1->in_label < map1->in_label)
    return -11;

  return 0;
}

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE

static s_int32_t
nsm_mpls_cmp_mapped_lsp_pbb_key (void *data1, void* data2)
{
  struct mapped_lsp_entry_pbb *map1, *map2;

  if (data1 == NULL || data2 == NULL)
    return -1;

  map1 = (struct mapped_lsp_entry_pbb *)data1;
  map2 = (struct mapped_lsp_entry_pbb *)data2;

  if (map1->type > map2->type)
    return 1;
  else if (map1->type < map2->type)
    return -1;

  if (map1->iif_ix > map2->iif_ix)
    return 1;
  else if (map1->iif_ix < map2->iif_ix)
    return -1;

  return (pal_mem_cmp (&map1->in_label, &map2->in_label, sizeof (struct pbb_te_label)));
}
#endif /* HAVE_PBB_TE */
#endif /* HAVE_GMPLS */

#ifdef HAVE_IPV6
static s_int32_t
nsm_mpls_cmp_nhlfe_ipv6_key (void *data1, void* data2)
{
  struct nhlfe_entry *nh1, *nh2;
  struct nhlfe_key *key1, *key2;
  s_int32_t ret;

  if (data1 == NULL || data2 == NULL)
    return -1;

  nh1 = (struct nhlfe_entry *) data1;
  nh2 = (struct nhlfe_entry *) data2;

  key1 = (struct nhlfe_key *) nh1->nkey;
  key2 = (struct nhlfe_key *) nh2->nkey;

  ret = pal_mem_cmp (&key1->u.ipv6.nh_addr, &key2->u.ipv6.nh_addr,
                     sizeof (struct pal_in6_addr));

  if (ret != 0)
    return ret;

  if (key1->u.ipv6.oif_ix > key2->u.ipv6.oif_ix)
    return 1;
  else if (key1->u.ipv6.oif_ix < key2->u.ipv6.oif_ix)
    return -1;

  if (key1->u.ipv6.out_label > key2->u.ipv6.out_label)
    return 1;
  else if (key1->u.ipv6.out_label < key2->u.ipv6.out_label)
    return -1;

  return 0;
}

#endif /* HAVE_IPV6 */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
static s_int32_t
nsm_mpls_cmp_pbb_te_ilm (void *data1, void* data2)
{
  struct ilm_entry *ilm1, *ilm2;

  if (data1 == NULL || data2 == NULL)
    return -1;

  ilm1 = (struct ilm_entry *) data1;
  ilm2 = (struct ilm_entry *) data2;

  if (ilm1->key.u.pbb_key.iif_ix > ilm2->key.u.pbb_key.iif_ix)
    return 1;
  else if (ilm1->key.u.pbb_key.iif_ix < ilm2->key.u.pbb_key.iif_ix)
    return -1;

  return (pal_mem_cmp (&ilm1->key.u.pbb_key.in_label,
                       &ilm2->key.u.pbb_key.in_label,
                       sizeof (struct pbb_te_label)));
}

static s_int32_t
nsm_mpls_cmp_nhlfe_pbb_key (void *data1, void* data2)
{
  struct nhlfe_entry *nh1, *nh2;
  struct nhlfe_pbb_key *key1, *key2;

  if (data1 == NULL || data2 == NULL)
    return -1;

  nh1 = (struct nhlfe_entry *) data1;
  nh2 = (struct nhlfe_entry *) data2;

  key1 = (struct nhlfe_pbb_key *) &nh1->nkey;
  key2 = (struct nhlfe_pbb_key *) &nh2->nkey;

  if (key1->oif_ix > key2->oif_ix)
    return 1;
  else if (key1->oif_ix < key2->oif_ix)
    return -1;

  return (pal_mem_cmp (&key1->lbl, &key2->lbl, sizeof (struct pbb_te_label)));
}

#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
static s_int32_t
nsm_mpls_cmp_tdm_ilm (void *data1, void* data2)
{
  struct ilm_entry *ilm1, *ilm2;

  if (data1 == NULL || data2 == NULL)
    return -1;

  ilm1 = (struct ilm_entry *) data1;
  ilm1 = (struct ilm_entry *) data2;

  if (ilm1->key.u.tdm_key.iif_ix > ilm2->key.u.tdm_key.iif_ix)
    return 1;
  else if (ilm1->key.u.tdm_key.iif_ix < ilm2->key.u.tdm_key.iif_ix)
    return -1;

  return (pal_mem_cmp (&ilm1->key.u.tdm_key.in_label,
                       &ilm2->key.u.tdm_key.in_label,
                       sizeof (struct tdm_label)))
}

static s_int32_t
nsm_mpls_cmp_nhlfe_tdm_key (void *data1, void* data2)
{
  struct nhlfe_entry *nh1, *nh2;
  struct nhlfe_tdm_key *key1, *key2;
  s_int32_t ret;

  if (data1 == NULL || data2 == NULL)
    return -1;

  nh1 = (struct nhlfe_entry *) data1;
  nh2 = (struct nhlfe_entry *) data2;

  key1 = (struct nhlfe_tdm_key *) &nh1->nkey;
  key2 = (struct nhlfe_tdm_key *) &nh2->nkey;

  if (data1 == NULL || data2 == NULL)
    return -1;

  if (key1->oif_ix > key2->oif_ix)
    return 1;
  else if (key1->iif_ix < key2->oif_ix)
    return -1;

  return (pal_mem_cmp (&key1->lbl, &key2->lbl, sizeof (struct tdm_label)));
}

#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
void
nsm_mpls_tree_array_add (struct nsm_master *nm, void *ptr, u_int32_t type)
{
  /* Invalid index to the array */
  if (type >= NSM_MPLS_TREE_ARRAY_MAX)
    return;

  /* More than allowed number of trees of a type */
  if (NSM_MPLS->trees [type].cnt >= NSM_MPLS_NUM_TREE)
    return;

  /* Now add the new pointer and increment cnt */
  NSM_MPLS->trees [type].ptr [NSM_MPLS->trees [type].cnt] = ptr;
  NSM_MPLS->trees [type].cnt ++;
  return;
}

void
nsm_mpls_tree_array_get (struct nsm_master *nm, void **ptr, u_int32_t type,
                         u_int32_t index)
{
  /* Invalid index to the array */
  if (type >= NSM_MPLS_TREE_ARRAY_MAX)
    return;

  /* More than allowed number of trees of a type */
  if (index > NSM_MPLS->trees [type].cnt)
    return;

  /* Now add the new pointer and increment cnt */
  *ptr = NSM_MPLS->trees [type].ptr [index];
  return;
}

void
nsm_mpls_tree_array_get_next (struct nsm_master *nm, void *curr, void **ptr,
                              u_int32_t type)
{
  u_int32_t cnt = 0;

  /* Invalid index to the array */
  if (type >= NSM_MPLS_TREE_ARRAY_MAX || NSM_MPLS->trees [type].cnt == 0)
    return;

  /* The last element can be at the index value cnt -1 (of the number of
     trees in the arrray), so we do not need to loop through that index */
  while (cnt < NSM_MPLS->trees [type].cnt - 1)
    {
      if (curr == NSM_MPLS->trees [type].ptr [cnt])
        {
          *ptr = NSM_MPLS->trees [type].ptr [cnt + 1];
          return;
        }
      cnt ++;
    }

  return;
}

/* Init for MPLS. */
int
nsm_mpls_init (struct nsm_master *nm)
{
  struct interface *ifp;
  struct nsm_mpls_if *mif = NULL;
  struct listnode *node;
  int ctr;
#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
  u_int16_t i;
#endif /* HAVE_BFD && HAVE_MPLS_OAM */

#ifdef HAVE_MPLS_VC
  int ret = NSM_FAILURE;
#endif /* HAVE_MPLS_VC */

  /* Initialize top structure. */
  NSM_MPLS = XMALLOC (MTYPE_NSM_MPLS, sizeof (struct nsm_mpls));
  pal_mem_set (NSM_MPLS, 0, sizeof (struct nsm_mpls));
#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))

  NSM_MPLS->min_label_val = LABEL_VALUE_INITIAL;
  NSM_MPLS->max_label_val = LABEL_VALUE_MAX;
  NSM_MPLS->ingress_ttl = -1;
  NSM_MPLS->egress_ttl = -1;
  NSM_MPLS->propagate_ttl = TTL_PROPAGATE_ENABLED;
#endif /* !HAVE_GMPLS || HAVE_PACKET  */

  NSM_MPLS->flags = 0;
#ifdef HAVE_MPLS_INSTALL_BK_LSP
  SET_FLAG (NSM_MPLS->flags, NSM_MPLS_FLAG_INSTALL_BK_LSP);
#endif /* HAVE_MPLS_INSTALL_BK_LSP */

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV
  {
    int i;

    NSM_MPLS->lsp_model     = LSP_MODEL_UNIFORM;

    /* EXP bit 0 to map to Best Effort DSCP by default. */
    NSM_MPLS->dscp_exp_map[0] = DIFFSERV_BEST_EFFORT;

    /* Initialize dscp_exp mapping to invalid. */
    for (i = 1; i < DIFFSERV_MAX_DSCP_EXP_MAPPINGS; i++)
      NSM_MPLS->dscp_exp_map[i] = DIFFSERV_INVALID_DSCP;

    /* Initalize Best Effort DSCP value. */
    NSM_MPLS->supported_dscp[DIFFSERV_BEST_EFFORT] = DIFFSERV_DSCP_SUPPORTED;
  }
#endif /* HAVE_DIFFSERV */

#ifdef HAVE_DSTE
  {
    int i;

    for (i = 0; i < MAX_CLASS_TYPE + 1; i++)
      NSM_MPLS->ct_name[i][0] = '\0';

    /* TE Class default value. */
    for (i = 0; i < MAX_CLASS_TYPE; i++)
      {
        NSM_MPLS->te_class[i].ct_num = CT_NUM_INVALID;
        NSM_MPLS->te_class[i].priority = TE_PRIORITY_INVALID;
      }

  }
#endif /* HAVE_DSTE */
#endif /* !defined (HAVE_GMPLS) || definded (HAVE_PACKET) */
#ifdef HAVE_TE
  admin_group_array_init (NSM_MPLS->admin_group_array);
#endif /* HAVE_TE */

  /* Init linklist for interfaces. */
  NSM_MPLS->iflist = list_new ();

  NSM_MPLS->b_lsp = list_new ();

  /* Tree arrays correctly initialized to all 0's */
  /* Init label space table. */
  NSM_MPLS->ls_table = route_table_init ();

  /* Init FTN table. */
  nsm_ptree_ix_table_init (&NSM_MPLS->ftn_ix_table4,
                           IPV4_MAX_PREFIXLEN, NULL);
  /* Add to FTN Tree Array */
  nsm_mpls_tree_array_add (nm, &NSM_MPLS->ftn_ix_table4,
                           NSM_MPLS_TREE_ARRAY_FTN_IX_TREES) ;
  if (NSM_MPLS->ftn_ix_table4.m_table)
    {
      nsm_mpls_tree_array_add (nm, NSM_MPLS->ftn_ix_table4.m_table,
                               NSM_MPLS_TREE_ARRAY_FTN_TREES);
    }

#ifdef HAVE_IPV6
  nsm_ptree_ix_table_init (&NSM_MPLS->ftn_ix_table6,
                           IPV6_MAX_PREFIXLEN,
                           NSM_MPLS->ftn_ix_table4.ix_mgr);

  /* Add to FTN Tree Array */
  nsm_mpls_tree_array_add (nm, &NSM_MPLS->ftn_ix_table6,
                           NSM_MPLS_TREE_ARRAY_FTN_IX_TREES) ;
  if (NSM_MPLS->ftn_ix_table6.m_table)
    {
      nsm_mpls_tree_array_add (nm, NSM_MPLS->ftn_ix_table6.m_table,
                               NSM_MPLS_TREE_ARRAY_FTN_TREES);
    }

#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  nsm_ptree_ix_table_init (&NSM_MPLS->ftn_pbb_table,
                           8 * sizeof (struct fec_entry_pbb_te),
                           NSM_MPLS->ftn_ix_table4.ix_mgr);

  /* Add to FTN Tree Array */
  nsm_mpls_tree_array_add (nm, &NSM_MPLS->ftn_pbb_table,
                           NSM_MPLS_TREE_ARRAY_FTN_IX_TREES) ;
  if (NSM_MPLS->ftn_pbb_table.m_table)
    {
      nsm_mpls_tree_array_add (nm, NSM_MPLS->ftn_pbb_table.m_table,
                               NSM_MPLS_TREE_ARRAY_FTN_TREES);
    }

#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
  nsm_ptree_ix_table_init (&NSM_MPLS->ftn_tdm_table,
                           8 * sizeof (struct fec_entry_tdm),
                           NSM_MPLS->ftn_ix_table4.ix_mgr);

  /* Add to FTN Tree Array */
  nsm_mpls_tree_array_add (nm, &NSM_MPLS->ftn_tdm_table,
                           NSM_MPLS_TREE_ARRAY_FTN_IX_TREES) ;
  if (NSM_MPLS->ftn_tdm_table.m_table)
    {
      nsm_mpls_tree_array_add (nm, NSM_MPLS->ftn_tdm_table.m_table,
                               NSM_MPLS_TREE_ARRAY_FTN_TREES);
    }

#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

  /* Init ILM table. */
  nsm_avl_ix_table_init (&NSM_MPLS->ilm_ix_table, 0,
                         NULL, nsm_mpls_cmp_pkt_ilm);

  /* Add to ILM Tree Array */
  nsm_mpls_tree_array_add (nm, &NSM_MPLS->ilm_ix_table,
                           NSM_MPLS_TREE_ARRAY_ILM_IX_TREES) ;
  if (NSM_MPLS->ilm_ix_table.m_table)
    {
      nsm_mpls_tree_array_add (nm, NSM_MPLS->ilm_ix_table.m_table,
                               NSM_MPLS_TREE_ARRAY_ILM_TREES);
    }


#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  nsm_avl_ix_table_init (&NSM_MPLS->pbb_ilm_ix, 0,
                         NSM_MPLS->ilm_ix_table.ix_mgr, nsm_mpls_cmp_pbb_te_ilm);

  /* Add to ILM Tree Array */
  nsm_mpls_tree_array_add (nm, &NSM_MPLS->pbb_ilm_ix,
                           NSM_MPLS_TREE_ARRAY_ILM_IX_TREES) ;
  if (NSM_MPLS->pbb_ilm_ix.m_table)
    {
      nsm_mpls_tree_array_add (nm, NSM_MPLS->pbb_ilm_ix.m_table,
                               NSM_MPLS_TREE_ARRAY_ILM_TREES);
    }

#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
  nsm_avl_ix_table_init (&NSM_MPLS->tdm_ilm_ix, 0,
                         NSM_MPLS->ilm_ix_table.ix_mgr, nsm_mpls_cmp_tdm_ilm);

  /* Add to ILM Tree Array */
  nsm_mpls_tree_array_add (nm, &NSM_MPLS->tdm_ilm_ix,
                           NSM_MPLS_TREE_ARRAY_ILM_IX_TREES) ;
  if (NSM_MPLS->tdm_ilm_ix.m_table)
    {
      nsm_mpls_tree_array_add (nm, NSM_MPLS->tdm_ilm_ix.m_table,
                               NSM_MPLS_TREE_ARRAY_ILM_TREES);
    }

#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

  /* Init XC table */
  nsm_avl_ix_table_init (&NSM_MPLS->xc_ix_table, 0, NULL,
                          nsm_mpls_cmp_xc_key);

  /* Init NHLFE table */
  nsm_avl_ix_table_init (&NSM_MPLS->nhlfe_ix_table4, 0,
                         NULL, nsm_mpls_cmp_nhlfe_ipv4_key);

  if (NSM_MPLS->nhlfe_ix_table4.m_table)
    {
      nsm_mpls_tree_array_add (nm, NSM_MPLS->nhlfe_ix_table4.m_table,
                               NSM_MPLS_TREE_ARRAY_NHLFE_TREES);
    }

#ifdef HAVE_IPV6
  nsm_avl_ix_table_init (&NSM_MPLS->nhlfe_ix_table6, 0,
                         NSM_MPLS->nhlfe_ix_table4.ix_mgr,
                         nsm_mpls_cmp_nhlfe_ipv6_key);

  if (NSM_MPLS->nhlfe_ix_table6.m_table)
    {
      nsm_mpls_tree_array_add (nm, NSM_MPLS->nhlfe_ix_table6.m_table,
                               NSM_MPLS_TREE_ARRAY_NHLFE_TREES);
    }

#endif /* HAVE_IPV6 */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  nsm_avl_ix_table_init (&NSM_MPLS->pbb_nhlfe_ix, 0,
                         NSM_MPLS->nhlfe_ix_table4.ix_mgr,
                         nsm_mpls_cmp_nhlfe_pbb_key);

  if (NSM_MPLS->pbb_nhlfe_ix.m_table)
    {
      nsm_mpls_tree_array_add (nm, NSM_MPLS->pbb_nhlfe_ix.m_table,
                               NSM_MPLS_TREE_ARRAY_NHLFE_TREES);
    }

#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
  nsm_avl_ix_table_init (&NSM_MPLS->tdm_nhlfe_ix, 0,
                         NSM_MPLS->nhlfe_ix_table4.ix_mgr,
                         nsm_mpls_cmp_nhlfe_tdm_key);

  if (NSM_MPLS->tdm_nhlfe_ix.m_table)
    {
      nsm_mpls_tree_array_add (nm, NSM_MPLS->tdm_nhlfe_ix.m_table,
                               NSM_MPLS_TREE_ARRAY_NHLFE_TREES);
    }

#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */
#ifdef HAVE_PACKET
  /* Mapped routes table. */
  NSM_MPLS->mapped_routes = ptree_init (IPV6_MAX_BITLEN);
  nsm_mpls_tree_array_add (nm, NSM_MPLS->mapped_routes,
                           NSM_MPLS_TREE_ARRAY_MAPROUTE_TREES);

  /* Mapped lsp table */
  avl_create (&NSM_MPLS->mapped_lsp, 0, nsm_mpls_cmp_mapped_lsp_pkt_key);

#endif /* HAVE_PACKET */
#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  NSM_MPLS->mapped_routes_pbb = ptree_init (sizeof (struct fec_entry_pbb_te));
  nsm_mpls_tree_array_add (nm, NSM_MPLS->mapped_routes_pbb,
                           NSM_MPLS_TREE_ARRAY_MAPROUTE_TREES);

  avl_create (&NSM_MPLS->mapped_lsp_pbb, 0, nsm_mpls_cmp_mapped_lsp_pbb_key);

#endif /* HAVE_PBB_TE */
#ifdef HAVE_TDM
  NSM_MPLS->mapped_routes_tdm = ptree_init (sizeof (struct fec_entry_tdm));
  nsm_mpls_tree_array_add (nm, NSM_MPLS->mapped_routes_tdm,
                           NSM_MPLS_TREE_ARRAY_MAPROUTE_TREES);

  avl_create (&NSM_MPLS->mapped_lsp_tdm, 0, nsm_mpls_cmp_mapped_lsp_tdm_key);

#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

  /* LSP dependency tables. */
  NSM_MPLS->lsp_dep_up = route_table_init ();
  NSM_MPLS->lsp_dep_down = route_table_init ();

#ifdef HAVE_MPLS_VC
  /* Init MPLS VC list and table. */
  NSM_MPLS->vc_table = route_table_init ();
  NSM_MPLS->vc_group_list = list_new ();

  /* Init the MPLS VC hash table */
  ret = nsm_mpls_vc_hash_init (nm);
  if (ret < 0)
    zlog_err (NSM_ZG, "%s: %d - MPLS VC hash table not created\n",
              __FUNCTION__, __LINE__);

#ifdef HAVE_MS_PW
  /* Init the MS-PW table */
  ret = nsm_ms_pw_hash_init (nm);
  if (ret < 0)
    zlog_err (NSM_ZG, "%s: %d - MS PW hash table not created\n",
              __FUNCTION__, __LINE__);
#endif /* HAVE_MS_PW */
#endif /* HAVE_MPLS_VC */

#ifdef HAVE_VPLS
  NSM_MPLS->vpls_table = ptree_init (VPLS_ID_LEN);
#endif /* HAVE_VPLS */

#ifdef HAVE_MPLS_OAM_ITUT
 NSM_MPLS->mpls_oam_itut_list = list_new ();
#endif /* HAVE_MPLS_OAM_ITUT */


#ifdef HAVE_NSM_MPLS_OAM
  /* MPLS OAM List */
  NSM_MPLS->mpls_vpn_list = list_new ();
  NSM_MPLS->mpls_oam_list = list_new ();
  nsm_mpls_oam_udp_create_socket(nm);
  oam_raw_create_socket(nm);
  THREAD_READ_ON (NSM_ZG, nm->nmpls->mpls_oam_read, nsm_mpls_oam_udp_read,
                  nm, nm->nmpls->oam_sock);
#endif /* HAVE_NSM_MPLS_OAM */

  /* Platform-wide labelspace init. */
  nsm_mpls_platform_ls_init (nm);

  /* Go through existing NSM iflist and create shadow MPLS if objects. */
  LIST_LOOP (nm->vr->ifm.if_list, ifp, node)
    {
      mif = nsm_mpls_if_add (ifp);
      if ((if_is_up (ifp)) && (mif))
        {
          nsm_mpls_ifp_set (mif, ifp);
          nsm_mpls_if_up (mif);
        }
    }

#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
  NSM_MPLS->bfd_fec_conf_list = list_new ();

  /* Setting Default values for the BFD attributes of an LSp-Type. */
  for (i = 0; i < BFD_MPLS_LSP_TYPE_UNKNOWN; i++)
    {
      NSM_MPLS->bfd_lsp_conf [i].lsp_ping_intvl =
        BFD_MPLS_LSP_PING_INTERVAL_DEF;

      NSM_MPLS->bfd_lsp_conf [i].min_tx = BFD_MPLS_MIN_TX_INTERVAL_DEF;
      NSM_MPLS->bfd_lsp_conf [i].min_rx = BFD_MPLS_MIN_RX_INTERVAL_DEF;
      NSM_MPLS->bfd_lsp_conf [i].mult = BFD_MPLS_DETECT_MULT_DEF;
    }

  NSM_MPLS->bfd_flag = 0;
#endif /* HAVE_BFD && HAVE_MPLS_OAM */

#ifdef HAVE_SNMP
  if (IS_APN_VR_PRIVILEGED (nm->vr))
    {
      /* Init link list for ILM Temp structure */
      ilm_entry_temp_list = list_new ();

      /* Init link list for NHLFE Temp structure */
      nhlfe_entry_temp_list = list_new ();

      /* Init link list for FTN Temp structure */
      ftn_entry_temp_list = list_new ();
    }

  NSM_MPLS->vc_pw_notification_rate = NSM_MPLS_DEF_MAX_TRAP;
#endif /* HAVE_SNMP */

#ifdef HAVE_VCCV
  /* Initialize the VCCV Statistics  to zero */
  pal_mem_set (&NSM_MPLS->vccv_stats, 0, sizeof (struct vccv_statistics));
#endif /* HAVE_VCCV */

#if (defined HAVE_MPLS_VC) || (defined HAVE_VPLS)
#ifdef HAVE_SNMP
  NSM_MPLS->vc_indx_mgr = bitmap_new (1280, 1, NSM_MPLS_L2_VC_MAX);
#endif /* HAVE_SNMP*/
#endif /* (defined HAVE_MPLS_VC) || (defined HAVE_VPLS) */

  for (ctr = GMPLS_LABEL_PACKET; ctr < GMPLS_LABEL_TYPE_MAX; ctr++)
    {
      nm->label_pool_table[ctr] = NULL;
    }

#ifdef HAVE_GMPLS
  /* Calling it first time to put the index to 1 from 0 */
  NSM_GMPLS_GIFINDEX_GET ();
  NSM_MPLS->loop_gindex = NSM_GMPLS_GIFINDEX_GET ();
#endif /* HAVE_GMPLS */

  return 0;
}

/* Deinit for MPLS. */
void
nsm_mpls_deinit (struct nsm_master *nm)
{
#ifdef HAVE_VRF
  struct listnode *ln;
  struct nsm_mpls_if *mif;
#endif /* HAVE_VRF */
  if ((! NSM_MPLS) || (! NSM_MPLS->iflist))
    return;

  NSM_MPLS->shutdown = NSM_TRUE;

#ifdef HAVE_MPLS_INSTALL_BK_LSP
  UNSET_FLAG (NSM_MPLS->flags, NSM_MPLS_FLAG_INSTALL_BK_LSP);
#endif /* HAVE_MPLS_INSTALL_BK_LSP */

#ifdef HAVE_VPLS
  if (NSM_MPLS->vpls_table)
    {
      nsm_vpls_cleanup_all (nm);
      ptree_finish (NSM_MPLS->vpls_table);
      NSM_MPLS->vpls_table = NULL;
    }
#endif /* HAVE_VPLS */

#ifdef HAVE_MPLS_VC
  /* Delete all VCs. */
  nsm_mpls_l2_circuit_delete_all (nm, NSM_FALSE);

  /* Delete vc fec tables. */
  nsm_mpls_vc_cleanup_all (nm, NSM_TRUE);

  /* Remove vc table. */
  route_table_finish (NSM_MPLS->vc_table);
  NSM_MPLS->vc_table = NULL;

  /* Delete group list. There should be no remaining groups. */
  NSM_ASSERT (NSM_MPLS->vc_group_list->count == 0);
  if (NSM_MPLS->vc_group_list->count != 0)
    zlog_err (NSM_ZG, "Internal Error: Virtual Circuit Group count mismatch");
  list_delete (NSM_MPLS->vc_group_list);
#endif /* HAVE_MPLS_VC */

#if (defined HAVE_MPLS_VC) || (defined HAVE_VPLS)
#ifdef HAVE_SNMP
  bitmap_free (NSM_MPLS->vc_indx_mgr);
#endif /*HAVE_SNMP */
#endif /*(defined HAVE_MPLS_VC) || (defined HAVE_VPLS) */

#ifdef HAVE_VRF
  /* Unbind interfaces from vrf */
  LIST_LOOP (NSM_MPLS->iflist, mif, ln)
    {
      if (if_is_loopback (mif->ifp))
        continue;

      nsm_mpls_if_vrf_bind (mif, VRF_ID_UNSPEC);
    }

  /* Delete vrf fec tables. */
  nsm_mpls_vrf_cleanup_all (nm, NSM_TRUE);
#endif /* HAVE_VRF */

  /* Delete all mapped-routes. */
  nsm_gmpls_mapped_route_delete_all (nm);

  /* Delete all mapped-routes. */
  nsm_gmpls_mapped_lsp_delete_all (nm);

  /* Delete global ftn table. */
  nsm_gmpls_ftn_cleanup_all (nm, GLOBAL_FTN_ID, NSM_TRUE);

  /* Delete ilm table. */
  nsm_gmpls_ilm_cleanup_all (nm, NSM_TRUE);

  /* Delete all XC tables */
  nsm_mpls_xc_cleanup_all (nm, NSM_TRUE);

  /* Delete NHLFE tables */
  nsm_mpls_nhlfe_cleanup_all (nm, NSM_TRUE);

  pal_mem_set (&NSM_MPLS->trees, '\0', sizeof (struct nsm_mpls_tree_array) *
                                       NSM_MPLS_TREE_ARRAY_MAX);
  /* Delete lsp_dep up table. */
  NSM_ASSERT (! route_table_has_info (NSM_MPLS->lsp_dep_up));
  if (route_table_has_info (NSM_MPLS->lsp_dep_up))
    zlog_err (NSM_ZG, "Internal Error: LSP Dependency UP table is not empty");
  route_table_finish (NSM_MPLS->lsp_dep_up);
  NSM_MPLS->lsp_dep_up = NULL;

  /* Delete lsp_dep down table. */
  NSM_ASSERT (! route_table_has_info (NSM_MPLS->lsp_dep_down));
  if (route_table_has_info (NSM_MPLS->lsp_dep_down))
    zlog_err (NSM_ZG, "Internal Error: LSP Dependency DOWN table is not empty");
  route_table_finish (NSM_MPLS->lsp_dep_down);
  NSM_MPLS->lsp_dep_down = NULL;

  /* Disable all interfaces for MPLS. */
  nsm_mpls_disable_all_interfaces (nm);

  /* Delete all interface objects, without sending update to clients. */
  nsm_mpls_if_delete_all (nm, NSM_FALSE);

  list_delete (NSM_MPLS->iflist);

  /* Delete bidirectional lsp list */
  list_delete (NSM_MPLS->b_lsp);

  /* Platform-wide labelspace deinit. */
  nsm_mpls_platform_ls_deinit (nm);

  /* Label space table removal. */
  NSM_ASSERT (! route_table_has_info (NSM_MPLS->ls_table));
  if (route_table_has_info (NSM_MPLS->ls_table))
    zlog_err (NSM_ZG, "Internal Error: Label space object count mismatch");
  route_table_finish (NSM_MPLS->ls_table);
  NSM_MPLS->ls_table = NULL;

#if defined (HAVE_BFD) && defined (HAVE_MPLS_OAM)
  list_delete (NSM_MPLS->bfd_fec_conf_list);
#endif /* HAVE_BFD && HAVE_MPLS_OAM */

#ifdef HAVE_SNMP
   if (IS_APN_VR_PRIVILEGED (nm->vr))
     {
       /* Init link list for ILM Temp structure */
       list_delete (ilm_entry_temp_list);

       /* Init link list for NHLFE Temp structure */
       list_delete (nhlfe_entry_temp_list);

       /* Init link list for FTN Temp structure */
       list_delete (ftn_entry_temp_list);
     }
#endif /* HAVE_SNMP */

  /* Delete top structure. */
  XFREE (MTYPE_NSM_MPLS, NSM_MPLS);
  NSM_MPLS = NULL;
}

/* Deinit top-level MPLS data. */
void
nsm_mpls_main_deinit ()
{
  /* Close connection to the MPLS Forwarder. */
  apn_mpls_close_all_handles (APN_PROTO_NSM);
}

/* Lookup table based on vrf_id. */
struct ptree_ix_table *
nsm_gmpls_ftn_table_lookup (struct nsm_master *nm, u_int32_t vrf_id,
                            struct fec_gen_entry *ftn)
{
  struct ptree_ix_table *tbl = NULL;
  if (! NSM_MPLS)
    return NULL;

#ifdef HAVE_PACKET
  if (ftn->type == gmpls_entry_type_ip)
    {
      if (vrf_id == GLOBAL_FTN_ID)
        {
          if (ftn->u.prefix.family == AF_INET)
            tbl = &(NSM_MPLS->ftn_ix_table4);
#ifdef HAVE_IPV6
          else if (ftn->u.prefix.family == AF_INET6)
            tbl = &(NSM_MPLS->ftn_ix_table6);
#endif /* HAVE_IPV6 */
        }

#ifdef HAVE_VRF
      else
        {
          struct vrf_table *vrf;

          /* Lookup vrf table list. */
          vrf = NSM_MPLS->vrf_hash[VRF_HASH_KEY (vrf_id)];
          for ( ; vrf; vrf = vrf->next)
            if (vrf->vrf_id == vrf_id)
              {
                if (ftn->u.prefix.family == AF_INET)
                  tbl = &(vrf->vrf_ix_table4);
#ifdef HAVE_IPV6
                 else if (ftn->u.prefix.family == AF_INET6)
                   tbl = &(vrf->vrf_ix_table6);
#endif /* HAVE_IPV6 */
              }
        }
#endif /* HAVE_VRF */
    }
#endif /* HAVE_PACKET */

#ifdef HAVE_GMPLS
#ifdef HAVE_PBB_TE
  if (ftn->type == gmpls_entry_type_pbb_te)
    {
      tbl = &(NSM_MPLS->ftn_pbb_table);
    }
#endif /* HAVE_PBB_TE */

#ifdef HAVE_TDM
  if (ftn->type == gmpls_entry_type_tdm)
    {
      tbl = &(NSM_MPLS->ftn_tdm_table);
    }
#endif /* HAVE_TDM */
#endif /* HAVE_GMPLS */

  if ((tbl) && (tbl->m_table) && (tbl->ix_mgr))
    return tbl;

  return NULL;
}

#if (!defined (HAVE_GMPLS) || defined (HAVE_PACKET))
#ifdef HAVE_DIFFSERV

/* Add a DSCP to EXP bit mapping. */
int
nsm_mpls_dscp_exp_map_add (struct nsm_master *nm, char *name, int exp_val)
{
  u_char dscp, i;

  if (! NSM_MPLS)
    return NSM_FAILURE;

  /* Translate class alias to dscp value. */
  CLASS_TO_DSCP (name, dscp);

  /* If class is invalid, return error. */
  if (dscp == DIFFSERV_INVALID_DSCP)
    return NSM_ERR_DSCP_INVALID;

  /* If class is not supported, return error. */
  if (NSM_MPLS->supported_dscp[dscp] == DIFFSERV_DSCP_NOT_SUPPORTED)
    return NSM_ERR_DSCP_NOT_SUPPORTED;

  /* If there is no change, return. */
  if (NSM_MPLS->dscp_exp_map[exp_val] == dscp)
    return NSM_SUCCESS;

  /* Set the dscp_exp mapping. */
  NSM_MPLS->dscp_exp_map[exp_val] = dscp;

  /* Update ftn and ilm table. */
  nsm_mpls_update_diffserv_info(nm);

  /* Send a update msg to signaling protocol. */
  nsm_mpls_dscp_exp_map_update (nm, exp_val, dscp);

  /* Set the flag if not already set. */
  for (i = 1; i < DIFFSERV_MAX_DSCP_EXP_MAPPINGS; i++)
    {
      if (NSM_MPLS->dscp_exp_map[i] != DIFFSERV_INVALID_DSCP)
        {
          SET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_DSCP_EXP_MAP);
          return NSM_SUCCESS;
        }
    }

  /* Best Effort case. */
  if (exp_val != 0)
    SET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_DSCP_EXP_MAP);
  else
    UNSET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_DSCP_EXP_MAP);

  return NSM_SUCCESS;
}

/* Delete a DSCP to EXP bit mapping. */
int
nsm_mpls_dscp_exp_map_del (struct nsm_master *nm, char *name, int exp_val)
{
  u_char dscp;
  int i;

  if (! NSM_MPLS)
    return NSM_FAILURE;

  /* Translate class alias to dscp value. */
  CLASS_TO_DSCP (name, dscp);

  /* If class is invalid, return error. */
  if (dscp == DIFFSERV_INVALID_DSCP)
    return NSM_ERR_DSCP_INVALID;

  /* If the pair isn't correct, return error. */
  if (NSM_MPLS->dscp_exp_map[exp_val] != dscp)
    return NSM_ERR_DSCP_EXP_MISMATCH;

  /* If there is no change, return. */
  if (NSM_MPLS->dscp_exp_map[exp_val] == DIFFSERV_INVALID_DSCP)
    return NSM_SUCCESS;

  /* Unset the dscp_exp mapping. */
  NSM_MPLS->dscp_exp_map[exp_val] = DIFFSERV_INVALID_DSCP;

  /* Update ftn and ilm table. */
  nsm_mpls_update_diffserv_info(nm);

  /* Send a update msg to signaling protocol. */
  nsm_mpls_dscp_exp_map_update (nm, exp_val, DIFFSERV_INVALID_DSCP);

  /* If EXP bit 0 is not mapped to Best Effort DSCP, return. */
  if (NSM_MPLS->dscp_exp_map[0] != DIFFSERV_BEST_EFFORT)
    {
      SET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_DSCP_EXP_MAP);
      return NSM_SUCCESS;
    }

  /* If at least one mapping is valid, return. */
  for (i = 1; i < DIFFSERV_MAX_DSCP_EXP_MAPPINGS; i++)
    {
      if (NSM_MPLS->dscp_exp_map[i] != DIFFSERV_INVALID_DSCP)
        return NSM_SUCCESS;
    }

  /* Unset flag. */
  UNSET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_DSCP_EXP_MAP);

  return NSM_SUCCESS;
}

/* Add support for a Diffserv class. */
int
nsm_mpls_dscp_support_add (struct nsm_master *nm, char *name)
{
  u_char dscp_val, i;

  if (! NSM_MPLS)
    return NSM_FAILURE;

  /* Translate class alias to dscp value. */
  CLASS_TO_DSCP (name, dscp_val);
  if (dscp_val == DIFFSERV_INVALID_DSCP)
    return NSM_ERR_DSCP_INVALID;

  /* Return if there is no change. */
  if (NSM_MPLS->supported_dscp[dscp_val] == DIFFSERV_DSCP_SUPPORTED)
    return NSM_SUCCESS;

  /* Set the support_dscp flag. */
  NSM_MPLS->supported_dscp[dscp_val] = DIFFSERV_DSCP_SUPPORTED;

  /* Send a update msg to signaling protocol. */
  nsm_mpls_supported_dscp_update (nm, dscp_val, DIFFSERV_DSCP_SUPPORTED);

  /* Set the config flag. */
  if (dscp_val != DIFFSERV_BEST_EFFORT)
    {
      SET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_SUPPORTED_DSCP);
      return NSM_SUCCESS;
    }
  else
    for (i = 1; i < DIFFSERV_MAX_SUPPORTED_DSCP; i++)
      if (NSM_MPLS->supported_dscp[i] == DIFFSERV_DSCP_SUPPORTED)
        {
          SET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_SUPPORTED_DSCP);
          return NSM_SUCCESS;
        }

  UNSET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_SUPPORTED_DSCP);

  return NSM_SUCCESS;
}

/* Delete support for a Diffserv class. */
int
nsm_mpls_dscp_support_del (struct nsm_master *nm, char *name)
{
  u_char dscp_val;
  int i;

  if (! NSM_MPLS)
    return NSM_FAILURE;

  /* Translate class alias to dscp value. */
  CLASS_TO_DSCP (name, dscp_val);
  if (dscp_val == DIFFSERV_INVALID_DSCP)
    return NSM_ERR_DSCP_INVALID;

  /* Return if there is no change. */
  if (NSM_MPLS->supported_dscp[dscp_val] == DIFFSERV_DSCP_NOT_SUPPORTED)
    return NSM_SUCCESS;

  /* Make sure the dscp val is not in dscp_exp_map */
  for(i = 0; i < DIFFSERV_MAX_DSCP_EXP_MAPPINGS; i++)
    if (NSM_MPLS->dscp_exp_map[i] == dscp_val)
      nsm_mpls_dscp_exp_map_del (nm, name, i);

  /* Unset the support_dscp. */
  NSM_MPLS->supported_dscp[dscp_val] = DIFFSERV_DSCP_NOT_SUPPORTED;

  /* Send a update msg to signaling protocol. */
  nsm_mpls_supported_dscp_update (nm, dscp_val, DIFFSERV_DSCP_NOT_SUPPORTED);

  /* Check best-effort support. */
  if (NSM_MPLS->supported_dscp[DIFFSERV_BEST_EFFORT] !=
      DIFFSERV_DSCP_SUPPORTED)
    {
      SET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_SUPPORTED_DSCP);
      return NSM_SUCCESS;
    }

  for (i = 1; i < DIFFSERV_MAX_SUPPORTED_DSCP; i++)
    if (NSM_MPLS->supported_dscp[i] == DIFFSERV_DSCP_SUPPORTED)
      return NSM_SUCCESS;

  /* Unset top flag. */
  UNSET_FLAG (NSM_MPLS->config, NSM_MPLS_CONFIG_SUPPORTED_DSCP);

  return NSM_SUCCESS;
}
#endif /* HAVE_DIFFSERV */
#endif /* !HAVE_GMPLS || HAVE_PACKET */

int
nsm_gmpls_static_label_range_check (u_int32_t label, struct interface *ifp,
                                   u_int32_t *min_label, u_int32_t *max_label)
{
  u_int32_t max_value;

  if (! NSM_MPLS_VALID_INTERFACE (ifp))
    {
      *min_label = LABEL_VALUE_INITIAL;
      max_value = LABEL_VALUE_MAX;
    }
  else
    {
      *min_label = ifp->ls_data.min_label_value;
      max_value = ifp->ls_data.max_label_value;
    }

  *max_label = (MPLS_STATIC_LABEL_RATIO * (max_value - *min_label)) / 100
               + *min_label;

  if (label >= *min_label && label <= *max_label)
    return 0;
  else
    return -1;
}

