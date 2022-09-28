/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)segment_xv.c	1.69	93/02/09 SMI"

#include <stdio.h>
#include <values.h>
#include <math.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <xview/cursor.h>
#include <xview/svrimage.h>
#include <X11/Xlib.h>
#include <xview/cms.h>

#include "atool_panel.h"
#include "atool_debug.h"
#include "atool_i18n.h"
#include "atool_sel.h"
#include "segment_canvas_impl.h"

extern INSTANCE;
Attr_attribute SEG_CANVAS_KEY;	/* global key for local segment data */

/* internal functions */
void		draw_canvas_repaint_event(Canvas canvas,
    		                          Xv_window paint_window,
    		                          Display *display,
    		                          Window xid,
    		                          Xv_xrectlist *rects);
Notify_value	draw_canvas_event(Xv_window win,
            	                  Event *event,
            	                  Notify_arg arg,
            	                  Notify_event_type type);

void
SegCanvas_INIT_GRAPH_PARAMS(ptr_t spd)
	       			    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	int				halfbase; /* size of pointer (pixel) */ 
	int				width;	/* of canvas in pixels */ 

	sp = (struct segment_canvas_data *) spd;

	/* save real pixel height and width */
	sp->Boxds_xv_width = (int)xv_get((Xv_opaque)sp->segcanvas, XV_WIDTH);
	sp->Boxds_xv_height = (int)xv_get((Xv_opaque)sp->segcanvas, XV_HEIGHT);

	/* get height and width of bar display area */
	sp->Boxds_height = (int)xv_get((Xv_opaque)sp->segcanvas, XV_HEIGHT) - 2;
	width = (int) xv_get((Xv_opaque)sp->segcanvas, XV_WIDTH);

	/* calculate scale of pointer */
	halfbase = (sp->Boxds_height / 7);

	/* set margin equal to half the pointer base */
	sp->Boxds_xstart = (halfbase > 10) ? halfbase : 10;

	/* calculate width minus left and right margins */
	sp->Boxds_width = width - 2 - (sp->Boxds_xstart * 2);

	sp->Hash_height = sp->Boxds_height / 8;
	sp->Big_hash_height = sp->Hash_height * 2;
	sp->Half_bar_height = sp->Boxds_height / 6;
	
	/* initialize display list of pointer */
	sp->Pointer_pts[0].y = (2 * halfbase);
	sp->Pointer_pts[1].x = - halfbase;
	sp->Pointer_pts[1].y = -(2 * halfbase);
	sp->Pointer_pts[2].x = (2 * halfbase);
	sp->Pointer_pts[2].y = 0;
	sp->Pointer_npts = 3;
}

/*
 * Initialize all the canvas parameters, graphics contexts, selection
 * state, and cursor.
 */
void
SegCanvas_INIT(ptr_t spd, ptr_t canvas_id, ptr_t owner_id)
	       		    		/* segment data pointer */
	       		          	/* xview canvas handle */
	       		         	/* xview handle to canvas owner */
{
	struct segment_canvas_data      *sp;    /* segment data pointer */
	Cms		cms;		/* Color map segment */
	Xv_window	canvas_win;	/* paint window */
	XGCValues	values;		/* X Graphics context */
	unsigned long	gcmask;		/* GC mask */

	sp = (struct segment_canvas_data *) spd;

	/* Init global keys, if necessary */
	if (INSTANCE == 0)
		INSTANCE = xv_unique_key();
	if (SEG_CANVAS_KEY == 0)
		SEG_CANVAS_KEY = xv_unique_key();

	/* get paint window handle */
	canvas_win = canvas_paint_window((Xv_opaque)canvas_id);

	/* save segment data pointer */
	xv_set(canvas_win, XV_KEY_DATA, SEG_CANVAS_KEY, sp, NULL);

	/* pass xid & display handle to repaint event proc */
	xv_set((Xv_opaque)canvas_id, CANVAS_X_PAINT_WINDOW, TRUE, NULL);

	xv_set(canvas_win, WIN_CONSUME_EVENTS,
		WIN_LEFT_KEYS,
		WIN_MOUSE_BUTTONS,
		LOC_WINENTER,
		LOC_WINEXIT,
		LOC_DRAG,
		LOC_MOVE,
		NULL, NULL);

	xv_set((Xv_opaque)canvas_id, CANVAS_REPAINT_PROC, 
		draw_canvas_repaint_event, NULL);

	notify_interpose_event_func(canvas_win,
		(Notify_func) draw_canvas_event, NOTIFY_SAFE);

	SegCanvas_INIT_GRAPH_PARAMS(sp);

	/* X mouse/pointer attribute */
	sp->Multi_click_time = (int)defaults_get_integer("MultiClickTimeout", 
					    "MultiClickTimeout", 4) * 100000;
	
	/* init selection */
	sp->S = NOTHING;
	sp->dbl_click_mode = FALSE;
	sp->ldbl_click = FALSE;
	sp->mdbl_click = FALSE;
	sp->tpl_click = FALSE;

	sp->Ext_left.x = 0;
	sp->Ext_left.secs = 0.0;
	sp->Ext_left.type = SEG_POSITION;

	sp->Ext_right.x = 0;
	sp->Ext_right.secs = 0.0;
	sp->Ext_right.type = SEG_POSITION;

	/* Get handles to display area */
	sp->Base_display = (Display *) xv_get((Xv_opaque)owner_id, XV_DISPLAY);
	sp->Boxds_xid = (XID) xv_get(canvas_win, XV_XID);
	sp->Boxds_win = canvas_win;

	sp->Boxds_pps = 1;

	/* initialize the label font */
	if (!(sp->Label_font = XLoadQueryFont(sp->Base_display, LABEL_FONT))) {
		fprintf(stderr,
		    MGET("Warning: can't load font %s\n")
		    , LABEL_FONT);
	}

	/* set up graphics context */
	cms = (Cms) xv_get((Xv_opaque)canvas_id, WIN_CMS);
	sp->Base_bg = xv_get(cms, CMS_BACKGROUND_PIXEL);
	sp->Base_fg = xv_get(cms, CMS_FOREGROUND_PIXEL);
	
	/* create graphics contexts */
	values.background = sp->Base_bg;
	values.foreground = sp->Base_bg;
	values.function = GXcopy;
	sp->Clear_gc = XCreateGC(sp->Base_display, sp->Boxds_xid,
		GCForeground|GCBackground|GCFunction, &values);

	/* for the copy GC we need to set the font value for painting
	 * the hash mark labels -- only if the font was loaded
	 */
	values.background = sp->Base_bg;
	values.foreground = sp->Base_fg;
	values.function = GXcopy;
	gcmask = GCForeground|GCBackground|GCFunction;

	if (sp->Label_font) {
		values.font = sp->Label_font->fid;
		gcmask |= GCFont;
	}

	sp->Copy_gc = XCreateGC(sp->Base_display, sp->Boxds_xid, gcmask, 
		&values);

	values.background = sp->Base_bg;
	values.foreground = sp->Base_bg ^ sp->Base_fg;
	values.function = GXxor;
	sp->Xor_gc = XCreateGC(sp->Base_display, sp->Boxds_xid,
		GCForeground|GCBackground|GCFunction, &values);

	sp->Boxds_cursor = 0;
	/* initialize the horizontal Xview cursor */
	cursor_init(sp, sp->Boxds_win);

	sp->Pts = NULL;
	sp->Pts_n = 0;

	/* init zero position */
	sp->Zero_pos.x = 0;
	sp->Zero_pos.secs = 0;
	sp->Zero_pos.type = SEG_POSITION;

	/* init file length */
	sp->File_len = sp->Zero_pos;

	/* initialize graphing state */
	SegCanvas_CLEAR(sp);

	sp->Hash_on = TRUE;

	sp->Boxds_time_incr = 30.0; /* default display increment */
}

