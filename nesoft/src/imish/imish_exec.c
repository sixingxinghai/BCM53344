/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>
#include <sys/wait.h>

#include "imish_cli.h"
#include "imish/imish_exec.h"
#if defined(__NetBSD__)
#include "imish/libedit/readline/readline.h"
#endif /* __NetBSD__ */


/* IMI shell process execution routines.  */

/* When WAIT_ANY is defined, use it.  */
#ifdef WAIT_ANY
#define CHILD_PID         WAIT_ANY
#else
#define CHILD_PID         -1
#endif /* WAIT_ANY. */

/* Define default pager.  */
char *pager = "more";

/* Maintain current process list.  */
struct process *process_list;
int external_command = 0;
#ifdef HAVE_CUSTOM1
extern int execflag2;
#endif /* HAVE_CUSTOM1 */

/* Memory type for child process argv.  */
#define MTYPE_IMISH_EXEC_ARGV    MTYPE_TMP
#define MTYPE_IMISH_PROCESS      MTYPE_TMP

/* Allocate a new process for pipeline.  */
struct process *
imish_pipe_add (struct process *prev)
{
  struct process *proc;

  proc = XCALLOC (MTYPE_IMISH_PROCESS, sizeof (struct process));
  if (! proc)
    return NULL;

  if (prev)
    prev->next = proc;

  return proc;
}

/* Free pipeline memory.  */
static void
imish_pipe_free (struct process *proc)
{
  struct process *next;

  while (proc)
    {
      next = proc->next;
      XFREE (MTYPE_IMISH_PROCESS, proc);
      proc = next;
    }
}

/* Return 1 when all child processes are completed.  */
static int
imish_process_completed ()
{
  struct process *p;

  for (p = process_list; p; p = p->next)
    if (! p->completed)
      return 0;
  return 1;
}

/* Mark process stautus.  */
static int
imish_update_process_status (pid_t pid, int status)
{
  struct process *p;

  if (pid > 0)
    {
      for (p = process_list; p; p = p->next)
        if (p->pid == pid)
          {
            p->status = status;

            if (! WIFSTOPPED (status))
              p->completed = 1;

            return 0;
          }
      fprintf (stderr, "No child process %d\n", pid);

      return -1;
    }
  else if (pid == 0 || errno == ECHILD)
    return -1;
  else
    {
      perror ("waitpid");
      return -1;
    }
}

/* Exec child process.  */
static void
imish_exec_process (struct process *p, int in_fd, int out_fd, int err_fd)
{
  /* Set the handler to the default.  */
  imish_signal_set (SIGINT, SIG_DFL);
  imish_signal_set (SIGQUIT, SIG_DFL);
  imish_signal_set (SIGCHLD, SIG_DFL);
  imish_signal_set (SIGALRM, SIG_DFL);
  imish_signal_set (SIGPIPE, SIG_DFL);
  imish_signal_set (SIGUSR1, SIG_DFL);

  /* No job control.  */
  imish_signal_set (SIGTSTP, SIG_IGN);
  imish_signal_set (SIGTTIN, SIG_IGN);
  imish_signal_set (SIGTTOU, SIG_IGN);

  /* Standard input, output and error.  */
  if (in_fd != PAL_STDIN_FILENO)
    {
      if (dup2 (in_fd, PAL_STDIN_FILENO) < 0)
      {
          perror ("dup2(in_fd)");
          exit (1);
      }
      close (in_fd);
    }
  if (out_fd != PAL_STDOUT_FILENO)
    {
      if (dup2 (out_fd, PAL_STDOUT_FILENO) < 0)
      {
          perror ("dup2(out_fd)");
          exit (1);
      }
      close (out_fd);
    }
  if (err_fd != PAL_STDERR_FILENO)
    {
      if (dup2 (err_fd, PAL_STDERR_FILENO) < 0)
      {
          perror ("dup2(err_fd)");
          exit (1);
      }
      close (err_fd);
    }

  /* Internal process.  */
  if (p->internal)
    {
      (*(p->func))(p->func_val, p->func_argc, p->func_argv);
      exit (0);
    }

#ifdef HAVE_CUSTOM1
  /* Exec new process.  */
  if (execflag2 == 2)
    {
      printf ("\n");
      exit (1);
    }
#endif /* HAVE_CUSTOM1 */

  execvp (p->argv[0], p->argv);

  /* Not reached heer unless error occurred.  */
  perror ("execvp");
  exit (1);
}

