#ifndef lint
static  char sccsid[] = "@(#)browser.c 3.53 93/12/06 Copyr 1991 Sun Microsystems, Inc.";
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
#include <ctype.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include "appt.h"
#include <sys/param.h>
#ifdef SVR4
#include <sys/systeminfo.h>
#endif "SVR4"
#include <xview/xview.h>
#include <xview/xv_xrect.h>
#include <xview/cms.h>
#include <xview/frame.h>
#include <xview/font.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/notice.h>
#include <xview/textsw.h>
#include <xview/scrollbar.h>
#include <ds_listbx.h>
#include <ds_popup.h>
#include "util.h"
#include "timeops.h"
#include "datefield.h"
#include "props.h"
#include "graphics.h"
#include "browser.h"
#include "editor.h"
#include "select.h"
#include "calendar.h"
#include "table.h"
#include "todo.h"
#include "alist.h"
#include "mail.h"
#include "gettext.h"
#include "common.h"
#include "blist.h"
#include "tempbr.h"

#define BOX_ROWS 7
#define DATESIZ 40

static unsigned short busy_icon_image[] = {
#include "busy.icon"
};
extern Boolean
browser_showing(b)
        Browser *b;
{
        if (b != NULL && b->frame != NULL && xv_get(b->frame, XV_SHOW))
                return true;
        return false;
}
extern Boolean
browser_exists(b)
        Browser *b;
{
        if (b != NULL && b->frame != NULL)
                return true;
        return false;
}

/* ARGSUSED */
extern void
mb_deregister_names(table, name, row)
        Panel_item table; 
        char *name; 
        int row; 
{ 
	Calendar *c = (Calendar*)xv_get(table, PANEL_CLIENT_DATA);

	/* dont deregister current calendar */
	if (strcmp(c->view->current_calendar, name) == 0)
                return;

	(void)table_request_deregistration(name);
}
static void
mb_reset_calbox(c)
	Calendar *c;
{
	Editor *e = (Editor*)c->editor;
	int i;

	if (!editor_showing(e)) return;

	/* appt selected */
	if ((i = xv_get(e->apptbox, PANEL_LIST_FIRST_SELECTED)) != -1) 
		xv_set(e->apptbox, PANEL_LIST_SELECT, i, FALSE, NULL);
		
	browser2calbox(c);
}
/* ARGSUSED */
static void
mb_deselect_all(menu, mi)
        Menu    menu;
        Menu_item mi;
{
        Calendar *c = calendar;
	Browser *b;
	register int i;

	b = (Browser*)c->browser;

	xv_set(b->frame, FRAME_BUSY, TRUE, NULL);
	xv_set(b->box, XV_SHOW, FALSE, NULL);
	list_items_selected(b->box, mb_deregister_names);
        list_deselect_all(b->box);
	for (i = 0; i < b->segs_in_array; i++) b->multi_array[i] = 0;
	/* remove the glyph if it exists */
	for (i = xv_get(b->box, PANEL_LIST_NROWS)-1; i >= 0; i--)
		if (xv_get(b->box, PANEL_LIST_GLYPH, i)) 
			xv_set(b->box, PANEL_LIST_GLYPH, i, NULL, NULL);
	mb_refresh_canvas(b);
	mb_reset_calbox(c);
	xv_set(b->frame, FRAME_BUSY, FALSE, NULL);
	xv_set(b->box, XV_SHOW, TRUE, NULL);

}

static Notify_value
browser_done_proc(frame)
{
	Browser *b = (Browser*)calendar->browser;
	int i=0;

	xv_set(frame, XV_SHOW, FALSE, 0);
	list_items_selected(b->box, mb_deregister_names);
	list_deselect_all(b->box);
	for (i = 0; i < b->segs_in_array; i++) b->multi_array[i] = 0;
	/* remove the glyph if it exists */
	for (i = xv_get(b->box, PANEL_LIST_NROWS)-1; i >= 0; i--)
		if (xv_get(b->box, PANEL_LIST_GLYPH, i)) 
			xv_set(b->box, PANEL_LIST_GLYPH, i, NULL, NULL);
	mb_refresh_canvas(b);
        return(NOTIFY_DONE);
}

static void 
mb_display_footermess(b)
	Browser *b;
{
	int num_cals;
	char buf[32];

	num_cals = list_num_selected(b->box);
	if (num_cals == 1)
		sprintf(buf,  MGET("%d Calendar Displayed") , num_cals);
	else
		sprintf(buf,  MGET("%d Calendars Displayed") , num_cals);
	xv_set(b->frame, FRAME_LEFT_FOOTER, buf, NULL);
}
/* ARGSUSED */
static void
mb_repaint_proc(canvas, paintw, dpy, xwin, area)
        Canvas canvas;
        Xv_Window paintw;
        Display *dpy;
        Window xwin;
        Xv_xrectlist *area;
{
        Calendar *c;
        Browser *b;
	/* Stuff to compress exposures */
        XRectangle *xr  = area->rect_array;
        XEvent ev;
        XRectangle clip;
        int i;
	XContext *xc;
        int minx        = xr->x;
        int miny        = xr->y;
        int maxx        = minx + xr->width;
        int maxy        = miny + xr->height;

        if ((c = (Calendar *) xv_get(canvas, WIN_CLIENT_DATA))==NULL)
                return;
	b = (Browser*)c->browser;
	if ((xc = b->xcontext)==NULL)
		return;
	/* look through this expose event building the bbox */
        for (i = 1, xr++; i < area->count; i++, xr++) {
                if (xr->x < minx)
                        minx = xr->x;
                if (xr->y < miny)
                        miny = xr->y;
                if ((int) (xr->x + xr->width) > maxx)
                        maxx = xr->x + xr->width;
                if ((int) (xr->y + xr->height) > maxy)
                        maxy = xr->y + xr->height;
        }
	/* look through pending expose events building the bbox */

        while (XEventsQueued(dpy, QueuedAfterFlush) &&
            (XPeekEvent(dpy, &ev), (ev.xexpose.type == Expose && ev.xexpose.window == xc->xid))) {
                XNextEvent(dpy, &ev);
                if (ev.xexpose.x < minx)
                        minx = ev.xexpose.x;
                if (ev.xexpose.y < miny)
                        miny = ev.xexpose.y;
                if ((int) (ev.xexpose.x + ev.xexpose.width) > maxx)
                        maxx = ev.xexpose.x + ev.xexpose.width;
                if ((int) (ev.xexpose.y + ev.xexpose.height) > maxy)
                        maxy = ev.xexpose.y + ev.xexpose.height;
        }
 
        /* confine drawing to the extent of the damage */
        clip.x = minx;
        clip.y = miny;
        clip.width = maxx - minx;
        clip.height = maxy - miny;
 
	gr_set_clip_rectangles(xc, 0, 0, &clip, 1, Unsorted);
 
	gr_clear_area(b->xcontext, 0, 0, b->canvas_w, b->canvas_h);
	mb_refresh_canvas(b);

	gr_clear_clip_rectangles(xc);
        XSync(dpy, 0);
}

