/* Copyright (C) 2009  All Rights Reserved. */

#include "config.h"

#ifdef HAVE_MARVELL

#include "hsl_os.h"
#include "hsl_types.h"
#include "hsl_logs.h"

#include "hsl_ifmgr.h"
#include "hal_socket.h"
#include "hsl_pkt.h"

/*
  L2 Control Frame DMACs.
*/

/* Multicast MAC. */
hsl_mac_address_t multicast_addr    = {0x1, 0x00, 0x5e, 0x00, 0x00, 0x00};

/* Bridge BPDUs. */
hsl_mac_address_t bpdu_addr         = {0x1, 0x80, 0xc2, 0x00, 0x00, 0x00};

/* GMRP BPDUs. */
hsl_mac_address_t gmrp_addr         = {0x1, 0x80, 0xc2, 0x00, 0x00, 0x20};

/* GVRP BPDUs. */
hsl_mac_address_t gvrp_addr         = {0x1, 0x80, 0xc2, 0x00, 0x00, 0x21};

/* LACP. */
hsl_mac_address_t lacp_addr         = {0x1, 0x80, 0xc2, 0x00, 0x00, 0x02};

/* EAPOL. */
hsl_mac_address_t eapol_addr        = {0x1, 0x80, 0xc2, 0x00, 0x00, 0x03};

/* LLDP. */
hsl_mac_address_t lldp_addr         = {0x1, 0x80, 0xc2, 0x00, 0x00, 0x0e};

/* EFM BPDUs. */
hsl_mac_address_t efm_addr         = {0x1, 0x80, 0xc2, 0x00, 0x00, 0x02};

/* Povider STP BPDUs. */
hsl_mac_address_t pro_bpdu_addr    = {0x1, 0x80, 0xc2, 0x00, 0x00, 0x08};

/*  Provider GVRP BPDUs  */
hsl_mac_address_t pro_gvrp_addr    = {0x1, 0x80, 0xc2, 0x00, 0x00, 0x0d}; 

struct hsl_pkt_hw_callbacks *hsl_pkt_hw_cb;
struct hsl_pkt_os_callbacks *hsl_pkt_os_cb;

void hsl_pkt_set_hw_callbacks (struct hsl_pkt_hw_callbacks *cb)
{

  HSL_FN_ENTER ();

  hsl_pkt_hw_cb = cb;

  HSL_FN_EXIT ();

}

void hsl_pkt_set_os_callbacks (struct hsl_pkt_os_callbacks *cb)
{
  HSL_FN_ENTER ();

  hsl_pkt_os_cb = cb;

  HSL_FN_EXIT ();
}

/* ether_type should be in host order */

enum hal_l2_proto_sock_id
hsl_pkt_get_l2_proto_sock_id (char *dmac, u_int16_t ether_type,
                             u_int8_t slow_proto_subtype)
{
  enum hal_l2_proto_sock_id proto;
  
  proto = HAL_SOCK_PROTO_MAX;

  if (! memcmp (dmac, eapol_addr, 6))
    {
      proto = HAL_SOCK_PROTO_8021X;
      goto EXIT;
    }

  if (! memcmp (dmac, bpdu_addr, 6)
      || ! memcmp (dmac, pro_bpdu_addr,6))
    {
      proto = HAL_SOCK_PROTO_MSTP;
      goto EXIT;
    }

  /* GVRP. */
  if (! memcmp (dmac, gvrp_addr, 6))
    {
      proto = HAL_SOCK_PROTO_GARP;
      goto EXIT;
    }

  /* GMRP. */
  if (! memcmp (dmac, gmrp_addr, 6))
    {
      proto = HAL_SOCK_PROTO_GARP;
      goto EXIT;
    }

  /* LACP. */
  if (! memcmp (dmac, lacp_addr, 6))
    {
      if (HSL_L2_SLOW_PROTO_LACP == slow_proto_subtype)
        {
          proto = HAL_SOCK_PROTO_LACP;
        }
      else if (HSL_L2_SLOW_PROTO_EFM == slow_proto_subtype)
        {
          proto = HAL_SOCK_PROTO_EFM;
        }
      goto EXIT;
    }

  /* CFM */
  if (ether_type == HSL_L2_ETH_P_CFM)
    {
      proto = HAL_SOCK_PROTO_CFM;
      goto EXIT;
    }

  /* LLDP */
  if (ether_type == HSL_L2_ETH_P_LLDP)
    {
      proto = HAL_SOCK_PROTO_LLDP;
      goto EXIT;
    }

EXIT:
  return proto;

}

int hsl_pkt_handle_l2_packet (u_int8_t *buf, u_int32_t len,
                              enum hal_l2_proto_sock_id proto)
{
}

#endif /* HAVE_MARVELL */
