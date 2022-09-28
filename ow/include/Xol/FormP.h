#ifndef _XOL_FORMP_H
#define	_XOL_FORMP_H

#pragma	ident	"@(#)FormP.h	302.1	92/03/26 include/Xol SMI"	/* form:include/openlook/FormP.h 1.12	*/

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

/**
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **/


#include <Xol/Form.h>
#include <Xol/ManagerP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*  Form constraint rec  */

typedef struct _FormConstraintRec {
	String          x_ref_name;	/* the name of the widget to reference */
	Widget          x_ref_widget;	/* the widget to reference */
	Dimension       x_offset;	/* the offset (pixels) from the reference */
	Boolean         x_add_width;	/* add width of the reference to x coord   */
	Boolean         x_vary_offset;	/* able to vary the seperation */
	Boolean         x_resizable;	/* able to resize in x direction */
	Boolean         x_attach_right;	/* attached to the right edge of the form */
	Dimension       x_attach_offset;/* offset (pixels) from attached edge */

	String          y_ref_name;	/* y constraints are the same as x */
	Widget          y_ref_widget;
	Dimension       y_offset;
	Boolean         y_add_height;
	Boolean         y_vary_offset;
	Boolean         y_resizable;
	Boolean         y_attach_bottom;
	Dimension       y_attach_offset;

	Boolean         managed;	/* whether the widget is managed or not */
	Position        x,
	                y;		/* location after constraint processing */
	Dimension       width,
	                height;		/* size after constraint processing */

	Position        set_x;		/* original y */
	Position        set_y;		/* original x */
	Dimension       set_width;	/* original or set values width of widget */
	Dimension       set_height;	/* original or set values height of widget */

	Dimension       width_when_unmanaged;
	Dimension       height_when_unmanaged;
}               FormConstraintRec;
      

/*  Form class structure  */

typedef struct _FormClassPart {
	int             foo;		/* No new fields needed */
}               FormClassPart;


/*  Full class record declaration for Form class  */

typedef struct _FormClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	FormClassPart		form_class;
}               FormClassRec;

extern FormClassRec formClassRec;


/*  The Form tree structure used for containing the constraint tree  */
  
struct _FormRef {
	Widget          c_this;		/* the widget these constaints are for */
	Widget          ref;		/* the widget this constraint is for */
	Dimension       offset;		/* offset (pixels) from parent ref */
	Boolean         add;		/* add (width or height) of parent ref */
	Boolean         vary;		/* able to vary the seperation */
	Boolean         resizable;	/* able to resize */
	Boolean         attach;		/* attached to the edge (right or bottom) */
	Dimension       attach_offset;	/* offset (pixels) from attached edge */

	Position        set_loc;	/* the initial or set value location */
	Dimension       set_size;	/* the initial or set value size */

	struct _FormRef **ref_to;	/* child references */
	int             ref_to_count;	/* number of child references */
};

typedef struct _FormRef FormRef;

typedef struct _FormProcess {
	FormRef        *ref;
	Position        loc;
	Dimension       size;
	Boolean         leaf;
}               FormProcess;


/*  The Form instance record  */

typedef struct _FormPart {
	FormRef        *width_tree;
	FormRef        *height_tree;
	long            save_border;
				/* 
				 * a flag to be used in constraint setvalues
				 * it is actually a border pixel value to be
				 * used by the intrinsics and set in
				 * cons. setvals
				 */
}               FormPart;


/*  Full instance record declaration  */

typedef struct _FormRec {
	CorePart        core;
	CompositePart   composite;
	ConstraintPart  constraint;
	ManagerPart     manager;
	FormPart        form;
}               FormRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_FORMP_H */
