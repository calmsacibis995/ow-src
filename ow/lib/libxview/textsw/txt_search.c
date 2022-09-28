#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)txt_search.c 20.48 93/09/13";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Text search popup frame creation and support.
 */


#include <xview_private/primal.h>
#include <xview_private/txt_impl.h>
#include <xview_private/ev_impl.h>
#include <sys/time.h>
#include <signal.h>
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

Pkg_private Panel_item search_panel_items[];

#define       DONT_RING_BELL               0x00000000
#define       RING_IF_NOT_FOUND            0x00000001
#define       RING_IF_ONLY_ONE             0x00000002

#define HELP_INFO(s) XV_HELP_DATA, s,

static int TEXTSW_FIND_MENU_PANEL_KEY=0;  /* unique key to handle panel 
					     in direction menu for 
					     find and replace */
/* for find and replace */
typedef enum {
    FIND_ITEM = 0,
    FIND_STRING_ITEM = 1,
    REPLACE_ITEM = 2,
    REPLACE_STRING_ITEM = 3,
    FIND_THEN_REPLACE_ITEM = 4,
    REPLACE_THEN_FIND_ITEM = 5,
    REPLACE_ALL_ITEM = 6,
    WRAP_ITEM = 7
} Search_panel_item_enum;

extern void textsw_SetSelection();

Pkg_private Textsw_view_handle text_view_frm_p_itm();

Pkg_private	Es_index
textsw_do_search_proc(view, direction, ring_bell_status, wrapping_off, is_global)
    Textsw_view_handle view;
    unsigned        direction;
    unsigned        ring_bell_status;
    int             wrapping_off;
    int		    is_global;

{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    Es_index        first, last_plus_one;
    wchar_t         buf[MAX_STR_LENGTH];
    int             str_len;
    Es_index        start_pos;


    if (!textsw_get_selection(view, &first, &last_plus_one, NULL, 0))
	first = last_plus_one = EV_GET_INSERT(folio->views);

    if (direction == EV_FIND_DEFAULT)
	first = last_plus_one;

    if (!multibyte) {
        strncpy((char *)buf,(char *)panel_get(search_panel_items
            [(int) FIND_STRING_ITEM],PANEL_VALUE, NULL), MAX_STR_LENGTH);
        str_len = strlen((char *)buf);
    } else {
        wsncpy(buf,(wchar_t *)panel_get(search_panel_items
             [(int) FIND_STRING_ITEM],PANEL_VALUE_WCS, NULL), MAX_STR_LENGTH);
        str_len = wslen(buf);
    }
    start_pos = (direction & EV_FIND_BACKWARD)
	? first : (first - str_len);

    textsw_find_pattern(folio, &first, &last_plus_one, buf, str_len, direction);

    if (wrapping_off) {
	if (direction == EV_FIND_DEFAULT)
	    first = (start_pos > last_plus_one) ? ES_CANNOT_SET : first;
	else
	    first = (start_pos < last_plus_one) ? ES_CANNOT_SET : first;
    }
    if ((first == ES_CANNOT_SET) || (last_plus_one == ES_CANNOT_SET)) {
	if (ring_bell_status & RING_IF_NOT_FOUND)
	    (void) window_bell(WINDOW_FROM_VIEW(view));
	return (ES_CANNOT_SET);
    } else {
	if ((ring_bell_status & RING_IF_ONLY_ONE) && (first == start_pos))
	    (void) window_bell(WINDOW_FROM_VIEW(view));
        if (!is_global)
	    textsw_possibly_normalize_and_set_selection(
	       VIEW_REP_TO_ABS(view), first, last_plus_one, EV_SEL_PRIMARY);
	else
            textsw_SetSelection(VIEW_REP_TO_ABS(view), first, last_plus_one,
                                                           EV_SEL_PRIMARY);
	(void) textsw_set_insert(folio, last_plus_one);
	textsw_record_find(folio, buf, str_len, (int) direction);
	return ((direction == EV_FIND_DEFAULT) ? last_plus_one : first);
    }
}

