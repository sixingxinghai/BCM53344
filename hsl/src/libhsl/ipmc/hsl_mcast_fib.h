/* Copyright (C) 2005   All Rights Reserved. */

#ifndef __HSL_MCAST_DB_H
#define __HSL_MCAST_DB_H 

#if defined  HAVE_MCAST_IPV4 || defined HAVE_IGMP_SNOOP || defined HAVE_MCAST_IPV6 || defined HAVE_MLD_SNOOP
/* 
  Protection macros. 
*/
#define HSL_MCAST_V4_DB_LOCK         hsl_mc_fib_tree_request(AF_INET, NULL)
#define HSL_MCAST_V4_DB_UNLOCK       hsl_mc_fib_tree_release(AF_INET)

#define HSL_MCAST_V6_DB_LOCK         hsl_mc_fib_tree_request(AF_INET6, NULL)
#define HSL_MCAST_V6_DB_UNLOCK       hsl_mc_fib_tree_release(AF_INET6)

/* IGMP Snooping version l2 port learned from. */ 
#define HSL_MCAST_FIB_INHERITED         (1 << 0)
#define HSL_MCAST_FIB_ACTUAL            (1 << 1)


#define HSL_MCAST_FIB_V4_GS_TREE         (p_hsl_ipv4_mc_db->mc_fib_tree)
#define HSL_MCAST_FIB_V6_GS_TREE         (p_hsl_ipv6_mc_db->mc_fib_tree)

/* L2/L3 interfaces tree */
#define HSL_MCAST_FIB_VLAN_TREE         (p_grp_src_entry->l2_vlan_tree)
#define HSL_MCAST_FIB_L3_IF_TREE        (p_grp_src_entry->l3_if_tree)
#define HSL_MCAST_FIB_L2_IF_TREE        (p_vlan_entry->l2_if_tree)

/* FIB operation Add/Delete */
#define HSL_MCAST_FIB_ADD            (HSL_TRUE)
#define HSL_MCAST_FIB_DEL            (HSL_FALSE)

#define HSL_MCAST_FIB_SRC            (p_grp_src_entry->src)
#define HSL_MCAST_FIB_GROUP          (p_grp_src_entry->group)

#define HSL_MCAST_FIB_SRC_FAMILY     ((p_grp_src_entry->src).family)
#define HSL_MCAST_FIB_GROUP_FAMILY   ((p_grp_src_entry->group).family)

#define HSL_MCAST_FIB_SRC4           (p_grp_src_entry->src.u.prefix4)
#define HSL_MCAST_FIB_GROUP4         (p_grp_src_entry->group.u.prefix4)

#ifdef HAVE_IPV6 
#define HSL_MCAST_FIB_SRC6           (p_grp_src_entry->src.u.prefix6)
#define HSL_MCAST_FIB_GROUP6         (p_grp_src_entry->group.u.prefix6)
#endif /* HAVE_IPV6 */

#ifdef  HAVE_MCAST_IPV4 
/* 
   Multicast interface manager. 
*/
#define HSL_MAX_VIFS                                  (100) 
#define HSL_VIFF_TUNNEL                               (0x1)
#define HSL_VIFF_REGISTER                             (0x4)


#define HSL_IPV4_MC_TTL_MIN                             (0)
#define HSL_IPV4_MC_TTL_MAX                           (255)
#define HSL_MC_TTL_VALID(ttl) (((ttl) > HSL_IPV4_MC_TTL_MIN) && \
                              (((ttl) < HSL_IPV4_MC_TTL_MAX)))

#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6

#define HSL_IPV6_MAX_VIFS                              (100) 
#define HSL_IPV6_VIFF_REGISTER                         (0x1)

#endif /* HAVE_MCAST_IPV6 */


struct hsl_mc_vif
{
  hsl_ifIndex_t ifindex;             /* Physical interface ifindex. */
  u_int32_t     flags;               /* Vif flags.                  */ 
  HSL_BOOL      valid;               /* Flag if entry is valid.     */
};



/* 
   Multicast outgoing interface entry. 
     data: 
       vid - Vlan id.
       l2_if_tree - tree of layer 2 ifindexes igmp join was received from.
*/

