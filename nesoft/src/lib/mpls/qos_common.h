/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_QOS_COMMON_H
#define _PACOS_QOS_COMMON_H

#include "pal.h"

#ifdef HAVE_MPLS
#include "mpls/mpls.h"
#include "prefix.h"
#endif /* HAVE_MPLS */

struct prefix;
struct stream;

/* Priorities are 0 -- 7 */
#define MAX_PRIORITIES                 8
#define TE_PRIORITY_INVALID            MAX_PRIORITIES

/* Types of traffic supported */
#define QOS_TRAFFIC_TYPE_UNDEFINED     0
#define QOS_TRAFFIC_TYPE_CONTROLLED    1
#define QOS_TRAFFIC_TYPE_GUARANTEED    2

/* Exclusivity flag */
#define QOS_NON_EXCLUSIVE              0
#define QOS_EXCLUSIVE                  1

/* Infinity specifications (single precision floating point) */
#define POSITIVE_INFINITY              0x7f80000
#define NEGATIVE_INFINITY              0x8000000

/* Either QOS_OK == 0 or error code */
typedef u_int32_t qos_status;

/* A generic token bucket */ 
struct qos_bucket
{
  /* Bucket flags */
#define QOS_BUCKET_RATE                (1 << 0)
#define QOS_BUCKET_SIZE                (1 << 1)
  u_int8_t flags;

  /* Token rate in bytes per second */
  float rate;

  /* Bucket size in tokens (bytes) */
  float size;
};

/* Traffic parameters specifier (semantics depend on system) */
struct qos_traffic_spec
{
  /* Service type (Guaranteed or Controlled Load) */
  u_int8_t service_type;

  /* Exclusive or not (one or zero) */
  u_int8_t is_exclusive;

  /* FLAGS to let client/server know what is being passed */
#define QOS_TSPEC_FLAG_PEAK_RATE          (1 << 0)
#define QOS_TSPEC_FLAG_COMMITTED_BUCKET   (1 << 1)
#define QOS_TSPEC_FLAG_EXCESS_BURST       (1 << 2)
#define QOS_TSPEC_FLAG_WEIGHT             (1 << 3)
#define QOS_TSPEC_FLAG_MIN_POLICED_UNIT   (1 << 4)
#define QOS_TSPEC_FLAG_MAX_PACKET_SIZE    (1 << 5)
  u_int8_t flags;

  /* Token peak rate (Optional) */
  float peak_rate;

  /* Token bucket parameters for the committed rate (Optional) */
  struct qos_bucket committed_bucket;

  /* The excess burst size for the committed rate (Optional) */
  float excess_burst;

  /* Indicates the proportion of this flow's excess bandwidth */
  /* from the overall excess bandwidth (Optional) */
  float weight;

  /* Minimal policed unit */
  u_int32_t mpu;

  /* Maximum packet size */
  u_int32_t max_packet_size;
};

/* Interfaces specifier (semantics depend on system) */
struct qos_if_spec
{
  /* Using ifindexes or prefixes */
#define QOS_IFSPEC_FLAG_PREFIX        (1 << 0)
  u_int8_t flags;

  /* Incoming interface on which to reserve */
  u_int32_t in_ifindex;

  /* Outgoing interface on which to reserve */
  u_int32_t out_ifindex;

  /* Previous hop (Optional) */
  struct prefix prev_hop;

  /* Next hop (Optional) */
  struct prefix next_hop;
};

/* Advertisement Spec (Statistical) data */
struct qos_ad_spec
{
#define QOS_ADSPEC_FLAG_SLACK         (1 << 0)
#define QOS_ADSPEC_FLAG_CTOT          (1 << 1)
#define QOS_ADSPEC_FLAG_DTOT          (1 << 2)
#define QOS_ADSPEC_FLAG_CSUM          (1 << 3)
#define QOS_ADSPEC_FLAG_DSUM          (1 << 4)
#define QOS_ADSPEC_FLAG_MPL           (1 << 5)
#define QOS_ADSPEC_FLAG_PBE           (1 << 6)
  u_int8_t flags;

  float slack; /* The Slack term */
  float Ctot;
  float Dtot;
  float Csum;
  float Dsum;
  float mpl;  /* Min path latency */
  float pbe;  /* Path bw estimate */
};

