#ifndef	_XOL_CONTROLARE_H
#define	_XOL_CONTROLARE_H

#pragma	ident	"@(#)ControlAre.h	302.1	92/03/26 include/Xol SMI"	/* control:include/openlook/ControlAre.h 1.13 	*/

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

/*
 *	Layouttype is one of OL_FIXEDCOLS, OL_FIXEDROWS, OL_FIXEDWIDTH,
 *	OL_FIXEDHEIGHT.
 *	measure is number of rows or columns for the first two cases,
 *	height or width for the other two cases.

Name		Type		Default		Meaning
---		----		-------		-------
XtNmeasure	XtRInt		1		maximum height/width/rows/cols
XtNlayoutType	ControlLayout	OL_FIXEDROWS	Type of layout
XtNhSpace	Dimension	4		interwidget horizonal spacing
XtNvSpace	Dimension	4		interwidget vertical spacing
XtNhPad		Dimension	4		horizonal padding at edges
XtNvPad		Dimension	4		vertical padding at edges
XtNsameSize	OlSameSize	OL_NONE		Force children to be same size?
XtNalignCaptions	Boolean	FALSE		Align captions?
XtNcenter	Boolean		FALSE		Center widgets in each column?
XtNpostSelect	XtCallbackList	NULL		Callbacks for children

*/


#include <Xol/Manager.h>	/* include superclasses' header */
#include <Xol/ChangeBar.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct _ControlClassRec *ControlAreaWidgetClass;
typedef struct _ControlRec      *ControlAreaWidget;
typedef struct _ControlAreaConstraintRec *	ControlAreaConstraints;

extern WidgetClass controlAreaWidgetClass;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_CONTROLARE_H */