/* Wait all of child processes.  Clear process list after all of child
   process is finished.  */
void
imish_wait_for_process ()
{
  pid_t pid;
  int status;

  do
    pid = waitpid (CHILD_PID, &status, WUNTRACED);
  while (imish_update_process_status (pid, status) == 0
         && imish_process_completed () == 0);

  imish_pipe_free (process_list);
  process_list = NULL;

  if (external_command)
    {
      external_command = 0;
#if defined(__NetBSD__)
      readline_set_time ();
#endif /* __NetBSD__ */
    }
}

/* Run pipe lines.  */
static void
imish_pipe_line (struct process *process)
{
  int in_fd;
  int out_fd;
  int err_fd;
  int pipe_fd[2];
  int ret;
  pid_t pid;
  struct process *p;

  in_fd = PAL_STDIN_FILENO;
  err_fd = PAL_STDERR_FILENO;
  process_list = process;

  for (p = process; p; p = p->next)
    {
      /* Prepare pipe.  */
      if (p->next)
        {
          ret = pipe (pipe_fd);
          if (ret < 0)
            {
              perror ("pipe");
              return;
            }
          out_fd = pipe_fd[1];
        }
      else
        {
          /* Redirection of output.  */
          if (p->path)
            {
              out_fd = open (p->path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
              if (out_fd < 0)
                {
                  fprintf (stderr, "%% Can't open output file %s\n", p->path);
                  return;
                }
            }
          else
            out_fd = PAL_STDOUT_FILENO;
        }

      /* Fork. */
      pid = fork ();
      if (pid < 0)
        {
          perror ("fork");
          return;
        }
      if (pid == 0)
        {
          /* Pipe's incoming fd is not used by child process.  */
          if (p->next)
            close (pipe_fd[0]);

          /* This is child process.  */
          imish_exec_process (p, in_fd, out_fd, err_fd);
        }
      else
        /* This is parent process.  */
        p->pid = pid;

      if (in_fd != PAL_STDIN_FILENO)
        close (in_fd);
      if (out_fd != PAL_STDOUT_FILENO)
        close (out_fd);
      in_fd = pipe_fd[0];
    }

  /* Wait for all of child processes.  */
  imish_wait_for_process ();
}

/* Send SIGKILL to all of child processes.  */
void
imish_stop_process ()
{
  struct process *p;

  if (process_list)
    {
      for (p = process_list; p; p = p->next)
        if (p->pid > 0 && ! p->completed)
          kill (p->pid, SIGKILL);

      imish_wait_for_process ();
    }
}

/* Execute command in child process.  */
int
imish_exec (char *command, ...)
{
  va_list args;
  int argc;
  char **argv;
  struct process *proc;

  /* Count arguments.  */
  va_start (args, command);
  for (argc = 1; va_arg (args, char *) != NULL; ++argc)
    continue;
  va_end (args);

  /* Malloc argv. Add 1 for NULL termination in last argv.  */
  argv = XCALLOC (MTYPE_IMISH_EXEC_ARGV, (argc + 1) * sizeof (*argv));
  if (! argv)
    return -1;

  /* Copy arguments to argv. */
  argv[0] = command;

  va_start (args, command);
  for (argc = 1; (argv[argc] = va_arg (args, char *)) != NULL; ++argc)
    continue;
  va_end (args);

  proc = imish_pipe_add (NULL);
  if (! proc)
    return -1;
  proc->argv = argv;

  /* Execute command. */
  external_command = 1;
  imish_pipe_line (proc);

  /* Free argv. */
  XFREE (MTYPE_IMISH_EXEC_ARGV, argv);

  return 0;
}

/* Execute command in child process.  Arguemnt 'argv' is passed to
   execvp().  */
int
imish_execvp (char **argv)
{
  struct process *proc;

  proc = imish_pipe_add (NULL);
  if (! proc)
    return -1;
  proc->argv = argv;

  /* Execute command. */
  external_command = 1;
  imish_pipe_line (proc);

  return 0;
}

/* Set internal func process.  */
static struct process *
imish_internal_pipe (INTERNAL_FUNC func, void *val, int argc, char **argv)
{
  struct process *proc;

  proc = imish_pipe_add (NULL);
  if (! proc)
    return NULL;

  proc->func = func;
  proc->func_val = val;
  proc->func_argc = argc;
  proc->func_argv = argv;
  proc->internal = 1;

  return proc;
}

/* More for show commands.  */
int
imish_more (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv)
{
  struct process *proc_top;
  struct process *proc;
  char *argv_more[2];

  /* Initialize more array. */
  argv_more[0] = pager;
  argv_more[1] = NULL;

  /* Internal command.  */
  proc_top = proc = imish_internal_pipe (func, cli, argc, argv);
  if (! proc_top)
    return -1;

  /* More */
  if (cli->lines)
    {
      proc = imish_pipe_add (proc);
      if (! proc)
        return -1;
      proc->argv = argv_more;
    }

  imish_pipe_line (proc_top);

  return 0;
}

/* Output modifier begin.  */
int
imish_begin (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv,
             char *regexp)
{
  struct process *proc_top;
  struct process *proc;
  char *argv_more[3];
  char *option = NULL;

  /* Initialize more array. */
  argv_more[0] = pager;
  argv_more[1] = NULL;
  argv_more[2] = NULL;

  /* Internal command.  */
  proc_top = proc = imish_internal_pipe (func, cli, argc, argv);
  if (! proc_top)
    return -1;

  /* More */
  proc = imish_pipe_add (proc);
  if (! proc)
    return -1;

  /* Prepare string for "more +/regexp".  */
  option = XCALLOC (MTYPE_TMP, strlen ("+/") + strlen (regexp) + 1);
  if (! option)
    return -1;
  strcat (option, "+/");
  strcat (option, regexp);
  argv_more[1] = option;
  proc->argv = argv_more;

  imish_pipe_line (proc_top);

  XFREE (MTYPE_TMP, option);

  return 0;
}

/* Output modifier include.  */
int
imish_include (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv,
               char *regexp)
{
  struct process *proc_top;
  struct process *proc;
  char *argv_grep[3];
  char *argv_more[2];

  /* Initialize grep array. */
  argv_grep[0] = "egrep";
  argv_grep[1] = regexp;
  argv_grep[2] = NULL;

  /* Initialize more array. */
  argv_more[0] = pager;
  argv_more[1] = NULL;

  /* Internal command.  */
  proc_top = proc = imish_internal_pipe (func, cli, argc, argv);
  if (! proc_top)
    return -1;

  /* egrep.  */
  proc = imish_pipe_add (proc);
  if (! proc)
    return -1;
  proc->argv = argv_grep;

  /* More */
  if (cli->lines)
    {
      proc = imish_pipe_add (proc);
      if (! proc)
        return -1;
      proc->argv = argv_more;
    }

  imish_pipe_line (proc_top);

  return 0;
}

/* Output modifier exclude.  */
int
imish_exclude (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv,
               char *regexp)
{
  struct process *proc_top;
  struct process *proc;
  char *argv_grep[4];
  char *argv_more[2];

  /* Initialize grep array. */
  argv_grep[0] = "egrep";
  argv_grep[1] = "-v";
  argv_grep[2] = regexp;
  argv_grep[3] = NULL;

  /* Initialize more array. */
  argv_more[0] = pager;
  argv_more[1] = NULL;

  /* Internal command.  */
  proc_top = proc = imish_internal_pipe (func, cli, argc, argv);
  if (! proc_top)
    return -1;

  /* Exclude using "egrep -v".  */
  proc = imish_pipe_add (proc);
  if (! proc)
    return -1;
  proc->argv = argv_grep;

  /* More */
  if (cli->lines)
    {
      proc = imish_pipe_add (proc);
      if (! proc)
        return -1;
      proc->argv = argv_more;
    }

  imish_pipe_line (proc_top);

  return 0;
}

/* Output modifier grep (hidden command).  */
int
imish_grep (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv,
            int argc_modifier, char **argv_modifier)
{
  struct process *proc_top;
  struct process *proc;
  char *argv_grep[4];
  char *argv_more[2];

  /* Internal command.  */
  proc_top = proc = imish_internal_pipe (func, cli, argc, argv);
  if (! proc_top)
    return -1;

  /* Pipe for grep".  */
  proc = imish_pipe_add (proc);
  if (! proc)
    return -1;

  /* Prepare grep command.  */
  argv_grep[0] = "grep";
  if (argc_modifier == 1)
    {
      argv_grep[1] = argv_modifier[0];
      argv_grep[2] = NULL;
    }
  else if (argc_modifier == 2)
    {
      argv_grep[1] = argv_modifier[0];
      argv_grep[2] = argv_modifier[1];
      argv_grep[3] = NULL;
    }
  proc->argv = argv_grep;

  /* Initialize more array. */
  argv_more[0] = pager;
  argv_more[1] = NULL;

  if (cli->lines)
    {
      proc = imish_pipe_add (proc);
      if (! proc)
        return -1;
      proc->argv = argv_more;
    }

  imish_pipe_line (proc_top);

  return 0;
}

/* Output modifier redirect.  */
int
imish_redirect (INTERNAL_FUNC func, struct cli *cli, int argc, char **argv,
                char *path)
{
  struct process *proc;

  /* Internal command.  */
  proc = imish_internal_pipe (func, cli, argc, argv);
  if (! proc)
    return -1;

  proc->path = path;

  imish_pipe_line (proc);

  return 0;
}

/* Check environment variable to set pager.  */
void
imish_set_pager ()
{
  char *custom_pager;

  custom_pager = getenv ("IMI_PAGER");
  if (custom_pager)
    {
      pager = custom_pager;
      printf ("pager is set to %s\n", pager);
    }
}


void
imish_exec_show(struct cli *cli)
{
  int ret;
  struct cli_node *node;

  /* Check modifier.  */
  if (ctree->modifier_node)
    {
      /* Run modifier command to check which modifier is
         this.  */
      node = ctree->modifier_node;
      ret = (*node->cel->func) (NULL, 0, NULL);
      node = ctree->exec_node;

      /* Put '\0' at the pipe.  */
      *(ctree->pipe) = '\0';

      /* Run command with modifier.  */
      switch (ret)
        {
          case IMISH_MODIFIER_BEGIN:
            imish_begin ((INTERNAL_FUNC) node->cel->func, cli,
                         ctree->argc, ctree->argv,
                         ctree->argv_modifier[0]);
            break;
          case IMISH_MODIFIER_INCLUDE:
            imish_include ((INTERNAL_FUNC) node->cel->func, cli,
                           ctree->argc, ctree->argv,
                           ctree->argv_modifier[0]);
            break;
          case IMISH_MODIFIER_EXCLUDE:
            imish_exclude ((INTERNAL_FUNC) node->cel->func, cli,
                           ctree->argc, ctree->argv,
                           ctree->argv_modifier[0]);
            break;
          case IMISH_MODIFIER_GREP:
            imish_grep ((INTERNAL_FUNC) node->cel->func, cli,
                        ctree->argc, ctree->argv,
                        ctree->argc_modifier, ctree->argv_modifier);
            break;
          case IMISH_MODIFIER_REDIRECT:
            imish_redirect ((INTERNAL_FUNC) node->cel->func, cli,
                            ctree->argc, ctree->argv,
                            ctree->argv_modifier[0]);
            break;
        }
    }
  else
    {
      node = ctree->exec_node;
      /* Run command with pager.  */
      imish_more ((INTERNAL_FUNC) node->cel->func, cli,
                  ctree->argc, ctree->argv);
    }
}

