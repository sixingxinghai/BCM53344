/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_VR

#include "cli.h"
#include "thread.h"
#include "imi_client.h"
#include "modbmap.h"

#include "nsm/nsmd.h"
#include "nsm/rib/rib.h"
#include "nsm/nsm_api.h"
#include "nsm/nsm_cli.h"
#include "nsm/nsm_interface.h"
#include "nsm/nsm_router.h"
#include "nsm/nsm_server.h"
#ifdef HAVE_SNMP
#include "nsm/nsm_snmp.h"
#endif /* HAVE_SNMP */

int show_ip_route (struct cli *, int, char **);
int show_ip_route_protocol (struct cli *, int, char **);

int
nsm_validate_vr_name(char *name)
{
  int i;

  if (NULL == name)
    return -1;

  /* Check restricted names. */
   if (!pal_strcasecmp(name, "all") || !pal_strcasecmp(name, "pvr"))
     return -2;

  for(i=0;i<pal_strlen(name);i++)
  {
    if ((name[i] == '-') || (name[i]=='_'))
      continue;
    if (name[i]<'0')
      return -1;
    if ((name[i]>'9') && (name[i]<'A') )
      return -1;
    if ((name[i]>'Z') && (name[i]<'a') )
      return -1;
    if( (name[i]>'z') )
      return -1;
  }
  return 0;
}


int
nsm_virtual_router_set (char *name)
{
  struct apn_vr *vr;
  struct nsm_master *nm;
  fib_id_t fib_id;
  int ret;

  /* Validate name length */
  if (pal_strlen (name) > LIB_VR_MAX_NAMELEN)
    return NSM_API_SET_ERR_VR_INVALID_NAMELEN;

  ret = nsm_validate_vr_name(name);

  if (ret == -2)
    return NSM_API_SET_ERR_VR_RESERVED_NAME;
  else if (ret < 0)
    return NSM_API_SET_ERR_VR_INVALID_NAME;

  /* First check if there is available FIB. */
  fib_id = nsm_fib_id_get (nzg);
  if (fib_id > FIB_ID_MAX)
    return NSM_API_SET_ERR_VRF_CANT_CREATE;

  vr = apn_vr_get_by_name (nzg, name);
  nm = nsm_master_lookup_by_id (nzg, vr->id);
  if (nm == NULL)
    {
      nm = nsm_master_init (vr);
      if (nm == NULL)
        return NSM_API_SET_ERR_VR_CANT_CREATE;

      /* Read the configuration.  */
      HOST_CONFIG_VR_START (vr);
    }
#ifdef HAVE_IMI
  nsm_server_send_vr_add_by_proto (nm, APN_PROTO_IMI);
#endif
#ifdef HAVE_SNMP
  nm->last_change_time = pal_time_current (NULL);
  nsm_ent_config_chg_trap ();
#endif /* HAVE_SNMP */
  return NSM_API_SET_SUCCESS;
}

