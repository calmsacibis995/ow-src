
#ifndef lint
static char sccsid[] = "@(#)xv.c 1.86 93/06/30";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

/*
 * xv.c - I think that some of these functions can go away...
 */

#include "image.h"
#include "imagetool.h"
#include "display.h"
#include "state.h"
#include "ui_imagetool.h"
#include "props.h"
#include "ds_popup.h"

#include <xview/dragdrop.h>
#include <xview/frame.h>
#include <xview/panel.h>
#include <xview/scrollbar.h>

extern Dnd			current_dnd_object;
extern Selection_requestor	current_sel;
extern int                      scroll_height;
extern int                      scroll_width;
extern int			started_thumbnails;
int		 max_x;
int		 max_y;
int		 min_x;
int		 min_y;
int              first_open = TRUE;

void
display_error (window, error)
    Window     window;
    char      *error;
{
    xv_set (window, FRAME_LEFT_FOOTER, error, NULL);
    window_bell (window);
}

/*
 * Drop target notify proc.
 */

int
drop_target_notify_proc (item, value, event)
Panel_item               item;
unsigned int             value;
Event                   *event;
{


/*
 * Get the interesting items associated with this drop target:
 * the dnd object, and the selection requestor.
 */

    current_sel = (Selection_requestor) xv_get (item, PANEL_DROP_SEL_REQ);
    current_dnd_object = (Dnd) xv_get (item, PANEL_DROP_DND);

    switch (event_action (event)) {

       case LOC_DRAG:
 
/*
 * Clear out the left footer, just in case we'll be displaying some
 * text there.
 */
 
         xv_set (base_window->base_window, 
				FRAME_LEFT_FOOTER, DGET(""), NULL);
 
         switch(value) {
 
             case XV_OK:
                  if (prog->verbose)
                     fprintf (stderr, MGET("Drag and Drop: Began\n"));
                  xv_set (base_window->base_window, FRAME_LEFT_FOOTER,
			  EGET("Drag and Drop: Began"), NULL);
                  break;
             case DND_TIMEOUT:
                  if (prog->verbose)
                     fprintf (stderr, MGET("Drag and Drop: Timed Out\n"));
                  display_error (base_window->base_window, 
			         EGET("Drag and Drop: Timed Out"));
                  break;
             case DND_ILLEGAL_TARGET:
                  if (prog->verbose)
                     fprintf (stderr, MGET("Drag and Drop: Illegal Target\n"));
                  display_error (base_window->base_window, 
				 EGET("Drag and Drop: Illegal Target"));
                  break;
             case DND_SELECTION:
                  if (prog->verbose)
                     fprintf (stderr, MGET("Drag and Drop: Bad Selection\n"));
                  display_error (base_window->base_window, 
                                 EGET("Drag and Drop: Bad Selection"));
                  break;
             case DND_ROOT:
                  if (prog->verbose)
                     fprintf (stderr,
                          EGET("Drag and Drop: Can't drop onto Root Window\n"));
                  display_error (base_window->base_window, 
                                 EGET("Drag and Drop: Can't drop onto Root Window"));
		  break;
             case XV_ERROR:
                  if (prog->verbose)
                     fprintf (stderr, MGET("Drag and Drop: Failed\n"));
                  display_error (base_window->base_window, 
		 	         EGET("Drag and Drop: Failed"));
                  break;
             }

          break;
 
       case ACTION_DRAG_COPY:
       case ACTION_DRAG_MOVE:
 
/*
 * Imagetool should only accept ACTION_DRAG_COPY.. so change
 * the event to this just in case it was ACTION_DRAG_MOVE.
 */
 
            event->action = ACTION_DRAG_COPY;
 
/*
 * Clear out the left footer, just in case we'll be displaying some
 * text there.
 */
 
            xv_set (base_window->base_window, 
			FRAME_LEFT_FOOTER, DGET(""), NULL);

/* If the user dropped over an acceptable                     
 * drop site, the owner of the drop site will
 * be sent an ACTION_DROP_{COPY, MOVE} event.
 */
 
/* To acknowledge the drop and to associate the
 * rank of the source's selection to our
 * requestor selection object, we call
 * dnd_decode_drop().
 */
 
            if (value != XV_ERROR)
 
               load_from_dragdrop();
 
            else {
               if (prog->verbose)
                  fprintf (stderr, MGET("drop error\n"));
               }
      }
 
/*
 * Return XV_ERROR since we're not done with the dnd event.
 */
 
    return (XV_ERROR);
}
 
