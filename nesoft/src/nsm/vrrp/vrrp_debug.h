/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_VRRP_DEBUG_H
#define _PACOS_VRRP_DEBUG_H

/* VRRP debug flags. */
#define VRRP_DEBUG_EVENT            0x01
#define VRRP_DEBUG_EVENT_ALL        0x01

#define VRRP_DEBUG_PACKET           0x10
#define VRRP_DEBUG_SEND             0x20
#define VRRP_DEBUG_RECV             0x40
#define VRRP_DEBUG_PACKET_ALL       0x70

#define VRRP_TERM_DEBUG_ON(F)                (vrrp_term_debug |= (F))
#define VRRP_TERM_DEBUG_OFF(F)               (vrrp_term_debug &= ~(F))
#define VRRP_CONF_DEBUG_ON(F)                (vrrp_conf_debug |= (F))
#define VRRP_CONF_DEBUG_OFF(F)               (vrrp_conf_debug &= ~(F))

/* Setting multiple debug packet flags. */

#define VRRP_DEBUG_PACKET_ON(v, mf)                                           \
    do {                                                                      \
      if ((v)->mode == EXEC_MODE)                                             \
        {                                                                     \
          VRRP_TERM_DEBUG_ON (mf);                                            \
        }                                                                     \
      else if ((v)->mode == CONFIG_MODE)                                      \
        {                                                                     \
          VRRP_TERM_DEBUG_ON (mf);                                            \
          VRRP_CONF_DEBUG_ON (mf);                                            \
        }                                                                     \
    } while (0)

/* Clearing multiple debug packet flags. */

#define VRRP_DEBUG_PACKET_OFF(v, mf)                                          \
    do {                                                                      \
      if ((v)->mode == EXEC_MODE)                                             \
        {                                                                     \
          (mf) &= vrrp_term_debug;                                            \
          VRRP_TERM_DEBUG_OFF (mf);                                           \
        }                                                                     \
      else if ((v)->mode == CONFIG_MODE)                                      \
        {                                                                     \
          VRRP_TERM_DEBUG_OFF (mf);                                           \
          VRRP_CONF_DEBUG_OFF (mf);                                           \
        }                                                                     \
    } while (0)

/* Setting a single debug flag. */

#define VRRP_DEBUG_ON(v, f)                                                   \
    do {                                                                      \
      if ((v)->mode == EXEC_MODE)                                             \
        {                                                                     \
          VRRP_TERM_DEBUG_ON (VRRP_DEBUG_ ## f);                              \
        }                                                                     \
      else if ((v)->mode == CONFIG_MODE)                                      \
        {                                                                     \
          VRRP_TERM_DEBUG_ON (VRRP_DEBUG_ ## f);                              \
          VRRP_CONF_DEBUG_ON (VRRP_DEBUG_ ## f);                              \
        }                                                                     \
    } while (0)

/* Unsetting a single debug flag. */

#define VRRP_DEBUG_OFF(v, f)                                                  \
    do {                                                                      \
      if ((v)->mode == EXEC_MODE)                                             \
        {                                                                     \
          VRRP_TERM_DEBUG_OFF (VRRP_DEBUG_ ## f);                             \
        }                                                                     \
      else if ((v)->mode == CONFIG_MODE)                                      \
        {                                                                     \
          VRRP_TERM_DEBUG_OFF (VRRP_DEBUG_ ## f);                             \
          VRRP_CONF_DEBUG_OFF (VRRP_DEBUG_ ## f);                             \
        }                                                                     \
    } while (0)

#define VRRP_DEBUG(f)          (vrrp_term_debug & VRRP_DEBUG_ ## f)
#define VRRP_CONF_DEBUG(f)     (vrrp_conf_debug & VRRP_DEBUG_ ## f)

#define IS_DEBUG_VRRP_EVENT             VRRP_DEBUG (EVENT)
#define IS_DEBUG_VRRP_PACKET            VRRP_DEBUG (PACKET)
#define IS_DEBUG_VRRP_SEND              VRRP_DEBUG (SEND)
#define IS_DEBUG_VRRP_RECV              VRRP_DEBUG (RECV)

extern u_int32_t vrrp_term_debug;
extern u_int32_t vrrp_conf_debug;

void vrrp_debug_init (void);

#endif /* _PACOS_VRRP_DEBUG_H */
