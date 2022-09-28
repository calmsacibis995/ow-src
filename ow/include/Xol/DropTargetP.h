#ifndef	_XOL_DROPTARGETP_H
#define	_XOL_DROPTARGETP_H

#pragma	ident	"@(#)DropTargetP.h	302.3	92/09/29 include/Xol SMI"	/* OLIT	*/

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


#include <Xol/DropTarget.h>
#include <Xol/OlgxP.h>
#include <Xol/PixmapP.h>
#include <Xol/PrimitiveP.h>

#include <X11/IntrinsicP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 * Class structure:
 */

typedef struct _DropTargetClassPart {
	int	unused;
} DropTargetClassPart;

typedef struct _DropTargetClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	PixmapClassPart		pixmap_class;
	DropTargetClassPart	droptarget_class;
} DropTargetClassRec;

extern DropTargetClassRec	dropTargetClassRec;

/*
 * Instance structure:
 */

typedef struct _DropTargetPart {
	/*
	 * Public:
	 */
	Boolean			full;
	Pixmap			busy_pixmap;
	OlDnDSitePreviewHints	preview_hints;
	XtCallbackList		previewCallback;
	XtCallbackList		triggerCallback;
	XtCallbackList		ownSelectionCallback;
	Atom			selectionAtom;
	Cursor			copyCursor;
	Cursor			moveCursor;
	Cursor			acceptCursor;
	Cursor			rejectCursor;
	XtCallbackList		animateCallback;

	/*
	 * Private:
	 */
	Boolean			busy;
	Cursor			dragCursor;
	Pixmap			normal_pixmap;
	OlDnDDropSiteID		dropsite_id;
	OlgxAttrs		*pAttrs;
	GC			bg_gc;
	GC			fg_gc;
	GC			hilite_gc;
} DropTargetPart;

typedef struct _DropTargetRec {
	CorePart	core;
	PrimitivePart	primitive;
	PixmapPart	pixmap;
	DropTargetPart	drop_target;
} DropTargetRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_DROPTARGETP_H */
