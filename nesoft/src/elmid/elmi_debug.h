/* Copyright (C) 2010  All Rights Reserved. */

#ifndef _PACOS_ELMI_DEBUG_H
#define _PACOS_ELMI_DEBUG_H

#define ELMI_DEBUG_EVENT           0x01
#define ELMI_DEBUG_EVENT_ALL       0x01
#define ELMI_DEBUG_TIMER           0x02
#define ELMI_DEBUG_PROTOCOL        0x04
#define ELMI_DEBUG_NSM             0x08
#define ELMI_DEBUG_PACKET          0x10
#define ELMI_DEBUG_TX              0x20
#define ELMI_DEBUG_RX              0x40
#define ELMI_DEBUG_DETAIL          0x80
#define ELMI_DEBUG_PACKET_ALL      0xe1

/* Debug related macros. */
#define ELMI_TERM_DEBUG_ON(a, b) (em->debug.term.a |= (ELMI_DEBUG_ ## b))
#define ELMI_TERM_DEBUG_OFF(a, b) (em->debug.term.a &= ~(ELMI_DEBUG_ ## b))
#define ELMI_CONF_DEBUG_ON(a, b) (em->debug.conf.a |= (ELMI_DEBUG_ ## b))
#define ELMI_CONF_DEBUG_OFF(a, b)  (em->debug.conf.a &= ~(ELMI_DEBUG_ ## b))

#define ELMI_DEBUG_ON(v, a, b, m)                             \
  do {                                                        \
      if ((v)->mode == EXEC_MODE)                             \
        {                                                     \
          ELMI_TERM_DEBUG_ON (a, b);                          \
          cli_out ((v), m " debugging is on\n");              \
        }                                                     \
      else if ((v)->mode == CONFIG_MODE)                      \
        {                                                     \
          ELMI_TERM_DEBUG_ON (a, b);                          \
          ELMI_CONF_DEBUG_ON (a, b);                           \
        }                                                     \
  } while (0)

#define ELMI_DEBUG_OFF(v, a, b, m)                                            \
  do {                                                                        \
      if ((v)->mode == EXEC_MODE)                                             \
        {                                                                     \
          ELMI_TERM_DEBUG_OFF (a, b);                                         \
          cli_out (cli, m " debugging is off\n");                             \
        }                                                                     \
      else if ((v)->mode == CONFIG_MODE)                                      \
        {                                                                     \
          ELMI_TERM_DEBUG_OFF (a, b);                                        \
          ELMI_CONF_DEBUG_OFF (a, b);                                          \
        }                                                                     \
  } while (0)

#define TERM_DEBUG_FLAG_ON(a, f)    (em->debug.term.a |= (f))
#define TERM_DEBUG_FLAG_OFF(a, f)   (em->debug.term &= ~(f))
#define CONF_DEBUG_FLAG_ON(a, f)    (em->debug.conf.a |= (f))
#define CONF_DEBUG_FLAG_OFF(a, f)   (em->debug.conf.a &= ~(f))

#define DEBUG_PACKET_ON(v, f)                                                 \
    do {                                                                      \
      if ((v)->mode == EXEC_MODE)                                             \
        {                                                                     \
          TERM_DEBUG_FLAG_ON (packet, f);                                     \
          if (CHECK_FLAG (f, RIP_DEBUG_TX)                                  \
              && CHECK_FLAG (f, RIP_DEBUG_RX))                              \
            cli_out ((v), "ELMI packet%s debugging is on\n",                   \
                     CHECK_FLAG (f, ELMI_DEBUG_DETAIL) ? " detail" : "");      \
          else if (CHECK_FLAG (f, ELMI_DEBUG_TX))                            \
            cli_out ((v), "ELMI send packet%s debugging is on\n",              \
                     CHECK_FLAG (f, ELMI_DEBUG_DETAIL) ? " detail" : "");      \
          else if (CHECK_FLAG (f, ELMI_DEBUG_RX))                            \
            cli_out ((v), "ELMI packet receive%s debugging is on\n",           \
                     CHECK_FLAG (f, ELMI_DEBUG_DETAIL) ? " detail" : "");      \
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
          (f) &= em->debug.term.packet;                                       \
          TERM_DEBUG_FLAG_OFF (packet, f);                                    \
          if (CHECK_FLAG (f, ELMI_DEBUG_TX)                                  \
              && CHECK_FLAG (f, ELMI_DEBUG_RX))                              \
            cli_out ((v), "ELMI packet%s debugging is off\n",                  \
                     CHECK_FLAG (f, ELMI_DEBUG_DETAIL) ? " detail" : "");      \
          else if (CHECK_FLAG (f, ELMI_DEBUG_SEND))                            \
            cli_out ((v), "ELMI send packet%s debugging is off\n",             \
                     CHECK_FLAG (f, ELMI_DEBUG_DETAIL) ? " detail" : "");      \
          else if (CHECK_FLAG (f, ELMI_DEBUG_RECV))                            \
            cli_out ((v), "ELMI packet receive%s debugging is off\n",          \
                     CHECK_FLAG (f, ELMI_DEBUG_DETAIL) ? " detail" : "");      \
          else if (CHECK_FLAG (f, ELMI_DEBUG_DETAIL))                          \
            cli_out ((v), "ELMI packet detail debugging is off\n");            \
        }                                                                     \
      else if ((v)->mode == CONFIG_MODE)                                      \
        {                                                                     \
          TERM_DEBUG_FLAG_OFF (packet, f);                                    \
          CONF_DEBUG_FLAG_OFF (packet, f);                                    \
        }                                                                     \
    } while (0)

#define ELMI_DEBUG(a, b)   (em->debug.term.a & ELMI_DEBUG_ ## b) 
#define CONF_ELMI_DEBUG(a, b)  (em->debug.conf.a & ELMI_DEBUG_ ## b)

void 
elmi_debug_init (struct elmi_master *);

#endif /* _PACOS_ELMI_DEBUG_H */

