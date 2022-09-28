#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)txt_edit.c 20.62 95/08/29";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Programming interface to editing facilities of text subwindows.
 */

#include <xview_private/primal.h>
#include <xview_private/txt_impl.h>
#include <xview_private/ev_impl.h>
#include <xview/pkg.h>
#include <xview/attrol.h>
#include <xview/notice.h>
#include <xview/frame.h>
#include <xview/server.h>


#define INSERT_BUFSIZE 8193
#define REPLACE_BUFSIZE	8193

#define THRESHOLD  20
#define UPDATE_SCROLLBAR(_delta, _old_length)\
	((THRESHOLD * _delta) >= _old_length)

Xv_private_data char *xv_shell_prompt;

extern void textsw_SetSelection();

Pkg_private void     textsw_notify_replaced();
Pkg_private Seln_rank textsw_acquire_seln();
Pkg_private Textsw_index textsw_replace();

Pkg_private     Es_handle
textsw_esh_for_span(view, first, last_plus_one, to_recycle)
    Textsw_view_handle view;
    Es_index        first, last_plus_one;
    Es_handle       to_recycle;
{
    Es_handle       esh = FOLIO_FOR_VIEW(view)->views->esh;

    return ((Es_handle)
	    es_get5(esh, ES_HANDLE_FOR_SPAN, first, last_plus_one,
		    to_recycle, 0, 0));
}

Pkg_private int
textsw_adjust_delete_span(folio, first, last_plus_one)
    register Textsw_folio folio;
    register Es_index *first, *last_plus_one;
/*
 * Returns: TXTSW_PE_EMPTY_INTERVAL iff *first < *last_plus_one, else
 * TEXTSW_PE_READ_ONLY iff NOTHING should be deleted, else TXTSW_PE_ADJUSTED
 * iff *first adjusted to reflect the constraint imposed by
 * folio->read_only_boundary, else 0.
 */
{
    if (*first >= *last_plus_one)
	return (TXTSW_PE_EMPTY_INTERVAL);
    if TXTSW_IS_READ_ONLY
	(folio)
	    return (TEXTSW_PE_READ_ONLY);
    if (!EV_MARK_IS_NULL(&folio->read_only_boundary)) {
	register Es_index mark_at;
	mark_at = textsw_find_mark_internal(folio,
					    folio->read_only_boundary);
	if AN_ERROR
	    (mark_at == ES_INFINITY)
		return (0);
	if (*last_plus_one <= mark_at)
	    return (TEXTSW_PE_READ_ONLY);
	if (*first < mark_at) {
	    *first = mark_at;
	    return (TXTSW_PE_ADJUSTED);
	}
    }
    return (0);
}

Pkg_private int
textsw_esh_failed_msg(view, preamble)
    Textsw_view_handle view;
    char           *preamble;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    Es_status       status;
    Xv_Notice	text_notice;
    Frame	frame;

    status = (Es_status)
	es_get(folio->views->esh, ES_STATUS);
    switch (status) {
      case ES_SHORT_WRITE:
	if (TEXTSW_OUT_OF_MEMORY(folio, status)) {
	    frame = FRAME_FROM_FOLIO_OR_VIEW(view);
	    text_notice = xv_get(frame, XV_KEY_DATA, text_notice_key, NULL);

	    if (!text_notice)  {
    	        text_notice = xv_create(frame, NOTICE, NULL);
	        xv_set(frame, 
		    XV_KEY_DATA, text_notice_key, text_notice, 
		    NULL);

            }
    	    xv_set(text_notice, 
			NOTICE_LOCK_SCREEN, FALSE,
			NOTICE_BLOCK_THREAD, TRUE,
		        NOTICE_BUTTON_YES, 
				XV_MSG("Continue"),
		        NOTICE_MESSAGE_STRINGS,
			  (strlen(preamble)) ? preamble : 
				XV_MSG("Action failed -"),
				XV_MSG("The memory buffer is full.\n\
If this is an isolated case, you can circumvent\n\
this condition by undoing the operation you just\n\
performed, storing the contents of the subwindow\n\
to a file using the text menu, and then redoing\n\
the operation.  Or, you can enlarge the size of\n\
this buffer by changing the appropriate value in\n\
the .Xdefaults file (Text.MaxDocumentSize)."),
		        0,
			XV_SHOW, TRUE, 
              0);
	    break;
	}
	/* else fall through */
      case ES_CHECK_ERRNO:
      case ES_CHECK_FERROR:
      case ES_FLUSH_FAILED:
      case ES_FSYNC_FAILED:
      case ES_SEEK_FAILED:{
	    frame = FRAME_FROM_FOLIO_OR_VIEW(view);
	    text_notice = xv_get(frame, XV_KEY_DATA, text_notice_key, NULL);

	    if (!text_notice)  {
    	        text_notice = xv_create(frame, NOTICE, NULL);
	        xv_set(frame, 
		    XV_KEY_DATA, text_notice_key, text_notice, 
		    NULL);

            }
    	    xv_set(text_notice, 
			NOTICE_LOCK_SCREEN, FALSE,
			NOTICE_BLOCK_THREAD, TRUE,
		        NOTICE_BUTTON_YES, XV_MSG("Continue"),
		        NOTICE_MESSAGE_STRINGS,
			  (strlen(preamble)) ? preamble : 
				XV_MSG("Action failed -"),
			XV_MSG("A problem with the file system has been detected.\n\
File system is probably full."),
		        0,
			XV_SHOW, TRUE, 
	      0);
	    break;
	}
      case ES_REPLACE_DIVERTED:
	break;
      default:
	break;
    }
}

