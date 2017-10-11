/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#include "lib.h"
#include "cli.h"
#include "filter.h"
#include "linklist.h"
#include "prefix.h"
#include "snprintf.h"
#include "vrep.h"

#include "imi/pal_fw.h"

#include "imi/imi.h"
#include "imi_filter.h"
#include "imi_fw.h"

#define IMI_FILTER_CLI_VREP_NUM_COLS 9

extern char *filter_prot_str(s_int16_t proto, char *str);


/* Operators. */
const char *operator_str[]=
{
  "",        /* NOOP */
  "=",       /* EQUAL */
  "!=",      /* NOT_EQUAL */
  "<",       /* LESS_THAN */
  ">",       /* GREATHAR_THAN */
  "-"        /* RANGE */
};

static const char *
imi_path_str (int path)
{
  switch (path)
    {
    case IMI_FILTER_INPUT:
      return "in";
    case IMI_FILTER_OUTPUT:
      return "out";
    case IMI_FILTER_FORWARD:
      return "forward";
    }
  return "";
}

static int
get_path_from_str (char *str)
{
  if (! pal_strncmp (str, "i", 1))
    return IMI_FILTER_INPUT;
  if (! pal_strncmp (str, "o", 1))
    return IMI_FILTER_OUTPUT;
  if (! pal_strncmp (str, "f", 1))
    return IMI_FILTER_FORWARD;
  return -1;
}

static struct listnode *
find_filter_acl_rule (struct list *rule_list, struct imi_rule *rule)
{
  struct listnode *node;
  struct imi_rule *r;

  LIST_LOOP (rule_list, r, node)
    {
      if (!strcmp (r->u.arule.acl, rule->u.arule.acl) &&
          r->path == rule->path)
        return node;
    }
  return NULL;
}

/* Delete the node corresponding to acl num from the list.
 */
void
imi_filter_chain_list_del_acl_num (struct list *acl_num_list, u_int32_t acl_num)
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

/* If the acl name is numeric then add it to the filter chain list
 * in sorted order of acl num and return the position of the node in the list.
 */
u_int16_t
imi_filter_chain_add_acl_num (int path, char *acl_name)
{
  u_int32_t acl_num;
  u_int32_t *acl_num_elem = NULL;
  struct imi_filter_chain *chain = NULL;
  u_int16_t rulenum = 0;
  char temp [20];

  if (pal_sscanf (acl_name, "%u%1s", (unsigned *)&acl_num, temp) == 1)
    {
      chain = &filter_chain_tbl [path];
      acl_num_elem = XCALLOC (MTYPE_ACCESS_LIST_STR, sizeof (u_int32_t));
      /* Append in the worst case. */
      if (! acl_num_elem)
        return 0;
      *acl_num_elem = acl_num;

      /* The position of the node in the list, after sorting it based on
         the acl list name gives the rulenum to be inserted.
      */
      rulenum = listnode_add_sort_index (&chain->acl_num_list, acl_num_elem);
      return rulenum;
    }

  /* Not numbered access list - will append. */
  return 0;
}

/* If the acl name is numeric then remove it from the filter chain list. */
void
imi_filter_chain_del_acl_num (int path, char *acl_name)
{
  u_int32_t acl_num;
  struct imi_filter_chain *chain = NULL;
  char temp [20];

  if (pal_sscanf (acl_name, "%u%1s", (unsigned *)&acl_num, temp) == 1)
    {
      chain = &filter_chain_tbl [path];

      /* Delete the node from the list based on the acl_num.
       */
      imi_filter_chain_list_del_acl_num (&chain->acl_num_list, acl_num);
    }
}

/* If op = 1, add the acl name to the filter chain list else delete. */
u_int16_t
imi_filter_chain_update_acl_num (int path, char *acl_name, int op)
{
  u_int16_t rulenum = 0;

  if (op)
    rulenum = imi_filter_chain_add_acl_num (path, acl_name);
  else
    imi_filter_chain_del_acl_num (path, acl_name);

  return rulenum;
}

