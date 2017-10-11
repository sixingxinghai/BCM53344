/* Copyright (C) 2003  All Rights Reserved.  */

#ifndef _IMISH_CLI_H
#define _IMISH_CLI_H

#define PING_IPV4                      0
#define PING_IPV6                      1
#define TRACEROUTE_IPV4                0
#define TRACEROUTE_IPV6                1

/* These could be different by platform.  */
#ifndef SHELL_PROG_NAME
#define SHELL_PROG_NAME                "bash"
#endif /* SHELL_PROG_NAME */

/* Output modifier function return value.  */
#define IMISH_MODIFIER_BEGIN        0
#define IMISH_MODIFIER_INCLUDE      1
#define IMISH_MODIFIER_EXCLUDE      2
#define IMISH_MODIFIER_GREP         3
#define IMISH_MODIFIER_REDIRECT     4

/* Skip white space. */
#define SKIP_WHITE_SPACE(P) \
  do { \
       while (pal_char_isspace ((int) * P) && *P != '\0') \
         P++; \
   } while (0)

/* Init functions.  */
void imish_cli_init (struct lib_globals *);
void imish_cli_show_init ();

/* Extracted command initializer. */
void imish_command_init ();

#ifndef HAVE_CUSTOM1
void imish_tech_cli_init();
#endif

/* Utility function.  */
int imish_yes_or_no ();

extern struct cli_tree *ctree;

#endif /* _IMISH_CLI_H */