/*
 *	Clear graph state, and canvas display area.
 */
void
SegCanvas_CLEAR(ptr_t spd)
	       			    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	/* Clear state of graph items */
	sp->Cursor_active = FALSE;
	sp->Pointer_active = FALSE;
	sp->Graph_active = FALSE;
	SET_STATE(sp, INSERT);

	/* manually clear display */
	XFillRectangle(sp->Base_display, sp->Boxds_xid, sp->Clear_gc, 
	    0, 0, 
	    (int) xv_get((Xv_opaque)sp->segcanvas, XV_WIDTH), 
	    (int) xv_get((Xv_opaque)sp->segcanvas, XV_HEIGHT));
}


/*
 * Turn on and draw hash marks for a screen width of "secs"
 */
void
SegCanvas_HASHDRAW(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	if (sp->Hash_on) {
		/* render hash marks */
		hash_draw(sp, sp->File_len.secs);
	}
}

/*
 * Clear hash marks if necessary.
 */
void
SegCanvas_HASHCLEAR(ptr_t spd)
	       			    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	if (sp->Hash_on) {
		/* clear old marks */
		hash_clear(sp);
	}
}

/*
 * Turn on and draw segment display
 */
void
SegCanvas_GRAPHDRAW(ptr_t spd)
	       			    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	if (!sp->Graph_active) {
		sp->Graph_active = TRUE;
		bd_draw(sp);
	}
}


/*
 * Turn off and clear segment graph
 */
void
SegCanvas_GRAPHCLEAR(ptr_t spd)
	       			    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	SegCanvas_Clearselect(sp);

	/* just clear area where graph is displayed */
	if (sp->Graph_active) {
		sp->Graph_active = FALSE;
		bd_clear(sp, 0);
	}
}

/*
 *	Initialize constants.  Calculate the pixel conversion constants and 
 * 	convert times to pixel values for rendering the display.
 */
void
SegCanvas_INITCONST(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	int				i; 	/* loop index */

	sp = (struct segment_canvas_data *) spd;

	/* do nothing if given nothing */
	if ((sp->Pts_n == 0) || (sp->Pts == NULL))
		return;

	/* save length of file in seconds */
	sp->File_len.secs = sp->Pts[sp->Pts_n - 1].secs;

	if (sp->Boxds_time_incr <= 1.0) {
		/* make display length is size of file */
		if (sp->File_len.secs <= 1.0)
			sp->Boxds_secs = 1.0;
		else
			sp->Boxds_secs = sp->File_len.secs;
	} else {
		/* display length is a multiple of the increment */
		sp->Boxds_secs = (sp->File_len.secs + sp->Boxds_time_incr) - 
		    ((int) ceil(sp->File_len.secs + sp->Boxds_time_incr) % 
					    (int) ceil(sp->Boxds_time_incr));
	}

	/* init conversion number for pixels to seconds */
	sp->Boxds_pps = (double) sp->Boxds_width / sp->Boxds_secs;

	/* calculate pixel values from times */
	for (i=0; i < sp->Pts_n; i++) {
		sp->Pts[i].x = SECS_TO_X(sp, sp->Pts[i].secs);
	}
	/* convert pointer too ... */
	sp->Pointer.x = SECS_TO_X(sp, sp->Pointer.secs);

	/* pixel just beyond end of file */
	sp->File_len.x =  sp->Pts[sp->Pts_n - 1].x + 1;
}


/*
 *	Render the hash marks in the bar display
 *	Return: length of the display in seconds.
 *
 *	NOTE:	It is assumed that BIG_HASH_TIME is a multiple of 
 *		SMALL_HASH_TIME.
 */
