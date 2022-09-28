#ifndef	_XOL_SCROLLEDWI_H
#define	_XOL_SCROLLEDWI_H

#pragma	ident	"@(#)ScrolledWi.h	302.1	92/03/26 include/Xol SMI"	/* scrollwin:include/Xol/ScrolledWi.h 1.9 	*/

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

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct _ScrolledWindowClassRec*	ScrolledWindowWidgetClass;
typedef struct _ScrolledWindowRec*	ScrolledWindowWidget;

typedef struct _OlSWGeometries {
	Widget			sw;
	Widget			vsb;
	Widget			hsb;
	Dimension		bb_border_width;
	Dimension		vsb_width;
	Dimension		vsb_min_height;
	Dimension		hsb_height;
	Dimension		hsb_min_width;
	Dimension		sw_view_width;
	Dimension		sw_view_height;
	Dimension		bbc_width;
	Dimension		bbc_height;
	Dimension		bbc_real_width;
	Dimension		bbc_real_height;
	Boolean			force_hsb;
	Boolean			force_vsb;
}			OlSWGeometries;


extern WidgetClass	scrolledWindowWidgetClass;


#if	defined(__STDC__) || defined(__cplusplus)

extern void		 OlLayoutScrolledWindow(ScrolledWindowWidget sw,
	int being_resized);
	
extern OlSWGeometries	 GetOlSWGeometries(ScrolledWindowWidget sw);

#else	/* __STDC__ || __cplusplus */

extern void		 OlLayoutScrolledWindow();
extern OlSWGeometries	 GetOlSWGeometries();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_SCROLLEDWI_H */
