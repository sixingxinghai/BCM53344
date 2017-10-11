/* Copyright (C) 2001-2006  All Rights Reserved. */
                                                                                
#ifndef _PACOS_OSPF_LSP_H
#define _PACOS_OSPF_LSP_H

struct ospf_igp_shortcut_lsp
{
  u_int16_t metric;
  u_int32_t tunnel_id;
  struct pal_in4_addr t_endp;
  struct route_node *rn;
  struct ospf_igp_shortcut_lsp *next;
  u_int16_t lock;

#define OSPF_IGP_SHORTCUT_LSP_INACTIVE  (1 << 0)
  u_char flags;
};

struct ospf_igp_shortcut_route
{
  u_int16_t metric;
  u_int32_t tunnel_id;
  struct prefix fec;
  struct pal_in4_addr t_endp;
  struct ospf_igp_shortcut_route *next;
};

struct ospf_igp_shortcut_lsp *
ospf_igp_shortcut_lsp_lookup (struct route_node *, u_int32_t);
int ospf_igp_shortcut_lsp_add (struct ospf_vrf *, 
                               struct nsm_msg_igp_shortcut_lsp *);
int ospf_igp_shortcut_lsp_delete (struct ospf_vrf *, 
                                  struct nsm_msg_igp_shortcut_lsp *);
int ospf_igp_shortcut_lsp_update (struct ospf_vrf *, 
                                  struct nsm_msg_igp_shortcut_lsp *);
void ospf_igp_shortcut_lsp_delete_all (struct ospf_vrf *);
void ospf_igp_shortcut_lsp_clean (struct ospf_vrf *);
void ospf_igp_shortcut_lsp_process (struct ospf_vrf *);
void ospf_igp_shortcut_nexthop_calculate (struct ospf_vertex *,
                                          struct ospf_vertex *,
                                          struct ospf *);
struct ospf_igp_shortcut_lsp *
ospf_igp_shortcut_lsp_lock (struct ospf_igp_shortcut_lsp *);
struct ospf_igp_shortcut_lsp *
ospf_igp_shortcut_lsp_unlock (struct ospf_igp_shortcut_lsp *);
#endif /* _PACOS_OSPF_LSP_H */
