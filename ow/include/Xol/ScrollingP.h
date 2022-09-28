#ifndef	_XOL_SCROLLINGP_H
#define	_XOL_SCROLLINGP_H

#pragma	ident	"@(#)ScrollingP.h	302.2	92/10/05 include/Xol SMI"	/* scrolllist:include/Xol/ListP.h 1.24 	*/

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


#include <Xol/ScrollingL.h>	/* include my public header */
#include <Xol/FormP.h>		/* include superclass' header */


#ifdef	__cplusplus
extern "C" {
#endif


/***********************************************************************
 *
 *	Constants / Macros
 */

#define _OlListPane(w)	( ((CompositeWidget)(w))->composite.children[1] )
#define _OlListSBar(w)	( ((CompositeWidget)(w))->composite.children[0] )

/* dynamic resource bit masks */
#define OL_B_LIST_FG		(1 << 0)
#define OL_B_LIST_FONTCOLOR	(1 << 1)

/***********************************************************************
 *
 *	External Functions
 */


/***********************************************************************
 *
 *	Class structure
 */
/* New fields for the List widget class record */
typedef struct {
    int no_class_fields;	/* make compiler happy */
} ListClassPart;

/* Full class record declaration */
typedef struct _ListClassRec {
    CoreClassPart	core_class;
    CompositeClassPart 	composite_class;
    ConstraintClassPart	constraint_class;
    ManagerClassPart	manager_class;
    FormClassPart	form_class;
    ListClassPart	list_class;
} ListClassRec;

/* Class record variable */
externalref ListClassRec listClassRec;
extern WidgetClass listWidgetClass;

/***********************************************************************
 *
 *	Instance (widget) structure
 */

/* New fields for the List widget record */
typedef struct { 
    XtCallbackList	userDeleteItems;
    XtCallbackList	userMakeCurrent;
    XtCallbackList	itemCurrentCallback;
    XtCallbackList	itemNotCurrentCallback;
    XtCallbackList	multiClickCallback;
    Widget		list_pane;
    unsigned char	dyn_flags;
    OlStrRep		text_format;
} ListPart;

/* Full instance record declaration */
typedef struct _ListRec {
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    ManagerPart		manager;
    FormPart		form;
    ListPart		list;
} ListRec;

/* constraint record */
typedef struct {
    int	no_fields;
} ListConstraintPart;

typedef struct _ListConstraintRec {
    FormConstraintRec	form;
    ListConstraintPart	list;
} ListConstraintRec, *ListConstraint;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_SCROLLINGP_H */
