/* Copyright (C) 2005-2006  All Rights Reserved. */

#include "pal.h"

#ifdef HAVE_SNMP

#include <asn1.h>
#include "snmp.h"
#include "lib.h"
#include "lacp_snmp.h"
#include "l2_snmp.h"
#include "lacpd.h"
#include "lacp_selection_logic.h"
#include "lacp_link.h"
#include "pal_math.h"
#include "lacp_mux.h"

#ifdef HAVE_HA
#include "lacp_cal.h"
#endif /* HAVE_HA */

extern struct lib_globals *lacpm;
static int lacp_int_val;
u_char *lacp_string_val;
static u_long lacp_ulong_val;
static pal_time_t lacp_start_time;
unsigned char port_admin_state_bitstring = '0';

WriteMethod lacp_snmp_write_dot3adAggTable;
WriteMethod lacp_snmp_write_dot3adAggPortTable;
WriteMethod lacp_snmp_write_dot3adAggPortTable_States;

static oid lacpd_oid [] = { LACPDOID };
static oid lacp_dot3adMIBObjects_oid [] = { LACPDOID , 1 };

/* function prototypes */
FindVarMethod lacp_snmp_dot3adAggTable;
FindVarMethod lacp_snmp_dot3adAggPortTable;
FindVarMethod lacp_snmp_dot3adTablesLastChanged;
FindVarMethod lacp_snmp_dot3adAggPortListTable;
FindVarMethod lacp_snmp_dot3adAggPortStatsTable;  
FindVarMethod lacp_snmp_dot3adAggPortDebugTable;
void
lacp_snmp_get_port_list(struct lacp_aggregator *agg, char *port_list);

