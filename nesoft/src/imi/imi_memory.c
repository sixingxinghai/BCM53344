/* Copyright (C) 2003  All Rights Reserved. */

#include <pal.h>

#ifdef MEMMGR

#include "lib.h"
#include "cli.h"
#include "snprintf.h"

#include "imi/imi.h"

#define IMI_MEMORY_LINE_BUFSIZ                          80


const char *
imi_memory_proto2cmd (int proto)
{
  if (proto == APN_PROTO_NSM)
    return "show memory nsm";
#ifdef HAVE_RIPD
  else if (proto == APN_PROTO_RIP)
    return "show memory rip";
#endif /* HAVE_RIPD */
#ifdef HAVE_RIPNGD
  else if (proto == APN_PROTO_RIPNG)
    return "show memory ipv6 rip";
#endif /* HAVE_RIPNGD */
#ifdef HAVE_OSPFD
  else if (proto == APN_PROTO_OSPF)
    return "show memory ospf";
#endif /* HAVE_OSPFD */
#ifdef HAVE_OSPF6D
  else if (proto == APN_PROTO_OSPF6)
    return "show memory ipv6 ospf";
#endif /* HAVE_OSPF6D */
#ifdef HAVE_ISISD
  else if (proto == APN_PROTO_ISIS)
    return "show memory isis";
#endif /* HAVE_ISISD */
#ifdef HAVE_BGPD
  else if (proto == APN_PROTO_BGP)
    return "show memory bgp";
#endif /* HAVE_BGPD */
#ifdef HAVE_LDPD
  else if (proto == APN_PROTO_LDP)
    return "show memory ldp";
#endif /* HAVE_LDPD */
#ifdef HAVE_RSVPD
  else if (proto == APN_PROTO_RSVP)
    return "show memory rsvp";
#endif /* HAVE_RSVPD */
#ifdef HAVE_PDMD
  else if (proto == APN_PROTO_PIMDM)
    return "show memory pim dense-mode";
#endif /* HAVE_PDMD */
#ifdef HAVE_PIMD
  else if (proto == APN_PROTO_PIMSM)
    return "show memory pim sparse-mode";
#endif /* HAVE_PIMD */
#ifdef HAVE_PIM6D
  else if (proto == APN_PROTO_PIMSM6)
    return "show memory ipv6 pim sparse-mode";
#endif /* HAVE_PIM6D */
#ifdef HAVE_DVMRPD
  else if (proto == APN_PROTO_DVMRP)
    return "show memory dvmrp";
#endif /* HAVE_DVMRPD */
#ifdef HAVE_AUTHD
  else if (proto == APN_PROTO_8021X)
    return "show memory dot1x";
#endif /* HAVE_AUTHD */
#ifdef HAVE_LACPD
  else if (proto == APN_PROTO_LACP)
    return "show memory lacp";
#endif /* HAVE_LACPD */
#ifdef HAVE_STPD
  else if (proto == APN_PROTO_STP)
    return "show memory stp";
#endif /* HAVE_STPD */
#ifdef HAVE_RSTPD
  else if (proto == APN_PROTO_RSTP)
    return "show memory rstp";
#endif /* HAVE_RSTPD */
#ifdef HAVE_MSTPD
  else if (proto == APN_PROTO_MSTP)
    return "show memory mstp";
#endif /* HAVE_MSTPD */
  else if (proto == APN_PROTO_ONM)
    return "show memory onm";
#ifdef HAVE_ELMID
  else if (proto == APN_PROTO_ELMI)
    return "show memory elmi";
#endif /* HAVE_ELMID */
  else
    return NULL;
}

void
imi_memory_parse (void *arg, char *line, module_id_t module)
{
  struct cli *cli = (struct cli *)arg;

  cli_out (cli, "%s\n", line);
}

void
imi_memory_show_memory_imi (struct cli *cli, int all)
{
  cli_out (cli, "\nLibrary memories for IMI\n\n");
  memmgr_show_memory_module (cli, APN_PROTO_UNSPEC);

  if (all)
    {
      cli_out (cli, "\nMemories for IMI\n\n");
      memmgr_show_memory_module (cli, APN_PROTO_IMI);
    }
}

void
imi_memory_show_memory_module (struct cli *cli, int proto, int all)
{
  char buf[IMI_MEMORY_LINE_BUFSIZ];
  char *str;

  str = (char *)imi_memory_proto2cmd (proto);
  if (str == NULL)
    return;

  zsnprintf (buf, sizeof (buf),
             "\nLibrary memories for %s\n\n", modname_strl (proto));
  imi_show_protocol (cli->vr, proto, buf, "show memory lib",
                     cli, &imi_memory_parse);

  if (all)
    {
      zsnprintf (buf, sizeof (buf),
                 "\nMemories for %s\n\n", modname_strl (proto));
      imi_show_protocol (cli->vr, proto, buf, str, cli, &imi_memory_parse);
    }
}

