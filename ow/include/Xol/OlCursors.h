#ifndef	_XOL_OLCURSORS_H
#define	_XOL_OLCURSORS_H

#pragma	ident	"@(#)OlCursors.h	302.1	92/03/26 include/Xol SMI"	/* olmisc:OlCursors.h 1.2 	*/

/*
 *        Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                    All rights reserved.
 *          Notice of copyright on this source code 
 *          product does not indicate publication. 
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 *
 *    Sun Microsystems, Inc., 2550 Garcia Avenue,
 *    Mountain View, California 94043.
 *
 */

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <Xol/OpenLook.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


/* The following two structs implement the cache for storing the 
 * appropriate 50 & 75 percent stipples PER screen 
 */

typedef struct _Ol50PercentGrey {
	Screen  *scr;
	Pixmap  pixmap;
	struct _Ol50PercentGrey *next;
} _Ol50PercentGrey;

typedef struct _Ol75PercentGrey {
	Screen  *scr;
	Pixmap  pixmap;
	struct _Ol75PercentGrey *next;
} _Ol75PercentGrey;


#if	defined(__STDC__) || defined(__cplusplus)

extern Cursor		OlGetMoveCursor(Widget widget);
extern Cursor		GetOlMoveCursor(Screen* screen);
extern Cursor		OlGetDuplicateCursor(Widget widget);
extern Cursor		GetOlDuplicateCursor(Screen* screen);
extern Cursor		OlGetBusyCursor(Widget widget);
extern Cursor		GetOlBusyCursor(Screen* screen);
extern Cursor		OlGetPanCursor(Widget widget);
extern Cursor		GetOlPanCursor(Screen* screen);
extern Cursor		OlGetQuestionCursor(Widget widget);
extern Cursor		GetOlQuestionCursor(Screen* screen);
extern Cursor		OlGetTargetCursor(Widget widget);
extern Cursor		GetOlTargetCursor(Screen* screen);
extern Cursor		OlGetStandardCursor(Widget widget);
extern Cursor		GetOlStandardCursor(Screen* screen);
extern Cursor		OlGetDocCursor(Widget widget);
extern Cursor		GetOlDocCursor(Screen* screen);
extern Cursor		OlGetDocStackCursor(Widget widget);
extern Cursor		GetOlDocStackCursor(Screen* screen);
extern Cursor		OlGetDropCursor(Widget widget);
extern Cursor		GetOlDropCursor(Screen* screen);
extern Cursor		OlGetDupeDocCursor(Widget widget);
extern Cursor		GetOlDupeDocCursor(Screen* screen);
extern Cursor		OlGetDupeDocDragCursor(Widget widget);
extern Cursor		GetOlDupeDocDragCursor(Screen* screen);
extern Cursor		OlGetDupeDocDropCursor(Widget widget);
extern Cursor		GetOlDupeDocDropCursor(Screen* screen);
extern Cursor		OlGetDupeDocNoDropCursor(Widget widget);
extern Cursor		GetOlDupeDocNoDropCursor(Screen* screen);
extern Cursor		OlGetDupeStackCursor(Widget widget);
extern Cursor		GetOlDupeStackCursor(Screen* screen);
extern Cursor		OlGetDupeStackDragCursor(Widget widget);
extern Cursor		GetOlDupeStackDragCursor(Screen* screen);
extern Cursor		OlGetDupeStackDropCursor(Widget widget);
extern Cursor		GetOlDupeStackDropCursor(Screen* screen);
extern Cursor		OlGetDupeStackNoDropCursor(Widget widget);
extern Cursor		GetOlDupeStackNoDropCursor(Screen* screen);
extern Cursor		OlGetMoveDocCursor(Widget widget);
extern Cursor		GetOlMoveDocCursor(Screen* screen);
extern Cursor		OlGetMoveDocDragCursor(Widget widget);
extern Cursor		GetOlMoveDocDragCursor(Screen* screen);
extern Cursor		OlGetMoveDocDropCursor(Widget widget);
extern Cursor		GetOlMoveDocDropCursor(Screen* screen);
extern Cursor		OlGetMoveDocNoDropCursor(Widget widget);
extern Cursor		GetOlMoveDocNoDropCursor(Screen* screen);
extern Cursor		OlGetMoveStackCursor(Widget widget);
extern Cursor		GetOlMoveStackCursor(Screen* screen);
extern Cursor		OlGetMoveStackDragCursor(Widget widget);
extern Cursor		GetOlMoveStackDragCursor(Screen* screen);
extern Cursor		OlGetMoveStackDropCursor(Widget widget);
extern Cursor		GetOlMoveStackDropCursor(Screen* screen);
extern Cursor		OlGetMoveStackNoDropCursor(Widget widget);
extern Cursor		GetOlMoveStackNoDropCursor(Screen* screen);
extern Cursor		OlGetNoDropCursor(Widget widget);
extern Cursor		GetOlNoDropCursor(Screen* screen);
extern Cursor		OlGetTextDupeDragCursor(Widget widget);
extern Cursor		GetOlTextDupeDragCursor(Screen* screen);
extern Cursor		OlGetTextDupeDropCursor(Widget widget);
extern Cursor		GetOlTextDupeDropCursor(Screen* screen);
extern Cursor		OlGetTextDupeNoDropCursor(Widget widget);
extern Cursor		GetOlTextDupeNoDropCursor(Screen* screen);
extern Cursor		OlGetTextMoveDragCursor(Widget widget);
extern Cursor		GetOlTextMoveDragCursor(Screen* screen);
extern Cursor		OlGetTextMoveDropCursor(Widget widget);
extern Cursor		GetOlTextMoveDropCursor(Screen* screen);
extern Cursor		OlGetTextMoveNoDropCursor(Widget widget);
extern Cursor		GetOlTextMoveNoDropCursor(Screen* screen);
extern Cursor		OlGetTextMoveInsertCursor(Widget widget);
extern Cursor		GetOlTextMoveInsertCursor(Screen* screen);
extern Cursor		OlGetTextDupeInsertCursor(Widget widget);
extern Cursor		GetOlTextDupeInsertCursor(Screen* screen);
extern Cursor		OlGetDataDupeDragCursor(Widget widget);
extern Cursor		GetOlDataDupeDragCursor(Screen* screen);
extern Cursor		OlGetDataDupeDropCursor(Widget widget);
extern Cursor		GetOlDataDupeDropCursor(Screen* screen);
extern Cursor		OlGetDataDupeInsertCursor(Widget widget);
extern Cursor		GetOlDataDupeInsertCursor(Screen* screen);
extern Cursor		OlGetDataDupeNoDropCursor(Widget widget);
extern Cursor		GetOlDataDupeNoDropCursor(Screen* screen);
extern Cursor		OlGetDataMoveDragCursor(Widget widget);
extern Cursor		GetOlDataMoveDragCursor(Screen* screen);
extern Cursor		OlGetDataMoveDropCursor(Widget widget);
extern Cursor		GetOlDataMoveDropCursor(Screen* screen);
extern Cursor		OlGetDataMoveInsertCursor(Widget widget);
extern Cursor		GetOlDataMoveInsertCursor(Screen* screen);
extern Cursor		OlGetDataMoveNoDropCursor(Widget widget);
extern Cursor		GetOlDataMoveNoDropCursor(Screen* screen);
extern Cursor		OlGetFolderCursor(Widget widget);
extern Cursor		GetOlFolderCursor(Screen* screen);
extern Cursor		OlGetFolderStackCursor(Widget widget);
extern Cursor		GetOlFolderStackCursor(Screen* screen);
extern Pixmap		OlGet50PercentGrey(Screen* screen);
extern Pixmap		OlGet75PercentGrey(Screen* screen);

