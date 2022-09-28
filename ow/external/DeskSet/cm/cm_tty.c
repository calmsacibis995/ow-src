#ifndef lint
static  char sccsid[] = "@(#)cm_tty.c 3.32 96/06/11 Copyr 1991 Sun Microsystems, Inc.";
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
#include <string.h>
#include <rpc/rpc.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include "util.h"
#include "appt.h"
#include "table.h"
#include "datefield.h"
#include "timeops.h"
#include "cm_tty.h"
#include "gettext.h"
#include "misc.h"

static char     cal_default[2 * BUFSIZ];
static char* empty_string();
static int mins_to_seconds();
static void format_menu_item();		/* format date lines */
static void formatheader();		/* format time into ascii date */
static Uid* cm_list[256];		/* list of Uid(tick) pointers for */
					/* each appointment */
static Boolean valid_time();		/* validate time string */
static void real_newline();		/* realize newlines */

static Insert_Info *i;

int
cm_tty_lookup(target,date,view)
char** target;
char* date;
char* view;
{
	char *buf;
	char date_str[256];
	Abb_Appt *a=NULL, *head;
	Range range;
	Lines *lines=NULL, *wlines;
	Uid* client_data;
	Uid** list  = cm_list;
	int tick, bytes;
	int lineno;
	int span = 1;			/* default to single day */
	int day = 0;
	int last_day = -1;
	static char def_view[] = "day";
	int appt_total = 0;
	void cm_tty_get_defaults();
	Boolean cmdline_calendar = true;

	if (!*target) {
		*target = table_get_credentials();
		/* get default calendar location from .desksetdefaults 
		later if no command line calendar specified */
		cmdline_calendar = false;
	}
	else if (strchr(*target, '@') == NULL) {
		fprintf(stdout,	"\nError: Specify a host for the calendar name.\n");
		return (0);
	}
	if (i == NULL)
		cm_tty_get_defaults(target, cmdline_calendar);

	if (!date) tick = now();
	else {
		int j, day_of_week = 0;
                datestr2mdy(date, i->ordering, i->separator, date_str);

		/* Is the entered date a day of the week? */
		for (j = 0; j < 7 && !day_of_week; j++)
			day_of_week = (strcasecmp (date, days2 [j] ) == 0);

		/* Make sure the entire date string is lower case to avoid
		   problems in yyparse (). */
		if (day_of_week) {
			for (j = 0; j < strlen (date); j++) 
				date [j] = tolower (date [j] );
		}
		if (day_of_week ||
			strcasecmp(date, "today") == 0 ||
			strcasecmp(date, "tomorrow") == 0 ||
			strcasecmp(date, "yesterday") == 0)
			sprintf(date_str, "%s 11:59 pm", date);
		tick = cm_getdate(date_str, NULL);
		if (tick < 0) {
			fprintf(stderr, "Error in Date Field...\n");
			return -1;
		}
	}
	if (!view) view = def_view;
	if (!strcmp(view,"day")) {
		day = 0;
		span = 1;
	}
	if (!strcmp(view,"week")) {
		day = dow(tick);	/* 0=Sun, 1=Mon,  etc. */
		span = 7;
	}
	if (!strcmp(view,"month")) {
		day = dom(tick) - 1;	/* 1- 31, adjust for no day 0 */
		span = monthlength(tick);
	}
		
	range.key1 = lowerbound(tick - (day * daysec));
	range.key2 = next_ndays(range.key1+1, span);
	range.next = NULL;
	table_abbrev_lookup_range(*target, &range, &a);

	head = a;
	lineno = 1;
	memset((char*) cm_list, 0, sizeof cm_list);
					/* clean table for load */
	formatheader(range.key1+1,date_str);
	while(a!=NULL && a->appt_id.tick < range.key2) { 
		appt_total++;
		day = dom(a->appt_id.tick);
		if(day != last_day) {
			formatheader(a->appt_id.tick,date_str);
			fprintf(stdout,	"\nAppointments for %s:\n",date_str);
		}
		last_day = day;
		a->what = empty_string(a->what);
		lines = text_to_lines(a->what,5);
		if (lines != NULL) {
			if (lines->s != NULL) {
				for (bytes = 0, wlines = lines; 
					wlines != NULL; 
					wlines = wlines->next)
 					bytes += cm_strlen(wlines->s);
				buf = (char *) ckalloc(bytes + 20); 
				buf [0] = '\0';
				format_menu_item(a, buf);
				client_data = (Uid*)ckalloc(sizeof(Uid));
				client_data->appt_id.tick = a->appt_id.tick;
				client_data->appt_id.key = a->appt_id.key;
				client_data->next = NULL;
				*list++ = client_data;
				fprintf(stdout,	"\t%d) %s\n",lineno++,buf);
				while(lines->next) {
					fprintf(stdout,	"\t         %s\n",lines->next->s);
					lines = lines->next;
				}
				fprintf(stdout,	"\n");
				free(buf); buf=NULL;
			}
			destroy_lines(lines); lines=NULL;
		}
		a = a->next;
	}
	destroy_abbrev_appt(head); 
	if(!appt_total) {
		fprintf(stdout,	"\nNo Appointments for %s:\n",date_str);
	}
	return(appt_total);
}

