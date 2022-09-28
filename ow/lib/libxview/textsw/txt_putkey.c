#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)txt_putkey.c 20.21 93/11/05";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * PUT key processing.
 */

#include <xview_private/primal.h>
#include <xview_private/txt_impl.h>
#include <xview_private/ev_impl.h>
#include <errno.h>


extern void textsw_SetSelection();

Pkg_private Ev_finger_handle ev_add_finger();
Pkg_private int      ev_get_selection();
Pkg_private Es_handle textsw_esh_for_span();
Pkg_private Seln_rank textsw_acquire_seln();
Pkg_private caddr_t  textsw_checkpoint_undo();

Pkg_private int
textsw_begin_put(view, inform_svc)
    Textsw_view_handle view;
    int             inform_svc;
{
    register Textsw_folio textsw = FOLIO_FOR_VIEW(view);

    textsw_begin_function(view, TXTSW_FUNC_PUT);
    ASSUME(EV_MARK_IS_NULL(&textsw->save_insert));
    EV_MARK_SET_MOVE_AT_INSERT(textsw->save_insert);
    (void) ev_add_finger(&textsw->views->fingers,
			 EV_GET_INSERT(textsw->views),
			 (opaque_t) 0, &textsw->save_insert);
    if (inform_svc)
	(void) textsw_inform_seln_svc(textsw, TXTSW_FUNC_PUT, TRUE);
}

Pkg_private int
textsw_end_put(view)
    register Textsw_view_handle view;
{
    Es_index        old_insert;
    int             easy;
    int             result = 0;
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);

    easy = textsw_inform_seln_svc(folio, TXTSW_FUNC_PUT, FALSE);
    if ((folio->func_state & TXTSW_FUNC_PUT) == 0)
	return (0);
    if ((folio->func_state & TXTSW_FUNC_EXECUTE) == 0)
	goto Done;
    (void) textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
				  (caddr_t) TEXTSW_INFINITY - 1);
    result = textsw_do_put(view, easy);
    (void) textsw_checkpoint_undo(VIEW_REP_TO_ABS(view),
				  (caddr_t) TEXTSW_INFINITY - 1);
Done:
    old_insert = textsw_get_saved_insert(folio);
    if AN_ERROR
	(old_insert == ES_INFINITY) {
    } else {
	(void) textsw_set_insert(folio, old_insert);
	ev_remove_finger(&folio->views->fingers, folio->save_insert);
	EV_INIT_MARK(folio->save_insert);
    }
    textsw_end_function(view, TXTSW_FUNC_PUT);
    return (result);
}

