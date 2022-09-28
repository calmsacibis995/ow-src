#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)txt_caret.c 20.31 94/03/09";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Routines for moving textsw caret.
 */


#include <xview_private/primal.h>
#include <xview_private/txt_impl.h>
#include <xview_private/ev_impl.h>

#define NEWLINE '\n'

extern void textsw_SetSelection();

Pkg_private void textsw_record_caret_motion();

static void
textsw_make_insert_visible(view, visible_status, old_insert, new_insert)
    Textsw_view_handle view;
    unsigned        visible_status;	/* For old_insert */
    Es_index        old_insert, new_insert;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    register Ev_handle ev_handle = view->e_view;
    register unsigned normalize_flag = (TXTSW_NI_NOT_IF_IN_VIEW | TXTSW_NI_MARK);
    int             lower_context, upper_context, scroll_lines;

    /*
     * This procedure will only do display if the new_insert position is not
     * in view. It will scroll, if the old_insert position is in view, but
     * not new_insert. It will repaint the entire view, if the old_insert and
     * new_insert position are not in view.
     */
    lower_context = (int) ev_get(ev_handle, EV_CHAIN_LOWER_CONTEXT);
    upper_context = (int) ev_get(ev_handle, EV_CHAIN_UPPER_CONTEXT);
    if (visible_status == EV_XY_VISIBLE) {
	int             lines_in_view = textsw_screen_line_count(VIEW_REP_TO_ABS(view));

	if (old_insert > new_insert)	/* scroll backward */
	    scroll_lines = ((upper_context > 0) && (lines_in_view >= upper_context))
		? -upper_context : -1;
	else
	    scroll_lines = ((lower_context > 0) && (lines_in_view >= lower_context))
		? lower_context : 1;

	while (!EV_INSERT_VISIBLE_IN_VIEW(ev_handle)) {
	    ev_scroll_lines(ev_handle, scroll_lines, FALSE);
	}
	textsw_update_scrollbars(folio, view);
    } else {
	if (old_insert <= new_insert) {
	    normalize_flag |= TXTSW_NI_AT_BOTTOM;
	    upper_context = 0;
	}
	lower_context = 0;
	textsw_normalize_internal(view, new_insert, new_insert,
			      upper_context, lower_context, normalize_flag);
    }
}

Pkg_private     Es_index
textsw_move_backward_a_word(view, pos)
    register Textsw_view_handle view;
    Es_index        pos;
{
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    register Ev_chain chain = folio->views;
    unsigned        span_flag = (EI_SPAN_WORD | EI_SPAN_LEFT_ONLY);
    Es_index        first, last_plus_one;
    unsigned        span_result = EI_SPAN_NOT_IN_CLASS;


    if (pos == 0)		/* This is already at the beginning of the
				 * file */
	return (ES_CANNOT_SET);

    while ((pos != 0) && (pos != ES_CANNOT_SET) &&
	   (span_result & EI_SPAN_NOT_IN_CLASS)) {
	span_result = ev_span(chain, pos, &first, &last_plus_one, span_flag);
	pos = ((pos == first) ? pos-- : first);
    }

    return (pos);
}


Pkg_private     Es_index
textsw_move_down_a_line(view, pos, file_length, lt_index, rect)
    register Textsw_view_handle view;
    Es_index        pos, file_length;
    int             lt_index;
    struct rect     rect;

{
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    register Ev_handle ev_handle = view->e_view;
    int             line_height = ei_line_height
    (ev_handle->view_chain->eih);
    int             new_x, new_y;
    Es_index        new_pos;
    Es_index        current_first, current_last_plus_one;
    Es_index        new_first, new_last_plus_one;
    unsigned        new_line_length;
    int             lower_context, scroll_lines;
    Ev_impl_line_seq line_seq = (Ev_impl_line_seq)
    ev_handle->line_table.seq;
    Pkg_private int textsw_get_recorded_x();


    if ((pos >= file_length))
	return (ES_CANNOT_SET);

