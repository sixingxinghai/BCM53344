/* Copyright (C) 2001-2003  All Rights Reserved.  */

/* mpls.c */
int init_module (void);  
void cleanup_module (void); 
int remove_handler (void);

/* mpls_fib.c */
struct ftn_entry *allocate_ftn (struct mpls_table *, struct mpls_prefix *,
                                 uint8 *, u_char, int);
struct ilm_entry *allocate_ilm (struct mpls_table *, struct mpls_prefix *,
                                uint8 *);
sint8 fib_add_ftnentry (struct mpls_table *, uint8, struct mpls_prefix *,
                        L3ADDR, uint32, LABEL, sint8, uint32, uint32,
                        struct mpls_owner_fwd *, uint32, u_char, int);
struct ftn_entry *fib_best_ftnentry (struct mpls_table *, struct sk_buff *,
                                     uint32, uint8, uint8);
struct ftn_entry *fib_find_ftnentry(struct mpls_table *, uint8,
                                    struct mpls_prefix *, uint32, int, int *);
sint8 fib_add_ilmentry (uint8, LABEL, uint32, struct mpls_prefix *,
                        L3ADDR, uint32, LABEL, sint8, uint32,
                        struct mpls_owner_fwd *);
struct ilm_entry *fib_find_ilmentry(struct mpls_table *, uint8, LABEL,
                                    uint32, int, int *);
sint8 fib_config(void);
void fib_delete_ftn (struct ftn_entry *);
struct ftn_entry *fib_ftn_new (uint8 *);

/* mpls_io.c */
int mpls_rcv(struct sk_buff *, struct net_device *, struct packet_type *, 
             struct net_device *);
int l2_process(struct sk_buff *, struct net_device *, struct packet_type *,
               struct mpls_interface *, struct net_device *);
void fwd_forward_frame(struct sk_buff *, struct net_device *,
                       struct packet_type *, struct mpls_interface *,
                       struct net_device *);
int mpls_forward(struct sk_buff *);
sint8 forwarder_process_nhlfe(struct sk_buff **, struct net_device *,
                              struct packet_type *, struct nhlfe_entry *,
                              struct mpls_interface *, uint32, uint32 *);
struct nhlfe_entry *forwarder_find_best_nhlfe(struct nhlfe_entry *);
int mpls_pre_forward (struct sk_buff *, struct rtable *, uint8);
int check_and_update_ttl (struct sk_buff *, struct shimhdr*, uint8);
int mpls_nhlfe_process_opcode (struct sk_buff **, struct nhlfe_entry *,
                               uint8, uint8, uint8, int);
int mpls_resolve_route (sint32, uint8, uint32);
int mpls_tcp_output (struct sk_buff **);
int push_label_out (struct sk_buff *, LABEL, uint8);
int mpls_pop_and_fwd_vc_label (struct sk_buff *, struct rtable *,
                               struct shimhdr*);
struct rtable *mpls_push_vc_label (struct sk_buff **, struct ftn_entry *);
int mpls_vc_rcv (struct sk_buff *, struct net_device *, struct ftn_entry *,
                 struct packet_type *);
void set_mpls_shim_net (struct shimhdr *, u32, u32, u32, u32);

/* mpls_proc.c */
struct proc_dir_entry *mpls_proc_net_create_ilm();
struct proc_dir_entry *mpls_proc_net_create_ftn();
struct proc_dir_entry *mpls_proc_net_create_vrf();
struct proc_dir_entry *mpls_proc_net_create_labelspace();
struct proc_dir_entry *mpls_proc_net_create_vc();
struct proc_dir_entry *mpls_proc_net_create_stats();
struct proc_dir_entry *mpls_proc_net_create_ftn_stats();
struct proc_dir_entry *mpls_proc_net_create_ilm_stats();
struct proc_dir_entry *mpls_proc_net_create_nhlfe_stats();
struct proc_dir_entry *mpls_proc_net_create_tunnel_stats();
struct proc_dir_entry *mpls_proc_net_create_if_stats();

/* mpls_utils.c */
int  mpls_initialize_fib_handle (void);
void mpls_destroy_fib_handle (void);
void mpls_destroy_ilm(struct mpls_table *, uint8);
void mpls_destroy_ftn(struct mpls_table *, uint8);
void mpls_cleanup_ok (void);
void set_mpls_refcount (uint8);
void unset_mpls_refcount (uint8);
uint32 mpls_if_key (struct mpls_interface *);
int mpls_if_cmp (struct mpls_interface *, struct mpls_interface *);
struct mpls_interface * mpls_if_create (uint32, uint16, uint8);
struct mpls_interface * mpls_if_lookup (uint32);
struct mpls_interface * mpls_if_update_vrf (uint32, int, int *);
void mpls_if_delete (struct mpls_interface *);
void mpls_if_delete_only (struct mpls_interface *);
struct mpls_table * mpls_vrf_create (int);
struct mpls_table * mpls_vrf_lookup_by_id (int, int);
void mpls_vrf_delete (int);
void mpls_vrf_delete_all (struct mpls_table *);
void mpls_if_unbind_vrf (struct mpls_hash_bucket *, void *);
void dst_data_init (uint32, struct net_device *, uint8, uint8);
int is_interface_specific (uint32);
int rt_output (struct sk_buff *);
struct rtable *rt_new (struct dst_ops *, uint32, struct net_device *,
                       int (*) (struct sk_buff *));
