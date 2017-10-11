/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_SNMP

#include "lib.h"
#include "if.h"
#include "prefix.h"
#include "log.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_vlink.h"
#include "ospfd/ospf_packet.h"
#include "ospfd/ospf_neighbor.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_abr.h"
#include "ospfd/ospf_api.h"
#include "ospfd/ospf_cli.h"
#include "ospfd/ospf_snmp.h"


/* Declare static local variables for convenience. */
static s_int32_t ospf_int_val;
static struct pal_in4_addr ospf_in_addr_val;
static u_char *ospf_char_ptr;

/* OSPF-MIB instances. */
oid ospf_oid [] = { OSPF2MIB };
oid ospfd_oid [] = { OSPFDOID };

/* Hook functions. */
static u_char *ospfGeneralGroup ();
static u_char *ospfAreaEntry ();
static u_char *ospfStubAreaEntry ();
static u_char *ospfLsdbEntry ();
static u_char *ospfAreaRangeEntry ();
static u_char *ospfHostEntry ();
static u_char *ospfIfEntry ();
static u_char *ospfIfMetricEntry ();
static u_char *ospfVirtIfEntry ();
static u_char *ospfNbrEntry ();
static u_char *ospfVirtNbrEntry ();
static u_char *ospfExtLsdbEntry ();
static u_char *ospfAreaAggregateEntry ();

struct variable ospf_variables[] =
{
  /* OSPF general variables */
  {OSPFROUTERID,              IPADDRESS, RWRITE, ospfGeneralGroup,
   2, {1, 1}},
  {OSPFADMINSTAT,             INTEGER, RWRITE, ospfGeneralGroup,
   2, {1, 2}},
  {OSPFVERSIONNUMBER,         INTEGER, RONLY, ospfGeneralGroup,
   2, {1, 3}},
  {OSPFAREABDRRTRSTATUS,      INTEGER, RONLY, ospfGeneralGroup,
   2, {1, 4}},
  {OSPFASBDRRTRSTATUS,        INTEGER, RWRITE, ospfGeneralGroup,
   2, {1, 5}},
  {OSPFEXTERNLSACOUNT,        GAUGE, RONLY, ospfGeneralGroup,
   2, {1, 6}},
  {OSPFEXTERNLSACKSUMSUM,     INTEGER, RONLY, ospfGeneralGroup,
   2, {1, 7}},
  {OSPFTOSSUPPORT,            INTEGER, RWRITE, ospfGeneralGroup,
   2, {1, 8}},
  {OSPFORIGINATENEWLSAS,      COUNTER, RONLY, ospfGeneralGroup,
   2, {1, 9}},
  {OSPFRXNEWLSAS,             COUNTER, RONLY, ospfGeneralGroup,
   2, {1, 10}},
  {OSPFEXTLSDBLIMIT,          INTEGER, RWRITE, ospfGeneralGroup,
   2, {1, 11}},
  {OSPFMULTICASTEXTENSIONS,   INTEGER, RWRITE, ospfGeneralGroup,
   2, {1, 12}},
  {OSPFEXITOVERFLOWINTERVAL,  INTEGER, RWRITE, ospfGeneralGroup,
   2, {1, 13}},
  {OSPFDEMANDEXTENSIONS,      INTEGER, RWRITE, ospfGeneralGroup,
   2, {1, 14}},

  /* OSPF area data structure. */
  {OSPFAREAID,                IPADDRESS, RONLY, ospfAreaEntry,
   3, {2, 1, 1}},
  {OSPFAUTHTYPE,              INTEGER, RWRITE, ospfAreaEntry,
   3, {2, 1, 2}},
  {OSPFIMPORTASEXTERN,        INTEGER, RWRITE, ospfAreaEntry,
   3, {2, 1, 3}},
  {OSPFSPFRUNS,               COUNTER, RONLY, ospfAreaEntry,
   3, {2, 1, 4}},
  {OSPFAREABDRRTRCOUNT,       GAUGE, RONLY, ospfAreaEntry,
   3, {2, 1, 5}},
  {OSPFASBDRRTRCOUNT,         GAUGE, RONLY, ospfAreaEntry,
   3, {2, 1, 6}},
  {OSPFAREALSACOUNT,          GAUGE, RONLY, ospfAreaEntry,
   3, {2, 1, 7}},
  {OSPFAREALSACKSUMSUM,       INTEGER, RONLY, ospfAreaEntry,
   3, {2, 1, 8}},
  {OSPFAREASUMMARY,           INTEGER, RWRITE, ospfAreaEntry,
   3, {2, 1, 9}},
  {OSPFAREASTATUS,            INTEGER, RWRITE, ospfAreaEntry,
   3, {2, 1, 10}},

  /* OSPF stub area information. */
  {OSPFSTUBAREAID,            IPADDRESS, RONLY, ospfStubAreaEntry,
   3, {3, 1, 1}},
  {OSPFSTUBTOS,               INTEGER, RONLY, ospfStubAreaEntry,
   3, {3, 1, 2}},
  {OSPFSTUBMETRIC,            INTEGER, RWRITE, ospfStubAreaEntry,
   3, {3, 1, 3}},
  {OSPFSTUBSTATUS,            INTEGER, RWRITE, ospfStubAreaEntry,
   3, {3, 1, 4}},
  {OSPFSTUBMETRICTYPE,        INTEGER, RWRITE, ospfStubAreaEntry,
   3, {3, 1, 5}},

  /* OSPF link state database. */
  {OSPFLSDBAREAID,            IPADDRESS, RONLY, ospfLsdbEntry,
   3, {4, 1, 1}},
  {OSPFLSDBTYPE,              INTEGER, RONLY, ospfLsdbEntry,
   3, {4, 1, 2}},
  {OSPFLSDBLSID,              IPADDRESS, RONLY, ospfLsdbEntry,
   3, {4, 1, 3}},
  {OSPFLSDBROUTERID,          IPADDRESS, RONLY, ospfLsdbEntry,
   3, {4, 1, 4}},
  {OSPFLSDBSEQUENCE,          INTEGER, RONLY, ospfLsdbEntry,
   3, {4, 1, 5}},
  {OSPFLSDBAGE,               INTEGER, RONLY, ospfLsdbEntry,
   3, {4, 1, 6}},
  {OSPFLSDBCHECKSUM,          INTEGER, RONLY, ospfLsdbEntry,
   3, {4, 1, 7}},
  {OSPFLSDBADVERTISEMENT,     STRING, RONLY, ospfLsdbEntry,
   3, {4, 1, 8}},

  /* Area range table. */
  {OSPFAREARANGEAREAID,       IPADDRESS, RONLY, ospfAreaRangeEntry,
   3, {5, 1, 1}},
  {OSPFAREARANGENET,          IPADDRESS, RONLY, ospfAreaRangeEntry,
   3, {5, 1, 2}},
  {OSPFAREARANGEMASK,         IPADDRESS, RWRITE, ospfAreaRangeEntry,
   3, {5, 1, 3}},
  {OSPFAREARANGESTATUS,       INTEGER, RWRITE, ospfAreaRangeEntry,
   3, {5, 1, 4}},
  {OSPFAREARANGEEFFECT,       INTEGER, RWRITE, ospfAreaRangeEntry,
   3, {5, 1, 5}},

  /* OSPF host table. */
  {OSPFHOSTIPADDRESS,         IPADDRESS, RONLY, ospfHostEntry,
   3, {6, 1, 1}},
  {OSPFHOSTTOS,               INTEGER, RONLY, ospfHostEntry,
   3, {6, 1, 2}},
  {OSPFHOSTMETRIC,            INTEGER, RWRITE, ospfHostEntry,
   3, {6, 1, 3}},
  {OSPFHOSTSTATUS,            INTEGER, RWRITE, ospfHostEntry,
   3, {6, 1, 4}},
  {OSPFHOSTAREAID,            IPADDRESS, RONLY, ospfHostEntry,
   3, {6, 1, 5}},

  /* OSPF interface table. */
  {OSPFIFIPADDRESS,           IPADDRESS, RONLY, ospfIfEntry,
   3, {7, 1, 1}},
  {OSPFADDRESSLESSIF,         INTEGER, RONLY, ospfIfEntry,
   3, {7, 1, 2}},
  {OSPFIFAREAID,              IPADDRESS, RWRITE, ospfIfEntry,
   3, {7, 1, 3}},
  {OSPFIFTYPE,                INTEGER, RWRITE, ospfIfEntry,
   3, {7, 1, 4}},
  {OSPFIFADMINSTAT,           INTEGER, RWRITE, ospfIfEntry,
   3, {7, 1, 5}},
  {OSPFIFRTRPRIORITY,         INTEGER, RWRITE, ospfIfEntry,
   3, {7, 1, 6}},
  {OSPFIFTRANSITDELAY,        INTEGER, RWRITE, ospfIfEntry,
   3, {7, 1, 7}},
  {OSPFIFRETRANSINTERVAL,     INTEGER, RWRITE, ospfIfEntry,
   3, {7, 1, 8}},
  {OSPFIFHELLOINTERVAL,       INTEGER, RWRITE, ospfIfEntry,
   3, {7, 1, 9}},
  {OSPFIFRTRDEADINTERVAL,     INTEGER, RWRITE, ospfIfEntry,
   3, {7, 1, 10}},
  {OSPFIFPOLLINTERVAL,        INTEGER, RWRITE, ospfIfEntry,
   3, {7, 1, 11}},
  {OSPFIFSTATE,               INTEGER, RONLY, ospfIfEntry,
   3, {7, 1, 12}},
  {OSPFIFDESIGNATEDROUTER,    IPADDRESS, RONLY, ospfIfEntry,
   3, {7, 1, 13}},
  {OSPFIFBACKUPDESIGNATEDROUTER, IPADDRESS, RONLY, ospfIfEntry,
   3, {7, 1, 14}},
  {OSPFIFEVENTS,              COUNTER, RONLY, ospfIfEntry,
   3, {7, 1, 15}},
  {OSPFIFAUTHKEY,             STRING,  RWRITE, ospfIfEntry,
   3, {7, 1, 16}},
  {OSPFIFSTATUS,              INTEGER, RWRITE, ospfIfEntry,
   3, {7, 1, 17}},
  {OSPFIFMULTICASTFORWARDING, INTEGER, RWRITE, ospfIfEntry,
   3, {7, 1, 18}},
  {OSPFIFDEMAND,              INTEGER, RWRITE, ospfIfEntry,
   3, {7, 1, 19}},
  {OSPFIFAUTHTYPE,            INTEGER, RWRITE, ospfIfEntry,
   3, {7, 1, 20}},

  /* OSPF interface metric table. */
  {OSPFIFMETRICIPADDRESS,     IPADDRESS, RONLY, ospfIfMetricEntry,
   3, {8, 1, 1}},
  {OSPFIFMETRICADDRESSLESSIF, INTEGER, RONLY, ospfIfMetricEntry,
   3, {8, 1, 2}},
  {OSPFIFMETRICTOS,           INTEGER, RONLY, ospfIfMetricEntry,
   3, {8, 1, 3}},
  {OSPFIFMETRICVALUE,         INTEGER, RWRITE, ospfIfMetricEntry,
   3, {8, 1, 4}},
  {OSPFIFMETRICSTATUS,        INTEGER, RWRITE, ospfIfMetricEntry,
   3, {8, 1, 5}},

  /* OSPF virtual interface table. */
  {OSPFVIRTIFAREAID,          IPADDRESS, RONLY, ospfVirtIfEntry,
   3, {9, 1, 1}},
  {OSPFVIRTIFNEIGHBOR,        IPADDRESS, RONLY, ospfVirtIfEntry,
   3, {9, 1, 2}},
  {OSPFVIRTIFTRANSITDELAY,    INTEGER, RWRITE, ospfVirtIfEntry,
   3, {9, 1, 3}},
  {OSPFVIRTIFRETRANSINTERVAL, INTEGER, RWRITE, ospfVirtIfEntry,
   3, {9, 1, 4}},
  {OSPFVIRTIFHELLOINTERVAL,   INTEGER, RWRITE, ospfVirtIfEntry,
   3, {9, 1, 5}},
  {OSPFVIRTIFRTRDEADINTERVAL, INTEGER, RWRITE, ospfVirtIfEntry,
   3, {9, 1, 6}},
  {OSPFVIRTIFSTATE,           INTEGER, RONLY, ospfVirtIfEntry,
   3, {9, 1, 7}},
  {OSPFVIRTIFEVENTS,          COUNTER, RONLY, ospfVirtIfEntry,
   3, {9, 1, 8}},
  {OSPFVIRTIFAUTHKEY,         STRING,  RWRITE, ospfVirtIfEntry,
   3, {9, 1, 9}},
  {OSPFVIRTIFSTATUS,          INTEGER, RWRITE, ospfVirtIfEntry,
   3, {9, 1, 10}},
  {OSPFVIRTIFAUTHTYPE,        INTEGER, RWRITE, ospfVirtIfEntry,
   3, {9, 1, 11}},

  /* OSPF neighbor table. */
  {OSPFNBRIPADDR,             IPADDRESS, RONLY, ospfNbrEntry,
   3, {10, 1, 1}},
  {OSPFNBRADDRESSLESSINDEX,   INTEGER, RONLY, ospfNbrEntry,
   3, {10, 1, 2}},
  {OSPFNBRRTRID,              IPADDRESS, RONLY, ospfNbrEntry,
   3, {10, 1, 3}},
  {OSPFNBROPTIONS,            INTEGER, RONLY, ospfNbrEntry,
   3, {10, 1, 4}},
  {OSPFNBRPRIORITY,           INTEGER, RWRITE, ospfNbrEntry,
   3, {10, 1, 5}},
  {OSPFNBRSTATE,              INTEGER, RONLY, ospfNbrEntry,
   3, {10, 1, 6}},
  {OSPFNBREVENTS,             COUNTER, RONLY, ospfNbrEntry,
   3, {10, 1, 7}},
  {OSPFNBRLSRETRANSQLEN,      GAUGE, RONLY, ospfNbrEntry,
   3, {10, 1, 8}},
  {OSPFNBMANBRSTATUS,         INTEGER, RWRITE, ospfNbrEntry,
   3, {10, 1, 9}},
  {OSPFNBMANBRPERMANENCE,     INTEGER, RONLY, ospfNbrEntry,
   3, {10, 1, 10}},
  {OSPFNBRHELLOSUPPRESSED,    INTEGER, RONLY, ospfNbrEntry,
   3, {10, 1, 11}},

  /* OSPF virtual neighbor table. */
  {OSPFVIRTNBRAREA,           IPADDRESS, RONLY, ospfVirtNbrEntry,
   3, {11, 1, 1}},
  {OSPFVIRTNBRRTRID,          IPADDRESS, RONLY, ospfVirtNbrEntry,
   3, {11, 1, 2}},
  {OSPFVIRTNBRIPADDR,         IPADDRESS, RONLY, ospfVirtNbrEntry,
   3, {11, 1, 3}},
  {OSPFVIRTNBROPTIONS,        INTEGER, RONLY, ospfVirtNbrEntry,
   3, {11, 1, 4}},
  {OSPFVIRTNBRSTATE,          INTEGER, RONLY, ospfVirtNbrEntry,
   3, {11, 1, 5}},
  {OSPFVIRTNBREVENTS,         COUNTER, RONLY, ospfVirtNbrEntry,
   3, {11, 1, 6}},
  {OSPFVIRTNBRLSRETRANSQLEN,  GAUGE, RONLY, ospfVirtNbrEntry,
   3, {11, 1, 7}},
  {OSPFVIRTNBRHELLOSUPPRESSED, INTEGER, RONLY, ospfVirtNbrEntry,
   3, {11, 1, 8}},

