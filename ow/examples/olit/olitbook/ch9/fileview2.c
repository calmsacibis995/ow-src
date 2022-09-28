/*****************************************************************************
 * fileview2.c: a file viewer
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

#include "fileview.h"

extern Widget create_scrollbar ();
short         large_rbearing = 0;

main (argc, argv)
  int       argc;
  char     *argv[];
{
  Widget       toplevel, rt, sb, sw;
  Arg          wargs[10];
  int          n;
  text_data    data;
  XtAppContext app;

  OlToolkitInitialize((XtPointer)NULL);
  toplevel = XtAppInitialize(&app, "Fileview", (XrmOptionDescList)NULL,
                             0, (Cardinal *)&argc, argv, NULL, 
                             (ArgList) NULL, 0);
  XtGetApplicationResources(toplevel, &data, resources,
                            XtNumber(resources), NULL, 0);
  /*
   * Read the file specified in argv[1] into the text buffer.
   */
  data.canvas_height = data.canvas_width = 0;
  load_file(&data, (argc == 2) ? argv[1] : NULL);
  /*
   * Create a RubberTile widget as a base.
   */
  n = 0;
  XtSetArg(wargs[n], XtNorientation, OL_HORIZONTAL); n++;
  rt = XtCreateManagedWidget("rt", rubberTileWidgetClass,
                              toplevel, wargs, n);
  /*
   * Create the drawing surface.
   */
  n = 0;
  XtSetArg(wargs[n], XtNborderWidth, 1); n++;
  XtSetArg(wargs[n], XtNwidth, large_rbearing+(MARGIN*2)); n++;
  XtSetArg(wargs[n], XtNheight,
          (data.fontheight*displaylines)+VERTMARGIN); n++;
  data.canvas= XtCreateManagedWidget("canvas", drawAreaWidgetClass,
                                     rt, wargs, n);
  /*
   * Determine the initial size of the canvas and store it
   */
  n = 0;
  XtSetArg(wargs[n], XtNheight, &data.canvas_height); n++;
  XtSetArg(wargs[n], XtNwidth, &data.canvas_width); n++;
  XtGetValues(data.canvas, wargs, n);
  /*
   * Create the scrollbar
   */
  data.scrollbar = create_scrollbar(rt, &data);
  /*
   * Register callbacks for resizes and exposes.
   */
  XtAddCallback(data.canvas, XtNexposeCallback,
                handle_exposures, &data);
  XtAddCallback(data.canvas, XtNresizeCallback,
                resize, &data);
  XtRealizeWidget(toplevel);
  create_gc(&data);
  XtAppMainLoop(app);
}

load_file(data, filename)
  text_data    *data;
  char         *filename;
{
  int           foreground, background, i, dir, ascent, desc;
  XCharStruct   char_info;
  FILE         *fp, *fopen();
  char          buf[MAXLINESIZE];

  /*
   * Open the file.
   */
  if((fp = fopen(filename, "r")) == NULL) {
    fprintf(stderr, "Unable to open %s\n", filename);
    exit(1);
  }
  /*
   * Read each line of the file into the buffer,
   * calculating and caching the extents of each line.
   */
  i = 0;
  while((fgets(buf, MAXLINESIZE, fp)) != NULL && i < MAXLINES) {
    data->chars[i] = XtMalloc(strlen(buf) + 1);
    buf[strlen(buf) - 1] = '\0';
    strcpy(data->chars[i], buf);
    data->length[i] = strlen(data->chars[i]);
    XTextExtents(data->font, data->chars[i], data->length[i], 
                 &dir, &ascent, &desc, &char_info);
    data->rbearing[i] = char_info.rbearing;
    if(large_rbearing < char_info.rbearing)
      large_rbearing = char_info.rbearing;
    data->descent     = desc;
    data->fontheight  = ascent + desc;
    i++;
  }
  /*
   * Close the file.
   */
  fclose(fp);
  /*
   * Remember the number of lines, and initialize the
   * current line number to be 0.
   */
  data->nitems = i;
  data->top = 0;
}

Widget
create_scrollbar(parent, data)
  Widget        parent;
  text_data    *data;
{
  Arg    wargs[10];
  int    n = 0;
  Widget scrollbar;

  /*
   * Set the scrollbar so that movements are reported 
   * in terms of lines of text. Set the scrolling 
   * granularity to a single line, and the proportion 
   * indicator to the size of the canvas widget.  
   * Also turn on the page indicator.
   */
  n = 0;
  XtSetArg(wargs[n], XtNsliderMin, 0); n++;
  XtSetArg(wargs[n], XtNsliderMax, data->nitems); n++;
  XtSetArg(wargs[n], XtNgranularity, 1); n++;
  XtSetArg(wargs[n], XtNproportionLength, displaylines); n++;
  XtSetArg(wargs[n], XtNweight, 0); n++;
  XtSetArg(wargs[n], XtNshowPage, OL_RIGHT); n++;
  scrollbar = XtCreateManagedWidget("scrollbar",
                                     scrollbarWidgetClass,
                                     parent, wargs, n);

  XtAddCallback(scrollbar, XtNsliderMoved,
                 scroll_bar_moved, data);
  return(scrollbar);
}

