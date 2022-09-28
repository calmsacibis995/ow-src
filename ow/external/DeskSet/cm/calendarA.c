#ifndef lint
static  char sccsid[] = "@(#)calendarA.c 3.81 97/05/14 Copyr 1991 Sun Microsystems, Inc.";
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
#include <sys/stat.h>
#include "appt.h"
#include <sys/param.h> /* MAXPATHLEN defined here */
#ifdef SVR4
#include <sys/systeminfo.h>
#endif "SVR4"
#include <dirent.h>
#include <ctype.h>
#include <tzfile.h>
#include <string.h>
#include <sys/time.h>
#include <rpc/rpc.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <X11/Xlib.h>
#include <xview/xview.h>
#include <xview/notice.h>
#include <xview/cms.h>
#include <xview/xv_xrect.h>
#include <xview/xv_error.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/scrollbar.h>
#include <xview/icon_load.h>
#include <xview/svrimage.h>
#include <xview/font.h>
#include <xview/openmenu.h>
#include <ds_listbx.h>
#include "util.h"
#include "timeops.h"
#include "datefield.h"
#include "props.h"
#include "select.h"
#include "editor.h"
#include "graphics.h"
#include "browser.h"
#include "blist.h"
#include "table.h"
#include "alarm.h"
#include "dayglance.h"
#include "yearglance.h"
#include "weekglance.h"
#include "calendar.h"
#include "ds_popup.h"
#include "todo.h"
#include "alist.h"
#include "find.h"
#include "goto.h"
#include "tempbr.h"
#include "gettext.h"
#include "namesvc.h"
#include "defnamesvc.h"
#include "monthglance.h"

static unsigned short cm_image[] = {
#include "cmgr.icon"
};
static unsigned short cm_mask[] = {
#include "cmgrmask.icon"
};
static Server_image icon_image;
static Server_image icon_mask;

#define	ITIMER_NULL ((struct itimerval *) 0)
#define VIEWMARGIN		15
#define NUM_MITEMS_PERCOL 	27
/* this must be updated with each new release! */
#define CM_VERSION             4

#define FIVE_MINUTES            (5*minsec)

extern time_t timelocal();	/* LINT */
extern char * getlogin();	/* LINT */
extern int errno;

static struct itimerval timer, rtimer;
static Menu make_tzmenu();
extern Notify_value update_handler();
extern void paint_grid();
extern Stat paint_year();
extern Stat paint_canvas();
extern caddr_t make_postup();
extern void notify_enable_rpc_svc();
extern int xv_decode_drop();
void init_strings();

static Notify_value reset_reminders();
static int system_time;

int		child;
int		yyylineno;
int		debug=0;
int		expert=0;

char            cmtarget[MAXNAMELEN]="";
int		cmicon=3;
Calendar	*calendar;

int		gargc;		/* saved for tooltalk initialization */
char		**gargv;

extern void
cache_dims(c, w, h)
	Calendar *c; int w, h;	
{
	Glance glance;
	Props *p = (Props*)c->properties;
	int margin;
        
	c->view->winw = w;
	c->view->winh = h;
	margin = c->view->outside_margin;
	glance = c->view->glance;
	switch(glance) {
		case weekGlance:
			c->view->boxw = (w-2*margin)/5;
			c->view->boxh = (h-4*margin)/2 - 
					(2*(int) xv_get(c->fonts->lucida12b,
					 FONT_COLUMN_WIDTH)); 
			c->view->topoffset = 3*margin;
			break;
		case monthGlance:
			c->view->boxw = (w-2*margin)/7;
			c->view->topoffset = 3*margin;
			c->view->boxh = (h-c->view->topoffset-
						2*margin)/c->view->nwks;
			break;
		case yearGlance:
			c->view->boxw = 20* (int) xv_get(
				c->fonts->lucida10,
				FONT_COLUMN_WIDTH) + margin;
			c->view->boxh = 7* (int) xv_get(
				c->fonts->lucida10,
				FONT_DEFAULT_CHAR_HEIGHT) + 
				2*xv_get(c->fonts->lucida12b,
				FONT_DEFAULT_CHAR_HEIGHT);
			c->view->topoffset=20;
			break;
		case dayGlance:
			c->view->topoffset = 30;
			c->view->boxw = APPT_AREA_WIDTH;
			c->view->boxh = (h - c->view->topoffset)
			 	/ (p->end_slider_VAL - p->begin_slider_VAL); 
			break;
		default:
			break;
	}
}
/* ARGSUSED */
void
resize_proc(canvas, width, height)
	Canvas canvas;
	int width, height;
{
	Calendar *c = (Calendar *)window_get(canvas, WIN_CLIENT_DATA);
	XContext *xc = c->xcontext;
	int 	gap=3;
	int offset;
	int margin = 10;
	int cw, moving_left = false, safe_offset;
	short safe_wid;
	Rect *b_rect, *s_rect, *f_rect, *safe_rect;
	short b_wid, s_wid, f_wid;

        /*      This is gross.  Do this to get around the fact 
                the resize proc is called before window_main_loop 
                when everything is set up correctly.    */ 
        if (xc==NULL) return;
	if (c->view->glance == dayGlance)
		init_dayview(c);
	gr_clear_area(xc, 0, 0, width, height);

	/* 
	 * 	Reposition the past/present/future button 
	 */

	offset  = (int) xv_get(c->frame, XV_WIDTH);

	/* assumes this gets called before cache_dims! */
	cw = (int) xv_get(c->canvas, XV_WIDTH);
	if (cw < c->view->winw)
		moving_left = true;

	/* get width of rect which we do not want to overwrite */
	safe_rect = (Rect *) xv_get(c->items->button5, PANEL_ITEM_RECT);
	safe_wid = (short) safe_rect->r_width;

	/* get x offset of non-overwritable rect + width + margin */
	safe_offset = safe_wid + (int) xv_get(c->items->button5, XV_X)+gap;
	/* get rects for the previous, present & future buttons */
	b_rect = (Rect *) xv_get(c->items->button8, PANEL_ITEM_RECT);
	s_rect = (Rect *) xv_get(c->items->button9, PANEL_ITEM_RECT);
	f_rect = (Rect *) xv_get(c->items->button10, PANEL_ITEM_RECT);

	/* get widths of previous, present & future buttons */
	b_wid = (short) b_rect->r_width;
	s_wid = (short) s_rect->r_width;
	f_wid = (short) f_rect->r_width;

	offset -= (b_wid+s_wid+f_wid+margin+2*gap);

	if (offset < safe_offset) offset = safe_offset;

	/* move the past/present/future button with right edge */
	if (moving_left) {
		(void)xv_set(c->items->button8, 
				XV_X, offset, NULL);
		(void)xv_set(c->items->button9, 
				XV_X, offset+b_wid+gap, NULL);
		(void)xv_set(c->items->button10, 
				XV_X, offset+b_wid+s_wid+2*gap, NULL);
	}
	else {
		(void)xv_set(c->items->button10,
				XV_X, offset+b_wid+s_wid+2*gap, NULL);
		(void)xv_set(c->items->button9, 
				XV_X, offset+b_wid+gap, NULL);
		(void)xv_set(c->items->button8, 
				XV_X, offset, NULL);
	}
}

	
/* ARGSUSED */
void
repaint_proc(canvas, paintw, dpy, xwin, area)
	Canvas canvas;
        Xv_Window paintw;
	Display *dpy;
	Window xwin;
        Xv_xrectlist *area;
{
	char *label;
	char 	buf[MAXNAMELEN];
	/* Stuff to compress exposures */
	XRectangle *xr	= area->rect_array;
	XEvent ev;
	XRectangle clip;
	int i;
	int minx	= xr->x;
	int miny	= xr->y;
	int maxx	= minx + xr->width;
	int maxy	= miny + xr->height;
	int w, h;
	Rect 		cliprect;

	Calendar *c;
	XContext *xc;

	if ((c = (Calendar *) xv_get(canvas, WIN_CLIENT_DATA))==NULL) 
		return;
	if ((xc = c->xcontext)==NULL)
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

	cache_dims(c, (int)xv_get(canvas, XV_WIDTH), 
			(int)xv_get(canvas, XV_HEIGHT));

	cliprect.r_left = clip.x;
	cliprect.r_top = clip.y;
	cliprect.r_width = clip.width;
	cliprect.r_height = clip.height;
	paint_canvas(c, &cliprect);
	
	xc->gcvals->clip_mask = None;
	gr_clear_clip_rectangles(xc);
	XSync(dpy, 0);
	if ((label = (char*)xv_get(c->frame, XV_LABEL)) == NULL || 
		label[0] == '\0') {
		(void)sprintf(buf, "%s: %s", (char*)cm_get_relname(), 
				c->view->current_calendar);
		(void)xv_set(c->frame, XV_LABEL, buf, 0);
	}
}
/* ARGSUSED */
static void
show_tempbr(m, mi)
	Menu m;
	Menu_item mi;
{
	Calendar *c;
	Tempbr *t;

	c = (Calendar *) xv_get(m, MENU_CLIENT_DATA);
	t = (Tempbr*)c->tempbr; 

	if (t == NULL || t->frame == NULL) {
                c->tempbr = (caddr_t)make_tempbr(c);
		t = (Tempbr*)c->tempbr; 
        }
	(void)xv_set(t->frame, XV_SHOW, TRUE, NULL);
	(void)xv_set(t->frame, FRAME_LEFT_FOOTER, "", NULL);
}
/* ARGSUSED */
static void
show_goto(m, mi)
	Menu m;
	Menu_item mi;
{
	Calendar *c;
	Goto *g;

	c = (Calendar *) xv_get(m, MENU_CLIENT_DATA);
	g = (Goto*)c->goTo; 

	if (g == NULL || g->frame == NULL) {
                c->goTo = (caddr_t)make_goto(c);
		g = (Goto*)c->goTo; 
        }
	(void)xv_set(g->frame, XV_SHOW, TRUE, NULL);
}


/* ARGSUSED */
static void
show_find(m, mi)
	Menu m;
	Menu_item mi;
{
	Calendar *c;
	Find *f;

	c = (Calendar *) xv_get(m, MENU_CLIENT_DATA);
	f = (Find*)c->find; 

	if (f == NULL || f->frame == NULL) {
                c->find = (caddr_t)make_find(c);
		f = (Find*)c->find; 
        }
	(void)xv_set(f->frame, XV_SHOW, TRUE, NULL);
	
}

/* ARGSUSED */
static void
show_browser(m, mi)
	Menu m;
	Menu_item mi;
{
	Calendar *c = (Calendar *) xv_get(m, MENU_CLIENT_DATA);
	Browser *b = (Browser *) c->browser;
	Editor *e = (Editor *) c->editor;

	if (b == NULL) {
                c->browser = (caddr_t)make_browser(c);
                (void)mb_init_browser(c);
                b = (Browser *) c->browser;
        }
	else {
		if (cal_update_props()) {
			list_flush(b->box);
			mb_init_blist(c);
		}
		if (!xv_get(b->frame, XV_SHOW)) {
			mb_init_canvas(c);
			mb_refresh_canvas(b);
		}
		xv_set(b->frame, XV_SHOW, TRUE, NULL);
	}
	if (editor_showing(e))
		add_to_calbox(c, c->calname);
}
/* ARGSUSED */
extern void
show_blist(m, mi)
        Menu m;
        Menu_item mi;
{
        Calendar *c;
        Browselist *bl;

        c = calendar;
        bl = (Browselist *) c->browselist;

        if (bl == NULL) {
                make_blist(c);
                bl = (Browselist *) c->browselist;
        }
	if (!browselist_showing(bl))
        	ds_position_popup(c->frame, bl->frame, DS_POPUP_LOR);
        (void)xv_set(bl->frame, XV_SHOW, TRUE, NULL);
	xv_set(bl->frame, FRAME_LEFT_FOOTER, "", NULL);
}

