/* Copyright (C) 2001-2003  All Rights Reserved.  */
 
#ifndef _PACOS_MPLS_FIB_H
#define _PACOS_MPLS_FIB_H

/*---------------------------------------------------------------------------
 * Structure to store the nexthop information. It consists of the nexthop
 * layer 3 address and the interface id of the outgoing interface on which
 * the outgoing packet should be sent.
 ---------------------------------------------------------------------------*/

/* Defs for find/delete */
#define FIB_FIND       0
#define FIB_DELETE     1

/*---------------------------------------------------------------------------
  Label primitive entry 
  ---------------------------------------------------------------------------*/
struct label_primitive
{
  /* Next in list. */
  struct label_primitive *next;

  /* label is only the most significant 20 bits of the shim header,
     signifying the label value. */
  LABEL label;

  /* Opreration in the label stack. */
  uint8 opcode;
};

/*---------------------------------------------------------------------------
  Next Hop Label Forwarding Entry 
  ---------------------------------------------------------------------------*/

/* NHLFE specific stats. */
struct nhlfe_entry_stats
{
#define NHLFE_ENTRY_STAT_TX_BYTES             1
#define NHLFE_ENTRY_STAT_TX_PKTS              2
#define NHLFE_ENTRY_STAT_ERROR_PKTS           3
#define NHLFE_ENTRY_STAT_DISCARD_PKTS         4
  uint32 tx_bytes;
  uint32 tx_pkts;
  uint32 error_pkts;
  uint32 discard_pkts;
};

#define NHLFE_ENTRY_INCR_STAT(f,t,v)          \
  do {                                        \
    switch (t)                                \
      {                                       \
      case NHLFE_ENTRY_STAT_TX_BYTES:         \
        (f)->stats.tx_bytes += (v);           \
        break;                                \
      case NHLFE_ENTRY_STAT_TX_PKTS:          \
        (f)->stats.tx_pkts += (v);            \
        break;                                \
      case NHLFE_ENTRY_STAT_ERROR_PKTS:       \
        (f)->stats.error_pkts += (v);         \
        break;                                \
      case NHLFE_ENTRY_STAT_DISCARD_PKTS:     \
        (f)->stats.discard_pkts += (v);       \
        break;                                \
      default:                                \
        break;                                \
      }                                       \
  } while (0)

struct nhlfe_entry
{
  /* Next in list. */
  struct nhlfe_entry *next;

  /* Routing info corresponding to the LSP. */
  struct rtable *rt;

  /* Pointer to the label/opcode list. Should be NULL terminated. */
  struct label_primitive *primitive;

  /* NHLFE index. */
  uint32 nhlfe_ix;

  /* NHLFE specific statistics. */
  struct nhlfe_entry_stats stats;
};

/* FTN specific stats. */
struct ftn_entry_stats
{
#define FTN_ENTRY_STAT_TX_BYTES               1
#define FTN_ENTRY_STAT_PUSHED_PKTS            2
#define FTN_ENTRY_STAT_MATCHED_BYTES          3
#define FTN_ENTRY_STAT_MATCHED_PKTS           4
  uint32 tx_bytes;
  uint32 pushed_pkts;
  uint32 matched_bytes;
  uint32 matched_pkts;
};

#define FTN_ENTRY_INCR_STAT(f,t,v)            \
  do {                                        \
    switch (t)                                \
      {                                       \
      case FTN_ENTRY_STAT_TX_BYTES:           \
        (f)->stats.tx_bytes += (v);           \
        break;                                \
      case FTN_ENTRY_STAT_PUSHED_PKTS:        \
        (f)->stats.pushed_pkts += (v);        \
        break;                                \
      case FTN_ENTRY_STAT_MATCHED_BYTES:      \
        (f)->stats.matched_bytes += (v);      \
        break;                                \
      case FTN_ENTRY_STAT_MATCHED_PKTS:       \
        (f)->stats.matched_pkts += (v);       \
        break;                                \
      default:                                \
        break;                                \
      }                                       \
  } while (0)

