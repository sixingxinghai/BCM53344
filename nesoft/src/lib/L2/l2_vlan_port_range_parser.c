/* Copyright (C) 2004  All Rights Reserved.  */

#include "pal.h"
#include "lib.h"
#include "if.h"
#include "cli.h"
#include "l2_vlan_port_range_parser.h"

static const char *l2_range_parse_err_str [] =
{
  "Parse Success",      /* RANGE_PARSER_SUCCESS       */
  "Input not validated", /* RANGE_PARSER_NOT_VALIDATED */
  "Invalid Arguement",  /* RANGE_PARSER_INVALID_ARG   */
};

void 
l2_range_parser_init (char *input, l2_range_parser_t *par)
{
  if ( !input || !par )
    return;
  pal_mem_set  (par->orig_str, 0, CLI_ARGV_MAX_LEN);
  pal_strncpy  (par->orig_str, input, strlen(input));
  pal_mem_set  (par->curr_str, 0, CLI_ARGV_MAX_LEN);
  pal_mem_set  (par->start_str, 0, CLI_ARGV_MAX_LEN);
  par->strtok_arg = NULL;
  par->type = NOTOKEN;
  par->first_call = PAL_TRUE;
  par->is_validated = PAL_FALSE;
  par->start = 0;
  par->curr  = 0;
  par->end   = 0;
}

static int 
l2_range_parser_get_token (char *input)
{
  char hyphen = '-';
  char comma = ',';

  if ( pal_strchr (input, hyphen ) != NULL )
      return HYPHEN;
  else if ( pal_strchr (input, comma) != NULL )
      return COMMA;
  else
      return NOTOKEN;
}

int
l2_range_parser_validate ( l2_range_parser_t *input , struct cli *cli, validate_input_func func)
{
  char *curr = NULL;
  char *c = NULL;
  char *tmp = NULL;
  char *end_ptr = NULL;
  l2_range_parse_error_t par_ret;
  int ret = 0;
  int start =0;
  int end = 0;
  char *delim_hyphen = "-";
  char str [CLI_ARGV_MAX_LEN];
  l2_range_parser_t par;

  if (!input || !cli || !func)
    {
      return CLI_ERROR;
    }

  pal_strncpy (str, input->orig_str, CLI_ARGV_MAX_LEN);

  if ( l2_range_parser_get_token (input->orig_str) == HYPHEN)
    {
       c = pal_strtok_r (str, delim_hyphen, &end_ptr);
       tmp = c;
       /* Skip through the initial string to get the starting
        * index if the range. For example if the range is
        * eth1-4, skip eth to get 1.
        */
       while ( *tmp )
         {
           if (pal_char_isdigit(*tmp))
             break;
           tmp++;
         }
       /* Get the starting index of the range */
       if (*tmp)
         {
           start = pal_strtos32( tmp, NULL, 10 );
         }
       else
         {
           cli_out (cli, "Invalid Input \n");
           return CLI_ERROR;
         }

       tmp = pal_strtok_r (NULL, delim_hyphen, &end_ptr);

       if ( tmp &&  (*tmp) )
         {
           end   = pal_strtos32( tmp, &end_ptr, 10 );

           /* Return Error if the trailing string after - is not
            * a valid number.
            */
           if ( (end_ptr) && (*end_ptr))
             {
               cli_out (cli, "Invalid Input \n");
               return CLI_ERROR;
             }
         }
       else
         {
           /* Return Error if there is no number specified after
            * hyphen.
            */
           cli_out (cli, "Invalid Input \n");
           return CLI_ERROR;
         }

         /* Return Error if ending index is greater than staring index. */
         if (start > end)
           {
             cli_out (cli, "Invalid Input \n");
             return CLI_ERROR;
           }
    }

  l2_range_parser_init (input->orig_str, &par);
  par.is_validated = PAL_TRUE;

  while ( (curr = l2_range_parser_get_next (&par, &par_ret) ) )
    {
       if (par_ret != RANGE_PARSER_SUCCESS)
         {
           cli_out (cli, "Invalid Input \n");
           return CLI_ERROR;
         }
       if ( (ret = func (curr, cli) ) != CLI_SUCCESS )
         return ret;
    }

  l2_range_parser_deinit (&par);
  input->is_validated = PAL_TRUE;
  return CLI_SUCCESS;
}

