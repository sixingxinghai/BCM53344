/* Copyright (C) 2002-2003  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "cli.h"
#include "show.h"
#include "snprintf.h"
#include "linklist.h"

#include "imi/imi.h"
#include "imi/imi_interface.h"

#ifdef HAVE_NAT
#include "imi_fw.h"
#endif


#ifdef HAVE_DHCP_CLIENT
#include "imi_dhcp.h"
#endif /* HAVE_DHCP_CLIENT. */

struct interface *
imi_if_create (char *ifname)
{
  struct interface *ifp;

  if (ifname == NULL)
    return NULL;

  ifp = ifg_get_by_name (&imim->ifg, ifname);

  return ifp;
}

const char *
get_if_scope_str (u_int8_t scope)
{
  switch (scope)
    {
    case NSM_IF_SCOPE_INSIDE:
      return "inside";
      break;
    case NSM_IF_SCOPE_OUTSIDE:
      return "outside";
      break;
    case NSM_IF_SCOPE_UNSPEC:
    default:
      return "both";
      break;
    }
  return "both";
}

int
imi_interface_scope_config_write (struct cli *cli, struct interface *ifp)
{
  struct imi_interface *imi_if = ifp->info;

  /* Set scope. */
  switch (imi_if->scope)
    {
#ifdef HAVE_NAT
    case NSM_IF_SCOPE_INSIDE:
      cli_out (cli, " ip nat inside\n");
      break;
    case NSM_IF_SCOPE_OUTSIDE:
      cli_out (cli, " ip nat outside\n");
      break;
#endif /* HAVE_NAT */
    case NSM_IF_SCOPE_UNSPEC:
    default:
      break;
    }

  return 0;
}

int
imi_interface_scope_show (struct cli *cli, struct interface *ifp)
{
  struct imi_interface *imi_if;

  if (ifp == NULL)
    return -1;

  imi_if  = ifp->info;
  cli_out (cli, "  Scope: %s\n", get_if_scope_str(imi_if->scope));

  return 0;
}

struct imi_interface *
imi_if_new (void)
{
  struct imi_interface *imi_if;

  imi_if = XCALLOC (MTYPE_TMP, sizeof (struct imi_interface));
  if (!imi_if)
    return NULL;

  imi_if->rule_list = list_new();
  imi_if->idc = NULL;

  return imi_if;
}

void
imi_if_free (struct imi_interface *imi_if)
{
#ifdef HAVE_NAT
  struct listnode *node;
  struct imi_rule *rdel;
#endif /* HAVE_NAT */

  /* NAT/FW free of imi_if->rule_list.  */
#ifdef HAVE_NAT
  LIST_LOOP (imi_if->rule_list, rdel, node)
    XFREE (MTYPE_IMI_RULE, rdel);
#endif /* HAVE_NAT */
  list_delete (imi_if->rule_list);

#ifdef HAVE_DHCP_CLIENT
  /* Free DHCP client structure.  */
  imi_dhcp_client_free (imi_if);
#endif /* HAVE_DHCP_CLIENT */

  XFREE (MTYPE_TMP, imi_if);
}

/* Called when interface structure allocated. */
int
imi_if_new_hook (struct interface *ifp)
{
  ifp->info = imi_if_new ();
  return 0;
}

/* Called when interface structure deleted. */
int
imi_if_delete_hook (struct interface *ifp)
{
  if (ifp->info != NULL)
    imi_if_free (ifp->info);

  ifp->info = NULL;

  return 0;
}

#ifdef HAVE_NAT

#define _IMI_IF_PROCES_RLIST(dir, scope, via_in, via_out)                    \
  if (LISTCOUNT((rlist = IMI_NAT_RLIST(dir, scope))))                       \
    {                                                                       \
      if (pool != NULL)                                                     \
        {                                                                   \
          LIST_LOOP (rlist, rule, node)                                     \
            {                                                               \
              if ( !strcmp(rule->u.arule.pool, pool->name) )                \
                {                                                           \
                  int res;                                                  \
                  acl = access_list_lookup (vr, AFI_IP, rule->u.arule.acl); \
                  if (!acl)                                                 \
                    continue;                                               \
                  res = imi_nat_process_acl (dir, scope, acl, pool,         \
                                             via_in, via_out, imi_if, op);  \
                  if (res != IMI_NAT_SUCCESS)                               \
                    return res;                                             \
                }                                                           \
            }                                                               \
        }                                                                   \
      else                                                                  \
        {                                                                   \
          LIST_LOOP (rlist, rule, node)                                     \
            {                                                               \
              /* If Pool is NULL Static Rule has to applied. */             \
              if (rule->flag == IMI_RULE_STATIC)                            \
                {                                                           \
                  int res;                                                  \
                  res = imi_nat_process_static (dir, scope, rule,           \
                                                via_in, via_out, imi_if, op);\
                  if (res != IMI_NAT_SUCCESS)                               \
                    return res;                                             \
                }                                                           \
            }                                                               \
        }                                                                   \
    }
