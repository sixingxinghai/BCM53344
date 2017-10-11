/* Copyright (C) 2005-2006  All Rights Reserved. */

#ifndef _PACOS_LACP_SNMP_H
#define _PACOS_LACP_SNMP_H

#include "lib.h"
#include "lacp_types.h"

/*
 * This is the root OID we use to register the objects handled in this
 * module.
 */

#define LACPDOID 1,2,840,10006,300,43

/*
 * SMUX magic numbers for registered objects.  
 */ 

#define DOT3ADAGGINDEX                              1
#define DOT3ADAGGMACADDRESS                         2
#define DOT3ADAGGACTORSYSTEMPRIORITY                3
#define DOT3ADAGGACTORSYSTEMID                      4
#define DOT3ADAGGAGGREGATEORINDIVIDUAL              5
#define DOT3ADAGGACTORADMINKEY                      6
#define DOT3ADAGGACTOROPERKEY                       7
#define DOT3ADAGGPARTNERSYSTEMID                    8
#define DOT3ADAGGPARTNERSYSTEMPRIORITY              9
#define DOT3ADAGGPARTNEROPERKEY                     10
#define DOT3ADAGGCOLLECTORMAXDELAY                  11
#define DOT3ADAGGPORTINDEX                          12
#define DOT3ADAGGPORTACTORSYSTEMPRIORITY            13
#define DOT3ADAGGPORTACTORSYSTEMID                  14
#define DOT3ADAGGPORTACTORADMINKEY                  15
#define DOT3ADAGGPORTACTOROPERKEY                   16
#define DOT3ADAGGPORTPARTNERADMINSYSTEMPRIORITY     17
#define DOT3ADAGGPORTPARTNEROPERSYSTEMPRIORITY      18
#define DOT3ADAGGPORTPARTNERADMINSYSTEMID           19
#define DOT3ADAGGPORTPARTNEROPERSYSTEMID            20
#define DOT3ADAGGPORTPARTNERADMINKEY                21
#define DOT3ADAGGPORTPARTNEROPERKEY                 22
#define DOT3ADAGGPORTSELECTEDAGGID                  23
#define DOT3ADAGGPORTATTACHEDAGGID                  24
#define DOT3ADAGGPORTACTORPORT                      25
#define DOT3ADAGGPORTACTORPORTPRIORITY              26
#define DOT3ADAGGPORTPARTNERADMINPORT               27
#define DOT3ADAGGPORTPARTNEROPERPORT                28
#define DOT3ADAGGPORTPARTNERADMINPORTPRIORITY       29
#define DOT3ADAGGPORTPARTNEROPERPORTPRIORITY        30
#define DOT3ADAGGPORTACTORADMINSTATE                31
#define DOT3ADAGGPORTACTOROPERSTATE                 32
#define DOT3ADAGGPORTPARTNERADMINSTATE              33
#define DOT3ADAGGPORTPARTNEROPERSTATE               34
#define DOT3ADAGGPORTAGGREGATEORINDIVIDUAL          35
#define DOT3ADTABLESLASTCHANGED                     36

#define DOT3ADAGGPORTSTATSLACPDUSRX                 37
#define DOT3ADAGGPORTSTATSMARKERPDUSRX              38
#define DOT3ADAGGPORTSTATSMARKERRESPONSEPDUSRX      39
#define DOT3ADAGGPORTSTATSUNKNOWNRX                 40
#define DOT3ADAGGPORTSTATSILLEGALRX                 41
#define DOT3ADAGGPORTSTATSLACPDUSTX                 42
#define DOT3ADAGGPORTSTATSMARKERPDUSTX              43
#define DOT3ADAGGPORTSTATSMARKERRESPONSEPDUSTX      44

