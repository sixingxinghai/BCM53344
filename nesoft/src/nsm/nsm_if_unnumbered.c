/* Copyright (C) 2005  All Rights Reserved.  */

#include "pal.h"

#ifdef HAVE_NSM_IF_UNNUMBERED

#include "lib.h"

#include "nsm/nsmd.h"
#include "nsm/nsm_interface.h"
#include "nsm/nsm_if_unnumbered.h"
#include "nsm/nsm_router.h"
#include "nsm/nsm_server.h"
#include "nsm/rib/rib.h"
#include "nsm/nsm_fea.h"
#include "nsm/nsm_api.h"
#include "nsm/nsm_cli.h"
#include "nsm/nsm_connected.h"

#ifdef HAVE_HA
#include "lib_cal.h"
#endif /* HAVE_HA */


struct connected *
nsm_if_ipv4_unnumbered_source_address (struct interface *ifp)
{
  struct connected *ifc;

  if (if_is_loopback (ifp))
    {
      for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
        if (!IN_LOOPBACK (pal_ntoh32 (ifc->address->u.prefix4.s_addr)))
          return ifc;
    }
  else
    for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
      if (!CHECK_FLAG (ifc->flags, NSM_IFA_SECONDARY))
        return ifc;

  return NULL;
}

int
nsm_if_ipv4_unnumbered_address_add (struct interface *ifp,
                                    struct connected *ifc,
                                    struct prefix *destination)
{
  struct connected *ifc_new = NULL;

#ifdef HAVE_L3
  ifc_new = ifc_get_ipv4 (&ifc->address->u.prefix4,
                          ifc->address->prefixlen, ifp);
#endif /*HAVE_L3 */

  if (ifc_new == NULL)
    return 0;

  if (destination)
    {
      ifc_new->destination = (struct prefix *) prefix_ipv4_new ();
      pal_mem_cpy (ifc_new->destination, destination,
                   sizeof (struct prefix_ipv4));
    }

#ifdef HAVE_UPDATE_ADDRESS_ON_UNNUMBERED
  /* Update to the kernel.  */
  if (CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
    nsm_fea_if_ipv4_address_add (ifp, ifc_new);
  else
    {
      if (ifp->vr)
        ifc_free (ifp->vr->zg, ifc_new);
      return 0;
    }
#endif /* HAVE_UPDATE_ADDRESS_ON_UNNUMBERED */

  /* Set the real flag.  */
  SET_FLAG (ifc_new->conf, NSM_IFC_REAL);

  /* Notify to the PMs.  */
  nsm_server_if_address_add_update (ifp, ifc_new);

  /* Register the connected route.  */
  if (if_is_running (ifp))
    nsm_connected_up_ipv4 (ifp, ifc_new);

  /* Add address to the interface.  */
  NSM_IF_ADD_IFC_IPV4 (ifp, ifc_new);

  return 1;
}

int
nsm_if_ipv4_unnumbered_address_delete (struct interface *ifp,
                                       struct connected *ifc)
{
  /* Notify to the PMs.  */
  nsm_server_if_address_delete_update (ifp, ifc);

#ifdef HAVE_UPDATE_ADDRESS_ON_UNNUMBERED
  /* Update to the kernel.  */
  if (CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
    nsm_fea_if_ipv4_address_delete (ifp, ifc);
#endif /* HAVE_UPDATE_ADDRESS_ON_UNNUMBERED */

#ifdef HAVE_L3
  /* UnRegister the connected route.  */
  if (if_is_running (ifp))
    nsm_connected_down_ipv4 (ifp, ifc);
#endif /*HAVE_L3 */

  /* Unset the real flag.  */
  UNSET_FLAG (ifc->conf, NSM_IFC_REAL);

#ifdef HAVE_HA
  lib_cal_modify_connected (ifp->vr->zg, ifc);
#endif /* HAVE_HA */

#ifdef HAVE_L3
  /* Delete address from the interface.  */
  NSM_IF_DELETE_IFC_IPV4 (ifp, ifc);
#endif /*HAVE_L3 */

  return 1;
}

int
nsm_if_ipv4_unnumbered_set (u_int32_t vr_id, char *ifname, char *ifname_source,
                            struct prefix_ipv4 *destination)
{
  struct interface *ifp, *ifp_source;
  struct nsm_master *nm;
  cindex_t cindex = 0;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  /* Unnumbered is allowed only on point-to-point interfaces.  */
  if (!if_is_pointopoint (ifp))
    return NSM_API_SET_ERR_UNNUMBERED_NOT_P2P;

