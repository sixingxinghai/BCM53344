/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"
#include "sal/core/sync.h"
#include "hsl_oss.h"

/************************************************************************************
 * Function: oss_sem_new  - Service routine to create a new semaphore               *
 * Parameters:                                                                      *
 *   IN  sem_name        - semaphore descriptor.                                    *
 *   IN  sem_type        - type of semaphore (binary/counting/mutex).               *
 *   IN  num_of_tokens   - initial tokens count for counting semaphore.             *
 *   IN  sem_flags       - special flags - like SEM_DELETE_SAFE/SEM_INVERSION_SAFE  *
 *                                              SEM_Q_PRIORITY.                     *
 *   OUT sem_id_ptr      - pointer to newly created semaphore.                      *
 * Return value:                                                                    *
 *   0 - semaphore creation successful.                                             *
 *   otherwise - semaphore creation failed.                                         *
 ************************************************************************************/
int 
oss_sem_new( char * sem_name,
             oss_sem_type_t sem_type,
             u_int32_t num_of_tokens,
             void *sem_flags,
             apn_sem_id *sem_id_ptr)
{

   if(!sem_name || !sem_id_ptr)
   {
      return STATUS_WRONG_PARAMS;
   }

   switch (sem_type) {
     case OSS_SEM_BINARY:
          *(sal_sem_t *)sem_id_ptr = sal_sem_create( sem_name, sal_sem_BINARY, num_of_tokens);
          break;
     case OSS_SEM_COUNTING:
          *(sal_sem_t *)sem_id_ptr = sal_sem_create( sem_name, sal_sem_COUNTING, num_of_tokens);
          break;
     case OSS_SEM_MUTEX:
          *(sal_mutex_t *)sem_id_ptr = sal_mutex_create( sem_name );
          break; 
   }
   return STATUS_OK;
}


/************************************************************************************
 * Function: oss_sem_delete  - Service routine to delete a semaphore                *
 * Parameters:                                                                      *
 *   IN  sem_type        - type of semaphore (binary/counting/mutex)                *
 *   IN  sem_id_ptr      - pointer to semaphore                                     *
 * Return value:                                                                    *
 *   0 - semaphore was successfully deleted.                                        *
 *   otherwise - semaphore deletion failed                                          *
 ************************************************************************************/
int 
oss_sem_delete( oss_sem_type_t sem_type, apn_sem_id sem_id_ptr)
{

   if(!sem_id_ptr)
   {
      return STATUS_WRONG_PARAMS;
   }

   switch (sem_type) {
     case OSS_SEM_BINARY:
     case OSS_SEM_COUNTING:
          sal_sem_destroy((sal_sem_t)sem_id_ptr);
          break;
     case OSS_SEM_MUTEX:
          sal_mutex_destroy((sal_mutex_t)sem_id_ptr);
          break; 
   }
   return STATUS_OK;
}

/************************************************************************************
 * Function: oss_sem_lock    - Service routine to lock a semaphore                  *
 * Parameters:                                                                      *
 *   IN  sem_type        - type of semaphore (binary/counting/mutex).               *
 *   IN  sem_id_ptr      - pointer to semaphore.                                    *
 *   IN  timeout         - miliseconds lock timeout value.                          *
 * Return value:                                                                    *
 *   0 - semaphore was successfully locked.                                         *
 *   otherwise - semaphore lock failed.                                             *
 ************************************************************************************/
int 
oss_sem_lock( oss_sem_type_t sem_type, apn_sem_id sem_id_ptr, int timeout)
{

   int status;
   int timeout_val;

   if (timeout == OSS_WAIT_FOREVER)
      timeout_val = sal_sem_FOREVER;
   else
      timeout_val = 1000 * timeout;

   if(!sem_id_ptr)
   {
      return STATUS_WRONG_PARAMS;
   }

   switch (sem_type) {
     case OSS_SEM_BINARY:
     case OSS_SEM_COUNTING:
          status = sal_sem_take((sal_sem_t)sem_id_ptr,timeout_val);
          break;
     case OSS_SEM_MUTEX:
          status = sal_mutex_take((sal_mutex_t)sem_id_ptr,timeout_val);
          break; 
     default: 
          status = STATUS_ERROR;
   }
   return (status)? STATUS_ERROR : STATUS_OK;
}

/************************************************************************************
 * Function: oss_sem_unlock    - Service routine to unlock a semaphore              *
 * Parameters:                                                                      *
 *   IN  sem_type        - type of semaphore (binary/counting/mutex).               *
 *   IN  sem_id_ptr      - pointer to semaphore.                                    *
 * Return value:                                                                    *
 *   0 - semaphore was successfully unlocked.                                       *
 *   otherwise - semaphore unlock failed.                                           *
 ************************************************************************************/
int 
oss_sem_unlock( oss_sem_type_t sem_type, apn_sem_id sem_id_ptr)
{

   int status; 

   if(!sem_id_ptr)
   {
      return STATUS_WRONG_PARAMS;
   }
   
   switch (sem_type) {
     case OSS_SEM_BINARY:
     case OSS_SEM_COUNTING:
          status = sal_sem_give((sal_sem_t)sem_id_ptr);
          break;
     case OSS_SEM_MUTEX:
          status = sal_mutex_give((sal_mutex_t)sem_id_ptr);
          break; 
     default: 
          status = STATUS_ERROR;
   }
   return (status)? STATUS_ERROR : STATUS_OK;
}
