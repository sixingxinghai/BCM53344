/* Copyright (C) 2004   All Rights Reserved. */

/* This file stores the map of the logical port to the "OS" interface
   naming. 
   In case of single CPU single slot configuration, these mappings will
   map one-to-one. For eg logical port 1 will map to fe1.
   
   In case of stacking or chassis systems, another map will be required
   to map a logical port to the OS name and the logical port to the 
   mapped name. 
   NOTE:  At this point in time, for stacking the lport is believed to give
   the details about the physicality of the port.
   For eg: lport 1 -> OS name "fe1" -> mapped name "FastEthernet 0/0"
*/

#include "config.h"
#include "hsl_os.h"

#include "bcm_incl.h"

#include "hsl_types.h"
#include "hsl_oss.h"
#include "hsl_logger.h"
#include "hsl_ifmgr.h"
#include "hsl_if_hw.h"
#include "hsl_bcm_ifmap.h"

#include "hsl_avl.h"

/* 
   Interface map. 
*/
static struct hsl_bcm_ifmap *p_hsl_bcm_ifmap;


/* 
   This function returns the mac address from the base incremented by the index.
 */
static void
_get_next_mac_address (u_char *in, u_char *out)
{
  int i;

  memcpy (out, in, HSL_ETHER_ALEN);

  for (i = HSL_ETHER_ALEN - 1; i >=0; i--)
    {
      if ((out[i] + 1) == 0)
        {
          out[i] = 0;
          continue;
        }
      else
        {
          out[i] += 1;
          break;
        }
    }

  return;
}
   
/* 
   Allocate lport ifmap. 
*/
static struct hsl_bcm_ifmap_lport *
_alloc_ifmap_lport (void)
{
  struct hsl_bcm_ifmap_lport *l;

  l = oss_malloc (sizeof (struct hsl_bcm_ifmap_lport), OSS_MEM_HEAP);

  return l;
}

/*
  Free lport ifmap. 
*/
static void
_free_ifmap_lport (struct hsl_bcm_ifmap_lport **l)
{
  if (*l)
    oss_free (*l, OSS_MEM_HEAP);

  *l = NULL;

  return;
}

/* User port to BCMX lport mapping. */
u_int32_t
hsl_bcm_port_u2l (int modid, int unit, int port)
{
  return ((modid << HSL_BCM_MODID_SHIFTS)
          | (unit << HSL_BCM_UNIT_SHIFTS)
          | port);
}

/* BCMX lport to User port mapping. */
void
hsl_bcm_port_l2u (u_int32_t uport, int *modid, int *unit, int *port)
{
  *port = uport & HSL_BCM_PORT_MASK;
  *unit = (uport >> HSL_BCM_UNIT_SHIFTS) & HSL_BCM_UNIT_MASK;
  *modid = (uport >> HSL_BCM_MODID_SHIFTS) & HSL_BCM_MODID_MASK;
}

/*
   Get the MAC for a SVI.
*/
char *
hsl_bcm_ifmap_svi_mac_get (void)
{
  return p_hsl_bcm_ifmap->mac_array[HSL_BCM_IFMAP_SVI_MAC_INDEX]; 
}

