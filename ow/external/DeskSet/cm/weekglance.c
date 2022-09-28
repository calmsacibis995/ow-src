#ifndef lint
static  char sccsid[] = "@(#)weekglance.c 3.29 96/06/11 Copyr 1991 Sun Microsystems, Inc.";
#endif

/* weekglance.c */

#include <stdio.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/textsw.h>
#include <xview/scrollbar.h>
#include <xview/font.h>
#include <xview/cms.h>
#include <ds_listbx.h>
#include "util.h"
#include "graphics.h"
#include "select.h"
#include "timeops.h"
#include "datefield.h"
#include "props.h"
#include "browser.h"
#include "ps_graphics.h"
#include "appt.h"
#include "table.h"
#include "editor.h"
#include "calendar.h"
#include "weekglance.h"
#include "gettext.h"

#define LINES_PER_PAGE 22

static int	paint_entry(), week_xytoclock(), week_xytohour();
static Abb_Appt *fill_day();
static Stat draw_week();
static void draw_chart();
static void refresh_appts();

#define inchart(w, x, y) \
		((x >= w->chart_x && x <= (w->chart_x + w->chart_width) && \
		y >= w->chart_y - w->label_height && \
		y <= (w->chart_y + w->chart_height-1)))

#define inweek(w, x, y) \
		((x >= w->x && x <= w->x + w->width && \
		y >= w->y && y <= w->y + w->day_height) || \
		(x >= w->x + 3 * w->day_width && x <= w->x + w->width && \
		y >= w->y + w->day_height && y <= w->y + w->height))

#define week_hotbox(w, x, y) \
		((x >= w->x && x <= w->x + w->width && \
		y >= w->y && y <= w->y + w->label_height) || \
		(x >= w->x + 3 * w->day_width && x <= w->x + w->width && \
		y >= w->y + w->day_height && y <= w->y + w->day_height + \
		w->label_height))


/* ARGSUSED */
extern Notify_value
week_button (m, mi)
	Menu m;
	Menu_item mi;
{
	Calendar *c =
	  (Calendar *) xv_get (m, MENU_CLIENT_DATA, 0);
	XContext *xc = c->xcontext;
	if (c->view->glance == weekGlance) 
		return NOTIFY_DONE;

	if ( strcmp("C", (char *)xv_get(calendar->frame, XV_LC_DISPLAY_LANG)) ) {
		xv_set(calendar->items->button8, PANEL_LABEL_STRING, LGET("Last Week"), NULL);
		xv_set(calendar->items->button9, PANEL_LABEL_STRING, LGET("This Week"), NULL);
		xv_set(calendar->items->button10, PANEL_LABEL_STRING, LGET("Next Week"), NULL);
	}

	gr_clear_area(xc, 0, 0, c->view->winw, c->view->winh);
	paint_weekview(c, NULL);

	return NOTIFY_DONE;
}
extern void
format_week_header(date, order, buf)
	Tick date;
	Ordering_Type order;
	char *buf;
{
	char tmp_buf[128];
	format_date(date, order, tmp_buf, 1);
	sprintf(buf, MGET("Week Starting %s"), tmp_buf); 
}

