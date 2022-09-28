#ifndef lint
static char sccsid[] = "@(#)select.c 15.50 90/05/22";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

/*
 * This textedit is the envelope that binds various text display and
 *   editing facilities together.
 */

#include <sys/param.h> /* MAXPATHLEN (include types.h if removed) */
#ifdef SVR4
#include <dirent.h>    /* MAXNAMLEN */
#else
#include <sys/dir.h>   /* MAXNAMLEN */
#endif /* SVR4 - even my mom wouldn't use it, and she's dead */
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#ifdef SVR4
#include <string.h>
#else
#include <strings.h>
#endif /* SVR4 - even BIFF wouldn't use it... */
#include <X11/X.h>
#include <xview/defaults.h>
#include <xview/font.h>
#include <xview/notice.h>
#include <xview/frame.h>
#include <xview/xview.h>
#include <xview/scrollbar.h>
#include <xview/text.h>
#include <xview/panel.h>
#include <xview/selection.h>
#include <xview/sel_svc.h>
#include <xview/sel_attrs.h>
#include <xview/cms.h>


#define draw_box(drawable, gc, x, y, w, h) \
        { \
        XDrawRectangle(dpy, drawable, gc, x, y, w, h); \
        }


/*
 * The presence of this line caused textedit not to be built
 * The function is already defined in the XView libraries
long	textsw_store_file();
*/

extern Drawable		big_bit_xid, preview_pixmap;
extern XImage          *rotate_image;
static GC     		one_gc;  
static GC     		two_gc;  
extern GC     		three_gc;

static	Xv_Window	 paint_window;

struct itimerval itv;
extern int	canvas_dimension;

static int	selection_held = 0;
int	wiping_on = 0;
int	selection_top;
int	selection_left;
int	selection_bottom;
int	selection_right;
static int	speed_value = 2;
static int	dash_value = 4;
static Seln_client canvas_client;
static int	selection_initialized = FALSE;
static int	timer_handle;
static Canvas	big_canvas;

extern int height_block, width_block;
extern int boarder_upper, boarder_lower, boarder_left, boarder_right, preview_boarder_upper, preview_boarder_left;
extern Display *dpy;

extern void set_select_cliprects();

timer_proc(client, which)
        Notify_client   client;
        int             which;

{

        static int      dash_count = 0;
	register int x, y, w, h;

	x = selection_left < selection_right 
	  ? selection_left : selection_right;
	y = selection_top < selection_bottom 
	  ? selection_top : selection_bottom;
	
	w = selection_left < selection_right 
	  ? selection_right - selection_left 
	    : selection_left - selection_right; 
	h = selection_top < selection_bottom 
	  ? selection_bottom - selection_top 
	    : selection_top - selection_bottom;

        if (dash_count & 1) {
	  draw_box(big_bit_xid, one_gc, x, y, w, h);
        } else {
	  draw_box(big_bit_xid, two_gc, x, y, w, h);
	}

        dash_count++;
        return (NOTIFY_DONE);
}


void
canvas_seln_func_proc(client_data, args)

char                    *client_data;
Seln_function_buffer    *args;

{
        Seln_holder     *holder;

        switch (seln_figure_response(args, &holder)) {

        case    SELN_IGNORE :
                                /* do nothing */
                                break;

        case    SELN_REQUEST :
                                /* do nothing */
                                break;

        case    SELN_SHELVE :
                                /* do nothing */
                                break;

        case    SELN_FIND :
                                /* do nothing */
                                break;

        case    SELN_DELETE :
                                break;
        }
 
}

Seln_result
canvas_seln_reply_proc(item, context, length)

Seln_attribute          item;
Seln_replier_data       *context;
int                     length;