extern void
mb_update_segs(b, tick, dur, start_index, end_index)
	Browser *b;
	Tick tick, dur;
	int *start_index, *end_index;
{
	int	num_segs, i, start, start_hour, duration, nday;
	Props *p;

	p = (Props*)calendar->properties;

	start_hour = hour(tick);
        
	if (start_hour >= p->end_slider_VAL) {
		*start_index = -1;
		*end_index = -1;
		return;
	}

	if (start_hour < p->begin_slider_VAL) {
		start = 0;
		duration = dur - ((double)(p->begin_slider_VAL - 
                 ((double)start_hour + (double)minute(tick)/(double)60)) 
			* (double)hrsec);
	} else{
		start = ((double)(start_hour - p->begin_slider_VAL) * 
			(double)60 + (double)minute(tick));
		duration = dur;
	}

	if (duration <= 0) {
		*start_index = -1;
		*end_index = -1;
		return;
	}
	
	nday = (nday=dow(tick))==0? 6: nday-1;
	num_segs = (double)start / (double)MINS_IN_SEG;
	*start_index = (double)start / (double)MINS_IN_SEG + (nday * (b->segs_in_array/7));
	if (start - (num_segs * MINS_IN_SEG) > 7)
		(*start_index)++;
	num_segs = ((double)duration / (double)60 / (double)MINS_IN_SEG);
	*end_index = num_segs + *start_index;
	if (((double)duration/(double)60-MINS_IN_SEG*num_segs) > 7)
		(*end_index)++;

	if (*end_index > (i = ((nday + 1) * (b->segs_in_array / 7))) ) 
		*end_index = i;

	for (i = *start_index; i < *end_index; i++) 
		if (b->add_to_array) 
			b->multi_array[i]++;
		else if (b->multi_array[i] > 0)
			b->multi_array[i]--;
}
extern void
mb_display_busyicon(table, entry_text, row)
        Panel_item table;
        char *entry_text;
        int row;
{
	Abb_Appt        *list=NULL, *a;
        Calendar      *c;
        Browser      *b;
        Range   range;
	Boolean has_appt = false;

        c = (Calendar*)xv_get(table, PANEL_CLIENT_DATA);
        b = (Browser*)c->browser;

        /* must get day range for overlapping appts. */
	range.key1 = b->begin_day_tick;
	range.key2 = b->end_hr_tick;
        range.next = NULL;
        table_abbrev_lookup_range(entry_text, &range, &list);
        for (a = list; a != NULL; a = a->next) {
		if (a->appt_id.tick+1 <= b->end_hr_tick &&
			(a->appt_id.tick+a->duration-1) >= 
				b->begin_hr_tick) {
			if (!xv_get(b->box, PANEL_LIST_GLYPH, row)) 
				xv_set(b->box, PANEL_LIST_GLYPH, row, 
					b->busy_icon, NULL);
				has_appt = true;
				break;
		}
        }
        destroy_abbrev_appt(list);
	if (!has_appt && xv_get(b->box, PANEL_LIST_GLYPH, row)) 
		xv_set(b->box, PANEL_LIST_GLYPH, row, NULL, NULL);
}
static void
mb_update_array(table, entry_text, row)
	Panel_item table; 
	char *entry_text; 
	int row;
{
	Abb_Appt	*list=NULL, *a;
	Browser	*b;
	Calendar	*c;
	Range	range;
	int start_ind, end_ind;

	c = (Calendar*)xv_get(table, PANEL_CLIENT_DATA);
	b = (Browser*)c->browser;

	range.key1 = b->begin_week_tick-1;
	range.key2 = next_ndays(b->begin_week_tick, 8);
	range.next = NULL;  

	table_abbrev_lookup_range(entry_text, &range, &list);	

	for(a = list; a != NULL;  a = a->next) 
		mb_update_segs(b, a->appt_id.tick, a->duration, 
			&start_ind, &end_ind);
	destroy_abbrev_appt(list);
}
extern void
mb_update_busyicon(table, entry_text, row)
        Panel_item table;
        char *entry_text;
        int row;
{
	Calendar        *c;
	Browser *b;
	Boolean selected = false;

	selected = xv_get(table, PANEL_LIST_SELECTED, row);

	c = (Calendar*)xv_get(table, PANEL_CLIENT_DATA);
	b = (Browser*)c->browser;

	/* remove glyph */
	if (selected)  
		mb_display_busyicon(table, entry_text, row);
	else 
		if (xv_get(b->box, PANEL_LIST_GLYPH, row)) 
			xv_set(b->box, PANEL_LIST_GLYPH, row, NULL, NULL);
}