Pkg_private     Es_index
textsw_delete_span(view, first, last_plus_one, flags)
    Textsw_view_handle view;
    Es_index        first, last_plus_one;
    register unsigned flags;
/*
 * Returns the change in indices resulting from the operation.  Result is: a)
 * usually < 0, b) 0 if span is empty or in a read_only area, c)
 * ES_CANNOT_SET if ev_delete_span fails
 */
{
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    Es_index        result;

    result = (flags & TXTSW_DS_ADJUST)
	? textsw_adjust_delete_span(folio, &first, &last_plus_one)
	: (first >= last_plus_one) ? TXTSW_PE_EMPTY_INTERVAL : 0;
    switch (result) {
      case TEXTSW_PE_READ_ONLY:
      case TXTSW_PE_EMPTY_INTERVAL:
	result = 0;
	break;
      case TXTSW_PE_ADJUSTED:
	if (flags & TXTSW_DS_CLEAR_IF_ADJUST(0)) {
	    textsw_SetSelection(VIEW_REP_TO_ABS(view),
				 ES_INFINITY, ES_INFINITY,
				 EV_SEL_BASE_TYPE(flags));
	}
	/* Fall through to do delete on remaining span. */
      default:
	if (flags & TXTSW_DS_SHELVE) {
	    folio->trash = textsw_esh_for_span(view, first, last_plus_one,
					       folio->trash);
	    textsw_acquire_seln(folio, SELN_SHELF);
	}
	switch (ev_delete_span(folio, first, last_plus_one, &result,
			       ((flags & TXTSW_DS_RETURN_BYTES) ? 1: 0))) {
	  case 0: {
	    Es_index line_first, line_last_plus_one;
    	    (void) ev_span(folio->views, first, &line_first,
                    &line_last_plus_one, EI_SPAN_LINE);
    	    if (line_first != ES_CANNOT_SET)
        	folio->column_offset = first - line_first;
	    if (flags & TXTSW_DS_RECORD) {
		textsw_record_delete(folio);
	    }
	    break;
          }
	  case 3:
	    textsw_esh_failed_msg(view, 
		XV_MSG("Deletion failed - "));
	    /* Fall through */
	  default:
	    result = ES_CANNOT_SET;
	    break;
	}
	break;
    }
    return (result);
}

Pkg_private     Es_index
textsw_do_pending_delete(view, type, flags)
    Textsw_view_handle view;
    unsigned        type;
    int             flags;
{
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    int             is_pending_delete;
    Es_index        first, last_plus_one, delta, insert;
    int             result = ev_get_selection(folio->views, &first, &last_plus_one, type);

    is_pending_delete = ((type == EV_SEL_PRIMARY) ?
			 (EV_SEL_PD_PRIMARY & result) :
			 (EV_SEL_PD_SECONDARY & result));

    if (first >= last_plus_one)
	return (0);
    textsw_take_down_caret(folio);
    insert = (flags & TFC_INSERT) ? EV_GET_INSERT(folio->views) : first;
    if (is_pending_delete &&
	(first <= insert) && (insert <= last_plus_one)) {
	if (folio->state & TXTSW_DELETE_REPLACES_CLIPBOARD) {
	    delta = textsw_delete_span(view, first, last_plus_one,
				       TXTSW_DS_ADJUST | TXTSW_DS_SHELVE |
				       TXTSW_DS_CLEAR_IF_ADJUST(type));
	} else {
	    delta = textsw_delete_span(view, first, last_plus_one,
				       TXTSW_DS_ADJUST | 
				       TXTSW_DS_CLEAR_IF_ADJUST(type));
	}
    } else {
	if (flags & TFC_SEL) {
	    textsw_SetSelection(VIEW_REP_TO_ABS(view),
				 ES_INFINITY, ES_INFINITY, type);
	}
	delta = 0;
    }
    return (delta);
}


static          Textsw_index
textswDelete(abstract, first, last_plus_one)
    Textsw          abstract;
    Es_index        first, last_plus_one;
{
    Textsw_view_handle view = VIEW_ABS_TO_REP(abstract);
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    int             result;

    textsw_implicit_commit(folio);

    textsw_take_down_caret(folio);
    result = textsw_delete_span(view, first, last_plus_one,
				TXTSW_DS_ADJUST | TXTSW_DS_SHELVE);
    if (result == ES_CANNOT_SET)
	return 0;
    return -result;
}

