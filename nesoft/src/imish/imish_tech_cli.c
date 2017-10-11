/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "pal_ping.h"
#include "pal_traceroute.h"

#include "cli.h"
#include "line.h"
#include "message.h"
#include "imi_client.h"
#include "modbmap.h"
#include "libedit/readline/readline.h"

#include "imish/imish.h"
#include "imish/imish_exec.h"
#include "imish/imish_cli.h"
#include "imish/imish_parser.h"
#include "imish/imish_system.h"
#include "imish/imish_line.h"
#include "imish/imish_readline.h"
#include "lib/cli.h"


#ifndef HAVE_CUSTOM1

typedef void (* imish_tech_show_fun_t)(struct cli *);


/*--------------------------------------------------------------------------
 * _imish_tech_feature - Descriptor of a single feature as a user selectable
 *                       report item.
 * Members:
 *    itf_name    - Feature name (bgp, rsvp, etc.)
 *    itf_fun     - Function to invoke feature specific show commands.
 *    itf_vr_only - Tells whether the feature is VR/L3 (TRUE) or PVR specific.
 */
typedef struct _imish_tech_feature
{
  char                  *itf_name;
  imish_tech_show_fun_t  itf_fun;
  bool_t                 itf_vr_only;
} imish_tech_feature_t;


/*------------------------------------------------------------------
 * _imish_tech_exec - Generic function to display output of single
 *                    show command.
 */
static void
_imish_tech_exec(struct cli *cli, char *cmdstr, int viewflag)
{
  if (viewflag)
    cli_out(cli, "\n*** %s ***\n", cmdstr);

  imish_parse (cmdstr, EXEC_MODE);

  if (viewflag)
    cli_out(cli, "\n");

  return;
}

/*------------------------------------------------------------------
 * _imish_tech_is_in_argv - Find out whether arg is on the argv list.
 */
static bool_t
_imish_tech_is_in_argv (char *arg, int argc, char **argv)
{
  s_int16_t aix;

  for (aix=0; aix<argc; aix++)
    if (! pal_strcmp(arg, argv[aix]))
      return PAL_TRUE;
  return PAL_FALSE;
}


/*===============================================================
 * Group show functions - one function per feature.
 */

static void
_imish_tech_write_bgp(struct cli *cli)
{
  cli_out(cli, "\n????  BGP content not available yet ????\n");
}

static void
_imish_tech_write_cef(struct cli *cli)
{
  cli_out(cli, "\n????  CEF content not available yet ????\n");
}

static void
_imish_tech_write_isis(struct cli *cli)
{
  _imish_tech_exec(cli, "show isis database", 1);
  _imish_tech_exec(cli, "show ip protocols", 1);
  _imish_tech_exec(cli, "show clns neighbors detail", 1);
  _imish_tech_exec(cli, "show isis interface", 1);
  _imish_tech_exec(cli, "show isis interface counter", 1);
}

static void
_imish_tech_write_lacp(struct cli *cli)
{
  cli_out(cli, "\n????  LACP content not available yet ????\n");
}

static void
_imish_tech_write_mpls(struct cli *cli)
{
  cli_out(cli, "\n????  MPLS content not available yet ????\n");
}

static void
_imish_tech_write_mstp(struct cli *cli)
{
  cli_out(cli, "\n????  MSTP content not available yet ????\n");
}

static void
_imish_tech_write_ospf(struct cli *cli)
{
  _imish_tech_exec(cli, "show ip ospf", 1);
  _imish_tech_exec(cli, "show ip ospf neighbor", 1);
  _imish_tech_exec(cli, "show ip ospf database", 1);
  _imish_tech_exec(cli, "show ip ospf interface", 1);
  _imish_tech_exec(cli, "show ip ospf route", 1);
}

static void
_imish_tech_write_pim(struct cli *cli)
{
  _imish_tech_exec(cli, "show ip pim sparse-mode mroute", 1);
  _imish_tech_exec(cli, "show ip pim sparse-mode interface", 1);
  _imish_tech_exec(cli, "show ip pim sparse-mode neighbor", 1);
  _imish_tech_exec(cli, "show ip pim sparse-mode rp mapping", 1);
  _imish_tech_exec(cli, "show ip pim sparse-mode nexthop", 1);
}

static void
_imish_tech_write_rip(struct cli *cli)
{
  _imish_tech_exec(cli, "show ip rip", 1);
  _imish_tech_exec(cli, "show ip rip interface", 1);
  _imish_tech_exec(cli, "show ip rip database", 1);
}

static void
_imish_tech_write_rsvp(struct cli *cli)
{
  cli_out(cli, "\n????  RSVP content not available yet ????\n");
}

/*===============================================================
 * Table of all features available for selection during show tech.
 */

