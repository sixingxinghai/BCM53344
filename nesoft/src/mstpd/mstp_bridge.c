/*
   Copyright (C) 2003  All Rights Reserved.

   LAYER 2 BRIDGE

   This module declares the interface to the Bridge
   functions.
*/

#include "pal.h"
#include "lib.h"
#include "nsm_message.h"
#include "nsm_client.h"
#include "table.h"
#include "l2_timer.h"
#include "l2lib.h"
#include "bitmap.h"

#include "mstp_config.h"
#include "mstp_types.h"
#include "mstp_bpdu.h"
#include "mstp_port.h"
#include "mstp_cist_proto.h"
#include "mstp_msti_proto.h"
#include "mstp_transmit.h"
#include "mstp_bridge.h"
#include "mstpd.h"
#include "mstp_rlist.h"
#include "mstp_api.h"

#include "nsm_message.h"
#include "nsm_client.h"
#include "auth_md5.h"

#ifdef HAVE_SNMP
#include "mstp_snmp.h"
#endif /* HAVE_SNMP */

#ifdef HAVE_HA
#include "mstp_cal.h"
#endif /* HAVE_HA */

#ifdef HAVE_OPENSSL_HMAC_H
/* #include <openssl/hmac.h> */
#endif

static struct mstp_bridge *bridge_list = NULL;

/*The list of bridge configuration maintained to store configuration */
struct mstp_bridge_config_list *mstp_bridge_config_list = NULL;

#define CONFIG_DIGEST_KEY_LEN 16

static u_char cd_signature_key[CONFIG_DIGEST_KEY_LEN] =
  { 0x13,0xAC,0x06,0xA6,0x2E,0x47,0xFD,0x51,
    0xF9,0x5D,0x2B,0xA2,0x43,0xCD,0x03,0x46 };

/* Forward declarations. */
u_int32_t mstp_nsm_get_port_path_cost (struct lib_globals *zg, const u_int32_t ifindex);

void
mstp_cist_handle_port_forward (struct mstp_port *port);

#if (defined HAVE_I_BEB)
static struct mstp_port * mstp_add_cist_port ( struct mstp_bridge *br,
                                               char *ifname,
                                               u_int16_t svid,
                                               uint32_t isid,int is_explicit,
                                               u_int8_t spanning_tree_disable);
#else
static struct mstp_port * mstp_add_cist_port ( struct mstp_bridge *br,
                                               char *ifname,
                                               u_int16_t svid,
                                               int is_explicit,
                                               u_int8_t spanning_tree_disable);

#endif

void mstp_gen_cfg_digest (struct mstp_bridge *br);
static int mstp_add_msti_port (struct mstp_bridge_instance *br_inst,
                               struct mstp_port *port, u_int32_t ifindex,
                               int instance, int vid);
static int mstp_delete_msti_port (struct mstp_instance_port *inst_port, bool_t);

static void
mstp_cist_handle_br_forward (struct mstp_port *port);

u_int8_t *
mstp_get_bridge_str (struct mstp_bridge *br)
{
  static u_int8_t bridge_str [MSTP_BRIDGE_STR_LEN + NSM_BRIDGE_NAMSIZ + 1];

  pal_mem_set (bridge_str, 0, sizeof (bridge_str));

  if (! br->is_default)
    pal_snprintf (bridge_str, sizeof (bridge_str), "bridge %s ", br->name);

  return bridge_str;

}

u_int8_t *
mstp_get_bridge_str_span (struct mstp_bridge *br)
{
  static u_int8_t bridge_str [MSTP_BRIDGE_STR_LEN + NSM_BRIDGE_NAMSIZ + 1];

  pal_mem_set (bridge_str, 0, sizeof (bridge_str));

  if (! br->is_default)
    pal_snprintf (bridge_str, sizeof (bridge_str), "bridge %s", br->name);
  else
    pal_snprintf (bridge_str, sizeof (bridge_str), "spanning-tree");

  return bridge_str;

}

u_int8_t *
mstp_get_bridge_group_str_span (struct mstp_bridge *br)
{
  static u_int8_t bridge_str [MSTP_BRIDGE_STR_LEN + NSM_BRIDGE_NAMSIZ + 1];

  pal_mem_set (bridge_str, 0, sizeof (bridge_str));

  if (! br->is_default)
    pal_snprintf (bridge_str, sizeof (bridge_str), "bridge-group %s", br->name);
  else
    pal_snprintf (bridge_str, sizeof (bridge_str), "spanning-tree");

  return bridge_str;

}

#if defined (HAVE_L2GP) || defined (HAVE_RPVST_PLUS)
inline void
mstp_make_bridgeid (struct bridge_id * br_id,
                    short new_priority,short system_id)
#else
static inline void
mstp_make_bridgeid (struct bridge_id * br_id,
                    short new_priority,short system_id)
#endif /* HAVE_L2GP || HAVE_RPVST_PLUS */

{
  /* Settable priority comprises only of four most sigificant bits */
  br_id->prio[0] = ((new_priority >> 8) & 0xf0) | ((system_id >> 8) & 0x0f);
  br_id->prio[1] = system_id & 0xff ;
}

void
mstp_link_bridge (struct mstp_bridge *new)
{
  new->next = bridge_list;
  if (new->next != NULL)
    new->next->pprev = &new->next;

  bridge_list = new;
  new->pprev = &bridge_list;
}

pal_inline void
mstp_unlink_bridge (struct mstp_bridge *curr)
{
  *(curr->pprev) = curr->next;
  if (curr->next != NULL)
    curr->next->pprev = curr->pprev;

  curr->next = NULL;
  curr->pprev = NULL;
}

u_int8_t *
mstp_get_ce_br_port_str (struct mstp_port *port)
{
  static u_int8_t port_str [MSTP_MAX_CLI_LEN];

  pal_mem_set (port_str, 0, sizeof (port_str));

  if (port->port_type == MSTP_VLAN_PORT_MODE_CE)
    pal_snprintf (port_str, sizeof (port_str), "customer-spanning-tree "
                                               "customer-edge");
  else
    pal_snprintf (port_str, sizeof (port_str), "customer-spanning-tree "
                                               "provider-edge svlan %d", port->svid);

  return port_str;
}

void *
mstp_get_first_bridge (void)
{
  struct mstp_bridge * br = bridge_list;
  return br;
}

void *
mstp_get_default_bridge (void)
{
  struct mstp_bridge * br = bridge_list;

  for (br = bridge_list; br; br = br->next)
    {
      if (br->is_default)
        return br;
    }

  return NULL;
}

void *
mstp_find_bridge (char * name)
{
  struct mstp_bridge * br = bridge_list;
  for (br = bridge_list; br; br = br->next)
    {
      if (pal_strncmp (br->name, name, L2_BRIDGE_NAME_LEN) == 0)
        return br;
    }
  return NULL;
}

void
mstp_link_port_instance (struct mstp_port *port ,
                         struct mstp_instance_port *inst_port)
{
  struct mstp_instance_port *curr;
  struct mstp_instance_port *prev = NULL;
  pal_assert (port);

  curr = port->instance_list;

  if (!curr)
    {
      port->instance_list = inst_port;
      return;
    }

  while ((curr) &&
         (curr->instance_id < inst_port->instance_id))
    {
      prev = curr;
      curr = curr->next_inst;
    }

  if (prev)
    prev->next_inst = inst_port;
  else
    port->instance_list = inst_port;

  inst_port->next_inst = curr;
}

void
mstp_unlink_port_instance (struct mstp_instance_port *inst_port)
{
  struct mstp_port *port = inst_port->cst_port;
  struct mstp_instance_port *temp_port = port->instance_list;

  if (temp_port == inst_port)
    {
      port->instance_list = inst_port->next_inst;
      inst_port->next_inst = NULL;
      return;
    }

  while (temp_port)
    {
      if (temp_port->next_inst == inst_port)
        {
          temp_port->next_inst = inst_port->next_inst;
          inst_port->next_inst = NULL;
          break;
        }
      temp_port = temp_port->next_inst;
    }
}



void
mstp_link_msti_port (struct mstp_bridge_instance *br_inst,
                     struct mstp_instance_port *inst_port)
{
  /* Add to the common port list first
   * if the port is not already present there*/
  pal_assert (inst_port);
  pal_assert (br_inst);
  inst_port->next = br_inst->port_list;
  if (inst_port->next != NULL)
    inst_port->next->pprev = &inst_port->next;

  br_inst->port_list = inst_port;
  inst_port->pprev = &br_inst->port_list;
}



void
mstp_link_cist_port (struct mstp_bridge *br, struct mstp_port *port)
{
  port->next = br->port_list;
  if (port->next != NULL)
    port->next->pprev = &port->next;

  br->port_list = port;
  port->pprev = &br->port_list;
}


void
mstp_unlink_cist_port (struct mstp_port *curr)
{
  if (!curr)
    return;

  *(curr->pprev) = curr->next;
  if (curr->next != NULL)
    curr->next->pprev = curr->pprev;

  curr->next = NULL;
  curr->pprev = NULL;
}


void
mstp_unlink_msti_port (struct mstp_instance_port *curr)
{
  if (!curr)
    return;

  *(curr->pprev) = curr->next;
  if (curr->next != NULL)
    curr->next->pprev = curr->pprev;

  curr->next = NULL;
  curr->pprev = NULL;
}




void *
mstp_find_cist_port (void *mstp_bridge, u_int32_t ifindex )
{
  struct mstp_port *curr;
  struct mstp_bridge *br = mstp_bridge;

  /* return here itslef if either the bridge is NULL
   * or the instance does not exist */
  if (!br)
    return NULL;

  for (curr = br->port_list; curr; curr = curr->next)
    {
      if (curr->ifindex == ifindex)
        return curr;
    }
  return NULL;
}

void *
mstp_find_ce_br_port (void *mstp_bridge, u_int16_t svid)
{
  struct mstp_port *curr;
  struct mstp_bridge *br = mstp_bridge;

  /* return here itslef if either the bridge is NULL.
   * or the instance does not exist */
  if (!br)
    return NULL;

  for (curr = br->port_list; curr; curr = curr->next)
    {
      if (curr->svid == svid)
        return curr;
    }

  return NULL;
}

struct mstp_instance_port *
mstp_find_msti_port (struct mstp_bridge_instance *br_inst,
                     u_int32_t ifindex )
{
  struct mstp_instance_port *curr;

  if (!br_inst)
    return NULL;

  for (curr = br_inst->port_list; curr; curr = curr->next)
    {
      if (curr->ifindex == ifindex)
        return curr;
    }

  return NULL;
}


struct mstp_instance_port *
mstp_find_msti_port_in_inst_list (struct mstp_port *port,
                                  int instance_id )
{
  struct mstp_instance_port *curr = port ? port->instance_list : NULL;
  if (!curr)
    {
      return NULL;
    }

  while (curr)
    {
      if (curr->instance_id == instance_id)
        break;

      curr = curr->next_inst;
    }

  return curr;
}

#ifdef HAVE_RPVST_PLUS
struct mstp_instance_port *
mstp_find_msti_port_in_vlan_list (struct mstp_port *port,
                                  struct mstp_instance_bpdu *inst_bpdu)
{
  struct mstp_instance_port *curr = NULL;
  int vlan_id = 0;
  struct mstp_bridge * br;

   if (port == NULL)
         return NULL;
   
  if (!inst_bpdu)
    return NULL;

  vlan_id = inst_bpdu->vlan_id;

  br = port->br;

  curr = port->instance_list;
  
  if (!curr)
      return NULL;

  if (!br)
    return NULL;

  /* Assign instance id from vlan id */

  while (curr)
    {
      if (curr->vlan_id == vlan_id)
        break;

      curr = curr->next_inst;
    }

  return curr;
}
#endif /* HAVE_RPVST_PLUS */

int
mstp_enable_bridge (struct mstp_bridge * br)
{
  struct mstp_port * port;
  struct mstp_instance_port *inst_port;

  if (br == NULL)
    return RESULT_ERROR ;

  if (br->mstp_enabled)
    {
      /* bridge is already enabled do nothing */
      return RESULT_OK;
    }

  /* First put all the ports to Discarding */
  BRIDGE_CIST_PORTLIST_LOOP (port, br)
    {

      if (port->spanning_tree_disable == PAL_TRUE
          && port->cist_state == STATE_FORWARDING)
       continue;

      mstp_cist_set_port_state (port, STATE_DISCARDING);
#ifdef HAVE_HA
      mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
      inst_port = port->instance_list;
      while (inst_port)
        {
          mstp_msti_set_port_state (inst_port, STATE_DISCARDING);
#ifdef HAVE_HA
          mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
          inst_port = inst_port->next_inst;
        }
     }

  br->mstp_enabled = PAL_TRUE;
  br->mstp_brforward = PAL_FALSE;

  BRIDGE_CIST_PORTLIST_LOOP(port,br)
    {
      if (mstp_is_port_up (port->ifindex))
        {
          if (port->spanning_tree_disable)
            mstp_cist_handle_port_forward (port);
          else
            mstp_cist_enable_port (port);
        }
    }
#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  return RESULT_OK;
}


int
mstp_disable_bridge (struct mstp_bridge * br, bool_t bridge_forward)
{
  struct mstp_port *port;

  if (br == NULL)
    return RESULT_ERROR;

  if (!br->mstp_enabled)
    {
      /* bridge is already disabled do nothing */
      return RESULT_OK;
    }

  BRIDGE_CIST_PORTLIST_LOOP(port,br)
    {
      if (port->cist_role != ROLE_DISABLED)
        {
          mstp_cist_disable_port (port);
        }
    }

  if (bridge_forward == PAL_TRUE)
    {
      BRIDGE_CIST_PORTLIST_LOOP(port,br)
        {
          mstp_cist_set_port_state_forward (port);
        }
    }

#ifdef HAVE_HA
  if (l2_is_timer_running(br->topology_change_timer))
    mstp_cal_delete_mstp_br_tc_timer (br);
  if (l2_is_timer_running(br->tcn_timer))
    mstp_cal_delete_mstp_tcn_timer (br);
#endif /* HAVE_HA */

  l2_stop_timer (&br->tcn_timer);
  l2_stop_timer (&br->topology_change_timer);

  br->mstp_enabled = PAL_FALSE;
  br->mstp_brforward = bridge_forward;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  return RESULT_OK;
}

