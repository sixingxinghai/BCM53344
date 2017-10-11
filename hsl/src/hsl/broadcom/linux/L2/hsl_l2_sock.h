/* Copyright (C) 2004   All Rights Reserved. */

#ifndef _HSL_L2_SOCK_H
#define _HSL_L2_SOCK_H

#include<linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
#include<linux/nsproxy.h>
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27) */

#define HSL_L2_ETH_P_PAE                0x888e
#define HSL_L2_AUTH_HDR_LEN             4

#define HSL_L2_ETH_P_LACP               0x8809

#define HSL_L2_ETH_P_8021Q              0x8100

#ifdef LINUX_KERNEL_2_6
#define HSL_SOCK_DESTRUCT(sk, app) do{                      \
  write_lock_bh (&app##_socklist_lock);                     \
  sk_del_node_init(sk);                                     \
  write_unlock_bh (&app##_socklist_lock);                   \
  /*                                                        \
   *    Now the socket is dead. No more input will appear.  \
   */                                                       \
  sock_orphan (sk);                                         \
  /* Purge queues */                                        \
  skb_queue_purge (&sk->sk_receive_queue);                  \
  sock_put (sk);                                            \
}while(0)
#else
#define HSL_SOCK_DESTRUCT(sk, app) do{                      \
  struct sock **skp;                                        \
                                                            \
  write_lock_bh (&app##_socklist_lock);                     \
  for (skp = &app##_socklist; *skp; skp = &(*skp)->next)    \
    {                                                       \
      if (*skp == sk)                                       \
        {                                                   \
          *skp = sk->next;                                  \
          break;                                            \
        }                                                   \
    }                                                       \
  write_unlock_bh (&app##_socklist_lock);                   \
  /*                                                        \
   *    Now the socket is dead. No more input will appear.  \
   */                                                       \
  sock_orphan (sk);                                         \
                                                            \
  /* Purge queues */                                        \
  skb_queue_purge (&sk->receive_queue);                     \
                                                            \
  sock_put (sk);                                            \
}while(0)
#endif  

#ifdef LINUX_KERNEL_2_6
#define HSL_SOCK_RELEASE(sk, app) do {                     \
  write_lock_bh (&app##_socklist_lock);                    \
  sk_del_node_init(sk);                                    \
  write_unlock_bh (&app##_socklist_lock);                  \
                                                           \
  /*                                                       \
   *    Now the socket is dead. No more input will appear. \
   */                                                      \
                                                           \
  sock_orphan (sk);                                        \
  sock->sk = NULL;                                         \
                                                           \
  /* Purge queues */                                       \
  skb_queue_purge (&sk->sk_receive_queue);                 \
                                                           \
  sock_put (sk);                                           \
}while(0)
#else
#define HSL_SOCK_RELEASE(sk, app) do {                     \
  struct sock **skp;                                       \
  write_lock_bh (&app##_socklist_lock);                    \
  for (skp = &app##_socklist; *skp; skp = &(*skp)->next)   \
    {                                                      \
      if (*skp == sk)                                      \
        {                                                  \
          *skp = sk->next;                                 \
          break;                                           \
        }                                                  \
    }                                                      \
  write_unlock_bh (&app##_socklist_lock);                  \
                                                           \
  /*                                                       \
   *    Now the socket is dead. No more input will appear. \
   */                                                      \
                                                           \
  sock_orphan (sk);                                        \
  sock->sk = NULL;                                         \
                                                           \
  /* Purge queues */                                       \
  skb_queue_purge (&sk->receive_queue);                    \
                                                           \
  sock_put (sk);                                           \
}while(0)
#endif 

#if (LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27))

#define HSL_SOCK_ALLOC(net, sock, _family, _protocol, app) do{  \
  struct sock *sk;                                         \
  sk = sk_alloc (net, _family, GFP_KERNEL, &app##_prot);    \
  if (sk == NULL)                                          \
    {                                                      \
      return -ENOBUFS;                                     \
    }                                                      \
                                                           \
  sock_init_data (sock,sk);                                \
                                                           \
  sk->sk_family = _family;                                 \
  sk->sk_protocol = _protocol;                             \
  sk->sk_destruct = app##_sock_destruct;                   \
  sock_hold (sk);                                          \
                                                           \
  write_lock_bh (&app##_socklist_lock);                    \
  sk_add_node(sk, &app##_socklist);                        \
  write_unlock_bh (&app##_socklist_lock);                  \
}while(0)

#else /* !LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */

#ifdef LINUX_KERNEL_2_6

#define HSL_SOCK_ALLOC(sock, _family, _protocol, app) do{  \
  struct sock *sk;                                         \
  sk = sk_alloc (_family, GFP_KERNEL, &app##_prot, 1);     \
  if (sk == NULL)                                          \
    {                                                      \
      return -ENOBUFS;                                     \
    }                                                      \
                                                           \
  sock_init_data (sock,sk);                                \
                                                           \
  sk->sk_family = _family;                                 \
  sk->sk_protocol = _protocol;                             \
  sk->sk_destruct = app##_sock_destruct;                   \
  sock_hold (sk);                                          \
                                                           \
  write_lock_bh (&app##_socklist_lock);                    \
  sk_add_node(sk, &app##_socklist);                        \
  write_unlock_bh (&app##_socklist_lock);                  \
}while(0)
#else /* !LINUX_KERNEL_2_6 */

#define HSL_SOCK_ALLOC(sock, _family, _protocol, app) do{  \
  struct sock *sk;                                         \
  sk = sk_alloc (_family, GFP_KERNEL, 1);                  \
  if (sk == NULL)                                          \
    {                                                      \
      return -ENOBUFS;                                     \
    }                                                      \
                                                           \
  sock_init_data (sock,sk);                                \
                                                           \
  sk->family = _family;                                    \
  sk->num = _protocol;                                     \
  sk->destruct = app##_sock_destruct;                      \
  sock_hold (sk);                                          \
                                                           \
  write_lock_bh (&app##_socklist_lock);                    \
  sk->next = app##_socklist;                               \
  app##_socklist = sk;                                     \
  write_unlock_bh (&app##_socklist_lock);                  \
}while(0)
#endif /*  LINUX_KERNEL_2_6 */

#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,27) */

#ifdef LINUX_KERNEL_2_6
#define HSL_SOCK_LIST_LOOP(sk, app)                        \
  for (sk = sk_head(&app##_socklist); sk; sk = sk_next(sk))

#define HSL_CHECK_SOCK_DEAD(sk)                            \
        (sock_flag(sk, SOCK_DEAD))

#define HSL_CHECK_SOCK_RECV_BUFF(sk, truesize)             \
  (atomic_read (&sk->sk_rmem_alloc) + truesize >= (unsigned)sk->sk_rcvbuf)

#else /* LINUX_KERNEL_2_6 */

#define HSL_SOCK_LIST_LOOP(sk, app)                        \
  for (sk = app##_socklist; sk; sk = sk->next)

#define HSL_CHECK_SOCK_DEAD(sk)                            \
        (sk->dead)

#define HSL_CHECK_SOCK_RECV_BUFF(sk, truesize)             \
  (atomic_read (&sk->rmem_alloc) + truesize >= (unsigned)sk->rcvbuf)

#endif /* LINUX_KERNEL_2_6 */

static struct sk_buff *_hsl_bcm_rx_handle_802_3_tagged (struct hsl_if *ifp, bcm_pkt_t *pkt);

int hsl_bcm_rx_handle_bpdu (struct hsl_if *ifp, bcm_pkt_t *pkt);
int hsl_bcm_rx_handle_eapol (struct hsl_if *ifp, bcm_pkt_t *pkt);
int hsl_bcm_rx_handle_lacp (struct hsl_if *ifp, bcm_pkt_t *pkt);
int hsl_bcm_rx_handle_gmrp (struct hsl_if *ifp, bcm_pkt_t *pkt);
int hsl_bcm_rx_handle_gvrp (struct hsl_if *ifp, bcm_pkt_t *pkt);
int hsl_bcm_rx_handle_igs (struct hsl_if *ifp, bcm_pkt_t *pkt);
int hsl_bcm_rx_handle_mlds (struct hsl_if *ifp, bcm_pkt_t *pkt);
int hsl_bcm_rx_handle_lldp (struct hsl_if *ifp, bcm_pkt_t *pkt);
int hsl_bcm_rx_handle_efm (struct hsl_if *ifp, bcm_pkt_t *pkt);
int hsl_bcm_rx_handle_cfm (struct hsl_if *ifp, bcm_pkt_t *pkt);
int hsl_sock_post_skb (struct sock *sk, struct sk_buff *skb);
int hsl_af_stp_post_packet (struct sk_buff *skb);
int hsl_af_lacp_post_packet (struct sk_buff *skb);
int hsl_af_eapol_post_packet (struct sk_buff *skb);
int hsl_af_garp_post_packet (struct sk_buff *skb);
int hsl_af_igs_post_packet (struct sk_buff *skb);
int hsl_af_mlds_post_packet (struct sk_buff *skb);
int hsl_af_efm_post_packet (struct sk_buff *skb);
int hsl_af_lldp_post_packet (struct sk_buff *skb);
int hsl_af_cfm_post_packet (struct sk_buff *skb);
int hsl_l2_sock_init ();
int hsl_l2_sock_deinit ();
#ifdef HAVE_MAC_AUTH
int hsl_bcm_rx_handle_auth_mac (struct hsl_if *ifp, bcm_pkt_t *pkt);
#endif /* HAVE_MAC_AUTH */

#endif /* _HSL_L2_SOCK_H_ */
