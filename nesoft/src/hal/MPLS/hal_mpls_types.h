/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_MPLS_TYPES_H_
#define _HAL_MPLS_TYPES_H_

/* Global FTN table. */
#define HAL_MPLS_GLOBAL_FTN_TABLE            -1


/* Opcodes for label operations. */
#define HAL_MPLS_PUSH                   1    
#define HAL_MPLS_POP                    2    
#define HAL_MPLS_SWAP                   3
#define HAL_MPLS_POP_FOR_VPN            4
#define HAL_MPLS_DLVR_TO_IP             5
#define HAL_MPLS_PUSH_AND_LOOKUP        6
#define HAL_MPLS_PUSH_FOR_VC            7
#define HAL_MPLS_PUSH_AND_LOOKUP_FOR_VC 8
#define HAL_MPLS_POP_FOR_VC             9
#define HAL_MPLS_SWAP_AND_LOOKUP        10
#define HAL_MPLS_NO_OP                  11
#define HAL_MPLS_FTN_LOOKUP             12
#define HAL_MPLS_POP_FOR_VPLS           13
#define HAL_MPLS_PHP                    14


/* Reserved label ranges. */
typedef enum
  {
    HAL_MPLS_IPV4_EXPLICIT_NULL,   /* When the Forwarder receives a packet with this label
                                      value at the bottom of the label stack, the stack 
                                      pops the label value; it bases all forwarding of
                                      this packet on the IPv4 header. */
    HAL_MPLS_ROUTER_ALERT_LABEL,   /* When the Forwarder receives a packet with this label
                                      value at the top of the label stack, the forwarder 
                                      uses the next label beneath it in the stack. The 
                                      Router Alert Label is pushed on top of the stack if 
                                      further forwarding is required. */
    HAL_MPLS_IPV6_EXPLICIT_NULL,   /* When the Forwarder receives a packet with this label
                                      value at the bottom of the label stack, it pops the 
                                      label stack and forwards the packet based on the 
                                      IPv6 header. */
    HAL_MPLS_IMPLICIT_NULL         /* When the Forwarder receives a packet with this label
                                      value at the top of thelabel stack, the LSR pops the
                                      stack. */
  } hal_mpls_label_range_t;


#define HAL_MPLS_VC_STYLE_NONE                 0
#define HAL_MPLS_VC_STYLE_MARTINI              1
#define HAL_MPLS_VC_STYLE_VPLS_MESH            2
#define HAL_MPLS_VC_STYLE_VPLS_SPOKE           3

#ifdef HAVE_DIFFSERV
/* LSP type. */
typedef enum
  {
    HAL_MPLS_LSP_DEFAULT,
    HAL_MPLS_ELSP_CONFIGURED,      /* ELSP_CONFIG. */
    HAL_MPLS_ELSP_SIGNALLED,       /* ELSP_SIGNALLED. */
    HAL_MPLS_LLSP                  /* LLSP. */
  } hal_mpls_lsp_type_t;

/* Diffserv information. */
struct hal_mpls_diffserv
{
  /* LSP type. */
  hal_mpls_lsp_type_t lsp_type;   

  /* DSCP-to-EXP mapping for ELSP. */
  unsigned char dscp_exp_map[8];

  /* DSCP value for LLSP. */
  unsigned char dscp;

  /* AF set. Per Hop Behaviour(PHB) scheduling class set. */
  unsigned char af_set;
};
#endif /* HAVE_DIFFSERV */

#ifdef HAVE_TE
struct hal_mpls_qos
{
  /* Resource ID. */
  u_int32_t resource_id;

  /* Class Type number for DSTE usage */
  u_char ct_num;

  /* Setup priority */
  u_int8_t setup_priority;

  /* Pre-emption specific priority */
  u_int8_t hold_priority;

  /* Service type */
  u_int8_t service_type;

  /* Minimum policed unit */
  u_int32_t min_policed_unit;

  /* Maximum packet size */
  u_int32_t max_packet_size;

  /* Token peak rate */
  float peak_rate;

  /* Token rate (byptes per second) */
  float rate;

  /* Bucket size (byptes) */
  float size;

  /* Excess burst size for committed rate */
  float excess_burst;

  /* Indicates the proportion of this flow's excess bandwidth from the 
   * overall excess bandwidth */
  float weight;

  /* Outgoing interface index */
  u_int32_t out_ifindex;
};
#endif /* HAVE_TE */

#endif /* HAL_MPLS_TYPES_H_ */
