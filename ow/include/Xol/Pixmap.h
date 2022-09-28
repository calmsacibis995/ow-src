#ifndef	_XOL_PIXMAP_H
#define	_XOL_PIXMAP_H

#pragma	ident	"@(#)Pixmap.h	302.1	92/03/26 include/Xol SMI"	/* OLIT	*/

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


#include <Xol/Primitive.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 * Widget declarations:
 */

typedef struct _PixmapClassRec*	PixmapWidgetClass;
typedef struct _PixmapRec*	PixmapWidget;


extern WidgetClass		pixmapWidgetClass;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_PIXMAP_H */
