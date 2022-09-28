#pragma ident	"@(#)CheckBox.c	302.22	97/03/26 lib/libXol SMI"	/* checkbox:src/CheckBox.c 1.29.2.25	*/

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
 *************************************************************************
 *
 * Description: CheckBox.c - CheckBox widget
 *
 ******************************file*header********************************
 */


#include <ctype.h>
#include <locale.h>
#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/CheckBoxP.h>
#include <Xol/Menu.h>
#include <Xol/NonexclusP.h>
#include <Xol/OpenLookP.h>
#include <Xol/SuperCaret.h>

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures 
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/
static void DrawBox(Widget w);
static Boolean PointInBox(Widget w, Position x, Position y, Boolean activate);
static void ResizeSelf(CheckBoxWidget ew, Boolean greq);

					/* class procedures		*/

static Boolean	AcceptFocus(Widget w, Time *time);
static Boolean	ActivateWidget (Widget, OlVirtualName, XtPointer);
static void ClassInitialize(void);
static XtGeometryResult GeometryManager(Widget widget, XtWidgetGeometry *request, XtWidgetGeometry *reply);
static void GetValuesHook(Widget w, ArgList args, Cardinal *num_args);
static void HighlightHandler (Widget, OlDefine);
static void Initialize(CheckBoxWidget request, CheckBoxWidget new, ArgList args, Cardinal *num_args);
static void InsertChild(Widget widget);
static XtGeometryResult QueryGeometry(Widget widget, XtWidgetGeometry *request, XtWidgetGeometry *preferred);
static void Realize(Widget widget, Mask *ValueMask, XSetWindowAttributes *attributes);
static void Redisplay(Widget widget, XEvent *event, Region region);
static void Resize(Widget w);
static Boolean SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);
static void    Destroy(Widget w);
static void     TransparentProc (Widget, Pixel, Pixmap);
static void 	CheckBoxQuerySCLocnProc(const Widget		target,
			const Widget		supercaret,
			const Dimension		sc_width,
			const Dimension		sc_height,
			unsigned int    *const	scale,	
			SuperCaretShape *const 	shape,
			Position        *const	x_center_return,
			Position        *const	y_center_return);
					/* action procedures		*/

static void CheckBoxHandler(Widget w, OlVirtualEvent ve);
static void MotionHandler(Widget w, OlVirtualEvent ve);
static Boolean PreviewState(Widget w, XEvent *event, OlVirtualName vname, Boolean activate);
static Boolean SetState(Widget w, XEvent *event, OlVirtualName vname, Boolean activate);

					/* public procedures		*/

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define offset(field) XtOffset(CheckBoxWidget, field)

static Boolean defTRUE = (Boolean) TRUE;
static Boolean defFALSE = (Boolean) FALSE;

#define BYTE_OFFSET	XtOffsetOf(CheckBoxRec, checkBox.dyn_flags)
static _OlDynResource dyn_res[] = {
{ { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET,
	 OL_B_CHECKBOX_FG, NULL },
{ { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET,
	 OL_B_CHECKBOX_FONTCOLOR, NULL },
};
#undef BYTE_OFFSET

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

#if 1
static char defaultTranslations[] = "\
	<FocusIn>:	OlAction() \n\
	<FocusOut>:	OlAction() \n\
	<Key>:		OlAction() \n\
	<BtnDown>:	OlAction() \n\
	<BtnUp>:	OlAction() \n\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
	<BtnMotion>:	OlAction()";
#else
static char defaultTranslations[] = "#augment\n\
	<Enter>:	OlAction() \n\
	<Leave>:	OlAction() \n\
	<BtnMotion>:	OlAction()";
#endif

static OlEventHandlerRec
CBevents[] = {
	{ ButtonPress,	CheckBoxHandler},
	{ ButtonRelease,CheckBoxHandler},
	{ EnterNotify,	CheckBoxHandler},
	{ LeaveNotify,	CheckBoxHandler},
	{ MotionNotify, MotionHandler},
};

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource resources[] = { 

	{ XtNtextFormat, XtCTextFormat, XtROlStrRep, sizeof(OlStrRep),
	offset(checkBox.text_format), XtRCallProc, 
	(XtPointer) _OlGetDefaultTextFormat },

	{XtNselect, XtCCallback, XtRCallback, sizeof(XtPointer), 
	 offset(checkBox.select), XtRCallback, (XtPointer) NULL},

	{XtNunselect, XtCCallback, XtRCallback, sizeof(XtPointer), 
	 offset(checkBox.unselect), XtRCallback, (XtPointer) NULL},

	{XtNset, XtCSet, XtRBoolean, sizeof(Boolean),
	 offset(checkBox.set), XtRBoolean, (XtPointer) &defFALSE},

	{XtNdim, XtCDim, XtRBoolean, sizeof(Boolean),
	 offset(checkBox.dim), XtRBoolean, (XtPointer) &defFALSE},

	{XtNlabel, XtCLabel, XtROlStr, sizeof(OlStr),
	 offset(checkBox.label), XtRString, (XtPointer) NULL},

	{XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	 offset(checkBox.foreground), XtRString,
	 (XtPointer) XtDefaultForeground},

	{XtNfont, XtCFont, XtROlFont, sizeof(OlFont),
	 offset(checkBox.font), XtRString,
	 (XtPointer) XtDefaultFont},

	{XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel),
	 offset(checkBox.fontcolor), XtRString,
	 (XtPointer) XtDefaultForeground},

 	{XtNlabelType, XtCLabelType, XtROlDefine, sizeof(OlDefine),
 	 offset(checkBox.labeltype), XtRImmediate,
 		(XtPointer) ((OlDefine) OL_STRING ) },

 	{XtNlabelJustify, XtCLabelJustify, XtROlDefine, sizeof(OlDefine),
 	 offset(checkBox.labeljustify), XtRImmediate,
 		(XtPointer) ((OlDefine) OL_LEFT ) },

 	{XtNposition, XtCPosition, XtROlDefine, sizeof(OlDefine),
 	  offset(checkBox.position),XtRImmediate,
		(XtPointer) ((OlDefine) OL_LEFT ) },

	{XtNlabelTile, XtCLabelTile, XtRBoolean, sizeof(Boolean),
	 offset(checkBox.labeltile), XtRBoolean, (XtPointer) &defFALSE},

	{XtNlabelImage, XtCLabelImage, XtRPointer, sizeof(XImage *),
	 offset(checkBox.labelimage), XtRPointer, (XtPointer)NULL},

	{XtNrecomputeSize, XtCRecomputeSize, XtRBoolean, sizeof(Boolean),
	 offset(checkBox.recompute_size), XtRBoolean, (XtPointer) &defTRUE },

	{ XtNaccelerator, XtCAccelerator, XtRString, sizeof(String),
	  offset(checkBox.accelerator), XtRString, (XtPointer) NULL },

	{ XtNacceleratorText, XtCAcceleratorText, XtRString, sizeof(String),
	  offset(checkBox.accelerator_text), XtRString, (XtPointer) NULL },

	{ XtNmnemonic, XtCMnemonic, OlRChar, sizeof(char),
	  offset(checkBox.mnemonic), XtRImmediate, (XtPointer) '\0' }

#ifdef	sun	/*OWV3*/
	,
	{XtNscale, XtCScale, XtROlScale, sizeof(int),
	 offset(checkBox.scale), XtRImmediate, (XtPointer) OL_DEFAULT_POINT_SIZE}
#endif	/*OWV3*/
};

#undef offset

/* 
 *************************************************************************
 *
 *  Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

CheckBoxClassRec checkBoxClassRec = {
  {
    (WidgetClass) &(managerClassRec),	/* superclass		  */	
    "CheckBox",				/* class_name		  */
    sizeof(CheckBoxRec),		/* widget_size		  */
    ClassInitialize,			/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    (XtInitProc) Initialize,		/* initialize		  */
    NULL,				/* initialize_hook	  */
    (XtRealizeProc) Realize,		/* realize		  */
    NULL,				/* actions		  */
    0,					/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* num_resources	  */
    NULLQUARK,				/* xrm_class		  */
    FALSE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    FALSE,				/* visible_interest	  */
    Destroy,				/* destroy		  */
    (XtWidgetProc) Resize,		/* resize		  */
    (XtExposeProc) Redisplay,		/* expose		  */
    (XtSetValuesFunc) SetValues,	/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    (XtArgsProc) GetValuesHook,		/* get_values_hook	  */
    AcceptFocus,			/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    defaultTranslations,		/* tm_table		  */
    (XtGeometryHandler) QueryGeometry,	/* query_geometry	  */
  },  /* CoreClass fields initialization */
  {
    (XtGeometryHandler) GeometryManager,/* geometry_manager	*/
    NULL,				/* changed_managed	*/
    (XtWidgetProc) InsertChild,		/* insert_child		*/
    XtInheritDeleteChild,		/* delete_child		*/
    NULL,				/* extension    	*/
  },  /* CompositeClass fields initialization */
  {
    /* resources	  */	NULL,
    /* num_resources	  */	0,
    /* constraint_size	  */	0,
    /* initialize	  */	NULL,
    /* destroy		  */	NULL,
    /* set_values	  */	NULL
  },	/* constraint_class fields */
  {
    /* highlight_handler  */	HighlightHandler,
    /* reserved		  */	NULL,
    /* reserved		  */	NULL,
    /* traversal_handler  */    NULL,
    /* activate		  */    ActivateWidget, 
    /* event_procs	  */    CBevents,
    /* num_event_procs	  */	XtNumber(CBevents),
    /* register_focus	  */	NULL,
    /* reserved		  */	NULL,
    /* version		  */	OlVersion,
    /* extension	  */	NULL,
    /* dyn_data		  */	{ dyn_res, XtNumber(dyn_res) },
    /* transparent_proc   */    TransparentProc,
    /* query_sc_locn_proc */	CheckBoxQuerySCLocnProc,
  },	/* manager_class fields   */
  {
    0					/* not used now */
  }  /* CheckBoxClass fields initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

   WidgetClass checkBoxWidgetClass = (WidgetClass) &checkBoxClassRec;  
/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 ************************************************************
 *
 *  GetAttrs - This function get the attributes for the checkBox box 
 *
 *********************function*header************************
 */

