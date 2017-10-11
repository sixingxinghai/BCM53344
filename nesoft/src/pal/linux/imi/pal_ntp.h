/* Copyright (C) 2003,   All Rights Reserved. */

#ifndef _PAL_NTP_H
#define _PAL_NTP_H

#define NTP_MV_RESTART_STR  "/etc/init.d/ntp restart"
#define NTP_RH_RESTART_STR  "/etc/init.d/ntpd restart"
#define NTP_MV_START_STR    "/etc/init.d/ntp start"
#define NTP_RH_START_STR    "/etc/init.d/ntpd start"
#define NTP_MV_STOP_STR     "/etc/init.d/ntp stop"
#define NTP_RH_STOP_STR     "/etc/init.d/ntpd stop"
#define NTP_KEYS_FILE       "/etc/ntp/keys"
#define NTP_CONF_FILE       "/etc/ntp.conf"

#define NTP_KEYS_TOP_FILE_TEXT   "# Created by IMI. /etc/ntp.keys\n# Key-number\t\tKey-type\t\tKey\n\n"

#define NTP_CONF_TOP_FILE_TEXT   "# Created by IMI. /etc/ntp.conf\n"
#define NTP_CONF_DRIFT_TEXT "\n# Drift file\ndriftfile /etc/ntp/drift\n"
#define NTP_CONF_KEY_TEXT "\n# Keys file. \nkeys /etc/ntp/keys\n"
#define NTP_CONF_BDELAY_TEXT "\n# Broadcast delay. \n"

/* NTP restriction strings. */
#define NTP_FLAG_IGNORE_STR        "ignore"
#define NTP_FLAG_NOMODIFY_STR      "nomodify"
#define NTP_FLAG_NOTRAP_STR        "notrap"
#define NTP_FLAG_NOQUERY_STR       "noquery"
#define NTP_FLAG_NOSERVE_STR       "noserve"

/* NTP restriction flags. */
#define NTP_FLAG_IGNORE             (1 << 0)
#define NTP_FLAG_NOMODIFY           (1 << 1)
#define NTP_FLAG_NOTRAP             (1 << 2)
#define NTP_FLAG_NOQUERY            (1 << 3)
#define NTP_FLAG_NOSERVE            (1 << 4)

/* NTP Peer retriction flags. */
#define NTP_FLAG_PERMIT_PEER        0
#define NTP_FLAG_DENY_PEER          NTP_FLAG_IGNORE

/* NTP Query only restriction flags. */
#define NTP_FLAG_PERMIT_QUERY_ONLY  (NTP_FLAG_NOMODIFY | NTP_FLAG_NOTRAP)
#define NTP_FLAG_DENY_QUERY_ONLY    (NTP_FLAG_IGNORE)

/* NTP Serve restriction flags. */
#define NTP_FLAG_PERMIT_SERVE       (NTP_FLAG_NOMODIFY | NTP_FLAG_NOTRAP)
#define NTP_FLAG_DENY_SERVE         (NTP_FLAG_NOSERVE | NTP_FLAG_NOMODIFY | NTP_FLAG_NOTRAP)

/* NTP Serve only restriction flags. */
#define NTP_FLAG_PERMIT_SERVE_ONLY  (NTP_FLAG_NOMODIFY | NTP_FLAG_NOTRAP | NTP_FLAG_NOQUERY)
#define NTP_FLAG_DENY_SERVE_ONLY    (NTP_FLAG_NOSERVE | NTP_FLAG_NOMODIFY | NTP_FLAG_NOTRAP | NTP_FLAG_NOQUERY)

#define CHECK_NTP_RESTRICT_FLAG(A,B)  ((A) & (B))

/* Maximum number of NTP fragments. */
#define NTP_MAXFRAGS             24

/* Maximum amount of NTP data. */
#define NTP_DATASIZE             (NTP_MAXFRAGS * 480)

/* Min and Max authentication length fields. */
#define MIN_MAC_LEN              3 * sizeof(u_int32_t)
#define MAX_MAC_LEN              5 * sizeof(u_int32_t)