struct hsl_mc_vlan_entry
{
   /* Vlan id.*/ 
   hsl_vid_t vid;         

   /* Tree of joined  L2 interfaces.*/ 
   struct hsl_avl_tree *l2_if_tree;
};

/* 
   Multicast outgoing interface entry. 
     data: 
       ifindex  - Interface index.
     l3 interface: 
        ttl      - Minimum ttl required in the packet to be forwarded.  
     l2_interface
        flags - Entry flags .
*/
struct hsl_mc_eggress_if_entry 
{
   /*  Outgoing interface. */ 
   hsl_ifIndex_t ifindex;

   /* Filter mode */
   u_char is_exclude;

   union
   {
     struct
     {
       /*  TTL. */
       u_char ttl;
     } l3;

     struct
     {
       /* Flags. */ 
       u_char flags;
     } l2;
  } u;
};


/* 
   IPV4 Multicast entry. 
     key (S,G) 
     data: 
       l2_vlan_tree - tree of igmp joins, sorted by vlan + list of layer 2 ifindexes.   
       l3_if_tree - tree of layer 3 downstream ifindexes mc route contains.
*/
struct hsl_mc_fib_entry 
{
   /* Multicast group.*/
   hsl_prefix_t group; 

   /* Multicast traffic source.*/
   hsl_prefix_t src;  

   /* Flags */
   u_char flags;
#define HSL_MC_FIB_L2_ENTRY      (1 << 0)
#define HSL_MC_FIB_L3_ENTRY      (1 << 1)
#define HSL_MC_FIB_IIF_L2_ENTRY  (1 << 2)
#define HSL_MC_FIB_IN_OS         (1 << 3)
#define HSL_MC_FIB_OS_ONLY_ENTRY (1 << 4)

   /*  Registered vif */
   HSL_BOOL  mirror_to_cpu;
  
   /*  Incoming interface. */ 
   hsl_ifIndex_t iif;

   /*  Incoming L2 interface for SVI. */ 
   hsl_ifIndex_t l2_iif;

   /*  Previous Incoming interface. */ 
   /* workaround for broadcom doesn't support route replacement. */
   hsl_ifIndex_t prev_iif;

   /* Counter of added entries (Inherited entries are not included.)*/
   u_int16_t  actual_joins_cntr;
  
   /* Old entry update flag */
   HSL_BOOL do_hw_update;

   /* Tree of joined  L2 interfaces.*/ 
   struct hsl_avl_tree *l2_vlan_tree;

   /* Tree of protocol configured L3 interfaces.*/
   struct hsl_avl_tree *l3_if_tree;
};

/*
    V4 Multicast database. 
    Contains:  
      1) Protection mutex
      2) Registered vif id
      3) Flag pim is running.
      4) Vif database.
      5) TCP/IP stack calbacks. 
      6) Hardware specific callbacks.
      7) IPv4 entries tree.
 */ 
struct hsl_ipv4_mc_db
{
  apn_sem_id             mutex;                   /* Protection mutex.   */ 
#ifdef HAVE_MCAST_IPV4
  u_int32_t              registered_vif;          /* Registered vif id.  */
  HSL_BOOL               pim_running;             /* pim is running flag */
  struct hsl_mc_vif      vif_entry[HSL_MAX_VIFS]; /* Vif database.       */
  struct hsl_fib_mc_os_callbacks *os_cb;          /* Callbacks for OS.   */
#endif /* HAVE_MCAST_IPV4 */
  struct hsl_fib_mc_hw_callbacks *hw_cb;          /* Callbacks for HW.   */
  struct hsl_avl_tree    *mc_fib_tree;            /* G,S ipv4 tree.      */
};

/*
    V6 Multicast database. 
    Contains:  
      1) Protection mutex
      2) Registered vif id
      3) Flag pim is running.
      4) Vif database.
      5) TCP/IP stack calbacks. 
      6) Hardware specific callbacks.
      7) IPv6 entries tree.
 */ 

