#include <sys/param.h> /* MAXPATHLEN (include types.h if removed) */
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <X11/X.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/cms.h>
#include <xview/font.h>

#include "gdd.h"
#include "base_gui.h"
#include "base.h"

#define MAX_UNDOS 8

#define MAX_FOOTER_LENGTH 64

extern base_window_objects	*base_window; 
extern int	canvas_dimension;

extern int boarder_upper, boarder_lower, boarder_left, boarder_right, height_block, width_block;
extern int preview_boarder_upper, preview_boarder_left;
extern XColor black, current_pen;
extern int current_undo, fill_pattern, undos_allowed, redos_allowed;
extern int redo_possible;
extern int current_color_state;

extern GC three_gc;

extern Undo_struct undo_images[MAX_UNDOS];

/*
 *extern XImage *undo_image[MAX_UNDOS];
 */


#define draw_box(drawable, gc, x, y, w, h) \
        { \
        XDrawLine(dpy, drawable, gc, x, y, x+w, y+h); \
        }


/*
 * The presence of this line caused textedit not to be built
 * The function is already defined in the XView libraries
long	textsw_store_file();
*/

extern Drawable		big_bit_xid;

static int	wiping_on = 0;
static int	selection_top;
static int	selection_left;
static int	selection_bottom;
static int	selection_right;
static int	dash_value = 4;
static Canvas	big_canvas;

extern Display *dpy;

extern GC gc, gc_rv, redraw_gc, fill_gc;
extern Window preview_xid;
extern Pixmap preview_pixmap, big_pixmap;
extern void b_draw_box();

box_interpose_proc(canvas, event, arg, type)

Canvas                  canvas;
Event                   *event;
Notify_arg              arg;
Notify_event_type       type;

{
  int x, y, w, h;

  if (event_action(event) == ACTION_SELECT)
    {
      
      if (event_is_up(event))
	{
	  /*	  
	    if ( selection_bottom < selection_top ) {
	    swap_temp = selection_top;
	    selection_top = selection_bottom;
	    selection_bottom = swap_temp;
	    }
	    if ( selection_right < selection_left ) {
	    swap_temp = selection_left;
	    selection_left = selection_right;
	    selection_right = swap_temp;
	    }
	    */ 

	  if (selection_top > selection_bottom) {
	      y = selection_bottom;
	      h = selection_top - selection_bottom;
	    } else {
	      y = selection_top;
	      h = selection_bottom - selection_top;
	    }
	  if (selection_left > selection_right) {
	      x = selection_right;
	      w = selection_left - selection_right;
	    } else {
	      x = selection_left;
	      w = selection_right - selection_left;
	    }
	  XDrawRectangle(dpy, big_bit_xid, three_gc, 
			 x, y, w, h);
	  b_draw_box(selection_left, selection_top, 
		     selection_right, selection_bottom);
	  wiping_on = FALSE;
	}
      else
	{
	  
	  selection_bottom = 
	    selection_top = 
	      ( ( event_y(event) - boarder_left ) / width_block ) 
		* width_block
		  + boarder_left + 1 + ( width_block / 2 );

	  selection_left = 
	    selection_right = 
	      ( ( event_x(event) - boarder_upper ) / height_block ) 
		* height_block 
		  + boarder_upper + 1 + ( height_block / 2 );

/* Does this do anything?  I don't think so.  */
/*	  
	  XDrawRectangle(dpy, big_bit_xid, three_gc, 
			 selection_left, selection_top, 
			 selection_right - selection_left, 
			 selection_bottom - selection_top); 
 */
	  wiping_on = TRUE;
	}
      return(NOTIFY_DONE);
    }
  else if (event_id(event) == LOC_DRAG)
    {
      if (wiping_on)
	{
	  if (selection_top > selection_bottom) {
	    y = selection_bottom;
	    h = selection_top - selection_bottom;
	  } else {
	    y = selection_top;
	    h = selection_bottom - selection_top;
	  }
	  if (selection_left > selection_right) {
	    x = selection_right;
	    w = selection_left - selection_right;
	  } else {
	    x = selection_left;
	    w = selection_right - selection_left;
	  }
	  XDrawRectangle(dpy, big_bit_xid, three_gc, 
			 x, y, w, h);
	  selection_bottom = ( ( event_y(event) - boarder_left ) 
			      / width_block ) * width_block  
				+ boarder_left + 1 
				  + ( width_block / 2 );
	  selection_right = ( ( event_x(event) - boarder_upper ) 
			     / height_block ) * height_block 
			       + boarder_upper + 1 
				 + ( height_block / 2 );
	  if (selection_top > selection_bottom) {
	    y = selection_bottom;
	    h = selection_top - selection_bottom;
	  } else {
	    y = selection_top;
	    h = selection_bottom - selection_top;
	  }
	  if (selection_left > selection_right) {
	    x = selection_right;
	    w = selection_left - selection_right;
	  } else {
	    x = selection_left;
	    w = selection_right - selection_left;
	  }
	  XDrawRectangle(dpy, big_bit_xid, three_gc, 
			 x, y, w, h);
	}
    }
}


void
b_draw_box(x1, y1, x2, y2)
int x1;
int y1;
int x2;
int y2;
{

  long pixel, temp_pixel, pixel_old;

  int corner_x1, corner_y1, corner_x2, corner_y2;
  int actual_x1, actual_y1, actual_x2, actual_y2;
  int x, y, w, h;

  Menu edit_menu;

    {
    
    corner_x1 = (( x1 - boarder_left ) / width_block ) * width_block 
      + boarder_left + 1;
    corner_y1 = (( y1 - boarder_upper ) / height_block ) * height_block 
      + boarder_upper + 1;

    actual_x1 = ( ( corner_x1 - boarder_left ) / height_block );
    actual_y1 = ( ( corner_y1 - boarder_upper ) / width_block );

    corner_x2 = ( ( x2 - boarder_left ) / width_block ) * width_block
      + boarder_left +1;
    corner_y2 = ( ( y2 - boarder_upper ) / height_block ) * height_block 
      + boarder_upper + 1;

    actual_x2 = ( corner_x2 - boarder_left ) / height_block;
    actual_y2 = ( corner_y2 - boarder_upper ) / width_block;

    x = (actual_x1 > actual_x2) ? actual_x2 + preview_boarder_left : actual_x1 
      + preview_boarder_left;
    y = (actual_y1 > actual_y2) ? actual_y2 + preview_boarder_upper : actual_y1
      + preview_boarder_upper;
    w = (actual_x1 > actual_x2) ? actual_x1 - actual_x2 : actual_x2 
      - actual_x1;
    h = (actual_y1 > actual_y2) ? actual_y1 - actual_y2 : actual_y2 
      - actual_y1;

    if (fill_pattern == 0)
      {
	XDrawRectangle(dpy, preview_xid, fill_gc, x, y, w, h);
	XDrawRectangle(dpy, preview_pixmap, fill_gc, x, y, w, h);
      }
    else
      {
	XFillRectangle(dpy, preview_xid, fill_gc, x, y, w+1, h+1);
	XFillRectangle(dpy, preview_pixmap, fill_gc, x, y, w+1, h+1);
      }

    common_paint_proc();

  }
}