/*---------------------------------------------------------------------------
 * FTN table entry
 ---------------------------------------------------------------------------*/
struct ftn_entry
{
  /* Protocol which made this entry. */
  uint8 protocol;

  /* FEC entry. */
  struct mpls_prefix fec;

  /* NHLFE list. */
  struct nhlfe_entry *nhlfe_list;

  /* FTN index of the entry in the user space process. Used for Fast ReRoute
     and other cases where we need to refer to an entry and not use a lookup */
  uint32 ftn_ix;

  /* FTN owner information. */
  struct mpls_owner_fwd owner;

  /* FTN specific statistics. */
  struct ftn_entry_stats stats;

  /*Bypass FTN index */
  u_int32_t bypass_ftn_ix;

  /* LSP_TYPE */
  u_char lsp_type;

  /* For VC tunnel_ftnix */
  u_int32_t tunnel_ftnix;

  /* Linklist related entry */
  struct ftn_entry *next;
};

/* ILM specific stats. */
struct ilm_entry_stats
{
#define ILM_ENTRY_STAT_RX_BYTES               1
#define ILM_ENTRY_STAT_TX_BYTES               2
#define ILM_ENTRY_STAT_RX_PKTS                3
#define ILM_ENTRY_STAT_RX_HC_BYTES            4
#define ILM_ENTRY_STAT_RX_ERRORS              5
#define ILM_ENTRY_STAT_SWAPPED_PKTS           6
#define ILM_ENTRY_STAT_POPPED_PKTS            7
  uint32 rx_bytes;
  uint32 tx_bytes;
  uint32 rx_pkts;
  uint64 rx_hc_bytes;
  uint32 rx_errors;
  time_t disc_time;
  uint32 swapped_pkts;
  uint32 popped_pkts;
};

#define ILM_ENTRY_INCR_STAT(i,t,v)            \
  do {                                        \
    switch (t)                                \
      {                                       \
      case ILM_ENTRY_STAT_RX_BYTES:           \
        (i)->stats.rx_bytes += (v);           \
        break;                                \
      case ILM_ENTRY_STAT_TX_BYTES:           \
        (i)->stats.tx_bytes += (v);           \
        break;                                \
      case ILM_ENTRY_STAT_RX_PKTS:            \
        (i)->stats.rx_pkts += (v);            \
        break;                                \
      case ILM_ENTRY_STAT_RX_HC_BYTES:        \
        (i)->stats.rx_hc_bytes += (v);        \
        break;                                \
      case ILM_ENTRY_STAT_RX_ERRORS:          \
        (i)->stats.rx_errors += (v);          \
        break;                                \
      case ILM_ENTRY_STAT_SWAPPED_PKTS:       \
        (i)->stats.swapped_pkts += (v);       \
        break;                                \
      case ILM_ENTRY_STAT_POPPED_PKTS:        \
        (i)->stats.popped_pkts += (v);        \
        break;                                \
      default:                                \
        break;                                \
      }                                       \
  } while (0)

/*---------------------------------------------------------------------------
 * ILM table entry
 ---------------------------------------------------------------------------*/
struct ilm_entry
{
  /* Protocol which made this entry. */
  uint8 protocol;

  /* Status. - Moved for better packing. */
  uint8 status;

  /* Incomin label. */
  LABEL label;

  /* Associated FEC. */
  struct mpls_prefix fec;

  /* Incoming interface index. */
  uint32 inl2id;

  /* NHLFE list. */
  struct nhlfe_entry *nhlfe_list;

  /* ILM owner information. */
  struct mpls_owner_fwd owner;

  /* ILM specific statistics. */
  struct ilm_entry_stats stats;
};

