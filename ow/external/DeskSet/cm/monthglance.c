#ifndef lint
static  char sccsid[] = "@(#)monthglance.c 3.28 94/09/01 Copyr 1991 Sun Microsystems, Inc.";
#endif

/* monthglance.c */

#include <stdio.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/font.h>
#include <xview/cms.h>
#include <ds_listbx.h>
#include "util.h"
#include "graphics.h"
#include "select.h"
#include "timeops.h"
#include "ps_graphics.h"
#include "datefield.h"
#include "props.h"
#include "appt.h"
#include "table.h"
#include "calendar.h"
#include "editor.h"
#include "gettext.h"

extern Abb_Appt *
paint_day_entries(day, x, y, list, rect)
        Tick day; int x, y; Abb_Appt *list; Rect *rect;
{
        char *buf;
        int nlines = 0, maxchars;
        Abb_Appt *a=NULL;
        Calendar *c=calendar;
	Props *p = (Props*)c->properties;
        Lines *lines=NULL;

        XContext *xc    = c->xcontext;
        Xv_Font pf      = c->fonts->lucida10;
        int boxw        = c->view->boxw;
        int boxh        = c->view->boxh;
        int pfy         = xv_get(pf, FONT_DEFAULT_CHAR_HEIGHT);
        int maxlines    = (boxh-5)/pfy;  /* -3 pixel fudge factor */
        Tick lower      = lowerbound(day);
        Tick upper      = next_ndays(day, 1);

        gr_clear_box(xc, x-1, y-pfy, boxw-4, boxh-pfy-3);

        a = list;
        while(a != NULL) {
                if (a->appt_id.tick > lower && a->appt_id.tick < upper) {
                        if (nlines < maxlines-1) {
				lines = text_to_lines(a->what, 1);
                                if (lines != NULL) {
                                        buf = ckalloc(cm_strlen(lines->s) + 18);
                                	format_line(a->appt_id.tick, lines->s, 
						buf, 0, a->tag->showtime,
						p->default_disp_VAL);
				}
                                else {
                                        buf = ckalloc(15);
                                	format_line(a->appt_id.tick, (char*)NULL, 
						buf, 0, a->tag->showtime,
						p->default_disp_VAL);
				}
				destroy_lines(lines); lines=NULL;
                                maxchars = gr_nchars(boxw-5, buf, pf);
				if (cm_strlen(buf) > maxchars)
					buf[maxchars] = NULL;
                                (void)gr_text(xc, x, y, pf, buf, rect);
                                free(buf); buf=NULL;
                                y = y + pfy;
                                nlines++;
                        }
                        a = a->next;
                }
                else break;
        }
        return(a);
}

