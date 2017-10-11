/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"

char proc_net_snmp[] = "/proc/net/snmp";

static void
dropline (FILE *fp)
{
  int c;

  while ((c = getc (fp)) != '\n')
    ;
}

result_t
pal_kernel_ipv4_forwarding_get (s_int32_t *state)
{
  FILE *fp;
  int ipforwarding = 0;
  char buf[10];

  fp = fopen (proc_net_snmp, "r");

  if (fp == NULL)
    return -1;

  /* We don't care about the first line. */
  dropline (fp);
  
  /* Get ip_statistics.IpForwarding : 
     1 => ip forwarding enabled 
     2 => ip forwarding off. */
  if (fgets (buf, 6, fp) == NULL)
    {
      *state = 0;
      return RESULT_ERROR;
    }

  sscanf (buf, "Ip: %d", &ipforwarding);

  if (ipforwarding == 1)
    {
      *state = 1;
    }
  else 
    {
      *state = 0;
    }

  fclose (fp);

  return RESULT_OK;
}

/* char proc_ipv4_forwarding[] = "/proc/sys/net/ipv4/conf/all/forwarding"; */
char proc_ipv4_forwarding[] = "/proc/sys/net/ipv4/ip_forward";

result_t
pal_kernel_ipv4_forwarding_set(s_int32_t state)
{
  FILE *fp;

  fp = fopen (proc_ipv4_forwarding, "w");
  
  if (fp == NULL)
    return -1;

  if(state == 0)
    {
      fprintf (fp, "0\n");
    }
  else
    {
      fprintf (fp, "1\n");
    }

  fclose (fp);

  return RESULT_OK;
}

#ifdef HAVE_IPV6

char proc_ipv6_forwarding[] = "/proc/sys/net/ipv6/conf/all/forwarding";

result_t
pal_kernel_ipv6_forwarding_get(s_int32_t *state)
{
  FILE *fp;
  char buf[5];
  int ipforwarding = 0;

  fp = fopen (proc_ipv6_forwarding, "r");

  if (fp == NULL)
    return -1;

  fgets (buf, 2, fp);
  sscanf (buf, "%d", &ipforwarding);

  *state = (s_int32_t)ipforwarding;

  fclose (fp);

  return RESULT_OK;
}

result_t
pal_kernel_ipv6_forwarding_set(s_int32_t state)
{
  FILE *fp;

  fp = fopen (proc_ipv6_forwarding, "w");
  
  if (fp == NULL)
    return -1;

  if (state == 0)
    {
      fprintf (fp, "0\n");
    }
  else 
    {
      fprintf (fp, "1\n");
    }

  fclose (fp);

  return RESULT_OK;
}

#endif /* HAVE_IPV6 */
