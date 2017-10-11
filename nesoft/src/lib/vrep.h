/* Copyright (C) 2008-2009  All Rights Reserved. */

/*---------------------------------------------------------------------------
 * Implementation of VREP container - a report formatting utility.
 *
 * Use:
 *  vrep_create - to create the report
 *  vrep_add    - to add a row or set of consecutive row fileds to the report
 *  vrep_show   - to display the report.
 *  vrep_delete - to destroy the container
 *  vrep_err_to_str - to map error code to error string
 * See respective function headers for detailed description.
 *---------------------------------------------------------------------------
 */

#ifndef __VREP_H__
#define __VREP_H__

#include "pal.h"
#include "vector.h"

#define VREP_MAX(a,b)    ((a) > (b) ? (a) : (b))
#define VREP_MAX_ROW_WIDTH (1024)
#define VREP_MAX_NUM_COLS (16)
#define VREP_MAX_ERR_STRING_SIZE (500)

/* Error definitions. */
#define VREP_SUCCESS    0
#define VREP_ERROR    -1
#define VREP_ERR_NULL_VREP_TABLE   -2
#define VREP_ERR_NULL_FORMAT_STR   -3
#define VREP_ERR_ROW_MEM_ALLOC    -4
#define VREP_ERR_ROW_BUF_MEM_ALLOC  -5
#define VREP_ERR_COLS_EXCEEDED_MAX  -6


struct vrep_table
{
  struct lib_globals *vrep_tab_zg;
  u_int32_t *vrep_tab_cols_max_width;
  u_int32_t vrep_tab_num_rows;
  u_int32_t vrep_tab_max_cols;
  u_int32_t vrep_tab_max_width;
  s_int16_t vrep_tab_err_code;
  vector vrep_tab_rows;
};


/*--------------------------------------------------------------------
 * vrep_create - Creation of the VREP report container.
 *
 * Purpose:
 *   To create the VREP container and make it ready for content addition.
 *   The VREP will expand dynamically with addition of subsequent rows.
 *
 * Parameters:
 *   max_cols - Maximum number of columns in the report,
 *   max_width- Maximum report width (in characters, including all spaces)
 *
 * Return:
 *  A pointer to newly created VREP container or NULL, in case of error.
 *--------------------------------------------------------------------
 */
struct vrep_table *
vrep_create (u_int32_t max_cols, u_int32_t max_width);

/*--------------------------------------------------------------------
 * vrep_add          - Addition of row (or part of row)
 * vrep_add_next_row - Addition of next row
 *
 * Purpose:
 *  To add to a given row a set of consecutive column fields.
 *  Note:
 *    - Rows always shall be filled from left to right.
 *    - A single row might be filled in multiple calls to vrep_add.
 *    - A single space " " can be used to fill a field with blank spaces.
 *    - Fields that are not filled in will remain empty on the report.
 *
 *  Examples:
 *
 *    - Adding a header of n fields to 0th row :
 *        vrep_add (vrep, 0, 0, " <str1> \t <str2> \t .... \t <strn> ");
 *
 *    - Adding fields to mth row, starting from 0th column to nth column :
 *        vrep_add (vrep, m, 0, " %<fd0> \t %<fd1> \t ...\t %<fdn>", <str0>, <str2>,... , <strn>);
 *          - %<fdn> is the format specifier
 *          - <strn> is the variable holding the value to be displayed
 *
 *    - Adding a field to mth row nth column :
 *        vrep_add (vrep, m, n, "%<fdn>\t", <strn>);
 *
 *
 * Parameters:
 *    vrep - A pointer to report container created with vrep_create
 *    row  - Row number at which to add the fields. Starts with 0.
 *    col  - Column number at which to add the first field. Starts with 0.
 *           Never exceed maximum number of columns declared in the vrep_create.
 *    fmt  - A pointer to the format string.
 *           Use \t to indicate end of a column field.
 *           Use spaces at the beginning and/or at the end of the field format
 *           specification to indicate required field alignment:
 *           - a space at the beginning and at the end indicate center alignment
 *           - a space at the end indicates left alignment
 *           - a space at the beginning indicates right alignment
 *           - no space at all indicates default center alignment
 *    ...    Values to encode using the 'fmt' string.
 * Return:
 *   This function returns one of the following error code in case of error:
 *
 *     VREP_ERR_NULL_VREP_TABLE   - VREP table NULL.
 *     VREP_ERR_NULL_FORMAT_STR   - Null format string.
 *     VREP_ERR_ROW_MEM_ALLOC     - Memory allocation failed for the row.
 *     VREP_ERR_ROW_BUF_MEM_ALLOC - Memory allocation failed for the row buffer.
 *     VREP_ERR_COLS_EXCEEDED_MAX - Number of columns exceeded the MAX num of columns defined.
 *
 *   (OR)
 *     VREP_SUCCESS  - Row added succesfully.
 *--------------------------------------------------------------------
 */
