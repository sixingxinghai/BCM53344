/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#ifdef HAVE_DNS

#include "imi/pal_dns.h"

#include "cli.h"
#include "prefix.h"
#include "linklist.h"

#include "imi/imi.h"


/* Refresh DNS after configuration change if DNS is enabled. */
static int
imi_dns_refresh ()
{
  int ret;

  /* Call PAL API to perform this action for different OS. */
  ret = pal_dns_refresh (imim, DNS);
  return ret;
}

/* Handle IMI refresh error. */
static void
imi_dns_print_refresh_error (struct cli *cli, int result)
{
  switch (result)
    {
    case IMI_API_SUCCESS:
      break;
    case IMI_API_FILE_OPEN_ERROR:
      cli_out (cli, "%% Error opening DNS configuration file\n");
      break;
    case IMI_API_FILE_SEEK_ERROR:
      cli_out (cli, "%% Error seeking start of DNS configuration file\n");
      break;
    case IMI_API_FILE_WRITE_ERROR:
      cli_out (cli, "%% Error writing to DNS configuration file\n");
      break;
    default:
      cli_out (cli, "%% Unknown error updating DNS\n");
      break;
    }
  return;
}

/* API functions for CLI/SNMP.  */

/* API: Enable DNS. */
int
imi_dns_enable ()
{
  DNS->enabled = PAL_TRUE;
  return imi_dns_refresh ();
}

/* API: Disable DNS. */
int
imi_dns_disable ()
{
  DNS->enabled = PAL_FALSE;
  return imi_dns_refresh ();
}

/* API: Add nameserver to DNS. */
int
imi_dns_name_server_add (char *nameserver)
{
  struct listnode *node;
  struct prefix_ipv4 p;
  struct prefix_ipv4 *p4, *tmp;
  int ret = 0;

  /* Change string to prefix. */
  ret = str2prefix_ipv4 (nameserver, &p);
  if (ret <= 0)
    return IMI_API_IPV4_ERROR;

  /* Check for duplicates. */
  if (listcount (DNS->ns_list) > 0)
    LIST_LOOP (DNS->ns_list, tmp, node)
    {
      if (tmp->family == p.family &&
          tmp->prefix.s_addr == p.prefix.s_addr &&
          tmp->prefixlen == p.prefixlen)
        return IMI_API_DUPLICATE_ERROR;
    }

  /* Allocate listnode prefix. */
  p4 = prefix_ipv4_new ();
  p4->family = p.family;
  p4->prefix = p.prefix;
  p4->prefixlen = p.prefixlen;

  /* Add to list. */
  listnode_add (DNS->ns_list, p4);

  return imi_dns_refresh ();
}

/* API: Delete nameserver from DNS. */
int
imi_dns_name_server_del (char *nameserver)
{
  struct listnode *node, *next;
  struct prefix_ipv4 p;
  struct prefix_ipv4 *tmp;
  bool_t found = PAL_FALSE;
  int ret = 0;

  /* Change string to prefix. */
  ret = str2prefix_ipv4 (nameserver, &p);
  if (ret <= 0)
    return IMI_API_IPV4_ERROR;

  /* Check for duplicates. */
  if (listcount (DNS->ns_list) > 0)
    for (node = LISTHEAD (DNS->ns_list); node; )
      {
        tmp = GETDATA (node);
        next = node->next;
        if (tmp->family == p.family &&
            tmp->prefix.s_addr == p.prefix.s_addr &&
            tmp->prefixlen == p.prefixlen)
          {
            /* Free prefix and node, & exit loop. */
            prefix_ipv4_free (tmp);
            list_delete_node (DNS->ns_list, node);
            found = PAL_TRUE;
            break;
          }
        node = next;
      }

  /* If not found, return error. */
  if (! found)
    return IMI_API_INVALID_ARG_ERROR;

  return imi_dns_refresh ();
}

/* API: Show DNS nameservers. */
void
imi_dns_show_name_server (struct cli *cli)
{
  struct listnode *node;
  struct prefix_ipv4 *p;

  /* Show nameservers. */
  if (listcount (DNS->ns_list))
    LIST_LOOP (DNS->ns_list, p, node)
    {
      cli_out (cli, "%r\n", &p->prefix);
    }
}

