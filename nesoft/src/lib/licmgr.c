/* Copyright (C) 2002-2003  All Rights Reserved. */

#include <pal.h>
#include "thread.h"
#include <lib.h>
#include "log.h"
#ifdef HAVE_LICENSE_MGR
#include <nsmd.h>
#include "licmgr.h"

int 
lic_mgr_get(char **feature, int try, char *version, struct lib_globals *zptr)
{
  int i;
  unsigned int retval;

  /***** Initializing *****/
  retval = LML_INITIALIZE();
  if (retval != LS_SUCCESS)
    return retval;

  /***** Get a license key *****/
  for (i=0; i<try; i++)
    {
      retval = LML_REQUEST(feature[i], version, &(zptr->handle));
      if (retval == LS_SUCCESS)
        break;
    }

  if (LS_SUCCESS != retval) 
    return retval;

  /***** Set up the timer thread to check the license expiration */
  if(need_timer == TIMER_NEEDED && zptr->master != NULL)
  {       
    if (zptr->protocol != APN_PROTO_NSM)
      zptr->t_check_expiration = thread_add_timer (zptr, lic_mgr_update, 
                             (void *) zptr, LIC_MGR_CHECK_INTERVAL); 
  }  
  return LS_SUCCESS;
}


int 
nsm_lic_mgr_get(char **feature, int try, char *version, struct nsm_client_lic *arg)
{
  int i;
  unsigned int retval;
  struct lib_globals *zptr; 

  zptr = arg->zg;

  /***** Initializing *****/
  retval = LML_INITIALIZE();
  if (retval != LS_SUCCESS)
    return retval;

  /***** Get a license key *****/
  for (i=0; i<try; i++)
    {
      retval = LML_REQUEST(feature[i], version, &(arg->handle));
      if (retval == LS_SUCCESS)
        break;
    }

  if (LS_SUCCESS != retval) 
    return retval;

  /***** Set up the timer thread to check the license expiration */
  if(need_timer == TIMER_NEEDED && zptr->master != NULL)
  {       
    if (zptr->protocol != APN_PROTO_NSM)
      arg->t_check_expiration = thread_add_timer (zptr, nsm_lic_mgr_update, 
                             (void *) arg, LIC_MGR_CHECK_INTERVAL); 
  }  
  return LS_SUCCESS;
}

int 
lic_mgr_update(struct thread *t) 
{
  struct lib_globals *zptr; 

  /***** Get handle argument ***/
  zptr = THREAD_ARG(t);

  /***** Renew license key (license heartbeat) *****/
  if (LS_SUCCESS != LML_UPDATE(zptr->handle)) 
    {
      pal_system_err ("%s:%s", modname_strs(zptr->protocol), ERR_RENLIC);
      exit(0);
    }

  /***** Reset up the timer thread to check the license expiration */
  zptr->t_check_expiration = thread_add_timer (zptr, lic_mgr_update, 
                             (void *) zptr, LIC_MGR_CHECK_INTERVAL); 

  return 0;
}

int 
nsm_lic_mgr_update(struct thread *t) 
{
  struct lib_globals *zptr; 
  struct nsm_client_lic *arg;

  /***** Get handle argument ***/
  arg = THREAD_ARG(t);
  zptr = arg->zg;

  /***** Renew license key (license heartbeat) *****/
  if (LS_SUCCESS != LML_UPDATE(arg->handle)) 
    {
      pal_system_err ("%s:%s", modname_strs(zptr->protocol), ERR_RENLIC);
      exit(0);
    }

  /***** Reset up the timer thread to check the license expiration */
  arg->t_check_expiration = thread_add_timer (zptr, nsm_lic_mgr_update, 
                             (void *) arg, LIC_MGR_CHECK_INTERVAL); 

  return 0;
}

int
lic_feature_get (char *feature, char *version, struct lib_globals *zptr)
{
  int ret;
  ret = LML_FEATURE_CHECK(feature, version, &(zptr->handle));
  if ( ret == LS_SUCCESS)
    return 1;
  else
    return 0;
}

void 
lic_mgr_finish (lic_mgr_handle_t handle)
{
  return;
}

#endif /* HAVE_LICENSE_MGR */
