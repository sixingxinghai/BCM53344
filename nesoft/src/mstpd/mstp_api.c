/* 
   Copyright (C) 2003  All Rights Reserved. 
   
   LAYER 2 BRIDGE-API
   
   This module provides the API for CLI interface 
   to talk to the MSTP module
  
*/

#include "pal.h"
#include "lib.h"

#include "l2_timer.h"
#include "l2_debug.h"
#include "l2lib.h"

#include "nsm_client.h"
#include "mstp_config.h"
#include "mstp_types.h"
#include "mstp_bridge.h"
#include "mstpd.h"
#include "mstp_api.h"
#include "mstp_bpdu.h"
#include "mstp_cist_proto.h"
#include "mstp_msti_proto.h"
#include "mstp_transmit.h"
#include "mstp_port.h"
#include "mstp_rlist.h"
#include "bitmap.h"

#ifdef HAVE_HA
#include "mstp_cal.h"
#endif /* HAVE_HA */

#ifdef HAVE_SNMP
#include "mstp_snmp.h"
#endif /*HAVE_SNMP */

u_int32_t mstp_nsm_get_port_path_cost (struct lib_globals *zg, const u_int32_t ifindex);

static inline int
mstp_is_fwd_delay_ge_maxage (s_int32_t forward_delay, s_int32_t max_age)
{
  if ((2 * (forward_delay - L2_TIMER_SCALE_FACT)) >= max_age)
    return PAL_TRUE;
  
  zlog_warn (mstpm," Violates Forward delay - Max age relationship"
             "2*(%d - 1) >= %d",forward_delay/L2_TIMER_SCALE_FACT,
             max_age/L2_TIMER_SCALE_FACT);
  return PAL_FALSE;

}

/* 802.1t - all values are in "time_unit" */
static inline int
mstp_is_maxage_ge_hello_time (s_int32_t max_age, s_int32_t hello_time)
{
  if (max_age >= 2 * (hello_time + L2_TIMER_SCALE_FACT))
    return PAL_TRUE;
  
  zlog_warn (mstpm," Violates Hello time - Max age relationship"
             "2*(%d + 1) <= %d",hello_time/L2_TIMER_SCALE_FACT,
             max_age/L2_TIMER_SCALE_FACT);
  return PAL_FALSE;
}


/* 802.1t section 8.10.2 */
static inline int
mstp_is_hello_time_gt_hold_time (s_int32_t hello_time)
{
  return PAL_TRUE;
}

int
mstp_api_set_transmit_hold_count (char * name, unsigned char txholdcount)
{

  int ret = RESULT_ERROR;
  struct mstp_bridge * br = mstp_find_bridge (name);
  struct mstp_port *port;
  unsigned count = 0;

  if(!br)
    return ret;
                                                                                
  port = br->port_list;

  if (txholdcount < MSTP_MIN_BRIDGE_TX_HOLD_COUNT ||
      txholdcount > MSTP_MAX_BRIDGE_TX_HOLD_COUNT )
    return ret;

  count = txholdcount;

  if (br != NULL)
    {
      br->transmit_hold_count = count;
      if (port != NULL)
        {
          port->tx_count = 0;
        }
#ifdef HAVE_HA
      mstp_cal_modify_mstp_bridge (br);
      if (port != NULL)
        {
          mstp_cal_modify_mstp_port (port);
        }
#endif /* HAVE_HA */
      ret = RESULT_OK;
    }
  return ret;
}

/* Get maximum number of transmissions of BPDUs by transmit state machine */
int
mstp_api_get_transmit_hold_count (char * name, u_char *txholdcount)
{

  int ret = RESULT_ERROR;
  struct mstp_bridge * br = mstp_find_bridge (name);

  if (br != NULL)
    {
      *txholdcount = br->transmit_hold_count;
      ret = RESULT_OK;
    }
  return ret;
}


#ifndef HAVE_L2GP
static void
mstp_put_prio_in_bridgeid (struct bridge_id *br_id, short new_priority)
#else
void
mstp_put_prio_in_bridgeid (struct bridge_id *br_id, short new_priority)
#endif /*HAVE_L2GP*/
{
  u_char old_priority;
  /* Store the old System ID extension */
  old_priority = br_id->prio[0] & 0x0f;
  /* Settable priority comprises only of four most sigificant bits */
  br_id->prio[0] = (new_priority >> 8) & 0xf0 ;
  br_id->prio[0] |= old_priority;

}

/* Add a port to a bridge instance. */
int
mstp_api_add_port (char * name, char *ifname, u_int16_t svid,
                   int instance, u_int8_t spanning_tree_disable )
{

  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  struct mstp_bridge * br = NULL;
  struct mstp_port *port = NULL;
  struct interface *ifp = NULL;
  struct mstp_interface *mstpif = NULL;
  int ret;

  br = mstp_find_bridge (name);

  if (br == NULL)
    {
      return RESULT_ERROR;
    }
    
  /*    Return error if the instance has not been created. */
  if ((instance != MST_INSTANCE_IST) && 
      (br->instance_list[instance] == NULL))
    return RESULT_ERROR;

  ret = mstp_add_port (br, ifname, svid, instance, spanning_tree_disable);

  if (instance != MST_INSTANCE_IST 
      || ret < 0)
    return ret;

  ifp = if_lookup_by_name (&vr->ifm, ifname);

  if (!ifp)
    return RESULT_ERROR;

  mstpif = ifp->info;

  if (!mstpif)
    return RESULT_ERROR;

  if (br->ce_bridge)
    port = mstp_find_ce_br_port (br, svid);
  else
    port = mstp_find_cist_port (br, ifp->ifindex);

  if (!port)
    return RESULT_ERROR;

  switch (br->type)
    {
      case NSM_BRIDGE_TYPE_STP:
      case NSM_BRIDGE_TYPE_STP_VLANAWARE:
        port->force_version = BR_VERSION_STP;
       break;
      case NSM_BRIDGE_TYPE_RSTP:
      case NSM_BRIDGE_TYPE_RSTP_VLANAWARE:
      case NSM_BRIDGE_TYPE_RPVST_PLUS:
      case NSM_BRIDGE_TYPE_BACKBONE_RSTP:
        port->force_version = BR_VERSION_RSTP;
        break;
      case NSM_BRIDGE_TYPE_MSTP:
      case NSM_BRIDGE_TYPE_PROVIDER_MSTP:
      case NSM_BRIDGE_TYPE_BACKBONE_MSTP:
        port->force_version = BR_VERSION_MSTP;
        break;
      default:
        break;
    }

  /* Set port force-version and mstpif version */
  port->force_version = br->force_version;
  mstpif->version = br->force_version;

  /* Set send_mstp based on Bridge force version */
  if(br->force_version == BR_VERSION_STP)
    port->send_mstp = PAL_FALSE;
  else
    port->send_mstp = PAL_TRUE;

  return ret;

}

/* Delete port from the bridge instance. */
int
mstp_api_delete_port (char * name, char *ifname, int instance, u_int16_t svid,
                      int force, bool_t notify_fwd)
{

  struct mstp_bridge * br = NULL;
  int ret;

  br = mstp_find_bridge (name);

  if (br == NULL)
    {
      return RESULT_ERROR;
    }
  /* Return error if the instance has not been created. */
  if ((instance != MST_INSTANCE_IST) && 
      (br->instance_list[instance] == NULL))
    return RESULT_ERROR;

  ret = mstp_delete_port (name, ifname, svid, instance, force, notify_fwd);

  return ret;
}

