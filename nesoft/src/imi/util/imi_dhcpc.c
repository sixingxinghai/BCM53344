/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#ifdef HAVE_DHCP_CLIENT

#include "imi/pal_dhcp.h"

#include "cli.h"

#include "imi/imi.h"


/* New DHCP client structure. */
static struct imi_dhcp_client *
imi_dhcp_client_new (struct interface *ifp)
{
  struct imi_dhcp_client *new;
  struct imi_interface *imi_if = ifp->info;

  /* Allocate & assign to interface. */
  new = XCALLOC (MTYPE_IMI_DHCP_CLIENT, sizeof (struct imi_dhcp_client));
  if (new == NULL)
    return NULL;

  new->clientid = NULL;
  new->hostname = NULL;
  new->suspended = PAL_FALSE;
  new->ifp = ifp;
  imi_if->idc = new;

  return new;
}

/* Free DHCP client structure. */
void
imi_dhcp_client_free (struct imi_interface *imi_if)
{
  struct imi_dhcp_client *idc = imi_if->idc;

  if (idc)
    {
      if (idc->hostname)
        XFREE (MTYPE_IMI_STRING, idc->hostname);
      XFREE (MTYPE_IMI_DHCP_CLIENT, idc);
    }

  imi_if->idc = NULL;
}

/* Execute dhcp client for an interface. */
static int
imi_dhcp_client_execute (struct interface *ifp, char *clientid,
                         char *hostname, int execute_now)
{
  struct apn_vr *vr = apn_vr_get_privileged (imim);
  struct imi_interface *imi_if = ifp->info;
  struct interface *client_ifp = NULL;
  struct imi_dhcp_client *idc;

  /* Lookup client-id interface. */
  if (clientid)
    {
      client_ifp = if_lookup_by_name (&vr->ifm, clientid);
      if (client_ifp == NULL)
        return IMI_API_INVALID_ARG_ERROR;
    }

  /* Is dhcp client already running on the interface? */
  if (imi_if->idc != NULL)
    return IMI_API_CANT_SET_ERROR;
  else
    idc = imi_dhcp_client_new (ifp);

  if (idc == NULL)
    return IMI_API_MEMORY_ERROR;
  else if (clientid)
    idc->clientid = client_ifp;

  /* Hostname set? */
  if (hostname)
    idc->hostname = XSTRDUP (MTYPE_IMI_STRING, hostname);

  /* Execute PAL API. */
  if (execute_now)
    pal_dhcp_client_start (imim, idc);
  else
    idc->suspended = PAL_TRUE;

  return IMI_API_SUCCESS;
}

/* Start previously suspended DHCP client. */
int
imi_dhcpc_resume (struct interface *ifp)
{
  struct imi_interface *imi_if = ifp->info;

  /* Send to PAL. */
  if (imi_if && imi_if->idc && imi_if->idc->suspended)
    {
      pal_dhcp_client_start (imim, imi_if->idc);

      /* Unset flag. */
      imi_if->idc->suspended = PAL_FALSE;
    }

  return IMI_API_SUCCESS;
}

/* Stop dhcp client on an interface. */
static int
imi_dhcp_client_stop (struct interface *ifp, char *clientid, char *hostname)
{
  struct imi_interface *imi_if = ifp->info;
  struct imi_dhcp_client *idc = imi_if->idc;
  struct interface *client_ifp = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (imim);

  /* Is DHCP client running? */
  if (idc == NULL)
    return IMI_API_NOT_SET_ERROR;

  /* Does clientid match? */
  if (clientid)
    client_ifp = if_lookup_by_name (&vr->ifm, clientid);

  /* Check match. */
  if (client_ifp)
    {
      if (idc->clientid == NULL
          || idc->clientid->ifindex != client_ifp->ifindex)
        return IMI_API_INVALID_ARG_ERROR;
    }
  else
    if (idc->clientid)
      return IMI_API_INVALID_ARG_ERROR;

  /* Check hostname. */
  if (hostname)
    {
      if (idc->hostname == NULL || pal_strcmp (hostname, idc->hostname))
        return IMI_API_INVALID_ARG_ERROR;
    }
  else
    if (idc->hostname)
      return IMI_API_INVALID_ARG_ERROR;

  /* If we're here, ok to stop it. */
  pal_dhcp_client_stop (imim, idc);
  imi_dhcp_client_free (ifp->info);
  
  return IMI_API_SUCCESS;
}

/* Print response from API. */
static int
imi_dhcp_client_response (struct cli *cli, int ret)
{
  char *str;

  switch (ret)
    {
    case IMI_API_CANT_SET_ERROR:
      str = "% DHCP client is already running on this interface.";
      break;
    case IMI_API_NOT_SET_ERROR:
      str = "% DHCP client is not running on this interface.";
      break;
    case IMI_API_MEMORY_ERROR:
      str = "% Error allocating memory for DHCP client structure.";
      break;
    case IMI_API_INVALID_ARG_ERROR:
      str = "% Invalid client-id or hostname provided.";
      break;
    default:
      str = NULL;
      break;
    }

  /* Error? */
  if (str)
    {
      cli_out (cli, "%s\n", str);
      return CLI_ERROR;
    }

  return CLI_SUCCESS;
}

