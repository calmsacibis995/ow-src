#ifndef lint
static  char sccsid[] = "@(#)find.c 3.15 93/12/09 Copyr 1991 Sun Microsystems, Inc.";
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
#include "appt.h"
#include "util.h"
#include "timeops.h"
#include "graphics.h"
#include "calendar.h"
#include "ds_popup.h"
#include "find.h"
#include "datefield.h"
#include "props.h"
#include "table.h"
#include "gettext.h"
#include "editor.h"
#include "yearglance.h"
#include "select.h"

static Notify_value
find_done_proc(frame)
	Frame frame;
{
	Calendar *c = (Calendar*)xv_get(frame, WIN_CLIENT_DATA);
	Find *f = (Find*)c->find;
	char *apptstr;

	apptstr = (char *)xv_get(f->apptstr, PANEL_VALUE);
	if (f->apptval != NULL)
		free(f->apptval);
	f->apptval = (char*)ckalloc(cm_strlen(apptstr)+1);
	cm_strcpy(f->apptval, apptstr);
	f->no_months = (int)xv_get(f->months, PANEL_VALUE);
	xv_set(frame, XV_SHOW, FALSE, NULL);
	xv_destroy_safe(frame);
	f->frame = NULL;
        return NOTIFY_DONE;
}
static void
get_dayrange(date, keyrange)
        Tick date;
        Keyrange *keyrange;
{
	Props *p = (Props*)calendar->properties;

	keyrange->tick1 = lowerbound(date) + (hrsec*p->begin_slider_VAL)+1;
	keyrange->tick2 = next_nhours(keyrange->tick1, 
			p->end_slider_VAL-p->begin_slider_VAL)-1;
}

