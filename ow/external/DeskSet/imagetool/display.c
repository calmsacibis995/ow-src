#ifndef lint
static char sccsid[] = "@(#)display.c 1.71 94/03/18";
#endif

/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

/*
 * display.c - Functions dealing with the display of the images.
 */

#include <xview/canvas.h>
#include <xview/font.h>
#include <xview/notice.h>
#include <xview/panel.h>
#include <xview/scrollbar.h>
#include <xview/xv_xrect.h>
#include "display.h"
#include "image.h"
#include "imagetool.h"
#include "ui_imagetool.h"
#include "state.h"
#include "props.h"
#include "ds_popup.h"
#include "dstt.h"

extern int  scroll_height;
extern int  scroll_width;
extern int  rendering_pages;
extern int  started_thumbnails;

DisplayInfo	*current_display;
DisplayInfo	*ps_display;
DisplayInfo	*image_display;

void
set_current_display ()
{
    if (((current_image->type_info)->type == POSTSCRIPT) ||
	((current_image->type_info)->type == EPSF))
       current_display = ps_display;
    else 
       current_display = image_display;

}

void
set_roi_on_display (roi)
    XilRoi roi;
{
    int	dx = abs (current_state->currentx - current_state->old_x);
    int	dy = abs (current_state->currenty - current_state->old_y);

    if ((dx == 0) && (dy == 0))
	return;

    if (current_state->old_x <= current_state->currentx) {
	if (dx != 0) {
	    xil_roi_add_rect (roi, current_state->old_x, current_state->old_y,
			dx, current_image->height);
	}

	if (dy != 0) {
	    if (current_state->old_y < current_state->currenty) {
		xil_roi_add_rect (roi, current_state->currentx, 
			current_state->old_y, 
			abs (current_image->width - dx), dy);
	    } else {
		xil_roi_add_rect (roi, current_state->currentx, 
			current_state->currenty+current_image->height,
			abs (current_image->width - dx), dy);
	    }
	}
    } else {
	if (dx != 0) {
	    xil_roi_add_rect (roi, current_state->currentx+current_image->width,
 		              current_state->old_y, dx, current_image->height);
	}

	if (dy != 0) {
	    if (current_state->old_y < current_state->currenty) {
		xil_roi_add_rect (roi, current_state->old_x, 
			current_state->old_y, 
			abs (current_image->width - dx), dy);
	    } else {
	      xil_roi_add_rect (roi, current_state->old_x, 
			current_state->currenty+current_image->height,
			abs (current_image->width - dx), dy);
	    } 
	} 
    } 

    xil_set_roi (current_display->display_win, roi);
}

void
update_page ()
{
    int		dx = abs (current_state->currentx - current_state->old_x);
    int		dy = abs (current_state->currenty - current_state->old_y);

/*
printf("NOTE: update_page\n");
printf("%d %d %d %d %d %d\n", current_display->pix_width,
current_display->pix_height, current_state->currentx, current_state->currenty,
current_state->old_x, current_state->old_y);
*/
    if (current_display->depth == 1) {
	XCopyPlane (current_display->xdisplay, current_display->pixmap1, 
		current_display->win, current_display->win_gc, 0, 0, 
		current_display->pix_width, current_display->pix_height, 
		current_state->currentx, current_state->currenty, 1L);
    } else {
	XCopyArea (current_display->xdisplay, current_display->pixmap1, 
		current_display->win, current_display->win_gc, 0, 0, 
		current_display->pix_width, current_display->pix_height, 
		current_state->currentx, current_state->currenty);
    }

    if (current_state->old_x <= current_state->currentx) {
	XFillRectangle (current_display->xdisplay, current_display->win, 
		current_display->fill_gc, current_state->old_x, 
		current_state->old_y, dx, current_display->pix_height);

	if (current_state->old_y < current_state->currenty) {
	    XFillRectangle (current_display->xdisplay, current_display->win, 
		current_display->fill_gc, current_state->currentx, 
		current_state->old_y, current_display->pix_width - dx, dy);
	} else {
	    XFillRectangle (current_display->xdisplay, current_display->win, 
		current_display->fill_gc, current_state->currentx, 
		current_state->currenty + current_display->pix_height,
		current_display->pix_width - dx, dy);
	}
    } else {
	XFillRectangle (current_display->xdisplay, current_display->win, 
		current_display->fill_gc, 
		current_state->currentx + current_display->pix_width, 
		current_state->old_y, dx, current_display->pix_height);

	if (current_state->old_y <= current_state->currenty) {
	    XFillRectangle (current_display->xdisplay, current_display->win, 
		current_display->fill_gc, current_state->old_x, 
		current_state->old_y, current_display->pix_width - dx, dy);
	} else {
	    XFillRectangle (current_display->xdisplay, current_display->win, 
		current_display->fill_gc, current_state->old_x, 
		current_state->currenty + current_display->pix_height,
		current_display->pix_width - dx, dy);
	} 
    }
    XFlush (current_display->xdisplay);
}
     
