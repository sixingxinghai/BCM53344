/* Copyright (C) 2004  All Rights Reserved. */

#include "pal.h"
#include "network.h"

#include "lib.h"
#include "line.h"
#include "vector.h"
#include "message.h"
#include "snprintf.h"

#include "imi/imi.h"
#include "imi/imi_line.h"
#include "imi/imi_server.h"


struct line *
imi_line_new (struct imi_master *im, u_char type, int index)
{
  struct line *new;

  /* Memory allocation.  */
  new = XCALLOC (MTYPE_IMI_LINE, sizeof (struct line));
  if (new == NULL)
    return NULL;

  /* Set index.  */
  new->zg = im->vr->zg;
  new->index = index;
  new->type = type;
  new->privilege = PRIVILEGE_NORMAL;
  new->exec_timeout_min = LINE_TIMEOUT_DEFAULT_MIN;
  new->exec_timeout_sec = LINE_TIMEOUT_DEFAULT_SEC;
  new->maxhist = SINT32_MAX;

  if (type == LINE_TYPE_VTY || type == LINE_TYPE_CONSOLE)
    SET_FLAG (new->config, LINE_CONFIG_LOGIN);

  /* Set the line to the line vector.  */
  vector_set_index (im->lines[type], index, new);

  return new;
}

void
imi_line_free (struct imi_master *im, u_char type, int index)
{
  struct line *line;

  if ((line = imi_line_lookup (im, type, index)))
    {
      XFREE (MTYPE_IMI_LINE, line);
      vector_slot (im->lines[type], index) = NULL;
    }
}

struct line *
imi_line_lookup (struct imi_master *im, u_char type, int index)
{
  if (index < 0 || index >= vector_max (im->lines[type]))
    return NULL;
  else
    return vector_slot (im->lines[type], index);
}

struct imi_server_entry *
imi_line_entry_lookup (struct imi_server *is, int type, int index)
{
  struct imi_server_entry *ise;
  struct imi_server_client *isc;

  if ((isc = vector_slot (is->client, APN_PROTO_IMISH)))
    for (ise = isc->head; ise; ise = ise->next)
      if (ise->line.type == type && ise->line.index == index)
        return ise;

  return NULL;
}
  
struct line *
imi_line_get (struct imi_master *im, u_char type, int index)
{
  struct line *line;

  if ((line = imi_line_lookup (im, type, index)) == NULL)
    return imi_line_new (im, type, index);
  else
    return line;
}

/* Send one line to shell.  */
int
imi_line_send (struct line *line, const char *format, ...)
{
  va_list args;

  /* Set command.  */
  line->code = LINE_CODE_COMMAND;
  line->mode = IMISH_MODE;

  /* Format strings.  */
  va_start (args, format);
  zvsnprintf (line->str, LINE_BODY_LEN, format, args);
  va_end (args);

  line_header_encode (line);

  /* Write the line.  */
  return writen (line->sock, (u_char *)line->buf, line->length);
}

void
imi_line_init (struct imi_master *im, u_char type, int max)
{
  int index;

  /* Store the line.  */
  im->lines[type] = vector_init (max);

  for (index = 0; index < max; index++)
    imi_line_new (im, type, index);
}

void
imi_line_shutdown (struct imi_master *im, u_char type)
{
  int index;
  int max = vector_max (im->lines[type]);

  for (index = 0; index < max; index++)
    imi_line_free (im, type, index);

  vector_free (im->lines[type]);
}