struct variable dot3adMIBObjects [] = {
  /*
   * magic number , variable type , ro/rw , callback fn  , L , oidsuffix
   * (L = length of the oidsuffix)
   */
  {DOT3ADAGGINDEX , ASN_INTEGER , RONLY , lacp_snmp_dot3adAggTable ,
   4, {1,1,1,1}},
  {DOT3ADAGGMACADDRESS , ASN_OCTET_STR , RONLY , lacp_snmp_dot3adAggTable ,
   4, {1,1,1,2}},
  {DOT3ADAGGACTORSYSTEMPRIORITY ,ASN_INTEGER,RWRITE , lacp_snmp_dot3adAggTable ,
   4, {1,1,1,3}},
  {DOT3ADAGGACTORSYSTEMID , ASN_OCTET_STR , RONLY , lacp_snmp_dot3adAggTable ,
   4, {1,1,1,4}},
  {DOT3ADAGGAGGREGATEORINDIVIDUAL ,ASN_INTEGER ,RONLY ,lacp_snmp_dot3adAggTable,
   4, {1,1,1,5}},
  {DOT3ADAGGACTORADMINKEY , ASN_INTEGER , RWRITE , lacp_snmp_dot3adAggTable ,
   4, {1,1,1,6}},
  {DOT3ADAGGACTOROPERKEY , ASN_INTEGER , RONLY , lacp_snmp_dot3adAggTable ,
   4, {1,1,1,7}},
  {DOT3ADAGGPARTNERSYSTEMID , ASN_OCTET_STR , RONLY , lacp_snmp_dot3adAggTable ,
   4, {1,1,1,8}},
  {DOT3ADAGGPARTNERSYSTEMPRIORITY,ASN_INTEGER ,RONLY ,lacp_snmp_dot3adAggTable,
   4, {1,1,1,9}},
  {DOT3ADAGGPARTNEROPERKEY , ASN_INTEGER , RONLY , lacp_snmp_dot3adAggTable ,
   4, {1,1,1,10}},
  {DOT3ADAGGCOLLECTORMAXDELAY , ASN_INTEGER ,RWRITE ,lacp_snmp_dot3adAggTable ,
   4, {1,1,1,11}},
  {DOT3ADAGGPORTLISTPORTS, ASN_OCTET_STR, RONLY,
  lacp_snmp_dot3adAggPortListTable, 4, {1, 2, 1, 1}},
  {DOT3ADAGGPORTINDEX , ASN_INTEGER , RONLY , lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,1}},
  {DOT3ADAGGPORTACTORSYSTEMPRIORITY,ASN_INTEGER,RWRITE,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,2}},
  {DOT3ADAGGPORTACTORSYSTEMID ,ASN_OCTET_STR,RONLY,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,3}},
  {DOT3ADAGGPORTACTORADMINKEY ,ASN_INTEGER,RWRITE,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,4}},
  {DOT3ADAGGPORTACTOROPERKEY , ASN_INTEGER ,RWRITE,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,5}},
  {DOT3ADAGGPORTPARTNERADMINSYSTEMPRIORITY,ASN_INTEGER,RWRITE,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,6}},
  {DOT3ADAGGPORTPARTNEROPERSYSTEMPRIORITY,ASN_INTEGER,RONLY,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,7}},
  {DOT3ADAGGPORTPARTNERADMINSYSTEMID,ASN_OCTET_STR,RWRITE,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,8}},
  {DOT3ADAGGPORTPARTNEROPERSYSTEMID,ASN_OCTET_STR,RONLY,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,9}},
  {DOT3ADAGGPORTPARTNERADMINKEY, ASN_INTEGER,RWRITE,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,10}},
  {DOT3ADAGGPORTPARTNEROPERKEY, ASN_INTEGER, RONLY,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,11}},
  {DOT3ADAGGPORTSELECTEDAGGID, ASN_INTEGER, RONLY,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,12}},
  {DOT3ADAGGPORTATTACHEDAGGID,ASN_INTEGER, RONLY,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,13}}, 
  {DOT3ADAGGPORTACTORPORT, ASN_INTEGER,RONLY,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,14}},
  {DOT3ADAGGPORTACTORPORTPRIORITY,ASN_INTEGER,RWRITE,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,15}},
  {DOT3ADAGGPORTPARTNERADMINPORT,ASN_INTEGER,RWRITE,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,16}},
  {DOT3ADAGGPORTPARTNEROPERPORT,ASN_INTEGER,RONLY,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,17}},
  {DOT3ADAGGPORTPARTNERADMINPORTPRIORITY,ASN_INTEGER,RWRITE,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,18}},
  {DOT3ADAGGPORTPARTNEROPERPORTPRIORITY,ASN_INTEGER,RONLY,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,19}},
  {DOT3ADAGGPORTACTORADMINSTATE,ASN_BITS,RWRITE,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,20}},
  {DOT3ADAGGPORTACTOROPERSTATE,ASN_BITS,RONLY,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,21}},
  {DOT3ADAGGPORTPARTNERADMINSTATE,ASN_BITS,RWRITE,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,22}},
  {DOT3ADAGGPORTPARTNEROPERSTATE,ASN_BITS,RONLY,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,23}},
  {DOT3ADAGGPORTAGGREGATEORINDIVIDUAL,ASN_INTEGER,RONLY,lacp_snmp_dot3adAggPortTable,
   4, {2,1,1,24}},
  {DOT3ADAGGPORTSTATSLACPDUSRX, ASN_COUNTER, RONLY,
    lacp_snmp_dot3adAggPortStatsTable, 4, {2, 2, 1, 1}},
  {DOT3ADAGGPORTSTATSMARKERPDUSRX, ASN_COUNTER, RONLY,
    lacp_snmp_dot3adAggPortStatsTable, 4, {2, 2, 1, 2}},
  {DOT3ADAGGPORTSTATSMARKERRESPONSEPDUSRX, ASN_COUNTER, RONLY,
    lacp_snmp_dot3adAggPortStatsTable, 4, {2, 2, 1, 3}},
  {DOT3ADAGGPORTSTATSUNKNOWNRX, ASN_COUNTER, RONLY,
    lacp_snmp_dot3adAggPortStatsTable, 4, {2, 2, 1, 4}},
  {DOT3ADAGGPORTSTATSILLEGALRX, ASN_COUNTER, RONLY,
    lacp_snmp_dot3adAggPortStatsTable, 4, {2, 2, 1, 5}},
  {DOT3ADAGGPORTSTATSLACPDUSTX, ASN_COUNTER, RONLY,
    lacp_snmp_dot3adAggPortStatsTable, 4, {2, 2, 1, 6}},
  {DOT3ADAGGPORTSTATSMARKERPDUSTX, ASN_COUNTER, RONLY,
    lacp_snmp_dot3adAggPortStatsTable, 4, {2, 2, 1, 7}},
  {DOT3ADAGGPORTSTATSMARKERRESPONSEPDUSTX, ASN_COUNTER, RONLY,
    lacp_snmp_dot3adAggPortStatsTable, 4, {2, 2, 1, 8}},
  {DOT3ADAGGPORTDEBUGRXSTATE, ASN_INTEGER, RONLY, lacp_snmp_dot3adAggPortDebugTable,
   4, { 2, 3, 1, 1}},
  {DOT3ADAGGPORTDEBUGLASTRXTIME, ASN_TIMETICKS, RONLY, lacp_snmp_dot3adAggPortDebugTable,
   4, { 2, 3, 1, 2}},
  {DOT3ADAGGPORTDEBUGMUXSTATE, ASN_INTEGER, RONLY, lacp_snmp_dot3adAggPortDebugTable,
   4, { 2, 3, 1, 3}},
  {DOT3ADAGGPORTDEBUGMUXREASON, ASN_OCTET_STR, RONLY, lacp_snmp_dot3adAggPortDebugTable,
   4, { 2, 3, 1, 4}},
  {DOT3ADAGGPORTDEBUGACTORCHURNSTATE, ASN_INTEGER, RONLY, lacp_snmp_dot3adAggPortDebugTable,
   4, { 2, 3, 1, 5}},
  {DOT3ADAGGPORTDEBUGPARTNERCHURNSTATE, ASN_INTEGER, RONLY, lacp_snmp_dot3adAggPortDebugTable,
   4, { 2, 3, 1, 6}},
  {DOT3ADAGGPORTDEBUGACTORCHURNCOUNT, ASN_COUNTER, RONLY, lacp_snmp_dot3adAggPortDebugTable,
   4, { 2, 3, 1, 7}},
  {DOT3ADAGGPORTDEBUGPARTNERCHURNCOUNT, ASN_COUNTER, RONLY, lacp_snmp_dot3adAggPortDebugTable,
   4, { 2, 3, 1, 8}},
  {DOT3ADAGGPORTDEBUGACTORSYNCTRANSITIONCOUNT, ASN_COUNTER, RONLY, lacp_snmp_dot3adAggPortDebugTable,
   4, { 2, 3, 1, 9}},
  {DOT3ADAGGPORTDEBUGPARTNERSYNCTRANSITIONCOUNT, ASN_COUNTER, RONLY, lacp_snmp_dot3adAggPortDebugTable,
   4, { 2, 3, 1, 10}},
  {DOT3ADAGGPORTDEBUGACTORCHANGECOUNT, ASN_COUNTER, RONLY, lacp_snmp_dot3adAggPortDebugTable,
   4, { 2, 3, 1, 11}},
  {DOT3ADAGGPORTDEBUGPARTNERCHANGECOUNT, ASN_COUNTER, RONLY, lacp_snmp_dot3adAggPortDebugTable,
   4, { 2, 3, 1, 12}},
                         
  {DOT3ADTABLESLASTCHANGED,ASN_TIMETICKS,RONLY,lacp_snmp_dot3adTablesLastChanged,
   1, {3}},

};

