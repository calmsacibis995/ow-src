#ifndef lint
static char sccsid[] = "@(#)state.c 1.51 96/06/18";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

#include <unistd.h>
#include <math.h>
#include <xview/panel.h>
#include <string.h>
#include "display.h"
#include "image.h"
#include "imagetool.h"
#include "state.h"
#include "ui_imagetool.h"

extern int       scroll_height;
extern int       scroll_width;
extern XColor    white_color;

StateInfo	*current_state;

void	identity_xform(), rotate_xform(), zoom_xform(), swap_matrix(),
	hflip_xform(), vflip_xform(), matrix_mul(), matrix_copy();

void	vflip_notify_proc(), hflip_notify_proc();

StateInfo *
init_state (type)
    FileTypes  type;
{
    StateInfo	*tmp_state = (StateInfo *) calloc (1, sizeof (StateInfo));

    if (type == POSTSCRIPT || type == EPSF)
      tmp_state->zoom = ((float) prog->def_ps_zoom) / 100.0;
    else
      tmp_state->zoom = 1.0;
    tmp_state->rotate_amt = 0;
    tmp_state->angle = 0;
    tmp_state->frontside = TRUE;
    tmp_state->hflip = OFF;
    tmp_state->vflip = OFF;
    (tmp_state->undo).op = NO_UNDO;
    (tmp_state->undo).zoom_amt = 0.0;
    tmp_state->pan = OFF;
    tmp_state->select = OFF;
    tmp_state->image_selected = FALSE;
    tmp_state->set_roi = FALSE;
    (tmp_state->sel_rect).r_top = 0;
    (tmp_state->sel_rect).r_left = 0;
    (tmp_state->sel_rect).r_width = 0;
    (tmp_state->sel_rect).r_height = 0;
    (tmp_state->save_rect).top_x = 0;
    (tmp_state->save_rect).top_y = 0;
    (tmp_state->save_rect).new_x = 0;
    (tmp_state->save_rect).new_y = 0;
    (tmp_state->save_rect).rect.r_top = 0;
    (tmp_state->save_rect).rect.r_left = 0;
    (tmp_state->save_rect).rect.r_width = 0;
    (tmp_state->save_rect).rect.r_height = 0;
    tmp_state->old_x = 0;
    tmp_state->old_y = 0;
    tmp_state->currentx = 0;
    tmp_state->currenty = 0;
    tmp_state->current_page = 1;
    tmp_state->next_page = 0;
    tmp_state->reversed = FALSE;
    tmp_state->using_dsc = FALSE;
    tmp_state->timeout_hit = FALSE;
    identity_xform(tmp_state->xform);
    identity_xform(tmp_state->old_xform);

/*
 * Keep same state as what's selected
 * on the palette.
 */
    if (palette) {
      if (xv_get(palette->select, PANEL_TOGGLE_VALUE, 0 ))
	tmp_state->select = ON;
      if (xv_get(palette->pan, PANEL_TOGGLE_VALUE, 0 ))
        tmp_state->pan = ON;
    }
    
    return (tmp_state);
}

