/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "hal_incl.h"

#include "lib.h"
#include "if.h"
#include "cli.h"
#include "table.h"
#include "linklist.h"
#include "snprintf.h"
#include "bitmap.h"
#include "nsm_message.h"
#include "nsm_client.h"

#include "thread.h"
#include "nsmd.h"
#include "nsm_server.h"
#include "nsm_interface.h"
#include "nsm_api.h"
#include "nsm_router.h"
#include "nsm_debug.h"
#include "nsm/nsm_fea.h"
#include "nsm_oam.h"

#include "nsm_pro_vlan.h"
#ifdef HAVE_G8032
#include "L2/nsm_bridge.h"
#endif

#define CENTISECS_TO_SECS(x) ((x)/(100))
#define CENTISECS_TO_MICROSECS(x) ((x)*10000)
#define SECS_TO_CENTISECS(x) ((x)*(100))
#define CENTISECS_TO_SUBSECS(x) \
     ( CENTISECS_TO_MICROSECS( (x) - (SECS_TO_CENTISECS(CENTISECS_TO_SECS(x)))))

struct thread *
nsm_l2_oam_start_timer (s_int32_t (*func) (struct thread *t),
                        struct nsm_efm_oam_if *efm_oam_if,
                        pal_time_t time)
{
  struct pal_timeval tv;

  tv.tv_sec = CENTISECS_TO_SECS(time);
  tv.tv_usec = CENTISECS_TO_SUBSECS(time);

  return thread_add_timer_timeval (nzg, func, efm_oam_if, tv);

}


void
nsm_l2_oam_stop_timer (struct thread **thread)
{
  if (thread && (*thread) && (*thread)->type != THREAD_UNUSED)
    {
      thread_cancel (*thread);
    }

  if (thread)
    *thread = 0;

  return;
}


void
nsm_l2_oam_master_init (struct nsm_master *nm)
{

  nm->l2_oam_master = XCALLOC (MTYPE_NSM_L2_OAM_MASTER,
                      sizeof (struct nsm_l2_oam_master));

  if (nm->l2_oam_master == NULL)
    return;

  nm->l2_oam_master->master = nm;
  nm->l2_oam_master->syscap = 0;
}

void
nsm_l2_oam_master_deinit (struct nsm_master *nm)
{

  if (nm->l2_oam_master != NULL)
    XFREE (MTYPE_NSM_L2_OAM_MASTER, nm->l2_oam_master);

  nm->l2_oam_master = NULL;
}

s_int32_t
nsm_efm_oam_send_efm_msg (struct nsm_msg_efm_if *msg,
                          u_int16_t opcode)
{
  s_int32_t i;
  s_int32_t nbytes;
  struct nsm_server_entry *nse;
  struct nsm_server_client *nsc;

  msg->opcode = opcode;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
   if (nsm_service_check (nse, NSM_SERVICE_INTERFACE) &&
       nse->service.protocol_id == APN_PROTO_ONM)
     {
       /* Set nse pointer and size. */
       nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
       nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

       /* Encode NSM EFM message. */

       nbytes = nsm_encode_efm_if_msg (&nse->send.pnt, &nse->send.size, msg);

       if (nbytes < 0)
         return nbytes;

       /* Send bridge message. */
       nsm_server_send_message (nse, 0, 0, NSM_MSG_EFM_OAM_IF, 0, nbytes);
     }

  return 0;
}

u_int16_t
nsm_efm_oam_str_to_event (u_int8_t *str)
{

  if (pal_strcmp (str, "link-fault") == 0)
    return NSM_EFM_OAM_LINK_FAULT_EVENT;
  else if (pal_strcmp (str, "critical-event") == 0)
    return NSM_EFM_OAM_CRITICAL_LINK_EVENT;
  else if (pal_strcmp (str, "dying-gasp") == 0)
    return NSM_EFM_OAM_DYING_GASP_EVENT;
  else
    return NSM_EFM_OAM_INVALID_EVENT;

  return NSM_EFM_OAM_INVALID_EVENT;

}

enum nsm_efm_opcode
nsm_efm_oam_str_to_opcode (u_int8_t *str)
{

  if (pal_strcmp (str, "symbol-period-errors") == 0)
    return NSM_EFM_SET_SYMBOL_PERIOD_ERROR;
  else if (pal_strcmp (str, "frame-period-errors") == 0)
    return NSM_EFM_SET_FRAME_PERIOD_ERROR;
  else if (pal_strcmp (str, "frame-errors") == 0)
    return NSM_EFM_SET_FRAME_EVENT_ERROR;
  else if (pal_strcmp (str, "frame-seconds-errors") == 0)
    return NSM_EFM_SET_FRAME_SECONDS_ERROR;
  else
    return NSM_EFM_OPERATION_INVALID;

  return NSM_EFM_OPERATION_INVALID;

}

s_int32_t
nsm_efm_oam_process_local_event (struct interface *ifp, u_int16_t event,
                                 u_int8_t enable)
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct nsm_msg_efm_if msg;
  struct nsm_efm_oam_if *efm_oam_if;

  zif = NULL;
  efm_oam_if = NULL;

  if (ifp != NULL)
    {
      zif = ifp->info;

      pal_mem_set (&msg, 0, sizeof (struct nsm_msg_efm_if));

      if (zif == NULL)
        return NSM_EFM_OAM_ERR_IF_NOT_FOUND;

      efm_oam_if = zif->efm_oam_if;

      if (efm_oam_if == NULL)
        return NSM_EFM_OAM_ERR_NOT_ENABLED;

      msg.ifindex = ifp->ifindex;
   }
 else
   msg.ifindex = NSM_EFM_INVALID_IFINDEX;

  msg.local_event = event;

  if (enable)
    {
      ret = nsm_efm_oam_send_efm_msg (&msg, NSM_EFM_SET_LOCAL_EVENT);

      if (ret < 0)
        return NSM_EFM_OAM_ERR_INTERNAL;

      if (efm_oam_if != NULL)
        SET_FLAG (efm_oam_if->local_event, event);
    }
  else
    {
      ret = nsm_efm_oam_send_efm_msg (&msg, NSM_EFM_UNSET_LOCAL_EVENT);

      if (ret < 0)
        return NSM_EFM_OAM_ERR_INTERNAL;

      if (efm_oam_if != NULL)
        UNSET_FLAG (efm_oam_if->local_event, event);
    }

  return NSM_L2_OAM_ERR_NONE;

}

