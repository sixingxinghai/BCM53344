/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "hal_debug.h"

/* For debug statement. */
u_int32_t term_hal_debug_event;
u_int32_t term_hal_debug_packet;
u_int32_t term_hal_debug_kernel;

u_int32_t conf_hal_debug_event;
u_int32_t conf_hal_debug_packet;
u_int32_t conf_hal_debug_kernel;

void
hal_debug_all_on (struct cli *cli)
{
  if (cli->mode == CONFIG_MODE)
    {
      CONF_DEBUG_ON (event, EVENT);
    }
  TERM_DEBUG_ON (event, EVENT);
}

void
hal_debug_all_off (struct cli *cli)
{
  if (cli->mode == CONFIG_MODE)
    {
      CONF_DEBUG_OFF (event, EVENT);
    }
  TERM_DEBUG_OFF (event, EVENT);
}

CLI (debug_hal,
     debug_hal_cmd,
     "debug nsm hal (all|)",
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "Hardware Abstraction Layer",
     "Enable all debugging")
{
  hal_debug_all_on (cli);
 if (cli->mode != CONFIG_MODE)
    cli_out (cli, "All possible debugging has been turned on\n");

  return CLI_SUCCESS;
}

CLI (no_debug_hal,
     no_debug_hal_cmd,
     "no debug nsm hal (all|)",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "Hardware Abstraction Layer",
     "Disable all debugging")
{
  hal_debug_all_off (cli);
  if (cli->mode != CONFIG_MODE)
    cli_out (cli, "All possible debugging has been turned off\n");

  return CLI_SUCCESS;
}

ALI (no_debug_hal,
     undebug_hal_cmd,
     "undebug hal (all|)",
     CLI_UNDEBUG_STR,
     CLI_NSM_STR,
     "Disable all debugging");

CLI (debug_hal_events,
     debug_hal_events_cmd,
     "debug nsm hal events",
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "Hardware Abstraction Layer",
     "NSM events")
{
  DEBUG_ON (cli, event, EVENT, "NSM HAL event");

  return CLI_SUCCESS;
}

CLI (no_debug_hal_events,
     no_debug_hal_events_cmd,
     "no debug nsm hal events",
     CLI_NO_STR,
     CLI_DEBUG_STR,
     CLI_NSM_STR,
     "Hardware Abstraction Layer",
     "NSM events")
{
  DEBUG_OFF (cli, event, EVENT, "NSM HAL events");

  return CLI_SUCCESS;
}

ALI (no_debug_hal_events,
     undebug_hal_events_cmd,
     "undebug nsm hal events",
     CLI_UNDEBUG_STR,
     CLI_NSM_STR,
     "Hardware Abstraction Layer",
     "NSM events");

void
hal_debug_init (void)
{
  /* Initialize debug variables. */
  term_hal_debug_event = 0;
  term_hal_debug_packet = 0;
  term_hal_debug_kernel = 0;

  conf_hal_debug_event = 0;
  conf_hal_debug_packet = 0;
  conf_hal_debug_kernel = 0;
}

void
hal_debug_cli_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  /* "debug hal" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &debug_hal_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &no_debug_hal_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &undebug_hal_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &debug_hal_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_debug_hal_cmd);

  /* "debug hal events" commands. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &debug_hal_events_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &no_debug_hal_events_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &undebug_hal_events_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &debug_hal_events_cmd);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_MAX, 0,
                   &no_debug_hal_events_cmd);
}

