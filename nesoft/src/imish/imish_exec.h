/* Copyright (C) 2003  All Rights Reserved.  */

#ifndef _PACOS_IMISH_EXEC_H
#define _PACOS_IMISH_EXEC_H

/* Internal function.  */
typedef int (*INTERNAL_FUNC) (void *, int, char **);

/* Process structure.  */
struct process
{
  /* Single linked list.  */
  struct process *next;

  /* Execute argv.  */
  char **argv;

  /* Process id of child process.  */
  pid_t pid;

  /* Status.  */
  int status;

  /* Completed.  */
  int completed;

  /* When the command is internal, set this flag.  */
  int internal;

  /* Internal function pointer.  */
  INTERNAL_FUNC func;

  /* Internal function value.  */
  void *func_val;

  /* Internal function argument count.  */
  int func_argc;

  /* Internal function arguments.  */
  char **func_argv;

  /* For redirect of output.  */
  char *path;
};

/* Execute command.  */

int imish_exec (char *command, ...);
int imish_execvp (char **argv);

/* Execute command with modifier.  */

int imish_more (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv);
int imish_begin (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv,
                 char *regexp);
int imish_include (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv,
                   char *regexp);
int imish_exclude (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv,
                   char *regexp);
int imish_grep (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv,
                int argc_modifier, char **argv_modifier);
int imish_redirect (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv,
                    char *path);

/* Stop and wait process.  */

void imish_stop_process ();
void imish_wait_for_process (void);

/* Set pager.  */
void imish_set_pager ();

void imish_exec_show(struct cli *cli);

/* Current running pipe line.  */

extern struct process *process_list;
extern int external_command;

/* Signal set function declaration. */
RETSIGTYPE *imish_signal_set (int, void (*)(int));

#endif /* _PACOS_IMISH_EXEC_H */
