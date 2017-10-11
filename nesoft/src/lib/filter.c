/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#include "lib.h"
#include "cli.h"


/*------------------------------------------------------------------
 * Definitions used in impl. of ZebSO extended lists
 */
typedef enum filter_param_type
{
  FILTER_PAR_NAME,
  FILTER_PAR_TYPE,       /* (deny|permit) */
  FILTER_PAR_PROTO,      /* ip|any) */
  FILTER_PAR_SRC_ADDR,   /* (A.B.C.D/M|any) */
  FILTER_PAR_SRC_PORT,   /* ... */
  FILTER_PAR_DST_ADDR,   /* (A.B.C.D/M|any) */
  FILTER_PAR_DST_PORT,   /* ... */
  FILTER_PAR_OPTIONAL,
  FILTER_PAR_END
} FILTER_PARAM_TYPE;

#define FILTER_ZEXT_ADDR_ONLY_TMPL \
    { FILTER_PAR_NAME,            \
      FILTER_PAR_TYPE,            \
      FILTER_PAR_PROTO,           \
      FILTER_PAR_SRC_ADDR,        \
      FILTER_PAR_DST_ADDR,        \
      FILTER_PAR_OPTIONAL,        \
      FILTER_PAR_END              \
    }
#define FILTER_ZEXT_ADDR_PORTS_TMPL \
    { FILTER_PAR_NAME,            \
      FILTER_PAR_TYPE,            \
      FILTER_PAR_PROTO,           \
      FILTER_PAR_SRC_ADDR,        \
      FILTER_PAR_SRC_PORT,        \
      FILTER_PAR_DST_ADDR,        \
      FILTER_PAR_DST_PORT,        \
      FILTER_PAR_OPTIONAL,        \
      FILTER_PAR_END              \
    }

#define FILTER_ZEXT_HELP_COMMON   \
     "Add an access list entry",  \
     "PacOS extended access list",\
     "PacOS access-list name",    \
     "Specify packets to reject", \
     "Specify packets to forward"


#define FILTER_ZEXT_HELP_PROTOCOLS \
     "IP packet",                  \
     "GRE packet",                 \
     "IGMP packet",                \
     "PIM packet",                 \
     "RSVP packet",                 \
     "OSPF packet",                \
     "VRRP packet",                \
     "IPComp packet",              \
     "ANY protocol packet",        \
     "IANA assigned protocol number"

#define FILTER_ZEXT_HELP_ADDR_ONLY      \
     "Source Address with network mask length", \
     "Source Address with wild card mask", \
     "Source Address's wild card mask (ignored bits)", \
     "Any source address",              \
     "Dest. Address with network mask length", \
     "Dest. Address with wild card mask", \
     "Dest. Address's wild card mask (ignored bits)", \
     "Any destination address"          \

#define FILTER_ZEXT_HELP_ADDR_PORTS     \
     "Source Address with network mask length", \
     "Source Address with wild card mask", \
     "Source Address's wild card mask (ignored bits)", \
     "Any source address",              \
     "Source port Equal to",            \
     "Source port Less than",           \
     "Source port Greater than",        \
     "Source port Not Equal to",        \
     "Source port Number",              \
     "Range of source port numbers",    \
     "Lowest value in the range",       \
     "Highest value in the range",      \
     "Dest. Address with network mask length", \
     "Dest. Address with wild card mask", \
     "Dest. Address's wild card mask (ignored bits)", \
     "Any destination address",         \
     "Destination port Equal to",       \
     "Destination port Less than",      \
     "Destination port Greater than",   \
     "Destination port Not Equal to",   \
     "Destination Port Number",         \
     "Range of destination port numbers", \
     "Lowest value in the range",       \
     "Highest value in the range"

/* (16(max-del),8(max-thru),4(max-rel),2(min-cost),0(norm) or appl. use) */
#define FILTER_ZEXT_OPTIONS             \
     "Label",                           \
     "Label value",                     \
     "Precedence",                      \
     "Precedence value",                \
     "Type of service (TOS)",           \
     "TOS value", \
     "TOS range of values (appl. use only", \
     "TOS lowest value in the range",   \
     "TOS highest value in the range",  \
     "Packet size",                     \
     "Packet size Less than",           \
     "Packet size Greater than",        \
     "Pcket size value",                \
     "Packet size Range",               \
     "Pcket size range low value",      \
     "Pcket size range high value",     \
     "Fragments",                       \
     "Log the Results",                 \
     "Interface",                       \
     "Incoming interface",              \
     "Outgoing interface",              \
     "Interface name"


#define FILTER_OK                  (0)
#define FILTER_ERR                (-1)
#define FILTER_ERR_CREATION       (-2)
#define FILTER_ERR_DUPLICATE      (-3)
#define FILTER_ERR_DELETED        (-4)
#define FILTER_ERR_FOUND_OTHER_FILTER (-5)
#define FILTER_ERR_FOUND_OTHER_TYPE   (-6)
#define FILTER_ERR_NOT_EXIST          (-7)

/* End of definition sfor PacOS extended lists.
----------------------------------------------------------------
*/

/*---------------------------------------------------------------
 * filter_prot_str - Return the protocol name.
 *
 * NOTE: The str parameter is only important if the caller is using
 *       protocol that is not assigned a name here. Otherwise, NULL
 *       should be passed.
 */
char *
filter_prot_str(s_int16_t proto, char *str)
{
  if (proto == ANY_PROTO)
    return "any";
  else if (proto == GRE_PROTO)
    return "gre";
  else if (proto == IPCOMP_PROTO)
    return  "ipcomp";
  else if (proto == IP_PROTO)
    return  "ip";
  else if (proto == ICMP_PROTO)
    return  "icmp";
  else if (proto == IGMP_PROTO)
    return  "igmp";
  else if (proto == OSPF_PROTO)
    return  "ospf";
  else if (proto == RSVP_PROTO)
    return  "rsvp";
  else if (proto == PIM_PROTO)
    return  "pim";
  else if (proto == UDP_PROTO)
    return  "udp";
  else if (proto == VRRP_PROTO)
    return  "vrrp";
  else if (proto == TCP_PROTO)
    return   "tcp";
  else if (str)
    {
      pal_sprintf(str, "%d", proto);
      return (str);
    }
  else
    return (NULL);
}

const char *op_str[] =
{
  "",         /* NOOP */
  "eq",       /* EQUAL */
  "ne",       /* NOT_EQUAL */
  "lt",       /* LESS_THAN */
  "gt",       /* GREATER_THAN */
  "range"     /* RANGE: port numbers and tos */
};

s_int16_t
protocol_type (char *str)
{
  if (! pal_strncmp(str, "a", 1))
    return (ANY_PROTO);
  else if (! pal_strncmp (str, "gr", 2))
    return (GRE_PROTO);
  else if (! pal_strncmp (str, "ipc", 3))
    return (IPCOMP_PROTO);
  else if (! pal_strncmp (str, "ip", 2))
    return (IP_PROTO);
  else if (! pal_strncmp (str, "ic", 2))
    return (ICMP_PROTO);
  else if (! pal_strncmp (str, "ig", 2))
    return (IGMP_PROTO);
  else if (! pal_strncmp (str, "o", 1))
    return (OSPF_PROTO);
  else if (! pal_strncmp (str, "r", 1))
    return (RSVP_PROTO);
  else if (! pal_strncmp (str, "p", 1))
    return (PIM_PROTO);
  else if (! pal_strncmp (str, "u", 1))
    return (UDP_PROTO);
  else if (! pal_strncmp (str, "v", 1))
    return (VRRP_PROTO);
  else if (! pal_strncmp (str, "t", 1))
    return (TCP_PROTO);
  else
    {
      int ret;
      int val = cmd_str2int (str, &ret);
      if (ret > -1)
        return val;
    }
  return (-1);
}

s_int16_t
operation_type (const char *str)
{
  if (! pal_strncmp (str, "eq", 2))
    return (EQUAL);
  else if (! pal_strncmp (str, "ne", 2))
    return (NOT_EQUAL);
  else if (! pal_strncmp (str, "lt", 2))
    return (LESS_THAN);
  else if (! pal_strncmp (str, "gt", 2))
    return (GREATER_THAN);
  else if (! pal_strncmp (str, "ra", 2))
    return (RANGE);

  return -1;
}

struct access_master *
access_master_get (struct apn_vr *vr, afi_t afi)
{
  if (afi == AFI_IP)
    return (&(vr->access_master_ipv4));
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    return (&(vr->access_master_ipv6));
#endif /* HAVE_IPV6 */

  return NULL;
}

/* Allocate new filter structure. */
struct filter_list *
filter_new ()
{
  struct filter_list *new;

  new = XCALLOC(MTYPE_ACCESS_FILTER, sizeof (struct filter_list));
  return (new);
}

void
filter_free (struct filter_list *filter)
{
  XFREE (MTYPE_ACCESS_FILTER, filter);
}

/* Return string of filter_type. */
static const char *
filter_type_str (struct filter_list *filter)
{
  switch (filter->type)
    {
    case FILTER_PERMIT:
      return ("permit");
    case FILTER_DENY:
      return ("deny");
    case FILTER_DYNAMIC:
      return ("dynamic");
    case FILTER_NO_MATCH:
      return ("no-match");
    default:
      return ("");
    }
}

int
_filter_cmd_ret(struct cli *cli, int res)
{
  char *str = NULL;

  switch (res)
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
  case LIB_API_SET_ERR_INVALID_PREFIX_RANGE:
    str = "Invalid prefix range -- make sure: len < ge-value <= le-value";
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
  case LIB_API_SET_ADDR_AND_MASK_DO_NOT_MATCH:
    str = "IP address contains ignored bits";
    break;
  case LIB_API_SET_ERR_DIFF_ACL_TYPE_PACOS_EXT:
    str =  "Duplicate name: extended pacos access list with this name already exists";
    break;
  case LIB_API_SET_ERR_ACL:
    str =  "Error during procesing of command";
    break;
  case LIB_API_SET_ERR_ACL_CREATION:
    str =  "Can't create access-list";
    break;
  case LIB_API_SET_ERR_ACL_DUPLICATE:
    str =  "Duplicate name: standard pacos access list "
            "with this name already exists";
    break;
  case LIB_API_SET_ERR_ACL_DELETED:
    str =  "Access-list not found";
    break;
  case LIB_API_SET_ERR_ACL_FILTER_DELETED:
    str =  "Access-list filter not found";
    break;
  case LIB_API_SET_ERR_ACL_FOUND_OTHER_FILTER:
    str =  "A list with the same name but different type exists";
    break;
  case LIB_API_SET_ERR_ACL_FOUND_OTHER_TYPE:
    str =  "Same filter but with the opposity access type exists";
    break;
  case LIB_API_SET_ERR_BAD_KERNEL_RULE:
    str =  "Failure to install kernel rule";
    break;
  default:
    str =  "Unspecified error type";
    break;
  }
  cli_out (cli, "%% %s\n", str);
  return CLI_ERROR;
}

/* If filter match to the prefix then return 1. */
static result_t
filter_match_common (struct filter_list *mfilter, struct prefix *p)
{
  struct filter_common *filter;
  struct pal_in4_addr mask;
  u_int32_t check_addr;
  u_int32_t check_mask;

  if (p->family != AF_INET)
    return (0);

  filter = &mfilter->u.cfilter;
  check_addr = p->u.prefix4.s_addr & ~filter->addr_mask.s_addr;

  if (filter->extended)
    {
      masklen2ip (p->prefixlen, &mask);
      check_mask = mask.s_addr & ~filter->mask_mask.s_addr;

      if (pal_mem_cmp (&check_addr, &filter->addr.s_addr, 4) == 0
          && pal_mem_cmp (&check_mask, &filter->mask.s_addr, 4) == 0)
        return (1);
    }
  else if (pal_mem_cmp (&check_addr, &filter->addr.s_addr, 4) == 0)
    return (1);

  return (0);
}

/* Excute a customised function to obtain the result */
static result_t
filter_match_common_custom (struct filter_list *mfilter,
                            result_t (* cust_func) (void *, void *),
                            void *object)
{
  struct prefix p;
  result_t ret;

  ret = PAL_FALSE;

  /* Prepare a 'prefix' structure */
  filter_get_prefix (mfilter, &p);

  if (cust_func)
    ret = cust_func (object, (void *) &p);

  return (ret);
}

/* If filter match to the prefix then return 1. */
static result_t
filter_match_pacos (struct filter_list *mfilter, struct prefix *p)
{
  struct filter_pacos *filter;

  filter = &mfilter->u.zfilter;

  if (filter->prefix.family == p->family)
    {
      if (filter->exact)
        {
          if (filter->prefix.prefixlen == p->prefixlen)
            return (prefix_match (&filter->prefix, p));
          else
            return (0);
        }
      else
        return (prefix_match (&filter->prefix, p));
    }
  else
    return (0);
}

/* Excute a customised function to obtain the result */
static result_t
filter_match_pacos_custom (struct filter_list *mfilter,
                           result_t (* cust_func) (void *, void *),
                           void *object)
{
  struct prefix p;
  result_t ret;

  ret = PAL_FALSE;

  /* Prepare a 'prefix' structure */
  filter_get_prefix (mfilter, &p);

  if (cust_func)
    ret = cust_func (object, (void *) &p);

  return (ret);
}

/* Apply filter list to object (which should be struct prefix *). */
enum filter_type
filter_list_type_apply (struct filter_list *filter, void *object,
                        enum filter_type type)
{
  struct prefix *p;

  p = (struct prefix *) object;

  if (filter == NULL)
    return (type);

  if (filter->common == FILTER_COMMON)
    {
      if (filter_match_common (filter, p))
        return (filter->type);
    }
  else if (filter->common == FILTER_PACOS)
    {
      if (filter_match_pacos (filter, p))
        return (filter->type);
    }
  return (type);
}


/* Allocate new access list structure. */
struct access_list *
access_list_new ()
{
  struct access_list *new;

  new = XCALLOC(MTYPE_ACCESS_LIST, sizeof (struct access_list));
  return (new);
}

/* Free allocated access_list. */
void
access_list_free (struct access_list *access)
{
  struct filter_list *filter;
  struct filter_list *next;

  for (filter = access->head; filter; filter = next)
    {
      next = filter->next;
      filter_free (filter);
    }

  if (access->name)
    XFREE (MTYPE_ACCESS_LIST_STR, access->name);

  if (access->remark)
    XFREE (MTYPE_TMP, access->remark);

  XFREE (MTYPE_ACCESS_LIST, access);
}

struct access_list *
access_list_lock (struct access_list *access)
{
  access->ref_cnt++;
  return (access);
}