Pkg_private void
search_event_proc(item, event)
    Panel_item      item;
    Event          *event;
{
    Panel           panel = panel_get(item, XV_OWNER, 0);
    Textsw_view_handle view = text_view_frm_p_itm(item);

    if (event_action(event) == ACTION_MENU && event_is_down(event)) {
	Menu     direction_menu;
	direction_menu = xv_get(panel,XV_KEY_DATA,TEXTSW_FIND_MENU_PANEL_KEY);
	(void) menu_show(direction_menu, panel, event, 0);
    } else {
	(void) panel_default_handle_event(item, event);
    }
}

static void
find_forwards_action_proc(menu, item)
    Menu            menu;
    Menu_item       item;
{
    int             wrapping_off;
    Panel           panel = xv_get(menu,XV_KEY_DATA,
				   TEXTSW_FIND_MENU_PANEL_KEY);
    int             i=0;
    Textsw_view_handle view;
    view = (Textsw_view_handle)xv_get(xv_get(panel,WIN_FRAME),
				      WIN_CLIENT_DATA);
    PANEL_EACH_ITEM(panel,search_panel_items[i])
	i++;
    PANEL_END_EACH
    wrapping_off = (int) panel_get(search_panel_items[(int) WRAP_ITEM], 
				   PANEL_VALUE, NULL);
    (void) textsw_do_search_proc(view, EV_FIND_DEFAULT,
          (RING_IF_NOT_FOUND | RING_IF_ONLY_ONE), wrapping_off, FALSE);
}

static void
find_backwards_action_proc(menu, item)
    Menu            menu;
    Menu_item       item;
{
    int             wrapping_off;
    Panel           panel = xv_get(menu,XV_KEY_DATA,
				   TEXTSW_FIND_MENU_PANEL_KEY);
    int             i=0;
    Textsw_view_handle view;
    view = (Textsw_view_handle)xv_get(xv_get(panel,WIN_FRAME),
				      WIN_CLIENT_DATA);
    PANEL_EACH_ITEM(panel,search_panel_items[i])
	i++;
    PANEL_END_EACH
    wrapping_off = (int) panel_get(search_panel_items[(int) WRAP_ITEM], 
				   PANEL_VALUE, NULL);
    (void) textsw_do_search_proc(view, EV_FIND_BACKWARD,
	 (RING_IF_NOT_FOUND | RING_IF_ONLY_ONE), wrapping_off, FALSE);
}

static int
do_replace_proc(view)
    Textsw_view_handle view;
{
    Textsw          textsw = VIEW_REP_TO_ABS(view);
    int             selection_found;
    int             first, last_plus_one;

    if (selection_found =
	    textsw_get_selection(view, &first, &last_plus_one, NULL, 0)) {
        if (!multibyte) {
            char buf[MAX_STR_LENGTH];
	    strncpy(buf, (char *)panel_get(
		search_panel_items[(int) REPLACE_STRING_ITEM],
		PANEL_VALUE, NULL), MAX_STR_LENGTH);
	    textsw_replace(textsw, first, last_plus_one, buf, strlen(buf));
        } else {
            wchar_t buf[MAX_STR_LENGTH];
	    wsncpy(buf, (wchar_t *)panel_get(
		search_panel_items[(int) REPLACE_STRING_ITEM],
		PANEL_VALUE_WCS, NULL), MAX_STR_LENGTH);
	    textsw_replace(textsw, first, last_plus_one, buf, wslen(buf));
        }
    }
    return (selection_found);
}



