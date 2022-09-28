#ifndef	_XOL_PIXMAPP_H
#define	_XOL_PIXMAPP_H

#pragma	ident	"@(#)PixmapP.h	302.3	92/09/29 include/Xol SMI"	/* OLIT	*/

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


#include <Xol/Pixmap.h>
#include <Xol/PrimitiveP.h>

#include <X11/CoreP.h>
#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


#if	defined(__STDC__) || defined(__cplusplus)

typedef	void		(*OlPixmapComputeGeometry)(Widget, Boolean);

#else	/* __STDC__ || __cplusplus */

typedef	void		(*OlPixmapComputeGeometry)();

#endif	/* __STDC__ || __cplusplus */


/* inheritance tokens */

#define XtInheritOlPixmapComputeGeometry \
	((OlPixmapComputeGeometry)_XtInherit)

/*
 * Class structure:
 */

typedef struct _PixmapClassPart {
	OlPixmapComputeGeometry	compute_geometry;
} PixmapClassPart;

typedef struct _PixmapClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	PixmapClassPart		pixmap_class;
} PixmapClassRec;

extern PixmapClassRec	pixmapClassRec;

/*
 * Instance structure:
 */

typedef struct _PixmapPart {
	/*
	 * Public:
	 */
	Pixmap		pixmap;
	Boolean		recompute_size;

	/*
	 * Private:
	 */
	XRectangle	pixmap_geometry;
	GC		fg_gc;
	Boolean		is_bitmap;
} PixmapPart;

typedef struct _PixmapRec {
	CorePart	core;
	PrimitivePart	primitive;
	PixmapPart	pixmap;
} PixmapRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_PIXMAPP_H */
