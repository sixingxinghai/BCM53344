/* Copyright (C) 2006  All Rights Reserved.  */
#define ZG      (azg)

#ifndef __MPLSONM_H__
#define __MPLSONM_H__

#include "bfd_message.h"

/* Macro to get label value from shim hdr (Redfiniton from MPLS-FWD code)*/
#define get_label_net(shim)      (((shim) >> 12) & 0xFFFFF)

void mplsonm_terminate (struct lib_globals *);

extern struct lib_globals *azg;

#ifdef HAVE_MPLS_OAM
struct oam_msg_mpls_ping_req mpls_oam;
#elif defined (HAVE_NSM_MPLS_OAM)
struct nsm_msg_mpls_ping_req  mpls_oam;
#endif /* HAVE_MPLS_OAM */

struct mplsonm_echo_data {
#ifdef HAVE_MPLS_OAM
 struct oam_msg_mpls_ping_req  mpls_oam;
#elif defined (HAVE_NSM_MPLS_OAM)
 struct nsm_msg_mpls_ping_req  mpls_oam;
#endif /* HAVE_MPLS_OAM */
 u_int32_t recv_count;
 u_int32_t reply_count;
 u_int32_t resp_count;
 float rtt_min;
 float rtt_avg;
 float rtt_max;
 struct pal_in4_addr fec_addr;
};

struct mplsonm_echo_data onm_data;

struct mpls_onm_cmd_options {
  u_char *option;
  u_char type;
};

#ifdef HAVE_MPLS_OAM
void
mplsonm_oam_init (struct lib_globals * zg);
#elif defined (HAVE_NSM_MPLS_OAM)
void
mplsonm_nsm_init (struct lib_globals * zg);
#endif /* HAVE_MPLS_OAM */

void
mpls_onm_detail (void);
void mpls_onm_disp_summary (void);
#ifdef HAVE_MPLS_OAM_ITUT

void 
mplsonm_nsm_itut_init(struct lib_globals *);

int
mplsonm_itut_start (void);
void
mplsonm_itut_stop (void);

#endif /* HAVE_MPLS_OAM_ITUT*/

int
mpls_onm_ping_response (struct bfd_msg_header *header,
                            void *arg, void *message);
int
mpls_onm_trace_response (struct bfd_msg_header *header,
                             void *arg, void *message);
int
mpls_onm_error (struct bfd_msg_header *header,
                             void *arg, void *message);
#endif
