/* Copyright (C) 2008  All Rights Reserved */

#include "pal.h"
#include "lib.h"
#include "memory.h"

#ifdef HAVE_SNMP
#include "nsm_pbb_mib.h"
#if defined (HAVE_I_BEB) || defined (HAVE_B_BEB)
#include "nsm_bridge_pbb.h"

#ifdef HAVE_VLAN
#include "if.h"
#include "log.h"
#include "prefix.h"
#include "avl_tree.h"
#include "table.h"
#include "ptree.h"
#include "snmp.h"
#include "l2_snmp.h"
#include "pal_math.h"
#include "nsmd.h"
#include "nsm_snmp_common.h"
#include "nsm_snmp.h"
#include "nsm_L2_snmp.h"
#include "nsm_bridge.h"
#include "nsmd.h"
#include "table.h"
#ifdef HAVE_HAL
#include "hal_incl.h"
#endif/*HAVE_HAL */

#ifdef HAVE_VLAN_CLASS
#include "nsm_vlanclassifier.h"
#endif/*HAVE_VLAN_CLASS*/

/* 
 * PBB MIB's objects are in complaince with the standard 802-1ap-d3-2 section 17.5.8
 */

/*
 * This is the root OID we use to register the PBB objects handled in this  module.
 */
oid pbb_oid [] = {BRIDGEMIBOID};
oid pbb_nsm_oid [] = {BRIDGEMIBOID, 12};

struct variable pbb_dot1ahBridge_variables[] ={

/*
 *  Edge Bridge OID's
 */
{IEEE8021PBBBACKBONEEDGEBRIDGEADDRESS,  ASN_OCTET_STR,  RONLY,
 pbb_snmp_ahBridge_scalars,  4,  {1,1,1,1}},
{IEEE8021PBBBACKBONEEDGEBRIDGENAME,	ASN_OCTET_STR,  RWRITE,
 pbb_snmp_ahBridge_scalars,  4,  {1,1,1,2}},
{IEEE8021PBBNUMBEROFICOMPONENTS, 	ASN_INTEGER,    RONLY,
 pbb_snmp_ahBridge_scalars,  4,  {1,1,1,3}},
{IEEE8021PBBNUMBEROFBCOMPONENTS, 	ASN_INTEGER,    RONLY,  
 pbb_snmp_ahBridge_scalars,  4,  {1,1,1,4}},
{IEEE8021PBBNUMBEROFBEBPORTS, 		ASN_INTEGER,    RONLY,   
 pbb_snmp_ahBridge_scalars,  4,  {1,1,1,5}},
{IEEE8021PBBBEBNAME, 			ASN_OCTET_STR,  RWRITE,
 pbb_snmp_ahBridge_scalars,  4,  {1,1,1,6}}, 

/*
 * VIP OID's
 */
#if defined HAVE_I_BEB
/*IEEE8021PBBVIPTABLE {1,1,2}
  IEEE8021PBBVIPENTRY {1,1,2,1}*/
{IEEE8021PBBVIPPAPNFINDEX,      ASN_INTEGER,    RCREATE,    
 pbb_snmp_ahVip_table,  5,  {1,1,2,1,1}}, 
{IEEE8021PBBVAPNSID,            ASN_INTEGER,    RCREATE,   
 pbb_snmp_ahVip_table,  5,  {1,1,2,1,2}}, 
{IEEE8021PBBVIPDEFAULTDSTBMAC,  ASN_OCTET_STR,  RONLY,     
 pbb_snmp_ahVip_table,  5,  {1,1,2,1,3}}, 
{IEEE8021PBBVIPTYPE,            ASN_INTEGER,    RCREATE,   
 pbb_snmp_ahVip_table,  5,  {1,1,2,1,4}}, 
{IEEE8021PBBVIPROWSTATUS,       ASN_INTEGER,	RCREATE,   
 pbb_snmp_ahVip_table,  5,  {1,1,2,1,5}}, 

/*
 * I-SID to VIP Cross reference OID's
 */
/*IEEE8021PBBISIDTOVIPTABLE {1,1,3}
  IEEE8021PBBISIDTOVIPENTRY {1,1,3,1}*/
{IEEE8021PBBISIDTOVAPNSID,         ASN_INTEGER,    NOACCESS,
 pbb_snmp_ahIsid_Vip,  5,  {1,1,3,1,1}}, 
{IEEE8021PBBISIDTOVIPCOMPONENTID,  ASN_INTEGER,    RONLY,
 pbb_snmp_ahIsid_Vip,  5,  {1,1,3,1,2}}, 
{IEEE8021PBBISIDTOVIPPORT,         ASN_INTEGER,    RONLY,
 pbb_snmp_ahIsid_Vip, 5,  {1,1,3,1,3}}, 

/*
 * PIP OID's
 */
/*IEEE8021PBBPIPTABLE {1,1,4}
  IEEE8021PBBPIPENTRY {1,1,4,1}*/ 
{IEEE8021PBBPAPNFINDEX,       ASN_INTEGER,    NOACCESS,
 pbb_snmp_ahPip_table,  5,  {1,1,4,1,1}}, 
{IEEE8021PBBPIPBMACADDRESS,   ASN_OCTET_STR,  RONLY,
 pbb_snmp_ahPip_table,  5,  {1,1,4,1,2}}, 
{IEEE8021PBBPIPNAME,          ASN_OCTET_STR,  RWRITE,
 pbb_snmp_ahPip_table,  5,  {1,1,4,1,3}}, 
{IEEE8021PBBPAPNCOMPONENTID,  ASN_INTEGER,    RONLY,
 pbb_snmp_ahPip_table,  5,  {1,1,4,1,4}}, 
{IEEE8021PBBPIPVIPMAP,        ASN_OCTET_STR,  RONLY,
 pbb_snmp_ahPip_table,  5,  {1,1,4,1,5}}, 

/*
 *  VIP-PIP-Mapping OID's 
 */
/*IEEE8021PBBVIPTOPIPMAPPINGTABLE  {1,1,5}
  IEEE8021PBBVIPTOPIPMAPPINGENTRY  {1,1,5,1}*/
{IEEE8021PBBVIPTOPIPMAPPINGPAPNFINDEX,   ASN_INTEGER,    RCREATE,
 pbb_snmp_ahVip_Pip_map,  5,  {1,1,5,1,1}}, 
{IEEE8021PBBVIPTOPIPMAPPINGBMACADDRESS,  ASN_OCTET_STR,  RONLY,
 pbb_snmp_ahVip_Pip_map,  5,  {1,1,5,1,2}}, 
{IEEE8021PBBVIPTOPIPMAPPINGSTORAGETYPE,  ASN_OCTET_STR,  RONLY,
 pbb_snmp_ahVip_Pip_map,  5,  {1,1,5,1,3}}, 
#endif /*HAVE_I_BEB*/

/*
 * Service Instance CBP OID's 
 */
#if defined (HAVE_B_BEB)
/*IEEE8021PBBCBPSERVICEMAPPINGTABLE {1,1,6}} 
  IEEE8021PBBCBPSERVICEMAPPINGENTRY {1,1,6,1}*/
{IEEE8021PBBCBPSERVICEMAPPINGBACKBONESID, 	   ASN_INTEGER,    NOACCESS,
 pbb_snmp_ahService_table,  5,  {1,1,6,1,1}}, 
{IEEE8021PBBCBPSERVICEMAPPINGBVID,                 ASN_INTEGER,    RWRITE,
 pbb_snmp_ahService_table,  5,  {1,1,6,1,2}}, 
{IEEE8021PBBCBPSERVICEMAPPINGDEFAULTBACKBONEDEST,  ASN_OCTET_STR,  RWRITE,
 pbb_snmp_ahService_table,  5,  {1,1,6,1,3}}, 
{IEEE8021PBBCBPSERVICEMAPPINGTYPE, 		   ASN_INTEGER,    RWRITE,
 pbb_snmp_ahService_table,  5,  {1,1,6,1,4}}, 
{IEEE8021PBBCBPSERVICEMAPPINGLOCALSID, 		   ASN_INTEGER,    RWRITE,
 pbb_snmp_ahService_table,  5,  {1,1,6,1,5}}, 
#endif /*HAVE_B_BEB*/

/*
 * CNP OID's These OID's are PacOS specific and are not specified in standard 
 */
#if defined HAVE_I_BEB
/*IEEE8021PBBCNPTABLE {1,1,7}
  IEEE8021PBBCNPENTRY  {1,1,7,1}*/
{IEEE8021PBBCNPSERVICEINTERFACETYPE,  ASN_INTEGER,  RCREATE,
 pbb_snmp_ahCnp_table,  5,  {1,1,7,1,1}},
{IEEE8021PBBCNPSERVIREFPORTNUMBER,    ASN_INTEGER,  RCREATE,
 pbb_snmp_ahCnp_table,  5,  {1,1,7,1,2}},
{IEEE8021PBBCNPSERVICESVIDLOW,        ASN_INTEGER,  RCREATE,
 pbb_snmp_ahCnp_table,  5,  {1,1,7,1,3}},
{IEEE8021PBBCNPSERVICESVIDHIGH,       ASN_INTEGER,  RCREATE,
 pbb_snmp_ahCnp_table,  5,  {1,1,7,1,4}},
{IEEE8021PBBCNPSERVICEPORTTYPE,       ASN_INTEGER,  RCREATE,
 pbb_snmp_ahCnp_table,  5,  {1,1,7,1,5}},
{IEEE8021PBBCNPROWSTATUS,             ASN_INTEGER,  RCREATE,
 pbb_snmp_ahCnp_table,  5,  {1,1,7,1,6}}
#endif /*HAVE_I_BEB*/
}; 

