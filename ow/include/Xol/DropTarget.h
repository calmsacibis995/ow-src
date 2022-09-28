#ifndef	_XOL_DROPTARGET_H
#define	_XOL_DROPTARGET_H

#pragma	ident	"@(#)DropTarget.h	302.3	92/10/06 include/Xol SMI"	/* OLIT	*/

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */


#include <Xol/OlDnDVCX.h>
#include <Xol/Pixmap.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 * Widget declarations:
 */

typedef struct _DropTargetClassRec	*DropTargetWidgetClass;
typedef struct _DropTargetRec		*DropTargetWidget;

extern WidgetClass	dropTargetWidgetClass;

/*
 *  DropTarget Callback Structure
 */

typedef struct
{
	int			reason;
	Widget			widget;
	Window			window;
	Position		root_x;
	Position		root_y;
	Atom			selection;
	Time			time;
	OlDnDDropSiteID		dropsiteid;
	OlDnDTriggerOperation	operation;
	Boolean			send_done;
	Boolean			forwarded;
	int			eventcode;
	Boolean			sensitivity;
} OlDropTargetCallbackStruct;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_DROPTARGET_H */