#define DOT3ADAGGPORTDEBUGRXSTATE                   45
#define DOT3ADAGGPORTDEBUGLASTRXTIME                46
#define DOT3ADAGGPORTDEBUGMUXSTATE                  47
#define DOT3ADAGGPORTDEBUGMUXREASON                 48
#define DOT3ADAGGPORTDEBUGACTORCHURNSTATE           49
#define DOT3ADAGGPORTDEBUGPARTNERCHURNSTATE         50
#define DOT3ADAGGPORTDEBUGACTORCHURNCOUNT           51
#define DOT3ADAGGPORTDEBUGPARTNERCHURNCOUNT         52
#define DOT3ADAGGPORTDEBUGACTORSYNCTRANSITIONCOUNT  53
#define DOT3ADAGGPORTDEBUGPARTNERSYNCTRANSITIONCOUNT 54
#define DOT3ADAGGPORTDEBUGACTORCHANGECOUNT           55
#define DOT3ADAGGPORTDEBUGPARTNERCHANGECOUNT         56

#define DOT3ADAGGPORTLISTPORTS                      57

#define ASN_BITS ASN_OCTET_STR

/* LacpState values */

#define DOT3ADAGGPORTACTORADMINSTATELACPACTIVITY    0
#define DOT3ADAGGPORTACTORADMINSTATELACPTIMEOUT     1
#define DOT3ADAGGPORTACTORADMINSTATEAGGREGATION     2
#define DOT3ADAGGPORTACTORADMINSTATESYNC            3
#define DOT3ADAGGPORTACTORADMINSTATECOLLECTING      4
#define DOT3ADAGGPORTACTORADMINSTATEDISTRIBUTING    5
#define DOT3ADAGGPORTACTORADMINSTATEDEFAULTED       6
#define DOT3ADAGGPORTACTORADMINSTATEEXPIRED         7

/* enums for dot3adAggAggregateOrIndividual */
#define DOT3ADAGGAGGREGATE                          1
#define DOT3ADAGGINDIVIDUAL                         2

/* enums for dot3adAggPortAggregateOrIndividual */
#define DOT3ADAGGPORTAGGREGATE                      1
#define DOT3ADAGGPORTINDIVIDUAL                     2

#define LACP_L2_SNMP_PORT_ARRAY_SIZE                33
#define LACP_L2_SNMP_HW_IF_INDEX                    5000

#if !defined(NELEMENTS)
#define NELEMENTS(x) ((sizeof(x))/(sizeof((x)[0])))
#endif

/* SNMP value return using static variables.  */

#define LACP_SNMP_RETURN_INTEGER(V) \
  do { \
    *var_len = sizeof (int); \
    lacp_int_val = V; \
    return (u_char *) &lacp_int_val; \
  } while (0)


#define LACP_SNMP_RETURN_TIMETICKS(V) \
  do { \
    *var_len = sizeof (u_long); \
    lacp_ulong_val = V; \
    return (u_char *) &lacp_ulong_val; \
  } while (0)

#define LACP_SNMP_RETURN_MACADDRESS(V) \
  do { \
    *var_len = ETHER_ADDR_LEN; \
    lacp_string_val = (u_char *)(V); \
    return lacp_string_val; \
  } while (0)

#define LACP_SNMP_RETURN_STRING(V,L) \
  do { \
    *var_len = L; \
    lacp_string_val = (u_char *)(V); \
    return lacp_string_val; \
  } while (0)

#define LACP_SNMP_RETURN_BITS(V) \
  do { \
    *var_len = sizeof (u_char); \
    lacp_string_val = V; \
    return lacp_string_val; \
  } while (0)

extern void
lacp_snmp_init (struct lib_globals *);

extern struct lacp_aggregator *
lacp_snmp_aggregator_lookup (int agg_index, int exact);

extern int
lacp_check_agg_admin_key (int admin_key);

extern struct lacp_link *
lacp_snmp_link_lookup (int ifindex, int exact);
int
lacp_snmp_actor_churn_state (enum lacp_actor_churn_state s);
int
lacp_snmp_partner_churn_state (enum lacp_partner_churn_state s);

#endif  /* _PACOS_LACP_SNMP_H */
