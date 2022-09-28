/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_SEGMENT_CANVAS_IMPL_H
#define	_MULTIMEDIA_SEGMENT_CANVAS_IMPL_H

#ident	"@(#)segment_canvas_impl.h	1.25	93/02/11 SMI"

#include <atool_types.h>

/* temporary hack XXX, remove when packaging */
#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <xview/cursor.h>
#include <xview/svrimage.h>
#include <X11/Xlib.h>
#include <xview/cms.h>

#define	PROPER_SPACING		4	/* space between hash marks in pixels */
#define	BIG_HASH_TIME		30 	/* seconds between big hash marks */
#define	SMALL_HASH_TIME		5	/* seconds between small hash marks */

#define	CURSOR_HEIGHT		1	/* height of cursor image */
#define	CURSOR_WIDTH		7	/* width of cursor image */


#define	POINTER_HEIGHT		10	/* height of pointer image */
#define	POINTER_HALFBASE	6	/* half base width of triangle */

#define	DISPLAY_OFFSET		2 * POINTER_HALFBASE	/* starting X offset */

/* XXX should not hardwire this */
#define LABEL_FONT		"-*-lucidatypewriter-medium-r-normal-*-8-*"
#define LABEL_X_OFFSET		2	/* how many pixels from hash mark */
#define LABEL_Y_OFFSET		4	/* how many pixels from canvas btm */

/* XXX yuck, fix all the macros with sp-> in them */
#ifdef DEBUG
#define	SET_STATE(sp, st)	set_state(sp,st)
#else
#define SET_STATE(sp, st)	sp->S = st
#endif

/* convert from pixels to seconds */
#define	X_TO_SECS(sp, x)	((double)(x) / sp->Boxds_pps)

/* convert from seconds to pixel offset, round pixel value to nearest integer */
#define	SECS_TO_X(sp, s)	((int)(floor((s * sp->Boxds_pps) + .5)))

/* internally used routines */
int	*secs_to_pixels();

/* external interface */
void	SegCanvas_INIT(ptr_t spd, ptr_t canvas_id, ptr_t owner_id);
void	SegCanvas_HASHDRAW(ptr_t spd);
void	SegCanvas_HASHCLEAR(ptr_t spd);
void	SegCanvas_GRAPHDRAW(ptr_t spd);
void	SegCanvas_GRAPHCLEAR(ptr_t spd);
void	SegCanvas_DETECT();
void	SegCanvas_CLEAR(ptr_t spd);
int	SegCanvas_GETSELECT();

			
/* types of display point */
typedef enum display_point_type { 
	SEG_SILENT_START, 	/* Start of a silent segment */
	SEG_SOUND_START, 	/* Start of a sound segment */
	SEG_EOF,		/* end of point list */
	SEG_POSITION 		/* a generic position in display */
} Display_point_type;

typedef struct display_point {
	double			secs;	/* real time offset in seconds */
	int			x;	/* relative pixel position on display */
	Display_point_type	type;	/* type of segment point */
} Display_point;

/* State of the selection */
typedef enum state { 
	NOTHING, 		/* No selection, or insert point */
	INSERT, 		/* Insert point has been chosen */
	MAKING_SELECT, 		/* Currently making a selection */
	SELECT_EXISTS 		/* A selection already exists */
} State;

struct segment_canvas_data {
	/* window panel handles */
	ptr_t		segcanvas;		/* canvas id */
	ptr_t		segowner;		/* canvas owner id */

	/* Window object handles */
	Display		*Base_display;	/* Xview handle to display area */
	XID		Boxds_xid;	/* X handle to display area */

	/* Segment Display area parameters */
	int		Boxds_xv_width;	/* XView width of canvas */
	int		Boxds_xv_height;/* XView height of canvas */
	Xv_Window	Boxds_win;	/* Window of segment display area */
	Xv_Cursor	Basic_ptr_cursor;	/* Openlook pointer */
	Server_image	Basic_ptr_im;	/* basic pointer image */
	Xv_Cursor	Boxds_cursor;	/* Cursor of segment display area */
	Server_image	Boxds_cimage;	/* Cursor image of segment display */
	int		Boxds_xstart;	/* X axis starting offset */
	int		Boxds_width;	/* Width of segment display area */
	double		Boxds_secs;	/* length display area is secs */
	int		Boxds_height;	/* Height of segment display area */
	double		Boxds_pps;	/* pixels per second */
	double		Boxds_time_incr;/* display increment in seconds */
					/* defaul is 0 */

	/* Hash mark info */
	int		Hash_on;	/* True if hash marks are on */
	double		Hash_secs;	/* length of audio data to mark */
	int		Big_hash_height;/* Pixel height of tall hash marks */
	int		Hash_height;	/* Pixel height of normal hash marks */

	/* Graph's display region */
	int		Graph_x;	/* x offset into canvas */
	int		Graph_y;	/* y offset into canvas */
	int		Graph_width;	/* width of display region */
	int		Graph_height;	/* height of display region */

	/* X drawing info */
	GC		Clear_gc;	/* graphic context for clearing dest */
	GC		Copy_gc;	/* graphic context for drawing */
	GC		Xor_gc;		/* for Xoring with dest */
	u_long		Base_bg;	/* background color */
	u_long		Base_fg;	/* foreground color */
	XFontStruct	*Label_font;	/* font for labeling hash marks */

	/* X event info */
	int		Multi_click_time; /* time between multiple clicks */

	/* Cursor state and position */
	int		Cursor_active;	/* cursor is active on the canvas */
	int		Cursor_x;	/* position of cursor */

	Display_point	Pointer;	/* position of pointer */
	int		Pointer_active;	/* Pointer is visible on the canvas */
	XPoint		Pointer_pts[3];	/* outline of pointer polygon */
	int		Pointer_npts;	/* number of points */

	/* button states */
	int		left_button_down; /* true if left button is down */
	int		middle_button_down; /* true if middle button is down */

	struct timeval	left_time;	/* time left button was pressed */
	struct timeval	middle_time;	/* time middle button was pressed */

	int		dbl_click_mode;	/* true if double click detected */
	int		ldbl_click;	/* true if left double click detected */
	int		mdbl_click;	/* middle double click detected */
	int		tpl_click;	/* true if triple click detected */

	/* button dragging states */
	int		Drag_select;	/* true if hit select over selection */
	int		dragging;	/* true, dragging with a button down */
	Display_point	insert_pt;	/* cursor position on left down */

	/* Bar graph display info */
	int		Graph_active;	/* true when segments are visible */
	int		Half_bar_height;/* half Pixel height of bars */
	int		Bar_yorigin;	/* y pixel position to draw bar at */

	State		S;		/* selection state */
	int		adjusting_right;/* right side selection adjustment */

	Display_point	Ext_left;	/* Left Extension point of selection */
	Display_point	Ext_right;	/* Right Extension point of selection */
	Display_point	Anchor;		/* anchor point of animated selection */

	Display_point	Zero_pos;	/* zero point */
	Display_point	*Pts;		/* segment point list for graph */
	int		Pts_n;		/* number of elements in list */

	/* Audio data info */
	Display_point		File_len;	/* last point in file */

	/* event call backs */
	int	(*cursor_position_proc)();
	int	(*pointer_position_proc)();
	int	(*insert_proc)();
	int	(*update_select_proc)();
	int	(*done_select_proc)();
};

#endif /* !_MULTIMEDIA_SEGMENT_CANVAS_IMPL_H */
