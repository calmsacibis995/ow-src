#ifndef lint
static  char sccsid[] = "@(#)yearglance.c 3.18 93/08/30 Copyr 1991 Sun Microsystems, Inc.";
#endif
/* yearglance.c */

#include <stdio.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/textsw.h>
#include <xview/scrollbar.h>
#include <xview/font.h>
#include "util.h"
#include "select.h"
#include "graphics.h"
#include "timeops.h"
#include "calendar.h"
#include "ps_graphics.h"
#include "datefield.h"
#include "props.h"
#include "appt.h"
#include "table.h"
#include "yearglance.h"
#include "editor.h"
#include "gettext.h"

extern Notify_value month_button();
extern int prolog_found;

static int init_month_strings;

static char *months[13] = {"",
	(char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL
	};

static char *months_alt[13] = {"",
	(char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL
	};

static void
init_month_strs()
{
	init_month_strings = 1;

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

	/* STRING_EXTRACTION SUNW_DESKSET_CM_MSG :
	 *
	 * The following are abbreviations for each month of the year starting
	 * with JAN = January, FEB = February...DEC = December. 
	 */
	months_alt[1] = MGET("JAN");
	months_alt[2] = MGET("FEB");
	months_alt[3] = MGET("MAR");
	months_alt[4] = MGET("APR");
	months_alt[5] = MGET("MAY");
	months_alt[6] = MGET("JUN");
	months_alt[7] = MGET("JUL");
	months_alt[8] = MGET("AUG");
	months_alt[9] = MGET("SEP");
	months_alt[10] = MGET("OCT");
	months_alt[11] = MGET("NOV");
	months_alt[12] = MGET("DEC");
}

extern void 
pad (buf)
	char *buf;
{
	int i;
	int l = cm_strlen (buf);
	i = l? l+1: 0;			/* bizarre */
	while (i < 21) {
		buf[i]=' ';
		i++;
	}
	buf[21]=NULL;
}

static int
xytomonth (x, y, date)
	int x, y, date;
{
	struct tm tm;
	tm = *localtime ((time_t*)&date);
	tm.tm_mon= x+(3*y);
	tm.tm_mday=1;
#ifdef SVR4
	tm.tm_isdst = -1;
	return (mktime(&tm));
#else
	return (timelocal(&tm));
#endif "SVR4"
}

extern int
year_event (c, event)
	Calendar *c;
	Event *event;
{
	static Event lastevent;
	static int x, y, lastx, lasty;
	struct pr_pos xy;
	int id;
	Menu m;
	Editor *e = (Editor*)c->editor;

	int date	= c->view->date;
	int margin	= c->view->outside_margin;
	int boxw	= c->view->boxw;
	int boxh	= c->view->boxh;
	x		= event_x(event);
	y		= event_y(event);
	xy.x		= (x-margin)/(boxw);
	xy.y		= (y-c->view->topoffset)/(boxh);

	/* boundary conditions */
	if ((x < margin || xy.x > 2) || xy.y > 3) {
		calendar_deselect(c);
		lastx=0;
		lasty=0;
		return;
	}

	id = event_id (event);
	switch (id) {
	case LOC_DRAG:
		if (xy.x!=lastx || xy.y!=lasty) { 
			calendar_deselect (c);
			c->view->olddate = c->view->date;
			c->view->date = xytomonth (xy.x, xy.y, date);
			calendar_select (c, monthSelect, (caddr_t)&xy);
			lastx=xy.x;
			lasty=xy.y;
		}	
		break;
	case MS_LEFT:
		if (event_is_down (event)) {
			if (xy.x > 2 || xy.y > 3) {	/* out of bounds */
				calendar_deselect (c);
				lastx=xy.x;
				lasty=xy.y;
				return;
			}
			if (ds_is_double_click(&lastevent, event)) {
				set_defaults(e);
				show_editor(c);
				lastx-=1;
				lasty-=1;
#if 0
				m = (Menu)xv_get(c->items->button2,
						 PANEL_ITEM_MENU);
				month_button(m, NULL);
#endif
			}
			else {
				calendar_deselect (c);
				c->view->olddate = c->view->date;
				c->view->date = xytomonth (xy.x, xy.y, date);
				calendar_select (c, monthSelect, (caddr_t)&xy);
				lastx=xy.x;
				lasty=xy.y;
			}
			if (editor_showing(e)) {
				set_default_calbox(c);
				reset_date_appts(c);
			}
		}
		break;
	default:
		break;
	}
	lastevent = *event;
}

static char *daystring[31] = {"1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20","21","22","23","24","25","26","27","28","29","30","31"};

static void
display_month(c, t, pos, rect)
	Calendar *c; Tick t; struct pr_pos *pos; Rect *rect; 
{
	int i, x, y, pfx, pfy, n, k;
	int double_start = 0;
	int ndays, dayslength, start, fday;
	char *str;
	char *days[7];
	Xv_Font pf1, pf2;
	XContext *xc;
	int gap = 0;  /* pixels between each character */

	if (c==NULL) return;

	if ( strcmp("C", (char *)xv_get(calendar->frame, XV_LC_DISPLAY_LANG)) ) {
		days[0] = MGET("S");
		days[1] = MGET("M");
		days[2] = MGET("T");
		days[3] = MGET("W");
		days[4] = MGET("R");
		days[5] = MGET("F");
		days[6] = MGET("Sa");
	} else {
		days[0] = MGET("S");
		days[1] = MGET("M");
		days[2] = MGET("T");
		days[3] = MGET("W");
		days[4] = MGET("T");
		days[5] = MGET("F");
		days[6] = MGET("S");
	}
	
	x		= pos->x;
	y		= pos->y;
 	pf1		= c->fonts->lucida10;
 	pf2		= c->fonts->lucida12b;

#ifdef OW_I18N
	pfx		= xv_get(pf1, FONT_COLUMN_WIDTH); 
#else
	pfx = (int)xv_get(pf1, FONT_DEFAULT_CHAR_WIDTH);
#endif


	pfy		= xv_get(pf1, FONT_DEFAULT_CHAR_HEIGHT);
	gap = 2 * pfx;
	ndays		= monthlength(t);
	fday		= fdom(t);

	/*	Month		*/
	if ( !init_month_strings ) {
		init_month_strs();
	}
	y += xv_get(pf2, FONT_DEFAULT_CHAR_HEIGHT);
	xc = c->xcontext;
	gr_text(xc, x, y, pf2, months[month(t)], rect);
	y += 4;
	gr_draw_line(xc, x, y, x+20*pfx, y, gr_solid, rect);
	y += xv_get(pf2, FONT_DEFAULT_CHAR_HEIGHT);

	/*	Day Names	*/
	x += pfx;
	for ( k = 0;  k < 7;  k++ ) {
		gr_text(xc, x+(k*(pfx+gap)), y, pf1, days[k], rect);
	}
	y+=pfy;

	/*	Days		*/
	start = fdom(t);
	for ( k = 0;  k < ndays;  k++ ) {
		if ( start > 6 ) {
			start = 0;
			y += pfy;
		} 
		if ( cm_strlen(daystring[k]) > 1 ) {
			x = pos->x + (start * 3) * pfx;
		} else {
			x = pos->x + (start * 3 + 1) * pfx;
		}
		gr_text(xc, x, y, pf1, daystring[k], rect);	
		start++;
	}
}

extern Stat
paint_year(c, rect)
	Calendar *c;
	Rect *rect;
{
	int i, j, margin, topoffset, key;
	int winw, winh, boxw, boxh, month;
	int bottom, right;
	char buf[5];
	struct pr_pos pr_pos;
	Xv_Font pf;
	XContext *xc;
	Stat stat;
	Uid id;
	Appt *a;

	/* we do this to ensure that the calendar exists and is valid */
	/* for V3 release. For V3 we have new status codes   */
	id.next = NULL;
	id.appt_id.tick = 0;
	id.appt_id.key = 0;
	stat = table_lookup(c->view->current_calendar, &id, &a);
	free(a);
	if (stat == status_param && c->general->version <= CMS_VERS_3)
                return status_param;

	winw		= (int) xv_get (c->canvas, XV_WIDTH, 0);
	winh		= (int) xv_get (c->canvas, XV_HEIGHT, 0);
	cache_dims(c, winw, winh);
	boxw		= c->view->boxw;
	boxh		= c->view->boxh;
	margin		= c->view->outside_margin;
	topoffset	= c->view->topoffset;
	key		= c->view->date;
	pf		= c->fonts->lucida12b;
	month		= jan1(key);

	/*	Year		*/
	xc = c->xcontext;
	gr_clear_area(xc, 0, 0, winw, winh);
	sprintf(buf, "%d", year(key));
	gr_text(xc, margin, xv_get(pf, FONT_DEFAULT_CHAR_HEIGHT), pf, buf, rect);

	for (i=0; i < 4; i++) {
		for (j=0; j < 3; j++) {
			pr_pos.x = j*boxw + margin + 15;
			pr_pos.y = i*boxh + topoffset-5;
			display_month(c, month, &pr_pos, rect);	
			month  = nextmonth (month);
		}
	}
	bottom = 4*boxh + topoffset-5;
	right = 3*boxw + margin + 15,0;

	if (winh<bottom) {
	    (void) xv_set (c->canvas, XV_HEIGHT, bottom,NULL);
            window_fit(c->frame);
	}
	if (winw<right) {
	    (void) xv_set (c->canvas, XV_WIDTH, right,NULL);
            window_fit(c->frame);
        }

}
	      
/* ARGSUSED */
extern Notify_value
year_button (m, mi)
	Menu m;
	Menu_item mi;
{
	int w, h, mo;
	Calendar *c;
	XContext *xc;
	struct pr_pos xy;

	c = (Calendar *)xv_get (m, MENU_CLIENT_DATA, 0);
	if (c->view->glance == yearGlance) 
		return NOTIFY_DONE;

	if ( strcmp("C", (char *)xv_get(calendar->frame, XV_LC_DISPLAY_LANG)) ) {
		xv_set(calendar->items->button8, PANEL_LABEL_STRING, LGET("Last Year"), NULL);
		xv_set(calendar->items->button9, PANEL_LABEL_STRING, LGET("This Year"), NULL);
		xv_set(calendar->items->button10, PANEL_LABEL_STRING, LGET("Next Year"), NULL);
	}

	w = (int)xv_get(c->canvas, XV_WIDTH, 0);
        h = (int)xv_get(c->canvas, XV_HEIGHT, 0);
	c->view->glance = yearGlance;
	/* must cache_dims for calendar_deselect() even though its 
		also done in paint_year() */
	cache_dims(c, w, h);
	xc = c->xcontext;
	gr_clear_area(xc, 0, 0, c->view->winw, c->view->winh);
	calendar_deselect (c);
	paint_year(c, NULL);
	mo = month(c->view->date);
	xy.y = month_row_col[mo-1][ROW];
	xy.x = month_row_col[mo-1][COL];
	calendar_select (c, monthSelect, (caddr_t)&xy);
}


/* 
 *	year print routines
 */


extern void
print_std_month(fp, mon, yr, wid, hgt)
	FILE *fp;
	int mon, yr;
	int wid, hgt;
{
	Tick j = monthdayyear(mon,1,yr);	/* starting date Jan 1, y */

	if (mon > 12)
		mon = (mon%13) + 1;
	else if (mon < 1)
		mon = mon + 12;

	if ( !init_month_strings ) {
		init_month_strs();
	}

	if (prolog_found) {
		ps_set_font(fp, "MonthFont", "LC_Times-Bold");
		ps_set_fontsize(fp, "MonthFont", 16);

		ps_set_font(fp, "DayFont", "LC_Times-Italic");
		ps_set_fontsize(fp, "DayFont", 12);

		ps_set_font(fp, "DateFont", "LC_Times-Bold");
		ps_set_fontsize(fp, "DateFont", 12);
	}
	else {
		fprintf (fp, "/ISO_fonts {\n");
		ps_set_font(fp, "MonthFont", "Times-Bold-ISOLatin1");
		fprintf (fp, "systemdict /Times-Italic-ISOLatin1 known {\n\t");
		ps_set_font(fp, "DayFont", "Times-Italic-ISOLatin1");
		fprintf (fp, "}{\n\t");
		ps_set_font(fp, "DayFont", "Courier");
		fprintf (fp, "} ifelse\n");
		ps_set_font(fp, "DateFont", "Times-Bold-ISOLatin1");
		fprintf (fp, "} def\n");
		fprintf (fp, "/Reg_fonts {\n");
		ps_set_font(fp, "MonthFont", "Times-Bold");
		ps_set_font(fp, "DayFont", "Times-Italic");
		ps_set_font(fp, "DateFont", "Times-Bold");
		fprintf (fp, "} def\n");
		fprintf (fp, "%%\n");
		fprintf (fp, "systemdict /ISOLatin1Encoding known\n");
		fprintf (fp, "{ISO_fonts} {Reg_fonts} ifelse\n");
		fprintf (fp, "%%\n");

		ps_set_fontsize(fp, "MonthFont", 16);
		ps_set_fontsize(fp, "DayFont", 12);
		ps_set_fontsize(fp, "DateFont", 12);
	}

	ps_init_std_month(fp, wid, hgt);
	ps_std_month_name(fp, months[mon]);
	ps_std_month_weekdays(fp);
	ps_std_month_dates(fp, fdom(j), monthlength(j));
	ps_finish_std_month(fp);
}

extern void
print_std_year(year)
	int year;
{
	FILE *fp;
	int lmarg, rmarg, tmarg, bmarg;
	int n, mon, incr_x, incr_y;
        Props *p = (Props *)calendar->properties;

	if ((fp=ps_open_file()) == NULL)
		return;
        
	lmarg = INCH;                   /* setup 1" margins */
	rmarg = PRINTER_WIDTH-INCH;
	tmarg = PRINTER_HEIGHT-(1.5*INCH);
	bmarg = 1.5*INCH;
	incr_x= (rmarg-lmarg) / 3;	/* 3 months wide */
	incr_y= (tmarg-bmarg) / 4;	/* 4 months deep */

	n = atoi(p->repeatVAL);
	if (n <= 0)
		n = 1;

	for (; n > 0; n--)
	{
		ps_init_printer(fp, PORTRAIT);
		ps_init_std_year(fp);

		/* print the year at top */
		if ( prolog_found ) {
			ps_set_font(fp, "MonthFont", "LC_Times-Bold");
		} else {
			fprintf (fp, "/ISO_fonts {\n");
			ps_set_font(fp, "MonthFont", "Times-Bold-ISOLatin1");
			fprintf (fp, "} def\n");
			fprintf (fp, "/Reg_fonts {\n");
			ps_set_font(fp, "MonthFont", "Times-Bold");
			fprintf (fp, "} def\n");
			fprintf (fp, "%%\n");
			fprintf (fp, "systemdict /ISOLatin1Encoding known\n");
			fprintf (fp, "{ISO_fonts} {Reg_fonts} ifelse\n");
			fprintf (fp, "%%\n");
		}
		ps_set_fontsize(fp, "MonthFont", 40);
		ps_std_year_name(fp, year);

		/* setup where to print */
		ps_translate(fp, lmarg, tmarg);

		/* print the months - 3 months wide and 4 months deep */
		for (mon = 1; mon <= 12; mon++)
		{
			print_std_month(fp, mon, year, incr_x, incr_y);
			if (mon % 3)
				/* go right */
				ps_translate(fp, incr_x, 0);	
			else
				/* go to next line */
				ps_translate(fp, -2*incr_x , -incr_y );
		}
		ps_finish_printer(fp);
		year++;
	}

	if (fp)
		fclose(fp);
	ps_print_file();
}

extern void
print_alt_month (fp, mon, yr, wid, hgt)
	FILE *fp;
	int mon, yr;
	int wid, hgt;
{
	Tick j = monthdayyear(mon,1,yr);	/* starting date mon 1, yr */

	if ( !init_month_strings ) {
		init_month_strs();
	}
	if ( prolog_found ) {
		ps_set_font(fp, "MonthFont", "LC_Helvetica-Bold");
		ps_set_fontsize(fp, "MonthFont", 18);

		ps_set_font(fp, "DayFont", "LC_Helvetica-Bold");
		ps_set_fontsize(fp, "DayFont", 9);

		ps_set_font(fp, "DateFont", "LC_Helvetica");
		ps_set_fontsize(fp, "DateFont", 10);
	} else {
		fprintf (fp, "/ISO_fonts {\n");
		/* Make sure the printer has the font */
		fprintf (fp, "systemdict /Helvetica-Bold-ISOLatin1 known {\n\t");
		ps_set_font(fp, "MonthFont", "Helvetica-Bold-ISOLatin1");
		ps_set_font(fp, "DayFont", "Helvetica-Bold-ISOLatin1");
		fprintf (fp, "}{\n\t");
		ps_set_font(fp, "MonthFont", "Courier");
		ps_set_font(fp, "DayFont", "Courier");
		fprintf (fp, "} ifelse\n");
		ps_set_font(fp, "DateFont", "Helvetica-ISOLatin1");
		fprintf (fp, "} def\n");
		fprintf (fp, "/Reg_fonts {\n");
		ps_set_font(fp, "MonthFont", "Helvetica-Bold");
		ps_set_font(fp, "DayFont", "Helvetica-Bold");
		ps_set_font(fp, "DateFont", "Helvetica");
		fprintf (fp, "} def\n");
		fprintf (fp, "%%\n");
		fprintf (fp, "systemdict /ISOLatin1Encoding known\n");
		fprintf (fp, "{ISO_fonts} {Reg_fonts} ifelse\n");
		fprintf (fp, "%%\n");

		ps_set_fontsize(fp, "MonthFont", 18);
		ps_set_fontsize(fp, "DayFont", 9);
		ps_set_fontsize(fp, "DateFont", 10);
	}

	ps_init_alt_month(fp, wid, hgt);
	ps_alt_month_name(fp, months_alt[mon]);
	if (mon == 1)
		ps_alt_month_weekdays (fp);
	ps_alt_month_boxes(fp);
	ps_alt_month_dates(fp, fdom(j), monthlength(j));
	ps_finish_alt_month(fp);
}

extern void
print_alt_year (year)
	int year;
{
	FILE *fp;
	int lmarg, rmarg, tmarg, bmarg;
	int n, mon, incr_x, incr_y;
	int year_font_size = 24;
        Props *p = (Props *)calendar->properties;

	if ((fp=ps_open_file()) == NULL)
		return;
        
	/* landscape mode */
	lmarg = INCH;                   /* setup 1" margins */
	rmarg = 10*INCH;
	tmarg = (int) (7.5*INCH);
	bmarg = INCH;
	incr_x= rmarg-lmarg;
	incr_y= (tmarg-bmarg-year_font_size) / 12;


	n = atoi(p->repeatVAL);
	if (n <= 0)
		n = 1;

	for (; n > 0; n--)
	{
		ps_init_printer(fp, LANDSCAPE);
		ps_init_alt_year(fp);

		/* print the year at top */
		if ( prolog_found ) {
			ps_set_font(fp, "MonthFont", "LC_Helvetica-Bold");
		} else {
			fprintf (fp, "/ISO_fonts {\n");
			fprintf (fp, "systemdict /Helvetica-Bold-ISOLatin1 known {\n\t");
			ps_set_font(fp, "MonthFont", "Helvetica-Bold-ISOLatin1");
			fprintf (fp, "}{\n\t");
			ps_set_font(fp, "MonthFont", "Courier");
			fprintf (fp, "} ifelse\n");
			fprintf (fp, "} def\n");
			fprintf (fp, "/Reg_fonts {\n");
			ps_set_font(fp, "MonthFont", "Helvetica-Bold");
			fprintf (fp, "} def\n");
			fprintf (fp, "%%\n");
			fprintf (fp, "systemdict /ISOLatin1Encoding known\n");
			fprintf (fp, "{ISO_fonts} {Reg_fonts} ifelse\n");
			fprintf (fp, "%%\n");
		}
		ps_set_fontsize(fp, "MonthFont", year_font_size);
		ps_alt_year_name(fp, year);

		/* setup where to print */
		ps_translate(fp, lmarg, tmarg-year_font_size);

		/* print the months - 1 month per line */
		for (mon = 1; mon <= 12; mon++)
		{
			print_alt_month(fp, mon, year, incr_x, incr_y);
			ps_translate(fp, 0, -incr_y );
		}
		ps_finish_printer(fp);
		year++;
	}

	if (fp)
		fclose(fp);
	ps_print_file();
}

/* ARGSUSED */
extern Notify_value
ps_std_year_button (m, mi)
        Menu m;
        Menu_item mi;
{
        Calendar *c;

        c = (Calendar *)xv_get (m, MENU_CLIENT_DATA, 0);
        print_std_year(year(c->view->date));

	return NOTIFY_DONE;
}

/* ARGSUSED */
extern Notify_value
ps_alt_year_button (m, mi)
        Menu m;
        Menu_item mi;
{
        Calendar *c;

        c = (Calendar *)xv_get (m, MENU_CLIENT_DATA, 0);
        print_alt_year(year(c->view->date));

	return NOTIFY_DONE;
}
