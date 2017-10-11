/* Copyright (C) 2003  All Rights Reserved. */

#ifndef _VTYSH_CONFIG_H
#define _VTYSH_CONFIG_H

#define VTYSH_BUFSIZ                            8192

#define VTYSH_DEFAULT_CONFIG_FILE "/usr/local/etc/PacOS.conf"

enum vtysh_config_type
  {
    vtysh_config_type_add,
    vtysh_config_type_add_unique,
    vtysh_config_type_sub
  };

/* Configuration linked list.  */
struct vtysh_config_fifo
{
  struct vtysh_config_line *next;
  struct vtysh_config_line *prev;
};

/* Configuration linked list.  */
struct vtysh_config_sub_fifo
{
  struct vtysh_config_mode *next;
  struct vtysh_config_mode *prev;
};

/* Mode's configuration storage.  */
struct vtysh_config_mode
{
  struct vtysh_config_mode *next;
  struct vtysh_config_mode *prev;

  /* Which module this command belongs to.  */
  u_int32_t pm;

  /* Index string.  */
  char *str;

  /* Main configuration.  */
  struct vtysh_config_fifo main;

  /* Match statement.  */
  struct vtysh_config_fifo match;

  /* Set statement.  */
  struct vtysh_config_fifo set;

  /* Sub configuration. */
  struct vtysh_config_sub_fifo sub;
};

/* Configuration string line.  */
struct vtysh_config_line
{
  /* Linked list.  */
  struct vtysh_config_line *next;
  struct vtysh_config_line *prev;

 /* Which module this command belongs to.  */
  u_int32_t pm;

  /* Configuration string.  */
  char *str;
};

/* VTYSH configuration.  */
struct vtysh_config
{
  /* Top.  */
  struct vtysh_state *top;

  /* Each mode's configuration. */
  vector modes;

  /* Current mode.  Used during parse.  */
  int mode;

  /* Current state.  */
  struct vtysh_config_mode *cm;
};

/* Function Prototypes. */

/* Lookup/Get */
struct vtysh_config_mode *vtysh_config_lookup (struct vtysh_config *, int);
struct vtysh_config_mode *vtysh_config_get (struct vtysh_config *, int, char *,
                                        enum vtysh_config_type);

/* New mode. */
struct vtysh_config_mode *vtysh_config_mode_new ();

/* Write global PacOS configuration to cli. */
void vtysh_config_write (struct cli *cli);

/* Load the configuration. */
int vtysh_startup_config (char *config_file, int);

/* Write configuration to the file.  */
int vtysh_write_config ();

/* Execute command over VTYSH command tree. */
int vtysh_startup_execute (struct cli_node *, struct cli *, char *, int);

/* Init. */
void vtysh_config_init ();

#endif /* _VTYSH_CONFIG_H */
