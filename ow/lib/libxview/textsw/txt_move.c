#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)txt_move.c 20.98 94/09/09";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Text subwindow move & duplicate
 */

#include <xview_private/primal.h>
#include <xview_private/txt_impl.h>
#include <xview_private/ev_impl.h>
#include <xview_private/draw_impl.h>
#include <xview_private/txt_18impl.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>
#include <xview/cursor.h>
#include <xview/font.h>
#include <xview/pkg.h>
#include <xview/attrol.h>
#include <xview/screen.h>
#include <xview/seln.h>
#include <xview/fullscreen.h>
#include <xview/svrimage.h>
#include <xview/server.h>
#include <xview/win_struct.h>
#include <xview/pixwin.h>
#include <xview/dragdrop.h>
#include <xview/notice.h>
#ifdef SVR4 
#include <stdlib.h> 
#endif /* SVR4 */

static int dnd_data_key = 0; /* XXX: Don't do this at home kids. */
static int dnd_view_key = 0; 
static int DndConvertProc();

Pkg_private Es_handle textsw_esh_for_span();
Pkg_private Es_index ev_resolve_xy();
Pkg_private Es_index textsw_do_balance_beam();
Pkg_private int ev_get_selection();
Pkg_private int xv_pf_text();
Pkg_private int textsw_end_quick_move();
Pkg_private void textsw_finish_move();
Pkg_private void textsw_finish_duplicate();
Pkg_private void textsw_reset_cursor();

Pkg_private void xv_do_move();

extern void setTextswInsertionPoint();
extern void textsw_do_get();

Pkg_private void
getTextswContents(textsw, start, buf, len)
    Textsw textsw;
    Es_index start;
    void *buf;
    int len;
{
    if (!multibyte)
       (void) xv_get(textsw, TEXTSW_CONTENTS, start, buf, len);
    else
       (void) xv_get(textsw, TEXTSW_CONTENTS_WCS, start, buf, len);
}

Pkg_private void
textsw_SetSelection(abstract, first, last_plus_one, type)
    Textsw abstract;
    Es_index first, last_plus_one;
    unsigned type;
{
    if (!multibyte)
        textsw_set_selection(abstract, first, last_plus_one, type);
    else
        textsw_set_selection_wcs(abstract, first, last_plus_one, type);
}


static unsigned short    drag_move_arrow_data[] = {
#include <images/text_move_cursor.pr>
};

mpr_static(drag_move_arrow_pr, 16, 16, 1, drag_move_arrow_data);

struct  textsw_context {
    int    size;
    char   *sel_buffer;
};


static int
stringLength(str)
void *str;
{
    if (!multibyte)
        return(strlen(str));
    else
        return(wslen(str));
}

Pkg_private	void
textsw_save_selection(folio)
    Textsw_folio    folio;
{
    static          repeat_call;

    if (!repeat_call) {
	(void) ev_get_selection(folio->views,
			     &folio->move_first, &folio->move_last_plus_one,
				EV_SEL_PRIMARY);
	repeat_call = TRUE;
    }
}


Pkg_private int
textsw_do_move(view, selection_is_local)
    Textsw_view_handle view;
    int             selection_is_local;
{
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    Seln_holder     holder;
    Seln_request   *result;
    char           *data;
    int             is_read_only,easy;

    /* same as a secondary paste, but need to delete the text afterwards. */
    
    easy = textsw_inform_seln_svc(folio, TXTSW_FUNC_GET, FALSE);
    textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
			   (caddr_t) TEXTSW_INFINITY - 1);
    textsw_do_get(view, easy);
    holder = seln_inquire(SELN_SECONDARY);
    result = seln_ask(&holder, SELN_REQ_IS_READONLY, 0, 0);
    if (result->status==SELN_SUCCESS) {
	data = result->data;
	/* Test if is SELN_IS_READONLY */
	data += sizeof(Seln_attribute);
	is_read_only = *(int *) data;
    } else {
	is_read_only = FALSE;
    }
    if (!is_read_only) {
	if (easy) {
	    Ev_chain chain = folio->views;
	    Es_index first, last_plus_one, ro_bdry;
	    Es_handle secondary=ES_NULL;

	    ro_bdry = textsw_read_only_boundary_is_at(folio);
	    ev_get_selection(chain, &first, &last_plus_one, EV_SEL_SECONDARY);
	    if (last_plus_one <= ro_bdry) {
		textsw_clear_secondary_selection(folio, EV_SEL_SECONDARY);
	    }
	    secondary = textsw_esh_for_span(view, first, 
					    last_plus_one, secondary);
	    if (folio->state & TXTSW_DELETE_REPLACES_CLIPBOARD) {
		textsw_delete_span(view, (first < ro_bdry) ? ro_bdry : first,
				   last_plus_one, TXTSW_DS_SHELVE);
	    } else {
		textsw_delete_span(view, (first < ro_bdry) ? ro_bdry : first,
				   last_plus_one, NULL);
	    }
	    if (first != ES_INFINITY)
		textsw_SetSelection(VIEW_REP_TO_ABS(view),
				    ES_INFINITY,ES_INFINITY,EV_SEL_SECONDARY);
	} else {
	    result = seln_ask(&holder,SELN_REQ_COMMIT_PENDING_DELETE,0,0);
	}
    }
    textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
			   (caddr_t) TEXTSW_INFINITY - 1);

    TEXTSW_DO_INSERT_MAKES_VISIBLE(view);
    folio->track_state &= ~TXTSW_TRACK_QUICK_MOVE;
}

