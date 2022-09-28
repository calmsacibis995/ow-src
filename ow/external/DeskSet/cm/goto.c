#ifndef lint
static  char sccsid[] = "@(#)goto.c 1.12 95/08/18 Copyr 1991 Sun Microsystems, Inc.";
#endif
/*
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */

#include <stdio.h>
#include <xview/xview.h>
#include <xview/font.h>
#include <xview/cms.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/xv_xrect.h>
#include <xview/notice.h>
#include "appt.h"
#include "util.h"
#include "timeops.h"
#include "graphics.h"
#include "datefield.h"
#include "props.h"
#include "calendar.h"
#include "ds_popup.h"
#include "goto.h"
#include "select.h"
#include "gettext.h"

static Notify_value
goto_done_proc(frame)
	Frame frame;
{
	xv_set(frame, XV_SHOW, FALSE, NULL);
        return NOTIFY_DONE;
}
static Notify_value
goto_date(item, event)
	Panel_item item;
	Event *event;
{
	Calendar *c;
	Props *p;
	Goto *g;
	Tick gotodate;
	Keyrange keyrange;
	char *date = NULL;

	c = (Calendar*)xv_get(item,  PANEL_CLIENT_DATA);
	p = (Props*)c->properties;
	g = (Goto*)c->goTo;

	date = get_date_str(p, g->datetext);
	if ( date == NULL ) {
		gotodate = -1;
	} else {
		gotodate = cm_getdate(date, NULL);
	}

	if (gotodate <= 0) {
		(void) notice_prompt(c->frame, (Event *)NULL,
		NOTICE_MESSAGE_STRINGS,
		MGET("Invalid Date"),
		0,
		NOTICE_BUTTON_YES, LGET("Continue"),
		NULL);
		xv_set(g->frame, FRAME_LEFT_FOOTER, 
			EGET("Invalid Date"), NULL);
		return;
	}
	xv_set(g->frame, FRAME_LEFT_FOOTER, "", NULL);

	get_range(c->view->glance, c->view->date, &keyrange);
	if (in_range(&keyrange, gotodate)) {
		/* date is in view; deselect and repaint new selection */
		calendar_deselect(c);
		c->view->olddate = c->view->date;
		c->view->date = gotodate;
		paint_selection(c);
	}
	else {
		/* repaint the entire canvas */
		c->view->olddate = c->view->date;
		c->view->date = gotodate;
		gr_clear_area(c->xcontext, 0, 0, 
			xv_get(c->canvas, XV_WIDTH),
		 	xv_get(c->canvas, XV_HEIGHT));
		if (c->view->glance == dayGlance) {
			init_mo(c);
			init_dayview(c);
		}
		paint_canvas(c, NULL);
	}

        return(NOTIFY_DONE);
}

extern caddr_t
make_goto(c)
	Calendar *c;
{
	Goto *g;
	Props *p = (Props*)c->properties;
	Panel panel;
	int row;
	char buf[40];
	Panel_item button;

	if (c->goTo == NULL)
		c->goTo = (caddr_t)ckalloc(sizeof(Goto));
	g = (Goto*)c->goTo;

	g->frame = xv_create(c->frame, FRAME_CMD,
		FRAME_INHERIT_COLORS, TRUE,
                FRAME_SHOW_LABEL, TRUE,
                FRAME_CMD_PUSHPIN_IN, TRUE,
                FRAME_SHOW_FOOTER, TRUE,
                FRAME_DONE_PROC, goto_done_proc,
		WIN_CMS,     (Cms)xv_get(c->frame, WIN_CMS),
                WIN_CLIENT_DATA, c,
		XV_LABEL,  MGET("CM: Go To Date") ,
		XV_HELP_DATA, "cm:GotoHelp",
		NULL);
	panel = xv_get(g->frame, FRAME_CMD_PANEL);

	xv_set(panel, 
                XV_X, 0,
                XV_Y, 1,
                PANEL_CLIENT_DATA, c,
		XV_HELP_DATA, "cm:GotoHelp",
                0);
	row = xv_row(panel, 1); 
	format_tick(time(0), p->ordering_VAL, p->separator_VAL, buf);
	g->datetext = xv_create(panel, PANEL_TEXT,
                PANEL_LABEL_STRING,  LGET("Date:") ,
                PANEL_LABEL_BOLD, TRUE,
                PANEL_VALUE_DISPLAY_LENGTH, 12,
                PANEL_VALUE_STORED_LENGTH, 50,
		PANEL_VALUE, buf,
                PANEL_NOTIFY_PROC, goto_date,
                XV_X, 15,
                XV_Y, row,
                PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:GotoDateHelp",
                0);
	button = xv_create(panel, PANEL_BUTTON,
                PANEL_LABEL_STRING,  LGET("Go To"),
                PANEL_NOTIFY_PROC, goto_date,
		PANEL_CLIENT_DATA, c,
                PANEL_LABEL_BOLD, TRUE,
		XV_Y, row-4,
                XV_X, xv_get(g->datetext, XV_X) +
			 xv_get(g->datetext, XV_WIDTH) + 5,
                XV_HELP_DATA, "cm:GotoAppt",
                0);

        (void)xv_set(panel, PANEL_DEFAULT_ITEM, button, 0);

	window_fit(panel);
	window_fit(g->frame);

	ds_position_popup(c->frame, g->frame, DS_POPUP_LOR);

	return (caddr_t)g;
}
