/*****************************************************************************
 * coloredit.c: A simple color editor.
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

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h> 
#include <Xol/OpenLook.h>
#include <Xol/StaticText.h>
#include <Xol/ControlAre.h>
#include <Xol/Form.h>
#include <Xol/Slider.h>
#include <stdio.h>
#include <unistd.h>

#define MAXCOLORS    64
#define RED          1
#define GREEN        2
#define BLUE         3

Display    *dpy;
Colormap    my_colormap;
XColor      current_color;
int         ncolors, ncells;

static Widget      red_slider, blue_slider, green_slider,  
            	   red_text, blue_text, green_text,
	    	   toplevel, sliders, form;

static void        slider_selected(Widget, XtPointer, XtPointer);
static void        slider_moved(Widget, XtPointer, XtPointer);
static void        set_current_color(Widget, XtPointer, XEvent *, Boolean *);
static void        update_color(Widget, unsigned short);
static Widget      make_slider(String, Widget, int);
static Widget      make_text(String, Widget);
static Widget      create_color_bar(Widget);

void
main(unsigned int argc, char **argv)
{
  XtAppContext app;
  Colormap     top_colormap;
  XColor      *Colors;
  int          i;
  Visual      *visual;

 /*
  * Initialize the Intrinsics and save pointer to the display.
  */
  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Coloredit", NULL, 0, &argc, 
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
  XtVaSetValues(toplevel, XtNcolormap, my_colormap, NULL);

  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

Widget
make_slider(
  String name,
  Widget parent,
  int    color)
{
  Widget  w;

  /*
   * Create a Slider widget.
   */
  w = XtVaCreateManagedWidget(name, sliderWidgetClass, 
			      parent,
			      XtNsliderMin, 0,
			      XtNsliderMax, 65535,
			      NULL);
  /*
   * Add callbacks to be invoked when the slider moves.
   */
  XtAddCallback(w, XtNsliderMoved, slider_moved, (XtPointer)color);

  return(w);
}

Widget
make_text(
  String name,
  Widget parent)
{
  Widget  w;

  /*
   * Create a StaticText widget.
   */
  w = XtVaCreateManagedWidget(name, staticTextWidgetClass,
                              parent,
			      XtNstring, "0",
			      NULL);
  return(w);
}

Widget
create_color_bar(Widget parent)
{
  Widget      panel;
  WidgetList  colors;
  int         i;
  char        name[10];

  colors = (WidgetList) XtMalloc( ncolors * sizeof(Widget));
  /*
   * Create the ControlArea manager to hold all color buttons.
   */
  panel = XtCreateManagedWidget("colorpanel",
                                controlAreaWidgetClass,
                                parent, NULL, 0);
  /*
   * Create ncolors widgets. Use the relative color cell
   * number as the name of each color. Add an event handler for
   * each cell with the color index as client_data.
   */
  for(i=0;i<ncolors;i++) {
    sprintf(name, "%d", i);
    colors[i] = XtVaCreateWidget(name, staticTextWidgetClass,
                                 panel,
				 XtNbackground, i,
				 XtNstring, name,
				 NULL);
    XtAddEventHandler(colors[i], ButtonPressMask, False,
                      set_current_color, (XtPointer)i);
  }
  XtManageChildren(colors, ncolors);

  return(panel);
}

void
slider_moved(
  Widget    widget,
  XtPointer client_data,
  XtPointer call_data)
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
update_color(
  Widget widget,
  unsigned short color)
{
  char str[32];

  /*
   * Update the digital display.
   */
  sprintf(str, "%d", color);
  XtVaSetValues(widget, XtNstring, str, NULL);
  /*
   * Update the current color.
   */
  XStoreColor(dpy, my_colormap, &current_color);
}

void
set_current_color(
  Widget    widget,
  XtPointer client_data,
  XEvent   *event,
  Boolean  *continue_to_dispatch)
{
  int number = (int)client_data;

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
  XtVaSetValues(red_slider,
		XtNsliderValue, current_color.red, 
		NULL);

  XtVaSetValues(green_slider, 
		XtNsliderValue, current_color.green,
		NULL);

  XtVaSetValues(blue_slider,
		XtNsliderValue, current_color.blue,
		NULL);
  update_color(red_text, current_color.red);
  update_color(green_text, current_color.green);
  update_color(blue_text, current_color.blue);
}
