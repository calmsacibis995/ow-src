#ifndef lint
static  char sccsid[] = "@(#)undelete.c 3.9 93/04/08 Copyr 1987 Sun Micro";
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
 * Mailtool - Mail subprocess handling
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdio.h>
#include <rpcsvc/ypclnt.h>

#include <xview/panel.h>
#include <xview/text.h>
#include <xview/font.h>
#include <xview/xview.h>
#include <xview/selection.h>
#include <xview/sel_svc.h>
#include <xview/sel_attrs.h>
#include <xview/svrimage.h>

#include "tool.h"
#include "tool_support.h"
#include "glob.h"
#include "cmds.h"
#include "header.h"
#include "ds_popup.h"
#include "instrument.h"
#include "mle.h"

Frame	mt_undel_frame;
static	Panel		undel_panel;
static	Panel_item	undel_list_item;
static	Panel_item	undel_button;

mt_undel_selected_items(item, event)

        Panel_item      item;
        Event           *event;
{
	register int i;
	struct msg *m = NULL;
	struct msg	*curmsg;
	struct header_data	*hd;

	hd = mt_get_header_data(item);
	curmsg = MT_CURMSG(hd);

	i = (int) xv_get(undel_list_item, PANEL_LIST_NROWS);
	while (--i >= 0)
	{
		if (xv_get(undel_list_item, PANEL_LIST_SELECTED, i)) {
			m = (struct msg *)xv_get(undel_list_item,
						PANEL_LIST_CLIENT_DATA, i);

			mt_do_undel(hd, m);

			xv_set(undel_list_item, PANEL_LIST_DELETE, i, 0);
			curmsg->mo_selected = 1;
		}
	}

	/* no msg is undeleted */
	if (m == NULL)
		return;

	mt_update_msgsw(m, FALSE, TRUE, TRUE, mt_get_header_data(MT_FRAME(hd)));

        mt_update_folder_status(hd);
}

mt_undel_list_clear()

{
	mt_clear_list(undel_list_item);
}


void
mt_undel_list_init(hd)

        struct header_data *hd;
{
	Notify_value	undel_event_proc();
	char *undelete;

	mt_undel_frame = xv_create(MT_FRAME(hd), FRAME_CMD, 
		                XV_KEY_DATA, KEY_HEADER_DATA, hd,
				WIN_IS_CLIENT_PANE,
				FRAME_CMD_PUSHPIN_IN,	TRUE,
				FRAME_SHOW_LABEL,	TRUE,
				FRAME_SHOW_FOOTER,	TRUE,
				FRAME_SHOW_RESIZE_CORNER,	TRUE,
				XV_SHOW,		FALSE,
#ifdef OW_I18N
				WIN_USE_IM,		FALSE,
#endif
				0);

        /* STRING_EXTRACTION -
         *
         * The name of the undelete panel, as well as the name of the
         * button.
         */
	undelete = gettext("Undelete");

	mt_label_frame(mt_undel_frame, undelete);

	(void)notify_interpose_event_func(mt_undel_frame,
					undel_event_proc, NOTIFY_SAFE);

	undel_panel = xv_get(mt_undel_frame, FRAME_CMD_PANEL, 0);
	xv_set(undel_panel,
		WIN_COLUMNS,	45,
		WIN_ROWS,	5,
		0);

	undel_list_item = xv_create(undel_panel, PANEL_LIST,
				PANEL_CHOOSE_ONE, FALSE,
				PANEL_CHOOSE_NONE, FALSE,
				PANEL_READ_ONLY, TRUE,
				XV_X,		xv_col(undel_panel, 0),
				XV_Y,		xv_row(undel_panel, 0),
				XV_HELP_DATA,	"mailtool:UndeleteMsgs",
				0);

	undel_button = xv_create(undel_panel, PANEL_BUTTON,
		                XV_KEY_DATA, KEY_HEADER_DATA, hd,
				PANEL_LABEL_STRING,	undelete,
				PANEL_NOTIFY_PROC,	mt_undel_selected_items,
				XV_HELP_DATA,	"mailtool:Undelete",
				0);

