/**@file elmi_show.c
 * * @brief  This file contains the display CLIs for ELMI protocol.
 * **/

/* Copyright (C) 2010  All Rights Reserved. */

#include "pal.h"
#include "cli.h"
#include "lib.h"
#include "avl_tree.h"
#include "ptree.h"
#include "lib/L2/l2_timer.h"
#include "lib/L2/l2lib.h"
#include "lib/avl_tree.h"
#include "timeutil.h"
#include "elmid.h"
#include "elmi_cli.h"
#include "elmi_api.h"
#include "elmi_error.h"
#include "elmi_types.h"
#include "elmi_debug.h"
#include "elmi_port.h"

#define MAX_VLANS_SHOW_PER_LINE 10

static u_int8_t *evc_status_str[] = {
  "Not Active",
  "New and Not Active",
  "Active",
  "New and Active",
  "Partially Active",
  "New and Partially Active",
  "",
};

CLI (show_elmi_evc_bridge,
  show_elmi_evc_bridge_cmd,
  "show ethernet lmi evc (bridge <1-32>|)",
  CLI_SHOW_STR,
  "Display ethernet lmi evc information at bridge level",
  "lmi information",
  "Display EVC information",
  BRIDGE_STR,
  BRIDGE_NAME_STR)
{
  struct elmi_bridge *bridge = NULL;
  s_int32_t ret = 0;

  if (argc > 0)
    bridge = elmi_find_bridge (argv[1]);
  else
    bridge = elmi_find_bridge (DEFAULT_BRIDGE);

  if (bridge == NULL)
    {
      cli_out (cli, "%% Bridge not found\n");
      return CLI_ERROR;
    }

  ret = elmi_display_bridge_evc_info (cli, bridge->name);

  return (elmi_cli_return (cli, ret));
}


CLI (show_elmi_evc_id_detail_bridge,
  show_elmi_evc_id_detail_bridge_cmd,
  "show ethernet lmi evc detail EVC_ID (bridge <1-32>|)",
  CLI_SHOW_STR,
  "Display ethernet lmi evc information at bridge level",
  "lmi information",
  "EVC information",
  "Display detailed information",
  "EVC ID string like EVC-TEST",
  BRIDGE_STR,
  BRIDGE_NAME_STR)
{
  struct elmi_bridge *bridge = NULL;
  s_int32_t ret = 0;

  if (pal_strlen (argv[0]) > ELMI_EVC_NAME_LEN)
    {
      cli_out (cli, "%% EVC ID should not exceed 100\n");
      return CLI_ERROR;
    }

  if (argc > 1) 
    {
      bridge = elmi_find_bridge (argv[2]);
    }
  else
    bridge = elmi_find_bridge (DEFAULT_BRIDGE);

  if (bridge == NULL)
    {
      cli_out (cli, "%% Bridge not found\n");
      return CLI_ERROR;
    }

  ret = elmi_display_br_evc_detail_by_name (cli, bridge->name, argv[0]);

  return (elmi_cli_return (cli, ret));
} 

CLI (show_elmi_evc_ref_id_detail_bridge,
  show_elmi_evc_ref_id_detail_bridge_cmd,
  "show ethernet lmi evc detail evc-ref-id EVC_REF_ID "
  "(bridge <1-32>|)",
  CLI_SHOW_STR,
  "Display ethernet lmi evc information at bridge level",
  "lmi information",
  "EVC information",
  "Display detailed information",
  "EVC Refernce id (svlan id) integer format",
  "Give valid EVC reference id i.e 10, 20",
  BRIDGE_STR,
  BRIDGE_NAME_STR)
{
  struct elmi_bridge *bridge = NULL;
  u_int16_t evc_ref_id = 0;
  s_int32_t ret = 0;

  CLI_GET_INTEGER_RANGE ("EVC Ref Id", evc_ref_id, argv[0], 
                         ELMI_VLAN_MIN, ELMI_VLAN_MAX);
  if (argc > 1) 
    {
      bridge = elmi_find_bridge (argv[2]);
    }
  else
    bridge = elmi_find_bridge (DEFAULT_BRIDGE);

  if (bridge == NULL)
    {
      cli_out (cli, "%% Bridge not found\n");
      return CLI_ERROR;
    }

  ret = elmi_display_br_evc_detail_by_vid (cli, bridge->name, evc_ref_id);

  return (elmi_cli_return (cli, ret));
} 

