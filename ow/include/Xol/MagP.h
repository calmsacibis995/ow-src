#ifndef	_XOL_MAGP_H
#define	_XOL_MAGP_H

#pragma	ident	"@(#)MagP.h	302.1	92/03/26 include/Xol SMI"	/* help:include/Xol/MagP.h 1.5 	*/

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

/*
 *************************************************************************
 *
 * Date:	March 1989
 *
 * Description:
 *		Private header file for the Magnifier widget
 *
 *******************************file*header*******************************
 */


#include <Xol/PrimitiveP.h>	/* include superclasses's header */
#include <Xol/Mag.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 ***********************************************************************
 *
 * Widget Private Data
 *
 ***********************************************************************
 */

			/* New fields for the widget class record	*/

typedef struct {
	int junk;			/* No new class procedures	*/
} MagClassPart;

				/* Full class record declaration 	*/

typedef struct _MagClassRec {
    CoreClassPart	core_class;
    PrimitiveClassPart	primitive_class;
    MagClassPart	mag_class;
} MagClassRec;

extern MagClassRec magClassRec;

/*
 ***********************************************************************
 *
 * Instance (widget) structure 
 *
 ***********************************************************************
 */

				/* New fields for the widget record	*/

typedef struct {
	Position	mouseX;		/* X Coordinate of mouse	*/
	Position	mouseY;		/* Y Coordinate of mouse	*/
	GC		copy_GC;	/* used to copy RootWindow	*/
	GC		hold_GC;	/* used to copy cached image	*/
	GC		mag_GC;		/* magnifier GC			*/
	Pixmap		mag_pixmap;	/* magnifier pixmap		*/
	Pixmap		hold_pixmap;	/* cached image pixmap		*/
} MagPart;

					/* Full Widget declaration	*/

typedef struct _MagRec {
    CorePart		core;
    PrimitivePart	primitive;
    MagPart		mag;
} MagRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_MAGP_H */
