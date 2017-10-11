/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_VR

#include "lib.h"
#include "cli.h"
#include "modbmap.h"

#include "imi/imi.h"
#include "imi_server.h"

#ifndef HAVE_IMISH
#include "imi_cli.h"
#endif

void
imi_vr_pm_update (struct apn_vr *vr)
{
  struct imi_globals *ig = vr->zg->proto;
  struct imi_master *im = vr->proto;

  /* Update the active PM bit.  */
  im->module = modbmap_and (im->module_config, ig->module);

#ifdef HAVE_IMISH
  /* Kick all the config sessions connected to this VR out to the
     CONFIG_MODE.
   */
  imi_server_line_pm_connect (vr);
#else
  /* Same as above for VTY sessions. */
  imi_vty_pm_connect(vr);
#endif
}

#ifdef HAVE_MCAST_IPV4
#define IMI_MCAST_VR_LOAD_STR         "|pimsm|pimdm"

#define IMI_MCAST_VR_LOAD_HLP_STR                                             \
  , CLI_PIM_STR " " CLI_PIMSM_STR,                                        \
    CLI_PIM_STR " " CLI_PIMDM_STR
#else
#define IMI_MCAST_VR_LOAD_STR         ""

#define IMI_MCAST_VR_LOAD_HLP_STR     ""
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
#define IMI_MCAST6_VR_LOAD_STR         "|pimsm6|pimdm"

#define IMI_MCAST6_VR_LOAD_HLP_STR                                             \
  , CLI_PIM_STR " " CLI_PIMSM6_STR,                                        \
    CLI_PIM_STR " " CLI_PIMDM6_STR
#else
#define IMI_MCAST6_VR_LOAD_STR         ""

#define IMI_MCAST6_VR_LOAD_HLP_STR     ""
#endif /* HAVE_MCAST_IPV4 */

CLI (imi_virtual_router_desc,
     imi_virtual_router_desc_cli,
     "description LINE",
     "VR specific description",
     "Characters describing this VR")
{
  struct apn_vr *vr = cli->index;
  struct imi_master *im = vr->proto;

  if (im->desc)
    XFREE (MTYPE_IMI_STRING, im->desc);

  im->desc = XSTRDUP (MTYPE_IMI_STRING, argv[0]);

  return CLI_SUCCESS;
}

CLI (imi_no_virtual_router_desc,
     imi_no_virtual_router_desc_cli,
     "no description",
     CLI_NO_STR,
     "VR specific description")
{
  struct apn_vr *vr = cli->index;
  struct imi_master *im = vr->proto;

  if (im->desc)
    {
      XFREE (MTYPE_IMI_STRING, im->desc);
      im->desc = NULL;
    }

  return CLI_SUCCESS;
}

CLI (imi_virtual_router_load_module,
     imi_virtual_router_load_module_cli,
     "load (ospf|bgp|rip|isis" IMI_MCAST_VR_LOAD_STR ")",
     "Load module to VR",
     "Open Shortest Path First (OSPF)",
     "Border Gateway Protocol (BGP)",
     "Routing Information Protocol (RIP)",
     "Intermediate System - Intermediate System (IS-IS)" IMI_MCAST_VR_LOAD_HLP_STR )
{
  struct apn_vr *vr = cli->index;
  struct imi_master *im = vr->proto;

#ifdef HAVE_MCAST_IPV4
  if (pal_strcmp (argv[0], "pimsm") == 0)
    MODBMAP_SET (im->module_config, APN_PROTO_PIMSM);
  else if (pal_strcmp (argv[0], "pimdm") == 0)
    MODBMAP_SET (im->module_config, APN_PROTO_PIMDM);
#endif /* HAVE_MCAST_IPV4 */

  if (pal_strncmp (argv[0], "o", 1) == 0)
    MODBMAP_SET (im->module_config, APN_PROTO_OSPF);
  else if (pal_strncmp (argv[0], "b", 1) == 0)
    MODBMAP_SET (im->module_config, APN_PROTO_BGP);
  else if (pal_strncmp (argv[0], "r", 1) == 0)
    MODBMAP_SET (im->module_config, APN_PROTO_RIP);
  else if (pal_strncmp (argv[0], "i", 1) == 0)
    MODBMAP_SET (im->module_config, APN_PROTO_ISIS);

  /* Update active PM bit.  */
  imi_vr_pm_update (vr);

  return CLI_SUCCESS;
}

