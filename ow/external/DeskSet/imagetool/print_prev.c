#include "display.h"
#include "image.h"
#include "ui_imagetool.h"
#include "imagetool.h"
#include "state.h"
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/xv_xrect.h>

extern float page_widths [];
extern float page_heights [];
extern int   set_cmap8();
extern char *get_printer_options ();

/*
 * The xil_image is display on the xil_win.
 */
static XilImage 	xil_win = NULL;
static XilImage 	prev_image = NULL;
static float	 	left = 0.0;
static float	 	top = 0.0;
static float	 	xpages;
static float	 	ypages;
static float	 	page_width;
static float	 	page_height;
static int		center;
static GC	 	prev_gc = NULL;
static Window   	prev_win;
static int		canvas_width;
static int		canvas_height;
static int             min_x;
static int             min_y;
static int             max_x;
static int             max_y;
static unsigned int    xil_width;
static unsigned int    xil_height;

/*
 * Called from resize_print_prev().
 */
void
create_xil_objs (print)
    PrintPreviewObjects *print;
{

    if (xil_win) {
      xil_destroy (xil_win);
      xil_win = NULL;
    }
    if (prev_image) {
      xil_destroy (prev_image);
      prev_image = NULL;
    }

    prev_win = xv_get (canvas_paint_window (print->page), XV_XID);
    xil_win = xil_create_from_window (image_display->state, 
				      current_display->xdisplay, prev_win);

}

void
draw_page_boundaries()
{
   int  i;
/*
 * Draw the outline of the pages if needed...
 */

    for (i = 1; i < (int) xpages; i++)
       XDrawLine (current_display->xdisplay, prev_win, prev_gc,
		  (int) (page_width * 32.0 * i), 0,
		  (int) (page_width * 32.0 * i), canvas_height);

    for (i = 1; i < (int) ypages; i++) 
       XDrawLine (current_display->xdisplay, prev_win, prev_gc,
		  0, (int) (page_height * 32.0 * i),
		  canvas_width, (int) (page_height * 32.0 * i));
}

void
check_printprev_canvas (width, height, canvas)
    int     width;
    int     height;
    Canvas  canvas;
{
/*
 * min and max values based on panning x, y at top-left.
 */
    max_y = canvas_height - height;
    max_x = canvas_width - width;
    min_y = 0;
    min_x = 0;
}

void
set_new_colormap (image, width, height, depth)
    ImageInfo  *image;
    int         width;
    int         height;
    int         depth;
{
  float         mult[1], offset[1]; 
  XilImage      tmp_image;

  image->offset = MAXCOLORS - image->colors;
  set_cmap8 (current_display->xdisplay, prev_win, image, depth);
 
  mult[0] = 1.0;
  offset[0] = (float) image->offset;
  tmp_image = xil_create (image_display->state, width, height, 1, XIL_BYTE);
  xil_rescale (image->orig_image, tmp_image, mult, offset);
  xil_destroy (image->orig_image);
  image->orig_image = tmp_image;
  
}

/*
Get current image, and get width and height.
do the xdpyinfo stuff to get resolution.
Get margin and scaling for printing.
Figure out how big the canvas needs to be.
If print_prev not there, create it.
If there is already print_prev, resize the canvas.
*/