#ifdef HAVE_GMPLS
/* GMPLS QoS IF Spec */ 
struct gmpls_qos_if_spec
{
#define GMPLS_QOS_IFSPEC_FLAG_TL_GIFINDEX  (1 << 0)
#define GMPLS_QOS_IFSPEC_FLAG_DL_GIFINDEX  (1 << 1)
#define GMPLS_QOS_IFSPEC_FLAG_CA_RTR_ID    (1 << 2)
  u_int8_t flags;
 
  /* TE Link gifindex */
  u_int32_t tel_gifindex;

  /* DATA Link gifindex */
  u_int32_t dl_gifindex;
  
  /* Control Adjacency Router ID */
  struct pal_in4_addr rtr_id;
};

struct gmpls_qos_attr
{
#define GMPLS_QOS_ATTR_FLAG_PROTECTION  (1 << 0)
#define GMPLS_QOS_ATTR_FLAG_GPID        (1 << 1)

  u_int8_t flags;
  
  /* Switching Capability */
  u_int8_t sw_cap;
    
  /* Encoding Type */
  u_int8_t encoding;

  /* Desired Link Protection (optional)*/
  u_int8_t protection;

  /* G-PID (optional)*/
  u_int16_t gpid;
};

/* GMPLS QoS IF Spec */ 
struct gmpls_qos_label_spec
{
#define GMPLS_QOS_LABEL_SPEC_LABEL_SET  (1 << 0)
#define GMPLS_QOS_LABEL_SPEC_SUGG_LABEL (1 << 1)
  u_int8_t flags;

  /* From and to indicate the range that the suggested label is sought from */
  u_int32_t  sl_lbl_set_from;
  u_int32_t  sl_lbl_set_to;

  /* Suggesetd label returned by RIB to RSVP */
  struct gmpls_gen_label   sugg_label;

};
#endif /*HAVE_GMPLS*/

/* QOS resource */
struct qos_resource
{
  /* Resource id. Use LABEL_VALUE_INVALID for probe message. */
  u_int32_t resource_id;

  /* Protocol id (Helps in cleanup etc). */
  u_int8_t protocol_id;

  /* QoS message id. */
  u_int32_t id;

  /* Owner. */
  struct mpls_owner owner;

  /* Resource specific flags. */
#define QOS_RESOURCE_FLAG_TRAFFIC_SPEC      (1 << 1)
#define QOS_RESOURCE_FLAG_IF_SPEC           (1 << 2)
#define QOS_RESOURCE_FLAG_AD_SPEC           (1 << 3)
#define QOS_RESOURCE_FLAG_SETUP_PRIORITY    (1 << 4)
#define QOS_RESOURCE_FLAG_HOLD_PRIORITY     (1 << 5)
#define QOS_RESOURCE_FLAG_GMPLS_IF_SPEC     (1 << 6)
#define QOS_RESOURCE_FLAG_GMPLS_ATTR        (1 << 7)
#define QOS_RESOURCE_FLAG_GMPLS_LABEL_SPEC  (1 << 8)
#define QOS_RESOURCE_FLAG_GMPLS_REVERSE     (1 << 9)
  u_int16_t flags;

  /* Traffic spec (Required). */
  struct qos_traffic_spec t_spec;

  /* Interface spec (Required). */
  struct qos_if_spec if_spec;

  /* Ad spec (Optional). */
  struct qos_ad_spec ad_spec;

  /* Setup specific priority. */
  u_int8_t setup_priority;

  /* Pre-emption specific priority. */
  u_int8_t hold_priority;

  /* PacOS server's client. To be used only when the resource is stored
     in the server */
  struct zserv *client;

  /* Void pointer for storing pointer to the holder in the protocol.
     Only used by clients. */
  void *data;

  /* Reference count -- for sharing a resource. */
  u_int32_t refcount;

  /* Class Type number. For DSTE usage. */
  u_char ct_num;

#ifdef HAVE_GMPLS
  /* GMPLS Interface Spec */
  struct gmpls_qos_if_spec gif_spec;

  /* GMPLS Attributes  */
  struct gmpls_qos_attr gmpls_attr;
   
  /* GMPLS Label Spec */
  struct gmpls_qos_label_spec label_spec;
#endif 
};

/*******************************************************
 * Begin Error Codes.
 */