/* Top level statistics. */
struct fib_stats
{
#define FIB_HANDLE_STAT_DROPPED_PKTS       1
#define FIB_HANDLE_STAT_IP_TX_PKTS         2
#define FIB_HANDLE_STAT_LABELED_TX_PKTS    3
#define FIB_HANDLE_STAT_DROPPED_LBL_PKTS   4
#define FIB_HANDLE_STAT_SWITCHED_PKTS      5

  /* Dropped packets. */
  uint32 dropped_pkts;

  /* Number of IP packets that could not be mapped to an FTN entry. */
  uint32 ip_tx_pkts;

  /* Number of IP packets that were mapped onto some FTN entry. */
  uint32 labeled_tx_pkts;

  /* Number of labeled packets that were dropped. */
  uint32 dropped_labeled_pkts;

  /* Number of labeled packets that were switched. */
  uint32 switched_pkts;
};

#define FIB_HANDLE_INCR_STAT(f,t,v)           \
  do {                                        \
    switch (t)                                \
      {                                       \
      case FIB_HANDLE_STAT_DROPPED_PKTS:      \
        (f)->stats.dropped_pkts += (v);       \
        break;                                \
      case FIB_HANDLE_STAT_IP_TX_PKTS:        \
        (f)->stats.ip_tx_pkts += (v);         \
        break;                                \
      case FIB_HANDLE_STAT_LABELED_TX_PKTS:   \
        (f)->stats.labeled_tx_pkts += (v);    \
        break;                                \
      case FIB_HANDLE_STAT_DROPPED_LBL_PKTS:  \
        (f)->stats.dropped_labeled_pkts +=(v);\
        break;                                \
      case FIB_HANDLE_STAT_SWITCHED_PKTS:     \
        (f)->stats.switched_pkts += (v);      \
        break;                                \
      }                                       \
  } while (0)

/* Interface specific stats. */
struct interface_entry_stats
{
#define IF_ENTRY_STAT_RX_PKTS                 1
#define IF_ENTRY_STAT_DROPPED_PKTS            2
#define IF_ENTRY_STAT_IP_TX_PKTS              3
#define IF_ENTRY_STAT_LABELED_TX_PKTS         4
#define IF_ENTRY_STAT_DROPPED_LABELED_PKTS    5
#define IF_ENTRY_STAT_SWITCHED_PKTS           6
#define IF_ENTRY_STAT_LABEL_LOOKUP_FAILURES   7
#define IF_ENTRY_STAT_OUT_FRAGMENTS           8
  uint32 rx_pkts;
  uint32 dropped_pkts;
  uint32 ip_tx_pkts;
  uint32 labeled_tx_pkts;
  uint32 dropped_labeled_pkts;
  uint32 switched_pkts;
  uint32 label_lookup_failures;
  uint32 out_fragments;
};

#define IF_ENTRY_INCR_STAT(i,t,v)             \
  do {                                        \
    switch (t)                                \
      {                                       \
      case IF_ENTRY_STAT_RX_PKTS:             \
        (i)->stats.rx_pkts += (v);            \
        break;                                \
      case IF_ENTRY_STAT_DROPPED_PKTS:        \
        (i)->stats.dropped_pkts += (v);       \
        break;                                \
      case IF_ENTRY_STAT_IP_TX_PKTS:          \
        (i)->stats.ip_tx_pkts += (v);         \
        break;                                \
      case IF_ENTRY_STAT_LABELED_TX_PKTS:     \
        (i)->stats.labeled_tx_pkts += (v);    \
        break;                                \
      case IF_ENTRY_STAT_DROPPED_LABELED_PKTS: \
        (i)->stats.dropped_labeled_pkts += (v); \
        break;                                \
      case IF_ENTRY_STAT_SWITCHED_PKTS:       \
        (i)->stats.switched_pkts += (v);      \
        break;                                \
      case IF_ENTRY_STAT_LABEL_LOOKUP_FAILURES: \
        (i)->stats.label_lookup_failures += (v); \
        break;                                \
      case IF_ENTRY_STAT_OUT_FRAGMENTS:       \
        (i)->stats.out_fragments += (v);      \
        break;                                \
      default:                                \
        break;                                \
      }                                       \
  } while (0)

