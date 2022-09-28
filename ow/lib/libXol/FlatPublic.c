#pragma ident	"@(#)FlatPublic.c	302.5	97/03/26 lib/libXol SMI"	/* flat:FlatPublic.c 1.3	*/

/*
 *	Copyright (C) 1986,1992  Sun Microsystems, Inc
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
 *************************************************************************
 *
 * Description:
 *	This file contains convenience routines for public consumption.
 *
 ******************************file*header********************************
 */


#include <stdio.h>

#include <X11/IntrinsicP.h>

#include <Xol/Error.h>
#include <Xol/FlatP.h>
#include <Xol/OpenLookP.h>


/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private  Procedures 
 *		2. Public  Procedures 
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static 	int	CheckId (Widget, const char *, Boolean, Cardinal);


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define FPART(w)	(((FlatWidget)(w))->flat)
#define FCPART(w)	(((FlatWidgetClass)XtClass(w))->flat_class)
#define CNAME(w)	(XtClass(w)->core_class.class_name)

#define ALLOC_STACK_ITEM(fcp,iptr,stk)\
	auto char	stk[512];\
	FlatItem	iptr = (FlatItem)(fcp.rec_size>sizeof(stk)?\
				XtMalloc(fcp.rec_size) : stk)
#define FREE_STACK_ITEM(iptr,stk) if ((char*)iptr!=stk){XtFree((char *)iptr);}

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ****************************private*procedures***************************
 */

/*
 *************************************************************************
 * CheckId -
 ****************************procedure*header*****************************
 */
static int
CheckId (Widget w, const char *proc_name,
	 Boolean check_index, Cardinal item_index)
{
	int	success = 0;

	if (w == (Widget)NULL)
	{
		OlVaDisplayWarningMsg((Display *)NULL, OleNnullWidget,
			OleTflatState, OleCOlToolkitWarning,
			OleMnullWidget_flatState, proc_name);
	}
	else if (XtIsSubclass(w, flatWidgetClass) == False)
	{
		OlVaDisplayWarningMsg(XtDisplayOfObject(w), "notFlatSubclass",
			OleTflatState, OleCOlToolkitWarning,
			"Procedure %s: Widget \"%s\" (class \"%s\") is\
 not a FlatWidget subclass", proc_name, XtName(w), CNAME(w));
	}
	else if (check_index == True && item_index > FPART(w).num_items)
	{
		OlVaDisplayWarningMsg(XtDisplay(w), OleNbadItemIndex,
			OleTflatState, OleCOlToolkitWarning,
			OleMbadItemIndex_flatState, XtName(w),
			CNAME(w), proc_name, item_index);
	}
	else
	{
		success = 1;
	}
	return(success);
} /* END OF CheckId() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * OlFlatCallAcceptFocus - public interface to setting focus
 * to a flattened widget item.
 ****************************procedure*header*****************************
 */
Boolean
OlFlatCallAcceptFocus (w, i, time)
 Widget			w;
 Cardinal		i;
 Time			time;
{
	Boolean	took_it = False;

	GetToken();
	if (CheckId(w, (const char *)"OlFlatCallAcceptFocus", True, i))
	{
		ALLOC_STACK_ITEM(FCPART(w), item, stack);

		took_it = _OlFlatItemAcceptFocus(w, item, time);

		FREE_STACK_ITEM(item,stack);
	}
	ReleaseToken();
	return (took_it);
} /* END OF OlFlatCallAcceptFocus() */

/*
 *************************************************************************
 * OlFlatGetFocusItem - returns the current focus item for a flat
 * widget.  If there is no current focus item, OL_NO_ITEM is returned.
 ****************************procedure*header*****************************
 */
Cardinal
OlFlatGetFocusItem (w)
 Widget		w;
{
Cardinal retval = (Cardinal)OL_NO_ITEM;

	GetToken();
	if (!CheckId(w, (const char *)"OlFlatGetFocusItem", False, 0))
	{
		ReleaseToken();
		return retval;
	}
	retval = (((FlatWidget)w)->flat.focus_item);
	ReleaseToken();
	return retval;
} /* END OF OlFlatGetFocusItem() */

/*
 *************************************************************************
 * OlFlatGetItemGeometry - returns the item at the given coordinates.
 ****************************procedure*header*****************************
 */
void
OlFlatGetItemGeometry (w, i, x_ret, y_ret, w_ret, h_ret)
 Widget			w;
 Cardinal		i;
 Position *		x_ret;
 Position *		y_ret;
 Dimension *		w_ret;
 Dimension *		h_ret;
{
	GetToken();
	if (!CheckId(w, (const char *)"OlFlatGetItemGeometry", True, i))
	{
		*x_ret = *y_ret = (Position)0;
		*w_ret = *h_ret = (Dimension)0;
	}
	else
	{
		OlFlatDrawInfo	di;

		_OlFlatGetDrawInfo(w, i, &di);

		*x_ret = di.x;
		*y_ret = di.y;
		*w_ret = di.width;
		*h_ret = di.height;
	}
	ReleaseToken();
} /* END OF OlFlatGetItemGeometry() */

/*
 *************************************************************************
 * OlFlatGetItemIndex - returns the item at the given coordinates.
 ****************************procedure*header*****************************
 */
Cardinal
OlFlatGetItemIndex (Widget w, Position x, Position y)
{
Cardinal retval = (Cardinal)OL_NO_ITEM;

	GetToken();
	if (!CheckId(w, (const char *)"OlFlatGetItemIndex", False, 0))
	{
		ReleaseToken();
		return(retval);
	}
	retval = (_OlFlatGetIndex(w, x, y, False));
	ReleaseToken();
	return retval;
} /* END OF OlFlatGetItemIndex() */