void rt_free (struct rtable *);
void rt_set (struct rtable *, struct dst_ops *, uint32,
              struct net_device *, int (*) (struct sk_buff *));
int dst_neigh_output (struct sk_buff *);
void mpls_if_ilm_unbind (struct mpls_interface *);
void mpls_ilm_clean_all (uint8);
void mpls_ilm_delete_all (uint8);
struct rtable *rt_make_copy (struct rtable *);
int rt_add (struct rtable *);
void rt_cleanup (unsigned long);
void rt_start_timer (void);
void rt_purge_list (uint8);
uint32 get_if_addr (struct net_device *, uint8);
struct net_device *get_dev_by_addr (uint32);
int mpls_if_vc_bind (struct net_device *, uint32);
int mpls_if_vc_unbind (struct net_device *, uint32);
int mpls_if_add_vc_ftnentry (uint8, uint32, LABEL, L3ADDR,
                             struct mpls_interface *, struct net_device *,
                             uint8, uint32);
int mpls_if_del_vc_ftnentry (struct mpls_interface *, uint8);
void mpls_vrf_clean_all (uint8);
void mpls_fib_handle_clear_stats (struct fib_handle *);
void mpls_ftn_entry_clear_stats (struct fib_handle *, struct ftn_entry *);
void mpls_ilm_entry_clear_stats (struct fib_handle *, struct ilm_entry *);

/* mpls_netlink.c */
void mpls_netlink_rcv_func (struct sk_buff *);
int  mpls_netlink_rcv_and_process_skb (struct sk_buff *);
int  mpls_netlink_process_message_type (struct nlmsghdr *, struct sk_buff *);
int  mpls_netlink_usock_mpls_ctrl (struct nlmsghdr *, struct sk_buff *, int);
int  mpls_netlink_usock_mpls_if_ctrl (struct nlmsghdr *, int);
int  mpls_netlink_usock_mpls_vc_ctrl (struct nlmsghdr *, int);
int  mpls_netlink_usock_mpls_vrf_ctrl (struct nlmsghdr *, int);
int  mpls_netlink_usock_mpls_if_update (struct nlmsghdr *);
int  mpls_netlink_usock_clean_fib (struct nlmsghdr *);
int  mpls_netlink_usock_clean_vrf (struct nlmsghdr *);
int  mpls_netlink_usock_newilm(struct nlmsghdr *);
int  mpls_netlink_usock_delilm(struct nlmsghdr *);
int  mpls_netlink_usock_newftn(struct nlmsghdr *);
int  mpls_netlink_usock_delftn(struct nlmsghdr *);
int  mpls_netlink_usock_new_vc_ftn (struct nlmsghdr *);
int  mpls_netlink_usock_del_vc_ftn (struct nlmsghdr *);
int  mpls_netlink_usock_ttl_handling (struct nlmsghdr *);
int  mpls_netlink_usock_local_pkt_handling (struct nlmsghdr *);
int  mpls_netlink_usock_debug_handling (struct nlmsghdr *);
int  mpls_netlink_usock_stats_clear_handling (struct nlmsghdr *);
void mpls_netlink_sock_close (void);
struct sock *mpls_netlink_sock_create (void);

/* MPLS OAM functions */
int mpls_netlink_usock_bypass_tunnel_send (struct nlmsghdr *);
void mpls_send_pkt_over_bypass (int, int, char *);
void
mpls_send_pkt_for_oam (int ftn_id, int pkt_len, char *data, 
                       int vrf_id, int flags, int ttl);
struct sk_buff *
mpls_form_skbuff(int pkt_len, char *data);
void
mpls_send_pkt_for_oam_vc (int vc_id, int pkt_len, char *data, 
                          int ifindex, int cc_type, int cc_input,
                          int flags, int ttl);
struct mpls_table *
mpls_oam_get_table (int vrf_id);
struct ftn_entry *
mpls_oam_get_ftn_entry (int ftn_id, struct mpls_table *table);
int
mpls_oam_do_lookup_fwd (struct sk_buff *sk, struct ftn_entry *ftnp, 
                        int ttl, int flags);
int
mpls_oam_do_second_lookup_fwd (struct ftn_entry *ftnp, int ftn_id, 
                               struct sk_buff *sk,int ttl,int8_t for_vpn );
struct ftn_entry *
mpls_oam_get_vc_ftn_entry (uint32 vc_id, uint32 ifindex);

void
mpls_oam_packet_process (struct sk_buff *, struct mpls_interface *,
                         uint8, uint8, uint32, uint32 *);