    if (pos >= line_seq[ev_handle->line_table.last_plus_one - 2].pos) {
	int             lines_in_view = textsw_screen_line_count(VIEW_REP_TO_ABS(view));

	lower_context = (int) ev_get(ev_handle, EV_CHAIN_LOWER_CONTEXT);
	scroll_lines = ((lower_context > 0) && (lines_in_view > lower_context)) ? lower_context + 1 : 1;
	ev_scroll_lines(ev_handle, scroll_lines, FALSE);
	new_y = rect.r_top - (line_height * (scroll_lines - 1));
	textsw_update_scrollbars(folio, view);
    } else
	new_y = rect.r_top + line_height;

    new_x = rect.r_left;

    (void) textsw_record_caret_motion(folio, TXTSW_NEXT_LINE, new_x);

    (void) ev_span(folio->views, pos, &current_first, &current_last_plus_one,
                    EI_SPAN_LINE);
    (void) ev_span(folio->views, current_last_plus_one, &new_first, 
		    &new_last_plus_one, EI_SPAN_LINE);

    new_line_length = new_last_plus_one - new_first;

    if (!new_line_length) {
            if (multibyte) {
	        wchar_t	buf[1];
	        (void) textsw_get_contents_wcs(folio, 
                                   current_last_plus_one - 1, buf, 1);
		return ((buf[0] == NEWLINE) ? file_length : pos);
            } 
	    else {
                char buf[1];
	        (void) textsw_get_contents(folio, 
                                   current_last_plus_one - 1, buf, 1);
		return ((buf[0] == NEWLINE) ? file_length : pos);
            }
    }
    
    /* if the column offset is greater than the length of the 
     * next line, put the caret at the end of the next line 
     */
    if (folio->column_offset >= new_line_length)
	/* make sure the caret goes before a newline */
        if (new_last_plus_one < file_length) 
	    new_pos = new_last_plus_one - 1;
	else {
            if (multibyte) {
	        wchar_t	buf[1];
	        (void) textsw_get_contents_wcs(folio, 
                                   new_last_plus_one - 1, buf, 1);
	        new_pos = (buf[0] == NEWLINE) ?
	            new_last_plus_one - 1 : new_last_plus_one;
            } else {
                char buf[1];
	        (void) textsw_get_contents(folio, 
                                   new_last_plus_one - 1, buf, 1);
	        new_pos = (buf[0] == NEWLINE) ?
	            new_last_plus_one - 1 : new_last_plus_one;
            }
	}
    else   /* column offset is less than length of next line */
	new_pos = new_first + folio->column_offset;

    return ((new_pos >= 0 && new_pos <= file_length) ?
	    new_pos : ES_CANNOT_SET);
}

Pkg_private     Es_index
textsw_move_forward_a_word(view, pos, file_length)
    Textsw_view_handle view;
    Es_index        pos;
    Es_index        file_length;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    register Ev_chain chain = folio->views;
    unsigned        span_flag = (EI_SPAN_WORD | EI_SPAN_RIGHT_ONLY);
    Es_index        first, last_plus_one;
    unsigned        span_result = EI_SPAN_NOT_IN_CLASS;

    if (pos >= file_length)
	return (ES_CANNOT_SET);

    /* Get to the end of current word, and use that as start pos */
    (void) ev_span(chain, pos, &first, &last_plus_one, span_flag);

    pos = ((last_plus_one == file_length) ? ES_CANNOT_SET : last_plus_one);

    while ((pos != ES_CANNOT_SET) && (span_result & EI_SPAN_NOT_IN_CLASS)) {
	span_result = ev_span(chain, pos, &first, &last_plus_one,
			      span_flag);

	if (pos == last_plus_one)
	    pos = ((pos == file_length) ? ES_CANNOT_SET : pos++);
	else
	    pos = last_plus_one;
    }


    return ((pos == ES_CANNOT_SET) ? ES_CANNOT_SET : first);

}

Pkg_private     Es_index
textsw_move_to_word_end(view, pos, file_length)
    Textsw_view_handle view;
    Es_index        pos;
    Es_index        file_length;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    register Ev_chain chain = folio->views;
    unsigned        span_flag = (EI_SPAN_WORD | EI_SPAN_RIGHT_ONLY);
    Es_index        first, last_plus_one;
    unsigned        span_result = EI_SPAN_NOT_IN_CLASS;

    if (pos >= file_length)
	return (ES_CANNOT_SET);

    while ((pos != ES_CANNOT_SET) && (span_result & EI_SPAN_NOT_IN_CLASS)) {
	span_result = ev_span(chain, pos, &first, &last_plus_one,
			      span_flag);

	if (pos == last_plus_one)
	    pos = ((pos == file_length) ? ES_CANNOT_SET : pos++);
	else
	    pos = last_plus_one;
    }


    return (pos);
}



