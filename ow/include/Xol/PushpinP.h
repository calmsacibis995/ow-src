#ifndef	_XOL_PUSHPINP_H
#define	_XOL_PUSHPINP_H

#pragma	ident	"@(#)PushpinP.h	302.4	92/10/02 include/Xol SMI"	/* menu:include/Xol/PushpinP.h 1.13 	*/

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
 ************************************************************************
 *
 * Description:
 *		"Private" include file for the Pushpin Widget.
 *
 *****************************file*header********************************
 */


#include <Xol/PrimitiveP.h>	/* include superclasses's header */
#include <Xol/Pushpin.h>
#include <Xol/OlgxP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 ************************************************************************
 *
 * Define the Pushpin's Class Part and then the Class Record
 *
 ************************************************************************
 */

typedef struct _PushpinClass  {
	int makes_compiler_happy;			/* Not used	*/
} PushpinClassPart;

typedef struct _PushpinClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	PushpinClassPart	pushpin_class;
} PushpinClassRec;

		/* Declare the public hook to the Pushpin Class Record	*/
			
extern PushpinClassRec pushpinClassRec;

/*************************************************************************
 *
 * Define the widget instance structure for the pushpin
 *
 ************************************************************************/

typedef struct {
					/* Public Resources		*/

    XtCallbackList	in_callback;	/* Pinning callback list	*/
    XtCallbackList	out_callback;	/* UnPinning callback list	*/
    Boolean		is_default;	/* Is pushpin a default ??	*/

					/* Private Resources		*/

    ShellBehavior	shell_behavior;	/* behavior of pushpin's shell	*/
    Widget		preview_widget;	/* Widget to preview pushpin in	*/
    XtPointer		preview_cache;	/* private preview cache pointer*/
    OlgxAttrs  		*pAttrs;	/* Graphics attributes		*/
    Boolean		selected;	/* has Pushpin been selected ??	*/
    OlDefine		pin_state;	/* binary pin state: OL_IN or
					 * OL_OUT			*/
} PushpinPart;

				/* Full Record Declaration		*/

typedef struct _PushpinRec {
	CorePart	core;
	PrimitivePart	primitive;
	PushpinPart	pushpin;
} PushpinRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_PUSHPINP_H */
