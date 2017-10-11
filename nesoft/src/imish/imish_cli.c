//** Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "pal_ping.h"
#include "pal_traceroute.h"

#include "cli.h"
#include "line.h"
#include "message.h"
#include "imi_client.h"
#include "modbmap.h"
#include "libedit/readline/readline.h"

#include "imish/imish.h"
#include "imish/imish_exec.h"
#include "imish/imish_cli.h"
#include "imish/imish_parser.h"
#include "imish/imish_system.h"
#include "imish/imish_line.h"
#include "imish/imish_readline.h"
#include "lib/cli.h"
#ifdef HAVE_MCAST_IPV4
#include "imish/imish_mtrace.h"
#endif /* HAVE_MCAST_IPV4 */

#define VRF_FIB_ID_MAX_LEN 5
char vrf_fib_id[VRF_FIB_ID_MAX_LEN];

struct cli_tree *ctree;

/* Extracted command initialization.  */
void imi_extracted_cmd_init (struct cli_tree *);
int cli_config_exit (struct cli *, int, char **);

#ifdef HAVE_VR
void imish_vr_cli_init (struct cli_tree *);
#endif /* HAVE_VR */


/* Utility function for asking user about yes or no.  */
int
imish_yes_or_no (char *str)
{
  char *line = NULL;

  line = imish_get_y_or_n (str);

  if (! line)
    return -1;

  SKIP_WHITE_SPACE (line);
  if (pal_strcmp (line, "y") == 0 || pal_strcmp (line, "Y") == 0)
    return 1;

  return 0;
}

/* Get hex pattern for input string.  Input string will contain
   0xABCD, output will be ABCD.  Returns -1 for non-hex patterns in
   input. */
static int
get_hex_pattern (char *input, char *output)
{
  char *cp = input;
  int i, j = 0;

  SKIP_WHITE_SPACE(cp);
  if (cp[0] == '0')
    {
      cp++;
      SKIP_WHITE_SPACE(cp);

      if (cp[0] == 'x')
        {
          cp++;

          for (i = 0; i < pal_strlen(cp); i++)
            {
              if (pal_char_isspace ((int)(cp[i])))
                continue;

              if ((cp[i] >= '0' && cp[i] <= '9')
                  || (cp[i] >= 'A' && cp[i] <= 'F')
                  || (cp[i] >= 'a' && cp[i] <= 'f'))
                output[j++] = cp[i];
              else
                return -1;
            }
          output[j] = '\0';
          return 0;
        }
      else
        return -1;
    }
  else
    return -1;

  return 0;
}

int
check_loopback_address (int argc, char **argv, struct cli *cli)
{
  int addr_count;
  struct pal_in4_addr p;

  pal_mem_set (&p, 0, sizeof(struct pal_in4_addr));

  for (addr_count=0; addr_count < argc; addr_count++)
     {
       if (strcmp (argv[addr_count],"destination") == 0)
         {
           addr_count++;
           CLI_GET_IPV4_ADDRESS("Destination", p, argv[addr_count]);
           if (!IN_LOOPBACK (pal_hton32(p.s_addr)))
             {
               cli_out (cli, "%%Destination should be in range 127/8 (127.x.x.x) \n");
               return CLI_ERROR;
             }
         }
     }
  return CLI_SUCCESS;
}


/* Do DNS resolution. 0 for success, < 0 for failure. */
static int
get_addr_by_dns (int family, char *host, char *ipaddr, int size)
{
  int ret;
  struct pal_addrinfo *res, hints;
  struct pal_sockaddr_in4 *addr;
#ifdef HAVE_IPV6
  struct pal_sockaddr_in6 *addr6;
#endif /* HAVE_IPV6 */

  if (family != AF_INET
#ifdef HAVE_IPV6
      && family != AF_INET6
#endif /* HAVE_IPV6 */
      )
    return -1;

  pal_mem_set (&hints, 0, sizeof(struct pal_addrinfo));
  hints.ai_family = family;
  ret = pal_sock_getaddrinfo (host, NULL, &hints, &res);
  if (!ret)
    {
      /* Consider the first entry only. */

      if (family == AF_INET)
        {
          addr = (struct pal_sockaddr_in4 *)res->ai_addr;
          pal_inet_ntop (family, &addr->sin_addr.s_addr, ipaddr, size);
          pal_sock_freeaddrinfo (res);
          return 0;
        }
#ifdef HAVE_IPV6
      else if (family == AF_INET6)
        {
          addr6 = (struct pal_sockaddr_in6 *)res->ai_addr;
          pal_inet_ntop (family, &addr6->sin6_addr.s6_addr, ipaddr, size);
          pal_sock_freeaddrinfo (res);
          return 0;
        }
#endif /* HAVE_IPV6 */
      else
        {
          pal_sock_freeaddrinfo (res);
          return -1;
        }
    }

  return -1;
}