void
position_image ()
{


/*
 * Determine if we need to make the canvas larger (ie. won't
 * fit on a 8.5 x 11 page).
 * NOTE: get info (margins, print height/width) from print
 *       pop up when it exists.
 */

    double	 left_margin;
    double	 top_margin;
    int		 orientation;
    int		 range;
    float	 image_width; 	 
    float	 image_height; 	 
    float	 width_inches;
    float	 height_inches;
    float	 xstart;
    float	 ystart;
    float	 scale_factor;
    float	 image_scale;
    float	 xscale;
    float	 yscale;
    char	*tmp_value;
    float	 tmpf;
    char	*printer;
    int		 copies;
    int		 header;
    ImageInfo   *tmp_imageinfo;
    Window       new_wins[3];
	
    unsigned long gc_mask;
    XGCValues	 gc_vals;
    XilDataType  datatype;
    unsigned int nbands, win_width, win_height;
/*
 * Init XIL if not already initialized.
 */
    if (prog->xil_opened == FALSE) {
      base_window_image_canvas_objects_create (base_window);
      prog->xil_opened = TRUE;
    }
/*
 * Always do everything in inches.. So, get the page_width and
 * page_height from the properties, and convert to inches if
 * necessary.
 */

/*
 * If print pop up not created, use defaults.
 */

    get_print_options (&left_margin, &top_margin, &center, &orientation,
		       &scale_factor, &range);

/* Delay the printer search until user decided to print */
/*
    printer = get_printer_options (&page_width, &page_height, &copies, &header);
*/
    page_width = page_widths [DEFAULT_PAGE_SIZE];
    page_height = page_heights [DEFAULT_PAGE_SIZE];
    copies = 1;
    header = HEADER_PAGE;

/*
 * If orientation is landscape, change the page height and width.
 */

    if (orientation == LANDSCAPE) {
       tmpf = page_height;
       page_height = page_width;
       page_width = tmpf;
       }

    width_inches = (float) current_image->width / current_display->res_x * 
		   scale_factor; 
				/*image_display->xresolution;*/
    image_width = width_inches;
    height_inches = (float) current_image->height / 
		    current_display->res_y * scale_factor; 
				/*image_display->yresolution;*/
    image_height = height_inches;

/*
 * If they are centering the image, then figure out the x and y
 * start position. Otherwise, use the margins (could be zero).
 */

    if (center == CENTERED) {
       xstart = (page_width - image_width) / 2.0 ; 
       ystart = (page_height - image_height) / 2.0 ;

/*
 * If xstart or ystart is negative, then it's too big for one
 * page. In this case, convert to a positive value which is
 * where it will start on the `previous' page.
 */

       if (xstart < 0)
/*	  xstart = page_width - abs (xstart); */
	  xstart = 0;
       if (ystart < 0)
	/*  ystart = page_height - abs (ystart); */
	  ystart = 0;

       }
    else {
       xstart = left_margin;
       ystart = top_margin;
       }

/* 
 * Figure out the total number of pages required to display the
 * image. Could be 1, 2, 4, etc..
 */

    
    xpages = (xstart + image_width) / page_width;
    ypages = (ystart + image_height) / page_height;

    if ( (xpages - (int) xpages) > 0.0)
       xpages += 1.0;

    if ( (ypages - (int) ypages) > 0.0)
       ypages += 1.0;

    resize_print_prev (print_preview, (int) xpages, (int) ypages,
			page_width, page_height);

/*
 * Create the GC for drawing the lines.
 */

    if (prev_gc == (GC) NULL) {
       gc_vals.line_width = 2;
       gc_vals.background = (int) current_display->white[3];
       gc_vals.foreground = current_display->black;
       gc_vals.line_style = LineDoubleDash;
       gc_mask = GCForeground | GCBackground |
	         GCLineWidth  | GCLineStyle;
       prev_gc = XCreateGC (current_display->xdisplay, prev_win,
			    gc_mask, &gc_vals);
       }
    else {
       XSetForeground (current_display->xdisplay, prev_gc, 
		       current_display->black);
       XSetBackground (current_display->xdisplay, prev_gc, 
		       current_display->white);
       }

/*
 * Display the image...
 * Image is to be displayed at left (x), top (y)
 * Image is to be scaled to xscale, yscale;
 * Make sure that the background is white. (when draw the XIL image onto
 *   the paint window).
 */   

    left = xstart * PRINT_PREVIEW_PAGE_FACTOR;
    top = ystart * PRINT_PREVIEW_PAGE_FACTOR;

/*
 * Figure out scaling...
 */

    xscale = (PRINT_PREVIEW_PAGE_FACTOR * image_width) / current_image->width;
    yscale = (PRINT_PREVIEW_PAGE_FACTOR * image_height) / current_image->height;

/*
 * Create temp imageinfo (PS will avoid cmap to be freed).
 * Create an XIL image (from PS).
 * Create a pseudocolor cmap (for PS) and set on preview canvas.
 * Fix later, this should only be for depth==8??
 * Get the nbands, datatype.
 * Calculate the new width, height.
 * Create the new image and display it.
 */
    if (current_display == ps_display) {
       tmp_imageinfo = init_image (DGET("tmp"), DGET("tmp"), 0, 
				   str_to_ftype ("postscript-file"),
				   (char *) NULL);
       tmp_imageinfo->orig_image = create_image_from_ps (tmp_imageinfo);
       xil_get_info (tmp_imageinfo->orig_image, &win_width, &win_height, 
	 	     &nbands, &datatype);
#if 0
       if (current_display->depth == 8 ||
	   current_display->depth == 4) {
         save_colormap (tmp_imageinfo);
         compress_colors (tmp_imageinfo);
         set_new_colormap (tmp_imageinfo, win_width, win_height, current_display->depth);
       }
#endif
    } else {
       xil_get_info (current_image->dest_image, &win_width, &win_height, 
	 	     &nbands, &datatype);
       if (current_display->depth == 8 ||
	   current_display->depth == 4) {
         XSetWindowColormap (current_display->xdisplay, prev_win, current_image->cmap);
       }
    }

    win_width = (int) ((float) win_width * xscale);
    win_height = (int) ((float) win_height * yscale);

    if (current_display == ps_display) {
	if (prev_image) {
	    xil_destroy(prev_image);
	    prev_image = NULL;
	}
	prev_image = xil_create (image_display->state, win_width, win_height, 
		nbands, datatype);
	xil_scale (tmp_imageinfo->orig_image, prev_image, "nearest", xscale, 
		yscale);
	if (tmp_imageinfo)
	    destroy_image (tmp_imageinfo);
    }
    else {
	XilImage tmp_image;
	float m1[6];
	extern void zoom_xform();
	extern void matrix_mul();

	if (prev_image) {
	    xil_destroy(prev_image);
	    prev_image = NULL;
	}
	tmp_image = xil_create (image_display->state, win_width, win_height, 
	    xil_get_nbands(current_image->dest_image), datatype);
	zoom_xform(xscale, yscale, m1);
	matrix_mul(m1, current_state->xform, m1);

	xil_set_origin(current_image->orig_image,
	    (float)xil_get_width(current_image->orig_image)/2.0,
	    (float)xil_get_height(current_image->orig_image)/2.0);
	xil_set_origin(tmp_image,
	    (float)win_width/2.0, (float)win_height/2.0);

	xil_affine(current_image->orig_image, tmp_image,
	    "nearest", m1);

	xil_set_origin(current_image->orig_image, 0.0, 0.0);
	xil_set_origin(tmp_image, 0.0, 0.0);

	prev_image = (current_image->display_func)(tmp_image);
	if (tmp_image) {
	    xil_destroy(tmp_image);
	    tmp_image = NULL;
	}
    }

/*
    xil_threshold (prev_image, prev_image, current_display->background,
		   current_display->background, current_display->white);
*/
    xil_set_value (xil_win, current_display->white);
    xil_set_origin (prev_image, (float)(0 - left), (float) (0 - top));
    xil_copy (prev_image, xil_win);

    canvas_width = xv_get (print_preview->page, CANVAS_WIDTH);
    canvas_height = xv_get (print_preview->page, CANVAS_HEIGHT);
   
    draw_page_boundaries();
/*
 * Set canvas boundaries for panning.
 */
    xil_get_info (prev_image, &xil_width, &xil_height, &nbands, &datatype);
    check_printprev_canvas (xil_width, xil_height, print_preview->page);
/*
 * Set WM_COLORMAP_WINDOWS property.
 */
    new_wins[0] = prev_win;
    new_wins[1] = xv_get (print_preview->print_prev, XV_XID);
    new_wins[2] = xv_get (base_window->base_window, XV_XID);
    XSetWMColormapWindows (current_display->xdisplay, 
			   xv_get (print_preview->print_prev, XV_XID),
		           new_wins, 3);


}

