/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#include "filter.h"
#include "snprintf.h"
#include "vty.h"
#ifdef HAVE_RESTART
#include "nsm_client.h"
#endif /* HAVE_RESTART */
#ifdef HAVE_BFD
#include "bfd_message.h"
#include "bfd_client.h"
#endif /* HAVE_BFD */

int
lib_vty_return (struct cli *cli, int ret)
{
  char *str = NULL;

  switch (ret)
    {
    case LIB_API_SET_SUCCESS:
      return CLI_SUCCESS;

    case LIB_API_SET_ERROR:
      str = "Invalid command";
      break;
    case LIB_API_SET_ERR_INVALID_VALUE:
      str = "Invalid argument value";
      break;
    case LIB_API_SET_ERR_MALFORMED_ADDRESS:
      str = "Malformed address";
      break;
    case LIB_API_SET_ERR_UNKNOWN_OBJECT:
      str = "This object doesn't exist";
      break;
    case LIB_API_SET_ERR_OBJECT_ALREADY_EXIST:
      str = "This object already exist";
      break;
    case LIB_API_SET_ERR_INVALID_FILTER_TYPE:
      str = "Type must be permit/deny";
      break;
    case LIB_API_SET_ERR_DUPLICATE_POLICY:
      str = "Duplicate policy -- Insertion failed";
      break;
    case LIB_API_SET_ERR_INVALID_PREFIX_RANGE:
      str = "Invalid prefix range -- make sure: len < ge-value <= le-value";
      break;
    case LIB_API_SET_ERR_RMAP_RULE_MISSING:
      str = "Can't find rule";
      break;
    case LIB_API_SET_ERR_RMAP_COMPILE_ERROR:
      str = "Argument is malformed";
      break;
    case LIB_API_SET_ERR_OOM:
      str = "Out-of-memory";
      break;
    case LIB_API_SET_ERR_EXCEED_LIMIT:
      str = "Limit exceeded";
      break;
    case LIB_API_SET_ERR_DIFF_ACL_TYPE:
      str = "An Access list with this name already exists";
      break;
#ifdef HAVE_PBR
    case LIB_API_SET_ERR_CANT_SET_SECONDARY_FIRST:
      str = "Can't set secondary nexthop first, set primary nexthop";
      break;
    case LIB_API_SET_ERR_CANT_DEL_PRIMARY:
      str = "Can't delete primary first, secondary nexthop exists,"
             " remove it and try ";
      break;
#endif /* HAVE_PBR */
    case LIB_API_SET_ERR_NEXTHOP_NOT_VALID:
      str = "Secondary nexthop configuration effects only when PBR enabled";
      break;
    default:
      str = "Unspecified error";
      break;
    }
      cli_out (cli, "%% %s\n", str);
      return CLI_ERROR;
}

/* Create the Standard access-list. */
int
access_list_standard_set (struct apn_vr *vr, char *name,
                          int direct, char *addr_str, char *mask_str)
{
  return filter_set_common (vr, name, direct,
                            addr_str, mask_str, NULL, NULL, 0, 1);
}

/* Delete the standard access-list. */
int
access_list_standard_unset (struct apn_vr *vr, char *name,
                            int direct, char *addr_str, char *mask_str)
{
  return filter_set_common (vr, name, direct,
                            addr_str, mask_str, NULL, NULL, 0, 0);
}

/* Create the extended access-list. */
int
access_list_extended_set (struct apn_vr *vr, char *name, int direct,
                          char *src_addr_str, char *src_mask_str,
                          char *dst_addr_str, char *dst_mask_str)
{
  return filter_set_common (vr, name, direct, src_addr_str, src_mask_str,
                            dst_addr_str, dst_mask_str, 1, 1);
}

/* Delete the extended access-list. */
int
access_list_extended_unset (struct apn_vr *vr, char *name, int direct,
                            char *src_addr_str, char *src_mask_str,
                            char *dst_addr_str, char *dst_mask_str)
{
  return filter_set_common (vr, name, direct, src_addr_str, src_mask_str,
                            dst_addr_str, dst_mask_str, 1, 0);
}

/* PacOS Style Access-list APIs: */

/* Create pacos style ipv4/ipv6 access-list. */
int
access_list_pacos_set (struct apn_vr *vr, char *name_str,
                       int direct, afi_t afi, char *prefix_str)
{
  return filter_set_pacos (vr, name_str, direct, afi, prefix_str, 0, 1);
}

