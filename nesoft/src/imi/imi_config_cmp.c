/* Copyright (C) 2001-2003  All Rights Reserved. */

#include <pal.h>

#ifdef HAVE_CONFIG_COMPARE

#include "cli.h"
#include "show.h"
#include "line.h"
#include "host.h"
#include "fifo.h"
#include "message.h"
#include "routemap.h"
#include "keychain.h"

#include "imi/imi.h"
#include "imi/imi_parser.h"
#include "imi/imi_config.h"
#include "imi/imi_server.h"

#include "sys/mman.h"

#define IMI_STAR_FILE               "/tmp/star"
#define IMI_CURRENT_CONFIG_FILE     "/tmp/current_config"

void
imi_star_on ()
{
  FILE *fp;

  fp = pal_fopen (IMI_STAR_FILE, PAL_OPEN_RW);
  if (fp)
    pal_fclose (fp);
}

void
imi_star_off ()
{
  pal_unlink (IMI_STAR_FILE);
}

int 
imi_cmp_file (int fd1, int fd2)
{    
  u_char *p1, *p2, *map_p1, *map_p2;
  off_t length, len;
  struct stat sb1, sb2;
  int ret = 0;

  ret = fstat (fd1, &sb1);
  if (ret < 0)
    return -1;

  ret = fstat (fd2, &sb2);
  if (ret < 0)
    return -1;

  if (sb1.st_size != sb2.st_size)
    return 1;  

  length = MIN (sb1.st_size, sb2.st_size);

  if ((map_p1 = (u_char *) mmap (NULL, (size_t)length,
                                 PROT_READ, MAP_FILE|MAP_SHARED, 
                                 fd1, 0)) == MAP_FAILED)
    return -1;

  madvise (map_p1, (size_t) length, MADV_SEQUENTIAL); 

  if ((map_p2 = (u_char *) mmap (NULL, (size_t)length,
                                 PROT_READ, MAP_FILE|MAP_SHARED, 
                                 fd2, 0)) == MAP_FAILED)
    {
      munmap (map_p1, (size_t)length);
      return -1;
    }

  madvise (map_p2, (size_t)length, MADV_SEQUENTIAL);

  for (p1 = map_p1, p2 = map_p2, len = length, ret = 0; len--; ++p1, ++p2)
    {
      if (*p1 != *p2)
        {
          ret = 1;
          break;
        }
    }

  munmap (map_p1, (size_t) length);
  munmap (map_p2, (size_t) length);

  return ret;
}   

void
imi_config_compare (struct cli *cli)
{
  struct host *host = cli->vr->host;
  int fd1;
  int fd2;
  int ret;

  imi_config_write (cli, IMI_CURRENT_CONFIG_FILE);

  /* Open file.  */
  fd1 = open (host->config_file, O_RDONLY, 0);
  if (! fd1)
    return;

  fd2 = open (IMI_CURRENT_CONFIG_FILE, O_RDONLY, 0);
  if (! fd2)
    {
      close (fd1);
      return;
    }

  ret = imi_cmp_file (fd1, fd2);

  close (fd1);
  close (fd2);
  
  if (ret)
    imi_star_on ();
  else
    imi_star_off ();
}

#endif /* HAVE_CONFIG_COMPARE */
