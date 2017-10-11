/*--------------------------------------------------------------------------
 * Copyright (C) 2008 All rights reserved
 *
 * File name:   lib_fm_api.h
 * Purpose:      To provide definitions of the Fault Management API.
 * Description:  It is designated for use by any application, which is
 *               component capable of detection and reporting faults.
 *               Faults types must be registered during application startup.
 *               Only faults that are registered are captured.
 *               Only the last fault of any given type is captured.
 *               Captured faults can be displayed on PacOS command shell.
 *               The component allows feedback captured faults into
 *               high-availability FM client.
 *               NOTE: The reason why this faunctionality were separated from
 *                     cal/fm or lib/ha is to let the CAL to be excluded from
 *                     a PacOS build. The lib_fm however belongs to fundamental
 *                     system features, therefore must always be included
 *                     into any each build.
 * Author:      STC
 * Data:        04/10/2008
 *----------------------------------------------------------------------------
 */

#ifndef _LIB_FM_API_H_
#define _LIB_FM_API_H_

#include "pal.h"
#include "lib_fm_gid.h"


#define FM_MEMORY_ALLOCATION_FAILURE   0

#define FM_MAX_GROUP_NAME   15
#define FM_MAX_FAULT_NAME   47
#define FM_DATA_SIZE        512

/*----------------------------------------------------------------------------
 * Name:      fmEventType_e
 *
 * Purpose:   SAF/NTF defined event types
 *            Used by SAF NTF service. Each PacOS fault must be assigned
 *            one of these.
 * Enumerands:
 *  FM_EVT_COM  - Communications
 *  FM_EVT_QOS  - QOS
 *  FM_EVT_PRO  - Processing
 *  FM_EVT_EQP  - Equipment
 *  FM_EVT_ENV  - Environment
 *  FM_EVT_ALL  - ALL events - used for subscriptions
 *----------------------------------------------------------------------------
 */
typedef enum fmEventType_e
{
  FM_EVT_COM,
  FM_EVT_QOS,
  FM_EVT_PRO,
  FM_EVT_EQP,
  FM_EVT_ENV,
  FM_EVT_ALL
} fmEventType_e;


#define FM_EVT_NAMES  \
  {"COMM  ",  \
   "QOS   ",  \
   "PROC  ",  \
   "EQUIP ",  \
   "ENVIR ",  \
   "ALL   ",  \
   "MAX   "   \
  }


/*----------------------------------------------------------------------------
 * Name:        fmSeverity_e
 *
 * Purpose:     To identify all severity levels
 *
 * Enumerands:
 *  FM_SEV_CLEAR  - (SAF: Cleared) This means that a previously reported alarm
 *                  is cleared. Clearing of alarms can be done based on matching
 *                  event types, probable cause, and specific problem.
 *                  Or it may be based on the parameters in correlated notifications.
 *  FM_SEV_UNKN   - (SAF: Indeterminate) Severity cannot be determined by the
 *                  reporting entity.
 *  FM_SEV_WARN   - (SAF: Warning) A potential service affecting condition,
 *                  before any significant effects are felt.
 *  FM_SEV_MINOR  - A non-service affecting condition; however, corrective
 *                  actions are needed to avoid more problems.
 *  FM_SEV_MAJOR  - An urgent corrective action is required to avoid a service
 *                  affecting condition
 *  FM_SEV_CRIT   - (SAF: Critical) A service affecting condition
 *----------------------------------------------------------------------------
 */
#define FM_SEV_INFO FM_SEV_UNKN
#define FM_SEV_ERR  FM_SEV_CRIT

typedef enum fmSeverity_e
{
  FM_SEV_CLEAR,
  FM_SEV_UNKN,
  FM_SEV_WARN,  /* Protocol errors - user errors - recoverable */
  FM_SEV_MINOR,
  FM_SEV_MAJOR,
  FM_SEV_CRIT,
  FM_SEV_MAX
} fmSeverity_e;


#define FM_SEV_NAMES  \
  {"CLEAR  ",  \
   "UNKNOWN",  \
   "WARNING",  \
   "MINOR  ",  \
   "MAJOR  ",  \
   "CRITIC ",  \
   "MAX "      \
  }