/* Register the LACP-MIB. */
void
lacp_snmp_init ( struct lib_globals *zg )
{
  /* Register ourselves with the agent to handle our mib tree objects */
  snmp_init (zg, lacpd_oid, NELEMENTS(lacpd_oid));
  
  REGISTER_MIB (zg, "dot3adMIBObjects", dot3adMIBObjects,
                variable ,lacp_dot3adMIBObjects_oid);

  lacp_start_time = pal_time_current (NULL);

  snmp_start (zg);

}

/* Aggregator look-up procedure. */
struct lacp_aggregator *
lacp_snmp_aggregator_lookup (int agg_index, int exact)
{
  register unsigned int agg_ndx = 0;
  struct lacp_aggregator *curr = NULL;
  struct lacp_aggregator *best = NULL; /* assume none found */

  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      curr = LACP_AGG_LIST[agg_ndx];

      if (curr != NULL)
        {
          if (exact)
            {
              if (curr->agg_ix == agg_index)
                {
                  /* GET request and instance match */
                  best = curr;
                  break;
                }
            }
          else   /* GETNEXT request */
            {
              if (agg_index < curr->agg_ix)
                {
                  /* save if this is a better candidate and
                   * continue search
                   */
                  if (best == NULL)
                    best = curr;
                  else if ( curr->agg_ix < best->agg_ix )
                    best = curr;
                }
            }
        }
    }
  return best;
}

/* Call-back procedure for lacp_snmp_dot3adAggTable. */
unsigned char *
lacp_snmp_dot3adAggTable (struct variable *vp,
                          oid * name,
                          size_t * length,
                          int exact,
                          size_t * var_len, WriteMethod ** write_method,
                          u_int32_t vr_id)
{
  int ret = 0;
  static u_int32_t index = 0;
  struct lacp_aggregator *agg = NULL;
  unsigned int sys_prio = 0;

  /* validate the index */
  ret = l2_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
    return NULL;
  
  agg = lacp_snmp_aggregator_lookup (index, exact);
  if ( !agg )
    return NULL;

  if (!exact)
    {
      l2_snmp_index_set (vp, name, length, agg->agg_ix);
    }

  switch (vp->magic)
    {
      case DOT3ADAGGINDEX :
        LACP_SNMP_RETURN_INTEGER (agg->agg_ix);
        break;

      case DOT3ADAGGMACADDRESS :
        LACP_SNMP_RETURN_MACADDRESS (agg->aggregator_mac_address); 
        break;

      case DOT3ADAGGACTORSYSTEMPRIORITY :
        *write_method = lacp_snmp_write_dot3adAggTable;
        sys_prio = lacp_instance->system_identifier.system_priority;
        LACP_SNMP_RETURN_INTEGER (sys_prio);
        break;        
      
      case DOT3ADAGGACTORSYSTEMID :
        LACP_SNMP_RETURN_MACADDRESS (lacp_instance->system_identifier.system_id); 
        break;

      case DOT3ADAGGAGGREGATEORINDIVIDUAL : 
        if (agg->individual_aggregator)
          LACP_SNMP_RETURN_INTEGER (DOT3ADAGGINDIVIDUAL);
        else
          LACP_SNMP_RETURN_INTEGER (DOT3ADAGGAGGREGATE);
        break;

      case DOT3ADAGGACTORADMINKEY :
        *write_method = lacp_snmp_write_dot3adAggTable;
        LACP_SNMP_RETURN_INTEGER (agg->actor_admin_aggregator_key);
        break;

      case DOT3ADAGGACTOROPERKEY :
        LACP_SNMP_RETURN_INTEGER (agg->actor_oper_aggregator_key); 
        break;

      case DOT3ADAGGPARTNERSYSTEMID :
        LACP_SNMP_RETURN_MACADDRESS (agg->partner_system); 
        break;

      case DOT3ADAGGPARTNERSYSTEMPRIORITY :
        LACP_SNMP_RETURN_INTEGER (agg->partner_system_priority);
        break;

      case DOT3ADAGGPARTNEROPERKEY :
        LACP_SNMP_RETURN_INTEGER (agg->partner_oper_aggregator_key); 
        break;

      case DOT3ADAGGCOLLECTORMAXDELAY :
        /* More of a forwarding parameter.
         * Not supported currently in the forwarder module.
         */
        *write_method = lacp_snmp_write_dot3adAggTable;
        LACP_SNMP_RETURN_INTEGER (agg->collector_max_delay);
        break;
  
      default :
        pal_assert (0);
    }
  return NULL;
}