Xv_public          Textsw_index
textsw_delete_wcs(abstract, first, last_plus_one)
    Textsw          abstract;
    Es_index        first, last_plus_one;
{
    if (multibyte)
        return(textswDelete(abstract, first, last_plus_one));
    else {    
        Textsw_view_handle view = VIEW_ABS_TO_REP(abstract);
        Textsw_folio    folio = FOLIO_FOR_VIEW(view);
        int             result;

        textsw_implicit_commit(folio);
        /*
         * This check is for the performance, because textsw_wcpos_from_mbpos()
         * is expensive process.
         */
        if (first < 0 || last_plus_one < 0 || first >= last_plus_one)
	    return 0;

        textsw_take_down_caret(folio);
        first = textsw_mbpos_from_wcpos(folio, first);
        last_plus_one = textsw_mbpos_from_wcpos(folio, last_plus_one);
        result = textsw_delete_span(view, first, last_plus_one,
		TXTSW_DS_ADJUST | TXTSW_DS_SHELVE | TXTSW_DS_RETURN_BYTES);
        if (result == ES_CANNOT_SET)
	    return 0;
        return -result;
    }
}

Xv_public          Textsw_index
textsw_delete(abstract, first, last_plus_one)
    Textsw          abstract;
    Es_index        first, last_plus_one;
{
    Textsw_view_handle view = VIEW_ABS_TO_REP(abstract);
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);

    if (!multibyte)
        return(textswDelete(abstract, first, last_plus_one));
    else {
        Textsw_view_handle view = VIEW_ABS_TO_REP(abstract);
        Textsw_folio    folio = FOLIO_FOR_VIEW(view);
        int             result;

        textsw_implicit_commit(folio);
        /*
         * This check is for the performance, because textsw_mbpos_from_wcpos()
         * is expensive process.
         */
        if (first < 0 || last_plus_one < 0 || first >= last_plus_one)
	    return 0;

        textsw_take_down_caret(folio);
        first = textsw_wcpos_from_mbpos(folio, first);
        last_plus_one = textsw_wcpos_from_mbpos(folio, last_plus_one);
        result = textsw_delete_span(view, first, last_plus_one,
		TXTSW_DS_ADJUST | TXTSW_DS_SHELVE | TXTSW_DS_RETURN_BYTES);
        if (result == ES_CANNOT_SET)
	    return 0;
        return -result;
    }
}


Pkg_private          Textsw_index
textswErase(abstract, first, last_plus_one)
    Textsw          abstract;
    Es_index        first, last_plus_one;
/*
 * This routine is identical to textsw_delete EXCEPT it does not affect the
 * contents of the shelf (useful for client implementing ^W/^U or mailtool).
 */
{
    Textsw_view_handle view = VIEW_ABS_TO_REP(abstract);
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    int             result;

#ifdef OW_I18N
    textsw_implicit_commit(folio);
#endif
    textsw_take_down_caret(folio);
    result = textsw_delete_span(view, first, last_plus_one,
				TXTSW_DS_ADJUST);
    if (result == ES_CANNOT_SET)
	return 0;
    return -result;
}

Xv_public          Textsw_index
textsw_erase_wcs(abstract, first, last_plus_one)
    Textsw          abstract;
    Es_index        first, last_plus_one;
{
    if (multibyte)
        return(textswErase(abstract, first, last_plus_one));
    else {    
        Textsw_view_handle view = VIEW_ABS_TO_REP(abstract);
        Textsw_folio    folio = FOLIO_FOR_VIEW(view);
        int             result;

        textsw_implicit_commit(folio);
        /*
         * This check is for the performance, because textsw_wcpos_from_mbpos()
         * is expensive process.
         */
        if (first < 0 || last_plus_one < 0 || first >= last_plus_one)
	    return 0;

        textsw_take_down_caret(folio);
        first = textsw_mbpos_from_wcpos(folio, first);
        last_plus_one = textsw_mbpos_from_wcpos(folio, last_plus_one);
        result = textsw_delete_span(view, first, last_plus_one,
				TXTSW_DS_ADJUST | TXTSW_DS_RETURN_BYTES);
        if (result == ES_CANNOT_SET)
	    return 0;
        return -result;
    }
}


Xv_public          Textsw_index
textsw_erase(abstract, first, last_plus_one)
    Textsw          abstract;
    Es_index        first, last_plus_one;
{
    if (!multibyte)
        return(textswErase(abstract, first, last_plus_one));
    else {
        Textsw_view_handle view = VIEW_ABS_TO_REP(abstract);
        Textsw_folio    folio = FOLIO_FOR_VIEW(view);
        int             result;

        textsw_implicit_commit(folio);
        /*
         * This check is for the performance, because textsw_mbpos_from_wcpos()
         * is expensive process.
         */
        if (first < 0 || last_plus_one < 0 || first >= last_plus_one)
	    return 0;

        textsw_take_down_caret(folio);
        first = textsw_wcpos_from_mbpos(folio, first);
        last_plus_one = textsw_wcpos_from_mbpos(folio, last_plus_one);
        result = textsw_delete_span(view, first, last_plus_one,
				TXTSW_DS_ADJUST | TXTSW_DS_RETURN_BYTES);
        if (result == ES_CANNOT_SET)
	    return 0;
        return -result;
    }
}


