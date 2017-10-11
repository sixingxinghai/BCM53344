/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HAL_SOCKET_H_
#define _HAL_SOCKET_H_

/* Supported address families */
#define AF_LACP                40
#define AF_EAPOL               41
#define AF_STP                 42
#define AF_HAL                 43
#define AF_GARP                44
#define AF_IGMP_SNOOP          45
#define AF_MLD_SNOOP           46
#define AF_LLDP                47
#define AF_CFM                 49
#define AF_EFM                 50 
#define AF_ELMI                51 
#define AF_PROTO_STP           1
#define AF_PROTO_RSTP          2
#define AF_PROTO_MSTP          3

enum hal_l2_proto_sock_id
{
  HAL_SOCK_PROTO_NSM,
  HAL_SOCK_PROTO_MSTP,
  HAL_SOCK_PROTO_LACP,
  HAL_SOCK_PROTO_IGMP_SNOOP,
  HAL_SOCK_PROTO_MLD_SNOOP,
  HAL_SOCK_PROTO_GARP,
  HAL_SOCK_PROTO_8021X,
  HAL_SOCK_PROTO_CFM,
  HAL_SOCK_PROTO_EFM,
  HAL_SOCK_PROTO_LLDP,
  HAL_SOCK_PROTO_ELMI,
  HAL_SOCK_PROTO_MAX,
};

typedef struct sockaddr_l2 sockaddr_stp_t;
typedef struct sockaddr_l2 sockaddr_lacp_t;
typedef struct sockaddr_l2 sockaddr_eapol_t;
typedef struct sockaddr_l2 sockaddr_garp_t;
typedef struct sockaddr_l2 sockaddr_pae_t;
typedef struct sockaddr_igs sockaddr_igs_t;

#endif /* _HAL_SOCKET_H_ */
