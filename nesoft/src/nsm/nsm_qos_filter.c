/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "cli.h"

#include "snprintf.h"
#include "vty.h"


#ifdef HAVE_QOS
#include "nsmd.h"
#include "nsm_qos.h"
#include "nsm_qos_list.h"
#include "nsm_qos_filter.h"


#ifdef QOS_ACCESS_LIST_ENTRY_MAX 
#undef QOS_ACCESS_LIST_ENTRY_MAX
#endif /* QOS_ACCESS_LIST_ENTRY_MAX */

#define QOS_ACCESS_LIST_ENTRY_MAX 4

const char *qos_prot_str[] =
  {
    "any",        /* QOS_ANY_PROTO */
    "ethernet2"   /* QOS_ETHERNET_II_PROTO */
    "802dot3"     /* QOS_802_3_PROTO */
    "snap"        /* QOS_SNAP_PROTO */
    "llc"         /* QOS_LLC_PROTO */
    "ip",         /* QOS_IP_PROTO */
    "icmp",       /* QOS_ICMP_PROTO */
    "udp",        /* QOS_UDP_PROTO */
    "tcp"         /* QOS_TCP_PROTO */
  };

const char *qos_op_str[] =
  {
    "",           /* QOS_NOOP */
    "eq",         /* QOS_EQUAL */
    "ne",         /* QOS_NOT_EQUAL  */
    "lt",         /* QOS_LESS_THAN */
    "gt"          /* QOS_GREATER_THAN */
  };

s_int16_t
qos_protocol_type (const char *str)
{
  if (! pal_strncmp(str, "a", 1))
    return QOS_ANY_PROTO;
  else if (! pal_strncmp (str, "et", 2))
    return QOS_ETHERNET_II_PROTO;
  else if (! pal_strncmp (str, "80", 2))
    return QOS_802_3_PROTO;
  else if (! pal_strncmp (str, "sn", 2))
    return QOS_SNAP_PROTO;
  else if (! pal_strncmp (str, "ll", 2))
    return QOS_LLC_PROTO;
  else if (! pal_strncmp (str, "ip", 2))
    return QOS_IP_PROTO;
  else if (! pal_strncmp (str, "ic", 2))
    return QOS_ICMP_PROTO;
  else if (! pal_strncmp (str, "u", 1))
    return QOS_UDP_PROTO;
  else if (! pal_strncmp (str, "t", 1))
    return QOS_TCP_PROTO;

  return -1;
}

s_int16_t
qos_operation_type (const char *str)
{
  if (! pal_strncmp (str, "e", 1))
    return QOS_EQUAL;
  else if (! pal_strncmp (str, "n", 1))
    return QOS_NOT_EQUAL;
  else if (! pal_strncmp (str, "l", 1))
    return QOS_LESS_THAN;
  else if (! pal_strncmp (str, "g", 1))
    return QOS_GREATER_THAN;

  return -1;
}

struct nsm_qos_acl_master *
nsm_qos_acl_master_get (struct nsm_master *nm)
{
  return nm->acl;
}

/* Allocate new filter structure. */
struct qos_filter_list *
qos_filter_new ()
{
  struct qos_filter_list *new;

  new = XCALLOC(MTYPE_TMP, sizeof (struct qos_filter_list));
  return new;
}

void
qos_filter_free (struct qos_filter_list *filter)
{
  XFREE (MTYPE_TMP, filter);
}

/* Return string of filter_type. */
static const char *
qos_filter_type_str (struct qos_filter_list *filter)
{
  switch (filter->type)
    {
    case QOS_FILTER_PERMIT:
      return "permit";
    case QOS_FILTER_DENY:
      return "deny";
    case QOS_FILTER_DYNAMIC:
      return "dynamic";
    case QOS_FILTER_NO_MATCH:
      return "no-match";
    case QOS_FILTER_SOURCE:
      return "source";
    case QOS_FILTER_DESTINATION:
      return "destination";
    default:
      return "";
    }
}


/* Allocate new access list structure. */
struct qos_access_list *
qos_access_list_new ()
{
  struct qos_access_list *new;

  new = XCALLOC(MTYPE_TMP, sizeof (struct qos_access_list));
  return new;
}


/* Free allocated access_list. */
void
qos_access_list_free (struct qos_access_list *access)
{
  struct qos_filter_list *filter;
  struct qos_filter_list *next;

  for (filter = access->head; filter; filter = next)
    {
      next = filter->next;
      qos_filter_free (filter);
    }

  if (access->name)
    XFREE (MTYPE_TMP, access->name);

  if (access->remark)
    XFREE (MTYPE_TMP, access->remark);

  XFREE (MTYPE_TMP, access);
}

struct qos_access_list *
qos_access_list_lock (struct qos_access_list *access)
{
  access->ref_cnt++;
  return access;
}

void
qos_access_list_unlock (struct qos_access_list *access)
{
  access->ref_cnt--;
  if (access->ref_cnt == 0)
    qos_access_list_free (access);
}

/* Delete access_list from access_master and free it. */
void
qos_access_list_delete (struct qos_access_list *access)
{
  struct qos_access_list_list *dlist;
  struct nsm_qos_acl_master *master;

  master = access->master;

  if (access->type == ACCESS_TYPE_NUMBER)
    dlist = &master->num;
  else
    dlist = &master->str;

  if (access->next)
    access->next->prev = access->prev;
  else
    dlist->tail = access->prev;

  if (access->prev)
    access->prev->next = access->next;
  else
    dlist->head = access->next;

  qos_access_list_unlock (access);
}

/* Insert new access list to list of access_list.  Each acceess_list
   is sorted by the name. */