static void
do_replace_all_proc(view, do_replace_first, direction)
    Textsw_view_handle view;
    int             do_replace_first;
    unsigned        direction;
{
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    int             start_checking = FALSE;	/* See if now is the time to
						 * check for wrap point */
    Es_index        cur_pos, prev_pos, cur_mark_pos;
    Ev_mark_object  mark;
    int             exit_loop = FALSE;
    int             first_time = TRUE, process_aborted;
    int             wrapping_off = (int) panel_get(search_panel_items
                                        [(int) WRAP_ITEM], PANEL_VALUE, NULL);
    int		    string_length_diff;    

    if (do_replace_first)
	(void) do_replace_proc(view);

    process_aborted = FALSE;

    cur_mark_pos = prev_pos = cur_pos = textsw_do_search_proc(view, 
                             direction, RING_IF_NOT_FOUND, wrapping_off, TRUE);

    exit_loop = (cur_pos == ES_CANNOT_SET);

    if (!multibyte)
        string_length_diff = 
	    strlen((char *)panel_get(search_panel_items[
			(int) REPLACE_STRING_ITEM], PANEL_VALUE, NULL)) -
	    strlen((char *)panel_get(search_panel_items[
			(int) FIND_STRING_ITEM], PANEL_VALUE, NULL));
    else
        string_length_diff = 
	    wslen((wchar_t *)panel_get(search_panel_items[
			(int) REPLACE_STRING_ITEM], PANEL_VALUE_WCS, NULL)) -
	    wslen((wchar_t *)panel_get(search_panel_items[
			(int) FIND_STRING_ITEM], PANEL_VALUE_WCS, NULL));

  /* Note: should use busy cursor since no update for global replace */	    
  /*  textsw_set_cursor(FOLIO_REP_TO_ABS(folio), CURSOR_BUSY_PTR); */

    while (!process_aborted && !exit_loop) {
	if (start_checking) {
	    cur_mark_pos = textsw_find_mark_internal(folio, mark);

	    exit_loop = (direction == EV_FIND_DEFAULT) ?
		(cur_mark_pos <= cur_pos) : (cur_mark_pos >= cur_pos);
	} else {
	    /* Did we wrap around the file already */
	    /* or for some reason search gives same pos */
	    if (!first_time && ((prev_pos == cur_pos) ||
				(cur_pos == cur_mark_pos)))
		/* Only one instance of the pattern in the file. */
		start_checking = TRUE;
	    else 
		start_checking = (direction == EV_FIND_DEFAULT) ?
		    (prev_pos > cur_pos) : (cur_pos > prev_pos);
	    /*
	     * This is a special case Start search at the first instance of
	     * the pattern in the file.
	     */

	    if (start_checking) {
		cur_mark_pos = textsw_find_mark_internal(folio, mark);
		exit_loop = (direction == EV_FIND_DEFAULT) ?
		    (cur_mark_pos <= cur_pos) : (cur_mark_pos >= cur_pos);
	    }
	}

	if (!exit_loop) {
	    (void) do_replace_proc(view);

	    if (first_time) {
		mark = textsw_add_mark_internal(folio, cur_mark_pos,
						TEXTSW_MARK_MOVE_AT_INSERT);
		first_time = FALSE;
	    }
	    prev_pos = cur_pos + string_length_diff;
	    cur_pos = textsw_do_search_proc(view, direction, DONT_RING_BELL,
                                                        wrapping_off, TRUE);
	    exit_loop = (cur_pos == ES_CANNOT_SET);
	}
    }
    
    if (prev_pos != ES_CANNOT_SET)
    textsw_normalize_view(VIEW_REP_TO_ABS(view), prev_pos);    
    
    if (process_aborted)
	window_bell(VIEW_REP_TO_ABS(view));
}