s_int32_t
nsm_efm_oam_process_period_error_event (struct interface *ifp,
                                        enum nsm_efm_opcode opcode,
                                        u_int16_t no_of_errors)
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct nsm_msg_efm_if msg;
  struct nsm_efm_oam_if *efm_oam_if;

  if(!ifp)
    {
      return NSM_EFM_OAM_ERR_IF_NOT_FOUND;
    }
  zif = ifp->info;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_efm_if));

  if (zif == NULL)
    {
    return NSM_EFM_OAM_ERR_IF_NOT_FOUND;
    }

  efm_oam_if = zif->efm_oam_if;

  if (efm_oam_if == NULL)
    {
    return NSM_EFM_OAM_ERR_NOT_ENABLED;
    }
  
  /*
  if (efm_oam_if->link_monitor_on == PAL_FALSE)
    {
    return NSM_EFM_OAM_LINK_MONITOR_OFF;
    }
  */

  msg.ifindex = ifp->ifindex;
  msg.num_of_error.l[0] = no_of_errors;
  
  ret = nsm_efm_oam_send_efm_msg (&msg, opcode);

  return NSM_L2_OAM_ERR_NONE;
}

s_int32_t
nsm_efm_oam_set_frame_errors (struct interface *ifp,
                              u_int16_t no_of_errors)
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct nsm_msg_efm_if msg;
  struct nsm_efm_oam_if *efm_oam_if;

  zif = ifp->info;
  ret = NSM_L2_OAM_ERR_NONE;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_efm_if));

  if (zif == NULL)
    return NSM_EFM_OAM_ERR_IF_NOT_FOUND;

  efm_oam_if = zif->efm_oam_if;

  if (efm_oam_if == NULL)
    return NSM_EFM_OAM_ERR_NOT_ENABLED;

#ifndef HAVE_HARDWARE_OAM_SUPPORT
#ifdef HAVE_HAL
  if (hal_efm_set_err_frames (no_of_errors) != HAL_SUCCESS)
    return NSM_EFM_OAM_ERR_HAL;
#endif /* HAVE_HAL */
#endif /* HAVE_HARDWARE_OAM_SUPPORT */

  efm_oam_if->no_of_err_last_frame_window.l[0] = 0;
  efm_oam_if->no_of_err_last_frame_window.l[1] = 0;

  return NSM_L2_OAM_ERR_NONE;
}

s_int32_t
nsm_efm_oam_set_frame_seconds_errors (struct interface *ifp,
                                      u_int16_t no_of_errors)
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct nsm_msg_efm_if msg;
  struct nsm_efm_oam_if *efm_oam_if;

  zif = ifp->info;
  ret = NSM_L2_OAM_ERR_NONE;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_efm_if));

  if (zif == NULL)
    return NSM_EFM_OAM_ERR_IF_NOT_FOUND;

  efm_oam_if = zif->efm_oam_if;

  if (efm_oam_if == NULL)
    return NSM_EFM_OAM_ERR_NOT_ENABLED;

#ifndef HAVE_HARDWARE_OAM_SUPPORT
#ifdef HAVE_HAL
  if (hal_efm_set_err_frame_seconds (no_of_errors) != HAL_SUCCESS)
    return NSM_EFM_OAM_ERR_HAL;
#endif /* HAVE_HAL */
#endif /* HAVE_HARDWARE_OAM_SUPPORT */

  return NSM_L2_OAM_ERR_NONE;
}

/* Set NSM server callbacks. */
s_int32_t
nsm_oam_set_server_callback (struct nsm_server *ns)
{
  nsm_server_set_callback (ns, NSM_MSG_EFM_OAM_IF,
                           nsm_parse_oam_efm_msg,
                           nsm_efm_oam_if_msg_process);
  nsm_server_set_callback (ns, NSM_MSG_OAM_LLDP,
                           nsm_parse_oam_lldp_msg,
                           nsm_lldp_msg_process);
  nsm_server_set_callback (ns, NSM_MSG_OAM_CFM,
                           nsm_parse_oam_cfm_msg,
                           nsm_cfm_msg_process);
  /*G8031*/
#ifdef HAVE_G8031  
  nsm_server_set_callback (ns, NSM_MSG_G8031_PG_INITIALIZED,
                           nsm_parse_oam_pg_init_msg,
                           nsm_pg_init_msg_process);

  nsm_server_set_callback (ns, NSM_MSG_G8031_PG_PORTSTATE,
                           nsm_parse_oam_g8031_portstate_msg,
                           nsm_g8031_portstate_msg_process);

#endif  

#ifdef HAVE_G8032
  nsm_server_set_callback (ns,NSM_MSG_BRIDGE_G8032_PORT_STATE,
                           nsm_parse_oam_g8032_portstate_msg,
                           nsm_g8032_portstate_msg_process);
#endif  /*HAVE_G8032 */
  
  /* CALL back for cfm uni-meg status msg*/
  nsm_server_set_callback (ns, NSM_MSG_CFM_OPERATIONAL,
                           nsm_parse_oam_cfm_status_msg,
                           nsm_cfm_status_msg_process);


  return 0;
}

s_int32_t
nsm_efm_oam_process_return (struct interface *ifp,
                            struct cli *cli,
                            s_int32_t retval)
{
  s_int32_t ret;

  ret = CLI_ERROR;

  switch (retval)
    {
      case NSM_EFM_OAM_ERR_IF_NOT_FOUND:
        if (cli)
          cli_out (cli, "Interface Not found %s\n", ifp ? ifp->name: "Unknown");
        else
          zlog_err (nzg, "Interface Not found %s\n", ifp ? ifp->name: "Unknown");
        break;
      case NSM_EFM_OAM_ERR_NOT_ENABLED:
        if (cli)
          cli_out (cli, "Ethernet OAM not enabled on found %s\n",
                   ifp ? ifp->name: "Unknown");
        else
          zlog_err (nzg, "Ethernet OAM not enabled on found %s\n",
                   ifp ? ifp->name: "Unknown");
        break;
      case NSM_EFM_OAM_ERR_ENABLED:
        if (cli)
          cli_out (cli, "Ethernet OAM already enabled on found %s\n",
                   ifp ? ifp->name: "Unknown");
        else
          zlog_err (nzg, "Ethernet OAM already enabled on found %s\n",
                   ifp ? ifp->name: "Unknown");
        break;
      case NSM_EFM_OAM_ERR_HAL:
        if (cli)
          cli_out (cli, "Hardware update error for interface %s\n",
                   ifp ? ifp->name: "Unknown");
        else
          zlog_err (nzg, "Hardware update error for interface %s\n",
                    ifp ? ifp->name: "Unknown");
        break;
      case NSM_EFM_OAM_ERR_MEMORY:
        if (cli)
          cli_out (cli, "Out of memory while updating %s\n",
                   ifp ? ifp->name: "Unknown");
        else
          zlog_err (nzg, "Out of memory while updating %s\n",
                    ifp ? ifp->name: "Unknown");
        break;
      case NSM_EFM_OAM_LINK_MONITOR_OFF:
        if (cli)
          cli_out (cli, "Link Monitoring turned off on this interface %s\n",
                   ifp ? ifp->name: "Unknown");
        else
          zlog_err (nzg, "Link Monitoring turned off on this interface %s\n",
                    ifp ? ifp->name: "Unknown");
        break;
      default:
        ret = CLI_SUCCESS;
    }

  return ret;
}

