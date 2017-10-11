/* Copyright (C) 2004   All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"

#include "bcm_incl.h"

#include "hsl_types.h"
#include "hsl_oss.h"
#include "hsl_logs.h"

#include "hsl_bcm_ifcb.h"
#include "hsl_bcm_ifmap.h"
#include "hsl_bcm_ifhdlr.h"
#include "hsl_os.h"
#include "hsl_ifmgr.h"


/* Global data for interface handler. */
static struct hsl_bcm_if_event_queue *p_hsl_bcm_if_event_queue = NULL;
static int aligned_sizeof_hsl_bcm_ifhdlr_msg;
extern struct hsl_if_db *p_hsl_if_db;
extern int _module_create_proc_if(struct hsl_if *ifp);
extern int _module_remove_proc_if(struct hsl_if *ifp);

/* Event post handler. */
static int
_hsl_bcm_if_post_event (struct hsl_bcm_ifhdlr_msg *msg)
{
  struct hsl_bcm_ifhdlr_msg *entry;

  HSL_FN_ENTER ();

  /* Queue the packet. */
  if (HSL_BCM_IF_EVENT_QUEUE_FULL)
    {
      /* Queue is full. */
      p_hsl_bcm_if_event_queue->drop++;

      HSL_FN_EXIT (-1);
    }
  else
    {
      /* Get entry. */
      entry = (struct hsl_bcm_ifhdlr_msg *)&p_hsl_bcm_if_event_queue->queue[p_hsl_bcm_if_event_queue->tail * aligned_sizeof_hsl_bcm_ifhdlr_msg];

      /* Copy the header contents. */
      memcpy (entry, msg, sizeof (struct hsl_bcm_ifhdlr_msg));

      /* Increment count. */
      p_hsl_bcm_if_event_queue->count++;

      /* Adjust tail. */
      p_hsl_bcm_if_event_queue->tail = HSL_BCM_IF_EVENT_QUEUE_NEXT (p_hsl_bcm_if_event_queue->tail);

      /* Give semaphore. */
      oss_sem_unlock (OSS_SEM_BINARY, p_hsl_bcm_if_event_queue->ifhdlr_sem);

      HSL_FN_EXIT (0);
    }

  HSL_FN_EXIT (0);
}

/* Callback function for port creation. */
bcmx_uport_t
_hsl_bcm_port_attach_cb (bcmx_lport_t lport, int unit, bcm_port_t port,
                         uint32 flags)
{
  struct hsl_bcm_ifhdlr_msg msg;
  u_int32_t uport;
  int u, p, m;
  int ret;

  HSL_FN_ENTER ();

  msg.type = HSL_BCM_PORT_ATTACH_NOTIFICATION;
  msg.u.port_attach.lport = lport;
  msg.u.port_attach.unit = unit;
  msg.u.port_attach.port = port;
  msg.u.port_attach.flags = flags;

  /* Post the event. */
  ret =  _hsl_bcm_if_post_event (&msg);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_FATAL, "Failed posting new port attachment event for port %d\n", lport);
      /* XXX Should treat this seriously than we are. */
    }

  u = BCMX_LPORT_BCM_UNIT (lport);
  p = BCMX_LPORT_BCM_PORT (lport);
  m = BCMX_LPORT_MODID (lport);

  uport = hsl_bcm_port_u2l (m, u, p);

  HSL_FN_EXIT ((void *)uport);
}

/* Callback function for port deletion. */
static void
_hsl_bcm_port_detach_cb (bcmx_lport_t lport, bcmx_uport_t uport)
{
  struct hsl_bcm_ifhdlr_msg msg;
  int ret;

  HSL_FN_ENTER ();

  msg.type = HSL_BCM_PORT_DETACH_NOTIFICATION;
  msg.u.port_detach.lport = lport;
  msg.u.port_detach.uport = uport;

  /* Post the event. */
  ret =  _hsl_bcm_if_post_event (&msg);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_FATAL, "Failed posting port dexttachment event for port %d\n", lport);
      /* XXX Should treat this seriously than we are. */
    }

  HSL_FN_EXIT (STATUS_OK);
}

/* Register callback for port creation. */
void
hsl_bcm_register_port_attach (void)
{
  bcmx_uport_create_callback_set (_hsl_bcm_port_attach_cb);
}

/* Unregister callback for port deletion. */
void
hsl_bcm_unregister_port_attach (void)
{
  bcmx_uport_create = NULL;
}

/* Register callback for port deletion. */
void
hsl_bcm_register_port_detach (void)
{
  bcmx_lport_remove_notify = _hsl_bcm_port_detach_cb;
}

/* Unregister callback for port deletion. */
void
hsl_bcm_unregister_port_detach (void)
{
  bcmx_lport_remove_notify = NULL;
}

