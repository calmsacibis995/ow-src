#ifndef lint
static  char sccsid[] = "@(#)format.c 3.7 96/06/11 Copyr 1991 Sun Microsystems, Inc.";
#endif   
/*
 *  Copyright (c) 1987-1992 Sun Microsystems, Inc.
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
#include <sys/time.h>
#include <xview/panel.h>
#include "util.h"
#include "timeops.h"
#include "appt.h"
#include "format.h"
#include "gettext.h"
#include "datefield.h"
#include "props.h"

/* convert tick to m, d, y string.  e.g. May 1, 1988 */
extern void
format_date(t, order, buf, and_day)
	Tick t; 
	Ordering_Type order;
	char *buf; 
	int and_day;
{
	int m, d, y, wd;
	struct tm *tm;

	buf[0]=NULL;
	tm = localtime(&t);
	m = tm->tm_mon+1;
	d = tm->tm_mday;
	y = tm->tm_year + 1900;
	wd = tm->tm_wday;

	switch(order) {
                case Order_DMY:     
			if (and_day) 
			/* STRING_EXTRACTION SUNW_DESKSET_CM_MSG :
			 *
			 * %s, %d %s %4d is a form of the date format. 
			 * Change this to reflect your local convention.
			 * eg. In the German locale, "%s, %d %s %4d" may be changed
			 * to "%s. %d %s %4d".
			 */
				(void)sprintf(buf, MGET("%s, %d %s %4d"), 
						days[wd], d, months[m], y);
			else 
				(void) sprintf(buf, "%s %4d", months[m], y);
                        break;
                case Order_YMD:    
			if (and_day) 
				(void)sprintf(buf, MGET("%s, %4d %s %d"), 
						days[wd], y, months[m], d);
			else 
				(void) sprintf(buf, "%4d %s", y, months[m]);
                        break;
                case Order_MDY:   
                default:
			if (and_day) 
				(void)sprintf(buf, MGET("%s, %s %d %4d"), 
						days[wd], months[m], d, y);
			else 
				(void) sprintf(buf, "%s %4d", months[m], y);
                        break;
	}

}
/* convert tick to m/d   */
extern void
format_date3(t, order, sep, buf)
	Tick t;
	Ordering_Type order;
	Separator_Type sep;
	char *buf; 
{
	int m, d;
	struct tm *tm;  
	extern char *separator[];

	buf[0]=NULL;
	tm = localtime(&t); 

	m = tm->tm_mon+1; 
	d = tm->tm_mday;
	switch(order) {
                case Order_YMD:    
                case Order_MDY:   
                default:
			(void) sprintf(buf, "%d%s%.2d", m, separator[sep], d);
                        break;
                case Order_DMY:     
			(void) sprintf(buf, "%.2d%s%d", d, separator[sep], m);
                        break;
	}
}


/* format 1 line of appt data */
extern void
format_line(tick, what, buf, duration, showtime, display)
        long tick; char *what, *buf; int duration; 
	Boolean showtime;
	DisplayType display;
{
        int hr, hr1, mn, mn1;
        Boolean am=true;
	struct tm *tm;

        if (buf==NULL) return;
        buf[0]=NULL;
	tm = localtime(&tick);
        hr = tm->tm_hour;
        mn = tm->tm_min;
        if (showtime && !magic_time(tick)) {
		if (display == hour12)
			am = adjust_hour(&hr);
                if (duration) {
                        hr1 = hour(tick + duration);
                        mn1 = minute(tick + duration);
			if (display == hour12) {
				adjust_hour(&hr1);
                        	(void) sprintf(buf, "%2d:%.2d -%2d:%.2d ", 
						hr, mn, hr1, mn1);
			}
			else
                        	(void) sprintf(buf, "%02d%02d -%02d%02d ", 
						hr, mn, hr1, mn1);
                }
                else {
                	/* Check to see if there are 2 digits in
                   	in initial time format. If so, pad with
                   	1 space; if not 2.  The font is not fixed
                   	width, so I have to line it up myself.. */
			if (display == hour12) {
                        	if (hr > 9)
                               		 (char *)sprintf(buf, "%2d:%.2d%s ", 
							hr, mn, am ? "a" : "p");
                        	else
                               		 (char *)sprintf(buf, "%3d:%.2d%s ", 
							hr, mn, am ? "a" : "p");
			}
			else
				 (char *)sprintf(buf, "%02d%02d ", hr, mn);
                }
        }
	if (what)
		(void) cm_strcat(buf, what);
}


