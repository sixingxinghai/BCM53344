/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_H_
#define _HAL_H_

/* 
   Name: hal_init

   Description: 
   Initialize the HAL component. 

   Parameters:
   None

   Returns:
   < 0 on error 
   HAL_SUCCESS
*/
int
hal_init (struct lib_globals *zg);

/* 
   Name: hal_deinit

   Description:
   Deinitialize the HAL component.

   Parameters:
   None

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_deinit (struct lib_globals *zg);

#endif /* _HAL_H_ */
