/* Copyright (C) 2001-2004  All Rights Reserved. */

#include "pal.h"
#include "lib.h"

#ifdef HAVE_SNMP

#include "asn1.h"
#include "if.h"
#include "log.h"
#include "prefix.h"
#include "table.h"
#include "snmp.h"
#include "l2lib.h"
#include "pal_math.h"
#include "l2_snmp.h"
#include "l2_timer.h"

#include "mstp_config.h"
#include "mstpd.h"
#include "mstp_types.h"
#include "mstp_bridge.h"
#include "mstp_api.h"
#include "mstp_port.h"
#include "mstp_snmp.h"
#include "mstp_transmit.h"

#include "mstp_snmp_sstp.h"
/* local variables for SNMP support*/

static pal_time_t mstp_start_time;
static int mstp_int_val;
static struct bridge_id mstp_snmp_bridge_id;
static u_long mstp_ulong_val;
static u_short mstp_ushort_val;
u_char *mstp_string_val;

u_int32_t
mstp_nsm_get_port_path_cost (struct lib_globals *zg,
                             const u_int32_t ifindex);

/* Prototypes of the functions defined in file */
FindVarMethod mstp_snmp_dot1dBridgeScalars;
FindVarMethod mstp_snmp_dot1dStpPortTable;
FindVarMethod mstp_snmp_dot1dStpExtPortTable;

WriteMethod mstp_snmp_write_dot1dBridgeScalars;
WriteMethod mstp_snmp_write_dot1dStpPortTable;
WriteMethod mstp_snmp_write_dot1dStpExtPortTable;

int mstp_snmp_dot1dBaseType = 2;                 /* transparent-only(2) */
int mstp_snmp_dot1dStpProtocolSpecification = 3; /* ieee8021d(3) */

/* need to instrument forwarder */
static u_int32_t dot1dTpLearnedEntryDiscards = 0;

/* The object ID's for registration. */
static oid mstp_oid[] = { BRIDGEMIBOID };

static oid mstp_dot1dBaseGroup_oid1 [] = { BRIDGEMIBOID, 1 , 1 };
static oid mstp_dot1dBaseGroup_oid2 [] = { BRIDGEMIBOID, 1 , 2 };
static oid mstp_dot1dBaseGroup_oid3 [] = { BRIDGEMIBOID, 1 , 3 };
static oid mstp_dot1dSTPGroup_oid [] = { BRIDGEMIBOID, 2 };
static oid mstp_dot1dTpGroup_oid1 [] = { BRIDGEMIBOID, 4 , 1 };
static oid mstp_dot1dTpGroup_oid2 [] = { BRIDGEMIBOID, 4 , 2 };

static oid mstpd_oid[] = { RSTPDOID };

static oid mstp_trap_oid [] = { BRIDGEMIBOID, 0 };

char  mstp_br_ports_bitstring[BITSTRINGSIZE(L2_BRIDGE_MAX_PORTS)];

/*
 * variable dot1dBridge_variables:
 *   this variable defines function callbacks and type return information
 *   for the  mib section
 */

struct variable mstp_dot1dBaseGroup1 [] = {
  /*
   * magic number        , variable type , ro/rw , callback fn  , L, oidsuffix
   * (L = length of the oidsuffix)
   */
  /* dot1dBase (1) */
  {DOT1DBASEBRIDGEADDRESS, ASN_OCTET_STR, RONLY, mstp_snmp_dot1dBridgeScalars,
   0, {0}},
};

struct variable mstp_dot1dBaseGroup2 [] = {

  {DOT1DBASENUMPORTS, ASN_INTEGER, RONLY, mstp_snmp_dot1dBridgeScalars,
   0, {0}},
};

struct variable mstp_dot1dBaseGroup3 [] = {

  {DOT1DBASETYPE, ASN_INTEGER, RONLY, mstp_snmp_dot1dBridgeScalars,
   0, {0}},
};

struct variable mstp_dot1dSTPGroup [] = {