void
revert_state ()
{
    if (current_display == ps_display)
      current_state->zoom = ((float) prog->def_ps_zoom) / 100.0;
    else
      current_state->zoom = 1.0;
    current_state->rotate_amt = 0;
    current_state->angle = 0;
    current_state->frontside = TRUE;
    current_state->hflip = OFF;
    current_state->vflip = OFF;
    (current_state->undo).op = NO_UNDO;
    (current_state->undo).zoom_amt = 0.0;
    current_state->image_selected = FALSE;
    (current_state->save_rect).top_x = 0;
    (current_state->save_rect).top_y = 0;
    (current_state->save_rect).new_x = 0;
    (current_state->save_rect).new_x = 0;
    (current_state->save_rect).rect.r_top = 0;
    (current_state->save_rect).rect.r_left = 0;
    (current_state->save_rect).rect.r_width = 0;
    (current_state->save_rect).rect.r_height = 0;
    current_state->old_x = 0;
    current_state->old_y = 0;
    current_state->currentx = 0;
    current_state->currenty = 0;

    identity_xform(current_state->xform);
    identity_xform(current_state->old_xform);
}

 
static void
rotate_matrix( angle, rmatrix )
    int angle;            /* Rotation angle in degrees */
    float rmatrix[3][3];
{
    rmatrix[0][0] = cos( RADIANS(angle) * -1.0);
    rmatrix[0][1] = sin( RADIANS(angle) * -1.0);
    rmatrix[0][2] = 0.0;
 
    rmatrix[1][0] = -sin( RADIANS(angle) * -1.0 );
    rmatrix[1][1] = cos( RADIANS(angle)  * -1.0);
    rmatrix[1][2] = 0.0;
 
    rmatrix[2][0] = 0.0;
    rmatrix[2][1] = 0.0;
    rmatrix[2][2] = 1.0;
}

static void
fmult( a, b, c )
   float  a[];
   float  b[][3];
   float  c[];
{
   c[0] = ( a[0] * b[0][0] ) + ( a[1] * b[1][0] ) + ( a[2] * b[2][0] );
   c[1] = ( a[0] * b[0][1] ) + ( a[1] * b[1][1] ) + ( a[2] * b[2][1] );   
   c[2] = ( a[0] * b[0][2] ) + ( a[1] * b[1][2] ) + ( a[2] * b[2][2] );    
}

static u_short
y_magnitude( a, b )
    float a[3],b[3];
{
    return( abs(a[1] - b[1]) );
}

static u_short
x_magnitude( a, b )
    float a[3],b[3];
{
    return( abs(a[0] - b[0]) );
}

void
get_dimensions (old_width, old_height, new_width, new_height)
    unsigned int   old_width;
    unsigned int   old_height;
    unsigned int  *new_width;
    unsigned int  *new_height;
{
    unsigned int   width,
		   height;
    float        t[3][3];
    float        corner1[3], new_corner1[3],
                 corner2[3], new_corner2[3],
                 corner3[3], new_corner3[3],
                 corner4[3], new_corner4[3];

    rotate_matrix(current_state->angle, &t[0][0]); 

    corner1[0] = corner1[1] = corner1[2] = 0.0;
    corner2[0] = (float)old_width; corner2[1] = corner2[2] = 0.0;
    corner3[0] = (float)old_width; corner3[1] = (float)old_height; corner3[2] = 0.0;
    corner4[0] = 0.0; corner4[1] = (float)old_height; corner4[2] = 0.0;
    
    fmult( corner1, &t, new_corner1 );	
    fmult( corner2, &t, new_corner2 );	
    fmult( corner3, &t, new_corner3 );	
    fmult( corner4, &t, new_corner4 );	
    
    if ( (current_state->angle == 0) || 
	((current_state->angle > 90 ) && (current_state->angle <= 180)) ||
	((current_state->angle > 270) && (current_state->angle <= 360)) ) {
      width  = x_magnitude (new_corner2, new_corner4);
      height = y_magnitude (new_corner1, new_corner3);
    }
    else if ( ((current_state->angle > 0) && (current_state->angle <= 90)) ||
	      ((current_state->angle > 180) && (current_state->angle <= 270)) ) {
      width =  x_magnitude (new_corner1, new_corner3);
      height = y_magnitude (new_corner2, new_corner4);
    }

    *new_width  = width;
    *new_height = height;
}

