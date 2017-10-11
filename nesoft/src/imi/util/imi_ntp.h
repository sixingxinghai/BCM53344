/* Copyright (C) 2003  All Rights Reserved. */
#ifndef _IMI_NTP_H
#define _IMI_NTP_H

#include "fp.h"

/* MIN, MAX, MAX3 macros. */
#define min(a,b)      (((a) < (b)) ? (a) : (b))
#define max(a,b)      (((a) > (b)) ? (a) : (b))
#define min3(a,b,c)   min(min((a),(b)), (c))

#define NTP_STR                  "Configure NTP"
#define NTP_SHOW_STR             "Network time protocol"
#define NTP_PEER_STR             "Configure NTP peer"
#define NTP_SERVER_STR           "Configure NTP server"
#define NTP_AUTHENTICATE_STR     "Authenticate time sources"

typedef u_int32_t ntp_auth_key_num;
typedef u_int16_t ntp_access_group_num;

enum ntp_nbr_type
  {
    NTP_MODE_SERVER,
    NTP_MODE_PEER,

    NTP_MODE_MAX
  };

enum ntp_preference
  {
    NTP_PREFERENCE_IGNORE,
    NTP_PREFERENCE_NO,
    NTP_PREFERENCE_YES
  };

/* Key type. */
enum ntp_key_type
  {
    NTP_KEY_MD5
  };

/* NTP Version. */
enum ntp_version
  {
    NTP_VERSION_IGNORE,
    NTP_VERSION_1,
    NTP_VERSION_2,
    NTP_VERSION_3,
    NTP_VERSION_4
  };

/* Linked list of NTP keys. */
struct ntp_key
{
  /* Key number. */
  ntp_auth_key_num key_num;

  /* Key. */
  char *key;

  /* Key type. */
  enum ntp_key_type type;
};

struct ntp_neighbor
{
  /* Neighbor name. */
  char *name;

  /* NTP preference. */
  enum ntp_preference preference;

  /* Key number. */
  ntp_auth_key_num key_num;

  /* Neighbor type. */
  enum ntp_nbr_type type;

  /* NTP version of neighbor. */
  enum ntp_version version;
};

enum ntp_access_group_type
{
  NTP_ACCESS_GROUP_PEER,
  NTP_ACCESS_GROUP_QUERY_ONLY,
  NTP_ACCESS_GROUP_SERVE,
  NTP_ACCESS_GROUP_SERVE_ONLY
};

struct ntp_access_group
{
  ntp_access_group_num peer;
  ntp_access_group_num query_only;
  ntp_access_group_num serve;
  ntp_access_group_num serve_only;
};

/* NTP status. */
struct ntp_sys_stats
{
  /* Status. */
  u_int16_t status;

  /* Leap. */
  u_char leap;

  /* Stratum. */
  u_char stratum;

  /* Precision. */
  s_int32_t precision;

  /* Root Delay. */
  struct long_fp rootdelay;

  /* Root Dispersion. */
  struct long_fp rootdispersion;

  /* Peer. */
  u_int32_t peer;

  /* Reference ID. */
  char *refid;

  /* Reference time. */
  struct long_fp reftime;

  /* Poll. */
  u_int32_t poll;

  /* Clock. */
  struct long_fp clock;

  /* State. */
  u_int32_t state;

  /* Offset. */
  struct long_fp offset;

  /* Frequency. */
  struct long_fp frequency;

  /* Jitter. */
  struct long_fp jitter;

  /* Stability. */
  struct long_fp stability;
};

/* NTP peer status. */
struct ntp_peer_stats
{
  /* Status. */
  u_int16_t status;

  /* Source address. */
  char *srcadr;

  /* Source port. */
  u_int16_t srcport;

  /* Destination address. */
  char *dstadr;

  /* Destination port. */
  u_int16_t dstport;

  /* Leap. */
  u_int16_t leap;

  /* Stratum. */
  u_char stratum;

  /* Time since last packet. */
  u_int32_t when;

  /* Precision. */
  s_int32_t precision;

  /* Root Delay. */
  struct long_fp rootdelay;

  /* Root Dispersion. */
  struct long_fp rootdispersion;

  /* Reference ID. */
  char *refid;

  /* Reach. */
  u_int32_t reach;

  /* Mode, either broadcast, active or client */
  u_int32_t hmode;

  /* Remote association mode. */
  u_int32_t pmode;

  /* Local poll interval. */
  u_int32_t hpoll;

  /* Remote poll interval. */
  u_int32_t ppoll;

  /* Protocol error test tally bits. */
  u_int32_t flash;

  /* Key ID. */
  u_int32_t keyid;

  /* Peer clock offset. */
  struct long_fp offset;

  /* Roundtrip delay. */
  struct long_fp delay;

  /* Clock dispersion. */
  struct long_fp dispersion;

  /* Jitter. */
  struct long_fp jitter;

  /* System reference time. */
  struct long_fp reftime;

  /* Originate time stamp. */
  struct long_fp org;

  /* Transmit time stamp. */
  struct long_fp xmt;

  /* Receive time stamp. */
  struct long_fp rec;

