/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_H_
#define _HSL_H_

/*
  Function prototypes for OS, TCP/IP stack and Hardware. 
*/

int hsl_init (void);
int hsl_deinit (void);
int hsl_os_init (void);
int hsl_os_deinit (void);
int hsl_hw_init (void);
int hsl_hw_deinit (void);
#ifdef HAVE_MPLS
int hsl_hw_mpls_init (void);
int hsl_hw_mpls_deinit (void);
#endif /* HAVE_MPLS */

/* CPU Related stuff */
int hsl_bcm_get_num_cpu (unsigned int *);
int hsl_bcm_set_master_cpu (unsigned char *);
int hsl_bcm_get_cpu_index (unsigned int, char *);
int hsl_bcm_get_dump_cpu_index (unsigned int, char *);
int hsl_bcm_get_master_cpu (char *);
#endif /* _HSL_H_ */