static void
GetAttrs (CheckBoxWidget cb)
{
    Pixel	fg;
    OlgxBG	bg;
    Boolean	pixmapBG;
    Pixel	focus_color;
    Boolean	has_focus;

    if (cb->checkBox.pAttrs)
	OlgxDestroyAttrs ((Widget)cb, cb->checkBox.pAttrs);

    /* Worry about input focus color conflicts */
    focus_color = cb->manager.input_focus_color;
    has_focus = cb->manager.has_focus;

    if (has_focus && 
		_OlInputFocusFeedback((Widget)cb) == OL_INPUT_FOCUS_COLOR)
    {
	if (cb->checkBox.foreground == focus_color ||
	    cb->core.background_pixel == focus_color)
	{
	    /* input focus color conflicts with either the foreground
	     * or background color.  Reverse fg and bg.
	     */
	    fg = cb->core.background_pixel;
	    bg.pixel = cb->checkBox.foreground;
	    pixmapBG = False;
	}
	else
	{
	    /* no color conflict */
	    fg = cb->checkBox.foreground;
	    bg.pixel = focus_color;
	    pixmapBG = False;
	}
    }
    else
    {
	/* normal coloration */
	fg = cb->checkBox.foreground;

	/* OLGX_TODO: OLGX doesn't support background pixmap yet; just say no */
	/*******************
	if (cb->core.background_pixmap != None &&
	    cb->core.background_pixmap != XtUnspecifiedPixmap)
	{
	    bg.pixmap = cb->core.background_pixmap;
	    pixmapBG = True;
	}
	else
	*******************/
	{
	    bg.pixel = cb->core.background_pixel;
	    pixmapBG = False;
	}
    }

    /* Although the checkbox rendered by OLGX doesn't use a text font,
     * (the Button widget will handle it) we pass in the font so that
     * the same attr structure may be shared between the CheckBox's
     * label(Button widget) and checkbox.
     */
    cb->checkBox.pAttrs = OlgxCreateAttrs ((Widget)cb, fg, &(bg), pixmapBG,
					  cb->checkBox.scale, 
					  cb->checkBox.text_format,
					  cb->checkBox.font);
}

/*
 ************************************************************
 *
 *  DrawBox - This function displays/redisplays the checkBox box 
 *		and redisplays/erases the check.
 *
 *********************function*header************************
 */

static void 
DrawBox(Widget w)
{
	CheckBoxWidget cb = (CheckBoxWidget) w;
	int flags = 0;

	if(XtIsRealized(w)==FALSE) 
	    return;

	if (cb->checkBox.set)
	    flags |= OLGX_CHECKED;

	if(cb->checkBox.dim || !XtIsSensitive(w))
	    flags |= OLGX_INACTIVE;

	if(cb->checkBox.preview_state) 
	    flags |= OLGX_INVOKED;

	olgx_draw_check_box(cb->checkBox.pAttrs->ginfo, XtWindow(w),
	    (int)cb->checkBox.x1, (int)cb->checkBox.y1, flags);
} /* DrawBox */

/*
 ************************************************************
 *
 *  PointInBox - This function checks to see if pointer 
 *  is within the boundaries of the checkBox box.
 *
 *********************function*header************************
 */

static Boolean
PointInBox(Widget w, Position x, Position y, Boolean activate)
{
	CheckBoxWidget cb = (CheckBoxWidget) w;

	if(activate) 
		return TRUE;

	if(x>=cb->checkBox.x1
		&& x<=cb->checkBox.x2
		&& y>=cb->checkBox.y1
		&& y<=cb->checkBox.y2)

		return TRUE;

	return FALSE;

}	/* PointInBox */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * AcceptFocus - this routine is to set focus to the checkBox itself
 * instead of looking for a child to set focus to (as XtInheritAcceptFocus
 * for the manager class does - code is same as for primitive).
 ****************************procedure*header*****************************
 */
static Boolean
AcceptFocus(Widget w, Time *time)
{
	if (OlCanAcceptFocus(w, *time))
	{
		OlSetInputFocus(w, RevertToNone, *time);
		return (True);
	}

	return (False);

} /* AcceptFocus */

/*
 *************************************************************************
 * ActivateWidget - this routine is used to activate the callbacks of
 * this widget.
 ****************************procedure*header*****************************
 */

