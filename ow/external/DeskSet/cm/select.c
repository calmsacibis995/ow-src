#ifndef lint
static  char sccsid[] = "@(#)select.c 3.9 93/05/03 Copyr 1991 Sun Microsystems, Inc.";
#endif
/* select.c */

#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/scrollbar.h>
#include <xview/font.h>
#include <xview/cms.h>
#include "util.h"
#include "select.h"
#include "graphics.h"
#include "calendar.h"
#include "timeops.h"
#include "browser.h"
#include "appt.h"
#include "alist.h"
#include "todo.h"
#include "table.h"
#include "dayglance.h"
#include "weekglance.h"
#include "yearglance.h"
#include "datefield.h"
#include "props.h"

static void select_weekday();
static void select_weekhotbox();

extern int
selection_active (s)
	Selection *s;
{
	return (s->active);
}

extern void
deactivate_selection (s)
	Selection *s;
{
	s->active=0;
}

extern void
activate_selection (s)
	Selection *s;
{
	s->active=1;
}


/*	selection service for all views.  ref is a client_data
	field which is cast depending on the selection unit.  if
	it's a daySelect, ref contains the number of weeks in the
	month.  if it's a monthSelect, ref contains a point to x,y
	coordinates.  ref is NULL on a weekSelect.		*/

extern void
calendar_select (c, unit, ref)
	Calendar *c; Selection_unit unit; caddr_t ref;
{
	int i, j;
	int boxw, boxh, margin, topoff, date;
	Selection *sel;
	XContext *xc;
	Cms cms;

	date	= c->view->date;
	sel	= (Selection *) c->view->current_selection;
	boxw	= c->view->boxw;
	boxh	= c->view->boxh;
	margin	= c->view->outside_margin;
	topoff	= c->view->topoffset;
	xc	= c->xcontext;

	switch (unit) {
		/* day selection on month glance */
		case daySelect:
			i=dow(date);
			j=wom(date);
			if (j >= 0) {
				sel->row=j-1;
				sel->col=i;
				sel->nunits=1;
				if (c->xcontext->screen_depth < 8) 
					gr_draw_box(xc, (i*boxw)+margin+
					2, (sel->row*boxh)+topoff+
					margin+2, boxw-4, boxh-4, NULL);
				else {
					cms = xv_get(c->panel, WIN_CMS);
					gr_draw_rgb_box(xc, (i*boxw)
						+margin+2, 
						(sel->row*boxh)+topoff+
                                        	margin+2, boxw-4, 
						boxh-4, CMS_BACKGROUND_PIXEL,
						cms);
					gr_draw_rgb_box(xc, (i*boxw)
						+margin+1, 
						(sel->row*boxh)+topoff+
                                        	margin+1, boxw-2, 
						boxh-2, CMS_BACKGROUND_PIXEL,
						cms);
				}
				activate_selection (sel);

			}
			break;
		/* week selection on month glance view */
		case weekSelect:
			i=0; j=7;
			sel->row=(int)ref;
			sel->col=i;
			sel->nunits=j-i;
			cms = xv_get(c->panel, WIN_CMS);
			while (i < j) {
				if (c->xcontext->screen_depth < 8) 
					gr_draw_box(xc, i*boxw+margin+2,
					(int)ref*boxh+margin+topoff+2, 
					boxw-4, boxh-4, NULL);
				else {
					gr_draw_rgb_box(xc, i*boxw+margin+2,
					(int)ref*boxh+margin+topoff+2, 
					boxw-4, boxh-4,
					CMS_BACKGROUND_PIXEL, cms);
					gr_draw_rgb_box(xc, i*boxw+margin+1,
					(int)ref*boxh+margin+topoff+1, 
					boxw-2, boxh-2,
					CMS_BACKGROUND_PIXEL, cms);
				}
				i++;
			}
			activate_selection (sel);
			break;
		/* month selection on year glance view */
		case monthSelect:
			if (ref != NULL) {
				i=((struct pr_pos *)ref)->x;
				j=((struct pr_pos *)ref)->y;
			}
			else {
				i = sel->col;
				j = sel->row;
			}
			if (c->xcontext->screen_depth < 8) 
				gr_draw_box(xc, i*boxw+margin+10,
					j*boxh+topoff-4, boxw-4, boxh-4, NULL);
			else {
				cms = xv_get(c->panel, WIN_CMS);
				gr_draw_rgb_box(xc, i*boxw+margin+10,
					j*boxh+topoff-5, boxw-2, boxh-2, 
					CMS_BACKGROUND_PIXEL, cms);
				gr_draw_rgb_box(xc, i*boxw+margin+11,
					j*boxh+topoff-4, boxw-2, boxh-2, 
					CMS_BACKGROUND_PIXEL, cms);
			}
			sel->row=j;
			sel->col=i;
			sel->nunits=1;
			activate_selection (sel);
			break;
		/* hour box selection day glance view */
		case hourSelect:
			if (ref != NULL)
				j=((struct pr_pos *)ref)->y;
			else
				j = sel->row;
			if (c->xcontext->screen_depth < 8) 
				gr_draw_box(xc, MOBOX_AREA_WIDTH+4,
					j*boxh+topoff+2, c->view->boxw-6,
					c->view->boxh-4, NULL);
			else {
				cms = xv_get(c->panel, WIN_CMS);
				gr_draw_rgb_box(xc, MOBOX_AREA_WIDTH+4,
					j*boxh+topoff+2, 
					c->view->boxw-6,
					c->view->boxh-4,
					CMS_BACKGROUND_PIXEL, cms);
				gr_draw_rgb_box(xc, MOBOX_AREA_WIDTH+3,
					j*boxh+topoff+1, 
					c->view->boxw-4,
					c->view->boxh-2,
					CMS_BACKGROUND_PIXEL, cms);
			}
			sel->row=j;
			sel->col=MOBOX_AREA_WIDTH+4;
			sel->nunits=1;
			activate_selection(sel);
			break;
		/* day selection on week glance */
		case weekdaySelect:
			select_weekday(c, true);
			activate_selection (sel);
			break;
		/* hotbox selection on week glance */
		case weekhotboxSelect:
			select_weekhotbox(c);
			activate_selection (sel);
			break;
		default:
			break;
	}
	common_update_lists(c);
}
	