extern void
print_week_pages(c)
        Calendar *c;
{

	Props *p = (Props *)c->properties;
	register Tick first_date = c->view->date;
	int num_weeks = atoi(p->repeatVAL);
        int day_of_week = dow(first_date) - 1;  /* returns -1=Sun, etc. */
        Boolean done = FALSE, first = TRUE, print_week();
        int num_page = 1;
        FILE *fp=NULL;

	/* get number of weeks needed to print */
	if (num_weeks <= 0)
		num_weeks = 1;

	first_date = lowerbound(first_date - (day_of_week * daysec)) + 1;
     	/* Start week view on Mon.  Map Sun to last day of week */
        if (day_of_week == -1)
                day_of_week = 6;

        if ((fp=ps_open_file()) == NULL)
                return;

	for (; num_weeks > 0; num_weeks--) {
        	while (!done) {
                	done = print_week(c, num_page, fp, first_date, p, first);
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
print_week (c, num_page, fp, first_date, p, first)
	Calendar *c;
	int num_page;
	FILE *fp;
	Tick first_date;
	Props *p;
	Boolean first;
{
	Boolean more, done = FALSE, all_done = TRUE, ps_print_multi_appts();
	Range range;
	Abb_Appt *a, *temp_a;
	int num_appts, day_of_week;
	static int total_pages;
	char	buf[128];
	int 	i, lo_hour, hi_hour;
	static Tick start_date = 0;

	static char *days[] = {
		"XXX",        (char *)NULL, (char *)NULL, (char *)NULL, 
		(char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL 
	};

 
	days[1] = MGET("Monday %d");
	days[2] = MGET("Tuesday %d");
	days[3] = MGET("Wednesday %d");
	days[4] = MGET("Thursday %d");
	days[5] = MGET("Friday %d");
	days[6] = MGET("Saturday %d");
	days[7] = MGET("Sunday %d");

        if (first)
                start_date = first_date;
	if (num_page > 1)
		start_date = prevweek(start_date);
	else
		total_pages = count_week_pages(c, start_date);

	ps_init_printer(fp, LANDSCAPE);
	ps_init_week(fp);
		 
	format_week_header(start_date, p->ordering_VAL, buf);

	ps_week_header(fp, buf, num_page, total_pages);
		 
	/* print appt boxes */
	ps_week_appt_boxes(fp);
		 
	/* print sched boxes including text info */
	ps_week_sched_boxes(fp);
		 
	/* print the times and text of appts */
	for (i = 1; i <= 7; i++) 
	{
		/* print <Weekday DD> centered at top of appt box */
		ps_week_sched_init();
		sprintf(buf, days[i], dom(start_date));

		/* setup a time limit for appts searched */
		lo_hour = (int)lowerbound (start_date);
		hi_hour = next_ndays(start_date, 1);

		range.key1 = lo_hour;
		range.key2 = hi_hour;
		range.next = NULL;
		table_abbrev_lookup_range(calendar->view->current_calendar,
			&range, &a);
		num_appts = count_multi_appts(a, c);
		if (num_appts > (LINES_PER_PAGE * num_page))
			more = TRUE;
		else
			more = FALSE;

		ps_week_daynames(fp, buf, more);

		temp_a = a;

		/* print out times and appts */
		done = ps_print_multi_appts (fp, a, num_page, hi_hour, weekGlance);

		if (!done)
			all_done = FALSE;

		ps_week_sched_draw(fp, i);
		start_date = nextday(start_date);
		destroy_abbrev_appt(temp_a);
	}

	ps_finish_printer(fp);

	return(all_done);
}

extern int
count_multi_appts(a, c)
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
                if (a->tag->showtime) {
                        count++;
			if (count == LINES_PER_PAGE)
				count++;
		}
                lines = text_to_lines(a->what, 10);
                while (lines != NULL) {
			count++;
                        lines = lines->next;
                }
                a = a->next;
        }        
        return(count);
}

static int
count_week_pages (c, start_date)
	Calendar *c;
	Tick start_date;
{
	Range range;
	Abb_Appt *a, *temp_a;
	int num_appts, i, lo_hour, hi_hour, max = 0, pages;


	/* count the times and text of appts */
	for (i = 1; i <= 7; i++) 
	{
		/* setup a time limit for appts searched */
		lo_hour = (int)lowerbound (start_date);
		hi_hour = next_ndays(start_date, 1);

		range.key1 = lo_hour;
		range.key2 = hi_hour;
		range.next = NULL;
		table_abbrev_lookup_range(calendar->view->current_calendar,
                                &range, &a);
		num_appts = count_multi_appts(a, c);
		if (num_appts > max)
                        max = num_appts;

		temp_a = a;

		start_date = nextday(start_date);
		destroy_abbrev_appt(temp_a);
	}

        pages = max / LINES_PER_PAGE;
        if ((max % LINES_PER_PAGE) > 0)
                pages++;

        return(pages);   
}



/* ARGSUSED */
extern Notify_value
ps_week_button (m, mi)
    Menu m;
    Menu_item mi;
{
    	Calendar *c = (Calendar *) xv_get (m, MENU_CLIENT_DATA, 0);
    	print_week_pages(c);

	return NOTIFY_DONE;
}

extern Stat
paint_weekview(c, rect)
	Calendar *c;
	Rect *rect;
{
	Stat stat;

	c->view->glance = weekGlance;
	init_week(c);
	stat = draw_week(c, rect);
	calendar_select(c, weekdaySelect, NULL);

	return stat;
}

static
init_week(c)
	Calendar *c;
{
	Week *w = (Week *)c->view->week_info;
	int	char_width, char_height;
	Props	*p;
	int	num_hrs,	day_of_week;
	int 	empty_space, day_box;

	/*
	 * The week view starts on Monday.  Map Sunday to the last day of the 
	 * week
	 */
	if ((day_of_week = dow(c->view->date)) == 0)
		day_of_week = 6;
	else
		day_of_week--;
	if (w->current_selection == NULL) {
		w->current_selection = ckalloc(sizeof(Selection));
		((Selection*)w->current_selection)->row = 0;
		((Selection*)w->current_selection)->nunits = 1;
	}
	((Selection*)w->current_selection)->col = day_of_week;

	w->start_date = lowerbound(c->view->date - (day_of_week * daysec)) + 1;

	/*
	 * Set up a bunch of variables which are needed to draw and fill
	 * the week at a glance screen
	 */

	w->canvas_w = (int)xv_get(c->canvas, CANVAS_WIDTH);
	w->canvas_h = (int)xv_get(c->canvas, CANVAS_HEIGHT);
	w->x = c->view->outside_margin;
	w->y = 2 * w->x;
	w->font = c->fonts->lucida12b;  
	w->small_font = c->fonts->lucida10;
	w->small_bold_font = c->fonts->lucida10b;
	char_height = xv_get(w->font, FONT_DEFAULT_CHAR_HEIGHT);
	char_width = xv_get(w->font, FONT_COLUMN_WIDTH);
	w->label_height = char_height * 2;
	w->day_width = (w->canvas_w - 2 * w->x) / 5;
	/* height of box with label */
	w->day_height = (w->canvas_h - 2 * w->y) / 2;
	/*
	 * We compute week dimensions from day dimensions to remove rounding
	 * errors
	 */
	w->width = w->day_width * 5;
	/* height from top of box to bottom of weekend boxes */
	w->height = w->day_height * 2;

	p = (Props *)c->properties;
	w->begin_hour = p->begin_slider_VAL;
	w->end_hour = p->end_slider_VAL;

	/* width of a column in chart */
	w->chart_day_width = (3 * w->day_width - 3 * char_width) / 7;
	/* width of chart */
	w->chart_width = w->chart_day_width * 7;
	num_hrs = w->end_hour - w->begin_hour;

	/* height of box without label */ 
	day_box = w->day_height - w->label_height;

	/* height of an hour in chart */
	w->chart_hour_height = day_box / num_hrs;
	/* chart_hour_height must be evenly divisble by BOX_SEG */
	w->chart_hour_height -= (w->chart_hour_height % BOX_SEG);
	w->chart_height = w->chart_hour_height * num_hrs;

	/* x point of upper left corner of chart */
	w->chart_x = w->x + 3 * xv_get(w->small_bold_font, FONT_COLUMN_WIDTH);
	/* y point of upper left corner of chart */
	w->chart_y = w->y + w->height - w->chart_height;

	/* left over empty space above chart after round off error */
	empty_space = day_box - w->chart_height;
	/* add pixels to the height of each hour box in chart to fill gap*/ 
	if (w->add_pixels = ((double)empty_space / (double)num_hrs)) {
		w->chart_y -= w->add_pixels * num_hrs; 
		w->chart_height += w->add_pixels * num_hrs;
	}

	w->segs_in_array = BOX_SEG * num_hrs * 7;
	if (w->time_array != NULL)
		free(w->time_array);
	w->time_array = (char*)ckalloc(w->segs_in_array);

	c->view->boxw = w->day_width;
	c->view->boxh = w->day_height - w->label_height;
	c->view->outside_margin = w->x;
	c->view->topoffset = w->y;
}
static void
cm_update_segs(w, tick, dur, start_index, end_index, addto)
        Week *w;
        Tick tick, dur;
        int *start_index, *end_index;
	Boolean addto;
{
        int     num_segs, i, start, start_hour, duration, nday;

        start_hour = hour(tick);
       
        if (start_hour >= w->end_hour) {
                *start_index = -1;
                *end_index = -1;
                return;
        }

        if (start_hour < w->begin_hour) {
                start = 0;
                duration = dur - ((w->begin_hour -
                 (start_hour + (double)minute(tick)/(double)60))
                        * hrsec);
        } else{
                start = ((start_hour - w->begin_hour) * 60 + minute(tick));
                duration = dur;
        }
 
        if (duration <= 0) {
                *start_index = -1;
                *end_index = -1;
                return;
        }
        
        nday = (nday=dow(tick))==0? 6: nday-1;
	num_segs = (double)start / (double)MINS_IN_SEG;
        *start_index = (double)start / (double)MINS_IN_SEG + (nday * (w->segs_in_array/7));
	if (start - (num_segs * MINS_IN_SEG) > 7) 
		(*start_index)++;
        num_segs = ((double)duration / (double)60 / (double)MINS_IN_SEG);
	*end_index = num_segs + *start_index;
	if (((double)duration/(double)60-MINS_IN_SEG*num_segs) > 7)
		(*end_index)++;
		  
        if (*end_index > (i = ((nday + 1) * (w->segs_in_array / 7))) )
                *end_index = i;
 
        for (i = *start_index; i < *end_index; i++)
		if (addto)
			w->time_array[i]++;
                else
                        w->time_array[i]--;
}
static void
add_extra_pixels(i, num_pixels, h)
	int i, num_pixels;
	int *h;
{
	if (((i+1) % BOX_SEG) == 0)
		*h += num_pixels;
}

static void
chart_draw_appts(w, start, end)
        Week *w;
        int start, end;
{
        int x, y, h, i, segs_in_col, no_boxes;
	Calendar *c = calendar;
	int col_remainder, boxseg_h;

	h = boxseg_h = (double)w->chart_hour_height / (double)BOX_SEG;
        segs_in_col = (w->segs_in_array / 7);
        /* find starting segment in column */
        col_remainder =  (start % segs_in_col);

        no_boxes = (double)(col_remainder+1) / (double)BOX_SEG;
        y = w->chart_y + ((double)col_remainder * (double)boxseg_h) + 
			((double)w->add_pixels * (double)no_boxes) + 1;
        x = w->chart_x + ((double)start / (double)segs_in_col * 
			(double)w->chart_day_width);

        for (i = start; i < end; i++) {
                if (w->time_array[i] == 0) {
			/* batch up repaints */
                        if ( (i+1) < w->segs_in_array && 
				w->time_array[i+1] == 0 &&
				((i+1) % segs_in_col) != 0 ) {
                                h += boxseg_h;
				add_extra_pixels(i, w->add_pixels, &h);
                                continue;
                        }
			add_extra_pixels(i, w->add_pixels, &h);
			gr_clear_area(c->xcontext, x, y, w->chart_day_width, h);
		}
                else if (w->time_array[i] == 1) {
			/* batch up for one repaint */
                        if ( (i+1) < w->segs_in_array
                                 && w->time_array[i+1] == 1 &&
                                 ((i+1) % segs_in_col) != 0 ) {
                                h += boxseg_h;
				add_extra_pixels(i, w->add_pixels, &h);
                                continue;
                        }
			add_extra_pixels(i, w->add_pixels, &h);
			if (c->xcontext->screen_depth < 8) 
				gr_make_gray(c->xcontext, x, y,
					w->chart_day_width, h, 25);
			else 
                        	gr_make_grayshade(c->xcontext, x, y, 
					w->chart_day_width, h, LIGHTGREY);
		}
                else if (w->time_array[i] == 2) {
			/* batch up for one repaint */
                        if ( (i+1) < w->segs_in_array
                                 && w->time_array[i+1] == 2 &&
                                 ((i+1) % segs_in_col) != 0 ) {
                                h += boxseg_h;
				add_extra_pixels(i, w->add_pixels, &h);
                                continue;
                        }
			add_extra_pixels(i, w->add_pixels, &h);
			if (c->xcontext->screen_depth < 8)
                        	gr_make_gray(c->xcontext, x, y, 
					w->chart_day_width, h, 50);
			else
                        	gr_make_rgbcolor(c->xcontext, x, y, 
					w->chart_day_width, h,
                                        MIDGREY, MIDGREY, MIDGREY);
                }
                else if (w->time_array[i] >= 3) {
			/* batch up for one repaint */
                        if ( (i+1) < w->segs_in_array
                                && w->time_array[i+1] >= 3 &&
                                ((i+1) % segs_in_col) != 0 ) {
                                h += boxseg_h;
				add_extra_pixels(i, w->add_pixels, &h);
                                continue;
                        }
			add_extra_pixels(i, w->add_pixels, &h);
			if (c->xcontext->screen_depth < 8)
                        	gr_make_gray(c->xcontext, x, y, 
					w->chart_day_width, h, 75);
			else 
                        	gr_make_grayshade(c->xcontext, x, y, 
					w->chart_day_width, h, DIMGREY);
		}
		if (i != 0 && (((i+1) % segs_in_col) == 0)) {
			x += w->chart_day_width;
			y = w->chart_y + 1;
			h = ((double)w->chart_hour_height/(double)BOX_SEG);
		}
		else {
			y += h;
			h = boxseg_h;
		}
        }
}

static Stat
draw_week(c, rect)
	Calendar	*c;
	Rect *rect;
{
	Week *w = (Week *)c->view->week_info;
	Abb_Appt	*head, *list = NULL, *a = NULL;
	register int	current_day;
	register int	n;
	register int	x, y;
	register int	middle;
	int		char_height;
	int		start_date;
	char		**day_names;
	char		label[80];
	struct Range	range;
	Font_string_dims dims1, dims2, dims3;
	int		start_ind, end_ind;
	int 		today_dom, day_om;
	XContext	*xc;
	Boolean inrange = false;
	Props 		*p = (Props*)c->properties;
	Stat 		stat = access_ok;
	Rect 		chartrect;
	
	char_height	= xv_get(w->font, FONT_DEFAULT_CHAR_HEIGHT);
	start_date	= w->start_date;
	xc		= c->xcontext;

	range.key1 = lowerbound(start_date);
	range.key2 = next_ndays(range.key1 + 1, 7);
	range.next = NULL;
	stat = table_abbrev_lookup_range(c->view->current_calendar, 
					&range, &list);
	if (stat == status_param && c->general->version <= CMS_VERS_3)
		return status_param;
	gr_clear_box(xc, 0, 0, w->canvas_w, w->canvas_h);

	xv_get(w->font, FONT_STRING_DIMS, days2[3], &dims1);
	xv_get(w->font, FONT_STRING_DIMS, "   00", &dims2);
	xv_get(w->font, FONT_STRING_DIMS, "Wed  00", &dims3);
	if (dims1.width + dims2.width <= w->day_width - 2)
		day_names = days2;
	else if (dims3.width <= w->day_width - 2)
		day_names = days;
	else
		day_names = days3;

	x = w->x;
	y = w->y;

	format_week_header(start_date, p->ordering_VAL, label);
	gr_text(xc, x, y - char_height / 2, w->font, label, rect);

	/*
	 * Draw bold box around first 5 days
	 */
	gr_draw_box(xc, x, y, w->width, w->day_height, rect);
	gr_draw_box(xc, x - 1, y - 1, w->width + 2, w->day_height + 2, rect);
	gr_draw_line(xc, x, y + w->label_height, x + w->width,
		y + w->label_height, gr_solid, rect);

	/*
	 * Draw bold box around last 2 days
	 */
	x += 3 * w->day_width;
	y += w->day_height;

	gr_draw_box(xc, x, y, 2 * w->day_width, w->day_height, rect);
	gr_draw_box(xc, x - 1, y, 2 * w->day_width + 2, w->day_height + 1, rect);
	gr_draw_line(xc, x, y + w->label_height,
		x + 2 * w->day_width, y + w->label_height, gr_solid, rect);
	y = w->y;
	x = w->x + w->day_width;
	for (n = 0; n < 4; n++) {
		if (n < 3) {
			gr_draw_line(xc, x, y, x, y + w->day_height, gr_solid, rect);
		} else {
			gr_draw_line(xc, x, y, x, y + 2 * w->day_height, gr_solid, rect);
		}
		x += w->day_width;
	}


	/*
	 * Fill in week with appointments
	 */
	x = w->x;
	y = w->y;
	head = list;
	current_day = start_date;
	inrange = today_inrange(c, range.key1+1);
	today_dom = dom(time(0));

	for (n = 0; n < 7; n++) {
		if (n == 5) {
			y += w->day_height;
			x = w->x + 3 * w->day_width;
		}

		day_om = dom(current_day);
		sprintf(label, "%s %d",
		   n == 6 ? day_names[0] : day_names[n + 1], day_om);

		middle = gr_center(w->day_width, label, w->font);
		if (c->xcontext->screen_depth >= 8 && inrange
			&& today_dom == day_om)
			gr_text_rgb(xc, x + middle , 
				y + char_height + char_height / 4, 
				w->font, label, CMS_BACKGROUND_PIXEL, 
				xv_get(c->panel, WIN_CMS), rect);
		else
			gr_text(xc, x + middle , 
				y + char_height + char_height / 4, 
				w->font, label, rect);
		list = fill_day(c, w, x, y, current_day, list, rect);
		x += w->day_width;
		current_day += daysec;
	}
	if (rect != NULL) {
                chartrect.r_left = w->x;
                chartrect.r_top = w->chart_y - w->label_height;
                chartrect.r_width = w->chart_width + 
			3*xv_get(w->small_bold_font, FONT_COLUMN_WIDTH);
                chartrect.r_height = w->chart_height + 2*w->label_height;
        }

	if (rect == NULL || myrect_intersectsrect(rect,  &chartrect)) {
		for(a = head; a != NULL;  a = a->next) 
			cm_update_segs(w, a->appt_id.tick, a->duration, 
				&start_ind, &end_ind, true);
		chart_draw_appts(w, 0, w->segs_in_array);
		draw_chart(c, w, NULL);
	}

	destroy_abbrev_appt(head);
	return stat;
}
static void
init_array_range(array, start, end)
	char *array;
	int start, end;
{
	register int i;

        for (i = start; i <= end; i++)
                array[i] = 0;
}

extern void
wk_update_entries(c, date)
	Calendar	*c;
	int		date;
{
	int		x, y, chart_x, chart_y, char_width;
	int		n;
	struct Range	range;
	XContext	*xc;
	Abb_Appt	*a = NULL, *list = NULL;
	Week 		*w = (Week *)c->view->week_info;
	int start_index, end_index, si, ei;

	/*
	 * Get the day of the week we need to re-draw, and get the 
	 * corrdinates associated with that day.
	 */
	((n = dow(date)) == 0) ? n = 6 : n--;

	chart_y = w->chart_y;
	chart_x = w->chart_x + n * w->chart_day_width;

	start_index = n * (w->segs_in_array / 7);
	end_index = start_index + (w->segs_in_array / 7 );

	if (n < 5) {
		y = w->y;
		x = w->x + n * w->day_width;
	} else {
		n -= 2;
		x = w->x + n * w->day_width;
		y = w->y + w->day_height;
	}

	/*
	 * Get entries for the day
	 */
	range.key1 = lowerbound(date);
	range.key2 = next_ndays(date, 1);
	range.next = NULL;
        table_abbrev_lookup_range(c->view->current_calendar, &range, 
					&list);
	/*
	 * Clear old data from week day and chart
	 */
	xc = c->xcontext;
	char_width = xv_get(w->small_font, FONT_COLUMN_WIDTH);
	gr_clear_area(xc, x + char_width-2, y + w->label_height + 1,
		w->day_width - char_width+2,
                w->day_height - w->label_height - 1);

        gr_clear_area(xc, chart_x + 1, chart_y + 1, w->chart_day_width - 1,
		w->chart_height - 1);

	/*
	 * Paint in new appointments
	 */
	fill_day(c, w, x, y, date, list, NULL);

	/* init array for day segs */ 
	init_array_range(w->time_array, start_index, end_index);

	for(a = list; a != NULL;  a = a->next)
                cm_update_segs(w, a->appt_id.tick, a->duration,
                                &si, &ei, true);
	chart_draw_appts(w, start_index, end_index);
	draw_chart(c, w, NULL);

	destroy_abbrev_appt(list);
}

static Abb_Appt *
fill_day(c, w, x, y, day, list, rect)
	Calendar*c;
	Week	*w;
	int	x, y;
	int	day;
	Abb_Appt	*list;
	Rect *rect;
{
	register Abb_Appt	*a;
	register int	lower = (int)lowerbound(day);
	register int	upper = (int)next_ndays(day, 1);
	register int	n;
	register int	nlines = 0;
	int	char_width = xv_get(w->small_font, FONT_COLUMN_WIDTH);
	int	char_height = xv_get(w->small_font, FONT_DEFAULT_CHAR_HEIGHT);
	int	maxlines = ((w->day_height - w->label_height) / char_height)- 1;
	int	maxchars = (w->day_width / char_width);

#if 0
	x += char_width;
#endif
	x += 3;
	y += (w->label_height + char_height);

	/*
	 * Fill in a day with appointments 
	 */
	a = list;
	while  (a != NULL && a->appt_id.tick > lower 
			&& a->appt_id.tick < upper) {
		if (nlines < maxlines) {
			n = paint_entry(c, x, y, maxchars, a, rect);
			y += n * char_height;
			nlines += n;
		}
		a = a->next;
	}
	return(a);
}

static int
paint_entry(c, x, y, maxchars, a, rect)
	Calendar *c;
	int	x, y;
	int	maxchars;
	Abb_Appt*a;
	Rect *rect;
{
	Props *p = (Props*)c->properties;
	int		nlines = 0;
	XContext	*xc = c->xcontext;
	char		buf1[128], buf2[WHAT_LEN+1];
	Week 		*w = (Week *)c->view->week_info;

	/*
	 * Write an appointment entry into a day
	 */

	if (maxchars >= 40)		/* maxed out possible=40 */
		maxchars = 40;

	buf1[0]=NULL; buf2[0]=NULL;

	format_line2(a, buf1, buf2, p->default_disp_VAL);

	if (a->tag->showtime && !magic_time(a->appt_id.tick) && (buf1[0] != NULL)) {
		maxchars = 
			gr_nchars(w->day_width - 5, buf1, c->fonts->lucida10b);
		buf1[min(cm_strlen(buf1), maxchars)]=NULL;
		gr_text(xc, x, y, c->fonts->lucida10b, buf1, rect);
		nlines++;
		y += xv_get(c->fonts->lucida10b, FONT_DEFAULT_CHAR_HEIGHT);
	}
	if (buf2[0] != '\0') {
		maxchars =
			gr_nchars(w->day_width - 5, buf2, c->fonts->lucida10);
		buf2[min(cm_strlen(buf2), maxchars)]=NULL;
		gr_text(xc, x, y, c->fonts->lucida10, buf2, rect);
		nlines++;
	}

	return(nlines);
}


static void
draw_chart(c, w, rect)
	Calendar *c;
	register Week *w;
	Rect *rect;
{
	register int	x, y;
	int	n;
	int	char_height = xv_get(w->font, FONT_DEFAULT_CHAR_HEIGHT);
	char	label[5];
	XContext *xc = c->xcontext;
	Props *p = (Props*)c->properties;

	/*
	 * Draw chart. We first draw all the lines, then the labels
	 * so that Xlib can batch the lines into 1 X request
	 */

	/* Draw horizontal lines for time */
	x = w->chart_x;
	y = w->chart_y;
	for (n = w->begin_hour; n <= w->end_hour; n++) {
		gr_draw_line(xc, x, y, x + w->chart_width, y, gr_solid, rect);
		y += w->chart_hour_height + w->add_pixels;
	}

	/* Draw vertical lines for days */
	y = w->chart_y;
	for (n = 0; n < 7; n++) {
		gr_draw_line(xc, w->chart_x + (w->chart_day_width * n),
			y, w->chart_x + (w->chart_day_width * n),
			y + w->chart_height, gr_solid, rect);
	}

	/*
	 * Draw box around the whole thing.
	 */
	gr_draw_box(xc, w->chart_x, w->chart_y, w->chart_width,
		    w->chart_height, rect);
	gr_draw_box(xc, w->chart_x - 1, w->chart_y - 1,
		    w->chart_width + 2, w->chart_height + 2, rect);

	/* Label horizontal lines with time of day */
	x = w->chart_x;
	y = w->chart_y;
	for (n = w->begin_hour; n <= w->end_hour; n++) {
		if (p->default_disp_VAL == hour12)
			sprintf(label, "%2d", n > 12 ? n - 12 : n);
		else
			sprintf(label, "%2d", n);
		gr_text(xc, w->x - 1, y+3, 
			w->small_bold_font, label, rect);
		y += w->chart_hour_height + w->add_pixels;
	}

	/* Label  vertical lines with day labels */
	y = w->chart_y;
	for (n = 0; n < 7; n++) {
		x = gr_center(w->chart_day_width, days3[n+1], w->font);
		gr_text(xc, w->chart_x + (w->chart_day_width * n) + x,
			y - char_height / 2, w->font, days3[n+1], rect);
	}

}

/* ARGSUSED */
extern void
week_event(c, event)
	Calendar *c;
	Event *event;
{
	Props *p = (Props*)c->properties;
	static int lastdate;
	static Event lastevent;
	int x, y, i, j, hr, id;
	Week 	*w = (Week *)c->view->week_info;
	Selection *wsel;
	Editor *e;
	static int lastrow, lastcol;
	int row, col;

	e	= (Editor *)c->editor;
	x	= event_x(event);
	y	= event_y(event);
	id	= event_id(event);
	wsel 	= (Selection *)w->current_selection;

	switch(id) {
	case LOC_DRAG:
		j = week_xytoclock(w, x, y);
		(col = dow(j)) == 0 ? col = 6 : col--;
		if (inchart(w, x, y)) 
			row = (double)(y - w->chart_y) / 
				(double)(w->chart_hour_height+ w->add_pixels);
		else 
			row = wsel->row;
		if (j != lastdate || lastcol != col || lastrow != row) { 
			refresh_appts(c, lastdate);
			                /* patch added to fix "double click" */
			                /* bug #1140009 */
			calendar_deselect(c);
			wsel->row = row;
			wsel->col = col;
			if (j > 0) {
				c->view->olddate = c->view->date;
				c->view->date = j;
				calendar_select(c, weekdaySelect, NULL);
			}
		}
		lastcol = wsel->col;
		lastrow = wsel->row;
		lastdate = c->view->date;
		break;
	case MS_LEFT:
		if (event_is_down(event)) {    
		 	if (ds_is_double_click(&lastevent, event)) { 
				j = week_xytoclock(w, x, y);
				if (j == lastdate) {
                                	if (cal_update_props())
                                                for (i=0; i < NO_OF_PANES; i++)
                                                        set_rc_vals((Props*)calendar->properties, i);
					set_defaults(e);
					show_editor(c);
				}
				if (inchart(w, x, y) && editor_showing(e)) {
					hr = week_xytohour(w, x, y);
					set_editor_time(p, hr, e);
				}
			}
			else {
				refresh_appts(c, lastdate);
			                /* patch added to fix "double click" */
			                /* bug #1140009 */
				calendar_deselect(c);
				j = week_xytoclock(w, x, y);
				(wsel->col = dow(j)) == 0 ? 
					wsel->col = 6 : wsel->col--;
				if (inchart(w, x, y)) 
					wsel->row = (double)(y - w->chart_y) / 
					(double)(w->chart_hour_height + w->add_pixels);
				if (j > 0) {
					c->view->olddate = c->view->date;
					c->view->date = j;
					calendar_select(c, weekdaySelect, NULL);
				}
				if (week_hotbox(w, x, y)) 
					calendar_select(c, weekhotboxSelect,
							 NULL);
				else if (editor_showing(e)) {
					hr = week_xytohour(w, x, y);
					set_editor_time(p, hr, e);
				}
			}
			lastdate = c->view->date;
			lastcol = wsel->col;
			lastrow = wsel->row;
			if (editor_showing(e)) {
				set_default_calbox(c);
				reset_date_appts(c);
			}
		}
		else {
			/* hotbox is above appt box */
			if (week_hotbox(w, x, y)) 
				paint_day(c);
		}
		break;
	default:
		break;
	};		/* switch */
	lastevent = *event;

}

static int
week_xytohour(w, x, y)
	Week	*w;
	int	x, y;
{
	if (!inchart(w, x, y)) 
		return(-1);
	y -= w->chart_y;
	return(w->begin_hour + ((double)y / 
		(double)(w->chart_hour_height + w->add_pixels)));
}

static int
week_xytoclock(w, x, y)
	Week	*w;
	int	x, y;

{
	int	dow;

	/*
	 * Convert the x and y location on the week view to a date
	 */
	if (inchart(w, x, y)) {
		dow = (double)(x - w->chart_x)/(double)w->chart_day_width;
	} else if (inweek(w, x, y)) {
		if (y < w->y + w->day_height)
			dow = (x - w->x)/w->day_width;
		else
			dow = (x - w->x - 3 * w->day_width)/w->day_width + 5;
	} else
		return(0);

	return(w->start_date + dow * daysec);
}

/*
 *    Patch for "disappearing appt" bug #1140009
 *    (scavin 19Aug93)
 *    Cloned from "wk_update_entries"
*/
extern void
refresh_appts(c, date)
	Calendar	*c;
	int		date;
{
	int		n;
	struct Range	range;
	XContext	*xc;
	Abb_Appt	*a = NULL, *list = NULL;
	Week 		*w = (Week *)c->view->week_info;
	int start_index, end_index, si, ei;

	/*
	 * Get the day of the week we need to refresh
	 */
	((n = dow(date)) == 0) ? n = 6 : n--;

	start_index = n * (w->segs_in_array / 7);
	end_index = start_index + (w->segs_in_array / 7 );

	/*
	 * Get entries for the day
	 */
	range.key1 = lowerbound(date);
	range.key2 = next_ndays(date, 1);
	range.next = NULL;
        table_abbrev_lookup_range(c->view->current_calendar, &range, 
					&list);
	/*
	 * Refresh appointment table
	 */
	/* init array for day segs */ 
	init_array_range(w->time_array, start_index, end_index);

	for(a = list; a != NULL;  a = a->next)
                cm_update_segs(w, a->appt_id.tick, a->duration,
                                &si, &ei, true);
	destroy_abbrev_appt(list);
}