  /* dot1dStp (2) */
  {DOT1DSTPPROTOCOLSPECIFICATION, ASN_INTEGER, RONLY,
   mstp_snmp_dot1dBridgeScalars,
   1, {1}},
  {DOT1DSTPPRIORITY, ASN_INTEGER, RWRITE, mstp_snmp_dot1dBridgeScalars,
   1, {2}},
  {DOT1DSTPTIMESINCETOPOLOGYCHANGE, ASN_TIMETICKS, RONLY,
   mstp_snmp_dot1dBridgeScalars,
   1, {3}},
  {DOT1DSTPTOPCHANGES, ASN_COUNTER, RONLY, mstp_snmp_dot1dBridgeScalars,
   1, {4}},
  {DOT1DSTPDESIGNATEDROOT, ASN_OCTET_STR, RONLY, mstp_snmp_dot1dBridgeScalars,
   1, {5}},
  {DOT1DSTPROOTCOST, ASN_INTEGER, RONLY, mstp_snmp_dot1dBridgeScalars,
   1, {6}},
  {DOT1DSTPROOTPORT, ASN_INTEGER, RONLY, mstp_snmp_dot1dBridgeScalars,
   1, {7}},
  {DOT1DSTPMAXAGE, ASN_INTEGER, RONLY, mstp_snmp_dot1dBridgeScalars,
   1, {8}},
  {DOT1DSTPHELLOTIME, ASN_INTEGER, RONLY, mstp_snmp_dot1dBridgeScalars,
   1, {9}},
  {DOT1DSTPHOLDTIME, ASN_INTEGER, RONLY, mstp_snmp_dot1dBridgeScalars,
   1, {10}},
  {DOT1DSTPFORWARDDELAY, ASN_INTEGER, RONLY, mstp_snmp_dot1dBridgeScalars,
   1, {11}},
  {DOT1DSTPBRIDGEMAXAGE, ASN_INTEGER, RWRITE, mstp_snmp_dot1dBridgeScalars,
   1, {12}},
  {DOT1DSTPBRIDGEHELLOTIME, ASN_INTEGER, RWRITE, mstp_snmp_dot1dBridgeScalars,
   1, {13}},
  {DOT1DSTPBRIDGEFORWARDDELAY, ASN_INTEGER, RWRITE,
   mstp_snmp_dot1dBridgeScalars,
   1, {14}},

  /* dot1dStpPortTable (2,15) */
  /* dot1dStpPortEntry (2,15,1) */
  {DOT1DSTPPORT, ASN_INTEGER, RONLY, mstp_snmp_dot1dStpPortTable,
   3, {15, 1, 1}},
  {DOT1DSTPPORTPRIORITY, ASN_INTEGER, RWRITE, mstp_snmp_dot1dStpPortTable,
   3, {15, 1, 2}},
  {DOT1DSTPPORTSTATE, ASN_INTEGER, RONLY, mstp_snmp_dot1dStpPortTable,
   3, {15, 1, 3}},
  {DOT1DSTPPORTENABLE, ASN_INTEGER, RWRITE, mstp_snmp_dot1dStpPortTable,
   3, {15, 1, 4}},
  {DOT1DSTPPORTPATHCOST, ASN_INTEGER, RWRITE, mstp_snmp_dot1dStpPortTable,
   3, {15, 1, 5}},
  {DOT1DSTPPORTDESIGNATEDROOT, ASN_OCTET_STR, RONLY,
   mstp_snmp_dot1dStpPortTable,
   3, {15, 1, 6}},
  {DOT1DSTPPORTDESIGNATEDCOST, ASN_INTEGER, RONLY, mstp_snmp_dot1dStpPortTable,
   3, {15, 1, 7}},
  {DOT1DSTPPORTDESIGNATEDBRIDGE, ASN_OCTET_STR, RONLY,
   mstp_snmp_dot1dStpPortTable,
   3, {15, 1, 8}},
  {DOT1DSTPPORTDESIGNATEDPORT, ASN_OCTET_STR, RONLY,
   mstp_snmp_dot1dStpPortTable,
   3, {15, 1, 9}},
  {DOT1DSTPPORTFORWARDTRANSITIONS, ASN_COUNTER, RONLY,
   mstp_snmp_dot1dStpPortTable,
   3, {15, 1, 10}},
  {DOT1DSTPPORTPATHCOST32, ASN_INTEGER, RWRITE, mstp_snmp_dot1dStpPortTable,
   3, {15, 1, 11}},
  {DOT1DSTPVERSION, ASN_INTEGER, RWRITE,
   mstp_snmp_dot1dBridgeScalars,
   1, {16}},
  {DOT1DSTPTXHOLDCOUNT, ASN_INTEGER, RWRITE,
   mstp_snmp_dot1dBridgeScalars,
   1, {17}},

  /* This object has been made obsolete in the latest Bridge RSTP MIB */
  /*
  {DOT1DSTPPATHCOSTDEFAULT, ASN_INTEGER, RWRITE,
   mstp_snmp_dot1dBridgeScalars,
   1, {18}},
  */

  /* dot1dStpExtPortTable (2,19) */
  /* dot1dStpExtPortEntry (2,19,1) */

