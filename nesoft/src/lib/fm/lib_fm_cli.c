
/*--------------------------------------------------------------------------
 * Copyright (C) 2008 All rights reserved
 *
 * File name:   lib_fm_cli.c
 * Purpose:     To provide implementation of Fault Management - Fault Recording
 *              CLI commands.
 * Author:      STC
 * Data:        04/10/2008
 *----------------------------------------------------------------------------
 */

#include "pal.h"
#include "timeutil.h"
#include "lib_fm.h"
/* #include "lib_fm_gid.h" */

#define LIB_FM_TEST 1

extern char *fm_sev_names[];
extern char *fm_evt_names[];
extern char *fm_rec_names[];


/*-----------------------------------------------------------------------
 * This should serve as an example of standardized API show structures.
 * The "path" struture is not defined as there is no nesting here.
 * These two structures shall be derrived directly from XML PacOS data
 * model.
 * _fmShowFaultsGetNext is just the example of typical API getNext operation.
 * It returns one fault at a time.
 */
typedef struct fmShowFaultsPath
{
} fmShowFaultsPath_t;

typedef struct fmShowFaultsElem
{
          /* Show params */
  char               fsd_ProcName[LIB_MAX_PROG_NAME+1];
  fmCmdScope_e       fsd_Scope;

  char               fsd_GroupName[FM_MAX_GROUP_NAME+1];
  u_int16_t          fsd_FaultId;
  u_int32_t          fsd_DataLen;
  u_int8_t          *fsd_pData;
  fmEventType_e      fsd_EventType;
  fmSeverity_e       fsd_Severity;
  fmRecovery_e       fsd_Recovery;
  fmProbableCause_e  fsd_ProbCause;
  char               fsd_FaultName[FM_MAX_FAULT_NAME+1];
  char               fsd_FileName[32];
  u_int32_t          fsd_FileLine;
  bool_t             fsd_State;
  u_int32_t          fsd_Cnt;
  u_int32_t          fsd_SeqNum;
  struct pal_timeval fsd_FirstTime;
  struct pal_timeval fsd_LastTime;
  struct pal_timeval fsd_ClearTime;
} fmShowFaultsElem_t;


/*--------------------------------------------------------------------------
 *                              SHOW FUNCTIONS
 *--------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------
 * Name:        _DumpData
 * Purpose:     To dump to the terminal screen opaque fault data.
 *----------------------------------------------------------------------------
 */
void
_DumpData(struct cli *pCli, u_int8_t *data, u_int16_t len, u_int8_t offset)
{
  int ix, printLen;
  char ins[offset+1];

  pal_mem_set(ins,' ',offset);
  /* No more than 64 bytes */
  printLen = (len > 64) ? 64 : len;
  for (ix=0; ix<printLen; ix++)
  {
    if ((ix & 0xF) == 0) {
      cli_out(pCli, "%s%02x: ", ins, ix);
    }
    cli_out(pCli, " %02x", data[ix]);
    if ((ix & 0xF) == 0xF) {
      cli_out(pCli, "\n");
    }
  }
  if ((ix & 0xF) != 0) {
    cli_out(pCli, "\n");
  }
}

/*------------------------------------------------------------
 * _fmShowFaultsGetNext - Get next fault on the fault list
 *
 * Purpose: To retrieve next element based on the previous
 *          context
 * Params:
 *      pLib    - Ptr tolib_globals
 *      pPath   - Ptr to context structure (not used here)
 *      pElem   - Ptr to element data
 *      pLocCtxt- Ptr to variable where to return context
 *                reference value. It will help to locate the
 *                last fault and retrieve the next one in subsequent
 *                call to this function.
 *                When starting from beigining this value should be 0.
 * Returns:
 *      ZRES_OK      - An element retrieved and returned
 *                       Caller may come back for next element.
 *      ZRES_NO_MORE - No element returned - do not come back
 *      ZRES_ERR     - Error detected - do not come back
 */