/* OK Status */
#define QOS_OK                        0x0

/* Unspecified error */
#define QOS_ERR                       0xFFFFFFFF

/*
 * End Error Codes.
 *******************************************************/

/*******************************************************
 * Begin Bucket macros.
 */

#define qos_bucket_check_rate(entryp) \
        (((entryp)->flags) & QOS_BUCKET_RATE)
#define qos_bucket_set_rate(entryp) \
        (((entryp)->flags) |= QOS_BUCKET_RATE)
#define qos_bucket_unset_rate(entryp) \
        (((entryp)->flags) &= ~QOS_BUCKET_RATE)

#define qos_bucket_check_size(entryp) \
        (((entryp)->flags) & QOS_BUCKET_SIZE)
#define qos_bucket_set_size(entryp) \
        (((entryp)->flags) |= QOS_BUCKET_SIZE)
#define qos_bucket_unset_size(entryp) \
        (((entryp)->flags) &= ~QOS_BUCKET_SIZE)

/*
 * End Bucket macros.
 ******************************************************/


/*******************************************************
 * Begin Traffic Spec macros.
 */

#define qos_tspec_check_peak_rate(entryp) \
        (((entryp)->flags) & QOS_TSPEC_FLAG_PEAK_RATE)
#define qos_tspec_set_peak_rate(entryp) \
        (((entryp)->flags) |= QOS_TSPEC_FLAG_PEAK_RATE)
#define qos_tspec_unset_peak_rate(entryp) \
        (((entryp)->flags) &= ~QOS_TSPEC_FLAG_PEAK_RATE)

#define qos_tspec_check_committed_bucket(entryp) \
        (((entryp)->flags) & QOS_TSPEC_FLAG_COMMITTED_BUCKET)
#define qos_tspec_set_committed_bucket(entryp) \
        (((entryp)->flags) |= QOS_TSPEC_FLAG_COMMITTED_BUCKET)
#define qos_tspec_unset_committed_bucket(entryp) \
        (((entryp)->flags) &= ~QOS_TSPEC_FLAG_COMMITTED_BUCKET)

#define qos_tspec_check_excess_burst(entryp) \
        (((entryp)->flags) & QOS_TSPEC_FLAG_EXCESS_BURST)
#define qos_tspec_set_excess_burst(entryp) \
        (((entryp)->flags) |= QOS_TSPEC_FLAG_EXCESS_BURST)
#define qos_tspec_unset_excess_burst(entryp) \
        (((entryp)->flags) &= ~QOS_TSPEC_FLAG_EXCESS_BURST)

#define qos_tspec_check_weight(entryp) \
        (((entryp)->flags) & QOS_TSPEC_FLAG_WEIGHT)
#define qos_tspec_set_weight(entryp) \
        (((entryp)->flags) |= QOS_TSPEC_FLAG_WEIGHT)
#define qos_tspec_unset_weight(entryp) \
        (((entryp)->flags) &= ~QOS_TSPEC_FLAG_WEIGHT)

#define qos_tspec_check_min_policed_unit(entryp) \
        (((entryp)->flags) & QOS_TSPEC_FLAG_MIN_POLICED_UNIT)
#define qos_tspec_set_min_policed_unit(entryp) \
        (((entryp)->flags) |= QOS_TSPEC_FLAG_MIN_POLICED_UNIT)
#define qos_tspec_unset_min_policed_unit(entryp) \
        (((entryp)->flags) &= ~QOS_TSPEC_FLAG_MIN_POLICED_UNIT)

#define qos_tspec_check_max_packet_size(entryp) \
        (((entryp)->flags) & QOS_TSPEC_FLAG_MAX_PACKET_SIZE)
#define qos_tspec_set_max_packet_size(entryp) \
        (((entryp)->flags) |= QOS_TSPEC_FLAG_MAX_PACKET_SIZE)
#define qos_tspec_unset_max_packet_size(entryp) \
        (((entryp)->flags) &= ~QOS_TSPEC_FLAG_MAX_PACKET_SIZE)

/*
 * End Traffic Spec macros.
 ******************************************************/

/*******************************************************
 * Begin Interface Spec macros.
 */

#define qos_ifspec_check_prefix(entryp) \
        (((entryp)->flags) & QOS_IFSPEC_FLAG_PREFIX)