static Boolean
ActivateWidget(
	Widget		w,
	OlVirtualName	type,
	XtPointer	data)
{
	Boolean ret=FALSE;
	Boolean activate=TRUE;
	XEvent dummy_event;

	ret=PreviewState(w,&dummy_event,type,activate);
	if(ret==FALSE)
		return FALSE;
	ret=SetState(w,&dummy_event,type,activate);
		return ret;

} /* END OF ActivateWidget() */

/*
 ************************************************************
 *
 *  ClassInitialize - Register OlDefine string values.
 *
 *********************function*header************************
 */

static void 
ClassInitialize(void)
{
	_OlAddOlDefineType ("left",   OL_LEFT);
	_OlAddOlDefineType ("right",  OL_RIGHT);
	_OlAddOlDefineType ("string", OL_STRING);
	_OlAddOlDefineType ("image",  OL_IMAGE);

}	/*  ClassInitialize  */

/*
 ************************************************************
 *
 *  Destroy
 *
 *********************function*header************************
 */

static void 
Destroy(Widget widget)
{
	CheckBoxWidget cb = (CheckBoxWidget) widget;

	if (cb->checkBox.pAttrs != (OlgxAttrs *) 0)
	    OlgxDestroyAttrs ((Widget)cb, cb->checkBox.pAttrs);

        XtRemoveAllCallbacks(widget, XtNselect);
        XtRemoveAllCallbacks(widget, XtNunselect);

}	/* Destroy */

/*
 ************************************************************
 *
 *  GeometryManager - This function is called when a button child
 *	wants to resize itself; the current policy is to allow
 *	the requested resizing.
 *
 *********************function*header************************
 */

static XtGeometryResult GeometryManager(Widget widget, XtWidgetGeometry *request, XtWidgetGeometry *reply)
{
	CheckBoxWidget cb;
	ButtonWidget lw;
	XtGeometryResult xtgeometryresult=XtGeometryYes;

	cb = (CheckBoxWidget) XtParent(widget);

	if ((request->height==(Dimension)0 &&
	     (request->request_mode & CWHeight)) ||
	    (request->width==(Dimension)0 &&
	     (request->request_mode & CWWidth))) {

	OlWarning(dgettext(OlMsgsDomain,
		"CheckBox: Height and/or width must be greater than 0"));
 
		return XtGeometryNo;
	}

	ResizeSelf(cb,TRUE);		/* does geometry request too */


					/* this is the string or image */
	if(widget == cb->checkBox.label_child) {
		lw= (ButtonWidget) cb->checkBox.label_child;
		reply->height= lw->core.height;
		reply->width= lw->core.width;
		if(reply->height==request->height 
			&& reply->width==request->width) {

			xtgeometryresult = XtGeometryYes;
		}
		else 	xtgeometryresult = XtGeometryAlmost;
	}			

	reply->request_mode=CWWidth | CWHeight;
	if(xtgeometryresult == XtGeometryYes)
		OlWidgetConfigured(widget);
	return xtgeometryresult;

} /* GeometryManager */

/*
 ************************************************************
 *
 *  GetValuesHook - This function gets values from the button
 *	widget label/image as needed.
 *
 *********************function*header************************
 */

static void 
GetValuesHook(Widget w, ArgList args, Cardinal *num_args)
{
	Cardinal          count;
	CheckBoxWidget	  cb = (CheckBoxWidget) w;
	ArgList	          new_list = (ArgList) NULL;
	static MaskArg    mask_list[] = {
		{ XtNlabel,	NULL,	OL_SOURCE_PAIR	},
		{ XtNfont,	NULL,	OL_SOURCE_PAIR	},
		{ XtNlabelImage,NULL,	OL_SOURCE_PAIR	}
	};

	_OlComposeArgList(args, *num_args, mask_list, XtNumber(mask_list),
			 &new_list, &count);

	if (cb->checkBox.label_child != (Widget) NULL && count > (Cardinal)0) {
		XtGetValues(cb->checkBox.label_child, new_list, count);
		XtFree((char *)new_list);
	}

} /* GetValuesHook */


/*
 *************************************************************************
 * HighlightHandler - changes the colors when this widget gains or loses
 * focus.
 ****************************procedure*header*****************************
 */
static void
HighlightHandler (Widget w, OlDefine type)
{
	GetAttrs ((CheckBoxWidget) w);
	Redisplay (w, NULL, NULL);
} /* END OF HighlightHandler() */


/*
 ************************************************************
 *
 *  Initialize - This function checks that checkBox variable
 *	values are within range and initializes private fields
 *
 *********************function*header************************
 */

