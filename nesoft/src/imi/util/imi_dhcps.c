/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#ifdef HAVE_DHCP_SERVER

#include "imi/pal_dhcp.h"

#include "cli.h"
#include "prefix.h"

#include "imi/imi.h"


/* Prototypes.  */
static int imi_dhcps_pool_range_flush (struct imi_dhcp_pool *pool);
static int imi_dhcps_ipv4_list_flush (struct list *list);

/* Static functions.  */

/* Create new interface structure. */
static struct imi_dhcp_pool *
imi_dhcps_pool_new ()
{
  struct imi_dhcp_pool *pool;

  pool = XCALLOC (MTYPE_IMI_DHCP_POOL, sizeof (struct imi_dhcp_pool));
  return pool;
}

static struct imi_dhcp_pool *
imi_dhcps_pool_create (char *pool_id, struct list *pool_list)
{
  struct imi_dhcp_pool *pool;

  /* Allocate memory & add to list. */
  pool = imi_dhcps_pool_new ();
  listnode_add (pool_list, pool);

  /* Initialize defaults. */
  pool->network_set = PAL_FALSE;
  pool->pool_id = XSTRDUP (MTYPE_IMI_STRING, pool_id);
  pool->addr_list = list_new ();
  pool->range_list = list_new ();
  pool->infinite = PAL_FALSE;
  pool->lease_seconds = IMI_DHCP_LEASE_DEFAULT_SEC;
  pool->lease_minutes = IMI_DHCP_LEASE_DEFAULT_MIN;
  pool->lease_hours = IMI_DHCP_LEASE_DEFAULT_HR;
  pool->lease_days = IMI_DHCP_LEASE_DEFAULT_DAY;
  pool->domain_name = NULL;
  pool->dns_list = list_new ();
  pool->dr_list = list_new ();

  return pool;
}

/* Delete and free pool structure. */
static void
imi_dhcps_pool_delete (struct imi_dhcp_pool *pool, struct list *pool_list)
{
  listnode_delete (pool_list, pool);

  /* Flush ranges. */
  imi_dhcps_pool_range_flush (pool);

  /* Flush all the ipv4 lists. */
  imi_dhcps_ipv4_list_flush (pool->addr_list);
  imi_dhcps_ipv4_list_flush (pool->dns_list);
  imi_dhcps_ipv4_list_flush (pool->dr_list);

  /* Delete lists. */
  list_delete (pool->addr_list);
  list_delete (pool->range_list);
  list_delete (pool->dns_list);
  list_delete (pool->dr_list);
  if (pool->domain_name)
    XFREE (MTYPE_IMI_STRING, pool->domain_name);

  if (pool->pool_id)
    XFREE(MTYPE_IMI_STRING, pool->pool_id);

  /* Free strcture. */
  XFREE (MTYPE_IMI_DHCP_POOL, pool);
}

/* Pool existance check by pool name/id. */
struct imi_dhcp_pool *
imi_dhcps_pool_lookup (char *name, struct list *pool_list)
{
  struct listnode *node;
  struct imi_dhcp_pool *pool;

  LIST_LOOP (pool_list, pool, node)
    {
      if (pal_strcmp (name, pool->pool_id) == 0)
        return pool;
    }
  return NULL;
}

/* Refresh DHCPs PAL configuration. */
static int
imi_dhcps_config_refresh ()
{
  int ret;

  /* Call PAL API to perform this action for different OS. */
  ret = pal_dhcp_refresh (imig->zg, DHCP);
  return ret;
}

/* Refresh DHCP server. */
int
imi_dhcps_refresh (struct interface *ifp)
{
  struct listnode *node;
  struct imi_dhcp_pool *pool;
  struct connected *ifc;
  struct prefix p;
  int ret = 0;

  if (DHCP->enabled == PAL_FALSE)
    return ret;

  for (ifc = ifp->ifc_ipv4; ifc; ifc = ifc->next)
    {
      LIST_LOOP (DHCP->pool_list, pool, node)
        {
          pal_mem_set (&p, 0, sizeof (struct prefix));
          p.family = pool->network.family;
          p.prefixlen = pool->network.prefixlen;
          p.u.prefix4 = pool->network.prefix;

          if (prefix_cmp (ifc->address, &p) == 0)
            {
             ret = imi_dhcps_config_refresh ();
             break;
            }
        }
    }
  return ret;
}


/* Add arg to IPv4 address list in DHCP. */
static int
imi_dhcps_ipv4_list_add (struct imi_dhcp_pool *pool,
                         char *ipv4_str,
                         struct list *list)
{
  struct listnode *node;
  struct prefix_ipv4 p, range;
  struct prefix_ipv4 *p4, *tmp;
  int ret = IMI_API_SUCCESS;

  /* Change string to prefix. */
  ret = str2prefix_ipv4 (ipv4_str, &p);
  if (ret <= 0)
    return IMI_API_IPV4_ERROR;

  /* Out-of-range check. */
  ret = str2prefix_ipv4 (ipv4_str, &range);
  if (ret <= 0)
    return IMI_API_IPV4_ERROR;
  range.prefixlen = pool->network.prefixlen;
  apply_mask_ipv4 (&range);

  /* Check for duplicates. */
  if (listcount (list) > 0)
    LIST_LOOP (list, tmp, node)
      {
        if (tmp->family == p.family &&
            (IPV4_ADDR_SAME (&tmp->prefix.s_addr, &p.prefix.s_addr)) &&
            tmp->prefixlen == p.prefixlen)
          return IMI_API_DUPLICATE_ERROR;
      }

  /* Allocate listnode prefix. */
  p4 = prefix_ipv4_new ();
  p4->family = p.family;
  p4->prefix = p.prefix;
  p4->prefixlen = p.prefixlen;

  /* Add to list. */
  listnode_add (list, p4);
  return imi_dhcps_config_refresh ();
}

/* Delete arg from IPv4 address list in DHCP. */
static int
imi_dhcps_ipv4_list_del (char *ipv4_str, struct list *list)
{
  struct listnode *node, *next;
  struct prefix_ipv4 p;
  struct prefix_ipv4 *tmp;
  bool_t found = PAL_FALSE;
  int ret = 0;

  /* Change string to prefix. */
  ret = str2prefix_ipv4 (ipv4_str, &p);
  if (ret <= 0)
    return IMI_API_IPV4_ERROR;

  /* Check for duplicates. */
  if (listcount (list) > 0)
    for (node = LISTHEAD (list); node; )
      {
        tmp = GETDATA (node);
        next = node->next;
        if (tmp->family == p.family &&
            (IPV4_ADDR_SAME (&tmp->prefix.s_addr, &p.prefix.s_addr)) &&
            tmp->prefixlen == p.prefixlen)
          {
            /* Free prefix and node, & exit loop. */
            prefix_ipv4_free (tmp);
            list_delete_node (list, node);
            found = PAL_TRUE;
            break;
          }
        node = next;
      }

  /* If not found, return error. */
  if (!found)
    return IMI_API_INVALID_ARG_ERROR;

  return imi_dhcps_config_refresh ();
}