Pkg_private int
textsw_end_quick_move(view)
    Textsw_view_handle view;
{
    extern void     textsw_init_selection_object();
    extern void     textsw_clear_secondary_selection();
    int             result = 0;
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    int             selection_is_local;


    selection_is_local = textsw_inform_seln_svc(folio, TXTSW_FUNC_DELETE, FALSE);
    if ((folio->func_state & TXTSW_FUNC_DELETE) == 0)
	return (0);
    /*
     * if ((folio->func_state & TXTSW_FUNC_EXECUTE) == 0) goto Done;
     */

    if (TXTSW_IS_READ_ONLY(folio)) {
	result = TEXTSW_PE_READ_ONLY;
	textsw_clear_secondary_selection(folio, EV_SEL_SECONDARY);
	goto Done;
    }

#ifdef OW_I18N
    textsw_implicit_commit(folio);
#endif

    textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
			   (caddr_t) TEXTSW_INFINITY - 1);
    ASSUME(allock());
    result = textsw_do_move(view, selection_is_local);
    ASSUME(allock());
    textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
			   (caddr_t) TEXTSW_INFINITY - 1);

Done:
    if (selection_is_local) {
	Xv_opaque server,screen;
	textsw_SetSelection(VIEW_REP_TO_ABS(view),
			    ES_INFINITY, ES_INFINITY,
			    EV_SEL_SECONDARY);
	screen = xv_get(FOLIO_REP_TO_ABS(folio), XV_SCREEN);
	server = xv_get(screen, SCREEN_SERVER);
	seln_give_up_selection(server,SELN_SECONDARY);
    }
    textsw_end_function(view, TXTSW_FUNC_DELETE);
    textsw_update_scrollbars(folio, TEXTSW_VIEW_NULL);
    folio->track_state &= ~TXTSW_TRACK_QUICK_MOVE;
    return (result);


}

Pkg_private void
textsw_track_move(view, ie)
    Textsw_view_handle view;
    register Event *ie;
{

    if (win_inputnegevent(ie))
	textsw_finish_move(view, ie);
}

Pkg_private void
textsw_finish_move(view, ie)
    Textsw_view_handle view;
    register Event *ie;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);

    xv_do_move(view, ie);
    folio->track_state &= ~TXTSW_TRACK_MOVE;
    textsw_reset_cursor(view);
}

Pkg_private	void
textsw_clear_move(view)
    Textsw_view_handle view;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);

    folio->track_state &= ~TXTSW_TRACK_MOVE;
    textsw_reset_cursor(view);
}

Pkg_private void
textsw_track_duplicate(view, ie)
    Textsw_view_handle view;
    register Event *ie;
{

    if (win_inputnegevent(ie))
	textsw_finish_duplicate(view, ie);
}

Pkg_private void
textsw_finish_duplicate(view, ie)
    Textsw_view_handle view;
    register Event *ie;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);

    textsw_do_duplicate(view, ie);
    folio->track_state &= ~TXTSW_TRACK_DUPLICATE;
    textsw_reset_cursor(view);
}

Pkg_private void
textsw_clear_duplicate(view)
    Textsw_view_handle view;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);

    folio->track_state &= ~TXTSW_TRACK_DUPLICATE;
    textsw_reset_cursor(view);
}

Pkg_private void
textsw_reset_cursor(view)
    Textsw_view_handle view;
{
    Xv_Cursor       main_cursor;
    Xv_object       screen, server;

    screen = xv_get(VIEW_REP_TO_ABS(view), XV_SCREEN);
    server = xv_get(screen, SCREEN_SERVER);
    main_cursor = (Xv_Cursor) xv_get(server,
				     XV_KEY_DATA, CURSOR_BASIC_PTR);
    xv_set(VIEW_REP_TO_ABS(view), WIN_CURSOR, main_cursor, 0);
}

/*
 * 1) if two spaces are left after deleting, they should be collapsed. 2) if
 * the selection is a word, when it is moved or duplicated, if there is a
 * space on one end there should be a space on both ends.
 */
/*
 * Menu proc() is called... sets SELN_FN_MOVE to true saves the location of
 * the selection in the folio. As long as track state is TXTSW_TRACK_MOVE,
 * all input events go to track move(). On button up, xv_do_move() is called...
 * resets track state.
 */