struct qos_access_list *
qos_access_list_insert (struct nsm_qos_acl_master *master, const char *name)
{
  int i;
  long number;
  struct qos_access_list *access;
  struct qos_access_list *point;
  struct qos_access_list_list *alist;

  if (master == NULL)
    return NULL;

  /* Allocate new access_list and copy given name. */
  access = qos_access_list_new ();
  access->name = XSTRDUP (MTYPE_TMP, name);
  access->master = master;
  qos_access_list_lock (access);

  /* If name is made by all digit character.  We treat it as
     number. */
  for (number = 0, i = 0; i < pal_strlen (name); i++)
    {
      if (pal_char_isdigit ((int) name[i]))
        number = (number * 10) + (name[i] - '0');
      else
        break;
    }

  /* In case of name is all digit character */
  if (i == pal_strlen (name))
    {
      access->type = QOS_ACCESS_TYPE_NUMBER;

      /* Set access_list to number list. */
      alist = &master->num;

      for (point = alist->head; point; point = point->next)
        if (pal_strtos32(point->name,NULL,10) >= number)
          break;
    }
  else
    {
      access->type = QOS_ACCESS_TYPE_STRING;

      /* Set access_list to string list. */
      alist = &master->str;

      /* Set point to insertion point. */
      for (point = alist->head; point; point = point->next)
        if (pal_strcmp (point->name, name) >= 0)
          break;
    }

  /* In case of this is the first element of master. */
  if (alist->head == NULL)
    {
      alist->head = alist->tail = access;
      return access;
    }

  /* In case of insertion is made at the tail of access_list. */
  if (point == NULL)
    {
      access->prev = alist->tail;
      alist->tail->next = access;
      alist->tail = access;
      return access;
    }

  /* In case of insertion is made at the head of access_list. */
  if (point == alist->head)
    {
      access->next = alist->head;
      alist->head->prev = access;
      alist->head = access;
      return access;
    }

  /* Insertion is made at middle of the access_list. */
  access->next = point;
  access->prev = point->prev;

  if (point->prev)
    point->prev->next = access;
  point->prev = access;

  return access;
}

/* Lookup access_list from list of access_list by name. */
struct qos_access_list *
qos_access_list_lookup (struct nsm_qos_acl_master *master, const char *name)
{
  struct qos_access_list *access;

  if (name == NULL)
    return NULL;

  if (master == NULL)
    return NULL;

  for (access = master->num.head; access; access = access->next)
    if (pal_strcmp (access->name, name) == 0)
      return access;

  for (access = master->str.head; access; access = access->next)
    if (pal_strcmp (access->name, name) == 0)
      return access;

  return NULL;
}


/* Get access list from list of access_list.  If there isn't matched
   access_list create new one and return it. */
struct qos_access_list *
qos_access_list_get (struct nsm_qos_acl_master *master, const char *name)
{
  struct qos_access_list *access;

  access = qos_access_list_lookup (master, name);
  if (access == NULL)
    access = qos_access_list_insert (master, name);
  return access;
}

int 
qos_check_same_name_and_acl_type_from_access_list (struct nsm_qos_acl_master *master,
                                                   const char *name,
                                                   int acl_type)
{
  struct qos_access_list *access;

  access = qos_access_list_lookup (master, name);
  if (!access)
    return 0;
  else if (access && (access->acl_type == acl_type))
    return 0;
  else
    return 1; /* same name and different acl_type */
}



/* Add new filter to the end of specified access_list. */
int
qos_access_list_filter_add (struct nsm_qos_acl_master *master,
                            struct qos_access_list *access,
                            struct qos_filter_list *filter)
{
#ifdef QOS_ACCESS_LIST_ENTRY_MAX
  int i = 0;
  struct qos_filter_list *f;

  for (f = access->head; f; f = f->next)
    i++;

  if (i >= QOS_ACCESS_LIST_ENTRY_MAX)
    {
      qos_filter_free (filter);
      return LIB_API_SET_ERR_EXCEED_LIMIT;
    }
#endif /* QOS_ACCESS_LIST_ENTRY_MAX  */

  filter->next = NULL;
  filter->prev = access->tail;

  if (access->tail)
    access->tail->next = filter;
  else
    access->head = filter;
  access->tail = filter;

#if 0 /* FFS */
  /* Run hook function. */
  if (access->master->add_hook)
    (*access->master->add_hook) (vr, access, filter);
#endif /* FFS */

  return LIB_API_SET_SUCCESS;
}


/* If access_list has no filter then return 1. */
static result_t
qos_access_list_empty (struct qos_access_list *access)
{
  if (access->head == NULL && access->tail == NULL)
    return 1;
  else
    return 0;
}

/* Delete filter from specified access_list.  If there is hook
   function execute it. */
void
qos_access_list_filter_delete (struct nsm_qos_acl_master *master,
                               struct qos_access_list *access,
                               struct qos_filter_list *filter)
{
#if 0 /* FFS */
  struct access_master *master;

  master = access->master;
#endif /* FFS */

  if (filter->next)
    filter->next->prev = filter->prev;
  else
    access->tail = filter->prev;

  if (filter->prev)
    filter->prev->next = filter->next;
  else
    access->head = filter->next;

  /* If access_list becomes empty delete it from access_master. */
  qos_access_list_lock (access);
  if (qos_access_list_empty (access))
    qos_access_list_delete (access);

#if 0 /* FFS */
  /* Run hook function. */
  if (master->delete_hook)
    (*master->delete_hook) (vr, access, filter);
#endif /* FFS */

  qos_access_list_unlock (access);
  qos_filter_free (filter);
}


/*
  deny    Specify packets to reject
  permit  Specify packets to forward
  dynamic ?
*/

/*
  Hostname or A.B.C.D  Address to match
  any                  Any source host
  host                 A single host address
*/


