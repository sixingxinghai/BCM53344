/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_NSM_DEBUG_H
#define _PACOS_NSM_DEBUG_H

#ifdef HAVE_BFD
/* NSM debug BFD flags. */
#define NSM_DEBUG_BFD          0x01
#endif /* HAVE_BFD */


/* NSM debug event flags. */
#define NSM_DEBUG_EVENT         0x01
#define NSM_DEBUG_EVENT_ALL     0x11

/* NSM debug packet flags. */
#define NSM_DEBUG_PACKET        0x01
#define NSM_DEBUG_SEND          0x20
#define NSM_DEBUG_RECV          0x40
#define NSM_DEBUG_DETAIL        0x80
#define NSM_DEBUG_PACKET_ALL    0xe1

/* NSM debug kernel flags. */
#define NSM_DEBUG_KERNEL        0x01
#define NSM_DEBUG_KERNEL_ALL    0x01

/* NSM debug high availibility flags. */
#define NSM_DEBUG_HA            0x01
#define NSM_DEBUG_HA_ALL        0x11

#ifdef HAVE_BFD
/* NSM debug BFD flags. */
#define NSM_DEBUG_BFD           0x01
#define NSM_DEBUG_BFD_ALL       0x11
#endif /* HAVE_BFD */

#define VRF_NAME(V)                                                           \
    ((V) != NULL && (V)->name != NULL ? (V)->name : "default")

#define NSM_VR_MASTER() ((struct nsm_master *)nzg->vr_in_cxt->proto)

#define NSM_VR_DEBUG() (NSM_VR_MASTER())->nm_debug

/* Debug related macros. */
#define TERM_DEBUG_ON(a, b)  \
              ((NSM_VR_DEBUG()).nmd_term.ndf_ ## a |= (NSM_DEBUG_ ## b))
#define TERM_DEBUG_OFF(a, b) \
              ((NSM_VR_DEBUG()).nmd_term.ndf_ ## a &= ~(NSM_DEBUG_ ## b))
#define CONF_DEBUG_ON(a, b)  \
              ((NSM_VR_DEBUG()).nmd_conf.ndf_ ## a |= (NSM_DEBUG_ ## b))
#define CONF_DEBUG_OFF(a, b) \
              ((NSM_VR_DEBUG()).nmd_conf.ndf_ ## a &= ~(NSM_DEBUG_ ## b))

#define DEBUG_ON(v, a, b, m)                                                  \
    do {                                                                      \
      if ((v)->mode == EXEC_MODE)                                             \
        {                                                                     \
          TERM_DEBUG_ON (a, b);                                               \
          cli_out ((v), m " debugging is on\n");                              \
        }                                                                     \
      else if ((v)->mode == CONFIG_MODE)                                      \
        {                                                                     \
          TERM_DEBUG_ON (a, b);                                               \
          CONF_DEBUG_ON (a, b);                                               \
        }                                                                     \
    } while (0)

#define DEBUG_OFF(v, a, b, m)                                                 \
    do {                                                                      \
      if ((v)->mode == EXEC_MODE)                                             \
        {                                                                     \
          TERM_DEBUG_OFF (a, b);                                              \
          cli_out (cli, m " debugging is off\n");                             \
        }                                                                     \
      else if ((v)->mode == CONFIG_MODE)                                      \
        {                                                                     \
          TERM_DEBUG_OFF (a, b);                                              \
          CONF_DEBUG_OFF (a, b);                                              \
        }                                                                     \
    } while (0)

#define TERM_DEBUG_FLAG_ON(a, f)  \
                        ((NSM_VR_DEBUG()).nmd_term.ndf_ ## a |= (f))
#define TERM_DEBUG_FLAG_OFF(a, f) \
                        ((NSM_VR_DEBUG()).nmd_term.ndf_ ## a &= ~(f))
#define CONF_DEBUG_FLAG_ON(a, f)  \
                        ((NSM_VR_DEBUG()).nmd_conf.ndf_ ## a |= (f))
#define CONF_DEBUG_FLAG_OFF(a, f) \
                        ((NSM_VR_DEBUG()).nmd_conf.ndf_ ## a &= ~(f))

#define DEBUG_PACKET_ON(v, f)                                                 \
    do {                                                                      \
      if ((v)->mode == EXEC_MODE)                                             \
        {                                                                     \
          TERM_DEBUG_FLAG_ON (packet, f);                                     \
        }                                                                     \
      else if ((v)->mode == CONFIG_MODE)                                      \
        {                                                                     \
          TERM_DEBUG_FLAG_ON (packet, f);                                     \
          CONF_DEBUG_FLAG_ON (packet, f);                                     \
        }                                                                     \
    } while (0)

#define DEBUG_PACKET_OFF(v, f)                                                \
    do {                                                                      \
      if ((v)->mode == EXEC_MODE)                                             \
        {                                                                     \
          (f) &= (NSM_VR_DEBUG()).nmd_term.ndf_packet;                                       \
          TERM_DEBUG_FLAG_OFF (packet, f);                                    \
        }                                                                     \
      else if ((v)->mode == CONFIG_MODE)                                      \
        {                                                                     \
          TERM_DEBUG_FLAG_OFF (packet, f);                                    \
          CONF_DEBUG_FLAG_OFF (packet, f);                                    \
        }                                                                     \
    } while (0)

#define NSM_DEBUG(a, b)         (NSM_VR_MASTER() ? ((NSM_VR_DEBUG()).nmd_term.ndf_ ## a & NSM_DEBUG_ ## b) : PAL_FALSE)
#define CONF_NSM_DEBUG(a, b)    (NSM_VR_MASTER() ? ((NSM_VR_DEBUG()).nmd_conf.ndf_ ## a & NSM_DEBUG_ ## b) : PAL_FALSE)

#ifdef HAVE_BFD
#define IS_NSM_DEBUG_BFD        NSM_DEBUG(bfd, BFD)
#endif /* HAVE_BFD */
#define IS_NSM_DEBUG_EVENT      NSM_DEBUG(event, EVENT)
#define IS_NSM_DEBUG_PACKET     NSM_DEBUG(packet, PACKET)
#define IS_NSM_DEBUG_SEND       NSM_DEBUG(packet, SEND)
#define IS_NSM_DEBUG_RECV       NSM_DEBUG(packet, RECV)
#define IS_NSM_DEBUG_DETAIL     NSM_DEBUG(packet, DETAIL)
#define IS_NSM_DEBUG_KERNEL     NSM_DEBUG(kernel, KERNEL)
#define IS_NSM_DEBUG_HA         NSM_DEBUG(ha, HA)
#define IS_NSM_DEBUG_HA_ALL     NSM_DEBUG(ha, HA_ALL)
#ifdef HAVE_BFD
#define IS_NSM_DEBUG_BFD        NSM_DEBUG(bfd, BFD)
#endif /* HAVE_BFd */


#endif /* _PACOS_NSM_DEBUG_H */

