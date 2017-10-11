
/* Copyright (C) 2009  All Rights Reserved.  */

#include "pal.h"
#include "lib.h"
#include "lib/linklist.h"
#include "stream.h"
#include "thread.h"
#include "elmid.h"
#include "elmi_types.h"
#include "elmi_sock.h"

s_int32_t
elmi_master_init (struct apn_vr *vr)
{
  struct elmi_master *em = NULL;

  em = XCALLOC (MTYPE_ELMI_MASTER, sizeof (struct elmi_master));
  em->zg = ZG;
  em->vr = vr;
  vr->proto = em;

  em->if_list = list_new();
  em->sockfd = elmi_sock_init (ZG); 

  em->bridge_list = list_new();

   /* Initialize ELMI debug flags.  */
  elmi_debug_init (em);

#ifndef HAVE_USER_HSL
  em->elmi_rcv_thread = thread_add_read_high (ZG, elmi_frame_recv, NULL,
      em->sockfd);
#endif /* HAVE_USER_HSL */

  return ELMI_SUCCESS;

}

struct elmi_master *
elmi_master_get (void)
{
  struct elmi_master *elmim = NULL;
  struct apn_vr *vr = NULL;

  ELMI_FN_ENTER ();

  vr = apn_vr_get_privileged (ZG);
  elmim = vr->proto;

  ELMI_FN_EXIT (elmim);
}

int
elmi_master_finish (struct apn_vr *vr)
{
  struct elmi_master *em = vr->proto;

  if (em == NULL)
    return 0;

  vr->proto = NULL;

 return ELMI_SUCCESS; 
}

void
elmi_terminate (struct lib_globals *zg)
{
  struct apn_vr *vr = NULL;
  s_int32_t i = 0;

  for (i = 0; i < vector_max (zg->vr_vec); i++)
    if ((vr = vector_slot (zg->vr_vec, i)))
      elmi_master_finish (vr);

}

void
elmi_init (struct lib_globals *zg)
{
  /* Initialize NSM IPC.  */
  elmi_nsm_init (zg);

  /* Initialize ONM IPC.  */
  elmi_onm_init (zg);

  /* Register VR callbacks.  */
  apn_vr_add_callback (zg, VR_CALLBACK_ADD, elmi_master_init);
  apn_vr_add_callback (zg, VR_CALLBACK_DELETE, elmi_master_finish);

}