static s_int32_t
nsm_efm_oam_port_enable (struct interface *ifp)
{
  struct nsm_if *zif;
  s_int32_t toggle_interface;
  struct nsm_efm_oam_if *efm_oam_if;

  zif = ifp->info;
  toggle_interface = PAL_FALSE;

  if (zif == NULL)
    return NSM_EFM_OAM_ERR_IF_NOT_FOUND;

  efm_oam_if = zif->efm_oam_if;

  if (efm_oam_if != NULL)
    return NSM_EFM_OAM_ERR_ENABLED;

  efm_oam_if = XCALLOC (MTYPE_NSM_EFM_OAM_IF, sizeof (struct nsm_efm_oam_if));

  if (efm_oam_if == NULL)
    return NSM_EFM_OAM_ERR_MEMORY;

  efm_oam_if->zif = zif;
  zif->efm_oam_if = efm_oam_if; 


  /* Set the all multi flags for this interface to receive
   * Ethernet OAM pdus
   */

  if (if_is_up(ifp))
    toggle_interface = 1;

  /* If IFF_UP is not given port will be down in hardware */
  if (toggle_interface)
    nsm_fea_if_flags_set (ifp, IFF_ALLMULTI | IFF_UP);
  else
    nsm_fea_if_flags_set (ifp, IFF_ALLMULTI);

  nsm_fea_if_flags_get (ifp);

  if (toggle_interface && (! if_is_running(ifp)))
    nsm_if_flag_up_set (ifp->vr->id, ifp->name, PAL_TRUE);

  nsm_oam_lldp_if_set_protocol_list (ifp, NSM_LLDP_PROTO_EFM_OAM,
                                     PAL_TRUE);

  return NSM_L2_OAM_ERR_NONE;
}

static s_int32_t
nsm_efm_oam_port_disable (struct interface *ifp)
{
  struct nsm_if *zif;
  struct nsm_efm_oam_if *efm_oam_if;

  zif = ifp->info;

  if (zif == NULL)
    return NSM_EFM_OAM_ERR_IF_NOT_FOUND;

  efm_oam_if = zif->efm_oam_if;

  if (efm_oam_if == NULL)
    return NSM_EFM_OAM_ERR_NOT_ENABLED;

  if (efm_oam_if->t_frame_window != NULL)
    {
      thread_cancel (efm_oam_if->t_frame_window);
      efm_oam_if->t_frame_window = NULL;
    }

  if (efm_oam_if->t_frame_secs != NULL)
    {
      thread_cancel (efm_oam_if->t_frame_secs);
      efm_oam_if->t_frame_secs = NULL;
    }

  if (efm_oam_if->t_frame_secs_window != NULL)
    {
      thread_cancel (efm_oam_if->t_frame_secs_window);
      efm_oam_if->t_frame_secs_window = NULL;
    }

  XFREE (MTYPE_NSM_EFM_OAM_IF, efm_oam_if);

  zif->efm_oam_if = NULL;

  nsm_oam_lldp_if_set_protocol_list (ifp, NSM_LLDP_PROTO_EFM_OAM,
                                     PAL_FALSE);

  /* Unset the all multi flags for this port */
  nsm_fea_if_flags_unset (ifp, IFF_ALLMULTI);

  return NSM_L2_OAM_ERR_NONE;
}

static s_int32_t
nsm_efm_oam_set_port_state (struct interface *ifp,
                            u_int16_t local_par_action,
                            u_int16_t local_mux_action)
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct nsm_efm_oam_if *efm_oam_if;

  ret = NSM_L2_OAM_ERR_NONE;
  zif = ifp->info;

  if (zif == NULL)
    return NSM_EFM_OAM_ERR_IF_NOT_FOUND;

  efm_oam_if = zif->efm_oam_if;

  if (efm_oam_if == NULL)
    return NSM_EFM_OAM_ERR_NOT_ENABLED;

#ifdef HAVE_HAL

  if (hal_efm_set_port_state (ifp->ifindex, local_par_action,
                              local_mux_action) != HAL_SUCCESS)
    ret = NSM_EFM_OAM_ERR_HAL;

#endif /* HAVE_HAL */

  return ret;
}

s_int32_t
nsm_efm_oam_frame_window_expiry (struct thread *thread)
{
  struct nsm_if *zif;
  struct interface *ifp;
  ut_int64_t no_of_errors;
  struct nsm_msg_efm_if msg;
  struct nsm_efm_oam_if *efm_oam_if;

  if (thread == NULL || thread->arg == NULL)
    return NSM_L2_OAM_ERR_NONE;

  efm_oam_if = (struct nsm_efm_oam_if *) thread->arg;

  efm_oam_if->t_frame_window = NULL;

  zif = efm_oam_if->zif;
  no_of_errors.l[0] = no_of_errors.l[1] = 0;
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_efm_if));

  if (zif == NULL || zif->ifp == NULL)
    return NSM_L2_OAM_ERR_NONE;

  ifp = zif->ifp;
  msg.ifindex = ifp->ifindex;

#ifdef HAVE_HAL

  if (hal_efm_get_err_frames (ifp->ifindex, &no_of_errors) != HAL_SUCCESS)
    return NSM_EFM_OAM_ERR_HAL;

#endif /* HAVE_HAL */

  if (efm_oam_if->no_of_err_last_frame_window.l[0] == 0
       && efm_oam_if->no_of_err_last_frame_window.l[1] == 0)
    pal_mem_cpy (&msg.num_of_error, &no_of_errors, sizeof (ut_int64_t));
  else
    PAL_SUB_64_UINT (no_of_errors, efm_oam_if->no_of_err_last_frame_window,
                     msg.num_of_error);

  efm_oam_if->no_of_err_last_frame_window.l[0] = no_of_errors.l[0];
  efm_oam_if->no_of_err_last_frame_window.l[1] = no_of_errors.l[1];

  efm_oam_if->t_frame_window = nsm_l2_oam_start_timer (
                           nsm_efm_oam_frame_window_expiry,
                           efm_oam_if, efm_oam_if->frame_event_window);

  return nsm_efm_oam_send_efm_msg (&msg, NSM_EFM_SET_FRAME_EVENT_ERROR);
}

s_int32_t
nsm_efm_oam_frame_secs_expiry (struct thread *thread)
{
  struct nsm_if *zif;
  struct interface *ifp;
  ut_int64_t no_of_errors;
  struct nsm_msg_efm_if msg;
  struct nsm_efm_oam_if *efm_oam_if;

  if (thread == NULL || thread->arg == NULL)
    return NSM_L2_OAM_ERR_NONE;

  efm_oam_if = (struct nsm_efm_oam_if *) thread->arg;

  efm_oam_if->t_frame_secs = NULL;

  zif = efm_oam_if->zif;
  no_of_errors.l[0] = no_of_errors.l[1] = 0;
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_efm_if));

  if (zif == NULL || zif->ifp == NULL)
    return NSM_L2_OAM_ERR_NONE;

  ifp = zif->ifp;
  msg.ifindex = ifp->ifindex;

