#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)txt_match.c 1.33 93/07/26";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Text match delimiter popup frame creation and support.
 */

#include <xview_private/primal.h>
#include <xview_private/txt_impl.h>
#include <xview_private/ev_impl.h>
#include <sys/time.h>
#include <signal.h>
#include <xview/notice.h>
#include <xview/frame.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/openmenu.h>
#include <xview/wmgr.h>
#include <xview/pixwin.h>
#include <xview/win_struct.h>
#include <xview/win_screen.h>

#define		MAX_DISPLAY_LENGTH	50
#define   	MAX_STR_LENGTH		1024

#define HELP_INFO(s) XV_HELP_DATA, s,

/* This is for find marked text */
typedef enum {
    CHOICE_ITEM = 0,
    FIND_PAIR_ITEM = 1,
    FIND_PAIR_CHOICE_ITEM = 2,
    INSERT_ITEM = 3,
    REMOVE_ITEM = 4
} Match_panel_item_enum;

extern Panel_item match_panel_items[];

Pkg_private Textsw_view_handle text_view_frm_p_itm();
Pkg_private Xv_Window frame_from_panel_item();

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

static wchar_t    *delimiter_pairs_wc[2][8] = {
    { l_paren, dbl_quote, sgl_quote, accent_grave, bar_gt, l_square_brace,
      l_curly_brace, open_comment,},
    { r_paren, dbl_quote, sgl_quote, accent_grave, lt_bar, r_square_brace,
      r_curly_brace, close_comment,}
};

static char    *delimiter_pairs[2][8] = {
    {"(", "\"", "'", "`", "|>", "[", "{", "/*"},
    {")", "\"", "'", "`", "<|", "]", "}", "*/"}
};