hash_draw(ptr_t spd, double length)
	       		    	/* segment data handle */
	      		       	/* length of audio in secs */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	double		x;	/* current x position of hash mark */
	double		pixgap;	/* number of pixels to next hash mark */
	int		nbig;	/* number of big hash marks */
	int		t;	/* time in seconds */
	char		stime[20]; /* time as a string */
	int		twidth;	/* text width */

	sp = (struct segment_canvas_data *) spd;

	/* if length is undefined, do not render hash marks */
	if (length == HUGE_VAL)
		return(0.0);

	/* if number of big hash marks greater than display width */
	nbig = (int) (length / (double) BIG_HASH_TIME);
	if (nbig >= sp->Boxds_width) {
		/* XXX if file is extremely large draw a solid black box */
		XFillRectangle(sp->Base_display, sp->Boxds_xid, sp->Copy_gc, 
			sp->Boxds_xstart, sp->Boxds_height - sp->Big_hash_height, 
			sp->Boxds_width, sp->Big_hash_height);
	}

	/* calculate pixel gap between two small hash marks */
	pixgap = (SECS_TO_X(sp, (double) SMALL_HASH_TIME));

	/* calculate max text width (based on total time). we determine
	 * when and if to draw depending on how many pixels the max
	 * time value will take up as a string
	 */
	if (sp->Label_font) {
		audio_secs_to_str(sp->Boxds_secs, stime, 0);
		twidth = XTextWidth(sp->Label_font, stime,  strlen(stime))
		    + (2*LABEL_X_OFFSET);
	}

	/* while pixel time is less that time length of display */
	for (t = 0; t < (int) ceil(sp->Boxds_secs); t += SMALL_HASH_TIME) {
		/* calculate pixel position from time */
		x = SECS_TO_X(sp, t);
		
		if ((t % BIG_HASH_TIME) != 0) {
			/* if there is a pixel gap, draw a small hash mark */
			if (pixgap > 1)
				XDrawLine(sp->Base_display, sp->Boxds_xid, 
				    sp->Copy_gc, 
				    sp->Boxds_xstart + irint(x), 
				    sp->Boxds_height, 
				    sp->Boxds_xstart + irint(x), 
				    sp->Boxds_height - sp->Hash_height);
		} else {
			/* draw a big hash mark */
			XDrawLine(sp->Base_display, sp->Boxds_xid, sp->Copy_gc,
				sp->Boxds_xstart + irint(x), sp->Boxds_height,
				sp->Boxds_xstart + irint(x), sp->Boxds_height - 
							sp->Big_hash_height);

		}

		/* draw label, if font exists, and there's room. this
		 * is messy. if the text width will fit between the small
		 * hash marks, draw it at every small hash mark, otherwise
		 * only draw it at the big hash marks -- if it'll fit
		 * otherwise don't draw it at all....
		 */
		if (sp->Label_font && ((twidth < pixgap) || 
		   (((t % BIG_HASH_TIME) == 0)
		   && (twidth < (pixgap*(BIG_HASH_TIME/SMALL_HASH_TIME)))))) {
			audio_secs_to_str((double)t, stime, 0);

			XDrawString(sp->Base_display, sp->Boxds_xid,
				    sp->Copy_gc,
				    sp->Boxds_xstart+irint(x)+LABEL_X_OFFSET,
				    sp->Boxds_height - (LABEL_Y_OFFSET +
						    sp->Label_font->descent + 1),
				    stime, strlen(stime));
		}

	}
}

/*
 * Clear the hash marks
 */
hash_clear(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	XFillRectangle(sp->Base_display, sp->Boxds_xid, sp->Clear_gc, 
		sp->Boxds_xstart, sp->Boxds_height - sp->Big_hash_height, 
		sp->Boxds_width, sp->Big_hash_height);
}

/***		Cursor routines		***/

void
cursor_crosshair(ptr_t spd)
	       		    			/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	/* 
	 * if the frame is busy, when it's unbusied it will clobber
	 * our change.  So momentarily unbusy the frame and set the cursor
	 */
	if ((sp->segowner) && xv_get((Xv_opaque)sp->segowner, FRAME_BUSY)) {
		xv_set((Xv_opaque)sp->segowner, FRAME_BUSY, FALSE, 0);
		/* change hardware cursor to small cross hash */
		xv_set(xv_get((Xv_opaque)sp->segcanvas, CANVAS_NTH_PAINT_WINDOW, 0), 
			WIN_CURSOR,		sp->Boxds_cursor, 
			NULL);
		xv_set((Xv_opaque)sp->segowner, FRAME_BUSY, TRUE, 0);
	} else {
		/* change hardware cursor to small cross hash */
		xv_set(xv_get((Xv_opaque)sp->segcanvas, CANVAS_NTH_PAINT_WINDOW, 0), 
			WIN_CURSOR,		sp->Boxds_cursor, 
			NULL);
	}
}

void
cursor_normal(ptr_t spd)
	       		    			/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	/* 
	 * if the frame is busy, when it's unbusied it will clobber
	 * our change.  So momentarily unbusy the frame and set the cursor
	 */
	if ((sp->segowner) && xv_get((Xv_opaque)sp->segowner, FRAME_BUSY)) {
		xv_set((Xv_opaque)sp->segowner, FRAME_BUSY, FALSE, 0);
		/* change hardware cursor to small cross hash */
		xv_set(xv_get((Xv_opaque)sp->segcanvas, CANVAS_NTH_PAINT_WINDOW, 0), 
			WIN_CURSOR,		sp->Basic_ptr_cursor, 
			NULL);
		xv_set((Xv_opaque)sp->segowner, FRAME_BUSY, TRUE, 0);
	} else {
		/* change hardware cursor to small cross hash */
		xv_set(xv_get((Xv_opaque)sp->segcanvas, CANVAS_NTH_PAINT_WINDOW, 0), 
			WIN_CURSOR,		sp->Basic_ptr_cursor, 
			NULL);
	}
}

/*
 *	Load cursor images, and create cursors
 */
cursor_init(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	static unsigned short		Crosshair_bits[] = {
#include "xhair_cursor.pr"
};
	static unsigned short		Basic_Ptr_bits[] = {
#include "basic_cursor.pr"
};
	Xv_singlecolor			*fg;	/* foreground color */
	Xv_singlecolor			*bg;	/* background color */

	sp = (struct segment_canvas_data *) spd;

	/* initialize foreground and background colors */
	fg = (Xv_singlecolor *) xv_get((Xv_opaque)sp->segowner, FRAME_FOREGROUND_COLOR);
	bg = (Xv_singlecolor *) xv_get((Xv_opaque)sp->segowner, FRAME_BACKGROUND_COLOR);

	sp->Boxds_cimage = (Server_image) xv_create(XV_NULL, SERVER_IMAGE,
		XV_WIDTH,		16,
		XV_HEIGHT,		16,
		SERVER_IMAGE_BITS,	Crosshair_bits,
		NULL);

	sp->Basic_ptr_im = (Server_image) xv_create(XV_NULL, SERVER_IMAGE,
		XV_WIDTH,		16,
		XV_HEIGHT,		16,
		SERVER_IMAGE_BITS,	Basic_Ptr_bits,
		NULL);

	/* create cursor object and assign to window */
	sp->Boxds_cursor = xv_create(XV_NULL, CURSOR,
		CURSOR_IMAGE,			sp->Boxds_cimage,
		CURSOR_OP,      		PIX_SRC ^ PIX_DST,
		CURSOR_FOREGROUND_COLOR,	fg,
		CURSOR_BACKGROUND_COLOR, 	bg,
		CURSOR_XHOT,			CURSOR_WIDTH / 2,
		CURSOR_YHOT,			0,
		NULL);

	/* create cursor object and assign to window */
	sp->Basic_ptr_cursor = xv_create(XV_NULL, CURSOR,
		CURSOR_IMAGE,	sp->Basic_ptr_im,
		CURSOR_OP,      PIX_SRC ^ PIX_DST,
		CURSOR_XHOT,	0,
		CURSOR_YHOT,	0,
		NULL);
}


/*
 *	Activate the vertical cursor.
 */