#ifdef HAVE_HAL
#ifndef HAVE_SWFWDR
  if (hal_efm_get_err_frames (ifp->ifindex, &no_of_errors) != HAL_SUCCESS)
    return NSM_EFM_OAM_ERR_HAL;
#else
  if (hal_efm_get_err_frames_secs (ifp->ifindex, &no_of_errors) != HAL_SUCCESS)
    return NSM_EFM_OAM_ERR_HAL;
#endif /* HAVE_SWFWDR */
#endif /* HAVE_HAL */

  if (no_of_errors.l[0] > 0 || no_of_errors.l[1] > 0)
    efm_oam_if->no_of_err_frame_sec += 1;

  efm_oam_if->t_frame_secs =
                thread_add_timer (nzg,
                       nsm_efm_oam_frame_secs_expiry,
                       efm_oam_if,
                       NSM_EFM_OAM_FRAME_SEC_INTERVAL);

  return NSM_L2_OAM_ERR_NONE;
}

s_int32_t
nsm_efm_oam_frame_secs_window_expiry (struct thread *thread)
{
  struct nsm_if *zif = NULL;
  struct interface *ifp = NULL;
  ut_int64_t no_of_errors;
  struct nsm_msg_efm_if msg;
  struct nsm_efm_oam_if *efm_oam_if;
  
  if (thread == NULL || thread->arg == NULL)
    return NSM_L2_OAM_ERR_NONE;

  efm_oam_if = (struct nsm_efm_oam_if *) thread->arg;

  efm_oam_if->t_frame_secs_window = NULL;

  zif = efm_oam_if->zif;

  if (zif == NULL || zif->ifp == NULL)
    return NSM_L2_OAM_ERR_NONE;

  ifp = zif->ifp;


  no_of_errors.l[0] = no_of_errors.l[1] = 0;
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_efm_if));
    
#ifdef HAVE_HAL
 
  msg.ifindex = ifp->ifindex;
  msg.num_of_error.l[0] = efm_oam_if->no_of_err_frame_sec;
  msg.num_of_error.l[1] = 0;
   
#endif /* HAVE_HAL */


#ifndef HAVE_HARDWARE_OAM_SUPPORT
#ifdef HAVE_HAL
  if (hal_efm_reset_err_frame_second_count () != HAL_SUCCESS)
    return NSM_EFM_OAM_ERR_HAL;
#endif /* HAVE_HAL */
#endif /* HAVE_HARDWARE_OAM_SUPPORT */

  efm_oam_if->no_of_err_frame_sec = 0;

  efm_oam_if->t_frame_secs_window = nsm_l2_oam_start_timer (
                           nsm_efm_oam_frame_secs_window_expiry,
                           efm_oam_if, efm_oam_if->frame_sec_sum_window);

  return nsm_efm_oam_send_efm_msg (&msg, NSM_EFM_SET_FRAME_SECONDS_ERROR);
}

static int
nsm_efm_oam_set_sym_period_window (struct interface *ifp,
                                   ut_int64_t sym_period_window)
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct nsm_efm_oam_if *efm_oam_if;

  ret = NSM_L2_OAM_ERR_NONE;
  zif = ifp->info;

  if (zif == NULL)
    return NSM_EFM_OAM_ERR_IF_NOT_FOUND;

  efm_oam_if = zif->efm_oam_if;

  if (efm_oam_if == NULL)
    return NSM_EFM_OAM_ERR_NOT_ENABLED;

#ifdef HAVE_HAL

  if (hal_efm_set_symbol_period_window (ifp->ifindex,
                                        sym_period_window) != HAL_SUCCESS)
    ret = NSM_EFM_OAM_ERR_HAL;

#endif /* HAVE_HAL */

  efm_oam_if->sym_period_window = sym_period_window;

  return ret;
}

static int
nsm_efm_oam_set_frame_window (struct interface *ifp,
                              u_int32_t frame_event_window)
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct nsm_efm_oam_if *efm_oam_if;

  ret = NSM_L2_OAM_ERR_NONE;
  zif = ifp->info;

  if (zif == NULL)
    return NSM_EFM_OAM_ERR_IF_NOT_FOUND;

  efm_oam_if = zif->efm_oam_if;

  if (efm_oam_if == NULL)
    return NSM_EFM_OAM_ERR_NOT_ENABLED;

  nsm_l2_oam_stop_timer (&efm_oam_if->t_frame_window);

  efm_oam_if->frame_event_window = frame_event_window;

  efm_oam_if->t_frame_window = nsm_l2_oam_start_timer (
                           nsm_efm_oam_frame_window_expiry,
                           efm_oam_if, frame_event_window);

  return ret;
}

static int
nsm_efm_oam_set_frame_period_window (struct interface *ifp,
                                     u_int32_t frame_period_window)
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct nsm_efm_oam_if *efm_oam_if;

  ret = NSM_L2_OAM_ERR_NONE;
  zif = ifp->info;
 
  if (zif == NULL)
    {
      return NSM_EFM_OAM_ERR_IF_NOT_FOUND;
    }

  efm_oam_if = zif->efm_oam_if;

  if (efm_oam_if == NULL)
    {
      return NSM_EFM_OAM_ERR_NOT_ENABLED;
    }

#ifdef HAVE_HAL
  if (hal_efm_set_frame_period_window (ifp->ifindex,
                                       frame_period_window) != HAL_SUCCESS)
    ret = NSM_EFM_OAM_ERR_HAL;

#endif /* HAVE_HAL */

  efm_oam_if->frame_period_window = frame_period_window;

  return ret;
}

static int
nsm_efm_oam_set_frame_sec_sum_window (struct interface *ifp,
                                      u_int32_t frame_sec_sum_window)
{
  struct nsm_if *zif;
  struct nsm_efm_oam_if *efm_oam_if;
  
  zif = ifp->info;

  if (zif == NULL)
    return NSM_EFM_OAM_ERR_IF_NOT_FOUND;

  efm_oam_if = zif->efm_oam_if;

  if (efm_oam_if == NULL)
    return NSM_EFM_OAM_ERR_NOT_ENABLED;

#ifdef HAVE_HAL

  if (efm_oam_if->t_frame_secs == NULL)
    efm_oam_if->t_frame_secs =
                thread_add_timer (nzg,
                       nsm_efm_oam_frame_secs_expiry,
                       efm_oam_if,
                       NSM_EFM_OAM_FRAME_SEC_INTERVAL);

#endif /* HAVE_HAL */



  nsm_l2_oam_stop_timer (&efm_oam_if->t_frame_secs_window);

  efm_oam_if->frame_sec_sum_window = frame_sec_sum_window;

  efm_oam_if->t_frame_secs_window = nsm_l2_oam_start_timer (
                           nsm_efm_oam_frame_secs_window_expiry,
                           efm_oam_if, frame_sec_sum_window);


  return NSM_L2_OAM_ERR_NONE;
}

