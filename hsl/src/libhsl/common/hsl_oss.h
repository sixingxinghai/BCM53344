/* Copyright (C) 2004  All Rights Reserved. */
#ifndef _HSL_OSS_H
#define _HSL_OSS_H 
 
typedef enum 
{
   OSS_SEM_BINARY,
   OSS_SEM_COUNTING,
   OSS_SEM_MUTEX
} oss_sem_type_t;

#define OSS_WAIT_FOREVER       (-1)
#define OSS_DO_NOT_WAIT         (0)

typedef enum 
{
   OSS_MEM_HEAP,
   OSS_MEM_DMA,
} oss_mem_type_t;


/* Boolean definition. */  
typedef enum 
{
   OSS_FALSE=0,
   OSS_TRUE,
} oss_boolean_t;

typedef int comp_result_t; 

#define OSS_LOG_BUFFER_SIZE (256)

#define   STATUS_OK                                  (0)     /* Operation succeess.                             */
#define   STATUS_WRONG_PARAMS                        (-1)    /* Wrong parameters passed to function             */
#define   STATUS_OUT_OF_RESOURCES                    (-2)    /* System resource exausted. e.g dma/shared memory.*/
#define   STATUS_MEM_EXHAUSTED                       (-3)    /* System heap memory exausted.                    */
#define   STATUS_KEY_NOT_FOUND                       (-4)    /* Search result - no entry found.                 */
#define   STATUS_DUPLICATE_KEY                       (-5)    /* Addition failed due to duplicate data.          */
#define   STATUS_SEMAPHORE_LOCK_ERROR                (-6)    /* Resource lock failed.                           */
#define   STATUS_SEMAPHORE_CREATE_ERROR              (-7)    /* Semaphore/mutex creation failed.                */
#define   STATUS_THREAD_CREATE_ERROR                 (-8)    /* Thread creation error.                          */ 
#define   STATUS_THREAD_DELETE_ERROR                 (-9)    /* Thread deletion error.                          */
#define   STATUS_ERR_IFMAP_MAX_FE_PORTS_NOT_SET      (-10)   /* Maximum number of fe ports is not initialized.  */
#define   STATUS_ERR_IFMAP_MAX_GE_PORTS_NOT_SET      (-11)   /* Maximum number of ge ports is not initialized.  */
#define   STATUS_ERR_IFMAP_MAX_XE_PORTS_NOT_SET      (-12)   /* Maximum number of xe ports is not initialized.  */
#define   STATUS_ERR_IFMAP_MAX_AGG_PORTS_NOT_SET     (-13)   /* Maximum number of agg ports is not initialized. */
#define   STATUS_ERR_IFMAP_UNKNOWN_DATA_PORT         (-14)   /* Unknown port type.(not fe/ge/xe).               */
#define   STATUS_WARN_IFMAP_LPORT_ALREADY_REGISTERED (-15)   /* Already registred interface.                    */ 
#define   STATUS_ERR_IFMAP_MAX_PORTS_ALLOCATED       (-16)   /* Maximum number of ports allocated.              */    
#define   STATUS_ERR_IFMAP_LPORT_NOT_FOUND           (-17)   /* Port not found.                                 */ 
#define   STATUS_ERROR                               (-(__LINE__))  /* General error status. Better don't use   */



/*
 * Comparison function result used for 
 * search/insertion/deletion.
 */
#define   HSL_COMP_EQUAL                                (0)
#define   HSL_COMP_LESS_THAN                           (-1)  
#define   HSL_COMP_GREATER_THAN                         (1)     



/* Interface name definition */
#define HSL_IFMAP_FE_NAME                              "fe"
#define HSL_IFMAP_GE_NAME                              "ge"
#define HSL_IFMAP_XE_NAME                              "xe"
#define HSL_IFMAP_AGG_NAME                             "agg"

/* Interface type flags */   
#define HSL_PORT_F_FE                                 (0x1) 
#define HSL_PORT_F_GE                                 (0x2) 
#define HSL_PORT_F_XE                                 (0x4) 
#define HSL_PORT_F_AGG                                (0x8) 