int
imi_filter_translate_acl (struct interface *ifp, struct access_list *acl,
                          struct imi_rule *rule, int opcode)
{
  struct filter_list *node;
  struct filter_pacos_ext *acl_filter;
  u_int16_t rulenum = 0;
  result_t res = LIB_API_SET_SUCCESS;

  for (node = acl->head; node; node = node->next)
    {
     /* Check if this filter is a PacOS extended one. */
      if (node->common != FILTER_PACOS_EXT)
        return LIB_API_SET_ERROR;

      acl_filter = &node->u.zextfilter;

     /* Add/delete the rulenum - 2 entries if log included. */
      if (acl_filter->log)
        rulenum = imi_filter_chain_update_acl_num (rule->path,acl->name, opcode);
      rulenum = imi_filter_chain_update_acl_num (rule->path, acl->name, opcode);
      res = pal_fw_translate_rule (opcode, rule->path, acl_filter->protocol,
                                   &acl_filter->sprefix,
                                   &acl_filter->dprefix,
                                   acl_filter->sport_lo, acl_filter->sport,
                                   acl_filter->sport_op,
                                   acl_filter->dport_lo, acl_filter->dport,
                                   acl_filter->dport_op,
                                   ifp->name,
                                   node->type,
                                   rulenum, /* The DENY/PERMIT rule number. */
                                   acl_filter->icmp_type,
                                   acl_filter->established,
                                   acl_filter->pkt_size_lo, acl_filter->pkt_size,
                                   acl_filter->pkt_size_op,
                                   acl_filter->tos_op == NOOP ? acl_filter->tos : -1,
                                   acl_filter->fragments,
                                   acl_filter->log);
      if (! res)
       rule->filter_ref_cnt += opcode ? 1 : -1;
      else
        break;
    }
  return (res <= 0 ? res : LIB_API_SET_ERR_BAD_KERNEL_RULE);
}
/* Executed at the time of binding/unbinding interface from VR.
   There is no one to return any error code to.
   It returns on the first error detected.
*/
void
imi_filter_process_if (struct interface *ifp, int opcode)
{
  struct access_list *acl;
  struct imi_interface *imi_if;
  struct listnode *rnode;
  struct imi_rule *rule;
  struct apn_vr *vr = apn_vr_get_privileged (imim);
  result_t res = LIB_API_SET_SUCCESS;
  /* Get IMI interface with the list of rules. */
  imi_if = ifp->info;
  if (!imi_if)
    return ;
  for (rnode = LISTHEAD (imi_if->rule_list); rnode; NEXTNODE (rnode))
    {
      rule = GETDATA (rnode);
      /* If reference count != 0 => rule already installed. */
      if (rule->filter_ref_cnt && opcode)
        return;
      /* Lookup ACL. */
      acl = access_list_lookup (vr, AFI_IP, rule->u.arule.acl);
      if (acl)
        {
          res = imi_filter_translate_acl (ifp, acl, rule, opcode);
          if (res)
            /* No mercy in case the rule installation fails. */
            return;
        }
    }
}
#ifdef HAVE_ACL
/*--------------------------------------------------------------------
 * imi_filter_acl_ntf_cb - Walk all filter rules and process these
 *                         which have this access-list referenced.
 * NOTE: If rule is marked as installed, uninstall it and install again.
 *       If rule is not instlled, intall it only.
 */
