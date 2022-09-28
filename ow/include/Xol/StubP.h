#ifndef	_XOL_STUBP_H
#define	_XOL_STUBP_H

#pragma	ident	"@(#)StubP.h	302.2	93/01/20 include/Xol SMI"	/* stub:include/Xol/StubP.h 1.9 	*/

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
 *
 * Description:
 *		"Private" include file for the Stub Widget.
 *
 *****************************file*header********************************
 */


#include <Xol/PrimitiveP.h>	/* include superclasses's header */
#include <Xol/OpenLookP.h>
#include <Xol/Stub.h>

#include <X11/Intrinsic.h>
#include <X11/CoreP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 ************************************************************************
 *
 * Define the Stub's Class Part and then the Class Record
 *
 ************************************************************************
 */

typedef struct _StubClass  {
	int makes_compiler_happy;			/* Not used	*/
} StubClassPart;

typedef struct _StubClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	StubClassPart		stub_class;
} StubClassRec;

		/* Declare the public hook to the Stub Class Record	*/
			
extern StubClassRec stubClassRec;

/*
 ************************************************************************
 *
 * Define the widget instance structure for the stub
 *
 ************************************************************************
 */

typedef struct {
    Window		window;			/* Initial window	*/
    Widget		reference_stub;		/* to inherit from	*/
    XtInitProc		initialize;		/* initialize instance	*/
    XtArgsProc		initialize_hook;	/* initialize subdata	*/
    XtRealizeProc	realize;		/* realize instance	*/
    XtWidgetProc	destroy;		/* destroy instance	*/
    XtWidgetProc	resize;			/* resize instance contents*/
    XtExposeProc	expose;			/* rediplay window	*/
    XtSetValuesFunc	set_values;		/* set instance resources*/
    XtArgsFunc		set_values_hook;	/* set subdata resources*/
    XtAlmostProc	set_values_almost;	/* set values almost geo.
						 * reply handler	*/
    XtArgsProc		get_values_hook;	/* get subdata resources*/
    XtGeometryHandler	query_geometry;		/* perferred geometry	*/

    XtAcceptFocusProc	accept_focus;		/* accept_focus 	*/
    OlActivateFunc	activate;		/* activate function    */
    OlHighlightProc	highlight;		/* highlight function   */
    OlRegisterFocusFunc	register_focus;		/* register_focus func. */
    OlTraversalFunc	traversal_handler;	/* traversal handler    */
    SuperCaretQueryLocnProc	query_sc_locn_proc;	/* query_sc_locn_proc   */
} StubPart;

				/* Full Record Declaration		*/

typedef struct _StubRec {
	CorePart	core;
	PrimitivePart	primitive;
	StubPart	stub;
} StubRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_STUBP_H */