/* API: Add domain to DNS domain-list. */
int
imi_dns_domain_list_add (char *name)
{
  struct listnode *node;
  char *tmp;
  char *domain;

  /* Check for duplicates. */
  if (listcount (DNS->domain_list) > 0)
    LIST_LOOP (DNS->domain_list, tmp, node)
    {
      if (! pal_strcmp (tmp, name))
        return IMI_API_DUPLICATE_ERROR;
    }

  /* Add to list. */
  domain = XSTRDUP (MTYPE_IMI_STRING, name);
  listnode_add (DNS->domain_list, domain);
  return imi_dns_refresh ();
}

/* API: Delete domain from DNS domain-list. */
int
imi_dns_domain_list_del (char *name)
{
  struct listnode *node, *next;
  char *tmp;
  bool_t found = PAL_FALSE;

  if (listcount (DNS->domain_list) == 0)
    return IMI_API_INVALID_ARG_ERROR;

  /* Search the list. */
  for (node = LISTHEAD (DNS->domain_list); node; )
    {
      tmp = GETDATA (node);
      next = node->next;
      if (! pal_strcmp (tmp, name))
        {
          /* Free domain string and node & exit loop. */
          XFREE (MTYPE_IMI_STRING, tmp);
          list_delete_node (DNS->domain_list, node);
          found = PAL_TRUE;
          break;
        }
      node = next;
    }

  /* If not found, return error. */
  if (!found)
    return IMI_API_INVALID_ARG_ERROR;

  return imi_dns_refresh ();
}

/* API: show domain list. */
void
imi_dns_show_domain_list (struct cli *cli)
{
  struct listnode *node;
  char *domain;

  /* Show domain name. */
  if (listcount (DNS->domain_list))
    {
      LIST_LOOP (DNS->domain_list, domain, node)
        {
          cli_out (cli, "%s\n", domain);
        }
    }
}

/* API: Set default domain name. */
result_t
imi_dns_set_default_domain (char *name)
{
  /* We accept only one default. */
  if (DNS->default_domain != NULL)
    return IMI_API_DUPLICATE_ERROR;
  else
    DNS->default_domain = XSTRDUP (MTYPE_IMI_STRING, name);
  return imi_dns_refresh ();
}

/* API: Delete default domain name. */
result_t
imi_dns_unset_default_domain (char *name)
{
  /* If no default domain, just return. */
  if (DNS->default_domain == NULL)
    return IMI_API_NOT_SET_ERROR;
  else
    {
      if (! pal_strcmp (DNS->default_domain, name))
        {
          XFREE (MTYPE_IMI_STRING, DNS->default_domain);
          DNS->default_domain = NULL;
        }
      else
        return IMI_API_INVALID_ARG_ERROR;
    }
  return imi_dns_refresh ();
}

/* API: show domain name. */
void
imi_dns_show_domain_name (struct cli *cli)
{
  /* Show domain name. */
  if (DNS->default_domain)
    cli_out (cli, "%s\n", DNS->default_domain);
}

/* API: Reset DNS & clear all configuration. */
result_t
imi_dns_reset ()
{
  struct listnode *node, *next;
  struct prefix_ipv4 *p;
  char *d;

  /* Remove all nameservers. */
  if (listcount (DNS->ns_list) > 0)
    for (node = LISTHEAD (DNS->ns_list); node; )
      {
        p = GETDATA (node);
        next = node->next;
        prefix_ipv4_free (p);
        list_delete_node (DNS->ns_list, node);
        node = next;
      }

  /* Remove search list. */
  if (listcount (DNS->domain_list) > 0)
    for (node = LISTHEAD (DNS->domain_list); node; )
      {
        d = GETDATA (node);
        next = node->next;
        XFREE (MTYPE_IMI_STRING, d);
        list_delete_node (DNS->domain_list, node);
        node = next;
      }

  /* Remove default domain. */
  if (DNS->default_domain)
    XFREE (MTYPE_IMI_STRING, DNS->default_domain);
  DNS->default_domain = NULL;

  /* Turn DNS off. */
  imi_dns_disable ();

  return RESULT_OK;
}