result_t
imi_filter_acl_ntf_cb (struct access_list *access,
                       struct filter_list *filter,
                       filter_opcode_t     opcode)
{
  struct interface *ifp;
  struct imi_interface *imi_if;
  struct listnode *node, *node1;
  struct imi_rule *rule;
  struct filter_pacos_ext *acl_filter;
  struct apn_vr *vr = apn_vr_get_privileged (imim);
  u_int16_t rulenum = 0;
  result_t res = LIB_API_SET_SUCCESS;
  /* XXX should detect access-list is invalid. */
  if (filter == NULL)
    return LIB_API_SET_ERROR;
  /* Check if this filter is a PacOS extended one. */
  if (filter->common != FILTER_PACOS_EXT)
    return LIB_API_SET_SUCCESS;
  /* Handle ACL filter addition to the filter list. */
  for (node = LISTHEAD (vr->ifm.if_list); node; NEXTNODE (node))
    {
      ifp = GETDATA (node);
      /* Get IMI interface with the list of rules. */
      imi_if = ifp->info;
      if (!imi_if)
        continue;
      for (node1 = LISTHEAD (imi_if->rule_list); node1; NEXTNODE (node1))
        {
          rule = GETDATA (node1);
          if (!pal_strcmp (rule->u.arule.acl, access->name))
            {
              acl_filter = &filter->u.zextfilter;
              /* Get the rulenum - 2 entries if log included. */
              if (acl_filter->log)
                rulenum = imi_filter_chain_update_acl_num (rule->path,
                                                           access->name,
                                                           opcode);
              rulenum = imi_filter_chain_update_acl_num (rule->path,
                                                         access->name, opcode);
          /* rulenum > 0 => insert; append otherwise */
         res = pal_fw_translate_rule (opcode, rule->path, acl_filter->protocol,
                                      &acl_filter->sprefix,
                                      &acl_filter->dprefix,
                                      acl_filter->sport_lo, acl_filter->sport,
                                      acl_filter->sport_op,
                                      acl_filter->dport_lo, acl_filter->dport,
                                      acl_filter->dport_op,
                                      ifp->name,
                                      filter->type,
                                      rulenum, 
                                      acl_filter->icmp_type,
                                      acl_filter->established,
                                      acl_filter->pkt_size_lo, acl_filter->pkt_size,
                                      acl_filter->pkt_size_op,
                                      acl_filter->tos_op == NOOP ? acl_filter->tos : -1,
                                      acl_filter->fragments,
                                      acl_filter->log);
      
            if (! res)
               rule->filter_ref_cnt += opcode ? 1 : -1;
            else
               goto imi_filter_acl_ntf_cb_exit;
            }
        }
    }
 imi_filter_acl_ntf_cb_exit:
  return (res <= 0 ? res : LIB_API_SET_ERR_BAD_KERNEL_RULE);
}
#endif /* HAVE_ACL */

int
imi_filter_acl (u_int32_t vr_id, char *acl_name,
                char *path_str, char *ifname, int op)
{
  struct access_list *acl;
  int path;
  struct interface *ifp;
  struct imi_interface *imi_if;
  struct apn_vr *vr;
  struct imi_rule *rule;
  struct listnode *node;
  result_t res = 0;
  
  if (vr_id != APN_PVR_ID)
    return -1;

  vr = apn_vr_get_privileged(imim);
  if (vr == NULL)
    return -1;

  /* Lookup interface - it must be there */
  ifp = ifg_get_by_name (&imim->ifg, ifname);
  if (!ifp)
    return -1;

  /* Get IMI interface with the list of rules. */
  imi_if = ifp->info;
  if (!imi_if)
    return -1;

  path = get_path_from_str (path_str);
  if (path == -1)
    return -1;

  /* Lookup ACL. */
  acl = access_list_lookup (vr, AFI_IP, acl_name);

  /* If it does not exist, we will save the rule, if it is not
     saved yet.
   */
  if (! acl)
    {
  if (op)
    {
      rule = XCALLOC (MTYPE_IMI_RULE, sizeof (struct imi_rule));
      rule->path = path;
      pal_strcpy (rule->u.arule.acl, acl_name);
      rule->flag = IMI_RULE_ACL;

      /* Find if the ACL rule exists. If yes, we are done here.
       */
      node = find_filter_acl_rule (imi_if->rule_list, rule);
      if (node)
        {
           XFREE(MTYPE_IMI_RULE, rule);
        }
      else
        /* Addition of new rule. */
        listnode_add (imi_if->rule_list, rule);
         return 0;
        }
    }
 /* Access-list is configured: add a new rule or delete an old one. */
  if (op)
    {
      rule = XCALLOC (MTYPE_IMI_RULE, sizeof (struct imi_rule));
      rule->path = path;
      pal_strcpy (rule->u.arule.acl, acl_name);
      rule->flag = IMI_RULE_ACL;

       /* Find if the ACL rule first.*/
      node = find_filter_acl_rule (imi_if->rule_list, rule);
      if (node)
        {
          /* Rule found. We have a duplicate here. Return success. */
          XFREE(MTYPE_IMI_RULE, rule);
          return LIB_API_SET_ERR_OBJECT_ALREADY_EXIST;
        }
      else
        { /* New rule: Install it in the kernel and save it as well. */
          res = imi_filter_translate_acl (ifp, acl, rule,  op);
          if (! res)
            listnode_add (imi_if->rule_list, rule);
          else
            { /* Delete any installed filters. */
              imi_filter_translate_acl (ifp, acl, rule,  0);
              XFREE(MTYPE_IMI_RULE, rule);
              return res;
            }
        }
    }
  else
    { /* Rule deletion. Fill in the search key. */
      struct imi_rule krule;
     pal_strcpy (krule.u.arule.acl, acl_name);
      krule.path = path;
      krule.flag = IMI_RULE_ACL;
     node = find_filter_acl_rule (imi_if->rule_list, &krule);
      if (node)
        {
          rule = GETDATA(node);
            list_delete_node (imi_if->rule_list, node);
          /* Translate the rule and remove it from the kernel. */
          if (acl)
            res = imi_filter_translate_acl (ifp, acl, rule, 0);
 
          XFREE(MTYPE_IMI_RULE, rule);
        }
      else
        return LIB_API_SET_ERR_UNKNOWN_OBJECT;
    }
  return res;
}

