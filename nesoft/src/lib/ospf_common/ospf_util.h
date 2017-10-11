/* Copyright (C) 2002  All Rights Reserved. */

#ifndef _PACOS_OSPF_UTIL_H
#define _PACOS_OSPF_UTIL_H

/* Macros. */
#define OSPF_PNT_UCHAR_GET(P,O)                                               \
    (*(((u_char *)(P)) + (O)))

#define OSPF_PNT_UINT16_GET(P,O)                                              \
    (((*(((u_char *)(P)) + (O))) << 8) +                                      \
     ((*(((u_char *)(P)) + (O) + 1))))

#define OSPF_PNT_UINT24_GET(P,O)                                              \
    (((*(((u_char *)(P)) + (O))) << 16) +                                     \
     ((*(((u_char *)(P)) + (O) + 1)) << 8) +                                  \
     ((*(((u_char *)(P)) + (O) + 2))))

#define OSPF_PNT_UINT32_GET(P,O)                                              \
    (((*(((u_char *)(P)) + (O))) << 24) +                                     \
     ((*(((u_char *)(P)) + (O) + 1)) << 16) +                                 \
     ((*(((u_char *)(P)) + (O) + 2)) << 8) +                                  \
     ((*(((u_char *)(P)) + (O) + 3))))

#define OSPF_PNT_IN_ADDR_GET(P,O)                                             \
    (struct pal_in4_addr *)(((u_char *)(P)) + (O))

/* CLI macro to convert string to OSPF area ID.  */
#define CLI_GET_OSPF_AREA_ID(V,F,STR)                                         \
{                                                                             \
  int _ret;                                                                   \
  _ret = ospf_str2area_id ((STR), &(V), &(F));                                \
  if (_ret < 0)                                                               \
    {                                                                         \
      cli_out (cli, "%% Invalid OSPF area ID\n");                             \
      return CLI_ERROR;                                                       \
    }                                                                         \
}

#define CLI_GET_OSPF_AREA_ID_NO_BB(NAME,V,F,STR)                              \
{                                                                             \
  int _ret;                                                                   \
  _ret = ospf_str2area_id ((STR), &(V), &(F));                                \
  if (_ret < 0)                                                               \
    {                                                                         \
      cli_out (cli, "%% Invalid OSPF area ID\n");                             \
      return CLI_ERROR;                                                       \
    }                                                                         \
  if (IS_AREA_ID_BACKBONE ((V)))                                              \
    {                                                                         \
      cli_out (cli, "%% You can't configure %s to backbone\n",                \
               NAME);                                                         \
    }                                                                         \
}

int ospf_str2area_id (char *, struct pal_in4_addr *, int *);
int ospf_str2metric (char *, int *);
int ospf_str2metric_type (char *, int *);
int ospf_str2network_type (char *, int *);

#endif /* _PACOS_OSPF_UTIL_H */
