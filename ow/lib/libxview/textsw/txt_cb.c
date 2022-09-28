#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)txt_cb.c 50.51 93/08/31";
#endif
#endif

/*
 *	(c) Copyright 1990 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */
 
#ifdef OW_I18N

#include <xview_private/primal.h>
#include <xview_private/txt_18impl.h>
#include <xview_private/txt_impl.h>
#include <xview_private/ev_impl.h>
#include <xview/textsw.h>

Xv_private void	textsw_preedit_start();
Xv_private void	textsw_preedit_draw();
Xv_private void	textsw_preedit_done();
static void textsw_update_region();
extern void textsw_SetSelection();

#define	WC_BUF_SIZE	256


Xv_private	void
textsw_preedit_start(ic, client_data, callback_data)
    XIC			ic;
    XPointer		client_data;
    XPointer		callback_data;
{
	/* This function dose nothing any longer */
}


Xv_private	void
textsw_preedit_done(ic, client_data, callback_data)
    XIC			ic;
    XPointer		client_data;
    XPointer		callback_data;
{
    Textsw		     textsw = (Textsw)client_data;
    register Textsw_folio    folio = TEXTSW_PRIVATE(textsw);

    /*
     *	Following case is that the preedit text still remains as IM-Server
     *  might die or exit.
     */
    if (folio->preedit_start) {
	XIMPreeditDrawCallbackStruct	callback_data;

	callback_data.chg_first = 0;
	callback_data.chg_length = EV_GET_INSERT(folio->views) -
			textsw_find_mark_internal(folio, folio->preedit_start);
	callback_data.text = (XIMText *)0;
	textsw_preedit_draw(folio->ic, (XPointer)FOLIO_REP_TO_ABS(folio),
			     &callback_data);
    }
}