void
update_image()
{
    XilRoi  roi = NULL;

    if (current_display == image_display) {

/*
 * Clear the backgound only when necessary
 */
	if ((xil_get_width(current_display->display_win) >
	     xil_get_width(current_image->view_image)) ||
	    (xil_get_height(current_display->display_win) >
	     xil_get_height(current_image->view_image))) {

/* 
 * Set ROI on display win if panning.
 */
	    if (current_state->set_roi) {
		roi = xil_roi_create (image_display->state);
		set_roi_on_display (roi);
		xil_set_value(current_display->display_win,
		    current_display->background);
/*
 * Destroy ROI and set back to NULL.
 */
		xil_roi_destroy (roi);
		xil_set_roi (current_display->display_win, NULL);
		current_state->set_roi = FALSE;
	    } else {
		xil_set_value (current_display->display_win,
		    current_display->background);
	    }

	}

	xil_set_origin (current_image->view_image,
	    (float)(0 - current_state->currentx),
	    (float)(0 - current_state->currenty));

	xil_copy (current_image->view_image, current_display->display_win);
	xil_set_origin (current_image->view_image, 0.0, 0.0);
    }
    else
      update_page();
}
 
void
repaint_image (canvas, window, display, xid, xrects)
Canvas           canvas ;               /* Canvas handle */
Xv_Window        window ;               /* Pixwin/Paint window */
Display         *display ;              /* Server connection */
Xv_Window        xid ;                  /* X11 window handle */
Xv_xrectlist    *xrects ;               /* Damage rectlist */
{
    XRectangle   xr;
    Rect         r;
    int          vieww, viewh;
    int          i;
    XilRoi       roi = NULL;

    if ((current_display == NULL) || (prog->frame_mapped == FALSE)) {
	return;
    } else if ((current_image == NULL) && (current_display == image_display)) {
/*
 * Case where no image on canvas and repaint occurs.
 */
	xil_set_value (current_display->display_win,
	    current_display->background);
    } else if (current_display == ps_display) {
/*
 * PS repaint. 
 */
	if (canvas != NULL) {
	    XSetClipRectangles (current_display->xdisplay, 
		current_display->fill_gc, 0, 0, xrects->rect_array, 
		xrects->count, Unsorted);
	}

	XFillRectangle (current_display->xdisplay, current_display->win,
	    current_display->fill_gc, 0, 0, current_display->width, 
		current_display->height); 
    } else if ((current_display == image_display) &&
	(current_image != (ImageInfo *) NULL) && (xrects != NULL)) {
/*
 * Image repaint.
 */
	roi = xil_roi_create (image_display->state);
	if (current_state && current_state->image_selected) {
	    make_rectangle ((current_state->save_rect).top_x,
		(current_state->save_rect).top_y,
		(current_state->save_rect).new_x,
		(current_state->save_rect).new_y, &r);
	    xil_roi_add_rect (roi, r.r_left - 1, r.r_top - 1,
		r.r_width + 2, r.r_height + 2);
	    xil_set_roi (current_display->display_win, roi); 
	}
	for (i = 0; i < xrects->count; i++) {
	    xil_roi_add_rect (roi, xrects->rect_array[i].x, 
		xrects->rect_array[i].y, xrects->rect_array[i].width, 
		xrects->rect_array[i].height);
	}
	xil_set_roi (current_display->display_win, roi);
	xil_roi_destroy (roi);
    }

    if (current_image != (ImageInfo *) NULL)
	update_image();
/*
 * Repaint box if needed.
 */
    if (current_state && (current_state->image_selected == TRUE))
      drawbox ((current_state->save_rect).top_x,
               (current_state->save_rect).top_y,
               (current_state->save_rect).new_x,
               (current_state->save_rect).new_y,
               &r, FALSE );      

    XSetClipMask (current_display->xdisplay, current_display->fill_gc,
                  (Pixmap) None);

    XFlush (current_display->xdisplay);
    if (current_display->display_win)
	xil_set_roi (current_display->display_win, NULL);
}

int
geometry_tt (ttmsg, key, status, w, h, x, y)
Tt_message  	 ttmsg;
void		*key;
int         	 status;
int         	 w, h, x, y;
{
   Rect   rect;

   rect.r_width = w;
   rect.r_height = h;
   rect.r_left = x;
   rect.r_top = y;

   ds_position_popup_rect (&rect, base_window->base_window, DS_POPUP_RIGHT);

   xv_set (base_window->base_window, XV_SHOW, TRUE, NULL);

   return (0);
}

void
fit_frame_to_image (image)
    ImageInfo  *image;
{
    int  frame_width, frame_height;
/*
 * "2" below comes from left and right border width (1 + 1).
 */
    frame_width = image->width + scroll_width + 2;
    frame_height = image->height + xv_get (base_window->base_panel, XV_HEIGHT) + 
                   scroll_height + 2;

    if (frame_width < base_window->min_size.r_width) 
      frame_width = base_window->min_size.r_width;
    else if (frame_width > base_window->max_size.r_width)
      frame_width = base_window->max_size.r_width;

    if (frame_height < base_window->min_size.r_height)
      frame_height = base_window->min_size.r_height;
    else if (frame_height > base_window->max_size.r_height)
      frame_height = base_window->max_size.r_height;

    xv_set (base_window->base_window, XV_WIDTH, frame_width,
 	                              XV_HEIGHT, frame_height, NULL);
/*
 * If we were started by tooltalk, ask for the geometry...
 * NOTE: Removed 7/12/93 - took too much time, and was useless since
 *  	 the window is so large that it's almost impossible for it
 *	 to appear next to the launching application.
 */

    if ((prog->tt_sender != (char *) NULL) && 
        (xv_get (base_window->base_window, XV_SHOW) == FALSE)) 
       xv_set (base_window->base_window, XV_SHOW, TRUE, 
					 XV_X, 20,
					 XV_Y, 0,
					 NULL);

}