static int
nsm_efm_oam_link_monitoring_on (struct interface *ifp)
{
  s_int32_t ret;
  struct nsm_if *zif;
  struct nsm_efm_oam_if *efm_oam_if;

  ret = NSM_L2_OAM_ERR_NONE;
  zif = ifp->info;

  if (zif == NULL)
    return NSM_EFM_OAM_ERR_IF_NOT_FOUND;

  efm_oam_if = zif->efm_oam_if;

  if (efm_oam_if == NULL)
    return NSM_EFM_OAM_ERR_NOT_ENABLED;

#ifdef HAVE_HAL

  if (hal_efm_set_symbol_period_window (ifp->ifindex,
                                        efm_oam_if->sym_period_window)
                                    != HAL_SUCCESS)
    ret = NSM_EFM_OAM_ERR_HAL;

  if (hal_efm_set_frame_period_window (ifp->ifindex,
                                       efm_oam_if->frame_period_window)
                                    != HAL_SUCCESS)
    ret = NSM_EFM_OAM_ERR_HAL;

#endif /* HAVE_HAL */

  if (efm_oam_if->t_frame_window == NULL)
    efm_oam_if->t_frame_window = nsm_l2_oam_start_timer (
                             nsm_efm_oam_frame_window_expiry,
                             efm_oam_if, efm_oam_if->frame_event_window);
  
  if (efm_oam_if->t_frame_secs == NULL)
    efm_oam_if->t_frame_secs =
                thread_add_timer (nzg,
                       nsm_efm_oam_frame_secs_expiry,
                       efm_oam_if,
                       NSM_EFM_OAM_FRAME_SEC_INTERVAL);
  

  if (efm_oam_if->t_frame_secs_window == NULL)
    efm_oam_if->t_frame_secs_window = nsm_l2_oam_start_timer (
                             nsm_efm_oam_frame_secs_window_expiry,
                             efm_oam_if, efm_oam_if->frame_sec_sum_window);

  efm_oam_if->link_monitor_on = PAL_TRUE;

  return NSM_L2_OAM_ERR_NONE;

}

static int
nsm_efm_oam_link_monitoring_off (struct interface *ifp)
{
  s_int32_t ret;
  struct nsm_if *zif;
  ut_int64_t sym_period_none;
  struct nsm_efm_oam_if *efm_oam_if;

  ret = NSM_L2_OAM_ERR_NONE;
  zif = ifp->info;

  pal_mem_set (&sym_period_none, 0, sizeof (ut_int64_t));

  if (zif == NULL)
    return NSM_EFM_OAM_ERR_IF_NOT_FOUND;

  efm_oam_if = zif->efm_oam_if;

  if (efm_oam_if == NULL)
    return NSM_EFM_OAM_ERR_NOT_ENABLED;

#ifdef HAVE_HAL

  if (hal_efm_set_symbol_period_window (ifp->ifindex,
                                        sym_period_none)
                                    != HAL_SUCCESS)
    ret = NSM_EFM_OAM_ERR_HAL;

  if (hal_efm_set_frame_period_window (ifp->ifindex,
                                       HAL_LINK_MONITOR_POLL_PERIOD_NONE)
                                    != HAL_SUCCESS)
    ret = NSM_EFM_OAM_ERR_HAL;

#endif /* HAVE_HAL */

  if (efm_oam_if->t_frame_window != NULL)
    {
      thread_cancel (efm_oam_if->t_frame_window);
      efm_oam_if->t_frame_window = NULL;
    }

  if (efm_oam_if->t_frame_secs != NULL)
    {
      thread_cancel (efm_oam_if->t_frame_secs);
      efm_oam_if->t_frame_secs = NULL;
    }

  if (efm_oam_if->t_frame_secs_window != NULL)
    {
      thread_cancel (efm_oam_if->t_frame_secs_window);
      efm_oam_if->t_frame_secs_window = NULL;
    }

  efm_oam_if->link_monitor_on = PAL_FALSE;

  return NSM_L2_OAM_ERR_NONE;

}

int
nsm_efm_oam_if_msg_process (struct nsm_msg_header *header,
                            void *arg, void *message)
{
  struct nsm_msg_efm_if *msg = message;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_master *nm;
  struct interface *ifp;
  s_int32_t ret;

  ret = NSM_L2_OAM_ERR_NONE;

  if (IS_NSM_DEBUG_RECV)
    nsm_efm_port_msg_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);

  if (nm == NULL)
    return 0;

  ifp = if_lookup_by_index (&nm->vr->ifm, msg->ifindex);

  if (ifp == NULL)
    return 0;

  switch (msg->opcode)
    {
      case NSM_EFM_ENABLE_OAM:
        ret = nsm_efm_oam_port_enable (ifp);
        break;

      case NSM_EFM_DISABLE_OAM:
        ret = nsm_efm_oam_port_disable (ifp);
        break;

      case NSM_EFM_SET_MUX_PAR_STATE:
        ret = nsm_efm_oam_set_port_state (ifp, msg->local_par_action,
                                          msg->local_mux_action);
        break;
      case NSM_EFM_SET_SYMBOL_PERIOD_WINDOW:
        ret = nsm_efm_oam_set_sym_period_window (ifp,
                              msg->symbol_period_window);
        break;
      case NSM_EFM_SET_FRAME_WINDOW:
        ret = nsm_efm_oam_set_frame_window (ifp,
                              msg->other_event_window);
        break;
      case NSM_EFM_SET_FRAME_PERIOD_WINDOW:
        ret = nsm_efm_oam_set_frame_period_window (ifp,
                              msg->other_event_window);
        break;
      case NSM_EFM_SET_FRAME_SECONDS_WINDOW:
        ret = nsm_efm_oam_set_frame_sec_sum_window (ifp,
                              msg->other_event_window);
        break;
      case NSM_EFM_SET_IF_DOWN:
        nsm_if_flag_up_unset (ifp->vr->id, ifp->name, PAL_FALSE);
        break;
      case NSM_EFM_SET_LINK_MONITORING_ON:
        ret = nsm_efm_oam_link_monitoring_on (ifp);
        break;
      case NSM_EFM_SET_LINK_MONITORING_OFF:
        ret = nsm_efm_oam_link_monitoring_off (ifp);
        break;
      default:
        break;
    }

  if (ret != NSM_L2_OAM_ERR_NONE)
    nsm_efm_oam_process_return (ifp, NULL, ret);

  return ret;
}

s_int32_t
nsm_lldp_set_dest_addr (struct nsm_master *nm, char *lldp_dest_addr)
{
  s_int32_t i;
  u_int8_t  ret;
  s_int32_t nbytes;
  struct nsm_msg_lldp msg;
  struct nsm_server_entry *nse;
  struct nsm_server_client *nsc;
 
  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_lldp));

  msg.opcode = NSM_LLDP_SET_DEST_ADDR;
  
  ret = hal_set_oam_dest_addr (lldp_dest_addr, HAL_PROTO_LLDP);

  if (ret != HAL_SUCCESS)
    msg.lldp_ret_status = NSM_LLDP_RET_OK;
  else
    msg.lldp_ret_status = NSM_LLDP_RET_FAIL;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
    if ( nse->service.protocol_id == APN_PROTO_ONM)
    {
      /* Set nse pointer and size. */
      nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
      nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

      /* Encode NSM EFM message. */

      nbytes = nsm_encode_lldp_msg (&nse->send.pnt, &nse->send.size, &msg);

      if (nbytes < 0)
        return NSM_OAM_LLDP_ERR_INTERNAL;

      /* Send bridge message. */
      if (nsm_server_send_message(nse, 0, 0, NSM_MSG_OAM_LLDP, 0, nbytes) < 0)
        return NSM_OAM_LLDP_ERR_INTERNAL;
    }

  return 0;
} 