void
place_drop_site (source_item, frame_width)
Panel_item 	 source_item;
int        	 frame_width;
{
    int     	x_pos;
    int	    	x_offset = 0;
    Scrollbar	sbar;

    if (current_display->canvas != 0) {
       if (current_display == image_display) 
	  sbar = base_window->vscroll;
       else
	  sbar = base_window->ps_vscroll;
       x_pos = xv_get (sbar, XV_X);
       if (x_pos != 0)
	  x_offset = 4;
       }

    x_pos = frame_width - xv_get (source_item, XV_WIDTH) - x_offset;  
    xv_set(source_item,	XV_X, x_pos, NULL); 

    x_pos -= (xv_get (base_window->page_forward_button, XV_WIDTH) + 30);
    xv_set (base_window->page_forward_button, XV_X, x_pos, NULL);
    
    x_pos -= (xv_get (base_window->page_backward_button, XV_WIDTH) + 10);
    xv_set (base_window->page_backward_button, XV_X, x_pos, NULL);

    xv_set (source_item, XV_SHOW, TRUE, NULL);
    xv_set (base_window->page_forward_button, XV_SHOW, TRUE, NULL);
    xv_set (base_window->page_backward_button, XV_SHOW, TRUE, NULL);
}

Notify_value
base_window_resize (frame, event, arg, type)
    Frame               frame;
    Event              *event;
    Notify_arg          arg;
    Notify_event_type   type;
{ 
    Rect                rect;
    Xv_drop_site        frame_dropsite;
    int                 panel_height;
    int                 curr_width, curr_height;
    static int          old_width = 0, old_height = 0;

/*
 * Clear out the left footer, just in case we'll be displaying some
 * text there.
    xv_set (base_window->base_window, FRAME_LEFT_FOOTER, DGET(""), NULL);
 */

/*
 * Return if no size change.
 */
    curr_width  = xv_get (frame, XV_WIDTH);
    curr_height = xv_get (frame, XV_HEIGHT);

    if ((curr_width == old_width) && (curr_height == old_height))
      return (notify_next_event_func (frame, (Notify_event)event, arg, type));

/*                                                       
 * User resized the window. Reposition the drop site.
 */
    place_drop_site (base_window->drop_target, curr_width);

/*
 * Check only if the canvas needs to be larger.
 * "2" below comes from left and right border of 1.
 */
    panel_height = xv_get (base_window->base_panel, XV_HEIGHT);

    if ((current_display != NULL) && (current_image != NULL)) {
      if ((curr_height - panel_height - scroll_height) >= current_image->height)
        xv_set (current_display->canvas, 
		CANVAS_HEIGHT, curr_height - panel_height - scroll_height - 2,
		NULL);
      else
	xv_set (current_display->canvas, CANVAS_HEIGHT, current_image->height, NULL);
  
      if (curr_width - scroll_width >= current_image->width)
	xv_set (current_display->canvas, 
		CANVAS_WIDTH, curr_width - scroll_width - 2,
		NULL);
      else
	xv_set (current_display->canvas, CANVAS_WIDTH, current_image->width, NULL);

      XSync (image_display->xdisplay, 0);
    
      if (prog->xil_opened && current_display == image_display) {  
        if (image_display->display_win) 
	  xil_destroy (image_display->display_win);
        image_display->display_win = 
	    xil_create_from_window (image_display->state, 
				    image_display->xdisplay, 
			            image_display->win);
      }
    }
    else if (current_display == ps_display) {
      xv_set (current_display->canvas,
	      CANVAS_HEIGHT, curr_height - panel_height - scroll_height - 2,
	      CANVAS_WIDTH, curr_width - scroll_width - 2,
	      NULL);
      current_display->height = xv_get (current_display->canvas, CANVAS_HEIGHT);
      current_display->width = xv_get (current_display->canvas, CANVAS_WIDTH);
    }


    if (current_image != NULL) {
       if ((current_image->type_info->type == POSTSCRIPT) ||
	   (current_image->type_info->type == EPSF))
          check_canvas (current_image->width, current_image->height,
                        current_display->canvas, base_window->ps_hscroll,
                        base_window->ps_vscroll, NULL, NULL, current_display);
       else
          check_canvas (current_image->width, current_image->height,
                        current_display->canvas, base_window->hscroll,
                        base_window->vscroll, NULL, NULL, current_display);
    }
    old_width = curr_width;
    old_height = curr_height;

    frame_dropsite = xv_get (base_window->base_window, XV_KEY_DATA, DROPSITE);
    rect.r_left = 0;
    rect.r_top = xv_get (base_window->base_panel, XV_HEIGHT);
    rect.r_width = xv_get (base_window->base_window, XV_WIDTH);
    rect.r_height = xv_get (base_window->base_window, XV_HEIGHT);
    
    xv_set (frame_dropsite,
		DROP_SITE_DELETE_REGION,	NULL,
		DROP_SITE_REGION,		&rect,
		NULL);

    return (notify_next_event_func (base_window->base_window, (Notify_event)event, 
				    arg, type));
}  
 