/* ntp control message structure. */
struct ntp_control
{
  u_char  li_vn_mode;   /* leap, version, mode */
  u_char  r_m_e_op;     /* response, more, error, opcode */
  unsigned short sequence; /* sequence number of request */
  unsigned short status;   /* status word for association */
  unsigned short associd;  /* association ID */
  unsigned short offset;   /* offset of this batch of data */
  unsigned short count;    /* count of data in this packet */
  u_char data[(480 + MAX_MAC_LEN)]; /* data + auth */
};

/* ntp association data. */
struct ntp_associations
{
  u_int16_t association_id;
  u_int16_t status;
};
#define MAX_NTP_ASSOCIATIONS   1024


/* NTP variable operands. */
enum ntpvar_operand
  {
    NTP_VAR_NULL,
    NTP_VAR_VERSION,
    NTP_VAR_PROCESSOR,
    NTP_VAR_SYSTEM,
    NTP_VAR_LEAP,
    NTP_VAR_STRATUM,
    NTP_VAR_PRECISION,
    NTP_VAR_ROOTDELAY,
    NTP_VAR_ROOTDISPERSION,
    NTP_VAR_PEER,
    NTP_VAR_REFID,
    NTP_VAR_REFTIME,
    NTP_VAR_POLL,
    NTP_VAR_CLOCK,
    NTP_VAR_STATE,
    NTP_VAR_OFFSET,
    NTP_VAR_FREQUENCY,
    NTP_VAR_JITTER,
    NTP_VAR_STABILITY,
    NTP_VAR_CONFIG,
    NTP_VAR_AUTHENABLE,
    NTP_VAR_AUTHENTIC,
    NTP_VAR_SRCADR,
    NTP_VAR_SRCPORT,
    NTP_VAR_DSTADR,
    NTP_VAR_DSTPORT,
    NTP_VAR_HMODE,
    NTP_VAR_PPOLL,
    NTP_VAR_HPOLL,
    NTP_VAR_ORG,
    NTP_VAR_REC,
    NTP_VAR_XMT,
    NTP_VAR_REACH,
    NTP_VAR_VALID,
    NTP_VAR_TIMER,
    NTP_VAR_DELAY,
    NTP_VAR_DISPERSION,
    NTP_VAR_KEYID,
    NTP_VAR_FILTDELAY,
    NTP_VAR_FILTOFFSET,
    NTP_VAR_PMODE,
    NTP_VAR_RECEIVED,
    NTP_VAR_SENT,
    NTP_VAR_FILTDISP,
    NTP_VAR_FLASH,
    NTP_VAR_TTL,
    NTP_VAR_TTLMAX
  };

/* ntp variables list. */
struct ntp_variables
{
  char *string;
  enum ntpvar_operand operand;
};

#define NTP_CONTROL_HDR_LEN      12
#define NTP_MAX_DATA_LEN         468

#define NTP_MODE_CONTROL         6

/* Types of requests. */
#define TYPE_SYS               1
#define TYPE_PEER              2

/* Timeouts. */
#define DEFAULTTIMEOUT           5   /* Primary timeout for read. */
#define DEFAULTSECONDARYTIMEOUT  2   /* Secondary timeout after primary. */

/*
 * Error code responses returned when the E bit is set.
 */
#define NTP_CERR_UNSPEC     0
#define NTP_CERR_PERMISSION 1
#define NTP_CERR_BADFMT     2
#define NTP_CERR_BADOP      3
#define NTP_CERR_BADASSOC   4
#define NTP_CERR_UNKNOWNVAR 5
#define NTP_CERR_BADVALUE   6
#define NTP_CERR_RESTRICT   7

#define NTP_CERR_NORESOURCE NTP_CERR_PERMISSION

#define NTP_CONTROL_OP_READSTAT  1
#define NTP_CONTROL_OP_READVAR   2

#include "pal_ntp.def"

#endif /* _PAL_NTP_H */