/* Delete pacos style ipv4/ipv6 access-list. */
int
access_list_pacos_unset (struct apn_vr *vr, char *name_str,
                         int direct, afi_t afi, char *prefix_str)
{
  return filter_set_pacos (vr, name_str, direct, afi, prefix_str, 0, 0);
}

/* Set the pacos style ipv4/ipv6 access-list which
   has the "exact-match" option. */
int
access_list_pacos_exact_set (struct apn_vr *vr, char *name_str,
                             int direct, afi_t afi, char *prefix_str)
{
  return filter_set_pacos (vr, name_str, direct, afi, prefix_str, 1, 1);
}

/* Delete the pacos style ipv4/ipv6 access-list which
   has the "exact-match" option. */
int
access_list_pacos_exact_unset (struct apn_vr *vr, char *name_str,
                               int direct, afi_t afi, char *prefix_str)
{
  return filter_set_pacos (vr, name_str, direct, afi, prefix_str, 1, 0);
}

/* Delete the entire ipv4/ipv6 access-list by name. */
int
access_list_unset_by_name (struct apn_vr *vr, afi_t afi, char *name_str)
{
  struct access_list *access;
  struct access_master *master;
  struct filter_list *node;

  if (afi != AFI_IP
#ifdef HAVE_IPV6
      && afi != AFI_IP6
#endif /* HAVE_IPV6 */
     )
   return LIB_API_SET_ERR_INVALID_VALUE;

  /* Looking up access_list. */
  access = access_list_lookup (vr, afi, name_str);
  if (access == NULL)
    return LIB_API_SET_ERR_UNKNOWN_OBJECT;

  master = access->master;

  for (node = access->head; node; node = node->next)
    {
      /* Only for COMMON and PACOS filter (excl. PACOS_EXT) */
      if (node->common != FILTER_COMMON && node->common != FILTER_PACOS)
        return LIB_API_SET_ERR_ACL_FOUND_OTHER_FILTER;
            
      /* Run hook function for all filters. */
      if (master->delete_hook)
        (*master->delete_hook) (vr, access, node);
    }

  /* Delete all filters from access-list. */
  access_list_lock (access);
  access_list_delete (access);
  if (master->delete_hook)
    (*master->delete_hook) (vr, access, NULL);
  access_list_unlock (access);

  return LIB_API_SET_SUCCESS;
}

/* Add the remark string of an ipv4/ipv6 access-list. */
int
access_list_remark_set (struct apn_vr *vr, afi_t afi,
                        char *name_str, char *remark_str)
{
  struct access_list *access;

  if (afi != AFI_IP
#ifdef HAVE_IPV6
      && afi != AFI_IP6
#endif /* HAVE_IPV6 */
     )
    return LIB_API_SET_ERR_INVALID_VALUE;

  if (remark_str == NULL)
    return LIB_API_SET_ERR_INVALID_VALUE;

  access = access_list_get (vr, afi, name_str);

  if (access->remark)
    {
      XFREE (MTYPE_ACCESS_LIST_STR, access->remark);
      access->remark = NULL;
    }
  access->remark = XSTRDUP (MTYPE_ACCESS_LIST_STR, remark_str);
  return LIB_API_SET_SUCCESS;
}

/* Delete the remark string of an ipv4/ipv6 access-list. */
int
access_list_remark_unset (struct apn_vr *vr, afi_t afi, char *name)
{
  struct access_list *access;

  if (afi != AFI_IP
#ifdef HAVE_IPV6
      && afi != AFI_IP6
#endif /* HAVE_IPV6 */
     )
    return LIB_API_SET_ERR_INVALID_VALUE;

  access = access_list_lookup (vr, afi, name);
  if (! access)
    return LIB_API_SET_ERR_UNKNOWN_OBJECT;

  if (access->remark)
    {
      XFREE (MTYPE_ACCESS_LIST_STR, access->remark);
      access->remark = NULL;
    }

  if (access->head == NULL &&
      access->tail == NULL &&
      access->remark == NULL)
    access_list_delete (access);

  return LIB_API_SET_SUCCESS;
}

/* IPv4/IPv6 prefix-list API functions. */