static int
adjust_position (x, y)
    int *x, *y;
{
    int display_height, display_width;

    display_height = xv_get (base_window->base_window, XV_HEIGHT) -
                     xv_get (base_window->base_panel, XV_HEIGHT) -
		     scroll_height - 2;

    display_width = xv_get (base_window->base_window, XV_WIDTH) -
                    scroll_width - 2;

    if (*x > display_width)
      *x = display_width;
    else if (*x < 0)
      *x = 0;

    if (*y > display_height)
      *y = display_height;
    else if (*y < 0)
      *y = 0;
}

Notify_value
base_window_event_proc (window, event, arg, type)
Xv_opaque                window;
Event                   *event;
Notify_arg               arg;
Notify_event_type        type;
{
    Notify_value      value;
    static int        erase = FALSE;
    static int        panel_height;
    static int        x, y, prevx, prevy;
    static int        top_left_x, top_left_y;
    int               frame_x, frame_y;
    static int        x_view_start, y_view_start;
    static Scrollbar  hscroll;
    static Scrollbar  vscroll;
    XEvent            ev;

/*
 * NOTE:
 * These events are also caught in canvas_event_proc().
 * Any changes to this code below will also need to be changed
 * in canvas_event_proc() to catch cases where
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
                fprintf(stderr, MGET("Keyboard: key '%c' %d pressed at %d,%d\n"),                       event_action(event), event_action(event),
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
  else {

    switch (event_action (event)) {
    case MS_LEFT:
    case ACTION_SELECT:
      if (current_state != (StateInfo *) NULL) {
         frame_x = event_x (event);
         frame_y = event_y (event);
         panel_height = xv_get (base_window->base_panel, XV_HEIGHT);
     if (prog->sb_right) 
        x = frame_x - 1;
      else
        x = frame_x - scroll_width - 1;
	
      y = frame_y - panel_height - 1;
      adjust_position (&x, &y);

/*
 * Selecting...
 * o Erase old box if something selected.
 * o Save new top left.
 * o Reset select region to 0.
 */
         if (current_state->select == TRUE) {

	   if (event_is_down (event)) {

	     if (current_display == ps_display) {
	       hscroll = base_window->ps_hscroll;
	       vscroll = base_window->ps_vscroll;
  	     }
	     else {
	       hscroll = base_window->hscroll;
	       vscroll = base_window->vscroll;
	     }
	     x_view_start = xv_get (hscroll, SCROLLBAR_VIEW_START);
	     y_view_start = xv_get (vscroll, SCROLLBAR_VIEW_START);
   
	     if (current_state->image_selected == TRUE) {
	    
	       drawbox ((current_state->save_rect).top_x, 
		        (current_state->save_rect).top_y, 
		        (current_state->save_rect).new_x, 
		        (current_state->save_rect).new_y, 
		        &(current_state->save_rect), FALSE);
               current_state->image_selected = FALSE;
	     }
             top_left_x = x + x_view_start;
	     top_left_y = y + y_view_start;
	     erase = FALSE;
	     (current_state->sel_rect).r_left = 0;
	     (current_state->sel_rect).r_top = 0;
  	     (current_state->sel_rect).r_width = 0;
	     (current_state->sel_rect).r_height = 0;
           }
           else if (event_is_up (event)) {
             drawbox (top_left_x, top_left_y, x + x_view_start, 
		      y + y_view_start, &(current_state->sel_rect), erase);
             (current_state->save_rect).top_x = top_left_x;
	     (current_state->save_rect).top_y = top_left_y;
	     (current_state->save_rect).new_x = x + x_view_start;
  	     (current_state->save_rect).new_y = y + y_view_start;
	     (current_state->save_rect).rect  = current_state->sel_rect;
	     if ((current_state->sel_rect).r_width == 0 ||
	         (current_state->sel_rect).r_height == 0) {
	       current_state->image_selected = FALSE;
	       set_save_selection (OFF);
	     }  
	     else {
	       current_state->image_selected = TRUE;
	       set_save_selection (ON); 
             }  
	     erase = FALSE;
           }
         } /* end if select */
/*
 * Panning...
 */
         else if ((current_state->pan == TRUE) && (event_is_down (event))) {
	   prevx = x;
	   prevy = y;
         }
         else if ((current_state->pan == TRUE) && (event_is_up (event))) {
         }
      }
      break;

    case LOC_DRAG:
      if (current_state != (StateInfo *) NULL) {
         while (XEventsQueued (current_display->xdisplay, QueuedAfterFlush) &&
	       (XPeekEvent (current_display->xdisplay, &ev), (ev.xmotion.type == MotionNotify))) {
	   XNextEvent (current_display->xdisplay, &ev);
         }
         frame_x = event_x (event);
         frame_y = event_y (event);
	 if (prog->sb_right)
	   x = frame_x - 1;
	 else
	   x = frame_x - scroll_width - 1;
         y = frame_y - panel_height - 1;
         adjust_position (&x, &y);
/*
 * Panning...
 */
         if (current_state->pan == TRUE) {
           int  diffx, diffy;

	   current_state->old_x = current_state->currentx;
	   current_state->old_y = current_state->currenty;
	   current_state->currentx += x - prevx;
	   current_state->currenty += y - prevy;
   
	   diffx = prevx - x;
	   diffy = prevy - y;
	       
	   if (current_state->currentx > max_x)
	     current_state->currentx = max_x;
   	   if (current_state->currentx < min_x)
	     current_state->currentx = min_x;
	   if (current_state->currenty > max_y)
	     current_state->currenty = max_y;
	   if (current_state->currenty < min_y)
	     current_state->currenty = min_y;

	   adjust_scrollbars (diffx, diffy);
           
	   if ((current_state->old_x != current_state->currentx) ||
	       (current_state->old_y != current_state->currenty)) {
	     current_state->set_roi = TRUE;
	     update_image();
	   }
           prevx = x;
           prevy = y;
         }
/*
 * Loc Drag with Selection Tool.
 */
         else if (current_state->select == TRUE) {
	   x_view_start = xv_get (hscroll, SCROLLBAR_VIEW_START);
           y_view_start = xv_get (vscroll, SCROLLBAR_VIEW_START);
	   drawbox (top_left_x, top_left_y, x + x_view_start, y + y_view_start,
		    &(current_state->sel_rect), erase);
	   erase = TRUE;
         }
      }
      break;
    case ACTION_DRAG_COPY:
    case ACTION_DRAG_MOVE:
      
