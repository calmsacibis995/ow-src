#pragma ident	"@(#)Caption.c	302.19	97/03/26 lib/libXol SMI" /* caption:src/Caption.c 1.52 */

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


#include <ctype.h>
#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>
#include <widec.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>

#include <Xol/CaptionP.h>
#include <Xol/OlDnDVCX.h>
#include <Xol/OlI18nP.h>
#include <Xol/OpenLookP.h>


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

					/* private procedures */

static void	PositionTextAndChild(CaptionWidget w);

static void	SetTextWidthAndHeight(CaptionWidget cw);
static void	GetGCs(CaptionWidget cw);
static void	GetWidthAndHeight(CaptionWidget w, Dimension *pwidth, Dimension *pheight);
static void	DrawCaptionText(CaptionWidget cw);

					/* class procedures */

static void	Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);
static void	Redisplay(Widget w, XEvent *event, Region region);
static void	Destroy(Widget widget);
static Boolean	SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);
static void	ClassInitialize(void);
static XtGeometryResult	GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply);
static void	InsertChild(Widget w);
static void	ChangeManaged(Widget w);
static Boolean	ActivateWidget (Widget, OlVirtualName, XtPointer);

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define offset(field) XtOffset(CaptionWidget, field)

#define BYTE_OFFSET	XtOffsetOf(CaptionRec, caption.dyn_flags)
static _OlDynResource dyn_res[] = {
{ { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultForeground }, BYTE_OFFSET,
	 OL_B_CAPTION_FONTCOLOR, NULL },
};
#undef BYTE_OFFSET

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource resources[] = {
	/* This should be the first resource */
	{XtNtextFormat, XtCTextFormat, XtROlStrRep, sizeof(OlStrRep),
		offset(caption.text_format), XtRCallProc,
		(XtPointer)_OlGetDefaultTextFormat},

	{XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel),
	 	offset(caption.fontcolor), XtRString, XtDefaultForeground},

	{XtNfont,  XtCFont, XtROlFont, sizeof(OlFont),
		offset(caption.font),XtRString, (XtPointer)OlDefaultBoldFont},

	{XtNlabel, XtCLabel, XtROlStr, sizeof(OlStr),
		offset(caption.label), XtRImmediate, NULL},

	{XtNposition, XtCPosition, XtROlDefine, sizeof(OlDefine),
		offset(caption.position), XtRImmediate, (XtPointer)OL_LEFT},

	{XtNalignment, XtCAlignment, XtROlDefine, sizeof(OlDefine),
		offset(caption.alignment), XtRImmediate, (XtPointer)OL_CENTER},

	{XtNspace, XtCSpace, XtRDimension, sizeof(Dimension),
		offset(caption.space), XtRImmediate, (XtPointer)4},

	{XtNcaptionWidth, XtCCaptionWidth, XtRDimension, sizeof(Dimension),
		offset(caption.caption_width), XtRDimension, NULL},

	{XtNmnemonic, XtCMnemonic, OlRChar, sizeof(char),
		offset(caption.mnemonic), XtRImmediate, (XtPointer) '\0'},

	{XtNrecomputeSize, XtCRecomputeSize, XtRBoolean, sizeof(Boolean),
		offset(caption.recompute_size), XtRImmediate, (XtPointer)True},

};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

