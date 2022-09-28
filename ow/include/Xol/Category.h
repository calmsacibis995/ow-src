#ifndef _XOL_CATEGORY_H
#define	_XOL_CATEGORY_H

#pragma	ident	"@(#)Category.h	302.1	92/03/26 include/Xol SMI"	/* category:Category.h 1.4	*/

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


#include <Xol/Manager.h>
#include <Xol/OpenLook.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct _OlCategoryNewPage {
	Widget          c_new;
	Widget          old;
	Boolean         apply_all;
}               OlCategoryNewPage;


extern char		XtNavailableWhenUnmanaged [];
extern char		XtNcategoryLabel          [];
extern char		XtNcategoryFont           [];
extern char		XtNchanged                [];
extern char		XtNleftFoot               [];
extern char		XtNnewPage                [];
extern char		XtNpageLabel              [];
extern char		XtNpageHeight             [];
extern char		XtNpageWidth              [];
extern char		XtNqueryChild             [];
extern char		XtNrightFoot              [];
extern char		XtNshowFooter             [];

extern char		XtCAvailableWhenUnmanaged [];
extern char		XtCCategoryLabel          [];
extern char		XtCCategoryFont           [];
extern char		XtCChanged                [];
extern char		XtCLeftFoot               [];
extern char		XtCNewPage                [];
extern char		XtCPageLabel              [];
extern char		XtCPageWidth              [];
extern char		XtCPageHeight             [];
extern char		XtCQueryChild             [];
extern char		XtCRightFoot              [];
extern char		XtCShowFooter             [];

extern WidgetClass	categoryWidgetClass;


typedef struct _CategoryClassRec*	CategoryWidgetClass;
typedef struct _CategoryRec*		CategoryWidget;
typedef struct _CategoryConstraintRec*	CategoryConstraints;


#if	defined(__STDC__) || defined(__cplusplus)

/*
 * Used to change the property sheet. Returns True if the
 * Apply All button should be displayed.
 */
extern Boolean		OlCategorySetPage(CategoryWidget w, Widget child);

#else	/* __STDC__ || __cplusplus */

extern Boolean		OlCategorySetPage();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_CATEGORY_H */