static struct mstp_vlan *
_mstp_vlan_create ()
{
  struct mstp_vlan *v;

  v = XCALLOC (MTYPE_MSTP_VLAN, sizeof (struct mstp_vlan));

#ifdef HAVE_HA
  mstp_cal_create_mstp_vlan (v);
#endif /* HAVE_HA */

  return v;
}

static void
_mstp_vlan_free (struct mstp_vlan *v)
{
  if (v)
    {
#ifdef HAVE_HA
      mstp_cal_delete_mstp_vlan (v);
#endif /* HAVE_HA */
      XFREE (MTYPE_MSTP_VLAN, v);
    }
}

int
_mstp_bridge_create_vlan_table (struct mstp_bridge *br)
{
  br->vlan_table = route_table_init ();
  if (! br->vlan_table)
    return -1;
  return 0;
}

int
mstp_bridge_free_vlan_table (struct mstp_bridge *br)
{
  struct route_node *tmp_node = NULL;
  struct route_node *node = NULL;
  struct mstp_vlan *vlan = NULL;

  if (! br || ! br->vlan_table)
    return 0;

  node = br->vlan_table->top;

  while (node)
    {
      if (node->l_left)
        {
          node = node->l_left;
          continue;
        }

      if (node->l_right)
        {
          node = node->l_right;
          continue;
        }

      tmp_node = node;
      node = node->parent;

      if (node != NULL)
        {
          if (node->l_left == tmp_node)
            node->l_left = NULL;
          else
            node->l_right = NULL;

          vlan = tmp_node->info;
          _mstp_vlan_free (vlan);
          vlan = NULL;

          route_node_free (tmp_node);
        }
      else
        {
          vlan = tmp_node->info;
          _mstp_vlan_free (vlan);
          vlan = NULL;

          route_node_free (tmp_node);
          break;
        }

    }

  XFREE (MTYPE_ROUTE_TABLE, br->vlan_table);
  br->vlan_table = NULL;

  return 0;
}

struct mstp_vlan *
mstp_bridge_vlan_lookup (char *bridge_name, u_int16_t vid)
{
  struct mstp_bridge *br;
  struct mstp_prefix_vlan p;
  struct route_node *rn;

  br = mstp_find_bridge (bridge_name);
  if (! br)
    return NULL;

  if (! br->vlan_table)
    return NULL;

  MSTP_PREFIX_VLAN_SET (&p, vid);
  rn = route_node_lookup (br->vlan_table, (struct prefix *) &p);
  if (rn)
    {
      route_unlock_node (rn);

      return (struct mstp_vlan *) rn->info;
    }

  return NULL;
}