/* ARGSUSED */
static Notify_value
event_proc(canvas, event, arg, when)
	Notify_client     canvas;
	Event		  *event;
	Notify_arg        arg;
	Notify_event_type when;
{
	Calendar *c;
	Props *p;
	Glance glance;
	Find *f;

	c = (Calendar *) xv_get(canvas, WIN_CLIENT_DATA); 
	p = (Props *) c->properties;
	glance = c->view->glance;

	switch(event_action(event)) {
		case ACTION_PROPS:
			(void)p_show_proc(p->frame);
			break;
		case ACTION_SELECT:
		case ACTION_MENU:
		case LOC_DRAG:
			switch(glance) {
				case monthGlance:
					month_event(c, event);
					break;
				case weekGlance:
					week_event(c, event);
					break;
				case dayGlance:
					day_event(c, event);
					break;
				case yearGlance:
					year_event(c, event);
					break;
			}
			c->general->last_canvas_touched = main_win;
			break;
		case ACTION_FIND_FORWARD:
		case ACTION_FIND_BACKWARD:
        		f = (Find*)c->find;
        		if (f == NULL || f->frame == NULL) { 
        	        	c->find = (caddr_t)make_find(c);
                		f = (Find*)c->find;
			}
        		(void)xv_set(f->frame, XV_SHOW, TRUE, NULL);
			break;
		default:
			break;
	}
	
	if (event_action(event) == ACTION_HELP && event_is_up(event))
	{
		switch(glance) {
			case monthGlance:
				xv_help_show(canvas, "cm:MonthView", event);
				break;
			case weekGlance:
				xv_help_show(canvas, "cm:WeekView", event);
				break;
			case dayGlance:
				xv_help_show(canvas, "cm:DayView", event);
				break;
			case yearGlance:
				xv_help_show(canvas, "cm:YearView", event);
				break;
		}
		return(NOTIFY_DONE);
	} else
		return (notify_next_event_func(canvas, (Notify_event)event, arg, when));
}


/* ARGSUSED */
static Notify_value
prev_button(item, event)
	Panel_item item;
	Event *event;
{
	Tick date;
	Panel_attribute parent;
	Calendar *c;
	XContext *xc;

	parent = (Panel_attribute) xv_get(item, XV_OWNER);
	c = (Calendar *) xv_get((Xv_Window)parent, WIN_CLIENT_DATA);
	xc = c->xcontext;
	switch(c->view->glance) {
		case weekGlance:
			c->view->olddate = c->view->date;
			c->view->date = last_ndays(c->view->date, 7);
			c->view->nwks = numwks(c->view->date); 
			paint_weekview(c, NULL);
		break;
		case monthGlance:
			date = previousmonth(c->view->date);
			if (timeok(date)) {
				c->view->nwks = numwks(date);
				calendar_deselect(c);
				c->view->olddate = c->view->date;
				c->view->date = date;
				c->view->nwks = numwks(c->view->date);
				gr_clear_area(xc, 0, 0, c->view->winw,
					c->view->winh);
				(void)paint_monthview(c, NULL);
			}
			calendar_select(c, daySelect, NULL);
			break;
		case yearGlance:
			date = lastjan1(c->view->date);
			if (timeok(date)) {
				c->view->olddate = c->view->date;
				c->view->date = date;
				c->view->nwks = numwks(c->view->date); 
				paint_year(c, NULL);
			}
			calendar_select(c, monthSelect, NULL);
			break;
		case dayGlance:
			c->view->olddate = c->view->date;
			c->view->date = last_ndays(c->view->date, 1);
			c->view->nwks = numwks(c->view->date);
			calendar_deselect(c);
			if (c->view->date <
				lowerbound(((Day *)(c->view->day_info))->month1)) { 
        			((Day *)(c->view->day_info))->month3 = c->view->date;
				((Day *)(c->view->day_info))->month2 = previousmonth(
							c->view->date);
				((Day *)(c->view->day_info))->month1 = previousmonth(
					((Day *)(c->view->day_info))->month2);
				init_dayview(c);
				paint_dayview(c, true, NULL);
			}
			else { 
				monthbox_deselect(c);
				monthbox_datetoxy(c);
				paint_dayview(c, false, NULL);
				monthbox_select(c);
			}
			calendar_select(c, hourSelect, NULL);
			break;
		default:
			break;
	}
	return(NOTIFY_DONE);
}

/* ARGSUSED */
static Notify_value
next_button(item, event)
	Panel_item item;
	Event *event;
{
	Tick date;
	Calendar *c;
	Panel_attribute parent;
	XContext *xc;

	parent = (Panel_attribute) xv_get(item, XV_OWNER);
	c = (Calendar *) xv_get((Xv_Window)parent, WIN_CLIENT_DATA);
	xc = c->xcontext;
	switch(c->view->glance) {
		case weekGlance:
			c->view->olddate = c->view->date;
			c->view->date = next_ndays(c->view->date, 7);
			c->view->nwks = numwks(c->view->date);
			paint_weekview(c, NULL);
			break;
		case monthGlance:
			date = nextmonth(c->view->date);
			if (timeok(date)) {
				calendar_deselect(c);
				c->view->olddate = c->view->date;
				c->view->date = date;
				c->view->nwks = numwks(date);
				gr_clear_area(xc, 0, 0, c->view->winw,
					c->view->winh);
				(void)paint_monthview(c, NULL);
			}
			calendar_select(c, daySelect, NULL);
			break;
		case yearGlance:
			date = nextjan1(c->view->date);
			if (timeok(date)) {
				c->view->olddate = c->view->date;
				c->view->date = date;
				c->view->nwks = numwks(date);
				paint_year(c, NULL);
			}
			calendar_select(c, monthSelect, NULL);
			break;
		case dayGlance:
			c->view->olddate = c->view->date;
			c->view->date = next_ndays(c->view->date, 1);
			c->view->nwks = numwks(c->view->date);
			/* beyond month range displayed */
			if (c->view->date > 
				upperbound(last_dom(
					(((Day *)(c->view->day_info))->month3)))) {
				((Day *)(c->view->day_info))->month1 = c->view->date;
        			((Day *)(c->view->day_info))->month2 = nextmonth(
							c->view->date);
				((Day *)(c->view->day_info))->month3 = nextmonth(
					((Day *)(c->view->day_info))->month2);
				calendar_deselect(c);
				init_dayview(c);
				paint_dayview(c, true, NULL);
				calendar_select(c, hourSelect, NULL);
			}
			else {
				calendar_deselect(c);
				monthbox_deselect(c);
				monthbox_datetoxy(c);
				paint_dayview(c, false, NULL);
				calendar_select(c, hourSelect, NULL);
				monthbox_select(c);
			}
			break;
		default:
			break;
	}
	return(NOTIFY_DONE);
}

/* ARGSUSED */
extern Notify_value
today_button(item, event)
	Panel_item item;
	Event *event;
{
	Panel_attribute parent;
	Calendar *c;
	Tick today, date;
	XContext *xc;
	int mo_da, mo_to;
	struct pr_pos xy;

	parent = (Panel_attribute) xv_get(item, XV_OWNER);
	c = (Calendar *) xv_get((Xv_Window)parent, WIN_CLIENT_DATA);
	date = c->view->date;
	c->view->olddate = c->view->date;
	c->view->date = today = now();
	mo_da = month(date);
	mo_to = month(today);
	c->view->nwks = numwks(today);
	calendar_deselect(c);
	switch(c->view ->glance) {
	case weekGlance:
		if (mo_da != mo_to || dom(date) != dom(today) ||
			year(date) != year(today)) {
			paint_weekview(c, NULL);
		}
		calendar_select(c, weekdaySelect, NULL);
		break;
	case monthGlance:
		if (mo_da != mo_to || year(date) != year(today)) {
			xc = c->xcontext;
			gr_clear_area(xc, 0, 0, c->view->winw,
				c->view->winh);
			(void)paint_monthview(c, NULL);
		}
		calendar_select(c, daySelect, (caddr_t)NULL);
		break;
	case yearGlance:
		if (year(date) != year(today)) 
			paint_year(c, NULL);
		else calendar_deselect(c); 
		xy.y = month_row_col[mo_to-1][ROW];
		xy.x = month_row_col[mo_to-1][COL];
		calendar_select(c, monthSelect, (caddr_t)&xy);
		break;
	case dayGlance:
		if (mo_da != mo_to || dom(date) != dom(today) ||
				year(date) != year(today)) {
			calendar_deselect(c);
			init_mo(c);
			init_dayview(c);
			paint_dayview(c, true, NULL);
		}
		calendar_select(c, hourSelect,(caddr_t)NULL);
		break;
	default:
		break;
	}
	return(NOTIFY_DONE);
}

static Menu
gen_tzmenu(item, op)
	Menu_item item;
	Menu_generate op;
{
	char *path;
	static Menu pullright;

	switch(op) {
	case MENU_DISPLAY	: 
		if (pullright == NULL) {
			path = (char *) xv_get(item, MENU_CLIENT_DATA);
			pullright = make_tzmenu(path);
			break;
		}
	case MENU_DISPLAY_DONE :
	case MENU_NOTIFY	:
	case MENU_NOTIFY_DONE	: break ;
	}
	return(pullright);
}

extern void
set_default_view(c, view)
	Calendar *c;
	int view;
{
	Menu menu;

	menu = (Menu)xv_get(c->items->button2, PANEL_ITEM_MENU);
	switch (view) {
		case 0:
			xv_set(menu, MENU_DEFAULT, 4, NULL); 
			break;
		case 1: 
			xv_set(menu, MENU_DEFAULT, 3, NULL); 
			break;
		case 2: 
			xv_set(menu, MENU_DEFAULT, 2, NULL); 
			break;
		case 3: 
			xv_set(menu, MENU_DEFAULT, 1, NULL); 
			break;
		default:
			xv_set(menu, MENU_DEFAULT, 3, NULL); 
			break;
	}
}