/*
 * Figure out what area to redraw.
 */

void
repaint_print_preview (canvas, window, display, xid, xrects)
Canvas           canvas ;               /* Canvas handle */
Xv_Window        window ;               /* Pixwin/Paint window */
Display         *display ;              /* Server connection */
Xv_Window        xid ;                  /* X11 window handle */
Xv_xrectlist    *xrects ;               /* Damage rectlist */
{
    int i;

    xil_set_origin (prev_image, (0.0 - left), (0.0 - top));
    xil_set_value (xil_win, current_display->white);
    xil_copy (prev_image, xil_win);
/*
 * Draw the outline of the pages if needed...
 */

    for (i = 1; i < (int) xpages; i++)
       XDrawLine (current_display->xdisplay, prev_win, prev_gc,
                  (int) (page_width * 32.0 * i), 0,
                  (int) (page_width * 32.0 * i), canvas_height);

    for (i = 1; i < (int) ypages; i++) 
       XDrawLine (current_display->xdisplay, prev_win, prev_gc,
                  0, (int) (page_height * 32.0 * i),
                  canvas_width, (int) (page_height * 32.0 * i));

}

/*
 * Update the value of Margins in INCHES/CM
 */
void
set_margin_value (value)
   int  value;
{
    float      top_margin;
    float      left_margin;
    char       buf [80];

    if (value == INCHES) {
      top_margin   = top / PRINT_PREVIEW_PAGE_FACTOR;
      left_margin = left / PRINT_PREVIEW_PAGE_FACTOR;
    }
    else if (value == CM) {
      top_margin   = (top / PRINT_PREVIEW_PAGE_FACTOR) * 2.54;
      left_margin = (left / PRINT_PREVIEW_PAGE_FACTOR) * 2.54;
    }

    if (top_margin < 0.0)
      top_margin = 0.0;
    
    if (left_margin < 0.0)
      left_margin = 0.0;
    
   sprintf (buf, "%.1f", top_margin);
   xv_set (print->top_margin, PANEL_VALUE, buf, NULL);
   sprintf (buf, "%.1f", left_margin);
   xv_set (print->left_margin, PANEL_VALUE, buf, NULL);      

}