Pkg_private     Es_index
textsw_move_to_line_start(view, pos)
    Textsw_view_handle view;
    Es_index        pos;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    unsigned        span_flag = (EI_SPAN_LINE | EI_SPAN_LEFT_ONLY);
    Es_index        first, last_plus_one;

    if (pos == 0)
	return (ES_CANNOT_SET);

    (void) ev_span(folio->views, pos, &first, &last_plus_one, span_flag);

    return (first);
}



pkg_private     Es_index
textsw_move_to_line_end(view, pos, file_length)
    Textsw_view_handle view;
    Es_index        pos;
    Es_index        file_length;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    register Ev_chain chain = folio->views;
    unsigned        span_flag = (EI_SPAN_LINE | EI_SPAN_RIGHT_ONLY);
    Es_index        first, last_plus_one;

    if (pos >= file_length)
	return (ES_CANNOT_SET);

    (void) ev_span(chain, pos, &first, &last_plus_one, span_flag);

    if (last_plus_one < file_length)
	return (--last_plus_one);
    else {
        if (!multibyte) {
	    char buf[1];
	    (void) textsw_get_contents(folio, --last_plus_one, buf, 1);
	    return ((buf[0] == NEWLINE) ? last_plus_one : file_length);
        } else {
            wchar_t buf[1];
	    (void) textsw_get_contents_wcs(folio, --last_plus_one, buf, 1);
	    return ((buf[0] == NEWLINE) ? last_plus_one : file_length);
        }
    }
}


pkg_private     Es_index
textsw_move_next_line_start(view, pos, file_length)
    Textsw_view_handle view;
    Es_index        pos;
    Es_index        file_length;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    unsigned        span_flag = (EI_SPAN_LINE | EI_SPAN_RIGHT_ONLY);
    Es_index        first, last_plus_one;

    if (pos >= file_length)
	return (ES_CANNOT_SET);

    (void) ev_span(folio->views, pos, &first, &last_plus_one, span_flag);

    return ((last_plus_one == file_length) ? ES_CANNOT_SET : last_plus_one);
}


pkg_private     Es_index
textsw_move_up_a_line(view, pos, file_length, lt_index, rect)
    register Textsw_view_handle view;
    Es_index        pos;
    Es_index        file_length;
    int             lt_index;
    struct rect     rect;
{
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    register Ev_handle ev_handle = view->e_view;
    int             line_height = ei_line_height(
						 ev_handle->view_chain->eih);
    int             new_x, new_y;
    Es_index        new_pos;
    Es_index        current_first, current_last_plus_one;
    Es_index        new_first, new_last_plus_one;
    unsigned        new_line_length;
    int             upper_context, scroll_lines;
    Ev_impl_line_seq line_seq = (Ev_impl_line_seq)
    ev_handle->line_table.seq;
    pkg_private int textsw_get_recorded_x();

    if ((pos == 0) || (line_seq[lt_index].pos == 0))
	return (ES_CANNOT_SET);

    if (pos < line_seq[1].pos) {
	int             lines_in_view = textsw_screen_line_count(VIEW_REP_TO_ABS(view));

	upper_context = (int) ev_get(ev_handle, EV_CHAIN_UPPER_CONTEXT);

	scroll_lines = ((upper_context > 0) && (lines_in_view > upper_context)) ? upper_context + 1 : 1;

	ev_scroll_lines(ev_handle, -scroll_lines, FALSE);
	textsw_update_scrollbars(folio, view);
	new_y = rect.r_top + (line_height * (scroll_lines - 1));
    } else
	new_y = rect.r_top - line_height;

    new_x = rect.r_left;

    (void) textsw_record_caret_motion(folio, TXTSW_PREVIOUS_LINE, new_x);