static void
do_insert_or_remove_delimiter(view, value, do_insert)
    Textsw_view_handle view;
    int             do_insert;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    Es_index        first, last_plus_one, ro_bdry, num_of_byte, temp;
    void           *buf, *sel_str, *temp_ptr;
    Xv_Notice	    text_notice;
    Frame	    frame;


    frame = FRAME_FROM_FOLIO_OR_VIEW(folio);

    if (TXTSW_IS_READ_ONLY(folio)) {
        text_notice = (Xv_Notice)xv_get(frame, 
			XV_KEY_DATA, text_notice_key, NULL);
        if (!text_notice)  {
            text_notice = xv_create(frame, NOTICE, NULL);
            xv_set(frame, 
                XV_KEY_DATA, text_notice_key, text_notice,
                NULL);
        }
        xv_set(text_notice, 
                NOTICE_LOCK_SCREEN, FALSE,
		NOTICE_BLOCK_THREAD, TRUE,
                NOTICE_MESSAGE_STRINGS,
	        XV_MSG("Operation is aborted.\n\
This text window is read only."),
                0,
                NOTICE_BUTTON_YES, XV_MSG("Continue"),
                XV_SHOW, TRUE, 
            NULL);
	return;
    }
    ro_bdry = textsw_read_only_boundary_is_at(folio);
    (void) ev_get_selection(folio->views, &first, &last_plus_one, 
                                                EV_SEL_PRIMARY);
    if ((ro_bdry != 0) && (last_plus_one <= ro_bdry)) {
        text_notice = (Xv_Notice)xv_get(frame, 
                                XV_KEY_DATA, text_notice_key, 
                                NULL);

        if (!text_notice)  {
            text_notice = xv_create(frame, NOTICE, NULL);
            xv_set(frame, 
                XV_KEY_DATA, text_notice_key, text_notice,
                NULL);
        }
        xv_set(text_notice, 
                NOTICE_LOCK_SCREEN, FALSE,
		NOTICE_BLOCK_THREAD, TRUE,
                NOTICE_MESSAGE_STRINGS,
		XV_MSG("Operation is aborted.\n\
Selected text is in read only area."),
                0,
                NOTICE_BUTTON_YES, XV_MSG("Continue"),
                XV_SHOW, TRUE, 
            NULL);
	return;

    }
    buf = sel_str = temp_ptr = NULL;
    if (do_insert) {
        if (!multibyte) {
	    temp_ptr = buf = xv_malloc((unsigned)((last_plus_one-first)+5));
	    ((char *)buf)[0] = NULL;
	    strcpy(buf, delimiter_pairs[0][value]);
	    temp_ptr = (char *)buf + strlen(buf);

	    if (first < last_plus_one) {
	        sel_str = xv_malloc((unsigned) (last_plus_one - first + 1));
	        if (textsw_get_selection_as_string(folio, 
                     EV_SEL_PRIMARY, sel_str, ((last_plus_one - first) + 1))) {
		    strcpy(temp_ptr, sel_str);
		    temp_ptr = (char *)buf + strlen(buf);
	        }
	    } else {
	        first = last_plus_one = EV_GET_INSERT(folio->views);
	    }
	    strcpy(temp_ptr, delimiter_pairs[1][value]);
        } else {
	    temp_ptr = buf = xv_malloc((unsigned)(
                      (last_plus_one-first)+5) * sizeof(wchar_t));
	    ((wchar_t *)buf)[0] = NULL;
	    wscpy(buf, delimiter_pairs_wc[0][value]);
	    temp_ptr = (wchar_t *)buf + wslen(buf);

	    if (first < last_plus_one) {
	        sel_str = xv_malloc((unsigned) (last_plus_one - first + 1)
                                                * sizeof(wchar_t));
	        if (textsw_get_selection_as_string(folio, 
                     EV_SEL_PRIMARY, sel_str, ((last_plus_one - first) + 1))) {
		    wscpy(temp_ptr, sel_str);
		    temp_ptr = (wchar_t *)buf + wslen(buf);
	        }
	    } else {
	        first = last_plus_one = EV_GET_INSERT(folio->views);
	    }
	    wscpy(temp_ptr, delimiter_pairs_wc[1][value]);
        }
    } else {
	int  sel_str_len = last_plus_one - first;

        if (!multibyte)
	    buf = xv_malloc(sel_str_len);
        else
            buf = xv_malloc(sel_str_len * sizeof(wchar_t));

	if (first < last_plus_one) {
            if (!multibyte)      
	        sel_str = xv_malloc(sel_str_len + 1);
            else
	        sel_str = xv_malloc((sel_str_len + 1) * sizeof(wchar_t));

	    if (textsw_get_selection_as_string(folio, EV_SEL_PRIMARY,
                                              sel_str, (sel_str_len + 1))) {
                int do_notice = FALSE;
                if (!multibyte) {
  	            int  del_len1 = strlen(delimiter_pairs[0][value]);
	            int  del_len2 = strlen(delimiter_pairs[1][value]);
     
		    temp_ptr = (char *)sel_str + (sel_str_len - del_len2);

		    if ((strncmp(delimiter_pairs[0][value], sel_str, 
                                         del_len1) == 0) &&
		        (strncmp(delimiter_pairs[1][value], temp_ptr, 
                                         del_len2) == 0)) {
		        strncpy(buf, (char *)sel_str + del_len1,
			         (sel_str_len - (del_len1 + del_len2)));
		        ((char *)buf)
                             [(sel_str_len - (del_len1 + del_len2))] = NULL;
                    } else 
                        do_notice = TRUE;
                }  else {
  	            int  del_len1 = wslen(delimiter_pairs_wc[0][value]);
	            int  del_len2 = wslen(delimiter_pairs_wc[1][value]);
     
		    temp_ptr = (wchar_t *)sel_str + (sel_str_len - del_len2);

		    if ((wsncmp(delimiter_pairs_wc[0][value], sel_str, 
                                         del_len1) == 0) &&
		        (wsncmp(delimiter_pairs_wc[1][value], temp_ptr, 
                                         del_len2) == 0)) {
		        wsncpy(buf, (wchar_t *)sel_str + del_len1,
			         (sel_str_len - (del_len1 + del_len2)));
		        ((wchar_t *)buf)
                          [(sel_str_len - (del_len1 + del_len2))] = NULL;
                    } else 
                        do_notice = TRUE;
                }
                        
                if (do_notice) {
                    text_notice = (Xv_Notice)xv_get(frame, 
                                    XV_KEY_DATA, text_notice_key, 
                                    NULL);

	            if (!text_notice)  {
	                text_notice = xv_create(frame, NOTICE, NULL);
		        xv_set(frame, 
		                XV_KEY_DATA, text_notice_key, text_notice,
		                NULL);
	            }
	            xv_set(text_notice, 
		            NOTICE_LOCK_SCREEN, FALSE,
			    NOTICE_BLOCK_THREAD, TRUE,
		            NOTICE_MESSAGE_STRINGS,
			    XV_MSG("Operation is aborted.\n\
Selection does not include the indicated pair."),
		            0,
		            NOTICE_BUTTON_YES, 
				XV_MSG("Continue"),
			    XV_SHOW, TRUE, 
		        NULL);
		    return;
		}
	    }
	} else {
            text_notice = (Xv_Notice)xv_get(frame, 
                                XV_KEY_DATA, text_notice_key, 
                                NULL);

            if (!text_notice)  {
                text_notice = xv_create(frame, NOTICE, NULL);
                xv_set(frame, 
                    XV_KEY_DATA, text_notice_key, text_notice,
                    NULL);
            }
            xv_set(text_notice, 
                    NOTICE_LOCK_SCREEN, FALSE,
		    NOTICE_BLOCK_THREAD, TRUE,
                    NOTICE_MESSAGE_STRINGS,
			XV_MSG("Operation is aborted, because no text is selected"),
                    0,
                    NOTICE_BUTTON_YES, XV_MSG("Continue"),
                    XV_SHOW, TRUE, 
                NULL);
	    return;
	}
    }

