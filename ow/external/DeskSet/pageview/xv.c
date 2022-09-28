#ifndef lint
static char *sccsid = "@(#)xv.c 3.11 96/06/27";
#endif

/*
 * Copyright (c) 1990 - Sun Microsystems Inc.
 */

/*
 * xc.c - xview top level frame for pageview.
 */

#include "pageview.h"
#include <xview/xv_xrect.h>
#include <xview/font.h>
#include <xview/file_chsr.h>
#include "emulator.h"
#include "emulator_dps.h"

#include <DPS/dpsXclient.h>  /* 2/22 */

/* Icons needed for the new drag and drop */

static unsigned short source_drag_image [] = {
#include "dupedoc_drag.icon"
};
 
static unsigned short source_drop_image [] = {
#include "dupedoc_drop.icon"
};

static unsigned short icon_bits[] = {
#include "ps.icon"
};

static unsigned short mask_bits[] = {
#include "psmask.icon"
};

#define DPS_EXTENSION   "Adobe-DPS-Extension"

Window 		win;
Server_image	source_drag_ptr_image;
Server_image	source_drop_ptr_image;

Panel_item	the_source_item;
Panel_item  	main_panel_drop_target;
Panel_item	edit_panel_item;

Frame 		baseframe;
Panel 		panel;
Canvas		canvas;

GC		low_memory_gc;

int		reverse = FALSE;

static Panel_slider_item pageslider;
static int  x1 = 0;
static int  y1 = 0;
static int  x2 = 0;
static int  y2 = 0;
static int  vieww;
static int  viewh;

Notify_value baseframe_done_proc ();

int  display_postscript_present(Display *dpy);

/*
 * local routines.
 */


void
popup_subframe(menu, menu_item)
    Menu        menu;
    Menu_item   menu_item;
{
    Frame       frame;
    extern      Frame file_chooser;

    frame = xv_get(menu_item, XV_KEY_DATA, UI1);
    if (frame == file_chooser)
      xv_set (frame, FILE_CHOOSER_UPDATE, NULL);

    xv_set(frame, XV_SHOW, TRUE, NULL);
}

void
show_prop()
{
    popup_subframe (xv_get (edit_panel_item, PANEL_ITEM_MENU),
		    xv_get (xv_get (edit_panel_item, PANEL_ITEM_MENU),
						     MENU_NTH_ITEM, 2));
}

static void
edit_callback(item, event)
    Panel_item  item;
    Event      *event;
{
    popup_subframe(item, event);
    edit_file(pathname);
}

static void
first_page()
{
    if (CurrentPage != 1) {
	setbusy();
	GotoPage(1);
	setactive();
    }
}

static void
last_page()
{
    if (CurrentPage != NumberOfPages) {
	setbusy();
	GotoPage(NumberOfPages);
	setactive();
    }
}

static void
next_page()
{
    if (CurrentPage < NumberOfPages) {
	setbusy();
	GotoPage(CurrentPage + 1);
	setactive();
    }
}

static void
prev_page()
{
    if (CurrentPage > 1) {
	setbusy();
	GotoPage(CurrentPage - 1);
	setactive();
    }
}

static void
reverse_pages ()
{
    long tmp_page;
    int  i;

    if (NumberOfPages > 1) {
	setbusy();
	if (reverse == TRUE)
	   reverse = FALSE;
	else
	   reverse = TRUE;
	GotoPage (CurrentPage);
	setactive();
    }
}

void
file_not_found(s)
    char       *s;
{
    notice_prompt(baseframe, NULL,
		  NOTICE_MESSAGE_STRINGS, EGET("File not found!"), s, NULL,
		  NOTICE_BUTTON, LGET("Ok"), 1,
		  NULL);
}

/*
 * baseframe_done_proc - Done proc for the base frame. Here, we just
 * reset the edit_textsw so that we don't get that annoying notice
 * asking us if we want to save our edits. (we don't!)
 */

Notify_value
baseframe_done_proc (client, status)
Notify_client 	client;
Destroy_status	status;
{
    if (status == DESTROY_CHECKING)
       textsw_reset (edit_textsw, 0, 0);
    else if (status == DESTROY_CLEANUP)
       return (notify_next_destroy_func (client, status));
    return (NOTIFY_DONE);
}

/*
 * Drop target notify proc.
 */

int
drop_target_notify_proc (item, value, event)
Panel_item		 item;
unsigned int		 value;
Event			*event;
{

    
/*
 * Get the interesting items associated with this drop target:
 * the dnd object, and the selection requestor.
 */

    Current_Dnd_object = (Dnd) xv_get (item, PANEL_DROP_DND);
    Current_Sel = (Selection_requestor) xv_get (item, PANEL_DROP_SEL_REQ);


    switch (event_action (event)) {

       case LOC_DRAG:

/* 
 * Clear out the left footer, just in case we'll be displaying some
 * text there.
 */

         xv_set (baseframe, FRAME_LEFT_FOOTER, " ", NULL);
 
         switch(value) {
 
             case XV_OK:
		  if (verbose)
		     fprintf (stderr, MGET("Drag and Drop: Began\n"));
                  xv_set(baseframe, FRAME_LEFT_FOOTER,  
					 MGET("Drag and Drop: Began"), 0);
                  break;
             case DND_TIMEOUT:
		  if (verbose)
		     fprintf (stderr, MGET("Drag and Drop: Timed Out\n"));
                  xv_set(baseframe, FRAME_LEFT_FOOTER, 
					 MGET("Drag and Drop: Timed Out"), 0);
                  break;
             case DND_ILLEGAL_TARGET:
		  if (verbose)
		     fprintf (stderr, MGET("Drag and Drop: Illegal Target\n"));
                  xv_set(baseframe, FRAME_LEFT_FOOTER,
                         	     MGET("Drag and Drop: Illegal Target"), 0);
                  break;
             case DND_SELECTION:
		  if (verbose)
		     fprintf (stderr, MGET("Drag and Drop: Bad Selection\n"));
                  xv_set(baseframe, FRAME_LEFT_FOOTER,
                            	      MGET("Drag and Drop: Bad Selection"), 0);
                  break;
             case DND_ROOT:
		  if (verbose)
		     fprintf (stderr, 
			  MGET("Drag and Drop: Can't drop onto Root Window\n"));
                  xv_set(baseframe, FRAME_LEFT_FOOTER,
                         MGET("Drag and Drop: Can't drop onto Root Window"), 0);
                  break;
             case XV_ERROR:
		  if (verbose)
		     fprintf (stderr, MGET("Drag and Drop: Failed\n"));
                  xv_set(baseframe, FRAME_LEFT_FOOTER,  
					 MGET("Drag and Drop: Failed"), 0);
                  break;
             }
 
          break;

       case ACTION_DRAG_COPY:
       case ACTION_DRAG_MOVE:

/*
 * Pageview should only accept ACTION_DRAG_COPY.. so change
 * the event to this just in case it was ACTION_DRAG_MOVE. 
 */

 	    event->action = ACTION_DRAG_COPY;

/*
 * Set the Current_Op to DND_OP so we know we're dealing with a 
 * dnd operation and not a tooltalk operation.
 */

    	    Current_Op = DND_OP;

/* 
 * Clear out the left footer, just in case we'll be displaying some
 * text there.
 */

            xv_set (baseframe, FRAME_LEFT_FOOTER, " ", NULL);
 
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
	       if (verbose)
		  fprintf (stderr,MGET("drop error\n"));
	       }
	
       }

/*
 * Return XV_ERROR since we're not done with the dnd event.
 */

    return (XV_ERROR);
}

