#ifndef	_XOL_CAPTION_H
#define	_XOL_CAPTION_H

#pragma	ident	"@(#)Caption.h	302.1	92/03/26 include/Xol SMI"	/* caption:include/openlook/Caption.h 1.9 	*/

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

/*
 *
 * Name		Type		Default		Meaning
 * ---		----		-------		-------
 * XtNforeground	XtRPixel	Black		Foreground pixel
 * XtNfont		XtRFontStruct	Fixed		Font for caption
 * XtNlabel	XtRString	NULL		text of caption
 * XtNposition	int		OL_LEFT		Where caption goes
 * XtNalignment	int		OL_CENTER 	Caption position fine-tuning
 * XtNcaptionWidth	Dimension	none		Width of caption
 * XtNspace	Dimension	4		spacing
 */


#include <Xol/Manager.h>	/* include superclasses' header */

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


/* Class record constants */

typedef struct _CaptionClassRec*	CaptionWidgetClass;
typedef struct _CaptionRec*		CaptionWidget;


extern WidgetClass 			captionWidgetClass;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_CAPTION_H */