int
cm_delete(target,index)
char **target;				/* target calendar (daemon db) */
int index;				/* index into Uid appt table */
{
	Appt *ca = NULL;		/* dynamic appt ptr */
        int     ntimes=0,j=0;
	Tick	tick=0;
	Appt	*p=NULL;
	Period interval;
	Boolean delete_succeeded = false, single_deleted=true;
	Uidopt uidopt;
	void cm_tty_get_defaults();
	Boolean cmdline_calendar = true;
	char c;

	if (!*target) {
		*target = table_get_credentials();
		/* get default calendar location from .desksetdefaults 
		later if no command line calendar specified */
		cmdline_calendar = false;
	}				/* get user@host name */
	else if (strchr(*target, '@') == NULL) {
                fprintf(stdout, "\nError: Specify a host for the calendar name.\n");
                return (0);
        }


	if (i == NULL)
		cm_tty_get_defaults(target, cmdline_calendar);

        if (--index < 0) return(-1);	/* adjust for 0 based table */

	table_lookup(*target, cm_list[index], &ca); 
					/* get appt */
	if(ca != NULL && ca->appt_id.tick < EOT) {
		uidopt.appt_id = ca->appt_id;
		uidopt.option = do_all;
		uidopt.next = NULL;
               	if (ca->period.period == single) {
        	       	if (table_delete(*target, &uidopt, &p) == status_denied) {
				if (ca->what[0] == '\0')
					fprintf(stderr,
					"Delete Access Denied: Appointment Not Deleted\n");
				else
					fprintf(stderr, 
					"Delete Access Denied: '%s' Not Deleted\n", ca->what);
			}
                }
                else {
			fprintf(stderr,
                                "The appointment %s",ca->what);
			fprintf(stderr,
				"is part of a repeating series.\n");
			fprintf(stderr,
				"Do you want to delete all of them?\n");

                       	switch(c = getc(stdin)) {
                        case 'y':
                        case 'Y':
                        	if (table_delete(*target, &uidopt, &p) == status_denied) {
					if (ca->what[0] == '\0')
						fprintf(stdout,
						"Delete Access Denied: Appointment Not Deleted\n");
					else
						fprintf(stdout,
						"Delete Access Denied: '%s' Not Deleted\n", ca->what);
				}
				else single_deleted = false;
                                break;
			default:
				uidopt.option = do_one;
                		if (table_delete(*target, &uidopt, &p) == status_denied) {
					if (ca->what[0] == '\0')
						fprintf(stdout,
						"Delete Access Denied: Appointment Not Deleted\n");
					else
						fprintf(stdout, 
						"Delete Access Denied: '%s' Not Deleted\n", ca->what);
				}
                                break;
                        }
			/* clears the \n after the yes or no */
			if (c != '\n') getc(stdin);
		}
	}
	if (p != NULL) {
		if (!single_deleted) {
			tick = p->appt_id.tick;
			ntimes = p->ntimes;
			interval = p->period;
			for(j = 0; j <= ntimes; j++) {
				tick = next_tick(tick, interval);
			}
		}
		destroy_appt(p);
		p = NULL;
		delete_succeeded = true;
	}
	return(1);
}