#ifdef OW_I18N
    textsw_implicit_commit(folio);
#endif

    if (!multibyte)
        num_of_byte = textsw_replace(FOLIO_REP_TO_ABS(folio), first, 
                          last_plus_one, buf, strlen(buf));
    else
        num_of_byte = textsw_replace(FOLIO_REP_TO_ABS(folio), first, 
                          last_plus_one, buf, wslen(buf));

    if (num_of_byte != 0)
	EV_SET_INSERT(folio->views, last_plus_one + num_of_byte, temp);
    if (buf != NULL)
	free(buf);
    if (sel_str != NULL)
	free(sel_str);

}

static          Panel_setting
match_cmd_proc(item, event)
    Panel_item      item;
    Event          *event;
{
    Textsw_view_handle view = text_view_frm_p_itm(item);
    int             delimiter_value = (int) panel_get(
		      match_panel_items[(int) CHOICE_ITEM], PANEL_VALUE, NULL);

    if (item == match_panel_items[(int) FIND_PAIR_ITEM]) {
	int             value = (int) panel_get(match_panel_items
                          [(int) FIND_PAIR_CHOICE_ITEM], PANEL_VALUE, NULL);

	switch (value) {
	  case 0:
            if (!multibyte)
	        (void) textsw_match_selection_and_normalize(view,
					delimiter_pairs[0][delimiter_value],
						  TEXTSW_DELIMITER_FORWARD);
            else
	        (void) textsw_match_selection_and_normalize(view,
					delimiter_pairs_wc[0][delimiter_value],
						  TEXTSW_DELIMITER_FORWARD);
	    break;
	  case 1:
            if (!multibyte)
	        (void) textsw_match_selection_and_normalize(view,
					delimiter_pairs[1][delimiter_value],
						 TEXTSW_DELIMITER_BACKWARD);
            else
	        (void) textsw_match_selection_and_normalize(view,
					delimiter_pairs_wc[1][delimiter_value],
						 TEXTSW_DELIMITER_BACKWARD);
	    break;
	  case 2:{
		Textsw_folio    folio = FOLIO_FOR_VIEW(view);
		int             first, last_plus_one;

		first = last_plus_one = EV_GET_INSERT(folio->views);
                if (!multibyte)
		    (void) textsw_match_field_and_normalize(view,
							&first,
							&last_plus_one,
					delimiter_pairs[1][delimiter_value],
				strlen(delimiter_pairs[1][delimiter_value]),
					   TEXTSW_DELIMITER_ENCLOSE, FALSE);
                else
		    (void) textsw_match_field_and_normalize(view,
							&first,
							&last_plus_one,
					delimiter_pairs[1][delimiter_value],
				wslen(delimiter_pairs_wc[1][delimiter_value]),
					   TEXTSW_DELIMITER_ENCLOSE, FALSE);
		break;
	    }
	}

    } else if (item == match_panel_items[(int) INSERT_ITEM]) {
	(void) do_insert_or_remove_delimiter(view, delimiter_value, TRUE);
    } else if (item == match_panel_items[(int) REMOVE_ITEM]) {
	(void) do_insert_or_remove_delimiter(view, delimiter_value, FALSE);
    }
    return PANEL_NEXT;
}
static void
create_match_items(panel, view)
    Panel           panel;
    Textsw_view_handle view;
{

    char           *curly_bracket = " { }  ";
    char           *parenthesis = " ( )  ";
    char           *double_quote = " \" \"  ";
    char           *single_quote = " ' '  ";
    char           *back_qoute = " ` `  ";
    char           *square_bracket = " [ ]  ";
    char           *field_marker = " |> <|  ";
    char           *comment = " /* */  ";

    char           *Insert_Pair = XV_MSG("Insert Pair");
    char           *Backward = XV_MSG("Backward");
    char           *Expand = XV_MSG("Expand");
    char           *Forward = XV_MSG("Forward");
    char           *Remove_Pair = XV_MSG("Remove Pair");
    char           *Find_Pair = XV_MSG("Find Pair");


    match_panel_items[(int) CHOICE_ITEM] =
	panel_create_item(panel, PANEL_CHOICE,
			  PANEL_LABEL_X, ATTR_COL(0),
			  PANEL_LABEL_Y, ATTR_ROW(0),
			  PANEL_CHOICE_STRINGS, parenthesis, double_quote,
			  single_quote, back_qoute,
			  field_marker, square_bracket,
			  curly_bracket, comment, 0,
			  XV_HELP_DATA, "textsw:fieldchoice",
			  0);


    match_panel_items[(int) FIND_PAIR_ITEM] =
	panel_create_item(panel, PANEL_BUTTON,
			  PANEL_LABEL_X, ATTR_COL(0),
			  PANEL_LABEL_Y, ATTR_ROW(1),
			  PANEL_LABEL_STRING, Find_Pair,
			  PANEL_NOTIFY_PROC, match_cmd_proc,
			  XV_HELP_DATA, "textsw:findpair",
			  0);

    match_panel_items[(int) FIND_PAIR_CHOICE_ITEM] =
	panel_create_item(panel, PANEL_CHOICE,
			  PANEL_LABEL_STRING, ":",
			  PANEL_CHOICE_STRINGS, Forward, Backward, Expand, 0,
			  XV_HELP_DATA, "textsw:findpairchoice",
			  0);

    match_panel_items[(int) INSERT_ITEM] =
	panel_create_item(panel, PANEL_BUTTON,
			  PANEL_LABEL_X, ATTR_COL(0),
			  PANEL_LABEL_Y, ATTR_ROW(3),
			  PANEL_LABEL_STRING, Insert_Pair,
			  PANEL_NOTIFY_PROC, match_cmd_proc,
			  XV_HELP_DATA, "textsw:insertpair",
			  0);

    match_panel_items[(int) REMOVE_ITEM] =
	panel_create_item(panel, PANEL_BUTTON,
			  PANEL_LABEL_STRING, Remove_Pair,
			  PANEL_NOTIFY_PROC, match_cmd_proc,
			  XV_HELP_DATA, "textsw:removepair",
			  0);

}

Pkg_private          Panel
textsw_create_match_panel(frame, view)
    Frame           frame;
    Textsw_view_handle view;
{
    Panel           panel;

    panel = (Panel) xv_get(frame, FRAME_CMD_PANEL,
			   XV_HELP_DATA, "textsw:fieldpanel",
			   0);
    (void) create_match_items(panel, view);

    return (panel);
}