    (void) ev_span(folio->views, pos, &current_first, 
		    &current_last_plus_one, EI_SPAN_LINE);
    (void) ev_span(folio->views, current_first - 1, &new_first, 
		    &new_last_plus_one, EI_SPAN_LINE);
    new_line_length = new_last_plus_one - new_first;

    if (current_last_plus_one != ES_CANNOT_SET) {
        new_pos = (folio->column_offset >= new_line_length) ?
	        new_last_plus_one - 1 : new_first + folio->column_offset;
    } else {     /* current_pos is the last character in textsw */
        if (multibyte) {
	    wchar_t buf[1];
	    (void) textsw_get_contents_wcs(folio, file_length - 1, buf, 1);
	    new_pos = ev_resolve_xy(ev_handle, new_x, new_y);
        } else {
            char buf[1];
	    (void) textsw_get_contents(folio, file_length - 1, buf, 1);
	    new_pos = ev_resolve_xy(ev_handle, new_x, new_y);
        }

	/* got the new pos, now adjust for column offset */
        (void) ev_span(folio->views, new_pos, &new_first,
                    &new_last_plus_one, EI_SPAN_LINE);
	/* if the new column offset is less than the saved column offset */
	if (new_pos - new_first < folio->column_offset) 
	    /* add the column offset unless we are at the end of the line */
	    new_pos = (new_first + folio->column_offset < new_last_plus_one) ?
	              new_first + folio->column_offset : new_last_plus_one - 1;
	else
	    /* save the new column offset */
            folio->column_offset = new_pos - new_first;
    }

    return ((new_pos >= 0 && new_pos <= file_length) ?
	    new_pos : ES_CANNOT_SET);
}

pkg_private void
textsw_move_caret(view, direction)
    Textsw_view_handle view;
    Textsw_Caret_Direction direction;

