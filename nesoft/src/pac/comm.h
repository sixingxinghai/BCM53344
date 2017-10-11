/******************************************************************************
 *
 *  pac/comm.h
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2012 Ku Ying <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  pac公共头文件，创建于2012.4.26
 *
 */

#ifndef _PAC_COMM_H_
#define _PAC_COMM_H_

typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef char CHAR;

typedef enum
{
    FALSE = 0,
    TRUE = (!FALSE)
}BOOL;


#define MAX_NAME_LEN        16
#define MAX_DESC_LEN        64

enum em_direction
{
    INGRESS,
    EGRESS,
    DIR_BUTT
};

#define IS_DIR_VALID(dir) ((INGRESS == dir) || (EGRESS == dir))

extern void comm_send_hal_msg(UINT16 type, void* data, UINT32 len, int (*filter) (struct hal_nlmsghdr *, void *), void *resp);
#endif