Pkg_private void
textsw_insert_makes_visible(textsw)
    Textsw          textsw;
{
    Textsw_view_handle view = VIEW_ABS_TO_REP(textsw);
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    Textsw_enum     old_insert_makes_visible =
    folio->insert_makes_visible;
    int             old_state = folio->state;

    folio->insert_makes_visible = TEXTSW_ALWAYS;
    folio->state |= TXTSW_DOING_EVENT;

    TEXTSW_DO_INSERT_MAKES_VISIBLE(view);

    folio->insert_makes_visible = old_insert_makes_visible;
    folio->state = old_state;

}

/*
 * If byte_return is 1, return number of deleted bytes. And if the value
 * is 0, return number of deleted characters.
 */
Pkg_private int
textsw_do_edit(view, unit, direction, byte_return)
    register Textsw_view_handle view;
    unsigned        unit, direction;
    unsigned        byte_return;
{
    extern struct ei_span_result
                    ev_span_for_edit();
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    struct ei_span_result span;
    int             delta;

    span = ev_span_for_edit(folio->views, (int) (unit | direction));
    if ((span.flags >> 16) == 0) {

	/* Don't join with next line for ERASE_LINE_END */

	if ((unit == EV_EDIT_LINE) && (direction == 0)) {
	    Es_index        file_length = es_get_length(folio->views->esh);

	    if (span.last_plus_one < file_length)
		span.last_plus_one--;
	}
	delta = textsw_delete_span(view, span.first, span.last_plus_one,
		 ((!byte_return) ? TXTSW_DS_ADJUST :
				   TXTSW_DS_ADJUST | TXTSW_DS_RETURN_BYTES));
	if (delta == ES_CANNOT_SET) {
	    delta = 0;
	} else {
	    TEXTSW_DO_INSERT_MAKES_VISIBLE(view);
	    textsw_record_edit(folio, unit, direction);
	    delta = -delta;
	}
    } else
	delta = 0;
    return (delta);
}

static          Textsw_index
textswEdit(abstract, unit, count, direction, byteArg)
    Textsw          abstract;
    register unsigned unit, count;
    unsigned        direction;
    unsigned        byteArg;
{
    Textsw_view_handle view = VIEW_ABS_TO_REP(abstract);
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    int             result = 0;

#ifdef OW_I18N
    textsw_implicit_commit(folio);
#endif

    if (direction)
	direction = EV_EDIT_BACK;
    switch (unit) {
      case TEXTSW_UNIT_IS_CHAR:
	unit = EV_EDIT_CHAR;
	break;
      case TEXTSW_UNIT_IS_WORD:
	unit = EV_EDIT_WORD;
	break;
      case TEXTSW_UNIT_IS_LINE:
	unit = EV_EDIT_LINE;
	break;
      default:
	return 0;
    }
    textsw_take_down_caret(folio);
    for (; count; count--) {
	result += textsw_do_edit(view, unit, direction, byteArg);
    }
    return (result);
}

Xv_public          Textsw_index
textsw_edit_wcs(abstract, unit, count, direction)
    Textsw          abstract;
    register unsigned unit, count;
    unsigned        direction;
{
    if (multibyte)
        return(textswEdit(abstract, unit, count, direction, 0));
    else
        return(textswEdit(abstract, unit, count, direction, 1));
}

Xv_public          Textsw_index
textsw_edit(abstract, unit, count, direction)
    Textsw          abstract;
    register unsigned unit, count;
    unsigned        direction;
{
    if (!multibyte)
        return(textswEdit(abstract, unit, count, direction, 0));
    else
        return(textswEdit(abstract, unit, count, direction, 1));
}

Pkg_private void
textsw_input_before(view, old_insert_pos, old_length)
    Textsw_view_handle view;
    Es_index       *old_insert_pos, *old_length;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    register Ev_chain chain = folio->views;
    Ev_chain_pd_handle private = EV_CHAIN_PRIVATE(chain);
    *old_length = es_get_length(chain->esh);
    *old_insert_pos = EV_GET_INSERT(chain);
    if (private->lower_context != EV_NO_CONTEXT) {
	ev_check_insert_visibility(chain);
    }
}

Pkg_private int
textsw_input_partial(view, buf, buf_len)
    Textsw_view_handle view;
    void           *buf;
    long int        buf_len;
{
    int             status;

    status = ev_input_partial(FOLIO_FOR_VIEW(view)->views, buf, buf_len);
    if (status) {
	textsw_esh_failed_msg(view, 
		XV_MSG("Insertion failed - "));
	return SELN_FAILED;
    }
    return (status ? SELN_FAILED : SELN_SUCCESS);
}