  {DOT1DSTPPORTPROTOCOLMIGRATION, ASN_INTEGER, RWRITE,
   mstp_snmp_dot1dStpExtPortTable,
   3, {19, 1, 1}},
  {DOT1DSTPPORTADMINEDGEPORT, ASN_INTEGER, RWRITE,
   mstp_snmp_dot1dStpExtPortTable,
   3, {19, 1, 2}},
  {DOT1DSTPPORTOPEREDGEPORT, ASN_INTEGER, RONLY,
   mstp_snmp_dot1dStpExtPortTable,
   3, {19, 1, 3}},
  {DOT1DSTPPORTADMINPOINTTOPOINT, ASN_INTEGER, RWRITE,
   mstp_snmp_dot1dStpExtPortTable,
   3, {19, 1, 4}},
  {DOT1DSTPPORTOPERPOINTTOPOINT, ASN_INTEGER, RONLY,
   mstp_snmp_dot1dStpExtPortTable,
   3, {19, 1, 5}},
  {DOT1DSTPPORTADMINPATHCOST, ASN_INTEGER, RWRITE,
   mstp_snmp_dot1dStpExtPortTable,
   3, {19, 1, 6}},

};

struct variable mstp_dot1dTpGroup1 [] = {
  /* dot1dTp (4 ,1) */
  {DOT1DTPLEARNEDENTRYDISCARDS, ASN_COUNTER, RONLY,
   mstp_snmp_dot1dBridgeScalars,
   0, {0}},
};

struct variable mstp_dot1dTpGroup2 [] = {
  /* dot1dTp (4 ,2) */
  {DOT1DTPAGINGTIME, ASN_INTEGER, RWRITE, mstp_snmp_dot1dBridgeScalars,
   0, {0}},
};

/* RSTP SMUX trap function */

void
mstp_snmp_smux_trap (oid *trap_oid, size_t trap_oid_len,
                     oid spec_trap_val, u_int32_t uptime,
                     struct snmp_trap_object *obj, size_t obj_len)
{
  snmp_trap2 (mstpm, trap_oid, trap_oid_len, spec_trap_val,
                  mstp_oid, sizeof (mstp_oid) / sizeof (oid),
                  (struct trap_object2 *) obj, obj_len, uptime);
}

/* Register the BRIDGE-MIB.
 */
void
mstp_snmp_init (struct lib_globals *zg)
{

  /*
   * register ourselves with the agent to handle our mib tree objects
   */
  snmp_init (zg, mstpd_oid, NELEMENTS(mstpd_oid));

  REGISTER_MIB (zg, "mstp_dot1dBaseGroup1", mstp_dot1dBaseGroup1, variable, mstp_dot1dBaseGroup_oid1);
  REGISTER_MIB (zg, "mstp_dot1dBaseGroup2", mstp_dot1dBaseGroup2, variable, mstp_dot1dBaseGroup_oid2);
  REGISTER_MIB (zg, "mstp_dot1dBaseGroup3", mstp_dot1dBaseGroup3, variable, mstp_dot1dBaseGroup_oid3);
  REGISTER_MIB (zg, "mstp_dot1dSTPGroup", mstp_dot1dSTPGroup, variable, mstp_dot1dSTPGroup_oid);
  REGISTER_MIB (zg, "dot1dTPGroup1", mstp_dot1dTpGroup1, variable, mstp_dot1dTpGroup_oid1);
  REGISTER_MIB (zg, "dot1dTPGroup2", mstp_dot1dTpGroup2, variable, mstp_dot1dTpGroup_oid2);

  init_dot1sStp(zg);
  
  mstp_start_time = pal_time_current (NULL);

  snmp_start (zg);

}

/*  Function to return the bridge port state value */
int 
mstp_snmp_get_port_state (struct mstp_port *port)
{
  if (port->br->type == NSM_BRIDGE_TYPE_STP
      || port->br->type == NSM_BRIDGE_TYPE_STP_VLANAWARE)
    {
      if (port->cist_role == ROLE_DISABLED)
        return DOT1DRSTPPORTSTATE_DISABLED;

      else if (port->cist_role == ROLE_BACKUP
               || port->cist_role == ROLE_ALTERNATE)
        return DOT1DRSTPPORTSTATE_BLOCKING;

      else if ((port->cist_state == STATE_DISCARDING)
               && (port->cist_role == ROLE_DESIGNATED
                   || port->cist_role == ROLE_ROOTPORT))
          return DOT1DRSTPPORTSTATE_LISTENING;

      else if (port->cist_state == STATE_FORWARDING)
         return DOT1DRSTPPORTSTATE_FORWARDING;

      else  if (port->cist_state == STATE_LEARNING)
        return DOT1DRSTPPORTSTATE_LEARNING;
    }
  return DOT1DRSTPPORTSTATE_BROKEN;
}