Pkg_private void
xv_do_move(view, ie)
    Textsw_view_handle view;
    register Event *ie;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    Textsw          textsw = VIEW_REP_TO_ABS(view);
    Es_index        first, last_plus_one;
    Es_index        pos, original_pos;
    wchar_t         sel[1024], buf[2];
    int             sel_len;

    (void) ev_get_selection(folio->views, &first, &last_plus_one,
			    EV_SEL_PRIMARY);

    textsw_get_selection_as_string(folio, EV_SEL_PRIMARY, sel, 1024);
    sel_len = stringLength(sel);

    pos = ev_resolve_xy(view->e_view, ie->ie_locx, ie->ie_locy);
    pos = textsw_do_balance_beam(view, ie->ie_locx, ie->ie_locy, pos, pos + 1);

    /* don't do anything if destination is within selection */
    if (pos >= first && pos <= last_plus_one)
	return;

    original_pos = pos;

    /* if spaces on either side, collapse them */
    getTextswContents(textsw, first - 1, buf, 2);
    if (buf[0] == ' ') {
	getTextswContents(textsw, last_plus_one, buf, 1);
	if (buf[0] == ' ') {
	    last_plus_one++;
	}
    }
    /* delete the source */
    textsw_delete(textsw, first, last_plus_one);

    /* correct for deletion */
    if (original_pos > first)
	pos -= last_plus_one - first;

    /* if punctuation to the right and space to the left, delete space */
    getTextswContents(textsw, first - 1, buf, 2);

    if (!multibyte) {        
        if (((char *)buf)[1] == '.' || ((char *)buf)[1] == ',' 
               || ((char *)buf)[1] == ';' || ((char *)buf)[1] == ':') {
	    if (((char *)buf)[0] == ' ') {
	        textsw_delete(textsw, first - 1, first);
	        if (original_pos > first)
		    pos--;
	    }
        }
    } else {
        if (((wchar_t *)buf)[1] == '.' || ((wchar_t *)buf)[1] == ',' 
               || ((wchar_t *)buf)[1] == ';' || ((wchar_t *)buf)[1] == ':') {
	    if (((wchar_t *)buf)[0] == ' ') {
	        textsw_delete(textsw, first - 1, first);
	        if (original_pos > first)
		    pos--;
            }
        }
    }

    setTextswInsertionPoint(textsw, pos);
    /* add leading/trailing space to selection if needed for new location */
    getTextswContents(textsw, pos - 1, buf, 2);
    if (!multibyte) {
        if (((char *)buf)[1] == ' ') {
	    if (((char *)buf)[0] != ' ') {
	        /* add leading space */
	        XV_BCOPY(sel, (char *)sel + 1, sel_len);
	        ((char *)sel)[0] = ' ';
	        sel_len++;
	        ((char *)sel)[sel_len] = '\0';
	        textsw_insert(textsw, (char *)sel, sel_len);
	        /* reset selection to original span */
	        textsw_SetSelection(textsw,
				      pos + 1, pos + sel_len, EV_SEL_PRIMARY);
	        return;
	    }
        } else {
	    if (((char *)buf)[0] == ' ') {
	        /* add trailing space */
	        ((char *)sel)[sel_len] = ' ';
	        sel_len++;
	        ((char *)sel)[sel_len] = '\0';
	        textsw_insert(textsw, (char *)sel, sel_len);
	        /* reset selection to original span */
	        textsw_SetSelection(textsw,
				      pos, pos + sel_len - 1, EV_SEL_PRIMARY);
	        /* correct insertion point */
	        setTextswInsertionPoint(textsw, pos + sel_len - 1);
	        return;
	    }
        }
        if (((char *)buf)[1] == '.' ||
	    ((char *)buf)[1] == ',' ||
	    ((char *)buf)[1] == ';' ||
	    ((char *)buf)[1] == ':') {
	    /* before punctuation mark -- add leading space */
	    XV_BCOPY(sel, (char *)sel + 1, stringLength(sel));
	    ((char *)sel)[0] = ' ';
	    sel_len++;
	    ((char *)sel)[sel_len] = '\0';
	    textsw_insert(textsw, (char *)sel, sel_len);
	    /* reset selection to original span */
	    textsw_SetSelection(textsw,
				  pos + 1, pos + sel_len, EV_SEL_PRIMARY);
	    return;
        } else {
	    /* don't add any spaces */
	    textsw_insert(textsw, (char *)sel, sel_len);
	    /* reset selection to original span */
	    textsw_SetSelection(textsw, pos, pos + sel_len, EV_SEL_PRIMARY);
	    return;
        }
    } else {
        if (((wchar_t *)buf)[1] == ' ') {
	    if (((wchar_t *)buf)[0] != ' ') {
	        /* add leading space */
	        XV_BCOPY(sel, sel + 1, sel_len*sizeof(wchar_t));
	        ((wchar_t *)sel)[0] = ' ';
	        sel_len++;
	        ((wchar_t *)sel)[sel_len] = '\0';
	        textsw_insert_wcs(textsw, (wchar_t *)sel, sel_len);
	        /* reset selection to original span */
	        textsw_SetSelection(textsw,
				      pos + 1, pos + sel_len, EV_SEL_PRIMARY);
	        return;
	    }
        } else {
	    if (((wchar_t *)buf)[0] == ' ') {
	        /* add trailing space */
	        ((wchar_t *)sel)[sel_len] = ' ';
	        sel_len++;
	        ((wchar_t *)sel)[sel_len] = '\0';
	        textsw_insert_wcs(textsw, (wchar_t *)sel, sel_len);
	        /* reset selection to original span */
	        textsw_SetSelection(textsw,
				      pos, pos + sel_len - 1, EV_SEL_PRIMARY);
	        /* correct insertion point */
	        setTextswInsertionPoint(textsw, pos + sel_len - 1);
	        return;
	    }
        }
        if (((wchar_t *)buf)[1] == '.' ||
	    ((wchar_t *)buf)[1] == ',' ||
	    ((wchar_t *)buf)[1] == ';' ||
	    ((wchar_t *)buf)[1] == ':') {
	    /* before punctuation mark -- add leading space */
	    XV_BCOPY(sel, sel + 1, stringLength(sel)*sizeof(wchar_t));
	    ((wchar_t *)sel)[0] = ' ';
	    sel_len++;
	    ((wchar_t *)sel)[sel_len] = '\0';
	    textsw_insert_wcs(textsw, (wchar_t *)sel, sel_len);
	    /* reset selection to original span */
	    textsw_SetSelection(textsw,
				  pos + 1, pos + sel_len, EV_SEL_PRIMARY);
	    return;
        } else {
	    /* don't add any spaces */
	    textsw_insert_wcs(textsw, (wchar_t *)sel, sel_len);
	    /* reset selection to original span */
	    textsw_SetSelection(textsw, pos, 
                                      pos + sel_len, EV_SEL_PRIMARY);
	    return;
        }
    }
}

