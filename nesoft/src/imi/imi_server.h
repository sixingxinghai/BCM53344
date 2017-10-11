/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _PACOS_IMI_SERVER_H
#define _PACOS_IMI_SERVER_H

/*
    +-------- imi_server --------+
    | Message Handler            |
    | Vector of imi_server_client|
    +--------------|-------------+
                   |
                   v
    +------- imi_server_client ------+
    |  IMI client module ID          |-+
    |  List of each imi_server_entry | |-+
    +--------------------------------+ | |
      +--------------------------------+ |
        +--------------------------------+
*/

#define IMI_SERVER_CLIENT_VEC_MIN_SIZE                          4

/* IMI server structure.  */
struct imi_server
{
  /* Message handler.  */
  struct message_handler *ms;

  /* Vector for IMI server client.  */
  vector client;
};

/* IMI server entry for each client connection.  */
struct imi_server_entry
{
  /* Linked list.  */
  struct imi_server_entry *next;
  struct imi_server_entry *prev;

  /* Pointer to message entry.  */
  struct message_entry *me;

  /* Pointer to NSM server structure.  */
  struct imi_server *is;

  /* Pointer to NSM server client.  */
  struct imi_server_client *isc;

  /* Connect time.  */
  pal_time_t connect_time;

  /* Line info.  */
  struct line line;
};

/* IMI server client which contains multiple IMI client entries
   indexed by protocol module ID.  */
struct imi_server_client
{
  /* Protocol Module ID. */
  module_id_t module_id;

  /* IMI server entry for sync connection.  */
  struct imi_server_entry *head;
  struct imi_server_entry *tail;
};


int imi_server_send_line (struct imi_server_entry *,
                          struct line *, struct line *);
int imi_server_proxy (struct line *, struct line *, bool_t do_mcast);
int imi_server_init (struct lib_globals *);
int imi_server_shutdown (struct lib_globals *);
void imi_server_flush_module(struct line *line);
u_int16_t imi_server_check_index (struct lib_globals *zg,  void *index,
                                  void *sub_index, s_int32_t mode);
void imi_server_update_index (struct lib_globals *zg,  void *index,
                              void *sub_index, s_int32_t mode,  void *new_index,
                              void *new_sub_index);
int imi_server_line_pm_connect (struct apn_vr *vr);
ZRESULT imi_server_line_pm_send (module_id_t , struct line *);

int imi_server_send_notify (struct imi_server_entry *ise,
                            struct line *line, u_char code);

#endif /* _PACOS_IMI_SERVER_H */
