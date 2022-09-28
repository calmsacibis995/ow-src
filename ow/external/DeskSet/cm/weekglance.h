/* static  char sccsid[] = "@(#)weekglance.h 3.2 92/09/24 Copyr 1991 Sun Microsystems, Inc.";
 *	weekglance.h
 */

typedef struct week {
        int     start_date;
        int     canvas_w;
        int     canvas_h;
        int     width;
        int     height;
        int     label_height;
        int     day_height;
        int     day_width;
        int     x;
        int     y;
        int     begin_hour;
        int     end_hour;
        int     chart_width;
        int     chart_height;
        int     chart_hour_height;
        int     chart_day_width;
        int     chart_x;
        int     chart_y;
        Xv_Font font;
        Xv_Font small_font;
        Xv_Font small_bold_font;
        char    *time_array;
        int     segs_in_array;
        caddr_t current_selection;
        int	add_pixels;
} Week;

extern Notify_value week_button();
extern Notify_value ps_week_button();
extern void wk_update_entries();
extern Stat paint_weekview();
extern void week_event();
extern void format_week_header();