/*
 *   mstp_snmp_dot1dBridgeScalars():
 *   This function is called every time the agent gets a request for
 *   a scalar variable found within the dot1dBridge tree.
 */
unsigned char *
mstp_snmp_dot1dBridgeScalars (struct variable *vp,
                              oid * name,
                              size_t * length,
                              int exact, size_t * var_len,
                              WriteMethod ** write_method,
                              u_int32_t vr_id)
{
  struct mstp_bridge *br;
  /*
   * variables we may use later
   */

  if (snmp_header_generic (vp, name, length, exact, var_len, write_method)
      == MATCH_FAILED)
    return NULL;

  br = mstp_get_first_bridge();
  if (br == NULL)
    return NULL;


  /*
   * this is where we do the value assignments for the mib results.
   */
  switch (vp->magic)
    {
    case DOT1DBASEBRIDGEADDRESS:
      MSTP_SNMP_RETURN_MACADDRESS (br->cist_bridge_id.addr);
      break;

    case DOT1DBASENUMPORTS:
      MSTP_SNMP_RETURN_INTEGER (br->num_ports);
      break;

    case DOT1DBASETYPE:
      MSTP_SNMP_RETURN_INTEGER (mstp_snmp_dot1dBaseType);
      break;

    case DOT1DSTPPROTOCOLSPECIFICATION:
      MSTP_SNMP_RETURN_INTEGER (mstp_snmp_dot1dStpProtocolSpecification);
      break;

    case DOT1DSTPPRIORITY:
      *write_method = mstp_snmp_write_dot1dBridgeScalars;
      MSTP_SNMP_RETURN_INTEGER (br->cist_bridge_priority);
      break;

    case DOT1DSTPTIMESINCETOPOLOGYCHANGE:
      MSTP_SNMP_RETURN_TIMETICKS (time_ticks(pal_time_current(NULL),
                                             br->time_last_topo_change));
      break;

    case DOT1DSTPTOPCHANGES:
      MSTP_SNMP_RETURN_INTEGER (br->num_topo_changes);
      break;

    case DOT1DSTPDESIGNATEDROOT:
      MSTP_SNMP_RETURN_BRIDGEID (br->cist_designated_root);
      break;

    case DOT1DSTPROOTCOST:
      MSTP_SNMP_RETURN_INTEGER (br->external_root_path_cost);
      break;

    case DOT1DSTPROOTPORT:
      MSTP_SNMP_RETURN_INTEGER (br->cist_root_port_id & 0xfff);
      break;

    case DOT1DSTPMAXAGE:
      MSTP_SNMP_RETURN_INTEGER (MSTP_SNMP_TIMER_TO_TIMEOUT
                                (br->bridge_max_age));
      break;

    case DOT1DSTPHELLOTIME:
      MSTP_SNMP_RETURN_INTEGER (MSTP_SNMP_TIMER_TO_TIMEOUT
                                (br->bridge_hello_time));
      break;

    case DOT1DSTPHOLDTIME:
      /* As per IEEE802.1w: Hold Time is defined as "not more than
       * TXHoldCount BPDUs transmitted in hello time. Hence we return
       * the value of Bridge Hello time as Hold time in MIB.
       */
      if (IS_BRIDGE_STP (br))
        MSTP_SNMP_RETURN_INTEGER (MSTP_SNMP_TIMER_TO_TIMEOUT
                                  (STP_HOLD_TIME * L2_TIMER_SCALE_FACT));

      else
        MSTP_SNMP_RETURN_INTEGER (MSTP_SNMP_TIMER_TO_TIMEOUT
                                  (br->bridge_hello_time));

      break;

    case DOT1DSTPFORWARDDELAY:
      MSTP_SNMP_RETURN_INTEGER (MSTP_SNMP_TIMER_TO_TIMEOUT (br->bridge_forward_delay));
      break;

    case DOT1DSTPBRIDGEMAXAGE:
      *write_method = mstp_snmp_write_dot1dBridgeScalars;
      MSTP_SNMP_RETURN_INTEGER (MSTP_SNMP_TIMER_TO_TIMEOUT
                                (br->bridge_max_age));
      break;

    case DOT1DSTPBRIDGEHELLOTIME:
      *write_method = mstp_snmp_write_dot1dBridgeScalars;
      MSTP_SNMP_RETURN_INTEGER (MSTP_SNMP_TIMER_TO_TIMEOUT
                                (br->bridge_hello_time));
      break;

    case DOT1DSTPBRIDGEFORWARDDELAY:
      *write_method = mstp_snmp_write_dot1dBridgeScalars;
      MSTP_SNMP_RETURN_INTEGER (MSTP_SNMP_TIMER_TO_TIMEOUT
                                (br->bridge_forward_delay));
      break;

    case DOT1DSTPVERSION:
      *write_method = mstp_snmp_write_dot1dBridgeScalars;
      MSTP_SNMP_RETURN_INTEGER (br->force_version);
      break;

    case DOT1DSTPTXHOLDCOUNT:
      *write_method = mstp_snmp_write_dot1dBridgeScalars;
      MSTP_SNMP_RETURN_INTEGER (br->transmit_hold_count);
      break;

    /*
    case DOT1DSTPPATHCOSTDEFAULT:
      *write_method = NULL;
      MSTP_SNMP_RETURN_INTEGER (STP8021T2001);
      break;
    */

    case DOT1DTPLEARNEDENTRYDISCARDS:
      MSTP_SNMP_RETURN_INTEGER (dot1dTpLearnedEntryDiscards);
      break;

    case DOT1DTPAGINGTIME:
      *write_method = mstp_snmp_write_dot1dBridgeScalars;
      MSTP_SNMP_RETURN_INTEGER (br->ageing_time);
      break;

    default:
      pal_assert (0);
      break;
    }
  return NULL;
}