/*
 * Done proc.. need to tell snapshot we're exiting (if we were
 * started via tooltalk.
 */

Notify_value
imagetool_done_proc( client, status )
Notify_client	client;
Destroy_status	status;
{
    Frame	frame;
    int		n = 1;

/*
 * If we were started by tooltalk, then set the timer and don't
 * really go away.
 */

    if ((status == DESTROY_CHECKING) && (prog->tt_started == TRUE)) {
       set_tt_timer ();
       notify_veto_destroy (client);
       xv_set (base_window->base_window, XV_SHOW, FALSE, NULL);
       while (frame = xv_get (base_window->base_window, FRAME_NTH_SUBFRAME, n++))
          xv_set (frame, FRAME_CMD_PUSHPIN_IN, FALSE,
                         XV_SHOW,              FALSE,
                         NULL);

       return (NOTIFY_DONE);
       }

    return (notify_next_destroy_func (client, status));
}

/*
 * XErrorHandler
 */
int
xerror_handler( dpy, event )
    Display       *dpy;
    XErrorEvent   *event;
{
    char buffer[128];
    char mesg[128];
    char number[32];
    char *mtype = DGET ("XlibMessage");
    FILE *fp = stderr;
 
    XGetErrorText (dpy, event->error_code, buffer, 128 );
    XGetErrorDatabaseText (dpy, mtype,  DGET ("XError"),  
			   DGET ("X Error (intercepted)"), mesg, 128);
    ( void )fprintf (fp,  DGET( "%s:  %s\n  "), mesg, buffer);
    XGetErrorDatabaseText (dpy, mtype,  DGET ("MajorCode"),  
			   DGET ("Request Major code %d"), mesg, 128);
    ( void )fprintf (fp, mesg, event->request_code);
    sprintf( number, "%d", event->request_code );
    XGetErrorDatabaseText (dpy,  DGET ("XRequest"), number, "", buffer, 128 );
    ( void )fprintf(fp,  " (%s)" , buffer);
    fputs (DGET ("\n"), fp);
    XGetErrorDatabaseText (dpy, mtype, DGET ("MinorCode"),  
			   DGET ("Request Minor code"), mesg, 128 );
    ( void )fprintf (fp, mesg, event->minor_code );
    fputs (DGET ("\n"), fp);
    XGetErrorDatabaseText (dpy, mtype,  DGET ("ResourceID"),  
			   DGET ("ResourceID 0x%x"), mesg, 128);
    ( void )fprintf (fp, mesg, event->resourceid );
    fputs (DGET ("\n"), fp);
    XGetErrorDatabaseText (dpy, mtype, DGET ("ErrorSerial"), 
			   DGET ("Error Serial #%d"), mesg, 128);
    ( void )fprintf (fp, mesg, event->serial);
    fputs (DGET ("\n"), fp);
    XGetErrorDatabaseText (dpy, mtype, DGET ("CurrentSerial"), 
			   DGET ("Current Serial #%d"), mesg, 128);
    ( void )fprintf (fp, mesg, NextRequest (dpy));
    fputs (DGET ("\n"), fp);

    if (event->error_code == BadAlloc) {
      notice_prompt (base_window->base_window, NULL,
		     NOTICE_MESSAGE_STRINGS, 
		     EGET ("Unable to allocate sufficient memory.\nImage Tool cannot continue."), NULL,
		     NOTICE_BUTTON, LGET ("Ok"), 1,
		     NULL);
      exit (1);
    }
/*
 * Force a core dump for stack trace, only if running in verbose mode.
 */
    if (prog->verbose == TRUE)
       abort();
}

/*
 * XILErrorHandler 
 */
Xil_boolean
xil_error_handler (error)
    XilError  error;
{
    if (xil_error_get_category (error) == XIL_ERROR_RESOURCE) {
	notice_prompt (base_window->base_window, NULL,
		     NOTICE_MESSAGE_STRINGS, 
		     EGET ("Unable to allocate sufficient memory.\nImage Tool cannot continue."), NULL,
		     NOTICE_BUTTON, LGET ("Ok"), 1,
		     NULL);
	exit (1);
    } else {
	if (xil_error_get_primary(error)) {
	    fprintf (stderr, MGET ("XIL Error: %s\n"), xil_error_get_string (error));
	}
    }

    return (TRUE);
}

void
setbusy ()
{
    Frame	 frame;
    int		 n = 1;

    xv_set (base_window->base_window, FRAME_BUSY, TRUE, NULL);
    while (frame = xv_get (base_window->base_window, FRAME_NTH_SUBFRAME, n++))
      if (xv_get (frame, XV_SHOW) == TRUE)
	  xv_set (frame, FRAME_BUSY, TRUE, NULL);
}

