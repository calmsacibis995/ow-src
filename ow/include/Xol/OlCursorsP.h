#ifndef	_XOL_OLCURSORS_H
#define	_XOL_OLCURSORS_H

#pragma	ident	"@(#)OlCursorsP.h	302.1	92/03/26 include/Xol SMI"	/* OLIT	*/

/*
 *	Copyright (C) 1986,1992  Sun Microsystems, Inc
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


#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


extern Cursor		_OlCreateCursorFromData(Screen* screen, Colormap cmap,
	const unsigned char* sbits, const unsigned char* mbits, int w, int h,
	int xhot, int yhot);

extern void		_OlFlush50PercentGreyCache(void);
extern void		_OlFlush75PercentGreyCache(void);


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_OLCURSORS_H */