Pkg_private int
textsw_do_duplicate(view, ie)
    Textsw_view_handle view;
    register Event *ie;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    Textsw          textsw = VIEW_REP_TO_ABS(view);
    Es_index        position, pos;
    char            buf[1024*sizeof(wchar_t)];
                    /* allocate for worst case (widechar) */
    int             len;

    pos = ev_resolve_xy(view->e_view, ie->ie_locx, ie->ie_locy);
    position = textsw_do_balance_beam(view, ie->ie_locx, 
                                         ie->ie_locy, pos, pos + 1);
    setTextswInsertionPoint(textsw, position);

    getTextswContents(textsw, position, buf, 1);
    if (!multibyte) {
        if (((char *)buf)[0] == ' ') {
	    getTextswContents(textsw, position - 1, buf, 1);
	    if (((char *)buf)[0] != ' ') {
	        /* space after -- add leading space */
	        ((char *)buf)[0] = ' ';
	        textsw_get_selection_as_string(folio, EV_SEL_PRIMARY,
                                                  (char *)buf + 1, 1024);
	        textsw_insert(textsw, buf, stringLength(buf));
	        /* reset selection to original span */
	        textsw_SetSelection(textsw, position + 1,
				position + stringLength(buf), EV_SEL_PRIMARY);
	        return;
	    }
        } else {
	    getTextswContents(textsw, position - 1, buf, 1);
	    if (((char *)buf)[0] == ' ') {
	        /* space before -- add trailing space */
	        textsw_get_selection_as_string(folio, EV_SEL_PRIMARY, 
                                                  buf, 1024);
	        len = stringLength(buf);
	        ((char *)buf)[len] = ' ';
	        ((char *)buf)[len + 1] = '\0';
	        textsw_insert(textsw, buf, stringLength(buf));
	        textsw_SetSelection(textsw, position,
	              position + stringLength(buf) - 1, EV_SEL_PRIMARY);
	        setTextswInsertionPoint(textsw,
		                position + stringLength(buf) - 1);
	        return;
	    }
        }

        getTextswContents(textsw, position, buf, 1);
        if (((char *)buf)[0] == '.' ||
	    ((char *)buf)[0] == ',' ||
	    ((char *)buf)[0] == ';' ||
	    ((char *)buf)[0] == ':') {
	    /* before punctuation mark -- add leading space */
	    ((char *)buf)[0] = ' ';
	    textsw_get_selection_as_string(folio, EV_SEL_PRIMARY, 
                                              (char *)buf + 1, 1024);
	    textsw_insert(textsw, buf, stringLength(buf));
	    /* reset selection to original span */
	    textsw_SetSelection(textsw, position + 1,
		  position + stringLength(buf), EV_SEL_PRIMARY);
	    return;
        } else {
	    /* don't add any spaces */
	    textsw_get_selection_as_string(folio, EV_SEL_PRIMARY, buf, 1024);
	    textsw_insert(textsw, buf, stringLength(buf));
	    /* reset selection to original span */
	    textsw_SetSelection(textsw, position,
				  position + stringLength(buf), EV_SEL_PRIMARY);
	    return;
        }
    } else {
        if (((wchar_t *)buf)[0] == ' ') {
	    getTextswContents(textsw, position - 1, buf, 1);
	    if (((wchar_t *)buf)[0] != ' ') {
	        /* space after -- add leading space */
	        ((wchar_t *)buf)[0] = ' ';
	        textsw_get_selection_as_string(folio, EV_SEL_PRIMARY,
                                                  (wchar_t *)buf + 1, 1024);
	        textsw_insert_wcs(textsw, (wchar_t *)buf, stringLength(buf));
	        /* reset selection to original span */
	        textsw_SetSelection(textsw, position + 1,
				position + stringLength(buf), EV_SEL_PRIMARY);
	        return;
	    }
        } else {
	    getTextswContents(textsw, position - 1, buf, 1);
	    if (((wchar_t *)buf)[0] == ' ') {
	        /* space before -- add trailing space */
	        textsw_get_selection_as_string(folio, EV_SEL_PRIMARY, 
                                                  buf, 1024);
	        len = stringLength(buf);
	        ((wchar_t *)buf)[len] = ' ';
	        ((wchar_t *)buf)[len + 1] = '\0';
	        textsw_insert_wcs(textsw, (wchar_t *)buf, stringLength(buf));
	        textsw_SetSelection(textsw, position,
	              position + stringLength(buf) - 1, EV_SEL_PRIMARY);
	        setTextswInsertionPoint(textsw, 
                            position + stringLength(buf) - 1);
	        return;
	    }
        }

        getTextswContents(textsw, position, buf, 1);
        if (((wchar_t *)buf)[0] == '.' ||
	    ((wchar_t *)buf)[0] == ',' ||
	    ((wchar_t *)buf)[0] == ';' ||
	    ((wchar_t *)buf)[0] == ':') {
	    /* before punctuation mark -- add leading space */
	    ((wchar_t *)buf)[0] = ' ';
	    textsw_get_selection_as_string(folio, EV_SEL_PRIMARY, 
                                              (wchar_t *)buf + 1, 1024);
	    textsw_insert_wcs(textsw, (wchar_t *)buf, stringLength(buf));
	    /* reset selection to original span */
	    textsw_SetSelection(textsw, position + 1,
		  position + stringLength(buf), EV_SEL_PRIMARY);
	    return;
        } else {
	    /* don't add any spaces */
	    textsw_get_selection_as_string(folio, EV_SEL_PRIMARY, buf, 1024);
	    textsw_insert_wcs(textsw, (wchar_t *)buf, stringLength(buf));
	    /* reset selection to original span */
	    textsw_SetSelection(textsw, position,
				  position + stringLength(buf), EV_SEL_PRIMARY);
	    return;
        }

    }
}

