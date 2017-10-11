/* Copyright (C) 2002-2003  All Rights Reserved. */

#ifndef _PACOS_QOS_SERV_H
#define _PACOS_QOS_SERV_H

/* Interface structure for QOS */
struct qos_interface
{
  struct interface *ifp;
  /*
    aggr_rsvd_bw [p] = sum of bandwidths reserved at 
    holding priority q across class types where q <= p
  */
  float32_t aggr_rsvd_bw[MAX_PRIORITIES];

#ifdef HAVE_DSTE
  /*
    sum of reserved bandwidths of all the established LSPs 
    belonging to class CTb with holding priority of q where :
    TE_Class [i] -- <CTb, p> and  q <= p
  */
  float32_t ct_aggr_rsvd_bw[MAX_TE_CLASS];
#endif /* HAVE_DSTE */
  
  /* Table array of resources keyed on resource id */
  /* The array index are based on hold priorities */
  struct route_table *resource_array [MAX_PRIORITIES];

  /* Status of interface */
#define QOS_INTERFACE_DISABLED     0
#define QOS_INTERFACE_ENABLED      1
  u_char status;
};

#define QOS_RESOURCE_KEY(p, id) \
do { \
  pal_mem_set ((p), 0, sizeof (struct prefix)); \
  (p)->family = AF_INET; \
  (p)->prefixlen = IPV4_MAX_PREFIXLEN; \
  PREP_FOR_NETWORK ((p)->u.prefix4.s_addr, (id), IPV4_MAX_PREFIXLEN); \
} while (0)
  

/* Begin prototypes */
int  nsm_qos_serv_update_max_bw (struct interface *, float32_t);
int  nsm_qos_serv_update_max_resv_bw (struct interface *, float32_t, bool_t);
#ifdef HAVE_DSTE
int  nsm_qos_serv_update_bw_constraint (struct interface *, float32_t, int);
void nsm_qos_serv_preempt_by_te_class (struct list *, struct qos_interface *,
                                       u_char);
void nsm_qos_serv_te_class_avail_bw_update (struct qos_interface *, u_char);
#endif /* HAVE_DSTE */
void nsm_qos_serv_init (struct nsm_master *);
void nsm_qos_serv_if_deinit (struct interface *, bool_t, bool_t, bool_t);
void nsm_qos_serv_clean_for (struct nsm_master *, u_char, u_int32_t);
struct qos_interface *nsm_qos_serv_if_init (struct interface *);
struct qos_interface *nsm_qos_serv_if_get (struct nsm_master *, u_int32_t);
void nsm_qos_serv_send_preempt (struct nsm_master *, struct list **, bool_t,
                                bool_t);
int nsm_read_qos_client_init (struct nsm_msg_header *, void *, void *);
int nsm_read_qos_client_release (struct nsm_msg_header *, void *, void *);
int nsm_read_qos_client_reserve (struct nsm_msg_header *, void *, void *);
int nsm_read_qos_client_probe (struct nsm_msg_header *, void *, void *);
int nsm_read_qos_client_modify (struct nsm_msg_header *, void *, void *);
int nsm_read_qos_client_clean (struct nsm_msg_header *, void *, void *);

#ifdef HAVE_GMPLS
int nsm_read_gmpls_qos_client_init (struct nsm_msg_header *, void *, void *);
int nsm_read_gmpls_qos_client_release (struct nsm_msg_header *, void *, void *);
int nsm_read_gmpls_qos_client_reserve (struct nsm_msg_header *, void *, void *);
int nsm_read_gmpls_qos_client_probe (struct nsm_msg_header *, void *, void *);
int nsm_read_gmpls_qos_client_modify (struct nsm_msg_header *, void *, void *);
int nsm_read_gmpls_qos_client_clean (struct nsm_msg_header *, void *, void *);
#endif /*HAVE_GMPLS*/                               
/* End prototypes */

#endif /* _PACOS_QOS_SERV_H */
