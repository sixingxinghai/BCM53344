/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"

#include "hsl_types.h"
#include "hsl_logger.h"

static hsllog_detail_t hsl_log_detail[] = {
  /*   module_id            level               max_logs            unit_in_sec   unit_start log_count */
  {HSL_MODULE_ALL,    HSL_LEVEL_DEFAULT ,HSL_LOG_MAX_PER_UNIT,HSL_LOG_TIME_UNIT,     0,     0     },
  {HSL_LOG_GENERAL,   HSL_LEVEL_DEFAULT ,HSL_LOG_MAX_PER_UNIT,HSL_LOG_TIME_UNIT,     0,     0     },
  {HSL_LOG_MSG,       HSL_LEVEL_DEFAULT ,HSL_LOG_MAX_PER_UNIT,HSL_LOG_TIME_UNIT,     0,     0     },
  {HSL_LOG_IFMGR,     HSL_LEVEL_DEFAULT ,HSL_LOG_MAX_PER_UNIT,HSL_LOG_TIME_UNIT,     0,     0     },
  {HSL_LOG_FIB,       HSL_LEVEL_DEFAULT ,HSL_LOG_MAX_PER_UNIT,HSL_LOG_TIME_UNIT,     0,     0     },
  {HSL_LOG_DEVDRV,    HSL_LEVEL_DEFAULT ,HSL_LOG_MAX_PER_UNIT,HSL_LOG_TIME_UNIT,     0,     0     },
  {HSL_LOG_PKTDRV,    HSL_LEVEL_DEFAULT ,HSL_LOG_MAX_PER_UNIT,HSL_LOG_TIME_UNIT,     0,     0     },
  {HSL_LOG_PLATFORM,  HSL_LEVEL_DEFAULT ,HSL_LOG_MAX_PER_UNIT,HSL_LOG_TIME_UNIT,     0,     0     },
  {HSL_LOG_GEN_PKT,   HSL_LEVEL_DEFAULT ,HSL_LOG_MAX_PER_UNIT,HSL_LOG_TIME_UNIT,     0,     0     },
  {HSL_LOG_BPDU_PKT,  HSL_LEVEL_DEFAULT ,HSL_LOG_MAX_PER_UNIT,HSL_LOG_TIME_UNIT,     0,     0     },
  {HSL_LOG_ARP_PKT,   HSL_LEVEL_DEFAULT ,HSL_LOG_MAX_PER_UNIT,HSL_LOG_TIME_UNIT,     0,     0     },
  {HSL_LOG_IGMP_PKT,  HSL_LEVEL_DEFAULT ,HSL_LOG_MAX_PER_UNIT,HSL_LOG_TIME_UNIT,     0,     0     },
  {HSL_LOG_LACP,      HSL_LEVEL_DEFAULT ,HSL_LOG_MAX_PER_UNIT,HSL_LOG_TIME_UNIT,     0,     0     }
};

/* u_int32_t hsl_log_info = 0; */
u_int32_t hsl_log_info = 1;

/***********************************************************
 * hsl_enable_log - Enable global logging                  *
 * Parameters : None                                       *
 * Returns : None                                          *
 ***********************************************************/
void
hsl_enable_log (void)
{
  hsl_log_info = 1;
}

/***********************************************************
 * hsl_disable_log - Enable global logging                  *
 * Parameters : None                                       *
 * Returns : None                                          *
 ***********************************************************/
void
hsl_disable_log (void)
{
  hsl_log_info = 0;
}

/************************************************************
 * hsl_add_log_level - Service routine to add log level     *
 * Parameters:                                              *
 *     buf - log buffer                                    * 
 *     level   - error level                                * 
 * Returns:                                                 *  
 *     length of level description                          *  
 ************************************************************/
