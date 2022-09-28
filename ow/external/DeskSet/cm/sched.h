/*static  char sccsid[] = "@(#)sched.h 3.1 92/04/03 Copyr 1991 Sun Microsystems, Inc.";*/

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

#define MAXINT 0x7fffffff

/* since 0 is Wed Dec 31, 4 is Sunday */
#define SUNDAY		(4*(60*60*24))
#define MONDAY		(5*(60*60*24))
#define TUESDAY		(6*(60*60*24))
#define WEDNESDAY	(7*(60*60*24))
#define THURSDAY	(8*(60*60*24))
#define FRIDAY		(9*(60*60*24))
#define SATURDAY	(10*(60*60*24))
#define NODAY		0

#define DEFAULT_WORKDAY  9*60	/* 9:00 am */
#define DEFAULT_DIR	".scheddir"
#define  MAXAPPTS 200	/* max number of appointments in one day */

struct appts {
	char	*a_msg;
	int	a_lnth;
	int	a_clock;	/* encodes date */
	int	a_time1;
	int	a_time2;
};

#define INDENTMODE	0
#define BLANKMODE	1
#define SINGLEMODE	2
#define NMODES		3
int	sepmode;		/* how to separate different appointments */
int	beeps;			/* how many times to beep */
int	chartmode;		/* what to do when creating chart */
int	chartbegin;		/* in hours */
int	chartend;		/* in hours */
int	numdirs;

char	*curmenuname;		/* name of cur directory (when numdirs) > 0 */
char	*dir;			/* where the files are stored */
int	beepon;			/* beep when alarm goes off */
int	autoopen;		/* open from iconic when alarm goes off */
int	flashon;		/* flash when alarm goes off */
int	snoozetime;		/* when snooze alarm was pushed */
int	iconday;		/* what day is on the icon */
char	labelstr[];		/* current label on frame */
int	prerange;		/* how long before meeting to ring alarm */
int	postrange;		/* how long past meeting to ring alarm */
char	tmpname[];		/* tmp file to keep textsw_load happy */
int	workday;		/* for am/pm interpretation */
struct	appts	*weeklymeetings;	/* periodic meetings */
struct	appts	*yearlymeetings;	/* periodic meetings */
int	winheight;		/* height of top subwindow */
int	freq;			/* how often to ring bell */
int	displayon;
int	client;			/* for setitimer */
char	*printfilt;
char	*printallfilt;
int	weekstoprint;
int	readonly;
char	**tool_attrs;
int	toolfd;
int	rootfd;
int	debug;
int	newdebug;
char	*monthnames[];
char	*lmonthnames[];
char	*wdaynames[];
char	*lwdaynames[];
/* 
 * Day currently displayed
 */
extern int	dy;
extern int	wdy;
extern int	mnth;
extern int	yr;
extern int	mwdy;			/* the weekday of the day before the first */

char	*index(), *rindex();
char	*makepath();
char	*tmpify();
char	*getdirname();
char	*getmenuname();
int	cmp(), cmp2();
struct	appts	*getappts();
struct	appts	*unionweek();
struct	appts	*unionyear();
