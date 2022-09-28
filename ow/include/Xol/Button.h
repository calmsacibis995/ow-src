#ifndef	_XOL_BUTTON_H
#define	_XOL_BUTTON_H

#pragma	ident	"@(#)Button.h	302.1	92/03/26 include/Xol SMI"	/* button:include/openlook/Button.h 1.15 	*/

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

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <Xol/EventObj.h>	/* include gadget's superclass header */
#include <Xol/Primitive.h>	/* include widget's superclass header */


#ifdef	__cplusplus
extern "C" {
#endif


/*
 *  These defines are for the scale resource.
 */
#define SMALL_SCALE		10
#define MEDIUM_SCALE		12
#define LARGE_SCALE		14
#define EXTRA_LARGE_SCALE	19

/*
 *  buttonWidgetClass is defined in Button.c
 */
typedef struct _ButtonClassRec*		ButtonWidgetClass;
typedef struct _ButtonRec*		ButtonWidget;

/*
 *  buttonGadgetClass is defined in Button.c
 */
typedef struct _ButtonGadgetClassRec*	ButtonGadgetClass;
typedef struct _ButtonGadgetRec *	ButtonGadget;


extern WidgetClass			buttonWidgetClass;
extern WidgetClass			buttonGadgetClass;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_BUTTON_H */
