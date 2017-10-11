/* Copyright (C) 2003  All Rights Reserved.  */

#ifndef _VTYSH_CLI_H
#define _VTYSH_CLI_H

#define PING_IPV4                      0
#define PING_IPV6                      1
#define TRACEROUTE_IPV4                0
#define TRACEROUTE_IPV6                1

/* These could be different by platform.  */
#define SHELL_PROG_NAME                "bash"

/* Output modifier function return value.  */
#define VTYSH_MODIFIER_BEGIN        0
#define VTYSH_MODIFIER_INCLUDE      1
#define VTYSH_MODIFIER_EXCLUDE      2
#define VTYSH_MODIFIER_GREP         3
#define VTYSH_MODIFIER_REDIRECT     4

/* Skip white space. */
#define SKIP_WHITE_SPACE(P) \
  do { \
       while (pal_char_isspace ((int) * P) && *P != '\0') \
         P++; \
   } while (0)

/* Init functions.  */
void vtysh_cli_init ();
void vtysh_cli_show_init ();

/* Extracted command initializer. */
void vtysh_command_init ();

extern struct cli_tree *ctree;

#endif /* _VTYSH_CLI_H */