/* Create the specified ipv4/ipv6 prefix-list entry. */
int
prefix_list_entry_set (struct apn_vr *vr, afi_t afi,
                       char *name, u_int32_t seqnum, int type,
                       char *prefix, u_int32_t genum, u_int32_t lenum)
{
  int ret;
  int any = 0;
  struct prefix p;

  pal_mem_set (&p, 0, sizeof (struct prefix));

  /* "any" is special token for matching any IPv4 addresses.  */
  if (afi == AFI_IP)
    {
      if (pal_strncmp ("any", prefix, pal_strlen (prefix)) == 0)
        {
          ret = str2prefix_ipv4 ("0.0.0.0/0", (struct prefix_ipv4 *) &p);
          genum = 0;
          lenum = IPV4_MAX_BITLEN;
          any = 1;
        }
      else
        ret = str2prefix_ipv4 (prefix, (struct prefix_ipv4 *) &p);

      if (ret <= 0)
        return LIB_API_SET_ERR_MALFORMED_ADDRESS;
    }
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    {
      if (pal_strncmp ("any", prefix, pal_strlen (prefix)) == 0)
        {
          ret = str2prefix_ipv6 ("::/0", (struct prefix_ipv6 *) &p);
          genum = 0;
          lenum = IPV6_MAX_BITLEN;
          any = 1;
        }
      else
        ret = str2prefix_ipv6 (prefix, (struct prefix_ipv6 *) &p);

      if (ret <= 0)
        return LIB_API_SET_ERR_MALFORMED_ADDRESS;
    }
#endif /* HAVE_IPV6 */
   else
      return LIB_API_SET_ERR_INVALID_VALUE;

  /* ge and le check. */
  if (genum && genum <= p.prefixlen)
    return LIB_API_SET_ERR_INVALID_PREFIX_RANGE;

  if (lenum && lenum <= p.prefixlen)
    return LIB_API_SET_ERR_INVALID_PREFIX_RANGE;

  if (lenum && genum > lenum)
    return LIB_API_SET_ERR_INVALID_PREFIX_RANGE;

  if (genum && lenum == (afi == AFI_IP ? 32 : 128))
    lenum = 0;

  return prefix_list_install (vr, afi, name, type, &p,
                              seqnum, genum, lenum, any);
}

/* Delete the entire prefix-list according prefix-list name. */
int
prefix_list_unset (struct apn_vr *vr, afi_t afi, char *name)
{
  struct prefix_list *plist;

  if ( afi != AFI_IP
#ifdef HAVE_IPV6
       && afi != AFI_IP6
#endif /* HAVE_IPV6 */
     )
    return LIB_API_SET_ERR_INVALID_VALUE;

  /* Check prefix list exist or not. */
  plist = prefix_list_lookup (vr, afi, name);
  if (! plist)
    return LIB_API_SET_ERR_UNKNOWN_OBJECT;

  /* delete the entire prefix-list. */
  prefix_list_delete (plist);

  return LIB_API_SET_SUCCESS;
}

/* Delete the specified ipv4/ipv6 prefix-list entry. */
int
prefix_list_entry_unset (struct apn_vr *vr, afi_t afi,
                         char *name, u_int32_t seqnum, int type,
                         char *prefix, u_int32_t genum, u_int32_t lenum)
{
  struct prefix p;
  int ret;

  if (seqnum == 0 && prefix == NULL &&
      genum == 0 && lenum == 0)
    return prefix_list_unset (vr, afi, name);

  if (prefix == NULL)
    return LIB_API_SET_ERR_MALFORMED_ADDRESS;

  /* "any" is special token for matching any IPv4 addresses.  */
  if (afi == AFI_IP)
    {
      if (pal_strncmp ("any", prefix, pal_strlen (prefix)) == 0)
        {
          ret = str2prefix_ipv4 ("0.0.0.0/0", (struct prefix_ipv4 *) &p);
          genum = 0;
          lenum = IPV4_MAX_BITLEN;
        }
      else
        ret = str2prefix_ipv4 (prefix, (struct prefix_ipv4 *) &p);

      if (ret <= 0)
        return LIB_API_SET_ERR_MALFORMED_ADDRESS;
    }
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    {
      if (pal_strncmp ("any", prefix, pal_strlen (prefix)) == 0)
        {
          ret = str2prefix_ipv6 ("::/0", (struct prefix_ipv6 *) &p);
          genum = 0;
          lenum = IPV6_MAX_BITLEN;
        }
      else
        ret = str2prefix_ipv6 (prefix, (struct prefix_ipv6 *) &p);

      if (ret <= 0)
        return LIB_API_SET_ERR_MALFORMED_ADDRESS;
    }
#endif /* HAVE_IPV6 */
  else
    return LIB_API_SET_ERR_INVALID_VALUE;

  return prefix_list_uninstall (vr, afi, name, &p, type, seqnum, genum, lenum);
}