static Stat
paint_month(c, key, rect)
        Calendar *c; Tick key;
	Rect *rect;
{
        int x, y, i;
        int ndays, firstdom, boxw, boxh, margin, today_dom;
        Tick day;
        char buf[128];
        struct tm tm;
        struct Range range;
        XContext *xc;
        Fonts *fonts=NULL;
        Xv_Font pf=NULL;
        Abb_Appt *list=NULL, *h=NULL;
	Boolean inrange;
	Props *p = (Props*)c->properties;
	Stat stat = access_ok;
 
        tm              = *localtime(&key);
        tm.tm_mday      = 1;
#ifdef SVR4
        tm.tm_isdst      = -1;
        day             = mktime(&tm);
#else
        day             = timelocal(&tm);
#endif "SVR4"
        ndays           = ((tm.tm_mon==1) && leapyr(tm.tm_year+TM_CENTURY))? 29 :
                        	monthdays[tm.tm_mon];
	tm              = *localtime(&day);
        firstdom             = tm.tm_wday;
        boxw            = c->view->boxw;
        boxh            = c->view->boxh;
        margin          = c->view->outside_margin;
        xc              = c->xcontext;
        fonts           = c->fonts;
 
        range.key1 = lowerbound(day);
        range.key2 = (int)nextmonth(day);
	range.next = NULL;

        if ((stat = table_abbrev_lookup_range(c->view->current_calendar, 
			&range, &list)) == status_param && 
			c->general->version <= CMS_VERS_3) 
		return stat;
	h = list;

        pf = (Xv_Font) fonts->lucida12b;
        x = firstdom; y = 0;

	inrange = today_inrange(c, day);
	today_dom = dom(time(0));

        for(i = 1; i <= ndays; i++) {
		if (c->xcontext->screen_depth>=8 && inrange 
			&& today_dom == atoi(numbers[i]))
                	(void) gr_text_rgb(xc, (x * boxw) + margin+3,
                        (y*boxh)+c->view->topoffset+margin+
                        xv_get(pf, FONT_DEFAULT_CHAR_HEIGHT), pf,
				numbers[i], CMS_BACKGROUND_PIXEL, 
				xv_get(c->panel, WIN_CMS), rect);
		else
                	(void) gr_text(xc, (x * boxw) + margin+3,
                        (y*boxh)+c->view->topoffset+margin+
                        xv_get(pf, FONT_DEFAULT_CHAR_HEIGHT), pf,
				numbers[i], rect);
                list = paint_day_entries(day,(x*boxw)+margin+3,
                        (y*boxh)+c->view->topoffset+margin+1+
                                xv_get(pf, FONT_DEFAULT_CHAR_HEIGHT)+
                                xv_get(fonts->lucida9,
                                FONT_DEFAULT_CHAR_HEIGHT), list, rect);
                day = nextday(day);
                x++;
                if (x > 6 & i != ndays) {
                        x = 0;
                        y++;
                }
        }
        destroy_abbrev_appt(h);
        format_date(key, p->ordering_VAL, buf, 0);
        (void)gr_text(xc, margin, c->view->topoffset/2+5, fonts->lucida12b, buf, rect);
	return stat;
}
 
 
extern void
paint_grid(c, rect)
        Calendar *c;
	Rect *rect;
{
        int i;
        int boxh        = c->view->boxh;
        int boxw        = c->view->boxw;
        int nrows       = c->view->nwks;
        int margin      = c->view->outside_margin;
        int rightmargin = margin + 7 * boxw;
        int bottomargin = c->view->topoffset+ margin+boxh*nrows;
        XContext *xc    = c->xcontext;
 
        /* horizontal */
        for (i = 1; i <= nrows; i++) {
                gr_draw_line(xc, margin,
                        (i*boxh)+c->view->topoffset+margin,
                	 rightmargin, i*boxh+c->view->topoffset+margin,
                 	gr_solid, rect);
        }
 
        /* vertical */
        for (i = 0; i < 8; i++) {
                gr_draw_line(xc, margin+(i*boxw),
                        c->view->topoffset+margin,
                	margin+(i*boxw), bottomargin, gr_solid, rect);
        }
        /* embolden grid outline */
        gr_draw_box(xc, margin-1,
                c->view->topoffset-1, 7*boxw+2,
                        nrows*boxh+2+margin, rect);
}
static void
paint_daynames(c, rect)
        Calendar *c;
	Rect	*rect;
{
        int i, middle;
        int boxw        = c->view->boxw;
        int margin      = c->view->outside_margin;
        Xv_Font pf      = c->fonts->lucida12b;
        XContext *xc    = c->xcontext;

        for(i = 0; i < 7; i++) {
                gr_draw_box(xc, (boxw * i)+margin,
                        c->view->topoffset,
                        boxw, margin, rect);
                middle = gr_center(boxw+margin, days[i], pf);
                gr_text(xc,
                        (boxw*i)+middle+3, c->view->topoffset+margin-3,
                        pf, days[i], rect);
        }
}
extern Stat
paint_monthview(c, rect)
        Calendar *c;
	Rect *rect;
{
        int w = (int) xv_get(c->canvas, XV_WIDTH);
        int h = (int) xv_get(c->canvas, XV_HEIGHT);
	Stat stat;
 
        c->view->nwks = numwks(c->view->date);
        cache_dims(c, w, h);
        paint_daynames(c, rect);
        paint_grid(c, rect);
        stat = paint_month(c, c->view->date, rect);
	return stat;
}

