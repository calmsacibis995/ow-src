/*****************************************************************************
 * draw.c: a simple graphics drawing program.
 *
 *         From:
 *                   The X Window System, 
 *            Programming and Applications with Xt
 *                   OPEN LOOK Edition
 *         by
 *              Douglas Young & John Pew
 *              Prentice Hall, 1993
 *
 *              Example described on pages: 
 *
 *
 *  Copyright 1993 by Prentice Hall
 *  All Rights Reserved
 *
 * This code is based on the OPEN LOOK Intrinsics Toolkit (OLIT) and 
 * the X Window System
 *
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation.
 *
 * Prentice Hall and the authors disclaim all warranties with regard to 
 * this software, including all implied warranties of merchantability and 
 * fitness.
 * In no event shall Prentice Hall or the authors be liable for any special,
 * indirect or consequential damages or any damages whatsoever resulting from 
 * loss of use, data or profits, whether in an action of contract, negligence 
 * or other tortious action, arising out of or in connection with the use 
 * or performance of this software.
 *
 * OPEN LOOK is a trademark of UNIX System Laboratories.
 * X Window System is a trademark of the Massachusetts Institute of Technology
 ****************************************************************************/

#include "draw.h"

void
main(unsigned int argc, char **argv)
{
  Widget        toplevel, canvas, framework, command, tiles;
  graphics_data data;
  XtAppContext  app;
    
  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Draw", NULL, 0, &argc,
			     argv, NULL, NULL, 0);
  framework = XtCreateManagedWidget("framework", formWidgetClass, 
                                    toplevel, NULL, 0);
  /*
   * Create the column to hold the commands.
   */
  command = XtVaCreateManagedWidget("command", 
                                    controlAreaWidgetClass, 
                                    framework,
				    XtNlayoutType, OL_FIXEDCOLS,
				    NULL);
  /*
   * Create the drawing surface and add the 
   * rubber banding callbacks.
   */
  canvas = XtVaCreateManagedWidget("canvas", drawAreaWidgetClass, 
                                   framework,
				   XtNxRefWidget,    command,
				   XtNxAddWidth,     TRUE,
				   XtNyAttachBottom, TRUE,
				   XtNyVaryOffset,   FALSE,
				   XtNyResizable,    TRUE,
				   XtNxAttachRight,  TRUE,
				   XtNxVaryOffset,   FALSE,
				   XtNxResizable,    TRUE,
				   NULL);
  XtAddCallback(canvas, XtNexposeCallback, refresh, &data);

  XtAddEventHandler(canvas, ButtonPressMask, FALSE,
                    start_rubber_band, &data);
  XtAddEventHandler(canvas, ButtonMotionMask, FALSE,
                    track_rubber_band, &data);
  XtAddEventHandler(canvas, ButtonReleaseMask, FALSE,
                    end_rubber_band, &data);
  /*
   * Initialize the graphics buffer and other data.
   */
  init_data(canvas, &data);
  /*
   * Add the drawing command panel.
   */ 
  create_drawing_commands(command, &data);
  /*
   * Add a palette of fill patterns.
   */
  tiles = create_pixmap_browser(command,bitmaps, 
                                XtNumber(bitmaps), 
                                set_fill_pattern,
				(XtPointer)&data,
                                NULL);
  XtManageChild(tiles);
  XtRealizeWidget(toplevel);
  /*
   * Establish a passive grab on the drawing canvas window.
   */
  XGrabButton(XtDisplay(canvas), AnyButton, AnyModifier, 
              XtWindow(canvas), TRUE, 
              ButtonPressMask | ButtonMotionMask | 
              ButtonReleaseMask,
              GrabModeAsync, GrabModeAsync,
              XtWindow(canvas), 
              XCreateFontCursor(XtDisplay(canvas),
                                XC_crosshair));
  XtAppMainLoop(app);
}