cursor_on(ptr_t spd, int x)
	       		    	/* segment data handle */
	   		  	/* X pixel position */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	if (sp->Cursor_active) {
		cursor_animate(sp, x);
	} else {
		/* activate vertical cursor */
		sp->Cursor_active = TRUE;
		sp->Cursor_x = x;
		cursor_draw(sp, sp->Cursor_x);
		(*sp->cursor_position_proc)(sp);
	}
}

/*
 *	Deactivate the vertical cursor.
 */
cursor_off(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	if (sp->Cursor_active) {
		cursor_clear(sp, sp->Cursor_x);
		sp->Cursor_active = FALSE;
	}
}

/*
 *	Erase the vertical cursor.
 */
cursor_clear(ptr_t spd, int x)
	       		    	/* segment data handle */
	   		  	/* X pixel position */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	if (sp->Cursor_active)
		/* since we use xor, clearing is same as drawing */
		XDrawLine(sp->Base_display, sp->Boxds_xid, sp->Xor_gc, 
		    sp->Boxds_xstart + x, 0, 
		    sp->Boxds_xstart + x, sp->Boxds_height);
}

/*
 *	Draw the vertical cursor.
 */
cursor_draw(ptr_t spd, int x)
	       		    	/* segment data handle */
	   		  	/* X pixel position */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	if (sp->Cursor_active)
		XDrawLine(sp->Base_display, sp->Boxds_xid, sp->Xor_gc, 
		    sp->Boxds_xstart + x, 0, 
		    sp->Boxds_xstart + x, sp->Boxds_height);
}

/*
 *	Track cursor movements.  Erase previous position, then
 *	render current position.
 */
cursor_animate(ptr_t spd, int newx)
	       		    	/* segment data handle */
	   		     	/* X pixel position */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	if (sp->Cursor_active && (newx != sp->Cursor_x)) {
		cursor_clear(sp, sp->Cursor_x);
		sp->Cursor_x = newx;
		cursor_draw(sp, sp->Cursor_x);
		(*sp->cursor_position_proc)(sp);
	}
}


/****	Current Position sp->Pointer routines *****/

/*
 * Turn off the pointer display
 */
pointer_off(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	if (sp->Pointer_active) {
		sp->Pointer_active = FALSE;
		pointer_clear(sp);
	}
}

/*
 *	Activate the pointer.
 */
pointer_on(ptr_t spd, Display_point x)
	       		    	/* segment data handle */
	             		  	/* X position */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	if (!sp->Pointer_active) {
		sp->Pointer_active = TRUE;
		sp->Pointer.secs = x.secs;
		sp->Pointer.x = x.x;
		pointer_draw(sp, x.x);
		(*sp->pointer_position_proc)(sp);
	} else {
		/* if pointer already on move to new position */
		pointer_animate(sp, x);
	}
}

/*
 *	Erase the pointer.
 */
pointer_clear(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	/* fill with the background color to clear */
	XFillPolygon(sp->Base_display, sp->Boxds_xid, sp->Xor_gc, 
		sp->Pointer_pts, sp->Pointer_npts,
		Convex, CoordModePrevious);
}

/*
 *	Draw the pointer.
 */
pointer_draw(ptr_t spd, int x)
	       		    	/* segment data handle */
	   		  	/* X pixel position */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	sp->Pointer_pts[0].x = sp->Boxds_xstart + x;
	XFillPolygon(sp->Base_display, sp->Boxds_xid, sp->Xor_gc, 
		sp->Pointer_pts, sp->Pointer_npts,
		Convex, CoordModePrevious);
}


/*
 *	Animate pointer and save new position.
 */
pointer_animate(ptr_t spd, Display_point newx)
	       		    	/* segment data handle */
	             	     	/* pixel position */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	if (sp->Pointer_active && (newx.x != sp->Pointer.x)) {
		/* Erase previous position */
		pointer_clear(sp);

		/* save current */
		sp->Pointer.secs = newx.secs;
		sp->Pointer.x = newx.x;

		pointer_draw(sp, sp->Pointer.x);

		(*sp->pointer_position_proc)(sp);
	}
}


/*
 * 	Draw a selection box.  x1 and x2 are inclusive pixel end points
 *	of the selection.
 */
select_draw(ptr_t spd, int x1, int x2)
	       		    	/* segment data handle */
	   		   	/* starting pixel */
	   		   	/* ending pixel */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	int		tmp;

	sp = (struct segment_canvas_data *) spd;

	/* make x1 the smallest pixel value */
	if (x1 > x2) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
	}
	XFillRectangle(sp->Base_display, sp->Boxds_xid, sp->Xor_gc, 
		sp->Boxds_xstart + x1, 
		(sp->Boxds_height / 2) - sp->Half_bar_height - 2, 
		x2 - x1 + 1, 
		(2 * sp->Half_bar_height) + 5);
}

/*
 * 	Clear selection box.  x1 and x2 are inclusive pixel end points
 *	of the selection.
 */
select_clear(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	int				tmp;
	int				x1;	/* starting pixel */
	int				x2;	/* ending pixel */

	sp = (struct segment_canvas_data *) spd;

	if ((sp->S == NOTHING) || (sp->S == INSERT))
		return;

	x1 = sp->Ext_left.x;
	x2 = sp->Ext_right.x;

	/* make x1 the smallest pixel value */
	if (x1 > x2) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
	}
	XFillRectangle(sp->Base_display, sp->Boxds_xid, sp->Xor_gc, 
		sp->Boxds_xstart + x1, 
		(sp->Boxds_height / 2) - sp->Half_bar_height - 2, 
		x2 - x1 + 1, 
		(2 * sp->Half_bar_height) + 5);
}

/*
 *	select_snap()
 *		Walk through detection list, and find position where
 *	x lies between two points.  Then set x to appropriate snap point.  
 *	If no selection exists, extend selection out to both points.
 */