/* ARGSUSED */
extern Notify_value
month_button (m, mi)
        Menu m;
        Menu_item mi;
{
        Calendar *c = (Calendar *) xv_get (m, MENU_CLIENT_DATA);
        XContext *xc = c->xcontext;

        if (c->view->glance==monthGlance)
		 return NOTIFY_DONE;
        c->view->glance = monthGlance;

	   if ( strcmp("C", (char *)xv_get(calendar->frame, XV_LC_DISPLAY_LANG)) ) {
		xv_set(calendar->items->button8, PANEL_LABEL_STRING, LGET("Last Month"), NULL);
		xv_set(calendar->items->button9, PANEL_LABEL_STRING, LGET("This Month"), NULL);
		xv_set(calendar->items->button10, PANEL_LABEL_STRING, LGET("Next Month"), NULL);
	   }

        cache_dims (c, xv_get (c->canvas, XV_WIDTH, 0),
                xv_get (c->canvas, XV_HEIGHT, 0));
        gr_clear_area(xc, 0, 0, c->view->winw, c->view->winh);
        paint_monthview (c, NULL);
        calendar_select(c, daySelect, NULL);

	return NOTIFY_DONE;
}

extern void
print_month_pages(c)
	Calendar *c;
{
	
        Props *p = (Props *)c->properties;
        register Tick first_date = c->view->date;
        int n = atoi(p->repeatVAL);
	Boolean done = FALSE, first = TRUE, print_month();
	int num_page = 1;
        FILE *fp;

        if (n <= 0)
                n = 1;

        if ((fp=ps_open_file()) == NULL)
                return;
        for (; n > 0; n--) {

		while (!done) {
			done = print_month(c, num_page, fp, first_date, p, first);
			num_page++;
			first = FALSE;
		}

		done = FALSE;
		num_page = 1;
	}
        if (fp)
                fclose(fp);
        ps_print_file();
}

static Boolean
print_month (c, num_page, fp, first_date, p, first)
        Calendar *c;
	int num_page;
	FILE *fp;
        Tick first_date;
        Props *p;
	Boolean first;
{
        int i, lines_per_box;
        int lo_hour, hi_hour, rows;
        char buf[50];
        struct tm tm;
        int ndays, dom, num_appts;
	Boolean more, done = FALSE, all_done = TRUE, ps_print_month_appts();
	Range range;
	Abb_Appt *a, *temp_a;
	static Tick tick = 0, total_pages;
	Tick day;
 
	if (first)
		tick = first_date;
	if (num_page > 1)
		tick = prevmonth_exactday(tick);

	/* print days of the week at the top */
	rows = numwks (tick);

	/* need minimum 5 rows to paint miniature months */
	if (rows == 4) rows++;

	if (rows == 5)
		lines_per_box = 7;
	else
		lines_per_box = 5;

	if (num_page == 1)
		total_pages = count_month_pages(c, tick, lines_per_box);

	tm    = *localtime(&tick);
	tm.tm_mday = 1;
#ifdef SVR4
	tm.tm_isdst = -1;
	day   = (int)mktime(&tm);
#else
	day   = (int)timelocal(&tm);
#endif "SVR4"
	ndays = ((tm.tm_mon==1) && leapyr(tm.tm_year+TM_CENTURY))? 29 :
                        monthdays[tm.tm_mon];
	tm   = *localtime(&day);
	dom  = tm.tm_wday;

	ps_init_printer(fp, LANDSCAPE);
	ps_init_month(fp);

	/* print month & year on top */
	format_date(tick, p->ordering_VAL, buf, 0);
	ps_print_header(fp, buf);

	/* print days of week in boxes at top, and monthly grid */
	ps_month_daynames(fp, rows, num_page, total_pages);
	if (total_pages == num_page)
		total_pages = 0;

	/* print the times and text of appts */
	for (i = 1; i <= ndays; i++)
	{

		/* setup a time limit for appts searched */
		lo_hour = (int)lowerbound (day);
		hi_hour = next_ndays(day, 1);

		range.key1 = lo_hour;
		range.key2 = hi_hour;
		range.next = NULL;
		table_abbrev_lookup_range(calendar->view->current_calendar,
				&range, &a);

        	num_appts = count_month_appts(a, c);
		if (num_appts > (lines_per_box * num_page))
			more = TRUE;
		else
			more = FALSE;

		/* clear box & print date */
		ps_month_timeslots(fp, i, dom++, more);
			 
		temp_a = a;

		/* print out times and appts */
		done = ps_print_month_appts (fp, a, num_page, hi_hour, lines_per_box, monthGlance);
		if (!done)
			all_done = FALSE;
 
		if (dom >= 7) dom = 0;
		day = nextday(day);
		destroy_abbrev_appt(temp_a);
	}

	/* paint miniature previous & next month */
	ps_print_little_months(fp, tick);
 
        ps_finish_printer(fp);
	tick = nextmonth(tick);
        
	return(all_done);
}