/*
 * Canvas event proc. Here, we're looking to see if the user
 * is dragging the canvas around to change the view.
 */

Notify_value
canvas_interpose_proc (window, event, arg, type)
Xv_opaque 	 	 window;
Event			*event;
Notify_arg	 	 arg;
Notify_event_type	 type;
{

    switch (event_action(event)) {
       case ACTION_DRAG_COPY:
       case ACTION_DRAG_MOVE:

/*
 * Get the current selection object and set the Current_Op to DND_CANVAS_OP
 * so we know we're dealing with a dnd operation to the pageview canvas.
 */

       	    Current_Sel = (Selection_requestor) 
					xv_get (window, XV_KEY_DATA, UI12);

    	    Current_Op = DND_CANVAS_OP;

/* 
 * Clear out the left footer, just in case we'll be displaying some
 * text there.
 */

            xv_set (baseframe, FRAME_LEFT_FOOTER, " ", NULL);
 
/* If the user dropped over an acceptable
 * drop site, the owner of the drop site will
 * be sent an ACTION_DROP_{COPY, MOVE} event.
 */

/* To acknowledge the drop and to associate the
 * rank of the source's selection to our
 * requestor selection object, we call
 * dnd_decode_drop().
 */

            if (dnd_decode_drop(Current_Sel, event) != XV_ERROR) {

/* We can use the macro dnd_site_id() to access
 * the site id of the drop site that was
 * dropped on.
 * In this case, must be site 3 (canvas).
 */

	       int site_id = dnd_site_id (event);

               if ((site_id == 3)  || (site_id == 4))
                  load_from_dragdrop();

               } 
	    else {
	       if (verbose)
		  fprintf (stderr,MGET("drop error\n"));
	       }
	
	default:
            return (notify_next_event_func (window, (Notify_event) event, 
					    arg, type));
	}

}

Notify_value
baseframe_event_proc (window, event, arg, type)
Xv_opaque		 window;
Event			*event;
Notify_arg		 arg;
Notify_event_type        type;
{
    int                 canvas_height, canvas_width;
    Rect                rect;

    Panel_item          the_source_item;
    Panel_item          the_last_item;
    Xv_drop_site        canvas_dropsite;

    if (event_action(event) == WIN_RESIZE) {

/*
 * Get the interesting items associated with this panel:
 *  The Drop site item, the dnd object, the panel message item used
 *  to display the dnd image and the last item on the panel prior
 *  to the dnd item (needed in case we're resizing the window and
 *  have to reposition the dnd object).
 */

       the_source_item = (Panel_item) xv_get (panel, XV_KEY_DATA, UI6);
       the_last_item = (Panel_item) xv_get (panel, XV_KEY_DATA, UI7);

/*
 * Clear out the left footer, just in case we'll be displaying some
 * text there.
 */
 
       /*xv_set (baseframe, FRAME_LEFT_FOOTER, " ", NULL); */

/*                                                       
 * User resized the window. Reposition the drop site.
 */
 
       place_drop_site (the_source_item, the_last_item, panel);

       if (low_memory == TRUE)
          xv_set (canvas, CANVAS_WIDTH,  pixw,
                          CANVAS_HEIGHT, pixh,
                          XV_WIDTH,      pixw,
                          XV_HEIGHT,     pixh,
                          NULL);
 
       canvas_dropsite = xv_get (canvas, XV_KEY_DATA, UI11);
       rect.r_left = 0;
       rect.r_top = 0;
       rect.r_width = xv_get (canvas, XV_WIDTH);
       rect.r_height = xv_get (canvas, XV_HEIGHT);
 
       xv_set (canvas_dropsite,
                     DROP_SITE_DELETE_REGION, NULL,
                     DROP_SITE_REGION,     &rect,
                     0);
  
       }
 
    return (notify_next_event_func (window, (Notify_event) event, arg, type));
}
 
void
make_pixmap ()
{
 
    low_memory = FALSE;
    pageview_ps_close ();
    GotoPage (CurrentPage);
 
}

void
canvas_event_proc (window, event)
   Xv_Window 		 window;
   Event		*event;
{
    static int  x,
                y;
    static int  mode;

    if ((event_is_ascii(event) || event_is_key_right(event) ||
	(event_action (event) == ACTION_ERASE_CHAR_BACKWARD) ) &&
						event_is_down(event)) {
	switch (event_action(event)) {
	case ACTION_GO_DOCUMENT_START:		/* Home */
	case ACTION_GO_LINE_BACKWARD:
	    first_page();
	    break;
	case ACTION_GO_DOCUMENT_END:		/* End */
	case ACTION_GO_LINE_END:
	    last_page();
	    break;
	case '\03':		 		/* Cntrl-C */
	    quit_pageview ();
	case '\r':				/* Return */
	case '\12':				/* Line Feed */
	case ' ':				/* Space */
	case ACTION_GO_PAGE_FORWARD:		/* PageDown */
	    next_page();
	    break;
	case '\b':				/* Cntrl-H */
	case ACTION_ERASE_CHAR_BACKWARD:	/* Backspace */
	case ACTION_GO_PAGE_BACKWARD:		/* PageUp */
	    prev_page();
	    break;
	default:
	    if (verbose)
		printf(MGET("Keyboard: key '%c' %d pressed at %d,%d\n"),
		       event_action(event), event_action(event),
		       event_x(event), event_y(event));
	}
    } else {
	switch (event_action(event)) {

	case ACTION_PROPS:
	    show_prop ();
	    break;
	case LOC_DRAG:
	    x1 = x2;
	    y1 = y2;
	    switch (mode) {
	    case 1:
		x2 += event_x(event) - x;
		y2 += event_y(event) - y;
		x = event_x(event);
		y = event_y(event);
		update_page();
		break;
	    case 2:
		x = event_x(event);
		y = event_y(event);
		if (pageview.aa) {
		    x2 = x - x * (pixw/AAscale) / vieww;
		    y2 = y - y * (pixh/AAscale) / viewh;
		} else {
		    x2 = x - x * pixw / vieww;
		    y2 = y - y * pixh / viewh;
		}
		update_page();
		break;
	    default:
		break;
	    }
	    break;
	case ACTION_SELECT:
	case MS_LEFT:
	    mode = 1;
	    if (low_memory == TRUE)
	       make_pixmap ();
	    x = event_x(event);
	    y = event_y(event);
	    break;
	case ACTION_ADJUST:
	case MS_MIDDLE:
	    mode = 2;
	    if (low_memory == TRUE)
	       make_pixmap ();
	    x1 = x2;
	    y1 = y2;
	    x = event_x(event);
	    y = event_y(event);
	    if (pageview.aa) {
		x2 = x - x * (pixw/AAscale) / vieww;
		y2 = y - y * (pixh/AAscale) / viewh;
	    } else {
		x2 = x - x * pixw / vieww;
		y2 = y - y * pixh / viewh;
	    }
	    update_page();
	    break;
	case ACTION_MENU:
	case MS_RIGHT:
	    mode = 3;
	    break;

	default:
            return ;
	}
    }
}

