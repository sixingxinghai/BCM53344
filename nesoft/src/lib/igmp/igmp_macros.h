/* Copyright (C) 2001-2005  All Rights Reserved. */

#ifndef _PACOS_IGMP_MACROS_H
#define _PACOS_IGMP_MACROS_H

/* Macro to obtain Lib-Globals from IGMP Instance */
#define IGMP_LG(INST)               ((INST)->igi_lg)

/* Macro for Starting a Read Thread */
#define IGMP_READ_ON(LIB_GLOB, THREAD, THREAD_ARG,                    \
                     THREAD_FUNC, SOCK_FD)                            \
do {                                                                  \
  if (! (THREAD))                                                     \
    (THREAD) = thread_add_read ((LIB_GLOB), (THREAD_FUNC),            \
                                (THREAD_ARG), (SOCK_FD));             \
} while (0)

/* Macro for Stopping a Read Thread */
#define IGMP_READ_OFF(LIB_GLOB, THREAD)                               \
do {                                                                  \
  if (THREAD)                                                         \
    {                                                                 \
      thread_cancel ((THREAD));                                       \
      (THREAD) = NULL;                                                \
    }                                                                 \
} while (0)

/* Macro for timer turn on */
#define IGMP_TIMER_ON(LIB_GLOB, THREAD, ARG, THREAD_FUNC, TIME_VAL)   \
do {                                                                  \
  if (!(THREAD))                                                      \
    (THREAD) = thread_add_timer ((LIB_GLOB), (THREAD_FUNC),           \
                                 (ARG), (TIME_VAL));                  \
} while (0)

/* Macro for timer timeval turn on */
#define IGMP_TIMER_TIMEVAL_ON(LIB_GLOB, THREAD, ARG, THREAD_FUNC, TIME_VAL)   \
do {                                                                          \
  if (!(THREAD))                                                              \
    (THREAD) = thread_add_timer_timeval ((LIB_GLOB), (THREAD_FUNC),           \
                                 (ARG), (TIME_VAL));                          \
} while (0)

/* Macro for timer turn off */
#define IGMP_TIMER_OFF(LIB_GLOB, THREAD)                              \
do {                                                                  \
  if (THREAD)                                                         \
    {                                                                 \
      thread_cancel (THREAD);                                         \
      (THREAD) = NULL;                                                \
    }                                                                 \
} while (0)

/* Macro to Convert IPv4 Multicast Addr into MAC Addr */
#define IGMP_CONVERT_IPV4MCADDR_TO_MAC(IPV4MCA, MAC)                  \
do {                                                                  \
  ((u_int8_t *) (MAC)) [0] = 0x01;                                    \
  ((u_int8_t *) (MAC)) [1] = 0x00;                                    \
  ((u_int8_t *) (MAC)) [2] = 0x5E;                                    \
  ((u_int8_t *) (MAC)) [3] = (((u_int8_t *) (IPV4MCA)) [1] & 0x7F);   \
  ((u_int8_t *) (MAC)) [4] = ((u_int8_t *) (IPV4MCA)) [2];            \
  ((u_int8_t *) (MAC)) [5] = ((u_int8_t *) (IPV4MCA)) [3];            \
} while (0)

#define IGMP_IF_VLAN_MASK      0xFFF

#define IGMP_IF_GET_VID(SUID)  ((SUID) & IGMP_IF_VLAN_MASK)

/* Macro to Stringize a variable */
#define IGMP_STRINGIZE(VAR)             # VAR
#define IGMP_SUPERSTR(VAR)              IGMP_STRINGIZE (VAR)

/* Macro for Catenatiion a variables */
#define IGMP_CATENIZE(VAR1, VAR2)       VAR1 ## VAR2
#define IGMP_SUPERCAT(VAR1, VAR2)       IGMP_CATENIZE (VAR1, VAR2)

/* Macro for un-referenced function parameter */
#define IGMP_UNREFERENCED_PARAM(PARAM) ((PARAM) = (PARAM))

/* Macro to Extract IGMPv3 defined Encoded Mantissa from 8-bit int */
#define IGMP_EXTRACT_MANT(INT8)                                       \
  ((u_int8_t) ((INT8) & IGMP_MSG_TIME_INTERVAL_MANTISSA_MASK))

