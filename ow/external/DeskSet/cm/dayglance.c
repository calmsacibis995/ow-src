#ifndef lint
static  char sccsid[] = "@(#)dayglance.c 3.32 97/05/16 Copyr 1991 Sun Microsystems, Inc.";
#endif

/* dayglance.c */

#include <stdio.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/scrollbar.h>
#include <xview/font.h>
#include <xview/cms.h>
#include "util.h"
#include "timeops.h"
#include "datefield.h"
#include "props.h"
#include "appt.h"
#include "table.h"
#include "ps_graphics.h"
#include "graphics.h"
#include "select.h"
#include "dayglance.h"
#include "calendar.h"
#include "editor.h"
#include "gettext.h"

extern int debug;

#define INSIDE_MARGIN 6


/* for positioning and drawing day view boxes */

static void
paint_month_header(c, date, y, w, rect)
	Calendar *c;
	Tick date;
	int y, w;
	Rect *rect;
{
	char buf[40];
	int x, h = xv_get(c->fonts->lucida14b, FONT_DEFAULT_CHAR_HEIGHT);
	struct tm tm;
	Props *p = (Props*)c->properties;

        buf[0]  = '\0';
	tm = *localtime(&date);
	if (p->ordering_VAL == Order_YMD)
        	(void) sprintf(buf, "%d %s", tm.tm_year+TM_CENTURY, months[tm.tm_mon+1]);
	else
        	(void) sprintf(buf, "%s %d", months[tm.tm_mon+1], tm.tm_year+TM_CENTURY);
	x = gr_center(w, buf, c->fonts->lucida14b) 
		+ c->view->outside_margin;
        gr_clear_box(c->xcontext, c->view->outside_margin, y-h+2, w, h);
        gr_text(c->xcontext, x, y, c->fonts->lucida14b, buf, rect);
}
extern void
paint_day_header(c, date, rect)
	Calendar *c;
        Tick date;
	Rect *rect;
{
	Props *p = (Props*)c->properties;
	Day *day_info = (Day *)c->view->day_info;
	int pfy, x;
	char buf[100];
	Boolean inrange = false;

        pfy= xv_get(c->fonts->lucida14b, FONT_DEFAULT_CHAR_HEIGHT);
        buf [0] = '\0';
        format_date(date, p->ordering_VAL, buf, 1);
	inrange = today_inrange(c, date);
	x = gr_center(c->view->winw-(int)MOBOX_AREA_WIDTH, buf,
			c->fonts->lucida14b) + (int)MOBOX_AREA_WIDTH;
	if (c->xcontext->screen_depth >= 8 && inrange) 
        	gr_text_rgb(c->xcontext, x, c->view->topoffset - (pfy/2),
                        c->fonts->lucida14b, buf, CMS_BACKGROUND_PIXEL,
			xv_get(c->panel, WIN_CMS), rect);
	else
        	gr_text(c->xcontext, x, c->view->topoffset - (pfy/2),
                        c->fonts->lucida14b, buf, rect);

	pfy = xv_get(c->fonts->fixed12b, FONT_DEFAULT_CHAR_HEIGHT);
	paint_month_header(c, day_info->month1, c->view->topoffset - pfy/2,
		 day_info->mobox_width, rect);
	paint_month_header(c, day_info->month2, day_info->month2_y - pfy/2,
		 day_info->mobox_width, rect);
	paint_month_header(c, day_info->month3, day_info->month3_y - pfy/2,
		 day_info->mobox_width, rect);
}
	