void
setactive ()
{
    Frame	 frame;
    int		 n = 1;

    while (frame = xv_get(base_window->base_window, FRAME_NTH_SUBFRAME, n++)) {
	if ((pageview != NULL) && (frame == pageview->pageview) && 
	    (rendering_pages == TRUE)) {
	    continue;
	}
	if (xv_get (frame, FRAME_BUSY) == TRUE) {
	    xv_set (frame, FRAME_BUSY, FALSE, NULL);
	}
    }
    xv_set (base_window->base_window, FRAME_BUSY, FALSE, NULL);

    if ((prog->frame_mapped == TRUE) && (current_display == ps_display) &&
	(current_state != (StateInfo *) NULL) && (prog->dps == TRUE) &&
	(started_thumbnails == FALSE)) {
	started_thumbnails = TRUE;
	do_dps_op (0);
    }
}

void
set_icon_label (bw, s)
BaseWindowObjects	*bw;
char			*s;
{
    Icon        icon;
    struct rect text_rect,
               *icon_rect;
    Font        font;
 
 
    if (xview_info->icon_label != (char *) NULL)
       free (xview_info->icon_label);

    xview_info->icon_label = malloc (strlen (s) + 1);
    strcpy (xview_info->icon_label, s);

    icon = xv_get (bw->base_window, FRAME_ICON);
    icon_rect = (Rect *) xv_get (icon, ICON_IMAGE_RECT);
    font = (Font) xv_get (icon, ICON_FONT);

/* adjust icon text top/height to match font height */

    text_rect.r_height = xv_get (font, FONT_DEFAULT_CHAR_HEIGHT);
    text_rect.r_top = icon_rect->r_height - (text_rect.r_height + 2);

/* center the icon text */                          

    text_rect.r_width = strlen (s) * (xv_get (font, FONT_DEFAULT_CHAR_WIDTH));
    if (text_rect.r_width > icon_rect->r_width)
       text_rect.r_width = icon_rect->r_width;
    text_rect.r_left = (icon_rect->r_width - text_rect.r_width) / 2;

    xv_set(icon, XV_LABEL, s,
                 ICON_LABEL_RECT, &text_rect,
                 NULL);

/* xv_set actually makes a copy of all the icon fields */

    xv_set (bw->base_window, FRAME_ICON, icon, NULL);
}



void 
set_labels (file)
char	*file;
{
    char	 label [256];
    char	*s = basename (file);
    char	*f_label = LGET ("File");

    if (xview_info->frame_label_set == FALSE) {
       sprintf (label, "%s %s     %s: %s", prog->name, prog->rel, f_label, s);
       if (xview_info->frame_label != (char *) NULL)
          free (xview_info->frame_label);

       xview_info->frame_label = malloc ( strlen (label) + 1);
       strcpy (xview_info->frame_label, label);

       xv_set (base_window->base_window, 
		FRAME_LABEL,	xview_info->frame_label,
		NULL);
       }

    if (xview_info->icon_label_set == FALSE) 
       set_icon_label (base_window, s);
   
    if (base_window->footer_set != TRUE)
       xv_set (base_window->base_window, FRAME_LEFT_FOOTER, DGET (""), NULL);
}

void
set_zoom_and_rotate ()
{
    if (palette != NULL) {
      if (current_display == ps_display) {
        xv_set (palette->zoom_value,  PANEL_VALUE, prog->def_ps_zoom, NULL);
        xv_set (palette->zoom_slider,  PANEL_VALUE, prog->def_ps_zoom, NULL);
      }
      else {
        xv_set (palette->zoom_value,  PANEL_VALUE, DEFAULT_ZOOM, NULL);
        xv_set (palette->zoom_slider,  PANEL_VALUE, DEFAULT_ZOOM, NULL);
      }
      xv_set (palette->rotate_value,
	      PANEL_VALUE, DEFAULT_ANGLE, NULL);
      xv_set (palette->rotate_slider,  PANEL_VALUE, DEFAULT_ANGLE, NULL);
    }
}

void
set_image_print_options ()
{

/*
 * Should really check # pages, because it seems that some file formats
 * (specifically tiff) might have more than one image in them...
 * REMEMBER TO PUT THIS IN LATER!!!

    if (current_image->pages < 1) 
       xv_set (print->page_range, PANEL_INACTIVE, TRUE, NULL);
    else 
       xv_set (print->page_range, PANEL_INACTIVE, FALSE, NULL);
 */

    xv_set (print->page_range, PANEL_INACTIVE, TRUE, NULL);
    xv_set (print->orientation, PANEL_INACTIVE, FALSE, NULL);
    xv_set (print->size, PANEL_INACTIVE, FALSE, NULL);
    xv_set (print->size_text, PANEL_INACTIVE, FALSE, NULL);
    xv_set (print->size_slider, PANEL_INACTIVE, FALSE, NULL);
    xv_set (print->size_percent, PANEL_INACTIVE, FALSE, NULL);
    xv_set (print->position, PANEL_INACTIVE, FALSE, NULL);
    if (xv_get (print->position, PANEL_VALUE) == MARGINS) {
       xv_set (print->top_margin, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->top_margin_text, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->left_margin, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->left_margin_text, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->units, PANEL_INACTIVE, FALSE, NULL);
       }

}