select_snap(ptr_t spd,
            int *adjusting_right,
            Display_point *pts,
            int list_n,
            Display_point *x)
	       		    	/* segment data handle */
	   		                 /* true if adjusting right side */
	             	     		/* seg point list */
	   		       		/* number of elements in list */
	             	   		/* current cursor position */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	int		s;		/* snap pixel index */
	Display_point	snap_xl;	/* left snap pixel position */
	Display_point	snap_xr;	/* right snap pixel position */

	sp = (struct segment_canvas_data *) spd;

	/* find segment to select */
	s = 0;
	while ((s < list_n) && (pts[s].x < (*x).x))
		s++;

	/* save snap positions */
	if (s == 0) {
		/* inside first segment */
		snap_xl.x = 0;
		snap_xl.secs = 0.0;
		snap_xl.type = SEG_POSITION;
		snap_xr = pts[s];
	} else if (s == list_n) {
		/* inside last segment */
		snap_xl = pts[s-1];
		snap_xr = sp->File_len;
	} else {
		/* somewhere amongst the segments */
		snap_xl = pts[s-1];
		snap_xr = pts[s];
	}

	/*
	 * If were in INSERT mode, and Insertion point lies between both
	 * snap points, then we're processing a double click over a
	 * segment, with no selection.  Expand selection to snap region.
	 */
	if ((sp->S == INSERT) && 
	    (snap_xl.x <= sp->Pointer.x) && (sp->Pointer.x <= snap_xr.x)) {
		/* select the whole snap region */
		sp->Ext_left = snap_xl;
		sp->Ext_right = snap_xr;

		/*
		 * This is a special case, since both sides of the
		 * selection are being modified at once.  The pointer
		 * should always appear to set the anchor at insertion time.
		 * To emulate that behavior, we always set adjusting_right
		 * to TRUE, forcing Select_animate to make the pointer 
		 * (which is always on the left) the anchor.
		 */
		sp->adjusting_right = TRUE;
		DBGOUT((10,"\tsp->adjusting_right = %s\n",
		    (sp->adjusting_right) ? "TRUE" : "FALSE"));
	}

	/* extend x to proper snap point. */
	(*x) = (sp->adjusting_right) ? snap_xr : snap_xl;
	DBGOUT((10,"Snap points are %f, %f \n",
	    snap_xl.secs, snap_xr.secs));
	DBGOUT((10,"Original selection was %f, %f : new x = %f\n",
	    sp->Ext_left.secs, sp->Ext_right.secs, (*x).secs));
}

/*
 *	Selection state machine
 *		All selections start by setting an insertion point.  Then
 *		they swith to an animation mode as the user makes the 
 *		selection, and end up in done state when the user lets go.
 *		The state is maintained by the functions Insert_set,
 *		Select_animate, and Select_done.
 */



/*
 * Clear any existing selection, then set insert point.
 */
Insert_set(ptr_t spd, Display_point x)
	       			    	/* segment data handle */
	             		  	/* new position of insert pointer */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	DBGOUT((10,"Insert_set = %f\n", x.secs));

	/* clear old selection if one exists */
	if ((sp->S == SELECT_EXISTS) || (sp->S == MAKING_SELECT)) {
		select_clear(sp);
	}

	SET_STATE(sp, INSERT);

	/* initialize anchor and selection end points */
	sp->Anchor = sp->Ext_left = sp->Ext_right = x;

	/* call the insert call back */
	(*sp->insert_proc)(sp); 
}

Select_done(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	/* update state */
	SET_STATE(sp, SELECT_EXISTS);

	/* call selection call back */

	/* grab ownership of audio primary selection */
#ifndef OWV2
	Audio_OWN_PRIMARY_SELECTION((Xv_opaque)sp->segcanvas);
#endif
}

/*
 *	Select_animate()
 *		Animate making a selection by clearing previous selection,
 *		and then rendering the new one.
 */
Select_animate(ptr_t spd, Display_point x, int snap)
	       		    	/* segment data handle */
	             	  	/* new X position to extend selection to */
	   		     	/* true to snap selection to box boundary */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	switch(sp->S) {
	/* 
	 * Handles cases: 
	 *	Insert (or Extend) was pressed and now is first drag event
	 *	Extend was pressed but not dragged, after an insert. 
	 *  Double click, with no previous selection.
	 */
	case INSERT:
		/* determine which side of the selection we're adjusting */
		sp->adjusting_right = 
		    !(x.x < sp->Ext_right.x - 
			((sp->Ext_right.x - sp->Ext_left.x) >>1));

		/* Snap will set x to nearest segment boundary */ 
		if (snap)
			select_snap(sp, &sp->adjusting_right, 
			    sp->Pts, sp->Pts_n, &x);

		/* set anchor point (select_snap updates Ext_left/right.x) */
		sp->Anchor = (sp->adjusting_right) ? 
		    sp->Ext_left : sp->Ext_right;
	break;
	/* 
	 * Handles case: 
	 *	Extend was pressed after a selection was made.
	 *  Double click, with previous selection.
	 */
	case SELECT_EXISTS:
		/* clear existing selection */
		select_clear(sp);

		/* determine which side of the selection we're adjusting */
		sp->adjusting_right = 
			!(x.x < sp->Ext_right.x - 
			    ((sp->Ext_right.x - sp->Ext_left.x)>>1));

		/* Snap will set x to nearest segment boundary */ 
		if (snap)
			select_snap(sp, &sp->adjusting_right, 
			    sp->Pts, sp->Pts_n, &x);

		/* set anchor point */
		sp->Anchor = (sp->adjusting_right) ? 
		    sp->Ext_left : sp->Ext_right;
	break;
	/* 
	 * Handles case when Insert (or extend) is being held down 
	 * and dragged We can't get here until we've gone through 
	 * one of the cases above first.
	 */
	case MAKING_SELECT:
		/* clear existing selection */
		select_clear(sp);
		/* 
		 * While making a selection, if the cursor crosses from one
		 * side of the anchor to another, the side anchoring the
		 * rubber banding must be switched.
		 */
		if (sp->adjusting_right) {
			/* but we're on the left */
			if (x.x < sp->Anchor.x) {
				/* right side is now anchor */
				sp->Ext_right = sp->Anchor;
				sp->adjusting_right = FALSE;
				DBGOUT((10,"\tsp->adjusting_right = %s\n",
				    (sp->adjusting_right) ? "TRUE" : "FALSE"));
			}
		} else {
			/* but we're on the right */
			if (x.x > sp->Anchor.x) {
				/* left side is now anchor */
				sp->Ext_left = sp->Anchor;
				sp->adjusting_right = TRUE;
				DBGOUT((10,"\tsp->adjusting_right = %s\n",
				    (sp->adjusting_right) ? "TRUE" : "FALSE"));
			}
		}

		/* Snap will set x to nearest segment boundary */ 
		if (snap)
			select_snap(sp, &sp->adjusting_right, 
			    sp->Pts, sp->Pts_n, &x);
	break;
	case NOTHING:	/* Invalid case */
	default:
		DBGOUT((10,"\t\t\tInvalid State in Select_animate!\n"));
	break;
	}

	/* enter state of making a selection */
	SET_STATE(sp, MAKING_SELECT); 

	/* 
	 * Draw new selection.  If cursor is right of the center of the 
	 * selection, adjust the right hand marker, and visa versa. 
	 */
	if (sp->adjusting_right) {
		/* adjust right marker */
		sp->Ext_right = x;

		/* display selection */
		select_draw(sp, sp->Ext_left.x, sp->Ext_right.x);
	} else {
		/* adjust left marker */
		sp->Ext_left = x;

		/* display selection */
		select_draw(sp, sp->Ext_left.x, sp->Ext_right.x);
	}
}

