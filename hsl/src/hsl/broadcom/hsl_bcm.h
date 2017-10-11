/* Copyright (C) 2004  All Rights Reserved. */

#ifndef __HSL_BCM_H__
#define __HSL_BCM_H__

#define TYPE_LENGTH_OFFSET              16     /* Offset for type/length field. */
#define ARP_TYPE                        0x806  /* ARP protocol type. */
#define IP_DST_OFFSET                   34     /* IP DST file offset. */
#define IP_PROTO_OFFSET                 28     /* IP Protocol field offset. */
#define IP_PROTO_UDP_TYPE               0x11   /* IP Protocol UDP type. */

#ifdef HAVE_ONMD
#define IP_PROTO_CFM_UNTAG_OFFSET       12
#define IP_PROTO_CFM_SINGLE_TAG_OFFSET  14
#define IP_PROTO_CFM_DOUBLE_TAG_OFFSET  16
#endif /* HAVE_ONMD */

#define IP_PROTO_ICMP_TYPE              0x01  /* IP Protocol ICMP type. */
#define INADDR_MULTICAST_ADDRESS_BASE   0xe0000000 /* 224.0.0.0 */

#define HSL_BCM_FEATURE_FILTER 1
#define HSL_BCM_FEATURE_FIELD  2


/* Router alert filter defines */
#define IP_OPTIONS_OFFSET          38 
#define ROUTER_ALERT1              0x94
#define ROUTER_ALERT2              0x04
#define ROUTER_ALERT3              0x00
#define ROUTER_ALERT4              0x00

typedef char mac_addr_t[6];
extern mac_addr_t bpdu_addr;
extern mac_addr_t pro_bpdu_addr;
extern mac_addr_t gmrp_addr;
extern mac_addr_t gvrp_addr;
extern mac_addr_t lacp_addr;
extern mac_addr_t eapol_addr;

/* Strata generation type. */
typedef enum 
  {
    HSL_BCM_SOC_TYPE_INVALID,
    HSL_BCM_SOC_TYPE_XGS2,
    HSL_BCM_SOC_TYPE_XGS3
  } hsl_bcm_soc_type;


/* Chip family */
typedef enum
{
  HSL_BCM_CHIP_FAMILY_UNKNOWN,
  HSL_BCM_CHIP_FAMILY_EASYRIDER,
  HSL_BCM_CHIP_FAMILY_TRIUMPH,
  HSL_BCM_CHIP_FAMILY_ENDURO
} hsl_bcm_chip_family_t;


#define HSL_QOS_FILTER_FMT_ETH_II        1
#define HSL_QOS_FILTER_FMT_802_3         2
#define HSL_QOS_FILTER_FMT_SNAP          4
#define HSL_QOS_FILTER_FMT_LLC           8

/*
  Initialize BCMX layer.
*/
int hsl_bcmx_init (void);

/*
  Deinitialize BCMX layer.
*/
int hsl_bcmx_deinit (void);

/* 
  Attach/detach callback (tailored for stack task)
*/
void hsl_attach_callback (int, int, cpudb_entry_t *, int);

/*
  Initialize Broadcom specific data.
*/
int hsl_hw_init (void);

/*
  Deinitialize Broadcom specific data.
*/
int hsl_hw_deinit (void);


int hsl_soft_init(void);
int hsl_soft_init(void);

/*
  Initialize the MAC base address.
*/
int hsl_hw_mac_base_set (bcm_mac_t mac);

/*
  Set SOC type.
*/
void hsl_hw_set_soc_type (hsl_bcm_soc_type type);

/* 
  Get SOC type. 
*/
hsl_bcm_soc_type hsl_hw_get_soc_type (void);


void hsl_hw_set_chip_family (hsl_bcm_chip_family_t family);

hsl_bcm_chip_family_t hsl_bcm_get_chip_family (void);

/* 
  Get type of filtering supported on the box.  
*/
int 
hsl_bcm_filter_type_get(void);

#ifdef HAVE_L3
int
hsl_bcm_router_alert_filter_install ();

int
hsl_bcm_router_alert_filter_uninstall ();

/* 
   Add prefix. 
*/
int 
hsl_bcm_prefix_add (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh);

/* 
   Delete prefix. 
*/
int
hsl_bcm_prefix_delete (hsl_fib_id_t fib_id, struct hsl_route_node *rnp, struct hsl_nh_entry *nh);

#endif /* HAVE_L3 */

int 
hsl_bcm_get_local_cpu (char *buf);
/*djg*/
int
hsl_bcm_timesync_filter_install (unsigned int index, unsigned int port);

int
hsl_bcm_timesync_filter_uninstall (unsigned int index);


#endif /* __HSL_BCM_H__ */