struct qos_filter_list *
qos_filter_lookup_common (struct qos_access_list *access, struct qos_filter_list *mnew)
{
  struct qos_filter_list *mfilter = NULL;
  struct mac_filter_common *mac_filter = NULL;
  struct mac_filter_common *mac_new = NULL;
  struct ip_filter_common *ip_filter = NULL;
  struct ip_filter_common *ip_new = NULL;

  if (mnew->acl_type == NSM_QOS_ACL_TYPE_IP)
    {
      ip_new = &mnew->u.ifilter;
    }
  else
    {
      mac_new = &mnew->u.mfilter;
    }

  for (mfilter = access->head; mfilter; mfilter = mfilter->next)
    {
      if (mfilter->acl_type == NSM_QOS_ACL_TYPE_IP)
        {
          ip_filter = &mfilter->u.ifilter;
          if (ip_filter->extended) 
            {
              if (ip_new && mfilter->type == mnew->type 
                  && ip_filter->addr.s_addr == ip_new->addr.s_addr
                  && ip_filter->addr_mask.s_addr == ip_new->addr_mask.s_addr
                  && ip_filter->mask.s_addr == ip_new->mask.s_addr
                  && ip_filter->mask_mask.s_addr == ip_new->mask_mask.s_addr)
                return mfilter;
            }
          else 
            { 
              if (ip_new && mfilter->type == mnew->type 
                  && ip_filter->addr.s_addr == ip_new->addr.s_addr
                  && ip_filter->addr_mask.s_addr == ip_new->addr_mask.s_addr)
                return mfilter;
            }
        }
      else
        {
          mac_filter = &mfilter->u.mfilter;
          if (mac_filter->extended)
            {
              if (mac_new && mfilter->type == mnew->type
                  && !(pal_mem_cmp (&mac_filter->a.mac[0], &mac_new->a.mac[0], 6))
                  && !(pal_mem_cmp (&mac_filter->a_mask.mac[0], &mac_new->a_mask.mac[0], 6))
                  && !(pal_mem_cmp (&mac_filter->m.mac[0], &mac_new->m.mac[0], 6))
                  && !(pal_mem_cmp (&mac_filter->m_mask.mac[0], &mac_new->m_mask.mac[0], 6)))
                return mfilter;
            }
          else
            {
              if (mac_new && mfilter->type == mnew->type
                  && !(pal_mem_cmp (&mac_filter->a.mac[0], &mac_new->a.mac[0], 6))
                  && !(pal_mem_cmp (&mac_filter->a_mask.mac[0], &mac_new->a_mask.mac[0], 6)))
                return mfilter;
            }
        }

    }

  return NULL;
}


int
qos_filter_set_common (struct nsm_qos_acl_master *master,
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
  result_t ret;
  struct qos_filter_list *mfilter = NULL;
  struct mac_filter_common *mac_filter = NULL;
  struct ip_filter_common *ip_filter = NULL;
  struct qos_access_list *access = NULL;
  struct mac_filter_addr a;
  struct mac_filter_addr a_mask;
  struct mac_filter_addr m;
  struct mac_filter_addr m_mask;
  struct pal_in4_addr addr;
  struct pal_in4_addr addr_mask;
  struct pal_in4_addr mask;
  struct pal_in4_addr mask_mask;
  int access_num;
  int ret_check;
  char *endptr = NULL;



  if (type != FILTER_PERMIT && type != FILTER_DENY)
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

  if (acl_type == NSM_QOS_ACL_TYPE_IP)
    {
      ret = pal_inet_pton (AF_INET, addr_str, (void*)&addr);
      if (ret <= 0)
        return LIB_API_SET_ERR_MALFORMED_ADDRESS;

      ret = pal_inet_pton (AF_INET, addr_mask_str, (void*)&addr_mask);
      if (ret <= 0)
        return LIB_API_SET_ERR_MALFORMED_ADDRESS;

      if (extended)
        {
          ret = pal_inet_pton (AF_INET, mask_str, (void*)&mask);
          if (ret <= 0)
            return LIB_API_SET_ERR_MALFORMED_ADDRESS;

          ret = pal_inet_pton (AF_INET, mask_mask_str, (void*)&mask_mask);
          if (ret <= 0)
            return LIB_API_SET_ERR_MALFORMED_ADDRESS;
        }
    }
  else if (acl_type == NSM_QOS_ACL_TYPE_MAC)
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


  mfilter = qos_filter_new ();
  mfilter->type = type;     /* deny/ permit */
  mfilter->acl_type = acl_type;

  mfilter->common = QOS_FILTER_COMMON;

  if (acl_type == NSM_QOS_ACL_TYPE_IP)
    {
      ip_filter = &mfilter->u.ifilter;
      ip_filter->extended = extended;
    }
  else
    {
      mac_filter = &mfilter->u.mfilter;
      mac_filter->extended = extended;
      mac_filter->packet_format = packet_format;
      mac_filter->priority = NSM_QOS_INVALID_DOT1P_PRI;
    }

  if (acl_type == NSM_QOS_ACL_TYPE_IP)
    {
      ip_filter->addr_config = addr;
      ip_filter->addr.s_addr = addr.s_addr & ~addr_mask.s_addr;
      ip_filter->addr_mask.s_addr = addr_mask.s_addr;
      if (extended)
        {
          ip_filter->mask_config = mask;
          ip_filter->mask.s_addr = mask.s_addr & ~mask_mask.s_addr;
          ip_filter->mask_mask.s_addr = mask_mask.s_addr;
        }
    }
  else
    {
      pal_mem_cpy (&(mac_filter->a.mac[0]), &(a.mac[0]), 6);
      pal_mem_cpy (&(mac_filter->a_mask.mac[0]), &(a_mask.mac[0]), 6);
      pal_mem_cpy (&(mac_filter->m.mac[0]), &(m.mac[0]), 6);
      pal_mem_cpy (&(mac_filter->m_mask.mac[0]), &(m_mask.mac[0]), 6);
    }

  /* Check if the same name and different acl type in acl */
  ret_check = qos_check_same_name_and_acl_type_from_access_list (master, name_str, acl_type);
  if (ret_check)
    {
      qos_filter_free (mfilter);
      return LIB_API_SET_ERR_DIFF_ACL_TYPE;
    }

  /* Install new filter to the access_list. */
  access = qos_access_list_get (master, name_str);

  /* Set the acl_type to acl list */
  access->acl_type = acl_type;

  if (set)
    {
      if (qos_filter_lookup_common (access, mfilter))
        qos_filter_free (mfilter);
      else
        return qos_access_list_filter_add (master, access, mfilter);
    }
  else
    {
      struct qos_filter_list *delete_filter;

      delete_filter = qos_filter_lookup_common (access, mfilter);
      if (delete_filter)
        qos_access_list_filter_delete (master, access, delete_filter);

      qos_filter_free (mfilter);
    }

  return LIB_API_SET_SUCCESS;
}