/* ARGSUSED */
static void 
Initialize(CheckBoxWidget request, CheckBoxWidget new, ArgList args, Cardinal *num_args)
{
	Widget ewidget,parent;
	CorePart *cp;
	CheckBoxPart *ep;
	ManagerPart	*mp;
	Arg arg[10];
	int n,n1;
	OlStr label;
	int validScale;

	ewidget= (Widget) new;
	cp = &(new->core);
	ep = &(new->checkBox);
	mp = &(new->manager);
	parent= XtParent(new);

	/* ******************************************* */
	/* check that core values correct or not reset */
	/* ******************************************* */

	if (cp->height==(Dimension)0) {
		cp->height=(Dimension)1;
	}

	if (cp->width==(Dimension)0) {
		cp->width=(Dimension)1;
	}

	if (cp->border_width!=(Dimension)0) {
		cp->border_width=(Dimension)0;
	}

	if (cp->background_pixel!=parent->core.background_pixel) {
		cp->background_pixel=parent->core.background_pixel;
	}

	if (cp->background_pixmap!=parent->core.background_pixmap) {
		cp->background_pixmap=parent->core.background_pixmap;
	}
	
	if (ep->dim == TRUE)
		cp->sensitive =  FALSE;

	/* Ensure scale is valid for OLGX */
	validScale = OlgxGetValidScale(ep->scale);
	ep->scale = validScale;

	ep->pAttrs = NULL;
	GetAttrs (new);

	if (ep->labeljustify!=(OlDefine)OL_LEFT 
		&& ep->labeljustify!=(OlDefine) OL_RIGHT ) {
		ep->labeljustify= (OlDefine) OL_LEFT;
		OlWarning(dgettext(OlMsgsDomain,
		"XtNlabelJustify must be OL_LEFT or OL_RIGHT: OL_LEFT used"));
	}

	if (ep->position!=(OlDefine)OL_LEFT 
		&& ep->position!=(OlDefine)OL_RIGHT ) {
		ep->position=(OlDefine)OL_LEFT;
		OlWarning(dgettext(OlMsgsDomain,
			"XtNposition must be OL_LEFT or OL_RIGHT: OL_LEFT used"));
	}

					/* make the label child */
	ep->label_child=(Widget) NULL;

	n=0;

	if (ep->labeltype!=(OlDefine)OL_STRING 
		&& ep->labeltype!=(OlDefine) OL_IMAGE ) {
		ep->labeltype=(OlDefine) OL_STRING;
		OlWarning(dgettext(OlMsgsDomain,
			"XtNlabelType: OL_STRING or OL_IMAGE: OL_STRING used"));
	}

	XtSetArg(arg[n],XtNlabelType,(XtArgVal)ep->labeltype);
	n++;

	if (ep->labelimage!=(XImage *)NULL) {
		XtSetArg(arg[n],XtNlabelImage,(XtArgVal)ep->labelimage);
		n++;
	}
	XtSetArg(arg[n],XtNlabelTile,(XtArgVal)ep->labeltile);
	n++;

	XtSetArg(arg[n],XtNbackground,(XtArgVal) cp->background_pixel);
	n++;

	if(cp->background_pixmap!=XtUnspecifiedPixmap) {
	XtSetArg(arg[n],XtNbackgroundPixmap,(XtArgVal)cp->background_pixmap);
	n++;
	}

	if(ep->font!=(OlFont)NULL) {
	XtSetArg(arg[n],XtNfont,(XtArgVal)ep->font);
	n++;
	}

	XtSetArg(arg[n],XtNfontColor,ep->fontcolor);
	n++;
	XtSetArg(arg[n],XtNtextFormat,ep->text_format);
	n++;

	n1=n-1;
    	ep->label_child = 
	XtCreateManagedWidget(cp->name,buttonWidgetClass,ewidget,arg,n);

			/* Delete this button Widget from the traversal
			 * list
			 */

	_OlDeleteDescendant(ep->label_child);

	ep->setvalue = ep->set;	/* to track press-drag-release behavior */
	ep->preview_state = (Boolean)False;

		/* add me to the traversal list */
	_OlUpdateTraversalWidget(ewidget, mp->reference_name,
				 mp->reference_widget, True);

/*
 * For mnemonics and accelerators, do SetValues() on button child 
 * to check validity of values and modify label display but then 
 * move  to checkBox since this is the widget that should be fired.
 */

	if(ep->label!=(OlStr)NULL) {
		label=ep->label;
	}
	else {
		if(ep->text_format != OL_WC_STR_REP)
			label=(OlStr)cp->name;
		else {
			wchar_t *ws;
			
			ws = (wchar_t *)XtMalloc((strlen(cp->name)+1)*
					sizeof(wchar_t));
			mbstowcs(ws,cp->name,strlen(cp->name)+1);
			label = (OlStr)ws;
		}
			
	}

	XtVaSetValues( ep->label_child,
		XtNmnemonic, (XtArgVal) ep->mnemonic,
		XtNaccelerator, (XtArgVal) ep->accelerator,
		XtNacceleratorText, (XtArgVal) ep->accelerator_text,
		XtNlabel, (XtArgVal) label, 
		NULL);

	XtVaGetValues( ep->label_child,
		XtNmnemonic, (XtArgVal) &ep->mnemonic,
		XtNaccelerator, (XtArgVal) &ep->accelerator,
		XtNacceleratorText, (XtArgVal) &ep->accelerator_text,
		NULL);

	if(ep->mnemonic!=(OlMnemonic) 0) {
		_OlRemoveMnemonic(ep->label_child,
				(XtPointer)0, FALSE, ep->mnemonic);
		_OlAddMnemonic((Widget) new, 
				(XtPointer)NULL, ep->mnemonic);

	}
		
	if(ep->accelerator!=(String) NULL) {
		ep->accelerator = XtNewString(ep->accelerator);
		_OlRemoveAccelerator(ep->label_child,
				(XtPointer)0, FALSE, ep->accelerator);
		_OlAddAccelerator((Widget) new, 
			(XtPointer)NULL, ep->accelerator);
	}
		
	if(ep->accelerator_text!=(String) NULL)
		ep->accelerator_text = XtNewString(ep->accelerator_text);

	/* reset fields necessary: note that these lines must be last */

	if(ep->label!=(OlStr)NULL) ep->label= (OlStr)NULL;
	if(ep->labelimage!=(XImage *)NULL) ep->labelimage=(XImage *)NULL;
	if(ep->font!=(OlFont)NULL) ep->font=(OlFont)NULL;

	ResizeSelf(new,TRUE);

} 	/* Initialize */

/*
 ************************************************************
 *
 *  InsertChild - This function prevents other children
 *	from being added --- needed since checkBox a composite widget.
 *
 *********************function*header************************
 */

static void
InsertChild(Widget widget)
{
    CheckBoxWidget	pw = (CheckBoxWidget) XtParent(widget);
    XtWidgetProc	insert_child = ((CompositeWidgetClass)
	(checkBoxClassRec.core_class.superclass))->composite_class.insert_child;

    if(pw->checkBox.label_child!=(Widget)NULL) {

	OlWarning(dgettext(OlMsgsDomain,
	"No other widgets can be added:\n\tdestroyed and results unpredictable"));
	XtDestroyWidget(widget);
	return;
    }

	 if (!insert_child) {
 		OlError(dgettext(OlMsgsDomain,
			"CheckBox: no InsertChild function."));
 		return;
	}

	(*insert_child)(widget);

    XtSetMappedWhenManaged(widget,TRUE); /* default but do it anyhow */

	_OlDeleteDescendant(widget);	/* remove button from traversal list */

}	/* InsertChild */

/*
 ************************************************************
 *
 *  QueryGeometry - This function is a rather rigid prototype
 *	since the widget insists upon its core height and width.
 *
 ************************************************************
*/

static XtGeometryResult QueryGeometry(Widget widget, XtWidgetGeometry *request, XtWidgetGeometry *preferred)
{
	CheckBoxWidget cb;

	cb= (CheckBoxWidget) widget;

		/* if not height or width it is okay with widget */

	if(!(request->request_mode & CWHeight 
			|| request->request_mode & CWWidth)) {

		return XtGeometryYes;
	}

		/* if still here look at requested height and/or width */

	if((request->request_mode & CWHeight) 
			&& (request->height!=cb->checkBox.normal_height)) {

			preferred->request_mode |=CWHeight;
			preferred->height=cb->checkBox.normal_height;
	};
		
	if((request->request_mode & CWWidth)
		&& (request->width!=cb->checkBox.normal_width)) {

		preferred->request_mode |=CWWidth;
		preferred->width=cb->checkBox.normal_width;
	};

	if((preferred->request_mode & CWHeight)
		|| (preferred->request_mode & CWWidth)) {

		return XtGeometryAlmost;
	}

	else return XtGeometryNo;

} /* QueryGeometry */

/*
 ************************************************************
 *
 *  Realize - This function realizes the CheckBox widget in
 *	order to be able to set window properties
 *
 *********************function*header************************
 */

static void 
Realize(Widget widget, Mask *ValueMask, XSetWindowAttributes *attributes)
{
	CheckBoxWidget cb = (CheckBoxWidget) widget;
	Widget label_child = cb->checkBox.label_child;

	(* (checkBoxClassRec.core_class.superclass->core_class.realize))
		(widget, ValueMask, attributes);

	XtRealizeWidget(label_child);
					/* checkBox to get select events */
	XtUninstallTranslations(label_child); 

} /* Realize */

/*
 ************************************************************
 *
 *  Resize - This function warns when checkBox truncated.
 *
 *********************function*header************************
 */

static void 
Resize(Widget w)
{
	CheckBoxWidget cb = (CheckBoxWidget) w;

	ResizeSelf(cb,FALSE);		/* do not make geometry request */

} 	/* Resize */

/*
 ************************************************************
 *
 *  Redisplay - This function is needed to redraw a 
 *		check once the widget is being mapped.
 *
 *********************function*header************************
 */

static void 
Redisplay(Widget widget, XEvent *event, Region region)
{
	DrawBox(widget);

}	/* Redisplay */