CaptionClassRec captionClassRec = {
  {
/* core_class fields */	
	/* superclass	  	*/	(WidgetClass) &managerClassRec,
	/* class_name	  	*/	"Caption",
	/* widget_size	  	*/	sizeof(CaptionRec),
	/* class_initialize   	*/	ClassInitialize,
	/* class_part_init	*/	NULL,
	/* class_inited?	*/	FALSE,
	/* initialize	  	*/	Initialize,
	/* initialize_hook	*/	NULL,
	/* realize	  	*/	XtInheritRealize,
	/* actions		*/	NULL,
	/* num_actions		*/	0,
	/* resources	  	*/	resources,
	/* num_resources	*/	XtNumber(resources),
	/* xrm_class	  	*/	NULLQUARK,
	/* compress_motion 	*/	TRUE,
	/* compress_exposure  	*/	TRUE,
	/* compress_enterleave	*/	TRUE,
	/* visible_interest	*/	FALSE,
	/* destroy	  	*/	Destroy,
	/* resize	  	*/	XtInheritResize,
	/* expose	  	*/	Redisplay,
	/* set_values	  	*/	SetValues,
	/* set_values_hook	*/	NULL,
	/* set_values_almost	*/	XtInheritSetValuesAlmost,
	/* get_values_hook	*/	NULL,
	/* accept_focus	 	*/	XtInheritAcceptFocus,
	/* version		*/	XtVersion,
	/* callback_private   	*/	NULL,
	/* tm_table	   	*/	XtInheritTranslations,
	/* query_geometr	*/	NULL,
	/* display_accelerator	*/	NULL,
	/* extension		*/	NULL
  },
  { /* composite_class fields */
	/* geometry_manager	*/	(XtGeometryHandler) GeometryManager,
	/* change_managed	*/	ChangeManaged,
	/* insert_child		*/	(XtWidgetProc) InsertChild,
	/* delete_child		*/	XtInheritDeleteChild,
	/* extension		*/	NULL
  },
  {/* constraint_class fields */
	/* resources		*/	(XtResourceList)NULL,
	/* num_resources	*/	0,
	/* constraint_size	*/	0,
	/* initialize		*/	(XtInitProc)NULL,
	/* destroy		*/	(XtWidgetProc)NULL,
	/* set_values		*/	(XtSetValuesFunc)NULL
  },
  {/* manager_class fields */
	/* highlight_handler	*/	NULL,
	/* reserved		*/	NULL,
	/* reserved		*/	NULL,
	/* traversal_handler	*/	NULL,
	/* activate		*/	ActivateWidget,
	/* event_procs		*/	NULL,
	/* num_event_procs	*/	0,
	/* register_focus	*/	NULL,
	/* reserved		*/	NULL,
	/* version		*/	OlVersion,
	/* extension		*/	NULL,
	/* dyn_data		*/	{ dyn_res, XtNumber(dyn_res) },
	/* transparent_proc	*/	_OlDefaultTransparentProc,
  }
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass captionWidgetClass = (WidgetClass)&captionClassRec;

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
 * SetTextWidthAndHeight - Calculate width and height of displayed text 
 * in pixels
 *
 ***************************private*procedures****************************
 */

static void 
SetTextWidthAndHeight(CaptionWidget cw)
{

	OlgxTextLbl	labeltext;
	CaptionPart	*cwp = &(cw->caption);

	labeltext.label = cwp->label;
	labeltext.text_format = cwp->text_format;
	labeltext.font = cwp->font;
	labeltext.qualifier = 0;
	labeltext.meta = False;
	labeltext.accelerator = 0;
	labeltext.mnemonic = cwp->mnemonic;
	labeltext.justification = 0;
	labeltext.flags = 0;

	OlgxSizeTextLabel (	XtScreen(cw),
				cwp->pAttrs,
				(XtPointer)&labeltext,
				&(cwp->caption_width), &(cwp->caption_height));
}

/*
 *************************************************************************
 *
 * GetGCs
 *
 ***************************private*procedures****************************
 */

static void 
GetGCs(CaptionWidget cw)
{
	XGCValues	values;
	XtGCMask	valueMask = GCForeground | GCBackground | GCFont;


	if(cw->caption.text_format == OL_SB_STR_REP) {
		valueMask = GCForeground | GCBackground | GCFont;
		values.font = ((XFontStruct *)cw->caption.font)->fid;
	} else 
		valueMask = GCForeground | GCBackground;

	values.foreground	= cw->caption.fontcolor;
	values.background	= cw->core.background_pixel;
	cw->caption.normal_GC = XtGetGC(
					(Widget)cw,
					valueMask,
					&values);

	values.foreground	= cw->core.background_pixel;
	values.background	= cw->caption.fontcolor;
	cw->caption.hilite_GC = XtGetGC(
					(Widget)cw,
					valueMask,
					&values);
}


/**************************************************************************
 *
 * PositionTextAndChild - Figure positions for text and child and put
 * them there.
 *
 **************************************************************************/

static void
PositionTextAndChild(CaptionWidget w)
{
    int textx, texty;
    Widget child;

    if (w->composite.num_children &&
	(child = w->composite.children[0], XtIsManaged(child))) {
	int childw = child->core.width + 2*child->core.border_width;
	int childh = child->core.height + 2*child->core.border_width;
	int childx, childy;

	switch (w->caption.position) {

	case OL_TOP:
	    texty = 0;
	    childy = w->caption.caption_height + w->caption.space;
	    goto J1;

	case OL_BOTTOM:
	    texty = childh + w->caption.space;
	    childy = 0;
	J1:
	    switch (w->caption.alignment) {
	    case OL_LEFT: textx = 0; break;
	    case OL_RIGHT: textx = childw-(int)w->caption.caption_width; break;
	    case OL_CENTER:
	    default: textx = (childw - (int)w->caption.caption_width)/2; break;
	    }
	    if (textx < 0) {childx = -textx; textx = 0;} else childx = 0;
	    break;

	case OL_RIGHT:
	    textx = childw + w->caption.space;
	    childx = 0;
	    goto J2;

	case OL_LEFT:
	default:
	    textx = 0;
	    childx = w->caption.caption_width + w->caption.space;
	J2:
	    switch (w->caption.alignment) {
	    case OL_TOP: texty = 0; break;
	    case OL_BOTTOM: texty= childh-(int)w->caption.caption_height;break;
	    case OL_CENTER:
	    default: texty = (childh-(int)w->caption.caption_height)/2;
		if (texty == 1) texty = 0; /* hack to match textLine */
		break;
	    }
	    if (texty < 0) {childy = -texty; texty = 0;} else childy = 0;
	    break;
	}

	_OlDnDSetDisableDSClipping(child, True);
	XtConfigureWidget(child, childx, childy, child->core.width,
			  child->core.height, child->core.border_width);
	_OlDnDSetDisableDSClipping(child, False);
	OlDnDWidgetConfiguredInHier(child);
    } else {textx = texty = 0;}

    w->caption.caption_x = textx;
    w->caption.caption_y = texty;
}

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

static void
ClassInitialize(void)
{
	_OlAddOlDefineType ("left",   OL_LEFT);
	_OlAddOlDefineType ("center", OL_CENTER);
	_OlAddOlDefineType ("right",  OL_RIGHT);
	_OlAddOlDefineType ("top",    OL_TOP);
	_OlAddOlDefineType ("bottom", OL_BOTTOM);
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
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	CaptionWidget cw = (CaptionWidget) new;
	char	*temp;
	wchar_t   *wtemp;
	int len;

	switch(cw->caption.text_format) {
		case OL_SB_STR_REP:
		case OL_MB_STR_REP:
		if (cw->caption.label != (OlStr)NULL) {
			temp = 
			(char *) XtMalloc(strlen(cw->caption.label)+1);
			strcpy( temp, (char *) cw->caption.label);
			cw->caption.label = (OlStr)temp;
		} else
			cw->caption.label = (OlStr)cw->core.name;
		break;
		case OL_WC_STR_REP:
		if (cw->caption.label != (OlStr)NULL) {
			wtemp = (wchar_t *) XtMalloc((wslen((wchar_t *)
				cw->caption.label)+1)*sizeof(wchar_t));
			wscpy(wtemp, (wchar_t *) cw->caption.label);
			cw->caption.label = (OlStr)wtemp;
		} else {
			len = strlen((char *)cw->core.name)+1;
			wtemp = (wchar_t *)XtMalloc(len*sizeof(wchar_t));
			mbstowcs(wtemp,(char *)cw->core.name,len);
			cw->caption.label = (OlStr)wtemp;
		}	
		break;
	} /* switch */

	if (cw->caption.mnemonic) {
		if (_OlAddMnemonic(new, (XtPointer)0, cw->caption.mnemonic) != OL_SUCCESS) {
			cw->caption.mnemonic = 0;
		}
	}

	GetGCs(cw);

	cw->caption.pAttrs = 
		OlgxCreateAttrs (	(Widget)cw,
					cw->caption.fontcolor,
					(OlgxBG *)&(cw->core.background_pixel),
					False,
					OL_DEFAULT_POINT_SIZE,
					cw->caption.text_format, 
					cw->caption.font);

	SetTextWidthAndHeight(cw);

	if (cw->core.width == 0) {
	    cw->core.width = cw->caption.caption_width;
	    if (cw->core.width < 1) cw->core.width = 1;
	}
			
	if (cw->core.height == 0) {
	    cw->core.height = cw->caption.caption_height;
	    if (cw->core.height < 1) cw->core.height = 1;
	}

} /* Initialize */


static void
Destroy(Widget widget)
{
    CaptionWidget cw = (CaptionWidget) widget;

    if (cw->caption.label != cw->core.name)
	XtFree((char *)cw->caption.label);

    OlgxDestroyAttrs((Widget)cw, cw->caption.pAttrs);
}


/*
 *************************************************************************
 *
 * Redisplay (do we need this?)
 *
 *************************************************************************
 */

static void 
Redisplay(Widget w, XEvent *event, Region region)
{
    OlgxTextLbl	labeltext;
    CaptionPart	*cwp = &(((CaptionWidget)w)->caption);
    int		flags = 0;

    PositionTextAndChild((CaptionWidget)w);

    labeltext.label = cwp->label;
    labeltext.text_format = cwp->text_format;
    labeltext.font = cwp->font;
    labeltext.qualifier = 0;
    labeltext.meta = False;
    labeltext.accelerator = 0;
    labeltext.mnemonic = cwp->mnemonic;
    labeltext.justification = 0;
    labeltext.flags = 0;

    if (!XtIsSensitive((Widget)w))
	flags |= OLGX_INACTIVE;
    if (cwp->text_format == OL_WC_STR_REP)
	flags |= OLGX_LABEL_IS_WCS;

    OlgxDrawTextButton (XtScreen(w), XtWindow(w),
			cwp->pAttrs,
			cwp->caption_x, cwp->caption_y,
			cwp->caption_width, cwp->caption_height,
			(XtPointer)&labeltext, OL_LABEL, flags);
}

/*
 *************************************************************************
 *
 * SetValues - look for changing caption text.
 *
 *************************************************************************
 */

/* ARGSUSED */
static Boolean 
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	CaptionWidget curcw = (CaptionWidget) current;
	CaptionWidget reqcw = (CaptionWidget) request;
	CaptionWidget newcw = (CaptionWidget) new;
	Boolean needs_redisplay = False;
	Dimension	requestHeight, requestWidth;
	char *temp;
	wchar_t *wtemp;
	int len;

	if (newcw->caption.mnemonic != curcw->caption.mnemonic) {
		if (curcw->caption.mnemonic)
			_OlRemoveMnemonic(new, (XtPointer)0, False,
					  curcw->caption.mnemonic);
		if (newcw->caption.mnemonic)
			if (_OlAddMnemonic(new, (XtPointer)0,
					newcw->caption.mnemonic) != OL_SUCCESS)
				newcw->caption.mnemonic = 0;
		needs_redisplay = True;
	}

	if (curcw->caption.label != newcw->caption.label) {
		if (curcw->caption.label != (OlStr)curcw->core.name)
			XtFree(curcw->caption.label);

	switch(newcw->caption.text_format) {
		case OL_SB_STR_REP:
		case OL_MB_STR_REP:
		if (newcw->caption.label != (OlStr)NULL) {
			temp = 
			(char *) XtMalloc(strlen(newcw->caption.label)+1);
			strcpy( temp, (char *) newcw->caption.label);
			newcw->caption.label = (OlStr)temp;
		} else
			newcw->caption.label = (OlStr)newcw->core.name;
		break;
		case OL_WC_STR_REP:
		if (newcw->caption.label != (OlStr)NULL) {
			wtemp = (wchar_t *) XtMalloc((wslen((wchar_t *)
				newcw->caption.label)+1)*sizeof(wchar_t));
			wscpy(wtemp, (wchar_t *) newcw->caption.label);
			newcw->caption.label = (OlStr)wtemp;
		} else {
			len = strlen((char *)newcw->core.name)+1;
			wtemp = (wchar_t *)XtMalloc(len*sizeof(wchar_t));
			mbstowcs(wtemp,(char *)newcw->core.name,len);
			newcw->caption.label = (OlStr)wtemp;
		}	
		break;
	} /* switch */

		needs_redisplay = True;
	}

	if ((curcw->caption.fontcolor != newcw->caption.fontcolor) || 
		(curcw->core.background_pixel != newcw->core.background_pixel) ||
		(curcw->caption.font != newcw->caption.font)) {

		needs_redisplay = True;
		OlgxDestroyAttrs((Widget)newcw, newcw->caption.pAttrs);
		newcw->caption.pAttrs = OlgxCreateAttrs ((Widget)newcw,
									newcw->caption.fontcolor,
									(OlgxBG *)&(newcw->core.background_pixel),
									False,
									OL_DEFAULT_POINT_SIZE,
									newcw->caption.text_format,
									newcw->caption.font);
	}

	if (needs_redisplay)
		SetTextWidthAndHeight(newcw);

	if (XtIsSensitive((Widget)curcw) != XtIsSensitive((Widget)newcw))
		needs_redisplay = True;

	if (curcw->caption.recompute_size != newcw->caption.recompute_size)
		needs_redisplay = True;

	if (needs_redisplay && newcw->caption.recompute_size) {
		Dimension new_width, new_height;
		GetWidthAndHeight(newcw, &new_width, &new_height);
		newcw->core.width = new_width;
		newcw->core.height = new_height;
	}

	if (newcw->core.width == 0)
		newcw->core.width = 1;
	if (newcw->core.height == 0)
		newcw->core.height = 1;

	return( needs_redisplay );
}