extern void
paint_dayview_hour(c, hr_tick, y, rect)
	Calendar *c;
	Tick hr_tick;	/* lowerbound of the hour, e.g. 7:00 */
	int y;
	Rect *rect;
{
	Range range;
	Abb_Appt *a = NULL, *head = NULL;
	int w = c->view->boxw;
	int h = c->view->boxh;
	int x;
	char *appt_str;
	Lines *lines = NULL, *headlines = NULL;
	int pfy, curr_line, maxlines;
	Xv_Font pf = c->fonts->lucida10b;
	Xv_Font pf2 = c->fonts->lucida10;
	Props *p = (Props*)c->properties;

	/* draw in appointments */
	range.key1 = hr_tick;
        range.key2 = next_nhours(range.key1+1, 1);
        range.next = NULL;
        table_abbrev_lookup_range(c->view->current_calendar, &range, &a);
        head = a;
	
	x = MOBOX_AREA_WIDTH + HRBOX_MARGIN + 6;
	pfy     = xv_get(pf, FONT_DEFAULT_CHAR_HEIGHT);
	maxlines = (h - 6) / pfy;
	curr_line = 0;
	gr_clear_box(c->xcontext, x-2, y+2, w-HRBOX_MARGIN, h-4);
	y += pfy;
	while (a != NULL) {
		if (curr_line < maxlines) {
			headlines = lines = text_to_lines(a->what, 4);
			if (lines != NULL && lines->s != NULL) { 
				appt_str = ckalloc(cm_strlen(lines->s)+18);
				format_line(a->appt_id.tick, lines->s, appt_str, 
					a->duration, a->tag->showtime, 
					p->default_disp_VAL);
				lines = lines->next;
			}
			else {
				appt_str = ckalloc(15);
				format_line(a->appt_id.tick, (char*)NULL, 
					appt_str, a->duration, a->tag->showtime, 
					p->default_disp_VAL);
			}
			appt_str[cm_strlen(appt_str)]=NULL;
			gr_text(c->xcontext, x, y, pf, appt_str, rect);
			y += pfy;
			free(appt_str); appt_str = NULL;
			curr_line++;
			if (curr_line < maxlines && lines != NULL) {
				appt_str = ckalloc(324);
				cm_strcpy(appt_str, "    ");
				while(lines != NULL)
					if (lines->s != NULL) {
						cm_strcat(appt_str, lines->s);
					lines = lines->next;
					if (lines != NULL && lines->s != NULL)
						cm_strcat(appt_str, " - ");
				}
				gr_text(c->xcontext, x, y, pf2, appt_str, rect);
				y += pfy;
				free(appt_str); appt_str = NULL;
				curr_line++;
			}
			destroy_lines(headlines); lines=NULL;
			a = a->next;
		}
		else 
			break;
	}
	destroy_abbrev_appt(head);

}

extern int
morning(hr)
	int hr;
{
	return(hr<12);
}
static void
paint_dayview_appts(c, a, rect)
	Calendar *c;
	Abb_Appt *a;
	Rect *rect;
{
	int w = c->view->boxw;
	int h = c->view->boxh;
	int begin_time, end_time;
	int x, x2, y, y2, num_hrs, i, last_hr, hr, x_off;
	Xv_Font pf = c->fonts->lucida10b;
	Xv_Font pf2 = c->fonts->lucida10;
	Props *p = (Props*)c->properties;
	Boolean am = true;
	char buf[5], *appt_str;
	int pfy, curr_line, maxlines;
	Lines *lines = NULL, *headlines = NULL;
	Abb_Appt *head = NULL;

	/* draw horizontal lines */
	begin_time = p->begin_slider_VAL;
	end_time = p->end_slider_VAL;
	num_hrs = end_time - begin_time;
	x = MOBOX_AREA_WIDTH+2;
	x2 = x + w;
	y = c->view->topoffset;
	for (i = 0; i < num_hrs; i++) {
		gr_draw_line(c->xcontext, x, y, x2, y, gr_solid, rect);
		y += h;
	}
	/* draw vertical line */
	y = c->view->topoffset;
	y2 = c->view->winh;
	x += HRBOX_MARGIN;
	gr_draw_line(c->xcontext, x, y, x, y2, gr_solid, rect);

	x = MOBOX_AREA_WIDTH+3;
	y += h/2+4;
	/* draw in hours */
	for (i = begin_time; i < end_time; i++) {
		hr = i;
		if (p->default_disp_VAL == hour12) {
			am = adjust_hour(&hr);
			(void) sprintf(buf, "%d%s", hr, am ? "a" : "p");
		}
		else
			(void) sprintf(buf, "%02d", hr);
		x_off = gr_center(HRBOX_MARGIN, buf, pf); 
		gr_text(c->xcontext, x+x_off, y, pf, buf, rect);
		y += h;
	}

	/* draw in appointments */
        head = a;

	x = MOBOX_AREA_WIDTH + HRBOX_MARGIN + 6;
	pfy     = xv_get(pf, FONT_DEFAULT_CHAR_HEIGHT);
	maxlines = (h - 6) / pfy;
	curr_line = last_hr = 0;
	while (a != NULL) {
		hr = hour(a->appt_id.tick);
		if (hr >= begin_time && hr < end_time) {
			if (last_hr != hr) curr_line = 0;
			y = c->view->topoffset + 2 + pfy;
			if (curr_line < maxlines) {
				y += (curr_line * pfy) + h * (hr - begin_time);
				headlines = lines = text_to_lines(a->what, 4);
				if (lines != NULL && lines->s != NULL) { 
					appt_str = ckalloc(cm_strlen(lines->s)+18);
					format_line(a->appt_id.tick, lines->s, 
						appt_str, a->duration, 
						a->tag->showtime, 
						p->default_disp_VAL);
					lines = lines->next;
				}
				else {
					appt_str = ckalloc(15);
					format_line(a->appt_id.tick, (char*)NULL, 
						appt_str, a->duration, 
						a->tag->showtime, 
						p->default_disp_VAL);
				}
				appt_str[cm_strlen(appt_str)]=NULL;
				gr_text(c->xcontext, x, y, pf, appt_str, rect);
				free(appt_str); appt_str = NULL;
				curr_line++;
				if (curr_line < maxlines && lines != NULL) {
				 	appt_str = ckalloc(324);
					cm_strcpy(appt_str, "    ");
					while (lines != NULL) { 
						if (lines->s != NULL) 
							cm_strcat(appt_str, lines->s);
						lines = lines->next;
						if (lines != NULL && lines->s != NULL)
							cm_strcat(appt_str, " - ");
					}
					y += pfy;
					gr_text(c->xcontext, x, y, pf2, 
						appt_str, rect);
					curr_line++;
					free(appt_str); appt_str = NULL;
				}
				destroy_lines(headlines); lines=NULL;
			}
		}
		a = a->next;
		if (a != NULL) {
			last_hr = hr;
			hr = hour(a->appt_id.tick);
		}
	}
	destroy_abbrev_appt(head);
}

