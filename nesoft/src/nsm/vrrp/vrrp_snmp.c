/* Copyright (C) 2003  All Rights Reserved. */

/* Copyright (C) 2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_SNMP

#include <asn1.h>

#include "if.h"
#include "prefix.h"
#include "ls_prefix.h"
#include "snmp.h"
#include "log.h"

#include "nsm/nsmd.h"
#include "vrrp.h"
#include "vrrp_api.h"
#include "vrrp_snmp.h"


/* Declare static local variables for convenience. */
static s_int32_t vrrp_int_val;
static u_int32_t vrrp_uint_val;
static vrrp_inx_addr_t vrrp_in_addr_val;
static struct vmac_addr vrrp_mac_addr_val;

/* VRRP MIB instances. */
oid vrrp_oid[] = { VRRPMIB};
oid vrrp_trap_oid[] = { VRRPMIB, 0};

/* Hook functions */
static u_char *vrrpNotificationCntl();
static u_char *vrrpOperationsTable ();
static u_char *vrrpAssociatedIpAddrTable ();
static u_char *vrrpStatisticsTable();
static u_char *vrrpRouterChecksumErrors();
static u_char *vrrpRouterVersionErrors();
static u_char *vrrpRouterVrIdErrors();
static u_char *vrrpNewMasterReason ();

struct variable vrrp_variables[] =
{
  {VRRPNOTIFICATIONCNTL,             INTEGER,        RWRITE,   vrrpNotificationCntl,      2, {1,2}},
   /* vrrpOper  - vrrp operation entry variable information. */
  {VRRPOPERATIONSINETADDRTYPE,       INTEGER,        NOACCESS, vrrpOperationsTable,       4, {1,7,1,1}},
  {VRRPOPERATIONSVRID,               INTEGER,        NOACCESS, vrrpOperationsTable,       4, {1,7,1,2}},
  {VRRPOPERATIONSVIRTUALMACADDR,     MACADDRESS,     RONLY,    vrrpOperationsTable,       4, {1,7,1,3}},
  {VRRPOPERATIONSSTATE,              INTEGER,        RONLY,    vrrpOperationsTable,       4, {1,7,1,4}},
  {VRRPOPERATIONSPRIORITY,           GAUGE,          RCREATE,  vrrpOperationsTable,       4, {1,7,1,5}},
  {VRRPOPERATIONSADDRCOUNT,          INTEGER,        RONLY,    vrrpOperationsTable,       4, {1,7,1,6}},
  {VRRPOPERATIONSMASTERIPADDR,       INETADDRESS,    RONLY,    vrrpOperationsTable,       4, {1,7,1,7}},
  {VRRPOPERATIONSPRIMARYIPADDR,      INETADDRESS,    RCREATE,  vrrpOperationsTable,       4, {1,7,1,8}},
  {VRRPOPERATIONSADVINTERVAL,        INTEGER,        RCREATE,  vrrpOperationsTable,       4, {1,7,1,9}},
  {VRRPOPERATIONSPREEMPTMODE,        INTEGER,        RCREATE,  vrrpOperationsTable,       4, {1,7,1,10}},
  {VRRPOPERATIONSACCEPTMODE,         INTEGER,        RCREATE,  vrrpOperationsTable,       4, {1,7,1,11}},
  {VRRPOPERATIONSUPTIME,             TIMETICKS,      RONLY,    vrrpOperationsTable,       4, {1,7,1,12}},
  {VRRPOPERATIONSSTORAGETYPE,        INTEGER,        RCREATE,  vrrpOperationsTable,       4, {1,7,1,13}},
  {VRRPOPERATIONSROWSTATUS,          INTEGER,        RCREATE,  vrrpOperationsTable,       4, {1,7,1,14}},

  {VRRPASSOCIATEDIPADDR,             INETADDRESS,    NOACCESS, vrrpAssociatedIpAddrTable, 4, {1,8,1,1}},
  {VRRPASSOCIATEDSTORAGETYPE,        INTEGER,        RCREATE,  vrrpAssociatedIpAddrTable, 4, {1,8,1,2}},
  {VRRPASSOCIATEDIPADDRROWSTATUS,    INTEGER,        RCREATE,  vrrpAssociatedIpAddrTable, 4, {1,8,1,3}},

  {VRRPNEWMASTERREASON,              INTEGER,        RONLY,    vrrpNewMasterReason,       2, {1,9}},

  {VRRPROUTERCHECKSUMERRORS,         COUNTER,        RONLY,  vrrpRouterChecksumErrors,    2, {2,1}},
  {VRRPROUTERVERSIONERRORS,          COUNTER,        RONLY,  vrrpRouterVersionErrors,     2, {2,2}},
  {VRRPROUTERVRIDERRORS,             COUNTER,        RONLY,  vrrpRouterVrIdErrors,        2, {2,3}},

