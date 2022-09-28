/*****************************************************************************
 * Dial.h: Public header file for Dial Widget Class
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

#ifndef  DIAL_H
#define  DIAL_H

extern WidgetClass XsdialWidgetClass;
typedef struct _XsDialClassRec * XsDialWidgetClass;
typedef struct _XsDialRec      * XsDialWidget;
/*
 * Define resource strings for the Dial widget.
 */
#define XtNselectCallback "selectCallback"
#define XtNmarkers        "markers"
#define XtNminimum        "minimum"
#define XtNmaximum        "maximum"
#define XtNindicatorColor "indicatorColor"
#define XtNposition       "position"
#define XtNmarkerLength   "markerLength"

#define XtCMarkers        "Markers"
#define XtCMin            "Min"
#define XtCMax            "Max"
#define Xs_SELECTED       1

typedef struct {
  XEvent *event;
  int     position;
} xsdialCallbackStruct;

#endif DIAL_H
