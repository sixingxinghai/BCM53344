/*
 *
 *  Copyright (C) 2002,   All Rights Reserved.
 *
 *  pal_fw.c -- PacOS PAL NAT/Firewall operations definitions
 *               for Linux
 */

#include "pal.h"

#if (defined HAVE_ACL || defined HAVE_NAT)
#include "linklist.h"
#include "if.h"
#include "filter.h"
#include "snprintf.h"

#include "imi/pal_fw.h"
#include "imi_fw.h"

static const char *opt_str[]=
{
  [OPT_NONE]           "",
  [OPT_SOURCE]         "-s",
  [OPT_DESTINATION]    "-d",
  [OPT_PROTOCOL]       "-p",
  [OPT_VIANAMEIN]      "-i",
  [OPT_VIANAMEOUT]     "-o",
  [OPT_SPORT]          "--sport",
  [OPT_DPORT]          "--dport",
  [OPT_TO_SOURCE]      "--to-source",
  [OPT_TO_DESTINATION] "--to-destination"
};

extern char *filter_prot_str(s_int16_t, char *);

result_t
pal_fw_start (struct lib_globals *zg)
{
  return 0;
}

result_t
pal_fw_stop (struct lib_globals *zg)
{
  return 0;
}

static void
get_protocol_opt (char *buf, int size, int proto)
{
  char proto_str[PROTO_STR_LEN];

  /* If not ANY_PROTO. */
  if (proto != ANY_PROTO)
    pal_snprintf (buf, size, "%s %s",
                  opt_str[OPT_PROTOCOL],
                  filter_prot_str(proto, proto_str));
  else
    pal_strcpy (buf, "");
}

static void
get_port_opt (char *buf, int size, int opt_type, int port_lo, int port, int op)
{
  if (port)
    {
      switch (op)
        {
        case EQUAL:
          pal_snprintf (buf, size, "%s %d", opt_str[opt_type], port);
          break;
        case NOT_EQUAL:
          pal_snprintf (buf, size, "%s ! %d", opt_str[opt_type], port);
          break;
        case LESS_THAN:
          pal_snprintf (buf, size, "%s ! %d:%d", opt_str[opt_type], port, PORT_MAX);
          break;
        case GREATER_THAN:
          pal_snprintf (buf, size, "%s ! %d:%d", opt_str[opt_type], PORT_MIN, port);
          break;
        case RANGE:
          pal_snprintf (buf, size, "%s %d:%d", opt_str[opt_type], port_lo, port);
          break;
        }
    }
  else
    pal_strcpy (buf, "");
}

static int
get_prefix_str (char *buf, int size, int opt_type, struct prefix_am *p)
{
  int ret = 0;
  char str[PREFIX_ADDR_STR_SIZE];

  if (prefix_am_incl_all(p))
    pal_strcpy (buf, "");
  else
    {
      ret = prefix2str_am ((struct prefix_am *)p, str, size, '/');
      if (!ret)
        {
          pal_snprintf (buf, size, "%s %s", opt_str[opt_type], str);

          return 0;
        }
      else
        return -1;
    }
  return 0;

}