/*
 *************************************************************************
 *
 * InsertChild - Make certain we don't get more than one child.
 *
 *************************************************************************
 */

static void
InsertChild (Widget w)
{
	int	i;
	CaptionWidget	cw = (CaptionWidget)XtParent(w);
	XtWidgetProc	insert_child = ((CompositeWidgetClass)
	(captionClassRec.core_class.superclass))->composite_class.insert_child;

	for (i = 0; i < cw->composite.num_children; i++) {
		if ((cw->composite.children[i])->core.being_destroyed == False)
			OlError(dgettext(OlMsgsDomain,
			    "The Caption widget takes only one child\n"));
	}

	if (insert_child)
		(*insert_child)(w);
}

static void
GetWidthAndHeight(CaptionWidget w, Dimension* pwidth, Dimension* pheight)
{
    Widget child;
    if (w->composite.num_children &&
	(child = w->composite.children[0], XtIsManaged(child))) {
	int childw = child->core.width + 2*child->core.border_width;
	int childh = child->core.height + 2*child->core.border_width;

	switch (w->caption.position) {
	case OL_TOP:
	case OL_BOTTOM:
	    *pwidth = _OlMax((int)w->caption.caption_width, childw);
	    *pheight = w->caption.caption_height+childh+w->caption.space;
	    break;
	default:
	    *pwidth = w->caption.caption_width+childw+w->caption.space;
	    *pheight = _OlMax((int)w->caption.caption_height, childh);
	    break;
	}
    } else {
	*pwidth = (w->caption.caption_width) ? w->caption.caption_width : w->core.width;
	*pheight = (w->caption.caption_height) ? w->caption.caption_height : w->core.height;
    }
}

