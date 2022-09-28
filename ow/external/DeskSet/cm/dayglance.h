/*static  char sccsid[] = "@(#)dayglance.h 3.2 92/09/24 Copyr 1991 Sun Microsystems, Inc.";*/
/*	static char sccsid[] = "@(#)dayglance.h 3.2 92/09/24 Copy 1989 Sun Micro;  
	dayglance.h 
*/

typedef struct dayglance {
	int mobox_height1;
	int mobox_height2;
	int mobox_height3;
	int month1; 		/* tick in 1st mo */ 
	int month1_y;
	int month2; 		/* tick in 2nd mo */
	int month2_y;
	int month3; 		/* tick in 3rd mo */
	int month3_y;
	int col_w;  		/* width of month box col */
	int row_h;  		/* height of month box row */
	int mobox_width;  	/* width of month box row */
	int day_selected;
	int day_selected_x;
	int day_selected_y;
	int day_selected_x2;
	int day_selected_y2;
} Day;

#define MOBOX_AREA_WIDTH (int)(c->view->winw*.4)
#define APPT_AREA_WIDTH (c->view->winw-MOBOX_AREA_WIDTH-2)

extern void paint_dayview_hour();
extern Stat paint_dayview();
extern void init_dayview();
extern void monthbox_datetoxy();
extern Notify_value day_button();
extern Notify_value ps_day_button();
extern Boolean in_moboxes();
extern void paint_day_header();

#define HRBOX_MARGIN  30