int 
hsl_add_log_level(char *buf,u_int16_t level) {

  if(NULL == buf)
    return 0; 

  switch (level) {
  case HSL_LEVEL_INFO:       
    sprintf(buf,"%s","INFO: ");
    break; 
  case HSL_LEVEL_DEBUG:        
    sprintf(buf,"%s","DEBUG: ");
    break;
  case HSL_LEVEL_WARN:      
    sprintf(buf,"%s","WARNING: ");
    break;
  case HSL_LEVEL_ERROR:     
    sprintf(buf,"%s","ERROR: ");
    break;
  case HSL_LEVEL_FATAL:     
    sprintf(buf,"%s","FATAL ERROR: ");
    break;
  case HSL_LEVEL_ADMIN:   
    sprintf(buf,"%s","ADMIN INFO: ");
    break;
  default: 
    sprintf(buf,"%s","INFO: ");
  }
  return (strlen(buf));
}
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
int hsl_log(u_int16_t module, u_int16_t level, const char* format, ...)
{
  char buf[OSS_LOG_BUFFER_SIZE];
  va_list args;
  int length; 

#if 0   
  /* Validate module id */
  if(module >= HSL_LOG_LAST_MODULE)
    return -1;

  /* Check if message important enough to log */     
  if(hsl_log_detail[module].level < level) 
    return 0;  
 
  /* Throttle logs  */
  if(0 > hsl_log_throttle(module))
    return 0;
#endif

  /* Add log level */ 
  length = hsl_add_log_level(buf,level);                      
  
  /* Get the message */
  va_start(args, format);
  length = vsprintf(buf + length, format, args);
  if( length  > OSS_LOG_BUFFER_SIZE)
    {
      /* We better reboot at this point, 
       * because memory overwrite has happened 
       */
    } 
  va_end(args);
  /*oss_printf("%s",buf);*/
#ifdef __KERNEL__  //modify by beta.guo 2011-12-15 for for --enable-user-hsl
    printk(KERN_DEBUG "%s", buf);
#else
    printf("%s", buf);
#endif
  return 0;
}

/************************************************************
 * hsl_log_throttle - Service routine to throttle logs      *
 * Parameters:                                              *
 *     module  - logging module name                        * 
 * Returns:                                                 *
 *  0  print log                                            *
 *  -1 drop log                                             *
 ************************************************************/
int hsl_log_throttle(u_int16_t module)
{
#if 0
  struct timeval current_time;
  unsigned long time_diff;
  hsllog_detail_t *log_ptr;
  
  /* Validate module id */
  if(module >= HSL_LOG_LAST_MODULE)
    return -1;

  log_ptr = hsl_log_detail + module;

  /* Don't throttle if maximum is not set */
  if(0 == log_ptr->max_logs_per_unit) 
    return 0;
  
  /* Get current time */
  gettimeofday(&current_time,NULL);
 
  if( current_time.tv_sec >= log_ptr->unit_start)
    time_diff =  current_time.tv_sec  - log_ptr->unit_start;
  else /* For 32 bit operations avoid wrap around */
    time_diff = (0xFFFFFFFFL - log_ptr->unit_start) + current_time.tv_sec;

  if( time_diff < log_ptr->log_time_unit)
    {
      log_ptr->log_count++;
    }
  else
    {
      log_ptr->log_count = 1;
      log_ptr->unit_start = current_time.tv_sec;
    }

  if( hsl_log_detail[module].log_count >= log_ptr->max_logs_per_unit)
    {
      /* Drop log */
      return -1;
    }
#endif 
  return 0;
}
 
/************************************************************
 * hsl_log_conf - Service routine to config log level     *
 * Parameters:                                              *
 *     module  - logging module name                        * 
 *     level   - error level                                * 
 * Returns:                                                 *
 *  0  config ok                                            *
 *  -1 Error occured                                        *
 ************************************************************/
int hsl_log_conf(u_int16_t module, u_int16_t level)
{
  /* Validate module id */
  if(module >= HSL_LOG_LAST_MODULE)
    return -1;

  hsl_log_detail[module].level = level;
  return 0;  
}