static void
reset_ticks(c)
	Calendar *c;
{
	Browser *b = (Browser*)c->browser;
	Props *p = (Props*)c->properties;
	int day_of_week;

	day_of_week = (((day_of_week = dow(b->date)) == 0) ? 6 : --day_of_week);                                
        b->begin_day_tick = b->begin_week_tick = lowerbound(b->date)-(day_of_week*daysec);
	b->begin_day_tick = next_ndays(b->begin_week_tick, b->col_sel+1);
	b->end_day_tick = next_nhours(b->begin_day_tick, p->end_slider_VAL+1);
	b->date = b->begin_hr_tick = next_nhours(b->begin_day_tick,
				p->begin_slider_VAL+b->row_sel);
	b->end_hr_tick = next_nhours(b->begin_hr_tick, 1);
}
static void
mb_reset_time(c)
	Calendar *c;
{
	Browser *b = (Browser*)c->browser;
	Editor *e = (Editor*)c->editor;
	Props *p = (Props*)c->properties;
	int am = 1, hr;
	char start_time[8], end_time[8];
	
	hr = hour(b->begin_hr_tick);
	start_time[0] = '\0';
	if (p->default_disp_VAL == hour12) {
		am = adjust_hour(&hr);
		(void)sprintf(start_time, "%d:00", hr);
		xv_set(e->ampm, PANEL_VALUE, am ? 0 : 1, NULL);
	}
	else 
		(void)sprintf(start_time, "%02d00", hr);

	xv_set(e->time, PANEL_VALUE, start_time, NULL);

	am = true;
	hr = hour(b->end_hr_tick);
	end_time[0] = '\0';
	if (p->default_disp_VAL == hour12) {
		am = adjust_hour(&hr);
		(void)sprintf(end_time, "%d:00", hr);
		xv_set(e->minhr, PANEL_VALUE, am ? 0 : 1, 0);
	}
	else
		(void)sprintf(end_time, "%02d00", hr);

	xv_set(e->duration, PANEL_VALUE, end_time, NULL);

	set_date_on_panel(b->date, e->datetext, p->ordering_VAL, 
				p->separator_VAL);
}
static Notify_value
mb_canvas_notify(client, event, arg, when)
        Notify_client     client;
        Event             *event;
        Notify_arg        arg;
        Notify_event_type when;
{
        Calendar *c = (Calendar*)xv_get(client, WIN_CLIENT_DATA);
        Browser *b = (Browser*)c->browser;
	Editor	*e;
        int i, x, y;
	struct pr_pos xy;
	static Event last_event;

	if (b == NULL) 
        	return (notify_next_event_func(client, 
			(Notify_event)event, arg, when));
	e = (Editor*)c->editor;
	x = event_x(event);
	y = event_y(event);
        if (event_id (event) == MS_LEFT && event_is_down(event)) 
	{
		if (x > b->chart_x && y > b->chart_y && 
			x < (b->chart_x + b->chart_width) 
			&& y < (b->chart_y + b->chart_height)) 
		{
			browser_deselect(b);
			b->col_sel = (x - b->chart_x) / b->boxw;
			b->row_sel = (y - b->chart_y) / b->boxh;
			xy.x = b->col_sel;
			xy.y = b->row_sel;
			browser_select(b, &xy);

			if (ds_is_double_click(&last_event, event)) { 
     				if (cal_update_props())
                			for (i=0; i < NO_OF_PANES; i++)
                        			set_rc_vals((Props*)calendar->properties, i);
				set_default_reminders(c);	
				set_default_scope_plus(e);	
        			set_default_what(e);
				set_default_privacy(e);
				show_editor(c);
			}
			reset_ticks(c);
			if (editor_showing(e)) {
				mb_reset_time(c);
				mb_reset_calbox(c);
				mb_add_times(e->apptbox);
			}
			list_items_selected(b->box, mb_display_busyicon);
			backup_values(c);
		}
		c->general->last_canvas_touched = browser;
	}
	last_event = *event;
 
        return (notify_next_event_func(client, (Notify_event)event, arg, when));
}

/* ARGSUSED */
static Boolean
register_names(table, name, row)
	Panel_item table;
	char *name;
	int row;
{
	Register_Status status;
	Browser *b;
	Calendar *c;
	char    buf[MAXPATHLEN];

	c = (Calendar*)xv_get(table, PANEL_CLIENT_DATA);

	if (name != NULL && name[0] != '\0') {
        	status = table_request_registration(name);
		if (status != register_succeeded) {
			if (status == register_notable)
				(void)sprintf(buf, EGET("Calendar %s does not exist..."),
				 name);
			else
				(void)sprintf(buf, EGET("Unable to access %s  ..."), name);
			b = (Browser*)c->browser;
			(void)xv_set(b->frame, FRAME_LEFT_FOOTER, buf, 0);
			xv_set(table, PANEL_LIST_SELECT, row, FALSE, NULL);
			return false;
                }
	}
	return true;
}
/* ARGSUSED */
static Notify_value 
mb_box_notify(pi, string, cd, op, event, row)
	Panel_item pi; 
	char *string;     /* string of row being operated on */
    	Xv_opaque cd;	  /* Client data of row */
    	Panel_list_op op; /* operation */
    	Event *event;
    	int row;		
{
	Browser       *b;
	Calendar      *c;
	Editor        *e;

	c = (Calendar*)xv_get(pi, PANEL_CLIENT_DATA);
	b = (Browser*)c->browser;
	e = (Editor*)c->editor;

	if (string == NULL || string[0] == '\0') 
		return NOTIFY_DONE;
	if (op == PANEL_LIST_OP_SELECT) {
		xv_set(b->frame, FRAME_BUSY, TRUE, NULL);
		if (!register_names(pi, string, row)) {
			xv_set(b->frame, FRAME_BUSY, FALSE, NULL);
			return NOTIFY_DONE;
		}
	}
	else if (op == PANEL_LIST_OP_DESELECT) {
		xv_set(b->frame, FRAME_BUSY, TRUE, NULL);
		mb_deregister_names(pi, string, row);
	}
	else return;

	/* add or delete from browser array ? */
	b->add_to_array = (Boolean)xv_get(pi, PANEL_LIST_SELECTED, row);
		
	mb_update_array(pi, string, row);
	mb_update_busyicon(pi, string, row);
	mb_refresh_canvas(b);
	if (editor_showing(e)) {
		mb_reset_calbox(c);
		update_mailto_entry(c, string);
	}
	xv_set(b->frame, FRAME_BUSY, FALSE, NULL);

	return NOTIFY_DONE;
}

extern void
mb_refresh_canvas(b)
	Browser *b;
{
	mb_draw_appts(b, 0, b->segs_in_array);
	mb_draw_chartgrid(b);
	mb_display_footermess(b);
}

