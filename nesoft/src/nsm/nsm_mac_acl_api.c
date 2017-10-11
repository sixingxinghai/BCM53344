#include "pal.h"
#ifdef HAVE_L2LERN
#include "lib.h"
#ifdef HAVE_HAL
#include "hal_incl.h"
#endif /* HAVE_HAL */

#include "table.h"
#include "nsmd.h"
#include "avl_tree.h"
#include "nsm_message.h"
#include "nsm_server.h"
#include "nsm_interface.h"
#include "nsm_acl.h"
#include "nsm_mac_acl_cli.h"
#include "nsm_mac_acl_api.h"
#include "hal_acl.h"
#ifdef HAVE_VLAN
#include "nsm_vlan.h"
#endif

/* Return string of filter_type. */
static const char *
mac_acl_type_str (struct mac_acl *access)
{
  switch (access->type)
    {
    case ACL_FILTER_PERMIT:
      return "permit";
    case ACL_FILTER_DENY:
      return "deny";
    default:
      return "";
    }
}

int
compare_mac_filter (u_int8_t *start_addr, u_int8_t val)
{
  int i;

  for (i=0 ; i < 6 ; i++ )
    {
      if ( *start_addr++ != val )
        return 1;
    }
  return 0;
}


#ifdef HAVE_VLAN
int
check_interface_vlan_filter ( struct interface *ifp)
{
  struct nsm_if *zif;
  struct avl_node *rn;
  struct nsm_vlan *vlan;
  struct nsm_bridge *bridge;
  struct nsm_bridge_port *br_port;

  zif = (struct nsm_if *)ifp->info;

  if (! zif
      || ! zif->switchport)
    return 1;

  br_port = zif->switchport;

  bridge = br_port->bridge;

  if (! bridge)
    return 0;

  if ( (bridge->type != NSM_BRIDGE_TYPE_STP_VLANAWARE) &&
       (bridge->type != NSM_BRIDGE_TYPE_RSTP_VLANAWARE) &&
       (bridge->type != NSM_BRIDGE_TYPE_RPVST_PLUS) &&
       (bridge->type != NSM_BRIDGE_TYPE_MSTP) )
    {
      return 0;
    }

  if ( bridge->vlan_table != NULL)
    for (rn = avl_top (bridge->vlan_table); rn; rn = avl_next (rn))
     {
       if (rn)
         {
           vlan = rn->info;
           if (! vlan)
             continue;
           if ( vlan->vlan_filter)
             {
               //route_unlock_node (rn);
               return 1;
             }
           //route_unlock_node (rn);
         }
     }
  return 0;
}
#endif

int
nsm_mac_hal_set_access_grp(struct mac_acl *access,
                           int ifindex,
                           int action,
                           int dir)
{
  struct hal_mac_access_grp hal_macc_grp;

  unsigned char *dst_mac_addr;
  unsigned char *src_mac_addr;
  unsigned char *deny_permit;
  unsigned char *packet_format;
  char *src_name;
  char *dst_name;
  int ret;


  /* Clear hal_cmap */
  memset (&hal_macc_grp, 0, sizeof (struct hal_mac_access_grp));

  deny_permit = &hal_macc_grp.deny_permit;
  *deny_permit = access->type;

  packet_format = &hal_macc_grp.l2_type;
  *packet_format = access->packet_format;

  dst_name = hal_macc_grp.name;
  src_name = access->name;

  memcpy (dst_name, src_name, strlen (access->name));
  printf ( "%s   %s", access->name, hal_macc_grp.name);

      /* Copy source MAC address */
  dst_mac_addr = (unsigned char *)&hal_macc_grp.a;
  src_mac_addr = access->a.mac;
  memcpy (dst_mac_addr, src_mac_addr, sizeof (struct hal_acl_mac_addr));

      /* Copy source MAC mask */
  dst_mac_addr = (unsigned char *)&hal_macc_grp.a_mask;
  src_mac_addr = (unsigned char *)&access->a_mask;
  memcpy (dst_mac_addr, src_mac_addr, sizeof (struct hal_acl_mac_addr));

      /* Copy destination MAC address */
  dst_mac_addr = (unsigned char *)&hal_macc_grp.m;
  src_mac_addr = (unsigned char *)&access->m;
  memcpy (dst_mac_addr, src_mac_addr, sizeof (struct hal_acl_mac_addr));

      /* Copy destination MAC mask */
  dst_mac_addr = (unsigned char *)&hal_macc_grp.m_mask;
  src_mac_addr = (unsigned char *)&access->m_mask;
  memcpy (dst_mac_addr, src_mac_addr, sizeof (struct hal_acl_mac_addr));

        /* Send class-map data */
   ret = hal_mac_set_access_grp (&hal_macc_grp, ifindex, action, dir);

   if (ret < 0)
     return ret;
   else
     /* Clear hal_cmap */
     memset (&hal_macc_grp, 0, sizeof (struct hal_mac_access_grp));

  return HAL_SUCCESS;
}

