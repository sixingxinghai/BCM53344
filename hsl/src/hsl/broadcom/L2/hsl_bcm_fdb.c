/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"

#include "hal_types.h"
#include "hal_l2.h"
#include "hal_msg.h"

#include "bcm_incl.h"

#include "hsl_oss.h"
#include "hsl_logger.h"
#include "hsl_error.h"
#include "hsl_ifmgr.h"
#include "hsl_bcm_if.h"
#include "hsl_bcm_ifmap.h"
#include "hsl_mac_tbl.h"
#include "hsl_bcm_fdb.h"

/*
  L2 address add/delete to FDB.
*/
void
hsl_bcm_fdb_addr_register (bcmx_l2_addr_t *addr, int insert, void *cookie)
{
  struct hsl_if *ifp;
  fdb_entry_t entry;
  fdb_entry_t eptr;
  fdb_entry_t key;
  int ret;
  int age_timer;
  
  /* Check interface type */
  if (addr->flags & BCM_L2_L3LOOKUP)
    return;

  /* Check multicast */
  if (addr->mac[0] & 0x1)
    return;  

  /* Ageing timer */
  ret = bcmx_l2_age_timer_get (&age_timer);
  if (ret != BCM_E_NONE)
    age_timer = 0;

  entry.ageing_timer_value = age_timer;

  /* Mac address */
  entry.mac_addr[0] = addr->mac[0];
  entry.mac_addr[1] = addr->mac[1];
  entry.mac_addr[2] = addr->mac[2];
  entry.mac_addr[3] = addr->mac[3];
  entry.mac_addr[4] = addr->mac[4];
  entry.mac_addr[5] = addr->mac[5];

  /* Port number */
  if (BCMX_LPORT_VALID (addr->lport))
    {
      ifp = hsl_bcm_ifmap_if_get(addr->lport);
      if (ifp)
      {
        entry.port_no = ifp->ifindex;
      }
      else
        entry.port_no = 0;
    }
  else
    {
      /* Could be learnt on a trunk member */
      ifp = hsl_bcm_ifmap_if_get (HSL_BCM_TRUNK_2_LPORT(addr->tgid));
      if (ifp)
        {
          entry.port_no = ifp->ifindex;
        }
      else
        entry.port_no = 0;
    }

  /* Vlan Id */
  entry.vid = addr->vid;

  /* Is static? check the flag */
  if (addr->flags & BCM_L2_STATIC)
    entry.is_static = 1;
  else
    entry.is_static = 0;

  /* Is local? */
  entry.is_local = 0;

#ifdef HAVE_MAC_AUTH
  /* Is forwarding? */
  if ((addr->flags & BCM_L2_DISCARD_DST)
      && (addr->flags & BCM_L2_DISCARD_SRC))
    entry.is_fwd = 0;
  else
#endif /* HAVE_MAC_AUTH */
    entry.is_fwd = 1;

  /* Snmp status */
  entry.snmp_status = 0;

  /* add & delete the forward entry */
  if (insert)
    {
      /* Add learned entry to FDB */
      ret = hsl_add_fdb_entry (&entry);
      if (ret != 0)
        {
          switch (ret)
            {
              case STATUS_WRONG_PARAMS:
                HSL_LOG (HSL_LOG_FDB, HSL_LEVEL_INFO,"Not Added [WRONG_PARAMETERS]\n");
                break;
              case STATUS_MEM_EXHAUSTED:
                HSL_LOG (HSL_LOG_FDB, HSL_LEVEL_INFO,"Not Added [MEMORY_EXHAUSTED]\n");
                break;
              case STATUS_KEY_NOT_FOUND:
                HSL_LOG (HSL_LOG_FDB, HSL_LEVEL_INFO,"Not Added [KEY_NOT_FOUND]\n");
                break;
              case STATUS_DUPLICATE_KEY:
                HSL_LOG (HSL_LOG_FDB, HSL_LEVEL_INFO,
                         "Not Added [MAC:%x:%x:%x:%x:%x:%x] [DUPLICATE_KEY]\n",
                         entry.mac_addr[0], entry.mac_addr[1],
                         entry.mac_addr[2], entry.mac_addr[3],
                         entry.mac_addr[4], entry.mac_addr[5]);
                break;
            }
        }
      else
        {
          key.mac_addr[0] = addr->mac[0];
          key.mac_addr[1] = addr->mac[1];
          key.mac_addr[2] = addr->mac[2];
          key.mac_addr[3] = addr->mac[3];
          key.mac_addr[4] = addr->mac[4];
          key.mac_addr[5] = addr->mac[5];

          if ((ret = hsl_get_fdb_entry (&eptr, SEARCH_BY_MAC, &key)) == 0)
            HSL_LOG (HSL_LOG_FDB, HSL_LEVEL_INFO,"Added [MAC:%x:%x:%x:%x:%x:%x] to FDB\n",
                 eptr.mac_addr[0], eptr.mac_addr[1], eptr.mac_addr[2],
                 eptr.mac_addr[3], eptr.mac_addr[4], eptr.mac_addr[5]);
          else
            {
              switch (ret)
                {
                  case STATUS_WRONG_PARAMS:
                    HSL_LOG (HSL_LOG_FDB, HSL_LEVEL_INFO,"Get entry [WRONG_PARAMETERS]\n");
                    break;
                  case STATUS_MEM_EXHAUSTED:
                    HSL_LOG (HSL_LOG_FDB, HSL_LEVEL_INFO,"Get entry [MEMORY_EXHAUSTED]\n");
                    break;
                  case STATUS_KEY_NOT_FOUND:
                    HSL_LOG (HSL_LOG_FDB, HSL_LEVEL_INFO,"Get entry [KEY_NOT_FOUND]\n");
                    break;
                  case STATUS_DUPLICATE_KEY:
                    HSL_LOG (HSL_LOG_FDB, HSL_LEVEL_INFO,"Get entry [DUPLICATE_KEY]\n");
                    break;
                }
            }
        }
    }
  else
    {
      /* Delete entry */
      ret = hsl_delete_fdb_entry (&entry);
      if (ret != 0)
        HSL_LOG (HSL_LOG_FDB, HSL_LEVEL_ERROR,
                 "Not Deleted [MAC:%x:%x:%x:%x:%x:%x] from FDB\n",
                 entry.mac_addr[0], entry.mac_addr[1], entry.mac_addr[2],
                 entry.mac_addr[3], entry.mac_addr[4], entry.mac_addr[5]);
      else
        HSL_LOG (HSL_LOG_FDB, HSL_LEVEL_INFO,
                 "Deleted [MAC:%x:%x:%x:%x:%x:%x] from FDB\n",
                 entry.mac_addr[0], entry.mac_addr[1], entry.mac_addr[2],
                 entry.mac_addr[3], entry.mac_addr[4], entry.mac_addr[5]);
    }
}

/*
  Register callbacks.
*/
void
hsl_fdb_hw_cb_register (void)
{
  if (!bcmx_l2_notify_register (hsl_bcm_fdb_addr_register, NULL))
      HSL_LOG (HSL_LOG_FDB, HSL_LEVEL_INFO, "Callback is registered!\n");
  else
      HSL_LOG (HSL_LOG_FDB, HSL_LEVEL_ERROR, "Callback is unregistered!\n");

  return;
}


/*
  Unregister callbacks.
*/
void
hsl_fdb_hw_cb_unregister (void)
{
  if (!bcmx_l2_notify_unregister (hsl_bcm_fdb_addr_register, NULL))
    HSL_LOG (HSL_LOG_FDB, HSL_LEVEL_INFO, "Callback is unregistered!\n");
  else
    HSL_LOG (HSL_LOG_FDB, HSL_LEVEL_ERROR, "ERROR: Callback cannot unregister!\n");

  return;
}