int
mstp_bridge_vlan_add (char *bridge_name, u_int16_t vid, char *vlan_name, int flags)
{
  struct mstp_bridge *br;
  struct mstp_vlan *v;
  struct mstp_prefix_vlan p;
  struct route_node *rn;
  int ret;

  br = mstp_find_bridge (bridge_name);
  if (! br)
    return RESULT_ERROR;

  if (! br->vlan_table)
    {
      ret = _mstp_bridge_create_vlan_table (br);
      if (ret < 0)
        return RESULT_ERROR;
    }

  MSTP_PREFIX_VLAN_SET (&p, vid);
  rn = route_node_lookup (br->vlan_table, (struct prefix *) &p);
  if (rn)
    {
      v = rn->info;

      if (vlan_name)
        pal_strcpy (v->name, vlan_name);

      v->flags = flags;

      /* Check if config exists for this vlan. */
      if ((CHECK_FLAG (flags, MSTP_VLAN_SUSPEND)
           || CHECK_FLAG (flags, MSTP_VLAN_ACTIVE))
          && CHECK_FLAG (flags, MSTP_VLAN_INSTANCE_CONFIGURED)
          && (v->instance != -1))
        {
          mstp_api_add_instance (bridge_name, v->instance, vid, 0);
        }

      /* Unset configured flag. */
      UNSET_FLAG (v->flags, MSTP_VLAN_INSTANCE_CONFIGURED);
      v->instance = -1;
    }
  else
    {
      v = _mstp_vlan_create ();
      if (! v)
        return RESULT_ERROR;

      v->vid = vid;
      if (vlan_name)
        pal_strcpy (v->name, vlan_name);
      v->flags = flags;
      v->instance = -1;
      pal_strcpy (v->bridge_name, br->name);

      rn = route_node_get (br->vlan_table, (struct prefix *) &p);
      if (! rn)
        {
          _mstp_vlan_free (v);
          return RESULT_ERROR;
        }

      MSTP_VLAN_INFO_SET (rn, v);
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_vlan (v);
#endif /* HAVE_HA */

  route_unlock_node (rn);
  return 0;
}

int
mstp_bridge_vlan_add2 (char *bridge_name, u_int16_t vid, s_int32_t instance,
                       int flags)
{
  struct mstp_vlan *v;
  struct mstp_prefix_vlan p;
  struct route_node *rn;
  int ret;
  struct mstp_bridge *br;

  br = mstp_find_bridge (bridge_name);
  if (! br)
    return RESULT_ERROR;

  if (! br->vlan_table)
    {
      ret = _mstp_bridge_create_vlan_table (br);
      if (ret < 0)
        return RESULT_ERROR;
    }

  MSTP_PREFIX_VLAN_SET (&p, vid);
  rn = route_node_lookup (br->vlan_table, (struct prefix *) &p);
  if (rn)
    {
      v = rn->info;

      v->vid = vid;
      v->instance = instance;
    }
  else
    {
      v = _mstp_vlan_create ();
      if (! v)
        return RESULT_ERROR;

      v->vid = vid;
      v->flags = flags;
      v->instance = instance;

      rn = route_node_get (br->vlan_table, (struct prefix *) &p);
      if (! rn)
        {
          _mstp_vlan_free (v);
          return RESULT_ERROR;
        }

      MSTP_VLAN_INFO_SET (rn, v);
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_vlan (v);
#endif /* HAVE_HA */

  route_unlock_node (rn);
  return 0;
}

int
mstp_bridge_vlan_instance_delete (char *bridge_name, int instance)
{
  struct route_node *node;
  struct mstp_vlan *vlan;
  struct mstp_bridge *br;

  br = mstp_find_bridge (bridge_name);
  if (! br)
    return RESULT_ERROR;

  if (! br->vlan_table)
    return RESULT_ERROR;

  for (node = route_top (br->vlan_table); node; node = route_next (node))
    {
      vlan = node->info;
      if (vlan)
        {
          if (CHECK_FLAG (vlan->flags, MSTP_VLAN_INSTANCE_CONFIGURED)
              && (vlan->instance == instance))
            {
              if (CHECK_FLAG (vlan->flags, MSTP_VLAN_ACTIVE))
                {
                  UNSET_FLAG (vlan->flags, MSTP_VLAN_INSTANCE_CONFIGURED);
                  vlan->instance = 0;
                }
              else
                {
                  _mstp_vlan_free (vlan);
                  node->info = NULL;
                  route_unlock_node (node);
                }
            }
        }
    }

  return 0;
}

int
mstp_bridge_vlan_delete (char *bridge_name, u_int16_t vid)
{
  struct route_node *rn;
  struct mstp_vlan *v;
  struct mstp_prefix_vlan p;
  struct mstp_bridge *br;

  br = mstp_find_bridge (bridge_name);
  if (! br)
    return RESULT_ERROR;

  if (! br->vlan_table)
    return RESULT_ERROR;

  MSTP_PREFIX_VLAN_SET (&p, vid);
  rn = route_node_lookup (br->vlan_table, (struct prefix *) &p);
  if (rn)
    {
      route_unlock_node (rn);

      if (! rn->info)
        return RESULT_ERROR;

      v = rn->info;

      MSTP_VLAN_INFO_UNSET (rn);

      _mstp_vlan_free (v);
    }

  return 0;
}

int
mstp_bridge_vlan_validate_vid (char *str, struct cli *cli)
{
  u_int16_t vid;

  CLI_GET_INTEGER_RANGE ("VLAN id", vid, str, 1, 4094);
  return CLI_SUCCESS;
}

#if (defined HAVE_I_BEB)
int
mstp_bridge_add (char * name,
                 u_int8_t type, bool_t ce_bridge, bool_t cn_bridge,
                 bool_t is_default, u_char topology_type,
                 struct mstp_port *ce_port)
#else
int
mstp_bridge_add (char * name,
                 u_int8_t type, bool_t ce_bridge,
                 bool_t is_default, u_char topology_type,
                 struct mstp_port *ce_port)
#endif
{
  bool_t is_vlan_aware = PAL_FALSE;
  struct mstp_bridge *br ;
  int len;
  int ret;

  if ((ce_bridge && !ce_port) || !name)
    return RESULT_ERROR;

  /* Check if the bridge already exists */
  br = (struct mstp_bridge *)mstp_find_bridge (name);

#ifdef HAVE_HA
  if (br != NULL)
    {
      br->ha_stale = PAL_FALSE;
      return RESULT_OK;
    }
#endif /* HAVE_HA */

  if (ce_bridge && ce_port->ce_br)
    return RESULT_OK;
#if (defined HAVE_I_BEB)
  else if(cn_bridge && ce_port->cn_br)
      {
        return RESULT_OK;
      }
#endif
  else if (!ce_bridge &&
           (br != NULL))
    return RESULT_OK;

  /* set up the MSTP related values */
  br = XCALLOC(MTYPE_MSTP_BRIDGE, sizeof (struct mstp_bridge)) ;

  /* Create VLAN tree. */
  if ((type == NSM_BRIDGE_TYPE_MSTP)
     || (type == NSM_BRIDGE_TYPE_PROVIDER_RSTP)
     || (type == NSM_BRIDGE_TYPE_PROVIDER_MSTP)
     || (type == NSM_BRIDGE_TYPE_BACKBONE_RSTP)
     || (type == NSM_BRIDGE_TYPE_BACKBONE_MSTP)
     || (type == NSM_BRIDGE_TYPE_CE)
#if (defined HAVE_I_BEB)
     || (type == NSM_BRIDGE_TYPE_CNP)
#endif
     || (type == NSM_BRIDGE_TYPE_STP_VLANAWARE)
     || (type == NSM_BRIDGE_TYPE_RSTP_VLANAWARE)
     || (type == NSM_BRIDGE_TYPE_RPVST_PLUS))
   is_vlan_aware = PAL_TRUE;

#ifdef HAVE_VLAN
  if (is_vlan_aware)
    {
      ret = _mstp_bridge_create_vlan_table (br);
      if (ret < 0)
        {
          XFREE (MTYPE_MSTP_BRIDGE, br);
          return RESULT_ERROR;
        }
    }
#endif /* HAVE_VLAN */

  br->port_index = 0;

  len = pal_strlen (name);

  if (len > L2_BRIDGE_NAME_LEN)
    {
      len = L2_BRIDGE_NAME_LEN;
      zlog_err (mstpm, "Name size is greater than max allowed %d",
                L2_BRIDGE_NAME_LEN);
    }

  pal_strncpy (br->name, name, len);

  br->type = type;

#if (defined HAVE_I_BEB)
  if(ce_bridge)
#endif
  br->ce_bridge = ce_bridge;

#if (defined HAVE_I_BEB)
  if (cn_bridge)
    br->cn_bridge = cn_bridge;
#endif

#if defined  HAVE_G8031 || defined HAVE_G8032
  MSTP_INSTANCE_BMP_INIT(br->mstpProtectionBmp);
#endif /* HAVE_G8031 ||  defined HAVE_G8032 */

  if (br->type == MSTP_BRIDGE_TYPE_STP
      || br->type == MSTP_BRIDGE_TYPE_STP_VLANAWARE)
    br->force_version = BR_VERSION_STP;
  else if (br->type == MSTP_BRIDGE_TYPE_RSTP
           || br->type == MSTP_BRIDGE_TYPE_RSTP_VLANAWARE
           || br->type == MSTP_BRIDGE_TYPE_RPVST_PLUS
           || br->type == MSTP_BRIDGE_TYPE_PROVIDER_RSTP)
    br->force_version = BR_VERSION_RSTP;
  else
    br->force_version = BR_VERSION_MSTP;

#if (defined HAVE_I_BEB)
  if (ce_bridge || cn_bridge)
#else
  if (ce_bridge)
#endif
    {
      br->cist_bridge_priority = CE_BRIDGE_DEFAULT_PRIORITY;
      mstp_make_bridgeid (&(br->cist_bridge_id),
                          (short)br->cist_bridge_priority, 0xfff);
    }
  else
    {
      br->cist_bridge_priority = BRIDGE_DEFAULT_PRIORITY;
      mstp_make_bridgeid (&(br->cist_bridge_id),
                          (short)br->cist_bridge_priority, 0);
    }

#ifdef HAVE_RPVST_PLUS
  if (br->type == MSTP_BRIDGE_TYPE_RPVST_PLUS)
    br->max_instances = RPVST_MAX_INSTANCES;
  else
#endif /* HAVE_RPVST_PLUS */
    br->max_instances = MST_MAX_INSTANCES;

  br->mstp_enabled = PAL_TRUE;
  br->bridge_enabled = PAL_TRUE;
  br->learning_enabled = PAL_TRUE;
  br->is_default = is_default;
  br->topology_type = topology_type;
  pal_mem_cpy (&br->cist_designated_root,
               &br->cist_bridge_id, MSTP_BRIDGE_ID_LEN);
  pal_mem_cpy (&br->cist_reg_root,
               &br->cist_bridge_id, MSTP_BRIDGE_ID_LEN);
  pal_mem_cpy (&br->cist_rcvd_reg_root,
               &br->cist_bridge_id, MSTP_BRIDGE_ID_LEN);
  pal_mem_cpy (&br->cist_designated_bridge,
               &br->cist_bridge_id, MSTP_BRIDGE_ID_LEN);
  br->external_root_path_cost = 0;
  br->cist_root_port_id = 0;
  br->cist_root_port_ifindex = 0;
  br->root_port = NULL;
  br->ageing_time = BRIDGE_TIMER_DEFAULT_AGEING_TIME;
  br->bridge_max_age = br->cist_max_age = BRIDGE_TIMER_DEFAULT_MAX_AGE *
    L2_TIMER_SCALE_FACT;
    br->max_age_count++;
  br->bridge_hello_time = br->cist_hello_time =
    BRIDGE_TIMER_DEFAULT_HELLO_TIME * L2_TIMER_SCALE_FACT;
  br->bridge_forward_delay = br->cist_forward_delay =
    BRIDGE_TIMER_DEFAULT_FWD_DELAY * L2_TIMER_SCALE_FACT;
  br->topology_change = PAL_FALSE;
  br->topology_change_detected = PAL_FALSE;
  br->bpduguard = PAL_FALSE;
  br->errdisable_timeout_interval =
    MSTP_BRIDGE_ERRDISABLE_DEFAULT_TIMEOUT_INTERVAL;
  br->errdisable_timeout_enable = PAL_FALSE;
  br->bpdu_filter = PAL_FALSE;

  br->bridge_max_hops = MST_DEFAULT_MAXHOPS;
  br->transmit_hold_count = MSTP_TX_HOLD_COUNT;
  br->num_ports = 0;

  /* Set default path cost method */
  if (br->type == MSTP_BRIDGE_TYPE_STP
      || br->type == MSTP_BRIDGE_TYPE_STP_VLANAWARE)
      br->path_cost_method = MSTP_PATHCOST_SHORT;
  else
     br->path_cost_method = MSTP_PATHCOST_LONG;
 
  /* Statistics */
  br->tc_flag = PAL_FALSE;
  br->tc_initiator = 0;
  pal_mem_set (&br->tc_last_rcvd_from, 0, ETHER_ADDR_LEN);

  /* set default config information */

  pal_mem_cpy(&(br->config.cfg_name),
              "Default", 8);

  br->config.cfg_format_id = 0; /* Format specified by IEEE 802.1s */

  br->config.cfg_revision_lvl = 0 ; /* Default revision level*/

#ifdef HAVE_HA
  mstp_cal_create_mstp_bridge (br);
#endif /* HAVE_HA */

  mstp_gen_cfg_digest (br);

  br->is_vlan_aware = is_vlan_aware;

#ifdef HAVE_VLAN
  br->vlan_list = NULL;
  if (is_vlan_aware)
    mstp_rlist_add (&br->vlan_list, MST_DEFAULT_VLAN, 0);
#endif /* HAVE_VLAN */

  if (ce_bridge)
    ce_port->ce_br = br;

#if (defined HAVE_I_BEB)
  if (cn_bridge)
    {
      br->port_list = NULL;
      ce_port->cn_br = br;
    }
#endif

  mstp_link_bridge (br);

  mstp_enable_bridge (br);

#ifdef HAVE_SNMP
  /* send trap for new root */
  mstp_snmp_new_root (br);
#endif /* HAVE_SNMP */

  return RESULT_OK;
}

int
mstp_bridge_delete (char * name, struct mstp_port *ce_port)
{
  int index;
  struct mstp_port *port;
  struct mstp_bridge * br = NULL;

  if (!name && !ce_port)
    return RESULT_ERROR;

  if (name)
    br = mstp_find_bridge (name);
  else
    br = ce_port->ce_br;

#ifdef HAVE_I_BEB
  if (!br)
    br = ce_port->cn_br;
#endif

  /* log it */
  if (br)
    {
      mstp_disable_bridge (br, PAL_FALSE);

      /* delete each of the instances */
      for (index = (br->max_instances- 1) ; index > 0 ;index--)
        {
          /* instance is not created */
          if (!br->instance_list[index])
            continue;

          mstp_delete_instance (br,index);
        }

      /* Remove the ports that have not been deleted by instances */
      while ((port = br->port_list))
        {
          mstp_delete_cist_port (port, PAL_TRUE, PAL_TRUE, PAL_TRUE);
        }

      mstp_rlist_free (&br->vlan_list);

      /* Free VLAN table. */
      mstp_bridge_free_vlan_table (br);

      /*  Delete the bridge from the global bridge list. */
      if(ce_port)
        {
          if (ce_port->ce_br)
          ce_port->ce_br = NULL;
#ifdef HAVE_I_BEB
          if (ce_port->cn_br)
          ce_port->cn_br = NULL;
#endif
        }
      mstp_unlink_bridge (br);

#ifdef HAVE_HA
       /* Delete bridge from CAL*/
      mstp_cal_delete_mstp_bridge (br);
#endif /* HAVE_HA */

      pal_mem_set (br, 0, sizeof (struct mstp_bridge));
      XFREE (MTYPE_MSTP_BRIDGE, br);
    }

  return RESULT_OK;
}

int
mstp_add_port (struct mstp_bridge *br, char *ifname,
               u_int16_t svid, int instance, u_int8_t spanning_tree_disable)
{
  u_int32_t ifindex;
  struct interface *ifp;
  struct mstp_port *port;
  struct mstp_instance_port *inst_port = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);

  ifp = if_lookup_by_name (&vr->ifm, ifname);

  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  if (br->ce_bridge)
    port = mstp_find_ce_br_port (br, svid);
  else
    port = mstp_find_cist_port (br, ifindex);

  if (!port)
    {
      if (br->ce_bridge)
        {
#if (defined HAVE_I_BEB)
        port = mstp_add_cist_port (br, ifname, svid,0,
                                   (instance == MST_INSTANCE_IST),
                                   spanning_tree_disable);

#else
        port = mstp_add_cist_port (br, ifname, svid,
                                   (instance == MST_INSTANCE_IST),
                                   spanning_tree_disable);
#endif
        }
      else
        {
#if (defined HAVE_I_BEB)
        port = mstp_add_cist_port (br, ifname, MSTP_VLAN_NULL_VID,0,
                                   (instance == MST_INSTANCE_IST),
                                   spanning_tree_disable);

#else
        port = mstp_add_cist_port (br, ifname, MSTP_VLAN_NULL_VID,
                                   (instance == MST_INSTANCE_IST),
                                   spanning_tree_disable);
#endif
       }


      if (!port)
        {
          return RESULT_ERROR;
        }
    }
  else
    {
#ifdef HAVE_HA
      /* Reset the stale flag as port is created during deposit*/
      if (port->ha_stale)
        {
          port->ha_stale = PAL_FALSE;
          return RESULT_OK;
        }
#endif /* HAVE_HA */

      if (instance == MST_INSTANCE_IST)
        {
          /* Return an error if the port is already added explicity */
          if (port->type == TYPE_EXPLICIT)
            return RESULT_ERROR;

          /* if the prior add type was implicit */
          port->type = TYPE_EXPLICIT;
          port->ref_count++;

#ifdef HAVE_HA
          mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

          return RESULT_OK;
        }
      inst_port = mstp_find_msti_port (br->instance_list[instance], ifindex);
      if (inst_port)
        {
          if (PAL_FALSE == inst_port->pathcost_configured)
            mstp_msti_set_port_path_cost
              (inst_port, mstp_nsm_get_port_path_cost (mstpm, ifindex));
          return MSTP_ERR_INSTANCE_ALREADY_BOUND;
        }

      port->ref_count++;
    }

  if (instance != MST_INSTANCE_IST)
    mstp_add_msti_port (br->instance_list[instance],
                        port, ifindex, instance, svid);
  if (!br->ce_bridge)
    {
      ifp->port = (void *)mstp_find_cist_port (br, ifindex);
    }

  /* store the bridge name for convienence */
  pal_strncpy (ifp->bridge_name, br->name, INTERFACE_NAMSIZ + 1);

  return RESULT_OK;
}

static void
mstp_cist_propagate_bridge_id (struct mstp_bridge * br)
{
  register struct mstp_port * port;
  struct mstp_port * selected_port = NULL;
  u_int32_t low_port = MAX_IFINDEX;

  if (br == NULL)
    return;

  /* When the last port is removed ,root id and designated root of the bridge
     is set to initial values. */
  if (br->port_list == NULL)
    {
      pal_mem_set (br->cist_bridge_id.addr, 0, ETHER_ADDR_LEN);
      pal_mem_cpy (br->cist_designated_root.addr, br->cist_bridge_id.addr,
                   ETHER_ADDR_LEN);
      pal_mem_cpy (br->cist_reg_root.addr ,br->cist_bridge_id.addr,
                   ETHER_ADDR_LEN);
      br->low_port = low_port;
      return;
    }
  for (port = br->port_list; port; port = port->next)
    {
      if (port->ifindex < low_port)
        {
          low_port = port->ifindex;
          selected_port = port;
        }
    }

  if (br->low_port != low_port)
    {
      u_char orig_addr[6];
      u_char * addr = selected_port->dev_addr;
      bool_t is_root = mstp_bridge_is_cist_root (br);

      br->low_port = low_port;

      pal_mem_cpy (orig_addr, br->cist_bridge_id.addr, ETHER_ADDR_LEN);
      if (addr)
        pal_mem_cpy (br->cist_bridge_id.addr, addr, ETHER_ADDR_LEN);

      for (port = br->port_list; port != 0; port = port->next)
        {
          if (!pal_mem_cmp (port->cist_designated_bridge.addr,
                            orig_addr, ETHER_ADDR_LEN))
            pal_mem_cpy (port->cist_designated_bridge.addr, addr,
                         ETHER_ADDR_LEN);

          if (!pal_mem_cmp (port->cist_root.addr, orig_addr, ETHER_ADDR_LEN))
            pal_mem_cpy (port->cist_root.addr, addr, ETHER_ADDR_LEN);

          if (!pal_mem_cmp (port->cist_reg_root.addr, orig_addr,
                            ETHER_ADDR_LEN))
            pal_mem_cpy (port->cist_reg_root.addr, addr, ETHER_ADDR_LEN);

          if (!pal_mem_cmp (port->cist_rcvd_reg_root.addr, orig_addr,
                            ETHER_ADDR_LEN))
            pal_mem_cpy (port->cist_rcvd_reg_root.addr, addr, ETHER_ADDR_LEN);

        }

      mstp_cist_port_role_selection (br);

      /* STP becoming root */
      if (IS_BRIDGE_STP (br) && ! is_root && mstp_bridge_is_cist_root (br))
        {
          stp_topology_change_detection (br);

#ifdef HAVE_HA
          mstp_cal_delete_mstp_tcn_timer (br);
#endif /* HAVE_HA */
          l2_stop_timer (&br->tcn_timer);

#ifdef HAVE_SNMP
          /* STP becoming root */
          mstp_snmp_new_root (br);
#endif /* HAVE_SNMP */

          stp_generate_bpdu_on_bridge (br);
        }

    }
}

int
mstp_add_ce_br_port (struct mstp_bridge *br, char *ifname,
                     u_int16_t svid)
{
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;
  struct mstp_port *port;

  ifp = if_lookup_by_name (&vr->ifm, ifname);

  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  port = mstp_find_ce_br_port (br, svid);
  if (!port)
    {
#if (defined HAVE_I_BEB)
      port = mstp_add_cist_port (br, ifname, svid,0, PAL_TRUE, PAL_FALSE);
#else
      port = mstp_add_cist_port (br, ifname, svid, PAL_TRUE, PAL_FALSE);
#endif

      if (!port)
        {
          return RESULT_ERROR;
        }
    }

  return RESULT_OK;
}

int
mstp_delete_ce_br_port (struct mstp_bridge *br, char *ifname,
                        u_int16_t svid)
{
  struct mstp_port *port;
  port = mstp_find_ce_br_port (br, svid);

  if (port)
    mstp_delete_cist_port (port, PAL_TRUE, PAL_TRUE, PAL_TRUE);

  return RESULT_OK;
}

#if (defined HAVE_I_BEB)

void *
mstp_find_cn_br_port (void *mstp_bridge, u_int16_t isid)
{
  struct mstp_port *curr;
  struct mstp_bridge *br = mstp_bridge;

  /* return here itslef if either the bridge is NULL.
   * or the instance does not exist */
  if (!br)
    return NULL;

  for (curr = br->port_list; curr; curr = curr->next)
     if (curr->isid == isid)
       return curr;

  return NULL;
}

/*
 *  same function can be used to find svid to isid mapping and
 *  isid to bvid mapping.  since both information contained in
 *  struct mstp_port, simply returning struct mstp_port *
 */
struct mstp_port *
mstp_get_cn_port_by_svid( struct mstp_bridge * cn_br, uint16_t svid )
{
   struct mstp_port * cn_port;
   struct nsm_msg_pbb_isid_to_pip * vip_node, *tmp_vip_node;
   struct listnode * vip_bundle_node, *tmp;

   cn_port = NULL;
   vip_node = tmp_vip_node = NULL;
   vip_bundle_node = tmp = NULL;

   if (!cn_br)
      return NULL;

   for (cn_port = cn_br->port_list ; cn_port ; cn_port = cn_port->next)
      {
         if (listcount (cn_port->svid_list) > 0)
           LIST_LOOP ( cn_port->svid_list, tmp_vip_node, vip_bundle_node)
           {
                 if ( svid >= tmp_vip_node->svid_low &&
                      svid <= tmp_vip_node->svid_high )
                   return cn_port;
           }
      }
    return NULL;
}

uint32_t
mstp_add_cn_br_port (struct mstp_bridge *br ,
                     char *ifname,
                     uint16_t isid,
                     struct nsm_msg_pbb_isid_to_pip * msg)
{
   struct mstp_port *port;
   struct mstp_port * vip_port;
   struct nsm_msg_pbb_isid_to_pip * vip_node, *tmp_vip_node;
   struct listnode * vip_bundle_node;

   port = vip_port = NULL;
   vip_node = tmp_vip_node = NULL;
   vip_bundle_node = NULL;

   port = vip_port = NULL;

   if (!br)
     return RESULT_ERROR;

   port = mstp_find_cn_br_port (br, isid);

   if (!port)
     {

         if (!msg)
           vip_port = mstp_add_cist_port (br, ifname,0,0, PAL_TRUE, PAL_FALSE);
         else
           vip_port = mstp_add_cist_port (br, ifname,0,msg->isid, \
                                          PAL_TRUE, PAL_FALSE);

         if (!vip_port)
            return RESULT_ERROR;
         if (!vip_port->svid_list)
            vip_port->svid_list = list_new();

         vip_node = XCALLOC(MTYPE_MSTP_BRIDGE_PBB_VIP_NODE,
                  sizeof(struct nsm_msg_pbb_isid_to_pip));

         if (!vip_node)
              return RESULT_ERROR;

         if (msg)
           {
               pal_mem_cpy(vip_node->bridge_name,msg->bridge_name, \
                                               NSM_BRIDGE_NAMSIZ);
               vip_node->cnp_ifindex = msg->cnp_ifindex;
               vip_node->pip_ifindex = msg->pip_ifindex;
               vip_node->svid_low    = msg->svid_low;
               vip_node->svid_high   = msg->svid_high;
               vip_node->isid        = msg->isid;
               vip_node->dispatch_status = msg->dispatch_status;

               listnode_add(vip_port->svid_list,(void *)vip_node);
               vip_port->isid = msg->isid;
               vip_port->pip_port = msg->pip_ifindex;
           }
      }
    else
      {
         /*
          * VIP port exists, need to check existence of svid
          */

         if (listcount (port->svid_list) > 0)
           LIST_LOOP ( port->svid_list, tmp_vip_node, vip_bundle_node)
           {
            if ( tmp_vip_node->isid == msg->isid &&
                 tmp_vip_node->cnp_ifindex == msg->cnp_ifindex &&
                 tmp_vip_node->pip_ifindex == msg->pip_ifindex &&
                 tmp_vip_node->svid_low == msg->svid_low &&
                 tmp_vip_node->svid_high == msg->svid_high )
              {
                  /* entry already exisits */
                   return RESULT_ERROR;
              }
           }

         vip_node = XCALLOC(MTYPE_MSTP_BRIDGE_PBB_VIP_NODE,
                     sizeof(struct nsm_msg_pbb_isid_to_pip));

         if (!vip_node)
            return RESULT_ERROR;

         pal_mem_cpy(vip_node->bridge_name,msg->bridge_name, \
                                    NSM_BRIDGE_NAMSIZ);
         vip_node->cnp_ifindex = msg->cnp_ifindex;
         vip_node->pip_ifindex = msg->pip_ifindex;
         vip_node->svid_low    = msg->svid_low;
         vip_node->svid_high   = msg->svid_high;
         vip_node->isid        = msg->isid;
         vip_node->dispatch_status = msg->dispatch_status;

         listnode_add(port->svid_list,(void *)vip_node);

     }

     return RESULT_OK;
}

uint32_t
mstp_del_cn_br_vip_port( struct mstp_bridge *br ,
                         char *ifname,
                         uint32_t isid,
                         struct nsm_msg_pbb_isid_to_pip * msg )
{
   struct mstp_port *port;
   struct nsm_msg_pbb_isid_to_pip *tmp_vip_node;
   struct listnode * vip_bundle_node;
   struct listnode * vip_next;

   tmp_vip_node = NULL;
   vip_bundle_node = NULL;
   vip_next = NULL;
   port = NULL;

   if (!br)
     return RESULT_ERROR;

   port = mstp_find_cn_br_port (br, isid);

   if (!port)
      return RESULT_ERROR;

   
   if (listcount (port->svid_list) > 0)
        LIST_LOOP_DEL ( port->svid_list, tmp_vip_node, vip_bundle_node, vip_next)
         {
             if ( tmp_vip_node->isid == msg->isid &&
                  tmp_vip_node->cnp_ifindex == msg->cnp_ifindex &&
                  tmp_vip_node->pip_ifindex == msg->pip_ifindex)
               {
                 /* if whole VLAN range is given delete all the nodes with 
                  * the matching interfaces */
                 if (msg->svid_low == MSTP_VLAN_MIN && 
                     msg->svid_high == MSTP_VLAN_MAX) 
                   {
                     list_delete_node(port->svid_list,vip_bundle_node);
                   }
                 else if(tmp_vip_node->svid_low == msg->svid_low &&
                         tmp_vip_node->svid_high == msg->svid_high )
                   {
                      list_delete_node(port->svid_list,vip_bundle_node);
                      return RESULT_OK;
                   }
               }
         }

   return RESULT_ERROR;
}

uint32_t
mstp_del_cn_br_port( struct mstp_bridge *br ,
                     uint32_t isid )
{

   struct mstp_port *port, *vip_port;

   if (!br)
     return RESULT_ERROR;

   port = mstp_find_cn_br_port (br, isid);

   if (!port)
      return RESULT_ERROR;

   /*
    * deleting svid_list inside a VIP port.
    */
   list_delete(port->svid_list);
   /*
    * delete the VIP port itself..
    */
    vip_port = port;
    mstp_unlink_cist_port(port);

    /*
     * free memory that is allocated to VIP port.
     */
    XFREE(MTYPE_MSTP_BRIDGE_PORT,port);

    return RESULT_OK;
}

/*
 * This function removes the existing configuration on cn_br port
 * and deletes the bridge.
 */
uint32_t
mstp_clean_cn_br_port (struct mstp_port *port)
{

  struct mstp_port *cn_port = NULL;
  struct mstp_bridge *cn_br = port->cn_br;

/*
 * clearing all the vip ports present as cn_br->port_list
 * before deleting the cn_br bridge
 */
  for (cn_port = cn_br->port_list; cn_port; cn_port = cn_port->next)
    {
      if(cn_port->svid_list)
        list_delete (cn_port->svid_list);
    }

  mstp_bridge_delete (NULL, port);
  return RESULT_OK;
}

uint32_t
mstp_cn_br_update_vip_list_status( struct mstp_bridge *br,
                                   uint32_t isid )
{
   struct mstp_port *port;
   struct nsm_msg_pbb_isid_to_pip *tmp_vip_node;
   struct listnode * vip_bundle_node;


   tmp_vip_node = NULL;
   vip_bundle_node = NULL;
   port = NULL;

   if (!br)
       return RESULT_ERROR;

   port = mstp_find_cn_br_port (br, isid);

   if (!port)
      return RESULT_ERROR;
   if (listcount (port->svid_list) > 0)
        LIST_LOOP ( port->svid_list, tmp_vip_node, vip_bundle_node)
             tmp_vip_node->dispatch_status = 1;

   return RESULT_OK;
}

uint32_t
mstp_updt_bvid_to_isid_mapping( struct mstp_bridge *br,
                                struct nsm_msg_pbb_isid_to_bvid *msg )
{
   struct mstp_port *port;
   struct nsm_msg_pbb_isid_to_bvid *tmp_vip_node;
   struct listnode * vip_bundle_node;


   tmp_vip_node = NULL;
   vip_bundle_node = NULL;
   port = NULL;

   if (!br)
       return RESULT_ERROR;

   port = mstp_find_cn_br_port (br, msg->isid);

   if (!port)
      return RESULT_ERROR;

   if (listcount (port->svid_list) > 0)
        LIST_LOOP ( port->svid_list, tmp_vip_node, vip_bundle_node)
             tmp_vip_node->bvid = msg->bvid;

   return RESULT_OK;

}

#endif

static void
mstp_msti_propagate_bridge_id (struct mstp_bridge_instance * br_inst)
{
  register struct mstp_instance_port * inst_port;
  struct mstp_instance_port * selected_port = NULL;
  s_int32_t low_port = MAX_IFINDEX;

  /* When the last port is removed ,root id and designated root of the bridge
     is set to initial values. */
  if (br_inst->port_list == NULL)
    {
      pal_mem_set (br_inst->msti_bridge_id.addr, 0, ETHER_ADDR_LEN);
      pal_mem_cpy (br_inst->msti_designated_root.addr,
                   br_inst->msti_bridge_id.addr, ETHER_ADDR_LEN);
      br_inst->low_port = low_port;
      return;
    }

  for (inst_port = br_inst->port_list; inst_port; inst_port = inst_port->next)
    {
      if (inst_port->ifindex < low_port)
        {
          low_port = inst_port->ifindex;
          selected_port = inst_port;
        }
    }

  if (br_inst->low_port != low_port)
    {
      u_char orig_addr[6];
      u_char * addr = NULL;

      if (selected_port)
        addr  = selected_port->cst_port->dev_addr;

      if (addr == NULL)
        return;
      
      br_inst->low_port = low_port;

      pal_mem_cpy (orig_addr, br_inst->msti_bridge_id.addr, ETHER_ADDR_LEN);
      pal_mem_cpy (br_inst->msti_bridge_id.addr, addr, ETHER_ADDR_LEN);

      for (inst_port = br_inst->port_list; inst_port;
           inst_port = inst_port->next)
        {
          if (!pal_mem_cmp (inst_port->designated_bridge.addr,
                            orig_addr, ETHER_ADDR_LEN))
            pal_mem_cpy (inst_port->designated_bridge.addr, addr,
                         ETHER_ADDR_LEN);

          if (!pal_mem_cmp (inst_port->msti_root.addr,
                            orig_addr, ETHER_ADDR_LEN))
            pal_mem_cpy (inst_port->msti_root.addr, addr, ETHER_ADDR_LEN);
        }

      mstp_msti_port_role_selection (br_inst);
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge_instance (br_inst);
#endif /* HAVE_HA */

  mstp_tx_bridge (br_inst->bridge);

}

void
mstp_cist_handle_port_forward (struct mstp_port *port)
{
  struct mstp_bridge *br;
  struct mstp_instance_port *inst_port;

  if (!port)
    return;

  br = port->br;

  if (!br)
    return;

  mstp_cist_set_port_role (port, ROLE_DISABLED);
  port->cist_info_type = INFO_TYPE_DISABLED;
  mstp_cist_set_port_state_forward (port);

  inst_port = port->instance_list;

  while (inst_port)
    {
       mstp_msti_set_port_role (inst_port, ROLE_DISABLED);
       inst_port->info_type = INFO_TYPE_DISABLED;
       mstp_msti_set_port_state (inst_port, STATE_FORWARDING);
#ifdef HAVE_HA
       mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
       inst_port = inst_port->next_inst;
     }
}

#if (defined HAVE_I_BEB)
struct mstp_port *
mstp_add_cist_port ( struct mstp_bridge *br, char  *ifname,
                     u_int16_t svid,uint32_t isid, int is_explicit,
                     u_int8_t spanning_tree_disable)

#else
struct mstp_port *
mstp_add_cist_port ( struct mstp_bridge *br, char  *ifname,
                     u_int16_t svid, int is_explicit,
                     u_int8_t spanning_tree_disable)
#endif
{
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifname);

  if (! ifp)
    return NULL;

  ifindex = ifp->ifindex;

  /* Allocate the port */
  port = XCALLOC(MTYPE_MSTP_BRIDGE_PORT, sizeof (struct mstp_port)) ;

  if (port == NULL )
    {
      /* Memory cannot be allocated */
      zlog_err (mstpm, "Cannot allocate memory for port (%u)", ifindex);
      return NULL;
    }

  /* populate fields */
  port->br = br;
  port->svid = svid;
  port->admin_edge = BRIDGE_PORT_DEFAULT_EDGE;
  port->admin_p2p_mac = BRIDGE_PORT_DEFAULT_P2P;
  port->oper_p2p_mac = BRIDGE_PORT_DEFAULT_P2P;
  port->ifindex = ifindex;
  pal_strncpy(port->name, ifname, INTERFACE_NAMSIZ);

  /* populate the default values for L2 security parameters */
  port->admin_bpduguard = MSTP_PORT_PORTFAST_BPDUGUARD_DEFAULT;
  port->oper_bpduguard = br->bpduguard;
  port->admin_bpdufilter = MSTP_PORT_PORTFAST_BPDUFILTER_DEFAULT;
  port->oper_bpdufilter = br->bpdu_filter;
  port->admin_rootguard = PAL_FALSE;
  port->oper_rootguard = PAL_FALSE;
  port->type = is_explicit ? TYPE_EXPLICIT : TYPE_IMPLICIT ;
  port->spanning_tree_disable = spanning_tree_disable;

  if (BRIDGE_TYPE_CVLAN_COMPONENT (br))
    {
      if (svid)
        port->port_type = MSTP_VLAN_PORT_MODE_PE;
      else
        port->port_type = MSTP_VLAN_PORT_MODE_CE;
    }
#if (defined HAVE_I_BEB)
  else if (BRIDGE_TYPE_SVLAN_COMPONENT (br))
    {
      if (isid)
        port->port_type = MSTP_VLAN_PORT_MODE_VIP;
    }
  else if (BRIDGE_TYPE_BVLAN_COMPONENT (br))
    {
      if (isid)
        port->port_type = MSTP_VLAN_PORT_MODE_CBP;
    }
#endif
  else if (BRIDGE_TYPE_PROVIDER (br))
    port->port_type = MSTP_VLAN_PORT_MODE_PN;
  else
    port->port_type = MSTP_VLAN_PORT_MODE_ACCESS;


  if (port->type == MSTP_VLAN_PORT_MODE_PE)
    port->cist_path_cost = 128;
  else
    {
       if (port->br->path_cost_method == MSTP_PATHCOST_SHORT)
         {
           port->cist_path_cost =
               mstp_nsm_get_default_port_path_cost_short_method (ifp->bandwidth);
         }
       else
         port->cist_path_cost =
               mstp_nsm_get_default_port_path_cost (ifp->bandwidth);
     }

  if (BRIDGE_TYPE_CVLAN_COMPONENT (br))
    port->cist_priority = CE_BRIDGE_DEFAULT_PORT_PRIORITY;
#if (defined HAVE_I_BEB)
  else if (BRIDGE_TYPE_SVLAN_COMPONENT(br))
    port->cist_priority=VIP_BRIDGE_DEFAULT_PORT_PRIORITY;
#endif
  else
    port->cist_priority = BRIDGE_DEFAULT_PORT_PRIORITY;

  port->ce_br = NULL;
  port->ref_count = 1;
  port->hello_time = BRIDGE_TIMER_DEFAULT_HELLO_TIME * L2_TIMER_SCALE_FACT;
  port->restricted_role = PAL_FALSE;
  port->restricted_tcn = PAL_FALSE;

  /* Port initially always in  forwarding state. */
 if (port->spanning_tree_disable == PAL_FALSE) 
   port->cist_state = STATE_FORWARDING;

  br->num_ports++;

#if (defined HAVE_I_BEB)
  port->svid_list = NULL;
  port->isid = isid;
  port->bvid = 0;
#endif

#ifdef HAVE_L2GP
  port->isL2gp = PAL_FALSE;
  port->enableBPDUrx = PAL_TRUE;
  port->enableBPDUtx = PAL_TRUE;
  pal_mem_set(&port->psuedoRootId,0,sizeof(port->psuedoRootId));
#endif /* HAVE_L2GP */

  /* Get port address. */
  ifp = if_lookup_by_index (&vr->ifm, ifindex);
  if (! ifp)
    {
      /* Releasing Memory Allocated to Port. */
      XFREE (MTYPE_MSTP_BRIDGE_PORT, port);

      /* Decrementing num of ports in bridge. */
      br->num_ports--;

      /* Cannot get address of the port. */
      return NULL;
    }
  pal_mem_cpy (port->dev_addr, ifp->hw_addr, ifp->hw_addr_len);

#ifdef HAVE_HA
  mstp_cal_create_mstp_port (port);
#endif /* HAVE_HA */

  mstp_cist_init_port (port);

  /* Link it to cist list */
  mstp_link_cist_port (br, port);

  mstp_cist_propagate_bridge_id (br);

  if (mstp_is_port_up (port->ifindex))
    {
      if (port->spanning_tree_disable)
        mstp_cist_handle_port_forward (port);
      else if (br->mstp_enabled)
        mstp_cist_enable_port (port);
      else
        mstp_cist_handle_br_forward (port);
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  return port;
}

static void
mstp_cist_handle_br_forward (struct mstp_port *port)
{
  struct mstp_bridge *br;
  struct mstp_instance_port *inst_port;

  if (!port)
    return;

  br = port->br;

  if (!br)
    return;

  if (!br->mstp_enabled && !br->mstp_brforward)
    return;

  if (!br->mstp_enabled && br->mstp_brforward)
    {
      mstp_cist_set_port_role (port, ROLE_DISABLED);
      port->cist_info_type = INFO_TYPE_DISABLED;
      mstp_cist_set_port_state_forward (port);

      inst_port = port->instance_list;
      while (inst_port)
        {
          mstp_msti_set_port_role (inst_port, ROLE_DISABLED);
          inst_port->info_type = INFO_TYPE_DISABLED;
          mstp_msti_set_port_state (inst_port, STATE_FORWARDING);
#ifdef HAVE_HA
          mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
          inst_port = inst_port->next_inst;
        }
    }
}


int
mstp_add_msti_port ( struct mstp_bridge_instance *br_inst,
                     struct mstp_port *port,
                     u_int32_t ifindex, int instance, int vid)
{
  struct mstp_instance_port *inst_port;

  /* Allocate the port */
  inst_port = XCALLOC(MTYPE_MSTP_PORT_INSTANCE,
                      sizeof (struct mstp_instance_port)) ;

  if (inst_port == NULL )
    {
      /* Memory cannot be allocated */
      zlog_err (mstpm, "Cannot allocate memory for port (%u)", ifindex);
      return RESULT_ERROR;
    }

  /* populate fields */
  if (port->type == MSTP_VLAN_PORT_MODE_PE)
    inst_port->msti_path_cost = 128;
  else
    inst_port->msti_path_cost = mstp_nsm_get_port_path_cost (mstpm, ifindex);

  inst_port->msti_priority = BRIDGE_DEFAULT_PORT_PRIORITY;
  inst_port->ifindex = ifindex;
  inst_port->cst_port = port;
  inst_port->br = br_inst;
  inst_port->instance_id = instance;
  inst_port->restricted_role = PAL_FALSE;
  inst_port->restricted_tcn = PAL_FALSE;

#ifdef HAVE_RPVST_PLUS
  inst_port->vlan_id = vid;
#endif /* HAVE_RPVST_PLUS */

  /* Port initially always in  forwarding state. */
  inst_port->state = STATE_FORWARDING;

#ifdef HAVE_HA
  mstp_cal_create_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

  mstp_msti_init_port (inst_port);

  /* Link it to br inst list */
  mstp_link_msti_port (br_inst, inst_port);

  /* link it to port inst list */
  mstp_link_port_instance (port,inst_port);

  /* propogate bridge info */
  mstp_msti_propagate_bridge_id (br_inst);

  if (mstp_is_port_up (inst_port->ifindex))
    mstp_msti_enable_port (inst_port);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

  return RESULT_OK;
}



/* Bridge to which the instance is to be added and instance id */
int
mstp_add_instance (struct mstp_bridge *br, short instance_id, mstp_vid_t vid,
                   u_int32_t vlan_range_indx)
{
  struct mstp_bridge_instance *br_inst = NULL;
  int ret;
  struct nsm_msg_vlan msg;
  int new_inst_flag = 0;
  struct rlist_info *vlan_list = NULL;
  u_int32_t bmp_index;
  u_int16_t invalid_range_idx = 0;

  if (!br)
    return RESULT_ERROR;

  /* Should not be able to add the common instance.
     Since instance count starts at zero we check for greater
     than or equal condition here. */
  if ((instance_id == MST_INSTANCE_IST) ||
      (instance_id >= br->max_instances))
    {
      zlog_err (mstpm,"Instance %d not within allowed range",instance_id);
      return RESULT_ERROR;
    }

  /* If the instance is already added, we  currently use the same
   * command to modify vlan mapping */
  br_inst = br->instance_list[instance_id];
  if (!br_inst)
    {
      br_inst = XCALLOC(MTYPE_MSTP_BRIDGE_INSTANCE,
                        sizeof (struct mstp_bridge_instance));

      br_inst->learning_enabled = PAL_TRUE;

      br_inst->internal_root_path_cost = 0 ;
      br_inst->msti_root_port_id = 0;
      br_inst->msti_root_port_ifindex = 0;

      br_inst->hop_count = MSTP_DEFAULT_HOP_COUNT;
      br_inst->msti_bridge_priority = BRIDGE_DEFAULT_PRIORITY;
      br_inst->port_list = NULL;
      br_inst->bridge = br;
      br_inst->port_index = 0;
      br_inst->instance_id = instance_id;

     /* Statistics for instance/vlan */
     br_inst->tc_flag = PAL_FALSE;
     br_inst->num_topo_changes = 0;
     br_inst->total_num_topo_changes = 0;
     br_inst->tc_initiator = 0;
     pal_mem_set (&br_inst->tc_last_rcvd_from, 0, ETHER_ADDR_LEN);

#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
      br_inst->mstp_enabled = PAL_TRUE;
#endif /*(HAVE_PROVIDER_BRIDGE) ||(HAVE_B_BEB)*/

#ifdef HAVE_RPVST_PLUS
      if (br->type == MSTP_BRIDGE_TYPE_RPVST_PLUS)
        {
          if (br->vlan_instance_map[instance_id] != vid)
            br->vlan_instance_map[instance_id] = vid;

          br_inst->message_age = BRIDGE_TIMER_DEFAULT_AGEING_TIME;
          br_inst->max_age = BRIDGE_TIMER_DEFAULT_MAX_AGE *
                             L2_TIMER_SCALE_FACT;
          br_inst->fwd_delay = BRIDGE_TIMER_DEFAULT_FWD_DELAY * L2_TIMER_SCALE_FACT;
          br_inst->hello_time = BRIDGE_TIMER_DEFAULT_HELLO_TIME *
                             L2_TIMER_SCALE_FACT;

          br_inst->vlan_id = vid;
          mstp_make_bridgeid (&(br_inst->msti_bridge_id),
                              (short)br_inst->msti_bridge_priority, vid);
        }
      else
#endif /* HAVE_RPVST_PLUS */
        mstp_make_bridgeid (&(br_inst->msti_bridge_id),
            (short)br_inst->msti_bridge_priority,instance_id);

      pal_mem_cpy (&br_inst->msti_designated_root,&br_inst->msti_bridge_id,
          MSTP_BRIDGE_ID_LEN);

      pal_mem_cpy (&br_inst->msti_designated_bridge,&br_inst->msti_bridge_id,
          MSTP_BRIDGE_ID_LEN);
      br_inst->vlan_list = NULL;
      br->instance_list[instance_id] = br_inst;
      new_inst_flag = 1;

#ifdef HAVE_HA
      mstp_cal_create_mstp_bridge_instance (br_inst);
#endif /* HAVE_HA */

      /* Data Structures for SNMP Operations.
       * Initilize vlan_range_index bmp
       */
      if (br_inst->vlan_range_index_bmp == NULL)
        br_inst->vlan_range_index_bmp = bitmap_new(4096,1,4096);

#if defined  HAVE_G8031 || defined HAVE_G8032
      /* Sending a message to Protection Groups in NSM marking the
       * the instanace as blocked for protection.
       */
      pal_mem_set(&msg, 0, sizeof(struct nsm_msg_vlan));
       
      msg.vid = 0;
 
      NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_BRIDGENAME);
      pal_strncpy (msg.bridge_name, br->name, NSM_BRIDGE_NAMSIZ);
       
      NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_INSTANCE);
      msg.br_instance = instance_id;
       
      nsm_client_send_vlan_msg (mstpm->nc, &msg, 
                                NSM_MSG_BLOCK_INSTANCE);
       
      pal_mem_set(&msg, 0, sizeof(struct nsm_msg_vlan));          
#endif /* HAVE_G8031 || defined HAVE_G8032 */
    }

  /* return if vlan id is zero; a vlan may be added later to this instance */
  if (vid == 0)
    {
      mstp_bridge_instance_activate_port (br->name, instance_id);
      return RESULT_OK;
    }

  /* Populate the vlan list */
  /* Delete the vlans from cist list */
  ret = mstp_rlist_delete (&br->vlan_list, vid, &invalid_range_idx);
  if (ret == RESULT_OK)
    {
      if (vlan_range_indx == 0 )
        {
          if (bitmap_request_index(br_inst->vlan_range_index_bmp,
              &bmp_index) != BITMAP_SUCCESS)
            {
              mstp_delete_instance (br, instance_id);
              return RESULT_ERROR;
            }
         }
       else 
         bmp_index = vlan_range_indx;

      ret = mstp_rlist_add (&br_inst->vlan_list, vid, bmp_index);
      
      if ((PAL_FALSE == ret) && (vlan_range_indx == 0))
        {
          bitmap_release_index (br_inst->vlan_range_index_bmp, bmp_index);
        }

      else
        {
          /* loop through rlist_info, for this vid, if the bmp_index
           * does not match rlist->vlan_range_indx then free bmp_index */
          vlan_list = br_inst->vlan_list;
          while (vlan_list)
            {
              if ((vlan_list->hi == vid) ||
                  (vlan_list->lo == vid))
                {
                  /* This is the overlap condition. Now for row 2, bitmap
                   * had already been reserved. If row 1 and row2 merge -
                   * are continuo - then row2 bitmap has to be released */
                  if (vlan_list->vlan_range_indx != bmp_index)
                    bitmap_release_index (br_inst->vlan_range_index_bmp,
                                          bmp_index);
                }
              vlan_list = vlan_list->next;
            }
        }

      /* Notify the forwarder about the association of
         vlan with this inst */
      /* Send message to nsm. */
      pal_mem_set(&msg, 0, sizeof(struct nsm_msg_vlan));
      msg.vid = vid;
      NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_BRIDGENAME);
      pal_strncpy (msg.bridge_name, br->name, NSM_BRIDGE_NAMSIZ);
      NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_INSTANCE);
      msg.br_instance = instance_id;
      nsm_client_send_vlan_msg(mstpm->nc, &msg, NSM_MSG_VLAN_ADD_TO_INST);
    }
  else
    {
      if (new_inst_flag == 1)
        mstp_delete_instance (br, instance_id);

      zlog_err (mstpm, "Cannot add vlan %d to instance %d : "
                "absent in common instance\n",vid,instance_id);
      return RESULT_ERROR;
    }

  mstp_gen_cfg_digest (br_inst->bridge);

  mstp_bridge_instance_activate_port (br->name, instance_id);

  return RESULT_OK;

}