static Menu
make_view_menu(c)
	Calendar *c;
{
	Menu view_menu;
	Menu year_pullright, day_pullright, week_pullright, mo_pullright;
	extern Notify_value month_button(), year_button();
	extern void cm_show_todo(), cm_show_appts();

	day_pullright = menu_create (
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_ACTION_ITEM,  LGET("Day View") ,  day_button,
                        MENU_VALUE, dayGlance,
                        XV_HELP_DATA, "cm:DayPR",
                        0),
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_ACTION_ITEM,  LGET("Appt List...") ,  cm_show_appts,
                        MENU_VALUE, dayGlance,
                        XV_HELP_DATA, "cm:DayPR",
                        0),
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_ACTION_ITEM,  LGET("ToDo List...") ,  cm_show_todo,
                        MENU_VALUE, dayGlance,
                        XV_HELP_DATA, "cm:DayPR",
                        0),
                MENU_CLIENT_DATA, c,
                0);
        week_pullright = menu_create (
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_ACTION_ITEM,  LGET("Week View") ,  week_button,
                        MENU_VALUE, weekGlance,
                        XV_HELP_DATA, "cm:WeekPR",
                        0),
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_ACTION_ITEM,  LGET("Appt List...") ,  cm_show_appts,
                        MENU_VALUE, weekGlance,
                        XV_HELP_DATA, "cm:WeekPR",
                        0),
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_ACTION_ITEM,  LGET("ToDo List...") ,  cm_show_todo,
                        MENU_VALUE, weekGlance,
                        XV_HELP_DATA, "cm:WeekPR",
                        0),
                MENU_CLIENT_DATA, c,
                0);
	mo_pullright = menu_create (
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_ACTION_ITEM,  LGET("Month View") ,  month_button,                        MENU_VALUE, monthGlance,
                        XV_HELP_DATA, "cm:MoPR",
                        0),
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_ACTION_ITEM,  LGET("Appt List...") ,  cm_show_appts,
                        MENU_VALUE, monthGlance,
                        XV_HELP_DATA, "cm:MoPR",
                        0),
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_ACTION_ITEM,  LGET("ToDo List...") ,  cm_show_todo,
                        MENU_VALUE, monthGlance,
                        XV_HELP_DATA, "cm:MoPR",
                        0),
                MENU_CLIENT_DATA, c,
                0);
        year_pullright = menu_create (
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_ACTION_ITEM,  LGET("Yr View") ,  year_button,
                        MENU_VALUE, yearGlance,
                        XV_HELP_DATA, "cm:YrPR",
                        0),
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_ACTION_ITEM,  LGET("Appt List...") ,  cm_show_appts,
                        MENU_VALUE, yearGlance,
                        XV_HELP_DATA, "cm:YrPR",
                        0),
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_ACTION_ITEM,  LGET("ToDo List...") ,  cm_show_todo,
                        MENU_VALUE, yearGlance,
                        XV_HELP_DATA, "cm:YrPR",
                        0),
                MENU_CLIENT_DATA, c,
               	0);

	view_menu = menu_create(
		MENU_APPEND_ITEM, menu_create_item(
			MENU_PULLRIGHT_ITEM,  LGET("Day") ,   day_pullright,
			XV_HELP_DATA, "cm:ViewDay",
			0),
		MENU_APPEND_ITEM, menu_create_item(
			MENU_PULLRIGHT_ITEM,  LGET("Week") ,  week_pullright,  
			XV_HELP_DATA, "cm:ViewWeek",
			0),
		MENU_APPEND_ITEM, menu_create_item(
			MENU_PULLRIGHT_ITEM,  LGET("Month") , mo_pullright,
			XV_HELP_DATA, "cm:ViewMonth",
			0),
		MENU_APPEND_ITEM, menu_create_item(
			MENU_PULLRIGHT_ITEM,  LGET("Year") ,  year_pullright,
			XV_HELP_DATA, "cm:ViewYear",
			0),
		MENU_APPEND_ITEM, menu_create_item(
			MENU_STRING,  "" ,
			MENU_FEEDBACK, FALSE,
			0),
                MENU_APPEND_ITEM, menu_create_item(
			MENU_STRING,  LGET("Time Zone") ,
			MENU_CLIENT_DATA,   TZDIR,
			MENU_GEN_PULLRIGHT, gen_tzmenu,
			XV_HELP_DATA, "cm:EditTimeZone",
			0),
		/* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL :
		 *
		 * Find = Find an occurance of a calendar appointment.
		 */
		MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("Find...") ,  show_find,
#ifdef KYBDACC
			MENU_ACCELERATOR, "coreset Find",
#endif
			XV_HELP_DATA, "cm:Find",
			0),
		MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("Go To...") ,  show_goto,
			XV_HELP_DATA, "cm:Goto",
			0),
		MENU_CLIENT_DATA, c,
		0);

	return(view_menu);
}

static Menu
gen_tzsubmenu(item, op)
	Menu_item item;
	Menu_generate op;
{
	char *path;
	Menu pullright=NULL;

	switch(op) {
	case MENU_DISPLAY	: 
			path = (char *) xv_get(item, MENU_CLIENT_DATA);
			pullright = make_tzmenu(path);
			break;
	case MENU_DISPLAY_DONE :
	case MENU_NOTIFY	:
	case MENU_NOTIFY_DONE	: break ;
	}
	return(pullright);
}


/* ARGSUSED */
static void
tzmenu_choose(menu, item)
	Menu menu; Menu_item item;
{
	char *tzbuf, *data, fstr[MAXSTRING], tz[MAXSTRING];

	sprintf(fstr, "%s/%%s", TZDIR) ;
        data = (char *) xv_get(item, MENU_CLIENT_DATA, 0) ;
        sscanf(data, fstr, tz) ;

#ifdef SVR4
	if (strcmp("localtime", data) == 0) {
		data = (char *) xv_get(item, MENU_STRING);
		cm_strcpy(tz, data);
	}
#endif
	set_timezone(tz);

	if (tz != NULL) {
#ifdef SVR4
		data = (char *) xv_get(item, MENU_CLIENT_DATA);
		if (strcmp("localtime", data) == 0) 
#else
		if (strcmp("localtime", tz) == 0) 
#endif "SVR4"
			xv_set(calendar->frame, FRAME_RIGHT_FOOTER, "",
				NULL);
		else {
			tzbuf = ckalloc(cm_strlen(tz)+cm_strlen(MGET("Time Zone: %s"))+1); 
			sprintf(tzbuf, MGET("Time Zone: %s"), tz); 	
			xv_set(calendar->frame, FRAME_RIGHT_FOOTER, tzbuf, 
				NULL);
			free(tzbuf);
		}
	}
	paint_canvas(calendar, NULL);
	if (editor_showing(calendar->editor))
 		add_times(((Editor*)calendar->editor)->apptbox);
}

static Menu
make_tzmenu(path)
	char *path;
{

	char *fname;		/* Pointer to current filename. */
	char *pname;		/* Pointer to current pathname. */
	DIR *dirp;		/* Stream for monitoring directory. */
	struct dirent *filep;	/* Pointer to current directory entry. */
	struct stat cstat;	/* For information on current file. */
	Menu menu;
	Menu_item item;
	static char tz[MAXSTRING];
	static Boolean default_menu_set;

	menu = (Menu) xv_create(XV_NULL, MENU, 0);
	if ((dirp = opendir(path)) == NULL) {
		fprintf(stderr,  EGET("Couldn't get timezone information.\n") );
		return(menu) ;
	}
	while ((filep = readdir(dirp)) != NULL) {
#ifdef SVR4
		if (filep->d_ino == 0) continue;
#else
		if (filep->d_fileno == 0) continue;
#endif "SVR4"
		if (!DOTS(filep->d_name))  { /* Is it the . or .. entry? */
          		fname = malloc((unsigned) (cm_strlen(filep->d_name)+1)) ;
			cm_strcpy(fname, filep->d_name) ;
			pname = malloc((unsigned) (cm_strlen(path) + 
					cm_strlen(filep->d_name)+2)) ;
			sprintf(pname, "%s/%s", path, filep->d_name) ;
			stat(pname, &cstat) ;
			/* Directory? */
			if ((cstat.st_mode & S_IFMT) == S_IFDIR) {  
				item = xv_create(XV_NULL,  MENUITEM,
					MENU_CLIENT_DATA,   pname,
					MENU_STRING,        fname,
					MENU_GEN_PULLRIGHT, gen_tzsubmenu,
                       	        0) ;
				xv_set(menu, MENU_APPEND_ITEM, item, 0) ;
			}
			/* Ordinary file? */
			else if ((cstat.st_mode & S_IFMT) == S_IFREG) {
				item = xv_create(XV_NULL, MENUITEM,
                             	MENU_CLIENT_DATA, pname,
                             	MENU_ACTION_ITEM, fname, tzmenu_choose,
                             	0);
				xv_set(menu, MENU_APPEND_ITEM, item, 0) ;
			}
#ifndef SVR4
			/* default is localtime */
			if (strcmp(filep->d_name, "localtime") == 0) {
				(void)xv_set(menu, MENU_DEFAULT_ITEM, item, NULL);
			}
#endif "SVR4"
		}
	}   
#ifdef SVR4
	if (!default_menu_set) {
		cm_strcpy(tz, (char*)getenv("TZ"));
		item = xv_create(XV_NULL, MENUITEM,
	     		MENU_CLIENT_DATA, "localtime",
	     		MENU_ACTION_ITEM, tz, tzmenu_choose,
	     		0);
		xv_set(menu, MENU_APPEND_ITEM, item, 
			MENU_DEFAULT_ITEM, item, NULL) ;
		default_menu_set = true;
	}
#endif "SVR4"
	closedir(dirp) ;
	return(menu) ;
}

static Notify_value
print_current(m, mi)
    Menu m;
    Menu_item mi;
{
        Calendar *c = (Calendar *)xv_get (m, MENU_CLIENT_DATA, 0);
	
	switch(c->view->glance) {
		case yearGlance:
			print_std_year(year(c->view->date));
		break;
		case monthGlance:
			print_month_pages(c);
		break;
		case weekGlance:
			print_week_pages(c);
		break;
		case dayGlance:
			print_day_pages(c);
		break;
		default:
			print_month_pages(c);
	}
	return NOTIFY_DONE;
}

static Menu
make_print_menu(c)
        Calendar *c;
{
        Menu print_menu;
        Menu year_pullright, day_pullright, week_pullright, mo_pullright;
	extern Notify_value ps_month_button(), ps_std_year_button();
	extern Notify_value ps_alt_year_button(), ps_todo_button();
	extern Notify_value ps_appt_button();

	if (c->todo == NULL) {
	   c->todo = (caddr_t) ckalloc(sizeof(Todo));
	                                    /* fix core dump bug */
                                            /* insure "todo" ptr exists */
	                                    /* (scavin 15Sep93) */
	}

          day_pullright = menu_create (
                MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("Day View") ,  ps_day_button,
			MENU_VALUE, dayGlance,
			XV_HELP_DATA, "cm:PrintDay",
			0),
                MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("Appt List") ,  ps_appt_button,
			MENU_VALUE, dayGlance,
			XV_HELP_DATA, "cm:PrintAppt",
			0),
                MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("To Do List") ,  ps_todo_button,
			MENU_VALUE, dayGlance,
			XV_HELP_DATA, "cm:PrintTodo",
			0),
                MENU_CLIENT_DATA, c,
                0);
        week_pullright = menu_create (
                MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("Week View") ,  ps_week_button,
			MENU_VALUE, weekGlance,
			XV_HELP_DATA, "cm:PrintWeek",
			0),
                MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("Appt List") ,  ps_appt_button,
			MENU_VALUE, weekGlance,
			XV_HELP_DATA, "cm:PrintAppt",
			0),
                MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("To Do List") ,  ps_todo_button,
			MENU_VALUE, weekGlance,
			XV_HELP_DATA, "cm:PrintTodo",
			0),
                MENU_CLIENT_DATA, c,
                0);
        mo_pullright = menu_create (
                MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("Month View") ,  ps_month_button,
			MENU_VALUE, monthGlance,
			XV_HELP_DATA, "cm:PrintMonth",
			0),
                MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("Appt List") ,  ps_appt_button,
			MENU_VALUE, monthGlance,
			XV_HELP_DATA, "cm:PrintAppt",
			0),
                MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("To Do List") ,  ps_todo_button,
			MENU_VALUE, monthGlance,
			XV_HELP_DATA, "cm:PrintTodo",
			0),
                MENU_CLIENT_DATA, c,
                0);
        year_pullright = menu_create (
                MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("Yr View (Std)") ,  ps_std_year_button,
			MENU_VALUE, yearGlance,
			XV_HELP_DATA, "cm:PrintYearStd",
			0),
                MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("Yr View (Alt)") ,  ps_alt_year_button,
			MENU_VALUE, yearGlance,
			XV_HELP_DATA, "cm:PrintYearAlt",
			0),
                MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("Appt List") ,  ps_appt_button,
			MENU_VALUE, yearGlance,
			XV_HELP_DATA, "cm:PrintAppt",
			0),
                MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("To Do List") ,  ps_todo_button,
			MENU_VALUE, yearGlance,
			XV_HELP_DATA, "cm:PrintTodo",
			0),
                MENU_CLIENT_DATA, c,
                0);

        print_menu = menu_create (
                MENU_APPEND_ITEM, menu_create_item(
			MENU_ACTION_ITEM,  LGET("Current View") ,  print_current,
#ifdef KYBDACC
                        MENU_ACCELERATOR, "coreset Print",
#endif
			XV_HELP_DATA, "cm:PrintCurrent",
			0),
                MENU_APPEND_ITEM, menu_create_item(
			MENU_PULLRIGHT_ITEM,  LGET("Day") ,   day_pullright,
			XV_HELP_DATA, "cm:PrintDay",
			0),
		MENU_APPEND_ITEM, menu_create_item(
			MENU_PULLRIGHT_ITEM,  LGET("Week") ,  week_pullright,
			XV_HELP_DATA, "cm:PrintWeek",
			0),
                MENU_APPEND_ITEM, menu_create_item(
			MENU_PULLRIGHT_ITEM,  LGET("Month") , mo_pullright,
			XV_HELP_DATA, "cm:PrintMonth",
			0),
                MENU_APPEND_ITEM, menu_create_item(
			MENU_PULLRIGHT_ITEM,  LGET("Year") , year_pullright,
			XV_HELP_DATA, "cm:PrintYear",
			0),
		MENU_CLIENT_DATA, c,
                0);
        return(print_menu);
}