int
nsm_virtual_router_unset (char *name)
{
  struct apn_vr *vr;
  struct nsm_master *nm;

  vr = apn_vr_lookup_by_name (nzg, name);
  if (vr == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  nm = nsm_master_lookup_by_id (nzg, vr->id);
  if (nm != NULL)
    {
      /* Connection close callback.  This callback should be called
         before the address deletion call to make sure the VTY client
         receives the TCP FIN message.  */
      if (vr->zg->vr_callback[VR_CALLBACK_CLOSE])
        (*vr->zg->vr_callback[VR_CALLBACK_CLOSE]) (vr);

      /* Finish NSM master. */
      nsm_master_finish (vr);
    }

  return NSM_API_SET_SUCCESS;
}

/* Virtual Router. */
CLI (virtual_router,
     virtual_router_cmd,
     "virtual-router WORD",
     CLI_VR_STR,
     CLI_VR_NAME_STR)
{
  int ret;
  char *name = argv[0];

  ret = nsm_virtual_router_set (name);
  if (ret != NSM_API_SET_SUCCESS)
    return nsm_cli_return (cli, ret);

  cli->mode = VR_MODE;
  cli->index = apn_vr_lookup_by_name (nzg, name);

  return CLI_SUCCESS;
}

CLI (no_virtual_router,
     no_virtual_router_cmd,
     "no virtual-router WORD",
     CLI_NO_STR,
     CLI_VR_STR,
     CLI_VR_NAME_STR)
{
  int ret;

  ret = nsm_virtual_router_unset (argv[0]);

  return nsm_cli_return (cli, ret);
}

CLI (virtual_router_desc,
     virtual_router_desc_cmd,
     "description LINE",
     "VR specific description",
     "Characters desribing this VR")
{
  struct apn_vr *vr = cli->index;
  struct nsm_master *nm = vr->proto;

  if (nm->desc)
    XFREE (MTYPE_NSM_DESC, nm->desc);

  nm->desc = XSTRDUP (MTYPE_NSM_DESC, argv[0]);

  return CLI_SUCCESS;
}

void
nsm_server_send_vr_add_by_proto (struct nsm_master *nm, u_int32_t proto)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  struct interface *ifp;
  struct connected *ifc;
  struct listnode *node;
  struct apn_vrf *vrf;
  int i;

  if (! MODBMAP_ISSET (nm->module_bits, proto))
    {
      MODBMAP_SET (nm->module_bits, proto);

      NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
        if (nse->service.protocol_id == proto)
          {
            nsm_server_send_vr_add (nse, nm->vr);
            vrf = apn_vrf_lookup_default (nm->vr);
            if (vrf)
              nsm_server_send_vrf_add (nse, vrf);

            LIST_LOOP (nm->vr->ifm.if_list, ifp, node)
             {
               nsm_server_send_interface_add (nse, ifp);
               /* Send interface address information.  */
              for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
                if (CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
                  nsm_server_send_interface_address (nse, ifc, NSM_IF_ADDRESS_ADD);
#ifdef HAVE_IPV6
              for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
                if (CHECK_FLAG (ifc->conf, NSM_IFC_REAL))
                  nsm_server_send_interface_address (nse, ifc, NSM_IF_ADDRESS_ADD);
#endif /* HAVE_IPV6 */
             }

            if (nsm_service_check (nse, NSM_SERVICE_ROUTER_ID))
              if (vrf)
                nsm_server_send_router_id_update (vrf->vr->id, vrf->id,
                                                   nse, vrf->router_id);
          }
    }
}

void
nsm_server_send_vr_delete_by_proto (struct nsm_master *nm, u_int32_t proto)
{
  struct nsm_server_client *nsc;
  struct nsm_server_entry *nse;
  int i;

  if (MODBMAP_ISSET (nm->module_bits, proto))
    {
      MODBMAP_UNSET (nm->module_bits, proto);

      NSM_SERVER_CLIENT_LOOP (ng, i, nsc, nse)
        if (nse->service.protocol_id == proto)
          nsm_server_send_vr_delete (nse, nm->vr);
    }
}

#ifdef HAVE_MCAST_IPV4
#define MCAST_VR_LOAD_STR         "|pimsm|pimdm"

#define MCAST_VR_LOAD_HLP_STR                                             \
  , CLI_PIM_STR " " CLI_PIMSM_STR,                                        \
    CLI_PIM_STR " " CLI_PIMDM_STR
#else
#define MCAST_VR_LOAD_STR         ""

#define MCAST_VR_LOAD_HLP_STR     ""
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
#define MCAST6_VR_LOAD_STR         "|pimsm6|pimdm"

#define MCAST6_VR_LOAD_HLP_STR                                             \
  , CLI_PIM_STR " " CLI_PIMSM6_STR,                                        \
    CLI_PIM_STR " " CLI_PIMDM6_STR
#else
#define MCAST6_VR_LOAD_STR         ""

#define MCAST6_VR_LOAD_HLP_STR     ""
#endif /* HAVE_MCAST_IPV4 */


CLI (virtual_router_load_module,
     virtual_router_load_module_cmd,
     "load (ospf|bgp|rip|isis" MCAST_VR_LOAD_STR ")",
     "Load module to VR",
     "Open Shortest Path First (OSPF)",
     "Border Gateway Protocol (BGP)",
     "Routing Information Protocol (RIP)",
     "Intermediate System - Intermediate System (IS-IS)" MCAST_VR_LOAD_HLP_STR )
{
  struct apn_vr *vr = cli->index;
  struct nsm_master *nm = vr->proto;
  u_int32_t proto = 0;

#ifdef HAVE_MCAST_IPV4
  if (pal_strcmp (argv[0], "pimsm") == 0)
    proto = APN_PROTO_PIMSM;
  else if (pal_strcmp (argv[0], "pimdm") == 0)
    proto = APN_PROTO_PIMDM;
#endif /* HAVE_MCAST_IPV4 */

  if (pal_strncmp (argv[0], "o", 1) == 0)
    proto = APN_PROTO_OSPF;
  else if (pal_strncmp (argv[0], "b", 1) == 0)
    proto = APN_PROTO_BGP;
  else if (pal_strncmp (argv[0], "r", 1) == 0)
    proto = APN_PROTO_RIP;
  else if (pal_strncmp (argv[0], "i", 1) == 0)
    proto = APN_PROTO_ISIS;

  if (proto)
    nsm_server_send_vr_add_by_proto (nm, proto);

  return CLI_SUCCESS;
}

#ifdef HAVE_IPV6
CLI (virtual_router_load_module_ipv6,
     virtual_router_load_module_ipv6_cmd,
     "load ipv6 (ospf|rip" MCAST6_VR_LOAD_STR ")",
     "Load module to VR",
     "Internet Protocol Version 6",
     "Open Shortest Path First (OSPF)",
     "Routing Information Protocol (RIP)" MCAST6_VR_LOAD_HLP_STR )
{
  struct apn_vr *vr = cli->index;
  struct nsm_master *nm = vr->proto;
  u_int32_t proto = 0;

#ifdef HAVE_MCAST_IPV6
  if (pal_strcmp (argv[0], "pimsm6") == 0)
    proto = APN_PROTO_PIMSM6;
  else if (pal_strcmp (argv[0], "pimdm") == 0)
    proto = APN_PROTO_PIMDM;
#endif /* HAVE_MCAST_IPV6 */

  if (pal_strncmp (argv[0], "o", 1) == 0)
    proto = APN_PROTO_OSPF6;
  else if (pal_strncmp (argv[0], "r", 1) == 0)
    proto = APN_PROTO_RIPNG;

  if (proto)
    nsm_server_send_vr_add_by_proto (nm, proto);

  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */

CLI (no_virtual_router_load_module,
     no_virtual_router_load_module_cmd,
     "no load (ospf|bgp|rip|isis" MCAST_VR_LOAD_STR ")",
     CLI_NO_STR,
     "Load module to VR",
     "Open Shortest Path First (OSPF)",
     "Border Gateway Protocol (BGP)",
     "Routing Information Protocol (RIP)",
     "Intermediate System - Intermediate System (IS-IS)" MCAST_VR_LOAD_HLP_STR )
{
  struct apn_vr *vr = cli->index;
  struct nsm_master *nm = vr->proto;
  u_int32_t proto = 0;

#ifdef HAVE_MCAST_IPV4
  if (pal_strcmp (argv[0], "pimsm") == 0)
    proto = APN_PROTO_PIMSM;
  else if (pal_strcmp (argv[0], "pimdm") == 0)
    proto = APN_PROTO_PIMDM;
#endif /* HAVE_MCAST_IPV4 */

  if (pal_strncmp (argv[0], "o", 1) == 0)
    proto = APN_PROTO_OSPF;
  else if (pal_strncmp (argv[0], "b", 1) == 0)
    proto = APN_PROTO_BGP;
  else if (pal_strncmp (argv[0], "r", 1) == 0)
    proto = APN_PROTO_RIP;
  else if (pal_strncmp (argv[0], "i", 1) == 0)
    proto = APN_PROTO_ISIS;

  if (proto)
    nsm_server_send_vr_delete_by_proto (nm, proto);

  return CLI_SUCCESS;
}

#ifdef HAVE_IPV6
CLI (no_virtual_router_load_module_ipv6,
     no_virtual_router_load_module_ipv6_cmd,
     "no load ipv6 (ospf|rip" MCAST6_VR_LOAD_STR ")",
     CLI_NO_STR,
     "Load module to VR",
     "Internet Protocol Version 6",
     "Open Shortest Path First (OSPF)",
     "Routing Information Protocol (RIP)" MCAST6_VR_LOAD_HLP_STR )
{
  struct apn_vr *vr = cli->index;
  struct nsm_master *nm = vr->proto;
  u_int32_t proto = 0;

#ifdef HAVE_MCAST_IPV6
  if (pal_strcmp (argv[0], "pimsm6") == 0)
    proto = APN_PROTO_PIMSM6;
  else if (pal_strcmp (argv[0], "pimdm") == 0)
    proto = APN_PROTO_PIMDM;
#endif /* HAVE_MCAST_IPV6 */

  if (pal_strncmp (argv[0], "o", 1) == 0)
    proto = APN_PROTO_OSPF6;
  else if (pal_strncmp (argv[0], "r", 1) == 0)
    proto = APN_PROTO_RIPNG;

  if (proto)
    nsm_server_send_vr_delete_by_proto (nm, proto);

  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */

CLI (virtual_router_enable,
     virtual_router_enable_cmd,
     "enable-vr",
     "Enable this Virtual Router")
{
  struct apn_vr *vr = cli->index;
  struct nsm_master *nm = vr->proto;

  nsm_server_send_vr_add_by_proto (nm, APN_PROTO_OSPF);
  nsm_server_send_vr_add_by_proto (nm, APN_PROTO_BGP);
  nsm_server_send_vr_add_by_proto (nm, APN_PROTO_RIP);
  nsm_server_send_vr_add_by_proto (nm, APN_PROTO_ISIS);
  nsm_server_send_vr_add_by_proto (nm, APN_PROTO_OSPF6);
  nsm_server_send_vr_add_by_proto (nm, APN_PROTO_RIPNG);

#ifdef HAVE_MCAST_IPV4
  nsm_server_send_vr_add_by_proto (nm, APN_PROTO_PIMSM);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  nsm_server_send_vr_add_by_proto (nm, APN_PROTO_PIMSM6);
#endif /* HAVE_MCAST_IPV6 */

#if defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6
  nsm_server_send_vr_add_by_proto (nm, APN_PROTO_PIMDM);
#endif /* define HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6 */

  return CLI_SUCCESS;
}

CLI (virtual_router_disable,
     virtual_router_disable_cmd,
     "disable-vr",
     "Disable this Virtual Router")
{
  struct apn_vr *vr = cli->index;
  struct nsm_master *nm = vr->proto;

  nsm_server_send_vr_delete_by_proto (nm, APN_PROTO_OSPF);
  nsm_server_send_vr_delete_by_proto (nm, APN_PROTO_BGP);
  nsm_server_send_vr_delete_by_proto (nm, APN_PROTO_RIP);
  nsm_server_send_vr_delete_by_proto (nm, APN_PROTO_ISIS);
  nsm_server_send_vr_delete_by_proto (nm, APN_PROTO_OSPF6);
  nsm_server_send_vr_delete_by_proto (nm, APN_PROTO_RIPNG);

#ifdef HAVE_MCAST_IPV4
  nsm_server_send_vr_delete_by_proto (nm, APN_PROTO_PIMSM);
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
  nsm_server_send_vr_delete_by_proto (nm, APN_PROTO_PIMSM6);
#endif /* HAVE_MCAST_IPV6 */

#if defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6
  nsm_server_send_vr_delete_by_proto (nm, APN_PROTO_PIMDM);
#endif /* define HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6 */

  return CLI_SUCCESS;
}


int
nsm_vr_forwarding_set (char *ifname, char *vr_name)
{
  struct nsm_master *nm;
  struct nsm_master *def_nm = NULL;
  struct interface *ifp;
  struct entPhysicalEntry *entPhy = NULL;
  struct listnode *node = NULL;
  struct apn_vr *vr = NULL;

  ifp = ifg_lookup_by_name (&nzg->ifg, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  if (! CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
    return NSM_API_SET_ERR_IF_NOT_ACTIVE;

  nm = nsm_master_lookup_by_name (nzg, vr_name);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  if (ifp->vr->id != 0)
    return NSM_API_SET_ERR_VR_BOUND;

  def_nm = nsm_master_lookup_by_id (nzg, 0);
  vr = apn_vr_lookup_by_name (nzg, vr_name);
  if (vr == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  if (def_nm)
    LIST_LOOP (def_nm->phyEntityList, entPhy, node)
      if (! (pal_strcmp (entPhy->entPhysicalName, ifp->name)))
        {
          listnode_add (vr->mappedPhyEntList, entPhy);
          break;
        }

  nsm_if_bind_vr (ifp, nm);

  return NSM_API_SET_SUCCESS;
}

int
nsm_vr_forwarding_unset (char *ifname, char *vr_name)
{
  struct nsm_master *nm;
  struct interface *ifp;
  struct nsm_master *def_nm = NULL;
  struct apn_vr *vr = NULL;
  struct entPhysicalEntry *entPhy = NULL;
  struct listnode *node = NULL;

  ifp = ifg_lookup_by_name (&nzg->ifg, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  if (! CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
    return NSM_API_SET_ERR_IF_NOT_ACTIVE;

  nm = nsm_master_lookup_by_name (nzg, vr_name);
  if (nm == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  if (ifp->vr->id == 0)
    return NSM_API_SET_ERR_VR_NOT_BOUND;

  if (ifp->vr != nm->vr)
    return NSM_API_SET_ERR_DIFFERENT_VR_BOUND;

  def_nm = nsm_master_lookup_by_id (nzg, 0);
  vr = apn_vr_lookup_by_name (nzg, vr_name);
  if (vr == NULL)
    return NSM_API_SET_ERR_VR_NOT_EXIST;

  if (def_nm)
    LIST_LOOP (def_nm->phyEntityList, entPhy, node)
      if (! (pal_strcmp (entPhy->entPhysicalName, ifp->name)))
        {
          listnode_delete (vr->mappedPhyEntList, entPhy);
          break;
        }

  nsm_if_unbind_vr (ifp, nm);

  return NSM_API_SET_SUCCESS;
}

CLI (no_virtual_router_desc,
     no_virtual_router_desc_cmd,
     "no description",
     CLI_NO_STR,
     "VR specific description")
{
  struct apn_vr *vr = cli->index;
  struct nsm_master *nm = vr->proto;

  if (nm->desc)
    {
      XFREE (MTYPE_NSM_DESC, nm->desc);
      nm->desc = NULL;
    }

  return CLI_SUCCESS;
}

CLI (virtual_router_forwarding,
     virtual_router_forwarding_cmd,
     "virtual-router forwarding WORD",
     CLI_VR_STR,
     "Associate this interface with specific Virtual Router",
     CLI_VR_NAME_STR)
{
  struct interface *ifp = cli->index;
  int ret;

  if (if_is_loopback (ifp))
    {
      cli_out (cli, "%% Loopback interface can not be bound to VR\n");
      return CLI_ERROR;
    }

  ret = nsm_vr_forwarding_set (ifp->name, argv[0]);

  return nsm_cli_return (cli, ret);
}

CLI (no_virtual_router_forwarding,
     no_virtual_router_forwarding_cmd,
     "no virtual-router forwarding WORD",
     CLI_NO_STR,
     CLI_VR_STR,
     "Associate this interface with specific Virtual Router",
     CLI_VR_NAME_STR)
{
  struct interface *ifp = cli->index;
  int ret;

  if (if_is_loopback (ifp))
    {
      cli_out (cli, "%% Loopback interface can not be unbound from VR\n");
      return CLI_ERROR;
    }

  ret = nsm_vr_forwarding_unset (ifp->name, argv[0]);

  return nsm_cli_return (cli, ret);
}


CLI (show_virtual_router_ip_route,
     show_virtual_router_ip_route_cmd,
     "show virtual-router WORD ip route (database|)",
     CLI_SHOW_STR,
     "Virtual Router information",
     CLI_VR_NAME_STR,
     CLI_IP_STR,
     "IP routing table",
     "IP routing table database")
{
  struct apn_vr *vr, *pvr;

  vr = apn_vr_lookup_by_name (nzg, argv[0]);
  if (vr == NULL)
    {
      cli_out (cli, "%% No such VR\n");
      return CLI_ERROR;
    }

  pvr = cli->vr;
  cli->vr = vr;

#ifdef HAVE_L3
  show_ip_route (cli, argc - 1, &argv[1]);
#endif /*HAVE_L3 */

  cli->vr = pvr;

  return CLI_SUCCESS;
}

ALI (show_virtual_router_ip_route,
     show_ip_route_virtual_router_cmd,
     "show ip route virtual-router WORD (database|)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IP routing table",
     "Virtual Router information",
     CLI_VR_NAME_STR,
     "IP routing table database");

CLI (show_virtual_router_ip_route_protocol,
     show_virtual_router_ip_route_protocol_cmd,
     "show virtual-router WORD ip route (database|) (bgp|connected|isis|kernel|ospf|rip|static)",
     CLI_SHOW_STR,
     "Virtual Router information",
     CLI_VR_NAME_STR,
     CLI_IP_STR,
     "IP routing table",
     "IP routing table database",
     "Border Gateway Protocol (BGP)",
     "Connected",
     "ISO IS-IS",
     "Kernel",
     "Open Shortest Path First (OSPF)",
     "Routing Information Protocol (RIP)",
     "Static routes")
{
  struct apn_vr *vr, *pvr;

  vr = apn_vr_lookup_by_name (nzg, argv[0]);
  if (vr == NULL)
    {
      cli_out (cli, "%% No such VR\n");
      return CLI_ERROR;
    }

  pvr = cli->vr;
  cli->vr = vr;

#ifdef HAVE_L3
  show_ip_route_protocol (cli, argc - 1, &argv[1]);
#endif /*HAVE_L3 */

  cli->vr = pvr;

  return CLI_SUCCESS;
}

ALI (show_virtual_router_ip_route_protocol,
     show_ip_route_virtual_router_protocol_cmd,
     "show ip route virtual-router WORD (database|) (bgp|connected|isis|kernel|ospf|rip|static)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "IP routing table",
     "Virtual Router information",
     CLI_VR_NAME_STR,
     "IP routing table database",
     "Border Gateway Protocol (BGP)",
     "Connected",
     "ISO IS-IS",
     "Kernel",
     "Open Shortest Path First (OSPF)",
     "Routing Information Protocol (RIP)",
     "Static routes");

void
nsm_show_virtual_router_brief (struct cli *cli)
{
  struct apn_vr *vr;
  struct interface *ifp;
  struct listnode *node;
  int count = 0;
  int i;

  for (i = 1; i < vector_max (nzg->vr_vec); i++)
    if ((vr = vector_slot (nzg->vr_vec, i)))
      {
        if (count == 0)
          {
            cli_out (cli, "\n");
            cli_out (cli, "Name            Router ID       Interfaces\n");
            cli_out (cli, "--------------- --------------- ----------------------\n");
          }

        /* VR name. */
        cli_out (cli, "%-15s ", vr->name);

        /* Router ID. */
        if (vr->router_id.s_addr)
          cli_out (cli, "%-15r ", &vr->router_id);
        else
          cli_out (cli, "%-15s ", "Unset");

        /* Interfaces. */
        if (listcount (vr->ifm.if_list) > 0)
          LIST_LOOP (vr->ifm.if_list, ifp, node)
            cli_out (cli, "%s ", ifp->name);

        cli_out (cli, "\n\n");

        count++;
      }

  if (count == 0)
    cli_out (cli, "%% No Virtual Routers are defined within NSM\n");
}

void
nsm_show_virtual_router_detail (struct cli *cli)
{
  struct apn_vr *vr;
  struct nsm_master *nm;
  struct interface *ifp;
  struct listnode *node;
  int i;
  int count = 0;

  for (i = 1; i < vector_max (nzg->vr_vec); i++)
    if ((vr = vector_slot (nzg->vr_vec, i)))
      {
        nm = vr->proto;

        cli_out (cli, "Virtual Router %s\n", vr->name);

        /* Description */
        if (nm->desc)
          cli_out (cli, "  Description: %s\n", nm->desc);

        /* Loaded Protocols */
        cli_out (cli, "  Loaded Protocols: \n");
        if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_BGP))
          cli_out (cli, "    bgp\n");
        if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_ISIS))
          cli_out (cli, "    isis\n");
        if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_OSPF))
          cli_out (cli, "    ospf\n");
        if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_RIP))
          cli_out (cli, "    rip\n");
#ifdef HAVE_MCAST_IPV4
        if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_PIMSM))
          cli_out (cli, "    pimsm\n");
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_IPV6
        if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_OSPF6))
          cli_out (cli, "    ipv6 ospf\n");
        if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_RIPNG))
          cli_out (cli, "    ipv6 rip\n");