/*
 * Called after panning on print preview to update 
 * margins fields.
 */

void
update_margins()
{
   extern void  position_notify_proc();
   float        top_margin;
   float        left_margin;
   char         buf [80];

   if ((print == (PrintObjects *) NULL) || (center == CENTERED))
      return;

/*
 * Set Margins on print->position.
 */

    xv_set (print->position, PANEL_VALUE, MARGINS, NULL);
/*
 * Set new values in INCHES/CM.
 */

    set_margin_value (xv_get (print->units, PANEL_VALUE));  

    position_notify_proc (NULL, MARGINS, NULL);
    XSync (image_display->xdisplay, 0);


}
static void
set_roi_on_prev (roi, old_x, curr_x, old_y, curr_y)
    XilRoi  roi;
    int     old_x, curr_x;
    int     old_y, curr_y; 
{
    int     dx = abs (curr_x - old_x);
    int     dy = abs (curr_y - old_y);

    if (old_x < curr_x) {
	if (dx) {
	    xil_roi_add_rect (roi, old_x, old_y, dx, xil_height);
	}
	if (dy) {
	    if (old_y < curr_y)
		xil_roi_add_rect (roi, curr_x, old_y,
			abs (xil_width - dx), dy);
	    else
		xil_roi_add_rect (roi, curr_x, curr_y + xil_height,
			  abs (xil_width - dx), dy);
	}
    } else {
	if (dx) {
	    xil_roi_add_rect (roi, curr_x + xil_width, old_y, dx, xil_height);
	}
	if (dy) {
	    if (old_y < curr_y)
		xil_roi_add_rect (roi, old_x, old_y, abs (xil_width - dx), dy);
	    else
		xil_roi_add_rect (roi, old_x, curr_y + xil_height,
		          abs (xil_width - dx), dy);
	}
    }
    
    xil_set_roi (xil_win, roi);

}

/*
 * Handle the panning.
 */
void
print_preview_pan (win, event)
Xv_Window  win;
Event     *event;
{
    static int   x, y;
    static float old_left, old_top;
    static int   image_moved = FALSE;
    XilRoi       roi;
   
    switch (event_action (event)) {
    case ACTION_SELECT:
    case MS_LEFT:
      if (event_is_down (event)) {
        x = event_x (event);
        y = event_y (event);
      }
      else if (event_is_up (event)) {
        if (image_moved) {
	  center = MARGINS;
	  update_margins();
          draw_page_boundaries();
	  image_moved = FALSE;
        }
      }
      break;
    case LOC_DRAG:
      old_left = left;
      old_top  = top;
      left -= x - event_x (event);
      top  -= y - event_y (event);
      x = event_x (event);
      y = event_y (event);
      if (left < min_x)
        left = min_x;
      if (left > max_x)
        left = max_x;
      if (top < min_y)
        top = min_y;
      if (top > max_y)
        top = max_y;
      
      if ((old_left != left) ||
	  (old_top  != top)) {
        xil_set_origin (prev_image, (float) (0 - (int)left), (float) (0 - (int)top));

	roi = xil_roi_create (image_display->state);
	set_roi_on_prev (roi, (int) old_left, (int) left, 
			      (int) old_top, (int) top);
/*
        xil_roi_add_rect (roi, old_left, old_top, 
			  xil_width, xil_height);
*/
        xil_set_value (xil_win, current_display->white);
        xil_roi_destroy (roi);
        xil_set_roi (xil_win, NULL);
    
        xil_copy (prev_image, xil_win);
        draw_page_boundaries();
        XFlush (image_display->xdisplay);
        image_moved = TRUE;
      }
      break;
    default:
      break;
    }

}