extern void
calendar_deselect (c)
	Calendar *c;
{
	int i, j, k;
	int boxh	= c->view->boxh;
	int boxw	= c->view->boxw;
	int margin	= c->view->outside_margin;
	int topoff	= c->view->topoffset;
	Selection *s	= (Selection *) c->view->current_selection;
	int nunits	= s->nunits;
	XContext *xc	= c->xcontext;

	if (selection_active(s)) {
		j=s->col;
		k=s->row;
		switch(c->view->glance) {
		case weekGlance:
			select_weekday(c, false);
			deactivate_selection (s);
			break;
		case dayGlance:
			for (i=0; i < nunits; i++) {
				gr_dissolve_box(xc, MOBOX_AREA_WIDTH+4,
					 k*boxh+topoff+2, boxw-6, boxh-4);
				gr_dissolve_box(xc, MOBOX_AREA_WIDTH+3,
					 k*boxh+topoff+1, boxw-4, boxh-2);
			}
			/* draw vertical line */
        		gr_draw_line(xc, MOBOX_AREA_WIDTH+2+HRBOX_MARGIN,
			c->view->topoffset, MOBOX_AREA_WIDTH+2+HRBOX_MARGIN,
			c->view->winh, gr_solid, NULL);
			break;
		case monthGlance:
			for (i=0; i<nunits; i++) {
				gr_dissolve_box(xc, (j*boxw)+margin+2,
					k*boxh+topoff+margin+2,
					boxw-4, boxh-4);
				gr_dissolve_box(xc, (j*boxw)+margin+1,
					k*boxh+topoff+margin+1,
					boxw-2, boxh-2);
				j++;
			}
			break;
		case yearGlance:
			for (i=0; i<nunits; i++) {
				gr_dissolve_box(xc, (j*boxw)+margin+10,
					k*boxh+topoff-5,
					boxw-2, boxh-2);
				gr_dissolve_box(xc, (j*boxw)+margin+11,
					k*boxh+topoff-4,
					boxw-2, boxh-2);
				j++;
			}
			break;
		}
		deactivate_selection (s);
	}
}
extern void
monthbox_deselect(c)
	Calendar *c;
{
	char buf[3];
	Day *day_info = (Day *)c->view->day_info;
	int x, y;

	if (day_info->day_selected == -1)
		return;

	if ( strcmp("C", (char *)xv_get(calendar->frame, XV_LC_DISPLAY_LANG)) ) {
		x = day_info->day_selected_x + 2;
		y = day_info->day_selected_y + 2;
	} else {     /* In C locale */
		x = day_info->day_selected_x;
		y = day_info->day_selected_y;
	}

	if (c->xcontext->screen_depth < 8)
		gr_make_gray(c->xcontext, 
			x,
			y,
			day_info->col_w+1,
			day_info->row_h+1, 25);
	else
		gr_make_grayshade(c->xcontext, 
			x,
			y,
			day_info->col_w+1,
			day_info->row_h+1, LIGHTGREY);
	buf [0] = '\0';
	sprintf(buf, "%d", day_info->day_selected);
	gr_text(c->xcontext, day_info->day_selected_x2+2, 
			day_info->day_selected_y2, 
			c->fonts->lucida12, buf, NULL);
}
extern void
monthbox_select(c)
	Calendar *c;
{
	char buf[3];
	Day *day_info = (Day *)c->view->day_info;
	int x, y;

	if ( strcmp("C", (char *)xv_get(calendar->frame, XV_LC_DISPLAY_LANG)) ) {
		x = day_info->day_selected_x + 2;
		y = day_info->day_selected_y + 2;
	} else {     /* In C locale */
		x = day_info->day_selected_x;
		y = day_info->day_selected_y;
	}

	gr_clear_box(c->xcontext, 
			x,
			y,
			day_info->col_w,
			day_info->row_h);
	gr_draw_box(c->xcontext,
			x,
			y,
			day_info->col_w,
			day_info->row_h, NULL);
	buf [0] = '\0';
	sprintf(buf, "%d", day_info->day_selected);
	gr_text(c->xcontext, day_info->day_selected_x2+2, 
			day_info->day_selected_y2, 
			c->fonts->lucida12, buf, NULL);
}

