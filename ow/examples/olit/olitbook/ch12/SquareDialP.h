/*****************************************************************************
 * SquareDialP.h:The SquareDial widget private header file.
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

#ifndef SQUAREDIALP_H
#define SQUAREDIALP_H

typedef struct _XsSquareDialClassPart {
   int ignore;	
} XsSquareDialClassPart;

typedef struct _XsSquareDialClassRec {
   CoreClassPart            core_class;
   XsDialClassPart          dial_class;
   XsSquareDialClassPart    square_dial_class;
} XsSquareDialClassRec;

extern XsSquareDialClassRec XssquareDialClassRec;

typedef struct _XsSquareDialPart {
  int ignore;
} XsSquareDialPart;

typedef struct _XsSquareDialRec {
   CorePart	          core;
   XsDialPart        dial;
   XsSquareDialPart  squaredial;
} XsSquareDialRec;

#endif SQUAREDIALP_H
