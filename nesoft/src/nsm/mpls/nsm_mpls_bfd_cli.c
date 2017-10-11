/* Copyright (C) 2009-10  All Rights Reserved. */

/**@file  - nsm_mpls_bfd_cli.c
 *
 * @brief - This file contains the CLIs to configure/Unconfiugre 
 * BFD for MPLS LSPs
 */
#include "pal.h"
#include "lib.h"
#include "cli.h"
#include "nsmd.h"
#include "rib.h"
#ifdef HAVE_DIFFSERV
#include "diffserv.h"
#endif /* HAVE_DIFFSERV */
#include "mpls_common.h"

#include "nsm_mpls_bfd_cli.h"

#include "nsm_api.h"
#include "nsm_mpls.h"
#include "nsm_mpls_bfd.h"
#include "nsm_mpls_bfd_api.h"
#ifdef HAVE_MPLS_VC
#include "nsm_mpls_vc.h"
#endif /* HAVE_MPLS_VC */

#if defined (HAVE_MPLS_OAM) && defined (HAVE_BFD)

CLI (nsm_mpls_bfd,
     nsm_mpls_bfd_cmd,
     "mpls bfd ((ldp A.B.C.D/M)|(rsvp tunnel-name NAME)|"
     " (static A.B.C.D/M)) (disable|{lsp-ping-intvl <1-4294967>|"
     "min-tx <50-4294967>|min-rx <50-4294967>|multiplier <1-255>|"
     "force-explicit-null}|)",
     CONFIG_MPLS_STR,
     CLI_BFD_CMD_STR,
     "LDP type LSP",
     "LDP FEC",
     "RSVP type LSP",
     "RSVP Tunnel",
     "Tunnel Name",
     "Static type LSP",
     "Static FEC",
     "Disable BFD for the entry",
     "Set LSP Ping Interval; Default : 5s",
     "seconds",
     "Set BFD Min Tx interval; Default : 50ms",
     "milliseconds",
     "Set BFD Min Rx interval; Default : 50ms",
     "milliseconds",
     "Set BFD Detection Multiplier; Default : 5",
     "multiplier value",
     "Force Explicit NULL")
{
  u_int32_t lsp_ping_intvl = BFD_MPLS_LSP_PING_INTERVAL_DEF;
  u_int32_t min_tx = BFD_MPLS_MIN_TX_INTERVAL_DEF;
  u_int32_t min_rx = BFD_MPLS_MIN_RX_INTERVAL_DEF;
  u_int32_t mult = BFD_MPLS_DETECT_MULT_DEF;
  bool_t force_explicit_null = PAL_FALSE;
  int ret = NSM_SUCCESS;
  char *input = NULL;
  u_int16_t i = 0;
  u_char attr = 0;

  /* argv[0] has the LSP-type. */

  /* Incase of RSVP type LSP, for BFD session for a specific trunk, 
   * argv[2] has the tunnel-name and BFD attributes start from argv[3] &
   * for other LSP-Types argv[1] has the FEC-name and BFD attributes 
   * start from argv[2].
   */
  if (pal_strncmp (argv [0], "r", 1) == 0)
    {
      input = argv [2];
      attr = 3;
    }
  else
    {
      input = argv [1];
      attr = 2;
    }

  /* Get the attributes. */
  for (i = attr; i < argc; i++)
    {
      if (pal_strncmp (argv [i] , "l", 1) == 0)
        {
          i++;
          CLI_GET_UINT32 ("LSP Ping Interval", lsp_ping_intvl,argv [i]);
        }
      else if (pal_strcmp (argv [i] , "min-tx") == 0)
        {
          i++;
          CLI_GET_UINT32 ("BFD Min Tx Interval", min_tx, argv [i]);
        }
      else if (pal_strcmp (argv [i] , "min-rx") == 0)
        {
          i++;
          CLI_GET_UINT32 ("BFD Min Rx Interval", min_rx, argv [i]);
        }
      else if (pal_strncmp (argv [i] , "multiplier", 2) == 0)
        {
          i++;
          CLI_GET_UINT32 ("BFD Detection Multiplier", mult, argv [i]);
        }
      else if (pal_strncmp (argv [i] , "force-explicit-null", 1) == 0)
        {
          force_explicit_null = PAL_TRUE;
        }
    }

  if ((argc > attr) && (pal_strncmp (argv [attr], "d", 1) == 0))
    {
      /* Disable BFD set. */
      ret = nsm_mpls_bfd_api_fec_disable_set (cli->vr->id, argv[0], input);
    }
  else
    {
      ret = nsm_mpls_bfd_api_fec_set (cli->vr->id, argv[0], input, 
                                      lsp_ping_intvl, min_tx, min_rx,
                                      mult, force_explicit_null);
    }

  return nsm_cli_return (cli, ret);
}