/*
 ************************************************************
 *
 *  ResizeSelf - This function calculates the size needed for the
 *	CheckBox widget, as a function of children's sizes,
 *	and repositions the latter accordingly.
 *
 *********************function*header************************
 */

static void 
ResizeSelf(CheckBoxWidget ew, Boolean greq)
{
	XtWidgetGeometry	request,
				reply;
	ButtonWidget		label;
	Widget			widget,
				parent;
	Position		xlabel,
				ylabel,
				xbox,
				ybox;
	Dimension		hbox,
				wbox;
	int			wlabel,
				hlabel,
				left_space,
				center_space,
				right_space,
				top_space,
				bottom_space,
				wcheckbox,
				maxheight;

	widget = (Widget) ew;
	parent = XtParent(widget);

	label = (ButtonWidget) ew->checkBox.label_child;

	wlabel= (int)label->button.normal_width;
	hlabel= (int)label->button.normal_height;
	left_space = 1;
	right_space = 3;
	top_space = 3;
	bottom_space = 1;
	center_space = label->primitive.scale;

	wbox = (Dimension)(CheckBox_Width(ew->checkBox.pAttrs->ginfo));
	hbox = (Dimension)(CheckBox_Height(ew->checkBox.pAttrs->ginfo));

	if((Dimension)hlabel >= hbox)
		maxheight=hlabel;
	else
		maxheight=hbox;

	ew->checkBox.normal_height = 
			(Dimension) (top_space + maxheight + bottom_space) ;

	ew->checkBox.normal_width =
			(Dimension) (left_space + wlabel 
			    + center_space + wbox + right_space);

	if(greq) {			/* geometry request */
	    /*
	     * If XtNrecomputeSize was set to False, use core dimensions
	     */
	    if (ew->checkBox.recompute_size) {
		request.height = ew->checkBox.normal_height;
		request.width = ew->checkBox.normal_width;
	    } else {
		request.height = ew->core.height;
		request.width = ew->core.width;
	    }

    	    request.request_mode = CWHeight | CWWidth ;

	    switch(XtMakeGeometryRequest((Widget)ew,&request,&reply)) {
		case XtGeometryDone:
			break;
		case XtGeometryYes:
			break;
		case XtGeometryAlmost:
			XtMakeGeometryRequest((Widget)ew,&reply,&reply);
			break;
		case XtGeometryNo:
			break;
	    }

	} /* if geometry request */

				/* nonexclusives sizes its own children */
	if(XtIsSubclass(parent,nonexclusivesWidgetClass) || !ew->checkBox.recompute_size) 
		wcheckbox=(int)ew->core.width;
	else 			/* assume normal width and clip */
		wcheckbox=(int)ew->checkBox.normal_width;

				/* compute label and box position */

	if(ew->checkBox.labeljustify==(OlDefine)OL_LEFT
			&&	ew->checkBox.position==OL_LEFT) {

		xbox = (Position) ( wcheckbox - (wbox + right_space));
		xlabel= (Position) left_space;
		if ((xlabel + wlabel) > (xbox - center_space))
		    wlabel = (Dimension) xbox - center_space - xlabel;
	}
	else if(ew->checkBox.labeljustify==(OlDefine)OL_RIGHT
			&&	ew->checkBox.position==(OlDefine)OL_LEFT) {

		xbox = (Position) ( wcheckbox - (wbox + right_space));
		xlabel= (Position) (xbox - (center_space + wlabel));
		if ( wlabel > (xbox - center_space) ) {
		    xlabel = (Position) left_space;
		    wlabel = (Dimension) xbox - center_space;
		}
	}
	else if(ew->checkBox.labeljustify==(OlDefine)OL_LEFT 
			&& ew->checkBox.position==(OlDefine)OL_RIGHT) {
		xbox = (Position) left_space;
		xlabel= (Position) (left_space + wbox + center_space);
		if ((xlabel + wlabel) > (wcheckbox))
                   wlabel = (Dimension) wcheckbox - (xlabel);
	}
	else if(ew->checkBox.labeljustify==(OlDefine)OL_RIGHT 
			&& ew->checkBox.position==(OlDefine)OL_RIGHT) {
		int width;
		xbox = (Position) left_space;
		xlabel= (Position) (wcheckbox -( right_space + wlabel)) ;
                if ((width = (int)((int)xbox + (int)wbox + center_space + wlabel))  > wcheckbox) {
                   xlabel = (Position) xbox + wbox + center_space;
                   wlabel = (Dimension) wcheckbox - xlabel;
		}
	}

	ylabel = (Position)
		((int)(ew->checkBox.normal_height 
				- label->button.normal_height) / (int)2);
	ybox = (Position) 
		(((int)((int)(ew->checkBox.normal_height) - hbox)) / (int)2);

			/* Note: for font, y is at baseline */

	ew->checkBox.x1 = xbox;
	ew->checkBox.x2 = xbox + (Position) wbox;
	ew->checkBox.y1 = ybox -(Position) 2;
	ew->checkBox.y2 = ybox + (Position) hbox -(Position)2;

	OlConfigureWidget(ew->checkBox.label_child,
		xlabel, ylabel,
		wlabel,
		hlabel, (Dimension)0);

} /* ResizeSelf */

/*
 ************************************************************
 *
 *  SetValues - This function checks and allows setting and
 *	resetting of CheckBox resources.
 *
 *********************function*header************************
 */