init_mo(c)
	Calendar *c;
{
	Day *day_info = (Day *)c->view->day_info;
	day_info->month1 = previousmonth(c->view->date);
	day_info->month2 = c->view->date;
	day_info->month3 = nextmonth(c->view->date);
}

extern void 
init_dayview(c)
	Calendar *c;
{
        int  tot_rows, wks_1, wks_2, wks_3;
	int w = (int) xv_get(c->canvas, XV_WIDTH);
        int h = (int) xv_get(c->canvas, XV_HEIGHT);
	Day *day_info = (Day *) c->view->day_info;

	(void)cache_dims(c, w, h);

	day_info->day_selected = -1;
	day_info->mobox_width = (int)MOBOX_AREA_WIDTH - 
			2*c->view->outside_margin;
	/* col width of day number in month boxes */
        day_info->col_w = 
		(day_info->mobox_width-INSIDE_MARGIN*2)/7;
	/* width of all of the month boxes */
        day_info->mobox_width = 7 * day_info->col_w + 
			2 * INSIDE_MARGIN;

	wks_1 = numwks(day_info->month1);
	wks_2 = numwks(day_info->month2);
	wks_3 = numwks(day_info->month3); 
	/* total rows in three months */ 
	tot_rows = wks_1 + wks_2 + wks_3 + 3; 

	/* row height of day number in month boxes */
        day_info->row_h = (c->view->winh - 3*c->view->topoffset-c->view->outside_margin) 
			/ tot_rows;

	/* height of 1st month */
        day_info->mobox_height1 = day_info->row_h * 
			(wks_1+1)+1;
	/* height of 2nd month */
        day_info->mobox_height2 = day_info->row_h * 
			(wks_2+1)+1;
	/* height of 2rd month */
        day_info->mobox_height3 = day_info->row_h * 
			(wks_3+1)+1;

	day_info->month1_y = c->view->topoffset;
	day_info->month2_y = 2*c->view->topoffset + 
			day_info->mobox_height1; 
	day_info->month3_y = 3*c->view->topoffset + 
			day_info->mobox_height1 +
			day_info->mobox_height2;
	((Selection*)(c->view->current_selection))->row = 0;
}
extern Tick
monthbox_xytodate(c, x, y)
	Calendar *c;
	int x, y;
{
	char str[5];
	Day *day_info = (Day *)c->view->day_info;
	int pfy   = xv_get(c->fonts->fixed12b, FONT_DEFAULT_CHAR_HEIGHT);
	int col_w = day_info->col_w;
        int row_h = day_info->row_h;
	int row, col, x_off;
	int day_selected, tmpx;

	col = (x-c->view->outside_margin-INSIDE_MARGIN) / col_w;
	if (col < 0) return;
	tmpx = c->view->outside_margin + INSIDE_MARGIN + col * col_w;

	if (y < (day_info->month1_y + 
			day_info->mobox_height1)) {
		row = (y-day_info->month1_y-row_h) / row_h;
        	day_selected = (7 * (row+1)) - fdom(day_info->month1)
				 - (6 - col);
		if (day_selected <= 0 || day_selected >
			 monthlength(day_info->month1)) 
				return;
		day_info->day_selected = day_selected;
		day_info->day_selected_y = 
			day_info->month1_y + (row+1)*row_h; 
		c->view->olddate = c->view->date;
		c->view->date = next_ndays(first_dom(day_info->month1),
			 day_info->day_selected);
	}
	else if (y < (day_info->month2_y +
                        day_info->mobox_height2)) {
		row = (y-day_info->month2_y-row_h) / row_h;
        	day_selected = (7 * (row+1)) - fdom(day_info->month2)
				 - (6 - col);
		if (day_selected <= 0 || day_selected >
			 monthlength(day_info->month2)) 
				return;
		day_info->day_selected = day_selected;
		day_info->day_selected_y = 
			day_info->month2_y + (row+1)*row_h; 
		c->view->olddate = c->view->date;
		c->view->date = next_ndays(first_dom(day_info->month2),
			 day_info->day_selected);
	}
	else if (y < (day_info->month3_y +
                        day_info->mobox_height3)) {
		row = (y-day_info->month3_y-row_h) / row_h;
        	day_selected = (7 * (row+1)) - fdom(day_info->month3) 
				- (6 - col);
		if (day_selected <= 0 || day_selected > 
				monthlength(day_info->month3)) 
				return;
		day_info->day_selected = day_selected;
		day_info->day_selected_y = 
			day_info->month3_y + (row+1)*row_h; 
		c->view->olddate = c->view->date;
		c->view->date = next_ndays(first_dom(day_info->month3),
			 day_info->day_selected);
	}
	day_info->day_selected_x = tmpx;
	sprintf(str, "%d", day_info->day_selected);
	x_off = gr_center(col_w, str, c->fonts->lucida14b);
	day_info->day_selected_x2 =
		day_info->day_selected_x+x_off;
	day_info->day_selected_y2 =
		day_info->day_selected_y + pfy;

}
extern void
monthbox_datetoxy(c)
	Calendar *c;
{
	char str[5];
	int week, x_off, dayw, daym, mo;
	Day *day_info = (Day *) c->view->day_info;
	int pfy   = xv_get(c->fonts->fixed12b, FONT_DEFAULT_CHAR_HEIGHT);
	int col_w = day_info->col_w;
	int row_h = day_info->row_h;
	struct tm tm;

	tm = *localtime(&c->view->date);
	mo = tm.tm_mon+1;
	dayw = tm.tm_wday;
	daym = tm.tm_mday;
	week = (12+tm.tm_mday-tm.tm_wday)/7;
	day_info->day_selected_x = c->view->outside_margin + 
			INSIDE_MARGIN + col_w*dayw;

	if (mo == month(day_info->month1)) 
		day_info->day_selected_y = 
			day_info->month1_y + 
			row_h*week;
	else if (mo == month(day_info->month2))  
		day_info->day_selected_y = 
			day_info->month2_y + 
			row_h*week;
	else if (mo == month(day_info->month3)) 
		day_info->day_selected_y = 
			day_info->month3_y + 
			row_h*week;
	sprintf(str, "%d", daym);
	x_off = gr_center(col_w, str, c->fonts->lucida14b);
	day_info->day_selected_x2 = 
			day_info->day_selected_x+x_off;
	day_info->day_selected_y2 = 
			day_info->day_selected_y + pfy;
	day_info->day_selected = daym;
}
extern Boolean
in_moboxes(c, x, y)
	Calendar *c;
	int x, y;
{
	int margin = c->view->outside_margin;
	int topoff = c->view->topoffset;
	Boolean in_mobox = false;
	Day *day_info = (Day *)c->view->day_info;
	int row_h = day_info->row_h;

	if (x < (MOBOX_AREA_WIDTH-margin-2*INSIDE_MARGIN) && 
	   	x > margin && y > topoff && 
		( (y < (day_info->month3_y+
			day_info->mobox_height3) &&
			y > (day_info->month3_y+row_h)) || 
		  (y < (day_info->month2_y+
                	day_info->mobox_height2) &&
			y > (day_info->month2_y+row_h)) ||
		  (y < (day_info->month1_y+
			day_info->mobox_height1) &&
			y > day_info->month1_y+row_h) ) )
		in_mobox = true;

	return in_mobox;
}
static void
paint_moboxes(c, rect)
        Calendar *c;
	Rect *rect;
{
        char str[3];
        int da_om, m_oy, moy, molength, i, x, y, pfy2, pfy;
	Day *day_info = (Day *)c->view->day_info;
	int col_w = day_info->col_w;
	int row_h = day_info->row_h;
	int mobox_width = day_info->mobox_width, x_off;
	Xv_Font f = c->fonts->lucida12;
	Xv_Font f2 = c->fonts->fixed12b;
	struct tm *tm;

	monthbox_deselect(c);
        pfy   = xv_get(f2, FONT_DEFAULT_CHAR_HEIGHT);
        pfy2   = xv_get(c->fonts->lucida12b, FONT_DEFAULT_CHAR_HEIGHT);
	tm = localtime(&c->view->date);
	da_om = tm->tm_mday;
	m_oy = tm->tm_mon+1;

	y = c->view->topoffset - pfy/2;
	paint_month_header(c, day_info->month1, y, mobox_width, rect);

        /* paint gray box */
	if (c->xcontext->screen_depth < 8)
        	gr_make_gray(c->xcontext, c->view->outside_margin, 
			day_info->month1_y, mobox_width, 
			day_info->mobox_height1, 25);
	else
        	gr_make_grayshade(c->xcontext, c->view->outside_margin, 
			day_info->month1_y, mobox_width, 
			day_info->mobox_height1, LIGHTGREY);
	/* outline the box */
	gr_draw_box(c->xcontext, c->view->outside_margin, 
			day_info->month1_y, mobox_width,
			 day_info->mobox_height1, rect);

	x = c->view->outside_margin + 2*INSIDE_MARGIN+2;
        y = day_info->month1_y + pfy2;
        /* paint day names */
	for (i = 0; i < 7; i++) {
        	gr_text(c->xcontext, x, y, f2, days3[i], rect);
		x += col_w;
	}
        x = c->view->outside_margin + INSIDE_MARGIN +
		 col_w * fdom(day_info->month1);
        y = day_info->month1_y + row_h + pfy;
	tm = localtime((Tick*)&(day_info->month1));
	molength = ((tm->tm_mon==1) && leapyr(tm->tm_year+TM_CENTURY))? 29 : 
		monthdays[tm->tm_mon];
	moy = tm->tm_mon+1;
        for (i = 1; i <= molength; i++) {
                sprintf(str, "%d", i);
		x_off = gr_center(col_w, str, f);
		gr_text(c->xcontext, x+x_off, y, f, str, rect);
		if (m_oy == moy &&  da_om == i) { 
			day_info->day_selected_x = x;
			day_info->day_selected_y = y - pfy;
			day_info->day_selected_x2 = x+x_off;
			day_info->day_selected_y2 = y;
			day_info->day_selected = i;
		}
		x += col_w;
                if (x > (mobox_width + c->view->outside_margin - 
				2*INSIDE_MARGIN)) {
                        y += row_h;
			x = c->view->outside_margin + INSIDE_MARGIN;
                }
        }

	
	y = day_info->month2_y - pfy/2;
	paint_month_header(c, day_info->month2, y, mobox_width, rect);

        /* paint gray box */
	if (c->xcontext->screen_depth < 8)
        	gr_make_gray(c->xcontext, c->view->outside_margin, 
			day_info->month2_y, mobox_width, 
			day_info->mobox_height2, 
			25);
	else
        	gr_make_grayshade(c->xcontext, c->view->outside_margin, 
			day_info->month2_y, mobox_width, 
			day_info->mobox_height2, 
			LIGHTGREY);
	/* outline the box */
	gr_draw_box(c->xcontext, c->view->outside_margin, 
			day_info->month2_y, mobox_width, 
			day_info->mobox_height2, rect);
 
	x = c->view->outside_margin + 2*INSIDE_MARGIN+2;
        y = day_info->month2_y + pfy2;
        /* paint day names */
	for (i = 0; i < 7; i++) {
        	gr_text(c->xcontext, x, y, f2, days3[i], rect);
		x += col_w;
	}
        
        x = c->view->outside_margin + INSIDE_MARGIN +
		 col_w * fdom(day_info->month2);
        y = day_info->month2_y + row_h + pfy;
	tm = localtime((Tick*)&(day_info->month2));
	molength = ((tm->tm_mon==1) && leapyr(tm->tm_year+TM_CENTURY))? 29 : 
			monthdays[tm->tm_mon];
	moy = tm->tm_mon+1;
        for (i = 1; i <= molength; i++) {
                sprintf(str, "%d", i);
		x_off = gr_center(col_w, str, f);
		gr_text(c->xcontext, x+x_off, y, f, str, rect);
		if (moy == m_oy && da_om == i) { 
			day_info->day_selected_x = x;
			day_info->day_selected_y = y - pfy;
			day_info->day_selected_x2 = x+x_off;
			day_info->day_selected_y2 = y;
			day_info->day_selected = i;
		}
		x += col_w;
                if (x > (mobox_width + c->view->outside_margin - 
				2*INSIDE_MARGIN)) {
                        y += row_h;
			x = c->view->outside_margin + INSIDE_MARGIN;
                }
        }

	/* paint third months box */
	y = day_info->month3_y - pfy/2;
	paint_month_header(c, day_info->month3, y, mobox_width, rect);

        /* paint gray box */
	if (c->xcontext->screen_depth < 8)
        	gr_make_gray(c->xcontext, c->view->outside_margin, 
			day_info->month3_y, 
			mobox_width, day_info->mobox_height3, 
			25);
	else
		gr_make_grayshade(c->xcontext, c->view->outside_margin,
                        day_info->month3_y,
                        mobox_width, day_info->mobox_height3,
                        LIGHTGREY);
        /* outline the box */
        gr_draw_box(c->xcontext, c->view->outside_margin, 
			day_info->month3_y, mobox_width, 
			day_info->mobox_height3, rect);

	x = c->view->outside_margin + 2*INSIDE_MARGIN+2;
        y = day_info->month3_y + pfy2;
        /* paint day names */
	for (i = 0; i < 7; i++) {
        	gr_text(c->xcontext, x, y, f2, days3[i], rect);
		x += col_w;
	}

	/* paint day numbers */
        x = c->view->outside_margin + INSIDE_MARGIN +
		 col_w * fdom(day_info->month3);
	y = day_info->month3_y + row_h + pfy;
	tm = localtime((Tick*)&(day_info->month3));
	molength = ((tm->tm_mon==1) && leapyr(tm->tm_year+TM_CENTURY))? 29 : 
			monthdays[tm->tm_mon];
	moy = tm->tm_mon+1;
        for (i = 1; i <= molength; i++) {
                sprintf(str, "%d", i);
		x_off = gr_center(col_w, str, f);
		gr_text(c->xcontext, x+x_off, y, f, str, rect);
		if (moy == m_oy && da_om == i) { 
			day_info->day_selected_x = x;
			day_info->day_selected_y = y - pfy;
			day_info->day_selected_x2 = x+x_off;
			day_info->day_selected_y2 = y;
			day_info->day_selected = i;
		}
		x += col_w;
                if (x > (mobox_width + c->view->outside_margin - 
				2*INSIDE_MARGIN)) {
                        y += row_h;
			x = c->view->outside_margin + INSIDE_MARGIN;
                }
        }
	monthbox_select(c);

}