int
mac_acl_extended_set (struct nsm_mac_acl_master *master,
                      char *name_str,
                      int type,
                      char *addr_str,
                      char *addr_mask_str,
                      char *mask_str,
                      char *mask_mask_str,
                      int extended,
                      int set,
                      int acl_type,
                      int packet_format)
{
  struct mac_acl *access = NULL;
  struct acl_mac_addr a;
  struct acl_mac_addr a_mask;
  struct acl_mac_addr m;
  struct acl_mac_addr m_mask;
  int access_num;
  char *endptr = NULL;



  if (type != ACL_FILTER_DENY && type != ACL_FILTER_PERMIT)
    return LIB_API_SET_ERR_INVALID_VALUE;

  /* Access-list name check. */
  access_num = pal_strtou32 (name_str, &endptr, 10);
  if (access_num == UINT32_MAX || *endptr != '\0')
    return LIB_API_SET_ERR_INVALID_VALUE;

  if (extended)
    {
      /* Extended access-list name range check. */
      if (access_num < 100 || access_num > 2699)
        return LIB_API_SET_ERR_INVALID_VALUE;
      if (access_num > 199 && access_num < 2000)
        return LIB_API_SET_ERR_INVALID_VALUE;
    }
  else
    {
      /* Standard access-list name range check. */
      if (access_num < 1 || access_num > 1999)
        return LIB_API_SET_ERR_INVALID_VALUE;
      if (access_num > 99 && access_num < 1300)
        return LIB_API_SET_ERR_INVALID_VALUE;
    }

  if (acl_type == NSM_ACL_TYPE_MAC)
    {
      /* Get the MAC address */
      if (pal_sscanf (addr_str, "%4hx.%4hx.%4hx",
                      (unsigned short *)&a.mac[0],
                      (unsigned short *)&a.mac[2],
                      (unsigned short *)&a.mac[4]) != 3)
        {
          return LIB_API_SET_ERR_MALFORMED_ADDRESS;
        }
      /* Convert to network order */
      *(unsigned short *)&a.mac[0] =
        pal_hton16(*(unsigned short *)&a.mac[0]);
      *(unsigned short *)&a.mac[2] =
        pal_hton16(*(unsigned short *)&a.mac[2]);
      *(unsigned short *)&a.mac[4] =
        pal_hton16(*(unsigned short *)&a.mac[4]);

      if (pal_sscanf (addr_mask_str, "%4hx.%4hx.%4hx",
                      (unsigned short *)&a_mask.mac[0],
                      (unsigned short *)&a_mask.mac[2],
                      (unsigned short *)&a_mask.mac[4]) != 3)
        {
          return LIB_API_SET_ERR_MALFORMED_ADDRESS;
        }
      /* Convert to network order */
      *(unsigned short *)&a_mask.mac[0] =
        pal_hton16(*(unsigned short *)&a_mask.mac[0]);
      *(unsigned short *)&a_mask.mac[2] =
        pal_hton16(*(unsigned short *)&a_mask.mac[2]);
      *(unsigned short *)&a_mask.mac[4] =
        pal_hton16(*(unsigned short *)&a_mask.mac[4]);

      if (pal_sscanf (mask_str, "%4hx.%4hx.%4hx",
                      (unsigned short *)&m.mac[0],
                      (unsigned short *)&m.mac[2],
                      (unsigned short *)&m.mac[4]) != 3)
        {
          return LIB_API_SET_ERR_MALFORMED_ADDRESS;
        }
      /* Convert to network order */
      *(unsigned short *)&m.mac[0] =
        pal_hton16(*(unsigned short *)&m.mac[0]);
      *(unsigned short *)&m.mac[2] =
        pal_hton16(*(unsigned short *)&m.mac[2]);
      *(unsigned short *)&m.mac[4] =
        pal_hton16(*(unsigned short *)&m.mac[4]);

      if (pal_sscanf (mask_mask_str, "%4hx.%4hx.%4hx",
                      (unsigned short *)&m_mask.mac[0],
                      (unsigned short *)&m_mask.mac[2],
                      (unsigned short *)&m_mask.mac[4]) != 3)
        {
          return LIB_API_SET_ERR_MALFORMED_ADDRESS;
        }
      /* Convert to network order */
      *(unsigned short *)&m_mask.mac[0] =
        pal_hton16(*(unsigned short *)&m_mask.mac[0]);
      *(unsigned short *)&m_mask.mac[2] =
        pal_hton16(*(unsigned short *)&m_mask.mac[2]);
      *(unsigned short *)&m_mask.mac[4] =
        pal_hton16(*(unsigned short *)&m_mask.mac[4]);

    }
  else
    return LIB_API_SET_ERR_MALFORMED_ADDRESS;


  if (set)
    {
      /* Install new filter to the access_list. */
      access = mac_acl_get (master, name_str);

      if (access->ref_cnt)
        return NSM_API_SET_ERR_MACACL_ATTACHED;

      /* Set the acl_type to acl list */
      access->acl_type = acl_type;
      access->type = type;
      access->extended = extended;
      access->packet_format = packet_format;

      pal_mem_cpy (&(access->a.mac[0]), &(a.mac[0]), 6);
      pal_mem_cpy (&(access->a_mask.mac[0]), &(a_mask.mac[0]), 6);
      pal_mem_cpy (&(access->m.mac[0]), &(m.mac[0]), 6);
      pal_mem_cpy (&(access->m_mask.mac[0]), &(m_mask.mac[0]), 6);
    }
  else
    {
      access = mac_acl_lookup (master, name_str);
      if (access)
        mac_acl_delete (access);
    }

  return LIB_API_SET_SUCCESS;
}

