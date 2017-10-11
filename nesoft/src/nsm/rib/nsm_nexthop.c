/* Copyright (C) 2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "vector.h"
#include "message.h"
#include "network.h"
#include "nsmd.h"
#include "nexthop.h"
#include "nsm_message.h"
#include "log.h"
#include "linklist.h"

#ifdef HAVE_MPLS
#include "mpls.h"
#endif /* HAVE_MPLS */

#include "rib/rib.h"
#include "nsm_server.h"
#include "rib/nsm_server_rib.h"
#include "nsm_debug.h"
#include "rib/nsm_table.h"
#include "rib/nsm_nexthop.h"

static int
nsm_nexthop_move_reg_to_nh_table (struct nsm_ptree_node *rn, 
                                  struct nsm_ptree_table *nh_table)
{
  struct listnode *lnode, *lnode_next, *lnode1, *lnode1_next;
  struct nexthop_lookup_reg *nh_reg, *nh_reg1;
  struct nsm_server_client *nsc;
  struct nsm_ptree_node *rnode;

  if (rn && rn->nh_list)
    {
      rnode = nsm_ptree_node_get (nh_table, rn->key, rn->key_len);
      if (!rnode)
        {
          /* Error. */
        }
      else
        {
          rnode->info = rnode;

          if (!rnode->nh_list)
            rnode->nh_list = list_new ();

          for (lnode = LISTHEAD (rn->nh_list); lnode; lnode = lnode_next)
            {
              lnode_next = lnode->next;
              
              nh_reg = GETDATA (lnode);
              if (!nh_reg)
                continue;

              nh_reg1 = XCALLOC (MTYPE_NSM_NEXTHOP_LOOKUP_REG, 
                                 sizeof (struct nexthop_lookup_reg));
              pal_mem_cpy (&nh_reg1->p, &nh_reg->p, sizeof (struct prefix));
              nh_reg1->lookup_type = nh_reg->lookup_type;
              
              for (lnode1 = LISTHEAD (&nh_reg->client_list); lnode1;
                   lnode1 = lnode1_next)
                {
                  lnode1_next = lnode1->next;

                  nsc = GETDATA (lnode1);
                  
                  /* Add to nh_table client_list. */
                  listnode_add (&nh_reg1->client_list, nsc);
                  
                  /* Delete from RIB's client_list. */
                  list_delete_node (&nh_reg->client_list, lnode1);
                }

              /* Add nh_reg to nh_table. */
              listnode_add (rnode->nh_list, nh_reg1);

              /* Delete nh_reg from rn. */
              XFREE (MTYPE_NSM_NEXTHOP_LOOKUP_REG, nh_reg);
              list_delete_node (rn->nh_list, lnode);
            }

          if (LIST_ISEMPTY (rn->nh_list))
            {
              list_free (rn->nh_list);
              rn->nh_list = 0;
            }
        }
    }

  return 0;
}

static int
nsm_nexthop_reg_move_to_route_node (struct nsm_ptree_node *rn, /* From. */
                                    struct nsm_ptree_node *rn1 /* To. */
                                    )
{
  struct listnode *lnode, *lnode_next, *lnode1, *lnode1_next;
  struct nexthop_lookup_reg *nh_reg, *nh_reg1;
  struct nsm_server_client *nsc;

  if (rn && rn1 && rn->nh_list)
    {
      for (lnode = LISTHEAD (rn->nh_list); lnode; lnode = lnode_next)
        {
          lnode_next = lnode->next;
              
          nh_reg = GETDATA (lnode);
          if (!nh_reg)
            continue;
    
          if ((nh_reg->p.prefixlen >= rn1->key_len    &&
               nh_reg->lookup_type == BEST_MATCH_LOOKUP) ||
              (nh_reg->p.prefixlen == rn1->key_len   &&
               nh_reg->lookup_type == EXACT_MATCH_LOOKUP))
            {
              if (!rn1->nh_list)         
                rn1->nh_list = list_new ();
                  
              nh_reg1 = XCALLOC (MTYPE_NSM_NEXTHOP_LOOKUP_REG,
                                 sizeof (struct nexthop_lookup_reg));

              pal_mem_cpy (&nh_reg1->p, &nh_reg->p, sizeof (struct prefix));
              nh_reg1->lookup_type = nh_reg->lookup_type;

              for (lnode1 = LISTHEAD (&nh_reg->client_list); lnode1;
                   lnode1 = lnode1_next)
                {
                  lnode1_next = lnode1->next;

                  nsc = GETDATA (lnode1);

                  /* Add to leaf's client_list. */
                  listnode_add (&nh_reg1->client_list, nsc);

                  /* Delete from parent's client_list. */
                  list_delete_node (&nh_reg->client_list, lnode1);
                }

              /* Add nh_reg to leaf. */
              listnode_add (rn1->nh_list, nh_reg1);

              /* Delete nh_reg from parent. */
              XFREE (MTYPE_NSM_NEXTHOP_LOOKUP_REG, nh_reg);
              list_delete_node (rn->nh_list, lnode);
            }
        }
    }

  return 0;
}

