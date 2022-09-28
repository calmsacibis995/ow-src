/*	static char sccsid[] = "@(#)browser.h 3.12 93/01/20 Copy 1988 Sun Micro; 
	browser.h
*/
#include <xview/cursor.h>

typedef struct {
	Frame           frame;
        Canvas          canvas;
        caddr_t         current_selection;      
	Panel_item	box;
        Panel_item      datetext;
        Panel_item      datefield;
        Tick            date;
        int     	col_sel;
        int     	row_sel;
        Tick     	begin_week_tick;
        int     	canvas_w;
        int     	canvas_h;
        int     	chart_width;
        int     	chart_height;
        int     	boxh;
        int     	boxw;
        int     	chart_x;
        int     	chart_y;
        Tick    	begin_day_tick;
        Tick    	end_day_tick;
        Tick    	begin_hr_tick;
        Tick    	end_hr_tick;
	XContext        *xcontext;
	char	 	*multi_array;
	int		segs_in_array;
        Server_image 	busy_icon;
	Boolean		add_to_array;
} Browser;

#define BOX_SEG 4 
#define MINS_IN_SEG (60/BOX_SEG)

extern void mb_draw_chartgrid(), mb_draw_appts(), mb_update_segs();
extern void mb_update_busyicon(), mb_init_blist(), mb_init_canvas();
extern void mb_init_browser(), mb_deregister_names(), mb_refresh_canvas();
extern caddr_t make_browser();
extern Boolean browser_exists(), browser_showing();
extern Notify_value sched_proc();
extern void set_default_reminders(), set_default_scope_plus(), set_default_what(), update_browser_display(), update_browser_display2();