  ifp_source = if_lookup_by_name (&nm->vr->ifm, ifname_source);
  if (ifp_source == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  /* Cannot use an unnumbered interface as a source.  */
  if (ifp_source == ifp || if_is_ipv4_unnumbered (ifp_source))
    return NSM_API_SET_ERR_UNNUMBERED_SOURCE;

  if (if_is_ipv4_unnumbered (ifp))
    {
      if ((GETDATA (ifp->unnumbered_ipv4->head) == ifp_source)
          && (! destination))
        return NSM_API_SET_SUCCESS;

      /* Unset the unnumbered interface first.  */
      nsm_if_ipv4_unnumbered_unset (vr_id, ifname);
    }
  else if (listcount (ifp->unnumbered_ipv4))
    return NSM_API_SET_ERR_UNNUMBERED_RECURSIVE;

#ifdef HAVE_L3  
  /* Cleanup the current IP addresses first.  */
  nsm_ip_address_uninstall_all (vr_id, ifname);
#endif /*HAVE_L3 */

  SET_FLAG (ifp->status, NSM_INTERFACE_IPV4_UNNUMBERED);

  /* Set the interface relationship.  */
  listnode_add (ifp->unnumbered_ipv4, ifp_source);
  listnode_add (ifp_source->unnumbered_ipv4, ifp);

  /* Set the interface status ctype in the cindex.  */
  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_STATUS);

  /* Announce to the PMs.  */
  nsm_server_if_update (ifp, cindex);

  /* Update the IP address.  */
  if (destination)
    nsm_if_ipv4_unnumbered_update (ifp, (struct prefix *)destination);

  return NSM_API_SET_SUCCESS;
}

int
nsm_if_ipv4_unnumbered_unset (u_int32_t vr_id, char *ifname)
{
  struct interface *ifp, *ifp_source;
  struct nsm_master *nm;
  cindex_t cindex = 0;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  /* Unnumbered is allowed only on point-to-point interfaces.  */
  if (!if_is_pointopoint (ifp))
    return NSM_API_SET_ERR_UNNUMBERED_NOT_P2P;

  if (!if_is_ipv4_unnumbered (ifp))
    return NSM_API_SET_SUCCESS;

  /* Cleanup the IP address.   */
  if (ifp->ifc_ipv4 != NULL)
    nsm_if_ipv4_unnumbered_address_delete (ifp, ifp->ifc_ipv4);

  /* Cleanup the interface relationship.  */
  if ((ifp_source = GETDATA (ifp->unnumbered_ipv4->head)) != NULL)
    {
      listnode_delete (ifp->unnumbered_ipv4, ifp_source);
      listnode_delete (ifp_source->unnumbered_ipv4, ifp);
    }

  UNSET_FLAG (ifp->status, NSM_INTERFACE_IPV4_UNNUMBERED);

  /* Set the interface status ctype in the cindex.  */
  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_STATUS);

  /* Announce to the PMs.  */
  nsm_server_if_update (ifp, cindex);

  return NSM_API_SET_SUCCESS;
}

int
nsm_if_ipv4_unnumbered_update (struct interface *ifp,
                               struct prefix *destination)
{
  struct interface *ifp_tmp;
  struct connected *ifc;
  struct listnode *node;

  if (!listcount (ifp->unnumbered_ipv4))
    return 0;