/* CLI functions.  */

/* Enable Domain Name Service (DNS). */
CLI (ip_dns_domain_lookup,
     ip_dns_domain_lookup_cli,
     "ip domain-lookup",
     CLI_IP_STR,
     "Enable Domain Name Service (DNS)")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dns_enable ();
  switch (ret)
    {
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      imi_dns_print_refresh_error (cli, ret);
      break;
    }
  return result;
}

/* Disable Domain Name Service (DNS). */
CLI (no_ip_dns_domain_lookup,
     no_ip_dns_domain_lookup_cli,
     "no ip domain-lookup",
     CLI_NO_STR,
     CLI_IP_STR,
     "Enable Domain Name Service (DNS)")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dns_disable ();
  switch (ret)
    {
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      imi_dns_print_refresh_error (cli, ret);
      break;
    }
  return result;
}

/* Add a Nameserver to DNS. */
CLI (ip_dns_name_server,
     ip_dns_name_server_cli,
     "ip name-server A.B.C.D",
     CLI_IP_STR,
     "Add a Nameserver to the DNS",
     "IP address of Nameserver to add")
{
  int ret;
  int result = CLI_ERROR;

  /* Are we reading the DNS configuration via IMI?
     If yes: If system config is present, clear it. */
  if (cli->source == CLI_SOURCE_FILE)
    if (DNS->sysconfig)
      {
        imi_dns_reset ();
        DNS->sysconfig = PAL_FALSE;
      }

  /* Call API. */
  ret = imi_dns_name_server_add (argv[0]);
  switch (ret)
    {
    case IMI_API_DUPLICATE_ERROR:
      cli_out (cli, "%% Nameserver already defined\n");
      break;
    case IMI_API_IPV4_ERROR:
      cli_out (cli, "%% Invalid nameserver address\n");
      break;
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      imi_dns_print_refresh_error (cli, ret);
      break;
    }

  return result;
}

/* Delete a Nameserver from DNS. */
CLI (no_ip_dns_name_server,
     no_ip_dns_name_server_cli,
     "no ip name-server A.B.C.D",
     CLI_NO_STR,
     CLI_IP_STR,
     "Add a Nameserver to the DNS",
     "IP address of Nameserver to add")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dns_name_server_del (argv[0]);
  switch (ret)
    {
    case IMI_API_INVALID_ARG_ERROR:
      cli_out (cli, "%% Nameserver doesn't exist\n");
      break;
    case IMI_API_IPV4_ERROR:
      cli_out (cli, "%% Invalid nameserver address\n");
      break;
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      imi_dns_print_refresh_error (cli, ret);
      break;
    }

  return result;
}

/* Show default domain. */
CLI (show_ip_dns_name_server,
     show_ip_dns_name_server_cli,
     "show ip name-server",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "DNS nameservers")
{
  /* Call API. */
  imi_dns_show_name_server (cli);
  return CLI_SUCCESS;
}

/* Add a domain to the DNS list. */
CLI (ip_dns_domain_list,
     ip_dns_domain_list_cli,
     "ip domain-list WORD",
     CLI_IP_STR,
     "Add a domain to the DNS",
     "Domain string (e.g. company.com)")
{
  int ret;
  int result = CLI_ERROR;

  /* Are we reading the DNS configuration via IMI?
     If yes: If system config is present, clear it. */
  if (cli->source == CLI_SOURCE_FILE)
    if (DNS->sysconfig)
      {
        imi_dns_reset ();
        DNS->sysconfig = PAL_FALSE;
      }

  /* Call API. */
  ret = imi_dns_domain_list_add (argv[0]);
  switch (ret)
    {
    case IMI_API_DUPLICATE_ERROR:
      cli_out (cli, "%% Error: domain already exists\n");
      break;
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      imi_dns_print_refresh_error (cli, ret);
      break;
    }

  return result;
}

/* Delete a domain from the DNS list. */
CLI (no_ip_dns_domain_list,
     no_ip_dns_domain_list_cli,
     "no ip domain-list WORD",
     CLI_NO_STR,
     CLI_IP_STR,
     "Add a domain to the DNS",
     "Domain string (e.g. company.com)")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dns_domain_list_del (argv[0]);
  switch (ret)
    {
    case IMI_API_INVALID_ARG_ERROR:
      cli_out (cli, "%% Invalid domain name\n");
      break;
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      imi_dns_print_refresh_error (cli, ret);
      break;
    }

  return result;
}