/* Apply dynamic or static rule.
   Returns IMI_NAT_... result code.
*/
int
imi_if_apply_nat (struct apn_vr *vr, struct interface *ifp,
                  struct imi_nat_address_pool *pool, u_int8_t if_nscope,
                  u_int8_t if_oscope, int op)
{
  struct list *rlist = NULL;
  struct listnode *node = NULL;
  struct imi_rule *rule = NULL;
  struct access_list *acl = NULL;
  struct imi_interface *imi_if = ifp->info;

  if ( NSM_IF_SCOPE_OUTSIDE == if_nscope ||
       ( (!op) && (NSM_IF_SCOPE_OUTSIDE == if_oscope) ) )
    {
      _IMI_IF_PROCES_RLIST(IMI_NAT_INSIDE, IMI_NAT_SOURCE, NULL, ifp->name);
      _IMI_IF_PROCES_RLIST(IMI_NAT_OUTSIDE,IMI_NAT_DESTINATION, NULL, ifp->name);
    }
  else if ( NSM_IF_SCOPE_INSIDE == if_nscope ||
           ( (!op) && (NSM_IF_SCOPE_INSIDE == if_oscope) ) )
    {
      _IMI_IF_PROCES_RLIST(IMI_NAT_OUTSIDE,IMI_NAT_SOURCE, ifp->name, NULL);
      _IMI_IF_PROCES_RLIST(IMI_NAT_INSIDE, IMI_NAT_DESTINATION, ifp->name, NULL);
    }
   return IMI_NAT_SUCCESS;
}

int
imi_if_process_nat (struct interface *ifp,
                    u_int8_t if_nscope, int op)
{
  struct imi_interface *imi_if = ifp->info;
  int ret = IMI_NAT_ERROR;

  if (imi_if==NULL)
    return IMI_NAT_ERROR;

  /* Always, silently ignore duplicate config. */
  if ((op && (imi_if->scope == if_nscope)) ||
      (!op && (imi_if->scope == NSM_IF_SCOPE_UNSPEC)))
    return IMI_NAT_SUCCESS;

  /* If the interface not bound to VR, the rule_ref_count must be 0.
     Save the config only.
   */
  if (ifp->vr == NULL)
    {
      if (imi_if->rule_ref_cnt != 0)
        return IMI_NAT_ERROR;

      imi_if->scope = if_nscope;
      return IMI_NAT_SUCCESS;
    }
  /* Change in config while ifp is bound to VR. */
  if (op && (imi_if->rule_ref_cnt != 0))
      /* First, unconfig - then config again. */
      return IMI_NAT_ERROR;

  /* For Processing the Pool List */
  ret = imi_if_process_nat_pool_list(ifp, if_nscope, op);
  if (ret != IMI_NAT_SUCCESS)
    goto imi_if_process_nat_exit;

  /* For Processing of the Static */
  ret = imi_if_process_nat_static (ifp, if_nscope, op);
  if (ret != IMI_NAT_SUCCESS)
    goto imi_if_process_nat_exit;

  /* If everything went smoothly... */
  imi_if->scope = if_nscope;
  return IMI_NAT_SUCCESS;
 
  imi_if_process_nat_exit:
  /* If error occured during applying the rules, undo the rules and
     return an error.
   */
  if (imi_if->rule_ref_cnt != 0 && op)
    {
      imi_if->scope = if_nscope;
      imi_if_process_nat_pool_list(ifp, NSM_IF_SCOPE_UNSPEC, 0);
      imi_if_process_nat_static (ifp, NSM_IF_SCOPE_UNSPEC, 0);
      imi_if->scope = NSM_IF_SCOPE_UNSPEC;
      imi_if->rule_ref_cnt = 0;
    }
  else if (! op)
    {
      /* User wanted to remove it, let's do that in no matter what.
       */
     imi_if->scope = NSM_IF_SCOPE_UNSPEC;
     imi_if->rule_ref_cnt = 0;
    }
   return ret;
}
	