void
pbb_ah_snmp_init (struct lib_globals *zg) 
{
   /*
   * register ourselves with the agent to handle our mib tree objects
   */
   REGISTER_MIB (zg, "PBBMIB", pbb_dot1ahBridge_variables, variable, pbb_nsm_oid);
}

/*
 * API for getting three integer indexes from the OID .
 */
int
pbb_snmp_integer_index3_get (struct variable *v, oid *name, size_t *length,
                             u_int32_t *index1, u_int32_t *index2,
                             u_int32_t *index3, int exact)
{
   int result, len;

   *index1 = 0;
   *index2 = 0;
   *index3 = 0;

   result = oid_compare (name, v->namelen, v->name, v->namelen);
   if (result != 0 )
     return MATCH_FAILED;
 
   if (exact)
     {     
            /* Check the length. */
        if (*length - v->namelen != 3)
        return MATCH_FAILED; 
        *index1 = name[v->namelen];
        *index2 = name[v->namelen+1];
        *index3 = name[v->namelen+2];
        return SNMP_ERR_NOERROR;
     }
   else
     {
        if (result == 0)
          {
             /* Use index value if present, otherwise 0. */
             len = *length - v->namelen;
             if (len >= 3)
               {
                  *index1 = name[v->namelen];
                  *index2 = name[v->namelen+1];
	          *index3 = name[v->namelen+2];
               }
             return SNMP_ERR_NOERROR;
          }
        else
          {
             /* Set the user's oid to be ours */
             pal_mem_cpy (name, v->name, ((int) v->namelen) * sizeof (oid));
             return SNMP_ERR_NOERROR;

          }
     }
}

/* API for setting three INTEGER indices. */
#if 0
void
l2_snmp_integer_index3_set (struct variable *v, oid * name, size_t * length,
                            u_int32_t index1, u_int32_t index2,
                            u_int32_t index3)
{
   name[v->namelen] = index1;
   name[v->namelen+1] = index2;
   name[v->namelen+2] = index3;
   *length = v->namelen + 3;
}
#endif /* 0 */
/*
 * API for adding VIP entry to VIP table.
 */
#if defined HAVE_I_BEB
int
pbb_snmp_vip_entry_add(struct avl_tree *tree_vip, struct avl_tree *tree_v2p,
                       u_int32_t icomp_id, u_int32_t base_port)
{
   struct vip_tbl *vip = NULL;
   struct vip2pip_map *vip_pip = NULL;

   vip = XCALLOC(MTYPE_NSM_BRIDGE_PBB_VIP_NODE, sizeof(struct vip_tbl));
   if (! vip)
     return SNMP_ERR_GENERR;

   pal_mem_set(vip, 0, sizeof(struct vip_tbl));

   vip_pip = XCALLOC(MTYPE_NSM_BRIDGE_PBB_VIP2PIP_NODE, sizeof(struct vip2pip_map));
   if (! vip_pip)
     return SNMP_ERR_GENERR;

   pal_mem_set(vip_pip, 0, sizeof(struct vip2pip_map));

   vip->key.icomp_id = icomp_id;
   vip->key.vip_port_num = base_port;
   vip->rowstatus = PBB_SNMP_ROW_STATUS_NOTREADY;

   vip_pip->key.icomp_id = icomp_id;
   vip_pip->key.vip_port_num = base_port;
   pal_strncpy (vip_pip->storage_type,"PERMANENT",32);

   avl_insert (tree_vip, (void *)vip);
   avl_insert (tree_v2p, (void *)vip_pip);
   return SNMP_ERR_NOERROR;
}

/*
 * API for adding Sid-Vip entry to Sid-Vip table.
 */
int
pbb_snmp_sid_vip_entry_add (struct avl_tree *tree,u_int32_t icomp_id,
                            u_int32_t vip_isid,  u_int32_t port_num )
{
   struct sid2vip_xref *sid_vip = NULL;

   sid_vip = XCALLOC(MTYPE_NSM_BRIDGE_PBB_SID2VIP_NODE,
                     sizeof(struct sid2vip_xref));
   if (! sid_vip)
     return SNMP_ERR_GENERR;

   pal_mem_set(sid_vip, 0, sizeof(struct sid2vip_xref));
   sid_vip->key.icomp_id = icomp_id;
   sid_vip->key.vip_sid = vip_isid;
   sid_vip->vip_port_num = port_num; 

   avl_insert (tree, (void *)sid_vip);
   return SNMP_ERR_NOERROR;
}

/*
 * API for adding CNP entry to CNP.
 */
int
pbb_snmp_cnp_entry_add(struct avl_tree *tree, u_int32_t icomp_id, u_int32_t isid,
                       u_int32_t base_port)
{
   struct cnp_srv_tbl *cnp = NULL;
   struct svlan_bundle *svlan_b;

   cnp = XCALLOC(MTYPE_NSM_BRIDGE_PBB_CNP_NODE, sizeof(struct cnp_srv_tbl));
   if (! cnp)
     return SNMP_ERR_GENERR;

   pal_mem_set(cnp, 0, sizeof(struct cnp_srv_tbl));
   cnp->key.icomp_id = icomp_id;
   cnp->key.icomp_port_num = base_port;
   cnp->key.sid = isid;
   cnp->rowstatus =  PBB_SNMP_ROW_STATUS_NOTREADY;

   if (cnp->svlan_bundle_list == NULL)
     cnp->svlan_bundle_list = list_new();
     
   svlan_b = XCALLOC(MTYPE_NSM_BRIDGE_PBB_SVLAN_BUNDLE,
                    sizeof(struct svlan_bundle));

   listnode_add(cnp->svlan_bundle_list, (void*)svlan_b);

   if (! svlan_b)
     return SNMP_ERR_GENERR ;

   avl_insert (tree, (void *)cnp);  
   return SNMP_ERR_NOERROR;
}

/*
 * VIP table lookup for getting the VIP tabel entries
 */
struct vip_tbl*
pbb_snmp_vip_lookup_by_portnum (struct avl_tree *tree, u_int32_t port_num,
                                int exact)
{
   struct avl_node *avl_port = NULL;
   struct vip_tbl *vip = NULL;
   struct vip_tbl *best_vip = NULL;

   for (avl_port = avl_top (tree) ;
        avl_port; avl_port = avl_next (avl_port))
     {
        if ((vip = (struct vip_tbl *)
             AVL_NODE_INFO (avl_port)) == NULL)
          continue;

        if (exact)
          {
             if (vip->key.vip_port_num == port_num)
               {
                  /* GET request and instance match */
                  best_vip = vip;
                  break;
               }
          }
        else  /* GETNEXT request */
          {
             if (port_num < vip->key.vip_port_num)
               {
                  /* Save if this is a better candidate and continue search */
                  if (best_vip == NULL)
                    best_vip = vip;
                  else if (vip->key.vip_port_num < best_vip->key.vip_port_num)
                    best_vip = vip;
               }
          }
     }
   return  best_vip;
}

/* 
 *SID_VIP table lookup for getting the SID_VIP tabel entries
 */
struct sid2vip_xref *
pbb_snmp_sid_vip_lookup_by_isid (struct avl_tree *tree, u_int32_t isid,
                                 int exact)
{
   struct avl_node *avl_port = NULL;
   struct sid2vip_xref *sid_vip = NULL;
   struct sid2vip_xref *best_sid_vip = NULL;

   for (avl_port = avl_top (tree) ;
        avl_port; avl_port = avl_next (avl_port))
     {
        if ((sid_vip = (struct sid2vip_xref *)
             AVL_NODE_INFO (avl_port)) == NULL)
          continue;

        if (exact)
          {
             if (sid_vip->key.vip_sid == isid)
               {
                  /* GET request and instance match */
                  best_sid_vip = sid_vip;
                  break;
               }
          }
        else  /* GETNEXT request */
          {
             if (isid < sid_vip->key.vip_sid)
               {
                  /* Save if this is a better candidate and continue search */
                  if (best_sid_vip == NULL)
                    best_sid_vip = sid_vip;
                  else if (sid_vip->key.vip_sid < best_sid_vip->key.vip_sid)
                    best_sid_vip = sid_vip;
               }
          }
     }

   return  best_sid_vip;
}

/*
 * PIP table lookup for getting the PIP tabel entries
*/
struct pip_tbl *
pbb_snmp_pip_lookup_by_ifindex (struct list *list, u_int32_t ifindex,
                                int exact)
{
   struct pip_tbl *pip = NULL;
   struct pip_tbl *best_pip = NULL;
   struct listnode *node;
   void *data;

   for (node = list->head; node; NEXTNODE (node))
     {
        data = GETDATA (node);
        pip = (struct pip_tbl*)data;

        if (exact)
          {
             if (pip->key.pip_port_num == ifindex )
               {
                  /* GET request and instance match */
                  best_pip = pip;
                  break;
               }
          }
        else  /* GETNEXT request */
          {
             if (ifindex < pip->key.pip_port_num)
               {
                  /* Save if this is a better candidate and continue search */
                  if (best_pip == NULL)
                    best_pip = pip;
                  else if (pip->key.pip_port_num < best_pip->key.pip_port_num)
                    best_pip = pip;
               }
          }
     }

   return  best_pip;
}

