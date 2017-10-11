/* Copyright (C) 2008-2009  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "snprintf.h"
#include "memory.h"
#include "vrep.h"


enum vrep_align_type
{
 VREP_ALIGN_NONE = 0,
 VREP_ALIGN_LEFT,
 VREP_ALIGN_CENTER,
 VREP_ALIGN_RIGHT,
 VREP_ALIGN_MAX
};

struct vrep_field
{
  enum vrep_align_type vrep_field_adjust;
  char               * vrep_field_content;
};

struct vrep_row
{
  char *vrep_row_buf_ptr;
  char *vrep_row_tail_ptr;
  s_int16_t vrep_row_err_code;
  struct vrep_field *vrep_row_fields[VREP_MAX_NUM_COLS];
};

static void _vrep_delete_row( struct vrep_row *row);

struct vrep_table *
vrep_create (u_int32_t max_cols, u_int32_t max_width)
{
  struct vrep_table *vrep;

  if (max_cols > VREP_MAX_NUM_COLS)
    return NULL;

  vrep = XCALLOC (MTYPE_LIB_VREP, sizeof (struct vrep_table));
  if (! vrep)
    {
      return NULL;
    }

  vrep->vrep_tab_cols_max_width = XCALLOC (MTYPE_LIB_VREP,
                                           sizeof(u_int32_t)*max_cols);

  if (! vrep->vrep_tab_cols_max_width)
    {
      XFREE(MTYPE_LIB_VREP, vrep);
      return NULL;
    }

  /* Initialize Vector : allocate at least one slot */
  vrep->vrep_tab_rows = vector_init (1) ;
  if (! vrep->vrep_tab_rows)
    {
      XFREE(MTYPE_LIB_VREP, vrep->vrep_tab_cols_max_width);
      XFREE(MTYPE_LIB_VREP, vrep);
    }

  vrep->vrep_tab_num_rows = 0;
  vrep->vrep_tab_max_cols = max_cols;
  vrep->vrep_tab_max_width = max_width + max_cols + 1;
  vrep->vrep_tab_err_code = VREP_SUCCESS;

  return vrep;
}

