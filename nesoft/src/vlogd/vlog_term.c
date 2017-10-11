/*=============================================================================
**
** Copyright (C) 2008-2009   All Rights Reserved.
**
** vlog_term.c - Implementation of teh VLOG_TERM (a terminal) and
**               VLOG_TERMS (container of terminals) objects.
*/

#include "vlogd.h"

extern VLOG_GLOBALS *vlog_vgb;

static void _vlog_term_del(VLOG_TERM *term);
static s_int32_t _vlog_term_compare(VLOG_TERM *term1, VLOG_TERM *term2);

/*--------------------------------------------------------------
 *                    THE VLOG_TERMS CONTAINER
 *--------------------------------------------------------------
 * vlog_terms_new - Constructor of the VLOG_TERMS object.
 */
VLOG_TERMS *
vlog_terms_new()
{
  VLOG_TERMS *terms;

  terms = XCALLOC(MTYPE_VLOG_TERM, sizeof (VLOG_TERMS));
  if (terms == NULL) {
    return NULL;
  }
  if (list_init(&terms->vts_list,
                (list_cmp_cb_t)_vlog_term_compare,
                (list_del_cb_t)_vlog_term_del) != &terms->vts_list)
  {
    XFREE(MTYPE_VLOG_TERM, terms);
    return NULL;
  }
  return terms;
}

static void
_vlog_term_set_shutdown_flag (VLOG_TERM *term, intptr_t null)
{
  term->vtm_shutdown = PAL_TRUE;
}

static void
_vlog_term_shutdown (VLOG_TERM *term, intptr_t null)
{
  VLOG_TERM_REQ wr_req;

  if (term->vtm_shutdown)
    {
      wr_req.vtr_orig_vrid = 0;
      wr_req.vtr_orig_vrname_len = 0;
      wr_req.vtr_msg = "\r\n\r\n #### VLOG sesion time out "
                       "after IMI shutdown #### \r\n\r\n";
      wr_req.vtr_tot_msg_len =  pal_strlen(wr_req.vtr_msg);

      vlog_term_write (term, &wr_req);
      vlog_term_delete(term);
    }
}

static int
_vlog_terms_shutdown_timeout(struct thread *thread)
{
  /* Walk through all terminals and shutdown those
     still marked.
   */
  vlog_terms_walk (vlog_vgb->vgb_terms, _vlog_term_shutdown, 0);

  vlog_vgb->vgb_timer_thread = NULL;

  return 0;
}

ZRESULT
vlog_terms_init_shutdown(VLOG_TERMS *terms)
{
  if (! terms)
    return ZRES_ERR;

  /* Walk through all terminals and set shutdown flag.
     Start timer.
     When timer expires - go and delete all that have the flag set.
  */
  vlog_terms_walk (terms, _vlog_term_set_shutdown_flag, 0);

  /* Start timer. */

  THREAD_TIMER_ON(vlog_vgb->vgb_lgb,
                  vlog_vgb->vgb_timer_thread,
                  _vlog_terms_shutdown_timeout,
                  NULL,
                  VLOG_TERM_SHUTDOWN_GRACE_PERIOD);

  vlog_vgb->vgb_timer_thread = NULL;

  return ZRES_OK;
}

/*----------------------------------------------------------------
 * vlog_terms_delete - Delete the VLOG_TERMS conatiner.
 */
ZRESULT
vlog_terms_delete(VLOG_TERMS *terms)
{
  XFREE(MTYPE_VLOG_TERM, terms);
  return ZRES_OK;
}

/*----------------------------------------------------------------
 * vlog_terms_lookup - Search for VLOG_TERM by name
 */
VLOG_TERM *
vlog_terms_lookup(VLOG_TERMS *terms, char *term_name)
{
  VLOG_TERM xterm;

  xterm.vtm_name = term_name;

  return list_lookup_data (&terms->vts_list, &xterm);
}

/*----------------------------------------------------------------
 * vlog_terms_link_term - Link an existing VLOG_TERM to the container's list.
 */
ZRESULT
vlog_terms_link_term(VLOG_TERMS *terms, VLOG_TERM *term)
{
  if (listnode_add(&terms->vts_list, term) == NULL)
    return ZRES_ERR;

  if (term->vtm_is_pvr)
    terms->vts_num_pvr_terms++;
  else
    terms->vts_num_vr_terms++;
  return ZRES_OK;
}

/*----------------------------------------------------------------
 * vlog_terms_unlink_term - Unling a VLOG_TERM from the container list.
 */
void
vlog_terms_unlink_term(VLOG_TERMS *terms, VLOG_TERM *term)
{
  if (term->vtm_is_pvr)
    terms->vts_num_pvr_terms--;
  else
    terms->vts_num_vr_terms--;

  listnode_delete(&terms->vts_list, term);
}

/*----------------------------------------------------------------
 * vlog_terms_write - Write to all terminals in the container.
 *                    Exterminate a terminal if write fails.
 */