int
qos_filter_set_hw_common (struct nsm_qos_acl_master *master,
                       char *name_str,
                       int type,
                       char *addr_str,
                       char *addr_mask_str,
                       char *mask_str,
                       char *mask_mask_str,
                       int extended,
                       int set,
                       int acl_type,
                       int packet_format,
                       int priority)
{
  struct qos_filter_list *mfilter = NULL;
  struct mac_filter_common *mac_filter = NULL;
  struct qos_access_list *access = NULL;
  struct mac_filter_addr a;
  struct mac_filter_addr a_mask;
  struct mac_filter_addr m;
  struct mac_filter_addr m_mask;
  int access_num;
  int ret_check;
  char *endptr = NULL;
  

  if (type != QOS_FILTER_SOURCE && type != QOS_FILTER_DESTINATION)
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


  if (acl_type == NSM_QOS_ACL_TYPE_MAC)
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

  mfilter = qos_filter_new ();
  mfilter->type = type;     /* source or destination */
  mfilter->acl_type = acl_type;

  mfilter->common = QOS_FILTER_COMMON;

  mac_filter = &mfilter->u.mfilter;
  mac_filter->extended = extended;
  mac_filter->packet_format = packet_format;
  mac_filter->priority = priority;

  pal_mem_cpy (&(mac_filter->a.mac[0]), &(a.mac[0]), 6);
  pal_mem_cpy (&(mac_filter->a_mask.mac[0]), &(a_mask.mac[0]), 6);
  pal_mem_cpy (&(mac_filter->m.mac[0]), &(m.mac[0]), 6);
  pal_mem_cpy (&(mac_filter->m_mask.mac[0]), &(m_mask.mac[0]), 6);

  /* Check if the same name and different acl type in acl */
  ret_check = qos_check_same_name_and_acl_type_from_access_list (master, 
                                                     name_str, acl_type);
  if (ret_check)
    {
      qos_filter_free (mfilter);
      return LIB_API_SET_ERR_DIFF_ACL_TYPE;
    }

  /* Install new filter to the access_list. */
  access = qos_access_list_get (master, name_str);

  /* Set the acl_type to acl list */
  access->acl_type = acl_type;

  if (set)
    {
      if (qos_filter_lookup_common (access, mfilter))
        qos_filter_free (mfilter);
      else
        return qos_access_list_filter_add (master, access, mfilter);
    }
  else
    {
      struct qos_filter_list *delete_filter;

      delete_filter = qos_filter_lookup_common (access, mfilter);
      if (delete_filter)
        qos_access_list_filter_delete (master, access, delete_filter);

      qos_filter_free (mfilter);
    }

  return LIB_API_SET_SUCCESS;
}

void qos_config_write_access_pacos (struct cli *, struct qos_filter_list *);
void qos_config_write_access_common (struct cli *, struct qos_filter_list *);
void qos_config_write_access_pacos_ext (struct cli *, struct qos_filter_list *);

static
const char *qos_get_filter_type_str (struct qos_filter_list *mfilter)
{
  switch (mfilter->common)
    {
    case QOS_FILTER_COMMON:
      {
        struct mac_filter_common *macfilter;
        struct ip_filter_common *ipfilter;

        if (mfilter->acl_type == NSM_QOS_ACL_TYPE_IP)
          {
            ipfilter = &mfilter->u.ifilter;
            if (ipfilter->extended)
              return "Extended";
            else
              return "Standard";
          }
        else
          {
            macfilter = &mfilter->u.mfilter;
            if (macfilter->extended)
              return "Extended";
            else
              return "Standard";
          }

      }
      break;
    case QOS_FILTER_PACOS:
      return "PacOS";
    case QOS_FILTER_PACOS_EXT:
      return "PacOS extended";
    }
  return "";
}

