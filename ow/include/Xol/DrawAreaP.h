#ifndef	_XOL_DRAWAREAP_H
#define	_XOL_DRAWAREAP_H

#pragma	ident	"@(#)DrawAreaP.h	302.2	92/08/25 include/Xol SMI"	/* OLIT	*/

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


#include <Xol/BulletinBP.h>	/* include superclasses' header */
#include <Xol/DrawArea.h>
#include <Xol/ManagerP.h>

#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/ConstrainP.h>
#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct {
	XtPointer               extension;	/* No used yet */
}                       DrawAreaClassPart;


typedef struct _DrawAreaClassRec {
	CoreClassPart           core_class;
	CompositeClassPart      composite_class;
	ConstraintClassPart     constraint_class;
	ManagerClassPart        manager_class;
	BulletinBoardClassPart  bulletin_class;
	DrawAreaClassPart       drawarea_class;
}                       DrawAreaClassRec;

extern DrawAreaClassRec drawAreaClassRec;

/* New fields for the Drawing Area widget record */
typedef struct {
	XtCallbackList          exposeCallback;
	XtCallbackList          gExposeCallback;
	XtCallbackList          noExposeCallback;
	XtCallbackList          resizeCallback;
	Visual*			visual;
	Boolean                 resizeflag;
	Pixel                   foreground;
	unsigned char           dyn_flags;
	XtPointer               extension;	/* No used yet */
}                       DrawAreaPart;


/*
 * Full instance record declaration
 */
typedef struct _DrawAreaRec {
	CorePart                core;
	CompositePart           composite;
	ConstraintPart          constraint;
	ManagerPart             manager;
	BulletinBoardPart       bulletin;
	DrawAreaPart            draw_area;
}                       DrawAreaRec;

/* dynamics resource bit masks */
#define	OL_B_DRAWAREA_FG	(1 << 0)


#if	defined(__STDC__) || defined(__cplusplus)

extern void		_OlAddWMColormapWindows(Widget w);
extern void		_OlDelWMColormapWindows(Widget w);

#else	/* __STDC__ || __cplusplus */

extern void		_OlAddWMColormapWindows();
extern void		_OlDelWMColormapWindows();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_DRAWAREAP_H */