result_t
mac_acl_show_name (struct cli *cli, struct nsm_mac_acl_master *master, char *name)
{
  struct mac_acl *access = NULL;

  if (master == NULL)
    return 0;
  for (access = master->num.head; access; access = access->next)
    {
      if (name && pal_strcmp (access->name, name) != 0)
        continue;

      if (access->acl_type == NSM_ACL_TYPE_MAC)
        {
          cli_out (cli, " MAC-ACCESS-LIST: %s \n", access->name);
          cli_out (cli, "  %s%s", mac_acl_type_str (access),
                   access->type == ACL_FILTER_DENY ? "  " : "");
          cli_out (cli, " mac");
          if (! compare_mac_filter (&access->a_mask.mac[0], 0xff))
            {
              cli_out (cli, " any");
            }
          else if (! compare_mac_filter (&access->a_mask.mac[0], 0))
            {
              cli_out (cli, " host %02X%02X.%02X%02X.%02X%02X",
                       access->a.mac[0], access->a.mac[1],
                       access->a.mac[2], access->a.mac[3],
                       access->a.mac[4], access->a.mac[5]);
            }
          else
            {
              cli_out (cli, " %02X%02X.%02X%02X.%02X%02X",
                       access->a.mac[0], access->a.mac[1],
                       access->a.mac[2], access->a.mac[3],
                       access->a.mac[4], access->a.mac[5]);
              cli_out (cli, " %02X%02X.%02X%02X.%02X%02X",
                       access->a_mask.mac[0], access->a_mask.mac[1],
                       access->a_mask.mac[2], access->a_mask.mac[3],
                       access->a_mask.mac[4], access->a_mask.mac[5]);
            }
          if (! compare_mac_filter (&access->m_mask.mac[0], 0xff))
            {
              cli_out (cli, " any");
            }
          else if (! compare_mac_filter (&access->m_mask.mac[0], 0))
            {
              cli_out (cli, " host %02X%02X.%02X%02X.%02X%02X",
                       access->m.mac[0], access->m.mac[1],
                       access->m.mac[2], access->m.mac[3],
                       access->m.mac[4], access->m.mac[5]);
            }
          else
            {
              cli_out (cli, " %02X%02X.%02X%02X.%02X%02X",
                       access->m.mac[0], access->m.mac[1],
                       access->m.mac[2], access->m.mac[3],
                       access->m.mac[4], access->m.mac[5]);
              cli_out (cli, " %02X%02X.%02X%02X.%02X%02X",
                       access->m_mask.mac[0], access->m_mask.mac[1],
                       access->m_mask.mac[2], access->m_mask.mac[3],
                       access->m_mask.mac[4], access->m_mask.mac[5]);
            }
          cli_out (cli, "\n");

        }

    }
  return 0;
}



