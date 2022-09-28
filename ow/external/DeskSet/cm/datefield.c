#ifndef lint
static  char sccsid[] = "@(#)datefield.c 3.6 93/02/25 Copyr 1991 Sun Microsystems, Inc.";
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
#include <xview/panel.h>
#include <xview/font.h>
#include "util.h"
#include "graphics.h"
#include "timeops.h"
#include "editor.h"
#include "calendar.h"
#include "datefield.h"
#include "props.h"
#include "gettext.h"
#include "misc.h"

/* Returns a date string that parser can handle */
extern char *
get_date_str(p, pi)
        Props *p;
	Panel_item pi;
{
        char mo[3], day[3], yr[5];
        char *date = NULL;
        static char buf[80];
 
        date = (char*)xv_get(pi, PANEL_VALUE);
        /* cm_getdate() likes m/d/y format */
	if (date == NULL || *date == '\0') {
		format_tick(calendar->view->date, Order_MDY,
			Separator_Slash, buf);
		return buf;
	}
	if ( datestr2mdy(date, p->ordering_VAL, p->separator_VAL, buf) ) {
        return buf;
	} else {
		return (char *)NULL;
	}
}
extern void
set_date_on_panel(t, pi, order, separator)
	Tick t;
	Panel_item pi;
	Ordering_Type order;
	Separator_Type separator;
{
	char buf[15];

	format_tick(t, order, separator, buf);
	xv_set(pi, PANEL_VALUE, buf, NULL);
}
extern Panel_item
create_datelabel(panel, x, font, msg, pi) 
	Panel panel;
	int  x;
	Xv_Font font;
	char *msg;
	Panel_item *pi;
{
	Xv_Font old_font;

	old_font = (Xv_Font)xv_get(panel, XV_FONT);
	xv_set(panel, XV_FONT, font, NULL);
	*pi = xv_create(panel, PANEL_MESSAGE,
		XV_X, x,
		XV_Y, xv_row(panel, 2),
		PANEL_LABEL_STRING, msg,
		NULL);
	xv_set(panel, XV_FONT, old_font, NULL);
}
