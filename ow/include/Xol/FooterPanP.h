#ifndef	_XOL_FOOTERPANP_H
#define	_XOL_FOOTERPANP_H

#pragma	ident	"@(#)FooterPanP.h	302.1	92/03/26 include/Xol SMI"	/* panel:include/Xol/FooterPanP.h 1.4 	*/

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
 * FooterPanP.h - Private definitions for FooterPanel widget
 */


#include <Xol/ManagerP.h>		/* include supclasses' header */
#include <Xol/FooterPane.h>		/* include public header */


#ifdef	__cplusplus
extern "C" {
#endif


/***********************************************************************
 *
 * Class structure
 *
 */

/* New fields for the FooterPanel widget class record */
typedef struct {
	int no_class_fields;		/* make compiler happy */
} FooterPanelClassPart;

/* Full class record declaration */
typedef struct _FooterPanelClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	FooterPanelClassPart	footer_panel_class;
} FooterPanelClassRec;

extern FooterPanelClassRec footerPanelClassRec;


/***********************************************************************
 *
 * Instance (widget) structure
 *
 */

/* New fields for the FooterPanel widget record */
typedef struct {
	int no_class_fields;		/* make compiler happy */
} FooterPanelPart;


/* Full instance record declaration */
typedef struct _FooterPanelRec {
	CorePart	core;
	CompositePart	composite;
	ConstraintPart	constraint;
	ManagerPart	manager;
	FooterPanelPart	footer_panel;
} FooterPanelRec;

/***********************************************************************
 *
 * Constants / Macros
 */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_FOOTERPANP_H */
