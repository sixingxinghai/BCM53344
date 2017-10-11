/*=============================================================================
**
** Copyright (C) 2002-2003   All Rights Reserved.
**
** plat_vrrp.h -- PacOS VRRP dataplane definitions for Linux.
**
*/
#ifndef _PLAT_VRRP_H
#define _PLAT_VRRP_H

/*
** Include files
*/
#include "pal.h"

#ifdef HAVE_VRRP

#include "nsm/vrrp/vrrp.h"

/*
** Definitions.
*/
#define ALLNODES_GROUP                  "ff02::1"
/*
** Datatypes.
*/

/*
** This struct stores virtual IP addresses for linux impl. #2
*/
#ifndef HAVE_VRRP_LINK_ADDR
struct vip_addr_struc
{
  struct pal_in4_addr vip;                   /* Virtual IP addresss */
  int vrid;                             /* VRRP virtual router ID */
  struct interface *ifp;                /* Interface pointer */
  struct vip_addr_struc *next;          /* Next one in the list */
  struct vip_addr_struc *prev;          /* Previous one in the list */
};

#ifdef HAVE_IPV6
struct vip6_addr_struc
{
  struct pal_in6_addr vip6;             /* Virtual IPv6 addresss */
  int vrid;                             /* VRRP virtual router ID */
  struct interface *ifp;                /* Interface pointer */
  struct vip6_addr_struc *next;          /* Next one in the list */
  struct vip6_addr_struc *prev;          /* Previous one in the list */
};
#endif /* HAVE_IPV6 */

#endif /* HAVE_VRRP_LINK_ADDR */

/*
** pal_vrrp_data_t structure.  This is used to store VRRP information for
** the lower layers.
*/
typedef struct pal_vrrp_data
{
  struct thread *t_vrrp_prom;           /* promiscuous receive */
  pal_sock_handle_t sock_garp;          /* sock for sending grat. arps */
  pal_sock_handle_t sock_promisc;       /* sock for receiving promiscuous */
  pal_sock_handle_t sock_net;           /* comm with network layer */
  int vmac_enabled;                     /* Flag indicating usage of VMAC or RMAC */
  PAL_MAC_ADDR vrrp_mac;                /* VRRP virtual-MAC for forwarding */
  u_int8_t old_mac[MAX_VRIDS+1][6];     /* Old Mac for each VRRP session. */

#ifdef HAVE_IPV6
  struct thread *t_vrrp_nd;
  struct stream *ibuf;
  pal_sock_handle_t sock_ndadvt;        /* sock for sending neighbor advrt. */
#endif /* HAVE_IPV6 */

#ifndef HAVE_VRRP_LINK_ADDR
  /* Global List Pointers */
  struct vip_addr_struc *vip_head;
  struct vip_addr_struc *vip_tail;

#ifdef HAVE_IPV6
  struct vip6_addr_struc *vip6_head;
  struct vip6_addr_struc *vip6_tail;
#endif /* HAVE_IPV6 */

#endif /* ! HAVE_VRRP_LINK_ADDR */

} pal_vrrp_data_t;

/*
** Pointer to pal_vrrp_data_t.
*/
typedef pal_vrrp_data_t *pal_vrrp_handle_t;


/*
** Structure to cast promiscuous packet.
*/
struct prom_rx {
  u_char      eth_dst[6];
  u_char      eth_src[6];
  u_int16_t     eth_type;

  u_char        iph_ver_hl;
  u_char        iph_tos;
  u_int16_t     iph_totlen;
  u_int16_t     iph_id;
  u_int16_t     iph_fl_off;
  u_char        iph_ttl;
  u_char        iph_prot;
};

#define         SIZE_PROM_RX            sizeof(struct prom_rx)


/*
** Prototypes.
*/
result_t plat_vrrp_init (struct lib_globals *);
result_t plat_vrrp_deinit (struct lib_globals *);

result_t plat_vrrp_send_garp (struct lib_globals *,
                              struct stream *,
                              struct interface *);

result_t plat_vrrp_set_vip (struct lib_globals *lib_node,
                            struct pal_in4_addr *,
                            struct interface *,
                            int);

result_t plat_vrrp_unset_vip (struct lib_globals *lib_node,
                              struct pal_in4_addr *,
                              struct interface *,
                              int);

result_t plat_vrrp_get_vmac_status (struct lib_globals *lib_node);
result_t plat_vrrp_set_vmac_status (struct lib_globals *lib_node, int status);
result_t plat_vrrp_send_nd_nbadvt (struct lib_globals *lib_node,
                                   struct stream *s, struct interface *ifp);

#ifdef HAVE_IPV6
result_t plat_vrrp_set_vip6 (struct lib_globals *lib_node,
                             struct pal_in6_addr *vip6,
                             struct interface *ifp, int vrid);
result_t plat_vrrp_unset_vip6 (struct lib_globals *lib_node,
                               struct pal_in6_addr *vip6,
                               struct interface *ifp, int vrid);
#endif /* HAVE_IPV6 */
#endif /* HAVE_VRRP */
#endif /* _PLAT_VRRP_H */
