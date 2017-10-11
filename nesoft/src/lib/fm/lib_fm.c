
/*--------------------------------------------------------------------------
 * Copyright (C) 2007 All rights reserved
 *
 * File name:   lib_fm.c
 * Purpose:     To provide implementation of the fundamental Fault Management
 *              - Fault Recorder functionality.
 * Author:      STC
 * Data:        05/25/2007
 *----------------------------------------------------------------------------
 */

#include "pal.h"
#include "timeutil.h"
#include "lib_fm.h"
/* #include "lib_fm_gid.h" */



char *fm_sev_names[]   = FM_SEV_NAMES;
char *fm_evt_names[]   = FM_EVT_NAMES;
char *fm_rec_names[]   = FM_REC_NAMES;


/*----------------------------------------------------------------------------
 * Name:        fmRegisterGroup
 * Purpose:     To register a group of faults.
 * Parameters:
 *      pLib    - Pointer to lib_globals
 *      groupId - Group id
 *      pGrpName- A unique group naem for presentation purpose
 *      regTab  - A table initialized with fault registration data
 *      regCnt  - Number of entries in the regTab (use FM_NUM_REG_FAULTS
 *                macro to calculate this value).
 * Description:
 *      We do not expect the faults definitions arrive in the enum ordered way.
 *      It creates an enum ordered  vector of references to group fault definitions.
 *
 * Returns:
 *      ZRES_OK
 *      ZRES_ERR
 *----------------------------------------------------------------------------
 */
ZRESULT
fmRegisterGroup(struct lib_globals *pLib,
                fmGroupId_e         groupId,
                char               *pGrpName,
                fmFaultReg_t        regTab[],
                u_int16_t           regCnt)
{
  fmInstance_t *pFmIns=NULL;
  fmFaultReg_t *pFaultReg=NULL;
  fmFaultDef_t *pFaultDef=NULL;
  fmGroup_t    *pFaultGrp=NULL;
  u_int16_t     regIx=0;

  /* Sanity checks. */
  if (pLib->lib_fm == NULL || groupId<=FM_GID_NONE ||
      groupId >= FM_GID_MAX || regCnt <= 0) {
    return (ZRES_ERR);
  }
  pFmIns = (fmInstance_t *)pLib->lib_fm;
  pFaultGrp  = pFmIns->fmi_GrpRefTab[groupId];
  if (pFaultGrp != NULL) {
    return (ZRES_ERR);
  }
  /* Validate the registration data. */
  for (regIx=0; regIx<regCnt; regIx++)
  {
    pFaultReg = &regTab[regIx];
    if (pFaultReg->ftr_FaultId>=regCnt ||
        ! pal_strlen(pFaultReg->ftr_Name) ||
        pFaultReg->ftr_EventType >= FM_EVT_ALL ||
        pFaultReg->ftr_Severity >= FM_SEV_MAX ||
        pFaultReg->ftr_Recovery >= FM_RV_MAX ||
        pFaultReg->ftr_ProbCause>= FM_PC_MAX) {
      return (ZRES_ERR);
    }
  }
  /* Add new group. */
  pFaultGrp = (fmGroup_t *)XCALLOC(MTYPE_FM_LIB, sizeof(fmGroup_t));
  if (pFaultGrp == NULL) {
    return (ZRES_ERR);
  }
  pal_strncpy(pFaultGrp->fgr_Name, pGrpName, FM_MAX_GROUP_NAME);

  pFaultGrp->fgr_pFaultsDef = (fmFaultDef_t *)XCALLOC(MTYPE_FM_LIB,
                                                      sizeof(fmFaultDef_t) * regCnt);
  if (pFaultGrp->fgr_pFaultsDef == NULL) {
    XFREE(MTYPE_FM_LIB, pFaultGrp);
    return (ZRES_ERR);
  }
  /* Copy fault registration data. */
  for (regIx=0; regIx<regCnt; regIx++)
  {
    pFaultReg = &regTab[regIx];
    pFaultDef = &(pFaultGrp->fgr_pFaultsDef[pFaultReg->ftr_FaultId]);
    if (pFaultDef->ftd_GroupId!= FM_GID_NONE) {
      XFREE(MTYPE_FM_LIB, pFaultGrp->fgr_pFaultsDef);
      XFREE(MTYPE_FM_LIB, pFaultGrp);
      return (ZRES_ERR); /* Duplicate fault registration. */
    }
    pFaultDef->ftd_GroupId = groupId;
    pFaultDef->ftd_FaultId = pFaultReg->ftr_FaultId;
    pFaultDef->ftd_EventType= pFaultReg->ftr_EventType;
    pFaultDef->ftd_ProbCause= pFaultReg->ftr_ProbCause;
    pFaultDef->ftd_Severity= pFaultReg->ftr_Severity;
    pFaultDef->ftd_Recovery= pFaultReg->ftr_Recovery;
    pFaultReg->ftr_Name[FM_MAX_FAULT_NAME] = 0;
    pal_strcpy(pFaultDef->ftd_Name, pFaultReg->ftr_Name);
  }
  /* Set total number of faults in the group. */
  pFaultGrp->fgr_Cnt = regCnt;

  /* Attach the new group to FM Instance. */
  pFmIns->fmi_GrpRefTab[groupId] = pFaultGrp;

  return (ZRES_OK);
}

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
                   char        *pFileName)
{
  fmFaultRecordExt(pLib, groupId, faultId, lineNum, pFileName, 0, NULL);
}