/* 
   Get the name of the interface. If present, send the current name. If not 
   present create a new entry. 
   This function will register the lport in the interface map and return the
   name and mac address assigned for the port. 
*/
int
hsl_bcm_ifmap_lport_register (bcmx_lport_t lport, uint32 flags, 
                              char *name, u_char *mac)
{
  struct hsl_avl_node *node;
  struct hsl_bcm_ifmap_lport ln, *lm;
  u_char *arr;
  char *n;
  int index, mac_index, max, found, type;

  ln.lport = lport;

  node = hsl_avl_lookup (p_hsl_bcm_ifmap->lport_map_tree, &ln);
  if (! node)
    {
      /* No entry found for this lport. Assign a name and index. */
      if (flags & BCMX_PORT_F_FE)
        {
          if (! p_hsl_bcm_ifmap->max_fe_ports)
            {
              HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Maximum number of FE ports not set\n");
              return HSL_BCM_ERR_IFMAP_MAX_FE_PORTS_NOT_SET;
            }

          arr = p_hsl_bcm_ifmap->fe_index_table;
          n = HSL_IFMAP_FE_NAME;
          max = p_hsl_bcm_ifmap->max_fe_ports;
          type = BCMX_PORT_F_FE;
        }
      else if (flags & BCMX_PORT_F_GE)
        {
          if (! p_hsl_bcm_ifmap->max_ge_ports)
            {
              HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Maximum number of GE ports not set\n");
              return HSL_BCM_ERR_IFMAP_MAX_GE_PORTS_NOT_SET;
            }

          arr = p_hsl_bcm_ifmap->ge_index_table;
          n = HSL_IFMAP_GE_NAME;
          max = p_hsl_bcm_ifmap->max_ge_ports;
          type = BCMX_PORT_F_GE;
        }
      else if (flags & BCMX_PORT_F_XE)
        {
          if (! p_hsl_bcm_ifmap->max_xe_ports)
            {
              HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Maximum number of XE ports not set\n");
              return HSL_BCM_ERR_IFMAP_MAX_XE_PORTS_NOT_SET;
            }

          arr = p_hsl_bcm_ifmap->xe_index_table;
          n = HSL_IFMAP_XE_NAME;
          max = p_hsl_bcm_ifmap->max_xe_ports;
          type = BCMX_PORT_F_XE;
        }
      else if (flags & HSL_PORT_F_AGG)
        {
          if (! p_hsl_bcm_ifmap->max_agg_ports)
            {
              HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Maximum number of AGG ports not set\n");
              return HSL_BCM_ERR_IFMAP_MAX_AGG_PORTS_NOT_SET;
            }

          arr = p_hsl_bcm_ifmap->agg_index_table;
          n = HSL_IFMAP_AGG_NAME;
          max = p_hsl_bcm_ifmap->max_agg_ports;
          type = HSL_PORT_F_AGG;
       }
      else
        {       
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_ERROR, "Unknown port type received for Broadcom interface map\n");
          return HSL_BCM_ERR_IFMAP_UNKNOWN_DATA_PORT;
        }

      /* find first free index to use. */
      found = 0;

      /* Keep consistence with Hardware configuration.  */
      if (!(flags & HSL_PORT_F_AGG))
        {
          int unit, port, dport;
          unit = BCMX_LPORT_BCM_UNIT (lport);
          port = BCMX_LPORT_BCM_PORT (lport);
          dport = soc_dport_from_port(unit, port);
          if (dport != -1)
            {
              if ((flags & BCMX_PORT_F_GE) ||(flags & BCMX_PORT_F_FE))
                index = dport - 1;
              if (flags & BCMX_PORT_F_XE)
                index = dport - 101;
              if (index >= 0 && index < max)
                found = 1;
            }
        }
      else
       for (index = 0; index < max; index++)
        {
          if (arr[index] == 0)
            {
              found = 1;
              break;
            }
        }

      if (! found)
        {
          /* Maximum number of entries reached.. */
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Maximum number of ports configured. Can't add new port to the Broadcom interface map\n");
          return HSL_BCM_ERR_IFMAP_MAX_PORTS_ALLOCATED;
        }

      arr[index] = 1;

      /* Find first free index for mac. */
      found = 0;
      arr = p_hsl_bcm_ifmap->mac_index_table;
      for (mac_index = 1; mac_index < p_hsl_bcm_ifmap->max_macindexes; mac_index++)
        {
          if (arr[mac_index] == 0)
            {
              found = 1;
              break;
            }
        }

      if (! found)
        {
          /* Maximum number of entries reached.. */
          HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_FATAL, "Maximum number of ports configured. Can't add new port to the Broadcom interface map\n");

          return HSL_BCM_ERR_IFMAP_MAX_PORTS_ALLOCATED;
        }

      arr[mac_index] = 1;

      lm = _alloc_ifmap_lport ();
      if (! lm)
        {
          /* XXX log */
          return HSL_BCM_ERR_IFMAP_NOMEM;
        }
      lm->lport = lport;
      lm->index = index;
      lm->type = flags;
      lm->mac_index = mac_index;

      /* Add this to the map. */
      hsl_avl_insert (p_hsl_bcm_ifmap->lport_map_tree, lm);

      /* Create the name. */
      sprintf (name, "%s%d", n, index);

      /* Get the mac address. */
      memcpy (mac, p_hsl_bcm_ifmap->mac_array[mac_index], HSL_ETHER_ALEN);
   
      return 0;
    }
  else
    {
      HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "Lport(%d) already registered\n", lport);
      return HSL_BCM_ERR_IFMAP_LPORT_ALREADY_REGISTERED;
    }

  HSL_LOG (HSL_LOG_PLATFORM, HSL_LEVEL_INFO, "Lport(%d) registered\n", lport);

  return 0;
}

