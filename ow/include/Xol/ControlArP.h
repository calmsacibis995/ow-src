#ifndef	_XOL_CONTROLARP_H
#define	_XOL_CONTROLARP_H

#pragma	ident	"@(#)ControlArP.h	302.3	92/06/12 include/Xol SMI"	/* control:include/openlook/ControlArP.h 1.16 	*/

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
 **   Copyright (c) 1988 by the Massachusetts Institute of Technology
 **/


#include	<Xol/ManagerP.h>	/* include superclasses' header */
#include	<Xol/ControlAre.h>
#include	<Xol/ChangeBar.h>


#ifdef	__cplusplus
extern "C" {
#endif


#define XtInheritDisplayLayoutList ((OlCrlAreDispLayoutLstProc) _XtInherit)

/*  Internally used layout structure  */

typedef struct _OlCLayoutList
{
   Widget	widget;
   Dimension	border;
   Position	x, y;
   Dimension	width, height;
} OlCLayoutList;

typedef void (*OlCrlAreDispLayoutLstProc)(
        ControlAreaWidget,
        OlCLayoutList **,
        int,
        Dimension *,
        Dimension *
);


/*  New fields for the Control Area widget class record  */

typedef struct
{
	OlCrlAreDispLayoutLstProc	display_layout_list;	
			/* display a list of widgets */
} ControlClassPart;


/* Full class record declaration */

typedef struct _ControlClassRec
{
   CoreClassPart	core_class;
   CompositeClassPart	composite_class;
   ConstraintClassPart	constraint_class;
   ManagerClassPart	manager_class;
   ControlClassPart	control_class;
} ControlClassRec;

extern ControlClassRec controlClassRec;

/* New fields for the Control Panel widget record */

typedef struct
{
   Dimension		h_space, v_space;
   Dimension		h_pad, v_pad;
   int			measure;
   OlDefine		layouttype;
   OlDefine		same_size;
   Boolean		align_captions;
   Boolean		center;
   XtCallbackList       post_select;
   Boolean		allow_change_bars;
   Cardinal		ncolumns;
   Position *		col_x;
   ChangeBar *		cb;
} ControlPart;


/*
** Full instance record declaration
*/

typedef struct _ControlRec
{
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    ManagerPart		manager;
    ControlPart		control;
} ControlRec;

/*
 * Constraint record:
 */
typedef struct	_ControlAreaConstraintRec {
	/*
	 * Public:
	 */
	OlDefine		change_bar;

	/*
	 * Private:
	 */
	Position		col;
}			ControlAreaConstraintRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_CONTROLARP_H */