/* Macro to Extract IGMPv3 defined Encoded Exponent from 8-bit int */
#define IGMP_EXTRACT_EXPONENT(INT8)                                   \
  ((u_int8_t) (((INT8) & IGMP_MSG_TIME_INTERVAL_EXPONENT_MASK) >>     \
               IGMP_MSG_TIME_INTERVAL_MANTISSA_BITSIZE))

/* Macro to Extract IGMPv3 defined Mantissa from 32-bit integer */
#define IGMP_GET_INT32_MANT(F_INT32)                                  \
  (((u_int8_t)(((*((u_int32_t *)((void *)(&(F_INT32))))) &            \
                IGMP_FLOAT32_SIGNIFICANT_MASK)                        \
               >> (IGMP_FLOAT32_SIGNIFICANT_BIT_SIZE                  \
                   - IGMP_MSG_TIME_INTERVAL_MANTISSA_BITSIZE)))       \
   & IGMP_MSG_TIME_INTERVAL_MANTISSA_MASK)

/* Macro to Extract IGMPv3 defined Exponent from 32-bit integer */
#define IGMP_GET_INT32_EXPONENT(F_INT32)                              \
  (((u_int8_t)((*((u_int32_t *)((void *)(&(F_INT32)))) &              \
                IGMP_FLOAT32_EXPONENT_MASK)                           \
                >> IGMP_FLOAT32_SIGNIFICANT_BIT_SIZE)                 \
    - IGMP_FLOAT32_1SCOMPLEMENT_EXPONENT)                             \
   - IGMP_MSG_TIME_INTERVAL_MANTISSA_BITSIZE                          \
   - IGMP_MSG_TIME_INTERVAL_BASE_EXPONENT)

/* Macro to obtain CLI Range String */
#define IGMP_CLI_RANGE_STR(VAR_MIN, VAR_MAX)                          \
  IGMP_SPACE_STR IGMP_SUPERSTR (<VAR_MIN-)                            \
  IGMP_SUPERSTR (IGMP_SUPERCAT (, VAR_MAX>))

/* Macro to obtain CLI Default Value String */
#define IGMP_CLI_DEF_VAL_STR(DEF_VAL)                                 \
  " (Default: " IGMP_STRINGIZE (DEF_VAL) ")"

/* Macro to convert IGMP Message type to string */
#define IGMP_MSG_TYPE_STR(MSG_TYPE)                                   \
  ((MSG_TYPE) == IGMP_MSG_MEMBERSHIP_QUERY ?                          \
   "IGMP Membership Query" :                                          \
   (MSG_TYPE) == IGMP_MSG_V1_MEMBERSHIP_REPORT ?                      \
   "IGMP V1 Membership Report" :                                      \
   (MSG_TYPE) == IGMP_MSG_V2_MEMBERSHIP_REPORT ?                      \
   "IGMP V2 Membership Report" :                                      \
   (MSG_TYPE) == IGMP_MSG_V3_MEMBERSHIP_REPORT ?                      \
   "IGMP V3 Membership Report" :                                      \
   (MSG_TYPE) == IGMP_MSG_V2_LEAVE_GROUP ?                            \
   "IGMP V2 Leave Group" : "Unkown")

/* Macro to convert IGMP IGR State to string */
#define IGMP_FMS_STR(STATE)                                           \
  ((STATE) == IGMP_FMS_INVALID ? "Invalid" :                          \
   (STATE) == IGMP_FMS_INCLUDE ? "Include" :                          \
   (STATE) == IGMP_FMS_EXCLUDE ? "Exclude" : "Unkown")