#ifdef HAVE_MCAST_IPV6
        if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_PIMSM6))
          cli_out (cli, "    ipv6 pimsm6\n");
#endif /* HAVE_MCAST_IPV6 */
#endif /* HAVE_IPV6 */
#if defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6
        if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_PIMDM))
          cli_out (cli, "    pimdm\n");
#endif /* defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6 */

        /* VR ID */
        cli_out (cli, "  VR-ID: %d\n", vr->id);

        /* Router ID. */
        if (vr->router_id.s_addr)
          cli_out (cli, "  Router-ID: %r\n", &vr->router_id);
        else
          cli_out (cli, "  Router-ID: %s\n", "Unset");

        /* Interfaces. */
        cli_out (cli, "  Interfaces:\n");
        if (listcount (vr->ifm.if_list) > 0)
          LIST_LOOP (vr->ifm.if_list, ifp, node)
            cli_out (cli, "    %s\n", ifp->name);

        cli_out (cli, "\n");
        count++;
      }

  if (count == 0)
    cli_out (cli, "%% No Virtual Routers are defined within NSM\n");
}

CLI (show_virtual_router,
     show_virtual_router_cmd,
     "show virtual-router",
     CLI_SHOW_STR,
     CLI_VR_STR)
{
  nsm_show_virtual_router_detail (cli);

  return CLI_SUCCESS;
}

