/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "table.h"
#include "pal.h"
#include "lib.h"
#include "if.h"
#include "table.h"
#include "prefix.h"
#include "thread.h"
#include "nexthop.h"
#include "cli.h"
#include "host.h"
#include "snprintf.h"
#include "lib/ptree.h"

#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#include "nsmd.h"
#include "nsm_interface.h"
#ifdef HAVE_L3
#include "rib.h"
#include "nsm_debug.h"
#include "nsm_router.h"
#include "nsm_fea.h"
#include "nsm_server.h"
#include "nsm_api.h"
#include "nsm/nsm_cli.h"

CLI (show_ip_arp,
     show_ip_arp_cmd,
     "show arp",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_ARP_STR)
{
  nsm_arp_show (cli);

  return CLI_SUCCESS;
}

CLI (nsm_arp_configure,
     nsm_arp_configure_cmd,
     "arp A.B.C.D MAC (alias|)",
     CLI_ARP_STR,
     "IP address of the ARP entry",
     "Mac (hardware) address of the ARP entry in HHHH.HHHH.HHHH format",
     "Respond to ARP requests for the IP address")
{
  struct nsm_master *nm = cli->vr->proto;
  unsigned char mac_addr [ETHER_ADDR_LEN];
  struct pal_in4_addr addr;
  struct interface *ifp_ip;
  struct nsm_if *zif_ip = NULL;
  struct connected *ifc;
  struct prefix_ipv4 pfx4;
  int ret;
  u_int32_t mac1;
  u_int16_t mac2;

  pal_mem_set (&addr, 0, sizeof (struct pal_in4_addr));
  pal_mem_set (mac_addr, 0, sizeof(mac_addr));

  ret = pal_inet_pton (AF_INET, argv[0], &addr);
  if (ret == 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  if (pal_sscanf (argv[1], "%4hx.%4hx.%4hx",
                 (unsigned short *)&mac_addr[0],
                 (unsigned short *)&mac_addr[2],
                 (unsigned short *)&mac_addr[4]) != 3)
    {
      cli_out (cli, "%% Unable to translate mac address %s\n", argv[1]);
      return CLI_ERROR;
    }

  if (! nsm_api_mac_address_is_valid (argv[1]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n",
               argv[1]);
      return CLI_ERROR;
    }
 
  mac1 = *(u_int32_t *)&mac_addr[0];
  mac2 = *(u_int16_t *)&mac_addr[4];
  if ((mac1 == 0 && mac2 == 0) ||
      (mac1 == 0xffffffff && mac2 == 0xffff))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n", argv[1]);
      return CLI_ERROR;
    }

  ifp_ip = if_match_all_by_ipv4_address (&cli->vr->ifm, &addr, &ifc);
  if (! ifp_ip || if_is_loopback (ifp_ip))
    return nsm_cli_return (cli, NSM_API_SET_ERR_IF_NOT_EXIST);


  pfx4.family = AF_INET;
  pfx4.prefixlen = ifc->address->prefixlen;
  pfx4.prefix.s_addr = pal_ntoh32 (addr.s_addr);
  if (! nsm_cli_ipv4_addr_check (&pfx4, ifp_ip))
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  if (argc ==3)
    {
      /* Proxy ARP case */
      zif_ip = (struct nsm_if *)ifp_ip->info;

      if ( !zif_ip)
        return nsm_cli_return (cli, NSM_API_SET_ERR_IF_NOT_EXIST);

      /* Check whether proxy arp enabled for both zif_ip & zif_mac */
      if (! CHECK_FLAG( zif_ip->flags, NSM_IF_SET_PROXY_ARP))
        return nsm_cli_return (cli, NSM_API_SET_ERR_IF_NOT_PROXY_ARP);

      ret = nsm_api_arp_entry_add (nm, &addr, mac_addr, ifc, ifp_ip, PAL_TRUE);
    }
  else
    ret = nsm_api_arp_entry_add (nm, &addr, mac_addr, ifc, ifp_ip, PAL_FALSE);

  return nsm_cli_return (cli, ret);
}