static ZRESULT
_fmShowFaultsGetNext(struct lib_globals *pLib,
                     fmShowFaultsPath_t *pPath,
                     fmShowFaultsElem_t *pElem,
                     intptr_t *pLocCtxt)
{
  fmInstance_t *pFmIns;
  fmFault_t    *pFault;
  fmFaultDef_t *pFaultDef;
  struct listnode *pPrevNode=(struct listnode *)(* pLocCtxt);
  struct listnode *pThisNode;
  bool_t found=PAL_FALSE;

  if ((pFmIns = pLib->lib_fm) == NULL) {
    return (ZRES_ERR);
  }
  /* Find the next fault of interest. */
  do
  {
    if (pPrevNode == NULL) {
      pThisNode = LISTHEAD(&pFmIns->fmi_FaultList);
    }
    else  {
      pThisNode = NEXTNODE(pPrevNode);
    }
    pFault = GETDATA(pThisNode);
    pPrevNode = pThisNode;

    if (pFault != NULL)
    {
      switch (pElem->fsd_Scope)
      {
      case FM_SCOPE_ALL:
        break;
      case FM_SCOPE_ACTIVE:
        if (pFault->flt_State != PAL_TRUE) {
          continue;
        }
        found = PAL_TRUE;
        break;
      case FM_SCOPE_CLEARED:
        if (pFault->flt_State != PAL_FALSE) {
          continue;
        }
        break;
      default:
        return (ZRES_ERR);
      }
      found = PAL_TRUE;
    }
  } while (!found && pThisNode!=NULL);

  *pLocCtxt = (intptr_t)pThisNode;

  if (pThisNode == NULL || pFault == NULL) {
    return (ZRES_NO_MORE);
  }
  /* pFault must be assigned: Copy data */
  pFaultDef = pFault->flt_pFaultDef;

  pal_strncpy(pElem->fsd_GroupName,
              pFmIns->fmi_GrpRefTab[pFaultDef->ftd_GroupId]->fgr_Name,
              sizeof(pElem->fsd_GroupName));
  pElem->fsd_FaultId    = pFaultDef->ftd_FaultId;
  pElem->fsd_DataLen    = pFault->flt_DataLen;
  pElem->fsd_pData      = pFault->flt_pData;
  pElem->fsd_EventType  = pFaultDef->ftd_EventType;
  pElem->fsd_Severity   = pFaultDef->ftd_Severity;
  pElem->fsd_Recovery   = pFaultDef->ftd_Recovery;
  pElem->fsd_ProbCause  = pFaultDef->ftd_ProbCause;
  pal_strncpy(pElem->fsd_FaultName, pFaultDef->ftd_Name,
              sizeof(pElem->fsd_FaultName));

  pElem->fsd_State      = pFault->flt_State;
  pElem->fsd_Cnt        = pFault->flt_Cnt;
  pElem->fsd_SeqNum     = pFault->flt_SeqNum;
  pElem->fsd_FirstTime  = pFault->flt_FirstTime;
  pElem->fsd_LastTime   = pFault->flt_LastTime;
  pElem->fsd_ClearTime  = pFault->flt_ClearTime;
  pal_strncpy(pElem->fsd_FileName,
              pFault->flt_FileName,
              sizeof(pElem->fsd_FileName));
  pElem->fsd_FileLine   = pFault->flt_FileLine;

  return (ZRES_OK);
}

/*-----------------------------------------------------------------------
 * _fmShowFaults - Show "faults" CLI handler
 *
 * Purpose: To display faults based on requried criteria
 *
 * Params:
 *      pLib    - Ptr to lib_globals
 *      cmdScope- Scope of teh command (all, active or cleared)
 *      pProcName- Ptr to process name - when directed to one process only
 *      pCli    - Ptr to CLI structure
 */
static ZRESULT
_fmShowFaults (struct lib_globals *pLib,
               fmCmdScope_e        cmdScope,
               char               *pProcName,
               struct cli         *pCli)
{
  char       timeStr[64];
  fmShowFaultsElem_t fltElem;
  intptr_t   locCtxt=0;