CLI (show_nsm_virtual_router,
     show_nsm_virtual_router_cmd,
     "show nsm virtual-router (brief|detail|)",
     CLI_SHOW_STR,
     "NSM Information",
     "Virtual Router Information",
     "Brief Information",
     "Detailed Information")
{
  if (argc > 0)
    {
      if (pal_strncmp (argv[0], "b", 1) == 0)
        nsm_show_virtual_router_brief (cli);
      else if (pal_strncmp (argv[0], "d", 1) == 0)
        nsm_show_virtual_router_detail (cli);
    }
  else
    nsm_show_virtual_router_brief (cli);

  return CLI_SUCCESS;
}

#ifndef HAVE_IMI
int
nsm_config_write_vr (struct cli *cli)
{
  struct apn_vr *vr;
  int write = 0;
  int i;

  if (cli->vr->id != 0)
    return write;

  for (i = 1; i < vector_max (nzg->vr_vec); i++)
    if ((vr = vector_slot (nzg->vr_vec, i)))
      {
        struct nsm_master *nm = vr->proto;

        if (write > 0)
          cli_out (cli, "!\n");

        cli_out (cli, "virtual-router %s\n", vr->name);

        if (nm->desc)
          cli_out (cli, " description %s\n", nm->desc);

        if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_RIP))
          cli_out (cli, " load rip\n");
        if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_OSPF))
          cli_out (cli, " load ospf\n");
        if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_ISIS))
          cli_out (cli, " load isis\n");
        if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_BGP))
          cli_out (cli, " load bgp\n");
