#ifndef	_XOL_MENUBUTTOP_H
#define	_XOL_MENUBUTTOP_H

#pragma	ident	"@(#)MenuButtoP.h	302.1	92/03/26 include/Xol SMI"	/* buttonstack:MenuButtoP.h 1.3 	*/

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

/*************************************************************************
 *
 * Description:
 *		Private ".h" file for the MenuButton Widget and Gadget
 *
 ***************************file*header***********************************/


#include <Xol/ButtonP.h>
#include <Xol/MenuButton.h>


#ifdef	__cplusplus
extern "C" {
#endif


/***********************************************************************
 *
 * MenuButton Widget Private Data
 *
 ***********************************************************************/


		/* New fields for the MenuButton widget class record	*/

typedef struct _MenuButtonClass {
    int makes_compiler_happy;  /* not used */
} MenuButtonClassPart;

				/* Full class record declaration 	*/

typedef struct _MenuButtonClassRec {
    CoreClassPart		core_class;
    PrimitiveClassPart		primitive_class;
    ButtonClassPart		button_class;
    MenuButtonClassPart		menubutton_class;
} MenuButtonClassRec;

typedef struct _MenuButtonGadgetClassRec {
    RectObjClassPart		rect_class;
    EventObjClassPart		event_class;
    ButtonClassPart		button_class;
    MenuButtonClassPart		menubutton_class;
} MenuButtonGadgetClassRec;

extern MenuButtonClassRec	menuButtonClassRec;
extern MenuButtonGadgetClassRec	menuButtonGadgetClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

			/* New fields for the MenuButton widget record */
typedef struct {
					/* Private data			*/

    Boolean             previewing_default; /* Is this widget previewing
					   its menu's default		*/
    Widget              preview_widget;	/* Widget previewing this one	*/
    Widget     		submenu;	/* Submenu for MenuButton	*/
    Position		root_x;		/* Temporary cache		*/
    Position		root_y;		/* Temporary cache		*/
} MenuButtonPart;


					/* Full widget declaration	*/
typedef struct _MenuButtonRec {
	CorePart	core;
	PrimitivePart	primitive;
	ButtonPart	button;
	MenuButtonPart	menubutton;
} MenuButtonRec;

					/* Full gadget declaration	*/
typedef struct _MenuButtonGadgetRec {
    ObjectPart		object;
    RectObjPart		rect;
    EventObjPart	event;
    ButtonPart		button;
    MenuButtonPart	menubutton;
} MenuButtonGadgetRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_MENUBUTTOP_H */