void
vlog_terms_write (VLOG_TERMS *terms, VLOG_TERM_REQ *wr_req)
{
  struct listnode *this_node, *next_node;
  VLOG_TERM *term;

  /* In case we cannot write to terminal, we will exterminate it. */
  LIST_LOOP_DEL(&terms->vts_list, term, this_node, next_node)
    if (vlog_term_write(term, wr_req) == ZRES_FAIL)
      /*  Remove the VLOG_TERM: unbind from all VRs and from its home
      *  container.
      */
      vlog_term_delete(term);
}

/*----------------------------------------------------------------
 * vlog_terms_walk - Walk all the terminals and execute given callback.
 */
void
vlog_terms_walk (VLOG_TERMS *terms, VLOG_TERMS_WALK_CB func, intptr_t user_ref)
{
  VLOG_TERM *term;
  struct listnode *this_node, *next_node;

  LIST_LOOP_DEL(&terms->vts_list, term, this_node, next_node)
    if (func)
      func (term, user_ref);
}

/*----------------------------------------------------------------
 * vlog_terms_walk_with_abort - Walk all the terminals and execute
 *                              given callback. Abort the loop
 *                              when callback returns value != ZRES_OK.
 */
ZRESULT
vlog_terms_walk_with_abort (VLOG_TERMS *terms,
                            VLOG_TERMS_WALK_WITH_ABORT_CB func,
                            intptr_t user_ref)
{
  VLOG_TERM *term;
  struct listnode *this_node, *next_node;
  ZRESULT zres;

  LIST_LOOP_DEL(&terms->vts_list, term, this_node, next_node)
    if (func)
      if ((zres = func (term, user_ref)) != ZRES_OK)
        return zres;
  return ZRES_OK;
}

/*-----------------------------------------------------------------
 * vlog_term_finish - Walk all terms and delete each of them.
 *
 */
void
vlog_terms_finish (VLOG_TERMS *terms)
{
  VLOG_TERM *term;
  struct listnode *this_node, *next_node;

  LIST_LOOP_DEL(&terms->vts_list, term, this_node, next_node)
      /* Remove the VLOG_TERM: unbind form VRs and from its home container */
    vlog_term_delete(term);
  vlog_terms_delete(terms);
}

/*----------------------------------------------------------------
 * vlog_terms_broadcast - Broadcast a message to all terminals.
 *
 */
void
vlog_terms_broadcast (VLOG_TERMS *terms, char *info)
{
  VLOG_TERM_REQ wr_req;
  char buf[256];

  wr_req.vtr_orig_vrid = 0;
  wr_req.vtr_orig_vrname_len = 0;

  wr_req.vtr_tot_msg_len =  pal_snprintf (buf, sizeof(buf),
                                          "\r\n\r\n"
                                          "#### VLOGD: %s ####"
                                          "\r\n\r\n",
                                          info);
  wr_req.vtr_msg = buf;

  vlog_terms_write(terms, &wr_req);
}


/*--------------------------------------------------------------
 *                    VLOG_TERM OBJECT
 *--------------------------------------------------------------
 * vlog_term_new - VLOG_TERM constructor
 *                 Creates and attaches itself to the main VLOG_TERMS
 *                 container.
 */
VLOG_TERM *
vlog_term_new(char *term_name, u_int32_t vr_id)
{
  VLOG_TERM *term;
#ifndef HAVE_IMISH
  int     sock_num;
#endif /* !HAVE_IMISH */
  ZRESULT res;

  term = XCALLOC(MTYPE_VLOG_TERM, sizeof(VLOG_TERM));
  if (term == NULL) {
    return NULL;
  }
  term->vtm_name = XSTRDUP (MTYPE_VLOG_TERM, term_name);

#ifdef HAVE_IMISH
  term->vtm_type = VLOG_TERM_TYPE_TTY;
  term->vtm_fd = open (term_name, O_WRONLY|O_NOCTTY);
  if (term->vtm_fd < 0)
    {
      XFREE(MTYPE_VLOG_TERM, term->vtm_name);
      XFREE(MTYPE_VLOG_TERM,term);
      return NULL;
    }
#else
  term->vtm_type = VLOG_TERM_TYPE_SOCK;
  sock_num = pal_strtos32 (term_name, NULL, 10);
  if (errno ==  ERANGE || errno == EINVAL)
  {
    XFREE(MTYPE_VLOG_TERM, term->vtm_name);
    XFREE(MTYPE_VLOG_TERM,term);
    return NULL;
  }
  term->vtm_fd    = sock_num;
#endif /* HAVE_IMISH */

  term->vtm_vr_id = vr_id;
  term->vtm_is_pvr= (vr_id == 0) ? PAL_TRUE : PAL_FALSE;
  /* Add terminal to the container. */
  res = vlog_terms_link_term(vlog_vgb->vgb_terms, term);
  if (res != ZRES_OK)
  {
#ifdef HAVE_IMISH
    close (term->vtm_fd);
#endif
    XFREE(MTYPE_VLOG_TERM, term->vtm_name);
    XFREE(MTYPE_VLOG_TERM,term);
    return NULL;
  }
  return term;
}