  /* Delay shift register. */
  struct long_fp filtdelay[8];

  /* Offset shift register. */
  struct long_fp filtoffset[8];

  /* Dispersion shift register. */
  struct long_fp filtdisp[8];
};

/* Wrapper structure for all peer status. */
struct ntp_all_peer_stats
{
  /* Number of peers. */
  u_int16_t num;

  /* Status of each peer. */
  struct ntp_peer_stats *stats;
};

/* NTP master structure. */
struct ntp_master
{
  /* Linked list of NTP neighbors of struct ntp_neighbor. */
  struct list *ntp_neighbor_list;

  /* Linked list of NTP keys of struct ntp_key. */
  struct list *ntp_key_list;

  /* Authenticate? */
  u_char authentication;
#define NTP_AUTHENTICATE                 1

  /* Linked list of trusted keys. */
  struct list *ntp_trusted_key_list;

  /* Broadcast delay. */
  u_int32_t broadcastdelay;

  /* Master. */
  u_char master_clock_flag;
#define NTP_MASTER_CLOCK                 1

  /* Master stratum. */
  u_char stratum;
#define NTP_STRATUM_ZERO                 0

  /* NTP Access list. */
  struct ntp_access_group   access;

  /* NTP status. */
  struct ntp_sys_stats stats;

  /* NTP peer status. */
  struct ntp_all_peer_stats peerstats;
};

/* Buffer sizes. */
#define NTP_TIME_BUFFER_SZ         100

/* FLAGS. */
#define NTP_SUCCESS                     0
#define NTP_FAILURE                    -1

/* Errors. */
#define NTP_ERROR_BASE             -100
#define NTP_ERROR_KEY_EXISTS       (NTP_ERROR_BASE - 1)
#define NTP_ERROR_KEY_ADD          (NTP_ERROR_BASE - 2)
#define NTP_ERROR_KEY_DEL          (NTP_ERROR_BASE - 3)
#define NTP_ERROR_AUTH_NOTSET      (NTP_ERROR_BASE - 4)
#define NTP_ERROR_AUTH_SET         (NTP_ERROR_BASE - 5)
#define NTP_ERROR_NBR_ADD          (NTP_ERROR_BASE - 6)
#define NTP_ERROR_NBR_DEL          (NTP_ERROR_BASE - 7)
#define NTP_ERROR_NBR_NOTEXISTS    (NTP_ERROR_BASE - 8)
#define NTP_ERROR_NTP_CONFIG       (NTP_ERROR_BASE - 9)

/* RFC 1305 specific NTP defines follows. */

#define NTP_MASTER_LOCAL_CLOCK    "127.127.1.0"

/* Time of day conversion constant.
   Epoch time starts from 1970. Ntp's time scale starts in 1900 */
#define JAN_1970     0x83aa7e80  /* 2208988800 1970 - 1900 in seconds */

/* Values for peer.leap, sys.leap. */
#define NTP_LEAP_NOWARNING  0x0   /* normal, no leap second warning. */
#define NTP_LEAP_ADDSECOND  0x1   /* last minute of day has 61 seconds. */
#define NTP_LEAP_DELSECOND  0x2   /* last minute of day has 59 seconds. */
#define NTP_LEAP_NOTINSYNC  0x3   /* overload, clock is free running. */

/* Peer modes. */
#define NTP_MODE_UNSPEC     0       /* unspecified (probably old NTP version) */
#define NTP_MODE_ACTIVE     1       /* symmetric active */
#define NTP_MODE_PASSIVE    2       /* symmetric passive */
#define NTP_MODE_CLIENT     3       /* client mode */
#define NTP_MODE_SERVER     4       /* server mode */
#define NTP_MODE_BROADCAST  5       /* broadcast mode */
#define NTP_MODE_CONTROL    6       /* control mode packet */
#define NTP_MODE_PRIVATE    7       /* implementation defined function */
#define NTP_MODE_BCLIENT    8       /* broadcast client mode */

/*
 * Define flasher bits (tests 1 through 11 in packet procedure)
 * These reveal the state at the last grumble from the peer and are
 * most handy for diagnosing problems, even if not strictly a state
 * variable in the spec. These are recorded in the peer structure.
 */
#define NTP_TEST1           0x0001  /* duplicate packet received */
#define NTP_TEST2           0x0002  /* bogus packet received */
#define NTP_TEST3           0x0004  /* protocol unsynchronized */
#define NTP_TEST4           0x0008  /* access denied */
#define NTP_TEST5           0x0010  /* authentication failed */
#define NTP_TEST6           0x0020  /* peer clock unsynchronized */
#define NTP_TEST7           0x0040  /* peer stratum out of bounds */
#define NTP_TEST8           0x0080  /* root delay/dispersion bounds check */
#define NTP_TEST9           0x0100  /* peer delay/dispersion bounds check */
#define NTP_TEST10          0x0200  /* autokey failed */
#define NTP_TEST11          0x0400  /* proventic not confirmed */

/* System status word. */
#define NTP_CONTROL_SYS_MAXEVENTS       15