/* Flush the entire IPv4 address list. */
static int
imi_dhcps_ipv4_list_flush (struct list *list)
{
  struct listnode *node, *next;
  struct prefix_ipv4 *p;

  /* Delete all nodes in the list. */
  if (listcount (list) > 0)
    for (node = LISTHEAD (list); node; )
      {
        p = GETDATA (node);
        next = node->next;
        prefix_ipv4_free (p);
        list_delete_node (list, node);
        node = next;
      }
  return imi_dhcps_config_refresh ();
}

/* Show IPv4 list to cli in DHCP. */
static void
imi_dhcps_ipv4_list_show (struct cli *cli, struct list *list)
{
  struct listnode *node;
  struct prefix_ipv4 *p;
  char buf[IPV4_ADDR_STRLEN];
  int i = 0;

  /* Show IPv4 addrs. */
    {
      LIST_LOOP (list, p, node)
        {
          pal_inet_ntoa (p->prefix, buf);
          cli_out (cli, "%s %s", (i > 0) ? "," : "", buf);
          i = 1;
        }
    }
}

/* API functions for CLI/SNMP.  */

/* API: Enable DHCP Server. */
int
imi_dhcps_enable ()
{
  int ret;

  DHCP->enabled = PAL_TRUE;
  ret = imi_dhcps_config_refresh ();
  return ret;
}

/* API: Disable DHCP Server. */
int
imi_dhcps_disable ()
{
  int ret;

  DHCP->enabled = PAL_FALSE;
  ret = imi_dhcps_config_refresh ();
  return ret;
}

/* API: Show state. */
int
imi_dhcps_state_show (struct cli *cli)
{
  if (DHCP->enabled)
    cli_out (cli, "service dhcp enabled\n");
  else
    cli_out (cli, "service dhcp disabled\n");
  return IMI_API_SUCCESS;
}

/* API: Create DHCP Pool / GoTo DHCP Pool Configuration. */
int
imi_dhcps_pool (char *id_str)
{
  struct imi_dhcp_pool *pool;

  /* Lookup pool. */
  pool = imi_dhcps_pool_lookup (id_str, DHCP->pool_list);

  /* Create if null. */
  if (!pool)
    imi_dhcps_pool_create (id_str, DHCP->pool_list);

  return IMI_API_SUCCESS;
}

/* API: Delete DHCP Pool. */
int
imi_dhcps_pool_del (char *id_str)
{
  struct imi_dhcp_pool *pool;

  /* Lookup pool. */
  pool = imi_dhcps_pool_lookup (id_str, DHCP->pool_list);

  /* Create if null. */
  if (pool)
    imi_dhcps_pool_delete (pool, DHCP->pool_list);
  else
    return IMI_API_INVALID_ARG_ERROR;

  return imi_dhcps_config_refresh ();
}

/* API: Set Network. */
int
imi_dhcps_network_set (struct imi_dhcp_pool *pool,
                       char *network_str,
                       char *mask_str)
{
  int ret;
  struct prefix_ipv4 p_tmp;
  struct pal_in4_addr mask;
  struct imi_dhcp_pool *pool_exist;
  struct listnode *node;

  /* Validate & store network. */
  ret = str2prefix_ipv4 (network_str, &p_tmp);
  if (ret <= 0)
    return IMI_API_IPV4_ERROR;

  /* Cisco like mask notation. */
  if (mask_str)
    {
      ret = pal_inet_pton (AF_INET, mask_str, &mask);
      if (ret == 0)
        return IMI_API_IPV4_ERROR;
    }

  /* CSR-37470: In PacOS we exclude any overlaping networks.
     Overlaping networks make sense only if the subnetworks
     inherit network settings of supernetworks. In such case,
     IP address can only be allocated from a subnetwork.
   */
  if (listcount (DHCP->pool_list) > 0)
    {
      if (mask_str)
        p_tmp.prefixlen = ip_masklen (mask);

      apply_mask_ipv4 (&p_tmp);

      LIST_LOOP (DHCP->pool_list, pool_exist, node)
        {
          if (pool_exist->network_set)
            {
              /* Check the pool network already set. */
              if (pool == pool_exist)
                {
                  if (prefix_same((struct prefix *) &p_tmp,
                                  (struct prefix *) &pool_exist->network))
                    return IMI_API_SUCCESS;
                  else
                    return IMI_API_CANT_SET_ERROR;
                }
              else
                {
                  if (prefix_overlap((struct prefix *) &p_tmp,
                                     (struct prefix *) &pool_exist->network))
                    return IMI_API_DUPLICATE_ERROR;
                }
            }
        }
    }

  /* If here, network is valid so save it. */
  ret = str2prefix_ipv4 (network_str, &pool->network);
  if (ret <= 0)
    return -1;
  if (mask_str)
    pool->network.prefixlen = ip_masklen (mask);
  pool->network_set = PAL_TRUE;

  /* Apply mask. */
  apply_mask_ipv4 (&pool->network);

  return imi_dhcps_config_refresh ();
}

/* API: Unset Network. */
int
imi_dhcps_network_unset (struct imi_dhcp_pool *pool)
{
  if (!pool->network_set)
    return IMI_API_SUCCESS;
  else
    {
      pal_mem_set (&pool->network, 0, sizeof (struct prefix_ipv4));
      pool->network_set = PAL_FALSE;
    }

  return imi_dhcps_config_refresh ();
}

/* API: Show Network. */
int
imi_dhcps_network_show (struct cli *cli, struct imi_dhcp_pool *pool)
{
  char buf[IPV4_ADDR_STRLEN];

  pal_inet_ntoa (pool->network.prefix, buf);
  cli_out (cli, "  network: %s/%d\n", buf, pool->network.prefixlen);
  return IMI_API_SUCCESS;
}