CLI (ip_access_group_acl,
     ip_access_group_acl_cmd,
     "ip access-group WORD (in|out|forward)",
     CLI_IP_STR,
     "Access group",
     "Access List name",
     "Incoming packets",
     "Outgoing packets",
     "Forwarded packets")
{
  struct interface *ifp = cli->index;
  int ret;

  ret = imi_filter_acl (cli->vr->id, argv[0], argv[1], ifp->name, 1);
  if (!ret)
    return CLI_SUCCESS;
  else if (ret == LIB_API_SET_ERR_BAD_KERNEL_RULE)
    cli_out (cli, "%% Failure to install kernel rule\n");
  else if (ret == LIB_API_SET_ERR_OBJECT_ALREADY_EXIST)
    cli_out (cli, "%% Access group already exists\n");
   else
    cli_out (cli, "%% Error during access group addition\n");
    return CLI_ERROR;
} 

CLI (no_ip_access_group_acl,
     no_ip_access_group_acl_cmd,
     "no ip access-group WORD (in|out|forward)",
     CLI_NO_STR,
     CLI_IP_STR,
     "Access group",
     "Access List name",
     "Incoming packets",
     "Outgoing packets",
     "Forwarded packets")
{
  struct interface *ifp = cli->index;
  int ret;

  ret = imi_filter_acl (cli->vr->id, argv[0], argv[1], ifp->name, 0);
  if (!ret)
    return CLI_SUCCESS;
  else if (ret == LIB_API_SET_ERR_BAD_KERNEL_RULE)
    cli_out (cli, "%% Failure to uninstall a kernel rule\n");
  else if (ret == LIB_API_SET_ERR_UNKNOWN_OBJECT)
    cli_out (cli, "%% No such access group\n");
  else
      cli_out (cli, "%% Error during access group deletion\n");
      return CLI_ERROR;
}

void
imi_filter_show_acl_translations (struct vrep_table *vrep, 
                                  struct interface *ifp,
                                  struct imi_rule *rule)
{
  struct apn_vr *vr = apn_vr_get_privileged (imim);
  struct access_list *acl;
  struct filter_list *filter;
  char sport_str[PORT_STR_LEN], dport_str[PORT_STR_LEN];
  enum operation sport_op, dport_op, tos_op, length_op;
  u_int16_t sport, dport, sport_lo, dport_lo;
  int tos, length_lo, length;
  bool_t fragm, estab, log;
  char sstr[PREFIX_AM_STR_SIZE+1], dstr[PREFIX_AM_STR_SIZE+1];
  char tos_str[OPT_STR_LEN], icmp_type_str[OPT_STR_LEN], length_str[OPT_STR_LEN];
  char proto_str[OPT_STR_LEN];
  struct prefix_am *sprefix, *dprefix;
  s_int16_t proto;
  s_int16_t icmp_type;