static void
mb_init_browchart(b)
 	Browser *b; 
{
	int	char_width, char_height, i, day_of_week;
	char	*date;
	int	label_height, label_width;
	Calendar *c;
	Props	*p;
	char buf[DATESIZ];


	c = (Calendar*)xv_get(b->box, PANEL_CLIENT_DATA);
	p = (Props*)c->properties;

	date = (char*)get_date_str(p, b->datetext);
	if (date != NULL) 
	       b->date = cm_getdate(date, NULL)+
			(b->row_sel+p->begin_slider_VAL)*hrsec;
	else {
		b->date = lowerbound(now())+ 
			(b->row_sel+p->begin_slider_VAL)*hrsec;
		format_tick(b->date, p->ordering_VAL, 
			p->separator_VAL, buf);
		xv_set(b->datetext, PANEL_VALUE, buf, NULL);
	}
		
	reset_ticks(c);

	b->canvas_w = (int)xv_get(b->canvas, XV_WIDTH);
	b->canvas_h = (int)xv_get(b->canvas, XV_HEIGHT);
	char_height = xv_get(c->fonts->lucida12b, FONT_DEFAULT_CHAR_HEIGHT);
    char_width = xv_get(c->fonts->lucida12b, FONT_COLUMN_WIDTH);
	label_height = char_height * 2;
	label_width = char_width + 2;
	b->chart_height = b->canvas_h - (c->view->outside_margin * 2)
			 - label_height - 5;
	b->chart_width = b->canvas_w - (c->view->outside_margin * 2) 
			- label_width;
	b->boxw = b->chart_width / 7;
	b->chart_width = b->boxw * 7;
	i = p->end_slider_VAL - p->begin_slider_VAL;
	b->boxh = b->chart_height / i;
	/* make sure boxh is evenly divisble by BOX_SEG */
	b->boxh -= (b->boxh % BOX_SEG);
	b->chart_height = b->boxh * i;
	b->chart_x = c->view->outside_margin + label_width;
	b->chart_y = c->view->outside_margin + label_height + 
			char_height ;
}
extern void
mb_init_browser(c)
	Calendar *c;
{
	Browser *b = (Browser*)c->browser;

	cal_update_props();
	mb_init_blist(c);	
	b->row_sel = b->col_sel = 0;
	mb_init_browchart(b);
	mb_init_canvas(c);
	mb_refresh_canvas(b);
	backup_values(c);
}
static void
mb_resize_proc(canvas, width, height)
        Canvas canvas;
        int width, height;
{
        Calendar *c = (Calendar *)xv_get(canvas, WIN_CLIENT_DATA);
        Browser *b = (Browser *)c->browser;

        /*      This is gross.  Do this to get around the fact
                the resize proc is called before window_main_loop
                when everything is set up correctly.    */

        if (b == NULL || (XContext*)b->xcontext == NULL)
		 return;
        gr_clear_area((XContext*)b->xcontext, 0, 0, width, height);
	mb_init_browchart(b);

}

extern void
mb_init_blist(c)
	Calendar *c;
{
	char	*tmp, *name, *namelist;
	Browser *b = (Browser*)c->browser;
	Props *p = (Props*)c->properties;
	int position;

	tmp = cm_get_property(property_names[CP_DAYCALLIST]);
	namelist =  (char*)ckalloc(cm_strlen(tmp)+1);
	cm_strcpy(namelist, tmp);
	if (namelist != NULL && *namelist != NULL ) {
                name = (char *) strtok(namelist, " ");
                while (name != NULL) {
                        position = xv_get(b->box, PANEL_LIST_NROWS);
                        list_add_entry(b->box, name, (Pixrect*)NULL, 
					NULL, position, FALSE);
                        name = (char *) strtok((char *)NULL, " ");
                }
        }
	if (!list_in_list(b->box, c->calname, &position))
		list_add_entry(b->box, c->calname, (Pixrect*)NULL, NULL, 0, FALSE);
	if (!list_in_list(b->box, p->defcal_VAL, &position))
		list_add_entry(b->box, p->defcal_VAL, (Pixrect*)NULL, NULL, 1, FALSE);
	free(namelist);
}

extern void
mb_init_canvas(c)
	Calendar *c;
{
	Browser *b = (Browser*)c->browser;

        list_entry_select_n(b->box, 0);
	b->add_to_array = true;
	reset_ticks(c);
	mb_update_array(b->box, c->calname, 0);
	mb_update_busyicon(b->box, c->calname, 0);
	gr_clear_area(b->xcontext, 0, 0, b->canvas_w, b->canvas_h);
	if (strcmp(c->view->current_calendar, c->calname) != 0) 
		register_names(b->box, c->calname, 0); 
}

extern int
self(s)
	char *s;
{
	Props *p = (Props*)calendar->properties;

	if (strcmp(s, calendar->calname)==0 ||
	   	strcmp(s, p->defcal_VAL)==0)
		return 1;
	return 0;
}
	
/* ARGSUSED */
extern void
change_it(m, mi)
	Menu m;
	Menu_item mi;
{
	char	*new_calendar;
	void	switch_it();

	new_calendar = (char*)xv_get(mi, MENU_STRING);	
	if (new_calendar == NULL) return;

	switch_it(new_calendar, main_win);
}

extern void
switch_it (new_calendar, win)
char	*new_calendar;
WindowType win;
{
	Calendar *c = calendar;
	Browser *b = (Browser*)c->browser;
	Tempbr *tb = (Tempbr*)c->tempbr;
	Editor *e = (Editor*)c->editor;
	Props *p = (Props*)c->properties;
	Register_Status status;
	char label[BUFSIZ], *save_name;
	Boolean dereg = false;
	int save_version;
	Stat stat;

	xv_set(c->frame, FRAME_LEFT_FOOTER, "", FRAME_BUSY, TRUE, 0);
	label[0] = '\0';

	/* already browsing it */
	if (strcmp(new_calendar, c->view->current_calendar) == 0) {
		notice_prompt(c->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
			EGET("You Are Already Browsing"),
			new_calendar,
			0,
                    	NOTICE_BUTTON_YES,  LGET("Continue"),
		0);
		xv_set(c->frame, FRAME_BUSY, FALSE, 0);
		return;
	}

	/* Register myself to the new calendar. */
	status = table_request_registration(new_calendar);
	if (status != register_succeeded) {
		if (status == register_notable) {
			if (strcmp(c->calname, new_calendar) == 0) {
				if ((status = table_create(new_calendar)) == status_ok) {
					status = table_request_registration(new_calendar);
					if (p->frame != NULL) {
						set_rc_vals(p, GROUP_ACCESS_LISTS);
						p_display_vals(p, GROUP_ACCESS_LISTS);
					}
				}
			}
			else {
				sprintf(label, EGET("Calendar %s does not exist..."),
					 new_calendar);
				xv_set(c->frame, FRAME_LEFT_FOOTER, label, 0);
				if (win == tempbrowser) 
        	        		xv_set(tb->frame, FRAME_LEFT_FOOTER, label, 0);
				xv_set(c->frame, FRAME_BUSY, FALSE, NULL);
				return;
			}
		}
		else { 
			(void) sprintf(label,  
				EGET("Unable to access %s  ..."),
					 new_calendar);
			xv_set(c->frame, FRAME_LEFT_FOOTER, label, 0);
			if (win == tempbrowser) 
                		xv_set(tb->frame, FRAME_LEFT_FOOTER, label, 0);
			xv_set(c->frame, FRAME_BUSY, FALSE, NULL);
			return;
		}
	}
	/* deregister if browser doesnt exist or if not selected in browser */
	if (!browser_exists(b) || 
		!list_is_selected(b->box, c->view->current_calendar)) {
		table_request_deregistration(c->view->current_calendar);
		dereg = true;
	}
	save_version = c->general->version; 
	save_name = cm_strdup(c->view->current_calendar); 

	c->general->version = get_data_version(new_calendar);
	/* must set this before paint_canvas() */
	if (c->view->current_calendar != NULL) 
		free(c->view->current_calendar);
	c->view->current_calendar = cm_strdup(new_calendar);

	stat = paint_canvas(c, NULL);

	/* registration probably failed so ... */
	if (stat == status_param && c->general->version <= CMS_VERS_3) {
		sprintf(label, EGET("Unable to access %s  ..."), 
			c->view->current_calendar);
                xv_set(c->frame, FRAME_LEFT_FOOTER, label, NULL);
		if (win == tempbrowser) 
			xv_set(tb->frame, FRAME_LEFT_FOOTER, label, 0);
		
		free(c->view->current_calendar);
		c->view->current_calendar = save_name;
		if (dereg)
			table_request_registration(c->view->current_calendar);
		c->general->version = save_version;
		xv_set(c->frame, FRAME_BUSY, FALSE, NULL);
		return;
	}

	if (win == tempbrowser) {
        	sprintf(label, EGET("Calendar: %s displayed."), new_calendar);
		xv_set(tb->frame, FRAME_LEFT_FOOTER, label, 0);
	}

	free(save_name);
	(void) sprintf(label, "%s: %s", (char*)cm_get_relname(), 
			c->view->current_calendar);
	xv_set(c->frame, XV_LABEL, label, 0);

	/* if todo list or appt list showing, reset to new calendar */
	common_update_lists(c);
	sprintf(label, "%s %s", MGET("CM Appointment Editor: "), 
		c->view->current_calendar);
	if (editor_exists(e)) {
		e_reset_proc(0, 0);
		xv_set(e->frame, XV_LABEL, label, NULL);
	}
	xv_set(c->frame, FRAME_BUSY, FALSE, NULL);
}
static void
br_display()
{
        Calendar *c = calendar;
        Browser *b = (Browser*)c->browser;
	register int i;
 
	for (i = 0; i < b->segs_in_array; i++) b->multi_array[i] = 0;
	b->add_to_array = true;
	list_items_selected(b->box, mb_update_array);
	list_items_selected(b->box, mb_update_busyicon);
	mb_refresh_canvas(b);
}

