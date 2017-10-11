/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_BCM_PKT_H_
#define _HSL_BCM_PKT_H_

/* 
   Rx queue length.
*/
/* Kept to original . During Customer Evaluation we bumped it to 10000 */
#define HSL_BCM_PKT_RX_QUEUE_SIZE      1000
#define ALIGN_TO                       4

/* HSL_BCM_RX_PRIO need be less than ATP_RX_PRIORITY for stacking */
#define HSL_BCM_RX_PRIO                 ATP_RX_PRIORITY - 10

/*
  Rx queue. Queue of HANDLE_OWNED packets. To be processed by
  pkt_thread outside the context of RX thread.
*/
struct hsl_bcm_rx_queue
{
  u_char *pkt_queue;                  /* BCM Packet queue of aligned bcm_pkt_t. */
  int total;                          /* Total queue size. */
  int head;                           /* Head of queue. */
  int tail;                           /* Tail of queue. */
  int count;                          /* Number of packets in queue. */
  int drop;                           /* Number of dropped packets. */
  struct sal_thread_s *pkt_thread;    /* Packet execution thread. */
  apn_sem_id pkt_sem;                 /* Packet semaphore. */
  int thread_exit;                    /* If 1, exit packet processing. */
};

/* 
   Rx queue helper macros. 
*/
#define HSL_BCM_PKT_RX_QUEUE_NEXT(index) ((index + 1) % p_hsl_bcm_pkt_master->rx.total)
#define HSL_BCM_PKT_RX_QUEUE_FULL        (p_hsl_bcm_pkt_master->rx.count == p_hsl_bcm_pkt_master->rx.total)
#define HSL_BCM_PKT_RX_QUEUE_EMPTY       (p_hsl_bcm_pkt_master->rx.count == 0)

/*
  Tx pkt list length.
*/
#define HSL_BCM_PKT_TX_LIST_LENGTH   300 

/* 
   Tx list of bcm_pkt_t structures. To be used for preallocated bcm_pkt_t structures
   for the various Tx users(i.e. L2 packet transmittors and L3 packet drivers).
*/
struct hsl_bcm_tx_queue
{
  u_char *pkt_list;                    /* BCM packet list of aligned bcm_pkt_t. */
  u_char *free_pkt_list;               /* Free list of aligned bcm_pkt_t. */
  int total;                           /* Total list size. */
  int count;                           /* Current count. */
  apn_sem_id pkt_sem;                  /* Semaphore to protect this list. */
};

/*
  Packet driver master structure. 
*/
struct hsl_bcm_pkt_master
{
  /* Rx. */
  struct hsl_bcm_rx_queue rx;

  /* Tx. */
  struct hsl_bcm_tx_queue tx;
};

/* 
   Helper macros for protecting the Tx pkt control structures. 
 */
#define HSL_BCM_TX_SEM_LOCK(T)          oss_sem_lock (OSS_SEM_MUTEX, p_hsl_bcm_pkt_master->tx.pkt_sem, (T))
#define HSL_BCM_TX_SEM_UNLOCK           oss_sem_unlock (OSS_SEM_MUTEX, p_hsl_bcm_pkt_master->tx.pkt_sem)


/* 
   Function prototypes. 
*/
int hsl_bcm_pkt_init ();
int hsl_bcm_pkt_deinit ();
bcm_pkt_t *hsl_bcm_tx_pkt_alloc (int num);
void hsl_bcm_tx_pkt_free (bcm_pkt_t *pkt);
int hsl_bcm_pkt_send (bcm_pkt_t *pkt, bcmx_lport_t dport, int tagged);
bcm_rx_t hsl_bcm_rx_cb (int unit, bcm_pkt_t *pkt, void *cookie);
int hsl_bcm_pkt_vlan_flood (bcm_pkt_t *pkt, int vid, struct hsl_if *vlanifp);


int  pkt_multi_send_init(void);
int SendPkt(unsigned  char  *dataptr, unsigned len,   unsigned int flag, bcm_pbmp_t  pbmap,   bcm_pbmp_t utagpbmap);



#endif /* _HSL_BCM_PKT_H_ */
