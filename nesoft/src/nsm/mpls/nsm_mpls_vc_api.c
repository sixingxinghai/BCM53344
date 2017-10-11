
/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "table.h"
#include "ptree.h"
#include "bitmap.h"
#include "nsmd.h"
#include "nsm_router.h"
#include <asn1.h>

#include "if.h"
#include "thread.h"
#include "prefix.h"
#include "snmp.h"
#include "log.h"
#include "mpls_common.h"
#include "mpls_client.h"
#include "mpls.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#ifdef HAVE_DSTE
#include "dste.h"
#include "nsm_dste.h"
#endif /* HAVE_DSTE */
#include "nsm_mpls.h"
#include "nsm_debug.h"
#include "rib/nsm_table.h"
#include "nsm_server.h"
#include "nsm_lp_serv.h"
#include "nsm_mpls_dep.h"
#include "nsm_mpls_rib.h"
#include "nsm_mpls_fwd.h"
#include "nsm_mpls_vc.h"
#include "nsm_mpls_vc_snmp.h"
#include "nsm_mpls_vc_api.h"
#ifdef HAVE_VPLS
#include "nsm_vpls.h"
#endif /*HAVE_VPLS */
#ifdef HAVE_MPLS
#ifdef HAVE_HAL
#include "hal_mpls.h"
#endif /*HAVE_HAL */
#endif /*HAVE_MPLS */
#ifdef HAVE_MPLS_VC
#ifdef HAVE_SNMP

#define L2_TIMER_SCALE_FACT 256

#define TICS_TO_MICROSECS(x)  (((x) & 0xff) * (1000000/L2_TIMER_SCALE_FACT))
#define MICROSECS_TO_TICS(x) ((x) / (1000000/L2_TIMER_SCALE_FACT))
#define TICS_TO_SECS(x)       ((x) / L2_TIMER_SCALE_FACT)
#define SECS_TO_TICS(x)       ((x) * L2_TIMER_SCALE_FACT)

extern pal_time_t nsm_get_sys_up_time ();
/* Support APIs for VC Table */
/* MPLS L2 lookup - Both temporary and main Data structures */
void
nsm_mpls_l2_circuit_lookup (struct nsm_master *nm, u_int32_t vc_id,
                            struct nsm_mpls_circuit_temp **vc_temp,
                            struct nsm_mpls_circuit **vc)
{
  *vc = nsm_mpls_l2_circuit_lookup_by_id (nm, vc_id);
  *vc_temp = nsm_mpls_l2_circuit_temp_lookup (vc_id, IDX_LOOKUP);
}

/** @brief Function to  loop through the L2VC's and MEsh VC's
    for snmp get request based on the vc_snmp_index

    @param struct nsm_master *nm            - NSM master 
    @param vc_index                         - vc_snmp_index
    @param struct nsm_mpls_circuit **vc     - VC structure
    @param struct nsm_mpls_circuit_temp     - vc temp structure
    @param struct nsm_vpls_peer **vpls_peer - Mesh VC structure
*/
void
nsm_mpls_vc_snmp_lookup (struct nsm_master *nm, u_int32_t vc_index,
                         struct nsm_mpls_circuit **vc,
                         struct nsm_mpls_circuit_temp **vc_temp,
                         struct nsm_vpls_peer **vpls_peer)
{
  
  *vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_index);
  if (!(*vc))
    *vc_temp = nsm_mpls_l2_circuit_snmp_temp_lookup_by_index (vc_index);
#ifdef HAVE_VPLS
  if (!(*vc) && !(*vc_temp))
    {
      if (vpls_peer != NULL)
        *vpls_peer = nsm_vpls_snmp_lookup_by_index (nm,vc_index);
    }  
#endif /*HAVE_VPLS */  
}

/** @brief Function to loop through the L2VC's and MEsh VC's
    for snmp getnext request based on the vc_snmp_index

    @param struct nsm_master *nm            - NSM master
    @param vc_index                         - vc_snmp_index
    @param struct nsm_mpls_circuit **vc     - VC structure
    @param struct nsm_mpls_circuit_temp     - vc temp structure
    @param struct nsm_vpls_peer **vpls_peer - Mesh VC structure
*/
void
nsm_mpls_vc_snmp_lookup_next (struct nsm_master *nm, u_int32_t vc_index,
                              struct nsm_mpls_circuit **vc,
                              struct nsm_mpls_circuit_temp **vc_temp,
                              struct nsm_vpls_peer **vpls_peer)
{
  struct nsm_mpls_circuit *vc_min = NULL;
  struct nsm_mpls_circuit_temp *vc_temp_min = NULL;
  u_int32_t min_vc_idx, min_vc_temp_idx, min_vpls_idx = 0;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_min = NULL;
#endif /*HAVE_VPLS */

  min_vc_idx = min_vc_temp_idx = min_vpls_idx = NSM_MPLS_L2_VC_MAX;

  vc_min = nsm_pw_snmp_get_vc_next_index (nm, NSM_MPLS->vc_table, vc_index);    
  vc_temp_min = nsm_mpls_l2_circuit_temp_snmp_lookup_next (vc_index);

#ifdef HAVE_VPLS
  if (vpls_peer != NULL)
    vpls_min = nsm_pw_snmp_get_all_peers_vpls_next_index (nm, vc_index);
#endif /*HAVE_VPLS*/   
  
  if (!vc_min && 
#ifdef HAVE_VPLS
      !vpls_min && 
#endif /*HAVE_VPLS*/   
      !vc_temp_min)
      return;
  
  if (vc_min)
    min_vc_idx = vc_min->vc_snmp_index;

  if (vc_temp_min)
    min_vc_temp_idx = vc_temp_min->vc_snmp_index;

#ifdef HAVE_VPLS  
  if (vpls_min)
    min_vpls_idx = vpls_min->vc_snmp_index;
#endif /*HAVE_VPLS */
  if (min_vc_idx < min_vc_temp_idx)
    {
#ifdef HAVE_VPLS
      if (min_vpls_idx < min_vc_idx)
        {
          if (vpls_peer)
            *vpls_peer = vpls_min;
        }
      else
#endif /* HAVE_VPLS*/
        *vc = vc_min;
    }
  else
    {
#ifdef HAVE_VPLS
      if (min_vpls_idx < min_vc_temp_idx)
        {
          if (vpls_peer)
            *vpls_peer = vpls_min;
        }
      else
#endif /* HAVE_VPLS */
        *vc_temp = vc_temp_min;
    }
}

/** @brief Function to loop through the L2VC's 
    for snmp getnext request based on the vc_snmp_index

    @param struct nsm_master *nm            - NSM master
    @param struct route_table *vc_table     - vc_table 
    @param vc_index                         - vc_snmp_index

    @return struct nsm_mpls_circuit * - VC structure
*/
struct nsm_mpls_circuit *
nsm_pw_snmp_get_vc_next_index (struct nsm_master *nm,
                              struct route_table *vc_table, 
                              u_int32_t vc_index)
{
  struct route_node *rn;
  struct nsm_mpls_circuit *vc_best_node = NULL;
  struct nsm_mpls_circuit *vc_curr_node = NULL;

  if (!vc_table)
    return NULL;

  for (rn = route_top (vc_table); rn; rn = route_next (rn))
    {
      vc_curr_node = rn->info;
      
      if (vc_curr_node)
        {
          if (vc_index < vc_curr_node->vc_snmp_index)
            {
              if (vc_best_node == NULL ||
                 (vc_curr_node->vc_snmp_index < vc_best_node->vc_snmp_index))
                vc_best_node = vc_curr_node; 
            }
        }
    }  
  return vc_best_node;
}

/** @brief Function to loop through the L2VC's 
    for snmp getnext request based on the vc_snmp_index and pw_enet_inst

    @param struct nsm_master *nm            - NSM master
    @param struct route_table *vc_table     - vc_table 
    @param vc_index                         - vc_snmp_index
    @param pw_enet_inst                     - index to identify exact binding 
                                              if vc has multiple bindings

    @return struct nsm_mpls_circuit * - VC structure
*/
struct nsm_mpls_circuit *
nsm_pw_enet_snmp_get_vc_next_index (struct nsm_master *nm,
                                    struct route_table *vc_table, 
                                    u_int32_t vc_index, u_int32_t pw_enet_inst)
{
  struct route_node *rn;
  struct nsm_mpls_circuit *vc_best_node = NULL;
  struct nsm_mpls_circuit *vc_curr_node = NULL;

  if (!vc_table)
    return NULL;

  for (rn = route_top (vc_table); rn; rn = route_next (rn))
    {
      vc_curr_node = rn->info;
      
      if (vc_curr_node && (vc_curr_node->vc_info || vc_curr_node->vc_info_temp))
        {
          if ((vc_index < vc_curr_node->vc_snmp_index) ||
              (vc_index == vc_curr_node->vc_snmp_index &&
               (vc_curr_node->vc_info ? 
                pw_enet_inst < vc_curr_node->vc_info->pw_enet_pw_instance :
               pw_enet_inst < vc_curr_node->vc_info_temp->pw_enet_pw_instance)))
            {
              if (vc_best_node == NULL ||
                 ((vc_curr_node->vc_snmp_index < vc_best_node->vc_snmp_index)))
                vc_best_node = vc_curr_node; 
            }
        }
    }  
  return vc_best_node;
}

#ifdef HAVE_VPLS

/** @brief Function to loop through the Mesh VCs
    and get the vpls_min_index

    @param vc_index                         - vc_snmp_index

    @return struct nsm_vpls_peer* - Mesh VC structure
*/

struct nsm_vpls_peer *
nsm_pw_snmp_get_all_peers_vpls_next_index (struct nsm_master *nm,
                                           u_int32_t vc_index)
{

  struct nsm_vpls *vpls = NULL;
  struct ptree_node *pn = NULL;
  struct nsm_vpls_peer *vpls_curr_node = NULL;
  struct nsm_vpls_peer *vpls_best_node = NULL;

  for (pn = ptree_top (NSM_MPLS->vpls_table); pn; pn = ptree_next (pn))
    {
      vpls = pn->info;
      if (! vpls)
        continue;

      vpls_curr_node = nsm_pw_snmp_get_vpls_next_index (nm, vpls->mp_table,
                                                       vc_index);
      if (vpls_curr_node != NULL)
        {
          if (vpls_best_node == NULL ||
              vpls_curr_node->vc_snmp_index < vpls_best_node->vc_snmp_index)
            vpls_best_node = vpls_curr_node;
        }
    }
  return vpls_best_node;
}

/** @brief Function to loop through the Mesh VCs
    for snmp getnext request based on the vc_snmp_index

    @param struct route_table *vpls_peer_table     - mp table
    @param vc_index                         - vc_snmp_index

    @return struct nsm_vpls_peer* - Mesh VC structure
*/
struct nsm_vpls_peer *
nsm_pw_snmp_get_vpls_next_index (struct nsm_master *nm,
                                struct route_table *vpls_peer_table,
                                u_int32_t vc_index)
{
  struct route_node *rn;
  struct nsm_vpls_peer *vpls_best_node = NULL;
  struct nsm_vpls_peer *vpls_curr_node = NULL;
  
  if (! vpls_peer_table)
    return NULL;
  
  for (rn = route_top (vpls_peer_table); rn; rn = route_next (rn))
    {
      vpls_curr_node = rn->info;

      if (vpls_curr_node)
        { 
          if (vc_index < vpls_curr_node->vc_snmp_index)
           {
             if (vpls_best_node == NULL ||
                (vpls_curr_node->vc_snmp_index < vpls_best_node->vc_snmp_index))
               vpls_best_node = vpls_curr_node;
           }
        }
    }
  return vpls_best_node;
}
#endif /*HAVE_VPLS*/

/* MPLS L2 lookup Next - Both temporary and main Data structures */
void
nsm_mpls_l2_circuit_lookup_next (struct nsm_master *nm, u_int32_t vc_id,
                                 struct nsm_mpls_circuit_temp **vc_temp,
                                 struct nsm_mpls_circuit **vc)
{
  *vc = nsm_mpls_l2_circuit_lookup_next_by_id (nm, vc_id);
  *vc_temp = nsm_mpls_l2_circuit_temp_lookup_next (vc_id);
}

struct nsm_mpls_circuit *
nsm_mpls_vc_snmp_lookup_next_by_intvl (struct nsm_master *nm, 
                                       u_int32_t *pw_ix,
                                       u_int32_t *pw_perf_intrvl_ix)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit *best_match = NULL;
  struct route_node *rn;
  u_int32_t vc_perf_count = 0;

  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      vc = rn->info;
      if (! vc )
        continue;

      if (vc->nsm_mpls_pw_snmp_perf_curr_list)
        vc_perf_count = LISTCOUNT(vc->nsm_mpls_pw_snmp_perf_curr_list);

      if ((*pw_ix < vc->vc_snmp_index) ||
          (((*pw_ix == vc->vc_snmp_index) &&
          (*pw_perf_intrvl_ix < vc_perf_count))))
        {
           if (best_match == NULL)
              best_match = vc;
           else if (vc->vc_snmp_index < best_match->vc_snmp_index)
               best_match = vc;
         }
    }
  
  if (best_match)
    {
      if (*pw_ix == best_match->vc_snmp_index)
        (*pw_perf_intrvl_ix)++ ;
      else if (*pw_ix < best_match->vc_snmp_index)
        *pw_perf_intrvl_ix = 1;
    }

  return best_match;
}

struct nsm_mpls_circuit *
nsm_mpls_vc_snmp_lookup_next_by_1dy_intvl (struct nsm_master *nm,
                                           u_int32_t *pw_ix,
                                           u_int32_t *pw_perf_intrvl_ix)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit *best_match = NULL;
  struct route_node *rn;

  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      vc = rn->info;
      if (! vc )
        continue;

      if ((*pw_ix < vc->vc_snmp_index) ||
         (((*pw_ix == vc->vc_snmp_index) &&
         (*pw_perf_intrvl_ix < NSM_MPLS_PW_MAX_1DY_PERF_COUNT))))
       {
         if (best_match == NULL)
           best_match = vc;
         else if (vc->vc_snmp_index < best_match->vc_snmp_index)
           best_match = vc;
       }

      if (best_match)
        {
          if (*pw_perf_intrvl_ix < NSM_MPLS_PW_MAX_1DY_PERF_COUNT)
            *pw_perf_intrvl_ix = NSM_MPLS_PW_MAX_1DY_PERF_COUNT;
        }
    }
  
  return best_match;
}

struct nsm_mpls_circuit *
nsm_mpls_l2_circuit_lookup_next_by_addr (struct nsm_master *nm,
                                         u_int32_t *pw_type,
                                         u_int32_t *peer_addr_type,       
                                         struct pal_in4_addr *peer_addr,
                                         u_int32_t *pw_ix )
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit *best_match = NULL;
  struct route_node *rn;
  u_int32_t addr_type = 0; 

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_table))
    return NULL;

  addr_type = nsm_mpls_get_vc_addr_fam (*peer_addr_type);
  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      vc = rn->info;
      if (! vc )
        continue;
      if (vc && !vc->vc_info)
        continue;
      
      if ((*pw_type < vc->vc_info->vc_type) ||
          ((*pw_type == vc->vc_info->vc_type) &&
          ((*pw_ix < vc->id) ||
          ((*pw_ix == vc->id) &&
          ((addr_type < vc->address.family) ||
          ((addr_type == vc->address.family) &&
          (vc->address.u.prefix4.s_addr < peer_addr->s_addr)))))))
         {
           if (best_match == NULL)
            {
              best_match = vc;
            }
           else if ((vc->vc_info->vc_type < best_match->vc_info->vc_type) ||
                    ((vc->vc_info->vc_type == best_match->vc_info->vc_type) &&
                    ((vc->id < best_match->id) ||
                    ((vc->id == best_match->id) &&
                    ((vc->address.family < best_match->address.family) ||
                    ((vc->address.family == best_match->address.family) &&
                    (vc->address.u.prefix4.s_addr <=
                     best_match->address.u.prefix4.s_addr)))))))
             {
               best_match = vc;
             }
         }
    }
  return best_match; 
}

/* This function needs to be changed for allowing multiple tunnels with same 
   tunnel id but different direction */
u_int32_t
nsm_mpls_getnext_tunnel_id (struct nsm_master *nm, struct nsm_mpls_circuit *vc,
                            u_int32_t *tunnel_id)
{
  u_int32_t tun_id_next = 0;
  struct route_node *rn;
  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      vc = rn->info;
      if (! vc)
        continue;
      if ((vc->ftn) && (vc->ftn->tun_id > *tunnel_id))
        {
          if ((! tun_id_next) || (vc->ftn->tun_id < tun_id_next))
            {
              tun_id_next = vc->ftn->tun_id;
            }
        }
    }
  return tun_id_next;
}

u_int32_t
nsm_mpls_getnext_tunnel_inst (struct nsm_master *nm, struct nsm_mpls_circuit *vc,
                     u_int32_t *tunnel_id, u_int32_t *tunnel_inst)
{
  u_int32_t tun_inst_next = 0;
  struct route_node *rn;
  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      vc = rn->info;
      if (! vc)
        continue;
      if ((vc->ftn) && (vc->ftn->tun_id != *tunnel_id))
        continue;
      if ((vc->ftn) && (vc->ftn->protected_lsp_id > *tunnel_inst))
        {
          if ((! tun_inst_next) || (vc->ftn->protected_lsp_id < tun_inst_next))
            {
              tun_inst_next = vc->ftn->protected_lsp_id;
            }
        }
    }
  return tun_inst_next;
}

u_int32_t
nsm_mpls_getnext_peer_lsr (struct nsm_master *nm, struct nsm_mpls_circuit *vc,
                  u_int32_t *tunnel_id, u_int32_t *tunnel_inst,
                  struct pal_in4_addr *peer_lsr)
{
  u_int32_t peer_next = 0;
  struct route_node *rn;
  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      vc = rn->info;
      if (! vc)
        continue;
      if ((vc->ftn) && ((vc->ftn->tun_id != *tunnel_id) ||
                        (vc->ftn->protected_lsp_id != *tunnel_inst)))
        continue;
      if (vc->address.u.prefix4.s_addr > peer_lsr->s_addr)
        {
          if ((! peer_next) || (vc->address.u.prefix4.s_addr < peer_next))
            {
              peer_next = vc->address.u.prefix4.s_addr;
            }
        }
    }
  return peer_next;
}

u_int32_t
nsm_mpls_getnext_lcl_lsr (struct nsm_master *nm, struct nsm_mpls_circuit *vc,
                 u_int32_t *tunnel_id, u_int32_t *tunnel_inst,
                 struct pal_in4_addr *peer_lsr, struct pal_in4_addr *lcl_lsr)
{
  u_int32_t lcl_next = 0;
  struct route_node *rn;
  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      vc = rn->info;
      if (! vc)
        continue;
      if ((vc->ftn) && ((vc->ftn->tun_id != *tunnel_id) ||
                        (vc->ftn->protected_lsp_id != *tunnel_inst) ||
                        (vc->address.u.prefix4.s_addr != peer_lsr->s_addr)))
        continue;
      if ((vc->vc_fib) && (vc->vc_fib->lsr_id > lcl_lsr->s_addr))
        {
          if ((! lcl_next) || (vc->vc_fib->lsr_id < lcl_next))
            {
              lcl_next = vc->vc_fib->lsr_id;
            }
        }
    }
  return lcl_next;
}

struct nsm_mpls_circuit *
nsm_mpls_l2_circuit_lookup_next_for_te_map (struct nsm_master *nm,
                                            u_int32_t *tunnel_id,
                                            u_int32_t *tunnel_inst,
                                            struct pal_in4_addr *peer_lsr,
                                            struct pal_in4_addr *lcl_lsr,
                                            u_int32_t *pw_ix)
{
  struct nsm_mpls_circuit *vc;
  int ret = PAL_FALSE;
  u_int32_t tun_id_next = 0;
  vc = NULL;

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_table))
    return NULL;
  do {
       ret = nsm_mpls_getnext_index_te_map_tunnel_inst (nm, &vc,
             tunnel_id, tunnel_inst, peer_lsr, lcl_lsr, pw_ix);
       if (!ret)
         {
           tun_id_next = nsm_mpls_getnext_tunnel_id (nm, vc, tunnel_id);
           *tunnel_id = tun_id_next;
         }
       else
         break;
     }while (tun_id_next);
  if (ret)
    return vc;
  else
    return NULL;
}

int
nsm_mpls_getnext_index_te_map_tunnel_inst (struct nsm_master *nm,
                                  struct nsm_mpls_circuit **vc,
                                  u_int32_t *tunnel_id,
                                  u_int32_t *tunnel_inst,
                                  struct pal_in4_addr *peer_lsr,
                                  struct pal_in4_addr *lcl_lsr,
                                  u_int32_t *pw_ix)
{
  int ret = PAL_FALSE;
  u_int32_t tun_inst_next = 0;
  do {
       ret = nsm_mpls_getnext_index_te_map_peer_lsr (nm, vc,
             tunnel_id, tunnel_inst, peer_lsr, lcl_lsr, pw_ix);
       if (!ret)
         {
           tun_inst_next = nsm_mpls_getnext_tunnel_inst (nm, *vc, tunnel_id,
                                                tunnel_inst);
           *tunnel_inst = tun_inst_next;
         }
       else
         break;
     }while (tun_inst_next);
  if (ret)
    return PAL_TRUE;
  else
    {
      *tunnel_inst = 0;
      return PAL_FALSE;
    }
}

int
nsm_mpls_getnext_index_te_map_peer_lsr (struct nsm_master *nm,
                               struct nsm_mpls_circuit **vc,
                               u_int32_t *tunnel_id,
                               u_int32_t *tunnel_inst,
                               struct pal_in4_addr *peer_lsr,
                               struct pal_in4_addr *lcl_lsr,
                               u_int32_t *pw_ix)
{
  int ret = PAL_FALSE;
  u_int32_t peer_next = 0;
  do {
       ret = nsm_mpls_getnext_index_te_map_lcl_lsr (nm, vc,
             tunnel_id, tunnel_inst, peer_lsr, lcl_lsr, pw_ix);
       if (!ret)
         {
           peer_next = nsm_mpls_getnext_peer_lsr (nm, *vc, tunnel_id,
                                         tunnel_inst, peer_lsr);
           peer_lsr->s_addr = peer_next;
         }
       else
         break;
     }while (peer_next);
  if (ret)
    return PAL_TRUE;
  else
    {
      peer_lsr->s_addr = 0;
      return PAL_FALSE;
    }
}

int
nsm_mpls_getnext_index_te_map_lcl_lsr (struct nsm_master *nm,
                              struct nsm_mpls_circuit **vc,
                              u_int32_t *tunnel_id,
                              u_int32_t *tunnel_inst,
                              struct pal_in4_addr *peer_lsr,
                              struct pal_in4_addr *lcl_lsr,
                              u_int32_t *pw_ix)
{
  int ret = PAL_FALSE;
  u_int32_t lcl_next = 0;
  do {
       ret = nsm_mpls_getnext_index_te_map_pw_ix (nm, vc,
             tunnel_id, tunnel_inst, peer_lsr, lcl_lsr, pw_ix);
       if (!ret)
         {
           lcl_next = nsm_mpls_getnext_lcl_lsr (nm, *vc, tunnel_id,
                                       tunnel_inst, peer_lsr, lcl_lsr);
           lcl_lsr->s_addr = lcl_next;
         }
       else
         break;
     }while (lcl_next);
  if (ret)
    return PAL_TRUE;
  else
    {
      lcl_lsr->s_addr = 0;
      return PAL_FALSE;
    }
}

int
nsm_mpls_getnext_index_te_map_pw_ix (struct nsm_master *nm,
                            struct nsm_mpls_circuit **vc,
                            u_int32_t *tunnel_id,
                            u_int32_t *tunnel_inst,
                            struct pal_in4_addr *peer_lsr,
                            struct pal_in4_addr *lcl_lsr,
                            u_int32_t *pw_ix)
{
  int ret = PAL_FALSE;
  struct route_node *rn;
  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      (*vc) = rn->info;
      if (! (*vc) || !((*vc)->ftn) || !((*vc)->vc_fib))
        continue;
      if ((((*vc)->owner) && ((*vc)->owner != MPLS_RSVP)) || (((*vc)->ftn) &&
                                   ((*vc)->ftn->owner.owner != MPLS_RSVP)))
        continue;
      if (((*vc)->ftn) && ((*vc)->vc_fib) &&
      (((*vc)->ftn->tun_id != *tunnel_id) ||
      ((*vc)->ftn->protected_lsp_id != *tunnel_inst) ||
      ((*vc)->vc_fib->lsr_id != lcl_lsr->s_addr) ||
      ((*vc)->address.u.prefix4.s_addr != peer_lsr->s_addr)))
        continue;
      if ((*vc)->vc_snmp_index > *pw_ix)
        {
          *pw_ix = (*vc)->vc_snmp_index;
          ret = PAL_TRUE;
          break;
        }
    }
  if (!ret)
    *pw_ix = 0;
  return ret;
}
/*lookup next support functions for mpls non te mapping table*/
u_int32_t
nsm_mpls_getnext_xc_ix (struct nsm_master *nm, struct nsm_mpls_circuit *vc,
               u_int32_t *dn, u_int32_t *xc_ix)
{
  u_int32_t xc_ix_next = 0;
  struct route_node *rn;
  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      vc = rn->info;
      if (! vc)
        continue;
      if ((vc->dn != *dn) && (vc->dn_in != *dn))
        continue;
      if ((vc->ftn) && (vc->ftn->xc) && (vc->dn == *dn) &&
          (vc->ftn->xc->key.xc_ix > *xc_ix))
        {
          if ((! xc_ix_next) || (vc->ftn->xc->key.xc_ix < xc_ix_next))
            {
              xc_ix_next = vc->ftn->xc->key.xc_ix;
            }
        }
      else
        {
          if ((vc->dn_in == *dn) && (vc->xc_ix > *xc_ix))
            {
              if ((! xc_ix_next) || (vc->xc_ix < xc_ix_next))
                {
                  xc_ix_next = vc->xc_ix;
                }
            }
        }
    }
  return xc_ix_next;
}

u_int32_t
nsm_mpls_getnext_dn (struct nsm_master *nm, struct nsm_mpls_circuit *vc,
            u_int32_t *dn)
{
  u_int32_t dn_next = 0;
  struct route_node *rn;
  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      vc = rn->info;
      if (! vc)
        continue;
      if (vc->dn > *dn)
        {
          if ((! dn_next) || (vc->dn < dn_next))
            {
              dn_next = vc->dn;
            }
        }
      else
        {
          if (vc->dn_in > *dn)
            {
              if ((! dn_next) || (vc->dn_in < dn_next))
                {
                  dn_next = vc->dn_in;
                }
            }
        }
    }
  return dn_next;
}

