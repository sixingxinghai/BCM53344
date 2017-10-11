/* Copyright (C) 2004   All Rights Reserved. */

#ifndef _HSL_BCM_MAP_H_
#define _HSL_BCM_MAP_H_


#define HSL_BCM_TRUNK_2_LPORT(trunk)         (0x80000000 | (trunk))
#define HSL_BCM_LPORT_2_TRUNK_ID(lport)      (lport & 0x7fffffff)

struct hsl_bcm_ifmap_lport
{
  /* Logical port. */
  bcmx_lport_t lport;

  /* Pointer to the ifp from Interface manager. */
  struct hsl_if *ifp;

  /* Index for fe/ge/xe port. */
  int index;

  /* Type of port i.e. fe/ge/xe */
  uint32 type;

  /* Mac address index for this port. */
  int mac_index;
};

/* Interface map data. */
struct hsl_bcm_ifmap
{
  /* Max FE ports. */
  int max_fe_ports;

  /* MAx GE ports. */
  int max_ge_ports;

  /* Max XE ports. */
  int max_xe_ports;

  /* Max AGG ports. */
  int max_agg_ports;

  /* Max ports */
  int max_ports; 
  
  u_char *fe_index_table;
  u_char *ge_index_table;
  u_char *xe_index_table;
  u_char *agg_index_table;

  char **mac_array;
  u_char *mac_index_table;
  int max_macindexes;
#define HSL_BCM_IFMAP_SVI_MAC_INDEX            0
  
  /* AVL tree for lport map information. */
  struct hsl_avl_tree *lport_map_tree;
};

/* 
   BCM user port map is as follows:                                                
   +---------------+---------------+----------------+
   | MODID(4 bits) | Units(3 bits) |  Port(5 bits)  |
   +---------------+---------------+----------------+
*/
#define HSL_BCM_PORT_SHIFTS  0
#define HSL_BCM_UNIT_SHIFTS  5
#define HSL_BCM_MODID_SHIFTS 8
#define HSL_BCM_TOTAL_SHIFTS 12
                                                                                
#define HSL_BCM_PORT_MASK    0x1f
#define HSL_BCM_UNIT_MASK    0x7
#define HSL_BCM_MODID_MASK   0xf

/* Errors from interface map. */
#define HSL_BCM_IFMAP_ERR_BASE                              -100
#define HSL_BCM_ERR_IFMAP_MAX_FE_PORTS_NOT_SET              (HSL_BCM_IFMAP_ERR_BASE + 1)
#define HSL_BCM_ERR_IFMAP_MAX_GE_PORTS_NOT_SET              (HSL_BCM_IFMAP_ERR_BASE + 2)
#define HSL_BCM_ERR_IFMAP_MAX_XE_PORTS_NOT_SET              (HSL_BCM_IFMAP_ERR_BASE + 3)
#define HSL_BCM_ERR_IFMAP_MAX_AGG_PORTS_NOT_SET             (HSL_BCM_IFMAP_ERR_BASE + 4)
#define HSL_BCM_ERR_IFMAP_UNKNOWN_DATA_PORT                 (HSL_BCM_IFMAP_ERR_BASE + 5)
#define HSL_BCM_ERR_IFMAP_MAX_PORTS_ALLOCATED               (HSL_BCM_IFMAP_ERR_BASE + 6)
#define HSL_BCM_ERR_IFMAP_NOMEM                             (HSL_BCM_IFMAP_ERR_BASE + 7)
#define HSL_BCM_ERR_IFMAP_INIT                              (HSL_BCM_IFMAP_ERR_BASE + 8)
#define HSL_BCM_ERR_IFMAP_LPORT_ALREADY_REGISTERED          (HSL_BCM_IFMAP_ERR_BASE + 9)
#define HSL_BCM_ERR_IFMAP_LPORT_NOT_FOUND                   (HSL_BCM_IFMAP_ERR_BASE + 10)

/* Function prototypes. */
int hsl_bcm_ifmap_lport_register (bcmx_lport_t lport, uint32 flags, char *name, u_char *mac);
int hsl_bcm_ifmap_lport_unregister (bcmx_lport_t lport);
int hsl_bcm_ifmap_init (int max_fe, int max_ge, int max_xe, u_char *mac_base);
int hsl_bcm_ifmap_deinit (void);
int hsl_bcm_ifmap_ifindex_set (bcmx_lport_t lport, hsl_ifIndex_t ifindex);
struct hsl_if *hsl_bcm_ifmap_if_get (bcmx_lport_t lport);
int hsl_bcm_ifmap_if_set (bcmx_lport_t lport, struct hsl_if *ifp);
u_int32_t hsl_bcm_port_u2l (int modid, int unit, int port);
void hsl_bcm_port_l2u (u_int32_t uport, int *modid, int *unit, int *port);
int hsl_bcm_ifmap_init (int max_fe, int max_ge, int max_xe, bcm_mac_t mac_base);
int hsl_bcm_ifmap_deinit (void);
char *hsl_bcm_ifmap_svi_mac_get (void);

#endif /* _HSL_BCM_MAP_H_ */
