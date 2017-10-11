/* Copyright (C) 2003  All Rights Reserved. */

#include "pal.h"

#include "cli.h"
#include "line.h"

#include "vtysh.h"
#include "vtysh_pm.h"
#include "vtysh_cli.h"

struct vtysh_pm_master *vtysh_pmaster;

#define MTYPE_VTYSH_PM         MTYPE_TMP
#define MTYPE_VTYSH_PM_MASTER  MTYPE_TMP

/* Free protocol master.  */
void
vtysh_pm_master_free (struct vtysh_pm_master *pmaster)
{
  vector_free (pmaster->pm);
  XFREE (MTYPE_VTYSH_PM_MASTER, pmaster);
}

/* Allocate a new protocol master.  */
struct vtysh_pm_master *
vtysh_pm_master_new ()
{
  struct vtysh_pm_master *pmaster;

  pmaster = XCALLOC (MTYPE_VTYSH_PM_MASTER, sizeof (struct vtysh_pm_master));
  pmaster->pm = vector_init (VECTOR_MIN_SIZE);
  if (! pmaster->pm)
    {
      vtysh_pm_master_free (pmaster);
      return NULL;
    }
  return pmaster;
}

/* Free protocol module structure.  */
void
vtysh_pm_free (struct vtysh_pm *pm)
{
  if (pm->name)
    XFREE (MTYPE_TMP, pm->name);
#ifndef HAVE_TCP_MESSAGE
  if (pm->path)
    XFREE (MTYPE_TMP, pm->path);
#endif /* ! HAVE_TCP_MESSAGE */
  XFREE (MTYPE_TMP, pm);
}

/* Allocate a new protocol module structure.  */
struct vtysh_pm *
vtysh_pm_new (char *name)
{
  struct vtysh_pm *pm;

  pm = XCALLOC (MTYPE_TMP, sizeof (struct vtysh_pm));
  if (! pm)
    return NULL;

  pm->name = XSTRDUP (MTYPE_TMP, name);
  if (! pm->name)
    {
      vtysh_pm_free (pm);
      return NULL;
    }

  pm->sock = -1;
  pm->connected = PAL_FALSE;
  pm->launched = PAL_FALSE;

  return pm;
}

/* Lookup protocol module from index.  */
struct vtysh_pm *
vtysh_pm_lookup (struct vtysh_pm_master *pmaster, int index)
{
  if (index >= APN_PROTO_MAX)
    return NULL;

  return vector_lookup_index (pmaster->pm, index);
}

/* Connect to the protocol module.  */
int
vtysh_pm_connect (struct vtysh_pm_master *pmaster, struct vtysh_pm *pm)
{
  pal_sock_handle_t sock;

  /* Already connected.  */
  if (pm->connected == PAL_TRUE)
    return -1;

  /* Connect using "line".  */
#ifdef HAVE_TCP_MESSAGE
  sock = line_client_sock (vtyshm, pm->port);
#else /* HAVE_TCP_MESSAGE */
  sock = line_client_sock (vtyshm, pm->path);
#endif /* HAVE_TCP_MESSAGE */
  if (sock >= 0)
    {
      pm->sock = sock;
      pm->connected = PAL_TRUE;
    }
  else
    pm->connected = PAL_FALSE;

  return 0;
}

/* Close protocol module connection.  */
void
vtysh_pm_close (struct vtysh_pm *pm)
{
  if (pm->sock)
    {
      pal_sock_close (vtyshm, pm->sock);
      pm->sock = -1;
    }
  pm->connected = PAL_FALSE;
}

/* Connect to protocol module which already invoked.  */
int
vtysh_pm_register (struct vtysh_pm_master *pmaster, char *name, int index)
{
  struct vtysh_pm *pm;
#ifdef HAVE_TCP_MESSAGE
  int port;
#else /* HAVE_TCP_MESSAGE */
  char *path;
#endif /* HAVE_TCP_MESSAGE */

  if (index >= APN_PROTO_MAX)
    return -1;

  if (vector_lookup_index (pmaster->pm, index))
    return -1;

  /* Allocate a new protocol module information.  */
  pm = vtysh_pm_new (name);
  if (! pm)
    return -1;
#ifdef HAVE_TCP_MESSAGE
  port = line_get_port_from_protocol (index);
  if (port < 0)
    {
      vtysh_pm_free (pm);
      return -1;
    }

  pm->port = port;
#else /* HAVE_TCP_MESSAGE */
  path = line_get_path_from_protocol (index);
  if (! path)
    {
      vtysh_pm_free (pm);
      return -1;
    }

  pm->path = XSTRDUP (MTYPE_TMP, path);
  if (! pm->path)
    {
      vtysh_pm_free (pm);
      return -1;
    }
#endif /* HAVE_TCP_MESSAGE */

  /* Register pm structure to the master. */
  vector_set_index (pmaster->pm, index, pm);

  /* Try to connect to the protocol module.  */
  if (! pm->connected)
    {
      vtysh_pm_connect (pmaster, pm);

      if (pm->connected)
        pm->status = VTYSH_PM_STATUS_PRE_LAUNCHED;
      else
        pm->status = VTYSH_PM_STATUS_NOT_RUNNING;
    }

  return 0;
}