  if (pProcName != NULL)
  {
    if (pal_strcmp(pCli->zg->progname, pProcName)!= 0) {
      return (ZRES_OK);
    }
  }
  cli_out(pCli, "\nFaults detected in process: %s\n", pCli->zg->progname);

  /* Copy the show parameters. */
  pal_strncpy(fltElem.fsd_ProcName, pCli->zg->progname,
              sizeof(fltElem.fsd_ProcName));
  fltElem.fsd_Scope = cmdScope;

  while (_fmShowFaultsGetNext(pCli->zg, NULL, &fltElem, &locCtxt) == ZRES_OK)
  {
    cli_out(pCli, "\n    %s:%s (%d)\n",
            fltElem.fsd_GroupName,
            fltElem.fsd_FaultName,
            fltElem.fsd_FaultId);
    cli_out(pCli, "    Event:%s  Sev:%s  Rec:%s  Cause:%d\n",
            fm_evt_names[fltElem.fsd_EventType],
            fm_sev_names[fltElem.fsd_Severity],
            fm_rec_names[fltElem.fsd_Recovery],
            fltElem.fsd_ProbCause);
    cli_out(pCli, "    State:%s Count:%d  SeqNum:%d  Loc:%s:%d\n",
            fltElem.fsd_State ? "Active " : "Cleared",
            fltElem.fsd_Cnt,
            fltElem.fsd_SeqNum,
            fltElem.fsd_FileName,
            fltElem.fsd_FileLine);

    pal_time_calendar (&fltElem.fsd_FirstTime.tv_sec, timeStr);
    timeStr[strlen(timeStr)-6]=0;
    cli_out(pCli, "    First:%s ::%lu [us]\n", timeStr, fltElem.fsd_FirstTime.tv_usec);
    if (fltElem.fsd_LastTime.tv_sec != 0)
    {
      pal_time_calendar (&fltElem.fsd_LastTime.tv_sec, timeStr);
      timeStr[strlen(timeStr)-6]=0;
      cli_out(pCli, "    Last :%s ::%lu [us]\n", timeStr, fltElem.fsd_LastTime.tv_usec);
    }
    if (fltElem.fsd_ClearTime.tv_sec != 0)
    {
      pal_time_calendar (&fltElem.fsd_ClearTime.tv_sec, timeStr);
      timeStr[strlen(timeStr)-6]=0;
      cli_out(pCli, "    Clear:%s ::%lu [us]\n", timeStr, fltElem.fsd_ClearTime.tv_usec);
    }
    cli_out(pCli, "    DataLen:%d  DataPtr:%p\n", fltElem.fsd_DataLen, fltElem.fsd_pData);
    if (fltElem.fsd_DataLen != 0)
    {
      _DumpData(pCli, fltElem.fsd_pData, fltElem.fsd_DataLen, 6);
    }
    cli_out(pCli, "\n");
  }
  return (ZRES_OK);
}


CLI (fm_show_faults,
     fm_show_faults_cmd,
     "show faults (all|active|cleared) (WORD|)",
     "Show command",
     "Show recorded faults",
     "All faults - including cleared",
     "Active only",
     "Cleared only",
     "Process name")
{
  /* Decode show command parameters. */

  fmCmdScope_e cmdScope;

  if (strcmp(argv[0], "all")==0) {
    cmdScope = FM_SCOPE_ALL;
  }
  else if (strcmp(argv[0], "active")==0) {
    cmdScope = FM_SCOPE_ACTIVE;
  }
  else {
    cmdScope = FM_SCOPE_CLEARED;
  }
  /* Invoke show formatting function. */
  _fmShowFaults(cli->zg, cmdScope, argv[1], cli);
  return (CLI_SUCCESS);
}