/*
 * Get the current selection object and set the Current_Op to DND_CANVAS_OP
 * so we know we're dealing with a dnd operation to the pageview canvas.
 */

      current_sel = (Selection_requestor)
	xv_get (window, XV_KEY_DATA, SELECTOR);
      
      current_dnd_object = (Dnd) xv_get (window, XV_KEY_DATA, DROPSITE);
      
/*
 * Clear out the left footer, just in case we'll be displaying some
 * text there.
 */
 
      xv_set (base_window->base_window, 
	      FRAME_LEFT_FOOTER, DGET(""), NULL);

/* If the user dropped over an acceptable                     
 * drop site, the owner of the drop site will
 * be sent an ACTION_DROP_{COPY, MOVE} event.
 */
 
/* To acknowledge the drop and to associate the
 * rank of the source's selection to our
 * requestor selection object, we call
 * dnd_decode_drop().
 */

      if (dnd_decode_drop (current_sel, event) != XV_ERROR) {

 
/* We can use the macro dnd_site_id() to access
 * the site id of the drop site that was
 * dropped on.
 * In this case, must be site 1 (frame).
 */
 
	int site_id = dnd_site_id (event);
	
	if (site_id == 1) 
	  load_from_dragdrop();
	
      }
      
      else {
	if (prog->verbose)
	  fprintf (stderr, MGET("drop error\n"));
      }
      
      break;

