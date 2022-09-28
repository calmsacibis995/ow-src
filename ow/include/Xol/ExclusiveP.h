#ifndef	_XOL_EXCLUSIVEP_H
#define	_XOL_EXCLUSIVEP_H

#pragma	ident	"@(#)ExclusiveP.h	302.1	92/03/26 include/Xol SMI"	/* exclusives:include/openlook/ExclusiveP.h 1.19 	*/

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
 * File:	ExclusivesP.h - Private definitions for Exclusives widget
 *
 *	Copyright (c) 1989 AT&T
 */


#include <Xol/Exclusives.h>
#include <Xol/ManagerP.h>	/* include superclasses' header */


#ifdef	__cplusplus
extern "C" {
#endif


/* New fields for the Exclusives widget class record */

typedef struct _ExclusivesClass 
  {
    int makes_compiler_happy;  /* not used */
  } ExclusivesClassPart;

   /* Full class record declaration */

typedef struct _ExclusivesClassRec {
    CoreClassPart	core_class;
    CompositeClassPart	composite_class;
    ConstraintClassPart	constraint_class;
    ManagerClassPart	manager_class;
    ExclusivesClassPart	exclusives_class;
} ExclusivesClassRec;

extern ExclusivesClassRec exclusivesClassRec;

		    /* New fields for the Exclusives widget record: */
typedef struct {

		   				/* fields for resources */
    OlDefine		layout;			/* public */
    int			measure;
    Boolean		noneset;
    Boolean		recompute_size;

    Boolean		is_default;		/* fields for resources */
    XtPointer		default_data;		/* private */
    Widget		preview;	
    Boolean		trigger;	
    int			shell_behavior;	
    int			reset_set;
    int			reset_default;
    XtCallbackList	postselect;
					/* fields for internal management */ 
    Widget		default_child;
    Widget		set_child;
    Widget		looks_set;
    int			usr_set;
    Dimension		max_height;
    Dimension		max_width;
    Dimension		normal_height;
    Dimension		normal_width;

} ExclusivesPart;

   /* Full widget declaration */

typedef struct _ExclusivesRec {
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    ManagerPart		manager;
    ExclusivesPart	exclusives;
} ExclusivesRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_EXCLUSIVEP_H */