void
access_list_unlock (struct access_list *access)
{
  access->ref_cnt--;
  if (access->ref_cnt == 0)
    access_list_free (access);
}

/* Delete access_list from access_master and free it. */
void
access_list_delete (struct access_list *access)
{
  struct access_list_list *dlist;
  struct access_master *master;

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

  access_list_unlock (access);
}

/* Insert new access list to list of access_list.  Each acceess_list
   is sorted by the name. */
struct access_list *
access_list_insert (struct apn_vr *vr, afi_t afi, const char *name)
{
  int i;
  long number;
  struct access_list *access;
  struct access_list *point;
  struct access_list_list *alist;
  struct access_master *master;

  master = access_master_get (vr, afi);
  if (master == NULL)
    return (NULL);

  /* Allocate new access_list and copy given name. */
  access = access_list_new ();
  access->name = XSTRDUP (MTYPE_ACCESS_LIST_STR, name);
  access->master = master;
  access_list_lock (access);

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
      access->type = ACCESS_TYPE_NUMBER;

      /* Set access_list to number list. */
      alist = &master->num;

      for (point = alist->head; point; point = point->next)
        if (pal_strtos32(point->name,NULL,10) >= number)
          break;
    }
  else
    {
      access->type = ACCESS_TYPE_STRING;

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
      return (access);
    }

  /* In case of insertion is made at the tail of access_list. */
  if (point == NULL)
    {
      access->prev = alist->tail;
      alist->tail->next = access;
      alist->tail = access;
      return (access);
    }

  /* In case of insertion is made at the head of access_list. */
  if (point == alist->head)
    {
      access->next = alist->head;
      alist->head->prev = access;
      alist->head = access;
      return (access);
    }

  /* Insertion is made at middle of the access_list. */
  access->next = point;
  access->prev = point->prev;

  if (point->prev)
    point->prev->next = access;
  point->prev = access;

  return (access);
}

/* Lookup access_list from list of access_list by name. */
struct access_list *
access_list_lookup (struct apn_vr *vr, afi_t afi, const char *name)
{
  struct access_list *access;
  struct access_master *master;

  if (name == NULL)
    return (NULL);

  master = access_master_get (vr, afi);
  if (master == NULL)
    return (NULL);

  for (access = master->num.head; access; access = access->next)
    if (pal_strcmp (access->name, name) == 0)
      return (access);

  for (access = master->str.head; access; access = access->next)
    if (pal_strcmp (access->name, name) == 0)
      return (access);

  return (NULL);
}

/* Validate reference to access_list as number/name */
bool_t
access_list_reference_validate (char *ref)
{
#ifdef HAVE_IPV6
  struct pal_in6_addr in6_addr;
#endif /* HAVE_IPV6 */
  struct pal_in4_addr in_addr;
  u_int32_t val;
  char *endptr;
  bool_t ret;

  ret = PAL_TRUE;
  endptr = NULL;

  /* Access-list reference cannot be a NULL string */
  if (! ref || ! pal_strlen (ref))
    ret = PAL_FALSE;
  /* Access-list reference cannot be a valid IP/IP6 Address */
  else if (pal_inet_pton (AF_INET, ref, &in_addr) > 0
#ifdef HAVE_IPV6
           || pal_inet_pton (AF_INET6, ref, &in6_addr) > 0
#endif /* HAVE_IPV6 */
           )
    ret = PAL_FALSE;
  /* Validate access-list number, if so */
  else
    {
      val = pal_strtou32 (ref, &endptr, 10);

      if (*endptr == '\0' || *endptr == '\n')
        {
          if (!((val >= 1 && val <= 199)
                || (val >= 1300 && val <= 2699)))
            ret = PAL_FALSE;
        }
    }
  return (ret);
}

/* Get access list from list of access_list.  If there isn't matched
   access_list create new one and return it. */
struct access_list *
access_list_get (struct apn_vr *vr, afi_t afi, const char *name)
{
  struct access_list *access;

  access = access_list_lookup (vr, afi, name);
  if (access == NULL)
    access = access_list_insert (vr, afi, name);
  return (access);
}

/* Apply access list to object (which should be struct prefix *). */
enum filter_type
access_list_apply (struct access_list *access, void *object)
{
  struct filter_list *filter;
  struct prefix *p;

  p = (struct prefix *) object;

  if (! access)
    return (FILTER_NO_MATCH);

  for (filter = access->head; filter; filter = filter->next)
    {
      if (filter->common == FILTER_COMMON)
        {
          if (filter_match_common (filter, p))
            return (filter->type);
        }
      else if (filter->common == FILTER_PACOS)
        {
          if (filter_match_pacos (filter, p))
            return (filter->type);
        }
    }

  return (FILTER_NO_MATCH);
}

/* Apply access list to object (which should be struct prefix *). */
enum filter_type
access_list_custom_apply (struct access_list *access,
                          result_t (* cust_func) (void *, void *),
                          void *object)
{
  struct filter_list *filter;

  if (! access)
    return (FILTER_NO_MATCH);

  for (filter = access->head; filter; filter = filter->next)
    {
      if (filter->common == FILTER_COMMON)
        {
          if (filter_match_common_custom (filter, cust_func, object))
            return (filter->type);
        }
      else if (filter->common == FILTER_PACOS)
        {
          if (filter_match_pacos_custom (filter, cust_func, object))
            return (filter->type);
        }
    }

  return (FILTER_DENY);
}

/* Add hook function. */
void
access_list_add_hook (struct apn_vr *vr,
                      void (*func) (struct apn_vr *,
                                    struct access_list *,
                                    struct filter_list *))
{
  vr->access_master_ipv4.add_hook = func;
#ifdef HAVE_IPV6
  vr->access_master_ipv6.add_hook = func;
#endif /* HAVE_IPV6 */
}

/* Delete hook function. */
void
access_list_delete_hook (struct apn_vr *vr,
                         void (*func) (struct apn_vr *,
                                       struct access_list *,
                                       struct filter_list *))
{
  vr->access_master_ipv4.delete_hook = func;
#ifdef HAVE_IPV6
  vr->access_master_ipv6.delete_hook = func;
#endif /* HAVE_IPV6 */
}

/* Registering the application notification callback. */
result_t
filter_set_ntf_cb (struct lib_globals *zg, filter_ntf_cb_t ntf_cb)
{
  zg->lib_acl_ntf_cb = ntf_cb;
  return LIB_API_SET_SUCCESS;
}

/* Add new filter to the end of specified access_list. */
int
access_list_filter_add (struct apn_vr *vr,
                        struct access_list *access,
                        struct filter_list *filter)
{
  int i = 0;
  struct filter_list *f;

  for (f = access->head; f; f = f->next)
    i++;

  if (i >= vr->access_master_ipv4.max_count)
    {
      filter_free (filter);
      return (LIB_API_SET_ERR_EXCEED_LIMIT);
    }

  filter->next = NULL;
  filter->prev = access->tail;

  if (access->tail)
    access->tail->next = filter;
  else
    access->head = filter;
  access->tail = filter;

  /* Run hook function. */
  if (access->master->add_hook)
    (*access->master->add_hook) (vr, access, filter);

  if (vr->zg && vr->zg->lib_acl_ntf_cb)
    return (vr->zg->lib_acl_ntf_cb (vr, access, filter, FILTER_OPCODE_CREATE));

  return (LIB_API_SET_SUCCESS);
}

/* If access_list has no filter then return 1. */
static result_t
access_list_empty (struct access_list *access)
{
  if (access->head == NULL && access->tail == NULL)
    return (1);
  else
    return (0);
}

/* Delete filter from specified access_list.
   If there is hook function execute it.
   Return the given pointer to access_list if the list remains not empty.
   Otherwise return NULL.
*/
struct access_list *
access_list_filter_delete (struct apn_vr *vr,
                           struct access_list *access,
                           struct filter_list *filter)
{
  struct access_list *ret_val=access;
  struct access_master *master;

  master = access->master;

  if (filter->next)
    filter->next->prev = filter->prev;
  else
    access->tail = filter->prev;

  if (filter->prev)
    filter->prev->next = filter->next;
  else
    access->head = filter->next;

  /* If access_list becomes empty delete it from access_master. */
  access_list_lock (access);
  if (access_list_empty (access))
    {
      access_list_delete (access);
      ret_val=NULL;
    }

  /* Run hook function. */
  if (master->delete_hook)
    (*master->delete_hook) (vr, access, filter);

  if (vr->zg && vr->zg->lib_acl_ntf_cb)
    vr->zg->lib_acl_ntf_cb (vr, access, filter, FILTER_OPCODE_DELETE);

  access_list_unlock (access);
  filter_free (filter);
  return ret_val;
}

/* Get prefix of common filter */
void
filter_common_get_prefix (struct filter_list *filter, struct prefix *p)
{
  struct pal_in4_addr mask;

  /* Set the prefix family */
  p->family = AF_INET; /* Common filters are IPv4 only */

  /* Set the prefix len */
  mask.s_addr = ~(filter->u.cfilter.addr_mask.s_addr);
  p->prefixlen = ip_masklen (mask);

  /* Get the address */
  p->u.prefix4.s_addr = filter->u.cfilter.addr.s_addr & ~
    (filter->u.cfilter.addr_mask.s_addr);

  return;
}

/* Get prefix of pacos filter */
void
filter_pacos_get_prefix (struct filter_list *filter, struct prefix *p)
{
  *p = filter->u.zfilter.prefix;
  return;
}

/* Get prefix of extended pacos filter */
void
filter_ext_pacos_get_prefix (struct filter_list *filter, struct prefix_am *p)
{
  *p = filter->u.zextfilter.sprefix;
  return;
}

/* Get the prefix of the filter */
void
filter_get_prefix (struct filter_list *filter, struct prefix *p)
{
  /* Get the prefix based on filter type */
  switch (filter->common)
    {
    case FILTER_COMMON:
      filter_common_get_prefix (filter, p);
      break;

    case FILTER_PACOS:
      filter_pacos_get_prefix (filter, p);
      break;
/* Incompatible prefix structure
    case FILTER_PACOS_EXT:
      filter_ext_pacos_get_prefix (filter, p);
      break;
*/
    }
}

/* Return the prefix of the first filter in the access list */
struct filter_list *
access_list_first_filter_prefix (struct access_list *al, struct prefix *p)
{
  struct filter_list *filter;

  if (al == NULL || p == NULL)
    return (NULL);

  if ((filter = al->head) == NULL)
    return (NULL);

  filter_get_prefix (filter, p);

  return (filter);
}