/*
 * Frame resize.
 */
    case WIN_RESIZE:
      value = base_window_resize (window, event, arg, type);
      break;
/*
 * This is to tell us when to display the palette.
 * We only do this on the first WIN_VIS notify event and
 * then set a flag.
 */
    case WIN_VISIBILITY_NOTIFY:
      if ((current_image != NULL) && (current_props->show_palette) &&
	  (prog->tt_timer_set == FALSE) && (first_open)) {
/*
 * Place palette is already showing.
 */
        if (palette) {
	    ds_position_popup (base_window->base_window, palette->palette, 
			       DS_POPUP_RIGHT);
	    xv_set (palette->palette, XV_SHOW, TRUE, 
		    FRAME_CMD_PUSHPIN_IN, TRUE, 
		    NULL);
	}
        else
	  palette_callback (NULL, MENU_NOTIFY);

	first_open = FALSE;
        set_zoom_and_rotate();
      }
      if ((prog->frame_mapped == FALSE) && (current_display == ps_display) &&
          (current_state != NULL) && (prog->dps == TRUE)) {
	 started_thumbnails = TRUE;
         do_dps_op (0);
	 }
      prog->tt_timer_set = FALSE;
      prog->frame_mapped = TRUE;
      break;

    default:
      break;
    }
 
  }
    value = notify_next_event_func (base_window->base_window, 
				    (Notify_event)event, arg, type);
    return (value);
}

void
set_color_depth_value (ftype)
    FileTypes    ftype;
{
    int    i, nitems;
    Menu   depth_menu;

/*
 * Set all menu items inactive.
 * Then turn on appropriate items per format type.
 */

    depth_menu = (Menu) xv_get (saveas->depth, PANEL_ITEM_MENU);
    nitems = xv_get (depth_menu, MENU_NITEMS);

    for (i = nitems; i > 0; i--) 
      xv_set (xv_get (depth_menu, MENU_NTH_ITEM, i), 
				MENU_INACTIVE, TRUE, NULL );

    if (ftype == RASTER || ftype == EPSF || ftype == POSTSCRIPT) {
       xv_set (xv_get (depth_menu, MENU_NTH_ITEM, 1), 
				MENU_INACTIVE, FALSE, NULL);  /* B/W */
       xv_set (xv_get (depth_menu, MENU_NTH_ITEM, 2),         /* 256 */
				MENU_INACTIVE, FALSE, NULL);
       xv_set (xv_get (depth_menu, MENU_NTH_ITEM, 3),         /* 24-bit */
				MENU_INACTIVE, FALSE, NULL);

       xv_set (saveas->depth_value, PANEL_LABEL_STRING, LGET ("8"), NULL);  /* 256 */
       }
    else if (ftype == TIF) {
       xv_set (xv_get (depth_menu, MENU_NTH_ITEM, 1),        /* B/W */
				MENU_INACTIVE, FALSE, NULL);
       xv_set (xv_get (depth_menu, MENU_NTH_ITEM, 2),        /* 256 */
				MENU_INACTIVE, FALSE, NULL);
       xv_set (xv_get (depth_menu, MENU_NTH_ITEM, 4), 
				MENU_INACTIVE, FALSE, NULL);  /* 32-bit */
       xv_set (saveas->depth_value, PANEL_LABEL_STRING, LGET ("8"), NULL);
       }
    else if (ftype == JFIF) {
       xv_set (xv_get (depth_menu, MENU_NTH_ITEM, 3),        /* 24-bit */
				MENU_INACTIVE, FALSE, NULL);
       xv_set (saveas->depth_value, PANEL_LABEL_STRING, LGET ("24"), NULL);
       }
    else {
       xv_set (xv_get (depth_menu, MENU_NTH_ITEM, 2),       /* 8-bit only */
				MENU_INACTIVE, FALSE, NULL);
       xv_set (saveas->depth_value, PANEL_LABEL_STRING, LGET ("8"), NULL);
       }
}