/* Procedure to check the duplicate value
 * of the aggregator's actor admin key  
 */
int
lacp_check_agg_admin_key (int admin_key)
{
  register unsigned int agg_ndx = 0;
  struct lacp_aggregator *curr = NULL;

  for (agg_ndx = 0; agg_ndx < LACP_AGG_COUNT_MAX; agg_ndx++)
    {
      curr = LACP_AGG_LIST[agg_ndx];
      if ( !curr )
        continue;

      if (curr->actor_admin_aggregator_key == admin_key)
        return -1;
    }
  return 0;
}

/* The WriteMethod for the lacp_snmp_dot3adAggTable */
int
lacp_snmp_write_dot3adAggTable (int action, u_char * var_val,
                                u_char var_val_type, size_t var_val_len,
                                u_char * statP, oid * name, size_t length,
                                struct variable *vp, u_int32_t vr_id)
{
  int ret = 0;
  u_int32_t index = 0;
  long intval = 0;
  struct lacp_aggregator *agg = NULL;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  if ( l2_snmp_index_get (vp, name, &length, &index, 1) < 0 )
    return SNMP_ERR_GENERR;

  pal_mem_cpy (&intval,var_val,4);
 
  agg = lacp_find_aggregator (index);
  if ( !agg )
    return SNMP_ERR_GENERR;
  
  switch (vp->magic)
    {
      case DOT3ADAGGACTORSYSTEMPRIORITY :
        if (intval < LACP_LINK_PRIORITY_MIN || intval > LACP_LINK_PRIORITY_MAX)
          return SNMP_ERR_BADVALUE;

        ret = lacp_set_system_priority (intval);          
        if (ret < 0)
          return SNMP_ERR_WRONGVALUE; 
        break;

      case DOT3ADAGGACTORADMINKEY :           
        if (intval < LACP_LINK_ADMIN_KEY_MIN || intval >LACP_LINK_ADMIN_KEY_MAX)
          return SNMP_ERR_BADVALUE;

        ret = lacp_check_agg_admin_key (intval);
        if (ret < 0)
          return SNMP_ERR_WRONGVALUE;

        agg->actor_admin_aggregator_key = intval;
#ifdef HAVE_HA
        lacp_cal_modify_lacp_agg (agg);
#endif /* HAVE_HA */
        break;

      case DOT3ADAGGCOLLECTORMAXDELAY :
        if (intval < LACP_AGG_COLLECTOR_MAX_DELAY_MIN ||
            intval > LACP_AGG_COLLECTOR_MAX_DELAY_MAX)
          return SNMP_ERR_BADVALUE;

        agg->collector_max_delay = intval;
#ifdef HAVE_HA
        lacp_cal_modify_lacp_agg (agg);
#endif /* HAVE_HA */
        break;

      default :
        pal_assert (0);
        return SNMP_ERR_GENERR;
    }

  return SNMP_ERR_NOERROR;
}

/* Lacp-Link look-up procedure. */
struct lacp_link *
lacp_snmp_link_lookup (int ifindex, int exact)
{
  register unsigned int link_ndx = 0;
  struct lacp_link *curr = NULL;
  struct lacp_link *best = NULL; /* assume none found */

  for (link_ndx = 0; link_ndx < LACP_MAX_LINKS; link_ndx++)
    {
      curr = LACP_LINKS[link_ndx];
     
      if ( curr != NULL )
        {
          if (exact)
            {
              if (curr->actor_port_number == ifindex)
                {
                  /* GET request and instance match */
                  best = curr;
                  break;
                }
            }
          else   /* GETNEXT request */
            {
              if (ifindex < curr->actor_port_number)
                {
                  /* save if this is a better candidate and
                   * continue search
                   */
                  if (best == NULL)
                    best = curr;
                  else if ( curr->actor_port_number < best->actor_port_number )
                    best = curr;
                }
            }
        }
    }
  
  return best;

}