Pkg_private int
textsw_clean_up_move(view, first, last_plus_one)
    Textsw_view_handle view;
    Es_index        first, last_plus_one;
{
    Textsw          textsw = VIEW_REP_TO_ABS(view);
    int             shift_left;

    /* if spaces on either side, collapse them */

    if (!multibyte) {

        char first_buf[1], last_buf[1];
        getTextswContents(textsw, first - 1, first_buf, 1);
        if (first_buf[0] == ' ') {
	    getTextswContents(textsw, last_plus_one, last_buf, 1);
	    if (last_buf[0] == ' ')
	        shift_left = TRUE;
        }
        getTextswContents(textsw, last_plus_one, last_buf, 1);
        /* if punctuation to the right and space to the left, delete space */
        if (last_buf[0] == '.' || last_buf[0] == ',' || last_buf[0] == ';'
	    || last_buf[0] == ':') {
	    if (first_buf[0] == ' ')
	        shift_left = TRUE;
        }
        if (shift_left)
	    textsw_delete(textsw, first - 1, first);
        return (shift_left);

    } else { 
       
        wchar_t first_buf[1], last_buf[1];
        getTextswContents(textsw, first - 1, first_buf, 1);
        if (first_buf[0] == ' ') {
	    getTextswContents(textsw, last_plus_one, last_buf, 1);
	    if (last_buf[0] == ' ')
	        shift_left = TRUE;
        }
        getTextswContents(textsw, last_plus_one, last_buf, 1);
        /* if punctuation to the right and space to the left, delete space */
        if (last_buf[0] == '.' || last_buf[0] == ',' || last_buf[0] == ';'
	    || last_buf[0] == ':') {
	    if (first_buf[0] == ' ')
	        shift_left = TRUE;
        }
        if (shift_left)
	    textsw_delete(textsw, first - 1, first);
        return (shift_left);
    }
}

static void
display_notice(public_view, dnd_status)
    Xv_opaque	 public_view;
    int		 dnd_status;
{
    char 	*error_msg;
    Xv_Notice	 notice;

    switch (dnd_status) {
      case XV_OK:
        return;
      case DND_TIMEOUT:
        error_msg = XV_MSG("Operation timed out");
        break;
      case DND_ILLEGAL_TARGET:
        error_msg = XV_MSG("Illegal drop target");
        break;
      case DND_SELECTION:
        error_msg = XV_MSG("Unable to acquire selection");
        break;
      case DND_ROOT:
        error_msg = XV_MSG("Root window is not a valid drop target");
        break;
      case XV_ERROR:
        error_msg = XV_MSG("Unexpected internal error");
        break;
    }
    notice = xv_create((Frame)xv_get(public_view, WIN_FRAME), NOTICE,
                    NOTICE_MESSAGE_STRINGS,
                        XV_MSG("Drag and Drop failed:"),
                        error_msg,
                        0,
                    XV_SHOW, TRUE,
                    0);
    xv_destroy(notice);
}

#define MAX_CHARS_SHOWN	 5	/* most chars shown in the drag move cursor */
#define SELECTION_BUF_SIZE	MAX_CHARS_SHOWN + 2