/* API: Add range to pool. */
int
imi_dhcps_pool_range_add (struct imi_dhcp_pool *pool,
                          char *low_str,
                          char *high_str)
{
  struct listnode *node, *next;
  struct prefix_ipv4 low, high, low_tmp, high_tmp;
  struct prefix_ipv4 *p4;
  struct imi_dhcp_range *tmp, *new;
  int ret = IMI_API_SUCCESS;
  bool_t delete = PAL_FALSE;

  /* If network not set, return error. */
  if (! pool->network_set)
    return IMI_API_CANT_SET_ERROR;

  /* Change string(s) to prefix. */
  ret = str2prefix_ipv4 (low_str, &low);
  if (ret <= 0)
    return IMI_API_IPV4_ERROR;
  if (high_str)
    {
      ret = str2prefix_ipv4 (high_str, &high);
      if (ret <= 0)
        return IMI_API_IPV4_ERROR;
    }

  /* Out-of-range check. */
  pal_mem_cpy (&low_tmp, &low, sizeof (struct prefix_ipv4));
  pal_mem_cpy (&high_tmp, &high, sizeof (struct prefix_ipv4));

  low_tmp.prefixlen = pool->network.prefixlen;
  apply_mask_ipv4 (&low_tmp);
  if (! IPV4_ADDR_SAME (&low_tmp.prefix.s_addr,
                        &pool->network.prefix.s_addr))
    return IMI_API_OUT_OF_RANGE_ERROR;

  if (high_str)
    {
      high_tmp.prefixlen = pool->network.prefixlen;
      apply_mask_ipv4 (&high_tmp);
      if (! IPV4_ADDR_SAME (&high_tmp.prefix.s_addr,
                            &pool->network.prefix.s_addr))
        return IMI_API_OUT_OF_RANGE_ERROR;
    }

  /* Order check. */
  if (high_str)
    {
      if (IPV4_ADDR_CMP (&low.prefix.s_addr, &high.prefix.s_addr) > 0)
        return IMI_API_ORDER_ERROR;
    }

  /* Note: Consolidation of ranges can be confusing and lead to
         CLI configuration errors, especially for multiple
         consilidations.
         As such, we prevent user from adding a range that overlaps
         with another range, however, if range overlaps with a
         single address, we consolidate them. */

  /* Check for overlapping ranges. */
  if (listcount (pool->range_list) > 0)
    LIST_LOOP (pool->range_list, tmp, node)
      {
        if (high_str)
          {
            /* Check whether new range lies within existing range or
            matches existing range. */
            if ((IPV4_ADDR_CMP (&high.prefix.s_addr,
                                &tmp->low.prefix.s_addr) >= 0)
                && (IPV4_ADDR_CMP (&low.prefix.s_addr,
                                   &tmp->high.prefix.s_addr) <= 0))
              return IMI_API_DUPLICATE_ERROR;
          }
        else
          {
            /* If single address falls in this range, ignore the additon. */
            if ((IPV4_ADDR_CMP (&low.prefix.s_addr,
                                &tmp->low.prefix.s_addr) >= 0)
                && (IPV4_ADDR_CMP (&low.prefix.s_addr,
                                   &tmp->high.prefix.s_addr) <= 0))
              return IMI_API_SUCCESS;;
          }
      }

   /* Check for overlapping addresses. */
  if (listcount (pool->addr_list) > 0)
    for (node = LISTHEAD (pool->addr_list); node; )
      {
        delete = PAL_FALSE;
        p4 = GETDATA (node);
        next = node->next;

        /* Comparing range or address? */
        if (high_str)
          {
            /* If address falls within range, we can consolidate. */
            if ((IPV4_ADDR_CMP (&high.prefix.s_addr,
                                &p4->prefix.s_addr) >= 0)
                && (IPV4_ADDR_CMP (&low.prefix.s_addr,
                                   &p4->prefix.s_addr) <= 0))
              delete = PAL_TRUE;
          }
        else
          {
            /* If address already exists, just return success. */
            if (IPV4_ADDR_SAME (&low.prefix.s_addr, &p4->prefix.s_addr))
              return IMI_API_SUCCESS;
          }

        /* Delete this address?  (It will be added with the range). */
        if (delete)
          {
            prefix_ipv4_free (p4);
            list_delete_node (pool->addr_list, node);
            ret = IMI_API_CONSOLIDATED_WARNING;
          }
        node = next;
      }

  /* Add new range or address. */
  if (high_str)
    {
      /* Allocate list range holder. */
      new = XCALLOC (MTYPE_IMI_DHCP_RANGE, sizeof (struct imi_dhcp_range));
      pal_mem_set (new, 0, sizeof (struct imi_dhcp_range));

      /* Copy prefixes. */
      ret = str2prefix_ipv4 (low_str, &new->low);
      if (ret <= 0)
        {
          XFREE (MTYPE_IMI_DHCP_RANGE, new);
          return -1;
        }
      ret = str2prefix_ipv4 (high_str, &new->high);
      if (ret <= 0)
        {
          XFREE (MTYPE_IMI_DHCP_RANGE, new);
          return -1;
        }

      /* Add to list. */
      listnode_add (pool->range_list, new);
      ret = imi_dhcps_config_refresh ();
      if ((ret == IMI_API_SUCCESS) && delete)
        ret = IMI_API_CONSOLIDATED_WARNING;
    }
  else
    {
      /* Add to address list. */
      ret = imi_dhcps_ipv4_list_add (pool, low_str, pool->addr_list);
    }

  return ret;
}

/* API: Delete range from pool. */
int
imi_dhcps_pool_range_del (struct imi_dhcp_pool *pool,
                          char *low_str,
                          char *high_str)
{
  struct listnode *node, *next;
  struct prefix_ipv4 low, high;
  struct imi_dhcp_range *tmp;
  bool_t found = PAL_FALSE;
  int ret;

  /* Change string(s) to prefix. */
  ret = str2prefix_ipv4 (low_str, &low);
  if (ret <= 0)
    return IMI_API_IPV4_ERROR;

  /* See if exists & remove. */
  if (high_str)
    {
      /* Change string(s) to prefix. */
      ret = str2prefix_ipv4 (high_str, &high);
      if (ret <= 0)
        return IMI_API_IPV4_ERROR;

      if (IPV4_ADDR_CMP(&low.prefix.s_addr, &high.prefix.s_addr) > 0)
          return IMI_API_ORDER_ERROR;

      if (listcount (pool->range_list) > 0)
        for (node = LISTHEAD (pool->range_list); node; )
          {
            tmp = GETDATA (node);
            next = node->next;

            /* Compare range. */
            if ((IPV4_ADDR_SAME (&low.prefix.s_addr,
                                 &tmp->low.prefix.s_addr))
                && (IPV4_ADDR_SAME (&high.prefix.s_addr,
                                    &tmp->high.prefix.s_addr)))
              {
                XFREE (MTYPE_IMI_DHCP_RANGE, tmp);
                list_delete_node (pool->range_list, node);
                found = PAL_TRUE;
              }
            node = next;
          }
        if (found)
          ret = imi_dhcps_config_refresh ();
        else
          ret = (IMI_API_RANGE_NOT_FOUND_ERROR);
    }
  else
    {
      /* Call static function to remove from addr list. */
      ret = imi_dhcps_ipv4_list_del (low_str, pool->addr_list);
    }
  /* Impossible but must return something. */
  return (ret);
}

