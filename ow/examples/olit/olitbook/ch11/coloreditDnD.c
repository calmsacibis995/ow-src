/*****************************************************************************
 * coloreditDnD.c: A simple color editor with dragging capability
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

#include "coloredit.h"
#include <X11/Xatom.h>
#include <Xol/OlDnDVCX.h>
#include <Xol/OlCursors.h>
#include <stdio.h>
#include <unistd.h>

static Widget display_color;
static char *supported[] = {"TARGETS", "STRING" };

void
main(unsigned int argc, char **argv)
{
  Colormap     top_colormap;
  XColor      *Colors;
  int          i;
  Arg          wargs[1];
  Visual      *visual;
  void         start_drag();
  XtAppContext app;

 /*
  * Initialize the Intrinsics and save pointer to the display.
  */
  OlToolkitInitialize((XtPointer)NULL);
  OlSetDefaultTextFormat(OL_MB_STR_REP);
  toplevel = XtAppInitialize(&app, "ColoreditDnD", NULL, 0, &argc, 
			     argv, NULL, NULL, 0);
  dpy = XtDisplay(toplevel);
  visual = OlVisualOfObject(toplevel);
  /*
   * If the application's colormap is readonly then 
   * inform the user and exit
   */
  switch(visual->class) {
  case StaticGray:
  case StaticColor:
  case TrueColor:
    printf("Coloredit's colormap is non-writable, Exiting...\n");
    exit(1);
  }
  /*
   * Determine the number of colors to be edited.
   */
  ncells = visual->map_entries;
  if(ncells > MAXCOLORS)
    ncolors = MAXCOLORS;
  else
    ncolors = ncells;
  /*
   * Create a base Form widget to hold everything.
   */
  form = XtCreateManagedWidget("base", formWidgetClass,
                               toplevel, NULL, 0);
  /*
   * Create the widget to display the choosen color
   */
  display_color = XtCreateManagedWidget("display_color", 
                                        staticTextWidgetClass,
                                        form, NULL, 0);
  /*
   * Create a grid of buttons, one for each
   * color to be edited.
   */
  create_color_bar(form);
  /*
   * Create a Form widget containing three Sliders,
   * and three StaticTexts, one for each color component.
   */
  sliders = XtCreateManagedWidget("sliderpanel", formWidgetClass,
                                  form, NULL, 0);
  red_text     = make_text("redtext", sliders);
  red_slider   = make_slider("red", sliders, RED);
  green_text   = make_text("greentext", sliders);
  green_slider = make_slider("green", sliders, GREEN);
  blue_text    = make_text("bluetext", sliders);
  blue_slider  = make_slider("blue",  sliders, BLUE);
  /*
   * Get the ID of toplevel's colormap.
   */
  top_colormap = OlColormapOfObject(toplevel);
  Colors = (XColor *) XtMalloc(ncells * sizeof(XColor));
  for( i = 0; i < ncells; i++ ) {
    Colors[i].pixel = i;
    Colors[i].flags = DoRed | DoGreen | DoBlue;
  }
  XQueryColors(dpy, top_colormap, Colors, ncells);
  my_colormap = XCreateColormap(dpy,
                       RootWindowOfScreen(XtScreenOfObject(toplevel)),
                       visual, AllocAll);
  XStoreColors(dpy, my_colormap, Colors, ncells);
  /*
   * Initialize the pixel member of the global color struct
   * To the first editable color cell.
   */
  current_color.pixel = 0;
  XtSetArg(wargs[0], XtNcolormap, my_colormap);
  XtSetValues(toplevel, wargs, 1);

  XtAddEventHandler(display_color, ButtonPress, False,
                    start_drag, NULL);
  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

void
start_drag(w, client_data, event, continue_to_dispatch)
  Widget    w;
  XtPointer client_data;
  XEvent   *event;
  Boolean  *continue_to_dispatch;
{
  Atom              atom;
  Window            drop_window;
  Position          x, y;
  OlDnDDragDropInfo rinfo;
  Boolean           ConvertSelection();
  void              CleanupTransaction();
  Cursor            DragCursor;

  atom = OlDnDAllocTransientAtom(w);
  DragCursor = OlGetDuplicateCursor(display_color);
  OlGrabDragPointer(w, DragCursor, None);
  if(OlDnDDragAndDrop(w, &drop_window, &x, &y, &rinfo, NULL, NULL)) {
    if(OlDnDOwnSelection(w, atom,
                  XtLastTimestampProcessed(XtDisplay(w)),
                  ConvertSelection, 
                  NULL, NULL, CleanupTransaction, NULL) == FALSE) {
      OlUngrabDragPointer(w);
      return;
    }
    OlDnDDeliverTriggerMessage(w, rinfo.root_window, rinfo.root_x,
                        rinfo.root_y, atom, OlDnDTriggerCopyOp,
                        rinfo.drop_timestamp);
  }
  OlUngrabDragPointer(w);
}

Boolean
ConvertSelection(w, selection, target, type, value, length, format)
  Widget w;
  Atom *selection, *target, *type;
  XtPointer *value;
  unsigned long *length;
  int *format;
{
  XrmValue source, dest;
  char hexcolor[8];
  int  i;
  Boolean first = TRUE;
  static Atom *targets;

  if(first) {
    targets = (Atom *) XtMalloc(XtNumber(supported) * sizeof(Atom));
    for(i=0;i<XtNumber(supported);i++) {
      source.size = strlen(supported[i])+1;
      source.addr = supported[i];
      dest.size = sizeof(Atom);
      dest.addr = (char *)&targets[i];
      XtConvertAndStore(w, XtRString, &source, XtRAtom, &dest);
    }
    first = FALSE;
  }
  if(*target == targets[0]) {
    *type = XA_ATOM;
    *value = (XtPointer)targets;
    *length = XtNumber(supported);
    *format = 32;
    return(TRUE);
  }
  if(*target == targets[1]) {   /* XA_STRING */
    *type = XA_STRING;
    sprintf(hexcolor, "#%02x%02x%02x", 
              current_color.red/256,
              current_color.green/256,
              current_color.blue/256);
    *value = (XtPointer)hexcolor;
    *length = 7;
    *format = 8;
    return(TRUE);
  }
  return(FALSE);
}