static      XRectangle
compress_damage(display, xid, xrects)
    Display    *display;
    Window      xid;
    Xv_xrectlist *xrects;
{
    XEvent      ev;
    XRectangle *xr = xrects->rect_array;
    XRectangle  clip;

    int         i;
    int         minx = xr->x;
    int         miny = xr->y;
    int         maxx = minx + xr->width;
    int         maxy = miny + xr->height;

    /* look through this expose event building the bbox */
    for (i = 1, xr++; i < xrects->count; i++, xr++) {
	if (xr->x < minx)
	    minx = xr->x;
	if (xr->y < miny)
	    miny = xr->y;
	if ((int) (xr->x + xr->width) > maxx)
	    maxx = xr->x + xr->width;
	if ((int) (xr->y + xr->height) > maxy)
	    maxy = xr->y + xr->height;
    }

    XSync(display, 0);
    /* look through pending expose events building the bbox */
    while (XPending(display) && (XPeekEvent(display, &ev),
			     (ev.type == Expose && ev.xany.window == xid))) {
	XNextEvent(display, &ev);
	if (ev.xexpose.x < minx)
	    minx = ev.xexpose.x;
	if (ev.xexpose.y < miny)
	    miny = ev.xexpose.y;
	if ((int) (ev.xexpose.x + ev.xexpose.width) > maxx)
	    maxx = ev.xexpose.x + ev.xexpose.width;
	if ((int) (ev.xexpose.y + ev.xexpose.height) > maxy)
	    maxy = ev.xexpose.y + ev.xexpose.height;
    }

    /* confine drawing to the extent of the damage */
    clip.x = minx;
    clip.y = miny;
    clip.width = maxx - minx;
    clip.height = maxy - miny;
    return (clip);
}

static void
canvas_resize_proc (canvas, pw, dsp, xwin, xrects)
    Canvas      canvas;
    Xv_window   pw;
    Display    *dsp;
    Window      xwin;
    Xv_xrectlist *xrects;
{
    int		 	canvas_height, canvas_width;
    Rect		rect;

    Panel_item		the_source_item;
    Panel_item		the_last_item;
    Xv_drop_site	canvas_dropsite;

/*
 * The canvas was resized... So, we need to repaint the drop site item
 * in the panel so that it is still displayed, and still on the right
 * side of the panel.
 */

/*
 * Get the interesting items associated with this panel:
 *  The Drop site item, the dnd object, the panel message item used
 *  to display the dnd image and the last item on the panel prior
 *  to the dnd item (needed in case we're resizing the window and
 *  have to reposition the dnd object).
 */

    the_source_item = (Panel_item) xv_get (panel, XV_KEY_DATA, UI6);
    the_last_item = (Panel_item) xv_get (panel, XV_KEY_DATA, UI7);

/* 
 * Clear out the left footer, just in case we'll be displaying some
 * text there.
 */

    /*xv_set (baseframe, FRAME_LEFT_FOOTER, " ", NULL);*/
 
/* 
 * User resized the window. Reposition the drop site.
 */

    place_drop_site (the_source_item, the_last_item, panel);

    canvas_dropsite = xv_get (canvas, XV_KEY_DATA, UI11);
    rect.r_left = 0; 
    rect.r_top = 0; 
    rect.r_width = xv_get (canvas, XV_WIDTH);
    rect.r_height = xv_get (canvas, XV_HEIGHT);

    xv_set (canvas_dropsite, 
                  DROP_SITE_DELETE_REGION, NULL,
                  DROP_SITE_REGION, 	   &rect,
                  0);

}

void
check_ps_error ()
{

   switch (pgv_notice) {
      case PS_ERROR:
           notice_prompt (baseframe, NULL,
               NOTICE_MESSAGE_STRINGS,
                 EGET ("A PostScript error was found in this file."),
                 EGET ("For more information, check the Output Log in"),
		 EGET ("the Edit->PostScript PopUp window for the"),
		 EGET ("PostScript error message."),
                 NULL,
               NOTICE_BUTTON_NO, LGET("Ok"),
               NULL);
	   break;
      case TIMEOUT:
           notice_prompt (baseframe, NULL,
               NOTICE_MESSAGE_STRINGS,
                 EGET ("Job timed out!"),
                 EGET ("This page may have a PostScript error such as"),
                 EGET ("string or procedure that does not end."),
                 EGET (" "),
                 EGET ("You can change the job timeout value"),
                 EGET ("in the Edit->Properties PopUp window."),
                 NULL,
               NOTICE_BUTTON_NO, LGET ("Ok"),
               NULL);
	   break;
      case NO_CONVENTIONS:
	   xv_set (baseframe, FRAME_LEFT_FOOTER, 
EGET ("This file doesn't use the Adobe PostScript Structuring Comments correctly."),
		   NULL);
	   footer_set = TRUE;
      }

    pgv_notice = NONE;
}

static void
repaint_proc(canvas, pw, dsp, xwin, xrects)
    Canvas      canvas;
    Xv_window   pw;
    Display    *dsp;
    Window      xwin;
    Xv_xrectlist *xrects;
{
    XRectangle  xr;

    vieww = xv_get(canvas, XV_WIDTH);
    viewh = xv_get(canvas, XV_HEIGHT);
    xr = compress_damage(dsp, xwin, xrects);

/*
 * If we're running in antialiasing mode, then call the ps_AAfix
 * function so just the damaged part gets repainted.
 */

    if (pageview.aa) {
	PageViewConn *tmp;
	int tag;

	tmp = get_current();
	set_current(NeWSconn);
	ps_AAfix (win, pixmap, x2, y2, (pixh/AAscale), (int) xr.x, (int) xr.y, 
			(int) xr.width, (int) xr.height);
        ps_synch();
        if (ps_read_tag(&tag) <= 0) {
            warning(MGET("Unknown response received from NeWS Server.. Aborting."));
            exit(-42);
        }
	set_current(tmp);
    } else {
	
	if (low_memory == FALSE) {
	   XSetClipRectangles(dsp, gc, 0, 0, &xr, 1, Unsorted);
	   XFillRectangle(dsp, win, gc, 0, 0, vieww, viewh);
	   update_page();
	   XSetClipMask(dsp, gc, (Pixmap) None);
	   }
	else {

	   if (dps == FALSE) {
	      if ( PostScript == (PSFILE *) NULL ) {
                 home_page ();
                 return ;
                 }
 
              ps_fix (win, (int) xr.x, (int) xr.y, (int) xr.width, 
		      (int) xr.height);
              ps_flush_PostScript ();
	      }
	   else {
	      if ( dps_context == (DPSContext) NULL ) {
                 home_page ();
                 return ;
                 }
              dps_fix ((int) xr.x, (int) xr.y, (int) xr.width, (int) xr.height);
	      }
	   update_page (); 
	   if (pgv_notice != NONE) 
	      check_ps_error ();	
	}
    }
}


/*
 * public routines.
 */

void
setbusy()
{
    Frame       frame;
    int         n = 1;

    xv_set(baseframe, FRAME_BUSY, TRUE, NULL);
    while (frame = xv_get(baseframe, FRAME_NTH_SUBFRAME, n++))
	xv_set(frame, FRAME_BUSY, TRUE, NULL);
}

void
setactive()
{
    Frame       frame;
    int         n = 1;

    if (pgv_notice != NONE) 
       check_ps_error ();
    dump_output ();

    while (frame = xv_get(baseframe, FRAME_NTH_SUBFRAME, n++))
	xv_set(frame, FRAME_BUSY, FALSE, NULL);
    xv_set(baseframe, FRAME_BUSY, FALSE, NULL);

}

void
set_frame_label (file)
    char       *file;
{
    static char label[256]; 
    char   *f_label;

    f_label = LGET ("File");
    
    sprintf(label, "%s     %s: %s", PV_Name, f_label, file);
    xv_set(baseframe,
	   FRAME_LABEL, label,
	   NULL);

/* 
 * Clear out the footer too if we haven't set it somewhere else..
 */

    if (footer_set != TRUE)
       xv_set (baseframe, FRAME_LEFT_FOOTER, "", NULL);
}

