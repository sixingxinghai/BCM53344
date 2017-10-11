/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_COMM_H_
#define _HAL_COMM_H_

/*
   Name: hal_ioctl

   Description:
   Generic hal ioctl handler.

   Parameters:
   arg0 - arg5 -- unsigned long arguments

   Returns:
   < 0 on error
   HAL_SUCCESS
*/
int
hal_ioctl (unsigned long arg0, unsigned long arg1, unsigned long arg2,
           unsigned long arg3, unsigned long arg4, unsigned long arg5,
           unsigned long arg6);

#endif /* _HAL_COMM_H_ */
