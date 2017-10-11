/* Copyright (C) 2001-2003  All Rights Reserved. */

#ifndef _PACOS_IMI_SERVER_H
#define _PACOS_IMI_SERVER_H


/* Default Web port. */
#define IMI_SERVER_PORT                         2700

/* Message length. */
#define IMI_SERVER_MESSAGE_MAX_LEN              4096

/* Maxlen of command string. */
#define IMI_SERVER_CMD_STRING_MAX_LEN           128

/* IMI return to web client.
   Note:  Last four (4) bytes of IMI return signals end-of-message.
     - Last 4 bytes are '\0' '\0' '\0' '<ret>'.
   The following values exist for <ret>:  */
#define IMI_OK                  '\1'            /* OK with no reply/message. */
#define IMI_RESPONSE            '\2'            /* OK or ERROR reply message.*/
#define IMI_ERR_FAILED          '\3'            /* Command failed Error. */
#define IMI_ERR_AMBIGUOUS       '\4'            /* Ambiguous command error. */
#define IMI_ERR_UNKNOWN         '\5'            /* Unknown command error. */
#define IMI_ERR_INCOMPLETE      '\6'            /* Incomplete command error. */
#define IMI_ERR_LOCKED          '\7'            /* IMI configuration locked. */


/* IMI returns OK to web client. */
#define IMI_OK_STR          "IMI-OK"
#define IMI_ERR_STR         "IMI-ERROR"

/* Is this a special command? */
#define IS_IMI_SERVER_EXTENDED_CMD              imi_server_extended_cmd_check

/* Skip white space. */
#define SKIP_WHITE_SPACE(P)                               \
  do {                                                    \
    while (pal_char_isspace ((int) * P) && *P != '\0')    \
      P++;                                                \
  } while (0)

/* Read next word in the string. */
#define READ_NEXT_WORD(cp,start,len)                      \
  do {                                                    \
    start = cp;                                           \
    while (!                                              \
           (pal_char_isspace ((int) * cp) || *cp == '\r'  \
            || *cp == '\n') && *cp != '\0')               \
      cp++;                                               \
    len = cp - start;                                     \
  } while (0)

/* Read next word in the string to provided buffer. */
#define READ_NEXT_WORD_TO_BUF(cp,start,buf,len)           \
  do {                                                    \
    start = cp;                                           \
    while (!                                              \
           (pal_char_isspace ((int) * cp) || *cp == '\r'  \
            || *cp == '\n') && *cp != '\0')               \
      cp++;                                               \
    len = cp - start;                                     \
    pal_strncpy (buf, start, len);                        \
    buf[len] = '\0';                                      \
  } while (0)

enum ext_api
{
  EXT_API_SHOW,
  EXT_API_IP,
  EXT_API_NO,
  EXT_API_INTERFACE,
  EXT_API_DHCPS,
  EXT_API_PPPOE,
  EXT_API_CLEAR,
  EXT_API_DEFAULT_GW,
  EXT_API_WAN_STATUS,
  EXT_API_SPECIAL_GET,
  EXT_API_COMMIT,
  EXT_API_RIP_ENABLE,
  EXT_API_RIP_DISABLE,
  EXT_API_RIPNG_ENABLE,
  EXT_API_RIPNG_DISABLE,
  EXT_API_TUNNEL,
  MAX_EXT_API
};

/* Data structures. */

/* IMI Server client. */
struct imi_serv
{
  /* Message buffer. */
  u_char buf[IMI_SERVER_MESSAGE_MAX_LEN];

  /* Message buffer length. */
  u_int16_t len;

  /* Output. */
  char outbuf[IMI_SERVER_MESSAGE_MAX_LEN];
  char *outptr;
  int outlen;

  /* Message pointer for parser. */
  u_char *pnt;

  /* Read thread. */
  struct thread *t_read;

  /* Write thread. */
  struct thread *t_write;

  /* Socket. */
  int sock;

  /* CLI structure. */
  struct cli *cli;
};

/* Extended API node. */
struct imi_ext_api
{
  /* Command string. */
  char *cmd_string;

  /* Callback function. */
  int (*func) (struct imi_serv *);
};

/* Function Prototypes: */

/* IMI Server Init. */
void imi_server_init ();

/* IMI Server Shutdown. */
void imi_server_shutdown ();

/* Execute config command via IMI server. */
int imi_server_config_command (struct imi_serv *, char *);

/* Execute show command via IMI server. */
int imi_server_show_command (struct imi_serv *);

#endif /* _PACOS_IMI_SERVER_H */