/* Return the prefix of the next filter in the access list */
struct filter_list *
access_list_next_filter_prefix (struct filter_list *f, struct prefix *p)
{
  struct filter_list *filter;

  if (f == NULL || p == NULL)
    return (NULL);

  if ((filter = f->next) == NULL)
    return (NULL);

  filter_get_prefix (filter, p);

  return (filter);
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


struct filter_list *
filter_lookup_common (struct access_list *access, struct filter_list *mnew)
{
  struct filter_list *mfilter;
  struct filter_common *filter;
  struct filter_common *new;

  new = &mnew->u.cfilter;

  for (mfilter = access->head; mfilter; mfilter = mfilter->next)
    {
      filter = &mfilter->u.cfilter;

      if (filter->extended)
        {
          if (mfilter->type == mnew->type
              && filter->addr.s_addr == new->addr.s_addr
              && filter->addr_mask.s_addr == new->addr_mask.s_addr
              && filter->mask.s_addr == new->mask.s_addr
              && filter->mask_mask.s_addr == new->mask_mask.s_addr)
            return (mfilter);
        }
      else
        {
          if (mfilter->type == mnew->type
              && filter->addr.s_addr == new->addr.s_addr
              && filter->addr_mask.s_addr == new->addr_mask.s_addr)
            return (mfilter);
        }
    }

  return (NULL);
}

struct filter_list *
filter_lookup_pacos (struct access_list *access, struct filter_list *mnew)
{
  struct filter_list *mfilter;
  struct filter_pacos *filter;
  struct filter_pacos *new;

  new = &mnew->u.zfilter;

  for (mfilter = access->head; mfilter; mfilter = mfilter->next)
    {
      filter = &mfilter->u.zfilter;

      if (filter->exact == new->exact
          && mfilter->type == mnew->type
          && prefix_same (&filter->prefix, &new->prefix))
        return (mfilter);
    }
  return (NULL);
}

struct filter_list *
filter_lookup_pacos_extended (struct access_list *access,
                              struct filter_list *mnew)
{
  struct filter_list *mfilter;
  struct filter_pacos_ext *filter;
  struct filter_pacos_ext *new;

  new = &mnew->u.zextfilter;

  for (mfilter = access->head; mfilter; mfilter = mfilter->next)
    {
      filter = &mfilter->u.zextfilter;

      /* This is just for the developer mental health purpose
          to find anything in this jungle using gdb.
      */
      if (filter->protocol == new->protocol)
        if (prefix_am_same (&filter->sprefix, &new->sprefix))
          if (filter->sport_op == new->sport_op)
            if (filter->sport == new->sport)
              if (filter->sport_lo == new->sport_lo)
                if (prefix_am_same (&filter->dprefix, &new->dprefix))
                  if (filter->dport_op == new->dport_op)
                    if (filter->dport == new->dport)
                      if (filter->dport_lo == new->dport_lo)
                        if (filter->label == new->label)
                          if (filter->icmp_type == new->icmp_type)
                            if (filter->established == new->established)
                              if (filter->precedence == new->precedence)
                                if (filter->tos_op == new->tos_op)
                                  if (filter->tos == new->tos)
                                    if (filter->pkt_size_op == new->pkt_size_op)
                                      if (filter->pkt_size == new->pkt_size)
                                        if (filter->pkt_size_lo == new->pkt_size_lo)
                                          if (filter->fragments == new->fragments)
                                            if (filter->log == new->log)
                                              if ((filter->ifdir == new->ifdir)
                                                  && ((filter->ifdir == IFDIR_NONE) ||
                                                      !pal_strcmp(filter->ifname,
                                                                  new->ifname)))
                                                return (mfilter);
    }
  return NULL;
}

int
filter_set_common (struct apn_vr *vr, char *name_str, int type,
                   char *addr_str, char *addr_mask_str,
                   char *mask_str, char *mask_mask_str,
                   int extended, int set)
{
  result_t ret;
  struct filter_list *mfilter;
  struct filter_common *filter;
  struct access_list *access;
  struct pal_in4_addr addr;
  struct pal_in4_addr addr_mask;
  struct pal_in4_addr mask;
  struct pal_in4_addr mask_mask;
  int access_num;
  char *endptr = NULL;

  if (type != FILTER_PERMIT && type != FILTER_DENY)
    return (LIB_API_SET_ERR_INVALID_VALUE);

  /* Access-list name check. */
  access_num = pal_strtou32 (name_str, &endptr, 10);
  if (access_num == UINT32_MAX || *endptr != '\0')
    return (LIB_API_SET_ERR_INVALID_VALUE);

  if (extended)
    {
      /* Extended access-list name range check. */
      if (access_num < 100 || access_num > 2699)
        return (LIB_API_SET_ERR_INVALID_VALUE);
      if (access_num > 199 && access_num < 2000)
        return (LIB_API_SET_ERR_INVALID_VALUE);
    }
  else
    {
      /* Standard access-list name range check. */
      if (access_num < 1 || access_num > 1999)
        return (LIB_API_SET_ERR_INVALID_VALUE);
      if (access_num > 99 && access_num < 1300)
        return (LIB_API_SET_ERR_INVALID_VALUE);
    }

  ret = pal_inet_pton (AF_INET, addr_str, (void*)&addr);
  if (ret <= 0)
    return (LIB_API_SET_ERR_MALFORMED_ADDRESS);

  ret = pal_inet_pton (AF_INET, addr_mask_str, (void*)&addr_mask);
  if (ret <= 0)
    return (LIB_API_SET_ERR_MALFORMED_ADDRESS);

  /* Address should fit the valid mask bits. */
  if (addr.s_addr != (addr.s_addr & ~addr_mask.s_addr))
    return (LIB_API_SET_ADDR_AND_MASK_DO_NOT_MATCH);
  
  if (extended)
    {
      ret = pal_inet_pton (AF_INET, mask_str, (void*)&mask);
      if (ret <= 0)
        return (LIB_API_SET_ERR_MALFORMED_ADDRESS);

      ret = pal_inet_pton (AF_INET, mask_mask_str, (void*)&mask_mask);
      if (ret <= 0)
        return (LIB_API_SET_ERR_MALFORMED_ADDRESS);

      /* Address should fit the valid mask bits. */
      if (mask.s_addr != (mask.s_addr & ~mask_mask.s_addr))
        return (LIB_API_SET_ADDR_AND_MASK_DO_NOT_MATCH);
    }

  mfilter = filter_new ();
  mfilter->type = type;
  mfilter->common = FILTER_COMMON;
  filter = &mfilter->u.cfilter;
  filter->extended = extended;
  filter->addr.s_addr = addr.s_addr;
  filter->addr_mask.s_addr = addr_mask.s_addr;

  if (extended)
    {
      filter->mask.s_addr = mask.s_addr;
      filter->mask_mask.s_addr = mask_mask.s_addr;
    }

  /* Install new filter to the access_list. */
  if (set)
    {
      access = access_list_get (vr, AFI_IP, name_str);
      if (!access)
        {
          filter_free (mfilter);
          return LIB_API_SET_ERROR;
        }
      if (filter_lookup_common (access, mfilter))
        filter_free (mfilter);
      else
        return access_list_filter_add (vr, access, mfilter);
    }
  else
    {
      struct filter_list *delete_filter;

      access = access_list_lookup (vr, AFI_IP, name_str);
      if (!access)
        {
          filter_free (mfilter);
          return LIB_API_SET_ERR_UNKNOWN_OBJECT;
        }

      delete_filter = filter_lookup_common (access, mfilter);
      if (delete_filter)
        access_list_filter_delete (vr, access, delete_filter);

      filter_free (mfilter);
    }

  return (LIB_API_SET_SUCCESS);
}


/* CLIs. */
#define CLI_GET_FILTER_TYPE(T,STR)                                            \
    do {                                                                      \
      /* Check of filter type. */                                             \
      if (pal_strncmp ((STR), "p", 1) == 0)                                   \
        (T) = FILTER_PERMIT;                                                  \
      else if (pal_strncmp ((STR), "d", 1) == 0)                              \
        (T) = FILTER_DENY;                                                    \
      else                                                                    \
        return LIB_API_SET_ERR_INVALID_VALUE;                                 \
    } while (0)

#define STANDARD_ACCESS_LIST_HELP_STR                                        \
       "IP standard access list",                                            \
       "IP standard access list (expanded range)",                           \
       CLI_ACCESS_DENY_STR,                                                  \
       CLI_ACCESS_PERMIT_STR

/* Standard access-list */
CLI (access_list_standard,
     access_list_standard_cli,
     "access-list (<1-99>|<1300-1999>) (deny|permit) A.B.C.D A.B.C.D",
     CLI_ACCESS_STR,
     STANDARD_ACCESS_LIST_HELP_STR,
     "Address to match",
     "Wildcard bits")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_standard_set (cli->vr, argv[0], type, argv[2], argv[3]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (maximum_access_list,
     maximum_access_list_cli,
     "maximum-access-list <1-4294967294>",
     "Maximum access-list entries",
     "Access list limit")
{
  struct apn_vr *vr = cli->vr;
  u_int32_t num;

  CLI_GET_INTEGER_RANGE ("max access-list limit", num, argv[0], 1, ACCESS_LIST_ENTRY_MAX);
  vr->access_master_ipv4.max_count = num;

  return (CLI_SUCCESS);
}

CLI (no_maximum_access_list,
     no_maximum_access_list_cli,
     "no maximum-access-list",
     CLI_NO_STR,
     "Maximum access-list entries")
{
   struct apn_vr *vr = cli->vr;

   vr->access_master_ipv4.max_count = ACCESS_LIST_ENTRY_MAX;

   return (CLI_SUCCESS);
}

CLI (access_list_standard_nomask,
     access_list_standard_nomask_cli,
     "access-list (<1-99>|<1300-1999>) (deny|permit) A.B.C.D",
     CLI_ACCESS_STR,
     STANDARD_ACCESS_LIST_HELP_STR,
     "Address to match")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_standard_set (cli->vr, argv[0], type, argv[2], "0.0.0.0");

  return (_filter_cmd_ret (cli, ret));
}

ALI (access_list_standard_nomask,
     access_list_standard_host_cli,
     "access-list (<1-99>|<1300-1999>) (deny|permit) host A.B.C.D",
     CLI_ACCESS_STR,
     STANDARD_ACCESS_LIST_HELP_STR,
     "A single host address",
     "Address to match");

CLI (access_list_standard_any,
     access_list_standard_any_cli,
     "access-list (<1-99>|<1300-1999>) (deny|permit) any",
     CLI_ACCESS_STR,
     STANDARD_ACCESS_LIST_HELP_STR,
     "Any source host")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_standard_set (cli->vr, argv[0], type,
                                  "0.0.0.0", "255.255.255.255");

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_access_list_standard,
     no_access_list_standard_cli,
     "no access-list (<1-99>|<1300-1999>) (deny|permit) A.B.C.D A.B.C.D",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     STANDARD_ACCESS_LIST_HELP_STR,
     "Address to match",
     "Wildcard bits")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_standard_unset (cli->vr, argv[0], type, argv[2], argv[3]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_access_list_standard_nomask,
     no_access_list_standard_nomask_cli,
     "no access-list (<1-99>|<1300-1999>) (deny|permit) A.B.C.D",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     STANDARD_ACCESS_LIST_HELP_STR,
     "Address to match")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_standard_unset (cli->vr, argv[0], type,
                                    argv[2], "0.0.0.0");

  return (_filter_cmd_ret (cli, ret));
}

ALI (no_access_list_standard_nomask,
     no_access_list_standard_host_cli,
     "no access-list (<1-99>|<1300-1999>) (deny|permit) host A.B.C.D",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     STANDARD_ACCESS_LIST_HELP_STR,
     "A single host address",
     "Address to match");

CLI (no_access_list_standard_any,
     no_access_list_standard_any_cli,
     "no access-list (<1-99>|<1300-1999>) (deny|permit) any",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     STANDARD_ACCESS_LIST_HELP_STR,
     "Any source host")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_standard_unset (cli->vr, argv[0], type,
                                    "0.0.0.0", "255.255.255.255");

  return (_filter_cmd_ret (cli, ret));
}

#define EXTENDED_ACCESS_LIST_HELP_STR                                       \
       "IP extended access list",                                           \
       "IP extended access list (expanded range)",                          \
       CLI_ACCESS_DENY_STR,                                         \
       CLI_ACCESS_PERMIT_STR

/* Extended access-list */
CLI (access_list_extended,
     access_list_extended_cli,
     "access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D A.B.C.D A.B.C.D",
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "Destination address",
     "Destination Wildcard bits")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_set (cli->vr, argv[0], type,
                                  argv[2], argv[3], argv[4], argv[5]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (access_list_extended_mask_any,
     access_list_extended_mask_any_cli,
     "access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D any",
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "Any destination host")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_set (cli->vr, argv[0], type, argv[2], argv[3],
                                  "0.0.0.0", "255.255.255.255");

  return (_filter_cmd_ret (cli, ret));
}

CLI (access_list_extended_any_mask,
     access_list_extended_any_mask_cli,
     "access-list (<100-199>|<2000-2699>) (deny|permit) ip any A.B.C.D A.B.C.D",
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Any source host",
     "Destination address",
     "Destination Wildcard bits")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_set (cli->vr, argv[0], type, "0.0.0.0",
                                  "255.255.255.255", argv[2], argv[3]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (access_list_extended_any_any,
     access_list_extended_any_any_cli,
     "access-list (<100-199>|<2000-2699>) (deny|permit) ip any any",
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Any source host",
     "Any destination host")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_set (cli->vr, argv[0], type,
                                  "0.0.0.0", "255.255.255.255",
                                  "0.0.0.0", "255.255.255.255");

  return (_filter_cmd_ret (cli, ret));
}

CLI (access_list_extended_mask_host,
     access_list_extended_mask_host_cli,
     "access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D host A.B.C.D",
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "A single destination host",
     "Destination address")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_set (cli->vr, argv[0], type,
                                  argv[2], argv[3], argv[4], "0.0.0.0");

  return (_filter_cmd_ret (cli, ret));
}

CLI (access_list_extended_host_mask,
     access_list_extended_host_mask_cli,
     "access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D A.B.C.D A.B.C.D",
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "Destination address",
     "Destination Wildcard bits")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_set (cli->vr, argv[0], type,
                                  argv[2], "0.0.0.0", argv[3], argv[4]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (access_list_extended_host_host,
     access_list_extended_host_host_cli,
     "access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D host A.B.C.D",
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "A single destination host",
     "Destination address")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_set (cli->vr, argv[0], type,
                                  argv[2], "0.0.0.0", argv[3], "0.0.0.0");

  return (_filter_cmd_ret (cli, ret));
}

CLI (access_list_extended_any_host,
     access_list_extended_any_host_cli,
     "access-list (<100-199>|<2000-2699>) (deny|permit) ip any host A.B.C.D",
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Any source host",
     "A single destination host",
     "Destination address")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_set (cli->vr, argv[0], type, "0.0.0.0",
                                  "255.255.255.255", argv[2], "0.0.0.0");

  return (_filter_cmd_ret (cli, ret));
}

CLI (access_list_extended_host_any,
     access_list_extended_host_any_cli,
     "access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D any",
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "Any destination host")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_set (cli->vr, argv[0], type, argv[2],
                                  "0.0.0.0", "0.0.0.0", "255.255.255.255");

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_access_list_extended,
     no_access_list_extended_cli,
     "no access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D A.B.C.D A.B.C.D",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "Destination address",
     "Destination Wildcard bits")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_unset (cli->vr, argv[0], type,
                                    argv[2], argv[3], argv[4], argv[5]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_access_list_extended_mask_any,
     no_access_list_extended_mask_any_cli,
     "no access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D any",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "Any destination host")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_unset (cli->vr, argv[0], type, argv[2], argv[3],
                                    "0.0.0.0", "255.255.255.255");

  return (_filter_cmd_ret (cli, ret));
}


CLI (no_access_list_extended_any_mask,
     no_access_list_extended_any_mask_cli,
     "no access-list (<100-199>|<2000-2699>) (deny|permit) ip any A.B.C.D A.B.C.D",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Any source host",
     "Destination address",
     "Destination Wildcard bits")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_unset (cli->vr, argv[0], type, "0.0.0.0",
                                    "255.255.255.255", argv[2], argv[3]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_access_list_extended_any_any,
     no_access_list_extended_any_any_cli,
     "no access-list (<100-199>|<2000-2699>) (deny|permit) ip any any",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Any source host",
     "Any destination host")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_unset (cli->vr, argv[0], type,
                                    "0.0.0.0", "255.255.255.255",
                                    "0.0.0.0", "255.255.255.255");

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_access_list_extended_mask_host,
     no_access_list_extended_mask_host_cli,
     "no access-list (<100-199>|<2000-2699>) (deny|permit) ip A.B.C.D A.B.C.D host A.B.C.D",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Source address",
     "Source wildcard bits",
     "A single destination host",
     "Destination address")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_unset (cli->vr, argv[0], type, argv[2], argv[3],
                                    argv[4], "0.0.0.0");

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_access_list_extended_host_mask,
     no_access_list_extended_host_mask_cli,
     "no access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D A.B.C.D A.B.C.D",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "Destination address",
     "Destination Wildcard bits")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_unset (cli->vr, argv[0], type, argv[2],
                                    "0.0.0.0", argv[3], argv[4]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_access_list_extended_host_host,
     no_access_list_extended_host_host_cli,
     "no access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D host A.B.C.D",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "A single destination host",
     "Destination address")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_unset (cli->vr, argv[0], type, argv[2],
                                    "0.0.0.0", argv[3], "0.0.0.0");

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_access_list_extended_any_host,
     no_access_list_extended_any_host_cli,
     "no access-list (<100-199>|<2000-2699>) (deny|permit) ip any host A.B.C.D",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "Any source host",
     "A single destination host",
     "Destination address")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_unset (cli->vr, argv[0], type, "0.0.0.0",
                                    "255.255.255.255", argv[2], "0.0.0.0");

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_access_list_extended_host_any,
     no_access_list_extended_host_any_cli,
     "no access-list (<100-199>|<2000-2699>) (deny|permit) ip host A.B.C.D any",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     EXTENDED_ACCESS_LIST_HELP_STR,
     "Any Internet Protocol",
     "A single source host",
     "Source address",
     "Any destination host")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_extended_unset (cli->vr, argv[0], type, argv[2],
                                    "0.0.0.0", "0.0.0.0", "255.255.255.255");

  return (_filter_cmd_ret (cli, ret));
}

