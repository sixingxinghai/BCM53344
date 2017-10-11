/* Copyright (C) 2003  All Rights Reserved. */

/*
        This modules contains the LAYER2 debug macros

*/

#ifndef _PACOS_L2_DEBUG_H
#define _PACOS_L2_DEBUG_H

/* STP debug event flags. */

/* Slow Timeouts */
#define LAYER2_DEBUG_TIMER          0x01
/* Hello Timeouts */
#define LAYER2_DEBUG_TIMER_DETAIL   0x02
/* State and role changes */
#define LAYER2_DEBUG_PROTO          0x04
/* All other protocol details */
#define LAYER2_DEBUG_PROTO_DETAIL   0x08
/* Echo CLI commands */
#define LAYER2_DEBUG_CLI_ECHO       0x10
/* Packet traces (directional) */
#define LAYER2_DEBUG_PACKET_TX      0x20
#define LAYER2_DEBUG_PACKET_RX      0x40
#define LAYER2_DEBUG_EVENT          0x80
#define LAYER2_DEBUG_HA            0x100


extern u_int32_t conf_layer2_debug_proto;
extern u_int32_t term_layer2_debug_proto;

/* Debug related macro. */
#define CONF_DEBUG_ON(a, b)     (conf_layer2_debug_ ## a |= (LAYER2_DEBUG_ ## b))
#define CONF_DEBUG_OFF(a, b)    (conf_layer2_debug_ ## a &= ~(LAYER2_DEBUG_ ## b))

#define TERM_DEBUG_ON(a, b)     (term_layer2_debug_ ## a |= (LAYER2_DEBUG_ ## b))
#define TERM_DEBUG_OFF(a, b)    (term_layer2_debug_ ## a &= ~(LAYER2_DEBUG_ ## b))

#define DEBUG_ON(v, a, b)                                                  \
    {                                                                      \
      if ((v)->mode == EXEC_MODE)                                             \
        {                                                                     \
          TERM_DEBUG_ON (a, b);                                               \
        }                                                                     \
      else if ((v)->mode == CONFIG_MODE)                                      \
        {                                                                     \
          TERM_DEBUG_ON (a, b);                                               \
          CONF_DEBUG_ON (a, b);                                               \
        }                                                                     \
    } 

#define DEBUG_OFF(a, b)                                                 \
    do {                                                                    \
          TERM_DEBUG_OFF (a, b);                                              \
          CONF_DEBUG_OFF (a, b);                                              \
    } while (0)

#define LAYER2_DEBUG(a, b)              (term_layer2_debug_ ## a & LAYER2_DEBUG_ ## b) 
#define CONF_LAYER2_DEBUG(a, b) (conf_layer2_debug_ ## a & LAYER2_DEBUG_ ## b)

void l2_debug_init (void);
void l2_debug_reset (void);

#endif /* _PACOS_L2_DEBUG_H */
