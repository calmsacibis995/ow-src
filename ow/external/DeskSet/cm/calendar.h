/* static  char sccsid[] = "@(#)calendar.h 3.14 92/09/24 Copyr 1991 Sun Microsystems, Inc.";
 * calendar.h 
 *
 */

typedef enum {
	yearGlance, monthGlance, weekGlance, dayGlance
} Glance;

typedef enum {
	main_win, browser, tempbrowser
} WindowType;


typedef struct {
	int		nwks;
	int		boxh;
	int		boxw;
	int		winw;
	int		winh;
	int		outside_margin;
	int		inside_margin;
	int		topoffset;
	long		date;			
	long		olddate;
	Glance		glance;
	caddr_t		day_info;		/* implemented in dayglance.c	*/
	caddr_t		week_info;		/* implemented in weekglance.c	*/
	caddr_t		current_calendar;	/* implemented in calendarA.c	*/
	caddr_t		next_alarm;		/* implemented in appt.c	*/
	caddr_t		current_selection;	/* implemented in select.c	*/
} View;

typedef struct {
	Xv_Font		fixed12;		/* default fixed width12	*/
	Xv_Font		fixed12b;		/* default fixed width12b	*/
	Xv_Font		lucida9;		/* lucida 9			*/
	Xv_Font		lucida10;		/* lucida 10			*/
	Xv_Font		lucida12;		/* lucida 12			*/
	Xv_Font		lucida9b;		/* lucida 9b			*/
	Xv_Font		lucida10b;		/* lucida 10b			*/
	Xv_Font		lucida12b;		/* lucida 12b			*/
	Xv_Font		lucida14b;		/* lucida 14b			*/
} Fonts;

typedef struct {
	Panel		panel;
	Panel_item	button1;		/* unused	*/
	Panel_item	button2;		/* view button  */
	Panel_item	button3;		/* edit button  */
	Panel_item	button4;		/* browse button*/
	Panel_item	button5;		/* print button */
	Panel_item	button6;		/* unused	*/
	Panel_item	button7;		/* unused	*/
	Panel_item	button8;		/* prev button  */
	Panel_item	button9;		/* today button */
	Panel_item	button10;		/* next button  */
} Items;

typedef struct {
	int		rc_ts; 			/* timestamp of rc file: browse */
	int		version; 		/* version of cm  */
	WindowType last_canvas_touched; /* either browser or main */
} General; 


typedef struct {
	Frame		frame;
	Panel		panel;
	Items		*items;
	General		*general;               /* stuff general to the calendar */
	Canvas		canvas;			/* main canvas for month-view & chart	*/
	Icon		icon;			/* little calendar 128 x 64		*/
	Fonts		*fonts;			/* collection of 6 fonts		*/
	View		*view;			/* cached info about current display	*/
	caddr_t		properties;		/* implemented in props.c		*/
	caddr_t		editor;			/* implemented in editor.c		*/
	caddr_t		browser;		/* implemented in browser.c		*/
	caddr_t		browselist;		/* implemented in blist.c
	*/
	caddr_t		todo;			/* implemented in todo.c		*/
	caddr_t		alist;			/* implemented in alist.c		*/
	caddr_t		print_opts;		/* implemented in printer.c		*/
	caddr_t		postup;			/* implemented in postup.c		*/
	caddr_t		user;			/* calendar driver			*/
	char*		calname;		/* calendar name			*/
	XContext	*xcontext;		/* XID, Display, GC, GCVals		*/
	caddr_t 	find; 			/* implemented in find.c */
	caddr_t 	goTo; 			/* implemented in goto.c */
	caddr_t 	tempbr;			/* implemented in tempbr.c */
	caddr_t         mail; 			/* implemented in mail.c */
} Calendar;

	
extern Calendar *calendar;
extern char cm_mailfile[];
extern int report_err (/* char *s */);
extern int yyyerror (/* char *s */);
#define MAXBUFLEN               4096

#define CMS_VERS_2  2
#define CMS_VERS_3  3
#define CMS_VERS_4  4

extern void cache_dims();
extern Boolean in_range();
extern char* cm_get_relname();
extern void make_browse_menu();
