#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)txt_field.c 20.32 93/07/26";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Procedures to do field matching in text subwindows.
 */

#include <string.h>
#include <xview_private/primal.h>
#include <xview_private/txt_impl.h>
#include <xview_private/ev_impl.h>
#include <xview_private/txt_18impl.h>
#include <sys/time.h>
#include <xview/panel.h>
#include <xview/wmgr.h>
#include <xview/frame.h>
#include <xview/win_struct.h>
#include <xview/win_screen.h>
#define    ONE_FIELD_LENGTH     2
#define    MAX_SYMBOLS		8
#define    NUM_OF_COL		2
#define    MAX_STR_LENGTH       1024

static void            textsw_get_match_symbol();
extern void textsw_SetSelection();

static wchar_t l_curly_brace[] = { '{', 0 };
static wchar_t l_paren[] = { '(', 0 };
static wchar_t dbl_quote[] = { '"', 0 };
static wchar_t sgl_quote[] = { '\'', 0 };
static wchar_t accent_grave[] = { '`', 0 };
static wchar_t l_square_brace[] = { '[', 0 };
static wchar_t bar_gt[] = { '|', '>', 0 };
static wchar_t open_comment[] = { '/', '*', 0 };

static wchar_t r_curly_brace[] = { '}', 0 };
static wchar_t r_paren[] = { ')', 0 };
static wchar_t r_square_brace[] = { ']', 0 };
static wchar_t lt_bar[] = { '<', '|', 0 };
static wchar_t close_comment[] = { '*', '/', 0 };

static wchar_t     *match_table_wc[NUM_OF_COL][MAX_SYMBOLS] =
{{ l_curly_brace, l_paren, dbl_quote, sgl_quote, accent_grave, l_square_brace,
   bar_gt, open_comment,},
 { r_curly_brace, r_paren, dbl_quote, sgl_quote, accent_grave, r_square_brace,
   lt_bar, close_comment,}};

static char     *match_table[NUM_OF_COL][MAX_SYMBOLS] =
{{"{", "(", "\"", "'", "`", "[", "|>", "/*",},
{"}", ")", "\"", "'", "`", "]", "<|", "*/",}};

#define TMP_BUFSIZE 256

Pkg_private void
textsw_begin_match_field(view)
    register Textsw_view_handle view;
{
    textsw_begin_function(view, TXTSW_FUNC_FIELD);
    (void) textsw_inform_seln_svc(FOLIO_FOR_VIEW(view),
				  TXTSW_FUNC_FIELD, TRUE);
}

Pkg_private int
textsw_end_match_field(view, event_code, x, y)
    register Textsw_view_handle view;
    int             x, y;
{
    Pkg_private int textsw_match_selection_and_normalize();
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    unsigned        field_flag = (event_code == TXTSW_NEXT_FIELD) ?
                              TEXTSW_FIELD_FORWARD : TEXTSW_FIELD_BACKWARD;
    void           *start_marker;

    (void) textsw_inform_seln_svc(folio, TXTSW_FUNC_FIELD, FALSE);
    if ((folio->func_state & TXTSW_FUNC_FIELD) == 0)
	return (ES_INFINITY);
    if ((folio->func_state & TXTSW_FUNC_EXECUTE) == 0)
	goto Done;
    if (multibyte)
        start_marker = (field_flag == TEXTSW_FIELD_FORWARD) ? 
               (wchar_t *)bar_gt : (wchar_t *)lt_bar;
    else
        start_marker = (field_flag == TEXTSW_FIELD_FORWARD) ? 
               (char *) "|>" : (char *) "<|";
    (void) textsw_match_selection_and_normalize(view, start_marker, field_flag);
Done:
    textsw_end_function(view, TXTSW_FUNC_FIELD);
    return (0);
}


