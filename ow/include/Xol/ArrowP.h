#ifndef	_XOL_ARROWP_H
#define	_XOL_ARROWP_H

#pragma	ident	"@(#)ArrowP.h	302.2	92/10/02 include/Xol SMI"	/* arrow:include/openlook/ArrowP.h 1.12 	*/

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

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * $Header: ArrowP.h,v 1.7 88/02/26 09:18:20 swick Exp $
 */

/*
 *  ArrowP.h - Private definitions for Arrow widget
 */


#include <Xol/PrimitiveP.h>	/* include superclasses's header */
#include <Xol/Arrow.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*****************************************************************************
 *
 * Arrow Widget Private Data
 *
 *****************************************************************************/

/* New fields for the Arrow widget class record */
typedef struct {
	int                     empty;
}                       ArrowClassPart;

/* Full Class record declaration */
typedef struct _ArrowClassRec {
	CoreClassPart           core_class;
	PrimitiveClassPart      primitive_class;
	ArrowClassPart          arrow_class;
}                       ArrowClassRec;

/* New fields for the Arrow widget record */
typedef struct {
	XtCallbackList          btnDown;
	XtCallbackList          btnMotion;
	XtCallbackList          btnUp;
	XtIntervalId            timerid;
	OlDefine                direction;	/* OL_NONE, OL_TOP, OL_BOTTOM,
						 * OL_LEFT, OL_RIGHT */
	int                     repeatRate;
	int                     initialDelay;
	int                     centerLine;	/* GetValues Only */
	GC                      fggc,
	                        bggc;
	Boolean                 normal;
}                       ArrowPart;

/*****************************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************************/

typedef struct _ArrowRec {
	CorePart                core;
	PrimitivePart           primitive;
	ArrowPart               arrow;
}                       ArrowRec;


extern ArrowClassRec    arrowClassRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_ARROWP_H */
