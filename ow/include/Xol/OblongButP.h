#ifndef	_XOL_OBLONGBUTP_H
#define	_XOL_OBLONGBUTP_H

#pragma	ident	"@(#)OblongButP.h	302.1	92/03/26 include/Xol SMI"	/* button:include/openlook/OblongButP.h 1.12 	*/

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


#include <Xol/ButtonP.h>
#include <Xol/OblongButt.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 *  There are no new fields in the OblongButton class record 
 */
typedef struct _OblongButtonClass {
	int makes_compiler_happy;  /* not used */
} OblongButtonClassPart;

typedef struct _OblongButtonClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	ButtonClassPart		button_class;
	OblongButtonClassPart	oblongButton_class;
} OblongButtonClassRec;

typedef struct _OblongButtonGadgetClassRec {
        RectObjClassPart        rect_class;
        EventObjClassPart       event_class;
        ButtonClassPart         button_class;
        OblongButtonClassPart   oblongButton_class;
} OblongButtonGadgetClassRec;


/*
 *  oblongButtonClassRec is defined in OblongButton.c
 */
extern OblongButtonClassRec oblongButtonClassRec;
extern OblongButtonGadgetClassRec oblongButtonGadgetClassRec;

/*
 *  declaration of the Button widget fields
 */
typedef struct {
	/*
	 *  Public resources
	 */
	int	unused;
} OblongButtonPart;


typedef struct _OblongButtonRec {
	CorePart		core;
	PrimitivePart		primitive;
	ButtonPart		button;
	OblongButtonPart	oblong;
} OblongButtonRec;

typedef struct _OblongButtonGadgetRec {
        ObjectPart              object;
        RectObjPart             rect;
        EventObjPart            event;
        ButtonPart              button;
        OblongButtonPart        oblong;
} OblongButtonGadgetRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_OBLONGBUTP_H */