struct {
  char   *name;
  void   (*func)(Widget, GC, Position, Position, Position, Position);
}  command_info[] = {
      "Line",             draw_line,
      "Circle",           draw_circle,
      "Rectangle",        draw_rectangle,
      "Filled Circle",    draw_filled_circle,
      "Filled Rectangle", draw_filled_rectangle
};

void
create_drawing_commands(
  Widget         parent,
  graphics_data *data)
{
  Widget  w, commands;
  int     i;
  /*
   * Group all commands in a column.
   */

  commands = XtVaCreateManagedWidget("commands", 
                                     exclusivesWidgetClass, parent, 
                                     XtNlayoutType, OL_FIXEDCOLS,
				     NULL);
  /*
   * Create a button for each drawing function.
   */
  for(i=0;i < XtNumber(command_info); i++){
    w = XtVaCreateManagedWidget(command_info[i].name,
                                rectButtonWidgetClass, 
                                commands,
				XtNuserData, command_info[i].func,
				NULL);
    XtAddCallback(w, XtNselect, activate, data);
    if(i == 0)
      XtCallCallbacks(w, XtNselect, NULL);
  }
}

void
set_fill_pattern(
  Widget    w,
  XtPointer client_data,
  XtPointer call_data)
{
  graphics_data *data = (graphics_data *)client_data;
  Pixmap    tile;
  XImage    *image;
  XGCValues values;
  GC        gc;

  static int mask = GCForeground | GCBackground | 
                    GCTile | GCFillStyle;

  XtVaGetValues(w, XtNuserData, &image, NULL);
  tile = XCreatePixmap(XtDisplay(w),
                       DefaultRootWindow(XtDisplay(w)),
                       solid_width, solid_height,
                       OlDepthOfObject(w));
  
  values.foreground = data->foreground;
  values.background = data->background;
  gc = XCreateGC(XtDisplay(w), DefaultRootWindow(XtDisplay(w)),
                GCForeground | GCBackground, &values);
  XPutImage(XtDisplay(w), tile, gc, image,
            0, 0, 0, 0, solid_width, solid_height);
  /* 
   * Get a GC using this tile pattern 
   */
  values.foreground = data->foreground;
  values.background = data->background;
  values.fill_style = FillTiled;
  values.tile       = tile;
  data->gc = XtGetGC(w, mask, &values);   
}

void
init_data(
  Widget          w,
  graphics_data  *data)
{
  XGCValues values;
  data->current_func = NULL;
  data->next_pos     = 0;

  /*
   * Get the colors the user has set for the widget. 
   */
  XtVaGetValues(w,
		XtNbackground, &data->background,
		XtNforeground, &data->foreground,
		NULL);
  /*
   * Fill in the values structure
   */  
  values.foreground = data->foreground;
  values.background = data->background;
  values.fill_style = FillTiled;
  /*
   * Get the GC used for drawing.
   */
  data->gc= XtGetGC(w, GCForeground | GCBackground |
                       GCFillStyle, &values);
  /*
   * Get a second GC in XOR mode for rubber banding.
   */
  data->xorgc = create_xor_gc(w);
}

void 
start_rubber_band(
  Widget          w,
  graphics_data  *data,
  XEvent         *event)
{
  if(data->current_func){
    /*
     * Store the starting point and draw the initial figure.
     */
    data->last_x = data->start_x = event->xbutton.x;
    data->last_y = data->start_y = event->xbutton.y;
    (*(data->current_func))(w, data->xorgc,
                            data->start_x, data->start_y, 
                            data->last_x, data->last_y);
  }
}

void
track_rubber_band(
  Widget          w,
  graphics_data  *data,
  XEvent         *event)
{ 
  if(data->current_func){
    /*
     * Erase the previous figure.
     */
    (*(data->current_func))(w, data->xorgc,
                            data->start_x, data->start_y, 
                            data->last_x, data->last_y);
    /*
     * Update the last point.
     */
    data->last_x  =  event->xbutton.x;
    data->last_y  =  event->xbutton.y;
    /*
     * Draw the figure in the new position.
     */
    (*(data->current_func))(w, data->xorgc,
                            data->start_x, data->start_y, 
                            data->last_x, data->last_y);
  }
}