/* API: Flush all ranges. */
int
imi_dhcps_pool_range_flush (struct imi_dhcp_pool *pool)
{
  struct listnode *node, *next;
  struct prefix_ipv4 *p4;
  struct imi_dhcp_range *tmp;

  /* Delete all addresses. */
  if (listcount (pool->addr_list) > 0)
    for (node = LISTHEAD (pool->addr_list); node; )
      {
        p4 = GETDATA (node);
        next = node->next;
        prefix_ipv4_free (p4);
        list_delete_node (pool->addr_list, node);
        node = next;
      }

  /* Delete all ranges. */
  if (listcount (pool->range_list) > 0)
    for (node = LISTHEAD (pool->range_list); node; )
      {
        tmp = GETDATA (node);
        next = node->next;
        XFREE (MTYPE_IMI_DHCP_RANGE, tmp);
        list_delete_node (pool->range_list, node);
        node = next;
      }

  return imi_dhcps_config_refresh ();
}

/* API: Display range(s) to cli. */
int
imi_dhcps_pool_range_show (struct cli *cli, struct imi_dhcp_pool *pool)
{
  struct listnode *node;
  struct prefix_ipv4 *p4;
  struct imi_dhcp_range *tmp;
  int ret = IMI_API_SUCCESS;
  char buf_low[IPV4_ADDR_STRLEN];
  char buf_high[IPV4_ADDR_STRLEN];

  cli_out (cli, "  address range(s):\n");

  /* Display addresses. */
  if (listcount (pool->addr_list) > 0)
    {
      LIST_LOOP (pool->addr_list, p4, node)
        {
          pal_inet_ntoa (p4->prefix, buf_low);
          cli_out (cli, "    add: %s\n", buf_low);
        }
    }

  /* Display all ranges. */
  if (listcount (pool->range_list) > 0)
    {
      LIST_LOOP (pool->range_list, tmp, node)
        {
          pal_inet_ntoa (tmp->low.prefix, buf_low);
          pal_inet_ntoa (tmp->high.prefix, buf_high);
          cli_out (cli, "    add: %s to %s\n", buf_low, buf_high);
        }
    }

  return ret;
}

/* API: Set lease time in days/hrs/mins. */
int
imi_dhcps_lease_set (struct imi_dhcp_pool *pool,
                     char *num_days_str,
                     char *num_hrs_str,
                     char *num_mins_str)
{
  u_int8_t num_days = pal_atoi (num_days_str);
  u_int8_t num_hrs = pal_atoi (num_hrs_str);
  u_int8_t num_mins = pal_atoi (num_mins_str);

  /* Must be > 0 */
  if ((num_days == 0) && (num_hrs == 0) && (num_mins == 0))
    return IMI_API_INVALID_ARG_ERROR;

  pool->infinite = PAL_FALSE;
  pool->lease_seconds = ((num_days * 24 * 60 * 60) +
                         (num_hrs * 60 * 60) +
                         (num_mins * 60));
  pool->lease_minutes = num_mins;
  pool->lease_hours = num_hrs;
  pool->lease_days = num_days;

  return imi_dhcps_config_refresh ();
}

/* API: Set lease time infinite. */
int
imi_dhcps_lease_infinite_set (struct imi_dhcp_pool *pool)
{
  pool->infinite = PAL_TRUE;
  pool->lease_seconds = 0;
  pool->lease_minutes = 0;
  pool->lease_hours = 0;
  pool->lease_days = 0;
  return imi_dhcps_config_refresh ();
}

/* API: Unset lease time. */
int
imi_dhcps_lease_unset (struct imi_dhcp_pool *pool)
{
  pool->infinite = PAL_FALSE;
  pool->lease_seconds = 0;
  pool->lease_minutes = 0;
  pool->lease_hours = 0;
  pool->lease_days = 0;
  return imi_dhcps_config_refresh ();
}

/* API: Set Domain. */
int
imi_dhcps_domain_set (struct imi_dhcp_pool *pool,
                      char *domain_str)
{
  /* We accept only one default. */
  if (pool->domain_name != NULL)
    return IMI_API_DUPLICATE_ERROR;
  else
    pool->domain_name = XSTRDUP (MTYPE_IMI_STRING, domain_str);
  return imi_dhcps_config_refresh ();
}

/* API: Unset Domain. */
int
imi_dhcps_domain_unset (struct imi_dhcp_pool *pool)
{
  /* If no domain, just return. */
  if (pool->domain_name == NULL)
    return IMI_API_NOT_SET_ERROR;
  else
    {
      XFREE (MTYPE_IMI_STRING, pool->domain_name);
      pool->domain_name = NULL;
    }
  return imi_dhcps_config_refresh ();
}


/* API: Add DNS server to list. */
int
imi_dhcps_dns_add (struct imi_dhcp_pool *pool, char *dns_server)
{
  /* If network not set, return error. */
  if (!pool->network_set)
    return IMI_API_CANT_SET_ERROR;

  return imi_dhcps_ipv4_list_add (pool, dns_server, pool->dns_list);
}

/* API: Delete DNS server from list. */
int
imi_dhcps_dns_del (struct imi_dhcp_pool *pool, char *dns_server)
{
  return imi_dhcps_ipv4_list_del (dns_server, pool->dns_list);
}

/* API: Show DNS server list. */
int
imi_dhcps_dns_show (struct cli *cli, struct imi_dhcp_pool *pool)
{
  /* Show nameservers. */
  if (listcount (pool->dns_list) == 0)
    cli_out (cli, "  no dns-servers");
  else
    cli_out (cli, "  dns-server(s): ");

  /* Show list. */
  imi_dhcps_ipv4_list_show (cli, pool->dns_list);

  cli_out (cli, "\n");
  return CLI_SUCCESS;
}

/* API: Add default router to list. */
int
imi_dhcps_default_router_add (struct imi_dhcp_pool *pool, char *default_router)
{
  int ret;

  /* If network not set, return error. */
  if (!pool->network_set)
    return IMI_API_CANT_SET_ERROR;

  /* Add first to see if there are any error with the arg. */
  ret = imi_dhcps_ipv4_list_add (pool, default_router, pool->dr_list);
  if (ret != IMI_API_SUCCESS)
    return ret;

  /* Else, flush the list & re-add it; there can only be one. */
  if (listcount (pool->dr_list) > 1)
    {
      imi_dhcps_ipv4_list_flush (pool->dr_list);
      ret = imi_dhcps_ipv4_list_add (pool, default_router, pool->dr_list);
    }

  return ret;
}