static int
mstp_delete_vlans (struct mstp_bridge *br, int instance,
                   struct rlist_info *vlan_list)
{
  int i;
  struct nsm_msg_vlan msg;
  /* Send instance delete info to nsm. */
  while (vlan_list)
    {
      for (i = vlan_list->lo; i <= vlan_list->hi; i++)
        {
          pal_mem_set(&msg, 0, sizeof(struct nsm_msg_vlan));
          msg.vid = i;
          NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_BRIDGENAME);
          pal_strncpy (msg.bridge_name, br->name, NSM_BRIDGE_NAMSIZ);
          NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_INSTANCE);
          msg.br_instance = instance;
          nsm_client_send_vlan_msg(mstpm->nc, &msg,
                                   NSM_MSG_VLAN_DELETE_FROM_INST);
        }
      vlan_list = vlan_list->next;
    }

  return 0;
}

static int
mstp_add_vlans (struct mstp_bridge *br, int instance,
                struct rlist_info *vlan_list)
{
  int i;
  struct nsm_msg_vlan msg;
  struct mstp_vlan *vlan;

  while (vlan_list)
    {
      for (i = vlan_list->lo; i <= vlan_list->hi; i++)
        {

          vlan = mstp_bridge_vlan_lookup (br->name, i);

          if (vlan != NULL)
            {
              vlan->instance = instance;
#ifdef HAVE_HA
              mstp_cal_modify_mstp_vlan (vlan);
#endif /* HAVE_HA */
            }

          /* Send message to nsm to add vlan.  */
          pal_mem_set(&msg, 0, sizeof(struct nsm_msg_vlan));
          msg.vid = i;
          NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_BRIDGENAME);
          pal_strncpy (msg.bridge_name, br->name, NSM_BRIDGE_NAMSIZ);
          NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_INSTANCE);
          msg.br_instance = instance;
          nsm_client_send_vlan_msg(mstpm->nc, &msg,
                                   NSM_MSG_VLAN_ADD_TO_INST);
        }

      vlan_list = vlan_list->next;
    }

  return 0;
}