/* Set ipv4/ipv6 prefix-list's include/exclude sequence-number. */
int
prefix_list_sequence_number_set (struct apn_vr *vr, afi_t afi)
{
  if (afi == AFI_IP)
    vr->prefix_master_ipv4.seqnum = 1;
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    vr->prefix_master_ipv6.seqnum = 1;
#endif /* HAVE_IPV6 */
  else
    return LIB_API_SET_ERR_INVALID_VALUE;

  return LIB_API_SET_SUCCESS;
}

/* Unset ipv4/ipv6 prefix-list's include/exclude sequence-number. */
int
prefix_list_sequence_number_unset (struct apn_vr *vr, afi_t afi)
{
  if (afi == AFI_IP)
    vr->prefix_master_ipv4.seqnum = 0;
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    vr->prefix_master_ipv6.seqnum = 0;
#endif /* HAVE_IPV6 */
  else
    return LIB_API_SET_ERR_INVALID_VALUE;

  return LIB_API_SET_SUCCESS;
}

/* Set ipv4/ipv6 prefix-list's specific description. */
int
prefix_list_description_set (struct apn_vr *vr, afi_t afi, char *name,
                             char *description)
{
  struct prefix_list *plist;

  if (afi != AFI_IP
#ifdef HAVE_IPV6
      && afi != AFI_IP6
#endif /* HAVE_IPV6 */
     )
   return LIB_API_SET_ERR_INVALID_VALUE;

  plist = prefix_list_get (vr, afi, name);
  if (plist->desc)
    {
      XFREE (MTYPE_PREFIX_LIST_DESC, plist->desc);
      plist->desc = NULL;
    }

  if (description)
    plist->desc = XSTRDUP (MTYPE_PREFIX_LIST_DESC, description);

  return LIB_API_SET_SUCCESS;
}

/* Unset ipv4/ipv6 prefix-list's specific description. */
int
prefix_list_description_unset (struct apn_vr *vr, afi_t afi, char *name)
{
  struct prefix_list *plist;

  if (afi != AFI_IP
#ifdef HAVE_IPV6
      && afi != AFI_IP6
#endif /* HAVE_IPV6 */
     )
    return LIB_API_SET_ERR_INVALID_VALUE;

  plist = prefix_list_lookup (vr, afi, name);
  if (! plist)
    return LIB_API_SET_ERR_UNKNOWN_OBJECT;

  if (plist->desc)
    {
      XFREE (MTYPE_PREFIX_LIST_DESC, plist->desc);
      plist->desc = NULL;
    }

  if (plist->head == NULL && plist->tail == NULL
      && plist->desc == NULL)
    prefix_list_delete (plist);

  return LIB_API_SET_SUCCESS;
}


/* CLI-APIs for route-map. */
int
route_map_index_set (struct apn_vr *vr, char *name, int permit, int pref)
{
  struct route_map_index *index = NULL;

  if (permit != LIB_API_RMAP_DENY
      && permit != LIB_API_RMAP_PERMIT )
    return LIB_API_SET_ERR_INVALID_FILTER_TYPE;

  if (pref < 1 || pref > 65535)
    return LIB_API_SET_ERR_INVALID_VALUE;

  index = route_map_index_install (vr, name, permit, pref);

  if (! index)
    return LIB_API_SET_ERROR; 

  return LIB_API_SET_SUCCESS;
}

/* Delete the specified route map index. */
int
route_map_index_unset (struct apn_vr *vr, char *name, int permit, int pref)
{
  if (permit != LIB_API_RMAP_DENY
      && permit != LIB_API_RMAP_PERMIT)
    return LIB_API_SET_ERR_INVALID_FILTER_TYPE;

  if (pref == 0 || pref > 65535)
    return LIB_API_SET_ERR_INVALID_VALUE;

  return route_map_index_uninstall (vr, name, permit, pref);
}

/* Delete entire route-map by name. */
int
route_map_unset (struct apn_vr *vr, char *name)
{
  return route_map_uninstall (vr, name);
}