/*
 * draw a selection 
 */
select_on(ptr_t spd, Display_point x, int snap)
	       		    	/* segment data handle */
	             	  	/* new position to extend selection to */
	   		     	/* true to snap selection to box boundary */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	DBGOUT((10,"select_on at %f\n", x.secs));

	Select_animate(sp, x, snap);

	(*sp->update_select_proc)(sp);
}



		/** Bar graphing routines **/

/*
 *	Render the bar display from segment point list
 */
bd_draw(ptr_t spd)
	       		    	/* segment data handle */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	int		i;		/* loop index */

	sp = (struct segment_canvas_data *) spd;

	/* if there are no graph points, or no data to graph, do nothing */
	if ((sp->Pts_n == 0) || (sp->Pts == NULL) || 
	    ((sp->Pts[1].type == SEG_EOF) && (sp->Pts[1].x == 0)))
		return;

	/* render segment display */
	for (i=0; i < sp->Pts_n; i++) {
		switch (sp->Pts[i].type) {
		case SEG_SILENT_START:
			/* draw a line for silence */
			barlink_draw(sp, sp->Pts[i].x, sp->Pts[i+1].x);
		break;
		case SEG_SOUND_START:
			/* draw a box for sound */
			bar_draw(sp, sp->Pts[i].x, sp->Pts[i+1].x);
		break;
		case SEG_EOF:
			/* do nothing for now */
		break;
		default:
			DBGOUT((10,"bad seg point type\n"));
		break;
		}
	}
}

bd_clear(ptr_t spd, int x)
	       	    	/* segment data handle */
	   	  	/* x position to start clear strip at */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	/* fill bar strip with the background color to clear */
	XFillRectangle(sp->Base_display, sp->Boxds_xid, sp->Clear_gc, 
		sp->Boxds_xstart + x, 
		(sp->Boxds_height / 2) - sp->Half_bar_height, 
		sp->Boxds_width, 
		2 * sp->Half_bar_height);
}

bar_draw(ptr_t spd, int x1, int x2)
	       		    	/* segment data handle */
	   		   	/* starting pixel */
	   		   	/* ending pixel */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	XDrawRectangle(sp->Base_display, sp->Boxds_xid, sp->Copy_gc, 
		sp->Boxds_xstart + x1, 
		(sp->Boxds_height / 2) - sp->Half_bar_height, 
		x2 - x1, 
		2 * sp->Half_bar_height);
}

bar_clear(ptr_t spd, int x1, int x2)
	       		    	/* segment data handle */
	   		   	/* starting pixel */
	   		   	/* ending pixel */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	XDrawRectangle(sp->Base_display, sp->Boxds_xid, sp->Copy_gc, 
		sp->Boxds_xstart + x1, 
		(sp->Boxds_height / 2) - sp->Half_bar_height, 
		x2 - x1, 
		2 * sp->Half_bar_height);
}

barlink_draw(ptr_t spd, int x1, int x2)
	       		    	/* segment data handle */
	   		   	/* starting pixel */
	   		   	/* ending pixel */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	XDrawLine(sp->Base_display, sp->Boxds_xid, sp->Copy_gc, 
		sp->Boxds_xstart + x1, (sp->Boxds_height / 2), 
		sp->Boxds_xstart + x2, (sp->Boxds_height / 2));
}

barlink_clear(ptr_t spd, int x1, int x2)
	       		    	/* segment data handle */
	   		   	/* starting pixel */
	   		   	/* ending pixel */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	XDrawLine(sp->Base_display, sp->Boxds_xid, sp->Copy_gc, 
		sp->Boxds_xstart + x1, (sp->Boxds_height / 2), 
		x2, (sp->Boxds_height / 2));
}


/*
 * set selection state and print value
 */
set_state(ptr_t spd, State ss)
	       		    	/* segment data handle */
	     		   		/* selection state */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	sp->S = ss;
	switch (sp->S) {
	    case NOTHING:
		    DBGOUT((10,"\t\tstate = NOTHING\n"));
	    break;
	    case INSERT:
		    DBGOUT((10,"\t\tstate = INSERT\n"));
	    break;
	    case MAKING_SELECT:
		    DBGOUT((10,"\t\tstate = MAKING_SELECT\n"));
	    break;
	    case SELECT_EXISTS:
		    DBGOUT((10,"\t\tstate = SELECT_EXISTS\n"));
	    break;
	    default:
		    DBGOUT((10,"\t\tstate = BAD_STATE!\n"));
	}
}


/*
 *	Return true if the mouse/cursor is over the canvas
 */
int
mouse_overCanvas(
	ptr_t		spd, 		/* segment data handle */
	int*		x		/* x pixel position of mouse */
	)
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	Rect*		r;		/* mouse coordinates */
	int		cheight;	/* canvas height */
	int		cwidth;		/* canvas width */

	sp = (struct segment_canvas_data *) spd;

	/* get mouse coordinates relative to canvas */
	r = (Rect *)xv_get(canvas_paint_window((Xv_opaque)sp->segcanvas), 
	    WIN_MOUSE_XY);
	cheight = (int) xv_get((Xv_opaque)sp->segcanvas, XV_HEIGHT);
	cwidth = (int) xv_get((Xv_opaque)sp->segcanvas, XV_WIDTH);

	*x = r->r_left - sp->Boxds_xstart;

	return (((r->r_left >= 0) && (r->r_left < cwidth)) && 
		((r->r_top >= 0) && (r->r_top < cheight)));
}

/*
 * Event callback function for `Draw_canvas'.
 */
