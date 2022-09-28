#pragma ident	"@(#)FooterPane.c	302.4	97/03/26 lib/libXol SMI"	/* FooterPane.c 1.16	*/

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


#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <Xol/FooterPanP.h>
#include <Xol/OpenLookP.h>


/**************************forward*declarations***************************
 *
 * Forward function definitions listed by category:
 *		1. Private functions
 *		2. Class   functions
 *		3. Action  functions
 *		4. Public  functions
 *
 */
                                                /* private procedures */
static void		CalcSize(FooterPanelWidget pw, Dimension *replyWidth, Dimension *replyHeight);
static void		Layout(FooterPanelWidget pw);
                              			/* class procedures */
static void		Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);
static void		ChangeManaged(Widget w);
static XtGeometryResult	GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply);
static XtGeometryResult	PreferredSize(Widget widget, XtWidgetGeometry *constraint, XtWidgetGeometry *preferred);
static void		Resize(Widget w);


/*****************************file*variables******************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 */

/***********************widget*translations*actions***********************
 *
 * Translations and Actions
 *
 */
/* None */

/****************************widget*resources*****************************
 *
 * FooterPanel Resources
 */

#define offset(field)		XtOffset(FooterPanelWidget, field)

#undef offset

/***************************widget*class*record***************************
 *
 * Full class record constant
 */

FooterPanelClassRec footerPanelClassRec = {
  {
/* core_class fields		*/
    /* superclass		*/	(WidgetClass) &managerClassRec,
    /* class_name		*/	"FooterPanel",
    /* widget_size		*/	sizeof(FooterPanelRec),
    /* class_initialize		*/	NULL,
    /* class_part_init		*/	NULL,
    /* class_inited		*/	FALSE,
    /* initialize		*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	XtInheritRealize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	NULL,
    /* num_resources		*/	0,
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	TRUE,
    /* compress_exposure	*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest		*/	FALSE,
    /* destroy			*/	NULL,
    /* resize			*/	Resize,
    /* expose			*/	NULL,
    /* set_values		*/	NULL,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus	 	*/	XtInheritAcceptFocus,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table	   		*/	XtInheritTranslations,
    /* query_geometry		*/	NULL,
  },
  { /* composite_class fields	*/
    /* geometry_manager		*/	GeometryManager,
    /* change_managed		*/	ChangeManaged,
    /* insert_child		*/	XtInheritInsertChild,
    /* delete_child		*/	XtInheritDeleteChild,
    /* extension		*/	NULL
  },
  { /* constraint_class fields	*/
    /* resources		*/	NULL,
    /* num_resources		*/	0,
    /* constraint_size		*/	0,
    /* initialize		*/	NULL,
    /* destroy			*/	NULL,
    /* set_values		*/	NULL
  },
  { /* manager_class fields	*/
    /* highlight_handler	*/	NULL,
    /* reserved			*/	NULL,
    /* reserved			*/	NULL,
    /* traversal_handler	*/	NULL,
    /* activate			*/	NULL,
    /* event_procs		*/	NULL,
    /* num_event_procs		*/	0,
    /* register_focus		*/	NULL,
    /* reserved			*/	NULL,
    /* version			*/	OlVersion,
    /* extension		*/	NULL,
    /* dyn_data			*/	{ NULL, 0 },
    /* transparent_proc		*/	_OlDefaultTransparentProc,
    /* query_sc_locn_proc       */	NULL,
  },
  { /* FooterPanel class fields	*/
    /* empty			*/	0,
  }
};

WidgetClass footerPanelWidgetClass = (WidgetClass)&footerPanelClassRec;

/***************************private*procedures****************************
 *
 * Private Functions
 *
 */

/******************************function*header****************************
 * CalcSize- calculate size necessary to bound (at most 2) children
 */

static void
CalcSize(FooterPanelWidget pw, Dimension *replyWidth, Dimension *replyHeight)
{
	Cardinal	child_cnt = pw->composite.num_children;
	Widget		kid;
	Widget		footer;

	if (child_cnt == 0) {
		*replyWidth	= 1;
		*replyHeight	= 1;
		return;
	}

	kid = pw->composite.children[0];

	if (child_cnt > 1) {
		footer		= pw->composite.children[1];

		*replyWidth	= _OlMax((Dimension)(_OlWidgetWidth(kid)),
					 (Dimension)(_OlWidgetWidth(footer)));

		*replyHeight	= _OlWidgetHeight(kid) +
					_OlWidgetHeight(footer);
	} else {
		*replyWidth	= _OlWidgetWidth(kid);
		*replyHeight	= _OlWidgetHeight(kid);
	}

} /* CalcSize */