make_edit_menu(c)
	Calendar *c;
{
	extern void cm_show_props();
	Menu edit_menu;

	edit_menu = menu_create(
		MENU_APPEND_ITEM, menu_create_item(
				MENU_ACTION_ITEM,  LGET("Appointment...") , new_editor, 
#ifdef KYBDACC
				MENU_ACCELERATOR, "coreset Open",
#endif
				XV_HELP_DATA, "cm:EditAppointment", 
		0),
                MENU_APPEND_ITEM, menu_create_item(
                                MENU_STRING,  "", 
				MENU_FEEDBACK, FALSE,
		0),
		MENU_APPEND_ITEM, menu_create_item(
				MENU_ACTION_ITEM,  LGET("Properties...") , cm_show_props, 
#ifdef KYBDACC
				MENU_ACCELERATOR, "coreset Props",
#endif
				XV_HELP_DATA, "cm:EditProperties", 
		0),
		MENU_CLIENT_DATA, c,
	0);
	return(edit_menu);
}

static void
mb_init_menu(c)
	Calendar *c;
{

	char    *tmp, *name;
	char	*namelist;
        Menu            menu;
        Menu_item       menu_item;
        extern void change_it();
	int num_mi, ncols; 
	Props *p = (Props*)c->properties;

	tmp = cm_get_property(property_names[CP_DAYCALLIST]);
	namelist =  (char*)ckalloc(cm_strlen(tmp)+1);
	cm_strcpy(namelist, tmp);
	menu = menu_create(
                MENU_CLIENT_DATA, c,
                MENU_NCOLS, 1,
        0);
	xv_set(c->items->button4, PANEL_ITEM_MENU, menu, NULL);

	menu_set(menu, MENU_APPEND_ITEM, 
		menu_create_item(MENU_ACTION_ITEM, 
			LGET("Show Multiple Calendars..."),
					 show_browser,
			XV_HELP_DATA, "cm:ManyBrowse",
			MENU_CLIENT_DATA, c,
		NULL),
	NULL);
	menu_set(menu, MENU_APPEND_ITEM, 
		menu_create_item(MENU_ACTION_ITEM, 
			LGET("Show Calendar..."), show_tempbr,
			XV_HELP_DATA, "cm:SingleBrowse",
			MENU_CLIENT_DATA, c,
		NULL),
	NULL);
	menu_set(menu, MENU_APPEND_ITEM, 
		menu_create_item(MENU_ACTION_ITEM, 
			LGET("Setup Menu..."), show_blist,
			XV_HELP_DATA, "cm:BrowseList",
			MENU_CLIENT_DATA, c,
		NULL),
	NULL);
	menu_set(menu, MENU_APPEND_ITEM, 
		menu_create_item( MENU_STRING, cm_strdup(""), 
			MENU_FEEDBACK, FALSE,
		NULL),
	NULL);
	menu_item = menu_create_item(MENU_STRING, 
			cm_strdup(c->calname),
		MENU_ACTION_PROC, change_it, 
		XV_HELP_DATA, "cm:BrowseCalendar",
		NULL);
	menu_set(menu, 
		MENU_APPEND_ITEM, menu_item, 
		MENU_DEFAULT, 5,
	NULL);
	if (namelist != NULL && *namelist != NULL) {
		name = (char *)strtok(namelist, " ");
		while (name != NULL && *name != NULL) {
			if (strcmp(c->calname, name) == 0) {
				name = (char *) strtok((char *)NULL, " ");
				continue;
			}
			menu_item = menu_create_item( MENU_STRING,
				cm_strdup(name),
				MENU_ACTION_PROC, change_it, 
				MENU_RELEASE,
				XV_HELP_DATA, "cm:BrowseCalendar",
				NULL);
			menu_set(menu, 
				MENU_APPEND_ITEM, 
				menu_item, 
			NULL);
			name = (char *) strtok((char *)NULL, " ");
		}
	}
	/* they may be different so add to list */ 
	if (strcmp(p->defcal_VAL, c->calname) != 0) {
		menu_item = menu_find(menu, MENU_STRING, p->defcal_VAL, 0);
		if (menu_item == NULL) {
			menu_item = menu_create_item( MENU_STRING,
				cm_strdup(p->defcal_VAL),
				MENU_ACTION_PROC, change_it, 
				MENU_RELEASE,
				XV_HELP_DATA, "cm:BrowseCalendar",
				NULL);
			menu_set(menu, 
				MENU_INSERT, 5, menu_item, 
			0);
		}
	}
	num_mi = (int)xv_get(menu, MENU_NITEMS);
	ncols = (num_mi / NUM_MITEMS_PERCOL) + 1;
	xv_set(menu, MENU_NCOLS, ncols, NULL);
	free (namelist);
}

extern void
make_browse_menu(c)
	Calendar *c;
{
	Menu browse_menu;

	if (c->items->button4 != 0) {
		browse_menu = (Menu)xv_get(c->items->button4, 
				PANEL_ITEM_MENU);
		if (browse_menu != NULL)	
			xv_destroy(browse_menu);
	}
	mb_init_menu(c);
}

static Notify_value
browse_notify_proc(item, event)
        Panel_item item; 
	Event *event;
{
	Calendar  *c;

	c = (Calendar*)xv_get(item, PANEL_CLIENT_DATA);
	if (cal_update_props())
		make_browse_menu(c);
}

static void
make_panel(panel, i)
	Panel panel; Items *i;
{
	Calendar *c;
	int gap=3, longest;

	c = (Calendar *) xv_get(panel, WIN_CLIENT_DATA);

	/* fix .h to reflect button reordering! */

	i->button2 = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,  LGET(" View") ,  
		PANEL_LABEL_BOLD, TRUE,
		XV_X,xv_col(panel,1),
		XV_Y,xv_row(panel,0),
		PANEL_ITEM_MENU, make_view_menu(c),
		XV_HELP_DATA, "cm:ViewButton",
		0);
	i->button3 = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,  LGET(" Edit") ,  
		XV_X,xv_get(i->button2, XV_X) +
			 xv_get(i->button2, XV_WIDTH) + gap,
		XV_Y,xv_row(panel, 0),
		PANEL_LABEL_BOLD, TRUE,
		PANEL_ITEM_MENU, make_edit_menu(c),  
		XV_HELP_DATA, "cm:EditButton",
		0);
	i->button4 = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,  LGET(" Browse") ,
		XV_X,xv_get(i->button3, XV_X) +
			 xv_get(i->button3, XV_WIDTH) + gap, 
		XV_Y,xv_row(panel,0),
		XV_SHOW, TRUE,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_NOTIFY_PROC, browse_notify_proc,
		PANEL_CLIENT_DATA, c,
		XV_HELP_DATA, "cm:BrowseButton",
		0);
	make_browse_menu(c);
	i->button5 = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,  LGET(" Print") ,
		XV_X,xv_get(i->button4, XV_X) +
			 xv_get(i->button4, XV_WIDTH) + gap, 
		XV_Y,xv_row(panel,0),
		XV_SHOW, TRUE,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_ITEM_MENU, make_print_menu(c),
		XV_HELP_DATA, "cm:PrintButton",
		0);

	/* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL :
	 *
	 * Prev = Previous
	 * If the calendar is displaying the day view, then Prev means
	 * the previous day (yesterday).  If the displayed view is the
	 * week view, then Prev means last week.  Likewise Prev means
	 * last month or last year in the month and year view respectively.
	 */
	i->button8 = panel_create_item(panel, PANEL_BUTTON,
		PANEL_NOTIFY_PROC, prev_button,
		PANEL_LABEL_STRING,  LGET("Prev") ,
		XV_X, xv_get(i->button5, XV_X) + 
				xv_get(i->button5, XV_WIDTH) + 40,
		XV_Y, xv_row(panel, 0),
		PANEL_SHOW_ITEM, TRUE,
		XV_HELP_DATA, "cm:PrevButton",
		0);
	i->button9 = panel_create_item(panel, PANEL_BUTTON,
		PANEL_NOTIFY_PROC, today_button,
		PANEL_LABEL_STRING,  LGET("Today") ,
		XV_X, xv_get(i->button8, XV_X) + 
				xv_get(i->button8, XV_WIDTH) + 3,
		XV_Y, xv_row(panel, 0),
		PANEL_SHOW_ITEM, TRUE,
		XV_HELP_DATA, "cm:TodayButton",
		0);
	/* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL :
	 *
	 * Next means next day (tomorrow) if calendar is displaying the day
	 * view.  It means next week, next month or next year in the
	 * week, month and year view respectively.
	 */
	i->button10 = panel_create_item(panel, PANEL_BUTTON,
		PANEL_NOTIFY_PROC, next_button,
		XV_X, xv_get(i->button9, XV_X) + 
				xv_get(i->button9, XV_WIDTH) + 3,
		XV_Y, xv_row(panel, 0),
		PANEL_LABEL_STRING,  LGET("Next") ,
		PANEL_SHOW_ITEM, TRUE,
		XV_HELP_DATA, "cm:NextButton",
		0);

	/* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL
	 *
	 * NOTE:  The button labels "Last Year", "Last Month", "Last Week" and
	 *        "Yesterday" can be translated to be the same as the translation
	 *        for "Prev".  In other words, if your local custom does not
	 *        require translation of "Last Year" or "Last XXX",
	 *        then just translate all these to have the same meaning.
	 *        For example, in the U.S., no matter
	 *        which calendar view the user is in, the "Last XXX" button will
	 *        always be labeled "Prev".  
	 *
	 *        The same goes for the "Next XXX" and "Tomorrow" labels.  These
	 *        can be translated to be the same as "Next".  For example, in
	 *        the U.S., no matter which calendar view the user is in, the
	 *        "Next XXX" button will always be labeled "Next".
	 *
	 *        The button labels "This Year", "This Month", "This Week", and
	 *        "Today" can be translated to have the same meaning as "Today".
	 *        If local convention permits, then all the "This XXX" labels can 
	 *        have the same meaning as "Today".
	 *
	 *        However, if the labels "Prev", "Next", "Today" are ambiguous,
	 *        then separate translation is needed for the "Last XXX",
	 *        "Next XXX" labels.
	 *
	 */
	if ( strcmp("C", (char *)xv_get(calendar->frame, XV_LC_DISPLAY_LANG)) ) {
                longest = get_longest_str(c);

                xv_set(i->button8, PANEL_LABEL_WIDTH, longest, NULL);
                xv_set(i->button9, PANEL_LABEL_WIDTH, longest, NULL);
                xv_set(i->button10, PANEL_LABEL_WIDTH, longest, NULL);

		/* Not in C locale */
		switch (calendar->view->glance) {
			case yearGlance:
				xv_set(i->button8, PANEL_LABEL_STRING, LGET("Last Year"), NULL);
				xv_set(i->button9, PANEL_LABEL_STRING, LGET("This Year"), NULL);
				xv_set(i->button10, PANEL_LABEL_STRING, LGET("Next Year"), NULL);
				break;
			case monthGlance:
				xv_set(i->button8, PANEL_LABEL_STRING, LGET("Last Month"), NULL);
				xv_set(i->button9, PANEL_LABEL_STRING, LGET("This Month"), NULL);
				xv_set(i->button10, PANEL_LABEL_STRING, LGET("Next Month"), NULL);
				break;
			case weekGlance:
				xv_set(i->button8, PANEL_LABEL_STRING, LGET("Last Week"), NULL);
				xv_set(i->button9, PANEL_LABEL_STRING, LGET("This Week"), NULL);
				xv_set(i->button10, PANEL_LABEL_STRING, LGET("Next Week"), NULL);
				break;
			case dayGlance:
				xv_set(i->button8, PANEL_LABEL_STRING, LGET("Yesterday"), NULL);
				xv_set(i->button9, PANEL_LABEL_STRING, LGET("Today"), NULL);
				xv_set(i->button10, PANEL_LABEL_STRING, LGET("Tomorrow"), NULL);
				break;
		}
	} 