#define qos_ifspec_set_prefix(entryp) \
        (((entryp)->flags) |= QOS_IFSPEC_FLAG_PREFIX)
#define qos_ifspec_unset_prefix(entryp) \
        (((entryp)->flags) &= ~QOS_IFSPEC_FLAG_PREFIX)

/*
 * End Traffic Spec macros.
 ******************************************************/

/*******************************************************
 * Begin Ad spec macros.
 */

#define qos_adspec_check_slack(entryp) \
        (((entryp)->flags) & QOS_ADSPEC_FLAG_SLACK)
#define qos_adspec_set_slack(entryp) \
        (((entryp)->flags) |= QOS_ADSPEC_FLAG_SLACK)
#define qos_adspec_unset_slack(entryp) \
        (((entryp)->flags) &= ~QOS_ADSPEC_FLAG_SLACK)

#define qos_adspec_check_ctot(entryp) \
        (((entryp)->flags) & QOS_ADSPEC_FLAG_CTOT)
#define qos_adspec_set_ctot(entryp) \
        (((entryp)->flags) |= QOS_ADSPEC_FLAG_CTOT)
#define qos_adspec_unset_ctot(entryp) \
        (((entryp)->flags) &= ~QOS_ADSPEC_FLAG_CTOT)

#define qos_adspec_check_dtot(entryp) \
        (((entryp)->flags) & QOS_ADSPEC_FLAG_DTOT)
#define qos_adspec_set_dtot(entryp) \
        (((entryp)->flags) |= QOS_ADSPEC_FLAG_DTOT)
#define qos_adspec_unset_dtot(entryp) \
        (((entryp)->flags) &= ~QOS_ADSPEC_FLAG_DTOT)

#define qos_adspec_check_csum(entryp) \
        (((entryp)->flags) & QOS_ADSPEC_FLAG_CSUM)
#define qos_adspec_set_csum(entryp) \
        (((entryp)->flags) |= QOS_ADSPEC_FLAG_CSUM)
#define qos_adspec_unset_csum(entryp) \
        (((entryp)->flags) &= ~QOS_ADSPEC_FLAG_CSUM)

#define qos_adspec_check_dsum(entryp) \
        (((entryp)->flags) & QOS_ADSPEC_FLAG_DSUM)
#define qos_adspec_set_dsum(entryp) \
        (((entryp)->flags) |= QOS_ADSPEC_FLAG_DSUM)
#define qos_adspec_unset_dsum(entryp) \
        (((entryp)->flags) &= ~QOS_ADSPEC_FLAG_DSUM)

#define qos_adspec_check_mpl(entryp) \
        (((entryp)->flags) & QOS_ADSPEC_FLAG_MPL)
#define qos_adspec_set_mpl(entryp) \
        (((entryp)->flags) |= QOS_ADSPEC_FLAG_MPL)
#define qos_adspec_unset_mpl(entryp) \
        (((entryp)->flags) &= ~QOS_ADSPEC_FLAG_MPL)

#define qos_adspec_check_pbe(entryp) \
        (((entryp)->flags) & QOS_ADSPEC_FLAG_PBE)
#define qos_adspec_set_pbe(entryp) \
        (((entryp)->flags) |= QOS_ADSPEC_FLAG_PBE)
#define qos_adspec_unset_pbe(entryp) \
        (((entryp)->flags) &= ~QOS_ADSPEC_FLAG_PBE)

/*
 * End Ad spec macros.
 ******************************************************/

/*******************************************************
 * Begin Resource macros.
 */

#define qos_resource_check_traffic_spec(entryp) \
        (((entryp)->flags) & QOS_RESOURCE_FLAG_TRAFFIC_SPEC)
#define qos_resource_set_traffic_spec(entryp) \
        (((entryp)->flags) |= QOS_RESOURCE_FLAG_TRAFFIC_SPEC)
#define qos_resource_unset_traffic_spec(entryp) \
        (((entryp)->flags) &= ~QOS_RESOURCE_FLAG_TRAFFIC_SPEC)

#define qos_resource_check_if_spec(entryp) \
        (((entryp)->flags) & QOS_RESOURCE_FLAG_IF_SPEC)