void
set_right_footer(page, npages)
    int         page;
    int         npages;
{
    static char right[80];
    sprintf(right, LGET("Page: %d of %d"), page, npages);
    xv_set(baseframe,
	   FRAME_RIGHT_FOOTER, right,
	   NULL);
}

void
set_icon_label(s)
    char       *s;
{
    Icon        icon;
    struct rect text_rect,
               *icon_rect;
    Font	font;

    s = basename(s);
    strcpy (iconname, s);

    set_frame_label (s);

    if (strcmp (s, NONAME) != 0) {
       xv_set (main_panel_drop_target, PANEL_DROP_FULL, TRUE, NULL);
       xv_set (textsw_drop_target, PANEL_DROP_FULL, TRUE, NULL);
       }
 
    icon = xv_get(baseframe, FRAME_ICON);
    icon_rect = (Rect *) xv_get (icon, ICON_IMAGE_RECT);
    font = (Font) xv_get(icon, ICON_FONT);

    /* adjust icon text top/height to match font height */
    text_rect.r_height = xv_get(font, FONT_DEFAULT_CHAR_HEIGHT);
    text_rect.r_top =
	icon_rect->r_height - (text_rect.r_height + 2);

    /* center the icon text */
    text_rect.r_width = strlen(s) * (xv_get(font, FONT_DEFAULT_CHAR_WIDTH));
    if (text_rect.r_width > icon_rect->r_width)
	text_rect.r_width = icon_rect->r_width;
    text_rect.r_left = (icon_rect->r_width - text_rect.r_width) / 2;

    if (icon_label != (char *) NULL)
       (void) xv_set(icon,
		  XV_LABEL, icon_label,
		  ICON_LABEL_RECT, &text_rect,
		  NULL);
    else
       (void) xv_set(icon,
		  XV_LABEL, s,
		  ICON_LABEL_RECT, &text_rect,
		  NULL);
    /* xv_set actually makes a copy of all the icon fields */
    (void) xv_set(baseframe, FRAME_ICON, icon, NULL);
}

void
update_page_slider()
{
    if (CurrentPage != xv_get(pageslider, PANEL_VALUE) ||
	    NumberOfPages != xv_get(pageslider, PANEL_MAX_VALUE)) {

	if (NumberOfPages < 2)
	    xv_set(pageslider, PANEL_INACTIVE, True,
		   PANEL_VALUE, 1,
		   PANEL_MAX_VALUE, 2,
		   PANEL_TICKS, 2,
		   NULL);
	else
	    xv_set(pageslider, PANEL_INACTIVE, False,
		   PANEL_VALUE, CurrentPage,
		   PANEL_MAX_VALUE, NumberOfPages,
		   PANEL_TICKS, NumberOfPages,
		   NULL);
    }
}


void
goto_page(item, event)
    Panel_item  item;
    Event      *event;
{
    int         pageno = xv_get(pageslider, PANEL_VALUE);
    if (pageno != CurrentPage) {
	setbusy();
	GotoPage(pageno);
	setactive();
    }
}

Frame
init_gotopage(parent)
    Frame       parent;
{
    Frame       frame;
    Panel       panel;
    char        fr_label [100];
    char       *gt_label;

    gt_label = LGET ("Goto Page");

    sprintf (fr_label, "%s: %s", PV_Name, gt_label);

    frame = (Frame) xv_create(parent, FRAME_CMD,
			      FRAME_LABEL, fr_label,
			      NULL);
    panel = (Panel) xv_get(frame, FRAME_CMD_PANEL);
    pageslider = xv_create(panel, PANEL_SLIDER,
			   PANEL_LABEL_STRING, LGET("Goto Page:"),
			   PANEL_MIN_VALUE, 1,
			   PANEL_MAX_VALUE, 1,
			   PANEL_SLIDER_END_BOXES, True,
			   PANEL_NOTIFY_PROC, goto_page,
			   XV_HELP_DATA, "pageview:pageslider",
			   NULL);
    window_fit(panel);
    xv_set(frame,
	   XV_WIDTH, xv_get(panel, XV_WIDTH),
	   XV_HEIGHT, xv_get(panel, XV_HEIGHT),
	   NULL);
    return (frame);
}

/*
 * Position the drop site icon on the right side of the panel.
 * Position changes depending on the size of the panel.
 * source_item is the drop site item.
 * rightmost_item is the rightmost panel item on the panel (NOT
 *  the drop site item - the one to left of this).
 * the_drop_site is the actual drop site object created (the region
 *  around the source_item.
 * window is the window on which the drop site is going to be placed.
 */

void
place_drop_site (source_item, rightmost_item, window)
Panel_item source_item;
Panel_item rightmost_item;
Xv_opaque window;

{
  	Rect	*tmp_rect;
	Rect	*find_item_rect;
	Rect	*source_item_rect;
	int	find_left, find_width, source_left, source_width;
	int	gap = xv_get(window, PANEL_ITEM_X_GAP);
	int	panel_width = xv_get(window, XV_WIDTH);

  	tmp_rect = (Rect *) xv_get (window, XV_RECT);

	find_item_rect = (Rect *) xv_get(rightmost_item, PANEL_ITEM_RECT);
	find_left = find_item_rect->r_left;
	find_width = find_item_rect->r_width;

	source_item_rect = (Rect *) xv_get(source_item, PANEL_ITEM_RECT);
	source_left = source_item_rect->r_left;
	source_width = source_item_rect->r_width;

	if ((panel_width  - source_width - gap) >  (find_width + find_left + gap))
	{

		/* there is enough space.  Lets place the item 
		   over against the left edge of the panel. */

		xv_set(source_item, 
			PANEL_ITEM_X, panel_width - source_width - gap, 
			0);

	}
	else
	{
		/* window too small top move item over.  
		   Butt it against the last item on the panel. */

		xv_set(source_item, 
			PANEL_ITEM_X, find_width + find_left + gap, 
			0);
	}

}

void
copy_default_cmap (xdpy, cmap, ncolors)
    Display    *xdpy;
    Colormap    cmap;
    int         ncolors;
{
    int     i;
    XColor  colors[256];

    for (i = 0; i < ncolors; i++)
      colors[i].pixel = i;

    XQueryColors (xdpy, DefaultColormap (xdpy, DefaultScreen (xdpy)), colors,
                  ncolors);

    XStoreColors (xdpy, cmap, colors, ncolors);
}