/* ARGSUSED */
static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	int oldvalue,newvalue;
	OlDefine olddefine,newdefine;
	Boolean needs_redisplay=FALSE;
	Widget label,parent;
	ButtonWidget lw,oldlw;
	Arg arg;
	XSetWindowAttributes attributes;
	Boolean Grequest=FALSE, reposition_children=FALSE;

	CheckBoxWidget currew =	(CheckBoxWidget) current;
	CheckBoxWidget newew =	(CheckBoxWidget) new;

	parent=XtParent(current);
	label=newew->checkBox.label_child;
	lw= (ButtonWidget) label;

	/* ******************************************* */
	/* check that core values correct or not reset */
	/* ******************************************* */

	if (newew->core.height==(Dimension)0) {
		newew->core.height=(Dimension)1;
		needs_redisplay=TRUE;
	}

	if (newew->core.width==(Dimension)0) {
		newew->core.width=(Dimension)1;
		needs_redisplay=TRUE;
	}

	if (newew->core.border_width!=(Dimension)0) {
		newew->core.border_width=(Dimension)0;
		needs_redisplay=TRUE;
	}

	if (newew->core.background_pixel!=parent->core.background_pixel) {
		newew->core.background_pixel=parent->core.background_pixel;
		if (XtIsRealized((Widget)newew)) {
		attributes.background_pixel=parent->core.background_pixel;
		XChangeWindowAttributes(XtDisplay(newew),XtWindow(newew),
			CWBackPixel,&attributes);
		needs_redisplay=TRUE;
		}
	}
	
		/* 	this is a kludgy check: in case parent's and
			widget's background pixel have both been changed,
			need to change label background */
		
	if(lw->core.background_pixel!=newew->core.background_pixel) {
		XtSetArg(arg,XtNbackground,newew->core.background_pixel);
		XtSetValues(label,&arg,1);
		needs_redisplay=TRUE;
	}

	if (newew->core.background_pixmap!=parent->core.background_pixmap) {
	     newew->core.background_pixmap=parent->core.background_pixmap;
	     if (XtIsRealized((Widget)newew)) {
		if (newew->core.background_pixmap!=XtUnspecifiedPixmap)
		{
		  attributes.background_pixmap=newew->core.background_pixmap;
		  XChangeWindowAttributes(XtDisplay(newew),XtWindow(newew),
			CWBackPixmap,&attributes);
		}
		else {
		  attributes.background_pixel=newew->core.background_pixel;
		  XChangeWindowAttributes(XtDisplay(newew),XtWindow(newew),
					CWBackPixel,&attributes);
		}
		needs_redisplay=TRUE;
	     }
	}

		/* 	this is also a kludgy check: in case parent's and
			widget's background pixmap have both been changed,
			need to change label background pixmap */
		
	if(lw->core.background_pixmap!=newew->core.background_pixmap) {
		if(newew->core.background_pixmap!=XtUnspecifiedPixmap) {

		XtSetArg(arg,XtNbackgroundPixmap,newew->core.background_pixmap);
		XtSetValues(label,&arg,1);
		 /* Hack: XtSetVals on Label widget's bgPixmap  does'nt
	  	    generate Expose events on it. Hence we force
	            Expose events from the Server      -JMK
		 */
		XClearArea(XtDisplay(label),XtWindow(label),0,0,0,0,TRUE); 
		}
		else {
		    Arg narg[2];
		    XtSetArg(narg[0],XtNbackgroundPixmap,XtUnspecifiedPixmap); 
		    XtSetArg(narg[1],XtNbackground,newew->core.background_pixel); 
		    XtSetValues(label,narg,2);
		}
		needs_redisplay=TRUE;
	}

	/* XtNforeground */

	if((newew->checkBox.foreground!=currew->checkBox.foreground) ||
	  (newew->core.background_pixel!=currew->core.background_pixel) ||
	    (newew->checkBox.scale != currew->checkBox.scale)) {
	        OlgxBG	bg;
		Boolean	bgPixmapType;
		CorePart *cp = &newew->core;
		int     validScale = OlgxGetValidScale(newew->checkBox.scale);
		newew->checkBox.scale = validScale;

		GetAttrs (newew);
		needs_redisplay=TRUE;
	}

	/* XtNfont */

	if(newew->checkBox.font !=(OlFont)NULL) {
		XtSetArg(arg,XtNfont,newew->checkBox.font);
		XtSetValues(label,&arg,1);
		newew->checkBox.font =(OlFont)NULL;
		Grequest=TRUE;
	}

	/* XtNfontColor */

	if(newew->checkBox.fontcolor!=currew->checkBox.fontcolor) {
		XtSetArg(arg,XtNfontColor,newew->checkBox.fontcolor);
		XtSetValues(label,&arg,1);
	}

	/* XtNlabelTile */

	if(newew->checkBox.labeltile!=currew->checkBox.labeltile) {
		XtSetArg(arg,XtNlabelTile,newew->checkBox.labeltile);
		XtSetValues(label,&arg,1);
	}
	
	/* ******************************** */
	/* check checkBox private resources */
	/* ******************************** */

	/* XtNselect resource */
	/* XtNunselect resource */

		/* intrinsics does work here */

	/* XtNset resource */

	oldvalue=currew->checkBox.set;
	newvalue=newew->checkBox.set;

	if((newvalue==FALSE && oldvalue!=FALSE)
		|| (newvalue!=FALSE && oldvalue==FALSE)) {

		needs_redisplay=TRUE;
		/*
		 * If the checkbox is not currently being previewed,
		 * i.e. this XtSetValues is from an application as opposed
		 * to from PreviewState, then we make 'setvalue' equal to
		 * 'set' to ensure that the state is correct (equivalent to
		 * if the user had clicked on the checkbox) and subsequent
		 * previewing works correctly.
		 */
		if (!newew->checkBox.preview_state)
		    newew->checkBox.setvalue = newew->checkBox.set;
	}

	/* XtNsensitive resource */

	if (XtIsSensitive(new) != XtIsSensitive(current))
		needs_redisplay=TRUE;

	/* XtNdim resource */

	oldvalue=currew->checkBox.dim;
	newvalue=newew->checkBox.dim;

	if (newvalue != oldvalue)
	{
		needs_redisplay = TRUE;
		XtSetSensitive((Widget)newew, (newvalue)? FALSE : TRUE);
	}

	/* XtNlabel resource */

	if (newew->checkBox.label!= (char *) NULL) { 
		XtSetArg(arg,XtNlabel,newew->checkBox.label);
		XtSetValues(newew->checkBox.label_child,&arg,1);
		newew->checkBox.label=(char *)NULL;
		Grequest=TRUE;
		reposition_children=TRUE;
	}

	/* XtNlabelJustify resource */

	olddefine=currew->checkBox.labeljustify;
	newdefine=newew->checkBox.labeljustify;
	if(newdefine!=olddefine) {
		if(newdefine==(OlDefine)OL_LEFT 
			|| newdefine==(OlDefine)OL_RIGHT) {
			reposition_children=TRUE;
		}

		else {
			newew->checkBox.labeljustify=olddefine;
			OlWarning(dgettext(OlMsgsDomain,
				"XtNlabelJustify illegal value: not changed"));
		}
	}

	/* XtNposition resource */

	olddefine=currew->checkBox.position;
	newdefine=newew->checkBox.position;
	if(newdefine!=olddefine) {
		if(newdefine==(OlDefine)OL_LEFT 
			|| newdefine==(OlDefine)OL_RIGHT) {

			reposition_children=TRUE;
		}
		else {
			newew->checkBox.position=olddefine;
			OlWarning(dgettext(OlMsgsDomain,
				"XtNposition illegal value: not changed"));
		}
	}

	/* XtNlabelType resource */

	olddefine=currew->checkBox.labeltype;
	newdefine=newew->checkBox.labeltype;

	if(newdefine!=olddefine) {
		if(newdefine==(OlDefine)OL_STRING  
			|| newdefine==(OlDefine)OL_IMAGE) {
			XtSetArg(arg,XtNlabelType,newew->checkBox.labeltype);
			XtSetValues(newew->checkBox.label_child,&arg,1);
		}
		else {
			newew->checkBox.labeltype=olddefine;
			OlWarning(dgettext(OlMsgsDomain,
				"XtNlabelType illegal value: not changed"));
		}
	
	}

	/* XtNlabelImage resource */

	if(newew->checkBox.labelimage!=(XImage *)NULL) {
		XtSetArg(arg,XtNlabelImage,
				newew->checkBox.labelimage);
		XtSetValues(label,&arg,1);
		newew->checkBox.labelimage=(XImage *)NULL;
	}

	/* XtNrecomputeSize resource */

	oldvalue=currew->checkBox.recompute_size;
	newvalue=newew->checkBox.recompute_size;

	if(newvalue!=oldvalue) {
		if((newvalue==FALSE && oldvalue!=FALSE)
			|| (newvalue!=FALSE && oldvalue==FALSE)) {
			Grequest=TRUE;
		}
	}

	/* XtNdefaultData resource */

		/* let widget writer set and get any pointer needed 
		   for updating "cloned" menus - intrinsics does work here */

	/* XtNmnemonic resource */

	if(newew->checkBox.mnemonic!=currew->checkBox.mnemonic) {

	OlMnemonic mnemonic = (OlMnemonic) 0;

	XtSetArg(arg,XtNmnemonic, (XtArgVal) newew->checkBox.mnemonic);
	XtSetValues(label,&arg,1); 
	XtSetArg(arg,XtNmnemonic, &mnemonic);
	XtGetValues(label,&arg,1);
		
	if(newew->checkBox.mnemonic==mnemonic)
	{
		_OlRemoveMnemonic(label,
			(XtPointer)0, FALSE, newew->checkBox.mnemonic);
		_OlRemoveMnemonic(new,
			(XtPointer)0, FALSE, currew->checkBox.mnemonic);
		_OlAddMnemonic(new,
				(XtPointer)NULL, newew->checkBox.mnemonic);
		Grequest = TRUE;
		needs_redisplay = TRUE;
	}
	else
		newew->checkBox.mnemonic = (OlMnemonic)0;
	}
	