/*----------------------------------------------------------------------------
 * Name:        fmRecovery_e - Recovery codes
 *
 * Purpose:     To identify recovery codes
 *              NOTE: For PacOS only: FM_RV_NONE, FM_RV_COMP_RST seems to be
 *                    useful.
 *
 * Members:
 *      FM_RV_NONE      - No recovery - typical for auto-recoverable faults
 *      FM_RV_COMP_RST  - Component restart
 *      FM_RV_COMP_FO   - Component failover
 *      FM_RV_NODE_SO   - Component switchover
 *      FM_RV_NODE_FO   - Node failover
 *      FM_RV_NODE_FF   - Node fail fast
 *      FM_RV_APPL_RST  - Application restart
 *      FM_RV_CLUS_RES  - Cluster restart
 *----------------------------------------------------------------------------
 */
typedef enum fmRecovery_e
{
  FM_RV_NONE,
  FM_RV_COMP_RST,
  FM_RV_COMP_FO,   /* not used */
  FM_RV_NODE_SO,   /* not used */
  FM_RV_NODE_FO,
  FM_RV_NODE_FF,
  FM_RV_APPL_RST,  /* not used */
  FM_RV_CLUS_RES,  /* not used */
  FM_RV_MAX
} fmRecovery_e;

#define FM_REC_NAMES  \
  {"NONE     ",     \
   "COMP_RST ",     \
   "COMP_F/O ",     \
   "NODE_S/O ",     \
   "NODE_F/O ",     \
   "NODE_F/F ",     \
   "APPL_RST ",     \
   "CLUS_RES ",     \
   "MAX      "      \
  }

/*----------------------------------------------------------------------------
 * Name:        fmProbableCause_e -
 *
 * Purpose:     To identify probable cause of fault occurance.
 *
 *----------------------------------------------------------------------------
 */