static          Panel_setting
search_cmd_proc(item, event)
    Panel_item      item;
    Event          *event;
{
    Textsw_view_handle view = text_view_frm_p_itm(item);
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    int             wrapping_off;
    Panel           panel = xv_get(item,XV_OWNER);
    int             i = 0;

    /* since there might be two textsw, make sure that we're looking at the
       right one. */
    PANEL_EACH_ITEM(panel,search_panel_items[i])
	i++;
    PANEL_END_EACH
    wrapping_off = (int) panel_get(search_panel_items[(int) WRAP_ITEM], PANEL_VALUE, NULL);
    if (item == search_panel_items[(int) FIND_ITEM]) {
	(void) textsw_do_search_proc(view,
			             EV_FIND_DEFAULT, 
                                     (RING_IF_NOT_FOUND | RING_IF_ONLY_ONE), 
                                     wrapping_off,
                                     FALSE);

    } else if (item == search_panel_items[(int) REPLACE_ITEM]) {
	if (TXTSW_IS_READ_ONLY(folio) || !do_replace_proc(view)) {
	    (void) window_bell(VIEW_REP_TO_ABS(view));
	}
    } else if (item == search_panel_items[(int) FIND_THEN_REPLACE_ITEM]) {
	if (!TXTSW_IS_READ_ONLY(folio)) {
	    if (textsw_do_search_proc(view,
				      EV_FIND_DEFAULT, 
				      RING_IF_NOT_FOUND, 
				      wrapping_off,
				      FALSE) != ES_CANNOT_SET)
		(void) do_replace_proc(view);
	}
    } else if (item == search_panel_items[(int) REPLACE_THEN_FIND_ITEM]) {
	if (!TXTSW_IS_READ_ONLY(folio)) {
	    (void) do_replace_proc(view);
	    (void) textsw_do_search_proc(view,
					 EV_FIND_DEFAULT, 
					 RING_IF_NOT_FOUND, 
					 wrapping_off,
					 FALSE);
	}
    } else if (item == search_panel_items[(int) REPLACE_ALL_ITEM]) {
	(void) do_replace_all_proc(view, FALSE, EV_FIND_DEFAULT);
    }
    return PANEL_NEXT;
}