/* show access-list command. */
result_t
qos_filter_show (struct cli *cli, char *name)
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_qos_acl_master *master;
  struct qos_access_list *access;
  struct qos_filter_list *mfilter;
  struct mac_filter_common *mac_filter;
  struct ip_filter_common *ip_filter;
  int write = 0;
  char ipa[16];


  if (nm == NULL)
    return 0;

  master = nsm_qos_acl_master_get (nm);

  if (master == NULL)
    return 0;

  for (access = master->num.head; access; access = access->next)
    {
      if (name && pal_strcmp (access->name, name) != 0) 
        continue;

      write = 1;

      for (mfilter = access->head; mfilter; mfilter = mfilter->next) 
        {
          if (mfilter->acl_type == NSM_QOS_ACL_TYPE_IP)
            {
              ip_filter = &mfilter->u.ifilter;
              if (write)
                {
                  cli_out (cli, "%s IP QOS access list: %s \n",
                           qos_get_filter_type_str (mfilter), access->name);
                  write = 0;
                }

              cli_out (cli, "    %s%s", qos_filter_type_str (mfilter),
                       mfilter->type == QOS_FILTER_DENY ? "  " : "");

              if (mfilter->common == QOS_FILTER_PACOS)
                {
                  qos_config_write_access_pacos (cli, mfilter);
                }
              else if (mfilter->common == QOS_FILTER_PACOS_EXT)
                {
                  qos_config_write_access_pacos_ext (cli, mfilter);
                }
              else if (ip_filter->extended)
                {
                  qos_config_write_access_common (cli, mfilter);
                }
              else
                {
                  if (ip_filter->addr_mask.s_addr == 0xffffffff)
                    {
                      cli_out (cli, " any\n");
                    }
                  else
                    {
                      pal_inet_ntop(AF_INET,(void*)&ip_filter->addr,ipa,16);
                      cli_out (cli, " %s", ipa);
                      if (ip_filter->addr_mask.s_addr != 0) 
                        {
                          pal_inet_ntop(AF_INET,(void*)&ip_filter->addr_mask,ipa,16);
                          cli_out (cli, ", wildcard bits %s", ipa);
                        }
                      cli_out (cli, "\n");
                    }
                }
            }
          else if (mfilter->acl_type == NSM_QOS_ACL_TYPE_MAC)
            {
              mac_filter = &mfilter->u.mfilter;
              if (write)
                {
                  cli_out (cli, "%s MAC QOS access list: %s \n",
                           qos_get_filter_type_str (mfilter), access->name);
                  write = 0;
                }

              cli_out (cli, "    %s%s", qos_filter_type_str (mfilter),
                       mfilter->type == QOS_FILTER_DENY ? "  " : "");

              if (mfilter->common == QOS_FILTER_PACOS)
                {
                  qos_config_write_access_pacos (cli, mfilter);
                }
              else if (mfilter->common == QOS_FILTER_PACOS_EXT)
                {
                  qos_config_write_access_pacos_ext (cli, mfilter);
                }
              else if (mac_filter->extended)
                {
                  qos_config_write_access_common (cli, mfilter);
                }
            }
        }
    }

  for (access = master->str.head; access; access = access->next)
    {
      if (name && pal_strcmp (access->name, name) != 0)
        continue;

      write = 1;

      for (mfilter = access->head; mfilter; mfilter = mfilter->next) 
        {
          if (mfilter->acl_type == NSM_QOS_ACL_TYPE_IP)
            {
              ip_filter = &mfilter->u.ifilter;

              if (write)
                {
                  cli_out (cli, "%s IP QoS access list: %s \n",
                           qos_get_filter_type_str (mfilter), access->name);
                  write = 0;
                }

              cli_out (cli, "    %s%s", qos_filter_type_str (mfilter),
                       mfilter->type == QOS_FILTER_DENY ? "  " : "");

              if (mfilter->common == QOS_FILTER_PACOS)
                {
                  qos_config_write_access_pacos (cli, mfilter);
                }
              else if (mfilter->common == QOS_FILTER_PACOS_EXT)
                {
                  qos_config_write_access_pacos_ext (cli, mfilter);
                }
              else if (ip_filter->extended)
                {
                  qos_config_write_access_common (cli, mfilter);
                }
              else
                {
                  if (ip_filter->addr_mask.s_addr == 0xffffffff)
                    {
                      cli_out (cli, " any\n");
                    }
                  else
                    {
                      pal_inet_ntop(AF_INET,(void*)&ip_filter->addr,ipa,16);
                      cli_out (cli, " %s",ipa);
                      if (ip_filter->addr_mask.s_addr != 0) 
                        {
                          pal_inet_ntop(AF_INET,(void*)&ip_filter->addr_mask,ipa,16);
                          cli_out (cli, ", wildcard bits %s", ipa);
                        }
                      cli_out (cli, "\n");
                    }
                }
            }
          else if (mfilter->acl_type == NSM_QOS_ACL_TYPE_MAC)
            {
              mac_filter = &mfilter->u.mfilter;

              if (write)
                {
                  cli_out (cli, "%s MAC QoS access list: %s \n",
                           qos_get_filter_type_str (mfilter), access->name);
                  write = 0;
                }

              cli_out (cli, "    %s%s", qos_filter_type_str (mfilter),
                       mfilter->type == QOS_FILTER_DENY ? "  " : "");

              if (mfilter->common == QOS_FILTER_PACOS)
                {
                  qos_config_write_access_pacos (cli, mfilter);
                }
              else if (mfilter->common == QOS_FILTER_PACOS_EXT)
                {
                  qos_config_write_access_pacos_ext (cli, mfilter);
                }
              else if (mac_filter->extended)
                {
                  qos_config_write_access_common (cli, mfilter);
                }
            }
        }
    }
  return CLI_SUCCESS;
}

