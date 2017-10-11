/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#ifdef HAVE_NAT

#include "imi/pal_fw.h"

#include "lib.h"
#include "if.h"
#include "log.h"
#include "prefix.h"
#include "filter.h"
#include "snprintf.h"
#include "cli.h"

#include "imi/imi.h"
#include "imi/imi_interface.h"
#include "imi/util/imi_server.h"
#include "imi/util/imi_fw.h"
#include "imi/util/imi_vs.h"
#include "imi/util/imi_filter.h"


#define CLI_EXECUTE_CMD(T,C,B,E)                                           \
   do {                                                                    \
        /* Execute command. */                                             \
        ret = cli_parse (T, C->mode, PRIVILEGE_MAX, B, 1, 0);              \
        switch (ret)                                                       \
        {                                                                  \
          case CLI_PARSE_NO_MODE:                                          \
             /* Ignore no mode line.  */                                   \
             break;                                                        \
          case CLI_PARSE_EMPTY_LINE:                                       \
             /* Ignore empty line.  */                                     \
             break;                                                        \
          case CLI_PARSE_SUCCESS:                                          \
              E = T->exec_node;                                            \
                                                                           \
              cli->cel = E->cel;                                           \
                                                                           \
              ret = (*E->cel->func)                                        \
                        (C, T->argc, T->argv);                             \
              break;                                                       \
          default:                                                         \
            cli_out (C, "%% Parser error\n");                              \
            ret = CLI_ERROR;                                               \
            break;                                                         \
          }                                                                \
       } while (0)

/* Virtual Servers list. */
struct list *virtual_server_list;

int
imi_virtual_server_configured (void)
{
  return LISTCOUNT (virtual_server_list);
}

int
imi_virtual_server_check (const char *protocol, u_int16_t pub_port,
                          struct pal_in4_addr *priv_ip, u_int16_t priv_port)
{
  char priv_ip_str [PREFIX_STR_LEN];
  struct imi_virtual_server *ivs;
  struct listnode *node;

  zsnprintf (priv_ip_str, PREFIX_STR_LEN, "%r", priv_ip);

  LIST_LOOP (virtual_server_list, ivs, node)
    if (!pal_strcmp (ivs->protocol, protocol)
        && ivs->pub_port == pub_port
        && !pal_strcmp (ivs->priv_ip, priv_ip_str)
        && ivs->priv_port == priv_port)
      return 1;

  return 0;
}

static int
imi_virtual_server_check2 (struct imi_virtual_server *ivs)
{
  struct imi_virtual_server *ivs1;
  struct listnode *node;

  LIST_LOOP (virtual_server_list, ivs1, node)
    if (! pal_strcmp (ivs1->protocol, ivs->protocol)
        && ivs1->pub_port == ivs->pub_port
        && !pal_strcmp (ivs1->priv_ip, ivs->priv_ip)
        && ivs1->priv_port == ivs->priv_port)
      return 1;

  return 0;
}