/* ARGSUSED */
static void
mb_select_all(menu, mi)
        Menu    menu;
        Menu_item mi;
{
        Calendar *c;
	Browser *b;

        c = (Calendar *) xv_get (mi, MENU_CLIENT_DATA);
	b = (Browser*)c->browser;

	xv_set(b->frame, FRAME_BUSY, TRUE, NULL);
	xv_set(b->box, XV_SHOW, FALSE, NULL);
        list_select_all(b->box);
	list_items_selected(b->box, register_names);
	br_display();
	mb_reset_calbox(c);
	xv_set(b->frame, FRAME_BUSY, FALSE, NULL);
	xv_set(b->box, XV_SHOW, TRUE, NULL);
}

/* ARGSUSED */
static void
prev_week_proc(item, event)
        Menu_item item;
        Event      *event;
{
        char buf[DATESIZ];
        Calendar *c;
        Browser *b;
        Props *p;
 
        c = (Calendar*)xv_get(item, MENU_CLIENT_DATA);
	b = (Browser*)c->browser;
	p = (Props*)c->properties;

	xv_set(b->frame, FRAME_BUSY, TRUE, NULL);
	b->date = last_ndays(b->date, 7);
	reset_ticks(c);

        format_tick(b->date, p->ordering_VAL, p->separator_VAL, buf);
        xv_set(b->datetext, PANEL_VALUE, buf, NULL);
	br_display();
	xv_set(b->frame, FRAME_BUSY, FALSE, NULL);
	backup_values(c);
}
/* ARGSUSED */
static void
next_week_proc(item, event)
        Menu_item item;
        Event      *event;
{
        char buf[DATESIZ];
        Calendar *c;
        Browser *b;
        Props *p;
 
        c = (Calendar*)xv_get(item, MENU_CLIENT_DATA);
	b = (Browser*)c->browser;
	p = (Props*)c->properties;
 
	xv_set(b->frame, FRAME_BUSY, TRUE, NULL);
	b->date = next_ndays(b->date, 7);
	reset_ticks(c);

        format_tick(b->date, p->ordering_VAL, p->separator_VAL, buf);
        xv_set(b->datetext, PANEL_VALUE, buf, NULL);

	br_display();
	xv_set(b->frame, FRAME_BUSY, FALSE, NULL);
	backup_values(c);
}
/* ARGSUSED */
static void
this_week_proc(item, event)
        Menu_item item;
        Event      *event;
{
        char buf[DATESIZ];
        Calendar *c;
        Browser *b;
        Props *p;
 
        c = (Calendar*)xv_get(item, MENU_CLIENT_DATA);
	b = (Browser*)c->browser;
	p = (Props*)c->properties;
 
	xv_set(b->frame, FRAME_BUSY, TRUE, NULL);
	b->date = now();
	reset_ticks(c);

        format_tick(b->date, p->ordering_VAL, p->separator_VAL, buf);
        xv_set(b->datetext, PANEL_VALUE, buf, NULL);

	br_display();
	xv_set(b->frame, FRAME_BUSY, FALSE, NULL);
	backup_values(c);
}
/* ARGSUSED */
static void
prev_month_proc(item, event)
        Menu_item item;
        Event      *event;
{
        char buf[DATESIZ];
        Calendar *c;
        Browser *b;
        Props *p;
 
        c = (Calendar*)xv_get(item, MENU_CLIENT_DATA);
	b = (Browser*)c->browser;
	p = (Props*)c->properties;

	xv_set(b->frame, FRAME_BUSY, TRUE, NULL);
	b->date = last_ndays(b->date, 28);
	reset_ticks(c);

        format_tick(b->date, p->ordering_VAL, p->separator_VAL, buf);
        xv_set(b->datetext, PANEL_VALUE, buf, NULL);

	br_display();
	xv_set(b->frame, FRAME_BUSY, FALSE, NULL);
	backup_values(c);
}
/* ARGSUSED */
static void
next_month_proc(item, event)
        Menu_item item;
        Event      *event;
{
        char buf[DATESIZ];
        Calendar *c;
        Browser *b;
        Props *p;
 
        c = (Calendar*)xv_get(item, MENU_CLIENT_DATA);
	b = (Browser*)c->browser;
	p = (Props*)c->properties;

	xv_set(b->frame, FRAME_BUSY, TRUE, NULL);
	b->date = next_ndays(b->date, 28);
	reset_ticks(c);

        format_tick(b->date, p->ordering_VAL, p->separator_VAL, buf);
        xv_set(b->datetext, PANEL_VALUE, buf, NULL);

	br_display();
	xv_set(b->frame, FRAME_BUSY, FALSE, NULL);
	backup_values(c);
}
static void
set_entry_date(c)
	Calendar* c;
{
        char *buf = NULL;
        Browser *b;
	Props *p;

	b = (Browser*)c->browser;
        p = (Props*)c->properties;
 
    if ( (buf = (char*)get_date_str(p, b->datetext)) == NULL ) {
		xv_set(b->frame, FRAME_LEFT_FOOTER, EGET("Error in Date Field..."), NULL);
		return;
	}
	xv_set(b->frame, FRAME_BUSY, TRUE, NULL);

        b->date = cm_getdate(buf, NULL);
        reset_ticks(c);
 
	br_display();
	xv_set(b->frame, FRAME_BUSY, FALSE, NULL);
	backup_values(c);
}
/* ARGSUSED */
static void
date_menu_proc(m, mi)
        Menu m;
        Menu_item      *mi;
{
        Calendar *c;
 
        c = (Calendar*)xv_get(m, MENU_CLIENT_DATA);

	set_entry_date(c);
}
/* ARGSUSED */
static void
date_entry_proc(item, event)
        Panel_item item;
        Event      *event;
{
        Calendar *c;
 
        c = (Calendar*)xv_get(item, PANEL_CLIENT_DATA);

	set_entry_date(c);
}