int
mstp_delete_instance (struct mstp_bridge *br, int instance)
{
  struct mstp_instance_port *inst_port = NULL;
  struct mstp_bridge_instance *br_inst = (br) ?
    br->instance_list[instance] : NULL ;

#if defined  HAVE_G8031 || defined HAVE_G8032
  struct nsm_msg_vlan msg;
#endif /* HAVE_G8031 || defined HAVE_G8032 */

  /* Cannot delete common instance */
  if ((!br_inst) || (instance ==  MST_INSTANCE_IST))
    return RESULT_ERROR;

  /*  TODO mstp_disable_instance  */

  while ((inst_port = br_inst->port_list))
    {
      if (LAYER2_DEBUG(proto, PROTO_DETAIL))
        zlog_debug (mstpm, " mstp_delete_instance:");
      mstp_delete_msti_port (inst_port, PAL_TRUE);
    }

  /* Delete vlans from instance in the forwarder. */
  mstp_delete_vlans (br, instance, br_inst->vlan_list);

  /* Add vlans to the default instance in the forwarder. */
  mstp_add_vlans (br, 0, br_inst->vlan_list);

  /* Move all the vlans associated with this instance to common instance */
  mstp_rlist_move (&br->vlan_list, &br_inst->vlan_list);

#ifdef HAVE_HA
  mstp_cal_delete_mstp_bridge_instance (br_inst);
#endif /* HAVE_HA */

  XFREE (MTYPE_MSTP_BRIDGE_INSTANCE, br_inst);

  br->instance_list[instance] = NULL;

#if defined  HAVE_G8031 || defined HAVE_G8032
  /* Sending a message to NSM to UnMask the instance
   * so that it can be used for Protection Groups.
   */
  msg.vid = 0;
        
  NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_BRIDGENAME);
  pal_strncpy (msg.bridge_name, br->name, NSM_BRIDGE_NAMSIZ);
        
  NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_INSTANCE);
  msg.br_instance = instance;
        
  nsm_client_send_vlan_msg(mstpm->nc, &msg, 
                           NSM_MSG_UNBLOCK_INSTANCE);
          
