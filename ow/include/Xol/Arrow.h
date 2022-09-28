#ifndef	_XOL_ARROW_H
#define	_XOL_ARROW_H

#pragma	ident	"@(#)Arrow.h	302.1	92/03/26 include/Xol SMI"	/* arrow:include/openlook/Arrow.h 1.6 	*/

/*
 *        Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                    All rights reserved.
 *          Notice of copyright on this source code 
 *          product does not indicate publication. 
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 *
 *    Sun Microsystems, Inc., 2550 Garcia Avenue,
 *    Mountain View, California 94043.
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
 *  Arrow.h - Public Definitions for Arrow widget (used by Scroll Widget)
 *
 */

/***************************************************************************
 *
 * Arrow Widget 
 *
 **************************************************************************/

/* Parameters:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 background	     Background		pixel		Black
 border		     BorderColor	pixel		Black
 borderWidth	     BorderWidth	int		0
 btnDown	     Callback		Pointer		NULL
 btnMotion	     Callback		Pointer		NULL
 btnUp		     Callback		Pointer		NULL
 destroyCallback     Callback		Pointer		NULL
 height		     Height		int		6
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 sensitive	     Sensitive		Boolean		True
 width		     Width		int		6
 x		     Position		int		0
 y		     Position		int		0

*/


#include <X11/Intrinsic.h>

#include <Xol/Primitive.h>		/* include superclasses' header */


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct {
	XEvent*		event; 		/* the event causing the ArrowAction */
	String*		params;		/* the TranslationTable params */
	Cardinal	num_params;	/* count of params */
}			ArrowCallDataRec, *ArrowCallData;

typedef struct _ArrowClassRec*	ArrowWidgetClass;
typedef struct _ArrowRec*	ArrowWidget;


extern WidgetClass		arrowWidgetClass;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_ARROW_H */