void
set_ps_print_options ()
{

    xv_set (print->page_range, PANEL_INACTIVE, FALSE, NULL);

/*
 * If print page as image selected...
 */

    if (xv_get (print->page_range, PANEL_TOGGLE_VALUE, 0) == TRUE) {
       xv_set (print->orientation, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->size, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->size_text, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->size_slider, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->size_percent, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print->position, PANEL_INACTIVE, FALSE, NULL);
       if (xv_get (print->position, PANEL_VALUE) == MARGINS) {
          xv_set (print->top_margin, PANEL_INACTIVE, FALSE, NULL);
          xv_set (print->top_margin_text, PANEL_INACTIVE, FALSE, NULL);
          xv_set (print->left_margin, PANEL_INACTIVE, FALSE, NULL);
          xv_set (print->left_margin_text, PANEL_INACTIVE, FALSE, NULL);
          xv_set (print->units, PANEL_INACTIVE, FALSE, NULL);
          }
       }

/*
 * Else print all or specific pages selected, to turn off most items.
 */

    else {
       xv_set (print->orientation, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->size, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->size_text, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->size_slider, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->size_percent, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->position, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->top_margin, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->top_margin_text, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->left_margin, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->left_margin_text, PANEL_INACTIVE, TRUE, NULL);
       xv_set (print->units, PANEL_INACTIVE, TRUE, NULL);

/*
 * If only one page, and Pages was selected, turn it off, and turn on
 * This Page (as image), and also turn on all of the image ops....
 */

       }

}

void
set_tool_options (old_image, new_image)
ImageInfo	*old_image;
ImageInfo	*new_image;
{
    int 	i;
    FileTypes	old_type;

/* 
 * No matter what the type, if the old type was NO_TYPE (no previously
 * loaded image), then let certain menu options appear.
 */ 

    if (old_image == (ImageInfo *) NULL) {
       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVEAS),
		MENU_INACTIVE,	FALSE,
		NULL);

       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, PRINTPREVIEW),
		MENU_INACTIVE,	FALSE,
		NULL);

       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, PRINTONE),
		MENU_INACTIVE,	FALSE,
		NULL);

       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, PRINTD),
		MENU_INACTIVE,	FALSE,
		NULL);

       xv_set (xv_get (base_window->view_menu, MENU_NTH_ITEM, IMAGEINFO),
		MENU_INACTIVE, FALSE,
		NULL);

       xv_set (xv_get (base_window->edit_menu, MENU_NTH_ITEM, PALETTE),
		MENU_INACTIVE,	FALSE,
		NULL);

       xv_set (base_window->drop_target, PANEL_DROP_FULL, TRUE, NULL);

       old_type = NO_TYPE;

       }

    else 
       old_type = (old_image->type_info)->type;

/*
 * If PS file just loaded, and old file wasn't PS, then 
 * unshow certain menu items...
 */

    switch ((new_image->type_info)->type) {
       case POSTSCRIPT:
       case EPSF:
	    if (print != (PrintObjects *) NULL)
	        set_ps_print_options ();

	    if (new_image->pages > 1) {
	       if (prog->dps == TRUE)
	          xv_set(
		    xv_get(base_window->view_menu, MENU_NTH_ITEM, PAGEVIEW),
		    MENU_INACTIVE, TRUE,
		    NULL);
	       else	
		xv_set(
		    xv_get(base_window->view_menu, MENU_NTH_ITEM, PAGEVIEW),
		    MENU_INACTIVE, FALSE,
		    NULL);
	       xv_set (base_window->page_forward_button, PANEL_INACTIVE, FALSE,
				NULL);
	       xv_set (base_window->page_backward_button, PANEL_INACTIVE, TRUE,
				NULL);
	       }
	    else {
	       xv_set (xv_get (base_window->view_menu, MENU_NTH_ITEM, PAGEVIEW),
			MENU_INACTIVE, TRUE,
			NULL);
	       xv_set (base_window->page_forward_button, PANEL_INACTIVE, TRUE,
				NULL);
	       xv_set (base_window->page_backward_button, PANEL_INACTIVE, TRUE,
				NULL);
	       }

	    if ((old_type != POSTSCRIPT) && (old_type != EPSF)) {
  	       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVEAS),       
                        MENU_INACTIVE, TRUE,
                        NULL);
	       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVEPAGEASIMAGE),
			MENU_INACTIVE, FALSE,
			NULL);
  	       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVESELECTIONAS),
                        MENU_INACTIVE, TRUE,
                        NULL);
               xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVE),
                        MENU_INACTIVE, FALSE,
                        NULL);
	       xv_set (xv_get (base_window->view_menu, 
					MENU_NTH_ITEM, PSOPTIONS),
			MENU_INACTIVE, FALSE,
			NULL);

	       if (base_window->canvas != NULL)
	          xv_set (base_window->canvas, WIN_BACK, NULL);

	       }