/* Interactive mode of ping.  */
int
imish_interactive_ping ()
{
  char *line = NULL;
  char *argv[25];
  int i = 1;
  int type = -1;
#ifdef HAVE_IPV6
#define IPADDR_SIZE            INET6_ADDRSTRLEN
#else
#define IPADDR_SIZE            32
#endif /* HAVE_IPV6 */

  char ipaddr_buf[IPADDR_SIZE];
#define DATAGRAM_SIZE          10
  char dgsize_buf[DATAGRAM_SIZE];
#define RPTCNT_SIZE            15
  char rptcnt_buf[RPTCNT_SIZE];
#define TIMEOUT_SIZE           5
  char timeout_buf[TIMEOUT_SIZE];
#define TOS_SIZE               5
  char tos_buf[TOS_SIZE];
  char ifname[IPADDR_SIZE];
#define PATTERN_SIZE           256
  char hexbuf[256];
  int done;
  int ret;
  int family = 0;
#ifdef HAVE_IPV6
  struct prefix_ipv6 p;
#endif /* HAVE_IPV6. */
  int argco;
  char **argvo;
  struct imi_client *ic = imishm->imh->info;
  char *name = NULL;

  /* For getopt to work. */
  argv[0] = "ping";

  /* Process protocol. */
  line = imish_gets ("Protocol [ip]: ");
  SKIP_WHITE_SPACE(line);
  if (*line == '\0')
    {
      argv[i++] = PING_FAMILY_OPTION;
      argv[i++] = PING_FAMILY_IPV4;
      type = PING_IPV4;
    }
  else
    {
      SKIP_WHITE_SPACE(line);

#ifdef HAVE_IPV6
      if (! pal_strncmp (line, "ipv6", 4))
        {
          argv[i++] = PING_FAMILY_OPTION;
          argv[i++] = PING_FAMILY_IPV6;
          type = PING_IPV6;
        }
      else
#endif /* HAVE_IPV6 */
        if (! pal_strncmp (line, "ip", 2))
          {
            argv[i++] = PING_FAMILY_OPTION;
            argv[i++] = PING_FAMILY_IPV4;
            type = PING_IPV4;
          }
        else
          {
            printf ("%% Unknown protocol - \"%s\", type \"ping ?\" for help\n", line);
            return -1;
          }
    }

  /* Process Target IP address. */
  line = imish_gets ("Target IP address: ");
  SKIP_WHITE_SPACE(line);
  if (*line == '\0')
    {
      printf ("%% Bad IP address or hostname\n");
      return -1;
    }
  else
    {
      SKIP_WHITE_SPACE(line);

      if (type == PING_IPV4)
        family = AF_INET;
#ifdef HAVE_IPV6
      else if (type == PING_IPV6)
        family = AF_INET6;
#endif /* HAVE_IPV6 */

      /* Try DNS resolution. */
      ret = get_addr_by_dns (family, line, ipaddr_buf, IPADDR_SIZE);
      if (ret < 0)
        {
          printf ("%% Bad IP address or hostname\n");
          return -1;
        }

      argv [i++] = PING_ADDRESS_OPTION;
      argv [i++] = ipaddr_buf;
    }

#ifdef HAVE_VRF
  line = imish_gets ("Name of the VRF : ");
  SKIP_WHITE_SPACE(line);
  name = line;
#endif
  if (ic)
    imish_line_sync_vrf (&ic->line, name);

  /* Process Repeat count. */
  done = 0;
  while (!done)
    {
      line = imish_gets ("Repeat count [5]: ");
      SKIP_WHITE_SPACE(line);
      if (*line == '\0')
        {
            argv[i++] = PING_REPEAT_COUNT_OPTION;

          pal_snprintf (rptcnt_buf, RPTCNT_SIZE, "%d", PING_DEFAULT_RPTCNT);
          argv[i++] = rptcnt_buf;

          done = 1;
        }
      else
        {
          int rptcnt;

          SKIP_WHITE_SPACE(line);

          rptcnt = cmd_str2int (line, &ret);
          if (ret < 0 || rptcnt < 1 || rptcnt > 2147483647)
            {
              printf ("%% A decimal number between 1 and 2147483647.\n");
              continue;
            }

            argv[i++] = PING_REPEAT_COUNT_OPTION;

          pal_snprintf (rptcnt_buf, RPTCNT_SIZE, "%d", rptcnt);
          argv[i++] = rptcnt_buf;

          done = 1;
        }
    }

  /* Process datagram size. */
  done = 0;
  while (!done)
    {
      line = imish_gets ("Datagram size [100]: ");
      SKIP_WHITE_SPACE(line);
      if (*line == '\0')
        {
            argv[i++] = PING_DATAGRAM_SIZE_OPTION;

          pal_snprintf (dgsize_buf, DATAGRAM_SIZE, "%d", PING_DEFAULT_DATAGRAM_SIZE);
          argv[i++] = dgsize_buf;

          done = 1;
        }
      else
        {
          int dg;

          SKIP_WHITE_SPACE(line);

          dg = cmd_str2int (line, &ret);
          if (ret < 0 || dg < 36 || dg > 18024)
            {
              printf ("%% A decimal number between 36 and 18024.\n");
              continue;
            }

            argv[i++] = PING_DATAGRAM_SIZE_OPTION;

          pal_snprintf (dgsize_buf, DATAGRAM_SIZE, "%d", dg);
          argv[i++] = dgsize_buf;

          done = 1;
        }
    }

  /* Process timeout. */
  done = 0;
  while (!done)
    {
      line = imish_gets ("Timeout in seconds [2]: ");
      SKIP_WHITE_SPACE(line);
      if (*line == '\0')
        {
            argv[i++] = PING_TIMEOUT_OPTION;

          pal_snprintf (timeout_buf, TIMEOUT_SIZE, "%d", PING_DEFAULT_TIMEOUT);
          argv[i++] = timeout_buf;

          done = 1;
        }
      else
        {
          int timeout;

          SKIP_WHITE_SPACE(line);

          timeout = cmd_str2int (line, &ret);
          if (ret < 0 || timeout < 0 || timeout > 3600)
            {
              printf ("%% A decimal number between 0 and 3600.\n");
              continue;
            }

            argv[i++] = PING_TIMEOUT_OPTION;

          pal_snprintf (timeout_buf, TIMEOUT_SIZE, "%d", timeout);
          argv[i++] = timeout_buf;

          done = 1;
        }
    }

  /* Process extended commands. */
  line = imish_gets ("Extended commands [n]: ");
  SKIP_WHITE_SPACE(line);
  if (*line != '\0')
    {
      SKIP_WHITE_SPACE(line);

      /* Process extended commands. */
      if (line[0] == 'y')
        {
          /* Process extended source address or interface. */
          line = imish_gets ("Source address or interface: ");
          SKIP_WHITE_SPACE(line);
          if (*line != '\0')
            {
              SKIP_WHITE_SPACE(line);

              /* Try it as a interface name. */

              if (pal_strlen (line) < INTERFACE_NAMSIZ)
                {
                    argv[i++] = PING_INTERFACE_OPTION;

                  pal_strcpy (ifname, line);
                  argv[i++] = ifname;
                }
            }

          /* Process extended type of service. */
          line = imish_gets ("Type of service [0]: ");
          SKIP_WHITE_SPACE(line);
          if (*line != '\0')
            {
              int tos;

              SKIP_WHITE_SPACE(line);

              tos = cmd_str2int (line, &ret);
              if (ret >= 0)
                {
                  pal_snprintf (tos_buf, TOS_SIZE, "%d", tos);

                    argv[i++] = PING_TOS_OPTION;

                  argv[i++] = tos_buf;
                }
            }

          /* Process extended DF bit. */
          done = 0;
          while (!done)
            {
              line = imish_gets ("Set DF bit in IP header? [no]: ");
              SKIP_WHITE_SPACE(line);
              if (*line != '\0')
                {
                      SKIP_WHITE_SPACE(line);

                      if (line[0] == 'y')
                        {
                          argv[i++] = PING_DF_OPTION;
                          argv[i++] = PING_DF_OPTION_YES;
                          done = 1;
                        }
                      else if (line[0] == 'n')
                        {
                          argv[i++] = PING_DF_OPTION;
                          argv[i++] = PING_DF_OPTION_NO;
                          done = 1;
                        }
                      else
                        printf ("%% Please answer 'yes' or 'no'.\n");
                }
              else
                {
                  argv[i++] = PING_DF_OPTION;
                  argv[i++] = PING_DF_OPTION_NO;
                  done = 1;
                }
            }

          /* Process extended validate reply. */
          /* Not supported by ping/ping6. */

          /* Process extended data pattern. */
          done = 0;
          while (!done)
            {
              line = imish_gets ("Data pattern [0xABCD]: ");
              SKIP_WHITE_SPACE(line);
              if (*line != '\0')
                {
                  SKIP_WHITE_SPACE(line);

                  if (pal_strlen(line) > PATTERN_SIZE)
                    continue;

                  ret = get_hex_pattern(line, hexbuf);
                  if (ret < 0 || pal_strlen(hexbuf) > 4)
                    {
                      printf ("Invalid pattern, try again.\n");
                      continue;
                    }

                    argv[i++] = PING_PATTERN_OPTION;

                  argv[i++] = hexbuf;

                  done = 1;
                }
              else
                {
                    argv[i++] = PING_PATTERN_OPTION;

                  argv[i++] = "ABCD";
                  done = 1;
                }
            }

          /* Process extended
             Loose, Strict, Record, Timestamp, Verbose[none] */
        }
    }

#ifdef HAVE_IPV6
  /* For link-local addresses accept interface parameter. */
  ret = str2prefix_ipv6 (ipaddr_buf, &p);
  if (ret > 0)
    {
      if (IN6_IS_ADDR_LINKLOCAL(&p.prefix))
        {
          /* Get interface name. */
          line = imish_gets ("Output Interface: ");
          SKIP_WHITE_SPACE(line);
          if (*line == '\0')
            {
              printf ("%% Interface required\n");
              return -1;
            }
          else
            {
              argv[i++] = PING_INTERFACE_OPTION;
              argv[i++] = line;
            }
        }
    }
#endif /* HAVE_IPV6. */

  argv[i] = NULL;

  /* Map ping options. */
  pal_ping (i, argv, &argco, &argvo, vrf_fib_id);

  /* Execute command. */
  imish_execvp (argvo);

  /* Free argvo */
  XFREE (MTYPE_TMP, argvo);

  return CLI_SUCCESS;
}