int
route_map_match_interface_set (struct apn_vr *vr, char *name,
                               int permit, int pref, char *arg)
{
  struct interface *ifp;

  ifp = if_lookup_by_name (&vr->ifm, arg);
  if (!ifp)
    return LIB_API_SET_ERR_INVALID_VALUE;
  else
    return route_map_match_set (vr, name, permit, pref, "interface", arg);
}

int
route_map_match_interface_unset (struct apn_vr *vr, char *name,
                                 int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref, "interface", arg);
}

int
route_map_match_metric_set (struct apn_vr *vr, char *name,
                            int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref, "metric", arg);
}

int
route_map_match_metric_unset (struct apn_vr *vr, char *name,
                              int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref, "metric", arg);
}

int
route_map_set_metric_set (struct apn_vr *vr, char *name,
                          int permit, int pref, char *arg)
{
  int flag = 0;

  return route_map_set_set (vr, name, permit, pref, "metric", arg, flag);
}

int
route_map_set_metric_unset (struct apn_vr *vr, char *name,
                            int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "metric", arg);
}


int
route_map_match_ip_address_set (struct apn_vr *vr, char *name,
                                int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref, "ip address", arg);
}

int
route_map_match_ip_address_unset (struct apn_vr *vr, char *name,
                                  int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref, "ip address", arg);
}

int
route_map_match_ip_address_prefix_list_set (struct apn_vr *vr, char *name,
                                            int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref,
                              "ip address prefix-list", arg);
}

int
route_map_match_ip_address_prefix_list_unset (struct apn_vr *vr, char *name,
                                              int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref,
                                "ip address prefix-list", arg);
}

int
route_map_match_ip_peer_set (struct apn_vr *vr, char *name,
                             int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref, "ip peer", arg);
}

int 
route_map_match_ip_peer_unset (struct apn_vr *vr, char *name,
                               int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref, "ip peer", arg);
}

int
route_map_match_ipv6_peer_set (struct apn_vr *vr, char *name,
                             int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref, "ipv6 peer", arg);
}

int 
route_map_match_ipv6_peer_unset (struct apn_vr *vr, char *name,
                               int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref, "ipv6 peer", arg);
}

int
route_map_match_ip_nexthop_set (struct apn_vr *vr, char *name,
                                int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref, "ip next-hop", arg);
}

int
route_map_match_ip_nexthop_unset (struct apn_vr *vr, char *name,
                                  int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref, "ip next-hop", arg);
}

int
route_map_match_ip_nexthop_prefix_list_set (struct apn_vr *vr, char *name,
                                            int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref,
                              "ip next-hop prefix-list", arg);
}

int
route_map_match_ip_nexthop_prefix_list_unset (struct apn_vr *vr, char *name,
                                              int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref,
                                "ip next-hop prefix-list", arg);
}

int
route_map_set_ip_nexthop_set (struct apn_vr *vr, char *name,
                              int permit, int pref, char *arg, 
                              s_int16_t nh_type, char *ifname)
{
  int flag = 0;

  return route_map_set_set_nexthop (vr, name, permit, pref, "ip next-hop", arg, 
                                    nh_type, ifname, flag);
}

int
route_map_set_ip_nexthop_unset (struct apn_vr *vr, char *name,
                                int permit, int pref, char *arg,
                                s_int16_t nh_type, char *ifname)
{
  return route_map_set_unset_nexthop (vr, name, permit, pref, "ip next-hop", 
                                      arg, nh_type, ifname);
}

int
route_map_set_ip_peer_set (struct apn_vr *vr, char *name,
                           int permit, int pref, char *arg)
{
  int flag = 0;
  return route_map_set_set (vr, name, permit, pref, "ip peer", arg, flag);
}

int
route_map_set_ip_peer_unset (struct apn_vr *vr, char *name,
                             int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "ip peer", arg);
}
        

int
route_map_set_ipv6_peer_set (struct apn_vr *vr, char *name,
                           int permit, int pref, char *arg)
{
  int flag = 0;
  return route_map_set_set (vr, name, permit, pref, "ipv6 peer", arg, flag);
}

int
route_map_set_ipv6_peer_unset (struct apn_vr *vr, char *name,
                             int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "ipv6 peer", arg);
}