struct nsm_mpls_circuit *
nsm_mpls_l2_circuit_lookup_next_for_non_te_map (struct nsm_master *nm,
                                                u_int32_t *dn,
                                                u_int32_t *xc_ix,
                                                u_int32_t *pw_ix)
{
  struct nsm_mpls_circuit *vc;
  int ret = PAL_FALSE;
  u_int32_t dn_next = 0;
  vc = NULL;

  if ((! NSM_MPLS) || (! NSM_MPLS->vc_table))
    return NULL;
  do {
       ret = nsm_mpls_getnext_index_non_te_map_xc_ix (nm, &vc, dn, xc_ix, pw_ix);
       if (!ret)
         {
           dn_next = nsm_mpls_getnext_dn (nm, vc, dn);
           *dn = dn_next;
         }
       else
         break;
     }while (dn_next);
  if (ret)
    return vc;
  else
    return NULL;
}

int
nsm_mpls_getnext_index_non_te_map_xc_ix (struct nsm_master *nm,
                                struct nsm_mpls_circuit **vc,
                                u_int32_t *dn,
                                u_int32_t *xc_ix,
                                u_int32_t *pw_ix)
{
  int ret = PAL_FALSE;
  u_int32_t xc_ix_next = 0;
  do {
       ret = nsm_mpls_getnext_index_non_te_map_pw_ix (nm, vc, dn, xc_ix, pw_ix);
       if (!ret)
         {
           xc_ix_next = nsm_mpls_getnext_xc_ix (nm, *vc, dn, xc_ix);
           *xc_ix = xc_ix_next;
         }
       else
         break;
     }while (xc_ix_next);
  if (ret)
    return PAL_TRUE;
  else
    {
      *xc_ix = 0;
      return PAL_FALSE;
    }
}

