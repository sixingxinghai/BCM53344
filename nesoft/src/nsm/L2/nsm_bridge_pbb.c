/* Copyright (C) 2007  All Rights Reserved */
#include "pal.h"
#include "lib.h"
#include "hal_incl.h"
#include "memory.h"
#include "thread.h"
#include "table.h"
#include "avl_tree.h"
#include "hash.h"
#include "bitmap.h"
#include "nsm_message.h"
#include "nsmd.h"
#include "nsm_server.h"
#include "snprintf.h"

#ifdef HAVE_L2
#include "nsm_bridge.h"
#include "ptree.h"
#include "avl_tree.h"
#ifdef HAVE_VLAN
#include "nsm_vlan.h"
#include "nsm_vlan_cli.h"
#endif /* HAVE_VLAN */
#endif /* HAVE_L2 */
#include "nsm_interface.h"
#include "nsm_api.h"
#include "nsm_debug.h"
#include "nsm_bridge_cli.h"
#ifdef HAVE_SNMP
#include "nsm_snmp_common.h"
#endif /* HAVE_SNMP */

#ifdef HAVE_PVLAN
#include "nsm_pvlan.h"
#endif /* HAVE_PVLAN */

#if defined (HAVE_I_BEB)||defined (HAVE_B_BEB)

#include "nsm_pbb_cli.h"
#include "nsm_bridge_pbb.h"
#ifdef HAVE_PBB_TE
#include "nsm/L2/nsm_pbb_te.h"
#endif /* HAVE_PBB_TE */

#if defined HAVE_I_BEB
int
nsm_pbb_icomp_vip_cmp (void * data1, void * data2)
{
  struct vip_tbl_key *key1 = (struct vip_tbl_key *) data1;
  struct vip_tbl_key *key2 = (struct vip_tbl_key *) data2;

  if ((!data1) || (!data2))
    return -1;


  if (pal_mem_cmp (key1, key2, sizeof (struct vip_tbl_key)) > 0)
    return 1;

  if (pal_mem_cmp (key2, key1, sizeof (struct vip_tbl_key)) > 0)
    return -1;

  return 0;
}


int
nsm_pbb_icomp_sid2vip_cmp (void * data1, void * data2)
{
  struct sid2vip_key *key1 = (struct sid2vip_key *) data1;
  struct sid2vip_key *key2 = (struct sid2vip_key *) data2;

  if ((!data1) || (!data2))
    return -1;


  if (pal_mem_cmp (key1, key2, sizeof (struct sid2vip_key)) > 0)
    return 1;

  if (pal_mem_cmp (key2, key1, sizeof (struct sid2vip_key)) > 0)
    return -1;

  return 0;
}

int
nsm_pbb_icomp_vip2pip_cmp (void * data1, void * data2)
{
  struct vip_tbl_key *key1 = (struct vip_tbl_key *) data1;
  struct vip_tbl_key *key2 = (struct vip_tbl_key *) data2;

  if ((!data1) || (!data2))
    return -1;


  if (pal_mem_cmp (key1, key2, sizeof (struct vip_tbl_key)) > 0)
    return 1;

  if (pal_mem_cmp (key2, key1, sizeof (struct vip_tbl_key)) > 0)
    return -1;

  return 0;
}

int
nsm_pbb_icomp_cnp_cmp (void * data1, void * data2)
{
  struct cnp_srv_tbl_key *key1 = (struct cnp_srv_tbl_key *) data1;
  struct cnp_srv_tbl_key *key2 = (struct cnp_srv_tbl_key *) data2;

  if ((!data1) || (!data2))
    return -1;


  if (pal_mem_cmp (key1, key2, sizeof (struct cnp_srv_tbl_key)) > 0)
    return 1;

  if (pal_mem_cmp (key2, key1, sizeof (struct cnp_srv_tbl_key)) > 0)
    return -1;

  return 0;
}

#endif /* HAVE_I_BEB */

#ifdef HAVE_B_BEB
int
nsm_pbb_bcomp_cbp_cmp (void * data1, void * data2)
{
  struct cbp_srvinst_key *key1 = (struct cbp_srvinst_key *) data1;
  struct cbp_srvinst_key *key2 = (struct cbp_srvinst_key *) data2;

  if ((!data1) || (!data2))
    return -1;


  if (pal_mem_cmp (key1, key2, sizeof (struct cbp_srvinst_key)) > 0)
    return 1;

  if (pal_mem_cmp (key2, key1, sizeof (struct cbp_srvinst_key)) > 0)
    return -1;

  return 0;
}
#endif /* HAVE_B_BEB */

#ifdef HAVE_I_BEB
/* Given the key values of cnp_srv_tbl this API
   return the unique cnp_srv_tbl from cnp_srv_list*/
struct cnp_srv_tbl *
nsm_pbb_search_cnp_node(struct avl_tree *tree, u_int32_t icomp_id,
                        u_int32_t isid, u_int32_t port_num)
{

  struct cnp_srv_tbl_key key;
  struct avl_node *rn = NULL;

  key.sid = isid;
  key.icomp_id = icomp_id;
  key.icomp_port_num = port_num;

  rn =  avl_search(tree, (void *) &key);
  if (rn)
    return (struct cnp_srv_tbl *)rn->info;
  else 
    return NULL;
}
#endif

#ifdef HAVE_B_BEB
/* Given the key values of cbp_srvinst_tbl this API
   return the unique cbp_srvinst_tbl from cbp_srv_list*/

struct cbp_srvinst_tbl *
nsm_pbb_search_cbp_node(struct avl_tree *tree, u_int32_t port_num,
                        u_int32_t isid)
{
  struct cbp_srvinst_key key;
  struct avl_node *rn = NULL;

  key.b_sid = isid;
  key.bcomp_id = 1; 
  /*bcomp_id value is always 1*/
  key.bcomp_port_num = port_num;

  rn =  avl_search(tree, (void *) &key);
  if (rn)
    return (struct cbp_srvinst_tbl *)rn->info;
  else
    return NULL;
}
#endif /*HAVE_B_BEB*/

#ifdef HAVE_I_BEB
/* Given the key values of pip_tbl this API
   return the unique pip_tbl from pip_tbl_list*/
struct pip_tbl *
nsm_pbb_search_pip_node(struct list *list, u_int32_t icomp_id,
                        u_int32_t port_num)
{
  struct pip_tbl *pip_node;
  struct listnode *node;
  void *data;

  for (node = list->head; node; NEXTNODE (node))
  {
    data = GETDATA (node);
    pip_node = (struct pip_tbl*)data;
    if ((pip_node->key.icomp_id == icomp_id) && 
       (pip_node->key.pip_port_num == port_num))
      return pip_node;
  }
  return NULL;
}

/* Given the key values of vip_tbl this API
   return the unique vip_tbl from vip_tbl_list*/
struct vip_tbl *
nsm_pbb_search_vip_node(struct avl_tree *tree, u_int32_t icomp_id,
                        u_int32_t port_num )
{
  struct vip_tbl_key key;
  struct avl_node *rn = NULL;

  key.icomp_id = icomp_id;
  key.vip_port_num = port_num;

  rn =  avl_search(tree, (void *) &key);
  if(rn)
    return (struct vip_tbl *)rn->info;
  else
    return NULL;
}

/* From this API given an ISID we can find a unique VIP 
   associated */
struct sid2vip_xref *
nsm_pbb_search_sid2vip_node(struct avl_tree *tree, u_int32_t icomp_id, 
                            u_int32_t isid)
{
  struct sid2vip_key key;
  struct avl_node *rn = NULL;

  
  key.icomp_id = icomp_id;
  key.vip_sid = isid;

  rn =  avl_search(tree, (void *) &key);
  if(rn)
    return (struct sid2vip_xref *)rn->info;
  else
    return NULL;
}

/* For a given vip node we can get a unique PIP associated */
struct vip2pip_map*
nsm_pbb_search_vip2pip_node(struct avl_tree *tree, 
                            u_int32_t icomp_id,
                            u_int32_t port_num)
{
  struct vip_tbl_key key;
  struct avl_node *rn = NULL;


  key.icomp_id = icomp_id;
  key.vip_port_num = port_num;

  rn =  avl_search(tree, (void *) &key);
  if(rn)
    return (struct vip2pip_map*)rn->info;
  else
    return NULL;
}

/**@searches isid value given isid name.
    @param *instance_name - name of the service instance.
    @param *icomp - icomponent container class.
    @return isid_value.
*/
/* expensive search ..iterates complete vip_tree */
pbb_isid_t
nsm_pbb_search_isid_by_instance_name (char *instance_name, struct nsm_pbb_icomponent *icomp)
{
  struct avl_node *node = NULL;
  struct vip_tbl *vip_node = NULL;

  if (!icomp)
    return NSM_PBB_ISID_NONE;

  for (node = avl_top(icomp->vip_table_list); node; node = avl_next(node))
    {
      vip_node = (struct vip_tbl *)node->info;
      if (pal_strncmp(vip_node->srv_inst_name, instance_name, NSM_PBB_ISID_NAME_SIZ))
        {
          continue;
        }
      else
        return vip_node->vip_sid;
    }
  return NSM_PBB_ISID_NONE;
}

/* This API searches for a VIP node given an isid value */
/* requires two avl_searches */
struct vip_tbl *
nsm_pbb_search_vip_by_isid (struct nsm_pbb_icomponent *icomp, 
                            u_int32_t isid, u_int32_t icomp_id)
{
  struct vip_tbl *vip_node = NULL;
  struct sid2vip_xref *s2v_node = NULL;

  s2v_node = nsm_pbb_search_sid2vip_node(icomp->sid2vip_xref_list, 
                                         icomp_id, isid);
  
  if (!s2v_node)
    return NULL;
  
  vip_node = nsm_pbb_search_vip_node (icomp->vip_table_list,
                                      icomp_id, s2v_node->vip_port_num);
  
  return vip_node;
}


/* This API sets the VIP port number in the vip2pip bitmap
   in the PIP node*/
void
nsm_pbb_set_pipvip_map(u_int8_t map[], u_int32_t vip_port_number)
{
  u_int8_t byte = vip_port_number/8 ;
  unsigned char temp = 1;
  temp = (temp << (vip_port_number% 8));
  map[byte] = map[byte] |temp;
}



/* This API unsets the VIP port number in the vip2pip bitmap
   in the PIP node*/
void 
nsm_pbb_unset_pipvip_map(u_int8_t map[], u_int32_t vip_port_number)
{
  u_int8_t byte = (vip_port_number-1)/8 ;
  u_int8_t temp = 1;
  temp = (temp << ((vip_port_number-1) % 8));
  map[byte] = map[byte] &(~temp);
}

/* Gets the next free bit to be associated to the VIP
   from bridge bitmap */
u_int32_t
nsm_pbb_get_vip_port_num(u_int8_t map[])
{

  unsigned int i,k,j= 0;
  for (i = 0; i < NSM_PBB_VIP_PIP_MAP_LEN/8; i++)
    {
      if (map[i] < 255)
        {
          for (j = 1, k = 0;j <= 128 && (map[i] & j);j = j<<1, k++);
            map[i] |= j;
          return ((i*8)+k)+1;
        }
    }
  return (u_int32_t)NSM_PBB_VIP_PIP_MAP_LEN;
}



/* adds a default mac to VIP interface */
void
nsm_pbb_set_vip_default_mac (struct vip_tbl* vip_node ,u_int32_t isid)
{
  u_int8_t dest_mac[ETHER_ADDR_LEN] = { 0x01, 0x1e, 0x83 };

  if (!vip_node)
    return;

  dest_mac[5]= isid  & 0xff;
  dest_mac[4]= (isid >> 8) & 0xff;
  dest_mac[3]= (isid >> 16) & 0xff;
 
  pal_mem_cpy(vip_node->default_dst_bmac,
              dest_mac,ETHER_ADDR_LEN);
}

#endif /* HAVE_I_BEB */



#if defined HAVE_I_BEB && defined HAVE_B_BEB
#ifdef HAVE_PBB_TE 
#if defined HAVE_GMPLS && defined HAVE_GELS

/* Compare function for Ingress Trunk Hash table. */
static bool_t
tesi_name_hash_cmp (struct pbb_te_tesi_name_to_id *tesi_data,
                    char *name)
{
  if (tesi_data
      && name
      && pal_strcmp (tesi_data->tesi_name, name) == 0)
    return PAL_TRUE;

  return PAL_FALSE;
}

/* Function to create key based on name for passed-in trunk. */
static u_int32_t
tesi_name_hash_key_make (char *name)
{
  int i, len;
  u_int32_t key;

  if (name)
    {
      key = 0;
      len = pal_strlen (name);
      for (i = 1; i <= len; i++)
        key += (name[i] * i);

      return key;
    }

  return 0;
}

#endif
#endif

/* this API send logical interface addition notification to PMs */
int
pbb_send_logical_interface_event (char *br_name, struct interface *ifp,
                                  bool_t add)
{
  struct nsm_msg_bridge_if msg;
  int nbytes, i;
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_BRIDGE))
      {
        pal_mem_set (&msg, 0, sizeof (struct nsm_msg_bridge_if));
        pal_strncpy (msg.bridge_name, br_name, NSM_BRIDGE_NAMSIZ);
        msg.num = 1;

        /* Allocate ifindex array. */
        msg.ifindex = XCALLOC (MTYPE_TMP, msg.num * sizeof (u_int32_t));
        msg.ifindex[0] = ifp->ifindex;

        /* Set nse pointer and size. */
        nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
        nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;
        msg.spanning_tree_disable = PAL_FALSE;
  
        /* Encode NSM bridge interface message. */
        nbytes = nsm_encode_bridge_if_msg (&nse->send.pnt, &nse->send.size,
                                           &msg);
        if (nbytes < 0)
          return nbytes;

        /* Send bridge interface message. */
        if (add)
          nsm_server_send_message (nse, 0, 0, NSM_MSG_BRIDGE_ADD_PORT, 0, nbytes);
        else
          nsm_server_send_message (nse, 0, 0, NSM_MSG_BRIDGE_DELETE_PORT, 0, nbytes);

        /* Free ifindex array. */
        XFREE (MTYPE_TMP, msg.ifindex);
      }
  return 0;
}

/**@Adds the logical pbb interfaces.
    @param *bridge - nsm bridge structure.
    @return Success or failure. */


/* API to add logical interfaces (CBP and PIP) */
struct interface *
nsm_bridge_add_pbb_interface (struct nsm_bridge* bridge, 
                              enum nsm_vlan_port_mode mode, char *ifname_l)
{
  struct interface *ifp_l = NULL;
  struct nsm_if *zif_l = NULL;
  char *bridge_id_str;
  struct apn_vr *vr = NULL;

#if defined HAVE_PBB_TE
#if defined (HAVE_I_BEB) && defined (HAVE_B_BEB)
#if defined HAVE_GMPLS && defined HAVE_GELS
      struct nsm_vlan_port *vlan_port = NULL;
      struct nsm_bridge_port *br_port = NULL;
#endif
#endif
#endif

  /* ensuring that the interface name is one of pbb_logical */
  /* if interface name doesnt start with cbp or pip return NULL */
  if ((pal_strncmp (ifname_l, "cbp.", 4)&& pal_strncmp (ifname_l, "pip.", 4)))
    return NULL;

  /* to reach the bridge id from inteface name */
  bridge_id_str = ifname_l + 4;