/*
 * If we were displaying a postscript file, and the page overview pop up
 * is up, then render the small pages. If there is only one page, the
 * take down the page overview pop up.
 */

	    else {
	       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVESELECTIONAS),
		       MENU_INACTIVE, TRUE,
		       NULL);

	       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVEPAGEASIMAGE),
		       MENU_INACTIVE, FALSE,
		       NULL);

/*
 * If dps is true, then we're going to render the thumbnails, so don't do
 * this!
 */

	       if ((pageview != NULL) && (prog->dps == FALSE) &&
		   (xv_get (pageview->pageview, XV_SHOW) == TRUE)) {
		  if (new_image->pages > 1) {
		     pageview_pixmaps_create (new_image->pages);
		     render_small_pages (new_image, FALSE);
		     pageview->pages = pageview_pages_create (pageview,
                                           	pageview->controls2, 	
						new_image->pages);
		     if (initial_reverse (FALSE) == TRUE)
			reverse_pageview_pages (pageview, new_image->pages, 
						TRUE);
		     xv_set (pageview->controls2, XV_SHOW, TRUE, NULL);
		     xv_set (pageview->pages, XV_SHOW, TRUE, NULL);
		     }
		  else 
		     xv_set (pageview->pageview,
				FRAME_CMD_PUSHPIN_IN,	FALSE,
				XV_SHOW,		FALSE,
				NULL);
		  }

/*
 * If the ps options pop up has been created, and only one page then make 
 * the panel item for reverse page order inactive.
 * Also, if we think the pages are in reverse page order, then set order
 * to Last page first
 */

	       if (ps_options != NULL) {
	 	  xv_set (ps_options->orientation, PANEL_VALUE, PORTRAIT, NULL);

/*
 * Note that we set the value here, but then may turn the option off
 * for a while, until we get the total number of pages... 
 * image->pages may be less than zero.
 */

		  if (initial_reverse (FALSE) == TRUE)
		     xv_set (ps_options->order, PANEL_VALUE, 1, NULL);
		  else
		     xv_set (ps_options->order, PANEL_VALUE, 0, NULL);

		  if (new_image->pages > 1) 
		     xv_set (ps_options->order, PANEL_INACTIVE, FALSE, NULL);
		  else 
		     xv_set (ps_options->order, PANEL_INACTIVE, TRUE, NULL);
		  }
	       }


	    break;

       default:
	    if (print != (PrintObjects *) NULL)
	        set_image_print_options ();

	    if ((old_type == POSTSCRIPT) || (old_type == EPSF) ||
		(old_type == NO_TYPE)) {

/*
 * Check to see if pagecounter is running, and if so, stop it.
 */

	       if (old_type != NO_TYPE)
	          check_pagecounter ();
               xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVEAS),
                        MENU_INACTIVE, FALSE,
                        NULL);
               xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, 
							SAVESELECTIONAS),
                        MENU_INACTIVE, TRUE,
                        NULL);

	       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, 
							SAVEPAGEASIMAGE),
			MENU_INACTIVE, TRUE,
			NULL);
	       xv_set (xv_get (base_window->view_menu, MENU_NTH_ITEM, PAGEVIEW),
			MENU_INACTIVE, TRUE,
			NULL);
	       xv_set (base_window->page_forward_button, PANEL_INACTIVE, TRUE,
				NULL);
	       xv_set (base_window->page_backward_button, PANEL_INACTIVE, TRUE,
				NULL);
	       if (pageview != NULL)
		  xv_set (pageview->pageview,
				FRAME_CMD_PUSHPIN_IN,	FALSE,
				XV_SHOW,		FALSE,
				NULL);

	       xv_set (xv_get (base_window->view_menu, 
					MENU_NTH_ITEM, PSOPTIONS),
			MENU_INACTIVE, TRUE,
			NULL);
	       if (ps_options != NULL)
		  xv_set (ps_options->ps_options,
				FRAME_CMD_PUSHPIN_IN,	FALSE,
				XV_SHOW,		FALSE,
				NULL);
/*
 * Turn on Save only if we support this format.
 */
    
  	       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVE),
                        MENU_INACTIVE, FALSE,
                        NULL);

/*
 * Clear out right footer.
 */
    	       xv_set (base_window->base_window, FRAME_RIGHT_FOOTER,
	      	       DGET(""), NULL);

	       if (base_window->ps_canvas != NULL)
	  	  xv_set (base_window->ps_canvas, WIN_BACK, NULL);

	       }
	     else {
               xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVE),
                        MENU_INACTIVE, FALSE,
                        NULL);
               xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVEAS),
                        MENU_INACTIVE, FALSE,
                        NULL);
               xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, 
							SAVESELECTIONAS),
                        MENU_INACTIVE, TRUE,
                        NULL);
 	     }

       }

/*
 * Turn on Save only if this format is supported.
 */
    if (save_format_supported (new_image->type_info) == TRUE)
      xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVE),
	      MENU_INACTIVE, FALSE,
	      NULL);
    else
      xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVE),
	      MENU_INACTIVE, TRUE,
	      NULL);
      
