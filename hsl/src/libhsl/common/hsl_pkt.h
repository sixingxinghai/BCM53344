/* Copyright (C) 2009  All Rights Reserved. */

#ifndef HSL_PKT_H_
#define HSL_PKT_H_

/*
  L2 Control Frame DMACs.
*/

/* Multicast MAC. */
extern hsl_mac_address_t multicast_addr;

/* Bridge BPDUs. */
extern hsl_mac_address_t bpdu_addr;

/* GMRP BPDUs. */
extern hsl_mac_address_t gmrp_addr;

/* GVRP BPDUs. */
extern hsl_mac_address_t gvrp_addr;

/* LACP. */
extern hsl_mac_address_t lacp_addr;

/* EAPOL. */
extern hsl_mac_address_t eapol_addr;

/* LLDP. */
extern hsl_mac_address_t lldp_addr;

/* EFM BPDUs. */
extern hsl_mac_address_t efm_addr;

/* Povider STP BPDUs. */
extern hsl_mac_address_t pro_bpdu_addr;

/*  Provider GVRP BPDUs  */
extern hsl_mac_address_t pro_gvrp_addr;

/*
   HSL Packet handler HW interface APIs. These APIs are to be
   implemented for each HW component.
*/

struct hsl_pkt_hw_callbacks
{

   /* Send packet through an interface.

      Parameters:
      IN -> ifp - interface pointer
      IN -> payload - Payload
      IN -> payload_len - Payload Length
      IN -> protocol  - Protocol entity trying to send the packet
      IN -> tagged - Whether the frame needs to be send tagged.

      Returns:
      0 on success
      < 0 on error
   */

  int (*hw_pkt_send) (struct hsl_if *ifp, u_int8_t *payload,
                      u_int16_t payload_len,
                      enum hal_l2_proto_sock_id protocol,
                      u_int8_t tagged);


};

/*
   HSL Packet handler OS interface APIs. These APIs are to be
   implemented for each OS component.
*/

struct hsl_pkt_os_callbacks
{

   /* Send packet through an interface.

      Parameters:
      IN -> payload - Payload
      IN -> payload_len - Payload Length
      IN -> protocol  - Protocol entity to send the packet

      Returns:
      0 on success
      < 0 on error
   */

  int (*os_pkt_handle_l2_packet) (u_int8_t *payload, u_int32_t payload_len,
                                  enum hal_l2_proto_sock_id proto);
};

extern struct hsl_pkt_hw_callbacks *hsl_pkt_hw_cb;
extern struct hsl_pkt_os_callbacks *hsl_pkt_os_cb;

void hsl_pkt_set_hw_callbacks (struct hsl_pkt_hw_callbacks *cb);
void hsl_pkt_set_os_callbacks (struct hsl_pkt_os_callbacks *cb);

enum hal_l2_proto_sock_id
hsl_pkt_get_l2_proto_sock_id (char *dmac, u_int16_t ether_type,
                              u_int8_t slow_proto_subtype);

#define HSL_PKT_HWCB_CHECK(FN)  (hsl_pkt_hw_cb && hsl_pkt_hw_cb->FN)
#define HSL_PKT_HWCB_CALL(FN)   (hsl_pkt_hw_cb->FN)

#define HSL_PKT_OSCB_CHECK(FN)  (hsl_pkt_os_cb && hsl_pkt_os_cb->FN)
#define HSL_PKT_OSCB_CALL(FN)   (hsl_pkt_os_cb->FN)

#define HSL_L2_ETH_P_SLOW_PROTO         0x8809
#define HSL_L2_ETH_P_LLDP               0x88cc
#define HSL_L2_ETH_P_CFM                0x8902

#define HSL_L2_SLOW_PROTO_LACP          0x01
#define HSL_L2_SLOW_PROTO_EFM           0x03

#endif /* HSL_PKT_H_ */