extern void
browser_deselect (b)
	Browser *b;
{
	int h, i, j, k, index, w, x, y;
	Selection *sel	= (Selection *) b->current_selection;
	Calendar *c = (Calendar*)xv_get(b->frame, WIN_CLIENT_DATA);

	if (!selection_active (sel))
		return;
	j = sel->col;
	k = sel->row;
	x =  (j*b->boxw) +  b->chart_x + 1;
	y =  (k*b->boxh) + b->chart_y + 1;
	h =  (b->boxh/BOX_SEG);
	w =  b->boxw-2;
	gr_clear_area(b->xcontext, x, y, b->boxw-1, b->boxh-1);
	index = j * (b->segs_in_array / 7) + (BOX_SEG * k);
	for (i = index;  i < (index + BOX_SEG); i++) {
		if (b->multi_array[i] == 1) {
			if (c->xcontext->screen_depth < 8) 
				gr_make_gray(b->xcontext, x, y, w+1, 
							h, 25);
			else 
				gr_make_grayshade(b->xcontext, x, y, 
					w+1, h, LIGHTGREY);
		}
		else if (b->multi_array[i] == 2) {
			if (c->xcontext->screen_depth < 8) 
				gr_make_gray(b->xcontext, x, y, 
					w+1, h, 50);
			else
				gr_make_rgbcolor(b->xcontext, x, y, 
					w+1, h, MIDGREY, MIDGREY, MIDGREY);
		}
		else if (b->multi_array[i] >= 3) {
			if (c->xcontext->screen_depth < 8) 
				gr_make_gray(b->xcontext, x, y, 
					w+1, h, 75);
			else 
				gr_make_grayshade(b->xcontext, x, y, 
					w+1, h, DIMGREY);
		}
		/* compensate for grid line pixel */ 
		if (i == (index+BOX_SEG-2))
			y += (h-1);
		else
			y += h;
	}
	deactivate_selection (sel);
}


extern void
browser_select(b, xy)
	Browser *b;
	struct pr_pos *xy;
{
	int i, j;
	Selection *sel	= (Selection *) b->current_selection;
	Calendar *c;
	Cms cms;