int
mstp_api_region_name (char *name, char *region_name)
{
  struct mstp_bridge *br = mstp_find_bridge (name);
  struct mstp_port * port = NULL;

  if (br == NULL)
    return RESULT_ERROR;

  if (region_name)
    if (!pal_strcmp (br->config.cfg_name, region_name))
      return RESULT_ERROR;
  
  if (region_name != NULL)
    pal_strncpy (br->config.cfg_name, region_name, MSTP_CONFIG_NAME_LEN);
  else
    br->config.cfg_name[0] = '\0';

  for (port = br->port_list; port; port = port->next)
    {
      port->reselect = PAL_TRUE;
      port->selected = PAL_FALSE;
      if (port->cist_role == ROLE_DISABLED)
        port->cist_info_type = INFO_TYPE_DISABLED;
      else
        port->cist_info_type = INFO_TYPE_MINE;
#ifdef HAVE_HA
      mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  mstp_cist_port_role_selection (br);

  mstp_tx_bridge (br);

  return RESULT_OK;

}

/* Get MST region name */
int
mstp_api_get_region_name (char *name, char *region_name)
{
  struct mstp_bridge *br = mstp_find_bridge (name);

  if (br == NULL)
    return RESULT_ERROR;

  pal_strncpy (region_name, br->config.cfg_name, MSTP_CONFIG_NAME_LEN);

  return RESULT_OK;
}

int
mstp_api_revision_number (char * name, u_int16_t rev_num)
{
  struct mstp_bridge *br = mstp_find_bridge (name);
  struct mstp_port *port = NULL;
 
  if (br == NULL)
    {
      return RESULT_ERROR;
    }
    
  br->config.cfg_revision_lvl = rev_num;
  
  for (port = br->port_list; port; port = port->next)
    {
      port->reselect = PAL_TRUE;
      port->selected = PAL_FALSE;
      if (port->cist_role == ROLE_DISABLED)
        port->cist_info_type = INFO_TYPE_DISABLED;
      else
        port->cist_info_type = INFO_TYPE_MINE;

#ifdef HAVE_HA
      mstp_cal_modify_mstp_port (port);
#endif  /* HAVE_HA */
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  mstp_cist_port_role_selection (br);

  mstp_tx_bridge (br);

  return RESULT_OK;
}

/* Get the revision number */
int
mstp_api_get_revision_number (char * name, u_int16_t *rev_num)
{
  struct mstp_bridge *br = mstp_find_bridge (name);
  
  if (br == NULL)
    {
      return RESULT_ERROR;
    }
    
  *rev_num = br->config.cfg_revision_lvl;

  return RESULT_OK;
}

int
mstp_api_add_instance (char * name, int instance, mstp_vid_t vid,
                       u_int32_t  vlan_range_indx)
{
  struct mstp_bridge *br = mstp_find_bridge (name);
  s_int32_t ret = 0;
  struct mstp_vlan *vlan;
#ifdef HAVE_PVLAN
  u_int16_t secondary_vid;
#endif /* HAVE_PVLAN */
  
  if (br == NULL)
    return RESULT_ERROR;
 
  vlan = mstp_bridge_vlan_lookup (br->name, vid);

#ifdef HAVE_PVLAN
  if ((vlan)
       && (vlan->pvlan_type == MSTP_PVLAN_SECONDARY))
    return RESULT_ERROR;
#endif /* HAVE_PVLAN */

#if defined  HAVE_G8031 || defined HAVE_G8032
  if (MSTP_INSTANCE_BMP_IS_MEMBER(br->mstpProtectionBmp, instance))
     return MSTP_INSTANCE_IN_USE_ERR;
#endif /* HAVE_G8031 || defined HAVE_G8032 */
  
  ret = mstp_add_instance (br, instance, vid, vlan_range_indx);

  if (ret < 0)
    return ret;

  if (vlan)
    vlan->instance = instance;

#ifdef HAVE_HA
 mstp_cal_modify_mstp_vlan (vlan);
#endif /* HAVE_HA */ 

#ifdef HAVE_PVLAN
  if (vlan)
    {
      for (secondary_vid = 2; secondary_vid < MSTP_VLAN_MAX; secondary_vid++)
         {
           if (MSTP_VLAN_BMP_IS_MEMBER (vlan->secondary_vlan_bmp,
                                        secondary_vid))
             ret = mstp_add_instance (br, instance, secondary_vid, 0);

           if (ret < 0)
             return ret;
         }
    }
#endif /* HAVE_PVLAN */

  return ret;
}

int 
mstp_api_delete_instance (char * name , int instance)
{
  struct mstp_bridge *br = mstp_find_bridge (name);
  int ret; 
 
  if (br == NULL)
    return RESULT_ERROR;

  if (!br->instance_list[instance])
    return RESULT_ERROR;
   
  ret = mstp_delete_instance (br, instance);

  return ret; 
}

int
mstp_api_delete_instance_vlan (char *name, int instance, mstp_vid_t vid)
{
  struct mstp_bridge *br = mstp_find_bridge (name);
  struct mstp_vlan *vlan;
  s_int32_t ret = 0;
#ifdef HAVE_PVLAN
  u_int16_t secondary_vid;
#endif /* HAVE_PVLAN */

  if (br == NULL)
    return RESULT_ERROR;

  ret = mstp_delete_instance_vlan (br, instance, vid);

  if (ret < 0)
    return ret;

  vlan = mstp_bridge_vlan_lookup (br->name, vid);

  if (vlan != NULL)
    {
      vlan->instance = 0;
#ifdef HAVE_HA
      mstp_cal_modify_mstp_vlan (vlan);
#endif /* HAVE_HA */
    }

#ifdef HAVE_PVLAN

  if (!vlan)
    return RESULT_ERROR;

  for (secondary_vid = 2; secondary_vid < MSTP_VLAN_MAX; secondary_vid ++)
    {
      if (MSTP_VLAN_BMP_IS_MEMBER (vlan->secondary_vlan_bmp, secondary_vid))
         ret = mstp_delete_instance_vlan (br, instance, secondary_vid);

      if (ret < 0)
        return ret;
    }
#endif /* HAVE_PVLAN */

  return 0;
}

/* Set the forward delay interval (in secs) for this bridge */
int
mstp_api_set_forward_delay (char * name, s_int32_t forward_delay)
{
  int ret = RESULT_ERROR;
  int forward;
  struct mstp_bridge *br = mstp_find_bridge (name);
  
  if ((forward_delay < MSTP_MIN_BRIDGE_FWD_DELAY) 
      || (forward_delay > MSTP_MAX_BRIDGE_FWD_DELAY))
    return ret;
  
  forward = forward_delay * L2_TIMER_SCALE_FACT;
  
  if ((br != NULL) 
      && (mstp_is_fwd_delay_ge_maxage (forward, br->bridge_max_age)))
    {
      br->bridge_forward_delay = forward; 
      if (mstp_bridge_is_cist_root (br))
        {
          br->cist_forward_delay = forward;
          mstp_cist_port_role_selection (br);
          mstp_tx_bridge (br);
        }
#ifdef HAVE_HA
      mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

      return RESULT_OK;
    }
  return ret;
}

/* Get the forward delay interval (in secs) for this bridge */
int
mstp_api_get_forward_delay (char * name, s_int32_t *forward_delay)
{
  int ret = RESULT_ERROR;
  struct mstp_bridge *br = mstp_find_bridge (name);
  
  if (br != NULL) 
    {
      *forward_delay = (br->bridge_forward_delay) / L2_TIMER_SCALE_FACT; 
      return RESULT_OK;
    }
  return ret;
}

/* Set the bridge hello interval (in secs) */

int
mstp_api_set_hello_time (char * name, s_int32_t hello_time)
{
  int ret = RESULT_ERROR;
  struct mstp_bridge *br = mstp_find_bridge (name);
  s_int32_t hello;
  int update_ports = PAL_FALSE;
  struct mstp_port * port;
  
  if ((hello_time < MSTP_MIN_BRIDGE_HELLO_TIME)
      || (hello_time > MSTP_MAX_BRIDGE_HELLO_TIME))
    return ret;

  if (IS_BRIDGE_MSTP (br)||
      IS_BRIDGE_RSTP (br))
    {
       zlog_warn (mstpm, "Can't configure hello-time for %s bridge %s",
                  IS_BRIDGE_RSTP(br) ? "RSTP" : "MSTP", br->name);
       return MSTP_ERR_HELLO_NOT_CONFIGURABLE;
    }

  hello = hello_time * L2_TIMER_SCALE_FACT;
  
  if ((br != NULL) 
      && mstp_is_maxage_ge_hello_time (br->bridge_max_age, hello) 
      && mstp_is_hello_time_gt_hold_time (hello))
    {
      if (hello < br->bridge_hello_time)
        {
          update_ports = PAL_TRUE;
        }
      br->bridge_hello_time = hello;

      if (mstp_bridge_is_cist_root (br))
        {
          br->cist_hello_time = hello;
          mstp_cist_port_role_selection (br);
          if (update_ports)
            {
              BRIDGE_CIST_PORTLIST_LOOP(port, br)
                {
                  if (l2_is_timer_running (port->hello_timer))
                    {
#ifdef HAVE_HA
                      mstp_cal_delete_mstp_hello_timer (port);
#endif /* HAVE_HA */
                      l2_stop_timer (&port->hello_timer);
                      port->hello_timer =
                      l2_start_timer (mstp_hello_timer_handler,
                                      port, br->cist_hello_time,mstpm);
#ifdef HAVE_HA
                      mstp_cal_create_mstp_hello_timer (port);
#endif /* HAVE_HA */
                    }
                }/*Bridge list loop */
            }/* if update ports */

          mstp_tx_bridge (br);
        } /* if mstp_bridge_is_cist_root */

#ifdef HAVE_HA
      mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

      return RESULT_OK;
    }
  return ret;
}

/* Get the bridge hello interval (in secs) */
int
mstp_api_get_hello_time (char * name, s_int32_t *hello_time)
{
  int ret = RESULT_ERROR;
  struct mstp_bridge *br = mstp_find_bridge (name);
  
  if (br != NULL) 
    {
      *hello_time = (br->bridge_hello_time) / L2_TIMER_SCALE_FACT;
      return RESULT_OK;
    }
  return ret;
}

/* Set the bridge dynamic ageing time (in seconds) */

int
mstp_api_set_ageing_time (char * name, s_int32_t ageing_time)
{
  struct mstp_bridge *br = mstp_find_bridge (name);
  int ret = RESULT_ERROR;
  if (br == NULL)
  {
    return ret;
  }

  /* send message to NSM */
  ret = mstp_nsm_send_ageing_time(name, ageing_time);
  if (ret != RESULT_OK)
    return ret;

  br->ageing_time = ageing_time;
  br->ageing_time_is_fwd_delay = PAL_FALSE;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* Get the bridge dynamic ageing time (in seconds) */
int
mstp_api_get_ageing_time (char * name, s_int32_t *ageing_time)
{
  struct mstp_bridge *br = mstp_find_bridge (name);
  int ret = RESULT_ERROR;
  if (br == NULL)
  {
    return ret;
  }

  *ageing_time = br->ageing_time;

  return RESULT_OK;
}

int
mstp_api_set_max_age (char * name, s_int32_t max_age)
{

  int ret = RESULT_ERROR;
  struct mstp_bridge * br = mstp_find_bridge (name);
  s_int32_t age;

  if (max_age < MSTP_MIN_BRIDGE_MAX_AGE || 
      max_age > MSTP_MAX_BRIDGE_MAX_AGE )
    return ret;

  age = max_age * L2_TIMER_SCALE_FACT;

  if ((br != NULL) 
      && mstp_is_maxage_ge_hello_time (age, br->bridge_hello_time)
      && mstp_is_fwd_delay_ge_maxage (br->bridge_forward_delay, age))
    {
      br->bridge_max_age = age;
       br->max_age_count++;
      if (mstp_bridge_is_cist_root (br))
        {
          br->cist_max_age = age;
          mstp_cist_port_role_selection (br);
          mstp_tx_bridge (br);
        }

#ifdef HAVE_HA
      mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

      ret = RESULT_OK;
    }
  return ret;
}

/* Get the bridge max age interval  (in secs) */
int
mstp_api_get_max_age (char * name, s_int32_t *max_age)
{
  int ret = RESULT_ERROR;
  struct mstp_bridge * br = mstp_find_bridge (name);

  if (br != NULL) 
    {
      *max_age = (br->bridge_max_age) / L2_TIMER_SCALE_FACT;
      ret = RESULT_OK;
    }
  return ret;
}

int
mstp_api_set_max_hops (char * name, s_int32_t max_hops)
{

  int ret = RESULT_ERROR;
  int instance = 0;
  struct mstp_bridge * br = mstp_find_bridge (name);
  struct mstp_port *port = NULL;
  struct mstp_instance_port *instance_port = NULL; 
  struct mstp_bridge_instance *br_inst = NULL;

  if (br == NULL)
    return RESULT_ERROR;
 
  if (max_hops < MSTP_MIN_BRIDGE_MAX_HOPS || 
      max_hops > MSTP_MAX_BRIDGE_MAX_HOPS )
    {
      return ret;
    }

  br->bridge_max_hops = max_hops;

  if (mstp_bridge_is_cist_root (br))
    {
      br->hop_count = max_hops;
      for (port = br->port_list; port; port = port->next)
        {
          port->hop_count = max_hops;
#ifdef HAVE_HA
          mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
        }
    }

  for (instance = 0; instance < br->max_instances; instance++)
    {
      if ((br_inst = br->instance_list[instance]))
        { 
          if (mstp_bridge_is_msti_root (br_inst))
            {
              br_inst->hop_count = max_hops;
#ifdef HAVE_HA
              mstp_cal_modify_mstp_bridge_instance (br_inst);
#endif /* HAVE_HA */
              for (instance_port = br_inst->port_list; instance_port; 
                   instance_port = instance_port->next)
                instance_port->hop_count = max_hops;
#ifdef HAVE_HA
                mstp_cal_modify_mstp_instance_port (instance_port);
#endif /* HAVE_HA */
            }
        }
    }
#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* Get max hops for a BPDU in an MST region */
int
mstp_api_get_max_hops (char * name, s_int32_t *max_hops)
{

  struct mstp_bridge * br = mstp_find_bridge (name);

  if (br == NULL)
    return RESULT_ERROR;
 
  *max_hops = br->bridge_max_hops;

  return RESULT_OK;
}

/* Set [unset] the port restricted role */
int
mstp_api_set_port_restricted_role (char *name, char * ifName,
                                   bool_t restricted_role)
{
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (! ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);

  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);

  if (!port)
    return RESULT_ERROR;

  port->restricted_role = restricted_role;

  mstp_cist_port_role_selection (port->br);
  mstp_tx_bridge (port->br);

  if (LAYER2_DEBUG(proto,PROTO))
    {
      zlog_debug (mstpm,"mstp api :port restricted role set ");
      zlog_debug (mstpm," port role is %d and state is %d ",
                  port->cist_role , port->cist_state);
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  return RESULT_OK;

}

/* Get the port restricted role */
int
mstp_api_get_port_restricted_role (char *name, char * ifName,
                                   s_int16_t *restricted_role)
{
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (! ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);

  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);

  if (!port)
    return RESULT_ERROR;

  *restricted_role = port->restricted_role;

  return RESULT_OK;

}

/* Set [unset] the port restricted tcn */
int
mstp_api_set_port_restricted_tcn (char *name, char * ifName,
                                  bool_t restricted_tcn)
{
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (! ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;

  port->restricted_tcn = restricted_tcn;

  if (LAYER2_DEBUG(proto,PROTO))
    {
      zlog_debug (mstpm,"mstp api :port restricted tcn set ");
      zlog_debug (mstpm," port role is %d and state is %d ",
                  port->cist_role , port->cist_state);
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* Get the port restricted tcn */
int
mstp_api_get_port_restricted_tcn (char *name, char * ifName,
                                  s_int16_t *restricted_tcn)
{
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (! ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;

  *restricted_tcn = port->restricted_tcn;

  return RESULT_OK;
}

/* Set the value of the  port hello time */
int
mstp_api_set_port_hello_time (char *name, char * ifName,
                              s_int32_t hello_time)
{
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;
  s_int32_t hello;
  s_int32_t prev_hello;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (! ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;

  hello = hello_time * L2_TIMER_SCALE_FACT;
  prev_hello = port->hello_time * L2_TIMER_SCALE_FACT;
  port->hello_time = hello;

  if (LAYER2_DEBUG(proto,PROTO))
    {
      zlog_debug (mstpm,"mstp api :port hello time set to "
                  "%d on port %u", hello_time, port->ifindex);
      zlog_debug (mstpm," port role is %d and state is %d ",
                  port->cist_role , port->cist_state);
    }

  if ((port->cist_role == ROLE_DESIGNATED))
    {
      port->cist_hello_time = port->hello_time;
      if (hello < prev_hello)
        {
          if (l2_is_timer_running (port->hello_timer))
            {
#ifdef HAVE_HA
              mstp_cal_delete_mstp_hello_timer (port);
#endif /* HAVE_HA */
              l2_stop_timer (&port->hello_timer);
              port->hello_timer =
                    l2_start_timer (mstp_hello_timer_handler,
                                    port, port->cist_hello_time, mstpm);
#ifdef HAVE_HA
              mstp_cal_create_mstp_hello_timer (port);
#endif /* HAVE_HA */
            }
        }
    }
#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  return RESULT_OK;

}

/* Get the value of the  port hello time */
int
mstp_api_get_port_hello_time (char *name, char * ifName,
                              s_int32_t *hello_time)
{
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (! ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;

  *hello_time = (port->hello_time) / L2_TIMER_SCALE_FACT;

  return RESULT_OK;
}

/* Section 8.8.5 */


/* Set[unset] the link type of the  port to point to point */
int
mstp_api_set_port_p2p (char *name, char * ifName, 
                       int is_p2p)
{

  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex; 

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (! ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;
  
  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;
  
  port->admin_p2p_mac = is_p2p;

 /* If is_p2p is auto check if the link is to be set shared or p2p */
  if (is_p2p == MSTP_ADMIN_LINK_TYPE_AUTO)
    {
     /* The check for Aggregator and if  members are aggregatable will be
        added when LACP is supported in MSTP */
      if (ifp->duplex == NSM_IF_FULL_DUPLEX)
        port->oper_p2p_mac = PAL_TRUE;
      else
        port->oper_p2p_mac = PAL_FALSE;
    }
  else
    port->oper_p2p_mac = is_p2p;

  if (LAYER2_DEBUG(proto,PROTO))
    {
      zlog_debug (mstpm,"mstp api :operation and admin p2p flags set to "
                  "%d on port %u", is_p2p, port->ifindex);
      zlog_debug (mstpm," port role is %d and state is %d ",
                  port->cist_role , port->cist_state);
    }
  
  if ((port->cist_role == ROLE_DESIGNATED) &&
      (port->cist_state != STATE_FORWARDING))
    mstp_cist_handle_designatedport_transition (port);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  return RESULT_OK;

}

/* Get the link type of the  port to point to point */
int
mstp_api_get_port_p2p (char *name, char * ifName, int *is_p2p)
{
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex; 

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (! ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;
  
  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;
  
  *is_p2p = port->admin_p2p_mac;
  
  return RESULT_OK;

}

/* Set[Unset] the port an edge port i.e. only connected to end stations  */
int
mstp_api_set_port_edge (char * name, char * ifName, 
                        int to_be_enabled, bool_t portfast_conf)
{

  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;
  
  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;
  
  port->oper_edge = port->admin_edge = (to_be_enabled ) ? PAL_TRUE : PAL_FALSE;

  port->portfast_conf = (portfast_conf) ? PAL_TRUE : PAL_FALSE;  
  
  if ((port->cist_role == ROLE_DESIGNATED) &&
      (port->cist_state != STATE_FORWARDING))
    mstp_cist_handle_designatedport_transition (port);

#ifdef HAVE_HA
   mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  return RESULT_OK;

}

/* Get the port an edge port i.e. only connected to end stations  */
int
mstp_api_get_port_edge (char * name, char * ifName, int *enabled)
{

  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;
  
  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;
  
  *enabled = port->oper_edge;

  return RESULT_OK;
}

/* Set[Unset] the port an auto port */

int
mstp_api_set_auto_edge (char * name, char * ifName,
                        int to_be_enabled)
{
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;

  port->auto_edge = to_be_enabled;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* Get the port auto edge info */
int
mstp_api_get_auto_edge (char * name, char * ifName,
                        int *enabled)
{
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;

  *enabled = port->auto_edge;

  return RESULT_OK;
}

int
mstp_api_set_bridge_forceversion (char *name,
                                  s_int32_t version)
{
  int index;
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct mstp_interface *mstpif = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);

  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  if ((version < BR_VERSION_STP) || (version > BR_VERSION_MAX)
       || (version == BR_VERSION_INVALID))
    return RESULT_ERROR;

  if (((br->type == MSTP_BRIDGE_TYPE_STP
          || br->type == MSTP_BRIDGE_TYPE_STP_VLANAWARE)
        && (version > BR_VERSION_STP))
          ||((br->type == MSTP_BRIDGE_TYPE_RSTP
          || br->type == MSTP_BRIDGE_TYPE_RSTP_VLANAWARE
          || br->type == MSTP_BRIDGE_TYPE_RPVST_PLUS)
            && (version > BR_VERSION_RSTP)))
    return RESULT_ERROR;

  for (port = br->port_list; port; port = port->next)
    {
      ifp = if_lookup_by_name (&vr->ifm, port->name);
      if (!ifp)
        return RESULT_ERROR;

      port->force_version = version;

      mstpif = ifp->info;
      mstpif->version = version;
      SET_FLAG (mstpif->config, MSTP_IF_CONFIG_FORCE_VERSION);

#ifdef HAVE_HA
      mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

      /* Run the protocol migration state machine */

      if (version < BR_VERSION_RSTP)
        mstp_sendstp (port);
      else
        mstp_sendmstp (port);
     }

  if (br->force_version >= BR_VERSION_MSTP
      && version < BR_VERSION_MSTP && (br->type != MSTP_BRIDGE_TYPE_RPVST_PLUS))
    {
      /* delete each of the instances */
      for (index = (br->max_instances- 1) ; index > 0 ;index--)
        {
          /* instance is not created */
          if (!br->instance_list[index])
            continue;

          mstp_delete_instance (br, index);
        }
    }

  br->force_version = version;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  return RESULT_OK;

}

int
mstp_api_set_bridge_mode (char *name, char *mode_str)
{
  int version;
  struct mstp_bridge *br;

  br = mstp_find_bridge (name);

  if (!br)
    return RESULT_ERROR;

  if (pal_strncmp (mode_str, "stp", pal_strlen ("stp")) == 0)
    {
      br->type = MSTP_BRIDGE_TYPE_STP_VLANAWARE;
      version = BR_VERSION_STP;
    }
  else if (pal_strncmp (mode_str, "rstp", pal_strlen ("rstp")) == 0)
    {
      br->type = MSTP_BRIDGE_TYPE_RSTP_VLANAWARE;
      version = BR_VERSION_RSTP;
    }
  else if (pal_strncmp (mode_str, "mstp", pal_strlen ("mstp")) == 0)
    {
      br->type = MSTP_BRIDGE_TYPE_MSTP;
      version = BR_VERSION_MSTP;
    }
   else if (pal_strncmp (mode_str, "rpvst", pal_strlen ("rpvst")) == 0)
    {
      br->type = MSTP_BRIDGE_TYPE_RPVST_PLUS;
      version = BR_VERSION_RSTP;
    }
  else if (pal_strncmp (mode_str, "provider-rstp",
                        pal_strlen ("provider-rstp")) == 0)
    {
      br->type = MSTP_BRIDGE_TYPE_PROVIDER_RSTP;
      version = BR_VERSION_RSTP;
    }
  else if (pal_strncmp (mode_str, "provider-mstp",
                        pal_strlen ("provider-mstp")) == 0)
    {
      br->type = MSTP_BRIDGE_TYPE_PROVIDER_MSTP;
      version = BR_VERSION_RSTP;
    }
  else
    return RESULT_ERROR;

  return mstp_api_set_bridge_forceversion (name, version);

}

int
mstp_api_set_port_forceversion (char *name, 
                                char *ifName, 
                                s_int32_t version)
{
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;
  
  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  /* Port version and Bridge version should always be the same */
  if (version != br->force_version)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;

  if ((version < BR_VERSION_STP) || (version > BR_VERSION_MAX))
    return RESULT_ERROR;
  
  if (((br->type == NSM_BRIDGE_TYPE_STP
        || br->type == NSM_BRIDGE_TYPE_STP_VLANAWARE) && (version > 0))
      ||((br->type == NSM_BRIDGE_TYPE_RSTP
          || br->type == NSM_BRIDGE_TYPE_RSTP_VLANAWARE
          || br->type == NSM_BRIDGE_TYPE_RPVST_PLUS) && (version > 2)))
    return RESULT_ERROR;

  port->force_version = version;
 
  /* Run the protocol migration state machine */ 
  if (version < BR_VERSION_RSTP)
    {
      l2_stop_timer(&port->migrate_timer);
      mstp_sendstp (port);
    }
  else
    mstp_sendmstp (port);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* Get port version */
int
mstp_api_get_port_forceversion (char *name, 
                                char *ifName, 
                                s_int32_t *version)
{
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;
  
  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;

  *version = port->force_version;
 
  return RESULT_OK;
}

/* Clear all the detected protocols */
int
mstp_api_mcheck (char *name, char *ifName)
{
  struct mstp_port *port= NULL;
  struct mstp_bridge *br;
  
  br = mstp_find_bridge (name);
  if (br == NULL)
    return RESULT_ERROR;
  
  if (ifName)
    {      
      struct interface *ifp;
      struct apn_vr *vr = apn_vr_get_privileged (mstpm);
      u_int32_t ifindex;

      ifp = if_lookup_by_name (&vr->ifm, ifName);
      if (!ifp)
        return RESULT_ERROR;

      ifindex = ifp->ifindex;

      port = mstp_find_cist_port (br, ifindex);
      if (port)
        {
          if (LAYER2_DEBUG(proto,TIMER))
            zlog_debug (mstpm,"Clearing detected protocols for port %u",
                        ifindex);
#ifdef HAVE_HA
          mstp_cal_delete_mstp_migrate_timer (port);
#endif /* HAVE_HA */

          l2_stop_timer (&port->migrate_timer);
          /* Run the state machine only if the force version is MSTP */
          if (port->force_version >= BR_VERSION_RSTP)
            mstp_sendmstp (port);
        }
      else
        return RESULT_ERROR;
    }
  else
    {
      BRIDGE_CIST_PORTLIST_LOOP(port, br)
        {

          if (LAYER2_DEBUG(proto,TIMER))
            zlog_debug (mstpm,"Clearing detected protocols for port %u",
                        port->ifindex);
#ifdef HAVE_HA
          mstp_cal_delete_mstp_migrate_timer (port);
#endif /* HAVE_HA */

          l2_stop_timer (&port->migrate_timer);

          if (port->force_version >= BR_VERSION_RSTP)
            mstp_sendmstp (port);
        }
        
    }

  return RESULT_OK;
}

/* Set CIST path cost for a port - Section 8.8.6 */
int
mstp_api_set_port_path_cost (char *name, char *ifName,
                             u_int16_t svid, u_int32_t cost)
{
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;
  
  br = mstp_find_bridge (name);

  if (br == NULL)
    return RESULT_ERROR;

  if (br->ce_bridge)
    port = mstp_find_ce_br_port (br, svid);
  else
    port = mstp_find_cist_port (br, ifindex);

  if (!port)
    return RESULT_ERROR;

  if (cost < MSTP_MIN_PATH_COST || cost > MSTP_MAX_PATH_COST)
    return RESULT_ERROR;


  port->cist_path_cost = cost;

  mstp_cist_port_role_selection (port->br);
  mstp_tx_bridge (port->br);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* Get CIST path cost for a port */
int
mstp_api_get_port_path_cost (char *name, char *ifName,
                             u_int16_t svid, u_int32_t *cost)
{
  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;
  
  br = mstp_find_bridge (name);

  if (br == NULL)
    return RESULT_ERROR;

  if (br->ce_bridge)
    port = mstp_find_ce_br_port (br, svid);
  else
    port = mstp_find_cist_port (br, ifindex);

  if (!port)
    return RESULT_ERROR;

  *cost = port->cist_path_cost;

  return RESULT_OK;
}

/* Section 8.8.5 */
int
mstp_api_set_port_priority (char *name, char *ifName,
                            u_int16_t svid, s_int16_t priority)
{
  u_int16_t port_id;
  struct mstp_bridge *br;
  struct mstp_port *port;  
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);

  if (br == NULL)
    return MSTP_ERR_BRIDGE_NOT_FOUND;

  if (br->ce_bridge)
    port = mstp_find_ce_br_port (br, svid);
  else
    port = mstp_find_cist_port (br, ifindex);

  if (port == NULL)
    return MSTP_ERR_PORT_NOT_FOUND;

  /* 802.1s Section-12.3j Port priority should be in multiples of 16*/
  if ((priority % MSTP_BRIDGE_PORT_PRIORITY_MULTIPLIER) != 0 )
    return MSTP_ERR_PRIORITY_VALUE_WRONG;

  if (priority < MSTP_MIN_PORT_PRIORITY || 
      priority > MSTP_MAX_PORT_PRIORITY )
    return MSTP_ERR_PRIORITY_OUTOFBOUNDS;

  port->cist_priority = priority & 0xff;

  if (BRIDGE_TYPE_CVLAN_COMPONENT (br))
    {
      if (port->port_type == MSTP_VLAN_PORT_MODE_CE)
        port_id = 0x0fff | ((port->cist_priority << 8) & 0xf000);
      else
        port_id = (port->svid & 0x0fff) |
                  ((port->cist_priority << 8) & 0xf000);
    }
  else
    port_id = mstp_cist_make_port_id (port);

  port->cist_port_id = port_id;

  mstp_cist_port_role_selection (port->br);
  mstp_tx_bridge (port->br);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* Get priority for a port */
int
mstp_api_get_port_priority (char *name, char *ifName,
                            u_int16_t svid, s_int16_t *priority)
{
  struct mstp_bridge *br;
  struct mstp_port *port;  
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);

  if (br == NULL)
    return MSTP_ERR_BRIDGE_NOT_FOUND;

  if (br->ce_bridge)
    port = mstp_find_ce_br_port (br, svid);
  else
    port = mstp_find_cist_port (br, ifindex);

  if (port == NULL)
    return MSTP_ERR_PORT_NOT_FOUND;

  *priority = port->cist_priority;

  return RESULT_OK;
}

/* This function updates the bridge priority to the requested value.
   It also updates the priority of the ports that use this bridge
   as the designated bridge.  The root bridge selection may change 
   as a result of calling this function.  Section 8.8.4 */

int
mstp_api_set_bridge_priority (char * name, u_int32_t new_priority)
{
  struct mstp_bridge * br = mstp_find_bridge (name);
  struct mstp_port * port;
  struct mstp_bridge_instance *br_inst = NULL;
  int instance = 0;
  int is_root;
  bool_t boundary_bridge = PAL_FALSE;
  
  if (br == NULL)
    return MSTP_ERR_BRIDGE_NOT_FOUND;

  /* 802.1q-2003 : Section 12.3 */
  if ((new_priority % MSTP_BRIDGE_PRIORITY_MULTIPLIER) != 0 )
    return MSTP_ERR_PRIORITY_VALUE_WRONG;

   /* Range checking. */
  if ( new_priority > MSTP_MAX_BRIDGE_PRIORITY )
    return MSTP_ERR_PRIORITY_OUTOFBOUNDS;
 
  /* Section 8.8.4.1 */
  is_root = mstp_bridge_is_cist_root (br);
  
  /* Section 8.8.4.2 */
  for (port = br->port_list; port; port = port->next)
    {
      if (port->cist_role != ROLE_DISABLED && mstp_cist_is_designated_port (port)) {
          mstp_put_prio_in_bridgeid (&(port->cist_designated_bridge),
                                     (u_int16_t)new_priority);
          port->reselect = PAL_TRUE;
          port->selected = PAL_FALSE;
          port->cist_info_type = INFO_TYPE_MINE;
#ifdef HAVE_HA
          mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
        }

      /* whether this is a boundary bridge or not for change of reg root */
      if (port->info_internal == PAL_FALSE || (IS_BRIDGE_RPVST_PLUS(br)))
        boundary_bridge = PAL_TRUE;
    }
  /* Section 8.8.4.3 */
  br->cist_bridge_priority = new_priority;
  mstp_put_prio_in_bridgeid ( &(br->cist_bridge_id),
                              (u_int16_t)new_priority);

  /* If the bridge is on a region boundary and the bridge priority changes
   * then this bridge should be made as regional root.
   *  As the regional root changes, for all the ports connected to within and
   * outside region, regional root has to be updated.
   * This makes proper selection of root port towards CIST ROOT outside
   * the region and makes proper computation of regional root. 
   * This is Not mentioned in the standard.
   */ 

  if (boundary_bridge)
    {
      pal_mem_cpy (&br->cist_reg_root, &br->cist_bridge_id,
                   MSTP_BRIDGE_ID_LEN);

      for (port = br->port_list; port; port = port->next)
        {
          pal_mem_cpy (&port->cist_reg_root, &br->cist_reg_root,
                       MSTP_BRIDGE_ID_LEN);
        }
    }
      
  mstp_cist_port_role_selection (br);

  for (instance = 0; instance < br->max_instances; instance++)
    {
      if ((br_inst = br->instance_list[instance]))
        {
          mstp_msti_port_role_selection (br_inst);
        }
    }
  
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
  else
    mstp_tx_bridge (br);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* Get bridge priority */
int
mstp_api_get_bridge_priority (char * name, u_int32_t *priority)
{
  struct mstp_bridge * br = mstp_find_bridge (name);
  
  if (br == NULL)
    return MSTP_ERR_BRIDGE_NOT_FOUND;

  *priority = br->cist_bridge_priority;

  return RESULT_OK;
}

/* Set [Unset] the instance restricted tcn */
int
mstp_api_set_msti_instance_restricted_role (char *name, char *ifName,
                                             int instance,
                                             bool_t restricted_role)
{
  struct mstp_bridge *br;
  struct mstp_bridge_instance *br_inst;
  struct mstp_instance_port *inst_port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
    if (!ifp)
      return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);

  if ((br == NULL) || (instance >= br->max_instances) ||
      (!(br_inst = br->instance_list[instance])))
    return RESULT_ERROR;

  inst_port = mstp_find_msti_port (br_inst, ifindex);

  if (!inst_port)
    return RESULT_ERROR;

  inst_port->restricted_role = restricted_role;

  mstp_msti_port_role_selection (inst_port->br);

  mstp_tx_bridge (br);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

  return CLI_SUCCESS;

}

/* Get the instance restricted tcn */
int
mstp_api_get_msti_instance_restricted_role (char *name, char *ifName,
                                             int instance,
                                             u_int32_t *restricted_role)
{
  struct mstp_bridge *br;
  struct mstp_bridge_instance *br_inst;
  struct mstp_instance_port *inst_port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
    if (!ifp)
      return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);

  if ((br == NULL) || (instance >= br->max_instances) || 
      (!(br_inst = br->instance_list[instance])))
    return RESULT_ERROR;

  inst_port = mstp_find_msti_port (br_inst, ifindex);

  if (!inst_port)
    return RESULT_ERROR;

  *restricted_role = inst_port->restricted_role;

  return RESULT_OK;
}


/* Set [Unset] the instance restricted tcn */
int
mstp_api_set_msti_instance_restricted_tcn (char *name, char *ifName,
                                           int instance,
                                           bool_t restricted_tcn)
{
  struct mstp_bridge *br;
  struct mstp_bridge_instance *br_inst;
  struct mstp_instance_port *inst_port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
    if (!ifp)
      return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);

  if ((br == NULL) || (instance >= br->max_instances) ||
      (!(br_inst = br->instance_list[instance])))
    return RESULT_ERROR;

  inst_port = mstp_find_msti_port (br_inst, ifindex);

  if (!inst_port)
    return RESULT_ERROR;

  inst_port->restricted_tcn = restricted_tcn;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */

  return CLI_SUCCESS;

}

/* Get the instance restricted tcn */
int
mstp_api_get_msti_instance_restricted_tcn (char *name, char *ifName,
                                           int instance,
                                           u_int32_t *restricted_tcn)
{
  struct mstp_bridge *br;
  struct mstp_bridge_instance *br_inst;
  struct mstp_instance_port *inst_port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
    if (!ifp)
      return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);

  if ((br == NULL) || (instance >= br->max_instances) ||
      (!(br_inst = br->instance_list[instance])))
    return RESULT_ERROR;

  inst_port = mstp_find_msti_port (br_inst, ifindex);

  if (!inst_port)
    return RESULT_ERROR;

  *restricted_tcn = inst_port->restricted_tcn;

  return RESULT_OK;
}


/* Set MSTI path cost for a port - Section 8.8.6 */

int
mstp_api_set_msti_port_path_cost (char *name, char *ifName,
                                  int instance,
                                  u_int32_t cost)
{
  struct mstp_bridge *br;
  struct mstp_bridge_instance *br_inst;
  struct mstp_instance_port *inst_port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
    if (!ifp)
      return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);

  if ((br == NULL) || (instance >= br->max_instances) ||
      (!(br_inst = br->instance_list[instance])))
    return RESULT_ERROR;

  inst_port = mstp_find_msti_port (br_inst, ifindex);

  if (!inst_port)
    return RESULT_ERROR;

  if (cost < MSTP_MIN_PATH_COST || cost > MSTP_MAX_PATH_COST)
    return RESULT_ERROR;

  inst_port->msti_path_cost = cost;
  if (cost != mstp_nsm_get_port_path_cost (mstpm, ifindex))
    inst_port->pathcost_configured = PAL_TRUE;
  else
    inst_port->pathcost_configured = PAL_FALSE;

  mstp_msti_port_role_selection (inst_port->br);

  mstp_tx_bridge (br);
   
#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
 
  return RESULT_OK;
}

/* Get MSTI path cost for a port - Section 8.8.6 */
int
mstp_api_get_msti_port_path_cost (char *name, char *ifName,
                                  int instance,
                                  u_int32_t *cost)
{
  struct mstp_bridge *br;
  struct mstp_bridge_instance *br_inst;
  struct mstp_instance_port *inst_port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
    if (!ifp)
      return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);

  if ((br == NULL) || (instance >= br->max_instances) ||
      (!(br_inst = br->instance_list[instance])))
    return RESULT_ERROR;

  inst_port = mstp_find_msti_port (br_inst, ifindex);

  if (!inst_port)
    return RESULT_ERROR;

  *cost = inst_port->msti_path_cost;
 
  return RESULT_OK;
}

/* Section 8.8.5 */
int
mstp_api_set_msti_port_priority (char *name, 
                                 char *ifName, int instance,
                                 s_int16_t priority)
{
  u_int16_t port_id;
  struct mstp_bridge *br;
  struct mstp_bridge_instance *br_inst;
  struct mstp_instance_port *inst_port;  
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);
  if (br == NULL) 
    return MSTP_ERR_BRIDGE_NOT_FOUND;

  if ( !(br_inst = br->instance_list[instance]))
    return MSTP_ERR_INSTANCE_NOT_FOUND;

  inst_port = mstp_find_msti_port (br_inst, ifindex);
  if (inst_port == NULL)
    return MSTP_ERR_PORT_NOT_FOUND;

  /* 802.1s Section-12.3j Port priority should be in multiples of 16*/
  if ((priority % MSTP_BRIDGE_PORT_PRIORITY_MULTIPLIER) != 0 )
    return MSTP_ERR_PRIORITY_VALUE_WRONG;
  
  if (priority < MSTP_MIN_PORT_PRIORITY || 
      priority > MSTP_MAX_PORT_PRIORITY )
    return MSTP_ERR_INSTANCE_OUTOFBOUNDS;

  inst_port->msti_priority = priority & 0xff;
  port_id = mstp_msti_make_port_id (inst_port);
  
  inst_port->msti_port_id = port_id;

  mstp_msti_port_role_selection (inst_port->br);

  mstp_tx_bridge (br);

#ifdef HAVE_HA
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
 
  return RESULT_OK;
}

/* Get MSTI port priority */
int
mstp_api_get_msti_port_priority (char *name, 
                                 char *ifName, int instance,
                                 s_int16_t *priority)
{
  struct mstp_bridge *br;
  struct mstp_bridge_instance *br_inst;
  struct mstp_instance_port *inst_port;  
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);
  if (br == NULL) 
    return MSTP_ERR_BRIDGE_NOT_FOUND;

  if ( !(br_inst = br->instance_list[instance]))
    return MSTP_ERR_INSTANCE_NOT_FOUND;

  inst_port = mstp_find_msti_port (br_inst, ifindex);
  if (inst_port == NULL)
    return MSTP_ERR_PORT_NOT_FOUND;

  *priority = inst_port->msti_priority;
 
  return RESULT_OK;
}

/* This function updates the bridge priority to the requested value.
   It also updates the priority of the ports that use this bridge
   as the designated bridge.  The root bridge selection may change 
   as a result of calling this function.  Section 8.8.4 */

int
mstp_api_set_msti_bridge_priority (char * name, int instance,
                                   u_int32_t new_priority)
{
  struct mstp_bridge * br = mstp_find_bridge (name);
  struct mstp_bridge_instance *br_inst = NULL;
  struct mstp_instance_port * inst_port = NULL;
  int is_root;
  
  if (br == NULL)
    return MSTP_ERR_BRIDGE_NOT_FOUND;

  if (instance < MST_MIN_INSTANCES || instance >= br->max_instances)
    return MSTP_ERR_INSTANCE_OUTOFBOUNDS; 

  if (!(br_inst = br->instance_list[instance]))
    return MSTP_ERR_INSTANCE_NOT_FOUND;

  /* 802.1q-2003 : Section 12.3 */
  if ((new_priority % MSTP_BRIDGE_PRIORITY_MULTIPLIER) != 0 )
    return MSTP_ERR_PRIORITY_VALUE_WRONG;

   /* Range checking. */
  if ( new_priority > MSTP_MAX_BRIDGE_PRIORITY )
    return MSTP_ERR_PRIORITY_OUTOFBOUNDS;

  /* Section 8.8.4.1 */
  is_root = mstp_bridge_is_msti_root (br_inst);

  
  /* Section 8.8.4.2 */
  for (inst_port = br_inst->port_list; inst_port; inst_port = inst_port->next)
    {
      if (inst_port->role != ROLE_DISABLED && 
                 mstp_msti_is_designated_port (inst_port))
        {
          mstp_put_prio_in_bridgeid (&(inst_port->designated_bridge),
                                     (u_int16_t)new_priority);
          inst_port->reselect = PAL_TRUE;
          inst_port->selected = PAL_FALSE;
          inst_port->info_type = INFO_TYPE_MINE; 
        }
    }
  /* Section 8.8.4.3 */
  br_inst->msti_bridge_priority = new_priority;
  mstp_put_prio_in_bridgeid ( &(br_inst->msti_bridge_id),
                              (u_int16_t)new_priority);

  mstp_msti_port_role_selection (br_inst);

  mstp_tx_bridge (br);
#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge_instance (br_inst);
  mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
  return RESULT_OK;
}

/* Get bridge instance priority */
int
mstp_api_get_msti_bridge_priority (char * name, int instance,
                                   u_int32_t *priority)
{
  struct mstp_bridge * br = mstp_find_bridge (name);
  struct mstp_bridge_instance *br_inst;
  
  if (br == NULL)
    return MSTP_ERR_BRIDGE_NOT_FOUND;

  if (instance < MST_MIN_INSTANCES || instance >= br->max_instances)
    return MSTP_ERR_INSTANCE_OUTOFBOUNDS; 

  if (!(br_inst = br->instance_list[instance]))
    return MSTP_ERR_INSTANCE_NOT_FOUND;

  *priority = br_inst->msti_bridge_priority;

  return RESULT_OK;
}

int
mstp_api_enable_bridge (char * name, u_int8_t br_type)
{
  s_int32_t ret;
  struct nsm_msg_bridge_enable msg;
  struct mstp_bridge * br = mstp_find_bridge (name);

  if (br == NULL)
    {
      zlog_err (mstpm,"Bridge %s Not Found ",name);
      return RESULT_ERROR;
    }

  pal_mem_set(&msg, 0, sizeof(struct nsm_msg_bridge_enable));

#ifdef HAVE_SMI
  if(br_type != SMI_DONT_CHK_TYPE)
#endif /* HAVE_SMI */
  if ((br_type != BRIDGE_TYPE_XSTP) &&
      (((IS_BRIDGE_MSTP (br)) && (br_type != NSM_BRIDGE_TYPE_MSTP)) ||
       ((IS_BRIDGE_RSTP (br)) && (br_type != NSM_BRIDGE_TYPE_RSTP)) ||
       ((IS_BRIDGE_STP (br)) && (br_type != NSM_BRIDGE_TYPE_STP)) ||
       ((IS_BRIDGE_RPVST_PLUS (br)) && (br_type != NSM_BRIDGE_TYPE_RPVST_PLUS))))
    {
      zlog_err (mstpm, "Bridge type mismatch\n");
      return RESULT_ERROR;
    }

  ret = mstp_enable_bridge (br);

  if (ret < 0)
    return ret;

  pal_strncpy(msg.bridge_name, br->name, NSM_BRIDGE_NAMSIZ);
  msg.enable = PAL_TRUE;

  return nsm_client_send_bridge_enable_msg (mstpm->nc, &msg);
}

int
mstp_api_disable_bridge (char * name, u_int8_t br_type, bool_t bridge_forward)
{
  s_int32_t ret;
  struct nsm_msg_bridge_enable msg;
  struct mstp_bridge * br = mstp_find_bridge (name);

  if (br == NULL)
    {
      zlog_err (mstpm,"Bridge %s Not Found ",name);
      return RESULT_ERROR;
    }

  pal_mem_set(&msg, 0, sizeof(struct nsm_msg_bridge_enable));

#ifdef HAVE_SMI
  if(br_type != SMI_DONT_CHK_TYPE)
#endif /* HAVE_SMI */
  if ((br_type != BRIDGE_TYPE_XSTP) &&
      (((IS_BRIDGE_MSTP (br)) && (br_type != NSM_BRIDGE_TYPE_MSTP)) ||
       ((IS_BRIDGE_RSTP (br)) && (br_type != NSM_BRIDGE_TYPE_RSTP)) ||
       ((IS_BRIDGE_RPVST_PLUS (br)) && (br_type != NSM_BRIDGE_TYPE_RPVST_PLUS))
       || ((IS_BRIDGE_STP (br)) && (br_type != NSM_BRIDGE_TYPE_STP))))
    {
      zlog_err (mstpm, "Bridge type mismatch\n");
      return RESULT_ERROR;
    }

  ret = mstp_disable_bridge (br, bridge_forward);

  if (ret < 0)
    return ret;

  pal_strncpy(msg.bridge_name, br->name, NSM_BRIDGE_NAMSIZ);
  msg.enable = PAL_FALSE;

  return nsm_client_send_bridge_enable_msg (mstpm->nc, &msg);

}


/* This function is called when a vlan is added using the vlan cli
 * create a new entry in the cist vlan list */
int
mstp_vlan_add_event (char * name , u_int16_t vid,
                     u_int16_t fid)
{
  struct mstp_bridge *br = mstp_find_bridge (name);

  /* later when vid type is resolved we can simply call the 
   * mstp_add_vlans function but till then we have to duplicate the work 
   * here. */
  if (!br)
    return RESULT_ERROR;

  mstp_rlist_add (&br->vlan_list, vid, 0);
  /* mstp_rlist_add (&br->fid_list, fid); */
  return RESULT_OK;

}


/* This function is called when a vlan is deleted using the vlan cli
 * delete the entry in the cist and msti vlan list */
int
mstp_vlan_delete_event (char * name , u_int32_t vid)
{
  int ret, instance_index;
  struct mstp_bridge *br = mstp_find_bridge (name);
  struct mstp_bridge_instance *br_inst;
  u_int16_t invalid_range_idx = 0;

  /* later when vid type is resolved we can simply call the 
   * mstp_add_vlans function but till then we have to duplicate the work 
   * here. */
  if (!br)
    return RESULT_ERROR;
 
  /* delete cist & msti vlan list */
  ret = mstp_rlist_delete (&br->vlan_list, vid, &invalid_range_idx);

  if (!ret)
    return RESULT_ERROR;

  for (instance_index = 1; instance_index < br->max_instances; instance_index++)
    {
      if (!br->instance_list[instance_index])
        continue;

      br_inst = br->instance_list[instance_index];
      ret = mstp_rlist_delete (&br_inst->vlan_list, vid, &invalid_range_idx);
    }

  mstp_gen_cfg_digest (br);

  return ret;
}

/* This function sets reselect = FALSE for all Ports
 * on the bridge */
int
mstp_msti_clear_reselect_bridge (struct mstp_bridge_instance *br_inst)
{
  struct mstp_instance_port *inst_port = NULL;
  pal_assert (br_inst);
 
  for (inst_port = br_inst->port_list; inst_port; 
                            inst_port = inst_port->next) {
    inst_port->reselect = PAL_FALSE;
#ifdef HAVE_HA
    mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
  }
  return PAL_FALSE;
}

/* This function check is any port hase reselect = TRUE
 * on the bridge */
int
mstp_msti_check_reselect_bridge (struct mstp_bridge_instance *br_inst)
{
  struct mstp_instance_port *inst_port;
  pal_assert (br_inst);
 
  for (inst_port = br_inst->port_list; inst_port; 
                            inst_port = inst_port->next) {
    if (inst_port->reselect == PAL_TRUE)
      return PAL_TRUE;
  }

  return PAL_FALSE;
}
/* This function sets the selected flag for all ports in 
   reselect = FALSE on all ports on the bridge */
void
mstp_msti_set_selected_bridge (struct mstp_bridge_instance *br_inst)
{
  struct mstp_instance_port *inst_port = NULL;
  pal_assert (br_inst);

  if (mstp_msti_check_reselect_bridge(br_inst) == PAL_FALSE) { 
    for (inst_port = br_inst->port_list; inst_port; 
                              inst_port = inst_port->next) 
      {
        inst_port->selected = PAL_TRUE;
#ifdef HAVE_HA
        mstp_cal_modify_mstp_instance_port (inst_port);
#endif /* HAVE_HA */
      }
  }
}

/* This function sets reselect = FALSE for all Ports
 * on the bridge */
int
mstp_cist_clear_reselect_bridge (struct mstp_bridge *br)
{
  struct mstp_port *port;
  pal_assert (br);
 
  for (port = br->port_list; port; port = port->next) {
    port->reselect = PAL_FALSE;
#ifdef HAVE_HA
    mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
  }
  return PAL_FALSE;
}

/* This function check is any port hase reselect = TRUE
 * on the bridge */
int
mstp_cist_check_reselect_bridge (struct mstp_bridge *br)
{
  struct mstp_port *port;
  pal_assert (br);
 
  for (port = br->port_list; port; port = port->next) {
    if (port->reselect == PAL_TRUE)
      return PAL_TRUE;
  }

  return PAL_FALSE;
}

/* This function sets the selected flag for all ports in 
   reselect = FALSE on all ports on the bridge */
void
mstp_cist_set_selected_bridge (struct mstp_bridge *br)
{
  struct mstp_port *port;
  pal_assert (br);

  if (mstp_cist_check_reselect_bridge(br) == PAL_FALSE) { 
    for (port = br->port_list; port; port = port->next){ 
      port->selected = PAL_TRUE;
#ifdef HAVE_HA
      mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
    }
  }
}

/* enable/disable the portfast bpdu gurad on the specified port */

int mstp_api_set_bridge_portfast_bpduguard (char *br_name, 
                                            bool_t bpduguard_enabled)
{
  struct mstp_bridge *br = NULL;
  struct mstp_port *port = NULL;

  br = mstp_find_bridge (br_name);
  if (!br)
    return RESULT_ERROR;

  if (bpduguard_enabled)
    br->bpduguard = PAL_TRUE;
  else
    br->bpduguard = PAL_FALSE;

  /* enable bpdu-guard on all the ports which have their bpdu-guard set to
   * default.
   */
  for (port = br->port_list; port; port = port->next)
    {
      if (port->admin_bpduguard == MSTP_PORT_PORTFAST_BPDUGUARD_DEFAULT)
        {
          port->oper_bpduguard = br->bpduguard;
#ifdef HAVE_HA
          mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
        }
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* Get the portfast bpdu gurad on the specified port */
int 
mstp_api_get_bridge_portfast_bpduguard (char *br_name, 
                                        int *bpduguard_enabled)
{
  struct mstp_bridge *br = NULL;

  br = mstp_find_bridge (br_name);
  if (!br)
    return RESULT_ERROR;

  *bpduguard_enabled = br->bpduguard;

  return RESULT_OK;
}

/* Set the value of errdisable-timeout interval */

int mstp_api_set_bridge_errdisable_timeout_enable (char *br_name, 
                                                   bool_t enabled)
{
  struct mstp_bridge *br = NULL;

  br = mstp_find_bridge (br_name);
  if (!br)
    return RESULT_ERROR;
    
  br->errdisable_timeout_enable = enabled;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  return RESULT_OK;

}

/* Get the value of errdisable-timeout enable */
int 
mstp_api_get_bridge_errdisable_timeout_enable (char *br_name, 
                                               int *enabled)
{
  struct mstp_bridge *br = NULL;

  br = mstp_find_bridge (br_name);
  if (!br)
    return RESULT_ERROR;
    
  *enabled = br->errdisable_timeout_enable;

  return RESULT_OK;
}

/* Set the value of errdisable-timeout interval */

int mstp_api_set_bridge_errdisable_timeout_interval (char *br_name, 
                                                     s_int32_t timeout)
{
  struct mstp_bridge *br = NULL;

  br = mstp_find_bridge (br_name);
  if (!br)
    return RESULT_ERROR;

  br->errdisable_timeout_interval = timeout*L2_TIMER_SCALE_FACT;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* Get the value of errdisable-timeout interval */
int 
mstp_api_get_bridge_errdisable_timeout_interval (char *br_name, 
                                                 s_int32_t *timeout)
{
  struct mstp_bridge *br = NULL;

  br = mstp_find_bridge (br_name);
  if (!br)
    return RESULT_ERROR;

  *timeout = (br->errdisable_timeout_interval) / L2_TIMER_SCALE_FACT;

  return RESULT_OK;
}


/* enable/disable the portfast bpdu filtering on the specified port */

int mstp_api_set_bridge_portfast_bpdufilter (char *br_name, bool_t enabled)
{
  struct mstp_bridge *br = NULL;
  struct mstp_port *port = NULL;

  br = mstp_find_bridge (br_name);
  if (!br)
    return RESULT_ERROR;

  if (enabled)
    br->bpdu_filter = PAL_TRUE;
  else
    br->bpdu_filter = PAL_FALSE;

  /* enable bpdu-filter on all the ports which have their bpdu-filter set to
   * default.
   */
  for (port = br->port_list; port; port = port->next)
    {
      if (port->admin_bpdufilter == MSTP_PORT_PORTFAST_BPDUFILTER_DEFAULT)
        {
          port->oper_bpdufilter = br->bpdu_filter;
#ifdef HAVE_HA
          mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */
        }
    }

#ifdef HAVE_HA
  mstp_cal_modify_mstp_bridge (br);
#endif /* HAVE_HA */

  return RESULT_OK;
}

/* Get portfast bpdu filtering on the specified port */
int 
mstp_api_get_bridge_portfast_bpdufilter (char *br_name, int *enabled)
{
  struct mstp_bridge *br = NULL;

  br = mstp_find_bridge (br_name);
  if (!br)
    return RESULT_ERROR;

  *enabled = br->bpdu_filter;

  return RESULT_OK;
}

/* Set the portfast bpduguard value for the cist port */
int mstp_api_set_port_bpduguard (char * name, char * ifName,
                                 u_char portfast_bpduguard)
{
  struct mstp_bridge *br = NULL;
  struct mstp_port *port = NULL;
  struct interface *ifp = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;

  port->admin_bpduguard = portfast_bpduguard;

  /* If the bpduguard for the port is set to default then it will be
   * overwritten by the bridge bpduguard value.
   */

  if (portfast_bpduguard == MSTP_PORT_PORTFAST_BPDUGUARD_ENABLED)
    port->oper_bpduguard = PAL_TRUE;
  else if (portfast_bpduguard == MSTP_PORT_PORTFAST_BPDUGUARD_DISABLED)
    port->oper_bpduguard = PAL_FALSE;
  else
    port->oper_bpduguard = br->bpduguard;

#ifdef HAVE_HA
    mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  return RESULT_OK;

}

/* Get the portfast bpduguard value for the cist port */
int 
mstp_api_get_port_bpduguard (char * name, char * ifName,
                             u_char *portfast_bpduguard)
{
  struct mstp_bridge *br = NULL;
  struct mstp_port *port = NULL;
  struct interface *ifp = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;

  *portfast_bpduguard = port->admin_bpduguard;

  return RESULT_OK;
}

/* Set/Unset Root guard for the port */

int mstp_api_set_port_rootguard (char * name,
                                 char * ifName,
                                 bool_t enabled)
{
  struct mstp_bridge *br = NULL;
  struct mstp_port *port = NULL;
  struct interface *ifp = NULL;
  struct mstp_instance_port *inst_port  = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;

  if (port->admin_rootguard == enabled)
    return RESULT_OK;

  port->admin_rootguard = enabled;


  if (port->admin_rootguard)
    {
      if (LAYER2_DEBUG (proto, PROTO))
        zlog_debug (mstpm, "mstp_cist_process_message: "
                    "received a superior bpdu on a root guard enabled"
                    "port %u. setting port state to root-inconsistent",
                    port->ifindex);

      if ((port->cist_role == ROLE_ROOTPORT)
          || (port->cist_role == ROLE_ALTERNATE)
          || (port->cist_role == ROLE_BACKUP))
        {
          port->oper_rootguard = PAL_TRUE;
          port->cist_info_type = INFO_TYPE_AGED;
          port->reselect = PAL_TRUE;
          port->selected = PAL_FALSE;

          mstp_cist_port_role_selection (port->br);
          if (!port->info_internal)
            {
              inst_port = port->instance_list;

              while (inst_port)
                {
                  mstp_msti_port_role_selection (inst_port->br);
                  inst_port = inst_port->next_inst;
                }
            }
        }
    }
  else
    port->oper_rootguard = PAL_FALSE;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

   mstp_tx_bridge (port->br);

   return RESULT_OK;

}

/* Get Root guard for the port */
int 
mstp_api_get_port_rootguard (char * name,
                             char * ifName,
                             s_int32_t *enabled)
{
  struct mstp_bridge *br = NULL;
  struct mstp_port *port = NULL;
  struct interface *ifp = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;

  *enabled = port->admin_rootguard;

   return RESULT_OK;
}


/* Set the portfast bpduguard value for the cist port */
int mstp_api_set_port_bpdufilter (char * name,
                                  char * ifName,
                                  u_char portfast_bpdufilter)
{
  struct mstp_bridge *br = NULL;
  struct mstp_port *port = NULL;
  struct interface *ifp = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;

  port->admin_bpdufilter = portfast_bpdufilter;

  /* If the bpdu filtering for the port is set to default then it will be
   * overwritten by the bridge bpdu filtering value.
   */

  if (portfast_bpdufilter == MSTP_PORT_PORTFAST_BPDUFILTER_ENABLED)
    port->oper_bpdufilter = PAL_TRUE;
  else if (portfast_bpdufilter == MSTP_PORT_PORTFAST_BPDUFILTER_DISABLED)
    port->oper_bpdufilter = PAL_FALSE;
  else
    port->oper_bpdufilter = br->bpdu_filter;

#ifdef HAVE_HA
  mstp_cal_modify_mstp_port (port);
#endif /* HAVE_HA */

  return RESULT_OK;

}

/* Get the portfast bpduguard value for the cist port */
int 
mstp_api_get_port_bpdufilter (char * name,
                              char * ifName,
                              u_char *portfast_bpdufilter)
{
  struct mstp_bridge *br = NULL;
  struct mstp_port *port = NULL;
  struct interface *ifp = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifName);
  if (!ifp)
    return RESULT_ERROR;

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  port = mstp_find_cist_port (br, ifindex);
  if (!port)
    return RESULT_ERROR;

  *portfast_bpdufilter = port->admin_bpdufilter;

  return RESULT_OK;
}

void
mstp_set_msti_port_path_cost (struct mstp_port *port)
{
  struct mstp_instance_port *inst_port;

  inst_port = NULL;

  if (!port)
    return;

  inst_port = port->instance_list;
  while (inst_port)
    {
      if (!inst_port->pathcost_configured)
        mstp_msti_set_port_path_cost(inst_port,
                mstp_nsm_get_port_path_cost (mstpm,inst_port->ifindex));
      inst_port = inst_port->next_inst;
    }
}

void
mstp_nsm_send_info (bool_t state, struct mstp_port *port)
{
  struct nsm_msg_bridge_port msg;
  
  /* Send messsage to nsm to set port state and learn. */
  pal_mem_set(&msg, 0, sizeof(struct nsm_msg_bridge_port));
  msg.cindex = 0;
  msg.ifindex = port->ifindex;
  msg.num = 1;
  msg.instance = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));
  msg.state = XCALLOC(MTYPE_TMP, msg.num * sizeof(u_int32_t));
  /*For CIST, the instance is 0 */
  if (state == PAL_TRUE)
    msg.state[0] = NSM_BRIDGE_PORT_STATE_ENABLED;
  else
    msg.state[0] = NSM_BRIDGE_PORT_STATE_DISABLED;

  pal_strncpy(msg.bridge_name, port->br->name, NSM_BRIDGE_NAMSIZ);
  NSM_SET_CTYPE(msg.cindex, NSM_BRIDGE_CTYPE_PORT_STATE);

  nsm_client_send_bridge_port_msg(mstpm->nc, &msg,
                                  NSM_MSG_BRIDGE_PORT_STATE);
  XFREE(MTYPE_TMP, msg.instance);
  XFREE(MTYPE_TMP, msg.state);
}

int
mstp_api_get_pathcost_method (char *br_name)
{
  struct mstp_bridge *br = mstp_find_bridge (br_name);

  if (!br)
    return RESULT_ERROR;

  if (br->path_cost_method == MSTP_PATHCOST_SHORT)
    return MSTP_PATHCOST_SHORT;
  else if (br->path_cost_method == MSTP_PATHCOST_LONG)
    return MSTP_PATHCOST_LONG;
  else
    return RESULT_ERROR;
}

int
mstp_api_set_pathcost_method (char *br_name, u_int8_t path_cost_method)
{
  struct mstp_bridge *br = mstp_find_bridge (br_name);
  struct mstp_port *port;
  struct mstp_instance_port *inst_port = NULL;

  if (!br)
    return RESULT_ERROR;

  if (path_cost_method == MSTP_PATHCOST_DEFAULT)
    {
      if (br->type == MSTP_BRIDGE_TYPE_STP
          || br->type == MSTP_BRIDGE_TYPE_STP_VLANAWARE)
        {
          if (br->path_cost_method == MSTP_PATHCOST_LONG)
            br->path_cost_method = MSTP_PATHCOST_SHORT;
          else
            return MSTP_ERR;
        }
      else
        {
          if (br->path_cost_method == MSTP_PATHCOST_SHORT)
            br->path_cost_method = MSTP_PATHCOST_LONG;
          else
            return MSTP_ERR;
        }
    }
  else
    {
      if (br->path_cost_method != path_cost_method)
        br->path_cost_method = path_cost_method;
      else
        return MSTP_ERR;
    }

   /* Set the new path cost method for all ports */
   for (port = br->port_list; port; port = port->next)
    {
      mstp_set_cist_port_path_cost (port);

     /* Set the new path cost method for all instances of port */
     for (inst_port = port->instance_list; inst_port ; 
                                inst_port = inst_port->next_inst)
       mstp_msti_set_port_path_cost(inst_port,
                mstp_nsm_get_port_path_cost (mstpm,inst_port->ifindex));  
    }
  return RESULT_OK;
}

#ifdef  HAVE_SMI

int
mstp_api_bridge_change_type(char * name, enum smi_bridge_type type,
                            enum smi_bridge_topo_type topo_type)
{
  struct mstp_bridge *br = NULL;

  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  br->type = type;
  br->topology_type = topo_type;

  return RESULT_OK;
}


/* Get spanning tree details of the specified bridge */
int
mstp_api_get_spanning_tree_details(char * name, 
                                   struct smi_mstp_spanning_tree_details 
                                   *stdetails)
{
  struct mstp_bridge *br = NULL;
  int index;
 
  br = mstp_find_bridge (name);
  if (!br)
    return RESULT_ERROR;

  pal_mem_set(stdetails, 0, sizeof(struct smi_mstp_spanning_tree_details));

  pal_snprintf(stdetails->bridge.name, SMI_BRIDGE_NAMSIZ, br->name);
  pal_mem_cpy(&(stdetails->bridge.config), &(br->config),
              sizeof(struct smi_mstp_config_details));
  stdetails->bridge.type = br->type;
  pal_mem_cpy(&(stdetails->bridge.bridge_addr), &(br->bridge_addr), 
              sizeof(struct smi_mac_addr));
  pal_mem_cpy(&(stdetails->bridge.cist_bridge_id), &(br->cist_bridge_id), 
              sizeof(struct smi_bridge_id));
  pal_mem_cpy(&(stdetails->bridge.cist_designated_root), 
              &(br->cist_designated_root), 
              sizeof(struct smi_bridge_id));
  pal_mem_cpy(&(stdetails->bridge.cist_reg_root), &(br->cist_reg_root), 
              sizeof(struct smi_bridge_id));
  stdetails->bridge.external_root_path_cost = br->external_root_path_cost;
  stdetails->bridge.cist_root_port_id = br->cist_root_port_id;
  stdetails->bridge.cist_root_port_ifindex = br->cist_root_port_ifindex;
  stdetails->bridge.cist_max_age = br->cist_max_age;
  stdetails->bridge.cist_message_age = br->cist_message_age;
  stdetails->bridge.cist_hello_time = br->cist_hello_time;
  stdetails->bridge.cist_forward_delay = br->cist_forward_delay;
  stdetails->bridge.hop_count = br->hop_count;
  stdetails->bridge.force_version = br->force_version;
  stdetails->bridge.bridge_max_age = br->bridge_max_age;
  stdetails->bridge.bridge_hello_time = br->bridge_hello_time;
  stdetails->bridge.bridge_forward_delay = br->bridge_forward_delay;
  stdetails->bridge.bridge_max_hops = br->bridge_max_hops;
  stdetails->bridge.cist_bridge_priority = br->cist_bridge_priority;
  stdetails->bridge.ageing_time = br->ageing_time;
  stdetails->bridge.topology_change = br->topology_change;
  stdetails->bridge.topology_change_detected = br->topology_change_detected;
  stdetails->bridge.time_last_topo_change = br->time_last_topo_change;
  stdetails->bridge.num_topo_changes = br->num_topo_changes;
  stdetails->bridge.bridge_enabled = br->bridge_enabled;
  stdetails->bridge.is_default = br->is_default;
  stdetails->bridge.mstp_enabled = br->mstp_enabled;
  stdetails->bridge.bpduguard = br->bpduguard;
  stdetails->bridge.errdisable_timeout_interval = 
                    br->errdisable_timeout_interval;
  stdetails->bridge.errdisable_timeout_enable = br->errdisable_timeout_enable;
  stdetails->bridge.bpdu_filter = br->bpdu_filter;
  stdetails->bridge.transmit_hold_count = br->transmit_hold_count;
  stdetails->bridge.oper_cisco = br->oper_cisco;

  SMI_BMP_INIT (stdetails->bridge.instanceBmp);
  stdetails->bridge.num_instances = 0;

  for (index = 0; index < br->max_instances; index++)
  {
    /* instance not created */
    if (!br->instance_list[index])
      continue;
    SMI_BMP_SET (stdetails->bridge.instanceBmp, index);
    stdetails->bridge.num_instances++;
  }

  return RESULT_OK;
}

/* Get spanning tree details of the bridge of the specified interface */
int
mstp_api_get_spanning_tree_details_interface(struct interface *ifp, 
                                             struct smi_mstp_port_details 
                                             *port_details)
{
  struct mstp_bridge *br = NULL;
  struct mstp_port *port = NULL;

  br = mstp_find_bridge (ifp->bridge_name);
  if(!br)
    return RESULT_ERROR;

  for (port = br->port_list; port;port = port->next)
  {
    if (port->ifindex == ifp->ifindex)
    {
      pal_snprintf(port_details->name, BRIDGE_NAMESIZ, ifp->bridge_name);
      pal_snprintf(port_details->ifname, INTERFACE_NAMSIZ, ifp->name);
      port_details->ifindex = port->ifindex;
      port_details->force_version = port->force_version;
      port_details->admin_p2p_mac = port->admin_p2p_mac;
      port_details->oper_p2p_mac = port->oper_p2p_mac;
      port_details->admin_edge = port->admin_edge;
      port_details->oper_edge = port->oper_edge;
      port_details->auto_edge = port->auto_edge;
      port_details->cist_port_id = port->cist_port_id;
      port_details->cist_external_rpc = port->cist_external_rpc;
      port_details->cist_internal_rpc = port->cist_internal_rpc;
      port_details->cist_path_cost = port->cist_path_cost;
      port_details->cist_designated_port_id = port->cist_designated_port_id;
      port_details->cist_priority = port->cist_priority;
      pal_mem_cpy(&(port_details->cist_designated_bridge), 
                  &(port->cist_designated_bridge), 
                  sizeof(struct smi_bridge_id));

      if (port->br->type == NSM_BRIDGE_TYPE_STP_VLANAWARE)
        {
          if (port->cist_role == ROLE_DISABLED)
            {
              port_details->cist_state = SMI_STATE_DISABLED;
            }  
          
          else if (port->cist_role == ROLE_BACKUP
                   || port->cist_role == ROLE_ALTERNATE)
            { 
              port_details->cist_state = SMI_STATE_BLOCKING;
            }  
          
          else if ((port->cist_state == STATE_DISCARDING)
                   && (port->cist_role == ROLE_DESIGNATED
                    || port->cist_role == ROLE_ROOTPORT))
            {   
              port_details->cist_state = SMI_STATE_LISTENING;           
            }   
          else 
            {
              if (port->cist_state == SMI_STP_PORT_STATE_0)
                 port_details->cist_state = SMI_STATE_BLOCKING;
              else if (port->cist_state == SMI_STP_PORT_STATE_1)
                 port_details->cist_state = SMI_STATE_LISTENING;
              else if (port->cist_state == SMI_STP_PORT_STATE_2)
                 port_details->cist_state = SMI_STATE_LEARNING;
              else if (port->cist_state == SMI_STP_PORT_STATE_3)
                 port_details->cist_state = SMI_STATE_FORWARDING;
              else if (port->cist_state == SMI_STP_PORT_STATE_4)
                 port_details->cist_state = SMI_STATE_BLOCKING;
              else if (port->cist_state > SMI_STP_PORT_STATE_4)
                 port_details->cist_state = SMI_STATE_ERROR;
             }
       } /* Bridge is STP */
     else
       {
         switch (port->cist_state)
          {
            case SMI_STP_PORT_STATE_0:
              port_details->cist_state = SMI_STATE_DISCARDING;
            break;
            case SMI_STP_PORT_STATE_1:
              port_details->cist_state = SMI_STATE_LISTENING;
            break;
            case SMI_STP_PORT_STATE_2:
              port_details->cist_state = SMI_STATE_LEARNING;
            break;
            case SMI_STP_PORT_STATE_3:
              port_details->cist_state = SMI_STATE_FORWARDING;
            break;
            case SMI_STP_PORT_STATE_4:
              port_details->cist_state = SMI_STATE_BLOCKING;
            break;
            default:
             port_details->cist_state = SMI_STATE_ERROR;
            break;
          }
       } /*Else Bridge */
      break;
    }
  }
  return RESULT_OK;
}

/* Get spanning tree details of the bridge instance and number of VLANs
   associated with it */
int
mstp_api_get_spanning_tree_mst(char *br_name, u_int16_t instance, 
                               struct smi_mstp_instance_details *inst_details)
{
  struct mstp_bridge *br;
  struct mstp_bridge_instance *br_inst;
  struct rlist_info *vlan_list;
  int i = 0;
  
  br = mstp_find_bridge(br_name);
  if(!br)
    return RESULT_ERROR;

  /* In case the instance is not created Set the return Structure to 0.
   * Don't return Error.
   */
  br_inst = br->instance_list[instance];
  if (!br_inst)
    {
      pal_mem_set (inst_details, 0, sizeof (struct smi_mstp_instance_details));
      return RESULT_OK;
    }

  inst_details->instance_id = instance;
  inst_details->master = br_inst->master;
  pal_mem_cpy(&(inst_details->msti_bridge_id), &(br_inst->msti_bridge_id),
               sizeof(struct bridge_id));
  inst_details->msti_bridge_priority = br_inst->msti_bridge_priority;
  pal_mem_cpy(&(inst_details->msti_designated_root), 
              &(br_inst->msti_designated_root),
              sizeof(struct bridge_id));
  pal_mem_cpy(&(inst_details->msti_designated_bridge), 
              &(br_inst->msti_designated_bridge),
              sizeof(struct bridge_id));
  inst_details->internal_root_path_cost = br_inst->internal_root_path_cost;
  inst_details->msti_root_port_id = br_inst->msti_root_port_id;
  inst_details->msti_root_port_ifindex = br_inst->msti_root_port_ifindex;
  inst_details->hop_count = br_inst->hop_count;

  SMI_BMP_INIT(inst_details->vlanMemberBmp);

  for (vlan_list = br_inst->vlan_list; vlan_list != NULL;
                   vlan_list = vlan_list->next)
  {
     if (vlan_list->lo != vlan_list->hi)
     {
         for (i = vlan_list->lo; i <= vlan_list->hi; i++)
           SMI_BMP_SET(inst_details->vlanMemberBmp, i);
     }
     else
      SMI_BMP_SET(inst_details->vlanMemberBmp, vlan_list->lo);
  }
  return RESULT_OK;
}

/* Get MSTP configuration information for a bridge */
int
mstp_api_get_spanning_tree_mst_config(char *br_name,
                                      struct smi_mstp_config_details 
                                      *conf_details)
{
  struct mstp_bridge *br;

  pal_mem_set(conf_details, 0, sizeof(struct smi_mstp_config_details));

  br = mstp_find_bridge(br_name);
  if(!br)
    return RESULT_ERROR;

  pal_mem_cpy(conf_details, &(br->config), 
              sizeof(struct smi_mstp_config_details));

  return RESULT_OK;
}

/* Get detailed information about the specified instance and all interfaces
   associated with the particular instance */
int
mstp_api_get_spanning_tree_mstdetail(char *br_name, u_int16_t instance, 
                                     struct smi_mstp_instance_details 
                                     *inst_details)
{
  struct mstp_bridge *br;
  struct mstp_bridge_instance *br_inst;
  struct mstp_instance_port *inst_port;
  struct rlist_info *vlan_list;
  int i = 0;
  
  br = mstp_find_bridge(br_name);
  if(!br)
    return RESULT_ERROR;

  br_inst = br->instance_list[instance];
  if(!br_inst)
    return RESULT_ERROR;

  br_inst = br->instance_list[instance];
  if(!br_inst)
    return RESULT_ERROR;

  inst_details->master = br_inst->master;
  pal_mem_cpy(&(inst_details->msti_bridge_id), &(br_inst->msti_bridge_id),
               sizeof(struct bridge_id));
  inst_details->msti_bridge_priority = br_inst->msti_bridge_priority;
  pal_mem_cpy(&(inst_details->msti_designated_root), 
              &(br_inst->msti_designated_root),
              sizeof(struct bridge_id));
  pal_mem_cpy(&(inst_details->msti_designated_bridge), 
              &(br_inst->msti_designated_bridge),
              sizeof(struct bridge_id));
  inst_details->internal_root_path_cost = br_inst->internal_root_path_cost;
  inst_details->msti_root_port_id = br_inst->msti_root_port_id;
  inst_details->msti_root_port_ifindex = br_inst->msti_root_port_ifindex;
  inst_details->hop_count = br_inst->hop_count;

  SMI_BMP_INIT(inst_details->vlanMemberBmp);

  for (vlan_list = br_inst->vlan_list; vlan_list != NULL;
                   vlan_list = vlan_list->next)
  {
     if (vlan_list->lo != vlan_list->hi)
     {
         for (i = vlan_list->lo; i <= vlan_list->hi; i++)
           SMI_BMP_SET(inst_details->vlanMemberBmp, i);
     }
     else
      SMI_BMP_SET(inst_details->vlanMemberBmp, vlan_list->lo);
  }

  SMI_BMP_INIT(inst_details->port_list);

  for (inst_port = br_inst->port_list; inst_port;
       inst_port = inst_port->next)
  {
    if(inst_port)
    {
      /* port 0 <=> fe1 <=> SMI_L2_IFINDEX_START + 1 */
      SMI_BMP_SET(inst_details->port_list, 
                  (inst_port->ifindex - (SMI_L2_IFINDEX_START + 1)));
    }
  }
  return RESULT_OK;
}

/* Get detailed information about the specified instance and the specified
   interface associated with that particular instance */
int
mstp_api_get_spanning_tree_mstdetail_interface(struct interface *ifp, 
                                         u_int16_t instance, 
                                         struct smi_mstp_instance_port_details 
                                         *port_details)
{
  struct mstp_bridge *br = NULL;
  struct mstp_port *port = NULL;
  struct mstp_bridge_instance *br_inst = NULL;
  struct mstp_instance_port *inst_port = NULL;

  br = mstp_find_bridge (ifp->bridge_name);
  if(!br)
    return RESULT_ERROR;
  
  br_inst = br->instance_list[instance];
  if(!br_inst)
    return RESULT_ERROR;

  br_inst = br->instance_list[instance];
  if(!br_inst)
    return RESULT_ERROR;

  inst_port = mstp_find_msti_port (br_inst, ifp->ifindex);

  if (!inst_port)
    return RESULT_ERROR;

  port = inst_port->cst_port;
  if (!port)
    return RESULT_ERROR;

  snprintf(port_details->name, BRIDGE_NAMESIZ + 1, port->br->name);
  port_details->ifindex = port->ifindex;
  port_details->instance_id = instance;
  snprintf(port_details->ifname, BRIDGE_NAMESIZ + 1, port->name);
  port_details->msti_port_id = inst_port->msti_port_id;
  port_details->msti_role = inst_port->role;
  port_details->msti_state = inst_port->state;
  port_details->internal_rpc = port->cist_internal_rpc;
  port_details->msti_path_cost = inst_port->msti_path_cost;
  port_details->designated_port_id = inst_port->designated_port_id;
  port_details->msti_priority = inst_port->msti_priority;
  pal_mem_cpy(&(port_details->msti_root), &(inst_port->msti_root),
              sizeof(struct bridge_id));
  pal_mem_cpy(&(port_details->msti_designated_bridge), 
              &(br_inst->msti_designated_bridge),
              sizeof(struct bridge_id));
  port_details->msti_message_age = inst_port->message_age;
  port_details->msti_max_age = inst_port->max_age;
  port_details->msti_fwd_delay = inst_port->fwd_delay;
  port_details->msti_hello_time = inst_port->hello_time;
  port_details->forward_delay_time_remain = 
           l2_timer_get_remaining_secs (port->forward_delay_timer);
  port_details->hello_timer_remain =
           l2_timer_get_remaining_secs (port->hello_timer);
  port_details->message_age_timer_remain =
           l2_timer_get_remaining_secs (port->message_age_timer);
  port_details->tc_timer_remain =
           l2_timer_get_remaining_secs (port->tc_timer);

  return RESULT_OK;
}

/* Get MSTP bridge status */
int
mstp_api_get_bridge_status (char *name, u_char *stp_enabled, 
                            int *br_forward)
{
  struct mstp_port * port;
  struct mstp_bridge * br = NULL;
  struct mstp_instance_port *inst_port;

  br = mstp_find_bridge (name);

  if (br == NULL)
    return RESULT_ERROR;

  if (stp_enabled)
    *stp_enabled = br->mstp_enabled;

  BRIDGE_CIST_PORTLIST_LOOP (port, br)
  {
    if (port->cist_state != STATE_FORWARDING)
    {
      if (br_forward)
      {
        *br_forward = SMI_BRIDGE_PORT_NONFORWARD_STATE;
        return RESULT_OK;
      }

      if (IS_BRIDGE_MSTP (port->br) || IS_BRIDGE_RPVST_PLUS(port->br))
      {
        inst_port = port->instance_list;
        while (inst_port)
        {
           if (inst_port->state != STATE_FORWARDING)
           {
             if (br_forward)
             {
               *br_forward = SMI_BRIDGE_PORT_NONFORWARD_STATE;
               return RESULT_OK;
             }
           }
           inst_port = inst_port->next_inst;
        }
      }
    }
  }

  if (br_forward)
    *br_forward = SMI_BRIDGE_PORT_FORWARD_STATE;

  return RESULT_OK;
}

#endif /* HAVE_SMI */

#ifdef HAVE_L2GP

int
mstp_api_set_l2gp_port(char * name, char * ifname, 
                       uint8_t l2gp_status, 
                       uint8_t enableBPDUrx,
                       struct bridge_id * psuedoRootId )
{

  struct mstp_bridge *br;
  struct mstp_port *port;
  struct interface *ifp;
  struct apn_vr *vr = apn_vr_get_privileged (mstpm);
  u_int32_t ifindex;

  ifp = if_lookup_by_name (&vr->ifm, ifname);

  pal_assert(ifp);

  if (!ifp)
    {
      return RESULT_ERROR;
    }

  ifindex = ifp->ifindex;

  br = mstp_find_bridge (name);

  pal_assert(br);

  if (!br)
    {
      return RESULT_ERROR;
    }

  port = mstp_find_cist_port (br, ifindex);

  pal_assert(port);

  if (!port)
    {
      return RESULT_ERROR;
    }

  port->isL2gp = l2gp_status;

  port->enableBPDUrx = enableBPDUrx;

  if (l2gp_status != PAL_FALSE)
    {
      pal_mem_cpy(&port->psuedoRootId, psuedoRootId, sizeof(struct bridge_id));
      port->enableBPDUtx = PAL_FALSE; 
    }
  else
    {
      pal_mem_set(&port->psuedoRootId,0,sizeof(struct bridge_id));
      port->enableBPDUtx = PAL_TRUE; 
    }
      
  return RESULT_OK;
}

#endif /* HAVE_L2GP */
#if defined (HAVE_PROVIDER_BRIDGE) || defined (HAVE_B_BEB)
/* API to enable msti on a bridge */
int
mstp_api_enable_bridge_msti (struct mstp_bridge_instance *mst_br_inst)
{
   return mstp_enable_bridge_instance (mst_br_inst);
}

/* API to disable msti on a bridge */
int
mstp_api_disable_bridge_msti (struct mstp_bridge_instance *mst_br_inst)
{
   return mstp_disable_bridge_instance (mst_br_inst);
}

#ifdef HAVE_I_BEB
void 
mstp_make_pbb_dst_mac ( uint8_t * buf, uint32_t isid )
{
   unsigned char pbb_oui[6] = { 0x01, 0x1e, 0x83 };
   
   if (!buf)
     return;

   isid = pal_hton32(isid);

   pbb_oui[3]= isid >> 8 & 0xff;
   pbb_oui[4]= isid >> 16 & 0xff;
   pbb_oui[5]= isid >> 24 & 0xff;

   pal_mem_cpy(buf,pbb_oui,sizeof(pbb_oui));
}

uint32_t 
mstp_encapsulate_bpdu( u_char * buf, 
                       uint32_t isid, 
                       struct sockaddr_vlan *l2_skaddr)
{
    uint16_t len = 0;
    uint16_t tpid = pal_hton16(PBB_TPID);

    /* tpid */
    pal_mem_cpy(buf+len,&tpid,2);
    len+=2;

    /* isid */
    isid = pal_hton32(isid);
    pal_mem_cpy(buf+len,&isid,4);
    len+=4;

    /* CDA */
    pal_mem_cpy(buf+len,l2_skaddr->dest_mac,ETHER_ADDR_LEN);
    len+=ETHER_ADDR_LEN;

    /* CSA */
    pal_mem_cpy(buf+len,l2_skaddr->src_mac,ETHER_ADDR_LEN);
    len+=ETHER_ADDR_LEN;

    return len;
}
#endif 

#endif /*(HAVE_PROVIDER_BRIDGE) || (HAVE_B_BEB)*/