  {VRRPSTATISTICSMASTERTRANSITIONS,  COUNTER,        RONLY,  vrrpStatisticsTable,         4, {2,5,1,1}},
  {VRRPSTATISTICSRCVDADVERTISEMENTS, COUNTER,        RONLY,  vrrpStatisticsTable,         4, {2,5,1,2}},
  {VRRPSTATISTICSADVINTERVALERRORS,  COUNTER,        RONLY,  vrrpStatisticsTable,         4, {2,5,1,3}},
  {VRRPSTATISTICSIPTTLERRORS,        COUNTER,        RONLY,  vrrpStatisticsTable,         4, {2,5,1,4}},
  {VRRPSTATISTICSRCVDPRIZEROPACKETS, COUNTER,        RONLY,  vrrpStatisticsTable,         4, {2,5,1,5}},
  {VRRPSTATISTICSSENTPRIZEROPACKETS, COUNTER,        RONLY,  vrrpStatisticsTable,         4, {2,5,1,6}},
  {VRRPSTATISTICSRCVDINVALIDTYPEPKTS,COUNTER,        RONLY,  vrrpStatisticsTable,         4, {2,5,1,7}},
  {VRRPSTATISTICSADDRESSLISTERRORS,  COUNTER,        RONLY,  vrrpStatisticsTable,         4, {2,5,1,8}},
  {VRRPSTATISTICSPACKETLENGTHERRORS, COUNTER,        RONLY,  vrrpStatisticsTable,         4, {2,5,1,9}},
  {VRRPSTATISTICSRCVDINVALIDAUTHENTICATIONS,COUNTER, RONLY,  vrrpStatisticsTable,         4, {2,5,1,10}},
  {VRRPSTATISTICSDISCONTINUITYTIME,  TIMETICKS,      RONLY,  vrrpStatisticsTable,         4, {2,5,1,11}},
  {VRRPSTATISTICSREFRESHRATE,        GAUGE,          RONLY,  vrrpStatisticsTable,         4, {2,5,1,12}},
};

struct vrrp_oper_table_index {
  u_int8_t aftype; /* stored here for convenience: AF_INET or AF_INET6 */
  unsigned int len;
  /* Please do not alter alignment of the following members. */
  u_int8_t  adtype; /* SNMP repr of address type */
  u_int32_t ifindex;
  u_int8_t  vrid;
};

struct vrrp_asso_ipaddr_table_index {
  u_int8_t aftype; /* stored here for convenience: AF_INET or AF_INET6 */
  unsigned int len;
  /* Please do not alter alignment of the following members. */
  u_int8_t  adtype;
  u_int32_t ifindex;
  u_int8_t  vrid;
  vrrp_inx_addr_t ipaddr; 
};
    
#define VRRP_SNMP_INDEX_GET_ERROR    0
#define VRRP_SNMP_INDEX_GET_SUCCESS  1

/* VRRP SMUX trap function */
void
vrrp_snmp_smux_trap (oid *trap_oid, size_t trap_oid_len,
                     oid spec_trap_val, u_int32_t uptime,
                     struct snmp_trap_object *obj, size_t obj_len)
{
  snmp_trap2 (nzg, trap_oid, trap_oid_len, spec_trap_val,
              vrrp_oid, sizeof (vrrp_oid) / sizeof (oid),
              (struct trap_object2 *) obj, obj_len, uptime);
}

/*--------------------------------------------------------
 * InetAddrType.VrID.ifIndex
 *--------------------------------------------------------
 */
void
vrrp_oid2_index (oid oid[], s_int32_t len, struct vrrp_oper_table_index *index)
{
  if (len <= 0)
    return;
  if (len>0) {
    oid_copy (&index->adtype, oid+0, 1);
    index->aftype = VRRP_AT2AF(index->adtype);
  }
  if (len>1) {
    oid_copy (&index->vrid,     oid+1, 1);
  }
  if (len>2) {
    oid_copy (&index->ifindex,  oid+2, 1);
  }
}

void
vrrp_index2_oid (struct vrrp_oper_table_index *index, oid oid[], s_int32_t len)
{
  if (len > 0) {
    index->adtype = VRRP_AF2AT(index->aftype);
    oid_copy (oid+0, &index->adtype, 1);    
  }
  if (len > 1) {
    oid_copy (oid+1, &index->vrid,     1); 
  }
  if (len > 2) {
    oid_copy (oid+2, &index->ifindex,  1);  
  }
}


void
vrrp_oid2_asso_index (oid oid[], s_int32_t len, struct vrrp_asso_ipaddr_table_index *index)
{
  if (len <= 0)
    return;

  if (len>0) {
    oid_copy (&index->adtype, oid, 1);
    index->aftype = VRRP_AT2AF(index->adtype);
  }
  if (len>1) {
    oid_copy (&index->vrid,     oid+1, 1);
  }
  if (len>2) {
    oid_copy (&index->ifindex,  oid+2, 1);
  }
  if (index->adtype == SNMP_AG_ADDR_TYPE_ipv4) {
    if (len>6) {
      oid2in_addr (oid+3, IN_ADDR_SIZE, &index->ipaddr.uia.in4_addr);
    }
  }
#ifdef HAVE_IPV6
  else if (index->adtype == SNMP_AG_ADDR_TYPE_ipv6) {
    if (len>18) {
      oid2in6_addr (oid+3, IN6_ADDR_SIZE, &index->ipaddr.uia.in6_addr);   
    }
  }
#endif
}

