/* Copyright (C) 2003  All Rights Reserved. */

#ifndef _PACOS_IMI_CONFIG_H
#define _PACOS_IMI_CONFIG_H

#define IMI_BUFSIZ                              8192

enum imi_config_type
  {
    imi_config_type_add,
    imi_config_type_add_unique,
    imi_config_type_sub
  };

/* Configuration linked list.  */
struct imi_config_fifo
{
  struct imi_config_line *next;
  struct imi_config_line *prev;
};

/* Configuration linked list.  */
struct imi_config_sub_fifo
{
  struct imi_config_mode *next;
  struct imi_config_mode *prev;
};

/* Mode's configuration storage.  */
struct imi_config_mode
{
  struct imi_config_mode *next;
  struct imi_config_mode *prev;

  /* Which module this command belongs to.  */
  u_int32_t pm;

  /* Index string.  */
  char *str;

  /* Main configuration.  */
  struct imi_config_fifo main;

  /* Match statement.  */
  struct imi_config_fifo match;

  /* Set statement.  */
  struct imi_config_fifo set;

  /* Sub configuration. */
  struct imi_config_sub_fifo sub;
};

/* Configuration string line.  */
struct imi_config_line
{
  /* Linked list.  */
  struct imi_config_line *next;
  struct imi_config_line *prev;

 /* Which module this command belongs to.  */
  u_int32_t pm;

  /* Configuration string.  */
  char *str;
};

/* IMI configuration.  */
struct imi_config
{
  /* Top.  */
  struct imi_state *top;

  /* Each mode's configuration. */
  vector modes;

  /* Current mode.  Used during parse.  */
  int mode;

  /* Current state.  */
  struct imi_config_mode *cm;
};

/* Macros.  */
#ifdef HAVE_IMISH

#define IMI_STARTUP_CONFIG(Z, F)                                              \
    do {                                                                      \
      /* Set the start-up configuration file.  */                             \
      host_startup_config_file_set (apn_vr_get_privileged (Z), F);            \
                                                                              \
      /* Read the configuration file.  */                                     \
      imi_config_read (apn_vr_get_privileged (Z));                            \
    } while (0);

#define IMI_LINE_CONFIG_WRITE(C)                                              \
    imi_line_config_write(C)

#else

#define IMI_STARTUP_CONFIG(Z, F)                                              \
    do {                                                                      \
      /* Set the start-up configuration file.  */                             \
      host_startup_config_file_set (apn_vr_get_privileged (Z), F);            \
                                                                              \
      /* Read the configuration file.  */                                     \
      host_config_read (apn_vr_get_privileged (Z));                           \
    } while (0);

#define IMI_LINE_CONFIG_WRITE(C)                                              \
    vty_config_write (C)

#endif /* HAVE_IMISH */

/* Function Prototypes.  */
void imi_config_init (struct lib_globals *);
int imi_config_write (struct cli *, char *,int);

#ifdef HAVE_VR
int imi_vr_config_encode (cfg_vect_t *cv);
#endif

#ifdef HAVE_IMISH
int imi_line_config_encode (struct imi_master *im, cfg_vect_t *cv);
#endif

#endif /* _PACOS_IMI_CONFIG_H */