static void
format_menu_item(a, b)
	Abb_Appt *a; char *b;
{
	int endhr, endmn, hr, mn, tick;
	char start_ampm[6], end_ampm[6];
	Lines *lines=NULL;

	if(a==NULL || b==NULL) return;
	tick = a->appt_id.tick;
  	hr = hour(tick);
  	mn = minute(tick);
  	endhr = hour(tick+a->duration);
	endmn = minute(tick+a->duration);
	if (a->tag->showtime && !magic_time(tick)) {
		if (i->time_format == 0) {
			/* 12 hour am/pm format */
			hr = hours_to_ampm(hr, start_ampm);
			endhr = hours_to_ampm(endhr, end_ampm);
			/* convert to AM/PM time */
			if (hr == endhr && mn == endmn && strcmp(start_ampm, end_ampm) == 0)
				sprintf(b, "%2d:%02d%s ", hr, mn, start_ampm);
			else
				sprintf(b, "%2d:%02d%s-%d:%02d%s ", hr, mn, start_ampm, endhr, endmn, end_ampm);
		}
		else {
			/* 24 hour format */
			if (endhr == hr && endmn == mn)
				sprintf(b, "%02d%02d ", hr, mn);
			else
				sprintf(b, "%02d%02d-%02d%02d ", hr, mn, endhr, endmn);
		}
	}
	a->what = empty_string(a->what);
	lines = text_to_lines(a->what, 1);
	if (lines != NULL) {
		if (lines->s != NULL) {
			(void) cm_strcat(b, lines->s); 
		}
		destroy_lines(lines);
	}
}

/*
*	Build ascii date/time line from integer (tick)
*/
static void
formatheader(tick, buf)
	Tick tick; char *buf;
{
	int mn, dm, dy, yr;
	if (buf==NULL) return;
	
	dy	= dow(tick);
	mn	= month(tick);
	dm	= dom(tick);
	yr	= year(tick);
	buf[0]	= NULL;
	if (i->ordering == Order_MDY)
		(void) sprintf(buf, "%s %s %d, %d", days2[dy], months[mn], dm, yr);
	else if (i->ordering == Order_DMY)
		(void) sprintf(buf, "%s %d %s, %d", days2[dy], dm, months[mn], yr);
	else if (i->ordering == Order_YMD)
		(void) sprintf(buf, "%s, %d %s %d", days2[dy], yr, months[mn], dm);
}
	
