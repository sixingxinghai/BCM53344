/* Copyright (C) 2001-2003  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "stream.h"
#include "label_pool.h"

/* Put label space data into a stream. */
void
label_space_data_put (struct label_space_data *ls_data, struct stream **s)
{
  /* Status. */
  stream_putc (*s, ls_data->status);

  /* Label space value. */
  stream_putw (*s, ls_data->label_space);

  /* Min label value. */
  stream_putl (*s, ls_data->min_label_value);

  /* Max label value. */
  stream_putl (*s, ls_data->max_label_value);
}

/* Get label space data from a stream. */
void
label_space_data_get (struct label_space_data *ls_data, struct stream **s)
{
  /* Status. */
  ls_data->status = stream_getc (*s);

  /* Label space value. */
  ls_data->label_space = stream_getw (*s);

  /* Min label value. */
  ls_data->min_label_value = stream_getl (*s);

  /* Max label value. */
  ls_data->max_label_value = stream_getl (*s);
}

/* Compare and update label space data. 'src' will be the latest
   label space data, and 'dst' will be the one retained by the
   protocol specific interface. */
int
label_space_data_update (struct label_space_data *dst,
                         struct label_space_data *src)
{
  int ret = 0;

  if (dst->status != src->status)
    {
      ret = LABEL_SPACE_DATA_MODIFIED;
      dst->status = src->status;
    }

  if (dst->label_space != src->label_space)
    {
      ret = LABEL_SPACE_DATA_MODIFIED;
      dst->label_space = src->label_space;
    }

  if (dst->min_label_value != src->min_label_value)
    {
      ret = LABEL_SPACE_DATA_MODIFIED;
      dst->min_label_value = src->min_label_value;
    }

  if (dst->max_label_value != src->max_label_value)
    {
      ret = LABEL_SPACE_DATA_MODIFIED;
      dst->max_label_value = src->max_label_value;
    }

  return ret;
}

/* This function returns the index into the service range array given the
 * service type enum value
 */
int
label_space_get_service_range_index (enum label_pool_id range_owner_id)
{
  switch (range_owner_id)
    {
      case LABEL_POOL_SERVICE_RSVP:
        return 0;

      case LABEL_POOL_SERVICE_LDP:
        return 1;

      case LABEL_POOL_SERVICE_LDP_LDP_PW:
        return 2;

      case LABEL_POOL_SERVICE_LDP_VPLS:
        return 3;

      case LABEL_POOL_SERVICE_BGP:
        return 4;

      default:
        return LABEL_POOL_SERVICE_ERROR;
    }
}

