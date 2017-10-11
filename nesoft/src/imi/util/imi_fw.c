/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#ifdef HAVE_NAT

#include "if.h"
#include "prefix.h"
#include "filter.h"
#include "snprintf.h"
#include "cli.h"
#include "vrep.h"

#include "imi/pal_fw.h"

#include "imi/imi.h"
#include "imi/imi_interface.h"
#include "imi/util/imi_fw.h"
#include "imi/util/imi_filter.h"
#include "imi/util/imi_vs.h"

/* Specifiers for NAT show number of columns. */
#define IMI_NAT_CLI_VREP_NUM_COLS            7
#define IMI_NAT_OUT_DST_CLI_VREP_NUM_COLS    9

/* Network address translation configuration.  */

/* NAT address pool list of struct imi_nat_address_pool */
struct list *imi_nat_address_pool_list;

struct imi_nat_timeouts nat_time_out;

/* Interface NAT scope. */
const int via_interface[IMI_NAT_DIRECTION_MAX][IMI_NAT_TRANS_MAX]=
  {
    {
      NSM_IF_SCOPE_OUTSIDE,
      NSM_IF_SCOPE_INSIDE
    },
    {
      NSM_IF_SCOPE_INSIDE,
      NSM_IF_SCOPE_OUTSIDE
    }
  };

/* NAT directions. */
const char *direction_str[]=
  {
    "inside",  /* IMI_NAT_INSIDE */
    "outside"  /* IMI_NAT_OUTSIDE */
  };

extern char *filter_prot_str(s_int16_t, char *);


struct imi_nat_address_pool *
imi_nat_address_pool_lookup (struct list *list, char *name)
{
  struct listnode *node;
  struct imi_nat_address_pool *pool;

  for (node = LISTHEAD (list); node; NEXTNODE (node))
    {
      pool = GETDATA (node);

      if (!pal_strcmp (pool->name, name))
        return pool;
    }
  return NULL;
}

int
imi_nat_address_pool (char *name,
                      char *start_ip, char *end_ip, char *mask,
                      u_int16_t op)
{
  struct imi_nat_address_pool *pool = NULL, *tmp = NULL;
  struct listnode *node, *node_next;

  pool = imi_nat_address_pool_lookup (imi_nat_address_pool_list, name);
  if (pool && op)
    return IMI_NAT_ERROR;

  pool = XCALLOC (MTYPE_IMI_NAT_POOL, sizeof (struct imi_nat_address_pool));
  if (!pool)
    return IMI_NAT_OUT_OF_MEM;

  pal_strcpy (pool->name, name);

  if (start_ip)
    pal_strcpy (pool->min_ip, start_ip);

  if (end_ip)
    pal_strcpy (pool->max_ip, end_ip);

  if (mask)
    pal_strcpy (pool->mask, mask);

  pool->refcnt = 0;

  if (op)
    /* Add pool. */
    listnode_add (imi_nat_address_pool_list, pool);
  else
    {
      /* Delete pool. */
      for (node = LISTHEAD (imi_nat_address_pool_list); node; node = node_next)
        {
          tmp = GETDATA (node);

          node_next = node->next;

          if ((!pal_strcmp (tmp->name, pool->name) &&
               !(start_ip && end_ip && mask)) ||
              (!pal_strcmp (tmp->name, pool->name) &&
               !pal_strcmp (tmp->min_ip, pool->min_ip) &&
               !pal_strcmp (tmp->max_ip, pool->max_ip) &&
               !pal_strcmp (tmp->mask, pool->mask)))
            {
              /* If pool is in use don't delete it. */
              if (tmp->refcnt)
                {
                  XFREE (MTYPE_IMI_NAT_POOL, pool);
                  return IMI_NAT_ERROR;
                }

              XFREE (MTYPE_IMI_NAT_POOL, tmp);
              list_delete_node (imi_nat_address_pool_list, node);
              XFREE (MTYPE_IMI_NAT_POOL, pool);
              return IMI_NAT_SUCCESS;
            }
        }
      XFREE (MTYPE_IMI_NAT_POOL, pool);
      return IMI_NAT_POOL_NO_MATCH;
    }

  return IMI_NAT_SUCCESS;
}

void
display_nat_pool (struct cli *cli, char *name)
{
  struct listnode *node;
  struct imi_nat_address_pool *tmp;

  for (node = LISTHEAD (imi_nat_address_pool_list); node; NEXTNODE(node))
    {
      tmp = GETDATA (node);

      if (!name)
        {
          cli_out (cli, "Pool name : %s\n", tmp->name);
          cli_out (cli, " Minimum IP address : %s\n", tmp->min_ip);
          cli_out (cli, " Maximum IP address : %s\n", tmp->max_ip);
          cli_out (cli, " Mask               : %s\n", tmp->mask);
          cli_out (cli, " References         : %d\n", tmp->refcnt);
        }
      else if (!pal_strcmp (tmp->name, name))
        {
          cli_out (cli, "Pool name : %s\n", name);
          cli_out (cli, " Minimum IP address : %s\n", tmp->min_ip);
          cli_out (cli, " Maximum IP address : %s\n", tmp->max_ip);
          cli_out (cli, " Mask               : %s\n", tmp->mask);
          cli_out (cli, " References         : %d\n", tmp->refcnt);
          break;
        }
    }
}

/*Function to clear the elements in acl_num_list for a particular rule chain */
void
imi_nat_chain_list_del_acl_num (struct list *acl_num_list, u_int32_t acl_num)
{
  struct listnode *node = NULL;
  u_int32_t *val = NULL;

  /* If the acl-name-list is empty free that list. */
  if (LIST_ISEMPTY (acl_num_list))
    return;

  LIST_LOOP (acl_num_list, val, node)
    {
      if (*val == acl_num)
        {
          XFREE (MTYPE_ACCESS_LIST_STR, val);
          list_delete_node (acl_num_list, node);
          return;
        }
    }
}

 /* If the acl name is numeric then add it to the  chain list in sorted
    order of acl num and return the position of the node in the list.*/
u_int16_t
imi_nat_chain_add_acl_num(int scope, char *acl_name)
{
  u_int32_t acl_num;
  u_int32_t *acl_num_elem;
  struct imi_nat_chain *chain;
  u_int16_t rulenum = 0;
  char temp [20];

  if (pal_sscanf (acl_name, "%u%1s", (unsigned *)&acl_num, temp) == 1)
    {
      chain = &nat_table.chain_tbl[scope];
      acl_num_elem = XCALLOC (MTYPE_ACCESS_LIST_STR, sizeof (u_int32_t));
      /* Append in the worst case. */
      if (! acl_num_elem)
         return 0;
      *acl_num_elem = acl_num;

      /* The position of the node in the list, after sorting it based on
         the acl list name gives the rulenum to be inserted.
      */
      rulenum = listnode_add_sort_index (&chain->acl_num_list, acl_num_elem);

      return (rulenum + chain->static_cnt);
    }
    /* Not numbered access list - will append. */
    return 0;
}

/*Function to clear the rule chain in nat tables and to clear acl_num_list which
  is the element of structure chain table.
 */
void
imi_nat_chain_del_acl_num(int scope, char *acl_name)
{
  u_int32_t acl_num;
  struct imi_nat_chain *chain;
  char temp [20];

  if (pal_sscanf (acl_name, "%u%1s", (unsigned *)&acl_num, temp) == 1)
    {
      chain = &nat_table.chain_tbl[scope];

      /* Get the node from the list based on the acl_num.
      */
      imi_nat_chain_list_del_acl_num (&chain->acl_num_list, acl_num);
    }
}

/* If op = 1, add the acl name to the nat chain list else delete. */
u_int16_t
imi_nat_chain_update_acl_num (int scope, char *acl_name, int op)
{
  u_int16_t rulenum = 0;

  if (op)
    rulenum = imi_nat_chain_add_acl_num(scope, acl_name);
  else
    imi_nat_chain_del_acl_num(scope, acl_name);
    return rulenum;
}


