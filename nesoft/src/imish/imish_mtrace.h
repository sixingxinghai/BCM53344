/* Copyright (C) 2005  All Rights Reserved.  */

#ifndef _PACOS_IMISH_MTRACE_H
#define _PACOS_IMISH_MTRACE_H

/* mtrace and mstat command options */
#define IMISH_MTRACE_CMD_STR                    "mtrace"
#define IMISH_MTRACE_DISPLAY_OPTION             "-s"
#define IMISH_MTRACE_VERBOSE_DISPLAY_OPTION     "-v"  
#define IMISH_MTRACE_TTL_OPTION                 "-t"  
#define IMISH_MTRACE_RESP_OPTION                "-r"  

#define IMISH_MTRACE_DEF_GRP                    "224.2.0.1"

#define IMISH_MTRACE_CMD_OPTIONS(s, argv, i)                                  \
do                                                                            \
{                                                                             \
  if (!pal_strncmp ((s), IMISH_MTRACE_CMD_STR, sizeof IMISH_MTRACE_CMD_STR))  \
    (argv)[(i)++] = IMISH_MTRACE_DISPLAY_OPTION;                              \
  else                                                                        \
    (argv)[(i)++] = IMISH_MTRACE_VERBOSE_DISPLAY_OPTION;                      \
} while(0)

#define IMISH_MTRACE_GET_IPV4_ADDRESS(NAME,V,STR)                             \
    do {                                                                      \
      int retv;                                                               \
      retv = pal_inet_pton (AF_INET, (STR), &(V));                            \
      if (!retv)                                                              \
        {                                                                     \
          printf ("%% Invalid %s value\n", NAME);                       \
          return CLI_ERROR;                                                   \
        }                                                                     \
    } while (0)

#define IMISH_MTRACE_GET_UINT32_RANGE(NAME,V,STR,MIN,MAX)                     \
    do {                                                                      \
      char *endptr = NULL;                                                    \
      (V) = pal_strtou32 ((STR), &endptr, 10);                                \
      if (*endptr != '\0' || (V) < (u_int32_t)(MIN)                           \
          || (V) > (u_int32_t)(MAX))                                          \
        {                                                                     \
          printf ("%% Invalid %s value\n", NAME);                       \
          return CLI_ERROR;                                                   \
        }                                                                     \
    } while (0)

#endif /* _PACOS_IMISH_MTRACE_H */