/* ARGSUSED */
static Notify_value
f_find_fwd(item, event)
        Panel_item item; Event *event;
{
	Calendar *c = (Calendar*)xv_get(item, PANEL_CLIENT_DATA);
	Props *p = (Props*)c->properties;
	Find *f = (Find*)c->find;
	int mos, i, j;
	char *astr;
	Range range;
	Abb_Appt   *list = NULL, *a = NULL;
	Keyrange keyrange;
	char buf[WHAT_LEN+1];
        int mo;
        struct pr_pos xy;
	
	buf[0] = NULL;
	mos = (int)xv_get(f->months, PANEL_VALUE);
	astr = (char*)xv_get(f->apptstr, PANEL_VALUE);

	if (astr == NULL || *astr == NULL) {
                xv_set(f->frame, FRAME_LEFT_FOOTER,
			MGET("Specify Appt String to Match."), NULL);
		return NOTIFY_DONE;
	}
	if (mos <=0 ) mos = 1;
	range.key1 = c->view->date+1;
	range.key2 = nextmonth_exactday(range.key1);
	range.next = NULL;

	for (i = 0; i < mos; i++) {
		table_abbrev_lookup_range(c->view->current_calendar, 
				&range, &list);
		for(a = list; a != NULL;  a = a->next) {
			for (j=0; a->what[j] != NULL; j++) {
				if (strncasecmp(astr, &(a->what[j]),
					cm_strlen(astr)) == 0) {
					format_abbrev_appt(a, buf, true, 
						p->default_disp_VAL);
					xv_set(f->frame, FRAME_LEFT_FOOTER, buf, NULL);
					if (c->view->glance == dayGlance)
						get_dayrange(c->view->date, &keyrange);
					else
						get_range(c->view->glance, c->view->date, &keyrange);
					if (in_range(&keyrange, a->appt_id.tick)) {
						calendar_deselect(c);
						c->view->olddate = c->view->date;
						c->view->date = a->appt_id.tick;
						paint_selection(c);
					}
					else {
						c->view->olddate = c->view->date;
						c->view->date = a->appt_id.tick;
						gr_clear_area(c->xcontext, 0, 0, 
							xv_get(c->canvas, XV_WIDTH),
							xv_get(c->canvas, XV_HEIGHT));
						if (c->view->glance == dayGlance) {
							init_mo(c);
							init_dayview(c);
							paint_selection(c);
						}
                                                else if (c->view->glance
== yearGlance) {
                                                        mo = month(c->view->date);
                                                        xy.y = month_row_col[mo-1][ROW];
                                                        xy.x = month_row_col[mo-1][COL];
                                                        calendar_select (c, monthSelect, (caddr_t)&xy);
                                                }
						paint_canvas(c, NULL);
					}
					destroy_abbrev_appt(list);
					return NOTIFY_DONE;
				}
			}
		}
		range.key1 = range.key2-1; 
        	range.key2 = nextmonth_exactday(range.key1);    
	}
	xv_set(f->frame, FRAME_LEFT_FOOTER, MGET("Appointment Not Found."),
			NULL);
	destroy_abbrev_appt(list);
	return NOTIFY_DONE;
	
}
/* ARGSUSED */
static Notify_value
f_find_back(item, event)
        Panel_item item; Event *event;
{
	Calendar *c = (Calendar*)xv_get(item, PANEL_CLIENT_DATA);
	Find *f = (Find*)c->find;
	Props *p = (Props*)c->properties;
	int mos, i, j;
	char *astr;
	Range range;
	Abb_Appt   *save_a = NULL, *list = NULL, *a = NULL;
	Boolean found = false;
	Keyrange keyrange;
	char buf[WHAT_LEN+1];
        int mo;
        struct pr_pos xy;
	
	buf[0] = NULL;
	mos = (int)xv_get(f->months, PANEL_VALUE);
	astr = (char*)xv_get(f->apptstr, PANEL_VALUE);

	if (astr == NULL || *astr == NULL) {
                xv_set(f->frame, FRAME_LEFT_FOOTER,
			MGET("Specify Appt String to Match."), NULL);
		return NOTIFY_DONE;
	}
	if (mos <=0 ) mos = 1;
	range.key1 = prevmonth_exactday(c->view->date);
	range.key2 = c->view->date-1;
	range.next = NULL;
	for (i = 0; i < mos; i++) {
		table_abbrev_lookup_range(c->view->current_calendar, 
				&range, &list);
		for(a = list; a != NULL;  a = a->next) {
			for (j=0; a->what[j] != NULL; j++) {
				if (strncasecmp(astr, &(a->what[j]),
				 	cm_strlen(astr)) == 0) {
					save_a = a;
					found = true;
				}
			}
		}
		if (found) {
			format_abbrev_appt(save_a, buf, true, 
					p->default_disp_VAL);
			xv_set(f->frame, FRAME_LEFT_FOOTER, buf, NULL);
			if (c->view->glance == dayGlance)
				get_dayrange(c->view->date, &keyrange);
			else
				get_range(c->view->glance, c->view->date, &keyrange);
			if (in_range(&keyrange, save_a->appt_id.tick)) {
				calendar_deselect(c);
				c->view->olddate = c->view->date;
				c->view->date = save_a->appt_id.tick;
				paint_selection(c);
			}
			else {
				c->view->olddate = c->view->date;
				c->view->date = save_a->appt_id.tick;
				gr_clear_area(c->xcontext, 0, 0, 
					xv_get(c->canvas, XV_WIDTH),
					xv_get(c->canvas, XV_HEIGHT));
				if (c->view->glance == dayGlance) {
					init_mo(c);
					init_dayview(c);
					paint_selection(c);
				}
                                else if (c->view->glance == yearGlance) {
                                        mo = month(c->view->date);
                                        xy.y = month_row_col[mo-1][ROW];
                                        xy.x = month_row_col[mo-1][COL];
                                        calendar_select (c, monthSelect,
(caddr_t)&xy);
                                }
				paint_canvas(c, NULL);
			}
			destroy_abbrev_appt(list);
			return NOTIFY_DONE;
		}
		range.key2 = range.key1+1; 
        	range.key1 = prevmonth_exactday(range.key1);    
	}
	xv_set(f->frame, FRAME_LEFT_FOOTER, MGET("Appointment Not Found."),
			NULL);
	destroy_abbrev_appt(list);
	return NOTIFY_DONE;

}
static Panel_setting
f_text_notify_proc(item, event)

        Panel_item      item;
        Event           *event;

