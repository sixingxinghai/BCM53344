/* Copyright (C) 2005-2006  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "hal_incl.h"

#ifdef HAVE_SNMP
#include <asn1.h>

#include "if.h"
#include "log.h"
#include "l2_snmp.h"
#include "prefix.h"
#include "table.h"
#include "avl_tree.h"
#include "smux.h"
#include "l2lib.h"
#include "linklist.h"
#include "pal_math.h"
#include "nsmd.h"
#include "nsm_snmp.h"
#include "nsm_L2_snmp.h"
#include "nsm_bridge.h"
#include "table.h"
#include "nsm_vlan.h"
#include "nsm_interface.h"
#include "nsmd.h"
#include "nsm_p_mib.h"
#include "nsm_q_mib.h"
#include "nsm_snmp_common.h"

int
write_dot1dPortGarpTime (int action,
                         u_char * var_val,
                         u_char var_val_type,
                         size_t var_val_len,
                         u_char * statP, oid * name, size_t name_len,
                         struct variable *v,int type);


struct nsm_bridge *
nsm_snmp_get_first_bridge ()
{
  struct apn_vr *vr = apn_vr_get_privileged (snmpg);
  struct nsm_master *master = NULL;
  struct nsm_bridge_master *bridge_master = NULL;

    if (! vr)
    return NULL;

  master = vr->proto;

  if (! master)
    return NULL;

  bridge_master = master->bridge;

  if (! bridge_master)
    return NULL;

  return (bridge_master->bridge_list);
}


u_int8_t
nsm_snmp_bridge_port_get_port_id (u_int32_t ifindex)
{
  struct nsm_if *zif;
  struct nsm_bridge *br;
  struct interface *ifp;
  struct avl_node *avl_port;
  struct nsm_bridge_port *br_port;

  br = nsm_snmp_get_first_bridge ();

  if (br == NULL)
    return NSM_SNMP_INVALID_PORT_ID;

  for (avl_port = avl_top (br->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;

      if (ifp->ifindex == ifindex)
        return br_port->port_id;

    }

  return NSM_SNMP_INVALID_PORT_ID;
}


s_int32_t
nsm_snmp_bridge_port_get_ifindex (u_int8_t port_id, u_int32_t *ifindex)
{
  struct nsm_if *zif;
  struct nsm_bridge *br;
  struct interface *ifp;
  struct avl_node *avl_port;
  struct nsm_bridge_port *br_port;

  br = nsm_snmp_get_first_bridge ();

  if (br == NULL
      || ifindex == NULL)
    return RESULT_ERROR;

  for (avl_port = avl_top (br->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;

      if (br_port->port_id == port_id)
        {
          *ifindex = ifp->ifindex;
          return RESULT_OK;
        }
    }

  return RESULT_ERROR;
}

/*
 * given a bridge, instance and exact/inexact request, returns a pointer to
 * the interface structure or NULL if none found.
 */
struct interface *
nsm_snmp_bridge_port_lookup (struct nsm_bridge *br, int port_instance,
                             int exact)
{
  struct nsm_if *zif;
  struct interface *ifp;
  struct avl_node *avl_port;
  struct interface *best = NULL; /* assume none found */
  struct nsm_bridge_port *br_port;

  for (avl_port = avl_top (br->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;
        
      if (exact)
        {
          if (ifp->ifindex == port_instance)
            {
              /* GET request and instance match */
              best = ifp;
              break;
            }
        }
      else                      /* GETNEXT request */
        {
          if (port_instance < ifp->ifindex)
            {
              /* save if this is a better candidate and
               *   continue search
               */
              if (best == NULL)
                best = ifp;
              else if (ifp->ifindex < best->ifindex)
                best = ifp;
            }
        }
    }

  return best;
}

/*
 * given a bridge, instance and exact/inexact request, returns a pointer to
 * the interface structure or NULL if none found.
 */
struct interface *
nsm_snmp_bridge_port_lookup_by_port_id (struct nsm_bridge *br, int port_id,
                                        int exact,
                                        struct nsm_bridge_port **ret_br_port)
{
  struct nsm_if *zif;
  struct interface *ifp;
  struct avl_node *avl_port;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge_port *best = NULL;

  if (ret_br_port != NULL)
    *ret_br_port = NULL;

  for (avl_port = avl_top (br->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                             AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;
        
      if (exact)
        {
          if (br_port->port_id == port_id)
            {
              /* GET request and instance match */
              best = br_port;
              break;
            }
        }
      else                      /* GETNEXT request */
        {
          if (port_id < br_port->port_id)
            {
              /* save if this is a better candidate and
               *   continue search
               */
              if (best == NULL)
                best = br_port;
              else if (br_port->port_id < best->port_id)
                best = br_port;
            }
        }
    }

  if (ret_br_port != NULL)
    *ret_br_port = best;

  return ((best != NULL && best->nsmif != NULL && best->nsmif->ifp != NULL)
                        ? best->nsmif->ifp : NULL);
}

/*
 * Given a bridge, instance and exact/inexact request, returns a pointer to
 * the interface structure or NULL if none found.
 */
/* Same port may contain multiple classification rules */
struct interface *
nsm_snmp_bridge_rule_port_lookup_by_port_id (struct nsm_bridge *br, int port_id,
                                             int exact,
                                             struct nsm_bridge_port 
                                             **ret_br_port)
{
  struct nsm_if *zif;
  struct interface *ifp;
  struct avl_node *avl_port;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge_port *best = NULL;

  if (ret_br_port != NULL)
    *ret_br_port = NULL;

  for (avl_port = avl_top (br->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      if ((br_port = (struct nsm_bridge_port *)
                      AVL_NODE_INFO (avl_port)) == NULL)
        continue;

      if ((zif = br_port->nsmif) == NULL)
        continue;

      if ((ifp = zif->ifp) == NULL)
        continue;
        
      if (exact)
        {
          if (br_port->port_id == port_id)
            {
              /* GET request and instance match */
              best = br_port;
              break;
            }
        }
      else                      /* GETNEXT request */
        {
          if (port_id <= br_port->port_id)
            {
              /* save if this is a better candidate and
               *   continue search
               */
              if (best == NULL)
                best = br_port;
              else if (br_port->port_id <= best->port_id)
                best = br_port;
            }
        }
    }

  if (ret_br_port != NULL)
    *ret_br_port = best;

  return ((best != NULL && best->nsmif != NULL && best->nsmif->ifp != NULL)
                        ? best->nsmif->ifp : NULL);
}

#endif /* HAVE_SNMP */
