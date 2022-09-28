#ifndef	_XOL_RECTBUTTOP_H
#define	_XOL_RECTBUTTOP_H

#pragma	ident	"@(#)RectButtoP.h	302.2	94/01/31 include/Xol SMI"	/* button:include/openlook/RectButtoP.h 1.7 	*/

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


#include <Xol/ButtonP.h>	/* include superclasses's header */
#include <Xol/RectButton.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 *  There are no new fields in the RectButton class
 */
typedef struct _RectButtonClass {
	int makes_compiler_happy;  /* not used */
	} RectButtonClassPart;

/*
 *  declare the class record for the RectButton widget
 */
typedef struct _RectButtonClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	 primitive_class;
	ButtonClassPart		button_class;
	RectButtonClassPart	rect_button_class;
	} RectButtonClassRec;

/*
 *  rectButtonClassRec is defined in RectButton.c
 */
extern RectButtonClassRec rectButtonClassRec;

/*
 *  declaration of the RectButton widget fields
 */
typedef struct {
	Boolean		parent_reset;
	Boolean		enterNotifyReceived;
	} RectButtonPart;

/*
 *  RectButton widget record declaration
 */
typedef struct _RectButtonRec {
	CorePart	core;
	PrimitivePart	primitive;
	ButtonPart	button;
	RectButtonPart	rect;
} RectButtonRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_RECTBUTTOP_H */
