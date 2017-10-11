/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "pal_ping.h"
#include "pal_traceroute.h"

#include "cli.h"
#include "line.h"
#include "host.h"
#include "version.h"

#include "vtysh.h"
#include "vtysh_exec.h"
#include "vtysh_cli.h"
#include "vtysh_parser.h"
#include "vtysh_system.h"
#include "vtysh_line.h"
#include "vtysh_readline.h"

struct cli_tree *ctree;

/* Extracted command initialization.  */
void imi_extracted_cmd_init (struct cli_tree *);

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
              if (pal_char_isspace (cp[i]))
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

  memset (&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = family;
  ret = pal_sock_getaddrinfo (host, NULL, &hints, &res);
  if (!ret)
    {
      /* Consider the first entry only. */

      if (family == AF_INET)
        {
          addr = (struct sockaddr_in *)res->ai_addr;
          pal_inet_ntop (family, &addr->sin_addr.s_addr, ipaddr, size);
          pal_sock_freeaddrinfo (res);
          return 0;
        }
#ifdef HAVE_IPV6
      else if (family == AF_INET6)
        {
          addr6 = (struct sockaddr_in6 *)res->ai_addr;
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
vtysh_interactive_ping ()
{
  char *line = NULL;
  char *argv[25];
  int i = 1;
  int type = -1;
#define IPADDR_SIZE            32
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

  /* For getopt to work. */
  argv[0] = "ping";

  /* Process protocol. */
  line = vtysh_gets ("Protocol [ip]: ");
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
  line = vtysh_gets ("Target IP address: ");
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

  /* Process Repeat count. */
  done = 0;
  while (!done)
    {
      line = vtysh_gets ("Repeat count [5]: ");
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
      line = vtysh_gets ("Datagram size [100]: ");
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
      line = vtysh_gets ("Timeout in seconds [2]: ");
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
  line = vtysh_gets ("Extended commands [n]: ");
  SKIP_WHITE_SPACE(line);
  if (*line != '\0')
    {
      SKIP_WHITE_SPACE(line);

      /* Process extended commands. */
      if (line[0] == 'y')
        {
          /* Process extended source address or interface. */
          line = vtysh_gets ("Source address or interface: ");
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
          line = vtysh_gets ("Type of service [0]: ");
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
              line = vtysh_gets ("Set DF bit in IP header? [no]: ");
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
              line = vtysh_gets ("Data pattern [0xABCD]: ");
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
          line = vtysh_gets ("Output Interface: ");
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
  pal_ping (i, argv, &argco, &argvo);

  /* Execute command. */
  vtysh_execvp (argvo);

  /* Free argvo */
  XFREE (MTYPE_TMP, argvo);

  return CLI_SUCCESS;
}

/* Interactive mode of traceroute.  */
int
vtysh_interactive_traceroute ()
{
  char *line = NULL;
  char *argv[25];
  int i = 0;
  int type = -1;
#define IPADDR_SIZE            32
  char ipaddr_buf[IPADDR_SIZE];
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

  /* For getopt to work. */
  argv[i++] = "traceroute";

  /* Process protocol. */
  line = vtysh_gets ("Protocol [ip]: ");
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
  line = vtysh_gets ("Target IP address: ");
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
      ret = get_addr_by_dns (family, line, ipaddr_buf, IPADDR_SIZE);
      if (ret < 0)
        {
          printf ("%% Bad IP address or hostname\n");
          return -1;
        }

      argv [i++] = TRACEROUTE_ADDRESS_OPTION;
      argv [i++] = ipaddr_buf;
    }

  /* Process source address. */
  line = vtysh_gets ("Source address: ");
  SKIP_WHITE_SPACE(line);
  if (*line != '\0')
    {
      struct prefix_ipv4 cp;

      SKIP_WHITE_SPACE(line);

      ret = str2prefix_ipv4 (line, &cp);
      if (ret <= 0)
        {
          printf ("%% Invalid source address\n");
          return -1;
        }

      argv[i++] = TRACEROUTE_SRC_ADDR_OPTION;
      pal_mem_cpy (ipaddr_buf, line, pal_strlen (line) + 1);
      argv [i++] = ipaddr_buf;
    }

  /* Process numeric display for addresses. */
  done = 0;
  while (!done)
    {
      line = vtysh_gets ("Numeric display [n]: ");
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
      line = vtysh_gets ("Timeout in seconds [2]: ");
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
      line = vtysh_gets ("Probe count [3]: ");
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
      line = vtysh_gets ("Maximum time to live [30]: ");
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
      line = vtysh_gets ("Port Number [33434]: ");
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
  pal_traceroute (i, argv, &argco, &argvo);

  /* Execute command. */
  vtysh_execvp (argvo);

  /* Free argvo. */
  XFREE (MTYPE_TMP, argvo);

  return CLI_SUCCESS;
}

/* Start shell.  */
CLI (vtysh_start_shell,
     vtysh_start_shell_cli,
     "start-shell",
     "Start shell")
{
  vtysh_exec (SHELL_PROG_NAME, NULL);
  return CLI_SUCCESS;
}

/* Ping.  */
CLI (vtysh_ping,
     vtysh_ping_cli,
     "ping WORD",
     "Send echo messages",
     "Ping destination address or hostname")
{
  char *argvi[6];
  int argco;
  char **argvo;

  argvi[0] = "ping";
  argvi[1] = PING_FAMILY_OPTION;
  argvi[2] = PING_FAMILY_IPV4;
  argvi[3] = PING_ADDRESS_OPTION;
  argvi[4] = argv[0];
  argvi[5] = NULL;

  /* Map ping options. */
  pal_ping (5, argvi, &argco, &argvo);

  /* Execute ping. */
  vtysh_execvp (argvo);

  /* Free argvo. */
  XFREE (MTYPE_TMP, argvo);

  return CLI_SUCCESS;
}

ALI (vtysh_ping,
     vtysh_ping_ip_cli,
     "ping ip WORD",
     "Send echo messages",
     "IP echo",
     "Ping destination address or hostname");

CLI (vtysh_ping_interactive,
     vtysh_ping_interactive_cli,
     "ping",
     "Send echo messages")
{
  vtysh_interactive_ping ();
  return CLI_SUCCESS;
}

/* Traceroute.  */
CLI (vtysh_traceroute,
     vtysh_traceroute_cli,
     "traceroute WORD",
     "Trace route to destination",
     "Trace route to destination address or hostname")
{
  char *argvi[6];
  int argco;
  char **argvo;

  argvi[0] = "traceroute";
  argvi[1] = TRACEROUTE_FAMILY_OPTION;
  argvi[2] = TRACEROUTE_FAMILY_IPV4;
  argvi[3] = TRACEROUTE_ADDRESS_OPTION;
  argvi[4] = argv[0];
  argvi[5] = NULL;

  /* Map traceroute options. */
  pal_traceroute (5, argvi, &argco, &argvo);

  /* Execute traceroute. */
  vtysh_execvp (argvo);

  /* Free argvo. */
  XFREE (MTYPE_TMP, argvo);

  return CLI_SUCCESS;
}

ALI (vtysh_traceroute,
     vtysh_traceroute_ip_cli,
     "traceroute ip WORD",
     "Trace route to destination",
     "IP Trace",
     "Trace route to destination address or hostname");

     CLI (vtysh_traceroute_interactive,
          vtysh_traceroute_interactive_cli,
          "traceroute",
          "Trace route to destination")
{
  vtysh_interactive_traceroute ();
  return CLI_SUCCESS;
}

#ifdef HAVE_IPV6
CLI (vtysh_ping_ipv6,
     vtysh_ping_ipv6_cli,
     "ping ipv6 WORD (|IFNAME)",
     "Send echo messages",
     "IPv6 echo",
     "Ping destination address or hostname",
     "Interface's name")
{
  struct prefix_ipv6 p;
  char *line = NULL;
  int ret;
  char *argvi[8];
  int argco;
  char **argvo;

  argvi[0] = "ping";

  if (argc == 2)
    {
      argvi[1] = PING_FAMILY_OPTION;
      argvi[2] = PING_FAMILY_IPV6;
      argvi[3] = PING_ADDRESS_OPTION;
      argvi[4] = argv[0];
      argvi[5] = PING_INTERFACE_OPTION;
      argvi[6] = argv[1];
      argvi[7] = NULL;

      /* Map ping options. */
      pal_ping (7, argvi, &argco, &argvo);

      /* Execute ping. */
      vtysh_execvp (argvo);

      /* Free argvo. */
      XFREE (MTYPE_TMP, argvo);
    }
  else
    {
      /* For link-local addresses accept interface parameter. */
      ret = str2prefix_ipv6 (argv[0], &p);
      if (ret > 0)
        {
          if (IN6_IS_ADDR_LINKLOCAL(&p.prefix))
            {
              line = vtysh_gets ("Output Interface: ");
              SKIP_WHITE_SPACE(line);
              if (*line == '\0')
                {
                  cli_out (cli, "%% Interface required\n");
                  return CLI_ERROR;
                }

              argvi[1] = PING_FAMILY_OPTION;
              argvi[2] = PING_FAMILY_IPV6;
              argvi[3] = PING_ADDRESS_OPTION;
              argvi[4] = argv[0];
              argvi[5] = PING_INTERFACE_OPTION;
              argvi[6] = line;
              argvi[7] = NULL;

              /* Map ping options. */
              pal_ping (7, argvi, &argco, &argvo);

              /* Execute ping. */
              vtysh_execvp (argvo);

              /* Free argvo. */
              XFREE (MTYPE_TMP, argvo);
            }
          else
            {
              argvi[1] = PING_FAMILY_OPTION;
              argvi[2] = PING_FAMILY_IPV6;
              argvi[3] = PING_ADDRESS_OPTION;
              argvi[4] = argv[0];
              argvi[5] = NULL;

              /* Map ping options. */
              pal_ping (5, argvi, &argco, &argvo);

              /* Execute ping. */
              vtysh_execvp (argvo);

              /* Free argvo. */
              XFREE (MTYPE_TMP, argvo);
            }
        }
      else
        {
          argvi[1] = PING_FAMILY_OPTION;
          argvi[2] = PING_FAMILY_IPV6;
          argvi[3] = PING_ADDRESS_OPTION;
          argvi[4] = argv[0];
          argvi[5] = NULL;

          /* Map ping options. */
          pal_ping (5, argvi, &argco, &argvo);

          /* Execute ping. */
          vtysh_execvp (argvo);

          /* Free argvo. */
          XFREE (MTYPE_TMP, argvo);
        }
    }
  return CLI_SUCCESS;
}

CLI (vtysh_traceroute_ipv6,
     vtysh_traceroute_ipv6_cli,
     "traceroute ipv6 WORD",
     "Trace route to destination",
     "IPv6 trace",
     "Trace route to destination address or hostname")
{
  char *argvi[6];
  int argco;
  char **argvo;

  argvi[0] = "traceroute";
  argvi[1] = TRACEROUTE_FAMILY_OPTION;
  argvi[2] = TRACEROUTE_FAMILY_IPV6;
  argvi[3] = TRACEROUTE_ADDRESS_OPTION;
  argvi[4] = argv[0];
  argvi[5] = NULL;

  /* Map traceroute options. */
  pal_traceroute (5, argvi, &argco, &argvo);

  /* Execute traceroute. */
  vtysh_execvp (argvo);

  /* Free argvo. */
  XFREE (MTYPE_TMP, argvo);

  return CLI_SUCCESS;
}
#endif /* HAVE_IPV6 */

/* Logout.  */
CLI (vtysh_logout,
     vtysh_logout_cli,
     "logout",
     "Exit from the EXEC")
{
  vtysh_exit (0);

  /* Not reached.  */
  return CLI_SUCCESS;
}

CLI (vtysh_do,
     vtysh_do_cli,
     "do LINE",
     "To run exec commands in config mode",
     " Exec Command")
{
  char *input;

  /* This is unique command.  */
  input = argv[0];

  vtysh_parse (input, EXEC_MODE);

  XFREE (MTYPE_TMP, input);

  return CLI_SUCCESS;
}

/* IMI commands.  */
CLI (vtysh_line_privilege,
     vtysh_line_privilege_cli,
     "privilege <0-15>",
     "Set privilege",
     "Privilege level")
{
  u_char level;

  CLI_GET_INTEGER ("privilege level", level, argv[0]);
  cli->privilege = level;

  return CLI_SUCCESS;
}

/* Set time out value. */
void
vtysh_exec_timeout (char *min_str, char *sec_str)
{
  u_int32_t timeout = 0;

  if (min_str)
    {
      timeout = pal_strtou32 (min_str, NULL, 10);
      timeout *= 60;
    }
  if (sec_str)
    timeout += pal_strtou32 (sec_str, NULL, 10);

  vtysh_timeout_set (timeout);
}

CLI (vtysh_line_exec_timeout,
     vtysh_line_exec_timeout_cli,
     "exec-timeout <0-35791>",
     "Set timeout value",
     "Timeout value in minutes")
{
  vtysh_exec_timeout (argv[0], NULL);
  return CLI_SUCCESS;
}

CLI (vtysh_line_exec_timeout_sec,
     vtysh_line_exec_timeout_sec_cli,
     "exec-timeout <0-35791> <0-2147483>",
     "Set timeout value",
     "Timeout value in minutes",
     "Timeout in seconds")
{
  vtysh_exec_timeout (argv[0], argv[1]);
  return CLI_SUCCESS;
}

/* Banner configuration.  */
CLI (vtysh_banner_motd_custom,
     vtysh_banner_motd_custom_cli,
     "banner motd LINE",
     "Define a login banner",
     "Set Message of the Day banner",
     "Custom string")
{
  printf ("%s\n", argv[0]);

  return CLI_SUCCESS;
}

/* Enable.  */
CLI (vtysh_enable,
     vtysh_enable_cli,
     "enable",
     "Turn on privileged mode command")
{
  char *line;
  int failure;

  if (cli->privilege == PRIVILEGE_MAX)
    return CLI_SUCCESS;

  if (! host_enable_password_check (cli->vr->host, NULL))
    {
      cli->auth = 0;

      for (failure = 0; failure < 3; failure++)
        {
          line = getpass ("Password: ");

          if (host_enable_password_check (cli->vr->host, line))
            {
              cli->privilege = PRIVILEGE_MAX;
              return CLI_SUCCESS;
            }
        }
      printf ("%% Bad passwords\n\n");
      return CLI_ERROR;
    }
  else
    {
      cli->privilege = PRIVILEGE_MAX;
      return CLI_SUCCESS;
    }

  return CLI_SUCCESS;
}

/* Disable.  */
CLI (vtysh_disable,
     vtysh_disable_cli,
     "disable",
     "Turn off privileged mode command")
{
  cli->privilege = 1;
  return CLI_SUCCESS;
}

/* "terminal length"  */
CLI (vtysh_terminal_length,
     vtysh_terminal_length_cli,
     "terminal length <0-512>",
     "Set terminal line parameters",
     "Set number of lines on a screen",
     "Number of lines on screen (0 for no pausing)")
{
  int lines;

  CLI_GET_INTEGER ("length", lines, argv[0]);
  cli->lines = lines;

  return CLI_SUCCESS;
}

CLI (vtysh_terminal_no_length,
     vtysh_terminal_no_length_cli,
     "terminal no length (<0-512>|)",
     "Unset terminal line parameters",
     CLI_NO_STR,
     "Unset number of lines on a screen",
     "Number of lines on screen (0 for no pausing)")
{
  cli->lines = -1;
  return CLI_SUCCESS;
}

/* "show history"  */
CLI (vtysh_show_history,
     vtysh_show_history_cli,
     "show history",
     CLI_SHOW_STR,
     "Display the session command history")
{
  vtysh_history_show ();
  return CLI_SUCCESS;
}

/* Show version.  */
CLI (vtysh_show_version,
     vtysh_show_version_cli,
     "show version",
     CLI_SHOW_STR,
     "Display PacOS version")
{
  char buf1[50], buf2[50];
  char *str = PLATFORM;

  cli_out (cli, "PacOS version %s%s%s %s\n",
           pacos_version (buf1, 50),
           pal_strlen (str) ? " " : "", str,
           BUILDDATE);
  cli_out (cli, " Build # is %s on host %s\n",
           pacos_buildno (buf2, 50),
           host_name);
  cli_out (cli, " %s\n", APN_COPYRIGHT);

#ifdef HAVE_LICENSE_MGR
  cli_out (cli, " %s\n",THIRD_PARTY_SOFTWARE_COPYRIGHT);
#endif /* HAVE_LICENSE_MGR */

  return CLI_SUCCESS;
}

CLI (vtysh_service_advanced_vty,
     vtysh_service_advanced_vty_cli,
     "service advanced-vty",
     "Set up miscellaneous service",
     "Enable advanced mode vty interface")
{
  struct host *host = cli->vr->host;

  SET_FLAG (host->flags, HOST_ADVANCED_VTYSH);

  return CLI_SUCCESS;
}

CLI (no_vtysh_service_advanced_vty,
     no_vtysh_service_advanced_vty_cli,
     "no service advanced-vty",
     CLI_NO_STR,
     "Set up miscellaneous service",
     "Enable advanced mode vty interface")
{
  struct host *host = cli->vr->host;

  UNSET_FLAG (host->flags, HOST_ADVANCED_VTYSH);

  return CLI_SUCCESS;
}

void
vtysh_cli_init ()
{
  ctree = vtyshm->ctree;

  ctree->mode = EXEC_MODE;

  /* Install CLI mode commands. */
  vtysh_command_init ();

  /* Install extracted CLI commands.  */
  imi_extracted_cmd_init (ctree);

  /* EXEC mode commands.  */
  cli_install (ctree, EXEC_MODE, &vtysh_logout_cli);

  /* ping and traceroute.  */
  cli_install (ctree, EXEC_MODE, &vtysh_ping_cli);
  cli_install (ctree, EXEC_MODE, &vtysh_ping_ip_cli);
  cli_install (ctree, EXEC_MODE, &vtysh_ping_interactive_cli);
  cli_install (ctree, EXEC_MODE, &vtysh_traceroute_cli);
  cli_install (ctree, EXEC_MODE, &vtysh_traceroute_ip_cli);
  cli_install (ctree, EXEC_MODE, &vtysh_traceroute_interactive_cli);
#ifdef HAVE_IPV6
  cli_install (ctree, EXEC_MODE, &vtysh_ping_ipv6_cli);
  cli_install (ctree, EXEC_MODE, &vtysh_traceroute_ipv6_cli);
#endif /* HAVE_IPV6 */

  /* Show history. */
  cli_install (ctree, EXEC_MODE, &vtysh_show_history_cli);

  /* Privileged EXEC mode commands.  */
  cli_install (ctree, EXEC_PRIV_MODE, &vtysh_start_shell_cli);

  /* Do command.  */
  cli_install (ctree, CONFIG_MODE, &vtysh_do_cli);

  /* vtysh mode.  */
  cli_install_imi (ctree, EXEC_MODE, PM_EMPTY, &vtysh_enable_cli);
  cli_install_imi (ctree, EXEC_MODE, PM_EMPTY, &vtysh_disable_cli);

  /* version.  */
  cli_install (ctree, EXEC_MODE, &vtysh_show_version_cli);

  /* Terminal length.  */
  cli_install (ctree, EXEC_MODE, &vtysh_terminal_length_cli);
  cli_install (ctree, EXEC_MODE, &vtysh_terminal_no_length_cli);

  /* Service advanced-vty.  */
  cli_install (ctree, EXEC_MODE, &vtysh_terminal_no_length_cli);

  /* Advanced vty.  */
  cli_install (ctree, CONFIG_MODE, &vtysh_service_advanced_vty_cli);
  cli_install (ctree, CONFIG_MODE, &no_vtysh_service_advanced_vty_cli);

  /* Host vtysh commands.  */
  host_vtysh_cli_init (ctree);
}