/*
 * VIP_PIP table lookup for getting the VIP_PIP tabel entries
*/
struct vip2pip_map*
pbb_snmp_vip_pip_lookup_by_portnum (struct avl_tree *tree, u_int32_t port_num,
                                    int exact)
{
   struct avl_node *avl_port = NULL;
   struct vip2pip_map *vip_pip = NULL;
   struct vip2pip_map *best_vip_pip = NULL;

   for (avl_port = avl_top (tree) ;
        avl_port; avl_port = avl_next (avl_port))
     {
        if ((vip_pip = (struct vip2pip_map *)
             AVL_NODE_INFO (avl_port)) == NULL)
          continue;

        if (exact)
          {
             if (vip_pip->key.vip_port_num == port_num)
               {
                  /* GET request and instance match */
                  best_vip_pip = vip_pip;
                  break;
               }
          }
        else  /* GETNEXT request */
          {
             if (port_num < vip_pip->key.vip_port_num)
               {
                  /* Save if this is a better candidate and continue search */
                  if (best_vip_pip == NULL)
                    best_vip_pip = vip_pip;
                  else if (vip_pip->key.vip_port_num < best_vip_pip->key.vip_port_num)
                    best_vip_pip = vip_pip;
               }
          }
     }

   return  best_vip_pip;
}

/*
 * CNP table lookup for getting the CNP tabel entries
*/
struct cnp_srv_tbl*
pbb_snmp_cnp_lookup_by_isid (struct avl_tree *tree, u_int32_t isid,
                             int exact)
{
   struct avl_node *avl_port = NULL;
   struct cnp_srv_tbl *cnp = NULL;
   struct cnp_srv_tbl *best_cnp = NULL;

   for (avl_port = avl_top (tree) ;
        avl_port; avl_port = avl_next (avl_port))
     {
        if ((cnp = (struct cnp_srv_tbl *)
             AVL_NODE_INFO (avl_port)) == NULL)
          continue;

        if (exact)
          {
             if (cnp->key.sid == isid)
               {
                 /* GET request and instance match */
                 best_cnp = cnp;
                 break;
               }
          }
        else  /* GETNEXT request */
          {
             if (isid < cnp->key.sid)
               {
                  /* Save if this is a better candidate and continue search */
                  if (best_cnp == NULL)
                    best_cnp = cnp;
                  else if (cnp->key.sid < best_cnp->key.sid)
                    best_cnp = cnp;
               }
          }
     }

   return  best_cnp;
}

#endif /*HAVE_I_BEB*/

#if defined HAVE_B_BEB
struct cbp_srvinst_tbl*
pbb_snmp_cbp_lookup_by_isid (struct avl_tree *tree, u_int32_t b_isid,
                             int exact)
{
   struct avl_node *avl_port = NULL;
   struct cbp_srvinst_tbl *cbp = NULL;
   struct cbp_srvinst_tbl *best_cbp = NULL;

   for (avl_port = avl_top (tree) ;
        avl_port; avl_port = avl_next (avl_port))
     {
        if ((cbp = (struct cbp_srvinst_tbl *)
             AVL_NODE_INFO (avl_port)) == NULL)
          continue;

        if (exact)
          {
             if (cbp->key.b_sid == b_isid)
               {
                  /* GET request and instance match */
                  best_cbp = cbp;
                  break;
               }
          }
        else  /* GETNEXT request */
          {
             if (b_isid < cbp->key.b_sid)
               {
                  /* Save if this is a better candidate and continue search */
                  if (best_cbp == NULL)
                    best_cbp = cbp;
                  else if (cbp->key.b_sid < best_cbp->key.b_sid)
                    best_cbp = cbp;
               }
          }
     }

   return  best_cbp;
}
#endif /*HAVE_B_BEB*/

/*
 * Callback function for reading and writing PBB Bridge OID's
 */