#ifdef HAVE_NAT
static result_t
pal_nat_inside_source_translate (int op,
                                 u_int16_t proto,
                                 struct prefix_am4 *src, struct prefix_am4 *dst,
                                 u_int16_t sport_lo, u_int16_t sport, int sport_op,
                                 u_int16_t dport_lo, u_int16_t dport, int dport_op,
                                 char *via_in, char *via_out,
                                 struct pal_in4_addr *min_ip,
                                 struct pal_in4_addr *max_ip,
                                 u_int16_t min_port, u_int16_t max_port,
                                 u_int16_t rulenum)
{
  /* SNATing. */
  char src_str[PREFIX_AM_STR_SIZE+1], dst_str[PREFIX_AM_STR_SIZE+1];
  char min_str[PREFIX_ADDR_STR_SIZE], max_str[PREFIX_ADDR_STR_SIZE];
  char sport_str[PORT_STR_LEN], dport_str[PORT_STR_LEN], port_str[PORT_STR_LEN];
  char proto_str[PROTO_STR_LEN];
  char buf[255];
  int ret = 0;

  /* Get Source address. */
  ret = get_prefix_str (src_str, PREFIX_AM_STR_SIZE, OPT_SOURCE, 
                        (struct prefix_am *)src);
  if (ret < 0)
    pal_strcpy (src_str, "");

  /* Get Destination address. */
  ret = get_prefix_str (dst_str, PREFIX_AM_STR_SIZE, OPT_DESTINATION, 
                        (struct prefix_am *)dst);
  if (ret < 0)
    pal_strcpy (dst_str, "");

  /* Get source port. */
  get_port_opt (sport_str, PORT_STR_LEN, OPT_SPORT, sport_lo, sport, sport_op);

  /* Get destination port. */
  get_port_opt (dport_str, PORT_STR_LEN, OPT_DPORT, dport_lo, dport, dport_op);

  /* Get protocol. */
  get_protocol_opt (proto_str, PROTO_STR_LEN, proto);

  zsnprintf (min_str, PREFIX_ADDR_STR_SIZE, "%r", min_ip);
  zsnprintf (max_str, PREFIX_ADDR_STR_SIZE, "%r", max_ip);

  if (min_port && max_port)
    pal_snprintf (port_str, PORT_STR_LEN, ":%d-%d", min_port, max_port);
  else
    pal_strcpy (port_str, "");

  if (op)
    {
      if (rulenum == 0)
        pal_snprintf (buf, 255, "%s %s %s %s %s %s %s %s %s %s %s-%s%s %s %s",
                      IPTABLES, NAT_TABLE, ADD_POSTROUTING, SNAT,
                      proto_str,
                      src_str, dst_str,
                      sport_str, dport_str,
                      opt_str[OPT_TO_SOURCE], min_str, max_str, port_str,
                      opt_str[OPT_VIANAMEOUT], via_out);
      else
        pal_snprintf (buf, 255, "%s %s %s %d %s %s %s %s %s %s %s %s-%s%s %s %s",
                      IPTABLES, NAT_TABLE, INSERT_POSTROUTING, rulenum, SNAT,
                      proto_str,
                      src_str, dst_str,
                      sport_str, dport_str,
                      opt_str[OPT_TO_SOURCE], min_str, max_str, port_str,
                      opt_str[OPT_VIANAMEOUT], via_out);
    }
  else
    pal_snprintf (buf, 255, "%s %s %s %s %s %s %s %s %s %s %s-%s%s %s %s",
                  IPTABLES, NAT_TABLE, DEL_POSTROUTING, SNAT,
                  proto_str,
                  src_str, dst_str,
                  sport_str, dport_str,
                  opt_str[OPT_TO_SOURCE], min_str, max_str, port_str,
                  opt_str[OPT_VIANAMEOUT], via_out);

#ifndef DEBUG
 ret =  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  return ret;
}

static result_t
pal_nat_inside_destination_translate (int op,
                                      u_int16_t proto,
                                      struct prefix_am4 *src, struct prefix_am4 *dst,
                                      u_int16_t sport_lo, u_int16_t sport, int sport_op,
                                      u_int16_t dport_lo, u_int16_t dport, int dport_op,
                                      char *via_in, char *via_out,
                                      struct pal_in4_addr *min_ip,
                                      struct pal_in4_addr *max_ip,
                                      u_int16_t min_port, u_int16_t max_port,
                                      u_int16_t rulenum)
{
  /* DNATing. */
  char src_str[PREFIX_AM_STR_SIZE+1], dst_str[PREFIX_AM_STR_SIZE+1];
  char min_str[PREFIX_ADDR_STR_SIZE], max_str[PREFIX_ADDR_STR_SIZE];
  char sport_str[PORT_STR_LEN], dport_str[PORT_STR_LEN], port_str[PORT_STR_LEN];
  char proto_str[PROTO_STR_LEN];
  char buf[255];
  int ret = 0;

  /* Get Source address. */
  ret = get_prefix_str (src_str, PREFIX_AM_STR_SIZE, OPT_SOURCE, 
                        (struct prefix_am *)src);
  if (ret < 0)
    pal_strcpy (src_str, "");

  /* Get Destination address. */
  ret = get_prefix_str (dst_str, PREFIX_AM_STR_SIZE, OPT_DESTINATION, 
                        (struct prefix_am *)dst);
  if (ret < 0)
    pal_strcpy (dst_str, "");

  /* Get source port. */
  get_port_opt (sport_str, PORT_STR_LEN, OPT_SPORT, sport_lo, sport, sport_op);

  /* Get destination port. */
  get_port_opt (dport_str, PORT_STR_LEN, OPT_DPORT, dport_lo, dport, dport_op);

  /* Get protocol. */
  get_protocol_opt (proto_str, PROTO_STR_LEN, proto);

  zsnprintf (min_str, PREFIX_ADDR_STR_SIZE, "%r", min_ip);
  zsnprintf (max_str, PREFIX_ADDR_STR_SIZE, "%r", max_ip);

  if (min_port && max_port)
    pal_snprintf (port_str, PORT_STR_LEN, ":%d-%d", min_port, max_port);
  else
    pal_strcpy (port_str, "");

  if (op)
    {
      if (rulenum == 0)
        pal_snprintf (buf, 255, "%s %s %s %s %s %s %s %s %s %s %s-%s%s %s %s",
                      IPTABLES, NAT_TABLE, ADD_PREROUTING, DNAT,
                      proto_str,
                      src_str, dst_str,
                      sport_str, dport_str,
                      opt_str[OPT_TO_DESTINATION], min_str, max_str, port_str,
                      opt_str[OPT_VIANAMEIN], via_in);
      else
        pal_snprintf (buf, 255, "%s %s %s %d %s %s %s %s %s %s %s %s-%s%s %s %s",
                      IPTABLES, NAT_TABLE, INSERT_PREROUTING, rulenum, DNAT,
                      proto_str,
                      src_str, dst_str,
                      sport_str, dport_str,
                      opt_str[OPT_TO_DESTINATION], min_str, max_str, port_str,
                      opt_str[OPT_VIANAMEIN], via_in);
    }
  else
    pal_snprintf (buf, 255, "%s %s %s %s %s %s %s %s %s %s %s-%s%s %s %s",
                  IPTABLES, NAT_TABLE, DEL_PREROUTING, DNAT,
                  proto_str,
                  src_str, dst_str,
                  sport_str, dport_str,
                  opt_str[OPT_TO_DESTINATION], min_str, max_str, port_str,
                  opt_str[OPT_VIANAMEIN], via_in);

#ifndef DEBUG
  ret = system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  return ret;
}