/* API: Delete default router from list. */
int
imi_dhcps_default_router_del (struct imi_dhcp_pool *pool, char *default_router)
{
  return imi_dhcps_ipv4_list_del (default_router, pool->dr_list);
}

/* API: Show default router list. */
int
imi_dhcps_default_router_show (struct cli *cli, struct imi_dhcp_pool *pool)
{
  /* Show nameservers. */
  if (listcount (pool->dr_list) == 0)
    cli_out (cli, "  no default-routers");
  else
    cli_out (cli, "  default-router(s): ");

  /* Show list. */
  imi_dhcps_ipv4_list_show (cli, pool->dr_list);

  cli_out (cli, "\n");
  return CLI_SUCCESS;
}

void
imi_dhcps_pool_show (struct cli *cli, struct imi_dhcp_pool *pool)
{
  /* Sanity check. */
  if (! pool)
    return;

  cli_out (cli, "Pool %s :\n", pool->pool_id);

  imi_dhcps_network_show (cli, pool);

  imi_dhcps_pool_range_show (cli, pool);

  if (pool->infinite)
    cli_out (cli, "  lease: infinite\n");
  else
    cli_out (cli, "  lease <days:hours:minutes> <%d:%d:%d>\n",
             pool->lease_days, pool->lease_hours, pool->lease_minutes);

  if (pool->domain_name)
    cli_out (cli, "  domain: %s\n", pool->domain_name);
  else
    cli_out (cli, "  no domain is defined\n");

  imi_dhcps_dns_show (cli, pool);

  imi_dhcps_default_router_show (cli, pool);

  return;
}

/* API: Reset DHCP server configuration; delete all pools. */
result_t
imi_dhcps_reset ()
{
  struct listnode *node;
  struct imi_dhcp_pool *pool;

  /* Delete all existing pools. */
  if (listcount (DHCP->pool_list) > 0)
    for (node = LISTHEAD (DHCP->pool_list); node; )
      {
        pool = GETDATA (node);
        NEXTNODE (node);
        imi_dhcps_pool_delete (pool, DHCP->pool_list);
      }

  return RESULT_OK;
}

/* CLI functions. */

/* Enable DHCP Server. */
CLI (service_dhcp,
     service_dhcp_cli,
     "service dhcp",
     "Setup miscellaneous service",
     "Enable DHCP server")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_enable ();
  switch (ret)
    {
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }
  return result;
}

/* Disable DHCP Server. */
CLI (no_service_dhcp,
     no_service_dhcp_cli,
     "no service dhcp",
     CLI_NO_STR,
     "Setup miscellaneous service",
     "Enable DHCP server")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_disable ();
  switch (ret)
    {
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }
  return result;
}

/* Show current status of dhcp service. */
CLI (show_service_dhcp,
     show_service_dhcp_cli,
     "show service dhcp",
     CLI_SHOW_STR,
     "Setup miscellaneous service",
     "DHCP service")
{
  /* Call API. */
  imi_dhcps_state_show (cli);
  return CLI_SUCCESS;
}

/* Create DHCP address pool / enter address pool configuration. */
CLI (ip_dhcp_pool,
     ip_dhcp_pool_cli,
     "ip dhcp pool WORD",
     CLI_IP_STR,
     "DHCP Server configuration",
     "Configure DHCP server address pool",
     "Pool identifier")
{
  int ret;
  int result = CLI_ERROR;
  struct imi_dhcp_pool *pool;

  /* Are we reading the DHCP configuration via IMI?
     If yes: If system config is present, clear it. */
  if (cli->source == CLI_SOURCE_FILE)
    if (DHCP->sysconfig)
      {
        imi_dhcps_reset ();
        DHCP->sysconfig = PAL_FALSE;
      }

  if (pal_strlen(argv[0]) > IMI_DHCP_POOL_ID_LEN_MAX)
    {
      cli_out (cli, "%% Pool Id length exceeds 255 characters\n");
      return CLI_ERROR;
    }

  /* Call API. */
  ret = imi_dhcps_pool (argv[0]);
  switch (ret)
    {
    case IMI_API_SUCCESS:
      pool = imi_dhcps_pool_lookup (argv[0], DHCP->pool_list);
      cli->index = pool;
      cli->mode = DHCPS_MODE;
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }

  return result;
}

/* Delete DHCP address pool. */
CLI (no_ip_dhcp_pool,
     no_ip_dhcp_pool_cli,
     "no ip dhcp pool WORD",
     CLI_NO_STR,
     CLI_IP_STR,
     "DHCP Server configuration",
     "Configure DHCP server address pool",
     "Pool identifier")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_pool_del (argv[0]);
  switch (ret)
    {
    case IMI_API_INVALID_ARG_ERROR:
      cli_out (cli, "%% Invalid pool-id\n");
      break;
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }

  return result;
}

/*
   For different versions / implementations of the IMI CLI, different
   configuration commands can be used to configure the DHCP server.  The two
   alternatives are provided below:

   Option 1: Specify ranges
     * Simpler then option 2, both to develop and for inexperienced users
       to learn.
     * When DHCP server will be run on a signle interface, this method
       poses no limitations.
     * This can be accomplished by placing a 'range' command under
       DHCP configuration mode - router(config-dhcp)#.

   Option 2: Specify exclude
     * Conforms to industry CLI for larger routers.
     * May be benefits when DHCP server can run on multiple interfaces,
       or handle multiple (overlapping) networks on single interface.
     * This is accomplished using 'exclude' commands under Global
       Configuration mode.  In other words, the admin configures
       ranges or addresses to exclude globally, and then each DHCP
         pool ensures that it doesn't violate these exclusions. */


static void
imi_dhcp_print_range_error (struct cli *cli, int result)
{
  switch (result)
    {
    case IMI_API_CANT_SET_ERROR:
      cli_out (cli, "%% Please configure network/mask first.\n");
      break;
    case IMI_API_IPV4_ERROR:
      cli_out (cli, "%% Malformed address\n");
      break;
    case IMI_API_OUT_OF_RANGE_ERROR:
      cli_out (cli, "%% Range doesn't match network; please check it\n");
      break;
    case IMI_API_ORDER_ERROR:
      cli_out (cli, "%% Order must be <low-addr> <high-addr>\n");
      break;
    case IMI_API_DUPLICATE_ERROR:
      cli_out (cli, "%% Error: Range overlap; please check it.\n");
      break;
    case IMI_API_CONSOLIDATED_WARNING:
      cli_out (cli, "%% Warning: Existing address consolidated into range\n");
      result = CLI_SUCCESS;
      break;
    case IMI_API_INVALID_ARG_ERROR:
      cli_out (cli, "%% Invalid range \n");
      break;
    case IMI_API_RANGE_NOT_FOUND_ERROR:
      cli_out (cli, "%% Range not found \n");
      break;
    default:
      cli_out (cli, "%% Unknown error \n");
      break;
    }
}