char *
l2_range_parser_get_next ( l2_range_parser_t *par, l2_range_parse_error_t *par_ret)
{

  char *c = NULL;
  char *tmp = NULL;
  char *delim_comma = ",";
  char *delim_hyphen = "-";
  char *delim_number = "0123456789";
  char hyphen = '-';
  char comma = ',';
  char *end_ptr = NULL;
  int  len, ret = 0;

  if (!par_ret)
    return NULL;

  *par_ret = RANGE_PARSER_SUCCESS;

  if (!par)
    {
      *par_ret = RANGE_PARSER_INVALID_ARG;
      return NULL;
    }

  if (par->is_validated != PAL_TRUE)
    {
      *par_ret = RANGE_PARSER_NOT_VALIDATED;
      return NULL;
    }

  if (par->first_call)
    {
      par->first_call = PAL_FALSE;
      if ( pal_strchr (par->orig_str, hyphen ) != NULL )
        {
          par->type = HYPHEN;
          c = pal_strtok_r (par->orig_str, delim_hyphen, &par->strtok_arg);
          len = pal_strlen (c);
          tmp = c;

          /* Skip through the initial string to get the starting 
           * index if the range. For example if the range is
           * eth1-4, skip eth to get 1.
           */
          while ( *tmp )
            {
              if (pal_char_isdigit(*tmp))
                break;
              tmp++;
            }

          /* Get the starting index of the range */
          par->start = pal_strtos32( tmp, NULL, 10 );
          par->curr  = par->start;

          tmp = pal_strtok_r (NULL, delim_hyphen, &par->strtok_arg);

          /*Get the ending index of the range */

          par->end   = pal_strtos32( tmp, NULL, 10 );

          /* If there is a valid prefix string copy it to start_str.
           * for example if the range is eth1-5, copy eth to start_str.
           */

          if ( !(pal_char_isdigit(*c)) )
            {
              pal_strtok_r (c, delim_number, &end_ptr);
              pal_strncpy (par->start_str, c, len);
            }
          else
            {
              pal_mem_set (par->start_str, 0, CLI_ARGV_MAX_LEN);
            }

          return c;

        }
      else if ( pal_strchr (par->orig_str, comma ) != NULL)
        {
          par->type = COMMA;
          return pal_strtok_r (par->orig_str, delim_comma, &par->strtok_arg);
        }
      else
        {
          par->type = NOTOKEN;
          return par->orig_str;
        }
    }
  else
    {
      switch (par->type)
        {
          case NOTOKEN:
            return NULL;
          case COMMA:
            return pal_strtok_r (NULL, delim_comma, &par->strtok_arg);
          case HYPHEN:
            par->curr++;
            if ( par->curr > par->end )
              return NULL;
            ret = pal_snprintf (par->curr_str, CLI_ARGV_MAX_LEN, "%s%ld", par->start_str, par->curr);
            if ( ( ret >= CLI_ARGV_MAX_LEN ) || (ret < 0) )
              return NULL;
            else
              return par->curr_str;
          default:
            return NULL;
        }

    }

  return NULL;

}

void
l2_range_parser_deinit (l2_range_parser_t *par)
{
  if ( !par )
    return;
  pal_mem_set  (par->orig_str, 0, CLI_ARGV_MAX_LEN);
  pal_mem_set  (par->curr_str, 0, CLI_ARGV_MAX_LEN);
  pal_mem_set  (par->start_str, 0, CLI_ARGV_MAX_LEN);
  par->strtok_arg = NULL;
  par->type = NOTOKEN;
  par->first_call = PAL_TRUE;
  par->start = 0;
  par->curr  = 0;
  par->end   = 0;

}

const char * 
l2_range_parse_err_to_str (l2_range_parse_error_t err)
{
  if ( (err >= 0 ) && (err < RANGE_PARSER_ERROR_MAX ) )
    return l2_range_parse_err_str [err];
  else
    return "Unknown Error";
}