int 
cm_tty_insert(target,date,start,end,what)
char** target;
char* date;
char* start;
char* end;
char* what;
{
	static char empty[] = "\0";
	static char scope[] = "forever";
	char date_str[25], save_buf[75];
        Appt    *a, *p;
	Attribute *attr;
	Lines *lines=NULL;
	char	*s1=NULL, *s2=NULL, *s3=NULL, *s4=NULL, *s5=NULL;
	char	*startval=NULL, *endval = NULL;
	int	l=0, secs=0, next=0, alert_res=0;
	void	cm_tty_get_defaults();
	Boolean cmdline_calendar = true, showtime = true;

	/* Start and end times are checked for validity. If they both
		exist and are valid then everything is just honky-dory. 
		If start and end times exist but either is invalid, 
		an error message is generated and routine returns.
		If start time exists and is valid and end time doesn't
		exist, then the end time defaults to start time plus one
		hour. If start time doesn't exist but end time exists,
		then its a error. If neither start nor end times exist,
		then it is a 'to-do item, whereby the start time is
		set to 3:41 am and the end time to 3:41 plus one minute.
		Got it??
	*/

	if (!*target) {
		/* get user@host name */
		*target = table_get_credentials();
		/* get default calendar location from .desksetdefaults 
		later if no command line calendar specified */
		cmdline_calendar = false;
	}
	else if (strchr(*target, '@') == NULL) {
                fprintf(stdout, "\nError: Specify a host for the calendar name.\n");
                return (-1);
        }


	if (i == NULL)
		cm_tty_get_defaults(target, cmdline_calendar);

	startval = start;
	endval  = end;

	date_str[0] = NULL;
	if (!date)
		format_tick(now(), Order_MDY, Separator_Slash, date_str);
	else {
                if (strcasecmp(date, "today") == 0 ||
                        strcasecmp(date, "tomorrow") == 0 ||
                        strcasecmp(date, "yesterday") == 0) {
                        strcpy(date_str, date);
			if (startval == NULL || *startval == NULL) {
                        	strcat(date_str, " 12:00 am");
				startval = (char*)-1;
			}
		}
		else {
                	datestr2mdy(date, i->ordering, i->separator, date_str);
			if (date_str == NULL || *date_str == NULL) {
				fprintf(stderr, "Error in Date Field...\n");
                        	return -1;
			}
		}
	}

	strcpy(save_buf, date_str);

	if (startval != (char*)-1 && startval != NULL && *startval != '\0') {
		if (valid_time(startval) && date_str[0] != NULL) 
			(void)sprintf(date_str, "%s %s", save_buf, startval);

		else { 
			fprintf(stderr,
				"Error in Starting Time...\n");
                        return(-1);
                }
		if (endval != NULL && *endval != '\0') {
			if (!valid_time(endval)) {
                		fprintf(stderr,
                        		"Error in Ending Time...\n");
                		return(-1);
			}
		}
	}
	else if (endval != NULL && *endval != '\0') {
	/* if start time is null but there is an end time: error */
		fprintf(stderr,
			"Specify a Starting Time...");
                        return(-1);
	}
	else {
	/* if start time and end times are null then showtime is false */
		showtime = false;
		if (startval != (char*)-1 && date_str[0] != NULL)
			(void) strcat(date_str, " 3:41 am");
	}

	/* create appt, build up date/time string from panel, send it 
		through parser to get a tick.		 */

	a = make_appt();
	a->author = *target;
	/* remind */
	a->appt_id.tick = (int) cm_getdate(date_str, NULL);
	if(a->appt_id.tick < 0) {
		destroy_appt(a);
		fprintf(stderr,
			"Error in Date Field...\n");
		return(-1);
	}

	/* stuff in what strings */

	real_newline(what);		/* realize newlines */
	what = empty_string(what);
	l = cm_strlen(what);
	a->what = (char *) ckalloc(l + 15);
	cm_strcpy(a->what,what);
	empty_string(a->what);

	/* Period Exclusive Choice */
	a->period.period = single;
	if (a->period.period != single) {
		if(strcmp(scope, "forever")==0) {
			a->ntimes=CMFOREVER;
		}
		else {
			a->ntimes = atoi(scope);
		}
	}

	/* Duration: */
	/* start and end times are NULL. */
	if (!showtime)  {
		a->duration = (long)minsec;
		a->tag->showtime = false;
	}
	else {
		/* endval = startval + 1 hour */
		if (endval == NULL || *endval == '\0')
			next = a->appt_id.tick + hrsec; 
		else {
                	(void) sprintf(date_str, "%s %s", save_buf, endval);
                	next = (int) cm_getdate(date_str, NULL);
		}
		if (next < a->appt_id.tick) {
			fprintf(stderr,
				"This appointment has an end\n");
			fprintf(stderr,
				"time earlier than its begin\n");
			fprintf(stderr,
				"time.  Do you want to schedule\n");
			fprintf(stderr,
				"it into the next day?  ");
			alert_res = getchar();
			switch((char)alert_res) {
				case 'y':
					a->duration =
						(next + daysec) - a->appt_id.tick;
					break;
				default:
					destroy_appt(a); 
					return(-1);
			}
		}
		else {
			a->duration = next - a->appt_id.tick;
		}
	}


	/* Reminder Non-exclusive Choice */
	if (i->beep_on) {
		attr = make_attr();
		attr->attr = cm_strdup("bp");
                secs = units_to_secs(i->beep_unit, i->beep_advance);
		sprintf(save_buf, "%d", secs);
		attr->value = cm_strdup(save_buf);
		attr->next = a->attr;
		a->attr = attr;
	}
	if (i->flash_on) {
		attr = make_attr();
		attr->attr = cm_strdup("fl");
                secs = units_to_secs(i->flash_unit, i->flash_advance);
		sprintf(save_buf, "%d", secs);
		attr->value = cm_strdup(save_buf);
		attr->next = a->attr;
		a->attr = attr;
	}
	if (i->open_on) {
		attr = make_attr();
		attr->attr = cm_strdup("op");
		secs = units_to_secs(i->open_unit, i->open_advance);
		sprintf(save_buf, "%d", secs);
		attr->value = cm_strdup(save_buf);
		attr->next = a->attr;
		a->attr = attr;
		}
	if (i->mail_on) {
		/* To Field */
		l = cm_strlen(i->mail_to);
		if(l) {
			attr = make_attr();
			attr->attr = cm_strdup("ml");
                	secs = units_to_secs(i->mail_unit, i->mail_advance);
			sprintf(save_buf, "%d", secs);
			attr->value = cm_strdup(save_buf);
			attr->clientdata = cm_strdup(i->mail_to);
			attr->next = a->attr;
			a->attr = attr;
		}
	}
        if (table_insert(*target, a, &p) == status_denied) {
			fprintf(stderr,
				"Insert Access Denied: ");
			fprintf(stderr,
				"Appointment Not Inserted.\n");
			return(-1);
	}
	return(0);
}