#define NULL_IF_ENTRY_INCR_STAT(i,t,v)        \
  do {                                        \
    switch (t)                                \
      {                                       \
      case IF_ENTRY_STAT_RX_PKTS:             \
        (i)->if_stats.rx_pkts += (v);         \
        break;                                \
      case IF_ENTRY_STAT_DROPPED_PKTS:        \
        (i)->if_stats.dropped_pkts += (v);    \
        break;                                \
      case IF_ENTRY_STAT_IP_TX_PKTS:          \
        (i)->if_stats.ip_tx_pkts += (v);      \
        break;                                \
      case IF_ENTRY_STAT_LABELED_TX_PKTS:     \
        (i)->if_stats.labeled_tx_pkts += (v); \
        break;                                \
      case IF_ENTRY_STAT_DROPPED_LABELED_PKTS: \
        (i)->if_stats.dropped_labeled_pkts += (v); \
        break;                                \
      case IF_ENTRY_STAT_SWITCHED_PKTS:       \
        (i)->if_stats.switched_pkts += (v);   \
        break;                                \
      case IF_ENTRY_STAT_LABEL_LOOKUP_FAILURES: \
        (i)->if_stats.label_lookup_failures += (v); \
        break;                                \
      case IF_ENTRY_STAT_OUT_FRAGMENTS:       \
        (i)->if_stats.out_fragments += (v);   \
        break;                                \
      default:                                \
        break;                                \
      }                                       \
  } while (0)

/* Top level structure. */
struct fib_handle
{
  /* FTN table */
  struct mpls_table *ftn;

  /* VRF table list */
  struct mpls_table *vrf_list;

  /* Interface specific ILM list */
  struct mpls_table *ilm_list;

  /* Refcount for overlying protocols */
  uint16 refcount;

  /* Top level stats. */
  struct fib_stats stats;

  /* Interface specific statistics. */
  struct interface_entry_stats if_stats;
};

/* The Interface structure in the MPLS Forwarder. */
struct mpls_interface
{
  /* Interface index. */
  uint32 ifindex;

#define MPLS_ENABLED      1
#define MPLS_DISABLED     0
  uint8 status;

  /* Label space for this interface. */
  uint16 label_space;

  /* Vitual Circuit data. */
  uint32 vc_id;
  struct ftn_entry *vc_ftn;

  /* VRF table to which this interface will point to, assuming this 
     interface is vrf-enabled. */
  struct mpls_table *vrf;

  /* ILM table used by this interface. */
  struct mpls_table *ilm;

  /* Refcount for overlying protocols. */
  uint8 refcount;

  /* Interface specific statistics. */
  struct interface_entry_stats stats;
#ifdef HAVE_SWFWDR
  void *l2_info;
#endif  /* HAVE_SWFWDR */

};

/*
 *  FEC prefix in host byte order
 */
#define  FEC_PREFIX(daddr, prefix)  \
          (htonl(ntohl(daddr)&(prefix ? (~0<<(32-(prefix))) : 0)))

/* rtable node entry. */
struct rt_node 
{
  struct rtable *rt;
  struct rt_node *next;
};

#define RSVP_CTRL_PKT_SEG      1 << 0 
#define RSVP_CTRL_PKT_BEGIN    1 << 1
#define RSVP_CTRL_PKT_END      1 << 2 

struct bypass_tunnel_msg
{
  uint32 ftn_index;
  uint32 btm_id;
  uint32 flags;
  uint32 total_len;
  uint32 seq_num;
  uint32 pkt_len;
  /* pointer to data */
  char *pkt;
  /* pointer to end of current data */
  char *ptr;

};

#endif /* _PACOS_MPLS_FIB_H */