static void
mb_init_array(b, begin, end)
	Browser *b;
	int begin, end;
{
	b->segs_in_array = BOX_SEG * (end - begin) * 7;
	b->multi_array = (char*)ckalloc(b->segs_in_array);
}
static Menu
make_box_menu(c)
	Calendar *c;
{
	Menu box_menu;

        box_menu = menu_create(
                MENU_TITLE_ITEM,  LGET("Browse List") ,
                MENU_CLIENT_DATA,       c,
		MENU_NOTIFY_STATUS, XV_ERROR,
                MENU_ITEM,
                        MENU_STRING,  LGET("Select All") ,
                        MENU_ACTION_PROC, mb_select_all,
                        MENU_CLIENT_DATA,       c,
                        0,
                MENU_ITEM,
                        MENU_STRING,  LGET("Deselect All") ,
                        MENU_ACTION_PROC, mb_deselect_all,
                        MENU_CLIENT_DATA,       c,
                        0,
                NULL);

		return box_menu;
}
/* ARGSUSED */
extern Notify_value
show_setupblist(item, event)
        Panel_item item;
        Event *event;
{
	extern void show_blist();
	show_blist(0, 0);
}


extern caddr_t
make_browser(c)
	Calendar *c;
{
	Menu goto_menu;
        char    *t, buf[DATESIZ];
	Browser	*b;
	Props *p;
	int x_gap, y_gap;
	extern Notify_value mail_proc();
	Panel panel;
	Panel_item tmp_pi, mailbutton, schedbutton, datestr;
	extern void show_blist();

	p = (Props*) c->properties;
	b = (Browser*) ckalloc(sizeof(Browser));

	mb_init_array(b, p->begin_slider_VAL, p->end_slider_VAL);
	b->date = c->view->date;
	b->current_selection = (caddr_t) ckalloc(sizeof(Selection));

	b->busy_icon = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, busy_icon_image,
		SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 16,
                XV_HEIGHT, 16,
                0);
	
	b->frame = (Frame) xv_create(c->frame, FRAME_CMD,
		FRAME_INHERIT_COLORS, TRUE,
                FRAME_SHOW_LABEL, TRUE,
                FRAME_CMD_PUSHPIN_IN, TRUE,
                FRAME_SHOW_FOOTER, TRUE,
                FRAME_SHOW_RESIZE_CORNER, TRUE,
		FRAME_DONE_PROC, browser_done_proc,
		WIN_CMS,     (Cms)xv_get(c->frame, WIN_CMS),
		XV_HEIGHT, 550, 
                WIN_CLIENT_DATA, c,
                0);

	panel = xv_get(b->frame, FRAME_CMD_PANEL);
	xv_set(panel, 
                XV_X, 0,
                XV_Y, 0,
                WIN_ROWS, 10,
                PANEL_CLIENT_DATA, c,
                OPENWIN_SHOW_BORDERS, FALSE,
                XV_HELP_DATA, "cm:BrPanel",
                0);
	goto_menu = menu_create(
                MENU_CLIENT_DATA, c,
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_STRING,  LGET("Entry") ,
                        MENU_ACTION_PROC, date_menu_proc,
                        XV_HELP_DATA, "cm:BrGoto",
                        0),
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_STRING,  LGET("Prev Week") ,
                        MENU_ACTION_PROC, prev_week_proc,
                        XV_HELP_DATA, "cm:BrGoto",
                        0),
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_STRING,  LGET("This Week") ,
                        MENU_ACTION_PROC, this_week_proc,
                        XV_HELP_DATA, "cm:BrGoto",
                        0),
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_STRING,  LGET("Next Week") ,
                        MENU_ACTION_PROC, next_week_proc,
                        XV_HELP_DATA, "cm:BrGoto",
                        0),
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_STRING,  LGET("Prev Month") ,
                        MENU_ACTION_PROC, prev_month_proc,
                        XV_HELP_DATA, "cm:BrGoto",
                        0),
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_STRING,  LGET("Next Month") ,
                        MENU_ACTION_PROC, next_month_proc,
                        XV_HELP_DATA, "cm:BrGoto",
                        0),
                0);

	schedbutton = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,  LGET("Schedule...") ,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, xv_col(panel,1),
                XV_Y, xv_row(panel,0),
                PANEL_NOTIFY_PROC, sched_proc,
		PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:SchedButton",
                0);

	x_gap = 18;
	mailbutton = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,  LGET("Mail...") ,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, xv_get(schedbutton, XV_X) + 
			xv_get(schedbutton, XV_WIDTH) + x_gap,
                XV_Y, xv_row(panel,0), 
                PANEL_NOTIFY_PROC, mail_proc,
		PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:MailButton",
                0);
	   /* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL :
	    *
	    * Goto = Go to or view the specified date. 
	    */
        datestr = xv_create(panel, PANEL_BUTTON,
                PANEL_LABEL_STRING,  LGET("Go To:") ,
                PANEL_CLIENT_DATA, c,
                PANEL_ITEM_MENU, goto_menu,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, xv_get(mailbutton, XV_X) +
			xv_get(mailbutton, XV_WIDTH) + x_gap,
                XV_Y, xv_row(panel,0),
                XV_HELP_DATA, "cm:BrGoto",
                0);
        format_tick(b->date, p->ordering_VAL, p->separator_VAL, buf);
        b->datetext = xv_create(panel, PANEL_TEXT,
                XV_SHOW, TRUE,
                XV_X, xv_get(datestr, XV_X) +
                        xv_get(datestr, XV_WIDTH) + 2,
                XV_Y,xv_row(panel,0),
                PANEL_READ_ONLY,  FALSE,
                PANEL_VALUE_DISPLAY_LENGTH, 11,
                PANEL_VALUE_STORED_LENGTH, 50,
                PANEL_VALUE,   buf,
		PANEL_NOTIFY_PROC, date_entry_proc,
		PANEL_CLIENT_DATA,       c,
                XV_HELP_DATA, "cm:dateStr",
                0);
 
	xv_create(panel, PANEL_MESSAGE,
		XV_X, 23,
                XV_Y, xv_row (panel, 1) - 5,
		PANEL_LABEL_IMAGE,   b->busy_icon,
                XV_HELP_DATA, "cm:CalBusy",
		NULL);

	xv_create(panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING,   MGET(" = Calendars Busy At Time Selected") ,
		XV_X, 42,
		XV_Y, 35,
                XV_HELP_DATA, "cm:CalBusy",
		NULL);
 
        b->box = xv_create(panel, PANEL_LIST,
                PANEL_LIST_DISPLAY_ROWS,        BOX_ROWS,
                PANEL_LIST_WIDTH,    -1,
                XV_X, xv_col (panel, 1),
                XV_Y, xv_row (panel, 2)-15,
                PANEL_CHOOSE_ONE,               FALSE,
                PANEL_CHOOSE_NONE,              TRUE,
                PANEL_NOTIFY_PROC,              mb_box_notify,
                PANEL_ITEM_MENU,                make_box_menu(c),
                PANEL_CLIENT_DATA,              c,
                XV_HELP_DATA, "cm:BrBox",
        	NULL);

        tmp_pi = xv_create(panel, PANEL_BUTTON,
                PANEL_LABEL_STRING,  LGET("Setup Menu..."),
                PANEL_LABEL_BOLD, TRUE,
                XV_Y, xv_get(b->box, XV_Y) 
			+ xv_get(b->box, XV_HEIGHT) + 10,
                XV_X, 130,
                PANEL_CLIENT_DATA, c,
                PANEL_NOTIFY_PROC, show_setupblist,
                XV_HELP_DATA, "cm:SetupMenu",
                0);
        
	y_gap = 10;

	xv_set(panel, XV_HEIGHT, xv_get(tmp_pi, XV_Y) +
		xv_get(tmp_pi, XV_HEIGHT)+10, NULL);

	b->canvas = (Canvas) xv_create(b->frame, CANVAS,
                XV_X, 0,
                CANVAS_FIXED_IMAGE, FALSE,
                CANVAS_RETAINED, FALSE,
                WIN_BIT_GRAVITY, ForgetGravity,
                CANVAS_NO_CLIPPING, TRUE,
                CANVAS_X_PAINT_WINDOW, TRUE,
                CANVAS_REPAINT_PROC, mb_repaint_proc,
                CANVAS_RESIZE_PROC, mb_resize_proc,
                OPENWIN_AUTO_CLEAR, TRUE,
                WIN_BELOW, panel,
                WIN_CONSUME_KBD_EVENTS, WIN_UP_EVENTS, 0,
                WIN_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:BrCanvas",
                0);

	b->xcontext = gr_create_xcontext(b->canvas,
				canvas_paint_window(b->canvas), gr_color);
	xv_set(b->xcontext->drawable, WIN_CLIENT_DATA, c, 0);
	(void)gr_init(b->xcontext);

	(void) notify_interpose_event_func(b->xcontext->drawable,
                        mb_canvas_notify, NOTIFY_SAFE);

        (void)xv_set(b->frame, XV_LABEL,  LGET("CM: Multiple Calendars") , 0);
        (void)xv_set(b->frame, XV_WIDTH, xv_get(b->datetext, XV_X)+
			xv_get(b->datetext, XV_WIDTH)+10, NULL);

	(void)xv_set(b->frame, XV_SHOW, TRUE, NULL);
	backup_values(c);
	return (caddr_t)b;
}
static void
mb_paint_canvas(b)
	Browser *b;
{
	mb_init_browchart(b);
	mb_refresh_canvas(b);
}

