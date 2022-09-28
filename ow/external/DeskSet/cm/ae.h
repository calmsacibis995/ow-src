/*
 * static  char sccsid[] = "@(#)ae.h 1.12 93/01/14 Copyr 1991 Sun Microsystems, Inc.";
 * ae.h
 */

#ifndef ae_h
#define ae_h

#include "datefield.h"

/* data structure to represent an appointment */
typedef struct miniappt {
	char	*datestr;
	char	*formattedstr;
	int	showstart;
	int	starthr;
	int	startmin;
	int	showstop;
	int	endhr;
	int	endmin;
	int	repeat;
	int	nth;
	int	ntimes;
	char	*what1;
	char	*what2;
	char	*what3;
	char	*what4;
} Miniappt;

/* data structure to hold values from .cm.rc */
typedef struct rcvalues {
	Ordering_Type	order;
	Separator_Type	sep;
	int		hour24;
	int		daybegin;
	int		dayend;
} Rc_value;

typedef enum repeattype {
	onetime = 0,
	daily = 1,
	weekly = 2,
	biweekly =3,
	monthly = 4,
	yearly = 5,
	nthWeekday = 6,
	everyNthDay = 7,
	everyNthWeek = 8,
	everyNthMonth = 9,
	otherPeriod = 10,
	monThruFri = 11,
	monWedFri = 12,
	tueThur = 13,
	daysOfWeek = 14
} Repeat_Type;

#define LASTREPEATVAL	15
#define DEFDAYBEGIN	7
#define DEFDAYEND	19
#define HOURSEC		3600

extern ae_window_objects	*Ae_window;
extern ae_repeat_win_objects	*Ae_repeat_win;

extern int debug;
extern char *file;
extern char *prop_names[];
extern int data_changed;

extern int load_proc(ae_window_objects *, char *, int);
extern int load_data(char *data, int size, int flag);
extern int get_rc_value();

extern Miniappt appttemp;

#endif /* ae_h */
