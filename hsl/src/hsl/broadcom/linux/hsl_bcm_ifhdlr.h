/* Copyright (C) 2004   All Rights Reserved. */

#ifndef _HSL_BCM_IFHDLR_H
#define _HSL_BCM_IFHDLR_H

/* Interface handler task message structure. */
struct hsl_bcm_ifhdlr_msg
{
  u_char type;
#define HSL_BCM_PORT_ATTACH_NOTIFICATION         1
#define HSL_BCM_PORT_DETACH_NOTIFICATION         2
#define HSL_BCM_PORT_LINKSCAN_NOTIFICATION       3

  union
  {
    /* Structure for port attachment. */
    struct
    {
      bcmx_lport_t lport;
      int unit;
      bcm_port_t port;
      uint32 flags;
    } port_attach;

    /* Structure for port detachment. */
    struct 
    {
      bcmx_lport_t lport;
      bcmx_uport_t uport;
    } port_detach;

    /* Structure for link scanning. */
    struct
    {
      bcmx_lport_t lport;
      bcm_port_info_t info;
    } link_scan;
  } u;
};

/* 
   Rx queue length.
*/
#define HSL_BCM_IF_EVENT_QUEUE_SIZE      60

/*
  Interface handler message handling queue.
*/
struct hsl_bcm_if_event_queue
{
  u_char *queue;                      /* Interface event queue. */
  int total;                          /* Total queue size. */
  int head;                           /* Head of queue. */
  int tail;                           /* Tail of queue. */
  int count;                          /* Number of events in queue. */
  int drop;                           /* Number of dropped events. Should not hit this... */
  struct sal_thread_s *ifhdlr_thread; /* Inter execution thread. */
  apn_sem_id ifhdlr_sem;              /* Interface event handler semaphore. */
  int thread_exit;                    /* If 1, exit packet processing. */
};

/* 
   Interface event queue helper macros. 
*/
#define HSL_BCM_IF_EVENT_QUEUE_NEXT(index) (((index)+ 1) % p_hsl_bcm_if_event_queue->total)
#define HSL_BCM_IF_EVENT_QUEUE_FULL        (p_hsl_bcm_if_event_queue->count == p_hsl_bcm_if_event_queue->total)
#define HSL_BCM_IF_EVENT_QUEUE_EMPTY       (p_hsl_bcm_if_event_queue->count == 0)

/* Errors. */
#define HSL_BCM_ERR_IFHDLR_BASE                      -100
#define HSL_BCM_ERR_IFMAP_REGISTER                   (HSL_BCM_ERR_IFHDLR_BASE - 1)
#define HSL_BCM_ERR_IFHDLR_NOMEM                     (HSL_BCM_ERR_IFHDLR_BASE - 2)
#define HSL_BCM_ERR_IFHDLR_INIT                      (HSL_BCM_ERR_IFHDLR_BASE - 3)

#endif /* _HSL_BCM_IFMGR_H */