int
nsm_nexthop_process_rib_add (struct nsm_ptree_node *rn, struct rib *rib)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct nexthop_lookup_reg *nh_reg;
  struct listnode *n1, *n2;

  if (rn->nh_list == NULL)
    return 0;

  LIST_LOOP (rn->nh_list, nh_reg, n1)
    LIST_LOOP (&nh_reg->client_list, nsc, n2)
      for (nse = nsc->head; nse; nse = nse->next)
        if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_ROUTE)
            || NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_ROUTE_LOOKUP))
          {
            if (nse->service.protocol_id == APN_PROTO_BGP) 
              SET_FLAG (rib->pflags,NSM_ROUTE_CHG_INFORM_BGP);

            if (rn->tree->ptype == AF_INET)
              nsm_send_ipv4_add (nse, 0, (struct prefix *)&nh_reg->p,
                                 rn->key_len, rib, rib->vrf, 0, NULL);
#ifdef HAVE_IPV6
            else if (NSM_CAP_HAVE_IPV6 && rn->tree->ptype == AF_INET6)
              nsm_send_ipv6_add (nse, 0, (struct prefix *)&nh_reg->p,
                                 rn->key_len, rib, rib->vrf, 0, NULL);
#endif /* HAVE_IPV6 */
          }

  return 0;
}

int
nsm_nexthop_process_rib_delete (struct nsm_ptree_node *rn, struct rib *rib)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct nexthop_lookup_reg *nh_reg;
  struct listnode *n1, *n2;

  if (rn->nh_list == NULL)
    return 0;

  LIST_LOOP (rn->nh_list, nh_reg, n1)
    LIST_LOOP (&nh_reg->client_list, nsc, n2)
      for (nse = nsc->head; nse; nse = nse->next)
        if (NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_ROUTE)
            || NSM_CHECK_CTYPE (nse->service.bits, NSM_SERVICE_ROUTE_LOOKUP))
          {
            if (nse->service.protocol_id == APN_PROTO_BGP)
              SET_FLAG (rib->pflags,NSM_ROUTE_CHG_INFORM_BGP);

            if (rn->tree->ptype == AF_INET)
              nsm_send_ipv4_delete (nse, (struct prefix *)&nh_reg->p,
                                    rn->key_len, rib, rib->vrf, 0);
#ifdef HAVE_IPV6
            else if (NSM_CAP_HAVE_IPV6 && rn->tree->ptype == AF_INET6)
              nsm_send_ipv6_delete (nse, (struct prefix *)&nh_reg->p,
                                    rn->key_len, rib, rib->vrf, 0);
#endif /* HAVE_IPV6 */
          }

  return 0;
}

int
nsm_nexthop_process_route_node (struct nsm_ptree_node *rn)
{
  int found = 0;
  struct rib *match;
  struct nexthop *nexthop;

  /* Pick up selected route. */
  for (match = rn->info; match; match = match->next)
    if (CHECK_FLAG (match->flags, RIB_FLAG_SELECTED))
      break;

  if (!match || match->type == APN_ROUTE_BGP)
    return 0;

  if (match->type == APN_ROUTE_CONNECT)
    found = 1;

  for (nexthop = match->nexthop; nexthop; nexthop = nexthop->next)
    if (CHECK_FLAG (nexthop->flags, NEXTHOP_FLAG_FIB))
      {
        found = 1;
        break;
      }

  if (found)
    nsm_nexthop_process_rib_add (rn, match);

  return 0;
}

struct nsm_ptree_node *
nsm_nexthop_process_route_node_add (struct nsm_ptree_node *rn, 
                                    struct nsm_ptree_table *nh_table)
{
  struct nsm_ptree_node *rnode = rn;
  struct nsm_ptree_node *rn1, *rn_head;

