/* Copyright (C) 2004  All Rights Reserved. */

#ifndef  _HSL_LOG_MODULE
#define  _HSL_LOG_MODULE

#include "hsl_oss.h"
#include "hsl_logs.h"

/***************************************************************************
 * To preserve system resources and in order to keep ONLY relevant logs,   * 
 * logs are throttled. We keep only up to max logs per unit of time        *
 ***************************************************************************/                                                                            
#define HSL_LOG_TIME_UNIT            (15) /* 15 seconds          */         
#define HSL_LOG_MAX_PER_UNIT        (750) /* 750 logs per 15 sec */ 
   
/*
 * Log level 
 */

#define HSL_LEVEL_INFO         (5)    /* General system information,profiling                                          */
#define HSL_LEVEL_DEBUG        (4)    /* Debug trace (For example: Function calls, rx/tx packet trace).                */
#define HSL_LEVEL_WARN         (3)    /* Warnings  (For example: System running out of resorces or miss configuration).*/
#define HSL_LEVEL_ERROR        (2)    /* Recoverable error (For example, configuration command failure).               */   
#define HSL_LEVEL_FATAL        (1)    /* Fatal failure occured process/system has to terminate/reboot.                 */
#define HSL_LEVEL_ADMIN        (0)    /* Administrative critical information (For example: New hw modules insertion ). */

#define HSL_LEVEL_DEFAULT      HSL_LEVEL_WARN 

/*
 * Modules 
 */
enum hsl_modules {
    HSL_MODULE_ALL = 0,   /* All modules - like shared libraries.*/
    HSL_LOG_GENERAL,      /* General logs.                       */
    HSL_LOG_IFMGR,        /* Interface management logs.          */
    HSL_LOG_BRIDGE,       /* Bridge management logs.             */
    HSL_LOG_MSG,          /* HAL-HSL message logs.               */
    HSL_LOG_FIB,          /* FIB management.                     */
    HSL_LOG_FDB,          /* FDB management.                     */
    HSL_LOG_DEVDRV,       /* Device Driver logs.                 */
    HSL_LOG_PKTDRV,       /* Packet driver logs.                 */
    HSL_LOG_PLATFORM,     /* Platform(Hardware) logs.            */
    HSL_LOG_GEN_PKT,      /* Generic packet trace.               */
    HSL_LOG_BPDU_PKT,     /* BPDU packet trace.                  */
    HSL_LOG_ARP_PKT,      /* ARP  packet trace.                  */
    HSL_LOG_IGMP_PKT,     /* IGMP packet trace.                  */
    HSL_LOG_LACP,         /* LACP Configuration                  */
    HSL_LOG_LAST_MODULE   /* Last module.                        */
};

typedef struct __hsllog_details {
   u_int16_t module_id;                  /* Module name.                          */
   u_int16_t level;                      /* Log level.                            */
   u_int16_t max_logs_per_unit;          /* Maximum number of logs per time unit. */
   u_int16_t log_time_unit;              /* Log throttling time unit.             */
   unsigned long  unit_start;                 /* Last unit start time.                 */
   u_int32_t   log_count;                  /* Current log count.                    */
} hsllog_detail_t;

/************************************************************
 * hsl_log - Service routine to log debug/error information *
 * Parameters:                                              *
 *     module  - logging module name                        * 
 *     level   - error level                                * 
 *     format  - data                                       * 
 * Returns:                                                 *
 *  0  Log posted OK                                        *
 *  -1 Error occured                                        *
 ************************************************************/
int hsl_log(u_int16_t module, u_int16_t level, const char* format, ...);

/************************************************************
 * hsl_log_throttle - Service routine to throttle logs      *
 * Parameters:                                              *
 *     module  - logging module name                        * 
 * Returns:                                                 *
 *  0  print log                                            *
 *  -1 drop log                                             *
 ************************************************************/
int hsl_log_throttle(u_int16_t module);
 
/************************************************************
 * hsl_log_conf - Service routine to config log level     *
 * Parameters:                                              *
 *     module  - logging module name                        * 
 *     level   - error level                                * 
 * Returns:                                                 *
 *  0  config ok                                            *
 *  -1 Error occured                                        *
 ************************************************************/
int hsl_log_conf(u_int16_t module, u_int16_t level);

extern u_int32_t hsl_log_info;


/*
  Log macro for readability.
*/
#ifdef HAVE_ISO_MACRO_VARARGS
#define HSL_LOG(M,L,...)                                        \
    do {                                                        \
         if (hsl_log_info)                                      \
            hsl_log(M,L,__VA_ARGS__);                           \
    } while (0)
#else
#define HSL_LOG(M,L,ARGS...)                                    \
    do {                                                        \
         if (hsl_log_info)                                      \
            hsl_log(M,L,ARGS);                                  \
    } while (0)
#endif /* ! HAVE_ISO_MACRO_VARARGS */


#define PKT_DUMP(A,B)                                      \
    do {                                                   \
      int i = 0, col = 0;                                  \
      static int count = 0;                                \
                                                           \
      HSL_LOG(HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "\nPacket %d, size %d:\n", ++count, (B));    \
      while (i < (B))                                      \
       {                                                   \
         HSL_LOG(HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "%02x ", A[i]);                           \
         i++;                                              \
         if (++col == 16)                                  \
          {                                                \
         HSL_LOG(HSL_LOG_IFMGR, HSL_LEVEL_ERROR, "\n");                           \
            col = 0;                                       \
          }                                                \
       }                                                   \
    } while (0)

#endif  /* _HSL_LOG_MODULE */