	if (xy != NULL) {
		i = xy->x;
		j = xy->y;
	}
	else {
		i = sel->col;
		j = sel->row;
	}
	if (j >= 0) {
		c = (Calendar*)xv_get(b->frame, WIN_CLIENT_DATA);
		if (c->xcontext->screen_depth < 8) {
			gr_draw_box(b->xcontext, (i * b->boxw) + 
				b->chart_x + 2, (j * b->boxh) + b->chart_y 
				+ 2, b->boxw - 4, b->boxh - 4, NULL);
		}
		else {
			cms = xv_get(c->panel, WIN_CMS);
			gr_draw_rgb_box(b->xcontext, (i * b->boxw) + 
				b->chart_x + 2, (j * b->boxh) + b->chart_y 
				+ 2, b->boxw - 4, b->boxh - 4, 
				CMS_BACKGROUND_PIXEL, cms);
			gr_draw_rgb_box(b->xcontext, (i * b->boxw) + 
				b->chart_x + 1, (j * b->boxh) + b->chart_y 
				+ 1, b->boxw - 2, b->boxh - 2, 
				CMS_BACKGROUND_PIXEL, cms);
		}
		sel->row = j;
		sel->col = i;
		sel->nunits = 1;
		activate_selection(sel);
	}
}
extern void
weekchart_deselect (c, n)
	Calendar *c;
	int n;
{
	int h, i, j, k, index, wi, x, y, chart_x, chart_y;
	Week *w = (Week *)c->view->week_info;
	Selection *sel	= (Selection *) w->current_selection;
        int     char_height = xv_get(w->font, FONT_DEFAULT_CHAR_HEIGHT);

	if (!selection_active (sel)) return;

	j = sel->col;
	k = sel->row;

	/* dissolves box around weekday letter over chart */
	chart_x = w->chart_x + (j * w->chart_day_width);
	chart_y = w->chart_y - char_height - 4;
	gr_dissolve_box(c->xcontext, chart_x, chart_y, w->chart_day_width, 
			char_height+1);
	gr_dissolve_box(c->xcontext, chart_x+1, chart_y-1, w->chart_day_width-2, 
			char_height+3);

	x =  (j*w->chart_day_width) +  w->chart_x + 1;
	y =  (k * (w->chart_hour_height + w->add_pixels)) +  w->chart_y + 1;
	h =  (w->chart_hour_height/BOX_SEG);
	wi =  w->chart_day_width-2;

	gr_clear_area(c->xcontext, x, y, w->chart_day_width-1, 
			w->chart_hour_height-1 + w->add_pixels);
	index = j * (w->segs_in_array / 7) + (BOX_SEG * k);
	for (i = index;  i < (index + BOX_SEG); i++) {
		/* compensate for the added pixel for displaying chart */
		if ((i+1) == (index + BOX_SEG)) h += w->add_pixels;
		if (w->time_array[i] == 1) {
			if (c->xcontext->screen_depth < 8) 
				gr_make_gray(c->xcontext, x, y, wi+1, h, 25);
			else 
				gr_make_grayshade(c->xcontext, x, y, wi+1, h, 
						LIGHTGREY);
		}
		else if (w->time_array[i] == 2) {
			if (c->xcontext->screen_depth < 8) 
				gr_make_gray(c->xcontext, x, y, 
					wi+1, h, 50);
			else
				gr_make_rgbcolor(c->xcontext, x, y, wi+1, h, 
						MIDGREY, MIDGREY, MIDGREY);
		}
		else if (w->time_array[i] >= 3) {
			if (c->xcontext->screen_depth < 8) 
				gr_make_gray(c->xcontext, x, y, wi+1, h, 75);
			else 
				gr_make_grayshade(c->xcontext, x, y, wi+1, h, 
						DIMGREY);
		}
		/* compensate for grid line pixel */ 
		if (i == (index+BOX_SEG-2))
			y += (h-1);
		else
			y += h;
	}
	deactivate_selection (sel);
}