result_t
qos_all_filter_show (struct cli *cli)
{
  struct nsm_master *nm = cli->vr->proto;
  struct nsm_qos_acl_master *master;
  struct qos_access_list *access;
  struct qos_filter_list *mfilter;
  struct mac_filter_common *mac_filter;
  struct ip_filter_common *ip_filter;
  int write = 0;
  char ipa[16];


  if (nm == NULL)
    return 0;

  master = nsm_qos_acl_master_get (nm);

  if (master == NULL)
    return 0;

  for (access = master->num.head; access; access = access->next)
    {
      if (! access->name)
        continue;

      write = 1;

      for (mfilter = access->head; mfilter; mfilter = mfilter->next) 
        {
          if (mfilter->acl_type == NSM_QOS_ACL_TYPE_IP)
            {
              ip_filter = &mfilter->u.ifilter;
              if (write)
                {
                  cli_out (cli, "\n");
                  cli_out (cli, "  %s IP-QOS-ACCESS-LIST: %s \n",
                           qos_get_filter_type_str (mfilter), access->name);
                  write = 0;
                }

              cli_out (cli, "    %s%s", qos_filter_type_str (mfilter),
                       mfilter->type == QOS_FILTER_DENY ? "  " : "");

              if (mfilter->common == QOS_FILTER_PACOS)
                {
                  qos_config_write_access_pacos (cli, mfilter);
                }
              else if (mfilter->common == QOS_FILTER_PACOS_EXT)
                {
                  qos_config_write_access_pacos_ext (cli, mfilter);
                }
              else if (ip_filter->extended)
                {
                  qos_config_write_access_common (cli, mfilter);
                }
              else
                {
                  if (ip_filter->addr_mask.s_addr == 0xffffffff)
                    {
                      cli_out (cli, " any\n");
                    }
                  else
                    {
                      pal_inet_ntop(AF_INET,(void*)&ip_filter->addr,ipa,16);
                      cli_out (cli, " %s", ipa);
                      if (ip_filter->addr_mask.s_addr != 0) 
                        {
                          pal_inet_ntop(AF_INET,(void*)&ip_filter->addr_mask,ipa,16);
                          cli_out (cli, ", wildcard bits %s", ipa);
                        }
                      cli_out (cli, "\n");
                    }
                }
            }
          else if (mfilter->acl_type == NSM_QOS_ACL_TYPE_MAC)
            {
              mac_filter = &mfilter->u.mfilter;
              if (write)
                {
                  cli_out (cli, "  %s MAC-QOS-ACCESS-LIST: %s \n",
                           qos_get_filter_type_str (mfilter), access->name);
                  write = 0;
                }

              cli_out (cli, "    %s%s", qos_filter_type_str (mfilter),
                       mfilter->type == QOS_FILTER_DENY ? "  " : "");

              if (mfilter->common == QOS_FILTER_PACOS)
                {
                  qos_config_write_access_pacos (cli, mfilter);
                }
              else if (mfilter->common == QOS_FILTER_PACOS_EXT)
                {
                  qos_config_write_access_pacos_ext (cli, mfilter);
                }
              else if (mac_filter->extended)
                {
                  qos_config_write_access_common (cli, mfilter);
                }
            }
        }
    }

  for (access = master->str.head; access; access = access->next)
    {
      if (! access->name)
        continue;

      write = 1;

      for (mfilter = access->head; mfilter; mfilter = mfilter->next) 
        {
          if (mfilter->acl_type == NSM_QOS_ACL_TYPE_IP)
            {
              ip_filter = &mfilter->u.ifilter;

              if (write)
                {
                  cli_out (cli, "\n");
                  cli_out (cli, "  %s IP-QOS-ACCESS-LIST: %s \n",
                           qos_get_filter_type_str (mfilter), access->name);
                  write = 0;
                }

              cli_out (cli, "    %s%s", qos_filter_type_str (mfilter),
                       mfilter->type == QOS_FILTER_DENY ? "  " : "");

              if (mfilter->common == QOS_FILTER_PACOS)
                {
                  qos_config_write_access_pacos (cli, mfilter);
                }
              else if (mfilter->common == QOS_FILTER_PACOS_EXT)
                {
                  qos_config_write_access_pacos_ext (cli, mfilter);
                }
              else if (ip_filter->extended)
                {
                  qos_config_write_access_common (cli, mfilter);
                }
              else
                {
                  if (ip_filter->addr_mask.s_addr == 0xffffffff)
                    {
                      cli_out (cli, " any\n");
                    }
                  else
                    {
                      pal_inet_ntop(AF_INET,(void*)&ip_filter->addr,ipa,16);
                      cli_out (cli, " %s",ipa);
                      if (ip_filter->addr_mask.s_addr != 0) 
                        {
                          pal_inet_ntop(AF_INET,(void*)&ip_filter->addr_mask,ipa,16);
                          cli_out (cli, ", wildcard bits %s", ipa);
                        }
                      cli_out (cli, "\n");
                    }
                }
            }
          else if (mfilter->acl_type == NSM_QOS_ACL_TYPE_MAC)
            {
              mac_filter = &mfilter->u.mfilter;

              if (write)
                {
                  cli_out (cli, "%s MAC-QOS-ACCESS-LIST: %s \n",
                           qos_get_filter_type_str (mfilter), access->name);
                  write = 0;
                }

              cli_out (cli, "    %s%s", qos_filter_type_str (mfilter),
                       mfilter->type == QOS_FILTER_DENY ? "  " : "");

              if (mfilter->common == QOS_FILTER_PACOS)
                {
                  qos_config_write_access_pacos (cli, mfilter);
                }
              else if (mfilter->common == QOS_FILTER_PACOS_EXT)
                {
                  qos_config_write_access_pacos_ext (cli, mfilter);
                }
              else if (mac_filter->extended)
                {
                  qos_config_write_access_common (cli, mfilter);
                }
            }
        }
    }
  return CLI_SUCCESS;
}

int
qos_compare_mac_filter (u_int8_t *start_addr, u_int8_t val)
{
  int i;

  for (i=0 ; i < 6 ; i++ )
    {
      if ( *start_addr++ != val )
        return 1;
    }
  return 0;
}