#endif /* HAVE_G8031 || defined HAVE_G8032 */

  mstp_gen_cfg_digest (br);

  return RESULT_OK;
}

int
mstp_delete_instance_vlan (struct mstp_bridge *br, int instance,
                           mstp_vid_t vid)
{
  struct mstp_bridge_instance *br_inst =
    (br) ? br->instance_list[instance] : NULL;
  int ret;
  struct nsm_msg_vlan msg;
  u_int16_t range_index = 0;

  if (br_inst == NULL || instance == MST_INSTANCE_IST)
    return RESULT_ERROR;

  ret = mstp_rlist_delete (&br_inst->vlan_list, vid, &range_index);
  if (ret == RESULT_OK)
    {
      if (range_index != 0)
        {
          /* Free the bitmap for range index at vr_instance */
          bitmap_release_index(br_inst->vlan_range_index_bmp, range_index);
        }

      mstp_rlist_add (&br->vlan_list, vid, 0);
      /* Send message to nsm. */
#ifdef HAVE_VLAN
      pal_mem_set(&msg, 0, sizeof(struct nsm_msg_vlan));
      msg.vid = vid;
      NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_BRIDGENAME);
      pal_strncpy (msg.bridge_name, br->name, NSM_BRIDGE_NAMSIZ);
      NSM_SET_CTYPE(msg.cindex, NSM_VLAN_CTYPE_INSTANCE);
      msg.br_instance = instance;
      nsm_client_send_vlan_msg(mstpm->nc, &msg,
                               NSM_MSG_VLAN_DELETE_FROM_INST);
#endif  /* HAVE_VLAN */
    }

  if (br_inst->vlan_list == NULL)
    {
      mstp_delete_instance(br, instance);
    }

  mstp_gen_cfg_digest (br);

  return RESULT_OK;
}

static int
mstp_delete_msti_port (struct mstp_instance_port *inst_port, bool_t notify_fwd)
{
  struct mstp_bridge_instance *br_inst;
  if (! inst_port)
    return RESULT_ERROR;
  br_inst = inst_port->br;

  /* Disable the port */
  if (LAYER2_DEBUG(proto, PROTO_DETAIL))
  zlog_debug (mstpm, "mstp_delete_msti_port:");
  mstp_msti_disable_port (inst_port);

#ifdef HAVE_HA
  mstp_cal_delete_mstp_inst_forward_delay_timer (inst_port);
  mstp_cal_delete_mstp_inst_message_age_timer (inst_port);
  mstp_cal_delete_mstp_inst_rb_timer (inst_port);
  mstp_cal_delete_mstp_inst_tc_timer (inst_port);
#endif /* HAVE_HA */

   /* Make sure all port timers are stopped */
  l2_stop_timer (&inst_port->message_age_timer);
  l2_stop_timer (&inst_port->forward_delay_timer);
  l2_stop_timer (&inst_port->recent_backup_timer);
  l2_stop_timer (&inst_port->tc_timer);

  if (l2_is_timer_running (inst_port->recent_root_timer))
    {
#ifdef HAVE_HA
      mstp_cal_delete_mstp_inst_rr_timer (inst_port);
#endif /* HAVE_HA */

      l2_stop_timer (&inst_port->recent_root_timer);
      inst_port->recent_root_timer = NULL;
      inst_port->br->br_inst_all_rr_timer_cnt--;
    }

  /* Remove the port for bridge instance list */
  mstp_unlink_msti_port (inst_port);

  /* Remove port from port instance list */
  mstp_unlink_port_instance (inst_port);

  mstp_delete_cist_port (inst_port->cst_port, PAL_FALSE,PAL_FALSE, notify_fwd);

  THREAD_TIMER_OFF (inst_port->tc_timer);

  pal_mem_set (inst_port, 0, sizeof(struct mstp_instance_port));

#ifdef HAVE_HA
  mstp_cal_delete_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

  XFREE (MTYPE_MSTP_PORT_INSTANCE, inst_port);
  mstp_msti_propagate_bridge_id (br_inst);

  return RESULT_OK;
}

int
mstp_delete_cist_port (struct mstp_port *port, int is_explicit , int force,
                       bool_t notify_fwd)
{
  enum add_type type = is_explicit ? TYPE_EXPLICIT : TYPE_IMPLICIT ;
  struct mstp_bridge *br = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  struct interface *ifp = NULL;

  pal_assert (port);
  br = port->br;

  /* Check if the port can be deleted */
  if ((port->type != type) && (!force))
    {
      if ((port->type != TYPE_EXPLICIT) || (port->ref_count <= 1))
        return RESULT_ERROR;

      port->ref_count-- ;
      return RESULT_OK;
    }

  if (port->ref_count > 0)
    port->ref_count--;
  else
    port->ref_count = 0;

  /* We cannot delete this port as some other instance is referencing it
   * or we were not explicitly told to do so */
  if ((!force) && (port->ref_count > 0))
    return RESULT_OK;

  /* Need to clean out ifp->port */
#ifdef HAVE_I_BEB
  if (!( br->ce_bridge || br->cn_bridge)
      && (ifp = if_lookup_by_index ( &vr->ifm, port->ifindex )))
    {
      ifp->port = NULL;
      pal_mem_set ( ifp->bridge_name, 0, INTERFACE_NAMSIZ + 1);
    }
#else
  if (!( br->ce_bridge )
      && (ifp = if_lookup_by_index ( &vr->ifm, port->ifindex )))
    {
      ifp->port = NULL;
      pal_mem_set ( ifp->bridge_name, 0, INTERFACE_NAMSIZ + 1);
    }
#endif

  /* Disable the port */
  if (port->spanning_tree_disable == PAL_FALSE)
    mstp_cist_disable_port (port);
  else
  {
#ifdef HAVE_HA
    if (l2_is_timer_running (port->message_age_timer))
      mstp_cal_delete_mstp_message_age_timer (port);
    if (l2_is_timer_running (port->migrate_timer))
      mstp_cal_delete_mstp_migrate_timer (port);
    if (l2_is_timer_running (port->hello_timer))
      mstp_cal_delete_mstp_hello_timer (port);
    if (l2_is_timer_running (port->forward_delay_timer))
      mstp_cal_delete_mstp_forward_delay_timer (port);
    if (l2_is_timer_running (port->recent_backup_timer))
      mstp_cal_delete_mstp_rb_timer (port);
    if (l2_is_timer_running (port->edge_delay_timer))
      mstp_cal_delete_mstp_edge_delay_timer (port);
    if (l2_is_timer_running (port->port_timer))
      mstp_cal_delete_mstp_port_timer (port);
    if (l2_is_timer_running (port->tc_timer))
      mstp_cal_delete_mstp_tc_timer (port);
#endif /* HAVE_HA */
  }

  /* Stop all port timers */
  l2_stop_timer (&port->message_age_timer);
  l2_stop_timer (&port->forward_delay_timer);
  l2_stop_timer (&port->migrate_timer);
  l2_stop_timer (&port->hello_timer);
  l2_stop_timer (&port->port_timer);
  l2_stop_timer (&port->recent_backup_timer);
  l2_stop_timer (&port->tc_timer);
  l2_stop_timer (&port->edge_delay_timer);

  if (l2_is_timer_running (port->recent_root_timer))
    {
#ifdef HAVE_HA
      mstp_cal_delete_mstp_rr_timer (port);
#endif /* HAVE_HA */

      l2_stop_timer (&port->recent_root_timer);
      port->recent_root_timer = NULL;
      port->br->br_all_rr_timer_cnt--;
    }

  /* Remove the port for bridge instance list */
  mstp_unlink_cist_port (port);

  if (port->ce_br)
    mstp_bridge_delete (NULL, port);

#ifdef HAVE_I_BEB
  if (port->cn_br)
    mstp_bridge_delete (NULL, port);
#endif

  br->num_ports--;

#ifdef HAVE_HA
  mstp_cal_delete_mstp_port (port);
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  pal_mem_set (port, 0, sizeof(struct mstp_port));
  XFREE (MTYPE_MSTP_BRIDGE_PORT, port);
  mstp_cist_propagate_bridge_id (br);

  return RESULT_OK;
}


int
mstp_delete_port (char * name, char * ifname, u_int16_t svid,
                  int instance, int force, bool_t notify_fwd)
{
  u_int32_t ifindex;
  struct interface *ifp;
  struct mstp_port *port;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  struct mstp_bridge *br = mstp_find_bridge (name);

  if (!br)
    {
      zlog_err (mstpm," Error: Bridge %s not found", name);
      return RESULT_ERROR;
    }

  ifp = if_lookup_by_name (&vr->ifm, ifname);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  if (br->ce_bridge)
    port = mstp_find_ce_br_port (br, svid);
  else
    port = mstp_find_cist_port (br, ifindex);

  if ((!br) || (!port))
    {
      zlog_err (mstpm," Error ;Port(%s) on bridge %s not found",
                ifname, name);
      return RESULT_ERROR;

    }

  if (instance != MST_INSTANCE_IST)
    {
      struct mstp_instance_port *inst_port;

      inst_port = mstp_find_msti_port (br->instance_list[instance], ifindex);

      if (!inst_port)
        {
          zlog_err (mstpm," Error ;Port(%s) for instance(%d) not found",
                    ifname,instance);
          return RESULT_ERROR;
        }

      mstp_delete_msti_port (inst_port, notify_fwd);
    }
  else
    {
      struct mstp_instance_port *curr,*next;

      for (curr = port->instance_list; curr; curr = next)
        {
          next = curr->next_inst;
          mstp_delete_msti_port (curr, notify_fwd);
        }

      if (br->ce_bridge)
        port = mstp_find_ce_br_port (br, svid);
      else
        port = mstp_find_cist_port (br, ifindex);

      if(port)
         mstp_delete_cist_port (port, PAL_TRUE, force, notify_fwd);
    }

  return RESULT_OK;
}

/* Update the port spanning-tree disable/enable */
int
mstp_port_update_spanning_tree_enable (char *name, char *ifname, 
                                       u_int8_t spanning_tree_disable)
{
  u_int32_t ifindex;
  struct interface *ifp = NULL;
  struct mstp_port *port = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  struct mstp_bridge *br = mstp_find_bridge (name);

  if (!br)
    {
      zlog_err (mstpm," Error: Bridge %s not found", name);
      return RESULT_ERROR;
    }

  if (ifname == NULL)
    return RESULT_ERROR;

  ifp = if_lookup_by_name (&vr->ifm, ifname);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  port = mstp_find_cist_port (br, ifindex);

  if ((!br) || (!port))
    {
      zlog_err (mstpm," Error ;Port(%s) on bridge %s not found",
                ifname, name);
      return RESULT_ERROR;

    }

  /* Update the spanning-tree mode */
  port->spanning_tree_disable = spanning_tree_disable;

  /* Disable the port */
  mstp_cist_disable_port (port);

  /* If port UP move it to forwarding state */
  if (mstp_is_port_up (port->ifindex))
    { 
      if (port->spanning_tree_disable == PAL_TRUE)
        mstp_cist_handle_port_forward (port);
      else if (br->mstp_enabled)
        mstp_cist_enable_port (port);
      else
        mstp_cist_handle_br_forward (port);
    } 
#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  return RESULT_OK;
}