int
nsm_mpls_getnext_index_non_te_map_pw_ix (struct nsm_master *nm,
                                struct nsm_mpls_circuit **vc,
                                u_int32_t *dn,
                                u_int32_t *xc_ix,
                                u_int32_t *pw_ix)
{
  int ret = PAL_FALSE;
  struct route_node *rn;
  for (rn = route_top (NSM_MPLS->vc_table); rn; rn = route_next (rn))
    {
      (*vc) = rn->info;
      if (! (*vc) || !((*vc)->ftn) || !((*vc)->vc_fib))
        continue;
      if ((((*vc)->ftn) && ((*vc)->ftn->xc) &&
      (((*vc)->dn != *dn) ||
      ((*vc)->ftn->xc->key.xc_ix != *xc_ix))) &&
       (((*vc)->dn_in != *dn) || ((*vc)->xc_ix != *xc_ix)))
        continue;
      if ((*vc)->vc_snmp_index > *pw_ix)
        {
          *pw_ix = (*vc)->vc_snmp_index;
          ret = PAL_TRUE;
          break;
        }
    }
  if (!ret)
    *pw_ix = 0;
  return ret;
}
/* VC Timer handler */
s_int32_t
vc_timer (struct thread *t_time_elpsd)
{
  struct lib_globals *zg;
  struct nsm_mpls_circuit *vc;
  struct nsm_mpls_pw_snmp_perf_curr *curr = NULL;
  struct nsm_mpls_pw_snmp_perf_curr *node = NULL;
#ifdef HAVE_HAL  
  int ret = 0;
#endif /*HAVE_HAL */  
  
 /* Obtain the Lib Global pointer */
  zg = THREAD_GLOB (t_time_elpsd);
  
  if (!zg)
    return -1;

  /* Obtain the VC structure */
  vc = THREAD_ARG (t_time_elpsd);

  if (vc)
    {
      /* create list for maintaining counters*/
      if (!vc->nsm_mpls_pw_snmp_perf_curr_list)
        {
          vc->nsm_mpls_pw_snmp_perf_curr_list = list_new ();
        }
      curr = XMALLOC (MTYPE_NSM_VC_PERF_CNTR,
              sizeof (struct nsm_mpls_pw_snmp_perf_curr));

      if (!curr)
        return -1;

      pal_mem_set (curr, 0, sizeof (struct nsm_mpls_pw_snmp_perf_curr));
#ifdef HAVE_HAL      
      ret = hal_mpls_pw_get_perf_cntr (curr);
      if (ret != 0)
        {
          XFREE (MTYPE_NSM_VC_PERF_CNTR, curr);
          return -1;
        }
      else
#endif /*HAVE_HAL*/
      if (LISTCOUNT(vc->nsm_mpls_pw_snmp_perf_curr_list) 
                      != NSM_MPLS_PW_MAX_PERF_COUNT)
        listnode_add (vc->nsm_mpls_pw_snmp_perf_curr_list , curr);
      else
        {
          node = listnode_head (vc->nsm_mpls_pw_snmp_perf_curr_list);
          listnode_delete (vc->nsm_mpls_pw_snmp_perf_curr_list, node);
          listnode_add (vc->nsm_mpls_pw_snmp_perf_curr_list , curr);
        }        
      
      vc->valid_intrvl_cnt++;
      vc->t_time_elpsd = NULL;
      VC_TIMER_ON(nzg, vc->t_time_elpsd, vc, vc_timer, VC_TIMER_15MIN);
    }
  return 0;
}
/* API for PW MIB Scalars */
u_int32_t
nsm_mpls_get_pw_index_next (struct nsm_master *nm, u_int32_t *ret)
{
  *ret = bitmap_get_first_free_index (NSM_MPLS->vc_indx_mgr); 

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_tot_err_pckts (struct nsm_master *nm, u_int32_t *ret)
{
  *ret = 0;
  return NSM_API_GET_SUCCESS;
}

/* APIs for PW MIB VC Table */
u_int32_t
nsm_mpls_get_pw_type (struct nsm_master *nm,
                      u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /*HAVE_VPLS */
  vc_id = pw_ix;
  
#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else  
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL); 
#endif /*HAVE_VPLS */ 
 
  if ((vc))
    {
      if (vc->vc_info)
        *ret = vc->vc_info->vc_type;
      else
        *ret = 0;
    } 
  else if (vc_temp)
    *ret = vc_temp->vc_type;
#ifdef HAVE_VPLS
   else if (vpls_peer)
    {
      vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        {
          pw_ix = vpls_peer->vc_snmp_index;
          *ret = vpls->vpls_type;
        }
      else
        {
           pw_ix = vpls_peer->vc_snmp_index;
           return NSM_API_GET_ERROR;
        }
    } 
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_type (struct nsm_master *nm,
                           u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id = *pw_ix ;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS  
  struct nsm_vpls_peer *vpls_peer = NULL;  
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  
#ifndef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#else
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#endif /*HAVE_VPLS */  

  if ((vc))
    {
      if (vc->vc_info)
       {
         *ret = vc->vc_info->vc_type;
         *pw_ix = vc->vc_snmp_index;
       }
    else
      {
        *ret = 0;
        *pw_ix = vc->vc_snmp_index;
      } 
    }
  else if (vc_temp)
    {
      *ret = vc_temp->vc_type;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS  
   else if (vpls_peer)
    {
      vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        {
          *pw_ix = vpls_peer->vc_snmp_index;
          *ret = vpls->vpls_type;
        }
      else
        {
          *pw_ix = vpls_peer->vc_snmp_index;
          return NSM_API_GET_ERROR;
        }
     }
#endif /*HAVE_VPLS */   
   else
     {
       *pw_ix = 0;
       return NSM_API_GET_ERROR;
     }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_owner (struct nsm_master *nm,
                       u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
 vc_id = pw_ix;
 
#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */
          
  if (vc) 
    *ret = vc->fec_type_vc;
  else if (vc_temp)
    *ret = vc_temp->pw_owner;
#ifdef HAVE_VPLS
   else if (vpls_peer)
     *ret = vpls_peer->fec_type_vc;
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_owner (struct nsm_master *nm,
                            u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;
  
#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
       *ret = vc->fec_type_vc;
       *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = vc_temp->pw_owner;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
       *ret = vpls_peer->fec_type_vc;
       *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

   return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_psn_type (struct nsm_master *nm,
                          u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;
  
#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    *ret = PW_PSN_TYPE_MPLS;
  else if (vc_temp)
    *ret = PW_PSN_TYPE_MPLS;
#ifdef HAVE_VPLS
   else if (vpls_peer)
     *ret = PW_PSN_TYPE_MPLS;
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_psn_type (struct nsm_master *nm,
                               u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;
  
#ifdef HAVE_VPLS
   nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc)
    {
      *ret = PW_PSN_TYPE_MPLS;
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = PW_PSN_TYPE_MPLS;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = PW_PSN_TYPE_MPLS;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_setup_prty (struct nsm_master *nm,
                            u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc)
    *ret = vc->setup_prty;
  else if (vc_temp)
    *ret = vc_temp->setup_prty;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = NSM_MPLS_PW_DEF_SETUP_PRIO;
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_setup_prty (struct nsm_master *nm,
                                 u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
   vc_id = *pw_ix;
   
#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc)
    {
      *ret = vc->setup_prty;
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = vc_temp->setup_prty;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = NSM_MPLS_PW_DEF_SETUP_PRIO;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_hold_prty (struct nsm_master *nm,
                           u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;
  
#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */


  if (vc)
    *ret = vc->hold_prty;
  else if (vc_temp)
    *ret = vc_temp->hold_prty;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = NSM_MPLS_PW_DEF_HOLD_PRIO;
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;
 
   return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_hold_prty (struct nsm_master *nm,
                                u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;
  
#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc)
    {
      *ret = vc->hold_prty;
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = vc_temp->hold_prty;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = NSM_MPLS_PW_DEF_SETUP_PRIO;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_peer_addr_type (struct nsm_master *nm,
                                u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;
  
#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc)
    {
      if (vc->address.family == AF_INET)
        *ret = PW_PEER_ADDR_TYPE_IPV4;
      else
        *ret = PW_PEER_ADDR_TYPE_UNKNOWN;
    }
  else if (vc_temp)
    *ret = vc_temp->peer_addr_type;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = PW_PEER_ADDR_TYPE_IPV4;
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}
u_int32_t
nsm_mpls_get_next_pw_peer_addr_type (struct nsm_master *nm,
                                     u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc)
    {
      if (vc->address.family == AF_INET)
        *ret = PW_PEER_ADDR_TYPE_IPV4;
      else
        *ret = PW_PEER_ADDR_TYPE_UNKNOWN;
      
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      if (vc_temp->peer_addr_type == AF_INET)
        *ret = PW_PEER_ADDR_TYPE_IPV4;
      else
        *ret = PW_PEER_ADDR_TYPE_UNKNOWN;
      
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = PW_PEER_ADDR_TYPE_IPV4;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_peer_addr (struct nsm_master *nm,
                           u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;
  
#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

   if (vc)
    {
      pal_inet_ntop (AF_INET,
                    &(vc->address.u.prefix4),
                    *ret,
                    NSM_PW_MAX_STRING_LENGTH);
    }
  else if (vc_temp)
    {
      pal_inet_ntop (AF_INET,
                    &(vc_temp->peer_addr),
                    *ret,
                    NSM_PW_MAX_STRING_LENGTH);
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      pal_inet_ntop (AF_INET,
                    &(vpls_peer->peer_addr.u.ipv4),
                    *ret,
                     NSM_PW_MAX_STRING_LENGTH);
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;
 
   return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_peer_addr (struct nsm_master *nm,
                                u_int32_t *pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc)
    {
      pal_inet_ntop (AF_INET,
                    &(vc->address.u.prefix4),
                    *ret,
                    NSM_PW_MAX_STRING_LENGTH);

      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      pal_inet_ntop (AF_INET,
                    &(vc_temp->peer_addr),
                    *ret,
                    NSM_PW_MAX_STRING_LENGTH);

      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      pal_inet_ntop (AF_INET,
                    &(vpls_peer->peer_addr.u.ipv4),
                    *ret,
                    NSM_PW_MAX_STRING_LENGTH);
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_attchd_pw_ix (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc)
    {
      if (vc->vc_info)
        *ret = vc->vc_info->pw_attchd_pw_ix;
      else
        *ret = 0;
    }
  else if (vc_temp)
    *ret = vc_temp->pw_attchd_pw_ix;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = NSM_MPLS_PW_PEER_ATTCH_IX;
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;
 
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_attchd_pw_ix (struct nsm_master *nm,
                                   u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;
  
#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
       if (vc->vc_info)
         *ret = vc->vc_info->pw_attchd_pw_ix;
       else
         *ret = 0;
      
       *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = vc_temp->pw_attchd_pw_ix;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = NSM_MPLS_PW_PEER_ATTCH_IX;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_if_ix (struct nsm_master *nm,
                       u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;
  
#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc)
    *ret = NSM_MPLS_PW_IF_INDEX;
  else if (vc_temp)
    *ret = NSM_MPLS_PW_IF_INDEX;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = NSM_MPLS_PW_IF_INDEX;
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;
  
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_if_ix (struct nsm_master *nm,
                            u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;
  
#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc)
    {
      *ret = NSM_MPLS_PW_IF_INDEX;
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = NSM_MPLS_PW_IF_INDEX;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = NSM_MPLS_PW_IF_INDEX;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_id (struct nsm_master *nm,
                    u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);  
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */
 
  if (vc)
    *ret = vc->id;
  else if (vc_temp)
    *ret = vc_temp->vc_id;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = vpls_peer->vpls_id;
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}
 
u_int32_t
nsm_mpls_get_next_pw_id (struct nsm_master *nm,
                         u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */
 
  if (vc)
    {
      *ret = vc->id;;
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = 0;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = vpls_peer->vpls_id;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_local_grp_id (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;
  
#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      if (vc->group)
        *ret = vc->group->id;
      else if (vc->vc_info &&
               vc->vc_info->mif && vc->vc_info->mif->ifp)
        *ret = vc->vc_info->mif->ifp->ifindex;
      else
        *ret = 0;
    }
  else if (vc_temp)
    *ret = vc_temp->local_grp_id;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        *ret = vpls->grp_id;
      else
        return NSM_API_GET_ERROR;
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_local_grp_id (struct nsm_master *nm,
                                   u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */
      
  if ((vc))
    {
      if (vc->group)
        *ret = vc->group->id;
      else if (vc->vc_info &&
                vc->vc_info->mif && vc->vc_info->mif->ifp)
        *ret = vc->vc_info->mif->ifp->ifindex;
      else
        *ret = 0;
      
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = vc_temp->local_grp_id;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        {
          *pw_ix = vpls_peer->vc_snmp_index;
          *ret = vpls->grp_id;
        }
      else
        {
          *pw_ix = vpls_peer->vc_snmp_index;
           return NSM_API_GET_ERROR;
        }
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_grp_attchmt_id (struct nsm_master *nm,
                                u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  u_int32_t retval = 0;
  vc_id = pw_ix;
 
#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc)
    {
      if (vc->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else 
        pal_sprintf (*ret, "%u", vc->id);
    }
  else if (vc_temp)
    {
      if (vc_temp->pw_owner != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%u", vc_temp->vc_id);
    } 
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if (vpls_peer->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%u", vpls_peer->vpls_id); 
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

   return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_grp_attchmt_id (struct nsm_master *nm,
                                     u_int32_t *pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  u_int32_t retval = 0;
  vc_id = *pw_ix;
  
#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      if (vc->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%u", vc->id);
      
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      if (vc_temp->pw_owner != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%u", vc_temp->vc_id);

      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if (vpls_peer->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%u", vpls_peer->vpls_id);

      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_local_attchmt_id (struct nsm_master *nm,
                                  u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  u_int32_t retval = 0;
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      if (vc->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%d",  vc->local_attchmt_id);
    }
  else if (vc_temp)
    {
      if (vc_temp->pw_owner != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%d",  vc_temp->local_attchmt_id);
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if (vpls_peer->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%d", NSM_PW_VPLS_LOC_ATTCH_ID);
    }
#endif /*HAVE_VPLS */
  else 
    return NSM_API_GET_ERROR;
 
   return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_local_attchmt_id (struct nsm_master *nm,
                                       u_int32_t *pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  u_int32_t retval = 0;
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      if (vc->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%d", vc->local_attchmt_id);

      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      if (vc_temp->pw_owner != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%d", vc_temp->local_attchmt_id);

      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if (vpls_peer->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%d", NSM_PW_VPLS_LOC_ATTCH_ID);

      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif  /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_peer_attchmt_id (struct nsm_master *nm,
                                 u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  u_int32_t retval = 0;
  vc_id = pw_ix;
 
#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      if (vc->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%d", vc->peer_attchmt_id);
    }
  else if (vc_temp)
    {
      if (vc_temp->pw_owner != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%d", vc_temp->peer_attchmt_id);
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if (vpls_peer->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%d", NSM_PW_VPLS_PEER_ATTCH_ID);
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

   return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_peer_attchmt_id (struct nsm_master *nm,
                                      u_int32_t *pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  u_int32_t retval = 0;
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

   if ((vc))
    {
      if (vc->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%d", vc->peer_attchmt_id);
  
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      if (vc_temp->pw_owner != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%d", vc_temp->peer_attchmt_id);
  
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if (vpls_peer->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING)
        pal_sprintf (*ret, "%d", retval);
      else
        pal_sprintf (*ret, "%d", NSM_PW_VPLS_PEER_ATTCH_ID);

      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_cw_prfrnce (struct nsm_master *nm,
                            u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls *vpls = NULL;
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      if (vc->cw)
        *ret = NSM_MPLS_PW_PAL_TRUE;
      else
        *ret = NSM_MPLS_PW_PAL_FALSE;
    }
  else if (vc_temp)
    {
      if (vc_temp->cw)
        *ret = NSM_MPLS_PW_PAL_TRUE;
      else
        *ret = NSM_MPLS_PW_PAL_FALSE;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        {
          if (vpls->c_word)
           *ret = NSM_MPLS_PW_PAL_TRUE;
          else
           *ret = NSM_MPLS_PW_PAL_FALSE;
        }
      else
        return NSM_API_GET_ERROR;
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;
 
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_cw_prfrnce (struct nsm_master *nm,
                                 u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls *vpls = NULL;
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

   if ((vc))
    {
      if (vc->cw)
         *ret = NSM_MPLS_PW_PAL_TRUE;
       else
         *ret = NSM_MPLS_PW_PAL_FALSE;

      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      if (vc_temp->cw)
         *ret = NSM_MPLS_PW_PAL_TRUE;
       else
         *ret = NSM_MPLS_PW_PAL_FALSE;
      
     *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        {
          if (vpls->c_word)
            *ret = NSM_MPLS_PW_PAL_TRUE;
          else
            *ret = NSM_MPLS_PW_PAL_FALSE;

          *pw_ix = vpls_peer->vc_snmp_index;
        }
      else
        {
          *pw_ix = vpls_peer->vc_snmp_index;
          return NSM_API_GET_ERROR;
        }
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_local_if_mtu (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

   if (vc) 
     {
       if (vc->vc_info)
         *ret = vc->vc_info->mif->ifp->mtu;
       else 
         *ret = 0;
     }
  else if (vc_temp)
     *ret = vc_temp->mtu;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        {
          pw_ix = vpls_peer->vc_snmp_index;
          *ret = vpls->ifmtu;
        }
      else
        return NSM_API_GET_ERROR;
    }
#endif /*HAVE_VPLS */
   else
     return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_local_if_mtu (struct nsm_master *nm,
                                   u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      if (vc->vc_info)
        *ret = vc->vc_info->mif->ifp->mtu;
      else
        *ret = 0;

      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = vc_temp->mtu;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        {
          *pw_ix = vpls_peer->vc_snmp_index;
          *ret = vpls->ifmtu;
        }
      else
        {
          *pw_ix = vpls_peer->vc_snmp_index;
          return NSM_API_GET_ERROR;
        }
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_local_if_string (struct nsm_master *nm,
                                 u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;
  
#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    *ret = NSM_MPLS_PW_PAL_FALSE;
  else if (vc_temp)
    *ret = NSM_MPLS_PW_PAL_FALSE;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = NSM_MPLS_PW_PAL_FALSE;
#endif
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_local_if_string (struct nsm_master *nm,
                                      u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

   if ((vc))
    {
      *ret = NSM_MPLS_PW_PAL_FALSE;
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = NSM_MPLS_PW_PAL_FALSE;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = NSM_MPLS_PW_PAL_FALSE;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_local_capab_advrt (struct nsm_master *nm,
                                   u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  u_int32_t retval = 0;
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    pal_sprintf (*ret, "%d", retval);
  else if (vc_temp)
    pal_sprintf (*ret, "%d", retval);
#ifdef HAVE_VPLS
  else if (vpls_peer)
    pal_sprintf (*ret, "%d", retval);
#endif /* HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_local_capab_advrt (struct nsm_master *nm,
                                        u_int32_t *pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  u_int32_t retval = 0;
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      pal_sprintf (*ret, "%d", retval);
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      pal_sprintf (*ret, "%d", retval);
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      pal_sprintf (*ret, "%d", retval);
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /* HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_remote_grp_id (struct nsm_master *nm,
                               u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc)
    {
      if ((vc->vc_info) &&
         ((vc->state == NSM_MPLS_L2_CIRCUIT_ACTIVE) ||
         (vc->state == NSM_MPLS_L2_CIRCUIT_UP)))
       {
         if ((vc->state == NSM_MPLS_L2_CIRCUIT_ACTIVE) ||
            (vc->state == NSM_MPLS_L2_CIRCUIT_UP))
          {
            if (vc->remote_grp_id)
              *ret = vc->remote_grp_id;
            else if (vc->group)
              *ret = vc->group->id;
          }
       }
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
     {
      vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        *ret = vpls->grp_id;
      else
        return NSM_API_GET_ERROR;
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_remote_grp_id (struct nsm_master *nm,
                                    u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc)
    {
      if ((vc->vc_info) &&
         ((vc->state == NSM_MPLS_L2_CIRCUIT_ACTIVE) ||
         (vc->state == NSM_MPLS_L2_CIRCUIT_UP)))
       {
         if ((vc->state == NSM_MPLS_L2_CIRCUIT_ACTIVE) ||
            (vc->state == NSM_MPLS_L2_CIRCUIT_UP))
          {
            if (vc->remote_grp_id)
              *ret = vc->remote_grp_id;
            else if (vc->group)
              *ret = vc->group->id;
          }
      }
      *pw_ix = vc->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        {
          *pw_ix = vpls_peer->vc_snmp_index;
          *ret = vpls->grp_id;
        }
      else
        {
          *pw_ix = vpls_peer->vc_snmp_index;
           return NSM_API_GET_ERROR;
        }
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }
 
   return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_cw_status (struct nsm_master *nm,
                           u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  u_int32_t cw_set = 0;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */
  if ((vc ))
    cw_set = vc->cw;
  else if (vc_temp)
    cw_set = vc_temp->cw_status;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
       vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        {
          pw_ix = vpls_peer->vc_snmp_index;
          *ret = vpls->c_word;
        }
      else
        {
          pw_ix = vpls_peer->vc_snmp_index;
          return NSM_API_GET_ERROR;
        }
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  if (cw_set == 1)
    *ret = PW_CW_SET;
  else 
    *ret = PW_CW_NOT_SET;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_cw_status (struct nsm_master *nm,
                                u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  u_int32_t cw_set = 0;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc)
    {
      cw_set = vc->cw;
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      cw_set = vc_temp->cw_status;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
       vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        {
          *pw_ix = vpls_peer->vc_snmp_index;
          *ret = vpls->c_word;
        }
      else
        {
          *pw_ix = vpls_peer->vc_snmp_index;
          return NSM_API_GET_ERROR;
        }
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  if (cw_set == 1)
    *ret = PW_CW_SET;
  else
    *ret = PW_CW_NOT_SET;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_remote_if_mtu (struct nsm_master *nm,
                               u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL; 
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc )
    {
      if (vc->vc_info)
       { 
         if ((vc->state == NSM_MPLS_L2_CIRCUIT_ACTIVE) ||
            (vc->state == NSM_MPLS_L2_CIRCUIT_UP))
           *ret = vc->vc_info->mif->ifp->mtu;
       }
     else
       *ret = 0;
    }
  else if (vc_temp)
    *ret = 0;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if ((vpls_peer->state == NSM_VPLS_PEER_ACTIVE)  ||
          (vpls_peer->state == NSM_VPLS_PEER_UP))
        {
          vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
          if (vpls)
            *ret = vpls->ifmtu;
          else
            return NSM_API_GET_ERROR;
        }
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_remote_if_mtu (struct nsm_master *nm,
                                    u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc ))
    {
      if ((vc->state == NSM_MPLS_L2_CIRCUIT_ACTIVE) ||
         (vc->state == NSM_MPLS_L2_CIRCUIT_UP))
       {
         if (vc->vc_info)
           {
             *ret = vc->vc_info->mif->ifp->mtu;
             *pw_ix = vc->vc_snmp_index;
           }
         else
           {
             *ret = 0;
             *pw_ix = vc->vc_snmp_index;
           }
       }
     else
       {
         *ret = 0;
         *pw_ix = vc->vc_snmp_index;
       }
    }
  else if (vc_temp)
    {
      *ret = 0;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if ((vpls_peer->state == NSM_VPLS_PEER_ACTIVE)  ||
          (vpls_peer->state == NSM_VPLS_PEER_UP))
        {    
          vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
          if (vpls)
           {
             *pw_ix = vpls_peer->vc_snmp_index;
             *ret = vpls->ifmtu;
           }
         else
           {
             *pw_ix = vpls_peer->vc_snmp_index;
             return NSM_API_GET_ERROR;
           }
       }
     *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_remote_if_string (struct nsm_master *nm,
                                  u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    *ret = 0;
  else if (vc_temp)
    *ret = 0;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = 0;
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_remote_if_string (struct nsm_master *nm,
                                       u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      *ret = 0;
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = 0;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = 0;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_remote_capab (struct nsm_master *nm,
                              u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;
  u_int32_t retval = 0;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

   if ((vc))
    pal_sprintf (*ret, "%d", retval);
  else if (vc_temp)
    pal_sprintf (*ret, "%d", retval);
#ifdef HAVE_VPLS
  else if (vpls_peer)
    pal_sprintf (*ret, "%d", retval);
#endif /* HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_remote_capab (struct nsm_master *nm,
                                   u_int32_t *pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;
  u_int32_t retval = 0;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      pal_sprintf (*ret, "%d", retval);
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      pal_sprintf (*ret, "%d", retval);
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      pal_sprintf (*ret, "%d", retval);
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /* HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_frgmt_cfg_size (struct nsm_master *nm,
                                u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    *ret = 0;
  else if (vc_temp)
    *ret = 0;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = 0;
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_frgmt_cfg_size (struct nsm_master *nm,
                                     u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      *ret = 0;
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = 0;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = 0;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_rmt_frgmt_capab (struct nsm_master *nm,
                                 u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;
  u_int32_t retval = 0;
 
#ifdef HAVE_VPLS
   nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    pal_sprintf (*ret, "%d", retval);
  else if (vc_temp)
    pal_sprintf (*ret, "%d", retval);
#ifdef HAVE_VPLS
  else if (vpls_peer)
    pal_sprintf (*ret, "%d", retval);
#endif /* HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;
  
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_rmt_frgmt_capab (struct nsm_master *nm,
                                      u_int32_t *pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;
  u_int32_t retval = 0;

#ifdef HAVE_VPLS
   nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      pal_sprintf (*ret, "%d", retval);
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      pal_sprintf (*ret, "%d", retval);
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      pal_sprintf (*ret, "%d", retval);
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif  /* HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}
u_int32_t
nsm_mpls_get_pw_fcs_retentn_cfg (struct nsm_master *nm,
                                 u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    *ret = FCS_RETENTN_DISABLE;
  else if (vc_temp)
    *ret = FCS_RETENTN_DISABLE;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = FCS_RETENTN_DISABLE;
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_fcs_retentn_cfg (struct nsm_master *nm,
                                      u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      *ret = FCS_RETENTN_DISABLE;
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = FCS_RETENTN_DISABLE;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = FCS_RETENTN_DISABLE;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_fcs_retentn_status (struct nsm_master *nm,
                                    u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;
  
#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    pal_sprintf (*ret, "%d", FCS_RETENTN_STATUS_DIS);
  else if (vc_temp)
    pal_sprintf (*ret, "%d", FCS_RETENTN_STATUS_DIS);
#ifdef HAVE_VPLS
  else if (vpls_peer)
    pal_sprintf (*ret, "%d", FCS_RETENTN_STATUS_DIS);
#endif /* HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_fcs_retentn_status (struct nsm_master *nm,
                                         u_int32_t *pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
 vc_id = *pw_ix;

#ifdef HAVE_VPLS
   nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
     nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
       pal_sprintf (*ret, "%d", FCS_RETENTN_STATUS_DIS);
       *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      pal_sprintf (*ret, "%d", FCS_RETENTN_STATUS_DIS);
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      pal_sprintf (*ret, "%d", FCS_RETENTN_STATUS_DIS);
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /* HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_outbd_label (struct nsm_master *nm,
                             u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc) 
   {
     if (vc->vc_fib)
      *ret = vc->vc_fib->out_label;
     else
      *ret = 0;
  }
  else if (vc_temp && vc_temp->vc_fib_temp)
    *ret = vc_temp->vc_fib_temp->out_label;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if (vpls_peer->vc_fib)
        *ret = vpls_peer->vc_fib->out_label;
      else
        *ret = 0;
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_outbd_label (struct nsm_master *nm,
                                  u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc) )
    {
      if (vc->vc_fib)
       {
         *ret = vc->vc_fib->out_label;
         *pw_ix = vc->vc_snmp_index;
       } 
     else
       {
         *ret = 0;
         *pw_ix = vc->vc_snmp_index;
       } 
  }
  else if (vc_temp && vc_temp->vc_fib_temp)
    {
      *ret = vc_temp->vc_fib_temp->out_label;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if ((vpls_peer) && (vpls_peer->vc_fib))
    {
      if (vpls_peer->vc_fib)
        {
          *ret = vpls_peer->vc_fib->out_label;
          *pw_ix = vpls_peer->vc_snmp_index;
        }
     else
        {
          *ret = 0;
          *pw_ix = vpls_peer->vc_snmp_index;
        } 
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_inbd_label (struct nsm_master *nm,
                            u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc) )
    {
      if (vc->vc_fib)
        *ret = vc->vc_fib->in_label;
      else
        *ret = 0;
    }
  else if (vc_temp && vc_temp->vc_fib_temp)
    *ret = vc_temp->vc_fib_temp->in_label;
#ifdef HAVE_VPLS
  else if ((vpls_peer) )
    {
      if (vpls_peer->vc_fib)
       *ret = vpls_peer->vc_fib->in_label;
      else
       *ret  = 0;
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_inbd_label (struct nsm_master *nm,
                                 u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
 vc_id = *pw_ix;

#ifdef HAVE_VPLS
   nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
     nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
   {
     if (vc->vc_fib)
      {
        *ret = vc->vc_fib->in_label;
        *pw_ix = vc->vc_snmp_index;
      }
    else
      { 
        *ret = 0;
        *pw_ix = vc->vc_snmp_index;
      }
    }
  else if (vc_temp && vc_temp->vc_fib_temp)
    {
      *ret = vc_temp->vc_fib_temp->in_label;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if ((vpls_peer)&& (vpls_peer->vc_fib))
    {
      if (vpls_peer->vc_fib)
       {
         *ret = vpls_peer->vc_fib->in_label;
         *pw_ix = vpls_peer->vc_snmp_index;
       }
     else
       {
         *ret = 0;
         *pw_ix = vpls_peer->vc_snmp_index;
       }
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_name (struct nsm_master *nm,
                      u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      if (vc->name)
        {
          pal_snprintf (*ret, NSM_PW_MAX_STRING_LENGTH, "%s",
                        vc->name);
        }
    }
  else if (vc_temp)
    {
      if (vc_temp->vc_name)
        {
          pal_snprintf (*ret, NSM_PW_MAX_STRING_LENGTH, "%s",
                        vc_temp->vc_name);
        }
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        {
          pal_snprintf (*ret, NSM_PW_MAX_STRING_LENGTH, "%s",
                        vpls->vpls_name);
        }
      else
        return NSM_API_GET_ERROR;
    }
#endif /*HAVE_VPLS */
    else
      return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_name (struct nsm_master *nm,
                           u_int32_t *pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc) )
    {
      if (vc->name)
        {
          *pw_ix = vc->vc_snmp_index;
          pal_snprintf (*ret, NSM_PW_MAX_STRING_LENGTH, "%s",
                        vc->name);
        }
    }
  else if (vc_temp)
    {
      if (vc_temp->vc_name)
        {
          *pw_ix = vc_temp->vc_snmp_index;
          pal_snprintf (*ret, NSM_PW_MAX_STRING_LENGTH, "%s",
                        vc_temp->vc_name);
        }
      else
        *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        {
          *pw_ix = vpls_peer->vc_snmp_index;
          pal_snprintf (*ret, NSM_PW_MAX_STRING_LENGTH, "%s",
                        vpls->vpls_name);
        }
      else
        {
          *pw_ix = vpls_peer->vc_snmp_index;
          return NSM_API_GET_ERROR;
        }
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_descr (struct nsm_master *nm,
                       u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc) )
    {
          if (vc->name)
           {
             pal_snprintf (*ret, NSM_PW_MAX_STRING_LENGTH, "%s",
                           vc->dscr);
           }
    }
  else if (vc_temp)
    {
      if (vc_temp->vc_name)
        {
          pal_snprintf (*ret, NSM_PW_MAX_STRING_LENGTH, "%s",
                        vc_temp->dscr);
        }
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        {
          pal_snprintf (*ret, NSM_PW_MAX_STRING_LENGTH, "%s",
                        vpls->desc);
        }
      else
        return NSM_API_GET_ERROR;
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_descr (struct nsm_master *nm,
                            u_int32_t *pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc) )
    {
      if (vc->name)
        {
          pal_snprintf (*ret, NSM_PW_MAX_STRING_LENGTH, "%s",
                        vc->dscr);
          *pw_ix = vc->vc_snmp_index;
        }
    }
  else if (vc_temp)
    {
      if (vc_temp->vc_name)
        {
          pal_snprintf (*ret, NSM_PW_MAX_STRING_LENGTH, "%s",
                        vc_temp->dscr);
         *pw_ix = vc_temp->vc_snmp_index;
       }
     else
       *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
      if (vpls)
        {
          *pw_ix = vpls_peer->vc_snmp_index;
           pal_snprintf (*ret, NSM_PW_MAX_STRING_LENGTH, "%s",
                         vpls->desc);
        }
      else
        {
          *pw_ix = vpls_peer->vc_snmp_index;
          return NSM_API_GET_ERROR;
        }
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_create_time (struct nsm_master *nm,
                             u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    *ret = vc->create_time * VC_SNMP_TIMETICKS;
  else if (vc_temp)
    *ret = 0;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = vpls_peer->create_time * VC_SNMP_TIMETICKS;
#endif /*HAVE_VPLS */
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_create_time (struct nsm_master *nm,
                                  u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
      
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      *ret = vc->create_time * VC_SNMP_TIMETICKS;
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = 0;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = vpls_peer->create_time * VC_SNMP_TIMETICKS;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_up_time (struct nsm_master *nm,
                         u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
       *ret = (vc->uptime == 0) ? 0 :
            ((nsm_get_sys_up_time() - vc->uptime) * VC_SNMP_TIMETICKS);
    }
  else if (vc_temp)
    *ret = 0;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = (vpls_peer->up_time == 0) ? 0 :
          ((nsm_get_sys_up_time() - vpls_peer->up_time) * VC_SNMP_TIMETICKS);
#endif  /*HAVE_VPLS */
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_up_time (struct nsm_master *nm,
                              u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
       *ret = (vc->uptime == 0) ? 0 :
            ((nsm_get_sys_up_time() - vc->uptime) * VC_SNMP_TIMETICKS);
       *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = 0;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = (vpls_peer->up_time == 0) ? 0 :
           ((nsm_get_sys_up_time() - vpls_peer->up_time)* VC_SNMP_TIMETICKS);
      
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_last_change (struct nsm_master *nm,
                             u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    *ret = vc->last_change * VC_SNMP_TIMETICKS;
  else if (vc_temp)
    *ret = 0;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = vpls_peer->last_change * VC_SNMP_TIMETICKS;;
#endif
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_last_change (struct nsm_master *nm,
                                  u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
       *ret = vc->last_change * VC_SNMP_TIMETICKS;
       *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = 0;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = vpls_peer->last_change * VC_SNMP_TIMETICKS;;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_admin_status (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    *ret = vc->admin_status;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = PW_SNMP_ADMIN_UP;
#endif /*HAVE_VPLS */
  else if (vc_temp)
    *ret = vc_temp->admin_status;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_admin_status (struct nsm_master *nm,
                                   u_int32_t *pw_ix, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, *pw_ix, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, *pw_ix, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      *ret = vc->admin_status;
      *pw_ix = vc->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = PW_SNMP_ADMIN_UP;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else if (vc_temp)
    {
      *ret = vc_temp->admin_status;
      *pw_ix = vc_temp->vc_snmp_index;
    }
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_oper_status (struct nsm_master *nm,
                             u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc) )
    {
      if ((vc->vc_fib != NULL) && 
          (vc->vc_fib->install_flag))
        *ret = PW_L2_STATUS_UP;
      else
        *ret = PW_L2_STATUS_DOWN;
    } 
  else if (vc_temp)
    *ret = PW_L2_STATUS_DOWN;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if (vpls_peer->state == NSM_VPLS_PEER_UP)
        *ret = PW_L2_STATUS_UP;
      else
        *ret = PW_L2_STATUS_DOWN;
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_oper_status (struct nsm_master *nm,
                                  u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc) )
    {
      if ((vc->vc_fib != NULL) &&
          (vc->vc_fib->install_flag))
        *ret = PW_L2_STATUS_UP;
      else
        *ret = PW_L2_STATUS_DOWN;
      
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = PW_L2_STATUS_DOWN;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS 
  else if (vpls_peer)
    {
      if (vpls_peer->state == NSM_VPLS_PEER_UP)
        *ret = PW_L2_STATUS_UP;
      else
        *ret = PW_L2_STATUS_DOWN;
      
      *pw_ix = vpls_peer->vc_snmp_index; 
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_local_status (struct nsm_master *nm,
                              u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;
  u_int32_t retval = 0;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    pal_sprintf (*ret, "%d", vc->pw_status);
  else if (vc_temp)
    pal_sprintf (*ret, "%d", retval);
#ifdef HAVE_VPLS
  else if (vpls_peer)
    pal_sprintf (*ret, "%d", retval);
#endif /* HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;
  
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_local_status (struct nsm_master *nm,
                                   u_int32_t *pw_ix, char **ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  u_int32_t vc_id;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;
  u_int32_t retval = 0;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      pal_sprintf (*ret, "%d", vc->pw_status);
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      pal_sprintf (*ret, "%d", retval);
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      pal_sprintf (*ret, "%d", retval);
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /* HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_rmt_status_capab (struct nsm_master *nm,
                                  u_int32_t pw_ix, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  u_int32_t vc_id; 
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc_temp)
    {
      *ret = PW_SNMP_REMOTE_STATUS_NOT_APPL;
    }
  else if (vc)
    {
      if (vc->fec_type_vc == PW_OWNER_MANUAL)
        *ret = PW_SNMP_REMOTE_STATUS_NOT_APPL;
      else if (!vc->vc_fib)
        *ret = PW_SNMP_REMOTE_STATUS_NOT_YET_KNOWN;
      else
        {
          if (vc->vc_fib->rem_pw_status_cap)
            *ret = PW_SNMP_REMOTE_STATUS_CAPABLE;
          else
            *ret = PW_SNMP_REMOTE_STATUS_NOT_CAPABLE;
        }
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = PW_SNMP_REMOTE_STATUS_NOT_APPL;
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_rmt_status_capab (struct nsm_master *nm,
                                       u_int32_t *pw_ix, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  u_int32_t vc_id;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;
 
#ifdef HAVE_VPLS
   nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if (vc_temp)
    {
      *ret = PW_SNMP_REMOTE_STATUS_NOT_APPL;
      *pw_ix = vc_temp->vc_snmp_index;
    }
  else if (vc)
    {
      if (vc->fec_type_vc == PW_OWNER_MANUAL)
        *ret = PW_SNMP_REMOTE_STATUS_NOT_APPL;
      else if (!vc->vc_fib)
        *ret = PW_SNMP_REMOTE_STATUS_NOT_YET_KNOWN;
      else
        {
          if (vc->vc_fib->rem_pw_status_cap)
            *ret = PW_SNMP_REMOTE_STATUS_CAPABLE;
          else
            *ret = PW_SNMP_REMOTE_STATUS_NOT_CAPABLE;
        }
      *pw_ix = vc->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = PW_SNMP_REMOTE_STATUS_NOT_APPL;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_rmt_status (struct nsm_master *nm,
                            u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */
  
  if ((vc))
    {
      if (!vc->vc_fib || !vc->vc_fib->rem_pw_status_cap)
        pal_sprintf (*ret, "%d", NSM_CODE_PW_NOT_FORWARDING);
      else
        pal_sprintf (*ret, "%d", vc->remote_pw_status);
    }
  else if (vc_temp)
    pal_sprintf (*ret, "%d", NSM_CODE_PW_NOT_FORWARDING);
#ifdef HAVE_VPLS
  else if (vpls_peer)
    pal_sprintf (*ret, "%d", NSM_CODE_PW_NOT_FORWARDING);
#endif /*HAVE_VPLS */

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_rmt_status (struct nsm_master *nm,
                                 u_int32_t *pw_ix, char **ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  u_int32_t vc_id;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      if (!vc->vc_fib || !vc->vc_fib->rem_pw_status_cap)
        pal_sprintf (*ret, "%d", NSM_CODE_PW_NOT_FORWARDING);
      else
        pal_sprintf (*ret, "%d", vc->remote_pw_status);

      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      pal_sprintf (*ret, "%d", NSM_CODE_PW_NOT_FORWARDING);
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      pal_sprintf (*ret, "%d", NSM_CODE_PW_NOT_FORWARDING);
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

# define pal_timersub(a, b, result)                 \
  do {                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;           \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;            \
    if ((result)->tv_usec < 0) {                \
      --(result)->tv_sec;                 \
      (result)->tv_usec += 1000000;               \
    }                       \
  } while (0)

u_int32_t
vc_timer_get_secs_elpsd (struct thread * t)
{
  struct pal_timeval timer_now;
  struct pal_timeval diff;
  struct pal_timeval start_time;

  if (t)
    {
      pal_time_tzcurrent (&timer_now, NULL);
      start_time.tv_sec = t->u.sands.tv_sec - VC_TIMER_15MIN;
      start_time.tv_usec = t->u.sands.tv_usec;
      pal_timersub (&timer_now, &start_time, &diff);
      return diff.tv_sec;
    }
  return 0;
}

static inline int
vc_timer_get_tics_elpsd (struct thread * t)
{
  struct pal_timeval timer_now;
  struct pal_timeval diff;
  struct pal_timeval start_time;

  if (t)
    {
      int tics;
      pal_time_tzcurrent (&timer_now, NULL);
      start_time.tv_sec = t->u.sands.tv_sec - VC_TIMER_15MIN;
      start_time.tv_usec = t->u.sands.tv_usec;
      pal_timersub (&timer_now, &start_time, &diff);
      tics = SECS_TO_TICS(diff.tv_sec) + MICROSECS_TO_TICS(diff.tv_usec);
      if (tics < 0)
        tics = 0;
      return tics;
    }
  return 0;
}

static inline int
vc_is_timer_running (struct thread *t)
{
  return (t && (t->type != THREAD_UNUSED));
}

u_int32_t
nsm_mpls_get_pw_time_elapsed (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct pal_timeval time_elpsd;
  vc_id = pw_ix;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */
  
  pal_time_tzcurrent(&time_elpsd,NULL);

  if ((vc))
    {
      if ((vc->t_time_elpsd) && vc_is_timer_running(vc->t_time_elpsd))
        *ret = TICS_TO_SECS(vc_timer_get_tics_elpsd(vc->t_time_elpsd));
      else
        *ret = 0; 
    }
  else if (vc_temp)
    *ret = 0;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = 0; 
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_time_elapsed (struct nsm_master *nm,
                                   u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

   if ((vc))
    {
      if ((vc->t_time_elpsd) && vc_is_timer_running(vc->t_time_elpsd))
        *ret = TICS_TO_SECS(vc_timer_get_tics_elpsd(vc->t_time_elpsd));
       
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = 0;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = 0;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_valid_intrvl (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int64_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

   if ((vc))
    {
      if (vc->nsm_mpls_pw_snmp_perf_curr_list)
       *ret = LISTCOUNT (vc->nsm_mpls_pw_snmp_perf_curr_list); 
      else
        *ret = 0;
    }
  else if (vc_temp)
    memset(ret, 0, sizeof (ut_int64_t));
#ifdef HAVE_VPLS
  else if (vpls_peer)
    memset(ret, 0, sizeof (ut_int64_t)); 
#endif /*HAVE_VPLS */
   else
     return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_valid_intrvl (struct nsm_master *nm,
                                   u_int32_t *pw_ix, u_int64_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

   if ((vc))
    {
      if (vc->nsm_mpls_pw_snmp_perf_curr_list)
        *ret = LISTCOUNT (vc->nsm_mpls_pw_snmp_perf_curr_list);
      else
        *ret = 0;

      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      memset(ret, 0, sizeof (ut_int64_t));  
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      memset(ret, 0, sizeof (ut_int64_t));
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_row_status (struct nsm_master *nm,
                            u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    *ret = vc->row_status;
  else if (vc_temp)
    *ret = vc_temp->row_status;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = SNMP_AG_ROW_active;
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_row_status (struct nsm_master *nm,
                                 u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      *ret = vc->row_status;
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = vc_temp->row_status;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *pw_ix = vpls_peer->vc_snmp_index;
      *ret = SNMP_AG_ROW_active;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_st_type (struct nsm_master *nm,
                         u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */


  if ((vc))
    *ret = ST_VOLATILE;
  else if (vc_temp)
    *ret = ST_VOLATILE;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = ST_VOLATILE;
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_st_type (struct nsm_master *nm,
                              u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
   nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */
    
  if ((vc))
    {
       *ret = ST_VOLATILE;
       *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = ST_VOLATILE;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
       *ret = ST_VOLATILE;
       *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_oam_en (struct nsm_master *nm,
                        u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

 /*TODO: Call to find VCCV enable to be placed here.
  * For now returning TRUE (1)value */     
  if ((vc))
    *ret = NSM_MPLS_PW_PAL_TRUE; 
  else if (vc_temp)
    *ret = NSM_MPLS_PW_PAL_TRUE;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    *ret = NSM_MPLS_PW_PAL_TRUE;
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_oam_en (struct nsm_master *nm,
                             u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      *ret = NSM_MPLS_PW_PAL_TRUE;
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      *ret = NSM_MPLS_PW_PAL_TRUE;
      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      *ret = NSM_MPLS_PW_PAL_TRUE;
      *pw_ix = vpls_peer->vc_snmp_index;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_gen_agi_type (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      if (vc->fec_type_vc == PW_OWNER_GEN_FEC_SIGNALING && vc->vc_info) 
        *ret = vc->vc_info->vc_type;
      else
        *ret = 0;
    }
  else if (vc_temp)
    {
      if (vc_temp->pw_owner == PW_OWNER_GEN_FEC_SIGNALING)
        *ret = vc_temp->vc_type;
      else
        *ret =0;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if (vpls_peer->fec_type_vc == PW_OWNER_GEN_FEC_SIGNALING)
        {
           vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
           if (vpls)
               *ret = vpls->vpls_type;
           else
               return NSM_API_GET_ERROR;
        }
      else
        *ret = 0;
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;
 
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_gen_agi_type (struct nsm_master *nm,
                                   u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
  struct nsm_vpls *vpls = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

   if ((vc))
    {
      if (vc->vc_info && vc->fec_type_vc == PW_OWNER_GEN_FEC_SIGNALING )
        *ret = vc->vc_info->vc_type;
      else
        *ret = 0;
      
      *pw_ix = vc->vc_snmp_index;
    }
  else if (vc_temp)
    {
      if (vc_temp->pw_owner == PW_OWNER_GEN_FEC_SIGNALING)
        *ret = vc_temp->vc_type;
      else
        *ret =0;

      *pw_ix = vc_temp->vc_snmp_index;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if (vpls_peer->fec_type_vc == PW_OWNER_GEN_FEC_SIGNALING)
        {
          vpls = nsm_vpls_lookup (nm, vpls_peer->vpls_id, NSM_FALSE);
          if (vpls)
            *ret = vpls->vpls_type;
          else
            return NSM_API_GET_ERROR;
          
          *pw_ix = vpls_peer->vc_snmp_index;
         }
       else
         *ret = 0;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_gen_loc_aii_type (struct nsm_master *nm,
                                  u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      if (vc->fec_type_vc == PW_OWNER_GEN_FEC_SIGNALING)
        *ret = 0;
      else
        return NSM_API_GET_ERROR;
    }
  else if (vc_temp)
    {
      if (vc_temp->pw_owner == PW_OWNER_GEN_FEC_SIGNALING)
        *ret = 0;
      else
        return NSM_API_GET_ERROR;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if (vpls_peer->fec_type_vc == PW_OWNER_GEN_FEC_SIGNALING)
        *ret = 0;
      else
        return NSM_API_GET_ERROR;
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_gen_loc_aii_type (struct nsm_master *nm,
                                       u_int32_t *pw_ix,
                                       u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      if (vc->fec_type_vc == PW_OWNER_GEN_FEC_SIGNALING)
        {
          *ret = 0;
          *pw_ix = vc->vc_snmp_index;
        }
     else
       return NSM_API_GET_ERROR;     
    }
  else if (vc_temp)
    {
      if (vc_temp->pw_owner ==  PW_OWNER_GEN_FEC_SIGNALING)
        {
          *ret = 0;
          *pw_ix = vc_temp->vc_snmp_index;
        }
      else 
        return NSM_API_GET_ERROR;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if (vpls_peer->fec_type_vc == PW_OWNER_GEN_FEC_SIGNALING)
        {
          *pw_ix = vpls_peer->vc_snmp_index;
          *ret = 0;
        }
      else
        return NSM_API_GET_ERROR;
    }
#endif /*HAVE_VPLS */
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_gen_rem_aii_type (struct nsm_master *nm,
                                  u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      if (vc->fec_type_vc == PW_OWNER_GEN_FEC_SIGNALING)
        *ret = 0;
    }
  else if (vc_temp)
    *ret = 0;
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if (vpls_peer->fec_type_vc == PW_OWNER_GEN_FEC_SIGNALING)
        *ret = 0;
      else
        return NSM_API_GET_ERROR;
    }
#endif /*HAVE_VPLS */
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_gen_rem_aii_type (struct nsm_master *nm,
                                       u_int32_t *pw_ix, 
                                       u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
#ifdef HAVE_VPLS
  struct nsm_vpls_peer *vpls_peer = NULL;
#endif /* HAVE_VPLS*/
  vc_id = *pw_ix;

#ifdef HAVE_VPLS
    nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, &vpls_peer);
#else
      nsm_mpls_vc_snmp_lookup_next (nm, vc_id, &vc, &vc_temp, NULL);
#endif /*HAVE_VPLS */

  if ((vc))
    {
      if (vc->fec_type_vc == PW_OWNER_GEN_FEC_SIGNALING)
        {
          *ret = 0;
          *pw_ix = vc->vc_snmp_index;
        }
      else
        return NSM_API_GET_ERROR;
    }
  else if (vc_temp)
    {
      if (vc_temp->pw_owner ==  PW_OWNER_GEN_FEC_SIGNALING)
        {
          *ret = 0;
          *pw_ix = vc_temp->vc_snmp_index;
        }
      else
        return NSM_API_GET_ERROR;
    }
#ifdef HAVE_VPLS
  else if (vpls_peer)
    {
      if (vpls_peer->fec_type_vc == PW_OWNER_GEN_FEC_SIGNALING)
        {
          *pw_ix = vpls_peer->vc_snmp_index;
          *ret = 0;
        }
      else
        return NSM_API_GET_ERROR;
    }
#endif /* HAVE_VPLS*/
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

/* APIs related to VC Table set */
u_int32_t
nsm_mpls_set_pw_type (struct nsm_master *nm,
                      u_int32_t pw_ix, u_int32_t pw_type)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  /* Check for validity of value */
  if (!MPLS_VC_IS_VALID(pw_type))
    return NSM_API_SET_ERROR;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if ((vc) && (vc->vc_info))
    vc->vc_info->vc_type = pw_type;

  else if (vc_temp)
    vc_temp->vc_type = pw_type;

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_owner (struct nsm_master *nm,
                       u_int32_t pw_ix, u_int32_t pw_owner)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  if (!((pw_owner == PW_OWNER_GEN_FEC_SIGNALING) ||
        (pw_owner == PW_OWNER_PWID_FEC_SIGNALING) ||
        (pw_owner == PW_OWNER_MANUAL)))
    return NSM_API_SET_ERROR;

#ifndef HAVE_FEC129
   if (pw_owner == PW_OWNER_GEN_FEC_SIGNALING)
     return NSM_API_SET_ERROR;
#endif

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if (vc)
    vc->fec_type_vc = pw_owner;

  else if (vc_temp)
    vc_temp->pw_owner = pw_owner;

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_psn_type (struct nsm_master *nm,
                          u_int32_t pw_ix, u_int32_t psn_type)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  if (psn_type != PW_PSN_TYPE_MPLS)
    return NSM_API_SET_ERROR;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if (vc || vc_temp) /* If entry present */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_pw_setup_prty (struct nsm_master *nm,
                            u_int32_t pw_ix, u_int32_t setup_prty)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if (setup_prty != NSM_MPLS_PW_DEF_SETUP_PRIO)
    return NSM_API_SET_ERROR;

  if (vc)
    vc->setup_prty = NSM_MPLS_PW_DEF_SETUP_PRIO;

  else if (vc_temp)
    vc_temp->setup_prty = NSM_MPLS_PW_DEF_SETUP_PRIO;

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_id (struct nsm_master *nm, u_int32_t pw_ix, u_int32_t pw_id)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if (vc && vc->row_status == RS_ACTIVE)
    return NSM_API_SET_ERROR;
  else if (vc_temp)
    {
      vc_temp->vc_id = pw_id;
      SET_FLAG (vc_temp->flags, VC_ENTRY_FLAG_ID_SET);

      if (CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_PEER_ADDR_TYPE_SET) &&
          CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_PEER_ADDR_SET) &&
          CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_NAME_SET) &&
          CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_ID_SET))
        {
          /* All mandatory objects present */
          /* Change rowStatus to Not in Service */
          vc_temp->row_status = RS_NOT_IN_SERVICE;
        }
    }
  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_hold_prty (struct nsm_master *nm,
                           u_int32_t pw_ix, u_int32_t hold_prty)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if (vc && vc->row_status == RS_ACTIVE)
    return NSM_API_SET_ERROR;
  
  if (hold_prty != NSM_MPLS_PW_DEF_HOLD_PRIO)
    return NSM_API_SET_ERROR;

  if (vc)
    vc->hold_prty = NSM_MPLS_PW_DEF_HOLD_PRIO;

  else if (vc_temp)
    vc_temp->hold_prty = NSM_MPLS_PW_DEF_HOLD_PRIO;

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_peer_addr_type (struct nsm_master *nm,
                                u_int32_t pw_ix, u_int32_t peer_addr_type)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  if (!(peer_addr_type == INET_AD_IPV4))
    return NSM_API_SET_ERROR;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if (vc_temp && !vc)
    {
      vc_temp->peer_addr_type = peer_addr_type;
      SET_FLAG (vc_temp->flags, VC_ENTRY_FLAG_PEER_ADDR_TYPE_SET);

      if (CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_PEER_ADDR_TYPE_SET) &&
          CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_PEER_ADDR_SET) &&
          CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_NAME_SET) &&
          CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_ID_SET))
        {
          /* All mandatory objects present */
          /* Change rowStatus to Not in Service */
          vc_temp->row_status = RS_NOT_IN_SERVICE;
        }
    }

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_peer_addr (struct nsm_master *nm, u_int32_t pw_ix,
                           struct pal_in4_addr *peer_addr)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if (vc_temp && !vc)
    {
      pal_mem_cpy(&vc_temp->peer_addr, peer_addr,
                  sizeof (struct pal_in4_addr));
      SET_FLAG (vc_temp->flags, VC_ENTRY_FLAG_PEER_ADDR_SET);

      if (CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_PEER_ADDR_TYPE_SET) &&
          CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_PEER_ADDR_SET) &&
          CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_NAME_SET) &&
          CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_ID_SET))
        {
          /* All mandatory objects present */
          /* Change rowStatus to Not in Service */
          vc_temp->row_status = RS_NOT_IN_SERVICE;
        }
    }

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_attchd_pw_ix (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int32_t attchd_ix)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
  
  if (attchd_ix != 0)
    return NSM_API_SET_ERROR;

  if ((vc) && (vc->vc_info))
    vc->vc_info->pw_attchd_pw_ix = 0;

  else if (vc_temp)
    vc_temp->pw_attchd_pw_ix = 0;

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_if_ix (struct nsm_master *nm,
                       u_int32_t pw_ix, u_int32_t if_ix)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if (if_ix != 0)
    return NSM_API_SET_ERROR;

  if (vc || vc_temp) /* If entry present */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_pw_local_grp_id (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int32_t local_grp_id)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if (vc && (vc->row_status == RS_ACTIVE))
    return NSM_API_SET_ERROR;
  else if (vc_temp)
    {
      /* If previously configured with a group with different grp_id, free it */
      if (vc_temp->grp_name && (vc_temp->local_grp_id != local_grp_id))
        {
          XFREE (MTYPE_TMP, vc_temp->grp_name);
          vc_temp->grp_name = NULL;
        }
      vc_temp->local_grp_id = local_grp_id;
    }
  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_grp_attchmt_id (struct nsm_master *nm,
                                u_int32_t pw_ix, u_int32_t grp_id)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
 
  if (vc && vc->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING) 
    return NSM_API_SET_ERROR;

  if (grp_id != vc_id)
    return NSM_API_SET_ERROR;

  if (vc)
     vc->grp_attchmt_id = vc_id;

  else if (vc_temp)
    vc_temp->grp_attchmt_id = vc_id;

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_local_attchmt_id (struct nsm_master *nm,
                                  u_int32_t pw_ix, u_int32_t local_id)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if (vc && vc->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING)
    return NSM_API_SET_ERROR;
  
  if (local_id != 0)
    return NSM_API_SET_ERROR;

  if (vc)
    vc->local_attchmt_id = 0;

  else if (vc_temp)
    vc_temp->local_attchmt_id = 0;

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_peer_attchmt_id (struct nsm_master *nm,
                                 u_int32_t pw_ix, u_int32_t peer_id)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
  
  if (vc && vc->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING)
    return NSM_API_SET_ERROR;
  
  if (peer_id != 0)
    return NSM_API_SET_ERROR;

  if (vc)
    vc->peer_attchmt_id = 0;

  else if (vc_temp)
    vc_temp->peer_attchmt_id = 0;

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_cw_prfrnce (struct nsm_master *nm,
                            u_int32_t pw_ix, u_int32_t cw)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  if (!((cw == NSM_MPLS_PW_PAL_TRUE) || (cw == NSM_MPLS_PW_PAL_FALSE)))
    return NSM_API_SET_ERROR;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
  
  if (vc && vc->row_status == RS_ACTIVE)
    return NSM_API_SET_ERROR;

  if (vc)
    vc->cw = cw;

  else if (vc_temp)
    vc_temp->cw = cw;

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_local_if_mtu (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int32_t mtu)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  if (mtu > 65535)
    return NSM_API_SET_ERROR;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if (vc && vc->row_status == RS_ACTIVE)
    return NSM_API_SET_ERROR;

  if (vc)
    vc->vc_info->mif->ifp->mtu = mtu;

  else if (vc_temp)
    vc_temp->mtu = mtu;

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_local_if_string (struct nsm_master *nm,
                                 u_int32_t pw_ix, u_int32_t if_string)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if (vc && vc->row_status == RS_ACTIVE)
    return NSM_API_SET_ERROR;
  
  if (if_string != NSM_MPLS_PW_PAL_FALSE)
    return NSM_API_SET_ERROR;

  if (vc || vc_temp)
    return NSM_API_SET_SUCCESS;

  else
    return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_pw_local_capab_advrt (struct nsm_master *nm,
                                   u_int32_t pw_ix, char *capab)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
 
  if (vc && vc->row_status == RS_ACTIVE)
    return NSM_API_SET_ERROR;
 
  if (pal_strcmp (capab, "0") != 0)
    return NSM_API_SET_ERROR;

  if (vc || vc_temp)
    return NSM_API_SET_SUCCESS;

  else
    return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_pw_frgmt_cfg_size (struct nsm_master *nm,
                                u_int32_t pw_ix, u_int32_t cfg_sz)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
  
  if (vc && vc->row_status == RS_ACTIVE)
    return NSM_API_SET_ERROR;

  if (cfg_sz != 0)
    return NSM_API_SET_ERROR;

  if (vc || vc_temp)
    return NSM_API_SET_SUCCESS;

  else
    return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_pw_fcs_retentn_cfg (struct nsm_master *nm,
                                 u_int32_t pw_ix, u_int32_t retn_cfg)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if (vc && vc->row_status == RS_ACTIVE)
    return NSM_API_SET_ERROR;

  if (retn_cfg != 1)
    return NSM_API_SET_ERROR;

  if (vc || vc_temp)
    return NSM_API_SET_SUCCESS;

  else
    return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_pw_outbd_label (struct nsm_master *nm,
                             u_int32_t pw_ix, u_int32_t out_label)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  if (! VALID_LABEL (out_label))
    return NSM_API_SET_ERROR;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if(vc && (vc->row_status == RS_ACTIVE || vc->fec_type_vc != PW_OWNER_MANUAL))
    return NSM_API_SET_ERROR;

  else if (vc_temp && vc_temp->vc_fib_temp)
    vc_temp->vc_fib_temp->out_label = out_label;

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_inbd_label (struct nsm_master *nm,
                            u_int32_t pw_ix, u_int32_t in_label)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  if (! VALID_LABEL (in_label))
    return NSM_API_SET_ERROR;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if(vc && (vc->row_status == RS_ACTIVE || vc->fec_type_vc != PW_OWNER_MANUAL))
    return NSM_API_SET_ERROR;

  else if (vc_temp && vc_temp->vc_fib_temp)
    vc_temp->vc_fib_temp->in_label = in_label;

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_name (struct nsm_master *nm,
                      u_int32_t pw_ix, char *name)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit *vc2 = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_circuit_temp *vc_temp2 = NULL;
  vc_id = pw_ix;

  if (name == NULL)
    return NSM_API_SET_ERROR;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
  
  if (vc)
    {
      vc2 = nsm_mpls_l2_circuit_lookup_by_name (nm, name);
      if (vc2)
        {
          if (vc != vc2)
            return NSM_API_SET_ERROR;
        }
      if (vc->name)
        XFREE (MTYPE_TMP, vc->name);

      vc->name = XSTRDUP (MTYPE_TMP, name);

      if (vc->vc_info)
        {
          if (vc->vc_info->vc_name)
            XFREE (MTYPE_TMP, vc->vc_info->vc_name);

          vc->vc_info->vc_name = XSTRDUP (MTYPE_TMP, name);
        }
    }
  else if (vc_temp)
    {
      vc2 = nsm_mpls_l2_circuit_lookup_by_name (nm, name);
      if (vc2)
        return NSM_API_SET_ERROR;

      vc_temp2 = nsm_mpls_l2_circuit_temp_lookup_by_name (name, IDX_LOOKUP);
      if (vc_temp2)
        {
          if (vc_temp != vc_temp2)
            return NSM_API_SET_ERROR;
        }
      if (vc_temp->vc_name)
        XFREE (MTYPE_TMP, vc_temp->vc_name);

      vc_temp->vc_name = XSTRDUP (MTYPE_TMP, name);
      SET_FLAG (vc_temp->flags, VC_ENTRY_FLAG_NAME_SET);

      if (CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_PEER_ADDR_TYPE_SET) &&
          CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_PEER_ADDR_SET) &&
          CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_NAME_SET) &&
          CHECK_FLAG (vc_temp->flags, VC_ENTRY_FLAG_ID_SET))
        {
          /* All mandatory objects present */
          /* Change rowStatus to Not in Service */
          vc_temp->row_status = RS_NOT_IN_SERVICE;
        }
    }

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_descr (struct nsm_master *nm,
                       u_int32_t pw_ix, char *name)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if (vc)
    vc->dscr = XSTRDUP (MTYPE_TMP, name);
  else if (vc_temp)
    vc_temp->dscr = XSTRDUP (MTYPE_TMP, name);
  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_admin_status (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int32_t admin_status)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct fec_gen_entry gen_entry;
  int ret;

  if ((admin_status != PW_SNMP_ADMIN_UP) && 
      (admin_status != PW_SNMP_ADMIN_DOWN))
    return NSM_API_SET_ERROR;

  if (pw_ix == 0)
    return NSM_API_SET_ERROR;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, &vc_temp, NULL);

  if (vc)
    {
      NSM_MPLS_PREFIX_TO_FEC_GEN_ENTRY(&gen_entry, &vc->address);

      if ((vc->vc_info) && (vc->vc_fib) && (vc->vc_fib->install_flag))
        {
          if (admin_status == PW_SNMP_ADMIN_DOWN)
            {
              if (vc->state >= NSM_MPLS_L2_CIRCUIT_COMPLETE &&
                  FLAG_ISSET (vc->flags, NSM_MPLS_VC_FLAG_DEP))
                {
                  gmpls_lsp_dep_del (nm, &gen_entry, CONFIRM_VC, vc);
                  UNSET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_DEP);
                }
            }
        }
      else 
        {
          if (admin_status == PW_SNMP_ADMIN_UP)
            {
              if (vc->state == NSM_MPLS_L2_CIRCUIT_COMPLETE &&
                  FLAG_ISSET (vc->flags, NSM_MPLS_VC_FLAG_SELECTED))
                {
                  gmpls_lsp_dep_add (nm, &gen_entry, CONFIRM_VC, vc);
                  SET_FLAG (vc->flags, NSM_MPLS_VC_FLAG_DEP);
                }
              else if (vc->state == NSM_MPLS_L2_CIRCUIT_ACTIVE && 
                  vc->vc_info && vc->vc_info->mif)
                {
                  ret = nsm_mpls_if_install_vc_data (vc->vc_info->mif, 
                      vc, vc->vc_info->vlan_id);
                  if (ret < 0)
                    {
                      if (vc->vc_info && vc->vc_info->mif &&
                          (CHECK_FLAG (vc->vc_info->if_bind_type, 
                                       NSM_IF_BIND_MPLS_VC) ||
                           CHECK_FLAG (vc->vc_info->if_bind_type, 
                             NSM_IF_BIND_MPLS_VC_VLAN)))
                        {
                          nsm_mpls_vc_info_del (vc->vc_info->mif, 
                              vc->vc_info->mif->vc_info_list, 
                              vc->vc_info);
                        }

                      if (vc->vc_info)
                        {
                          nsm_mpls_vc_info_free (vc->vc_info);
                          vc->vc_info = NULL;
                        }
                    }
                }
            }
        }
      vc->admin_status = admin_status;
    }
  else if (vc_temp)
    {
      vc_temp->admin_status = admin_status;
    }
  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_row_status (struct nsm_master *nm,
                            u_int32_t pw_ix, u_int32_t row_status)
{
  int res = -1;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  int ret = NSM_FAILURE;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, &vc_temp, NULL);

  switch (row_status)
    {
      case RS_ACTIVE:
        if (vc)  /* Already Active */
          return NSM_API_SET_SUCCESS;
        if (vc_temp && vc_temp->row_status == RS_NOT_IN_SERVICE)
          {
            ret = nsm_mpls_create_new_nsm_mpls_circuit (nm, &vc_temp, &vc);
            if (ret != NSM_SUCCESS)  /* Creation of new entry fails */
              return NSM_API_SET_ERROR;

            vc->last_change = vc->create_time;
            listnode_delete (vc_entry_temp_list, (void *)vc_temp);
            if (vc_temp->grp_name)
              XFREE (MTYPE_TMP, vc_temp->grp_name);
            XFREE (MTYPE_NSM_VIRTUAL_CIRCUIT, vc_temp);

            return NSM_API_SET_SUCCESS;
          }
        return NSM_API_SET_ERROR;
        break;

      case RS_NOT_IN_SERVICE:
        /* Already Not in Service */
        if (vc_temp && vc_temp->row_status == RS_NOT_IN_SERVICE)
          return NSM_API_SET_SUCCESS;
        if (vc)  /* Current state - Active */
          {
            ret = nsm_mpls_create_new_vc_temp(nm, vc, pw_ix);
            if (ret != NSM_SUCCESS)  /* Creation of new temp entry fails */
              return NSM_API_SET_ERROR;

            VC_TIMER_OFF(vc->t_time_elpsd);
            memset(&vc->valid_intrvl_cnt,  0, sizeof(u_int32_t));
            /* Free memory */ 
            res = nsm_mpls_l2_circuit_del (nm, vc, NSM_TRUE);
            if (res < 0)
              return NSM_API_SET_ERROR;

            /* Index will be released during deletion above, 
             * reserve it again for temp */
            ret = bitmap_reserve_index (NSM_MPLS->vc_indx_mgr, pw_ix);
            if (ret < 0)
              return NSM_API_SET_ERROR;

            return NSM_API_SET_SUCCESS;
          }
        return NSM_API_SET_ERROR;
        break;

      case RS_NOT_READY:
        return NSM_API_SET_ERROR;
        break;

      case RS_CREATE_GO:
        return NSM_API_SET_ERROR;
        break;

      case RS_CREATE_WAIT:
        if (vc || vc_temp)  /* Entry already present. Cant recreate */
          return NSM_API_SET_ERROR;

        /* Reserve the pw ix. If not available return error */
        ret = bitmap_reserve_index (NSM_MPLS->vc_indx_mgr, pw_ix);
        if (ret < 0)
          return NSM_API_SET_ERROR;

        ret = nsm_mpls_create_new_vc_temp(nm, vc, pw_ix);
        if (ret != NSM_SUCCESS)  /* Creation of new temp entry fails */
          return NSM_API_SET_ERROR;

        return NSM_API_SET_SUCCESS;
        break;

      case RS_DESTROY:
        if (vc)
          {
            /* Free memory */ 
            res = nsm_mpls_l2_circuit_del (nm, vc, NSM_TRUE);
            if (res < 0)
              return NSM_API_SET_ERROR;

          }
        else if (vc_temp)
          {
            ret = bitmap_release_index (NSM_MPLS->vc_indx_mgr, pw_ix);
            if (ret < 0)
              return NSM_FAILURE;

            /* Delete node from temp. list and free memory */
            listnode_delete (vc_entry_temp_list, (void *)vc_temp);
            XFREE (MTYPE_NSM_VIRTUAL_CIRCUIT, vc_temp);
          }
        else
          return NSM_API_SET_ERROR;

        return NSM_API_SET_SUCCESS;
        break;

      default:
        return NSM_API_SET_ERROR;
        break;
    }
}

u_int32_t
nsm_mpls_set_pw_st_type (struct nsm_master *nm,
                         u_int32_t pw_ix, u_int32_t st_type)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
  
  if (vc_id > 0 && ! vc && ! vc_temp)
    return NSM_API_SET_ERROR;

  if (st_type != ST_VOLATILE)
    return NSM_API_SET_ERROR;

  if (vc || vc_temp)
    return NSM_API_SET_SUCCESS;

  else
    return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_pw_oam_enable (struct nsm_master *nm,
                            u_int32_t pw_ix, u_int32_t oam_en)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;
 
  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
  /*TODO: For now allowing only TRUE to be set. 
          VCCV call to be placed here */
  if (oam_en != PAL_TRUE) 
    return NSM_API_SET_ERROR;
 
  if (vc || vc_temp)
    return NSM_API_SET_SUCCESS;

  else
    return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_pw_gen_agi_type (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int32_t pw_gen_agi_type)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  /* Check for validity of value */
  if (!MPLS_VC_IS_VALID(pw_gen_agi_type))
    return NSM_API_SET_ERROR;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
  
  if (vc_id > 0 && ! vc && ! vc_temp)
    return NSM_API_SET_ERROR;

  if ((vc) && (vc->vc_info))
    vc->vc_info->vc_type = pw_gen_agi_type;

  else if (vc_temp)
    vc_temp->vc_type = pw_gen_agi_type;

  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_gen_loc_aii_type (struct nsm_master *nm,
                                   u_int32_t pw_ix, u_int32_t gen_loc_aii_type)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);
  
  if (vc_id > 0 && ! vc && ! vc_temp)
    return NSM_API_SET_ERROR;

  /*Filling the SAII and TAII fields with empty values as
    they are not currently used */
  if (gen_loc_aii_type != 0)
    return NSM_API_SET_ERROR;

  if (vc || vc_temp)
    return NSM_API_SET_SUCCESS;

  else
    return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_set_pw_gen_rem_aii_type (struct nsm_master *nm,
                                   u_int32_t pw_ix, u_int32_t gen_rem_aii_type)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_ix;

  nsm_mpls_vc_snmp_lookup (nm, vc_id, &vc, &vc_temp, NULL);

  if (vc_id > 0 && ! vc && ! vc_temp)
    return NSM_API_SET_ERROR;

  if (gen_rem_aii_type != 0)
    return NSM_API_SET_ERROR;

  if (vc || vc_temp)
    return NSM_API_SET_SUCCESS;

  else
    return NSM_API_SET_ERROR;
}

/* PW Performance Current Table */

u_int64_t
nsm_mpls_get_pw_crnt_in_hc_pckts (struct nsm_master *nm,
                                  u_int32_t pw_ix, u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, &vc_temp, NULL);

  if ((vc))
    {
      if ((vc->nsm_mpls_pw_snmp_perf_curr_list))
       {
         perf_curr = GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list)); 
         if (perf_curr)
           *ret = perf_curr->pw_crnt_in_pckts;
         else
           *ret = 0; 
       }
      else
       *ret = 0;
    } 
  else if (vc_temp)
     memset(ret, 0, sizeof (u_int64_t));

  return NSM_API_GET_SUCCESS;
}

u_int64_t
nsm_mpls_get_next_pw_crnt_in_hc_pckts (struct nsm_master *nm, u_int32_t *pw_ix, 
                                       u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup_next (nm, *pw_ix, &vc, &vc_temp, NULL);

  if (vc && vc->nsm_mpls_pw_snmp_perf_curr_list)
   {
     perf_curr =  GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list));
     if (perf_curr)
       {
         *ret = perf_curr->pw_crnt_in_pckts;
         *pw_ix = vc->vc_snmp_index;
       }
     else
       { 
         *ret = 0;
         *pw_ix = vc->vc_snmp_index;
       }   
   }
  else if (vc_temp)
    {
      memset(ret, 0, sizeof (u_int64_t));
      *pw_ix = vc_temp->vc_snmp_index;
    }
  else
   {
     *pw_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}


u_int64_t
nsm_mpls_get_pw_crnt_in_hc_bytes (struct nsm_master *nm,
                                  u_int32_t pw_ix, u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, &vc_temp, NULL);

  if ((vc) && (vc->nsm_mpls_pw_snmp_perf_curr_list))
    {
      perf_curr = GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list));
      if (perf_curr)        
        *ret = perf_curr->pw_crnt_in_bytes;
      else
        *ret = 0; 
    }
  else if (vc_temp)
     memset(ret, 0, sizeof (u_int64_t));
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;

}

u_int64_t
nsm_mpls_get_next_pw_crnt_in_hc_bytes (struct nsm_master *nm, u_int32_t *pw_ix, 
                                       u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
 
  nsm_mpls_vc_snmp_lookup_next (nm, *pw_ix, &vc, &vc_temp, NULL);

  if ((vc) && vc->nsm_mpls_pw_snmp_perf_curr_list)
   { 
     perf_curr = GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list));
     if (perf_curr)
       {
         *ret = perf_curr->pw_crnt_in_bytes;
         *pw_ix = vc->vc_snmp_index;
       }
     else
       {
         *ret = 0;
         *pw_ix = vc->vc_snmp_index;
       }
   }
  else if (vc_temp)
   {
     memset(ret, 0, sizeof (u_int64_t));
     *pw_ix = vc_temp->vc_snmp_index;
   }

  else
   {
     *pw_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}

u_int64_t
nsm_mpls_get_pw_crnt_out_hc_pckts (struct nsm_master *nm,
                                   u_int32_t pw_ix, u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, &vc_temp, NULL);

  if ((vc) && (vc->nsm_mpls_pw_snmp_perf_curr_list))
    {
      perf_curr = GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list));
      if (perf_curr)
        *ret = perf_curr->pw_crnt_out_pckts;
      else
        *ret = 0;
    }
  else if (vc_temp)
    memset(ret, 0, sizeof (u_int64_t));
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;

}

u_int64_t
nsm_mpls_get_next_pw_crnt_out_hc_pckts (struct nsm_master *nm, u_int32_t *pw_ix,
                                        u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup_next (nm, *pw_ix, &vc, &vc_temp, NULL); 

  if ((vc) )
   {
     if (vc->nsm_mpls_pw_snmp_perf_curr_list)
       {
         perf_curr = GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list));
         if (perf_curr)
           {
             *ret = perf_curr->pw_crnt_out_pckts;
             *pw_ix = vc->vc_snmp_index;
           }
         else
           {
             *ret = 0;
             *pw_ix = vc->vc_snmp_index; 
           }    
       }
     else
       {
         *ret = 0;
         *pw_ix = vc->vc_snmp_index;
       }
   }
  else if (vc_temp)
   {
     memset(ret, 0, sizeof (u_int64_t));
     *pw_ix = vc_temp->vc_snmp_index;
   }
  else
   {
     *pw_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}


u_int64_t
nsm_mpls_get_pw_crnt_out_hc_bytes (struct nsm_master *nm,
                                   u_int32_t pw_ix, u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, &vc_temp, NULL);

  if ((vc) && (vc->nsm_mpls_pw_snmp_perf_curr_list))
   {
     perf_curr = GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list));
     if (perf_curr)
       *ret = perf_curr->pw_crnt_out_bytes;
     else
       *ret = 0;
   }
  else if (vc_temp)
    memset(ret, 0, sizeof (u_int64_t));
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;

}

u_int64_t
nsm_mpls_get_next_pw_crnt_out_hc_bytes (struct nsm_master *nm, u_int32_t *pw_ix,                              
                                        u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup_next (nm, *pw_ix, &vc, &vc_temp, NULL);

  if ((vc) )
   {
     if (vc->nsm_mpls_pw_snmp_perf_curr_list)
       {
         perf_curr = GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list));
         if (perf_curr)
           {
             *ret = perf_curr->pw_crnt_out_bytes;
             *pw_ix = vc->vc_snmp_index;
           }
         else
           {
             *ret = 0;
             *pw_ix = vc->vc_snmp_index;      
           }         
       }
     else
       {
         *ret = 0;
         *pw_ix = vc->vc_snmp_index;
       }
   }
  else if (vc_temp)
   {
     memset(ret, 0, sizeof (u_int64_t));
     *pw_ix = vc_temp->vc_snmp_index;
   }
  else
   {
     *pw_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_crnt_in_pckts (struct nsm_master *nm,
                               u_int32_t pw_ix, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, &vc_temp, NULL);

  if ((vc) && (vc->nsm_mpls_pw_snmp_perf_curr_list))
    {
      perf_curr = GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list));
      if (perf_curr)
        *ret = perf_curr->pw_crnt_in_pckts;
      else
        *ret = 0;
    }
  else if (vc_temp)
    memset(ret, 0, sizeof (u_int64_t));
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;

}

u_int32_t
nsm_mpls_get_next_pw_crnt_in_pckts (struct nsm_master *nm, u_int32_t *pw_ix, 
                                    u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup_next (nm, *pw_ix, &vc, &vc_temp, NULL);

  if ((vc) )
   {
     if (vc->nsm_mpls_pw_snmp_perf_curr_list)
       {
         perf_curr = GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list));
         if (perf_curr)
           {
             *ret = perf_curr->pw_crnt_in_pckts;
             *pw_ix = vc->vc_snmp_index;
           }
         else
           {
             *ret  = 0;
             *pw_ix = vc->vc_snmp_index;
           }       
       }
     else
       {
         *ret = 0;
         *pw_ix = vc->vc_snmp_index;
       }
   }
  else if (vc_temp)
   {
     memset(ret, 0, sizeof (u_int64_t));
     *pw_ix = vc_temp->vc_snmp_index;
   }
  else
   {
     *pw_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_crnt_in_bytes (struct nsm_master *nm,
                               u_int32_t pw_ix, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, &vc_temp, NULL);

  if ((vc) && (vc->nsm_mpls_pw_snmp_perf_curr_list))
    {
      perf_curr = GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list));
      if (perf_curr)
        *ret = perf_curr->pw_crnt_in_bytes;
      else
        *ret = 0;
    }
  else if (vc_temp)
    memset(ret, 0, sizeof (u_int64_t));
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;

}

u_int32_t
nsm_mpls_get_next_pw_crnt_in_bytes (struct nsm_master *nm, u_int32_t *pw_ix, 
                                    u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup_next (nm, *pw_ix, &vc, &vc_temp, NULL);

  if ((vc) )
   {
     if (vc->nsm_mpls_pw_snmp_perf_curr_list)
       {
         perf_curr = GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list));
         if (perf_curr)
           {
             *ret = perf_curr->pw_crnt_in_bytes;
             *pw_ix = vc->vc_snmp_index;
           }
         else
           {
             *ret = 0;
             *pw_ix = vc->vc_snmp_index;
           }  
       }
     else
       {
         *ret = 0;
         *pw_ix = vc->vc_snmp_index;
       }
   }
  else if (vc_temp)
   {
     memset(ret, 0, sizeof (u_int64_t));
     *pw_ix = vc_temp->vc_snmp_index;
   }
  else
   {
     *pw_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_crnt_out_pckts (struct nsm_master *nm,
                                u_int32_t pw_ix, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, &vc_temp, NULL);

  if ((pw_ix) && (vc) && (vc->nsm_mpls_pw_snmp_perf_curr_list))
    {
      perf_curr = GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list));
      if (perf_curr)
        *ret = perf_curr->pw_crnt_out_pckts;
      else
        *ret = 0;
    }
  else if (vc_temp)
    memset(ret, 0, sizeof (u_int64_t));
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;

}

u_int32_t
nsm_mpls_get_next_pw_crnt_out_pckts (struct nsm_master *nm, u_int32_t *pw_ix, 
                                     u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup_next (nm, *pw_ix, &vc, &vc_temp, NULL); 

  if ((vc) )
   {
     if (vc->nsm_mpls_pw_snmp_perf_curr_list)
       {
         perf_curr = GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list));
         if (perf_curr)
           {
             *ret = perf_curr->pw_crnt_out_pckts;
             *pw_ix = vc->vc_snmp_index;
           }
         else
           {
            *ret = 0;
            *pw_ix = vc->vc_snmp_index;    
           }           
       }
     else
       {
         *ret = 0;
         *pw_ix = vc->vc_snmp_index;
       }
   }
  else if (vc_temp)
   {
     memset(ret, 0, sizeof (u_int64_t));
     *pw_ix = vc_temp->vc_snmp_index;
   }
  else
   {
     *pw_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_crnt_out_bytes (struct nsm_master *nm,
                                u_int32_t pw_ix, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, &vc_temp, NULL);

  if ((vc) && (vc->nsm_mpls_pw_snmp_perf_curr_list))
    {
      perf_curr = GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list));
      if (perf_curr)
        *ret = perf_curr->pw_crnt_out_bytes;
      else
        *ret = 0;
    }
  else if (vc_temp)
    memset(ret, 0, sizeof (u_int64_t));
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;

}

u_int32_t
nsm_mpls_get_next_pw_crnt_out_bytes (struct nsm_master *nm, u_int32_t *pw_ix, 
                                     u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup_next (nm, *pw_ix, &vc, &vc_temp, NULL);

  if ((vc))
   {
     if (vc->nsm_mpls_pw_snmp_perf_curr_list)
       {
         perf_curr = GETDATA (LISTHEAD (vc->nsm_mpls_pw_snmp_perf_curr_list));
         if (perf_curr)
           {
             *ret = perf_curr->pw_crnt_out_bytes;
             *pw_ix = vc->vc_snmp_index;
           }
         else
           {
              *ret = 0;
              *pw_ix = vc->vc_snmp_index;  
           } 
       }
     else
       {
         *ret = 0;
         *pw_ix = vc->vc_snmp_index;
       }
   }
  else if (vc_temp)
   {
     memset(ret, 0, sizeof (u_int64_t));
     *pw_ix = vc_temp->vc_snmp_index;
   }
  else
   {
     *pw_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}

/* PW Performance Interval Table */

u_int32_t
nsm_mpls_get_pw_intrvl_valid_dt (struct nsm_master *nm,
                                 u_int32_t pw_ix, 
                                 u_int32_t pw_perf_intrvl_ix,u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL);

  if (vc && pw_perf_intrvl_ix > LISTCOUNT(vc->nsm_mpls_pw_snmp_perf_curr_list))
    return NSM_API_GET_ERROR;

  if ((vc))
    *ret = NSM_MPLS_PW_PAL_TRUE ;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_intrvl_valid_dt (struct nsm_master *nm, u_int32_t *pw_ix, 
                                      u_int32_t *pw_perf_intrvl_ix,
                                      u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_vc_snmp_lookup_next_by_intvl (nm, pw_ix, pw_perf_intrvl_ix); 

  if (vc)
   {
     *ret = NSM_MPLS_PW_PAL_TRUE;
     *pw_ix = vc->vc_snmp_index;
   }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_intrvl_time_elpsd (struct nsm_master *nm,
                                   u_int32_t pw_ix, 
                                   u_int32_t pw_perf_intrvl_ix,u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL);

  if (vc && pw_perf_intrvl_ix > LISTCOUNT(vc->nsm_mpls_pw_snmp_perf_curr_list))
    return NSM_API_GET_ERROR;

  if (vc)
    *ret = vc_timer_get_secs_elpsd (vc->t_time_elpsd);
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_intrvl_time_elpsd(struct nsm_master *nm, u_int32_t *pw_ix, 
                                      u_int32_t *pw_perf_intrvl_ix, 
                                      u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_vc_snmp_lookup_next_by_intvl (nm, pw_ix, pw_perf_intrvl_ix);
  if (vc)
   {
     *ret = vc_timer_get_secs_elpsd (vc->t_time_elpsd);
     *pw_ix = vc->vc_snmp_index;
   }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}

u_int64_t
nsm_mpls_get_pw_intrvl_in_hc_pckts (struct nsm_master *nm,
                                    u_int32_t pw_ix, 
                                    u_int32_t pw_perf_intrvl_ix,
                                    u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL);

  if (vc && pw_perf_intrvl_ix > LISTCOUNT(vc->nsm_mpls_pw_snmp_perf_curr_list))
    return NSM_API_GET_ERROR;

  if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_in_pckts;
              return NSM_API_GET_SUCCESS;
            }  
        }
    }

  return NSM_API_GET_ERROR;
}

u_int64_t
nsm_mpls_get_next_pw_intrvl_in_hc_pckts (struct nsm_master *nm, 
                                         u_int32_t *pw_ix, 
                                         u_int32_t *pw_perf_intrvl_ix, 
                                         u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode               *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

   vc = nsm_mpls_vc_snmp_lookup_next_by_intvl (nm, pw_ix, pw_perf_intrvl_ix);
   if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == *pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_in_pckts;
              *pw_ix = vc->vc_snmp_index;
              return NSM_API_GET_SUCCESS;
            }
        }
    }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}

u_int64_t
nsm_mpls_get_pw_intrvl_in_hc_bytes (struct nsm_master *nm,
                                    u_int32_t pw_ix, 
                                    u_int32_t pw_perf_intrvl_ix,
                                    u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode               *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL);

  if (vc && pw_perf_intrvl_ix > LISTCOUNT(vc->nsm_mpls_pw_snmp_perf_curr_list))
     return NSM_API_GET_ERROR;

  if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_in_bytes;
              return NSM_API_GET_SUCCESS;
            }
        }
    }

  return NSM_API_GET_ERROR;
}

u_int64_t
nsm_mpls_get_next_pw_intrvl_in_hc_bytes (struct nsm_master *nm, 
                                         u_int32_t *pw_ix, 
                                         u_int32_t *pw_perf_intrvl_ix,
                                         u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

  vc = nsm_mpls_vc_snmp_lookup_next_by_intvl (nm, pw_ix, pw_perf_intrvl_ix); 

  if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == *pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_in_bytes;
              *pw_ix = vc->vc_snmp_index;
              return NSM_API_GET_SUCCESS;
            }
        }
    }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}

u_int64_t
nsm_mpls_get_pw_intrvl_out_hc_pckts (struct nsm_master *nm,
                                     u_int32_t pw_ix,
                                     u_int32_t pw_perf_intrvl_ix,
                                     u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL);

  if (vc && pw_perf_intrvl_ix > LISTCOUNT(vc->nsm_mpls_pw_snmp_perf_curr_list))
    return NSM_API_GET_ERROR;

   if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_out_pckts;
              return NSM_API_GET_SUCCESS;
            }
        }
    }

  return NSM_API_GET_ERROR;
}

u_int64_t
nsm_mpls_get_next_pw_intrvl_out_hc_pckts (struct nsm_master *nm, 
                                          u_int32_t *pw_ix, 
                                          u_int32_t *pw_perf_intrvl_ix, 
                                          u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

  vc = nsm_mpls_vc_snmp_lookup_next_by_intvl (nm, pw_ix, pw_perf_intrvl_ix); 

  if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == *pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_out_pckts;
              *pw_ix = vc->vc_snmp_index;
              return NSM_API_GET_SUCCESS;
            }
        }
    }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}
                  

u_int64_t
nsm_mpls_get_pw_intrvl_out_hc_bytes (struct nsm_master *nm,
                                     u_int32_t pw_ix, 
                                     u_int32_t pw_perf_intrvl_ix,
                                     u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL);

  if (vc && pw_perf_intrvl_ix > LISTCOUNT(vc->nsm_mpls_pw_snmp_perf_curr_list))
    return NSM_API_GET_ERROR;

   if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_out_bytes;
              return NSM_API_GET_SUCCESS;
            }
        }
    }

  return NSM_API_GET_ERROR;
}

u_int64_t
nsm_mpls_get_next_pw_intrvl_out_hc_bytes(struct nsm_master *nm, 
                                         u_int32_t *pw_ix, 
                                         u_int32_t *pw_perf_intrvl_ix, 
                                         u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

  vc = nsm_mpls_vc_snmp_lookup_next_by_intvl (nm, pw_ix, pw_perf_intrvl_ix);

   if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == *pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_out_bytes;
              *pw_ix = vc->vc_snmp_index;
              return NSM_API_GET_SUCCESS;
            }
        }
    }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}


u_int32_t
nsm_mpls_get_pw_intrvl_in_pckts (struct nsm_master *nm,
                                 u_int32_t pw_ix, 
                                 u_int32_t pw_perf_intrvl_ix,u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode               *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL);

  if (vc && pw_perf_intrvl_ix > LISTCOUNT(vc->nsm_mpls_pw_snmp_perf_curr_list))
    return NSM_API_GET_ERROR;

   if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_in_pckts;
              return NSM_API_GET_SUCCESS;
            }
        }
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_mpls_get_next_pw_intrvl_in_pckts(struct nsm_master *nm, u_int32_t *pw_ix, 
                                     u_int32_t *pw_perf_intrvl_ix, 
                                     u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

  vc = nsm_mpls_vc_snmp_lookup_next_by_intvl (nm, pw_ix, pw_perf_intrvl_ix);

  if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == *pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_in_pckts;
              *pw_ix = vc->vc_snmp_index;
              return NSM_API_GET_SUCCESS;
            }
        }
    }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}


u_int32_t
nsm_mpls_get_pw_intrvl_in_bytes (struct nsm_master *nm,
                                 u_int32_t pw_ix, 
                                 u_int32_t pw_perf_intrvl_ix,u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL);

  if (vc && pw_perf_intrvl_ix > LISTCOUNT(vc->nsm_mpls_pw_snmp_perf_curr_list))
    return NSM_API_GET_ERROR;

   if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_in_bytes;
              return NSM_API_GET_SUCCESS;
            }
        }
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_mpls_get_next_pw_intrvl_in_bytes (struct nsm_master *nm, u_int32_t *pw_ix, 
                                      u_int32_t *pw_perf_intrvl_ix, 
                                      u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

  vc = nsm_mpls_vc_snmp_lookup_next_by_intvl (nm, pw_ix, pw_perf_intrvl_ix);
   if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == *pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_in_bytes;
              *pw_ix = vc->vc_snmp_index;
              return NSM_API_GET_SUCCESS;
            }
        }
    }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}