s_int16_t
vrep_show (struct vrep_table *vrep,
           vrep_show_cb_t show_cb,
           intptr_t usr_ref)
{
  struct vrep_row *cur_vrep_row;
  u_int32_t row;
  u_int32_t col;
  u_int32_t max_col_width;
  u_int32_t cur_col_width;
  char buffer [VREP_MAX_ROW_WIDTH +1];
  char *str = NULL;
  char err_str [VREP_MAX_ERR_STRING_SIZE];
  enum vrep_align_type type;
  u_int32_t space_left = 0;
  int ret = VREP_SUCCESS;

  pal_mem_set (err_str, 0, VREP_MAX_ERR_STRING_SIZE);

  if (! vrep)
    {
      pal_strcpy (err_str,
                  "%% VREP NULL. Memory allocation failed for VREP table\n");
      show_cb (usr_ref, err_str);

      return VREP_ERROR;
    }

  /* Displaying the vrep error strings */
  if (vrep->vrep_tab_err_code != VREP_SUCCESS)
    {
      pal_sprintf (err_str,"%% VREP Error : %s\n",
                           vrep_err_to_str (vrep->vrep_tab_err_code));

      ret = show_cb (usr_ref, err_str);
      if (ret != VREP_SUCCESS)
        return VREP_ERROR;
    }

  /* Displaying all the Error strings occured during the row formatting */
  for (row = 0; row < vrep->vrep_tab_num_rows; row++)
    {
      cur_vrep_row = vector_lookup_index (vrep->vrep_tab_rows, row);
      if (! cur_vrep_row)
        continue;

      if (cur_vrep_row->vrep_row_err_code != VREP_SUCCESS)
        {
          pal_sprintf (err_str,
                   "%% VREP Formattig Error Occured :: Row : %d, ERROR: %s\n",
                    row, vrep_err_to_str (cur_vrep_row->vrep_row_err_code));

          ret = show_cb (usr_ref, err_str);
          if (ret != VREP_SUCCESS)
            return VREP_ERROR;
        }
    }

  /* finding the maximum width for each column. */
  for (col = 0; col < vrep->vrep_tab_max_cols; col++)
    {
      max_col_width = 0;
      for (row = 0; row < vrep->vrep_tab_num_rows; row++)
        {
          cur_vrep_row = vector_lookup_index (vrep->vrep_tab_rows, row);
          if (! cur_vrep_row)
            continue;

          str = cur_vrep_row->vrep_row_fields[col]->vrep_field_content;
          if (str)
            {
              max_col_width = VREP_MAX (max_col_width, pal_strlen(str));
            }
          else
            {
              /* Filling empty field with a blank space */
              pal_strcpy (cur_vrep_row->vrep_row_tail_ptr, " ");
              cur_vrep_row->vrep_row_fields[col]->vrep_field_content =
                      cur_vrep_row->vrep_row_tail_ptr;

              cur_vrep_row->vrep_row_tail_ptr += 2;
              vector_set_index (vrep->vrep_tab_rows, row, cur_vrep_row);
            }
        }

      vrep->vrep_tab_cols_max_width[col] = max_col_width;
    }

  /* Start with empty line. */
  show_cb (usr_ref, "\n");

  /* Start with empty line. */
  show_cb (usr_ref, "\n");

  for (row = 0; row < vrep->vrep_tab_num_rows; row++)
    {
      cur_vrep_row = vector_lookup_index (vrep->vrep_tab_rows, row);
      if (! cur_vrep_row)
        continue;

      for (col = 0; col < vrep->vrep_tab_max_cols; col++)
        {
          str = cur_vrep_row->vrep_row_fields[col]->vrep_field_content;
          cur_col_width = pal_strlen (str);

          type = cur_vrep_row->vrep_row_fields[col]->vrep_field_adjust;
          switch (type)
            {
              case VREP_ALIGN_LEFT:
                {
                  zsnprintf (buffer, VREP_MAX_ROW_WIDTH, "%-*s",
                             vrep->vrep_tab_cols_max_width[col], str);

                  ret = show_cb (usr_ref, buffer);
                  if (ret != VREP_SUCCESS)
                    return VREP_ERROR;

                  break;
                }
              case VREP_ALIGN_RIGHT :
                {
                 zsnprintf (buffer, VREP_MAX_ROW_WIDTH, "%*s",
                                    vrep->vrep_tab_cols_max_width[col], str);

                 ret = show_cb (usr_ref, buffer);
                 if (ret != VREP_SUCCESS)
                   return VREP_ERROR;

                 break;
                }
              case VREP_ALIGN_CENTER:
              case VREP_ALIGN_NONE:
              default:
                {
                  space_left = (vrep->vrep_tab_cols_max_width[col] - cur_col_width)/2;

                  zsnprintf (buffer, VREP_MAX_ROW_WIDTH, "%*s", space_left, "");

                  ret = show_cb (usr_ref, buffer);
                  if (ret != VREP_SUCCESS)
                    return VREP_ERROR;

                  zsnprintf (buffer, VREP_MAX_ROW_WIDTH, "%-*s",
                                     vrep->vrep_tab_cols_max_width[col] - space_left, str);

                  ret = show_cb (usr_ref, buffer);
                  if (ret != VREP_SUCCESS)
                    return VREP_ERROR;

                  break;
                }
            }

          ret = show_cb (usr_ref, "  ");
          if (ret != VREP_SUCCESS)
            return VREP_ERROR;
        }

      ret = show_cb (usr_ref, "\n");
      if (ret != VREP_SUCCESS)
        return VREP_ERROR;
    }

  return VREP_SUCCESS;
}

char *
vrep_err_to_str (s_int16_t err_code)
{
  switch (err_code)
    {
    case VREP_SUCCESS:
      return("VREP SUCCESS");

    case VREP_ERR_NULL_VREP_TABLE:
      return ("VREP table NULL");
    case VREP_ERR_NULL_FORMAT_STR:
      return ("Null format string");
    case VREP_ERR_ROW_MEM_ALLOC:
      return ("Memory allocation failed for the row");
    case VREP_ERR_ROW_BUF_MEM_ALLOC:
      return ("Memory allocation failed for the row buffer");
    case VREP_ERR_COLS_EXCEEDED_MAX:
      return ("Number of columns exceeded the MAX num of columns declared");
    case VREP_ERROR:
    default:
      break;
   }
  return ("VREP ERROR");
}

