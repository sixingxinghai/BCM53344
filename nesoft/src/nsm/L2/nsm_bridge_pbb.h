/* Copyright (C) 2008  All Rights Reserved */

#ifndef __NSM_BRIDGE_PBB_H__
#define __NSM_BRIDGE_PBB_H__

#define BEB_BRIDGE_NAME 32
#define BACKBONE_NAME_LEN 32 
#define NSM_PBB_PORT_NAME_SIZ 32
#define NSM_PBB_ISID_NAME_SIZ 32
#define NSM_PBB_VIP_PIP_MAP_LEN 4096
#if defined HAVE_PBB_TE
#if defined (HAVE_I_BEB) && defined (HAVE_B_BEB)
#if defined HAVE_GMPLS && defined HAVE_GELS
  #define VLAN_MAP_TABLE_LEN 512
#endif
#endif
#endif

#define HAVE_PBB_DEBUG
/* To check the service flow type of the port */
enum nsm_port_service_type 
{
  NSM_PORT_SERVICE_BOTH,
  NSM_PORT_SERVICE_INGRESS,
  NSM_PORT_SERVICE_EGRESS
};

/*
 * PBB Snmp row status values
 */
enum pbb_snmp_rowstatus
{
   PBB_SNMP_ROW_STATUS_ACTIVE = 1,
   PBB_SNMP_ROW_STATUS_NOTINSERVICE,
   PBB_SNMP_ROW_STATUS_NOTREADY,
   PBB_SNMP_ROW_STATUS_CREATEANDGO,
   PBB_SNMP_ROW_STATUS_CREATEANDWAIT,
   PBB_SNMP_ROW_STATUS_DESTROY
};

enum pbb_snmp_storage_type
{
    PBB_SNMP_STORAGE_TYPE_OTHER = 1,
    PBB_SNMP_STORAGE_TYPE_VOLATILE,
    PBB_SNMP_STORAGE_TYPE_NONVOLATILE,
    PBB_SNMP_STORAGE_TYPE_PERMANENT,
    PBB_SNMP_STORAGE_TYPE_READONLY
};


/* To check the status of the service */
enum nsm_pbb_service_status
{
  NSM_SERVICE_STATUS_NOT_DISPATCHED,
  NSM_SERVICE_STATUS_DISPATCHED,   
  NSM_SERVICE_STATUS_RUNNING
};
/* To check the service interface type at CNP */ 
enum nsm_pbb_service_interface_type
{
  NSM_SERVICE_INTERFACE_SVLAN_SINGLE,
  NSM_SERVICE_INTERFACE_SVLAN_MANY,
  NSM_SERVICE_INTERFACE_SVLAN_ALL,
  NSM_SERVICE_INTERFACE_PORTBASED
};

#ifdef HAVE_I_BEB
/* I component container class*/
struct nsm_pbb_icomponent
{
  struct avl_tree              *vip_table_list;
  struct avl_tree              *sid2vip_xref_list;
  struct list                  *pip_tbl_list;
  struct avl_tree              *vip2pip_map_list;
  struct avl_tree              *cnp_srv_tbl_list;
};
#endif /*HAVE_I_BEB*/

#ifdef HAVE_B_BEB
/*B component container class*/
struct nsm_pbb_bcomponent
{
  struct avl_tree              *cbp_srvinst_list;
#if defined HAVE_I_BEB && defined HAVE_PBB_TE
  struct avl_tree              *pbb_te_inst_table;
  struct avl_tree              *pbb_te_aps_table;
  struct avl_tree              *pbb_te_aps_isid_table;
#if defined HAVE_GMPLS && defined HAVE_GELS
  struct hash                  *pbb_tesi_name_to_id_table;
  struct avl_tree              *pbb_remote_mac_to_tesid_table;
#endif
#endif /* HAVE_I_BEB && HAVE_PBB_TE */
};
#endif /*HAVE_B_BEB*/
/* BEB master structure */
struct nsm_beb_bridge
{
  struct nsm_bridge_master        *master;
#ifdef HAVE_I_BEB
  struct nsm_pbb_icomponent       *icomp;
  u_int32_t    beb_tot_icomp;
/* number of I-comp in this BEB. May range from .0. to many.
   802.1ah 12.16.1.2/3 */
#endif /* HAVE_I_BEB */

#ifdef HAVE_B_BEB
  struct nsm_pbb_bcomponent       *bcomp;
  u_int32_t    beb_tot_bcomp;
/* number of B-comp in this BEB. May be .0. or .1..
   802.1ah 12.16.1.2/3 */
#endif /* HAVE_B_BEB */
  u_int32_t    beb_tot_ext_ports; 
/* number of externally facing ports (CNPs, PIPs, CBPs, and PNPs).
   802.1ah 12.16.1.2/3*/
  u_int8_t     beb_mac_addr [ETHER_ADDR_LEN]; 
/* BEB bridge MAC address used to access the bridge configuration.
   802.1ah 12.16.1.1.3, 12.16.1.2/3*/
  u_char       beb_name [BEB_BRIDGE_NAME + 1]; 
  u_char       backbone_bridge_name[BACKBONE_NAME_LEN + 1];
/* BEB name text string of local significance.
   802.1ah 12.16.1.2/3*/

};

