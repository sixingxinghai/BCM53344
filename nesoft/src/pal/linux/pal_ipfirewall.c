/* Copyright (C) 2005 i All Rights Reserved. */

#include "pal.h"
#include "nsmd.h"
#include "nsm/firewall/nsm_firewall.h"
#include "pal_firewall.h"
#include "pal_ipfirewall.h"

/* General Comment :
   We have maintained the same data structure types as that of Interpeak for
   data communication through socket with Interpeak Ipfirewall Kernel so
   that the data types and Size remains the same. The variables used for
   local processing in the function within this file has been used declared
   as per PacOS data structure types.
*/

/* Temporary structure for protocol used in this file */
struct firewall_protocol
{
  u_char number;
  char *name;
} ipfirewall_protocols[] = {{IP_IPPROTO_ICMP,   "icmp"},
                            {IP_IPPROTO_IGMP,   "igmp"},
                            {IP_IPPROTO_APNP,   "ipip"},
                            {IP_IPPROTO_TCP,    "tcp"},
                            {IP_IPPROTO_UDP,    "udp"},
                            {IP_IPPROTO_ESP,    "esp"},
                            {IP_IPPROTO_AH,     "ah"},
                            {IP_IPPROTO_ICMPV6, "ipv6-icmp"}};

static s_int32_t
_ipfirewall_cmd_setsockopt (s_int32_t sock, int level, int optname,
                            const void *optval, u_int32_t optlen)
{
  return setsockopt (sock, level, optname, optval, optlen);
}

static s_int32_t
_ipfirewall_cmd_getsockopt (s_int32_t sock, int level, int optname, void *optval,
                            u_int32_t *optlenp)
{
  return getsockopt (sock, level, optname, optval, optlenp);

}

/* function to get a single command */
static s_int32_t
_ipfirewall_setsockopt_single (s_int32_t cmd)
{
  s_int32_t ret = RESULT_OK;
  Ipfirewall_ctrl ctrl;
  s_int32_t sock;
#ifdef HAVE_IPV6
  int family = AF_INET6;
  int optname = IP_IPV6_FW;
  int level = IPPROTO_IPV6;
#else
  int family = AF_INET;
  int optname = IP_IP_FW;
  int level = IPPROTO_IP;
#endif /* HAVE_IPV6 */

  /* Create a socket */
  sock = socket (family, SOCK_DGRAM, 0);
  if (sock < 0)
    return RESULT_ERROR;

  ctrl.cmd = cmd;

  /* Send the data accress the socket */
  if (_ipfirewall_cmd_setsockopt (sock, level, optname, &ctrl,
                                  sizeof(ctrl)) != 0)
     ret = RESULT_ERROR;

  /* Close the socket */
  if (sock < 0)
    close (sock);

  return ret;
}