static int
check_selection(buf, buf_len, first, last_plus_one,
		marker1, marker1_len, field_flag)
    Es_index       *first, *last_plus_one;
    void           *buf, *marker1;
    unsigned        buf_len, marker1_len;
    unsigned        field_flag;
{
    int             do_search;

    if ((field_flag == TEXTSW_FIELD_FORWARD) ||
	(field_flag == TEXTSW_DELIMITER_FORWARD)) {
	if (buf_len >= marker1_len) {
            if (!multibyte) {
	        if (strncmp(buf, marker1, marker1_len) == 0) {
		    if (buf_len >= (marker1_len * 2)) {	
		        char            marker2[3];
		        int             marker2_len;
		        unsigned        direction;
		        static void     textsw_get_match_symbol();

		        buf = (char *)buf + (buf_len - marker1_len);
		        (void) textsw_get_match_symbol(marker1, marker1_len,
					 marker2, &marker2_len, &direction);
		        if ((strncmp(buf, marker2, marker2_len) == 0) ||
			    (buf_len >= (MAX_STR_LENGTH - 1))) {
                                  /* Buffer overflow case */
			    if (*first == *last_plus_one)
			        *first = (*first - buf_len);
			  
			    *first = *first + marker1_len;
			    do_search = TRUE;
		        }
		    }
	        } else {
		    do_search = TRUE;
	        }
            } else {
	        if (wsncmp(buf, marker1, marker1_len) == 0) {
		    if (buf_len >= (marker1_len * 2)) {	
		        wchar_t         marker2[3];
		        int             marker2_len;
		        unsigned        direction;
		        static void     textsw_get_match_symbol();

		        buf = (wchar_t *)buf + (buf_len - marker1_len);
		        (void) textsw_get_match_symbol(marker1, marker1_len,
					 marker2, &marker2_len, &direction);
		        if ((wsncmp(buf, marker2, marker2_len) == 0) ||
			    (buf_len >= (MAX_STR_LENGTH - 1))) {
                                  /* Buffer overflow case */
			    if (*first == *last_plus_one)
			        *first = (*first - buf_len);
			  
			    *first = *first + marker1_len;
			    do_search = TRUE;
		        }
		    }
	        } else {
		    do_search = TRUE;
	        }
            }
	} else
	    do_search = TRUE;
    } else if ((field_flag & TEXTSW_FIELD_BACKWARD) ||
	       (field_flag & TEXTSW_DELIMITER_BACKWARD)) {
	if (buf_len >= marker1_len) {
            if (!multibyte) {
	        char   *temp;

   	        temp = (char *)buf + (buf_len - marker1_len);
	        if (strncmp(temp, marker1, marker1_len) == 0) {
		    if (buf_len >= (marker1_len * 2)) {
		        char            marker2[3];
		        int             marker2_len;
		        unsigned        direction;
		        static void     textsw_get_match_symbol();

		        (void) textsw_get_match_symbol(marker1, marker1_len,
					 marker2, &marker2_len, &direction);
		        if (strncmp(buf, marker2, marker2_len) == 0) {
			    *last_plus_one = *last_plus_one - marker2_len;
			    *first = *last_plus_one;
			    do_search = TRUE;
		        }
		    }
	        } else {
		    if (buf_len >= (MAX_STR_LENGTH - 1)) {	
		        *last_plus_one = *last_plus_one - marker1_len;
		        *first = *last_plus_one;
		    }
		    do_search = TRUE;
	        }
            } else {
	        wchar_t   *temp;

   	        temp = (wchar_t *)buf + (buf_len - marker1_len);
	        if (wsncmp(temp, marker1, marker1_len) == 0) {
		    if (buf_len >= (marker1_len * 2)) {
		        wchar_t            marker2[3];
		        int             marker2_len;
		        unsigned        direction;
		        static void     textsw_get_match_symbol();

		        (void) textsw_get_match_symbol(marker1, marker1_len,
					 marker2, &marker2_len, &direction);
		        if (wsncmp(buf, marker2, marker2_len) == 0) {
			    *last_plus_one = *last_plus_one - marker2_len;
			    *first = *last_plus_one;
			    do_search = TRUE;
		        }
		    }
	        } else {
		    if (buf_len >= (MAX_STR_LENGTH - 1)) {	
		        *last_plus_one = *last_plus_one - marker1_len;
		        *first = *last_plus_one;
		    }
		    do_search = TRUE;
	        }

            }
	} else
	    do_search = TRUE;
    }
    return (do_search);
}

static          Es_index
get_start_position(folio, first, last_plus_one,
		   symbol1, symbol1_len,
		   symbol2, symbol2_len, field_flag, do_search)
    Textsw_folio    folio;
    Es_index       *first, *last_plus_one;
    void           *symbol1, *symbol2;
    unsigned        symbol1_len, symbol2_len;
    unsigned        field_flag;
    int             do_search;

