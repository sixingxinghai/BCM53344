/* Copyright (C) 2004  All Rights Reserved. */


#include "config.h"

#include "hsl_os.h"

#include "hsl_types.h"

/* Change to - have stacking */ 

#ifdef HAVE_L2
#include "hal_types.h"
#include "hal_l2.h"
#endif /* HAVE_L2 */

#include "hal_msg.h"

#include "hsl_avl.h"
#include "hsl_error.h"
#include "bcm_incl.h"
#include <appl/cputrans/next_hop.h>
#include <appl/cpudb/cpudb.h>
#include <bcm_int/rpc/rpc.h>
#include <bcm_int/rpc/rlink.h>
#include <appl/discover/disc.h>
#include <appl/stktask/stktask.h>
#include <appl/stktask/topology.h>
#include <appl/stktask/topo_pkt.h>
#include <appl/stktask/topo_brd.h>
#include <appl/stktask/attach.h>

#include "hsl_types.h"
#include "hsl_oss.h"
#include "hsl_logger.h"
#include "hsl_ifmgr.h"
#include "hsl_if_hw.h"
#include "hsl_bcm.h"
#include "hsl_bcm_if.h"

#include "hsl_bcm_cpu.h"

cpudb_ref_t hsl_db_refs[HSL_MAX_DBS];
int hsl_cur_db = 0;
int hsl_num_db = 0;

static int 
pkt_preinit(void)
{
    int i;
    int rv;

    for (i = 0; i < soc_ndev; i++) {
        rv = bcm_rx_init(i);
        if (rv < 0) {
            printk("ERROR: BCM RX init failed, unit %d: %s\n",
                   i, bcm_errmsg(rv));
            return -1;
        }

        if (!bcm_rx_active(i)) {
            rv = bcm_rx_start(i, NULL);
            if (rv < 0) {
                printk("ERROR: BCM RX start failed, unit %d: %s\n",
                       i, bcm_errmsg(rv));
                return -1;
            }
        }
    }

    if (!cputrans_tx_setup_done()) {

        rv = cputrans_tx_pkt_setup(100, &bcm_trans_ptr);
        if (rv < 0) {
            printk("ERROR: cputrans TX setup failed: %s\n",
                   bcm_errmsg(rv));
            return -1;
        }
    }

    if (!cputrans_rx_setup_done()) {
        rv = cputrans_rx_pkt_setup(-1, NULL);  /* Use defaults */
        if (rv < 0) {
            printk("ERROR: cputrans RX setup failed: %s\n",
                   bcm_errmsg(rv));
            return -1;
        }
    }

    return 0;
}


int
hsl_bcm_cpudb_create ()
{
  int ii;
  
  for (ii = 0; ii < HSL_MAX_DBS; ii++) {
      if (hsl_db_refs[ii] == NULL) {
          break;
      }
  }

  if (ii >= HSL_MAX_DBS) {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Too many DBs\n");
      HSL_FN_EXIT (-1);
  }
  if ((hsl_db_refs[ii] = cpudb_create()) == NULL) {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
           "ERROR: cpudb %d create failed\n", ii);
  } else {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ADMIN, "cpudb %d created\n", ii);
       hsl_num_db++;
  }

  HSL_FN_EXIT (0);
}

static cpudb_key_t current_key;

#define CHECK_SET(dest, val) if ((val) >= 0) (dest) = (val)

/*************************************************************************
*                        CPUDB MODIFICATION
*                            ROUTINES
**************************************************************************/