/* Call-back procedure for lacp_snmp_dot3adAggPortTable. */
unsigned char *
lacp_snmp_dot3adAggPortTable (struct variable *vp,
                              oid * name,
                              size_t * length,
                              int exact,
                              size_t * var_len,
                              WriteMethod ** write_method,
                              u_int32_t vr_id)
{
  static u_int32_t index = 0;
  int ret = 0;
  unsigned int agg_ix = 0; 
  unsigned int sys_prio = 0;
  struct lacp_link *link = NULL;

  /* validate the index */
  ret = l2_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
    return NULL;

  link = lacp_snmp_link_lookup (index, exact);
  if ( !link)
    return NULL;

  if (!exact)
    {
      l2_snmp_index_set (vp, name, length,link->actor_port_number);
    }

  switch (vp->magic)
    {
      case DOT3ADAGGPORTINDEX :
        LACP_SNMP_RETURN_INTEGER (link->actor_port_number);
        break;

      case DOT3ADAGGPORTACTORSYSTEMPRIORITY :
        *write_method = lacp_snmp_write_dot3adAggPortTable;
        sys_prio = lacp_instance->system_identifier.system_priority;
        LACP_SNMP_RETURN_INTEGER (sys_prio);
        break;

      case DOT3ADAGGPORTACTORSYSTEMID :
        LACP_SNMP_RETURN_MACADDRESS(lacp_instance->system_identifier.system_id);
        break;

      case DOT3ADAGGPORTACTORADMINKEY :
        *write_method = lacp_snmp_write_dot3adAggPortTable;
        LACP_SNMP_RETURN_INTEGER (link->actor_admin_port_key);
        break;

      case DOT3ADAGGPORTACTOROPERKEY :
        *write_method = lacp_snmp_write_dot3adAggPortTable;
        LACP_SNMP_RETURN_INTEGER (link->actor_oper_port_key);
        break;

      case DOT3ADAGGPORTPARTNERADMINSYSTEMPRIORITY :
        *write_method = lacp_snmp_write_dot3adAggPortTable;
        LACP_SNMP_RETURN_INTEGER (link->partner_admin_system_priority); 
        break;

      case DOT3ADAGGPORTPARTNEROPERSYSTEMPRIORITY :
        LACP_SNMP_RETURN_INTEGER (link->partner_oper_system_priority);
        break;

      case DOT3ADAGGPORTPARTNERADMINSYSTEMID :
        /* Currently set operation not supported. */
        LACP_SNMP_RETURN_MACADDRESS (link->partner_admin_system);
        break;

      case DOT3ADAGGPORTPARTNEROPERSYSTEMID :
        /* Currently set operation not supported. */
        LACP_SNMP_RETURN_MACADDRESS (link->partner_oper_system);
        break;

      case DOT3ADAGGPORTPARTNERADMINKEY :
        *write_method = lacp_snmp_write_dot3adAggPortTable;
        LACP_SNMP_RETURN_INTEGER (link->partner_admin_key);
        break;
 
      case DOT3ADAGGPORTPARTNEROPERKEY :
        LACP_SNMP_RETURN_INTEGER (link->partner_oper_key);
        break;

      case DOT3ADAGGPORTSELECTEDAGGID :
        if (link->selected == SELECTED)
          agg_ix = link->aggregator->agg_ix;

        LACP_SNMP_RETURN_INTEGER (agg_ix);
        break;

      case DOT3ADAGGPORTATTACHEDAGGID :
        if (link->mux_machine_state == MUX_ATTACHED)
          agg_ix = link->aggregator->agg_ix;

        LACP_SNMP_RETURN_INTEGER (agg_ix);
        break; 

      case DOT3ADAGGPORTACTORPORT :
        LACP_SNMP_RETURN_INTEGER (link->actor_port_number);
        break;

      case DOT3ADAGGPORTACTORPORTPRIORITY :
        *write_method = lacp_snmp_write_dot3adAggPortTable;
        LACP_SNMP_RETURN_INTEGER (link->actor_port_priority);
        break;

      case DOT3ADAGGPORTPARTNERADMINPORT :
        *write_method = lacp_snmp_write_dot3adAggPortTable;
        LACP_SNMP_RETURN_INTEGER (link->partner_admin_port_number);
        break;

      case DOT3ADAGGPORTPARTNEROPERPORT :
        LACP_SNMP_RETURN_INTEGER (link->partner_oper_port_number);
        break;

      case DOT3ADAGGPORTPARTNERADMINPORTPRIORITY :
        *write_method = lacp_snmp_write_dot3adAggPortTable;
        LACP_SNMP_RETURN_INTEGER (link->partner_admin_port_priority);
        break;

      case DOT3ADAGGPORTPARTNEROPERPORTPRIORITY :
        LACP_SNMP_RETURN_INTEGER (link->partner_oper_port_priority);
        break;

      case DOT3ADAGGPORTACTORADMINSTATE :
        *write_method = lacp_snmp_write_dot3adAggPortTable_States;
        LACP_SNMP_RETURN_BITS (&link->actor_admin_port_state);
        break;

      case DOT3ADAGGPORTACTOROPERSTATE :
        LACP_SNMP_RETURN_BITS (&link->actor_oper_port_state);
        break;

      case DOT3ADAGGPORTPARTNERADMINSTATE :
        *write_method = lacp_snmp_write_dot3adAggPortTable_States;
        LACP_SNMP_RETURN_BITS (&link->partner_admin_port_state);
        break;

      case DOT3ADAGGPORTPARTNEROPERSTATE :
        LACP_SNMP_RETURN_BITS (&link->partner_oper_port_state);
        break;

      case DOT3ADAGGPORTAGGREGATEORINDIVIDUAL :
        if (link->aggregator->individual_aggregator)
          LACP_SNMP_RETURN_INTEGER (DOT3ADAGGPORTINDIVIDUAL);
        else
          LACP_SNMP_RETURN_INTEGER (DOT3ADAGGPORTAGGREGATE);
        break;

      default :
        pal_assert (0);
    }

   return NULL;
}