/* Show default domain. */
CLI (show_ip_dns_domain_list,
     show_ip_dns_domain_list_cli,
     "show ip domain-list",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "Domain list for DNS")
{
  /* Call API. */
  imi_dns_show_domain_list (cli);
  return CLI_SUCCESS;
}

/* Set default domain name for DNS. */
CLI (ip_dns_domain_name,
     ip_dns_domain_name_cli,
     "ip domain-name WORD",
     CLI_IP_STR,
     "Set default domain for DNS",
     "Domain string (e.g. company.com)")
{
  int ret;
  int result = CLI_ERROR;

  /* Are we reading the DNS configuration via IMI?
     If yes: If system config is present, clear it. */
  if (cli->source == CLI_SOURCE_FILE)
    if (DNS->sysconfig)
      {
        imi_dns_reset ();
        DNS->sysconfig = PAL_FALSE;
      }

  /* Call API. */
  ret = imi_dns_set_default_domain (argv[0]);
  switch (ret)
    {
    case IMI_API_DUPLICATE_ERROR:
      cli_out (cli, "%% Default domain already set to %s\n",
               DNS->default_domain);
      break;
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      imi_dns_print_refresh_error (cli, ret);
      break;
    }

  return result;
}

/* Set default domain name for DNS. */
CLI (no_ip_dns_domain_name,
     no_ip_dns_domain_name_cli,
     "no ip domain-name WORD",
     CLI_NO_STR,
     CLI_IP_STR,
     "Set default domain for DNS",
     "Domain string (e.g. company.com)")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dns_unset_default_domain (argv[0]);
  switch (ret)
    {
    case IMI_API_INVALID_ARG_ERROR:
      cli_out (cli, "%% Domain name does not exist\n");
      break;
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      imi_dns_print_refresh_error (cli, ret);
      break;
    }

  return result;
}

/* Show default domain. */
CLI (show_ip_dns_domain_name,
     show_ip_dns_domain_name_cli,
     "show ip domain-name",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "Default domain for DNS")
{
  /* Call API. */
  imi_dns_show_domain_name (cli);
  return CLI_SUCCESS;
}

/* Show default domain. */
CLI (show_hosts,
     show_hosts_cli,
     "show hosts",
     CLI_SHOW_STR,
     "IP domain-name, lookup style and nameservers")
{
  struct listnode *node;
  char *domain;
  struct prefix_ipv4 *p;

  /* Default domain. */
  cli_out (cli, "Default domain is ");
  if (DNS->default_domain)
    cli_out (cli, "%s\n", DNS->default_domain);
  else
    cli_out (cli, "not set\n");

  /* Domain lists. */
  if (listcount (DNS->domain_list))
    {
      cli_out (cli, "Domain list:");
      LIST_LOOP (DNS->domain_list, domain, node)
        {
          cli_out (cli, " %s", domain);
        }
      cli_out (cli, "\n");
    }

  /* Name lookup status.  */
  if (DNS->enabled == PAL_TRUE)
    {
      cli_out (cli, "Name/address lookup uses domain service\n");

      /* Show nameservers. */
      if (listcount (DNS->ns_list))
        {
          cli_out (cli, "Name servers are");
          LIST_LOOP (DNS->ns_list, p, node)
            {
              cli_out (cli, " %r", &p->prefix);
            }
          cli_out (cli, "\n");
        }
    }
  else
    cli_out (cli, "Name/address lookup is disabled\n");


  return CLI_SUCCESS;
}

/* Init / Shutdown.  */