  if (vr == NULL)
    return;

  acl = access_list_lookup (vr, AFI_IP, rule->u.arule.acl);
  if (!acl)
    return;

  for (filter = acl->head; filter; filter = filter->next)
    {
      if (filter->common != FILTER_PACOS_EXT)
        continue;

      proto = filter->u.zextfilter.protocol;

      sport_op = filter->u.zextfilter.sport_op;
      sport    = filter->u.zextfilter.sport;
      sport_lo = filter->u.zextfilter.sport_lo;
      if (sport_op == RANGE)
        pal_snprintf (sport_str, PORT_STR_LEN, "%d-%d", sport_lo, sport);
      else if (sport_op != NOOP )
        pal_snprintf (sport_str, PORT_STR_LEN, "%s%d",
                      operator_str[sport_op], sport);
      else
        pal_strcpy (sport_str, "all");

      dport_op = filter->u.zextfilter.dport_op;
      dport    = filter->u.zextfilter.dport;
      dport_lo = filter->u.zextfilter.dport_lo;
      if (dport_op == RANGE)
        pal_snprintf (dport_str, PORT_STR_LEN, "%d-%d", dport_lo, dport);
      else if (dport_op != NOOP)
        pal_snprintf (dport_str, PORT_STR_LEN, "%s%d",
                      operator_str[dport_op], dport);
      else
        pal_strcpy (dport_str, "all");

      sprefix = &filter->u.zextfilter.sprefix;
      if (prefix_am_incl_all(sprefix))
        pal_strcpy (sstr, "all");
      else
        prefix2str_am(sprefix, sstr, PREFIX_AM_STR_SIZE, '/');

      dprefix = &filter->u.zextfilter.dprefix;
      if (prefix_am_incl_all(dprefix))
        pal_strcpy (dstr, "all");
      else
       prefix2str_am (dprefix, dstr, PREFIX_AM_STR_SIZE, '/');
      pal_strcpy(icmp_type_str, "");
      if (proto == ICMP_PROTO)
        {
          icmp_type = filter->u.zextfilter.icmp_type;
          if (icmp_type >= 0 )
            pal_snprintf (icmp_type_str, OPT_STR_LEN, "icmp:%d ", icmp_type);
          else
            pal_strcpy(icmp_type_str, "");
        }
      length_op = filter->u.zextfilter.pkt_size_op;
      length_lo = filter->u.zextfilter.pkt_size_lo;
      length    = filter->u.zextfilter.pkt_size;
      if (length_op == RANGE)
        pal_snprintf (length_str, OPT_STR_LEN, " len:%d-%d ", length_lo, length);
      else if (length_op == LESS_THAN)
        pal_snprintf (length_str, OPT_STR_LEN, " len:0-%d ", length-1);
      else if (length_op == GREATER_THAN)
        pal_snprintf (length_str, OPT_STR_LEN, " len:%d-65535 ", length+1);
      else
        pal_strcpy(length_str, "");

      estab = filter->u.zextfilter.established;
      fragm = filter->u.zextfilter.fragments;
      log   = filter->u.zextfilter.log;

      /* With tos we do not support range here. */
      tos_op= filter->u.zextfilter.tos_op;
      tos   = filter->u.zextfilter.tos;
      if (tos_op == NOOP)
        {
         if (tos >= 0)
            pal_snprintf (tos_str, OPT_STR_LEN, "tos:%d ", tos);
         else
            pal_strcpy (tos_str, "");
       }
      vrep_add_next_row (vrep, NULL,
                         "%s(%s) \t %s \t %s \t %s \t %s \t %s \t %s \t %s \t %s%s%s%s%s%s",
               ifp->name, imi_path_str (rule->path),
               acl->name,
                         (filter->type ? "permit" : "deny"),
                         filter_prot_str(proto, proto_str),
               sstr,
               sport_str,
               dstr,
                         dport_str,
                         icmp_type_str,
                         length_str,
                         tos_str,
                         estab ? "ESTAB ":"",
                         fragm ? "FRAG " : "",
                         log ? "LOG" : "");
    }

}

