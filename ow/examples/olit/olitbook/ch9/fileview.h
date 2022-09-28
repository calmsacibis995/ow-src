/*****************************************************************************
 * fileview.h: declarations for fileview program
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

#include <stdio.h>
#include <X11/Intrinsic.h> 
#include <X11/StringDefs.h>
#include <X11/Xutil.h>
#include <Xol/OpenLook.h>
#include <Xol/DrawArea.h>
#include <Xol/Scrollbar.h>
#include <Xol/RubberTile.h>

#define MAXLINESIZE  300
#define MAXLINES     2000
#define MIN(a,b)     (((a) < (b)) ? (a) : (b))
#define ABS(a)       (((a) >= 0) ? (a) : -(a))
#define MAX(a,b)     ((a > b) ? a : b)
#define MARGIN       5
#define VERTMARGIN   3
#define DISPLAYLINES 50


typedef struct {
  char         *chars[MAXLINES];   /* Lines of text         */
  int           length[MAXLINES];  /* Length of each line   */
  int           rbearing[MAXLINES];/* right bearing of line */
  int           descent;           /* descent below baseline*/
  XFontStruct  *font;              /* The font struct       */
  GC            gc;                /* A read/write GC       */
  Widget        scrollbar;
  Widget        canvas;
  Dimension     canvas_height;     /* canvas dimensions     */
  Dimension     canvas_width;
  int           fontheight;        /* descent + ascent      */
  int           nitems;            /* number of text lines  */
  int           top;               /* line at top of window */
} text_data, *text_data_ptr;

int        displaylines = DISPLAYLINES;
void       handle_exposures();
void       scroll_bar_moved ();
void       resize ();
void       graphics_exposure ();

static XtResource resources[] = {
 {XtNfont, XtCFont, XtRFontStruct, sizeof (XFontStruct *),
    XtOffset(text_data_ptr, font), XtRString, "Fixed"      },
};