#ifdef HAVE_IPV6
int
route_map_match_ipv6_address_set (struct apn_vr *vr, char *name,
                                  int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref, "ipv6 address", arg);
}

int
route_map_match_ipv6_address_unset (struct apn_vr *vr, char *name,
                                    int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref, "ipv6 address", arg);
}

int
route_map_match_ipv6_address_prefix_list_set (struct apn_vr *vr, char *name,
                                              int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref,
                              "ipv6 address prefix-list", arg);
}

int
route_map_match_ipv6_address_prefix_list_unset (struct apn_vr *vr, char *name,
                                                int permit, int pref,
                                                char *arg)
{
  return route_map_match_unset (vr, name, permit, pref,
                                "ipv6 address prefix-list", arg);
}

int
route_map_match_ipv6_nexthop_set (struct apn_vr *vr, char *name,
                                  int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref, "ipv6 next-hop", arg);
}

int
route_map_match_ipv6_nexthop_unset (struct apn_vr *vr, char *name,
                                    int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref, "ipv6 next-hop", arg);
}

int
route_map_match_ipv6_nexthop_prefix_list_set (struct apn_vr *vr, char *name,
                                              int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref,
                              "ipv6 next-hop prefix-list", arg);
}

int
route_map_match_ipv6_nexthop_prefix_list_unset (struct apn_vr *vr, char *name,
                                                int permit, int pref,
                                                char *arg)
{
  return route_map_match_unset (vr, name, permit, pref,
                                "ipv6 next-hop prefix-list", arg);
}

int
route_map_set_ipv6_nexthop_set (struct apn_vr *vr, char *name,
                                int permit, int pref, char *arg)
{
  int flag = 0;

  return route_map_set_set (vr, name, permit, pref,
                            "ipv6 next-hop", arg, flag);
}

int
route_map_set_ipv6_nexthop_unset (struct apn_vr *vr, char *name,
                                  int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref,
                              "ipv6 next-hop", arg);
}

int
route_map_set_ipv6_nexthop_local_set (struct apn_vr *vr, char *name,
                                      int permit, int pref, char *arg)
{
  int flag = 0;

  return route_map_set_set (vr, name, permit, pref,
                            "ipv6 next-hop local", arg, flag);
}

int
route_map_set_ipv6_nexthop_local_unset (struct apn_vr *vr, char *name,
                                        int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref,
                              "ipv6 next-hop local", arg);
}
#endif /* HAVE_IPV6 */


int
route_map_set_vpnv4_nexthop_set (struct apn_vr *vr, char *name,
                                 int permit, int pref, char *arg)
{
  int flag = 0;

  return route_map_set_set (vr, name, permit, pref,
                            "vpnv4 next-hop", arg, flag);
}

int
route_map_set_vpnv4_nexthop_unset (struct apn_vr *vr, char *name,
                                   int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "vpnv4 next-hop", arg);
}


int
route_map_match_tag_set (struct apn_vr *vr, char *name,
                         int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref, "tag", arg);
}

int
route_map_match_tag_unset (struct apn_vr *vr, char *name,
                           int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref, "tag", arg);
}

int
route_map_match_route_type_set (struct apn_vr *vr, char *name,
                                int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref,
                              "route-type external", arg);
}

int
route_map_match_route_type_unset (struct apn_vr *vr, char *name,
                                  int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref,
                                "route-type external", arg);
}

int
route_map_set_tag_set (struct apn_vr *vr, char *name,
                       int permit, int pref, char *arg)
{
  int flag = 0;

  return route_map_set_set (vr, name, permit, pref, "tag", arg, flag);
}

int
route_map_set_tag_unset (struct apn_vr *vr, char *name,
                         int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "tag", arg);
}

int
route_map_set_metric_type_set (struct apn_vr *vr, char *name,
                               int permit, int pref, char *arg)
{
  int flag = 0;

  return route_map_set_set (vr, name, permit, pref, "metric-type", arg, flag);
}

int
route_map_set_metric_type_unset (struct apn_vr *vr, char *name,
                                 int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "metric-type", arg);
}

int
route_map_set_level_set (struct apn_vr *vr, char *name,
                         int permit, int pref, char *arg)
{
  int flag = 0;
  return route_map_set_set (vr, name, permit, pref, "level", arg, flag);
}

int
route_map_set_level_unset (struct apn_vr *vr, char *name,
                           int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "level", arg);
}