static void
mstp_gen_digest ( u_char * data , int len , u_char * digest)
{
#ifdef HAVE_OPENSSL_HMAC_H
  HMAC_CTX  md5;
  u_int32_t md5_len = CONFIG_DIGEST_KEY_LEN;

  pal_mem_set (digest, 0, CONFIG_DIGEST_KEY_LEN);

  hmac_init_ctx (&md5, cd_signature_key,
                 CONFIG_DIGEST_KEY_LEN,
                 EVP_md5());
  hmac_process_bytes (&md5, data, len);
  hmac_finish_ctx (&md5, digest, &md5_len);
#else
  auth_hmac_md5 (data, len, cd_signature_key,
                 CONFIG_DIGEST_KEY_LEN, digest);
#endif
}

void
mstp_gen_cfg_digest (struct mstp_bridge *br)
{
  short config_table[4096];
  register int index;
  int instance;
  struct rlist_info *vlan_list;
  struct mstp_bridge_instance *br_inst;

  if (!br)
    return;

  /*Initialize -set all vids to 0 */
  for (index = 0 ; index < 4096; index++)
    config_table[index] = 0;

  for (instance =1; instance < br->max_instances; instance++)
    {
      br_inst = br->instance_list[instance];

      if (!br_inst)
        continue;

      vlan_list = br_inst->vlan_list;
      while (vlan_list)
        {
          for (index = vlan_list->lo; index <= vlan_list->hi ;index++)
            {
              config_table[index] = pal_hton16(instance);
            }

          vlan_list = vlan_list->next;
        }

    }


  mstp_gen_digest ((u_char *)config_table , 4096*2 ,
                   (u_char *) br->config.cfg_digest);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

}

void
mstp_bridge_set_cisco_interoperability (struct mstp_bridge *br,
                                        unsigned char enable)
{
  if (!br)
    return;

  br->admin_cisco = enable;

  if (!enable)
    {
      br->oper_cisco = PAL_FALSE;
      mstp_gen_cfg_digest (br);
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  return;

}



/* mstp_bridge_is_root returns non-zero if the specified
   argument bridge is the root bridge. */

inline int
mstp_bridge_is_cist_root (struct mstp_bridge * br)
{
  return !pal_mem_cmp (&br->cist_bridge_id,
                       &br->cist_designated_root, MSTP_BRIDGE_ID_LEN);
}


/* mstp_bridge_is_root returns non-zero if the specified
   argument bridge is the root bridge. */

inline int
mstp_bridge_is_msti_root (struct mstp_bridge_instance * br_inst)
{
  return !pal_mem_cmp (&br_inst->msti_bridge_id,
                       &br_inst->msti_designated_root, MSTP_BRIDGE_ID_LEN);
}


void
mstp_bridge_terminate ()
{
  register struct mstp_bridge * curr;

  curr = bridge_list;
  while (curr )
    {
      struct mstp_bridge *next = curr->next;

      mstp_bridge_delete (curr->name, NULL);
      curr = next;
    }

  /* Stop the system. */
  lib_stop (mstpm);
  mstpm = NULL;
}


/* Unlink a bridge config information from the bridge config list before
   deleting */
void mstp_bridge_config_list_unlink (struct mstp_bridge_config_list *old_conf_list)
{
  struct mstp_bridge_config_list *tmp = NULL;
  struct mstp_bridge_config_list *prev = NULL;

  tmp = mstp_get_mstp_bridge_config_list_head();
  prev = tmp;
  while (tmp)
    {
      if ( tmp == old_conf_list )
        {
          prev->next = tmp->next;
          if (mstp_bridge_config_list == tmp)
            mstp_bridge_config_list = tmp->next;
        }
      prev = tmp;
      tmp = tmp->next;
    }

  return;
}

/* Free the memory allocated for bridge config */
void mstp_bridge_config_list_free (struct mstp_bridge_info *br_config_info)
{
  struct mstp_bridge_config_list *br_conf_list = NULL;

  for (br_conf_list = mstp_get_mstp_bridge_config_list_head(); br_conf_list;
       br_conf_list = br_conf_list->next)
    {
      if (memcmp (&(br_conf_list->br_info), br_config_info,
                  sizeof (struct mstp_bridge_info)) == 0)
        {
          mstp_bridge_config_list_unlink(br_conf_list);
          XFREE (MTYPE_MSTP_BRIDGE_CONFIG, br_conf_list);
          br_conf_list = NULL;
          return;
        }
    }

  return;
}

/* Free the memory allocated for instance config */
void mstp_bridge_instance_list_free (struct mstp_bridge_info *br_config_info)
{
  struct mstp_bridge_instance_list *inst_list = NULL;
  struct mstp_bridge_instance_list *next_inst_list = NULL;

  for (inst_list = br_config_info->instance_list; inst_list;
       inst_list = next_inst_list)
    {
      next_inst_list = inst_list->next;
      mstp_rlist_free (&inst_list->instance_info.vlan_list);
      mstp_br_config_unlink_instance (br_config_info, inst_list);
      XFREE (MTYPE_MSTP_BRIDGE_CONFIG_INSTANCE_LIST, inst_list);
    }

  return;

}

/*Activate the bridge specific configuration */
void mstp_bridge_configuration_activate (char *bridge_name, u_char bridge_type)
{
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_instance_list *br_inst_list = NULL;
  struct mstp_bridge_instance_list *next_inst_list = NULL;
  int ret = 0;

  br_config_info = mstp_bridge_config_info_get (bridge_name);

  if (br_config_info == NULL)
    return;

  if (CHECK_FLAG (br_config_info->config,
                  MSTP_BRIDGE_CONFIG_MSTP_ENABLE))
    mstp_api_enable_bridge (bridge_name, br_config_info->mstp_enable_br_type);

  if (CHECK_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_PRIORITY))
    ret = mstp_api_set_bridge_priority (bridge_name, br_config_info->priority);

  if (CHECK_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_HELLO_TIME))
    ret = mstp_api_set_hello_time (bridge_name, br_config_info->hello_time);

  if (CHECK_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_MAX_AGE))
    ret = mstp_api_set_max_age (bridge_name, br_config_info->max_age);

  if (CHECK_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_FORWARD_TIME))
    ret = mstp_api_set_forward_delay (bridge_name, br_config_info->forward_time);

  if (CHECK_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_TX_HOLD_COUNT))
    mstp_api_set_transmit_hold_count (bridge_name,
                                      br_config_info->transmit_hold_count);

  if (CHECK_FLAG (br_config_info->config,
                  MSTP_BRIDGE_CONFIG_PORTFAST_BPDUGUARD))
    mstp_api_set_bridge_portfast_bpduguard (bridge_name,
                                            br_config_info->bpdugaurd);

  if (CHECK_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_ERRDISABLE))
    mstp_api_set_bridge_errdisable_timeout_enable (bridge_name,
                                                   br_config_info->err_disable);

  if (CHECK_FLAG (br_config_info->config,
                  MSTP_BRIDGE_CONFIG_ERRDISABLE_TIMEOUT))
    mstp_api_set_bridge_errdisable_timeout_interval (bridge_name,
                                                     br_config_info->errdisable_timeout_interval);

  if (CHECK_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_PATH_COST_METHOD))
    mstp_api_set_pathcost_method (bridge_name,
                                   br_config_info->path_cost_method);

  if (CHECK_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_BPDUFILTER))
    mstp_api_set_bridge_portfast_bpdufilter (bridge_name, PAL_TRUE);

  if (CHECK_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_FORCE_VERSION))
    mstp_api_set_bridge_forceversion (bridge_name,
                                      br_config_info->force_version);

  if (CHECK_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_SHUT_FORWARD))
     mstp_api_disable_bridge (bridge_name, BRIDGE_TYPE_XSTP, PAL_TRUE);

  if (bridge_type == NSM_BRIDGE_TYPE_MSTP)
    {
      if (CHECK_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_MAX_HOPS))
        mstp_api_set_max_hops (bridge_name, br_config_info->max_hops);

      if (CHECK_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_REVISION_LEVEL))
        mstp_api_revision_number (bridge_name, br_config_info->cfg_revision_lvl);

      if (CHECK_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_REGION_NAME))
        mstp_api_region_name (bridge_name, br_config_info->cfg_name);

      if (br_config_info->cisco_interop)
        {
          struct mstp_bridge *br = NULL;
          br = (struct mstp_bridge *) mstp_find_bridge (bridge_name);
          mstp_bridge_set_cisco_interoperability (br, PAL_TRUE);
        }

      for (br_inst_list = br_config_info->instance_list; br_inst_list;
        br_inst_list = next_inst_list)
        {

          next_inst_list = br_inst_list->next;

          if (CHECK_FLAG (br_inst_list->instance_info.config,
                          MSTP_BRIDGE_CONFIG_INSTANCE))
            {
              mstp_api_add_instance (bridge_name, br_inst_list->instance_info.instance,
                                     0, 0);

              if (CHECK_FLAG (br_inst_list->instance_info.config,
                              MSTP_BRIDGE_CONFIG_INSTANCE_PRIORITY))
                mstp_api_set_msti_bridge_priority (bridge_name,
                                                   br_inst_list->instance_info.instance,
                                                   br_inst_list->instance_info.priority);

              mstp_br_config_unlink_instance (br_config_info, br_inst_list);
              mstp_rlist_free (&br_inst_list->instance_info.vlan_list);
              XFREE (MTYPE_MSTP_BRIDGE_CONFIG_INSTANCE_LIST, br_inst_list);
            }
        }

      /* Free the instance list only when there no instance configured
      * with vlan. When there are instances configured with vlan
      * we have to activate the configuration only when receive
      * vlan add. Hence it cannot be freed here.
      */

      if (!CHECK_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_VLAN_INSTANCE))
        {
          mstp_bridge_instance_list_free (br_config_info);
          mstp_bridge_config_list_free (br_config_info);
        }
    }

  return;

}

void mstp_bridge_instance_config_add (char *bridge_name, int instance, u_int16_t vid)
{
  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;
  struct mstp_bridge_instance_info *br_inst_info = NULL;
  struct mstp_bridge_instance_list *br_inst_list = NULL;
  struct mstp_bridge *br;
  struct mstp_prefix_vlan p;
  struct route_node *rn;
  struct mstp_vlan *v;
  u_int16_t invalid_range_idx = 0;

  br = mstp_find_bridge (bridge_name);

  if (br && br->vlan_table)
    {
      MSTP_PREFIX_VLAN_SET (&p, vid);
      rn = route_node_lookup (br->vlan_table, (struct prefix *) &p);
      if (rn)
        {
          v = rn->info;
          route_unlock_node (rn);
          if (v && (v->vid == vid) && (v->instance != instance))
            return ;
        }
    }

  br_config_info = mstp_bridge_config_info_get (bridge_name);

  if (!br_config_info)
    {
      br_conf_list = mstp_bridge_config_list_new (bridge_name);
      if (br_conf_list == NULL)
        {
          return;
        }
      mstp_bridge_config_list_link (br_conf_list);
      br_config_info = &(br_conf_list->br_info);
    }

  br_inst_info = mstp_br_config_find_instance (br_config_info, instance);

  if (!br_inst_info)
    {
      br_inst_list = mstp_br_config_instance_new (instance);
      if (br_inst_list == NULL)
        {
           return;
        }

      mstp_br_config_link_instance_new (br_config_info, br_inst_list);
      br_inst_info = & (br_inst_list->instance_info);
    }

  if (vid)
    {
      mstp_rlist_delete (&br_inst_info->vlan_list, vid, &invalid_range_idx);
      mstp_rlist_add (&br_inst_info->vlan_list, vid, 0);
      UNSET_FLAG (br_inst_info->config, MSTP_BRIDGE_CONFIG_INSTANCE);
      SET_FLAG (br_inst_info->config, MSTP_BRIDGE_CONFIG_VLAN_INSTANCE);
      SET_FLAG (br_config_info->config, MSTP_BRIDGE_CONFIG_VLAN_INSTANCE);
    }
  else
    {
      SET_FLAG (br_inst_info->config, MSTP_BRIDGE_CONFIG_INSTANCE);
    }
}

/*Activate the bridge instance configuration */
void mstp_bridge_instance_activate (char *bridge_name, int vid)
{
  struct mstp_bridge_info          *br_config_info = NULL;
  struct mstp_bridge_instance_list *br_inst_list   = NULL;
  struct mstp_bridge_instance_list *next_inst_list = NULL;
  struct interface                 *ifp            = NULL;
  struct listnode                  *ln             = NULL;
  struct rlist_info *vlan_list = NULL;
  int l_vid = 0;
  char found = PAL_FALSE;
  u_int16_t invalid_range_idx = 0;

  br_config_info = mstp_bridge_config_info_get (bridge_name);

  if (br_config_info == NULL)
    return;

  for (br_inst_list = br_config_info->instance_list; br_inst_list;
       br_inst_list = next_inst_list)
     {
       next_inst_list = br_inst_list->next;
       if (!CHECK_FLAG (br_inst_list->instance_info.config,
                       MSTP_BRIDGE_CONFIG_VLAN_INSTANCE))
         continue;
       vlan_list = br_inst_list->instance_info.vlan_list;
       while (vlan_list)
         {
           for (l_vid = vlan_list->lo ; l_vid <= vlan_list->hi ; l_vid++)
             {
               if (l_vid != vid)
                 continue;

               mstp_api_add_instance (bridge_name,
                                      br_inst_list->instance_info.instance,
                                      vid, 0);
               if (CHECK_FLAG (br_inst_list->instance_info.config,
                               MSTP_BRIDGE_CONFIG_INSTANCE_PRIORITY))
                 {
                   mstp_api_set_msti_bridge_priority (bridge_name,
                                                      br_inst_list->instance_info.instance,
                                                      br_inst_list->instance_info.priority);
                   UNSET_FLAG (br_inst_list->instance_info.config,
                               MSTP_BRIDGE_CONFIG_INSTANCE_PRIORITY);
                 }
               found = PAL_TRUE;
               mstp_rlist_delete (&br_inst_list->instance_info.vlan_list, vid, 
                                  &invalid_range_idx);
               break;
             }

           if (found)
             break;
           vlan_list = vlan_list->next;
         }

       if (found && br_inst_list->instance_info.port_list)
         {
           LIST_LOOP (br_inst_list->instance_info.port_list,
                      ifp, ln)
             {
               mstp_api_add_port (bridge_name, ifp->name, MSTP_VLAN_NULL_VID,
                                  br_inst_list->instance_info.instance,
                                  PAL_FALSE);
             }
           list_delete (br_inst_list->instance_info.port_list);
         }

       if (found)
         {
           if (!br_inst_list->instance_info.vlan_list)
             {
               mstp_br_config_unlink_instance (br_config_info, br_inst_list);
               XFREE (MTYPE_MSTP_BRIDGE_CONFIG_INSTANCE_LIST, br_inst_list);
             }

           if (!br_config_info->instance_list)
             {
               mstp_bridge_config_list_free (br_config_info);
             }
           break;
         }
     }
}