/* selects day in chart */
extern void
weekchart_select(c)
	Calendar *c;
{
	int i, j, chart_x, chart_y;
	Week *w = (Week *)c->view->week_info;
	Selection *sel	= (Selection *) w->current_selection;
	Cms cms;
        int     char_height = xv_get(w->font, FONT_DEFAULT_CHAR_HEIGHT);

	i = sel->col;
	j = sel->row;
	if (j >= 0) {
        	chart_x = w->chart_x + (i * w->chart_day_width);
        	chart_y = w->chart_y - char_height - 4;
		if (c->xcontext->screen_depth < 8) {
			gr_draw_box(c->xcontext, (i * w->chart_day_width) +
			 	w->chart_x + 2, (j * w->chart_hour_height) + 
				(j * w->add_pixels) + w->chart_y + 2, 
				w->chart_day_width - 4, 
				w->chart_hour_height - 4 + w->add_pixels, NULL);
			gr_draw_box(c->xcontext, chart_x, chart_y, 
				w->chart_day_width, char_height + 1, NULL);
		}
		else  { 
			cms = xv_get(c->panel, WIN_CMS);
			gr_draw_rgb_box(c->xcontext, (i * w->chart_day_width)
				+ w->chart_x + 2,(j * w->chart_hour_height) + 
				(j * w->add_pixels) + w->chart_y + 2, 
				w->chart_day_width - 4, w->chart_hour_height - 4
				+ w->add_pixels, CMS_BACKGROUND_PIXEL, cms);
			gr_draw_rgb_box(c->xcontext, (i * w->chart_day_width)
				+ w->chart_x + 1, (j * w->chart_hour_height) 
				+ (j * w->add_pixels) + w->chart_y + 1, 
				w->chart_day_width - 2, w->chart_hour_height - 2
				+ w->add_pixels, CMS_BACKGROUND_PIXEL, cms);
			gr_draw_rgb_box(c->xcontext, chart_x, chart_y,
                                w->chart_day_width, char_height + 1, 
				CMS_BACKGROUND_PIXEL, cms);
			gr_draw_rgb_box(c->xcontext, chart_x+1, chart_y-1,
                                w->chart_day_width-2, char_height+3, 
				CMS_BACKGROUND_PIXEL, cms);
		}
		sel->nunits = 1;
		activate_selection(sel);
	}
}
/* selects day is main boxes: not chart */
static void
select_weekday(c, select)
        Calendar *c;
        Boolean select;
{
        int     n, x, y;
        XContext*xc     = c->xcontext;
	Week 	*w = (Week *)c->view->week_info;
	Cms 	cms;

        /* Draw selection feedback on week view */
        (n = dow(c->view->date)) == 0 ? n = 6 : n--;

        if (n < 5) {
                x = w->x + n * w->day_width + 2;
                y = w->y + w->label_height + 2;
        } else {
                n -= (5 - 3);
                x = w->x + n * w->day_width + 2;
                y = w->y + w->day_height + w->label_height + 2;
        }
 
        if (select) {
                if (c->xcontext->screen_depth < 8) {
                        gr_draw_box(xc, x, y, w->day_width - 4,
                        w->day_height - w->label_height - 4, NULL);
		}
                else {
			cms = xv_get(c->panel, WIN_CMS);
                        gr_draw_rgb_box(xc, x, y, w->day_width - 4,
                        		w->day_height - w->label_height - 4,
				 	CMS_BACKGROUND_PIXEL, cms);
                        gr_draw_rgb_box(xc, x-1, y-1, w->day_width-2,
                        		w->day_height - w->label_height-2,
				 	CMS_BACKGROUND_PIXEL, cms);
		}
                weekchart_select(c);
        } else {
                gr_dissolve_box(xc, x, y, w->day_width - 4, 
				w->day_height - w->label_height - 4);
                gr_dissolve_box(xc, x-1, y-1, w->day_width-2,
				 w->day_height - w->label_height - 2);
                weekchart_deselect(c);
        }
 
}

static void
select_weekhotbox(c)
        Calendar *c;
{
        int     n, x, y;
        XContext*xc     = c->xcontext;
	Week *w = (Week *)c->view->week_info;
        long    date    = c->view->date;
	Cms 	cms;

        /* Draw selection feedback on week view */
        if ((n = dow(date)) == 0)
                n = 6;
        else
                n--;

        if (n < 5) {
                x = w->x + n * w->day_width + 2;
                y = w->y + 2;
        }
	else {
                n -= (5 - 3);
                x = w->x + n * w->day_width + 2;
                y = w->y + w->day_height + 2;
        }
 
	if (c->xcontext->screen_depth < 8)
		gr_draw_box(xc, x, y, w->day_width - 4, w->label_height - 4, NULL);
	else {
		cms = xv_get(c->panel, WIN_CMS);
		gr_draw_rgb_box(xc, x, y, w->day_width - 4, w->label_height - 4,
				CMS_BACKGROUND_PIXEL, cms);
		gr_draw_rgb_box(xc, x-1, y-1, w->day_width-2, w->label_height - 2,
				CMS_BACKGROUND_PIXEL, cms);
	}
}

extern void
paint_selection(c)
        Calendar *c;
{
        Props  *p = (Props*)c->properties;
        Selection *sel;
        int d, mo;
        Week *w = (Week *)c->view->week_info;
        struct pr_pos xy;

        switch ((Glance)c->view->glance) {
                case monthGlance:
                        calendar_select(c, daySelect, (caddr_t)NULL);
                        break;
                case dayGlance:
                        sel = (Selection *) c->view->current_selection;
                        sel->row = hour(c->view->date) - p->begin_slider_VAL;
                        calendar_select(c, hourSelect, (caddr_t)NULL);
                        break;
                case weekGlance:
                        sel = (Selection*)w->current_selection;
                        sel->col =  (d = dow(c->view->date)) == 0 ? 6 : --d;                        	calendar_select(c, weekdaySelect, (caddr_t)NULL);
                        break;
                case yearGlance:
                        mo = month(c->view->date);
                        xy.y = month_row_col[mo-1][ROW];
                        xy.x = month_row_col[mo-1][COL];
                        calendar_select(c, monthSelect, (caddr_t)&xy);
                        break;
        }
}