{
    int             ok = TRUE;
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    register Ev_chain chain = folio->views;
    Es_index        old_pos, pos;
    unsigned        visible_status, set_last_column;
    Es_index        file_length = es_get_length(chain->esh);
    int             lt_index;
    struct rect     rect;
    Es_index        first, last_plus_one;
    Es_index        new_first, new_last_plus_one;
    Ev_handle       ev_handle = view->e_view;

    if (file_length == 0) {
	(void) window_bell(WINDOW_FROM_VIEW(view));
	return;
    }
    textsw_flush_caches(view, TFC_STD);
    textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
			   (caddr_t) TEXTSW_INFINITY - 1);
    pos = ES_CANNOT_SET;
    set_last_column = FALSE;
    old_pos = EV_GET_INSERT(chain);
    visible_status = ev_xy_in_view(ev_handle, old_pos, &lt_index, &rect);

    switch (direction) {
      case TXTSW_CHAR_BACKWARD:
	pos = ((old_pos == 0) ? ES_CANNOT_SET : (old_pos-1));
	set_last_column = TRUE;
	break;
      case TXTSW_CHAR_FORWARD:
	pos = ((old_pos >= file_length) ? ES_CANNOT_SET : (old_pos+1));
        set_last_column = TRUE;
	break;
      case TXTSW_DOCUMENT_END:

	if ((old_pos < file_length) ||
	    (visible_status != EV_XY_VISIBLE)) {
	    pos = file_length;
	    visible_status = EV_XY_BELOW;
            set_last_column = TRUE;
	} else
	    pos = ES_CANNOT_SET;

	break;
      case TXTSW_DOCUMENT_START:
	if ((old_pos > 0) || (visible_status != EV_XY_VISIBLE)) {
	    pos = 0;
	    visible_status = EV_XY_ABOVE;
            set_last_column = TRUE;
	} else
	    pos = ES_CANNOT_SET;

	break;
      case TXTSW_LINE_END:
	pos = textsw_move_to_line_end(view, old_pos, file_length);
        set_last_column = TRUE;
	break;
      case TXTSW_LINE_START:
	pos = textsw_move_to_line_start(view, old_pos);
        set_last_column = TRUE;
	break;
      case TXTSW_NEXT_LINE_START:
	pos = textsw_move_next_line_start(view, old_pos, file_length);
        set_last_column = TRUE;
	break;
      case TXTSW_NEXT_LINE:{
	    int             lower_context =
	    (int) ev_get(ev_handle, EV_CHAIN_LOWER_CONTEXT);

	    if (visible_status != EV_XY_VISIBLE) {
		textsw_normalize_internal(view, old_pos, old_pos,
					  0, lower_context + 1,
				  (TXTSW_NI_NOT_IF_IN_VIEW | TXTSW_NI_MARK |
				   TXTSW_NI_AT_BOTTOM));
		visible_status = ev_xy_in_view(ev_handle, old_pos,
					       &lt_index, &rect);
	    }
	    if (visible_status == EV_XY_VISIBLE)
		pos = textsw_move_down_a_line(view, old_pos, file_length,
					      lt_index, rect);
	    break;
	}
      case TXTSW_PREVIOUS_LINE:{
	    int             upper_context =
	    (int) ev_get(ev_handle, EV_CHAIN_UPPER_CONTEXT);

	    if (visible_status != EV_XY_VISIBLE) {
		textsw_normalize_internal(view, old_pos, old_pos,
					  upper_context + 1, 0,
				 (TXTSW_NI_NOT_IF_IN_VIEW | TXTSW_NI_MARK));
		visible_status = ev_xy_in_view(ev_handle, old_pos,
					       &lt_index, &rect);
	    }
	    if (visible_status == EV_XY_VISIBLE)
		pos = textsw_move_up_a_line(view, old_pos, file_length,
					    lt_index, rect);
	    break;
	}
      case TXTSW_WORD_BACKWARD:
	pos = textsw_move_backward_a_word(view, old_pos);
        set_last_column = TRUE;
	break;
      case TXTSW_WORD_FORWARD:
	pos = textsw_move_forward_a_word(view, old_pos, file_length);
        set_last_column = TRUE;
	break;
      case TXTSW_WORD_END:
	pos = textsw_move_to_word_end(view, old_pos, file_length);
        set_last_column = TRUE;
	break;
      default:
	ok = FALSE;
	break;
    }
    
    if (ok) {
	if ((pos == ES_CANNOT_SET) && (visible_status != EV_XY_VISIBLE))
	    pos = old_pos;	/* Put insertion point in view anyway */

	if (pos == ES_CANNOT_SET)
	    (void) window_bell(WINDOW_FROM_VIEW(view));
	else {
	    textsw_set_insert(folio, pos);
	    textsw_make_insert_visible(view, visible_status, old_pos, pos);
	    if (set_last_column) {
	        (void) ev_span(folio->views, pos, &new_first, 
			&new_last_plus_one, EI_SPAN_LINE);
		if (new_first != ES_CANNOT_SET)
		    folio->column_offset = pos - new_first;
		else {
		    (void) ev_span(folio->views, pos - 1, &new_first, 
				&new_last_plus_one, EI_SPAN_LINE);
		    if (multibyte) {
			wchar_t	buf[1];
			(void) textsw_get_contents_wcs(folio, 
                                   new_last_plus_one - 1, buf, 1);
	 	        if (buf[0] == NEWLINE) 
		           folio->column_offset = 0;
		        else
			   folio->column_offset = new_last_plus_one - new_first;
		    }
		    else {
			char buf[1];
			(void) textsw_get_contents(folio, 
                                   new_last_plus_one - 1, buf, 1);
	 	        if (buf[0] == NEWLINE) 
 		           folio->column_offset = 0;
		        else
			   folio->column_offset = new_last_plus_one - new_first;
 		    }
		}
	    }
	    /* Replace pending delete with primary selection, but only
	       if not read only */
	    if ((EV_SEL_PENDING_DELETE & ev_get_selection(
			     chain, &first, &last_plus_one, EV_SEL_PRIMARY)) &&
		!(TXTSW_IS_READ_ONLY(folio)))
		(void) textsw_SetSelection(
				VIEW_REP_TO_ABS(view), first, last_plus_one,
					    EV_SEL_PRIMARY);

	}
	textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
			       (caddr_t) TEXTSW_INFINITY - 1);

	if ((direction != TXTSW_NEXT_LINE) &&
	    (direction != TXTSW_PREVIOUS_LINE))
	    (void) textsw_record_caret_motion(folio, direction, -1);

    }
}