CLI (no_nsm_arp_configure,
     no_nsm_arp_configure_cmd,
     "no arp A.B.C.D",
     CLI_NO_STR,
     CLI_ARP_STR,
     "IP address of the ARP entry")
{
  struct nsm_master *nm = cli->vr->proto;
  unsigned char mac_addr [ETHER_ADDR_LEN];
  struct pal_in4_addr addr;
  struct interface *ifp;
  struct connected *ifc;
  int ret;

  pal_mem_set (&addr, 0, sizeof (struct pal_in4_addr));
  pal_mem_set (mac_addr, 0, sizeof(mac_addr));

  ret = pal_inet_pton (AF_INET, argv[0], &addr);
  if (ret == 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  ifp = if_match_all_by_ipv4_address (&cli->vr->ifm, &addr, &ifc);
  if (! ifp || if_is_loopback (ifp))
    return nsm_cli_return (cli, NSM_API_SET_ERR_IF_NOT_EXIST);

  ret = nsm_api_arp_entry_del (nm, &addr, mac_addr, ifp);
  return nsm_cli_return (cli, ret);
}


CLI (clear_nsm_arp,
     clear_nsm_arp_cmd,
     "clear arp-cache",
     CLI_CLEAR_STR,
     "Arp cache")
{
  nsm_api_arp_del_all (cli->vr->proto, NSM_API_ARP_FLAG_DYNAMIC);
  return nsm_cli_return (cli, NSM_SUCCESS);
}
CLI (nsm_if_arp_ageing_timeout,
     nsm_if_arp_ageing_timeout_cmd,
     "arp-ageing-timeout <1-3000>",
     "Set arp age timeout value to interface",
     "ARP Ageing timeout in sec")
{
  struct interface *ifp = cli->index;
  int arp_ageing_timeout;
  int ret;

  NSM_CHECK_L3_COMMANDS(cli, ifp, l3_if_commands);

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  CLI_GET_INTEGER_RANGE ("ARP_AGEING_TIMEOUT", arp_ageing_timeout, argv[0], 1, 3000);

  ret = nsm_if_arp_ageing_timeout_set (cli->vr->id, ifp->name, arp_ageing_timeout);

  return nsm_cli_return (cli, ret);
}

CLI (no_nsm_if_arp_ageing_timeout,
     no_nsm_if_arp_ageing_timeout_cmd,
     "no arp-ageing-timeout",
     CLI_NO_STR,
     "Set default arp ageing timeout value to interface")
{
  struct interface *ifp = cli->index;
  int ret;

  NSM_CHECK_L3_COMMANDS(cli, ifp, l3_if_commands);

#ifdef HAVE_LACPD
  NSM_INTERFACE_CHECK_AGG_MEM_PROPERTY(cli, ifp);
#endif /* HAVE_LACPD */
  ret = nsm_if_arp_ageing_timeout_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}
CLI (nsm_if_proxy_arp,
     nsm_if_proxy_arp_cmd,
     "ip proxy-arp",
     CLI_IP_STR,
     "Proxy arp")
{
  int ret;
  struct nsm_if *zif;

  struct interface *ifp = cli->index;
  zif = (struct nsm_if *)ifp->info;

  if (! zif)
    return nsm_cli_return (cli, NSM_API_SET_ERR_IF_NOT_EXIST);

  SET_FLAG (zif->flags, NSM_IF_SET_PROXY_ARP);

  ret = nsm_if_proxy_arp_set (ifp, PAL_TRUE);

  if ( ret == RESULT_NO_SUPPORT)
    {
      cli_out (cli, "%% Proxy ARP is not supported for this platform\n");
      return CLI_ERROR;
    }
  else if ( ret == RESULT_ERROR)
    {
      cli_out (cli, "%% Unable to enable proxy arp %s\n");
      return CLI_ERROR;
    }
  return nsm_cli_return (cli, ret);
}

CLI (no_nsm_if_proxy_arp,
     no_nsm_if_proxy_arp_cmd,
     "no ip proxy-arp",
     CLI_NO_STR,
     CLI_IP_STR,
     "Proxy arp")
{
  int ret;
  struct nsm_if *zif;

  struct interface *ifp = cli->index;
  zif = (struct nsm_if *)ifp->info;

  if (! zif)
    return CLI_ERROR;

  UNSET_FLAG (zif->flags, NSM_IF_SET_PROXY_ARP);

  ret = nsm_if_proxy_arp_set (ifp, PAL_FALSE);

  if ( ret == RESULT_NO_SUPPORT)
    {
      cli_out (cli, "%% Proxy ARP is not supported for this platform\n");
      return CLI_ERROR;
    }
  else if ( ret == RESULT_ERROR)
    {
      cli_out (cli, "%% Unable to disable proxy arp %s\n");
      return CLI_ERROR;
    }
  return nsm_cli_return (cli, ret);
}

int
nsm_arp_config_write (struct cli *cli)
{
   struct nsm_master *nm = cli->vr->proto;
   struct nsm_arp_master *arp_master = NULL;
   struct ptree *arp_static_list = NULL;
   struct ptree_node *node      = NULL;
   struct nsm_static_arp_entry *arp_entry = NULL;
   char   ip_addr [INET_ADDRSTRLEN];
#ifdef HAVE_IPV6
   struct nsm_ipv6_static_nbr_entry *nbr_entry = NULL;
#endif /* HAVE_IPV6 */
   int write = 0;

   pal_mem_set (ip_addr, 0, INET_ADDRSTRLEN);

   if ( (!(arp_master = nm->arp)) ||
        (!(arp_static_list = arp_master->nsm_static_arp_list)))
     return 0;

   for (node = ptree_top (arp_static_list); node; node = ptree_next (node))
     {
       if (!(arp_entry = node->info))
         continue;

       if ( arp_entry->is_arp_proxy == PAL_TRUE)
         cli_out (cli,"arp %s %.04hx.%.04hx.%.04hx alias \n",
                  pal_inet_ntop (AF_INET, &arp_entry->addr, ip_addr, INET_ADDRSTRLEN),
                  ((unsigned short *)arp_entry->mac_addr)[0],
                  ((unsigned short *)arp_entry->mac_addr)[1],
                  ((unsigned short *)arp_entry->mac_addr)[2]);
       else
         cli_out (cli,"arp %s %.04hx.%.04hx.%.04hx \n",
                  pal_inet_ntop (AF_INET, &arp_entry->addr, ip_addr, INET_ADDRSTRLEN),
                  ((unsigned short *)arp_entry->mac_addr)[0],
                  ((unsigned short *)arp_entry->mac_addr)[1],
                  ((unsigned short *)arp_entry->mac_addr)[2]);

       write++;

     }

#ifdef HAVE_IPV6
   if (arp_master->ipv6_static_nbr_table != NULL)
     {
       for (node = ptree_top (arp_master->ipv6_static_nbr_table); node;
            node = ptree_next (node))
          {
            if (! node->info)
              continue;

            nbr_entry = node->info;

            cli_out (cli, "ipv6 neighbor %R %s %.04hx.%.04hx.%.04hx\n",
                     &nbr_entry->addr, nbr_entry->ifp->name,
                    ((unsigned short *)nbr_entry->mac_addr)[0],
                    ((unsigned short *)nbr_entry->mac_addr)[1],
                    ((unsigned short *)nbr_entry->mac_addr)[2]);

            write++;
          }
     }
#endif /* HAVE_IPV6 */

   return write;
}

#ifdef HAVE_IPV6
CLI (nsm_ipv6_neighbor_add,
     nsm_ipv6_neighbor_add_cmd,
     "ipv6 neighbor X:X::X:X IFNAME MAC",
     CLI_IPV6_STR,
     "IPV6 neighbor",
     "Neighbor's IPv6 address",
     "Interface name",
     "MAC (hardware) address in HHHH.HHHH.HHHH format")
{
  struct nsm_master *nm = cli->vr->proto;
  unsigned char mac_addr [ETHER_ADDR_LEN];
  struct interface *ifp;
  struct prefix_ipv6 p;
  u_int32_t mac1;
  u_int16_t mac2;
  int ret;

  pal_mem_set (&p, 0, sizeof (struct prefix_ipv6));
  ret = str2prefix_ipv6 (argv[0], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  pal_mem_set (mac_addr, 0, sizeof(mac_addr));

  if (pal_sscanf (argv[2], "%4hx.%4hx.%4hx",
                  (unsigned short *)&mac_addr[0],
                  (unsigned short *)&mac_addr[2],
                  (unsigned short *)&mac_addr[4]) != 3)
    {
      cli_out (cli, "%% Unable to translate mac address %s\n", argv[1]);
      return CLI_ERROR;
    }

  if (! nsm_api_mac_address_is_valid (argv[2]))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n",
               argv[2]);
      return CLI_ERROR;
    }

  mac1 = *(u_int32_t *)&mac_addr[0];
  mac2 = *(u_int16_t *)&mac_addr[4];
  if ((mac1 == 0 && mac2 == 0) ||
      (mac1 == 0xffffffff && mac2 == 0xffff))
    {
      cli_out (cli, "%% Unable to translate mac address %s\n", argv[1]);
      return CLI_ERROR;
    }

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);
  if (! ifp)
    return nsm_cli_return (cli, NSM_API_SET_ERR_IF_NOT_EXIST);

  ret = nsm_api_ipv6_nbr_add (nm, &p, ifp, mac_addr);
  return nsm_cli_return (cli, ret);
}