int
imi_virtual_server_set (struct cli *cli, char *protocol,
                        u_int16_t pub_port,
                        char *priv_ip, u_int16_t priv_port,
                        char *desc)
{
  struct imi_virtual_server *ivs;
  char *ip_nat = "ip nat outside destination static";
  char buf[1024];
  int ret = CLI_SUCCESS;
  struct interface *ifp;
  struct connected *ifc;
  struct imi_interface *imi_if;
  struct listnode *node;
  int found = 0;
  struct prefix *p;
  struct cli_node *exec_node;
  struct apn_vr *vr = apn_vr_get_privileged (imim);

  ivs = XCALLOC (MTYPE_IMI_VIRTUAL_SERVER, sizeof (struct imi_virtual_server));

  pal_strcpy (ivs->protocol, protocol);
  ivs->pub_port = pub_port;
  pal_strcpy (ivs->priv_ip, priv_ip);
  ivs->priv_port = priv_port;
  ivs->desc = XSTRDUP (MTYPE_IMI_VIRTUAL_SERVER_DESC, desc);

  if (imi_virtual_server_check2 (ivs))
    {
      cli_out (cli, "Virtual Server with given parameters already existing\n");
      XFREE (MTYPE_IMI_VIRTUAL_SERVER, ivs);
      return CLI_ERROR;
    }

  LIST_LOOP (vr->ifm.if_list, ifp, node)
    {
      imi_if = ifp->info;

      if (imi_if->scope == NSM_IF_SCOPE_OUTSIDE)
        {
          /* Connected addresses. */
          for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
            {
              found = 1;

              p = ifc->address;

              /* Create the NAT commands to be executed. */
              zsnprintf (buf, 1024, "%s %s %r %d %s %d",
                         ip_nat, protocol, &p->u.prefix4, pub_port,
                         priv_ip, priv_port);

              /* Execute command. */
              CLI_EXECUTE_CMD(imim->ctree, cli, buf, exec_node);
            }
        }
    }

  if (found)
    {
      if (ret != CLI_SUCCESS)
        {
          cli_out (cli, "Outside destination NAT rule exists.\n");
          XFREE (MTYPE_IMI_VIRTUAL_SERVER, ivs);
          return ret;
        }
      else
        {
          /* Add to list. */
          listnode_add (virtual_server_list, ivs);

          return ret;
        }
    }
  else
    {
      cli_out (cli, "No outside interfaces defined/or address configured.\n");
      XFREE (MTYPE_IMI_VIRTUAL_SERVER, ivs);

      return CLI_ERROR;
    }
}

int
imi_virtual_server_unset (struct cli *cli, char *protocol,
                          u_int16_t pub_port,
                          char *priv_ip, u_int16_t priv_port,
                          char *desc)
{
  char *ip_nat = "no ip nat outside destination static";
  char buf[1024];
  int ret = CLI_SUCCESS;
  struct interface *ifp;
  struct connected *ifc;
  struct imi_virtual_server *ivs;
  struct imi_interface *imi_if;
  struct listnode *node, *node1, *next_node;
  struct prefix *p;
  int found = 0;
  struct cli_node *exec_node;
  struct apn_vr *vr = apn_vr_get_privileged (imim);

  for (node = LISTHEAD (virtual_server_list); node; node = next_node)
    {
      ivs = GETDATA (node);

      next_node = node->next;

      if (! pal_strcmp (ivs->protocol, protocol) &&
          (ivs->pub_port == pub_port) &&
          ! pal_strcmp (ivs->priv_ip, priv_ip) &&
          (ivs->priv_port == priv_port) &&
          ! (pal_strcmp (ivs->desc, desc)))
        {
          XFREE (MTYPE_IMI_VIRTUAL_SERVER_DESC, ivs->desc);
          XFREE (MTYPE_IMI_VIRTUAL_SERVER, ivs);
          list_delete_node (virtual_server_list, node);

          for (node1 = LISTHEAD (vr->ifm.if_list); node1; NEXTNODE (node1))
            {
              ifp = GETDATA (node1);
              imi_if = ifp->info;

              if (imi_if->scope == NSM_IF_SCOPE_OUTSIDE)
                {
                  /* Connected addresses. */
                  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
                    {
                      found = 1;
                      p = ifc->address;

                      /* Create the NAT commands to be executed. */
                      zsnprintf (buf, 1024, "%s %s %r %d %s %d",
                                 ip_nat, protocol, &p->u.prefix4, pub_port,
                                 priv_ip, priv_port);

                      /* Execute command. */
                      CLI_EXECUTE_CMD(imim->ctree, cli, buf, exec_node);
                    }
                }
            }
        }
    }

  return ret;
}