s_int32_t
nsm_cfm_set_dest_addr (char *cfm_dest_addr)
{
  u_int8_t  ret;
 
  ret = hal_set_oam_dest_addr (cfm_dest_addr, HAL_PROTO_CFM);

  if (ret != HAL_SUCCESS)
    return NSM_OAM_CFM_ERR_HAL;

  return NSM_L2_OAM_ERR_NONE;

}

s_int32_t
nsm_cfm_set_ether_addr (u_int16_t ether_type)
{
  u_int8_t  ret;

  ret = hal_set_oam_ether_type (ether_type, HAL_PROTO_CFM);

  if (ret != HAL_SUCCESS)
    return NSM_OAM_CFM_ERR_HAL;
                                                                                                                                                             
  return NSM_L2_OAM_ERR_NONE;

}

s_int32_t
nsm_cfm_enable_ccm_level (u_int8_t level)
{
  u_int8_t  ret;
                                                                                                                                                             
  ret = hal_set_cfm_trap_level_pdu (level, HSL_CFM_CCM_FRAME);

  if (ret != HAL_SUCCESS)
    return NSM_OAM_CFM_ERR_HAL;
                                                                                                                                                             
  return NSM_L2_OAM_ERR_NONE;

}

s_int32_t
nsm_cfm_enable_tr_level (u_int8_t level)
{
  u_int8_t  ret;
                                                                                                                                                             
  ret = hal_set_cfm_trap_level_pdu (level, HSL_CFM_TR_FRAME);
                                                                                                                                                             
  if (ret != HAL_SUCCESS)
    return NSM_OAM_CFM_ERR_HAL;
                                                                                                                                                             
  return NSM_L2_OAM_ERR_NONE;

}

s_int32_t
nsm_cfm_disable_ccm_level (u_int8_t level)
{
  u_int8_t  ret;
                                                                                                                                                             
  ret = hal_unset_cfm_trap_level_pdu (level, HSL_CFM_CCM_FRAME);

  if (ret != HAL_SUCCESS)
    return NSM_OAM_CFM_ERR_HAL;
                                                                                                                                                             
  return NSM_L2_OAM_ERR_NONE;

}

s_int32_t
nsm_cfm_disable_tr_level (u_int8_t level)
{
  u_int8_t  ret;
                                                                                                                                                             
  ret = hal_unset_cfm_trap_level_pdu (level, HSL_CFM_TR_FRAME);
                                                                                                                                                             
  if (ret != HAL_SUCCESS)
    return NSM_OAM_CFM_ERR_HAL;
                                                                                                                                                             
  return NSM_L2_OAM_ERR_NONE;

}

s_int32_t
nsm_oam_lldp_set_sys_cap (u_int32_t vr_id, u_int16_t syscap, u_int8_t enable)
{
  s_int32_t i;
  s_int32_t nbytes;
  struct nsm_master *nm;
  struct nsm_msg_lldp msg;
  struct nsm_server_entry *nse;
  struct nsm_server_client *nsc;
 
  nm = nsm_master_lookup_by_id (nzg, vr_id);

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_lldp));

  msg.ifindex = NSM_LLDP_INVALID_IFINDEX;

  if (nm == NULL)
    return 0;

  if (enable == PAL_TRUE) 
    {
      if (nm->l2_oam_master != NULL)
        SET_FLAG (nm->l2_oam_master->syscap, syscap);
      msg.opcode = NSM_LLDP_SET_SYSTEM_CAP;
    }
  else
    {
      if (nm->l2_oam_master != NULL)
        UNSET_FLAG (nm->l2_oam_master->syscap, syscap);
      msg.opcode = NSM_LLDP_UNSET_SYSTEM_CAP;
    }

  msg.syscap = syscap;
  
  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
   if ( nse->service.protocol_id == APN_PROTO_ONM)
     {
       /* Set nse pointer and size. */
       nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
       nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

       /* Encode NSM EFM message. */

       nbytes = nsm_encode_lldp_msg (&nse->send.pnt, &nse->send.size, &msg);

       if (nbytes < 0)
         return nbytes;

       /* Send bridge message. */
       nsm_server_send_message (nse, 0, 0, NSM_MSG_OAM_LLDP, 0, nbytes);
     }

  return 0;
} 

int
nsm_oam_lldp_if_enable (struct nsm_master *nm, u_int32_t ifindex)
{
  struct nsm_lldp_oam_if *lldp_oam_if;
  s_int32_t toggle_interface = 0;
  struct interface *ifp;
  struct nsm_if *zif;
  s_int32_t ret;
  
  ret = NSM_L2_OAM_ERR_NONE;
  
  ifp = if_lookup_by_index (&nm->vr->ifm, ifindex);

  if (ifp == NULL)
    return NSM_L2_OAM_ERR_NONE;

  if ((zif = ifp->info) == NULL)
    return NSM_L2_OAM_ERR_NONE;

  if ((lldp_oam_if = zif->lldp_oam_if) == NULL)
    return NSM_L2_OAM_ERR_NONE;

  /* Set the all multi flags for this interface to receive
   * LLDP pdus
   */
  
  if (if_is_up(ifp)) 
    toggle_interface = 1;
  
  /* If IFF_UP is not given port will be down in hardware */
  if (toggle_interface)
    nsm_fea_if_flags_set (ifp, IFF_ALLMULTI | IFF_UP);
  else
    nsm_fea_if_flags_set (ifp, IFF_ALLMULTI);
  
  nsm_fea_if_flags_get (ifp);
  
  if (toggle_interface && (! if_is_running(ifp)))
    nsm_if_flag_up_set (ifp->vr->id, ifp->name, PAL_TRUE);
  
  nsm_oam_lldp_if_set_protocol_list (ifp, lldp_oam_if->protocol,
                                     PAL_TRUE);

  nsm_lldp_set_agg_port_id (ifp, lldp_oam_if->agg_port_id);

  return NSM_L2_OAM_ERR_NONE;

}