Pkg_private void
textsw_do_drag_copy_move(view, ie, is_copy)
    Textsw_view_handle 	 view;
    Event          	*ie;
    int             	 is_copy;
{
    Xv_opaque       	 public_view = VIEW_REP_TO_ABS(view);
    Textsw_folio 	 folio = FOLIO_FOR_VIEW(view);
    Xv_Cursor       	 dnd_cursor,
			 dnd_accept_cursor;
    wchar_t            	 buf[SELECTION_BUF_SIZE];
    short           	 l;
    Dnd			 dnd;
    int			 DndConvertProc(),
			 dnd_status;
    void 		*buffer;
    char 		*buffer_mb;
    Es_index  		 first, last_plus_one;

    l = textsw_get_selection_as_string(folio, EV_SEL_PRIMARY, buf,
			MAX_CHARS_SHOWN + 1);

    dnd_cursor = xv_create(public_view, CURSOR,
			   ((multibyte) ? CURSOR_STRING_WCS : CURSOR_STRING),
			   buf,
			CURSOR_DRAG_TYPE,
			     	(is_copy ? CURSOR_DUPLICATE : CURSOR_MOVE),
			NULL);

    dnd_accept_cursor = xv_create(public_view, CURSOR,
			   ((multibyte) ? CURSOR_STRING_WCS : CURSOR_STRING),
			   buf,
			CURSOR_DRAG_TYPE,
				     (is_copy ? CURSOR_DUPLICATE : CURSOR_MOVE),
			CURSOR_DRAG_STATE,	CURSOR_ACCEPT,
			NULL);

    dnd = xv_create(public_view, DRAGDROP,
			DND_TYPE, 		(is_copy ? DND_COPY : DND_MOVE),
			DND_CURSOR, 		dnd_cursor,
			DND_ACCEPT_CURSOR,	dnd_accept_cursor,
			SEL_CONVERT_PROC,	DndConvertProc,
			0);

    (void)ev_get_selection(folio->views, &first, &last_plus_one,EV_SEL_PRIMARY);
    if (multibyte)
        buffer = (wchar_t *)xv_malloc((last_plus_one-first+1)*sizeof(wchar_t));
    else
        buffer = (char *)xv_malloc(last_plus_one - first + 1);

    (void)textsw_get_selection_as_string(folio, EV_SEL_PRIMARY, buffer,
					 last_plus_one - first + 1);
					 
    if (!dnd_data_key)
        dnd_data_key = xv_unique_key();
    if (!dnd_view_key)
        dnd_view_key = xv_unique_key();

    if (multibyte) {
        buffer_mb = _xv_wcstombsdup(buffer);
        xv_set(dnd, XV_KEY_DATA, dnd_data_key, buffer_mb, NULL);
        if (buffer)
	    free((char *)buffer);
    } else 
        xv_set(dnd, XV_KEY_DATA, dnd_data_key, buffer, NULL);

    xv_set(dnd, XV_KEY_DATA, dnd_view_key, view, NULL);

    if ((dnd_status = dnd_send_drop(dnd)) != XV_OK) {
       if (dnd_status != DND_ABORTED)
           display_notice(public_view, dnd_status);
       xv_destroy(dnd);
    }

    xv_destroy(dnd_cursor);
    xv_destroy(dnd_accept_cursor);
}

static int
DndConvertProc(dnd, type, data, length, format)
    Dnd      	 dnd;
    Atom        *type;
    Xv_opaque	*data;
    unsigned long  *length;
    int         *format;
{
    Xv_Server server = XV_SERVER_FROM_WINDOW(xv_get((Xv_opaque)dnd, XV_OWNER));
    Textsw_view_handle 	 view = (Textsw_view_handle)xv_get(dnd, XV_KEY_DATA,
							   dnd_view_key);
    Textsw_folio        folio = FOLIO_FOR_VIEW(view);

    if (*type == (Atom)xv_get(server, SERVER_ATOM, "_SUN_DRAGDROP_DONE")) {
	xv_set(dnd, SEL_OWN, False, 0);
	xv_free((char *)xv_get(dnd, XV_KEY_DATA, dnd_data_key)); 
	xv_destroy_safe(dnd);
	*format = 32;
	*length = 0;
	*data = NULL;
        *type = (Atom)xv_get(server, SERVER_ATOM, "NULL");
        return(True);
    } else if (*type == (Atom)xv_get(server, SERVER_ATOM, "DELETE")) {
                        /* Destination asked us to delete the selection.
                         * If it is appropriate to do so, we should.
                         */
        Es_index  first, last_plus_one, ro_bound;

	view = (Textsw_view_handle)xv_get(dnd, XV_KEY_DATA, dnd_view_key);
	folio = FOLIO_FOR_VIEW(view);
        (void)ev_get_selection(folio->views, &first, &last_plus_one,
			       EV_SEL_PRIMARY);
	if (!TXTSW_IS_READ_ONLY(folio)) {
	  ro_bound = textsw_read_only_boundary_is_at(folio);
          textsw_take_down_caret(folio);
	  if (folio->state & TXTSW_DELETE_REPLACES_CLIPBOARD) {
	    textsw_delete_span(view, (first < ro_bound) ? ro_bound : first,
			       last_plus_one, TXTSW_DS_SHELVE);
	  } else {
	    textsw_delete_span(view, (first < ro_bound) ? ro_bound : first,
					       last_plus_one, NULL);
	  }
          textsw_show_caret(folio);
	}
        *format = 32;
        *length = 0;
        *data = NULL;
        *type = (Atom)xv_get(server, SERVER_ATOM, "NULL");
        return(True);
    } else if (*type == (Atom)xv_get(server, SERVER_ATOM,
						   "_SUN_SELN_IS_READONLY")) {
	static int results;
	if (TXTSW_IS_READ_ONLY (folio) )
		results = TRUE;
	else
		results = FALSE;
	*format = 32;
	*length = 1;
	*data = (Xv_opaque)&results;
	*type = XA_INTEGER;
	return(True);
    } else if (*type == XA_STRING || *type == (Atom)xv_get(server, SERVER_ATOM,
								    "TEXT")) {
	char *buf = (char *)xv_get(dnd, XV_KEY_DATA, dnd_data_key);
	*format = 8;
	*length = strlen(buf);
	*data = (Xv_opaque)buf;
	*type = XA_STRING;
	return(True);
    } else if (*type == (Atom)xv_get(server, SERVER_ATOM,"COMPOUND_TEXT")) {
	XTextProperty text_prop;
	int state;
	char *buf = (char *)xv_get(dnd, XV_KEY_DATA, dnd_data_key);
	state = XmbTextListToTextProperty((Display *)xv_get(server,XV_DISPLAY),
					  &buf,
					  1,
					  XCompoundTextStyle,
					  &text_prop);
	if (state<0) {
	    return(False);
	}
	*format = 8;
	*data = (Xv_opaque) text_prop.value;
	*length = strlen((char *)text_prop.value);
	return(True);
    } else if (*type == (Atom)xv_get(server, SERVER_ATOM, "TARGETS")) {
	static Atom atom_list[8];
	int i;

	i=0;
	atom_list[i++] = (Atom)xv_get(server, SERVER_ATOM, "_SUN_DRAGDROP_DONE");
	atom_list[i++] = (Atom)xv_get(server, SERVER_ATOM, "DELETE");
	atom_list[i++] = (Atom)xv_get(server, SERVER_ATOM,
						"_SUN_SELN_IS_READONLY");
	atom_list[i++] = XA_STRING;
	atom_list[i++] = (Atom)xv_get(server, SERVER_ATOM, "TEXT");
	atom_list[i++] = (Atom)xv_get(server, SERVER_ATOM, "TARGETS");
	atom_list[i++] = (Atom)xv_get(server, SERVER_ATOM, "TIMESTAMP");
	/* only understand compound text if we are in a multibyte locale.
	   This is to be consistent with the copy/paste in txt_selsvc.c */
	if (multibyte) {
	    atom_list[i++] = (Atom)xv_get(server, 
					  SERVER_ATOM, "COMPOUND_TEXT");
	}
	*format = 32;
	*length = i;
	*data = (Xv_opaque)atom_list;
	*type = XA_ATOM;
	return(True);
    }
    return(sel_convert_proc(dnd, type, data, length, format));
}