  ifp_l = ifg_lookup_by_name (&nzg->ifg, ifname_l);
  if (!ifp_l)
    {
      ifp_l = ifg_get_by_name (&nzg->ifg, ifname_l);
      if (!ifp_l) 
        return NULL;

      ifp_l->type = NSM_IF_TYPE_L2;
      SET_FLAG (ifp_l->flags, IFF_UP);
      SET_FLAG (ifp_l->flags, IFF_MULTICAST);
      ifp_l->hw_type = IF_TYPE_PBB_LOGICAL;
      ifp_l->hw_addr_len = ETHER_ADDR_LEN;
      pal_mem_set (ifp_l->hw_addr, 0, ETHER_ADDR_LEN);

      if (mode == NSM_VLAN_PORT_MODE_CBP)
        ifp_l->ifindex = NSM_LOGICAL_INTERFACE_INDEX - 
                         pal_atoi (bridge_id_str);
      else
        ifp_l->ifindex = NSM_LOGICAL_INTERFACE_INDEX + 
                         bridge->bridge_id;
   
      vr = bridge->master->nm->vr;

      ifp_l->vr = vr;
  
      listnode_add (vr->ifm.if_list, ifp_l);

      ifp_l->vrf = apn_vrf_lookup_default (vr);

      nsm_if_add (ifp_l);
      zif_l = (struct nsm_if *)ifp_l->info;
      zif_l->type = NSM_IF_TYPE_L2;
      /* the message to onmd is posted by this  API */
      nsm_bridge_port_add (bridge->master, bridge->name,
                           ifp_l, PAL_FALSE, PAL_TRUE);

#if defined HAVE_PBB_TE
#if defined (HAVE_I_BEB) && defined (HAVE_B_BEB)
#if defined HAVE_GMPLS && defined HAVE_GELS

      br_port = zif_l->switchport;
      if(br_port)
        vlan_port = &br_port->vlan_port;

      if((vlan_port) &&(mode == NSM_VLAN_PORT_MODE_CBP))
      {
        vlan_port->cbp_vlan_alloc_map1 = XCALLOC (MTYPE_NSM_BRIDGE_PORT, sizeof (struct nsm_vlan_bmp));
        pal_mem_set (vlan_port->cbp_vlan_alloc_map1, 0 ,sizeof (struct nsm_vlan_bmp));

        vlan_port->cbp_vlan_alloc_map2 = XCALLOC (MTYPE_NSM_BRIDGE_PORT, sizeof (struct nsm_vlan_bmp));
        pal_mem_set (vlan_port->cbp_vlan_alloc_map2, 0 ,sizeof (struct nsm_vlan_bmp));
      }

#endif
#endif
#endif
    }
  return ifp_l;
}


/**@Deleting the logical pbb interfaces.
    @param *bridge - nsm bridge structure.
    @param *ifname_l - interface name.
    @return Success or failure.*/

/* Deleting the logical pbb interfaces 
   This will be triggered from i/b bridge deletion */
int
nsm_bridge_delete_pbb_interface (struct nsm_bridge* bridge,
                                 char *ifname_l)
{
  struct interface *ifp_l = NULL;
  struct apn_vr *vr = NULL;

  /* check for interface type */
  ifp_l = ifg_lookup_by_name (&nzg->ifg, ifname_l);

  if (!ifp_l)
    return RESULT_OK;
  
  vr = bridge->master->nm->vr;
  
  nsm_bridge_port_delete (bridge->master, bridge->name,
                          ifp_l, PAL_FALSE, PAL_TRUE);

  nsm_if_delete (ifp_l, ifp_l->vrf->proto);
  
  /* The logical interface node is deleted from vr->ifm.if_list */
  if (vr)
    listnode_delete (vr->ifm.if_list, ifp_l);

  return RESULT_ERROR;
}

/**@Adds the logical pbb interfaces.
    @param *bridge - nsm bridge structure.
    @return Success or failure.*/

/* Logical port addition for IB_BEB scenario
   For every I component addition a PIP interface will be added
   After backbone bridge addition CBP will be added for every I comp */

int 
nsm_pbb_add_logical_interfaces (struct nsm_bridge *bridge)
{
  struct nsm_bridge_master *master;
  struct nsm_bridge *curr = NULL;
  char ifname_l[INTERFACE_NAMSIZ + 1];
  struct interface *ifp_l_pip = NULL;
  struct interface *ifp_l_cbp = NULL;

  master = bridge->master;

  if (bridge->backbone_edge && bridge->name[0]!='b')
    {
      /* case for i-comp add a logical PIP
         if case backbone bridge is added add a
         logical cbp interface as well */

      zsnprintf (ifname_l, INTERFACE_NAMSIZ, "%s.%d", "pip", bridge->bridge_id);

      ifp_l_pip = nsm_bridge_add_pbb_interface (bridge, NSM_VLAN_PORT_MODE_PIP,
                                                ifname_l);

      if (!ifp_l_pip)
          return NSM_PBB_LOGICAL_INTERFACE_ERROR;

      /* Once port adds have been sent, the port-type is sent */
      nsm_vlan_api_set_port_mode (ifp_l_pip, NSM_VLAN_PORT_MODE_PIP,
                                  NSM_VLAN_PORT_MODE_TRUNK, PAL_FALSE,
                                  PAL_FALSE);

      nsm_vlan_port_add_pip (ifp_l_pip);

      if (master->b_bridge)
        {
          zsnprintf (ifname_l, INTERFACE_NAMSIZ, "%s.%d", "cbp", bridge->bridge_id);
          ifp_l_cbp = nsm_bridge_add_pbb_interface (master->b_bridge,
                                                    NSM_VLAN_PORT_MODE_CBP,
                                                    ifname_l);

          if (!ifp_l_cbp)
            return NSM_PBB_LOGICAL_INTERFACE_ERROR;

          /* Once port adds have been sent, the port-type is sent */
          nsm_vlan_api_set_port_mode (ifp_l_cbp, NSM_VLAN_PORT_MODE_CBP,
                                      NSM_VLAN_PORT_MODE_TRUNK, PAL_FALSE,
                                      PAL_FALSE);

       }
    }
  else if (bridge->name[0] == 'b')
    {
      curr = master->bridge_list;
      while (curr)
        {
          if (curr->backbone_edge)
            {
              zsnprintf (ifname_l, INTERFACE_NAMSIZ, "%s.%d", "cbp", curr->bridge_id);

              ifp_l_cbp = nsm_bridge_add_pbb_interface (master->b_bridge,
                                                        NSM_VLAN_PORT_MODE_CBP,
                                                        ifname_l);

              if (!ifp_l_cbp)
                return NSM_PBB_LOGICAL_INTERFACE_ERROR;

              nsm_vlan_api_set_port_mode (ifp_l_cbp, NSM_VLAN_PORT_MODE_CBP,
                                          NSM_VLAN_PORT_MODE_TRUNK, PAL_FALSE,
                                          PAL_FALSE);

            }
          curr = curr->next;
        }
    }
  else
    /* do nothing */
    ;
  return RESULT_OK;
}



int
nsm_pbb_delete_logical_interfaces (struct nsm_bridge *bridge)
{
  char ifname_l[INTERFACE_NAMSIZ + 1];
  struct nsm_bridge *curr = NULL;
  struct interface *ifp_l = NULL;
  struct nsm_bridge_master *master = NULL;
#ifdef HAVE_PBB_TE
  struct pbb_te_inst_entry *pbb_te_entry = NULL;
  struct avl_node *te_node = NULL;
#endif /* HAVE_PBB_TE */
  master = bridge->master;

  if (bridge->backbone_edge && bridge->name[0]!='b')
    {
      /* case for i-comp add a logical PIP
         if case backbone bridge is added add a
         logical cbp interface as well */

      zsnprintf (ifname_l, INTERFACE_NAMSIZ, "%s.%d", "pip", bridge->bridge_id);

      ifp_l = ifg_lookup_by_name (&nzg->ifg, ifname_l);
      nsm_vlan_port_del_pip (ifp_l);

      nsm_bridge_delete_pbb_interface (bridge, ifname_l);

      if (master->b_bridge)
        {
          zsnprintf (ifname_l, INTERFACE_NAMSIZ, "%s.%d", "cbp", bridge->bridge_id);

#ifdef HAVE_PBB_TE
          if (master->beb->bcomp->pbb_te_inst_table)
            {
              /* Deleting the ESPs associated to the cbp */  
              AVL_TREE_LOOP (master->beb->bcomp->pbb_te_inst_table, pbb_te_entry, te_node)
                {
                  if (pbb_te_entry->cbp_ifp &&
                      (pal_strcmp (pbb_te_entry->cbp_ifp->name, ifname_l) == 0))
                    {
                      avl_tree_free (&pbb_te_entry->esp_comp_list, nsm_pbb_te_esp_del);
                      pbb_te_entry->cbp_ifp = NULL;
                      pbb_te_entry->esp_comp_list = NULL;
                    }
                } 

            }
#endif /* HAVE_PBB_TE */
          nsm_bridge_delete_pbb_interface (master->b_bridge,
                                           ifname_l);
       }
    }
  else if (bridge->name[0] == 'b')
    {

      curr = master->bridge_list;
      while (curr)
        {
          if (curr->backbone_edge)
            {
              zsnprintf (ifname_l, INTERFACE_NAMSIZ, "%s.%d", "cbp", curr->bridge_id);

              nsm_bridge_delete_pbb_interface (master->b_bridge,
                                               ifname_l);
            }
          curr = curr->next;
        }
    }
  else
    /* do nothing */
    ;
  return RESULT_OK;
}

int
nsm_pbb_ib_bridge_init (struct nsm_bridge *bridge)
{
  struct nsm_bridge_master *master = NULL;
  
  master = bridge->master;

  if (master == NULL)
    return RESULT_ERROR;

  /* Add logical interface if I-BEB or Backbone bridge */
  if ((bridge->backbone_edge) || (bridge->name[0] == 'b'))
    nsm_pbb_add_logical_interfaces (bridge);
  else
    return RESULT_OK;

#ifdef HAVE_PBB_TE
  if (bridge->name[0] == 'b')
    {
       /* Creating the avl_trees for te_inst_table, 
       * pbb_te_aps_table, pbb_te_aps_isid_table on adding the backbone bridge.
       * This will be the case when backbone bridge is deleted and reconfigured.
       */
      if (master->beb->bcomp->pbb_te_inst_table == NULL)
        avl_create (&master->beb->bcomp->pbb_te_inst_table, 0,
            nsm_pbb_tesid_cmp);

      if (master->beb->bcomp->pbb_te_aps_table == NULL)
        avl_create (&master->beb->bcomp->pbb_te_aps_table, 0,
            nsm_pbb_grpid_cmp);

      if (master->beb->bcomp->pbb_te_aps_isid_table == NULL)
        avl_create (&master->beb->bcomp->pbb_te_aps_isid_table, 0,
            nsm_pbb_te_isid_cmp);

#if defined HAVE_GMPLS && defined HAVE_GELS
      if (master->beb->bcomp->pbb_tesi_name_to_id_table == NULL)
        {
          master->beb->bcomp->pbb_tesi_name_to_id_table =
            hash_create (tesi_name_hash_key_make, tesi_name_hash_cmp);
        }
      if (master->beb->bcomp->pbb_remote_mac_to_tesid_table == NULL)
        {
          avl_create (&master->beb->bcomp->pbb_remote_mac_to_tesid_table, 0,
              nsm_pbb_te_rmac_cmp);
        }
#endif

    }
#endif /* HAVE_PBB_TE */
  return RESULT_OK;
}

int
nsm_pbb_ib_bridge_deinit (struct nsm_bridge *bridge)
{
  struct nsm_bridge_master *master = NULL;
  
  master = bridge->master;

  if (master == NULL)
    return RESULT_ERROR;
  
#ifdef HAVE_PBB_TE
  if (bridge->name[0] == 'b')
    {
      /* On deleting backbone bridge, the te_inst_table,
       * te_aps_isid_table, pbb_te_aps_table are freed
       */
      if (master->beb->bcomp->pbb_te_inst_table)
        {
          avl_tree_free (&master->beb->bcomp->pbb_te_inst_table, NULL);
          master->beb->bcomp->pbb_te_inst_table = NULL;
        }

      if (master->beb->bcomp->pbb_te_aps_isid_table)
        {
          avl_tree_free (&master->beb->bcomp->pbb_te_aps_isid_table, NULL);
          master->beb->bcomp->pbb_te_aps_isid_table = NULL;
        }
      if (master->beb->bcomp->pbb_te_aps_table)
        {
          avl_tree_free (&master->beb->bcomp->pbb_te_aps_table, NULL);
          master->beb->bcomp->pbb_te_aps_table = NULL;
        }
#if defined HAVE_GMPLS && defined HAVE_GELS
      if (master->beb->bcomp->pbb_tesi_name_to_id_table)
        {
          hash_clean (master->beb->bcomp->pbb_tesi_name_to_id_table,
              tesi_name_hash_free);
          hash_free(master->beb->bcomp->pbb_tesi_name_to_id_table);
          master->beb->bcomp->pbb_tesi_name_to_id_table = NULL;
        }

      /* The Delete function passed to this function will delete the list in
         each of the AVL tree nodes */
      if (master->beb->bcomp->pbb_remote_mac_to_tesid_table)
        {
          avl_tree_free (&master->beb->bcomp->pbb_remote_mac_to_tesid_table,
              nsm_pbb_te_rmac_del);
          master->beb->bcomp->pbb_remote_mac_to_tesid_table = NULL;
        }
#endif

    }
#endif /* HAVE_PBB_TE */

  /* IF I-BEB or Backbone bridge, then delete the logical interface */
  if ((bridge->backbone_edge) || (bridge->name[0] == 'b'))
    nsm_pbb_delete_logical_interfaces (bridge);

  return RESULT_OK;
}

#endif /* HAVE_I_BEB && HAVE_B_BEB */

#ifdef HAVE_I_BEB
int
nsm_pbb_i_comp_deinit (struct nsm_bridge *bridge)
{
  struct vip_tbl *vip_node = NULL;
  struct sid2vip_xref *s2v_node = NULL;
  struct nsm_pbb_icomponent *icomp = NULL;
  struct avl_node *node = NULL;
  struct avl_node *next_node = NULL;
  struct nsm_bridge_master *master = NULL;
  struct cnp_srv_tbl *cnp_node = NULL;
  struct vip2pip_map *v2p_node = NULL;
  struct pip_tbl *pip_node = NULL;
 
  master = bridge->master;

  if (master == NULL)
    return RESULT_ERROR;

  icomp = master->beb->icomp;

  if (icomp == NULL)
    return RESULT_ERROR;
      
  AVL_LOOP_DELETE (icomp->sid2vip_xref_list, s2v_node, node, next_node)
    {
      /*Deleting the isids associated to the i-component */
      if (s2v_node->key.icomp_id == bridge->bridge_id)
        {
          vip_node = nsm_pbb_search_vip_node (icomp->vip_table_list,
              bridge->bridge_id,
              s2v_node->vip_port_num);

          if (vip_node == NULL)
            continue;
          
          if (icomp->vip2pip_map_list == NULL)
            continue;

          v2p_node = nsm_pbb_search_vip2pip_node (icomp->vip2pip_map_list,
              bridge->bridge_id,
              vip_node->key.vip_port_num);
          
#ifndef HAVE_B_BEB
          
          if (v2p_node)
            pip_node = nsm_pbb_search_pip_node(icomp->pip_tbl_list,
                bridge->bridge_id, v2p_node->pip_port_number);
           
#else
          pip_node = nsm_pbb_search_pip_node(icomp->pip_tbl_list,
              bridge->bridge_id,
              (NSM_LOGICAL_INTERFACE_INDEX +
               bridge->bridge_id));
#endif /* HAVE_B_BEB */

          if (v2p_node && (v2p_node->key.icomp_id == bridge->bridge_id))
            {
              avl_remove (icomp->vip2pip_map_list, (void *)v2p_node);
              XFREE (MTYPE_NSM_BRIDGE_PBB_VIP2PIP_NODE, v2p_node);
            }

          if (pip_node)
            {
              /*On deleting the isid, the pip_node will not be deleted.
               * The isid mapping to the pip will be unset.
               * So there is no avl_remove
               */
              nsm_pbb_unset_pipvip_map(pip_node->pip_vip_map,
                  vip_node->key.vip_port_num);

              nsm_pbb_unset_pipvip_map(bridge->vip_port_map,
                  vip_node->key.vip_port_num);
            }

          avl_remove (icomp->vip_table_list, (void *)vip_node);
          avl_remove (icomp->sid2vip_xref_list, (void *)s2v_node);
          XFREE (MTYPE_NSM_BRIDGE_PBB_VIP_NODE, vip_node);
          XFREE (MTYPE_NSM_BRIDGE_PBB_SID2VIP_NODE, s2v_node);

        }
    }
  
  AVL_LOOP_DELETE (icomp->cnp_srv_tbl_list, cnp_node, node, next_node)
    {
      if (cnp_node->key.icomp_id == bridge->bridge_id)
        {
          avl_remove (icomp->cnp_srv_tbl_list, (void *)cnp_node);
          XFREE (MTYPE_NSM_BRIDGE_PBB_CNP_NODE, cnp_node);
        }
    }

  return RESULT_OK;
}
#endif /*HAVE_I_BEB */
/* Set beb port with default VID. */
/* For I_BEB PIP node is created when interface mode is made PIP and 
   PIP node is not edited anywhere else except its bitmap*/