static result_t
pal_nat_outside_source_translate (int op,
                                  u_int16_t proto,
                                  struct prefix_am4 *src, struct prefix_am4 *dst,
                                  u_int16_t sport_lo, u_int16_t sport, int sport_op,
                                  u_int16_t dport_lo, u_int16_t dport, int dport_op,
                                  char *via_in, char *via_out,
                                  struct pal_in4_addr *min_ip,
                                  struct pal_in4_addr *max_ip,
                                  u_int16_t min_port, u_int16_t max_port,
                                  u_int16_t rulenum)
{
  /* SNATing. */
  char src_str[PREFIX_AM_STR_SIZE+1], dst_str[PREFIX_AM_STR_SIZE+1];
  char min_str[PREFIX_ADDR_STR_SIZE], max_str[PREFIX_ADDR_STR_SIZE];
  char sport_str[PORT_STR_LEN], dport_str[PORT_STR_LEN], port_str[PORT_STR_LEN];
  char proto_str[PROTO_STR_LEN];
  char buf[255];
  int ret = 0;

  /* Get Source address. */
  ret = get_prefix_str (src_str, PREFIX_ADDR_STR_SIZE, OPT_SOURCE, 
                        (struct prefix_am *)src);
  if (ret < 0)
    pal_strcpy (src_str, "");

  /* Get Destination address. */
  ret = get_prefix_str (dst_str, PREFIX_ADDR_STR_SIZE, OPT_DESTINATION, 
                        (struct prefix_am *)dst);
  if (ret < 0)
    pal_strcpy (dst_str, "");

  /* Get source port. */
  get_port_opt (sport_str, PORT_STR_LEN, OPT_SPORT, sport_lo, sport, sport_op);

  /* Get destination port. */
  get_port_opt (dport_str, PORT_STR_LEN, OPT_DPORT, dport_lo, dport, dport_op);

  /* Get protocol. */
  get_protocol_opt (proto_str, PROTO_STR_LEN, proto);

  zsnprintf (min_str, PREFIX_ADDR_STR_SIZE, "%r", min_ip);
  zsnprintf (max_str, PREFIX_ADDR_STR_SIZE, "%r", max_ip);

  if (min_port && max_port)
    pal_snprintf (port_str, PORT_STR_LEN, ":%d-%d", min_port, max_port);
  else
    pal_strcpy (port_str, "");

  if (op)
    {
      if (rulenum == 0)
        pal_snprintf (buf, 255, "%s %s %s %s %s %s %s %s %s %s %s-%s%s %s %s",
                      IPTABLES, NAT_TABLE, ADD_POSTROUTING, SNAT,
                      proto_str,
                      src_str, dst_str,
                      sport_str, dport_str,
                      opt_str[OPT_TO_SOURCE], min_str, max_str, port_str,
                      opt_str[OPT_VIANAMEOUT], via_in);
      else
        pal_snprintf (buf, 255, "%s %s %s %d %s %s %s %s %s %s %s %s-%s%s %s %s",
                      IPTABLES, NAT_TABLE, INSERT_POSTROUTING, rulenum, SNAT,
                      proto_str,
                      src_str, dst_str,
                      sport_str, dport_str,
                      opt_str[OPT_TO_SOURCE], min_str, max_str, port_str,
                      opt_str[OPT_VIANAMEOUT], via_in);
    }
  else
    pal_snprintf (buf, 255, "%s %s %s %s %s %s %s %s %s %s %s-%s%s %s %s",
                  IPTABLES, NAT_TABLE, DEL_POSTROUTING, SNAT,
                  proto_str,
                  src_str, dst_str,
                  sport_str, dport_str,
                  opt_str[OPT_TO_SOURCE], min_str, max_str, port_str,
                  opt_str[OPT_VIANAMEOUT], via_in);

#ifndef DEBUG
 ret =  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  return ret;
}