void
fmFaultRecordExt(struct lib_globals *pLib,
                 fmGroupId_e  groupId,
                 int32_t      faultId,
                 int          lineNum,
                 char        *pFileName,
                 int          dataLen,
                 u_int8_t    *pData)
{
  fmInstance_t *pFmIns=NULL;
  fmGroup_t    *pGroup=NULL;
  fmFault_t    *pFault=NULL;
  fmFaultDef_t *pFaultDef=NULL;
#define _FM_LOG_STR_MAX 150
  char          logStr[_FM_LOG_STR_MAX+1];
  fmSubscr_t   *pSub=NULL;
  u_int16_t     subIx, subCnt;
  struct listnode *pThisNode=NULL;

  /* Sanity check */
  if (groupId <= FM_GID_NONE || groupId >= FM_GID_MAX) {
    return;
  }
  if ((pFmIns = (fmInstance_t *)pLib->lib_fm) == 0) {
    return;
  }
  pFmIns->fmi_SeqNum++;
  if ((pGroup = pFmIns->fmi_GrpRefTab[groupId]) == NULL) {
    return;
  }
  if (faultId >= pGroup->fgr_Cnt ) {
    return;
  }
  /* First, we will try to locate the fault on the fault list. */
  LIST_LOOP (&pFmIns->fmi_FaultList, pFault, pThisNode)
  {
    pFaultDef = pFault->flt_pFaultDef;

    if (pFaultDef->ftd_GroupId == groupId &&
        pFaultDef->ftd_FaultId == faultId &&
        pFault->flt_DataLen == dataLen &&
        ((dataLen == 0 && (pData == pFault->flt_pData)) ||
         (dataLen != 0 && pData != NULL && pFault->flt_pData != NULL &&
          pal_mem_cmp(pData, pFault->flt_pData, dataLen) == 0)))
    {
      /* Found it - exit the loop. */
      break;
    }
    pFault = NULL;
  }
  if (pFault != NULL )
  {
    /* Move it to the head of teh list. */
    LISTNODE_REMOVE(&pFmIns->fmi_FaultList, pThisNode);
    LISTNODE_ADD_HEAD(&pFmIns->fmi_FaultList, pThisNode);
  }
  else
  {
    /* Add a new node at the head. */
    pFault = XCALLOC(MTYPE_FM_LIB, sizeof(* pFault));
    if (pFault == NULL) {
      return;
    }
    pFaultDef = &pGroup->fgr_pFaultsDef[faultId];
    pFault->flt_pFaultDef = pFaultDef;

    /* If dataLen eq. 0 save pData, otherwise alloc memory and copy data. */
    pFault->flt_DataLen = dataLen;
    if (dataLen == 0) {
      pFault->flt_pData = pData;
    }
    else {
      pFault->flt_pData = (u_int8_t *)XMALLOC(MTYPE_FM_LIB, dataLen);
      if (pData != NULL) {
        pal_mem_cpy(pFault->flt_pData, pData, dataLen);
      }
    }
    listnode_add_before (&pFmIns->fmi_FaultList,
                         LISTHEAD(&pFmIns->fmi_FaultList),
                         pFault);
  }
  pFault->flt_State = PAL_TRUE;
  pFault->flt_Cnt++;
  pFault->flt_SeqNum = pFmIns->fmi_SeqNum;

  pal_strncpy (pFault->flt_FileName, pFileName, sizeof(pFault->flt_FileName)-1);;
  pFault->flt_FileName[sizeof(pFault->flt_FileName)-1] = 0;
  pFault->flt_FileLine = lineNum;

  pal_time_tzcurrent(&pFault->flt_LastTime, NULL);

  if (pFault->flt_FirstTime.tv_sec == (u_int32_t)0)
  {
    pFault->flt_FirstTime = pFault->flt_LastTime;
    pFmIns->fmi_ActiveCnt++;
  }

  /* ZLOG a message */
  pal_sprintf(logStr,
              "Fault:\"%s\" RAISED Gr:%s Id:%d Len:%d Ptr:%p Sev:%s Rec:%s File:%s Line:%u Cnt:%u",
              pFault->flt_pFaultDef->ftd_Name,
              pFmIns->fmi_GrpRefTab[groupId]->fgr_Name,
              pFaultDef->ftd_FaultId,
              pFault->flt_DataLen,
              pFault->flt_pData,
              fm_sev_names[pFault->flt_pFaultDef->ftd_Severity],
              fm_rec_names[pFault->flt_pFaultDef->ftd_Recovery],
              pFileName,
              lineNum,
              pFault->flt_Cnt);

  switch (pFault->flt_pFaultDef->ftd_Severity)
  {
  case FM_SEV_CLEAR:
    zlog_warn(pLib, logStr);
    break;
  case FM_SEV_UNKN:
    zlog_info(pLib, logStr);
    break;
  case FM_SEV_WARN:
  case FM_SEV_MINOR:
  case FM_SEV_MAJOR:
    zlog_warn(pLib, logStr);
    break;
  case FM_SEV_CRIT:
    zlog_err(pLib, logStr);
    break;
  default:
    break;
  }
  /* Dispatch to subscribers */
  subCnt = pFmIns->fmi_SubTabCnt;
  for (subIx=0; subIx<FM_MAX_SUBSCRIPTIONS && subCnt>0; subIx++)
  {
    if ((pSub = &pFmIns->fmi_SubTab[subIx]) == NULL) {
      continue;
    }
    if ((pSub->fms_GroupId == groupId ||
         pSub->fms_GroupId == FM_GID_ALL) &&
        (pSub->fms_EventType == FM_EVT_ALL ||
         pSub->fms_EventType == pFaultDef->ftd_EventType) &&
        pSub->fms_SevLevel<=pFault->flt_pFaultDef->ftd_Severity &&
        pSub->fms_NtfCb != NULL)
    {
      pSub->fms_NtfCb(pSub->fms_SubCtx,
                      groupId,
                      faultId,
                      pFaultDef->ftd_EventType,
                      pFaultDef->ftd_Severity,
                      pFaultDef->ftd_Recovery,
                      pFaultDef->ftd_ProbCause,
                      pFault->flt_LastTime,
                      dataLen,
                      pData,
                      pFileName,
                      lineNum);
    }
  }
}