int
nsm_vlan_add_beb_port (struct interface *ifp, nsm_vid_t vid,
                       enum nsm_vlan_port_mode mode,
                       enum nsm_vlan_port_mode sub_mode,
                       bool_t iterate_members, bool_t notify_kernel)
{

/* Adding default vlan id to that port*/
  return nsm_vlan_set_common_port (ifp, mode, sub_mode, vid, iterate_members, 
                                   notify_kernel);
}




/* Delete default VLAN from a beb port. */
int
nsm_vlan_del_beb_port (struct interface *ifp, nsm_vid_t vid, 
                       enum nsm_vlan_port_mode mode,
                       enum nsm_vlan_port_mode sub_mode,
                       bool_t iterate_members, bool_t notify_kernel)
{

  return nsm_vlan_delete_common_port (ifp, mode, sub_mode, vid, iterate_members, 
                                      notify_kernel);
}

#ifdef HAVE_I_BEB

/**@Creates an VIP node and in case of IB_BEB mapps it to a PIP.
    @param master - bridge master structure.
    @param *bridge_name - name of the bridge instance associated.
    @param isid - service instance.
    @param *instance_name - name of the service instance.
    @return RESULT_SUCCESS
*/
int
nsm_pbb_create_isid (struct nsm_bridge_master *master, char *bridge_name, u_int32_t isid, 
                     char *instance_name)
{

  struct vip_tbl *vip_node = NULL;
  struct sid2vip_xref *s2v_node = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_pbb_icomponent *icomp = NULL;
#ifdef HAVE_B_BEB
  struct vip2pip_map *v2p_node = NULL;
  struct pip_tbl *pip_node = NULL;
#endif /* HAVE_B_BEB */

  /* Check if i-comp is configured */
  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    return NSM_BRIDGE_ERR_NOTFOUND;

  /* check if the bridge is valid i-component */
  if (!bridge->backbone_edge)
    return NSM_BRIDGE_ERR_NOTFOUND;

  icomp = bridge->master->beb->icomp;

  if (icomp == NULL)
    {
      return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;
    }

  /* search for the existance of ISID in given I component */
  /* Check for the extistance of the same service instance thru sid2vip */

  s2v_node = nsm_pbb_search_sid2vip_node(icomp->sid2vip_xref_list, bridge->bridge_id, isid);
  
  if (s2v_node)
    return NSM_PBB_ISID_EXISTS;

  vip_node = XCALLOC(MTYPE_NSM_BRIDGE_PBB_VIP_NODE,
                     sizeof(struct vip_tbl));
  if (!vip_node)
    {
      return NSM_BRIDGE_ERR_MEM;
    }

  vip_node->key.icomp_id = bridge->bridge_id;

  vip_node->key.vip_port_num = nsm_pbb_get_vip_port_num(bridge->vip_port_map);

  if (vip_node->key.vip_port_num == NSM_PBB_VIP_PIP_MAP_LEN)
    {
      XFREE (MTYPE_NSM_BRIDGE_PBB_VIP_NODE, vip_node);
      return NSM_PBB_VIP_PIP_MAP_EXHAUSTED;
    }

  vip_node->vip_sid = isid;

  vip_node->rowstatus = PBB_SNMP_ROW_STATUS_NOTREADY;

  pal_strncpy(vip_node->srv_inst_name, instance_name,NSM_PBB_ISID_NAME_SIZ);

  /* setting the default mac to oui+isid */
  nsm_pbb_set_vip_default_mac (vip_node, isid);


  /* srvtype and status all are 0 no need to update that.*/

  /* Adding a node to cross reference list as new VIP is being added*/
  s2v_node = nsm_pbb_add_sid2vip_xref (bridge, icomp, isid,
                                       vip_node->key.vip_port_num);
  if (!s2v_node)
    {
      XFREE (MTYPE_NSM_BRIDGE_PBB_VIP_NODE, vip_node);
      return NSM_BRIDGE_ERR_MEM;
    }
#ifdef HAVE_B_BEB
  /* In IB_BEB scenario all instance belonging to a i-comp 
     will fall to same PIP so binding is done here itself 
     where as in only I-BEB scenario binding of isid to vip will depend 
     on the service instance mapping created at PIP interface */

  pip_node = nsm_pbb_search_pip_node(icomp->pip_tbl_list,
                                     bridge->bridge_id,
                                    (NSM_LOGICAL_INTERFACE_INDEX +
                                     bridge->bridge_id));

  if (pip_node)
    {
      nsm_pbb_set_pipvip_map(pip_node->pip_vip_map,
                             vip_node->key.vip_port_num);

      v2p_node = nsm_pbb_add_vip2pip_map(icomp,pip_node,
                                         vip_node->key.vip_port_num);
      if (!v2p_node)
        {
          avl_remove (icomp->sid2vip_xref_list, s2v_node);
          XFREE (MTYPE_NSM_BRIDGE_PBB_SID2VIP_NODE, s2v_node);
          XFREE (MTYPE_NSM_BRIDGE_PBB_VIP_NODE, vip_node);
          return NSM_BRIDGE_ERR_MEM;
        }
    }
  /* else   
     if no pip means corresponding i-comp is not created
     which cannot be the case */
#endif /* HAVE_B_BEB */

  /* insert the vip to the vip_list in i-comp container class */
  avl_insert(icomp->vip_table_list,(void *)vip_node);
  return RESULT_OK;
}

/**@deletes ISID and all its associations.
    @param master - bridge master structure.
    @param *bridge_name - name of the bridge instance associated.
    @param isid - service instance.
    @return RESULT_SUCCESS
*/

int
nsm_pbb_delete_isid (struct nsm_bridge_master *master, char *bridge_name, 
                     u_int32_t isid)
{
  struct vip_tbl *vip_node = NULL;
  struct sid2vip_xref *s2v_node = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_pbb_icomponent *icomp = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct avl_node *avl_port = NULL;
  struct interface *ifp = NULL;
  struct nsm_if *zif = NULL;
  struct cnp_srv_tbl *cnp_node = NULL;
#ifdef HAVE_B_BEB
  struct vip2pip_map *v2p_node = NULL;
  struct pip_tbl *pip_node = NULL;
#endif /* HAVE_B_BEB */


  /* Check if i-comp is configured */
  bridge = nsm_lookup_bridge_by_name (master, bridge_name);

  if (! bridge)
    return NSM_BRIDGE_ERR_NOTFOUND;

  icomp = bridge->master->beb->icomp;

  if (icomp == NULL)
    {
      return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;
    }

  /* search for the existance of ISID in given I component */
  /* Check for the extistance of the same service instance thru sid2vip */

  if (!isid)
    return NSM_PBB_ISID_NOT_CREATED;

  /* search VIP by ISID */
  s2v_node = nsm_pbb_search_sid2vip_node(icomp->sid2vip_xref_list, bridge->bridge_id, isid);

  if (!s2v_node)
    return NSM_PBB_ISID_NOT_CREATED;

  vip_node = nsm_pbb_search_vip_node (icomp->vip_table_list,
                                      bridge->bridge_id,
                                      s2v_node->vip_port_num);
  if (vip_node)
    {
       avl_remove (icomp->vip_table_list, (void *)vip_node);
       avl_remove (icomp->sid2vip_xref_list, (void *)s2v_node);
    }
  else
   return NSM_PBB_ISID_NOT_CREATED;
  /* else will never be reached */

  /* Delete the svlan or portbased mapping to this isid */
  if (bridge->port_tree)
    {
      AVL_TREE_LOOP (bridge->port_tree, br_port, avl_port )
        {
          if ((zif = br_port->nsmif) == NULL)
            continue;

          if ((ifp = zif->ifp) == NULL)
            continue;

          vlan_port = &br_port->vlan_port;

          if (vlan_port->mode != NSM_VLAN_PORT_MODE_CNP)
            continue;

          cnp_node = nsm_pbb_search_cnp_node (icomp->cnp_srv_tbl_list, 
                                      bridge->bridge_id, isid, 
                                      ifp->ifindex);

          if (!cnp_node)
            continue;

          if (cnp_node->srv_type == NSM_SERVICE_INTERFACE_PORTBASED)
            {
              nsm_vlan_port_beb_del_cnp_portbased (ifp, isid);
            }
          else
            {
              nsm_vlan_port_beb_del_cnp_svlan_based (ifp, isid);
            }
        }
    }

#ifdef HAVE_B_BEB 
  v2p_node = nsm_pbb_search_vip2pip_node (icomp->vip2pip_map_list,
                                          bridge->bridge_id,
                                          vip_node->key.vip_port_num);


  pip_node = nsm_pbb_search_pip_node(icomp->pip_tbl_list,
                                     bridge->bridge_id,
                                    (NSM_LOGICAL_INTERFACE_INDEX +
                                     bridge->bridge_id));

  if (!pip_node || !v2p_node)
    return NSM_PBB_ISID_NOT_CREATED;
  /* above if is invalid condition and will never be reached */


  avl_remove (icomp->vip2pip_map_list, (void *)v2p_node);
  
  nsm_pbb_unset_pipvip_map(pip_node->pip_vip_map,
                           vip_node->key.vip_port_num);

  nsm_pbb_unset_pipvip_map(bridge->vip_port_map,
                           vip_node->key.vip_port_num);

  XFREE (MTYPE_NSM_BRIDGE_PBB_VIP2PIP_NODE, v2p_node);
#endif /* HAVE_B_BEB */

  XFREE (MTYPE_NSM_BRIDGE_PBB_VIP_NODE, vip_node);
  XFREE (MTYPE_NSM_BRIDGE_PBB_SID2VIP_NODE, s2v_node);

  return RESULT_OK;
}