/* Function to fill the Ipfirewall_fw_rule structure  */
static Ipfirewall_rule *
_fill_ipfirewall_rule (struct pal_firewall_rule *rule, u_int32_t *rule_index)
{
  Ipfirewall_rule *fw_rule = NULL;
  u_int32_t i;

  fw_rule = malloc (sizeof (Ipfirewall_rule));

  if (fw_rule == NULL)
    return NULL; /* Memory allocation failure */

  memset (fw_rule, 0, sizeof (Ipfirewall_rule));

  /* Copy the family */
  fw_rule->family = rule->src.family;

  /* Copy the index */
  rule_index = 0;

  /* Copy the action */
  fw_rule->block = rule->block;

  /* Copy the direction */
  fw_rule->in = rule->in;

  /* Copy the log */
  fw_rule->log = rule->log;

  /* Copy the interface */
  fw_rule->on = rule->on;
  strcpy ((char *)fw_rule->ifname, rule->ifname);

  /* Copy the protocol portion */
  fw_rule->proto = rule->proto;
  fw_rule->tcpudp = rule->tcpudp;
  fw_rule->protocol = rule->protocol;

  /* Copy the address part */
  if (rule->from)
    {
      fw_rule->from = rule->from;
      if (rule->src_any)
        {
          fw_rule->src_any = rule->src_any;
        }
      else
        {
          memcpy (&fw_rule->src.addr, &rule->src.u.prefix,
                        sizeof (rule->src.u));
          if (fw_rule->family == AF_INET)
            {
              /* Store in network endian */
              fw_rule->src.mask.v4 = 0;

              for (i = 0; i < rule->src.prefixlen; i++)
                SET_FLAG (fw_rule->src.mask.v4, 1<<(31-i));

              fw_rule->src.mask.v4 = htonl(fw_rule->src.mask.v4);
            }
#ifdef HAVE_IPV6
          else if (fw_rule->family == AF_INET6)
            {
              fw_rule->src.mask.v6[0] = 0;
              fw_rule->src.mask.v6[1] = 0;
              fw_rule->src.mask.v6[2] = 0;
              fw_rule->src.mask.v6[3] = 0;
              for (i = 0; i < rule->src.prefixlen; i++)
                {
                  if (i < 32)
                    SET_FLAG (fw_rule->src.mask.v6[0], 1<<(31-i));
                  else if (i < 64)
                    SET_FLAG (fw_rule->src.mask.v6[1], 1<<(31-(i-32)));
                  else if (i < 96)
                    SET_FLAG (fw_rule->src.mask.v6[2], 1<<(31-(i-64)));
                  else
                    SET_FLAG (fw_rule->src.mask.v6[3], 1<<(31-(i-96)));
                }
              /* Store in network endian */
              fw_rule->src.mask.v6[0] = htonl (fw_rule->src.mask.v6[0]);
              /* Store in network endian */
              fw_rule->src.mask.v6[1] = htonl (fw_rule->src.mask.v6[1]);
              /* Store in network endian */
              fw_rule->src.mask.v6[2] = htonl (fw_rule->src.mask.v6[2]);
              /* Store in network endian */
              fw_rule->src.mask.v6[3] = htonl (fw_rule->src.mask.v6[3]);
            }
#endif /* HAVE_IPV6 */
        }

      /* Copy the Source port */
      /* Do a check of the port for validity */
      if (rule->src_port)
        {
          fw_rule->src_port = rule->src_port;
          fw_rule->src_op = rule->src_op;
          fw_rule->src_port_hi = rule->src_port_hi;
       }
    }

  if (rule->to)
    {
      fw_rule->to = rule->to;
      if (rule->dst_any)
        {
          fw_rule->dst_any = rule->dst_any;
        }
      else
        {
          memcpy (&fw_rule->dst.addr, &rule->dst.u.prefix,
                        sizeof (rule->dst.u));
          if (fw_rule->family == AF_INET)
            {
              /* Store in network endian */
              fw_rule->dst.mask.v4 = 0;
              for (i = 0; i < rule->dst.prefixlen; i++)
                SET_FLAG (fw_rule->dst.mask.v4, 1<<(31-i));

              fw_rule->dst.mask.v4 = htonl (fw_rule->dst.mask.v4);
            }
#ifdef HAVE_IPV6
          else if (fw_rule->family == AF_INET6)
            {
              fw_rule->dst.mask.v6[0] = 0;
              fw_rule->dst.mask.v6[1] = 0;
              fw_rule->dst.mask.v6[2] = 0;
              fw_rule->dst.mask.v6[3] = 0;
              for (i = 0; i < rule->dst.prefixlen; i++)
                {
                  if (i < 32)
                    SET_FLAG (fw_rule->dst.mask.v6[0], 1<<(31-i));
                  else if (i < 64)
                    SET_FLAG (fw_rule->dst.mask.v6[1], 1<<(31-(i-32)));
                  else if (i < 96)
                    SET_FLAG (fw_rule->dst.mask.v6[2], 1<<(31-(i-64)));
                  else
                    SET_FLAG (fw_rule->dst.mask.v6[3], 1<<(31-(i-96)));
                }
              /* Store in network endian */
              fw_rule->dst.mask.v6[0] = htonl (fw_rule->dst.mask.v6[0]);
              /* Store in network endian */
              fw_rule->dst.mask.v6[1] = htonl (fw_rule->dst.mask.v6[1]);
              /* Store in network endian */
              fw_rule->dst.mask.v6[2] = htonl (fw_rule->dst.mask.v6[2]);
              /* Store in network endian */
              fw_rule->dst.mask.v6[3] = htonl (fw_rule->dst.mask.v6[3]);
            }
#endif /* HAVE_IPV6 */
        }

      /* Copy the dst port */
      /* Do a check of the port for validity */
      if (rule->dst_port)
        {
          fw_rule->dst_port = rule->dst_port;
          fw_rule->dst_op = rule->dst_op;
          fw_rule->dst_port_hi = rule->dst_port_hi;
       }
    }

  /* Copy the ICMP-type */
  fw_rule->icmp_type = rule->icmp_type;
  fw_rule->icmp_num = rule->icmp_num;

  /* Copy the group no */
  fw_rule->group_head = rule->group_head;
  fw_rule->group_no = rule->group_no;

  return fw_rule;
}