void
resize_canvas ()
{
    int  canvas_width, canvas_height;
    int  frame_width, frame_height, panel_height;
    int  display_width, display_height;
    int  length;
    int  size_changed = FALSE;
 
    canvas_width = xv_get (current_display->canvas, CANVAS_WIDTH);
    canvas_height = xv_get (current_display->canvas, CANVAS_HEIGHT);
    frame_width = xv_get (base_window->base_window, XV_WIDTH);
    frame_height = xv_get (base_window->base_window, XV_HEIGHT);
    panel_height = xv_get (base_window->base_panel, XV_HEIGHT);
    display_height = frame_height - panel_height - scroll_height - 2;
    display_width = frame_width - scroll_width - 2;

    if (current_image->width + current_state->currentx > canvas_width) {
      if (display_width <= current_image->width + current_state->currentx) {
        current_state->currentx = canvas_width - current_image->width;
        if (current_state->currentx < 0)
          current_state->currentx = 0;
      }
      xv_set (current_display->canvas,
	      CANVAS_WIDTH, current_image->width + current_state->currentx, 
	      NULL);
      size_changed = TRUE;
    }
    else if (current_image->width + current_state->currentx < canvas_width) {

      length = current_image->width + current_state->currentx;  
      if (length <= display_width)
        length = display_width;
 
       xv_set (current_display->canvas,
	      CANVAS_WIDTH, length,
	      NULL);
      size_changed = TRUE;
    }

    if (current_image->height + current_state->currenty > canvas_height) {
      if (display_height <= current_image->height + current_state->currenty) {
        current_state->currenty = canvas_height - current_image->height;
        if (current_state->currenty < 0)
          current_state->currenty = 0;
      }
      xv_set (current_display->canvas, 
	      CANVAS_HEIGHT, current_image->height + current_state->currenty, 
	      NULL);
      size_changed = TRUE;
    }
    else if (current_image->height + current_state->currenty < canvas_height) {
      length = current_image->height + current_state->currenty;
      if (length <= display_height)
        length = display_height;
      xv_set (current_display->canvas, 
	      CANVAS_HEIGHT, length,
	      NULL);
      size_changed = TRUE;
    }

    if ((size_changed) && (current_display == image_display)) {
      if (image_display->display_win) 
        xil_destroy (image_display->display_win);
      image_display->display_win = 
	xil_create_from_window (image_display->state, 
				image_display->xdisplay, image_display->win);
    }
}

void
image_turnover_func (image)
    XilImage     image;
{
    XilImage      tmp_image;
    XilDataType   datatype;
    unsigned int  width, height, nbands;

/*
    if (current_state->frontside)
      return;

    xil_get_info (image, &width, &height, &nbands, &datatype);
    tmp_image = xil_create (current_display->state, width, height, nbands, datatype);
    xil_set_value (tmp_image, current_display->background);
    xil_transpose (image, tmp_image, XIL_FLIP_Y_AXIS); 

    if (current_image->view_image != NULL)
      xil_destroy (current_image->view_image);

    current_image->view_image = tmp_image;
*/
}

void
image_zoom_func ()
{
    float	  tmp_m[6];

    if (current_state->zoom != 1.0) {
	zoom_xform(current_state->zoom_amt, current_state->zoom_amt, tmp_m);
	matrix_copy(current_state->xform, current_state->old_xform);
	matrix_mul(tmp_m, current_state->xform, current_state->xform);
    }
}

void
image_rotate_func()
{
    float	  tmp_m[6];

    rotate_xform(current_state->rotate_amt, tmp_m);
    matrix_copy(current_state->xform, current_state->old_xform);
    matrix_mul(tmp_m, current_state->xform, current_state->xform);
}

void
image_vflip_func ()
{
    float	  tmp_m[6];

    vflip_xform(tmp_m);
    matrix_copy(current_state->xform, current_state->old_xform);
    matrix_mul(tmp_m, current_state->xform, current_state->xform);
}

void
image_hflip_func ()
{
    float	  tmp_m[6];

    hflip_xform(tmp_m);
    matrix_copy(current_state->xform, current_state->old_xform);
    matrix_mul(tmp_m, current_state->xform, current_state->xform);
}