int
filter_set_pacos (struct apn_vr *vr, char *name_str, int type,
                  afi_t afi, char *prefix_str, int exact, int set)
{
  result_t ret;
  struct filter_list *mfilter;
  struct filter_pacos *filter;
  struct access_list *access;
  struct prefix p;

  pal_mem_set (&p, 0, sizeof (struct prefix));
  /* Check string format of prefix and prefixlen. */
  if (afi == AFI_IP)
    ret = str2prefix_ipv4 (prefix_str, (struct prefix_ipv4 *)&p);
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    ret = str2prefix_ipv6 (prefix_str, (struct prefix_ipv6 *) &p);
#endif /* HAVE_IPV6 */
  else
    return (LIB_API_SET_ERR_INVALID_VALUE);

  if (ret <= 0)
    return (LIB_API_SET_ERR_MALFORMED_ADDRESS);

  mfilter = filter_new ();
  mfilter->type = type;
  mfilter->common = FILTER_PACOS;
  filter = &mfilter->u.zfilter;
  prefix_copy (&filter->prefix, &p);

  /* "exact-match" */
  if (exact)
    filter->exact = 1;

  /* Install new filter to the access_list. */
  if (set)
    {
      access = access_list_lookup (vr, afi, name_str);
      if (!access)
        {
          access = access_list_insert (vr, afi, name_str);
         if (!access)
           {
             filter_free (mfilter);
             return (LIB_API_SET_ERROR);
           }
        }
      else if (access->head && (access->head->common != FILTER_PACOS))
        {
          filter_free (mfilter);
          return LIB_API_SET_ERR_DIFF_ACL_TYPE_PACOS_EXT;
        }
      if (filter_lookup_pacos (access, mfilter))
        filter_free (mfilter);
      else
        return access_list_filter_add (vr, access, mfilter);
    }
  else
    {
      struct filter_list *delete_filter;

      access = access_list_lookup (vr, afi, name_str);
      if (!access)
        {
          filter_free (mfilter);
          return LIB_API_SET_ERR_UNKNOWN_OBJECT;
        }

      delete_filter = filter_lookup_pacos (access, mfilter);
      if (delete_filter)
        access_list_filter_delete (vr, access, delete_filter);

      filter_free (mfilter);
    }

  return (LIB_API_SET_SUCCESS);
}

/* PacOS access-list */
CLI (access_list,
     access_list_cli,
     "access-list WORD (deny|permit) A.B.C.D/M",
     CLI_ACCESS_STR,
     CLI_ACCESS_PACOS_STR,
     CLI_ACCESS_DENY_STR,
     CLI_ACCESS_PERMIT_STR,
     "Prefix to match. e.g. 10.0.0.0/8")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_pacos_set (cli->vr, argv[0], type, AFI_IP, argv[2]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (access_list_exact,
     access_list_exact_cli,
     "access-list WORD (deny|permit) A.B.C.D/M exact-match",
     CLI_ACCESS_STR,
     CLI_ACCESS_PACOS_STR,
     CLI_ACCESS_DENY_STR,
     CLI_ACCESS_PERMIT_STR,
     "Prefix to match. e.g. 10.0.0.0/8",
     "Exact match of the prefixes")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_pacos_exact_set (cli->vr, argv[0], type, AFI_IP, argv[2]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (access_list_any,
     access_list_any_cli,
     "access-list WORD (deny|permit) any",
     CLI_ACCESS_STR,
     CLI_ACCESS_PACOS_STR,
     CLI_ACCESS_DENY_STR,
     CLI_ACCESS_PERMIT_STR,
     "Any source host")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_pacos_set (cli->vr, argv[0], type, AFI_IP, "0.0.0.0/0");

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_access_list,
     no_access_list_cli,
     "no access-list WORD (deny|permit) A.B.C.D/M",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     CLI_ACCESS_PACOS_STR,
     CLI_ACCESS_DENY_STR,
     CLI_ACCESS_PERMIT_STR,
     "Prefix to match. e.g. 10.0.0.0/8")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_pacos_unset (cli->vr, argv[0], type, AFI_IP, argv[2]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_access_list_exact,
     no_access_list_exact_cli,
     "no access-list WORD (deny|permit) A.B.C.D/M exact-match",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     CLI_ACCESS_PACOS_STR,
     CLI_ACCESS_DENY_STR,
     CLI_ACCESS_PERMIT_STR,
     "Prefix to match. e.g. 10.0.0.0/8",
     "Exact match of the prefixes")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_pacos_exact_unset (cli->vr, argv[0], type,
                                       AFI_IP, argv[2]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_access_list_any,
     no_access_list_any_cli,
     "no access-list WORD (deny|permit) any",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     CLI_ACCESS_PACOS_STR,
     CLI_ACCESS_DENY_STR,
     CLI_ACCESS_PERMIT_STR,
     "Any source host")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_pacos_unset (cli->vr, argv[0], type, AFI_IP, "0.0.0.0/0");

  return (_filter_cmd_ret (cli, ret));
}

#define STANDARD_EXT_ACCESS_LIST_HELP_STR                                   \
       "IP standard access list",                                           \
       "IP extended access list",                                           \
       "IP standard access list (expanded range)",                          \
       "IP extended access list (expanded range)"