extern Stat
paint_dayview(c, repaint, rect)
	Calendar *c;
	Boolean repaint;
	Rect *rect;
{
	Props *p = (Props*)c->properties;
	Selection *sel;
	Stat stat;
	int num_hrs;
	Abb_Appt *a = NULL;
	Range range;

	num_hrs = p->end_slider_VAL - p->begin_slider_VAL;
	range.key1 = lower_bound(p->begin_slider_VAL, c->view->date);
        range.key2 = next_nhours(range.key1, num_hrs+1);
        range.next = NULL;
        stat = table_abbrev_lookup_range(c->view->current_calendar, 
					&range, &a);
	if (stat == status_param && c->general->version <= CMS_VERS_3)
		return stat;
	/* repaint all */
	if (repaint) {
		gr_clear_area(c->xcontext, 0, 0, 
			c->view->winw, c->view->winh);
		/* draw line separating mo. boxes and appts. */
        	gr_draw_line(c->xcontext, (int)MOBOX_AREA_WIDTH+1,
			 0, (int)MOBOX_AREA_WIDTH+1,
                         c->view->winh, gr_solid, rect);
        	gr_draw_line(c->xcontext, (int)MOBOX_AREA_WIDTH+2, 
			 0, (int)MOBOX_AREA_WIDTH+2,
                         c->view->winh, gr_solid, rect);
        	gr_draw_line(c->xcontext, (int)MOBOX_AREA_WIDTH+2, 
			 c->view->topoffset-1, c->view->winw,
                         c->view->topoffset-1,  gr_solid, rect);
        	gr_draw_line(c->xcontext, (int)MOBOX_AREA_WIDTH+2, 
			 c->view->topoffset, c->view->winw,
                         c->view->topoffset, gr_solid, rect);
		paint_moboxes(c, rect);
		paint_dayview_appts(c, a, rect);
	}
	/* just repaint schedule area */
	else {
		gr_clear_area(c->xcontext, 
				(int)MOBOX_AREA_WIDTH+4, 0, 
				c->view->winw - (int)MOBOX_AREA_WIDTH+4,
				c->view->winh);
        	gr_draw_line(c->xcontext, (int)MOBOX_AREA_WIDTH+2, 
			 c->view->topoffset, c->view->winw,
                         c->view->topoffset,  gr_solid, rect);
        	gr_draw_line(c->xcontext, (int)MOBOX_AREA_WIDTH+2, 
			 c->view->topoffset+1, c->view->winw,
                         c->view->topoffset+1, gr_solid, rect);
		paint_dayview_appts(c, a, rect);
	}
	paint_day_header(c, c->view->date, rect);
	return stat;
}
extern void
paint_day(c)
	Calendar *c;
{
        if (c->view->glance==dayGlance) 
		return;
        c->view->glance=dayGlance;

	if ( strcmp("C", (char *)xv_get(calendar->frame, XV_LC_DISPLAY_LANG)) ) {
		xv_set(calendar->items->button8, PANEL_LABEL_STRING, LGET("Yesterday"), NULL);
		xv_set(calendar->items->button9, PANEL_LABEL_STRING, LGET("Today"), NULL);
		xv_set(calendar->items->button10, PANEL_LABEL_STRING, LGET("Tomorrow"), NULL);
	}
	
	init_mo(c);
	(void)init_dayview(c);
	gr_clear_area(c->xcontext, 0, 0, c->view->winw, c->view->winh);
	paint_dayview(c, true, NULL); 
	calendar_select(c, hourSelect, (caddr_t)NULL);
}
	
