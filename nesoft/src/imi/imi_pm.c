/* Copyright (C) 2003  All Rights Reserved. */

#include "pal.h"

#include "line.h"

#include "imi/imi.h"
#include "imi/imi_server.h"


#ifdef HAVE_CRX
/* Set pid for a module.  */
int
imi_pm_set_pid (module_id_t module_id, int pid)
{
  struct imi_server_entry *ise;
  struct imi_server_client *isc;

  /* Lookup module.  */
  isc = imi_server_client_lookup (imim->imh, module_id);
  if (isc == NULL || (ise = isc->head) == NULL)
    return -1;

  /* XXX Set PID.  */
  ise->pid = pid;

  return 0;
}

/* Protocol module start.  */
int
imi_pm_start (module_id_t module_id)
{
  /* Start the protocol module.  */
  if (imi_server_client_lookup (imim->imh, module_id) == NULL)
    pal_imi_start_pm (protocol);

  return 0;
}

/* Stop the protocol daemon.  */
int
imi_pm_stop (module_id_t protocol)
{
  /* Stop the protocol daemon. */
  if (imi_server_client_lookup (imim->imh, module_id))
    pal_imi_stop_pm (protocol);

  return 0;
}
#endif /* HAVE_CRX */