/* Link scan callback. */
static void
_hsl_bcm_linkscan_cb (bcmx_lport_t lport, bcm_port_info_t *info)
{
  struct hsl_bcm_ifhdlr_msg msg;
  int ret;

  HSL_FN_ENTER ();

  msg.type = HSL_BCM_PORT_LINKSCAN_NOTIFICATION;
  msg.u.link_scan.lport = lport;
  memcpy (&msg.u.link_scan.info, info, sizeof (bcm_port_info_t));

  /* Post the event. */
  ret =  _hsl_bcm_if_post_event (&msg);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_FATAL, "Failed posting link scan message %d\n", lport);
      /* XXX Should treat this seriously than we are. */
    }

  HSL_FN_EXIT (ret);
}

/* Register callback for link scanning. */
int
hsl_bcm_register_link_scan (void)
{
  int ret;
  
  HSL_FN_ENTER ();

  ret = bcmx_linkscan_register (_hsl_bcm_linkscan_cb);

  HSL_FN_EXIT (ret);
}

/* Unregister callback for link scanning. */
int
hsl_bcm_unregister_link_scan ()
{
  int ret;

  HSL_FN_ENTER ();

  ret = bcmx_linkscan_unregister (_hsl_bcm_linkscan_cb);

  HSL_FN_EXIT (ret);
}

/* Background message processing. */
static int
_hsl_bcm_ifhdr_process_msg (struct hsl_bcm_ifhdlr_msg *msg)
{
  HSL_FN_ENTER ();

  switch (msg->type)
    {
    case HSL_BCM_PORT_ATTACH_NOTIFICATION:
      {
        /* If port is invalid, skip this port. */
        if (! (msg->u.port_attach.flags & BCMX_PORT_F_VALID))
          {
            HSL_FN_EXIT (0);
          }

        /* Process only data ports for interface manager. */
        if(!((BCMX_PORT_F_IS_STACK_PORT (msg->u.port_attach.flags))
           || (msg->u.port_attach.flags & BCMX_PORT_F_HG)
           || (msg->u.port_attach.flags & BCMX_PORT_F_CPU)))
          {
            /* Process port attachment. */
            hsl_bcm_ifcb_port_attach (msg->u.port_attach.lport,
                                      msg->u.port_attach.unit,
                                      msg->u.port_attach.port,
                                      msg->u.port_attach.flags);
          }
        else
          {
            HSL_FN_EXIT (0);
          }
      }
      break;
    case HSL_BCM_PORT_DETACH_NOTIFICATION:
      {
        /* Process only data ports for interface manager. */
        if(!((BCMX_PORT_F_IS_STACK_PORT (msg->u.port_attach.flags))
           || (msg->u.port_attach.flags & BCMX_PORT_F_HG)
           || (msg->u.port_attach.flags & BCMX_PORT_F_CPU)))
          {
            /* Process port detachment. */
            hsl_bcm_ifcb_port_detach (msg->u.port_detach.lport,
                                      msg->u.port_detach.uport);
          }
        else
          HSL_FN_EXIT (0);
      }
      break;
    case HSL_BCM_PORT_LINKSCAN_NOTIFICATION:
      {
        /* Process linkscan notification. */
        hsl_bcm_ifcb_link_scan (msg->u.link_scan.lport,
                                &msg->u.link_scan.info);
      }
      break;
    default:
      HSL_FN_EXIT (0);
    }

  HSL_FN_EXIT (0);
}

/* Interface handler task entry function. */
static void
_hsl_bcm_ifhdlr_func (void *param)
{
  struct hsl_bcm_ifhdlr_msg *msg;
  int spl;

  HSL_FN_ENTER ();

  while (! p_hsl_bcm_if_event_queue->thread_exit)
    {
      /* Service packets. */
      while (! HSL_BCM_IF_EVENT_QUEUE_EMPTY)
        {
          /* Set interrupt level to high. */
          spl = sal_splhi ();

          /* Get head event. */
          msg = (struct hsl_bcm_ifhdlr_msg *)&p_hsl_bcm_if_event_queue->queue[p_hsl_bcm_if_event_queue->head * aligned_sizeof_hsl_bcm_ifhdlr_msg];

          /* Increment head. */
          p_hsl_bcm_if_event_queue->head = HSL_BCM_IF_EVENT_QUEUE_NEXT (p_hsl_bcm_if_event_queue->head);

          /* Decrement count. */
          p_hsl_bcm_if_event_queue->count--;

          /* Unlock interrupt level. */
          sal_spl (spl);

          /* Process the link message. */
          _hsl_bcm_ifhdr_process_msg (msg);
        }

      oss_sem_lock (OSS_SEM_BINARY, p_hsl_bcm_if_event_queue->ifhdlr_sem, OSS_WAIT_FOREVER);
    }

  /* Exit packet thread. */
  oss_sem_delete (OSS_SEM_BINARY, p_hsl_bcm_if_event_queue->ifhdlr_sem);

  oss_free (p_hsl_bcm_if_event_queue, OSS_MEM_HEAP);
  p_hsl_bcm_if_event_queue = NULL;

  sal_thread_exit (0);
}