s_int16_t vrep_add (struct vrep_table *vrep,
                    u_int32_t row,
                    u_int32_t col,
                    const char *fmt, ...);

/*--------------------------------------------------------------------
 * vrep_add_next_row - Addition of next row
 *
 * Purpose:
 *  To add a next row to the report
 *
 * Parameters:
 *    vrep     - A pointer to report container created with vrep_create
 *    this_row - A pointer to a variable to store the index of added row.
 *               This might be used to fill individual fields with the use
 *               of vrep_add() function. It may be NULL, in which case this
 *               parameter is ignored.
 *    fmt  - A pointer to the format string. Same as vrep_add().
 *    ...    Values to encode using the 'fmt' string.
 * Return:
 *   Error codes - same as vrep_add
 *   VREP_SUCCESS - Row added succesfully.
 *                  this_row (if not NULL) points to an index of added row.
 *--------------------------------------------------------------------
 */
s_int16_t vrep_add_next_row (struct vrep_table *vrep,
                             u_int32_t *this_row,
                             const char *fmt, ...);

/*--------------------------------------------------------------------
 * vrep_show_cb_t - Type of callback to be implemented by the user.
 *
 * Purpose:
 *  This callback will implement a user specific method of displaying the
 *  report. E.g. Report can be written to CLI, disk file, etc.
 *
 * Parameters:
 *   vrep    - A pointer to report container created with vrep_create.
 *   usr_ref - This is a reference given by the user.
 *   str     - This will be usually a portion of the report (a single
 *             column field value).
 * Return:
 *    This function may return:
 *       VREP_SUCCESS  - Output stream is ready to accept more data.
 *       VREP_ERROR    - An error occured and showing of the report shall be
 *                       abandoned.
 *--------------------------------------------------------------------
 */

typedef s_int16_t (vrep_show_cb_t)(intptr_t usr_ref, char *str);

/*--------------------------------------------------------------------
 * vrep_show - The report show function.
 *
 * Purpose
 *   To show the formatted report using the user provided callback.
 *
 * Parameters:
 *   vrep    - A pointer to report container created with vrep_create.
 *   show_cb - User implemented callback.
 *   usr_ref - User reference returned with the callback.
 *
 * Return:
 *    This function may return:
 *       VREP_SUCCESS  - The display of report was successful.
 *       VREP_ERROR    - An error occured and showing of the report shall be
 *                       abandoned.
 *--------------------------------------------------------------------
 */
s_int16_t vrep_show (struct vrep_table *vrep,
                     vrep_show_cb_t     show_cb,
                     intptr_t           usr_ref);


/*--------------------------------------------------------------------
 * vrep_err_to_str - Error to string mapping function
 *
 * Purpose
 *   To map VREP reported error to error string.
 *
 * Parameters:
 *   err_code - Error code.
 *
 * Return:
 *   This function returns the Mapped error String to the error code received.
 *--------------------------------------------------------------------
 */
char *vrep_err_to_str (s_int16_t err_code);

/*--------------------------------------------------------------------
 * vrep_delete - Delete the VREP container.
 *
 * Purpose
 *   To delete the VREP container and free all the allocated memory.
 *
 * Parameters:
 *   vrep  - A pointer to report container created with vrep_create.
 *
 * Return:
 *   None.
 *--------------------------------------------------------------------
 */
void vrep_delete (struct vrep_table *vrep);

#endif /* __VREP_H__*/