  /* Update for the unnumbered interface.  */
  if (if_is_ipv4_unnumbered (ifp))
    {
      ifp_tmp = GETDATA (ifp->unnumbered_ipv4->head);
      if (ifp_tmp == NULL)
        return 0;

      /* Get the address from the source interface.  */
      ifc = nsm_if_ipv4_unnumbered_source_address (ifp_tmp);

      /* Address is not changed.  */
      if (ifc == ifp->ifc_ipv4
          || (ifc != NULL && ifp->ifc_ipv4 != NULL
              && prefix_same (ifc->address, ifp->ifc_ipv4->address)))
        return 0;

      /* Remove the old address.  */
      if (ifp->ifc_ipv4 != NULL)
        nsm_if_ipv4_unnumbered_address_delete (ifp, ifp->ifc_ipv4);

      /* Install the new address.  */
      if (ifc != NULL)
        nsm_if_ipv4_unnumbered_address_add (ifp, ifc, destination);
    }
  /* Update for the source interface.  */
  else
    {
      /* Get the address from the source interface.  */
      ifc = nsm_if_ipv4_unnumbered_source_address (ifp);

      /* Update for the source interface.  */
      LIST_LOOP (ifp->unnumbered_ipv4, ifp_tmp, node)
        {
          /* Address is not changed.  */
          if (ifc == ifp_tmp->ifc_ipv4
              || (ifc != NULL && ifp_tmp->ifc_ipv4 != NULL
                  && prefix_same (ifc->address, ifp_tmp->ifc_ipv4->address)))
            continue;

          /* Remove the old address.  */
          if (ifp_tmp->ifc_ipv4 != NULL)
            nsm_if_ipv4_unnumbered_address_delete (ifp_tmp, ifp_tmp->ifc_ipv4);

          /* Install the new address.  */
          if (ifc != NULL)
            nsm_if_ipv4_unnumbered_address_add (ifp_tmp, ifc, destination);
        }
    }

  return 1;
}

int
nsm_if_ipv4_unnumbered_delete (struct interface *ifp, u_int32_t vr_id)
{
  struct interface *ifp_tmp;
  struct listnode *node, *next_node;

  if (if_is_ipv4_unnumbered (ifp))
    {
     /* If the interface is an unnumbered interface,
        delete its entry from the source interface list.  */
      if ((ifp->unnumbered_ipv4) && ((ifp_tmp = GETDATA (ifp->unnumbered_ipv4->head)) != NULL))
        listnode_delete (ifp_tmp->unnumbered_ipv4, ifp);
    }
  else
    {
     /* If the interface is an source interface to unnumbered
        interfaces, delete its ipv4/ipv6 list and accordingly
        update the memeber ports.  */
      if (ifp->unnumbered_ipv4)
        LIST_LOOP_DEL (ifp->unnumbered_ipv4, ifp_tmp, node, next_node)
          nsm_if_ipv4_unnumbered_unset (vr_id, ifp_tmp->name);
    }

  if (ifp->unnumbered_ipv4)
    list_delete (ifp->unnumbered_ipv4);
  
  ifp->unnumbered_ipv4 = NULL;

  return 1;
}

#ifdef HAVE_IPV6
struct connected *
nsm_if_ipv6_unnumbered_source_address (struct interface *ifp)
{
  struct connected *ifc;

  if (if_is_loopback (ifp))
    {
      for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
        if (!IN6_IS_ADDR_LOOPBACK (&ifc->address->u.prefix6))
          return ifc;
    }
  else
    for (ifc = ifp->ifc_ipv6; ifc; ifc = ifc->next)
      if (IN6_IS_ADDR_LINKLOCAL (&ifc->address->u.prefix6))
        return ifc;

  return NULL;
}

int
nsm_if_ipv6_unnumbered_address_add (struct interface *ifp,
                                    struct connected *ifc)
{
  struct connected *ifc_new;

  ifc_new = ifc_get_ipv6 (&ifc->address->u.prefix6,
                          ifc->address->prefixlen, ifp);
  if (ifc_new == NULL)
    return 0;

  /* Add address to the interface.  */
  if_add_ifc_ipv6 (ifp, ifc_new);

#ifdef HAVE_UPDATE_ADDRESS_ON_UNNUMBERED
  /* Update to the kernel.  */
  if (CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
    nsm_fea_if_ipv6_address_add (ifp, ifc_new);
  else
    return 0;
#endif /* HAVE_UPDATE_ADDRESS_ON_UNNUMBERED */

  /* Set the real flag.  */
  SET_FLAG (ifc_new->conf, NSM_IFC_REAL);

  /* Notify to the PMs.  */
  nsm_server_if_address_add_update (ifp, ifc_new);

  return 1;
}

int
nsm_if_ipv6_unnumbered_address_delete (struct interface *ifp,
                                       struct connected *ifc)
{
  /* Notify to the PMs.  */
  nsm_server_if_address_delete_update (ifp, ifc);

#ifdef HAVE_UPDATE_ADDRESS_ON_UNNUMBERED
  /* Update to the kernel.  */
  if (CHECK_FLAG (ifp->status, NSM_INTERFACE_ACTIVE))
    nsm_fea_if_ipv6_address_delete (ifp, ifc);
#endif /* HAVE_UPDATE_ADDRESS_ON_UNNUMBERED */