void
vrrp_asso_index2_oid (struct vrrp_asso_ipaddr_table_index *index, oid oid[], s_int32_t len)
{
  if (len>0) {
    oid_copy (oid, &index->adtype,   1);    
  }
  if (len>0) {
    oid_copy (oid+1, &index->vrid,     1); 
  }
  if (len>0) {
    oid_copy (oid+2, &index->ifindex,  1);  
  }
  if (index->adtype == SNMP_AG_ADDR_TYPE_ipv4) {
    if (len>0) {
      oid_copy_addr (oid+3, &index->ipaddr.uia.in4_addr, IN_ADDR_SIZE);
    }
  }
#ifdef HAVE_IPV6
  else if (index->adtype == SNMP_AG_ADDR_TYPE_ipv6) {
    if (len>0) {
      oid_copy_in6_addr (oid+3, &index->ipaddr.uia.in6_addr, IN6_ADDR_SIZE);    
    }
  }
#endif
}

/* Utility function to get vrrpOperTable index.  */
int 
vrrp_snmp_index_get (struct variable *v, oid *name, size_t *length, 
                    struct vrrp_oper_table_index *index, int exact)
{
  index->len = *length - v->namelen;

  if (index->len == 0)
    return (VRRP_SNMP_INDEX_GET_SUCCESS);

  if (exact)
    if (index->len != VRRP_OPER_TABLE_INDEX_LEN)
      return (VRRP_SNMP_INDEX_GET_ERROR);

  if (index->len > VRRP_OPER_TABLE_INDEX_LEN)
    return (VRRP_SNMP_INDEX_GET_ERROR);

  vrrp_oid2_index(name + v->namelen, index->len, index);

#ifndef HAVE_IPV6
  if (index->adtype==SNMP_AG_ADDR_TYPE_ipv6) {
    return VRRP_SNMP_INDEX_GET_ERROR;
  }
#endif
  return (VRRP_SNMP_INDEX_GET_SUCCESS);
}

/* Utility function to set vrrpOperTable index.  */
void 
vrrp_snmp_index_set (struct variable *v, oid *name, size_t *length, 
                    struct vrrp_oper_table_index *index)
{
  index->len = VRRP_OPER_TABLE_INDEX_LEN;
  vrrp_index2_oid (index, name + v->namelen, index->len);
  *length = index->len + v->namelen;
}

/* Utility function to get vrrpAssoIpAddrTable index.  */
int 
vrrp_snmp_asso_index_get (struct variable *v, oid *name, size_t *length, 
                          struct vrrp_asso_ipaddr_table_index *index, int exact)
{
  index->len = *length - v->namelen;

  if (index->len == 0)
    return (VRRP_SNMP_INDEX_GET_SUCCESS);

  vrrp_oid2_asso_index(name + v->namelen, index->len, index);

#ifndef HAVE_IPV6
  if (index->adtype==SNMP_AG_ADDR_TYPE_ipv6) {
    return VRRP_SNMP_INDEX_GET_ERROR;
  }
#endif
  if (exact) {
    if (index->len>0) {
      if (index->adtype==SNMP_AG_ADDR_TYPE_ipv4) {
        if (index->len != VRRP_ASSO_IPV4_TABLE_INDEX_LEN)
          return (VRRP_SNMP_INDEX_GET_ERROR);
      }
#ifdef HAVE_IPV6
      else if (index->adtype==SNMP_AG_ADDR_TYPE_ipv6) {
        if (index->len != VRRP_ASSO_IPV6_TABLE_INDEX_LEN)
          return (VRRP_SNMP_INDEX_GET_ERROR);
      }
#endif
    }
  }
  else {
    if (index->adtype==SNMP_AG_ADDR_TYPE_ipv4) {
      if (index->len > VRRP_ASSO_IPV4_TABLE_INDEX_LEN)
        return (VRRP_SNMP_INDEX_GET_ERROR);
    }
#ifdef HAVE_IPV6
    else if (index->adtype==SNMP_AG_ADDR_TYPE_ipv6) {
      if (index->len != VRRP_ASSO_IPV6_TABLE_INDEX_LEN)
        return (VRRP_SNMP_INDEX_GET_ERROR);
    }
#endif
  }
  return (VRRP_SNMP_INDEX_GET_SUCCESS);        
} 

/* Utility function to set vrrpAssoIpAddrTable index.  */
void 
vrrp_snmp_asso_index_set (struct variable *v, oid *name, size_t *length, 
                          struct vrrp_asso_ipaddr_table_index *index)
{
  index->adtype = VRRP_AF2AT(index->aftype);
  if (index->adtype == SNMP_AG_ADDR_TYPE_ipv4) {
    index->len = VRRP_ASSO_IPV4_TABLE_INDEX_LEN;
  }
  else if (index->adtype == SNMP_AG_ADDR_TYPE_ipv6) {
    index->len = VRRP_ASSO_IPV6_TABLE_INDEX_LEN;    
  }
  vrrp_asso_index2_oid (index, name + v->namelen, index->len);
  *length = index->len + v->namelen;
}

/*--------------------------------------------------------
 * vrrpNodeVersion
 *--------------------------------------------------------
 */