int
hsl_bcm_set_cpudb_properties (int db_idx, char *key_val, char *mac,
                              int base_dest_port, int flags, 
                              int master_pri, int slot_id, int dest_unit, 
                              int tx_unit, int tx_port, int dest_mod,
                              int dest_port, int topo_idx)
{
  cpudb_ref_t      db_ref;
  cpudb_entry_t    *entry;
  cpudb_base_t     *base;
  cpudb_key_t      key;

  sal_memset(&key, 0, sizeof(cpudb_key_t));
  if (db_idx < 0 || db_idx >= HSL_MAX_DBS) {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
          "Bad db index %d\n", db_idx);
      HSL_FN_EXIT (-1);
  }

  db_ref = hsl_db_refs[db_idx];
  if (!cpudb_valid(db_ref)) {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
           "DB reference %d is not valid\n", db_idx);
       HSL_FN_EXIT (-1);
  }
  
  sal_memcpy(&key.key, key_val, sizeof(cpudb_key_t));
  CPUDB_KEY_SEARCH(db_ref, key, entry);
  if (entry != NULL) {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ADMIN, 
           "CPUDB PARSE ENTRY:  WARNING key already in DB " CPUDB_KEY_FMT_EOLN,
           CPUDB_KEY_DISP(key));
  }

  CPUDB_KEY_COPY(current_key, key);
  entry = cpudb_entry_create(db_ref, key, FALSE);
  if (entry == NULL) {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ADMIN, 
          "CPUDB PARSE ENTRY:  ERROR failed to create entry for "
          CPUDB_KEY_FMT_EOLN, CPUDB_KEY_DISP(key));
       HSL_FN_EXIT (-1);
  }

  base = &entry->base;
  sal_memcpy(entry->base.mac, mac, sizeof(bcm_mac_t));

  CHECK_SET(entry->flags, flags);
  CHECK_SET(base->master_pri, master_pri);
  CHECK_SET(base->slot_id, slot_id);
  CHECK_SET(base->dest_unit, dest_unit);
  CHECK_SET(base->dest_port, base_dest_port);
  CHECK_SET(entry->tx_unit, tx_unit);
  CHECK_SET(entry->tx_port, tx_port);
  CHECK_SET(entry->dest_mod, dest_mod);
  CHECK_SET(entry->dest_port, dest_port);
  CHECK_SET(entry->topo_idx, topo_idx);
  entry->base.num_units = 0;
  entry->base.num_stk_ports = 0;
   
  HSL_FN_EXIT (0);
}



int
hsl_bcm_set_modid (int db_idx, char *key_val, int mod_id, 
                   int pref_mod_id, int mod_ids_req)
{
  cpudb_ref_t      db_ref;
  cpudb_entry_t    *entry;
  cpudb_key_t      key;
  int              cur_unit;

  sal_memset(&key, 0, sizeof(cpudb_key_t));
  if (db_idx < 0 || db_idx >= HSL_MAX_DBS) {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
          "Bad db index %d\n", db_idx);
      HSL_FN_EXIT (-1);
  }

  db_ref = hsl_db_refs[db_idx];
  if (!cpudb_valid(db_ref)) {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
           "DB reference %d is not valid\n", db_idx);
       HSL_FN_EXIT (-1);
  }
  
  sal_memcpy(&key.key, key_val, sizeof(cpudb_key_t));
  CPUDB_KEY_SEARCH(db_ref, key, entry);
  if (entry == NULL) {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
           "CPUDB PARSE UNIT_MOD_IDS:  Current DB key not found\n");
       HSL_FN_EXIT (-1);
  }

  cur_unit = entry->base.num_units++;
  entry->mod_ids[cur_unit] = mod_id;
  entry->base.pref_mod_id[cur_unit] = pref_mod_id;
  entry->base.mod_ids_req[cur_unit] = mod_ids_req;
  
  HSL_FN_EXIT (0);
}