/*----------------------------------------------------------------------------
 * Name:        fmFaultClearExt
 * Purpose:     To clear previously recorded runtime recoverable fault
 *              The fault will stay on the list to let the user to notice it
 *              via the show command
 * NOTE: To clear a fault, the following key values must match:
 *       - group id
 *       - fault id
 *       - dataLen
 *       - pData value or content of the area
 *
 * Parameters:
 *      pLib    - Pointer to lib_globals
 *      groupId - Group id
 *      faultId - Fault id (local to a group)
 *      dataLen - Length of opaque data
 *      pData   - Pointer to opaque data
 * Returns: NONE
 *----------------------------------------------------------------------------
 */
void
fmFaultClearExt(struct lib_globals *pLib,
                fmGroupId_e  groupId,
                int32_t      faultId,
                int          dataLen,
                u_int8_t    *pData)
{
  fmInstance_t *pFmIns=NULL;
  fmGroup_t    *pGroup=NULL;
  fmFault_t    *pFault=NULL;
  fmFaultDef_t *pFaultDef=NULL;
  fmSubscr_t   *pSub=NULL;
  u_int16_t     subIx, subCnt;
  struct listnode *pThisNode=NULL;

  /* Sanity check */
  if (groupId <= FM_GID_NONE || groupId >= FM_GID_MAX) {
    return;
  }
  if ((pFmIns = (fmInstance_t *)pLib->lib_fm) == 0) {
    return;
  }
  if ((pGroup = pFmIns->fmi_GrpRefTab[groupId]) == NULL) {
    return;
  }
  if (faultId >= pGroup->fgr_Cnt ) {
    return;
  }
  /* Locate the fault on the fault list. */
  LIST_LOOP (&pFmIns->fmi_FaultList, pFault, pThisNode)
  {
    pFaultDef = pFault->flt_pFaultDef;

    if (pFaultDef->ftd_GroupId == groupId &&
        pFaultDef->ftd_FaultId == faultId &&
        pFault->flt_DataLen == dataLen &&
        ((dataLen == 0 && (pData == pFault->flt_pData)) ||
         (dataLen != 0 && pData != NULL && pFault->flt_pData != NULL &&
          pal_mem_cmp(pData, pFault->flt_pData, dataLen) == 0)))
    {
      /* Found it - exit the loop. */
      break;
    }
    pFault = NULL;
  }
  if (pFault == NULL || pFault->flt_State == PAL_FALSE)
  {
    return;
  }
  pFmIns->fmi_SeqNum++;
  /* Process the fault - Move it to the head of the list. */
  LISTNODE_REMOVE(&pFmIns->fmi_FaultList, pThisNode);
  LISTNODE_ADD_HEAD(&pFmIns->fmi_FaultList, pThisNode);

  pal_time_tzcurrent(&pFault->flt_ClearTime, NULL);

  pFault->flt_State = PAL_FALSE;
  pFault->flt_SeqNum= pFmIns->fmi_SeqNum;

  /* ZLOG a message */
  zlog_warn (pLib,
             "Fault:\"%s\" CLEARED Gr:%s Id:%d Len:%d Ptr:%p Sev:%s Rec:%s File:%s Line:%u Cnt:%u",
             pFaultDef->ftd_Name,
             pFmIns->fmi_GrpRefTab[groupId]->fgr_Name,
             pFaultDef->ftd_FaultId,
             pFault->flt_DataLen,
             pFault->flt_pData,
             fm_sev_names[pFaultDef->ftd_Severity],
             fm_rec_names[pFaultDef->ftd_Recovery],
             pFault->flt_FileName,
             pFault->flt_FileLine,
             pFault->flt_Cnt);

  /* Dispatch to subscribers */
  subCnt = pFmIns->fmi_SubTabCnt;
  for (subIx=0; subIx<FM_MAX_SUBSCRIPTIONS && subCnt>0; subIx++)
  {
    if ((pSub = &pFmIns->fmi_SubTab[subIx]) == NULL) {
      continue;
    }
    if ((pSub->fms_GroupId == groupId ||
         pSub->fms_GroupId == FM_GID_ALL) &&
        (pSub->fms_EventType == FM_EVT_ALL ||
         pSub->fms_EventType == pFaultDef->ftd_EventType) &&
        pSub->fms_SevLevel<=pFaultDef->ftd_Severity &&
        pSub->fms_NtfCb != NULL)
    {
      pSub->fms_NtfCb(pSub->fms_SubCtx,
                      groupId,
                      faultId,
                      pFaultDef->ftd_EventType,
                      FM_SEV_CLEAR,
                      pFaultDef->ftd_Recovery,
                      pFaultDef->ftd_ProbCause,
                      pFault->flt_ClearTime,
                      dataLen,
                      pData,
                      pFault->flt_FileName,
                      pFault->flt_FileLine);
    }
  }
}