static
imish_tech_feature_t _imish_tech_feature_tab[] =
{
/* name        write function       VR/L3 only? */

  {"bgp" , _imish_tech_write_bgp ,  PAL_TRUE },
  {"cef" , _imish_tech_write_cef ,  PAL_FALSE},
  {"isis", _imish_tech_write_isis,  PAL_TRUE },
  {"lacp", _imish_tech_write_lacp,  PAL_FALSE},
  {"mpls", _imish_tech_write_mpls,  PAL_FALSE},
  {"mstp", _imish_tech_write_mstp,  PAL_FALSE},
  {"ospf", _imish_tech_write_ospf,  PAL_TRUE },
  {"pim" , _imish_tech_write_pim ,  PAL_TRUE },
  {"rip" , _imish_tech_write_rip ,  PAL_TRUE },
  {"rsvp", _imish_tech_write_rsvp,  PAL_FALSE}
};

#define _IMISH_TECH_FEATURE_MAX() \
         (sizeof(_imish_tech_feature_tab)/sizeof(imish_tech_feature_t))

/*------------------------------------------------------------------
 * _show_tech_cmd_handler - The command handler.
 *
 * NOTE: This function is executed in recursively. In first run we find out
 *       whether paging is requried or not. Then we call imish_exec_with_pager
 *       that spawns w new process from which we execute the second
 *       part and generate the show output.
 */
static int
_show_tech_cmd_handler(struct cli *cli, int argc, char **argv, bool_t vr_only)
{
  s_int32_t back_lines;
  static imish_tech_feature_t *show_tab[_IMISH_TECH_FEATURE_MAX()], *fe;
  static int _imish_tech_pager_on = PAL_FALSE;
  s_int8_t  aix = 0;
  s_int16_t fix = 0;
  s_int16_t fcnt = 0;

  /* If paging is not set, we need to check whether it is ordered by the user
     and execute this handler again via imish_exec_with_pager.
   */
  if (! ctree->nested_show)
    {
      for (aix = 0; aix < argc; aix++)
        {
          if (! pal_strcmp(argv[aix], "page"))
            {
              _imish_tech_pager_on = PAL_TRUE;
              break;
            }
        }
      back_lines = cli->lines;
      if (! _imish_tech_pager_on)
        {
          /* Enforce no paging. */
          cli->lines = 0;
        }
      ctree->nested_show = PAL_TRUE;
      imish_exec_show(cli);
      cli->lines = back_lines;
      ctree->nested_show = PAL_FALSE;
      _imish_tech_pager_on = PAL_FALSE;
      return CLI_SUCCESS;
    }
  else
    { /* Create a table of fetaures to show.
       */
      for (fix = 0; fix < _IMISH_TECH_FEATURE_MAX(); fix++)
        {
          fe = &_imish_tech_feature_tab[fix];
          if (_imish_tech_is_in_argv (fe->itf_name, argc, argv))
              show_tab[fcnt++] = fe;
        }
      /* If no features selected, then include all of them.
       */
      if (! fcnt)
        /* Copy all entries for this PVR/VR context. */
        for (fix = 0; fix < _IMISH_TECH_FEATURE_MAX(); fix++)
          {
            fe = &_imish_tech_feature_tab[fix];
            if (! vr_only || (fe->itf_vr_only && vr_only))
              show_tab[fcnt++] = fe;
          }
    }
  /* Execute common commands */
  _imish_tech_exec(cli, "show version", 1);
  _imish_tech_exec(cli, "show process", 1);
  _imish_tech_exec(cli, "show ip interface brief", 1);
  _imish_tech_exec(cli, "show running-config", 1);
  _imish_tech_exec(cli, "show interface", 1);
  _imish_tech_exec(cli, "show arp", 1);
  _imish_tech_exec(cli, "show ip route", 1);
  _imish_tech_exec(cli, "show ip route summary", 1);

  fix = 0;
  while (fcnt > fix)
    {
      fe = show_tab[fix++];
      fe->itf_fun(cli);
    }
  return CLI_SUCCESS;
}


CLI (show_tech_pvr,
     show_tech_pvr_cli,
     "show tech-support ({bgp|cef|isis|lacp|mpls|mstp|ospf|page|pim|rip|rsvp}|)",
     CLI_SHOW_STR,
     "Show router technical information",
     "BGP related information",
     "CEF related information",
     "ISIS related information",
     "LACP related information",
     "MPLS related information",
     "MSTP related information",
     "OSPF related information",
     "Pagining the command output",
     "PIM related information",
     "RIP related information",
     "RSVP related information")
{
  return _show_tech_cmd_handler(cli, argc, argv, PAL_FALSE);
}

#ifdef HAVE_VR
CLI (show_tech_vr,
     show_tech_vr_cli,
     "show tech-support-vr ({bgp|isis|ospf|page|pim|rip}|)",
     CLI_SHOW_STR,
     "Show technical information of non privileged",
     "BGP related information",
     "ISIS related information",
     "OSPF related information",
     "Pagining the command output",
     "PIM related information",
     "RIP related information")
{
  return _show_tech_cmd_handler(cli, argc, argv, PAL_TRUE);
}
#endif


void
imish_tech_cli_init()
{
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_PVR_MAX, CLI_FLAG_NO_PAGER,
                   &show_tech_pvr_cli);
#ifdef HAVE_VR
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, CLI_FLAG_NO_PAGER,
                   &show_tech_vr_cli);
#endif
}

#endif /* HAVE_CUSTOM1 */