/*
 * Undo turned off... since we haven't done anything yet to the
 * newly loaded file.
 */
    xv_set (xv_get (base_window->edit_menu, MENU_NTH_ITEM, UNDO),
	    MENU_INACTIVE,	TRUE,
	    NULL);

/*
 * Set the labels.. for the frame and icon.
 */

    set_labels (new_image->file);

}

void
set_undo_option (status)
int	status;
{
    if (status == ON)
       xv_set (xv_get (base_window->edit_menu, MENU_NTH_ITEM, UNDO),
		MENU_INACTIVE, FALSE,
		NULL);
    else
       xv_set (xv_get (base_window->edit_menu, MENU_NTH_ITEM, UNDO),
		MENU_INACTIVE, TRUE,
		NULL);
}

void
set_load_args (s)
char	*s;
{
    char 	*tmp_file;
    char	*tmp = basename( s );
 
/* 
 * if tmp = s then we just have the file name, so assume prog->directory 
 * is current directory. 
 */
 
    if ( strcmp (tmp, s) == 0) {
#ifdef FILECHOOSER
       xv_set (openfc, FILE_CHOOSER_DIRECTORY, prog->directory, NULL);
#else
       xv_set( openf->directory, PANEL_VALUE, prog->directory, NULL );
       if (saveas != NULL)
          xv_set( saveas->directory, PANEL_VALUE, prog->directory, NULL );
       if (openas != NULL)
          xv_set( openas->directory, PANEL_VALUE, prog->directory, NULL );
#endif
       }
    else {
       tmp_file = strip_filename (s);
#ifdef FILECHOOSER
       xv_set (openfc, FILE_CHOOSER_DIRECTORY, tmp_file, NULL);
       set_current_doc (openfc, basename (s));
#else
       xv_set( openf->directory, PANEL_VALUE, tmp_file, NULL);
       if (saveas != NULL)
          xv_set( saveas->directory, PANEL_VALUE, tmp_file, NULL);
       if (openas != NULL)
          xv_set( openas->directory, PANEL_VALUE, tmp_file, NULL);
       free (tmp_file);
#endif
       }

    if ( strcmp( tmp, LGET ("(None)")) != 0 ) {
#ifndef FILECHOOSER
       xv_set( openf->file, PANEL_VALUE, tmp, NULL );
       if (openas != NULL)
          xv_set( openas->file, PANEL_VALUE, tmp, NULL );
#endif
       }
        
}

void
set_save_selection (status)
int	status;
{

/*
 * Turn on Save Selection As.
 *   If the Save As or Save Page As Image is showing, change to
 *     Save Selection As.
 */
    if (status == ON) {
       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVESELECTIONAS),
	       MENU_INACTIVE,   FALSE,
	       NULL);
       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVE),
	       MENU_INACTIVE,	TRUE,
	       NULL);
       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVEAS),
	       MENU_INACTIVE,	TRUE,
	       NULL);
       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVEPAGEASIMAGE),
	       MENU_INACTIVE,	TRUE,
	       NULL);
	if (saveasfc &&
	    (xv_get (saveasfc->saveasfc, XV_SHOW) == TRUE)) {
    	  char  *label;

	  label = (char *) xv_get (saveasfc->saveasfc, XV_LABEL);
 	  if (strcmp (label, LGET ("Image Tool:  Save As")) == 0 ||
	      strcmp (label, LGET ("Image Tool:  Save Page As Image")) == 0)
	    xv_set (saveasfc->saveasfc, 
		    XV_LABEL, LGET ("Image Tool:  Save Selection As"), 
		    NULL);
        }
     }
 
/*
 * Turn OFF Save Selection As.
 *   If the Save Selection As is showing, change to:
 *      Save As if image  or
 *      Save Page As Image if PostScript.
 */
    else {
       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVESELECTIONAS),
		MENU_INACTIVE,	TRUE,
		NULL);
       xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVE),
	       MENU_INACTIVE,	FALSE,
	       NULL);

       if (current_display == ps_display)
         xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, 
							SAVEPAGEASIMAGE),
	         MENU_INACTIVE,	FALSE,
	         NULL);
       else
         xv_set (xv_get (base_window->file_menu, MENU_NTH_ITEM, SAVEAS),
	         MENU_INACTIVE,	FALSE,
	         NULL);

	if (saveasfc &&
	    (xv_get (saveasfc->saveasfc, XV_SHOW) == TRUE)) {
    	  char  *label;

	  label = (char *) xv_get (saveasfc->saveasfc, XV_LABEL);
 	  if (strcmp (label, LGET ("Image Tool:  Save Selection As")) == 0)
	    if (current_display == ps_display)
	      xv_set (saveasfc->saveasfc, 
		      XV_LABEL, LGET ("Image Tool:  Save Page As Image"), 
		      NULL);
	    else
	      xv_set (saveasfc->saveasfc, 
		      XV_LABEL, LGET ("Image Tool:  Save As"), 
		      NULL);
        }
    }
}