/* format 2 lines of appt data */
extern void
format_maxchars(a, buf1, maxchars, display)
        Abb_Appt  *a;
        char *buf1;
	int maxchars;
	DisplayType display;
{
        Tick    tick = a->appt_id.tick;
        int     hour1, min1, hour2, min2;
        Lines   *lines;
        char    *s1, *s2;
	struct tm *tm;

 
        *buf1 = NULL;
        if (a == NULL || a->what == NULL) return;
        tm = localtime(&tick);
        hour1 = tm->tm_hour;
        min1  = tm->tm_min;
	if (a->tag->showtime && !magic_time(tick)) {
        	s1 = s2 = "am";
		if (display == hour12 && !adjust_hour(&hour1))
			s1="pm";
		hour2 = hour(tick + a->duration);
		if (display == hour12 && !adjust_hour(&hour2))
			s2="pm";
	 
		min2 = minute(tick + a->duration);
		if (hour1 == hour2 && min1 == min2) {
			if (display == hour24) 
				sprintf(buf1, "%02d%02d  ", hour1, min1);
			else if (strcmp(s1, s2) == 0)
				sprintf(buf1, "%d:%.2d%s  ", hour1, min1, s1);
		}
		else {
			if (display == hour12)
				sprintf(buf1, "%d:%.2d%s-%d:%.2d%s  ", 
					hour1, min1, s1, hour2, min2, s2);
			else
				sprintf(buf1, "%02d%02d-%02d%02d  ", 
					hour1, min1, hour2, min2);
		}
	}
 
        lines = (Lines *) text_to_lines(a->what, 10);
 
	while (lines != NULL) {
		if ((cm_strlen(buf1) + cm_strlen(lines->s)) < (maxchars-3)) {
                	cm_strcat(buf1, lines->s);
			lines = lines->next;
			if (lines != NULL) 
                		cm_strcat(buf1, " - ");
		}
		else {
			strncat(buf1, lines->s, (maxchars - cm_strlen(buf1)-1));
			break;
		}
	}
        destroy_lines(lines);
}


/* format 2 lines of appt data */
extern void
format_line2(a, buf1, buf2, display)
        Abb_Appt  *a;
        char *buf1, *buf2;
	DisplayType display;
{
        Tick    tick = a->appt_id.tick;
        int     hour1, min1, hour2, min2;
        Lines   *lines;
        char    *s1, *s2;
	struct tm *tm;

        /*
         * Extract an appointment and format it into 2 lines of no more
         * then maxchars
         */
        *buf1 = *buf2 = NULL;
        if (a == NULL || a->what == NULL) return;
        tm = localtime(&tick);
        hour1 = tm->tm_hour;
        min1  = tm->tm_min;
 
        if (!a->tag->showtime || magic_time(tick)) {
                lines = (Lines *) text_to_lines(a->what, 1);
                if (lines==NULL) return;
                strncpy(buf2, lines->s, 256);
                destroy_lines(lines);
                return;
        }
 
        s1 = s2 = "am";
	if (display == hour12 && !adjust_hour(&hour1))
		s1="pm";
        hour2 = hour(tick + a->duration);
        min2 = minute(tick + a->duration);
	if (display == hour12 && !adjust_hour(&hour2))
			s2="pm";
 
        if (hour1 == hour2 && min1 == min2 && (strcmp(s1, s2) == 0)) {
		if (display == hour24) 
			sprintf(buf1, "%02d%.2d", hour1, min1);
		else 
			sprintf(buf1, "%d:%.2d%s", hour1, min1, s1);
	}	
	else {
		if (display == hour12) 
			sprintf(buf1, "%d:%.2d%s-%d:%.2d%s", hour1, min1, s1,
				 hour2, min2, s2);
		else
			sprintf(buf1, "%02d%02d-%02d%02d", hour1, min1,
				 hour2, min2);
	}
        
 
        lines = (Lines *) text_to_lines(a->what, 1);
 
        if (lines == NULL || lines->s == NULL ||
		(cm_strlen(lines->s) == 1 && lines->s[0] == ' '))
                buf2[0] = NULL;
        else
                sprintf(buf2, " %s", lines->s);
        destroy_lines(lines);
}


extern void
format_abbrev_appt(a, b, show_am, display)
        Abb_Appt   *a; char *b;
        Boolean show_am;
	DisplayType display;
{
        int hr, mn, tick;
        Lines *lines=NULL;
        Boolean am = true;
	struct tm *tm;
 
        if(a==NULL || b==NULL) return;
        tick = a->appt_id.tick;
        tm = localtime((time_t*)&tick);
        hr = tm->tm_hour;
        mn = tm->tm_min;
        if (a->tag->showtime && !magic_time(tick)) {
		if (display == hour12) {
			am = adjust_hour(&hr);
                	if (show_am)
                        	sprintf(b, "%2d:%02d%s ", hr, mn, am ? 
						"a" : "p");
			else
				sprintf(b, "%2d:%02d ", hr, mn);
		}
		else
			sprintf(b, "%02d%02d ", hr, mn);
        }
        lines = text_to_lines(a->what, 1);
        if (lines != NULL && lines->s != NULL) {
                (void) cm_strcat(b, lines->s);
                destroy_lines(lines);
        }
}


extern void
format_appt(a, b, display)
        Appt  *a; 
	char *b;
	DisplayType display;
{
        int hr, mn, tick;
        Lines *lines=NULL;
	struct tm *tm;
 
        if(a==NULL || b==NULL) return;
        tick = a->appt_id.tick;
	tm = localtime((time_t*)&tick);
        hr = tm->tm_hour;
        mn = tm->tm_min;
        if (a->tag->showtime && !magic_time(tick)) {
		if (display == hour12) {
			adjust_hour(&hr);
                	sprintf(b, "%2d:%02d ", hr, mn);
		}
		else
                	sprintf(b, "%02d%02d ", hr, mn);
        }
        lines = text_to_lines(a->what, 1);
        if (lines != NULL && lines->s != NULL) {
                (void) cm_strcat(b, lines->s);
                destroy_lines(lines);
        }
}