/*----------------------------------------------------------------------------
 * Name:        fmSubscribe
 * Purpose:     To subscribe for notification when a fault occur.
 * Description: FaultRecorder user can subscribe for notifications for a single
 *              fault group or all groups. Different users can subscribe to the
 *              same fault group as well as a single user can use the same
 *              context rerference in multiple subscriptions to selected groups.
 *              However, the pair: (groupId, subCtx) uniquely identifies
 *              a single subscritpion.
 * Parameters:
 *      pLib    - Pointer to lib_globals
 *      eventType- Specific event type or FM_EVT_ALL
 *      sevLevel- Threshold of subscriber sensitivity. It will be notified
 *                of any faults with this or higher level of severity.
 *      groupId - Fault group identifier. It allows to subscribe to faults
 *                of specific group or all faults (FM_GID_ALL);
 *      ntfCb   - Subscriber's notification callback.
 *      subCtx  - Subscriber's context reference. This is a unique subscriber
 *                identification. This MAY NOT be a pointer to lib_globals.
 *
 * Returns:
 *      ZRES_OK
 *      ZRES_ERR
 *----------------------------------------------------------------------------
 */
ZRESULT
fmSubscribe(struct lib_globals *pLib,
            fmEventType_e       eventType,
            fmSeverity_e        sevLevel,
            fmGroupId_e         groupId,
            fmFaultNtfCb_t      ntfCb,
            intptr_t            subCtx)
{
  fmInstance_t *pFmIns=NULL;
  u_int16_t subIx, subCnt, emptyIx=FM_MAX_SUBSCRIPTIONS;
  fmSubscr_t   *pSub=NULL;

  if (pLib==NULL || ntfCb==NULL ||
      (pFmIns = (fmInstance_t *)pLib->lib_fm) == NULL ||
      (groupId >= FM_GID_MAX && groupId != FM_GID_ALL)) {
    return (ZRES_ERR);
  }
  if (pFmIns->fmi_SubTabCnt >= FM_MAX_SUBSCRIPTIONS) {
    return (ZRES_ERR);
  }
  if (eventType > FM_EVT_ALL)
  {
    eventType = FM_EVT_ALL;
  }
  /* Check for duplicate subscriptions.
     Find first empty slot.
     Compansate for the iteration for empty slot.
  */
  subCnt = pFmIns->fmi_SubTabCnt+1;
  for (subIx=0; subIx<FM_MAX_SUBSCRIPTIONS && subCnt>0; subIx++)
  {
    if ((pSub = &pFmIns->fmi_SubTab[subIx])->fms_NtfCb != NULL)
    {
      if (pSub->fms_GroupId == groupId && (pSub->fms_SubCtx == subCtx)) {
        return (ZRES_ERR);
      }
      subCnt--;
    }
    else { /* Take note of the first empty slot. */
      if (emptyIx >= FM_MAX_SUBSCRIPTIONS)
      {
        emptyIx = subIx;
        subCnt--;
      }
    }
  }
  if (emptyIx >= FM_MAX_SUBSCRIPTIONS) {
    return (ZRES_ERR);
  }
  pSub = &pFmIns->fmi_SubTab[emptyIx];

  pSub->fms_EventType= eventType;
  pSub->fms_SevLevel = sevLevel;
  pSub->fms_GroupId  = groupId;
  pSub->fms_NtfCb    = ntfCb;
  pSub->fms_SubCtx   = subCtx;

  pFmIns->fmi_SubTabCnt++;

  return (ZRES_OK);
}

