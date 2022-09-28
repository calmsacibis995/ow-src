#ifndef	_XOL_MENUBUTTON_H
#define	_XOL_MENUBUTTON_H

#pragma	ident	"@(#)MenuButton.h	302.1	92/03/26 include/Xol SMI"	/* buttonstack:MenuButton.h 1.4 	*/

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
 *		This is the "public" include file for the MenuButton
 *	Widget and Gadget.
 *
 *****************************file*header********************************
 */


#include <Xol/Button.h>

#include <X11/Shell.h>			/* need this for XtNtitle	*/


#ifdef	__cplusplus
extern "C" {
#endif


extern WidgetClass				menuButtonWidgetClass;
typedef struct _MenuButtonClassRec *		MenuButtonWidgetClass;
typedef struct _MenuButtonRec *			MenuButtonWidget;

extern WidgetClass				menuButtonGadgetClass;
typedef struct _MenuButtonGadgetClassRec *	MenuButtonGadgetClass;
typedef struct _MenuButtonGadgetRec *		MenuButtonGadget;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_MENUBUTTON_H */
