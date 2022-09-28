#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)header_selection.c 3.8 93/05/19 Copyr 1987 Sun Micro";
#endif
#endif

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

/*
 * Mailtool - tool command handling
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef SVR4
#include <sys/kbd.h>
#include <sys/kbio.h>
#else
#include <sundev/kbd.h>
#endif SVR4

#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/text.h>
#include <xview/font.h>
#include <xview/selection.h>
#include <xview/sel_svc.h>
#include <xview/sel_attrs.h>
#include <xview/scrollbar.h>

#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "header.h"
#include "mail.h"
#include "create_panels.h"
#include "mle.h"
#include "ds_popup.h"
#include "../maillib/assert.h"

#define TO_OR_CC_STRING		"to/cc"

#ifdef	CONTEXT_SEARCH
#define	CONTEXT_STRING		"context"
#define	MINUS			'-'
typedef	short	BOOL;
#endif	CONTEXT_SEARCH


char	*mt_get_char_field();
int	mt_get_int_field();

static struct msg *find_start_index();

static
contains(large_string, small_string)
char  *large_string;
char  *small_string;

{
        short   length1;
        short   length2;
        short   i;
  
        length1 = strlen(large_string);
        length2 = strlen(small_string);
  
        for (i = 0; i <= (length1 - length2); i++)
        {
                if (strncasecmp(large_string, small_string, length2) == 0)
                        return(TRUE);
  
                large_string++;
        }
  
        return(FALSE);
}

static
match_criteria(panel, m)

	Panel	panel;
	struct msg	*m;
{
	Panel_item	item;
	char		*p;
	char		*field;
	char		*search_str;


	/* Loop through all items in the panel */
	PANEL_EACH_ITEM(panel, item) {

		/* Skip anything which is not a visible text field */
		if (!xv_get(item, PANEL_SHOW_ITEM) ||
		   ((Panel_item_type) xv_get(item, PANEL_ITEM_CLASS) !=
							PANEL_TEXT_ITEM)) {
				continue;
		}

		/* Get the string to match */
		search_str = (char *)xv_get(item, PANEL_VALUE);

		/* Emptry field -- skip it */
		if (*search_str == '\0')
			continue;

		/* Get the field name */
		if ((field = (char *)xv_get(item, PANEL_CLIENT_DATA)) == NULL)
			continue;

		/* Special case the "To/Cc" field */
		if (strcmp(field, TO_OR_CC_STRING) == 0) {

			/* Must match To or Cc field */
			p = mt_get_char_field("to", m, FALSE);
			if (contains(p, search_str)) {
				ck_free(p);
				continue; /* Match. Go to next field in popup */
			}
			ck_free(p);

			p = mt_get_char_field("cc", m, FALSE);
			if (contains(p, search_str)) {
				ck_free(p);
				continue; /* Match. Go to next field */
			}
			ck_free(p);

#ifdef	CONTEXT_SEARCH
		} else if (strcmp(field, CONTEXT_STRING) == 0) {
			/* Shell-like pattern matching for context searching */
			int	insert = 0;

			/* Add the leading and trailing *'s if necessary */
			if (*search_str != '*')
				insert |= 0x1;
			if (((p = strrchr(search_str, '*')) == NULL) ||
			    (p[1] != '\0') || (p == search_str))
				insert |= 0x2;
			if (insert)
			{
				p = (char *)ck_malloc(strlen(search_str) + 3);
				*p = '\0';
				if (insert & 0x1)
					strcat(p, "*");
				strcat(p, search_str);
				if (insert & 0x2)
					strcat(p, "*");
				search_str = p;
			}
			
			if (pmatch(m->mo_body_ptr, m->mo_body_ptr+m->mo_len,
					search_str)) {
				if (insert)
					ck_free(search_str);
				continue; /* Match. Go to next field */
			}
			if (insert)
				ck_free(search_str);
#endif	CONTEXT_SEARCH
		} else {
			/* Field other then "To/Cc". Check for match */
			p = mt_get_char_field(field, m, FALSE);
			if (contains(p, search_str)) {
				ck_free(p);
				continue; /* Match. Go to next field */
			}
			ck_free(p);
		}

		/* Field contained a value but match failed */
		return(FALSE);

	} PANEL_END_EACH;

	return(TRUE);
}


static
clear_find_fields_proc(item, ie)

	Panel_item	item;
	Event		*ie;