struct hsl_ipv6_mc_db
{
  apn_sem_id             mutex;                   /* Protection mutex.   */ 
#ifdef HAVE_MCAST_IPV6
  u_int32_t              registered_vif;               /* Registered vif id.  */
  HSL_BOOL               pim_running;                  /* pim is running flag */
  struct hsl_mc_vif      vif_entry[HSL_IPV6_MAX_VIFS]; /* Vif database.       */
  struct hsl_fib_ipv6_mc_os_callbacks *os_cb;          /* Callbacks for OS.   */
#endif /* HAVE_MCAST_IPV6 */
  struct hsl_fib_ipv6_mc_hw_callbacks *hw_cb;          /* Callbacks for HW.   */
  struct hsl_avl_tree    *mc_fib_tree;                 /* G,S ipv6 tree.      */
};


#ifdef HAVE_IGMP_SNOOP

/* Add mac address to ipv4 multicast entry.
Name: hsl_mc_v4_igs_add_entry 
Parameters:
  IN -> bridge_name - Bridge name.     
  IN -> src   - Mcast Source address.     
  IN -> group - Mcast Group address.
  IN -> vid   - Vlan id.
  IN -> count - Number of ifindexes. 
  IN -> ifindexes - Array of ifindexes.

Returns:
  0 on success
  < 0 on error
*/
int
hsl_mc_v4_igs_add_entry(char *bridge_name, hsl_ipv4Address_t src,
                        hsl_ipv4Address_t group, u_int16_t vid,
                        u_int16_t count, u_int32_t *ifindexes, char is_exclude);

/*  Delete mac address from ipv4 multicast entry.

Name: hsl_mc_v4_igs_del_entry 
Parameters:
  IN -> bridge_name - Bridge name.     
  IN -> src   - Mcast Source address.     
  IN -> group - Mcast Group address.
  IN -> vid   - Vlan id.
  IN -> count - Number of ifindexes. 
  IN -> ifindexes - Array of ifindexes.

Returns:
0 on success
< 0 on error
*/

int
hsl_mc_v4_igs_del_entry(char *bridge_name, hsl_ipv4Address_t src,
                        hsl_ipv4Address_t group, u_int16_t vid,
                        u_int16_t count, u_int32_t *ifindexes, char is_exclude);


#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP

/* Add mac address to ipv4 multicast entry.
Name: hsl_mc_v6_mlds_add_entry.
Parameters:
  IN -> bridge_name - Bridge name......
  IN -> src   - Mcast Source address......
  IN -> group - Mcast Group address.
  IN -> vid   - Vlan id.
  IN -> count - Number of ifindexes..
  IN -> ifindexes - Array of ifindexes.

Returns:
  0 on success
  < 0 on error
*/
int
hsl_mc_v6_mlds_add_entry(char *bridge_name, hsl_ipv6Address_t *src,
                         hsl_ipv6Address_t *group, u_int16_t vid,
                         u_int16_t count, u_int32_t *ifindexes, char is_exclude);

/*  Delete mac address from ipv4 multicast entry.

Name: hsl_mc_v6_mlds_del_entry.
Parameters:
  IN -> bridge_name - Bridge name......
  IN -> src   - Mcast Source address......
  IN -> group - Mcast Group address.
  IN -> vid   - Vlan id.
  IN -> count - Number of ifindexes..
  IN -> ifindexes - Array of ifindexes.

Returns:
0 on success
< 0 on error
*/

int
hsl_mc_v6_mlds_del_entry(char *bridge_name, hsl_ipv6Address_t *src,
                        hsl_ipv6Address_t *group, u_int16_t vid,
                        u_int16_t count, u_int32_t *ifindexes, char is_exclude);

#endif

#ifdef HAVE_MCAST_IPV4
/* Add multicast ipv4 route 
  Name: hsl_mc_v4_route_add
  Parameters:
    IN -> src        - Mcast Source address.     
    IN -> group      - Mcast Group address.
    IN -> iif_vif    - Incoming interface.
    IN -> num_olist  - Number of ifindexes. 
    IN -> olist_ttls - Array of TTLs.

  Returns:
    0 on success
    < 0 on error
*/
int hsl_mc_v4_route_add(hsl_ipv4Address_t src,hsl_ipv4Address_t group,
                    u_int32_t iif_vif,u_int32_t num_olist,u_char *olist_ttls);