#define qos_resource_set_if_spec(entryp) \
        (((entryp)->flags) |= QOS_RESOURCE_FLAG_IF_SPEC)
#define qos_resource_unset_if_spec(entryp) \
        (((entryp)->flags) &= ~QOS_RESOURCE_FLAG_IF_SPEC)

#define qos_resource_check_ad_spec(entryp) \
        (((entryp)->flags) & QOS_RESOURCE_FLAG_AD_SPEC)
#define qos_resource_set_ad_spec(entryp) \
        (((entryp)->flags) |= QOS_RESOURCE_FLAG_AD_SPEC)
#define qos_resource_unset_ad_spec(entryp) \
        (((entryp)->flags) &= ~QOS_RESOURCE_FLAG_AD_SPEC)

#define qos_resource_check_setup_priority(entryp) \
        (((entryp)->flags) & QOS_RESOURCE_FLAG_SETUP_PRIORITY)
#define qos_resource_set_setup_priority(entryp) \
        (((entryp)->flags) |= QOS_RESOURCE_FLAG_SETUP_PRIORITY)
#define qos_resource_unset_setup_priority(entryp) \
        (((entryp)->flags) &= ~QOS_RESOURCE_FLAG_SETUP_PRIORITY)

#define qos_resource_check_hold_priority(entryp) \
        (((entryp)->flags) & QOS_RESOURCE_FLAG_HOLD_PRIORITY)
#define qos_resource_set_hold_priority(entryp) \
        (((entryp)->flags) |= QOS_RESOURCE_FLAG_HOLD_PRIORITY)
#define qos_resource_unset_hold_priority(entryp) \
        (((entryp)->flags) &= ~QOS_RESOURCE_FLAG_HOLD_PRIORITY)
#ifdef HAVE_GMPLS
#define qos_resource_set_gmpls_if_spec(entryp) \
        (((entryp)->flags) |= QOS_RESOURCE_FLAG_GMPLS_IF_SPEC)
#define qos_resource_unset_gmpls_if_spec(entryp) \
        (((entryp)->flags) &= ~QOS_RESOURCE_FLAG_GMPLS_IF_SPEC)
#define qos_resource_set_gmpls_attr(entryp) \
        (((entryp)->flags) |= QOS_RESOURCE_FLAG_GMPLS_ATTR)
#define qos_resource_unset_gmpls_attr(entryp) \
        (((entryp)->flags) &= ~QOS_RESOURCE_FLAG_GMPLS_ATTR)
#define qos_resource_set_gmpls_label_spec(entryp) \
        (((entryp)->flags) |= QOS_RESOURCE_FLAG_GMPLS_LABEL_SPEC)
#define qos_resource_unset_gmpls_label_spec(entryp) \
        (((entryp)->flags) &= ~QOS_RESOURCE_FLAG_GMPLS_LABEL_SPEC)
#define qos_ifspec_set_gmpls_tl_gifindex(entryp) \
        (((entryp)->flags) != GMPLS_QOS_IFSPEC_FLAG_TL_GIFINDEX)
#define qos_ifspec_unset_gmpls_tl_gifindex(entryp) \
        (((entryp)->flags) &= ~GMPLS_QOS_IFSPEC_FLAG_TL_GIFINDEX)
#define qos_ifspec_set_gmpls_dl_gifindex(entryp) \
        (((entryp)->flags) != GMPLS_QOS_IFSPEC_FLAG_DL_GIFINDEX)
#define qos_ifspec_unset_gmpls_dl_gifindex(entryp) \
        (((entryp)->flags) &= ~GMPLS_QOS_IFSPEC_FLAG_DL_GIFINDEX)
#endif /*HAVE_GMPLS*/
/*
 * End Resource macros.
 ******************************************************/

/* Begin prototypes */
struct qos_resource *qos_common_resource_new (void);
void qos_common_resource_init (struct qos_resource *);
void qos_common_resource_delete (struct qos_resource *);
void qos_common_resource_dump (struct qos_resource *);
void qos_common_resource_populate (struct stream *, struct qos_resource **,
                                   u_int8_t);
void qos_common_stream_populate (struct qos_resource *, u_int8_t,
                                 struct stream **);
const char *qos_common_traffic_type_str (u_int8_t);

/* End prototypes */

#endif /* #define _PACOS_QOS_COMMON_H */