int
nsm_oam_lldp_if_set_protocol_list (struct interface *ifp,
                                   u_int32_t protocol,
                                   u_int8_t enable)
{
  s_int32_t i;
  s_int32_t ret;
  s_int32_t nbytes;
  struct nsm_if *zif;
  struct nsm_master *nm;
  struct nsm_msg_lldp msg;
  struct nsm_server_entry *nse;
  struct nsm_server_client *nsc;
  struct nsm_lldp_oam_if *lldp_oam_if;

  ret = NSM_L2_OAM_ERR_NONE;

  if (ifp == NULL)
    return NSM_OAM_LLDP_ERR_IF_NOT_FOUND;

  if ((zif = ifp->info) == NULL)
    return NSM_OAM_LLDP_ERR_IF_NOT_FOUND;

  if ((lldp_oam_if = zif->lldp_oam_if) == NULL)
    return NSM_OAM_LLDP_ERR_IF_NOT_FOUND;

  if (ifp->vr == NULL)
    return NSM_OAM_LLDP_ERR_INTERNAL;

  if ((nm = ifp->vr->proto) == NULL)
    return NSM_OAM_LLDP_ERR_INTERNAL;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_lldp));

  msg.ifindex = ifp->ifindex;
  msg.protocol = protocol;

  if (enable == PAL_TRUE)
    {
      SET_FLAG (lldp_oam_if->protocol, protocol);
      msg.opcode = NSM_LLDP_IF_PROTO_SET;
    }
  else
    {
      UNSET_FLAG (lldp_oam_if->protocol, protocol);
      msg.opcode = NSM_LLDP_IF_PROTO_UNSET;
    }

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
   if ( nse->service.protocol_id == APN_PROTO_ONM)
     {
       /* Set nse pointer and size. */
       nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
       nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

       /* Encode NSM EFM message. */

       nbytes = nsm_encode_lldp_msg (&nse->send.pnt, &nse->send.size, &msg);

       if (nbytes < 0)
         return nbytes;

       /* Send bridge message. */
       nsm_server_send_message (nse, 0, 0, NSM_MSG_OAM_LLDP, 0, nbytes);
     }

  return NSM_L2_OAM_ERR_NONE;

}

int
nsm_lldp_set_agg_port_id (struct interface *ifp,
                          u_int32_t ifindex)
{
  s_int32_t i;
  s_int32_t ret;
  s_int32_t nbytes;
  struct nsm_if *zif;
  struct nsm_master *nm;
  struct nsm_msg_lldp msg;
  struct nsm_server_entry *nse;
  struct nsm_server_client *nsc;
  struct nsm_lldp_oam_if *lldp_oam_if;

  ret = NSM_L2_OAM_ERR_NONE;

  if (ifp == NULL)
    return NSM_OAM_LLDP_ERR_IF_NOT_FOUND;

  if ((zif = ifp->info) == NULL)
    return NSM_OAM_LLDP_ERR_IF_NOT_FOUND;

  if ((lldp_oam_if = zif->lldp_oam_if) == NULL)
    return NSM_OAM_LLDP_ERR_IF_NOT_FOUND;

  if (ifp->vr == NULL)
    return NSM_OAM_LLDP_ERR_INTERNAL;

  if ((nm = ifp->vr->proto) == NULL)
    return NSM_OAM_LLDP_ERR_INTERNAL;

  pal_mem_set (&msg, 0, sizeof (struct nsm_msg_lldp));

  lldp_oam_if->agg_port_id = ifindex;
  msg.ifindex = ifp->ifindex;
  msg.agg_ifindex = ifindex;
  msg.opcode = NSM_LLDP_IF_SET_AGG_PORT_ID;

  NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
   if ( nse->service.protocol_id == APN_PROTO_ONM)
     {
       /* Set nse pointer and size. */
       nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
       nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

       /* Encode NSM EFM message. */

       nbytes = nsm_encode_lldp_msg (&nse->send.pnt, &nse->send.size, &msg);

       if (nbytes < 0)
         return nbytes;

       /* Send bridge message. */
       nsm_server_send_message (nse, 0, 0, NSM_MSG_OAM_LLDP, 0, nbytes);
     }

  return NSM_L2_OAM_ERR_NONE;
}

int
nsm_lldp_msg_process (struct nsm_msg_header *header,
                      void *arg, void *message)
{
  struct nsm_msg_lldp *msg = message;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_master *nm;
  s_int32_t ret;

  ret = NSM_L2_OAM_ERR_NONE;

  if (IS_NSM_DEBUG_RECV)
    nsm_lldp_msg_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);

  if (nm == NULL)
    return NSM_OAM_LLDP_ERR_INTERNAL;

  switch (msg->opcode)
    {
      case NSM_LLDP_GET_SYSTEM_CAP:
        if (nm->l2_oam_master != NULL)
          ret = nsm_oam_lldp_set_sys_cap
                        (nm->vr->id, nm->l2_oam_master->syscap, PAL_TRUE);
        break;
      case NSM_LLDP_IF_ENABLE:
         ret = nsm_oam_lldp_if_enable (nm, msg->ifindex);
        break;
      case NSM_LLDP_SET_DEST_ADDR:
         ret = nsm_lldp_set_dest_addr (nm, msg->lldp_dest_addr);
         break;
     
      default:
        break;
    }

  return 0;
}

int
nsm_cfm_msg_process (struct nsm_msg_header *header,
                     void *arg, void *message)
{
  struct nsm_msg_cfm *msg = message;
  struct nsm_server_entry *nse = arg;
  struct nsm_server *ns = nse->ns;
  struct nsm_master *nm;
  s_int32_t ret;

  ret = NSM_L2_OAM_ERR_NONE;

  if (IS_NSM_DEBUG_RECV)
    nsm_cfm_msg_dump (ns->zg, msg);

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);

  if (nm == NULL)
    return NSM_OAM_CFM_ERR_INTERNAL;

  switch (msg->opcode)
    {
      case NSM_CFM_SET_DEST_ADDR:
        ret = nsm_cfm_set_dest_addr (msg->cfm_dest_addr);
        break;
      case NSM_CFM_SET_ETHER_TYPE:
        ret = nsm_cfm_set_ether_addr (msg->ether_type);
        break;
      case NSM_CFM_ENABLE_CCM_LEVEL:
        ret = nsm_cfm_enable_ccm_level (msg->level);
        break;
      case NSM_CFM_ENABLE_TR_LEVEL:
        ret = nsm_cfm_enable_tr_level (msg->level);
        break;
      case NSM_CFM_DISABLE_CCM_LEVEL:
        ret = nsm_cfm_disable_ccm_level (msg->level);
        break;
      case NSM_CFM_DISABLE_TR_LEVEL:
        ret = nsm_cfm_disable_tr_level (msg->level);
        break;
      default:
        break;
    }
  return 0;
}