CLI (ip_dhcp_include_addr,
     ip_dhcp_include_addr_cli,
     "range A.B.C.D",
     "Include address range",
     "Low address in range")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_pool_range_add (cli->index, argv[0], NULL);
  switch (ret)
    {
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      imi_dhcp_print_range_error (cli, ret);
      break;
    }

  return result;
}

CLI (no_ip_dhcp_include_addr,
     no_ip_dhcp_include_addr_cli,
     "no range A.B.C.D",
     CLI_NO_STR,
     "Include address range",
     "Low address in range")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_pool_range_del (cli->index, argv[0], NULL);
  switch (ret)
    {
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      imi_dhcp_print_range_error (cli, ret);
      break;
    }

  return result;
}

CLI (ip_dhcp_include_range,
     ip_dhcp_include_range_cli,
     "range A.B.C.D A.B.C.D",
     "Include address range",
     "Low address in range",
     "High address in range")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_pool_range_add (cli->index, argv[0], argv[1]);
  switch (ret)
    {
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      imi_dhcp_print_range_error (cli, ret);
      break;
    }

  return result;
}

CLI (no_ip_dhcp_include_range,
     no_ip_dhcp_include_range_cli,
     "no range A.B.C.D A.B.C.D",
     CLI_NO_STR,
     "Include address range",
     "Low address in range",
     "High address in range")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_pool_range_del (cli->index, argv[0], argv[1]);
  switch (ret)
    {
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      imi_dhcp_print_range_error (cli, ret);
      break;
    }

  return result;
}

CLI (no_ip_dhcp_range,
     no_ip_dhcp_range_cli,
     "no range",
     CLI_NO_STR,
     "Include address range")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_pool_range_flush (cli->index);
  switch (ret)
    {
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }

  return result;
}

/* Configure address pool network & mask. */
CLI (ip_dhcp_pool_network,
     ip_dhcp_pool_network_cli,
     "network A.B.C.D/M",
     "IP subnet network number and mask for this address pool",
     "IP subnet network number and mask (e.g. 10.0.0.0/8)")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_network_set (cli->index, argv[0], NULL);
  switch (ret)
    {
    case IMI_API_CANT_SET_ERROR:
      cli_out (cli, "%% Network already set\n");
      break;
    case IMI_API_IPV4_ERROR:
      cli_out (cli, "%% Malformed address\n");
      break;
    case IMI_API_DUPLICATE_ERROR:
      cli_out (cli, "%% Network matches with other DHCP pools\n");
      break;
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }
  return result;
}

CLI (ip_dhcp_pool_network_mask,
     ip_dhcp_pool_network_mask_cli,
     "network A.B.C.D A.B.C.D",
     "IP subnet network number and mask for this address pool",
     "IP subnet network number",
     "IP subnet network mask")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_network_set (cli->index, argv[0], argv[1]);
  switch (ret)
    {
    case IMI_API_CANT_SET_ERROR:
      cli_out (cli, "%% Network already set\n");
      break;
    case IMI_API_IPV4_ERROR:
      cli_out (cli, "%% Malformed address\n");
      break;
    case IMI_API_DUPLICATE_ERROR:
      cli_out (cli, "%% Network matches with other DHCP pools\n");
      break;
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }

  return result;
}

/* Delete address pool network & mask. */
CLI (no_ip_dhcp_pool_network,
     no_ip_dhcp_pool_network_cli,
     "no network",
     CLI_NO_STR,
     "IP subnet network number and mask for this address pool")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_network_unset (cli->index);
  switch (ret)
    {
    case IMI_API_CANT_SET_ERROR:
      cli_out (cli, "%% Please unset range, dns and default router first\n");
      break;
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }

  return result;
}


/* DHCP lease time commands. */
CLI (ip_dhcp_lease,
     ip_dhcp_lease_cli,
     "lease <0-30> <0-24> <0-60>",
     "Configure lease time",
     "DAYS",
     "HOURS",
     "MINUTES")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_lease_set (cli->index, argv[0], argv[1], argv[2]);
  switch (ret)
    {
    case IMI_API_INVALID_ARG_ERROR:
      cli_out (cli, "%% Lease must be at least one minute\n");
      break;
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }

  return result;
}

CLI (ip_dhcp_lease_infinite,
     ip_dhcp_lease_infinite_cli,
     "lease infinite",
     "Configure lease time",
     "Infinite lease")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_lease_infinite_set (cli->index);
  switch (ret)
    {
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }

  return result;
}

CLI (no_ip_dhcp_lease,
     no_ip_dhcp_lease_cli,
     "no lease",
     CLI_NO_STR,
     "Unset lease time")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_lease_unset (cli->index);
  switch (ret)
    {
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }

  return result;
}


/* Domain-name configuration. */
CLI (ip_dhcp_domain,
     ip_dhcp_domain_cli,
     "domain-name WORD",
     "Configure domain-name",
     "Domain name (e.g. engineering)")
{
  int ret;
  int result = CLI_ERROR;
  struct imi_dhcp_pool *pool = cli->index;

  if (pal_strlen(argv[0]) > IMI_DHCP_POOL_ID_LEN_MAX)    
    {      
      cli_out (cli, "%% Pool Id length exceeds 255 characters\n");      
      return CLI_ERROR; 
    }

  /* Call API. */
  ret = imi_dhcps_domain_set (cli->index, argv[0]);
  switch (ret)
    {
    case IMI_API_DUPLICATE_ERROR:
      cli_out (cli, "%% Domain already set to %s\n", pool->domain_name);
      break;
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }

  return result;
}

CLI (no_ip_dhcp_domain,
     no_ip_dhcp_domain_cli,
     "no domain-name",
     CLI_NO_STR,
     "Domain name (e.g. engineering)")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_domain_unset (cli->index);
  switch (ret)
    {
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }

  return result;
}

/* DNS server configuration. */
CLI (ip_dhcp_dns,
     ip_dhcp_dns_cli,
     "dns-server A.B.C.D",
     "Add a DNS server to the list",
     "DNS server address")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_dns_add (cli->index, argv[0]);
  switch (ret)
    {
    case IMI_API_CANT_SET_ERROR:
      cli_out (cli, "%% Please configure network/mask first.\n");
      break;
    case IMI_API_DUPLICATE_ERROR:
      cli_out (cli, "%% Error: domain already exists\n");
      break;
    case IMI_API_IPV4_ERROR:
      cli_out (cli, "%% Invalid nameserver address\n");
      break;
    case IMI_API_OUT_OF_RANGE_ERROR:
      cli_out (cli, "%% DNS server address network mismatch; please check it\n");
      break;
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }

  return result;
}