create_gc(data)
  text_data  *data;
{
  XGCValues  gcv;
  Display   *dpy  = XtDisplay(data->canvas);
  Window     w    = XtWindow(data->canvas);
  int        mask = GCFont | GCForeground | GCBackground;
  Arg        wargs[10];
  int        n;

  /*
   * Create a GC using the colors of the canvas widget.
   */
  n = 0;
  XtSetArg(wargs[n], XtNbackground, &gcv.background); n++;
  XtSetArg(wargs[n], XtNforeground, &gcv.foreground); n++;
  XtGetValues(data->canvas, wargs, n);

  gcv.font       = data->font->fid;
  data->gc       = XCreateGC(dpy, w, mask, &gcv);
}

void
handle_exposures(w, client_data, call_data)
  Widget          w;
  XtPointer       client_data;
  XtPointer       call_data;
{
  text_data *data = (text_data *)client_data;
  int     yloc = 0, index = data->top;
  Region  region;
  OlDrawAreaCallbackStruct *cb = 
                          (OlDrawAreaCallbackStruct *)call_data;

  /*
   * Create a region and add the contents of the event
   */
  region = XCreateRegion();
  XtAddExposureToRegion(cb->event, region);
  /*
   * Set the clip mask of the GC.
   */
  XSetRegion(XtDisplay(w), data->gc, region);
  /*
   * Loop through each line until the bottom of the
   * window is reached, or we run out of lines. Redraw any
   * lines that intersect the exposed region.
   */
  while(index < data->nitems && yloc < (int)data->canvas_height) {
    yloc += data->fontheight;
    if(XRectInRegion(region, MARGIN, yloc - data->fontheight,
                     data->rbearing[index],
                     data->fontheight) != RectangleOut)
       XDrawImageString(XtDisplay(w), XtWindow(w), data->gc,
                        MARGIN, yloc, data->chars[index],
                        data->length[index]);
    index++;
  }
  /*
   * Free the region.
   */
  XDestroyRegion(region);
}

void
scroll_bar_moved(w, client_data, call_data)
  Widget          w;
  XtPointer       client_data;
  XtPointer       call_data;
{
  text_data *data = (text_data *)client_data;
  OlScrollbarVerify *cb = (OlScrollbarVerify *)call_data;
  Arg     wargs[10];
  int     page;
  int     n = 0;
  int     sliderpos = cb->new_location;
  int     ysrc,  redraw_top, delta;

  page = (cb->new_location/displaylines)+1;
  cb->new_page = page;
  
  /*
   * Compute number of pixels the text needs to be moved.
   */
  delta = ABS((data->top - sliderpos) * data->fontheight);  
  delta = MIN(delta, (int)data->canvas_height);
  /*
   * If we are scrolling down, we start at zero and simply
   * move by the delta. The portion that must be redrawn
   * is simply between zero and delta.
   */
  ysrc = redraw_top = 0;
  /*
   * If we are scrolling up, we start at the delta and move
   * to zero. The part to redraw lies between the bottom
   * of the window and the bottom - delta.
   */
  if(sliderpos >= data->top) {
    ysrc        =  delta;
    redraw_top  =  data->canvas_height - delta;
  }
  /*
   * Set the top line of the text buffer.
   */
  data->top = sliderpos;
  /*
   * Move the existing text to its new position.
   * Turn off any clipping on the GC first.
   */
  XSetClipMask(XtDisplay(w), data->gc, None);
  XCopyArea(XtDisplay(data->canvas), XtWindow(data->canvas),
                      XtWindow(data->canvas), data->gc,
                      0, ysrc,
                      data->canvas_width, 
                      data->canvas_height - delta,
                      0,  delta - ysrc);
  /*
   * Clear the remaining area of any old text,
   * Request server to generate Expose events for the
   * area by setting exposures to TRUE.
   */
  XClearArea(XtDisplay(w), XtWindow(data->canvas),
             0, redraw_top,
             0, delta, TRUE);
}

void
resize(w, client_data, call_data)
  Widget          w;
  XtPointer       client_data;
  XtPointer       call_data;
{
  text_data *data = (text_data *)client_data;
  Arg   wargs[10];
  int   n;

  /*
   * Determine the new widget of the canvas widget.
   */
  n = 0;
  XtSetArg(wargs[n], XtNheight, &data->canvas_height);n++;
  XtSetArg(wargs[n], XtNwidth,  &data->canvas_width);n++;
  XtGetValues(w, wargs, n);
  displaylines = (int)data->canvas_height / data->fontheight;
  /*
   * Reset the scrollbar slider to indictae the relative
   * proportion of text displayed and also the new page size.
   */
  n = 0;
  XtSetArg(wargs[n], XtNproportionLength, displaylines); n++;
  XtSetValues(data->scrollbar, wargs, n);
}
