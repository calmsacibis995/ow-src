/*      static  char sccsid[] = "@(#)ps_graphics.h 3.3 93/02/02 Copyr 1991 Sun Microsystems, Inc.";
 *	ps_graphics.h 
 *
 */

#define PRINTER_WIDTH   612     /*  8.5"  paper width */
#define PRINTER_HEIGHT  792     /* 11.0"  paper height */
#define PRINTER_HMARGIN  18     /*  0.25" (LaserWriter) non-printing edge */
#define PRINTER_VMARGIN   3     /*  0.04" (LaserWRiter) non-printing edge */
#define INCH             72     /* 72 pts = 1 inch */
#define PORTRAIT       TRUE
#define LANDSCAPE     FALSE

extern FILE *ps_open_file 	(/* FILE *fp */);
extern void ps_init_printer 	(/* FILE *fp */);
extern void ps_finish_printer 	(/* FILE *fp */);
extern void ps_print_file 	();

extern void ps_set_font		(/* FILE *fp; char *font_type, *font */);
extern void ps_set_fontsize 	(/* FILE *fp; char *font_type; int size */);
extern void ps_translate 	(/* FILE *fp; int x, y */);
extern void ps_scale 		(/* FILE *fp; int x, y */);
extern void ps_rotate 		(/* FILE *fp; int angle */);
extern void ps_landscape_mode 	(/* FILE *fp */);

extern void ps_init_day 	(/* FILE *fp */);
extern void ps_init_month 	(/* FILE *fp */);
extern void ps_print_header     (/* FILE *fp; char *str */);
extern void ps_day_header 	(/* FILE *fp; int timeslots */);
extern void ps_day_timeslots	(/* FILE *fp; int i */);

extern void ps_month_daynames	(/* FILE *fp; int i */);
extern void ps_month_timeslots	(/* FILE *fp; int i */);
extern void ps_print_time 	(/* FILE *fp; int i */);
extern void ps_print_text 	(/* FILE *fp; int i; char *str */);
extern Boolean ps_print_month_appts 	(/* FILE *fp; Abb_Appt a, int num_page, hi_hour, Glance view */);
extern Boolean ps_print_multi_appts 	(/* FILE *fp; Abb_Appt a, int num_page, hi_hour, Glance view */);
extern void ps_init_std_month 	(/* FILE *fp; int wid, hgt */);

extern void ps_init_std_month 	(/* FILE *fp; int wid, hgt */);
extern void ps_std_year_name 	(/* FILE *fp; int year */);
extern void ps_std_month_name 	(/* FILE *fp; int mon */);
extern void ps_std_month_weekdays (/* FILE *fp; */);
extern void ps_std_month_dates	(/* FILE *fp; int fdom, monlen */);

extern void ps_init_alt_month 	(/* FILE *fp; int wid, hgt */);
extern void ps_alt_year_name 	(/* FILE *fp; int year */);
extern void ps_alt_month_name 	(/* FILE *fp; int mon */);
extern void ps_alt_month_weekdays (/* FILE *fp */);
extern void ps_alt_month_boxes 	(/* FILE *fp */);
extern void ps_alt_month_dates	(/* FILE *fp; int fdom, monlen */);

extern void ps_todo_outline	(/* FILE *fp; Todo t */);
extern void ps_print_todo_months(/* FILE *fp; */);
extern void ps_init_list();
extern void ps_print_todo	(/* FILE *fp; Abb_Appt *a; Event_Type appt_type; Glance glance;  */);

extern void ps_print_little_months();
extern void ps_week_sched_init();
extern void ps_week_sched_draw();

extern void ps_finish_std_month();
extern void ps_finish_alt_month();
extern void ps_finish_std_year();
extern void ps_finish_alt_year();
extern void ps_init_std_year();
extern void ps_init_alt_year();