/* Macro to convert IGMP IGR Events to string */
#define IGMP_FME_STR(EVENT)                                           \
  ((EVENT) == IGMP_FME_INVALID ? "Invalid" :                          \
   (EVENT) == IGMP_FME_MODE_IS_INCL ? "Mode Is Include" :             \
   (EVENT) == IGMP_FME_MODE_IS_EXCL ? "Mode Is Exclude" :             \
   (EVENT) == IGMP_FME_CHG_TO_INCL ? "Change To Include" :            \
   (EVENT) == IGMP_FME_CHG_TO_EXCL ? "Change To Exclude" :            \
   (EVENT) == IGMP_FME_ALLOW_NEW_SRCS ? "Allow New Sources" :         \
   (EVENT) == IGMP_FME_BLOCK_OLD_SRCS ? "Block Old Sources" :         \
   (EVENT) == IGMP_FME_GROUP_TIMER_EXPIRY ? "Group Tmr Expry" :       \
   (EVENT) == IGMP_FME_SOURCE_TIMER_EXPIRY ? "Source Tmr Expry" :     \
   (EVENT) == IGMP_FME_IMMEDIATE_LEAVE ? "Immediate Leave" :          \
   (EVENT) == IGMP_FME_MANUAL_CLEAR ? "Manual Clear" :                \
   "Unkown")

/* Macro to convert IGMP Host-side FSM State to string */
#define IGMP_HFMS_STR(STATE)                                          \
  ((STATE) == IGMP_HFMS_INVALID ? "Invalid" :                         \
   (STATE) == IGMP_HFMS_INCLUDE ? "Include" :                         \
   (STATE) == IGMP_HFMS_EXCLUDE ? "Exclude" :                         \
   "Unkown")

/* Macro to convert IGMP Host-side FSM Events to string */
#define IGMP_HFME_STR(EVENT)                                          \
  ((EVENT) == IGMP_HFME_INVALID ? "Invalid" :                         \
   (EVENT) == IGMP_HFME_INCL ? "Include" :                            \
   (EVENT) == IGMP_HFME_EXCL ? "Exclude" :                            \
   (EVENT) == IGMP_HFME_MFC_MSG ? "MFC Msg" :                         \
   (EVENT) == IGMP_HFME_MANUAL_CLEAR ? "Manual Clear" :               \
   "Unkown")

/*
 * IGMP SMP Function Macros
 */
