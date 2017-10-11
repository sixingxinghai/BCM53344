/* Copyright (C) 2004   All Rights Reserved. */

#include "pal.h"

result_t
pal_term_winsize_set (pal_sock_handle_t sock,
                      u_int16_t row, u_int16_t col)
{
  struct winsize w;
  int ret;

  w.ws_row = row;
  w.ws_col = col;

  ret = ioctl (sock, TIOCSWINSZ, &w);
  if (ret < 0)
    return -1;

  return RESULT_OK;
}

result_t
pal_term_winsize_get (pal_sock_handle_t sock,
                      u_int16_t *row, u_int16_t *col)
{
  struct winsize w;
  int ret;

  ret = ioctl (sock, TIOCGWINSZ, &w);
  if (ret < 0)
    return -1;

  *row = w.ws_row;
  *col = w.ws_col;

  return RESULT_OK;
}
