#ifndef	_XOL_NONEXCLUSP_H
#define	_XOL_NONEXCLUSP_H

#pragma	ident	"@(#)NonexclusP.h	302.1	92/03/26 include/Xol SMI"	/* nonexclus:include/openlook/NonexclusP.h 1.18 	*/

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
/*	Copyright (c) 1989 AT&T	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <Xol/ManagerP.h>	/* include superclasses' header */
#include <Xol/Nonexclusi.h>


#ifdef	__cplusplus
extern "C" {
#endif


/* New fields for the Nonexclusives widget class record */

typedef struct _NonexclusivesClass 
  {
    int makes_compiler_happy;  /* not used */
  } NonexclusivesClassPart;

   /* Full class record declaration */
typedef struct _NonexclusivesClassRec {
    CoreClassPart	core_class;
    CompositeClassPart	composite_class;
    ConstraintClassPart	constraint_class;
    ManagerClassPart	manager_class;
    NonexclusivesClassPart	nonexclusives_class;
} NonexclusivesClassRec;

extern NonexclusivesClassRec nonexclusivesClassRec;

		    /* New fields for the Nonexclusives widget record: */
typedef struct {

		   				/* fields for resources */
    OlDefine		layout;			/* public */
    int			measure;
    Boolean		recompute_size;

    Boolean		is_default;		/* private */
    XtPointer		default_data;
    Widget		preview;	
    Boolean		trigger;	
    int			shell_behavior;
    XtCallbackList	postselect;

    int			reset_default;

					/* fields for internal management */ 
    int			class_children;
    Widget		default_child;
    Dimension		max_height;
    Dimension		max_width;
    Dimension		normal_height;
    Dimension		normal_width;

} NonexclusivesPart;

/* Full widget declaration */
typedef struct _NonexclusivesRec {
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    ManagerPart		manager;
    NonexclusivesPart	nonexclusives;
} NonexclusivesRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_NONEXCLUSP_H */
