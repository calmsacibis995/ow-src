#ifndef	_XOL_NOTICEP_H
#define	_XOL_NOTICEP_H

#pragma	ident	"@(#)NoticeP.h	302.2	92/04/09 include/Xol SMI"	/* notice:include/openlook/NoticeP.h 1.14 	*/

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
 * NoticeP.h - Private definitions for Notice widget
 */


#include <Xol/Notice.h>

#include <X11/ShellP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/***********************************************************************
 *
 *	Class structure
 *
 **********************************************************************/

/* New fields for the Notice widget class record */
typedef struct {
    int no_class_fields;	/* make compiler happy */
} NoticeShellClassPart;

/* Full class record declaration */
typedef struct _NoticeShellClassRec {
    CoreClassPart		core_class;
    CompositeClassPart  	composite_class;
    ShellClassPart		shell_class;
    WMShellClassPart            wm_shell_class;
    VendorShellClassPart        vendor_shell_class;
    TransientShellClassPart	transient_shell_class;
    NoticeShellClassPart	notice_shell_class;
} NoticeShellClassRec;

/* Class record variable */
externalref NoticeShellClassRec noticeShellClassRec;

/***********************************************************************
 *
 *	Instance (widget) structure
 *
 **********************************************************************/

/* New fields for the Notice widget record */
typedef struct {
    /* "public" (resource) members */
    Widget	control;	/* control box for buttons */
    Widget	emanate;	/* originating widget */
    Widget	text;		/* notice text widget */
    Boolean	warp_pointer;
    OlStrRep	text_format;

    /* private members */
    Boolean	do_unwarp;	/* unwarp only when no motion */
    int		root_x;		/* to restore warped pointer */
    int		root_y;		/* to restore warped pointer */
} NoticeShellPart;

/* Full instance record declaration */
typedef struct _NoticeShellRec {
    CorePart		core;
    CompositePart	composite;
    ShellPart		shell;
    WMShellPart         wm;
    VendorShellPart     vendor;
    TransientShellPart	transient;
    NoticeShellPart	notice_shell;
} NoticeShellRec;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_NOTICEP_H */