void
CleanupTransaction(w, selection, state, timestamp, closure)
  Widget                  w;
  Atom                    selection;
  OlDnDTransactionState   state;
  Time                    timestamp;
  XtPointer               closure;
{
  switch (state) {
    case OlDnDTransactionDone:
    case OlDnDTransactionRequestorError:
    case OlDnDTransactionRequestorWindowDeath:
      OlDnDFreeTransientAtom(w, selection);
      OlDnDDisownSelection(w, selection, 
                           XtLastTimestampProcessed(XtDisplay(w)));
      break;
    case OlDnDTransactionBegins:
    case OlDnDTransactionEnds:
      break;
  }
}

Widget
make_slider(name, parent, color)
  char  *name;
  Widget parent;
  int    color;
{
  Widget  w;
  int     n;
  Arg     wargs[3];

  /*
   * Create a Slider widget.
   */
  n = 0;
  XtSetArg(wargs[n], XtNsliderMin, 0); n++;
  XtSetArg(wargs[n], XtNsliderMax, 65535); n++;
  w = XtCreateManagedWidget(name, sliderWidgetClass, parent, wargs, n);
  /*
   * Add callbacks to be invoked when the slider moves.
   */
  XtAddCallback(w, XtNsliderMoved, slider_moved, (XtPointer)color);

  return(w);
}

Widget
make_text(name, parent)
  char  *name;
  Widget parent;
{
  Widget  w;
  int     n;
  Arg     wargs[3];

  /*
   * Create a StaticText widget.
   */
  n = 0;
  XtSetArg(wargs[n], XtNstring, "0"); n++;
  w = XtCreateManagedWidget(name, staticTextWidgetClass,
                            parent, wargs, n);
  return(w);
}

Widget
create_color_bar(parent)
  Widget parent;
{
  Widget      panel;
  WidgetList  colors;
  int         i, n;
  char        name[10];
  Arg         wargs[3];

  colors = (WidgetList) XtMalloc( ncolors * sizeof(Widget));
  /*
   * Create the ControlArea manager to hold all color buttons.
   */
  n = 0;
  panel = XtCreateManagedWidget("colorpanel",
                                controlAreaWidgetClass,
                                parent, wargs, n);
  /*
   * Create ncolors widgets. Use the relative color cell
   * number as the name of each color. Add an event handler for
   * each cell with the color index as client_data.
   */
  for(i=0;i<ncolors;i++) {
    n = 0;
    XtSetArg(wargs[n], XtNbackground, i); n++;
    sprintf(name, "%d", i);
    XtSetArg(wargs[n], XtNstring, name); n++;
    colors[i] = XtCreateWidget(name, staticTextWidgetClass,
                               panel, wargs, n);
    XtAddEventHandler(colors[i], ButtonPressMask, False,
                      set_current_color, (XtPointer)i);
  }
  XtManageChildren(colors, ncolors);

  return(panel);
}

void
slider_moved(w, client_data, call_data)
  Widget   w;
  XtPointer client_data;
  XtPointer call_data;
{
  int color = (int)client_data;
  /*
   * Set the red color components of  the global
   * current_color structure.
   */
  switch(color) {
  case RED:
      current_color.red = *(int *)call_data;
      update_color(red_text, current_color.red);
      break;
  case GREEN:
      current_color.green = *(int *)call_data;
      update_color(green_text, current_color.green);
      break;
  case BLUE:
      current_color.blue = *(int *)call_data;
      update_color(blue_text, current_color.blue);
      break;
  }
}

void
update_color(w, color)
  Widget w;
  unsigned short color;
{
  char str[25];

  /*
   * Update the digital display.
   */
  sprintf(str, "%d", color);
  XtVaSetValues(w, XtNstring, str, NULL);
  /*
   * Update the current color.
   */
  XStoreColor(dpy, my_colormap, &current_color);
}

void
set_current_color(w, client_data, event, continue_to_dispatch)
  Widget    w;
  XtPointer client_data;
  XEvent   *event;
  Boolean  *continue_to_dispatch;
{
  int number = (int)client_data;
  Arg wargs[2];

  current_color.flags = DoRed | DoGreen | DoBlue;
  /*
   * Get the current color components of the selected button.
   */
  current_color.pixel = number;
  XQueryColor(dpy, my_colormap, &current_color);
  /*
   * Use each color component to set the new
   * position of the corresponding slider.
   */
  XtSetArg(wargs[0], XtNsliderValue, current_color.red);
  XtSetValues(red_slider, wargs, 1);

  XtSetArg(wargs[0], XtNsliderValue, current_color.green);
  XtSetValues(green_slider, wargs, 1);

  XtSetArg(wargs[0], XtNsliderValue, current_color.blue);
  XtSetValues(blue_slider, wargs, 1);
  update_color(red_text, current_color.red);
  update_color(green_text, current_color.green);
  update_color(blue_text, current_color.blue);
  XtSetArg(wargs[0], XtNbackground, number);
  XtSetValues(display_color, wargs, 1);
}