  /* OSPF link state database, external. */
  {OSPFEXTLSDBTYPE,           INTEGER, RONLY, ospfExtLsdbEntry,
   3, {12, 1, 1}},
  {OSPFEXTLSDBLSID,           IPADDRESS, RONLY, ospfExtLsdbEntry,
   3, {12, 1, 2}},
  {OSPFEXTLSDBROUTERID,       IPADDRESS, RONLY, ospfExtLsdbEntry,
   3, {12, 1, 3}},
  {OSPFEXTLSDBSEQUENCE,       INTEGER, RONLY, ospfExtLsdbEntry,
   3, {12, 1, 4}},
  {OSPFEXTLSDBAGE,            INTEGER, RONLY, ospfExtLsdbEntry,
   3, {12, 1, 5}},
  {OSPFEXTLSDBCHECKSUM,       INTEGER, RONLY, ospfExtLsdbEntry,
   3, {12, 1, 6}},
  {OSPFEXTLSDBADVERTISEMENT,  STRING,  RONLY, ospfExtLsdbEntry,
   3, {12, 1, 7}},

  /* OSPF area aggregate table. */
  {OSPFAREAAGGREGATEAREAID,   IPADDRESS, RONLY, ospfAreaAggregateEntry,
   3, {14, 1, 1}},
  {OSPFAREAAGGREGATELSDBTYPE, INTEGER, RONLY, ospfAreaAggregateEntry,
   3, {14, 1, 2}},
  {OSPFAREAAGGREGATENET,      IPADDRESS, RONLY, ospfAreaAggregateEntry,
   3, {14, 1, 3}},
  {OSPFAREAAGGREGATEMASK,     IPADDRESS, RONLY, ospfAreaAggregateEntry,
   3, {14, 1, 4}},
  {OSPFAREAAGGREGATESTATUS,   INTEGER, RWRITE, ospfAreaAggregateEntry,
   3, {14, 1, 5}},
  {OSPFAREAAGGREGATEEFFECT,   INTEGER, RWRITE, ospfAreaAggregateEntry,
   3, {14, 1, 6}}
};


#define OSPF_SNMP_INDEX_VARS_MAX  8
struct ospf_snmp_index
{
  unsigned int len;
  unsigned int val[OSPF_SNMP_INDEX_VARS_MAX];
};

struct ospf_area_table_index
{
  unsigned int len;
  struct pal_in4_addr area_id;
};

struct ospf_stub_area_table_index
{
  unsigned int len;
  struct pal_in4_addr area_id;
  signed int tos;
};

struct ospf_lsdb_table_index
{
  unsigned int len;
  struct pal_in4_addr area_id;
  int type;
  struct pal_in4_addr ls_id;
  struct pal_in4_addr adv_router;
};

struct ospf_area_range_table_index
{
  unsigned int len;
  struct pal_in4_addr area_id;
  struct pal_in4_addr net;
};

struct ospf_host_table_index
{
  unsigned int len;
  struct pal_in4_addr addr;
  signed int tos;
};

struct ospf_if_table_index
{
  unsigned int len;
  struct pal_in4_addr addr;
  signed int ifindex;
};

struct ospf_if_metric_table_index
{
  unsigned int len;
  struct pal_in4_addr addr;
  signed int ifindex;
  signed int tos;
};

struct ospf_virt_if_table_index
{
  unsigned int len;
  struct pal_in4_addr area_id;
  struct pal_in4_addr nbr_id;
};

struct ospf_nbr_table_index
{
  unsigned int len;
  struct pal_in4_addr nbr_addr;
  signed int ifindex;
};

struct ospf_virt_nbr_table_index
{
  unsigned int len;
  struct pal_in4_addr area_id;
  struct pal_in4_addr nbr_id;
};

struct ospf_ext_lsdb_table_index
{
  unsigned int len;
  signed int type;
  struct pal_in4_addr ls_id;
  struct pal_in4_addr adv_router;
};

struct ospf_area_aggregate_table_index
{
  unsigned int len;
  struct pal_in4_addr area_id;
  signed int type;
  struct pal_in4_addr net;
  struct pal_in4_addr mask;
};


#define OSPF_SNMP_INDEX_GET_ERROR    0
#define OSPF_SNMP_INDEX_GET_SUCCESS  1
#define OSPF_SNMP_INDEX_SET_SUCCESS  1

/* Get index array from oid. */
int
ospf_snmp_index_get (struct variable *v, oid *name, size_t *length,
		     struct ospf_snmp_index *index, int type, int exact)
{
  int i = 0;
  int offset = 0;

  index->len = *length - v->namelen;

  if (index->len == 0)
    return OSPF_SNMP_INDEX_GET_SUCCESS;

  if (exact)
    if (index->len != ospf_api_table_def[type].len)
      return OSPF_SNMP_INDEX_GET_ERROR;

  if (index->len > ospf_api_table_def[type].len)
    return OSPF_SNMP_INDEX_GET_ERROR;

  while (offset < index->len)
    {
      unsigned int val = 0;
      unsigned int vartype = ospf_api_table_def[type].vars[i];
      int j;

      if (vartype == LS_INDEX_NONE)
	break;

      switch (vartype)
	{
	case LS_INDEX_INT8:
	case LS_INDEX_INT16:
	case LS_INDEX_INT32:
	  index->val[i++] = name[v->namelen + offset++];
	  break;
	case LS_INDEX_INADDR:
	  for (j = 1; j <= 4 && offset < index->len; j++)
	    val += name[v->namelen + offset++] << ((4 - j) * 8);

	  index->val[i++] = pal_hton32 (val);
	  break;
	default:
	  pal_assert (0);
	}
    }

  return OSPF_SNMP_INDEX_GET_SUCCESS;
}

/* Set index array to oid. */
int
ospf_snmp_index_set (struct variable *v, oid *name, size_t *length,
		     struct ospf_snmp_index *index, int type)
{
  int i = 0;
  int offset = 0;

  index->len = ospf_api_table_def[type].len;

  while (offset < index->len)
    {
      unsigned long val;
      unsigned int vartype = ospf_api_table_def[type].vars[i];

      if (vartype == LS_INDEX_NONE)
	break;

      switch (vartype)
	{
	case LS_INDEX_INT8:
	case LS_INDEX_INT16:
	case LS_INDEX_INT32:
	  name[v->namelen + offset++] = index->val[i++];
	  break;
	case LS_INDEX_INADDR:
	  val = pal_ntoh32 (index->val[i++]);
	  name[v->namelen + offset++] = (val >> 24) & 0xff;
	  name[v->namelen + offset++] = (val >> 16) & 0xff;
	  name[v->namelen + offset++] = (val >> 8) & 0xff;
	  name[v->namelen + offset++] = val & 0xff;
	  break;
	default:
	  pal_assert (0);
	}
    }

  *length = v->namelen + index->len;

  return OSPF_SNMP_INDEX_SET_SUCCESS;
}


/* SMUX callback functions. */
int
write_ospf_router_id (int action, u_char *var_val,
		      u_char var_val_type, size_t var_val_len,
		      u_char *statP, oid *name, size_t length,
		      struct variable *v, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  struct pal_in4_addr addr;
  int ret;

  if (!ospf_proc_lookup_any (proc_id, vr_id))
    return SNMP_ERR_GENERR;

  if (var_val_type != ASN_IPADDRESS)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (struct pal_in4_addr))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy (&addr.s_addr, var_val, 4);

  if (!OSPF_SNMP_SET (router_id, addr, vr_id))
    return OSPF_SNMP_RETURN_ERROR (ret);

  return SNMP_ERR_NOERROR;
}