/* Delete multicast ipv4 route 
  Name: hsl_mc_v4_route_del
  Parameters:
    IN -> src        - Mcast Source address.     
    IN -> group      - Mcast Group address.

  Returns:
    0 on success
    < 0 on error
*/
int hsl_mc_v4_route_del(hsl_ipv4Address_t src, hsl_ipv4Address_t group);
/* Get multicast ipv4 route usage statistics 
   Name: hsl_mc_v4_sg_stat
   Parameters:
     IN -> src        - Mcast Source address.     
     IN -> group      - Mcast Group address.

   Returns:
     0 on success
     < 0 on error
*/
int
hsl_mc_v4_sg_stat (hsl_ipv4Address_t src,hsl_ipv4Address_t group, u_int32_t iif_vif,
                   u_int32_t *pktcnt,u_int32_t *bytecnt, u_int32_t *wrong_if);

/* Register incoming L2 interface for SVI iif 
  Name: hsl_mc_v4_fib_register_iif_l2_port
  Parameters:
    IN -> l3_hdr_ptr - IPv4 header pointer.     
    IN -> ifp        - L3 incoming interface.
    IN -> l2_ifp     - L2 incoming port.

  Returns:
    None.
*/
void
hsl_mc_v4_fib_register_iif_l2_port (void *l3_hdr_ptr, struct hsl_if *ifp,
    struct hsl_if *l2_ifp);

#endif /* HAVE_MCAST_IPV4 */

/* Get vlan multicast entry.
   Name: hsl_mc_fib_vlan_entry_get
   Parameters:
     IN -> p_grp_src_entry - G,S entry vlan being added to.
     IN -> vid - vlan id.
   Returns:
     NULL if vlan entry was not found.
     vlan_entry if found.
*/
struct hsl_mc_vlan_entry *
hsl_mc_fib_vlan_entry_get(struct hsl_mc_fib_entry *p_grp_src_entry, hsl_vid_t vid);

/* Log hsl_mcast fib message
  Name: _hsl_mc_fib_log_msg
  Parameters:
    IN ->level - Log level;  
    IN -> p_grp_src_entry - grp source information.
    IN -> message. 
  Returns:
    void
*/
void
hsl_mc_fib_log_msg(u_int16_t level, struct hsl_mc_fib_entry *p_grp_src_entry, const char* format, ...);

/* Check if pim is running for specific address family
  Name: hsl_mc_is_pim_running
  Parameters:
    IN -> family - Address family ;  
  Returns:
     TRUE/FALSE pim running.
*/
HSL_BOOL 
hsl_mc_is_pim_running(int family);

int hsl_ipv4_mc_db_init(void);
int hsl_ipv4_mc_db_deinit(void);
int hsl_ipv4_mc_init(void);
int hsl_ipv4_mc_deinit(void);
int hsl_ipv4_mc_pim_init(void);
int hsl_ipv4_mc_pim_deinit(void);
int
hsl_ipv4_mc_route_add(struct hsl_mc_fib_entry *p_grp_src_entry, 
                      u_int32_t iif_vif,u_int32_t num_olist,u_char *olist_ttls);

int hsl_ipv4_mc_route_del(struct hsl_mc_fib_entry *p_grp_src_entry);

#ifdef HAVE_MCAST_IPV4
int hsl_ipv4_mc_sg_stat (struct hsl_mc_fib_entry *p_grp_src_entry,
                         u_int32_t *pktcnt,u_int32_t *bytecnt, u_int32_t *wrong_if);



int hsl_mc_ipv4_get_vif_ifindex(u_int32_t vif_index,hsl_ifIndex_t *ifindex);
int hsl_ipv4_mc_vif_add(u_int32_t vif_index,
                        hsl_ifIndex_t ifindex,
                        hsl_ipv4Address_t loc_addr,
                        hsl_ipv4Address_t rmt_addr,
                        u_int16_t flags);
int hsl_ipv4_mc_vif_del(u_int32_t vif_index);
int hsl_ipv4_get_registered_vif();