static Boolean
valid_time(time_str)
	char *time_str;
{
	char 	*ptr;
	int 	time;
	Boolean ampm = false, colon = false;

	for (ptr = time_str; ptr != NULL && *ptr != '\0'; ptr++) {
		if (*ptr == ':' || *ptr == ' ') {
			if (*ptr == ':') colon = true;
			continue;	
		}
		if (!strcmp(ptr,"am") || !strcmp(ptr,"AM") ||
			 !strcmp(ptr,"pm") || !strcmp(ptr,"PM")) {
			ampm = true;
			break;
		}
		if (*ptr <  '0' || *ptr > '9')
			return false;
	}
	/* if 12 hour format you must have am or pm set if no colon,
	   for example -s 2 will not work, but 2:00 will work */
	if (i->time_format == 0 && !colon && !ampm)
		strcat(time_str, " am");

	return true;
}

/*
*	Realize newlines in what string
*/

static void
real_newline(string)
char* string;
{
	char* from = string;
	char* to = string;

	while (*from) {
		switch (*from++) {
		case '\\':		/* handle backslash escapes */
			switch (*from++) {
			case 'n':
				*to++ = '\n';
				break;
			case 't':
				*to++ = '\t';
				break;
			default:
				*to++ = *(from-1);
			}
			break;
		default:
			*to++ = *(from-1);
					/* just copy existing character */
			break;
		}
	}
	*to = '\0';			/* tag end of string */
}

static int
hours_to_secs(n)
        int n;
{
        return(n *(int)hrsec);
}

static int
mins_to_seconds(n)
	int n;
{
	return(n *(int)minsec);
}

/*
*	Convert 24 hour time to meridian (AM/PM) time
*/