int
route_map_match_as_path_set (struct apn_vr *vr, char *name,
                             int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref, "as-path", arg);
}

int
route_map_match_as_path_unset (struct apn_vr *vr, char *name,
                               int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref, "as-path", arg);
}

int
route_map_match_origin_set (struct apn_vr *vr, char *name,
                            int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref, "origin", arg);
}

int
route_map_match_origin_unset (struct apn_vr *vr, char *name,
                              int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref, "origin", arg);
}

int
route_map_match_community_set (struct apn_vr *vr, char *name,
                               int permit, int pref, char *arg)
{
  return route_map_match_set (vr, name, permit, pref, "community", arg);
}

int
route_map_match_community_unset (struct apn_vr *vr, char *name,
                                 int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref, "community", arg);
}


int
route_map_match_ecommunity_set (struct apn_vr *vr, char *name,
                               int permit, int pref, char *arg)
{ 
  return route_map_match_set (vr, name, permit, pref, "extcommunity", arg);
}

int
route_map_match_ecommunity_unset (struct apn_vr *vr, char *name,
                                 int permit, int pref, char *arg)
{
  return route_map_match_unset (vr, name, permit, pref, "extcommunity", arg);
}

int
route_map_set_as_path_prepend_set (struct apn_vr *vr, char *name,
                                   int permit, int pref, char *arg)
{
  int flag = 0;

  return route_map_set_set (vr, name, permit, pref,
                            "as-path prepend", arg, flag);
}

int
route_map_set_as_path_prepend_unset (struct apn_vr *vr, char *name,
                                     int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "as-path prepend", arg);
}

int
route_map_set_origin_set (struct apn_vr *vr, char *name,
                          int permit, int pref, char *arg)
{
  int flag = 0;

  return route_map_set_set (vr, name, permit, pref, "origin", arg, flag);
}

int
route_map_set_origin_unset (struct apn_vr *vr, char *name,
                            int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "origin", arg);
}

int
route_map_set_local_preference_set (struct apn_vr *vr, char *name,
                                    int permit, int pref, char *arg)
{
  int flag = 0;

  return route_map_set_set (vr, name, permit, pref,
                            "local-preference", arg, flag);
}

int
route_map_set_local_preference_unset (struct apn_vr *vr, char *name,
                                      int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "local-preference", arg);
}

int
route_map_set_weight_set (struct apn_vr *vr, char *name,
                          int permit, int pref, char *arg)
{
  int flag = 0;

  return route_map_set_set (vr, name, permit, pref, "weight", arg, flag);
}

int
route_map_set_weight_unset (struct apn_vr *vr, char *name,
                            int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "weight", arg);
}

int
route_map_set_atomic_aggregate_set (struct apn_vr *vr, char *name,
                                    int permit, int pref)
{
  int flag = 0;
  char *arg = NULL;

  return route_map_set_set (vr, name, permit, pref,
                            "atomic-aggregate", arg, flag);
}

int
route_map_set_atomic_aggregate_unset (struct apn_vr *vr, char *name,
                                      int permit, int pref)
{
  char *arg = NULL;

  return route_map_set_unset (vr, name, permit, pref, "atomic-aggregate", arg);
}

int
route_map_set_aggregator_as_set (struct apn_vr *vr, char *name,
                                 int permit, int pref, char *arg)
{
  int flag = 0;

  return route_map_set_set (vr, name, permit, pref,
                            "aggregator as", arg, flag);
}

int
route_map_set_aggregator_as_unset (struct apn_vr *vr, char *name,
                                   int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "aggregator as", arg);
}

int
route_map_set_originator_id_set (struct apn_vr *vr, char *name,
                                 int permit, int pref, char *arg)
{
  int flag = 0;

  return route_map_set_set (vr, name, permit, pref,
                            "originator-id", arg, flag);
}

int
route_map_set_originator_id_unset (struct apn_vr *vr, char *name,
                                   int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "originator-id", arg);
}

int
route_map_set_community_delete_set (struct apn_vr *vr, char *name,
                                    int permit, int pref, char *arg)
{
  int ret;
  int len;
  char *str;
  int flag = ROUTE_MAP_FLAG_PRIORITY;

  len = pal_strlen (arg) + pal_strlen (" delete") + 1;
  str = XCALLOC (MTYPE_TMP, len);
  zsnprintf (str, len, "%s delete", arg);

  ret = route_map_set_set (vr, name, permit, pref, "comm-list", str, flag);

  XFREE (MTYPE_TMP, str);

  return ret;
}

