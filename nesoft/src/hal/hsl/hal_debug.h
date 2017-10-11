/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_DEBUG_H
#define _HAL_DEBUG_H

/* HAL debug event flags. */
#define HAL_DEBUG_EVENT         0x01

/* Debug related macros. */
#define TERM_DEBUG_ON(a, b)     (term_hal_debug_ ## a |= (HAL_DEBUG_ ## b))
#define TERM_DEBUG_OFF(a, b)    (term_hal_debug_ ## a &= ~(HAL_DEBUG_ ## b))
#define CONF_DEBUG_ON(a, b)     (conf_hal_debug_ ## a |= (HAL_DEBUG_ ## b))
#define CONF_DEBUG_OFF(a, b)    (conf_hal_debug_ ## a &= ~(HAL_DEBUG_ ## b))

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

#define TERM_DEBUG_FLAG_ON(a, f)                (term_hal_debug_ ## a |= (f))
#define TERM_DEBUG_FLAG_OFF(a, f)               (term_hal_debug_ ## a &= ~(f))
#define CONF_DEBUG_FLAG_ON(a, f)                (conf_hal_debug_ ## a |= (f))
#define CONF_DEBUG_FLAG_OFF(a, f)               (conf_hal_debug_ ## a &= ~(f))

#define HAL_DEBUG(a, b)         (term_hal_debug_ ## a & HAL_DEBUG_ ## b) 
#define CONF_HAL_DEBUG(a, b)    (conf_hal_debug_ ## a & HAL_DEBUG_ ## b)

#define IS_HAL_DEBUG_EVENT      HAL_DEBUG (event, EVENT)

extern u_int32_t term_hal_debug_event;
extern u_int32_t term_hal_debug_packet;
extern u_int32_t term_hal_debug_kernel;

extern u_int32_t conf_hal_debug_event;
extern u_int32_t conf_hal_debug_packet;
extern u_int32_t conf_hal_debug_kernel;

void hal_debug_init (void);
void hal_debug_cli_init (struct lib_globals *zg);

#endif /* _HAL_DEBUG_H */