Notify_value
draw_canvas_event(Xv_window win,
                  Event *event,
                  Notify_arg arg,
                  Notify_event_type type)
{
	struct segment_canvas_data	*sp;	/* segment data pointer */
	Display_point	cur;		/* position of cursor */
	long		secs;		/* seconds */
	long		usecs;		/* micro seconds */
	int		err;		
	int		tmp_x;		/* temporary pixel value */
	char    	name[MAXPATHLEN+1];	/* drop file name */
	char		*ns, *ne;	/* start and end index of path */
	char		tmpchar;
	ptr_t ap;

	/* get segment data pointer */
	sp = (struct segment_canvas_data *) 
		xv_get((Xv_opaque)win, XV_KEY_DATA, SEG_CANVAS_KEY);
	

	/* save current position accounting for display offset */
	cur.x = event_x(event) - sp->Boxds_xstart;
	cur.secs = X_TO_SECS(sp, cur.x);
	cur.type = SEG_POSITION;

	/* keep virtual cursor position constrained to the graph */
	if (cur.x < 0) 
		cur = sp->Zero_pos; 
	else if (cur.x >= sp->File_len.x) {
		cur = sp->File_len;
		cur.x = sp->File_len.x - 1;
	}

	/* if no file is currently active, pass events to other handlers */
	if (!sp->Graph_active)
	    return notify_next_event_func(win, (Notify_event) event, 
					  arg, type);

	/* catch mouse and button events */
	switch(event_action(event)) {
	    case ACTION_SELECT:
		/*
		DBGOUT((10,"\t\tLEFT button is %s\n",
		    event_is_down(event) ? "DOWN" : "UP"));

		DBGOUT((10,"Cursor pixel, real: %d		virtual: %d",
		    event_x(event), cur.x));
		*/

		/* if no data to graph, turn off all select events */
		if ((sp->Pts[1].type == SEG_EOF) && (sp->Pts[1].x == 0))
			break;

		/* starting a selection or just setting the insert point */
		if (event_is_down(event)) {
			sp->left_button_down = TRUE;

			/* detect double click */
			secs = event_time(event).tv_sec - sp->left_time.tv_sec;
			if (secs <= 1) {
				usecs = event_time(event).tv_usec +
				    (secs * 1000000) - sp->left_time.tv_usec;
				if (usecs < sp->Multi_click_time) {
					if (sp->ldbl_click) 
						sp->tpl_click = TRUE;
					else
						sp->ldbl_click = TRUE;
					sp->dbl_click_mode = TRUE;
					DBGOUT((10,"sp->ldbl_click = %s\n",
					    sp->ldbl_click ? "TRUE" : "FALSE"));
				} else {
					/* reset click state */
					sp->ldbl_click = FALSE;
					sp->tpl_click = FALSE;
					sp->mdbl_click = FALSE;
					sp->dbl_click_mode = FALSE;
				}
			} else {
				/* reset click state */
				sp->ldbl_click = FALSE;
				sp->tpl_click = FALSE;
				sp->mdbl_click = FALSE;
				sp->dbl_click_mode = FALSE;
			}
			DBGOUT((10,"sp->dbl_click_mode = %s\n",
			    sp->dbl_click_mode ? "TRUE" : "FALSE"));
			DBGOUT((10,"sp->ldbl_click = %s\n",
			    sp->ldbl_click ? "TRUE" : "FALSE"));
			DBGOUT((10,"sp->tpl_click = %s\n",
			    sp->tpl_click ? "TRUE" : "FALSE"));
			/* save time stamp */
			sp->left_time = event_time(event);

			if (sp->tpl_click) {
				DBGOUT((10,"detected sp->tpl_click\n"));


				/* select entire file.  */
				Insert_set(sp, sp->Zero_pos);
				select_on(sp, sp->File_len, TRUE);
			} else if (sp->ldbl_click) {
				/* select segment mouse is over */
				select_on(sp, cur, sp->ldbl_click);
			} else {
				/* save place to insert */
				sp->insert_pt = cur;

				/* if selecting over a selection */ 
				if ((sp->S == SELECT_EXISTS) &&
				    (sp->Ext_left.x <= cur.x) && 
				    (cur.x <= sp->Ext_right.x)) {
					sp->Drag_select = TRUE;
				} else {
					/* 
					 * we're not over a selection.
					 * set insert the point.
					 */
					Insert_set(sp, cur);
					sp->Drag_select = FALSE;
				}
			}
			break;
		} 

		/* no selection made, call insert event callback */
		if (event_is_up(event) && (sp->S != MAKING_SELECT)) {
			/* check for trying to insert over a selection */
			if (sp->Drag_select) { 
				/* didn't drag beyond threshold */
				if (!sp->dragging)
					Insert_set(sp, sp->insert_pt);
			}

			/* reset state */
			sp->dragging = FALSE;
			sp->Drag_select = FALSE;
			sp->left_button_down = FALSE;
			break;
		}

		/* finished making a selection */
		if (event_is_up(event) && (sp->S == MAKING_SELECT)) {
			/* if zero length selection, set insert point */
			if (cur.x == sp->Anchor.x) {
				/* not a selection, change state */
				Insert_set(sp, cur);
			} else {
				/* finished making selection */
				Select_done(sp);

				/* call selection call back */
				(*sp->done_select_proc)(sp);
			}

			/* reset select button state */
			sp->dragging = FALSE;
			sp->Drag_select = FALSE;
			sp->left_button_down = FALSE;
			break;
		}
			
	    break;
	    case ACTION_ADJUST:
		DBGOUT((10,"\t\tMIDDLE button is %s\n",
		    event_is_down(event) ? "DOWN" : "UP"));

		/* if no data to graph, turn off all ADJUST events */
		if ((sp->Pts[1].type == SEG_EOF) && (sp->Pts[1].x == 0))
			break;

		/* starting a selection or just setting the insert point */
		if (event_is_down(event)) {
			sp->middle_button_down = TRUE;

			/* detect double click */
			secs = 
			    event_time(event).tv_sec - sp->middle_time.tv_sec;
			if (secs <= 1) {
				usecs = event_time(event).tv_usec +
				    (secs * 1000000) - sp->middle_time.tv_usec;
				if (usecs < sp->Multi_click_time) {
					if (sp->mdbl_click)
						sp->tpl_click = TRUE;
					else
						sp->mdbl_click = TRUE;
					DBGOUT((10,"sp->mdbl_click = %s\n",
					    sp->mdbl_click ? "TRUE" : "FALSE"));
				} else {
					sp->tpl_click = FALSE;
					sp->mdbl_click = FALSE;
				}
			} else {
				sp->tpl_click = FALSE;
				sp->mdbl_click = FALSE;
			}
			/* save time stamp */
			sp->middle_time = event_time(event);

			/*
			 * Detected triple click, clear old selection, then
			 * select entire file.
			 */
			if (sp->tpl_click) {
				DBGOUT((10,"detected sp->tpl_click\n"));
				if ((sp->S == SELECT_EXISTS) || 
				    (sp->S == MAKING_SELECT))
				/* select entire file */
				Insert_set(sp, sp->Zero_pos);
				select_on(sp, sp->File_len, TRUE);
				break;
			}

			/* single click on extend */
			switch (sp->S) {
			case NOTHING:
			case INSERT:
				/*
				 * If there is no selection to extend,
				 * extend the selection from the
				 * current insert (play) position.
				 */
				Insert_set(sp, sp->Pointer);
				select_on(sp, cur, sp->mdbl_click);
				break;
			case MAKING_SELECT:
			case SELECT_EXISTS:
				/* extend the current selection */
				select_on(sp, cur, 
					sp->mdbl_click || sp->dbl_click_mode);
				break;
			default:
				DBGOUT((10,"state = BAD_STATE!\n"));
			}
			break;	/* done with event */
		}

		/* no selection made, call insert event callback */
		if (event_is_up(event) && (sp->S != MAKING_SELECT)) {
			sp->dragging = FALSE;
			sp->middle_button_down = FALSE;
			break;
		}

		/* finish making selection */
		if (event_is_up(event) && 
		    ((sp->S == MAKING_SELECT) || (sp->S == SELECT_EXISTS))) {
			/* if zero length selection, set insert point */
			if (cur.x == sp->Anchor.x) {
				Insert_set(sp, cur);
			}

			Select_done(sp);
			(*sp->done_select_proc)(sp);

			sp->middle_button_down = FALSE;
			sp->dragging = FALSE;
		}
	    break;

	    case ACTION_MENU:
		if (event_is_up(event)) {
			DBGOUT((10,"\n\t\tPixel\tSecs\n"));
			DBGOUT((10,"Left Select: \t%d\t%.2f\n",
				sp->Ext_left.x, sp->Ext_left.secs));
			DBGOUT((10,"Right Select: \t%d\t%.2f\n",
				sp->Ext_right.x, sp->Ext_right.secs));
			DBGOUT((10,"sp->Pointer: \t%d\t%.2f\n",
				sp->Pointer.x, sp->Pointer.secs));
			DBGOUT((10,"Cursor: \t%d\t%.2f\n",
				cur.x, cur.secs));
			DBGOUT((10,"File len: \t%d\t%.2f\n",
				sp->File_len.x, sp->File_len.secs));
			DBGOUT((10,"\n"));

			/* XXX - don't do this. bring up a menu (atool_xv.c) */
#ifdef notdef
			/* set insert pointer at zero */
			Insert_set(sp, sp->Zero_pos);
#endif
		}
		break;
	    case LOC_MOVEWHILEBUTDOWN:

#define	MULTI_CLICK_JITTER 5

		/* update cursor if dragging right button */
		if (!(sp->left_button_down || sp->middle_button_down)) {
			cursor_animate(sp, cur.x);
			break;
		}

		/*
		 * Account for jitter when user tries to insert.
		 * (Drag_select is inserting over a selection)
		 * Compare cursor position on down event (insert_pt) to
		 * current position, if less than jitter, just update 
		 * the cursor
		 */
		if (((sp->S == INSERT) || (sp->Drag_select)) && 
		    (abs(sp->insert_pt.x - cur.x) < MULTI_CLICK_JITTER)) {
			DBGOUT(
			  (10,"LOC_MOVEWHILEBUTDOWN, CAUGHT jitter = %d\n",
			    (cur.x - sp->Anchor.x)));

			/* update the cursor and return */
			cursor_animate(sp, cur.x);
			break;
		} 
		DBGOUT((10,"LOC_MOVEWHILEBUTDOWN, jitter = %d\n",
		    (cur.x - sp->Anchor.x)));

		/* detect first drag past threshold */
		if (!sp->dragging) sp->dragging = TRUE;

		if (sp->Drag_select) {
#ifndef OWV2
			/* dragging a selection, start DnD  */
			Audio_SENDDROP((Xv_opaque)sp->segcanvas);
#endif
		} else if (sp->tpl_click) {
			/* 
			 * if dragging between the down and up event of a 
			 * triple click, just update cursor position.  Don't
			 * modify selection.
			 */
			cursor_animate(sp, cur.x);
		} else {
			/* animate selection */
			select_on(sp, cur, sp->dbl_click_mode);

			/* update cursor position */
			cursor_animate(sp, cur.x);
		}
	    break;
	    case LOC_MOVE:
		cursor_animate(sp, cur.x);
	    break;
	    case LOC_WINENTER:
		cursor_on(sp, cur.x);
	    break;
	    case LOC_WINEXIT:
		cursor_off(sp);
	    break;
	    default:
		    DBGOUT((10,"segment: draw_canvas_event: event %d\n",
			cur.x));
	}

        return notify_next_event_func(win, (Notify_event) event, arg, type);
}