int
route_map_set_community_delete_unset (struct apn_vr *vr, char *name,
                                      int permit, int pref, char *arg)
{
  int ret;
  int len;
  char *str;

  len = pal_strlen (arg) + pal_strlen (" delete") + 1;
  str = XCALLOC (MTYPE_TMP, len);
  zsnprintf (str, len, "%s delete", arg);

  ret = route_map_set_unset (vr, name, permit, pref, "comm-list", str);

  XFREE (MTYPE_TMP, str);

  return ret;
}

int
route_map_set_community_set (struct apn_vr *vr, char *name,
                             int permit, int pref, char *arg, int additive)
{
  return route_map_set_set (vr, name, permit, pref, "community", arg,
                            additive ? ROUTE_MAP_FLAG_ADDITIVE : 0);
}

int
route_map_set_community_unset (struct apn_vr *vr, char *name,
                               int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "community", arg);
}

int
route_map_set_ext_community_rt_set (struct apn_vr *vr, char *name,
                                    int permit, int pref, char *arg)
{
  int flag = 0;

  return route_map_set_set (vr, name, permit, pref,
                            "extcommunity rt", arg, flag);
}

int
route_map_set_ext_community_rt_unset (struct apn_vr *vr, char *name,
                                      int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "extcommunity rt", arg);
}

int
route_map_set_ext_community_soo_set (struct apn_vr *vr, char *name,
                                     int permit, int pref, char *arg)
{
  int flag = 0;

  return route_map_set_set (vr, name, permit, pref,
                            "extcommunity soo", arg, flag);
}

int
route_map_set_ext_community_soo_unset (struct apn_vr *vr, char *name,
                                       int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "extcommunity soo", arg);
}

int
route_map_set_dampening_set (struct apn_vr *vr, char *name,
                             int permit, int pref, char *arg)
{
  int flag = 0;

  return route_map_set_set (vr, name, permit, pref, "dampening", arg, flag);
}

int
route_map_set_dampening_unset (struct apn_vr *vr, char *name,
                               int permit, int pref, char *arg)
{
  return route_map_set_unset (vr, name, permit, pref, "dampening", arg);
}

int
lib_api_client_send_preserve (struct lib_globals *zg,
			      u_int32_t preserve_time)
{
  int ret = LIB_API_SUCCESS;

#ifdef HAVE_RESTART
  if (zg->nc)
    {
      ret = nsm_client_send_preserve (zg->nc, preserve_time);
      if (ret < 0)
	return ret;
    }
#endif /* HAVE_RESTART */

#ifdef HAVE_BFD
  if (zg->bc != NULL)
    {
      ret = bfd_client_send_preserve (zg->bc, preserve_time);
      if (ret < 0)
	return ret;
    }
#endif /* HAVE_BFD */

  return ret;
}

int
lib_api_client_send_preserve_with_val (struct lib_globals *zg,
				       u_int32_t preserve_time,
				       u_char *restart_val,
				       u_int16_t restart_length)
{
  int ret = LIB_API_SUCCESS;

#ifdef HAVE_RESTART
  if (zg->nc)
    {
      ret = nsm_client_send_preserve_with_val (zg->nc, preserve_time,
					       restart_val, restart_length);
      if (ret < 0)
	return ret;
    }
#endif /* HAVE_RESTART */

#ifdef HAVE_BFD
  if (zg->bc != NULL)
    {
      ret = bfd_client_send_preserve (zg->bc, preserve_time);
      if (ret < 0)
	return ret;
    }
#endif /* HAVE_BFD */

  return ret;
}

int
lib_api_client_send_stale_remove (struct lib_globals *zg,
				  afi_t afi, safi_t safi)
{
  int ret = LIB_API_SUCCESS;

#ifdef HAVE_RESTART
  if (zg->nc != NULL)
    {
      ret = nsm_client_send_stale_remove (zg->nc, afi, safi);
      if (ret < 0)
	return ret;
    }
#endif /* HAVE_RESTART */

#ifdef HAVE_BFD
  if (zg->bc != NULL)
    {
      ret = bfd_client_send_stale_remove (zg->bc);
      if (ret < 0)
	return ret;
    }
#endif /* HAVE_BFD */

  return ret; 
}