/* The WriteMethod for the lacp_snmp_dot3adAggPortTable */
int
lacp_snmp_write_dot3adAggPortTable (int action, u_char * var_val,
                                    u_char var_val_type, size_t var_val_len,
                                    u_char * statP, oid * name, size_t length,
                                    struct variable *vp, u_int32_t vr_id)
{
  u_int32_t index = 0;
  int ret = 0;
  long intval = 0;
  struct lacp_link *link = NULL;

  if ( l2_snmp_index_get (vp, name, &length, &index, 1) < 0 )
    return SNMP_ERR_GENERR;

  link = lacp_find_link (index);
  if ( !link )
    return SNMP_ERR_GENERR;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy (&intval, var_val, 4);

  switch (vp->magic)
    {
      case DOT3ADAGGPORTACTORSYSTEMPRIORITY :
        if (intval < LACP_LINK_PRIORITY_MIN || intval > LACP_LINK_PRIORITY_MAX)
          return SNMP_ERR_BADVALUE;

        ret = lacp_set_system_priority (intval);
        if (ret < 0)
          return SNMP_ERR_WRONGVALUE;       
        break;

      case DOT3ADAGGPORTACTORADMINKEY :
        if (intval < LACP_LINK_ADMIN_KEY_MIN || intval >LACP_LINK_ADMIN_KEY_MAX)
          return SNMP_ERR_BADVALUE;

        link->actor_admin_port_key = intval;
#ifdef HAVE_HA
        lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */

        break;

      case DOT3ADAGGPORTACTOROPERKEY :
        if (intval < LACP_LINK_OPER_KEY_MIN || intval > LACP_LINK_OPER_KEY_MAX)
          return SNMP_ERR_BADVALUE;

        link->actor_oper_port_key = intval;
#ifdef HAVE_HA
        lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */
        break;

      case DOT3ADAGGPORTPARTNERADMINSYSTEMPRIORITY :
        if (intval < LACP_LINK_PRIORITY_MIN || intval > LACP_LINK_PRIORITY_MAX)
          return SNMP_ERR_BADVALUE;

        link->partner_admin_system_priority = intval;
#ifdef HAVE_HA
        lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */
        break;

      case DOT3ADAGGPORTPARTNERADMINSYSTEMID :
        /* Currently not supported. */
        break;

      case DOT3ADAGGPORTPARTNERADMINKEY :
        if (intval < LACP_LINK_ADMIN_KEY_MIN || intval >LACP_LINK_ADMIN_KEY_MAX)
          return SNMP_ERR_BADVALUE;

        link->partner_admin_key = intval;
#ifdef HAVE_HA
        lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */
        break;

      case DOT3ADAGGPORTACTORPORTPRIORITY :
        if (intval < LACP_LINK_PRIORITY_MIN || intval > LACP_LINK_PRIORITY_MAX)
          return SNMP_ERR_BADVALUE;
      
        link->actor_port_priority = intval; 
#ifdef HAVE_HA
        lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */
        break;

      case DOT3ADAGGPORTPARTNERADMINPORT :
        if (intval < LACP_LINK_PRIORITY_MIN || intval > LACP_LINK_PRIORITY_MAX)
          return SNMP_ERR_BADVALUE;
 
        link->partner_admin_port_number = intval; 
#ifdef HAVE_HA
        lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */
        break;

      case DOT3ADAGGPORTPARTNERADMINPORTPRIORITY :
        if (intval < LACP_LINK_PRIORITY_MIN || intval > LACP_LINK_PRIORITY_MAX)
          return SNMP_ERR_BADVALUE;

        link->partner_admin_port_priority = intval;
#ifdef HAVE_HA
        lacp_cal_modify_lacp_link (link);
#endif /* HAVE_HA */
        break;

      default :
        pal_assert (0);
        return SNMP_ERR_GENERR;
    }

  return SNMP_ERR_NOERROR;

}

/* The WriteMethod for the lacp_snmp_dot3adAggPortTable_States */
int
lacp_snmp_write_dot3adAggPortTable_States (int action, u_char * var_val,
                                           u_char var_val_type,
                                           size_t var_val_len,
                                           u_char * statP, oid * name,
                                           size_t length,
                                           struct variable *vp, 
                                           u_int32_t vr_id)
{
  u_int32_t index = 0;
  size_t val_len;
  struct lacp_link *link = NULL;

  if ( l2_snmp_index_get (vp, name, &length, &index, 1) < 0 )
    return SNMP_ERR_GENERR;

  link = lacp_find_link (index);
  if ( !link )
    return SNMP_ERR_GENERR;

  if (var_val_type != ASN_OCTET_STR)
    return SNMP_ERR_WRONGTYPE;

  val_len = pal_strlen (var_val);

  if (val_len != sizeof(port_admin_state_bitstring))
    return SNMP_ERR_WRONGLENGTH;

  switch (vp->magic)
    {
      case DOT3ADAGGPORTACTORADMINSTATE :
      /*Returning when they set ADMINSTATE, as
         * write is not supported.*/
        return SNMP_ERR_GENERR;
        
       /* The code is commented out as SET is not supported now.
        * pal_mem_cpy(&link->actor_admin_port_state,var_val,
                     sizeof(link->actor_admin_port_state));
#ifdef HAVE_HA
        lacp_cal_modify_lacp_link (link);
#endif*/
        break;

      case DOT3ADAGGPORTPARTNERADMINSTATE :
        
        /*Returning when they set ADMINSTATE, as
         * write is not supported */
        return SNMP_ERR_GENERR;

        /* The code is commented out as SET is not supported now.
         * pal_mem_cpy(&link->partner_admin_port_state,var_val,
                    sizeof(link->partner_admin_port_state));

#ifdef HAVE_HA
        lacp_cal_modify_lacp_link (link);
#endif*/
        break;

      default :
        pal_assert (0);
        return SNMP_ERR_GENERR;
    }

  return SNMP_ERR_NOERROR;

}