  if(rn)
    {
      while (rnode && rnode->parent && (rnode->parent->info == NULL))
        rnode = rnode->parent;
      if (rnode && rnode->parent)
        rnode = rnode->parent;

      /* If a valid node is found, move all registrations from parent
         to the leaf only if
         1. Lookup type == BEST_MATCH_LOOKUP &&
         2. Prefixlen >= new rn's prefixlen */
      if (rnode)
        {
          /* Move client registrations downwards if conditions met. */
          if (rnode->nh_list)
            {
              nsm_nexthop_reg_move_to_route_node (rnode, rn);

              if (LIST_ISEMPTY (rnode->nh_list))
                {
                  list_free (rnode->nh_list);
                  rnode->nh_list = NULL;
                  return rnode;
                }
              else
                return NULL;
            }

          /* Process pending registrations in nexthop tree. */
          rn_head = nsm_ptree_node_match (nh_table, rn->key, rn->key_len);

          for (rn1 = rn_head; rn1; rn1 = nsm_ptree_next_until (rn1, rn_head))
            {
              if (! rn1->info)
                continue;

              /* Move client registrations from nh_table to RIB
                 if conditions match. */
              nsm_nexthop_reg_move_to_route_node (rn1, rn);

              if (LIST_ISEMPTY (rn1->nh_list))
                {
                  list_free (rn1->nh_list);
                  rn1->nh_list = NULL;

                  rn1->info = NULL;
                  nsm_ptree_unlock_node (rn1);
                }
            }
        }
    }
  return 0;
}

struct nsm_ptree_node *
nsm_nexthop_process_route_node_delete (struct nsm_ptree_node *rn, 
                                       struct rib *rib,
                                       struct nsm_ptree_table *nh_table)
{
  struct nsm_ptree_node *rnode = rn;
  
  if ((rn && !rn->info)
      || (rn && rn->info
          && !CHECK_FLAG (((struct rib *)(rn->info))->flags,
                          RIB_FLAG_SELECTED)))
    {
      while (rnode && rnode->parent && (rnode->parent->info == NULL))
        rnode = rnode->parent;
      
      if (rnode && rnode->parent)
        rnode = rnode->parent;
      
      if (rnode && rnode->info)
        {
          /* Move client registrations upwards if conditions met. */
          if (rn->nh_list)
            {
              nsm_nexthop_reg_move_to_route_node (rn, rnode);
              if (LIST_ISEMPTY (rn->nh_list))
                {
                  list_free (rn->nh_list);
                  rn->nh_list = NULL;
                }
              else
                {
                  /* Send delete to clients. */
                  nsm_nexthop_process_rib_delete (rn, rib);

                  /* Move remaining registrations(exact match) to nh_table. */
                  nsm_nexthop_move_reg_to_nh_table (rn, nh_table);
                }

              return rnode;
            }
          else
            return NULL;
        }

      /* Send delete to clients. */
      nsm_nexthop_process_rib_delete (rn, rib);
      
      /* Move registrations to nh_table. */
      nsm_nexthop_move_reg_to_nh_table (rn, nh_table);
    }
  
  return NULL;      
}

static void
nsm_nexthop_rib_clean (struct nsm_ptree_table *table, 
                       struct nsm_server_client *nsc)
{
  struct nsm_ptree_node *rn;
  struct listnode *node, *node_next, *node1, *node1_next;
  struct nexthop_lookup_reg *nh_reg;
  struct nsm_server_client *nnsc;

  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    {
      if (rn->nh_list)
        {
          for (node = LISTHEAD (rn->nh_list); node; node = node_next)
            {
              node_next = node->next;

              nh_reg = GETDATA (node);
              if (!nh_reg)
                continue;

              for (node1 = LISTHEAD (&nh_reg->client_list); node1;
                   node1 = node1_next)
                {
                  node1_next = node1->next;

                  nnsc = GETDATA (node1);
                  if (!nnsc)
                    continue;

                  if (nsc == nnsc)
                    list_delete_node (&nh_reg->client_list, node1);
                }

              if (LIST_ISEMPTY (&nh_reg->client_list))
                {
                  XFREE (MTYPE_NSM_NEXTHOP_LOOKUP_REG, nh_reg);
                  list_delete_node (rn->nh_list, node);
                }
            }

          if (LIST_ISEMPTY (rn->nh_list))
            {
              list_free (rn->nh_list);
              rn->nh_list = NULL;
            }
        }
    }
}