Xv_private	void
textsw_preedit_draw(ic, client_data, callback_data)
    XIC			ic;
    XPointer		client_data;
    XIMPreeditDrawCallbackStruct	*callback_data;
{
    Textsw		    textsw = (Textsw)client_data;
    register Textsw_folio   folio = TEXTSW_PRIVATE(textsw);
    Textsw_view_handle	    view = VIEW_FROM_FOLIO_OR_VIEW(folio);
    Es_index		    first, last_plus_one;

    if (!folio->preedit_start) { /* preedit text dosen't exist. */
	if (callback_data->text) { /* First time to draw preedit text. */

	    if (callback_data->chg_first || callback_data->chg_length) {
		/*
		 * Both chg_first and chg_length should be NULL.
		 * So, abandon this wrong data.
		 */
		return;
	    }
	    (void)ev_get_selection(folio->views, &first,
				   &last_plus_one, EV_SEL_PRIMARY);
	    if (first != last_plus_one) { /* primary selection exists */
		int	ro_point;

		ro_point = textsw_read_only_boundary_is_at(folio);
		if (!ro_point || first >= ro_point) {
		    textsw_take_down_caret(folio);
		    textsw_delete_span(view, first, last_plus_one,
				       TXTSW_DS_ADJUST);
		    first = EV_GET_INSERT(folio->views);
		}
		else {
		    textsw_set_selection(textsw, ES_INFINITY, ES_INFINITY,
					 EV_SEL_PRIMARY);
		    if (ro_point < last_plus_one) {
			textsw_take_down_caret(folio);
			textsw_delete_span(view, ro_point, last_plus_one,
					   TXTSW_DS_ADJUST);
			first = ro_point;
		    }
		    else
			first = EV_GET_INSERT(folio->views);
		}
	    }
	    else
		first = EV_GET_INSERT(folio->views);

	    folio->preedit_start = textsw_add_mark_internal(folio, first, NULL);

	    /* Turn off undo and again before inserting preedit text */
	   folio->state |= (TXTSW_NO_UNDO_RECORDING | TXTSW_NO_AGAIN_RECORDING);
	}
	else {	/* no preedit text */
	    textsw_take_down_caret(folio); /* This is for 1075306 */
	    return;
	}
    }

    first = textsw_find_mark_internal(folio, folio->preedit_start) +
	    callback_data->chg_first;
    last_plus_one = first + callback_data->chg_length;

    if (callback_data->chg_first == 0)
	ev_remove_all_op_bdry(folio->views, first, last_plus_one,
			EV_SEL_PRIMARY | EV_SEL_SECONDARY, EV_BDRY_TYPE_ONLY);
    /*
     *  NULL text in callback_data implictly means erase the pre-edit text
     *  for commiting, or erase-character is coming.
     */
    if (callback_data->text) {
	if ((callback_data->text->string.wide_char) ||
	    (callback_data->text->string.multi_byte)) {

	    if (callback_data->text->encoding_is_wchar)
		textsw_replace(textsw, first, last_plus_one,
			       callback_data->text->string.wide_char,
			       callback_data->text->length);
	    else {
		int		len = callback_data->text->length;
		static wchar_t	wc_buffer[WC_BUF_SIZE];
		wchar_t	       *wbuf;

		wbuf = (len > WC_BUF_SIZE) ? MALLOC(len) : wc_buffer;
		mbstowcs(wbuf, callback_data->text->string.multi_byte, len);
		textsw_replace(textsw, first, last_plus_one, wbuf, len);
		if (len > WC_BUF_SIZE)
		    free((char *)wbuf);
	    }
        }
	textsw_update_region(textsw, callback_data, FALSE);
    }
    else {
	if (first == last_plus_one) {
	    /*
	     * This callback data is wrong as erasing preedit text. Right one
	     * will come next. folio->preedit_start have to remain.
	     */
	    return;
	}
	textsw_take_down_caret(folio);
	textsw_delete_span(view, first, last_plus_one, TXTSW_DS_ADJUST);

	/* 0 means erasing the entire preedit text for commiting. */
	if (callback_data->chg_first == 0) {
	    textsw_remove_mark_internal(folio, folio->preedit_start);
	    EV_INIT_MARK(folio->preedit_start);

	    /* Resume undo and again for commited text */
	    folio->state &= ~(TXTSW_NO_UNDO_RECORDING | TXTSW_NO_AGAIN_RECORDING);
	    /* This is a workaround for 1090046. */
	    if (folio->blocking_newline) {
		textsw_do_newline(VIEW_FROM_FOLIO_OR_VIEW(folio), '\n');
		folio->blocking_newline = FALSE;
	    }
	}
    }
}


static void
textsw_update_region(textsw, preedit_data, reset_region)
    Textsw        	    textsw;
    XIMPreeditDrawCallbackStruct	*preedit_data;
    int			    reset_region;
{
    register Textsw_folio   folio = TEXTSW_PRIVATE(textsw);
    register unsigned 	    i;

    if (folio->preedit_start) {
	Textsw_index 	    first, last_plus_one, region_start;

	first = region_start = textsw_find_mark_internal(folio,
			       folio->preedit_start) + preedit_data->chg_first;
	if (reset_region) {
	    ev_remove_all_op_bdry(folio->views, first,
				  first + preedit_data->chg_length,
				  EV_SEL_PRIMARY | EV_SEL_SECONDARY,
				  EV_BDRY_TYPE_ONLY);
	}

	if (!preedit_data->text->feedback)
	    return;

	for (i = 0; i < preedit_data->text->length; i++) {
	    if ((i == (preedit_data->text->length - 1)) ||
		(preedit_data->text->feedback[i] !=
					preedit_data->text->feedback[i+1])) {
		last_plus_one = region_start + i + 1;
		if (preedit_data->text->feedback[i] == XIMReverse)
		    ev_set_preedit_region(folio->views,
				       first, last_plus_one, EV_SEL_PRIMARY);
		else if (preedit_data->text->feedback[i] == XIMUnderline)
		    ev_set_preedit_region(folio->views,
				       first, last_plus_one, EV_SEL_SECONDARY);
		first = last_plus_one;
	    }
	}
	ev_display_range(folio->views, region_start, last_plus_one);
    }
}

#endif /* OW_I18N */