{
    Es_index        start_at = ES_CANNOT_SET;


    unsigned        direction = ((field_flag == TEXTSW_FIELD_FORWARD) ||
				 (field_flag == TEXTSW_DELIMITER_FORWARD)) ?
    EV_FIND_DEFAULT : EV_FIND_BACKWARD;

    if (do_search) {
	(void) textsw_find_pattern(folio, first, last_plus_one,
				   symbol1, symbol1_len, direction);
    }
    switch (field_flag) {
      case TEXTSW_NOT_A_FIELD:
      case TEXTSW_FIELD_FORWARD:
      case TEXTSW_DELIMITER_FORWARD:
	start_at = *first;
	break;
      case TEXTSW_FIELD_ENCLOSE:
      case TEXTSW_DELIMITER_ENCLOSE:{
	    unsigned        dummy;

	    if (symbol2_len == 0)
		textsw_get_match_symbol(symbol1, symbol1_len,
					symbol2, &symbol2_len, &dummy);

	    start_at = ev_find_enclose_end_marker(folio->views->esh,
						  symbol1, symbol1_len,
						  symbol2, symbol2_len,
						  *first);
	    break;
	}
      case TEXTSW_FIELD_BACKWARD:
      case TEXTSW_DELIMITER_BACKWARD:{
	    start_at = ((*first == ES_CANNOT_SET)
			? ES_CANNOT_SET : *last_plus_one);
	    break;
	}
    }

    return (start_at);
}

static void
textsw_get_match_symbol(buf, buf_len, match_buf, match_buf_len, direction)
    void           *buf, *match_buf;
    int             buf_len;
    int            *match_buf_len;
    unsigned       *direction;
{
    int             i, j, index;

    *match_buf_len = 0;
    *direction = EV_FIND_DEFAULT;
    if (!multibyte) {
        ((char *)match_buf)[0] = NULL;

        for (i = 0; i < NUM_OF_COL; i++) {
	    for (j = 0; j < MAX_SYMBOLS; j++) {
	        if (strncmp(buf, match_table[i][j], buf_len) == 0) {
		    index = ((i == 0) ? 1 : 0);
		    strcpy(match_buf, match_table[index][j]);
		    *match_buf_len = strlen(match_buf);
		    if (index == 0)
		        *direction = EV_FIND_BACKWARD;
		    return;
	        }
	    }
        }
    } else {
        ((wchar_t *)match_buf)[0] = NULL;

        for (i = 0; i < NUM_OF_COL; i++) {
	    for (j = 0; j < MAX_SYMBOLS; j++) {
	        if (wsncmp(buf, match_table_wc[i][j], buf_len) == 0) {
		    index = ((i == 0) ? 1 : 0);
		    wscpy(match_buf, match_table_wc[index][j]);
		    *match_buf_len = wslen(match_buf);
		    if (index == 0)
		        *direction = EV_FIND_BACKWARD;
		    return;
	        }
	    }
        }
    }
}


Pkg_private     Es_index
textsw_match_same_marker(folio, marker, marker_len, start_pos, direction)
    Textsw_folio    folio;
    void           *marker;
    int             marker_len;
    Es_index        start_pos;
    unsigned        direction;
{
    Es_index        first, last_plus_one;
    Es_index        result_pos;
    int             plus_or_minus_one = (direction == EV_FIND_BACKWARD) ? -1 : 1;

    first = last_plus_one = start_pos + plus_or_minus_one;

    (void) textsw_find_pattern(folio, &first, &last_plus_one,
			       marker, marker_len, direction);

    result_pos = (direction == EV_FIND_BACKWARD) ? last_plus_one : first;
    if (result_pos == start_pos)
	result_pos = ES_CANNOT_SET;
    else if (result_pos != ES_CANNOT_SET)
	result_pos = result_pos + plus_or_minus_one;

    return (result_pos);
}

