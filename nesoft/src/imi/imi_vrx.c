/* Copyright (C) 2001-2003  All Rights Reserved. */
                                                                                
#include "pal.h"
#include "lib.h"

#ifdef HAVE_VRX

#include "line.h"
#include "host.h"
#include "cli.h"
#include "snprintf.h"
#include "nsm_client.h"

#include "imi/imi.h"
#include "imi/imi_config.h"
#include "imi/imi_pm.h"


/* Read vrf update message from NSM. */
int
imi_vrx_read_vrf_update (u_int16_t msg_type, u_int32_t msg_id,
                         void *arg, void *message)
{
  struct nsm_client *nc;
  struct nsm_client_handler *nch;
  struct nsm_msg_vrf *msg;
  struct host *host;

  nch = arg;
  nc = nch->nc;
  msg = message;

  if (nc->debug)
    nsm_vrf_dump (nc->zg, msg, 0);

  /* New VR? */
  host = vr_lookup_by_vrid (imim, msg->vrf_id);
  if (! host)
    host = vr_new_by_id (imim, msg->vrf_id);

  return 0;
}

void
imi_vrx_nsm_init (struct nsm_client *nc)
{
  /* Register VRF update hook for IMI. */
  nsm_client_set_callback (nc, NSM_MSG_VRF_UPDATE, imi_vrx_read_vrf_update);
}
#endif /* HAVE_VRX */