CLI (no_nsm_ipv6_neighbor_add,
     no_nsm_ipv6_neighbor_add_cmd,
     "no ipv6 neighbor X:X::X:X IFNAME",
     CLI_NO_STR,
     CLI_IPV6_STR,
     "IPV6 neighbor",
     "Neighbor's IPv6 address",
     "Interface name")
{
  struct nsm_master *nm = cli->vr->proto;
  struct interface *ifp;
  struct prefix_ipv6 p;
  int ret;

  pal_mem_set (&p, 0, sizeof (struct prefix_ipv6));
  ret = str2prefix_ipv6 (argv[0], &p);
  if (ret <= 0)
    return nsm_cli_return (cli, NSM_API_SET_ERR_MALFORMED_ADDRESS);

  ifp = if_lookup_by_name (&cli->vr->ifm, argv[1]);
  if (! ifp)
    return nsm_cli_return (cli, NSM_API_SET_ERR_IF_NOT_EXIST);

  ret = nsm_api_ipv6_nbr_del (nm, &p, ifp);
  return nsm_cli_return (cli, ret);
}

CLI (clear_nsm_ipv6_neighbors,
     clear_nsm_ipv6_neighbors_cmd,
     "clear ipv6 neighbors",
     CLI_CLEAR_STR,
     CLI_IPV6_STR,
     "IPv6 neighors")
{
  nsm_api_ipv6_nbr_del_all (cli->vr->proto, NSM_API_ARP_FLAG_DYNAMIC);
  return nsm_cli_return (cli, NSM_SUCCESS);
}

CLI (show_ipv6_nbr_all,
     show_ipv6_nbr_all_cmd,
     "show ipv6 neighbors",
     CLI_SHOW_STR,
     CLI_IPV6_STR,
     "IPV6 neighbors")
{
  nsm_api_ipv6_nbr_all_show (cli);

  return CLI_SUCCESS;
}

#endif /* HAVE_IPV6 */

void
nsm_cli_init_arp (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  cli_install_config (ctree, ARP_MODE, nsm_arp_config_write);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_arp_configure_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_nsm_arp_configure_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &clear_nsm_arp_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ip_arp_cmd);
 /* "proxy arp" CLI. */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &nsm_if_proxy_arp_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_nsm_if_proxy_arp_cmd);
  /* "arp ageing timeout" CLI.  */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &nsm_if_arp_ageing_timeout_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                  &no_nsm_if_arp_ageing_timeout_cmd);

#ifdef HAVE_IPV6
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &show_ipv6_nbr_all_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_ipv6_neighbor_add_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_nsm_ipv6_neighbor_add_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &clear_nsm_ipv6_neighbors_cmd);
#endif /* HAVE_IPV6 */
}
#endif /* HAVE_L3 */