{
        static FILE     *primary_selection;
        static int      selection_current;
        char            *destbuf;
        int             will_fit;
        int             result;
        struct          stat statbuf;

        switch (context->rank) {

        case SELN_PRIMARY:
			if (!selection_held)
				return(SELN_DIDNT_HAVE);
                        break;

        case SELN_SECONDARY:
                        return(SELN_DIDNT_HAVE);
                        break;

        case SELN_SHELF:
                        return(SELN_DIDNT_HAVE);
                        break;

        default: 
                        return(SELN_DIDNT_HAVE);
                        break;
        }

        switch (item) {

        case    SELN_REQ_CONTENTS_ASCII:

                break;

        case    SELN_REQ_YIELD:
       		itv.it_interval.tv_sec = 0;
       		itv.it_interval.tv_usec = 0;
       		itv.it_value.tv_sec = 0;
       		itv.it_value.tv_usec = 0;
       		(void)notify_set_itimer_func((Notify_client) &timer_handle, 
					     (Notify_func) timer_proc,
					     ITIMER_REAL, &itv, 0);
		repaint_big_canvas(big_canvas, paint_window, 
				   dpy, big_bit_xid, NULL);
		selection_bottom = selection_top = 0;
		selection_left = selection_right = 0;
		selection_held = FALSE;

                return(SELN_SUCCESS);
                break;

        case    SELN_REQ_BYTESIZE:
                *context->response_pointer++ = (char *) 0;
                return(SELN_SUCCESS);
                break;
 
        case    SELN_REQ_FIRST:
                *context->response_pointer++ = (char *) 0;
                return(SELN_SUCCESS);
                break;
 
        case    SELN_REQ_LEVEL:
                *context->response_pointer++ = (char *) SELN_LEVEL_FIRST;
                return(SELN_SUCCESS);
                break;
 
        case    SELN_REQ_END_REQUEST:
                return(SELN_SUCCESS);
                break;
 
        default:
                return(SELN_UNRECOGNIZED);
                break;
        }
}

select_interpose_proc(canvas, event, arg, type)

Canvas                  canvas;
Event                   *event;
Notify_arg              arg;
Notify_event_type       type;