int
hsl_bcm_add_stack_port (int db_idx, int unit, int stk_port, int weight,
                        int base_flags, int flags, int tx_stk_idx, 
                        int rx_stk_idx, char *key_val, char *tx_key, 
                        char *rx_key)
{
  cpudb_ref_t      db_ref;
  cpudb_entry_t    *entry;
  cpudb_key_t      key, rx_cpu_key, tx_cpu_key;
  int              cur_sp;
  cpudb_stk_port_t *sp;

  sal_memset(&key, 0, sizeof(cpudb_key_t));
  sal_memset(&rx_cpu_key, 0, sizeof(cpudb_key_t));
  sal_memset(&tx_cpu_key, 0, sizeof(cpudb_key_t));

  if (db_idx < 0 || db_idx >= HSL_MAX_DBS) {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
          "Bad db index %d\n", db_idx);
      HSL_FN_EXIT (-1);
  }

  db_ref = hsl_db_refs[db_idx];
  if (!cpudb_valid(db_ref)) {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
           "DB reference %d is not valid\n", db_idx);
       HSL_FN_EXIT (-1);
  }
  
  sal_memcpy(&key.key, key_val, sizeof(cpudb_key_t));
  CPUDB_KEY_SEARCH(db_ref, key, entry);
  if (entry == NULL) {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
           "Add stack port:  Current DB key not found\n");
       HSL_FN_EXIT (-1);
  }

  sal_memcpy(&tx_cpu_key.key, tx_key, sizeof(cpudb_key_t));
  sal_memcpy(&rx_cpu_key.key, rx_key, sizeof(cpudb_key_t));
  
  cur_sp = entry->base.num_stk_ports++;
  entry->base.stk_ports[cur_sp].weight = weight;
  entry->base.stk_ports[cur_sp].bflags = base_flags;
  entry->base.stk_ports[cur_sp].unit = unit;
  entry->base.stk_ports[cur_sp].port = stk_port;
  sp = &entry->sp_info[cur_sp];
  sp->flags = flags;
  CPUDB_KEY_COPY(sp->tx_cpu_key, tx_cpu_key);
  sp->tx_stk_idx = tx_stk_idx;
  CPUDB_KEY_COPY(sp->rx_cpu_key, rx_cpu_key);
  sp->rx_stk_idx = rx_stk_idx;
  
  HSL_FN_EXIT (0);
}


int
hsl_bcm_set_master (int db_idx, char *key_val)
{
  cpudb_ref_t      db_ref;
  cpudb_entry_t    *entry;
  cpudb_key_t      key;

  sal_memset(&key, 0, sizeof(cpudb_key_t));
  if (db_idx < 0 || db_idx >= HSL_MAX_DBS) {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
          "Bad db index %d\n", db_idx);
      HSL_FN_EXIT (-1);
  }

  db_ref = hsl_db_refs[db_idx];
  if (!cpudb_valid(db_ref)) {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
           "DB reference %d is not valid\n", db_idx);
       HSL_FN_EXIT (-1);
  }
  
  sal_memcpy(&key.key, key_val, sizeof(cpudb_key_t));
  CPUDB_KEY_SEARCH(db_ref, key, entry);
  if (entry == NULL) {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
           "Set Master:  Current DB key not found\n");
       HSL_FN_EXIT (-1);
  }
  db_ref->master_entry = entry;
  
  HSL_FN_EXIT (0);
}


int
hsl_bcm_set_local_key (int db_idx, char *key_val)
{
  cpudb_ref_t      db_ref;
  cpudb_entry_t    *entry;
  cpudb_key_t      key;

  sal_memset(&key, 0, sizeof(cpudb_key_t));
  if (db_idx < 0 || db_idx >= HSL_MAX_DBS) {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
          "Bad db index %d\n", db_idx);
      HSL_FN_EXIT (-1);
  }

  db_ref = hsl_db_refs[db_idx];
  if (!cpudb_valid(db_ref)) {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
           "DB reference %d is not valid\n", db_idx);
       HSL_FN_EXIT (-1);
  }
  
  sal_memcpy(&key.key, key_val, sizeof(cpudb_key_t));
  CPUDB_KEY_SEARCH(db_ref, key, entry);
  if (entry == NULL) {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
           "Set Local:  Current DB key not found\n");
       HSL_FN_EXIT (-1);
  }
  db_ref->local_entry = entry;
  
  HSL_FN_EXIT (0);
}

int 
hsl_bcm_cpudb_destroy (int db_num)
{
  int ret = -1;

  if (db_num > hsl_num_db) {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "DB %d does not exist\n", db_num);
        HSL_FN_EXIT (ret);
    }

  if (hsl_db_refs[db_num] != NULL) {
      ret = cpudb_destroy (hsl_db_refs[db_num]);
      HSL_LOG(HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, 
          "DB destroy returned %d\n", db_num);
  }
  else {
      HSL_LOG(HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
          "DB %d does not exist\n", db_num);
  }
  HSL_FN_EXIT (ret);
}

