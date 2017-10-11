/* Copyright (C) 2001-2006  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "table.h"

#include "ospfd/ospfd.h"
#ifdef HAVE_MPLS
#include "ospfd/ospf_mpls.h"
#endif /* HAVE_MPLS */
#include "ospfd/ospf_vrf.h"
#include "ospfd/ospf_spf.h"
#include "ospfd/ospf_lsa.h"


#ifdef HAVE_MPLS 
struct ospf_igp_shortcut_lsp *
ospf_igp_shortcut_lsp_new (struct pal_in4_addr *t_endp, 
                           u_int16_t metric, u_int32_t t_id)
{
  struct ospf_igp_shortcut_lsp *lsp;

  lsp = XCALLOC (MTYPE_OSPF_IGP_SHORTCUT_LSP, 
                    sizeof (struct ospf_igp_shortcut_lsp));
  if (! lsp)
    return NULL;

  IPV4_ADDR_COPY (&lsp->t_endp, t_endp);
  lsp->metric = metric;
  lsp->tunnel_id = t_id;
  lsp->next = NULL;
  lsp->lock = 0;

  return lsp;
}

void
ospf_igp_shortcut_lsp_free (struct ospf_igp_shortcut_lsp *lsp)
{
  XFREE (MTYPE_OSPF_IGP_SHORTCUT_LSP, lsp);
}

void
ospf_igp_shortcut_lsp_delete_node (struct ospf_igp_shortcut_lsp *lsp)
{
  struct ospf_igp_shortcut_lsp *tmp;
  struct ospf_igp_shortcut_lsp **list;
  struct route_node *rn;

  rn = lsp->rn;
  if (! rn || ! rn->info)
    return;

  list = (struct ospf_igp_shortcut_lsp **)&rn->info;
  if (*list == NULL)
    return;

  /* If first... */
  if (*list == lsp)
    {
      *list = lsp->next;
      lsp->rn = NULL;
      return;
    }

  /* Else... */
  for (tmp = *list; tmp; tmp = tmp->next)
    {
      if (tmp->next == lsp)
        {
          tmp->next = lsp->next;
          lsp->rn = NULL;
          return;
        }
    }
}

struct ospf_igp_shortcut_lsp *
ospf_igp_shortcut_lsp_lock (struct ospf_igp_shortcut_lsp *lsp)
{
  if (! lsp)
    return NULL;

  lsp->lock++;

  return lsp;
}

struct ospf_igp_shortcut_lsp *
ospf_igp_shortcut_lsp_unlock (struct ospf_igp_shortcut_lsp *lsp)
{
  if (! lsp)
    return NULL;

  lsp->lock--;

  if (lsp->lock == 0)
    {
      ospf_igp_shortcut_lsp_delete_node (lsp);
      ospf_igp_shortcut_lsp_free (lsp);
      return NULL;      
    }

  return lsp;
}

/* sorted by metric */
void
ospf_igp_shortcut_lsp_add_node (struct route_node *rn, 
                                struct ospf_igp_shortcut_lsp *lsp_new) 
{
  struct ospf_igp_shortcut_lsp **list;
  struct ospf_igp_shortcut_lsp *lsp, *prev_lsp = NULL;

  if (! lsp_new || ! rn)
    return;
 
  list = (struct ospf_igp_shortcut_lsp **)&rn->info; 

  if (! *list)
    {
      *list = lsp_new;
      lsp_new->rn = rn;
      return;
    }

  for (lsp = *list; lsp; lsp = lsp->next)
    {
      if (lsp->metric <= lsp_new->metric)
        prev_lsp = lsp;
      else 
        break;
    }      

  if (prev_lsp)
    {
      lsp_new->next = prev_lsp->next;
      prev_lsp->next = lsp_new;
    }
  else
    {
      lsp_new->next = *list;
      *list = lsp_new;
    } 

  lsp_new->rn = rn;
}

void
ospf_igp_shortcut_lsp_update_node (struct route_node *rn,
                                 struct ospf_igp_shortcut_lsp *lsp)
{
  ospf_igp_shortcut_lsp_delete_node (lsp);
  /* lsp_add_node will add by metric */
  ospf_igp_shortcut_lsp_add_node (rn, lsp);
}