u_int32_t
nsm_mpls_get_pw_intrvl_out_pckts (struct nsm_master *nm,
                                  u_int32_t pw_ix, 
                                  u_int32_t pw_perf_intrvl_ix,u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL); 
  
  if (vc && pw_perf_intrvl_ix > LISTCOUNT(vc->nsm_mpls_pw_snmp_perf_curr_list))
    return NSM_API_GET_ERROR;

   if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_out_pckts;
              return NSM_API_GET_SUCCESS;
            }
        }
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_mpls_get_next_pw_intrvl_out_pckts (struct nsm_master *nm, u_int32_t *pw_ix,                              
                                       u_int32_t *pw_perf_intrvl_ix, 
                                       u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

  vc = nsm_mpls_vc_snmp_lookup_next_by_intvl (nm, pw_ix, pw_perf_intrvl_ix);

   if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == *pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_out_pckts;
              *pw_ix = vc->vc_snmp_index;
              return NSM_API_GET_SUCCESS;
            }
        }
    }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}


u_int32_t
nsm_mpls_get_pw_intrvl_out_bytes (struct nsm_master *nm,
                                  u_int32_t pw_ix, 
                                  u_int32_t pw_perf_intrvl_ix,u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL);

  if (vc && pw_perf_intrvl_ix > LISTCOUNT(vc->nsm_mpls_pw_snmp_perf_curr_list))
    return NSM_API_GET_ERROR;

   if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_out_bytes;
              return NSM_API_GET_SUCCESS;
            }
        }
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_mpls_get_next_pw_intrvl_out_bytes (struct nsm_master *nm, u_int32_t *pw_ix, 
                                       u_int32_t *pw_perf_intrvl_ix, 
                                       u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  u_int32_t count = 0;

  vc = nsm_mpls_vc_snmp_lookup_next_by_intvl (nm, pw_ix, pw_perf_intrvl_ix);

   if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        {
          count++;
          if (count == *pw_perf_intrvl_ix)
            {
              *ret = perf_curr->pw_crnt_out_bytes;
              *pw_ix = vc->vc_snmp_index;
              return NSM_API_GET_SUCCESS;
            }
        }
    }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}