#ifdef HAVE_I_BEB
struct vip_tbl_key
{
  u_int32_t  icomp_id ;              
  u_int32_t  vip_port_num ;
};

/* virtual instance port (VIP) configuration table. 802.1ah 12.16.3.1/2*/
struct vip_tbl 
{
  struct vip_tbl_key             key;
  u_int32_t                      vip_sid;
  u_char                         srv_inst_name[NSM_PBB_ISID_NAME_SIZ];
  u_int8_t                       default_dst_bmac [ETHER_ADDR_LEN]; 
  enum nsm_port_service_type     srv_type; 
#ifndef HAVE_B_BEB 
  /* This parameter is added to keep a count of isid mappings to PIP*/
  u_int16_t                      pip_ref_count;
#endif /*HAVE_B_BEB && HAVE_I_BEB*/
  enum  nsm_pbb_service_status   status;
  enum  pbb_snmp_rowstatus       rowstatus;
};

struct sid2vip_key
{
  u_int32_t                     icomp_id;
  u_int32_t                     vip_sid;
};
/* 12.16.3.1/2 I-SID to VIP cross-reference table */
struct sid2vip_xref
{
  struct sid2vip_key            key;
  u_int32_t                     vip_port_num;
};

struct pip_tbl_key 
{
  u_int32_t                     icomp_id;
  u_int32_t                     pip_port_num;
};
/* 802.1ah 12.16.4.1/2 PIP configuration table*/
struct pip_tbl
{
  struct pip_tbl_key            key;
  u_int8_t                      pip_src_bmac[ETHER_ADDR_LEN];
  u_char                        pip_port_name[NSM_PBB_PORT_NAME_SIZ+1];
  u_int8_t                      pip_vip_map[NSM_PBB_VIP_PIP_MAP_LEN/8];
};

/* 802.1ah 12.16.4.3/4 virtual instance port (VIP) to 
   provider instance port (PIP) mapping table.*/
struct vip2pip_map 
{
  struct vip_tbl_key            key;
  /*need to add this parameter in the HLD/LLD*/
  u_int32_t                     pip_port_number; 
  u_int8_t                      pip_src_bmac[ETHER_ADDR_LEN];
  u_char                        storage_type[32+1]; 
/* This field tells in which type of memory the mapping 
   is stored. This should be stored in permanent memory.*/
  enum  nsm_pbb_service_status  status;
  enum  pbb_snmp_rowstatus      rowstatus;
 /* PIP Name is not added */
};

struct cnp_srv_tbl_key 
{
  u_int32_t                     sid;
  u_int32_t                     icomp_id;
  u_int32_t                     icomp_port_num;
};

/* list of CNPs for each [I-SID,I-comp] pair. 802.1ah/d3.5 12.16.6.1/2, item f
   can not find any equivalent of this in 802.1ap/D3.2 however we do need the
   information in this table. We will use this table internally but there is no
   equivalent MIB table to maintain */
struct cnp_srv_tbl 
{
  struct cnp_srv_tbl_key        key;
  enum nsm_pbb_service_interface_type  srv_type;
  enum nsm_pbb_service_status          srv_status;
  struct list                          *svlan_bundle_list; /* need to optimize this as a bitmap*/
  u_int32_t                            ref_port_num; /* PIP ifinidex */
  /* added one more parameter in the cnp service table to get 
   * pip/pnp interface index*/
  enum  pbb_snmp_rowstatus             rowstatus;
};

struct svlan_bundle 
{
/*u_int32_t                     range_index; A carry over from the depreciated 802.1ah/d3.5 version*/
  nsm_vid_t                       svid_low;
  nsm_vid_t                       svid_high;
  enum nsm_port_service_type      service_type;
  enum  nsm_pbb_service_status    bundle_status;
};
#endif /*HAVE_I_BEB*/

#ifdef HAVE_B_BEB
struct cbp_srvinst_key 
{
  u_int32_t                       bcomp_id;
  u_int32_t                       bcomp_port_num;
  u_int32_t                       b_sid;
};

