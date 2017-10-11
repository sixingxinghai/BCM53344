/* Copyright (C) 2004  All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
#include "hsl_types.h"

#define HEX_TO_DIGIT(x)  (((x) >= 'A') ? ((x) - 'A' + 10) : ((x) - '0'))

static char hexChar[] = "0123456789ABCDEF";

/* Convert Ethernet address to string. */
char *
hsl_etherAddrToStr(char *p, char *str)
{
 int i, j = 0;

  for (i = 0; i < 6; i++) 
    {
    str[j++] = hexChar[(p[i]) >> 4];
    str[j++] = hexChar[(p[i]) & 0x0F];
    str[j++] = ':';
    }
  str[j-1] = '\0';
  return str;
}

/* Convert Ethernet address from string to ethernet structure. */
char *
hsl_etherStrToAddr (char *str, char *p)
{
 int i, j = 0;
 char c;
 u_char b1, b2;

  for (i = 0; i < 6; i++) 
    {
      c = str[j++];
      if (c >= 'a' && c <= 'f')
        c &= 0x0DF;           /* Force to Uppercase. */
      b1 = HEX_TO_DIGIT(c);
      c = str[j++];
      if (c >= 'a' && c <= 'f')
        c &= 0x0DF;           /* Force to Uppercase. */
      b2 = HEX_TO_DIGIT(c);
      p[i] = b1 * 16 + b2;
      j++;             /* Skip a colon. */
    }
  return p;
}