int hsl_ipv4_mc_os_cb_reg   (struct hsl_fib_mc_os_callbacks *cb);
int hsl_ipv4_mc_os_cb_unreg   (void);
int hsl_ipv4_flush_vif_db();
#endif /* HAVE_MCAST_IPV4 */
int hsl_ipv4_mc_hw_cb_reg   (struct hsl_fib_mc_hw_callbacks *cb);
int hsl_ipv4_mc_hw_cb_unreg   (void);
                                   
#ifdef HAVE_IPV6
int hsl_ipv6_mc_db_init(void);
int hsl_ipv6_mc_db_deinit(void);
int hsl_ipv6_mc_init(void);
int hsl_ipv6_mc_deinit(void);
int hsl_ipv6_mc_pim_init(void);
int hsl_ipv6_mc_pim_deinit(void);
int hsl_ipv6_mc_route_add(struct hsl_mc_fib_entry *p_grp_src_entry, 
                          u_int32_t iif_vif,u_int32_t num_olist,
                          u_int16_t *olist);
int hsl_ipv6_mc_route_del(struct hsl_mc_fib_entry *p_grp_src_entry);

#ifdef HAVE_MCAST_IPV6
int hsl_ipv6_mc_sg_stat (struct hsl_mc_fib_entry *p_grp_src_entry,
    u_int32_t *pktcnt,u_int32_t *bytecnt, 
    u_int32_t *wrong_if);

int hsl_mc_ipv6_get_vif_ifindex(u_int32_t vif_index,hsl_ifIndex_t *ifindex);
int hsl_ipv6_mc_vif_add(u_int32_t vif_index,
                        hsl_ifIndex_t ifindex,
                        u_int16_t flags);
int hsl_ipv6_mc_vif_del(u_int32_t vif_index);
int hsl_ipv6_get_registered_vif();
int hsl_ipv6_mc_os_cb_reg   (struct hsl_fib_ipv6_mc_os_callbacks *cb);
int hsl_ipv6_mc_os_cb_unreg   (void);
int hsl_ipv6_flush_vif_db();
int hsl_mc_v6_route_add(hsl_ipv6Address_t *src,hsl_ipv6Address_t *group,
                        u_int32_t iif_vif,u_int32_t num_olist,u_int16_t *olist);
int hsl_mc_v6_route_del(hsl_ipv6Address_t *src, hsl_ipv6Address_t *group);
int hsl_mc_v6_sg_stat (hsl_ipv6Address_t *src,hsl_ipv6Address_t *group, u_int32_t iif_vif,
                       u_int32_t *pktcnt,u_int32_t *bytecnt, u_int32_t *wrong_if);

/* Register incoming L2 interface for SVI iif 
  Name: hsl_mc_v6_fib_register_iif_l2_port
  Parameters:
    IN -> l3_hdr_ptr - IPv6 header pointer.     
    IN -> ifp        - L3 incoming interface.
    IN -> l2_ifp     - L2 incoming port.

  Returns:
    None.
*/
void
hsl_mc_v6_fib_register_iif_l2_port (void *l3_hdr_ptr, struct hsl_if *ifp,
    struct hsl_if *l2_ifp);
#endif /* HAVE_MCAST_IPV6 */
int hsl_ipv6_mc_hw_cb_reg   (struct hsl_fib_ipv6_mc_hw_callbacks *cb);
int hsl_ipv6_mc_hw_cb_unreg   (void);
#endif /* HAVE_IPV6 */


#ifdef HAVE_L2
/* Delete layer 2 multicast route.
  Name: hsl_l2_mc_route_delete
  Parameters:
    IN -> p_grp_src_entry - G,S entry information;  
    IN -> vid  - Removed vlan id;  
  Returns:
     TRUE/FALSE pim running.
*/
int 
hsl_l2_mc_route_delete(struct hsl_mc_fib_entry *p_grp_src_entry, hsl_vid_t vid);
#endif /* HAVE_L2 */

void 
hsl_mc_fib_entry_free(void *ptr);

#endif /* HAVE_MCAST_IPV4 || HAVE_IGMP_SNOOP || HAVE_MCAST_IPV6 || HAVE_MLD_SNOOP */

#endif /* __HSL_MCAST_FIB_H  */