void
make_std_colormaps (obj, vis_class)
Xv_opaque	obj;
int		vis_class;
{
    int		 status;
    Colormap	 canvas_cmap;
    int		 depth = xv_get (obj, XV_DEPTH);
    int		 win = xv_get (canvas_paint_window (obj), XV_XID);
    Display     *dpy = (Display *) xv_get (obj, XV_DISPLAY);
    int		 screen = DefaultScreen (dpy);
    XVisualInfo	 vinfo;

    canvas_cmap = (Colormap) xv_get (xv_get (obj, WIN_CMS), CMS_CMAP_ID);

    status = XMatchVisualInfo (dpy, screen, depth, vis_class, &vinfo);

/*
 * Only values of vis_class that are passed to this function are:
 * StaticGray (could be depth 1 or 8), StaticColor, TrueColor (24 bit only),
 * GrayScale or PseudoColor.
 */

    if (vis_class == StaticGray) {
       gray_cmap = (XStandardColormap *) calloc (1, sizeof (XStandardColormap));
       gray_cmap->colormap = canvas_cmap;
       status = XDPSCreateStandardColormaps (dpy, win, vinfo.visual, 
					     0, 0, 0, 0, rgb_cmap, gray_cmap,
					     False);
       }

/*
 * TrueColor, and StaticColor handled the same.
 */

    else if ((vis_class == TrueColor) || (vis_class == StaticColor)) {
       rgb_cmap = (XStandardColormap *) calloc (1, sizeof (XStandardColormap));
       gray_cmap = (XStandardColormap *) calloc (1, sizeof (XStandardColormap));
       rgb_cmap->colormap = canvas_cmap;
       gray_cmap->colormap = canvas_cmap;
       status = XDPSCreateStandardColormaps (dpy, win, vinfo.visual, 
					     0, 0, 0, 0, rgb_cmap, gray_cmap,
					     False);
       }

/*
 * PseudoColor and GrayScale need new colormaps created. Also, copy
 * values from the default colormap so that flashing is minimized.
 */

    else {		/* vis is PseudoColor or GrayScale */
       unsigned long plane_masks [1];
       unsigned long pixels [40];

/*
 * Check if the canvas_cmap is the Default... if so, then create a new
 * one. If not, then use it.
 */

       if (canvas_cmap == DefaultColormap (dpy, screen))
          canvas_cmap = XCreateColormap (dpy, win, vinfo.visual, AllocNone);

       if (depth == 4) {
          status = XAllocColorCells (dpy, canvas_cmap, True, plane_masks,
                                     0, pixels, 6);
          copy_default_cmap (dpy, canvas_cmap, 6);
          }
       else if (DefaultDepth (dpy, screen) == 4) {
          status = XAllocColorCells (dpy, canvas_cmap, True, plane_masks,
                                     0, pixels, 16);
          copy_default_cmap (dpy, canvas_cmap, 16);
          }
       else {
          status = XAllocColorCells (dpy, canvas_cmap, True, plane_masks,
                                     0, pixels, 40);
          copy_default_cmap (dpy, canvas_cmap, 40);
          }

       gray_cmap = (XStandardColormap *) calloc (1, sizeof (XStandardColormap));
       gray_cmap->colormap = canvas_cmap;

       if (vis_class == PseudoColor) {
          rgb_cmap = (XStandardColormap *) 
				calloc (1, sizeof (XStandardColormap));
          rgb_cmap->colormap = canvas_cmap;
   	  }

       status = XDPSCreateStandardColormaps (dpy, win, vinfo.visual, 
					     0, 0, 0, 0, rgb_cmap, gray_cmap,
					     False);
       XSetWindowColormap (dpy, win, canvas_cmap);
       }
}



int display_postscript_present(dpy)
Display *dpy;
{
 DPSContext ctxt;
    XStandardColormap gray_ramp = {0};
    int present = FALSE;
    
    /* make sure it doesn't choke on invalid grayramp */
    gray_ramp.red_max = 1;
    gray_ramp.red_mult = 1;

    ctxt = XDPSCreateContext(dpy, None, None, 0, 0, 0,
			     &gray_ramp, NULL, 0,
			     DPSDefaultTextBackstop,
			     DPSDefaultErrorProc,
			     NULL);

    if ( ctxt != (DPSContext) NULL ) {
	present = TRUE;
	DPSDestroySpace(DPSSpaceFromContext(ctxt));
    }

    return present;
}