CLI (show_elmi_evc_interface,
  show_elmi_evc_interface_cmd,
  "show ethernet lmi evc interface IFNAME",
  CLI_SHOW_STR,
  "Display ethernet lmi evc information at bridge level",
  "lmi information",
  "Display EVC information",
  "Interface information",
  "Interface name")
{
  struct elmi_ifp *elmi_if = NULL;
  struct interface *ifp = NULL;
  s_int32_t ret = 0;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(ZG, "show ethernet lmi evc |bridge <id>|");

  ifp = if_lookup_by_name(&cli->vr->ifm, argv[0]);

  if ((ifp == NULL) || (ifp->info == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR;
    }

  elmi_if = ifp->info;

  ret = elmi_display_interface_evc_details (cli, elmi_if);

  return (elmi_cli_return (cli, ret));
} 

CLI (show_elmi_evc_id_interface,
  show_elmi_evc_id_interface_cmd,
  "show ethernet lmi evc detail EVC_ID interface IFNAME",
  CLI_SHOW_STR,
  "Display ethernet lmi evc information at bridge level",
  "lmi information",
  "Display EVC information",
  "ELMI information in detail",
  "EVC ID in string",
  "Interface information",
  "Interface name")
{
  struct elmi_ifp *elmi_if = NULL;
  struct interface *ifp = NULL;
  s_int32_t ret = 0;

  /* Check if EVC ID size does not exceed 100 characters */
  if (pal_strlen (argv[0]) > ELMI_EVC_NAME_LEN)
    {
      cli_out (cli, "%% EVC ID should not exceed 16\n");
      return CLI_ERROR;
    }

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);

  if ((ifp == NULL) || (ifp->info == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", argv[1]);
      return CLI_ERROR;
    }

  elmi_if = ifp->info;

  ret = elmi_display_if_evc_detail_by_name (cli, elmi_if, argv[0]);
  
  return (elmi_cli_return (cli, ret));
} 

CLI (show_elmi_evc_ref_id_interface,
  show_elmi_evc_ref_id_interface_cmd,
  "show ethernet lmi evc detail evc-ref-id EVC_REF_ID interface IFNAME",
  CLI_SHOW_STR,
  "Display ethernet lmi evc information at bridge level",
  "lmi information",
  "Display EVC information",
  "Details",
  "EVC Rerefence Id",
  "Give valid EVC reference id i.e 10, 20",
  "Interface information",
  "Interface name")
{
  struct elmi_ifp *elmi_if = NULL;
  struct interface *ifp = NULL;
  u_int16_t evc_ref_id = 0;
  s_int32_t ret = 0;

  CLI_GET_INTEGER_RANGE ("EVC Ref Id", evc_ref_id, argv[0], 
                         ELMI_VLAN_MIN, ELMI_VLAN_MAX);

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);

  if ((ifp == NULL) || (ifp->info == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", argv[1]);
      return CLI_ERROR;
    }

  elmi_if = ifp->info;

  ret = elmi_display_if_evc_detail_by_vid (cli, elmi_if, evc_ref_id);
  
  return (elmi_cli_return (cli, ret));

}

CLI (show_elmi_evc_map,
  show_elmi_elmi_evc_cmd,
  "show ethernet lmi evc map interface IFNAME",
  CLI_SHOW_STR,
  "Display ethernet lmi cevlan evc map information",
  "lmi information",
  "evc information",
  "cevlan to evc map information",
  "Interface information",
  "Interface name")
{
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  s_int32_t ret = 0;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(ZG, "show ethernet lmi evc |bridge <id>|");

  ifp = if_lookup_by_name(&cli->vr->ifm, argv[0]);

  if ((ifp == NULL) || (ifp->info == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR;
    }

  elmi_if = ifp->info;
  ret = elmi_display_lmi_evc_map_info (cli, elmi_if);

  return (elmi_cli_return (cli, ret));
} 

CLI (show_elmi_uni_interface,
  show_elmi_uni_interface_cmd,
  "show ethernet lmi uni interface IFNAME",
  CLI_SHOW_STR,
  "Display ethernet lmi cevlan evc map information",
  "lmi information",
  "uni information",
  "Interface information",
  "Interface name")
{
  struct interface *ifp = NULL;
  s_int32_t ret = 0;

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);

  if ((ifp == NULL) || (ifp->info == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR;
    }

  ret = elmi_display_uni_info (cli, ifp->name);

  return (elmi_cli_return (cli, ret));
} 

/* Show CLIs */
CLI (show_elmi_lmi_parameters_interface,
  show_elmi_lmi_parameters_interface_cmd,
  "show ethernet lmi parameters interface IFNAME",
  CLI_SHOW_STR,
  "Display ethernet lmi parameters information at interface level",
  "lmi information",
  "Display System Parameters Information",
  CLI_INTERFACE_STR,
  CLI_IFNAME_STR)
{
  struct interface *ifp = NULL;
  s_int32_t ret = 0;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(ZG, "show ethernet lmi parameters interface  IFNAME");

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);

  if ((ifp == NULL) || (ifp->info == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR;
    }

  ret = elmi_display_lmi_parameters_info_if (ifp->name, cli);

  if (ret < 0)
    return CLI_ERROR;

  return CLI_SUCCESS;
}

/* Statistics Information */

CLI (show_elmi_statistics_interface,
  show_elmi_statistics_interface_cmd,
  "show ethernet lmi statistics interface IFNAME",
  CLI_SHOW_STR,
  "Display ethernet lmi parameters information at interface level",
  "lmi information",
  "ELMI Statistics",
  CLI_INTERFACE_STR,
  CLI_IFNAME_STR)
{
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  s_int32_t ret = 0;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(ZG, "show ethernet lmi parameters interface  IFNAME");

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);

  if ((ifp == NULL) || (ifp->info == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR;
    }

  elmi_if = ifp->info;

  ret = elmi_display_statistics_if (cli, elmi_if);

  return (elmi_cli_return (cli, ret));
} 


CLI (show_elmi_statistics_br,
  show_elmi_statsitics_br_cmd,
  "show ethernet lmi statistics (bridge <1-32>|)",
  "CLI_SHOW_STR",
  "ELMI_ETHERNET_STR",
  "Local management interface",
  "Statistics",
  BRIDGE_STR,
  BRIDGE_NAME_STR)
{
  struct elmi_bridge *bridge = NULL;
  s_int32_t ret = 0;

  if (argc > 0)
    {
      bridge = elmi_find_bridge (argv[1]);
      zlog_debug (ZG, "clear ethernet lmi statistics \n", argv[1]);
    } 
  else
    bridge = elmi_find_bridge (DEFAULT_BRIDGE);

  if (!bridge)
    {
      cli_out (cli, "%% Bridge not found\n");
      return CLI_ERROR;
    }

  ret = elmi_display_statistics_bridge (cli, bridge->name);

  return elmi_cli_return  (cli, ret);

}


CLI (clear_elmi_statistics_interface,
  clear_elmi_statistics_interface_cmd,
  "clear ethernet lmi statistics interface IFNAME",
  "clear",
  "ethernet",
  "lmi",
  "ELMI Statistics",
  CLI_INTERFACE_STR,
  CLI_IFNAME_STR)
{
  struct interface *ifp = NULL;
  struct  elmi_ifp *elmi_if = NULL;
  s_int32_t ret = 0;

  if (LAYER2_DEBUG(proto, CLI_ECHO))
    zlog_debug(ZG, "show ethernet lmi parameters interface  IFNAME");

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[0]);

  if ((ifp == NULL) || (ifp->info == NULL))
    {
      cli_out (cli, "%% Can't find interface %s\n", argv[0]);
      return CLI_ERROR;
    }

  elmi_if = ifp->info;

  ret = elmi_clear_statistics_interface (cli, elmi_if);

  return (elmi_cli_return (cli, ret));
} 

CLI (clear_elmi_statistics_br,
  clear_elmi_statsitics_br_cmd,
  "clear ethernet lmi statistics (bridge <1-32>|)",
  "clear",
  "ELMI_ETHERNET_STR",
  "Local management interface",
  "Statistics",
  BRIDGE_STR,
  BRIDGE_NAME_STR)
{
  struct elmi_bridge *bridge = NULL;
  s_int32_t ret = 0;

  if (argc > 0)
    {
      bridge = elmi_find_bridge (argv[1]);
      zlog_debug (ZG, "clear ethernet lmi statistics \n", argv[1]);
    } 
  else
    bridge = elmi_find_bridge (DEFAULT_BRIDGE);

  if (!bridge)
    {
      cli_out (cli, "%% Bridge not found\n");
      return CLI_ERROR;
    }

  ret = elmi_clear_statistics_bridge (cli, bridge->name);

  return elmi_cli_return  (cli, ret);
}

void
elmi_display_evc_status_footer (struct cli *cli)
{
  cli_out (cli, " Key: St=Status, A=Active, P=Partially Active, I=Inactive, "
           "N_A=New and Active,\n N_P=New and Partially Active, "
           "N_I=New and Not Active, ?=Link Down\n");
}

u_char *
elmi_map_service_attr_to_str (enum cvlan_evc_map_type map_type)
{
  switch (map_type)
    {
    case ELMI_ALL_TO_ONE_BUNDLING:
      return "All-to-one-bundling";
    case ELMI_SERVICE_MULTIPLEXING:
      return "Service-Multiplexing-with-no-bundling";
    case ELMI_BUNDLING:
      return "Bundling";
    default:
      return "";
    }

  return NULL;
}

/* Get evc type in string format. */
static char *
elmi_get_evc_type_str (u_int32_t evc_type)
{
  switch (evc_type)
    {
    case ELMI_PT_PT_EVC:
      return "point-point";
    case ELMI_MLPT_MLPT_EVC:
      return "Multipoint-to-Multipoint";
    default:
      return "";
    }
  return NULL;
}

/* Get EVC status in string format */
void
elmi_get_evc_status_type (u_int8_t *buf, u_int8_t evc_status)
{

  switch (evc_status)
    {
    case EVC_STATUS_NOT_ACTIVE:
      sprintf(buf, "%s", "I");
      break;
    case EVC_STATUS_NEW_AND_NOTACTIVE:
      sprintf(buf, "%s", "N_I");
      break;
    case EVC_STATUS_ACTIVE:
      sprintf(buf, "%s", "A");
      break;
    case EVC_STATUS_NEW_AND_ACTIVE:
      sprintf(buf, "%s", "N_A");
      break;
    case EVC_STATUS_PARTIALLY_ACTIVE:
      sprintf(buf, "%s", "P");
      break;
    case EVC_STATUS_NEW_PARTIALLY_ACTIVE:
      sprintf(buf, "%s", "N_P");
      break;
    default:
      break;
    }

}

int
elmi_display_lmi_evc_map_info (struct cli *cli, struct elmi_ifp *elmi_if)
{
  struct elmi_port *port = NULL;
  struct listnode *node = NULL;
  struct elmi_cvlan_evc_map *cevlan_evc_map = NULL; 
  struct elmi_evc_status *evc_node = NULL;
  u_int16_t cvid = 0;
  u_int32_t line_count = 0;
  u_int32_t vlans_output = 0;
  u_int8_t buf[5];
  u_int8_t *ptr = NULL;

  ptr = buf;

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  if (!elmi_if->elmi_enabled)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  port = elmi_if->port;

  if (!elmi_if->port)
    return ELMI_ERR_PORT_NOT_FOUND;

  if (!elmi_if->uni_info)
    return ELMI_ERR_NO_VALID_CONFIG;

  if (!elmi_if->cevlan_evc_map_list)
    return ELMI_ERR_CEVLAN_EVC_MAP;

  cli_out (cli, "UNI Id: %s\n", elmi_if->uni_info->uni_id);
  cli_out (cli, "Ether LMI Link Status: %s\n", 
           ((port->elmi_operational_state) ?  "UP": "DOWN"));

  cli_out (cli, " St     Evc Id                                   CE-VLAN\n"); 
  cli_out (cli, " ---- -----------------------------------------   -------\n");

  LIST_LOOP (elmi_if->evc_status_list, evc_node, node)
    {
      elmi_get_evc_status_type (buf, evc_node->evc_status_type);

      cli_out (cli, " %s       %s                               ", 
               buf, evc_node->evc_id); 

      cevlan_evc_map = elmi_lookup_cevlan_evc_map (elmi_if,
                                                   evc_node->evc_ref_id);

      if (cevlan_evc_map)
        {
          line_count = 1;
          for(cvid = ELMI_VLAN_MIN; cvid <= ELMI_VLAN_MAX; cvid++)
            {
              if (ELMI_VLAN_BMP_IS_MEMBER(&cevlan_evc_map->cvlanbitmap, cvid))
                {
                  cli_out(cli, "   %u", cvid);
                  vlans_output++;
                  if( vlans_output == (MAX_VLANS_SHOW_PER_LINE * line_count))
                    {
                      cli_out(cli, "\n                                   ");
                      line_count++;
                    }
                }
            }
          cli_out (cli, "\n");
        }
    }

  cli_out (cli, "\n");

  elmi_display_evc_status_footer (cli);

  return RESULT_OK;
}

/* APIs for show clis */
int
elmi_display_lmi_parameters_info_if (char *ifname, struct cli *cli)
{
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_port *port = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (ZG);
  struct interface *ifp = NULL;

  if (!vr)
    return ELMI_ERR_INVALID_INTERFACE;

  ifp = if_lookup_by_name (&vr->ifm, ifname);
  if (!ifp || !ifp->info)
    return ELMI_ERR_INVALID_INTERFACE;

  elmi_if = ifp->info;

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  port = elmi_if->port;

  cli_out (cli, "\n");
  cli_out (cli, "E-LMI Parameters for interface %s\n", ifp->name);

  if (port)
  cli_out (cli, "Ether LMI Link Status: %s\n",
      ((port->elmi_operational_state) ?
       "UP": "DOWN"));

  if (elmi_if->uni_mode == ELMI_UNI_MODE_UNIC)
    {
      cli_out (cli, " Mode: %s\n", "CE");

      cli_out (cli, " T391: %d\n", elmi_if->polling_time);
      cli_out (cli, " N391: %d\n", elmi_if->polling_counter_limit);
    }
  else
    {
      cli_out (cli, " Mode: %s\n", "PE");

      cli_out (cli, " T392: %d\n",
               elmi_if->polling_verification_time);

      cli_out (cli, " Async-Interval: %d\n",
               elmi_if->asynchronous_time);
    }

  cli_out (cli, " N393: %d\n", elmi_if->status_counter_limit);

  cli_out (cli, "\n");

  return CLI_SUCCESS;

}


int
elmi_display_evc_detail_info (struct cli *cli, struct elmi_ifp *elmi_if,
                              struct elmi_evc_status *evc_info)
{
  struct elmi_port *port = NULL;
  struct elmi_bw_profile *evc_bw = NULL;
  struct elmi_cvlan_evc_map *cvlan_map = NULL;
  u_int8_t evc_id_len = 0;
  u_int8_t cos_id = 0;
  u_int8_t cos_pri = 0;
  u_int16_t cvid = 0;
  u_int32_t line_count = 0;
  u_int32_t vlans_output = 0;
  u_int8_t timebuf[TIME_BUF];
  u_char *evc_type_str = NULL; 
  u_char *map_str = NULL; 

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  port  =  elmi_if->port;
  if (!port)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  if (!evc_info)
    return ELMI_ERR_EVC_STATUS;

  evc_id_len = pal_strlen(evc_info->evc_id);

  evc_type_str = elmi_get_evc_type_str (evc_info->evc_type);

  cli_out (cli, "\n");     
  cli_out (cli, "EVC Id: %s\n", evc_info->evc_id);     

  cli_out (cli, "Interface: %s\n", elmi_if->ifp->name); 

  if (!port->stats.last_full_status_rcvd)
    pal_snprintf (timebuf , sizeof("00:00:00"), "00:00:00");
  else
    timeutil_uptime (timebuf, TIME_BUF,
                     (pal_time_current (NULL) -
                     port->stats.last_full_status_rcvd));

  if (elmi_if->uni_mode == ELMI_UNI_MODE_UNIC)
    cli_out (cli, "Time since Last Full Report: %s\n", timebuf);  

  cli_out (cli, "Ether LMI Link Status: %s\n", 
           ((port->elmi_operational_state) ?  "UP" : "DOWN"));

  cli_out (cli, "\n");     

  if (elmi_if->uni_info != NULL)
    {
      cli_out (cli, "UNI Status: %s\n", 
          ((if_is_up (elmi_if->ifp)) ?  " UP" : "DOWN"));

      cli_out (cli, "UNI Id: %s\n", elmi_if->uni_info->uni_id);

      map_str = 
        elmi_map_service_attr_to_str (elmi_if->uni_info->cevlan_evc_map_type);

      if (map_str)
        cli_out (cli, "CE-VLAN/EVC Map Type: %s\n", map_str); 
    }

  cli_out (cli, "\n");     

  cli_out (cli, "EVC Reference Id(svid): %d\n", evc_info->evc_ref_id);  

  cli_out (cli, "EVC Status: %s\n", evc_status_str[evc_info->evc_status_type]);
  cli_out (cli, "EVC Type: %s\n", evc_type_str);

  cvlan_map = elmi_lookup_cevlan_evc_map (elmi_if, evc_info->evc_ref_id);
  if (cvlan_map)
    {
      if (elmi_if->uni_info &&
          elmi_if->uni_info->cevlan_evc_map_type == ELMI_BUNDLING)
        {
          cli_out (cli, "Default EVC: %s\n",
                   (cvlan_map->default_evc) ? "TRUE" : "FALSE");
        }
      if (elmi_if->uni_info &&
          elmi_if->uni_info->cevlan_evc_map_type != ELMI_ALL_TO_ONE_BUNDLING)
        {
          cli_out (cli, "Untagged/Priority Tagged: %s\n",
                   (cvlan_map->untag_or_pri_frame) ? "TRUE" : "FALSE");
        }

      cli_out (cli, "CE-VLAN to EVC memership:\n"); 

      line_count = 1;
      for(cvid = ELMI_VLAN_MIN; cvid <= ELMI_VLAN_MAX; cvid++)
        {
          if (ELMI_VLAN_BMP_IS_MEMBER(&cvlan_map->cvlanbitmap, cvid)) 
            {
              cli_out(cli, "%5u", cvid);
              vlans_output++;
              if( vlans_output == (MAX_VLANS_SHOW_PER_LINE * line_count) )
                {
                  cli_out(cli, "\n                           ");
                  line_count++;
                }
            }
        }
    }

  cli_out(cli, "\n");

  /* Display Bandwidth profile parameters */
  if (evc_info->bw_profile_level == ELMI_EVC_BW_PROFILE_PER_EVC)
   {
     cli_out (cli, " \n");
     cli_out (cli, "%% Ingress Bandwidth Profile Set Per: EVC\n");

     evc_bw = evc_info->evc_bw;
     if (evc_bw)
       {
         cli_out (cli, " %-8s%-8s%-8s%-8s%-16s%-12s\n", "CIR", "CBS", "EIR",
             "EBS", "Coupling-flag", "Color-mode");
         cli_out (cli, " ======= ======= ======= ======= =============== "
             "============ \n");

         cli_out (cli, " %-8d%-8d%-8d%-8d%-16s%-12s\n",
             evc_bw->cir, evc_bw->cbs, evc_bw->eir, evc_bw->ebs,
             (evc_bw->cf == PAL_TRUE)? "enable" : "disable",
             (evc_bw->cm == PAL_TRUE)? "color-aware" : "color-blind");

         cli_out (cli, " \n");
       }

   }
  else if (evc_info->bw_profile_level == ELMI_EVC_BW_PROFILE_PER_COS)
    {
      cli_out (cli, " \n");
      cli_out (cli, "%% Ingress Bandwidth Profile Set Per: EVC COS\n");

      for (cos_id = 0; cos_id < ELMI_MAX_COS_ID; cos_id++)
        {
          if (evc_info->evc_cos_bw[cos_id]) 
            {
              for (cos_pri = 1; cos_pri < ELMI_MAX_COS_ID; cos_pri++)
                {
                  if (FLAG_ISSET(evc_info->evc_cos_bw[cos_id]->per_cos, 
                                 cos_pri))
                    cli_out (cli, "%% Bandwidth Profile Set for COS : "
                             "%d \n", cos_pri-1);
                }

              cli_out (cli, "%-8s%-8s%-8s%-8s%-16s%-12s\n", "CIR", "CBS", "EIR",
                       "EBS", "Coupling-flag", "Color-mode");
              cli_out (cli, " ======= ======= ======= ======= =============== "
                       "============ \n");

              cli_out (cli, " %-8d%-8d%-8d%-8d%-16s%-12s\n",
                  evc_info->evc_cos_bw[cos_id]->cir, 
                  evc_info->evc_cos_bw[cos_id]->cbs, 
                  evc_info->evc_cos_bw[cos_id]->eir, 
                  evc_info->evc_cos_bw[cos_id]->ebs,
                  (evc_info->evc_cos_bw[cos_id]->cf == PAL_TRUE)? 
                  "enable" : "disable",
                  (evc_info->evc_cos_bw[cos_id]->cm == PAL_TRUE)? 
                  "color-aware" : "color-blind");
            }
        }

    }

  return RESULT_OK;

}

s_int32_t 
elmi_display_evc_info (struct cli *cli, struct elmi_ifp *elmi_if)
{
  struct elmi_port *port = NULL;
  struct elmi_evc_status *evc_info = NULL;
  struct listnode *node = NULL;
  u_int8_t buf[5];
  u_int8_t *ptr = NULL;

  ptr = buf;

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  if (!elmi_if->elmi_enabled)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  port  =  elmi_if->port;
  if (!port)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  cli_out (cli, "St    EVC Id                                          Port\n");
  cli_out (cli, "--- ----------------------------------------          ----\n");

  /* Get the list of evcs received from UNI-N side */
  LIST_LOOP (elmi_if->evc_status_list, evc_info, node)
    {
      elmi_get_evc_status_type (buf, evc_info->evc_status_type);

      cli_out (cli, "%s   %-45s  %s\n", 
               buf, evc_info->evc_id, elmi_if->ifp->name);     
    }

  cli_out (cli, "\n");

  elmi_display_evc_status_footer (cli);

  cli_out (cli, "\n");

  return RESULT_OK;
}

int
elmi_display_interface_evc_info (struct cli *cli, struct elmi_ifp *elmi_if)
{
  s_int16_t ret = 0;

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  if (!elmi_if->elmi_enabled)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  ret = elmi_display_evc_info (cli, elmi_if);

  return (elmi_cli_return (cli, ret));  
}

int
elmi_display_interface_evc_details (struct cli *cli, struct elmi_ifp *elmi_if)
{
  s_int16_t ret = 0;
  struct elmi_evc_status *evc_info = NULL;
  struct listnode *evc_node = NULL;

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  if (!elmi_if->elmi_enabled)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  /* Get the list of evcs received from UNI-N side */
  LIST_LOOP (elmi_if->evc_status_list, evc_info, evc_node)
    {
      ret = elmi_display_evc_detail_info (cli, elmi_if, evc_info);
    }

  return (elmi_cli_return (cli, ret));  

}


int
elmi_display_if_evc_detail_by_name (struct cli *cli, 
                                    struct elmi_ifp *elmi_if, 
                                    u_char *evc_id)
{
  s_int16_t ret = RESULT_ERROR;
  struct elmi_evc_status *evc_status_info = NULL;

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  if (!elmi_if->elmi_enabled)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  /* Check if EVC name size exceed 100 characters */
  if ((pal_strlen (evc_id) > ELMI_EVC_NAME_LEN))
    {
      return ELMI_ERR_INVALID_EVC_LEN;
    }

  evc_status_info = elmi_lookup_evc_by_name (elmi_if, evc_id);

  if (evc_status_info)
    ret = elmi_display_evc_detail_info (cli, elmi_if, evc_status_info);

  return ret;

}

int
elmi_display_if_evc_detail_by_vid (struct cli *cli, 
                                   struct elmi_ifp *elmi_if, 
                                   u_int16_t evc_ref_id)
{
  s_int16_t ret = 0;
  struct elmi_evc_status *evc_status_info = NULL;

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  if (!elmi_if->elmi_enabled)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  if (evc_ref_id  < ELMI_VLAN_MIN || evc_ref_id > ELMI_VLAN_MAX)
    return ELMI_ERR_INVALID_EVC_LEN;

  evc_status_info = elmi_evc_look_up (elmi_if, evc_ref_id);
  if (evc_status_info)
    ret = elmi_display_evc_detail_info (cli, elmi_if, evc_status_info);

  return RESULT_OK;

}

s_int32_t
elmi_display_bridge_evc_info (struct cli *cli, u_int8_t *br_name)
{
  struct elmi_bridge *bridge = NULL;
  struct elmi_ifp *elmi_ifp = NULL;
  struct avl_node *avl_port = NULL;
  s_int16_t ret = 0;

  bridge = elmi_find_bridge (br_name);

  if (!bridge)
    return ELMI_ERR_BRIDGE_NOT_FOUND;

  /* Get the list of ports on which elmi is enabled */
  AVL_TREE_LOOP (bridge->port_tree, elmi_ifp, avl_port)
    {
      if (elmi_ifp->elmi_enabled)
        {
          ret = elmi_display_evc_info (cli, elmi_ifp);
        }
    }

  return (elmi_cli_return (cli, ret));  

}

int
elmi_display_br_evc_detail_by_name (struct cli *cli, u_int8_t *br_name,
                                    u_int8_t *evc_id)
{
  struct elmi_bridge *bridge = NULL;
  struct elmi_ifp *elmi_if = NULL;
  s_int16_t ret = 0;
  struct avl_node *avl_port = NULL;
  struct elmi_evc_status *evc_status_info = NULL;

  bridge = elmi_find_bridge (br_name);

  if (!bridge)
    return ELMI_ERR_BRIDGE_NOT_FOUND;

  /* Check if EVC name size exceed 100 characters */
  if ((pal_strlen (evc_id) > ELMI_EVC_NAME_LEN))
    {
      return ELMI_ERR_INVALID_EVC_LEN;
    }

  /* Get the list of ports on which elmi is enabled */
  AVL_TREE_LOOP (bridge->port_tree, elmi_if, avl_port)
    {
      if (elmi_if->elmi_enabled)
        {
          evc_status_info = elmi_lookup_evc_by_name (elmi_if, evc_id);
          if (evc_status_info)
            ret = elmi_display_evc_detail_info (cli, elmi_if, 
                                                evc_status_info);
        }

    }

  return ret;

}

int
elmi_display_br_evc_detail_by_vid (struct cli *cli, u_int8_t *br_name, 
                                   u_int16_t evc_ref_id)
{

  struct elmi_bridge *bridge = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct avl_node *avl_port = NULL;
  struct elmi_evc_status *evc_status_info = NULL;
  s_int16_t ret = 0;

  bridge = elmi_find_bridge (br_name);

  if (bridge == NULL)
    return ELMI_ERR_BRIDGE_NOT_FOUND;

  if (evc_ref_id  < ELMI_VLAN_MIN || evc_ref_id > ELMI_VLAN_MAX)
    return ELMI_ERR_INVALID_EVC_LEN;

  /* Get the list of ports on which elmi is enabled */
  AVL_TREE_LOOP (bridge->port_tree, elmi_if, avl_port)
    {
      if (elmi_if->elmi_enabled)
        {
          evc_status_info = elmi_evc_look_up (elmi_if, evc_ref_id);
          if (evc_status_info)
            ret = elmi_display_evc_detail_info (cli, elmi_if, 
                                                evc_status_info);
        }
    }

   return ret;
}

int 
elmi_display_uni_info (struct cli *cli, u_int8_t *if_name)
{
  struct elmi_port *port = NULL;
  struct elmi_uni_info *uni_info = NULL;
  struct elmi_bw_profile *uni_bw = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct interface *ifp = NULL;
  struct apn_vr *vr = NULL;
  u_int8_t buf[5];
  u_int8_t *ptr = NULL;
  u_char *map_str = NULL;

  ptr = buf;

  if (!if_name)
    return ELMI_ERR_PORT_NOT_FOUND;

  vr = apn_vr_get_privileged (ZG);

  ifp = if_lookup_by_name (&vr->ifm, if_name);

  if (!ifp || !ifp->info)
    return ELMI_ERR_PORT_NOT_FOUND;

  elmi_if = ifp->info;

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  port  =  elmi_if->port;
  if (!port)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  uni_info = elmi_if->uni_info;
  if (!uni_info)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  cli_out (cli, " UNI Id:      %s\n", uni_info->uni_id);

  map_str = elmi_map_service_attr_to_str (uni_info->cevlan_evc_map_type);

  if (map_str)
    cli_out (cli, " CE-VLAN/EVC Map Type:    %s\n\n", map_str);

  /* Display UNI BW profile parameters */
  uni_bw = uni_info->uni_bw;

  if (uni_bw)
    {
      cli_out (cli, " Bandwidth Profile Per UNI\n");
      cli_out (cli, " %-8s%-8s%-8s%-8s%-16s%-12s\n", "CIR", "CBS", "EIR",
               "EBS", "Coupling-flag", "Color-mode");
      cli_out (cli, " ======= ======= ======= ======= =============== "
               "============ \n");

      cli_out (cli, " %-8d%-8d%-8d%-8d%-16s%-12s\n",
               uni_bw->cir, uni_bw->cbs, uni_bw->eir, uni_bw->ebs,  
               (uni_bw->cf == PAL_TRUE)? "enable" : "disable",
               (uni_bw->cm == PAL_TRUE)? "color-aware" : "color-blind");

      cli_out (cli, " \n");
    }

  /* Display UNI information */
  elmi_display_evc_info (cli, elmi_if); 
  
  return RESULT_OK;
}

int
elmi_display_statistics_if (struct cli *cli, struct elmi_ifp *elmi_if)
{
  struct elmi_port *port = NULL;
  struct interface *ifp = NULL;
  u_char timebuf[TIME_BUF];

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  if (!elmi_if || !elmi_if->ifp)
    return ELMI_ERR_PORT_NOT_FOUND;

  ifp = elmi_if->ifp;
    if (!ifp)
      return ELMI_ERR_PORT_NOT_FOUND;

  if (!elmi_if->elmi_enabled)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  port  =  elmi_if->port;
  if (!port)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  if (!elmi_if->uni_info)
    return ELMI_ERR_NO_VALID_CONFIG;

  pal_mem_set (timebuf, 0, TIME_BUF);

  cli_out (cli, "\n");
  cli_out (cli, "ELMI statistics for interface %s\n", elmi_if->ifp->name);
  cli_out (cli, "\n");

  cli_out (cli, "Ether LMI Link Status: %s\n", 
           ((port->elmi_operational_state) ?  "UP": "DOWN"));

  cli_out (cli, "UNI Id: %s\n", 
      elmi_if->uni_info->uni_id); 

  cli_out (cli, "------\n");

  cli_out (cli, "Reliability Errors:\n");

  if (elmi_if->uni_mode == ELMI_UNI_MODE_UNIC)
    {
      cli_out (cli, " Status Timeouts         %d\t\t"
               "Invalid Sequence Number    %d\n",
               port->stats.status_enq_timeouts,   
               port->stats.inval_seq_num);

      cli_out (cli, " Invalid Status Response %d\t\t" 
               "Unsolicit Status Rcvd      %d\n",
               port->stats.inval_status_resp,   
               port->stats.unsolicited_status_rcvd);
    }
  else
    {
      cli_out (cli, " Status Enq Timeouts      %d\t\t"
               "Invalid Sequence Number    %d\n",
               port->stats.status_enq_timeouts,   
               port->stats.inval_seq_num);
    }

  cli_out (cli, "\nProtocol Errors:\n");

  cli_out (cli, " Invalid Protocol Version %d\t\t"
           "Invalid EVC Reference Id   %d\n",
           port->stats.inval_protocol_ver, 
           port->stats.inval_evc_ref_id);

  cli_out (cli, " Invalid Message Type     %d\t\t"
           "Out of Sequence IE         %d\n",
           port->stats.inval_msg_type,
           port->stats.outof_seq_ie);

  cli_out (cli, " Duplicated IE            %d\t\t"
           "Mandatory IE Missing       %d\n",
           port->stats.duplicate_ie, 
           port->stats.mandatory_ie_missing);

  cli_out (cli, " Invalid Mandatory IE     %d\t\t"
           "Invalid non-Mandatory IE   %d\n", 
           port->stats.inval_mandatory_ie, 
           port->stats.inval_non_mandatory_ie);

  cli_out (cli, " Unrecognized IE          %d\t\t"
           "Unexpected IE              %d\n",
           port->stats.unrecognized_ie, 
           port->stats.unexpected_ie);

  cli_out (cli, " Short Message            %d\n", port->stats.short_msg);

  if (elmi_if->uni_mode == ELMI_UNI_MODE_UNIC)
    {
      if (!port->stats.last_full_status_enq_sent)
        pal_snprintf (timebuf , sizeof("00:00:00"), "00:00:00");
      else
        timeutil_uptime (timebuf, TIME_BUF,
                         (pal_time_current (NULL) - 
                         port->stats.last_full_status_enq_sent));

      cli_out (cli, " Last Full Status Enq Sent: %-8s", timebuf);

      if (!port->stats.last_full_status_rcvd)
        pal_snprintf (timebuf , sizeof("00:00:00"), "00:00:00");
      else 
        timeutil_uptime (timebuf, TIME_BUF,
                         (pal_time_current (NULL) - 
                         port->stats.last_full_status_rcvd));

      cli_out (cli, "    Last Full Status Rcvd: %-8s\n", timebuf);

      if (!port->stats.last_status_check_sent)
        pal_snprintf (timebuf , sizeof("00:00:00"), "00:00:00");
      else
        timeutil_uptime (timebuf, TIME_BUF,
                         (pal_time_current (NULL) - 
                         port->stats.last_status_check_sent));

      cli_out (cli, " Last Status Check Sent: %-8s", timebuf);

      if (!port->stats.last_status_check_rcvd)
        pal_snprintf (timebuf , sizeof("00:00:00"), "00:00:00");
      else
        timeutil_uptime (timebuf, TIME_BUF,
                         (pal_time_current (NULL) - 
                         port->stats.last_status_check_rcvd));

      cli_out (cli, "       Last Status Check Rcvd : %-8s\n", timebuf);
    }
  else
    {
      if (!port->stats.last_full_status_enq_rcvd)
        pal_snprintf (timebuf , sizeof("00:00:00"), "00:00:00");
      else
        timeutil_uptime (timebuf, TIME_BUF,
                         (pal_time_current (NULL) - 
                         port->stats.last_full_status_enq_rcvd));

      cli_out (cli, " Last Full Status Enq Rcvd : %-8s\n", timebuf);
    }

  if (!port->stats.last_counters_cleared)
    pal_snprintf (timebuf , sizeof("00:00:00"), "00:00:00");
  else
    timeutil_uptime (timebuf, TIME_BUF,
                     (pal_time_current (NULL) - 
                     port->stats.last_counters_cleared));

  cli_out (cli, " Last counters cleared : %-8s\n", timebuf);

  return RESULT_OK;

}

int
elmi_display_statistics_bridge (struct cli *cli, u_int8_t *br_name)
{
  struct elmi_bridge *bridge = NULL;
  struct elmi_ifp *elmi_ifp = NULL;
  s_int16_t ret = 0;
  struct avl_node *avl_port = NULL;

  bridge = elmi_find_bridge (br_name);

  if (bridge == NULL)
    return ELMI_ERR_BRIDGE_NOT_FOUND;

  /* Get the list of ports on which elmi is enabled */
  AVL_TREE_LOOP (bridge->port_tree, elmi_ifp, avl_port)
    {
      if (elmi_ifp->elmi_enabled)
        {
          ret = elmi_display_statistics_if (cli, elmi_ifp);
        }
    }

  return RESULT_OK;

}

int
elmi_clear_statistics_interface (struct cli *cli, struct elmi_ifp *elmi_if)
{
  struct elmi_port *port = NULL;
  struct interface *ifp = NULL;

  if (!elmi_if)
    return ELMI_ERR_PORT_NOT_FOUND;

  if (!elmi_if || !elmi_if->ifp)
    return ELMI_ERR_PORT_NOT_FOUND;

  ifp = elmi_if->ifp;
  if (!ifp)
    return ELMI_ERR_PORT_NOT_FOUND;

  if (!elmi_if->elmi_enabled)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  port  =  elmi_if->port;
  if (!port)
    return ELMI_ERR_ELMI_NOT_ENABLED;

  port->stats.status_enq_timeouts = 0;
  port->stats.inval_seq_num = 0;
  port->stats.inval_status_resp = 0;
  port->stats.unsolicited_status_rcvd = 0;

  port->stats.inval_protocol_ver = 0;
  port->stats.inval_msg_type = 0;
  port->stats.duplicate_ie = 0;
  port->stats.inval_mandatory_ie = 0;
  port->stats.unrecognized_ie = 0;
  port->stats.short_msg = 0;
  port->stats.inval_evc_ref_id = 0;
  port->stats.outof_seq_ie = 0;

  port->stats.mandatory_ie_missing = 0;
  port->stats.inval_non_mandatory_ie = 0;
  port->stats.unexpected_ie = 0;
  port->stats.last_full_status_enq_sent = 0;
  port->stats.last_full_status_rcvd = 0;
  port->stats.last_status_check_sent = 0;
  port->stats.last_counters_cleared = pal_time_current (0);

  cli_out (cli, "Cleared statistics for Interface %s\n", ifp->name);

  return RESULT_OK;
}

int
elmi_clear_statistics_bridge (struct cli *cli, u_int8_t *br_name)
{

  struct elmi_bridge *bridge = NULL;
  struct elmi_ifp *elmi_ifp = NULL;
  s_int16_t ret = 0;
  struct avl_node *avl_port = NULL;

  bridge = elmi_find_bridge (br_name);

  if (!bridge)
    return ELMI_ERR_BRIDGE_NOT_FOUND;

  if (bridge->bridge_type != ELMI_BRIDGE_TYPE_MEF_CE)
    return ELMI_ERR_WRONG_PORT_TYPE;

  /* Get the list of ports on which elmi is enabled */
  AVL_TREE_LOOP (bridge->port_tree, elmi_ifp, avl_port)
    {
      if (elmi_ifp->elmi_enabled)
        {
          ret = elmi_clear_statistics_interface (cli, elmi_ifp);
        }
    }

  return ret;
}

void
elmi_cli_show_init (struct lib_globals * ezg)
{

  struct cli_tree *ctree = ezg->ctree;
  /* install show clis */
  cli_install (ctree, EXEC_MODE, &show_elmi_lmi_parameters_interface_cmd);
  cli_install (ctree, EXEC_MODE, &show_elmi_evc_bridge_cmd);
  cli_install (ctree, EXEC_MODE, &show_elmi_evc_id_detail_bridge_cmd);
  cli_install (ctree, EXEC_MODE, &show_elmi_evc_ref_id_detail_bridge_cmd);
  cli_install (ctree, EXEC_MODE, &show_elmi_evc_interface_cmd);
  cli_install (ctree, EXEC_MODE, &show_elmi_evc_id_interface_cmd);
  cli_install (ctree, EXEC_MODE, &show_elmi_evc_ref_id_interface_cmd);
  cli_install (ctree, EXEC_MODE, &show_elmi_elmi_evc_cmd);
  cli_install (ctree, EXEC_MODE, &show_elmi_uni_interface_cmd);
  cli_install (ctree, EXEC_MODE, &show_elmi_statistics_interface_cmd);
  cli_install (ctree, EXEC_MODE, &clear_elmi_statistics_interface_cmd);
  cli_install (ctree, EXEC_MODE, &show_elmi_statsitics_br_cmd);
  cli_install (ctree, EXEC_MODE, &clear_elmi_statsitics_br_cmd);
}
