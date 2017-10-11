/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "smi_client.h"

int smiclient_create(int debug, char *);

int
main(int argc, char **argv, char **env)
{
  unsigned int ret = 0;

  if(argc > 2) 
    {
      printf("Exiting...");
      exit(1);
    }

  /* TODO: Replace syslog with zlog */
  syslog(LOG_ERR, "ceregonapi.c: Calling apiclient_start");

  ret = smiclient_create(SMI_AC_DEBUG, argv[1]);
  if(ret < 0)
    {
      if(ret & SMI_AC_NSM_INITERR)
        printf("\tError in initializing NSM API client\n\n");
      if(ret & SMI_AC_LACP_INITERR)
        printf("\tError in initializing LACP API client\n\n");
      if(ret & SMI_AC_MSTP_INITERR)
        printf("\tError in initializing MSTP API client\n\n");
      if(ret & SMI_AC_RMON_INITERR)
        printf("\tError in initializing RMON API client\n\n");
      if(ret & SMI_AC_ONM_INITERR)
        printf("\tError in initializing ONM API client\n\n");
    }
  exit(0);
}