Frame
init_frame()
{
    Server_image 	icon_image;
    Server_image 	mask_image;
    Icon        	icon;
    Menu        	menu;
    Xv_drop_site 	Canvas_Drop_site;
    Selection_requestor Canvas_Sel;
    Dnd			Panel_Dnd_object;
    Rect		rect;
    int			vis;
    XVisualInfo	       *vis_list;
    XVisualInfo		vis_template;
    int			vis_class;
    int			num_vis;
    int			i;
    XGCValues		gc_vals;
    XGCValues		low_memory_gc_vals;
    unsigned long	gc_mask;
    int			depth_1 = FALSE;
    int			depth_8 = FALSE;
    int			depth_24 = FALSE;
    Colormap		cmap;
    XColor		ccolor;
    int			color_count;
    int		  	color_set = FALSE;
    Cms			old_cms, new_cms;
    Xv_singlecolor     *old_colors;
    Xv_singlecolor	black_color = {0, 0, 0};
    int			major_opcode, first_event, first_error;

    icon_image = (Server_image) xv_create(NULL, SERVER_IMAGE,
					  XV_WIDTH, 64,
					  XV_HEIGHT, 64,
					  SERVER_IMAGE_DEPTH, 1,
					  SERVER_IMAGE_BITS, icon_bits,
					  NULL);

    mask_image = (Server_image) xv_create(NULL, SERVER_IMAGE,
					  XV_WIDTH, 64,
					  XV_HEIGHT, 64,
					  SERVER_IMAGE_DEPTH, 1,
					  SERVER_IMAGE_BITS, mask_bits,
					  NULL);

    source_drag_ptr_image = (Server_image) xv_create (NULL, SERVER_IMAGE,
				 XV_WIDTH, 64,
				 XV_HEIGHT, 64,
				 SERVER_IMAGE_BITS, source_drag_image,
				 SERVER_IMAGE_DEPTH, 1,
				 NULL);

    source_drop_ptr_image = (Server_image) xv_create (NULL, SERVER_IMAGE,
				 XV_WIDTH, 64,
				 XV_HEIGHT, 64,
				 SERVER_IMAGE_BITS, source_drop_image,
				 SERVER_IMAGE_DEPTH, 1,
				 NULL);

    icon = (Icon) xv_create(NULL, ICON,
			    ICON_IMAGE, icon_image,
			    ICON_MASK_IMAGE, mask_image,
			    ICON_TRANSPARENT, TRUE,
			    WIN_RETAINED, TRUE,
			    NULL);

    dragdrop_init ();

    baseframe = (Frame) xv_create(NULL, FRAME,
				  FRAME_LABEL, LGET("PageView"),
				  FRAME_SHOW_FOOTER, TRUE,
				  FRAME_LEFT_FOOTER, "",
				  FRAME_RIGHT_FOOTER, LGET("Page: 1 of 1"),
#ifdef OW_I18N
				  WIN_USE_IM, FALSE,
#endif
				  FRAME_ICON, icon,
				  XV_WIDTH, pageview.orient > 1 ? 722 : 935,
				  XV_HEIGHT, pageview.orient > 1 ? 850 : 722,
				  NULL);

    if (low_memory == TRUE) {
       old_cms = (Cms) xv_get (baseframe, WIN_CMS);
       color_count = (int) xv_get (old_cms, CMS_SIZE);
       old_colors = (Xv_singlecolor *) malloc ( sizeof (Xv_singlecolor) *
						(color_count + 1));
       old_colors = (Xv_singlecolor *) xv_get (old_cms, CMS_COLORS, old_colors);

       for (i = 0; (i < color_count) && (color_set == FALSE); i++) {
	   if ((old_colors [i].red == black_color.red) &&
	       (old_colors [i].green == black_color.green) &&
	       (old_colors [i].blue == black_color.blue)) {
	      xv_set (baseframe, WIN_BACKGROUND_COLOR, i, NULL);
	      color_set = TRUE;
	      }
	   }

       if (color_set == FALSE) {
	  old_colors [color_count] = black_color;

	  new_cms = (Cms) xv_create (NULL, CMS,
					CMS_SIZE, color_count + 1,
					CMS_COLORS, old_colors,
					NULL);
	  xv_set (baseframe, WIN_CMS, new_cms, NULL);
	  xv_set (baseframe, WIN_BACKGROUND_COLOR, color_count + 1, NULL);
	  }

       free (old_colors);
       }

    panel = (Panel) xv_create(baseframe, PANEL, NULL);

    (void) xv_create(panel, PANEL_BUTTON,
		     PANEL_LABEL_STRING, LGET("File"),
		     PANEL_ITEM_MENU,
		     xv_create(NULL, MENU_COMMAND_MENU,
			       MENU_ITEM, MENU_STRING, LGET("Load..."),
			       MENU_NOTIFY_PROC, popup_subframe,
			       XV_KEY_DATA, UI1, init_file(baseframe),
			       NULL,
			       MENU_ITEM, MENU_STRING, LGET("Print..."),
			       MENU_NOTIFY_PROC, popup_subframe,
			       XV_KEY_DATA, UI1, init_print(baseframe),
			       NULL,
			       NULL),
		     XV_HELP_DATA, "pageview:File",
		     NULL);

    (void) xv_create(panel, PANEL_BUTTON,
		     PANEL_LABEL_STRING, LGET("View"),
		     PANEL_ITEM_MENU,
		     xv_create(NULL, MENU_COMMAND_MENU,
			       MENU_GEN_PIN_WINDOW, baseframe, LGET("View"),
			       MENU_ITEM,
			       MENU_STRING, LGET("First Page"),
			       MENU_NOTIFY_PROC, first_page,
			       NULL,
			       MENU_ITEM,
			       MENU_STRING, LGET("Last Page"),
			       MENU_NOTIFY_PROC, last_page,
			       NULL,
			       MENU_ITEM,
			       MENU_STRING, LGET("Next Page"),
			       MENU_NOTIFY_PROC, next_page,
			       NULL,
			       MENU_ITEM,
			       MENU_STRING, LGET("Previous Page"),
			       MENU_NOTIFY_PROC, prev_page,
			       NULL,
			       MENU_ITEM,
			       MENU_STRING, LGET("Goto Page..."),
			       MENU_NOTIFY_PROC, popup_subframe,
			       XV_KEY_DATA, UI1, init_gotopage(baseframe),
			       NULL,
			       MENU_ITEM,
			       MENU_STRING, LGET("Reverse Page Order"),
			       MENU_NOTIFY_PROC, reverse_pages,
			       NULL,
			       MENU_DEFAULT, 4,	/* Next */
			       NULL),
		     XV_HELP_DATA, "pageview:View",
		     NULL);


    edit_panel_item = xv_create(panel, PANEL_BUTTON,
		     PANEL_LABEL_STRING, LGET("Edit"),
		     PANEL_ITEM_MENU,
		     xv_create(NULL, MENU_COMMAND_MENU,
			       MENU_ITEM, MENU_STRING, LGET("PostScript..."),
			       MENU_NOTIFY_PROC, popup_subframe,
			       XV_KEY_DATA, UI1, init_edit(baseframe),
			       NULL,
			       MENU_ITEM, MENU_STRING, LGET("Properties..."),
			       MENU_NOTIFY_PROC, popup_subframe,
			       XV_KEY_DATA, UI1, init_props(baseframe),
			       NULL,
			       NULL),
		     XV_HELP_DATA, "pageview:Edit",
		     NULL);


/*
 * Create objects requried for drag and drop (for the base frame panel).
 */

    Panel_Dnd_object = xv_create(panel, DRAGDROP,
                             SEL_CONVERT_PROC, PageviewSelectionConvert,
                             DND_TYPE, DND_COPY,
                             DND_CURSOR, xv_create(NULL, CURSOR,
                                            CURSOR_IMAGE, source_drag_ptr_image,
                                            CURSOR_XHOT, 17,
                                            CURSOR_YHOT, 24,
					    CURSOR_OP, PIX_SRC^PIX_DST,
                                            0),
                             DND_ACCEPT_CURSOR, xv_create(NULL, CURSOR,
                                            CURSOR_IMAGE, source_drop_ptr_image,
                                            CURSOR_XHOT, 17,
                                            CURSOR_YHOT, 24,
					    CURSOR_OP, PIX_SRC^PIX_DST,
                                            0),
                             0);

    main_panel_drop_target = xv_create (panel, PANEL_DROP_TARGET,
				   PANEL_DROP_DND,     	    Panel_Dnd_object,
				   PANEL_DROP_DND_TYPE,     
						PANEL_DROP_COPY_ONLY,
				   PANEL_NOTIFY_PROC,
						drop_target_notify_proc,
				   XV_HELP_DATA,	
						"pageview:SourceDrag",
				   NULL);

    xv_set (panel, XV_KEY_DATA, UI6, main_panel_drop_target, NULL);
    xv_set (panel, XV_KEY_DATA, UI7, edit_panel_item, NULL);
    xv_set (panel, XV_KEY_DATA, UI9, Panel_Dnd_object, NULL);

    window_fit_height(panel);

    place_drop_site (main_panel_drop_target, edit_panel_item, panel);

    if (pageview.orient < 2)
       xv_set (baseframe, XV_HEIGHT, xv_get (panel, XV_HEIGHT) + 722, NULL);

/*
 * Let's find out when the user is going to quit, and NOT put up the
 * window if he's edited any text (via the textsw).
 */

    notify_interpose_destroy_func (baseframe, baseframe_done_proc);
    notify_interpose_event_func ((Notify_client) baseframe, 
				 (Notify_func) baseframe_event_proc, 
				 NOTIFY_SAFE);

    dsp = (Display *) xv_get(baseframe, XV_DISPLAY);
    screen = DefaultScreen(dsp);

/*
 * Check if DPS extension is found..
 */
 
    dps = display_postscript_present(dsp); 

    depth = xv_get (baseframe, WIN_DEPTH);
    vis = xv_get (baseframe, XV_VISUAL_CLASS);
   
/*
 * Check the depth... Then, check the default visual, and set our
 * canvas' visual based on that info.
 * For now, depth = 4 is a bit of a problem. Can't define a pixmap
 * or canvas of that depth, so check to see if we can create one of
 * depth 8 (so color will look OK), and if not, try depth 24, and if
 * can't get that, then give them a canvas of depth 1 and they'll have 
 * to live with it.
 */

    if (dps == FALSE) {
       switch (depth) {
          case 1:  vis_class = StaticGray;
		   break;
          case 4:  vis_template.screen = screen;
		   vis_list = XGetVisualInfo (dsp, VisualScreenMask,
					      &vis_template, &num_vis);

/*
 * If we didn't get any visuals, then exit, since we can't do
 * anything anyway (we should *never* get zero visuals!).
 */

		   if ((vis_list == NULL) || (num_vis == 0)) 
		      if (verbose) {
		         fprintf (stderr, MGET("%s: No Visuals?\n"), PV_Name);
		         exit (2);
		         }

/* 
 * Check if we can find a visual of depth 8, depth 24 or of depth 1..
 */

	 	   for (i = 0; i < num_vis; i++)
		       if (vis_list[i].depth == 8)
		          depth_8 = TRUE;
		       else if (vis_list[i].depth == 1)
		          depth_1 = TRUE;
		       else if (vis_list[i].depth == 24)
		          depth_24 = TRUE;
   
		   if (depth_8 == TRUE) {
		      depth = 8;
		      vis_class = StaticColor;
		      if ((vis == GrayScale) || (vis == StaticGray))
		         vis_class = StaticGray;
		      }
  		   else if (depth_24 == TRUE) {
		      depth = 24;
		      vis_class = TrueColor;
		      }
  		   else if (depth_1 == TRUE) {
		      depth = 1;
		      vis_class = StaticGray;
	 	      }
		   else /* Only 4 bit visuals... big trouble! */
		      if (verbose) {
		         fprintf (stderr, MGET("%s: Only 4 bit Visuals.\n"), 
								   PV_Name);
		         exit (2);
		         }
		   break;
   	
          case 8:  vis_class = StaticColor;
		   if ((vis == GrayScale) || (vis == StaticGray))
		      vis_class = StaticGray;
		   break;
          case 24: vis_class = TrueColor;
		   break;
          default: if (verbose)
		      fprintf (stderr, MGET("%s: Unknown Depth: %d\n"),
					    PV_Name, depth);
		   vis_class = StaticColor;
          }
       }
    else {   /* dps server */

       vis_class = vis;

/* 
 * If we're running on a dps server, the only visuals we can't handle
 * are DirectColor and TrueColor (for depth 4, 8, 16?).
 */

       if (depth < 24) {
          if ((vis_class == DirectColor) || (vis_class == TrueColor)) 
	     vis_class = PseudoColor;
   	  }
       else if (vis_class == DirectColor)
	  vis_class = TrueColor;
       }

    canvas = xv_create(baseframe, CANVAS,
		       XV_X, 0,
		       WIN_BELOW, panel,
		       XV_VISUAL_CLASS, vis_class, 
		       XV_DEPTH, depth,
		       CANVAS_RETAINED, FALSE,
		       CANVAS_REPAINT_PROC, repaint_proc,
		       CANVAS_RESIZE_PROC, canvas_resize_proc,
		       CANVAS_FIXED_IMAGE, FALSE,
		       OPENWIN_AUTO_CLEAR, FALSE,
		       CANVAS_X_PAINT_WINDOW, TRUE,
		       XV_HELP_DATA, "pageview:MainWindowInfo",
		       CANVAS_PAINTWINDOW_ATTRS,
			   WIN_TRANSPARENT,
			   WIN_EVENT_PROC, canvas_event_proc,
			   WIN_CONSUME_EVENTS,
				LOC_DRAG, WIN_ASCII_EVENTS, WIN_MOUSE_BUTTONS,
				WIN_LEFT_KEYS, WIN_RIGHT_KEYS,
				NULL,
			   NULL,
		       NULL);

    if (low_memory == TRUE)
       xv_set (canvas, CANVAS_AUTO_EXPAND, FALSE,
		       CANVAS_AUTO_SHRINK, FALSE,
		       NULL);

    win = (Window) xv_get(canvas_paint_window(canvas), XV_XID);

    Canvas_Drop_site = xv_create (canvas, DROP_SITE_ITEM, 
			    DROP_SITE_DEFAULT,		TRUE,
			    DROP_SITE_ID,		3,
			    DROP_SITE_EVENT_MASK, DND_MOTION | DND_ENTERLEAVE,
                	    0);

    rect.r_left = 0;
    rect.r_top = 0;
    rect.r_width = xv_get (canvas, XV_WIDTH);
    rect.r_height = xv_get (canvas, XV_HEIGHT);

    xv_set (Canvas_Drop_site,
		  DROP_SITE_DELETE_REGION, NULL,
		  DROP_SITE_REGION,        &rect,
		  0);

    Canvas_Sel = xv_create (canvas, SELECTION_REQUESTOR, 0);

    xv_set (canvas, XV_KEY_DATA, UI11, Canvas_Drop_site, NULL);
    xv_set (canvas, XV_KEY_DATA, UI12, Canvas_Sel, NULL);

    xv_set (canvas, WIN_BACK, NULL);

    notify_interpose_event_func ((Notify_client) canvas, 
			  	 (Notify_func) canvas_interpose_proc, 
				 NOTIFY_SAFE);

/*
 * If we're running under dps, create the colormap.
 */

    if (dps == TRUE) {
       Window **wins;
       Window *new_wins;
       int count = 0;
       int i;
       make_std_colormaps (canvas, vis_class);

/*
 * Install colormap property on the canvas paint window.
 */

       wins = (Window **) malloc (sizeof (Window *));
       XGetWMColormapWindows (dsp, xv_get (baseframe, XV_XID), wins, &count);

       new_wins = (Window *) malloc (sizeof (Window) * (count + 2));
       for (i = 0; i < count; i++)
           new_wins [i] = wins [0][i];

       new_wins [count++] = win;
       new_wins [count++] = xv_get (baseframe, XV_XID);
       XSetWMColormapWindows (dsp, xv_get (baseframe, XV_XID), new_wins, count);
       }


/*
 * Must now create a GC rather than use DefaultGC...
 * First get the colormap...
 */

    cmap = xv_get (xv_get (canvas, WIN_CMS), CMS_CMAP_ID);

/*
 * Next, get the pixel index for White
 */

    ccolor.red = ccolor.blue = ccolor.green = 65535;
    XAllocColor (dsp, cmap, &ccolor);
    gc_vals.background = ccolor.pixel;
    low_memory_gc_vals.foreground = ccolor.pixel;

/*
 * Next, get the pixel index for Black
 */

    ccolor.red = ccolor.blue = ccolor.green = 0;
    XAllocColor (dsp, cmap, &ccolor);

    gc_vals.foreground = ccolor.pixel;
    low_memory_gc_vals.background = ccolor.pixel;

    gc_mask = GCForeground | GCBackground;
    gc = XCreateGC (dsp, win, gc_mask, &gc_vals);

    low_memory_gc = XCreateGC (dsp, win, gc_mask, &low_memory_gc_vals);

    XFlush (dsp);
    return (baseframe);
}