extern void
mb_draw_chartgrid(b)
        Browser *b;
{
        int    x, y;
        int     n;
	Calendar	*c = (Calendar*)xv_get(b->box, PANEL_CLIENT_DATA);
	Props	*p = (Props*)c->properties;
        int     char_width_small = xv_get(c->fonts->lucida10, FONT_COLUMN_WIDTH)-3;
        int     char_height = xv_get(c->fonts->lucida12b, FONT_DEFAULT_CHAR_HEIGHT);
        int     char_width = xv_get(c->fonts->lucida12b, FONT_COLUMN_WIDTH);
        char    label[5], buf[160];
        XContext *xc = b->xcontext;
	int dayy, dayx, dayweek;
	Tick daytick;


        /*      Draw chart. It'll be filled in later.
                Draw grid lines and hourly labels.      */
        x = b->chart_x;
        y = b->chart_y;

	/* clear header */
	gr_clear_area(xc, 0, 0, b->canvas_w, b->chart_y);
	label[0] = '\0';

	/* draw hour labels */
        for (n = p->begin_slider_VAL; n <= p->end_slider_VAL; n++) {
		if (p->default_disp_VAL == hour12)
                	sprintf(label, "%2d", n > 12 ? n - 12 : n);
		else
                	sprintf(label, "%2d", n);
                gr_text(xc, c->view->outside_margin-char_width_small-3, y+3,
                        c->fonts->lucida10b, label, NULL);
                gr_draw_line(xc, x, y, x + b->chart_width,
			 y, gr_solid, NULL);
                y += b->boxh;
        }

        /*
         * Draw vertical lines and day labels
         */
        y = b->chart_y;
	dayy = y - char_height - 4; 
	dayweek = dow(b->date);
	daytick = last_ndays(b->date, dayweek == 0 ? 6 : dayweek-1);
	x = gr_center(b->boxw, days3[n+1], c->fonts->lucida12b) - char_width/2;
	dayx = gr_center(b->boxw, numbers[n+1], c->fonts->lucida12b);

	/* draw month */
	format_date(b->begin_week_tick+1, p->ordering_VAL, buf, 0);
	gr_text(xc, c->view->outside_margin+4,
		 dayy-char_height-4, c->fonts->lucida12b, buf, NULL);

        for (n = 0; n < 7; n++) {
                gr_text(xc, b->chart_x + (b->boxw * n) + dayx,
                        dayy, c->fonts->lucida12b, days3[n+1], NULL);
                gr_text(xc, b->chart_x + (b->boxw * n) + x,
                        y - char_height / 2, c->fonts->lucida12b, 
			numbers[dom(daytick)], NULL);
		daytick += daysec;
                gr_draw_line(xc, b->chart_x + (b->boxw * n),
                        y, b->chart_x + (b->boxw * n),
                        y + b->chart_height, gr_solid, NULL);
        }

        /*
         * Draw box around the whole thing.
         */
        gr_draw_box(xc, b->chart_x, b->chart_y, b->chart_width, b->chart_height, NULL);
        gr_draw_box(xc, b->chart_x-1, b->chart_y-1, b->chart_width+2, b->chart_height+2, NULL);      
}
extern void
mb_draw_appts(b, start, end)
	Browser *b;
	int start, end;
{
	Calendar *c = (Calendar*)xv_get(b->frame, WIN_CLIENT_DATA);
	int x, y, h, i;
	int end_of_day;
	
	h = (b->boxh/BOX_SEG);
	end_of_day = (b->segs_in_array / 7);

        y = b->chart_y + (start % end_of_day) * h;
        x = b->chart_x + (start/end_of_day * b->boxw);

	i = start;
	while (i < end) {
		if (b->multi_array[i] <= 0) {
			gr_clear_area(b->xcontext, x, y, b->boxw, h);
			y += h;
			i++;
		}
		else if (b->multi_array[i] == 1) {
			/* batch up for one repaint */
			if ( ((i+1) < b->segs_in_array)
				 && b->multi_array[i+1] == 1 && 
				( ((i+1) % end_of_day) != 0)) {
				h += (b->boxh/BOX_SEG);
				if (++i < end)
					continue;
			}
			if (c->xcontext->screen_depth < 8)
				gr_make_gray(b->xcontext, x, y, b->boxw, h, 25);
			else
				gr_make_grayshade(b->xcontext, x, y, b->boxw, h, 
							LIGHTGREY);
			y += h;
			h = (b->boxh/BOX_SEG);
			i++;
		}
		else if (b->multi_array[i] == 2) {
			/* batch up for one repaint */
			if ( ((i+1) < b->segs_in_array)
				 && b->multi_array[i+1] == 2 && 
				( ((i+1) % end_of_day) != 0) ) {
				h += (b->boxh/BOX_SEG);
				if (++i < end)
					continue;
			}
			if (c->xcontext->screen_depth < 8)
				gr_make_gray(b->xcontext, x, y, b->boxw, h, 50);
			else
				gr_make_rgbcolor(b->xcontext, x, y, b->boxw, h, 
							MIDGREY, MIDGREY, MIDGREY);		
			y += h;
			h = (b->boxh/BOX_SEG);
			i++;
		}
		else if (b->multi_array[i] >= 3) {
			/* batch up for one repaint */
			if ( ((i+1) < b->segs_in_array) 
				&& b->multi_array[i+1] >= 3 && 
				( ((i+1) % end_of_day) != 0) ) {
				h += (b->boxh/BOX_SEG);
				if (++i < end)
					continue;
			}
			if (c->xcontext->screen_depth < 8)
				gr_make_gray(b->xcontext, x, y, b->boxw, h, 75);
			else
				gr_make_grayshade(b->xcontext, x, y, b->boxw, h, 
							DIMGREY);
			y += h;
			h = (b->boxh/BOX_SEG);
			i++;
		}
		if (i != 0 && ((i % end_of_day) == 0)) {
                        x += b->boxw;
                        y = b->chart_y;
			h = (b->boxh/BOX_SEG);
                }
	}
	browser_select(b, NULL);

}
/* ARGSUSED */
extern Notify_value
sched_proc(item, event)
        Panel_item item;
        Event *event;
{
	int i;
        Calendar *c = (Calendar*)xv_get(item, PANEL_CLIENT_DATA);
        Editor *e = (Editor*)c->editor;

	if (!editor_exists(e))
		make_editor(c);
        if (cal_update_props()) {
                for (i=0; i < NO_OF_PANES; i++) {
                        set_rc_vals((Props*)calendar->properties, i);
                }
                set_defaults(e);
        }
	reset_ticks(c);
	mb_reset_time(c);
	mb_add_times(e->apptbox);
	show_editor(c);
	mb_reset_calbox(c);

        return NOTIFY_DONE;
}