count_month_appts(a, c)
	Abb_Appt *a;
	Calendar *c;
{
        int count = 0, meoval;
	Props *pr = (Props*)c->properties;
	Lines *lines;

	meoval = pr->meoVAL;

        while (a != NULL) {
                switch(a->privacy) {
                        case public:
                        if ((meoval == 2) || (meoval == 4)
                                 || (meoval == 6)) {
                                        a=a->next;
                                        continue;
                                }
                                break;
                        case semiprivate:
                        if ((meoval == 1) || (meoval == 4)
                                || (meoval == 5)) {
                                        a=a->next;
                                        continue;
                                }
                                break;
                        case private:
                        if ((meoval == 1) || (meoval == 2)
                                || (meoval == 3)) {
                                        a=a->next;
                                        continue;
                                }
                                break;
                        default:
                                break;
                }
		count++;
                a = a->next;
        }
        return(count);
}

static int
count_month_pages(c, start_date, lines_per_box)
        Calendar *c;
	Tick start_date;
	int lines_per_box;
{
        int i;
        int lo_hour, hi_hour, rows, pages;
        char buf[50];
        Props *p = (Props *)c->properties;
        struct tm tm;
        int day, ndays, num_appts, max = 0;
	Range range;
	Abb_Appt *a, *temp_a;
 
	tm    = *localtime(&start_date);
	tm.tm_mday = 1;
#ifdef SVR4
	tm.tm_isdst = -1;
	day   = (int)mktime(&tm);
#else
	day   = (int)timelocal(&tm);
#endif "SVR4"
	ndays = ((tm.tm_mon==1) && leapyr(tm.tm_year+TM_CENTURY))? 29 :
                        monthdays[tm.tm_mon];

	/* print days of the week at the top */
	rows = numwks (start_date);

	/* need minimum 5 rows to paint miniature months */
	if (rows == 4) rows++;

	/* print the times and text of appts */
	for (i = 1; i <= ndays; i++)
	{
		/* setup a time limit for appts searched */
		lo_hour = (int)lowerbound (day);
		hi_hour = next_ndays(day, 1);

		range.key1 = lo_hour;
		range.key2 = hi_hour;
		range.next = NULL;
		table_abbrev_lookup_range(calendar->view->current_calendar,
				&range, &a);

		num_appts = count_month_appts(a, c);
		if (num_appts > max)
			max = num_appts;

		temp_a = a;

		day = nextday(day);
		destroy_abbrev_appt(temp_a);
	}


	pages = max / lines_per_box;
	if ((max % lines_per_box) > 0)
		pages++;
        
	return(pages);
}

/* ARGSUSED */
extern Notify_value
ps_month_button (m, mi)
    Menu m;
    Menu_item mi;
{
        Calendar *c = (Calendar *) xv_get(m, MENU_CLIENT_DATA);

        print_month_pages(c);

	return NOTIFY_DONE;
}