/* Caller must set *first to be position at which to start the search. */
Pkg_private void
textsw_match_field(textsw, first, last_plus_one, symbol1, symbol1_len,
		   symbol2, symbol2_len, field_flag, do_search)
    Textsw_folio    textsw;
    Es_index       *first, *last_plus_one;
    void           *symbol1, *symbol2;
    int             symbol1_len, symbol2_len;
    unsigned        field_flag;
    int             do_search;	/* If TRUE, is called by textsw_match_bytes */
{

    Es_handle       esh = textsw->views->esh;
    Es_index        start_at = *first;
    Es_index        result_pos;
    int             rc;
    unsigned        direction = ((field_flag == TEXTSW_FIELD_FORWARD) ||
				 (field_flag == TEXTSW_DELIMITER_FORWARD)) ?
    EV_FIND_DEFAULT : EV_FIND_BACKWARD;


    start_at = get_start_position(textsw, first, last_plus_one,
				  symbol1, symbol1_len,
			       symbol2, symbol2_len, field_flag, do_search);

    if ((symbol1_len == 0) || (start_at == ES_CANNOT_SET)) {
	*first = ES_CANNOT_SET;
	return;
    }
    if (symbol2_len == 0)
	textsw_get_match_symbol(symbol1, symbol1_len,
				symbol2, &symbol2_len, &direction);

    if ((symbol2_len == 0) || (symbol2_len != symbol1_len)) {
	*first = ES_CANNOT_SET;
	return;
    }
    if ((direction == EV_FIND_BACKWARD) &&
	(field_flag == TEXTSW_NOT_A_FIELD)) {
	start_at = *last_plus_one;
    }

    if (!multibyte)
        rc = strncmp(symbol1, symbol2, symbol1_len);
    else 
        rc = wsncmp(symbol1, symbol2, symbol1_len);

    if (rc == 0) {

	direction = ((field_flag == TEXTSW_NOT_A_FIELD) ||
		     (field_flag == TEXTSW_FIELD_FORWARD) ||
		     (field_flag == TEXTSW_DELIMITER_FORWARD)) ?
	    EV_FIND_DEFAULT : EV_FIND_BACKWARD;
	result_pos = textsw_match_same_marker(textsw, symbol1, symbol1_len, start_at, direction);
    } else
	result_pos = ev_match_field_in_esh(esh, symbol1, symbol1_len,
					   symbol2, symbol2_len, start_at,
					   direction);

    if ((field_flag == TEXTSW_FIELD_FORWARD) ||
	(field_flag == TEXTSW_DELIMITER_FORWARD) ||
	((field_flag == TEXTSW_NOT_A_FIELD) && (direction != EV_FIND_BACKWARD))) {

	*first = start_at;
	*last_plus_one = ((result_pos >= start_at)
			  ? result_pos : ES_CANNOT_SET);
    } else {
	*first = ((result_pos <= start_at) ? result_pos : ES_CANNOT_SET);
	*last_plus_one = start_at;

    }

}

Pkg_private int
textsw_match_field_and_normalize(
	  view, first, last_plus_one, buf1, buf_len1, field_flag, do_search)
    Textsw_view_handle view;
    register Es_index *first, *last_plus_one;
    void           *buf1;
    int             buf_len1;
    unsigned        field_flag;
    int             do_search;

{

    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    wchar_t         buf2[MAX_STR_LENGTH];
    int             buf_len2 = 0;
    register Es_index ro_bdry;
    register int    want_pending_delete;
    int             matched = FALSE;
    Es_index        save_first = *first;
    Es_index        save_last_plus_one = *last_plus_one;

    (void) textsw_match_field(folio, first, last_plus_one,
			      buf1, buf_len1, buf2, buf_len2,
			      field_flag, do_search);

    if (((*first == save_first) && (*last_plus_one == save_last_plus_one)) ||
	((*first == ES_CANNOT_SET) || (*last_plus_one == ES_CANNOT_SET))) {
	(void) window_bell(WINDOW_FROM_VIEW(view));
    } else {
	/*
	 * Unfortunately, textsw_possibly_normalize_and_set_selection will
	 * not honor request for pending-delete, so we call
	 * textsw_SetSelection explicitly in such cases. WARNING!  However,
	 * we don't allow pending-delete if we overlap the read-only portion
	 * of the window.
	 */
	want_pending_delete = ((field_flag == TEXTSW_FIELD_FORWARD) ||
			       (field_flag == TEXTSW_FIELD_BACKWARD) ||
			       (field_flag == TEXTSW_FIELD_ENCLOSE));

	if (want_pending_delete) {
	    ro_bdry = TXTSW_IS_READ_ONLY(folio) ? *last_plus_one
		: textsw_read_only_boundary_is_at(folio);
	    if (*last_plus_one <= ro_bdry) {
		want_pending_delete = FALSE;
	    }
	}
	(void) textsw_possibly_normalize_and_set_selection(
			      VIEW_REP_TO_ABS(view), *first, *last_plus_one,
			      ((want_pending_delete) ? 0 : EV_SEL_PRIMARY));
	if (want_pending_delete) {
	    (void) textsw_SetSelection(
			      VIEW_REP_TO_ABS(view), *first, *last_plus_one,
				      (EV_SEL_PRIMARY | EV_SEL_PD_PRIMARY));
	}
	(void) textsw_set_insert(folio, *last_plus_one);
	textsw_record_match(folio, field_flag, buf1);
	matched = TRUE;
    }
    return (matched);
}