/*----------------------------------------------------------------
 * _vlog_term_del - Finalize the VLOG_TERM deletion.
 *
 */
static void
_vlog_term_del(VLOG_TERM *term)
{
#ifdef HAVE_IMISH
  close (term->vtm_fd);
#endif
  XFREE(MTYPE_VLOG_TERM, term->vtm_name);
  XFREE(MTYPE_VLOG_TERM, term);
}

/*----------------------------------------------------------------
 * _vlog_term_compare - Compare names of 2 VLOG_TERM objects.
 *
 */
static s_int32_t
_vlog_term_compare(VLOG_TERM *term1, VLOG_TERM *term2)
{
  return pal_strcmp(term1->vtm_name, term2->vtm_name);
}

/*------------------------------------------------------------------
 * vlog_term_del_unused - Deletion of terminal if the reference count
 *                        has reached zero.
 */
void
vlog_term_del_unused(VLOG_TERM *term)
{
  if (term->vtm_vr_cnt <= 0)
    {
      vlog_terms_unlink_term(vlog_vgb->vgb_terms, term);
      _vlog_term_del(term);
    }
}

/*------------------------------------------------------------------
 * vlog_term_delete - Deletion of terminal on the occasion of
 *                 write error, term VR count reaching zero,
 *                 process termination, etc.
 */
void
vlog_term_delete(VLOG_TERM *term)
{
  /* Needs to run through VRs and delete from distribution lists.
     NOTE: Once all the binding for this terminal are removed,
           the terminal is exterminated automatically
           and the walk returns right away.
  */
  vlog_vrs_walk ((VLOG_VRS_WALK_CB)vlog_vr_unbind_term, (intptr_t)term);
}

/*-------------------------------------------------------------------
 * vlog_term_include_vr - Just increment reference count.
 *                        No VLOG_VR container here...
 *
 */
ZRESULT
vlog_term_include_vr(VLOG_TERM *term, VLOG_VR *vvr)
{
  term->vtm_vr_cnt++;
  return ZRES_OK;
}

/*-------------------------------------------------------------------
 * vlog_term_exclude_vr - Dencrement reference count.
 *                        Delete yourself if the reference # reaches 0.
 *                        No VLOG_VR container here...
 *
 */
ZRESULT
vlog_term_exclude_vr(VLOG_TERM *term, VLOG_VR *vvr)
{
  term->vtm_vr_cnt--;

  if (term->vtm_vr_cnt == 0)
  {
    /* Close the communication link to the terminal, free memory. */
    vlog_terms_unlink_term(vlog_vgb->vgb_terms, term);
    _vlog_term_del(term);
    return ZRES_NO_MORE;  /* No more VRs */
  }
  return ZRES_OK;
}

/*----------------------------------------------------------------
 * vlog_term_bind_all_vr - Bind newly added VR to any term that requests
 *                         "all" VRs debug output.
 * Purpose:
 *   To be called from the VLOG_TERMS "walk" function.
 *
 * Params:
 *    term  - Terminal on the global list of terminals.
 *    vvr   - The VR that is being added.
 */
ZRESULT
vlog_term_bind_all_vr(VLOG_TERM *term, VLOG_VR *vvr)
{
  /* We will just call the VR's method to bind this two. */
  if (term->vtm_all_vrs)
  {
    return vlog_vr_bind_term(vvr, term);
  }
  return ZRES_OK;
}

/*-------------------------------------------------------------------
 * vlog_term_write - Write log message to a terminal.
 *
 */
ZRESULT
vlog_term_write (VLOG_TERM *term, VLOG_TERM_REQ *wr_req)
{
  u_int16_t msg_offs = 0;
  u_int16_t msg_len;
  char *msg_ptr;
  ZRESULT res;

  if (! term->vtm_is_pvr)
    msg_offs = wr_req->vtr_orig_vrname_len;

  /* Skip the VR name if both: msg source and user VRs are the same. */
  msg_len = wr_req->vtr_tot_msg_len - msg_offs;
  msg_ptr = wr_req->vtr_msg+msg_offs;

  /* String termination is tdifferent for telnet and IMISH session. */
  switch (term->vtm_type)
  {
  case VLOG_TERM_TYPE_SOCK:
    /* The string is terminated with \r\n */
    res = pal_sock_write (term->vtm_fd, msg_ptr, msg_len);
    break;

  case VLOG_TERM_TYPE_TTY:
    /* Replace the \r with \n and short the msg_len by 1 char.
       Then restore original string.
    */
    res = pal_sock_write (term->vtm_fd, msg_ptr, msg_len);
    break;

  default:
      res = ZRES_ERR;
  }
  if (res == msg_len)
    return ZRES_OK;
  else
    /* Will indicate a need to exterminate this VLOG_TERM object. */
    return ZRES_FAIL;
}

