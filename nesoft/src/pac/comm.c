/******************************************************************************
 *
 *  pac/vport/pw_cli.c
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2012 Ku Ying <PacBridge Corp.>
 *  All rights reserved.
 *
 *  == 说明 ==
 *  pw(Pseudo Wire)CLI部分，创建于2012.2.10
 *
 */

#include "pal.h"
#include "hsl/hal_netlink.h"
#include "hsl/hal_comm.h"
#include "nsmd.h"
#include "comm.h"


/*
 * 发送消息到hal
 */
void comm_send_hal_msg(UINT16 type, void* data, UINT32 len, int (*filter) (struct hal_nlmsghdr *, void *data), void *resp)
{
    int ret;

    struct hal_msg
    {
        struct hal_nlmsghdr head;
        CHAR body[1];
    } *msg;

    msg = XCALLOC (MTYPE_TMP, sizeof(struct hal_nlmsghdr) + len);
    msg->head.nlmsg_len = HAL_NLMSG_LENGTH (len);
    msg->head.nlmsg_flags = HAL_NLM_F_CREATE | HAL_NLM_F_REQUEST;
    msg->head.nlmsg_type = type;
    msg->head.nlmsg_seq = ++hallink_cmd.seq;
	
    pal_mem_cpy(msg->body, data, len);

    ret = hal_talk (&hallink_cmd, &msg->head, filter, resp);
    if (ret < 0)
    {
        int i = 0;
        zlog_info (NSM_ZG, "->hal_send_msg fail(ret: %d): type: %d, len: %d, flag: %d, seq: %d\n",
            ret, msg->head.nlmsg_type, msg->head.nlmsg_len, msg->head.nlmsg_flags, msg->head.nlmsg_seq);

        for (i = 0; i < len; i ++)
            zlog_info (NSM_ZG, "%02X ", *((char* )data + i));
    }

    zlog_info (NSM_ZG, "->hal_send_msg success(ret: %d): type: %d, len: %d, flag: %d, seq: %d\n",
            ret, msg->head.nlmsg_type, msg->head.nlmsg_len, msg->head.nlmsg_flags, msg->head.nlmsg_seq);

    XFREE(MTYPE_TMP, msg);
    return;
}