/* Interactive mode of traceroute.  */
int
imish_interactive_traceroute ()
{
  char *line = NULL;
  char *argv[25];
  int i = 0;
  int type = -1;
#ifdef HAVE_IPV6
#define IPADDR_SIZE            INET6_ADDRSTRLEN
#else
#define IPADDR_SIZE            32
#endif /* HAVE_IPV6 */

  char src_ipaddr_buf[IPADDR_SIZE];
  char dst_ipaddr_buf[IPADDR_SIZE];

#define PROBECNT_SIZE          10
  char probecnt_buf[PROBECNT_SIZE];
#define TIMEOUT_SIZE           5
  char timeout_buf[TIMEOUT_SIZE];
#define TTL_SIZE               4
  char ttl_buf[TTL_SIZE];
#define PORT_SIZE              6
  char port_buf[PORT_SIZE];
  int done;
  int ret;
  int family = 0;
  int argco;
  char **argvo;
  struct imi_client *ic = imishm->imh->info;
  char *name = NULL;

  /* For getopt to work. */
  argv[i++] = "traceroute";

  /* Process protocol. */
  line = imish_gets ("Protocol [ip]: ");
  SKIP_WHITE_SPACE(line);
  if (*line == '\0')
    {
      argv[i++] = TRACEROUTE_FAMILY_OPTION;
      argv[i++] = TRACEROUTE_FAMILY_IPV4;
      type = TRACEROUTE_IPV4;
    }
  else
    {
      SKIP_WHITE_SPACE(line);

#ifdef HAVE_IPV6
      if (! pal_strncmp (line, "ipv6", 4))
        {
          argv[i++] = TRACEROUTE_FAMILY_OPTION;
          argv[i++] = TRACEROUTE_FAMILY_IPV6;
          type = TRACEROUTE_IPV6;
        }
      else
#endif /* HAVE_IPV6 */
        if (! pal_strncmp (line, "ip", 2))
          {
            argv[i++] = TRACEROUTE_FAMILY_OPTION;
            argv[i++] = TRACEROUTE_FAMILY_IPV4;
            type = TRACEROUTE_IPV4;
          }
        else
          {
            printf ("%% Unknown protocol - \"%s\", type \"trace ?\" for help\n", line);
            return -1;
          }
    }

  /* Process Target IP address. */
  line = imish_gets ("Target IP address: ");
  SKIP_WHITE_SPACE(line);
  if (*line == '\0')
    {
      printf ("%% Bad IP address or hostname\n");
      return -1;
    }
  else
    {
      SKIP_WHITE_SPACE(line);

      if (type == TRACEROUTE_IPV4)
        family = AF_INET;
#ifdef HAVE_IPV6
      else if (type == TRACEROUTE_IPV6)
        family = AF_INET6;
#endif /* HAVE_IPV6 */

      /* Try DNS resolution. */
      ret = get_addr_by_dns (family, line, dst_ipaddr_buf, IPADDR_SIZE);
      if (ret < 0)
        {
          printf ("%% Bad IP address or hostname\n");
          return -1;
        }

      argv [i++] = TRACEROUTE_ADDRESS_OPTION;
      argv [i++] = dst_ipaddr_buf;
    }

  /* Process source address. */
  line = imish_gets ("Source address: ");
  SKIP_WHITE_SPACE(line);
  if (*line != '\0')
    {
      struct prefix_ipv4 cp;
#ifdef HAVE_IPV6
      struct prefix_ipv6 p;
#endif /* HAVE_IPV6 */

      SKIP_WHITE_SPACE(line);

      if (type == TRACEROUTE_IPV4)
        ret = str2prefix_ipv4 (line, &cp);
#ifdef HAVE_IPV6
      else if (type == TRACEROUTE_IPV6)
        ret = str2prefix_ipv6 (line, &p);
#endif /* HAVE_IPV6 */

      if (ret <= 0)
        {
          printf ("%% Invalid source address\n");
          return -1;
        }

      argv[i++] = TRACEROUTE_SRC_ADDR_OPTION;
      pal_mem_cpy (src_ipaddr_buf, line, pal_strlen (line) + 1);
      argv [i++] = src_ipaddr_buf;
    }

#ifdef HAVE_VRF
  line = imish_gets ("Name of the VRF : ");
  SKIP_WHITE_SPACE(line);
  name = line;
#endif
  if (ic)
    imish_line_sync_vrf (&ic->line, name);

  /* Process numeric display for addresses. */
  done = 0;
  while (!done)
    {
      line = imish_gets ("Numeric display [n]: ");
      SKIP_WHITE_SPACE(line);
      if (*line != '\0')
        {
          SKIP_WHITE_SPACE(line);

          if (line[0] == 'y')
            {
              argv[i++] = TRACEROUTE_NUMERIC_OPTION;
              done = 1;
            }
          else if (line[0] == 'n')
            {
              done = 1;
            }
          else
            printf ("%% Please answer 'yes' or 'no'.\n");
        }
      else
        {
          done = 1;
        }
    }

  /* Process timeout. */
  done = 0;
  while (!done)
    {
      line = imish_gets ("Timeout in seconds [2]: ");
      SKIP_WHITE_SPACE(line);
      if (*line == '\0')
        {
            argv[i++] = TRACEROUTE_TIMEOUT_OPTION;

          pal_snprintf (timeout_buf, TIMEOUT_SIZE, "%d", TRACEROUTE_DEFAULT_TIMEOUT);
          argv[i++] = timeout_buf;

          done = 1;
        }
      else
        {
          int timeout;

          SKIP_WHITE_SPACE(line);

          timeout = cmd_str2int (line, &ret);
          if (ret < 0 || timeout < 0 || timeout > 3600)
            {
              printf ("%% A decimal number between 0 and 3600.\n");
              continue;
            }

            argv[i++] = TRACEROUTE_TIMEOUT_OPTION;

          pal_snprintf (timeout_buf, TIMEOUT_SIZE, "%d", timeout);
          argv[i++] = timeout_buf;

          done = 1;
        }
    }

  /* Process probe count. */
  done = 0;
  while (!done)
    {
      line = imish_gets ("Probe count [3]: ");
      SKIP_WHITE_SPACE(line);
      if (*line == '\0')
        {
            argv[i++] = TRACEROUTE_PROBE_CNT_OPTION;

          pal_snprintf (probecnt_buf, PROBECNT_SIZE, "%d", TRACEROUTE_DEFAULT_PROBECNT);
          argv[i++] = probecnt_buf;

          done = 1;
        }
      else
        {
          int probecnt;

          SKIP_WHITE_SPACE(line);

          probecnt = cmd_str2int (line, &ret);
          if (ret < 0 || probecnt < 1 || probecnt > 65535)
            {
              printf ("%% A decimal number between 1 and 65535.\n");
              continue;
            }

            argv[i++] = TRACEROUTE_PROBE_CNT_OPTION;

          pal_snprintf (probecnt_buf, TIMEOUT_SIZE, "%d", probecnt);
          argv[i++] = probecnt_buf;

          done = 1;
        }
    }

  /* Process maximum time to live. */
  done = 0;
  while (!done)
    {
      line = imish_gets ("Maximum time to live [30]: ");
      SKIP_WHITE_SPACE(line);
      if (*line == '\0')
        {
            argv[i++] = TRACEROUTE_MAX_TTL_OPTION;

          pal_snprintf (ttl_buf, TTL_SIZE, "%d", TRACEROUTE_DEFAULT_MAX_TTL);
          argv[i++] = ttl_buf;

          done = 1;
        }
      else
        {
          int ttl;

          SKIP_WHITE_SPACE(line);

          ttl = cmd_str2int (line, &ret);
          if (ret < 0 || ttl < 1 || ttl > 255)
            {
              printf ("%% A decimal number between 1 and 255.\n");
              continue;
            }

            argv[i++] = TRACEROUTE_MAX_TTL_OPTION;

          pal_snprintf (ttl_buf, TTL_SIZE, "%d", ttl);
          argv[i++] = ttl_buf;

          done = 1;
        }
    }

  /* Process port number. */
  done = 0;
  while (!done)
    {
      line = imish_gets ("Port Number [33434]: ");
      SKIP_WHITE_SPACE(line);
      if (*line == '\0')
        {
            argv[i++] = TRACEROUTE_PORTNO_OPTION;

          pal_snprintf (port_buf, PORT_SIZE, "%d", TRACEROUTE_DEFAULT_PORTNO);
          argv[i++] = port_buf;

          done = 1;
        }
      else
        {
          int port;

          SKIP_WHITE_SPACE(line);

          port = cmd_str2int (line, &ret);
          if (ret < 0 || port < 1025 || port > 65535)
            {
              printf ("%% A decimal number between 1025 and 65535.\n");
              continue;
            }

            argv[i++] = TRACEROUTE_PORTNO_OPTION;

          pal_snprintf (port_buf, PORT_SIZE, "%d", port);
          argv[i++] = port_buf;

          done = 1;
        }
    }

  /* Process  Loose, Strict, Record, Timestamp, Verbose[none] */

  argv[i] = NULL;

  /* Map traceroute options. */
  pal_traceroute (i, argv, &argco, &argvo, vrf_fib_id);

  /* Execute command. */
  imish_execvp (argvo);

  /* Free argvo. */
  XFREE (MTYPE_TMP, argvo);

  return CLI_SUCCESS;
}

#ifndef DISABLE_START_SHELL
/* Start shell.  */
CLI (imish_start_shell,
     imish_start_shell_cli,
     "start-shell",
     "Start shell")
{
  readline_pacos_cancel_excl_raw_input_mode ();
  imish_exec (SHELL_PROG_NAME, NULL);
  readline_pacos_set_excl_raw_input_mode ();
  return CLI_SUCCESS;
}
#endif /* ! DISABLE_START_SHELL */