int
hsl_bcm_cpudb_set_current (int db_num)
{
  int ret = -1;

  if (db_num > hsl_num_db) {
        HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
            "DB %d does not exist\n", db_num);
        HSL_FN_EXIT (ret);
  }

  if (hsl_db_refs[db_num] != NULL) {
      hsl_cur_db = db_num;

      ret = cpudb_valid (hsl_db_refs[db_num]);
      if (ret) {
          HSL_LOG(HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT, 
              "Current DB set to %d\n", db_num);
      }
      else {
          HSL_LOG(HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT, 
               "Current DB set to %d - Not yet valid \n", db_num);
      }
      ret = 0;
  }
  else {
      HSL_LOG(HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
          "DB %d does not exist\n", db_num);
  }
  HSL_FN_EXIT (ret);
}


int
hsl_bcm_stack_mode_set (int stk_unit, int hg, int sl, int none)
{
   int rv;
   unsigned int flags = 0;

   flags |= (none > 0) ? BCM_STK_NONE : 0;
   flags |= (sl > 0) ? BCM_STK_SL : 0;
   flags |= (hg > 0) ? BCM_STK_HG : 0;

   rv = bcm_stk_mode_set(stk_unit, flags);

   if (rv < 0) {
       HSL_LOG(HSL_LOG_PLATFORM, HSL_LEVEL_ERROR,
           "ERROR: bcm_stk_mode_set returns: %s Stk Unit = %d flags = %d \n", 
           bcm_errmsg(rv), stk_unit, flags);
       rv = -1;
   }
   
   HSL_FN_EXIT (rv);
}

/*************************************************************************
* 
*   Code for NEXTHOP to work
*
*
*************************************************************************/
int
hsl_bcm_print_nexthop_info(cpudb_key_t *key)
{
  int did_print = FALSE, idx;
  int s_unit = 0, s_port = 0, duplex = 0;

  for (idx = 0; idx < next_hop_num_ports_get(); idx++) {
      if (next_hop_port_get(idx, &s_unit, &s_port, &duplex)
                == BCM_E_NONE) {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT,
              "NH idx %d:  Unit %d, Port %d: %sarked duplex.\n",
               idx, s_unit, s_port, duplex ? "M" : "Not m");
      } else {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT,
              "NH idx %d:  INVALID?\n", idx);
      }
          did_print = TRUE;
  }
  for (idx = 0; idx < next_hop_max_entries_get(); idx++) {
      key = (cpudb_key_t *)next_hop_key_get(idx);
      if (key != NULL) {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT,
              "KEY %x:%x:%x:%x:%x:%x is in NH DB\n",
              (*key).key[0], (*key).key[1], (*key).key[2],
              (*key).key[3], (*key).key[4], (*key).key[5]);
              did_print = TRUE;
      }
  }
  if (!did_print) {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT,
          "Next Hop database and ports empty\n");
  }
  HSL_FN_EXIT (0);
}

int
hsl_bcm_start_nexthop (int db_idx, int thrd_pri, int rx_pri,
                       int nhvlan, int nhcos)
{
  int rv;
  cpudb_ref_t       db_ref;

  if (cputrans_trans_count() == 0) {
      cputrans_trans_add(&bcm_trans_ptr);
  }

  if (db_idx < 0 || db_idx >= HSL_MAX_DBS ||
      hsl_db_refs[db_idx] == CPUDB_REF_NULL) {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT,
          "Bad db index: %d\n", db_idx);
      HSL_FN_EXIT (-1);
  }

  if (hsl_db_refs[db_idx]->local_entry == NULL) {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT,
          "No local DB entry in cpudb %d\n", db_idx);
      HSL_FN_EXIT (-1);
  }

  if (nhcos >= 0) {
      next_hop_cos_set(nhcos);
  }

  if (nhvlan >= 0) {
      next_hop_vlan_set(nhvlan);
      /* Load the next hop mac addr into L2 tables */
      rv = nh_tx_dest_install(TRUE, nhvlan);
      if (rv < 0) {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT,
              "NH TX dest install failed: %s\n", bcm_errmsg(rv));
          HSL_FN_EXIT (-1);
      }
  }

  db_ref = hsl_db_refs[db_idx];
  if (pkt_preinit() < 0) {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT, "Pre-init failed\n");
      HSL_FN_EXIT (-1);
  }

  if (next_hop_running()) {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT,
          "Next hop is running; stopping: %d\n",
      next_hop_stop());
  }

  if (thrd_pri >= 0 || rx_pri >= 0) {
      if (next_hop_config_set(NULL, thrd_pri, rx_pri) < 0) {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT, 
              "Warning: config set failed\n");
      }
  }
 
  rv = next_hop_start(&hsl_db_refs[db_idx]->local_entry->base);
  if (rv < 0) {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT,
          "ERROR: next_hop_start failed: %s\n", bcm_errmsg(rv));
  }

  HSL_FN_EXIT (0);
}

