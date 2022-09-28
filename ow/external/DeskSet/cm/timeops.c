#ifndef lint
static  char sccsid[] = "@(#)timeops.c 3.30 97/05/13 Copyr 1991 Sun Microsystems, Inc.";
#endif

/* timeops.c */
/* NOTHING SHOULD GO INTO THIS FILE THAT IS TOOLKIT OR CM DEPENDENT! */

#include <stdio.h>
#include "appt.h"      /* leave before <sys/param.h> [vmh] */
#include <sys/param.h>
#include <sys/time.h>
#include <rpc/rpc.h>
#include "util.h"
#include "timeops.h"

extern int debug;
int nexttickcalled;
int totalskipped;

typedef enum {dstoff, dston, nochange} DSTchange;

static unsigned int weekdaymasks[] = {
	0x1,	/* sunday */
	0x2,	/* monday */
	0x4,	/* tuesday */
	0x8,	/* wednesday */
	0x10,	/* thursday */
	0x20,	/* friday */
	0x40	/* saturday */
};

Tick bot, eot;

char *months[] = {"",
	   (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL,
	   (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL,
	   (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL
};

char *months2[] = {"",
	   (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL,
	   (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL,
	   (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL
};

int monthdays[12] = {
	31,	28,	31,
	30,	31,	30,
	31,	31,	30,
	31,	30,	31
};

int monthsecs[12] = {
	31*daysec,	28*daysec,	31*daysec,
	30*daysec,	31*daysec,	30*daysec,
	31*daysec,	31*daysec,	30*daysec,
	31*daysec,	30*daysec,	31*daysec
};

char *days[8] = {(char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL};

char *days2[8] = {(char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL};

char *days3[8] = {(char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL};

char *days4[8] = {(char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL};

char *numbers[32] = {"",
        " 1 ", " 2 ", " 3 ", " 4 ", " 5 ", " 6 ", " 7 ",
        " 8 ", " 9 ", "10", "11", "12", "13", "14",
        "15", "16", "17", "18", "19", "20", "21",
        "22", "23", "24", "25", "26", "27", "28",
        "29", "30", "31"
};

char *hours[24] = {
	"12", " 1", " 2", " 3", " 4", " 5", " 6", " 7", " 8", " 9", "10", "11",
	"12", " 1", " 2", " 3", " 4", " 5", " 6", " 7", " 8", " 9", "10", "11"
};

/* for testing purpose
int localtimecalled;
static struct tm *
mylocaltime(tick)
	long *tick;
{
	localtimecalled++;
	return(localtime(tick));
}
*/

/*
 * Given a weekmask and the first day of the week, calculate
 * the number of times outstanding in the week.
 */
extern int
ntimes_this_week(weekmask, firstday)
	u_int weekmask; int firstday;
{
	int i, ntimes, weekdaymask = 1 << firstday;

	if (weekmask == 0)
		return 0;

	for (i=firstday, ntimes=0; i < 7; i++, weekdaymask <<= 1) {
		if (weekdaymask & weekmask)
			ntimes++;
	}
	return ntimes;
}

/* Given a weekmask, find the last appointment in the week */
extern int
lastapptofweek(mask)
	u_int mask;
{
	int n;

	if (mask == 0)
		return -1;

	for (n = -1; mask != 0; n++, mask = mask >> 1);

	return n;
}

/*      Remove after v3 release */
extern Boolean
magic_time(t)
        Tick t;
{
        Boolean magic=false;
        struct tm *tm;

        tm = localtime(&t);

        if (tm->tm_hour == 3 && tm->tm_min == 41)
                 magic=true;
        return(magic);
}
/* for 12 hour clock, if 24 subtract 12 */
extern Boolean
adjust_hour(hr)
	int *hr;
{
	Boolean am = true;

	if (*hr == 0)
		*hr = 12;
	else if (*hr >= 12) {
		if (*hr > 12)
			*hr -= 12;
		am = false;
	}
	return am;
}


extern int
timeok(t)
	Tick t;
{
	int r =((t >= bot) &&(t < eot));
	return(r);
}

static DSTchange
dst_changed(old, new) 
	Tick old, new;
{	
	struct tm oldtm;
	struct tm newtm;
	oldtm	= *localtime(&old);
	newtm	= *localtime(&new);
	
	if(oldtm.tm_isdst ==  newtm.tm_isdst) return(nochange);
	switch(oldtm.tm_isdst) {
		case 1:
			return(dstoff);
		case 0:
			return(dston);
		default:
			return(nochange);
	}
}
	
extern int
seconds(n, unit)
	int n; Unit unit;
{
	return(n *(int)unit);
}

static double
seconds_dble(n, unit)
	double n; Unit unit;
{
	return((double)(n * (int)unit));
}

extern int
year(t)
	Tick t;
{
	struct tm *tm;
	tm = localtime(&t);
	return(tm->tm_year + 1900);
}

extern int
month(t)
	Tick t;
{
	struct tm *tm;
	tm = localtime(&t);
	return(tm->tm_mon+1);
}

extern int
hour(t)
	Tick t;
{
	struct tm *tm;
	tm = localtime(&t);
	return(tm->tm_hour);
}

extern int
minute(t)
	Tick t;
{
	struct tm *tm;
	tm = localtime(&t);
	return(tm->tm_min);
}

extern int
leapyr(y)
	int y;
{
	if (y<TM_CENTURY)
	  y+=TM_CENTURY;

	return
	 (y % 4 == 0 && y % 100 !=0 || y % 400 == 0);
}

extern int
monthlength(t)
	Tick t;
{
	int mon;
	struct tm tm;

	tm = *localtime(&t);
	mon = tm.tm_mon;
	return(((mon==1) && leapyr(tm.tm_year+TM_CENTURY))? 29 : monthdays[mon]);
}

extern int
monthseconds(t)
	Tick t;
{
	int mon;
	struct tm tm;
	
	tm = *localtime(&t);
	mon = tm.tm_mon;
	return(((mon==1) && leapyr(tm.tm_year+TM_CENTURY))? 29*daysec : monthsecs[mon]);
}

extern int
dom(t)
	Tick t;
{
	struct tm *tm;
	tm = localtime(&t);
	return(tm->tm_mday);
}

extern int
wom(t)
	Tick t;
{
	struct tm *tm;
	tm = localtime(&t);
	return((12 + tm->tm_mday - tm->tm_wday)/7);
}

/*
 * If the result falls beyond the system limit, EOT is returned.
 */
Tick
next_nmonth(t, n)
	Tick t;
	int n;
{
	struct tm tm;
	int	n12;

	n12 = n/12;
	n = n%12;

	tm = *localtime(&t);
	tm.tm_hour=0;
	tm.tm_min=0;
	tm.tm_sec=0;
	tm.tm_mday=1;
	if (n12 > 0)
		tm.tm_year += n12;

	if ((tm.tm_mon = tm.tm_mon + n) > 11) {
		tm.tm_mon -= 12;
		tm.tm_year++;
	}

	if (tm.tm_year >= 138)
		return(EOT);

#ifdef SVR4
	tm.tm_isdst = -1;
	return(mktime(&tm));
#else
	return(timelocal(&tm));
#endif "SVR4"
}

extern Tick
nextmonth(t)
	Tick t;
{
	return(next_nmonth(t, 1));
}

extern Boolean
weekofmonth(t, wk)
	Tick t;
	int *wk;
{
	struct tm tm, tm1, tm2;
	Tick	firstday;

	firstday = previousmonth(next_nmonth(t, 1));
	tm = *localtime(&firstday);
	tm1 = *localtime(&t);

	*wk = wom(t);
	if (tm1.tm_wday < tm.tm_wday)
		(*wk)--;

	if (*wk < 4)
		return false;
	else {
		t += seconds(7, daysec);
		tm2 = *localtime(&t);

		return((tm1.tm_mon == tm2.tm_mon) ? false : true);
	}
}

extern int
dow(t)
	Tick t;
{
	struct tm tm;
	tm = *localtime(&t);
	return(tm.tm_wday);
}

extern int /* find dow(0-6) that 1st dom falls on */
fdom(t)
	Tick t;  
{
	struct tm tm;
	tm		= *localtime(&t);
	tm.tm_mday	= 1;
#ifdef SVR4
	tm.tm_isdst	= -1;
	t               = mktime(&tm);
#else
	t               = timelocal(&tm);
#endif "SVR4"
	tm		= *localtime(&t);
	return(tm.tm_wday);
}

extern int
ldom(t) /* find dow(0-6) that last dom falls on */
	Tick t;
{
	struct tm tm;
	tm		= *localtime(&t);
	tm.tm_mday	= monthlength(t);
#ifdef SVR4
	tm.tm_isdst	= -1;
	t               = mktime(&tm);
#else
	t		= timelocal(&tm);
#endif "SVR4"
	tm		= *localtime(&t);
	return(tm.tm_wday);
}

/* returns tick of last day of month */
extern Tick
last_dom(tick)
        Tick tick;
{
        return(upperbound(next_ndays(tick, monthlength(tick) - dom(tick))));
}

/* returns tick of first day of month */
extern Tick
first_dom(tick)
        Tick tick;
{
        return(lowerbound(last_ndays(tick, dom(tick)-1)));
}

/* returns tick of first day of week */
extern Tick
first_dow(tick)
        Tick tick;
{
	int d;

	if ((d = dow(tick)) == 0)
		d = 6;
	else 
		d--;

        return(lowerbound(last_ndays(tick, d)));
}

/* returns tick of first day of week */
extern Tick
last_dow(tick)
        Tick tick;
{
	int d;

	if ((d = dow(tick)) == 0)
		d = 6;
	else 
		d--;
	d = 6 - d;

        return(upperbound(next_ndays(tick, d)));
}
/* returns number of weeks in month */
extern int
numwks(tick)  
        Tick tick;
{
        return (wom(last_dom(tick)));
}

extern int
adjust_dst(start, next)
	Tick start, next; 
{
	DSTchange change;
	change = dst_changed(start, next);
	switch(change) {
		case nochange:
		break;
		case dstoff:
			next+=(int) hrsec;
		break;
		case dston:
			next-=(int) hrsec;
		break;
	}
	return(next);
}

extern Tick
next_nhours(t, n)
	Tick t; int n;
{
	Tick next;
	struct tm tm;
        tm              = *localtime(&t);
        tm.tm_sec       = 0;
        tm.tm_min       = 0;
 
#ifdef SVR4
	next            = mktime(&tm);
#else
        next            = timelocal(&tm);
#endif SVR4"
	next		= next + seconds(n, hrsec);
	next		= adjust_dst(t, next);
	return(next);
}

extern Tick
last_ndays(t, n)
	Tick t; int n;
{
	Tick last;
	struct tm tm;

	tm		= *localtime(&t);
	tm.tm_sec	= 0;
	tm.tm_min	= 0;
	tm.tm_hour	= 0;

#ifdef SVR4
	last            = mktime(&tm);
#else
	last		= timelocal(&tm);
#endif SVR4
	last		= last - seconds(n, daysec);
	last		= adjust_dst(t, last);
	return(last);
}

extern Tick
next_ndays(t, n)
	Tick t; int n;
{
	Tick next;
	struct tm tm;

	tm		= *localtime(&t);
	tm.tm_sec	= 0;
	tm.tm_min	= 0;
	tm.tm_hour	= 0;

#ifdef SVR4
	next            = mktime(&tm);
#else
	next		= timelocal(&tm);
#endif "SVR4"
	next		= next + seconds(n, daysec);
	next		= adjust_dst(t, next);
	return(next);
}

extern Tick
nextday(t)
	Tick t;
{
	Tick next;

	next	= t +(int)daysec;
	next	= adjust_dst(t, next);
	return(next);
}

extern Tick
prevday(t)
	Tick t;
{
	Tick prev;

	prev	= t - (int)daysec;
	prev	= adjust_dst(t, prev);
	return(prev);
}

extern Tick
nextweek(t)
	Tick t;
{
	Tick next;

	next	= t + seconds(7, daysec);
	next	= adjust_dst(t, next);
	return(next);
}

extern Tick
prevweek(t)
	Tick t;
{
	Tick prev;

	prev	= t - seconds(7, daysec);
	prev	= adjust_dst(t, prev);
	return(prev);
}

extern Tick
next2weeks(t)
	Tick t;
{
	Tick next;

	next	= t + seconds(14, daysec);
	next	= adjust_dst(t, next);
	return(next);
}

extern Tick
prev2weeks(t)
	Tick t;
{
	Tick prev;

	prev	= t - seconds(14, daysec);
	prev	= adjust_dst(t, prev);
	return(prev);
}

/*		WORK OUT A BETTER WAY TO DO THIS!!	*/
extern Tick
prevmonth_exactday(t)
	Tick t;
{
	Tick prev; int day;
	struct tm tm;
	int sdelta;

	tm = *localtime(&t);
	sdelta = tm.tm_hour * hrsec + tm.tm_min * minsec + tm.tm_sec; 
	day = tm.tm_mday;
	if((tm.tm_mday < 31 && tm.tm_mon != 0) ||	/* at least 30 days everywhere, except Feb.*/
 	   (tm.tm_mday==31 && tm.tm_mon==6)    ||	/* two 31s -- Jul./Aug.		*/
	   (tm.tm_mday==31 && tm.tm_mon==11)   ||	/* two 31s -- Dec./Jan.		*/
	   (tm.tm_mon == 0 &&(tm.tm_mday < 29  ||(tm.tm_mday==29 && leapyr(tm.tm_year+TM_CENTURY))))) {	
		prev = t-monthseconds(previousmonth(t));
		prev = adjust_dst(t, prev);
	}
	else {  /* brute force */
		prev = previousmonth(previousmonth(t));		/* hop over the month */
		tm = *localtime(&prev);
		tm.tm_mday = day;
#ifdef SVR4
		tm.tm_isdst = -1;
		prev =(mktime(&tm)) + sdelta;
#else
		prev =(timelocal(&tm)) + sdelta;
#endif "SVR4"

	}
	return(prev);
}

/*		WORK OUT A BETTER WAY TO DO THIS!!	*/
extern Tick
nextmonth_exactday(t)
	Tick t;
{
	Tick next; int day;
	struct tm tm;
	int sdelta;

	tm = *localtime(&t);
	sdelta = tm.tm_hour * hrsec + tm.tm_min * minsec + tm.tm_sec; 
	day = tm.tm_mday;
	if((tm.tm_mday < 31 && tm.tm_mon != 0) ||	/* at least 30 days everywhere, except Feb.*/
 	   (tm.tm_mday==31 && tm.tm_mon==6)    ||	/* two 31s -- Jul./Aug.		*/
	   (tm.tm_mday==31 && tm.tm_mon==11)   ||	/* two 31s -- Dec./Jan.		*/
	   (tm.tm_mon == 0 &&(tm.tm_mday < 29  ||(tm.tm_mday==29 && leapyr(tm.tm_year+TM_CENTURY))))) {	
		next = t+monthseconds(t);
		next = adjust_dst(t, next);
	}
	else {  /* brute force */
		next = next_nmonth(t, 2);		/* hop over the month */
		tm = *localtime(&next);
		tm.tm_mday = day;
#ifdef SVR4
		tm.tm_isdst = -1;
		next = mktime(&tm) + sdelta;
#else
		next =(timelocal(&tm)) + sdelta;
#endif "SVR4"
	}
	return(next);
}

extern Tick
nextnyear(t, n)
	Tick t; int n;
{
	struct tm tm;

	tm	= *localtime(&t);
	tm.tm_year += n;
#ifdef SVR4
	return(mktime(&tm));
#else
	return(timelocal(&tm));
#endif "SVR4"
}

extern Tick
nextyear(t)
	Tick t;
{
	return(nextnyear(t, 1));
}

extern Tick
prevnyear(t, n)
	Tick t; int n;
{
	struct tm tm;

	tm = *localtime(&t);
	tm.tm_year -= n;
#ifdef SVR4
	return(mktime(&tm));
#else
	return(timelocal(&tm));
#endif "SVR4"
}

extern Tick
prevyear(t)
	Tick t;
{
	return(prevnyear(t, 1));
}

extern Tick
previousmonth(t)
	Tick t;
{
	struct tm tm;

	tm = *localtime(&t);
	tm.tm_hour=0;
	tm.tm_min=0;
	tm.tm_sec=0;
	if(tm.tm_mon==0) {
		tm.tm_mon=11;
		tm.tm_mday=1;
		tm.tm_year--;
	}
	else {
		tm.tm_mday=1;
		tm.tm_mon--;
	}
#ifdef SVR4
	tm.tm_isdst = -1;
	return(mktime(&tm));
#else
	return(timelocal(&tm));
#endif "SVR4"
}

Tick
prev_nmonth(t, n)
	Tick t;
	int n;
{
	struct tm tm;
	int	n12;

	n12 = n/12;
	n = n%12;

	tm = *localtime(&t);
	tm.tm_hour=0;
	tm.tm_min=0;
	tm.tm_sec=0;
	tm.tm_mday=1;
	if (n12 > 0)
		tm.tm_year -= n12;

	if ((tm.tm_mon = tm.tm_mon - n) < 0) {
		tm.tm_mon += 12;
		tm.tm_year--;
	}
#ifdef SVR4
	tm.tm_isdst = -1;
	return(mktime(&tm));
#else
	return(timelocal(&tm));
#endif "SVR4"
}

extern Tick
lastnthweekday(t, nth, ntimes)
	Tick t; int nth; int ntimes;
{
	struct tm tm1, tm2;
	Tick tick, ntick;
	int delta;
	int sdelta;

	/*
	 * if nth is not specified, assume it's the
	 * 4th week for the ambiguous case.
	 */
	if (nth == 0) {
		weekofmonth(t, &nth);
		if (nth > 4)
			nth = -1;
	}

	tm1 = *localtime(&t);
	sdelta = tm1.tm_hour * hrsec + tm1.tm_min * minsec + tm1.tm_sec; 

	if (nth > 0) {
		if ((tick = next_nmonth(t, ntimes)) == EOT)
			return(EOT);

		tm2 = *localtime(&tick);

		delta = tm1.tm_wday - tm2.tm_wday;
		if (delta < 0)
			delta += 7;

		ntick = tick + seconds((nth - 1) * 7 + delta, daysec) + sdelta;
	} else {
		if ((tick = next_nmonth(t, ntimes + 1)) == EOT)
			return(EOT);

		tm2 = *localtime(&tick);

		delta = tm2.tm_wday - tm1.tm_wday;
		if (tm1.tm_wday >= tm2.tm_wday)
			delta += 7;

		ntick = tick - seconds(delta, daysec) + sdelta;
	}
	ntick = adjust_dst(tick, ntick);

	return (ntick);
}

extern Tick
nextnthweekday(t, nth)
	Tick t; int nth;
{
	struct tm tm1, tm2;
	Tick tick, ntick;
	int delta;
	int sdelta;

	/*
	 * if nth is not specified, assume it's the
	 * 4th week for the ambiguous case.
	 */
	if (nth == 0) {
		weekofmonth(t, &nth);
		if (nth > 4)
			nth = -1;
	}

	tm1 = *localtime(&t);
	sdelta = tm1.tm_hour * hrsec + tm1.tm_min * minsec + tm1.tm_sec; 

	if (nth > 0) {
		tick = next_nmonth(t, 1);
		tm2 = *localtime(&tick);

		delta = tm1.tm_wday - tm2.tm_wday;
		if (delta < 0)
			delta += 7;

		ntick = tick + seconds((nth - 1) * 7 + delta, daysec) + sdelta;
	} else {
		tick = next_nmonth(t, 2);
		tm2 = *localtime(&tick);

		delta = tm2.tm_wday - tm1.tm_wday;
		if (tm1.tm_wday >= tm2.tm_wday)
			delta += 7;

		ntick = tick - seconds(delta, daysec) + sdelta;
	}
	ntick = adjust_dst(tick, ntick);

	return (ntick);
}

extern Tick
prevnthweekday(t, nth)
	Tick t; int nth;
{
	struct tm tm1, tm2;
	Tick tick, ptick;
	int delta;
	int sdelta;

	/*
	 * if nth is not specified, assume it's the
	 * 4th week for the ambiguous case.
	 */
	if (nth == 0) {
		weekofmonth(t, &nth);
		if (nth > 4)
			nth = -1;
	}

	tm1 = *localtime(&t);
	sdelta = tm1.tm_hour * hrsec + tm1.tm_min * minsec + tm1.tm_sec; 

	if (nth > 0) {
		tick = previousmonth(t);
		tm2 = *localtime(&tick);

		delta = tm1.tm_wday - tm2.tm_wday;
		if (delta < 0)
			delta += 7;

		ptick = tick + seconds((nth - 1) * 7 + delta, daysec) + sdelta;
	} else {
		tick = previousmonth(next_nmonth(t, 1));
		tm2 = *localtime(&tick);

		delta = tm2.tm_wday - tm1.tm_wday;
		if (tm1.tm_wday >= tm2.tm_wday)
			delta += 7;

		ptick = tick - seconds(delta, daysec) + sdelta;
	}
	ptick = adjust_dst(tick, ptick);

	return (ptick);
}

/* use double in this routine to avoid integer overflow
 * in case n is very large.
 */
extern Tick
nextnday_exacttime(t, n)
	Tick t;
	int n;
{
	double next;

	next	= t + seconds_dble((double)n, daysec);
	if (next >= EOT)
		return(EOT);
	else {
		next = adjust_dst(t, (Tick)next);
		return((Tick)next);
	}
}

extern Tick
prevnday_exacttime(t, n)
	Tick t;
	int n;
{
	Tick prev;

	prev	= t - seconds(n, daysec);
	prev	= adjust_dst(t, prev);
	return(prev);
}

/* use double in this routine to avoid integer overflow
 * in case n is very large.
 */
extern Tick
nextnwk_exacttime(t, n)
	Tick t;
	int n;
{
	double next;

	next	= t + seconds_dble(((double)n * 7), daysec);
	if (next >= EOT)
		return(EOT);
	else {
		next = adjust_dst(t, (Tick)next);
		return((Tick)next);
	}
}

extern Tick
prevnwk_exacttime(t, n)
	Tick t;
	int n;
{
	Tick prev;

	prev	= t - seconds(n * 7, daysec);
	prev	= adjust_dst(t, prev);
	return(prev);
}

extern Tick
nextnmth_exactday(t, n)
	Tick t;
	int n;
{
	struct tm tm1, tm2;
	Boolean done = false;
	Tick next;

	tm1 = *localtime(&t);
	while (!done) {
		if ((next = next_nmonth(t, n)) == EOT)
			return(EOT);

		tm2 = *localtime(&next);

		/* 1. at least 30 days except feb
		 * 2. 2/29 on leap year
		 * 3. 31st on the appropriate month
		 */
		if ((tm1.tm_mday < 31 && tm2.tm_mon != 1) ||
		    (tm2.tm_mon == 1 && (tm1.tm_mday < 29 ||
		      (tm1.tm_mday == 29 && leapyr(tm2.tm_year+TM_CENTURY)))) ||
		    (tm1.tm_mday == 31 && ((tm2.tm_mon > 6 && tm2.tm_mon % 2) ||
		      ((tm2.tm_mon <= 6 && (tm2.tm_mon % 2 == 0)))))) {
			tm2.tm_sec = tm1.tm_sec;
			tm2.tm_min = tm1.tm_min;
			tm2.tm_hour = tm1.tm_hour;
			tm2.tm_mday = tm1.tm_mday;
			done = true;
		} else
			t = next;
	}

#ifdef SVR4
	tm2.tm_isdst = -1;
	next = mktime(&tm2);
#else
	next = (timelocal(&tm2));
#endif
	return(next);
}

extern Tick
prevnmth_exactday(t, n)
	Tick t;
	int n;
{
	struct tm tm1, tm2;
	Boolean done = false;
	Tick prev;

	tm1 = *localtime(&t);
	while (!done) {
		prev = prev_nmonth(t, n);
		tm2 = *localtime(&prev);

		if ((tm1.tm_mday < 30 && tm2.tm_mon != 1) ||
		    (tm2.tm_mon == 1 && (tm1.tm_mday < 29 ||
		      (tm1.tm_mday == 29 && leapyr(tm2.tm_year+TM_CENTURY)))) ||
		    (tm1.tm_mday == 31 && ((tm2.tm_mon > 6 && tm2.tm_mon % 2) ||
		      ((tm2.tm_mon <= 6 && (tm2.tm_mon % 2 == 0)))))) {
			tm2.tm_sec = tm1.tm_sec;
			tm2.tm_min = tm1.tm_min;
			tm2.tm_hour = tm1.tm_hour;
			tm2.tm_mday = tm1.tm_mday;
			done = true;
		} else
			t = prev;
	}

#ifdef SVR4
	tm2.tm_isdst = -1;
	prev = mktime(&tm2);
#else
	prev = (timelocal(&tm2));
#endif
	return(prev);
}

extern Tick
nextmonTofri(t)
	Tick t;
{
	struct tm *tm;
	Tick next;

	tm = localtime(&t);

	if (tm->tm_wday < 5)
		next = t + (int)daysec;
	else
		next = t + (int)daysec * (8 - tm->tm_wday);

	next = adjust_dst(t, next);
	return(next);
}

extern Tick
prevmonTofri(t)
	Tick t;
{
	struct tm *tm;
	Tick prev;

	tm = localtime(&t);

	if (tm->tm_wday > 1)
		prev = t - (int)daysec;
	else
		prev = t - (int)daysec * (2 + tm->tm_wday);

	prev = adjust_dst(t, prev);
	return(prev);
}

extern Tick
nextmonwedfri(t)
	Tick t;
{
	struct tm *tm;
	Tick next;

	tm = localtime(&t);

	if (tm->tm_wday == 5)
		next = t + (int)daysec * 3;
	else if (tm->tm_wday % 2 || tm->tm_wday == 6)
		next = t + (int)daysec * 2;
	else
		next = t + (int)daysec;

	next = adjust_dst(t, next);
	return(next);
}

extern Tick
prevmonwedfri(t)
	Tick t;
{
	struct tm *tm;
	Tick prev;

	tm = localtime(&t);

	if (tm->tm_wday == 1)
		prev = t - (int)daysec * 3;
	else if (tm->tm_wday % 2 || tm->tm_wday == 0)
		prev = t - (int)daysec * 2;
	else
		prev = t - (int)daysec;

	prev = adjust_dst(t, prev);
	return(prev);
}

extern Tick
nexttuethur(t)
	Tick t;
{
	struct tm *tm;
	Tick next;

	tm = localtime(&t);

	if (tm->tm_wday < 4) {
		if (tm->tm_wday % 2)
			next = t + (int)daysec;
		else
			next = t + (int)daysec * 2;
	} else
		next = t + (int)daysec * (9 - tm->tm_wday);

	next = adjust_dst(t, next);
	return(next);
}

extern Tick
prevtuethur(t)
	Tick t;
{
	struct tm *tm;
	Tick prev;

	tm = localtime(&t);

	if (tm->tm_wday > 2) {
		if (tm->tm_wday % 2)
			prev = t - (int)daysec;
		else
			prev = t - (int)daysec * 2;
	} else
		prev = t - (int)daysec * (3 + tm->tm_wday);

	prev = adjust_dst(t, prev);
	return(prev);
}

/*
 * the 7-bit mask should be put in the last 7 bits of the int
 */
extern Tick
nextdaysofweek(t, weekmask)
	Tick t;
	int weekmask;
{
	unsigned int doublemask;
	struct tm *tm; 
	int i, ndays, daymask;
	Tick next;

	doublemask = weekmask | (weekmask << 7);
	tm = localtime(&t);
	daymask = weekdaymasks[tm->tm_wday] << 1;

	for (i = 0, ndays = 1; i < 7; i++) {
		if (daymask & doublemask)
			break;
		else {
			ndays++;
			doublemask >>= 1;
		}
	}

	next = t + (int)daysec * ndays;
	next = adjust_dst(t, next);
	return(next);
}

extern Tick
prevdaysofweek(t, weekmask)
	Tick t;
	int weekmask;
{
	unsigned int doublemask, daymask;
	struct tm *tm; 
	int i, ndays;
	Tick prev;

	doublemask = weekmask | (weekmask << 7);
	tm = localtime(&t);
	daymask = weekdaymasks[tm->tm_wday] << 6; 

	for (i = 0, ndays = 1; i < 7; i++) {
		if (daymask & doublemask)
			break;
		else {
			ndays++;
			doublemask <<= 1;
		}
	}

	prev = t - (int)daysec * ndays;
	prev = adjust_dst(t, prev);
	return(prev);
}

extern Tick
jan1(t)
	Tick t;
{
	struct tm tm;
	tm		= *localtime(&t);
	tm.tm_mon	= 0;
	tm.tm_mday	= 1;
#ifdef SVR4
	tm.tm_isdst = -1;
	return(mktime(&tm));
#else
	return(timelocal(&tm));
#endif "SVR4"
}

extern Tick
nextjan1(t)
	Tick t;
{
	struct tm tm;
	tm		= *localtime(&t);
	tm.tm_mon	= 0;
	tm.tm_mday	= 1;
	tm.tm_year++;	 
#ifdef SVR4
	tm.tm_isdst = -1;
	return(mktime(&tm));
#else
	return(timelocal(&tm));
#endif "SVR4"
}

extern Tick
lastjan1(t)
	Tick t;
{
	struct tm tm;
	tm		= *localtime(&t);
	tm.tm_mon	= 0;
	tm.tm_mday	= 1;
	tm.tm_year--;	 
#ifdef SVR4
	tm.tm_isdst = -1;
	return(mktime(&tm));
#else
	return(timelocal(&tm));
#endif "SVR4"
}

extern Tick
lowerbound(t)
	Tick t;
{
	struct tm tm;
	tm		=  *localtime(&t);
	tm.tm_sec	=  0;
	tm.tm_min	=  0;
	tm.tm_hour	=  0;
#ifdef SVR4
	return(mktime(&tm)-1);
#else
	return(timelocal(&tm)-1);
#endif "SVR4"
}
extern Tick
lower_bound(i, t)
        int i; Tick t;
{
        struct tm tm;
        tm              =  *localtime (&t);
        tm.tm_sec       =  0;
        tm.tm_min       =  0;
        tm.tm_hour      =  i;
#ifdef SVR4
	return(mktime(&tm)-1);
#else
        return (timelocal(&tm)-1);
#endif "SVR4"
}
extern Tick
upperbound(t)
        Tick t;
{
        struct tm tm;
        tm              =  *localtime(&t);
        tm.tm_sec       =  0;
        tm.tm_min       =  0;
        tm.tm_hour      =  24;
#ifdef SVR4
	tm.tm_isdst = -1;
	return(mktime(&tm)-1);
#else
        return(timelocal(&tm)-1);
#endif "SVR4"
}

static int
leapsB4(y)
	int y;
{
	return((y-1)/4 -(y-1)/100 +(y-1)/400);
}

extern Tick
xytoclock(x, y, t)
	int x, y; Tick t;
{
	int dd, mn, yr, ly, leaps;
	char buf[10];
	struct tm tm;

	tm	= *localtime(&t);
	mn	= tm.tm_mon + 1;
	yr	= tm.tm_year + 1900;
	leaps	= leapsB4(yr);
	/* To fix bug #1175511, the 2nd addition of 1900 to the year is wrong
	 * and should not be done. It was causing, for the year 2000, to 
	 * be seen as 3900, which is NOT a leap year! so ly was being evaluated
	 * incorrectly.
	 */
	ly	= leapyr(yr);

	dd = 7*(y-1) + x - 
	 (yr+leaps+3055L*(mn+2)/100-84-(mn>2)*(2-ly))%7;
	(void) sprintf(buf, "%2d/%2d/%02d", mn, dd, (tm.tm_year % 100));
	return(cm_getdate(buf, NULL));
}

extern Tick
now()
{
	return(time(0));
}

/*
 * Given a time, calculate the closest instance whose
 * tick is later than the time.
 * If the calculated tick does not pass timeok(), ftick is
 * returned and ordinal set to 1.
 */
extern Tick
closest_tick(target, ftick, period, ordinal)
	Tick target; Tick ftick; Period period; int *ordinal;
{
	Tick ctick;
	int delta = 0;
	int remainder = 0;
	int ndays;
	struct tm *tm;
	struct tm tm1, tm2;

	if (target <= ftick) {
		*ordinal = 1;
		return(ftick);
	}

	if (period.period < monthly || period.period == everyNthDay ||
	    period.period == everyNthWeek) {
		tm1 = *localtime(&ftick);
		tm2 = *localtime(&target);
	}
	switch(period.period) {
	case daily:
		delta = (target - ftick) / daysec;
		remainder = target - ftick - daysec * delta;
		if (tm1.tm_isdst == 1 && tm1.tm_isdst != tm2.tm_isdst)
			remainder -= hrsec;
		*ordinal = delta + (remainder>0?1:0) + 1;
		ctick = nextnday_exacttime(ftick, *ordinal - 1);
		break;
	case weekly:
		delta = (target - ftick) / wksec;
		remainder = target - ftick - wksec * delta;
		if (tm1.tm_isdst == 1 && tm1.tm_isdst != tm2.tm_isdst)
			remainder -= hrsec;
		*ordinal = delta + (remainder>0?1:0) + 1;
		ctick = nextnwk_exacttime(ftick, *ordinal - 1);
		break;
	case biweekly:
		delta = (target - ftick) / (wksec * 2);
		remainder = target - ftick - wksec * 2 * delta;
		if (tm1.tm_isdst == 1 && tm1.tm_isdst != tm2.tm_isdst)
			remainder -= hrsec;
		*ordinal = delta + (remainder>0?1:0) + 1;
		ctick = nextnwk_exacttime(ftick, 2 * (*ordinal - 1));
		break;
	case monthly:
		tm = localtime(&ftick);
		/*
		 * Calculate the closest tick only if the date
		 * is < 29; otherwise just return the first tick.
		 * Use 32 to take care of dst time difference.
		 * Without dst, we can use 31.
		 */
		if (tm->tm_mday < 29) {
			delta = (target - ftick) / (daysec * 32);
			remainder = target - ftick - (daysec * 32) * delta;
			*ordinal = delta + (remainder>0?1:0) + 1;
			ctick = nextnmth_exactday(ftick, *ordinal - 1);
		} else {
			ctick = ftick;
			*ordinal = 1;
		}
		break;
	case yearly:
		tm = localtime(&ftick);
		if (tm->tm_mday == 29 && tm->tm_mon == 1) {
			delta = (target - ftick) / (yrsec * 4 + daysec);
			remainder = target - ftick - (yrsec * 4) * delta;
			*ordinal = delta + (remainder>0?1:0) + 1;
			ctick = nextnyear(ftick, (*ordinal - 1) * 4);
		} else {
			delta = (target - ftick) / yrsec;
			/* adjustment for leap year */
			remainder = tm->tm_year % 4;
			if (remainder == 0 || (remainder + delta) > 3)
				delta--;
			remainder = target - ftick - yrsec * delta;
			*ordinal = delta + (remainder>0?1:0) + 1;
			ctick = nextnyear(ftick, *ordinal - 1);
		}
		break;
	case nthWeekday:
		/* 36 is 5 weeks ==> maximum interval between 2 instances */
		delta = (target - ftick) / (daysec * 36);
		remainder = target - ftick - (daysec * 36) * delta;
		*ordinal = delta + (remainder>0?1:0) + 1;
		ctick = lastnthweekday(ftick, period.nth, *ordinal - 1);
		break;
	case everyNthDay:
		delta = (target - ftick) / (daysec * period.nth);
		remainder = target - ftick - (daysec * period.nth) * delta;
		if (tm1.tm_isdst == 1 && tm1.tm_isdst != tm2.tm_isdst)
			remainder -= hrsec;
		*ordinal = delta + (remainder>0?1:0) + 1;
		ctick = nextnday_exacttime(ftick,
				period.nth * (*ordinal - 1));
		break;
	case everyNthWeek:
		delta = (target - ftick) / (wksec * period.nth);
		remainder = target - ftick - (wksec * period.nth) * delta;
		if (tm1.tm_isdst == 1 && tm1.tm_isdst != tm2.tm_isdst)
			remainder -= hrsec;
		*ordinal = delta + (remainder>0?1:0) + 1;
		ctick = nextnwk_exacttime(ftick,
				period.nth * (*ordinal - 1));
		break;
	case everyNthMonth:
		tm = localtime(&ftick);
		if (tm->tm_mday < 29) {
			delta = (target - ftick) / (daysec * 32 * period.nth);
			remainder = target-ftick-(daysec*32*period.nth)*delta;
			*ordinal = delta + (remainder>0?1:0) + 1;
			ctick = nextnmth_exactday(ftick,
					period.nth * (*ordinal - 1));
		} else {
			ctick = ftick;
			*ordinal = 1;
		}
		break;
	case monThruFri:
	case monWedFri:
	case tueThur:
	case daysOfWeek:
		delta = (target - ftick) / wksec;
		tm = localtime(&ftick);

		switch (period.period) {
		case monThruFri:
			*ordinal = delta * 5 + 6 - tm->tm_wday;
			break;
		case monWedFri:
			*ordinal = delta * 3 + (7 - tm->tm_wday) / 2;
			break;
		case tueThur:
			*ordinal = delta * 2 + ((tm->tm_wday == 2) ? 2 : 1);
			break;
		case daysOfWeek:
			*ordinal = delta * ntimes_this_week((u_int)period.nth,0)
					+ ntimes_this_week((u_int)period.nth,
						tm->tm_wday);
		}

		/* delta*daysperweek+(lastapptofweek-firstday in first week) */
		if (period.period == daysOfWeek) {
			ndays = delta * 7 +
				lastapptofweek((u_int)period.nth) - tm->tm_wday;
			ctick = ftick + seconds(ndays, daysec);
		} else if (period.period == tueThur) {
			ndays = delta * 7 + 4 - tm->tm_wday;
			ctick = ftick + seconds(ndays, daysec);
		} else {
			ndays = delta * 7 + 5 - tm->tm_wday;
			ctick = ftick + seconds(ndays, daysec);
		}

		if (ctick > target) { /* need to go back 1 week */
			ndays -= 7;
			if (ndays < 0) {
				*ordinal = 1;
				ctick = ftick;
			} else {
				if (period.period == monThruFri)
					*ordinal -= 5;
				else if (period.period == monWedFri)
					*ordinal -= 3;
				else if (period.period == tueThur)
					*ordinal -= 2;
				ctick -= seconds(7, daysec);
			}
		}
		ctick = adjust_dst(ftick, ctick);
		break;
	default:
		*ordinal = 1;
		ctick = ftick;
	}

	totalskipped += delta;

	if (timeok(ctick))
		return(ctick);
	else {
		*ordinal = 1;
		return(ftick);
	}
}

/*
 * Calculate the tick of the last instance of a repeating event.
 * If the calculated tick does not pass timeok(), EOT is returned.
 */
extern Tick
last_tick(ftick, period, ntimes)
        Tick ftick; Period period; int ntimes;
{
	struct tm *tm;
	double dltick;
	Tick ltick;
	int i;

	if (ntimes >= CMFOREVER)
		return(EOT);

	if (period.enddate != 0)
		return(period.enddate);

	switch(period.period) {
	case weekly:
		ltick = nextnwk_exacttime(ftick, ntimes - 1);
		break;
	case biweekly:
		/* 2 * (ntimes-1) won't overflow an integer since
		 * we make sure ntimes is < CMFOREVER
		 */
		ltick = nextnwk_exacttime(ftick, 2 * (ntimes - 1));
		break;
	case daily:
		ltick = nextnday_exacttime(ftick, ntimes - 1);
		break;
	case monthly:
		tm = localtime(&ftick);
		/*
		 * calculate the last tick only if the date
		 * is < 29; otherwise return EOT to force calculation
		 */
		if (tm->tm_mday < 29)
			ltick = nextnmth_exactday(ftick, ntimes - 1);
		else
			ltick = EOT;
		break;
	case yearly:
		/* 2038 is the last year that can be represented.
		 * this check is to prevent (ntimes-1)*4 from integer overflow
		 */
		if (ntimes > 2038)
			ltick = EOT;
		else {
			tm = localtime(&ftick);
			if (tm->tm_mday == 29 && tm->tm_mon == 1)
				ltick = nextnyear(ftick, (ntimes - 1) * 4);
			else
				ltick = nextnyear(ftick, ntimes - 1);
		}
		break;
	case nthWeekday:
		ltick = lastnthweekday(ftick, period.nth, ntimes - 1);
		break;
	case everyNthDay:
		ltick = nextnday_exacttime(ftick, period.nth * 
			(((ntimes+(period.nth-1))/period.nth) - 1));
		break;
	case everyNthWeek:
		ltick = nextnwk_exacttime(ftick, period.nth *
			(((ntimes+(period.nth-1))/period.nth) - 1));
		break;
	case everyNthMonth:
		tm = localtime(&ftick);
		if (tm->tm_mday < 29)
			ltick = nextnmth_exactday(ftick, period.nth *
				(((ntimes+(period.nth-1))/period.nth) -1));
		else
			ltick = EOT;
		break;
	case monThruFri:
	case monWedFri:
	case tueThur:
	case daysOfWeek:
		tm = localtime(&ftick);

		/* (ntimes-1)*daysperweek+(lastapptofweek-fstapptofFstweek) */
		if (period.period == daysOfWeek)
			dltick = ftick +
				seconds_dble((ntimes-1)*(double)7+
				lastapptofweek((u_int)period.nth)-tm->tm_wday,
				daysec);
		else if (period.period == tueThur)
			dltick = ftick +
				seconds_dble((ntimes-1)*(double)7+
				(4 - tm->tm_wday),daysec);
		else
			dltick = ftick+
				seconds_dble((ntimes-1)*(double)7+
				(5 - tm->tm_wday),daysec);

		if (dltick >= EOT)
			ltick = EOT;
		else
			ltick = adjust_dst(ftick, (Tick)dltick);
		break;
	default:
		break;
	}
	if(timeok(ltick))
		return(ltick);
	else
		return(EOT);
}

/*
 * Calculate the tick of next instance.
 * If the calculated tick does not pass timeok(), EOT is returned.
 */
extern Tick
next_tick(tick, period)
        Tick tick; Period period;
{
        Tick next;
	struct tm *tm;

	nexttickcalled++;
        switch(period.period) {
                case weekly:
                        next = nextweek(tick);
                        break;
                case biweekly:
                        next = next2weeks(tick);
                        break;
                case daily:
                        next = nextday(tick);
                        break;
                case monthly:
                        next = nextmonth_exactday(tick);
                        break;
                case yearly:
			tm = localtime(&tick);
			if (tm->tm_mday == 29 && tm->tm_mon == 1)
				next = nextnyear(tick, 4);
			else
                        	next = nextnyear(tick, 1);
                        break;
		case nthWeekday:
			next = nextnthweekday(tick, period.nth);
			break;
		case everyNthDay:
			next = nextnday_exacttime(tick, period.nth);
			break;
		case everyNthWeek:
			next = nextnwk_exacttime(tick, period.nth);
			break;
		case everyNthMonth:
			next = nextnmth_exactday(tick, period.nth);
			break;
		case monThruFri:
			next = nextmonTofri(tick);
			break;
		case monWedFri:
			next = nextmonwedfri(tick);
			break;
		case tueThur:
			next = nexttuethur(tick);
			break;
		case daysOfWeek:
			next = nextdaysofweek(tick, period.nth);
			break;
                default:
                        break;
        }
        if(next != tick && timeok(next)) return(next);
        else return(EOT);
}

/*
 * Calculate the tick of previous instance.
 * If the calculated tick does not pass timeok(), bot-1 is returned.
 */
extern Tick
prev_tick(tick, period)
        Tick tick; Period period;
{
        Tick prev;
	struct tm *tm;

        switch(period.period) {
                case weekly:
                        prev = prevweek(tick);
                        break;
                case biweekly:
                        prev = prev2weeks(tick);
                        break;
                case daily:
                        prev = prevday(tick);
                        break;
                case monthly:
                        prev = prevmonth_exactday(tick);
                        break;
                case yearly:
			tm = localtime(&tick);
			if (tm->tm_mday == 29 && tm->tm_mon == 1)
				prev = prevnyear(tick, 4);
			else
                        	prev = prevnyear(tick, 1);
                        break;
		case nthWeekday:
			prev = prevnthweekday(tick, period.nth);
			break;
		case everyNthDay:
			prev = prevnday_exacttime(tick, period.nth);
			break;
		case everyNthWeek:
			prev = prevnwk_exacttime(tick, period.nth);
			break;
		case everyNthMonth:
			prev = prevnmth_exactday(tick, period.nth);
			break;
		case monThruFri:
			prev = prevmonTofri(tick);
			break;
		case monWedFri:
			prev = prevmonwedfri(tick);
			break;
		case tueThur:
			prev = prevtuethur(tick);
			break;
		case daysOfWeek:
			prev = prevdaysofweek(tick, period.nth);
			break;
                default:
                        break;
        }
        if(prev != tick && timeok(prev)) return(prev);
        else return(bot-1);
}

extern void
set_timezone(tzname)
	char *tzname;
{
	static char tzenv[MAXPATHLEN];

#ifdef SVR4
        /* I don't like using 'system', but this does the right
	 * thing according to tha man pages
	 */
	if (tzname==NULL) system("unset TZ\n");
#else
	if (tzname==NULL) tzsetwall();
#endif "SVR4"

	else {
		sprintf(tzenv, "TZ=%s", tzname);
		putenv(tzenv);
		tzset();
	}
}

extern long
gmt_off()
{
	struct tm tm;
	Tick t;
	static Tick gmt;

#ifdef SVR4
	gmt             = timezone;
#else
	t	= now();
	tm	= *localtime(&t);
	gmt = tm.tm_gmtoff;
#endif "SVR4"
	return(gmt);
}

/* [howes 3/11/94]
 * cm now supports upto end of 2038.  
 */
extern void
init_time()
{
	struct tm tm, gm;
	Tick t, g, gmt;
	t		= now(); 
	tm 		= *localtime(&t);
#ifdef SVR4
	g               = now();
	gm              = *gmtime(&g);
	gmt             = mktime(&gm) - mktime(&tm);
#else
	gmt             = tm.tm_gmtoff;
#endif "SVR4"
	bot		= (Tick) (-gmt);
	tm.tm_sec	=0;
	tm.tm_min	=0;
	tm.tm_hour	=0;
	tm.tm_mday	=1;
	tm.tm_mon	=0;
	tm.tm_year	=138;			/* Jan. 1, 2038 */
	tm.tm_wday	=0;
	tm.tm_yday	=0;
#ifdef SVR4
	tm.tm_isdst = -1;
	eot             =mktime(&tm);
#else
	eot		=timelocal(&tm);
#endif "SVR4"
}

extern int
seconds_to_hours(n, rem)
        int n, *rem;
{
	*rem = n % hrsec;
        return(n/(int)hrsec);
}

extern int
hours_to_seconds(n)
        int n;
{
        return(n *(int)hrsec);
}

extern int
seconds_to_minutes(n)
        int n;
{
        return(n/60);
}

extern int
minutes_to_seconds(n)
        int n;
{
        return(n *(int)minsec);
}


extern int
days_to_seconds(n)
        int n;
{
        return(n *(int)daysec);
}

seconds_to_days(n, rem)
        int n, *rem;
{
	*rem = n % daysec;
        return(n/(int)daysec);
}

/*
extern int
weeks_to_seconds(n)
        int n;
{
        return(n *(int)wksec);
}

extern int
seconds_to_weeks(n)
        int;
{
        return(n/(int)wksec);
}
*/

extern Tick
monthdayyear(m,d,y)
        int m, d, y;
{
        int t;
        char buf[10];
 
        (void) sprintf(buf, "%d/%d/%d", m, d, y);
        t = cm_getdate(buf, NULL);
        return(t);
}