/*
  Unset the ifmap for a lport. 
*/
int
hsl_bcm_ifmap_lport_unregister (bcmx_lport_t lport)
{
  struct hsl_avl_node *node;
  struct hsl_bcm_ifmap_lport ln, *lm;
  u_char *arr;

  ln.lport = lport;

  node = hsl_avl_lookup (p_hsl_bcm_ifmap->lport_map_tree, &ln);
  if (! node)
    return HSL_BCM_ERR_IFMAP_LPORT_NOT_FOUND;

  lm = HSL_AVL_NODE_INFO(node);
  if (! lm)
    return 0;

  /* Delete node from AVL tree. */
  hsl_avl_delete (p_hsl_bcm_ifmap->lport_map_tree, lm);

  /* Free the port index. */
  if (lm->type == BCMX_PORT_F_FE)
    arr = p_hsl_bcm_ifmap->fe_index_table;
  else if (lm->type == BCMX_PORT_F_GE)
    arr = p_hsl_bcm_ifmap->ge_index_table;
  else if (lm->type == BCMX_PORT_F_XE)
    arr = p_hsl_bcm_ifmap->xe_index_table;
  else if (lm->type == HSL_PORT_F_AGG)
    arr = p_hsl_bcm_ifmap->agg_index_table;
  else
    return HSL_BCM_ERR_IFMAP_LPORT_NOT_FOUND;

  arr[lm->index] = 0;

  /* Free the MAC index. */
  p_hsl_bcm_ifmap->mac_index_table[lm->mac_index] = 0;

  /* Free lport ifindex mapping. */
  _free_ifmap_lport (&lm);

  return 0;
}

/* 
   Get the mapped name of the interface. Typically will used for stacked solutions. 
*/
int
hsl_bcm_ifmap_mapped_name_get (bcmx_lport_t lport, int unit, int port, uint32 flags,
                               char *name)
{
  return -1;
}

/*
  Comparision function. 
*/
static int
_hsl_bcm_ifmap_cmp (void *p1, void *p2)
{
  struct hsl_bcm_ifmap_lport *lp1 = (struct hsl_bcm_ifmap_lport *)p1;
  struct hsl_bcm_ifmap_lport *lp2 = (struct hsl_bcm_ifmap_lport *)p2;

  /* Less than. */
  if (lp1->lport < lp2->lport)
    return -1;

  /* Greater than. */
  if (lp1->lport > lp2->lport)
    return 1;

  /* Equals to. */
  return 0;
}

/*
 * Function:hsl_bcm_ifmap_max_ports_set - Service routine to set max number 
 *                                        of port in interface map per port type.                         
 */