static void
find_bg_color (image)
  ImageInfo *image;
{
  int     i, index = 0;
  XColor  colors[256];
  
  if (image->depth == 24) {
    image_display->background[0] = 255.0;
    image_display->background[1] = 255.0;
    image_display->background[2] = 255.0;
  }
  else if (image->depth == 8) {

    for (i = image->offset; i < 256; i++, index++) {
      colors[index].pixel = i;
      colors[index].red = (image->red[index] << 8) + image->red[index];
      colors[index].green = (image->green[index] << 8) + image->green[index];
      colors[index].blue = (image->blue[index] << 8) + image->blue[index];
    }

/*
 * If saving to 8, get the index.
 * else put closest to 255, 255, 255.
 */
    image_display->background[0] = 
      (float) closest_match (&white_color, colors, 256 - image->offset);
  }
  else if (image->depth == 1) 
    image_display->background[0] = 0.0;

}

void
regenerate_image (image)
    ImageInfo  *image;
{
    XilDataType   datatype;
    unsigned int  width, height, nbands;
    XilImage      save_image;
    int           save_width, save_height;
    float         bg0, bg1, bg2;
    unsigned int  scanline_stride, pixel_stride;
    u_char       *tmp_data;
/*
 * Save a copy of current_image->view_image beause
 * it gets destroyed in the turnover funcs.
 */
    save_width = current_image->width;
    save_height = current_image->height;
    bg0 = current_display->background[0];
    bg1 = current_display->background[1];
    bg2 = current_display->background[2];

    if (current_state->angle != 0) 
      find_bg_color (image);
  
/*
 * Save the view image.
 */
    xil_get_info (current_image->view_image, &width, &height, &nbands, 
		  &datatype);
    save_image = xil_create (image_display->state, width, height, nbands,
			     datatype);
    xil_copy (current_image->view_image, save_image);
    
/*
 * Using current_image->view_image as placeholder here.
 * Assuming current_image->view is not needed
 * when this is called.
 */

    (*current_image->zoom_func) (image->revert_image);
    (*current_image->turnover_func) (current_image->view_image);
    (*current_image->rotate_func) (current_image->view_image);
 
    image->width = current_image->width;
    image->height = current_image->height;
 
/*
 * Copy into image->view_image.
 */
    xil_get_info (current_image->view_image, &width, &height, &nbands, &datatype);
    image->view_image = xil_create (image_display->state, width, height, 
				    nbands, datatype);
    xil_copy (current_image->view_image, image->view_image);
/* 
 * Recalculate bytes_per_line.
 */
    tmp_data = retrieve_data (image->view_image, &pixel_stride,
			      &scanline_stride);
    image->bytes_per_line = scanline_stride;
    xil_import (image->view_image, FALSE);

/* 
 * Restore.
 */
    current_image->width = save_width;
    current_image->height = save_height;
    current_image->view_image = save_image;
    current_display->background[0] = bg0;
    current_display->background[1] = bg1;
    current_display->background[2] = bg2;

}
 
void
reset_select()
{
   Rect  r;
  
   if (xv_get(palette->select, PANEL_TOGGLE_VALUE, 0 )) 
     xv_set(palette->select, PANEL_TOGGLE_VALUE, 0, FALSE, NULL );
/*
 * Erase the ROI.
 */
    if (current_state && (current_state->image_selected == TRUE)) 
      drawbox ((current_state->save_rect).top_x, 
	       (current_state->save_rect).top_y, 
	       (current_state->save_rect).new_x, 
	       (current_state->save_rect).new_y, 
	       &r, FALSE );

   current_state->select = OFF;
   reset_cursor();
   if ( current_state->image_selected == TRUE) {
     current_state->image_selected = FALSE;
     set_save_selection( OFF );
   }
   if (saveasfc && (xv_get (saveasfc->saveasfc, XV_SHOW) == TRUE)) {
     char  *label;
     label = (char *) xv_get (saveasfc->saveasfc, XV_LABEL);
     if (strcmp (label, LGET ("Image Tool:  Save Selection As")) == 0)
       if (current_display == ps_display)
         xv_set (saveasfc->saveasfc,
		 XV_LABEL, LGET ("Image Tool:  Save Page As Image"), NULL);
       else
         xv_set (saveasfc->saveasfc,
		 XV_LABEL, LGET ("Image Tool:  Save As"), NULL);
   }

   xv_set( base_window->base_window, FRAME_LEFT_FOOTER, DGET(""), NULL );
}