/*
 *************************************************************************
 *
 * Resize myself as the child changes.
 *
 *************************************************************************
 */

static void
ChangeManaged (Widget w)
{
    XtWidgetGeometry request;
    XtGeometryResult result;
    GetWidthAndHeight((CaptionWidget)w,&request.width,&request.height);

    if (request.width != w->core.width || request.height != w->core.height) {
	request.request_mode = CWWidth|CWHeight;
	_OlDnDSetDisableDSClipping(w, True);
	while (XtMakeGeometryRequest(w,&request,&request) == XtGeometryAlmost);
	_OlDnDSetDisableDSClipping(w, False);
	OlDnDWidgetConfiguredInHier(w);
    }
}

/*
 *************************************************************************
 *
 * GeometryManager - About as simple as they come.
 *
 *************************************************************************
 */

static XtGeometryResult 
GeometryManager(Widget w, XtWidgetGeometry *desired, XtWidgetGeometry *reply)
{
    CaptionWidget cw = (CaptionWidget)XtParent(w);
    XtWidgetGeometry request;
    XtGeometryResult result;
    Dimension	save_width, save_height, save_border_width;

    if ((desired->request_mode & CWX && desired->x != w->core.x) ||
        (desired->request_mode & CWY && desired->y != w->core.y))
	return (XtGeometryNo);
    save_width = w->core.width;
    if (desired->request_mode & CWWidth) w->core.width = desired->width;
    save_height = w->core.height;
    if (desired->request_mode & CWHeight) w->core.height = desired->height;
    save_border_width = w->core.border_width;
    if (desired->request_mode & CWBorderWidth) w->core.border_width = desired->border_width;

    GetWidthAndHeight(cw,&request.width,&request.height);

    if (request.width == cw->core.width && request.height == cw->core.height)
	result = XtGeometryYes;
    else {
	request.request_mode =
	    CWWidth | CWHeight | (desired->request_mode&XtCWQueryOnly);
	_OlDnDSetDisableDSClipping((Widget)cw, True);
	result = XtMakeGeometryRequest((Widget)cw, &request, 0);
	_OlDnDSetDisableDSClipping((Widget)cw, False);
	OlDnDWidgetConfiguredInHier((Widget)cw);
	if (result == XtGeometryAlmost) result = XtGeometryNo;
    }
    if (result != XtGeometryYes || (desired->request_mode&XtCWQueryOnly)) {
	w->core.width = save_width;
	w->core.height = save_height;
	w->core.border_width = save_border_width;
    }
    return (result);
}

/*
 *************************************************************************
 * ActivateWidget - this procedure allows external forces to activate this
 * widget indirectly.
 ****************************procedure*header*****************************
*/
/* ARGSUSED */
static Boolean
ActivateWidget (Widget w, OlVirtualName type, XtPointer data)
{
	CaptionWidget cw = (CaptionWidget) w;

	if (cw -> composite.num_children == 0) {
		return(True);
	}

	if (OlCallAcceptFocus(cw->composite.children[0], CurrentTime)) {
		OlMoveFocus (cw->composite.children[0], OL_IMMEDIATE, CurrentTime);
	}

	OlActivateWidget(cw->composite.children[0], OL_SELECTKEY, NULL);

	return(True);
} /* END OF ActivateWidget() */
