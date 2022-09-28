#ifndef	_XOL_ABBREVMENP_H
#define	_XOL_ABBREVMENP_H

#pragma	ident	"@(#)AbbrevMenP.h	302.4	92/10/02 include/Xol SMI"	/* abbrevstack:AbbrevMenP.h 1.5	*/

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

/**************************************************************************
 *
 *	Private ".h" file for the AbbreviatedMenuButton Widget
 *
 **************************************************************************/


#include <Xol/AbbrevMenu.h>
#include <Xol/OlgxP.h>
#include <Xol/PrimitiveP.h>	/* include superclasses's header */

#include <X11/CoreP.h>
#include <X11/IntrinsicP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/**************************************************************************
 *
 * Widget Private Data
 *
 **************************************************************************/

/* New fields for the widget class record */
typedef struct _AbbrevMenuButtonClass {
    int makes_compiler_happy;		/* not used */
} AbbrevMenuButtonClassPart;

/* Full class record declaration  */
typedef struct _AbbrevMenuButtonClassRec {
    CoreClassPart		core_class;
    PrimitiveClassPart		primitive_class;
    AbbrevMenuButtonClassPart	menubutton_class;
} AbbrevMenuButtonClassRec;

extern AbbrevMenuButtonClassRec abbrevMenuButtonClassRec;


/**************************************************************************
 *
 * Widget Instance Data
 *
 **************************************************************************/

/* New fields for the widget record */
typedef struct _AbbrevMenuButtonPart {
	/* Public data */
	Boolean			busy;
	Boolean			set;
	Widget			preview_widget;	/* Id where abbrev. does
						   its its previewing. */

	/* Private data	*/
	ShellBehavior		shell_behavior;	/* Behavior of Abbrev.'s
						  shell */
	Widget			submenu;	/* Submenu for AbbrevMenuButton
						 */
	Boolean			previewing_default;
	OlgxAttrs*		pAttrs;		/* GCs and what not */
	XtCallbackList		post_select;
	Position		root_x;		/* Temporary cache */
	Position		root_y;		/* Temporary cache */
	String			current_item_label;
} AbbrevMenuButtonPart;

/* Full widget declaration */
typedef struct _AbbrevMenuButtonRec {
	CorePart		core;
	PrimitivePart		primitive;
	AbbrevMenuButtonPart	menubutton;
} AbbrevMenuButtonRec;


#ifdef	__cplusplus
}
#endif


#endif /* _XOL_ABBREVMENP_H */