#else	/* __STDC__ || __cplusplus */

extern Cursor		OlGetMoveCursor();
extern Cursor		GetOlMoveCursor();
extern Cursor		OlGetDuplicateCursor();
extern Cursor		GetOlDuplicateCursor();
extern Cursor		OlGetBusyCursor();
extern Cursor		GetOlBusyCursor();
extern Cursor		OlGetPanCursor();
extern Cursor		GetOlPanCursor();
extern Cursor		OlGetQuestionCursor();
extern Cursor		GetOlQuestionCursor();
extern Cursor		OlGetTargetCursor();
extern Cursor		GetOlTargetCursor();
extern Cursor		OlGetStandardCursor();
extern Cursor		GetOlStandardCursor();
extern Cursor		OlGetDocCursor();
extern Cursor		GetOlDocCursor();
extern Cursor		OlGetDocStackCursor();
extern Cursor		GetOlDocStackCursor();
extern Cursor		OlGetDropCursor();
extern Cursor		GetOlDropCursor();
extern Cursor		OlGetDupeDocCursor();
extern Cursor		GetOlDupeDocCursor();
extern Cursor		OlGetDupeDocDragCursor();
extern Cursor		GetOlDupeDocDragCursor();
extern Cursor		OlGetDupeDocDropCursor();
extern Cursor		GetOlDupeDocDropCursor();
extern Cursor		OlGetDupeDocNoDropCursor();
extern Cursor		GetOlDupeDocNoDropCursor();
extern Cursor		OlGetDupeStackCursor();
extern Cursor		GetOlDupeStackCursor();
extern Cursor		OlGetDupeStackDragCursor();
extern Cursor		GetOlDupeStackDragCursor();
extern Cursor		OlGetDupeStackDropCursor();
extern Cursor		GetOlDupeStackDropCursor();
extern Cursor		OlGetDupeStackNoDropCursor();
extern Cursor		GetOlDupeStackNoDropCursor();
extern Cursor		OlGetMoveDocCursor();
extern Cursor		GetOlMoveDocCursor();
extern Cursor		OlGetMoveDocDragCursor();
extern Cursor		GetOlMoveDocDragCursor();
extern Cursor		OlGetMoveDocDropCursor();
extern Cursor		GetOlMoveDocDropCursor();
extern Cursor		OlGetMoveDocNoDropCursor();
extern Cursor		GetOlMoveDocNoDropCursor();
extern Cursor		OlGetMoveStackCursor();
extern Cursor		GetOlMoveStackCursor();
extern Cursor		OlGetMoveStackDragCursor();
extern Cursor		GetOlMoveStackDragCursor();
extern Cursor		OlGetMoveStackDropCursor();
extern Cursor		GetOlMoveStackDropCursor();
extern Cursor		OlGetMoveStackNoDropCursor();
extern Cursor		GetOlMoveStackNoDropCursor();
extern Cursor		OlGetNoDropCursor();
extern Cursor		GetOlNoDropCursor();
extern Cursor		OlGetTextDupeDragCursor();
extern Cursor		GetOlTextDupeDragCursor();
extern Cursor		OlGetTextDupeDropCursor();
extern Cursor		GetOlTextDupeDropCursor();
extern Cursor		OlGetTextDupeNoDropCursor();
extern Cursor		GetOlTextDupeNoDropCursor();
extern Cursor		OlGetTextMoveDragCursor();
extern Cursor		GetOlTextMoveDragCursor();
extern Cursor		OlGetTextMoveDropCursor();
extern Cursor		GetOlTextMoveDropCursor();
extern Cursor		OlGetTextMoveNoDropCursor();
extern Cursor		GetOlTextMoveNoDropCursor();
extern Cursor		OlGetTextMoveInsertCursor();
extern Cursor		GetOlTextMoveInsertCursor();
extern Cursor		OlGetTextDupeInsertCursor();
extern Cursor		GetOlTextDupeInsertCursor();
extern Cursor		OlGetDataDupeDragCursor();
extern Cursor		GetOlDataDupeDragCursor();
extern Cursor		OlGetDataDupeDropCursor();
extern Cursor		GetOlDataDupeDropCursor();
extern Cursor		OlGetDataDupeInsertCursor();
extern Cursor		GetOlDataDupeInsertCursor();
extern Cursor		OlGetDataDupeNoDropCursor();
extern Cursor		GetOlDataDupeNoDropCursor();
extern Cursor		OlGetDataMoveDragCursor();
extern Cursor		GetOlDataMoveDragCursor();
extern Cursor		OlGetDataMoveDropCursor();
extern Cursor		GetOlDataMoveDropCursor();
extern Cursor		OlGetDataMoveInsertCursor();
extern Cursor		GetOlDataMoveInsertCursor();
extern Cursor		OlGetDataMoveNoDropCursor();
extern Cursor		GetOlDataMoveNoDropCursor();
extern Cursor		OlGetFolderCursor();
extern Cursor		GetOlFolderCursor();
extern Cursor		OlGetFolderStackCursor();
extern Cursor		GetOlFolderStackCursor();
extern Pixmap		OlGet50PercentGrey();
extern Pixmap		OlGet75PercentGrey();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_OLCURSORS_H */