Pkg_private int
textsw_match_selection_and_normalize(view, start_marker, field_flag)
    register Textsw_view_handle view;
    register unsigned field_flag;
    void           *start_marker;
{
    register Textsw_folio textsw = FOLIO_FOR_VIEW(view);
    Pkg_private int textsw_get_selection();
    Es_index        first, last_plus_one;
    wchar_t         buf[MAX_STR_LENGTH];
    int             str_length = MAX_STR_LENGTH;
    int             do_search = TRUE;

    /*
     * This procedure should only be called if field flag is set to
     * TEXTSW_FIELD_FORWARD, TEXTSW_FIELD_BACKWARD, TEXTSW_DELIMITER_FORWARD,
     * TEXTSW_DELIMITER_BACKWARD.
     */

    if (textsw_get_selection(view, &first, &last_plus_one, NULL, 0)) {
	if ((last_plus_one - first) < MAX_STR_LENGTH)
	    str_length = (last_plus_one - first);
        if (multibyte)
	    xv_get(VIEW_REP_TO_ABS(view), TEXTSW_CONTENTS_WCS, first, buf,
                             str_length);
        else
	    xv_get(VIEW_REP_TO_ABS(view), TEXTSW_CONTENTS, first, buf, 
                                                              str_length);

	if (str_length == MAX_STR_LENGTH)
	    str_length--;

        if (!multibyte)
	    ((char *)buf)[str_length] = NULL;
        else
            ((wchar_t *)buf)[str_length] = NULL;

	if (field_flag == TEXTSW_NOT_A_FIELD) {
	    if (str_length > ONE_FIELD_LENGTH) {
		(void) window_bell(WINDOW_FROM_VIEW(view));
		return (FALSE);	/* Not a vaild marker size */
	    }
	    start_marker = buf;
	    do_search = FALSE;
	} else {
            if (multibyte)
	        do_search = check_selection(buf,str_length, &first,
                                  &last_plus_one, start_marker, 
                                 wslen(start_marker), field_flag);
            else	        
               do_search = check_selection(buf,str_length, &first,
                                  &last_plus_one, start_marker, 
                                 strlen(start_marker), field_flag);
	}
    } else {
	if (field_flag == TEXTSW_NOT_A_FIELD) {	/* Matching must have a
						 * selection */
	    (void) window_bell(WINDOW_FROM_VIEW(view));
	    return (FALSE);
	}
	first = last_plus_one = EV_GET_INSERT(textsw->views);
    }

    if (!multibyte)
        return (textsw_match_field_and_normalize(view, &first, &last_plus_one,
		start_marker, strlen(start_marker), field_flag, do_search));
    else
        return (textsw_match_field_and_normalize(view, &first, &last_plus_one,
		start_marker, wslen(start_marker), field_flag, do_search));

}

