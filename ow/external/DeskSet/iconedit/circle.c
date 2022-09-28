#include <sys/param.h> /* MAXPATHLEN (include types.h if removed) */
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <math.h>
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

extern int boarder_upper, boarder_lower, boarder_left, boarder_right, height_block, width_block;
extern int preview_boarder_upper, preview_boarder_left;
extern XColor black, current_pen;
extern int current_undo, fill_pattern;
extern int redo_possible, undos_allowed, redos_allowed;
extern int current_color_state;

extern GC three_gc;

extern Undo_struct undo_images[MAX_UNDOS];

/*
 *Undo_struct undo_images[MAX_UNDOS];extern XImage *undo_image[MAX_UNDOS];
 */

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
static int	speed_value = 2;
static int	dash_value = 4;
static Canvas	big_canvas;

extern Display *dpy;

extern GC gc, gc_rv, redraw_gc, fill_gc;
extern Window preview_xid;
extern Pixmap preview_pixmap, big_pixmap;
extern void c_draw_circle();

circle_interpose_proc(canvas, event, arg, type)

Canvas                  canvas;
Event                   *event;
Notify_arg              arg;
Notify_event_type       type;

{

  int x, y, w, h, center_x, center_y, distance, big_number;
  double passing;
  if (event_action(event) == ACTION_SELECT)
    {
      
      if (event_is_up(event))
	{
	  
	  passing = ((selection_top - selection_bottom) 
		     * (selection_top - selection_bottom)) 
	    + ((selection_left - selection_right) 
	       * (selection_left - selection_right));
	  
	  distance = 2 * sqrt(passing);
	  
	  y = selection_top - distance/2;
	  x = selection_left - distance/2;
	  
	  XDrawArc(dpy, big_bit_xid, three_gc, x, y, 
		   distance, distance, 0, 360*64); 
	  
	  c_draw_circle(x, y, distance, distance);
	  wiping_on = FALSE;
	}
      else
	{
	  
	  selection_bottom = selection_top = event_y(event);
	  selection_left = selection_right = event_x(event);
	  passing = ((selection_top - selection_bottom) 
		     * (selection_top - selection_bottom)) 
	    + ((selection_left - selection_right) 
	       * (selection_left - selection_right));
	  
	  distance = 2 * sqrt(passing);
	  
	  y = selection_top - distance/2;
	  x = selection_left - distance/2;
	  
	  XDrawArc(dpy, big_bit_xid, three_gc, x, y, 
		   distance, distance, 0, 360*64); 
	  
	  wiping_on = TRUE;
	}
      return(NOTIFY_DONE);
    }
  else if (event_id(event) == LOC_DRAG)
    
    {
      if (wiping_on)
	{
	  
	  passing = ((selection_top - selection_bottom) 
		     * (selection_top - selection_bottom)) 
	    + ((selection_left - selection_right) 
	       * (selection_left - selection_right));

	  distance = 2 * sqrt(passing);
	  
	  y = selection_top - distance/2;
	  x = selection_left - distance/2;
	  
	  XDrawArc(dpy, big_bit_xid, three_gc, x, y, 
		   distance, distance, 0, 360*64); 
	  
	  selection_bottom = event_y(event);
	  selection_right = event_x(event);
	  
	  passing = ((selection_top - selection_bottom) 
		     * (selection_top - selection_bottom)) 
	    + ((selection_left - selection_right) 
	       * (selection_left - selection_right));

	  distance = 2 * sqrt(passing);
	  
	  y = selection_top - distance/2;
	  x = selection_left - distance/2;
	  
	  XDrawArc(dpy, big_bit_xid, three_gc, x, y,
		   distance, distance, 0, 360*64); 
	  
	}
    }
}


void
c_draw_circle(x1, y1, x2, y2)
int x1;
int y1;
int x2;
int y2;
{

  long pixel, temp_pixel, pixel_old;

  int corner_x1, corner_y1, corner_x2, corner_y2;
  
  int actual_x1, actual_y1, actual_x2, actual_y2, x, y, w, h;

  {
    
    corner_x1 = ((((x1-boarder_left)/width_block)*width_block) 
		 + boarder_left +1);
    corner_y1 = (((y1-boarder_upper)/height_block)*height_block 
		 + boarder_upper +1);
    
    actual_x1 = ((corner_x1 - boarder_left)/height_block);
    actual_y1 = ((corner_y1 - boarder_upper)/width_block);

    w = (x2/height_block);
    h = (y2/width_block);
    
    x = actual_x1 + preview_boarder_left;
    y = actual_y1 + preview_boarder_upper;
    
    if (fill_pattern == 0)
      {
	XDrawArc(dpy, preview_xid, fill_gc, x, y, w, h, 0, 360*64);
	XDrawArc(dpy, preview_pixmap, fill_gc, x, y, w, h, 0, 360*64);
      }
    else
      {
	XFillArc(dpy, preview_xid, fill_gc, x, y, w+1, h+1, 0, 360*64);
	XFillArc(dpy, preview_pixmap, fill_gc, x, y, w+1, h+1, 0, 360*64);
      }
    
    common_paint_proc();
    
  }
}