static result_t
pal_nat_outside_destination_translate (int op,
                                       u_int16_t proto,
                                       struct prefix_am4 *src, struct prefix_am4 *dst,
                                       u_int16_t sport_lo, u_int16_t sport, int sport_op,
                                       u_int16_t dport_lo, u_int16_t dport, int dport_op,
                                       char *via_in, char *via_out,
                                       struct pal_in4_addr *min_ip,
                                       struct pal_in4_addr *max_ip,
                                       u_int16_t min_port, u_int16_t max_port,
                                       u_int16_t rulenum)
{
  /* SNATing. */
  char src_str[PREFIX_AM_STR_SIZE+1], dst_str[PREFIX_AM_STR_SIZE+1];
  char min_str[PREFIX_ADDR_STR_SIZE], max_str[PREFIX_ADDR_STR_SIZE];
  char sport_str[PORT_STR_LEN], dport_str[PORT_STR_LEN], port_str[PORT_STR_LEN];
  char proto_str[PROTO_STR_LEN];
  char buf[255];
  int ret = 0;

  /* Get Source address. */
  ret = get_prefix_str (src_str, PREFIX_AM_STR_SIZE, OPT_SOURCE, 
                        (struct prefix_am *)src);
  if (ret < 0)
    pal_strcpy (src_str, "");

  /* Get Destination address. */
  ret = get_prefix_str (dst_str, PREFIX_AM_STR_SIZE, OPT_DESTINATION, 
                        (struct prefix_am *)dst);
  if (ret < 0)
    pal_strcpy (dst_str, "");

  /* Get source port. */
  get_port_opt (sport_str, PORT_STR_LEN, OPT_SPORT, sport_lo, sport, sport_op);

  /* Get destination port. */
  get_port_opt (dport_str, PORT_STR_LEN, OPT_DPORT, dport_lo, dport, dport_op);

  /* Get protocol. */
  get_protocol_opt (proto_str, PROTO_STR_LEN, proto);

  zsnprintf (min_str, PREFIX_ADDR_STR_SIZE, "%r", min_ip);
  zsnprintf (max_str, PREFIX_ADDR_STR_SIZE, "%r", max_ip);

  if (min_port && max_port)
    pal_snprintf (port_str, PORT_STR_LEN, ":%d-%d", min_port, max_port);
  else
    pal_strcpy (port_str, "");

  if (op)
    {
      if (rulenum == 0)
        pal_snprintf (buf, 255, "%s %s %s %s %s %s %s %s %s %s %s-%s%s %s %s",
                      IPTABLES, NAT_TABLE, ADD_PREROUTING, DNAT,
                      proto_str,
                      src_str, dst_str,
                      sport_str, dport_str,
                      opt_str[OPT_TO_DESTINATION], min_str, max_str, port_str,
                      opt_str[OPT_VIANAMEIN], via_out);
      else
        pal_snprintf (buf, 255, "%s %s %s %d %s %s %s %s %s %s %s %s-%s%s %s %s",
                      IPTABLES, NAT_TABLE, INSERT_PREROUTING, rulenum, DNAT,
                      proto_str,
                      src_str, dst_str,
                      sport_str, dport_str,
                      opt_str[OPT_TO_DESTINATION], min_str, max_str, port_str,
                      opt_str[OPT_VIANAMEIN], via_out);
    }
  else
    pal_snprintf (buf, 255, "%s %s %s %s %s %s %s %s %s %s %s-%s%s %s %s",
                  IPTABLES, NAT_TABLE, DEL_PREROUTING, DNAT,
                  proto_str,
                  src_str, dst_str,
                  sport_str, dport_str,
                  opt_str[OPT_TO_DESTINATION],
                  min_str, max_str, port_str,
                  opt_str[OPT_VIANAMEIN], via_out);

#ifndef DEBUG
 ret =  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  return ret;
}