/* Ping.  */
#ifdef HAVE_VRF
CLI (imish_ping,
     imish_ping_cli,
     "ping WORD (vrf NAME|) ",
     "Send echo messages",
     "Ping destination address or hostname",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
#else
CLI (imish_ping,
     imish_ping_cli,
     "ping WORD",
     "Send echo messages",
     "Ping destination address or hostname")
#endif
{
  char *argvi[6];
  int argco;
  char **argvo;
  struct imi_client *ic = imishm->imh->info;
  char *name = NULL;

  argvi[0] = "ping";
  argvi[1] = PING_FAMILY_OPTION;
  argvi[2] = PING_FAMILY_IPV4;
  argvi[3] = PING_ADDRESS_OPTION;
  argvi[4] = argv[0];
  argvi[5] = NULL;

  if (argc > 1 && !pal_strcmp(argv[1], "vrf"))
    name = argv[2];

  if (ic)
    imish_line_sync_vrf (&ic->line, name);

  /* Map ping options. */
  pal_ping (5, argvi, &argco, &argvo, vrf_fib_id);

  /* Execute ping. */
  imish_execvp (argvo);

  /* Free argvo. */
  XFREE (MTYPE_TMP, argvo);

  return CLI_SUCCESS;
}

ALI (imish_ping,
     imish_ping_ip_cli,
     "ping ip WORD",
     "Send echo messages",
     "IP echo",
     "Ping destination address or hostname");

CLI (imish_ping_interactive,
     imish_ping_interactive_cli,
     "ping",
     "Send echo messages")
{
  imish_interactive_ping ();
  return CLI_SUCCESS;
}

#if defined HAVE_MPLS_OAM || defined HAVE_NSM_MPLS_OAM
/* Ping and Traceroute for MPLS Networks */

#ifdef HAVE_VCCV
CLI (imish_mpls_ping,
     imish_mpls_ping_cli,
     "ping mpls (ldp A.B.C.D/M|rsvp (tunnel-name NAME|egress A.B.C.D)|"
     "l2-circuit "
     "(vccv|)" 
     "<1-10000> |vpls <1-10000> peer A.B.C.D/M|l3vpn VRFNAME A.B.C.D/M"
     "|ipv4 A.B.C.D/M) ({reply-mode (1|2)|flags|destination A.B.C.D"
     "|source A.B.C.D|ttl <1-255>|timeout <1-500>|repeat <5-5000>"
     "|interval <2-20000>|force-explicit-null|detail}|)",
     "Ping command",
     "MPLS LSP ping",
     "FEC Type LDP",
     "LDP FEC prefix Address",
     "FEC type RSVP",
     "RSVP tunnel name",
     "Tunnel name string",
     "RSVP Egress",
     "RSVP tunnel egress address",
     "FEC Type MPLS L2 Circuit",
     "VCCV",
     "L2 Circuit id",
     "FEC Type MPLS VPLS (L2-VPN)",
     "VPLS instance id",
     "VPLS peer",
     "VPLS peer address",
     "FEC type MPLS VPN (L3-VPN)",
     "VPN Instance Name",
     "VPN Prefix",
     "FEC Type Generic - used for Static/SNMP lsp's",
     "IPv4 Prefix address",
     "Reply-mode",
     "1 - Reply via UDP/IP packet (default)",
     "2 - Reply via IP Pakcet with Router Alert",
     "Validate FEC",
     "Destination",
     "IPv4 Address",
     "Source",
     "IPv4 Address",
     "Time-to-live",
     "ttl Value",
     "Timeout of ping",
     "timeout value",
     "Count",
     "Number of pings to send",
     "Interval",
     "Interval between pings in Milliseconds",
     "Force Explicit NULL label",
     "detailed output"
     )
#else
CLI (imish_mpls_ping,
     imish_mpls_ping_cli,
     "ping mpls (ldp A.B.C.D/M|rsvp (tunnel-name NAME|egress A.B.C.D)|"
     "l2-circuit "
     "<1-10000> |vpls <1-10000> peer A.B.C.D/M|l3vpn VRFNAME A.B.C.D/M"
     "|ipv4 A.B.C.D/M) ({reply-mode (1|2)|flags|destination A.B.C.D"
     "|source A.B.C.D|ttl <1-255>|timeout <1-500>|repeat <5-5000>"
     "|interval <2-20000>|force-explicit-null|detail}|)",
     "Ping command",
     "MPLS LSP ping",
     "FEC Type LDP",
     "LDP FEC prefix Address",
     "FEC type RSVP",
     "RSVP tunnel name",
     "Tunnel name string",
     "RSVP Egress",
     "RSVP tunnel egress address",
     "FEC Type MPLS L2 Circuit",
     "L2 Circuit id",
     "FEC Type MPLS VPLS (L2-VPN)",
     "VPLS instance id",
     "VPLS peer",
     "VPLS peer address",
     "FEC type MPLS VPN (L3-VPN)",
     "VPN Instance Name",
     "VPN Prefix",
     "FEC Type Generic - used for Static/SNMP lsp's",
     "IPv4 Prefix address",
     "Reply-mode",
     "1 - Reply via UDP/IP packet (default)",
     "2 - Reply via IP Pakcet with Router Alert",
     "Validate FEC",
     "Destination",
     "IPv4 Address",
     "Source",
     "IPv4 Address",
     "Time-to-live",
     "ttl Value",
     "Timeout of ping",
     "timeout value",
     "Count",
     "Number of pings to send",
     "Interval",
     "Interval between pings in Milliseconds",
     "Force Explicit NULL label",
     "detailed output"
     )
#endif /* HAVE_VCCV */
{

  int argco;
  char **argvo;
  if (check_loopback_address (argc, argv, cli)
        != CLI_SUCCESS)
    return CLI_ERROR;

  /* Map ping options. */
  pal_mpls_oam (argc, "ping", argv, &argco, &argvo);

  /* Execute ping. */
  imish_execvp (argvo);

  XFREE (MTYPE_TMP, argvo);
  return CLI_SUCCESS;
}

CLI (imish_mpls_trace,
     imish_mpls_trace_cli,
     "trace mpls (ldp A.B.C.D/M|rsvp (tunnel-name NAME|egress A.B.C.D)|"
     "l2-circuit <1-10000>|vpls <1-10000> peer A.B.C.D/M|l3vpn VRFNAME A.B.C.D/M"
     "|ipv4 A.B.C.D/M) ({reply-mode (1|2)|flags|destination A.B.C.D"
     "|source A.B.C.D|ttl <1-255>|timeout <1-500>"
     "|force-explicit-null|detail}|)",
     "Traceroute command",
     "MPLS LSP Trace",
     "FEC Type LDP",
     "LDP FEC prefix Address",
     "FEC type RSVP",
     "RSVP tunnel name",
     "Tunnel name string",
     "RSVP egress",
     "RSVP tunnel egress address",
     "FEC Type MPLS L2 Circuit",
     "L2 Circuit id",
     "FEC Type MPLS VPLS (L2-VPN)",
     "VPLS instance id",
     "VPLS peer",
     "VPLS peer address",
     "FEC type MPLS VPN (L3-VPN)",
     "VPN Instance Name",
     "VPN Prefix",
     "FEC Type Generic - used for Static/SNMP lsp's",
     "IPv4 Prefix address",
     "Reply-mode",
     "1 - Reply via UDP/IP packet (default)",
     "2 - Reply via IP Pakcet with Router Alert",
     "Validate FEC",
     "Destination",
     "IPv4 Address",
     "Source",
     "IPv4 Address",
     "Time-to-live",
     "ttl Value",
     "Timeout of packets",
     "timeout value",
     "Force Explicit NULL label",
     "detailed output"
     )
{
  int argco;
  char **argvo;

  if (check_loopback_address (argc, argv, cli)
        != CLI_SUCCESS)
    return CLI_ERROR;

  /* Map ping options. */
  pal_mpls_oam (argc, "trace", argv, &argco, &argvo);

  /* Execute ping. */
  imish_execvp (argvo);

  /* Free argvo. */
  XFREE (MTYPE_TMP, argvo);


  return CLI_SUCCESS;
}

#endif /* HAVE_MPLS_OAM || HAVE_NSM_MPLS_OAM */
/* "mtrace" and "mstat" commands */
#ifdef HAVE_MCAST_IPV4
/* Interactive mode of ping.  */
int
imish_interactive_mtrace (u_int8_t short_display)
{
  char *line = NULL;
  char *argv[10];
  int i = 1;
#define IPADDR_STR_SIZE            16
  char srcaddr[IPADDR_STR_SIZE];
  char dstaddr[IPADDR_STR_SIZE];
  char grpaddr[IPADDR_STR_SIZE];
  char rspaddr[IPADDR_STR_SIZE];
  u_int8_t grp = 0;
#define TTL_SIZE               4
  u_int32_t ttl;
  char ttl_buf[TTL_SIZE];

  struct pal_in4_addr dummy;

  pal_mem_set (srcaddr, 0, IPADDR_STR_SIZE);
  pal_mem_set (dstaddr, 0, IPADDR_STR_SIZE);
  pal_mem_set (grpaddr, 0, IPADDR_STR_SIZE);
  pal_mem_set (rspaddr, 0, IPADDR_STR_SIZE);

  argv[0] = IMISH_MTRACE_CMD_STR;
  if (short_display)
    argv[i++] = IMISH_MTRACE_DISPLAY_OPTION;
  else
    argv[i++] = IMISH_MTRACE_VERBOSE_DISPLAY_OPTION;

  /* Process source address. Mandatory */
  line = imish_gets ("Source address: ");
  SKIP_WHITE_SPACE(line);
  if (*line == '\0')
    {
      printf ("%% Source address must be specified\n");
      return -1;
    }
  else
    {
      SKIP_WHITE_SPACE(line);

      IMISH_MTRACE_GET_IPV4_ADDRESS ("Source address", dummy, line);

      if (IN_MULTICAST (pal_ntoh32 (dummy.s_addr)))
        {
          printf ("%% Source cannot be a multicast address\n");
          return CLI_ERROR;
        }

      /* Copy into buffer */
      pal_strncpy (srcaddr, line, IPADDR_STR_SIZE);
    }

  /* Process destination address. Mandatory */
  line = imish_gets ("Destination address: ");
  SKIP_WHITE_SPACE(line);
  if (*line == '\0')
    {
      printf ("%% Destination address must be specified\n");
      return -1;
    }
  else
    {
      SKIP_WHITE_SPACE(line);

      IMISH_MTRACE_GET_IPV4_ADDRESS ("Destination address", dummy,
          line);

      if (IN_MULTICAST (pal_ntoh32 (dummy.s_addr)))
        {
          printf ("%% Destination cannot be a multicast address\n");
          return CLI_ERROR;
        }

      /* Copy into buffer */
      pal_strncpy (dstaddr, line, IPADDR_STR_SIZE);
    }

  /* Process group address. Optional */
  line = imish_gets ("Group address: ");
  SKIP_WHITE_SPACE(line);
  if (*line != '\0')
    {
      SKIP_WHITE_SPACE(line);

      IMISH_MTRACE_GET_IPV4_ADDRESS ("Group address", dummy,
          line);

      if (! IN_MULTICAST (pal_ntoh32 (dummy.s_addr)))
        {
          printf ("%% Group address is not a multicast address\n");
          return CLI_ERROR;
        }

      /* Copy into buffer */
      pal_strncpy (grpaddr, line, IPADDR_STR_SIZE);

      grp = 1;
    }

  /* Process TTL. Optional */
  line = imish_gets ("Multicast request TTL [127]: ");
  SKIP_WHITE_SPACE(line);
  if (*line != '\0')
    {
      SKIP_WHITE_SPACE(line);

      IMISH_MTRACE_GET_UINT32_RANGE ("TTL", ttl, line, 1, 255);

      pal_snprintf (ttl_buf, TTL_SIZE, "%u", ttl);

      argv[i++] = IMISH_MTRACE_TTL_OPTION;
      argv[i++] = ttl_buf;
    }

  /* Process response address. Optional */
  line = imish_gets ("Response address for mtrace: ");
  SKIP_WHITE_SPACE(line);
  if (*line != '\0')
    {
      SKIP_WHITE_SPACE(line);

      IMISH_MTRACE_GET_IPV4_ADDRESS ("Response address", dummy,
          line);

      /* Copy into buffer */
      pal_strncpy (rspaddr, line, IPADDR_STR_SIZE);

      argv[i++] = IMISH_MTRACE_RESP_OPTION;
      argv[i++] = rspaddr;
    }

  /* Set the source destination and optional group address */
  argv[i++] = srcaddr;
  argv[i++] = dstaddr;
  if (grp)
    argv[i++] = grpaddr;

  argv[i] = NULL;

  /* Execute command. */
  imish_execvp (argv);

  return CLI_SUCCESS;
}

CLI (imish_mtrace_interactive,
    imish_mtrace_interactive_cmd,
    "(mtrace|mstat)",
    CLI_MTRACE_STR,
    CLI_MSTAT_STR)
{
  if (!pal_strncmp (argv[0], IMISH_MTRACE_CMD_STR,
        sizeof (IMISH_MTRACE_CMD_STR)))
    imish_interactive_mtrace (1);
  else
    imish_interactive_mtrace (0);

  return CLI_SUCCESS;
}

CLI (imish_mtrace_src,
    imish_mtrace_src_cmd,
    "(mtrace|mstat) A.B.C.D",
    CLI_MTRACE_STR,
    CLI_MSTAT_STR,
    CLI_MTRACE_SRC_STR)
{
  char *argvi[5];
  struct pal_in4_addr src;
  int i = 1;

  CLI_GET_IPV4_ADDRESS ("Source address", src, argv[1]);

  if (IN_MULTICAST (pal_ntoh32 (src.s_addr)))
    {
      cli_out (cli, "%% Source cannot be a multicast address\n");
      return CLI_ERROR;
    }

  argvi[0] = IMISH_MTRACE_CMD_STR;

  /* Form the arguments for mtrace */
  IMISH_MTRACE_CMD_OPTIONS (argv[0], argvi, i);
  argvi[i++] = argv[1];
  argvi[i++] = IMISH_MTRACE_DEF_GRP;
  argvi[i++] = NULL;

  /* Execute mtrace. */
  imish_execvp (argvi);

  return CLI_SUCCESS;
}

CLI (imish_mtrace_src_dst,
    imish_mtrace_src_dst_cmd,
    "(mtrace|mstat) A.B.C.D A.B.C.D",
    CLI_MTRACE_STR,
    CLI_MSTAT_STR,
    CLI_MTRACE_SRC_STR,
    CLI_MTRACE_DST_STR)
{
  char *argvi[6];
  struct pal_in4_addr src;
  struct pal_in4_addr dst;
  int i = 1;

  CLI_GET_IPV4_ADDRESS ("Source address", src, argv[1]);
  CLI_GET_IPV4_ADDRESS ("Destination address", dst, argv[2]);

  if (IN_MULTICAST (pal_ntoh32 (src.s_addr)))
    {
      cli_out (cli, "%% Source cannot be a multicast address\n");
      return CLI_ERROR;
    }

  argvi[0] = IMISH_MTRACE_CMD_STR;

  /* Form the arguments for mtrace */
  IMISH_MTRACE_CMD_OPTIONS (argv[0], argvi, i);
  argvi[i++] = argv[1];
  argvi[i++] = argv[2];

  if (!IN_MULTICAST (pal_ntoh32 (dst.s_addr)))
    argvi[i++] = IMISH_MTRACE_DEF_GRP;

  argvi[i++] = NULL;

  /* Execute mtrace. */
  imish_execvp (argvi);

  return CLI_SUCCESS;
}

CLI (imish_mtrace_src_dst_grp,
    imish_mtrace_src_dst_grp_cmd,
    "(mtrace|mstat) A.B.C.D A.B.C.D A.B.C.D (<1-255>|)",
    CLI_MTRACE_STR,
    CLI_MSTAT_STR,
    CLI_MTRACE_SRC_STR,
    CLI_MTRACE_DST_STR,
    CLI_MTRACE_GRP_STR,
    CLI_MTRACE_TTL_STR)
{
  char *argvi[8];
  struct pal_in4_addr src;
  struct pal_in4_addr dst;
  struct pal_in4_addr grp;
  u_int32_t ttl;
  int i = 1;

  CLI_GET_IPV4_ADDRESS ("Source address", src, argv[1]);
  CLI_GET_IPV4_ADDRESS ("Destination address", dst, argv[2]);
  CLI_GET_IPV4_ADDRESS ("Group address", grp, argv[3]);
  if (argc > 4)
    CLI_GET_UINT32_RANGE ("TTL", ttl, argv[4], 1, 255);

  if (IN_MULTICAST (pal_ntoh32 (src.s_addr)))
    {
      cli_out (cli, "%% Source cannot be a multicast address\n");
      return CLI_ERROR;
    }
  if (IN_MULTICAST (pal_ntoh32 (dst.s_addr)))
    {
      cli_out (cli, "%% Destination cannot be a multicast address\n");
      return CLI_ERROR;
    }
  if ((grp.s_addr != 0) && ! IN_MULTICAST (pal_ntoh32 (grp.s_addr)))
    {
      cli_out (cli, "%% Group address is not a multicast address\n");
      return CLI_ERROR;
    }

  argvi[0] = IMISH_MTRACE_CMD_STR;

  /* Form the arguments for mtrace */
  IMISH_MTRACE_CMD_OPTIONS (argv[0], argvi, i);

  /* Set the TTL if required */
  if (argc > 4)
    {
      argvi[i++] = IMISH_MTRACE_TTL_OPTION;
      argvi[i++] = argv[4];
    }
  argvi[i++] = argv[1];
  argvi[i++] = argv[2];
  argvi[i++] = argv[3];

  argvi[i++] = NULL;

  /* Execute mtrace. */
  imish_execvp (argvi);

  return CLI_SUCCESS;
}
#endif /* HAVE_MCAST_IPV4 */

#ifndef HAVE_SPLAT
/* Telnet */
CLI (imish_telnet,
     imish_telnet_cli,
     "telnet WORD",
     "Open a telnet connection",
     "IP address or hostname of a remote system")
{
  if (argc == 1)
    imish_exec ("telnet", argv[0], NULL);
  else
    imish_exec ("telnet", argv[0], argv[1], NULL);

  return CLI_SUCCESS;
}

ALI (imish_telnet,
     imish_telnet_port_cli,
     "telnet WORD PORT",
     "Open a telnet connection",
     "IP address or hostname of a remote system",
     "TCP Port number");
#endif /* HAVE_SPLAT */

/* Traceroute.  */
#ifdef HAVE_VRF
CLI (imish_traceroute,
     imish_traceroute_cli,
     "traceroute WORD (vrf NAME|)",
     "Trace route to destination",
     "Trace route to destination address or hostname",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
#else
CLI (imish_traceroute,
     imish_traceroute_cli,
     "traceroute WORD",
     "Trace route to destination",
     "Trace route to destination address or hostname")
#endif
{
  char *argvi[6];
  int argco;
  char **argvo;
  struct imi_client *ic = imishm->imh->info;
  char *name = NULL;

  argvi[0] = "traceroute";
  argvi[1] = TRACEROUTE_FAMILY_OPTION;
  argvi[2] = TRACEROUTE_FAMILY_IPV4;
  argvi[3] = TRACEROUTE_ADDRESS_OPTION;
  argvi[4] = argv[0];
  argvi[5] = NULL;

  if (argc == 3)
    name = argv[2];

  if (ic)
    imish_line_sync_vrf (&ic->line, name);
  /* Map traceroute options. */
  pal_traceroute (5, argvi, &argco, &argvo, vrf_fib_id);

  /* Execute traceroute. */
  imish_execvp (argvo);

  /* Free argvo. */
  XFREE (MTYPE_TMP, argvo);

  return CLI_SUCCESS;
}

ALI (imish_traceroute,
     imish_traceroute_ip_cli,
     "traceroute ip WORD",
     "Trace route to destination",
     "IP Trace",
     "Trace route to destination address or hostname");

     CLI (imish_traceroute_interactive,
          imish_traceroute_interactive_cli,
          "traceroute",
          "Trace route to destination")
{
  imish_interactive_traceroute ();
  return CLI_SUCCESS;
}

#ifdef HAVE_IPV6
#ifdef HAVE_VRF
CLI (imish_ping_ipv6,
     imish_ping_ipv6_cli,
     "ping ipv6 WORD (|IFNAME) (vrf NAME|)",
     "Send echo messages",
     "IPv6 echo",
     "Ping destination address or hostname",
     "Interface's name",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
#else
CLI (imish_ping_ipv6,
     imish_ping_ipv6_cli,
     "ping ipv6 WORD (|IFNAME)",
     "Send echo messages",
     "IPv6 echo",
     "Ping destination address or hostname",
     "Interface's name")
#endif
{
  struct prefix_ipv6 p;
  char *line = NULL;
  int ret;
  char *argvi[8];
  int argco;
  char **argvo;
  struct imi_client *ic = imishm->imh->info;
  char *name = NULL;

  argvi[0] = "ping6";

  if (argc == 4)
    {
      argvi[1] = PING_FAMILY_OPTION;
      argvi[2] = PING_FAMILY_IPV6;
      argvi[3] = PING_ADDRESS_OPTION;
      argvi[4] = argv[0];
      argvi[5] = PING_INTERFACE_OPTION;
      argvi[6] = argv[1];
      argvi[7] = NULL;

      name = argv[3];
      if (ic)
        imish_line_sync_vrf (&ic->line, name);

      /* Map ping options. */
      pal_ping (7, argvi, &argco, &argvo, vrf_fib_id);

      /* Execute ping. */
      imish_execvp (argvo);

      /* Free argvo. */
      XFREE (MTYPE_TMP, argvo);
    }
  else
    {
      char *pIp_Add = argv[0];
      if ( (argc == 3) && (pal_strcmp(argv[1], "vrf") == 0))
        name = argv[2];

      if (ic)
        imish_line_sync_vrf (&ic->line, name);

       /* For link-local addresses accept interface parameter. */
      ret = str2prefix_ipv6 (pIp_Add, &p);
      if (ret > 0)
        {
          if (IN6_IS_ADDR_LINKLOCAL(&p.prefix))
            {
              line = imish_gets ("Output Interface: ");
              SKIP_WHITE_SPACE(line);
              if (*line == '\0')
                {
                  cli_out (cli, "%% Interface required\n");
                  return CLI_ERROR;
                }

              argvi[1] = PING_FAMILY_OPTION;
              argvi[2] = PING_FAMILY_IPV6;
              argvi[3] = PING_ADDRESS_OPTION;
              argvi[4] = pIp_Add;
              argvi[5] = PING_INTERFACE_OPTION;
              argvi[6] = line;
              argvi[7] = NULL;

              /* Map ping options. */
              pal_ping (7, argvi, &argco, &argvo, vrf_fib_id);

              /* Execute ping. */
              imish_execvp (argvo);

              /* Free argvo. */
              XFREE (MTYPE_TMP, argvo);
            }
          else
            {
              argvi[1] = PING_FAMILY_OPTION;
              argvi[2] = PING_FAMILY_IPV6;
              argvi[3] = PING_ADDRESS_OPTION;
              argvi[4] = pIp_Add;
              argvi[5] = NULL;

              /* Map ping options. */
              pal_ping (5, argvi, &argco, &argvo, vrf_fib_id);

              /* Execute ping. */
              imish_execvp (argvo);

              /* Free argvo. */
              XFREE (MTYPE_TMP, argvo);
            }
        }
      else
        {
          argvi[1] = PING_FAMILY_OPTION;
          argvi[2] = PING_FAMILY_IPV6;
          argvi[3] = PING_ADDRESS_OPTION;
          argvi[4] = pIp_Add;
          argvi[5] = NULL;

          /* Map ping options. */
          pal_ping (5, argvi, &argco, &argvo, vrf_fib_id);

          /* Execute ping. */
          imish_execvp (argvo);

          /* Free argvo. */
          XFREE (MTYPE_TMP, argvo);
        }
    }
  return CLI_SUCCESS;
}

#ifdef HAVE_VRF
CLI (imish_traceroute_ipv6,
     imish_traceroute_ipv6_cli,
     "traceroute ipv6 WORD (vrf NAME|)",
     "Trace route to destination",
     "IPv6 trace",
     "Trace route to destination address or hostname",
     CLI_VRF_STR,
     CLI_VRF_NAME_STR)
#else
CLI (imish_traceroute_ipv6,
     imish_traceroute_ipv6_cli,
     "traceroute ipv6 WORD",
     "Trace route to destination",
     "IPv6 trace",
     "Trace route to destination address or hostname")
#endif
{
  char *argvi[6];
  int argco;
  char **argvo;
  int i;
  struct imi_client *ic = imishm->imh->info;
  char *name = NULL;

  if (argc == 3)
    name = argv[2];

  argvi[0] = "traceroute6";
  argvi[1] = TRACEROUTE_FAMILY_OPTION;
  argvi[2] = TRACEROUTE_FAMILY_IPV6;
  argvi[3] = TRACEROUTE_ADDRESS_OPTION;
  argvi[4] = argv[0];
  argvi[5] = NULL;

   if (ic)
     imish_line_sync_vrf (&ic->line, name);

  /* Map traceroute options. */
  pal_traceroute (5, argvi, &argco, &argvo, vrf_fib_id);

  for (i = 0 ; i < argco; i++)
    if (argvo[i])
      printf("%s\n", argvo[i]);
  /* Execute traceroute. */
  imish_execvp (argvo);

  /* Free argvo. */
  XFREE (MTYPE_TMP, argvo);

  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */

/* Logout. */
CLI (imish_config_exit,
     imish_config_exit_cli,
     "(exit|logout|quit)",
     "Exit from the EXEC",
     "Exit from the EXEC",
     "Exit from the EXEC")
{
  struct imi_client *ic = imishm->imh->info;
  struct line *line = &ic->line;

  line->mode = IMISH_MODE;

  if (cli->vr->id != 0)
    if (CHECK_FLAG (cli->flags, CLI_FROM_PVR))
      {
        imish_line_send (line, "login virtual-router");
        imish_line_read (line);
      }

  cli_config_exit (cli, argc, argv);

  return CLI_SUCCESS;
}

CLI (imish_do,
     imish_do_cli,
     "do LINE",
     "To run exec commands in config mode",
     "Exec Command")
{
  char *input;

  /* This is unique command.  */
  input = argv[0];

  imish_parse (input, EXEC_MODE);

  XFREE (MTYPE_TMP, input);

  return CLI_SUCCESS;
}


/* IMI commands.  */
CLI (imish_line_privilege,
     imish_line_privilege_cli,
     "privilege <0-15>",
     "Set privilege",
     "Privilege level")
{
  u_char level;

  CLI_GET_INTEGER_RANGE ("privilege level", level, argv[0],
                         PRIVILEGE_MIN, PRIVILEGE_MAX);
  cli->privilege = level;

  return CLI_SUCCESS;
}

#ifdef HAVE_VR
ALI (imish_line_privilege,
     imish_line_privilege_pvr_cli,
     "privilege (16)",
     "Set privilege",
     "Privilege level");
#endif /* HAVE_VR */

/* Set history max value. */
CLI (imish_history_max_set,
     imish_history_max_set_cli,
     "history max <0-2147483647>",
     "Configure history",
     "Set max value",
     "Number of commands")
{
  int history_val = 0;

  history_val = pal_strtou32 (argv[0], NULL, 10);
  rl_set_history_max (history_val);

  return CLI_SUCCESS;
}

/* Set time out value. */
void
imish_exec_timeout (char *min_str, char *sec_str)
{
  u_int32_t timeout = 0;

  if (min_str)
    {
      timeout = pal_strtou32 (min_str, NULL, 10);
      timeout *= 60;
    }
  if (sec_str)
    timeout += pal_strtou32 (sec_str, NULL, 10);

  imish_timeout_set (timeout);
}

CLI (imish_line_exec_timeout,
     imish_line_exec_timeout_cli,
     "exec-timeout <0-35791>",
     "Set timeout value",
     "Timeout value in minutes")
{
  imish_exec_timeout (argv[0], NULL);
  return CLI_SUCCESS;
}

CLI (imish_line_exec_timeout_sec,
     imish_line_exec_timeout_sec_cli,
     "exec-timeout <0-35791> <0-2147483>",
     "Set timeout value",
     "Timeout value in minutes",
     "Timeout in seconds")
{
  imish_exec_timeout (argv[0], argv[1]);
  return CLI_SUCCESS;
}

/* Banner configuration.  */
CLI (imish_banner_motd_custom,
     imish_banner_motd_custom_cli,
     "banner motd LINE",
     "Define a login banner",
     "Set Message of the Day banner",
     "Custom string")
{
  if (! suppress)
    printf ("%s\n", argv[0]);

  return CLI_SUCCESS;
}

/* Hostname configuration from IMI.  */
CLI (imish_hostname,
     imish_hostname_cli,
     "hostname WORD",
     "Set system's network name",
     "This system's network name")
{
  imish_hostname_set (argv[0]);
  return CLI_SUCCESS;
}

CLI (imish_hostname_custom,
     imish_hostname_custom_cli,
     "hostname WORD",
     "Set system's network name",
     "This system's network name")
{
  imish_hostname_set (argv[0]);
  return CLI_SUCCESS;
}

CLI (imish_no_hostname,
     imish_no_hostname_cli,
     "no hostname (WORD|)",
     CLI_NO_STR,
     "Set system's network name",
     "This system's network name")
{
  imish_hostname_unset ();
  return CLI_SUCCESS;
}

CLI (imish_no_hostname_custom,
     imish_no_hostname_custom_cli,
     "no hostname (WORD|)",
     CLI_NO_STR,
     "Set system's network name",
     "This system's network name")
{
  imish_hostname_unset ();
  return CLI_SUCCESS;
}

/* FIB ID.  */
CLI (imish_fib_id,
     imish_fib_id_cli,
     "fib-id WORD",
     CLI_FIB_STR,
     "FIB ID")
{
  imish_fib_id_set (pal_atoi (argv[0]));
  return CLI_SUCCESS;
}

/*  FIB_ID for a particular VRF */
CLI (imish_fib_vrf_id,
     imish_fib_id_vrf_cli,
     "fib-vrf-id WORD",
     CLI_FIB_STR,
     "FIB ID")
{
  memset (vrf_fib_id, '\0', VRF_FIB_ID_MAX_LEN);
  pal_strcpy (vrf_fib_id, argv[0]);
  return CLI_SUCCESS;
}

/* Enable.  */
CLI (imish_enable,
     imish_enable_cli,
     "enable",
     "Turn on privileged mode command")
{
  int failure;
  char *password;
  struct imi_client *ic = imishm->imh->info;
  struct line *line = &ic->line;

  if (cli->privilege >= PRIVILEGE_ENABLE (cli->vr))
    return CLI_SUCCESS;

  if (cli->auth)
    {
      cli->auth = 0;

      for (failure = 0; failure < 3; failure++)
        {
          password = getpass ("Password: ");

          if (pal_strlen (password) > HOST_MAX_PASSWD_LEN)
            continue;

          line->mode = EXEC_MODE;
          imish_line_send (line, "enable %s", password);
          imish_line_read (line);
          if (line->code == LINE_CODE_SUCCESS)
            {
              cli->privilege = PRIVILEGE_ENABLE (cli->vr);
              return CLI_SUCCESS;
            }
        }
      printf ("%% Bad passwords\n\n");
      return CLI_ERROR;
    }
  else
    {
      cli->privilege = PRIVILEGE_ENABLE (cli->vr);
      return CLI_SUCCESS;
    }
}

/* Disable.  */
CLI (imish_disable,
     imish_disable_cli,
     "disable",
     "Turn off privileged mode command")
{
  cli->privilege = PRIVILEGE_NORMAL;
  return CLI_SUCCESS;
}


/* "write" commands  */
CLI (imish_write,
     imish_write_cli,
     "write",
     CLI_WRITE_STR)
{
#ifndef HAVE_CUSTOM1
  /* Print out message.  */
  printf ("Building configuration...\n");
#else
  printf ("Writing to flash memory...\n");
  imish_exec ("flashrw", "config", "write", NULL);
#endif /* HAVE_CUSTOM1 */
  return CLI_SUCCESS;
}

ALI (imish_write,
     imish_write_file_cli,
     "write file",
     CLI_WRITE_STR,
     "Write to file");

ALI (imish_write,
     imish_write_memory_cli,
     "write memory",
     CLI_WRITE_STR,
     "Write to NV memory");

ALI (imish_write,
     imish_copy_runconfig_startconfig_cli,
     "copy running-config startup-config",
     "Copy from one file to another",
     "Copy from current system configuration",
     "Copy to startup configuration");

/* "terminal length"  */
CLI (imish_terminal_length,
     imish_terminal_length_cli,
     "terminal length <0-512>",
     "Set terminal line parameters",
     "Set number of lines on a screen",
     "Number of lines on screen (0 for no pausing)")
{
  int lines;

  CLI_GET_INTEGER ("length", lines, argv[0]);
  cli->lines = lines;

  imish_win_row_orig = imish_win_row;
  imish_win_row = lines;

  imish_signal_set (SIGWINCH, imish_sigwinch);
  pal_term_winsize_set (PAL_STDIN_FILENO, imish_win_row, imish_win_col);

  return CLI_SUCCESS;
}

CLI (imish_terminal_no_length,
     imish_terminal_no_length_cli,
     "terminal no length (<0-512>|)",
     "Unset terminal line parameters",
     CLI_NO_STR,
     "Unset number of lines on a screen",
     "Number of lines on screen (0 for no pausing)")
{
  cli->lines = -1;

  imish_signal_set (SIGWINCH, SIG_IGN);

  imish_win_row = imish_win_row_orig;
  pal_term_winsize_set (PAL_STDIN_FILENO, imish_win_row, imish_win_col);

  return CLI_SUCCESS;
}

/* "show history"  */
CLI (imish_show_history,
     imish_show_history_cli,
     "show history",
     CLI_SHOW_STR,
     "Display the session command history")
{
  imish_history_show ();
  return CLI_SUCCESS;
}

/* "service advanced-vty"  */
CLI (imish_service_advanced_vty,
     imish_service_advanced_vty_cli,
     "service advanced-vty",
     "Set up miscellaneous service",
     "Enable advanced mode vty interface")
{
  imish_advance_set ();
  return CLI_SUCCESS;
}

CLI (imish_no_service_advanced_vty,
     imish_no_service_advanced_vty_cli,
     "no service advanced-vty",
     CLI_NO_STR,
     "Set up miscellaneous service",
     "Disable advanced mode vty interface")
{
  imish_advance_unset ();
  return CLI_SUCCESS;
}

CLI (imish_no_service_advanced_vty_custom,
     imish_no_service_advanced_vty_custom_cli,
     "no service advanced-vty",
     CLI_NO_STR,
     "Set up miscellaneous service",
     "Disable advanced mode vty interface")
{
  imish_advance_unset ();
  return CLI_SUCCESS;
}

#ifdef HAVE_RELOAD
CLI (imish_reload,
     imish_reload_cli,
     "reload",
     "Halt and perform a cold restart")
{
  int ret;

#ifdef HAVE_CUSTOM1
  if (imish_star ())
    if (imish_yes_or_no ("save running config? (y/n): ") > 0)
      imish_config_save_force (cli);
#endif /* HAVE_CUSTOM1 */

  ret = imish_yes_or_no ("reboot system? (y/n): ");
  if (ret <= 0)
    return CLI_ERROR;

#ifdef HAVE_CUSTOM1
  system ("/etc/rc.d/syslogd stop");
#endif /* HAVE_CUSTOM1 */

  return CLI_SUCCESS;
}

#ifdef HAVE_REBOOT
ALI (imish_reload,
     imish_reboot_cli,
     "reboot",
     "Halt and perform a cold restart");
#endif /* HAVE_REBOOT */
#endif /* HAVE_RELOAD */
/* Ping.  */


void
imish_cli_system_init ()
{
#ifndef DISABLE_START_SHELL
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, 0,
                   &imish_start_shell_cli);
#endif /* ! DISABLE_START_SHELL */
}

void
imish_cli_init (struct lib_globals *zg)
{
  int i;

  ctree = zg->ctree;

  /* Install CLI mode commands. */
  imish_command_init ();

  /* Install extracted CLI commands.  */
  imi_extracted_cmd_init (ctree);

  /* Install system commands (start-sh). */
  imish_cli_system_init ();

  /* EXEC mode commands.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_config_exit_cli);

  /* ping and traceroute.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_ping_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_ping_ip_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_ping_interactive_cli);
#if defined HAVE_MPLS_OAM || defined HAVE_NSM_MPLS_OAM
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_mpls_ping_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_mpls_trace_cli);
#endif /* HAVE_MPLS_OAM || HAVE_NSM_MPLS_OAM */

#ifdef HAVE_MCAST_IPV4
  /* mtrace and mstat */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_mtrace_src_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_mtrace_src_dst_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_mtrace_src_dst_grp_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_mtrace_interactive_cmd);
#endif /* HAVE_MCAST_IPV4 */

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_traceroute_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_traceroute_ip_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_traceroute_interactive_cli);
#ifndef HAVE_SPLAT
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_telnet_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_telnet_port_cli);
#endif /* HAVE_SPLAT */
#ifdef HAVE_IPV6
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_ping_ipv6_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &imish_traceroute_ipv6_cli);
#endif /* HAVE_IPV6 */

  /* Show history. */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_show_history_cli);

  /* Do command.  */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_do_cli);

  /* Hostname command.  */
  cli_install_imi (ctree, CONFIG_MODE, PM_HOSTNAME, PRIVILEGE_NORMAL, 0,
                   &imish_hostname_cli);
  cli_install_imi (ctree, CONFIG_MODE, PM_HOSTNAME, PRIVILEGE_NORMAL, 0,
                   &imish_no_hostname_cli);

  /* Service advanced VTY.  */
  cli_install_imi (ctree, CONFIG_MODE, PM_EMPTY, PRIVILEGE_NORMAL, 0,
                   &imish_service_advanced_vty_cli);
  cli_install_imi (ctree, CONFIG_MODE, PM_EMPTY, PRIVILEGE_NORMAL, 0,
                   &imish_no_service_advanced_vty_cli);

  /* IMI commands.  */
  cli_install_gen (ctree, IMISH_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_line_privilege_cli);
