/* Copyright (C) 2003  All Rights Reserved.  */

#include <pal.h>

#include "cli.h"
#include "show.h"
#include "version.h"
#include "modbmap.h"

#include "vtysh.h"
#include "vtysh_system.h"
#include "vtysh_exec.h"
#include "vtysh_cli.h"
#include "vtysh_line.h"

/* All of "show" commands are installed in EXEC_MODE.  One exception
   is "show running-config", it will be installed into all modes for
   user convenience.  */

#ifndef HAVE_TCP_MESSAGE
static char *
vtysh_get_show_path (modbmap_t module)
{
  if (MODBMAP_ISSET (module, APN_PROTO_NSM))
    return NSM_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_RIP))
    return RIP_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_RIPNG))
    return RIPNG_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_OSPF))
    return OSPF_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_OSPF6))
    return OSPF6_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_ISIS))
    return ISIS_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_BGP))
    return BGP_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_PIMDM))
    return PIMDM_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_PIMSM))
    return PIMSM_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_PIMSM6))
    return PIMSM6_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_DVMRP))
    return DVMRP_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_LDP))
    return LDP_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_RSVP))
    return RSVP_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_STP))
    return STP_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_RSTP))
    return RSTP_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_8021X))
    return AUTH_SHOW_PATH;
  else if (MODBMAP_ISSET (module, APN_PROTO_ONM))
    return ONMD_SHOW_PATH;
  else
      return IMI_SHOW_PATH;
}
#else /* HAVE_TCP_MESSAGE */
static int
vtysh_get_show_port (modbmap_t module)
{
  if (MODBMAP_ISSET (module, APN_PROTO_NSM))
    return NSM_SHOW_PORT;
  else if (MODBMAP_ISSET (module, APN_PROTO_RIP))
    return RIP_SHOW_PORT;
  else if (MODBMAP_ISSET (module, APN_PROTO_RIPNG))
    return RIPNG_SHOW_PORT;
  else if (MODBMAP_ISSET (module, APN_PROTO_OSPF))
    return OSPF_SHOW_PORT;
  else if (MODBMAP_ISSET (module, APN_PROTO_OSPF6))
    return OSPF6_SHOW_PORT;
  else if (MODBMAP_ISSET (module, APN_PROTO_ISIS))
    return ISIS_SHOW_PORT;
  else if (MODBMAP_ISSET (module, APN_PROTO_BGP))
    return BGP_SHOW_PORT;
  else if (MODBMAP_ISSET (module, APN_PROTO_PIMDM))
    return PIMDM_SHOW_PORT;
  else if (MODBMAP_ISSET (module, APN_PROTO_PIMSM))
    return PIMSM_SHOW_PORT;
  else if (MODBMAP_ISSET (module, APN_PROTO_PIMSM6))
    return PIMSM6_SHOW_PORT;
  else if (MODBMAP_ISSET (module, APN_PROTO_DVMRP))
    return DVMRP_SHOW_PORT;
  else if (MODBMAP_ISSET (module, APN_PROTO_LDP))
    return LDP_SHOW_PORT;
  else if (MODBMAP_ISSET (module, APN_PROTO_RSVP))
    return RSVP_SHOW_PORT;
  else if (MODBMAP_ISSET (module, APN_PROTO_STP))
    return STP_SHOW_PORT;
  else if (MODBMAP_ISSET (module, APN_PROTO_RSTP))
    return RSTP_SHOW_PORT;
  else if (MODBMAP_ISSET (module, APN_PROTO_8021X))
    return AUTH_SHOW_PORT;
  else
      return IMI_SHOW_PORT;
}
#endif /* ! HAVE_TCP_MESSAGE */

CLI (vtysh_show_func,
     vtysh_show_func_cli,
     "Internal_function",
     "Internal_function")
{
  int sock;
  char buf[BUFSIZ];
  int nbytes;
  struct cli_element *cel;
  vrf_id_t vrf_id;
#ifdef HAVE_TCP_MESSAGE
  int port;
#else /* HAVE_TCP_MESSAGE */
  char *path;
#endif /* HAVE_TCP_MESSAGE */

  cel = cli->cel;

#ifdef HAVE_TCP_MESSAGE
  port = vtysh_get_show_port (cel->module);
  if (port < 0)
    return -1;

  /* Open socket.  */
  sock = vtysh_line_open (port);
#else /* HAVE_TCP_MESSAGE */
  path = vtysh_get_show_path (cel->module);
  if (! path)
    return -1;

  /* Open socket. */
  sock = vtysh_line_open (path);
#endif /* HAVE_TCP_MESSAGE */
  if (sock < 0)
    {
      printf ("\n");
      return -1;
    }

#ifdef HAVE_VR
  vrf_id = imiline.vrf_id;
#else
  vrf_id = VRF_ID_UNSPEC;  /* ignored */
#endif /* HAVE_VR */

  /* Send command.  */
  show_line_write (vtyshm, sock, cli->str, strlen (cli->str), vrf_id);

  /* Print output to stdout.  */
  while (1)
    {
      nbytes = read (sock, buf, BUFSIZ);
      if (nbytes <= 0)
        break;
      write (STDOUT_FILENO, buf, nbytes);
    }

  /* Close socket.  */
  close (sock);

  return CLI_SUCCESS;
}

CLI (vtysh_show_privilege,
     vtysh_show_privilege_cli,
     "show privilege",
     "Show running system information",
     "Show current privilege level")
{
  printf ("Current privilege level is %d\n", cli->privilege);
  return CLI_SUCCESS;
}

ALI (vtysh_show_func,
     vtysh_show_interface_cli,
     "show interface (IFNAME|)",
     "Show running system information",
     "Interface status and configuration",
     "Interface name");

void
vtysh_cli_show_init ()
{
  int i;

  /* IMI shell local "show" commands.  */
  cli_install (ctree, EXEC_MODE, &vtysh_show_privilege_cli);

  /* IMI "show interface" command. */
  cli_install_imi (ctree, EXEC_MODE, PM_NSM, &vtysh_show_interface_cli);

  /* Shortcut commands.  */
  cli_install_shortcut (ctree, EXEC_MODE, "*p=ping", "p", "ping");
  cli_install_shortcut (ctree, EXEC_MODE, "*h=help", "h", "help");
  cli_install_shortcut (ctree, EXEC_MODE, "*lo=logout", "lo", "logout");
  cli_install_shortcut (ctree, EXEC_MODE, "*u=undebug", "u", "undebug");
  cli_install_shortcut (ctree, EXEC_MODE, "*un=undebug", "un", "undebug");

  /* Set show flag to "show" and "*s=show" node.  */
  for (i = 0; i < MAX_MODE; i++)
    {
      if (i != EXEC_PRIV_MODE)
        cli_install_shortcut (ctree, i, "*s=show", "s", "show");

      if (i != EXEC_MODE)
        {
          cli_set_node_flag (ctree, i, "show", CLI_FLAG_SHOW);
          cli_set_node_flag (ctree, i, "s", CLI_FLAG_SHOW);
        }
    }

  /* Sort CLI commands.  */
  cli_sort (ctree);
}