result_t
mac_acl_show (struct cli *cli, struct nsm_mac_acl_master *master)
{
  struct mac_acl *access = NULL;

  if (master == NULL)
    return 0;
  for (access = master->num.head; access; access = access->next)
    {
      if (! access->name)
        continue;

      if (access->acl_type == NSM_ACL_TYPE_MAC)
        {
          cli_out (cli, " MAC-ACCESS-LIST: %s \n", access->name);
          cli_out (cli, "  %s%s", mac_acl_type_str (access),
                   access->type == ACL_FILTER_DENY ? "  " : "");
          cli_out (cli, " mac");
          if (! compare_mac_filter (&access->a_mask.mac[0], 0xff))
            {
              cli_out (cli, " any");
            }
          else if (! compare_mac_filter (&access->a_mask.mac[0], 0))
            {
              cli_out (cli, " host %02X%02X.%02X%02X.%02X%02X",
                       access->a.mac[0], access->a.mac[1],
                       access->a.mac[2], access->a.mac[3],
                       access->a.mac[4], access->a.mac[5]);
            }
          else
            {
              cli_out (cli, " %02X%02X.%02X%02X.%02X%02X",
                       access->a.mac[0], access->a.mac[1],
                       access->a.mac[2], access->a.mac[3],
                       access->a.mac[4], access->a.mac[5]);
              cli_out (cli, " %02X%02X.%02X%02X.%02X%02X",
                       access->a_mask.mac[0], access->a_mask.mac[1],
                       access->a_mask.mac[2], access->a_mask.mac[3],
                       access->a_mask.mac[4], access->a_mask.mac[5]);
            }
          if (! compare_mac_filter (&access->m_mask.mac[0], 0xff))
            {
              cli_out (cli, " any");
            }
          else if (! compare_mac_filter (&access->m_mask.mac[0], 0))
            {
              cli_out (cli, " host %02X%02X.%02X%02X.%02X%02X",
                       access->m.mac[0], access->m.mac[1],
                       access->m.mac[2], access->m.mac[3],
                       access->m.mac[4], access->m.mac[5]);
            }
          else
            {
              cli_out (cli, " %02X%02X.%02X%02X.%02X%02X",
                       access->m.mac[0], access->m.mac[1],
                       access->m.mac[2], access->m.mac[3],
                       access->m.mac[4], access->m.mac[5]);
              cli_out (cli, " %02X%02X.%02X%02X.%02X%02X",
                       access->m_mask.mac[0], access->m_mask.mac[1],
                       access->m_mask.mac[2], access->m_mask.mac[3],
                       access->m_mask.mac[4], access->m_mask.mac[5]);
            }
          cli_out (cli, "\n");

        }
    }
  return 0;
}

result_t
mac_access_grp_show (struct cli *cli, struct nsm_master *nm)
{
  struct nsm_bridge_port *br_port = NULL;
  struct nsm_bridge *bridge = NULL;
  struct avl_node *avl_port = NULL;
  struct nsm_if *zif = NULL;
  bridge = nsm_get_first_bridge ( nm->bridge);

  if(! bridge)
   return 1;

  for (avl_port = avl_top (bridge->port_tree); avl_port;
       avl_port = avl_next (avl_port))
    {
      br_port = (struct nsm_bridge_port *) AVL_NODE_INFO (avl_port);

      zif = br_port->nsmif;

      if (! zif || ! zif->ifp)
        return 1;
      else if ( zif->mac_acl)
        {
          cli_out (cli, " Mac access group for %s MAC ACL %s\n", zif->ifp->name,
                   zif->mac_acl->name);
        }
      else
        {
          cli_out (cli, " No mac access group for %s\n", zif->ifp->name);
        }
    }
  return 0;
}

#endif /* HAVE_L2LERN */