s_int16_t
imi_filter_vrep_show_cb (intptr_t usr_ref, char *str)
{
  struct cli *cli = (struct cli *)usr_ref;
  if (! cli)
    return VREP_ERROR;
  cli_out (cli, "%s", str);
  return VREP_SUCCESS;
}

int
imi_filter_show_translations (struct cli *cli)
{
  struct listnode *node, *node1;
  struct interface *ifp;
  struct imi_interface *imi_if;
  struct imi_rule *r;
  struct vrep_table *vrep = vrep_create (IMI_FILTER_CLI_VREP_NUM_COLS,
                                         VREP_MAX_ROW_WIDTH);
  if (vrep == NULL) {
    cli_out(cli, "%% Cannot initiate report.");
    return CLI_ERROR;
  }
  /* Two rows header. */
  vrep_add (vrep, 0, 0,
            " Interface \t Access \t Permit \t Proto \t Source \t Source \t Dest. \t Dest. \t Options ");
  vrep_add (vrep, 1, 0,
            " (Path) \t list \t /Deny \t  \t address \t port \t address \t port \t \t");
  vrep_add (vrep, 2, 0, "");

  LIST_LOOP (cli->vr->ifm.if_list, ifp, node)
    {
      imi_if = ifp->info;

      LIST_LOOP (imi_if->rule_list, r, node1)
        {
          switch (r->flag)
            {
            case IMI_RULE_ACL:
              imi_filter_show_acl_translations (vrep, ifp, r);
              break;
            case IMI_RULE_STATIC:
              break;
            }
        }
    }
  vrep_add_next_row (vrep, NULL,"");
  vrep_show (vrep, imi_filter_vrep_show_cb, (intptr_t)cli);
  vrep_delete (vrep);
  return CLI_SUCCESS;
}

CLI (show_ip_filter_trans,
     show_ip_filter_trans_cmd,
     "show ip filter translations",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_FILTER_STR,
     "translations")
{
  return imi_filter_show_translations (cli);
}

int
imi_acl_if_config_write (struct cli *cli, struct interface *ifp)
{
  struct imi_interface *imi_if;
  struct imi_rule *rule;
  struct listnode *node;

  imi_if = ifp->info;
  if (imi_if == NULL)
    return -1;

  LIST_LOOP (imi_if->rule_list, rule, node)
    cli_out (cli, " ip access-group %s %s\n",
             rule->u.arule.acl, imi_path_str (rule->path));

  return 0;
}

int
imi_access_list_delete (char *name)
{
  struct access_list *acl;
  struct apn_vr *vr = apn_vr_get_privileged (imim);

  acl = access_list_lookup (vr, AFI_IP, name);
  if (!acl)
    return RESULT_OK;

  access_list_delete (acl);

  return RESULT_OK;
}

void
imi_acl_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

#ifdef HAVE_IPV6
  /*  access_list_init_ipv6_extended (zg);*/
#endif /* HAVE_IPV6 */

  cli_install_imi (ctree, INTERFACE_MODE, PM_IMI, PRIVILEGE_PVR_MAX, 0,
                   &ip_access_group_acl_cmd);
  cli_install_imi (ctree, INTERFACE_MODE, PM_IMI, PRIVILEGE_PVR_MAX, 0,
                   &no_ip_access_group_acl_cmd);

  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_PVR_MAX, 0,
                   &show_ip_filter_trans_cmd);

  /* Clear all Filter rules.  */
  pal_filter_clear_all_rules ();
}

void
imi_acl_finish (struct lib_globals *zg)
{
  /* Clear all Filter rules. */
  pal_filter_clear_all_rules ();
}