extern int
hours_to_ampm(hour,ampm)
int hour;
char *ampm;
{

	if (hour<0) return 0;		/* no negative time */
	if (hour<12) {
		cm_strcpy(ampm, "am");
		if (hour == 0) hour = 12;
					/* zero is 12:00 AM (midnight) */
		return(hour);
	}
	if (hour<24) {
		cm_strcpy(ampm, "pm");
		hour-=12;		/* adjust for PM */
		if (hour == 0) hour = 12;
					/* zero is 12:00 PM (noon) */
		return(hour);
	}
	return(0);			/* range error */
}

static char*
empty_string(str)
	char *str;
{
	if (str==NULL) return(NULL);
	if (str[0] == '\0') {
		str[0] = ' ';
		str[1] = '\0';
	}
	return str;
}
extern void
init_strings()
{
        days2[0] = "Sunday";
        days2[1] = "Monday";
        days2[2] = "Tuesday";
        days2[3] = "Wednesday";
        days2[4] = "Thursday";
        days2[5] = "Friday";
        days2[6] = "Saturday";
        days2[7] = "Sunday";

        months[1] = "January";
        months[2] = "February";
        months[3] = "March";
        months[4] = "April";
        months[5] = "May";
        months[6] = "June";
        months[7] = "July";
        months[8] = "August";
        months[9] = "September";
        months[10] ="October";
        months[11] ="November";
        months[12] = "December";
}