void
reset_pan()
{

  if (xv_get(palette->pan, PANEL_TOGGLE_VALUE, 0 )) 
    xv_set(palette->pan, PANEL_TOGGLE_VALUE, 0, FALSE, NULL );

  current_state->pan = OFF;
  reset_cursor();
  xv_set( base_window->base_window, FRAME_LEFT_FOOTER, DGET(""), NULL );

}


void
image_revert_func ()
{
    XilDataType   datatype;
    unsigned int  width, height, nbands;

    xil_get_info (current_image->orig_image, &width, &height, 
		  &nbands, &datatype);
    if (current_image->dest_image) {
	xil_destroy(current_image->dest_image);
    }
    current_image->dest_image = xil_create_copy(current_image->orig_image,
	0, 0, width, height, 0, nbands);

    if (current_image->view_image) {
	xil_destroy (current_image->view_image);
    }
    current_image->view_image = (current_image->display_func)(
	current_image->dest_image);

    current_image->width = width;
    current_image->height = height;
    current_image->nbands = nbands;
    current_image->datatype = datatype;
}

/*
 * Callback for Revert.
 */
void
revert_notify_proc( item, event)
    Panel_item  item;
    Event      *event;
{
    xv_set (palette->palette, FRAME_LEFT_FOOTER, DGET (""), NULL);

    setbusy();

    revert_state ();
    set_undo_option (OFF);
    set_zoom_and_rotate ();

    (*current_image->revert_func)();

    if (current_display == ps_display) 
       fit_frame_to_image (current_image);

    display_new_image();

    setactive();
}

void 
undo_callback (menu, item)
Menu 	menu;
Panel_item item;
{
    int i;
    float tmp;
    extern void zoom_slider_proc();

    if (current_display == ps_display) {
	switch ((current_state->undo).op) {
	   case HFLIP:    hflip_notify_proc (NULL, NULL);
			  break;
	   case VFLIP:    vflip_notify_proc (NULL, NULL);
			  break;
	   case ROTATE_R: 
	   case ROTATE_L: setbusy ();

			  if ((current_state->undo).op == ROTATE_R) {
			    current_state->angle += current_state->rotate_amt;
			    if (current_state->angle >= 360)
			      current_state->angle -= 360;
			    (current_state->undo).op = ROTATE_L;
			  }
			  else {
			    current_state->angle -= current_state->rotate_amt;
			    if (current_state->angle < 0)
			      current_state->angle += 360;
			    (current_state->undo).op = ROTATE_R;
			  }

			  (*current_image->zoom_func) (current_image->orig_image);
			  (*current_image->turnover_func) 
					    (current_image->view_image);
			  (*current_image->rotate_func) (current_image->view_image);
			  display_new_image();
			  setactive ();
			  break;
	   case ZOOM:     setbusy ();
			  tmp = (current_state->undo).zoom_amt;
			  (current_state->undo).zoom_amt = current_state->zoom;
			  current_state->zoom = tmp;

			  (*current_image->zoom_func) (current_image->orig_image);
			  (*current_image->turnover_func) 
					    (current_image->view_image);
			  (*current_image->rotate_func) (current_image->view_image);

			  xv_set (palette->zoom_slider, PANEL_VALUE, 
				  (int) (current_state->zoom * 100.0), NULL);
			  zoom_slider_proc (NULL, (int) (current_state->zoom * 100), NULL);
			  display_new_image();
			  setactive ();
	   }
	} else {
	    swap_matrix(current_state->xform, current_state->old_xform);
	    display_new_image();
	}

}

