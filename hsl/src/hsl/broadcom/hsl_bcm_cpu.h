/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_BCM_STACKING_H_
#define _HSL_BCM_STACKING_H_

/* HAVE_STACKING */
#if 1

#define HSL_MAX_DBS                         10
#define HSL_MAX_NUM_STK_PORTS_ON_SYSTEM     6
#define HSL_DEFAULT_PRIO                    100  

struct hsl_bcm_port_list_t {
  int num_ports;
  int unit[HSL_MAX_NUM_STK_PORTS_ON_SYSTEM];
  int port[HSL_MAX_NUM_STK_PORTS_ON_SYSTEM];
  int weight[HSL_MAX_NUM_STK_PORTS_ON_SYSTEM];
};

/* This same structure should be reftlected in the hal calls too */
typedef struct hsl_bcm_port_list_t hsl_bcm_port_list;

int
hsl_bcm_cpudb_create ();

int
hsl_bcm_cpudb_destroy (int db_num);

int
hsl_bcm_cpudb_set_current (int db_num);


int
hsl_bcm_set_cpudb_properties (int db_idx, char *key_val, char *mac,
                              int base_dest_port, int flags,
                              int master_pri, int slot_id, int dest_unit,
                              int tx_unit, int tx_port, int dest_mod,
                              int dest_port, int topo_idx);

int
hsl_bcm_set_modid (int db_idx, char *key_val, int mod_id,
                   int pref_mod_id, int mod_ids_req);

int
hsl_bcm_add_stack_port (int db_idx, int unit, int stk_port, int weight,
                        int base_flags, int flags, int tx_stk_idx,
                        int rx_stk_idx, char *key_val, char *tx_key,
                        char *rx_key);

int
hsl_bcm_set_master (int db_idx, char *key_val);

int
hsl_bcm_set_local_key (int db_idx, char *key_val);

int
hsl_bcm_stack_mode_set (int stk_unit, int hg, int sl, int none);

int
hsl_bcm_print_nexthop_info(cpudb_key_t *key);

int
hsl_bcm_start_nexthop (int db_idx, int thrd_pri, int rx_pri,
                       int nhvlan, int nhcos);

int
hsl_bcm_start_atp(int db_idx, int thrd_pri, int rx_pri, int tx_pri,
                  int atp_vlan, int atp_cos);

int
hsl_bcm_rpc_start ();

int
hsl_bcm_rpc_stop ();

int
hsl_bcm_start_stk_task (int db_idx, int disc_vlan, int disc_cos, int auto_trunk,
                    int cfg_to_us, int timeout_us, int retrx_us, int pri);


int
hsl_bcm_start_stacking (unsigned char *mac, int unit,
                    unsigned int base_dest_port,
                    hsl_bcm_port_list *port_struct, int master_prio,
                    int slot_id, int dest_unit, unsigned int sl,
                    unsigned int higig, int pref_mod_id, int mod_ids_req,
                    int vlan, int cos);


#endif /* HAVE_STACKING */
#endif /* _HSL_BCM_STACKING_H_ */