#ifdef KYBDACC
        /* bind menus to frame for the accelerators */
        xv_set(calendar->frame, FRAME_MENU_ADD, xv_get(i->button2, PANEL_ITEM_MENU), NULL);
        xv_set(calendar->frame, FRAME_MENU_ADD, xv_get(i->button3, PANEL_ITEM_MENU), NULL);
        xv_set(calendar->frame, FRAME_MENU_ADD, xv_get(i->button5, PANEL_ITEM_MENU), NULL);
#endif

}

static int
get_longest_str(c)
        Calendar *c;
{
        int longest = 0;
        char *t; 
        Font_string_dims dims;
        Xv_Font pf;

        pf = xv_get(c->frame, XV_FONT);
        t = LGET("Last Week");
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        if (dims.width > longest)
                longest = dims.width;
        t = LGET("This Week");
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        if (dims.width > longest)
                longest = dims.width;
        t = LGET("Next Week");
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        if (dims.width > longest)
                longest = dims.width;
        t = LGET("Last Month");
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        if (dims.width > longest)
                longest = dims.width;
        t = LGET("This Month");
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        if (dims.width > longest)
                longest = dims.width;
        t = LGET("Next Month");
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        if (dims.width > longest)
                longest = dims.width;
        t = LGET("Last Year");
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        if (dims.width > longest)
                longest = dims.width;
        t = LGET("This Year");
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        if (dims.width > longest)
                longest = dims.width;
        t = LGET("Next Year");
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        if (dims.width > longest)
                longest = dims.width;
        t = LGET("Yesterday");
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        if (dims.width > longest)
                longest = dims.width;
        t = LGET("Today");
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        if (dims.width > longest)
                longest = dims.width;
        t = LGET("Tomorrow");
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        if (dims.width > longest)
                longest = dims.width;
        return(longest);
}

  
static void
open_all_fonts(c)
	Calendar *c;
{
	/*	It's really 8 pt. instead of 9, but
		what's a couple of pixels between
		friends?				*/
	c->fonts->lucida9	= 
	(Xv_Font) xv_find(c->frame, FONT,
#ifdef OW_I18N
		FONT_FAMILY,	FONT_FAMILY_SANS_SERIF,
		FONT_SCALE,	WIN_SCALE_SMALL,
#else
		FONT_FAMILY, 	FONT_FAMILY_LUCIDA,
		FONT_SIZE,	8,
#endif
		0);

	c->fonts->lucida9b	= 
	(Xv_Font) xv_find(c->frame, FONT,
#ifdef OW_I18N
		FONT_FAMILY,	FONT_FAMILY_SANS_SERIF,
		FONT_SCALE,	WIN_SCALE_SMALL,
#else
		FONT_FAMILY, 	FONT_FAMILY_LUCIDA,
		FONT_SIZE,	8,
#endif
		FONT_STYLE,	FONT_STYLE_BOLD,
		0);

	c->fonts->lucida10	=
	(Xv_Font) xv_find(c->frame, FONT,
#ifdef OW_I18N
		FONT_FAMILY,	FONT_FAMILY_SANS_SERIF,
		FONT_SCALE,	WIN_SCALE_SMALL,
#else
		FONT_FAMILY, 	FONT_FAMILY_LUCIDA,
		FONT_SIZE,	10,
#endif
		0);

	c->fonts->lucida10b	= 
	(Xv_Font) xv_find(c->frame, FONT,
#ifdef OW_I18N
		FONT_FAMILY,	FONT_FAMILY_SANS_SERIF,
		FONT_SCALE,	WIN_SCALE_SMALL,
#else
		FONT_FAMILY, 	FONT_FAMILY_LUCIDA,
		FONT_SIZE,	10,
#endif
		FONT_STYLE,	FONT_STYLE_BOLD,
		0);

	c->fonts->fixed12	= 
	(Xv_Font) xv_find(c->frame, FONT,
#ifdef OW_I18N
		FONT_FAMILY, 	FONT_FAMILY_SANS_SERIF,
		FONT_SCALE,	WIN_SCALE_MEDIUM,
#else
		FONT_FAMILY, 	FONT_FAMILY_DEFAULT_FIXEDWIDTH,
		FONT_SIZE,	12,
#endif
		0);

	c->fonts->lucida12	= 
	(Xv_Font) xv_find(c->frame, FONT,
#ifdef OW_I18N
		FONT_FAMILY,	FONT_FAMILY_SANS_SERIF,
		FONT_SCALE,	WIN_SCALE_MEDIUM,
#else
		FONT_FAMILY, 	FONT_FAMILY_LUCIDA,
		FONT_SIZE,	12,
#endif
		0);

	c->fonts->fixed12b	=
	(Xv_Font) xv_find(c->frame, FONT,
#ifdef OW_I18N
		FONT_FAMILY, 	FONT_FAMILY_SANS_SERIF,
		FONT_SCALE,	WIN_SCALE_MEDIUM,
#else
		FONT_FAMILY, 	FONT_FAMILY_DEFAULT_FIXEDWIDTH,
		FONT_SIZE,	12,
#endif
		FONT_STYLE,	FONT_STYLE_BOLD,
		0);

	c->fonts->lucida12b	= 
	(Xv_Font) xv_find(c->frame, FONT,
#ifdef OW_I18N
		FONT_FAMILY,	FONT_FAMILY_SANS_SERIF,
		FONT_SCALE,	WIN_SCALE_MEDIUM,
#else
		FONT_FAMILY, 	FONT_FAMILY_LUCIDA,
		FONT_SIZE,	12,
#endif
		FONT_STYLE,	FONT_STYLE_BOLD,
		0);

	c->fonts->lucida14b	= 
	(Xv_Font) xv_find(c->frame, FONT,
#ifdef OW_I18N
		FONT_FAMILY,	FONT_FAMILY_SANS_SERIF,
		FONT_SCALE,	WIN_SCALE_LARGE,
#else
		FONT_FAMILY, 	FONT_FAMILY_LUCIDA,
		FONT_SIZE,	14,
#endif
		FONT_STYLE,	FONT_STYLE_BOLD,
		0);
}	



/*	Library candidate	*/
static Reminder * 
get_next_reminder(tick, stat)
	Tick tick;
	Stat *stat;
{
	Reminder	*reminder=NULL;

	*stat = table_lookup_next_reminder(calendar->calname, tick, 
				&reminder);

	return(Reminder*)reminder;
}

static char *daystring[31] = {"1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20","21","22","23","24","25","26","27","28","29","30","31"};