static int
hsl_bcm_ifmap_max_ports_set (u_int32_t num,u_int16_t port_type)
{
  u_char **tbl_ptr;
  int *num_ptr;   

  switch(port_type)
  {
  case HSL_PORT_F_FE:
    p_hsl_bcm_ifmap->max_fe_ports = num;
    tbl_ptr = &p_hsl_bcm_ifmap->fe_index_table;
    num_ptr = &p_hsl_bcm_ifmap->max_fe_ports;
    break;
  case HSL_PORT_F_GE:
    p_hsl_bcm_ifmap->max_ge_ports = num;
    tbl_ptr = &p_hsl_bcm_ifmap->ge_index_table;
    num_ptr = &p_hsl_bcm_ifmap->max_ge_ports;
    break;
  case HSL_PORT_F_XE:
    p_hsl_bcm_ifmap->max_xe_ports = num;
    tbl_ptr = &p_hsl_bcm_ifmap->xe_index_table;
    num_ptr = &p_hsl_bcm_ifmap->max_xe_ports;
    break;
  case HSL_PORT_F_AGG:
    p_hsl_bcm_ifmap->max_agg_ports = num;
    tbl_ptr = &p_hsl_bcm_ifmap->agg_index_table;
    num_ptr = &p_hsl_bcm_ifmap->max_agg_ports;
    break;
  default:
    return STATUS_ERROR;
  }

  *tbl_ptr = oss_malloc ((sizeof (u_char) * num),OSS_MEM_HEAP);
  if (! tbl_ptr)
  {
    *num_ptr = 0;
    return HSL_BCM_ERR_IFMAP_NOMEM;
  }
  p_hsl_bcm_ifmap->max_ports = p_hsl_bcm_ifmap->max_fe_ports +
                               p_hsl_bcm_ifmap->max_ge_ports +
                               p_hsl_bcm_ifmap->max_xe_ports +
                               p_hsl_bcm_ifmap->max_agg_ports;

  return 0;
}

/* 
   Set the interface for a lport. 
*/
int
hsl_bcm_ifmap_if_set (bcmx_lport_t lport, struct hsl_if *ifp)
{
  struct hsl_bcm_ifmap_lport ln, *lm;
  struct hsl_avl_node *node;

  ln.lport = lport;

  node = hsl_avl_lookup (p_hsl_bcm_ifmap->lport_map_tree, &ln);
  if (! node)
    return HSL_BCM_ERR_IFMAP_LPORT_NOT_FOUND;

  lm = HSL_AVL_NODE_INFO(node);
  if (! lm)
    return 0;

  lm->ifp = ifp;

  return 0;
}

/*
  Get the interface for a lport. 
*/
struct hsl_if *
hsl_bcm_ifmap_if_get (bcmx_lport_t lport)
{
  struct hsl_bcm_ifmap_lport ln, *lm;
  struct hsl_avl_node *node;

  ln.lport = lport;

  node = hsl_avl_lookup (p_hsl_bcm_ifmap->lport_map_tree, &ln);
  if (! node)
    return NULL;

  lm = HSL_AVL_NODE_INFO(node);
  if (! lm)
    return NULL;

  return lm->ifp;
}