/******************************function*header****************************
 * Layout- layout (at most 2) children
 */

static void
Layout(FooterPanelWidget pw)
{
	Cardinal	child_cnt = pw->composite.num_children;
	Dimension	top_height;
	Widget		top;
	Widget		footer;

	if (child_cnt == 0)
		return;

	top	= pw->composite.children[0];

	if (child_cnt > 1) {
		footer = pw->composite.children[1];

		/* calculate height for 'top' child */
		top_height = pw->core.height - 2*top->core.border_width - 
				_OlWidgetHeight(footer);

		/* configure footer */
		XtConfigureWidget(footer,
			0,						/* x */
			pw->core.height - _OlWidgetHeight(footer),	/* y */
			pw->core.width - 2*footer->core.border_width,
			footer->core.height,
			footer->core.border_width);
	} else {
		/* calculate height for 'top' child */
		top_height = pw->core.height - 2*top->core.border_width;
	}

	/* configure 'top' child */
	XtConfigureWidget(top, 0, 0, pw->core.width - 2*top->core.border_width,
		top_height, top->core.border_width);

} /* Layout */


/*************************************************************************
 *
 * Class Procedures
 *
 */

/******************************function*header****************************
 * Initialize-
 */

/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{

	static Arg arg = { XtNallowShellResize, TRUE };

	if (new->core.width == 0)
		new->core.width = 10;

	if (new->core.height == 0)
		new->core.height = 10;

	/* if we're a child of a shell, tell it to allow resizes */
	if (XtIsSubclass(XtParent(new), shellWidgetClass))
		XtSetValues(XtParent(new), &arg, 1);

} /* Initialize */

/******************************function*header****************************
 * ChangeManaged-
 */

static void
ChangeManaged(Widget w)
{
    FooterPanelWidget fpw = (FooterPanelWidget)w;
    Dimension width, height;

    CalcSize(fpw, &width, &height);

    if (XtMakeResizeRequest(w, width, height, NULL, NULL) == XtGeometryYes)
	Layout(fpw);

} /* ChangeManaged */

/******************************function*header****************************
 * GeometryManager-
 */

/*ARGSUSED*/
static XtGeometryResult
GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply)
{
	Dimension	save_w, save_h, save_bw;
	Dimension	width, height;
	FooterPanelWidget	parent;		/* that's me */

	/* Position request always denied */
	if ((request->request_mode & CWX && request->x != w->core.x) ||
	    (request->request_mode & CWY && request->y != w->core.y))
		return (XtGeometryNo);

	/* if other than size change request, grant it */
	if ( !(request->request_mode & (CWWidth | CWHeight | CWBorderWidth)))
		return (XtGeometryYes);

	/* Make all three fields in the request valid */
	if ( !(request->request_mode & CWWidth))
		request->width = w->core.width;
	if ( !(request->request_mode & CWHeight))
		request->height = w->core.height;
	if ( !(request->request_mode & CWBorderWidth))
		request->border_width = w->core.border_width;

	/* Save current size and set to new size */
	save_w		= w->core.width;
	save_h		= w->core.height;
	save_bw		= w->core.border_width;
	w->core.width	= request->width;
	w->core.height	= request->height;
	w->core.border_width = request->border_width;

	parent = (FooterPanelWidget)XtParent(w);

	CalcSize(parent, &width, &height);

	if (XtMakeResizeRequest((Widget) parent, width, height, NULL, NULL)
	    == XtGeometryYes) {
		Layout(parent);
		return (XtGeometryYes);

	} else {
						/* restore child geometry */
		w->core.width	= save_w;
		w->core.height	= save_h;
		w->core.border_width = save_bw;
		return (XtGeometryNo);
	}

} /* GeometryManager */

/******************************function*header****************************
 * PreferredSize-
 */

static XtGeometryResult
PreferredSize(Widget widget, XtWidgetGeometry *constraint, XtWidgetGeometry *preferred)
{
	return (XtGeometryYes);
}

/******************************function*header****************************
 * Resize-
 */

static void
Resize(Widget w)
{
    Layout((FooterPanelWidget)w);
} /* Resize */