Pkg_private     Es_index
textsw_input_after(view, old_insert_pos, old_length, record)
    Textsw_view_handle view;
    Es_index        old_insert_pos, old_length;
    int             record;
{
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    Es_index        delta;

    delta = ev_input_after(folio->views, old_insert_pos, old_length);
    if (delta != ES_CANNOT_SET) {
	TEXTSW_DO_INSERT_MAKES_VISIBLE(view);
	if (record) {
	    Es_handle       pieces;
	    pieces = textsw_esh_for_span(folio->first_view,
			   old_insert_pos, old_insert_pos + delta, ES_NULL);
	    textsw_record_piece_insert(folio, pieces);
	}
	if ((folio->state & TXTSW_EDITED) == 0)
	    textsw_possibly_edited_now_notify(folio);
	if (folio->notify_level & TEXTSW_NOTIFY_EDIT) {
	    textsw_notify_replaced((Textsw_opaque) folio->first_view,
				 old_insert_pos, old_length, old_insert_pos,
				   old_insert_pos, delta);
	}
	(void) textsw_checkpoint(folio);
    }
    return (delta);
}

static wchar_t * 
memchr_wcs(buf, c, buf_len)
    wchar_t		*buf;
    char		c;
    long int            buf_len;		
{
    wchar_t		wc;
    int			i;
    
    mbtowc(&wc, &c, 1);
    for (i = 0; i < buf_len; i++) {
        if (*buf == wc)
            return(buf);
        else
            buf++;    
    }
    return(NULL);	
}

Pkg_private     Es_index
textsw_do_input(view, buf, buf_len, flag)
    Textsw_view_handle view;
    void           *buf;
    long int        buf_len;
    unsigned        flag;
{
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    register Ev_chain chain = folio->views;
    int             record;
    Es_index        delta, old_insert_pos, old_length;
    Es_index        new_first, new_last_plus_one;

    /* possibly use escape sequences ? */
    if (xv_get((Xv_opaque)XV_SERVER_FROM_WINDOW(VIEW_REP_TO_ABS(view)), SERVER_JOURNALLING))

        if (multibyte) {
	    if (memchr_wcs(buf, xv_shell_prompt[0], buf_len))
	        xv_set((Xv_opaque)XV_SERVER_FROM_WINDOW(VIEW_REP_TO_ABS(view)), 
                                 SERVER_JOURNAL_SYNC_EVENT, 1, 0);
        } else        
 	    if (memchr(buf, xv_shell_prompt[0], buf_len))
	        xv_set((Xv_opaque)XV_SERVER_FROM_WINDOW(VIEW_REP_TO_ABS(view)), 
                                  SERVER_JOURNAL_SYNC_EVENT, 1, 0);

    textsw_input_before(view, &old_insert_pos, &old_length);
    if (textsw_input_partial(view, buf, buf_len) == ES_CANNOT_SET)
	return (0);
    record = (TXTSW_DO_AGAIN(folio) &&
	      ((folio->func_state & TXTSW_FUNC_AGAIN) == 0));
    delta = textsw_input_after(view, old_insert_pos, old_length,
			       record && (buf_len > 100));
    if (delta == ES_CANNOT_SET)
	return (0);

    if ((int) ev_get(view->e_view, EV_CHAIN_DELAY_UPDATE) == 0) {
	ev_update_chain_display(chain);

	if (flag & TXTSW_UPDATE_SCROLLBAR)
	    textsw_update_scrollbars(folio, TEXTSW_VIEW_NULL);
	else if ((flag & TXTSW_UPDATE_SCROLLBAR_IF_NEEDED) &&
		 UPDATE_SCROLLBAR(delta, old_length))
	    textsw_update_scrollbars(folio, TEXTSW_VIEW_NULL);
    }
    if (record && (buf_len <= 100))
	textsw_record_input(folio, buf, buf_len);

    (void) ev_span(folio->views, old_insert_pos + delta, &new_first,
                &new_last_plus_one, EI_SPAN_LINE);
    if (new_first != ES_CANNOT_SET)
        folio->column_offset = (old_insert_pos + delta) - new_first;
    return (delta);
}

static          Textsw_index
textswInsert(abstract, buf, buf_len)
    Textsw          abstract;
    void           *buf;
    long int        buf_len;
{
    Textsw_view_handle view = VIEW_ABS_TO_REP(abstract);
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    Es_index        result;

#ifdef OW_I18N
    textsw_implicit_commit(folio);
#endif

    textsw_take_down_caret(folio);
    result = textsw_do_input(view, buf, buf_len,
			     TXTSW_UPDATE_SCROLLBAR);    
    return (result);
}