#ifdef HAVE_IPV6
CLI (imi_virtual_router_load_module_ipv6,
     imi_virtual_router_load_module_ipv6_cli,
     "load ipv6 (ospf|rip" IMI_MCAST6_VR_LOAD_STR ")",
     "Load module to VR",
     "Internet Protocol Version 6",
     "Open Shortest Path First (OSPF)",
     "Routing Information Protocol (RIP)" IMI_MCAST6_VR_LOAD_HLP_STR )
{
  struct apn_vr *vr = cli->index;
  struct imi_master *im = vr->proto;

#ifdef HAVE_MCAST_IPV6
  if (pal_strcmp (argv[0], "pimsm6") == 0)
    MODBMAP_SET (im->module_config, APN_PROTO_PIMSM6);
  else if (pal_strcmp (argv[0], "pimdm") == 0)
    MODBMAP_SET (im->module_config, APN_PROTO_PIMDM);
#endif /* HAVE_MCAST_IPV6 */

  if (pal_strncmp (argv[0], "o", 1) == 0)
    MODBMAP_SET (im->module_config, APN_PROTO_OSPF6);
  else if (pal_strncmp (argv[0], "r", 1) == 0)
    MODBMAP_SET (im->module_config, APN_PROTO_RIPNG);

  /* Update active PM bit.  */
  imi_vr_pm_update (vr);

  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */

CLI (imi_no_virtual_router_load_module,
     imi_no_virtual_router_load_module_cli,
     "no load (ospf|bgp|rip|isis" IMI_MCAST_VR_LOAD_STR ")",
     CLI_NO_STR,
     "Load module to VR",
     "Open Shortest Path First (OSPF)",
     "Border Gateway Protocol (BGP)",
     "Routing Information Protocol (RIP)",
     "Intermediate System - Intermediate System (IS-IS)" IMI_MCAST_VR_LOAD_HLP_STR )
{
  struct apn_vr *vr = cli->index;
  struct imi_master *im = vr->proto;

#ifdef HAVE_MCAST_IPV4
  if (pal_strcmp (argv[0], "pimsm") == 0)
    MODBMAP_UNSET (im->module_config, APN_PROTO_PIMSM);
  else if (pal_strcmp (argv[0], "pimdm") == 0)
    MODBMAP_UNSET (im->module_config, APN_PROTO_PIMDM);
#endif /* HAVE_MCAST_IPV4 */

  if (pal_strncmp (argv[0], "o", 1) == 0)
    MODBMAP_UNSET (im->module_config, APN_PROTO_OSPF);
  else if (pal_strncmp (argv[0], "b", 1) == 0)
    MODBMAP_UNSET (im->module_config, APN_PROTO_BGP);
  else if (pal_strncmp (argv[0], "r", 1) == 0)
    MODBMAP_UNSET (im->module_config, APN_PROTO_RIP);
  else if (pal_strncmp (argv[0], "i", 1) == 0)
    MODBMAP_UNSET (im->module_config, APN_PROTO_ISIS);

  /* Update active PM bit.  */
  imi_vr_pm_update (vr);

  return CLI_SUCCESS;
}

#ifdef HAVE_IPV6
CLI (imi_no_virtual_router_load_module_ipv6,
     imi_no_virtual_router_load_module_ipv6_cli,
     "no load ipv6 (ospf|rip" IMI_MCAST6_VR_LOAD_STR ")",
     CLI_NO_STR,
     "Load module to VR",
     "Internet Protocol Version 6",
     "Open Shortest Path First (OSPF)",
     "Routing Information Protocol (RIP)" IMI_MCAST6_VR_LOAD_HLP_STR )
{
  struct apn_vr *vr = cli->index;
  struct imi_master *im = vr->proto;

#ifdef HAVE_MCAST_IPV6
  if (pal_strcmp (argv[0], "pimsm6") == 0)
    MODBMAP_UNSET (im->module_config, APN_PROTO_PIMSM6);
  else if (pal_strcmp (argv[0], "pimdm") == 0)
    MODBMAP_UNSET (im->module_config, APN_PROTO_PIMDM);
#endif /* HAVE_MCAST_IPV6 */

  if (pal_strncmp (argv[0], "o", 1) == 0)
    MODBMAP_UNSET (im->module_config, APN_PROTO_OSPF6);
  else if (pal_strncmp (argv[0], "r", 1) == 0)
    MODBMAP_UNSET (im->module_config, APN_PROTO_RIPNG);

  /* Update active PM bit.  */
  imi_vr_pm_update (vr);

  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */

CLI (imi_virtual_router_enable,
     imi_virtual_router_enable_cli,
     "enable-vr",
     "Enable this Virtual Router")
{
  struct apn_vr *vr = cli->index;
  struct imi_master *im = vr->proto;

  /* Enables all the VR capable PMs.  */
  im->module_config = PM_VR;

  /* Update active PM bit.  */
  imi_vr_pm_update (vr);

  return CLI_SUCCESS;
}

CLI (imi_virtual_router_disable,
     imi_virtual_router_disable_cli,
     "disable-vr",
     "Disable this Virtual Router")
{
  struct apn_vr *vr = cli->index;
  struct imi_master *im = vr->proto;

  /* Disable all the VR capable PMs.  */
  im->module_config = PM_NSM;

  /* Update active PM bit.  */
  imi_vr_pm_update (vr);

  return CLI_SUCCESS;
}

CLI (imi_virtual_router_configuration_file,
     imi_virtual_router_configuration_file_cli,
     "configuration file WORD",
     "Virtual Router configuration",
     "Specify filename",
     "Filename")
{
  struct apn_vr *vr = cli->index;

  if (pal_strchr (argv[0], PAL_FILE_SEPARATOR) != NULL)
    {
      cli_out (cli, "%% You can not specify filename including `%c'\n",
               PAL_FILE_SEPARATOR);
      return CLI_ERROR;
    }

  imi_config_file_set (vr, argv[0]);

#ifdef HAVE_IMISH
  imi_config_read (vr);
#endif /* HAVE_IMISH */

  return CLI_SUCCESS;
}

CLI (imi_no_virtual_router_configuration_file,
     imi_no_virtual_router_configuration_file_cli,
     "no configuration file",
     CLI_NO_STR,
     "Virtual Router configuration",
     "Specify filename")
{
  struct apn_vr *vr = cli->index;

  imi_config_file_set (vr, NULL);

#ifdef HAVE_IMISH
  imi_config_read (vr);
#endif /* HAVE_IMISH */

  return CLI_SUCCESS;
}

ALI (imi_no_virtual_router_configuration_file,
     imi_virtual_router_configuration_file_default_cli,
     "configuration file default",
     "Virtual Router configuration",
     "Specify filename",
     "Set config filename to default");

CLI (imi_virtual_router_configuration_read_write,
     imi_virtual_router_configuration_read_write_cli,
     "configuration (read|write)",
     "Virtual Router configuration",
     "Read file",
     "Write file")
{
  return CLI_SUCCESS;
}

void
imi_vr_cli_init (struct cli_tree *ctree)
{
  cli_install_imi (ctree, VR_MODE, PM_NSM, PRIVILEGE_PVR_MAX, 0,
                   &imi_virtual_router_load_module_cli);
  cli_set_imi_cmd (&imi_virtual_router_load_module_cli, VR_MODE, CFG_DTYP_VR);

#ifdef HAVE_IPV6
  cli_install_imi (ctree, VR_MODE, PM_NSM, PRIVILEGE_PVR_MAX, 0,
                   &imi_virtual_router_load_module_ipv6_cli);
  cli_set_imi_cmd (&imi_virtual_router_load_module_ipv6_cli, VR_MODE, CFG_DTYP_VR);
#endif /* HAVE_IPV6 */

  cli_install_imi (ctree, VR_MODE, PM_NSM, PRIVILEGE_PVR_MAX, 0,
                   &imi_no_virtual_router_load_module_cli);
#ifdef HAVE_IPV6
  cli_install_imi (ctree, VR_MODE, PM_NSM, PRIVILEGE_PVR_MAX, 0,
                   &imi_no_virtual_router_load_module_ipv6_cli);
#endif /* HAVE_IPV6 */

  cli_install_imi (ctree, VR_MODE, PM_NSM, PRIVILEGE_PVR_MAX, CLI_FLAG_HIDDEN,
                   &imi_virtual_router_enable_cli);
  cli_set_imi_cmd (&imi_virtual_router_enable_cli, VR_MODE, CFG_DTYP_VR);

  cli_install_imi (ctree, VR_MODE, PM_NSM, PRIVILEGE_PVR_MAX, CLI_FLAG_HIDDEN,
                   &imi_virtual_router_disable_cli);
  cli_set_imi_cmd (&imi_virtual_router_disable_cli, VR_MODE, CFG_DTYP_VR);

  cli_install_imi (ctree, VR_MODE, PM_NSM, PRIVILEGE_PVR_MAX, 0,
                   &imi_virtual_router_desc_cli);
  cli_set_imi_cmd (&imi_virtual_router_desc_cli, VR_MODE, CFG_DTYP_VR);

  cli_install_imi (ctree, VR_MODE, PM_NSM, PRIVILEGE_PVR_MAX, 0,
                   &imi_no_virtual_router_desc_cli);

  cli_install_gen (ctree, VR_MODE, PRIVILEGE_PVR_MAX, 0,
                   &imi_virtual_router_configuration_file_cli);

  cli_install_gen (ctree, VR_MODE, PRIVILEGE_PVR_MAX, 0,
                   &imi_no_virtual_router_configuration_file_cli);

  cli_install_gen (ctree, VR_MODE, PRIVILEGE_PVR_MAX, CLI_FLAG_HIDDEN,
                   &imi_virtual_router_configuration_file_default_cli);

  cli_install_gen (ctree, VR_MODE, PRIVILEGE_PVR_MAX, CLI_FLAG_HIDDEN,
                   &imi_virtual_router_configuration_read_write_cli);
}

#endif /* HAVE_VR */