/* Configuration write for DNS Client. */
int
imi_dns_config_encode (struct imi_dns *dns, cfg_vect_t *cv)
{
  struct listnode *node;
  struct prefix_ipv4 *p;
  char *str;
  char pref[IPV4_ADDR_STRLEN];

  /* DNS enabled? */
  if (DNS->enabled)
    {
      /* Default domain. */
      if (DNS->default_domain)
        cfg_vect_add_cmd (cv, "ip domain-name %s\n", dns->default_domain);

      /* Search domain(s). */
      if (listcount (DNS->domain_list) > 0)
        LIST_LOOP (DNS->domain_list, str, node)
        {
          cfg_vect_add_cmd (cv, "ip domain-list %s\n", str);
        }

      /* Name server(s). */
      if (listcount (DNS->ns_list) > 0)
        LIST_LOOP (DNS->ns_list, p, node)
        {
          pal_inet_ntoa (p->prefix, pref);
          cfg_vect_add_cmd (cv, "ip name-server %s\n", pref);
        }

      /* Enable. */
      cfg_vect_add_cmd (cv, "ip domain-lookup\n");
    }
  else
    /* Disable message. */
    cfg_vect_add_cmd (cv, "no ip domain-lookup\n");

  cfg_vect_add_cmd (cv, "!\n");

  return 0;
}


int
imi_dns_config_write (struct cli *cli)
{
  cli->cv = cfg_vect_init(cli->cv);
  imi_dns_config_encode(DNS, cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
  return 0;
}

/* Shutdown DNS. */
void
imi_dns_shutdown ()
{
  struct listnode *node;
  struct prefix_ipv4 *p;
  char *str;

  /* Set shutdown flag so PAL is aware. */
  DNS->shutdown_flag = PAL_TRUE;

  /* Free nameservers & list. */
  LIST_LOOP (DNS->ns_list, p, node)
    {
      prefix_ipv4_free (p);
    }
  list_delete (DNS->ns_list);

  /* Free search strings & list. */
  LIST_LOOP (DNS->domain_list, str, node)
    {
      XFREE (MTYPE_IMI_STRING, str);
    }
  list_delete (DNS->domain_list);

  /* Free default domain. */
  if (DNS->default_domain)
    XFREE (MTYPE_IMI_STRING, DNS->default_domain);

  /* Refresh system DNS. */
  imi_dns_refresh ();

  /* Stop PAL. */
  pal_dns_stop (IMI->zg, DNS->pal_dns);

  /* Free DNS. */
  XFREE (MTYPE_IMI_DNS, DNS);
}

/* Initialize DNS.  */
void
imi_dns_init (struct imi_globals *imi)
{
  struct cli_tree *ctree = imi->zg->ctree;
  struct imi_dns *dns;

  /* Allocate DNS data structure.  */
  dns = XCALLOC (MTYPE_IMI_DNS, sizeof (struct imi_dns));
  if (! dns)
    return;

  /* Initialize DNS data structure.  */
  dns->ns_list = list_new ();
  dns->domain_list = list_new ();
  dns->default_domain = NULL;
  dns->sysconfig = PAL_FALSE;
  dns->shutdown_flag = PAL_FALSE;
  imi->dns = dns;

  /* Initialize DNS PAL.  */
  dns->pal_dns = pal_dns_start (imi->zg, dns);

  /* Install DNS commands.  */
  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_dns_domain_lookup_cli);
  cli_set_imi_cmd (&ip_dns_domain_lookup_cli, CONFIG_MODE, CFG_DTYP_IMI_DNS);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_dns_domain_lookup_cli);
  cli_set_imi_cmd (&no_ip_dns_domain_lookup_cli, CONFIG_MODE, CFG_DTYP_IMI_DNS);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_dns_name_server_cli);
  cli_set_imi_cmd (&ip_dns_name_server_cli, CONFIG_MODE, CFG_DTYP_IMI_DNS);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_dns_name_server_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_dns_domain_list_cli);
  cli_set_imi_cmd (&ip_dns_domain_list_cli, CONFIG_MODE, CFG_DTYP_IMI_DNS);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_dns_domain_list_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_dns_domain_name_cli);
  cli_set_imi_cmd (&ip_dns_domain_name_cli, CONFIG_MODE, CFG_DTYP_IMI_DNS);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_dns_domain_name_cli);

  /* Show commands.  */
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ip_dns_domain_list_cli);
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ip_dns_name_server_cli);
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_hosts_cli);
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ip_dns_domain_name_cli);

  /* Default is enabled.  */
  imi_dns_enable ();
}

#endif /* HAVE_DNS */