{
	Panel	panel;
	void	ds_clear_text_items();

	panel = (Panel)xv_get(item, XV_OWNER);

	ds_clear_text_items(panel);
}


mt_do_header_selection_proc(item, ie)

	Panel_item	item;
	Event		*ie;

{
	register struct msg *m;
	register struct msg *first_item_selected = NULL;
	register struct msg *start_index;
	register struct msg *limit;
	register struct msg *try;
	register int	inc;
	register int	items_selected = 0;
	register short	wrapped;
	Panel		panel;
	int	select;
	char	*ptr;
	int	base_line;
	int	selected_items_onscreen = 0;
	char	confirm_msg[64];
	int	headerlines;
	struct	header_data *hd;


	hd = mt_get_header_data(xv_get(item, PANEL_PARENT_PANEL));

	base_line = (int)xv_get(hd->hd_scrollbar, SCROLLBAR_VIEW_START) + 1;

	/*
	 * If user pressed the Select key then we want to select all messages
	 * which match the criteria.  If the user pressed the Find key then
	 * we want to go to the next message which matches the criteria
	 */
	select = (item == hd->hd_findselect);

	/*
	 * Clear "n items selected" message on left footer of popup
	 */
        xv_set(hd->hd_findframe, FRAME_LEFT_FOOTER, "", 0);

	acquire_selection(SELN_PRIMARY);

	headerlines = (int)xv_get(hd->hd_canvas, XV_HEIGHT) / hd->hd_lineheight;

	/*
	 * Set loop limits depending on if we are selecting, searching
	 * forward or searching backward
	 */
	if (select) {
		inc = 1;
		limit = LAST_NDMSG(CURRENT_FOLDER(hd));
		start_index = FIRST_NDMSG(CURRENT_FOLDER(hd));
	} else if (item == hd->hd_findforward) {
		inc = 1;
		limit = LAST_NDMSG(CURRENT_FOLDER(hd));
		start_index = find_start_index(hd);
		if (start_index) start_index = NEXT_NDMSG(start_index);
		if (start_index == NULL)
			start_index = FIRST_NDMSG(CURRENT_FOLDER(hd));
	} else {
		inc = -1;
		limit = FIRST_NDMSG(CURRENT_FOLDER(hd));
		start_index = find_start_index(hd);
		if (start_index) start_index = PREV_NDMSG(start_index);
		if (start_index == NULL)
			start_index = msg_methods.mm_last(CURRENT_FOLDER(hd));
	}

	/* check for no messages available */
	if (! start_index) return;

	mt_save_curmsgs(hd);

	wrapped = FALSE;
	m = start_index;
	panel = (Panel)xv_get(item, XV_OWNER);
	for( ;; ) {

		if( inc > 0 ) {
			try = NEXT_NDMSG( limit );
		} else {
			try = PREV_NDMSG( limit );
		}
		if (m == try) break;

		if (!match_criteria(panel, m))
			goto CONTINUE;


		/*
		 * As soon as we hit something to select, de-select
		 * previous selections
		 */
		if (first_item_selected == NULL)
			mt_clear_selected_glyphs(hd);

		mt_toggle_glyph(m->m_lineno, hd);
		items_selected++;

		if ((m->m_lineno >= base_line) &&
		    (m->m_lineno < (base_line + headerlines)))
			selected_items_onscreen++;

		if (first_item_selected == NULL) {
			first_item_selected = m;
			if (!select)
				break;
		}

CONTINUE:
		/*
		 * If at end of list and not selecting, wrap.
		 */
		 if (m == limit && !select && !wrapped) {
			if (inc > 0) {
				m = FIRST_NDMSG(CURRENT_FOLDER(hd));
			} else {
				m = LAST_NDMSG(CURRENT_FOLDER(hd));
			}
			limit = start_index;
			wrapped = TRUE;
		} else {
			if( inc > 0 ) {
				m = NEXT_NDMSG( m );
			} else {
				m = PREV_NDMSG( m );
			}
		}
	}

	/* if none of the selected items were onscreen, 
	 * scroll the header subwindow so that the 
	 * first on is visible.
	 */

	if (items_selected && !selected_items_onscreen) {
		mt_update_headersw(first_item_selected, hd);
	}

	if (items_selected) {
		mt_activate_functions();

		if (select) {
                        /* STRING_EXTRACTION -
                         *
                         * The user hit "select all" from the find menu.
                         * This is the status message that is displayed in
                         * the footer pane.  %d is the number of messages
                         * that were found.
                         */
			sprintf(confirm_msg, gettext("%d messages selected"),
				items_selected);
			xv_set(hd->hd_findframe, FRAME_LEFT_FOOTER,
				confirm_msg, 0);
		} else if (mt_view_windows_up(hd)) {
			mt_create_views(hd, TRUE, MSG_SELECTED);
		}
	} else {
                /* STRING_EXTRACTION -
                 *
                 * The selection failed: the status message in the find
                 * window footer pane is "No Match".
                 */
		xv_set(hd->hd_findframe, FRAME_LEFT_FOOTER,
			gettext("No match"), 0);
	}
}