/*
 * AntiAliased canvas colormap rtn
 * Create a PseudoColor CANVAS in the same position as the
 * StaticColor CANVAS.  Attach an Xlib Colormap with a simple
 * linear ramp in indexes  0,8,16,...128.  WIN_BACK the StaticColor
 * CANVAS and WIN_FRONT the PseudoColor CANVAS.  To turn off AntiAliasing
 * just WIN_BACK the PseudoColor CANVAS and WIN_FRONT the StaticColor CANVAS.
 */
void
recreate_AAcanvas(flag)
int	flag;
{
    XColor	pixelx[17];
    int		i, j;
    Rect		rect;
    Xv_drop_site 	Canvas_Drop_site;
    Selection_requestor Canvas_Sel;
static Colormap colormap = NULL;
static int	lastFlag = OFF;
static Canvas	offCanvas = NULL;
static int	pixels[17] = { 0x0000,	/* linear ramp gamma(2.5) */
	0x5400, 0x6f00, 0x8300, 0x9300, 0xa000, 0xac00, 0xb700, 0xc200,
	0xcb00, 0xd400, 0xdc00, 0xe400, 0xeb00, 0xf200, 0xf900, 0xff00 };

    if (flag && (lastFlag == OFF)) {		/* turn AA canvas on */
	if (!offCanvas) {
	    offCanvas = canvas;			/* create AA canvas */
	    canvas = xv_create(baseframe, CANVAS,
		       XV_X, xv_get(offCanvas, XV_X),
		       XV_Y, xv_get(offCanvas, XV_Y),
		       CANVAS_WIDTH, xv_get(offCanvas, CANVAS_WIDTH),
		       CANVAS_HEIGHT, xv_get(offCanvas, CANVAS_HEIGHT),
		       XV_VISUAL_CLASS, PseudoColor,
		       XV_DEPTH, depth,
		       CANVAS_RETAINED, FALSE,
		       CANVAS_REPAINT_PROC, repaint_proc,
		       CANVAS_RESIZE_PROC, canvas_resize_proc,
		       CANVAS_FIXED_IMAGE, FALSE,
		       OPENWIN_AUTO_CLEAR, FALSE,
		       CANVAS_X_PAINT_WINDOW, TRUE,
		       XV_HELP_DATA, "pageview:MainWindowInfo",
		       CANVAS_PAINTWINDOW_ATTRS,
			   WIN_TRANSPARENT,
			   WIN_EVENT_PROC, canvas_event_proc,
			   WIN_CONSUME_EVENTS,
				LOC_DRAG, WIN_ASCII_EVENTS, WIN_MOUSE_BUTTONS,
				WIN_LEFT_KEYS, WIN_RIGHT_KEYS,
				NULL,
			   NULL,
		       NULL);

	    Canvas_Drop_site = xv_create (canvas, DROP_SITE_ITEM, 
			    DROP_SITE_ID,		4,
			    DROP_SITE_EVENT_MASK, DND_MOTION | DND_ENTERLEAVE,
			    0);

	    Canvas_Sel = xv_create (canvas, SELECTION_REQUESTOR, 0);
 
	    xv_set (canvas, XV_KEY_DATA, UI11, Canvas_Drop_site, NULL);
	    xv_set (canvas, XV_KEY_DATA, UI12, Canvas_Sel, NULL);
 
	    notify_interpose_event_func ((Notify_client) canvas,
				 (Notify_func) canvas_interpose_proc,
				 NOTIFY_SAFE);

            rect.r_left = 0; 
    	    rect.r_top = 0; 
    	    rect.r_width = xv_get (canvas, CANVAS_WIDTH);
    	    rect.r_height = xv_get (canvas, CANVAS_HEIGHT);
 
     	    xv_set (Canvas_Drop_site,
                  	DROP_SITE_DELETE_REGION, NULL,
                  	DROP_SITE_REGION, 	   &rect,
                  	0);

	    if (!colormap) {                    /* create Xlib colormap */
		Window basewin;
		XWindowAttributes attrs;
		Atom cmap_atom;
		unsigned long pixel_returns[129];
 
		basewin = (Window) xv_get(baseframe, XV_XID);
		win = (Window) xv_get(canvas_paint_window(canvas), XV_XID);
		XGetWindowAttributes (dsp, win, &attrs);
		colormap = XCreateColormap (dsp, win, attrs.visual, AllocNone);
		XSetWindowColormap (dsp,win,colormap);
		XAllocColorCells (dsp, colormap, True, NULL, 0,
					pixel_returns, 129);
		for (i = 128, j = 0; i >= 0; i -= 8, j += 1) {
		    pixelx[j].pixel = i;
		    pixelx[j].red = pixels[j];
		    pixelx[j].green = pixels[j];
		    pixelx[j].blue = pixels[j];
		    pixelx[j].flags = DoRed | DoGreen | DoBlue;
		}
		for (i = 0, j = 0; i < 129; i += 1) {
		    if (((i&0x07) != 0) || (i >= 129)) {
                        pixel_returns[j] = i;
                        j += 1;
                    }
                }      
                XFreeColors(dsp, colormap, pixel_returns, 129-17, 0);
                XStoreColors(dsp, colormap, pixelx, 17);
                cmap_atom = XInternAtom (dsp, "WM_COLORMAP_WINDOWS", False);
                XChangeProperty (dsp,basewin,cmap_atom,XA_WINDOW,
                        32, PropModeAppend, (unsigned char *) &win, 1);
            }
            xv_set(offCanvas, WIN_BACK, NULL);
        } else {                                /* Turn AA canvas on */
            Canvas tmpCanvas = offCanvas;
 
            offCanvas = canvas;
            canvas = tmpCanvas;
            xv_set(offCanvas, WIN_BACK, NULL);
            win = (Window) xv_get(canvas_paint_window(canvas), XV_XID);
        }
        XSync(dsp, False);
        lastFlag = flag;
    } else if (!flag && (lastFlag == ON)) {     /* Turn AA canvas off */
        if (offCanvas) {
            Canvas tmpCanvas = offCanvas;
 
            offCanvas = canvas;
            canvas = tmpCanvas;
            xv_set(offCanvas, WIN_BACK, NULL);
        }
        win = (Window) xv_get(canvas_paint_window(canvas), XV_XID);
        XSync(dsp, False);
        lastFlag = flag;
    }
    return;
}    