result_t
imi_nat_acl_ntf_cb (struct access_list *access, 
                    struct filter_list *filter, 
                    filter_opcode_t opcode)
{
  struct imi_nat_address_pool *pool;
  struct imi_rule *rule;
  struct list *rlist = NULL;
  struct prefix_am4 min_ip, max_ip;
  struct listnode *node, *lnode;
  struct filter_pacos_ext *acl_filter;
  struct interface *ifp;
  struct imi_interface *imi_if;
  int if_scope;
  struct apn_vr *vr = apn_vr_get_privileged (imim);
  u_int16_t rulenum = 0;
  result_t res = LIB_API_SET_SUCCESS;
  result_t ret = LIB_API_SET_SUCCESS;


  /* XXX should detect access-list is invalid. */
  if (filter == NULL)
    return LIB_API_SET_ERROR;

  /* Check if this filter is a PacOS extended one. */
  if (filter->common != FILTER_PACOS_EXT)
    return LIB_API_SET_SUCCESS;

  /* Inside source translations. */
  rlist = IMI_NAT_RLIST(IMI_NAT_INSIDE, IMI_NAT_SOURCE);

  for (node = LISTHEAD (rlist); node; NEXTNODE (node))
    {
      rule = GETDATA (node);

      if ((rule->flag == IMI_RULE_ACL) && !pal_strcmp (rule->u.arule.acl, access->name))
        {
          pool = imi_nat_address_pool_lookup (imi_nat_address_pool_list,
                                              rule->u.arule.pool);

          str2prefix_am4 (pool->min_ip, NULL, &min_ip);
          str2prefix_am4 (pool->max_ip, NULL, &max_ip);

          if_scope = via_interface [IMI_NAT_INSIDE][IMI_NAT_SOURCE];

          for (lnode = LISTHEAD (vr->ifm.if_list); lnode; NEXTNODE (lnode))
            {
              ifp = GETDATA (lnode);
              imi_if = ifp->info;

              if (imi_if->scope == if_scope)
                {
                  acl_filter = &filter->u.zextfilter;
                  rulenum = imi_nat_chain_update_acl_num (IMI_NAT_SOURCE,
                                                          access->name, opcode);
                  res = pal_nat_translate_rule (opcode, IMI_NAT_INSIDE, IMI_NAT_SOURCE,
                                                acl_filter->protocol,
                                                (struct prefix_am4 *)&acl_filter->sprefix,
                                                (struct prefix_am4 *)&acl_filter->dprefix,
                                                acl_filter->sport_lo, acl_filter->sport,
                                                acl_filter->sport_op,
                                                acl_filter->dport_lo, acl_filter->dport,
                                                acl_filter->dport_op,
                                                NULL, ifp->name,
                                                &min_ip.addr4, &max_ip.addr4,
                                                0, 0,
                                                rulenum);
                 if (res && opcode)
                    goto imi_nat_acl_ntf_cb_error;
                 else if (res && !ret)
                    /* Just store the error code if error and deletion... */
                    ret = res;

                }
            }
        }
    }

  /* Inside destination translations. */
  rlist = IMI_NAT_RLIST(IMI_NAT_INSIDE, IMI_NAT_DESTINATION);

  for (node = LISTHEAD (rlist); node; NEXTNODE (node))
    {
      rule = GETDATA (node);

      if ((rule->flag == IMI_RULE_ACL) && !pal_strcmp (rule->u.arule.acl,
           access->name))
        {
          pool = imi_nat_address_pool_lookup (imi_nat_address_pool_list,
                                              rule->u.arule.pool);

          str2prefix_am4 (pool->min_ip, NULL, &min_ip);
          str2prefix_am4 (pool->max_ip, NULL, &max_ip);

          if_scope = via_interface [IMI_NAT_INSIDE][IMI_NAT_DESTINATION];

          for (lnode = LISTHEAD (vr->ifm.if_list); lnode; NEXTNODE (lnode))
            {
              ifp = GETDATA (lnode);
              imi_if = ifp->info;

              if (imi_if->scope == if_scope)
                {
                  acl_filter = &filter->u.zextfilter;
                  rulenum = imi_nat_chain_update_acl_num (IMI_NAT_DESTINATION,
                                                          access->name, opcode);
                  res = pal_nat_translate_rule (opcode, IMI_NAT_INSIDE, IMI_NAT_DESTINATION,
                                          acl_filter->protocol,
                                          (struct prefix_am4 *)&acl_filter->sprefix,
                                          (struct prefix_am4 *)&acl_filter->dprefix,
                                          acl_filter->sport_lo, acl_filter->sport,
                                          acl_filter->sport_op,
                                          acl_filter->dport_lo, acl_filter->dport,
                                          acl_filter->dport_op,
                                          ifp->name, NULL,
                                          &min_ip.addr4, &max_ip.addr4,
                                          0, 0,
                                          rulenum);
                  if (res && opcode)
                    goto imi_nat_acl_ntf_cb_error;
                  else if (res && !ret)
                    /* Just store the error code if error and deletion... */
                    ret = res;
                }
            }
        }
    }

  /* Outside source translations. */
  rlist = IMI_NAT_RLIST(IMI_NAT_OUTSIDE, IMI_NAT_SOURCE);

  for (node = LISTHEAD (rlist); node; NEXTNODE (node))
    {
      rule = GETDATA (node);

      if ((rule->flag == IMI_RULE_ACL) && !pal_strcmp (rule->u.arule.acl,
                                                       access->name))
        {
          pool = imi_nat_address_pool_lookup (imi_nat_address_pool_list,
                                              rule->u.arule.pool);

          str2prefix_am4 (pool->min_ip, NULL, &min_ip);
          str2prefix_am4 (pool->max_ip, NULL, &max_ip);

          if_scope = via_interface [IMI_NAT_OUTSIDE][IMI_NAT_SOURCE];

          for (lnode = LISTHEAD (vr->ifm.if_list); lnode; NEXTNODE (lnode))
            {
              ifp = GETDATA (lnode);
              imi_if = ifp->info;

              if (imi_if->scope == if_scope)
                {
                  acl_filter = &filter->u.zextfilter;
                  rulenum = imi_nat_chain_update_acl_num (IMI_NAT_SOURCE,
                                                          access->name, opcode);
                  res = pal_nat_translate_rule (opcode, IMI_NAT_OUTSIDE, IMI_NAT_SOURCE,
                                                acl_filter->protocol,
                                                (struct prefix_am4 *)&acl_filter->sprefix,
                                                (struct prefix_am4 *)&acl_filter->dprefix,
                                                acl_filter->sport_lo, acl_filter->sport,
                                                acl_filter->sport_op,
                                                acl_filter->dport_lo, acl_filter->dport,
                                                acl_filter->dport_op,
                                                ifp->name, NULL,
                                                &min_ip.addr4, &max_ip.addr4,
                                                0, 0,
                                                rulenum);

                 if (res && opcode)
                    goto imi_nat_acl_ntf_cb_error;
                  else if (res && ! ret)
                    /* Just store the error code if error and deletion... */
                    ret = res;
                }
            }
        }
    }

  /* Outside destination translations. */
  rlist = IMI_NAT_RLIST(IMI_NAT_OUTSIDE, IMI_NAT_DESTINATION);

  for (node = LISTHEAD (rlist); node; NEXTNODE (node))
    {
      rule = GETDATA (node);

      if ((rule->flag == IMI_RULE_ACL) && !pal_strcmp (rule->u.arule.acl,
                                                       access->name))
        {
          pool = imi_nat_address_pool_lookup (imi_nat_address_pool_list,
                                              rule->u.arule.pool);

          str2prefix_am4 (pool->min_ip, NULL, &min_ip);
          str2prefix_am4 (pool->max_ip, NULL, &max_ip);

          if_scope = via_interface [IMI_NAT_OUTSIDE][IMI_NAT_DESTINATION];

          for (lnode = LISTHEAD (vr->ifm.if_list); lnode; NEXTNODE (lnode))
            {
              ifp = GETDATA (lnode);
              imi_if = ifp->info;

              if (imi_if->scope == if_scope)
                {
                  acl_filter = &filter->u.zextfilter;
                  rulenum = imi_nat_chain_update_acl_num (IMI_NAT_DESTINATION,
                                                          access->name, opcode);
                  ret = pal_nat_translate_rule (opcode, IMI_NAT_OUTSIDE, IMI_NAT_DESTINATION,
                                                acl_filter->protocol,
                                                (struct prefix_am4 *)&acl_filter->sprefix,
                                                (struct prefix_am4 *)&acl_filter->dprefix,
                                                acl_filter->sport_lo, acl_filter->sport,
                                                acl_filter->sport_op,
                                                acl_filter->dport_lo, acl_filter->dport,
                                                acl_filter->dport_op,
                                                NULL, ifp->name,
                                                &min_ip.addr4, &max_ip.addr4,
                                                0, 0,
                                                rulenum);
                   if (res && opcode)
                     goto imi_nat_acl_ntf_cb_error;
                   else if (res && ! ret)
                     /* Just store the error code if error and deletion... */
                     ret = res;
   
                }
            }
        }
    }
  /* Processing completed: Return success or first error encountered.
   */
   res = ret;
   /* Fall through */
 imi_nat_acl_ntf_cb_error:
   /* Processing aborted: Return success or first error encountered. */
   return (res <= 0 ? res : LIB_API_SET_ERR_BAD_KERNEL_RULE);

 }

static int
imi_nat_pool_config_encode (cfg_vect_t *cv)
{
  int written = 0;
  struct listnode *node;
  struct imi_nat_address_pool *pool;

  for (node = LISTHEAD (imi_nat_address_pool_list); node; NEXTNODE(node))
    {
      pool = GETDATA (node);

      cfg_vect_add_cmd (cv, "ip nat pool %s %s %s %s \n",
               pool->name, pool->min_ip, pool->max_ip, pool->mask);
      written++;
    }

  return written;
}

int
imi_nat_translations_config_encode (int direction, cfg_vect_t *cv)
{
  struct list *rlist = NULL;
  struct imi_rule *rule;
  struct listnode *node;
  struct imi_rule_static *srule;
  int write = 0;
  char addr[32],addr1[32],proto_str[32];

  rlist = IMI_NAT_RLIST(direction, IMI_NAT_SOURCE);

  for (node = LISTHEAD (rlist); node; NEXTNODE (node))
    {
      rule = GETDATA (node);

      switch (rule->flag)
        {
        case IMI_RULE_ACL:
          cfg_vect_add_cmd (cv, "ip nat %s source list %s pool %s\n",
                   direction_str[direction], rule->u.arule.acl,
                   rule->u.arule.pool);
          write++;
          break;
        case IMI_RULE_STATIC:
          srule = &rule->u.srule;
          pal_inet_ntop (AF_INET, &srule->sprefix.addr4, addr, 32);
          pal_inet_ntop (AF_INET, &rule->u.srule.min_ip, addr1, 32);
          cfg_vect_add_cmd (cv, "ip nat %s source static %s %s\n",
                            direction_str[direction],addr,addr1);
          write++;
          break;
        default:
          break;
        }
    }

  rlist = IMI_NAT_RLIST(direction, IMI_NAT_DESTINATION);
  for (node = LISTHEAD (rlist); node; NEXTNODE (node))
    {
      rule = GETDATA (node);

      switch (rule->flag)
        {
        case IMI_RULE_ACL:
          cfg_vect_add_cmd (cv, "ip nat %s destination list %s pool %s\n",
                   direction_str[direction],
                   rule->u.arule.acl, rule->u.arule.pool);
          write++;
          break;
        case IMI_RULE_STATIC:
          srule = &rule->u.srule;

          pal_inet_ntop (AF_INET, &srule->dprefix.addr4, addr, 32);
          pal_inet_ntop (AF_INET, &srule->min_ip, addr1, 32);

          if ((srule->protocol != TCP_PROTO) && (srule->protocol != UDP_PROTO))
            {
              cfg_vect_add_cmd (cv, "ip nat %s destination static %s %s\n",
                                direction_str[direction],
                                addr, addr1);
              write++;
            }
          else
            {
              /* If virtual server configured from Web, ignore writing this
                 configuration as the virtual-server configuration is used
                 instead. */
              if (imi_virtual_server_check (filter_prot_str (srule->protocol,
                                                             proto_str),
                                            srule->dport,
                                            &srule->min_ip, srule->min_port))
                break;
              else
                {
                  cfg_vect_add_cmd (cv,
                                    "ip nat %s destination static %s %s %d %s %d\n",
                                    direction_str[direction],
                                    filter_prot_str (srule->protocol,
                                                     proto_str),
                                    addr, srule->dport,
                                    addr1, srule->min_port);
                  write++;
                }
            }
          break;
        default:
          break;
        }
    }

  return write;
}

static int
imi_nat_translations_timeout_config_encode (cfg_vect_t *cv)
{
  int written = 0;

  if (nat_time_out.generic_timeout != DEFAULT_GENERIC_TIMEOUT &&
      nat_time_out.generic_timeout != 0 )
    {
      cfg_vect_add_cmd (cv, "ip nat translations time-out %d\n",
               nat_time_out.generic_timeout);
      written++;
    }
  if (nat_time_out.icmp_timeout != DEFAULT_ICMP_TIMEOUT &&
      nat_time_out.icmp_timeout != 0)
    {
      cfg_vect_add_cmd (cv, "ip nat translations icmp-timeout %d\n",
                        nat_time_out.icmp_timeout);
      written++;
    }

  if (nat_time_out.tcp_timeout != DEFAULT_TCP_TIMEOUT &&
      nat_time_out.tcp_timeout != 0)
    {
      cfg_vect_add_cmd (cv, "ip nat translations tcp-timeout %d\n",
                        nat_time_out.tcp_timeout);
      written++;
    }

  if (nat_time_out.tcp_fin_timeout != DEFAULT_TCP_FIN_TIMEOUT &&
      nat_time_out.tcp_fin_timeout != 0)
    {
      cfg_vect_add_cmd (cv, "ip nat translations tcp-fin-timeout %d\n",
                        nat_time_out.tcp_fin_timeout);
      written++;
    }

  if (nat_time_out.tcp_timeout != DEFAULT_UDP_TIMEOUT &&
      nat_time_out.udp_timeout != 0)
    {
      cfg_vect_add_cmd (cv, "ip nat translations udp-timeout %d\n",
                        nat_time_out.udp_timeout);
      written++;
    }

  return written;
}