/* PW Performance 1 Day Interval Table */

u_int32_t
nsm_mpls_get_pw_1dy_valid_dt (struct nsm_master *nm,
                              u_int32_t pw_ix, 
                              u_int32_t pw_perf_intrvl_ix,u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL);

  if (vc && pw_perf_intrvl_ix != NSM_MPLS_PW_MAX_1DY_PERF_COUNT)
    return NSM_API_GET_ERROR;

  if (vc)
    *ret = NSM_MPLS_PW_PAL_TRUE;
  else
    return NSM_API_GET_ERROR;


  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_1dy_valid_dt (struct nsm_master *nm, u_int32_t *pw_ix, 
                                   u_int32_t *pw_perf_intrvl_ix, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_vc_snmp_lookup_next_by_1dy_intvl (nm, pw_ix, pw_perf_intrvl_ix);
  
  if (vc)
   {
     *ret = NSM_MPLS_PW_PAL_TRUE;
     *pw_ix = vc->vc_snmp_index;
     return NSM_API_GET_SUCCESS;
   }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}


u_int32_t
nsm_mpls_get_pw_1dy_moni_secs (struct nsm_master *nm,
                               u_int32_t pw_ix, 
                               u_int32_t pw_perf_intrvl_ix,u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  u_int32_t interval = 1;
  u_int32_t time = 0;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL);  

  if (vc && pw_perf_intrvl_ix != NSM_MPLS_PW_MAX_1DY_PERF_COUNT)
    return NSM_API_GET_ERROR;

  if ((vc))
    {
      if (vc->nsm_mpls_pw_snmp_perf_curr_list)
        interval = LISTCOUNT (vc->nsm_mpls_pw_snmp_perf_curr_list);
 
      if (interval > 1 ) 
        time = ((interval - 1) * VC_TIMER_15MIN) + 
          vc_timer_get_secs_elpsd (vc->t_time_elpsd);
      else 
        time = vc_timer_get_secs_elpsd (vc->t_time_elpsd);

      *ret = time;
    }
  else
    return NSM_API_GET_ERROR;


  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_1dy_moni_secs (struct nsm_master *nm, u_int32_t *pw_ix, 
                                    u_int32_t *pw_perf_intrvl_ix, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  u_int32_t interval = 1;
  u_int32_t time = 0;

  vc = nsm_mpls_vc_snmp_lookup_next_by_1dy_intvl (nm, pw_ix, pw_perf_intrvl_ix); 

  if (vc)
   {
     if (vc->nsm_mpls_pw_snmp_perf_curr_list)
       interval = LISTCOUNT (vc->nsm_mpls_pw_snmp_perf_curr_list);
    
     if (interval > 1 )
        time = (((interval - 1) * VC_TIMER_15MIN) +
          vc_timer_get_secs_elpsd (vc->t_time_elpsd));
      else
        time = vc_timer_get_secs_elpsd (vc->t_time_elpsd);
 
     *ret = time;
     *pw_ix = vc->vc_snmp_index;
     return NSM_API_GET_SUCCESS;
   }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}

u_int64_t
nsm_mpls_get_pw_1dy_in_hc_pckts (struct nsm_master *nm,
                                 u_int32_t pw_ix, 
                                 u_int32_t pw_perf_intrvl_ix,
                                 u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL);

  if (vc && pw_perf_intrvl_ix != NSM_MPLS_PW_MAX_1DY_PERF_COUNT)
    return NSM_API_GET_ERROR;

  if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        *ret = *ret + perf_curr->pw_crnt_in_pckts;

      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int64_t
nsm_mpls_get_next_pw_1dy_in_hc_pckts (struct nsm_master *nm, u_int32_t *pw_ix,
                                      u_int32_t *pw_perf_intrvl_ix, 
                                      u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;
  
  vc = nsm_mpls_vc_snmp_lookup_next_by_1dy_intvl (nm, pw_ix, pw_perf_intrvl_ix);

  if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        *ret = *ret + perf_curr->pw_crnt_in_pckts;
      *pw_ix = vc->vc_snmp_index;
    }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}

u_int64_t
nsm_mpls_get_pw_1dy_in_hc_bytes (struct nsm_master *nm,
                                 u_int32_t pw_ix, 
                                 u_int32_t pw_perf_intrvl_ix,
                                 u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode               *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL);

  if (vc && pw_perf_intrvl_ix != NSM_MPLS_PW_MAX_1DY_PERF_COUNT)
    return NSM_API_GET_ERROR;

  if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        *ret = *ret + perf_curr->pw_crnt_in_bytes;

      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int64_t
nsm_mpls_get_next_pw_1dy_in_hc_bytes (struct nsm_master *nm, u_int32_t *pw_ix, 
                                      u_int32_t *pw_perf_intrvl_ix, 
                                      u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  vc = nsm_mpls_vc_snmp_lookup_next_by_1dy_intvl (nm, pw_ix, pw_perf_intrvl_ix);
  if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        *ret = *ret + perf_curr->pw_crnt_in_bytes;

      *pw_ix = vc->vc_snmp_index;
    }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}

u_int64_t
nsm_mpls_get_pw_1dy_out_hc_pckts (struct nsm_master *nm,
                                  u_int32_t pw_ix, 
                                  u_int32_t pw_perf_intrvl_ix,
                                  u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL);

  if (vc && pw_perf_intrvl_ix != NSM_MPLS_PW_MAX_1DY_PERF_COUNT)
    return NSM_API_GET_ERROR;

  if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        *ret = *ret + perf_curr->pw_crnt_out_pckts;
      
      return NSM_API_GET_SUCCESS;      
    }

  return NSM_API_GET_ERROR;
}

u_int64_t
nsm_mpls_get_next_pw_1dy_out_hc_pckts (struct nsm_master *nm, u_int32_t *pw_ix, 
                                       u_int32_t *pw_perf_intrvl_ix,
                                       u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  vc = nsm_mpls_vc_snmp_lookup_next_by_1dy_intvl (nm, pw_ix, pw_perf_intrvl_ix);

  if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        *ret = *ret + perf_curr->pw_crnt_out_pckts;

      *pw_ix = vc->vc_snmp_index;
    }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}

