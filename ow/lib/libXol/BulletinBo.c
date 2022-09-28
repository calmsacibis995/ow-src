#pragma ident	"@(#)BulletinBo.c	302.4	97/03/26 lib/libXol SMI"	/* bboard:src/BulletinBo.c 1.18	*/

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

/**************************************************************************
 *
 * Description:	Open Look Bulletin Board widget
 *
 *******************************file*header********************************/


#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/BulletinBP.h>
#include <Xol/OlDnDVCX.h>
#include <Xol/OpenLookP.h>
#include <Xol/SuperCaret.h>


/**************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations****************************/

					/* private procedures		*/
static Boolean		LayoutChildren(BulletinBoardWidget w, Boolean resizable);

					/* class procedures		*/

static void		ClassInitialize(void);
static void 		Resize(BulletinBoardWidget w);
static void		Initialize(BulletinBoardWidget request, BulletinBoardWidget new, ArgList args, Cardinal *num_args);
static Boolean		SetValues(BulletinBoardWidget current, BulletinBoardWidget request, BulletinBoardWidget new, ArgList args, Cardinal *num_args);
static XtGeometryResult	GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply);
static void		ChangeManaged(BulletinBoardWidget w);

					/* action procedures		*/

					/* public procedures		*/

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define	DEFAULT_SIZE	10

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource resources[] =
{

   { XtNlayout, XtCLayout, XtROlDefine, sizeof(OlDefine),
      XtOffset(BulletinBoardWidget, bulletin.layout), XtRImmediate, 
 		(XtPointer) ((OlDefine) OL_MINIMIZE) },
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

BulletinBoardClassRec bulletinBoardClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &managerClassRec,
    /* class_name         */    "BulletinBoard",
    /* widget_size        */    sizeof(BulletinBoardRec),
    /* class_initialize   */    ClassInitialize,
    /* class_part_init    */	NULL,
    /* class_inited       */	FALSE,
    /* initialize         */    (XtInitProc)Initialize,
    /* initialize_hook    */    NULL,
    /* realize            */    XtInheritRealize,
    /* actions		  */	NULL,
    /* num_actions	  */	0,
    /* resources          */    resources,
    /* num_resources      */    XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	TRUE,
    /* compress_enterlv   */    TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    NULL,
    /* resize             */    (XtWidgetProc)Resize,
    /* expose             */    NULL,
    /* set_values         */    (XtSetValuesFunc)SetValues,
    /* set_values_hook    */    NULL,
    /* set_values_almost  */    XtInheritSetValuesAlmost,
    /* get_values_hook    */	NULL,
    /* accept_focus       */    XtInheritAcceptFocus,
    /* version            */	XtVersion,
    /* callback_private   */	NULL,
    /* tm_table           */	XtInheritTranslations,
    /* query_geometry     */	NULL, 
  },{
/* composite_class fields */
    /* geometry_manager   */    GeometryManager,
    /* change_managed     */    (XtWidgetProc)ChangeManaged,
    /* insert_child	  */	XtInheritInsertChild,
    /* delete_child	  */	XtInheritDeleteChild,
    /* extension	  */    NULL
  },{
/* constraint_class fields */
    /* resources	  */	NULL,
    /* num_resources	  */	0,
    /* constraint_size	  */	0,
    /* initialize	  */	NULL,
    /* destroy		  */	NULL,
    /* set_values	  */	NULL
   },{
/* manager_class fields   */
    /* highlight_handler  */	NULL,
    /* reserved		  */	NULL,
    /* reserved		  */	NULL,
    /* traversal_handler  */    NULL,
    /* activate		  */    NULL,
    /* event_procs	  */    NULL,
    /* num_event_procs	  */	0,
    /* register_focus	  */	NULL,
    /* reserved		  */	NULL,
    /* version		  */	OlVersion,
    /* extension	  */	NULL,
    /* dyn_data		  */	{ NULL, 0 },
    /* transparent_proc   */	_OlDefaultTransparentProc,
    /* query_sc_locn_proc */	NULL,
   },{
/* bulletin board class */     
    /* none		*/	NULL
 }	
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass bulletinBoardWidgetClass = (WidgetClass)&bulletinBoardClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */


/*
 *************************************************************************
 *
 *  LayoutChildren - Calculate the positions of all managed children. 
 *
 *  if layout == OL_MINIMIZE, try to resize yourself to fit.  
 *  if layout == OL_MAXIMIZE, resize yourself only if you need to grow.
 *  if layout == OL_IGNORE, do nothing.
 *
 *  If resizable == False, you aren't allowed to try to resize at all.
 *  Return False if any attempted resize fails.
 *
 *************************************************************************
 */