static Panel_setting
text_notify_proc(item, event)

	Panel_item	item;
	Event		*event;

{
	struct header_data *hd;

	hd = mt_get_header_data(xv_get(item, PANEL_PARENT_PANEL));

	/*
	 * If field is empty advance to next field.  Else do find
	 */
	if (*((char *)xv_get(item, PANEL_VALUE)) == '\0') {
		return(panel_text_notify(item, event));
	} else {
		mt_do_header_selection_proc(hd->hd_findforward, event);
	}

	return (PANEL_NONE);
}


mt_start_header_selection_proc(hd)

struct header_data *hd;

{
	Panel	selection_panel;
	Panel_item	clear_item, backward_item;
	int		row;
	int		min_cols = 40;

	if (!hd->hd_findframe)
	{

		/* create a popup frame for the header selection, and 
		   assemble the panel */
	
		hd->hd_findframe = xv_create(MT_FRAME(hd), FRAME_CMD,
			WIN_IS_CLIENT_PANE,
			FRAME_CMD_PUSHPIN_IN,	TRUE,
			FRAME_SHOW_FOOTER,	TRUE,
			FRAME_SHOW_LABEL,	TRUE,
			XV_SHOW,		FALSE,
			0);

		mt_window_header_data(hd, hd->hd_findframe);
		
                /* STRING_EXTRACTION -
                 *
                 * The frame title for the find window: Find Messages
                 */
		mt_label_frame(hd->hd_findframe, gettext("Find Messages"));

		selection_panel = xv_get(hd->hd_findframe, FRAME_CMD_PANEL,0);
		mt_window_header_data(hd, selection_panel);

		(void)xv_set(selection_panel,
			WIN_BORDER,	TRUE,
			PANEL_ITEM_Y_GAP, (int)xv_get(selection_panel, 
							PANEL_ITEM_Y_GAP) / 2,
			0);
	
                /* STRING_EXTRACTION -
                 *
                 * The find panel item titles: Sender, Subject
                 */
		row = 0;
		(void)xv_create(selection_panel, PANEL_TEXT,
				PANEL_LABEL_STRING,	gettext("From:"),
				PANEL_CLIENT_DATA,	"from",
				PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
				PANEL_NOTIFY_STRING,	"\r",
				PANEL_NOTIFY_PROC,	text_notify_proc,
				PANEL_VALUE_DISPLAY_LENGTH,	min_cols,
				XV_X,		0,
				XV_Y,	 	xv_row(selection_panel, row), 
				XV_HELP_DATA,   "mailtool:SelectBySender",
				0);

		row++;
		(void)xv_create(selection_panel, PANEL_TEXT,
				PANEL_LABEL_STRING,	gettext("To/Cc:"),
				PANEL_CLIENT_DATA,	TO_OR_CC_STRING,
				PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
				PANEL_NOTIFY_STRING,	"\r",
				PANEL_NOTIFY_PROC,	text_notify_proc,
				PANEL_VALUE_DISPLAY_LENGTH,	min_cols,
				XV_X,		0,
				XV_Y,	 	xv_row(selection_panel, row), 
				XV_HELP_DATA,   "mailtool:SelectByToCc",
				0);

		row++;
		(void)xv_create(selection_panel, PANEL_TEXT,
				PANEL_LABEL_STRING,	gettext("To:"),
				PANEL_CLIENT_DATA,	"to",
				PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
				PANEL_NOTIFY_STRING,	"\r",
				PANEL_NOTIFY_PROC,	text_notify_proc,
				PANEL_VALUE_DISPLAY_LENGTH,	min_cols,
				XV_X,		0,
				XV_Y,	 	xv_row(selection_panel, row), 
				XV_HELP_DATA,   "mailtool:SelectByTo",
				0);

		row++;
		(void)xv_create(selection_panel, PANEL_TEXT,
				PANEL_LABEL_STRING,	gettext("Cc:"),
				PANEL_CLIENT_DATA,	"cc",
				PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
				PANEL_NOTIFY_STRING,	"\r",
				PANEL_NOTIFY_PROC,	text_notify_proc,
				PANEL_VALUE_DISPLAY_LENGTH,	min_cols,
				XV_X,		0,
				XV_Y,	 	xv_row(selection_panel, row), 
				XV_HELP_DATA,   "mailtool:SelectByCc",
				0);

		row++;
		(void)xv_create(selection_panel, PANEL_TEXT,
				PANEL_LABEL_STRING,	gettext("Subject:"),
				PANEL_CLIENT_DATA,	"subject",
				PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
				PANEL_NOTIFY_STRING,	"\r",
				PANEL_NOTIFY_PROC,	text_notify_proc,
				PANEL_VALUE_DISPLAY_LENGTH,	min_cols,
				XV_X,		0,
				XV_Y,	 	xv_row(selection_panel, row), 
				XV_HELP_DATA,   "mailtool:SelectBySubject",
				0);

#ifdef	CONTEXT_SEARCH
		row++;
		(void)xv_create(selection_panel, PANEL_TEXT,
				PANEL_LABEL_STRING,	gettext("Context:"),
				PANEL_CLIENT_DATA,	CONTEXT_STRING,
				PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
				PANEL_NOTIFY_STRING,	"\r",
				PANEL_NOTIFY_PROC,	text_notify_proc,
				PANEL_VALUE_DISPLAY_LENGTH,	min_cols,
				XV_X,		0,
				XV_Y,		xv_row(selection_panel, row), 
				XV_HELP_DATA,	"mailtool:SelectByContents",
				0);
#endif	CONTEXT_SEARCH

                /* STRING_EXTRACTION -
                 *
                 * The three find action buttons: Find Forward, Find Backward,
                 * and Select All, plus the button to clear all of the find
		 * fields: Clear
                 */

		row++;
		hd->hd_findforward = xv_create(selection_panel, PANEL_BUTTON,
				PANEL_LABEL_STRING, gettext("Find Forward"),
				PANEL_NOTIFY_PROC,  mt_do_header_selection_proc,
				XV_X,		0,
				XV_Y,	 	xv_row(selection_panel, row), 
				XV_HELP_DATA,   "mailtool:FindForwardButton",
				0);

		(void)xv_set(selection_panel, PANEL_DEFAULT_ITEM,
				hd->hd_findforward,
				0);

		backward_item = xv_create(selection_panel, PANEL_BUTTON,
				PANEL_LABEL_STRING, gettext("Find Backward"),
				PANEL_NOTIFY_PROC,  mt_do_header_selection_proc,
				XV_HELP_DATA,   "mailtool:FindBackwardButton",
				0);

		hd->hd_findselect = xv_create(selection_panel, PANEL_BUTTON,
				PANEL_LABEL_STRING, gettext("Select All"),
				PANEL_NOTIFY_PROC,  mt_do_header_selection_proc,
				XV_HELP_DATA,   "mailtool:SelectButton",
				0);

		clear_item =(Panel_item)xv_create(selection_panel, PANEL_BUTTON,
				PANEL_LABEL_STRING, gettext("Clear"),
				PANEL_NOTIFY_PROC,  clear_find_fields_proc,
				XV_HELP_DATA,   "mailtool:ClearButton",
				0);

		window_fit(selection_panel);
		ds_justify_items(selection_panel, TRUE);
		ds_center_items(selection_panel, row, hd->hd_findforward,
			backward_item, hd->hd_findselect, clear_item, 0);
		window_fit(hd->hd_findframe);
		ds_position_popup(MT_FRAME(hd), hd->hd_findframe, DS_POPUP_LOR);
	}