/*
 *  lacp_snmp_dot3adAggPortListTable():
 *  Handle this table separately from the scalar value case.
 *  The workings of this are basically the same as for var_lagMIB above.
 *  */
unsigned char  *
lacp_snmp_dot3adAggPortListTable(struct variable *vp, oid * name,
                                 size_t * length, int exact,
                                 size_t * var_len,
                                 WriteMethod ** write_method,
                                 u_int32_t vr_id)
{
    int ret = 0;
    static u_int32_t index = 0;
    struct lacp_aggregator *agg = NULL;
    static char agg_port_list[BITSTRINGSIZE(LACP_L2_SNMP_PORT_ARRAY_SIZE)];

    pal_mem_set(agg_port_list, 0, sizeof(agg_port_list));

    /* validate the index */
    ret = l2_snmp_index_get (vp, name, length, &index, exact);
    if (ret < 0)
      return NULL;

    agg = lacp_snmp_aggregator_lookup (index, exact);
    if ( !agg )
      return NULL;

    lacp_snmp_get_port_list(agg, agg_port_list);

    if (!exact)
      {
        l2_snmp_index_set (vp, name, length, agg->agg_ix);
      }

    /* this is where we do the value assignments for the mib results. */
    switch (vp->magic) {
      case DOT3ADAGGPORTLISTPORTS:
       LACP_SNMP_RETURN_STRING(agg_port_list, sizeof(agg_port_list));
        break;
      default:
        return NULL;
    }
    return NULL;
}

/*
 *  lacp_snmp_dot3adAggPortStatsTable():
 *  Handle this table separately from the scalar value case.
 *  The workings of this are basically the same as for var_lagMIB above.
 *  */
unsigned char  *
lacp_snmp_dot3adAggPortStatsTable(struct variable *vp, oid * name,
    size_t * length, int exact,
    size_t * var_len, WriteMethod ** write_method,
    u_int32_t vr_id)
{
    static u_int32_t index = 0;
    int ret = 0;
    struct lacp_link *link = NULL;

    /* validate the index */
    ret = l2_snmp_index_get (vp, name, length, &index, exact);
    if (ret < 0)
      return NULL;

    link = lacp_snmp_link_lookup (index, exact);
    if ( !link)
      return NULL;

    if (!exact)
      {
        l2_snmp_index_set (vp, name, length, link->actor_port_number);
      }

    /*
     *  this is where we do the value assignments for the mib results.
     */
    switch (vp->magic) {
      case DOT3ADAGGPORTSTATSLACPDUSRX:
        LACP_SNMP_RETURN_INTEGER(link->lacpdu_recv_count);
        break;
      case DOT3ADAGGPORTSTATSMARKERPDUSRX:
        LACP_SNMP_RETURN_INTEGER(link->mpdu_recv_count);
        break;
      case DOT3ADAGGPORTSTATSMARKERRESPONSEPDUSRX:
        LACP_SNMP_RETURN_INTEGER(link->mpdu_response_recv_count);
        break;
      case DOT3ADAGGPORTSTATSUNKNOWNRX:
        LACP_SNMP_RETURN_INTEGER(link->pckt_unknown_rx_count);
        break;
      case DOT3ADAGGPORTSTATSILLEGALRX:
        LACP_SNMP_RETURN_INTEGER(link->pckt_recv_err_count);
        break;
      case DOT3ADAGGPORTSTATSLACPDUSTX:
        LACP_SNMP_RETURN_INTEGER(link->lacpdu_sent_count);
        break;
      case DOT3ADAGGPORTSTATSMARKERPDUSTX:
        LACP_SNMP_RETURN_INTEGER(link->mpdu_sent_count);
        break;
      case DOT3ADAGGPORTSTATSMARKERRESPONSEPDUSTX:
        LACP_SNMP_RETURN_INTEGER(link->mpdu_response_sent_count);
        break;
      default:
        return NULL;
    }

}

void
lacp_snmp_get_port_list(struct lacp_aggregator *agg, char *port_list)
{
    int link_ndx = 0;

    if (!agg)
      return;

    for (link_ndx = 0; link_ndx < LACP_MAX_AGGREGATOR_LINKS; link_ndx++)
      {
        if (agg->link[link_ndx])
          {
            if (agg->link[link_ndx]->actor_port_number >
                LACP_L2_SNMP_HW_IF_INDEX)
              bitstring_setbit(port_list,
                               (agg->link[link_ndx]->actor_port_number -
                                LACP_L2_SNMP_HW_IF_INDEX));
            else
               bitstring_setbit(port_list,
                                (agg->link[link_ndx]->actor_port_number ));
          }
      }
}