void
end_rubber_band(
  Widget          w,
  graphics_data  *data,
  XEvent         *event)
{
  if(data->current_func){
    /*
     * Erase the XOR image. 
     */
    (*(data->current_func))(w, data->xorgc,
                            data->start_x, data->start_y, 
                            data->last_x, data->last_y);
    /*
     * Draw the figure using the normal GC. 
     */
    (*(data->current_func))(w, data->gc,
                            data->start_x, data->start_y, 
                            event->xbutton.x, 
                            event->xbutton.y);
    /*
     * Update the data, and store the object in 
     * the graphics buffer.
     */
    data->last_x  =  event->xbutton.x;
    data->last_y  =  event->xbutton.y;
    store_object(data);
  }
}

void 
draw_line(
  Widget   w,
  GC       gc,
  Position x,
  Position y,
  Position x2,
  Position y2)
{
  Display *dpy = XtDisplay(w);
  Window   win = XtWindow(w);
  XDrawLine(dpy, win, gc, x, y, x2, y2);
}

void 
draw_rectangle(
  Widget   w,
  GC       gc,
  Position x,
  Position y,
  Position x2,
  Position y2)
{
  Display *dpy = XtDisplay(w);
  Window   win = XtWindow(w);

  check_points(&x, &y, &x2, &y2);
  XDrawRectangle(dpy, win, gc,  x, y, x2 - x, y2 - y);
}

void
draw_filled_rectangle(
  Widget    w,
  GC        gc,
  Position  x,
  Position  y,
  Position  x2,
  Position  y2)
{
  Display *dpy = XtDisplay(w);
  Window   win = XtWindow(w);

  check_points(&x, &y, &x2, &y2);
  XFillRectangle(dpy, win, gc, x, y, x2 - x, y2 - y);
}

void
check_points(
  Position *x,
  Position *y,
  Position *x2,
  Position *y2)
{
  if(*x2 < *x){ Position tmp = *x; *x = *x2; *x2 = tmp;}
  if(*y2 < *y){ Position tmp = *y; *y = *y2; *y2 = tmp;}
}

void
draw_circle(
  Widget    w,
  GC        gc,
  Position  x,
  Position  y,
  Position  x2,
  Position  y2)
{
  Display *dpy = XtDisplay(w);
  Window   win = XtWindow(w);

  check_points(&x, &y, &x2, &y2);
  XDrawArc(dpy, win, gc, x, y, x2 - x, y2 - y, 0, 64 * 360);
}

void
draw_filled_circle(
  Widget    w,
  GC        gc,
  Position  x,
  Position  y,
  Position  x2,
  Position  y2)
{
  Display *dpy = XtDisplay(w);
  Window   win = XtWindow(w);

  check_points(&x, &y, &x2, &y2);
  XFillArc(dpy, win, gc, x, y, x2 - x, y2 - y, 0, 64 * 360);
}

void
activate(
  Widget    w,
  XtPointer client_data,
  XtPointer call_data)
{
  graphics_data  *data = (graphics_data *)client_data;
  int (*func)();

  XtVaGetValues(w, XtNuserData, &func, NULL);
  data->current_func = func; 
}

void
store_object(graphics_data *data)
{
  /* 
   * Check for space.
   */
  if(data->next_pos >= MAXOBJECTS) {
    printf("Warning: Graphics buffer is full\n");
    return;
  }
  /*
   * Save everything we need to draw this object again.
   */
  data->buffer[data->next_pos].x1 = data->start_x;
  data->buffer[data->next_pos].y1 = data->start_y;
  data->buffer[data->next_pos].x2 = data->last_x;
  data->buffer[data->next_pos].y2 = data->last_y;
  data->buffer[data->next_pos].func = data->current_func;
  data->buffer[data->next_pos].gc = data->gc;
  /*
   * Increment the next position index.
   */
  data->next_pos++;
}