int
hsl_bcm_start_atp(int db_idx, int thrd_pri, int rx_pri, int tx_pri,
                  int atp_vlan, int atp_cos)
{
  int rv;

  if (cputrans_trans_count() == 0) {
      cputrans_trans_add(&bcm_trans_ptr);
  }

  if (pkt_preinit() < 0) {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT, "Pre-init failed\n");
      HSL_FN_EXIT (-1);
  }

  if (db_idx < 0 || db_idx >= HSL_MAX_DBS ||
      hsl_db_refs[db_idx] == CPUDB_REF_NULL) {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT, 
           "Bad db index: %d\n", db_idx);
       HSL_FN_EXIT (-1);
  }

  if (hsl_db_refs[db_idx]->local_entry == NULL) {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT, 
           "No local DB entry in cpudb %d\n", db_idx);
       HSL_FN_EXIT (-1);
  }

  atp_cos_vlan_set(atp_cos, atp_vlan);
  rv = atp_config_set(tx_pri, rx_pri, NULL);
  if (rv < 0) {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT, 
           "ERROR: ATP config set failed: %s\n", bcm_errmsg(rv));
  }

  atp_db_update(hsl_db_refs[db_idx]);
  rv = atp_start(ATP_F_LEARN_SLF,
                 ATP_UNITS_ALL,
                 BCM_RCO_F_ALL_COS);
  if (rv < 0) {
       HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT, 
           "ERROR: ATP start failed: %s\n", bcm_errmsg(rv));
  }

  HSL_FN_EXIT (0);
}


int
hsl_bcm_rpc_start ()
{
  int rv;

  rv = bcm_rpc_start();
  if (rv < 0) {
     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, " rpc_start: %s\n",
            bcm_errmsg(rv));
  }

  rv = bcm_rlink_start();
  if (rv < 0) {
     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, " rlink_start: %s\n",
            bcm_errmsg(rv));
  }

  HSL_FN_EXIT (0);
}


int
hsl_bcm_rpc_stop ()
{
  int rv;

  rv = bcm_rpc_stop();
  if (rv < 0) {
     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, " rpc_start: %s\n",
            bcm_errmsg(rv));
  }

  rv = bcm_rlink_start();
  if (rv < 0) {
     HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, " rlink_start: %s\n",
            bcm_errmsg(rv));
  }

  HSL_FN_EXIT (0);
}


static sal_thread_t st_tid = SAL_THREAD_ERROR;
#define TOPO_ATP_FLAGS (ATP_F_NEXT_HOP | ATP_F_REASSEM_BUF)

static void
_stk_port_update(int unit, bcm_port_t port, uint32 flags, void *cookie)
{
    int stk_cos;
    int rv;

    if (flags & BCM_STK_ENABLE) {
        if (flags & BCM_STK_SL) {
            stk_cos = PTR_TO_INT(cookie);
            rv = bcm_switch_control_port_set(unit, port,
                                             bcmSwitchCpuProtocolPrio,
                                             stk_cos);
            if ((rv < 0) && (rv != BCM_E_UNAVAIL)) {
                HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
                       "STACK: Error setting CPU protocol priority"
                       " on unit %d, port %d: %s\n", unit, port,
                       bcm_errmsg(rv));
            }
        }
    } else {
        /* Do any special processing needed for a port which is
         * removed from stacking.
         */
    }
}

static void
tks_st_thread(void *cookie)
{
    bcm_st_config_t *cfg;
    int rv;

    cfg = (bcm_st_config_t *)cookie;

    rv = topo_pkt_handle_init(TOPO_ATP_FLAGS);
    if (rv < 0) {
        printk("WARNING: topo pkt handle init returned %s\n", bcm_errmsg(rv));
    }

    rv = bcm_st_start(cfg, TRUE);
    printk("ST thread exitted with value %d\n", rv);
    st_tid = SAL_THREAD_ERROR;

    printk("ST: Stopping RPC and rlink\n");
    (void)bcm_rlink_stop();
    (void)bcm_rpc_stop();
    sal_thread_exit(rv);
}