/* ADDED FOR PRINTING */
extern void
get_time_str (a, buf)
        Abb_Appt *a; char *buf;
{
	Calendar *c = calendar;
	Props *p = (Props*)c->properties;
	struct tm *tm;
	int hr, mn;

        if (a==NULL || !a->tag->showtime || magic_time(a->appt_id.tick))
                return;

	tm = localtime(&(a->appt_id.tick));
        hr = tm->tm_hour;
        mn = tm->tm_min;
        buf[0]=NULL;

	if (p->default_disp_VAL == hour12) {
		adjust_hour(&hr);
        	(void) sprintf(buf, "%2d:%02d ", hr, mn);
	}
	else
        	(void) sprintf(buf, "%02d%02d ", hr, mn);
}

static Boolean
hot_box(c, x, y)
	Calendar *c; int x, y;
{
	int margin	= c->view->outside_margin;
	int boxw	= c->view->boxw;
	int boxh	= c->view->boxh;
	int col		= (x-margin)/boxw;
	int row		= (y-c->view->topoffset-margin)/boxh;
	Boolean hot	= false;
	Xv_Font pf	= c->fonts->lucida10b;

	if ((x-(boxw*col+margin) <
             3*xv_get(pf, FONT_COLUMN_WIDTH)) &&
             ((y-(boxh*row)-c->view->topoffset-margin) < 
			xv_get(pf, FONT_DEFAULT_CHAR_HEIGHT)+5))
			hot = true;
	return(hot);
}
	
extern int
month_event(c, event)
	Calendar *c;
	Event *event;
{
	static int lastcol, lastrow;
	static Event lastevent;
	int x, y, i, j, toffset; 
	int boxw, boxh, row, col, margin, id;
	Tick date;
	Props *p = (Props*)c->properties;
	Editor *e = (Editor*)c->editor;
	static Boolean hot_box_flag;

	boxw	= c->view->boxw;
	boxh	= c->view->boxh;
	margin	= c->view->outside_margin;
	date	= c->view->date;
	x	= event_x(event);
	y	= event_y(event);
	id	= event_id(event);
	toffset = c->view->topoffset;

	if (boxw == 0)
		col = 0;
	else 
		col	= (x-margin)/boxw;
	if  (boxh == 0)
		row = 0;
	else 
		row	= (y-c->view->topoffset-margin)/boxh;

	/* boundary conditions */
	if (x < margin || col > 6 || y < (margin+toffset) || row > c->view->nwks-1) {
		calendar_deselect(c);
		lastcol=0; lastrow=0;
		return; 
	}

	switch(id) {
	case LOC_DRAG:
		if (col !=lastcol || row !=lastrow) {
			calendar_deselect(c);
			j = xytoclock(col+1, row+1, date);
			if (j > 0) {
				c->view->olddate = c->view->date;
				c->view->date = j;
				calendar_select(c, daySelect, (caddr_t)NULL);
			}
			lastcol=col;
			lastrow=row;
		}
		break;
	case MS_LEFT:
		if (event_is_down(event)) {    
		 	if (ds_is_double_click(&lastevent, event)) {
				if (lastcol == col && lastrow == row) {
                                        if (cal_update_props())
                                                for (i=0; i < NO_OF_PANES; i++)
                                                        set_rc_vals((Props*)calendar->properties, i);

					set_defaults(e);
					show_editor(c);
				}
			}
			else {
				calendar_deselect(c);
				j = xytoclock(col+1, row+1, date);
				if (j > 0)
					if (hot_box(c, x, y)) {
						c->view->olddate = c->view->date;
						c->view->date = j;
						calendar_select(c, 
							weekSelect, row);
						hot_box_flag = true;
					}
					else {
						c->view->olddate = c->view->date;
						c->view->date = j;
						calendar_select(c, daySelect,
							 (caddr_t)NULL);
					}
			}
			if (editor_showing(e)) {
				set_default_calbox(c);
				reset_date_appts(c);
			}
			lastcol=col;
			lastrow=row;
		}
		else if (xytoclock(col+1, row+1, date) > 0)
			if (hot_box_flag || hot_box(c, x, y)) { 
				week_button((Menu)xv_get(c->items->button2, PANEL_ITEM_MENU), NULL);
				hot_box_flag = false;
			}
		break;
	default:
	break;
	};		/* switch */
	lastevent = *event;

}