#ifdef HOME_GATEWAY
int
imi_virtual_server_reset (struct imi_serv *imi_serv)
{
  char *conf_t = "configure terminal";
  char *end = "end";
  char *ip_nat = "no ip nat outside destination static";
  char buf[IMI_SERVER_CMD_STRING_MAX_LEN];
  struct cli *cli = imi_serv->cli;
  struct interface *ifp;
  struct connected *ifc;
  struct imi_virtual_server *ivs;
  struct imi_interface *imi_if;
  struct listnode *node, *node1, *next_node;
  struct prefix *p;
  int ret;
  struct cli_node *exec_node;
  int clear = 0;
  struct apn_vr *vr = apn_vr_get_privileged (imim);

  /* First, goto config-terminal mode. */
  CLI_EXECUTE_CMD(imim->ctree, cli, conf_t, exec_node);

  for (node = LISTHEAD (virtual_server_list); node; node = next_node)
    {
      ivs = GETDATA (node);

      next_node = node->next;

      for (node1 = LISTHEAD (vr->ifm.if_list); node1; NEXTNODE (node1))
        {
          ifp = GETDATA (node1);
          imi_if = ifp->info;

          if (imi_if->scope == NSM_IF_SCOPE_OUTSIDE)
            {
              /* Connected addresses. */
              for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
                {
                  p = ifc->address;

                  /* Create the NAT commands to be executed. */
                  zsnprintf (buf, 1024, "%s %s %r %d %s %d",
                             ip_nat, ivs->protocol,
                             &p->u.prefix4, ivs->pub_port,
                             ivs->priv_ip, ivs->priv_port);

                  XFREE (MTYPE_IMI_VIRTUAL_SERVER_DESC, ivs->desc);
                  XFREE (MTYPE_IMI_VIRTUAL_SERVER, ivs);
                  list_delete_node (virtual_server_list, node);
                  clear = 1;

                  /* Execute command. */
                  CLI_EXECUTE_CMD(imim->ctree, cli, buf, exec_node);
                }
            }
        }
      if (!clear)
      {
        XFREE (MTYPE_IMI_VIRTUAL_SERVER_DESC, ivs->desc);
        XFREE (MTYPE_IMI_VIRTUAL_SERVER, ivs);
        list_delete_node (virtual_server_list, node);
      }
    }

  /* Back to enable-mode. */
  CLI_EXECUTE_CMD(imim->ctree, cli, end, exec_node);

  return RESULT_OK;
}
#endif /* HOME_GATEWAY */

int
imi_virtual_server_show (struct cli *cli)
{
  struct imi_virtual_server *ivs;
  struct listnode *node;

  cli_out (cli, "%-8s %-16s %-16s %-16s %s\n",
           "Prot", "Public-port", "Private-IP", "Private-port", "Description");

  LIST_LOOP (virtual_server_list, ivs, node)
    cli_out (cli, "%-8s %-16d %-16s %-16d %s\n", ivs->protocol,
             ivs->pub_port, ivs->priv_ip, ivs->priv_port, ivs->desc);

  return 0;
}

static int
imi_virtual_server_if_address (struct connected *c, int op)
{
  struct interface *ifp = c->ifp;
  struct imi_interface *imi_if;
  struct listnode *node, *next_node;
  struct imi_virtual_server *ivs;
  struct imi_rule rule;
  struct prefix addr;
  int ret;

  if (!ifp)
    return -1;

  imi_if = ifp->info;
  if (!imi_if || imi_if->scope != NSM_IF_SCOPE_OUTSIDE)
    return 0;

  for (node = LISTHEAD (virtual_server_list); node; node = next_node)
    {
      ivs = GETDATA (node);

      next_node = node->next;

      pal_mem_set (&rule, 0, sizeof (struct imi_rule));

      rule.flag = IMI_RULE_STATIC;
      rule.u.srule.protocol = protocol_type (ivs->protocol);

      SET_FLAG (rule.u.srule.flags, IMI_RULE_DESTINATION);
      pal_mem_cpy (&rule.u.srule.dprefix, c->address, sizeof (struct prefix_am4));
      rule.u.srule.dport = ivs->pub_port;
      if (str2prefix (ivs->priv_ip, &addr) <= 0)
        {
          zlog_info (imim, "PacOS: Malformed Prefix %s!\n", ivs->priv_ip);
          continue; 
        }
      rule.u.srule.min_ip = addr.u.prefix4;
      rule.u.srule.max_ip = addr.u.prefix4;

      rule.u.srule.min_port = ivs->priv_port;
      rule.u.srule.max_port = ivs->priv_port;

      ret = imi_nat_translation_static (IMI_NAT_OUTSIDE, &rule, IMI_NAT_DESTINATION, op);
      if (ret < 0)
        zlog_info (imim, "PacOS: Failed to add NAT translation");

    }

  return 0;
}