CLI (nsm_mpls_bfd_all,
     nsm_mpls_bfd_all_cmd,
     "mpls bfd (ldp|rsvp|static) all ({lsp-ping-intvl <1-4294967>|"
     "min-tx <50-4294967>|min-rx <50-4294967>|multiplier <1-255>|"
     "force-explicit-null}|)",
     CONFIG_MPLS_STR,
     CLI_BFD_CMD_STR,
     "LDP type LSP",
     "RSVP type LSP",
     "Static type LSP",
     "All LSPs",
     "Set LSP Ping Interval; Default : 5s",
     "seconds",
     "Set BFD Min Tx interval; Default : 50ms",
     "milliseconds",
     "Set BFD Min Rx interval; Default : 50ms",
     "milliseconds",
     "Set BFD Detection Multiplier; Default : 5",
     "multiplier value",
     "Force Explicit NULL")
{
  u_int32_t lsp_ping_intvl = BFD_MPLS_LSP_PING_INTERVAL_DEF;
  u_int32_t min_tx = BFD_MPLS_MIN_TX_INTERVAL_DEF;
  u_int32_t min_rx = BFD_MPLS_MIN_RX_INTERVAL_DEF;
  u_int32_t mult = BFD_MPLS_DETECT_MULT_DEF;
  bool_t force_explicit_null = PAL_FALSE;
  int ret = NSM_SUCCESS;
  u_int16_t i = 0;

  /* argv[0] has the LSP-type. Attributes start from argv [1]. */

  /* Get the attributes. */
  for (i = 1; i < argc; i++)
    {
      if (pal_strncmp (argv [i] , "l", 1) == 0)
        {
          i++;
          CLI_GET_UINT32 ("LSP Ping Interval", lsp_ping_intvl,argv [i]);
        }
      else if (pal_strcmp (argv [i] , "min-tx") == 0)
        {
          i++;
          CLI_GET_UINT32 ("BFD Min Tx Interval", min_tx, argv [i]);
        }
      else if (pal_strcmp (argv [i] , "min-rx") == 0)
        {
          i++;
          CLI_GET_UINT32 ("BFD Min Rx Interval", min_rx, argv [i]);
        }
      else if (pal_strncmp (argv [i] , "multiplier", 2) == 0)
        {
          i++;
          CLI_GET_UINT32 ("BFD Detection Multiplier", mult, argv [i]);
        }
      else if (pal_strncmp (argv [i] , "force-explicit-null", 1) == 0)
        {
          force_explicit_null = PAL_TRUE;
        }
    }

  ret = nsm_mpls_bfd_api_lsp_all_set (cli->vr->id, argv[0], lsp_ping_intvl, 
                                      min_tx, min_rx, mult, 
                                      force_explicit_null);

  return nsm_cli_return (cli, ret);
}

CLI (no_nsm_mpls_bfd,
     no_nsm_mpls_bfd_cmd,
     "no mpls bfd ((ldp A.B.C.D/M)|(rsvp tunnel-name NAME)|"
     " (static A.B.C.D/M)) (disable|)",
     CLI_NO_STR,
     CONFIG_MPLS_STR,
     CLI_BFD_CMD_STR,
     "LDP type LSP",
     "LDP FEC",
     "RSVP type LSP",
     "RSVP Tunnel",
     "Tunnel Name",
     "Static type LSP",
     "Static FEC",
     "Disable BFD for the entry")
{
  char *input = NULL;
  int ret = NSM_SUCCESS;

  /* argv[0] has the LSP-type. */
  if (pal_strncmp (argv [0], "r", 1) == 0)
    input = argv [2];
  else
    input = argv [1];

  ret = nsm_mpls_bfd_api_fec_unset (cli->vr->id, argv[0], input); 

  return nsm_cli_return (cli, ret);
}

CLI (no_nsm_mpls_bfd_all,
     no_nsm_mpls_bfd_all_cmd,
     "no mpls bfd (ldp|rsvp|static) all",
     CLI_NO_STR,
     CONFIG_MPLS_STR,
     CLI_BFD_CMD_STR,
     "LDP type LSP",
     "RSVP type LSP",
     "Static type LSP",
     "All LSPs")
{
  char *input;
  int ret;

  /* Initializing. */
  ret = NSM_SUCCESS;
  input = NULL;

  /* argv[0] has the LSP-type. */

  ret = nsm_mpls_bfd_api_lsp_all_unset (cli->vr->id, argv[0]);

  return nsm_cli_return (cli, ret);
}

