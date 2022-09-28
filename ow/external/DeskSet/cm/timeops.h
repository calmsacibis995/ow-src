/*static  char sccsid[] = "@(#)timeops.h 3.10 94/03/16 Copyr 1991 Sun Microsystems, Inc.";
 *	timeops.h	
 */

#define EOT			2147483647
#define CMFOREVER			999999999
#define TM_CENTURY 1900                /* years to add to tm_year */
				       /* in tm structures */
typedef enum {
	minsec=60,
	fivemins=300,
	hrsec=3600,
	daysec=86400,
	wksec=604800,
	yrsec=31536000,
	leapyrsec=31622400
} Unit;

typedef long Tick;		/* tick=seconds since epoch */

extern Tick bot;		/* beginning of time */

extern Tick eot;		/* end of time */

extern char *months[];

extern char *months2[];

extern int monthsecs[];

extern char *days[];

extern char *days2[];

extern char *days3[];

extern char *days4[];

extern char *numbers[];

extern char *hours[];

/*	given a tick, return the year	*/

extern int year(/* Tick t */);

/*	given a tick, return the month	*/

extern int month(/* Tick t */);

/*	given a tick, return the hour	*/

extern int hour(/* Tick t */);

/*	given a tick, return the minute	*/

extern int minute(/* Tick t */);

/*	given a tick, return the number of days in the month */

extern int monthlength(/* Tick t */);

/*	given a tick, return the day of the month (1-31)	*/

extern int dom(/* Tick t */);		/* day of month */

/*	given a tick, return the week of the month (1-5)	*/

extern int wom(/* Tick t */);		/* week of month */

/*	given a tick, return the day of the week (1-7)	*/

extern int dow(/* Tick t */);		/* day of week */

/*	given a tick, return the first day of the month which 
	contains that day					*/

extern int fdom(/* Tick t */);		/* first day (1-7) of month */

/*	given a tick, return the last day of
	the month which contains that day			*/

extern int ldom (/* Tick t */);		/* last day (1-7) of month */

/*	given a year, month, and day (e.g. 1989 12 15) return a tick	*/

extern Tick sse(/* int y, m, d */);	/* seconds since epoch */

/*	given a start tick and an advance of n hours, return the tick
	which advances time n hours, compensating for daylight
	savings and leap year irregularities		
	(e.g. 2:00p.m. go forward 2 -> 4:00p.m			*/
	
extern Tick next_nhours(/* Tick t; int n */);

/*	given a start tick, return the tick
	which advances time 1 day, compensating for daylight
	savings and leap year irregularities
	(e.g. 2:15p.m. go forward 2 -> 4:15p.m.			*/ 
        
extern Tick nextday(/* Tick t */);	

/*	given a start tick, return the tick 
	which advances time n days, compensating for daylight
	savings and leap year irregularities                      
	(e.g. Tuesday -> Wednesday 12:00a.m.)			*/
 
extern Tick next_ndays(/* Tick t; int n */);

/*	given a start tick, return the tick  
	which turns back time n days, compensating for daylight 
	savings and leap year irregularities
	(e.g. Tuesday, go forward 3 -> Friday 12:00a.m.)	*/

extern Tick last_ndays(/* Tick t; int n */);

/*	given a start tick anywhere in a month, return the
	tick of the first second of the previous month
	(e.g. Friday, go back 3 -> Tuesday 12:00a.m.)		*/

extern Tick previousmonth(/* Tick t */);

/*      given a start tick anywhere in a month, return the
        tick of the first second of the next month
	(e.g. Jun 21 -> May 1, 12:00a.m.)			*/

extern Tick nextmonth(/* Tick t */);

/*      given a start tick anywhere in a month, return the 
        tick of the first second of the exact day of the
	next month (e.g. Jan 21 -> Feb 21, 12:00a.m.)            */

extern Tick nextmonth_exactday(/* Tick t */);

/*	given a month, day, year, return the tick 
	(e.g. 1 1 89)						*/

/*      given a start tick anywhere in a year, return the 
        tick of the first second of the previous year 
	(e.g. Jun 21, 1988  -> Jan 1, 1987 12:00a.m.)            */

extern Tick lastjan1(/* Tick t */);

/*      given a start tick anywhere in a year, return the 
        tick of the first second of the next year 
	(e.g. Jun 21, 1988  -> Jan 1, 1989 12:00a.m.)            */

extern Tick nextjan1(/* Tick t */);

/*	given a start tick anywhere in a year, return the
	tick of the next year, same day and time	
	(e.g. Jun 21, 1988 1:00p.m. -> Jun 21, 1989 1:00p.m.	*/

extern Tick nextyear(/* Tick t */);

/*	given a tick, compute the beginning of the day it represents
	at 12a.m.  used for narrowing range for appointment lookup
	and later display.  a lookup range is usually
	lowerbound(t)-nextday(t).				*/

extern Tick lowerbound(/* Tick t */);	

/*	given an x, y coordinate and a previous tick for reference,
	compute exact tick.  used for mapping position on calendar
	grid to an exact day.						*/

extern Tick xytoclock(/* int x, y; Tick t */);

/*	Return the tick for current second.	*/

extern Tick now();

/*	Return the tick for 12:00 a.m. of next day */

extern Tick midnight();

/*	set TZ to timezone name			*/

extern void set_timezone(/* char *tzname */);

/*	return the offset in seconds from GMT	*/

extern long gmt_off();

/*	initialize the end of beginning of time (bot) to
	be (what? fix this) and the end of time (eot) to
	be Jan 1, 2001.						*/

extern void init_time();

/* Given a target, calculate the closest instance whose tick is
 * later than the time.
 * If the calculated tick does not pass timeok(), ftick is returned
 * and ordinal set to 1.
 */
extern Tick closest_tick(/* Tick target; Tick ftick; Period period; int *ordinal */);

/* Calculate the tick of the last instance of a repeating appointment.
 * If the calculated tick does not pass timeok(), EOT is returned
 */
extern Tick last_tick(/* Tick ftick; Period period; int ntimes */);

/* Calculate the tick of next instance.
 * If the calculated tick does not pass timeok(), EOT is returned.
 */
extern Tick next_tick(/* Tick tick; Period period */);

/* Calculate the tick of previous instance.
 * If the calculated tick does not pass timeok(), bot-1 is returned.
 */
extern Tick prev_tick(/* Tick tick; Period period */);

/* Given a weekmask and the first day of the week, calculate
 * the number of times outstanding in the week.
 */
extern int ntimes_this_week(/* u_int weekmask; int firstday */);

/* Given a weekmask, find the last appointment in the week */
extern int lastapptofweek(/* u_int mask */);

extern int seconds_to_hrs();
extern int hours_to_seconds();
extern int seconds_to_minutes();
extern int minutes_to_seconds();
extern int days_to_seconds();
extern int seconds_to_days();
extern int weeks_to_seconds();
extern int seconds_to_weeks();
extern int numwks();
extern Tick upperbound();
extern Tick last_dom();
extern Tick first_dom();
extern Tick last_dow();
extern Tick first_dow();
extern Tick lower_bound();	
extern Boolean magic_time();
extern int monthdays[];
extern Tick monthdayyear(/* m, d, y */);
