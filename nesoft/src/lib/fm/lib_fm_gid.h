/*--------------------------------------------------------------------------
* Copyright (C) 2007 All rights reserved
*
* File name:    lib_fm_gid.h
* Purpose:      To define Fault Groups Identifiers.
* Description:  This file provide global scope definitions of all
*               Fault Groups Identifiers. Application components, local
*               C files are free to define local froup of faults, but each
*               such group must have an id, which is defined here.
*               The group id must be submitted during group definition as well
*               duirng fault reporting.
* Author:       STC
* Data:         05/25/2007
*----------------------------------------------------------------------------
*/

#ifndef _LIB_FM_GID_H_
#define _LIB_FM_GID_H_

typedef enum fmGroupId_e {
  FM_GID_NONE = 0,
  FM_GID_CAL  = 1,
  FM_GID_CMI  = 2,
  FM_GID_NSM  = 3,
  FM_GID_LACP = 4,
  FM_GID_RMON = 5,
  FM_GID_ONM  = 6,
  FM_GID_MSTP = 7,
  FM_GID_IMI  = 8,
  FM_GID_FREC = 9,
  FM_GID_MAX,
  FM_GID_ALL
} fmGroupId_e;

#endif /* _LIB_FM_GID_H_ */