/* Call-back procedure for lacp_snmp_dot3adAggPortTable. */
unsigned char *
lacp_snmp_dot3adTablesLastChanged (struct variable *vp,
                                   oid * name,
                                   size_t * length,
                                   int exact,
                                   size_t * var_len,
                                   WriteMethod ** write_method,
                                   u_int32_t vr_id)
{
  if (snmp_header_generic (vp, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return NULL;

  switch (vp->magic)
    {
      case DOT3ADTABLESLASTCHANGED :
        LACP_SNMP_RETURN_TIMETICKS (0);/* Currently not supported. */
        break;

      default :
        pal_assert (0);
    }

   return NULL;

}

int
lacp_snmp_rcv_state (enum lacp_rcv_state s)
{
  switch (s)
    {
    case RCV_INVALID:
      return 0;
    case RCV_INITIALIZE:
      return 4;
    case RCV_PORT_DISABLED:
      return 6;
    case RCV_LACP_DISABLED:
      return 5;
    case RCV_EXPIRED:
      return 2;
    case RCV_DEFAULTED:
      return 3;
    case RCV_CURRENT:
      return 1;
    default:
      return 0;
    }
}

/* Returns the SNMP value for actor and partner churn state*/
int
lacp_snmp_actor_churn_state (enum lacp_actor_churn_state s)
{
  switch (s)
    {
    case ACTOR_CHURN_INVALID:
      return 0;
    case ACTOR_CHURN_MONITOR:
      return 3;
    case NO_ACTOR_CHURN:
      return 1;
    case ACTOR_CHURN:
      return 2;
    default:
      return 0;
    }

}

int
lacp_snmp_partner_churn_state (enum lacp_partner_churn_state s)
{
  switch (s)
    {
    case PARTNER_CHURN_INVALID:
      return 0;
    case PARTNER_CHURN_MONITOR:
      return 3;
    case NO_PARTNER_CHURN:
      return 1;
    case PARTNER_CHURN:
      return 2;
    default:
      return 0;
    }

}

/* Call-back procedure for lacp_snmp_dot3adAggPortTable. */
unsigned char *
lacp_snmp_dot3adAggPortDebugTable(struct variable *vp,
                                  oid * name,
                                  size_t * length,
                                  int exact,
                                  size_t * var_len,
                                  WriteMethod ** write_method,
                                  u_int32_t vr_id)
{
    
  static u_int32_t index = 0;
  int ret = 0;
  struct lacp_link *link = NULL;
  char *mux_state;

  /* validate the index */
  ret = l2_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
    return NULL;
 
  link = lacp_snmp_link_lookup (index, exact);
  if ( !link)
      return NULL;

  if (!exact)
      {
            l2_snmp_index_set (vp, name, length, link->actor_port_number);
              }

  /*
   *  this is where we do the value assignments for the mib results.
   */
  
  switch (vp->magic) {
    case DOT3ADAGGPORTDEBUGRXSTATE:
      LACP_SNMP_RETURN_INTEGER(lacp_snmp_rcv_state (link->rcv_state));
      break;
    case DOT3ADAGGPORTDEBUGLASTRXTIME:
      LACP_SNMP_RETURN_TIMETICKS (time_ticks(pal_time_current(NULL),
                  link->last_pdu_rx_time));
      break;
    case DOT3ADAGGPORTDEBUGMUXSTATE:
      LACP_SNMP_RETURN_INTEGER(link->mux_machine_state + 1);
      break;
    case DOT3ADAGGPORTDEBUGMUXREASON:
      mux_state = mux_state_str (link->mux_machine_state );
      LACP_SNMP_RETURN_STRING(mux_state, strlen (mux_state));
      break;
    case DOT3ADAGGPORTDEBUGACTORCHURNSTATE:
      LACP_SNMP_RETURN_INTEGER(lacp_snmp_actor_churn_state
                               (link->actor_churn_state));
      break;
    case DOT3ADAGGPORTDEBUGPARTNERCHURNSTATE:
      LACP_SNMP_RETURN_INTEGER(lacp_snmp_partner_churn_state
                               (link->partner_churn_state));
      break;
    case DOT3ADAGGPORTDEBUGACTORCHURNCOUNT:
      LACP_SNMP_RETURN_INTEGER(link->actor_churn_count);
      break;
    case DOT3ADAGGPORTDEBUGPARTNERCHURNCOUNT:
      LACP_SNMP_RETURN_INTEGER(link->partner_churn_count);
      break;
    case DOT3ADAGGPORTDEBUGACTORSYNCTRANSITIONCOUNT:
      LACP_SNMP_RETURN_INTEGER(link->actor_sync_transition_count);
      break;
    case DOT3ADAGGPORTDEBUGPARTNERSYNCTRANSITIONCOUNT:
      LACP_SNMP_RETURN_INTEGER(link->partner_sync_transition_count);
      break;
    case DOT3ADAGGPORTDEBUGACTORCHANGECOUNT:
      LACP_SNMP_RETURN_INTEGER(lacp_instance->actor_change_count);
      break;
    case DOT3ADAGGPORTDEBUGPARTNERCHANGECOUNT:
      LACP_SNMP_RETURN_INTEGER(link->partner_change_count);
      break;

    default:
      return NULL;
  }

  return NULL;
}

#endif /* HAVE_SNMP */
