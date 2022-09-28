/*****************************************************************************
 * draw.h: declarations for the draw program
 *
 *         From:
 *                   The X Window System, 
 *            Programming and Applications with Xt
 *                   OPEN LOOK Edition
 *         by
 *              Douglas Young & John Pew
 *              Prentice Hall, 1991
 *
 *              Example described on pages: 
 *
 *
 *  Copyright 1991 by Prentice Hall
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
#include <X11/cursorfont.h>
#include <X11/Intrinsic.h> 
#include <X11/Xutil.h>
#include <Xol/OpenLook.h>
#include <Xol/DrawArea.h>
#include <Xol/ControlAre.h>
#include <Xol/Form.h>
#include <Xol/RectButton.h>
#include <Xol/Exclusives.h>
#include "bitmaps.h"

typedef struct _bitmap_struct {
    unsigned char *bitmap;
    Dimension width;
    Dimension height;
} bitmap_struct;

bitmap_struct bitmaps[] = {
  solid_bits,       solid_width,       solid_height,
  clear_bits,       clear_width,       clear_height,
  vertical_bits,    vertical_width,    vertical_height,
  horizontal_bits,  horizontal_width,  horizontal_height,
  slant_right_bits, slant_right_width, slant_right_height,
  slant_left_bits,  slant_left_width,  slant_left_height,
  fg50_bits,        fg50_width,        fg50_height,
  fg25_bits,        fg25_width,        fg25_height,
  cross_bits,       cross_width,       cross_height,
};

#define MAXOBJECTS 1000

typedef struct {
  int  x1, y1, x2, y2;
  int  (*func) ();
  GC   gc;
} GBUFFER;

typedef struct {
  int          start_x, start_y, last_x, last_y;
  GC           xorgc;
  GC           gc;
  int          (*current_func)();
  int          foreground, background;
  GBUFFER      buffer[MAXOBJECTS];
  int          next_pos;
} graphics_data;

static void  draw_line(Widget, GC, Position, Position, Position, Position);
static void  draw_circle(Widget, GC, Position, Position, Position, Position);
static void  draw_rectangle(Widget, GC, Position, Position, Position, Position);
static void  draw_filled_circle(Widget, GC, Position, 
				Position, Position, Position);
static void  draw_filled_rectangle(Widget, GC, Position, 
				Position, Position, Position);

static void  activate(Widget, XtPointer, XtPointer);
static void  refresh(Widget, XtPointer, XtPointer);
static void  start_rubber_band(Widget, graphics_data *, XEvent *);
static void  track_rubber_band(Widget, graphics_data *, XEvent *);
static void  end_rubber_band(Widget, graphics_data *, XEvent *);
static void  set_fill_pattern(Widget, XtPointer, XtPointer);
static void  create_drawing_commands(Widget, graphics_data *);
static void  check_points(Position *, Position *, Position *, Position *);
static void  init_data(Widget, graphics_data *);
static void  store_object(graphics_data *);
static GC     create_xor_gc(Widget);
static Widget create_pixmap_button(Widget, XImage *);
static Widget create_pixmap_browser(Widget, bitmap_struct *,
		    int, void (*)(), XtPointer, WidgetList *);
static Widget create_pixmap_button(Widget, XImage *);
static XImage *create_image(Widget, unsigned char *, Dimension, Dimension);