void
imi_memory_show_memory (struct cli *cli, int all)
{
  int proto;

  /* IMI.  */
  imi_memory_show_memory_imi (cli, all);

  /* NSM and PMs.  */
  FOREACH_APN_PROTO (proto)
    if (imi_memory_proto2cmd (proto) != NULL)
      imi_memory_show_memory_module (cli, proto, all);
}

void
imi_memory_show_memory_summary (struct cli *cli)
{
  char buf[IMI_MEMORY_LINE_BUFSIZ];
  int proto;

  /* IMI.  */
  cli_out (cli, "\nMemory summary for IMI\n\n");
  memmgr_show_memory_summary (cli);

  /* NSM and PMs.  */
  FOREACH_APN_PROTO (proto)
    if (imi_memory_proto2cmd (proto) != NULL)
      {
        zsnprintf (buf, sizeof (buf),
                   "\nMemory summary for %s\n\n", modname_strl (proto));
        imi_show_protocol (cli->vr, proto, buf, "show memory summary",
                           cli, &imi_memory_parse);
      }
}

void
imi_memory_show_memory_free (struct cli *cli)
{
  char buf[IMI_MEMORY_LINE_BUFSIZ];
  int proto;

  /* IMI.  */
  cli_out (cli, "\nFreed memories for IMI\n\n");
  memmgr_show_memory_free (cli);

  /* NSM and PMs.  */
  FOREACH_APN_PROTO (proto)
    if (imi_memory_proto2cmd (proto) != NULL)
      {
        zsnprintf (buf, sizeof (buf),
                   "\nFreed memories for %s\n\n", modname_strl (proto));
        imi_show_protocol (cli->vr, proto, buf, "show memory free",
                           cli, &imi_memory_parse);
      }
}

CLI (imi_show_memory,
     imi_show_memory_cmd,
     "show memory (all|lib|)",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     "All memory information",
     "Library information")
{
  int all = 0;

  if (argc == 0 || pal_strncmp (argv[0], "a", 1) == 0)
    all = 1;

  /* Show all the module's memory.  */
  imi_memory_show_memory (cli, all);

  return CLI_SUCCESS;
}

CLI (imi_show_memory_summary,
     imi_show_memory_summary_cmd,
     "show memory summary",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     "Summary memory information")
{
  imi_memory_show_memory_summary (cli);
  return CLI_SUCCESS;
}

CLI (imi_show_memory_free,
     imi_show_memory_free_cmd,
     "show memory free",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     "Free memory pool information")
{
  imi_memory_show_memory_free (cli);
  return CLI_SUCCESS;
}

CLI (imi_show_memory_imi,
     imi_show_memory_imi_cmd,
     "show memory imi",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_IMI_STR)
{
  imi_memory_show_memory_imi (cli, 1);
  return CLI_SUCCESS;
}

CLI (imi_show_memory_nsm,
     imi_show_memory_nsm_cmd,
     "show memory nsm",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_NSM_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_NSM, 1);
  return CLI_SUCCESS;
}