static int
textsw_match_bytesWC(abstract, first, last_plus_one, start_sym, start_sym_len,
		   end_sym, end_sym_len, field_flag)
    Textsw          abstract;	/* find in this textsw */
    Textsw_index   *first;	/* start here, return start of found pattern
				 * here */
    Textsw_index   *last_plus_one;	/* return end of found pattern */
    char           *start_sym, *end_sym;	/* begin and end pattern */
    int             start_sym_len, end_sym_len;	/* patterns length */
    unsigned        field_flag;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));
    int             save_first = *first;
    int             save_last_plus_one = *last_plus_one;
    wchar_t         start_wsym_buf[TMP_BUFSIZE], end_wsym_buf[TMP_BUFSIZE];
    wchar_t        *start_wsym = start_wsym_buf; /* begin pattern */
    wchar_t        *end_wsym = end_wsym_buf;	 /* end pattern */
    int             start_wsym_len, end_wsym_len; /* patterns length */
    int             big_len_flag;

    if (start_sym_len >= TMP_BUFSIZE)
	start_wsym = xv_malloc((start_sym_len + 1) * sizeof(wchar_t));
    start_wsym_len = textsw_mbstowcs_by_mblen(start_wsym, start_sym,
					      &start_sym_len, &big_len_flag);
    start_wsym[start_wsym_len] = 0;
    /*
     * when original start_sym_len is bigger than length of string in
     * start_sym, do error retrun as the generic textsw's behavior.
     */
    if (big_len_flag)
	goto Error_return;

    if (end_sym_len >= TMP_BUFSIZE)
	end_wsym = xv_malloc((end_sym_len + 1) * sizeof(wchar_t) );
    end_wsym_len = textsw_mbstowcs_by_mblen(end_wsym, end_sym,
					    &end_sym_len, &big_len_flag);
    end_wsym[end_wsym_len] = 0;
    if (big_len_flag)
	goto Error_return;

    *first = textsw_wcpos_from_mbpos(folio, *first);
    if ((field_flag == TEXTSW_DELIMITER_FORWARD) ||
	(field_flag == TEXTSW_FIELD_FORWARD)) {
	(void) textsw_match_field(folio, first, last_plus_one,
				  start_wsym, start_wsym_len,
				  end_wsym, end_wsym_len, field_flag, TRUE);
    } else {
	(void) textsw_match_field(folio, first, last_plus_one,
				  end_wsym, end_wsym_len,
				  start_wsym, start_wsym_len, field_flag,
			       ((field_flag == TEXTSW_DELIMITER_BACKWARD) ||
				(field_flag == TEXTSW_FIELD_BACKWARD)));
    }
    if (start_wsym != start_wsym_buf)
	free(start_wsym);
    if (end_wsym != end_wsym_buf)
	free(end_wsym);

    if ((*first == ES_CANNOT_SET) || (*last_plus_one == ES_CANNOT_SET)) {
	*first = save_first;
	*last_plus_one = save_last_plus_one;
	return -1;
    } else {
	save_first = *first; /* save character based first */
	*first = textsw_mbpos_from_wcpos(folio, *first);
	*last_plus_one = textsw_mbpos_from_wcpos(folio, *last_plus_one);
	return *first;
    }

Error_return:
    if (start_wsym != start_wsym_buf)
	free(start_wsym);
    if (end_wsym != end_wsym_buf)
	free(end_wsym);
    return -1;
}

static int
textsw_match_bytesSB(abstract, first, last_plus_one, start_sym, start_sym_len,
		   end_sym, end_sym_len, field_flag)
    Textsw          abstract;	/* find in this textsw */
    Textsw_index   *first;	/* start here, return start of found pattern
				 * here */
    Textsw_index   *last_plus_one;	/* return end of found pattern */
    wchar_t        *start_sym, *end_sym;	/* begin and end pattern */
    int             start_sym_len, end_sym_len;	/* patterns length */
    unsigned        field_flag;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));
    int             save_first = *first;
    int             save_last_plus_one = *last_plus_one;
    char            start_wsym_buf[TMP_BUFSIZE], end_wsym_buf[TMP_BUFSIZE];
    char           *start_wsym = start_wsym_buf; /* begin pattern */
    char           *end_wsym = end_wsym_buf;	 /* end pattern */
    int             start_wsym_len, end_wsym_len; /* patterns length */
    int             big_len_flag;

    if (start_sym_len >= TMP_BUFSIZE)
	start_wsym = xv_malloc(start_sym_len + 1);
    start_wsym_len = wcstombs(start_wsym, start_sym, start_sym_len);
    start_wsym[start_wsym_len] = 0;
    if (end_sym_len >= TMP_BUFSIZE)
	end_wsym = xv_malloc(end_sym_len + 1);
    end_wsym_len = wcstombs(end_wsym, end_sym, end_sym_len);
    end_wsym[end_wsym_len] = 0;
    *first = textsw_mbpos_from_wcpos(folio, *first);
    if ((field_flag == TEXTSW_DELIMITER_FORWARD) ||
	(field_flag == TEXTSW_FIELD_FORWARD)) {
	(void) textsw_match_field(folio, first, last_plus_one,
				  start_wsym, start_wsym_len,
				  end_wsym, end_wsym_len, field_flag, TRUE);
    } else {
	(void) textsw_match_field(folio, first, last_plus_one,
				  end_wsym, end_wsym_len,
				  start_wsym, start_wsym_len, field_flag,
			       ((field_flag == TEXTSW_DELIMITER_BACKWARD) ||
				(field_flag == TEXTSW_FIELD_BACKWARD)));
    }
    if (start_wsym != start_wsym_buf)
	free(start_wsym);
    if (end_wsym != end_wsym_buf)
	free(end_wsym);

    if ((*first == ES_CANNOT_SET) || (*last_plus_one == ES_CANNOT_SET)) {
	*first = save_first;
	*last_plus_one = save_last_plus_one;
	return -1;
    } else {
	save_first = *first; /* save character based first */
	*first = textsw_wcpos_from_mbpos(folio, *first);
	*last_plus_one = textsw_wcpos_from_mbpos(folio, *last_plus_one);
	return *first;
    }