{
#define CHANGED(field)	(newew->checkBox.field != currew->checkBox.field)

	Boolean changed_accelerator = CHANGED(accelerator);
	Boolean changed_accelerator_text = CHANGED(accelerator_text);

	/* XtNaccelerator resource */

	if(changed_accelerator) {

	String accelerator = (String) NULL;

	XtSetArg(arg,XtNaccelerator, (XtArgVal) newew->checkBox.accelerator);
	XtSetValues(label,&arg,1);
	XtSetArg(arg,XtNaccelerator, &accelerator);
	XtGetValues(label,&arg,1);
		
	if(accelerator!=(String)NULL) {

		_OlRemoveAccelerator(label,
			(XtPointer)0, FALSE, newew->checkBox.accelerator);
		_OlRemoveAccelerator(new,
			(XtPointer)0, FALSE, currew->checkBox.accelerator);
		_OlAddAccelerator(new,
				(XtPointer)NULL, newew->checkBox.accelerator);
		XtFree(currew->checkBox.accelerator);
		newew->checkBox.accelerator = 
			XtNewString(newew->checkBox.accelerator);
		Grequest = TRUE;
		needs_redisplay = TRUE;
	}
	else
		newew->checkBox.accelerator = (String) NULL;
	}

	/* XtNacceleratorText resource */

	if(changed_accelerator || changed_accelerator_text) {

	String accelerator_text = (String) NULL;

	XtFree(currew->checkBox.accelerator_text);
	currew->checkBox.accelerator_text = (String)NULL;

	if(changed_accelerator && !changed_accelerator_text)
			newew->checkBox.accelerator_text = (String)NULL;

	XtSetArg(arg,XtNacceleratorText, 
		(XtArgVal) newew->checkBox.accelerator_text);
	XtSetValues(label,&arg,1);
	XtSetArg(arg,XtNacceleratorText, &accelerator_text);
	XtGetValues(label,&arg,1);
		
	if(accelerator_text!=(String)NULL) {
		newew->checkBox.accelerator_text = 
			XtNewString(newew->checkBox.accelerator_text);
	}
	else 
		newew->checkBox.accelerator_text = (String) NULL;

	Grequest = TRUE;
	needs_redisplay = TRUE;
	}
#undef CHANGED
}
	/* do not make geometry request - intrinsics will request */

	if(reposition_children || Grequest) {
		ResizeSelf(newew,FALSE);  

		if(reposition_children) {
			needs_redisplay=TRUE;
		}

			/* nonexclusives sizes its own children */
		if(Grequest && 
		   !XtIsSubclass(parent,nonexclusivesWidgetClass) &&
		   newew->checkBox.recompute_size){ 
			newew->core.width=newew->checkBox.normal_width;
			newew->core.height=newew->checkBox.normal_height;
		}
	}

	return needs_redisplay;

}	/* SetValues */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 ************************************************************
 *
 *  CheckBoxHandler - this function is called by OPEN LOOK
 *	for requested events.
 *
 *********************function*header************************
 */

static void
    CheckBoxHandler(Widget w, OlVirtualEvent ve)
{
    
    Boolean activate=FALSE;
    
    switch(ve->xevent->type) {
	
    case EnterNotify:
    case LeaveNotify:
	if(ve->virtual_name==OL_SELECT
	   || ve->virtual_name==OL_MENU) {
	    ve->consumed =
		PreviewState(w,ve->xevent,ve->virtual_name,activate);
	}
	break;
	
    case ButtonPress:
	(void) _OlUngrabPointer(w);
	if(ve->virtual_name==OL_SELECT) {
	    ve->consumed =
		PreviewState(w,ve->xevent, ve->virtual_name,
			     activate);
	}
	else if (ve->virtual_name == OL_MENU) {
	    Widget shell = _OlGetShellOfWidget(w);
	    
	    if (shell != (Widget)NULL &&
		XtIsSubclass(shell, menuShellWidgetClass) == True) {
		ve->consumed = PreviewState(w,ve->xevent,ve->virtual_name,
					activate);
	    }
	}
	break;
	
    case ButtonRelease:
	if(ve->virtual_name==OL_SELECT) {
	    ve->consumed =
		SetState(w,ve->xevent,ve->virtual_name,
			 activate);
	}
	else if (ve->virtual_name == OL_MENU) {
	    Widget shell = _OlGetShellOfWidget(w);
	    
	    if (shell != (Widget)NULL &&
		XtIsSubclass(shell, menuShellWidgetClass) == True) {
		ve->consumed = SetState(w,ve->xevent,ve->virtual_name,
					activate);
	    }
	}
	break;
    }
} /* CheckBoxHandler */

/*
 ************************************************************
 *
 * MotionHandler: temporary code
 *
 *********************function*header************************
 */

static void MotionHandler (Widget w, OlVirtualEvent ve)
{
    static Boolean activate = FALSE;
    
    switch (ve->virtual_name)
	{
	case OL_SELECT:
	    ve->consumed = True;
	    PreviewState(w,ve->xevent,ve->virtual_name,activate);
	    break;
	case OL_MENU:
	    {
		Widget shell = _OlGetShellOfWidget(w);
		
		if (shell != (Widget)NULL &&
		    XtIsSubclass(shell, menuShellWidgetClass) == True) {
		    ve->consumed = True;
		    PreviewState(w, ve->xevent, ve->virtual_name,
				 activate);
		}
		break;
	    }
	}
} /* MotionHandler */

/*
 ************************************************************
 *
 *  PreviewState - this function is called on a select button 
 *	down event.  It previews the appearance of the button if it
 *	were to be selected or unselected.
 *
 *********************function*header************************
 */