u_char * 
pbb_snmp_ahBridge_scalars (struct variable *vp,
                           oid * name,
                           size_t * length,
                           int exact, size_t * var_len, 
                           WriteMethod ** write_method,
                           u_int32_t vr_id) 
{
   struct apn_vr *vr = NULL;
   struct nsm_master *master = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;

   if (snmp_header_generic(vp, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return NULL;

   vr = apn_vr_get_privileged (snmpg);
   if (! vr)
     return (unsigned char *)SNMP_ERR_GENERR;

   master = vr->proto;
   if (! master)
     return NULL;

   bridge_master = nsm_bridge_get_master (master);
   if (! bridge_master)
     return NULL;

   beb_bridge = bridge_master->beb;
   if (! beb_bridge)
     return NULL;
   
   switch (vp->magic)
   {
      case IEEE8021PBBBACKBONEEDGEBRIDGEADDRESS: /* ROnly */
           XSTP_SNMP_RETURN_MACADDRESS (beb_bridge->beb_mac_addr);

      case IEEE8021PBBBACKBONEEDGEBRIDGENAME:  /* RWRITE */
  	   *write_method = pbb_snmp_write_bridge_name;
           * var_len = strlen(beb_bridge->beb_name);
           return beb_bridge->beb_name;
#ifdef HAVE_I_BEB
      case IEEE8021PBBNUMBEROFICOMPONENTS:   /* ROnly */
           XSTP_SNMP_RETURN_INTEGER64 (beb_bridge->beb_tot_icomp);
#endif /* HAVE_I_BEB */
#ifdef HAVE_B_BEB
      case IEEE8021PBBNUMBEROFBCOMPONENTS:  /* ROnly */
           XSTP_SNMP_RETURN_INTEGER64 (beb_bridge->beb_tot_bcomp);
#endif
      case IEEE8021PBBNUMBEROFBEBPORTS:    /* ROnly */
           XSTP_SNMP_RETURN_INTEGER64 (beb_bridge->beb_tot_ext_ports);

      case IEEE8021PBBBEBNAME:            /* RWRITE */
    	   *write_method = pbb_snmp_write_bridge_name;
           * var_len = strlen(beb_bridge->beb_name);
           return beb_bridge->beb_name;
   }
   return NULL;
}

/*
 * Write method for PBB Bridge name OID's
 */
int
pbb_snmp_write_bridge_name (int action, u_char * var_val,
                            u_char var_val_type, size_t var_val_len,
                            u_char * statP, oid * name, size_t name_len,
                            struct variable *v, u_int32_t vr_id)
{
   struct nsm_master *master = NULL;
   struct apn_vr *vr = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;
   
   vr = apn_vr_get_privileged (snmpg);
   if (! vr)
     return SNMP_ERR_GENERR;

   master = vr->proto;
   if (! master)
     return SNMP_ERR_GENERR;

   bridge_master = nsm_bridge_get_master (master);
   if (! bridge_master)
     return SNMP_ERR_GENERR;

   beb_bridge = bridge_master->beb;
   if (! beb_bridge)
     return SNMP_ERR_GENERR;

   if (var_val_type != ASN_OCTET_STR)
     return SNMP_ERR_WRONGTYPE;

   switch (v->magic)
   {
      case IEEE8021PBBBACKBONEEDGEBRIDGENAME:  /* RWRITE */ 
      case IEEE8021PBBBEBNAME:                 /* RWRITE */
           pal_mem_set (beb_bridge->beb_name, 0, BEB_BRIDGE_NAME);

           if (var_val != NULL)
             pal_mem_cpy ((beb_bridge->beb_name), var_val, BEB_BRIDGE_NAME);

           break;
   }
   return SNMP_ERR_NOERROR;
}

/*
 * Callback function for reading and writing VIP OID's
 */
#if defined HAVE_I_BEB
u_char *
pbb_snmp_ahVip_table (struct variable *vp, oid * name,
                      size_t * length, int exact,
                      size_t * var_len, WriteMethod ** write_method,
                      u_int32_t vr_id)
{
   struct apn_vr *vr = NULL;
   struct nsm_master *master = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;
   struct nsm_pbb_icomponent *pbb_icomp = NULL;
   struct vip_tbl *vip = NULL;
   u_int32_t icomp_id ;
   u_int32_t port_num ;
   int ret;

   /* validate the index */
   ret = l2_snmp_integer_index_get (vp, name, length, &icomp_id, &port_num, exact);
   if (ret < 0)
     return NULL;
  
   if (exact && (icomp_id < PBB_ICOMP_ID_MIN || icomp_id > PBB_ICOMP_ID_MAX))
     return NULL;
   
   vr = apn_vr_get_privileged (snmpg);
   if (! vr)
     return NULL;
  
   master = vr->proto;
   if (! master)
     return NULL;

   bridge_master = nsm_bridge_get_master(master);
   if (! bridge_master)
     return NULL ;

   beb_bridge = bridge_master->beb;
   if (! beb_bridge)
     return NULL;

   pbb_icomp = beb_bridge->icomp;
   if (! pbb_icomp)
     return NULL;

   vip = pbb_snmp_vip_lookup_by_portnum (pbb_icomp->vip_table_list, port_num,
                                         exact);
   if (! exact)
     {
        if (! vip)
          return NULL;
        l2_snmp_integer_index_set (vp, name, length, vip->key.icomp_id,
                                   vip->key.vip_port_num);
     }

   switch (vp->magic)
   {
      case IEEE8021PBBVIPPAPNFINDEX:  /* RCreate */
           *write_method = pbb_snmp_write_vip_table;
           if (! vip)
             return NULL;

	   XSTP_SNMP_RETURN_INTEGER64 (vip->key.vip_port_num);

      case IEEE8021PBBVAPNSID:       /* RCreate */
           *write_method = pbb_snmp_write_vip_table;
           if (! vip)
             return NULL;

           XSTP_SNMP_RETURN_INTEGER64 (vip->vip_sid); 

      case IEEE8021PBBVIPDEFAULTDSTBMAC:  /* ROnly */
	   if (! vip)
             return NULL;

           XSTP_SNMP_RETURN_MACADDRESS (vip->default_dst_bmac); 

      case IEEE8021PBBVIPTYPE:        /* RCreate */
           *write_method = pbb_snmp_write_vip_table;
           if (! vip)
             return NULL;

           XSTP_SNMP_RETURN_INTEGER64 (vip->srv_type);

      case IEEE8021PBBVIPROWSTATUS:   /* RCreate */
           *write_method = pbb_snmp_write_vip_Rowstatus;
	   if (! vip)
             return NULL;

           XSTP_SNMP_RETURN_INTEGER64 (vip->rowstatus);
   }
   return NULL;
}

/*
 *  Write method for VIP ROWSTATUS
 */
int
pbb_snmp_write_vip_Rowstatus (int action, u_char * var_val,
                              u_char var_val_type, size_t var_val_len,
                              u_char * statP, oid * name, size_t name_len,
                              struct variable *v, u_int32_t vr_id)
{
   struct nsm_master *master = NULL;
   struct apn_vr *vr = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;
   struct nsm_pbb_icomponent *pbb_icomp = NULL;
   struct vip_tbl *vip = NULL;
   struct vip2pip_map  *vip_pip = NULL;
   struct sid2vip_xref *sid_vip = NULL;
   struct pip_tbl *pip = NULL;
   long   setval;
   int ret;
   u_int32_t icomp_id = 0;
   u_int32_t port_num = 0;

   /* validate the index */
   ret = l2_snmp_integer_index_get (v, name, &name_len , &icomp_id, &port_num, PAL_TRUE);
   if (ret < 0)
     return SNMP_ERR_GENERR;

   if (var_val_type != ASN_INTEGER)
     return SNMP_ERR_WRONGTYPE;

   if (icomp_id < PBB_ICOMP_ID_MIN || icomp_id > PBB_ICOMP_ID_MAX)
     return SNMP_ERR_BADVALUE;

   pal_mem_cpy(&setval, var_val, SET_VAL_SIZE);

   if ((setval < PBB_SNMP_ROW_STATUS_ACTIVE) ||
       (setval > PBB_SNMP_ROW_STATUS_DESTROY))
      return SNMP_ERR_BADVALUE;

   vr = apn_vr_get_privileged (snmpg);
   if (! vr)
     return SNMP_ERR_GENERR;

   master = vr->proto;
   if (! master)
     return SNMP_ERR_GENERR;

   bridge_master = nsm_bridge_get_master (master);
   if (! bridge_master)
     return SNMP_ERR_GENERR;

   beb_bridge= bridge_master->beb;
   if (! beb_bridge)
     return SNMP_ERR_GENERR;

   pbb_icomp = beb_bridge->icomp;
   if (! pbb_icomp)
     return SNMP_ERR_GENERR;

   vip = nsm_pbb_search_vip_node(pbb_icomp->vip_table_list,
                                 icomp_id , port_num);

   switch (setval)
   {
      case PBB_SNMP_ROW_STATUS_ACTIVE:
           break;

      case PBB_SNMP_ROW_STATUS_CREATEANDGO:
           break;

      case PBB_SNMP_ROW_STATUS_NOTINSERVICE:
           if (! vip)
             return SNMP_ERR_GENERR;

           vip->rowstatus = PBB_SNMP_ROW_STATUS_NOTINSERVICE;
           break;

      case PBB_SNMP_ROW_STATUS_NOTREADY:
           if (! vip)
             return SNMP_ERR_GENERR;

           vip->rowstatus = PBB_SNMP_ROW_STATUS_NOTREADY; 
           break;

      case PBB_SNMP_ROW_STATUS_CREATEANDWAIT:
           if (! vip)
             {
                ret = pbb_snmp_vip_entry_add (pbb_icomp->vip_table_list,
                                              pbb_icomp->vip2pip_map_list,
                                              icomp_id, port_num);
                if (ret != SNMP_ERR_NOERROR)
                  return SNMP_ERR_GENERR;
             }
           break;

      case PBB_SNMP_ROW_STATUS_DESTROY:

           if (! vip)
             return SNMP_ERR_GENERR;

           vip_pip = nsm_pbb_search_vip2pip_node (pbb_icomp->vip2pip_map_list,
                                                  icomp_id, port_num);
           if (vip_pip)
             {
                avl_remove(pbb_icomp->vip2pip_map_list, (void *) vip_pip);
                XFREE(MTYPE_NSM_BRIDGE_PBB_VIP2PIP_NODE, vip_pip);
             }

           sid_vip = nsm_pbb_search_sid2vip_node (pbb_icomp->sid2vip_xref_list,
                                                  icomp_id, vip->vip_sid);
           if (sid_vip)
             {
                avl_remove(pbb_icomp->sid2vip_xref_list, (void *) sid_vip);
                XFREE(MTYPE_NSM_BRIDGE_PBB_SID2VIP_NODE, sid_vip);
             }
           
           pip = nsm_pbb_search_pip_node (pbb_icomp->pip_tbl_list,
                                          icomp_id, port_num);
           if (pip)
             nsm_pbb_unset_pipvip_map(pip->pip_vip_map, vip->key.vip_port_num);

           avl_remove(pbb_icomp->vip_table_list, (void *) vip);
           XFREE(MTYPE_NSM_BRIDGE_PBB_VIP_NODE, vip);
           
           break;
   }
   return SNMP_ERR_NOERROR;
}

/*
 *  Write method for VIP OID's
 */
int
pbb_snmp_write_vip_table (int action, u_char * var_val, u_char var_val_type,
                          size_t var_val_len, u_char * statP, oid * name,
                          size_t name_len, struct variable *v, u_int32_t vr_id)
{
   struct nsm_master *master = NULL;
   struct apn_vr *vr = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;
   struct nsm_pbb_icomponent *pbb_icomp = NULL;
   struct vip_tbl *vip = NULL;
   struct vip2pip_map  *vip_pip = NULL;
   struct sid2vip_xref *sid_vip = NULL;
   struct cnp_srv_tbl *cnp = NULL;
   long   setval;
   int ret;
   u_int32_t icomp_id = 0;
   u_int32_t port_num = 0;

   /* validate the index */
   ret = l2_snmp_integer_index_get (v, name, &name_len, &icomp_id, &port_num, PAL_TRUE);
   if (ret < 0)
     return SNMP_ERR_GENERR;

   if (var_val_type != ASN_INTEGER)
     return SNMP_ERR_WRONGTYPE;

   if (icomp_id < PBB_ICOMP_ID_MIN || icomp_id > PBB_ICOMP_ID_MAX)
     return SNMP_ERR_BADVALUE;

   vr = apn_vr_get_privileged (snmpg);
   if (! vr)
     return SNMP_ERR_GENERR;

   master = vr->proto;
   if (! master)
     return SNMP_ERR_GENERR;

   bridge_master = nsm_bridge_get_master (master);
   if (! bridge_master)
     return SNMP_ERR_GENERR;

   beb_bridge= bridge_master->beb;
   if (! beb_bridge)
     return SNMP_ERR_GENERR;

   pbb_icomp = beb_bridge->icomp;
   if (! pbb_icomp)
     return SNMP_ERR_GENERR;
  
   vip = nsm_pbb_search_vip_node (pbb_icomp->vip_table_list,icomp_id,
                                  port_num); 
   if (! vip)
     return SNMP_ERR_GENERR;

   pal_mem_cpy (&setval, var_val, SET_VAL_SIZE);

   switch (v->magic)
   {
      case IEEE8021PBBVIPPAPNFINDEX:  /* RCreate */
           if ((vip->rowstatus == PBB_SNMP_ROW_STATUS_NOTINSERVICE ||
                vip->rowstatus ==  PBB_SNMP_ROW_STATUS_NOTREADY ) &&
                vip->vip_sid)
	     {
                if (setval < PBB_MIN_VAL)
                  return SNMP_ERR_BADVALUE;

               	vip->key.vip_port_num = setval;  

		sid_vip = nsm_pbb_search_sid2vip_node (pbb_icomp->sid2vip_xref_list,
                                                       icomp_id, vip->vip_sid);
                if (! sid_vip)
                  return SNMP_ERR_GENERR;
                sid_vip->vip_port_num = setval;

                vip_pip = nsm_pbb_search_vip2pip_node (pbb_icomp->vip2pip_map_list,
                                          icomp_id, port_num);  
                if (! vip_pip)
                  return SNMP_ERR_GENERR;
	        vip_pip->key.vip_port_num = setval;
 	     }
           break;

      case IEEE8021PBBVAPNSID:       /* RCreate */
           if (vip->rowstatus == PBB_SNMP_ROW_STATUS_NOTINSERVICE ||
               vip->rowstatus ==  PBB_SNMP_ROW_STATUS_NOTREADY ) 
	     {
                if (setval < PBB_ISID_MIN || setval > PBB_ISID_MAX)
                  return SNMP_ERR_BADVALUE;

                sid_vip = nsm_pbb_search_sid2vip_node (pbb_icomp->sid2vip_xref_list,
                                                       icomp_id, vip->vip_sid);
                cnp = pbb_snmp_cnp_lookup_by_isid (pbb_icomp->cnp_srv_tbl_list,
                                                   vip->vip_sid, PAL_TRUE); 
                vip->vip_sid = setval;
                nsm_pbb_set_vip_default_mac (vip, vip->vip_sid);

                if (! sid_vip)
		  pbb_snmp_sid_vip_entry_add (pbb_icomp->sid2vip_xref_list,
                                              icomp_id, vip->vip_sid,
                                              vip->key.vip_port_num);
                else
                  sid_vip->key.vip_sid = setval;

                if (! cnp)
                  return SNMP_ERR_GENERR;
                else
                  cnp->key.sid = setval;
	     }
           break;

      case IEEE8021PBBVIPTYPE:        /* RCreate */
           if ((vip->rowstatus == PBB_SNMP_ROW_STATUS_NOTINSERVICE ||
                vip->rowstatus ==  PBB_SNMP_ROW_STATUS_NOTREADY ) &&
                vip->vip_sid)
             {
                if (setval < NSM_PORT_SERVICE_BOTH || setval > NSM_PORT_SERVICE_EGRESS)
                  return SNMP_ERR_BADVALUE;
                   
	        vip->srv_type = setval;
             }
           break;
   }

   return SNMP_ERR_NOERROR;
}

/*

 * Callback funcion for reading I-SID to VIP Crossreference OID's
 */
u_char *
pbb_snmp_ahIsid_Vip (struct variable *vp, oid * name, size_t * length,
                     int exact, size_t * var_len, WriteMethod ** write_method,
                     u_int32_t vr_id)
{
   struct apn_vr *vr = NULL;
   struct nsm_master *master = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;
   struct nsm_pbb_icomponent *pbb_icomp = NULL;
   struct sid2vip_xref *sid_vip = NULL;
   int ret;
   u_int32_t vip_sid = 0;

   /* validate the index */
   ret = l2_snmp_index_get (vp, name, length, &vip_sid, exact);
   if (ret < 0)
     return NULL;

   vr = apn_vr_get_privileged (snmpg);
   if (! vr)
     return NULL;

   master = vr->proto;
   if (! master)
     return NULL;

   bridge_master = nsm_bridge_get_master (master);
   if (! bridge_master)
     return NULL ;

   beb_bridge = bridge_master->beb;
   if (! beb_bridge)
     return NULL;

   pbb_icomp = beb_bridge->icomp;
   if (! pbb_icomp)
     return NULL;

   sid_vip = pbb_snmp_sid_vip_lookup_by_isid (pbb_icomp->sid2vip_xref_list,
                                              vip_sid, exact);
   if (! sid_vip )
     return NULL;

   if (! exact)
     l2_snmp_index_set (vp, name, length, sid_vip->key.vip_sid);

   switch(vp->magic)
   {
      case IEEE8021PBBISIDTOVIPCOMPONENTID: /* ROnly */
           XSTP_SNMP_RETURN_INTEGER64 (sid_vip->key.icomp_id);
           break;

      case IEEE8021PBBISIDTOVIPPORT:        /* ROnly */
           XSTP_SNMP_RETURN_INTEGER64 (sid_vip->vip_port_num);
           break;
   }

   return NULL;
}

/*
 * Callback function for reading and writing PIP OID's
 */
u_char *
pbb_snmp_ahPip_table (struct variable *vp, oid * name, size_t * length,
                      int exact, size_t * var_len, WriteMethod ** write_method,
                      u_int32_t vr_id)
{
   struct apn_vr *vr = NULL;
   struct nsm_master *master = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;
   struct nsm_pbb_icomponent *pbb_icomp = NULL;
   struct pip_tbl *pip = NULL;
   int ret;
   u_int32_t port_num = 0;

   /* validate the index */
   ret = l2_snmp_index_get (vp, name, length, &port_num, exact);
   if (ret < 0)
     return NULL;

   vr = apn_vr_get_privileged (snmpg);
   if (! vr)
     return NULL;

   master = vr->proto;
   if (! master)
     return NULL;

   bridge_master = nsm_bridge_get_master (master);
   if (! bridge_master)
     return NULL;

   beb_bridge = bridge_master->beb;
   if (! beb_bridge)
     return NULL;

   pbb_icomp = beb_bridge->icomp;
   if (! pbb_icomp)
     return NULL ;

   pip = pbb_snmp_pip_lookup_by_ifindex (pbb_icomp->pip_tbl_list,
                                         port_num, exact);
   if (! pip)
     return NULL ;

   if (! exact)
     l2_snmp_index_set (vp, name, length, pip->key.pip_port_num);


   switch(vp->magic)
   {
      case IEEE8021PBBPIPBMACADDRESS:  /* ROnly */
           XSTP_SNMP_RETURN_MACADDRESS (pip->pip_src_bmac);

      case IEEE8021PBBPIPNAME:         /* RWrite */
	   *write_method = pbb_snmp_write_pip_table;
           * var_len = strlen(pip->pip_port_name);
           return pip->pip_port_name;

      case IEEE8021PBBPAPNCOMPONENTID: /* ROnly */
	   XSTP_SNMP_RETURN_INTEGER64 (pip->key.icomp_id);

      case IEEE8021PBBPIPVIPMAP:       /* ROnly */  
           XSTP_SNMP_RETURN_INTEGER64(pip->pip_vip_map);
   }
   return NULL;
}

/*
 * Write method for PIP OID's
 */
int
pbb_snmp_write_pip_table (int action, u_char * var_val, u_char var_val_type,
                          size_t var_val_len, u_char * statP, oid * name,
                          size_t name_len, struct variable *v, u_int32_t vr_id)
{
   struct nsm_master *master = NULL;
   struct apn_vr *vr = NULL;
   struct nsm_bridge *br = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;
   struct nsm_pbb_icomponent *pbb_icomp = NULL;
   struct pip_tbl *pip = NULL;
   int ret;
   u_int32_t port_num = 0;

   /* validate the index */
   ret = l2_snmp_index_get (v, name, &name_len, &port_num, PAL_TRUE);
   if (ret < 0)
     return SNMP_ERR_GENERR;

   if (var_val_type != ASN_OCTET_STR)
     return SNMP_ERR_WRONGTYPE;

   vr = apn_vr_get_privileged (snmpg);
   if (! vr)
     return SNMP_ERR_GENERR;

   master = vr->proto;
   if (! master)
     return SNMP_ERR_GENERR;

   bridge_master = nsm_bridge_get_master (master);
   if (! bridge_master)
     return SNMP_ERR_GENERR;

   beb_bridge = bridge_master->beb;
   if (! beb_bridge)
     return SNMP_ERR_GENERR;

   pbb_icomp = beb_bridge->icomp;
   if (! pbb_icomp)
     return  SNMP_ERR_GENERR;

   br = nsm_snmp_get_first_bridge();
   if (br == NULL)
     return SNMP_ERR_GENERR;

   pip = nsm_pbb_search_pip_node (pbb_icomp->pip_tbl_list,
                                  br->bridge_id, port_num);
   if (! pip)
     return SNMP_ERR_GENERR;

   switch (v->magic)
   {
      case IEEE8021PBBPIPNAME:         /* RWrite */
           pal_mem_cpy (pip->pip_port_name, var_val, NSM_PBB_PORT_NAME_SIZ);
           break;
   }
   return SNMP_ERR_NOERROR;
}

/*
 * Callback function for reading and writing VIP-PIP-Mapping OID's 
 */
u_char *
pbb_snmp_ahVip_Pip_map (struct variable *vp, oid * name, size_t * length,
                        int exact, size_t * var_len,
                        WriteMethod ** write_method, u_int32_t vr_id)
{
   struct nsm_master *master = NULL;
   struct apn_vr *vr = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;
   struct nsm_pbb_icomponent *pbb_icomp = NULL;
   struct vip2pip_map  *vip_pip = NULL;
   int ret;
   u_int32_t icomp_id = 0;
   u_int32_t port_num  = 0;

   /* validate the index */
   ret = l2_snmp_integer_index_get (vp, name, length, &icomp_id, &port_num, exact);
   if (ret < 0)
     return NULL;

   if (exact && (icomp_id < PBB_ICOMP_ID_MIN  || icomp_id > PBB_ICOMP_ID_MAX))
     return NULL;

   vr = apn_vr_get_privileged (snmpg);
   if (! vr)
     return NULL;

   master = vr->proto;
   if (! master)
     return NULL;

   bridge_master = nsm_bridge_get_master (master);
   if (! bridge_master)
     return NULL;

   beb_bridge = bridge_master->beb;
   if (! beb_bridge)
     return NULL;

   pbb_icomp = beb_bridge->icomp;
   if (! pbb_icomp)
     return NULL;

   vip_pip = pbb_snmp_vip_pip_lookup_by_portnum (pbb_icomp->vip2pip_map_list, port_num,
                                                 exact);
   if (! exact)
     {
        if (! vip_pip)
          return NULL;
        l2_snmp_integer_index_set (vp, name, length, vip_pip->key.icomp_id,
                                   vip_pip->key.vip_port_num);
     }

   switch(vp->magic)
   {
      case IEEE8021PBBVIPTOPIPMAPPINGPAPNFINDEX:  /* RCreate */
	   *write_method =  pbb_snmp_write_Vip_Pip_map;
           if (! vip_pip)
             return NULL;
           XSTP_SNMP_RETURN_INTEGER64 (vip_pip->pip_port_number);

      case IEEE8021PBBVIPTOPIPMAPPINGBMACADDRESS: /* RCreate */
	   *write_method =  pbb_snmp_write_Vip_Pip_map;
           if (! vip_pip)
             return NULL;
           XSTP_SNMP_RETURN_MACADDRESS (vip_pip->pip_src_bmac);

      case IEEE8021PBBVIPTOPIPMAPPINGSTORAGETYPE: /* ROnly */
           *write_method =  pbb_snmp_write_Vip_Pip_map;
           if (! vip_pip)
             return NULL;
            * var_len = strlen(vip_pip->storage_type);
           return vip_pip->storage_type;
   }
   return NULL;
}

/*
 * Write method for VIP-PIP-Mapping Rowstatus
 */
int
pbb_snmp_write_Vip_Pip_Rowstatus (int action, u_char * var_val,
                                  u_char var_val_type,
                                  size_t var_val_len,
                                  u_char * statP, oid * name, size_t name_len,
                                  struct variable *v, u_int32_t vr_id)
{
   struct nsm_master *master = NULL;
   struct apn_vr *vr = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;
   struct nsm_pbb_icomponent *pbb_icomp = NULL;
   struct vip_tbl *vip = NULL;
   struct pip_tbl *pip = NULL;
   struct vip2pip_map  *vip_pip = NULL;
   long   setval;
   u_int32_t icomp_id = 0;
   u_int32_t port_num = 0;
   int ret;

   /* validate the index */
   ret = l2_snmp_integer_index_get (v, name, &name_len, &icomp_id, &port_num, PAL_TRUE);
   if (ret < 0)
     return SNMP_ERR_GENERR;

   if (var_val_type != ASN_INTEGER)
     return SNMP_ERR_WRONGTYPE;

   if (icomp_id < PBB_ICOMP_ID_MIN || icomp_id > PBB_ICOMP_ID_MAX)
     return SNMP_ERR_BADVALUE;
   
   pal_mem_cpy(&setval, var_val, SET_VAL_SIZE);

   if ((setval < PBB_SNMP_ROW_STATUS_ACTIVE) ||
       (setval > PBB_SNMP_ROW_STATUS_DESTROY))
     return SNMP_ERR_BADVALUE;
  
   vr = apn_vr_get_privileged (snmpg);
   if (! vr)
     return SNMP_ERR_GENERR;

   master = vr->proto;
   if (! master)
     return SNMP_ERR_GENERR;

   bridge_master = nsm_bridge_get_master (master);
   if (! bridge_master)
     return SNMP_ERR_GENERR;

   beb_bridge = bridge_master->beb;
   if (! beb_bridge)
     return SNMP_ERR_GENERR;

   pbb_icomp = beb_bridge->icomp;
   if (! pbb_icomp)
     return SNMP_ERR_GENERR;

   vip_pip = nsm_pbb_search_vip2pip_node (pbb_icomp->vip2pip_map_list,
                                          icomp_id, port_num);
   switch (setval)
   {
      case PBB_SNMP_ROW_STATUS_ACTIVE:
           break;

      case PBB_SNMP_ROW_STATUS_CREATEANDGO:
           break;

      case PBB_SNMP_ROW_STATUS_NOTINSERVICE:
           if (! vip_pip)
             return SNMP_ERR_GENERR;           
           break;

      case PBB_SNMP_ROW_STATUS_NOTREADY:
           if (! vip_pip)
             return SNMP_ERR_GENERR;
           break;

      case PBB_SNMP_ROW_STATUS_CREATEANDWAIT:
           if (! vip_pip)
             {
  	        pip = nsm_pbb_search_pip_node (pbb_icomp->pip_tbl_list,
                                               icomp_id, port_num);
                if (pip && (pip->key.pip_port_num == port_num))
                  {
                    if (vip)
                      {
                        nsm_pbb_set_pipvip_map (pip->pip_vip_map,
                            vip->key.vip_port_num);
                        nsm_pbb_add_vip2pip_map (pbb_icomp, pip,
                            vip->key.vip_port_num);
                      }
                  }
             }
           break;

      case PBB_SNMP_ROW_STATUS_DESTROY:
           if (! vip_pip)
             return SNMP_ERR_GENERR;

           avl_remove(pbb_icomp->vip2pip_map_list, (void *) vip_pip);
           XFREE(MTYPE_NSM_BRIDGE_PBB_VIP_NODE, vip_pip);
           break;
   }
   return SNMP_ERR_NOERROR;
}

/*
 * Write method for VIP-PIP-Mapping OID's
 */
int
pbb_snmp_write_Vip_Pip_map (int action, u_char * var_val, u_char var_val_type,
                            size_t var_val_len, u_char * statP, oid * name,
                            size_t name_len, struct variable *v, u_int32_t vr_id)
{
   struct nsm_master *master = NULL;
   struct apn_vr *vr = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;
   struct nsm_pbb_icomponent *pbb_icomp = NULL;
   struct vip2pip_map  *vip_pip = NULL;
   int ret;
   u_int32_t icomp_id = 0;
   u_int32_t port_num = 0;
   long setval;

   /* validate the index */
   ret = l2_snmp_integer_index_get (v, name, &name_len, &icomp_id, &port_num, PAL_TRUE);
   if (ret < 0)
     return SNMP_ERR_GENERR;

   if (icomp_id < PBB_ICOMP_ID_MIN || icomp_id > PBB_ICOMP_ID_MAX)
     return SNMP_ERR_BADVALUE;

   vr = apn_vr_get_privileged (snmpg);
   if (! vr)
     return SNMP_ERR_GENERR;

   master = vr->proto;
   if (! master)
     return SNMP_ERR_GENERR;

   bridge_master = nsm_bridge_get_master (master);
   if (! bridge_master)
     return SNMP_ERR_GENERR;

   beb_bridge = bridge_master->beb;
   if (! beb_bridge)
     return SNMP_ERR_GENERR;

   pbb_icomp = beb_bridge->icomp;
   if (! pbb_icomp)
     return SNMP_ERR_GENERR;

   vip_pip = nsm_pbb_search_vip2pip_node (pbb_icomp->vip2pip_map_list,
                                          icomp_id, port_num);
   if (! vip_pip)
     return SNMP_ERR_GENERR;

   pal_mem_cpy (&setval, var_val,  SET_VAL_SIZE);

   switch(v->magic)
   {
      case IEEE8021PBBVIPTOPIPMAPPINGPAPNFINDEX:  /* RCreate */
           if (var_val_type != ASN_INTEGER)
             return SNMP_ERR_WRONGTYPE;

           if (setval < PBB_MIN_VAL)
             return SNMP_ERR_BADVALUE;
           else 
             vip_pip->pip_port_number = setval;
           break;
   }
   return SNMP_ERR_NOERROR;
}
#endif /*HAVE_I_BEB*/

/*
 * Callback function for reading and writing Service Instance OID's
 */
#if defined HAVE_B_BEB
u_char *
pbb_snmp_ahService_table (struct variable *vp, oid * name, size_t * length,
                          int exact, size_t * var_len,
                          WriteMethod ** write_method, u_int32_t vr_id)
{
   struct nsm_master *master = NULL;
   struct apn_vr *vr = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;
   struct nsm_pbb_bcomponent *pbb_bcomp = NULL;
   struct cbp_srvinst_tbl *cbp = NULL;
   int ret;
   u_int32_t bcomp_id = 0;
   u_int32_t port_num = 0;
   u_int32_t backbone_sid = 0;

   /* validate the index */
   ret = pbb_snmp_integer_index3_get (vp, name, length, &bcomp_id, &port_num,
				      &backbone_sid, exact);
   if (ret != SNMP_ERR_NOERROR)
     return NULL ;

   vr = apn_vr_get_privileged (snmpg);
   if (! vr)
     return NULL;

   master = vr->proto;
   if (! master)
     return NULL;

   bridge_master = nsm_bridge_get_master (master);
   if (! bridge_master)
     return NULL;

   beb_bridge = bridge_master->beb;
   if (! beb_bridge)
     return NULL; 

   pbb_bcomp = beb_bridge->bcomp;
   if (! pbb_bcomp)
     return NULL;

   cbp = pbb_snmp_cbp_lookup_by_isid (pbb_bcomp->cbp_srvinst_list,
                                      backbone_sid, exact);
   if (! cbp)
          return NULL;

   if (! exact)
     l2_snmp_integer_index3_set (vp, name, length, cbp->key.bcomp_id,
                                    cbp->key.bcomp_port_num, cbp->key.b_sid);

   switch( vp->magic )
   {
      case IEEE8021PBBCBPSERVICEMAPPINGBVID:     /* RWrite */
	   *write_method = pbb_snmp_write_Service_table;
           XSTP_SNMP_RETURN_INTEGER64 (cbp->bvid);

      case IEEE8021PBBCBPSERVICEMAPPINGDEFAULTBACKBONEDEST:  /* RWrite */
           XSTP_SNMP_RETURN_MACADDRESS (cbp->default_dst_bmac);

      case IEEE8021PBBCBPSERVICEMAPPINGTYPE:     /* RWrite */
	   *write_method = pbb_snmp_write_Service_table;
           XSTP_SNMP_RETURN_INTEGER64 (cbp->pt_mpt_type);            

      case IEEE8021PBBCBPSERVICEMAPPINGLOCALSID:   /* RWrite */
	   *write_method = pbb_snmp_write_Service_table; 
           XSTP_SNMP_RETURN_INTEGER64 (cbp->local_sid);
   }

   return NULL;
}

/*
 * Write method for Service Instance OID's
 */
int
pbb_snmp_write_Service_table (int action, u_char * var_val,
                              u_char var_val_type, size_t var_val_len,
                              u_char * statP, oid * name, size_t name_len,
                              struct variable *v, u_int32_t vr_id)
{
   struct nsm_master *master = NULL;
   struct apn_vr *vr = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;
   struct nsm_pbb_bcomponent *pbb_bcomp = NULL;
   struct cbp_srvinst_tbl *cbp = NULL;
   int ret;
   u_int32_t bcomp_id = 0;
   u_int32_t port_num = 0;
   u_int32_t backbone_sid = 0;
   long setval;
   struct nsm_vlan vlan;
   nsm_vid_t bvid = 0;
   struct avl_node *rn = NULL;
   
   /* validate the index */
   ret = pbb_snmp_integer_index3_get (v, name, &name_len, &bcomp_id, &port_num,
                                      &backbone_sid, 1);
   if (ret != SNMP_ERR_NOERROR)
     return SNMP_ERR_GENERR;

   vr = apn_vr_get_privileged (snmpg);
   if (! vr)
     return SNMP_ERR_GENERR;

   master = vr->proto;
   if (! master)
     return SNMP_ERR_GENERR;

   bridge_master = nsm_bridge_get_master (master);
   if (! bridge_master)
     return SNMP_ERR_GENERR;

   beb_bridge = bridge_master->beb;
   if (! beb_bridge)
     return SNMP_ERR_GENERR;

   pbb_bcomp = beb_bridge->bcomp;
   if (! pbb_bcomp)
     return SNMP_ERR_GENERR;
   
   cbp = nsm_pbb_search_cbp_node (pbb_bcomp->cbp_srvinst_list,
                                  port_num, backbone_sid);
   if (! cbp)
     return SNMP_ERR_GENERR;

   pal_mem_cpy(&setval, var_val, SET_VAL_SIZE);

   switch(v->magic)
   {
      case IEEE8021PBBCBPSERVICEMAPPINGBVID:     /* RWrite */
           if (var_val_type != ASN_INTEGER)
             return SNMP_ERR_WRONGTYPE;

           if (setval < PBB_BVID_MIN || setval > PBB_BVID_MAX)
             return SNMP_ERR_BADVALUE;
   
           bvid = (nsm_vid_t) setval;

           NSM_VLAN_SET (&vlan, bvid);
           
           rn = avl_search (bridge_master->b_bridge->bvlan_table,
                            (void *)&vlan);
           if (rn == NULL  || rn->info == NULL)
             return SNMP_ERR_BADVALUE;

           cbp->bvid = setval;
           break;

      case IEEE8021PBBCBPSERVICEMAPPINGDEFAULTBACKBONEDEST:  /* RWrite */
           if (var_val_type != ASN_OCTET_STR)
             return SNMP_ERR_WRONGTYPE;

           pal_mem_cpy (cbp->default_dst_bmac, var_val, SET_MAC_SIZE);
           break;

      case IEEE8021PBBCBPSERVICEMAPPINGTYPE:     /* RWrite */
           if (var_val_type != ASN_INTEGER)
             return SNMP_ERR_WRONGTYPE;

           if (setval == NSM_BVLAN_P2P || setval == NSM_BVLAN_M2M)
             cbp->pt_mpt_type = setval;
           else
             return SNMP_ERR_BADVALUE;
           break;

      case IEEE8021PBBCBPSERVICEMAPPINGLOCALSID:   /* RWrite */
           if (var_val_type != ASN_INTEGER)
             return SNMP_ERR_WRONGTYPE;

           if (setval < PBB_ISID_MIN || setval > PBB_ISID_MAX)
             return SNMP_ERR_BADVALUE;
           else
             cbp->local_sid = setval;
           break;
   }
   return SNMP_ERR_NOERROR;
}
#endif /*HAVE_ B_BEB*/

/*
 * Callback function for reading and writing CNP OID's
 */
#if defined HAVE_I_BEB
u_char *
pbb_snmp_ahCnp_table (struct variable *vp, oid * name, size_t * length,
                     int exact, size_t * var_len, WriteMethod ** write_method,
                     u_int32_t vr_id)
{  
   struct nsm_master *master = NULL;
   struct apn_vr *vr = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;
   struct nsm_pbb_icomponent *pbb_icomp = NULL;
   struct cnp_srv_tbl *cnp = NULL;
   struct svlan_bundle *svlan_bdl = NULL;
   struct listnode *lcnp_node = NULL;
   u_int32_t icomp_id = 0;
   u_int32_t port_num = 0;
   u_int32_t isid = 0;
   int ret;

   /* validate the index */
   ret = pbb_snmp_integer_index3_get (vp, name, length, &icomp_id, &port_num,
                                      &isid, exact);
   if (ret != SNMP_ERR_NOERROR)
     return NULL;

   if (exact && (icomp_id < PBB_ICOMP_ID_MIN || icomp_id > PBB_ICOMP_ID_MAX))
     return NULL;
   
   vr = apn_vr_get_privileged (snmpg);
   if (! vr)	
     return NULL;
  
   master = vr->proto;
   if (! master)
     return NULL;

   bridge_master = nsm_bridge_get_master(master);
   if (! bridge_master)
     return NULL ;

   beb_bridge = bridge_master->beb;
   if (! beb_bridge)
     return NULL;

   pbb_icomp = beb_bridge->icomp;
   if (! pbb_icomp)
     return NULL;

   cnp = pbb_snmp_cnp_lookup_by_isid (pbb_icomp->cnp_srv_tbl_list ,isid,
                                      exact);

   if (! exact)
     {
        if (! cnp)
          return NULL;
        l2_snmp_integer_index3_set (vp, name, length, cnp->key.icomp_id,
                                    cnp->key.icomp_port_num, cnp->key.sid);
     }

   switch(vp->magic)
   {
      case IEEE8021PBBCNPSERVICEINTERFACETYPE: /* RCreate */
           *write_method = pbb_snmp_write_Cnp_table;
           if (! cnp)
             return NULL;

	   XSTP_SNMP_RETURN_INTEGER64 (cnp->srv_type);
   
      case IEEE8021PBBCNPSERVIREFPORTNUMBER: /* RCreate */
           *write_method = pbb_snmp_write_Cnp_table;
           if (! cnp)
             return NULL;

           XSTP_SNMP_RETURN_INTEGER64 (cnp->ref_port_num);

      case IEEE8021PBBCNPSERVICESVIDLOW:       /* RCreate */
           *write_method = pbb_snmp_write_Cnp_table;
           if (! cnp)
             return NULL;

           LIST_LOOP (cnp->svlan_bundle_list, svlan_bdl, lcnp_node);
           if (! svlan_bdl)
             return NULL;

           XSTP_SNMP_RETURN_INTEGER64 (svlan_bdl->svid_low);

      case IEEE8021PBBCNPSERVICESVIDHIGH:      /* RCreate */
           *write_method = pbb_snmp_write_Cnp_table;
           if (! cnp)
             return NULL;

 	   LIST_LOOP (cnp->svlan_bundle_list, svlan_bdl, lcnp_node);
           if (! svlan_bdl)
             return NULL;

           XSTP_SNMP_RETURN_INTEGER64 (svlan_bdl->svid_high);

      case IEEE8021PBBCNPSERVICEPORTTYPE:      /* RCreate */
           *write_method = pbb_snmp_write_Cnp_table;
           if (! cnp)
             return NULL;
       
           LIST_LOOP (cnp->svlan_bundle_list, svlan_bdl, lcnp_node);
           if (! svlan_bdl)
             return NULL;

           XSTP_SNMP_RETURN_INTEGER64 (svlan_bdl->service_type);

      case IEEE8021PBBCNPROWSTATUS:            /* RCreate */
           *write_method = pbb_snmp_write_Cnp_Rowstatus;
           if (! cnp)
             return NULL;

           XSTP_SNMP_RETURN_INTEGER64 (cnp->rowstatus);
           break;
   }

   return NULL;
}

/*
 * Write method for CNP Rowstatus 
 */
int
pbb_snmp_write_Cnp_Rowstatus (int action, u_char * var_val,
                              u_char var_val_type, size_t var_val_len,
                              u_char * statP, oid * name, size_t name_len,
                              struct variable *v, u_int32_t vr_id)
{
   struct nsm_master *master = NULL;
   struct apn_vr *vr = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;
   struct nsm_pbb_icomponent *pbb_icomp = NULL;
   struct cnp_srv_tbl *cnp = NULL;
   long setval;
   u_int32_t icomp_id = 0;
   u_int32_t port_num = 0;
   u_int32_t isid = 0;
   int ret;

   /* validate the index */
   ret = pbb_snmp_integer_index3_get (v, name, &name_len, &icomp_id, &port_num,
                                      &isid, PAL_TRUE);
   if (ret != SNMP_ERR_NOERROR)
     return SNMP_ERR_GENERR;

   if (var_val_type != ASN_INTEGER)
     return SNMP_ERR_WRONGTYPE;

   if (icomp_id < PBB_ICOMP_ID_MIN || icomp_id > PBB_ICOMP_ID_MAX)
     return SNMP_ERR_GENERR;

   pal_mem_cpy(&setval, var_val, SET_VAL_SIZE);

   if ((setval < PBB_SNMP_ROW_STATUS_ACTIVE) ||
       (setval > PBB_SNMP_ROW_STATUS_DESTROY))
     return SNMP_ERR_BADVALUE;

   vr = apn_vr_get_privileged (snmpg);
   if (! vr)
     return SNMP_ERR_GENERR;

   master = vr->proto;
   if (! master)
     return SNMP_ERR_GENERR;

   bridge_master = nsm_bridge_get_master(master);
   if (! bridge_master)
     return SNMP_ERR_GENERR;

   beb_bridge = bridge_master->beb;
   if (! beb_bridge)
     return SNMP_ERR_GENERR;

   pbb_icomp = beb_bridge->icomp;
   if (! pbb_icomp)
     return SNMP_ERR_GENERR;

   cnp = nsm_pbb_search_cnp_node (pbb_icomp->cnp_srv_tbl_list,
                                  icomp_id, isid, port_num);
   switch (setval)
   {
      case PBB_SNMP_ROW_STATUS_ACTIVE:
           break;

      case PBB_SNMP_ROW_STATUS_CREATEANDGO:
           break;

      case PBB_SNMP_ROW_STATUS_NOTINSERVICE:
           if (! cnp)
             return SNMP_ERR_GENERR;

           cnp->rowstatus = PBB_SNMP_ROW_STATUS_NOTINSERVICE;
           break;

      case PBB_SNMP_ROW_STATUS_NOTREADY:
           if (! cnp)
             return SNMP_ERR_GENERR;

           cnp->rowstatus = PBB_SNMP_ROW_STATUS_NOTREADY; 
           break;

      case PBB_SNMP_ROW_STATUS_CREATEANDWAIT:
           if (! cnp)
             {
                ret = pbb_snmp_cnp_entry_add (pbb_icomp->cnp_srv_tbl_list, icomp_id, isid, port_num) ;
                if (ret != SNMP_ERR_NOERROR)
                  return SNMP_ERR_GENERR;
             }
           break;

      case PBB_SNMP_ROW_STATUS_DESTROY:
           if (! cnp)
             return SNMP_ERR_GENERR;

           avl_remove(pbb_icomp->cnp_srv_tbl_list, (void *) cnp);
           XFREE(MTYPE_NSM_BRIDGE_PBB_CNP_NODE, cnp);
           break;
   }
   return SNMP_ERR_NOERROR;
}

/*
 * Write method for CNP table
 */
int pbb_snmp_write_Cnp_table (int action,
                              u_char * var_val,
                              u_char var_val_type,
                              size_t var_val_len,
                              u_char * statP, oid * name,
                              size_t name_len,
                              struct variable *v, u_int32_t vr_id)
{
   struct nsm_master *master = NULL;
   struct apn_vr *vr = NULL;
   struct nsm_bridge_master *bridge_master = NULL;
   struct nsm_beb_bridge *beb_bridge = NULL;
   struct nsm_pbb_icomponent *pbb_icomp = NULL;
   struct cnp_srv_tbl *cnp = NULL;
   struct svlan_bundle *svlan_bdl = NULL;
   struct listnode *lcnp_node = NULL;
   long setval;
   u_int32_t icomp_id = 0;
   u_int32_t port_num = 0;
   u_int32_t isid = 0;
   int ret;

   /* validate the index */
   ret = pbb_snmp_integer_index3_get (v, name, &name_len, &icomp_id, &port_num,
                                      &isid, PAL_TRUE);
   if (ret != SNMP_ERR_NOERROR)
     return SNMP_ERR_GENERR;

   if (var_val_type != ASN_INTEGER)
     return SNMP_ERR_WRONGTYPE;

   if (icomp_id < PBB_ICOMP_ID_MIN  || icomp_id > PBB_ICOMP_ID_MAX)
     return SNMP_ERR_GENERR;

   vr = apn_vr_get_privileged (snmpg);
   if (! vr)
     return SNMP_ERR_GENERR;

   master = vr->proto;
   if (! master)
     return SNMP_ERR_GENERR;

   bridge_master = nsm_bridge_get_master(master);
   if (! bridge_master)
     return SNMP_ERR_GENERR;

   beb_bridge = bridge_master->beb;
   if (! beb_bridge)
     return SNMP_ERR_GENERR;

   pbb_icomp = beb_bridge->icomp;
   if (! pbb_icomp)
     return SNMP_ERR_GENERR;

   cnp = nsm_pbb_search_cnp_node (pbb_icomp->cnp_srv_tbl_list,
                                  icomp_id, isid, port_num);
   if (!cnp)
     return SNMP_ERR_GENERR;

   pal_mem_cpy (&setval, var_val, SET_VAL_SIZE);

   switch(v->magic)
   {
      case IEEE8021PBBCNPSERVICEINTERFACETYPE: /* RCreate */
           if (cnp->rowstatus == PBB_SNMP_ROW_STATUS_NOTINSERVICE ||
               cnp->rowstatus ==  PBB_SNMP_ROW_STATUS_NOTREADY )
             {
                if ((setval < NSM_SERVICE_INTERFACE_SVLAN_SINGLE ) ||
                    (setval > NSM_SERVICE_INTERFACE_PORTBASED))
                  return SNMP_ERR_BADVALUE;
                else
                  cnp->srv_type = setval;
             }
           break;

      case IEEE8021PBBCNPSERVIREFPORTNUMBER: /* RCreate */
           if (cnp->rowstatus == PBB_SNMP_ROW_STATUS_NOTINSERVICE ||
               cnp->rowstatus ==  PBB_SNMP_ROW_STATUS_NOTREADY )
             {
                if (setval < PBB_MIN_VAL)
                  return SNMP_ERR_BADVALUE;
                else
                  cnp->ref_port_num = setval;
             }
           break;

      case IEEE8021PBBCNPSERVICESVIDLOW:       /* RCreate */
           if (cnp->rowstatus == PBB_SNMP_ROW_STATUS_NOTINSERVICE ||
               cnp->rowstatus ==  PBB_SNMP_ROW_STATUS_NOTREADY )
             {
                if (cnp->srv_type ==  NSM_SERVICE_INTERFACE_PORTBASED)
                  return SNMP_ERR_BADVALUE;

		LIST_LOOP (cnp->svlan_bundle_list, svlan_bdl, lcnp_node);
                if (! svlan_bdl)
                  return SNMP_ERR_GENERR;

                if (setval < PBB_BVID_MIN || setval > svlan_bdl->svid_high
                    || setval > PBB_BVID_MAX)
                  return SNMP_ERR_BADVALUE;
                else
                  svlan_bdl->svid_low = setval;
	     }
           break;

      case IEEE8021PBBCNPSERVICESVIDHIGH:      /* RCreate */
           if (cnp->rowstatus == PBB_SNMP_ROW_STATUS_NOTINSERVICE ||
               cnp->rowstatus ==  PBB_SNMP_ROW_STATUS_NOTREADY )
             {
                if (cnp->srv_type ==  NSM_SERVICE_INTERFACE_PORTBASED)
                  return SNMP_ERR_BADVALUE;
 
	        LIST_LOOP (cnp->svlan_bundle_list, svlan_bdl, lcnp_node);
                if (! svlan_bdl)
                  return SNMP_ERR_GENERR;

                if (setval < PBB_BVID_MIN || setval < svlan_bdl->svid_low
                    || setval > PBB_BVID_MAX)
                  return SNMP_ERR_BADVALUE;
                else
                  svlan_bdl->svid_high = setval;
             }
           break;

      case IEEE8021PBBCNPSERVICEPORTTYPE:      /* RCreate */
           if (cnp->rowstatus == PBB_SNMP_ROW_STATUS_NOTINSERVICE ||
               cnp->rowstatus ==  PBB_SNMP_ROW_STATUS_NOTREADY )
	     {
                if (cnp->srv_type ==  NSM_SERVICE_INTERFACE_PORTBASED)
                  return SNMP_ERR_BADVALUE;

  	        LIST_LOOP (cnp->svlan_bundle_list, svlan_bdl, lcnp_node);
                if (! svlan_bdl)
                  return SNMP_ERR_GENERR;

                if (setval < NSM_PORT_SERVICE_BOTH || setval > NSM_PORT_SERVICE_EGRESS)
                  return SNMP_ERR_BADVALUE;
                else
                  svlan_bdl->service_type= setval;
	     }
           break;
   }
   return SNMP_ERR_NOERROR;
}
#endif /*HAVE_I_BEB*/
#endif/* HAVE_SNMP*/
#endif/*  HAVE_VLAN*/
#endif/*(HAVE_I_BEB) || defined (HAVE_B_BEB)*/