{
	Calendar *c = (Calendar*)xv_get(item, PANEL_CLIENT_DATA);

        /* * If field is empty advance to next field.  Else do find
		forward */
        if (*((char *)xv_get(item, PANEL_VALUE)) == '\0')
                return(panel_text_notify(item, event));
        else
		f_find_fwd(((Find*)c->find)->ff_button, (Event*)NULL);

        return (PANEL_NONE);
}
extern caddr_t
make_find(c)
	Calendar *c;
{
	Find *f;
	int wd, gap = 5, row;
	char *t;
        Font_string_dims dims;
        struct pr_size size1, size2;
	Xv_Font pf;
	Panel panel;

	if (c->find == NULL) {
		c->find = (caddr_t)ckalloc(sizeof(Find));
		f = (Find*)c->find;
		f->no_months = 12;
	}
	else
		f = (Find*)c->find;

	pf = xv_get(c->frame, XV_FONT);
	f->frame = xv_create(c->frame, FRAME_CMD,
		WIN_USE_IM, TRUE,
		FRAME_INHERIT_COLORS, TRUE,
                FRAME_SHOW_LABEL, TRUE,
                FRAME_CMD_PUSHPIN_IN, TRUE,
                FRAME_SHOW_FOOTER, TRUE,
                FRAME_DONE_PROC, find_done_proc,
		WIN_CMS,     (Cms)xv_get(c->frame, WIN_CMS),
                WIN_CLIENT_DATA, c,
		XV_LABEL,  MGET("CM: Find") ,
		XV_WIDTH,  200, 
		XV_HEIGHT, 70,
		XV_HELP_DATA, "cm:FindHelp",
                0);
	panel = xv_get(f->frame, FRAME_CMD_PANEL);

	xv_set(panel, 
                XV_X, 0,
                XV_Y, 1,
                PANEL_CLIENT_DATA, c,
		XV_HELP_DATA, "cm:FindHelp",
                0);

	t =  MGET("Match Appt:") ;
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size1.x = dims.width;
        size1.y = dims.height;

	t =  MGET("Months:") ;
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size2.x = dims.width;
        size2.y = dims.height;

	wd = max(size1.x, size2.x) + c->view->outside_margin;

	f->apptstr = xv_create(panel, PANEL_TEXT,
                PANEL_LABEL_STRING,  LGET("Match Appt:") ,
                PANEL_VALUE_DISPLAY_LENGTH, 15,
                PANEL_VALUE_STORED_LENGTH, WHAT_LEN,
		PANEL_VALUE, (char*)f->apptval,
                PANEL_NOTIFY_PROC, f_text_notify_proc,
		PANEL_CLIENT_DATA, c,
		XV_Y, xv_row(panel, 0),
		PANEL_VALUE_X, wd + gap,
                XV_HELP_DATA, "cm:FindAppt",
                0);
	f->months = xv_create(panel, PANEL_NUMERIC_TEXT,
		PANEL_MIN_VALUE, 1,
                PANEL_LABEL_STRING,  LGET("Months:") ,
                PANEL_VALUE_DISPLAY_LENGTH, 6,
		PANEL_VALUE, f->no_months,
		XV_Y, xv_row(panel, 1),
		PANEL_VALUE_X, wd + gap,
                XV_HELP_DATA, "cm:FindMonths",
                0);
	row = xv_row(panel, 2);
	f->ff_button = xv_create(panel, PANEL_BUTTON,
                PANEL_LABEL_STRING,  LGET("Find Forward") ,
                PANEL_LABEL_BOLD, TRUE,
                PANEL_NOTIFY_PROC, f_find_fwd,
		XV_X, 15,
		XV_Y, row,
                PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:FindFwd",
                0);
	xv_create(panel, PANEL_BUTTON,
                PANEL_LABEL_STRING,  LGET("Find Backward") ,
                PANEL_LABEL_BOLD, TRUE,
                PANEL_NOTIFY_PROC, f_find_back,
		XV_Y, row,
		XV_X, xv_get(f->ff_button, XV_X) +
			 xv_get(f->ff_button, XV_WIDTH) + 10,
                PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:FindBack",
                0);

        (void)xv_set(panel, PANEL_DEFAULT_ITEM, f->ff_button, 0);

	ds_justify_items(panel, TRUE);
	ds_position_popup(c->frame, f->frame, DS_POPUP_LOR);

	window_fit(panel);
	window_fit(f->frame);

	return (caddr_t)f;
}