{
  
  int x, y, w, h, preview_x, preview_y, preview_w, preview_h;
  
  
  x = selection_left < selection_right 
    ? selection_left : selection_right;
  y = selection_top < selection_bottom 
    ? selection_top : selection_bottom;
  
  w = selection_left < selection_right 
    ? selection_right - selection_left 
      : selection_left - selection_right; 
  h = selection_top < selection_bottom 
    ? selection_bottom - selection_top 
      : selection_top - selection_bottom;

  if (event_action(event) == ACTION_SELECT)
    {
      
      if (event_is_up(event))
	{
	  
	  draw_box(big_bit_xid, three_gc, 
		   x, y, w, h);

	  if (( selection_left != selection_right) 
	      && (selection_top != selection_bottom ))
	    {
	      wiping_on = FALSE;
	      itv.it_interval.tv_sec = 0;
	      itv.it_interval.tv_usec = speed_value * 10000;
	      itv.it_value.tv_sec = 0;
	      itv.it_value.tv_usec = 300;
	      (void)notify_set_itimer_func((Notify_client) &timer_handle, 
					   (Notify_func) timer_proc,
					   ITIMER_REAL, &itv, 0);

	      preview_x = (x-boarder_left)/width_block; 
	      preview_y = (y-boarder_upper)/height_block; 
	      preview_w = w/width_block; preview_h = h/height_block;
	      preview_x = preview_x + preview_boarder_left; 
	      preview_y = preview_y + preview_boarder_upper;

	      rotate_image = XGetImage(dpy, preview_pixmap, 
				       preview_x, preview_y, 
				       preview_w, preview_h, 
				       -1, ZPixmap); 
	    } /* if (( selection_left != selection_right) ... */
	  else
	    selection_top = selection_bottom 
	      = selection_left = selection_right = 0;
	} /* if (event_is_up(event)) */
      else /*   if (event_action(event) == ACTION_SELECT) */
	{
	  selection_held = (int) seln_acquire(canvas_client, SELN_PRIMARY);

	  itv.it_interval.tv_sec = 0;
	  itv.it_interval.tv_usec = 0;
	  itv.it_value.tv_sec = 0;
	  itv.it_value.tv_usec = 0;
	  (void)notify_set_itimer_func((Notify_client) &timer_handle, 
				       (Notify_func) timer_proc,
				       ITIMER_REAL, &itv, 0);
	  repaint_big_canvas(big_canvas, paint_window, dpy, big_bit_xid, NULL);
	  
	  selection_bottom = selection_top 
	    = (((event_y(event)-boarder_left)/width_block)*width_block) 
	      + boarder_left;
	  selection_left = selection_right 
	    = (((event_x(event)-boarder_upper)/height_block)*height_block) 
	      + boarder_upper;
	  
	  if (selection_top < boarder_upper) {
	    selection_top = boarder_upper;
	    selection_bottom = boarder_upper;
	  } else 
	    if (selection_top > (canvas_dimension - boarder_lower)) {
	      selection_top = (canvas_dimension - boarder_lower) -1;
	      selection_bottom = (canvas_dimension - boarder_lower) -1;
	    }

	  if (selection_left < boarder_left) {
	    selection_left = boarder_left;
	    selection_right = boarder_left;
	  }
	  else 
	    if (selection_left > (canvas_dimension - boarder_right)) {
	      selection_left = (canvas_dimension - boarder_right) -1;
	      selection_right = (canvas_dimension - boarder_right) -1;
	    }

	  wiping_on = TRUE;
	} /* else */
      return(NOTIFY_DONE);
    } /* if (event_action(event) == ACTION_SELECT)  {} else */
  else if (event_id(event) == LOC_DRAG)
    {
      if (wiping_on)
	{
	  draw_box(big_bit_xid, three_gc, 
		   x, y, w, h);

	  selection_bottom = ((event_y(event) - boarder_left) / width_block) 
	    * width_block + boarder_left;
	  selection_right = ((event_x(event) - boarder_upper) / height_block)
	    * height_block + boarder_upper;

	  if (selection_right < boarder_left) 
	    selection_right = boarder_left;
	  if (selection_right > (canvas_dimension - boarder_right)) 
	    selection_right = (canvas_dimension - boarder_right) -1;
	  if (selection_bottom < boarder_upper) 
	    selection_bottom = boarder_upper;
	  if (selection_bottom > (canvas_dimension - boarder_lower)) 
	    selection_bottom = (canvas_dimension - boarder_lower) -1;

	  x = selection_left < selection_right 
	    ? selection_left : selection_right;
	  y = selection_top < selection_bottom 
	    ? selection_top : selection_bottom;
	  
	  w = selection_left < selection_right 
	    ? selection_right - selection_left 
	      : selection_left - selection_right; 
	  h = selection_top < selection_bottom 
	    ? selection_bottom - selection_top 
	      : selection_top - selection_bottom;
	  
	  draw_box(big_bit_xid, three_gc, 
		   x, y, w, h);

	} /* if (wiping_on) */
    } /*   else if (event_id(event) == LOC_DRAG) */
}


selection_init(canvas)