/* Callback function for cfm uni-meg status from cfm */
int
nsm_cfm_status_msg_process (struct nsm_msg_header *header,
                     void *arg, void *message)
{
  struct nsm_msg_cfm_status *msg = message;
  struct nsm_master *nm;
  struct interface *ifp = NULL;
  struct nsm_if *zif = NULL;
  struct nsm_bridge_port *br_port = NULL;
  s_int32_t ret = PAL_FALSE;

  ret = NSM_L2_OAM_ERR_NONE;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);

  if (nm == NULL)
    return NSM_OAM_CFM_ERR_INTERNAL;

  ifp = if_lookup_by_index (&nm->vr->ifm, msg->ifindex);

  if (ifp == NULL)
    return 0;

   zif = (struct nsm_if *) ifp->info;

  if (! zif
      || ! zif->switchport)
    {
      return 0;
    }

  br_port = zif->switchport;

  if (! br_port)
    {
      return ret;
    }
  
  /* Updating Local and remote uni-mep status at NSM*/ 
  if ((msg->uni_mep_status == NSM_CFM_LOCAL_LINK_UP)||
      (msg->uni_mep_status == NSM_CFM_LOCAL_LINK_DOWN))
    br_port->uni_type_status.local_uni_mep_status = msg->uni_mep_status;
  else if ((msg->uni_mep_status == NSM_CFM_REMOTE_LINK_DOWN)||
      (msg->uni_mep_status == NSM_CFM_REMOTE_LINK_UP))
    br_port->uni_type_status.remote_uni_mep_status =  msg->uni_mep_status;
  
  /*If both local and remote uni-meps are UP, the cfm status is set Operational
   *Else if local or remote uni-mep is down, then the cfm status is set to
   *Not-Operational.
   */
  if ((br_port->uni_type_status.local_uni_mep_status == NSM_CFM_LOCAL_LINK_UP)
   && (br_port->uni_type_status.remote_uni_mep_status ==
     NSM_CFM_REMOTE_LINK_UP)) 
    br_port->uni_type_status.cfm_status = NSM_CFM_OPERATIONAL;
  else 
    br_port->uni_type_status.cfm_status = NSM_UNI_TYPE_DEFAULT;
    
  /* Triggering uni type detect to evaluate uni type based on the status
   * received */
  ret = nsm_uni_type_detect (ifp, br_port->uni_type_status.uni_type_mode);
  
  return 0;
}

#ifdef HAVE_G8031
int
nsm_pg_init_msg_process (struct nsm_msg_header *header,
                     void *arg, void *message)
{
  struct nsm_msg_pg_initialized *msg;
  struct nsm_master *nm;
  struct g8031_protection_group    * pg = NULL;
  struct nsm_bridge                * bridge = NULL;
  struct nsm_bridge_master         * master;
  s_int32_t ret;

  ret = NSM_L2_OAM_ERR_NONE;

  msg = message ;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);

  if (nm == NULL)
    return -1;

  master = nsm_bridge_get_master(nm);
  pal_assert(master);

  bridge = nsm_lookup_bridge_by_name (master,msg->bridge_name);

  if (bridge == NULL)
    return -1;

  pg = nsm_g8031_find_protection_group (bridge,msg->eps_id);
  if (msg->g8031_pg_state)
    pg->pg_state = G8031_PG_INITALIZED;

  return 0;
}

int
nsm_g8031_portstate_msg_process (struct nsm_msg_header *header,
                                 void *arg, void *message)
{
    struct nsm_msg_g8031_portstate *msg;
    struct nsm_master *nm;
    struct nsm_bridge                * bridge = NULL;
    struct nsm_bridge_master         * master;
    s_int32_t ret;

    ret = NSM_L2_OAM_ERR_NONE;

    msg = message ;

    nm = nsm_master_lookup_by_id (nzg, header->vr_id);

    if (nm == NULL)
       return -1;

    master = nsm_bridge_get_master(nm);
    pal_assert(master);

    bridge = nsm_lookup_bridge_by_name (master,msg->bridge_name);

    nsm_bridge_set_g8031_port_state(bridge,msg->eps_id,msg->ifindex_fwd,
                            msg->state_f,msg->ifindex_blck,msg->state_b);
    return 0;
}

#endif /*HAVE_G8031 */
void
nsm_oam_lldp_if_add (struct interface *ifp)
{
  struct nsm_if *zif;

  zif = ifp->info;

  if (zif == NULL)
    return;

  if (zif->lldp_oam_if != NULL)
    return;

  zif->lldp_oam_if = XCALLOC (MTYPE_NSM_LLDP_IF,
                              sizeof (struct nsm_lldp_oam_if));

  return;
}

#ifdef HAVE_G8032
int
nsm_g8032_portstate_msg_process (struct nsm_msg_header *header,
                                 void *arg, void *message)
{
  int nbytes;
  struct apn_vr *vr;
  struct nsm_server *ns;
  struct nsm_master *nm;
  struct nsm_server_entry *nse;
  struct nsm_bridge_master *master;
  struct nsm_bridge        *bridge = NULL;
  struct nsm_msg_bridge_g8032_port *msg;
  struct nsm_msg_bridge_g8032_port reply;
  int ret = NSM_L2_OAM_ERR_NONE;

  if ((message == NULL) || (arg == NULL))
    return RESULT_ERROR;

  /* Set connection and message.  */
  nse = arg;
  ns = nse->ns;
  msg = (struct nsm_msg_bridge_g8032_port *)message;

  nm = nsm_master_lookup_by_id (nzg, header->vr_id);
  
  if (nm == NULL)
    return 0;

  master = nsm_bridge_get_master (nm);
  
  if (master == NULL)
    return 0;

  vr = nm->vr;

  /* Initializing the Reply Message to 0. */
  pal_mem_set (&reply, 0, sizeof (struct nsm_msg_bridge_g8032_port));

  /* Copying the message to replyn message as most
   * of the fields in the structure will
   * be unchanged.
   */
  pal_mem_cpy (&reply, msg, sizeof (struct nsm_msg_bridge_g8032_port));

  /* State and fdb_flush will depend on the return type from Hardware. */
  reply.state = 0;

  reply.fdb_flush = 0;

  /* Checking for the bridge. */
  bridge = nsm_lookup_bridge_by_name (master, msg->bridge_name);

  if (bridge == NULL)
    return RESULT_ERROR;
 
  ret = nsm_bridge_set_g8032_port_state (bridge, msg);

  if (ret == NSM_OAM_G8032_FAIL_SET_PORT_STATE)
    reply.state = NSM_BRIDGE_G8032_PORTSTATE_ERROR;
  else
    reply.fdb_flush = G8032_FLUSH_FAIL;

  /* Set encode pointer and size.  */
  nse->send.pnt = nse->send.buf + NSM_MSG_HEADER_SIZE;
  nse->send.size = nse->send.len - NSM_MSG_HEADER_SIZE;

  nbytes = nsm_encode_g8032_port_msg (&nse->send.pnt, &nse->send.size, &reply);

  if (nbytes < 0)
    return RESULT_ERROR;

  nsm_server_send_message (nse, 0, 0, NSM_MSG_BRIDGE_G8032_PORT_STATE,
                           header->message_id, nbytes);

  return 0;
}
#endif /* HAVE_G8032*/

void
nsm_oam_lldp_if_delete (struct interface *ifp)
{
  struct nsm_if *zif;

  zif = ifp->info;

  if (zif == NULL)
    return;

  if (zif->lldp_oam_if != NULL)
    return;

  XFREE (MTYPE_NSM_LLDP_IF, zif->lldp_oam_if);

  zif->lldp_oam_if = NULL;

  return;
}