/* Set interface to use DHCP Client. */
CLI (ip_address_dhcp,
     ip_address_dhcp_cli,
     "ip address dhcp",
     "Interface Internet Protocol config commands",
     "IP address configuration commands",
     "Use DHCP Client to obtain IP address for this interface")
{
  int ret;
  int cmd = 1;

  /* If loading config-file, suspend dhcp client until complete interface
     information is received from the NSM.  */
  if (cli->source == CLI_SOURCE_FILE)
    cmd = 0;

  /* Execute. */
  ret = imi_dhcp_client_execute (cli->index, NULL, NULL, cmd);
  return imi_dhcp_client_response (cli, ret);
}

CLI (ip_address_dhcp_clientid,
     ip_address_dhcp_clientid_cli,
     "ip address dhcp client-id IFNAME",
     "Interface Internet Protocol config commands",
     "IP address configuration commands",
     "Use DHCP Client to obtain IP address for this interface",
     "Specify the client identifier",
     CLI_IFNAME_STR)
{
  int ret;
  int cmd = 1;
  char buf[INTERFACE_NAMSIZ];
  char *ifname;
  
  /* If loading config-file, suspend dhcp client until complete interface
     information is received from the NSM.  */
  if (cli->source == CLI_SOURCE_FILE)
    cmd = 0;

  /* Resolve ifname. */
  if (argc == 1)
    ifname = argv[0];
  else
    ifname = cli_interface_resolve (buf, INTERFACE_NAMSIZ, argv[0], argv[1]);

  /* Execute API. */
  ret = imi_dhcp_client_execute (cli->index, ifname, NULL, cmd);
  return imi_dhcp_client_response (cli, ret);
}

CLI (ip_address_dhcp_hostname,
     ip_address_dhcp_hostname_cli,
     "ip address dhcp hostname WORD",
     "Interface Internet Protocol config commands",
     "IP address configuration commands",
     "Use DHCP Client to obtain IP address for this interface",
     "Specify the hostname only",
     "Hostname for this DHCP client")
{
  int ret;
  int cmd = 1;
  
  /* If loading config-file, suspend dhcp client until complete interface
     information is received from the NSM.  */
  if (cli->source == CLI_SOURCE_FILE)
    cmd = 0;

  /* Execute API. */
  ret = imi_dhcp_client_execute (cli->index, NULL, argv[0], cmd);
  return imi_dhcp_client_response (cli, ret);
}

CLI (ip_address_dhcp_clientid_host,
     ip_address_dhcp_clientid_host_cli,
     "ip address dhcp client-id IFNAME hostname WORD",
     "Interface Internet Protocol config commands",
     "IP address configuration commands",
     "Use DHCP Client to obtain IP address for this interface",
     "Specify the client identifier",
     CLI_IFNAME_STR,
     "Specify the hostname",
     "Hostname for this DHCP client")
{
  int ret;
  int cmd = 1;
  char buf[INTERFACE_NAMSIZ];
  char *ifname;

  /* If loading config-file, suspend dhcp client until complete interface
     information is received from the NSM.  */
  if (cli->source == CLI_SOURCE_FILE)
    cmd = 0;

  /* Resolve ifname. */
  if (argc == 1)
    ifname = argv[0];
  else
    ifname = cli_interface_resolve (buf, INTERFACE_NAMSIZ, argv[0], argv[1]);

  /* Execute API. */
  ret = imi_dhcp_client_execute (cli->index, ifname, argv[2], cmd);
  return imi_dhcp_client_response (cli, ret);
}

CLI (no_ip_address_dhcp,
     no_ip_address_dhcp_cli,
     "no ip address dhcp",
     CLI_NO_STR,
     "Interface Internet Protocol config commands",
     "IP address configuration commands",
     "Use DHCP Client to obtain IP address for this interface")
{
  int ret;

  /* Execute API. */
  ret = imi_dhcp_client_stop (cli->index, NULL, NULL);
  return imi_dhcp_client_response (cli, ret);
}

CLI (no_ip_address_dhcp_clientid,
     no_ip_address_dhcp_clientid_cli,
     "no ip address dhcp client-id IFNAME",
     CLI_NO_STR,
     "Interface Internet Protocol config commands",
     "IP address configuration commands",
     "Use DHCP Client to obtain IP address for this interface",
     "Specify the client identifier",
     CLI_IFNAME_STR)
{
  int ret;
  char buf[INTERFACE_NAMSIZ];
  char *ifname;

  /* Resolve ifname. */
  if (argc == 1)
    ifname = argv[0];
  else
    ifname = cli_interface_resolve (buf, INTERFACE_NAMSIZ, argv[0], argv[1]);

  /* Execute API. */
  ret = imi_dhcp_client_stop (cli->index, ifname, NULL);
  return imi_dhcp_client_response (cli, ret);
}