static struct vrep_row *
vrep_get_vrep_row (struct vrep_table *vrep, u_int32_t row)
{
  struct vrep_row *new_row;
  struct vrep_row *cur_vrep_row = NULL;
  s_int32_t col_ix;

  cur_vrep_row = vector_lookup_index (vrep->vrep_tab_rows, row);

  if (! cur_vrep_row)
    {
      /* Create the row */
      new_row = XCALLOC (MTYPE_LIB_VREP, sizeof (struct vrep_row));
      if (NULL == new_row)
        return NULL;

      /* Allocate memory for the row buffer */
      new_row->vrep_row_buf_ptr = XCALLOC (MTYPE_LIB_VREP,
                                           vrep->vrep_tab_max_width);

      if (! new_row->vrep_row_buf_ptr)
        {
          XFREE(MTYPE_LIB_VREP, new_row);
          return NULL;
        }
      /* Initialize the tail pointer of this row */
      new_row->vrep_row_tail_ptr = new_row->vrep_row_buf_ptr;

      /* Initialize the row error code */
      new_row->vrep_row_err_code = VREP_SUCCESS;

      /* Allocate memory for the row fields */
      for (col_ix = 0; col_ix < vrep->vrep_tab_max_cols; col_ix++)
        {
          new_row->vrep_row_fields[col_ix] = XCALLOC (MTYPE_LIB_VREP,
                                                   sizeof (struct vrep_field));

          if (! new_row->vrep_row_fields[col_ix])
            {
              _vrep_delete_row(new_row);
              return NULL;
            }
        }
      /* The new row in the tab of rows */
      vector_set_index (vrep->vrep_tab_rows, row, new_row);

      vrep->vrep_tab_num_rows = VREP_MAX (vrep->vrep_tab_num_rows, row+1);

      /* Set the new_row to cur_vrep_row to return */
      cur_vrep_row = new_row;
    }

  return cur_vrep_row;
}

enum vrep_align_type
vrep_set_alignment (struct vrep_row *cur_row, u_int32_t cur_col, char *str)
{
  enum vrep_align_type type;
  u_int32_t len;

  type = VREP_ALIGN_NONE;

  if (str)
    {
      len = pal_strlen (str);
      if ((*str == ' ') && ( *(str + len -1) == ' '))
        {
          type = VREP_ALIGN_CENTER;
        }
      else if (*(str + len -1) == ' ')
       {
         type = VREP_ALIGN_LEFT;
        }
      else if ( *str == ' ')
        {
          type = VREP_ALIGN_RIGHT;
        }
    }

  cur_row->vrep_row_fields[cur_col]->vrep_field_adjust = type;

  return  type;
}


static s_int16_t
_vrep_add_common (struct vrep_table *vrep,
                  struct vrep_row   *cur_row,
                  u_int32_t column,
                  const char *format,
                  va_list args)
{
  char buffer [VREP_MAX_ROW_WIDTH +1];
  char *str = NULL;
  char *ptr = NULL;
  char *tail_ptr = NULL;
  u_int32_t cur_col = column;
  enum vrep_align_type type;
  u_int32_t len = 0;

  pal_mem_set (buffer, 0, sizeof (buffer));
  zvsnprintf (buffer, VREP_MAX_ROW_WIDTH, format, args);

  tail_ptr = cur_row->vrep_row_tail_ptr;

  str = pal_strtok_r (buffer, "\t", &ptr);
  if (! str)
    return VREP_SUCCESS;

  do
  {
    if (cur_col >= vrep->vrep_tab_max_cols)
      {
        cur_row->vrep_row_err_code = VREP_ERR_COLS_EXCEEDED_MAX;
        break;
      }
    type = vrep_set_alignment (cur_row, cur_col,  str);

    /* Removing all the spaces used for setting alignment type
       at the beginning of the string, if str != " " */
    if (type == VREP_ALIGN_CENTER || type == VREP_ALIGN_RIGHT)
      do
      {
        if (pal_strlen (str) > 1)
          str++;
      } while (pal_strlen (str) > 1 && *str == ' ');

    /* Removing all the spaces used for setting alignment type
       at the end of the string, if str != " " */
    len = pal_strlen (str);
    if (type == VREP_ALIGN_LEFT || type == VREP_ALIGN_CENTER)
      do
      {
        if (len > 1)
          len--;
      } while (len > 1 && (str [len-1] == ' '));

    str [len] = '\0';

    len = pal_strlen(str);
    pal_strncpy (tail_ptr, str, len);
    cur_row->vrep_row_fields[cur_col]->vrep_field_content = tail_ptr;

    tail_ptr += len;
    *tail_ptr = '\0';
    tail_ptr++;
    cur_col++;

    str = pal_strtok_r (NULL, "\t", &ptr);
  } while (str != NULL);

  cur_row->vrep_row_tail_ptr = tail_ptr;

  return cur_row->vrep_row_err_code;
}