/* ARGSUSED */
extern Notify_value
paint_icon2(client, arg)
        Notify_client client; int arg;
{
        /* the icon repaints at midnight + 30 secs. */
 
        int j, mid, start;
        int w, h, pfy, pfx, x, y, ndays;
        Tick n;
        char buf[25];
        Xv_Font pf;
        XContext *xc;
        Server_image si;
        Icon i;
        Calendar *c = calendar;
        int gap = 0;   /* pixels between each character */
 
	static void reset_icon();
 
        i       = c->icon;
        w       = (int) icon_get(i, ICON_WIDTH);
        h       = (int) icon_get(i, ICON_HEIGHT);
        pf      = (Xv_Font) icon_get(i, ICON_FONT);
        si      = (Server_image ) icon_get(i, ICON_IMAGE);
        pfy     = xv_get(pf, FONT_DEFAULT_CHAR_HEIGHT);
	pfx = xv_get(pf, FONT_COLUMN_WIDTH);
        x       = pfx;
        y       = pfy;
        n       = now();
        ndays   = monthlength(n);
	gap		= 2 * pfx;

	xc = gr_create_xcontext(c->icon, si, gr_mono);

        gr_clear_area(xc, 0, 0, w, h);
 
        /*       Month           */
        (void) sprintf(buf, "%s %d", months[month(n)], year(n));
        mid = gr_center(w, buf, pf);
        (void)gr_text(xc, mid, y+2, pf, buf, NULL);
        y+=pfy;
 
        /*       Day Names              */
		x += pfx;
    	for ( j = 0;  j < 7;  j++ ) {
        	gr_text(xc, x+(j*(pfx+gap)), y, pf, days3[j], NULL);
    	}
    	y+=pfy;
 
        /*       Days                   */
	start = fdom(n);
	for ( j = 0;  j < ndays;  j++ ) {
        	if ( start > 6 ) {
            		start = 0;
            		y += pfy;
        	}
        	if ( cm_strlen(daystring[j]) > 1 ) {
            		x = (pfx) + (start * 3) * pfx;
        	} else {
            		x = (pfx) + (start * 3 + 1) * pfx;
        	}
        	gr_text(xc, x, y, pf, daystring[j], NULL);
        	start++;
    	}
 
        /*      Draw Border     */
        gr_draw_box(xc, 0, 0, w, h, NULL);
        gr_draw_box(xc, 2, 2, w-4, h-4, NULL);
 
        x = dow(n)*(3*pfx)+(3*pfx)/4; 
        y = pfy + wom(n)*pfy + pfy/4;
	if ( strcmp("C", (char *)xv_get(calendar->frame, XV_LC_DISPLAY_LANG)) ) {
                x -= 2;
                y -= 2;
	} 
 
        /*      Highlight Today  */
        gr_draw_box(xc, x, y, 3*pfx, pfy, NULL);
 
        (void)icon_set(i, ICON_IMAGE, si, 0);
        (void)window_set(c->frame, FRAME_ICON, i, 0);

	gr_free_xcontext(xc);
 
	reset_icon();
}
/* ARGSUSED */
extern Notify_value
paint_icon3(client, arg)
	Notify_client client; int arg;
{
	char buf[25];
	int n, h;
	Calendar *c = calendar;
	Xv_Font pf;
	XContext *xc;
	Server_image si;

	static void reset_icon();

	n	= now();
	h	= (int) icon_get(c->icon, ICON_HEIGHT);
	si	= (Server_image) icon_get(c->icon, ICON_IMAGE);

	pf = (Xv_Font)xv_find(c->frame, FONT,
#ifdef OW_I18N
		FONT_FAMILY, FONT_FAMILY_SANS_SERIF,
		FONT_SCALE, WIN_SCALE_LARGE,
#else
		FONT_FAMILY,	"helvetica",
		FONT_SIZE,	18,
#endif
		FONT_STYLE,	FONT_STYLE_BOLD,  
		0);

	xc = gr_create_xcontext(c->icon, si, gr_mono);

	(void) xv_set(icon_image, SERVER_IMAGE_BITS, cm_image, 0);
	(void) xv_set(c->icon, ICON_IMAGE, icon_image, 0);

	/* day of the month */
	(void) sprintf(buf, "%2d", dom(n));
	gr_text(xc, 20, h/2, pf, buf, NULL);

	/* set icon label to month string */
	(void) sprintf(buf, "%s", months[month(n)]);
	(void) xv_set(c->icon, XV_LABEL, buf, 0);

	gr_free_xcontext(xc);

	reset_icon();
	if (!xv_get(c->frame, FRAME_CLOSED))
		paint_canvas(c, NULL);
	return(NOTIFY_DONE);
}
static void
set_icon_font(iconfont)
char *iconfont;
{
	Xv_Font         font;
	Icon 		ficon;

	if (iconfont == NULL)
                if (defaults_exists("icon.font.name", "Icon.Font.Name"))
                        iconfont = strdup((char *)defaults_get_string(
                                "icon.font.name", "Icon.Font.Name", ""));
        if (iconfont != NULL) {
                font = (Xv_Font) xv_find(calendar->frame, FONT,
                       FONT_NAME, iconfont,
                       0) ;
                if (font != 0)
			ficon = xv_get(calendar->frame, FRAME_ICON);
                        xv_set(ficon, ICON_FONT, font, 0) ;
        }

}
	
static Icon
init_icon(which)
	int which;
{
	int h, w;
	Icon i;
	Xv_Font pf;

	switch(which){
	case 2:
		pf =
        		xv_find(calendar->frame, FONT,
#ifdef OW_I18N
                	FONT_FAMILY,    FONT_FAMILY_SANS_SERIF,
                	FONT_SCALE,     WIN_SCALE_SMALL ,
#else
                	FONT_FAMILY,    FONT_FAMILY_DEFAULT_FIXEDWIDTH,
                	FONT_SIZE,      10,
#endif
                	0);

        	h = 8*(int)xv_get(pf, FONT_DEFAULT_CHAR_HEIGHT) + 6;
    		w = 22*(int)xv_get(pf, FONT_COLUMN_WIDTH) + 6;

        	icon_image = xv_create(XV_NULL, SERVER_IMAGE,
                                XV_WIDTH,       w,
                                XV_HEIGHT,      h,
                                SERVER_IMAGE_DEPTH, 1,
                                0);
 
        	i = xv_create(XV_NULL,  ICON,
                        XV_WIDTH, w,
                        XV_HEIGHT, h,
                        ICON_IMAGE, icon_image,
                        XV_FONT, pf,
                        0);
	break;

	default:
		icon_image = xv_create(XV_NULL, SERVER_IMAGE,
			SERVER_IMAGE_BITS, cm_image,
			SERVER_IMAGE_DEPTH, 1,
			XV_WIDTH, 64,
			XV_HEIGHT, 64,
		0);
		icon_mask = xv_create(XV_NULL, SERVER_IMAGE,
			SERVER_IMAGE_BITS, cm_mask,
			SERVER_IMAGE_DEPTH, 1,
			XV_WIDTH, 64,
			XV_HEIGHT, 64,
			0);
		i = xv_create(XV_NULL,	ICON,
			XV_WIDTH, 64,
			XV_HEIGHT, 64,
			ICON_IMAGE, icon_image, 
			ICON_MASK_IMAGE, icon_mask,
			ICON_TRANSPARENT, TRUE,
			WIN_RETAINED, TRUE,
			0);
	break;
	}
	
	return(i);
}
/* We are doing this for the case where the system time changes
	(as in suspend/resume for gypsy, or other cases). reminders 
	may become obsolete and cm gets confused so we flush reminder 
	queue if system time has changed */

static void
reset_reminder_timer()
{

        if (calendar->canvas==NULL) return;

        system_time = time(0);
        rtimer.it_value.tv_usec = 0;
        rtimer.it_value.tv_sec = FIVE_MINUTES;
        rtimer.it_interval.tv_usec = 0;
        rtimer.it_interval.tv_sec = 0;
        (void) notify_set_itimer_func(calendar->canvas,
                reset_reminders, ITIMER_REAL, &rtimer, ITIMER_NULL);
}

extern Stat 
reset_timer()
{
	Tick n;
	Reminder *r=NULL, *lastr=NULL;
	Calendar *c=NULL;
	Stat 	stat;

	c = calendar;
	if (c->frame==NULL) return;
	n = now();
	lastr = (Reminder *)c->view->next_alarm;
	if (lastr != NULL) {
		r = get_next_reminder(lastr->tick, &stat);
		destroy_reminder(lastr);
		c->view->next_alarm = NULL;
	}
	else
		r = get_next_reminder(n, &stat);

	if (r != NULL) {
		c->view->next_alarm = (caddr_t)r;
		timer.it_value.tv_usec = 0;
		timer.it_value.tv_sec = max(r->tick-n, 1);
		timer.it_interval.tv_usec = 0;
		timer.it_interval.tv_sec = 0;
		(void) notify_set_itimer_func(c->frame, reminder_driver,
			ITIMER_REAL, &timer, ITIMER_NULL);
	}
	return stat;
}

static Notify_value
reset_reminders(client, arg)
        Notify_client client; int arg;
{
        int time_it_should_be, time_now = time(0);

        /* We add 30 secs on both sides of the time it should be
        to allow for processing time. If the time_it_should_be
        is way off from what it is now, that means the system time
        was reset so we reset the reminders */
        time_it_should_be = system_time + FIVE_MINUTES;
        if ( time_it_should_be > (time_now+30) ||
                time_it_should_be < (time_now-30) ) {
                if (calendar->view->next_alarm != NULL) {
                        destroy_reminder(calendar->view->next_alarm);
                        calendar->view->next_alarm = NULL;
                }
                reset_timer();
        }
        reset_reminder_timer();
}

static void
reset_icon()
{
	Tick midnight;
	Calendar *c=NULL;

	c = calendar;
	if (c->panel==NULL) return;
	midnight = next_ndays(now(), 1)+30; 

	timer.it_value.tv_usec = 0;
	timer.it_value.tv_sec = max(midnight-now(), 1); 
	timer.it_interval.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	(void) notify_set_itimer_func(c->panel,
		(cmicon==2)?paint_icon2:paint_icon3,
		ITIMER_REAL, &timer, ITIMER_NULL);
}
	

/* ARGSUSED */
static Notify_value
wait3_handler(frame, pid, status, rusage)
        Notify_client frame;
        int pid;
        int status;
        struct rusage *rusage;
{
	destroy_agent();
        exit(1);
	return NOTIFY_DONE;
}

/* ARGSUSED */
static Notify_value
quit_handler(frame, status)
	Notify_client frame;
	Destroy_status status;
{

	/* The calendar is exiting.  Go thru the registration
	  (ticket) list, retrieve the tickets from 
	   the daemons so that they will not waste time
	   with rcp callback errors, unregister the ticket service
	   at the portmapper, kill the ticket service, exit. */

	Browser *b = (Browser*)calendar->browser;
	register int i;

        if (status==DESTROY_SAVE_YOURSELF)      /* for save workspace */
                return(notify_next_destroy_func(frame, status));

	if (browser_showing(b))
		list_items_selected(b->box, mb_deregister_names);

	(void) table_request_deregistration(calendar->view->current_calendar);
	destroy_agent();
	if (child != 0) {
		(void) kill(child, SIGKILL);
	}
	exit(0);
}

/* ARGSUSED */
static Notify_value
pipe_handler(me,fd)
	Notify_client me;
	int fd;
{

	/* OpenWindows was aborted, causing a SIGPIPE signal */
        /* Quit as though the user had quit Calendar Manager */

	Browser *b = (Browser*)calendar->browser;
	register int i;

	if (browser_showing(b))
		list_items_selected(b->box, mb_deregister_names);

	(void) table_request_deregistration(calendar->view->current_calendar);
	destroy_agent();
	if (child != 0) {
		(void) kill(child, SIGKILL);
	}
	exit(0);
}


static Notify_value
frame_notify(client, event, arg, when)
	Notify_client     client;
	Event		  *event;
	Notify_arg        arg;
	Notify_event_type when;
{
	int gap = 3;

	/* only do this the very first time to ensure the buttons 
		are visible */
	if (event_id (event) == WIN_REPARENT_NOTIFY) {
		 /*
                 *      Reposition the past/present/future button
                 */
 
                int offset  = (int) xv_get(calendar->frame, XV_WIDTH);
                int margin = 10;
 
                /* assumes this gets called before cache_dims! */
                int cw = (int) xv_get(calendar->canvas, XV_WIDTH);
                int moving_left = (cw < calendar->view->winw);
 
                /* get width of rect which we do not want to overwrite */
                Rect *safe_rect = (Rect *)
                        xv_get(calendar->items->button5, PANEL_ITEM_RECT);
                short safe_wid = (short) safe_rect->r_width;
		/* get x offset of non-overwritable rect + width + margin */
		int safe_offset = safe_wid + (int)
                        xv_get(calendar->items->button5, XV_X)+gap;
 
                /* get rects for the previous, present & future buttons */
                Rect *b_rect = (Rect *)
                        xv_get(calendar->items->button8, PANEL_ITEM_RECT);
                Rect *s_rect = (Rect *)
                        xv_get(calendar->items->button9, PANEL_ITEM_RECT);
                Rect *f_rect = (Rect *)
                        xv_get(calendar->items->button10, PANEL_ITEM_RECT);
 
                /* get widths of previous, present & future buttons */
                short b_wid = (short) b_rect->r_width;
                short s_wid = (short) s_rect->r_width;
                short f_wid = (short) f_rect->r_width;
 
                offset -= (b_wid+s_wid+f_wid+margin+2*gap);
 
                if (offset < safe_offset) offset = safe_offset;
 
                /* move the past/present/future button with right edge */
                if (moving_left) {
                        (void)xv_set(calendar->items->button8,
                                        XV_X, offset, NULL);
                        (void)xv_set(calendar->items->button9,
                                        XV_X, offset+b_wid+gap, NULL);
                        (void)xv_set(calendar->items->button10,
                                        XV_X, offset+b_wid+s_wid+2*gap, NULL);
		}
                else {
                        (void)xv_set(calendar->items->button10,
                                        XV_X, offset+b_wid+s_wid+2*gap, NULL);
                        (void)xv_set(calendar->items->button9,
                                        XV_X, offset+b_wid+gap, NULL);
                        (void)xv_set(calendar->items->button8,
                                        XV_X, offset, NULL);
                }
		if (xv_get(calendar->frame, XV_WIDTH) <
			(xv_get(calendar->items->button10, XV_X) +
			xv_get(calendar->items->button10, XV_WIDTH))) 
			xv_set(calendar->frame, XV_WIDTH, 
				xv_get(calendar->items->button10, XV_X) + 
				xv_get(calendar->items->button10, XV_WIDTH) + 40,
				NULL);
		else
			window_fit(calendar->frame);
	}
	return (notify_next_event_func(client, (Notify_event)event, arg, when));
}