extern void
update_browser_display2(b, p)
        Browser *b;
        Props *p;
{
        free(b->multi_array);
        gr_clear_area(b->xcontext, 0, 0, b->canvas_w, b->canvas_h);
        mb_init_array(b, p->begin_slider_VAL, p->end_slider_VAL);
        b->add_to_array = true;
        list_items_selected(b->box, mb_update_array);
        mb_refresh_canvas(b);
}
/* this is for browser_selection, must readjust col */
static void
adjust_row(b, new_begin, new_end)
        Browser *b;
        int new_begin, new_end;
{
        struct pr_pos xy;
	extern int temp_begin_slider_VAL, temp_end_slider_VAL;

        browser_deselect(b);
        b->row_sel += temp_begin_slider_VAL - new_begin;
        if (b->row_sel < 0)
                b->row_sel = 0;
        else if ((b->row_sel + new_begin) >= new_end)
                b->row_sel = new_end - new_begin - 1;

        xy.x = b->col_sel;
        xy.y = b->row_sel;
        browser_select(b, &xy);
        temp_begin_slider_VAL = new_begin;
        temp_end_slider_VAL = new_end;

}
extern void
update_browser_display(b, p)
        Browser *b;
        Props *p;
{
        free(b->multi_array);
        gr_clear_area(b->xcontext, 0, 0, b->canvas_w, b->canvas_h);
        mb_init_browchart(b);
        mb_init_array(b, p->begin_slider_VAL, p->end_slider_VAL);
        adjust_row(b, p->begin_slider_VAL, p->end_slider_VAL);
        b->add_to_array = true;
        list_items_selected(b->box, mb_update_array);
        list_items_selected(b->box, mb_update_busyicon);
        mb_paint_canvas(b);
}

