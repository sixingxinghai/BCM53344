/* Copyright (C) 2005  All Rights Reserved. */

#ifndef _HSL_MPLS_H_
#define _HSL_MPLS_H_

#define HSL_MPLS_IF_KEY_LEN          32
#define HSL_MPLS_ILM_KEY_LEN         52

#define HSL_MPLS_VPN_MARTINI     1
#define HSL_MPLS_VPN_VPLS        2
#define HSL_MPLS_VPN_VRF         3

#define HSL_MPLS_LABEL_VALUE_INVALID 1048576 

#define HSL_MPLS_DEFAULT_TTL_VALUE   255 /*default TTL Value for MPLS Header*/

/* Shift and convert to network-byte order. */
#define HSL_PREP_FOR_NETWORK(val,v,l)               \
{                                               \
  val = v;                                      \
  val = htonl (((val) << (IPV4_MAX_BITLEN - l))); \
}

struct hsl_mpls_ilm_key
{
  u_int32_t in_ifindex;
  u_int32_t in_label;
};


struct hsl_mpls_ilm_entry
{
  u_int32_t in_label;
  struct hsl_if *in_ifp;
  u_int32_t swap_label;
  u_int32_t tunnel_label;
  struct hsl_nh_entry *nh_entry;
  struct hsl_if *out_ifp;
  u_int32_t vpn_id;
  u_char opcode;

  #define HSL_MPLS_ILM_INSTALLED    (1 << 0)
  u_char flags;
  struct hsl_mpls_ilm_entry *nh_ilm_next;

#ifdef HAVE_MPLS_VC 
  struct hsl_mpls_vpn_vc *vc;
#endif /* HAVE_MPLS_VC */
};


/*
  MPLS fib table.
*/
struct hsl_mpls_table
{
  struct hsl_ptree *mpls_ilm_table;

  struct hsl_ptree *mpls_if_table;

  struct hsl_ptree *mpls_vpn_table;

  /* Protection semaphore */
  // for compile
  //ipi_sem_id mutex;

  struct hsl_mpls_hw_callbacks *hw_cb;       /* Callbacks for HW. */
};

#ifdef HAVE_MPLS_VC
struct hsl_mpls_vpn_vc
{
  u_char vc_type;
  u_int32_t vc_id;
  u_int32_t vc_peer_addr;
  u_int32_t vc_peer_nhop;
  u_int32_t vc_ftn_label;

  struct hsl_mpls_ilm_entry *vc_ilm;
  struct hsl_mpls_vpn_entry *ve;

  struct hsl_nh_entry *vc_nh_entry;
  struct hsl_mpls_vpn_vc *vc_next;
  struct hsl_mpls_vpn_vc *nh_vc_next;
};

#endif /* HAVE_MPLS_VC */

#define HSL_MPLS_VPN_KEY_LEN                  32

struct hsl_mpls_vpn_entry
{
  u_char vpn_type;
  u_int32_t vpn_id;
  u_int32_t vlan_id;
  u_int16_t vc_count;
  u_int16_t port_count;

  #define HSL_MPLS_VPN_RSVD_VLAN     (1 << 0)
  u_char flags;
#ifdef HAVE_MPLS_VC
  struct hsl_mpls_vpn_vc *vc_list;
#endif /* HAVE_MPLS_VC */
  struct hsl_mpls_vpn_port *port_list;
};

struct hsl_mpls_vpn_port
{
  u_int32_t port_id;
  u_int16_t port_vlan_id;
  struct hsl_mpls_vpn_port *next;
};


/* 
   MPLS HW callbacks.
*/
struct hsl_mpls_hw_callbacks
{
  int (*hw_mpls_ftn_add) (struct hsl_route_node *rnp, struct hsl_nh_entry *nh);
  int (*hw_mpls_ftn_del) (struct hsl_route_node *rnp, struct hsl_nh_entry *nh);
  int (*hw_mpls_ilm_add) (struct hsl_mpls_ilm_entry *ilm, struct hsl_nh_entry *nh);
  int (*hw_mpls_ilm_del) (struct hsl_mpls_ilm_entry *ilm);

  int (*hw_mpls_vpn_add) (struct hsl_mpls_vpn_entry *);
  int (*hw_mpls_vpn_del) (struct hsl_mpls_vpn_entry *);
  int (*hw_mpls_vpn_if_bind) (struct hsl_mpls_vpn_entry *, 
                              struct hsl_mpls_vpn_port *);
  int (*hw_mpls_vpn_if_unbind) (struct hsl_mpls_vpn_entry *, 
                                struct hsl_mpls_vpn_port *);
#ifdef HAVE_MPLS_VC
  int (*hw_mpls_vpn_vc_ftn_add) (struct hsl_mpls_vpn_vc *, struct hsl_nh_entry *nh);
  int (*hw_mpls_vpn_vc_ftn_del) (struct hsl_mpls_vpn_vc *);
#endif /* HAVE_MPLS_VC */
};


int hsl_mpls_init ();
int hsl_mpls_deinit ();
int hsl_mpls_ftn_add (struct hal_msg_mpls_ftn_add *);
int hsl_mpls_ftn_del (struct hal_msg_mpls_ftn_del *);
int hsl_mpls_ilm_add (struct hal_msg_mpls_ilm_add *, u_int32_t, u_int32_t);
int hsl_mpls_ilm_del (struct hal_msg_mpls_ilm_del *);
int hsl_mpls_ftn_add_to_hw (struct hsl_route_node *, struct hsl_nh_entry *);
int hsl_mpls_ftn_del_from_hw (struct hsl_route_node *, struct hsl_nh_entry *);
int hsl_mpls_ilm_add_to_hw (struct hsl_mpls_ilm_entry *, struct hsl_nh_entry *);
int hsl_mpls_ilm_del_from_hw (struct hsl_mpls_ilm_entry *);
#ifdef HAVE_MPLS_VC
struct hsl_mpls_vpn_vc *hsl_mpls_vpn_vc_create (struct hsl_mpls_vpn_entry *, 
                                                u_int32_t, u_int32_t, u_char);
void hsl_mpls_vpn_vc_free (struct hsl_mpls_vpn_vc *);
struct hsl_mpls_vpn_vc *hsl_mpls_vpn_vc_lookup (struct hsl_mpls_vpn_entry *,
                                                u_int32_t, u_int32_t, u_char,
                                                int);
int hsl_mpls_vc_ftn_add (struct hal_msg_mpls_vc_fib_add *);
int hsl_mpls_vc_ftn_del (struct hal_msg_mpls_vc_fib_del *);
int hsl_mpls_vpn_vc_ftn_add_to_hw (struct hsl_mpls_vpn_vc *,
                                   struct hsl_nh_entry *);
int hsl_mpls_vc_fib_add (struct hal_msg_mpls_vc_fib_add *);
int hsl_mpls_vc_fib_del (struct hal_msg_mpls_vc_fib_del *);
#endif /* HAVE_MPLS_VC */
int hsl_mpls_vpn_add (u_int32_t, u_char vpn_type);
int hsl_mpls_vpn_del (u_int32_t, u_char vpn_type);
int hsl_mpls_vpn_if_bind (u_int32_t, u_int32_t, u_int16_t, u_char vpn_type);
int hsl_mpls_vpn_if_unbind (u_int32_t, u_int32_t, u_int16_t, u_char vpn_type);
#endif /* _HSL_MPLS_H_ */
