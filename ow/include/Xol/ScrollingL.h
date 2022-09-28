#ifndef	_XOL_SCROLLINGL_H
#define	_XOL_SCROLLINGL_H

#pragma	ident	"@(#)ScrollingL.h	302.1	92/03/26 include/Xol SMI" /* scrolllist:include/Xol/List.h 1.20 	*/

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
/*	All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * New Resources:
 *
 * Name			Type		Default	   Meaning
 * ----			----		-------	   -------
 * XtNapplAddItem	function	n/a	get and call to add item
 * XtNapplDeleteItem	function	n/a	get and call to delete item
 * XtNapplEditClose	function	n/a	get and call to begin edit
 * XtNapplEditOpen	function	n/a	get and call to end edit
 * XtNapplTouchItem	function	n/a	get and call to change item
 * XtNapplUpdateView	function	n/a	get and call to (un)lock view
 * XtNapplViewItem	function	n/a	get and call to view item
 * XtNselectable	Boolean		True	List is selectable
 * XtNuserDeleteItems	Callback	NULL	user event: delete (cut)
 * XtNuserMakeCurrent	Callback	NULL	user event: select
 * XtNresizeCallback	Callback	NULL	called on resize
 * XtNviewHeight	Cardinal	0	number of items in view
 */


#include <Xol/Form.h>		/* include superclass' header */
#include <Xol/ListPane.h>
#include <Xol/OpenLook.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct _ListClassRec*	ScrollingListWidgetClass;
typedef struct _ListRec*	ScrollingListWidget;


extern WidgetClass		scrollingListWidgetClass;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_SCROLLINGL_H */