/* Function called from NSM to Add/Delete a rule from the kernel */
static s_int32_t
_ipfirewall_addremove_parsed_rule (Ipfirewall_rule *fw_rule,
                                   u_int32_t rule_index, bool_t remove)
{
  Ipfirewall_ctrl *ctrl = NULL;
  s_int32_t ret = RESULT_OK;
  u_int32_t len;
  s_int32_t sock;
#ifdef HAVE_IPV6
  int family = AF_INET6;
  int optname = IP_IPV6_FW;
  int level = IPPROTO_IPV6;
#else
  int family = AF_INET;
  int optname = IP_IP_FW;
  int level = IPPROTO_IP;
#endif /* HAVE_IPV6 */

  /* Create a socket */
  sock = socket (family, SOCK_DGRAM, 0);
  if (sock < 0)
    {
      ret = RESULT_ERROR;
      goto exit;
    }

  /* Allocate memory for the setsockopt call */
  len = sizeof (Ipfirewall_ctrl);
  ctrl = malloc (len);

  pal_mem_set (ctrl, 0, len);

  if (ctrl == NULL)
    {
      ret = RESULT_ERROR;
      goto exit;
    }

  /* Set the command type */
  if (remove == PAL_TRUE)
    {
      ctrl->cmd = IPFIREWALL_CTRL_DEL_RULE;
      memcpy (&ctrl->type.rule, fw_rule, sizeof (Ipfirewall_rule));
    }
  else
    {
      ctrl->cmd = IPFIREWALL_CTRL_ADD_RULE;
      ctrl->seqno = rule_index;
      memcpy (&ctrl->type.rule, fw_rule, sizeof(Ipfirewall_rule));
    }

  /* Send the data through the socket */
  if (_ipfirewall_cmd_setsockopt (sock, level, optname, ctrl, len) < 0)
    {
      ret = RESULT_ERROR;
      goto exit;
    }

exit:
  /* Close the socket */
  if (sock < 0)
    close (sock);

  /* Free the ctrl */
  if (ctrl != NULL)
    free (ctrl);

  return ret;
}

