#ifndef _L2_VLAN_PORT_RANGE_PARSER_H
#define _L2_VLAN_PORT_RANGE_PARSER_H

typedef int (*validate_input_func) ( char *input, struct cli *cli);

typedef enum l2_range_parse_error
   {
     RANGE_PARSER_SUCCESS,
     RANGE_PARSER_NOT_VALIDATED,
     RANGE_PARSER_INVALID_ARG, 
     RANGE_PARSER_ERROR_MAX
   } l2_range_parse_error_t;

typedef enum l2_range_token
   {
     HYPHEN,
     COMMA,
     NOTOKEN
   }l2_range_token_t;

typedef struct l2_range_parser
{
  l2_range_token_t type;
  char             curr_str  [CLI_ARGV_MAX_LEN];
  char             start_str [CLI_ARGV_MAX_LEN];
  char             orig_str  [CLI_ARGV_MAX_LEN];
  char             *strtok_arg;
  bool_t           first_call;
  bool_t           is_validated;
  long int         start;
  long int         curr;
  long int         end;
} l2_range_parser_t;

/*
   Name: l2_range_parser_init

   Description:
   This API initializes the parser with the string given as 
   input.

   Parameters:
   IN -> input - The string to be parsed.
   OUT -> par  - The parser context to be initialised.

   Returns:
   None.
*/


void
l2_range_parser_init ( char *input, l2_range_parser_t *par);

/*
   Name: l2_range_parser_validate

   Description:
   This API validates the fact that the input string is given in the following
   format.

   The input string should be of the form
   str1 or
   str1,str2,str3
   str1-3

   Parameters:
   IN -> par  -  The parser context to be validated.
   IN -> cli  -  cli used to print error messages.
   IN -> func -  The appilcation function that is used to 
                 perform application specific validaion.

   Returns:
   CLI_ERROR is the format of the input string is incorrect.
*/

int
l2_range_parser_validate ( l2_range_parser_t *par, struct cli *cli, validate_input_func func);

/*
   Name: l2_range_parser_get_next

   Description:
   This API gets the next string from the input. l2_range_parser_validate
   has to be called before calling this function.

   Parameters:
   IN  -> par -  The parser context from which next string is returned.
   OUT -> ret -  The appilcation function that is used to
                 perform application specific validaion.

   Returns:
   The next string if available or NULL if end of input.
*/

char *
l2_range_parser_get_next ( l2_range_parser_t *par, l2_range_parse_error_t *ret);

/*
   Name: l2_range_parser_init

   Description:
   This API resets the members of the parser given as
   input.

   Parameters:
   OUT -> par  - The parser context to be deinitialised.

   Returns:
   None.
*/


void
l2_range_parser_deinit (l2_range_parser_t *par);

const char *
l2_range_parse_err_to_str (l2_range_parse_error_t err);

#endif