#define BUFSIZE 255
static int myabort(dpy, event)
        Display *dpy;
        XErrorEvent  *event;
{
char buffer[BUFSIZE];
  char mesg[BUFSIZE];
  char number[32];
  char *mtype = "XlibMessage";
  FILE *fp = stdout;

  XGetErrorText( dpy, event->error_code, buffer, BUFSIZE );
  XGetErrorDatabaseText( dpy, mtype, "XError", "X Error (intercepted)",
                         mesg, BUFSIZE );
  ( void )fprintf( fp, "%s:  %s\n  ", mesg, buffer );
  XGetErrorDatabaseText( dpy, mtype, "MajorCode", "Request Major code %d",
                         mesg, BUFSIZE );
  ( void )fprintf( fp, mesg, event->request_code );
  sprintf( number, "%d", event->request_code );
  XGetErrorDatabaseText( dpy, "XRequest", number, "", buffer, BUFSIZE );
 ( void )fprintf(fp, " (%s)", buffer );
  fputs("\n  ", fp );
  XGetErrorDatabaseText( dpy, mtype, "MinorCode", "Request Minor code",
                         mesg, BUFSIZE );
  ( void )fprintf( fp, mesg, event->minor_code );
  fputs("\n  ", fp );
  XGetErrorDatabaseText( dpy, mtype, "ResourceID", "ResourceID 0x%x",
                         mesg, BUFSIZE );
  ( void )fprintf(fp, mesg, event->resourceid );
  fputs("\n  ", fp );
  XGetErrorDatabaseText( dpy, mtype, "ErrorSerial", "Error Serial #%d",
                         mesg, BUFSIZE );
  ( void )fprintf( fp, mesg, event->serial );
   fputs("\n  ", fp );
  XGetErrorDatabaseText( dpy, mtype, "CurrentSerial", "Current Serial #%d",
                         mesg, BUFSIZE );
  ( void )fprintf( fp, mesg, NextRequest(dpy)-1 );
fputs( "\n", fp );
        abort();
}

static void
error_nocallog(uname)
	char *uname;
{
	char buf[MAXNAMELEN];
	char *name, *name2;

	name = cm_target2name(uname);
	name2 = cm_get_uname();
	/* trying to create users calendar */
	if (strcmp(name, name2) == 0) {
		notice_prompt(calendar->frame, (Event *)NULL,
		NOTICE_MESSAGE_STRINGS,
			MGET("Error in creation of Calendar file for"),
			uname,
		NULL, 
		NOTICE_BUTTON_YES,  LGET("Continue"),
		NULL);
		sprintf(buf, "%s: %s.", EGET("Error in creation of Calendar file "), 
			uname);
		xv_set(calendar->frame, FRAME_LEFT_FOOTER, buf, NULL);
	}
	/* cannot create someone elses calendar */
	else {
		notice_prompt(calendar->frame, (Event *)NULL,
		NOTICE_MESSAGE_STRINGS,
			MGET("Calendar file does not exist for"),
			uname,
		NULL, 
		NOTICE_BUTTON_YES,  LGET("Continue"),
		NULL);
		sprintf(buf, "%s: %s.", EGET("Calendar file does not exist for "), 
			uname);
		xv_set(calendar->frame, FRAME_LEFT_FOOTER, buf, NULL);
	}
	free(name);
	(void)sprintf(buf, "%s: NO NAME", (char*)cm_get_relname());
	(void)xv_set(calendar->frame, XV_LABEL, buf, NULL);

}
static void
error_nodaemon(name)
	char *name;
{
	char buf[MAXNAMELEN];
	char *calloc;

	(void)sprintf(buf, "%s: %s.", EGET("rpc.cmsd not responding for"), 
			name);
	xv_set(calendar->frame, FRAME_LEFT_FOOTER, buf, NULL);
	calloc = cm_target2location(name);
	notice_prompt(calendar->frame, (Event *)NULL,
		NOTICE_MESSAGE_STRINGS,
			MGET("rpc.cmsd is not responding for"),
			name,
			MGET("Make sure the inetd process is running"),
			MGET("and the entry in /etc/inetd.conf for"),
			MGET("rpc.cmsd is correct on host"),
			calloc, 
		NULL, 
		NOTICE_BUTTON_YES,  LGET("Continue"),
	NULL);
	free(calloc);
	(void)sprintf(buf, "%s: NO NAME", (char*)cm_get_relname());
	(void)xv_set(calendar->frame, XV_LABEL, buf, NULL);

}
static void
error_noloc(name)
	char *name;
{
	char buf[MAXNAMELEN];

	(void)sprintf(buf, "%s: %s", 
		EGET("No Calendar Location specified for"), name);
	xv_set(calendar->frame, FRAME_LEFT_FOOTER, buf, NULL);
	notice_prompt(calendar->frame, (Event *)NULL,
		NOTICE_MESSAGE_STRINGS,
			MGET("No Calendar Location Specified For"), name,
			MGET("Add a hostname to the Initial Calendar View"), 
			MGET("in Properties/Display Setting. If you ran CM"),
			MGET("with the -c command line option make sure it"),
			MGET("has a hostname specified."),
		NULL,
		NOTICE_BUTTON_YES, LGET("Continue"),
	NULL);
}
static Boolean
register_client(name)
	char *name;
{
	char buf[MAXNAMELEN];
	char 	*calloc=NULL;
	Register_Status status;
	
	calloc = cm_target2location(name);
	if (calloc == NULL) {
		error_noloc(name);
		calendar->general->version = 0;
		return false;
	}
	if ((calendar->general->version = table_version(calloc)) == 0) {
		error_nodaemon(name);
		free(calloc);
		return false;
	}
	free(calloc);
	status = table_request_registration(name);
	if (status == register_notable) {
		/* try to create file */
		if (table_create(name) == status_ok) {
			status = table_request_registration(name);
			if (status != register_succeeded) { 
				error_nodaemon(name);
				return false; 
			}
		}
		else {
			error_nocallog(name);
			return false; 
		}
	}
	else if (status != register_succeeded) { 
		error_nodaemon(name);
		return false; 
	}
	if (calendar->general->version != CM_VERSION) 
		xv_set(calendar->frame, FRAME_LEFT_FOOTER, 
MGET("CM and rpc.cmsd versions are different."), NULL);
	/* daemon started successfully */
	(void)sprintf(buf, "%s: %s", (char*)cm_get_relname(), name);
	(void)xv_set(calendar->frame, XV_LABEL, buf, 0);
	return true; 
}

static void
init_calendar(argc, argv)
	int argc; char **argv;
{
	register int i;
	char *value_p=NULL;
	Tick today;
	extern	char	*property_names[];
	extern	char	*cm_get_property();
	void	parse_args();
	char bind_home[MAXPATHLEN];
	Boolean registered = false;
	char     **argscanner = argv;
	char 	*iconfont=NULL;


	while (*argscanner) {
                if (strcmp(*argscanner,  "-icon_font" ) == 0 ||
                        strcmp(*argscanner,  "-WT" ) == 0) {
                        iconfont = strdup(*(argscanner+1));
                }
                argscanner++;
        }

	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
		XV_USE_LOCALE, TRUE,
		XV_X_ERROR_PROC, myabort,
		NULL);

	ds_expand_pathname("$OPENWINHOME/lib/locale", bind_home);
	bindtextdomain(MSGFILE_ERROR, bind_home);
	bindtextdomain(MSGFILE_LABEL, bind_home);
	bindtextdomain(MSGFILE_MESSAGE, bind_home);

	init_time();
	today = now();
	calendar = (Calendar*) ckalloc(sizeof(Calendar));
	calendar->fonts = (Fonts*) ckalloc(sizeof(Fonts));
	calendar->view = (View*) ckalloc(sizeof(View));
	calendar->general = (General*) ckalloc(sizeof(General));
	calendar->view->nwks = numwks(today);

	parse_args(argc, argv);
	
	calendar->frame = (Frame) xv_create((Xv_Window)NULL, FRAME_BASE,
		WIN_IS_CLIENT_PANE,
		XV_HEIGHT, 440,
		XV_WIDTH, 485,
		WIN_USE_IM, FALSE,
		WIN_ROW_GAP, 9,
		WIN_BIT_GRAVITY, ForgetGravity,
		FRAME_SHOW_LABEL, TRUE,
		WIN_CLIENT_DATA, calendar,
		FRAME_CLOSED, TRUE,
		FRAME_SHOW_FOOTER, TRUE,
		FRAME_ARGC_PTR_ARGV, &argc, argv,
		FRAME_MIN_SIZE, 380, 260, 
		FRAME_WM_COMMAND_ARGC_ARGV, (argc-1), &(argv[1]),
		0);

	open_all_fonts(calendar);
	cal_convert_cmrc();
	cal_update_props();

	value_p = cm_get_property(property_names[CP_DEFAULTVIEW]);
	if (value_p)
	{
		switch (atoi(value_p)) {

		case 0: calendar->view->glance = yearGlance;
			break;

		case 1: calendar->view->glance = monthGlance;
			break;

		case 2: calendar->view->glance = weekGlance;
			break;

		case 3: calendar->view->glance = dayGlance;
			break;
		}
	}
	else
		calendar->view->glance = monthGlance;

	calendar->view->date = today;
	calendar->view->outside_margin = VIEWMARGIN;
	calendar->view->current_selection = (caddr_t) ckalloc(sizeof(Selection));
	calendar->user = table_get_credentials();


	/* init_icon after creation of calendar->frame */
	calendar->icon = init_icon(cmicon);
	xv_set(calendar->frame,	FRAME_ICON, calendar->icon, NULL);

	xv_set(calendar->frame, FRAME_LEFT_FOOTER, 
		 MGET("Copyright (c) 1987-1997 Sun Microsystems, Inc.") , 0);

	if ((int) xv_get(calendar->frame, XV_HEIGHT) < 100)
		(void)xv_set(calendar->frame, XV_HEIGHT, 100, 0);

	if ((int) xv_get(calendar->frame, XV_WIDTH) < 100)
		(void)xv_set(calendar->frame, XV_WIDTH, 100, 0);

	calendar->properties = (caddr_t) ckalloc(sizeof(Props));

