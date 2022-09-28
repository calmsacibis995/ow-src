#ifndef lint
static  char sccsid[] = "@(#)tempbr.c 1.4 93/01/29 Copyr 1991 Sun Microsystems, Inc.";
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
#include <xview/frame.h>
#include <xview/font.h>
#include <xview/cms.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/xv_xrect.h>
#include <xview/notice.h>
#include "util.h"
#include "timeops.h"
#include "graphics.h"
#include "calendar.h"
#include "ds_popup.h"
#include "tempbr.h"
#include "gettext.h"

static Notify_value
tempbr_done_proc(frame)
	Frame frame;
{
	xv_set(frame, XV_SHOW, FALSE, NULL);
        return NOTIFY_DONE;
}
static Notify_value
temp_browse(item, event)
	Panel_item item;
	Event *event;
{
	Calendar *c;
	Tempbr *t;
	char *name;

	c = (Calendar*)xv_get(item,  PANEL_CLIENT_DATA);
	t = (Tempbr*)c->tempbr;

	name = (char*)xv_get(t->name, PANEL_VALUE);
	if (name == NULL || *name == NULL){
		notice_prompt(t->frame, (Event *)NULL,
                        NOTICE_MESSAGE_STRING,
                        EGET("Specify a User Name."),
                        NOTICE_BUTTON_YES,  LGET("Continue"),
                0);
        	xv_set(t->frame, FRAME_LEFT_FOOTER, 
			EGET("Specify a User Name."), NULL);
		return;
	}
        xv_set(t->frame, FRAME_LEFT_FOOTER, "", NULL);
	switch_it(name, tempbrowser);
        return(NOTIFY_DONE);
}

extern caddr_t
make_tempbr(c)
	Calendar *c;
{
	Tempbr *t;
	Panel panel;
	int row;
	Panel_item button;

	if (c->tempbr == NULL) {
		c->tempbr = (caddr_t)ckalloc(sizeof(Tempbr));
		t = (Tempbr*)c->tempbr;
	}
	else
		t = (Tempbr*)c->tempbr;

	t->frame = xv_create(c->frame, FRAME_CMD,
		FRAME_INHERIT_COLORS, TRUE,
                FRAME_SHOW_LABEL, TRUE,
                FRAME_CMD_PUSHPIN_IN, TRUE,
                FRAME_SHOW_FOOTER, TRUE,
                FRAME_DONE_PROC, tempbr_done_proc,
		WIN_CMS,     (Cms)xv_get(c->frame, WIN_CMS),
                WIN_CLIENT_DATA, c,
		XV_LABEL,  MGET("CM: Show Calendar"),
                0);
	panel = xv_get(t->frame, FRAME_CMD_PANEL);

	xv_set(panel, 
                XV_X, 0,
                XV_Y, 1,
                PANEL_CLIENT_DATA, c,
		XV_HELP_DATA, "cm:tempbr",
                0);
	row = xv_row(panel, 1); 
	t->name = xv_create(panel, PANEL_TEXT,
                PANEL_LABEL_STRING,  LGET("User Name:"),
                PANEL_LABEL_BOLD, TRUE,
                PANEL_VALUE_DISPLAY_LENGTH, 20,
                PANEL_NOTIFY_PROC, temp_browse,
                XV_X, 15,
                XV_Y, row,
                PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:ShowCalname",
                0);

	button = xv_create(panel, PANEL_BUTTON,
                PANEL_LABEL_STRING,  LGET("Show"),
                PANEL_NOTIFY_PROC, temp_browse,
		PANEL_CLIENT_DATA, c,
                PANEL_LABEL_BOLD, TRUE,
		XV_Y, row-4,
                XV_X, xv_get(t->name, XV_X) +
			 xv_get(t->name, XV_WIDTH) + 5,
                XV_HELP_DATA, "cm:ShowCalbutt",
                0);

        (void)xv_set(panel, PANEL_DEFAULT_ITEM, button, 0);

	ds_position_popup(c->frame, t->frame, DS_POPUP_LOR);

	window_fit(panel);
	window_fit(t->frame);

	return (caddr_t)t;
}