/* ARGSUSED */
extern Notify_value
day_button (m, mi)
	Menu m;
	Menu_item mi;
{
	paint_day(xv_get(m, MENU_CLIENT_DATA));

	return NOTIFY_DONE;
}

extern void
print_day_pages(c)
        Calendar *c;
{

	Props *p = (Props *)c->properties;
	int n = atoi(p->repeatVAL);
	register Tick first_date = c->view->date;
        Boolean done = FALSE, first = TRUE, print_day();
        int num_page = 1, lines_per_page;
	Props *pr = (Props*)c->properties;
        FILE *fp;

	if (n <= 0) n = 1;

	lines_per_page = get_lines_per_page(pr);

        if ((fp=ps_open_file()) == NULL)
                return;

	for (; n > 0; n--) {
        	while (!done) {
                	done = print_day(c, num_page, lines_per_page, fp, first_date, p, first);
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

extern Boolean
print_day(c, num_page, lines_per_page, fp, first_date, p, first)
    	Calendar *c;
	int num_page, lines_per_page;
	FILE	*fp;
	Tick first_date;
	Props *p;
	Boolean first;
{
	int	n, i, timeslots, num_appts; 
	static int total_pages;
	Tick	lo_hour, hi_hour;
	char	buf[100];
	int       daybegin = p->begin_slider_VAL;
	int       dayend   = p->end_slider_VAL;
        Boolean more, done = FALSE, all_done = TRUE, ps_print_multi_appts();
	Range	range;
	Abb_Appt *a, *temp_a;
	static Tick tick = 0;
 
       if (first)
                tick = first_date;
        if (num_page > 1)
                tick = prevday(tick);
        else
                total_pages = count_day_pages(c, lines_per_page, tick);

	ps_init_printer(fp, PORTRAIT);
	ps_init_day(fp);

	/*
	 * Need to find the max number of timeslots which will be shown
	 * in one column, for later calculation of box height.
	 */  
	if ((!morning(daybegin)) || dayend <= 12) 
		timeslots = dayend - daybegin;
	else
		timeslots = ((12-daybegin) > (dayend-12)) ?
			     (12-daybegin) : (dayend-12);
	 
	format_date(tick, p->ordering_VAL, buf, 1);
	ps_print_header(fp, buf);        /* print date at top */
	ps_day_header(fp, timeslots, num_page, total_pages);    /* print morning/afternoon boxes */
	 
	for (i=daybegin; i < dayend; i++) {
		lo_hour = lower_bound(i, tick);
		hi_hour = next_nhours(lo_hour+1, 1);

		range.key1 = lo_hour;
		range.key2 = hi_hour;
		range.next = NULL;
		table_abbrev_lookup_range(calendar->view->current_calendar,
                	&range, &a);
 
		num_appts = count_multi_appts(a, c);
		if (num_appts > (lines_per_page * num_page))
			more = TRUE;
		else
			more = FALSE;

		ps_day_timeslots (fp, i, more);    /* print hourly boxes */
		temp_a = a;
		done = ps_print_multi_appts (fp, a, num_page, hi_hour, dayGlance);

		if (!done)
			all_done = FALSE;
		destroy_abbrev_appt(temp_a);
	}
 
	ps_finish_printer(fp);	/* print page and set for next one */
	tick = nextday(tick); 

	return(all_done);
}

static int
count_day_pages(c, lines_per_page, tick)
    	Calendar *c;
	int lines_per_page;
	Tick tick;
{
	int	n, i, timeslots, num_appts, pages, max = 0; 
	Tick	lo_hour, hi_hour;
	Props *p = (Props *)c->properties;
	int       daybegin = p->begin_slider_VAL;
	int       dayend   = p->end_slider_VAL;
	Range	range;
	Abb_Appt *a, *temp_a;
 
	for (i=daybegin; i < dayend; i++) {
		lo_hour = lower_bound(i, tick);
		hi_hour = next_nhours(lo_hour+1, 1);

		range.key1 = lo_hour;
		range.key2 = hi_hour;
		range.next = NULL;
		table_abbrev_lookup_range(calendar->view->current_calendar,
			&range, &a);
 
		num_appts = count_multi_appts(a, c);
		if (num_appts > max)
			max = num_appts;

		temp_a = a;
		destroy_abbrev_appt(temp_a);
	}
 
       	pages = max / lines_per_page;
        if ((max % lines_per_page) > 0)
                pages++;
 
        return(pages);
}

extern int 
get_lines_per_page(pr)
	Props *pr;
{
	int lines, hours;

	hours = pr->end_slider_VAL - pr->begin_slider_VAL;


	if (hours > 13)
		lines = 4;
	else if (hours > 10)
		lines = 9;
	else if (hours > 4)
		lines = 11;
	else if (hours > 2)
		lines = 13;
	else if (hours > 1)
		lines = 20;
	else
		lines = 40;

	return(lines);
}

/* ARGSUSED */
extern Notify_value
ps_day_button (m, mi)
    Menu m;
    Menu_item mi;
{
    	Calendar *c = (Calendar *)xv_get (m, MENU_CLIENT_DATA, 0);

    	print_day_pages(c);

	return NOTIFY_DONE;
}
static int
day_xytoclock(c, x, y, t)
	Calendar *c;
	int x, y;
	Tick t;
{
	int daybegin, hr, val;
	char buf[10];
	struct tm tm;
	Props *p;

	p	= (Props *)c->properties;
        daybegin= p->begin_slider_VAL; 
	tm	= *localtime(&t);

	hr = (x == 1) ? (12 + y) : (y + daybegin);

	(void)sprintf(buf, "%d/%d/%02d", tm.tm_mon+1, tm.tm_mday, tm.tm_year % 100);
	val	=cm_getdate(buf, NULL);
	val	= val+(hr*(int)hrsec);
	adjust_dst(t, val);
	return(val);
}

extern int
day_event(c, event)
    Calendar *c;
    Event *event;
{
	static int lastcol, lastrow;
        static Event lastevent;
	struct pr_pos xy;
        int x, y, i, j;
        int boxw, boxh, margin, id;
        Tick date = c->view->date;
	Boolean in_mbox = false; /* in month boxes ? */
	Editor *e = (Editor*)c->editor;
	Day *day_info = (Day *)c->view->day_info;

        boxw    = c->view->boxw;
        boxh    = c->view->boxh;
        margin  = c->view->outside_margin;
        x	= event_x(event);
        y	= event_y(event);
        id      = event_id(event);


	/* boundary conditions */
	if ((!(in_mbox = in_moboxes(c, x, y)) && x < MOBOX_AREA_WIDTH+2) ||
		(x > MOBOX_AREA_WIDTH+2 && y < c->view->topoffset)) {
		lastcol=0; lastrow=0;
		return;
	}
	if (in_mbox) {
        	xy.x = (x - margin)/ day_info->col_w;
        	xy.y = (x - c->view->topoffset)/ day_info->row_h;
	}
	else {
		xy.x	= boxw;
		xy.y	= (y - c->view->topoffset)/boxh;
	}

        switch(id) {
        case LOC_DRAG:
		if (!in_mbox) {
                	if (xy.x !=lastcol || xy.y !=lastrow) {
				calendar_deselect(c);
                        	j = day_xytoclock(c, xy.x, xy.y, date);
                        	if (j >= 0) {
					c->view->olddate = c->view->date;
					c->view->date = j;
                                	calendar_select(c, hourSelect, (caddr_t)&xy);
                       		}
				lastcol=xy.x;
                        	lastrow=xy.y;
                	}
		}
                break;
        case MS_LEFT:
                if (event_is_down(event)) {
			if (ds_is_double_click(&lastevent, event)) {
				if (lastcol == xy.x && lastrow == xy.y) {
        				if (cal_update_props())
                				for (i=0; i < NO_OF_PANES; i++)
                        				set_rc_vals((Props*)calendar->properties, i);
					set_defaults(e);
					show_editor(c);
				}
				if (in_mbox) {
					set_default_calbox(c);
					monthbox_deselect(c);
					monthbox_xytodate(c, x, y);
					monthbox_select(c);

					/* Added to fix 1203437 - DPT 6/Nov/96 */
					reset_date_appts(c);

				}
				else {
					j = day_xytoclock(c, xy.x, xy.y, date);
					if (editor_showing(e)) {
						set_default_calbox(c);
						reset_time_date_appts(c, hour(j));
					}
				}
			}
			else {
				if (in_mbox) {
					monthbox_deselect(c);
					monthbox_xytodate(c, x, y);	
        				paint_dayview(c, false, NULL);
                			calendar_select(c, hourSelect,
							(caddr_t)NULL);
					monthbox_select(c);
					if (editor_showing(e)) {
						set_default_calbox(c);
						reset_date_appts(c);
					}
				}
				else {
					calendar_deselect(c);
					j = day_xytoclock(c, xy.x, xy.y, date);
					if (j >= 0) {
						c->view->olddate = c->view->date;
                                       		c->view->date = j;
                                       		calendar_select(c, hourSelect, (caddr_t)&xy);
						if (editor_showing(e)) {
							set_default_calbox(c);
							reset_time_date_appts(c, hour(j));
						}
				
					}
				}
			}
			lastcol=xy.x;
			lastrow=xy.y;
                }
                break;
	default:
        break;
        };              /* switch */
	lastevent = *event;
 
}