CLI (no_ip_dhcp_dns,
     no_ip_dhcp_dns_cli,
     "no dns-server A.B.C.D",
     CLI_NO_STR,
     "Add a DNS server to the list",
     "DNS server address")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_dns_del (cli->index, argv[0]);
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
      break;
    }

  return result;
}

/* Default router list. */
CLI (ip_dhcp_default_router,
     ip_dhcp_default_router_cli,
     "default-router A.B.C.D",
     "Add a default router to the list",
     "Default router address")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_default_router_add (cli->index, argv[0]);
  switch (ret)
    {
    case IMI_API_CANT_SET_ERROR:
      cli_out (cli, "%% Please configure network/mask first.\n");
      break;
    case IMI_API_DUPLICATE_ERROR:
      cli_out (cli, "%% Error: default router specified already exists.\n");
      break;
    case IMI_API_IPV4_ERROR:
      cli_out (cli, "%% Invalid nameserver address\n");
      break;
    case IMI_API_OUT_OF_RANGE_ERROR:
      cli_out (cli, "%% Router address network mismatch; please check it\n");
      break;
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }

  return result;
}

CLI (no_ip_dhcp_default_router,
     no_ip_dhcp_default_router_cli,
     "no default-router A.B.C.D",
     CLI_NO_STR,
     "Add a default router to the list",
     "Default router address")
{
  int ret;
  int result = CLI_ERROR;

  /* Call API. */
  ret = imi_dhcps_default_router_del (cli->index, argv[0]);
  switch (ret)
    {
    case IMI_API_INVALID_ARG_ERROR:
      cli_out (cli, "%% Default-router doesn't exist\n");
      break;
    case IMI_API_IPV4_ERROR:
      cli_out (cli, "%% Invalid default-router address\n");
      break;
    case IMI_API_SUCCESS:
      result = CLI_SUCCESS;
      break;
    default:
      break;
    }

  return result;
}

/* Show DHCP Pool. */
CLI (show_ip_dhcp_pool_name,
     show_ip_dhcp_pool_name_cli,
     "show ip dhcp pool WORD",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_DHCP_STR,
     "DHCP Address pool",
     "Pool identifier")
{
  struct imi_dhcp_pool *pool;

  /* Lookup pool. */
  pool = imi_dhcps_pool_lookup (argv[0], DHCP->pool_list);

  /* If pool doesn't exist, print notice & exit. */
  if (!pool)
    {
      cli_out (cli, "%% Invalid Pool Identifier\n");
      return CLI_ERROR;
    }

  imi_dhcps_pool_show (cli, pool);
  return CLI_SUCCESS;
}

/* Show All pools. */
CLI (show_ip_dhcp_pool,
     show_ip_dhcp_pool_cli,
     "show ip dhcp pool",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_DHCP_STR,
     "DHCP Address pool")
{
  struct imi_dhcp_pool *pool;
  struct listnode *node;

  /* Any defined? */
  if (listcount (DHCP->pool_list) <= 0)
    cli_out (cli, "no dhcp pools defined\n");
  else
    {
      /* Display all. */
      LIST_LOOP (DHCP->pool_list, pool, node)
        {
          cli_out (cli, "\n");
          imi_dhcps_pool_show (cli, pool);
        }
    }
  return CLI_SUCCESS;
}

CLI (show_ip_dhcp,
     show_ip_dhcp_cli,
     "show ip dhcp",
     CLI_SHOW_STR,
     CLI_IP_STR,
     CLI_DHCP_STR)
{
  struct imi_dhcp_pool *pool;
  struct listnode *node;

  /* enabled/disabled? */
  if (DHCP->enabled)
    cli_out (cli, "dhcp server enabled\n");
  else
    cli_out (cli, "dhcp server disabled\n");

  /* Valid pool ids. */
  cli_out (cli, "dhcp pool list:");
  if (listcount (DHCP->pool_list) <= 0)
    cli_out (cli, " <none>\n");
  else
    {
      LIST_LOOP (DHCP->pool_list, pool, node)
        {
          cli_out (cli, " %s", pool->pool_id);
        }
      cli_out (cli, "\n");
    }

  return CLI_SUCCESS;
}

/* Init / Shutdown.  */

/*----------------------------------------------------------------------
 * DHCP Service information CLI encoding/writing function.
 *----------------------------------------------------------------------
 */
int
imi_dhcps_service_encode (struct imi_dhcp *dhcp, cfg_vect_t *cv)
{
  if (DHCP->enabled == PAL_TRUE)
    cfg_vect_add_cmd(cv, "service dhcp\n");
  else
    cfg_vect_add_cmd(cv, "no service dhcp\n");
  return 0;
}

