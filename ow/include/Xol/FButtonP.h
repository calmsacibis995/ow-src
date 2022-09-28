#ifndef	_XOL_FBUTTONP_H
#define	_XOL_FBUTTONP_H

#pragma	ident	"@(#)FButtonP.h	302.1	92/03/26 include/Xol SMI"	/* flat:FButtonP.h 1.2 	*/

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
 * Description:
 *	This is the flat compound button container's private header file.
 ************************************************************************	
 */


#include <Xol/FButton.h>
#include <Xol/FlatP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 ************************************************************************	
 * Define Widget Class Part and Class Rec
 ************************************************************************	
 */

				/* Define new fields for the class part	*/

typedef struct {
    int		empty;
} FlatButtonClassPart;

				/* Full class record declaration 	*/

typedef struct _FlatButtonClassRec {
    CoreClassPart	core_class;
    PrimitiveClassPart	primitive_class;
    FlatClassPart	flat_class;
    FlatButtonClassPart	button_class;
} FlatButtonClassRec;

				/* External class record declaration	*/

extern FlatButtonClassRec	flatButtonClassRec;

/*
 ************************************************************************	
 * Define Widget Instance Structure
 ************************************************************************	
 */

				/* Define Expanded sub-object instance	*/

typedef struct {
	Boolean		set;		/* is this item set ?		*/
	Boolean		busy;		/* is this item busy ?		*/
	OlDefine	button_type;	/* type of button		*/
	XtPointer	client_data;	/* for callbacks		*/
	XtCallbackProc	select_proc;	/* select callback		*/
	Widget		menu;		/* id of submenu		*/
} FlatButtonItemPart;

			/* Item's Full Instance record declaration	*/

typedef struct {
	FlatItemPart		flat;
	FlatButtonItemPart	button;
} FlatButtonItemRec, *FlatButtonItem;

			/* Define new fields for the instance part	*/

typedef struct {
	Boolean		menu_descendant; /* is the shell a menu ?	*/
	Cardinal	default_item;	/* the default item		*/
	Cardinal	current_item;	/* the currect item		*/
	Cardinal	preview_item;	/* preview widget sub-object	*/
	Widget		preview;	/* widget to preview in		*/
	XtCallbackList	post_select;	/* call after selection		*/
	FlatButtonItemPart item_part;/* sub-object data for this part*/
} FlatButtonPart;

				/* Full instance record declaration	*/

typedef struct _FlatButtonRec {
    CorePart		core;
    PrimitivePart	primitive;
    FlatPart		flat;
    FlatButtonPart	button;
} FlatButtonRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_FBUTTONP_H */