Error_return:
    if (start_wsym != start_wsym_buf)
	free(start_wsym);
    if (end_wsym != end_wsym_buf)
	free(end_wsym);
    return -1;
}


/*
 * If the pattern is matched, return the index where it is found, else return
 * -1.
 */
Xv_public int
textsw_match_wcs(abstract, first, last_plus_one, start_sym, start_sym_len,
		   end_sym, end_sym_len, field_flag)
    Textsw          abstract;	/* find in this textsw */
    Textsw_index   *first;	/* start here, return start of found pattern
				 * here */
    Textsw_index   *last_plus_one;	/* return end of found pattern */
    wchar_t        *start_sym, *end_sym;	/* begin and end pattern */
    int             start_sym_len, end_sym_len;	/* patterns length */
    unsigned        field_flag;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));
    int             save_first = *first;
    int             save_last_plus_one = *last_plus_one;

    if (multibyte) {
        if ((field_flag == TEXTSW_DELIMITER_FORWARD) ||
	    (field_flag == TEXTSW_FIELD_FORWARD)) {
	    (void) textsw_match_field(folio, first, last_plus_one,
				  start_sym, start_sym_len,
				  end_sym, end_sym_len, field_flag, TRUE);
        } else {
	    (void) textsw_match_field(folio, first, last_plus_one,
				  end_sym, end_sym_len,
				  start_sym, start_sym_len, field_flag,
			       ((field_flag == TEXTSW_DELIMITER_BACKWARD) ||
				(field_flag == TEXTSW_FIELD_BACKWARD)));
        }

        if ((*first == ES_CANNOT_SET) || (*last_plus_one == ES_CANNOT_SET)) {
	    *first = save_first;
	    *last_plus_one = save_last_plus_one;
	    return -1;
        } else {
	    return *first;
        }
    } else 
        return (textsw_match_bytesSB(abstract, first, last_plus_one, 
                start_sym, start_sym_len, end_sym, end_sym_len, field_flag));
}

Xv_public int
textsw_match_bytes(abstract, first, last_plus_one, start_sym, start_sym_len,
		   end_sym, end_sym_len, field_flag)
    Textsw          abstract;	/* find in this textsw */
    Textsw_index   *first;	/* start here, return start of found pattern
				 * here */
    Textsw_index   *last_plus_one;	/* return end of found pattern */
    char           *start_sym, *end_sym;	/* begin and end pattern */
    int             start_sym_len, end_sym_len;	/* patterns length */
    unsigned        field_flag;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(VIEW_ABS_TO_REP(abstract));
    int             save_first = *first;
    int             save_last_plus_one = *last_plus_one;


    if (!multibyte) {
        if ((field_flag == TEXTSW_DELIMITER_FORWARD) ||
	    (field_flag == TEXTSW_FIELD_FORWARD)) {
	    (void) textsw_match_field(folio, first, last_plus_one,
				  start_sym, start_sym_len,
				  end_sym, end_sym_len, field_flag, TRUE);
        } else {
	    (void) textsw_match_field(folio, first, last_plus_one,
				  end_sym, end_sym_len,
				  start_sym, start_sym_len, field_flag,
			       ((field_flag == TEXTSW_DELIMITER_BACKWARD) ||
				(field_flag == TEXTSW_FIELD_BACKWARD)));
        }

        if ((*first == ES_CANNOT_SET) || (*last_plus_one == ES_CANNOT_SET)) {
	    *first = save_first;
	    *last_plus_one = save_last_plus_one;
	    return -1;
        } else {
	    return *first;
        }
    } else
        return(textsw_match_bytesWC(abstract, first, last_plus_one, start_sym,
                start_sym_len, end_sym, end_sym_len, field_flag));
}

#undef TMP_BUFSIZE