typedef enum fmProbableCause_e
{
  FM_PC_ADAPTER_ERROR,
  FM_PC_APPLICATION_SUBSYSTEM_FAILURE,          /**/
  FM_PC_BANDWIDTH_REDUCED,
  FM_PC_CALL_ESTABLISHMENT_ERROR,
  FM_PC_COMMUNICATIONS_PROTOCOL_ERROR,
  FM_PC_COMMUNICATIONS_SUBSYSTEM_FAILURE,
  FM_PC_CONFIGURATION_OR_CUSTOMIZATION_ERROR,
  FM_PC_CONGESTION,
  FM_PC_CORRUPT_DATA,                           /**/
  FM_PC_CPU_CYCLES_LIMIT_EXCEEDED,
  FM_PC_DATASET_OR_MODEM_ERROR,
  FM_PC_DEGRADED_SIGNAL,
  FM_PC_D_T_E,
  FM_PC_ENCLOSURE_DOOR_OPEN,
  FM_PC_EQUIPMENT_MALFUNCTION,
  FM_PC_EXCESSIVE_VIBRATION,
  FM_PC_FILE_ERROR,                             /**/
  FM_PC_FIRE_DETECTED,
  FM_PC_FLOOD_DETECTED,
  FM_PC_FRAMING_ERROR,
  FM_PC_HEATING_OR_VENTILATION_OR_COOLING_SYSTEM_PROBLEM,
  FM_PC_HUMIDITY_UNACCEPTABLE,
  FM_PC_INPUT_OUTPUT_DEVICE_ERROR,
  FM_PC_INPUT_DEVICE_ERROR,
  FM_PC_L_A_N_ERROR,
  FM_PC_LEAK_DETECTED,
  FM_PC_LOCAL_NODE_TRANSMISSION_ERROR,
  FM_PC_LOSS_OF_FRAME,
  FM_PC_LOSS_OF_SIGNAL,
  FM_PC_MATERIAL_SUPPLY_EXHAUSTED,
  FM_PC_MULTIPLEXER_PROBLEM,
  FM_PC_OUT_OF_MEMORY,                          /**/
  FM_PC_OUTPUT_DEVICE_ERROR,
  FM_PC_PERFORMANCE_DEGRADED,
  FM_PC_POWER_PROBLEM,
  FM_PC_PRESSURE_UNACCEPTABLE,
  FM_PC_PROCESSOR_PROBLEM,
  FM_PC_PUMP_FAILURE,
  FM_PC_QUEUE_SIZE_EXCEEDED,                    /**/
  FM_PC_RECEIVE_FAILURE,
  FM_PC_RECEIVER_FAILURE,
  FM_PC_REMOTE_NODE_TRANSMISSION_ERROR,
  FM_PC_RESOURCE_AT_OR_NEARING_CAPACITY,
  FM_PC_RESPONSE_TIME_EXCESSIVE,                /**/
  FM_PC_RETRANSMISSION_RATE_EXCESSIVE,
  FM_PC_SOFWARE_ERROR,                          /**/
  FM_PC_SOFWARE_PROGRAM_ABNORMALLY_TERMINATED,  /**/
  FM_PC_SOFTWARE_PROGRAM_ERROR,                 /**/
  FM_PC_STORAGE_CAPACITY_PROBLEM,
  FM_PC_TEMPERATURE_UNACCEPTABLE,
  FM_PC_THRESHOLD_CROSSED,
  FM_PC_TIMING_PROBLEM,
  FM_PC_TOXIC_LEAK_DETECTED,
  FM_PC_TRANSMIT_FAILURE,
  FM_PC_TRANSMITTER_FAILURE,
  FM_PC_UNDERLYING_RESOURCE_UNAVAILABLE,
  FM_PC_VERSION_MISMATCH,                       /**/
  FM_PC_AUTHENTICATION_FAILURE,
  FM_PC_BREACH_OF_CONFIDENTIALITY,
  FM_PC_CABLE_TAMPER,
  FM_PC_DELAYED_INFORMATION,
  FM_PC_DENIAL_OF_SERVICE,
  FM_PC_DUPLICATE_INFORMATION,
  FM_PC_INFORMATION_MISSING,
  FM_PC_INFORMATION_MODIFICATION_DETECTED,
  FM_PC_INFORMATION_OUT_OF_SEQUENCE,            /**/
  FM_PC_INTRUSION_DETECTION,
  FM_PC_KEY_EXPIRED,
  FM_PC_NON_REPUDIATION_FAILURE,
  FM_PC_OUT_OF_HOURS_ACTIVITY,
  FM_PC_OUT_OF_SERVICE,
  FM_PC_PROCEDURAL_ERROR,
  FM_PC_UNAUTHORIZED_ACCESS_ATTEMPT,
  FM_PC_UNEXPECTED_INFORMATION,
  FM_PC_UNSPECIFIED_REASON,                     /**/
  FM_PC_MAX,
} fmProbableCause_e;

/*----------------------------------------------------------------------------
 * Name:        fmFaultReg_t
 *
 * Purpose:     To hold a single fault registration data
 *
 * Members:
 *   ftr_FaultId  - Fault id - local to its group
 *   ftr_EventType- SAF defined event type
 *   ftr_Severity - SAF defined severity level
 *   ftr_Recovery - SAF defined recovery type
 *   ftr_ProbCause- SAF defined probable cause
 *   ftr_Name     - Fault name used for presentation purpose
 *----------------------------------------------------------------------------
 */
typedef struct fmFaultReg
{
  u_int16_t           ftr_FaultId;
  fmEventType_e       ftr_EventType;
  fmSeverity_e        ftr_Severity;
  fmRecovery_e        ftr_Recovery;
  fmProbableCause_e   ftr_ProbCause;
  char                ftr_Name[FM_MAX_FAULT_NAME+1];
} fmFaultReg_t;

/*------------------------------------------------------------------------------
 * These macros are to ease the pain with extensive lengths of initialization
 * sequence for fault definitions.
 * The macro name is structured as follows:
 *
 *      FM_DEF_<fmEventType_e>_<fmSeverity_e>_<fmRecovery_e>
 *
 * The application developer can use existing macros or add soem new.
 */
#define FM_DEF_COM_UNKN_RECNONE(fid, fstr, cause) \
  { fid, FM_EVT_COM, FM_SEV_UNKN,  FM_RV_NONE, cause, fstr }