void
update_page()
{
    int         dx = abs(x2 - x1);
    int         dy = abs(y2 - y1);

    if (pageview.aa && PostScript) {
	int AApixh = pixh/AAscale;
	int AApixw = pixw/AAscale;
	PageViewConn *tmp;
	int tag;

	tmp = get_current();
	set_current(NeWSconn);
	ps_AAimagecanvas(win, pixmap, x2, y2, AApixh);
        if (x1 <= x2) {
            ps_AAclearedge(win, x1, y1, dx, AApixh);
            if (y1 <= y2)
                ps_AAclearedge(win, x2, y1, AApixw - dx, dy);
            else
                ps_AAclearedge(win, x2, y2 + AApixh, AApixw - dx, dy);
        } else {
            ps_AAclearedge(win, x2 + AApixw, y1, dx, AApixh);
            if (y1 <= y2)
                ps_AAclearedge(win, x1, y1, AApixw - dx, dy);
            else
                ps_AAclearedge(win, x1, y2 + AApixh, AApixw - dx, dy);
        }
        ps_synch();
        if (ps_read_tag(&tag) <= 0) {
            warning(MGET("Unknown response received from NeWS Server.. Aborting."));
            exit(-42);
        }
	set_current(tmp);
	return;
    }

    if (low_memory == FALSE) {
       if (mono)
	   XCopyPlane(dsp, pixmap, win, gc, 0, 0, pixw, pixh, x2, y2, 1L);
       else
	   XCopyArea(dsp, pixmap, win, gc, 0, 0, pixw, pixh, x2, y2);

       if (x1 <= x2) {
	   XFillRectangle(dsp, win, gc, x1, y1, dx, pixh);
	   if (y1 <= y2)
	       XFillRectangle(dsp, win, gc, x2, y1, pixw - dx, dy);
	   else
	       XFillRectangle(dsp, win, gc, x2, y2 + pixh, pixw - dx, dy);
       } else {
	   XFillRectangle(dsp, win, gc, x2 + pixw, y1, dx, pixh);
	   if (y1 <= y2)
	       XFillRectangle(dsp, win, gc, x1, y1, pixw - dx, dy);
	   else
	       XFillRectangle(dsp, win, gc, x1, y2 + pixh, pixw - dx, dy);
	   }
       }

    else
       GotoPage (CurrentPage);

}

void
home_page()
{
    x2 = y2 = 0;
    if (pageview.aa && PostScript) {
	PageViewConn *tmp;
	int tag;

	tmp = get_current();
	set_current(NeWSconn);
	ps_AAclearedge(win, 0, 0, vieww, viewh);
	ps_AAimagecanvas(win, pixmap, x2, y2, (pixh/AAscale));
        ps_synch();
        if (ps_read_tag(&tag) <= 0) {
            warning(MGET("Unknown response received from NeWS Server.. Aborting."));
            exit(-42);
        }
	set_current(tmp);
	return;
    }

    if (low_memory == FALSE) {
       XFillRectangle(dsp, win, gc, 0, 0, vieww, viewh);
       if (mono)
	   XCopyPlane(dsp, pixmap, win, gc, 0, 0, pixw, pixh, x2, y2, 1L);
       else
	   XCopyArea(dsp, pixmap, win, gc, 0, 0, pixw, pixh, x2, y2);
       }
    else
       XFillRectangle(dsp, win, low_memory_gc, 0, 0, vieww, viewh);

}