/* CBP port service instance mapping table. 802.1ah 12.16.5.1/2*/
struct cbp_srvinst_tbl 
{
  struct cbp_srvinst_key          key;
  nsm_vid_t                       bvid; 
  u_int8_t                        default_dst_bmac[ETHER_ADDR_LEN] ;
  u_int16_t                       pt_mpt_type ;
  u_int32_t                       local_sid;
  u_int8_t                        edge;
  enum  nsm_pbb_service_status    status;
};
#endif /* HAVE_B_BEB */

/*  PBB API Proto Types need to be placed here */


int 
nsm_pbb_init(struct nsm_master *);

void  
nsm_pbb_deinit(struct nsm_master *nm);
/*
int
nsm_vlan_port_beb_add_entry(struct interface *ifp, struct nsm_beb_bridge *beb, 
                            enum nsm_vlan_port_mode mode);
*/
int
nsm_vlan_add_beb_port (struct interface *ifp, nsm_vid_t vid,
                       enum nsm_vlan_port_mode mode,
                       enum nsm_vlan_port_mode sub_mode,
                       bool_t iterate_members, bool_t notify_kernel);
#ifdef HAVE_I_BEB
struct cnp_srv_tbl * 
nsm_pbb_search_cnp_node(struct avl_tree *tree, u_int32_t icomp_id, 
                        u_int32_t isid, u_int32_t port_num);

struct pip_tbl *
nsm_pbb_search_pip_node(struct list *list, u_int32_t icomp_id, 
                        u_int32_t port_num);

struct vip_tbl *
nsm_pbb_search_vip_node(struct avl_tree *tree, u_int32_t icomp_id, 
                        u_int32_t port_num);

struct sid2vip_xref *
nsm_pbb_search_sid2vip_node(struct avl_tree *tree, u_int32_t icomp_id, 
                            u_int32_t isid);

struct vip2pip_map*
nsm_pbb_search_vip2pip_node(struct avl_tree *tree, u_int32_t icomp_id, 
                            u_int32_t port_num);

u_int32_t
nsm_pbb_get_vip_port_num(u_int8_t map[]);

void
nsm_pbb_set_pipvip_map(u_int8_t map[], u_int32_t port_num);

void
nsm_pbb_unset_pipvip_map(u_int8_t map[], u_int32_t vip_port_number);

char *
get_pbb_instance_name (u_int32_t isid, struct nsm_pbb_icomponent *icomp);

void
nsm_pbb_set_vip_default_mac (struct vip_tbl* vip_node ,u_int32_t isid);

int
nsm_vlan_port_config_vip(struct interface *ifp, u_int32_t isid,
                         u_char *instance_name, u_int8_t default_mac[],
                         enum nsm_port_service_type egress_type);

int 
nsm_vlan_port_beb_add_cnp_portbased(struct interface *ifp, 
                                    struct interface *ifp1, 
                                    u_int32_t isid, u_char *instance_name);

int 
nsm_vlan_port_beb_add_cnp_svlan_based(struct interface *ifp, 
                                      struct interface *ifp1,
                                      u_int32_t isid,u_char * instance_name,
                                      nsm_vid_t start_vid, nsm_vid_t end_vid);

int
nsm_vlan_port_beb_del_cnp_svlan_based (struct interface *ifp, u_int32_t isid);

char *
get_pbb_pip_portname(u_int32_t isid,struct nsm_pbb_icomponent *icomp);

int
nsm_vlan_port_beb_del_cnp_portbased(struct interface *ifp, u_int32_t isid);

int
nsm_vlan_port_beb_del_cnp_svlan(struct interface *ifp, u_int32_t isid,
                               nsm_vid_t start_vid, nsm_vid_t end_vid);

struct sid2vip_xref *
nsm_pbb_add_sid2vip_xref(struct nsm_bridge *bridge,
                         struct nsm_pbb_icomponent *icomp,
                         u_int32_t isid, u_int32_t vip_port_num);
struct vip2pip_map *
nsm_pbb_add_vip2pip_map(struct nsm_pbb_icomponent *icomp,
                        struct pip_tbl *pip_node,
                        u_int32_t vip_port_num);

int
nsm_vlan_port_add_pip (struct interface *ifp);

int
nsm_vlan_port_del_pip (struct interface *ifp);

int
nsm_vlan_port_beb_pip_mac_update (struct interface *ifp, u_int8_t src_bmac[]);

int 
nsm_pbb_create_isid (struct nsm_bridge_master *master, char *bridge_name, 
                     u_int32_t isid, char *isid_name);