CLI (no_ip_address_dhcp_hostname,
     no_ip_address_dhcp_hostname_cli,
     "no ip address dhcp hostname WORD",
     CLI_NO_STR,
     "Interface Internet Protocol config commands",
     "IP address configuration commands",
     "Use DHCP Client to obtain IP address for this interface",
     "Specify the hostname",
     "Hostname for this DHCP client")
{
  int ret;

  /* Execute API. */
  ret = imi_dhcp_client_stop (cli->index, NULL, argv[0]);
  return imi_dhcp_client_response (cli, ret);
}

CLI (no_ip_address_dhcp_clientid_host,
     no_ip_address_dhcp_clientid_host_cli,
     "no ip address dhcp client-id IFNAME hostname WORD",
     CLI_NO_STR,
     "Interface Internet Protocol config commands",
     "IP address configuration commands",
     "Use DHCP Client to obtain IP address for this interface",
     "Specify the client identifier",
     CLI_IFNAME_STR,
     "Specify the hostname",
     "Hostname for this DHCP client")
{ 
  int ret;
  char buf[INTERFACE_NAMSIZ];
  char *ifname;

  /* Resolve ifname. */
  if (argc == 1)
    ifname = argv[0];
  else
    ifname = cli_interface_resolve (buf, INTERFACE_NAMSIZ, argv[0], argv[1]);

  /* Execute API. */
  ret = imi_dhcp_client_stop (cli->index, ifname, argv[2]);
  return imi_dhcp_client_response (cli, ret);
}

/* Show:
   - Since DHCP client info is stored per-interface, it should be displayed
   with all other interface info received from the PacOS NSM.  The
   below function is called from imi_interface_dump().  */
void
imi_dhcpc_show_interface (struct cli *cli, struct interface *ifp)
{
  struct imi_interface *imi_if;
  struct imi_dhcp_client *idc;

  if (ifp == NULL)
    return;

  imi_if = ifp->info;
  idc = imi_if->idc;

  /* Is DHCP client configured on this interface? */
  if (idc)
    {
      cli_out (cli, "  DHCP client is enabled <client-id=%s,hostname=%s>.\n", 
               idc->clientid ? idc->clientid->name : ifp->name,
               idc->hostname ? idc->hostname : "");
    }
  else
    cli_out (cli, "  DHCP client is disabled.\n");
}

/* Configuration write for DHCP Client. */
void
imi_dhcpc_config_write_if (struct cli *cli, struct interface *ifp)
{
  struct imi_interface *imi_if = ifp->info;
  struct imi_dhcp_client *idc = imi_if->idc;

  /* Is DHCP client configured on this interface? */
  if (idc)
    {
      cli_out (cli, " ip address dhcp%s%s%s%s\n",
               idc->clientid ? " client-id " : "",
               idc->clientid ? idc->clientid->name : "",
               idc->hostname ? " hostname " : "",
               idc->hostname ? idc->hostname : "");
    }
}

/* Shutdown. */
void
imi_dhcpc_shutdown ()
{
  struct listnode *node;
  struct interface *ifp;
  struct imi_interface *imi_if;
  struct apn_vr *vr = apn_vr_get_privileged (imim);

  /* Cycle through interfaces. */
  LIST_LOOP (vr->ifm.if_list, ifp, node)
    {
      imi_if = ifp->info;

      if (imi_if->idc)
        {
          /* Free dhcp client memory. */
          pal_dhcp_client_stop (imim, imi_if->idc);
          imi_dhcp_client_free (ifp->info);
        }
    }
}

/* Initialization. */
void
imi_dhcpc_init ()
{
  struct cli_tree *ctree = imim->ctree;

  cli_install_imi (ctree, INTERFACE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_address_dhcp_cli);
  cli_install_imi (ctree, INTERFACE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_address_dhcp_clientid_cli);
  cli_install_imi (ctree, INTERFACE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_address_dhcp_hostname_cli);
  cli_install_imi (ctree, INTERFACE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_address_dhcp_clientid_host_cli);

  cli_install_imi (ctree, INTERFACE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_address_dhcp_cli);
  cli_install_imi (ctree, INTERFACE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_address_dhcp_clientid_cli);
  cli_install_imi (ctree, INTERFACE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_address_dhcp_hostname_cli);
  cli_install_imi (ctree, INTERFACE_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_address_dhcp_clientid_host_cli);
}

#endif /* HAVE_DHCP_CLIENT */