/* Function to Fill the rule istructure for show Purpose */
static void
_fill_show_fw_rule (Ipfirewall_rule *fw_rule, struct pal_firewall_rule *rule,
                    s_int32_t rule_index)
{
  struct pal_firewall_rule *temp_rule;
  u_int32_t mask4;
  u_int32_t count = 0;
  s_int32_t i;
#ifdef HAVE_IPV6
  u_int32_t mask6[4];
#endif /* HAVE_IPV6 */

  /* Allocate the memory for the temporary variable used for show */
  temp_rule = XCALLOC (MTYPE_TMP, sizeof (struct pal_firewall_rule));
  if (temp_rule == NULL)
    return;

  /* Populate the various parameters of the rule */
  temp_rule->rule_no = rule_index;
  temp_rule->family = fw_rule->family;
  temp_rule->block = fw_rule->block;
  temp_rule->in = fw_rule->in;
  temp_rule->log = fw_rule->log;

  temp_rule->on = fw_rule->on;
  strcpy ((char *)&temp_rule->ifname, fw_rule->ifname);

  temp_rule->proto = fw_rule->proto;
  temp_rule->tcpudp = fw_rule->tcpudp;
  temp_rule->protocol = fw_rule->protocol;

  temp_rule->from = fw_rule->from;
  if (fw_rule->from)
    {
      temp_rule->src_any = fw_rule->src_any;
      if (! fw_rule->src_any)
        {
          memcpy (&temp_rule->src.u, &fw_rule->src.addr,
                        sizeof (temp_rule->src.u));

          temp_rule->src.family = fw_rule->family;

          if (fw_rule->family == AF_INET)
            {
              /* get the mask */
              mask4 = ntohl (fw_rule->src.mask.v4);
              if (mask4 != 0xffffffff)
                {
                  for (i = 31; i >= 0; i--)
                    if (CHECK_FLAG (mask4, 1<<i))
                      ++count;

                  temp_rule->src.prefixlen = count;
                }
            }
#ifdef HAVE_IPV6
          else if (fw_rule->family == AF_INET6)
            {
              /* Print the mask */
               mask6[0] = ntohl (fw_rule->src.mask.v6[0]);
               mask6[1] = ntohl (fw_rule->src.mask.v6[1]);
               mask6[2] = ntohl (fw_rule->src.mask.v6[2]);
               mask6[3] = ntohl (fw_rule->src.mask.v6[3]);
               if (mask6[3] != 0xffffffff)
                 {

                   for (i = 31; i >= 0; i--)
                     {
                       if (CHECK_FLAG (mask6[0], 1<<i))
                         ++count;
                       if (CHECK_FLAG (mask6[1], 1<<i))
                         ++count;
                       if (CHECK_FLAG (mask6[2], 1<<i))
                         ++count;
                       if (CHECK_FLAG (mask6[3], 1<<i))
                         ++count;
                     }

                   temp_rule->src.prefixlen = count;
                }
            }
#endif /* HAVE_IPV6 */
        }

      temp_rule->src_port = fw_rule->src_port;
      if (fw_rule->src_port)
        {
          temp_rule->src_op = fw_rule->src_op;
          temp_rule->src_port_hi = fw_rule->src_port_hi;
        }
    }

  temp_rule->to = fw_rule->to;
  if (fw_rule->to)
    {
      temp_rule->dst_any = fw_rule->dst_any;
      if (! fw_rule->dst_any)
        {
          memcpy (&temp_rule->dst.u.prefix, &fw_rule->dst.addr,
                        sizeof (temp_rule->dst.u));

          temp_rule->dst.family = fw_rule->family;

          if (fw_rule->family == AF_INET)
            {
              /* get the mask */
              mask4 = ntohl (fw_rule->dst.mask.v4);
              if (mask4 != 0xffffffff)
                {
                  /* Reset the counter */
                  count = 0;
                  for (i = 31; i >= 0; i--)
                    if (CHECK_FLAG (mask4, 1<<i))
                      ++count;

                  temp_rule->dst.prefixlen = count;
                }
            }
#ifdef HAVE_IPV6
          else if (fw_rule->family == AF_INET6)
            {
              /* Print the mask */
               mask6[0] = ntohl (fw_rule->dst.mask.v6[0]);
               mask6[1] = ntohl (fw_rule->dst.mask.v6[1]);
               mask6[2] = ntohl (fw_rule->dst.mask.v6[2]);
               mask6[3] = ntohl (fw_rule->dst.mask.v6[3]);
               if (mask6[3] != 0xffffffff)
                 {
                   /* Reset the counter */
                   count = 0;
                   for (i = 31; i >= 0; i--)
                     {
                       if (CHECK_FLAG (mask6[0], 1<<i))
                         ++count;
                       if (CHECK_FLAG (mask6[1], 1<<i))
                         ++count;
                       if (CHECK_FLAG (mask6[2], 1<<i))
                         ++count;
                       if (CHECK_FLAG (mask6[3], 1<<i))
                         ++count;
                     }

                   temp_rule->dst.prefixlen = count;
                }
            }
#endif /* HAVE_IPV6 */
        }

      temp_rule->dst_port = fw_rule->dst_port;
      if (fw_rule->dst_port)
        {
          temp_rule->dst_op = fw_rule->dst_op;
          temp_rule->dst_port_hi = fw_rule->dst_port_hi;
        }
    }

  temp_rule->icmp_type = fw_rule->icmp_type;
  temp_rule->icmp_num = fw_rule->icmp_num;

  temp_rule->group_no = fw_rule->group_no;

  /* If the rule is already populated add to list of that rule */
  if (rule->rule_no != 0 )
    listnode_add (&rule->list_rule, temp_rule);
  else
    pal_mem_cpy (rule, temp_rule, sizeof (struct pal_firewall_rule));
}