/* Try to connect all of possible protocols.  */
void
vtysh_pm_register_all (struct vtysh_pm_master *pmaster)
{
  vtysh_pm_register (pmaster, "NSM", APN_PROTO_NSM);
  vtysh_pm_register (pmaster, "RIP", APN_PROTO_RIP);
  vtysh_pm_register (pmaster, "RIPng", APN_PROTO_RIPNG);
  vtysh_pm_register (pmaster, "OSPF", APN_PROTO_OSPF);
  vtysh_pm_register (pmaster, "OSPFv3", APN_PROTO_OSPF6);
  vtysh_pm_register (pmaster, "IS-IS", APN_PROTO_ISIS);
  vtysh_pm_register (pmaster, "BGP", APN_PROTO_BGP);
  vtysh_pm_register (pmaster, "LDP", APN_PROTO_LDP);
  vtysh_pm_register (pmaster, "RSVP", APN_PROTO_RSVP);
  vtysh_pm_register (pmaster, "PIM-DM", APN_PROTO_PIMDM);
  vtysh_pm_register (pmaster, "PIM", APN_PROTO_PIMSM);
  vtysh_pm_register (pmaster, "PIM6", APN_PROTO_PIMSM6);
  vtysh_pm_register (pmaster, "DVMRP", APN_PROTO_DVMRP);
  vtysh_pm_register (pmaster, "STP", APN_PROTO_STP);
  vtysh_pm_register (pmaster, "RSTP", APN_PROTO_RSTP);
  vtysh_pm_register (pmaster, "AUTH", APN_PROTO_8021X);
  vtysh_pm_register (pmaster, "ONMD", APN_PROTO_ONM);
  vtysh_pm_register (pmaster, "PIMPKTGEN", APN_PROTO_PIMPKTGEN);
}

const char *
vtysh_pm_status_str (int status)
{
  switch (status)
    {
    case VTYSH_PM_STATUS_NOT_RUNNING:
      return "Not running";
    case VTYSH_PM_STATUS_PRE_LAUNCHED:
      return "Pre-launched";
    case VTYSH_PM_STATUS_LAUNCHED:
      return "Launced by IMI";
    case VTYSH_PM_STATUS_RESTARTED:
      return "Restarted";
    default:
      return "";
    }
}

CLI (show_process,
     show_process_cli,
     "show process",
     CLI_SHOW_STR,
     "Process")
{
  int i;
  struct vtysh_pm *pm;

  cli_out (cli, "%-10s%-15s%10s\n", "Process", "Status", "FD");
  cli_out (cli, "------------------------------------\n");

  for (i = 0; i < vector_max (vtysh_pmaster->pm); i++)
    if ((pm = vector_slot (vtysh_pmaster->pm, i)) != NULL)
      {
        if (pm->connected == PAL_TRUE)
          cli_out (cli, "%-10s%-15s%10d\n",
                   pm->name, vtysh_pm_status_str (pm->status), pm->sock);
      }
  cli_out (cli, "------------------------------------\n");

  return CLI_SUCCESS;
}

CLI (show_vtysh_client,
     show_vtysh_client_cli,
     "show vtysh client",
     CLI_SHOW_STR
     "VTYSH",
     "Clients")
{
  int i;
  struct vtysh_pm *pm;

  for (i = 0; i < vector_max (vtysh_pmaster->pm); i++)
    if ((pm = vector_slot (vtysh_pmaster->pm, i)) != NULL)
      {
        if (pm->connected == PAL_TRUE)
          cli_out (cli, "VTYSH Client: %-10s\n  socket : %10d\n",
                   pm->name, pm->sock);
      }

  return CLI_SUCCESS;
}

/* Shutdown the protocol module connection.  */
void
vtysh_pm_shutdown ()
{
  int i;
  struct vtysh_pm *pm;

  for (i = 0; i < APN_PROTO_MAX; i++)
    {
      pm = vtysh_pm_lookup (vtysh_pmaster, i);
      if (! pm)
        continue;

      if (pm->sock)
        pal_sock_close (vtyshm, pm->sock);

      vtysh_pm_free (pm);
    }
}

/* Initialization of protocol damon manager.  */
void
vtysh_pm_init ()
{
  /* Allocate a new protocol module master.  */
  vtysh_pmaster = vtysh_pm_master_new ();
  if (! vtysh_pmaster)
    return;

  /* Some protocol may be launched already.  */
  vtysh_pm_register_all (vtysh_pmaster);

  /* "show process" */
  cli_install (ctree, EXEC_MODE, &show_process_cli);
  cli_install (ctree, EXEC_PRIV_MODE, &show_vtysh_client_cli);
}