#ifdef HAVE_VR
  cli_install_gen (ctree, IMISH_MODE, PRIVILEGE_MAX, 0,
                   &imish_line_privilege_pvr_cli);
#endif /* HAVE_VR */
  cli_install_gen (ctree, IMISH_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_line_exec_timeout_cli);
  cli_install_gen (ctree, IMISH_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_history_max_set_cli);
  cli_install_gen (ctree, IMISH_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_line_exec_timeout_sec_cli);
  cli_install_gen (ctree, IMISH_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_banner_motd_custom_cli);
  cli_install_gen (ctree, IMISH_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_hostname_custom_cli);
  cli_install_gen (ctree, IMISH_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_no_hostname_custom_cli);
  cli_install_gen (ctree, IMISH_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_terminal_length_cli);
  cli_install_gen (ctree, IMISH_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_terminal_no_length_cli);
  cli_install_gen (ctree, IMISH_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_no_service_advanced_vty_custom_cli);
  cli_install_gen (ctree, IMISH_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_fib_id_cli);
  cli_install_gen (ctree, IMISH_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_fib_id_vrf_cli);
#ifndef HAVE_CUSTOM1
  imish_tech_cli_init();
#endif

  /* IMI EXEC MODE.  */
  cli_install_imi (ctree, EXEC_MODE, PM_EMPTY, PRIVILEGE_NORMAL, 0,
                   &imish_enable_cli);
  cli_install_imi (ctree, EXEC_MODE, PM_EMPTY, PRIVILEGE_NORMAL, 0,
                   &imish_disable_cli);

  /* "write" commands.  */
  cli_install_imi (ctree, EXEC_MODE, PM_EMPTY, PRIVILEGE_VR_MAX,
                   CLI_FLAG_LOCAL_FIRST, &imish_write_cli);
  cli_install_imi (ctree, EXEC_MODE, PM_EMPTY, PRIVILEGE_VR_MAX,
                   CLI_FLAG_LOCAL_FIRST, &imish_write_file_cli);
  cli_install_imi (ctree, EXEC_MODE, PM_EMPTY, PRIVILEGE_VR_MAX,
                   CLI_FLAG_LOCAL_FIRST, &imish_write_memory_cli);
  cli_install_imi (ctree, EXEC_MODE, PM_EMPTY, PRIVILEGE_VR_MAX,
                   CLI_FLAG_LOCAL_FIRST,
                   &imish_copy_runconfig_startconfig_cli);

  /* "write memory" hidden command.  */
  for (i = 0; i < MAX_MODE; i++)
    if (i != EXEC_MODE && i != EXEC_PRIV_MODE)
      cli_install_imi (ctree, i, PM_EMPTY, PRIVILEGE_VR_MAX,
                       (CLI_FLAG_HIDDEN|CLI_FLAG_LOCAL_FIRST),
                       &imish_write_memory_cli);

  /* Terminal length.  */
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_terminal_length_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_NORMAL, 0,
                   &imish_terminal_no_length_cli);

#ifdef HAVE_RELOAD
  /* Reboot command.  */
  cli_install_imi (ctree, EXEC_MODE, PM_EMPTY, PRIVILEGE_VR_MAX,
                   CLI_FLAG_LOCAL_FIRST, &imish_reload_cli);
#ifdef HAVE_REBOOT
  cli_install_imi (ctree, EXEC_MODE, PM_EMPTY, PRIVILEGE_VR_MAX,
                   (CLI_FLAG_HIDDEN|CLI_FLAG_LOCAL_FIRST),
                   &imish_reboot_cli);
#endif /* HAVE_REBOOT */
#endif /* HAVE_RELOAD */

#ifdef HAVE_VR
  /* VR commands. */
  imish_vr_cli_init (ctree);
#endif /* HAVE_VR */

#ifdef HAVE_CUSTOM1
  /* Custom1 commands. */
  imish_custom1_cli_init (ctree);
#endif /* HAVE_CUSTOM1 */
}