/*----------------------------------------------------------------------------
 * Name:        _fmDeleteFaults
 *
 * Purpose:     To clear currently active faults
 *
 * Params:
 *      pLib     - Ptr to lib_globals
 *      cmdScope - Scope of the command (all, active or cleared)
 *      pProcName- Ptr to process name - when directed to one process only
 *      pCli     - Ptr to CLI structure
 *
 * Return:
 *      ZRES_OK
 *      ZRES_ERR
 *----------------------------------------------------------------------------
 */
static ZRESULT
_fmDeleteFaults (struct lib_globals *pLib,
                 fmCmdScope_e cmdScope,
                 char *pProcName,
                 struct cli *pCli)
{
  fmInstance_t *pFmIns;
  fmFault_t    *pFault;
  struct listnode *pThisNode, *pNextNode;

  if (pProcName != NULL) {
    if (pal_strcmp(pCli->zg->progname, pProcName)!= 0) {
      return ZRES_OK;
    }
  }
  /* FM instance must be initialized */
  if ((pFmIns = pCli->zg->lib_fm) == NULL) {
    return ZRES_OK;
  }
  LIST_LOOP_DEL(&pFmIns->fmi_FaultList, pFault, pThisNode, pNextNode)
  {
    switch (cmdScope)
    {
    case FM_SCOPE_ALL:
      break;
    case FM_SCOPE_ACTIVE:
      if (pFault->flt_State != PAL_TRUE) {
        continue;
      }
      break;
    case FM_SCOPE_CLEARED:
      if (pFault->flt_State != PAL_FALSE) {
        continue;
      }
      break;
    default:
      return (ZRES_ERR);
    }
    list_delete_node(&pFmIns->fmi_FaultList, pThisNode);
    XFREE(MTYPE_FM_LIB, pFault);
  }
  return ZRES_OK;
}

CLI (fm_faults_delete,
     fm_faults_delete_cmd,
     "faults delete (all|active|cleared) (WORD|)",
     "Fault management command",
     "Fault delete command",
     "Delete all faults - including cleared",
     "Delete active only",
     "Delete cleared only",
     "Process name")
{
  fmCmdScope_e cmdScope;
  int rc;

  if (strcmp(argv[0], "all")==0) {
    cmdScope = FM_SCOPE_ALL;
  }
  else if (strcmp(argv[0], "active")==0) {
    cmdScope = FM_SCOPE_ACTIVE;
  }
  else {
    cmdScope = FM_SCOPE_CLEARED;
  }
  rc = _fmDeleteFaults(cli->zg, cmdScope, argv[1], cli);
  if (rc != ZRES_OK)
  {
    return (CLI_ERROR);
  }
  return (CLI_SUCCESS);
}

CLI (fm_show_proc_names,
     fm_show_proc_names_cmd,
     "show proc-names",
     "Show command",
     "Show process names")
{
  cli_out(cli, "%s\n", cli->zg->progname);
  return (CLI_SUCCESS);
}

#ifdef LIB_FM_TEST

typedef enum libFmTestFaults
{
  LIB_FM_TST_UNKN=0,
  LIB_FM_TST_WARN=1,
  LIB_FM_TST_MINOR=2,
  LIB_FM_TST_MAJOR=3,
  LIB_FM_TST_CRIT=4,
} libFmTestFaults_e;


fmFaultReg_t iibFmTestRegTab[] =
{
  FM_DEF_COM_WARN_RECNONE (LIB_FM_TST_UNKN, "FaultRecTest/Evt=COM/Sev=Unknown/Rec=None",
                           FM_PC_UNEXPECTED_INFORMATION),
  FM_DEF_COM_WARN_RECNONE (LIB_FM_TST_WARN, "FaultRecTest/Evt=COM/Sev=Warning/Rec=None",
                           FM_PC_UNEXPECTED_INFORMATION),
  FM_DEF_COM_MINOR_RECNONE(LIB_FM_TST_MINOR,"FaultRecTest/Evt=COM/Sev=Minor/Rec=None",
                           FM_PC_UNEXPECTED_INFORMATION),
  FM_DEF_COM_MAJOR_RECNONE(LIB_FM_TST_MAJOR,"FaultRecTest/Evt=COM/Sev=Major/Rec=None",
                           FM_PC_UNEXPECTED_INFORMATION),
  FM_DEF_PRO_CRIT_COMPRST (LIB_FM_TST_CRIT, "FaultRecTest/Evt=PRO/Sev=CRIT/Rec=CompRst",
                           FM_PC_UNEXPECTED_INFORMATION)
};