struct ospf_igp_shortcut_lsp *
ospf_igp_shortcut_lsp_get_active (struct route_node *rn)
{
  struct ospf_igp_shortcut_lsp *lsp;

  for (lsp = rn->info; lsp; lsp = lsp->next)
    if (! CHECK_FLAG (lsp->flags, OSPF_IGP_SHORTCUT_LSP_INACTIVE))
      return lsp;

  return NULL;
}

struct ospf_igp_shortcut_lsp *
ospf_igp_shortcut_lsp_lookup (struct route_node *rn, u_int32_t t_id)
{
  struct ospf_igp_shortcut_lsp **lsp_list; 
  struct ospf_igp_shortcut_lsp *lsp;

  if (! rn || ! rn->info)
    return NULL;

  lsp_list = (struct ospf_igp_shortcut_lsp **)&rn->info;

  for (lsp = *lsp_list; lsp; lsp = lsp->next)
    {
      if (lsp->tunnel_id == t_id &&
          ! CHECK_FLAG (lsp->flags, OSPF_IGP_SHORTCUT_LSP_INACTIVE))
        return lsp;
    }

  return NULL;
}

int
ospf_igp_shortcut_lsp_add (struct ospf_vrf *ov, 
                           struct nsm_msg_igp_shortcut_lsp *msg)
{
  struct prefix p;
  struct route_node *rn;
  struct ospf_igp_shortcut_lsp *lsp, *lsp_new;

  if (! ov || ! ov->igp_lsp_table)
    return 0;

  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  IPV4_ADDR_COPY (&p.u.prefix4, &msg->t_endp);

  rn = route_node_get (ov->igp_lsp_table, &p);
  if (! rn)
    return 0;

  /* ospf alway use new lsp, nsm will taking care of only send lsp-add once
     for every new lsp. When "no router ospf" might lead orphan lsp */
  if ((lsp = ospf_igp_shortcut_lsp_lookup (rn, msg->tunnel_id)))
    { 
      if (! CHECK_FLAG (lsp->flags, OSPF_IGP_SHORTCUT_LSP_INACTIVE))
        {
          SET_FLAG (lsp->flags, OSPF_IGP_SHORTCUT_LSP_INACTIVE);
          ospf_igp_shortcut_lsp_unlock (lsp); 
        }
      /* For inactive lsp, we need create new one so spf could add
         igp-shortcut route link to new lsp and send to nsm */
    }

  lsp_new = ospf_igp_shortcut_lsp_new (&msg->t_endp, msg->metric, 
                                       msg->tunnel_id);
  if (! lsp_new)
    return 0;

  ospf_igp_shortcut_lsp_add_node (rn, lsp_new);
  ospf_igp_shortcut_lsp_lock (lsp_new); 
  return 1;
}

int
ospf_igp_shortcut_lsp_delete (struct ospf_vrf *ov, 
                              struct nsm_msg_igp_shortcut_lsp *msg)
{
  struct prefix p;
  struct route_node *rn;
  struct ospf_igp_shortcut_lsp *lsp;

  if (! ov || ! ov->igp_lsp_table)
    return 0;

  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  IPV4_ADDR_COPY (&p.u.prefix4, &msg->t_endp);

  rn = route_node_get (ov->igp_lsp_table, &p);
  if (! rn)
    return 0;

  /* remove from list */
  lsp = ospf_igp_shortcut_lsp_lookup (rn, msg->tunnel_id);
  if (lsp)
    {
      if (! CHECK_FLAG (lsp->flags, OSPF_IGP_SHORTCUT_LSP_INACTIVE))
        {
          SET_FLAG (lsp->flags, OSPF_IGP_SHORTCUT_LSP_INACTIVE);
          ospf_igp_shortcut_lsp_unlock (lsp); 
        }
      return 1;
    }

  return 0;
}

void
ospf_igp_shortcut_lsp_delete_all (struct ospf_vrf *ov)
{
  struct route_node *rn;
  struct ospf_igp_shortcut_lsp *lsp, *lsp_next;

  if (! ov || ! ov->igp_lsp_table)
    return;

  for (rn = route_top (ov->igp_lsp_table); rn; rn = route_next (rn))
    {
      if ((lsp = rn->info) != NULL)
        {
          for (lsp = (struct ospf_igp_shortcut_lsp *)rn->info; lsp; 
               lsp = lsp_next)
            {
              lsp_next = lsp->next;
              ospf_igp_shortcut_lsp_unlock (lsp);
            }
        }

      rn->info = NULL;
      route_unlock_node (rn);      
    }
}

