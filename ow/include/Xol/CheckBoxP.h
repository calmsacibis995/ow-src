#ifndef	_XOL_CHECKBOXP_H
#define	_XOL_CHECKBOXP_H

#pragma	ident	"@(#)CheckBoxP.h	302.3	92/09/29 include/Xol SMI"	/* checkbox:include/Xol/CheckBoxP.h 1.22 	*/

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
 ************************************************************
 *
 * File: CheckBoxP.h - Private definitions for CheckBox widget
 * 
 ************************************************************
 */


#include <Xol/ManagerP.h>
#include <Xol/ButtonP.h>
#include <Xol/CheckBox.h>
#include <Xol/OlgxP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/* New fields for the CheckBox widget class record */

typedef struct _CheckBoxClass 
  {
    int makes_compiler_happy;  /* not used */
  } CheckBoxClassPart;

   /* Full class record declaration */
typedef struct _CheckBoxClassRec {
    CoreClassPart	core_class;
    CompositeClassPart	composite_class;
    ConstraintClassPart	constraint_class;
    ManagerClassPart	manager_class;
    CheckBoxClassPart	checkBox_class;
} CheckBoxClassRec;

extern CheckBoxClassRec checkBoxClassRec;

		    /* New fields for the CheckBox widget record: */
typedef struct {

		   				/* fields for resources */

						/* public */
    XtCallbackList	select;
    XtCallbackList	unselect;
    Boolean		set;
    Boolean		dim;
    Boolean		setvalue;
    OlStr		label;
    Pixel		foreground;
    Pixel		fontcolor;
    OlDefine		labeljustify;
    OlDefine		position;
    OlDefine		labeltype;
    Boolean		labeltile;
    XImage	       *labelimage;
    Boolean		recompute_size;
    OlFont	        font;
    OlStrRep	        text_format;
    OlMnemonic		mnemonic;
    String		accelerator;
    String		accelerator_text;

					/* private fields */ 
    Widget		label_child;
    Dimension		normal_height;
    Dimension		normal_width;
    Position		x1,x2;		/* position of checkbox box */
    Position		y1,y2;		
    unsigned char	dyn_flags;
    OlgxAttrs		*pAttrs;
    int			scale;
    /* preview_state: private resource to  implement the recessed 
     * look when SELECT butt is pressed  over checkbox
     */
    Boolean		preview_state; 	
} CheckBoxPart;

   /* Full widget declaration */
typedef struct _CheckBoxRec {
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    ManagerPart		manager;
    CheckBoxPart	checkBox;
} CheckBoxRec;


/* dynamics resource bit masks */
#define OL_B_CHECKBOX_FG		(1 << 0)
#define OL_B_CHECKBOX_FONTCOLOR		(1 << 1)


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_CHECKBOXP_H */