/*
 * When a textsw gets a ACTION_DRAG_MOVE event, this routines gets called to
 * get the primary selection from another process and do move/copy
 * ACTION_DRAG_MOVE is a result of XSendEvent called by the subwindow that
 * originally sees the button-down that starts the drag move
 */
Pkg_private
textsw_do_remote_drag_copy_move(view, ie, is_copy)
    register Textsw_view_handle  view;
    Event          		*ie;
    short           		 is_copy;
{
    Selection_requestor 	 sel;
    register Textsw_folio 	 folio = FOLIO_FOR_VIEW(view);
    char           		*string = NULL;
    wchar_t           		*string_wc = NULL;
    unsigned long		 length;
    int				 format,
    				*is_read_only;
    Es_index        		 ro_bdry,
				 pos,
				 index,
				 temp;
    void	    		 DndReplyProc();
    struct textsw_context  	 context;
    Atom                        *target_list;
    Xv_Server                    server;

    is_read_only = NULL;
    /*
     * First, process insertion point .
     */
    ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, TRUE, 0);
    ro_bdry = textsw_read_only_boundary_is_at(folio);
    pos = ev_resolve_xy(view->e_view,
			event_x(ie), event_y(ie));

    if (pos < ro_bdry) {
	Es_index	insert;

	insert = EV_GET_INSERT(folio->views);
	if (insert >= ro_bdry)
	    pos = insert;
	else
	    return;
    }

    if (!dnd_data_key)
        dnd_data_key = xv_unique_key();

    sel = xv_create(VIEW_REP_TO_ABS(view), SELECTION_REQUESTOR,
			  SEL_REPLY_PROC,	DndReplyProc,
			  SEL_TYPE_NAME,	"_SUN_SELN_IS_READONLY",
			  NULL);
    if (dnd_decode_drop(sel, ie) == XV_ERROR) {
        xv_destroy(sel);
	return;
    }
	/* If the source and the dest is the same process, see if the
	 * drop happened within the primary selection, in which case we
	 * don't do anything.
	 */

    /* make sure drop doesn't happen in read-only text */
    if (TXTSW_IS_READ_ONLY(folio)) {
        dnd_done(sel);
	xv_destroy(sel);
	textsw_read_only_msg(folio,event_x(ie), event_y(ie));
	return;
    }

    if (dnd_is_local(ie)) {
        Es_index 	first,
			last_plus_one;
	
	(void)ev_get_selection(folio->views, &first, &last_plus_one,
			       EV_SEL_PRIMARY);
	pos = ev_resolve_xy(view->e_view, event_x(ie), event_y(ie));
	/* make sure drop doesn't happen in read-only part of cmdtool */
	if (pos < ro_bdry) {
	  Es_index	insert;
	  
	  insert = EV_GET_INSERT(folio->views);
	  if (insert >= ro_bdry)
	    pos = insert;
	  else
	    pos = ro_bdry + 1;
	}

	if (pos >= first && pos < last_plus_one) {
	    dnd_done(sel);
    	    ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, FALSE, 0);
	    return;
	}
    }
	/* If this is a move operation, see if the source is read only. */
    if (!is_copy) {
        is_read_only = (int *)xv_get(sel, SEL_DATA, &length, &format);
        if (length == SEL_ERROR) {
	    /* don't know if is_read_only got set, so set it back to NULL */
	    is_read_only = NULL;
	    is_copy = True;
	}
    }

    /* Ask for the selection to be converted into compound text if 
       available from TARGETS or string by default.
       Note: DndReplyProc() doesn't handle TARGETS so we handle it here.  
       However, it does handle COMPOUND_TEXT and STRING so we don't
       handle those targets here. */

    server = XV_SERVER_FROM_WINDOW(VIEW_REP_TO_ABS(view));
    xv_set(sel, 
	   SEL_TYPE,(Atom)xv_get(server,SERVER_ATOM,"TARGETS"),
	   NULL);
    target_list = (Atom *)xv_get(sel, SEL_DATA, &length, &format);
    xv_set(sel, SEL_TYPE, XA_STRING, 0);
    if (length!=SEL_ERROR && target_list) {
	int i;
	Atom ctext;
	ctext = (Atom)xv_get(server, SERVER_ATOM, "COMPOUND_TEXT");
	for (i=0; i<length; i++) {
	    if (target_list[i]==ctext) {
		xv_set(sel, SEL_TYPE,ctext, NULL);
		break;
	    }
	}
	xv_free(target_list);
    }
    xv_set(sel,XV_KEY_DATA,dnd_data_key,NULL,NULL);
    string = (char *)xv_get(sel, SEL_DATA, &length, &format);
    if (length == SEL_ERROR) {
	if (string)
	    xv_free(string);
	if (is_read_only)
	    xv_free(is_read_only);
	dnd_done(sel);
	xv_destroy(sel);
	return;
    }

    /* the transfer could have not converted any characters and so
       no text will be inserted so don't do anything. */
    string = (char *)xv_get(sel, XV_KEY_DATA, dnd_data_key);
    if (!string) {
	if (is_read_only)
	    xv_free(is_read_only);
	dnd_done(sel);
	xv_destroy(sel);
	return;
    }
    
    ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, FALSE, 0);
    EV_SET_INSERT(folio->views, pos, temp);

    if (multibyte) {
        string_wc = _xv_mbstowcsdup(string);
        index = textsw_do_input(view, string_wc, 
              (long int)stringLength(string_wc), TXTSW_UPDATE_SCROLLBAR);
        if (string_wc)
	    free((char *)string_wc);
    } else
        index = textsw_do_input(view, (char *)string, (long int)strlen(string),
			    TXTSW_UPDATE_SCROLLBAR);

    /* If this is a move operation and we were able to insert the text,
     * ask the source to delete the selection.*/
    if (!is_copy && !*is_read_only && index) {
	xv_set(sel, SEL_TYPE_NAME, "DELETE", 0);
	(void)xv_get(sel, SEL_DATA, &length, &format);
    }

    free((char *) string);
    if (is_read_only)
	xv_free(is_read_only);
    dnd_done(sel);
    xv_destroy(sel);
    TEXTSW_DO_INSERT_MAKES_VISIBLE(view);
}