static bcm_st_config_t hsl_st_cfg = {
    disc_start,
    disc_abort,
    NULL,              /* Use default master election routine */

    NULL,              /* Use default topology update function */
    NULL,              /* Use default attach update function */
    NULL,              /* Use default transition notify function */

    /* Base is filled in from current db */
    {{{0}}},
};

int 
hsl_bcm_start_stk_task (int db_idx, int disc_vlan, int disc_cos, int auto_trunk, 
                        int cfg_to_us, int timeout_us, int retrx_us, int pri)
{
  int ii, rv = 0;
  bcm_pbmp_t vports, vports_untag;
  cpudb_ref_t db_ref;
  bcm_st_config_t *cfg = &hsl_st_cfg;

  if (st_tid != SAL_THREAD_ERROR) {
    HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_DEFAULT, 
        "Stack Task is already running\n");
    HSL_FN_EXIT (0);
 }

 disc_timeout_set(timeout_us, cfg_to_us, retrx_us);
 if (disc_cos >= 0) {
     disc_cos_set(disc_cos);
     rv = bcm_stk_update_callback_register(0, _stk_port_update,
         INT_TO_PTR(disc_cos));

      if (rv < 0) {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
              "ST: Could not set stack callback: %s\n", bcm_errmsg(rv));
      }
  }

  if (disc_vlan >= 0) {
      disc_vlan_set(disc_vlan);
  }

  if (auto_trunk >= 0) {
      bcm_board_auto_trunk_set(auto_trunk);
  }

  if (!cpudb_valid(hsl_db_refs[db_idx]) ||
     hsl_db_refs[db_idx]->local_entry == NULL) {
     cfg = NULL;
  } else {
      db_ref = hsl_db_refs[db_idx];
      disc_vlan_get(&disc_vlan);
      BCM_PBMP_CLEAR(vports_untag);

      /* Put CPU in VLAN for all devices */
      for (ii = 0; ii < db_ref->local_entry->base.num_units; ii++) {
           rv = bcm_vlan_create(ii, disc_vlan);
           if (rv < 0 && rv != BCM_E_EXISTS) {
               HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
                   "ERROR: vlan %d create failed: %s\n",
                        disc_vlan, bcm_errmsg(rv));
                    HSL_FN_EXIT (-1);
           }
           BCM_PBMP_CLEAR(vports);
           BCM_PBMP_PORT_ADD(vports, CMIC_PORT(ii));
           rv = bcm_vlan_port_add(ii, disc_vlan,
                 vports, vports_untag);
           if (rv < 0) {
               HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
                   "ERROR: vlan %d CPU port add failed: %s\n",
                   disc_vlan, bcm_errmsg(rv));
               HSL_FN_EXIT (-1);
           }
      }

      /*
      * For each local stack port, if SL, set DISABLE_IF_CUT;
      * Set the weight inversely to the speed
      */
      for (ii = 0; ii < db_ref->local_entry->base.num_stk_ports; ii++) {
          int speed;
          cpudb_unit_port_t *sp = &db_ref->local_entry->base.stk_ports[ii];

          if (bcm_port_speed_get(sp->unit, sp->port, &speed) < 0) {
               HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
              "Error getting port speed unit %d, port %d\n", sp->unit, sp->port);
              HSL_FN_EXIT (-1);
          }

          if (speed < 10000) {  /* Assume SL port */
              sp->bflags |= CPUDB_UPF_DISABLE_IF_CUT;
          }
          sp->weight = 1000000/speed;
      }
      sal_memcpy(&hsl_st_cfg.base, &db_ref->local_entry->base, sizeof(cpudb_base_t));
  }

  st_tid = sal_thread_create("bcmSTACK",
                              SAL_THREAD_STKSZ,
                              pri,
                              tks_st_thread,
                              cfg);

  HSL_FN_EXIT (0);
}