void
ospf_igp_shortcut_lsp_clean (struct ospf_vrf *ov)
{
  ospf_igp_shortcut_lsp_delete_all (ov);
  ospf_igp_shortcut_lsp_process (ov);
}

int
ospf_igp_shortcut_lsp_update (struct ospf_vrf *ov, 
                              struct nsm_msg_igp_shortcut_lsp *msg)
{
  struct prefix p;
  struct route_node *rn;
  struct ospf_igp_shortcut_lsp *lsp;

  if (! ov || ! ov->igp_lsp_table)
    return 0;

  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  IPV4_ADDR_COPY (&p.u.prefix4, &msg->t_endp);

  rn = route_node_get (ov->igp_lsp_table, &p);
  if (! rn)
    return 0;

  lsp = ospf_igp_shortcut_lsp_lookup (rn, msg->tunnel_id);
  if (! lsp)
    return 0;

  if (lsp->metric == msg->metric)
    return 0;

  lsp->metric = msg->metric;
  ospf_igp_shortcut_lsp_update_node (rn, lsp);
  return 1;
}

void
ospf_igp_shortcut_lsp_process (struct ospf_vrf *ov)
{
  struct ospf *top;
  struct listnode *node;

  if (! ov)
    return;

  LIST_LOOP (ov->ospf, top, node)
    ospf_spf_calculate_timer_add_all (top);
}

int
ospf_igp_shortcut_check_route_back (struct ospf_lsa *w_lsa,
                                    struct ospf_lsa *v_lsa)
{
  char *p;
  char *lim;
  struct ospf_link_desc link;

  p = ((u_char *)v_lsa->data) + OSPF_LSA_HEADER_SIZE + 4;
  lim = ((u_char *) v_lsa->data) + pal_ntoh16 (v_lsa->data->length);

  /* In case of W is Network LSA. */
  if (w_lsa->data->type == OSPF_NETWORK_LSA)
    {
      if (v_lsa->data->type != OSPF_NETWORK_LSA)
        while (p + OSPF_LINK_DESC_SIZE <= lim)
          {
            OSPF_LINK_DESC_GET (p, link);
            if (IPV4_ADDR_SAME (link.id, &w_lsa->data->id))
              return 1;

            p += OSPF_LINK_DESC_SIZE + OSPF_LINK_TOS_SIZE * link.num_tos;
          }
    }

  return 0;
}

void
ospf_igp_shortcut_nexthop_calculate (struct ospf_vertex *v, 
                                    struct ospf_vertex *w, struct ospf *top)
{
  struct ospf_vrf *ov = top->ov;
  struct ospf_lsa *lsa = w->lsa;
  struct ospf_igp_shortcut_lsp *lsp;
  struct prefix p;
  struct route_node *rn;
  int route_back = 0;
  int distance = 0;

  p.family = AF_INET;
  p.prefixlen = IPV4_MAX_PREFIXLEN;
  p.u.prefix4 = lsa->data->adv_router; 

  route_back = ospf_igp_shortcut_check_route_back (w->lsa, v->lsa);
  if (route_back && ! v->t_lsp)
    return;

  distance = w->t_distance - v->t_distance;

  if (route_back && v->t_lsp)
    {
      if ((v->t_lsp->metric + distance) <= w->t_distance)
        {
          w->t_distance = v->t_lsp->metric +distance;
          w->t_lsp = v->t_lsp;
          return;
        }
    }

  /* Get tunnel endpoint from igp_shortcut_table */
  rn = route_node_get (ov->igp_lsp_table, &p); 
  if (! rn)
    return;

  lsp = ospf_igp_shortcut_lsp_get_active (rn);

  if (lsp)
    {
      /* This directly reachable tunnel has lower metric, use it. */
      if (lsp->metric <= w->t_distance)
        {
          w->t_distance = lsp->metric; 
          w->t_lsp = lsp;
          return;
        }

      /* If directly reachable tunnel has higher metric than parent's 
         tunnel's metric + metric between parent and W, inherit from 
         parent. This as same as no directly reachable tunnel. */
    }

  /* v(parent)'s t_lsp might be NULL */
  w->t_lsp = v->t_lsp;
  if (v->t_lsp)
    w->t_distance = v->t_lsp->metric + distance;
}

#endif /* HAVE_MPLS */