	xv_set(hd->hd_findframe,
		FRAME_CMD_PUSHPIN_IN,	TRUE,
		XV_SHOW, TRUE,
		WIN_FRONT,
		0);
}


static struct msg *
find_start_index(hd)
	struct header_data *hd;
{
	register struct msg *m;
	struct msg *index;

	/*
	 * Find out where we should start the find.  Find should start at
	 * the first selected message. If no selected message it should start at
	 * the current message.  If no current message it should
	 * start at message 1.
	 */
	index = NULL;
	for (m = FIRST_NDMSG(CURRENT_FOLDER(hd)); m != NULL; m = NEXT_NDMSG( m ))
	{
		if (m->mo_selected) {
			index = m;
			break;
		}

		if (m->mo_current && (index == NULL))
		{
			index = m;
		}

	}

	if (index != NULL)
		return(index);
	else
		return(FIRST_NDMSG(CURRENT_FOLDER(hd)));
}


/*
 * Return the string value for the specified field.  The returned value
 * should be treated as read-only.  The return value is allocated, so
 * it is the responsibility of the caller to free it later.
 */
char	*
mt_get_char_field(field, m, skip_re)

	char		*field;
	struct msg	*m;
	int		skip_re;

{
	char *value;

	ASSERT(field != NULL);

	value = msg_methods.mm_get(m, MSG_HEADER, field);

	if (strcasecmp(field, "subject") == 0 && value != NULL && skip_re) {
		u_char *newvalue;

		if (strncasecmp(value, "Re:", 3) == 0) {
			newvalue = (u_char *) value + 3;
			while (isspace(*newvalue)) {
				newvalue++;
			}

			newvalue = (u_char *) strdup((char *)newvalue);
			ck_free(value);
			value = (char *) newvalue;
		}
	}

	if (value == NULL) {
		value = strdup("");
	}

	return(value);
}