u_char *
vrrpNewMasterReason (struct variable *v, oid *name, size_t *length, int exact,
                     size_t *var_len, WriteMethod **write_method, u_int32_t apn_vrid)
{
/*   static int reason=0; */

  if (snmp_header_generic(v, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return (NULL);
/* This polling of vrrpNewMasterReason does not maek sense.
   Confirmed by MIB editor.

  if (VRRP_SNMP_GET (new_master_reason, &reason)) {
    return (u_char *) &reason;
  }
  else
*/
  return (0);
}

/*--------------------------------------------------------
 * vrrpNotificationCntl
 *--------------------------------------------------------
 */
int
write_vrrpNotificationCntl (int action, u_char *var_val,
                            u_char var_val_type, size_t var_val_len,
                            u_char *statP, oid *name, size_t length,
                            struct variable *v, u_int32_t apn_vrid)
{
  long intval = 0;

  if (var_val_type == ASN_INTEGER) {
      if (var_val_len != sizeof (long)) 
      return (SNMP_ERR_WRONGLENGTH);

      pal_mem_cpy(&intval,var_val,4);   
    }
  else
    return (SNMP_ERR_GENERR);
  
  if (intval == VRRP_NOTIFICATIONCNTL_ENA
      || intval == VRRP_NOTIFICATIONCNTL_DIS) {
    if (vrrp_set_notify (apn_vrid, intval) != VRRP_API_SET_SUCCESS)
      return (SNMP_ERR_GENERR);
  }
  else
    return (SNMP_ERR_BADVALUE);   
  
  return (SNMP_ERR_NOERROR);
}   

static u_char *
vrrpNotificationCntl(struct variable *v, oid *name, size_t *length, int exact,
                     size_t *var_len, WriteMethod **write_method, 
                     u_int32_t apn_vrid)
{
  static int notify;

  if (snmp_header_generic(v, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return (NULL);

  *write_method = write_vrrpNotificationCntl;
  
  if (VRRP_SNMP_GET (notify, &notify))
    return(u_char *) &notify;
  else
    return (0);
}

/*--------------------------------------------------------
 * vrrpRouterChecksumErrors
 *--------------------------------------------------------
 */
static u_char *
vrrpRouterChecksumErrors(struct variable *v, oid *name, 
                         size_t *length, int exact, 
                         size_t *var_len, WriteMethod **write_method,
                         u_int32_t apn_vrid)
{
  static int ckerrs;

  if (snmp_header_generic(v, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return (NULL);

  if (VRRP_SNMP_GET (checksum_errors, &ckerrs)) {
    return(u_char *) &ckerrs;
  }
  else
    return (0);
}

/*--------------------------------------------------------
 * vrrpRouterVersionErrors
 *--------------------------------------------------------
 */
static u_char *
vrrpRouterVersionErrors (struct variable *v, oid *name, 
                         size_t *length, int exact,
                         size_t *var_len, WriteMethod **write_method,
                         u_int32_t apn_vrid)
{
  static int verrs;

  if (snmp_header_generic(v, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return (NULL);

  if (VRRP_SNMP_GET (version_errors, &verrs)) {
    return(u_char *) &verrs;
  }
  else
    return (0);
}

/*--------------------------------------------------------
 * vrrpRouterVrIdErrors
 *--------------------------------------------------------
 */
static u_char *
vrrpRouterVrIdErrors (struct variable *v, oid *name, size_t *length, int exact,
                      size_t *var_len, WriteMethod **write_method, u_int32_t apn_vrid)
{
  static int iderrs;

  if (snmp_header_generic(v, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return (NULL);

  if (VRRP_SNMP_GET (vrid_errors, &iderrs)) {
    return(u_char *) &iderrs;
  }
  else
    return (0);
}

/*--------------------------------------------------------
 * vrrpOperationsTable
 * 
 * write/get/getnext
 *--------------------------------------------------------
 */
int
write_vrrpOperationsTable (int action, u_char *var_val,
                     u_char var_val_type, size_t var_val_len,
                     u_char *statP, oid *name, size_t length,
                           struct variable *v, u_int32_t apn_vrid)
{
  long intval;
  vrrp_inx_addr_t ipaddr;
  int  ret; 
  struct vrrp_oper_table_index index;

  pal_mem_set (&index, 0, sizeof( struct vrrp_oper_table_index));
  ret = vrrp_snmp_index_get (v, name, &length, &index, 1);
  if (ret < 0) 
    return (SNMP_ERR_NOSUCHNAME);

  if (index.adtype != SNMP_AG_ADDR_TYPE_ipv4 && index.adtype != SNMP_AG_ADDR_TYPE_ipv6) {
    return (SNMP_ERR_NOSUCHNAME);    
  }
  if (var_val_type == ASN_INTEGER) {
    pal_mem_cpy(&intval,var_val,4);
  }
  else if (var_val_type == ASN_GAUGE) {
    pal_mem_cpy(&intval,var_val,4);
  }
  else if (v->magic==VRRPOPERATIONSPRIMARYIPADDR)
  {
    if (index.adtype == SNMP_AG_ADDR_TYPE_ipv4) {
      if (var_val_len != IN_ADDR_SIZE) {
        return (SNMP_ERR_WRONGLENGTH);
      }
      pal_mem_cpy (ipaddr.uia.in_addr, var_val, IN_ADDR_SIZE);     
    }
#ifdef HAVE_IPV6
    else if (index.adtype == SNMP_AG_ADDR_TYPE_ipv6) {
      if (var_val_len != IN6_ADDR_SIZE) {
        return (SNMP_ERR_WRONGLENGTH);
      }
      pal_mem_cpy (ipaddr.uia.in_addr, var_val, IN6_ADDR_SIZE);
    }
#endif  
  }
  else {
    return (SNMP_ERR_WRONGTYPE);
  }

  switch (v->magic) {
  case VRRPOPERATIONSPRIORITY:
    if (intval <= VRRP_MIN_OPER_PRIORITY  
        || intval >= VRRP_MAX_OPER_PRIORITY)
      return (SNMP_ERR_BADVALUE);

    if (vrrp_set_oper_priority (apn_vrid, index.aftype, index.vrid, index.ifindex, 
                                intval)
          != VRRP_API_SET_SUCCESS)
      return (SNMP_ERR_GENERR);
      break;
    
  case VRRPOPERATIONSPRIMARYIPADDR:
    if (vrrp_set_oper_primary_ipaddr (apn_vrid, index.aftype, index.vrid, index.ifindex, 
                                      ipaddr.uia.in_addr)
          != VRRP_API_SET_SUCCESS)
      return (SNMP_ERR_GENERR);
      break;
    
  case VRRPOPERATIONSADVINTERVAL:
    if (intval < VRRP_MIN_ADVERTISEMENT_INTERVAL 
          || intval > VRRP_MAX_ADVERTISEMENT_INTERVAL)
      return (SNMP_ERR_BADVALUE);

    if (vrrp_set_oper_adv_interval (apn_vrid, index.aftype, index.vrid, index.ifindex, 
                                    intval)
          != VRRP_API_SET_SUCCESS)
      return (SNMP_ERR_GENERR);
      break;

  case VRRPOPERATIONSPREEMPTMODE:
    if ((intval != SNMP_TRUE) && (intval != SNMP_FALSE)) 
        return (SNMP_ERR_BADVALUE);
    if (vrrp_set_oper_preempt_mode (apn_vrid, index.aftype, index.vrid, index.ifindex, 
                                    VRRP_SNMP2BOOL(intval))
          != VRRP_API_SET_SUCCESS)
      return (SNMP_ERR_GENERR);
      break;

  case VRRPOPERATIONSACCEPTMODE:
    if ((intval != SNMP_TRUE) && (intval != SNMP_FALSE))
      return (SNMP_ERR_BADVALUE);
    if (vrrp_set_oper_accept_mode (apn_vrid, index.aftype, index.vrid, index.ifindex, 
                                   VRRP_SNMP2BOOL(intval))
          != VRRP_API_SET_SUCCESS)
      return (SNMP_ERR_GENERR);
      break;

  case VRRPOPERATIONSSTORAGETYPE:
    if ((intval < SNMP_AG_STOR_other) && (intval > SNMP_AG_STOR_readOnly))
      return (SNMP_ERR_BADVALUE);
    if (vrrp_set_oper_storage_type (apn_vrid, index.aftype, index.vrid, index.ifindex, 
                                    intval)
          != VRRP_API_SET_SUCCESS)
      return (SNMP_ERR_GENERR);
    break;

  case VRRPOPERATIONSROWSTATUS:
    if (vrrp_set_oper_rowstatus (apn_vrid, index.aftype, index.vrid, index.ifindex,
                                 intval)
        != VRRP_API_SET_SUCCESS)
      return (SNMP_ERR_GENERR);
      break;

    default:
      break;
    }
  return (SNMP_ERR_NOERROR);
}

u_char *
vrrpOperationsTable (struct variable *v, oid *name, size_t *length,
               int exact, size_t *var_len, WriteMethod **write_method,
                     u_int32_t apn_vrid)
{
  int ret;
  struct vrrp_oper_table_index index;

  *write_method = NULL;

  pal_mem_set (&index, 0, sizeof (struct vrrp_oper_table_index));

  ret = vrrp_snmp_index_get (v, name, length, &index, exact);
  if (ret < 0) 
    return (NULL);

  if (index.adtype != SNMP_AG_ADDR_TYPE_ipv4 && index.adtype != SNMP_AG_ADDR_TYPE_ipv6 
      && exact) {
    return (NULL);    
  }
  /* Return the current value of the variable */
  switch (v->magic) {
  case VRRPOPERATIONSVIRTUALMACADDR:
    if (VRRP_SNMP_GET3NEXT (oper_virtual_macaddr, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_mac_addr_val)) {
      if (! exact)
        vrrp_snmp_index_set (v, name, length, &index);
      VRRP_SNMP_RETURN_MACADDRESS (vrrp_mac_addr_val);
    }
    break; 
    
  case VRRPOPERATIONSSTATE:         
    if (VRRP_SNMP_GET3NEXT (oper_state, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
      if (! exact)
        vrrp_snmp_index_set (v, name, length, &index);
      VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
    }
    break; 
    
  case VRRPOPERATIONSPRIORITY:      
    *write_method = write_vrrpOperationsTable;
    if (VRRP_SNMP_GET3NEXT (oper_priority, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
      if (! exact)
        vrrp_snmp_index_set (v, name, length, &index);
      VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
    }
    break; 
    
  case VRRPOPERATIONSADDRCOUNT:     
    if (VRRP_SNMP_GET3NEXT (oper_addr_count, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
      if (! exact)
        vrrp_snmp_index_set (v, name, length, &index);
      VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
    }
    break; 
    
  case VRRPOPERATIONSMASTERIPADDR:
    if (VRRP_SNMP_GET3NEXT (oper_master_ipaddr, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, vrrp_in_addr_val.uia.in_addr)) {
      if (! exact)
        vrrp_snmp_index_set (v, name, length, &index);
      
      if (index.adtype == SNMP_AG_ADDR_TYPE_ipv4) {
        VRRP_SNMP_RETURN_IPADDRESS (IN_ADDR_SIZE, vrrp_in_addr_val.uia.in_addr);
      }
#ifdef HAVE_IPV6
      else if (index.adtype == SNMP_AG_ADDR_TYPE_ipv6) {
        VRRP_SNMP_RETURN_IPADDRESS (IN6_ADDR_SIZE, vrrp_in_addr_val.uia.in_addr);
      }
#endif
      else {
        return NULL;
      }
    }
    break; 
    
  case VRRPOPERATIONSPRIMARYIPADDR: 
    *write_method = write_vrrpOperationsTable;
    if (VRRP_SNMP_GET3NEXT (oper_primary_ipaddr, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, vrrp_in_addr_val.uia.in_addr)) {
      if (! exact)
        vrrp_snmp_index_set (v, name, length, &index);
      if (index.adtype == SNMP_AG_ADDR_TYPE_ipv4) {
        VRRP_SNMP_RETURN_IPADDRESS (IN_ADDR_SIZE, vrrp_in_addr_val.uia.in_addr);
      }
#ifdef HAVE_IPV6
      else if (index.adtype == SNMP_AG_ADDR_TYPE_ipv6) {
        VRRP_SNMP_RETURN_IPADDRESS (IN6_ADDR_SIZE, vrrp_in_addr_val.uia.in_addr);
      }
#endif
      else {
        return NULL;
      }
    }
    break; 
    
  case VRRPOPERATIONSADVINTERVAL:
    *write_method = write_vrrpOperationsTable;
    if (VRRP_SNMP_GET3NEXT (oper_adv_interval, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
      if (! exact)
        vrrp_snmp_index_set (v, name, length, &index);
      VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
    }
    break; 
    
  case VRRPOPERATIONSPREEMPTMODE:
    *write_method = write_vrrpOperationsTable;
    if (VRRP_SNMP_GET3NEXT (oper_preempt_mode, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
      if (! exact)
        vrrp_snmp_index_set (v, name, length, &index);
      VRRP_SNMP_RETURN_BOOL (vrrp_int_val);
    }
    break; 
    
  case VRRPOPERATIONSACCEPTMODE:
    *write_method = write_vrrpOperationsTable;
    if (VRRP_SNMP_GET3NEXT (oper_accept_mode, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
      if (! exact)
        vrrp_snmp_index_set (v, name, length, &index);
      VRRP_SNMP_RETURN_BOOL (vrrp_int_val);
    }
    break; 
  
  case VRRPOPERATIONSUPTIME:   
    if (VRRP_SNMP_GET3NEXT (oper_uptime, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_uint_val)) {
      if (! exact)
        vrrp_snmp_index_set (v, name, length, &index);
      VRRP_SNMP_RETURN_UINT32 (vrrp_uint_val);
    }
    break; 
    
  case VRRPOPERATIONSSTORAGETYPE:  
    *write_method = write_vrrpOperationsTable;
    if (VRRP_SNMP_GET3NEXT (oper_storage_type, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
      if (! exact)
        vrrp_snmp_index_set (v, name, length, &index);
      VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
    }
    break; 
    
  case VRRPOPERATIONSROWSTATUS:
    *write_method = write_vrrpOperationsTable;
    if (VRRP_SNMP_GET3NEXT (oper_rowstatus, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
      if (! exact)
        vrrp_snmp_index_set (v, name, length, &index);
      VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
    }
    break; 
  }
  return (NULL);
} 
  

/*--------------------------------------------------------
 * vrrpAssociatedIpAddrTable
 * 
 * write/get/getnext
 *--------------------------------------------------------
 */
int
write_vrrpAssociatedIpAddrTable (int       action, 
                                 u_char   *var_val,
                                 u_char    var_val_type, 
                                 size_t    var_val_len,
                                 u_char   *statP, 
                                 oid      *name, 
                                 size_t    length,
                                 struct    variable *v, 
                                 u_int32_t apn_vrid)
{
  int ret = 0;
  long intval = 0;
  struct vrrp_asso_ipaddr_table_index index;

  if (var_val_type == ASN_INTEGER) {
      pal_mem_cpy(&intval,var_val,4);
    }
  else
    return (SNMP_ERR_WRONGTYPE);

  pal_mem_set (&index, 0, sizeof (struct vrrp_asso_ipaddr_table_index));

  ret = vrrp_snmp_asso_index_get (v, name, &length, &index, 1);
  if (ret < 0) 
    return (SNMP_ERR_NOSUCHNAME);

  if (index.adtype!=SNMP_AG_ADDR_TYPE_ipv4 && index.adtype!=SNMP_AG_ADDR_TYPE_ipv6) {
    return (SNMP_ERR_NOSUCHNAME);  
  }

  switch (v->magic) {
  case VRRPASSOCIATEDSTORAGETYPE:
    if (vrrp_set_asso_storage_type (apn_vrid, 
                                    index.aftype, index.vrid, index.ifindex, index.ipaddr.uia.in_addr, 
                                    intval) != VRRP_API_SET_SUCCESS)
      return (SNMP_ERR_GENERR);
    break;
    
  case VRRPASSOCIATEDIPADDRROWSTATUS:
    if (vrrp_set_asso_ipaddr_rowstatus (apn_vrid, 
                                        index.aftype, index.vrid, index.ifindex, index.ipaddr.uia.in_addr, 
                                        intval) != VRRP_API_SET_SUCCESS)
      return (SNMP_ERR_GENERR);
    break;
    
  default:
    return (SNMP_ERR_GENERR);
    break;
  }
  return (SNMP_ERR_NOERROR);
}   
  
u_char *
vrrpAssociatedIpAddrTable (struct variable *v, 
                           oid    *name, 
                           size_t *length,
                           int     exact, 
                           size_t *var_len, 
                           WriteMethod **write_method,
                           u_int32_t apn_vrid)
{
  int ret;
  struct vrrp_asso_ipaddr_table_index index;
  *write_method = NULL;

  pal_mem_set (&index, 0, sizeof (struct vrrp_asso_ipaddr_table_index));

  ret = vrrp_snmp_asso_index_get (v, name, length, &index, exact);
  if (ret < 0) 
    return (NULL);

  if (index.adtype!=SNMP_AG_ADDR_TYPE_ipv4 && index.adtype!=SNMP_AG_ADDR_TYPE_ipv6 
      && exact) {
    return (NULL);  
  }
  /* Return the current value of the variable */
  switch (v->magic) {
  case VRRPASSOCIATEDSTORAGETYPE:
    *write_method = write_vrrpAssociatedIpAddrTable;
    if (VRRP_SNMP_GET4NEXT (asso_storage_type, 
                            index.aftype, index.vrid, index.ifindex, index.ipaddr.uia.in_addr, 
                            index.len, &vrrp_int_val)) {
          if (! exact)
            vrrp_snmp_asso_index_set (v, name, length, &index);
          VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
        }
      break;

  case VRRPASSOCIATEDIPADDRROWSTATUS:
    *write_method = write_vrrpAssociatedIpAddrTable;
    if (VRRP_SNMP_GET4NEXT (asso_ipaddr_rowstatus, 
                            index.aftype, index.vrid, index.ifindex, index.ipaddr.uia.in_addr, 
                            index.len, &vrrp_int_val)) {
      if (! exact)
        vrrp_snmp_asso_index_set (v, name, length, &index);
      VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
    }
    break;
  }
  return (NULL);
} 

/*--------------------------------------------------------
 * vrrpStatisticsTable
 * 
 * get/getnext
 *--------------------------------------------------------
 */
u_char *
vrrpStatisticsTable (struct variable *v, 
                     oid *name, 
                     size_t *length,
                     int exact, 
                     size_t *var_len, 
                     WriteMethod **write_method,
                     u_int32_t apn_vrid)
{
  int ret;
  struct vrrp_oper_table_index index;

  *write_method = NULL;

  pal_mem_set (&index, 0, sizeof (struct vrrp_oper_table_index));

  ret = vrrp_snmp_index_get (v, name, length, &index, exact);
  if (ret < 0) 
    return(NULL);

  if (index.adtype!=SNMP_AG_ADDR_TYPE_ipv4 && index.adtype!=SNMP_AG_ADDR_TYPE_ipv6 && exact) {
    return(NULL);  
  }

  /* Return the current value of the variable */
  switch (v->magic) {
  case VRRPSTATISTICSMASTERTRANSITIONS:     /* 1 -- vrrpStatisticsMasterTransitions */
    if (VRRP_SNMP_GET3NEXT (stats_master_transitions, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
            if (! exact)
              vrrp_snmp_index_set (v, name, length, &index);
            VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
          }
      break; 
  
  case VRRPSTATISTICSRCVDADVERTISEMENTS:    /* 2 -- vrrpStatisticssRcvdAdertisements */
    if (VRRP_SNMP_GET3NEXT (stats_rcvd_advertisements, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
            if (! exact)
              vrrp_snmp_index_set (v, name, length, &index);
            VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
          }
      break; 
  
  case VRRPSTATISTICSADVINTERVALERRORS:     /* 3 -- vrrpStatistisAdvIntervalErrors */
    if (VRRP_SNMP_GET3NEXT (stats_adv_interval_errors, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
            if (! exact)
              vrrp_snmp_index_set (v, name, length, &index);
            VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
          }
      break; 
  
  case VRRPSTATISTICSIPTTLERRORS:           /* 4 -- vrrpStatisticsIpTtlErrors */
    if (VRRP_SNMP_GET3NEXT (stats_ip_ttl_errors, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
            if (! exact)
              vrrp_snmp_index_set (v, name, length, &index);
            VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
          }
      break; 
  
  case VRRPSTATISTICSRCVDPRIZEROPACKETS:    /* 5 -- vrrpStatisticsRcvdPriZeroPkts */
    if (VRRP_SNMP_GET3NEXT (stats_rcvd_pri_zero_packets, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
            if (! exact)
              vrrp_snmp_index_set (v, name, length, &index);
            VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
          }
      break; 
  
  case VRRPSTATISTICSSENTPRIZEROPACKETS:    /* 6 -- vrrpStatisticsSentPriZeroPkts */
    if (VRRP_SNMP_GET3NEXT (stats_sent_pri_zero_packets, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
            if (! exact)
              vrrp_snmp_index_set (v, name, length, &index);
            VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
          }
      break; 
  
  case VRRPSTATISTICSRCVDINVALIDTYPEPKTS:  /* 7 -- vrrpStatisticsRcvdInvalidTypePkts*/
    if (VRRP_SNMP_GET3NEXT (stats_rcvd_invalid_type_pkts, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
            if (! exact)
              vrrp_snmp_index_set (v, name, length, &index);
            VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
          }
      break; 
  
  case VRRPSTATISTICSADDRESSLISTERRORS:     /* 8 -- vrrpStatisticsAddressListErrors*/
    if (VRRP_SNMP_GET3NEXT (stats_address_list_errors, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
            if (! exact)
              vrrp_snmp_index_set (v, name, length, &index);
            VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
          }
      break; 
  
  case VRRPSTATISTICSPACKETLENGTHERRORS:    /* 9 -- vrrpStatisticsPacketLengthErrors*/
    if (VRRP_SNMP_GET3NEXT (stats_packet_length_errors, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
            if (! exact)
              vrrp_snmp_index_set (v, name, length, &index);
            VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
          }
      break; 
  
  case VRRPSTATISTICSRCVDINVALIDAUTHENTICATIONS: /* 10 -- vrrpStatisticsRcvdInvalidAuthenthication */
    if (VRRP_SNMP_GET3NEXT (stats_rcvd_invalid_authentications, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
            if (! exact)
              vrrp_snmp_index_set (v, name, length, &index);
            VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
          }
        break; 

  case VRRPSTATISTICSDISCONTINUITYTIME:     /* 11 -- vrrpStatisticsDiscontinuityTime */
    if (VRRP_SNMP_GET3NEXT (stats_discontinuity_time, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_uint_val)) {
            if (! exact)
              vrrp_snmp_index_set (v, name, length, &index);
      VRRP_SNMP_RETURN_UINT32 (vrrp_uint_val);
    }
      break; 
  
  case VRRPSTATISTICSREFRESHRATE:           /* 12 -- vrrpStatisticsRefreshRate */
    if (VRRP_SNMP_GET3NEXT (stats_refresh_rate, 
                            index.aftype, index.vrid, index.ifindex,
                            index.len, &vrrp_int_val)) {
            if (! exact)
              vrrp_snmp_index_set (v, name, length, &index);
            VRRP_SNMP_RETURN_INTEGER (vrrp_int_val);
          }
      break; 
  }
  return(NULL);
}

void
vrrp_trap_new_master (VRRP_SESSION *sess, int reason)
{
  struct trap_object2 obj[2];
  struct vrrp_inx_addr master_ipaddr;
  int namelen;
  oid *ptr;
  
  pal_mem_set (&master_ipaddr, 0, sizeof (struct vrrp_inx_addr));
  namelen = sizeof vrrp_oid / sizeof (oid);

  /*Get vrrpOperMasterIpAddr */
  vrrp_get_oper_master_ipaddr (sess->vrrp->nm->vr->id, 
                               sess->af_type, 
                               sess->vrid, 
                               sess->ifindex, 
                               (u_int8_t *)&master_ipaddr);
  ptr = obj[0].name;
  OID_COPY (ptr, vrrp_oid, namelen);
  OID_SET_ARG4 (ptr, 1, 7, 1, 7);
  if (sess->af_type==AF_INET) {
    OID_SET_VAL (obj[0], ptr - obj[0].name, IPADDRESS, IN_ADDR_SIZE, &master_ipaddr.uia.in4_addr);
  }
#ifdef HAVE_IPV6
  else {
    OID_SET_VAL (obj[0], ptr - obj[0].name, IPADDRESS, IN6_ADDR_SIZE, &master_ipaddr.uia.in6_addr);
  }
#endif
  ptr = obj[1].name;
  OID_COPY (ptr, vrrp_oid, namelen);
  OID_SET_ARG2 (ptr, 1, 9);
  OID_SET_VAL (obj[1], ptr - obj[1].name, INTEGER, sizeof(int), &reason);

  snmp_trap2 (nzg,
              vrrp_trap_oid, sizeof vrrp_trap_oid / sizeof (oid),
              VRRPTRAPNEWMASTER,
              vrrp_oid, sizeof vrrp_oid / sizeof (oid),
              obj, sizeof obj / sizeof (struct trap_object2),
              sess->vrrp_uptime);
}

void
vrrp_trap_proto_error (VRRP_SESSION *sess, int error)
{
  struct trap_object2 obj[1];
  int namelen;
  oid *ptr;

  namelen = sizeof vrrp_oid / sizeof (oid); 

  ptr = obj[0].name;
  OID_COPY (ptr, vrrp_oid, namelen);
  OID_SET_ARG2 (ptr, 1, 10);
  OID_SET_VAL (obj[0], ptr - obj[0].name, INTEGER, sizeof(int), &error);

  snmp_trap2 (nzg,
              vrrp_trap_oid, sizeof vrrp_trap_oid / sizeof (oid),
              VRRPTRAPPROTOERROR,
              vrrp_oid, sizeof vrrp_oid / sizeof (oid),
              obj, sizeof obj / sizeof (struct trap_object2),
              sess->vrrp_uptime);
}

/* Register VRRP MIB. */
void
vrrp_snmp_init (struct lib_globals *zg)
{
  REGISTER_MIB (zg, "mibII/vrrp", vrrp_variables, variable, vrrp_oid);
}

#ifdef HAVE_SNMP_RESTART
void
vrrp_snmp_restart ()
{
  snmp_restart (nzg);
}

#endif /* HAVE_SNMP_RESTART */
#endif /* HAVE_SNMP */