Xv_public          Textsw_index
textsw_insert_wcs(abstract, buf, buf_len)
    Textsw          abstract;
    wchar_t        *buf;
    long int        buf_len;
{
    if (multibyte)
        return(textswInsert(abstract, buf, buf_len));
    else {

        /* single byte locale so all wide chars will be reduced to
           single bytes by wcstombs.
        */

        Textsw_view_handle view = VIEW_ABS_TO_REP(abstract);
        register Textsw_folio folio = FOLIO_FOR_VIEW(view);
        Es_index        result;
        char            cbuf[INSERT_BUFSIZE];
        char	       *cbuf_ptr;
        int             cbuf_len;

        textsw_implicit_commit(folio);

        if (buf_len >= INSERT_BUFSIZE)
	    cbuf_ptr = xv_malloc(buf_len + 1);
        else
	    cbuf_ptr = cbuf;

        if ((cbuf_len = wcstombs(cbuf_ptr, buf, buf_len)) == -1)
            cbuf_len = 0;

        cbuf_ptr[cbuf_len] = NULL;

        textsw_take_down_caret(folio);
        result = textsw_do_input(view, cbuf_ptr, cbuf_len,
			     TXTSW_UPDATE_SCROLLBAR);
        if (buf_len >= INSERT_BUFSIZE)
    	    free((char *)cbuf_ptr);
        if (result) /* not error, or cbuf_len is not 0 */
	    result = buf_len;
        return (result);
    }
}

Xv_public          Textsw_index
textsw_insert(abstract, buf, buf_len)
    Textsw          abstract;
    char           *buf;
    long int        buf_len;
{
    if (!multibyte)
        return(textswInsert(abstract, buf, buf_len));
    else {
        Textsw_view_handle view = VIEW_ABS_TO_REP(abstract);
        register Textsw_folio folio = FOLIO_FOR_VIEW(view);
        Es_index        result;
        wchar_t         wbuf[INSERT_BUFSIZE];
        wchar_t	       *wbuf_ptr;
        int             wbuf_len, unused, unconverted_bytes = buf_len;

        textsw_implicit_commit(folio);

        if (buf_len >= INSERT_BUFSIZE)
	    wbuf_ptr = xv_malloc((buf_len + 1) * sizeof(wchar_t));
        else
	    wbuf_ptr = wbuf;

        wbuf_len = textsw_mbstowcs_by_mblen(wbuf_ptr, buf, 
                                     &unconverted_bytes, &unused);

        wbuf_ptr[wbuf_len] = NULL;

        textsw_take_down_caret(folio);
        result = textsw_do_input(view, wbuf_ptr, wbuf_len,
			     TXTSW_UPDATE_SCROLLBAR);
        if (buf_len >= INSERT_BUFSIZE)
    	    free((char *)wbuf_ptr);
        if (result) /* not error, or wbuf_len is not 0 */
	    result = buf_len - unconverted_bytes;
        return (result);
    }

}

static          Textsw_index
textswReplaceOther(abstract, first, last_plus_one, buf, buf_len)
    Textsw          abstract;
    Es_index        first, last_plus_one;
    void           *buf;
    long int        buf_len;
{
    extern void     textsw_remove_mark_internal();
    pkg_private     Ev_mark_object
                    textsw_add_mark_internal();
    Ev_mark_object  saved_insert_mark;
    Es_index        saved_insert, temp;
    Textsw_view_handle view = VIEW_ABS_TO_REP(abstract);
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    register Ev_chain chain = folio->views;
    Es_index        result, insert_result;
    int             lower_context;

    textsw_implicit_commit(folio);
    if (first < 0) /* wrong specfication */
	return ((Es_index) 0);
    insert_result = 0;
    textsw_take_down_caret(folio);
    /* BUG ALERT: change this to avoid the double paint. */
    if (first < last_plus_one) {
	ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, TRUE, 0);

        /* there is some reverse logic here because of calling sequence. */
        if (multibyte) {
	    first = textsw_wcpos_from_mbpos(folio, first);
	    last_plus_one = textsw_wcpos_from_mbpos(folio, last_plus_one);
        } else {
	    first = textsw_mbpos_from_wcpos(folio, first);
	    last_plus_one = textsw_mbpos_from_wcpos(folio, last_plus_one);
        }

	result = textsw_delete_span(view, first, last_plus_one,
				    TXTSW_DS_ADJUST | TXTSW_DS_RETURN_BYTES);
	ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, FALSE, 0);
        /* if delete_span failed */
        if (result == 0)
            return 0;
	if (result == ES_CANNOT_SET) {
	    if (ES_REPLACE_DIVERTED == (Es_status)
		es_get(folio->views->esh, ES_STATUS)) {
		result = 0;
	    }
	}
    } else {
	/* need first for insertion pos. */
        if (multibyte)
	    first = textsw_wcpos_from_mbpos(folio, first);
        else
	    first = textsw_mbpos_from_wcpos(folio, first);
	result = 0;
    }
  
    /* changing none to all should perform correctly */
    if(result == ES_CANNOT_SET && 
       first == 0 && 
       last_plus_one == TEXTSW_INFINITY)
      result = 1;

    if (result == ES_CANNOT_SET) {
	result = 0;
    } else {
	wchar_t    wbuf[REPLACE_BUFSIZE];
	wchar_t   *wbuf_ptr;
	int	wbuf_len, unused, unconverted_bytes = buf_len;

	if (buf_len >= REPLACE_BUFSIZE)
		wbuf_ptr=xv_malloc((buf_len + 1)*sizeof(wchar_t));
	else
		wbuf_ptr = wbuf;

        if (multibyte) {
	    wbuf_len = textsw_mbstowcs_by_mblen(wbuf_ptr, buf,
					    &unconverted_bytes, &unused);
	    wbuf_ptr[wbuf_len] = NULL;
        } else {
	    if ((wbuf_len = wcstombs((char *)wbuf_ptr,(wchar_t *)buf, buf_len))
                  == -1 )
                wbuf_len = 0;
	    ((char *)wbuf_ptr)[wbuf_len] = NULL;
            unconverted_bytes -= wbuf_len;
        }
	ev_check_insert_visibility(chain);
	lower_context =
	    (int) ev_get(view->e_view, EV_CHAIN_LOWER_CONTEXT);
	ev_set(view->e_view,
	       EV_CHAIN_LOWER_CONTEXT, EV_NO_CONTEXT, 0);

	saved_insert = EV_GET_INSERT(chain);
	saved_insert_mark =
	    textsw_add_mark_internal(folio, saved_insert,
				     TEXTSW_MARK_MOVE_AT_INSERT);
	EV_SET_INSERT(chain, first, temp);
	insert_result += textsw_do_input(view, wbuf_ptr, wbuf_len,
					 TXTSW_DONT_UPDATE_SCROLLBAR);
	if (buf_len >= REPLACE_BUFSIZE)  
		free ((char *)wbuf_ptr);

	if (insert_result) /* not error, or wbuf_len is not 0 */
		result += buf_len - unconverted_bytes;
	saved_insert = textsw_find_mark_internal(folio, saved_insert_mark);
	if AN_ERROR
	    (saved_insert == ES_INFINITY) {
	} else
	    EV_SET_INSERT(chain, saved_insert, temp);
	textsw_remove_mark_internal(folio, saved_insert_mark);

	ev_set(view->e_view,
	       EV_CHAIN_LOWER_CONTEXT, lower_context, 0);
	ev_scroll_if_old_insert_visible(chain,
					saved_insert, insert_result);
	textsw_update_scrollbars(folio, TEXTSW_VIEW_NULL);
    }
    return (result);
}