#ifdef HAVE_MCAST_IPV4
        else if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_PIMSM))
          cli_out (cli, " load pimsm\n");
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_IPV6
        else if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_OSPF6))
          cli_out (cli, " load ipv6 ospf\n");
        else if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_RIPNG))
          cli_out (cli, " load ipv6 rip\n");
#ifdef HAVE_MCAST_IPV6
        else if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_PIMSM6))
          cli_out (cli, " load ipv6 pimsm6\n");
#endif /* HAVE_MCAST_IPV6 */
#endif /* HAVE_IPV6 */
#if defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6
        else if (MODBMAP_ISSET (nm->module_bits, APN_PROTO_PIMDM))
          cli_out (cli, " load pimdm\n");
#endif /* defined HAVE_MCAST_IPV4 || defined HAVE_MCAST_IPV6 */

        host_config_write_user_all_vr (cli, vr->host);

        write++;
      }

  return write;
}
#endif /* HAVE_IMI */

void
nsm_vr_cli_init (struct cli_tree *ctree)
{
  cli_install_default (ctree, VR_MODE);
#ifndef HAVE_IMI
  cli_install_config (ctree, VR_MODE, nsm_config_write_vr);
#endif /* HAVE_IMI */

  cli_install_gen (ctree, VR_MODE, PRIVILEGE_PVR_MAX, 0,
                   &virtual_router_desc_cmd);
  cli_install_gen (ctree, VR_MODE, PRIVILEGE_PVR_MAX, 0,
                   &no_virtual_router_desc_cmd);

  cli_install_gen (ctree, VR_MODE, PRIVILEGE_PVR_MAX, 0,
                   &virtual_router_load_module_cmd);
#ifdef HAVE_IPV6
  cli_install_gen (ctree, VR_MODE, PRIVILEGE_PVR_MAX, 0,
                   &virtual_router_load_module_ipv6_cmd);
#endif /* HAVE_IPV6 */
  cli_install_gen (ctree, VR_MODE, PRIVILEGE_PVR_MAX, 0,
                   &no_virtual_router_load_module_cmd);
#ifdef HAVE_IPV6
  cli_install_gen (ctree, VR_MODE, PRIVILEGE_PVR_MAX, 0,
                   &no_virtual_router_load_module_ipv6_cmd);
#endif /* HAVE_IPV6 */

  cli_install_gen (ctree, VR_MODE, PRIVILEGE_PVR_MAX, CLI_FLAG_HIDDEN,
                   &virtual_router_enable_cmd);
  cli_install_gen (ctree, VR_MODE, PRIVILEGE_PVR_MAX, CLI_FLAG_HIDDEN,
                   &virtual_router_disable_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_PVR_MAX, 0,
                   &virtual_router_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_PVR_MAX, 0,
                   &no_virtual_router_cmd);

  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_PVR_MAX, 0,
                   &virtual_router_forwarding_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_PVR_MAX, 0,
                   &no_virtual_router_forwarding_cmd);
#ifndef HAVE_IMI
  host_vr_cli_init (ctree);
#endif /* HAVE_IMI */
}

void
nsm_vr_show_init (struct cli_tree *ctree)
{
  /* "show virtual-router WORD ip route" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_PVR_MAX, 0,
                   &show_virtual_router_ip_route_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_PVR_MAX, 0,
                   &show_virtual_router_ip_route_protocol_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_PVR_MAX, CLI_FLAG_HIDDEN,
                   &show_ip_route_virtual_router_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_PVR_MAX, CLI_FLAG_HIDDEN,
                   &show_ip_route_virtual_router_protocol_cmd);

  /* "show nsm virtual-router" command. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_PVR_MAX, CLI_FLAG_HIDDEN,
                   &show_nsm_virtual_router_cmd);

  /* "show virtual-router" command. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_PVR_MAX, 0,
                   &show_virtual_router_cmd);
}

#endif /* HAVE_VR */