void
pan_notify_proc( item, event )
  Panel_item  item;
  Event      *event;
{

  xv_set (palette->palette, FRAME_LEFT_FOOTER, DGET (""), NULL);
  reset_select();

/*
 * If deselecting pan, reset it.
 */
  if (xv_get (palette->pan, PANEL_TOGGLE_VALUE, 0) == 0) 
    reset_pan();

  else {

/*
 * Selected pan so display message in frame.
 */
    xv_set( base_window->base_window, FRAME_LEFT_FOOTER,
            EGET( "Drag SELECT in View window to pan image.\n" ), NULL );
/*
 * Change the cursor to pan.
 */
    xv_set (current_display->paint_win,
            WIN_CURSOR,     cursor->pan_cursor,
            NULL);

    current_state->pan = ON;
  }

}

void
rotate_slider_proc (item, value, event)
Panel_item	 item;
int		 value;
Event		*event;
{
    xv_set (palette->rotate_value, PANEL_VALUE, value, NULL);
}

void
zoom_slider_proc (item, value, event)
Panel_item	 item;
int		 value;
Event		*event;
{
    xv_set (palette->zoom_value, PANEL_VALUE, value, NULL);
}


void
identity_xform(m)
    float *m;
{
    m[0] = m[3] = 1.0;
    m[1] = m[2] = m[4] = m[5] = 0.0;
}

void
rotate_xform(angle, m)
    int angle;		/* rotation angle in degrees */
    float *m;
{
    double r = RADIANS(angle);

    m[0] = m[3] = (float)cos(r);
    m[1] = (float)-sin(r);
    m[2] = -m[1];
    m[4] = m[5] = 0.0;
}

void
zoom_xform(xfac, yfac, m)
    float xfac, yfac;
    float *m;
{
    m[0] = xfac;
    m[3] = yfac;
    m[1] = m[2] = m[4] = m[5] = 0.0;
}

void
hflip_xform(m)
    float *m;
{
    m[0] = 1.0;
    m[3] = -1.0;
    m[1] = m[2] = m[4] = m[5] = 0.0;
}

void
vflip_xform(m)
    float *m;
{
    m[0] = -1.0;
    m[3] = 1.0;
    m[1] = m[2] = m[4] = m[5] = 0.0;
}

void
matrix_mul(m1, m2, m3)
    float *m1, *m2, *m3;	/* 3x2 transforms */
{
    float m[6];

    m[0] = m1[0] * m2[0] + m1[2] * m2[1];
    m[1] = m1[1] * m2[0] + m1[3] * m2[1];
    m[2] = m1[0] * m2[2] + m1[2] * m2[3];
    m[3] = m1[1] * m2[2] + m1[3] * m2[3];
    m[4] = m1[0] * m2[4] + m1[2] * m2[5] + m1[4];
    m[5] = m1[1] * m2[4] + m1[3] * m2[5] + m1[5];

    memcpy(m3, m, sizeof(float)*6);
}

void
matrix_copy(src_m, dst_m)
    float *src_m, *dst_m;
{
    memcpy(dst_m, src_m, sizeof(float)*6);
}

void
swap_matrix(m1, m2)
    float *m1, *m2;
{
    float tmp[6];

    matrix_copy(m1, tmp);
    matrix_copy(m2, m1);
    matrix_copy(tmp, m2);
}

dump_matrix(m)
    float *m;
{
    printf("%fX + %fY + %f\n", m[0], m[2], m[4]);
    printf("%fX + %fY + %f\n", m[1], m[3], m[5]);
    printf("\n");
}

/*
 * Callback from rotate menu.
 */