/*----------------------------------------------------------------------------
 * Name:        fmRetrieve
 * Purpose:     To retrieve all recorded faults or only faults for a given
 *              subscriber. The function uses the registered callback(s)
 *              to notify subscriber about recorded faults.
 * Parameters:
 *      pLib    - Pointer to lib_globals
 *      subCtx  - Subscriber's context reference, same as submitted in the
 *                fmSubscribe().
 * Returns:
 *      ZRES_OK
 *      ZRES_ERR
 *----------------------------------------------------------------------------
 */
ZRESULT
fmRetrieve(struct lib_globals *pLib, intptr_t subCtx)
{
  u_int16_t     subCnt, subIx;
  fmSubscr_t   *pSub=NULL;
  fmInstance_t *pFmIns=NULL;
  fmFault_t    *pFault=NULL;
  fmFaultDef_t *pFaultDef=NULL;
  struct listnode *pThisNode=NULL;

  if (pLib==NULL || subCtx==0 ||
      (pFmIns = (fmInstance_t *)pLib->lib_fm) == NULL) {
    return (ZRES_ERR);
  }
  /* Locate the fault on the fault list. */
  LIST_LOOP (&pFmIns->fmi_FaultList, pFault, pThisNode)
  {
    /* Only active faults are reported. */
    if (pFault->flt_State == PAL_FALSE) {
      continue;
    }
    pFaultDef = pFault->flt_pFaultDef;

    /* We may have many subscriptions with the same subscriber context. */
    subCnt = pFmIns->fmi_SubTabCnt;
    for (subIx=0; subIx<FM_MAX_SUBSCRIPTIONS && subCnt>0; subIx++)
    {
      if ((pSub = &pFmIns->fmi_SubTab[subIx]) != NULL)
      {
        subCnt--;
        /* Only subscribers with subCxt. */
        if (subCtx != pSub->fms_SubCtx) {
          continue;
        }
        /* Does it subscribe to this fault's group? */
        if (pSub->fms_GroupId != FM_GID_ALL &&
            pSub->fms_GroupId != pFaultDef->ftd_GroupId) {
          continue;
        }
        if (pSub->fms_EventType != FM_EVT_ALL &&
            pSub->fms_EventType != pFaultDef->ftd_EventType)
        {
          continue;
        }
        /* Check the severity level. */
        if (pSub->fms_SevLevel > pFaultDef->ftd_Severity) {
          continue;
        }
        /* Notify the subscriber about this fault. */
        pSub->fms_NtfCb(subCtx,
                        pFaultDef->ftd_GroupId,
                        pFaultDef->ftd_FaultId,
                        pFaultDef->ftd_EventType,
                        pFaultDef->ftd_Severity,
                        pFaultDef->ftd_Recovery,
                        pFaultDef->ftd_ProbCause,
                        pFault->flt_LastTime,
                        pFault->flt_DataLen,
                        pFault->flt_pData,
                        pFault->flt_FileName,
                        pFault->flt_FileLine);
        /* Go and check the next fault. */
        break;
      }
    }
  }
  return (ZRES_OK);
}

/*----------------------------------------------------------------------------
 * Name:        fmInitFaultRecording
 * Purpose:     To initialize fault recording library.
 * Parameters:
 *      pLib   - Pointer to lib_globals instance.
 * Return:
 *      ZRES_OK
 *      ZRES_ERR
 *----------------------------------------------------------------------------
 */
ZRESULT
fmInitFaultRecording(struct lib_globals *pLib)
{
  fmInstance_t *pFmIns=NULL;

  pFmIns = XCALLOC(MTYPE_FM_LIB, sizeof(fmInstance_t));
  if (pFmIns == NULL) {
    return (ZRES_ERR);
  }
  list_init(&pFmIns->fmi_FaultList, NULL, NULL);

  pLib->lib_fm = pFmIns;

  fmCliInit(pLib);

  return (ZRES_OK);
}