static int
textsw_do_put(view, local_operands)
    register Textsw_view_handle view;
    int             local_operands;
{
    /*
     * The following table indicates what the final contents of the trashbin
     * should be based on the modes of the primary and secondary selections.
     * P-d is short for pending-delete. An empty primary selection is treated
     * as not pending-delete.
     * 
     * Primary ~P-d	 P-d Secondary	================= Empty		|
     * Pri.	| Pri.	| ~P-d		| Tbin	| Tbin	| P-d		|
     * Sec.	| Sec.	| =================
     */
    register Textsw_folio textsw = FOLIO_FOR_VIEW(view);
    register Ev_chain views = textsw->views;
    register Es_handle primary = ES_NULL;
    register int    is_pending_delete;
    Es_index        delta, first, last_plus_one, sec_insert;
    Es_index        ro_bdry;
    int             result = 0;

    /*
     * First, pre-process the primary selection.
     */
    ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, TRUE, 0);
    ro_bdry = textsw_read_only_boundary_is_at(textsw);
    if (local_operands) {
	(void) ev_get_selection(views, &first, &last_plus_one,
				EV_SEL_PRIMARY);
	if (first < last_plus_one) {
	    primary =
		textsw_esh_for_span(view, first, last_plus_one, ES_NULL);
	}
    }
    /*
     * Second, completely process the secondary selection.
     */
    is_pending_delete = (EV_SEL_PENDING_DELETE &
			 ev_get_selection(views, &first, &last_plus_one,
					  EV_SEL_SECONDARY));
    switch (textsw_adjust_delete_span(textsw,
				      &first, &last_plus_one)) {
      case TXTSW_PE_EMPTY_INTERVAL:
	sec_insert = ES_INFINITY;
	break;
      case TEXTSW_PE_READ_ONLY:
	/*
	 * If part of span was readonly, adjust_delete_span moves first to
	 * ro_bdry and returns TXTSW_PE_ADJUSTED. But if the whole span is
	 * readonly, adjust_delete_span doesn't adjust even when the span
	 * borders on ro_bdry.
	 */
	if (last_plus_one == ro_bdry) {
	    first = ro_bdry;
	    is_pending_delete = 0;
	} else {
	    result = TEXTSW_PE_READ_ONLY;
	    if (is_pending_delete) {
		sec_insert = ro_bdry - 1;
		break;
	    }
	}
	/* else fall through */
      default:
	sec_insert = EV_GET_INSERT(views);
	if ((sec_insert != first) && (sec_insert != last_plus_one))
	    sec_insert = ro_bdry - 1;
	if (is_pending_delete) {
	    delta = textsw_delete_span(view, first, last_plus_one,
				       TXTSW_DS_SHELVE);
	    if (first < sec_insert) {
		sec_insert += delta;
	    }
	}
    }
    if (first != ES_INFINITY) {
	textsw_SetSelection(VIEW_REP_TO_ABS(view),
			     ES_INFINITY, ES_INFINITY,
			     EV_SEL_SECONDARY);
    }
    /*
     * Third, post-process the primary selection.
     */
    if (local_operands) {
	is_pending_delete = (EV_SEL_PENDING_DELETE & ev_get_selection(
			    views, &first, &last_plus_one, EV_SEL_PRIMARY));
	if (is_pending_delete) {
	    switch (textsw_adjust_delete_span(textsw,
					      &first, &last_plus_one)) {
	      case TXTSW_PE_EMPTY_INTERVAL:
		break;
	      case TEXTSW_PE_READ_ONLY:
		result = TEXTSW_PE_READ_ONLY;
		break;
	      default:
		/*
		 * We still have a non-empty primary selection (it could have
		 * been deleted because of the secondary selection but was
		 * not).
		 */
		if (sec_insert != ES_INFINITY) {
		    (void) ev_delete_span(textsw, first, last_plus_one,
					  &delta, 0);
		    if (sec_insert >= first)
			sec_insert = (sec_insert < last_plus_one)
			    ? first : sec_insert + delta;
		}
	    }
	}
	if ((first != ES_INFINITY) && (sec_insert != ES_INFINITY)) {
	    textsw_SetSelection(VIEW_REP_TO_ABS(view),
				 ES_INFINITY, ES_INFINITY, EV_SEL_PRIMARY);
	}
    }
    ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, FALSE, 0);
    if (sec_insert == ES_INFINITY) {
	Es_handle       temp = textsw->trash;
	textsw->trash = primary;
	primary = temp;		/* Will free former trash below */
	textsw_acquire_seln(textsw, SELN_SHELF);
    } else if TXTSW_IS_READ_ONLY
	(textsw) {
	result = TEXTSW_PE_READ_ONLY;
    } else if (ro_bdry <= sec_insert) {
	/*
	 * Fourth, insert the text being gotten.
	 */
	if (local_operands) {
	    last_plus_one = textsw_insert_pieces(view, sec_insert,
						 primary);
	} else {
	    long unsigned   save_func_state = textsw->func_state;
	    /* Don't record this insert */
	    textsw->func_state |= TXTSW_FUNC_AGAIN;
	    /*
	     * textsw_stuff_selection does so at current insert, so we have
	     * to set it.  The old insert need not be restored because
	     * textsw_end_put always does it anyway.
	     */
	    (void) textsw_set_insert(textsw, sec_insert);
	    textsw_stuff_selection(view, EV_SEL_PRIMARY);
	    if ((save_func_state & TXTSW_FUNC_AGAIN) == 0)
		textsw->func_state &= ~TXTSW_FUNC_AGAIN;
	}
    }
    if (primary)
	es_destroy(primary);
    return (result);
}

Pkg_private int
textsw_put(view)
    Textsw_view_handle view;
{
    textsw_begin_put(view, TRUE);
    (void) textsw_end_put(view);
}