result_t
pal_nat_translate_rule (int op, int direction, int scope,
                        u_int16_t proto,
                        struct prefix_am4 *src, struct prefix_am4 *dst,
                        u_int16_t sport_lo, u_int16_t sport,int sport_op,
                        u_int16_t dport_lo,u_int16_t dport, int dport_op,
                        char *via_in, char *via_out,
                        struct pal_in4_addr *min_ip,
                        struct pal_in4_addr *max_ip,
                        u_int16_t min_port, u_int16_t max_port,
                         u_int16_t rulenum)
{
 result_t res = 0;
  switch (direction)
    {
    case IMI_NAT_INSIDE:
      {
        switch (scope)
          {
          case IMI_NAT_SOURCE:
            {
              pal_nat_inside_source_translate (op, proto, src, dst,
                                               sport_lo, sport, sport_op,
                                               dport_lo, dport, dport_op,
                                               via_in, via_out,
                                               min_ip, max_ip, min_port, max_port,
                                               rulenum);
            }
            break;
          case IMI_NAT_DESTINATION:
            {
              pal_nat_inside_destination_translate (op, proto, src,dst,
                                                    sport_lo, sport, sport_op,
                                                    dport_lo, dport, dport_op,
                                                    via_in, via_out, min_ip, max_ip,
                                                    min_port, max_port,
                                                    rulenum);
            }
            break;
          default:
            break;
          }
      }
      break;
    case IMI_NAT_OUTSIDE:
      {
        switch (scope)
          {
          case IMI_NAT_SOURCE:
            {
              pal_nat_outside_source_translate (op, proto, src, dst,
                                                sport_lo, sport, sport_op,
                                                dport_lo, dport, dport_op,
                                                via_in, via_out, min_ip, max_ip,
                                                min_port, max_port,
                                                rulenum);
            }
            break;
          case IMI_NAT_DESTINATION:
            {
              pal_nat_outside_destination_translate (op, proto, src, dst,
                                                     sport_lo, sport, sport_op,
                                                     dport_lo, dport, dport_op,
                                                     via_in, via_out, min_ip, max_ip,
                                                     min_port, max_port,
                                                     rulenum);
            }
            break;
          }
      }
      break;
    default:
      break;
    }

  return res;
}

result_t
pal_nat_clear_all_translations (void)
{
  char buf[255];

  pal_snprintf (buf, 255, "%s %s %s", IPTABLES, NAT_TABLE, FLUSH_PREROUTING);
#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  pal_snprintf (buf, 255, "%s %s %s", IPTABLES, NAT_TABLE, FLUSH_POSTROUTING);
#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  pal_snprintf (buf, 255, "%s %s %s", IPTABLES, NAT_TABLE, FLUSH_OUTPUT);
#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  return 0;
}

int
pal_nat_translation_timeout_set(int timeout)
{
  char buf[PAL_NAT_BUF_SIZE];

  pal_snprintf (buf, PAL_NAT_BUF_SIZE, "%s %s=%d", SYSCTL_WRITE,
                GENERIC_TIMEOUT, timeout);

#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  return RESULT_OK;
}

int
pal_nat_translation_icmp_timeout_set(int timeout)
{
  char buf[PAL_NAT_BUF_SIZE];

  pal_snprintf (buf, PAL_NAT_BUF_SIZE, "%s %s=%d", SYSCTL_WRITE, ICMP_TIMEOUT,
                timeout);

#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  return RESULT_OK;
}

int
pal_nat_translation_tcp_timeout_set(int timeout)
{
  char buf[PAL_NAT_BUF_SIZE];

  pal_snprintf (buf, PAL_NAT_BUF_SIZE, "%s %s=%d", SYSCTL_WRITE, TCP_TIMEOUT,
                timeout);

#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  return RESULT_OK;
}

int
pal_nat_translation_tcp_fin_timeout_set(int timeout)
{
  char buf[PAL_NAT_BUF_SIZE];

  pal_snprintf (buf, PAL_NAT_BUF_SIZE, "%s %s=%d", SYSCTL_WRITE,
                TCP_FIN_TIMEOUT, timeout);

#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  return RESULT_OK;
}

int
pal_nat_translation_udp_timeout_set(int timeout)
{
  char buf[PAL_NAT_BUF_SIZE];

  pal_snprintf (buf, PAL_NAT_BUF_SIZE, "%s %s=%d", SYSCTL_WRITE, UDP_TIMEOUT,
                timeout);

#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  return RESULT_OK;
}

int
pal_nat_translation_timeout_read (int *time_out)
{
  FILE *fp = NULL;
  char buf[PAL_NAT_BUF_SIZE];
  char *ptr = NULL;
  char  *delim = NULL;

  *time_out = 0;

  fp = fopen (_NAT_TRANSLATION_TIMEOUT, "r");
  if (fp == NULL)
    {
      return RESULT_ERROR;
    }

  ptr = fgets (buf, PAL_NAT_BUF_SIZE, fp);
  if (ptr)
   {
      ptr = strtok_r (buf, "=", &delim);
      ptr = strtok_r (NULL, "=", &delim);
      sscanf (ptr, "%d", time_out);
    }
  fclose(fp);
  return RESULT_OK;

}