int
imi_virtual_server_if_address_add (struct connected *c)
{
  return imi_virtual_server_if_address (c, 1);
}

int
imi_virtual_server_if_address_delete (struct connected *c)
{
  return imi_virtual_server_if_address (c, 0);
}

CLI (imi_virtual_server,
     imi_virtual_server_cmd,
     "virtual-server (tcp|udp) <0-65535> A.B.C.D <0-65535> WORD",
     "Virtual-server configuration",
     "TCP",
     "UDP",
     "Public port",
     "Private IP address",
     "Private port",
     "Virtual-server name")
{
  u_int16_t pub_port, priv_port;
  int ret;
  struct pal_in4_addr gate;

  /* Check if IP Address is Ok. */
  ret = pal_inet_pton (AF_INET, argv[2], &gate);
  if (! ret)
    {
      cli_out (cli, "%% Malformed address\n");
      return CLI_ERROR;
    }

  pub_port = pal_atoi (argv[1]);
  priv_port = pal_atoi (argv[3]);

  ret = imi_virtual_server_set (cli, argv[0], pub_port,
                                argv[2], priv_port, argv[4]);

  return ret;
}

CLI (no_imi_virtual_server,
     no_imi_virtual_server_cmd,
     "no virtual-server (tcp|udp) <0-65535> A.B.C.D <0-65535> WORD",
     CLI_NO_STR,
     "Virtual-server configuration",
     "TCP",
     "UDP",
     "Public port",
     "Private IP address",
     "Private port",
     "Virtual-server name")
{
  u_int16_t pub_port, priv_port;
  int ret;

  pub_port = pal_atoi (argv[1]);
  priv_port = pal_atoi (argv[3]);

  ret = imi_virtual_server_unset (cli, argv[0], pub_port,
                                  argv[2], priv_port, argv[4]);

  return ret;
}

CLI (show_imi_virtual_server,
     show_imi_virtual_server_cmd,
     "show virtual-servers",
     CLI_SHOW_STR,
     "Virtual-servers")
{
  imi_virtual_server_show (cli);
  return CLI_SUCCESS;
}

int
imi_virtual_server_config_encode (cfg_vect_t *cv)
{
  struct imi_virtual_server *ivs;
  struct listnode *node;
  int write = 0;

  LIST_LOOP (virtual_server_list, ivs, node)
    {
      cfg_vect_add_cmd (cv, "virtual-server %s %d %s %d %s\n", ivs->protocol,
               ivs->pub_port, ivs->priv_ip, ivs->priv_port, ivs->desc);
      write++;
    }
  if (write) {
    cfg_vect_add_cmd(cv, "!\n");
  }
  return write;
}

int
imi_virtual_server_config_write (struct cli *cli)
{
  cli->cv = cfg_vect_init(cli->cv);
  imi_virtual_server_config_encode(cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
  return 0;
}

int
imi_virtual_server_cli_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &imi_virtual_server_cmd);
  cli_set_imi_cmd (&imi_virtual_server_cmd, CONFIG_MODE, CFG_DTYP_IMI_VIRTUAL_SERVER);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_MAX, 0,
                   &no_imi_virtual_server_cmd);

  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_imi_virtual_server_cmd);

  return 0;
}

int
imi_virtual_server_init (struct lib_globals *zg)
{
  virtual_server_list = list_new ();

  imi_virtual_server_cli_init (zg);

  return 0;
}

int
imi_virtual_server_finish (struct lib_globals *zg)
{
  list_delete (virtual_server_list);

  return 0;
}

#endif /* HAVE_NAT */
