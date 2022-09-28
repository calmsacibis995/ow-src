#ifndef lint
static  char sccsid[] = "@(#)repeat.c 1.11 94/09/01 Copyr 1991 Sun Microsystems, Inc.";
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
#include "calendar.h"
#include "ds_popup.h"
#include "repeat.h"
#include "gettext.h"
#include "editor.h"
#include "misc.h"

static Notify_value
repeat_done_proc(frame)
	Frame frame;
{
	xv_set(frame, XV_SHOW, FALSE, NULL);
        return NOTIFY_DONE;
}
static Notify_value
apply_proc(item, event)
	Panel_item item;
	Event *event;
{
	Calendar *c = calendar;
	Editor *e = (Editor*)c->editor;
	Repeat *r = (Repeat*)e->repeat;
	int val=0;
	Interval unit;
	char buf[80];

	xv_set(r->frame, FRAME_LEFT_FOOTER, "", NULL);
	if ((val = (int)atoi((char*)xv_get(r->repeatunit, PANEL_VALUE))) < 1) {
		notice_prompt(r->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRING,
			MGET("Invalid Entry. Enter a Number."),
			NOTICE_BUTTON_YES,
			LGET("Continue") ,
		NULL);
		xv_set(r->frame, FRAME_LEFT_FOOTER, MGET("Invalid Entry. Enter a Number."), NULL);
        	return(NOTIFY_DONE);
	}
        
	unit = repeatstr_to_interval(
		(char*)xv_get(r->repeatunitmessage, PANEL_LABEL_STRING));
	sprintf(buf, "%s %d %s", MGET("Every"), val, periodstr[unit]);
	e->nthval = val;
	e->periodval = unit;
	xv_set(e->periodunit, PANEL_LABEL_STRING, buf, NULL);
	activate_scope(e);
	xv_set(e->scope, PANEL_VALUE, repeatval[(int)e->periodval], 0);
        xv_set(e->scopeunit, PANEL_LABEL_STRING, repeatstr[(int)e->periodval], 0);

        return(NOTIFY_DONE);
}
static void
repeat_set_unit(m, mi)
        Menu m;
        Menu_item mi;
{
        Repeat *r;

        r = (Repeat*)xv_get(m, MENU_CLIENT_DATA);
        r->repeatunitval = (int)xv_get(mi, MENU_VALUE);
        xv_set(r->repeatunitmessage, PANEL_LABEL_STRING,
                (char*)xv_get(mi, MENU_STRING), NULL);
	xv_set(m, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
}

static Menu
repeatunit_menu(r)
        Repeat *r;
{
        Menu menu;

        if (r==NULL) return(NULL);

        menu = menu_create(
                MENU_CLIENT_DATA, r,
                MENU_NOTIFY_PROC, repeat_set_unit,
                MENU_NOTIFY_STATUS, XV_ERROR,
                MENU_ITEM,
                        MENU_STRING,  periodstr[everyNthDay],
                        MENU_VALUE, 7,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[everyNthWeek],
                        MENU_VALUE, 8,
                        0,
                MENU_ITEM,
                        MENU_STRING,  periodstr[everyNthMonth],
                        MENU_VALUE, 9,
                        0,
                XV_HELP_DATA, "cm:RepeatUnitMenu",
                0);

        return(menu);
}


extern caddr_t
make_repeat(c)
	Calendar *c;
{
	Repeat *r;
	Panel panel;
	Panel_item apply_button;
	int int_buf;
	Editor *e = (Editor*)c->editor;
	Xv_Font pf = xv_get(c->frame, XV_FONT);
	Font_string_dims dims;

	if (e->repeat == NULL) 
		e->repeat = (caddr_t)ckalloc(sizeof(Repeat));
	r = (Repeat*)e->repeat;

	r->repeatunitval = 7;
	r->frame = xv_create(e->frame, FRAME_CMD,
		FRAME_INHERIT_COLORS, TRUE,
                FRAME_SHOW_LABEL, TRUE,
                FRAME_CMD_PUSHPIN_IN, FALSE,
                FRAME_SHOW_FOOTER, TRUE,
                FRAME_DONE_PROC, repeat_done_proc,
		WIN_CMS,     (Cms)xv_get(c->frame, WIN_CMS),
		WIN_USE_IM, FALSE,
		XV_LABEL,  MGET("CM: Repeat Every") ,
		XV_HELP_DATA, "cm:RepeatHelp",
		NULL);

	panel = xv_get(r->frame, FRAME_CMD_PANEL);

	xv_set(panel, 
                XV_X, 0,
                XV_Y, 1,
                PANEL_CLIENT_DATA, c,
		XV_HELP_DATA, "cm:RepeatHelp",
                0);

	int_buf = xv_row(panel, 1);
	r->repeatunit = xv_create(panel, PANEL_TEXT,
                PANEL_LABEL_STRING,  LGET("Repeat Every:") ,
                PANEL_LABEL_BOLD, TRUE,
                PANEL_VALUE_DISPLAY_LENGTH, 8,
                PANEL_VALUE_STORED_LENGTH, 10,
		PANEL_VALUE, "5",
                PANEL_NOTIFY_PROC, apply_proc,
                XV_X, 15,
                XV_Y, int_buf,
                XV_HELP_DATA, "cm:Repeatinterval",
                0);
	
	r->repeat_menu = xv_create(panel, PANEL_ABBREV_MENU_BUTTON,
                XV_X, xv_get(r->repeatunit, XV_X) + 
			xv_get(r->repeatunit, XV_WIDTH) + 7,
                XV_Y, int_buf,
                PANEL_ITEM_MENU, repeatunit_menu(r),
		PANEL_VALUE, periodstr[(int)r->repeatunitval],
                XV_HELP_DATA, "cm:RepeatUnitMenu",
                0);

	r->repeatunitmessage = xv_create(panel, PANEL_MESSAGE,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, xv_get(r->repeat_menu, XV_X) + 
			xv_get(r->repeat_menu, XV_WIDTH) + 10,
                XV_Y, int_buf,
		PANEL_LABEL_STRING, periodstr[(int)r->repeatunitval],
                XV_HELP_DATA, "cm:RepeatUnitMenu",
                0);

	apply_button = xv_create(panel, PANEL_BUTTON,
                PANEL_LABEL_BOLD, TRUE,
                PANEL_LABEL_STRING,  LGET("  Apply  "),
                PANEL_NOTIFY_PROC, apply_proc,
                XV_Y, xv_row(panel, 2),
                XV_X, 100,
                XV_HELP_DATA, "cm:RepeatApply",
                0);


	(void)xv_get(pf, FONT_STRING_DIMS, repeatstr[7], &dims);
	int_buf = dims.width;
	(void)xv_get(pf, FONT_STRING_DIMS, repeatstr[8], &dims);
	if (int_buf < dims.width)
		int_buf = dims.width;
	(void)xv_get(pf, FONT_STRING_DIMS, repeatstr[9], &dims);
	if (int_buf < dims.width)
		int_buf = dims.width;

	xv_set(panel, XV_WIDTH, xv_get(r->repeatunitmessage, XV_X) +
		int_buf + 20, NULL);
	xv_set(panel, XV_HEIGHT, xv_get(apply_button, XV_Y) +
		xv_get(apply_button, XV_HEIGHT) + 15, NULL);

	window_fit(r->frame);

	ds_position_popup(c->frame, r->frame, DS_POPUP_LOR);

	return (caddr_t)r;
}
extern void
show_repeat(m , mi)
        Menu m;
        Menu_item mi;
{
        Calendar *c = calendar;
        Editor *e = (Editor*)c->editor;
        Repeat *r = (Repeat*)e->repeat;

        if (r == NULL || r->frame == NULL) {
                e->repeat = (caddr_t)make_repeat(c);
                r = (Repeat*)e->repeat;
        }
        (void)xv_set(r->frame, XV_SHOW, TRUE, NULL);
}
