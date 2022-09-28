/*****************************************************************************
 * fractal.h: declarations for the fractal program
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
#include <X11/Intrinsic.h> 
#include <Xol/OpenLook.h>
#include <Xol/DrawArea.h> 
#include <X11/Xutil.h>

/*
 * Structure to represent a complex number.
 */
typedef struct {
  float  real, imag;
} complex;
/*
 * Assorted information needed to generate and draw the image.
 */
typedef struct {
  int          depth, ncolors;
  float        range, max_distance;
  complex      origin;
  GC           gc;
  Pixmap       pix;
  Dimension    width, height;
} image_data, *image_data_ptr;

static void resize(Widget, XtPointer, XtPointer);
static void redisplay(Widget, XtPointer, XtPointer);
static void create_image(Widget, image_data *);
static void init_data(Widget, image_data *);
static void init_buffer(image_data *);
static void flush_buffer(Widget, image_data *);
static void buffer_point(Widget, image_data *, int, int, int);

/*
 * Resource that affect the appearance of the fractal image.
 */
static XtResource resources[] = {
 {"depth", "Depth", XtRInt, sizeof (int),
   XtOffset(image_data_ptr, depth), XtRString, "20"         },
 {"real_origin", "RealOrigin", XtRFloat, sizeof (float),
   XtOffset(image_data_ptr, origin.real), XtRString, "-1.4" },
 {"imaginary_origin","ImaginaryOrigin",XtRFloat,sizeof(float),
   XtOffset(image_data_ptr, origin.imag), XtRString, "1.0"  },
 {"range", "Range", XtRFloat, sizeof(float),
   XtOffset(image_data_ptr,range), XtRString, "2.0"         },
 {"max_distance", "MaxDistance", XtRFloat, sizeof (float),
   XtOffset(image_data_ptr, max_distance),XtRString, "4.0"  }
 };