void
canvas_event_proc (xvwin, event)
Xv_window	 xvwin;
Event		*event;
{

/*
 * NOTE:
 * These events are also caught in base_window_event_proc().
 * Any changes to this code below will also need to be changed
 * in base_window_event_proc() to catch cases where
 * "click to type" is set.
 */
    if ((event_is_ascii(event) || event_is_key_right(event) ||
        (event_action (event) == ACTION_ERASE_CHAR_BACKWARD)) &&
         event_is_down(event) && (current_image != NULL) &&
         (current_display == ps_display)) {
        switch (event_action(event)) {
        case ACTION_GO_DOCUMENT_START:          /* Home */
	case ACTION_GO_LINE_BACKWARD:
            first_page();
            break;
        case ACTION_GO_DOCUMENT_END:            /* End */
 	case ACTION_GO_LINE_END:
            last_page();
            break;
        case '\03':                             /* Cntrl-C */
            exit (3);
        case '\r':                              /* Return */
        case '\12':                             /* Line Feed */
        case ' ':                               /* Space */
        case ACTION_GO_PAGE_FORWARD:            /* PageDown */
            next_page();
            break;
        case '\b':                              /* Cntrl-H */
        case ACTION_ERASE_CHAR_BACKWARD:        /* Backspace */
        case ACTION_GO_PAGE_BACKWARD:           /* PageUp */
            prev_page();
            break;
        default:
            if (prog->verbose)
                fprintf(stderr, 
			MGET("Keyboard: key '%c' %d pressed at %d,%d\n"), 
	                event_action(event), event_action(event),
                        event_x(event), event_y(event));
        }
    } 

    else if (event_is_down (event) && (event_action (event) == ACTION_PROPS)) {
            (void)props_callback (NULL, MENU_NOTIFY);
    }
    else if (event_is_down (event) && (event_action (event) == ACTION_UNDO)) {
            if (xv_get (xv_get (base_window->edit_menu, MENU_NTH_ITEM, UNDO),
                  MENU_INACTIVE) == FALSE)
              undo_callback (NULL, NULL);
    }
    else if ((event_action (event) == WIN_CLIENT_MESSAGE) &&
	     (current_display == ps_display))
       dps_event_func (event->ie_xevent);

/*
    else {
        switch (event_action(event)) {
 *
 * Fix this later...
 * if NOT olwm then...
 *
	case LOC_WINENTER:
	  XInstallColormap (current_display->xdisplay, current_image->cmap);
	  break;

        default:
            return;
        }      
    }
*/

}

Pixmap
create_pixmap (dsp, width, height, depth, set_display)
DisplayInfo *dsp;
int width;
int height;
int depth;
int set_display;
{
    XGCValues   gc_vals;
    Pixmap      pixmap;
    GC          pixmap_gc;
    unsigned long gc_mask;

    if (set_display == TRUE) {
       dsp->pix_height = height;
       dsp->pix_width = width;
       dsp->depth = depth;
       }
    pixmap = XCreatePixmap(dsp->xdisplay, RootWindow(dsp->xdisplay,
                                          DefaultScreen(dsp->xdisplay)),
                           width, height, depth ); 

    gc_vals.function = GXclear;
    gc_mask = GCFunction;
    pixmap_gc = XCreateGC (dsp->xdisplay, pixmap, gc_mask, &gc_vals);
    XFillRectangle (dsp->xdisplay, pixmap, pixmap_gc, 0, 0, width, height);

    XFreeGC (dsp->xdisplay, pixmap_gc);
    XFlush (dsp->xdisplay);

    return (pixmap);
}

void
update_page_number (current, last, old)
int	current;
int	last;
int	old;
{
    static char right[80];
    int len;

    if (last < 0) {
       sprintf (right, LGET ("Page: %d of %d"), current, 0);
       len = strlen (right);
       right [len - 1] = 'x';
       }
    else
       sprintf (right, LGET ("Page: %d of %d"), current, last);

    xv_set (base_window->base_window,
                FRAME_RIGHT_FOOTER, right,
                NULL);

    if (old == abs (last)) {
       if (xv_get (base_window->page_forward_button, PANEL_INACTIVE) == TRUE)
          xv_set (base_window->page_forward_button, PANEL_INACTIVE, FALSE, 
									NULL);
       }
    else if (old == 1) {
       if (xv_get (base_window->page_backward_button, PANEL_INACTIVE) == TRUE)
          xv_set (base_window->page_backward_button, PANEL_INACTIVE, FALSE, 
									NULL);
       }

    if (current == abs (last)) {
       if (xv_get (base_window->page_forward_button, PANEL_INACTIVE) == FALSE)
          xv_set (base_window->page_forward_button, PANEL_INACTIVE, TRUE, NULL);
       }
    else if (current == 1) {
       if (xv_get (base_window->page_backward_button, PANEL_INACTIVE) == FALSE)
          xv_set (base_window->page_backward_button, PANEL_INACTIVE, TRUE, 
									NULL);
       }
}

/*
 * Return current selection on panel list.
 */

int
current_list_selection (list)
Panel_item	list;
{
    register int i, nrows = xv_get( list, PANEL_LIST_NROWS );

  for ( i = 0; i < nrows; i++ )
    if ( list_item_selected( list, i ) )
      return( i );

  return( -1 );
}