Pkg_private          Textsw_index
textswReplaceWCS(abstract, first, last_plus_one, buf, buf_len)
    Textsw          abstract;
    Es_index        first, last_plus_one;
    wchar_t        *buf;
    long int        buf_len;
{
    if (multibyte)
        return (textsw_replace(abstract, first, last_plus_one, buf, buf_len));
    else {
        return (textswReplaceOther(abstract,first,last_plus_one,buf,buf_len));
    }
}

Xv_public          Textsw_index
textsw_replace_wcs(abstract, first, last_plus_one, buf, buf_len)
    Textsw          abstract;
    Es_index        first, last_plus_one;
    wchar_t        *buf;
    long int        buf_len;
{
    Textsw_view_handle	view = VIEW_ABS_TO_REP(abstract);
    textsw_implicit_commit(FOLIO_FOR_VIEW(view));
    return (textswReplaceWCS(abstract, first, last_plus_one, buf, buf_len));
}

Pkg_private          Textsw_index
textswReplaceBytes(abstract, first, last_plus_one, buf, buf_len)
    Textsw          abstract;
    Es_index        first, last_plus_one;
    char           *buf;
    long int        buf_len;
{
    if (!multibyte)
        return (textsw_replace(abstract, first, last_plus_one, buf, buf_len));
    else {
        return (textswReplaceOther(abstract,first,last_plus_one,buf,buf_len));
   }
}

Xv_public          Textsw_index
textsw_replace_bytes(abstract, first, last_plus_one, buf, buf_len)
    Textsw          abstract;
    Es_index        first, last_plus_one;
    char           *buf;
    long int        buf_len;
{
    int browse_state, read_only_state;
    Textsw_index index;
    Textsw_view_handle	view = VIEW_ABS_TO_REP(abstract);

    /* Save the initial read_only and browsing states. BID 1152756 */
    browse_state = (int) xv_get (abstract, TEXTSW_BROWSING);
    read_only_state = (int) xv_get (abstract, TEXTSW_READ_ONLY);
    xv_set (abstract, TEXTSW_BROWSING, FALSE, NULL);
    xv_set (abstract, TEXTSW_READ_ONLY, FALSE, NULL);
    textsw_implicit_commit(FOLIO_FOR_VIEW(view));
    index = textswReplaceBytes(abstract, first, last_plus_one, buf, buf_len);

    /* Restore the original read_only and browsing states. BID 1152756 */
    xv_set (abstract, TEXTSW_BROWSING, browse_state, NULL);
    xv_set (abstract, TEXTSW_READ_ONLY, read_only_state, NULL);
    return (index);
}