int
imi_nat_config_encode (cfg_vect_t *cv)
{
  int write = 0;

  write += imi_nat_pool_config_encode (cv);
  write += imi_nat_translations_config_encode (IMI_NAT_INSIDE, cv);
  write += imi_nat_translations_config_encode (IMI_NAT_OUTSIDE, cv);

  if (write) {
    cfg_vect_add_cmd(cv, "!\n");
  }
  imi_nat_translations_timeout_config_encode(cv);
  if (write) {
    cfg_vect_add_cmd(cv, "!\n");
  }

  return write;
}

int
imi_nat_config_write (struct cli *cli)
{
  cli->cv = cfg_vect_init(cli->cv);
  imi_nat_config_encode(cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
  return 0;
}

static void
imi_nat_show_inside_src_translations (struct vrep_table *vrep)
{
  struct list *rlist = NULL;
  struct imi_rule *rule;
  struct filter_list *filter;
  struct imi_rule_static *srule;
  struct listnode *node;
  struct access_list *acl = NULL;
  struct imi_nat_address_pool *pool = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (imim);
  char   pstr[PREFIX_AM_STR_SIZE+1];

  rlist = IMI_NAT_RLIST(IMI_NAT_INSIDE, IMI_NAT_SOURCE);

  for (node = LISTHEAD (rlist); node; NEXTNODE (node))
    {
      rule = GETDATA (node);

      switch (rule->flag)
        {
        case IMI_RULE_ACL:
          {
            acl = access_list_lookup (vr, AFI_IP, rule->u.arule.acl);
            if (!acl)
              continue;

            pool = imi_nat_address_pool_lookup (imi_nat_address_pool_list,
                                                rule->u.arule.pool);
            if (!pool)
              continue;
            for (filter = acl->head; filter; filter = filter->next)
              {
                if (filter->common != FILTER_PACOS_EXT)
                  continue;

                vrep_add_next_row (vrep, NULL,
                                   " Src \t %s-%s \t %r \t---\t---\t %s \t %s ",
                         pool->min_ip, pool->max_ip,
                                    (prefix2str_am(&filter->u.zextfilter.sprefix,
                                                   pstr,
                                                   PREFIX_AM_STR_SIZE,
                                                   '/') == 0) ? pstr : "???",
                         acl->name,
                         pool->name);
              }
          }
          break;
        case IMI_RULE_STATIC:
          srule = &rule->u.srule;

          vrep_add_next_row (vrep, NULL,
                             " Src \t %r \t %r \t---\t---\n",
                   &srule->min_ip, &srule->sprefix.addr4);
          break;
        default:
          break;
        }
    }
}

static void
imi_nat_show_inside_dst_translations (struct vrep_table *vrep)
{
  struct list *rlist = NULL;
  struct imi_rule *rule;
  struct filter_list *filter;
  struct imi_rule_static *srule;
  struct listnode *node;
  struct access_list *acl = NULL;
  struct imi_nat_address_pool *pool = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (imim);
  char   pstr[PREFIX_AM_STR_SIZE+1];

  rlist = IMI_NAT_RLIST(IMI_NAT_INSIDE, IMI_NAT_DESTINATION);

  for (node = LISTHEAD (rlist); node; NEXTNODE (node))
    {
      rule = GETDATA (node);

      switch (rule->flag)
        {
        case IMI_RULE_ACL:
          {
            acl = access_list_lookup (vr, AFI_IP, rule->u.arule.acl);
            if (!acl)
              continue;

            pool = imi_nat_address_pool_lookup (imi_nat_address_pool_list,
                                                rule->u.arule.pool);
            if (!pool)
              continue;

            for (filter = acl->head; filter; filter = filter->next)
              {
                if (filter->common != FILTER_PACOS_EXT)
                  continue;

                vrep_add_next_row (vrep, NULL,
                                   " Dst \t %s-%s \t %s \t---\t---\t %s \t %s ",
                         pool->min_ip, pool->max_ip,
                                   (prefix2str_am(&filter->u.zextfilter.dprefix,
                                                  pstr,
                                                  PREFIX_AM_STR_SIZE,
                                                  '/') == 0) ? pstr : "???",
                         acl->name,
                         pool->name);
              }
          }
          break;
        case IMI_RULE_STATIC:
          srule = &rule->u.srule;

          vrep_add_next_row (vrep, NULL,
                             " Dst \t %r \t %r \t---\t---",
                             &srule->min_ip, &srule->dprefix.addr4);
          break;
        default:
          break;
        }
    }
}

static void
imi_nat_show_outside_src_translations (struct vrep_table *vrep)
{
  struct list *rlist = NULL;
  struct imi_rule *rule;
  struct listnode *node;
  struct filter_list *filter;
  struct imi_rule_static *srule;
  struct access_list *acl = NULL;
  struct imi_nat_address_pool *pool = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (imim);
  char   pstr[PREFIX_AM_STR_SIZE+1];

  rlist = IMI_NAT_RLIST(IMI_NAT_OUTSIDE, IMI_NAT_SOURCE);

  for (node = LISTHEAD (rlist); node; NEXTNODE (node))
    {
      rule = GETDATA (node);

      switch (rule->flag)
        {
        case IMI_RULE_ACL:
          {
            acl = access_list_lookup (vr, AFI_IP, rule->u.arule.acl);
            if (!acl)
              continue;

            pool = imi_nat_address_pool_lookup (imi_nat_address_pool_list,
                                                rule->u.arule.pool);
            if (!pool)
              continue;

            for (filter = acl->head; filter; filter = filter->next)
              {
                if (filter->common != FILTER_PACOS_EXT)
                  continue;

                vrep_add_next_row (vrep, NULL,
                                   " Src \t---\t---\t %s \t %s-%s \t %s \t %s ",
                                   (prefix2str_am(&filter->u.zextfilter.sprefix,
                                                  pstr,
                                                  PREFIX_AM_STR_SIZE,
                                                  '/') == 0) ? pstr : "???",
                         pool->min_ip, pool->max_ip,
                         acl->name,
                         pool->name);
              }
          }
          break;
        case IMI_RULE_STATIC:
          srule = &rule->u.srule;

          vrep_add_next_row (vrep, NULL,
                             " Src \t---\t---\t %r \t %r ",
                             &srule->sprefix.addr4, &srule->min_ip);
          break;
        default:
          break;
        }
    }
}

static void
imi_nat_show_outside_dst_translations (struct vrep_table *vrep)
{
  struct list *rlist = NULL;
  struct imi_rule *rule;
  struct listnode *node;
  struct imi_rule_static *srule;
  struct filter_list *filter;
  struct access_list *acl = NULL;
  struct imi_nat_address_pool *pool = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (imim);
  char proto_str[32];
  char   pstr[PREFIX_AM_STR_SIZE+1];

  rlist = IMI_NAT_RLIST(IMI_NAT_OUTSIDE, IMI_NAT_DESTINATION);

  for (node = LISTHEAD (rlist); node; NEXTNODE (node))
    {
      rule = GETDATA (node);

      switch (rule->flag)
        {
        case IMI_RULE_ACL:
          {
            acl = access_list_lookup (vr, AFI_IP, rule->u.arule.acl);
            if (!acl)
              continue;

            pool = imi_nat_address_pool_lookup (imi_nat_address_pool_list,
                                                rule->u.arule.pool);
            if (!pool)
              continue;

            for (filter = acl->head; filter; filter = filter->next)
              {
                if (filter->common != FILTER_PACOS_EXT)
                  continue;

                vrep_add_next_row (vrep, NULL,
                                   " Dst \t---\t---\t %r \t%s-%s\t %s \t %s ",
                                   (prefix2str_am(&filter->u.zextfilter.dprefix,
                                                  pstr,
                                                  PREFIX_AM_STR_SIZE,
                                                  '/') == 0) ? pstr : "???",
                                   pool->min_ip, pool->max_ip,
                                   acl->name,
                                   pool->name);
              }
          }
          break;
        case IMI_RULE_STATIC:
          srule = &rule->u.srule;
          if (srule->protocol)
            vrep_add_next_row (vrep, NULL,
                               " Dst \t---\t---\t%r\t%r\t---\t---\t%s\t%d->%d",
                               &srule->dprefix.addr4,  &srule->min_ip,
                               filter_prot_str (srule->protocol, proto_str),
                               srule->dport,
                               srule->min_port);
            else
             vrep_add_next_row (vrep, NULL,
                                " Dst \t---\t---\t%r\t%r\t---\t---",
                                &srule->dprefix.addr4,  &srule->min_ip);
          break;
        default:
          break;
        }
    }
}

static void
imi_nat_show_inside_translations (struct vrep_table *vrep)
{
  imi_nat_show_inside_src_translations (vrep);
  imi_nat_show_inside_dst_translations (vrep);
}

static void
imi_nat_show_outside_translations (struct vrep_table *vrep)
{
  imi_nat_show_outside_src_translations (vrep);
  imi_nat_show_outside_dst_translations (vrep);
}

void
imi_nat_show_translations (struct vrep_table *vrep)
{
  imi_nat_show_inside_translations (vrep);
  imi_nat_show_outside_translations (vrep);
}

static void
imi_nat_clear_inside_src_translations ()
{
  struct list *rlist = NULL;
  struct imi_rule *rule;
  struct listnode *node, *node_next;

  rlist = IMI_NAT_RLIST(IMI_NAT_INSIDE, IMI_NAT_SOURCE);

  for (node = LISTHEAD (rlist); node; node = node_next)
    {
      node_next = node->next;

      rule = GETDATA (node);

      if (rule->flag == IMI_RULE_STATIC)
        imi_nat_translation_static (IMI_NAT_INSIDE, rule, IMI_NAT_SOURCE, 0);
      else if (rule->flag == IMI_RULE_ACL)
        imi_nat_translation_acl (IMI_NAT_INSIDE, rule, IMI_NAT_SOURCE, 0);
    }
}

static void
imi_nat_clear_inside_dst_translations ()
{
  struct list *rlist = NULL;
  struct imi_rule *rule;
  struct listnode *node, *node_next;

  rlist = IMI_NAT_RLIST(IMI_NAT_INSIDE, IMI_NAT_DESTINATION);

  for (node = LISTHEAD (rlist); node; node = node_next)
    {
      node_next = node->next;

      rule = GETDATA (node);

      if (rule->flag == IMI_RULE_STATIC)
        imi_nat_translation_static (IMI_NAT_INSIDE, rule, IMI_NAT_DESTINATION, 0);
      else if (rule->flag == IMI_RULE_ACL)
        imi_nat_translation_acl (IMI_NAT_INSIDE, rule, IMI_NAT_DESTINATION, 0);
    }
}

static void
imi_nat_clear_outside_src_translations ()
{
  struct list *rlist = NULL;
  struct imi_rule *rule;
  struct listnode *node, *node_next;

  rlist = IMI_NAT_RLIST(IMI_NAT_OUTSIDE, IMI_NAT_SOURCE);

  for (node = LISTHEAD (rlist); node; node = node_next)
    {
      node_next = node->next;

      rule = GETDATA (node);

      if (rule->flag == IMI_RULE_STATIC)
        imi_nat_translation_static (IMI_NAT_OUTSIDE, rule, IMI_NAT_SOURCE, 0);
      else if (rule->flag == IMI_RULE_ACL)
        imi_nat_translation_acl (IMI_NAT_OUTSIDE, rule, IMI_NAT_SOURCE, 0);
    }
}

static void
imi_nat_clear_outside_dst_translations ()
{
  struct list *rlist = NULL;
  struct imi_rule *rule;
  struct listnode *node, *node_next;
  char proto_str[32];

  rlist = IMI_NAT_RLIST(IMI_NAT_OUTSIDE, IMI_NAT_DESTINATION);

  for (node = LISTHEAD (rlist); node; node = node_next)
    {
      node_next = node->next;

      rule = GETDATA (node);

      /* Check for existing virtual server. If virtual server with similar
         configuration exists, reject command. */
      if (rule->u.srule.protocol == TCP_PROTO ||
          rule->u.srule.protocol == UDP_PROTO)
        {
          if (imi_virtual_server_check (filter_prot_str (rule->u.srule.protocol,
                                                         proto_str),
                                        rule->u.srule.dport,
                                        &rule->u.srule.min_ip,
                                        rule->u.srule.min_port))
            continue;
          else
            {
              if (rule->flag == IMI_RULE_STATIC)
                imi_nat_translation_static (IMI_NAT_OUTSIDE, rule,
                                            IMI_NAT_DESTINATION, 0);
              else if (rule->flag == IMI_RULE_ACL)
                imi_nat_translation_acl (IMI_NAT_OUTSIDE, rule,
                                         IMI_NAT_DESTINATION, 0);
            }
        }
      else
        {
          if (rule->flag == IMI_RULE_STATIC)
            imi_nat_translation_static (IMI_NAT_OUTSIDE, rule,
                                        IMI_NAT_DESTINATION, 0);
          else if (rule->flag == IMI_RULE_ACL)
            imi_nat_translation_acl (IMI_NAT_OUTSIDE, rule,
                                     IMI_NAT_DESTINATION, 0);
        }
    }
}

static void
imi_nat_clear_inside_translations ()
{
  imi_nat_clear_inside_src_translations ();
  imi_nat_clear_inside_dst_translations ();
}

static void
imi_nat_clear_outside_translations ()
{
  imi_nat_clear_outside_src_translations ();
  imi_nat_clear_outside_dst_translations ();
}

void
imi_nat_clear_translations ()
{
  imi_nat_clear_inside_translations ();
  imi_nat_clear_outside_translations ();
}

s_int16_t
imi_nat_vrep_show_cb (intptr_t usr_ref, char *str)
{
  struct cli *cli = (struct cli *)usr_ref;

  if (! cli)
    return VREP_ERROR;

  cli_out (cli, "%s", str);
  return VREP_SUCCESS;
}

void
imi_nat_show_translation_timeout_all(struct cli *cli,
                                     struct imi_nat_timeouts *timeout)
{
  int i = 0;

  for (i = 0; i < IMI_NAT_MAX_TIME_OUTS; i++)
     {
       timeout->flag = 1 << i;
       pal_nat_translation_timeout_get(timeout);
     }

  cli_out (cli, "\n");
  cli_out (cli, "%-16s\t%-16s\n", "Time-out", "Value");
  cli_out (cli, "%-16s\t%-16s\n", "========", "=====");
  cli_out(cli, "%Generic time-out\t%d\n", timeout->generic_timeout);
  cli_out(cli, "%ICMP time-out\t\t%d\n", timeout->icmp_timeout);
  cli_out(cli, "%UDP time-out\t\t%d\n", timeout->udp_timeout);
  cli_out(cli, "%TCP time-out\t\t%d\n", timeout->tcp_timeout);
  cli_out(cli, "%TCP FIN time-out \t%d\n", timeout->tcp_fin_timeout);

}

/* Show NAT translations. */
CLI (show_ip_nat_trans,
     show_ip_nat_trans_cmd,
     "show ip nat translations",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "translations")
{
  struct vrep_table *vrep = vrep_create (IMI_NAT_OUT_DST_CLI_VREP_NUM_COLS,
                                         VREP_MAX_ROW_WIDTH);
  if (vrep == NULL) {
    cli_out(cli, "% Cannot initiate report.");
    return CLI_ERROR;
  }
  /* Two rows header. */
  vrep_add (vrep, 0, 0, " S/D \t Inside \t Inside \t Outside "
                         "\t Outside \t ACL \t NAT \t Prot \t Port ");
  vrep_add (vrep, 1, 0, " \t global \t local \t global "
                         "\t local \t name \t pool \t \t transl. ");

  imi_nat_show_translations (vrep);

  vrep_show (vrep, imi_nat_vrep_show_cb, (intptr_t)cli);
  vrep_delete (vrep);

  return CLI_SUCCESS;
}

/* Show NAT inside source translations. */
CLI (show_ip_nat_inside_src_trans,
     show_ip_nat_inside_src_trans_cmd,
     "show ip nat translations inside source",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "translations",
     "Inside",
     "Source")
{
  struct vrep_table *vrep = vrep_create (IMI_NAT_OUT_DST_CLI_VREP_NUM_COLS,
                                         VREP_MAX_ROW_WIDTH);

  if (vrep == NULL) {
    cli_out(cli, "% Cannot initiate report.");
    return CLI_ERROR;
  }
  /* Two rows header. */
  vrep_add (vrep, 0, 0, " S/D \t Inside \t Inside \t Outside "
                         "\t Outside \t ACL \t NAT \t Prot \t Port ");
  vrep_add (vrep, 1, 0, " \t global \t local \t global "
                         "\t local \t name \t pool \t \t transl. ");

  imi_nat_show_inside_src_translations (vrep);

  vrep_show (vrep, imi_nat_vrep_show_cb, (intptr_t)cli);
  vrep_delete (vrep);

  return CLI_SUCCESS;
}

/* Show NAT inside destination translations. */
CLI (show_ip_nat_inside_dst_trans,
     show_ip_nat_inside_dst_trans_cmd,
     "show ip nat translations inside destination",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "translations",
     "Inside",
     "Destination")
{
  struct vrep_table *vrep = vrep_create (IMI_NAT_CLI_VREP_NUM_COLS,
                                         VREP_MAX_ROW_WIDTH);
  if (vrep == NULL) {
    cli_out(cli, "% Cannot initiate report.");
    return CLI_ERROR;
  }
  /* Two rows header. */
  vrep_add (vrep, 0, 0, " S/D \t Inside \t Inside \t Outside "
                         "\t Outside \t ACL \t NAT ");
  vrep_add (vrep, 1, 0, " \t global \t local \t global "
                         "\t local \t name \t pool ");

  imi_nat_show_inside_dst_translations (vrep);

  vrep_show (vrep, imi_nat_vrep_show_cb, (intptr_t)cli);
  vrep_delete (vrep);

  return CLI_SUCCESS;
}

/* Show NAT outside source translations. */
CLI (show_ip_nat_outside_src_trans,
     show_ip_nat_outside_src_trans_cmd,
     "show ip nat translations outside source",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "translations",
     "Outside",
     "Source")
{
  struct vrep_table *vrep = vrep_create (IMI_NAT_CLI_VREP_NUM_COLS,
                                         VREP_MAX_ROW_WIDTH);
  if (vrep == NULL) {
    cli_out(cli, "% Cannot initiate report.");
    return CLI_ERROR;
  }
  /* Two rows header. */
  vrep_add (vrep, 0, 0, " S/D \t Inside \t Inside \t Outside "
                         "\t Outside \t ACL \t NAT ");
  vrep_add (vrep, 1, 0, " \t global \t local \t global "
                         "\t local \t name \t pool ");

  imi_nat_show_outside_src_translations (vrep);

  vrep_show (vrep, imi_nat_vrep_show_cb, (intptr_t)cli);
  vrep_delete (vrep);

  return CLI_SUCCESS;
}

/* Show NAT outside destination translations. */
CLI (show_ip_nat_outside_dst_trans,
     show_ip_nat_outside_dst_trans_cmd,
     "show ip nat translations outside destination",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "translations",
     "Outside",
     "Destination")
{
  struct vrep_table *vrep = vrep_create (IMI_NAT_OUT_DST_CLI_VREP_NUM_COLS,
                                         VREP_MAX_ROW_WIDTH);
  if (vrep == NULL) {
    cli_out(cli, "% Cannot initiate report.");
    return CLI_ERROR;
  }
  /* Two rows header. */
  vrep_add (vrep, 0, 0, " S/D \t Inside \t Inside \t Outside "
                         "\t Outside \t ACL \t NAT \t Prot \t Port ");
  vrep_add (vrep, 1, 0, " \t global \t local \t global "
                         "\t local \t name \t pool \t \t transl. ");

  imi_nat_show_outside_dst_translations (vrep);

  vrep_show (vrep, imi_nat_vrep_show_cb, (intptr_t)cli);
  vrep_delete (vrep);

  return CLI_SUCCESS;
}

#define PREFIX_ZERO(X,Y)                                \
        (((X).family == 0 && (Y).family == 0) &&        \
         ((X).prefixlen == 0 && (Y).prefixlen == 0))

int
imi_nat_process_static (int direction, int scope,
                        struct imi_rule *rule,
                        char *via_in, char *via_out,
                        struct imi_interface *imi_if,
                        int op)
{
  result_t res = IMI_NAT_SUCCESS;

  switch (op)
    {
    case 0:
      {
        struct imi_rule_static *s1;

        s1 = &rule->u.srule;

        /* Decrement interface scope reference count. */
        if (imi_if)
          imi_if->rule_ref_cnt--;

        /* Decrement # static rules for this chain. */
        nat_table.chain_tbl[scope].static_cnt--;

        res = pal_nat_translate_rule (op, direction, scope,
                                rule->u.srule.protocol,
                                &s1->sprefix, &s1->dprefix,
                                0, s1->sport, EQUAL,
                                0, s1->dport, EQUAL,
                                via_in, via_out,
                                &s1->min_ip, &s1->max_ip, s1->min_port,
                                s1->max_port, 0);

      }
    break;
    case 1:
      {
        /* Increment interface scope reference count. */
        if (imi_if)
          imi_if->rule_ref_cnt++;

        /* Increment # static rules for this chain. */
        nat_table.chain_tbl[scope].static_cnt++;

        res = pal_nat_translate_rule (op, direction, scope,
                                rule->u.srule.protocol,
                                &rule->u.srule.sprefix,
                                &rule->u.srule.dprefix,
                                0, rule->u.srule.sport, EQUAL,
                                0, rule->u.srule.dport, EQUAL,
                                via_in, via_out,
                                &rule->u.srule.min_ip, &rule->u.srule.max_ip,
                                rule->u.srule.min_port,
                                rule->u.srule.max_port, 1);

      }
    break;
    }
  return (res <= 0 ? res : IMI_NAT_BAD_KERNEL_RULE);
}

static int
imi_nat_check_translation_static (int direction, struct imi_rule *rule, int scope)
{
  int if_scope;
  struct list *rlist = NULL;
  struct imi_rule_static *s1, *s2;
  struct listnode *node;

  if_scope = via_interface [direction][scope];

  rlist = IMI_NAT_RLIST(direction, scope);

  if (rlist)
    {
      struct imi_rule *rule1;

      s1 = &rule->u.srule;

      for (node = LISTHEAD (rlist); node; NEXTNODE (node))
        {
          rule1 = GETDATA (node);

          s2 = &rule1->u.srule;

          if ((s1->protocol == s2->protocol) &&
              (PREFIX_ZERO (s1->sprefix, s2->sprefix) ||
               prefix_am_same ((struct prefix_am *)&s1->sprefix,
                               (struct prefix_am *)&s2->sprefix)) &&
              (s1->sport == s2->sport) &&
              (s1->dport == s2->dport) &&
              (PREFIX_ZERO (s1->dprefix, s2->dprefix) ||
               prefix_am_same ((struct prefix_am *)&s1->dprefix,
                               (struct prefix_am *)&s2->dprefix))
             &&
               IPV4_ADDR_SAME((struct pal_in4_addr *)&s1->min_ip,
                                  (struct pal_in4_addr *)&s2->min_ip))
            return 1;
        }
    }

  return 0;
}

int
imi_nat_translation_static (int direction, struct imi_rule *rule, int scope, int op)
{
  int if_scope = -1;
  struct list *rlist = NULL;
  struct interface *ifp = NULL;
  struct listnode *node = NULL;
  struct imi_rule *rdel = NULL;
  struct imi_rule *rnew = NULL;
  struct listnode *if_node = NULL;
  struct imi_interface *imi_if = NULL;
  char *via_in = NULL, *via_out = NULL;
  struct imi_rule_static *s1 = NULL, *s2 = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (imim);
  int ret = IMI_NAT_SUCCESS;

  if_scope = via_interface [direction][scope];

  if (imi_nat_check_translation_static (direction, rule, scope))
    {
      if (op)
        return IMI_NAT_RULE_EXISTS;
    }
  else
    {
      if (!op)
        return IMI_NAT_RULE_NOT_EXISTS;
    }

  rlist = IMI_NAT_RLIST(direction, scope);

  if (op == 1)
    {
      /* Add the rule to the rule-list */
      rnew = XCALLOC (MTYPE_IMI_RULE, sizeof( struct imi_rule));
      if (! rnew)
        return IMI_NAT_OUT_OF_MEM;

      pal_mem_cpy (rnew, rule, sizeof (struct imi_rule));

      /* Static rules always go first. */
      listnode_add_before (rlist, LISTHEAD(rlist), rnew);
    }

  for (if_node = LISTHEAD (vr->ifm.if_list); if_node; NEXTNODE (if_node))
    {
      ifp = GETDATA (if_node);
      imi_if = ifp->info;

      if (imi_if->scope == if_scope )
        {
          if (if_scope == NSM_IF_SCOPE_INSIDE)
            {
              via_in = ifp->name;
              via_out = NULL;
            }
          if (if_scope == NSM_IF_SCOPE_OUTSIDE)
            {
              via_out = ifp->name;
              via_in = NULL;
            }
          ret = imi_nat_process_static (direction, scope, rule, via_in,
                                  via_out, imi_if, op);
          if (ret < 0 && op)
            return ret;
        }
    }

  if (op == 0)
    {
      /* Find the rule and delete it. */
      LIST_LOOP(rlist, rdel, node)
        {
          s1 = &rdel->u.srule;
          s2 = &rule->u.srule;

          if ((s1->protocol == s2->protocol) &&
              (PREFIX_ZERO (s1->sprefix, s2->sprefix) |
               prefix_am_same ((struct prefix_am *)&s1->sprefix, 
                               (struct prefix_am *)&s2->sprefix)) &&
              (s1->sport == s2->sport) &&
              (s1->dport == s2->dport) &&
              (PREFIX_ZERO (s1->dprefix, s2->dprefix) ||
               prefix_am_same ((struct prefix_am *)&s1->dprefix, 
                               (struct prefix_am *)&s2->dprefix))
               &&
               IPV4_ADDR_SAME((struct pal_in4_addr *)&s1->min_ip,
                                      (struct pal_in4_addr *)&s2->min_ip))
            {
              list_delete_node (rlist, node);
              XFREE (MTYPE_IMI_RULE, rdel);
              break;
            }
        }
    }
  return IMI_NAT_SUCCESS;
}

int
imi_nat_process_acl (int direction,
                     int scope,
                     struct access_list *acl,
                     struct imi_nat_address_pool *pool,
                     char *via_in, char *via_out,
                     struct imi_interface *imi_if,
                     int op)
{
  struct prefix_am4 min_ip;
  struct prefix_am4 max_ip;
  struct filter_list *filter;
  u_int16_t rulenum;
  result_t res = IMI_NAT_SUCCESS;

  str2prefix_am4 (pool->min_ip, NULL, &min_ip);
  str2prefix_am4 (pool->max_ip, NULL, &max_ip);

  switch (op)
    {
    case 0:
      {
                /* Decrement interface scope reference count. */
        if (imi_if)
          imi_if->rule_ref_cnt--;

        for (filter = acl->head; filter; filter = filter->next)
          {
            if (filter->common != FILTER_PACOS_EXT)
              continue;

            imi_nat_chain_del_acl_num(scope, acl->name);

            res = pal_nat_translate_rule (op, direction, scope,
                                         filter->u.zextfilter.protocol,
                                         (struct prefix_am4 *)&filter->u.zextfilter.sprefix,
                                         (struct prefix_am4 *)&filter->u.zextfilter.dprefix,
                                         filter->u.zextfilter.sport_lo,
                                         filter->u.zextfilter.sport,
                                         filter->u.zextfilter.sport_op,
                                         filter->u.zextfilter.dport_lo,
                                         filter->u.zextfilter.dport,
                                         filter->u.zextfilter.dport_op,
                                         via_in, via_out,
                                         &min_ip.addr4, &max_ip.addr4, 0, 0, 0);
           if (res != IMI_NAT_SUCCESS)
             break;
          }
      }
    break;
    case 1:
      {
        /* Increment interface scope reference count. */
        if (imi_if)
	          imi_if->rule_ref_cnt++;

        for (filter = acl->head; filter; filter = filter->next)
          {
            if (filter->common != FILTER_PACOS_EXT)
              continue;

            rulenum = imi_nat_chain_add_acl_num(scope, acl->name);
            /* rulenum > 0 => insert; append otherwise */
             res = pal_nat_translate_rule (op, direction, scope,
                                           filter->u.zextfilter.protocol,
                                           (struct prefix_am4 *)&filter->u.zextfilter.sprefix,
                                           (struct prefix_am4 *)&filter->u.zextfilter.dprefix,
                                            filter->u.zextfilter.sport_lo,
                                            filter->u.zextfilter.sport,
                                            filter->u.zextfilter.sport_op,
                                            filter->u.zextfilter.dport_lo,
                                            filter->u.zextfilter.dport,
                                            filter->u.zextfilter.dport_op,
                                            via_in, via_out,
                                            &min_ip.addr4, &max_ip.addr4, 0, 0,
                                            rulenum);
            if (res != IMI_NAT_SUCCESS)
               break;
          }
      }
      break;
    }

  return (res <= 0 ? res : IMI_NAT_BAD_KERNEL_RULE);
}

static int
imi_nat_check_translation_acl (int direction, struct imi_rule *rule, int scope)
{
  struct list *rlist = NULL;
  struct imi_rule_acl *a1, *a2;


  rlist = IMI_NAT_RLIST(direction, scope);

  if (rlist)
    {
      struct listnode *node;
      struct imi_rule *rule1;

      a2 = &rule->u.arule;

      for (node = LISTHEAD (rlist); node; NEXTNODE (node))
        {
          rule1 = GETDATA (node);

          a1 = &rule1->u.arule;

          if (! pal_strcmp (a1->acl, a2->acl) &&
              ! pal_strcmp (a1->pool, a2->pool))
            return 1;
        }
    }

  return 0;
}

int
imi_nat_translation_acl (int direction, struct imi_rule *rule, int scope, int op)
{
  struct list *rlist = NULL;
  struct interface *ifp = NULL;
  struct access_list *acl = NULL;
  struct imi_nat_address_pool *pool = NULL;
  int if_scope = -1;
  struct listnode *node = NULL;
  struct imi_interface *imi_if = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (imim);
  char *via_in = NULL, *via_out = NULL;
  struct imi_rule *rdel = NULL, *rnew = NULL;
  int ret = IMI_NAT_SUCCESS;

  if_scope = via_interface [direction][scope];

  acl = access_list_lookup (vr, AFI_IP, rule->u.arule.acl);
  if (!acl)
    return IMI_NAT_ACL_ERROR;

  if (acl->head->common != FILTER_PACOS_EXT)
    return IMI_NAT_ACL_NOT_PERMITTED;

  pool = imi_nat_address_pool_lookup (imi_nat_address_pool_list, rule->u.arule.pool);
  if (!pool)
    return IMI_NAT_POOL_ERROR;

  if (imi_nat_check_translation_acl (direction, rule, scope))
    {
      if (op)
        return IMI_NAT_RULE_EXISTS;
    }
  else
    {
      if (! op)
        return IMI_NAT_RULE_NOT_EXISTS;
    }

  rlist = IMI_NAT_RLIST(direction, scope);

  /* Add the new rule. */
  if (op == 1)
    {
      rnew = XCALLOC (MTYPE_IMI_RULE, sizeof( struct imi_rule));
      if (!rnew)
        return IMI_NAT_OUT_OF_MEM;
      pal_strcpy (rnew->u.arule.acl, acl->name);
      pal_strcpy (rnew->u.arule.pool, pool->name);
      /* Add rule to list. */
      listnode_add (rlist, rnew);

      /* Increment pool reference count. */
      if (pool)
        pool->refcnt++;
      acl->ref_cnt++;
    }

  /* Apply the addition or deletion. */
  for (node = LISTHEAD (vr->ifm.if_list); node; NEXTNODE (node))
    {
      ifp = GETDATA (node);
      imi_if = ifp->info;

      if (imi_if->scope == if_scope )
        {
          if (if_scope == NSM_IF_SCOPE_INSIDE)
            {
              via_in = ifp->name;
              via_out= NULL;
            }
          else if (if_scope == NSM_IF_SCOPE_OUTSIDE)
            {
              via_out = ifp->name;
              via_in = NULL;
            }
          ret = imi_nat_process_acl (direction, scope, acl, pool, via_in, via_out,
                                     imi_if, op);
          if (ret < 0 && op)
            return ret;
        }
    }

  /* Delete the rule. */
  if (op == 0)
    {
      LIST_LOOP(rlist, rdel, node)
        {
          if (!strcmp (rdel->u.arule.acl, acl->name) &&
              !strcmp (rdel->u.arule.pool, pool->name))
            {
              list_delete_node (rlist, node);
              XFREE (MTYPE_IMI_RULE, rdel);
              /* Decrement pool reference count. */
              if (pool)
                pool->refcnt--;
              acl->ref_cnt--;
              break;
            }
        }
    }

  return IMI_NAT_SUCCESS;
}

int
imi_nat_cli_cmd_ret(struct cli *cli, int ret)
{
  switch (ret)
    {
    case IMI_NAT_SUCCESS:
      return CLI_SUCCESS;
    case IMI_NAT_ACL_ERROR:
      cli_out (cli, "%% Access list not defined.\n");
      break;
    case IMI_NAT_POOL_ERROR:
      cli_out (cli, "%% NAT pool not defined.\n");
      break;
    case IMI_NAT_IF_ERROR:
      cli_out (cli, "Interface configuration error.\n");
      break;
    case IMI_NAT_NO_RULE_FOUND:
      cli_out (cli, "NAT Rule not found.\n");
      break;
    case IMI_NAT_RULE_EXISTS:
      cli_out (cli, "NAT rule already exists.\n");
      break;
    case IMI_NAT_RULE_NOT_EXISTS:
      cli_out (cli, "NAT rule does not exist.\n");
      break;
    case IMI_NAT_ACL_NOT_PERMITTED:
      cli_out (cli, "%% Only \"access-list pacos\" are permitted.\n");
      break;
    case IMI_NAT_BAD_KERNEL_RULE:
      cli_out (cli, "Failure to install kernel rule.\n");
      break;
    case IMI_NAT_POOL_NO_MATCH:
      cli_out (cli, "No pool matches the given pool name.\n");
      break;
    case IMI_NAT_OUT_OF_MEM:
      cli_out (cli, "Out of memory.\n");
      break;
    default:
      cli_out (cli, "Failure in processing command.\n");
      break;
    }
  return CLI_ERROR;
}
/* Static translation between inside local address and inside
 * global address. */
CLI (ip_nat_inside_src_static,
     ip_nat_inside_src_static_cmd,
     "ip nat inside source static A.B.C.D A.B.C.D",
     CLI_IP_STR,
     CLI_NAT_STR,
     "Inside",
     "Source address",
     "Static",
     "Inside local IP address (A.B.C.D)",
     "Inside global IP address (A.B.C.D)")
{
  struct imi_rule rule;
  struct prefix_am4 addr;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_STATIC;

  SET_FLAG (rule.u.srule.flags, IMI_RULE_SOURCE);
  str2prefix_am4 (argv[0], NULL, &rule.u.srule.sprefix);

  str2prefix_am4 (argv[1], NULL, &addr);
  rule.u.srule.min_ip = addr.addr4;
  rule.u.srule.max_ip = addr.addr4;

  ret = imi_nat_translation_static (IMI_NAT_INSIDE, &rule, IMI_NAT_SOURCE, 1);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

CLI (no_ip_nat_inside_src_static,
     no_ip_nat_inside_src_static_cmd,
     "no ip nat inside source static A.B.C.D A.B.C.D",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Inside",
     "Source address",
     "Static",
     "Inside local IP address (A.B.C.D)",
     "Inside global IP address (A.B.C.D)")
{
  struct imi_rule rule;
  struct prefix_am4 addr;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_STATIC;

  SET_FLAG (rule.u.srule.flags, IMI_RULE_SOURCE);
  str2prefix_am4 (argv[0], NULL, &rule.u.srule.sprefix);

  str2prefix_am4 (argv[1], NULL, &addr);
  rule.u.srule.min_ip = addr.addr4;
  rule.u.srule.max_ip = addr.addr4;

  ret = imi_nat_translation_static (IMI_NAT_INSIDE, &rule, IMI_NAT_SOURCE, 0);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

CLI (ip_nat_inside_dst_static,
     ip_nat_inside_dst_static_cmd,
     "ip nat inside destination static A.B.C.D A.B.C.D",
     CLI_IP_STR,
     CLI_NAT_STR,
     "Inside",
     "Destination address",
     "Static",
     "Inside local IP address (A.B.C.D)",
     "Inside global IP address (A.B.C.D)")
{
  struct imi_rule rule;
  struct prefix_am4 addr;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_STATIC;

  SET_FLAG (rule.u.srule.flags, IMI_RULE_DESTINATION);
  str2prefix_am4 (argv[0],NULL, &rule.u.srule.dprefix);

  str2prefix_am4 (argv[1],NULL, &addr);
  rule.u.srule.min_ip = addr.addr4;
  rule.u.srule.max_ip = addr.addr4;

  ret = imi_nat_translation_static (IMI_NAT_INSIDE, &rule, IMI_NAT_DESTINATION, 1);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

CLI (no_ip_nat_inside_dst_static,
     no_ip_nat_inside_dst_static_cmd,
     "no ip nat inside destination static A.B.C.D A.B.C.D",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Inside",
     "Destination address",
     "Static",
     "Inside local IP address (A.B.C.D)",
     "Inside global IP address (A.B.C.D)")
{
  struct imi_rule rule;
  struct prefix_am4 addr;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_STATIC;

  SET_FLAG (rule.u.srule.flags, IMI_RULE_DESTINATION);
  str2prefix_am4 (argv[0],NULL, &rule.u.srule.dprefix);

  str2prefix_am4 (argv[1],NULL, &addr);
  rule.u.srule.min_ip = addr.addr4;
  rule.u.srule.max_ip = addr.addr4;

  ret = imi_nat_translation_static (IMI_NAT_INSIDE, &rule, IMI_NAT_DESTINATION, 0);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

/* Static translation between inside local address and inside
 * global address.
 * Packets with source address that pass the access list are
 * dynamically translated using the addresses in the named pool. */
CLI (ip_nat_inside_src_access_list_pool_static,
     ip_nat_inside_src_access_list_pool_static_cmd,
     "ip nat inside source list WORD pool WORD",
     CLI_IP_STR,
     CLI_NAT_STR,
     "Inside",
     "Source address",
     "Access list",
     "Access list name",
     "Address pool",
     "Address pool name")
{
  struct imi_rule rule;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_ACL;

  pal_strcpy (rule.u.arule.acl, argv[0]);
  pal_strcpy (rule.u.arule.pool, argv[1]);

  ret = imi_nat_translation_acl (IMI_NAT_INSIDE, &rule, IMI_NAT_SOURCE, 1);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

CLI (no_ip_nat_inside_src_access_list_pool_static,
     no_ip_nat_inside_src_access_list_pool_static_cmd,
     "no ip nat inside source list WORD pool WORD",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Inside",
     "Source address",
     "Access list",
     "Access list name",
     "Address pool",
     "Address pool name")
{
  struct imi_rule rule;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_ACL;

  pal_strcpy (rule.u.arule.acl, argv[0]);
  pal_strcpy (rule.u.arule.pool, argv[1]);

  ret = imi_nat_translation_acl (IMI_NAT_INSIDE, &rule, IMI_NAT_SOURCE, 0);
  
  return (imi_nat_cli_cmd_ret(cli, ret));
}

/* Translation between inside destination address and inside
 * global address.
 * Packets with source address that pass the access list are
 * dynamically translated using the addresses in the named pool. */
CLI (ip_nat_inside_dst_access_list_pool_static,
     ip_nat_inside_dst_access_list_pool_static_cmd,
     "ip nat inside destination list WORD pool WORD",
     CLI_IP_STR,
     CLI_NAT_STR,
     "Inside",
     "Destination address",
     "Access list",
     "Access list name",
     "Address pool",
     "Address pool name")
{
  struct imi_rule rule;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_ACL;

  pal_strcpy (rule.u.arule.acl, argv[0]);
  pal_strcpy (rule.u.arule.pool, argv[1]);

  ret = imi_nat_translation_acl (IMI_NAT_INSIDE, &rule, IMI_NAT_DESTINATION, 1);
  
  return (imi_nat_cli_cmd_ret(cli, ret));
}

CLI (no_ip_nat_inside_dst_access_list_pool_static,
     no_ip_nat_inside_dst_access_list_pool_static_cmd,
     "no ip nat inside destination list WORD pool WORD",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Inside",
     "Destination address",
     "Access list",
     "Access list name",
     "Address pool",
     "Address pool name")
{
  struct imi_rule rule;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_ACL;

  pal_strcpy (rule.u.arule.acl, argv[0]);
  pal_strcpy (rule.u.arule.pool, argv[1]);

  ret = imi_nat_translation_acl (IMI_NAT_INSIDE, &rule, IMI_NAT_DESTINATION, 0);

  return (imi_nat_cli_cmd_ret(cli, ret))	;
}

/* Static translation between outside source address and inside
 * local address. */
CLI (ip_nat_outside_src_static,
     ip_nat_outside_src_static_cmd,
     "ip nat outside source static A.B.C.D A.B.C.D",
     CLI_IP_STR,
     CLI_NAT_STR,
     "Outside",
     "Source address",
     "Static",
     "Outside global IP address (A.B.C.D)",
     "Outside local IP address (A.B.C.D)")
{
  struct imi_rule rule;
  struct prefix_am4 addr;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_STATIC;

  SET_FLAG (rule.u.srule.flags, IMI_RULE_SOURCE);
  str2prefix_am4 (argv[0], NULL, &rule.u.srule.sprefix);

  str2prefix_am4 (argv[1], NULL, &addr);
  rule.u.srule.min_ip = addr.addr4;
  rule.u.srule.max_ip = addr.addr4;

  ret = imi_nat_translation_static (IMI_NAT_OUTSIDE, &rule, IMI_NAT_SOURCE, 1);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

CLI (no_ip_nat_outside_src_static,
     no_ip_nat_outside_src_static_cmd,
     "no ip nat outside source static A.B.C.D A.B.C.D",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Outside",
     "Source address",
     "Static",
     "Outside global IP address (A.B.C.D)",
     "Outside local IP address (A.B.C.D)")
{
  struct imi_rule rule;
  struct prefix_am4 addr;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_STATIC;

  SET_FLAG (rule.u.srule.flags, IMI_RULE_SOURCE);
  str2prefix_am4 (argv[0], NULL, &rule.u.srule.sprefix);

  str2prefix_am4 (argv[1], NULL, &addr);
  rule.u.srule.min_ip = addr.addr4;
  rule.u.srule.max_ip = addr.addr4;

  ret = imi_nat_translation_static (IMI_NAT_OUTSIDE, &rule, IMI_NAT_SOURCE, 0);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

/* Have to find how Cisco does this translation. Its not present in the
 * manuals.
 */
CLI (ip_nat_outside_dst_static,
     ip_nat_outside_dst_static_cmd,
     "ip nat outside destination static A.B.C.D A.B.C.D",
     CLI_IP_STR,
     CLI_NAT_STR,
     "Outside",
     "Destination address",
     "Static",
     "Outside global IP address (A.B.C.D)",
     "Outside local IP address (A.B.C.D)")
{
  struct imi_rule rule;
  struct prefix_am4 addr;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_STATIC;

  SET_FLAG (rule.u.srule.flags, IMI_RULE_DESTINATION);
  str2prefix_am4 (argv[0], NULL ,&rule.u.srule.dprefix);

  str2prefix_am4 (argv[1], NULL, &addr);
  rule.u.srule.min_ip = addr.addr4;
  rule.u.srule.max_ip = addr.addr4;

  ret = imi_nat_translation_static (IMI_NAT_OUTSIDE, &rule, IMI_NAT_DESTINATION, 1);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

CLI (no_ip_nat_outside_dst_static,
     no_ip_nat_outside_dst_static_cmd,
     "no ip nat outside destination static A.B.C.D A.B.C.D",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Outside",
     "Destination address",
     "Static",
     "Outside global IP address (A.B.C.D)",
     "Outside local IP address (A.B.C.D)")
{
  struct imi_rule rule;
  struct prefix_am4 addr;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_STATIC;

  SET_FLAG (rule.u.srule.flags, IMI_RULE_DESTINATION);
  str2prefix_am4 (argv[0], NULL, &rule.u.srule.dprefix);

  str2prefix_am4 (argv[1], NULL, &addr);
  rule.u.srule.min_ip = addr.addr4;
  rule.u.srule.max_ip = addr.addr4;

  ret = imi_nat_translation_static (IMI_NAT_OUTSIDE, &rule, IMI_NAT_DESTINATION, 0);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

/* Outside destination with port translations. */
CLI (ip_nat_outside_dst_static_port,
     ip_nat_outside_dst_static_port_cmd,
     "ip nat outside destination static (tcp|udp) A.B.C.D <0-65535> A.B.C.D <0-65535>",
     CLI_IP_STR,
     CLI_NAT_STR,
     "Outside",
     "Destination address",
     "Static",
     "TCP",
     "UDP",
     "Outside global IP address (A.B.C.D)",
     "Destination port number",
     "Outside local IP address (A.B.C.D)",
     "Translated destination port number")
{
  struct imi_rule rule;
  struct prefix_am4 addr;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_STATIC;

  rule.u.srule.protocol = protocol_type (argv[0]);

  SET_FLAG (rule.u.srule.flags, IMI_RULE_DESTINATION);
  str2prefix_am4 (argv[1], NULL, &rule.u.srule.dprefix);

  rule.u.srule.dport = pal_atoi (argv[2]);

  str2prefix_am4 (argv[3], NULL, &addr);
  rule.u.srule.min_ip = addr.addr4;
  rule.u.srule.max_ip = addr.addr4;

  rule.u.srule.min_port = pal_atoi 	(argv[4]);
  rule.u.srule.max_port = rule.u.srule.min_port;

  /* Check for existing virtual server. If virtual server with similar
     configuration exists, reject command. */
  if (imi_virtual_server_check (filter_prot_str(rule.u.srule.protocol, NULL),
                                rule.u.srule.dport,
                                &rule.u.srule.min_ip, rule.u.srule.min_port))
    {
      cli_out (cli, "Virtual server with similar configuration exists.\n");
      return CLI_ERROR;
    }

  ret = imi_nat_translation_static (IMI_NAT_OUTSIDE, &rule, IMI_NAT_DESTINATION, 1);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

CLI (no_ip_nat_outside_dst_static_port,
     no_ip_nat_outside_dst_static_port_cmd,
     "no ip nat outside destination static (tcp|udp) A.B.C.D <0-65535> A.B.C.D <0-65535>",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Outside",
     "Destination address",
     "Static",
     "TCP",
     "UDP",
     "Outside global IP address (A.B.C.D)",
     "Destination port number",
     "Outside local IP address (A.B.C.D)",
     "Translated destination port number")
{
  struct imi_rule rule;
  struct prefix_am4 addr;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_STATIC;

  rule.u.srule.protocol = protocol_type (argv[0]);

  SET_FLAG (rule.u.srule.flags, IMI_RULE_DESTINATION);
  str2prefix_am4 (argv[1], NULL, &rule.u.srule.dprefix);

  rule.u.srule.dport = pal_atoi (argv[2]);

  str2prefix_am4 (argv[3], NULL, &addr);
  rule.u.srule.min_ip = addr.addr4;
  rule.u.srule.max_ip = addr.addr4;

  rule.u.srule.min_port = pal_atoi (argv[4]);
  rule.u.srule.max_port = rule.u.srule.min_port;

  /* Check for existing virtual server. If virtual server with similar
     configuration exists, reject command. */
  if (imi_virtual_server_check (argv[0],
                                rule.u.srule.dport,
                                &rule.u.srule.min_ip, rule.u.srule.min_port))
    {
      cli_out (cli, "Virtual server with similar configuration exists.\n");
      return CLI_ERROR;
    }

  ret = imi_nat_translation_static (IMI_NAT_OUTSIDE, &rule, IMI_NAT_DESTINATION, 0);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

/* Static translation between outside source address and inside
 * local address.
 * Packets with source address that pass the access list are
 * dynamically translated using the addresses in the named pool. */
CLI (ip_nat_outside_src_access_list_pool_static,
     ip_nat_outside_src_access_list_pool_static_cmd,
     "ip nat outside source list WORD pool WORD",
     CLI_IP_STR,
     CLI_NAT_STR,
     "Outside",
     "Source address",
     "Access list",
     "Access list name",
     "Address pool",
     "Address pool name")
{
  struct imi_rule rule;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_ACL;

  pal_strcpy (rule.u.arule.acl, argv[0]);
  pal_strcpy (rule.u.arule.pool, argv[1]);

  ret = imi_nat_translation_acl (IMI_NAT_OUTSIDE, &rule, IMI_NAT_SOURCE, 1);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

CLI (no_ip_nat_outside_src_access_list_pool_static,
     no_ip_nat_outside_src_access_list_pool_static_cmd,
     "no ip nat outside source list WORD pool WORD",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Outside",
     "Source address",
     "Access list",
     "Access list name",
     "Address pool",
     "Address pool name")
{
  struct imi_rule rule;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_ACL;

  pal_strcpy (rule.u.arule.acl, argv[0]);
  pal_strcpy (rule.u.arule.pool, argv[1]);

  ret = imi_nat_translation_acl (IMI_NAT_OUTSIDE, &rule, IMI_NAT_SOURCE, 0);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

/* Have to find how Cisco does this translation. Its not present in the
 * manuals.
 */
CLI (ip_nat_outside_dst_access_list_pool_static,
     ip_nat_outside_dst_access_list_pool_static_cmd,
     "ip nat outside destination list WORD pool WORD",
     CLI_IP_STR,
     CLI_NAT_STR,
     "Outside",
     "Source address",
     "Access list",
     "Access list name",
     "Address pool",
     "Address pool name")
{
  struct imi_rule rule;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_ACL;

  pal_strcpy (rule.u.arule.acl, argv[0]);
  pal_strcpy (rule.u.arule.pool, argv[1]);

  ret = imi_nat_translation_acl (IMI_NAT_OUTSIDE, &rule, IMI_NAT_DESTINATION, 1);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

CLI (no_ip_nat_outside_dst_access_list_pool_static,
     no_ip_nat_outside_dst_access_list_pool_static_cmd,
     "no ip nat outside destination list WORD pool WORD",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Outside",
     "Destination address",
     "Access list",
     "Access list name",
     "Address pool",
     "Address pool name")
{
  struct imi_rule rule;
  int ret;

  pal_mem_set (&rule, 0, sizeof (struct imi_rule));

  rule.flag = IMI_RULE_ACL;

  pal_strcpy (rule.u.arule.acl, argv[0]);
  pal_strcpy (rule.u.arule.pool, argv[1]);

  ret = imi_nat_translation_acl (IMI_NAT_OUTSIDE, &rule, IMI_NAT_DESTINATION, 0);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

/* Define a pool of global addresses to be allocated as needed. */
CLI (ip_nat_pool,
     ip_nat_pool_cmd,
     "ip nat pool WORD A.B.C.D A.B.C.D A.B.C.D",
     CLI_IP_STR,
     CLI_NAT_STR,
     "Pool",
     "Name of the IP address pool",
     "Start IP address of pool (A.B.C.D)",
     "End IP address of pool (A.B.C.D)",
     "Netmask")
{
  int ret;
  ret = imi_nat_address_pool (argv[0], argv[1], argv[2], argv[3], 1);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

CLI (no_ip_nat_pool_by_name,
     no_ip_nat_pool_by_name_cmd,
     "no ip nat pool WORD",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Pool",
     "Name of the IP address pool")
{
  int ret;

  ret = imi_nat_address_pool (argv[0], NULL, NULL, NULL, 0);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

CLI (no_ip_nat_pool,
     no_ip_nat_pool_cmd,
     "no ip nat pool WORD A.B.C.D A.B.C.D A.B.C.D",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Pool",
     "Name of the IP address pool",
     "Start IP address of pool (A.B.C.D)",
     "End IP address of pool (A.B.C.D)",
     "Netmask")
{
  int ret;

  ret = imi_nat_address_pool (argv[0], argv[1], argv[2], argv[3], 0);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

CLI (show_ip_nat_pool_by_name,
     show_ip_nat_pool_by_name_cmd,
     "show ip nat pool WORD",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Pool",
     "Name of the IP address pool")
{
  display_nat_pool (cli, argv[0]);
  return CLI_SUCCESS;
}

CLI (show_ip_nat_pool,
     show_ip_nat_pool_cmd,
     "show ip nat pool",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Pool")
{
  display_nat_pool (cli, NULL);
  return CLI_SUCCESS;
}

CLI (clear_ip_nat_trans_all,
     clear_ip_nat_trans_all_cmd,
     "clear ip nat translations *",
     CLI_CLEAR_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Translation",
     "Clear all translations")
{
  imi_nat_clear_translations ();
  return CLI_SUCCESS;
}

CLI (ip_nat_translation_timeout,
     ip_nat_translation_timeout_cmd,
     "ip nat translations time-out <0-536870>",
     CLI_IP_STR,
     CLI_NAT_STR,
     "Translations",
     "Generic time-out for nat translations",
     "Specify timeout value for translations in seconds")
{
  int nat_timeout = 0;

  CLI_GET_INTEGER_RANGE ("timeout", nat_timeout, argv[0], 0, 536870);

  imi_nat_translation_timeout_set(nat_timeout);

  return CLI_SUCCESS;
}

CLI (no_ip_nat_translation_timeout,
     no_ip_nat_translation_timeout_cmd,
     "no ip nat translations time-out",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Translations",
     "Generic time-out for nat translations")
{
  imi_nat_translation_timeout_unset();

  return CLI_SUCCESS;
}

CLI (ip_nat_translation_icmp_timeout,
     ip_nat_translation_icmp_timeout_cmd,
     "ip nat translations icmp-timeout <0-536870>",
     CLI_IP_STR,
     CLI_NAT_STR,
     "Translations",
     "ICMP timeout for nat translations",
     "Specify timeout for NAT ICMP flows in seconds")
{
  int icmp_timeout = 0;

  CLI_GET_INTEGER_RANGE ("timeout", icmp_timeout, argv[0], 0, 536870);

  imi_nat_translation_icmp_timeout_set(icmp_timeout);

  return CLI_SUCCESS;
}

CLI (no_ip_nat_translation_icmp_timeout,
     no_ip_nat_translation_icmp_timeout_cmd,
     "no ip nat translations icmp-timeout",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Translations",
     "ICMP timeout for nat translations")
{

  imi_nat_translation_icmp_timeout_unset();

  return CLI_SUCCESS;
}

CLI (ip_nat_translation_tcp_timeout,
     ip_nat_translation_tcp_timeout_cmd,
     "ip nat translations tcp-timeout <0-536870>",
     CLI_IP_STR,
     CLI_NAT_STR,
     "Translations",
     "TCP timeout for translations",
     "Specify timeout for NAT TCP flows in seconds")
{
  int tcp_timeout = 0;

  CLI_GET_INTEGER_RANGE ("timeout", tcp_timeout, argv[0], 0, 536870);

  imi_nat_translation_tcp_timeout_set(tcp_timeout);

  return CLI_SUCCESS;
}

CLI (no_ip_nat_translation_tcp_timeout,
     no_ip_nat_translation_tcp_timeout_cmd,
     "no ip nat translations tcp-timeout",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Translations",
     "TCP timeout for translations")
{

  imi_nat_translation_tcp_timeout_unset();

  return CLI_SUCCESS;
}

CLI (ip_nat_translation_tcp_fin_timeout,
     ip_nat_translation_tcp_fin_timeout_cmd,
     "ip nat translations tcp-fin-timeout <0-536870>",
     CLI_IP_STR,
     CLI_NAT_STR,
     "Translations",
     "TCP FIN timeout for translations",
     "Specify timeout for NAT TCP flows in seconds")
{
  int tcp_fin_timeout = 0;

  CLI_GET_INTEGER_RANGE ("timeout", tcp_fin_timeout, argv[0], 0, 536870);

  imi_nat_translation_tcp_fin_timeout_set(tcp_fin_timeout);

  return CLI_SUCCESS;
}

CLI (no_ip_nat_translation_tcp_fin_timeout,
     no_ip_nat_translation_tcp_fin_timeout_cmd,
     "no ip nat translations tcp-fin-timeout",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Translations",
     "TCP FIN timeout for translations")
{

  imi_nat_translation_tcp_fin_timeout_unset();

  return CLI_SUCCESS;
}

CLI (ip_nat_translation_udp_timeout,
     ip_nat_translation_udp_timeout_cmd,
     "ip nat translations udp-timeout <0-536870>",
     CLI_IP_STR,
     CLI_NAT_STR,
     "Translations",
     "UDP timeout for translations",
     "Specify timeout for NAT UDP flows in seconds")
{
  int udp_timeout = 0;

  CLI_GET_INTEGER_RANGE ("timeout", udp_timeout, argv[0], 0, 536870);

  imi_nat_translation_udp_timeout_set(udp_timeout);

  return CLI_SUCCESS;
}

CLI (no_ip_nat_translation_udp_timeout,
     no_ip_nat_translation_udp_timeout_cmd,
     "no ip nat translations udp-timeout",
     CLI_NO_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Translations",
     "UDP timeout for translations")
{

  imi_nat_translation_udp_timeout_unset();

  return CLI_SUCCESS;
}

CLI (show_ip_nat_translation_timeout,
     show_ip_nat_translation_timeout_cmd,
     "show ip nat translations timeouts",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_NAT_STR,
     "Translations",
     "time-outs for NAT Translations")
{
  struct imi_nat_timeouts timeout;

  pal_mem_set(&timeout, 0, sizeof( struct imi_nat_timeouts));

  imi_nat_show_translation_timeout_all(cli, &timeout);

  return CLI_SUCCESS;
}

void
imi_nat_cli_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  /* NAT pool commands. */
  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_nat_pool_cmd);
  cli_set_imi_cmd (&ip_nat_pool_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_pool_by_name_cmd);
  cli_set_imi_cmd (&no_ip_nat_pool_by_name_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_pool_cmd);
  cli_set_imi_cmd (&no_ip_nat_pool_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ip_nat_pool_by_name_cmd);
  cli_set_imi_cmd (&show_ip_nat_pool_by_name_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ip_nat_pool_cmd);
  cli_set_imi_cmd (&show_ip_nat_pool_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  /* NAT commands. */
  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_nat_inside_src_static_cmd);
  cli_set_imi_cmd (&ip_nat_inside_src_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_inside_src_static_cmd);
  cli_set_imi_cmd (&no_ip_nat_inside_src_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_nat_inside_src_access_list_pool_static_cmd);
  cli_set_imi_cmd (&ip_nat_inside_src_access_list_pool_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_inside_src_access_list_pool_static_cmd);
  cli_set_imi_cmd (&no_ip_nat_inside_src_access_list_pool_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);


  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_nat_inside_dst_static_cmd);
  cli_set_imi_cmd (&ip_nat_inside_dst_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_inside_dst_static_cmd);
  cli_set_imi_cmd (&no_ip_nat_inside_dst_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_nat_inside_dst_access_list_pool_static_cmd);
  cli_set_imi_cmd (&ip_nat_inside_dst_access_list_pool_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_inside_dst_access_list_pool_static_cmd);
  cli_set_imi_cmd (&no_ip_nat_inside_dst_access_list_pool_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);


  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_nat_outside_src_static_cmd);
  cli_set_imi_cmd (&ip_nat_outside_src_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_outside_src_static_cmd);
  cli_set_imi_cmd (&no_ip_nat_outside_src_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_nat_outside_src_access_list_pool_static_cmd);
  cli_set_imi_cmd (&ip_nat_outside_src_access_list_pool_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_outside_src_access_list_pool_static_cmd);
  cli_set_imi_cmd (&no_ip_nat_outside_src_access_list_pool_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);


  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_nat_outside_dst_static_cmd);
  cli_set_imi_cmd (&ip_nat_outside_dst_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_outside_dst_static_cmd);
  cli_set_imi_cmd (&no_ip_nat_outside_dst_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_nat_outside_dst_static_port_cmd);
  cli_set_imi_cmd (&ip_nat_outside_dst_static_port_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_outside_dst_static_port_cmd);
  cli_set_imi_cmd (&no_ip_nat_outside_dst_static_port_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_nat_outside_dst_access_list_pool_static_cmd);
  cli_set_imi_cmd (&ip_nat_outside_dst_access_list_pool_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_outside_dst_access_list_pool_static_cmd);
  cli_set_imi_cmd (&no_ip_nat_outside_dst_access_list_pool_static_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_nat_translation_timeout_cmd);
  cli_set_imi_cmd (&ip_nat_translation_timeout_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_translation_timeout_cmd);
  cli_set_imi_cmd (&no_ip_nat_translation_timeout_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_nat_translation_tcp_timeout_cmd);
  cli_set_imi_cmd (&ip_nat_translation_tcp_timeout_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_translation_tcp_timeout_cmd);
  cli_set_imi_cmd (&no_ip_nat_translation_tcp_timeout_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_nat_translation_udp_timeout_cmd);
  cli_set_imi_cmd (&ip_nat_translation_udp_timeout_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_translation_udp_timeout_cmd);
  cli_set_imi_cmd (&no_ip_nat_translation_udp_timeout_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_nat_translation_icmp_timeout_cmd);
  cli_set_imi_cmd (&ip_nat_translation_icmp_timeout_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_translation_icmp_timeout_cmd);
  cli_set_imi_cmd (&no_ip_nat_translation_icmp_timeout_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_nat_translation_tcp_fin_timeout_cmd);
  cli_set_imi_cmd (&ip_nat_translation_tcp_fin_timeout_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_nat_translation_tcp_fin_timeout_cmd);
  cli_set_imi_cmd (&no_ip_nat_translation_tcp_fin_timeout_cmd, CONFIG_MODE, CFG_DTYP_IMI_NAT);

  /* Show commands. */
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ip_nat_trans_cmd);
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ip_nat_inside_src_trans_cmd);
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ip_nat_inside_dst_trans_cmd);
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ip_nat_outside_src_trans_cmd);
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ip_nat_outside_dst_trans_cmd);
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ip_nat_translation_timeout_cmd);

  /* Clear NAT rules commands. */
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &clear_ip_nat_trans_all_cmd);
}

/* Initialize firewall configuration.  */
void
imi_nat_init (struct lib_globals *zg)
{
  /* Actually this function does nothing on Linux.  */
  pal_fw_start (zg);

  /* Initialize global variables. */
  imi_nat_address_pool_list = list_new ();

  /* Clear all NAT translations. */
  pal_nat_clear_all_translations ();

  imi_nat_cli_init (zg);

  /* Initialize virtual-server feature.  */
  imi_virtual_server_init (zg);
}

void
imi_nat_finish (struct lib_globals *zg)
{
  /* Finish virtual-server feature.  */
  imi_virtual_server_finish (zg);

  /* Free NAT address pool list. */
  list_free (imi_nat_address_pool_list);

  /* Clear all NAT translations. */
  pal_nat_clear_all_translations ();

  /* Clear all NAT timeouts. */
  pal_nat_clear_all_timeouts ();
}

void
imi_nat_translation_timeout_set(int timeout)
{

  pal_nat_translation_timeout_set(timeout);

  SET_FLAG(nat_time_out.check_flag, IMI_GENERIC_TIMEOUT);
  nat_time_out.generic_timeout = timeout;

}

void
imi_nat_translation_timeout_unset()
{

  if (CHECK_FLAG(nat_time_out.check_flag, IMI_GENERIC_TIMEOUT))
    {
      pal_nat_translation_timeout_set(DEFAULT_GENERIC_TIMEOUT);
      nat_time_out.generic_timeout = DEFAULT_GENERIC_TIMEOUT;
      UNSET_FLAG(nat_time_out.check_flag, IMI_GENERIC_TIMEOUT);
    }

}

void
imi_nat_translation_icmp_timeout_set(int timeout)
{

  pal_nat_translation_icmp_timeout_set(timeout);

  SET_FLAG(nat_time_out.check_flag, IMI_ICMP_TIMEOUT);
  nat_time_out.icmp_timeout = timeout;

}

void
imi_nat_translation_icmp_timeout_unset()
{

  if (CHECK_FLAG(nat_time_out.check_flag, IMI_ICMP_TIMEOUT))
    {
      pal_nat_translation_icmp_timeout_set(DEFAULT_ICMP_TIMEOUT);

      nat_time_out.icmp_timeout = DEFAULT_ICMP_TIMEOUT;
      UNSET_FLAG(nat_time_out.check_flag, IMI_ICMP_TIMEOUT);
    }

}

void
imi_nat_translation_udp_timeout_set(int timeout)
{

  pal_nat_translation_udp_timeout_set(timeout);

  SET_FLAG(nat_time_out.check_flag, IMI_UDP_TIMEOUT);
  nat_time_out.udp_timeout = timeout;

}

void
imi_nat_translation_udp_timeout_unset()
{

  if (CHECK_FLAG(nat_time_out.check_flag, IMI_UDP_TIMEOUT))
    {
      pal_nat_translation_udp_timeout_set(DEFAULT_UDP_TIMEOUT);

      nat_time_out.udp_timeout = DEFAULT_UDP_TIMEOUT;
      UNSET_FLAG(nat_time_out.check_flag, IMI_UDP_TIMEOUT);
    }

}

void
imi_nat_translation_tcp_timeout_set(int timeout)
{
  pal_nat_translation_tcp_timeout_set(timeout);

  SET_FLAG(nat_time_out.check_flag, IMI_TCP_TIMEOUT);
  nat_time_out.tcp_timeout = timeout;

}

void
imi_nat_translation_tcp_timeout_unset()
{

  if (CHECK_FLAG(nat_time_out.check_flag, IMI_TCP_TIMEOUT))
    {
      pal_nat_translation_tcp_timeout_set(DEFAULT_TCP_TIMEOUT);

      UNSET_FLAG(nat_time_out.check_flag, IMI_TCP_TIMEOUT);
      nat_time_out.tcp_timeout = DEFAULT_TCP_TIMEOUT;
    }

}

void
imi_nat_translation_tcp_fin_timeout_set(int timeout)
{

  pal_nat_translation_tcp_fin_timeout_set(timeout);

  SET_FLAG(nat_time_out.check_flag, IMI_TCP_FIN_TIMEOUT);
  nat_time_out.tcp_fin_timeout = timeout;

}

void
imi_nat_translation_tcp_fin_timeout_unset()
{
  if (CHECK_FLAG(nat_time_out.check_flag, IMI_TCP_FIN_TIMEOUT))
    {
      pal_nat_translation_tcp_fin_timeout_set(DEFAULT_TCP_FIN_TIMEOUT);

      UNSET_FLAG(nat_time_out.check_flag, IMI_TCP_FIN_TIMEOUT);
      nat_time_out.tcp_fin_timeout = DEFAULT_TCP_FIN_TIMEOUT;
    }
}

#endif /* HAVE_NAT */