/* Functiono to get the rules fromn Kernel */
s_int32_t
pal_ipfirewall_get_rules (s_int32_t group, struct pal_firewall_rule *rule)
{
  Ipfirewall_ctrl *ctrl = NULL;
  Ipfirewall_rule *fw_rule;
  u_int32_t len;
  s_int32_t i = 0;
  s_int32_t sock;
#ifdef HAVE_IPV6
  int family = AF_INET6;
  int optname = IP_IPV6_FW;
  int level = IPPROTO_IPV6;
#else
  int family = AF_INET;
  int optname = IP_IP_FW;
  int level = IPPROTO_IP;
#endif

  /* Create a socket */
  sock = socket (family, SOCK_DGRAM, 0);
  if (sock < 0)
    goto exit;

  /* Allocate memory for the getsockopt call */
  len = sizeof (Ipfirewall_ctrl);
  ctrl = malloc (len);
  if (ctrl == NULL)
    goto exit;

  /* Get rule entries */
  ctrl->cmd = IPFIREWALL_CTRL_GET_RULE;
  ctrl->seqno = i;
  while (_ipfirewall_cmd_getsockopt (sock, level, optname, ctrl, &len) == 0
         && len == sizeof (Ipfirewall_ctrl))
    {
      /* Get the rule and prepare for next */
      fw_rule = &ctrl->type.rule;
      ctrl->seqno = ++i;
      assert (i <= IPFIREWALL_MAX_RULES);

      /* If the group is specified then get all those rules of that group */
      if (group != -1 && group != (s_int32_t)fw_rule->group_no)
        continue;

      /* Fill the rule structure passed from the nsm for show */
      _fill_show_fw_rule (fw_rule, rule, i);
    }

exit:
  /* Free the ctrl structure */
  if (ctrl != NULL)
    free (ctrl);

  /* Close the Socket */
  if (sock < 0)
    close (sock);

  return 0;
}

/* Function to flush all rules */
s_int32_t
pal_ipfirewall_flush_rules (void)
{
  return _ipfirewall_setsockopt_single (IPFIREWALL_CTRL_FLUSH_RULES);
}

/* Function Called by NSM to add/remove a rule to ipfirewall kernel */
s_int32_t
pal_ipfirewall_addremove_rule (struct pal_firewall_rule *rule, bool_t remove)
{
  Ipfirewall_rule *fw_rule = NULL;
  u_int32_t rule_index;
  s_int32_t ret;

  /* Sanity check */
  if (rule == NULL)
    return -1;

  /* Fill the Interpeak Ipfirewall_rule structure from the pal_firewall_rule
     structure.
  */
  fw_rule = _fill_ipfirewall_rule (rule, &rule_index);

  if (fw_rule)
    {
      /* Add the rule to the Kernel */
      ret = _ipfirewall_addremove_parsed_rule (fw_rule, rule_index, remove);
    }
  else
    {
      ret = -1;
      goto exit;
    }

exit:
  /* Free the Ipfirewall_rule structure */
  if (fw_rule)
    free (fw_rule);

  return ret;
}
/* End of File */
