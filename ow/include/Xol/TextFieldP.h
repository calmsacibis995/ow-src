#ifndef	_XOL_TEXTFIELDP_H
#define	_XOL_TEXTFIELDP_H

#pragma	ident	"@(#)TextFieldP.h	302.11	93/02/09 include/Xol SMI"	/* textfield:include/Xol/TextFieldP.h 1.16	*/

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


#include <Xol/ManagerP.h>        /* include superclasses' header */
#include <Xol/OlgxP.h>
#include <Xol/TextField.h>

#include <X11/Intrinsic.h>
#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/ConstrainP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/* dynamic resource bit masks */
#define OL_B_TEXTFIELD_BG	(1 << 0)
#define OL_B_TEXTFIELD_FG	(1 << 1)
#define OL_B_TEXTFIELD_FC       (1 << 2)

typedef enum { POLL_LEFT, POLL_RIGHT, POLL_OFF } PollMode;

typedef struct {
	int keep_compiler_happy;   /* No new procedures */
} TextFieldClassPart;

typedef struct _TextFieldClassRec {
	CoreClassPart         core_class;
	CompositeClassPart    composite_class;
	ConstraintClassPart   constraint_class;
	ManagerClassPart      manager_class;
	TextFieldClassPart    textfield_class;
} TextFieldClassRec;

typedef struct {
	char *          string;
	Widget          textWidget;
	int             initialDelay;
	int             repeatRate;
	int             maximumSize;
	int             charsVisible;
	XtCallbackList  verification;
	Dimension       arrowWidth;
	Dimension       arrowHeight;
	OlFont		font;
	OlStrRep	text_format;
	char            l_on;
	char            r_on;
	unsigned char	dyn_flags;
	Boolean		insertTab;
	PollMode        polling;
	OlgxAttrs	*pAttrs;	/* drawing attributes */
	Pixel		foreground;
	Pixel           font_color;
	Pixel		input_focus_color; 
	int             scale;
	OlImPreeditStyle	pre_edit_style;
	OlEditMode	editMode;
} TextFieldPart;

typedef struct _TextFieldRec {
	CorePart        core;
	CompositePart   composite;
	ConstraintPart  constraint;
	ManagerPart     manager;
	TextFieldPart   textField;
} TextFieldRec;

extern TextFieldClassRec textFieldClassRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_TEXTFIELDP_H */