int
pal_nat_translation_timeout_get(struct imi_nat_timeouts *timeout)
{
  char buf[PAL_NAT_BUF_SIZE];
  int ret_time_out = 0;

  if (timeout == NULL)
    return RESULT_ERROR;

  switch(timeout->flag)
  {
    case IMI_GENERIC_TIMEOUT:
    {
      pal_snprintf (buf, PAL_NAT_BUF_SIZE,
                    "%s %s > "_NAT_TRANSLATION_TIMEOUT"", SYSCTL_READ,
                    GENERIC_TIMEOUT);
      system (buf);
      pal_nat_translation_timeout_read(&ret_time_out);
      timeout->generic_timeout = ret_time_out;
    }
    break;
    case IMI_UDP_TIMEOUT:
    {
      pal_snprintf (buf, PAL_NAT_BUF_SIZE,
                    "%s %s > "_NAT_TRANSLATION_TIMEOUT"", SYSCTL_READ,
                    UDP_TIMEOUT);
      system (buf);
      pal_nat_translation_timeout_read(&ret_time_out);
      timeout->udp_timeout = ret_time_out;
    }
    break;
    case IMI_ICMP_TIMEOUT:
    {
      pal_snprintf (buf, PAL_NAT_BUF_SIZE,
                    "%s %s > "_NAT_TRANSLATION_TIMEOUT"",
                    SYSCTL_READ, ICMP_TIMEOUT);
      system (buf);

      pal_nat_translation_timeout_read(&ret_time_out);
      timeout->icmp_timeout = ret_time_out;
    }
   break;
   case IMI_TCP_TIMEOUT:
    {
      pal_snprintf (buf, PAL_NAT_BUF_SIZE,
                    "%s %s > "_NAT_TRANSLATION_TIMEOUT"",
                    SYSCTL_READ, TCP_TIMEOUT);
      system (buf);

      pal_nat_translation_timeout_read(&ret_time_out);
      timeout->tcp_timeout = ret_time_out;
    }
   break;
   case IMI_TCP_FIN_TIMEOUT:
    {
      pal_snprintf (buf, PAL_NAT_BUF_SIZE,
                    "%s %s > "_NAT_TRANSLATION_TIMEOUT"", SYSCTL_READ,
                    TCP_FIN_TIMEOUT);
      system (buf);
      pal_nat_translation_timeout_read(&ret_time_out);
      timeout->tcp_fin_timeout = ret_time_out;
    }
   break;
   default:
    return RESULT_ERROR;
  }

  return RESULT_OK;

}

result_t
pal_nat_clear_all_timeouts (void)
{
  char buf[PAL_NAT_BUF_SIZE];

  pal_snprintf (buf, PAL_NAT_BUF_SIZE,
                "%s %s=%d", SYSCTL_WRITE, GENERIC_TIMEOUT,
                DEFAULT_GENERIC_TIMEOUT);
#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

   pal_snprintf (buf, PAL_NAT_BUF_SIZE,
                 "%s %s=%d", SYSCTL_WRITE, UDP_TIMEOUT, DEFAULT_UDP_TIMEOUT);

#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  pal_snprintf (buf, PAL_NAT_BUF_SIZE, "%s %s=%d", SYSCTL_WRITE,
                TCP_TIMEOUT, DEFAULT_TCP_TIMEOUT);

#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

   pal_snprintf (buf, PAL_NAT_BUF_SIZE, "%s %s=%d", SYSCTL_WRITE,
                 TCP_FIN_TIMEOUT, DEFAULT_TCP_FIN_TIMEOUT);

#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  pal_snprintf (buf, PAL_NAT_BUF_SIZE, "%s %s=%d", SYSCTL_WRITE,
                ICMP_TIMEOUT, DEFAULT_ICMP_TIMEOUT);

#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  return 0;
}
#endif /* HAVE_NAT */

#ifdef HAVE_ACL
result_t
pal_filter_clear_all_rules (void)
{
  char buf[255];

  pal_snprintf (buf, 255, "%s %s %s", IPTABLES, FILTER_TABLE, FLUSH_INPUT);
#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  pal_snprintf (buf, 255, "%s %s %s", IPTABLES, FILTER_TABLE, FLUSH_OUTPUT);
#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  pal_snprintf (buf, 255, "%s %s %s", IPTABLES, FILTER_TABLE, FLUSH_FORWARD);
#ifndef DEBUG
  system (buf);
#else
  printf ("%s\n", buf);
#endif /* DEBUG */

  return 0;
}

  /* ICMP: icmp_type > 0 ICMP type 0 .. 255 */