static void
create_search_items(panel, view)
    Panel           panel;
    Textsw_view_handle view;
{

    static char    *search = "Find";
    static char    *replace = "Replace";
    static char    *all = "Replace All";
    static char    *search_replace = "Find then Replace";
    static char    *replace_search = "Replace then Find";
    static char    *backward = "Backward";
    static char    *forward = "Forward";
    static int	   init_str = 0;
    wchar_t        search_string[MAX_STR_LENGTH];
    int            dummy;
    Pkg_private void search_event_proc();
    Menu            direction_menu;

    if (!init_str)  {
        /*
         * FIX_ME: The current gettext/dgettext return the uniq
         * pointer for all messages, but future version and/or
         * different implementation may behave differently.  If it is
         * the case, you should wrap around following gettext by
         * strdup call.
         */
        search          = XV_MSG("Find");
        replace         = XV_MSG("Replace");
        all             = XV_MSG("Replace All");
        search_replace  = XV_MSG("Find then Replace");
        replace_search  = XV_MSG("Replace then Find");
        backward        = XV_MSG("Backward");
        forward         = XV_MSG("Forward");
        init_str = 1;
    }

    if (!TEXTSW_FIND_MENU_PANEL_KEY) {
	TEXTSW_FIND_MENU_PANEL_KEY = xv_unique_key();
    }

    search_string[0] = NULL;
    (void) textsw_get_selection(view, &dummy, &dummy, search_string,
				MAX_STR_LENGTH);

    direction_menu = xv_create(NULL, MENU,
			       MENU_ITEM,
			       MENU_STRING, forward,
			       MENU_VALUE, 1,
			       HELP_INFO("textsw:mdirforward")
			       MENU_ACTION_PROC, find_forwards_action_proc,
			       0,
			       MENU_ITEM,
			       MENU_STRING, backward,
			       MENU_VALUE, 2,
			       MENU_ACTION_PROC, find_backwards_action_proc,
			       HELP_INFO("textsw:mdirbackward")
			       0,
			       HELP_INFO("textsw:mdirection")
			       XV_KEY_DATA,TEXTSW_FIND_MENU_PANEL_KEY,panel,
			       0);

    xv_set(panel,XV_KEY_DATA,TEXTSW_FIND_MENU_PANEL_KEY,direction_menu,NULL);

    search_panel_items[(int) FIND_ITEM] =
	panel_create_item(panel, PANEL_BUTTON,
			  PANEL_LABEL_STRING, search,
    /*
     * PANEL_NOTIFY_PROC,      search_cmd_proc,
     */
			  PANEL_EVENT_PROC, search_event_proc,
			  PANEL_ITEM_MENU, direction_menu,
			  HELP_INFO("textsw:find")
			  0);

 /*  if (multibyte)
        search_panel_items[(int) FIND_STRING_ITEM] =
	    panel_create_item(panel, PANEL_TEXT,
			  PANEL_LABEL_Y, ATTR_ROW(0),
			  PANEL_VALUE_DISPLAY_LENGTH, MAX_DISPLAY_LENGTH,
			  PANEL_VALUE_STORED_LENGTH, MAX_STR_LENGTH,
			  PANEL_LABEL_STRING, "   :",
			  PANEL_VALUE_WCS, search_string,
			  HELP_INFO("textsw:findstring")
			  0);
    else */
        search_panel_items[(int) FIND_STRING_ITEM] =
	    panel_create_item(panel, PANEL_TEXT,
			  PANEL_LABEL_Y, ATTR_ROW(0),
			  PANEL_VALUE_DISPLAY_LENGTH, MAX_DISPLAY_LENGTH,
			  PANEL_VALUE_STORED_LENGTH, MAX_STR_LENGTH,
			  PANEL_LABEL_STRING, "   :",
			  PANEL_VALUE, search_string,
			  HELP_INFO("textsw:findstring")
			  0);


    search_panel_items[(int) REPLACE_ITEM] =
	panel_create_item(panel, PANEL_BUTTON,
			  PANEL_LABEL_X, ATTR_COL(0),
			  PANEL_LABEL_Y, ATTR_ROW(1),
			  PANEL_LABEL_STRING, replace,
			  PANEL_NOTIFY_PROC, search_cmd_proc,
			  HELP_INFO("textsw:replace")
			  0);


    search_panel_items[(int) REPLACE_STRING_ITEM] = panel_create_item(panel, PANEL_TEXT,
						 PANEL_LABEL_Y, ATTR_ROW(1),
			     PANEL_VALUE_DISPLAY_LENGTH, MAX_DISPLAY_LENGTH,
				  PANEL_VALUE_STORED_LENGTH, MAX_STR_LENGTH,
						    PANEL_LABEL_STRING, ":",
					   HELP_INFO("textsw:replacestring")
								      0);


    search_panel_items[(int) FIND_THEN_REPLACE_ITEM] =
	panel_create_item(panel, PANEL_BUTTON,
			  PANEL_LABEL_X, ATTR_COL(0),
			  PANEL_LABEL_Y, ATTR_ROW(2),
			  PANEL_LABEL_STRING, search_replace,
			  PANEL_NOTIFY_PROC, search_cmd_proc,
			  HELP_INFO("textsw:findreplace")
			  0);

    search_panel_items[(int) REPLACE_THEN_FIND_ITEM] =
	panel_create_item(panel, PANEL_BUTTON,
			  PANEL_LABEL_STRING, replace_search,
			  PANEL_NOTIFY_PROC, search_cmd_proc,
			  HELP_INFO("textsw:replacefind")
			  0);

    search_panel_items[(int) REPLACE_ALL_ITEM] =
	panel_create_item(panel, PANEL_BUTTON,
			  PANEL_LABEL_STRING, all,
			  PANEL_NOTIFY_PROC, search_cmd_proc,
			  HELP_INFO("textsw:replaceall")
			  0);

    search_panel_items[(int) WRAP_ITEM] =
	panel_create_item(panel, PANEL_CYCLE,
			  PANEL_CHOICE_STRINGS, 
			      XV_MSG("All Text"), 
			      XV_MSG("To End"), 
			  0,
			  HELP_INFO("textsw:wrap")
			  0);

    /* this just works in both cases because we init the string to null. */
    if (search_string[0] != NULL)
	xv_set(panel, PANEL_CARET_ITEM,
	       search_panel_items[(int) REPLACE_STRING_ITEM], 0);
    else {
	xv_set(panel, PANEL_CARET_ITEM,
	       search_panel_items[(int) FIND_STRING_ITEM], 0);
    }

}
Pkg_private	Panel
textsw_create_search_panel(frame, view)
    Frame           frame;
    Textsw_view_handle view;
{
    Panel           panel;

    panel = (Panel) xv_get(frame, FRAME_CMD_PANEL,
			   HELP_INFO("textsw:searchpanel")
			   0);
    (void) create_search_items(panel, view);

    return (panel);
}