#ifdef HAVE_RIPD
CLI (imi_show_memory_rip,
     imi_show_memory_rip_cmd,
     "show memory rip",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_RIP_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_RIP, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_RIPD */

#ifdef HAVE_RIPNGD
CLI (imi_show_memory_ripng,
     imi_show_memory_ripng_cmd,
     "show memory ipv6 rip",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_IPV6_STR,
     CLI_RIPNG_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_RIPNG, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_RIPNG */

#ifdef HAVE_OSPFD
CLI (imi_show_memory_ospf,
     imi_show_memory_ospf_cmd,
     "show memory ospf",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_OSPF_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_OSPF, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_OSPFD */

#ifdef HAVE_OSPF6D
CLI (imi_show_memory_ospf6,
     imi_show_memory_ospf6_cmd,
     "show memory ipv6 ospf",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_IPV6_STR,
     CLI_OSPF6_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_OSPF6, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_OSPF6D */

#ifdef HAVE_ISISD
CLI (imi_show_memory_isis,
     imi_show_memory_isis_cmd,
     "show memory isis",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_ISIS_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_ISIS, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_ISISD */

#ifdef HAVE_BGPD
CLI (imi_show_memory_bgp,
     imi_show_memory_bgp_cmd,
     "show memory bgp",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_BGP_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_BGP, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_BGPD */

#ifdef HAVE_LDPD
CLI (imi_show_memory_ldp,
     imi_show_memory_ldp_cmd,
     "show memory ldp",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_LDP_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_LDP, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_LDPD */

#ifdef HAVE_RSVPD
CLI (imi_show_memory_rsvp,
     imi_show_memory_rsvp_cmd,
     "show memory rsvp",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_RSVP_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_RSVP, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_RSVPD */

#ifdef HAVE_PDMD
CLI (imi_show_memory_pim_dm,
     imi_show_memory_pim_dm_cmd,
     "show memory pim dense-mode",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_PIM_STR,
     "Dense-mode")
{
  imi_memory_show_memory_module (cli, APN_PROTO_PIMDM, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_PDMD */

#ifdef HAVE_PIMD
CLI (imi_show_memory_pim,
     imi_show_memory_pim_cmd,
     "show memory pim sparse-mode",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_PIM_STR,
     "Sparse-mode")
{
  imi_memory_show_memory_module (cli, APN_PROTO_PIMSM, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_PIMD */

#ifdef HAVE_PIM6D
CLI (imi_show_memory_pim6,
     imi_show_memory_pim6_cmd,
     "show memory ipv6 pim sparse-mode",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_IPV6_STR,
     CLI_PIM6_STR,
     "Sparse-mode")
{
  imi_memory_show_memory_module (cli, APN_PROTO_PIMSM6, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_PIM6D */

#ifdef HAVE_DVMRPD
CLI (imi_show_memory_dvmrp,
     imi_show_memory_dvmrp_cmd,
     "show memory dvmrp",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_DVMRP_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_DVMRP, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_DVMRPD */

#ifdef HAVE_AUTHD
CLI (imi_show_memory_dot1x,
     imi_show_memory_dot1x_cmd,
     "show memory dot1x",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_8021X_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_8021X, 1);
  return CLI_SUCCESS;
}

ALI (imi_show_memory_dot1x,
     imi_show_memory_auth_cmd,
     "show memory auth",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_8021X_STR);
#endif /* HAVE_AUTHD */

#ifdef HAVE_ONMD
CLI (imi_show_memory_onmd,
     imi_show_memory_onmd_cmd,
     "show memory onmd",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_ONMD_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_ONM, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_ONMD */

#ifdef HAVE_LACPD
CLI (imi_show_memory_lacp,
     imi_show_memory_lacp_cmd,
     "show memory lacp",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_LACP_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_LACP, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_LACPD */

#ifdef HAVE_STPD
CLI (imi_show_memory_stp,
     imi_show_memory_stp_cmd,
     "show memory stp",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_STP_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_STP, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_STPD */

#ifdef HAVE_RSTPD
CLI (imi_show_memory_rstp,
     imi_show_memory_rstp_cmd,
     "show memory rstp",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_RSTP_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_RSTP, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_RSTPD */

#ifdef HAVE_MSTPD
CLI (imi_show_memory_mstp,
     imi_show_memory_mstp_cmd,
     "show memory mstp",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_MSTP_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_MSTP, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_MSTPD */

#ifdef HAVE_ELMID
CLI (imi_show_memory_elmi,
     imi_show_memory_elmi_cmd,
     "show memory elmi",
     CLI_SHOW_STR,
     CLI_SHOW_MEMORY_STR,
     CLI_ELMI_STR)
{
  imi_memory_show_memory_module (cli, APN_PROTO_ELMI, 1);
  return CLI_SUCCESS;
}
#endif /* HAVE_ELMID */

void
imi_memory_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_cmd);
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_summary_cmd);
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_free_cmd);

  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_imi_cmd);
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_nsm_cmd);

#ifdef HAVE_RIPD
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_rip_cmd);
#endif /* HAVE_RIPD */

#ifdef HAVE_RIPNGD
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_ripng_cmd);
#endif /* HAVE_RIPNGD */

#ifdef HAVE_OSPFD
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_ospf_cmd);
#endif /* HAVE_OSPFD */

#ifdef HAVE_OSPF6D
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_ospf6_cmd);
#endif /* HAVE_OSPF6D */

#ifdef HAVE_ISISD
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_isis_cmd);
#endif /* HAVE_ISISD */

#ifdef HAVE_BGPD
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_bgp_cmd);
#endif /* HAVE_BGPD */

#ifdef HAVE_LDPD
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_ldp_cmd);
#endif /* HAVE_LDPD */
   
#ifdef HAVE_RSVPD
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_rsvp_cmd);
#endif /* HAVE_RSVPD */

#ifdef HAVE_PDMD
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_pim_dm_cmd);
#endif /* HAVE_PDMD */

#ifdef HAVE_PIMD
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_pim_cmd);
#endif /* HAVE_PIMD */

#ifdef HAVE_PIM6D
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_pim6_cmd);
#endif /* HAVE_PIM6D */

#ifdef HAVE_DVMRPD
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_dvmrp_cmd);
#endif /* HAVE_DVMRPD */

#ifdef HAVE_AUTHD
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_dot1x_cmd);
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, CLI_FLAG_HIDDEN,
                   &imi_show_memory_auth_cmd);
#endif /* HAVE_AUTHD */

#ifdef HAVE_LACPD
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_lacp_cmd);
#endif /* HAVE_LACPD */

#ifdef HAVE_STPD
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_stp_cmd);
#endif /* HAVE_STPD */

#ifdef HAVE_MSTPD
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_mstp_cmd);
#endif /* HAVE_MSTPD */

#ifdef HAVE_RSTPD
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_rstp_cmd);
#endif /* HAVE_RSTPD */

#ifdef HAVE_ELMID
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_show_memory_elmi_cmd);
#endif /* HAVE_ELMID */
}

#endif /* MEMMGR */