/* Adds a portbased CNP instance */
/* name parameter is always NULL */
int 
nsm_vlan_port_beb_add_cnp_portbased (struct interface *ifp, 
                                     struct interface * ifp1, 
                                     u_int32_t isid,
                                     u_char *instance_name)
{
  struct nsm_if *zif = NULL;
  struct nsm_bridge *bridge;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port;
  struct nsm_pbb_icomponent *icomp = NULL; 
  struct cnp_srv_tbl *cnp_node = NULL;
  struct vip_tbl *vip_node = NULL;
  struct avl_node *node;
#ifndef HAVE_B_BEB
  struct nsm_if *zif1 = NULL;
  struct nsm_vlan_port *vlan_port1 = NULL;
  struct nsm_bridge_port *br_port1 = NULL;
  struct vip2pip_map *v2p_node = NULL;
  struct pip_tbl *pip_node = NULL;
#endif /*HAVE_B_BEB*/ 
  struct nsm_msg_pbb_isid_to_pip isid2pip;
  int ret = 0;

  if (ifp) 
    zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

#ifndef HAVE_B_BEB
  if (ifp1)
    zif1 = (struct nsm_if *)ifp1->info;

  if (zif1 == NULL || zif1->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port1 = zif1->switchport;
  vlan_port1 = &br_port1->vlan_port;
#endif /* !HAVE_B_BEB */

#ifndef HAVE_B_BEB
  if(zif->bridge && (zif->bridge == zif1->bridge))  
    bridge = zif->bridge;
  else
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

#else /*HAVE_B_BEB*/
  if (zif->bridge )
    {
      bridge = zif->bridge;
    }
  else
    return NSM_VLAN_ERR_IFP_NOT_BOUND;
#endif /*HAVE_B_BEB*/     

  icomp = bridge->master->beb->icomp;

  if (!icomp)
    return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

  if (!vlan_port || (vlan_port->mode != NSM_VLAN_PORT_MODE_CNP))
    return NSM_PBB_VLAN_ERR_PORT_MODE_INVALID; 

#ifndef HAVE_B_BEB
  /* check if ifp1 is PIP */
  if (!vlan_port1 || vlan_port1->mode != NSM_VLAN_PORT_MODE_PIP)
    return NSM_PBB_VLAN_ERR_PORT_MODE_INVALID;
#endif /* !HAVE_B_BEB */

  /*Check to see if any cnp configuartion is done on this interface*/
  /* This check is only for portbased CNPs */
  for (node = avl_top(icomp->cnp_srv_tbl_list); node; node = avl_next (node))
    {
      if ((cnp_node = (struct cnp_srv_tbl *)
           AVL_NODE_INFO (node)) == NULL)
                  continue;
      if (cnp_node->key.icomp_port_num == ifp->ifindex)
        {
           return NSM_PBB_VLAN_ERR_PORT_CNP;
        }
    }

  vip_node = nsm_pbb_search_vip_by_isid (icomp, isid, bridge->bridge_id);

  if (!vip_node)
    return NSM_PBB_ISID_NOT_CREATED;

#ifndef HAVE_B_BEB
  /* check if same service is mapped to another PIP */

  v2p_node = nsm_pbb_search_vip2pip_node(icomp->vip2pip_map_list,
                                         bridge->bridge_id,
                                         vip_node->key.vip_port_num);


  if (v2p_node && v2p_node->pip_port_number != ifp1->ifindex)
    return NSM_PBB_SERVICE_MAPPED_TO_DIFF_PIP;
#endif /* HAVE_B_BEB */

/* no configuration needs to be updated if same cnp is there..
   need to delete and add another portbased config. */

   cnp_node = XCALLOC (MTYPE_NSM_BRIDGE_PBB_CNP_NODE,
                        sizeof(struct cnp_srv_tbl));
   if (!cnp_node)
     return NSM_BRIDGE_ERR_MEM;

   pal_mem_set (cnp_node , 0, sizeof(struct cnp_srv_tbl));

   cnp_node->key.icomp_id = bridge->bridge_id;
   cnp_node->srv_type = NSM_SERVICE_INTERFACE_PORTBASED;
   cnp_node->key.icomp_port_num = ifp->ifindex;
   cnp_node->key.sid = isid;
   cnp_node->srv_status = NSM_SERVICE_STATUS_NOT_DISPATCHED;
   cnp_node->svlan_bundle_list = NULL;
#ifndef HAVE_B_BEB
   cnp_node->ref_port_num = ifp1->ifindex;
#else
   cnp_node->ref_port_num = NSM_LOGICAL_INTERFACE_INDEX + bridge->bridge_id;
#endif /* !HAVE_I_BEB */
   cnp_node ->rowstatus = PBB_SNMP_ROW_STATUS_NOTREADY;

   avl_insert (icomp->cnp_srv_tbl_list, (void *)cnp_node);

  /* create service to PIP mapping in case it is not yet created*/  

#ifndef HAVE_B_BEB  
  if (!v2p_node)
    {
      pip_node = nsm_pbb_search_pip_node(icomp->pip_tbl_list,
                                         bridge->bridge_id, 
                                         cnp_node->ref_port_num);
      if (pip_node)
        {
          nsm_pbb_set_pipvip_map(pip_node->pip_vip_map,
                                 vip_node->key.vip_port_num);
          v2p_node = nsm_pbb_add_vip2pip_map(icomp,pip_node,
                                             vip_node->key.vip_port_num);

          if (!v2p_node)
            {
              ret = NSM_BRIDGE_ERR_MEM; 
              goto ERROR_EXIT;
            }
        }
    }
#endif /* HAVE_B_BEB */

  /*Sending add portbased message to PMs*/
  pal_mem_cpy(isid2pip.bridge_name, bridge->name, pal_strlen(bridge->name));
  isid2pip.cnp_ifindex       = ifp->ifindex;
  isid2pip.pip_ifindex       = cnp_node->ref_port_num;
  isid2pip.svid_low          = vlan_port->pvid;
  isid2pip.svid_high         = vlan_port->pvid;
  isid2pip.isid              = isid;
  isid2pip.dispatch_status   = NSM_SERVICE_STATUS_NOT_DISPATCHED;

  nsm_pbb_send_isid2svid(bridge, &isid2pip, NSM_MSG_PBB_ISID_TO_PIP_ADD);

  return ret;

  
#ifndef HAVE_B_BEB  
ERROR_EXIT:
  if (v2p_node)
    {
      avl_remove (icomp->vip2pip_map_list, (void *)v2p_node);
      nsm_pbb_unset_pipvip_map (bridge->vip_port_map,
                                vip_node->key.vip_port_num);
      XFREE (MTYPE_NSM_BRIDGE_PBB_VIP2PIP_NODE, v2p_node);
      if (pip_node)
        nsm_pbb_unset_pipvip_map(pip_node->pip_vip_map,
                                 vip_node->key.vip_port_num);           
    }
#endif /* HAVE_B_BEB */
  return ret;
}

int 
nsm_vlan_port_beb_add_cnp_svlan_based(struct interface *ifp, 
                                      struct interface *ifp1,
                                      u_int32_t isid,u_char * instance_name,
                                      nsm_vid_t start_vid, nsm_vid_t end_vid)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  struct nsm_pbb_icomponent *icomp = NULL;
  struct cnp_srv_tbl *cnp_node = NULL;
  struct vip_tbl *vip_node = NULL;
  struct svlan_bundle *svlan_b;
  struct avl_node *node;
  int ret = 0;
  struct nsm_msg_pbb_isid_to_pip isid2pip;
  struct avl_tree *vlan_table = NULL;
  int index = 0;
  struct nsm_vlan p;
 
#ifndef HAVE_B_BEB
  struct nsm_if *zif1;
  struct nsm_vlan_port *vlan_port1;
  struct nsm_bridge_port *br_port1;
  struct vip2pip_map *v2p_node = NULL;
  struct pip_tbl *pip_node = NULL;
#endif /* HAVE_B_BEB */

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

#ifndef HAVE_B_BEB
  zif1 = (struct nsm_if *)ifp1->info;

  if (zif1 == NULL || zif1->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port1 = zif1->switchport;
  vlan_port1 = &br_port1->vlan_port;
#endif /* !HAVE_B_BEB */

  bridge = zif->bridge;

#ifndef HAVE_B_BEB
  if(zif->bridge != zif1->bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;
#endif /*HAVE_B_BEB*/

  icomp = bridge->master->beb->icomp;

  if (!vlan_port||vlan_port->mode != NSM_VLAN_PORT_MODE_CNP)
    return NSM_PBB_VLAN_ERR_PORT_MODE_INVALID;

#ifndef HAVE_B_BEB
  if (!vlan_port || vlan_port1->mode != NSM_VLAN_PORT_MODE_PIP)
    return NSM_PBB_VLAN_ERR_PORT_MODE_INVALID;
#endif /* !HAVE_B_BEB */

  /* Check if all the vlans are added to the bridge */
  /* Need to check if these vlans are associated with 
     the interface as well TBD */

  vlan_table = bridge->svlan_table;

  if (!vlan_table)
    return NSM_PBB_VLAN_ERR_PORT_CNP;

  /* API always receives start_vid <= end_vid */
  for (index = start_vid ; index <= end_vid; index++)
    {
       NSM_VLAN_SET (&p, index);

       node = avl_search (vlan_table, (void *)&p);
       if (!node)
         {
           /* vlan not added to bridge yet..cannot be used for mapping */
           return NSM_PBB_VLAN_ERR_PORT_CNP;
         }
    }

  /* Need to optimize below search by placing a flag in 
     zif structure TBD */
  for (node = avl_top(icomp->cnp_srv_tbl_list);
                      node; node = avl_next (node))
    {
      if ((cnp_node = (struct cnp_srv_tbl *)
           AVL_NODE_INFO (node)) == NULL)
        continue;
      if (cnp_node->key.icomp_port_num == ifp->ifindex
          && cnp_node->srv_type == NSM_SERVICE_INTERFACE_PORTBASED)
       {
          return NSM_PBB_VLAN_ERR_PORT_CNP;
       }
    }

  vip_node = nsm_pbb_search_vip_by_isid (icomp, isid, bridge->bridge_id);

  if (!vip_node)
    return NSM_PBB_ISID_NOT_CREATED;

#ifndef HAVE_B_BEB
  /* check if same service is mapped to another PIP */
  /* This can happen only in the case of only I_BEB */
  v2p_node = nsm_pbb_search_vip2pip_node(icomp->vip2pip_map_list,
                                         bridge->bridge_id,
                                         vip_node->key.vip_port_num);


  if (v2p_node && v2p_node->pip_port_number != ifp1->ifindex)
    return NSM_PBB_SERVICE_MAPPED_TO_DIFF_PIP;
#endif /* HAVE_B_BEB */

  cnp_node = nsm_pbb_search_cnp_node (icomp->cnp_srv_tbl_list,
                                      bridge->bridge_id, isid, ifp->ifindex);

  if (cnp_node && (cnp_node->srv_status == NSM_SERVICE_STATUS_DISPATCHED))
    return NSM_PBB_VLAN_ERR_SERVICE_DISPATCHED;

  /* create a cnp node here */
  if (!cnp_node)
    {
      cnp_node = XCALLOC (MTYPE_NSM_BRIDGE_PBB_CNP_NODE,
                          sizeof(struct cnp_srv_tbl));

      if (!cnp_node)
        return NSM_BRIDGE_ERR_MEM;

      pal_mem_set(cnp_node, 0, sizeof(struct cnp_srv_tbl));

      cnp_node->key.icomp_id = bridge->bridge_id;
      cnp_node->key.icomp_port_num = ifp->ifindex;
      cnp_node->key.sid = isid;
      cnp_node->srv_status = NSM_SERVICE_STATUS_NOT_DISPATCHED;
      cnp_node->svlan_bundle_list = NULL;
#ifdef HAVE_B_BEB
      cnp_node->ref_port_num = NSM_LOGICAL_INTERFACE_INDEX + bridge->bridge_id;
#else
      cnp_node->ref_port_num = ifp1->ifindex;
#endif /* HAVE_B_BEB */
      cnp_node ->rowstatus = PBB_SNMP_ROW_STATUS_NOTREADY;

      if ((start_vid == NSM_VLAN_CLI_MIN) && (end_vid == NSM_VLAN_CLI_MAX))
        cnp_node->srv_type = NSM_SERVICE_INTERFACE_SVLAN_ALL;
      else
        cnp_node->srv_type = NSM_SERVICE_INTERFACE_SVLAN_MANY;

      avl_insert (icomp->cnp_srv_tbl_list, (void *)cnp_node);
    }


  if (cnp_node->svlan_bundle_list == NULL)
    {
      cnp_node->svlan_bundle_list = list_new();
    }

  svlan_b = XCALLOC(MTYPE_NSM_BRIDGE_PBB_SVLAN_BUNDLE,
                    sizeof(struct svlan_bundle));
  if (!svlan_b)
    {
      ret = NSM_BRIDGE_ERR_MEM;
      goto ERROR_EXIT;
    }

  svlan_b->svid_low = start_vid;
  svlan_b->svid_high = end_vid;
  svlan_b->service_type = NSM_PORT_SERVICE_BOTH;
  svlan_b->bundle_status = NSM_SERVICE_STATUS_NOT_DISPATCHED;
  listnode_add(cnp_node->svlan_bundle_list, (void*)svlan_b);
     
#ifndef HAVE_B_BEB
  if (!v2p_node)
    {
      pip_node = nsm_pbb_search_pip_node(icomp->pip_tbl_list,
                                         bridge->bridge_id,
                                         cnp_node->ref_port_num);
      if (pip_node)
        {
          nsm_pbb_set_pipvip_map(pip_node->pip_vip_map,
                                 vip_node->key.vip_port_num);
          v2p_node = nsm_pbb_add_vip2pip_map(icomp,pip_node,
                                             vip_node->key.vip_port_num);

          if (!v2p_node)
            {
              ret = NSM_BRIDGE_ERR_MEM;
              goto ERROR_EXIT;
            }
        }
    }
#endif /* HAVE_B_BEB */

  /* Sending message to other PMs the addition 
     notification of vlan to ISID mapping */

  pal_mem_cpy(isid2pip.bridge_name, bridge->name, pal_strlen(bridge->name));
  isid2pip.cnp_ifindex       = ifp->ifindex;
  isid2pip.pip_ifindex       = cnp_node->ref_port_num;
  isid2pip.svid_low          = start_vid;
  isid2pip.svid_high         = end_vid;
  isid2pip.isid              = isid;
  isid2pip.dispatch_status   = NSM_SERVICE_STATUS_NOT_DISPATCHED;

  nsm_pbb_send_isid2svid(bridge, &isid2pip, NSM_MSG_PBB_ISID_TO_PIP_ADD);

  return ret;

ERROR_EXIT:
  if(cnp_node)
    {
      avl_remove (icomp->cnp_srv_tbl_list, (void *)cnp_node);
      XFREE (MTYPE_NSM_BRIDGE_PBB_CNP_NODE, cnp_node);
    }
  return ret;
}


int 
nsm_vlan_port_beb_del_cnp_portbased(struct interface *ifp, u_int32_t isid)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  struct cnp_srv_tbl *cnp_node,*cnp_node_temp;
  struct vip_tbl *vip_node = NULL;
  struct pip_tbl *pip_node = NULL;
  struct vip2pip_map *v2p_node = NULL;
  struct avl_node *node = NULL;
  u_char flag = 0;
  struct sid2vip_xref *s2v_node = NULL;
  struct nsm_pbb_icomponent *icomp = NULL;
  struct nsm_msg_pbb_isid_to_pip isid2pip;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *vlan_port = NULL;

  zif = (struct nsm_if *)ifp->info;

  if (!zif || !zif->switchport || !zif->bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  bridge = zif->bridge;
  icomp = bridge->master->beb->icomp;

  cnp_node = nsm_pbb_search_cnp_node (icomp->cnp_srv_tbl_list, 
                                      bridge->bridge_id, isid, 
                                      ifp->ifindex);

  if (!cnp_node)
    return NSM_PBB_VLAN_ERR_PORT_ENTRY_DOESNOT_EXIST;

  if (cnp_node->srv_type != NSM_SERVICE_INTERFACE_PORTBASED)
     return NSM_PBB_VLAN_ERR_PORT_ENTRY_DOESNOT_EXIST;

  if (cnp_node->srv_status ==  NSM_SERVICE_STATUS_DISPATCHED)
    return NSM_PBB_VLAN_ERR_SERVICE_DISPATCHED;

  avl_remove (icomp->cnp_srv_tbl_list,
                     (void *)cnp_node);

  /* Check for VIP-PIP mapping deletion */
  /* expensive search need optimization here */
  for (node =avl_top(icomp->cnp_srv_tbl_list); node; node = avl_next (node))
    {
      cnp_node_temp = (struct cnp_srv_tbl *)AVL_NODE_INFO (node);
      if (cnp_node_temp->key.sid == cnp_node->key.sid)
        {
          flag = 1;
          break;
        }
    }

  if (!flag)
    {
      /* The case when this is the last mapping for this ISID */
      s2v_node = nsm_pbb_search_sid2vip_node (icomp->sid2vip_xref_list,
                                              bridge->bridge_id,
                                              isid);

      if (!s2v_node)
        return RESULT_ERROR; /* Should never reach here */

      vip_node = nsm_pbb_search_vip_node (icomp->vip_table_list,
                                          bridge->bridge_id,
                                          s2v_node->vip_port_num);

      if (!vip_node)
        return RESULT_ERROR; /* Should never reach here */

      v2p_node = nsm_pbb_search_vip2pip_node (icomp->vip2pip_map_list,
                                              bridge->bridge_id,
                                              vip_node->key.vip_port_num);

      pip_node = nsm_pbb_search_pip_node(icomp->pip_tbl_list,
                                         bridge->bridge_id,
                                         cnp_node->ref_port_num);

      if (!v2p_node || !pip_node)
        return RESULT_ERROR; /* Should never reach here */

      nsm_pbb_unset_pipvip_map (pip_node->pip_vip_map,
                                vip_node->key.vip_port_num);

      avl_remove (icomp->vip2pip_map_list, (void *)v2p_node);
    }

  /* sending delete message to PMs*/
  pal_mem_cpy(isid2pip.bridge_name, bridge->name, pal_strlen(bridge->name));
  isid2pip.cnp_ifindex       = ifp->ifindex;
  isid2pip.pip_ifindex       = cnp_node->ref_port_num;
  isid2pip.svid_low          = vlan_port->pvid;
  isid2pip.svid_high         = vlan_port->pvid;
  isid2pip.isid              = isid;
  nsm_pbb_send_isid2svid(bridge, &isid2pip,NSM_MSG_PBB_SVID_TO_ISID_DEL);

  XFREE (MTYPE_NSM_BRIDGE_PBB_CNP_NODE, cnp_node);
  return 0;
}


/* This API delete the range of svlan mapping to isid */
/* the isid mapping with PIP is kept intact though */
/* For now only the exact range which was added can be deleted */
/* need to optimize this with svlan bitmaps TBD */
int 
nsm_vlan_port_beb_del_cnp_svlan(struct interface *ifp, u_int32_t isid,
                                nsm_vid_t start_vid, nsm_vid_t end_vid)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  struct nsm_pbb_icomponent *icomp = NULL;
  struct cnp_srv_tbl *cnp_node = NULL;
  struct svlan_bundle *svlan_bdl = NULL;
  struct listnode *svlan_node = NULL;
  struct nsm_msg_pbb_isid_to_pip isid2pip;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;
  bridge = zif->bridge;

  if (!bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  icomp = bridge->master->beb->icomp;

  cnp_node = nsm_pbb_search_cnp_node(icomp->cnp_srv_tbl_list, bridge->bridge_id,
                                     isid, ifp->ifindex);
  if (!cnp_node)
    return NSM_PBB_VLAN_ERR_PORT_ENTRY_DOESNOT_EXIST;

  if (cnp_node->srv_status == NSM_SERVICE_STATUS_DISPATCHED)
    return NSM_PBB_VLAN_ERR_SERVICE_DISPATCHED ;

  if (cnp_node->svlan_bundle_list && 
      listcount (cnp_node->svlan_bundle_list) > 0 )
    LIST_LOOP ( cnp_node->svlan_bundle_list, svlan_bdl, svlan_node)
      if (svlan_bdl->svid_low == start_vid
          && svlan_bdl->svid_high == end_vid)
        {
          listnode_delete(cnp_node->svlan_bundle_list, (void *) svlan_bdl);
          XFREE (MTYPE_NSM_BRIDGE_PBB_SVLAN_BUNDLE, svlan_bdl);
          break;
        }
  /* Posting message to other PMs */
  pal_mem_set(&isid2pip,0,sizeof(isid2pip));

  isid2pip.cnp_ifindex       = ifp->ifindex;
  isid2pip.pip_ifindex       = cnp_node->ref_port_num;
  isid2pip.svid_low          = start_vid;
  isid2pip.svid_high         = end_vid;
  isid2pip.isid              = isid;
  nsm_pbb_send_isid2svid(bridge, &isid2pip, NSM_MSG_PBB_SVID_TO_ISID_DEL);
                    
  return 0;
}

int
nsm_vlan_port_beb_del_cnp_svlan_based (struct interface *ifp, u_int32_t isid)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  struct nsm_pbb_icomponent *icomp = NULL;
  struct cnp_srv_tbl *cnp_node, *cnp_node_temp;
  u_char flag = 0;
  struct vip_tbl *vip_node = NULL;
  struct pip_tbl *pip_node = NULL;
  struct vip2pip_map *v2p_node = NULL;
  struct sid2vip_xref *s2v_node = NULL;
  struct avl_node *node;
  struct nsm_msg_pbb_isid_to_pip isid_at_pip_delete;

  zif = (struct nsm_if *)ifp->info;

  if (!zif || !zif->switchport || !zif->bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  bridge = zif->bridge;
  icomp = bridge->master->beb->icomp;

  cnp_node = nsm_pbb_search_cnp_node(icomp->cnp_srv_tbl_list, bridge->bridge_id,
                                     isid, ifp->ifindex);

  if (!cnp_node)
    return NSM_PBB_VLAN_ERR_PORT_ENTRY_DOESNOT_EXIST; 

  if (cnp_node->srv_status == NSM_SERVICE_STATUS_DISPATCHED)
    return NSM_PBB_VLAN_ERR_SERVICE_DISPATCHED;

  /* If port based do not delete the entry from this command*/
  if (cnp_node->srv_type == NSM_SERVICE_INTERFACE_PORTBASED)
    return NSM_PBB_VLAN_ERR_PORT_ENTRY_DOESNOT_EXIST;

  avl_remove (icomp->cnp_srv_tbl_list, (void *)cnp_node);

  if (cnp_node->svlan_bundle_list)
    {
      list_delete_all_node(cnp_node->svlan_bundle_list);
      list_delete(cnp_node->svlan_bundle_list);   
    }

  /* Check for deletion of VIP */

  for (node =avl_top(icomp->cnp_srv_tbl_list); node;
       node = avl_next (node))
    {
      cnp_node_temp = (struct cnp_srv_tbl *)AVL_NODE_INFO (node);
      if (cnp_node_temp->key.sid == isid)
        {
          flag = 1;
          break;
        }
    }
 
  if (!flag)
    {
      /* The case when this is the last mapping for this ISID */
      s2v_node = nsm_pbb_search_sid2vip_node (icomp->sid2vip_xref_list,
                                              bridge->bridge_id,
                                              isid);

      if (!s2v_node)
        return RESULT_ERROR; /* Should never reach here */

      vip_node = nsm_pbb_search_vip_node (icomp->vip_table_list,
                                          bridge->bridge_id,
                                          s2v_node->vip_port_num);

      if (!vip_node)
        return RESULT_ERROR; /* Should never reach here */

      v2p_node = nsm_pbb_search_vip2pip_node (icomp->vip2pip_map_list,
                                              bridge->bridge_id,
                                              vip_node->key.vip_port_num);

      pip_node = nsm_pbb_search_pip_node(icomp->pip_tbl_list,
                                         bridge->bridge_id,
                                         cnp_node->ref_port_num);

      if (!v2p_node || !pip_node)
        return RESULT_ERROR; /* Should never reach here */

      nsm_pbb_unset_pipvip_map (pip_node->pip_vip_map,
                                vip_node->key.vip_port_num);

      avl_remove (icomp->vip2pip_map_list, (void *)v2p_node);
    }

  pal_mem_set(&isid_at_pip_delete, 0, sizeof(isid_at_pip_delete));

  isid_at_pip_delete.cnp_ifindex       = ifp->ifindex;
  isid_at_pip_delete.pip_ifindex       = cnp_node->ref_port_num;
  isid_at_pip_delete.svid_low          = NSM_VLAN_CLI_MIN;
  isid_at_pip_delete.svid_high         = NSM_VLAN_CLI_MAX;
  isid_at_pip_delete.isid              = isid;
  nsm_pbb_send_isid2svid(bridge, &isid_at_pip_delete, NSM_MSG_PBB_SVID_TO_ISID_DEL);

  XFREE (MTYPE_NSM_BRIDGE_PBB_CNP_NODE, cnp_node);
  return 0;
}

/* Below are PIP addition and deletion API's for only I_BEB */
#if (defined HAVE_I_BEB) 
int 
nsm_vlan_port_add_pip (struct interface *ifp)
{
  struct nsm_if *zif;
  struct nsm_bridge *br;
  zif = (struct nsm_if *)ifp->info;
  struct pip_tbl *pip_node = NULL;

  if  (!zif || !zif->bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br = zif->bridge;

  pip_node = nsm_pbb_search_pip_node(br->master->beb->icomp->pip_tbl_list,
                                     br->bridge_id, ifp->ifindex);
  if (!pip_node)
    {
      pip_node = XCALLOC (MTYPE_NSM_BRIDGE_PBB_PIP_NODE,
                          sizeof (struct pip_tbl));
      if (!pip_node)
        return NSM_BRIDGE_ERR_MEM;
      pal_mem_set ((void*)pip_node, 0, sizeof(struct pip_tbl));

      pip_node->key.icomp_id = br->bridge_id;
      pip_node->key.pip_port_num = ifp->ifindex;
      pal_mem_cpy (pip_node->pip_src_bmac, ifp->hw_addr, ETHER_ADDR_LEN);
      pal_strncpy (pip_node->pip_port_name, ifp->name,
                   NSM_PBB_PORT_NAME_SIZ);
      /*pip_node->pip_vip_map is already set to 0;*/
      listnode_add (br->master->beb->icomp->pip_tbl_list, (void *)pip_node);
    }
  return 0;
}

int
nsm_vlan_port_del_pip (struct interface *ifp)
{
  struct nsm_if *zif;
  struct nsm_bridge *br;

  struct pip_tbl *pip_node = NULL;

  if (!ifp)
    return RESULT_ERROR;

  zif = (struct nsm_if *)ifp->info;
  if  (!zif || !zif->bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br = zif->bridge;


  pip_node = nsm_pbb_search_pip_node(br->master->beb->icomp->pip_tbl_list,
                                     br->bridge_id, ifp->ifindex);

  if (pip_node)
    {
      listnode_delete(br->master->beb->icomp->pip_tbl_list,
                      (void *) pip_node);
      XFREE(MTYPE_NSM_BRIDGE_PBB_PIP_NODE, pip_node);
    }
  return 0;
}
#endif /* HAVE_I_BEB */


/* configure a VIP node */
/* API will always receive name as NULL */ 
int 
nsm_vlan_port_config_vip(struct interface *ifp, u_int32_t isid, 
                         u_char *instance_name, u_int8_t default_mac[],
                         enum nsm_port_service_type egress_type)
{

  struct vip_tbl *vip_node;
  struct nsm_bridge *bridge;
  struct nsm_if *zif;
  struct nsm_pbb_icomponent *icomp ;
  struct vip2pip_map *v2p_node = NULL;
  int ret = NSM_PBB_VLAN_ERR_PORT_ENTRY_DOESNOT_EXIST;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL || !zif->bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  bridge = zif->bridge;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  icomp = bridge->master->beb->icomp;

  if (vlan_port && vlan_port->mode != NSM_VLAN_PORT_MODE_PIP)
    return NSM_PBB_VLAN_ERR_PORT_MODE_INVALID;

  vip_node = nsm_pbb_search_vip_by_isid (icomp, isid, 
                                         bridge->bridge_id);
  
  if (!vip_node)
    return ret; /* isid not configured */
  
   v2p_node = nsm_pbb_search_vip2pip_node (icomp->vip2pip_map_list,
                                           bridge->bridge_id,
                                           vip_node->key.vip_port_num);
  if (!v2p_node)
    return ret;

  /* check if VIP is bound to given PIP */
  if (v2p_node->pip_port_number != ifp->ifindex)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;
 
  if (default_mac)
    pal_mem_cpy (vip_node->default_dst_bmac, default_mac, ETHER_ADDR_LEN);

  if (egress_type != -1)
    vip_node->srv_type = egress_type;

  return 0;
}
#endif /*HAVE_I_BEB*/

#if (defined HAVE_I_BEB) && (defined HAVE_B_BEB)

int 
nsm_vlan_port_beb_pip_mac_update (struct interface *ifp, u_int8_t src_bmac[])
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  struct nsm_pbb_icomponent *icomp = NULL;
  char ifname[INTERFACE_NAMSIZ + 1];
  struct pip_tbl *pip_node = NULL;
  cindex_t cindex = 0;

  if (ifp)
    zif = (struct nsm_if *)ifp->info;
  else
    return NSM_VLAN_ERR_IFP_NOT_BOUND;


  if (!zif || !zif->switchport || !zif->bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;
  
  if (pal_strncmp(ifp->name, "pip", 3))
    return NSM_PBB_VLAN_ERR_PORT_MODE_INVALID; 

  bridge = zif->bridge;
  icomp = bridge->master->beb->icomp;

  pip_node = nsm_pbb_search_pip_node (icomp->pip_tbl_list, bridge->bridge_id,
                                      ifp->ifindex);
  if (pip_node)
    {
      pal_mem_cpy (pip_node->pip_src_bmac, src_bmac, ETHER_ADDR_LEN);
      pal_mem_cpy (ifp->hw_addr, src_bmac, ETHER_ADDR_LEN);       

      /* updating MAC address for corresponding CBP as well */
      zsnprintf (ifname, INTERFACE_NAMSIZ, "%s.%d", "cbp", bridge->bridge_id);
      ifp = ifg_lookup_by_name (&nzg->ifg, ifname);
      if (ifp)
        pal_mem_cpy (ifp->hw_addr, src_bmac, ETHER_ADDR_LEN);
      /* send nsm link update message to cfm */
      NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_HW_ADDRESS);
      if (ifp)
      /* Announce to the PMs.  */
      nsm_server_if_update (ifp, cindex);

      ifp = NULL;
      /* updating MAC address for corresponding PIP as well */
      zsnprintf (ifname, INTERFACE_NAMSIZ, "%s.%d", "pip", bridge->bridge_id);
      ifp = ifg_lookup_by_name (&nzg->ifg, ifname);
      if (ifp)
        pal_mem_cpy (ifp->hw_addr, src_bmac, ETHER_ADDR_LEN);
      /* send nsm link update message to cfm */
      NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_HW_ADDRESS);
      if (ifp)
      /* Announce to the PMs.  */
      nsm_server_if_update (ifp, cindex);
    }
  return 0;
}

#endif /* HAVE_I_BEB && HAVE_B_BEB */

#ifdef HAVE_B_BEB

int  
nsm_vlan_port_beb_add_cbp_srv_inst (struct interface *ifp, u_int32_t isid, 
                                    u_int32_t isid_map, nsm_vid_t bvid, 
                                    enum nsm_vlan_port_mode type, 
                                    u_int8_t default_mac[], bool_t edge)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  struct nsm_pbb_bcomponent *bcomp = NULL;
  struct cbp_srvinst_tbl *cbp_node=NULL;
  struct nsm_msg_pbb_isid_to_bvid isid_to_bvid;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  struct nsm_vlan p;
  struct avl_node *rn = NULL;
#if (defined HAVE_I_BEB) && (defined HAVE_B_BEB)
  u_int8_t bmac[ETHER_ADDR_LEN+1] = {0};
#endif /* HAVE_I_BEB && HAVE_B_BEB */
  if (ifp)
    zif = (struct nsm_if *)ifp->info;
  else 
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

#if (defined HAVE_I_BEB) && (defined HAVE_B_BEB)  
  if (!pal_mem_cmp (ifp->hw_addr, bmac, ETHER_ADDR_LEN))
    return NSM_PBB_VLAN_ERR_PORT_ADDR_NOT_SET;
#endif
  if (!zif || !zif->switchport || !zif->bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  if (vlan_port && vlan_port->mode != NSM_VLAN_PORT_MODE_CBP)
    return NSM_PBB_VLAN_ERR_PORT_MODE_INVALID;

  bridge = zif->bridge;
  /* check for if the bvid had been added to the bridge table */

  NSM_VLAN_SET (&p, bvid);

  rn = avl_search (bridge->bvlan_table, (void *)&p);
  if (rn == NULL  || rn->info == NULL)
    return NSM_VLAN_ERR_VLAN_NOT_FOUND;

  if (pal_strncmp(bridge->name, "backbone", 8))
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  bcomp =  bridge->master->beb->bcomp;
  
  cbp_node = nsm_pbb_search_cbp_node (bcomp->cbp_srvinst_list, 
                                      ifp->ifindex, isid);
  if (!cbp_node)
    {
    /* Incase entry exists edit the remaining fields ?*/
      cbp_node = XCALLOC(MTYPE_NSM_BRIDGE_PBB_CBP_NODE, 
                         sizeof(struct cbp_srvinst_tbl));
      if (!cbp_node)
        return NSM_BRIDGE_ERR_MEM;

      pal_mem_set (cbp_node, 0, sizeof(struct cbp_srvinst_tbl));
 
      cbp_node->key.bcomp_id = 1; /* 1 for b-comp backbone bridge */
      cbp_node->key.bcomp_port_num = ifp->ifindex;
      cbp_node->key.b_sid = isid;
      cbp_node->status = NSM_SERVICE_STATUS_NOT_DISPATCHED;
      cbp_node->pt_mpt_type = NSM_BVLAN_P2P;
      avl_insert(bcomp->cbp_srvinst_list, (void *) cbp_node);
    }
  if (cbp_node->status == NSM_SERVICE_STATUS_DISPATCHED)
    return NSM_PBB_VLAN_ERR_SERVICE_DISPATCHED;

  cbp_node->bvid = bvid;

  if (type != NSM_BVLAN_P2P)
    cbp_node->pt_mpt_type = type;

  if (isid_map)
    cbp_node->local_sid = isid_map;

  if (default_mac)
    pal_mem_cpy (cbp_node->default_dst_bmac, 
                 default_mac, ETHER_ADDR_LEN);
  if (edge)
    cbp_node->edge = edge;

  pal_mem_cpy (isid_to_bvid.bridge_name, bridge->name, pal_strlen(bridge->name));
  isid_to_bvid.cbp_ifindex       = ifp->ifindex;
  isid_to_bvid.isid              = isid;
  isid_to_bvid.bvid              = bvid;
  isid_to_bvid.dispatch_status   = NSM_SERVICE_STATUS_NOT_DISPATCHED;

  nsm_pbb_send_isid2bvid (bridge, &isid_to_bvid, NSM_MSG_PBB_ISID_TO_BVID_ADD);

  return 0;
}

int 
nsm_vlan_port_beb_delete_cbp_srv_inst (struct interface *ifp, u_int32_t isid)
{
  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  struct nsm_pbb_bcomponent *bcomp = NULL;
  struct cbp_srvinst_tbl *cbp_node = NULL;
#ifndef HAVE_I_BEB
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
#endif /* !HAVE_I_BEB */
  struct nsm_msg_pbb_isid_to_bvid isid_to_bvid;
  u_int16_t bvid;

  zif = (struct nsm_if *)ifp->info;

  if (!zif || !zif->bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  bridge = zif->bridge;
 
  if (pal_strncmp(bridge->name, "backbone", 8))
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

#ifndef HAVE_I_BEB
  if (!zif->switchport)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  if (vlan_port && vlan_port->mode != NSM_VLAN_PORT_MODE_CBP)
    return NSM_PBB_VLAN_ERR_PORT_MODE_INVALID;
#else
  if (pal_strncmp (ifp->name, "cbp", 3))
    return NSM_VLAN_ERR_IFP_NOT_BOUND;
#endif /* !HAVE_I_BEB */

  bcomp =  bridge->master->beb->bcomp;  

  cbp_node = nsm_pbb_search_cbp_node(bcomp->cbp_srvinst_list, 
                                     ifp->ifindex, isid);

  if (!cbp_node)
    return NSM_PBB_VLAN_ERR_PORT_ENTRY_DOESNOT_EXIST;

  if (cbp_node->status == NSM_SERVICE_STATUS_DISPATCHED)
    return NSM_PBB_VLAN_ERR_SERVICE_DISPATCHED;

#ifdef HAVE_B_BEB
  bvid = cbp_node->bvid;
#endif /* HAVE_B_BEB */

  avl_remove (bcomp->cbp_srvinst_list,(void *)cbp_node);

  XFREE(MTYPE_NSM_BRIDGE_PBB_CBP_NODE, cbp_node);

  pal_mem_cpy(isid_to_bvid.bridge_name, bridge->name, pal_strlen(bridge->name));
  isid_to_bvid.cbp_ifindex       = ifp->ifindex;
  isid_to_bvid.isid              = isid;
  isid_to_bvid.bvid              = bvid;
  isid_to_bvid.dispatch_status   = 0;

  nsm_pbb_send_isid2bvid (bridge, &isid_to_bvid, NSM_MSG_PBB_ISID_TO_BVID_DEL);
              
  return 0;
}


/* Adding BVID to PNP port */
int 
nsm_vlan_port_add_pnp(struct interface *ifp, nsm_vid_t bvid)
{

  /* nothing needs to be done for pnp as there is no related structure for pnp in beb table*/

  return nsm_vlan_add_common_port (ifp, NSM_VLAN_PORT_MODE_PNP, 
                                   NSM_VLAN_PORT_MODE_TRUNK, 
                                   bvid, NSM_VLAN_EGRESS_TAGGED, 
                                   PAL_TRUE, PAL_TRUE);
}


/* Deleting BVID from PNP port*/
int 
nsm_vlan_port_del_pnp (struct interface *ifp, nsm_vid_t bvid)
{

  /* nothing needs to be done for pnp as there is no related structure for pnp in beb table*/

  return nsm_vlan_delete_common_port (ifp, NSM_VLAN_PORT_MODE_PNP, 
                                      NSM_VLAN_PORT_MODE_HYBRID, 
                                      bvid,  PAL_TRUE, PAL_TRUE);
}
#endif /*HAVE_B_BEB*/


int 
nsm_pbb_check_service (struct interface *ifp,u_int32_t instance_id, 
                       u_char *instance_name)
{

  struct nsm_if *zif;
  struct nsm_bridge *bridge;
#ifdef HAVE_I_BEB
  struct cnp_srv_tbl *cnp_node;
  struct vip_tbl *vip_node = NULL;
  struct nsm_pbb_icomponent *icomp = NULL;
  struct sid2vip_xref  *sid2vip_node = NULL;  
#endif
#ifdef HAVE_B_BEB
  struct nsm_pbb_bcomponent *bcomp = NULL;
  struct cbp_srvinst_tbl* cbp_node;
#endif
  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL || !zif->bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  bridge = zif->bridge;

#ifdef HAVE_I_BEB 
  icomp =  bridge->master->beb->icomp;  
  cnp_node = nsm_pbb_search_cnp_node (icomp->cnp_srv_tbl_list, 
                                      bridge->bridge_id, instance_id, 
                                      ifp->ifindex);
  /* No CNP node, service cant be dispatched */
  if (!cnp_node)
    return NSM_PBB_SERVICE_DISPATCH_ERROR;
  else if (cnp_node->srv_status != NSM_SERVICE_STATUS_DISPATCHED)
    {
      sid2vip_node = nsm_pbb_search_sid2vip_node (icomp->sid2vip_xref_list, 
                                                  bridge->bridge_id, instance_id);

      if (sid2vip_node)
        vip_node = nsm_pbb_search_vip_node (icomp->vip_table_list, 
                                            bridge->bridge_id, 
                                            sid2vip_node->vip_port_num);
     
      if (vip_node && vip_node->status!= NSM_SERVICE_STATUS_DISPATCHED)
        {
#ifdef HAVE_B_BEB
          bcomp = bridge->master->beb->bcomp;
          cbp_node = nsm_pbb_search_cbp_node (bcomp->cbp_srvinst_list, 
                                              cnp_node->ref_port_num, 
                                              instance_id);

          if (cbp_node && cbp_node->status != NSM_SERVICE_STATUS_DISPATCHED)
            return 0;
#else
          return 0;

#endif 
        }
    }
#else
  bcomp = bridge->master->beb->bcomp;
  cbp_node = nsm_pbb_search_cbp_node (bcomp->cbp_srvinst_list, 
                                      ifp->ifindex, instance_id);
  if (cbp_node && cbp_node->status != NSM_SERVICE_STATUS_DISPATCHED)
    return 0;
  
#endif
  return NSM_PBB_SERVICE_DISPATCH_ERROR;
}
        

int 
nsm_pbb_dispatch_service (struct interface *ifp, u_int32_t isid, u_char *name)
{

#ifdef HAVE_B_BEB 
  struct nsm_pbb_bcomponent *bcomp = NULL;
  struct cbp_srvinst_tbl* cbp_node = NULL;
#endif
#ifdef HAVE_I_BEB
  struct nsm_pbb_icomponent *icomp = NULL;
  struct cnp_srv_tbl *cnp_node = NULL;
  struct vip_tbl *vip_node = NULL;
  struct listnode *lcnp_node = NULL;
  struct pip_tbl *pip_node = NULL;
  struct sid2vip_xref  *sid2vip_node = NULL;  
  struct vip2pip_map *vip2pip_node = NULL;
  struct svlan_bundle *svlan_bdl = NULL;
#endif
  struct nsm_if *zif = NULL;
  struct nsm_bridge *bridge = NULL;
  int ret = 0;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;


  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL || !zif->bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  bridge = zif->bridge;

  /*Sanity check to make sure that dispatch is done at appropriate 
  interface */

#ifdef HAVE_I_BEB
  if (!vlan_port || vlan_port->mode != NSM_VLAN_PORT_MODE_CNP)
    return NSM_PBB_VLAN_ERR_PORT_MODE_INVALID;

#else
  if (!vlan_port || vlan_port->mode != NSM_VLAN_PORT_MODE_CBP)
    return NSM_PBB_VLAN_ERR_PORT_MODE_INVALID;

#endif


#if defined (HAVE_I_BEB) 
  icomp = bridge->master->beb->icomp;
  cnp_node = nsm_pbb_search_cnp_node (icomp->cnp_srv_tbl_list, 
                                      bridge->bridge_id, isid, ifp->ifindex);

  if (cnp_node && cnp_node->srv_status != NSM_SERVICE_STATUS_DISPATCHED)
    {
      LIST_LOOP (cnp_node->svlan_bundle_list, svlan_bdl, lcnp_node)
        if (svlan_bdl->bundle_status != NSM_SERVICE_STATUS_DISPATCHED)
          {
#ifdef HAVE_HAL
            ret = hal_pbb_dispatch_service_cnp(bridge->name, ifp->ifindex, 
                                               isid, svlan_bdl->svid_high,
                                               svlan_bdl->svid_low,
                                               svlan_bdl->service_type);
#endif 
            svlan_bdl->bundle_status = NSM_SERVICE_STATUS_DISPATCHED;
          }
    }
  else  	 
    return NSM_PBB_SERVICE_DISPATCH_ERROR;
    /*No CNP node or already dispatched service cant be dispatched */

  sid2vip_node = nsm_pbb_search_sid2vip_node(icomp->sid2vip_xref_list,
                                             bridge->bridge_id, isid);
  if (sid2vip_node)
    vip_node = nsm_pbb_search_vip_node(icomp->vip_table_list, 
                                       bridge->bridge_id,
                                       sid2vip_node->vip_port_num);

  if (vip_node && vip_node->status != NSM_SERVICE_STATUS_DISPATCHED)
    {
#ifdef HAVE_HAL
      ret = hal_pbb_dispatch_service_vip(bridge->name, ifp->ifindex, isid, 
                                         vip_node->default_dst_bmac, 
                                         vip_node->srv_type);
#endif
    }
  else
    return NSM_PBB_SERVICE_DISPATCH_ERROR;
  /*No VIP node or already dispatched service cant be dispatched */
  	
#ifdef HAVE_B_BEB
  bcomp = bridge->master->beb->bcomp;

  pip_node = nsm_pbb_search_pip_node (icomp->pip_tbl_list, bridge->bridge_id, 
                                      ifp->ifindex);

  vip2pip_node = nsm_pbb_search_vip2pip_node (icomp->vip2pip_map_list, 
                                              bridge->bridge_id, 
                                              vip_node->key.vip_port_num);

  if (pip_node && vip2pip_node && 
      vip2pip_node->status!= NSM_SERVICE_STATUS_DISPATCHED)
    {
#ifdef HAVE_HAL
      ret = hal_pbb_dispatch_service_pip(bridge->name, ifp->ifindex, isid);
#endif
    }
  else
    return NSM_PBB_SERVICE_DISPATCH_ERROR;

  
  cbp_node = nsm_pbb_search_cbp_node(bcomp->cbp_srvinst_list, 
                                     cnp_node->ref_port_num, isid);

  if (cbp_node && cbp_node->status != NSM_SERVICE_STATUS_DISPATCHED)
    {
#ifdef HAVE_HAL
      ret = hal_pbb_dispatch_service_cbp("backbone", cnp_node->ref_port_num, 
                                         cbp_node->bvid, cbp_node->key.b_sid, 
                                         cbp_node->local_sid, 
                                         cbp_node->default_dst_bmac, 
                                         cbp_node->pt_mpt_type);
#endif
      cbp_node->status = NSM_SERVICE_STATUS_DISPATCHED;
    }
  else
    return NSM_PBB_SERVICE_DISPATCH_ERROR;

#else /*only I component scenario*/
  pip_node = nsm_pbb_search_pip_node (icomp->pip_tbl_list,
                                      bridge->bridge_id,
                                      cnp_node->ref_port_num);

  vip2pip_node = nsm_pbb_search_vip2pip_node (icomp->vip2pip_map_list,
                                              bridge->bridge_id,
                                              vip_node->key.vip_port_num );

  if (pip_node && vip2pip_node &&
      vip2pip_node->status!= NSM_SERVICE_STATUS_DISPATCHED)
    {
#ifdef HAVE_HAL
      ret = hal_pbb_dispatch_service_pip(bridge->name, cnp_node->ref_port_num,
                                         isid);
#endif
    }
  else
    return NSM_PBB_SERVICE_DISPATCH_ERROR;

#endif 
  cnp_node->srv_status = NSM_SERVICE_STATUS_DISPATCHED;
  vip2pip_node->status = NSM_SERVICE_STATUS_DISPATCHED;
  vip_node->status = NSM_SERVICE_STATUS_DISPATCHED;
#else /*only HAVE_B_BEB */
  bcomp = bridge->master->beb->bcomp;
  cbp_node = nsm_pbb_search_cbp_node(bcomp->cbp_srvinst_list, 
                                     ifp->ifindex, isid);
  /* need to find out the correct index in this case */
  if (cbp_node && cbp_node->status!= NSM_SERVICE_STATUS_DISPATCHED)
    {
      ret = hal_pbb_dispatch_service_cbp("backbone", ifp->ifindex, 
                                         cbp_node->bvid, cbp_node->key.b_sid, 
		 	                 cbp_node->local_sid,
                                         cbp_node->default_dst_bmac, 
		 	                 cbp_node->pt_mpt_type);
      
      /* need to find out the correct index in this case */
      cbp_node->status = NSM_SERVICE_STATUS_DISPATCHED;
    }
  else
    return NSM_PBB_SERVICE_DISPATCH_ERROR;

#endif /*HAVE_I_BEB*/	
  return ret;
}

int 
nsm_pbb_remove_service(struct interface *ifp, u_int32_t isid, u_char *name)
{

  struct nsm_if *zif;
  struct nsm_bridge *bridge;
  int ret = 0;
#ifdef HAVE_I_BEB
  struct nsm_pbb_icomponent *icomp = NULL;
  struct cnp_srv_tbl *cnp_node = NULL;
  struct vip_tbl *vip_node = NULL;
  struct sid2vip_xref *sid2vip_node = NULL;
  struct vip2pip_map *v2p_node = NULL;
#endif
#ifdef HAVE_B_BEB
  struct cbp_srvinst_tbl* cbp_node = NULL;
  struct nsm_pbb_bcomponent *bcomp = NULL;
#endif
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;

  zif = (struct nsm_if *)ifp->info;

  if (zif == NULL || zif->switchport == NULL || !zif->bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  br_port = zif->switchport;
  vlan_port = &br_port->vlan_port;

  bridge = zif->bridge;
#ifdef HAVE_I_BEB 
  icomp = bridge->master->beb->icomp;
#endif
#ifdef HAVE_B_BEB
  bcomp = bridge->master->beb->bcomp;
#endif

#ifdef HAVE_I_BEB
  if (!vlan_port || vlan_port->mode != NSM_VLAN_PORT_MODE_CNP)
    return NSM_PBB_VLAN_ERR_PORT_MODE_INVALID;

#else
  if (!vlan_port || vlan_port->mode != NSM_VLAN_PORT_MODE_CBP)
    return NSM_PBB_VLAN_ERR_PORT_MODE_INVALID;

#endif

#ifdef HAVE_I_BEB

  sid2vip_node = nsm_pbb_search_sid2vip_node (icomp->sid2vip_xref_list,
                                              bridge->bridge_id, isid);

  if (sid2vip_node)
    vip_node = nsm_pbb_search_vip_node (icomp->vip_table_list,
                                        bridge->bridge_id,
                                        sid2vip_node->vip_port_num);

  if (!vip_node || (vip_node->status == NSM_SERVICE_STATUS_NOT_DISPATCHED))
    return NSM_PBB_SERVICE_REMOVE_ERROR;

  cnp_node = nsm_pbb_search_cnp_node (icomp->cnp_srv_tbl_list,
                                      bridge->bridge_id,
                                      isid, ifp->ifindex);

  if (!cnp_node || (cnp_node->srv_status == NSM_SERVICE_STATUS_NOT_DISPATCHED))
    return NSM_PBB_SERVICE_REMOVE_ERROR;

 
#ifdef HAVE_B_BEB
  v2p_node = nsm_pbb_search_vip2pip_node (icomp->vip2pip_map_list,
                                          bridge->bridge_id,
                                          vip_node->key.vip_port_num);

  cbp_node = nsm_pbb_search_cbp_node (bcomp->cbp_srvinst_list,
                                      cnp_node->ref_port_num, isid);

  if (!cbp_node || cbp_node->status == NSM_SERVICE_STATUS_NOT_DISPATCHED)
     return NSM_PBB_SERVICE_REMOVE_ERROR;
#else

  v2p_node = nsm_pbb_search_vip2pip_node (icomp->vip2pip_map_list,
                                          bridge->bridge_id,
                                          vip_node->key.vip_port_num);
    
#endif
  if (!v2p_node || v2p_node->status == NSM_SERVICE_STATUS_NOT_DISPATCHED)
    return NSM_PBB_SERVICE_REMOVE_ERROR;

  cnp_node->srv_status = NSM_SERVICE_STATUS_NOT_DISPATCHED;
  vip_node->status = NSM_SERVICE_STATUS_NOT_DISPATCHED;
  v2p_node->status = NSM_SERVICE_STATUS_NOT_DISPATCHED;

#ifdef HAVE_B_BEB
  cbp_node->status = NSM_SERVICE_STATUS_NOT_DISPATCHED;
#endif

#else/*only B_BEB*/
  cbp_node = nsm_pbb_search_cbp_node (bcomp->cbp_srvinst_list, 
                                      ifp->ifindex, isid);
  if (!cbp_node || cbp_node->status == NSM_SERVICE_STATUS_NOT_DISPATCHED)
    return NSM_PBB_SERVICE_REMOVE_ERROR;
  else 
    cbp_node->status = NSM_SERVICE_STATUS_NOT_DISPATCHED;
#endif

#ifdef HAVE_HAL
  ret = hal_pbb_remove_service (bridge->name, ifp->ifindex, isid);
#endif

  return ret;
}



#ifdef HAVE_I_BEB

struct sid2vip_xref *
nsm_pbb_add_sid2vip_xref(struct nsm_bridge *bridge, 
                         struct nsm_pbb_icomponent *icomp,
                         u_int32_t isid,
                         u_int32_t vip_port_num)
{

  struct sid2vip_xref *s2v_node;

  s2v_node = XCALLOC (MTYPE_NSM_BRIDGE_PBB_SID2VIP_NODE, 
                      sizeof(struct sid2vip_xref));
  if(!s2v_node)
    return NULL;
  s2v_node->key.icomp_id = bridge->bridge_id;
  s2v_node->key.vip_sid = isid;
  s2v_node->vip_port_num = vip_port_num;
  avl_insert (icomp->sid2vip_xref_list, (void *) s2v_node);
  return s2v_node;
}

struct vip2pip_map *
nsm_pbb_add_vip2pip_map (struct nsm_pbb_icomponent *icomp, 
                         struct pip_tbl *pip_node, 
                         u_int32_t vip_port_num)
{

  struct vip2pip_map *v2p_node;
  
  v2p_node = XCALLOC (MTYPE_NSM_BRIDGE_PBB_VIP2PIP_NODE, 
                      sizeof(struct vip2pip_map));
  if(!v2p_node)
    return NULL;

  v2p_node->key.icomp_id  = pip_node->key.icomp_id;
  v2p_node->key.vip_port_num = vip_port_num;
  
  pal_mem_cpy (v2p_node->pip_src_bmac, pip_node->pip_src_bmac, ETHER_ADDR_LEN);
  v2p_node->pip_port_number = pip_node->key.pip_port_num;
  pal_strncpy (v2p_node->storage_type,"PERMANENT",32);
  v2p_node->status = NSM_SERVICE_STATUS_NOT_DISPATCHED;
  avl_insert (icomp->vip2pip_map_list, (void *) v2p_node);
  return v2p_node;
}
#endif/*HAVE_I_BEB*/



int 
nsm_pbb_init (struct nsm_master *nm)
{
  u_int8_t ret;
  struct nsm_beb_bridge *beb;
  struct nsm_bridge_master *master;
#ifdef HAVE_I_BEB
  struct nsm_pbb_icomponent *icomp;
#endif
#ifdef HAVE_B_BEB
  struct nsm_pbb_bcomponent *bcomp;
#endif

  ret = 0;

  master = nsm_bridge_get_master (nm);

  if (! master)
    {
      zlog_warn (nm->zg, "NSM Bridge Master is not initialised");
      return RESULT_ERROR;
    }

  if (master->beb)
    {
      return 0;
    }

  beb = XCALLOC (MTYPE_NSM_BRIDGE_PBB, sizeof(struct nsm_beb_bridge));

  if (! beb)
    return NSM_BRIDGE_ERR_MEM;

  master->beb = beb;
  beb->master = master;
#ifdef HAVE_I_BEB
  beb->beb_tot_icomp = 0;
#endif /* HAVE_I_BEB */
#ifdef HAVE_B_BEB
  beb->beb_tot_bcomp = 0;
#endif /* HAVE_B_BEB */
  beb->beb_tot_ext_ports = 0;
  pal_strncpy (beb->beb_name, "BEB Bridge", 11);
  pal_strncpy (beb->backbone_bridge_name, "BEB Backbone Bridge", 20); 
  
/* Initialise I-Component tables*/
#ifdef HAVE_I_BEB
  icomp = XCALLOC (MTYPE_NSM_BRIDGE_PBB_ICOMP, 
                   sizeof(struct nsm_pbb_icomponent));
  if(!icomp)
    return NSM_BRIDGE_ERR_MEM;
  /* check for icomp==NULL */
  beb->icomp = icomp;

  ret = avl_create (&beb->icomp->vip_table_list, 0, nsm_pbb_icomp_vip_cmp);
  ret = avl_create (&beb->icomp->sid2vip_xref_list, 0, nsm_pbb_icomp_sid2vip_cmp);
  ret = avl_create (&beb->icomp->vip2pip_map_list, 0, nsm_pbb_icomp_vip2pip_cmp);
  ret = avl_create (&beb->icomp->cnp_srv_tbl_list, 0, nsm_pbb_icomp_cnp_cmp);

  icomp->pip_tbl_list = list_create (NULL, NULL);
  
#endif /* HAVE_I_BEB */

/* Initialise B-Component tables*/

#ifdef HAVE_B_BEB
  bcomp = XCALLOC (MTYPE_NSM_BRIDGE_PBB_BCOMP, 
                   sizeof(struct nsm_pbb_bcomponent));
  /* check for NULL of bcomp */
  if(!bcomp)
    return NSM_BRIDGE_ERR_MEM;

  beb->bcomp = bcomp;
  ret = avl_create (&beb->bcomp->cbp_srvinst_list, 0, nsm_pbb_bcomp_cbp_cmp);

#if defined HAVE_PBB_TE && defined HAVE_I_BEB
  ret = avl_create (&beb->bcomp->pbb_te_inst_table, 0, nsm_pbb_tesid_cmp);
  ret = avl_create (&beb->bcomp->pbb_te_aps_table, 0, nsm_pbb_grpid_cmp);
  ret = avl_create (&beb->bcomp->pbb_te_aps_isid_table, 0, nsm_pbb_te_isid_cmp);
#if defined HAVE_GMPLS && defined HAVE_GELS
  beb->bcomp->pbb_tesi_name_to_id_table = hash_create (tesi_name_hash_key_make, tesi_name_hash_cmp);
  ret = avl_create (&beb->bcomp->pbb_remote_mac_to_tesid_table, 0, nsm_pbb_te_rmac_cmp);
#endif
#endif /* HAVE_PBB_TE && HAVE_I_BEB */
#endif /* HAVE_B_BEB */ 
  return 0;
}

void 
nsm_pbb_deinit (struct nsm_master *nm)
{
  struct nsm_bridge_master *master;
  struct nsm_beb_bridge *beb;

  if (!nm )
    return;

  master = nsm_bridge_get_master (nm);

  if (! master || ! master->beb)
    return;

  beb = master->beb;

#ifdef HAVE_I_BEB
  avl_tree_free (&beb->icomp->vip_table_list, NULL);
  avl_tree_free (&beb->icomp->sid2vip_xref_list, NULL);
  avl_tree_free (&beb->icomp->vip2pip_map_list, NULL);
  avl_tree_free (&beb->icomp->cnp_srv_tbl_list, NULL);
  list_delete (beb->icomp->pip_tbl_list);   

  XFREE (MTYPE_NSM_BRIDGE_PBB_ICOMP, beb->icomp);
#endif /* HAVE_I_BEB */
 
#ifdef HAVE_B_BEB
  avl_tree_free (&beb->bcomp->cbp_srvinst_list, NULL);
#if defined HAVE_PBB_TE && defined HAVE_I_BEB
  avl_tree_free (&beb->bcomp->pbb_te_inst_table, NULL);
  avl_tree_free (&beb->bcomp->pbb_te_aps_table, NULL);
  avl_tree_free (&beb->bcomp->pbb_te_aps_isid_table, NULL);
#if defined HAVE_GMPLS && defined HAVE_GELS
  hash_clean (beb->bcomp->pbb_tesi_name_to_id_table, tesi_name_hash_free);
  hash_free(beb->bcomp->pbb_tesi_name_to_id_table);
  beb->bcomp->pbb_tesi_name_to_id_table = NULL;

  /* The Delete function passed to this function will delete the list in
     each of the AVL tree nodes */
  avl_tree_free (&beb->bcomp->pbb_remote_mac_to_tesid_table, nsm_pbb_te_rmac_del);
  beb->bcomp->pbb_remote_mac_to_tesid_table = NULL;
#endif
#endif /* HAVE_PBB_TE && HAVE_I_BEB */

  XFREE (MTYPE_NSM_BRIDGE_PBB_BCOMP, beb->bcomp);

#endif /* HAVE_B_BEB */

  XFREE (MTYPE_NSM_BRIDGE_PBB, beb);
  master->beb = NULL;

  return;
}


void  
nsm_bridge_beb_show (struct cli *cli, u_char *brname)
{
  struct nsm_master *nm = NULL;
  struct nsm_bridge_master *master = NULL;
  struct nsm_bridge *bridge = NULL;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port;
  struct interface *ifp = NULL;
  struct nsm_if *zif = NULL;
  struct avl_node *avl_port;

  if ( (!cli) || (!brname))
    return;
  
  nm = cli->vr->proto;
  
  if (nm)
    master = nsm_bridge_get_master (nm);

  if (!master)
    return;  

  bridge = nsm_lookup_bridge_by_name (master, brname);

  if ( !bridge )
    return ;

  if (!bridge->port_tree)
    return ;

  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
          AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;
     
      if (zif->bridge)
        if (pal_strcmp (zif->bridge->name, brname) == 0) 
          {
            if ( !(zif = (struct nsm_if *)ifp->info) )
              return;
            if ( !(br_port = zif->switchport) )
              return;
            if ( !(vlan_port = &br_port->vlan_port) )
              {
                cli_out(cli, "No Port Vlan Info for %s\n", ifp->name);
                return;
              }
       
            cli_out (cli, " Interface name         : %s\n", ifp->name);
            cli_out (cli, " Switchport mode        : ");

            if (vlan_port->mode == NSM_VLAN_PORT_MODE_PNP)/* due to port overloading*/
              cli_out (cli, "%s\n", "pnp");
            else
              cli_out (cli, "%s\n", nsm_vlan_if_mode_to_str (vlan_port->mode));
          }
    }
}

void
nsm_bridge_beb_port (struct cli *cli, char *brname, enum nsm_vlan_port_mode mode)
{
  struct nsm_master *nm = NULL;
  struct nsm_bridge_master *master = NULL;
  struct nsm_bridge *br = NULL;
  struct interface *ifptemp = NULL;
  struct nsm_vlan *vlan;
  struct avl_node *rn,*node;
#ifdef HAVE_I_BEB
  char *isid_name;
  struct svlan_bundle *svlan_bdl;
  struct listnode *svlan_node, *pip_lnode;
  struct cnp_srv_tbl *cnp_node;
  struct vip_tbl *vip_node;
  struct pip_tbl *pip_node;
  u_int32_t isid;
#endif
#ifdef HAVE_B_BEB
  struct cbp_srvinst_tbl *cbp_node;
#endif 
  
  if ( (!cli) || (!brname))
    return;

  nm = cli->vr->proto;

  if (nm)
    master = nsm_bridge_get_master (nm);

  if (!master)
    return;

  br = nsm_lookup_bridge_by_name(master, brname);

  if ( !br )
    return ;
  
  switch (mode)
    {
#ifdef HAVE_I_BEB
      case NSM_VLAN_PORT_MODE_CNP:
        {
          cli_out (cli, "CNP configuration for bridge %s \n", brname);
          cli_out (cli, "========================================================\n");
          cli_out (cli, "Interface  ServiceType ServiceID ServiceName svlans\n");
          cli_out (cli, "========================================================\n");
          for (node = avl_top(br->master->beb->icomp->cnp_srv_tbl_list);
               node; node = avl_next (node))
            {
              if ((cnp_node = (struct cnp_srv_tbl *)
                  AVL_NODE_INFO (node)) == NULL)
                continue;
              if (br->bridge_id != cnp_node->key.icomp_id)
                continue;
              isid = cnp_node->key.sid;
              /* API to get instance name from ISID */
              isid_name = get_pbb_instance_name (isid, br->master->beb->icomp);

              if (!isid_name)
                continue;
              
              ifptemp = if_lookup_by_index (&br->master->nm->vr->ifm,
                                            cnp_node->key.icomp_port_num);
              if (ifptemp == NULL)
                continue;

              switch (cnp_node->srv_type)
                {
                  case NSM_SERVICE_INTERFACE_PORTBASED:
                    cli_out (cli," %s   PortBased %u     %s \n", 
                             ifptemp->name, isid, isid_name);
                  break;
                  case NSM_SERVICE_INTERFACE_SVLAN_ALL:
                    cli_out (cli," %s   Svlan All %u     %s \n",
                             ifptemp->name, isid, isid_name);
                  break;
          
                  case NSM_SERVICE_INTERFACE_SVLAN_SINGLE:
                    cli_out (cli," %s   SvlanBased  %u    %s "
                             , ifptemp->name, isid, isid_name);

                    if (listcount (cnp_node->svlan_bundle_list) > 0)
                      LIST_LOOP (cnp_node->svlan_bundle_list, svlan_bdl, svlan_node)
                        {
                           cli_out (cli,"  %d \n", svlan_bdl->svid_low);
                        }
                    cli_out (cli, "\n"); 
                  break;
                  case NSM_SERVICE_INTERFACE_SVLAN_MANY:
                    cli_out (cli," %s   SvlanBased %u     %s "
                             , ifptemp->name, isid, isid_name);

                    if (listcount (cnp_node->svlan_bundle_list) > 0)
                      LIST_LOOP (cnp_node->svlan_bundle_list, svlan_bdl,
                                 svlan_node)
                        {
                          cli_out (cli," %d to %d \n", svlan_bdl->svid_low,
                                   svlan_bdl->svid_high);
                        }
                  default:
                  break;
                }

              for (rn = avl_top (br->vlan_table); rn; rn = avl_next (rn))
                if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
                  {
                    cli_out (cli," Default vlan %d customer-network \n", 
                             vlan->vid);
                  }
            }
        }
      break;
      case NSM_VLAN_PORT_MODE_VIP:
        {
          cli_out (cli, "VIP configuration for bridge %s \n", brname);
          cli_out (cli, "========================================================\n");
          cli_out (cli, "Interface  Instance InstanceName def-DBMAC status\n");
          cli_out (cli, "========================================================\n");
 
          for (node =avl_top(br->master->beb->icomp->vip_table_list); node;
               node = avl_next (node))
            {
              if ((vip_node = (struct vip_tbl *)
                  AVL_NODE_INFO (node)) == NULL)
                continue;
              if ( vip_node->key.icomp_id != br->bridge_id)
                continue;
              cli_out (cli, " %d     %u      %s ", vip_node->key.vip_port_num,
                       vip_node->vip_sid, vip_node->srv_inst_name); 

              cli_out (cli,"  %.04hx.%.04hx.%.04hx ",
                       pal_ntoh16(((unsigned short *)vip_node->default_dst_bmac)[0]),
                       pal_ntoh16(((unsigned short *)vip_node->default_dst_bmac)[1]),
                       pal_ntoh16(((unsigned short *)vip_node->default_dst_bmac)[2]));


              if (vip_node->status == NSM_SERVICE_STATUS_DISPATCHED)
                {
                  cli_out (cli, " Dispatched \n");
                }
              else
                {
                  cli_out (cli, " Not Dispatched \n");
                }
            }
        }
      break;
      case NSM_VLAN_PORT_MODE_PIP:
        {
          cli_out (cli, "PIP configuration for bridge %s \n", brname);
          cli_out (cli, "========================================================\n");
          cli_out (cli, "icompid port_num port_name \n");
          cli_out (cli, "========================================================\n");

          LIST_LOOP (br->master->beb->icomp->pip_tbl_list, pip_node, pip_lnode)
            {
              if ( br->bridge_id != pip_node->key.icomp_id)
                continue;
              cli_out (cli, "%d    %d     %s  \n", pip_node->key.icomp_id,
                       pip_node->key.pip_port_num, pip_node->pip_port_name);
            }

        }
      break;
#endif /*HAVE_I_BEB*/

#ifdef HAVE_B_BEB
      case NSM_VLAN_PORT_MODE_CBP:
        {
          if (!pal_strncmp(brname,"backbone",8))
            {
              for (rn = avl_top (br->bvlan_table); rn;
                   rn = avl_next (rn))
                if ((vlan = rn->info) && (vlan->vid != NSM_VLAN_DEFAULT_VID))
                  {
                    cli_out (cli," Default vlan %d customer-backbone \n" , vlan->vid);
                  }
                cli_out (cli, "CBP configuration for %s bridge \n", brname);
                cli_out (cli, "========================================================\n");
                cli_out (cli, "Interface    BVID    Instance    status\n");
                cli_out (cli, "========================================================\n");

                for (node = avl_top(br->master->beb->bcomp->cbp_srvinst_list);
                     node; node = avl_next (node))
                  {
                    if ((cbp_node = (struct cbp_srvinst_tbl *)
                        AVL_NODE_INFO (node)) == NULL)
                      continue;
                    ifptemp = if_lookup_by_index (&br->master->nm->vr->ifm,
                                                  cbp_node->key.bcomp_port_num);
                    if (ifptemp == NULL)
                      continue;

                    cli_out (cli, " %s  %d    %u    %s \n", ifptemp->name, 
                             cbp_node->bvid, cbp_node->key.b_sid, 
                             (cbp_node->status == NSM_SERVICE_STATUS_DISPATCHED )?
                              "Dispatched":"Not Dispatched");
                  }
            }
          }
        break;
        case NSM_VLAN_PORT_MODE_PNP:
          {
            cli_out (cli, "No mapping for PNP in BEB bridges \n");
          }
        break;
#endif /*HAVE_B_BEB*/
        default:
        break;
    }
}


void 
nsm_bridge_beb_vlan(struct cli *cli, u_char *name)
{
  struct nsm_bridge_master *master;
  struct nsm_bridge *bridge;
  struct nsm_master *nm;
  struct nsm_vlan *vlan;
  struct avl_node *an;
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_vlan_port *port = NULL;
  struct interface *ifp = NULL;
  struct listnode *ln = NULL;
  struct nsm_if *zif = NULL;
  
  if ((cli == NULL) || (name == NULL))
    return;

  nm = cli->vr->proto;
  if (nm == NULL)
    return;

  master = nsm_bridge_get_master (nm);
  if (master == NULL)
    return;

  bridge = nsm_lookup_bridge_by_name(master, name);
  if (bridge == NULL)
    return;
 
  if (bridge->vlan_table == NULL)
    return;
  for (an = avl_top(bridge->vlan_table); an; an = avl_next (an))
    {
      vlan = an->info;
      if (vlan != NULL)
        {

          cli_out (cli, "%-16s%-8s%-17s%-8s%-31s\n", "Bridge", "VLAN ID", 
                   " Name", "State", "Member ports");
          cli_out (cli, "%s\n", "=============== ======= ================"
                    "=======  ===============================");
          if ((vlan->type == NSM_VLAN_DYNAMIC))
            {
              cli_out (cli, "%-16s%-8d*%-16s%-8s", bridge->name,
                       vlan->vid, vlan->vlan_name,
                      (( vlan->vlan_state == NSM_VLAN_ACTIVE) ? "ACTIVE" :
                      (( vlan->vlan_state == NSM_VLAN_DISABLED) ? "SUSPEND":
                      "INVALID")));
            }
          else  
            {
              cli_out (cli, "%-16s%-8d%-17s%-8s", bridge->name,
                       vlan->vid, vlan->vlan_name,
                      (( vlan->vlan_state == NSM_VLAN_ACTIVE) ? "ACTIVE" :
                      (( vlan->vlan_state == NSM_VLAN_DISABLED) ? "SUSPEND":
                      "INVALID")));
            }
        }
      else 
        continue;

      if ( !(vlan->port_list) )
        return ;

      LIST_LOOP (vlan->port_list, ifp, ln)
        {
          if ( (zif = (struct nsm_if *)(ifp->info)) 
                && (br_port = zif->switchport) )
            { 
              if ( (port = &br_port->vlan_port) )
                {
                  cli_out (cli, "%s", ifp->name);
                }
            }/* if port */
        }    
    } /* end for */

  cli_out(cli, "\n");
     
  for (an = avl_top (bridge->svlan_table); an; an = avl_next (an))
    {
      vlan = an->info;
                  
      if (vlan != NULL)
        {
          cli_out (cli, "\n");
          cli_out (cli, "Service VLANs\n");
        }
      cli_out (cli, "%-16s%-8s%-17s%-8s%-31s\n", "Bridge", "VLAN ID", 
               " Name", "State", "Member ports");
      cli_out (cli, "%s\n", "=============== ======= ================ =======" 
               "===============================");
      if (vlan == NULL)
        continue;

      if ((vlan->type == NSM_VLAN_DYNAMIC))
        {
          cli_out (cli, "%-16s%-8d*%-16s%-8s", bridge->name,
                   vlan->vid, vlan->vlan_name,
                   (( vlan->vlan_state == NSM_VLAN_ACTIVE) ? "ACTIVE" :
                   (( vlan->vlan_state == NSM_VLAN_DISABLED) ? "SUSPEND":
                    "INVALID")));
        }
      else  
        {
          cli_out (cli, "%-16s%-8d%-17s%-8s", bridge->name,
                   vlan->vid, vlan->vlan_name,
                   (( vlan->vlan_state == NSM_VLAN_ACTIVE) ? "ACTIVE" :
                   (( vlan->vlan_state == NSM_VLAN_DISABLED) ? "SUSPEND":
                    "INVALID")));
        }
      if ( !(vlan->port_list) )
        return ;

      LIST_LOOP (vlan->port_list, ifp, ln)
        {
          if ( (zif = (struct nsm_if *)(ifp->info)) 
              && (br_port = zif->switchport) )
            { 
              if ( (port = &br_port->vlan_port) )
                {
                  cli_out (cli, "%s", ifp->name);
                }
            }/* if port */
        }    
    }
  cli_out (cli, "\n");
  
  if (bridge->backbone_edge == PAL_TRUE)
#ifdef HAVE_B_BEB    
  for (an = avl_top (bridge->bvlan_table); an; an = avl_next (an))
    {
      vlan = an->info;
      if (vlan != NULL)
        {
          cli_out (cli, "\n");
          cli_out (cli, "Backbone VLANs\n");
        }
      cli_out (cli, "%-16s%-8s%-17s%-8s%-31s\n", "Bridge", "VLAN ID", 
               " Name", "State", "Member ports");
      cli_out (cli, "%s\n", "=============== ======= ================" 
               "======= ===============================");
      if (vlan == NULL)
        continue;

      if ((vlan->type == NSM_VLAN_DYNAMIC))
        {
          cli_out (cli, "%-16s%-8d*%-16s%-8s", bridge->name,
                   vlan->vid, vlan->vlan_name,
                   (( vlan->vlan_state == NSM_VLAN_ACTIVE) ? "ACTIVE" :
                   (( vlan->vlan_state == NSM_VLAN_DISABLED) ? "SUSPEND":
                    "INVALID")));
        } 
      else  
        {
          cli_out (cli, "%-16s%-8d%-17s%-8s", bridge->name,
                   vlan->vid, vlan->vlan_name,
                   (( vlan->vlan_state == NSM_VLAN_ACTIVE) ? "ACTIVE" :
                   (( vlan->vlan_state == NSM_VLAN_DISABLED) ? "SUSPEND":
                    "INVALID")));
        }
      if ( !(vlan->port_list) )
        return ;
      LIST_LOOP (vlan->port_list, ifp, ln)
        {
          if ( (zif = (struct nsm_if *)(ifp->info)) 
              && (br_port = zif->switchport) )
            { 
              if ( (port = &br_port->vlan_port) )
                {
                  cli_out (cli, "%s", ifp->name);
                }
            }/* if port */
        }    
    }
#endif    
  cli_out (cli, "\n");
}

void  
nsm_bridge_beb_service_show(struct cli *cli, u_char *instance_name, u_int32_t isid)
{
  struct avl_node *node = NULL;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master = nsm_bridge_get_master (nm);
#ifdef HAVE_I_BEB
  struct vip_tbl *vip_node = NULL;

  for (node = avl_top(master->beb->icomp->vip_table_list); node; 
       node = avl_next (node))
    {
      vip_node = (struct vip_tbl *)AVL_NODE_INFO (node);
      if (vip_node &&  ((vip_node->vip_sid == isid)||
                       (instance_name && 
                        (!pal_strncmp(instance_name,vip_node->srv_inst_name,
                                      NSM_PBB_ISID_NAME_SIZ)))))
        break;
    }
  if (node && vip_node)
    if (vip_node->status == NSM_SERVICE_STATUS_DISPATCHED)
      {
        cli_out (cli, "intance %d dispatched \n ", vip_node->vip_sid);
      }
    else
      {
        cli_out (cli, "intance %d not dispatched \n ", vip_node->vip_sid);
      }
  else
    cli_out (cli, "Instance not found \n");
#endif
#ifndef HAVE_I_BEB
  struct cbp_srvinst_tbl *cbp_node = NULL;
  for (node =avl_top(master->beb->bcomp->cbp_srvinst_list); node;
       node = avl_next (node))
    {
      cbp_node = (struct cbp_srvinst_tbl *)AVL_NODE_INFO (node);
      if (cbp_node &&  (cbp_node->key.b_sid == isid))
          break;
    }
  if (node && cbp_node) 
    if (cbp_node->status == NSM_SERVICE_STATUS_DISPATCHED)
      {
        cli_out (cli, "intance %d dispatched \n ", cbp_node->key.b_sid);
      }
    else
      {
        cli_out (cli, "intance %d not dispatched \n ",
                 cbp_node->key.b_sid);
      }
    else
      cli_out (cli, "Instance not found \n");
#endif
}

#ifdef HAVE_PBB_DEBUG
/* Debug command to help view the internal tables */

void
nsm_bridge_pbb_list_tables_debug (struct cli *cli, char *brname, u_int8_t table_type)
{
  struct avl_node *node;
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_bridge_master *master;
#ifdef HAVE_I_BEB
  struct cnp_srv_tbl *cnp_node;
  struct vip_tbl *vip_node;
  struct pip_tbl *pip_node;
  struct sid2vip_xref *s2v_node;
  struct vip2pip_map *v2p_node;
  struct svlan_bundle *svlan_bdl;
  struct listnode *svlan_node,*pip_lnode;
#endif
#ifdef HAVE_B_BEB
  struct cbp_srvinst_tbl *cbp_node;
#endif
  master = nsm_bridge_get_master (nm);
  switch(table_type)
    {
#ifdef HAVE_I_BEB
      case 1 : /* All CNP's */
        {
          cli_out (cli, "Printing All CNP Nodes \n");
          cli_out (cli, "========================================================\n");
          cli_out (cli, "sid icompid port_num service_type srv_stat ref_port svlans\n");
          cli_out (cli, "========================================================\n");
          for (node = avl_top(master->beb->icomp->cnp_srv_tbl_list);
               node; node = avl_next (node))
            {
              if ((cnp_node = (struct cnp_srv_tbl *)
                  AVL_NODE_INFO (node)) == NULL)
                continue;
              cli_out(cli, "%u    %d        %d        %d          %d        %d ",
              cnp_node->key.sid, cnp_node->key.icomp_id, cnp_node->key.icomp_port_num,
              cnp_node->srv_type, cnp_node->srv_status, cnp_node->ref_port_num);
         
              if (listcount (cnp_node->svlan_bundle_list) > 0)
                LIST_LOOP ( cnp_node->svlan_bundle_list, svlan_bdl, svlan_node)
                  {
                    cli_out (cli," %d to %d,  ", svlan_bdl->svid_low,
                             svlan_bdl->svid_high);
                  }
              cli_out (cli, "\n");
    
              /* API to get instance name from ISID */
            }
        }
      break;
      case 2 :
        {
          cli_out (cli, "Printing All VIP Nodes \n");
          cli_out (cli, "========================================================\n");
          cli_out (cli, "icompid port_num ISID ISIDName def-DBMAC status\n");
          cli_out (cli, "========================================================\n");

          for (node =avl_top(master->beb->icomp->vip_table_list); node;
               node = avl_next (node))
            {
              if ((vip_node = (struct vip_tbl *)AVL_NODE_INFO (node)) == NULL)
                continue;
              cli_out (cli, "%d       %d        %u         %s ", vip_node->key.icomp_id,
                       vip_node->key.vip_port_num, vip_node->vip_sid,
                       vip_node->srv_inst_name);
              cli_out (cli, "  %.04hx.%.04hx.%.04hx  ",
                       pal_ntoh16(((unsigned short *)vip_node->default_dst_bmac)[0]),
                       pal_ntoh16(((unsigned short *)vip_node->default_dst_bmac)[1]),
                       pal_ntoh16(((unsigned short *)vip_node->default_dst_bmac)[2]));

              cli_out (cli,"%d \n",vip_node->status);
            }
        }
      break;
      case 3 :
        {
          cli_out (cli, "Printing All PIP Nodes \n");
          cli_out (cli, "========================================================\n");
          cli_out (cli, "icompid port_num port_name \n");
          cli_out (cli, "========================================================\n");

          LIST_LOOP (master->beb->icomp->pip_tbl_list, pip_node, pip_lnode)
            {
      
              cli_out (cli, "  %d        %d          %s  \n", pip_node->key.icomp_id,
                       pip_node->key.pip_port_num, pip_node->pip_port_name);
            }
        } 
      break;
      case 4 :
        {
          cli_out (cli, "Printing All sid2vip_xref Nodes \n");
          cli_out (cli, "========================================================\n");
          cli_out (cli, "icompid ISID vip_port_num \n");
          cli_out (cli, "========================================================\n");

          for (node =avl_top(master->beb->icomp->sid2vip_xref_list); node;
               node = avl_next (node))
            {
              if ((s2v_node = (struct sid2vip_xref *)
                  AVL_NODE_INFO (node)) == NULL)
                continue;
              cli_out (cli, " %d      %u       %d  \n", s2v_node->key.icomp_id,
                       s2v_node->key.vip_sid, s2v_node->vip_port_num);

            }
        }
      break;
      case 5 :
        {
          cli_out (cli, "Printing All vip2pip_map Nodes \n");
          cli_out (cli, "========================================================\n");
          cli_out (cli, "icompid vip_port_num pip_port status \n");
          cli_out (cli, "========================================================\n");

          for (node =avl_top(master->beb->icomp->vip2pip_map_list); node;
               node = avl_next (node))
            {
              if ((v2p_node = (struct vip2pip_map *)
                  AVL_NODE_INFO (node)) == NULL)
                continue;
              cli_out (cli, "%d      %d      %d       %d \n", v2p_node->key.icomp_id,
                       v2p_node->key.vip_port_num, v2p_node->pip_port_number,
                       v2p_node->status);

            }

        }
      break;
#endif /*HAVE_I_BEB*/
#ifdef HAVE_B_BEB
      case 6 :
        {
          cli_out (cli, "CBP configurations \n");
          cli_out (cli, "========================================================\n");
          cli_out (cli, "bcompid portnum BVID  Instance  status\n");
          cli_out (cli, "========================================================\n");

          for (node = avl_top(master->beb->bcomp->cbp_srvinst_list);
               node; node = avl_next (node))
            {
              if ((cbp_node = (struct cbp_srvinst_tbl *)
                   AVL_NODE_INFO (node)) == NULL)
                continue;
              cli_out (cli, " %d    %d    %d      %u        %s \n",
                       cbp_node->key.bcomp_id ,cbp_node->key.bcomp_port_num ,
                       cbp_node->bvid , cbp_node->key.b_sid, 
                       (cbp_node->status == NSM_SERVICE_STATUS_DISPATCHED )? 
                        "Dispatched":"Not Dispatched");
            }

        }
      break;
#endif /*HAVE_B_BEB*/
      default :
      break;
         
    }
}
#endif /*HAVE_PBB_DEBUG*/

u_int32_t
nsm_pbb_if_config_exists(struct interface *ifp, enum nsm_vlan_port_mode mode)
{

  struct nsm_if *zif = NULL;
  struct nsm_bridge *bridge = NULL;
  struct avl_node *node = NULL;

#ifdef HAVE_I_BEB
  struct nsm_pbb_icomponent *icomp = NULL;
#endif

#ifdef HAVE_B_BEB
  struct nsm_pbb_bcomponent *bcomp = NULL;
#endif
  if (ifp)
    zif = (struct nsm_if *)ifp->info;

  if (!zif || !zif->switchport ||!zif->bridge)
    return NSM_VLAN_ERR_IFP_NOT_BOUND;

  if (ifp->hw_type == IF_TYPE_PBB_LOGICAL)
    return 0;

  bridge = zif->bridge;

  switch (mode)
    {
#ifdef HAVE_I_BEB
       case NSM_VLAN_PORT_MODE_CNP:
         {
            struct cnp_srv_tbl *cnp_node = NULL;
            icomp = bridge->master->beb->icomp;
            for (node = avl_top(icomp->cnp_srv_tbl_list);
                 node; node = avl_next (node))
              {
                if ((cnp_node = (struct cnp_srv_tbl *)
                    AVL_NODE_INFO (node)) == NULL)
                  continue;
                if ( cnp_node->key.icomp_port_num == ifp->ifindex)
                  {
                    return NSM_VLAN_ERR_PBB_CONFIG_EXIST;
                  }
              }
         }
       break;
       case NSM_VLAN_PORT_MODE_PIP:
         {
           struct pip_tbl *pip_node =NULL;
           icomp = bridge->master->beb->icomp;
           pip_node = nsm_pbb_search_pip_node(icomp->pip_tbl_list, bridge->bridge_id, ifp->ifindex);
           if (pip_node)
              return NSM_VLAN_ERR_PBB_CONFIG_EXIST;
         }
       break;
#endif/*HAVE_I_BEB*/
     
#ifdef HAVE_B_BEB
       case NSM_VLAN_PORT_MODE_CBP:
         {
           struct cbp_srvinst_tbl *cbp_node = NULL;
           bcomp = bridge->master->beb->bcomp;

           for (node = avl_top(bcomp->cbp_srvinst_list);
                node; node = avl_next (node))
             {
                if ((cbp_node = (struct cbp_srvinst_tbl *)
                    AVL_NODE_INFO (node)) == NULL)
                  continue;
                if (cbp_node->key.bcomp_port_num == ifp->ifindex)
                    return NSM_VLAN_ERR_PBB_CONFIG_EXIST;
             }
         }
       break;
#endif /*HAVE_B_BEB*/
       default:
         return 0;
       break;
    }
  return 0;
}

u_int32_t 
nsm_pbb_if_vlan_config_exists (struct nsm_bridge* bridge, nsm_vid_t vid)
{
  struct avl_node *node = NULL;
#ifdef HAVE_B_BEB 
  struct cbp_srvinst_tbl *cbp_node=NULL;
  struct nsm_pbb_bcomponent *bcomp = NULL;
#endif /* HAVE_B_BEB */
  
#ifdef HAVE_I_BEB 
  struct cnp_srv_tbl *cnp_node = NULL;
  struct svlan_bundle *svlan_bdl;
  struct listnode *svlan_node;
  struct nsm_pbb_icomponent *icomp = NULL;
#endif /* HAVE_I_BEB */
  
#ifdef HAVE_B_BEB
  if (NSM_BRIDGE_TYPE_BACKBONE(bridge))
    {
      bcomp =  bridge->master->beb->bcomp;

      if (bcomp == NULL)
        return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;

      AVL_TREE_LOOP (bcomp->cbp_srvinst_list, cbp_node, node)
        {
          if (vid == cbp_node->bvid)
            return NSM_VLAN_ERR_PBB_CONFIG_EXIST;
        }
    }/*  if (NSM_BRIDGE_TYPE_BACKBONE(bridge)) */     
#endif /* HAVE_B_BEB */
  
#ifdef HAVE_I_BEB
  if (bridge->type == NSM_BRIDGE_TYPE_PROVIDER_MSTP 
      || bridge->type == NSM_BRIDGE_TYPE_PROVIDER_RSTP)
    {
      /* I-COMP */
      icomp = bridge->master->beb->icomp;

      if (icomp == NULL)
        {
          return NSM_VLAN_ERR_BRIDGE_NOT_FOUND;
        }
      AVL_TREE_LOOP (icomp->cnp_srv_tbl_list, cnp_node, node)
        {
          if (bridge->bridge_id == cnp_node->key.icomp_id )
            {
              LIST_LOOP (cnp_node->svlan_bundle_list, svlan_bdl, svlan_node)
                {
                  if ((vid >= svlan_bdl->svid_low) &&
                      (vid <= svlan_bdl->svid_high))
                    return NSM_VLAN_ERR_PBB_CONFIG_EXIST;
                }
            }
        }
    }
#endif /* HAVE_I_BEB */
  return 0;
}
#endif