/* The write method for dot1dBridge Scalar objects */
int
mstp_snmp_write_dot1dBridgeScalars (int action, u_char * var_val,
                                    u_char var_val_type, size_t var_val_len,
                                    u_char * statP, oid * name, size_t length,
                                    struct variable *v, u_int32_t vr_id)
{
  long intval;
  struct mstp_bridge *br = mstp_get_first_bridge();
  int ret;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  pal_mem_cpy(&intval,var_val,4);

  switch (v->magic)
    {
    case DOT1DSTPBRIDGEHELLOTIME:
      if ((intval % 100) || intval < 100 || intval > 1000)
        return SNMP_ERR_BADVALUE;
      if (mstp_api_set_hello_time (br->name, intval / 100) !=
          RESULT_OK)
        return SNMP_ERR_BADVALUE;
      break;

    case DOT1DTPAGINGTIME:
      if (intval < 10 || intval > 1000000)
        return SNMP_ERR_BADVALUE;
      if (mstp_api_set_ageing_time (br->name, intval) != RESULT_OK)
        return SNMP_ERR_BADVALUE;
      break;

    case DOT1DSTPBRIDGEFORWARDDELAY:
      if ((intval % 100) || intval < 400 || intval > 3000)
        return SNMP_ERR_BADVALUE;
      if (mstp_api_set_forward_delay (br->name, intval / 100) !=
          RESULT_OK)
        return SNMP_ERR_BADVALUE;
      break;

    case DOT1DSTPPRIORITY:
      if ((intval % 4096) || intval < 0 || intval > 61440)
        return SNMP_ERR_BADVALUE;
      if (mstp_api_set_bridge_priority (br->name, intval) != RESULT_OK)
        return SNMP_ERR_BADVALUE;
      break;

    case DOT1DSTPBRIDGEMAXAGE:
      if ((intval % 100) || intval < 600 || intval > 4000)
        return SNMP_ERR_BADVALUE;
      if (mstp_api_set_max_age (br->name, intval / 100) != RESULT_OK)
        return SNMP_ERR_BADVALUE;
      break;

    case DOT1DSTPVERSION:
      ret = mstp_api_set_bridge_forceversion (br->name, intval);
      if (ret < 0)
        return SNMP_ERR_BADVALUE;
      break;

    case DOT1DSTPTXHOLDCOUNT:
      if ((intval < 1) || (intval > 10))
        return SNMP_ERR_BADVALUE;
      mstp_api_set_transmit_hold_count (br->name, intval);
      break;

    default:
      pal_assert (0);
      return SNMP_ERR_GENERR;
    }

  return SNMP_ERR_NOERROR;
}

/*
 * given a bridge, instance and exact/inexact request, returns a pointer to
 * an stp_port structure or NULL if none found.
 */
struct mstp_port *
mstp_snmp_bridge_port_lookup (struct mstp_bridge *br, u_int32_t port_instance,
                              int exact)
{
  struct mstp_port *curr = NULL;
  struct mstp_port *best = NULL; /* assume none found */

  for (curr = br->port_list; curr; curr = curr->next)
    {
      if (exact)
        {
          if (curr->ifindex == port_instance)
            {
              /* GET request and instance match */
              best = curr;
              break;
            }
        }
      else                      /* GETNEXT request */
        {
          if (port_instance < curr->ifindex)
            {
              /* save if this is a better candidate and
               *   continue search
               */
              if (best == NULL)
                best = curr;
              else if (curr->ifindex < best->ifindex)
                best = curr;
            }
        }
    }
  return best;
}

/*
 *   mstp_snmp_dot1dStpPortTable():
 *   Callback procedure for dot1dStpPortTable 
 */