/* Initialize the interface handler. */
int
hsl_bcm_ifhdlr_init (void)
{
  int port_attach = 0, port_detach = 0, link_scan = 0;
  int total;
  int ret = 0;

  HSL_FN_ENTER ();

  /* Set the aligned size of struct hsl_bcm_ifhdlr_msg, so that we don;t calculate everytime. */
  aligned_sizeof_hsl_bcm_ifhdlr_msg = (((sizeof (struct hsl_bcm_ifhdlr_msg) * 3) / 4) * 4);

  if (! p_hsl_bcm_if_event_queue)
    {
      p_hsl_bcm_if_event_queue = oss_malloc (sizeof (struct hsl_bcm_if_event_queue), OSS_MEM_HEAP);
      if (! p_hsl_bcm_if_event_queue)
        {
          HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_FATAL, "Out of memory\n");
          ret = HSL_BCM_ERR_IFHDLR_NOMEM;
          goto ERR;
        }
    }

  /* Create interface event queue. */
  total = aligned_sizeof_hsl_bcm_ifhdlr_msg * HSL_BCM_IF_EVENT_QUEUE_SIZE;
  p_hsl_bcm_if_event_queue->queue = oss_malloc (total, OSS_MEM_HEAP);
  if (! p_hsl_bcm_if_event_queue->queue)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_FATAL, "Out of memory\n");
      ret = HSL_BCM_ERR_IFHDLR_NOMEM;
      goto ERR;   
    }

  /* Initialize the queue. */
  memset (p_hsl_bcm_if_event_queue->queue, 0, total);

  /* Initialize counters. */
  p_hsl_bcm_if_event_queue->total = HSL_BCM_IF_EVENT_QUEUE_SIZE;
  p_hsl_bcm_if_event_queue->count = 0;
  p_hsl_bcm_if_event_queue->head = 0;
  p_hsl_bcm_if_event_queue->tail = 0;
  p_hsl_bcm_if_event_queue->drop = 0;

  /* Initialize semaphore. */
  ret = oss_sem_new ("ifhdlr_event_queue_sem",
                     OSS_SEM_BINARY,
                     0,
                     NULL,
                     &p_hsl_bcm_if_event_queue->ifhdlr_sem);
  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_FATAL, "Failed creating interface event handler semaphore\n");
      ret = HSL_BCM_ERR_IFHDLR_NOMEM;
      goto ERR;
    }
  p_hsl_bcm_if_event_queue->thread_exit = 0;

  /* Create interface event dispather thread. */
  if ((p_hsl_bcm_if_event_queue->ifhdlr_thread = 
       sal_thread_create ("zBCMIFHDLR", 
                          SAL_THREAD_STKSZ, 
                          50,
                          _hsl_bcm_ifhdlr_func,
                          (void *)0)) == SAL_THREAD_ERROR)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_FATAL, "Cannot start interface event dispatcher thread\n");
      ret = HSL_BCM_ERR_IFHDLR_INIT;
      goto ERR;
    }
  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "---Start interface event dispatcher thread:zBCMIFHDLR (_hsl_bcm_ifhdlr_func)---\n");

  p_hsl_if_db->proc_if_create_cb = _module_create_proc_if;
  p_hsl_if_db->proc_if_remove_cb = _module_remove_proc_if;

  /* Register for port attachments. */
  hsl_bcm_register_port_attach ();
  port_attach = 1;

  /* Register for port detachments. */
  hsl_bcm_register_port_detach ();
  port_detach = 1;

  /* Register for link scanning. */
  ret = hsl_bcm_register_link_scan ();

  if (ret < 0)
    {
      HSL_LOG (HSL_LOG_IFMGR, HSL_LEVEL_FATAL, "Cannot start link scanning thread\n");
      ret = HSL_BCM_ERR_IFHDLR_INIT;
      goto ERR;
    }
  link_scan = 1;

  HSL_FN_EXIT (0);

 ERR:
  if (port_attach)
    hsl_bcm_unregister_port_attach ();
  if (port_detach)
    hsl_bcm_unregister_port_detach ();
  if (link_scan)
    hsl_bcm_unregister_link_scan ();
  if (p_hsl_bcm_if_event_queue)
    {
      if (p_hsl_bcm_if_event_queue->ifhdlr_thread)
        sal_thread_destroy (p_hsl_bcm_if_event_queue->ifhdlr_thread);

      if (p_hsl_bcm_if_event_queue->ifhdlr_sem)
        sal_sem_destroy (p_hsl_bcm_if_event_queue->ifhdlr_sem);
      
    }

  HSL_FN_EXIT (ret);
}

/* Deinitialize the interface handler. */
int
hsl_bcm_ifhdlr_deinit (void)
{
  HSL_FN_ENTER ();

  /* Unregister port attachment handler. */
  hsl_bcm_unregister_port_attach ();

  /* Unregister port detachment handler. */
  hsl_bcm_unregister_port_detach ();

  /* Unregister link scan handler. */
  hsl_bcm_unregister_link_scan ();

  if (p_hsl_bcm_if_event_queue)
    {
      p_hsl_bcm_if_event_queue->thread_exit = 1;

      sal_sem_give (p_hsl_bcm_if_event_queue->ifhdlr_sem);
    }

  HSL_FN_EXIT (0);
}