#if 0 /* use this when we go with mapped names */
	cm_get_yptarget(cm_get_uname(), &calendar->calname);
	if (calendar->calname == NULL) 
		/* No mapping in NIS+ db */
		calendar->calname = cm_get_deftarget();
#endif
	calendar->calname = cm_get_deftarget();

	for (i=0; i < NO_OF_PANES; i++) {
		set_default_vals((Props*)calendar->properties, i);
		set_rc_vals((Props*)calendar->properties, i);
	}

	calendar->items = (Items *) ckalloc(sizeof(Items));
	calendar->panel =
		xv_create(calendar->frame, PANEL,
		WIN_X, 0,
		WIN_Y, 0,
		WIN_ROWS, 1,	
		WIN_WIDTH, WIN_EXTEND_TO_EDGE,
		WIN_CLIENT_DATA, calendar,
		WIN_ROW_GAP, 12,
		OPENWIN_SHOW_BORDERS, FALSE,
		XV_HELP_DATA, "cm:CommandPanel",
		0);

	make_panel(calendar->panel, calendar->items);
	if (value_p)
		set_default_view(calendar, atoi(value_p));
	else
		set_default_view(calendar, 1);

#if LATER
	if (cmtarget != NULL && cmtarget[0] != NULL) 
		map_name(cmtarget, &calendar->view->current_calendar);
        else
                map_name(((Props*)calendar->properties)->defcal_VAL,
                        &calendar->view->current_calendar);
#endif
	if (cmtarget != NULL && cmtarget[0] != NULL) 
		calendar->view->current_calendar = cm_strdup(cmtarget);
	else
		calendar->view->current_calendar = 
			cm_strdup(((Props*)calendar->properties)->defcal_VAL);

	/* setup to reposition past/present/future button when resized */
	(void)notify_interpose_event_func(calendar->frame, frame_notify, NOTIFY_SAFE);

	calendar->canvas = (Canvas) xv_create(calendar->frame, CANVAS,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		CANVAS_FIXED_IMAGE, FALSE,
		CANVAS_RETAINED, FALSE,
		WIN_BIT_GRAVITY, ForgetGravity,
		CANVAS_NO_CLIPPING, TRUE,
		CANVAS_X_PAINT_WINDOW, TRUE,
		CANVAS_REPAINT_PROC, repaint_proc,  
		CANVAS_RESIZE_PROC, resize_proc,     
		OPENWIN_AUTO_CLEAR, TRUE,
		XV_X, 0,
		WIN_BELOW, calendar->panel,
		WIN_CONSUME_PICK_EVENT, LOC_DRAG, 
		WIN_CONSUME_KBD_EVENTS, WIN_TOP_KEYS, WIN_UP_EVENTS, 0,
		WIN_CLIENT_DATA, calendar, 
		0);

	init_strings();   /* strings in timeops.c */

	calendar->xcontext = gr_create_xcontext(calendar->canvas,
 			canvas_paint_window(calendar->canvas), gr_color);

	(void)gr_init(calendar->xcontext);

        xv_set(calendar->xcontext->drawable,
		WIN_CONSUME_EVENT, LOC_DRAG,
		WIN_CLIENT_DATA, calendar,
		0);
        (void)notify_interpose_event_func(calendar->xcontext->drawable,
                event_proc, NOTIFY_SAFE);
        (void)notify_interpose_event_func(calendar->canvas,
                event_proc, NOTIFY_SAFE);

	/*	Make property sheet before editor because editor
		takes values from the property sheet.
	*/

	calendar->view->week_info = (caddr_t) ckalloc(sizeof(Week));
	calendar->view->day_info = (caddr_t) ckalloc(sizeof(Day));
	init_mo(calendar);
	init_dayview(calendar);

	(cmicon==2) ? (paint_icon2(calendar->frame,(int)NULL)):
		(paint_icon3(calendar->frame,(int)NULL)); 

	set_icon_font(iconfont);

	calendar->postup = (caddr_t) make_postup();

	(void) notify_set_signal_func(calendar->frame, update_handler,
		SIGUSR1, NOTIFY_ASYNC);
	(void) notify_set_signal_func(calendar->frame, pipe_handler,
		SIGPIPE, NOTIFY_ASYNC);
	(void) notify_set_wait3_func(calendar->frame, wait3_handler, child);
	(void) notify_set_destroy_func(calendar->frame, quit_handler);
	(void) notify_interpose_destroy_func(calendar->frame, quit_handler);
	(void) notify_set_signal_func(calendar->frame,
		quit_handler, SIGINT, NOTIFY_ASYNC);

	/* set version of cms and register client */
	if (!(registered =
		register_client(calendar->view->current_calendar))) {
		if (strcmp(calendar->view->current_calendar, 
				calendar->calname) != 0) 
			if (registered = 
				register_client(calendar->calname)) {
				free(calendar->view->current_calendar);
				calendar->view->current_calendar = 
					calendar->calname;
			}
	}

	if (registered)
		reset_timer();
	calendar->editor = (caddr_t)ckalloc(sizeof(Editor));
	init_dragdrop();

	/* saved for tooltalk initialization */
	gargc = argc;
	gargv = argv;

	/* This is in case the system time is reset via 'date', etc. */
        reset_reminder_timer();
}

static void
cm_usage()
{
	(void)fprintf(stderr,  EGET("Usage: cm -c calendar -i [2-3] [ generic-tool-arguments ]\n") );
	if (child != 0)
	{
		destroy_agent();
		(void)kill(child, SIGKILL);
	}
	exit(1);
}

static char**
grab(argv,buf,stop_key)
char**argv;                             /* command line arguments */
char *buf;                              /* buffer for keyed data */
char stop_key;
{
        if (!argv || !*argv) return(argv);
        cm_strcpy (buf,*argv++);
        while(argv && *argv) {
                if (*(*argv) == stop_key) break;
                cm_strcat(buf," ");
                cm_strcat(buf,*argv++);
        }
        argv--;
        return(argv);
}

static void
parse_args(argc, argv)
	int argc;
	char **argv;
{
	int i;
	char buf[3];

        while (++argv && *argv) {
                switch(*(*argv+1)) {
                case 't':
                case 'c':
                        argv = grab(++argv,cmtarget,'-');
                        break;
		case 'i':
			if (*(*argv+2) != '\0') {
			        cmicon = atoi(*argv+2);
			} else {
			        argv = grab(++argv,buf, '-');
			        cmicon = atoi(buf);
			}
			if ((cmicon < 2) || (cmicon > 3)) {
			        cm_usage();
			        exit(1);
			}
			break;
                case 'd':
			debug=1;
                        break;
                case 'x':
			expert=1;
                        break;
                default:
			cm_usage();
                        exit(1);
                }
        }
}

main(argc, argv)
        int argc; char **argv;
{

	init_agent();

        /* Let's notifier hand rpc events down to me */
        notify_enable_rpc_svc(1);

        init_calendar(argc, argv);

        xv_main_loop(calendar->frame);

        exit(0);
}
extern Stat
paint_canvas(c, rect)
        Calendar *c;
	Rect *rect;
{
        Glance glance = c->view->glance;
	Stat stat = access_ok;

        switch(glance) {
        case monthGlance:
                calendar_deselect(c);
                stat = paint_monthview(c, rect);
                calendar_select(c, daySelect,(caddr_t)NULL);
                break;
        case yearGlance:
                stat = paint_year(c, rect);
		calendar_select(c, monthSelect, NULL);
                break;
        case weekGlance:
                calendar_deselect(c);
                stat = paint_weekview(c, rect);
                break;
        case dayGlance:
                calendar_deselect(c);
                stat = paint_dayview(c, true, rect);
                calendar_select(c, hourSelect, (caddr_t)NULL);
                break;
        default:
		stat = access_ok;
                paint_grid(c, rect);
                break;
        }
	return stat;
}
extern char*
cm_get_relname()
{
        char *s;
        char *relname;
        static char* CM_name;
        extern char *ds_relname();

        /* we now make a composite name for the tool, combining
         * "Calendar Manager" with a release identifier
         */
	if (CM_name == NULL) {
        	relname = ds_relname();
        	s = MGET("Calendar Manager");
        	CM_name = ckalloc(cm_strlen(relname) + cm_strlen(s) + 2);
	 
        	sprintf(CM_name, "%s %s", s, relname);
	}
 
        return (char*)CM_name;
}
extern Boolean
in_range(kr, tick)
        Keyrange *kr;
        Tick tick;
{
        if (tick >= kr->tick1 && tick <= kr->tick2)
                return true;
        return false;
}

extern Boolean
today_inrange(c, day_in_range)
	Calendar *c;
	Tick day_in_range;
{
	Keyrange keyrange;
	Boolean inrange = false;

	/* is today in range of current view? */
        get_range(c->view->glance, day_in_range, &keyrange);
        inrange = in_range(&keyrange, time(0));
	return inrange;
}
void
init_strings()
{
	char *display_lang;

	display_lang = (char *)xv_get(calendar->frame, XV_LC_DISPLAY_LANG);

	months[1] = MGET("January");
	months[2] = MGET("February");
	months[3] = MGET("March");
	months[4] = MGET("April");
	months[5] = MGET("May");
	months[6] = MGET("June");
	months[7] = MGET("July");
	months[8] = MGET("August");
	months[9] = MGET("September");
	months[10] = MGET("October");
	months[11] = MGET("November");
	months[12] = MGET("December");

	months2[1] = MGET("Jan");
	months2[2] = MGET("Feb");
	months2[3] = MGET("Mar");
	months2[4] = MGET("Apr");
	months2[5] = MGET("May");
	months2[6] = MGET("Jun");
	months2[7] = MGET("Jul");
	months2[8] = MGET("Aug");
	months2[9] = MGET("Sep");
	months2[10] = MGET("Oct");
	months2[11] = MGET("Nov");
	months2[12] = MGET("Dec");

	days[0] = MGET("Sun");
	days[1] = MGET("Mon");
	days[2] = MGET("Tue");
	days[3] = MGET("Wed");
	days[4] = MGET("Thu");
	days[5] = MGET("Fri");
	days[6] = MGET("Sat");
	days[7] = MGET("Sun");

	days2[0] = MGET("Sunday");
	days2[1] = MGET("Monday");
	days2[2] = MGET("Tuesday");
	days2[3] = MGET("Wednesday");
	days2[4] = MGET("Thursday");
	days2[5] = MGET("Friday");
	days2[6] = MGET("Saturday");
	days2[7] = MGET("Sunday");

	if ( strcmp(display_lang, "C") == 0 ) {
		days3[0] = MGET("S");
		days3[1] = MGET("M");
		days3[2] = MGET("T");
		days3[3] = MGET("W");
		days3[4] = MGET("T");
		days3[5] = MGET("F");
		days3[6] = MGET("S");
		days3[7] = MGET("S");
	} else {
		days3[0] = MGET("S");
		days3[1] = MGET("M");
		days3[2] = MGET("T");
		days3[3] = MGET("W");
		days3[4] = MGET("R");
		days3[5] = MGET("F");
		days3[6] = MGET("Sa");
		days3[7] = MGET("S");
	}

	days4[0] = MGET("SUN");
	days4[1] = MGET("MON");
	days4[2] = MGET("TUE");
	days4[3] = MGET("WED");
	days4[4] = MGET("THU");
	days4[5] = MGET("FRI");
	days4[6] = MGET("SAT");
	days4[7] = MGET("SUN");

	/* strings used in editor for the repeat and for fields */
	init_periodstr();
}