unsigned char *
mstp_snmp_dot1dStpPortTable (struct variable *vp,
                             oid * name,
                             size_t * length,
                             int exact,
                             size_t * var_len, WriteMethod ** write_method,
                             u_int32_t vr_id)
{
  int ret;
  static u_int32_t index = 0;
  struct mstp_bridge *br = NULL;
  struct mstp_port *br_port;

  /* validate the index */
  ret = l2_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
    return NULL;

  br = mstp_get_first_bridge ();
  if (br == NULL)
    return NULL;

  br_port = mstp_snmp_bridge_port_lookup (br, index, exact);
  if (br_port == NULL)
    return NULL;

  if (!exact)
    {
      l2_snmp_index_set (vp, name, length, br_port->ifindex);
    }

  switch (vp->magic)
    {
    case DOT1DSTPPORT:
      MSTP_SNMP_RETURN_INTEGER (br_port->ifindex);
      break;

    case DOT1DSTPPORTPRIORITY:
      *write_method = mstp_snmp_write_dot1dStpPortTable;
      MSTP_SNMP_RETURN_INTEGER (br_port->cist_priority);
      break;

    case DOT1DSTPPORTSTATE:
      MSTP_SNMP_RETURN_INTEGER (mstp_snmp_get_port_state (br_port));
      break;

    case DOT1DSTPPORTENABLE:
      *write_method = mstp_snmp_write_dot1dStpPortTable;
      MSTP_SNMP_RETURN_INTEGER ( br_port->cist_role == ROLE_DISABLED ?
                                 DOT1DRSTPPORTENABLE_DISABLED :
                                 DOT1DRSTPPORTENABLE_ENABLED);
      break;

    case DOT1DSTPPORTPATHCOST:
      *write_method = mstp_snmp_write_dot1dStpPortTable;

    /* This check has been obsoleted by latest RFC */
#if 0
      if (br_port->cist_path_cost > 65535)
         MSTP_SNMP_RETURN_INTEGER (DOT1DSTPPORTPATHCOST_MAX);
      else   
#endif
      MSTP_SNMP_RETURN_INTEGER (br_port->cist_path_cost);
      break;

    case DOT1DSTPPORTDESIGNATEDROOT:
      MSTP_SNMP_RETURN_BRIDGEID (br_port->cist_root);
      break;

    case DOT1DSTPPORTDESIGNATEDCOST:
      MSTP_SNMP_RETURN_INTEGER (br_port->cist_external_rpc);
      break;

    case DOT1DSTPPORTDESIGNATEDBRIDGE:
      MSTP_SNMP_RETURN_BRIDGEID (br_port->cist_designated_bridge);
      break;

    case DOT1DSTPPORTDESIGNATEDPORT:
      mstp_ushort_val = pal_hton16 (br_port->cist_designated_port_id);
      MSTP_SNMP_RETURN_STRING (&mstp_ushort_val, sizeof (mstp_ushort_val));
      break;

    case DOT1DSTPPORTFORWARDTRANSITIONS:
      MSTP_SNMP_RETURN_INTEGER (br_port->cist_forward_transitions);
      break;

    case DOT1DSTPPORTPATHCOST32:
      *write_method = mstp_snmp_write_dot1dStpPortTable;
      MSTP_SNMP_RETURN_INTEGER (br_port->cist_path_cost);
      break;

    default:
      pal_assert (0);
    }
  return NULL;
}

/* Write method for dot1dStpPortTable */

int
mstp_snmp_write_dot1dStpPortTable (int action, u_char * var_val,
                                   u_char var_val_type, size_t var_val_len,
                                   u_char * statP, oid * name, size_t length,
                                   struct variable *vp, u_int32_t vr_id)
{
  struct mstp_bridge *br;
  struct mstp_port *br_port;
  u_int32_t   port;
  long intval;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  /* get the port number */
  if ( l2_snmp_index_get (vp, name, &length, &port, 1) < 0 )
    return SNMP_ERR_GENERR;

  pal_mem_cpy(&intval,var_val,4);

  br = mstp_get_first_bridge ();
  if (br == NULL)
    return SNMP_ERR_GENERR;

  br_port = mstp_find_cist_port (br, port);
  if (br_port == NULL)
    return SNMP_ERR_GENERR;