#define FM_DEF_COM_WARN_RECNONE(fid, fstr, cause) \
  { fid, FM_EVT_COM, FM_SEV_WARN,  FM_RV_NONE, cause, fstr }
#define FM_DEF_COM_MINOR_RECNONE(fid, fstr, cause) \
  { fid, FM_EVT_COM, FM_SEV_WARN,  FM_RV_NONE, cause, fstr }
#define FM_DEF_COM_MAJOR_RECNONE(fid, fstr, cause) \
  { fid, FM_EVT_COM, FM_SEV_WARN,  FM_RV_NONE, cause, fstr }

#define FM_DEF_PRO_CRIT_COMPRST(fid, fstr, cause) \
  { fid, FM_EVT_PRO, FM_SEV_CRIT,  FM_RV_COMP_RST, cause, fstr }


/*----------------------------------------------------------------------------
 * Name:        fmFaultNtfCb_t
 *
 * Purpose:     To be implmented by FaultRecorder subscriber to be notified
 *              about fault occurence.
 *
 * Parameters:
 *     subCtx   - Unique subscriber context identifier
 *     grpId    - Fault group identifier
 *     faultId  - Local to group - fault identifier
 *     eventType- SAF defined event type
 *     severity - SAF definde severity of the fault
 *     recovery - SAF defined suggested recovery type
 *     probCause- SAF defined probable cause
 *     lastTime - Time of last occurence
 *     dataLen  - Lenght of opaque fault data or 0
 *     pData    - Pointer to opaque fault data or NULL. The subscriber shall not
 *                make any assumption about the buffer life time.
 *     pFileName- Source code file name
 *     lineNum  - Line number
 * Returns:
 *      NONE
 *----------------------------------------------------------------------------
 */
typedef void (* fmFaultNtfCb_t)(intptr_t       subCtx,
                                fmGroupId_e    grpId,
                                u_int32_t      faultId,
                                fmEventType_e  eventType,
                                fmSeverity_e   severity,
                                fmRecovery_e   recovery,
                                fmProbableCause_e probCause,
                                struct pal_timeval lastTime,
                                u_int32_t      dataLen,
                                u_int8_t      *pData,
                                char          *pFileName,
                                u_int32_t      lineNum);

/*-----------------------------------------------------------------------------
 * FM_NUM_REG_FAULTS    - Calculates number of registered faults
 * FM_REPORT            - Reporting a fault without opaque data
 * FM_REPORT_EXT        - Reporting a fault with opaque data
 * FM_ASSERT            - Reporting a fault depending on condition
 *
 * Parameters:
 *      tab     - Name of the table with fault registration data
 *      pLib    - Pointer to lib_globals
 *      gid     - Fault group identifier
 *      fid     - Fault identifier
 *      len     - Length of opaque data
 *      pData   - Pointer to opaque data
 *      c       - Condition: if evaluated to FALSE, a fault is generated
 *-----------------------------------------------------------------------------
 */
#define FM_NUM_REG_FAULTS(tab) (sizeof(tab) / sizeof(tab[0]))
#define FM_REPORT(zg, gid, fid) fmFaultRecord(zg, gid, fid,__LINE__,__FILE__)
#define FM_REPORT_EXT(zg, gid, fid, len, pData) \
          fmFaultRecordExt(zg, gid, fid, __LINE__, __FILE__, len, pData)
#define FM_ASSERT(zg, gid, fid, c) if (!(c)) \
          fmFaultRecord(zg, gid, fid, __LINE__, __FILE__)
#define FM_CLEAR_EXT(zg, gid, fid, len, pData) \
          fmFaultClearExt(zg, gid, fid, len, pData)

/*----------------------------------------------------------------------------
 * Name:        fmRegisterGroup
 * Purpose:     To register a group of faults.
 * Parameters:
 *      pLib      - Pointer to lib_globals
 *      groupId - Group id
 *      regTab  - A table initialized with fault registration data
 *      regCnt  - Number of entries in the regTab (use FM_NUM_REG_FAULTS
 *                macro to calculate this value).
 * Returns:
 *      ZRES_OK
 *      ZRES_ERR
 *
 * NOTE: All faults defiitions given via regTab parameter must be statically defined.
 *       The FM Recorder is using references to user defined registration data.
 *----------------------------------------------------------------------------
 */