#ifdef FILECHOOSER
void
set_colors_value (ftype)
    FileTypes    ftype;
{
    int    i, nitems;
    Menu   colors_menu;

/*
 * Set all menu items inactive.
 * Then turn on appropriate items per format type.
 */

    colors_menu = (Menu) xv_get (saveasfc->colors, PANEL_ITEM_MENU);
    nitems = xv_get (colors_menu, MENU_NITEMS);

    for (i = nitems; i > 0; i--) 
      xv_set (xv_get (colors_menu, MENU_NTH_ITEM, i), 
				MENU_INACTIVE, TRUE, NULL );

    xv_set (colors_menu, MENU_DEFAULT_ITEM, 
	       xv_get (colors_menu, MENU_NTH_ITEM, 2), NULL);

    if (ftype == RASTER || ftype == EPSF || ftype == POSTSCRIPT) {
       xv_set (xv_get (colors_menu, MENU_NTH_ITEM, 1), 
				MENU_INACTIVE, FALSE, NULL);   /* B/W */
       xv_set (xv_get (colors_menu, MENU_NTH_ITEM, 2),         /* 256 */
				MENU_INACTIVE, FALSE, 
	                        NULL);
       xv_set (xv_get (colors_menu, MENU_NTH_ITEM, 3),         /* 24-bit */
				MENU_INACTIVE, FALSE, NULL);
       xv_set (saveasfc->colors_value, PANEL_LABEL_STRING, LGET ("256"), NULL);  /* 256 */
       }
    else if (ftype == TIF) {
       xv_set (xv_get (colors_menu, MENU_NTH_ITEM, 1),        /* B/W */
				MENU_INACTIVE, FALSE, NULL);
       xv_set (xv_get (colors_menu, MENU_NTH_ITEM, 2),        /* 256 */
				MENU_INACTIVE, FALSE, NULL);
       xv_set (xv_get (colors_menu, MENU_NTH_ITEM, 3), 
				MENU_INACTIVE, FALSE, NULL);  /* 32-bit */
       xv_set (saveasfc->colors_value, PANEL_LABEL_STRING, LGET ("256"), NULL);
       }
    else if (ftype == JFIF) {
       xv_set (xv_get (colors_menu, MENU_NTH_ITEM, 3),        /* 24-bit */
				MENU_INACTIVE, FALSE, NULL);
      xv_set (colors_menu, MENU_DEFAULT_ITEM, 
	       xv_get (colors_menu, MENU_NTH_ITEM, 3), NULL);
       xv_set (saveasfc->colors_value, PANEL_LABEL_STRING, LGET ("Millions"), NULL);
       }
    else {
       xv_set (xv_get (colors_menu, MENU_NTH_ITEM, 2),       /* 8-bit only */
				MENU_INACTIVE, FALSE, NULL);
       xv_set (saveasfc->colors_value, PANEL_LABEL_STRING, LGET ("256"), NULL);
       }
}
#endif