  /* Unset the real flag.  */
  UNSET_FLAG (ifc->conf, NSM_IFC_REAL);

#ifdef HAVE_HA
  lib_cal_modify_connected (ifp->vr->zg, ifc);
#endif /* HAVE_HA */

  /* Delete address from the interface.  */
  if_delete_ifc_ipv6 (ifp, ifc);

  return 1;
}

int
nsm_if_ipv6_unnumbered_set (u_int32_t vr_id, char *ifname, char *ifname_source)
{
  struct interface *ifp, *ifp_source;
  struct nsm_master *nm;
  cindex_t cindex = 0;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  /* Unnumbered is allowed only on point-to-point interfaces.  */
  if (!if_is_pointopoint (ifp))
    return NSM_API_SET_ERR_UNNUMBERED_NOT_P2P;

  ifp_source = if_lookup_by_name (&nm->vr->ifm, ifname_source);
  if (ifp_source == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  /* Cannot use an unnumbered interface as a source.  */
  if (ifp_source == ifp || if_is_ipv6_unnumbered (ifp_source))
    return NSM_API_SET_ERR_UNNUMBERED_SOURCE;

  if (if_is_ipv6_unnumbered (ifp))
    {
      if (GETDATA (ifp->unnumbered_ipv6->head) == ifp_source)
        return NSM_API_SET_SUCCESS;

      /* Unset the unnumbered interface first.  */
      nsm_if_ipv6_unnumbered_unset (vr_id, ifname);
    }
  else if (listcount (ifp->unnumbered_ipv6))
    return NSM_API_SET_ERR_UNNUMBERED_RECURSIVE;

  /* Cleanup the current IP addresses first.  */
  nsm_ipv6_address_uninstall_all (vr_id, ifname);

  SET_FLAG (ifp->status, NSM_INTERFACE_IPV6_UNNUMBERED);

  /* Set the interface relationship.  */
  listnode_add (ifp->unnumbered_ipv6, ifp_source);
  listnode_add (ifp_source->unnumbered_ipv6, ifp);

  /* Set the interface status ctype in the cindex.  */
  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_STATUS);

  /* Announce to the PMs.  */
  nsm_server_if_update (ifp, cindex);
  
  /* Update the IP address.  */
  nsm_if_ipv6_unnumbered_update (ifp);

  return NSM_API_SET_SUCCESS;
}

int
nsm_if_ipv6_unnumbered_unset (u_int32_t vr_id, char *ifname)
{
  struct interface *ifp, *ifp_source;
  struct nsm_master *nm;
  cindex_t cindex = 0;

  nm = nsm_master_lookup_by_id (nzg, vr_id);
  if (nm == NULL)
    return NSM_API_SET_ERR_MASTER_NOT_EXIST;

  ifp = if_lookup_by_name (&nm->vr->ifm, ifname);
  if (ifp == NULL)
    return NSM_API_SET_ERR_IF_NOT_EXIST;

  /* Unnumbered is allowed only on point-to-point interfaces.  */
  if (!if_is_pointopoint (ifp))
    return NSM_API_SET_ERR_UNNUMBERED_NOT_P2P;

  if (!if_is_ipv6_unnumbered (ifp))
    return NSM_API_SET_SUCCESS;

  /* Cleanup the IP address.   */
  if (ifp->ifc_ipv6 != NULL)
    nsm_if_ipv6_unnumbered_address_delete (ifp, ifp->ifc_ipv6);

  /* Cleanup the interface relationship.  */
  if ((ifp_source = GETDATA (ifp->unnumbered_ipv6->head)) != NULL)
    {
      listnode_delete (ifp->unnumbered_ipv6, ifp_source);
      listnode_delete (ifp_source->unnumbered_ipv6, ifp);
    }

  UNSET_FLAG (ifp->status, NSM_INTERFACE_IPV6_UNNUMBERED);

  /* Set the interface status ctype in the cindex.  */
  NSM_SET_CTYPE (cindex, NSM_LINK_CTYPE_STATUS);

  /* Announce to the PMs.  */
  nsm_server_if_update (ifp, cindex);

  return NSM_API_SET_SUCCESS;
}

int
nsm_if_ipv6_unnumbered_update (struct interface *ifp)
{
  struct interface *ifp_tmp;
  struct connected *ifc;
  struct listnode *node;

  if (!listcount (ifp->unnumbered_ipv6))
    return 0;

  /* Update for the unnumbered interface.  */
  if (if_is_ipv6_unnumbered (ifp))
    {
      ifp_tmp = GETDATA (ifp->unnumbered_ipv6->head);
      if (ifp_tmp == NULL)
        return 0;

      /* Get the address from the source interface.  */
      ifc = nsm_if_ipv6_unnumbered_source_address (ifp_tmp);

      /* Address is not changed.  */
      if (ifc == ifp->ifc_ipv6
          || (ifc != NULL && ifp->ifc_ipv6 != NULL
              && prefix_same (ifc->address, ifp->ifc_ipv6->address)))
        return 0;

      /* Remove the old address.  */
      if (ifp->ifc_ipv6 != NULL)
        nsm_if_ipv6_unnumbered_address_delete (ifp, ifp->ifc_ipv6);

      /* Install the new address.  */
      if (ifc != NULL)
        nsm_if_ipv6_unnumbered_address_add (ifp, ifc);
    }
  /* Update for the source interface.  */
  else
    {
      /* Get the address from the source interface.  */
      ifc = nsm_if_ipv6_unnumbered_source_address (ifp);

      /* Update for the source interface.  */
      LIST_LOOP (ifp->unnumbered_ipv6, ifp_tmp, node)
        {
          /* Address is not changed.  */
          if (ifc == ifp_tmp->ifc_ipv6
              || (ifc != NULL && ifp_tmp->ifc_ipv6 != NULL
                  && prefix_same (ifc->address, ifp_tmp->ifc_ipv6->address)))
            continue;

          /* Remove the old address.  */
          if (ifp_tmp->ifc_ipv6 != NULL)
            nsm_if_ipv6_unnumbered_address_delete (ifp_tmp, ifp_tmp->ifc_ipv6);

          /* Install the new address.  */
          if (ifc != NULL)
            nsm_if_ipv6_unnumbered_address_add (ifp_tmp, ifc);
        }
    }

  return 1;
}

int
nsm_if_ipv6_unnumbered_delete (struct interface *ifp, u_int32_t vr_id)
{
  struct interface *ifp_tmp;
  struct listnode *node, *next_node;

  if (if_is_ipv6_unnumbered (ifp))
    {
     /* If the interface is an unnumbered interface,
        delete its entry from the source interface list.  */
      if ((ifp->unnumbered_ipv6) && ((ifp_tmp = GETDATA (ifp->unnumbered_ipv6->head)) != NULL))
        listnode_delete (ifp_tmp->unnumbered_ipv6, ifp);
    }
  else
    {
     /* If the interface is an source interface to unnumbered
        interfaces, delete its ipv4/ipv6 list and accordingly
        update the memeber ports.  */
      if (ifp->unnumbered_ipv6)
        LIST_LOOP_DEL (ifp->unnumbered_ipv6, ifp_tmp, node, next_node)
          nsm_if_ipv6_unnumbered_unset (vr_id, ifp_tmp->name);
    }

  if (ifp->unnumbered_ipv6)
    list_delete (ifp->unnumbered_ipv6);
  ifp->unnumbered_ipv6 = NULL;

  return 1;
}
#endif /* HAVE_IPV6 */


void
nsm_if_unnumbered_show (struct cli *cli, struct interface *ifp)
{
  struct interface *ifp_source;
  struct pal_in4_addr addr4;
#ifdef HAVE_IPV6
  struct pal_in6_addr addr6;
#endif /* HAVE_IPV6 */

  if (if_is_ipv4_unnumbered (ifp))
    if ((ifp_source = GETDATA (ifp->unnumbered_ipv4->head)))
      {
        pal_mem_set (&addr4, 0, sizeof (struct pal_in4_addr));
        cli_out (cli,
                 "  Interface is unnumbered. Using IPv4 address of %s (%r)\n",
                 ifp_source->name,
                 ifp->ifc_ipv4 ? &ifp->ifc_ipv4->address->u.prefix4 : &addr4);
        if (ifp->ifc_ipv4 && ifp->ifc_ipv4->destination)
          cli_out (cli, "  Remote address: %O\n", ifp->ifc_ipv4->destination);
      }

#ifdef HAVE_IPV6
  if (if_is_ipv6_unnumbered (ifp))
    if ((ifp_source = GETDATA (ifp->unnumbered_ipv6->head)))
      {
        pal_mem_set (&addr6, 0, sizeof (struct pal_in6_addr));
        cli_out (cli,
                 "  Interface is unnumbered. Using IPv6 address of %s (%R)\n",
                 ifp_source->name,
                 ifp->ifc_ipv6 ? &ifp->ifc_ipv6->address->u.prefix6 : &addr6);
      }
#endif /* HAVE_IPV6 */
}

void
nsm_if_ipv4_unnumbered_config_write (struct cli *cli, struct interface *ifp)
{
  struct interface *ifp_tmp = GETDATA (ifp->unnumbered_ipv4->head);
  if (ifp_tmp != NULL)
    {
      cli_out (cli, " ip unnumbered %s\n", ifp_tmp->name);

      if (ifp->ifc_ipv4 && ifp->ifc_ipv4->destination) 
        cli_out (cli, " ip remote-address %O\n", 
                 ifp->ifc_ipv4->destination);
    }
}

#ifdef HAVE_IPV6
void
nsm_if_ipv6_unnumbered_config_write (struct cli *cli, struct interface *ifp)
{
  struct interface *ifp_tmp = GETDATA (ifp->unnumbered_ipv6->head);
  if (ifp_tmp != NULL)
    cli_out (cli, " ipv6 unnumbered %s\n", ifp_tmp->name);
}
#endif /* HAVE_IPV6 */

CLI (nsm_if_ip_unnumbered,
     nsm_if_ip_unnumbered_cmd,
     "ip unnumbered IFNAME",
     "Interface Internet Protocol config commands",
     "Enable IP processing without an explicit address",
     CLI_IFNAME_STR)
{
  struct interface *ifp = cli->index;
  int ret;

  NSM_CHECK_L3_COMMANDS (cli, ifp, ipv4_if_commands);

  ret = nsm_if_ipv4_unnumbered_set (cli->vr->id, ifp->name, argv[0], NULL);

  return nsm_cli_return (cli, ret);
}

CLI (nsm_if_ip_remote_address,
     nsm_if_ip_remote_address_cmd,
     "ip remote-address A.B.C.D/M",
     "Interface Internet Protocol config commands",
     "Remote address of the PPP link",
     "IP Remote-address (e.g. 10.1.1.1/32)")
{
  struct interface *ifp = cli->index;
  struct interface *ifp_source;
  struct prefix_ipv4 destination;
  int ret;

  NSM_CHECK_L3_COMMANDS (cli, ifp, ipv4_if_commands);

  CLI_GET_IPV4_PREFIX ("Remote address", destination, argv[0]);

  if (! if_is_pointopoint (ifp))
    {
     cli_out (cli, "%% Not a Point-to-Point interface\n");
     return CLI_ERROR;
    }

  if (! if_is_ipv4_unnumbered(ifp))
    {
      cli_out (cli, "%% Not an unnumbered interface\n");
      return CLI_ERROR;
    }

  ifp_source = GETDATA (ifp->unnumbered_ipv4->head);
  if (! ifp_source)
    return CLI_ERROR;

  ret = nsm_if_ipv4_unnumbered_set (cli->vr->id, ifp->name, ifp_source->name,
                                    &destination);

  return nsm_cli_return (cli, ret);
}

CLI (no_nsm_if_ip_remote_address,
     no_nsm_if_ip_remote_address_cmd,
     "no ip remote-address",
     CLI_NO_STR,
     "Interface Internet Protocol config command",
     "Remote address of the PPP link")
{
  struct interface *ifp = cli->index;
  int ret;

  NSM_CHECK_L3_COMMANDS (cli, ifp, ipv4_if_commands);

  if (! if_is_pointopoint (ifp))
    {
     cli_out (cli, "%% Not a Point-to-Point interface\n");
     return CLI_ERROR;
    }

  if (! if_is_ipv4_unnumbered(ifp))
    {
      cli_out (cli, "%% Not an unnumbered interface\n");
      return CLI_ERROR;
    }

  if (ifp->ifc_ipv4 == NULL)
    {
      cli_out (cli, "%% No Remote-Address is configured\n");
      return CLI_ERROR;
    }

  ret = nsm_if_ipv4_unnumbered_address_delete(ifp, ifp->ifc_ipv4);

  return nsm_cli_return (cli, ret);
}

CLI (no_nsm_if_ip_unnumbered,
     no_nsm_if_ip_unnumbered_cmd,
     "no ip unnumbered",
     CLI_NO_STR,
     "Interface Internet Protocol config commands",
     "Enable IP processing without an explicit address")
{
  struct interface *ifp = cli->index;
  int ret;

  NSM_CHECK_L3_COMMANDS (cli, ifp, ipv4_if_commands);

  ret = nsm_if_ipv4_unnumbered_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}

#ifdef HAVE_IPV6
CLI (nsm_if_ipv6_unnumbered,
     nsm_if_ipv6_unnumbered_cmd,
     "ipv6 unnumbered IFNAME",
     "Interface Internet Protocol v6 config commands",
     "Enable IP processing without an explicit address",
     CLI_IFNAME_STR)
{
  struct interface *ifp = cli->index;
  int ret;

  NSM_CHECK_L3_COMMANDS (cli, ifp, ipv6_if_commands);

  ret = nsm_if_ipv6_unnumbered_set (cli->vr->id, ifp->name, argv[0]);

  return nsm_cli_return (cli, ret);
}

CLI (no_nsm_if_ipv6_unnumbered,
     no_nsm_if_ipv6_unnumbered_cmd,
     "no ipv6 unnumbered",
     CLI_NO_STR,
     "Interface Internet Protocol v6 config commands",
     "Enable IP processing without an explicit address")
{
  struct interface *ifp = cli->index;
  int ret;

  NSM_CHECK_L3_COMMANDS (cli, ifp, ipv6_if_commands);

  ret = nsm_if_ipv6_unnumbered_unset (cli->vr->id, ifp->name);

  return nsm_cli_return (cli, ret);
}
#endif /* HAVE_IPV6 */

void
nsm_if_unnumbered_cli_init (struct cli_tree *ctree)
{
  /* "ip unnumbered" CLIs.  */
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &nsm_if_ip_unnumbered_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &nsm_if_ip_remote_address_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_nsm_if_ip_unnumbered_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_nsm_if_ip_remote_address_cmd);
#ifdef HAVE_IPV6
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &nsm_if_ipv6_unnumbered_cmd);
  cli_install_gen (ctree, INTERFACE_MODE, PRIVILEGE_NORMAL, 0,
                   &no_nsm_if_ipv6_unnumbered_cmd);
#endif /* HAVE_IPV6 */
}

void
nsm_if_unnumbered_init (struct interface *ifp)
{
  if (ifp->unnumbered_ipv4 == NULL)
    ifp->unnumbered_ipv4 = list_new ();
#ifdef HAVE_IPV6
  if (ifp->unnumbered_ipv6 == NULL)
    ifp->unnumbered_ipv6 = list_new ();
#endif /* HAVE_IPV6 */
}

void
nsm_if_unnumbered_finish (struct interface *ifp, u_int32_t vr_id)
{
  nsm_if_ipv4_unnumbered_delete (ifp, vr_id);
#ifdef HAVE_IPV6
  nsm_if_ipv6_unnumbered_delete (ifp, vr_id);
#endif /* HAVE_IPV6 */
}

#endif /* HAVE_NSM_IF_UNNUMBERED */
