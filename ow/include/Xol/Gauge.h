#ifndef	_XOL_GAUGE_H
#define	_XOL_GAUGE_H

#pragma	ident	"@(#)Gauge.h	302.1	92/03/26 include/Xol SMI"	/* slider:Gauge.h 1.1 	*/

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

/***********************************************************************
 *
 * Gauge Widget (subclass of PrimitiveClass)
 *
 ***********************************************************************/


#include <Xol/Primitive.h>	/* include superclasses' header */

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


/* Class record constants */

typedef struct _SliderClassRec*	GaugeWidgetClass;
typedef struct _SliderRec*	GaugeWidget;


extern WidgetClass		gaugeWidgetClass;


/* extern functions */
#if	defined(__STDC__) || defined(__cplusplus)
	extern void		OlSetGaugeValue(Widget w, int value);
#else	/* __STDC__ || __cplusplus */
	extern void		OlSetGaugeValue();
#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_GAUGE_H */