u_int64_t
nsm_mpls_get_pw_1dy_out_hc_bytes (struct nsm_master *nm,
                                  u_int32_t pw_ix, 
                                  u_int32_t pw_perf_intrvl_ix,
                                  u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  nsm_mpls_vc_snmp_lookup (nm, pw_ix, &vc, NULL, NULL); 

  if (vc && pw_perf_intrvl_ix != NSM_MPLS_PW_MAX_1DY_PERF_COUNT)
    return NSM_API_GET_ERROR;

  if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        *ret = *ret + perf_curr->pw_crnt_out_bytes;
    
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int64_t
nsm_mpls_get_next_pw_1dy_out_hc_bytes (struct nsm_master *nm, u_int32_t *pw_ix, 
                                       u_int32_t *pw_perf_intrvl_ix,
                                       u_int64_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct listnode *curr_node = NULL;
  struct nsm_mpls_pw_snmp_perf_curr  *perf_curr = NULL;

  vc = nsm_mpls_vc_snmp_lookup_next_by_1dy_intvl (nm, pw_ix, pw_perf_intrvl_ix);

  if (vc)
    {
      LIST_LOOP (vc->nsm_mpls_pw_snmp_perf_curr_list,perf_curr,curr_node)
        *ret = *ret + perf_curr->pw_crnt_out_bytes;

      *pw_ix = vc->vc_snmp_index;
    }
  else
   {
     *pw_ix = 0;
     *pw_perf_intrvl_ix = 0;
     return NSM_API_GET_ERROR;
   }

  return NSM_API_GET_SUCCESS;
}


u_int32_t
nsm_mpls_get_pw_id_map_pw_ix (struct nsm_master *nm,
                              u_int32_t pw_type, u_int32_t pw_ix,
                              u_int32_t peer_addr_type,
                              struct pal_in4_addr peer_addr, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  int flag = 0;
  vc_id = pw_ix;

  nsm_mpls_l2_circuit_lookup (nm, vc_id, &vc_temp, &vc);

  if ((vc) && (vc->vc_info))
    {
      if (vc->vc_info->vc_type != pw_type)
        flag = 1;
    }
  if (vc)
    {
      if ((vc->address.family != nsm_mpls_get_vc_addr_fam (peer_addr_type)) ||
          (vc->address.u.prefix4.s_addr != peer_addr.s_addr))
        flag = 1;
    }
  if (vc_temp)
    {
      if ((vc_temp->vc_type != pw_type) ||
          (vc_temp->peer_addr_type != peer_addr_type) ||
          (vc_temp->peer_addr.s_addr != peer_addr.s_addr))
        flag = 1;
    }

  if (flag == 0)
    {
      if ((vc && vc->fec_type_vc == PW_OWNER_GEN_FEC_SIGNALING))
        *ret = vc->vc_snmp_index;
      else if ((vc_temp && vc_temp->pw_owner == PW_OWNER_GEN_FEC_SIGNALING))
        *ret = vc_temp->vc_snmp_index;
      else
         *ret = 0;
    
      return NSM_API_GET_SUCCESS;  
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_mpls_get_next_pw_id_map_pw_ix (struct nsm_master *nm,
                                   u_int32_t *pw_type, u_int32_t *pw_ix,
                                   u_int32_t *peer_addr_type,
                                   struct pal_in4_addr *peer_addr,
                                   u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = *pw_ix;

  vc = nsm_mpls_l2_circuit_lookup_next_by_addr (nm, pw_type, peer_addr_type,
                                                peer_addr, pw_ix);
  
  if ((vc) && vc->vc_info ) 
    {
      if (vc->fec_type_vc == PW_OWNER_GEN_FEC_SIGNALING)
        {
          *ret = vc->vc_snmp_index;
          *pw_type = vc->vc_info->vc_type;
          *pw_ix = vc->id;
          *peer_addr_type = INET_AD_IPV4;
          peer_addr->s_addr = vc->address.u.prefix4.s_addr;
        }
      else
        {
          *ret = 0;
          *pw_type = vc->vc_info->vc_type;
          *pw_ix = vc->id;
          *peer_addr_type = INET_AD_IPV4;
          peer_addr->s_addr = vc->address.u.prefix4.s_addr;
        }
    }
  else
    {
      *ret = 0;
      *pw_type = 0;
      *pw_ix = 0;
      *peer_addr_type = 0;
      peer_addr->s_addr = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_peer_map_pw_ix (struct nsm_master *nm,
                                u_int32_t peer_addr_type,
                                struct pal_in4_addr peer_addr,
                                u_int32_t pw_type,
                                u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  int flag = 0;

  vc_id = pw_ix;

  nsm_mpls_l2_circuit_lookup (nm, vc_id, &vc_temp, &vc);

  if ((vc) && (vc->vc_info))
    {
      if (vc->vc_info->vc_type != pw_type)
        flag = 1;
    }
  if (vc)
    {
      if ((vc->address.family != nsm_mpls_get_vc_addr_fam (peer_addr_type)) ||
          (vc->address.u.prefix4.s_addr != peer_addr.s_addr))
        flag = 1;
    }
  if (vc_temp)
    {
      if ((vc_temp->vc_type != pw_type) ||
          (vc_temp->peer_addr_type != peer_addr_type) ||
          (vc_temp->peer_addr.s_addr != peer_addr.s_addr))
        flag = 1;
    }

  if (flag == 0)
    {
      if (vc)
        *ret = vc->vc_snmp_index;
      else if (vc_temp)
        *ret = vc_temp->vc_snmp_index;

      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_mpls_get_next_pw_peer_map_pw_ix (struct nsm_master *nm,
                                     u_int32_t *peer_addr_type,
                                     struct pal_in4_addr *peer_addr,
                                     u_int32_t *pw_type,
                                     u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = *pw_ix;

  vc = nsm_mpls_l2_circuit_lookup_next_by_addr (nm, pw_type, peer_addr_type,
                                                peer_addr, pw_ix);

  if ((vc) && (vc->vc_info))
    {
      *ret = vc->vc_snmp_index;
      *peer_addr_type = INET_AD_IPV4;
      peer_addr->s_addr = vc->address.u.prefix4.s_addr;
      *pw_type = vc->vc_info->vc_type;
      *pw_ix = vc->id;
    }
  else
    {
      *peer_addr_type = 0;
      peer_addr->s_addr = 0;
      *pw_type = 0;
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}
#ifdef HAVE_FEC129
/* Generalised FEC Table */
u_int32_t
nsm_mpls_get_pw_gen_fec_vc_id (struct nsm_master *nm,
                               u_int32_t pw_gen_fec_agi_type,
                               u_int32_t pw_gen_fec_agi,
                               u_int32_t pw_gen_fec_loc_aii_type,
                               u_int32_t pw_gen_fec_loc_aii,
                               u_int32_t pw_gen_fec_rmte_aii_type,
                               u_int32_t pw_gen_fec_rmte_aii, 
                               u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = pw_gen_fec_agi;

  nsm_mpls_l2_circuit_lookup (nm, vc_id, &vc_temp, &vc);

  if ((vc) && (vc->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING))
     return NSM_API_GET_ERROR;

  if ((vc_temp) && (vc_temp->pw_owner != PW_OWNER_GEN_FEC_SIGNALING))
     return NSM_API_GET_ERROR; 

  if ((vc) && (vc->vc_info))
     *ret = vc->vc_snmp_index;
  else if (vc_temp)
     *ret = vc_temp->vc_snmp_index;
  else
      return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_gen_fec_vc_id (struct nsm_master *nm,
                                     u_int32_t *pw_gen_fec_agi_type,
                                     u_int32_t *pw_gen_fec_agi,
                                     u_int32_t *pw_gen_fec_loc_aii_type,
                                     u_int32_t *pw_gen_fec_loc_aii,
                                     u_int32_t *pw_gen_fec_rmte_aii_type,
                                     u_int32_t *pw_gen_fec_rmte_aii, 
                                     u_int32_t *ret)

{ 
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  vc_id = *pw_gen_fec_agi;

  nsm_mpls_l2_circuit_lookup_next (nm, vc_id, &vc_temp, &vc);

  if ((vc) && vc->fec_type_vc != PW_OWNER_GEN_FEC_SIGNALING)
    return NSM_API_GET_ERROR;

  if ((vc_temp) && vc_temp->pw_owner != PW_OWNER_GEN_FEC_SIGNALING)
    return NSM_API_GET_ERROR;

  if ((vc))
    {
      if (vc->vc_info)
        {
          *ret = vc->vc_snmp_index;
          *pw_gen_fec_agi_type = vc->vc_info->vc_type;
          *pw_gen_fec_agi = vc->id;
          *pw_gen_fec_loc_aii_type = 0;
          *pw_gen_fec_loc_aii = 0;
          *pw_gen_fec_rmte_aii_type = 0;
          *pw_gen_fec_rmte_aii = 0;
        }
      else
        {
          *ret = 0;
          *pw_gen_fec_agi_type = 0;
          *pw_gen_fec_agi = 0;
          *pw_gen_fec_loc_aii_type = 0;
          *pw_gen_fec_loc_aii = 0;
          *pw_gen_fec_rmte_aii_type = 0;
          *pw_gen_fec_rmte_aii = 0;
          return NSM_API_GET_ERROR;
        }
    }
  else if (vc_temp)
    {
      *ret = vc_temp->vc_snmp_index;
      *pw_gen_fec_agi_type = vc_temp->vc_type;
      *pw_gen_fec_agi = vc->id;
      *pw_gen_fec_loc_aii_type = 0;
      *pw_gen_fec_loc_aii = 0;
      *pw_gen_fec_rmte_aii_type = 0;
      *pw_gen_fec_rmte_aii = 0;
    }
  else
    {
      *pw_gen_fec_agi_type = 0;
      *pw_gen_fec_agi = 0;
      *pw_gen_fec_loc_aii_type = 0;
      *pw_gen_fec_loc_aii = 0;
      *pw_gen_fec_rmte_aii_type = 0;
      *pw_gen_fec_rmte_aii = 0;

      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}
#endif /*HAVE_FEC129 */


/* Create new Active NSM MPLS Circuit */
u_int32_t
nsm_mpls_create_new_nsm_mpls_circuit (struct nsm_master *nm,
                             struct nsm_mpls_circuit_temp **vc_temp_ptr,
                             struct nsm_mpls_circuit **vc_ptr)
{
  struct nsm_mpls_circuit *vc = NULL;
  struct nsm_mpls_circuit_container *vc_container = NULL;
  struct nsm_mpls_circuit_temp *vc_temp = NULL;
  struct nsm_mpls_vc_group *group = NULL;
  char group_name [255];
  int ret = NSM_SUCCESS;

  pal_mem_set (group_name, 0, (sizeof (char) * 255));

  vc_temp = *vc_temp_ptr;

  /* Lookup using name and id. */
  vc_container = nsm_mpls_vc_get_by_name (nm, vc_temp->vc_name);

  if (!vc_container)
    return NSM_ERR_MEM_ALLOC_FAILURE;

  /* If this is a new one. */
  if (! vc_container->vc)
    {
      vc_container->vc = nsm_mpls_l2_circuit_create (nm, 
                                                     vc_temp->vc_name,
                                                     vc_temp->vc_id, 
                                                     NSM_FALSE, &ret);
    }

  vc = vc_container->vc;
  if (!vc)
    return NSM_FAILURE; /* nsm_err_mem_alloc_failure */

  /* set the other temp values to vc */
  vc->vc_snmp_index = vc_temp->vc_snmp_index;

  /* Set the group name from the VC temp. */
  if (vc_temp->local_grp_id)
    {
      if (vc_temp->grp_name)
        pal_strcpy (group_name, vc_temp->grp_name);
      else
        {
          group = nsm_mpls_l2_vc_group_lookup_by_id (nm, vc_temp->local_grp_id);
          if (group)
            pal_strcpy (group_name, group->name);
          else
            {
              pal_sprintf (group_name, "L2VCGROUP%d", vc_temp->local_grp_id);

              /* Group with the generated name has been configured by user
               * for different grp_id, unrecoverable Failure 
               */
              if (nsm_mpls_l2_vc_group_lookup_by_name (nm, group_name) != NULL)
                return NSM_FAILURE;
            }
        }
    }

  ret = nsm_mpls_vc_fill_data (nm,
                               vc_temp->vc_id,
                               &vc_temp->peer_addr,
                               (vc_temp->local_grp_id ? group_name : NULL), 
                               vc_temp->local_grp_id,
                               vc_temp->cw,
                               MPLS_VC_MAPPING_NONE,
                               NULL, 0,
                               vc_temp->pw_owner,
#ifdef HAVE_VCCV
                               0, 0,
#endif /* HAVE_VCCV */
                               vc);
  if (ret < 0)
    return NSM_FAILURE;

  if (vc_temp->remote_grp_id)
    vc->remote_grp_id = vc_temp->remote_grp_id;

  if (vc->vc_info)
    {
      vc->vc_info->pw_attchd_pw_ix = vc_temp->pw_attchd_pw_ix;
      /* If vc_temp->mtu is not set, set it to Default*/
      if (vc_temp->mtu)
        vc->vc_info->mif->ifp->mtu = vc_temp->mtu;
      else
        vc->vc_info->mif->ifp->mtu = IF_ETHER_DEFAULT_MTU;

      /* If vc_temp->vc_type is not set, set it to Default*/
      if (vc_temp->vc_type)
        vc->vc_info->vc_type = vc_temp->vc_type;
      else
        vc->vc_info->vc_type = VC_TYPE_ETHERNET;
    }
 
  vc->grp_attchmt_id = vc_temp->grp_attchmt_id;
  vc->local_attchmt_id = vc_temp->local_attchmt_id;
  vc->peer_attchmt_id = vc_temp->peer_attchmt_id;

  if (vc_temp->vc_fib_temp)
    nsm_mpls_vc_fib_add_msg_process (nm, vc_temp->vc_fib_temp);

  vc->setup_prty = vc_temp->setup_prty;
  vc->hold_prty = vc_temp->hold_prty;
  vc->row_status = RS_ACTIVE;
  *vc_ptr = vc;
  return NSM_SUCCESS;
}

u_int32_t
nsm_mpls_create_new_vc_temp (struct nsm_master *nm,
                             struct nsm_mpls_circuit *vc, u_int32_t pw_ix)
{
  struct nsm_mpls_circuit_temp *vc_temp = NULL;

  vc_temp = XCALLOC (MTYPE_NSM_VIRTUAL_CIRCUIT,
                     sizeof (struct nsm_mpls_circuit_temp));
  if (!vc_temp)
    return NSM_FAILURE;
  
  vc_temp->vc_snmp_index = pw_ix;

  SET_FLAG (vc_temp->flags, VC_ENTRY_FLAG_ID_SET);
  if (vc)
    {
      vc_temp->vc_id = vc->id;
      vc_temp->vc_name = XSTRDUP (MTYPE_TMP, vc->name);
      vc_temp->cw = vc->cw;
      if (vc->address.family == AF_INET)
        vc_temp->peer_addr_type = PW_PEER_ADDR_TYPE_IPV4;
#ifdef HAVE_IPV6
      else if (vc->address.family == AF_INET6)
        vc_temp->peer_addr_type = PW_PEER_ADDR_TYPE_IPV6;
#endif /* HAVE_IPV6 */
    
      IPV4_ADDR_COPY (&vc_temp->peer_addr,&vc->address.u.prefix4);
      if (vc->vc_info)
        {
          vc_temp->pw_attchd_pw_ix = vc->vc_info->pw_attchd_pw_ix;
   
          if (vc->vc_info->mif && vc->vc_info->mif->ifp)
            vc_temp->mtu = vc->vc_info->mif->ifp->mtu;
    
          if (vc->vc_info->vc_type)
            vc_temp->vc_type = vc->vc_info->vc_type;
          else
            vc_temp->vc_type = VC_TYPE_ETHERNET;
       }
   
      if (vc->group)
        {
          vc_temp->local_grp_id = vc->group->id;
          vc_temp->grp_name = XSTRDUP (MTYPE_TMP, vc->group->name);
        }

      if (vc->remote_grp_id)
        vc_temp->remote_grp_id = vc->remote_grp_id;

      vc_temp->grp_attchmt_id = vc->grp_attchmt_id;
      vc_temp->local_attchmt_id = vc->local_attchmt_id;
      vc_temp->peer_attchmt_id = vc->peer_attchmt_id;
      if (vc->vc_fib)
        {
          vc_temp->cw_status = vc->vc_fib->c_word;
          vc_temp->oper_status = vc->vc_fib->install_flag;
        }
      vc_temp->pw_owner = vc->fec_type_vc;
      vc_temp->setup_prty = vc->setup_prty;
      vc_temp->hold_prty = vc->hold_prty;
      vc_temp->pw_owner = vc->fec_type_vc;
      vc_temp->row_status = RS_NOT_IN_SERVICE;

      SET_FLAG (vc_temp->flags, VC_ENTRY_FLAG_ID_SET);
      SET_FLAG (vc_temp->flags, VC_ENTRY_FLAG_NAME_SET);
      SET_FLAG (vc_temp->flags, VC_ENTRY_FLAG_PEER_ADDR_SET);
      SET_FLAG (vc_temp->flags, VC_ENTRY_FLAG_PEER_ADDR_TYPE_SET);
      if (vc->vc_fib && vc->fec_type_vc == PW_OWNER_MANUAL)
        {
          vc_temp->vc_fib_temp = 
            XMALLOC (MTYPE_TMP, sizeof (struct nsm_msg_vc_fib_add));

          if (!vc_temp->vc_fib_temp)
            return NSM_API_SET_ERROR;

          pal_mem_set (vc_temp->vc_fib_temp, 0, 
              sizeof (struct nsm_msg_vc_fib_add));

          vc_temp->vc_fib_temp->addr.afi = AFI_IP;
          pal_mem_cpy (&vc_temp->vc_fib_temp->addr.u.ipv4, 
              &vc->address.u.prefix4, sizeof (struct pal_in4_addr));
          vc_temp->vc_fib_temp->vc_style = MPLS_VC_STYLE_MARTINI;
          vc_temp->vc_fib_temp->vc_id = vc->id;
          vc_temp->vc_fib_temp->lsr_id = vc->vc_fib->lsr_id;
          vc_temp->vc_fib_temp->in_label = vc->vc_fib->in_label;
          vc_temp->vc_fib_temp->out_label = vc->vc_fib->out_label;
          vc_temp->vc_fib_temp->nw_if_ix = vc->vc_fib->nw_if_ix;
          vc_temp->vc_fib_temp->ac_if_ix = vc->vc_fib->ac_if_ix;
        }
    }
  else  /* Entry to be newly created */
    vc_temp->row_status = RS_NOT_READY;

  listnode_add (vc_entry_temp_list, vc_temp);
  return NSM_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_up_dn_notify (struct nsm_mpls *nmpls, u_int32_t val)
{
  if (nmpls != NULL)
    {
      SET_PW_VAL(nmpls->vc_pw_notification,NSM_MPLS_VC_UP_DN_NOTIFN,val);
      return NSM_API_SET_SUCCESS;
    } 
  return NSM_API_SET_ERROR;  
}

u_int32_t
nsm_mpls_get_pw_up_dn_notify (struct nsm_master *nm, u_int32_t *val)
{
  if (nm != NULL)
    {
      if (nm->nmpls->vc_pw_notification & NSM_MPLS_VC_UP_DN_NOTIFN)
        *val = NSM_MPLS_SNMP_VC_UP_DN_NTFY_ENA;
      else
        *val = NSM_MPLS_SNMP_VC_UP_DN_NTFY_DIS;

      return NSM_API_SET_SUCCESS;
    }
  
  return NSM_API_GET_ERROR;
}


u_int32_t
nsm_mpls_set_pw_del_notify (struct nsm_mpls *nmpls, u_int32_t val)
{
  if (nmpls != NULL)
    {
      SET_PW_VAL(nmpls->vc_pw_notification,NSM_MPLS_VC_DEL_NOTIFN,(val<<1));
      return NSM_API_SET_SUCCESS;
    } 
  
  return NSM_API_SET_ERROR;  
}

u_int32_t
nsm_mpls_get_pw_del_notify (struct nsm_master *nm, u_int32_t *val)
{
  if (nm != NULL)
    {
      if ((nm->nmpls->vc_pw_notification & NSM_MPLS_VC_DEL_NOTIFN) >>1)
        *val = NSM_MPLS_SNMP_VC_DEL_NTFY_ENA;
      else
        *val = NSM_MPLS_SNMP_VC_DEL_NTFY_DIS;

      return NSM_API_SET_SUCCESS;
    }

    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_mpls_set_pw_notify_rate (struct nsm_mpls *nmpls, u_int32_t val)
{
  if (nmpls != NULL)
    {
      nmpls->vc_pw_notification_rate = val ;
      return NSM_API_SET_SUCCESS;
    } 
  return NSM_API_SET_ERROR;  
}

u_int32_t
nsm_mpls_get_pw_notify_rate (struct nsm_master *nm, u_int32_t *val)
{
  if (nm != NULL)
    {
      *val = nm->nmpls->vc_pw_notification_rate;
      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_mpls_set_pw_enet_pw_vlan (struct nsm_master *nm, u_int32_t pw_ix, 
                              u_int32_t pw_instance, u_int32_t pw_vlan)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);
  if (! vc)
    return NSM_API_SET_ERROR;

  /* if VC info temp does not exists, rowStatus is not NOTINSERVICE,
   * so not ready for updates */
  if (!vc->vc_info_temp || vc->vc_info_temp->pw_enet_pw_instance != pw_instance)
    return NSM_API_SET_ERROR;

  vc->vc_info_temp->pw_enet_pw_vlan = pw_vlan;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_enet_vlan_mode (struct nsm_master *nm, u_int32_t pw_ix, 
                                u_int32_t pw_instance, u_int32_t pw_vlan_mode)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);
  if (! vc)
    return NSM_API_SET_ERROR;

  /* if VC info temp does not exists, rowStatus is not NOTINSERVICE,
   * so not ready for updates */
  if (!vc->vc_info_temp || vc->vc_info_temp->pw_enet_pw_instance != pw_instance)
    return NSM_API_SET_ERROR;

  /* For Port based vlans */
  if (vc->vc_info_temp->pw_enet_pw_vlan == NSM_PW_DEFAULT_VLAN)
    {
      if (pw_vlan_mode != PW_ENET_MODE_PORT_BASED)
        return NSM_API_SET_ERROR;
    }
  else
    {
      /* We only support noChange for Vlan based VC's */
      if (pw_vlan_mode != PW_ENET_MODE_NO_CHANGE)
        return NSM_API_SET_ERROR;
    }

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_enet_port_vlan (struct nsm_master *nm, u_int32_t pw_ix, 
                              u_int32_t pw_instance, u_int32_t pw_port_vlan)
{
  struct nsm_mpls_circuit *vc = NULL;

  /* Cannot set to 0.*/
  if (!pw_port_vlan)
    return NSM_API_SET_ERROR;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);

  if (! vc)
    return NSM_API_SET_ERROR;

  /* if VC info temp does not exists, rowStatus is not NOTINSERVICE,
   * so not ready for updates */
  if (!vc->vc_info_temp || vc->vc_info_temp->pw_enet_pw_instance != pw_instance)
    return NSM_API_SET_ERROR;

  /* We only support pwEentVlanMode noChange/portBased, In these cases
   * value for this OID should be equal to pwEnetpwVlan */
  if (vc->vc_info_temp->pw_enet_pw_vlan != pw_port_vlan)
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_enet_port_if_index (struct nsm_master *nm, u_int32_t pw_ix, 
                                    u_int32_t pw_instance, u_int32_t ifindex)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);

  if (! vc)
    return NSM_API_SET_ERROR;

  /* if VC info temp does not exists, rowStatus is not NOTINSERVICE,
   * so not ready for updates */
  if (!vc->vc_info_temp || vc->vc_info_temp->pw_enet_pw_instance != pw_instance)
    return NSM_API_SET_ERROR;

  /* Not validation done for ifp, Same will be done when rowstatus
   * is set to ACTIVE, where actual binding will happen */
  vc->vc_info_temp->pw_enet_port_ifindex = ifindex;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_enet_pw_if_index (struct nsm_master *nm, u_int32_t pw_ix, 
                                  u_int32_t pw_instance, u_int32_t pw_ifindex)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);

  if (! vc)
    return NSM_API_SET_ERROR;

  /* if VC info temp does not exists, rowStatus is not NOTINSERVICE,
   * so not ready for updates */
  if (!vc->vc_info_temp || vc->vc_info_temp->pw_enet_pw_instance != pw_instance)
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_enet_row_status (struct nsm_master *nm,
                                 u_int32_t pw_ix, 
                                 u_int32_t pw_instance, 
                                 u_int32_t row_status)
{
  struct nsm_mpls_vc_info_temp *vc_info_temp;
  u_int16_t vc_type = VC_TYPE_UNKNOWN;
  struct nsm_mpls_circuit *vc = NULL;
  struct interface *ifp = NULL;
  u_int16_t vlan_id = 0;
  u_int16_t vc_mode = 0;
  int ret = NSM_FAILURE;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);
  if (! vc)
    return NSM_API_SET_ERROR;

  switch (row_status)
    {
    case RS_ACTIVE:
      /* If vc->vc_info exists, rowStatus is already ACTIVE. Simply return */
      if (vc->vc_info)
        return NSM_API_SET_SUCCESS;

      /* VC temp info is not yet created, or if user provided the incorrect
       * enet pw instance value, Return Error */
      if (!vc->vc_info_temp || 
          vc->vc_info_temp->pw_enet_pw_instance != pw_instance)
        return NSM_API_SET_ERROR;

      /* Check if we have the enough data to perform the set operation */
      if (!vc->vc_info_temp->pw_enet_port_ifindex)
        return NSM_API_SET_ERROR;

      /* Validations on the interface provided by the user */
      ifp = if_lookup_by_index (&nm->vr->ifm, 
                                vc->vc_info_temp->pw_enet_port_ifindex);

      /* Interface Not found or interface is not layer2 interface. 
       * Return error. 
       */
      if (!ifp || ifp->type != IF_TYPE_L2)
        return NSM_API_SET_ERROR;

      /* VPLS binding exists on the interface, return Error. We seem to be 
       * allowing multiple VC's binding though on same interface 
       */
#ifdef HAVE_VPLS
      if (CHECK_FLAG (ifp->bind, NSM_IF_BIND_VPLS))
        return NSM_API_SET_ERROR;
#endif /* HAVE_VPLS */

      /* Based on the vlan_id set the mode */
      if (vc->vc_info_temp->pw_enet_pw_vlan == NSM_PW_DEFAULT_VLAN)
        {
          vc_type = VC_TYPE_ETHERNET;
          vlan_id = 0;
        }
      else
        {
          vc_type = VC_TYPE_ETH_VLAN;
          vlan_id = vc->vc_info_temp->pw_enet_pw_vlan;

#ifdef HAVE_VLAN
          ret = nsm_vlan_id_validate_by_interface (ifp, vlan_id);
#endif /* HAVE_VLAN */
          if (ret != NSM_SUCCESS)
            return NSM_API_SET_ERROR;
        }

      /* Keep the vc_info_temp pointer and set vc->vc_info_temp to NULL.
       * Otherwise bind will fail below. */
      vc_info_temp = vc->vc_info_temp;
      vc->vc_info_temp = NULL;

      ret = nsm_mpls_l2_circuit_bind_by_ifp (ifp, vc->name, vc_type, 
                                            vlan_id, vc_info_temp->vc_mode);

      if (ret < 0 || !vc->vc_info)
        {
          vc->vc_info_temp = vc_info_temp;
          return NSM_API_SET_ERROR;
        }

      vc->vc_info->pw_enet_pw_instance = pw_instance;

      /* Delete the VC info temp */
      XFREE (MTYPE_TMP, vc_info_temp);
      break;

    case RS_NOT_IN_SERVICE:
      if (vc->vc_info)
        {
          /* Make sure user provided the correct enet pw instance */
          if (vc->vc_info->pw_enet_pw_instance != pw_instance)
            return NSM_API_SET_ERROR;

          /* Only Ethernet bindings are supported by this MIB */
          if ((vc->vc_info->vc_type != VC_TYPE_ETHERNET &&
                vc->vc_info->vc_type != VC_TYPE_ETH_VLAN))
            return NSM_API_SET_ERROR;

          /* Null checks for mif and ifp */
          if (! vc->vc_info->mif || ! vc->vc_info->mif->ifp)
            return NSM_API_SET_ERROR;

          ifp = vc->vc_info->mif->ifp;
          vlan_id = vc->vc_info->vlan_id;

          /* Store the mode to re-use when entry is set to active again */
          vc_mode = vc->vc_info->vc_mode;

          ret = nsm_mpls_l2_circuit_unbind_by_ifp (ifp, vc->name, vlan_id);
          if (ret < 0)
            return NSM_API_SET_ERROR;

          /* Create the Temporary entry */
          vc->vc_info_temp = XMALLOC (MTYPE_TMP, 
                                      sizeof (struct nsm_mpls_vc_info_temp));

          /* memory allocation failure */
          if (!vc->vc_info_temp)
            return NSM_ERR_MEM_ALLOC_FAILURE;

          /* Copy the req values from the vc structure */
          vc->vc_info_temp->pw_enet_pw_instance = pw_instance;

          if (vlan_id)
            vc->vc_info_temp->pw_enet_pw_vlan = vlan_id;
          else
            vc->vc_info_temp->pw_enet_pw_vlan = NSM_PW_DEFAULT_VLAN;

          vc->vc_info_temp->pw_enet_port_ifindex = ifp->ifindex;

          vc->vc_info_temp->vc_mode = vc_mode;
        }
      else if (vc->vc_info_temp)
        {
          if (vc->vc_info_temp->pw_enet_pw_instance != pw_instance)
            return NSM_API_SET_ERROR;

          /* If req data to set the status to Active is not available,
           * status MUST BE RS_NOT_READY */
          if (!vc->vc_info_temp->pw_enet_port_ifindex)
            return NSM_API_SET_ERROR;
          else
            return NSM_API_SET_SUCCESS;
        }
      else
        return NSM_API_SET_ERROR;

      break;

    case RS_NOT_READY:
      /* This rowStatus is internally set, not Settable by the user */
      return NSM_API_SET_ERROR;
      break;

    case RS_CREATE_GO:
      /* Not Supported. We only support Create and wait for rowcreation */
      return NSM_API_SET_ERROR;
      break;

    case RS_CREATE_WAIT:
      /* Binding already exists for the VC. Return Error */
      if (vc->vc_info || vc->vc_info_temp)
        return NSM_API_SET_ERROR;

      /* Create the temporary entry and set the defaults */
      vc->vc_info_temp = XMALLOC (MTYPE_TMP, 
                                  sizeof (struct nsm_mpls_vc_info_temp));
      vc->vc_info_temp->pw_enet_pw_instance = pw_instance;
      vc->vc_info_temp->pw_enet_pw_vlan = NSM_PW_DEFAULT_VLAN;
      vc->vc_info_temp->vc_mode = 0;

      break;

    case RS_DESTROY:
      if (vc->vc_info)
        {
          /* Only Ethernet related PW's are handled as part of this MIB */
          if ((vc->vc_info->vc_type != VC_TYPE_ETHERNET &&
                vc->vc_info->vc_type != VC_TYPE_ETH_VLAN))
            return NSM_API_SET_ERROR;

          /* Make sure user provided the correct enet pw instance */
          if (vc->vc_info->pw_enet_pw_instance != pw_instance)
            return NSM_API_SET_ERROR;

          /* Sanity check for mif */
          if (! vc->vc_info->mif)
            return NSM_API_SET_ERROR;

          ret = nsm_mpls_l2_circuit_unbind_by_ifp (vc->vc_info->mif->ifp, 
                                                   vc->name, 
                                                   vc->vc_info->vlan_id);
          if (ret < 0)
            return NSM_API_SET_ERROR;
        }
      else if (vc->vc_info_temp)
        {
          /* Make sure user provided the correct enet pw instance */
          if (vc->vc_info_temp->pw_enet_pw_instance != pw_instance)
            return NSM_API_SET_ERROR;

          /* just free the temp */
          XFREE (MTYPE_TMP, vc->vc_info_temp);
          vc->vc_info_temp = NULL;
        }
      else
        return NSM_API_SET_ERROR;

      break;   

    default:
      return NSM_API_SET_ERROR;
      break;
    }

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_enet_storage_type (struct nsm_master *nm, 
                                   u_int32_t pw_ix, 
                                   u_int32_t pw_instance, 
                                   u_int32_t pw_storage_type)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);
  if (! vc)
    return NSM_API_SET_ERROR;

  /* if VC info temp does not exists, rowStatus is not NOTINSERVICE,
   * so not ready for updates */
  if (!vc->vc_info_temp || vc->vc_info_temp->pw_enet_pw_instance != pw_instance)
    return NSM_API_SET_ERROR;

  /* Since only one supported value, no need to store */
  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_enet_pw_vlan (struct nsm_master *nm, u_int32_t pw_ix, 
                              u_int32_t pw_instance, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);
  if (! vc)
    return NSM_API_GET_ERROR;

  if (vc->vc_info)
    {
      if ((vc->vc_info->vc_type != VC_TYPE_ETHERNET &&
          vc->vc_info->vc_type != VC_TYPE_ETH_VLAN) ||
          vc->vc_info->pw_enet_pw_instance != pw_instance)
        return NSM_API_GET_ERROR;


      if (vc->vc_info->vlan_id)
        *ret = vc->vc_info->vlan_id;
      else
        *ret = NSM_PW_DEFAULT_VLAN;

      return NSM_API_GET_SUCCESS;
    }
  else if (vc->vc_info_temp)
    {
      if (vc->vc_info_temp->pw_enet_pw_instance != pw_instance)
        return NSM_API_GET_ERROR;

      *ret = vc->vc_info_temp->pw_enet_pw_vlan;

      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_ERROR; 
}

u_int32_t
nsm_mpls_get_next_pw_enet_pw_vlan (struct nsm_master *nm, u_int32_t *pw_ix, 
                                   u_int32_t *pw_instance, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_pw_enet_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, 
                                           *pw_ix, *pw_instance);  
  if (!vc)
    return NSM_API_GET_ERROR;

  if (vc->vc_info)
    {
      if ((vc->vc_info->vc_type != VC_TYPE_ETHERNET &&
          vc->vc_info->vc_type != VC_TYPE_ETH_VLAN))
        return NSM_API_GET_ERROR;


      if (vc->vc_info->vlan_id)
        *ret = vc->vc_info->vlan_id;
      else
        *ret = NSM_PW_DEFAULT_VLAN;

      *pw_ix = vc->vc_snmp_index;
      *pw_instance =vc->vc_info->pw_enet_pw_instance;
    }
  else if (vc->vc_info_temp)
    {
      *ret = vc->vc_info_temp->pw_enet_pw_vlan;

      *pw_ix = vc->vc_snmp_index;
      *pw_instance =vc->vc_info_temp->pw_enet_pw_instance;
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS; 
}

u_int32_t
nsm_mpls_get_pw_enet_vlan_mode (struct nsm_master *nm, u_int32_t pw_ix, 
                                u_int32_t pw_instance, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);
  if (!vc)
    return NSM_API_GET_ERROR;

  if (vc->vc_info)
    {
      if (vc->vc_info->pw_enet_pw_instance != pw_instance)
        return NSM_API_GET_ERROR;

      if (vc->vc_info->vc_type == VC_TYPE_ETHERNET)
        *ret = PW_ENET_MODE_PORT_BASED;
      else if (vc->vc_info->vc_type == VC_TYPE_ETH_VLAN)
        *ret = PW_ENET_MODE_NO_CHANGE;
      else
        return NSM_API_GET_ERROR;
    }
  else if (vc->vc_info_temp)
    {
      if (vc->vc_info_temp->pw_enet_pw_instance != pw_instance)
        return NSM_API_GET_ERROR;

      if (vc->vc_info_temp->pw_enet_pw_vlan == NSM_PW_DEFAULT_VLAN)
        *ret = PW_ENET_MODE_PORT_BASED;
      else
        *ret = PW_ENET_MODE_NO_CHANGE;
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_enet_vlan_mode (struct nsm_master *nm, u_int32_t *pw_ix, 
                                     u_int32_t *pw_instance, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_pw_enet_snmp_get_vc_next_index (nm, NSM_MPLS->vc_table, 
                                           *pw_ix, *pw_instance);
  if (!vc)
    return NSM_API_GET_ERROR;

  if (vc->vc_info)
    {
      if (vc->vc_info->vc_type == VC_TYPE_ETHERNET)
        *ret = PW_ENET_MODE_PORT_BASED;
      else if (vc->vc_info->vc_type == VC_TYPE_ETH_VLAN)
        *ret = PW_ENET_MODE_NO_CHANGE;
      else
        return NSM_API_GET_ERROR;

      *pw_ix = vc->vc_snmp_index;
      *pw_instance = vc->vc_info->pw_enet_pw_instance;
    }
  else if (vc->vc_info_temp)
    {
      if (vc->vc_info_temp->pw_enet_pw_vlan == NSM_PW_DEFAULT_VLAN)
        *ret = PW_ENET_MODE_PORT_BASED;
      else
        *ret = PW_ENET_MODE_NO_CHANGE;

      *pw_ix = vc->vc_snmp_index;
      *pw_instance = vc->vc_info_temp->pw_enet_pw_instance;
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_enet_port_vlan (struct nsm_master *nm, u_int32_t pw_ix, 
                                u_int32_t pw_instance, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);
  if (! vc)
    return NSM_API_GET_ERROR;

  if (vc->vc_info)
    {
      if ((vc->vc_info->vc_type != VC_TYPE_ETHERNET &&
          vc->vc_info->vc_type != VC_TYPE_ETH_VLAN) ||
          vc->vc_info->pw_enet_pw_instance != pw_instance)
        return NSM_API_GET_ERROR;

      if (vc->vc_info->vlan_id)
        *ret = vc->vc_info->vlan_id;
      else
        *ret = NSM_PW_DEFAULT_VLAN;
    }
  else if (vc->vc_info_temp)
    {
      if (vc->vc_info_temp->pw_enet_pw_instance != pw_instance)
        return NSM_API_GET_ERROR;

      *ret = vc->vc_info_temp->pw_enet_pw_vlan;
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_enet_port_vlan (struct nsm_master *nm, u_int32_t *pw_ix, 
                                     u_int32_t *pw_instance, u_int32_t *ret)
{ 
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_pw_enet_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, 
                                           *pw_ix, *pw_instance);
  if (! vc)
    return NSM_API_GET_ERROR;

  if (vc->vc_info)
    {
      if ((vc->vc_info->vc_type != VC_TYPE_ETHERNET &&
          vc->vc_info->vc_type != VC_TYPE_ETH_VLAN))
        return NSM_API_GET_ERROR;

      if (vc->vc_info->vlan_id)
        *ret = vc->vc_info->vlan_id;
      else
        *ret = NSM_PW_DEFAULT_VLAN;

      *pw_ix = vc->vc_snmp_index;
      *pw_instance = vc->vc_info->pw_enet_pw_instance;
    }
  else if (vc->vc_info_temp)
    {
      *ret = vc->vc_info_temp->pw_enet_pw_vlan;

      *pw_ix = vc->vc_snmp_index;
      *pw_instance = vc->vc_info_temp->pw_enet_pw_instance;
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;

}

u_int32_t
nsm_mpls_get_pw_enet_port_if_index (struct nsm_master *nm, u_int32_t pw_ix, 
                                    u_int32_t pw_instance, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);
  if (!vc)
    return NSM_API_GET_ERROR;

  if (vc->vc_info)
    {
      if ((vc->vc_info->vc_type != VC_TYPE_ETHERNET &&
          vc->vc_info->vc_type != VC_TYPE_ETH_VLAN) ||
          vc->vc_info->pw_enet_pw_instance != pw_instance)
        return NSM_API_GET_ERROR;

      if (!vc->vc_info->mif || !vc->vc_info->mif->ifp)
        return NSM_API_GET_ERROR;

      *ret = vc->vc_info->mif->ifp->ifindex;
    }
  else if (vc->vc_info_temp)
    {
      if (vc->vc_info_temp->pw_enet_pw_instance != pw_instance)
        return NSM_API_GET_ERROR;

      *ret = vc->vc_info_temp->pw_enet_port_ifindex;
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_enet_port_if_index (struct nsm_master *nm, 
                                         u_int32_t *pw_ix, 
                                         u_int32_t *pw_instance, 
                                         u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_pw_enet_snmp_get_vc_next_index (nm, NSM_MPLS->vc_table, 
                                           *pw_ix, *pw_instance);
  if (!vc)
    return NSM_API_GET_ERROR;

  if (vc->vc_info)
    {
      if ((vc->vc_info->vc_type != VC_TYPE_ETHERNET &&
          vc->vc_info->vc_type != VC_TYPE_ETH_VLAN))
        return NSM_API_GET_ERROR;

      if (!vc->vc_info->mif || !vc->vc_info->mif->ifp)
        return NSM_API_GET_ERROR;

      *ret = vc->vc_info->mif->ifp->ifindex;

      *pw_ix = vc->vc_snmp_index;
      *pw_instance = vc->vc_info->pw_enet_pw_instance;
    }
  else if (vc->vc_info_temp)
    {
      *ret = vc->vc_info_temp->pw_enet_port_ifindex;

      *pw_ix = vc->vc_snmp_index;
      *pw_instance = vc->vc_info_temp->pw_enet_pw_instance;
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_enet_pw_if_index (struct nsm_master *nm, u_int32_t pw_ix, 
                                  u_int32_t pw_instance, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc =  nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);
  if (!vc)
    return NSM_API_GET_ERROR;

  if (vc->vc_info)
    {
      if ((vc->vc_info->vc_type != VC_TYPE_ETHERNET &&
          vc->vc_info->vc_type != VC_TYPE_ETH_VLAN) ||
          vc->vc_info->pw_enet_pw_instance != pw_instance)
        return NSM_API_GET_ERROR;
    }
  else if (vc->vc_info_temp)
    {
      if (vc->vc_info_temp->pw_enet_pw_instance != pw_instance)
        return NSM_API_GET_ERROR;
    }
  else
    return NSM_API_GET_ERROR;

  /* Value returned is always zero, since we don't map the PW in the ifTable
   * as an interface */
  *ret = 0;
  return NSM_API_GET_SUCCESS;  
}

u_int32_t
nsm_mpls_get_next_pw_enet_pw_if_index (struct nsm_master *nm, u_int32_t *pw_ix,
                                       u_int32_t *pw_instance, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_pw_enet_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, 
                                           *pw_ix, *pw_instance);
  if (!vc)
    return NSM_API_GET_ERROR;

  if (vc->vc_info)
    {
      if ((vc->vc_info->vc_type != VC_TYPE_ETHERNET &&
          vc->vc_info->vc_type != VC_TYPE_ETH_VLAN))
        return NSM_API_GET_ERROR;

      *pw_ix = vc->vc_snmp_index;
      *pw_instance = vc->vc_info->pw_enet_pw_instance;
    }
  else if (vc->vc_info_temp)
    {
      *pw_ix = vc->vc_snmp_index;
      *pw_instance = vc->vc_info_temp->pw_enet_pw_instance;
    }
  else
    return NSM_API_GET_ERROR;

  /* Value returned is always zero, since we don't map the PW in the ifTable
   * as an interface */
  *ret = 0;
  return NSM_API_GET_SUCCESS;  

}

u_int32_t
nsm_mpls_get_pw_enet_row_status(struct nsm_master *nm, u_int32_t pw_ix, 
                                u_int32_t pw_instance, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);
  if (! vc)
    return NSM_API_GET_ERROR;

  if (vc->vc_info)
    {
      if ((vc->vc_info->vc_type != VC_TYPE_ETHERNET &&
          vc->vc_info->vc_type != VC_TYPE_ETH_VLAN) ||
          vc->vc_info->pw_enet_pw_instance != pw_instance)
        return NSM_API_GET_ERROR;

      /* vc->vc_info exists means row is in ACTIVE state */
      *ret = RS_ACTIVE;
    }
  else if (vc->vc_info_temp)
    {
      if (vc->vc_info_temp->pw_enet_port_ifindex)
        *ret = RS_NOT_IN_SERVICE;
      else
        *ret = RS_NOT_READY;
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS; 
}

u_int32_t
nsm_mpls_get_next_pw_enet_row_status(struct nsm_master *nm, u_int32_t *pw_ix, 
                                     u_int32_t *pw_instance, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_pw_enet_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, 
                                           *pw_ix, *pw_instance);
  if (! vc)
    return NSM_API_GET_ERROR;

  if (vc->vc_info)
    {
      if ((vc->vc_info->vc_type != VC_TYPE_ETHERNET &&
          vc->vc_info->vc_type != VC_TYPE_ETH_VLAN))
        return NSM_API_GET_ERROR;

      *ret = RS_ACTIVE;

      *pw_ix = vc->vc_snmp_index;
      *pw_instance = vc->vc_info->pw_enet_pw_instance;
    }
  else if (vc->vc_info_temp)
    {
      if (vc->vc_info_temp->pw_enet_port_ifindex)
        *ret = RS_NOT_IN_SERVICE;
      else
        *ret = RS_NOT_READY;

      *pw_ix = vc->vc_snmp_index;
      *pw_instance = vc->vc_info_temp->pw_enet_pw_instance;
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS; 

}

u_int32_t
nsm_mpls_get_pw_enet_storage_type (struct nsm_master *nm, u_int32_t pw_ix,
                                   u_int32_t pw_instance, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);
  if (!vc)
    return NSM_API_GET_ERROR;

  if (vc->vc_info)
    {
      if ((vc->vc_info->vc_type != VC_TYPE_ETHERNET &&
          vc->vc_info->vc_type != VC_TYPE_ETH_VLAN) ||
          vc->vc_info->pw_enet_pw_instance != pw_instance)
        return NSM_API_GET_ERROR;
    }
  else if (vc->vc_info_temp)
    {
      if (vc->vc_info_temp->pw_enet_pw_instance != pw_instance)
        return NSM_API_GET_ERROR;
    }
  else
    return NSM_API_GET_ERROR;

  /* We only support volatile */
  *ret = ST_VOLATILE;
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_enet_storage_type (struct nsm_master *nm, u_int32_t *pw_ix,
                                        u_int32_t *pw_instance, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_pw_enet_snmp_get_vc_next_index (nm, NSM_MPLS->vc_table, 
                                           *pw_ix, *pw_instance);
  if (!vc)
    return NSM_API_GET_ERROR;

  if (vc->vc_info)
    {
      if ((vc->vc_info->vc_type != VC_TYPE_ETHERNET &&
          vc->vc_info->vc_type != VC_TYPE_ETH_VLAN))
        return NSM_API_GET_ERROR;

      *pw_ix = vc->vc_snmp_index;
      *pw_instance = vc->vc_info->pw_enet_pw_instance;
    }
  else if (vc->vc_info_temp)
    {
      *pw_ix = vc->vc_snmp_index;
      *pw_instance = vc->vc_info_temp->pw_enet_pw_instance;
    }
  else
    return NSM_API_GET_ERROR;

  /* We only support volatile */
  *ret = ST_VOLATILE;
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_enet_stats_illegal_vlan (struct nsm_master *nm, 
                                         u_int32_t pw_ix,
                                         u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);

  if (vc && vc->vc_info &&
      (vc->vc_info->vc_type == VC_TYPE_ETHERNET ||
       vc->vc_info->vc_type == VC_TYPE_ETH_VLAN))
    {
      *ret = 0;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_mpls_get_next_pw_enet_stats_illegal_vlan (struct nsm_master *nm, 
                                              u_int32_t *pw_ix, 
                                              u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, *pw_ix);

  if (vc && vc->vc_info &&
      (vc->vc_info->vc_type == VC_TYPE_ETHERNET ||
       vc->vc_info->vc_type == VC_TYPE_ETH_VLAN))
    {
      *ret = 0;
      *pw_ix = vc->vc_snmp_index;
    }
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_enet_stats_illegal_length (struct nsm_master *nm,
                                           u_int32_t pw_ix, 
                                           u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, pw_ix);

  if (vc && vc->vc_info &&
      (vc->vc_info->vc_type == VC_TYPE_ETHERNET ||
       vc->vc_info->vc_type == VC_TYPE_ETH_VLAN))
    {
      *ret = 0;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_mpls_get_next_pw_enet_stats_illegal_length (struct nsm_master *nm,
                                                u_int32_t *pw_ix, 
                                                u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, *pw_ix);

  if (vc && vc->vc_info &&
      (vc->vc_info->vc_type == VC_TYPE_ETHERNET ||
       vc->vc_info->vc_type == VC_TYPE_ETH_VLAN))
    {           
      *ret = 0;
      *pw_ix = vc->vc_snmp_index;
    }
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  return NSM_API_GET_SUCCESS;
}


u_int32_t
nsm_mpls_get_pw_mpls_mpls_type (struct nsm_master *nm,
                                u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if ((vc) && !(vc->owner) && (vc->ftn))
    {
      if ((vc->ftn->owner.owner == MPLS_OTHER) ||
          (vc->ftn->owner.owner == MPLS_SNMP))
        pal_sprintf (*ret, "%d", PWONLY);
      else if (vc->ftn->owner.owner == MPLS_LDP)
        pal_sprintf (*ret, "%d", MPLSNONTE);
      else if (vc->ftn->owner.owner == MPLS_RSVP)
        pal_sprintf (*ret, "%d", MPLSTE);
      else
        return NSM_API_GET_ERROR;
    }

  else if (vc)
    {
      if ((vc->owner == MPLS_OTHER) || (vc->owner == MPLS_SNMP))
        pal_sprintf (*ret, "%d", PWONLY);
      else if (vc->owner == MPLS_LDP)
        pal_sprintf (*ret, "%d", MPLSNONTE);
      else if (vc->owner == MPLS_RSVP)
        pal_sprintf (*ret, "%d", MPLSTE);
      else
        return NSM_API_GET_ERROR;
    }

  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_mpls_type (struct nsm_master *nm,
                                     u_int32_t *pw_ix, char **ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;
  while (NSM_TRUE)
  {
    vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, vc_id);
    if (! vc)
       return NSM_API_GET_ERROR;

    if (!(vc->owner) && (vc->ftn))
      {
        if ((vc->ftn->owner.owner == MPLS_OTHER) ||
            (vc->ftn->owner.owner == MPLS_SNMP))
          pal_sprintf (*ret, "%d", PWONLY);
        else if (vc->ftn->owner.owner == MPLS_LDP)
          pal_sprintf (*ret, "%d", MPLSNONTE);
        else if (vc->ftn->owner.owner == MPLS_RSVP)
          pal_sprintf (*ret, "%d", MPLSTE);
        else
          return NSM_API_GET_ERROR;
      }
    else
      {
        if ((vc->owner == MPLS_OTHER) || (vc->owner == MPLS_SNMP))
          pal_sprintf (*ret, "%d",PWONLY);
        else if (vc->owner == MPLS_LDP)
          pal_sprintf (*ret, "%d", MPLSNONTE);
        else if (vc->owner == MPLS_RSVP)
          pal_sprintf (*ret, "%d", MPLSTE);
        else
          return NSM_API_GET_ERROR;
      }

    *pw_ix = vc->vc_snmp_index;
    return NSM_API_GET_SUCCESS;
  }
}

u_int32_t
nsm_mpls_set_pw_mpls_mpls_type (struct nsm_master *nm,
                                u_int32_t pw_ix, char *mpls_type)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  int type = MPLSTE;
  vc_id = pw_ix;

  /* Check for validity of value */
  if (mpls_type)
    {
      if (pal_strcmp (mpls_type, "2") == 0)
        type = PWONLY;
      else if (pal_strcmp (mpls_type, "1") == 0)
        type = MPLSNONTE;
      else if (pal_strcmp (mpls_type, "0") == 0)
        type = MPLSTE;
      else
        return NSM_API_SET_ERROR;
    }

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (vc)
    {
      if (type == PWONLY)
        {
          vc->owner = MPLS_SNMP;
          vc->dn = PSNBOUND;
        }
      else if (type == MPLSNONTE)
        {
          vc->owner = MPLS_LDP;
          vc->dn = PSNBOUND;
        }
      else
        vc->owner = MPLS_RSVP;
    }
  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_mpls_exp_bits_mode (struct nsm_master *nm,
                                    u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (vc)
    *ret = NSM_MPLS_PW_OUTER_TUNNEL;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_exp_bits_mode (struct nsm_master *nm,
                                         u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, vc_id);

  if (vc)
    {
      *ret = NSM_MPLS_PW_OUTER_TUNNEL;
      *pw_ix = vc->vc_snmp_index;
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_mpls_exp_bits_mode (struct nsm_master *nm,
                                    u_int32_t pw_ix, u_int32_t mode)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (mode != NSM_MPLS_PW_OUTER_TUNNEL)
    return NSM_API_SET_ERROR;

  if (vc) /* If entry present */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_get_pw_mpls_exp_bits (struct nsm_master *nm,
                               u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (vc)
    *ret = 0;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_exp_bits (struct nsm_master *nm,
                                    u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, vc_id);

  if (vc)
    {
      *ret = 0;
      *pw_ix = vc->vc_snmp_index;
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_mpls_exp_bits (struct nsm_master *nm,
                               u_int32_t pw_ix, u_int32_t bits)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (bits != 0)
    return NSM_API_SET_ERROR;

  if (vc) /* If entry present */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_get_pw_mpls_ttl (struct nsm_master *nm,
                          u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (vc)
    *ret = NSM_MPLS_PW_TTL_VALUE;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_ttl (struct nsm_master *nm,
                               u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, vc_id);

  if (vc)
    {
      *ret = NSM_MPLS_PW_TTL_VALUE;
      *pw_ix = vc->vc_snmp_index;
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_mpls_ttl (struct nsm_master *nm,
                          u_int32_t pw_ix, u_int32_t ttl)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (ttl != NSM_MPLS_PW_TTL_VALUE)
    return NSM_API_SET_ERROR;

  if (vc) /* If entry present */
    return NSM_API_SET_SUCCESS;

  return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_get_pw_mpls_lcl_ldp_id (struct nsm_master *nm,
                                 u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct pal_in4_addr lcl_ldp_id;
  vc_id = pw_ix;
  
  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (!vc)
    return NSM_API_GET_ERROR;
  if (vc->vc_fib)
    {
      lcl_ldp_id.s_addr = pal_hton32 (vc->vc_fib->lsr_id);
      pal_inet_ntop (AF_INET,
                     &(lcl_ldp_id),
                     *ret,
                     NSM_PW_MAX_STRING_LENGTH);
      pal_strcat (*ret, ":");
      pal_strcat (*ret, "00");
    }
  else if (nm->nmpls)
    {
      lcl_ldp_id.s_addr = pal_hton32 (nm->nmpls->lsr_id);
      pal_inet_ntop (AF_INET,
                     &(lcl_ldp_id),
                     *ret,
                     NSM_PW_MAX_STRING_LENGTH);
      pal_strcat (*ret, ":");
      pal_strcat (*ret, "00");
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_lcl_ldp_id (struct nsm_master *nm,
                                      u_int32_t *pw_ix, char **ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;
  struct pal_in4_addr lcl_ldp_id;

  if (nm->nmpls)
    vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, vc_id);
  if (!vc)
    return NSM_API_GET_ERROR;

  if (vc->vc_fib)
    {
      lcl_ldp_id.s_addr = pal_hton32 (vc->vc_fib->lsr_id);
      pal_inet_ntop (AF_INET,
                     &(lcl_ldp_id),
                     *ret,
                     NSM_PW_MAX_STRING_LENGTH);
      pal_strcat (*ret, ":");
      pal_strcat (*ret, "00");
    }
  else if (nm->nmpls)
    {
      lcl_ldp_id.s_addr = pal_hton32 (nm->nmpls->lsr_id);
      pal_inet_ntop (AF_INET,
                     &(lcl_ldp_id),
                     *ret,
                     NSM_PW_MAX_STRING_LENGTH);
      pal_strcat (*ret, ":");
      pal_strcat (*ret, "00");
    }
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  *pw_ix = vc->vc_snmp_index;
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_mpls_lcl_ldp_id (struct nsm_master *nm, u_int32_t pw_ix,
                                 struct ldp_id *lcl_ldp_id)
{
  u_int32_t vc_id;
  int  res = -1;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);
  if (!vc)
    return NSM_API_SET_ERROR;
  if (vc->vc_fib)
    {
      if (vc->vc_fib->lsr_id != lcl_ldp_id->lsr_id)
        res = nsm_mpls_l2_circuit_del (nm, vc, NSM_TRUE);
    }
  else if (nm->nmpls)
    {
      if (nm->nmpls->lsr_id != lcl_ldp_id->lsr_id)
        res = nsm_mpls_l2_circuit_del (nm, vc, NSM_TRUE);
    }
  else
    return NSM_API_SET_ERROR;
  if (res < 0)
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_mpls_lcl_ldp_entty_ix (struct nsm_master *nm,
                                       u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;
  
  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);
  if (!vc)
    return NSM_API_GET_ERROR;
  if (vc->vc_fib)
    *ret = vc->vc_fib->index;

  else if (nm->nmpls)
    *ret = nm->nmpls->index;

  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_lcl_ldp_entty_ix (struct nsm_master *nm,
                                            u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;

  if (nm->nmpls)
    vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, vc_id);
  if (!vc)
    return NSM_API_GET_ERROR;
  if (vc->vc_fib)
    *ret = vc->vc_fib->index;
  else if (nm->nmpls)
    *ret = nm->nmpls->index;
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  *pw_ix = vc->vc_snmp_index;
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_mpls_lcl_ldp_entty_ix (struct nsm_master *nm,
                                       u_int32_t pw_ix, u_int32_t entity_ix)
{
  u_int32_t vc_id;
  int  res = -1;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);
  if (!vc)
    return NSM_API_SET_ERROR;
  if (vc->vc_fib)
    {
      if (vc->vc_fib->index != entity_ix)
        res = nsm_mpls_l2_circuit_del (nm, vc, NSM_TRUE);
    }
  else if (nm->nmpls)
    {
      if (nm->nmpls->index != entity_ix)
        res = nsm_mpls_l2_circuit_del (nm, vc, NSM_TRUE);
    }
  else
    return NSM_API_SET_ERROR;
  if (res < 0)
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_mpls_peer_ldp_id (struct nsm_master *nm,
                                  u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;
  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (vc)
    pal_inet_ntop (AF_INET,
                   &(vc->address.u.prefix4),
                   *ret,
                   NSM_PW_MAX_STRING_LENGTH);
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_peer_ldp_id (struct nsm_master *nm,
                                       u_int32_t *pw_ix, char **ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;
  vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, vc_id);

  if (vc)
    {
      pal_inet_ntop (AF_INET,
                     &(vc->address.u.prefix4),
                     *ret,
                     NSM_PW_MAX_STRING_LENGTH);
      *pw_ix = vc->vc_snmp_index;
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_mpls_st_type (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (vc)
    {
      *ret = ST_VOLATILE;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_st_type (struct nsm_master *nm,
                                   u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, vc_id);

  if (vc)
    {
      *ret = ST_VOLATILE;
      *pw_ix = vc->vc_snmp_index;
      return NSM_API_GET_SUCCESS;
    }

  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_mpls_set_pw_mpls_st_type (struct nsm_master *nm,
                              u_int32_t pw_ix, u_int32_t st_type)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (st_type != ST_VOLATILE)
    return NSM_API_SET_ERROR;

  if (vc)
    return NSM_API_SET_SUCCESS;

  else
    return NSM_API_SET_ERROR;
}

u_int32_t
nsm_mpls_get_pw_mpls_outbd_lsr_xc_ix (struct nsm_master *nm,
                                      u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;
  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (!vc)
    return NSM_API_GET_ERROR;

  if (((vc->owner) && (vc->owner != MPLS_RSVP)) || ((vc->ftn) &&
                                   (vc->ftn->owner.owner != MPLS_RSVP)))
    {
      if ((vc->ftn) && (vc->ftn->xc))
        *ret = vc->ftn->xc->key.xc_ix;
      else
        *ret = 0;
    }
  else
    *ret = 0;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_outbd_lsr_xc_ix (struct nsm_master *nm,
                                           u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;
  vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, vc_id);

  if (!vc)
    return NSM_API_GET_ERROR;

  if (((vc->owner) && (vc->owner != MPLS_RSVP)) || ((vc->ftn) &&
                                   (vc->ftn->owner.owner != MPLS_RSVP)))
    {
      if ((vc->ftn) && (vc->ftn->xc))
        *ret = vc->ftn->xc->key.xc_ix;
      else
        *ret = 0;
    }
  else
    *ret = 0;

  *pw_ix = vc->vc_snmp_index;
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_mpls_outbd_lsr_xc_ix (struct nsm_master *nm,
                                      u_int32_t pw_ix, u_int32_t xc_ix)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (!vc)
    return NSM_API_SET_ERROR;
  if (((vc->owner) && (vc->owner != MPLS_RSVP)) || ((vc->ftn) &&
                                   (vc->ftn->owner.owner != MPLS_RSVP)))
    return NSM_API_SET_ERROR;

  else if ((vc->owner == MPLS_OTHER) || (vc->owner == MPLS_SNMP) ||
           ((vc->ftn) && (vc->ftn->owner.owner == MPLS_OTHER)) ||
           ((vc->ftn) && (vc->ftn->owner.owner == MPLS_SNMP)))
    {
      if ((vc->ftn) && (vc->ftn->xc))
        vc->ftn->xc->key.xc_ix = xc_ix;
    }
  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_mpls_outbd_tnl_ix (struct nsm_master *nm,
                                   u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;
  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (!vc)
    return NSM_API_GET_ERROR;
  if (((vc->owner) && (vc->owner != MPLS_RSVP)) || ((vc->ftn) &&
                                   (vc->ftn->owner.owner != MPLS_RSVP)))
    *ret = 0;
  else if (vc->ftn)
    *ret = vc->ftn->tun_id;
  else
    *ret = 0;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_outbd_tnl_ix (struct nsm_master *nm,
                                        u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;
  vc = nsm_pw_snmp_get_vc_next_index (nm, NSM_MPLS->vc_table, vc_id);

  if (!vc)
    return NSM_API_GET_ERROR;
  if (((vc->owner) && (vc->owner != MPLS_RSVP)) || ((vc->ftn) &&
                                   (vc->ftn->owner.owner != MPLS_RSVP)))
    *ret = 0;
  else if (vc->ftn)
    *ret = vc->ftn->tun_id;
  else
    *ret = 0;

  *pw_ix = vc->vc_snmp_index;
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_mpls_outbd_tnl_ix (struct nsm_master *nm,
                                   u_int32_t pw_ix, u_int32_t tnl_ix)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (!vc)
    return NSM_API_SET_ERROR;
  if (((vc->owner) && (vc->owner != MPLS_RSVP)) || ((vc->ftn) &&
                                   (vc->ftn->owner.owner != MPLS_RSVP)))
    return NSM_API_SET_ERROR;

  if ((vc->ftn) && (tnl_ix != vc->ftn->tun_id))
    return NSM_API_SET_ERROR;
  else
    return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_mpls_outbd_tnl_instnce (struct nsm_master *nm,
                                        u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;
  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (!vc)
    return NSM_API_GET_ERROR;
  if (((vc->owner) && (vc->owner != MPLS_RSVP)) || ((vc->ftn) &&
                                   (vc->ftn->owner.owner != MPLS_RSVP)))
    *ret = 0;
  else if (vc->ftn)
    *ret = vc->ftn->protected_lsp_id;
  else
    *ret = 0;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_outbd_tnl_instnce (struct nsm_master *nm,
                                             u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;
  vc = nsm_pw_snmp_get_vc_next_index (nm, NSM_MPLS->vc_table, vc_id);

  if (!vc)
    return NSM_API_GET_ERROR;
  if (((vc->owner) && (vc->owner != MPLS_RSVP)) || ((vc->ftn) &&
                                   (vc->ftn->owner.owner != MPLS_RSVP)))
    *ret = 0;
  else if (vc->ftn)
    *ret = vc->ftn->protected_lsp_id;
  else
    *ret = 0;

  *pw_ix = vc->vc_snmp_index;
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_mpls_outbd_tnl_lcl_lsr (struct nsm_master *nm,
                                        u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  struct pal_in4_addr lcl_ldp_id;
  vc_id = pw_ix;
  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);
  if (!vc)
    return NSM_API_GET_ERROR;
  if (((vc->owner) && (vc->owner != MPLS_RSVP)) || ((vc->ftn) &&
                                   (vc->ftn->owner.owner != MPLS_RSVP)))
    **ret = 0;
  else if (vc->vc_fib)
    {
      lcl_ldp_id.s_addr = pal_hton32 (vc->vc_fib->lsr_id);
      pal_inet_ntop (AF_INET,
                     &(lcl_ldp_id),
                     *ret,
                     NSM_PW_MAX_STRING_LENGTH);
    }
  else if (nm->nmpls)
    {
      lcl_ldp_id.s_addr = pal_hton32 (nm->nmpls->lsr_id);
      pal_inet_ntop (AF_INET,
                     &(lcl_ldp_id),
                     *ret,
                     NSM_PW_MAX_STRING_LENGTH);
    }
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_outbd_tnl_lcl_lsr (struct nsm_master *nm,
                                             u_int32_t *pw_ix, char **ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;
  struct pal_in4_addr lcl_ldp_id;

  if (nm->nmpls)
    vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, vc_id);
  if (!vc)
    return NSM_API_GET_ERROR;

  if (((vc->owner) && (vc->owner != MPLS_RSVP)) || ((vc->ftn) &&
                                   (vc->ftn->owner.owner != MPLS_RSVP)))
    **ret = 0;
  else if (vc->vc_fib)
    {
      lcl_ldp_id.s_addr = pal_hton32 (vc->vc_fib->lsr_id);
      pal_inet_ntop (AF_INET,
                     &(lcl_ldp_id),
                     *ret,
                     NSM_PW_MAX_STRING_LENGTH);
    }
  else if (nm->nmpls)
    {
      lcl_ldp_id.s_addr = pal_hton32 (nm->nmpls->lsr_id);
      pal_inet_ntop (AF_INET,
                     &(lcl_ldp_id),
                     *ret,
                     NSM_PW_MAX_STRING_LENGTH);
    }
  else
    {
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }

  *pw_ix = vc->vc_snmp_index;
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_mpls_outbd_tnl_lcl_lsr (struct nsm_master *nm,
                                        u_int32_t pw_ix,
                                        struct pal_in4_addr *lcl_lsr)
{
  u_int32_t vc_id;
  int  res = -1;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);
  if (!vc)
    return NSM_API_SET_ERROR;
  if (((vc->owner) && (vc->owner != MPLS_RSVP)) || ((vc->ftn) &&
                                   (vc->ftn->owner.owner != MPLS_RSVP)))
    return NSM_API_SET_ERROR;

  if (vc->vc_fib)
    {
      if (vc->vc_fib->lsr_id != lcl_lsr->s_addr)
        res = nsm_mpls_l2_circuit_del (nm, vc, NSM_TRUE);
    }
  else if (nm->nmpls)
    {
      if (nm->nmpls->lsr_id != lcl_lsr->s_addr)
        res = nsm_mpls_l2_circuit_del (nm, vc, NSM_TRUE);
    }
  else
    return NSM_API_SET_ERROR;
  if (res < 0)
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_mpls_outbd_tnl_peer_lsr (struct nsm_master *nm,
                                         u_int32_t pw_ix, char **ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;
  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);
  if (!vc)
    return NSM_API_GET_ERROR;
  if (((vc->owner) && (vc->owner != MPLS_RSVP)) || ((vc->ftn) &&
                                   (vc->ftn->owner.owner != MPLS_RSVP)))
    **ret = 0;
  else 
    pal_inet_ntop (AF_INET,
                   &(vc->address.u.prefix4),
                   *ret,
                   NSM_PW_MAX_STRING_LENGTH);

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_outbd_tnl_peer_lsr (struct nsm_master *nm,
                                              u_int32_t *pw_ix, char **ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;
  vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, vc_id);
  if (!vc)
    return NSM_API_GET_ERROR;
  if (((vc->owner) && (vc->owner != MPLS_RSVP)) || ((vc->ftn) &&
                                   (vc->ftn->owner.owner != MPLS_RSVP)))
    **ret = 0;
  else
    pal_inet_ntop (AF_INET,
                   &(vc->address.u.prefix4),
                   *ret,
                   NSM_PW_MAX_STRING_LENGTH);

  *pw_ix = vc->vc_snmp_index;
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_set_pw_mpls_outbd_tnl_peer_lsr (struct nsm_master *nm,
                                         u_int32_t pw_ix,
                                         struct pal_in4_addr *peer_lsr)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);
  if (!vc)
    return NSM_API_SET_ERROR;
  if (((vc->owner) && (vc->owner != MPLS_RSVP)) || ((vc->ftn) &&
                                   (vc->ftn->owner.owner != MPLS_RSVP)))
    return NSM_API_SET_ERROR;

  if (vc)
    {
      if (vc->address.u.prefix4.s_addr != peer_lsr->s_addr)
        return NSM_API_SET_ERROR;
    }

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_mpls_outbd_if_ix (struct nsm_master *nm,
                                  u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;
  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (!vc)
    return NSM_API_GET_ERROR;
  if (!(vc->vc_fib))
    return NSM_API_GET_ERROR;
  if (((vc->ftn) && ((vc->ftn->owner.owner == MPLS_OTHER) ||
      (vc->ftn->owner.owner == MPLS_SNMP))) ||
      ((vc->owner) && ((vc->owner == MPLS_OTHER) ||
                       (vc->owner == MPLS_SNMP))))
    {
#ifdef HAVE_GMPLS
      *ret = nsm_gmpls_get_ifindex_from_gifindex (nm, vc->vc_fib->nw_if_ix);
#else /* HAVE_GMPLS */
      *ret = vc->vc_fib->nw_if_ix;
#endif /* HAVE_GMPLS */
    }
  else
    *ret = 0;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_outbd_if_ix (struct nsm_master *nm,
                                       u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;
  while (NSM_TRUE)
    {
      vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, vc_id);

      if (!vc)
        return NSM_API_GET_ERROR;

      if (!(vc->vc_fib))
        {
          vc_id = vc->vc_snmp_index;
          continue;
        }

      if (((vc->ftn) && ((vc->ftn->owner.owner == MPLS_OTHER) ||
           (vc->ftn->owner.owner == MPLS_SNMP))) ||
           ((vc->owner) && ((vc->owner == MPLS_OTHER) ||
                            (vc->owner == MPLS_SNMP))))
        {
#ifdef HAVE_GMPLS
          *ret = nsm_gmpls_get_ifindex_from_gifindex (nm, vc->vc_fib->nw_if_ix);
#else /* HAVE_GMPLS */
          *ret = vc->vc_fib->nw_if_ix;
#endif /* HAVE_GMPLS */
          *pw_ix = vc->vc_snmp_index;
          return NSM_API_GET_SUCCESS;
        }
      else 
       {
         *ret = 0;
         *pw_ix = vc->vc_snmp_index;
         return NSM_API_GET_SUCCESS;
       }
    }
}

u_int32_t
nsm_mpls_set_pw_mpls_outbd_if_ix (struct nsm_master *nm,
                                  u_int32_t pw_ix, u_int32_t if_ix)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if (!vc)
    return NSM_API_SET_ERROR;
  if (!(vc->vc_fib))
    return NSM_API_SET_ERROR;
  if (((vc->ftn) && ((vc->ftn->owner.owner == MPLS_OTHER) ||
      (vc->ftn->owner.owner == MPLS_SNMP))) ||
      ((vc->owner) && ((vc->owner == MPLS_OTHER) ||
                       (vc->owner == MPLS_SNMP))))
    {
#ifdef HAVE_GMPLS
      if (if_ix != nsm_gmpls_get_ifindex_from_gifindex (nm, vc->vc_fib->nw_if_ix))
#else /* HAVE_GMPLS */
      if (if_ix != vc->vc_fib->nw_if_ix)
#endif /* HAVE_GMPLS */
        return NSM_API_SET_ERROR;
    }
  else
    return NSM_API_SET_ERROR;

  return NSM_API_SET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_mpls_outbd_tnl_typ_inuse (struct nsm_master *nm,
                                          u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;

  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);

  if ((vc) && !(vc->owner) && (vc->ftn))
    {
      if ((vc->ftn->owner.owner == MPLS_OTHER) ||
          (vc->ftn->owner.owner == MPLS_SNMP))
        *ret = TNL_TYPE_PWONLY;
      else if (vc->ftn->owner.owner == MPLS_LDP)
        *ret = TNL_TYPE_MPLSNONTE;
      else if (vc->ftn->owner.owner == MPLS_RSVP)
        *ret = TNL_TYPE_MPLSTE;
      else
        *ret = TNL_TYPE_NOT_KNOWN;
    }

  else if ((vc) && (vc->owner))
    {
      if ((vc->owner == MPLS_OTHER) || (vc->owner == MPLS_SNMP))
        *ret = TNL_TYPE_PWONLY;
      else if (vc->owner == MPLS_LDP)
        *ret = TNL_TYPE_MPLSNONTE;
      else if (vc->owner == MPLS_RSVP)
        *ret = TNL_TYPE_MPLSTE;
      else
        *ret= TNL_TYPE_NOT_KNOWN;
    }

  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_outbd_tnl_typ_inuse (struct nsm_master *nm,
                                               u_int32_t *pw_ix,
                                               u_int32_t *ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;
      vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, vc_id);
      if (!vc)
        return NSM_API_GET_ERROR;

      if (!(vc->owner) && (vc->ftn))
        {
         if ((vc->ftn->owner.owner == MPLS_OTHER) ||
             (vc->ftn->owner.owner == MPLS_SNMP))
           *ret = TNL_TYPE_PWONLY;
         else if (vc->ftn->owner.owner == MPLS_LDP)
           *ret = TNL_TYPE_MPLSNONTE;
         else if (vc->ftn->owner.owner == MPLS_RSVP)
           *ret = TNL_TYPE_MPLSTE;
         else
           return NSM_API_GET_ERROR;
        }

      else
        {
          if ((vc->owner == MPLS_OTHER) || (vc->owner == MPLS_SNMP))
            *ret = TNL_TYPE_PWONLY;
          else if (vc->owner == MPLS_LDP)
            *ret = TNL_TYPE_MPLSNONTE;
          else if (vc->owner == MPLS_RSVP)
            *ret = TNL_TYPE_MPLSTE;
          else
	        return NSM_API_GET_ERROR;
       }
    *pw_ix = vc->vc_snmp_index;
    return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_mpls_inbd_xc_ix (struct nsm_master *nm,
                                 u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc = NULL;
  vc_id = pw_ix;
  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);
  if (vc)
    *ret = vc->xc_ix;
  else
    return NSM_API_GET_ERROR;

  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_inbd_xc_ix (struct nsm_master *nm,
                                      u_int32_t *pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id = *pw_ix;
  struct nsm_mpls_circuit *vc = NULL;
  vc = nsm_pw_snmp_get_vc_next_index (nm,  NSM_MPLS->vc_table, vc_id);
  if (vc)
    *ret = vc->xc_ix;
  else
    return NSM_API_GET_ERROR;

  *pw_ix = vc->vc_snmp_index;
  return NSM_API_GET_SUCCESS;
}

u_int32_t
nsm_mpls_get_pw_mpls_non_te_map_pw_ix (struct nsm_master *nm,
                                       u_int32_t dn, u_int32_t xc_ix,
                                       u_int32_t if_ix,
                                       u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc;
  int flag = 0;
  vc_id = pw_ix;
  vc = NULL;

   vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);
  if (!vc)
    return NSM_API_GET_ERROR;

  if ((vc->owner == MPLS_RSVP) || ((vc->ftn) &&
      (vc->ftn->owner.owner == MPLS_RSVP)))
    return NSM_API_GET_ERROR;
  
  if (vc->dn || vc->dn_in)
    {
      if ((vc->dn != dn) && (vc->dn_in != dn))
        flag = 1;
    }
  if ((vc->ftn) && (vc->ftn->xc))
    {
      if (((vc->owner == MPLS_SNMP) || (vc->owner == MPLS_OTHER) ||
          (vc->ftn->owner.owner == MPLS_SNMP) ||
          (vc->ftn->owner.owner == MPLS_OTHER)) && (0 != xc_ix))
        flag = 1;
      else
        {
          if ((vc->dn == dn) && (vc->ftn->xc->key.xc_ix != xc_ix))
            flag = 1;
        }
    }
  else
    {
      if ((vc->dn_in == dn) && (vc->xc_ix != xc_ix))
        flag = 1;
      else
        {
          if (0 != xc_ix)
            flag = 1;
        }
    }
  if (vc->vc_fib)
    {
      if ((vc->owner == MPLS_LDP) ||
         ((vc->ftn) && (vc->ftn->owner.owner == MPLS_LDP))) 
        {
          if (0 != if_ix)
            flag = 1;
        }
      else
        {
#ifdef HAVE_GMPLS
          if (nsm_gmpls_get_ifindex_from_gifindex (nm, vc->vc_fib->nw_if_ix) != if_ix)
#else /* HAVE_GMPLS */
          if (vc->vc_fib->nw_if_ix != if_ix)
#endif /* HAVE_GMPLS */
            flag = 1;
        }
    }
  else
    {
      if (0 != if_ix)
        flag = 1;
    }
   if (flag == 0)
    {
      *ret = vc_id;
      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_non_te_map_pw_ix (struct nsm_master *nm,
                                   u_int32_t *dn, u_int32_t *xc_ix,
                                   u_int32_t *if_ix, u_int32_t *pw_ix,
                                   u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_l2_circuit_lookup_next_for_non_te_map (nm, dn, xc_ix, pw_ix);

  if (!vc)
     return NSM_API_GET_ERROR;
  if ((vc->owner == MPLS_RSVP) || ((vc->ftn) &&
      (vc->ftn->owner.owner == MPLS_RSVP)))
    return NSM_API_GET_ERROR;
  if (*dn == FROMPSN)
    {
      *ret = vc->vc_snmp_index;
      *dn = vc->dn_in;
      *xc_ix = vc->xc_ix;
      *if_ix = 0;
      *pw_ix = vc->vc_snmp_index;
      return NSM_API_GET_SUCCESS;
    }
  else if ((vc->ftn) && (vc->ftn->xc) && (vc->vc_fib))
    {
      *ret = vc->vc_snmp_index;
      *dn = vc->dn;
      if ((vc->owner == MPLS_SNMP) || (vc->owner == MPLS_OTHER) ||
          (vc->ftn->owner.owner == MPLS_SNMP) ||
          (vc->ftn->owner.owner == MPLS_OTHER))
        *xc_ix = 0;
      else
        *xc_ix = vc->ftn->xc->key.xc_ix;
      if ((vc->owner == MPLS_LDP) ||
          ((vc->ftn) && (vc->ftn->owner.owner == MPLS_LDP)))
        *if_ix = 0;
      else
#ifdef HAVE_GMPLS
        *if_ix = nsm_gmpls_get_ifindex_from_gifindex(nm, vc->vc_fib->nw_if_ix);
#else /* HAVE_GMPLS */
        *if_ix = vc->vc_fib->nw_if_ix;
#endif /* HAVE_GMPLS */
        *pw_ix = vc->vc_snmp_index;
        return NSM_API_GET_SUCCESS;
    }
  else
    {
      *dn = 0;
      *xc_ix = 0;
      *if_ix = 0;
      *pw_ix = 0;
      return NSM_API_GET_ERROR;
    }
  return NSM_API_GET_ERROR;
}

u_int32_t
nsm_mpls_get_pw_mpls_te_map_pw_ix (struct nsm_master *nm,
                                   u_int32_t tunl_ix, u_int32_t tnl_inst,
                                   struct pal_in4_addr peer_lsr,
                                   struct pal_in4_addr lcl_lsr,
                                   u_int32_t pw_ix, u_int32_t *ret)
{
  u_int32_t vc_id;
  struct nsm_mpls_circuit *vc;
  int flag = 0;
  vc_id = pw_ix;
  vc = NULL;


  vc = nsm_mpls_l2_circuit_snmp_lookup_by_index (nm, vc_id);
  if (!vc)
    return NSM_API_GET_ERROR;

  if (((vc->owner) && (vc->owner != MPLS_RSVP)) || ((vc->ftn) &&
                                   (vc->ftn->owner.owner != MPLS_RSVP)))
    return NSM_API_GET_ERROR;
  if (vc->ftn)
    {
      if (vc->ftn->tun_id != tunl_ix)
        flag = 1;
      if (vc->ftn->protected_lsp_id != tnl_inst)
        flag = 1;
    }
  else
    {
      if ((tunl_ix != 0) || (tnl_inst != 0))
        flag = 1;
    }

  if (vc->address.u.prefix4.s_addr != peer_lsr.s_addr)
        flag = 1;

  if (vc->vc_fib)
    {
      if (vc->vc_fib->lsr_id != lcl_lsr.s_addr)
        flag = 1;
    }
  else
    {
      if (lcl_lsr.s_addr != 0)
        flag = 1;
    }

  if (flag == 0)
    {
      *ret = vc_id;
      return NSM_API_GET_SUCCESS;
    }
  else
    return NSM_API_GET_ERROR;
}

u_int32_t
nsm_mpls_get_next_pw_mpls_te_map_pw_ix (struct nsm_master *nm,
                                        u_int32_t *tunl_ix,
                                        u_int32_t *tnl_inst,
                                        struct pal_in4_addr *peer_lsr,
                                        struct pal_in4_addr *lcl_lsr,
                                        u_int32_t *pw_ix, u_int32_t *ret)
{
  struct nsm_mpls_circuit *vc = NULL;

  vc = nsm_mpls_l2_circuit_lookup_next_for_te_map (nm, tunl_ix,
                                                   tnl_inst, peer_lsr,
                                                   lcl_lsr, pw_ix);
  if (!vc)
     return NSM_API_GET_ERROR;

  if ((vc) && (vc->vc_fib) && (vc->ftn))
    {
      *ret = vc->vc_snmp_index;
      *tunl_ix = vc->ftn->tun_id;
      *tnl_inst = vc->ftn->protected_lsp_id;
      peer_lsr->s_addr = vc->address.u.prefix4.s_addr;
      lcl_lsr->s_addr = vc->vc_fib->lsr_id;
      *pw_ix = vc->vc_snmp_index;
      return NSM_API_GET_SUCCESS;
    }
  return NSM_API_GET_ERROR;
}

/** @brief Function to map the vc->addr.family
    to the peer_addr_type of the vc as their
    enums differ by stds.

    @param u_int32_t peer_addr_type     - Peer Addr Type

    @return AF_INET 
            AF_INET6 
            NSM_API_GET_ERROR
*/
int
nsm_mpls_get_vc_addr_fam (u_int32_t peer_addr_type)
{
  switch (peer_addr_type)
   {
     case PW_PEER_ADDR_TYPE_IPV4:
        return AF_INET;
     case PW_PEER_ADDR_TYPE_IPV6:
       return AF_INET6;   
   }   
  return NSM_API_GET_ERROR;
}
#endif /* HAVE_SNMP */
#endif /* HAVE_MPLS_VC */