static ZRESULT
_fmTestInit(struct lib_globals *pLib)
{
  return fmRegisterGroup(pLib,
                         FM_GID_FREC,
                         "FREC",
                         iibFmTestRegTab,
                         FM_NUM_REG_FAULTS(iibFmTestRegTab));
}


CLI (fm_faults_test,
     fm_faults_test_cmd,
     "faults test (raise|clear) WORD (WORD|)",
     "Fault management command",
     "Fault test command",
     "Raise a fault",
     "Clear fault",
     "Fault identifier",
     "Fault instance")
{
  int faultId = pal_atoi(argv[1]);

  if (argc>2 && argv[2]!=0)
  {
    if (strcmp(argv[0], "raise")==0) {
      FM_REPORT_EXT(cli->zg, FM_GID_FREC, faultId, strlen(argv[2]), argv[2]);
    }
    else {
      FM_CLEAR_EXT(cli->zg, FM_GID_FREC, faultId, strlen(argv[2]), argv[2]);
    }
  }
  else
  {
    if (strcmp(argv[0], "raise")==0) {
      FM_REPORT(cli->zg, FM_GID_FREC, faultId);
    }
    else {
      FM_CLEAR_EXT(cli->zg, FM_GID_FREC, faultId, 0, 0);
    }
  }
  return (CLI_SUCCESS);
}

static void
_fmFaultNtfCb(intptr_t       subCtx,
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
              u_int32_t      lineNum)
{
  fmInstance_t *pFmIns = (fmInstance_t *)subCtx;

  printf("\nFault notification >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

  printf("%s \"%s\" Gr:%s Id:%d Len:%d Ptr:%p Evt:%s Sev:%s Rec:%s File:%s Line:%u Cause:%u\n",
         severity == FM_SEV_CLEAR ? "CLEARED" : "RAISED",
         pFmIns->fmi_GrpRefTab[grpId]->fgr_pFaultsDef[faultId].ftd_Name,
         pFmIns->fmi_GrpRefTab[grpId]->fgr_Name,
         faultId,
         dataLen,
         pData,
         fm_evt_names[eventType],
         fm_sev_names[severity],
         fm_rec_names[recovery],
         pFileName,
         lineNum,
         probCause);
  printf("\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");

}


#endif

/*----------------------------------------------------------------------------
 * Name:        fmCliInit
 * Purpose:     To initialize fault recording library.
 * Parameters:
 *      pLib       - Pointer to lib_globals instance.
 * Return:
 *      ZRES_OK
 *      ZRES_ERR
 *----------------------------------------------------------------------------
 */
ZRESULT
fmCliInit(struct lib_globals *pLib)
{
  struct cli_tree *ctree = pLib->ctree;

  pal_assert(ctree!=0);

  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, CLI_FLAG_MCAST, &fm_show_faults_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, CLI_FLAG_MCAST, &fm_faults_delete_cmd);
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, CLI_FLAG_MCAST, &fm_show_proc_names_cmd);

#ifdef LIB_FM_TEST
  cli_install_gen (ctree, EXEC_MODE, PRIVILEGE_MAX, CLI_FLAG_MCAST, &fm_faults_test_cmd);
  _fmTestInit(pLib);
  fmSubscribe(pLib, FM_EVT_ALL, FM_SEV_CLEAR, FM_GID_ALL, _fmFaultNtfCb, (intptr_t)pLib->lib_fm);
#endif

  return (ZRES_OK);
}