void
rotate_notify_proc( item, value, event)
    Panel_item  item;
    int         value;
    Event      *event;
{
    int rotate_value;
    int min, max;

/*
 * Verify a valid rotate value.
 */
    rotate_value = (int)xv_get(palette->rotate_value, PANEL_VALUE);
/*
 * Check value in case they didn't
 * hit return on the text line.
 */
    max = (int) xv_get (palette->rotate_value, PANEL_MAX_VALUE);
    min = (int) xv_get (palette->rotate_value, PANEL_MIN_VALUE);

    if (rotate_value < min) {
	rotate_value = min;
	xv_set(palette->rotate_value, PANEL_VALUE, min, NULL);
    } else if (rotate_value > max) {
	rotate_value = max;
	xv_set (palette->rotate_value, PANEL_VALUE, max, NULL);
    }

    xv_set (palette->rotate_slider, PANEL_VALUE, rotate_value, NULL);
/*
 * Return if 0 degrees.
    if (rotate_value == 0) {
	xv_set(palette->rotate, PANEL_TOGGLE_VALUE, value, FALSE, NULL);
	return;
    } else {
	xv_set (palette->palette, FRAME_LEFT_FOOTER, DGET (""), NULL);
    }
 */

/*
 * Reset Select/Pan.
 * This has to be before setbusy() or resetting
 * the cursors won't work.
 */
    reset_select();
    reset_pan();

    setbusy();
    set_undo_option (ON);

/*
 * Compute the angle of rotation.
 * value == 0  Rotate Right
 * value == 1  Rotate Left
 */
    if (value == 0) {   
	(current_state->undo).op = ROTATE_L;
	current_state->angle += rotate_value;
	current_state->rotate_amt = -rotate_value;
	if (current_state->angle >= 360)
	    current_state->angle -= 360;
    } else if (value == 1) {
	(current_state->undo).op = ROTATE_R;
	current_state->angle -= rotate_value;
	current_state->rotate_amt = rotate_value;
	if (current_state->angle < 0)
	    current_state->angle += 360;
    }

    if (current_display == ps_display) {
	(*current_image->zoom_func) (current_image->revert_image);
	(*current_image->turnover_func) (current_image->view_image);
	(*current_image->rotate_func) (current_image->view_image);

	resize_canvas ();

	if (rotate_value == 90)
	   fit_frame_to_image (current_image);
    } else {
	(*current_image->rotate_func) (current_image->view_image);
    }

    display_new_image();

    setactive();
/*
 * Unselect the button when done.
 */
    xv_set( palette->rotate, PANEL_TOGGLE_VALUE, value, FALSE, NULL );
}

/*
 * Notify callback function for changing rotate_value text.
 */
/*ARGSUSED*/
Panel_setting
rotate_value_notify (item, event)
    Panel_item  item;
    Event      *event;
{
    int  rotate_value;

    if (event_is_up (event)) {
	return (PANEL_NONE);
    } else if (event_is_down (event)) {
	rotate_value = (int)xv_get(palette->rotate_value, PANEL_VALUE);
	xv_set(palette->rotate_slider, PANEL_VALUE, rotate_value, NULL);
    }

    switch (event_action (event)) {
	case '\n':
	case '\r':
	    /*
	     * Verify a valid rotate value.
	     */
	    rotate_value = (int) xv_get (palette->rotate_value, PANEL_VALUE);
	    rotate_notify_proc(NULL, NULL, NULL);
	    return(PANEL_NONE);
	break;
	default:
	break;
    }

    return (panel_text_notify (item, event));
}

/*
 * Callback for vertical flip.
 */
void
vflip_notify_proc ( item, value, event )
    Panel_item item;
    int value;
    Event *event;
{
    xv_set (palette->palette, FRAME_LEFT_FOOTER, DGET (""), NULL);
/*
 * Reset Select/Pan.
 * This has to be before setbusy() or resetting
 * the cursors won't work.
 */
    reset_select();
    reset_pan();

    setbusy();
    set_undo_option(ON);

    (current_state->undo).op = VFLIP;

    if (current_state->vflip == ON) {
	current_state->vflip = OFF;
    } else {
	current_state->vflip = ON;
    }

    if (current_display == ps_display) {
	(*current_image->zoom_func) (current_image->revert_image);
	(*current_image->vflip_func) (current_image->view_image);
	(*current_image->rotate_func) (current_image->view_image);
    } else {
	(*current_image->vflip_func) (current_image->view_image);
    }

    display_new_image();

    if (item != NULL)
      xv_set( item, PANEL_TOGGLE_VALUE, value, FALSE, NULL );

    setactive();
}