void
qos_config_write_access_common (struct cli *cli, struct qos_filter_list *mfilter)
{
  struct mac_filter_common *mac_filter;
  struct ip_filter_common *ip_filter;
  char ipa[16];


  if (mfilter->acl_type == NSM_QOS_ACL_TYPE_IP)
    {
      ip_filter = &mfilter->u.ifilter;

      if (ip_filter->extended)
        {
          cli_out (cli, " ip");
          if (ip_filter->addr_mask.s_addr == 0xffffffff)
            {
              cli_out (cli, " any");
            } 
          else if (ip_filter->addr_mask.s_addr == 0) 
            {
              pal_inet_ntop(AF_INET,(void*)&ip_filter->addr,ipa,16);
              cli_out (cli, " host %s", ipa);
            } 
          else 
            {
              pal_inet_ntop(AF_INET,(void*)&ip_filter->addr,ipa,16);
              cli_out (cli, " %s", ipa);
              pal_inet_ntop(AF_INET,(void*)&ip_filter->addr_mask,ipa,16);
              cli_out (cli, " %s", ipa);
            }

          if (ip_filter->mask_mask.s_addr == 0xffffffff)
            {
              cli_out (cli, " any");
            }
          else if (ip_filter->mask_mask.s_addr == 0)
            {
              pal_inet_ntop(AF_INET,(void*)&ip_filter->mask,ipa,16);
              cli_out (cli, " host %s", ipa);
            }
          else 
            {
              pal_inet_ntop(AF_INET,(void*)&ip_filter->mask,ipa,16);
              cli_out (cli, " %s", ipa);
              pal_inet_ntop(AF_INET,(void*)&ip_filter->mask_mask,ipa,16);
              cli_out (cli, " %s", ipa);
            }
          cli_out (cli, "\n");
        }
      else
        {
          if (ip_filter->addr_mask.s_addr == 0xffffffff)
            cli_out (cli, " any\n");
          else
            {
              pal_inet_ntop(AF_INET,(void*)&ip_filter->addr,ipa,16);
              cli_out (cli, " %s", ipa);
              if (ip_filter->addr_mask.s_addr != 0)
                {
                  pal_inet_ntop(AF_INET,(void*)&ip_filter->addr_mask,ipa,16);
                  cli_out (cli, " %s", ipa);
                }
              cli_out (cli, "\n");
            }
        }
    }
  else if (mfilter->acl_type == NSM_QOS_ACL_TYPE_MAC)
    {
      mac_filter = &mfilter->u.mfilter;

      if (mac_filter->extended)
        {
          cli_out (cli, " mac");
          if (! qos_compare_mac_filter (&mac_filter->a_mask.mac[0], 0xff))
            {
              cli_out (cli, " any");
            }
          else if (! qos_compare_mac_filter (&mac_filter->a_mask.mac[0], 0))
            {
              cli_out (cli, " host %02X%02X.%02X%02X.%02X%02X",
                       mac_filter->a.mac[0], mac_filter->a.mac[1],
                       mac_filter->a.mac[2], mac_filter->a.mac[3],
                       mac_filter->a.mac[4], mac_filter->a.mac[5]);
            }
          else
            {
              cli_out (cli, " %02X%02X.%02X%02X.%02X%02X",
                       mac_filter->a.mac[0], mac_filter->a.mac[1],
                       mac_filter->a.mac[2], mac_filter->a.mac[3],
                       mac_filter->a.mac[4], mac_filter->a.mac[5]);
              cli_out (cli, " %02X%02X.%02X%02X.%02X%02X",
                       mac_filter->a_mask.mac[0], mac_filter->a_mask.mac[1],
                       mac_filter->a_mask.mac[2], mac_filter->a_mask.mac[3],
                       mac_filter->a_mask.mac[4], mac_filter->a_mask.mac[5]);
            }
          if (! qos_compare_mac_filter (&mac_filter->m_mask.mac[0], 0xff))
            {
              cli_out (cli, " any");
            }
          else if (! qos_compare_mac_filter (&mac_filter->m_mask.mac[0], 0))
            {
              cli_out (cli, " host %02X%02X.%02X%02X.%02X%02X",
                       mac_filter->m.mac[0], mac_filter->m.mac[1],
                       mac_filter->m.mac[2], mac_filter->m.mac[3],
                       mac_filter->m.mac[4], mac_filter->m.mac[5]);
            }
          else
            {
              cli_out (cli, " %02X%02X.%02X%02X.%02X%02X",
                       mac_filter->m.mac[0], mac_filter->m.mac[1],
                       mac_filter->m.mac[2], mac_filter->m.mac[3],
                       mac_filter->m.mac[4], mac_filter->m.mac[5]);
              cli_out (cli, " %02X%02X.%02X%02X.%02X%02X",
                       mac_filter->m_mask.mac[0], mac_filter->m_mask.mac[1],
                       mac_filter->m_mask.mac[2], mac_filter->m_mask.mac[3],
                       mac_filter->m_mask.mac[4], mac_filter->m_mask.mac[5]);
            }
          cli_out (cli, "\n");
        }
    }
}


void
qos_config_write_access_pacos (struct cli *cli, struct qos_filter_list *mfilter)
{
  struct qos_filter_pacos *filter;
  struct prefix *p;
  char buf[BUFSIZ];

  if (mfilter->acl_type == NSM_QOS_ACL_TYPE_IP)
    {
      filter = &mfilter->u.zfilter;
      p = &filter->prefix;

      if (p->prefixlen == 0 && ! filter->exact)
        {
          cli_out (cli, " any");
        }
      else
        {
          pal_inet_ntop(p->family,&(p->u.prefix),buf,BUFSIZ);
          cli_out (cli, " %s/%d%s", buf, p->prefixlen,
                   filter->exact ? " exact-match" : "");
        }
      cli_out (cli, "\n");
    }
}