Canvas	canvas;
{

	Xv_Font         	 my_font;
        XGCValues       	 gc_vals;
        int             	 gc_flags;
	Cms             	 cms;
	char     	      	 dashes[2];
	XRectangle               big_rect[1];

	dashes[0] = dash_value;
	dashes[1] = dash_value;

	paint_window = (Xv_Window)xv_get(canvas, CANVAS_NTH_PAINT_WINDOW, 0);


	/* create a selection service client, to handle selection requests */

	canvas_client = seln_create(canvas_seln_func_proc, 
				    canvas_seln_reply_proc, 0);

        my_font = (Xv_Font)xv_create(0,FONT,
#ifdef OW_I18N
                FONT_FAMILY, FONT_FAMILY_SANS_SERIF,
#else
                FONT_FAMILY, FONT_FAMILY_LUCIDA_FIXEDWIDTH,
#endif
                FONT_STYLE, FONT_STYLE_NORMAL,
                FONT_SCALE, FONT_SCALE_DEFAULT, 
                0);


	 cms = (Cms)xv_get(paint_window, WIN_CMS);
	          
        gc_vals.font = ((XFontStruct *)xv_get(my_font, FONT_INFO))->fid;
        gc_vals.function = GXcopy;
        gc_vals.foreground = xv_get(cms, CMS_FOREGROUND_PIXEL);
        gc_vals.background = xv_get(cms, CMS_BACKGROUND_PIXEL);
        gc_vals.fill_style = FillSolid; 
	
          
        gc_flags = GCFont | GCFunction | GCForeground | GCBackground |
                GCFillStyle;
 
        /*
         * Create GC used for text and line drawing.
         */
        one_gc = XCreateGC(dpy, big_bit_xid, gc_flags, &gc_vals);
	XSetDashes(dpy, one_gc, 0, dashes, 2);
	XSetLineAttributes(dpy, one_gc, 0, 
			   LineDoubleDash, CapNotLast, JoinMiter);

	          
        gc_vals.font = ((XFontStruct *)xv_get(my_font, FONT_INFO))->fid;
        gc_vals.function = GXcopy;
        gc_vals.background = xv_get(cms, CMS_FOREGROUND_PIXEL);
        gc_vals.foreground = xv_get(cms, CMS_BACKGROUND_PIXEL);
        gc_vals.fill_style = FillSolid; 
        gc_flags = GCFont | GCFunction | GCForeground | GCBackground |
                GCFillStyle;

        two_gc = XCreateGC(dpy, big_bit_xid, gc_flags, &gc_vals);
	XSetDashes(dpy, two_gc, 0, dashes, 2);
	XSetLineAttributes(dpy, two_gc, 0, 
			   LineDoubleDash, CapNotLast, JoinMiter);
	
	big_rect[0].x = boarder_left; big_rect[0].y = boarder_upper; 
	big_rect[0].width = (canvas_dimension - boarder_right) 
	  - boarder_left;
	big_rect[0].height = (canvas_dimension - boarder_lower) 
	  - boarder_upper;
	
	XSetClipRectangles( dpy, one_gc, 0, 0, big_rect, 1, YSorted);
	
	XSetClipRectangles( dpy, two_gc, 0, 0, big_rect, 1, YSorted);

	          
        gc_vals.font = ((XFontStruct *)xv_get(my_font, FONT_INFO))->fid;
        gc_vals.function = GXxor;
        gc_vals.foreground = xv_get(cms, CMS_FOREGROUND_PIXEL) 
	  ^ xv_get(cms, CMS_BACKGROUND_PIXEL);
        gc_vals.background = xv_get(cms, CMS_BACKGROUND_PIXEL);
        gc_vals.fill_style = FillSolid; 
	
          
        gc_flags = GCFont | GCFunction | GCForeground | GCBackground |
                GCFillStyle;
 
}

selection_on(canvas)

Canvas	canvas;

{
	if (!selection_initialized)
	{
		selection_init(canvas);
		big_canvas = canvas;
		selection_initialized = TRUE;
	}
}


selection_off()

{
	if (selection_held)
	{
        	itv.it_interval.tv_sec = 0;
        	itv.it_interval.tv_usec = 0;
        	itv.it_value.tv_sec = 0;
        	itv.it_value.tv_usec = 0;
        	(void)notify_set_itimer_func((Notify_client) &timer_handle, (Notify_func) timer_proc,
                	ITIMER_REAL, &itv, 0);
		repaint_big_canvas(big_canvas, paint_window, dpy, big_bit_xid, NULL);
        	selection_bottom = selection_top = 0;
        	selection_left = selection_right = 0;
        	selection_held = FALSE;

/*
 *              arrow_buttons_on(FALSE); 
 */
 
	}
}

void 
set_select_cliprects()
{
  XRectangle               big_rect[1];
  
  if (selection_initialized)
    {
      big_rect[0].x = boarder_left; big_rect[0].y = boarder_upper; 
      big_rect[0].width = (canvas_dimension - boarder_right) - boarder_left; big_rect[0].height = (canvas_dimension - boarder_lower) - boarder_upper;

      XSetClipRectangles( dpy, one_gc, 0, 0, big_rect, 1, YSorted);

      XSetClipRectangles( dpy, two_gc, 0, 0, big_rect, 1, YSorted);
    }
}
