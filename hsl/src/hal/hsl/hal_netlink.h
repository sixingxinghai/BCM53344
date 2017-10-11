/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_NETLINK_H
#define _HAL_NETLINK_H

/* RFC3549. Linux Netlink as a IP services Protocol. */

/*
  Address family for HSL backend.
*/
#define AF_HSL                                      48
  
/* 
   These are the groups of messages(async) that HSL provides.
*/
#define HAL_GROUP_LINK                              (1 << 0)


/* Netlink message header. */
struct hal_nlmsghdr
{
  u_int32_t nlmsg_len;           /* Length of message including header. */
  u_int16_t nlmsg_type;          /* Message content. */
  u_int16_t nlmsg_flags;         /* Flags. */
  u_int32_t nlmsg_seq;           /* Sequence number. */
  u_int32_t nlmsg_pid;           /* Sending process pid. */
};

#define HAL_NLMSGHDR_SIZE            sizeof(struct hal_nlmsghdr)  

/* Netlink error response structure. */
struct hal_nlmsgerr
{
  int error;
  struct hal_nlmsghdr msg;           /* Request nlmsghdr. */
};

/* Sockaddr for Netlink. */
struct hal_sockaddr_nl
{
  u_int16_t nl_family;
  u_int16_t pad1;
  u_int32_t nl_pid;
  u_int32_t nl_groups;
  u_char padding[14];
};

/* Flags. */
#define HAL_NLM_F_REQUEST          1    /* Request message. */
#define HAL_NLM_F_MULTI            2    /* Multipart message terminated by
                                           NLMSG_DONE. */
#define HAL_NLM_F_ACK              3    /* Reply with ACK. */

/* Additional flag bits for GET requests. */
#define HAL_NLM_F_ROOT             0x100 /* Return complete table. */
#define HAL_NLM_F_MATCH            0x200 /* Return all entries matching 
                                        criteria in message content. */
#define HAL_NLM_F_ATOMIC           0x400 /* Atomic snapshot of the table being
                                        referenced. */

/* Additional flag bits for NEW requests. */
#define HAL_NLM_F_REPLACE          0x100 /* Replace existing matching config
                                        object. */
#define HAL_NLM_F_EXCL             0x200 /* Don't replace the config object
                                        if it already exists. */
#define HAL_NLM_F_CREATE           0x400 /* Create config object if it doesn't
                                        exist. */
#define HAL_NLM_F_APPEND           0x800 /* Add to end of list. */

/* Convenience macros. */
#define HAL_NLMSG_ALIGNTO          4
#define HAL_NLMSG_ALIGN(len)       (((len) + HAL_NLMSG_ALIGNTO - 1) & ~(HAL_NLMSG_ALIGNTO - 1))
#define HAL_NLMSG_LENGTH(len)      (HAL_NLMSG_ALIGN(HAL_NLMSGHDR_SIZE) + len)
#define HAL_NLMSG_SPACE(len)       HAL_NLMSG_ALIGN(HAL_NLMSG_LENGTH(len))
#define HAL_NLMSG_DATA(nlh)        ((void*)(((char*)nlh) + HAL_NLMSG_LENGTH(0)))
#define HAL_NLMSG_NEXT(nlh,len)    ((len) -= HAL_NLMSG_ALIGN((nlh)->nlmsg_len), (struct hal_nlmsghdr*)(((char*)(nlh)) + HAL_NLMSG_ALIGN((nlh)->nlmsg_len)))
#define HAL_NLMSG_OK(nlh,len) ((len) > 0 && (nlh)->nlmsg_len >= HAL_NLMSGHDR_SIZE && (nlh)->nlmsg_len <= (len))
#define HAL_NLMSG_PAYLOAD(nlh,len) ((nlh)->nlmsg_len - HAL_NLMSG_SPACE((len)))

/* Message type. */
#define HAL_NLMSG_NOOP             0x1 /* Message is ignored. */
#define HAL_NLMSG_ERROR            0x2 /* Error. */
#define HAL_NLMSG_DONE             0x4 /* Message terminates a multipart msg. */

#endif /* _HAL_NETLINK_H */