int
nsm_pbb_delete_isid (struct nsm_bridge_master *master, char *bridge_name,
                     u_int32_t isid);
int
nsm_pbb_i_comp_deinit (struct nsm_bridge *bridge);

#endif /*HAVE_I_BEB*/


#ifdef HAVE_B_BEB

int  
nsm_vlan_port_beb_add_cbp_srv_inst(struct interface *ifp, u_int32_t isid, 
                                   u_int32_t isid_map, nsm_vid_t bvid, 
                                   enum nsm_vlan_port_mode vlan_type, 
                                   u_int8_t default_mac[], bool_t edge);


struct cbp_srvinst_tbl *
nsm_pbb_search_cbp_node(struct avl_tree *tree, u_int32_t port_num,
                        u_int32_t isid);

int 
nsm_vlan_port_beb_delete_cbp_srv_inst(struct interface *ifp, u_int32_t isid);

int 
nsm_vlan_port_add_pnp(struct interface *ifp, nsm_vid_t bvid);

int 
nsm_vlan_port_del_pnp(struct interface *ifp, nsm_vid_t bvid);
#endif /* HAVE_B_BEB */

/*
void
nsm_pbb_copy_mac(u_int8_t *, u_int8_t *);
*/
/*
int
nsm_vlan_port_beb_del_entry (struct interface *ifp, struct nsm_beb_bridge *beb,
                             enum nsm_vlan_port_mode mode);
*/
int
nsm_vlan_del_beb_port (struct interface *ifp, nsm_vid_t vid,
                       enum nsm_vlan_port_mode mode,
                       enum nsm_vlan_port_mode sub_mode,
                       bool_t iterate_members, bool_t notify_kernel);


int 
nsm_pbb_check_service(struct interface *ifp,u_int32_t instance_id, 
                      u_char *instance_name);

int 
nsm_pbb_dispatch_service(struct interface *ifp, u_int32_t isid, u_char *name);

int 
nsm_pbb_remove_service(struct interface *ifp, u_int32_t isid, u_char *name);

void  
nsm_bridge_beb_port(struct cli *cli, char *brname, 
                    enum nsm_vlan_port_mode mode);

void
nsm_bridge_beb_show(struct cli *cli, u_char *brname);


void
nsm_bridge_beb_vlan(struct cli *cli, u_char *name);

void
nsm_bridge_beb_service_show(struct cli *cli, u_char *instance_name, 
                            u_int32_t isid);

void
nsm_bridge_pbb_list_tables_debug (struct cli *cli, 
                                  char *brname, 
                                  u_int8_t table_type);

u_int32_t
nsm_pbb_if_config_exists(struct interface *ifp, enum nsm_vlan_port_mode mode);

u_int32_t 
nsm_pbb_if_vlan_config_exists (struct nsm_bridge* bridge, nsm_vid_t vid);

struct interface *
nsm_bridge_add_pbb_interface (struct nsm_bridge* bridge,
                              enum nsm_vlan_port_mode mode, char *ifname_l);


int
pbb_send_logical_interface_event (char *br_name, struct interface *ifp,
                                  bool_t add);

#if defined HAVE_I_BEB
int
nsm_pbb_add_pip_node (struct nsm_pbb_icomponent *icomp, u_int32_t icomp_id,
                      u_int32_t port_num);

struct vip_tbl *
nsm_pbb_search_vip_by_isid (struct nsm_pbb_icomponent *icomp,
                            u_int32_t isid, u_int32_t icomp_id);
#endif /* HAVE_I_BEB */

int
nsm_pbb_add_logical_interfaces (struct nsm_bridge *bridge);

int
nsm_pbb_delete_logical_interfaces (struct nsm_bridge *bridge);

int
nsm_pbb_ib_bridge_init (struct nsm_bridge *bridge);

int
nsm_pbb_ib_bridge_deinit (struct nsm_bridge *bridge);



#if defined HAVE_I_BEB
u_int32_t
nsm_pbb_search_isid_by_instance_name (char *instance_name, 
                                      struct nsm_pbb_icomponent *icomp);
#endif /* HAVE_I_BEB */

/* This Macro can be used when all the node in the avl_tree must be deleted
 * while looping through the avl_tree */
#define AVL_LOOP_DELETE(T,V,N,NN) \
  if (T) \
    for ((N) = avl_top (T), NN = ((N)!=NULL) ? avl_next(N) : NULL;  \
         (N);                                                       \
        (N) = (NN), NN = ((N)!=NULL) ? avl_getnext(T, N) : NULL)         \
       if (((V) = (N)->info) != NULL)

#endif /* __HAVE_BRIDGE_PBB_H__ */