int
imi_dhcps_service_write (struct cli *cli)
{
  int write=0;

  cli->cv = cfg_vect_init(cli->cv);
  imi_dhcps_service_encode(DHCP, cli->cv);
  write = cfg_vect_count(cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
  return write;
}


/*----------------------------------------------------------------------
 * DHCP Configuration CLI encoding/writing function.
 *----------------------------------------------------------------------
 */
int
imi_dhcps_config_encode (struct imi_dhcp *dhcp, cfg_vect_t *cv)
{
  struct listnode *node;
  struct listnode *pool_node;
  struct prefix_ipv4 *p;
  struct imi_dhcp_range *r;
  char pref[IPV4_ADDR_STRLEN];
  char pref2[IPV4_ADDR_STRLEN];
  char mask[IPV4_ADDR_STRLEN];
  struct pal_in4_addr in4;
  struct imi_dhcp_pool *pool;

  /* Any pools defined? */
  if (listcount (DHCP->pool_list) > 0)
    {
      /* Write each pool. */
      LIST_LOOP (DHCP->pool_list, pool, pool_node)
        {
          /* Pool identifier. */
          cfg_vect_add_cmd (cv, "ip dhcp pool %s\n", pool->pool_id);

          /* Network. */
          if (pool->network_set)
            {
              /* Network(s). */
              masklen2ip (pool->network.prefixlen, &in4);
              pal_inet_ntoa (pool->network.prefix, pref);
              pal_inet_ntoa (in4, mask);
              cfg_vect_add_cmd (cv, " network %s %s\n", pref, mask);

              /* Range(s). */
              if (listcount (pool->addr_list) > 0)
                LIST_LOOP (pool->addr_list, p, node)
                  {
                    pal_inet_ntoa (p->prefix, pref);
                    cfg_vect_add_cmd (cv, " range %s\n", pref);
                  }
              if (listcount (pool->range_list) > 0)
                LIST_LOOP (pool->range_list, r, node)
                  {
                    pal_inet_ntoa (r->low.prefix, pref);
                    pal_inet_ntoa (r->high.prefix, pref2);
                    cfg_vect_add_cmd (cv, " range %s %s\n", pref, pref2);
                  }

              /* DNS server(s). */
              if (listcount (pool->dns_list) > 0)
                LIST_LOOP (pool->dns_list, p, node)
                  {
                    pal_inet_ntoa (p->prefix, pref);
                    cfg_vect_add_cmd (cv, " dns-server %s\n", pref);
                  }

              /* Default router(s). */
              if (listcount (pool->dr_list) > 0)
                LIST_LOOP (pool->dr_list, p, node)
                  {
                    pal_inet_ntoa (p->prefix, pref);
                    cfg_vect_add_cmd (cv, " default-router %s\n", pref);
                  }
            }

          /* Lease. */
          if (pool->infinite)
            cfg_vect_add_cmd (cv, " lease infinite\n");
          else
            {
              if (pool->lease_seconds > 0
                  && (pool->lease_days != IMI_DHCP_LEASE_DEFAULT_DAY
                      || pool->lease_hours != IMI_DHCP_LEASE_DEFAULT_HR
                      || pool->lease_minutes != IMI_DHCP_LEASE_DEFAULT_MIN))
                cfg_vect_add_cmd (cv, " lease %d %d %d\n", pool->lease_days,
                         pool->lease_hours, pool->lease_minutes);
            }

          /* Domain name. */
          if (pool->domain_name)
            cfg_vect_add_cmd (cv, " domain-name %s\n", pool->domain_name);

          cfg_vect_add_cmd (cv, "!\n");
        }
    }

  return 0;
}

int
imi_dhcps_config_write (struct cli *cli)
{
  cli->cv = cfg_vect_init(cli->cv);
  imi_dhcps_config_encode(DHCP, cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
  return 0;
}

/* Shutdown DHCP Server. */
void
imi_dhcps_shutdown ()
{
  struct listnode *node, *next;
  struct imi_dhcp_pool *pool;

  /* Set shutdown flag to notify PAL layer. */
  DHCP->shutdown_flag = PAL_TRUE;

  /* Free each of the pools. */
  if (listcount (DHCP->pool_list) > 0)
    for (node = LISTHEAD (DHCP->pool_list); node; )
      {
        pool = GETDATA (node);
        next = node->next;
        imi_dhcps_pool_delete (pool, DHCP->pool_list);
        node = next;
      }

  /* Free DHCP structure. */
  list_delete (DHCP->pool_list);

  /* Stop PAL. */
  pal_dhcp_stop (IMI->zg, DHCP->pal_dhcp);

  /* Free strcture. */
  XFREE (MTYPE_IMI_DHCP, DHCP);
}

void
imi_dhcps_init ()
{
  struct cli_tree *ctree = imim->ctree;

  /* Initialize DHCP data structure. */
  IMI->dhcp = XCALLOC (MTYPE_IMI_DHCP, sizeof (struct imi_dhcp));
  pal_mem_set (IMI->dhcp, 0, sizeof (struct imi_dhcp));
  DHCP->enabled = PAL_FALSE;
  DHCP->pool_list = list_new ();
  DHCP->sysconfig = PAL_FALSE;
  DHCP->shutdown_flag = PAL_FALSE;

  /* Initialize DHCP PAL. */
  DHCP->pal_dhcp = pal_dhcp_start (IMI->zg, DHCP);

  /* Install IMI default command for DHCPS_MODE.  */
  cli_install_default (ctree, DHCPS_MODE);

  /* Enable/disable DHCP server. */
  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &service_dhcp_cli);
  cli_set_imi_cmd(&service_dhcp_cli, CONFIG_MODE, CFG_DTYP_IMI_DHCPS_SERVICE);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_service_dhcp_cli);
  cli_set_imi_cmd(&no_service_dhcp_cli, CONFIG_MODE, CFG_DTYP_IMI_DHCPS_SERVICE);


  /* Create/delete address pool. */
  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_dhcp_pool_cli);
  cli_set_imi_cmd(&ip_dhcp_pool_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  cli_install_imi (ctree, CONFIG_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_dhcp_pool_cli);


  /* Address include commands. */
  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_dhcp_include_addr_cli);
  cli_set_imi_cmd(&ip_dhcp_include_addr_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_dhcp_include_addr_cli);
  cli_set_imi_cmd(&no_ip_dhcp_include_addr_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_dhcp_include_range_cli);
  cli_set_imi_cmd(&ip_dhcp_include_range_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_dhcp_include_range_cli);
  cli_set_imi_cmd(&no_ip_dhcp_include_range_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_dhcp_range_cli);
  cli_set_imi_cmd(&no_ip_dhcp_range_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  /* DHCP address pool network & mask */
  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_dhcp_pool_network_cli);
  cli_set_imi_cmd(&ip_dhcp_pool_network_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_dhcp_pool_network_mask_cli);
  cli_set_imi_cmd(&ip_dhcp_pool_network_mask_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_dhcp_pool_network_cli);
  cli_set_imi_cmd(&no_ip_dhcp_pool_network_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  /* DHCP lease command. */
  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_dhcp_lease_cli);
  cli_set_imi_cmd(&ip_dhcp_lease_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_dhcp_lease_infinite_cli);
  cli_set_imi_cmd(&ip_dhcp_lease_infinite_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_dhcp_lease_cli);
  cli_set_imi_cmd(&no_ip_dhcp_lease_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  /* Domain name. */
  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_dhcp_domain_cli);
  cli_set_imi_cmd(&ip_dhcp_domain_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_dhcp_domain_cli);
  cli_set_imi_cmd(&no_ip_dhcp_domain_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  /* DNS server. */
  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_dhcp_dns_cli);
  cli_set_imi_cmd(&ip_dhcp_dns_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_dhcp_dns_cli);
  cli_set_imi_cmd(&no_ip_dhcp_dns_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  /* Default router. */
  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &ip_dhcp_default_router_cli);
  cli_set_imi_cmd(&ip_dhcp_default_router_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  cli_install_imi (ctree, DHCPS_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &no_ip_dhcp_default_router_cli);
  cli_set_imi_cmd(&no_ip_dhcp_default_router_cli, DHCPS_MODE, CFG_DTYP_IMI_DHCPS);

  /* Show commands. */
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_service_dhcp_cli);
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ip_dhcp_cli);
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ip_dhcp_pool_cli);
  cli_install_imi (ctree, EXEC_MODE, PM_IMI, PRIVILEGE_NORMAL, 0,
                   &show_ip_dhcp_pool_name_cli);

  /* Default is disabled to minimize disruption when reading the IMI
     configuration file. */
  imi_dhcps_disable ();
}
#endif /* HAVE_DHCP_SERVER */