static void
get_icmp_type_opt(char *buf, int size, int proto, int icmp_type)
{
  if (proto == ICMP_PROTO && icmp_type >= 0 && icmp_type <= 255 )
    pal_snprintf (buf, size, "--icmp-type %d", icmp_type);
  else
    pal_strcpy (buf, "");
}

/* TCP: ESTABLISHED */
static void
get_state_opt(char *buf, int size, int proto, bool_t estab)
{
  /* -m state --ctstate ESTABLISHED */

  if (proto == TCP_PROTO && estab)
    pal_snprintf (buf, size, "-m state --state ESTABLISHED");
  else
    pal_strcpy (buf, "");
}

/* < 0 -> 0..-lenght 0 -> length..65535 */
static void
get_length_opt(char *buf, int size, int length_lo, int length, int length_op)
{
  /* -m length --length <from>:<to> */
  if (length_op == LESS_THAN)
    {
      pal_snprintf (buf, size, "-m length --length 0:%d", length-1);
    }
  else if (length_op == GREATER_THAN)
    {
      pal_snprintf (buf, size, "-m length --length %d:65535", length+1);
    }
  else if (length_op == RANGE)
    {
      pal_snprintf (buf, size, "-m length --length %d:%d", length_lo, length);
    }
  else
    pal_strcpy (buf, "");
}

/* -1 -> not valid; filtering only few allowed. */
static void
get_tos_opt(char *buf, int size, int tos)
{
  /* -m tos --tos <tos> */
  switch(tos)
    {
    case TOS_NORMAL_SERVICE:
    case TOS_MIN_COST:
    case TOS_MAX_RELIABILITY:
    case TOS_MAX_THROUGHPUT:
    case TOS_MIN_DELAT:
      pal_snprintf (buf, size, "-m tos --tos %d", tos);
      break;
    default:
      pal_strcpy (buf, "");
    }
}

static void
get_fragments_opt(char *buf, int size, bool_t frag)
{
  if (frag)
    pal_strcpy (buf, "-f");
  else
    pal_strcpy (buf, "");
}

static void
get_target_str(char *buf, int size, int filter, bool_t log)
{
  if (log)
    {
      pal_strcpy(buf, "-j LOG");
    }
  else if (filter == FILTER_DENY)
    {
      pal_strcpy(buf, "-j DROP");
    }
  else if (filter == FILTER_PERMIT)
    {
      pal_strcpy(buf, "-j ACCEPT");
    }
  else
    {
      pal_strcpy (buf, "");
    }
};