int
write_ospfGeneralGroup (int action, u_char *var_val,
			u_char var_val_type, size_t var_val_len,
			u_char *statP, oid *name, size_t length,
			struct variable *v, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  long intval;
  int ret;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy (&intval, var_val, 4);
  
  switch (v->magic)
    {
    case OSPFADMINSTAT:            /* 2 -- ospfAdminStat. */
      if (!OSPF_SNMP_SET (admin_stat, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFASBDRRTRSTATUS:       /* 5 -- ospfAsbdrRtrStatus. */
      if (!OSPF_SNMP_SET (asbdr_rtr_status, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFTOSSUPPORT:           /* 8 -- ospfTOSSupport. */
      if (!OSPF_SNMP_SET (tos_support, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFEXTLSDBLIMIT:         /* 11 -- ospfExtLsdbLimit. */
      if (!OSPF_SNMP_SET (ext_lsdb_limit, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFMULTICASTEXTENSIONS:  /* 12 -- ospfMulticastExtensions. */
      if (!OSPF_SNMP_SET (multicast_extensions, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFEXITOVERFLOWINTERVAL: /* 13 -- ospfExitOverFlowInterval. */
      if (!OSPF_SNMP_SET (exit_overflow_interval, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFDEMANDEXTENSIONS:     /* 14 -- ospfDemandExtensions. */
      if (!OSPF_SNMP_SET (demand_extensions, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    }

  return SNMP_ERR_NOERROR;
}

/* ospfGeneralGroup { ospf 1 } */
static u_char *
ospfGeneralGroup (struct variable *v, oid *name, size_t *length,
		  int exact, size_t *var_len, WriteMethod **write_method, 
                  u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;

  /* Check whether the instance identifier is valid */
  if (snmp_header_generic (v, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return NULL;

  /* Return the current value of the variable */
  switch (v->magic)
    {
    case OSPFROUTERID:             /* 1 -- ospfRouterId. */
      *write_method = write_ospf_router_id;
      if (OSPF_SNMP_GET (router_id, &ospf_in_addr_val, vr_id))
	{
	  *var_len = sizeof (ospf_in_addr_val);
	  return (u_char *)&ospf_in_addr_val;
	}
      break;
    case OSPFADMINSTAT:	           /* 2 -- ospfAdminStat. */
      *write_method = write_ospfGeneralGroup;
      if (OSPF_SNMP_GET (admin_stat, &ospf_int_val, vr_id))
	{
	  *var_len = sizeof (ospf_int_val);
	  return (u_char *)&ospf_int_val;
	}
      break;
    case OSPFVERSIONNUMBER:        /* 3 -- ospfVersionNumber. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET (version_number, &ospf_int_val, vr_id))
	{
	  *var_len = sizeof (ospf_int_val);
	  return (u_char *)&ospf_int_val;
	}
      break;
    case OSPFAREABDRRTRSTATUS:     /* 4 -- ospfAreaBdrRtrStatus. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET (area_bdr_rtr_status, &ospf_int_val, vr_id))
	{
	  *var_len = sizeof (ospf_int_val);
	  return (u_char *)&ospf_int_val;
	}
      break;
    case OSPFASBDRRTRSTATUS:       /* 5 -- ospfASBdrRtrStatus. */
      *write_method = write_ospfGeneralGroup;
      if (OSPF_SNMP_GET (asbdr_rtr_status, &ospf_int_val, vr_id))
	{
	  *var_len = sizeof (ospf_int_val);
	  return (u_char *)&ospf_int_val;
	}
      break;
    case OSPFEXTERNLSACOUNT:	   /* 6 -- ospfExternLsaCount. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET (extern_lsa_count, &ospf_int_val, vr_id))
	{
	  *var_len = sizeof (ospf_int_val);
	  return (u_char *)&ospf_int_val;
	}
      break;
    case OSPFEXTERNLSACKSUMSUM:    /* 7 -- ospfExternLsaCksumSum. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET (extern_lsa_cksum_sum, &ospf_int_val, vr_id))
	{
	  *var_len = sizeof (ospf_int_val);
	  return (u_char *)&ospf_int_val;
	}
      break;
    case OSPFTOSSUPPORT:           /* 8 -- ospfTOSSupport. */
      *write_method = write_ospfGeneralGroup;
      if (OSPF_SNMP_GET (tos_support, &ospf_int_val, vr_id))
	{
	  *var_len = sizeof (ospf_int_val);
	  return (u_char *)&ospf_int_val;
	}
      break;
    case OSPFORIGINATENEWLSAS:     /* 9 -- ospfOriginateNewLsas. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET (originate_new_lsas, &ospf_int_val, vr_id))
	{
	  *var_len = sizeof (ospf_int_val);
	  return (u_char *)&ospf_int_val;
	}
      break;
    case OSPFRXNEWLSAS:            /* 10 -- ospfRxNewLsas. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET (rx_new_lsas, &ospf_int_val, vr_id))
	{
	  *var_len = sizeof (ospf_int_val);
	  return (u_char *)&ospf_int_val;
	}
      break;
    case OSPFEXTLSDBLIMIT:         /* 11 -- ospfExtLsdbLimit. */
      *write_method = write_ospfGeneralGroup;
      if (OSPF_SNMP_GET (ext_lsdb_limit, &ospf_int_val, vr_id))
	{
	  *var_len = sizeof (ospf_int_val);
	  return (u_char *)&ospf_int_val;
	}
      break;
    case OSPFMULTICASTEXTENSIONS:  /* 12 -- ospfMulticastExtensions. */
      *write_method = write_ospfGeneralGroup;
      if (OSPF_SNMP_GET (multicast_extensions, &ospf_int_val, vr_id))
	{
	  *var_len = sizeof (ospf_int_val);
	  return (u_char *)&ospf_int_val;
	}
      break;
    case OSPFEXITOVERFLOWINTERVAL: /* 13 -- ospfExitOverflowInterval. */
      *write_method = write_ospfGeneralGroup;
      if (OSPF_SNMP_GET (exit_overflow_interval, &ospf_int_val, vr_id))
	{
	  *var_len = sizeof (ospf_int_val);
	  return (u_char *)&ospf_int_val;
	}
      break;
    case OSPFDEMANDEXTENSIONS:     /* 14 -- ospfDemandExtensions. */
      *write_method = write_ospfGeneralGroup;
      if (OSPF_SNMP_GET (demand_extensions, &ospf_int_val, vr_id))
	{
	  *var_len = sizeof (ospf_int_val);
	  return (u_char *)&ospf_int_val;
	}
      break;
    default:
      return NULL;
    }

  return NULL;
}

int
write_ospfAreaEntry (int action, u_char *var_val,
		     u_char var_val_type, size_t var_val_len,
		     u_char *statP, oid *name, size_t length,
		     struct variable *v, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_area_table_index, index);
  long intval;
  int ret;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  if (!ospf_snmp_index_get (v, name, &length, &index.snmp,
			    OSPF_AREA_TABLE, OSPF_SNMP_INDEX_EXACT))
    return SNMP_ERR_NOSUCHNAME;

  pal_mem_cpy (&intval, var_val, 4);

  switch (v->magic)
    {
    case OSPFAUTHTYPE:		/* 2 -- ospfAuthtype. */
      if (!OSPF_SNMP_SET1 (auth_type, index.table.area_id, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFIMPORTASEXTERN:    /* 3 -- ospfImportAsExtern. */
      if (!OSPF_SNMP_SET1 (import_as_extern, index.table.area_id, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFAREASUMMARY:       /* 9 -- ospfAreaSummary. */
      if (!OSPF_SNMP_SET1 (area_summary, index.table.area_id, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFAREASTATUS:	/* 10 -- ospfAreaStatus. */
      if (!OSPF_SNMP_SET1 (area_status, index.table.area_id, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    default:
      break;
    }

  return SNMP_ERR_NOERROR;
}


/* ospfAreaTable ::= { ospf 2 }
   ospfAreaEntry
     INDEX { ospfAreaId }
     ::= { ospfAreaTable 1 } */
static u_char *
ospfAreaEntry (struct variable *v, oid *name, size_t *length, int exact,
	       size_t *var_len, WriteMethod **write_method, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_area_table_index, index);

  pal_mem_set (&index, 0, sizeof (struct ospf_area_table_index));

  /* Get table index. */
  if (!ospf_snmp_index_get (v, name, length, &index.snmp,
			    OSPF_AREA_TABLE, exact))
    return NULL;

  /* Return the current value of the variable */
  switch (v->magic)
    {
    case OSPFAREAID:		/* 1 -- ospfAreaId. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET_NEXT (area_id, index.table.area_id, index.table.len,
			      &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_AREA_TABLE);
      break;
    case OSPFAUTHTYPE:		/* 2 -- ospfAuthtype. */
      *write_method = write_ospfAreaEntry;
      if (OSPF_SNMP_GET_NEXT (auth_type, index.table.area_id, index.table.len,
			      &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_AREA_TABLE);
      break;
    case OSPFIMPORTASEXTERN:	/* 3 -- ospfImportAsExtern. */
      *write_method = write_ospfAreaEntry;
      if (OSPF_SNMP_GET_NEXT (import_as_extern, index.table.area_id, index.table.len,
			      &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_AREA_TABLE);
      break;
    case OSPFSPFRUNS:		/* 4 -- ospfSpfRuns. */
    /* This Object is read-only. */
      if (OSPF_SNMP_GET_NEXT (spf_runs, index.table.area_id, index.table.len,
			      &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_AREA_TABLE);
      break;
    case OSPFAREABDRRTRCOUNT:	/* 5 -- ospfAreaBdrRtrCount. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET_NEXT (area_bdr_rtr_count, index.table.area_id, index.table.len,
			      &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_AREA_TABLE);
      break;
    case OSPFASBDRRTRCOUNT:	/* 6 -- ospfAsBdrRtrCount. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET_NEXT (asbdr_rtr_count, index.table.area_id, index.table.len,
			      &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_AREA_TABLE);
      break;
    case OSPFAREALSACOUNT:	/* 7 -- ospfAreaLsaCount. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET_NEXT (area_lsa_count, index.table.area_id, index.table.len,
			      &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_AREA_TABLE);
      break;
    case OSPFAREALSACKSUMSUM:	/* 8 -- ospfAreaLsaCksumSum. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET_NEXT (area_lsa_cksum_sum, index.table.area_id, index.table.len,
			      &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_AREA_TABLE);
      break;
    case OSPFAREASUMMARY:	/* 9 -- ospfAreaSummary. */
      *write_method = write_ospfAreaEntry;
      if (OSPF_SNMP_GET_NEXT (area_summary, index.table.area_id, index.table.len,
			      &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_AREA_TABLE);
      break;
    case OSPFAREASTATUS:	/* 10 -- ospfAreaStatus. */
      *write_method = write_ospfAreaEntry;
      if (OSPF_SNMP_GET_NEXT (area_status, index.table.area_id, index.table.len,
			      &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_AREA_TABLE);
      break;
    default:
      return NULL;
      break;
    }
  return NULL;
}

int
write_ospfStubAreaEntry (int action, u_char *var_val,
			 u_char var_val_type, size_t var_val_len,
			 u_char *statP, oid *name, size_t length,
			 struct variable *v, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_stub_area_table_index, index);
  long intval;
  int ret;

  pal_mem_set (&index, 0, sizeof (struct ospf_stub_area_table_index));

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  if (!ospf_proc_lookup_any (proc_id, vr_id))
    return SNMP_ERR_GENERR;

  if (!ospf_snmp_index_get (v, name, &length, &index.snmp,
			    OSPF_STUB_AREA_TABLE, OSPF_SNMP_INDEX_EXACT))
    return SNMP_ERR_NOSUCHNAME;

  pal_mem_cpy (&intval, var_val, 4);

  switch (v->magic)
    {
    case OSPFSTUBMETRIC:
      if (!OSPF_SNMP_SET2 (stub_metric, index.table.area_id, index.table.tos, intval,
                           vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFSTUBSTATUS:
      if (!OSPF_SNMP_SET2 (stub_status, index.table.area_id, index.table.tos, intval,
                           vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFSTUBMETRICTYPE:
      if (!OSPF_SNMP_SET2 (stub_metric_type, index.table.area_id, index.table.tos, intval,
                           vr_id)) 
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    default:
      break;
    }
  return SNMP_ERR_NOERROR;
}

/* ospfStubAreaTable ::= { ospf 3 },
   ospfStubAreaEntry
     INDEX { ospfStubAreaId, ospfStubTOS }
     ::= { ospfStubAreaTable 1 } */
static u_char *
ospfStubAreaEntry (struct variable *v, oid *name, size_t *length,
		   int exact, size_t *var_len, WriteMethod **write_method,
                   u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_stub_area_table_index, index);

  pal_mem_set (&index, 0, sizeof (struct ospf_stub_area_table_index));

  /* Get table index. */
  if (!ospf_snmp_index_get (v, name, length, &index.snmp,
			    OSPF_STUB_AREA_TABLE, exact))
    return NULL;

  /* Return the current value of the variable */
  switch (v->magic)
    {
    case OSPFSTUBAREAID:	/* 1 -- ospfStubAreaId. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (stub_area_id, index.table.area_id, index.table.tos,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_STUB_AREA_TABLE);
      break;
    case OSPFSTUBTOS:		/* 2 -- ospfStubTOS. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (stub_tos, index.table.area_id, index.table.tos,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_STUB_AREA_TABLE);
      break;
    case OSPFSTUBMETRIC:	/* 3 -- ospfStubMetric. */
      *write_method = write_ospfStubAreaEntry;
      if (OSPF_SNMP_GET3NEXT (stub_metric, index.table.area_id, index.table.tos,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_STUB_AREA_TABLE);
      break;
    case OSPFSTUBSTATUS:	/* 4 -- ospfStubStatus. */
      *write_method = write_ospfStubAreaEntry;
      if (OSPF_SNMP_GET3NEXT (stub_status, index.table.area_id, index.table.tos,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_STUB_AREA_TABLE);
      break;
    case OSPFSTUBMETRICTYPE:	/* 5 -- ospfStubMetricType. */
      *write_method = write_ospfStubAreaEntry;
      if (OSPF_SNMP_GET3NEXT (stub_metric_type, index.table.area_id, index.table.tos,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_STUB_AREA_TABLE);
      break;
    default:
      return NULL;
      break;
    }
  return NULL;
}

/* ospfLsdbTable ::= { ospf 4 }
   ospfLsdbEntry
     INDEX { ospfLsdbAreaId, ospfLsdbType, ospfLsdbLsid, ospfLsdbRouterId }
     ::= { ospLsdbTable 1 } */
static u_char *
ospfLsdbEntry (struct variable *v, oid *name, size_t *length, int exact,
	       size_t *var_len, WriteMethod **write_method, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_lsdb_table_index, index);

  pal_mem_set (&index, 0, sizeof (struct ospf_lsdb_table_index));

  /* Check OSPF instance. */
  if (!(ospf_proc_lookup_any (proc_id, vr_id)))
    return NULL;

  /* Get table index. */
  if (!ospf_snmp_index_get (v, name, length, &index.snmp,
			    OSPF_LSDB_TABLE, exact))
    return NULL;

  /* Return the current value of the variable */
  switch (v->magic)
    {
    case OSPFLSDBAREAID:               /* 1 -- ospfLsdbAreaId. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET5NEXT (lsdb_area_id, index.table.area_id, index.table.type,
			      index.table.ls_id, index.table.adv_router,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_LSDB_TABLE);
      break;
    case OSPFLSDBTYPE:                 /* 2 -- ospfLsdbType. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET5NEXT (lsdb_type, index.table.area_id, index.table.type,
			      index.table.ls_id, index.table.adv_router,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_LSDB_TABLE);
      break;
    case OSPFLSDBLSID:                 /* 3 -- ospfLsdbLsid. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET5NEXT (lsdb_lsid, index.table.area_id, index.table.type,
			      index.table.ls_id, index.table.adv_router,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_LSDB_TABLE);
      break;
    case OSPFLSDBROUTERID:             /* 4 -- ospfLsdbRouterId. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET5NEXT (lsdb_router_id, index.table.area_id, index.table.type,
			      index.table.ls_id, index.table.adv_router,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_LSDB_TABLE);
      break;
    case OSPFLSDBSEQUENCE:             /* 5 -- ospfLsdbSequence. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET5NEXT (lsdb_sequence, index.table.area_id, index.table.type,
			      index.table.ls_id, index.table.adv_router,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_LSDB_TABLE);
      break;
    case OSPFLSDBAGE:                  /* 6 -- ospfLsdbAge. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET5NEXT (lsdb_age, index.table.area_id, index.table.type,
			      index.table.ls_id, index.table.adv_router,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_LSDB_TABLE);
      break;
    case OSPFLSDBCHECKSUM:             /* 7 -- ospfLsdbChecksum. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET5NEXT (lsdb_checksum, index.table.area_id, index.table.type,
			      index.table.ls_id, index.table.adv_router,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_LSDB_TABLE);
      break;
    case OSPFLSDBADVERTISEMENT:        /* 8 -- ospfLsdbAdvertisement. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET4CHAR_T (lsdb_advertisement, index.table.area_id, index.table.type,
			      index.table.ls_id, index.table.adv_router,
			      index.table.len, &ospf_char_ptr, vr_id))
	OSPF_SNMP_RETURN_CHAR_T (ospf_char_ptr, OSPF_LSDB_TABLE);
      break;
    default:
      return NULL;
      break;
    }
  return NULL;
}

int
write_ospfAreaRangeEntry (int action, u_char *var_val,
			  u_char var_val_type, size_t var_val_len,
			  u_char *statP, oid *name, size_t length,
			  struct variable *v, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_area_range_table_index, index);
  long intval;
  int ret;

  pal_mem_set (&index, 0, sizeof (struct ospf_area_range_table_index));

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  if (!ospf_proc_lookup_any (proc_id, vr_id))
    return SNMP_ERR_GENERR;

  if (!ospf_snmp_index_get (v, name, &length, &index.snmp,
			    OSPF_AREA_RANGE_TABLE, OSPF_SNMP_INDEX_EXACT))
    return SNMP_ERR_NOSUCHNAME;

  pal_mem_cpy (&intval, var_val, 4);

  switch (v->magic)
    {
    case OSPFAREARANGESTATUS:	/* 4 -- ospfAreaRangeStatus. */
      if (!OSPF_SNMP_SET2 (area_range_status,
			   index.table.area_id, index.table.net, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFAREARANGEEFFECT:	/* 5 -- ospfAreaRangeEffect. */
      if (!OSPF_SNMP_SET2 (area_range_effect,
			   index.table.area_id, index.table.net, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    }
  return SNMP_ERR_NOERROR;
}

int
write_ospfAreaRangeEntry_in_addr (int action, u_char *var_val,
				  u_char var_val_type, size_t var_val_len,
				  u_char *statP, oid *name, size_t length,
				  struct variable *v, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_area_range_table_index, index);
  struct pal_in4_addr inaddrval;
  int ret;

  pal_mem_set (&index, 0, sizeof (struct ospf_area_range_table_index));

  if (var_val_type != ASN_IPADDRESS)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (struct pal_in4_addr))
    return SNMP_ERR_WRONGLENGTH;

  if (!ospf_proc_lookup_any (proc_id, vr_id))
    return SNMP_ERR_GENERR;

  if (!ospf_snmp_index_get (v, name, &length, &index.snmp,
			    OSPF_AREA_RANGE_TABLE, OSPF_SNMP_INDEX_EXACT))
    return SNMP_ERR_NOSUCHNAME;

  pal_mem_cpy (&inaddrval.s_addr, var_val, 4);

  switch (v->magic)
    {
    case OSPFAREARANGEMASK:	/* 3 -- ospfAreaRangeMask. */
      if (!OSPF_SNMP_SET2 (area_range_mask,
			   index.table.area_id, index.table.net, inaddrval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    }
  return SNMP_ERR_NOERROR;
}

/* ospfAreaRangeTable ::= { ospf 5 }
   ospfAreaRangeEntry
     INDEX { ospfAreaRangeAreaId, ospfAreaRangeNet }
     ::= { ospfAreaRangeTable 1 } */
static u_char *
ospfAreaRangeEntry (struct variable *v, oid *name, size_t *length, int exact,
		    size_t *var_len, WriteMethod **write_method,
                    u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_area_range_table_index, index);

  pal_mem_set (&index, 0, sizeof (struct ospf_area_range_table_index));

  /* Check OSPF instance. */
  if (!(ospf_proc_lookup_any (proc_id, vr_id)))
    return NULL;

  if (!ospf_snmp_index_get (v, name, length, &index.snmp,
			    OSPF_AREA_RANGE_TABLE, exact))
    return NULL;

  /* Return the current value of the variable */
  switch (v->magic)
    {
    case OSPFAREARANGEAREAID:	/* 1 -- ospfAreaRangeAreaId. */
      /* This object is read-only. */
      if (OSPF_SNMP_GET3NEXT (area_range_area_id, index.table.area_id, index.table.net,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_AREA_RANGE_TABLE);
      break;
    case OSPFAREARANGENET:	/* 2 -- ospfAreaRangeNet. */
      /* This object is read-only. */
      if (OSPF_SNMP_GET3NEXT (area_range_net, index.table.area_id, index.table.net,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_AREA_RANGE_TABLE);
      break;
    case OSPFAREARANGEMASK:	/* 3 -- ospfAreaRangeMask. */
      *write_method = write_ospfAreaRangeEntry_in_addr;
      if (OSPF_SNMP_GET3NEXT (area_range_mask, index.table.area_id, index.table.net,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_AREA_RANGE_TABLE);
      break;
    case OSPFAREARANGESTATUS:	/* 4 -- ospfAreaRangeStatus. */
      *write_method = write_ospfAreaRangeEntry;
      if (OSPF_SNMP_GET3NEXT (area_range_status, index.table.area_id, index.table.net,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_AREA_RANGE_TABLE);
      break;
    case OSPFAREARANGEEFFECT:	/* 5 -- ospfAreaRangeEffect. */
      *write_method = write_ospfAreaRangeEntry;
      if (OSPF_SNMP_GET3NEXT (area_range_effect, index.table.area_id, index.table.net,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_AREA_RANGE_TABLE);
      break;
    default:
      return NULL;
      break;
    }
  return NULL;
}


int
write_ospfHostEntry (int action, u_char *var_val,
		     u_char var_val_type, size_t var_val_len,
		     u_char *statP, oid *name, size_t length,
		     struct variable *v, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_host_table_index, index);
  long intval;
  int ret;

  pal_mem_set (&index, 0, sizeof (struct ospf_host_table_index));

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (!ospf_proc_lookup_any (proc_id, vr_id))
    return SNMP_ERR_GENERR;

  if (!ospf_snmp_index_get (v, name, &length, &index.snmp,
			    OSPF_HOST_TABLE, OSPF_SNMP_INDEX_EXACT))
    return SNMP_ERR_NOSUCHNAME;

  pal_mem_cpy (&intval, var_val, 4);

  switch (v->magic)
    {
    case OSPFHOSTMETRIC:	/* 3 -- ospfHostMetric. */
	if (!OSPF_SNMP_SET2 (host_metric, index.table.addr, index.table.tos, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFHOSTSTATUS:	/* 4 -- ospfHostStatus. */
      if (!OSPF_SNMP_SET2 (host_status, index.table.addr, index.table.tos, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    default:
      break;
    }
  return SNMP_ERR_NOERROR;
}

/* ospfHostTable ::= { ospf 6 }
   ospfHostEntry
     INDEX { ospfHostIpAddress, ospfHostTOS }
     ::= { ospfHostTable 1 } */
static u_char *
ospfHostEntry (struct variable *v, oid *name, size_t *length, int exact,
	       size_t *var_len, WriteMethod **write_method, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_host_table_index, index);

  pal_mem_set (&index, 0, sizeof (struct ospf_host_table_index));

  /* Check OSPF instance. */
  if (!(ospf_proc_lookup_any (proc_id, vr_id)))
    return NULL;

  /* Get table indices. */
  if (!ospf_snmp_index_get (v, name, length, &index.snmp,
			    OSPF_HOST_TABLE, exact))		
    return NULL;

  /* Return the current value of the variable. */
  switch (v->magic)
    {
    case OSPFHOSTIPADDRESS:	/* 1 -- ospfHostIpAddress. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (host_ip_address, index.table.addr, index.table.tos,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_HOST_TABLE);
      break;
    case OSPFHOSTTOS:		/* 2 -- ospfHostTOS. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (host_tos, index.table.addr, index.table.tos,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_HOST_TABLE);
      break;
    case OSPFHOSTMETRIC:	/* 3 -- ospfHostMetric. */
      *write_method = write_ospfHostEntry;
      if (OSPF_SNMP_GET3NEXT (host_metric, index.table.addr, index.table.tos,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_HOST_TABLE);
      break;
    case OSPFHOSTSTATUS:	/* 4 -- ospfHostStatus. */
      *write_method = write_ospfHostEntry;
      if (OSPF_SNMP_GET3NEXT (host_status, index.table.addr, index.table.tos,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_HOST_TABLE);
      break;
    case OSPFHOSTAREAID:	/* 5 -- ospfHostAreaId. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (host_area_id, index.table.addr, index.table.tos,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_HOST_TABLE);
      break;
    default:
      return NULL;
      break;
    }
  return NULL;
}


int
write_ospfIfEntry_in_addr (int action, u_char *var_val,
			   u_char var_val_type, size_t var_val_len,
			   u_char *statP, oid *name, size_t length,
			   struct variable *v, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  struct pal_in4_addr inaddrval;
  OSPF_SNMP_UNION(ospf_if_table_index, index);
  int ret;

  if (var_val_type != ASN_IPADDRESS)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (struct pal_in4_addr))
    return SNMP_ERR_WRONGLENGTH;

  if (!ospf_proc_lookup_any (proc_id, vr_id))
    return SNMP_ERR_GENERR;

  if (!ospf_snmp_index_get (v, name, &length, &index.snmp,
			    OSPF_IF_TABLE, OSPF_SNMP_INDEX_EXACT))
    return SNMP_ERR_NOSUCHNAME;

  pal_mem_cpy (&inaddrval.s_addr, var_val, 4);

  switch (v->magic)
    {
    case OSPFIFAREAID:		/* 3 -- ospfIfAreaId. */
      if (!OSPF_SNMP_SET2 (if_area_id, index.table.addr, index.table.ifindex, inaddrval,
                           vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    }
  return SNMP_ERR_NOERROR;
}

int
write_ospfIfEntry_char (int action, u_char *var_val,
			u_char var_val_type, size_t var_val_len,
			u_char *statP, oid *name, size_t length,
			struct variable *v, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_if_table_index, index);
  char authkey[OSPF_AUTH_SIMPLE_SIZE + 1];
  size_t val_len;
  int ret;
  
  if (var_val_type != ASN_OCTET_STR)
    return SNMP_ERR_WRONGTYPE;

  if (!ospf_proc_lookup_any (proc_id, vr_id))
    return SNMP_ERR_GENERR;

  if (!ospf_snmp_index_get (v, name, &length, &index.snmp,
			    OSPF_IF_TABLE, OSPF_SNMP_INDEX_EXACT))
    return SNMP_ERR_NOSUCHNAME;

  /* Reject non-printable character key. */
  if (!ospf_printable_str (var_val, var_val_len))
    return SNMP_ERR_BADVALUE; 

  switch (v->magic)
    {
    case OSPFIFAUTHKEY:		/* 16 -- ospfIfAuthKey. */
      pal_strcpy (authkey, "00000000");
      if (var_val != NULL)
        {
          val_len = pal_strlen (var_val);
          if (val_len > OSPF_AUTH_SIMPLE_SIZE)
            return SNMP_ERR_WRONGLENGTH;    
          pal_mem_cpy (authkey, var_val, val_len);
        }
      if (!OSPF_SNMP_SET3 (if_auth_key, index.table.addr,
			   index.table.ifindex, pal_strlen (authkey), authkey, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    default:
      return SNMP_ERR_GENERR;
    }
  return SNMP_ERR_NOERROR;
}

int
write_ospfIfEntry (int action, u_char *var_val,
		   u_char var_val_type, size_t var_val_len,
		   u_char *statP, oid *name, size_t length,
		   struct variable *v, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_if_table_index, index);
  long intval;
  int ret;

  pal_mem_set (&index, 0, sizeof (struct ospf_if_table_index));

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE; 

  if (!ospf_proc_lookup_any (proc_id, vr_id))
    return SNMP_ERR_GENERR;

  if (!ospf_snmp_index_get (v, name, &length, (struct ospf_snmp_index *)&index,
			    OSPF_IF_TABLE, OSPF_SNMP_INDEX_EXACT))
    return SNMP_ERR_NOSUCHNAME;

  pal_mem_cpy (&intval, var_val, 4);
 
  switch (v->magic)
    {
    case OSPFIFTYPE:		/* 4 -- ospfIfType. */
      if (!OSPF_SNMP_SET2 (if_type, index.table.addr, index.table.ifindex, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFIFADMINSTAT:	/* 5 -- ospfIfAdminStat. */
      if (!OSPF_SNMP_SET2 (if_admin_stat,
			   index.table.addr, index.table.ifindex, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFIFRTRPRIORITY:	/* 6 -- ospfIfRtrPriority. */
      if (!OSPF_SNMP_SET2 (if_rtr_priority,
			   index.table.addr, index.table.ifindex, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFIFTRANSITDELAY:	/* 7 -- ospfIfTransitDelay. */
      if (!OSPF_SNMP_SET2 (if_transit_delay,
			   index.table.addr, index.table.ifindex, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFIFRETRANSINTERVAL:	/* 8 -- ospfIfRetransInterval. */
      if (!OSPF_SNMP_SET2 (if_retrans_interval,
			   index.table.addr, index.table.ifindex, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFIFHELLOINTERVAL:	/* 9 -- ospfIfHelloInterval. */
      if (!OSPF_SNMP_SET2 (if_hello_interval,
			   index.table.addr, index.table.ifindex, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFIFRTRDEADINTERVAL:	/* 10 -- ospfIfRtrDeadInterval. */
      if (!OSPF_SNMP_SET2 (if_rtr_dead_interval,
			   index.table.addr, index.table.ifindex, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFIFPOLLINTERVAL:	/* 11 -- ospfIfPollInterval. */
      if (!OSPF_SNMP_SET2 (if_poll_interval,
			   index.table.addr, index.table.ifindex, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFIFSTATUS:		/* 17 -- ospfIfStatus. */
      if (!OSPF_SNMP_SET2 (if_status, index.table.addr, index.table.ifindex, intval, 
                           vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFIFMULTICASTFORWARDING: /* 18 -- ospfIfMulticastForwarding. */
      if (!OSPF_SNMP_SET2 (if_multicast_forwarding,
			   index.table.addr, index.table.ifindex, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFIFDEMAND:		/* 19 -- ospfIfDemand. */
      if (!OSPF_SNMP_SET2 (if_demand, index.table.addr, index.table.ifindex, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFIFAUTHTYPE:	/* 20 -- ospfIfAuthType. */
      if (!OSPF_SNMP_SET2 (if_auth_type, index.table.addr, index.table.ifindex, intval,
                           vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    }
  return SNMP_ERR_NOERROR;
}

/* ospfIfTable ::= { ospf 7 }
   ospfIfEntry
     INDEX { ospfIfIpAddress, ospfAddressLessIf }
     ::= { ospfIfTable 1 } */
static u_char *
ospfIfEntry (struct variable *v, oid *name, size_t *length, int exact,
	     size_t *var_len, WriteMethod **write_method, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_if_table_index, index);

  pal_mem_set (&index, 0, sizeof (struct ospf_if_table_index));

  /* Check OSPF instance. */
  if (!(ospf_proc_lookup_any (proc_id, vr_id)))
    return NULL;

  /* Get table indices. */
  if (!ospf_snmp_index_get (v, name, length, &index.snmp,
			    OSPF_IF_TABLE, exact))
    return NULL;

  /* Return the current value of the variable */
  switch (v->magic)
    {
    case OSPFIFIPADDRESS:	/* 1 -- ospfIfIpAddress. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (if_ip_address, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_IF_TABLE);
      break;
    case OSPFADDRESSLESSIF:	/* 2 -- ospfAddressLessIf. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (address_less_if, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_TABLE);
      break;
    case OSPFIFAREAID:		/* 3 -- ospfIfAreaId. */
      *write_method = write_ospfIfEntry_in_addr;
      if (OSPF_SNMP_GET3NEXT (if_area_id, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_IF_TABLE);
      break;
    case OSPFIFTYPE:		/* 4 -- ospfIfType. */
      *write_method = write_ospfIfEntry;
      if (OSPF_SNMP_GET3NEXT (if_type, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_TABLE);
      break;
    case OSPFIFADMINSTAT:	/* 5 -- ospfIfAdminStat. */
      *write_method = write_ospfIfEntry;
      if (OSPF_SNMP_GET3NEXT (if_admin_stat, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_TABLE);
      break;
    case OSPFIFRTRPRIORITY:	/* 6 -- ospfIfRtrPriority. */
      *write_method = write_ospfIfEntry;
      if (OSPF_SNMP_GET3NEXT (if_rtr_priority, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_TABLE);
      break;
    case OSPFIFTRANSITDELAY:	/* 7 -- ospfIfTransitDelay. */
      *write_method = write_ospfIfEntry;
      if (OSPF_SNMP_GET3NEXT (if_transit_delay, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_TABLE);
      break;
    case OSPFIFRETRANSINTERVAL:	/* 8 -- ospfIfRetransInterval. */
      *write_method = write_ospfIfEntry;
      if (OSPF_SNMP_GET3NEXT (if_retrans_interval, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_TABLE);
      break;
    case OSPFIFHELLOINTERVAL:	/* 9 -- ospfIfHelloInterval. */
      *write_method = write_ospfIfEntry;
      if (OSPF_SNMP_GET3NEXT (if_hello_interval, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_TABLE);
      break;
    case OSPFIFRTRDEADINTERVAL:	/* 10 -- ospfIfRtrDeadInterval. */
      *write_method = write_ospfIfEntry;
      if (OSPF_SNMP_GET3NEXT (if_rtr_dead_interval, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_TABLE);
      break;
    case OSPFIFPOLLINTERVAL:	/* 11 -- ospfIfPollInterval. */
      *write_method = write_ospfIfEntry;
      if (OSPF_SNMP_GET3NEXT (if_poll_interval, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_TABLE);
      break;
    case OSPFIFSTATE:		/* 12 -- ospfIfState. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (if_state, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_TABLE);
      break;
    case OSPFIFDESIGNATEDROUTER: /* 13 -- ospfIfDesignatedRouter. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (if_designated_router, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_IF_TABLE);
      break;
    case OSPFIFBACKUPDESIGNATEDROUTER: /* 14 -- ospfIfBackupDesignatedRouter.*/
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (if_backup_designated_router, index.table.addr,
			      index.table.ifindex, index.table.len,
			      &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_IF_TABLE);
      break;
    case OSPFIFEVENTS:		/* 15 -- ospfIfEvents. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (if_events, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_TABLE);
      break;
    case OSPFIFAUTHKEY:		/* 16 -- ospfIfAuthKey. */
      *write_method = write_ospfIfEntry_char;
      if (OSPF_SNMP_GET3NEXT (if_auth_key, index.table.addr,
                              index.table.ifindex, index.table.len,
                              (char **)((char *) &ospf_char_ptr), vr_id))
	OSPF_SNMP_RETURN_SIZE0 (ospf_char_ptr, OSPF_IF_TABLE);
      break;
    case OSPFIFSTATUS:		/* 17 -- ospfIfStatus. */
      *write_method = write_ospfIfEntry;
      if (OSPF_SNMP_GET3NEXT (if_status, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_TABLE);
      break;
    case OSPFIFMULTICASTFORWARDING: /* 18 -- ospfIfMulticastForwardinpg. */
      *write_method = write_ospfIfEntry;
      if (OSPF_SNMP_GET3NEXT (if_multicast_forwarding, index.table.addr,
			      index.table.ifindex, index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_TABLE);
      break;
    case OSPFIFDEMAND:		/* 19 -- ospfIfDemand. */
      *write_method = write_ospfIfEntry;
      if (OSPF_SNMP_GET3NEXT (if_demand, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_TABLE);
      break;
    case OSPFIFAUTHTYPE:	/* 20 -- ospfIfAuthType. */
      *write_method = write_ospfIfEntry;
      if (OSPF_SNMP_GET3NEXT (if_auth_type, index.table.addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_TABLE);
      break;
    default:
      return NULL;
      break;
    }
  return NULL;
}


int
write_ospfIfMetricEntry (int action, u_char *var_val,
			 u_char var_val_type, size_t var_val_len,
			 u_char *statP, oid *name, size_t length,
			 struct variable *v, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_if_metric_table_index, index);
  long intval;
  int ret;

  pal_mem_set (&index, 0, sizeof (struct ospf_if_metric_table_index));

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  if (!ospf_proc_lookup_any (proc_id, vr_id))
    return SNMP_ERR_GENERR;

  if (!ospf_snmp_index_get (v, name, &length, &index.snmp,
			    OSPF_IF_METRIC_TABLE, OSPF_SNMP_INDEX_EXACT))
    return SNMP_ERR_NOSUCHNAME;

  pal_mem_cpy (&intval, var_val, 4);

  switch (v->magic)
    {
    case OSPFIFMETRICVALUE:		/* 4 -- ospfIfMetricValue. */
      if (!OSPF_SNMP_SET3 (if_metric_value, index.table.addr,
			   index.table.ifindex, index.table.tos, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFIFMETRICSTATUS:		/* 5 -- ospfIfMetricStatus. */
      if (!OSPF_SNMP_SET3 (if_metric_status, index.table.addr,
			   index.table.ifindex, index.table.tos, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    default:
      break;
    }
  return SNMP_ERR_NOERROR;
}

/* ospfIfMetricTable ::= { ospf 8 }
   ospfIfMetricEntry
     INDEX { ospfIfMetricIpAddress, ospfIfMetricAddressLessIf,
             ospfIfMetricTOS }
     ::= { ospfIfMetricTable 1 } */
static u_char *
ospfIfMetricEntry (struct variable *v, oid *name, size_t *length, int exact,
		   size_t *var_len, WriteMethod **write_method, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_if_metric_table_index, index);

  pal_mem_set (&index, 0, sizeof (struct ospf_if_metric_table_index));

  /* Check OSPF instance. */
  if (!(ospf_proc_lookup_any (proc_id, vr_id)))
    return NULL;

  if (!ospf_snmp_index_get (v, name, length, &index.snmp,
			    OSPF_IF_METRIC_TABLE, exact))
    return NULL;

  /* Return the current value of the variable */
  switch (v->magic)
    {
    case OSPFIFMETRICIPADDRESS:		/* 1 -- ospfIfMetricIpAddress. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET4NEXT (if_metric_ip_address, index.table.addr, index.table.ifindex,
			      index.table.tos, index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_IF_METRIC_TABLE);
      break;
    case OSPFIFMETRICADDRESSLESSIF:	/* 2 -- ospfIfMetricAddressLessIf. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET4NEXT (if_metric_address_less_if, index.table.addr,
			      index.table.ifindex, index.table.tos, index.table.len,
			      &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_METRIC_TABLE);
      break;
    case OSPFIFMETRICTOS:		/* 3 -- ospfIfMetricTOS. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET4NEXT (if_metric_tos, index.table.addr, index.table.ifindex,
			      index.table.tos, index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_METRIC_TABLE);
      break;
    case OSPFIFMETRICVALUE:		/* 4 -- ospfIfMetricValue. */
      *write_method = write_ospfIfMetricEntry;
      if (OSPF_SNMP_GET4NEXT (if_metric_value, index.table.addr, index.table.ifindex,
			      index.table.tos, index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_METRIC_TABLE);
      break;
    case OSPFIFMETRICSTATUS:		/* 5 -- ospfIfMetricStatus. */
      *write_method = write_ospfIfMetricEntry;
      if (OSPF_SNMP_GET4NEXT (if_metric_status, index.table.addr, index.table.ifindex,
			      index.table.tos, index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_IF_METRIC_TABLE);
      break;
    default:
      return NULL;
      break;
    }
  return NULL;
}


int
write_ospfVirtIfEntry (int action, u_char *var_val,
		       u_char var_val_type, size_t var_val_len,
		       u_char *statP, oid *name, size_t length,
		       struct variable *v, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_virt_if_table_index, index);
  long intval;
  int ret;

  pal_mem_set (&index, 0, sizeof (struct ospf_virt_if_table_index));

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  if (!ospf_proc_lookup_any (proc_id, vr_id))
    return SNMP_ERR_GENERR;

  if (!ospf_snmp_index_get (v, name, &length, &index.snmp,
			    OSPF_VIRT_IF_TABLE, OSPF_SNMP_INDEX_EXACT))
    return SNMP_ERR_NOSUCHNAME;

  pal_mem_cpy (&intval, var_val, 4);

  switch (v->magic)
    {
    case OSPFVIRTIFTRANSITDELAY:	/* 3 -- ospfVirtIfTransitDelay. */
      if (!OSPF_SNMP_SET2 (virt_if_transit_delay,
			   index.table.area_id, index.table.nbr_id, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFVIRTIFRETRANSINTERVAL:	/* 4 -- ospfVirtItRetransInterval. */
      if (!OSPF_SNMP_SET2 (virt_if_retrans_interval,
			   index.table.area_id, index.table.nbr_id, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFVIRTIFHELLOINTERVAL:	/* 5 -- ospfVirtIfHelloInterval. */
      if (!OSPF_SNMP_SET2 (virt_if_hello_interval,
			   index.table.area_id, index.table.nbr_id, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFVIRTIFRTRDEADINTERVAL:	/* 6 -- ospfVirtIfRtrDeadInterval. */
      if (!OSPF_SNMP_SET2 (virt_if_rtr_dead_interval,
			   index.table.area_id, index.table.nbr_id, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFVIRTIFSTATUS:		/* 10 -- ospfVirtIfStatus. */
      if (!OSPF_SNMP_SET2 (virt_if_status,
			   index.table.area_id, index.table.nbr_id, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFVIRTIFAUTHTYPE:		/* 11 -- ospfVirtIfAuthType. */
      if (!OSPF_SNMP_SET2 (virt_if_auth_type,
			   index.table.area_id, index.table.nbr_id, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    }
  return SNMP_ERR_NOERROR;
}

int
write_ospfVirtIfEntry_char (int action, u_char *var_val,
			    u_char var_val_type, size_t var_val_len,
			    u_char *statP, oid *name, size_t length,
			    struct variable *v, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  char authkey[OSPF_AUTH_SIMPLE_SIZE + 1] = "";
  OSPF_SNMP_UNION(ospf_virt_if_table_index, index);
  size_t val_len;
  int ret;

  if (var_val_type != ASN_OCTET_STR)
    return SNMP_ERR_WRONGTYPE;

  if (!ospf_proc_lookup_any (proc_id, vr_id))
    return SNMP_ERR_GENERR;

  if (!ospf_snmp_index_get (v, name, &length, (struct ospf_snmp_index *)&index,
			    OSPF_VIRT_IF_TABLE, OSPF_SNMP_INDEX_EXACT))
    return SNMP_ERR_NOSUCHNAME;

  switch (v->magic)
    {
    case OSPFVIRTIFAUTHKEY:		/* 9 -- ospfVirtIfAuthKey. */
      val_len = pal_strlen (var_val);

      if (val_len > OSPF_AUTH_SIMPLE_SIZE)
        return SNMP_ERR_WRONGLENGTH;

      pal_mem_cpy (authkey, var_val, val_len);
      authkey[val_len] = '\0';
      if (!OSPF_SNMP_SET3 (virt_if_auth_key, index.table.area_id,
			   index.table.nbr_id, var_val_len, authkey, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    default:
      return SNMP_ERR_GENERR;
    }
  return SNMP_ERR_NOERROR;
}

/* ospfVirtIfTable ::= { ospf 9 }
   ospfVirtIfEntry
     INDEX { ospfVirtIfAreaId, ospfVirtIfNeighbor }
     ::= { ospfVirtIfTable 1 } */
static u_char *
ospfVirtIfEntry (struct variable *v, oid *name, size_t *length, int exact,
		 size_t  *var_len, WriteMethod **write_method, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_virt_if_table_index, index);

  pal_mem_set (&index, 0, sizeof (struct ospf_virt_if_table_index));

  if (!(ospf_proc_lookup_any (proc_id, vr_id)))
    return NULL;

  if (!ospf_snmp_index_get (v, name, length, &index.snmp,
			    OSPF_VIRT_IF_TABLE, exact))
    return NULL;

  /* Return the current value of the variable */
  switch (v->magic)
    {
    case OSPFVIRTIFAREAID:		/* 1 -- ospfVirtIfAreaId. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (virt_if_area_id, index.table.area_id, index.table.nbr_id,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_VIRT_IF_TABLE);
      break;
    case OSPFVIRTIFNEIGHBOR:		/* 2 -- ospfVirtIfNeighbor. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (virt_if_neighbor, index.table.area_id, index.table.nbr_id,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_VIRT_IF_TABLE);
      break;
    case OSPFVIRTIFTRANSITDELAY:	/* 3 -- ospfVirtIfTransitDelay. */
      *write_method = write_ospfVirtIfEntry;
      if (OSPF_SNMP_GET3NEXT (virt_if_transit_delay, index.table.area_id,
			      index.table.nbr_id, index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_VIRT_IF_TABLE);
      break;
    case OSPFVIRTIFRETRANSINTERVAL:	/* 4 -- ospfVirtIfRetransInterval. */
      *write_method = write_ospfVirtIfEntry;
      if (OSPF_SNMP_GET3NEXT (virt_if_retrans_interval, index.table.area_id,
			      index.table.nbr_id, index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_VIRT_IF_TABLE);
      break;
    case OSPFVIRTIFHELLOINTERVAL:	/* 5 -- ospfVirtIfHelloInterval. */
      *write_method = write_ospfVirtIfEntry;
      if (OSPF_SNMP_GET3NEXT (virt_if_hello_interval, index.table.area_id,
			      index.table.nbr_id, index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_VIRT_IF_TABLE);
      break;
    case OSPFVIRTIFRTRDEADINTERVAL:	/* 6 -- ospfVirtIfRtrDeadInterval. */
      *write_method = write_ospfVirtIfEntry;
      if (OSPF_SNMP_GET3NEXT (virt_if_rtr_dead_interval, index.table.area_id,
			      index.table.nbr_id, index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_VIRT_IF_TABLE);
      break;
    case OSPFVIRTIFSTATE:		/* 7 -- ospfVirtIfState. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (virt_if_state, index.table.area_id, index.table.nbr_id,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_VIRT_IF_TABLE);
      break;
    case OSPFVIRTIFEVENTS:		/* 8 -- ospfVirtIfEvents. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (virt_if_events, index.table.area_id, index.table.nbr_id,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_VIRT_IF_TABLE);
      break;
    case OSPFVIRTIFAUTHKEY:		/* 9 -- ospfVirtIfAuthKey. */
      *write_method = write_ospfVirtIfEntry_char;
      if (OSPF_SNMP_GET3NEXT (virt_if_auth_key, index.table.area_id,
                              index.table.nbr_id, index.table.len,
                              (char **)((char *) &ospf_char_ptr), vr_id))
	OSPF_SNMP_RETURN_SIZE0 (ospf_char_ptr, OSPF_VIRT_IF_TABLE);
      break;
    case OSPFVIRTIFSTATUS:		/* 10 -- ospfVirtIfStatus. */
      *write_method = write_ospfVirtIfEntry;
      if (OSPF_SNMP_GET3NEXT (virt_if_status, index.table.area_id, index.table.nbr_id,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_VIRT_IF_TABLE);
      break;
    case OSPFVIRTIFAUTHTYPE:		/* 11 -- ospfVirtIfAuthType. */
      *write_method = write_ospfVirtIfEntry;
      if (OSPF_SNMP_GET3NEXT (virt_if_auth_type, index.table.area_id, index.table.nbr_id,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_VIRT_IF_TABLE);
      break;
    default:
      return NULL;
      break;
    }
  return NULL;
}


int
write_ospfNbrEntry (int action, u_char *var_val,
		    u_char var_val_type, size_t var_val_len,
		    u_char *statP, oid *name, size_t length,
		    struct variable *v, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_nbr_table_index, index);
  long intval;
  int ret;

  pal_mem_set (&index, 0, sizeof (struct ospf_nbr_table_index));

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  if (!ospf_proc_lookup_any (proc_id, vr_id))
    return SNMP_ERR_GENERR;

  if (!ospf_snmp_index_get (v, name, &length, &index.snmp,
			    OSPF_NBR_TABLE, OSPF_SNMP_INDEX_EXACT))
    return SNMP_ERR_NOSUCHNAME;

  pal_mem_cpy (&intval, var_val, 4);

  switch (v->magic)
    {
    case OSPFNBRPRIORITY:		/* 5 -- ospfNbrPriority. */
      if (!OSPF_SNMP_SET2 (nbr_priority,
			   index.table.nbr_addr, index.table.ifindex, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFNBMANBRSTATUS:		/* 9 -- ospfNbmaNbrStatus. */
      if (!OSPF_SNMP_SET2 (nbma_nbr_status,
			   index.table.nbr_addr, index.table.ifindex, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    }
  return SNMP_ERR_NOERROR;
}

/* ospfNbrTable ::= { ospf 10 }
   ospfNbrEntry
     INDEX { ospfNbrIpAddr, ospfNbrAddressLessIndex }
     ::= { ospfNbrTable 1 } */
static u_char *
ospfNbrEntry (struct variable *v, oid *name, size_t *length, int exact,
	      size_t  *var_len, WriteMethod **write_method, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_nbr_table_index, index);

  pal_mem_set (&index, 0, sizeof (struct ospf_nbr_table_index));

  /* Check OSPF instance. */
  if (!(ospf_proc_lookup_any (proc_id, vr_id)))
    return NULL;

  /* Get table indices. */
  if (!ospf_snmp_index_get (v, name, length, &index.snmp,
			    OSPF_NBR_TABLE, exact))
    return NULL;

  /* Return the current value of the variable */
  switch (v->magic)
    {
    case OSPFNBRIPADDR:			/* 1 -- ospfNbrIpAddr. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (nbr_ip_addr, index.table.nbr_addr, index.table.ifindex,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_NBR_TABLE);
      break;
    case OSPFNBRADDRESSLESSINDEX:	/* 2 -- ospfNbrAddressLessIndex.Table. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (nbr_address_less_index, index.table.nbr_addr,
			      index.table.ifindex, index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_NBR_TABLE);
      break;
    case OSPFNBRRTRID:			/* 3 -- ospfNbrRtrId. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (nbr_rtr_id, index.table.nbr_addr, index.table.ifindex,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_NBR_TABLE);
      break;
    case OSPFNBROPTIONS:		/* 4 -- ospfNbrOptions. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (nbr_options, index.table.nbr_addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_NBR_TABLE);
      break;
    case OSPFNBRPRIORITY:		/* 5 -- ospfNbrPriority. */
      *write_method = write_ospfNbrEntry;
      if (OSPF_SNMP_GET3NEXT (nbr_priority, index.table.nbr_addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_NBR_TABLE);
      break;
    case OSPFNBRSTATE:			/* 6 -- ospfNbrState. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (nbr_state, index.table.nbr_addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_NBR_TABLE);
      break;
    case OSPFNBREVENTS:			/* 7 -- ospfNbrEvents. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (nbr_events, index.table.nbr_addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_NBR_TABLE);
      break;
    case OSPFNBRLSRETRANSQLEN:		/* 8 -- ospfNbrLsRetransQLen. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (nbr_ls_retrans_qlen, index.table.nbr_addr,
			      index.table.ifindex, index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_NBR_TABLE);
      break;
    case OSPFNBMANBRSTATUS:		/* 9 -- ospfNbmaNbrStatus. */
      *write_method = write_ospfNbrEntry;
      if (OSPF_SNMP_GET3NEXT (nbma_nbr_status, index.table.nbr_addr, index.table.ifindex,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_NBR_TABLE);
      break;
    case OSPFNBMANBRPERMANENCE:		/* 10 -- ospfNbmaPermanence. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (nbma_nbr_permanence, index.table.nbr_addr,
			      index.table.ifindex, index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_NBR_TABLE);
      break;
    case OSPFNBRHELLOSUPPRESSED:	/* 11 -- ospfNbrHelloSuppressed. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (nbr_hello_suppressed, index.table.nbr_addr,
			      index.table.ifindex, index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_NBR_TABLE);
      break;
    default:
      return NULL;
      break;
    }
  return NULL;
}


/* ospfVirtNbrTable ::= { ospf 11 }
   ospfVirtNbrEntry
     INDEX { ospfVirtNbrArea, ospfVirtNbrRtrId }
     ::= { ospfVirtNbrTable 1 } */
static u_char *
ospfVirtNbrEntry (struct variable *v, oid *name, size_t *length, int exact,
		  size_t  *var_len, WriteMethod **write_method, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_virt_nbr_table_index, index);

  pal_mem_set (&index, 0, sizeof (struct ospf_virt_nbr_table_index));

  /* Check OSPF instance. */
  if (!(ospf_proc_lookup_any (proc_id, vr_id)))
    return NULL;

  if (!ospf_snmp_index_get (v, name, length, &index.snmp,
			    OSPF_VIRT_NBR_TABLE, exact))
    return NULL;

  /* Return the current value of the variable */
  switch (v->magic)
    {
    case OSPFVIRTNBRAREA:		/* 1 -- ospfVirtNbrArea. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (virt_nbr_area, index.table.area_id, index.table.nbr_id,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_VIRT_NBR_TABLE);
      break;
    case OSPFVIRTNBRRTRID:		/* 2 -- ospfVirtNbrRtrId. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (virt_nbr_rtr_id, index.table.area_id, index.table.nbr_id,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_VIRT_NBR_TABLE);
      break;
    case OSPFVIRTNBRIPADDR:		/* 3 -- ospfVirtNbrIpAddr. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (virt_nbr_ip_addr, index.table.area_id, index.table.nbr_id,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_VIRT_NBR_TABLE);
      break;
    case OSPFVIRTNBROPTIONS:		/* 4 -- ospfVirtNbrOptions. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (virt_nbr_options, index.table.area_id, index.table.nbr_id,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_VIRT_NBR_TABLE);
      break;
    case OSPFVIRTNBRSTATE:		/* 5 -- ospfVirtNbrState. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (virt_nbr_state, index.table.area_id, index.table.nbr_id,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_VIRT_NBR_TABLE);
      break;
    case OSPFVIRTNBREVENTS:		/* 6 -- ospfVirtNbrEvents. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (virt_nbr_events, index.table.area_id, index.table.nbr_id,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_VIRT_NBR_TABLE);
      break;
    case OSPFVIRTNBRLSRETRANSQLEN:	/* 7 -- ospfVirtNbrLsRetransQLen. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (virt_nbr_ls_retrans_qlen, index.table.area_id,
			      index.table.nbr_id, index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_VIRT_NBR_TABLE);
      break;
    case OSPFVIRTNBRHELLOSUPPRESSED:	/* 8 -- ospfVirtnbrHelloSuppressed. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET3NEXT (virt_nbr_hello_suppressed, index.table.area_id,
			      index.table.nbr_id, index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_VIRT_NBR_TABLE);
      break;
    default:
      return NULL;
      break;
    }
  return NULL;
}


/* ospfExtLsdbTable ::= { ospf 12 }
   ospfExtLsdbEntry
     INDEX { ospfExtLsdbType, ospfExtLsdbLsid, ospfExtLsdbRouterId }
     ::= { ospfExtLsdbTable 1 } */
static u_char *
ospfExtLsdbEntry (struct variable *v, oid *name, size_t *length, int exact,
		  size_t *var_len, WriteMethod **write_method, 
                  u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_ext_lsdb_table_index, index);

  pal_mem_set (&index, 0, sizeof (struct ospf_ext_lsdb_table_index));

  /* Check OSPF instance. */
  if (!(ospf_proc_lookup_any (proc_id, vr_id)))
    return NULL;

  /* Get table index. */
  if (!ospf_snmp_index_get (v, name, length, &index.snmp,
			    OSPF_EXT_LSDB_TABLE, exact))
    return NULL;

  /* Return the current value of the variable */
  switch (v->magic)
    {
    case OSPFEXTLSDBTYPE:		/* 1 -- ospfExtLsdbType. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET4NEXT (ext_lsdb_type, index.table.type, index.table.ls_id,
			      index.table.adv_router, index.table.len, &ospf_int_val,
                              vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_EXT_LSDB_TABLE);
      break;
    case OSPFEXTLSDBLSID:		/* 2 -- ospfExtLsdbLsid. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET4NEXT (ext_lsdb_lsid, index.table.type, index.table.ls_id,
			      index.table.adv_router, index.table.len,
			      &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_EXT_LSDB_TABLE);
      break;
    case OSPFEXTLSDBROUTERID:		/* 3 -- ospfExtLsdbRouterId. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET4NEXT (ext_lsdb_router_id, index.table.type, index.table.ls_id,
			      index.table.adv_router, index.table.len,
			      &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_EXT_LSDB_TABLE);
      break;
    case OSPFEXTLSDBSEQUENCE:		/* 4 -- ospfExtLsdbSequence. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET4NEXT (ext_lsdb_sequence, index.table.type, index.table.ls_id,
			      index.table.adv_router, index.table.len, &ospf_int_val,
                              vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_EXT_LSDB_TABLE);
      break;
    case OSPFEXTLSDBAGE:		/* 5 -- ospfExtLsdbAge. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET4NEXT (ext_lsdb_age, index.table.type, index.table.ls_id,
			      index.table.adv_router, index.table.len, &ospf_int_val,
                              vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_EXT_LSDB_TABLE);
      break;
    case OSPFEXTLSDBCHECKSUM:		/* 6 -- ospfExtLsdbCheckSum. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET4NEXT (ext_lsdb_checksum, index.table.type, index.table.ls_id,
			      index.table.adv_router, index.table.len, &ospf_int_val,
                              vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_EXT_LSDB_TABLE);
      break;
    case OSPFEXTLSDBADVERTISEMENT:      /* 7 -- ospfExtLsdbAdvertisement. */
      /* This Obeject is read-only. */
      if (OSPF_SNMP_GET3CHAR_T (ext_lsdb_advertisement, index.table.type, index.table.ls_id,
			        index.table.adv_router, index.table.len, &ospf_char_ptr,
                                vr_id))
	OSPF_SNMP_RETURN_CHAR_T (ospf_char_ptr, OSPF_EXT_LSDB_TABLE);
      break;
    default:
      return NULL;
      break;
    }
  return NULL;
}


int
write_ospfAreaAggregateEntry (int action, u_char *var_val,
			      u_char var_val_type, size_t var_val_len,
			      u_char *statP, oid *name, size_t length,
			      struct variable *v, u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_area_aggregate_table_index, index);
  long intval;
  int ret;

  pal_mem_set (&index, 0, sizeof (struct ospf_area_aggregate_table_index));

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  if (!ospf_proc_lookup_any (proc_id, vr_id))
    return SNMP_ERR_GENERR;

  if (!ospf_snmp_index_get (v, name, &length, &index.snmp,
			    OSPF_AREA_AGGREGATE_TABLE, OSPF_SNMP_INDEX_EXACT))
    return SNMP_ERR_NOSUCHNAME;

  pal_mem_cpy (&intval, var_val, 4);

  switch (v->magic)
    {
    case OSPFAREAAGGREGATESTATUS:	/* 5 -- ospfAreaAggregateStatus. */
      if (!OSPF_SNMP_SET4 (area_aggregate_status, index.table.area_id,
			   index.table.type, index.table.net, index.table.mask, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    case OSPFAREAAGGREGATEEFFECT:	/* 6 -- ospfAreaAggregateEffect. */
      if (!OSPF_SNMP_SET4 (area_aggregate_effect, index.table.area_id,
			   index.table.type, index.table.net, index.table.mask, intval, vr_id))
	return OSPF_SNMP_RETURN_ERROR (ret);
      break;
    }
  return SNMP_ERR_NOERROR;
}

/* ospfAreaAggregateTable ::= { ospf 14 }
   ospfAreaAggregateEntry
     INDEX { ospfAreaAggregateAreaID, ospfAreaAggregateLsdbType,
             ospfAreaAggregateNet, ospfAreaAggregateMask }
     ::= { ospfAreaAggregateTable 1 } */
static u_char *
ospfAreaAggregateEntry (struct variable *v, oid *name, size_t *length,
			int exact, size_t *var_len, WriteMethod **write_method,
	                u_int32_t vr_id)
{
  int proc_id = OSPF_PROCESS_ID_ANY;
  OSPF_SNMP_UNION(ospf_area_aggregate_table_index, index);

  pal_mem_set (&index, 0, sizeof (struct ospf_area_aggregate_table_index));

  if (!(ospf_proc_lookup_any (proc_id, vr_id)))
    return NULL;

  /* Get table index. */
  if (!ospf_snmp_index_get (v, name, length, &index.snmp,
			    OSPF_AREA_AGGREGATE_TABLE, exact))
    return NULL;

  /* Return the current value of the variable */
  switch (v->magic)
    {
    case OSPFAREAAGGREGATEAREAID:	/* 1 -- ospfAreaAggregateAreaId. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET5NEXT (area_aggregate_area_id, index.table.area_id,
			      index.table.type, index.table.net, index.table.mask,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_AREA_AGGREGATE_TABLE);
      break;
    case OSPFAREAAGGREGATELSDBTYPE:	/* 2 -- ospfAreaAggregateLsdbType. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET5NEXT (area_aggregate_lsdb_type, index.table.area_id,
			      index.table.type, index.table.net, index.table.mask,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_AREA_AGGREGATE_TABLE);
      break;
    case OSPFAREAAGGREGATENET:		/* 3 -- ospfAreaAggregateNet. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET5NEXT (area_aggregate_net, index.table.area_id,
			      index.table.type, index.table.net, index.table.mask,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_AREA_AGGREGATE_TABLE);
      break;
    case OSPFAREAAGGREGATEMASK:		/* 4 -- ospfAreaAggregateMask. */
      /* This Object is read-only. */
      if (OSPF_SNMP_GET5NEXT (area_aggregate_mask, index.table.area_id,
			      index.table.type, index.table.net, index.table.mask,
			      index.table.len, &ospf_in_addr_val, vr_id))
	OSPF_SNMP_RETURN (ospf_in_addr_val, OSPF_AREA_AGGREGATE_TABLE);
      break;
    case OSPFAREAAGGREGATESTATUS:	/* 5 -- ospfAreaAggregateStatus. */
      *write_method = write_ospfAreaAggregateEntry;
      if (OSPF_SNMP_GET5NEXT (area_aggregate_status, index.table.area_id,
			      index.table.type, index.table.net, index.table.mask,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_AREA_AGGREGATE_TABLE);
      break;
    case OSPFAREAAGGREGATEEFFECT:	/* 6 -- ospfAreaAggregateEffect. */
      *write_method = write_ospfAreaAggregateEntry;
      if (OSPF_SNMP_GET5NEXT (area_aggregate_effect, index.table.area_id,
			      index.table.type, index.table.net, index.table.mask,
			      index.table.len, &ospf_int_val, vr_id))
	OSPF_SNMP_RETURN (ospf_int_val, OSPF_AREA_AGGREGATE_TABLE);
      break;
    default:
      return NULL;
      break;
    }
  return NULL;
}


/* OSPF Traps. */
/* ospfTrap ::= { ospf 16 } */
/* ospfTraps ::= { ospfTrap 2 } */
static oid ospf_trap_id[] = { OSPF2MIB, 16, 2 };

int
ospf_min_dead_interval (struct ospf *top)
{
  struct ospf_interface *oi;
  struct ls_node *rn;
  int interval = 65535;

  for (rn = ls_table_top (top->if_table); rn; rn = ls_route_next (rn))
    if ((oi = RN_INFO (rn, RNI_DEFAULT)))
      if (ospf_if_dead_interval_get (oi) < interval)
	interval = ospf_if_dead_interval_get (oi);

  return interval;
}

void
ospfVirtIfStateChange (struct ospf_vlink *vlink)
{
  struct ospf_master *om = vlink->oi->top->om;
  struct ospf *top = vlink->oi->top;
  struct trap_object2 obj[4];
  struct pal_in4_addr router_id, area_id, nbr_id;
  int proc_id = top->ospf_id;
  SNMP_TRAP_CALLBACK func;
  int state;
  int namelen;
  oid *ptr;
  int i;
  u_int32_t vr_id = 0;

  if (ospf_min_dead_interval (top) > ospf_proc_uptime (top))
    return;

  namelen = sizeof ospf_oid / sizeof (oid);

  /* Get ospfRouterID. */
  ospf_get_router_id (proc_id, &router_id, vr_id);
  ptr = obj[0].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 1, 1, 0);
  OID_SET_VAL (obj[0], ptr - obj[0].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &router_id);

  /* Get ospfVirtIfAreaId. */
  ospf_get_virt_if_area_id (proc_id, vlink->area_id, vlink->peer_id, &area_id,
                            vr_id);
  ptr = obj[1].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 9, 1, 1);
  OID_SET_IP_ADDR (ptr, &vlink->area_id);
  OID_SET_IP_ADDR (ptr, &vlink->peer_id);
  OID_SET_VAL (obj[1], ptr - obj[1].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &area_id);

  /* Get ospfVirtIfNeighbor. */
  ospf_get_virt_if_neighbor (proc_id, vlink->area_id, vlink->peer_id, &nbr_id,
                             vr_id);
  ptr = obj[2].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 9, 1, 2);
  OID_SET_IP_ADDR (ptr, &vlink->area_id);
  OID_SET_IP_ADDR (ptr, &vlink->peer_id);
  OID_SET_VAL (obj[2], ptr - obj[2].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &nbr_id);

  /* Get ospfVirtIfState. */
  ospf_get_virt_if_state (proc_id, vlink->area_id, vlink->peer_id, &state,
                          vr_id);
  ptr = obj[3].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 9, 1, 7);
  OID_SET_IP_ADDR (ptr, &vlink->area_id);
  OID_SET_IP_ADDR (ptr, &vlink->peer_id);
  OID_SET_VAL (obj[3], ptr - obj[3].name, INTEGER, sizeof (int), &state);

  /* Call registered trap callbacks.  */
  for (i = 0; i < vector_max (om->traps[OSPFVIRTIFSTATECHANGE - 1]); i++)
    if ((func = vector_slot (om->traps[OSPFVIRTIFSTATECHANGE - 1], i)))
      (*func) (ospf_trap_id, sizeof (ospf_trap_id) / sizeof (oid),
	       OSPFVIRTIFSTATECHANGE, ospf_proc_uptime (top),
	       (struct snmp_trap_object *)obj,
	       sizeof (obj) / sizeof (struct snmp_trap_object));
}

void
ospfNbrStateChange (struct ospf_neighbor *nbr)
{
  struct ospf_master *om = nbr->oi->top->om;
  struct ospf *top = nbr->oi->top;
  struct trap_object2 obj[5];
  struct pal_in4_addr router_id, nbr_addr, nbr_id, _nbr_addr;
  int ifindex, _ifindex, state;
  int proc_id = top->ospf_id;
  SNMP_TRAP_CALLBACK func;
  int namelen;
  oid *ptr;
  int i;
  u_int32_t vr_id = 0;

  if (ospf_min_dead_interval (top) > ospf_proc_uptime (top))
    return; 

  namelen = sizeof ospf_oid / sizeof (oid);

  _nbr_addr = nbr->ident.address->u.prefix4;
  _ifindex = ospf_address_less_ifindex (nbr->oi);

  /* Get ospfRouterId. */
  ospf_get_router_id (proc_id, &router_id, vr_id);
  ptr = obj[0].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 1, 1, 0);
  OID_SET_VAL (obj[0], ptr - obj[0].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &router_id);

  /* Get ospfNbrIpAddr. */
  ospf_get_nbr_ip_addr (proc_id, _nbr_addr, _ifindex, &nbr_addr, vr_id);
  ptr = obj[1].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 10, 1, 1);
  OID_SET_IP_ADDR (ptr, &_nbr_addr);
  OID_SET_ARG1 (ptr, _ifindex);
  OID_SET_VAL (obj[1], ptr - obj[1].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &nbr_addr);

  /* Get ospfNbrAddressLessIndex. */
  ospf_get_nbr_address_less_index (proc_id, _nbr_addr, _ifindex, &ifindex,
                                   vr_id);
  ptr = obj[2].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 10, 1, 2);
  OID_SET_IP_ADDR (ptr, &_nbr_addr);
  OID_SET_ARG1 (ptr, _ifindex);
  OID_SET_VAL (obj[2], ptr - obj[2].name, INTEGER,
	       sizeof (int), &ifindex);

  /* Get ospfNbrRtrId. */
  ospf_get_nbr_rtr_id (proc_id, _nbr_addr, _ifindex, &nbr_id, vr_id);
  ptr = obj[3].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 10, 1, 3);
  OID_SET_IP_ADDR (ptr, &_nbr_addr);
  OID_SET_ARG1 (ptr, _ifindex);
  OID_SET_VAL (obj[3], ptr - obj[3].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &nbr_id);

  /* Get ospfNbrState. */
  ospf_get_nbr_state (proc_id, _nbr_addr, _ifindex, &state, vr_id);
  ptr = obj[4].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 10, 1, 6);
  OID_SET_IP_ADDR (ptr, &_nbr_addr);
  OID_SET_ARG1 (ptr, _ifindex);
  OID_SET_VAL (obj[4], ptr - obj[4].name, INTEGER, sizeof (int), &state);

  /* Call registered trap callbacks.  */
  for (i = 0; i < vector_max (om->traps[OSPFNBRSTATECHANGE - 1]); i++)
    if ((func = vector_slot (om->traps[OSPFNBRSTATECHANGE - 1], i)))
      (*func) (ospf_trap_id, sizeof (ospf_trap_id) / sizeof (oid),
	       OSPFNBRSTATECHANGE, ospf_proc_uptime (top),
	       (struct snmp_trap_object *)obj,
	       sizeof (obj) / sizeof (struct snmp_trap_object));
}

void
ospfVirtNbrStateChange (struct ospf_neighbor *nbr)
{
  struct ospf_master *om = nbr->oi->top->om;
  struct ospf_vlink *vlink = nbr->oi->u.vlink;
  struct ospf *top = nbr->oi->top;
  struct trap_object2 obj[4];
  struct pal_in4_addr router_id, area_id, nbr_id;
  int proc_id = top->ospf_id;
  SNMP_TRAP_CALLBACK func;
  int state;
  int namelen;
  oid *ptr;
  int i;
  u_int32_t vr_id = 0;

  if (ospf_min_dead_interval (top) > ospf_proc_uptime (top))
    return;

  namelen = sizeof ospf_oid / sizeof (oid);

  /* Get ospfRouterId. */
  ospf_get_router_id (proc_id, &router_id, vr_id);
  ptr = obj[0].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 1, 1, 0);
  OID_SET_VAL (obj[0], ptr - obj[0].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &router_id);

  /* Get ospfVirtNbrArea. */
  ospf_get_virt_nbr_area (proc_id, vlink->area_id, vlink->peer_id, &area_id,
                          vr_id);
  ptr = obj[1].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 11, 1, 1);
  OID_SET_IP_ADDR (ptr, &vlink->area_id);
  OID_SET_IP_ADDR (ptr, &vlink->peer_id);
  OID_SET_VAL (obj[1], ptr - obj[1].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &area_id);

  /* Get ospfVirtNbrRtrId. */
  ospf_get_virt_nbr_rtr_id (proc_id, vlink->area_id, vlink->peer_id, &nbr_id,
                            vr_id);
  ptr = obj[2].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 11, 1, 2);
  OID_SET_IP_ADDR (ptr, &vlink->area_id);
  OID_SET_IP_ADDR (ptr, &vlink->peer_id);
  OID_SET_VAL (obj[2], ptr - obj[2].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &nbr_id);

  /* Get ospfVirtNbrState. */
  ospf_get_virt_nbr_state (proc_id, vlink->area_id, vlink->peer_id, &state,
                           vr_id);
  ptr = obj[3].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 11, 1, 5);
  OID_SET_IP_ADDR (ptr, &vlink->area_id);
  OID_SET_IP_ADDR (ptr, &vlink->peer_id);
  OID_SET_VAL (obj[3], ptr - obj[3].name, INTEGER, sizeof (int), &state);

  /* Call registered trap callbacks.  */
  for (i = 0; i < vector_max (om->traps[OSPFVIRTNBRSTATECHANGE - 1]); i++)
    if ((func = vector_slot (om->traps[OSPFVIRTNBRSTATECHANGE - 1], i)))
      (*func) (ospf_trap_id, sizeof (ospf_trap_id) / sizeof (oid),
	       OSPFVIRTNBRSTATECHANGE, ospf_proc_uptime (top),
	       (struct snmp_trap_object *)obj,
	       sizeof (obj) / sizeof (struct snmp_trap_object));
}

void
ospfIfConfigError (struct ospf_interface *oi,
		   struct pal_in4_addr src, int error, int type)
{
  struct ospf_master *om = oi->top->om;
  struct trap_object2 obj[6];
  struct pal_in4_addr router_id, ifaddr, _ifaddr;
  int ifindex, _ifindex;
  int proc_id = oi->top->ospf_id;
  SNMP_TRAP_CALLBACK func;
  int namelen;
  int trap_id;
  oid *ptr;
  int i;
  u_int32_t vr_id = 0;

  if (IS_OSPF_IPV4_UNNUMBERED (oi))
    {
      _ifaddr = IPv4AddressUnspec;
      _ifindex = oi->u.ifp->ifindex;
    }
  else
    {
      _ifaddr = oi->ident.address->u.prefix4;
      _ifindex = 0;
    }

  if (error == OSPF_AUTH_TYPE_MISMATCH
      || error == OSPF_AUTH_FAILURE)
    trap_id = OSPFIFAUTHFAILURE;
  else
    trap_id = OSPFIFCONFIGERROR;

  namelen = sizeof ospf_oid / sizeof (oid);

  /* Get ospfRouterId. */
  ospf_get_router_id (proc_id, &router_id, vr_id);
  ptr = obj[0].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 1, 1, 0);
  OID_SET_VAL (obj[0], ptr - obj[0].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &router_id);

  /* Get ospfIfIpAddress. */
  ospf_get_if_ip_address (proc_id, _ifaddr, _ifindex, &ifaddr, vr_id);
  ptr = obj[1].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 7, 1, 1);
  OID_SET_IP_ADDR (ptr, &_ifaddr);
  OID_SET_ARG1 (ptr, _ifindex);
  OID_SET_VAL (obj[1], ptr - obj[1].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &ifaddr);

  /* Get ospfAddressLessIf. */
  ospf_get_address_less_if (proc_id, _ifaddr, _ifindex, &ifindex, vr_id);
  ptr = obj[2].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 7, 1, 2);
  OID_SET_IP_ADDR (ptr, &_ifaddr);
  OID_SET_ARG1 (ptr, _ifindex);
  OID_SET_VAL (obj[2], ptr - obj[2].name, INTEGER, sizeof (int), &ifindex);

  /* Get ospfPacketSrc. */
  ptr = obj[3].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG4 (ptr, 16, 1, 4, 0);
  OID_SET_VAL (obj[3], ptr - obj[3].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &src);

  /* Get ospfConfigErrorType. */
  ptr = obj[4].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG4 (ptr, 16, 1, 2, 0);
  OID_SET_VAL (obj[4], ptr - obj[4].name, INTEGER, sizeof (int), &error);

  /* Get ospfPacketType. */
  ptr = obj[5].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG4 (ptr, 16, 1, 3, 0);
  OID_SET_VAL (obj[5], ptr - obj[5].name, INTEGER, sizeof (int), &type);

  /* Call registered trap callbacks.  */
  for (i = 0; i < vector_max (om->traps[trap_id - 1]); i++)
    if ((func = vector_slot (om->traps[trap_id - 1], i)))
      (*func) (ospf_trap_id, sizeof (ospf_trap_id) / sizeof (oid),
	       trap_id, ospf_proc_uptime (oi->top),
	       (struct snmp_trap_object *)obj,
	       sizeof (obj) / sizeof (struct snmp_trap_object));
}

void
ospfVirtIfConfigError (struct ospf_vlink *vlink, int error, int type)
{
  struct ospf_master *om = vlink->oi->top->om;
  struct ospf *top = vlink->oi->top;
  struct trap_object2 obj[5];
  struct pal_in4_addr router_id, area_id, nbr_id;
  int proc_id = top->ospf_id;
  SNMP_TRAP_CALLBACK func;
  int namelen;
  int trap_id;
  oid *ptr;
  int i;
  u_int32_t vr_id = 0;

  namelen = sizeof ospf_oid / sizeof (oid);

  if (error == OSPF_AUTH_TYPE_MISMATCH
      || error == OSPF_AUTH_FAILURE)
    trap_id = OSPFVIRTIFAUTHFAILURE;
  else
    trap_id = OSPFVIRTIFCONFIGERROR;

  /* Get ospfRouterId. */
  ospf_get_router_id (proc_id, &router_id, vr_id);
  ptr = obj[0].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 1, 1, 0);
  OID_SET_VAL (obj[0], ptr - obj[0].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &router_id);

  /* Get ospfVirtIfAreaId. */
  ospf_get_virt_if_area_id (proc_id, vlink->area_id, vlink->peer_id, &area_id,
                            vr_id);
  ptr = obj[1].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 9, 1, 1);
  OID_SET_IP_ADDR (ptr, &vlink->area_id);
  OID_SET_IP_ADDR (ptr, &vlink->peer_id);
  OID_SET_VAL (obj[1], ptr - obj[1].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &area_id);

  /* Get ospfVirtIfNeighbor. */
  ospf_get_virt_if_neighbor (proc_id, vlink->area_id, vlink->peer_id, &nbr_id,
                             vr_id);
  ptr = obj[2].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 9, 1, 2);
  OID_SET_IP_ADDR (ptr, &vlink->area_id);
  OID_SET_IP_ADDR (ptr, &vlink->peer_id);
  OID_SET_VAL (obj[2], ptr - obj[2].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &nbr_id);

  /* Get ospfConfigErrorType. */
  ptr = obj[3].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG4 (ptr, 16, 1, 2, 0);
  OID_SET_VAL (obj[3], ptr - obj[3].name, INTEGER, sizeof (int), &error);

  /* Get ospfPacketType. */
  ptr = obj[4].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG4 (ptr, 16, 1, 3, 0);
  OID_SET_VAL (obj[4], ptr - obj[4].name, INTEGER, sizeof (int), &type);

  /* Call registered trap callbacks.  */
  for (i = 0; i < vector_max (om->traps[trap_id - 1]); i++)
    if ((func = vector_slot (om->traps[trap_id - 1], i)))
      (*func) (ospf_trap_id, sizeof (ospf_trap_id) / sizeof (oid),
	       trap_id, ospf_proc_uptime (top),
	       (struct snmp_trap_object *)obj,
	       sizeof (obj) / sizeof (struct snmp_trap_object));
}

void
ospfIfRxBadPacket (struct ospf_interface *oi,
		   struct pal_in4_addr src, int type)
{
  struct ospf_master *om = oi->top->om;
  struct trap_object2 obj[5];
  struct pal_in4_addr router_id, ifaddr, _ifaddr;
  int ifindex, _ifindex;
  int proc_id = oi->top->ospf_id;
  SNMP_TRAP_CALLBACK func;
  int namelen;
  oid *ptr;
  int i;
  u_int32_t vr_id = 0;

  if (IS_OSPF_IPV4_UNNUMBERED (oi))
    {
      _ifaddr = IPv4AddressUnspec;
      _ifindex = oi->u.ifp->ifindex;
    }
  else
    {
      _ifaddr = oi->ident.address->u.prefix4;
      _ifindex = 0;
    }

  namelen = sizeof ospf_oid / sizeof (oid);

  /* Get ospfRouterId. */
  ospf_get_router_id (proc_id, &router_id, vr_id);
  ptr = obj[0].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 1, 1, 0);
  OID_SET_VAL (obj[0], ptr - obj[0].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &router_id);

  /* Get ospfIfIpAddress. */
  ospf_get_if_ip_address (proc_id, _ifaddr, _ifindex, &ifaddr, vr_id);
  ptr = obj[1].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 7, 1, 1);
  OID_SET_IP_ADDR (ptr, &_ifaddr);
  OID_SET_ARG1 (ptr, _ifindex);
  OID_SET_VAL (obj[1], ptr - obj[1].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &ifaddr);

  /* Get ospfAddressLessIf. */
  ospf_get_address_less_if (proc_id, _ifaddr, _ifindex, &ifindex, vr_id);
  ptr = obj[2].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 7, 1, 2);
  OID_SET_IP_ADDR (ptr, &_ifaddr);
  OID_SET_ARG1 (ptr, _ifindex);
  OID_SET_VAL (obj[2], ptr - obj[2].name, INTEGER, sizeof (int), &ifindex);

  /* Get ospfPacketSrc. */
  ptr = obj[3].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG4 (ptr, 16, 1, 4, 0);
  OID_SET_VAL (obj[3], ptr - obj[3].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &src);

  /* Get ospfPacketType. */
  ptr = obj[4].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG4 (ptr, 16, 1, 3, 0);
  OID_SET_VAL (obj[4], ptr - obj[4].name, INTEGER, sizeof (int), &type);

  /* Call registered trap callbacks.  */
  for (i = 0; i < vector_max (om->traps[OSPFIFRXBADPACKET - 1]); i++)
    if ((func = vector_slot (om->traps[OSPFIFRXBADPACKET - 1], i)))
      (*func) (ospf_trap_id, sizeof (ospf_trap_id) / sizeof (oid),
	       OSPFIFRXBADPACKET, ospf_proc_uptime (oi->top),
	       (struct snmp_trap_object *)obj,
	       sizeof (obj) / sizeof (struct snmp_trap_object));
}


void
ospfVirtIfRxBadPacket (struct ospf_vlink *vlink,
                       int type)
{
  struct ospf_master *om = vlink->oi->top->om;
  struct ospf *top = vlink->oi->top;
  struct trap_object2 obj[4];
  struct pal_in4_addr router_id, area_id, nbr_id;
  int proc_id = top->ospf_id;
  SNMP_TRAP_CALLBACK func;
  int namelen;
  oid *ptr;
  int i;
  u_int32_t vr_id = 0;

  namelen = sizeof ospf_oid / sizeof (oid);

  /* Get ospfRouterId. */
  ospf_get_router_id (proc_id, &router_id, vr_id);
  ptr = obj[0].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 1, 1, 0);
  OID_SET_VAL (obj[0], ptr - obj[0].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &router_id);

  /* Get ospfVirtIfAreaId. */
  ospf_get_virt_if_area_id (proc_id, vlink->area_id, vlink->peer_id, &area_id,
                            vr_id);
  ptr = obj[1].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 9, 1, 1);
  OID_SET_IP_ADDR (ptr, &vlink->area_id);
  OID_SET_IP_ADDR (ptr, &vlink->peer_id);
  OID_SET_VAL (obj[1], ptr - obj[1].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &area_id);

  /* Get ospfVirtIfNeighbor. */
  ospf_get_virt_if_neighbor (proc_id, vlink->area_id, vlink->peer_id, &nbr_id,
                             vr_id);
  ptr = obj[2].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 9, 1, 2);
  OID_SET_IP_ADDR (ptr, &vlink->area_id);
  OID_SET_IP_ADDR (ptr, &vlink->peer_id);
  OID_SET_VAL (obj[2], ptr - obj[2].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &nbr_id);

  /* Get ospfPacketType. */
  ptr = obj[3].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG4 (ptr, 16, 1, 3, 0);
  OID_SET_VAL (obj[3], ptr - obj[3].name, INTEGER, sizeof (int), &type);

  /* Call registered trap callbacks.  */
  for (i = 0; i < vector_max (om->traps[OSPFVIRTIFRXBADPACKET - 1]); i++)
    if ((func = vector_slot (om->traps[OSPFVIRTIFRXBADPACKET - 1], i)))
      (*func) (ospf_trap_id, sizeof (ospf_trap_id) / sizeof (oid),
	       OSPFVIRTIFRXBADPACKET, ospf_proc_uptime (top),
	       (struct snmp_trap_object *)obj,
	       sizeof (obj) / sizeof (struct snmp_trap_object));
}

void
ospfTxRetransmit (struct ospf_neighbor *nbr, struct ospf_area *area,
                  int type, int lsdb_type, struct pal_in4_addr  *ls_id,
                  struct pal_in4_addr *adv_router)
{
  struct ospf_master *om = nbr->oi->top->om;
  struct ospf_interface *oi = nbr->oi;
  struct ospf *top = oi->top;
  struct trap_object2 obj[8];
  struct pal_in4_addr router_id, ifaddr, _ifaddr, _nbr_addr, nbr_id, area_id;
  int ifindex, _ifindex;
  int proc_id = top->ospf_id;
  SNMP_TRAP_CALLBACK func;
  int namelen;
  oid *ptr;
  int i;
  u_int32_t vr_id = 0;

  if (ospf_min_dead_interval (top) > ospf_proc_uptime (top))
    return;

  _nbr_addr = nbr->ident.address->u.prefix4;

  if (IS_OSPF_IPV4_UNNUMBERED (oi))
    {
      _ifaddr = IPv4AddressUnspec;
      _ifindex = oi->u.ifp->ifindex;
    }
  else
    {
      _ifaddr = oi->ident.address->u.prefix4;
      _ifindex = 0;
    }

  if (area != NULL)
    area_id = area->area_id;
  else
    area_id = OSPFBackboneAreaID;

  namelen = sizeof ospf_oid / sizeof (oid);

  /* Get ospfRouterId. */
  ospf_get_router_id (proc_id, &router_id, vr_id);
  ptr = obj[0].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 1, 1, 0);
  OID_SET_VAL (obj[0], ptr - obj[0].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &router_id);

  /* Get ospfIfIpAddress. */
  ospf_get_if_ip_address (proc_id, _ifaddr, _ifindex, &ifaddr, vr_id);
  ptr = obj[1].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 7, 1, 1);
  OID_SET_IP_ADDR (ptr, &_ifaddr);
  OID_SET_ARG1 (ptr, _ifindex);
  OID_SET_VAL (obj[1], ptr - obj[1].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &ifaddr);

  /* Get ospfAddressLessIf. */
  ospf_get_address_less_if (proc_id, _ifaddr, _ifindex, &ifindex, vr_id);
  ptr = obj[2].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 7, 1, 2);
  OID_SET_IP_ADDR (ptr, &_ifaddr);
  OID_SET_ARG1 (ptr, _ifindex);
  OID_SET_VAL (obj[2], ptr - obj[2].name, INTEGER, sizeof (int), &ifindex);

  /* Get ospfNbrRtrId. */
  ospf_get_nbr_rtr_id (proc_id, _nbr_addr, _ifindex, &nbr_id, vr_id);
  ptr = obj[3].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 10, 1, 3);
  OID_SET_IP_ADDR (ptr, &_nbr_addr);
  OID_SET_ARG1 (ptr, _ifindex);
  OID_SET_VAL (obj[3], ptr - obj[3].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &nbr_id);

  /* Get ospfPacketType. */
  ptr = obj[4].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG4 (ptr, 16, 1, 3, 0);
  OID_SET_VAL (obj[4], ptr - obj[4].name, INTEGER, sizeof (int), &type);

  /* Get ospfLsdbType. */
  ptr = obj[5].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 4, 1, 2);
  OID_SET_IP_ADDR (ptr, &area_id);
  OID_SET_ARG1 (ptr, lsdb_type);
  OID_SET_IP_ADDR (ptr, ls_id);
  OID_SET_IP_ADDR (ptr, adv_router);
  OID_SET_VAL (obj[5], ptr - obj[5].name, INTEGER, sizeof (int), &lsdb_type);

  /* Get ospfLsdbLsid. */
  ptr = obj[6].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 4, 1, 3);
  OID_SET_IP_ADDR (ptr, &area_id);
  OID_SET_ARG1 (ptr, lsdb_type);
  OID_SET_IP_ADDR (ptr, ls_id);
  OID_SET_IP_ADDR (ptr, adv_router);
  OID_SET_VAL (obj[6], ptr - obj[6].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), ls_id);

  /* Get ospfLsdbRouterId. */
  ptr = obj[7].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 4, 1, 4);
  OID_SET_IP_ADDR (ptr, &area_id);
  OID_SET_ARG1 (ptr, lsdb_type);
  OID_SET_IP_ADDR (ptr, ls_id);
  OID_SET_IP_ADDR (ptr, adv_router);
  OID_SET_VAL (obj[7], ptr - obj[7].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), adv_router);

  /* Call registered trap callbacks.  */
  for (i = 0; i < vector_max (om->traps[OSPFTXRETRANSMIT - 1]); i++)
    if ((func = vector_slot (om->traps[OSPFTXRETRANSMIT - 1], i)))
      (*func) (ospf_trap_id, sizeof (ospf_trap_id) / sizeof (oid),
	       OSPFTXRETRANSMIT, ospf_proc_uptime (top),
	       (struct snmp_trap_object *)obj,
	       sizeof (obj) / sizeof (struct snmp_trap_object));
}

void
ospfVirtIfTxRetransmit (struct ospf_vlink *vlink,
                        int type, int lsdb_type, struct pal_in4_addr  *ls_id,
                        struct pal_in4_addr *adv_router)
{
  struct ospf_master *om = vlink->oi->top->om;
  struct ospf *top = vlink->oi->top;
  struct trap_object2 obj[7];
  struct pal_in4_addr router_id, area_id, nbr_id;
  int proc_id = top->ospf_id;
  SNMP_TRAP_CALLBACK func;
  int namelen;
  oid *ptr;
  int i;
  u_int32_t vr_id = 0;

  if (ospf_min_dead_interval (top) > ospf_proc_uptime (top))
    return;

  namelen = sizeof ospf_oid / sizeof (oid);

  /* Get ospfRouterId. */
  ospf_get_router_id (proc_id, &router_id, vr_id);
  ptr = obj[0].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 1, 1, 0);
  OID_SET_VAL (obj[0], ptr - obj[0].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &router_id);

  /* Get ospfVirtIfAreaId. */
  ospf_get_virt_if_area_id (proc_id, vlink->area_id, vlink->peer_id, &area_id,
                            vr_id);
  ptr = obj[1].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 9, 1, 1);
  OID_SET_IP_ADDR (ptr, &vlink->area_id);
  OID_SET_IP_ADDR (ptr, &vlink->peer_id);
  OID_SET_VAL (obj[1], ptr - obj[1].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &area_id);

  /* Get ospfVirtIfNeighbor. */
  ospf_get_virt_if_neighbor (proc_id, vlink->area_id, vlink->peer_id, &nbr_id,
                             vr_id);
  ptr = obj[2].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 9, 1, 2);
  OID_SET_IP_ADDR (ptr, &vlink->area_id);
  OID_SET_IP_ADDR (ptr, &vlink->peer_id);
  OID_SET_VAL (obj[2], ptr - obj[2].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &nbr_id);

  /* Get ospfPacketType. */
  ptr = obj[3].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG4 (ptr, 16, 1, 3, 0);
  OID_SET_VAL (obj[3], ptr - obj[3].name, INTEGER, sizeof (int), &type);

  /* Get ospfLsdbType. */
  ptr = obj[4].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 4, 1, 2);
  OID_SET_IP_ADDR (ptr, &OSPFBackboneAreaID);
  OID_SET_ARG1 (ptr, lsdb_type);
  OID_SET_IP_ADDR (ptr, ls_id);
  OID_SET_IP_ADDR (ptr, adv_router);
  OID_SET_VAL (obj[4], ptr - obj[4].name, INTEGER, sizeof (int), &lsdb_type);

  /* Get ospfLsdbLsid. */
  ptr = obj[5].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 4, 1, 3);
  OID_SET_IP_ADDR (ptr, &OSPFBackboneAreaID);
  OID_SET_ARG1 (ptr, lsdb_type);
  OID_SET_IP_ADDR (ptr, ls_id);
  OID_SET_IP_ADDR (ptr, adv_router);
  OID_SET_VAL (obj[5], ptr - obj[5].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), ls_id);

  /* Get ospfLsdbRouterId. */
  ptr = obj[6].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 4, 1, 4);
  OID_SET_IP_ADDR (ptr, &OSPFBackboneAreaID);
  OID_SET_ARG1 (ptr, lsdb_type);
  OID_SET_IP_ADDR (ptr, ls_id);
  OID_SET_IP_ADDR (ptr, adv_router);
  OID_SET_VAL (obj[6], ptr - obj[6].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), adv_router);

  /* Call registered trap callbacks.  */
  for (i = 0; i < vector_max (om->traps[OSPFVIRTIFTXRETRANSMIT - 1]); i++)
    if ((func = vector_slot (om->traps[OSPFVIRTIFTXRETRANSMIT - 1], i)))
      (*func) (ospf_trap_id, sizeof (ospf_trap_id) / sizeof (oid),
	       OSPFVIRTIFTXRETRANSMIT, ospf_proc_uptime (top),
	       (struct snmp_trap_object *)obj,
	       sizeof (obj) / sizeof (struct snmp_trap_object));
}

void
ospfOriginateLsa (struct ospf *top, struct ospf_lsa *lsa, int trap_id)
{
  struct ospf_master *om = top->om;
  struct trap_object2 obj[5];
  struct pal_in4_addr router_id, area_id;
  int proc_id = top->ospf_id;
  SNMP_TRAP_CALLBACK func;
  int namelen;
  int type;
  oid *ptr;
  int i;
  u_int32_t vr_id = 0;

  if (ospf_min_dead_interval (top) > ospf_proc_uptime (top))
    return;

  namelen = sizeof ospf_oid / sizeof (oid);

  switch (LSA_FLOOD_SCOPE (lsa->data->type))
    {
    case LSA_FLOOD_SCOPE_LINK:
#ifdef HAVE_OPAQUE_LSA
      area_id = lsa->lsdb->u.oi->area->area_id;
#endif /* HAVE_OPAQUE_LSA */
      break;
    case LSA_FLOOD_SCOPE_AREA:
      area_id = lsa->lsdb->u.area->area_id;
      break;
    case LSA_FLOOD_SCOPE_AS:
      area_id = OSPFBackboneAreaID;
      break;
    default:
      return;
    }

  type = lsa->data->type;

  /* Get ospfRouterId. */
  ospf_get_router_id (proc_id, &router_id, vr_id);
  ptr = obj[0].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 1, 1, 0);
  OID_SET_VAL (obj[0], ptr - obj[0].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &router_id);

  /* Get ospfLsdbAreaId. */
  ptr = obj[1].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 4, 1, 1);
  OID_SET_IP_ADDR (ptr, &area_id);
  OID_SET_ARG1 (ptr, type);
  OID_SET_IP_ADDR (ptr, &lsa->data->id);
  OID_SET_IP_ADDR (ptr, &lsa->data->adv_router);
  OID_SET_VAL (obj[1], ptr - obj[1].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &area_id);

  /* Get ospfLsdbType. */
  ptr = obj[2].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 4, 1, 2);
  OID_SET_IP_ADDR (ptr, &area_id);
  OID_SET_ARG1 (ptr, type);
  OID_SET_IP_ADDR (ptr, &lsa->data->id);
  OID_SET_IP_ADDR (ptr, &lsa->data->adv_router);
  OID_SET_VAL (obj[2], ptr - obj[2].name, INTEGER, sizeof (int), &type);

  /* Get ospfLsdbLsid. */
  ptr = obj[3].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 4, 1, 3);
  OID_SET_IP_ADDR (ptr, &area_id);
  OID_SET_ARG1 (ptr, type);
  OID_SET_IP_ADDR (ptr, &lsa->data->id);
  OID_SET_IP_ADDR (ptr, &lsa->data->adv_router);
  OID_SET_VAL (obj[3], ptr - obj[3].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &lsa->data->id);

  /* Get ospfLsdbRouterId. */
  ptr = obj[4].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 4, 1, 4);
  OID_SET_IP_ADDR (ptr, &area_id);
  OID_SET_ARG1 (ptr, type);
  OID_SET_IP_ADDR (ptr, &lsa->data->id);
  OID_SET_IP_ADDR (ptr, &lsa->data->adv_router);
  OID_SET_VAL (obj[4], ptr - obj[4].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &lsa->data->adv_router);

  /* Call registered trap callbacks.  */
  for (i = 0; i < vector_max (om->traps[trap_id - 1]); i++)
    if ((func = vector_slot (om->traps[trap_id - 1], i)))
      (*func) (ospf_trap_id, sizeof (ospf_trap_id) / sizeof (oid),
	       trap_id, ospf_proc_uptime (top),
	       (struct snmp_trap_object *)obj,
	       sizeof (obj) / sizeof (struct snmp_trap_object));
}

void
ospfLsdbOverflow (struct ospf *top, int trap_id)
{
  struct ospf_master *om = top->om;
  struct trap_object2 obj[2];
  struct pal_in4_addr router_id;
  int proc_id = top->ospf_id;
  SNMP_TRAP_CALLBACK func;
  int namelen, limit;
  oid *ptr;
  int i;
  u_int32_t vr_id = 0;

  namelen = sizeof ospf_oid / sizeof (oid);

  /* Get ospfRouterID. */
  ospf_get_router_id (proc_id, &router_id, vr_id);
  ptr = obj[0].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 1, 1, 0);
  OID_SET_VAL (obj[0], ptr - obj[0].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &router_id);

  /* Get ospfExtLsdbLimit. */
  ospf_get_ext_lsdb_limit (proc_id, &limit, vr_id);
  ptr = obj[1].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 1, 11, 0);
  OID_SET_VAL (obj[1], ptr - obj[1].name, INTEGER, sizeof (int), &limit);

  /* Call registered trap callbacks.  */
  for (i = 0; i < vector_max (om->traps[trap_id - 1]); i++)
    if ((func = vector_slot (om->traps[trap_id - 1], i)))
      (*func) (ospf_trap_id, sizeof (ospf_trap_id) / sizeof (oid),
	       trap_id, ospf_proc_uptime (top),
	       (struct snmp_trap_object *)obj,
	       sizeof (obj) / sizeof (struct snmp_trap_object));
}

void
ospfIfStateChange (struct ospf_interface *oi)
{
  struct ospf_master *om = oi->top->om;
  struct trap_object2 obj[4];
  struct pal_in4_addr router_id, ifaddr, _ifaddr;
  int ifindex, state, _ifindex;
  int proc_id = oi->top->ospf_id;
  SNMP_TRAP_CALLBACK func;
  int namelen;
  oid *ptr;
  int i;
  u_int32_t vr_id = 0;

  if (ospf_min_dead_interval (oi->top) > ospf_proc_uptime (oi->top))
    return; 

  if (IS_OSPF_IPV4_UNNUMBERED (oi))
    {
      _ifaddr = IPv4AddressUnspec;
      _ifindex = oi->u.ifp->ifindex;
    }
  else
    {
      _ifaddr = oi->ident.address->u.prefix4;
      _ifindex = 0;
    }

  namelen = sizeof ospf_oid / sizeof (oid);

  /* Get ospfRouterID. */
  ospf_get_router_id (proc_id, &router_id, vr_id);
  ptr = obj[0].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 1, 1, 0);
  OID_SET_VAL (obj[0], ptr - obj[0].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &router_id);

  /* Get ospfIfIpAddress. */
  ospf_get_if_ip_address (proc_id, _ifaddr, _ifindex, &ifaddr, vr_id);
  ptr = obj[1].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 7, 1, 1);
  OID_SET_IP_ADDR (ptr, &_ifaddr);
  OID_SET_ARG1 (ptr, _ifindex);
  OID_SET_VAL (obj[1], ptr - obj[1].name, IPADDRESS,
	       sizeof (struct pal_in4_addr), &ifaddr);

  /* Get ospfAddressLessIf. */
  ospf_get_address_less_if (proc_id, _ifaddr, _ifindex, &ifindex, vr_id);
  ptr = obj[2].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 7, 1, 2);
  OID_SET_IP_ADDR (ptr, &_ifaddr);
  OID_SET_ARG1 (ptr, _ifindex);
  OID_SET_VAL (obj[2], ptr - obj[2].name, INTEGER, sizeof (int), &ifindex);

  /* Get ospfIfState. */
  ospf_get_if_state (proc_id, _ifaddr, _ifindex, &state, vr_id);
  ptr = obj[3].name;
  OID_COPY (ptr, ospf_oid, namelen);
  OID_SET_ARG3 (ptr, 7, 1, 12);
  OID_SET_IP_ADDR (ptr, &_ifaddr);
  OID_SET_ARG1 (ptr, _ifindex);
  OID_SET_VAL (obj[3], ptr - obj[3].name, INTEGER, sizeof (int), &state);

  /* Call registered trap callbacks.  */
  for (i = 0; i < vector_max (om->traps[OSPFIFSTATECHANGE - 1]); i++)
    if ((func = vector_slot (om->traps[OSPFIFSTATECHANGE - 1], i)))
      (*func) (ospf_trap_id, sizeof (ospf_trap_id) / sizeof (oid),
	       OSPFIFSTATECHANGE, ospf_proc_uptime (oi->top),
	       (struct snmp_trap_object *)obj,
	       sizeof (obj) / sizeof (struct snmp_trap_object));
}


/* OSPF SMUX trap function */
void
ospf_snmp_trap (oid *trap_oid, size_t trap_oid_len,
		oid spec_trap_val, u_int32_t uptime,
		struct snmp_trap_object *obj, size_t obj_len)
{
  snmp_trap2 (ZG, trap_oid, trap_oid_len, spec_trap_val,
	      ospf_oid, sizeof (ospf_oid) / sizeof (oid),
	      (struct trap_object2 *) obj, obj_len, uptime);
}

/* Register OSPF2-MIB. */
void
ospf_snmp_init (struct lib_globals *zg)
{
  snmp_init (zg, ospfd_oid, sizeof (ospfd_oid) / sizeof (oid));
  REGISTER_MIB (zg, "mibII/ospf", ospf_variables, variable, ospf_oid);
  
  /* Register all OSPF traps. */
  ospf_trap_callback_set (OSPF_TRAP_ALL, ospf_snmp_trap);
  
  snmp_start (zg);
}

#ifdef HAVE_SNMP_RESTART
void
ospf_snmp_restart ()
{
  snmp_restart (ZG);
}
#endif /* HAVE_SNMP_RESTART */
#endif /* HAVE_SNMP */