/* 
   Initialize ifmap. 
*/
int 
hsl_bcm_ifmap_init (int max_fe, int max_ge, int max_xe, bcm_mac_t mac_base)
{
  int ret = 0, i;
  u_char *prev;

  HSL_FN_ENTER ();

  p_hsl_bcm_ifmap = oss_malloc (sizeof (struct hsl_bcm_ifmap), OSS_MEM_HEAP);
  if (! p_hsl_bcm_ifmap)
    {
      /* XXX log */

      ret = HSL_BCM_ERR_IFMAP_NOMEM;
      goto ERR;
    }

  /* Initialize AVL tree for lport map. */
  ret = hsl_avl_create (&p_hsl_bcm_ifmap->lport_map_tree, 
                        0, 
                        _hsl_bcm_ifmap_cmp);
  if (ret < 0)
    {
      /* XXX log */
      ret = HSL_BCM_ERR_IFMAP_NOMEM;
      goto ERR;
    }

  /* Set maximum number of fe, ge, xe, agg, and total ports. */
  hsl_bcm_ifmap_max_ports_set (max_fe,HSL_PORT_F_FE);
  if (ret < 0)
  {
    ret = HSL_BCM_ERR_IFMAP_NOMEM;
    goto ERR;
  }
  ret = hsl_bcm_ifmap_max_ports_set (max_ge,HSL_PORT_F_GE);
  if (ret < 0)
  {
    ret = HSL_BCM_ERR_IFMAP_NOMEM;
    goto ERR;
  }
  ret = hsl_bcm_ifmap_max_ports_set (max_xe,HSL_PORT_F_XE);
  if (ret < 0)
  {
    ret = HSL_BCM_ERR_IFMAP_NOMEM;
    goto ERR;
  }
  /* Maximum number of trunks on tiger family */
  ret = hsl_bcm_ifmap_max_ports_set (32, HSL_PORT_F_AGG);
  if (ret < 0)
  {
    ret = HSL_BCM_ERR_IFMAP_NOMEM;
    goto ERR;
  }

  /* Set the maximum number of MAC address to preallocate. 
     This is equal to the max_port + 1 (for SVI interfaces).
     We just allocate one MAC for all SVIs for now. */
  p_hsl_bcm_ifmap->max_macindexes = p_hsl_bcm_ifmap->max_ports + 1;

  /* Create mac index array. */
  p_hsl_bcm_ifmap->mac_index_table = oss_malloc (sizeof (u_char) * p_hsl_bcm_ifmap->max_macindexes, OSS_MEM_HEAP);
  if (! p_hsl_bcm_ifmap->mac_index_table)
    {
      ret = HSL_BCM_ERR_IFMAP_NOMEM;
      goto ERR;
    }

  /* Compute and store precalculated mac addresses. */
  p_hsl_bcm_ifmap->mac_array = oss_malloc (p_hsl_bcm_ifmap->max_macindexes * sizeof (u_char *), OSS_MEM_HEAP);
  if (! p_hsl_bcm_ifmap->mac_array)
    {
      ret = HSL_BCM_ERR_IFMAP_NOMEM;
      goto ERR;
    }

  prev = mac_base;
  for (i = 0; i < p_hsl_bcm_ifmap->max_macindexes; i++)
    {
      p_hsl_bcm_ifmap->mac_array[i] = oss_malloc (HSL_ETHER_ALEN * sizeof (u_char), OSS_MEM_HEAP);
      if (! p_hsl_bcm_ifmap->mac_array[i])
        {
          /* XXX log */
          ret = HSL_BCM_ERR_IFMAP_NOMEM;
          goto ERR;
        }

      _get_next_mac_address (prev, (u_char*)p_hsl_bcm_ifmap->mac_array[i]);
      prev = (u_char*)p_hsl_bcm_ifmap->mac_array[i];
    }

  HSL_FN_EXIT (0);

 ERR:
  hsl_bcm_ifmap_deinit ();

  HSL_FN_EXIT (ret);
}

/*
  Deinitialize ifmap. 
*/
int
hsl_bcm_ifmap_deinit (void)
{
  int i;

  if (! p_hsl_bcm_ifmap)
    return 0;

  /* Delete AVL tree. */
   hsl_avl_tree_free (&p_hsl_bcm_ifmap->lport_map_tree, hsl_free);

  /* Free index table. */
  if (p_hsl_bcm_ifmap->fe_index_table)
    oss_free (p_hsl_bcm_ifmap->fe_index_table, OSS_MEM_HEAP);
  if (p_hsl_bcm_ifmap->ge_index_table)
    oss_free (p_hsl_bcm_ifmap->ge_index_table, OSS_MEM_HEAP);
  if (p_hsl_bcm_ifmap->xe_index_table)
    oss_free (p_hsl_bcm_ifmap->xe_index_table, OSS_MEM_HEAP);
  if (p_hsl_bcm_ifmap->agg_index_table)
    oss_free (p_hsl_bcm_ifmap->agg_index_table,OSS_MEM_HEAP);
  if (p_hsl_bcm_ifmap->mac_index_table)
    oss_free (p_hsl_bcm_ifmap->mac_index_table, OSS_MEM_HEAP);

  /* Free MAC table. */
  if (p_hsl_bcm_ifmap->mac_array)
    for (i = 0; i < p_hsl_bcm_ifmap->max_macindexes; i++)
        oss_free (p_hsl_bcm_ifmap->mac_array[i], OSS_MEM_HEAP);

  oss_free (p_hsl_bcm_ifmap->mac_array, OSS_MEM_HEAP);
        
        /* Free ifmap. */
  oss_free (p_hsl_bcm_ifmap, OSS_MEM_HEAP);

  return 0;
}