  switch (vp->magic)
    {
    case DOT1DSTPPORTPRIORITY:
      if (intval % 16 || intval < 0 || intval > 240)
        return SNMP_ERR_BADVALUE;
      mstp_api_set_port_priority (br->name, br_port->name,
                                  MSTP_VLAN_NULL_VID, intval);
      break;

    case DOT1DSTPPORTENABLE:
      switch (intval)
        {
        case DOT1DRSTPPORTENABLE_ENABLED:
          mstp_nsm_send_info (PAL_TRUE, br_port);
          break;

        case DOT1DRSTPPORTENABLE_DISABLED:
          mstp_nsm_send_info (PAL_FALSE, br_port);
          break;

        default:
          return SNMP_ERR_BADVALUE;
          break;
        }
      break;

    case DOT1DSTPPORTPATHCOST:
    case DOT1DSTPPORTPATHCOST32:
      if (intval < 1 || intval > 200000000)
        return SNMP_ERR_BADVALUE;
      mstp_api_set_port_path_cost (br->name, br_port->name,
                                   MSTP_VLAN_NULL_VID, intval);
      break;

    default:
      pal_assert (0);
      return SNMP_ERR_GENERR;
    }

  return SNMP_ERR_NOERROR;
}

/* Callback procedure for dot1dStpExtPortTable */

unsigned char *
mstp_snmp_dot1dStpExtPortTable (struct variable *vp,
                                oid * name,
                                size_t * length,
                                int exact,
                                size_t * var_len, WriteMethod ** write_method,
                                u_int32_t vr_id)
{
  int ret;
  static u_int32_t index = 0;
  struct mstp_bridge *br;
  struct mstp_port *br_port;

  /* validate the index */
  ret = l2_snmp_index_get (vp, name, length, &index, exact);
  if (ret < 0)
    return NULL;

  br = mstp_get_first_bridge();
  if (br == NULL)
    return NULL;

  br_port = mstp_snmp_bridge_port_lookup (br, index, exact);
  if (br_port == NULL)
    return NULL;

  if (!exact)
    {
      l2_snmp_index_set (vp, name, length, br_port->ifindex);
    }

  switch (vp->magic)
    {
    case DOT1DSTPPORTPROTOCOLMIGRATION:
      *write_method = mstp_snmp_write_dot1dStpExtPortTable;
      /* As per rstp v4 draft this mib object should always return FALSE
       * when read.
       */
      MSTP_SNMP_RETURN_INTEGER (MSTP_PORT_PROTOCOL_MIGRATION);
      break;

    case DOT1DSTPPORTADMINEDGEPORT:
      *write_method = mstp_snmp_write_dot1dStpExtPortTable;
      if (br_port->admin_edge) 
        MSTP_SNMP_RETURN_INTEGER (DOT1DSTPPORTADMINEDGEPORT_TRUE);
      else
        MSTP_SNMP_RETURN_INTEGER (DOT1DSTPPORTADMINEDGEPORT_FALSE);
      break;

    case DOT1DSTPPORTOPEREDGEPORT:
      if (br_port->oper_edge)
        MSTP_SNMP_RETURN_INTEGER (DOT1DSTPPORTOPEREDGEPORT_TRUE);
      else 
        MSTP_SNMP_RETURN_INTEGER (DOT1DSTPPORTOPEREDGEPORT_FALSE);
      break;

    case DOT1DSTPPORTADMINPOINTTOPOINT:
      *write_method = mstp_snmp_write_dot1dStpExtPortTable; 
      if (br_port->admin_p2p_mac == MSTP_ADMIN_LINK_TYPE_AUTO)
        MSTP_SNMP_RETURN_INTEGER (DOT1DSTPPORTADMINPOINT2POINT_AUTO);
      else if (br_port->admin_p2p_mac)
        MSTP_SNMP_RETURN_INTEGER (DOT1DSTPPORTADMINPOINT2POINT_TRUE);
      else
        MSTP_SNMP_RETURN_INTEGER (DOT1DSTPPORTADMINPOINT2POINT_FALSE);
      break;
        
    case DOT1DSTPPORTOPERPOINTTOPOINT:
      if (br_port->oper_p2p_mac)
        MSTP_SNMP_RETURN_INTEGER (DOT1DSTPPORTOPERPOINT2POINT_TRUE);
      else 
        MSTP_SNMP_RETURN_INTEGER (DOT1DSTPPORTOPERPOINT2POINT_FALSE);
      break;

    case DOT1DSTPPORTADMINPATHCOST:
      *write_method = mstp_snmp_write_dot1dStpExtPortTable;
      MSTP_SNMP_RETURN_INTEGER (br_port->cist_path_cost); 
      break;

    default:
      pal_assert (0);
    }
  return NULL;
}

/* write method for dot1dStpExtPortTable */

int mstp_snmp_write_dot1dStpExtPortTable (int action, u_char * var_val,
                                          u_char var_val_type, size_t var_val_len,
                                          u_char * statP, oid * name, size_t length,
                                          struct variable *vp, u_int32_t vr_id)
{
  struct mstp_bridge *br;
  struct mstp_port *br_port;
  u_int32_t   port;
  long intval;

