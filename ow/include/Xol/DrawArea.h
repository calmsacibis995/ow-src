#ifndef	_XOL_DRAWAREA_H
#define	_XOL_DRAWAREA_H

#pragma	ident	"@(#)DrawArea.h	302.3	92/10/06 include/Xol SMI"	/* OLIT	*/

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


#include <Xol/BulletinBo.h>	/* include superclasses' header */
#include <Xol/OpenLook.h>

#include <X11/Intrinsic.h>
#include <X11/Shell.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct _DrawAreaClassRec*	DrawAreaWidgetClass;
typedef struct _DrawAreaRec*		DrawAreaWidget;


extern WidgetClass	drawAreaWidgetClass;

/*
 *  DrawArea Callback Structure
 */
typedef struct {
	int		reason;
	XEvent*		event;
	Position	x;
	Position	y;
	Dimension	width;
	Dimension	height;
	Region		region;
} OlDrawAreaCallbackStruct;

#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_DRAWAREA_H */
