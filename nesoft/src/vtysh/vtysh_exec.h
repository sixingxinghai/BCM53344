/* Copyright (C) 2003  All Rights Reserved.  */

#ifndef _VTYSH_EXEC_H
#define _VTYSH_EXEC_H

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

int vtysh_exec (char *command, ...);
int vtysh_execvp (char **argv);

/* Execute command with modifier.  */

int vtysh_more (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv);
int vtysh_begin (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv,
                 char *regexp);
int vtysh_include (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv,
                   char *regexp);
int vtysh_exclude (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv,
                   char *regexp);
int vtysh_grep (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv,
                int argc_modifier, char **argv_modifier);
int vtysh_redirect (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv,
                    char *path);

/* Stop and wait process.  */

void vtysh_stop_process ();
void vtysh_wait_for_process (void);

/* Set pager.  */
void vtysh_set_pager ();

/* Current running pipe line.  */

extern struct process *process_list;

#endif /* _VTYSH_EXEC_H */