static result_t
_pal_fw_translate_rule (int op, int scope, u_int16_t proto,
                       struct prefix_am *src, struct prefix_am *dst,
                       s_int16_t sport_lo,s_int16_t sport, int sport_op,
                       s_int16_t dport_lo,s_int16_t dport, int dport_op,
                       char *interface, int filter, int rulenum,
                       int icmp_type,   /* ICMP type 0 .. 255 */
                       bool_t established, /* ESTABLISHED        */
                       int length_lo, int length, int length_op, /* GT, LT, RANGE */
                       int tos,
                       bool_t fragments,
                       bool_t log)
{
  char buf[255];
  char op_str [CHAIN_STR_LEN];
  char src_str[PREFIX_AM_STR_SIZE+1], dst_str[PREFIX_AM_STR_SIZE+1];
  char sport_str[PORT_STR_LEN], dport_str[PORT_STR_LEN];
  char proto_str[PROTO_STR_LEN];
  char icmp_type_str[OPT_STR_LEN];
  char state_str[OPT_STR_LEN];
  char length_str[OPT_STR_LEN];
  char tos_str[OPT_STR_LEN];
  char frag_str[OPT_STR_LEN];
  char target_str[OPT_STR_LEN];
  int ret = 0;

  /* Get Source address. */
  ret = get_prefix_str (src_str, PREFIX_AM_STR_SIZE, OPT_SOURCE, src);
  if (ret < 0)
    pal_strcpy (src_str, "");

  /* Get Destination address. */
  ret = get_prefix_str (dst_str, PREFIX_AM_STR_SIZE, OPT_DESTINATION, dst);
  if (ret < 0)
    pal_strcpy (dst_str, "");

  /* Get source port. */
  get_port_opt (sport_str, PORT_STR_LEN, OPT_SPORT, sport_lo, sport, sport_op);

  /* Get destination port. */
  get_port_opt (dport_str, PORT_STR_LEN, OPT_DPORT, dport_lo, dport, dport_op);

  /* Get protocol. */
  get_protocol_opt (proto_str, PREFIX_ADDR_STR_SIZE, proto);

  /* ICMP: icmp_type > 0 ICMP type 0 .. 255 */
  get_icmp_type_opt(icmp_type_str, OPT_STR_LEN, proto, icmp_type);

  /* TCP: ESTABLISHED        */
  get_state_opt(state_str, OPT_STR_LEN, proto, established);

  get_length_opt(length_str, OPT_STR_LEN, length_lo, length, length_op);

  get_tos_opt(tos_str, OPT_STR_LEN, tos);

  get_fragments_opt(frag_str, OPT_STR_LEN, fragments);

  get_target_str(target_str, OPT_STR_LEN, filter, log);


  switch (scope)
    {
    case IMI_FILTER_INPUT:
      {
        if (op)
          {
            /* If rulenum > 0, insert using -I chain <rulenum>
             * else append using -A
             */
            if (rulenum)
              pal_sprintf (op_str, "%s %d", INSERT_INPUT, rulenum);
            else
              pal_strcpy (op_str, ADD_INPUT);
          }
        else
          pal_strcpy (op_str, DEL_INPUT);

        pal_snprintf (buf, 255, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
                      IPTABLES, FILTER_TABLE, op_str,
                      proto_str,
                      src_str, dst_str,
                      sport_str, dport_str,
                      opt_str[OPT_VIANAMEIN], interface,
                      target_str,
                      icmp_type_str,
                      state_str,
                      length_str,
                      tos_str,
                      frag_str);

#ifndef DEBUG
   ret = system (buf);
#else
        printf ("%s\n", buf);
#endif /* DEBUG */
      }
      break;
    case IMI_FILTER_OUTPUT:
      {
        if (op)
          {
            /* If rulenum > 0, insert using -I chain <rulenum>
             * else append using -A
             */
            if (rulenum)
              pal_sprintf (op_str, "%s %d", INSERT_OUTPUT, rulenum);
            else
              pal_strcpy (op_str, ADD_OUTPUT);
          }
        else
          pal_strcpy (op_str, DEL_OUTPUT);

        pal_snprintf (buf, 255, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
                      IPTABLES, FILTER_TABLE, op_str,
                      proto_str,
                      src_str, dst_str,
                      sport_str, dport_str,
                      opt_str[OPT_VIANAMEOUT], interface,
                      target_str,
                      icmp_type_str,
                      state_str,
                      length_str,
                      tos_str,
                      frag_str);

#ifndef DEBUG
   ret = system (buf);
#else
        printf ("%s\n", buf);
#endif /* DEBUG */
      }
      break;
    case IMI_FILTER_FORWARD:
      {
        if (op)
          {
            /* If rulenum > 0, insert using -I chain <rulenum>
             * else append using -A
             */
            if (rulenum)
              pal_sprintf (op_str, "%s %d", INSERT_FORWARD, rulenum);
            else
              pal_strcpy (op_str, ADD_FORWARD);
          }
        else
          pal_strcpy (op_str, DEL_FORWARD);

        pal_snprintf (buf, 255, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",
                      IPTABLES, FILTER_TABLE, op_str,
                      proto_str,
                      src_str, dst_str,
                      sport_str, dport_str,
                      opt_str[OPT_VIANAMEIN], interface,
                      target_str,
                      icmp_type_str,
                      state_str,
                      length_str,
                      tos_str,
                      frag_str);

#ifndef DEBUG
    ret = system (buf);
#else
        printf ("%s\n", buf);
#endif /* DEBUG */

      }
      break;
    }

  return ret;
}

result_t
pal_fw_translate_rule (int op, int scope, u_int16_t proto,
                        struct prefix_am *src, struct prefix_am *dst,
                        s_int16_t sport_lo, s_int16_t sport, int sport_op,
                        s_int16_t dport_lo, s_int16_t dport, int dport_op,
                        char *interface, int filter, int rulenum,
                        int icmp_type,
                        bool_t established,
                        int length_lo, int length, int length_op,
                        int tos,
                        bool_t fragments,
                        bool_t log)
{
  int ret = 0;

  if (log)
    {
      ret = _pal_fw_translate_rule(op, scope, proto, src, dst, sport_lo, sport, sport_op,
                                   dport_lo, dport, dport_op,
                                   interface, filter,
                                   rulenum-1,
                                   icmp_type,
                                   established,
                                   length_lo, length, length_op,
                                   tos,
                                   fragments,
                                   PAL_TRUE);
    }
  if (! ret)
    return _pal_fw_translate_rule(op, scope, proto, src, dst, sport_lo, sport, sport_op,
                                  dport_lo, dport, dport_op,
                                  interface, filter,
                                  rulenum,
                                  icmp_type,
                                  established,
                                  length_lo, length, length_op,
                                  tos,
                                  fragments,
                                  PAL_FALSE);
  return ret;
}

#endif /* HAVE_ACL */
#endif /* HAVE_ACL || HAVE_NAT */