	undel_resize(hd);
	window_fit(mt_undel_frame);
}

mt_undel_list_show(menu, menu_item)

	Menu		menu;
	Menu_item	menu_item;

{
	static short	previously_shown;
        struct header_data *hd;

	TRACK_BUTTON(menu, menu_item, "undelete_from_list");

        hd = mt_get_header_data(menu);
	if (!mt_undel_frame)
		mt_undel_list_init(hd);

	if (!previously_shown) {
		ds_position_popup(MT_FRAME(hd), mt_undel_frame, DS_POPUP_LOR);
		previously_shown = TRUE;
	}

	wmgr_top(mt_undel_frame);
	xv_set(mt_undel_frame, XV_SHOW, TRUE, WIN_FRONT, 0);
}

mt_undel_list_add_member(hd, m)

	struct header_data *hd;
	struct msg *m;

{
	char *strbuf;

	if (undel_list_item)
	{
		/* Need to copy the string, since it is freed when the
		 * item is deleted from the list.
		 */
		strbuf = ck_strdup(m->m_header);
		xv_set(undel_list_item, 
			PANEL_LIST_INSERT, 0,
			PANEL_LIST_STRING, 0, strbuf,
			PANEL_LIST_CLIENT_DATA, 0, m,
			/* Use a fixed width font so columns look good */
			PANEL_LIST_FONT, 0, hd->hd_textfont,
			0);
	}
}

mt_remove_last_from_undel_list()

{
	xv_set(undel_list_item, PANEL_LIST_DELETE, 0, 0);
}

static Notify_value
undel_event_proc(frame, event, arg, type)

	Frame	frame;
	Event	*event;
	Notify_arg	arg;
	Notify_event_type	type;
{
	Notify_value	value;
	struct header_data	*hd;

	hd = mt_get_header_data(frame);
	value = notify_next_event_func(frame, (Notify_event) event, arg, type);

	if (event_action(event) == WIN_RESIZE)
		undel_resize(hd);

	return(value);

}


static
undel_resize(hd)

	struct header_data	*hd;
{
	int	panel_h, panel_w;
	int	rows;
	Rect *	rect;


	/*
	 * The frame has been re-sized.  Layout the pop-up.  If we didn't
	 * do this then the bottom panel would change in size instead of
	 * the canvas
	 */
	panel_h = (int)xv_get(undel_panel, XV_HEIGHT);
	panel_w = (int)xv_get(undel_panel, XV_WIDTH);

	rect = (Rect *)xv_get(undel_button, PANEL_ITEM_RECT);

	rows = (panel_h - 2 * rect->r_height) / (int)xv_get(undel_list_item,
						PANEL_LIST_ROW_HEIGHT);
	rows--;

	if (rows < 0)
		rows = 1;

	/*
	 * Set the list box dimensions.  Subtract the width of the scrollbar.
	 * We actually subtract the width of the headr canvas scrollbar since
	 * I'm not sure how to get ahold of the list box scrollbar, but they
	 * should be the same width.
	 */
	xv_set(undel_list_item,
		XV_X,			0,
		XV_Y,			0,
		PANEL_LIST_DISPLAY_ROWS,	rows,
		PANEL_LIST_WIDTH,
			panel_w - mt_get_scrollbar_width(hd->hd_canvas),
		0);

	xv_set(undel_button,
		XV_Y, panel_h - rect->r_height - rect->r_height / 4,
		XV_X, (panel_w - rect->r_width) / 2,
		0);
}

mt_clear_list(list)

	Panel_item	list;

{
	int	nrows;

	if (list) {
		nrows = (int) xv_get(list, PANEL_LIST_NROWS);
		(void)xv_set(list, PANEL_LIST_DELETE_ROWS, 0, nrows, 0);
	}
}