/*
 * Return the integer value for the specified field. 
 * Right now this supports:
 *	date
 *	size
 *	status
 *	msg_number
 * Anything else returns -1
 * 
 */
mt_get_num_field(field, m)

	char		*field;
	struct msg	*m;

{
	int	value;

	if (strcasecmp(field, "date") == 0) {
		value = get_time(m);
	} else if (strcasecmp(field, "msg_number") == 0) {
		value = (int)m->mo_msg_number;
	} else if (strcasecmp(field, "size") == 0) {
		value = (int) msg_methods.mm_get(m, MSG_NUM_BYTES);
	} else if (strcasecmp(field, "status") == 0) {
		if (m->mo_new)
			value = 2;
		else if (m->mo_read)
			value = 0;
		else
			value = 1;
	} else
		value = -1;

	return(value);
}

#ifdef	CONTEXT_SEARCH
/*
 * Shell pattern maching function (derived from Unix shell):
 *
 * "*" in params matches r.e ".*"
 * "?" in params matches r.e. "."
 * "[...]" in params matches character class
 * "[...a-z...]" in params matches a through z.
 */

static int
pmatch(s, e, p)
register unsigned char	*s, *e, *p;
{
	register unsigned char scc;
	unsigned char c;

	if (s >= e)
		return(0);

	scc = *s++;
	switch (c = *p++)
	{
	case '[':
		{
			BOOL ok;
			int lc = -1;
			int notflag = 0;

			ok = 0;
			if (*p == '!')
			{
				notflag = 1;
				p++;
			}
			while (c = *p++)
			{
				if (c == ']')
					return(ok ? pmatch(s, e, p) : 0);
				else if (c == MINUS && lc > 0 && *p!= ']')
				{
					if (notflag)
					{
						if (scc < (u_char) lc ||
						    scc > *(p++))
							ok++;
						else
							return(0);
					}
					else
					{
						if ((u_char) lc <= scc &&
						    scc <= (*p++))
							ok++;
					}
				}
				else
				{
					/* skip to quoted character */
					if(c == '\\')
						c = *p++;
					lc = c;
					if (notflag)
					{
						if (scc && scc != lc)
							ok++;
						else
							return(0);
					}
					else
					{
						if (scc == lc)
							ok++;
					}
				}
			}
			return(0);
		}

	case '\\':	
		c = *p++; /* skip to quoted character and see if it matches */
	default:
		if (c != scc)
			return(0);

	case '?':
		return(scc ? pmatch(s, e, p) : 0);

	case '*':
		while (*p == '*')
			p++;

		if (*p == 0)
			return(1);
		--s;
		while (s < e)
		{
			if (pmatch(s++, e, p))
				return(1);
		}
		return(0);

	case 0:
		return(scc == 0);
	}
}
#endif	CONTEXT_SEARCH