/*
 * Callback for horizontal flip.
 */
void
hflip_notify_proc ( item, value, event )
    Panel_item item;
    int value;
    Event *event;
{ 
    xv_set (palette->palette, FRAME_LEFT_FOOTER, DGET (""), NULL);
    setbusy();
    set_undo_option(ON);

    (current_state->undo).op = HFLIP;

    if (current_state->hflip == ON) {
	current_state->hflip = OFF;
    } else {
	current_state->hflip = ON;
    }

    if (current_display == ps_display) {
	(*current_image->zoom_func) (current_image->revert_image);
	(*current_image->hflip_func) (current_image->view_image);
	(*current_image->rotate_func) (current_image->view_image);
    } else {
	(*current_image->hflip_func) (current_image->view_image);
    }

    display_new_image();

    if (item != NULL) {
	xv_set( item, PANEL_TOGGLE_VALUE, value, FALSE, NULL );
    }

    setactive();
}

/*
 * Callback from zoom menu.
 */
void
zoom_notify_proc( item, value, event)
    Panel_item  item;
    Event      *event;
{
   int zoom_value;
   int min, max;

/*
 * Verify a valid zoom value.
 */
   xv_set (palette->palette, FRAME_LEFT_FOOTER, DGET (""), NULL);
   zoom_value = (int) xv_get (palette->zoom_value, PANEL_VALUE);
/*
 * Check value in case they didn't
 * hit return on the text line.
 */
    max = (int) xv_get(palette->zoom_value, PANEL_MAX_VALUE);
    min = (int) xv_get(palette->zoom_value, PANEL_MIN_VALUE);
    if (zoom_value < min) {
	zoom_value = min;
    }
    else if (zoom_value > max) {
	zoom_value = max;
    }

    xv_set (palette->zoom_value, PANEL_VALUE, zoom_value, NULL);
    xv_set (palette->zoom_slider, PANEL_VALUE, zoom_value, NULL);
/*
 * Reset Select/Pan.
 * This has to be before setbusy() or resetting
 * the cursors won't work.
 */
    reset_select();
    reset_pan();
        
    setbusy();
    set_undo_option(ON);

    (current_state->undo).op = ZOOM;
    (current_state->undo).zoom_amt = current_state->zoom;
    current_state->zoom_amt = zoom_value * 0.01;
    current_state->zoom *= current_state->zoom_amt;

    (*current_image->zoom_func)(current_image->revert_image);

    if (current_display == ps_display) {
	(*current_image->turnover_func)(current_image->view_image);
	(*current_image->rotate_func)(current_image->view_image);
	resize_canvas ();
    }

    display_new_image();

    setactive();
/*
 * Deselect the selected button.
 */
    xv_set( palette->zoom, PANEL_TOGGLE_VALUE, 0, FALSE, NULL );

}

/*
 * Notify callback function for changing zoom_value text.
 */
/*ARGSUSED*/
Panel_setting
zoom_value_notify (item, event)
    Panel_item  item;
    Event      *event;
{
    int  zoom_value;

    if (event_is_up (event)) {
	return (PANEL_NONE);
    }
    else if (event_is_down (event)) {
	zoom_value = (int)xv_get(palette->zoom_value, PANEL_VALUE);
	xv_set (palette->zoom_slider, PANEL_VALUE, zoom_value, NULL);
    }

    switch (event_action (event)) {
	case '\n':
	case '\t':
	case '\r':
	    /*
	     * Verify a valid zoom value.
	     */
	    zoom_value = (int)xv_get(palette->zoom_value, PANEL_VALUE);
	    zoom_notify_proc(NULL, NULL);
	    return(PANEL_NONE);
	break;

	default:
	break;
    }

    return (panel_text_notify( item, event ));
}