Pkg_private          Textsw_index
textsw_replace(abstract, first, last_plus_one, buf, buf_len)
    Textsw          abstract;
    Es_index        first, last_plus_one;
    void           *buf;
    long int        buf_len;
{
    extern void     textsw_remove_mark_internal();
    pkg_private     Ev_mark_object
                    textsw_add_mark_internal();
    Ev_mark_object  saved_insert_mark;
    Es_index        saved_insert, temp;
    Textsw_view_handle view = VIEW_ABS_TO_REP(abstract);
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    register Ev_chain chain = folio->views;
    Es_index        result, insert_result;
    int             lower_context;

    insert_result = 0;
    textsw_take_down_caret(folio);
    /* BUG ALERT: change this to avoid the double paint. */
    if (first < last_plus_one) {
	ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, TRUE, 0);
	result = textsw_delete_span(view, first, last_plus_one,
				    TXTSW_DS_ADJUST);
	ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, FALSE, 0);
        /* if delete_span failed */
        if (result == 0)
            return 0;
	if (result == ES_CANNOT_SET) {
	    if (ES_REPLACE_DIVERTED == (Es_status)
		es_get(folio->views->esh, ES_STATUS)) {
		result = 0;
	    }
	}
    } else {
	result = 0;
    }
  
    /* changing none to all should perform correctly */
    if(result == ES_CANNOT_SET && 
       first == 0 && 
       last_plus_one == TEXTSW_INFINITY)
      result = 1;

    if (result == ES_CANNOT_SET) {
	result = 0;
    } else {
	ev_check_insert_visibility(chain);
	lower_context =
	    (int) ev_get(view->e_view, EV_CHAIN_LOWER_CONTEXT);
	ev_set(view->e_view,
	       EV_CHAIN_LOWER_CONTEXT, EV_NO_CONTEXT, 0);

	saved_insert = EV_GET_INSERT(chain);
	saved_insert_mark =
	    textsw_add_mark_internal(folio, saved_insert,
				     TEXTSW_MARK_MOVE_AT_INSERT);
	EV_SET_INSERT(chain, first, temp);
	insert_result += textsw_do_input(view, buf, buf_len,
					 TXTSW_DONT_UPDATE_SCROLLBAR);
	result += insert_result;
	saved_insert = textsw_find_mark_internal(folio, saved_insert_mark);
	if AN_ERROR
	    (saved_insert == ES_INFINITY) {
	} else
	    EV_SET_INSERT(chain, saved_insert, temp);
	textsw_remove_mark_internal(folio, saved_insert_mark);

	ev_set(view->e_view,
	       EV_CHAIN_LOWER_CONTEXT, lower_context, 0);
	ev_scroll_if_old_insert_visible(chain,
					saved_insert, insert_result);
	textsw_update_scrollbars(folio, TEXTSW_VIEW_NULL);
    }
    return (result);
}


/*
 *   *byte_len returns the number of bytes was unconverted.
 *   If all bytes in str is converted, and *byte_len > 0 (means that
 *   *byte_len is bigger than the number of bytes in buf),
 *   *big_len_flag returns 1, else returns 0. 
 */
pkg_private int
textsw_mbstowcs_by_mblen(wstr, str, byte_len, big_len_flag)
    register wchar_t   *wstr;
    register char   *str;
    int		    *byte_len;
    int		    *big_len_flag;
{
    register int    rest_byte = *byte_len; /* for a bit performance up */
    register int    bytes;
    wchar_t	   *wstr_org = wstr;

    *big_len_flag = 0;
    if (str == NULL || wstr == NULL)
	return (0);

    while (rest_byte > 0 && *str) {
	/*
 	 * Following ifdef is for performance up. Direct cast multibye to wide
	 * char is available against ascii characters with sun compiler.
 	 */
#ifdef sun
	if (isascii(*str)) {
	    *wstr++ = (wchar_t)*str++;
	    rest_byte--;
	    continue;
	}
#endif
	if ((bytes = mbtowc(wstr++, str, rest_byte)) == -1) {
	    *(--wstr) = NULL;
	    goto Done;
	}
	str += bytes;
	rest_byte -= bytes;
    }
    if (rest_byte > 0)
	*big_len_flag = 1;
Done:
    *byte_len = rest_byte;
    return (wstr - wstr_org);
}


/* Skip unconverted byte untill the last limit_bytes of str. */
pkg_private int
textsw_error_skip_mbstowcs(wstr, str, byte_len, limit_bytes)
    register wchar_t	*wstr;
    register char	*str;
    int			*byte_len;
    int			limit_bytes;
{
    register	rest_byte = *byte_len; /* for a bit performance up */
    register	bytes;
    wchar_t       *wstr_org = wstr;

    if (str == NULL || wstr == NULL)
	return (0);

    while (rest_byte > 0 && *str) {
	/*
 	 * Following ifdef is for performance up. Direct cast multibye to wide
	 * char is available against ascii characters with sun compiler.
 	 */
#ifdef sun
	if (isascii(*str)) {
	    *wstr++ = (wchar_t)*str++;
	    rest_byte--;
	    continue;
	}
#endif
	if ((bytes = mbtowc(wstr, str, rest_byte)) == -1) { /* unconverted */
	    if (rest_byte >= limit_bytes) {	/* then skip one byte */
		str++;
		rest_byte--;
		continue;
	    }
	    break;
	}
	str += bytes;
	rest_byte -= bytes;
	wstr++;
    }
    *byte_len = rest_byte;
    return (wstr - wstr_org);
}

#undef INSERT_BUFSIZE
#undef REPLACE_BUFSIZE
