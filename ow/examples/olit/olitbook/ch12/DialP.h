/*****************************************************************************
 * DialP.h: Private header file for Dial Widget Class
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

#ifndef DIALP_H
#define DIALP_H

#define MAXSEGMENTS 200

typedef struct _XsDialClassPart {
       int ignore;
} XsDialClassPart;

typedef struct _XsDialClassRec {
   CoreClassPart    core_class;
   XsDialClassPart  dial_class;
} XsDialClassRec;

extern XsDialClassRec XsdialClassRec;


typedef struct _XsDialPart {
   Pixel     indicator_color;  /* Color of the           */
   Pixel     foreground;       /*  indicator and markers */
   int       minimum;          /* minimum value          */
   int       maximum;          /* maximum value          */
   int       markers;          /* number of marks        */
   Dimension marker_length;    /* in pixels              */
   Position  position;         /* indicator position     */
   Position  indicator_x;      /* x,y position of tip    */
   Position  indicator_y;      /*     of the indicator   */
   Position  center_x;         /* coordinates of the     */
   Position  center_y;         /*     dial center        */
   Position  inner_diam;       /* inside of markers      */
   Position  outer_diam;       /* outside of markers     */  
   GC        dial_GC;          /* assorted gc's          */
   GC        indicator_GC;
   GC        inverse_GC;
   XSegment  segments[MAXSEGMENTS];
   XtCallbackList select;    /* callback list          */
} XsDialPart;

typedef struct _XsDialRec {
   CorePart          core;
   XsDialPart        dial;
} XsDialRec;

#endif DIALP_H