#define NTP_CONTROL_SYS_STATUS(li, source, nevnt, evnt) \
                (((((unsigned short)(li))<< 14)&0xc000) | \
                (((source)<<8)&0x3f00) | \
                (((nevnt)<<4)&0x00f0) | \
                ((evnt)&0x000f))

/* (En,De)coding of the peer status word */
#define NTP_CONTROL_PST_CONFIG          0x80
#define NTP_CONTROL_PST_AUTHENABLE      0x40
#define NTP_CONTROL_PST_AUTHENTIC       0x20
#define NTP_CONTROL_PST_REACH           0x10
#define NTP_CONTROL_PST_UNSPEC          0x08

#define NTP_CONTROL_SYS_LI(status)      (((status)>>14) & 0x3)
#define NTP_CONTROL_SYS_SOURCE(status)  (((status)>>8) & 0x3f)
#define NTP_CONTROL_SYS_NEVNT(status)   (((status)>>4) & 0xf)
#define NTP_CONTROL_SYS_EVENT(status)   ((status) & 0xf)

/* Stuff for extracting things from li_vn_mode */
#define NTP_PACKET_MODE(li_vn_mode)    ((u_char)((li_vn_mode) & 0x7))
#define NTP_PACKET_VERSION(li_vn_mode) ((u_char)(((li_vn_mode) >> 3) & 0x7))
#define NTP_PACKET_LEAP(li_vn_mode)    ((u_char)(((li_vn_mode) >> 6) & 0x3))

/* Stuff for putting things back into li_vn_mode */
#define NTP_PACKET_LI_VN_MODE(li, vn, md) \
        ((u_char)((((li) << 6) & 0xc0) | (((vn) << 3) & 0x38) | ((md) & 0x7)))

/* Decoding for the r_m_e_op field */
#define NTP_CONTROL_RESPONSE    0x80
#define NTP_CONTROL_ERROR       0x40
#define NTP_CONTROL_MORE        0x20
#define NTP_CONTROL_OP_MASK     0x1f

#define NTP_CONTROL_ISRESPONSE(r_m_e_op)        (((r_m_e_op) & 0x80) != 0)
#define NTP_CONTROL_ISMORE(r_m_e_op)    (((r_m_e_op) & 0x20) != 0)
#define NTP_CONTROL_ISERROR(r_m_e_op)   (((r_m_e_op) & 0x40) != 0)
#define NTP_CONTROL_OP(r_m_e_op)        ((r_m_e_op) & NTP_CONTROL_OP_MASK)

#define NTP_CONTROL_PEER_STATUS(status, nevnt, evnt) \
                ((((status)<<8) & 0xff00) | \
                (((nevnt)<<4) & 0x00f0) | \
                ((evnt) & 0x000f))

#define NTP_CONTROL_PEER_STATVAL(status)(((status)>>8) & 0xff)
#define NTP_CONTROL_PEER_NEVNT(status)  (((status)>>4) & 0xf)
#define NTP_CONTROL_PEER_EVENT(status)  ((status) & 0xf)

#define FRAC       4294967296.     /* 2^32 as a double */

/* NTP protocol parameters.  See section 3.2.6 of the specification.  */
#define NTP_VERSION     ((u_char)4) /* current version number */
#define NTP_OLDVERSION  ((u_char)1) /* oldest credible version */
#define NTP_PORT        123     /* included for sake of non-unix machines */
#define NTP_UNREACH     16      /* poll interval backoff count */
#define NTP_MINDPOLL    6       /* log2 default min poll interval (64 s) */
#define NTP_MAXDPOLL    10      /* log2 default max poll interval (~17 m) */
#define NTP_MINPOLL     4       /* log2 min poll interval (16 s) */
#define NTP_MAXPOLL     17      /* log2 max poll interval (~4.5 h) */
#define NTP_MINCLOCK    3       /* minimum survivors */
#define NTP_MAXCLOCK    10      /* maximum candidates */
#define NTP_SHIFT       8       /* 8 suitable for crystal time base */
#define NTP_MAXKEY      65535   /* maximum authentication key number */
#define NTP_MAXSESSION  100     /* maximum session key list entries */
#define NTP_AUTOMAX     13      /* log2 default max session key lifetime */
#define KEY_REVOKE      16      /* log2 default key revoke timeout */
#define NTP_FWEIGHT     .5      /* clock filter weight */
#define CLOCK_SGATE     4.      /* popcorn spike gate */
#define BURST_INTERVAL1 4       /* first interburst interval (log2) */
#define BURST_INTERVAL2 1       /* succeeding interburst intervals (log2) */
#define HUFFPUFF        900     /* huff-n'-puff sample interval (s) */

/* Prototypes. */
void imi_ntp_acl_add_hook (struct access_list *access, struct filter_list *filter);
void imi_ntp_acl_delete_hook (struct access_list *access, struct filter_list *filter);
int imi_ntp_init ();
int imi_ntp_deinit ();
int imi_ntp_config_write (struct cli *cli);
int imi_ntp_config_encode (cfg_vect_t *cv);
int ntp_acl_cmp_by_name (char *);

#endif /* _IMI_NTP_H */