s_int16_t
vrep_add (struct vrep_table *vrep,
          u_int32_t row,
          u_int32_t column,
          const char *format, ...)
{
  struct vrep_row *new_row = NULL;
  char *fmptr = (char *)format;
  s_int16_t rc;
  va_list args;

  if (! vrep)
    {
      return VREP_ERR_NULL_VREP_TABLE;
    }

  if (NULL == fmptr)
    {
      return VREP_ERR_NULL_FORMAT_STR;
    }

  new_row = vrep_get_vrep_row (vrep, row);
  if (NULL == new_row)
    {
      vrep->vrep_tab_err_code = VREP_ERR_ROW_MEM_ALLOC;
      return VREP_ERR_ROW_MEM_ALLOC;
    }

  va_start (args, format);
  rc = _vrep_add_common (vrep, new_row, column, format, args);
  va_end (args);

  return rc;
}

s_int16_t
vrep_add_next_row (struct vrep_table *vrep,
                   u_int32_t *this_row,
                   const char *format, ...)
{
  u_int32_t new_row_ix;
  struct vrep_row *new_row = NULL;
  char *fmptr = (char *)format;
  s_int16_t rc;
  va_list args;

  if (NULL == vrep)
    {
      return VREP_ERR_NULL_VREP_TABLE;
    }

  if (NULL == fmptr)
    {
      return VREP_ERR_NULL_FORMAT_STR;
    }

  new_row_ix = vrep->vrep_tab_num_rows;
  new_row = vrep_get_vrep_row (vrep, new_row_ix);
  if (NULL == new_row)
    {
      vrep->vrep_tab_err_code = VREP_ERR_ROW_MEM_ALLOC;
      return VREP_ERR_ROW_MEM_ALLOC;
    }
  vrep->vrep_tab_num_rows++;

  va_start (args, format);
  rc = _vrep_add_common (vrep, new_row, 0, format, args);
  va_end (args);

  if (rc != VREP_SUCCESS)
    {
      if (this_row)
        {
          *this_row = new_row_ix;
        }
    }
  return rc;
}

static void
_vrep_delete_row( struct vrep_row *row)
{
  u_int32_t col_ix;

  for (col_ix = 0; col_ix < VREP_MAX_NUM_COLS; col_ix++)
    {
      if (row->vrep_row_fields[col_ix])
        XFREE (MTYPE_LIB_VREP, row->vrep_row_fields[col_ix]);
    }
  if (row->vrep_row_buf_ptr)
    XFREE (MTYPE_LIB_VREP, row->vrep_row_buf_ptr);

  XFREE (MTYPE_LIB_VREP, row);
}


void
vrep_delete (struct vrep_table * vrep)
{
  struct vrep_row *cur_vrep_row = NULL;
  u_int32_t row_ix;

  if (vrep)
    {
      for (row_ix = 0; row_ix < vrep->vrep_tab_num_rows; row_ix++)
        {
          cur_vrep_row = vector_lookup_index (vrep->vrep_tab_rows, row_ix);
          if (cur_vrep_row)
              _vrep_delete_row(cur_vrep_row);
        }
      if (vrep->vrep_tab_cols_max_width)
        XFREE (MTYPE_LIB_VREP, vrep->vrep_tab_cols_max_width);

      vector_free (vrep->vrep_tab_rows);
      XFREE (MTYPE_LIB_VREP, vrep);
    }
}