int
imi_if_process_nat_pool_list (struct interface *ifp, u_int8_t if_nscope, int op)
{
  struct listnode *node = NULL;
  struct imi_interface *imi_if = ifp->info;
  struct imi_nat_address_pool *pool = NULL;
  u_int8_t if_oscope;
  int ret = IMI_NAT_SUCCESS;

  if (imi_if == NULL)
    return IMI_NAT_ERROR;

  if_oscope = imi_if->scope;

/*(  printf("\nimi_if_process_nat_pool_list: Applying for if=%s opcode=%d\n", ifp->name, op); */

  for (node = LISTHEAD (imi_nat_address_pool_list); node; NEXTNODE(node))
    {
      if ( (pool = GETDATA(node)) )
        {
          ret = imi_if_apply_nat (ifp->vr, ifp, pool, if_nscope, if_oscope, op);
          if (ret != IMI_NAT_SUCCESS)
             return ret;
        }
    }
  return IMI_NAT_SUCCESS;
}

int
imi_if_process_nat_static (struct interface *ifp, u_int8_t if_nscope, int op)
{
  struct imi_interface *imi_if = ifp->info;
  u_int8_t if_oscope = 0;

  if (imi_if == NULL)
    return IMI_NAT_ERROR;

  if_oscope = imi_if->scope;

  /* For applying the Static Rules on the Interface. */
  return (imi_if_apply_nat (ifp->vr, ifp, NULL, if_nscope, if_oscope, op));
}