void
qos_config_write_access_pacos_ext (struct cli *cli, struct qos_filter_list *mfilter)
{
  struct qos_filter_pacos_ext *filter;
  struct prefix p;


  if (mfilter->acl_type == NSM_QOS_ACL_TYPE_IP)
    {
      pal_mem_set (&p, 0, sizeof (struct prefix));
      p.family = AF_INET;

      filter = &mfilter->u.zextfilter;

      cli_out (cli, " %s", qos_prot_str [filter->protocol]);

      if (prefix_same (&p, &filter->sprefix))
        cli_out (cli, " any");
      else
        cli_out (cli, " %r/%d", &filter->sprefix.u.prefix4,
                 filter->sprefix.prefixlen);
      if (filter->sport_op != 0)
        cli_out (cli, " %s %d", qos_op_str [filter->sport_op], filter->sport);

      if (prefix_same (&p, &filter->dprefix))
        cli_out (cli, " any");
      else
        cli_out (cli, " %r/%d", &filter->dprefix.u.prefix4,
                 filter->dprefix.prefixlen);
      if (filter->dport_op != 0)
        cli_out (cli, " %s %d", qos_op_str [filter->dport_op], filter->dport);

      cli_out (cli, "\n");
    }
}


/* clean all access-list */
int
nsm_acl_list_clean (struct nsm_qos_acl_master *master)
{
  struct qos_access_list *access;
  struct qos_access_list *access_next;

  access_next = NULL;

  for (access = master->num.head; access; access = access_next)
    {
      if (! access->name)
        continue;

      access_next = access->next;

      qos_access_list_delete (access);
    }

  access_next = NULL; 
 
  for (access = master->str.head; access; access = access_next) 
    {
      if (! access->name)
        continue;

      access_next = access->next;

      qos_access_list_delete (access);
    }
  return CLI_SUCCESS;
}

/* Create the extended mac-access-list. */ 
int
mac_access_list_extended_set (struct nsm_qos_acl_master *master,
                              char *name,
                              int direct,
                              char *addr_str,
                              char *addr_mask_str,
                              char *mask_str,
                              char *mask_mask_str,
                              int acl_type,
                              int packet_format)
{
  return qos_filter_set_common (master,
                                name,
                                direct,
                                addr_str,
                                addr_mask_str,
                                mask_str,
                                mask_mask_str,
                                1,
                                1,
                                acl_type,
                                packet_format);
}
/* Create the extended mac-access-list. */
int
mac_access_list_extended_hw_set (struct nsm_qos_acl_master *master,
                              char *name,
                              int direct,
                              char *addr_str,
                              char *addr_mask_str,
                              char *mask_str,
                              char *mask_mask_str,
                              int acl_type,
                              int packet_format,
                              int priority)
{
  return qos_filter_set_hw_common (master,
                                name,
                                direct,
                                addr_str,
                                addr_mask_str,
                                mask_str,
                                mask_mask_str,
                                1,
                                1,
                                acl_type,
                                packet_format,
                                priority);
}

/* Delete the ip_extended access-list. */
int
mac_access_list_extended_hw_unset (struct nsm_qos_acl_master *master,
                                char *name,
                                int direct,
                                char *addr_str,
                                char *addr_mask_str,
                                char *mask_str,
                                char *mask_mask_str,
                                int acl_type,
                                int packet_format,
                                int priority)
{
  return qos_filter_set_hw_common (master,
                                name,
                                direct,
                                addr_str,
                                addr_mask_str,
                                mask_str,
                                mask_mask_str,
                                1,
                                0,
                                acl_type,
                                packet_format,
                                priority);
}

/* Delete the ip_extended access-list. */
int
mac_access_list_extended_unset (struct nsm_qos_acl_master *master,
                                char *name,
                                int direct,
                                char *addr_str,
                                char *addr_mask_str,
                                char *mask_str,
                                char *mask_mask_str,
                                int acl_type,
                                int packet_format)
{
  return qos_filter_set_common (master,
                                name,
                                direct,
                                addr_str,
                                addr_mask_str,
                                mask_str,
                                mask_mask_str,
                                1,
                                0,
                                acl_type,
                                packet_format);
}

/* Create the extended ip-access-list. */ 
int
ip_access_list_standard_set (struct nsm_qos_acl_master *master,
                             char *name,
                             int direct,
                             char *addr_str,
                             char *addr_mask_str,
                             int acl_type,
                             int packet_format)
{
  return qos_filter_set_common (master,
                                name,
                                direct,
                                addr_str,
                                addr_mask_str,
                                NULL,
                                NULL,
                                0,
                                1,
                                acl_type,
                                packet_format);
}

/* Delete the extended ip-access-list. */
int
ip_access_list_standard_unset (struct nsm_qos_acl_master *master,
                               char *name,
                               int direct,
                               char *addr_str,
                               char *addr_mask_str,
                               int acl_type,
                               int packet_format)
{
  return qos_filter_set_common (master,
                                name,
                                direct,
                                addr_str,
                                addr_mask_str,
                                NULL,
                                NULL,
                                0,
                                0,
                                acl_type,
                                packet_format);
}

/* Create the extended ip-access-list. */ 
int
ip_access_list_extended_set (struct nsm_qos_acl_master *master,
                             char *name,
                             int direct,
                             char *addr_str,
                             char *addr_mask_str,
                             char *mask_str,
                             char *mask_mask_str,
                             int acl_type,
                             int packet_format)
{
  return qos_filter_set_common (master,
                                name,
                                direct,
                                addr_str,
                                addr_mask_str,
                                mask_str,
                                mask_mask_str,
                                1,
                                1,
                                acl_type,
                                packet_format);
}

/* Delete the extended ip-access-list. */
int
ip_access_list_extended_unset (struct nsm_qos_acl_master *master,
                               char *name,
                               int direct,
                               char *addr_str,
                               char *addr_mask_str,
                               char *mask_str,
                               char *mask_mask_str,
                               int acl_type,
                               int packet_format)
{
  return qos_filter_set_common (master,
                                name,
                                direct,
                                addr_str,
                                addr_mask_str,
                                mask_str,
                                mask_mask_str,
                                1,
                                0,
                                acl_type,
                                packet_format);
}

#endif /* HAVE_QOS */