CLI (no_access_list_all,
     no_access_list_all_cli,
     "no access-list (<1-99>|<100-199>|<1300-1999>|<2000-2699>|WORD)",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     STANDARD_EXT_ACCESS_LIST_HELP_STR,
     CLI_ACCESS_PACOS_STR)
{
  int ret;

  ret = access_list_unset_by_name (cli->vr, AFI_IP, argv[0]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (access_list_remark,
     access_list_remark_cli,
     "access-list (<1-99>|<100-199>|<1300-1999>|<2000-2699>|WORD) remark LINE",
     CLI_ACCESS_STR,
     STANDARD_EXT_ACCESS_LIST_HELP_STR,
     CLI_ACCESS_PACOS_STR,
     "Access list entry comment",
     "Comment up to 100 characters")
{
  int ret;

  /* Remark Set API function. */
  ret = access_list_remark_set (cli->vr, AFI_IP, argv[0], argv[1]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_access_list_remark,
     no_access_list_remark_cli,
     "no access-list (<1-99>|<100-199>|<1300-1999>|<2000-2699>|WORD) remark",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     STANDARD_EXT_ACCESS_LIST_HELP_STR,
     CLI_ACCESS_PACOS_STR,
     "Access list entry comment")
{
  int ret;

  ret = access_list_remark_unset (cli->vr, AFI_IP, argv[0]);

  return (_filter_cmd_ret (cli, ret));
}

ALI (no_access_list_remark,
     no_access_list_remark_arg_cli,
     "no access-list (<1-99>|<100-199>|<1300-1999>|<2000-2699>|WORD) remark LINE",
     CLI_NO_STR,
     CLI_ACCESS_STR,
     STANDARD_EXT_ACCESS_LIST_HELP_STR,
     CLI_ACCESS_PACOS_STR,
     "Access list entry comment",
     "Comment up to 100 characters");

/* Ipv4 Extended PacOS Access-list CLIs */

static s_int16_t
_filter_type (const char *str)
{
  /* Check of filter type. */
  if (! pal_strncmp (str, "p", 1))
    return FILTER_PERMIT;
  else if (! pal_strncmp (str, "d", 1))
    return FILTER_DENY;
  else
    return (-1);
}

static s_int16_t
_filter_ifdir(const char *str)
{
  /* Check of filter type. */
  if (! pal_strncmp (str, "i", 1))
    return IFDIR_INPUT;
  else if (! pal_strncmp (str, "o", 1))
    return IFDIR_OUTPUT;
  else
    return (-1);
}

 /* Decodes "A.B.C.D/M" or A.B.C.D A.B.C.D" or any or
  * "X:X::X:X/M" or "X:X::X:X X:X::X:X"
  * Returns <= 0 if error, or number of consumed argv's.
  */

static s_int16_t
_filter_decode_prefix(afi_t afi, char *argv[],int arg_cnt, struct prefix_am *sp)
{
  s_int16_t ret = 1;

  pal_mem_set(sp, 0, sizeof(*sp));

  if (afi == AFI_IP)
    {
      if (! pal_strncmp (argv[0], "a", 1))
        prefix_am4_init((struct prefix_am4 *)sp);
      else if (arg_cnt > 1)
        /* We may have "addr mask" format. */
        ret = str2prefix_am4_invert(argv[0], argv[1], (struct prefix_am4 *)sp);
      else
        ret = str2prefix_am4 (argv[0], NULL, (struct prefix_am4 *)sp);
      /* Check mask bits include all address bits. */
      if (ret <= 0)
        return LIB_API_SET_ERR_MALFORMED_ADDRESS;
      if (ret == 2 && prefix_am_check_mask(sp) != 0)
        return LIB_API_SET_ADDR_AND_MASK_DO_NOT_MATCH;
    }
#ifdef HAVE_IPV6
  else if (afi == AFI_IP6)
    {
      if (! pal_strncmp (argv[0], "a", 1))
        prefix_am6_init((struct prefix_am6 *)sp);
      else if (arg_cnt > 1)
        /* We may have "addr mask" format. */
        ret = str2prefix_am6_invert (argv[0], argv[1], (struct prefix_am6 *)sp);
      else
        ret = str2prefix_am6 (argv[0], NULL, (struct prefix_am6 *)sp);
      /* Check mask bits include all address bits. */
      if (ret <= 0)
        return LIB_API_SET_ERR_MALFORMED_ADDRESS;
      if (ret == 2 && prefix_am_check_mask(sp) != 0)
        return LIB_API_SET_ADDR_AND_MASK_DO_NOT_MATCH;
    }
#endif
  return (ret);
}

/* Returns # of consumed tokens. */
static s_int16_t
_filter_decode_port(char *argv[],
                    enum operation *port_op,
                    int  *port,
                    int  *port_lo)
{
  int op;
  if ((op = operation_type(argv[0])) < 0)
    return 0;

  *port_op = op;

  pal_sscanf (argv[1], "%d", port);
  if (*port_op != RANGE)
    return 2;

  /* We are dealing with a range of ports. */
  *port_lo = *port;

  pal_sscanf (argv[2], "%d", port);
  /* Correct the relation if wrong. */
  if (*port_lo > *port)
    return 1;

  return 3;
}

static int
_filter_decode_icmp_type(char *argv[],
                         int *icmp_type)
{
  int val;
  int ret;

  val = cmd_str2int (argv[0], &ret);
  if (ret < 0)
    return (CLI_ERROR);

  /* Check for the icmp-type validity */
  if ((val == ICMP_ECHOREPLY) ||
      (val == ICMP_DEST_UNREACH) ||
      (val == ICMP_SOURCE_QUENCH) ||
      (val == ICMP_REDIRECT) ||
      (val == ICMP_ECHO) ||
      (val == ICMP_TIME_EXCEEDED) ||
      (val == ICMP_PARAMETERPROB) ||
      (val == ICMP_TIMESTAMP) ||
      (val == ICMP_TIMESTAMPREPLY) ||
      (val == ICMP_INFO_REQUEST) ||
      (val == ICMP_INFO_REPLY) ||
      (val == ICMP_ADDRESS) ||
      (val == ICMP_ADDRESSREPLY) ||
      (val == NR_ICMP_TYPES))
    {
      *icmp_type = val;
      return CLI_SUCCESS;
    }
  return (CLI_ERROR);
}


static s_int16_t
_filter_decode_tos(char *argv[],
                   enum operation *tos_op,
                   int  *tos,
                   int  *tos_lo)
{
  int xtos;
  *tos_op = operation_type(argv[0]);
  if (*tos_op == RANGE)
    {
      pal_sscanf(argv[1], "%d", tos_lo);
      pal_sscanf(argv[2], "%d", tos);
      if (*tos_lo > *tos)
        {
          xtos    = *tos_lo;
          *tos_lo = *tos;
          *tos    = xtos;
        }
      return 3;
    }
  else
    { /* No operand here. */
      *tos_op = NOOP;
      pal_sscanf (argv[0], "%d", tos);
      return 1;
    }
}

static s_int16_t
_filter_decode_pkt_size(char *argv[],
                        enum operation *pkt_size_op,
                        int  *pkt_size,
                        int  *pkt_size_lo)
{
  *pkt_size_op = operation_type(argv[0]);
  if (*pkt_size_op == RANGE)
    {
      pal_sscanf (argv[1], "%d", pkt_size_lo);
      pal_sscanf (argv[2], "%d", pkt_size);

      if (*pkt_size_lo > *pkt_size)
        return 0;

      return 3;
    }
  else
    {
      pal_sscanf (argv[1], "%d", pkt_size);
      return 2;
    }
}

static s_int16_t
_filter_decode_interface(char *argv[],
                         enum filter_ifdir *ifdir,
                         char  *ifname)
{
  if (pal_strlen(argv[1]) <= INTERFACE_NAMSIZ)
    {
      *ifdir = _filter_ifdir(argv[0]);
      pal_strcpy(ifname, argv[1]);
      return 2;
    }
  return -1;
}

int
filter_pacos_ext_decode (struct cli       *cli,
                         afi_t             afi,
                         int               argc,
                         char             *argv[],
                         FILTER_PARAM_TYPE tmpl[],
                         char            **acl_name,
                         enum filter_type *filter_type,
                         struct filter_pacos_ext *filter)
{
  int tix = 0;
  int aix = 0;  /* argv index */
  struct prefix_am pfx;
  int res;
  struct lib_globals *zg = cli->zg;
  struct apn_vr      *vr = zg->vr_in_cxt;
  struct interface   *ifp;

  /* First init the filter to default values. */
  pal_mem_set(filter, 0, sizeof(*filter));

  *acl_name    = NULL;
  *filter_type = -1;

  filter->precedence = -1;
  filter->icmp_type  = -1;
  filter->tos_op     = -1;


  while (aix < argc)
    {
      /* The command template is never empty and terminates with FILTER_PAR_END.
         The command template never starts with optional parameter.
       */
      switch (tmpl[tix])
        {
        case FILTER_PAR_NAME:
          /* Positional parameter. */
          *acl_name = argv[aix++];
          break;

        case FILTER_PAR_TYPE:
          /* Positional parameter. */
          *filter_type = _filter_type(argv[aix++]);
          break;

        case FILTER_PAR_PROTO:
          /* Positional parameter. */
          filter->protocol = protocol_type(argv[aix++]);
          break;

        case FILTER_PAR_SRC_ADDR:
          /* Positional parameter. */
          res = _filter_decode_prefix(afi, &argv[aix], argc-aix, &pfx);
          if (res == LIB_API_SET_ERR_MALFORMED_ADDRESS)
            {
              cli_out (cli, "$$ Malformed IP source address\n");
              return (CLI_ERROR);
            }
          else if (res == LIB_API_SET_ADDR_AND_MASK_DO_NOT_MATCH)
            {
            cli_out (cli, "$$ Source IP address contains ignored bits\n");
              return (CLI_ERROR);
            }
          filter->sprefix = pfx;
          aix += res;
          break;

        case FILTER_PAR_SRC_PORT:
          /* We must trust the command parser. This is either a correct
             SRC_PORT entry or DST_ADDR.
          */
          res = _filter_decode_port(&argv[aix],
                                    &filter->sport_op,
                                    &filter->sport,
                                    &filter->sport_lo);
          if (res == 1)
            {
              cli_out (cli, "%% Wrong Src port: Init range value greater than end range value\n");
              return CLI_ERROR;
            }

          /* We consumed 0, 2 or 3 parameters. */
          aix += res;
          break;

        case FILTER_PAR_DST_ADDR:
          /* Positional parameter. */
          res = _filter_decode_prefix(afi, &argv[aix], argc-aix, &pfx);
          if (res == LIB_API_SET_ERR_MALFORMED_ADDRESS)
            {
              cli_out (cli, "$$ Malformed IP destination address\n");
              return (CLI_ERROR);
            }
          else if (res == LIB_API_SET_ADDR_AND_MASK_DO_NOT_MATCH)
            {
              cli_out (cli, "$$ Destination IP address contains ignored bits\n");
              return (CLI_ERROR);
            }
          filter->dprefix = pfx;
          aix += res;
          break;

        case FILTER_PAR_DST_PORT:
          /* Optional parameter. Here yet again we trust the command parser.
            This is either a correct DST_PORT port or we go further.
           */
          res = _filter_decode_port(&argv[aix],
                                    &filter->dport_op,
                                    &filter->dport,
                                    &filter->dport_lo);

          if (res == 1)
            {
              cli_out (cli, "%% Wrong Dst port: Init range value greater than end range value\n");
              return CLI_ERROR;
            }

          /* We consumed 0, 2 or 3 parameters. */
          aix += res;
          break;

        case FILTER_PAR_OPTIONAL:
          /* Optional parameters. */
          if (! pal_strncmp(argv[aix], "icmp-type", 9))
            {
              res = _filter_decode_icmp_type(&argv[aix+1],
                                             &filter->icmp_type);
              if (res != CLI_SUCCESS)
                {
                  cli_out (cli, "%% Wrong ICMP type\n");
                  return CLI_ERROR;
                }
              aix += 2;
              break;
            }
          if (! pal_strncmp(argv[aix], "est", 3))
            {
              filter->established = 1;
              aix++;
              break;
            }
          if (! pal_strncmp(argv[aix], "label", 5))
            {
              pal_sscanf (argv[aix+1], "%d", &filter->label);
              aix += 2;
              break;
            }
          if (! pal_strncmp(argv[aix], "pre", 3))
            {
              pal_sscanf (argv[aix+1], "%d", &filter->precedence);
              aix += 2;
              break;
            }
          if (! pal_strncmp(argv[aix], "tos", 3))
            {
              res = _filter_decode_tos(&argv[aix+1],
                                       &filter->tos_op,
                                       &filter->tos,
                                       &filter->tos_lo);
              /* The decoder function consumed 1 or 3 parameters. */
              aix += res+1;
              break;
            }
          if (! pal_strncmp(argv[aix], "pkt", 3))
            {
              res = _filter_decode_pkt_size(&argv[aix+1],
                                            &filter->pkt_size_op,
                                            &filter->pkt_size,
                                            &filter->pkt_size_lo);
              if (!res)
                {
                  cli_out (cli, "%% Wrong pkt size: Init range value greater than end range value\n");
                  return CLI_ERROR;
                }

              /* The decoder function consumed 2 or 3 parameters. */
              aix += res+1;
              break;
            }
          if (! pal_strncmp(argv[aix], "fra", 3))
            {
              filter->fragments = 1;
              aix++;
              break;
            }
          if (! pal_strncmp(argv[aix], "log", 3))
            {
              filter->log = 1;
              aix++;
              break;
            }
          if (! pal_strncmp(argv[aix], "int", 3))
            {
              if (_filter_decode_interface(&argv[aix+1],
                                           &filter->ifdir,
                                           filter->ifname) < 0)
                {
                  cli_out (cli, "$$ Bad interface selector\n");
                  return (CLI_ERROR);
                }
              if (vr->id == 0)
                ifp = ifg_lookup_by_name (&zg->ifg, filter->ifname);
              else
                ifp = if_lookup_by_name (&vr->ifm, filter->ifname);
              if (! ifp)
                {
                  cli_out (cli, "$$ No such interface\n");
                  return (CLI_ERROR);
                }
              aix += 3;
              break;
            }
          break;

        case FILTER_PAR_END:
          /* In any case, if there is no optional parameter. */
          return CLI_ERROR;

        default:
          cli_out(cli, "%% Internal PacOS Error: Ignored command parameter.");
          return CLI_ERROR;
        }
      /* Ad vance the template cursor but only if it is not on the optional
         parameter position. Once it reaches this position it stays there.
       */
      tix += tmpl[tix] == FILTER_PAR_OPTIONAL ? 0 : 1;
    }
  return (CLI_SUCCESS);
}

/*--------------------------------------------------------------------
 * This is the best true API function we can invent here.
 * Because we are in lib, we need to pass lib_globals pointer as well.
 */
ZRESULT
filter_pacos_ext_add(struct lib_globals *zg,
                     u_int32_t vrid,
                     afi_t     afi,
                     char     *acl_name,
                     enum filter_type flt_type,
                     struct filter_pacos_ext *filter)
{
  struct filter_list *mfilter;
  struct filter_list *found_filter;
  struct access_list *access;
  struct apn_vr *vr;
  int ret = LIB_API_SET_SUCCESS;

  vr = apn_vr_get_by_id (zg, vrid);
  if (! vr)
    return LIB_API_SET_ERR_ACL;

  mfilter = filter_new ();
  mfilter->type = flt_type;
  mfilter->common = FILTER_PACOS_EXT;
  mfilter->u.zextfilter = *filter;

  access = access_list_lookup (vr, afi, acl_name);
  if (!access)
    {
      access = access_list_insert (vr, afi, acl_name);
      if (!access)
        {
          ret = LIB_API_SET_ERR_ACL_CREATION;
          goto _filter_pacos_ext_add_exit;
        }
    }
  else if (access->head->common != FILTER_PACOS_EXT)
    {
      ret = LIB_API_SET_ERR_ACL_DUPLICATE;
      goto _filter_pacos_ext_add_exit;
    }

  if (! (found_filter = filter_lookup_pacos_extended (access, mfilter)))
    {
      /* It will free the mfilter if needed. */
      return access_list_filter_add (vr, access, mfilter);
    }
  else
    {
      if (mfilter->type == found_filter->type)
        ret = LIB_API_SET_SUCCESS;
      else
        ret = LIB_API_SET_ERR_ACL_FOUND_OTHER_TYPE;
    }

_filter_pacos_ext_add_exit:
  filter_free (mfilter);
  return ret;
}

ZRESULT
filter_pacos_ext_del(struct lib_globals *zg,
                     u_int32_t vrid,
                     afi_t     afi,
                     char     *acl_name,
                     enum filter_type flt_type,
                     struct filter_pacos_ext *filter)
{
  struct filter_list *mfilter;
  struct access_list *access;
  struct filter_list *found_filter;
  struct apn_vr *vr;

  vr = apn_vr_get_by_id (zg, vrid);
  if (! vr)
    return LIB_API_SET_ERR_ACL;

  access = access_list_lookup (vr, afi, acl_name);
  if (!access)
    {
#ifdef HAVE_IMI
      if (zg->protocol != APN_PROTO_IMI)
      /* Relaxed rules: if filter does not exists in PM, then return success. */
        return  LIB_API_SET_SUCCESS;
      else
      /* Only IMI will return error. */
#endif
      return LIB_API_SET_ERR_ACL_DELETED;
    }
  /* If access type unspecified, we delete the whole list. */
  if (flt_type == -1)
    {
      while (access)
        {
          mfilter = access->head;
          if (mfilter->common != FILTER_PACOS_EXT)
            return LIB_API_SET_ERR_ACL_FOUND_OTHER_FILTER;
          access = access_list_filter_delete (vr, access, mfilter);
        }
      /* We assume the access_list is gone. */
      return LIB_API_SET_SUCCESS;
    }
  else
    {
      /* Deleting an individual filter. */
      mfilter = filter_new ();
      mfilter->type = flt_type;
      mfilter->common = FILTER_PACOS_EXT;
      mfilter->u.zextfilter = *filter;

      found_filter = filter_lookup_pacos_extended (access, mfilter);
      filter_free (mfilter);
      if (found_filter)
        {
          if (found_filter->common != FILTER_PACOS_EXT)
            return (LIB_API_SET_ERR_ACL_FOUND_OTHER_FILTER);
          if (found_filter->type == flt_type)
            {
              access_list_filter_delete (vr, access, found_filter);
              return (LIB_API_SET_SUCCESS);
            }
        }
#ifdef HAVE_IMI
    if (zg->protocol != APN_PROTO_IMI)
      /* Relaxed rules: if filter does not exists in PM, then return success. */
      return (LIB_API_SET_SUCCESS);
    else
#endif
      return (LIB_API_SET_ERR_ACL_FILTER_DELETED);
    }
}


/* CLI for src and dst only */
CLI (access_list_pacos_ip_any,
     access_list_pacos_ip_any_cmd,
     "access-list pacos WORD (deny|permit) "
     "(ip|gre|igmp|pim|rsvp|ospf|vrrp|ipcomp|any|<0-255>) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) "
     "({label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)",
     FILTER_ZEXT_HELP_COMMON,
     FILTER_ZEXT_HELP_PROTOCOLS,
     FILTER_ZEXT_HELP_ADDR_ONLY,
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum filter_type flt_type;
  int res = CLI_SUCCESS;
  FILTER_PARAM_TYPE cmd_tmpl[] = FILTER_ZEXT_ADDR_ONLY_TMPL;

  if (filter_pacos_ext_decode(cli, AFI_IP, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                              &filter) == CLI_ERROR)
    return CLI_ERROR;

  res = filter_pacos_ext_add(cli->zg, cli->vr->id, AFI_IP,
                             acl_name, flt_type, &filter);
  return _filter_cmd_ret(cli, res);
}

CLI (access_list_pacos_icmp,
     access_list_pacos_icmp_cmd,
     "access-list pacos WORD (deny|permit) (icmp) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) "
     "({icmp-type ICMP-TYPE"
     "|label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)",
     FILTER_ZEXT_HELP_COMMON,
     "ICMP packet",
     FILTER_ZEXT_HELP_ADDR_ONLY,
     "ICMP type",
     "ICMP Value",
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum  filter_type flt_type;
  int res = CLI_SUCCESS;
  FILTER_PARAM_TYPE cmd_tmpl[] =  FILTER_ZEXT_ADDR_ONLY_TMPL;

  if (filter_pacos_ext_decode(cli, AFI_IP, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                              &filter) == CLI_ERROR)
    return CLI_ERROR;

  res = filter_pacos_ext_add(cli->zg, cli->vr->id, AFI_IP,
                             acl_name, flt_type, &filter);
  return _filter_cmd_ret(cli, res);
}


CLI (access_list_pacos_udp,
     access_list_pacos_udp_cmd,
     "access-list pacos WORD (deny|permit) (udp) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "({label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)",
     FILTER_ZEXT_HELP_COMMON,
     "UDP packet",
     FILTER_ZEXT_HELP_ADDR_PORTS,
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum  filter_type flt_type;
  int res = CLI_SUCCESS;
  FILTER_PARAM_TYPE cmd_tmpl[] =  FILTER_ZEXT_ADDR_PORTS_TMPL;

  if (filter_pacos_ext_decode(cli, AFI_IP, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                              &filter) == CLI_ERROR)
    return CLI_ERROR;

  res = filter_pacos_ext_add(cli->zg, cli->vr->id, AFI_IP,
                             acl_name, flt_type, &filter);
  return _filter_cmd_ret(cli, res);
}

CLI (access_list_pacos_tcp,
     access_list_pacos_tcp_cmd,
     "access-list pacos WORD (deny|permit) (tcp) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "({established"
     "|label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)",
     FILTER_ZEXT_HELP_COMMON,
     "TCP packet",
     FILTER_ZEXT_HELP_ADDR_PORTS,
     "Established connections",
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum  filter_type flt_type;
  int res = CLI_SUCCESS;
  FILTER_PARAM_TYPE cmd_tmpl[] =  FILTER_ZEXT_ADDR_PORTS_TMPL;

  if (filter_pacos_ext_decode(cli, AFI_IP, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                              &filter) == CLI_ERROR)
    return CLI_ERROR;

  res = filter_pacos_ext_add(cli->zg, cli->vr->id, AFI_IP,
                             acl_name, flt_type, &filter);
  return _filter_cmd_ret(cli, res);
}

CLI (no_access_list_pacos_ip_any,
     no_access_list_pacos_ip_any_cmd,
     "no access-list pacos WORD ((deny|permit) "
     "(ip|gre|igmp|pim|rsvp|ospf|vrrp|ipcomp|any|<0-255>) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) "
     "({label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)|)",
     CLI_NO_STR,
     FILTER_ZEXT_HELP_COMMON,
     FILTER_ZEXT_HELP_PROTOCOLS,
     FILTER_ZEXT_HELP_ADDR_ONLY,
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum  filter_type flt_type;
  int res = CLI_SUCCESS;
  FILTER_PARAM_TYPE cmd_tmpl[] = FILTER_ZEXT_ADDR_ONLY_TMPL;

  if (filter_pacos_ext_decode(cli, AFI_IP, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                              &filter) == CLI_ERROR)
    return CLI_ERROR;

  res = filter_pacos_ext_del(cli->zg, cli->vr->id, AFI_IP,
                             acl_name, flt_type, &filter);
  return _filter_cmd_ret(cli, res);
}

CLI (no_access_list_pacos_icmp,
     no_access_list_pacos_icmp_cmd,
     "no access-list pacos WORD (deny|permit) (icmp) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) "     
     "({icmp-type ICMP-TYPE"
     "|label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)",
     CLI_NO_STR,
     FILTER_ZEXT_HELP_COMMON,
     "ICMP packet",
     FILTER_ZEXT_HELP_ADDR_ONLY,
     "ICMP type",
     "ICMP Value",
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum  filter_type flt_type;
  int res = CLI_SUCCESS;
  FILTER_PARAM_TYPE cmd_tmpl[] =  FILTER_ZEXT_ADDR_ONLY_TMPL;

  if (filter_pacos_ext_decode(cli, AFI_IP, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                              &filter) == CLI_ERROR)
    return CLI_ERROR;

  res = filter_pacos_ext_del(cli->zg, cli->vr->id, AFI_IP,
                             acl_name, flt_type, &filter);
  return _filter_cmd_ret(cli, res);
}


CLI (no_access_list_pacos_udp,
     no_access_list_pacos_udp_cmd,
     "no access-list pacos WORD (deny|permit) (udp) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "({label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)",
     CLI_NO_STR,
     FILTER_ZEXT_HELP_COMMON,
     "UDP packet",
     FILTER_ZEXT_HELP_ADDR_PORTS,
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum  filter_type flt_type;
  int res = CLI_SUCCESS;

  FILTER_PARAM_TYPE cmd_tmpl[] =  FILTER_ZEXT_ADDR_PORTS_TMPL;

  if (filter_pacos_ext_decode(cli, AFI_IP, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                              &filter) == CLI_ERROR)
    return CLI_ERROR;

  res = filter_pacos_ext_del(cli->zg, cli->vr->id, AFI_IP,
                             acl_name, flt_type, &filter);
  return _filter_cmd_ret(cli, res);
}

CLI (no_access_list_pacos_tcp,
     no_access_list_pacos_tcp_cmd,
     "no access-list pacos WORD (deny|permit) (tcp) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "(A.B.C.D/M|A.B.C.D A.B.C.D|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "({established"
     "|label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)",
     CLI_NO_STR,
     FILTER_ZEXT_HELP_COMMON,
     "TCP packet",
     FILTER_ZEXT_HELP_ADDR_PORTS,
     "Established connections",
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum  filter_type flt_type;
  int res = CLI_SUCCESS;
  FILTER_PARAM_TYPE cmd_tmpl[] =  FILTER_ZEXT_ADDR_PORTS_TMPL;

  if (filter_pacos_ext_decode(cli, AFI_IP, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                              &filter) == CLI_ERROR)
    return CLI_ERROR;

  res = filter_pacos_ext_del(cli->zg, cli->vr->id, AFI_IP,
                             acl_name, flt_type, &filter);
  return _filter_cmd_ret(cli, res);
}

#ifdef HAVE_IPV6
CLI (ipv6_access_list,
     ipv6_access_list_cli,
     "ipv6 access-list WORD (deny|permit) X:X::X:X/M",
     CLI_IPV6_STR,
     CLI_ACCESS_STR,
     CLI_ACCESS6_PACOS_STR,
     CLI_ACCESS_DENY_STR,
     CLI_ACCESS_PERMIT_STR,
     "Prefix to match. e.g. 3ffe:506::/32")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_pacos_set (cli->vr, argv[0], type, AFI_IP6, argv[2]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (ipv6_access_list_exact,
     ipv6_access_list_exact_cli,
     "ipv6 access-list WORD (deny|permit) X:X::X:X/M exact-match",
     CLI_IPV6_STR,
     CLI_ACCESS_STR,
     CLI_ACCESS6_PACOS_STR,
     CLI_ACCESS_DENY_STR,
     CLI_ACCESS_PERMIT_STR,
     "Prefix to match. e.g. 3ffe:506::/32",
     "Exact match of the prefixes")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_pacos_exact_set (cli->vr, argv[0], type, AFI_IP6, argv[2]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (ipv6_access_list_any,
     ipv6_access_list_any_cli,
     "ipv6 access-list WORD (deny|permit) any",
     CLI_IPV6_STR,
     CLI_ACCESS_STR,
     CLI_ACCESS6_PACOS_STR,
     CLI_ACCESS_DENY_STR,
     CLI_ACCESS_PERMIT_STR,
     "Any prefix to match")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_pacos_set (cli->vr, argv[0], type, AFI_IP6, "::/0");

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_ipv6_access_list,
     no_ipv6_access_list_cli,
     "no ipv6 access-list WORD (deny|permit) X:X::X:X/M",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ACCESS_STR,
     CLI_ACCESS6_PACOS_STR,
     CLI_ACCESS_DENY_STR,
     CLI_ACCESS_PERMIT_STR,
     "Prefix to match. e.g. 3ffe:506::/32")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_pacos_unset (cli->vr, argv[0], type, AFI_IP6, argv[2]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_ipv6_access_list_exact,
     no_ipv6_access_list_exact_cli,
     "no ipv6 access-list WORD (deny|permit) X:X::X:X/M exact-match",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ACCESS_STR,
     CLI_ACCESS6_PACOS_STR,
     CLI_ACCESS_DENY_STR,
     CLI_ACCESS_PERMIT_STR,
     "Prefix to match. e.g. 3ffe:506::/32",
     "Exact match of the prefixes")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_pacos_exact_unset (cli->vr, argv[0], type,
                                       AFI_IP6, argv[2]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_ipv6_access_list_any,
     no_ipv6_access_list_any_cli,
     "no ipv6 access-list WORD (deny|permit) any",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ACCESS_STR,
     CLI_ACCESS6_PACOS_STR,
     CLI_ACCESS_DENY_STR,
     CLI_ACCESS_PERMIT_STR,
     "Any prefix to match")
{
  enum filter_type type;
  int ret;

  CLI_GET_FILTER_TYPE (type, argv[1]);

  ret = access_list_pacos_unset (cli->vr, argv[0], type,
                                 AFI_IP6, "::/0");

  return (_filter_cmd_ret (cli, ret));
}


CLI (no_ipv6_access_list_all,
     no_ipv6_access_list_all_cli,
     "no ipv6 access-list WORD",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ACCESS_STR,
     CLI_ACCESS6_PACOS_STR)
{
  int ret;

  ret = access_list_unset_by_name (cli->vr, AFI_IP6, argv[0]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (ipv6_access_list_remark,
     ipv6_access_list_remark_cli,
     "ipv6 access-list WORD remark LINE",
     CLI_IPV6_STR,
     CLI_ACCESS_STR,
     CLI_ACCESS6_PACOS_STR,
     "Access list entry comment",
     "Comment up to 100 characters")
{
  int ret;

  ret = access_list_remark_set (cli->vr, AFI_IP6, argv[0], argv[1]);

  return (_filter_cmd_ret (cli, ret));
}

CLI (no_ipv6_access_list_remark,
     no_ipv6_access_list_remark_cli,
     "no ipv6 access-list WORD remark",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ACCESS_STR,
     CLI_ACCESS6_PACOS_STR,
     "Access list entry comment")
{
  int ret;

  ret = access_list_remark_unset (cli->vr, AFI_IP6, argv[0]);

  return (_filter_cmd_ret (cli, ret));
}

ALI (no_ipv6_access_list_remark,
     no_ipv6_access_list_remark_arg_cli,
     "no ipv6 access-list WORD remark LINE",
     CLI_NO_STR,
     CLI_IPV6_STR,
     CLI_ACCESS_STR,
     CLI_ACCESS6_PACOS_STR,
     "Access list entry comment",
     "Comment up to 100 characters");

/* ipv6 PacOS extended Access-list CLIs*/

CLI (ipv6_access_list_pacos_ip_any,
     ipv6_access_list_pacos_ip_any_cmd,
     "ipv6 access-list pacos WORD (deny|permit) "
     "(ip|gre|igmp|pim|rsvp|ospf|vrrp|ipcomp|any|<0-255>) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) "
     "({label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)",
     CLI_IPV6_STR,
     FILTER_ZEXT_HELP_COMMON,
     FILTER_ZEXT_HELP_PROTOCOLS,
     FILTER_ZEXT_HELP_ADDR_ONLY,
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum filter_type flt_type;
  int res = CLI_SUCCESS;

  FILTER_PARAM_TYPE cmd_tmpl[] =  FILTER_ZEXT_ADDR_ONLY_TMPL;
  if (filter_pacos_ext_decode(cli, AFI_IP6, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                              &filter) == CLI_ERROR)
    return CLI_ERROR;

  res = filter_pacos_ext_add(cli->zg, cli->vr->id, AFI_IP6,
                             acl_name, flt_type, &filter);
  return _filter_cmd_ret(cli, res);
}


CLI (ipv6_access_list_pacos_icmp,
     ipv6_access_list_pacos_icmp_cmd,
     "ipv6 access-list pacos WORD (deny|permit) (icmp) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) "
     "({icmp-type ICMP-TYPE"
     "|label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)",
     CLI_IPV6_STR,
     FILTER_ZEXT_HELP_COMMON,
     "ICMP packet",
     FILTER_ZEXT_HELP_ADDR_ONLY,
     "ICMP type",
     "ICMP Value",
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum  filter_type flt_type;
  int res = CLI_SUCCESS;

  FILTER_PARAM_TYPE cmd_tmpl[] =  FILTER_ZEXT_ADDR_ONLY_TMPL;

  if (filter_pacos_ext_decode(cli, AFI_IP6, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                              &filter) == CLI_ERROR)
    return CLI_ERROR;

  res = filter_pacos_ext_add(cli->zg, cli->vr->id, AFI_IP6,
                             acl_name, flt_type, &filter);
  return _filter_cmd_ret(cli, res);
}
  
  
CLI (ipv6_access_list_pacos_udp,
     ipv6_access_list_pacos_udp_cmd,
     "ipv6 access-list pacos WORD (deny|permit) (udp) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "({label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)",
     CLI_IPV6_STR,
     FILTER_ZEXT_HELP_COMMON,
     "UDP packet",
     FILTER_ZEXT_HELP_ADDR_PORTS,
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum  filter_type flt_type;
  int res = CLI_SUCCESS;
  FILTER_PARAM_TYPE cmd_tmpl[] =  FILTER_ZEXT_ADDR_PORTS_TMPL;

  if (filter_pacos_ext_decode(cli, AFI_IP6, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                              &filter) == CLI_ERROR)
      return CLI_ERROR;
  
  res = filter_pacos_ext_add(cli->zg, cli->vr->id, AFI_IP6,
                             acl_name, flt_type, &filter);
  return _filter_cmd_ret(cli, res);
}
  
CLI (ipv6_access_list_pacos_tcp,
     ipv6_access_list_pacos_tcp_cmd,
     "ipv6 access-list pacos WORD (deny|permit) (tcp) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "({established"
     "|label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)",
     CLI_IPV6_STR,
     FILTER_ZEXT_HELP_COMMON,
     "TCP packet",
     FILTER_ZEXT_HELP_ADDR_PORTS,
     "Established connections",
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum  filter_type flt_type;
  int res = CLI_SUCCESS;

  FILTER_PARAM_TYPE cmd_tmpl[] =  FILTER_ZEXT_ADDR_PORTS_TMPL;

  if (filter_pacos_ext_decode(cli, AFI_IP6, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                              &filter) == CLI_ERROR)
    return CLI_ERROR;

  res = filter_pacos_ext_add(cli->zg, cli->vr->id, AFI_IP6,
                            acl_name, flt_type, &filter);
    return _filter_cmd_ret(cli, res);
}

/* No Ipv6 pacos extended access-list CLIs*/

CLI (no_ipv6_access_list_pacos_ip_any,
     no_ipv6_access_list_pacos_ip_any_cmd,
     "no ipv6 access-list pacos WORD ((deny|permit) "
     "(ip|gre|igmp|pim|rsvp|ospf|vrrp|ipcomp|any|<0-255>) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) "
     "({label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)|)",
     CLI_NO_STR,
     CLI_IPV6_STR,
     FILTER_ZEXT_HELP_COMMON,
     FILTER_ZEXT_HELP_PROTOCOLS,
     FILTER_ZEXT_HELP_ADDR_ONLY,
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum filter_type flt_type;
  int res = CLI_SUCCESS;
  FILTER_PARAM_TYPE cmd_tmpl[] =  FILTER_ZEXT_ADDR_ONLY_TMPL;
  if (filter_pacos_ext_decode(cli, AFI_IP6, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                             &filter) == CLI_ERROR)
    return CLI_ERROR;

  res = filter_pacos_ext_del(cli->zg, cli->vr->id, AFI_IP6,
                             acl_name, flt_type, &filter);
  return _filter_cmd_ret(cli, res);
}


CLI (no_ipv6_access_list_pacos_icmp,
     no_ipv6_access_list_pacos_icmp_cmd,
     "no ipv6 access-list pacos WORD (deny|permit) (icmp) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) "
     "({icmp-type ICMP-TYPE"
     "|label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)",
     CLI_NO_STR,
     CLI_IPV6_STR,
     FILTER_ZEXT_HELP_COMMON,
     "ICMP packet",
     FILTER_ZEXT_HELP_ADDR_ONLY,
     "ICMP type",
     "ICMP Value",
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum  filter_type flt_type;
  int res = CLI_SUCCESS;
  FILTER_PARAM_TYPE cmd_tmpl[] =  FILTER_ZEXT_ADDR_ONLY_TMPL;

  if (filter_pacos_ext_decode(cli, AFI_IP6, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                              &filter) == CLI_ERROR)
    return CLI_ERROR;

  res = filter_pacos_ext_del(cli->zg, cli->vr->id, AFI_IP6,
                             acl_name, flt_type, &filter);
  return _filter_cmd_ret(cli, res);
}
  
  
CLI (no_ipv6_access_list_pacos_udp,
     no_ipv6_access_list_pacos_udp_cmd,
     "no ipv6 access-list pacos WORD (deny|permit) (udp) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "({label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)",
     CLI_NO_STR,
     CLI_IPV6_STR,
     FILTER_ZEXT_HELP_COMMON,
     "UDP packet",
     FILTER_ZEXT_HELP_ADDR_PORTS,
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum  filter_type flt_type;
  int res = CLI_SUCCESS;
  FILTER_PARAM_TYPE cmd_tmpl[] =  FILTER_ZEXT_ADDR_PORTS_TMPL;

  if (filter_pacos_ext_decode(cli, AFI_IP6, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                              &filter) == CLI_ERROR)
    return CLI_ERROR;

  res = filter_pacos_ext_del(cli->zg, cli->vr->id, AFI_IP6,
                             acl_name, flt_type, &filter);
  return _filter_cmd_ret(cli, res);
}
  
CLI (no_ipv6_access_list_pacos_tcp,
     no_ipv6_access_list_pacos_tcp_cmd,
     "no ipv6 access-list pacos WORD (deny|permit) (tcp) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "(X:X::X:X/M|X:X::X:X X:X::X:X|any) ((eq|lt|gt|ne) <0-65535> | range <0-65535> <0-65535>|) "
     "({established"
     "|label <1-65535>|precedence <0-7>|tos (<0-255>| range <0-255> <0-255>)"
     "|pkt-size ((lt|gt) <0-65535>|range <0-65535> <0-65535>)|fragments|log|"
     "interface (in|out) IFNAME}|)",
     CLI_NO_STR,
     CLI_IPV6_STR,
     FILTER_ZEXT_HELP_COMMON,
     "TCP packet",
     FILTER_ZEXT_HELP_ADDR_PORTS,
     "Established connections",
     FILTER_ZEXT_OPTIONS)
{
  struct filter_pacos_ext filter;
  char *acl_name;
  enum  filter_type flt_type;
  int res = CLI_SUCCESS;
  FILTER_PARAM_TYPE cmd_tmpl[] =  FILTER_ZEXT_ADDR_PORTS_TMPL;

  if (filter_pacos_ext_decode(cli, AFI_IP6, argc, argv, cmd_tmpl,
                              &acl_name, &flt_type,
                              &filter) == CLI_ERROR)
    return CLI_ERROR;

  res = filter_pacos_ext_del(cli->zg, cli->vr->id, AFI_IP6,
                             acl_name, flt_type, &filter);
  return _filter_cmd_ret(cli, res);
}


#endif /* HAVE_IPV6 */

void config_write_access_pacos (struct cli *, struct filter_list *);
void config_write_access_common (struct cli *, struct filter_list *);
void config_write_access_pacos_ext (struct cli *, struct filter_list *);

static
const char *get_filter_type_str (struct filter_list *mfilter)
{
  switch (mfilter->common)
    {
    case FILTER_COMMON:
      {
        struct filter_common *filter;

        filter = &mfilter->u.cfilter;
        if (filter->extended)
          return ("Extended");
        else
          return ("Standard");
      }
      break;
    case FILTER_PACOS:
      return ("PacOS");
    case FILTER_PACOS_EXT:
      return ("PacOS extended");
    }
  return ("");
}

/* show access-list command. */
result_t
filter_show (struct cli *cli, char *name, afi_t afi)
{
  struct access_list *access;
  struct access_master *master;
  struct filter_list *mfilter;
  struct filter_common *filter;
  int write = 0;
  char ipa[16];

  master = access_master_get (cli->vr, afi);
  if (master == NULL)
    return (0);

  for (access = master->num.head; access; access = access->next)
    {
      if (name && pal_strcmp (access->name, name) != 0)
        continue;

      write = 1;

      for (mfilter = access->head; mfilter; mfilter = mfilter->next)
        {
          filter = &mfilter->u.cfilter;

          if (write)
            {
#ifdef HAVE_IPV6
              cli_out (cli, "%s IP%s access list %s\n",
                       get_filter_type_str (mfilter),
                       afi == AFI_IP6 ? "v6" : "", access->name);
#else
              cli_out (cli, "%s IP access list %s\n",
                       get_filter_type_str (mfilter),
                       access->name);
#endif /* HAVE_IPV6 */
              write = 0;
            }

          cli_out (cli, "    %s", filter_type_str (mfilter),
                   mfilter->type == FILTER_DENY ? "  " : "");

          if (mfilter->common == FILTER_PACOS)
            config_write_access_pacos (cli, mfilter);
          else if (mfilter->common == FILTER_PACOS_EXT)
            config_write_access_pacos_ext (cli, mfilter);
          else if (filter->extended)
            config_write_access_common (cli, mfilter);
          else
            {
        if (filter->addr_mask.s_addr == 0xffffffff)
        {
                cli_out (cli, " any\n");
        }
        else
        {
                pal_inet_ntop(AF_INET,(void*)&filter->addr,ipa,16);
                cli_out (cli, " %s", ipa);
          if (filter->addr_mask.s_addr != 0)
          {
                  pal_inet_ntop(AF_INET,(void*)&filter->addr_mask,ipa,16);
                  cli_out (cli, ", wildcard bits %s", ipa);
                }
                cli_out (cli, "\n");
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
          filter = &mfilter->u.cfilter;

          if (write)
            {
#ifdef HAVE_IPV6
              cli_out (cli, "%s IP%s access list %s\n",
                       get_filter_type_str (mfilter),
                       afi == AFI_IP6 ? "v6" : "",
                       access->name);
#else
              cli_out (cli, "%s IP access list %s\n",
                       get_filter_type_str (mfilter),
                       access->name);
#endif /* HAVE_IPV6 */
              write = 0;
            }

          cli_out (cli, "    %s%s", filter_type_str (mfilter),
                   mfilter->type == FILTER_DENY ? "  " : "");

          if (mfilter->common == FILTER_PACOS)
            config_write_access_pacos (cli, mfilter);
          else if (mfilter->common == FILTER_PACOS_EXT)
            config_write_access_pacos_ext (cli, mfilter);
          else
            {
        if (filter->addr_mask.s_addr == 0xffffffff)
        {
                cli_out (cli, " any");
        }
        else
        {
                pal_inet_ntop(AF_INET,(void*)&filter->addr,ipa,16);
                cli_out (cli, " %s",ipa);
          if (filter->addr_mask.s_addr != 0)
          {
                  pal_inet_ntop(AF_INET,(void*)&filter->addr_mask,ipa,16);
                  cli_out (cli, ", wildcard bits %s", ipa);
                }
                cli_out (cli, "\n");
              }
            }
        }
    }
  return (CLI_SUCCESS);
}

CLI (show_access_list,
     show_access_list_cli,
     "show access-list",
     CLI_SHOW_STR,
     "List IP access lists")
{
  filter_show (cli, NULL, AFI_IP);
#ifdef HAVE_IPV6
  filter_show (cli, NULL, AFI_IP6);
#endif /* HAVE_IPV6 */
  return (CLI_SUCCESS);
}

CLI (show_ip_access_list,
     show_ip_access_list_cli,
     "show ip access-list",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "List IP access lists")
{
  return (filter_show (cli, NULL, AFI_IP));
}

CLI (show_ip_access_list_name,
     show_ip_access_list_name_cli,
     "show ip access-list (<1-99>|<100-199>|<1300-1999>|<2000-2699>|WORD)",
     CLI_SHOW_STR,
     CLI_IP_STR,
     "List IP access lists",
     STANDARD_EXT_ACCESS_LIST_HELP_STR,
     CLI_ACCESS_PACOS_STR)
{
  return (filter_show (cli, argv[0], AFI_IP));
}

#ifdef HAVE_IPV6
CLI (show_ipv6_access_list,
     show_ipv6_access_list_cli,
     "show ipv6 access-list",
     CLI_SHOW_STR,
     CLI_IPV6_STR,
     "List IPv6 access lists")
{
  return (filter_show (cli, NULL, AFI_IP6));
}

CLI (show_ipv6_access_list_name,
     show_ipv6_access_list_name_cli,
     "show ipv6 access-list WORD",
     CLI_SHOW_STR,
     CLI_IPV6_STR,
     "List IPv6 access lists",
     CLI_ACCESS6_PACOS_STR)
{
  return (filter_show (cli, argv[0], AFI_IP6));
}
#endif /* HAVE_IPV6 */

void
config_encode_access_common (struct filter_list *mfilter, cfg_vect_t *cv)
{
  struct filter_common *filter;
  char ipa[16];

  filter = &mfilter->u.cfilter;

  if (filter->extended)
    {
      cfg_vect_add_cmd (cv, " ip");
      if (filter->addr_mask.s_addr == 0xffffffff)
        {
          cfg_vect_add_cmd (cv, " any");
        }
      else if (filter->addr_mask.s_addr == 0)
        {
          pal_inet_ntop(AF_INET,(void*)&filter->addr,ipa,16);
          cfg_vect_add_cmd (cv, " host %s", ipa);
        }
      else
        {
          pal_inet_ntop(AF_INET,(void*)&filter->addr,ipa,16);
          cfg_vect_add_cmd (cv, " %s", ipa);
          pal_inet_ntop(AF_INET,(void*)&filter->addr_mask,ipa,16);
          cfg_vect_add_cmd (cv, " %s", ipa);
        }

      if (filter->mask_mask.s_addr == 0xffffffff)
        {
          cfg_vect_add_cmd (cv, " any");
        }
      else if (filter->mask_mask.s_addr == 0)
        {
          pal_inet_ntop(AF_INET,(void*)&filter->mask,ipa,16);
          cfg_vect_add_cmd (cv, " host %s", ipa);
        }
      else
        {
          pal_inet_ntop(AF_INET,(void*)&filter->mask,ipa,16);
          cfg_vect_add_cmd (cv, " %s", ipa);
          pal_inet_ntop(AF_INET,(void*)&filter->mask_mask,ipa,16);
          cfg_vect_add_cmd (cv, " %s", ipa);
        }
        cfg_vect_add_cmd (cv, "\n");
    }
  else
    {
      if (filter->addr_mask.s_addr == 0xffffffff)
        cfg_vect_add_cmd (cv, " any\n");
      else
        {
          pal_inet_ntop(AF_INET,(void*)&filter->addr,ipa,16);
          cfg_vect_add_cmd (cv, " %s", ipa);
          if (filter->addr_mask.s_addr != 0)
            {
            pal_inet_ntop(AF_INET,(void*)&filter->addr_mask,ipa,16);
              cfg_vect_add_cmd (cv, " %s", ipa);
            }
          cfg_vect_add_cmd (cv, "\n");
        }
    }
}

void
config_write_access_common (struct cli *cli, struct filter_list *mfilter)
{
  cli->cv = cfg_vect_init(cli->cv);
  config_encode_access_common(mfilter, cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
}

void
config_encode_access_pacos (struct filter_list *mfilter, cfg_vect_t *cv)
{
  struct filter_pacos *filter;
  struct prefix *p;
  char buf[BUFSIZ];

  filter = &mfilter->u.zfilter;
  p = &filter->prefix;

  if (p->prefixlen == 0 && ! filter->exact)
    {
      cfg_vect_add_cmd (cv, " any");
    }
  else
    {
      pal_inet_ntop(p->family,&(p->u.prefix),buf,BUFSIZ);
      cfg_vect_add_cmd (cv, " %s/%d%s",
                        buf,
                        p->prefixlen,
                        filter->exact ? " exact-match" : "");
    }
    cfg_vect_add_cmd (cv, "\n");
}

void
config_write_access_pacos (struct cli *cli, struct filter_list *mfilter)
{
  cli->cv = cfg_vect_init(cli->cv);
  config_encode_access_pacos(mfilter, cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
}

void
config_encode_access_pacos_ext (struct filter_list *mfilter, cfg_vect_t *cv)
{
  struct filter_pacos_ext *filter;
  struct prefix_am p;
  char tmp_string[PREFIX_AM_STR_SIZE+1];
  char prot_name[32];

  pal_mem_set (&p, 0, sizeof (struct prefix_am));

  filter = &mfilter->u.zextfilter;

  if (filter->sprefix.family == AF_INET)
    {
    p.family = AF_INET;
    }
#ifdef HAVE_IPV6
  else if (filter->sprefix.family == AF_INET6)
    {
    p.family = AF_INET6;
    }
#endif /* HAVE_IPV6 */

  cfg_vect_add_cmd (cv, " %s", filter_prot_str (filter->protocol, prot_name));

  if (prefix_am_same (&p, &filter->sprefix))
    {
      cfg_vect_add_cmd (cv, " any");
    }
  else if (filter->sprefix.family == AF_INET)
    {
      prefix2str_am_invert (&filter->sprefix, tmp_string,  PREFIX_AM_STR_SIZE, ' ');
      cfg_vect_add_cmd (cv, " %s", tmp_string);
    }
#ifdef HAVE_IPV6
  else if (filter->sprefix.family == AF_INET6)
    {
      prefix2str_am_invert (&filter->sprefix, tmp_string,  PREFIX_AM_STR_SIZE, ' ');
      cfg_vect_add_cmd (cv, " %s", tmp_string);
    }
#endif /* HAVE_IPV6 */
  if (filter->sport_op != NOOP)
    {
      if (filter->sport_op == RANGE)
        {
          cfg_vect_add_cmd (cv, " %s %d %d", op_str [filter->sport_op],
                            filter->sport_lo, filter->sport);
        }
      else
        {
      cfg_vect_add_cmd (cv, " %s %d", op_str [filter->sport_op], filter->sport);
    }
    }
  if (prefix_am_same (&p, &filter->dprefix))
    {
      cfg_vect_add_cmd (cv, " any");
    }
  else if (filter->sprefix.family == AF_INET)
    {
      prefix2str_am_invert (&filter->dprefix, tmp_string,  PREFIX_AM_STR_SIZE, ' ');
      cfg_vect_add_cmd (cv, " %s", tmp_string);
    }
#ifdef HAVE_IPV6
  else if (filter->sprefix.family == AF_INET6)
    {
      prefix2str_am_invert (&filter->dprefix, tmp_string,  PREFIX_AM_STR_SIZE, ' ');
      cfg_vect_add_cmd (cv, " %s", tmp_string);
    }
#endif /* HAVE_IPV6 */
  if (filter->dport_op != NOOP)
    {
      if (filter->dport_op == RANGE)
        {
          cfg_vect_add_cmd (cv, " %s %d %d", op_str [filter->dport_op],
                            filter->dport_lo, filter->dport);
        }
      else
        {
      cfg_vect_add_cmd (cv, " %s %d", op_str [filter->dport_op], filter->dport);
    }
    }
  if (filter->protocol == ICMP_PROTO)
    {
      if (filter->icmp_type != -1)
        {
          cfg_vect_add_cmd (cv, " icmp-type %d", filter->icmp_type);
        }
    }
  if (filter->protocol == TCP_PROTO)
    {
      if (filter->established == 1)
        {
          cfg_vect_add_cmd (cv, " established");
        }
    }
  if (filter->label >= 1)
    {
      cfg_vect_add_cmd (cv, " label %d", filter->label);
    }
  if (filter->precedence != -1)
    {
      cfg_vect_add_cmd (cv, " precedence %d", filter->precedence);
    }
  if (filter->tos_op == NOOP) /* no operator in the command */
    {
      cfg_vect_add_cmd (cv, " tos %d", filter->tos);
    }
  else if (filter->tos_op == RANGE)
    {
      cfg_vect_add_cmd (cv, " tos range %d %d", filter->tos_lo, filter->tos);
    }
  if (filter->pkt_size_op == LESS_THAN)
    {
      cfg_vect_add_cmd (cv, " pkt-size lt %d", filter->pkt_size);
    }
  else if (filter->pkt_size_op == GREATER_THAN)
    {
      cfg_vect_add_cmd (cv, " pkt-size gt %d", filter->pkt_size);
    }
  else if (filter->pkt_size_op == RANGE)
    {
      cfg_vect_add_cmd (cv, " pkt-size range %d %d", filter->pkt_size_lo,
                        filter->pkt_size);
    }

  if (filter->fragments)
    {
      cfg_vect_add_cmd (cv, " fragments");
    }
  if (filter->log)
    {
      cfg_vect_add_cmd (cv, " log");
    }
  if (filter->ifdir==IFDIR_INPUT || filter->ifdir==IFDIR_OUTPUT)
    {
      cfg_vect_add_cmd (cv, " interface %s %s",
                        filter->ifdir==IFDIR_INPUT ? "in" : "out",
                        filter->ifname);
    }

  cfg_vect_add_cmd (cv, "\n");
}

void
config_write_access_pacos_ext (struct cli *cli, struct filter_list *mfilter)
{
  cli->cv = cfg_vect_init(cli->cv);
  config_encode_access_pacos_ext (mfilter, cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
}

int
config_encode_access (struct apn_vr *vr, afi_t afi, cfg_vect_t *cv)
{
  struct access_list *access;
  struct access_master *master;
  struct filter_list *mfilter;
  int write = 0;

  master = access_master_get (vr, afi);
  if (master == NULL)
    return (0);

  for (access = master->num.head; access; access = access->next)
    {
      if (access->remark)
        {
          cfg_vect_add_cmd (cv, "%saccess-list %s remark %s\n",
                   afi == AFI_IP ? "" : "ipv6 ",
                   access->name, access->remark);
          write++;
        }

      for (mfilter = access->head; mfilter; mfilter = mfilter->next)
        {
          if (mfilter->common == FILTER_COMMON
              || mfilter->common == FILTER_PACOS)
            {
              cfg_vect_add_cmd (cv, "%saccess-list %s %s",
                     afi == AFI_IP ? "" : "ipv6 ",
                     access->name,
                     filter_type_str (mfilter));

            }
          if (mfilter->common == FILTER_COMMON)
            {
              config_encode_access_common (mfilter, cv);
            }
          else if (mfilter->common == FILTER_PACOS)
            {
              config_encode_access_pacos (mfilter, cv);
            }
          else if (mfilter->common == FILTER_PACOS_EXT)
            {
              cfg_vect_add_cmd (cv, "%saccess-list pacos %s %s",
                       afi == AFI_IP ? "" : "ipv6 ",
                       access->name,
                       filter_type_str (mfilter));
              config_encode_access_pacos_ext (mfilter, cv);
            }
          write++;
        }
    }
  for (access = master->str.head; access; access = access->next)
    {
      if (access->remark)
        {
          cfg_vect_add_cmd (cv, "%saccess-list %s remark %s\n",
                   afi == AFI_IP ? "" : "ipv6 ",
                   access->name, access->remark);
          write++;
        }
      for (mfilter = access->head; mfilter; mfilter = mfilter->next)
        {
          if ((mfilter->common == FILTER_COMMON) ||
              (mfilter->common == FILTER_PACOS))
            {
              cfg_vect_add_cmd (cv, "%saccess-list %s %s",
                     afi == AFI_IP ? "" : "ipv6 ",
                     access->name,
                     filter_type_str (mfilter));
            }
          if (mfilter->common == FILTER_COMMON)
            {
              config_encode_access_common (mfilter, cv);
            }
          else if (mfilter->common == FILTER_PACOS)
            {
              config_encode_access_pacos (mfilter, cv);
            }
          else if (mfilter->common == FILTER_PACOS_EXT)
            {
              cfg_vect_add_cmd (cv, "%saccess-list pacos %s %s",
                       afi == AFI_IP ? "" : "ipv6 ",
                       access->name,
                       filter_type_str (mfilter));
              config_encode_access_pacos_ext (mfilter, cv);
            }
          write++;
        }
    }
  return (write);
}

void
config_encode_access_max (struct apn_vr *vr, afi_t afi, cfg_vect_t *cv)
{
  struct access_master *master;

  master = access_master_get (vr, afi);
  if (master == NULL)
    return;

  if (master->max_count != ACCESS_LIST_ENTRY_MAX )
    {
      cfg_vect_add_cmd (cv, "maximum-access-list %lu\n", master->max_count);
      cfg_vect_add_cmd (cv, "!\n");
    }
}

int
config_encode_access_ipv4 (struct apn_vr *vr, cfg_vect_t *cv)
{
  config_encode_access_max (vr, AFI_IP, cv);
  if (config_encode_access (vr, AFI_IP, cv) > 0)
    {
      cfg_vect_add_cmd (cv, "!\n");
    }
  return(0);
}


int
config_write_access_ipv4 (struct cli *cli)
{
#ifdef HAVE_IMI
  if (cli->zg->protocol != APN_PROTO_IMI)
    return (0);
#endif /* HAVE_IMI */

  cli->cv = cfg_vect_init(cli->cv);
  config_encode_access_ipv4(cli->vr, cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
  return(0);
}

void
access_list_finish_ipv4 (struct apn_vr *vr)
{
  struct access_list *access;
  struct access_list *next;
  struct access_master *master;

  master = access_master_get (vr, AFI_IP);
  if (master == NULL)
    return;

  for (access = master->num.head; access; access = next)
    {
      next = access->next;
      access_list_delete (access);
    }
  for (access = master->str.head; access; access = next)
    {
      next = access->next;
      access_list_delete (access);
    }

  pal_assert (master->num.head == NULL);
  pal_assert (master->num.tail == NULL);
  pal_assert (master->str.head == NULL);
  pal_assert (master->str.tail == NULL);
}


/* Install vty related command. */
void
access_list_init_ipv4 (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  cli_install_config (ctree, ACCESS_MODE, config_write_access_ipv4);

#ifndef HAVE_IMI
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &show_access_list_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &show_ip_access_list_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &show_ip_access_list_name_cli);
#endif /* !HAVE_IMI */

  /* PacOS access-list */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_exact_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_any_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &maximum_access_list_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_maximum_access_list_cli);
  cli_set_imi_cmd (&no_maximum_access_list_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_exact_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_any_cli);

  /* Standard access-list */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_standard_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_standard_nomask_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_standard_host_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_standard_any_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_standard_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_standard_nomask_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_standard_host_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_standard_any_cli);

  /* Extended access-list */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_any_mask_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_mask_any_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_any_any_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_host_mask_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_mask_host_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_host_host_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_any_host_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_host_any_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_any_mask_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_mask_any_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_any_any_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_host_mask_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_mask_host_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_host_host_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_any_host_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_host_any_cli);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_remark_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_all_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_remark_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_remark_arg_cli);

  /* Ipv4 Extended pacos access-list CLI */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_pacos_ip_any_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_pacos_icmp_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_pacos_udp_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &access_list_pacos_tcp_cmd);

  /* No CLIs */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_pacos_ip_any_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_pacos_icmp_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_pacos_udp_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_access_list_pacos_tcp_cmd);
}

#ifdef HAVE_IPV6

int
config_encode_access_ipv6 (struct apn_vr *vr, cfg_vect_t *cv)
{
  if (config_encode_access (vr, AFI_IP6, cv) > 0)
    {
      cfg_vect_add_cmd (cv, "!\n");
    }
  return(0);
}

int
config_write_access_ipv6 (struct cli *cli)
{
#ifdef HAVE_IMI
  if (cli->zg->protocol != APN_PROTO_IMI)
    return(0);
#endif /* HAVE_IMI */

  cli->cv = cfg_vect_init(cli->cv);
  config_encode_access_ipv6(cli->vr, cli->cv);
  cfg_vect_out(cli->cv, (cfg_vect_out_fun_t)cli->out_func, cli->out_val);
  return(0);
}


void
access_list_finish_ipv6 (struct apn_vr *vr)
{
  struct access_list *access;
  struct access_list *next;
  struct access_master *master;

  master = access_master_get (vr, AFI_IP6);
  if (master == NULL)
    return;

  for (access = master->num.head; access; access = next)
    {
      next = access->next;
      access_list_delete (access);
    }
  for (access = master->str.head; access; access = next)
    {
      next = access->next;
      access_list_delete (access);
    }

  pal_assert (master->num.head == NULL);
  pal_assert (master->num.tail == NULL);
  pal_assert (master->str.head == NULL);
  pal_assert (master->str.tail == NULL);
}

void
access_list_init_ipv6 (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  cli_install_config (ctree, ACCESS_IPV6_MODE, config_write_access_ipv6);

#ifndef HAVE_IMI
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &show_ipv6_access_list_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &show_ipv6_access_list_name_cli);
#endif /* !HAVE_IMI */

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_exact_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_any_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_exact_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_any_cli);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_all_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_remark_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_remark_cli);
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_remark_arg_cli);

  /* PacOS extended Access-List */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_pacos_ip_any_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_pacos_icmp_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_pacos_udp_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_pacos_tcp_cmd);


  /* No CLIs */
  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_pacos_ip_any_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_pacos_icmp_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_pacos_udp_cmd);

  cli_install_gen (ctree, CONFIG_MODE, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_pacos_tcp_cmd);

}
#endif /* HAVE_IPV6 */

void
access_list_init (struct lib_globals *zg)
{
  access_list_init_ipv4 (zg);
#ifdef HAVE_IPV6
  access_list_init_ipv6 (zg);
#endif /* HAVE_IPV6 */
}

void
access_list_finish (struct apn_vr *vr)
{
  access_list_finish_ipv4 (vr);
#ifdef HAVE_IPV6
  access_list_finish_ipv6 (vr);
#endif /* HAVE_IPV6 */
}


#ifdef HAVE_IMI
void
imi_access_list_init (struct lib_globals *zg)
{
  struct cli_tree *ctree = zg->ctree;

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &show_access_list_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &show_ip_access_list_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &show_ip_access_list_name_cli);

  /* PacOS access-list */

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_cli);
  cli_set_imi_cmd (&access_list_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_exact_cli);
  cli_set_imi_cmd (&access_list_exact_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_any_cli);
  cli_set_imi_cmd (&access_list_any_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_cli);
  cli_set_imi_cmd (&no_access_list_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_exact_cli);
  cli_set_imi_cmd (&no_access_list_exact_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_any_cli);
  cli_set_imi_cmd (&no_access_list_any_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &maximum_access_list_cli);
  cli_set_imi_cmd (&maximum_access_list_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_maximum_access_list_cli);
  cli_set_imi_cmd (&no_maximum_access_list_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  /* Standard access-list */
  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_standard_cli);
  cli_set_imi_cmd (&access_list_standard_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_standard_nomask_cli);
  cli_set_imi_cmd (&access_list_standard_nomask_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_standard_host_cli);
  cli_set_imi_cmd (&access_list_standard_host_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_standard_any_cli);
  cli_set_imi_cmd (&access_list_standard_any_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_standard_cli);
  cli_set_imi_cmd (&no_access_list_standard_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_standard_nomask_cli);
  cli_set_imi_cmd (&access_list_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_standard_host_cli);
  cli_set_imi_cmd (&no_access_list_standard_nomask_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_standard_any_cli);
  cli_set_imi_cmd (&no_access_list_standard_any_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  /* Extended access-list */
  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_cli);
  cli_set_imi_cmd (&access_list_extended_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_any_mask_cli);
  cli_set_imi_cmd (&access_list_extended_any_mask_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_mask_any_cli);
  cli_set_imi_cmd (&access_list_extended_mask_any_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_any_any_cli);
  cli_set_imi_cmd (&access_list_extended_any_any_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_host_mask_cli);
  cli_set_imi_cmd (&access_list_extended_host_mask_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_mask_host_cli);
  cli_set_imi_cmd (&access_list_extended_mask_host_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_host_host_cli);
  cli_set_imi_cmd (&access_list_extended_host_host_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_any_host_cli);
  cli_set_imi_cmd (&access_list_extended_any_host_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_extended_host_any_cli);
  cli_set_imi_cmd (&access_list_extended_host_any_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_any_mask_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_mask_any_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_any_any_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_host_mask_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_mask_host_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_host_host_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_any_host_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_extended_host_any_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_remark_cli);
  cli_set_imi_cmd (&access_list_remark_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_all_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_remark_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_remark_arg_cli);

  /* Extended pacos access-list CLI */

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_pacos_ip_any_cmd);
  cli_set_imi_cmd (&access_list_pacos_ip_any_cmd, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_pacos_icmp_cmd);
  cli_set_imi_cmd (&access_list_pacos_icmp_cmd, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);


  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_pacos_udp_cmd);
  cli_set_imi_cmd (&access_list_pacos_udp_cmd, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &access_list_pacos_tcp_cmd);
  cli_set_imi_cmd (&access_list_pacos_tcp_cmd, CONFIG_MODE, CFG_DTYP_ACCESS_IPV4);

 /* No CLIs */
  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_pacos_ip_any_cmd);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_pacos_icmp_cmd);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_pacos_udp_cmd);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_access_list_pacos_tcp_cmd);

#ifdef HAVE_IPV6
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &show_ipv6_access_list_cli);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_VR_MAX, 0,
                   &show_ipv6_access_list_name_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_cli);
  cli_set_imi_cmd (&ipv6_access_list_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV6);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_exact_cli);
  cli_set_imi_cmd (&ipv6_access_list_exact_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV6);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_any_cli);
  cli_set_imi_cmd (&ipv6_access_list_any_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV6);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_exact_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_any_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_all_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_remark_cli);
  cli_set_imi_cmd (&ipv6_access_list_remark_cli, CONFIG_MODE, CFG_DTYP_ACCESS_IPV6);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_remark_cli);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_remark_arg_cli);

  /* Ipv6 PacOS extended Access-list CLIs */
  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_pacos_ip_any_cmd);
  cli_set_imi_cmd (&ipv6_access_list_pacos_ip_any_cmd, CONFIG_MODE, CFG_DTYP_ACCESS_IPV6);


  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_pacos_icmp_cmd);
  cli_set_imi_cmd (&ipv6_access_list_pacos_icmp_cmd, CONFIG_MODE, CFG_DTYP_ACCESS_IPV6);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_pacos_udp_cmd);
  cli_set_imi_cmd (&ipv6_access_list_pacos_udp_cmd, CONFIG_MODE, CFG_DTYP_ACCESS_IPV6);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &ipv6_access_list_pacos_tcp_cmd);
  cli_set_imi_cmd (&ipv6_access_list_pacos_tcp_cmd, CONFIG_MODE, CFG_DTYP_ACCESS_IPV6);


  /* No CLIs */
  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_pacos_ip_any_cmd);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_pacos_icmp_cmd);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_pacos_udp_cmd);

  cli_install_imi (ctree, CONFIG_MODE, PM_ACCESS, PRIVILEGE_NORMAL, 0,
                   &no_ipv6_access_list_pacos_tcp_cmd);
#endif /* HAVE_IPV6 */
}
#endif /* HAVE_IMI */