/* Default vlan name */
#define HSL_DEFAULT_VLAN_NAME                         "vlan"

typedef void *apn_sem_id;


            /********************************** 
             * Mutual exclusions declarations *  
             **********************************/
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
             apn_sem_id *sem_id_ptr);


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
oss_sem_delete( oss_sem_type_t sem_type, apn_sem_id sem_id_ptr);

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
oss_sem_lock( oss_sem_type_t sem_type, apn_sem_id sem_id_ptr, int timeout);

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
oss_sem_unlock( oss_sem_type_t sem_type, apn_sem_id sem_id_ptr);

            /********************************** 
             * Memory management declarations *  
             **********************************/
/************************************************************************************
 * Function: oss_malloc  - Service routine to allocate memory                       *
 * Parameters:                                                                      *
 *   IN  size     - required size in bytes.                                         *
 *   IN  mem_type - type of memory required heap/dma.                               *
 * Return value:                                                                    *
 *   void *ptr - pointer to allocated memory buffer                                 *
 *   NULL - if memory allocation failed                                             *
 ************************************************************************************/
void * 
oss_malloc(unsigned long size,oss_mem_type_t mem_type);
/************************************************************************************
 * Function: oss_free - Service routine to free allocated memory                    *
 * Parameters:                                                                      *
 *   IN  ptr - pointer to freed memory.                                             *
 *   IN  mem_type - type of memory freed heap/dma.                                  *
 * Return value:                                                                    *
 *   void                                                                           * 
 ************************************************************************************/
void  
oss_free(void *ptr, oss_mem_type_t mem_type);
            /********************************** 
             * Logging utility   declarations *  
             **********************************/
/************************************************************************************
 * Function: oss_printf  - Service routine to log a message                         *
 * Parameters:                                                                      *
 *   IN  format - message format.                                                   *
 *   arguments                                                                      *
 * Return value:                                                                    *
 *   void                                                                           *
 ************************************************************************************/
void  
oss_printf(const char* format, ...);

            /**************************
             * Atomic operations APIs *
             **************************/
typedef struct { volatile int counter; } oss_atomic_t;

/************************************************************************************
 * Function: oss_atomic_inc  - Increment atomically                                 *
 * Parameters:                                                                      *
 *   IN  val                                                                        *
 *   arguments                                                                      *
 * Return value:                                                                    *
 *   void                                                                           *
 ************************************************************************************/
void
oss_atomic_inc (oss_atomic_t *val);

/************************************************************************************
 * Function: oss_atomic_dec  - Decrement atomically                                 *
 * Parameters:                                                                      *
 *   IN  val                                                                        *
 *   arguments                                                                      *
 * Return value:                                                                    *
 *   void                                                                           *
 ************************************************************************************/
void
oss_atomic_dec (oss_atomic_t *val);

/************************************************************************************
 * Function: oss_atomic_set  - Set value atomically                                 *
 * Parameters:                                                                      *
 *   IN val                                                                         *
 *   IN set                                                                         *
 *   arguments                                                                      *
 * Return value:                                                                    *
 *   void                                                                           *
 ************************************************************************************/
void
oss_atomic_set (oss_atomic_t *val, int set);

/************************************************************************************
 * Function: oss_atomic_dec_and_test  - Decrement atomically and test               *
 * Parameters:                                                                      *
 *   IN  val                                                                        *
 *   arguments                                                                      *
 * Return value:                                                                    *
 *   void                                                                           *
 ************************************************************************************/
int
oss_atomic_dec_and_test (oss_atomic_t *val);

#define OSS_RANDOM_MIN     0
#define OSS_RANDOM_MAX     0x7fffffff
/************************************************************************************
 * Function: oss_rand - Get a random number                                         *
 * Parameters:                                                                      *
 *   IN  val                                                                        *
 *   arguments                                                                      *
 * Return value:                                                                    *
 *   void                                                                           *
 ************************************************************************************/
int oss_rand (void);

#endif  /*_HSL_OSS_H */