/*
 * Repaint callback function for `Draw_canvas'.
 */
void
draw_canvas_repaint_event(Canvas canvas,
                          Xv_window paint_window,
                          Display *display,
                          Window xid,
                          Xv_xrectlist *rects)
{
	/*
	XExposeEvent    ev;
	XRectangle      clip;
	XRectangle      *xr = rects->rect_array;
	int             i;
	int             minx = xr->x;
	int             miny = xr->y;
	int             maxx = minx + xr->width;
	int             maxy = miny + xr->height;
	* for more efficient updating, use X clipping features.... XXX	*/

	struct segment_canvas_data	*sp;	/* segment data pointer */

	/* get segment data pointer */
	sp = (struct segment_canvas_data *) 
		    xv_get((Xv_opaque)paint_window, XV_KEY_DATA, SEG_CANVAS_KEY);

	redisplay_canvas(sp);
}

redisplay_canvas(ptr_t spd)
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	SegCanvas_INIT_GRAPH_PARAMS(sp);
	SegCanvas_INITCONST(sp);
	SegCanvas_Displayfile(sp);
}

/*
 * 	Resize the display canvas
 */
SegCanvas_Resize(
	ptr_t spd, 
	int hdelta, 		/* change in height of canvas */
	int wdelta)		/* change in width of canvas */
{
	struct segment_canvas_data	*sp;	/* segment data pointer */

	sp = (struct segment_canvas_data *) spd;

	xv_set((Xv_opaque)sp->segcanvas,
	    XV_HEIGHT, sp->Boxds_xv_height + hdelta, NULL);
	xv_set((Xv_opaque)sp->segcanvas,
	    XV_WIDTH, sp->Boxds_xv_width + wdelta, NULL);

	sp->Boxds_xv_height = (int)xv_get((Xv_opaque)sp->segcanvas, XV_HEIGHT);
	sp->Boxds_xv_width = (int)xv_get((Xv_opaque)sp->segcanvas, XV_WIDTH);

	redisplay_canvas(sp);
}
