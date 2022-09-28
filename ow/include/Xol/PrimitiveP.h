#ifndef	_XOL_PRIMITIVEP_H
#define	_XOL_PRIMITIVEP_H

#pragma	ident	"@(#)PrimitiveP.h	302.6	93/01/20 include/Xol SMI"	/* primitive:PrimitiveP.h 1.11 	*/

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


#include <Xol/OpenLookP.h>
#include <Xol/Primitive.h>
#include <Xol/VendorI.h>

#include <X11/CoreP.h>

#ifdef	__cplusplus
extern "C" {
#endif


/***********************************************************************
 *
 * Class record
 *
 ***********************************************************************/

/* New fields for the PrimitiveWidget class record */
typedef struct {
	XtPointer		reserved1;	/* obsolete can_accept_focus */
	OlHighlightProc		highlight_handler;
	OlTraversalFunc		traversal_handler;
	OlRegisterFocusFunc	register_focus;
	OlActivateFunc		activate;
	OlEventHandlerList	event_procs;
	Cardinal		num_event_procs;
	XtVersionType		version;
	XtPointer		extension;
	_OlDynData		dyn_data;
	OlTransparentProc	transparent_proc;
	SuperCaretQueryLocnProc query_sc_locn_proc;
} PrimitiveClassPart;

/* Full class record declaration */
typedef struct _PrimitiveClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
} PrimitiveClassRec;

extern PrimitiveClassRec	primitiveClassRec;


/***********************************************************************
 *
 * Instance record
 *
 ***********************************************************************/

/* New fields for the PrimitiveWidget record */
typedef struct _PrimitivePart {
	/* Resource-related data */
	XtPointer		user_data;
	Widget			reference_widget;
	String			accelerator;
	String			qualifier_text;
	Boolean			meta_key;
	String			accelerator_text;
	String			reference_name;
	XtCallbackList		consume_event;
	OlStrRep		text_format;
	OlFont			font;
	Pixel			input_focus_color;
	Pixel			font_color;
	Pixel			foreground;
	OlMnemonic		mnemonic;
	Boolean			traversal_on;
        int			scale;

	/* Non-resource-related data */
	Boolean			has_focus;
	unsigned char		dyn_flags;

	XtCallbackList		unrealize_callbacks;
	OlVendorPartExtension	ext_part;
	OlDefine		input_focus_feedback;
} PrimitivePart;


/* Full instance record declaration */
typedef struct _PrimitiveRec {
	CorePart		core;
	PrimitivePart		primitive;
} PrimitiveRec;


/***********************************************************************
 *
 * Constants
 *
 ***********************************************************************/

#define	_OlIsPrimitive(w)	(XtIsSubclass((w), primitiveWidgetClass))
#define	_OL_IS_PRIMITIVE	_OlIsPrimitive

/* dynamic resources bit masks */
#define OL_B_PRIMITIVE_BG		(1 << 0)
#define OL_B_PRIMITIVE_FG		(1 << 1)
#define OL_B_PRIMITIVE_FONTCOLOR	(1 << 2)
#define OL_B_PRIMITIVE_FOCUSCOLOR	(1 << 3)
#define OL_B_PRIMITIVE_BORDERCOLOR	(1 << 4)


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_PRIMITIVEP_H */
