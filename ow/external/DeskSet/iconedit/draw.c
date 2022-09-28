#include <stdio.h>

#include <X11/X.h>
#include <X11/Xlib.h>

#include <xview/xview.h>
#include <xview/panel.h>

#include "base_gui.h"
#include "base.h"

#define MONO 1
#define COLOR 0

#define MAX_UNDOS 8
#define MAX_FOOTER_LENGTH 64

extern base_window_objects     *base_window;
extern int	canvas_dimension;

extern Undo_struct undo_images[MAX_UNDOS];

extern int current_color_state; 
extern int current_undo, undos_allowed, redos_allowed, redo_possible;
extern int Edited;

extern int boarder_upper, boarder_lower, boarder_left, boarder_right, height_block, width_block;
extern int icon_height, icon_width, preview_boarder_upper, preview_boarder_left;

extern Display *dpy;
extern Pixmap preview_pixmap, big_pixmap;
extern Window preview_xid, big_bit_xid;

extern GC redraw_gc;


extern XColor black, white, current_pen;


void 
common_paint_proc()
{
  Menu edit_menu;
  long pixel, temp_pixel, pixel_old;
  int x, y, corner_x, corner_y;


  undo_images[current_undo%MAX_UNDOS].state = current_color_state;
  undo_images[current_undo%MAX_UNDOS].height = icon_height;
  undo_images[current_undo%MAX_UNDOS].width  = icon_width;
  current_undo++;
  if (undos_allowed < (MAX_UNDOS -1))
    undos_allowed++;
  
  Edited++;
  undo_images[current_undo%MAX_UNDOS].edited++;
  redos_allowed = 0;
  
  if (undos_allowed == 1)
    {
      edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);

      xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		MENU_INACTIVE, FALSE, 
		NULL);
    }

  set_iconedit_footer();

  undo_images[current_undo%MAX_UNDOS].image = 
	XGetImage(dpy, preview_pixmap, preview_boarder_left, 
		preview_boarder_upper, icon_width, icon_height, 
		-1, ZPixmap);

  temp_pixel = current_pen.pixel;
  
  for (x = 0; x < icon_width; x++)
    {
      corner_x = (x * width_block) + boarder_left;
      for (y = 0; y < icon_height; y++)
	{
	  corner_y = (y * height_block) + boarder_upper;
	  pixel = XGetPixel(undo_images[current_undo%MAX_UNDOS].image, x, y);
	  pixel_old = XGetPixel(undo_images[(current_undo-1)%MAX_UNDOS].image, x, y);
	  if (!(pixel == pixel_old)) 
	    {
	      current_pen.pixel = pixel;
	      XSetForeground(dpy, redraw_gc, current_pen.pixel);
	      XFillRectangle(dpy, big_bit_xid, redraw_gc, corner_x+1, corner_y+1, width_block-1, height_block-1); 
	      XFillRectangle(dpy, big_pixmap, redraw_gc, corner_x+1, corner_y+1, width_block-1, height_block-1);	    
	      XDrawPoint(dpy, preview_pixmap, redraw_gc, x+preview_boarder_left, y+preview_boarder_upper);
	      XDrawPoint(dpy, preview_xid, redraw_gc, x+preview_boarder_left, y+preview_boarder_upper);
	    }
	}
    }

  XSetForeground(dpy, redraw_gc, black.pixel);

  XDrawRectangle(dpy, big_bit_xid, redraw_gc, boarder_left -1, boarder_upper -1, canvas_dimension - ( boarder_left + boarder_right)+1, canvas_dimension - ( boarder_upper + boarder_lower)+1);
  XDrawRectangle(dpy, big_pixmap, redraw_gc, boarder_left -1, boarder_upper -1, canvas_dimension - ( boarder_left + boarder_right)+1, canvas_dimension - ( boarder_upper + boarder_lower)+1);

  current_pen.pixel = temp_pixel;
  
  if (redo_possible == TRUE)
    {
      redo_possible = FALSE;
      
      edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
      xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), MENU_INACTIVE, TRUE, NULL);
      
    }    
}

