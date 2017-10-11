/**@file elmi_main.c
   @brief  This file contains the init part for ELMI module.
*/

/* Copyright (C) 2010  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "thread.h"
#include "log.h"
#include "l2_debug.h"
#include "l2lib.h"
#include "elmi_types.h"
#include "elmi_bridge.h"
#include "elmid.h"
#include "elmi_sock.h"
#include "elmi_cli.h"

/* Global variable container. */
struct lib_globals * elmi_zg;

const char elmi_dest_addr[] =  {0x01, 0x80, 0xc2, 0x00, 0x00, 0x07};

/* Forward declarations. */
void elmip_nsm_init (struct lib_globals * zg);

s_int32_t
elmi_start (s_int32_t deamon_mode, char *config_file,
            s_int32_t vty_port, char *progname)
{
  /* read config info ?? */
  struct thread thread;
  s_int32_t ret = 0;

  /* Initialize memory */
  memory_init (APN_PROTO_ELMI);

  pal_mem_set(&thread, 0, sizeof (struct thread));

  /* Allocate a global variable container */
  ZG = lib_create (progname);
  if (ZG == NULL)
    return RESULT_ERROR;

 ZG->protocol = APN_PROTO_ELMI;
 ZG->vr_instance = 0;
 ZG->log = openzlog (ZG, ZG->vr_instance, APN_PROTO_ELMI,
                         LOGDEST_MAIN);
#ifdef TEST_FM
  pal_signal_set(SIGUSR1, siguser_handler);
#endif

  /* deamon mode */
  if (deamon_mode)
    pal_daemonize (0, 0);

#ifdef HAVE_PID
  PID_REGISTER (PATH_ELMID_PID); 
#endif /* HAVE_PID */

  ret = lib_start (ZG);
  if (ret < 0)
    {
      lib_stop (ZG);
      return ret;
    }

  host_vty_init (ZG);

  /* Initialize ELMI master for privileged VR.  */
  elmi_master_init (apn_vr_get_privileged (ZG));
  
  elmi_init(ZG);
  elmi_cli_init (ZG);

  /* Start the configuration management.  */
  host_config_start (ZG, config_file, vty_port);

  /* Execute each thread. */
  while (thread_fetch (ZG, &thread))
    {
      thread_call (&thread);
    }

  return ELMI_SUCCESS;
}

void
elmi_stop (void)
{
  SET_LIB_IN_SHUTDOWN (ZG);

  /* Terminate ELMI.  */
  elmi_terminate (ZG);

#ifdef HAVE_PID
  PID_REMOVE (PATH_ELMID_PID);
#endif /* HAVE_PID */

  /* Stop the system. */
  lib_stop (elmi_zg);
  ZG = NULL;
}