static void
cm_tty_get_defaults(target, cmdline_calendar)
char **target ;
Boolean *cmdline_calendar ;
{
        FILE    *rc_file;
        char    file_name[MAXNAMELEN];
        char    *c_ptr, *d_ptr, *key;
        char    buffer[BUFSIZ];
	int	content_gotten;
	Advunit cm_get_unit();
	Pentry *headp, *pentries;
	int my_cal = 0;
	char *login=NULL, *uname=NULL; 

	i = (Insert_Info *) ckalloc (sizeof(Insert_Info));

	i->beep_advance = cm_strdup("5");
	i->flash_advance = cm_strdup("5");
	i->open_advance = cm_strdup("5");
	i->mail_advance = cm_strdup("5");
	i->beep_on = 1;
	i->flash_on = 1;
	i->open_on = 1;
	i->mail_on = 0;
	i->beep_unit = minsunit;
	i->flash_unit = minsunit;
	i->open_unit = minsunit;
	i->mail_unit = minsunit;
	i->time_format = 0;
	i->mail_to = cm_strdup(*target);
	i->separator = Separator_Slash;
	i->ordering = Order_MDY;

	login = get_head(*target, '@');
	uname = cm_get_uname();
	if (login != NULL && uname != NULL && strcmp(uname, login) == 0)
		my_cal = 1; 
	if (login)
		free(login);

     	/* open the property file */

	headp = pentries = cm_get_resources();
	while (pentries != NULL) {

		if ((!strcmp(pentries->property_name, "Calendar.BeepOn")) || (!strcmp(pentries->property_name, "deskset.calendar.BeepOn")))
			i->beep_on = cm_get_onoff(pentries->property_value);
		else if ((!strcmp(pentries->property_name, "Calendar.FlashOn")) || (!strcmp(pentries->property_name, "deskset.calendar.FlashOn")))
			i->flash_on = cm_get_onoff(pentries->property_value);
		else if ((!strcmp(pentries->property_name, "Calendar.OpenOn")) || (!strcmp(pentries->property_name, "deskset.calendar.OpenOn")))
			i->open_on = cm_get_onoff(pentries->property_value);
		else if ((!strcmp(pentries->property_name, "Calendar.MailOn")) || (!strcmp(pentries->property_name, "deskset.calendar.MailOn")))
			i->mail_on = cm_get_onoff(pentries->property_value);

		else if ((!strcmp(pentries->property_name, "Calendar.BeepAdvance")) || (!strcmp(pentries->property_name, "deskset.calendar.BeepAdvance")))
			i->beep_advance = cm_strdup(pentries->property_value);
		else if ((!strcmp(pentries->property_name, "Calendar.FlashAdvance")) || (!strcmp(pentries->property_name, "deskset.calendar.FlashAdvance")))
			i->flash_advance = cm_strdup(pentries->property_value);
		else if ((!strcmp(pentries->property_name, "Calendar.OpenAdvance")) || (!strcmp(pentries->property_name, "deskset.calendar.OpenAdvance")))
			i->open_advance = cm_strdup(pentries->property_value);
		else if ((!strcmp(pentries->property_name, "Calendar.MailAdvance")) || (!strcmp(pentries->property_name, "deskset.calendar.MailAdvance")))
			i->mail_advance = cm_strdup(pentries->property_value);

		else if ((!strcmp(pentries->property_name, "Calendar.BeepUnit")) || (!strcmp(pentries->property_name, "deskset.calendar.BeepUnit")))
			i->beep_unit = cm_get_unit(pentries->property_value);
		else if ((!strcmp(pentries->property_name, "Calendar.FlashUnit")) || (!strcmp(pentries->property_name, "deskset.calendar.FlashUnit")))
			i->flash_unit = cm_get_unit(pentries->property_value);
		else if ((!strcmp(pentries->property_name, "Calendar.OpenUnit")) || (!strcmp(pentries->property_name, "deskset.calendar.OpenUnit")))
			i->open_unit = cm_get_unit(pentries->property_value);
		else if ((!strcmp(pentries->property_name, "Calendar.MailUnit")) || (!strcmp(pentries->property_name, "deskset.calendar.MailUnit")))
			i->mail_unit = cm_get_unit(pentries->property_value);

		else if (my_cal && ((!strcmp(pentries->property_name, "Calendar.MailTo")) || (!strcmp(pentries->property_name, "deskset.calendar.MailTo"))))
			i->mail_to = cm_strdup(pentries->property_value);
		else if ((!strcmp(pentries->property_name, "Calendar.DefaultDisplay")) || (!strcmp(pentries->property_name, "deskset.calendar.DefaultDisplay")))
			i->time_format = atoi(pentries->property_value);

		else if ((!strcmp(pentries->property_name, "Calendar.DateSeparator")) || (!strcmp(pentries->property_name, "deskset.calendar.DateSeparator")))
			i->separator = atoi(pentries->property_value);
		else if ((!strcmp(pentries->property_name, "Calendar.DateOrdering")) || (!strcmp(pentries->property_name, "deskset.calendar.DateOrdering")))
			i->ordering = atoi(pentries->property_value);
		else if (!cmdline_calendar && ((!strcmp(pentries->property_name, "Calendar.Location")) || (!strcmp(pentries->property_name, "deskset.calendar.Location")))) {
			free(*target);
			*target =  (char*)malloc(cm_strlen(pentries->property_value) + 
				cm_strlen(cm_get_uname()) + 2);
			sprintf(*target, "%s@%s", cm_get_uname(), pentries->property_value);
		}

		pentries = pentries->next;
        }
}

static int
cm_get_onoff(cal_default)
char 	*cal_default;
{
	if ((!strncmp(cal_default, "Y", 1)) || (!strncmp(cal_default, "T", 1))
		|| (!strcmp(cal_default, "1")))

		return(1);
	else
		return(0);
}


static Advunit
cm_get_unit(unit_str)
char	*unit_str;
{
	Advunit val;

        if (strcmp(unit_str, MGET("Mins")) == 0)
                val = minsunit;
        else if (strcmp(unit_str, MGET("Hrs")) == 0)
                val = hrsunit;
        else if (strcmp(unit_str, MGET("Days")) == 0)
                val = daysunit;
        else
                val = minsunit;

        return val;
}

static int
units_to_secs(unit, adv)
        Advunit unit;
        char *adv;
{
        int secs=0;

        switch(unit) {
                case minsunit:
                        secs = minutes_to_seconds( atoi(adv));
                        break;
                case hrsunit:
                        secs = hours_to_seconds( atoi(adv));
                        break;
                case daysunit:
                        secs = days_to_seconds( atoi(adv));
                        break;
        }

        return secs;
}