CLI (ip_nat_outside,
     ip_nat_outside_cli,
     "ip nat outside",
     "Interface Internet Protocol config commands",
     CLI_NAT_STR,
     "Outside interface")
{
  int ret;
  struct interface *ifp = cli->index;

  ret = imi_if_process_nat(ifp, NSM_IF_SCOPE_OUTSIDE, 1);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

CLI (no_nat_ip_outside,
     no_nat_ip_outside_cli,
     "no ip nat outside",
     CLI_NO_STR,
     "Interface Internet Protocol config commands",
     CLI_NAT_STR,
     "Outside interface")
{
  int ret;
  struct interface *ifp = cli->index;
  struct imi_interface *imi_if = ifp->info;

  if (NSM_IF_SCOPE_OUTSIDE != imi_if->scope)
    {
      cli_out (cli, "%% Interface is not configured as outside.\n");
      return CLI_ERROR;
    }
  ret = imi_if_process_nat (ifp, NSM_IF_SCOPE_UNSPEC, 0);

  return (imi_nat_cli_cmd_ret(cli, ret));
}

CLI (ip_nat_inside,
     ip_nat_inside_cli,
     "ip nat inside",
     "Interface Internet Protocol config commands",
     CLI_NAT_STR,
     "Inside interface")
{
  int ret;
  struct interface *ifp = cli->index;

  ret = imi_if_process_nat(ifp, NSM_IF_SCOPE_INSIDE, 1);

  return (imi_nat_cli_cmd_ret(cli, ret));

  return ret;
}

CLI (no_nat_ip_address_inside,
     no_nat_ip_inside_cli,
     "no ip nat inside",
     CLI_NO_STR,
     "Interface Internet Protocol config commands",
     CLI_NAT_STR,
     "Inside interface")
{
  int ret;
  struct interface *ifp = cli->index;
  struct imi_interface *imi_if = ifp->info;

  if (NSM_IF_SCOPE_INSIDE != imi_if->scope)
    {
      cli_out (cli, "%% Interface is not configured as inside.\n");
      return CLI_ERROR;
    }
  ret = imi_if_process_nat(ifp, NSM_IF_SCOPE_UNSPEC, 0);

  return (imi_nat_cli_cmd_ret(cli, ret));
}
#endif /* HAVE_NAT */

/* Convert line to ifp. */
struct interface *
imi_interface_line2ifp (char *line)
{
  struct interface *ifp = NULL;
  char ifname[INTERFACE_NAMSIZ];
  char *cp, *start;
  int len;

  /* Make sure matches syntax. */
  if (! pal_strncmp (line, "interface ", pal_strlen ("interface ")) ||
      ! pal_strncmp (line, "Interface ", pal_strlen ("Interface ")))
    {
      cp = line + pal_strlen ("interface ");

      /* Return if there's only white spaces. */
      if (*cp == '\0')
        return NULL;

      /* Else, read the interface name. */
      start = cp;
      while (!
             (pal_char_isspace ((int) * cp) || *cp == '\r'
              || *cp == '\n') && *cp != '\0')
        cp++;
      len = cp - start;

      /* Copy the name. */
      pal_strncpy (ifname, start, len);
      ifname[len] = '\0';

      /* Get interface structure. */
      ifp = ifg_lookup_by_name (&imim->ifg, ifname);
    }

  return ifp;
}

/* Interface data is merged between NSM and IMI. */
void
imi_interface_show (void *arg, char *line, module_id_t module)
{
#if defined(HAVE_DHCP_CLIENT) || defined(HAVE_NAT)
  static struct interface *show_ifp;
  static bool_t displayed;
#endif /* defined(HAVE_DHCP_CLIENT) || defined(HAVE_NAT) */
  struct cli *cli = (struct cli *)arg;

#ifdef HAVE_DHCP_CLIENT
  /* Anything to show before a line? */
  if ((!displayed) &&
      (! pal_strncmp (line, "  inet ", pal_strlen ("  inet "))))
    {
      imi_dhcpc_show_interface (cli, show_ifp);
      displayed = PAL_TRUE;
    }
#endif /* HAVE_DHCP_CLIENT */

  /* Show the line. */
  cli_out (cli, line);
  cli_out (cli, "\n");

#ifdef HAVE_NAT
  /* Show after the line. */
  if (! pal_strncmp (line, "Interface ", pal_strlen ("Interface ")))
    {
      show_ifp = imi_interface_line2ifp (line);
      displayed = PAL_FALSE;

      if (show_ifp)
        imi_interface_scope_show (cli, show_ifp);
    }
#endif /* HAVE_NAT */

  return;
}

int
imi_cli_show_interface (struct cli *cli, char *ifname)
{
  char buf[SHOW_LINE_SIZE];
  int ret;

  zsnprintf (buf, sizeof (buf), "show interface %s\n", ifname);

  /* Send message to NSM.  */
  ret = imi_show_protocol (cli->vr, APN_PROTO_NSM, NULL, buf,
                           cli, &imi_interface_show);

  return ret;
}

CLI (imi_show_interface,
     imi_show_interface_cli,
     "show interface (IFNAME|)",
     CLI_SHOW_STR,
     CLI_INTERFACE_STR,
     CLI_IFNAME_STR)
{
  char buf[INTERFACE_NAMSIZ];
  struct interface *ifp = NULL;
  struct listnode *node = NULL;
  char *ifname;

  if (argc > 0)
    {
      if (argc == 1)
        ifname = argv[0];
      else
        ifname = cli_interface_resolve (buf, sizeof (buf), argv[0], argv[1]);

      imi_cli_show_interface (cli, ifname);
    }
  else
    LIST_LOOP (cli->vr->ifm.if_list, ifp, node)
      imi_cli_show_interface (cli, ifp->name);

  return CLI_SUCCESS;
}

void
imi_if_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  if_add_hook (&zg->ifg, IF_CALLBACK_NEW, imi_if_new_hook);
  if_add_hook (&zg->ifg, IF_CALLBACK_DELETE, imi_if_delete_hook);

  /* Install show interface command. */
#ifndef HAVE_CUSTOM1
  cli_install (ctree, EXEC_MODE, &imi_show_interface_cli);
#endif /* HAVE_CUSTOM1 */

#ifdef HAVE_NAT
  /* Install IMI interface specific CLIs. */
  cli_install (ctree, INTERFACE_MODE, &ip_nat_outside_cli);
  cli_install (ctree, INTERFACE_MODE, &no_nat_ip_outside_cli);
  cli_install (ctree, INTERFACE_MODE, &ip_nat_inside_cli);
  cli_install (ctree, INTERFACE_MODE, &no_nat_ip_inside_cli);
#endif /* HAVE_NAT */
}