ZRESULT fmRegisterGroup(struct lib_globals *pLib,
                        fmGroupId_e         groupId,
                        char               *pGrpName,
                        fmFaultReg_t        regTab[],
                        u_int16_t           regCnt);

/*----------------------------------------------------------------------------
 * Name:        fmFaultRecord/fmFaultRecordExt
 * Purpose:     To record fault occurrence (standard of extended recording).
 *              NOTE: Application shall use FM_RECORD and FM_ASSERT
 *                    macros to report a fault occurence.
 * Parameters:
 *      pLib    - Pointer to lib_globals
 *      groupId - Group id
 *      faultId - Fault id (local to a group)
 *      lineNum - Source code line #
 *      pFileName- Source code file name
 *      dataLen - Length of opaque data
 *      pData   - Pointer to opaque data
 * Returns: NONE
 *----------------------------------------------------------------------------
 */
void fmFaultRecord(struct lib_globals *pLib,
                   fmGroupId_e  groupId,
                   int32_t      faultId,
                   int          lineNum,
                   char        *pFileName);

void fmFaultRecordExt(struct lib_globals *pLib,
                      fmGroupId_e  groupId,
                      int32_t      faultId,
                      int          lineNum,
                      char        *pFileName,
                      int          dataLen,
                      u_int8_t    *pData);

void fmFaultClearExt(struct lib_globals *pLib,
                     fmGroupId_e  groupId,
                     int32_t      faultId,
                     int          dataLen,
                     u_int8_t    *pData);

/*----------------------------------------------------------------------------
 * Name:        fmSubscribe
 * Purpose:     To subscribe for notification when a fault occur.
 * Description: FaultRecorder user can subscribe for notifications for a single
 *              fault group or all groups. Different users can subscribe to the
 *              same fault group as well as a single user can use the same
 *              notification callback in multiple subscriptions to selected groups.
 *              However, the pair: (groupId, ntfCb) uniquely identifies
 *              a single subscritpion.
 * Parameters:
 *      pLib    - Pointer to lib_globals
 *      eventType- Specific event type or FM_EVT_ALL
 *      sevLevel- Threshold of subscriber sensitivity. It will be notified
 *                of any faults with this or higher level of severity.
 *      groupId - Fault group identifier. It allows to subscribe to faults
 *                of specific group or all faults (FM_GID_ALL);
 *      ntfCb   - Subscriber's notification callback.
 *      subCtx  - A unique subscriber's context reference and identification.
 *                This MAY NOT be a pointer to lib_globals.
 *
 * Returns:
 *      ZRES_OK
 *      ZRES_ERR
 *----------------------------------------------------------------------------
 */
ZRESULT fmSubscribe(struct lib_globals *pLib,
                    fmEventType_e       eventType,
                    fmSeverity_e        sevLevel,
                    fmGroupId_e         groupId,
                    fmFaultNtfCb_t      ntfCb,
                    intptr_t            subCtx);

/*----------------------------------------------------------------------------
 * Name:        fmRetrieve
 * Purpose:     To retrieve all recorder faults for a given subscriber. The
 *              function uses the registered callback(s) to notify subscriber about
 *              recorded faults.
 * Parameters:
 *      pLib    - Pointer to lib_globals
 *      subCtx  - Subscriber's context reference, same as submitted in the
 *                fmSubscribe.
 * Returns:
 *      ZRES_OK
 *      ZRES_ERR
 *----------------------------------------------------------------------------
 */
ZRESULT fmRetrieve(struct lib_globals *pLib, intptr_t subCxt);


/*----------------------------------------------------------------------------
 * Name:        fmInitFaultRecording
 * Purpose:     To initialize fault recording library.
 * Parameters:
 *      pLib    - Pointer to lib_globals instance.
 * Return:
 *      ZRES_OK
 *      ZRES_ERR
 *----------------------------------------------------------------------------
 */
ZRESULT fmInitFaultRecording(struct lib_globals *pLib);


#endif /* _LIB_FM_H_ */