  if (var_val_type != ASN_INTEGER)
    return SNMP_ERR_WRONGTYPE;

  if (var_val_len != sizeof (long))
    return SNMP_ERR_WRONGLENGTH;

  /* get the port number */
  if ( l2_snmp_index_get (vp, name, &length, &port, 1) < 0 )
    return SNMP_ERR_GENERR;

  pal_mem_cpy(&intval,var_val,4);

  br = mstp_get_first_bridge ();
  if (br == NULL)
    return SNMP_ERR_GENERR;

  br_port = mstp_find_cist_port (br, port);
  if (br_port == NULL)
    return SNMP_ERR_GENERR;

  switch (vp->magic)
    {
    case DOT1DSTPPORTPROTOCOLMIGRATION: 
      switch (intval)
        {
        case DOT1DSTPPORTPROTOCOLMIGRATION_TRUE:
          mstp_api_mcheck (br->name, br_port->name); 
          break;
        case DOT1DSTPPORTPROTOCOLMIGRATION_FALSE:
          /* AS per rstp v4 draft any opearation other than setting
           * this value to TRUE has no effect.
           */
          break;
        default:
          return SNMP_ERR_BADVALUE;
        }
      break;

    case DOT1DSTPPORTADMINEDGEPORT:
      switch (intval)
        {
        case DOT1DSTPPORTADMINEDGEPORT_TRUE:
          mstp_api_set_port_edge (br->name, br_port->name, PAL_TRUE,
                                  MSTP_CONFIG_PORTFAST);
          break;
        case DOT1DSTPPORTOPEREDGEPORT_FALSE:
          mstp_api_set_port_edge (br->name, br_port->name, PAL_FALSE,
                                  MSTP_CONFIG_PORTFAST);
          break; 
        default:
          return SNMP_ERR_BADVALUE;
        }
      break;

    case DOT1DSTPPORTADMINPOINTTOPOINT:
      switch (intval)
        {
        case DOT1DSTPPORTADMINPOINT2POINT_TRUE:
          mstp_api_set_port_p2p (br->name, br_port->name, PAL_TRUE);
          break;
        case DOT1DSTPPORTADMINPOINT2POINT_FALSE:
          mstp_api_set_port_p2p (br->name, br_port->name, PAL_FALSE);
          break; 
        case DOT1DSTPPORTADMINPOINT2POINT_AUTO:
          mstp_api_set_port_p2p (br->name, br_port->name, 
                                 MSTP_ADMIN_LINK_TYPE_AUTO);
          break;
        default:
          return SNMP_ERR_BADVALUE;
        }
      break;

    case DOT1DSTPPORTADMINPATHCOST:
      if (intval < 0 || intval > 200000000)
        return SNMP_ERR_BADVALUE;
      /* As per rstp v4 draft a value of 0 would mean default path cost
       * Default path ocst is obtained using l2_nsm_get_port_path_cost.
       */ 
      if (intval == 0)
        mstp_api_set_port_path_cost (br->name, br_port->name,
                                     MSTP_VLAN_NULL_VID,
                                     mstp_nsm_get_port_path_cost (mstpm, port));
      else
        mstp_api_set_port_path_cost (br->name, br_port->name,
                                     MSTP_VLAN_NULL_VID, intval);
      break;

    default:
      pal_assert (0);
      return SNMP_ERR_GENERR;
    }

  return SNMP_ERR_NOERROR;
}

void
mstp_snmp_trap_specific ( u_long trap_val )
{

  mstp_snmp_smux_trap (mstp_trap_oid, sizeof (mstp_trap_oid) / sizeof (oid),
                       trap_val, pal_time_current (NULL) - mstp_start_time,
                       NULL, 0 );
}


void
mstp_snmp_topology_change (struct mstp_port *port, int state)
{
  struct mstp_bridge *br = mstp_get_first_bridge();

  if (( pal_strncmp(port->br->name, br->name, IFNAMSIZ) == 0) &&
      ((port->cist_state == STATE_LEARNING && state == STATE_FORWARDING) ||
       (port->cist_state == STATE_FORWARDING && state == STATE_BLOCKING)))
    mstp_snmp_trap_specific( DOT1DTOPOLOGYCHANGE );
}

void
mstp_snmp_new_root (struct mstp_bridge * br)
{
  struct mstp_bridge *firstbr = mstp_get_first_bridge();
  if (pal_strncmp(br->name, firstbr->name, IFNAMSIZ) == 0)
    mstp_snmp_trap_specific( DOT1DNEWROOT );

}

#endif  /* HAVE_SNMP */ 