void
set_compression_value( ftype )
    FileTypes  ftype;
{
    int    i, nitems;
    Menu   compression_menu;
    Xv_opaque  compression_value;

/*
 * Set all menu items inactive.
 * Then turn on appropriate items per format type.
 */
#ifdef FILECHOOSER
    compression_menu = (Menu) xv_get(saveasfc->compression, PANEL_ITEM_MENU);
    compression_value = saveasfc->compression_value;
#else
    compression_menu = (Menu) xv_get(saveas->compression, PANEL_ITEM_MENU);
    compression_value = saveas->compression_value;
#endif
    nitems = xv_get (compression_menu, MENU_NITEMS);

    for (i = nitems; i > 0; i--) 
        xv_set (xv_get (compression_menu, MENU_NTH_ITEM, i), 
					MENU_INACTIVE, TRUE, NULL);

    if (ftype == TIF) {
       xv_set (xv_get (compression_menu, MENU_NTH_ITEM, 1), 
					MENU_INACTIVE, FALSE, NULL);
       xv_set (xv_get (compression_menu, MENU_NTH_ITEM, 2), 
					MENU_INACTIVE, FALSE, NULL);
       xv_set (xv_get (compression_menu, MENU_NTH_ITEM, 4), 
					MENU_INACTIVE, FALSE, NULL);
       xv_set (compression_value, PANEL_LABEL_STRING, LGET ("LZW"), NULL);
       xv_set (compression_menu, MENU_DEFAULT_ITEM, 
	       xv_get (compression_menu, MENU_NTH_ITEM, 4), NULL);

       }
    else if (ftype == RASTER) {
       xv_set (xv_get (compression_menu, MENU_NTH_ITEM, 1), 
					MENU_INACTIVE, FALSE, NULL);
       xv_set (xv_get (compression_menu, MENU_NTH_ITEM, 2), 
					MENU_INACTIVE, FALSE, NULL);
       xv_set (xv_get (compression_menu, MENU_NTH_ITEM, 3), 
					MENU_INACTIVE, FALSE, NULL);
       xv_set (compression_menu, MENU_DEFAULT_ITEM, 
	       xv_get (compression_menu, MENU_NTH_ITEM, 1), NULL);
       xv_set (compression_value, PANEL_LABEL_STRING, LGET ("None"), NULL);
       }
    else if (ftype == JFIF) {
       xv_set (xv_get (compression_menu, MENU_NTH_ITEM, 5), 
					MENU_INACTIVE, FALSE, NULL);
       xv_set (compression_value, PANEL_LABEL_STRING, LGET ("JPEG"), NULL);
       xv_set (compression_menu, MENU_DEFAULT_ITEM, 
	       xv_get (compression_menu, MENU_NTH_ITEM, 5), NULL);
       }
    else if (ftype == GIF) {
       xv_set (xv_get (compression_menu, MENU_NTH_ITEM, 1), 
					MENU_INACTIVE, FALSE, NULL);
       xv_set (compression_value, PANEL_LABEL_STRING, LGET ("None"), NULL);
       xv_set (compression_menu, MENU_DEFAULT_ITEM, 
	       xv_get (compression_menu, MENU_NTH_ITEM, 1), NULL);
       }
    else {
       xv_set (xv_get (compression_menu, MENU_NTH_ITEM, 1), 
					MENU_INACTIVE, FALSE, NULL);
       xv_set (xv_get (compression_menu, MENU_NTH_ITEM, 2), 
					MENU_INACTIVE, FALSE, NULL);
       xv_set (compression_menu, MENU_DEFAULT_ITEM, 
	       xv_get (compression_menu, MENU_NTH_ITEM, 1), NULL);
       xv_set (compression_value, PANEL_LABEL_STRING, LGET ("None"), NULL);
       }
}

void
check_image_format (item, string, client_data, op, event, row)
    Panel_item     item;
    char          *string;
    caddr_t        client_data;
    Panel_list_op  op;
    Event         *event;
    int            row;
{
    int         format;
    TypeInfo   *ftype;
 
    switch (op) {
    case PANEL_LIST_OP_SELECT:
      format = xv_get (item, PANEL_LIST_CLIENT_DATA, row);
      ftype = &all_types [format];
#ifdef FILECHOOSER
      set_colors_value (ftype->type);
#else
      set_color_depth_value (ftype->type);
#endif
      set_compression_value (ftype->type);
      break;
 
    case PANEL_LIST_OP_DESELECT:
    default:
      break;
    }
 
}

void
show_list_selection (list)
    Panel_item  list;
{
    Scrollbar  sb = (Scrollbar) xv_get (list, PANEL_LIST_SCROLLBAR);
    int        drows, nrows, greatest, view_start;

    view_start = current_list_selection (list);
    if (view_start == -1) 
      return;

    drows = xv_get (list, PANEL_LIST_DISPLAY_ROWS);    
    nrows = xv_get (list, PANEL_LIST_NROWS);    
    greatest = nrows - drows;
    if (view_start > greatest)
      view_start = greatest;
    xv_set (sb, SCROLLBAR_VIEW_START, view_start, NULL);
    
}