static Boolean
LayoutChildren (BulletinBoardWidget w, Boolean resizable)
{
	Widget child;
	register int i;
	Dimension tempsize;

	Dimension managerWidth, managerHeight;
	Dimension replyWidth, replyHeight;

	if (w->bulletin.layout == OL_IGNORE)
		return (True);

	managerWidth = DEFAULT_SIZE;
	managerHeight = DEFAULT_SIZE;

	for (i = 0; i < w -> composite.num_children; i++) {
		child = w -> composite.children[i];

		if (child -> core.managed) {

			tempsize =	(int) child->core.x +
					(int) child->core.width +
					(2 * (int) child->core.border_width);
			if (tempsize > managerWidth) {
				managerWidth = tempsize;
			}

			tempsize =	(int) child->core.y +
					(int) child->core.height +
					(2 * (int) child->core.border_width);
			if (tempsize > managerHeight) {
				managerHeight = tempsize;
			}

		}
	}

	if (w->bulletin.layout == OL_MAXIMIZE) {
		if (managerWidth < w->core.width)
			managerWidth = w->core.width;
		if (managerHeight < w->core.height)
			managerHeight = w->core.height;
	}

	if (	(w->bulletin.layout != OL_IGNORE) &&
			(managerWidth != w->core.width || 
			 managerHeight != w->core.height)) {

		 if (!resizable) {
			return (False);
		}

		else {

			switch (XtMakeResizeRequest (	(Widget) w, 
						managerWidth, managerHeight,
				                &replyWidth, &replyHeight)) {
			case XtGeometryYes:
				break;

			case XtGeometryNo:
/*
** This picks up the case where parent answers "No" but sets replyHeight and 
** replyWidth to what I asked for.  Don't ask why; I don't understand either.
*/
				if ((replyWidth <= w->core.width) &&
		    			(replyHeight <= w->core.height)) {
		    			return (True);
				}
				else {
					return (False);
				}

			case XtGeometryAlmost:
				XtMakeResizeRequest (	(Widget) w, 
						replyWidth, replyHeight,
						NULL, NULL );
				break;
			}
		}
	}

	return (True);
}

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 *
 * ChangeManaged - Child has changed.  Lay yourself out again, with
 * resizing allowed.
 *
 *************************************************************************
 */

static void
ChangeManaged (BulletinBoardWidget w)
{
	(void) LayoutChildren (w,True);
}

/*
 *************************************************************************
 *
 * Initialize
 *
 *************************************************************************
 */

/* ARGSUSED */
static void
Initialize (BulletinBoardWidget request, BulletinBoardWidget new, ArgList args, Cardinal *num_args)
{
	if (!new->core.height)
		new->core.height = DEFAULT_SIZE;

	if (!new->core.width)
		new->core.width = DEFAULT_SIZE;

    
} /* Initialize */


/*
 *************************************************************************
 *
 * SetValues
 *
 * Refuse to change height/width unless layout is OL_IGNORE.
 *
 *************************************************************************
 */

/* ARGSUSED */
static Boolean
SetValues(BulletinBoardWidget current, BulletinBoardWidget request, BulletinBoardWidget new, ArgList args, Cardinal *num_args)
{
	if (current->bulletin.layout != OL_IGNORE) {
		new->core.width = current->core.width;
		new->core.height = current->core.height;
	}

	return (False);
}


/*
 *************************************************************************
 * ClassInitialize - Register OlDefine string values
 *************************************************************************
 */

static void 
ClassInitialize(void)
{
	_OlAddOlDefineType ("maximize", OL_MAXIMIZE);
	_OlAddOlDefineType ("minimize", OL_MINIMIZE);
	_OlAddOlDefineType ("ignore",   OL_IGNORE);
}

/*
 *************************************************************************
 *
 * Resize - You've been resized.  Lay out the children, and don't try to
 * resize yourself.
 *
 *************************************************************************
 */

static void 
Resize(BulletinBoardWidget w)
{
	Boolean	configured;

	_OlDnDSetDisableDSClipping((Widget)w, True);
	configured = LayoutChildren(w, False);
	_OlDnDSetDisableDSClipping((Widget)w, False);
	if (configured)
		OlDnDWidgetConfiguredInHier((Widget)w);
}

/*
 *************************************************************************
 *
 * GeometryManager - Partially lifted from Xw/Box.c
 *
 *************************************************************************
 */

static XtGeometryResult 
GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply)
{
	Dimension	width, height, borderWidth;
	Position	x,y;
	BulletinBoardWidget bbw;

/*
** Make all fields in the request valid
*/
	if ((request->request_mode & CWWidth) == 0)
	    request->width = w->core.width;
	if ((request->request_mode & CWHeight) == 0)
	    request->height = w->core.height;
        if ((request->request_mode & CWBorderWidth) == 0)
	    request->border_width = w->core.border_width;
        if ((request->request_mode & CWX) == 0)
	    request->x = w->core.x;
        if ((request->request_mode & CWY) == 0)
	    request->y = w->core.y;
/* 
** Save current values and set to new ones
*/
	width = w->core.width;
	height = w->core.height;
	borderWidth = w->core.border_width;
	x = w->core.x;
	y = w->core.y;

	w->core.width = request->width;
	w->core.height = request->height;
	w->core.border_width = request->border_width;
	w->core.x = request->x;
	w->core.y = request->y;

/* 
** See if new values work
*/

	bbw = (BulletinBoardWidget) w->core.parent;

	_OlDnDSetDisableDSClipping((Widget)bbw, True);
	if (LayoutChildren(bbw,True)) {
/* 
** Layout would work.  Make it so.
*/
		_OlDnDSetDisableDSClipping((Widget)bbw, False);
		OlDnDWidgetConfiguredInHier(w);
		OlWidgetConfigured(w);
		return (XtGeometryYes);
	} else {

/* 
** Cannot satisfy request, change back to original geometry
*/
		_OlDnDSetDisableDSClipping((Widget)bbw, False);

		w->core.width = width;
		w->core.height = height;
		w->core.border_width = borderWidth;
		w->core.x = x;
		w->core.y = y;
		return (XtGeometryNo);
	}
}