void
refresh(
  Widget    w,
  XtPointer client_data,
  XtPointer call_data)
{
  graphics_data  *data = (graphics_data *)client_data;
  int i;

  for(i=0;i<data->next_pos;i++)
    (*(data->buffer[i].func))(w, data->buffer[i].gc,
                                 data->buffer[i].x1,
                                 data->buffer[i].y1,
                                 data->buffer[i].x2,
                                 data->buffer[i].y2);
}

GC
create_xor_gc(Widget w)
{
  XGCValues values;
  GC        gc;

  /*
   * Get the background and foreground colors.
   */
  XtVaGetValues(w,
		XtNforeground, &values.foreground,
		XtNbackground, &values.background,
		NULL);
  /*
   * Set the fg to the XOR of the fg and bg, so if it is
   * XOR'ed with bg, the result will be fg and vice-versa.
   * This effectively achieves inverse video for the line.
   */
  values.foreground = values.foreground ^ values.background;
  /*
   * Set the rubber band gc to use XOR mode and draw 
   * a dashed line.
   */
  values.line_style = LineOnOffDash;
  values.function   = GXxor;
  gc = XtGetGC(w, GCForeground | GCBackground | 
               GCFunction | GCLineStyle, &values);
  return gc;
}

Widget 
create_pixmap_browser(
  Widget         parent,         /* widget to manage the browser */
  bitmap_struct *bitmaps,        /* list of bitmaps              */
  int            n_bmaps,        /* how many bitmaps             */
  void           (*callback)(),  /* invoked when state changes   */
  XtPointer      data,           /* data to be passed to callback*/
  WidgetList    *widgets)
{
  Widget       browser;
  WidgetList   buttons;
  XImage     **images;
  int          i;

  /*
   * Malloc room for button widgets.
   */
  buttons = (WidgetList) XtMalloc(n_bmaps * sizeof(Widget));
  images = (XImage **) XtMalloc(n_bmaps * sizeof(XImage *));
  /*
   * Create an Exclusives widget.
   */
  browser = XtVaCreateManagedWidget("browser",
                                    exclusivesWidgetClass,
                                    parent,
				    XtNlayoutType, OL_FIXEDCOLS,
				    XtNmeasure,    3,
				    NULL);
  /*
   * Create a button for each tile. If a callback function
   * has been given, register it as an XtNselect
   */
  for(i=0;i< n_bmaps;i++) {
    images[i] = create_image(parent, bitmaps[i].bitmap,
                             bitmaps[i].width,
                             bitmaps[i].height);

    buttons[i] = create_pixmap_button(browser, images[i]);
    if(callback)
      XtAddCallback(buttons[i], XtNselect, callback, data);
  }
  /*
   * Manage all buttons and return the Exclusives widget
   */ 
  XtManageChildren(buttons, n_bmaps);
  if(widgets != NULL)
    *widgets = buttons;
  return(browser);
}

Widget
create_pixmap_button(
  Widget   parent,
  XImage  *image)
{
  Widget   button;

  /*
   * Display the XImage in the button and also store it
   * so it can be retrieved from the button later.
   */
  button = XtVaCreateWidget("", rectButtonWidgetClass,
                            parent,
			    XtNlabelType,  OL_IMAGE,
			    XtNlabelImage, image,
			    XtNuserData,   image,
			    NULL);
  /*
   * Return the unmanaged button.
   */
  return(button);
}

XImage *
create_image(
  Widget         widget,
  unsigned char *bits,
  Dimension      width,
  Dimension      height)
{
  XImage *image;
  image = XCreateImage(XtDisplay(widget),
                       OlVisualOfObject(widget), 
                       1, XYBitmap, 0, 
                       bits, width, height, 32, 0);
  return(image);
}