static Boolean
PreviewState(Widget w, XEvent *event, OlVirtualName vname, Boolean activate)
{
	CheckBoxWidget cb = (CheckBoxWidget) w;
	Arg arg;
	XCrossingEvent *xce;
	XButtonEvent *xbe;
	XMotionEvent *xme;
	Position x,y;
	Boolean point_in_box=FALSE;
	Boolean toggle=FALSE;

	if(!XtIsSensitive(w) )
		return TRUE;

	if(!activate) {			/* filter out pointer crossings */

	if(event->type==EnterNotify || event->type==LeaveNotify) {
		xce = (XCrossingEvent *) &(event->xcrossing); 
		if(xce->mode!=NotifyNormal) return FALSE;
	}
	}

	(void) _OlUngrabPointer(w);

	if(!activate && event->type==ButtonPress) {
		xbe = (XButtonEvent *) &(event->xbutton);
		x= xbe->x;
		y= xbe->y;
		if(PointInBox(w,x,y,activate)) {
			toggle=TRUE;
			cb->checkBox.preview_state = (Boolean)TRUE;
		}
	}
	else if(activate)
		toggle=TRUE;

	
	else if(event->type == MotionNotify) {
		xme = (XMotionEvent *) &(event->xmotion);
		x= xme->x;
		y= xme->y;
		point_in_box = PointInBox(w,x,y,activate); 

		if (point_in_box && (cb->checkBox.setvalue==cb->checkBox.set)) {
			 cb->checkBox.preview_state = (Boolean)TRUE;
			 toggle = TRUE;
		}
		if (!point_in_box && (cb->checkBox.setvalue!=cb->checkBox.set)) {
			cb->checkBox.preview_state = (Boolean)FALSE;
			toggle = TRUE;
		}
	}

	else if(event->type == EnterNotify) {
		if (point_in_box && cb->checkBox.setvalue==cb->checkBox.set) {
			 cb->checkBox.preview_state = (Boolean)TRUE;
			 toggle = TRUE;
		}
		if (!point_in_box && cb->checkBox.setvalue!=cb->checkBox.set) {
			cb->checkBox.preview_state = (Boolean)TRUE;
			toggle = TRUE;
		}
	}
	else if(event->type == LeaveNotify) {
		if(cb->checkBox.setvalue!=cb->checkBox.set) {
			cb->checkBox.preview_state = (Boolean)FALSE;
			toggle = TRUE;
		}
	}

	if(toggle) {
		if(cb->checkBox.set==FALSE) {
			XtSetArg(arg, XtNset, TRUE);
			XtSetValues(w, &arg, 1);
			return TRUE;
		}
		XtSetArg(arg, XtNset, FALSE);
		XtSetValues(w, &arg, 1);
		return TRUE;
	}

	return FALSE;

}	/* PreviewState */

 /*
  ************************************************************
  *
  *  SetState - this function is called on a select button up
  *	event.  It invokes the users select and unselect callback(s) 
  *	and changes the state of the checkBox button from 
  *	set to unset or unset to set.
  *
  *********************function*header************************
  */
 
static Boolean 
SetState(Widget w, XEvent *event, OlVirtualName vname, Boolean activate)
{
 	CheckBoxWidget cb = (CheckBoxWidget) w;
 	Arg arg;
 	XButtonEvent *xbe;
 
 	if(!XtIsSensitive(w)) 
 		return FALSE;
 
	if(!activate && event->type!=ButtonRelease)
		return FALSE;

 	if(event->type==ButtonRelease) {
 		xbe = (XButtonEvent *) &(event->xbutton);
 		if(!PointInBox(w,(Position) xbe->x,(Position) xbe->y,activate))
 			return FALSE;
	}
 
	cb->checkBox.setvalue = cb->checkBox.set;

	if (!activate)
	    cb->checkBox.preview_state = False;

	if(!cb->checkBox.set) {
	    _OlManagerCallPreSelectCBs(w, event);

	    if(cb->checkBox.unselect!=(XtCallbackList)NULL)
		XtCallCallbacks(w,XtNunselect,(XtPointer)NULL);

	    _OlManagerCallPostSelectCBs(w, event);
	} 
 						/* here if unset */
	else {

	    if(cb->checkBox.dim) {
		XtSetArg(arg,XtNdim,FALSE);
		XtSetValues(w,&arg,1);
	    }

	    _OlManagerCallPreSelectCBs(w, event);

	    if(cb->checkBox.select!=(XtCallbackList)NULL)
		XtCallCallbacks(w,XtNselect,(XtPointer)NULL); 

	    _OlManagerCallPostSelectCBs(w, event);
	} 

	if (!activate) { /* We do not do visual previewing for
			     Activate events */
	    DrawBox(w);
	}

	return TRUE;

}	/* SetState */ 



/* Transparent Proc for CheckBox. We require this mainly because we 
 * cannot use bgPixmap == ParentRelative for rendering our "BOX"
 * and also we need to reconstruct the GCs, whenever there is a change
 * in the pixels/pixmaps from above the hierarchy .. 	-JMK
*/
static void
TransparentProc (Widget w, Pixel pixel, Pixmap pixmap)
{

	XSetWindowAttributes wattr;
	unsigned long valuemask;
	Display *display;
	Window window;
	CheckBoxWidget cb = (CheckBoxWidget)w;
	Widget label;
	OlTransparentProc proc;

	display = XtDisplayOfObject(w);
	window = XtWindowOfObject(w);

	if (pixmap == ParentRelative)	/* Need valid pixmap to render "BOX"*/
		pixmap = _OlGetRealPixmap(w);
	
	if (pixmap != XtUnspecifiedPixmap) {
		wattr.background_pixmap = pixmap;
		valuemask = CWBackPixmap;
	}
	else {
		wattr.background_pixel = pixel;
		valuemask = CWBackPixel;
	}

	w->core.background_pixel = pixel;
	w->core.background_pixmap = pixmap;
	GetAttrs((CheckBoxWidget)w);	/* Remake self's GCs */

	XChangeWindowAttributes(display, window, valuemask, &wattr);
	XClearArea(display, window, 0, 0, 0, 0, TRUE);

	label = cb->checkBox.label_child;
	if (proc = _OlGetTransProc(label))
		 (*proc)(label,pixel, (pixmap == XtUnspecifiedPixmap) ?
				 XtUnspecifiedPixmap : ParentRelative);
}

static void
CheckBoxQuerySCLocnProc(const Widget		w,
			const Widget		supercaret,
			const Dimension		sc_width,
			const Dimension		sc_height,
			unsigned int    *const	scale,	
			SuperCaretShape *const 	shape,
			Position        *const	x_center_return,
			Position        *const	y_center_return)
{
	Widget		fwp = XtParent(w);
	Dimension	fwp_width = fwp->core.width, 
			fwp_height = fwp->core.height;
	CheckBoxWidget	cb = (CheckBoxWidget)w;
	SuperCaretShape rs = *shape;
	Dimension  	wbox, hbox;
	
	*shape = (((int)fwp_width > (int)fwp_height)
				? SuperCaretBottom
				: SuperCaretLeft);

        if (cb->checkBox.scale != *scale || rs != *shape) {
                *scale = cb->checkBox.scale;
                return; /* try again */
        }

	wbox = (Dimension)(CheckBox_Width(cb->checkBox.pAttrs->ginfo));
	hbox = (Dimension)(CheckBox_Height(cb->checkBox.pAttrs->ginfo));

	*x_center_return = (Position)cb->checkBox.x1;
	*y_center_return = (Position)cb->checkBox.y1;

	switch (*shape) {
		case SuperCaretBottom:
			*x_center_return += wbox/2;
			*y_center_return += hbox + (sc_height / 6);
			break;
		case SuperCaretLeft:
			*x_center_return -= (int)(sc_width  / 6);
			*y_center_return += hbox / 2;
			break;
	}
}

	

 /*
  *************************************************************************
  *
  * Public Procedures
  *
  ****************************public*procedures****************************
  */