/* Get the head of the bridge configuration list */
struct mstp_bridge_config_list *mstp_get_mstp_bridge_config_list_head ()
{
  return mstp_bridge_config_list;
}

/* Get the bridge config info for a particular bridge */
struct mstp_bridge_info * mstp_bridge_config_info_get (char *br_name)
{
  struct mstp_bridge_config_list *br_conf_list = NULL;

  for (br_conf_list = mstp_get_mstp_bridge_config_list_head(); br_conf_list;
       br_conf_list = br_conf_list->next)
    if (pal_strncmp (br_conf_list->br_info.bridge_name, br_name,
                     L2_BRIDGE_NAME_LEN) == 0)
      return &(br_conf_list->br_info);

  return NULL;
}

void mstp_br_config_instance_add_port (char *br_name, struct interface *ifp,
                                       s_int32_t instance)
{

  struct mstp_bridge_info *br_config_info = NULL;
  struct mstp_bridge_instance_list *br_inst_list = NULL;
  struct mstp_bridge_instance_info *br_inst_info = NULL;
  struct mstp_bridge_config_list *br_conf_list = NULL;

  br_config_info = mstp_bridge_config_info_get (br_name);

  if (!br_config_info)
    {
      br_conf_list = mstp_bridge_config_list_new (br_name);
      if ( !br_conf_list)
        {
          zlog_err (mstpm," Unable to allocate memory for bridge %s",
                            br_name);
          return;
        }
      mstp_bridge_config_list_link (br_conf_list);
      br_config_info = &(br_conf_list->br_info);
    }

   br_inst_info = mstp_br_config_find_instance (br_config_info, instance);

   if (!br_inst_info)
     {
       br_inst_list = mstp_br_config_instance_new (instance);
       if ( !br_inst_list)
        {
          zlog_err (mstpm," Unable to allocate memory for bridge %s",
                            br_name);
          return;
        }
        mstp_br_config_link_instance_new (br_config_info, br_inst_list);
        br_inst_info = & (br_inst_list->instance_info);
     }

   if (!listnode_lookup (br_inst_info->port_list, ifp))
     {
       listnode_add (br_inst_info->port_list, ifp);
     }

   SET_FLAG (br_inst_info->config, MSTP_BRIDGE_CONFIG_INSTANCE_PORT);

   return;
}

void mstp_bridge_instance_activate_port (char *bridge_name,
                                         s_int32_t instance)
{

  struct interface                 *ifp            = NULL;
  struct mstp_bridge_info          *br_config_info = NULL;
  struct mstp_bridge_instance_list *br_inst_list   = NULL;
  struct mstp_bridge_instance_info *br_inst_info   = NULL;
  struct listnode                  *ln             = NULL;

  br_config_info = mstp_bridge_config_info_get (bridge_name);

  if (!br_config_info)
    return;

  MSTP_BRIDGE_INSTANCE_CONFIG_GET (br_config_info->instance_list,
                                   br_inst_list, instance);

  if (!br_inst_list)
    return;

  br_inst_info = &(br_inst_list->instance_info);

  if (!br_inst_info || !br_inst_info->port_list)
    return;

  LIST_LOOP (br_inst_info->port_list, ifp, ln)
  {
#ifndef HAVE_RPVST_PLUS
    mstp_api_add_port (bridge_name, ifp->name, MSTP_VLAN_NULL_VID,
                       instance, PAL_FALSE);
#else
    mstp_api_add_port (bridge_name, ifp->name, br_inst_info->vlan_id,
                       instance, PAL_FALSE);
#endif /* !HAVE_RPVST_PLUS */
  }

  /* If we need to activate only the port configuration, free the
   * instance structure.
   */

  if ( !(br_inst_info->config & (~MSTP_BRIDGE_CONFIG_INSTANCE_PORT)) )
    {
      mstp_br_config_unlink_instance (br_config_info, br_inst_list);
      list_delete (br_inst_info->port_list);
      XFREE (MTYPE_MSTP_BRIDGE_CONFIG_INSTANCE_LIST, br_inst_list);
    }
  else
    {
      list_delete (br_inst_info->port_list);
      br_inst_info->port_list = NULL;
    }

  if (!br_config_info->instance_list)
    {
      mstp_bridge_config_list_free (br_config_info);
    }
}

void mstp_bridge_instance_config_delete (char *bridge_name,
                                         s_int32_t instance,
                                         mstp_vid_t vid)
{

  struct mstp_bridge_info          *br_config_info = NULL;
  struct mstp_bridge_instance_list *br_inst_list   = NULL;
  struct mstp_bridge_instance_info *br_inst_info   = NULL;
  u_int16_t invalid_range_idx = 0;

  br_config_info = mstp_bridge_config_info_get (bridge_name);

  if (!br_config_info)
    return;

  MSTP_BRIDGE_INSTANCE_CONFIG_GET (br_config_info->instance_list,
                                   br_inst_list, instance);

  if (!br_inst_list)
    return;

  br_inst_info = &(br_inst_list->instance_info);

  if (!br_inst_info)
    return;

  if (br_inst_info->vlan_list && vid)
    mstp_rlist_delete (&br_inst_info->vlan_list, vid, &invalid_range_idx);

  /* If this is the last vlan for this instance, free the instance */

  if ( vid && br_inst_info->vlan_list)
    return;

  mstp_br_config_unlink_instance (br_config_info, br_inst_list);

  if (br_inst_info->port_list)
    list_delete (br_inst_info->port_list);

  if (br_inst_info->vlan_list)
    mstp_rlist_free (&br_inst_info->vlan_list);

  XFREE (MTYPE_MSTP_BRIDGE_CONFIG_INSTANCE_LIST, br_inst_list);

  if (!br_config_info->instance_list)
    {
      mstp_bridge_config_list_free (br_config_info);
    }

  return;

}

void mstp_bridge_port_instance_config_delete (struct interface *ifp,
                                              char *bridge_name,
                                              s_int32_t instance)
{

  struct mstp_bridge_info          *br_config_info = NULL;
  struct mstp_bridge_instance_list *br_inst_list   = NULL;
  struct mstp_bridge_instance_info *br_inst_info   = NULL;

  br_config_info = mstp_bridge_config_info_get (bridge_name);

  if (!br_config_info)
    return;

  MSTP_BRIDGE_INSTANCE_CONFIG_GET (br_config_info->instance_list,
                                   br_inst_list, instance);

  if (!br_inst_list)
    return;

  br_inst_info = &(br_inst_list->instance_info);

  if (!br_inst_info || !br_inst_info->port_list)
    return;

  listnode_delete (br_inst_info->port_list, ifp);

  if (LIST_ISEMPTY (br_inst_info->port_list) &&
      (!(br_inst_info->config & (~MSTP_BRIDGE_CONFIG_INSTANCE_PORT))) )
    {
      mstp_br_config_unlink_instance (br_config_info, br_inst_list);
      list_delete (br_inst_info->port_list);
      XFREE (MTYPE_MSTP_BRIDGE_CONFIG_INSTANCE_LIST, br_inst_list);
    }

  if (!br_config_info->instance_list)
    {
      mstp_bridge_config_list_free (br_config_info);
    }

  return;
}


/* Allocate a new bridge config list for the given bridge */
struct mstp_bridge_config_list * mstp_bridge_config_list_new (char *br_name)
{
  struct mstp_bridge_config_list *br_conf_list = NULL;

  br_conf_list = XCALLOC (MTYPE_MSTP_BRIDGE_CONFIG,
                          sizeof (struct mstp_bridge_config_list));

  if (br_conf_list == NULL)
    return NULL;

  pal_strcpy (br_conf_list->br_info.bridge_name, br_name);
  return br_conf_list;
}

/* Link a new bridge config information to the bridge config list */
void mstp_bridge_config_list_link (
                                   struct mstp_bridge_config_list *new_conf_list)
{
  struct mstp_bridge_config_list *tmp = NULL;

  tmp = mstp_bridge_config_list;
  new_conf_list->next = tmp;
  mstp_bridge_config_list = new_conf_list;

  return;
}

struct mstp_bridge_instance_list * mstp_br_config_instance_new (int instance)
{
  struct mstp_bridge_instance_list *instance_list;

  instance_list = XCALLOC (MTYPE_MSTP_BRIDGE_CONFIG_INSTANCE_LIST,
                           sizeof (struct mstp_bridge_instance_list));

  if (instance_list == NULL)
    return NULL;

  instance_list->instance_info.instance = instance;
  instance_list->instance_info.vlan_list = NULL;
  instance_list->instance_info.port_list = list_new();


  if (!instance_list->instance_info.port_list)
    {
      XFREE (MTYPE_MSTP_BRIDGE_CONFIG_INSTANCE_LIST,
             instance_list);
      return NULL;
    }

  return instance_list;
}

/*Link the new created fdb list entry to existing link list */
void mstp_br_config_link_instance_new (
                                       struct mstp_bridge_info *br_info,
                                       struct mstp_bridge_instance_list *instance_new_list)
{
  struct mstp_bridge_instance_list *tmp = NULL;

  tmp = br_info->instance_list;
  instance_new_list->next = tmp;
  br_info->instance_list = instance_new_list;

  return;
}

/*Link the new created fdb list entry to existing link list */
void mstp_br_config_unlink_instance (
                                     struct mstp_bridge_info *br_info,
                                     struct mstp_bridge_instance_list *conf_instance_list)
{
  struct mstp_bridge_instance_list *tmp = NULL;
  struct mstp_bridge_instance_list *prev = NULL;

  tmp = br_info->instance_list;
  prev = tmp;
  while (tmp)
    {
      if ( tmp == conf_instance_list )
        {
          prev->next = tmp->next;
          if (br_info->instance_list == tmp)
            br_info->instance_list = tmp->next;
        }
      prev = tmp;
      tmp = tmp->next;
    }

  return;
}

struct mstp_bridge_instance_info *mstp_br_config_find_instance (
                                                                struct mstp_bridge_info *br_info,
                                                                int instance)
{
  struct mstp_bridge_instance_list *br_inst_list = NULL;

  for (br_inst_list = br_info->instance_list; br_inst_list;
       br_inst_list = br_inst_list->next)
    if (br_inst_list->instance_info.instance == instance)
      return &(br_inst_list->instance_info);

  return NULL;
}

int
mstp_cist_has_designated_port (struct mstp_bridge *bridge)
{
  struct mstp_port *port;

  for (port = bridge->port_list; port; port = port->next)
     if (port->cist_role == ROLE_DESIGNATED)
       return PAL_TRUE;

  return PAL_FALSE;

}

#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
int mstp_enable_bridge_instance (struct mstp_bridge_instance *mstp_br_inst)
{
   struct mstp_instance_port *inst_port = NULL;

   if (mstp_br_inst == NULL)
     return RESULT_ERROR ;

   if (mstp_br_inst->mstp_enabled)
     /* bridge is already enabled do nothing */
     return RESULT_OK;

   inst_port = mstp_br_inst->port_list;
   if (inst_port == NULL)
     return RESULT_ERROR;

   for (inst_port = mstp_br_inst->port_list; inst_port; inst_port = inst_port->next)
     {
        if (inst_port->cst_port->spanning_tree_disable == PAL_TRUE
            && inst_port->cst_port->cist_state == STATE_FORWARDING)
          continue;

        mstp_msti_set_port_state (inst_port, STATE_DISCARDING);
#ifdef HAVE_HA
        mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
        mstp_msti_enable_port (inst_port);
     }

   mstp_br_inst->mstp_enabled = PAL_TRUE;

#ifdef HAVE_HA
   mstp_cal_modify_mstp_bridge (mstp_br_inst->bridge);
   mstp_cal_modify_mstp_bridge_instance (mstp_br_inst);
#endif /* HAVE_HA */

   return RESULT_OK;
}

int mstp_disable_bridge_instance (struct mstp_bridge_instance *mstp_br_inst)
{
   struct mstp_instance_port *port = NULL;

   if (mstp_br_inst == NULL)
     return RESULT_ERROR;

   if (! mstp_br_inst->mstp_enabled)
     /* bridge instance is already disabled do nothing */
     return RESULT_OK;

   port = mstp_br_inst->port_list;
   if (port == NULL)
     return RESULT_ERROR;

   for (port = mstp_br_inst->port_list; port ; port = port->next)
     {
        mstp_msti_disable_port (port);
        mstp_msti_set_port_state_forward (port);
#ifdef HAVE_HA
        mstp_cal_delete_mstp_inst_tc_timer (port);
        mstp_cal_delete_mstp_inst_rr_timer (port);
        mstp_cal_delete_mstp_inst_rb_timer (port);
        mstp_cal_delete_mstp_inst_forward_delay_timer (port);
#endif /* HAVE_HA */

        /* Make sure all port timers are stopped */
        l2_stop_timer (&port->message_age_timer);
        l2_stop_timer (&port->forward_delay_timer);
        l2_stop_timer (&port->recent_backup_timer);
        l2_stop_timer (&port->tc_timer);
     }

   mstp_br_inst->mstp_enabled = PAL_FALSE;

#ifdef HAVE_HA
   mstp_cal_modify_mstp_bridge_instance (mstp_br_inst);
#endif /* HAVE_HA */

   return RESULT_OK;
}

#endif /*(HAVE_PROVIDER_BRIDGE) || (HAVE_B_BEB)*/
