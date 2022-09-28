
#ifndef lint
static char sccsid[] = "@(#)select.c 1.22 94/03/14";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

#include <xview/panel.h>
#include <xview/notice.h>
#include "display.h"
#include "image.h"
#include "imagetool.h"
#include "state.h"
#include "ui_imagetool.h"

void
make_rectangle( x, y, x1, y1, r )
    int  x, y, x1, y1;
    Rect *r;
{
    if ( x > x1 )
       r->r_left = x1;
    else
       r->r_left = x;

    if ( y1 > y )
       r->r_top = y;
    else
       r->r_top = y1;

    r->r_width = abs( x - x1 );
    r->r_height = abs( y - y1 );
          
}

void
drawbox( top_left_x, top_left_y, new_x, new_y, r, erase )
    int   top_left_x, top_left_y, new_x, new_y;
    Rect *r;
    int   erase;
{

/*
 * Erase old box.
 */
    if ( erase ) 
      XDrawRectangle( current_display->xdisplay, current_display->win, 
		      current_display->select_gc, r->r_left, r->r_top,
		      r->r_width, r->r_height );
    
/*
 * Make new Rect and draw it.
 */ 
    make_rectangle( top_left_x, top_left_y, new_x, new_y, r );
    XDrawRectangle( current_display->xdisplay, current_display->win, 
		    current_display->select_gc, r->r_left, r->r_top, 
		    r->r_width, r->r_height );
}

void
reset_cursor()
{
    xv_set( current_display->paint_win,
               WIN_CURSOR, cursor->default_cursor,
	       NULL );
}

void
select_notify_proc( item, event )
  Panel_item  item;
  Event      *event;
{

    reset_pan();

/*
 * If deselecting select, reset it.
 */
    if (xv_get (palette->select, PANEL_TOGGLE_VALUE, 0) == 0)
      reset_select();

    else {
/*
 * Selected select so display message in frame and set cursor.
 */

    xv_set( base_window->base_window, FRAME_LEFT_FOOTER,
	      EGET( "Drag SELECT to select a region within the image." ), NULL );

    xv_set( current_display->paint_win,
              WIN_CURSOR, cursor->select_cursor,
	      NULL );

    current_state->select = ON;
  }

}

XilImage 
crop_region (image, xilimage)
ImageInfo	*image;
XilImage	xilimage;
{
  XilImage  tmp_image = NULL;
  Rect	    r;

/*
 * Make a copy of current rect in case we need to go back.
 */
  r.r_top = current_state->sel_rect.r_top;
  r.r_left = current_state->sel_rect.r_left;
  r.r_width = current_state->sel_rect.r_width;
  r.r_height = current_state->sel_rect.r_height;

  xv_set (base_window->base_window, FRAME_LEFT_FOOTER, DGET(""), NULL);

/*
 * Check for crop region width, height > 0.
 */

  if (r.r_width <= 0 || r.r_height <= 0) {
    display_error (base_window->base_window,
		   EGET ("Selected region too small; select a larger area."));
    return ((XilImage)NULL);
  }

/*
 * Check if entire rect is on image.
 */

  if (r.r_left > current_state->currentx + image->width || 
      r.r_top > current_state->currenty + image->height ||
      r.r_left + r.r_width < current_state->currentx || 
      r.r_top + r.r_height < current_state->currenty) {
    display_error (base_window->base_window,
		   EGET ("Select a region within the image."));
    return ((XilImage)NULL);
  }


/*
 * Adjust the region's top_y left_x if region
 * is larger thant he image.
 */
  if (r.r_left < current_state->currentx) {
    r.r_width = r.r_width - (current_state->currentx - r.r_left);
    r.r_left = current_state->currentx;
  }
  if (r.r_top < current_state->currenty) {
    r.r_height = r.r_height - (current_state->currenty - r.r_top);
    r.r_top = current_state->currenty;
  }

/*
 * Adjust top left of rect relative to currentx and currenty.
 */
  r.r_top = r.r_top - current_state->currenty;
  r.r_left = r.r_left - current_state->currentx;
  
/*
 * Adjust the region's width/height if region
 * is larger than the image.
 */
  if ((r.r_left + r.r_width) > image->width)
    r.r_width = image->width - r.r_left;
  if ((r.r_top + r.r_height) > image->height)
    r.r_height = image->height - r.r_top;

    tmp_image = xil_create_child(xilimage, 
        r.r_left, r.r_top, r.r_width, r.r_height,
	0, xil_get_nbands(xilimage)); 

    return(tmp_image);
}