static void
hsl_bcm_device_attach_register (int unit, int attached, cpudb_entry_t *cpuent, int cpuinit)
{
  if (attached) 
  {
      /* Attached */
  }
  else
  {
      /* Dispatch */
  }
  
  return;
}

#if 0 /* Not used currently */
static void
hsl_bcm_port_attach_register (bcmx_lport_t lport,
                              int unit,
                              bcm_port_t port,
                              uint32 flags)
{
  if (flags & CPUDB_F_IS_MASTER)
  {
       /* */
      if (flags & CPUDB_F_IS_LOCAL)
      {
           /* Incase of local entry - stack port should be removed from the system if also 
              master */
      }
      else
      {
           /* Non local so add the port to the system */
      }
  }
 
  return;
}
#endif

int 
hsl_bcm_start_stacking (unsigned char *mac, int unit, 
                         unsigned int base_dest_port,
                        hsl_bcm_port_list *port_struct, int master_prio,
                        int slot_id, int dest_unit, unsigned int sl, 
                        unsigned int higig, int pref_mod_id, int mod_ids_req,
                        int vlan, int cos)
{
  int rv, ii;
  cpudb_key_t  rx_cpu_key, tx_cpu_key;
  static int hsl_bcmx_registered = FALSE;

  sal_memset(&rx_cpu_key, 0, sizeof(cpudb_key_t));
  sal_memset(&tx_cpu_key, 0, sizeof(cpudb_key_t));

  if (!hsl_bcmx_registered) 
  {
     if ((rv = bcm_stack_attach_register(hsl_attach_callback)) < 0) {
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
              "Could not register BCMX with stack attach: %s\n", bcm_errmsg(rv));
    } else {
        hsl_bcmx_registered = TRUE;
    }
  }

  /* First create the CPUDB - assuming only a single instance for now.
     To make it work with the Virtual Router scenario we need to
      */
  rv = hsl_bcm_cpudb_create ();
  
  if (rv < 0)
  {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, 
          "ERROR: vlan %d CPU port add failed: %s\n", vlan);
      HSL_FN_EXIT (-1);
  }

  /* Set the DB as current */
  hsl_bcm_cpudb_set_current (0);

  /* now create CPUDB entry for self */
  hsl_bcm_set_cpudb_properties (0, (char*)mac/* KEY */, (char*)mac, base_dest_port, 0/* flags */,
      master_prio, slot_id, dest_unit, -1/* TX unit */, -1 /* TX port */, 
      -1 /* Dest Mod */, -1 /* Dest Port */, -1 /* Topo idx */);

  
  for (ii = 0; ((ii < port_struct->num_ports) && 
               (ii < HSL_MAX_NUM_STK_PORTS_ON_SYSTEM)); ii++)
  {  
      hsl_bcm_add_stack_port (0, port_struct->unit[ii], port_struct->port[ii],
          port_struct->weight[ii], 0 /* Base flags*/, 0 /* flags */, 
          -1 /* Tx stack IDX */, -1 /* Rx stack IDX */, (char*)mac, (char*)&(tx_cpu_key.key),
          (char*)&(rx_cpu_key.key));
  }
 
  hsl_bcm_set_modid (0, (char*)mac, -1 /* Mod ID*/, pref_mod_id, mod_ids_req);

  hsl_bcm_set_local_key (0, (char*)mac);

  hsl_bcm_stack_mode_set (0, higig, sl, 0); 

  hsl_bcm_start_nexthop (0, -1 /* Thread Prio*/, -1 /* Rx Prio */, vlan, cos);
  hsl_bcm_start_atp (0, -1/* Thread Prio*/, -1/* Rx Pri */, -1/* TX Pri */,
      vlan, cos);

  hsl_bcm_rpc_start ();

  hsl_bcm_start_stk_task (0, vlan, cos, 1/* Auto Trunk */, -1/* Cfg2us*/, 
      -1/* timeout_us */, -1 /* retrx_us */, HSL_DEFAULT_PRIO); 

  /* Also register functions to be called when attach or detach of a device happens */
  bcm_stack_attach_register (hsl_bcm_device_attach_register);
#if 0
  bcmx_uport_create_callback_set (hsl_bcm_port_attach_register);
#endif

  HSL_FN_EXIT (0);
}