void
nsm_nexthop_rib_clean_all (struct nsm_ptree_table *table)
{
  struct nsm_ptree_node *rn;
  struct listnode *node, *node_next, *node1, *node1_next;
  struct nexthop_lookup_reg *nh_reg;

  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    {
      if (rn->nh_list)
        {
          for (node = LISTHEAD (rn->nh_list); node; node = node_next)
            {
              node_next = node->next;

              nh_reg = GETDATA (node);
              if (!nh_reg)
                continue;

              for (node1 = LISTHEAD (&nh_reg->client_list); node1;
                   node1 = node1_next)
                {
                  node1_next = node1->next;
                  list_delete_node (&nh_reg->client_list, node1);
                }

              if (LIST_ISEMPTY (&nh_reg->client_list))
                {
                  XFREE (MTYPE_NSM_NEXTHOP_LOOKUP_REG, nh_reg);
                  list_delete_node (rn->nh_list, node);
                }
            }

          if (LIST_ISEMPTY (rn->nh_list))
            {
              list_free (rn->nh_list);
              rn->nh_list = NULL;
            }
        }
    }
}

static void
nsm_nexthop_nh_table_clean (struct nsm_ptree_table *table, 
                            struct nsm_server_client *nsc)
{
  struct nsm_ptree_node *rn;
  struct listnode *node, *node_next, *node1, *node1_next;
  struct nexthop_lookup_reg *nh_reg;
  struct nsm_server_client *nnsc;

  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    {
      if (rn->nh_list)
        {
          for (node = LISTHEAD (rn->nh_list); node; node = node_next)
            {
              node_next = node->next;

              nh_reg = GETDATA (node);
              if (!nh_reg)
                continue;
        
              for (node1 = LISTHEAD (&nh_reg->client_list); node1;
                   node1 = node1_next)
                {
                  nnsc = GETDATA (node1);
        
                  node1_next = node1->next;

                  if (!nnsc)
                    continue;           

                  if (nsc == nnsc)
                    list_delete_node (&nh_reg->client_list, node1);
                }

              if (LIST_ISEMPTY (&nh_reg->client_list))
                {
                  XFREE (MTYPE_NSM_NEXTHOP_LOOKUP_REG, nh_reg);
                  list_delete_node (rn->nh_list, node);
                }
            }

          if (LIST_ISEMPTY (rn->nh_list))
            {
              list_free (rn->nh_list);
              rn->nh_list = NULL;

              rn->info = NULL;

              nsm_ptree_unlock_node (rn);
            }
        }
    }
}

void
nsm_nexthop_nh_table_clean_all (struct nsm_ptree_table *table)
{
  struct nsm_ptree_node *rn;
  struct listnode *node, *node_next, *node1, *node1_next;
  struct nexthop_lookup_reg *nh_reg;

  for (rn = nsm_ptree_top (table); rn; rn = nsm_ptree_next (rn))
    {
      if (rn->nh_list)
        {
          for (node = LISTHEAD (rn->nh_list); node; node = node_next)
            {
              node_next = node->next;

              nh_reg = GETDATA (node);
              if(!nh_reg)
                continue;

              for (node1 = LISTHEAD (&nh_reg->client_list); node1;
                   node1 = node1_next)
                {
                  node1_next = node1->next;
                  list_delete_node (&nh_reg->client_list, node1);
                }

              if (LIST_ISEMPTY (&nh_reg->client_list))
                {
                  XFREE (MTYPE_NSM_NEXTHOP_LOOKUP_REG, nh_reg);
                  list_delete_node (rn->nh_list, node);
                }
            }

          if (LIST_ISEMPTY (rn->nh_list))
            {
              list_free (rn->nh_list);
              rn->nh_list = NULL;

              rn->info = NULL;

              nsm_ptree_unlock_node (rn);
            }
        }
    }

  /* Free up tree. */
  nsm_ptree_table_finish (table);
}

void
nsm_nexthop_reg_clean (struct nsm_master *nm, struct nsm_server_client *nsc)
{
  struct apn_vrf *iv;
  struct nsm_vrf *nv;
  afi_t afi;
  int i;

  for (i = 0; i < vector_max (nm->vr->vrf_vec); i++)
    if ((iv = vector_slot (nm->vr->vrf_vec, i)) != NULL)
      {
        nv = iv->proto;
        NSM_AFI_LOOP (afi)
          {
            nsm_nexthop_rib_clean (nv->afi[afi].rib[SAFI_UNICAST], nsc);
            nsm_nexthop_nh_table_clean (nv->afi[afi].nh_reg, nsc);
          }
      }
}
