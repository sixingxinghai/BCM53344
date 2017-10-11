/* Copyright (C) 2006  All Rights Reserved.  */

#include "pal.h"
#include "lib.h"
#include "thread.h"
#include "log.h"

#include "mplsonm.h"
#ifdef HAVE_MPLS_OAM
#include "bfd_client.h"
#else
#include "nsm_client.h"
#endif /* HAVE_MPLS_OAM */

#ifdef HAVE_HA
#include "fm_api.h"
#endif /* HAVE_HA */

/* Global variable container. */
struct lib_globals *azg;

int
mplsonm_start (int deamon_mode, char *config_file, int vty_port, char *progname)
{
  struct thread thread;
  result_t ret;

  /* Initialize memory.  */
  memory_init (APN_PROTO_MPLSONM);

  /* Allocate a global variable container.  */
  ZG = lib_create (progname);
  if (ZG == NULL)
    return -1;

  ret = lib_start (ZG);
  if (ret < 0)
    {
      lib_stop (ZG);
      return ret;
    }

  ZG->protocol = APN_PROTO_MPLSONM;
  ZG->log = openzlog (ZG, ZG->vr_instance, APN_PROTO_MPLSONM , LOGDEST_MAIN);

#ifdef HAVE_HA
  fm_init(ZG,NULL);
#endif /* HAVE_HA */

/* Initialize ONM  protocol stuff.  */
#ifdef HAVE_MPLS_OAM
  mplsonm_oam_init (ZG);
#else /* HAVE_NSM_MPLS_OAM */
  mplsonm_nsm_init (ZG);
#endif /* HAVE_MPLS_OAM */

#ifdef HAVE_PID
PID_REGISTER (PATH_MPLSONM_PID);
#endif /* HAVE_PID */

  /* Send Message to OAM/NSM for the MPLS OAM Request */
  onm_data.mpls_oam = mpls_oam;
#ifdef HAVE_MPLS_OAM
  bfd_client_send_mpls_onm_request(ZG->bc, &mpls_oam);
#else /* HAVE_NSM_MPLS_OAM */
  nsm_client_send_mpls_onm_request(ZG->nc, &mpls_oam);
#endif /* HAVE_MPLS_OAM */
  mpls_onm_detail ();

  /* Execute each thread. */
  while (thread_fetch (ZG, &thread))
    thread_call (&thread);

  return 0;
}

void
mplsonm_stop (void)
{
  /* Mark lib in shutdown for HA */
  SET_LIB_IN_SHUTDOWN (ZG);

#ifdef HAVE_PID
  PID_REMOVE (PATH_MPLSONM_PID);
#endif /* HAVE_PID */
  mpls_onm_disp_summary();
  /* Stop the system.  */
  lib_stop (ZG);
}

#ifdef HAVE_MPLS_OAM_ITUT
#if 0
int
mplsonm_itut_start (void)
{
  struct thread thread;
  result_t ret;
  char *progname = "mplsoamitut";

  /* Initialize memory.  */
  memory_init (APN_PROTO_ITUTMPLSONM);

  /* Allocate a global variable container.  */
  ZG = lib_create (progname);
  if (ZG == NULL)
    return -1;

  ret = lib_start (ZG);
  if (ret < 0)
  {
    lib_stop (ZG);
    return ret;
  }

  ZG->protocol = APN_PROTO_ITUTMPLSONM;
  ZG->log = openzlog (ZG, ZG->vr_instance, APN_PROTO_ITUTMPLSONM , LOGDEST_DEFAULT);

  /* initialize ITUT OAM protocol data */
  mplsonm_nsm_itut_init(ZG);

  #ifdef HAVE_HA
    fm_init(ZG,NULL);
  #endif /* HAVE_HA */


  #ifdef HAVE_PID
    PID_REGISTER (PATH_MPLSONMITUT_PID);
  #endif /* HAVE_PID */
  /* Execute each thread. */
  while (thread_fetch (ZG, &thread))
   thread_call (&thread);

  return 0;
}

void
mplsonm_itut_stop (void)
{
  /* Mark lib in shutdown for HA */
  SET_LIB_IN_SHUTDOWN (ZG);

 #ifdef HAVE_PID
   PID_REMOVE (PATH_MPLSONM_PID);
 #endif /* HAVE_PID */

  /* Stop the system.  */
  lib_stop (ZG);
}
#endif
#endif /* HAVE_MPLS_OAM_ITUT */