#define IGMP_FN_DESC(FN_DESC)                                         \
    (# FN_DESC == "" ? __FUNCTION__ : # FN_DESC)
#ifdef HAVE_IGMP_DEBUG
#define IGMP_DBG_FN_DESC                dbg_fn_desc
#ifdef HAVE_ISO_MACRO_VARARGS
#define IGMP_FN_ENTER(...)                                            \
    u_int8_t IGMP_DBG_FN_DESC [IGMP_DBG_FN_DESC_MAX];                 \
    pal_strncpy (IGMP_DBG_FN_DESC, IGMP_FN_DESC (__VA_ARGS__),        \
                 IGMP_DBG_FN_DESC_MAX - 1);                           \
    IGMP_DBG_FN_DESC [IGMP_DBG_FN_DESC_MAX - 1] = '\0';
#define IGMP_FN_EXIT(...)                                             \
    IGMP_DBG_FN_DESC [0] = '\0';                                      \
    return __VA_ARGS__;
#else
#define IGMP_FN_ENTER(ARGS...)                                        \
    u_int8_t IGMP_DBG_FN_DESC [IGMP_DBG_FN_DESC_MAX];                 \
    pal_strncpy (IGMP_DBG_FN_DESC, IGMP_FN_DESC (ARGS),               \
                 IGMP_DBG_FN_DESC_MAX - 1);                           \
    IGMP_DBG_FN_DESC [IGMP_DBG_FN_DESC_MAX - 1] = '\0';
#define IGMP_FN_EXIT(ARGS...)                                         \
    IGMP_DBG_FN_DESC [0] = '\0';                                      \
    return ARGS;
#endif /* HAVE_ISO_MACRO_VARARGS */
#else
#define IGMP_DBG_FN_DESC                ""
#define IGMP_FN_ENTER(FN_DESC)
#ifdef HAVE_ISO_MACRO_VARARGS
#define IGMP_FN_EXIT(...)                                             \
    return __VA_ARGS__;
#else
#define IGMP_FN_EXIT(ARGS...)                                         \
    return ARGS;
#endif /* HAVE_ISO_MACRO_VARARGS */
#endif /* HAVE_IGMP_DEBUG */

/*
 * IGMP Debug Macros
 */
#define IGMP_DEBUG_CONF_ON(INST, VAR)                                 \
    (INST->igi_conf_dbg_flags |= IGMP_INST_DBG_ ## VAR)

#define IGMP_DEBUG_CONF_OFF(INST, VAR)                                \
    (INST->igi_conf_dbg_flags &= ~(IGMP_INST_DBG_ ## VAR))

#define IGMP_DEBUG_TERM_ON(INST, VAR)                                 \
    (INST->igi_term_dbg_flags |= IGMP_INST_DBG_ ## VAR)

#define IGMP_DEBUG_TERM_OFF(INST, VAR)                                \
    (INST->igi_term_dbg_flags &= ~(IGMP_INST_DBG_ ## VAR))

#define IGMP_DEBUG(INST, VAR)                                         \
    (INST->igi_term_dbg_flags & IGMP_INST_DBG_ ## VAR)

#define IGMP_DEBUG_CONF(INST, VAR)                                    \
    (INST->igi_conf_dbg_flags & IGMP_INST_DBG_ ## VAR)

#ifdef HAVE_ISO_MACRO_VARARGS
#define IGMP_DBG_INFO(INST, SUBMOD, FMT, ...)                         \
do {                                                                  \
  if (IGMP_DEBUG (INST, SUBMOD))                                      \
    zlog_info (IGMP_LG (INST), "[IGMP-" # SUBMOD "] " "%s%s" FMT,     \
               IGMP_DBG_FN_DESC, ": " , ##__VA_ARGS__);               \
} while (0)
#define IGMP_DBG_WARN(INST, SUBMOD, FMT, ...)                         \
do {                                                                  \
  zlog_warn (IGMP_LG (INST), "[IGMP-" # SUBMOD "] " "%s%s" FMT,       \
             IGMP_DBG_FN_DESC, ": " , ##__VA_ARGS__);                 \
} while (0)
#define IGMP_DBG_ERR(INST, SUBMOD, FMT, ...)                          \
do {                                                                  \
  zlog_err (IGMP_LG (INST), "[IGMP-" # SUBMOD "] " "%s%s" FMT,        \
            IGMP_DBG_FN_DESC, ": " , ##__VA_ARGS__);                  \
} while (0)
#else
#define IGMP_DBG_INFO(INST, SUBMOD, FMT, ARGS...)                     \
do {                                                                  \
  if (IGMP_DEBUG (INST, SUBMOD))                                      \
    zlog_info (IGMP_LG (INST), "[IGMP-" # SUBMOD "] " "%s%s" FMT,     \
               IGMP_DBG_FN_DESC, ": " , ##ARGS);                      \
} while (0)
#define IGMP_DBG_WARN(INST, SUBMOD, FMT, ARGS...)                     \
do {                                                                  \
  zlog_warn (IGMP_LG (INST), "[IGMP-" # SUBMOD "] " "%s%s" FMT,       \
             IGMP_DBG_FN_DESC, ": " , ##ARGS);                        \
} while (0)
#define IGMP_DBG_ERR(INST, SUBMOD, FMT, ARGS...)                      \
do {                                                                  \
  zlog_err (IGMP_LG (INST), "[IGMP-" # SUBMOD "] " "%s%s" FMT,        \
            IGMP_DBG_FN_DESC, ": " , ##ARGS);                         \
} while (0)
#endif /* HAVE_ISO_MACRO_VARARGS */

#ifdef HAVE_SNMP
#ifdef HAVE_ISO_MACRO_VARARGS
#define IGMP_SNMP_GET(EXACT, FUNC, ...)                               \
   ((EXACT) ?                                                         \
    (igmp_if_ ## FUNC ## _get (__VA_ARGS__) == IGMP_ERR_NONE) :       \
    (igmp_if_ ## FUNC ## _get_next (__VA_ARGS__) == IGMP_ERR_NONE))
#else
#define IGMP_SNMP_GET(EXACT, FUNC, ARGS...)                           \
   ((EXACT) ?                                                         \
    (igmp_if_ ## FUNC ## _get (ARGS) == IGMP_ERR_NONE) :              \
    (igmp_if_ ## FUNC ## _get_next (ARGS) == IGMP_ERR_NONE))
#endif /* HAVE_ISO_MACRO_VARARGS */
#endif /* HAVE_SNMP */

#endif /* _PACOS_IGMP_MACROS_H */
