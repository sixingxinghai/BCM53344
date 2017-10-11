#include <pal.h>

#ifdef HAVE_CONFIG_COMPARE

#include "cli.h"
#include "line.h"
#include "message.h"
#include "imi_client.h"

#include "imish/imish.h"
#include "imish/imish_exec.h"
#include "imish/imish_system.h"
#include "imish/imish_line.h"
#include "imish/imish_readline.h"
#include "imish/imish_parser.h"

int
imish_star ()
{
  int ret;
  struct stat info;

  ret = stat ("/tmp/star", &info);
  if (ret < 0)  
    return 0;
  else
    return 1;
}

void
imish_config_save_force (struct cli *cli)
{
  char *cli_str;

  cli_str = pal_strdup (MTYPE_TMP, cli->str);
  imish_parse ("write mem", EXEC_MODE);
  cli->str = cli_str;
}

#endif /* HAVE_CONFIG_COMPARE */
