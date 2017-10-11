#include "pal.h"

#ifdef HAVE_ONMD
#include "cli.h"
#include "nsmd.h"
#include "nsm_message.h"
#include "nsm_oam.h"
#include "nsm_l2_oam_cli.h"

CLI (nsm_efm_oam_local_event,
     nsm_efm_oam_local_event_cmd,
     "ethernet oam local-event (link-fault|critical-event|dying-gasp)",
     NSM_L2_OAM_ETHER_STR,
     NSM_L2_OAM_ETHER_OAM_STR,
     NSM_L2_OAM_LOCAL_EVENT_STR,
     NSM_L2_OAM_LINK_FAULT_STR,
     NSM_L2_OAM_CRITICAL_LINK_STR,
     NSM_L2_OAM_DYING_GASP_STR)
{
  s_int32_t ret;
  struct interface *ifp = cli->index;

  ret = nsm_efm_oam_process_local_event (ifp,
                        nsm_efm_oam_str_to_event (argv [0]), PAL_TRUE);
  
  return nsm_efm_oam_process_return (ifp, cli, ret);

}

CLI (no_nsm_efm_oam_local_event,
     no_nsm_efm_oam_local_event_cmd,
     "no ethernet oam local-event (link-fault|critical-event|dying-gasp)",
     CLI_NO_STR,
     NSM_L2_OAM_ETHER_STR,
     NSM_L2_OAM_ETHER_OAM_STR,
     NSM_L2_OAM_LOCAL_EVENT_STR,
     NSM_L2_OAM_LINK_FAULT_STR,
     NSM_L2_OAM_CRITICAL_LINK_STR,
     NSM_L2_OAM_DYING_GASP_STR)
{
  s_int32_t ret;
  struct interface *ifp = cli->index;

  ret = nsm_efm_oam_process_local_event (ifp,
                    nsm_efm_oam_str_to_event (argv [0]), PAL_FALSE);

  return nsm_efm_oam_process_return (ifp, cli, ret);

}

CLI (nsm_efm_oam_period_errors,
     nsm_efm_oam_period_errors_cmd,
     "ethernet oam link-monitor "
     "{symbol-period-errors <0-65535>"
     "|frame-period-errors <0-65535>}",
     NSM_L2_OAM_ETHER_STR,
     NSM_L2_OAM_ETHER_OAM_STR,
     NSM_L2_OAM_LINK_MONITOR_STR,
     NSM_L2_OAM_SYMBOL_PERIOD_ERR_STR,
     NSM_L2_OAM_ERR_VAL_STR,
     NSM_L2_OAM_FRAME_PERIOD_ERR_STR,
     NSM_L2_OAM_ERR_VAL_STR)
{
  s_int32_t i;
  s_int32_t ret;
  s_int32_t no_errors;
  enum nsm_efm_opcode opcode;
  struct interface *ifp = cli->index;

  ret = NSM_L2_OAM_ERR_NONE;

  for (i = 0; i < argc; i++)
     {
       opcode = nsm_efm_oam_str_to_opcode (argv [i]);

       CLI_GET_INTEGER_RANGE ("Number of Period Errors", no_errors,
                              argv [++i],
                              NSM_L2_OAM_ERROR_VAL_MIN,
                              NSM_L2_OAM_ERROR_VAL_MAX);

       ret = nsm_efm_oam_process_period_error_event (ifp, opcode, no_errors);
    }

  return nsm_efm_oam_process_return (ifp, cli, ret);

}

CLI (nsm_efm_oam_frame_errors,
     nsm_efm_oam_frame_errors_cmd,
     "ethernet oam link-monitor "
     "{frame-seconds-errors <1-900>"
     "|frame-errors <0-65535>}",
     NSM_L2_OAM_ETHER_STR,
     NSM_L2_OAM_ETHER_OAM_STR,
     NSM_L2_OAM_LINK_MONITOR_STR,
     NSM_L2_OAM_FRAME_SECONDS_ERR_STR,
     NSM_L2_OAM_FRAME_SECS_ERR_VAL_STR,
     NSM_L2_OAM_FRAME_ERR_STR,
     NSM_L2_OAM_ERR_VAL_STR)
{
  s_int32_t i;
  s_int32_t ret;
  s_int32_t no_errors;
  enum nsm_efm_opcode opcode;
  struct interface *ifp = cli->index;

  ret = NSM_L2_OAM_ERR_NONE;

  for (i = 0; i < argc; i++)
     {
       opcode = nsm_efm_oam_str_to_opcode (argv [i]);

       switch (opcode)
         {
           case NSM_EFM_SET_FRAME_EVENT_ERROR:
             CLI_GET_INTEGER_RANGE ("Number of Frame Errors", no_errors,
                                    argv [++i],
                                    NSM_L2_OAM_ERROR_VAL_MIN,
                                    NSM_L2_OAM_ERROR_VAL_MAX);

             ret = nsm_efm_oam_set_frame_errors (ifp, no_errors);

             break;
          case NSM_EFM_SET_FRAME_SECONDS_ERROR:
            CLI_GET_INTEGER_RANGE ("Number of Frame Second Errors", no_errors,
                                   argv [++i],
                                   NSM_L2_OAM_FRAME_SECS_ERR_VAL_MIN,
                                   NSM_L2_OAM_FRAME_SECS_ERR_VAL_MAX);

            ret = nsm_efm_oam_set_frame_seconds_errors (ifp, no_errors);

            break;
          default:
            break;
        }
    }

  return nsm_efm_oam_process_return (ifp, cli, ret);

}

void
nsm_efm_oam_cli_init (struct lib_globals *zg)
{
  struct cli_tree *ctree;

  ctree = zg->ctree;

#ifdef HAVE_EFM
  cli_install (ctree, INTERFACE_MODE, &nsm_efm_oam_local_event_cmd);
  cli_install (ctree, INTERFACE_MODE, &no_nsm_efm_oam_local_event_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_efm_oam_period_errors_cmd);
  cli_install (ctree, INTERFACE_MODE, &nsm_efm_oam_frame_errors_cmd);
#endif /* HAVE_EFM */
}

#endif /* HAVE_ONMD */