ALI (no_nsm_mpls_bfd,
     no_nsm_mpls_bfd_ext_cmd,
     "no mpls bfd ((ldp A.B.C.D/M)|(rsvp tunnel-name NAME)|"
     " (static A.B.C.D/M)) ({lsp-ping-intvl <1-4294967>|"
     "min-tx <50-4294967>|min-rx <50-4294967>|multiplier <1-255>|"
     "force-explicit-null}|)",
     CLI_NO_STR,
     CONFIG_MPLS_STR,
     CLI_BFD_CMD_STR,
     "LDP type LSP",
     "LDP FEC",
     "RSVP type LSP",
     "RSVP Tunnel",
     "Tunnel Name",
     "Static type LSP",
     "Static FEC",
     "Set LSP Ping Interval; Default : 5s",
     "seconds",
     "Set BFD Min Tx interval; Default : 50ms",
     "milliseconds",
     "Set BFD Min Rx interval; Default : 50ms",
     "milliseconds",
     "Set BFD Detection Multiplier; Default : 5",
     "multiplier value",
     "Force Explicit NULL");

ALI (no_nsm_mpls_bfd_all,
     no_nsm_mpls_bfd_all_ext_cmd,
     "no mpls bfd (ldp|rsvp|static) all ({lsp-ping-intvl <1-4294967>|"
     "min-tx <50-4294967>|min-rx <50-4294967>|multiplier <1-255>|"
     "force-explicit-null}|)",
     CLI_NO_STR,
     CONFIG_MPLS_STR,
     CLI_BFD_CMD_STR,
     "LDP type LSP",
     "RSVP type LSP",
     "Static type LSP",
     "All LSPs",
     "Set LSP Ping Interval; Default : 5s",
     "seconds",
     "Set BFD Min Tx interval; Default : 50ms",
     "milliseconds",
     "Set BFD Min Rx interval; Default : 50ms",
     "milliseconds",
     "Set BFD Detection Multiplier; Default : 5",
     "multiplier value",
     "Force Explicit NULL");

#ifdef HAVE_VCCV
CLI (mpls_bfd_vccv_trigger,
     mpls_bfd_vccv_trigger_cmd,
     "mpls bfd-vccv (start|stop) <1-1000000>",
     "mpls oam command",
     "bfd vccv",
     "start bfd-vccv",
     "stop bfd-vccv",
     "Virtual Circuit Identifier")
{
  struct nsm_master *nm = cli->vr->proto;
  u_int32_t vc_id;
  int ret, op;

  CLI_GET_UINT32 ("vc-id", vc_id, argv[1]);

  if (vc_id < NSM_MPLS_L2_VC_MIN ||
      vc_id > NSM_MPLS_L2_VC_MAX)
    {
      cli_out (cli, "%% Invalid Virtual Circuit Identifier\n");
      return CLI_ERROR;
    }  

  if (pal_strncmp (argv[0], "start", 5) == 0)
    op = BFD_VCCV_OP_START;
  else
    op = BFD_VCCV_OP_STOP;

  ret =  nsm_mpls_bfd_api_vccv_trigger (nm, vc_id, op);

  if (ret == NSM_SUCCESS)
    return CLI_SUCCESS;

  switch (ret)
    {
    case NSM_ERR_VC_ID_NOT_FOUND:
      cli_out (cli, "%% Invalid Virtual Circuit Identifier\n");
      break;
    case NSM_MPLS_BFD_VCCV_ERR_NOT_CONFIGURED:
      cli_out (cli, "%% bfd vccv is not configured for this VC.\n");
      break;
    case NSM_MPLS_BFD_VCCV_ERR_SESS_NOT_EXISTS:
      cli_out (cli, "%% bfd vccv session is not running.\n");
      break;
    case NSM_MPLS_BFD_VCCV_ERR_SESS_EXISTS:
      cli_out (cli, "%% bfd vccv session is already running.\n");
      break;
    default:
      cli_out (cli, "%% Failed to perform %s operation on VC %s\n",argv[0], argv[1]);
      break;
    };

  return CLI_ERROR;
}

CLI (mpls_vccv_statistics,
     mpls_vccv_statistics_cmd,
     "show vccv statistics",
     CLI_SHOW_STR,
     "Show mpls vccv statistics")
{
  struct nsm_master *nm = cli->vr->proto;

  if (NSM_MPLS)
    {
      cli_out (cli, "%s %d", "CC Mismatch Discards :\t", 
                    NSM_MPLS->vccv_stats.cc_mismatch_discards);
    }

  return CLI_SUCCESS;
}
#endif /* HAVE_VCCV */

/* Install the NSM MPLS BFD commands. */
/**@brief - This function Installs the NSM MPLS BFD commands.

 *  @param *ctree - CLI Tree Structure.

 *  @return NONE
 */
void
nsm_mpls_bfd_cli_init (struct cli_tree *ctree)
{
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_bfd_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &nsm_mpls_bfd_all_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_nsm_mpls_bfd_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_nsm_mpls_bfd_all_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_nsm_mpls_bfd_ext_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_nsm_mpls_bfd_all_ext_cmd);

#ifdef HAVE_VCCV
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &mpls_bfd_vccv_trigger_cmd);

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &mpls_vccv_statistics_cmd); 
#endif /* HAVE_VCCV */
}

#endif /* HAVE_MPLS_OAM && HAVE_BFD */