static char*
DndHandleCompoundText(owner,server, target, value, length, format)
     Xv_opaque       owner;
     Xv_Server       server;
     Atom            target;
     Xv_opaque       value;
     int            *length;
     int             format;
{
    char *real_value;
    char **list;
    int count;
    int state;
    XTextProperty text_prop;
    /* if length==0 then probably end of INCR selection so
       don't convert anything. */
    if (*length) {
	text_prop.value = (unsigned char *) value;
	text_prop.encoding = target;
	text_prop.format = format;
	text_prop.nitems = *length;
	state = XmbTextPropertyToTextList(XV_DISPLAY_FROM_WINDOW(owner),
					  &text_prop,
					  &list,
					  &count);
	if ((state>=0) && list && list[0]) {
	    int len = strlen(list[0]);
	    real_value = (char *)xv_malloc(len*sizeof(char)+1);
	    XV_BCOPY(list[0],real_value,len);
	    real_value[len]=NULL;
	    *length = len;
	    XFreeStringList(list);
	    xv_free(value); /* since we made a copy, we need to free it */
	} else {
	    return (char *)value;
	}
    } else {
	real_value = NULL;
    }
    return (char *)real_value;
}

void
DndReplyProc(sel, target, type, value, length, format)
    Selection_requestor sel;
    Atom            target;
    Atom            type;
    Xv_opaque       value;
    int             length;
    int             format;
{
    Xv_opaque	 owner = xv_get(sel, XV_OWNER);	
    Xv_Server	 server = XV_SERVER_FROM_WINDOW(owner);
    static int	 incr = False,
		 str_size = 0;
    static char *string;
    char        *real_value;

    /* this reply proc only handles XA_STRING or a one item COMPOUND_TEXT. */

    if (length == SEL_ERROR) {
        return;
    }

    if ((target!=XA_STRING)&&
	(target!=(Atom)xv_get(server,SERVER_ATOM,"COMPOUND_TEXT"))) {
	return;
    }
    if (type == (Atom)xv_get(server, SERVER_ATOM, "INCR")) {
	incr = True;
    } else if (!incr) {
	char *real_value;
	if (target == XA_STRING) {
	    real_value = (char *)value;
	} else {
	    real_value = DndHandleCompoundText(owner,server,target,
					       value,&length,format);
	}
	xv_set(sel, XV_KEY_DATA, dnd_data_key, real_value, NULL);
	str_size = 0;
    } else if (length) {
	if (!str_size)
	    string = (char *)xv_malloc(length+1);
	else
	    string = (char *)xv_realloc(string, str_size + length + 1);
	XV_BCOPY((char *)value, string + str_size, length);
	xv_free(value); /* need to free the reply value since we made a 
			 copy of it in string */
	str_size += length;
    } else {
	char *real_value;
	if (str_size && incr) {
	    string[str_size] = NULL;
	}
	if (target == XA_STRING) {
	    real_value = (char *)string;
	} else {
	    real_value = DndHandleCompoundText(owner,server,target,
					       string,&str_size,format);
	}
	xv_set(sel, XV_KEY_DATA, dnd_data_key, real_value, NULL);
	incr = str_size = 0;
    }
}
