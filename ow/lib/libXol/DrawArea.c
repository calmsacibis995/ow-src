#pragma ident	"@(#)DrawArea.c	302.6	93/01/20 lib/libXol SMI"	/* OLIT	*/

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

/*
 *************************************************************************
 *
 * Description:	Open Look Drawing Area widget
 *
 *******************************file*header*******************************
 */

#include <libintl.h>

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <X11/StringDefs.h>

#include <Xol/DrawAreaP.h>
#include <Xol/OlDnDVCX.h>
#include <Xol/OlI18nP.h>
#include <Xol/OpenLookP.h>
#include <Xol/VendorI.h>
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
static void	CallResizeCallBack(Widget w);

					/* class procedures		*/

static void	ClassPartInitialize(WidgetClass wc);
static void	Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);
static void	Realize(Widget widget, Mask *value_mask, XSetWindowAttributes *attributes);
static void	Destroy(Widget widget);
static void 	Resize(Widget w);
static void 	Redisplay(Widget widget, XEvent *event, Region region);
static Boolean	SetValues(Widget current, Widget request, Widget new,
			  ArgList args, Cardinal *num_args);
static XtGeometryResult	GeometryManager(Widget w, XtWidgetGeometry *request,
					XtWidgetGeometry *reply);
static void	ChangeManaged(Widget w);


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define BYTE_OFFSET     XtOffsetOf(DrawAreaRec, draw_area.dyn_flags)
static _OlDynResource dyn_res[] = {
{ { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel), 0,         XtRString, XtDefaultForeground }, BYTE_OFFSET, OL_B_DRAWAREA_FG, NULL },
};
#undef BYTE_OFFSET

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource resources[] =
{
	{ XtNexposeCallback, XtCCallback, XtRCallback,
	  sizeof(XtPointer),
	  XtOffset(DrawAreaWidget, draw_area.exposeCallback),
	  XtRCallback, (XtPointer) NULL},

	{ XtNgraphicsExposeCallback, XtCCallback, XtRCallback,
	  sizeof(XtPointer),
	  XtOffset(DrawAreaWidget, draw_area.gExposeCallback),
	  XtRCallback, (XtPointer) NULL},

	{ XtNnoExposeCallback, XtCCallback, XtRCallback,
	  sizeof(XtPointer),
	  XtOffset(DrawAreaWidget, draw_area.noExposeCallback),
	  XtRCallback, (XtPointer) NULL},

	{ XtNresizeCallback, XtCCallback, XtRCallback,
	  sizeof(XtPointer),
	  XtOffset(DrawAreaWidget, draw_area.resizeCallback),
	  XtRCallback, (XtPointer) NULL},

	{ XtNvisual, XtCVisual, XtRVisual, sizeof(Visual *),
	  XtOffset(DrawAreaWidget, draw_area.visual),
	  XtRCallProc, (XtPointer) _OlCopyParentsVisual},

	{ XtNcolormap, XtCColormap, XtRColormap, 
	  sizeof(Colormap),
	  XtOffset(DrawAreaWidget, core.colormap), 
	  XtRCallProc, (XtPointer) _OlSetupColormap},

	{ XtNforeground, XtCForeground, XtRPixel,
	  sizeof(Pixel),
	  XtOffset(DrawAreaWidget, draw_area.foreground),
	  XtRString, (XtPointer) XtDefaultForeground},
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

DrawAreaClassRec drawAreaClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &bulletinBoardClassRec,
    /* class_name         */    "DrawArea",
    /* widget_size        */    sizeof(DrawAreaRec),
    /* class_initialize   */    NULL,
    /* class_part_init    */	ClassPartInitialize,
    /* class_inited       */	FALSE,
    /* initialize         */    Initialize,
    /* initialize_hook    */    NULL,
    /* realize            */    Realize,
    /* actions		  */	NULL,
    /* num_actions	  */	0,
    /* resources          */    resources,
    /* num_resources      */    XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion	  */	FALSE,
    /* compress_exposure  */	TRUE|XtExposeGraphicsExpose|XtExposeNoExpose,
    /* compress_enterlv   */    TRUE,
    /* visible_interest   */    TRUE,
    /* destroy            */    Destroy,
    /* resize             */    Resize,
    /* expose             */    Redisplay,
    /* set_values         */    SetValues,
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
    /* change_managed     */    ChangeManaged,
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
    /* dyn_data		  */	{ dyn_res, XtNumber(dyn_res) },
    /* transparent_proc   */	_OlDefaultTransparentProc,
   },{
/* bulletin board class */
    /* none             */      NULL
   },{
/* draw area class */     
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

WidgetClass drawAreaWidgetClass = (WidgetClass)&drawAreaClassRec;

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
 * CallResizeCallBack -
 *
 ****************************procedure*header*****************************
 */
static void
CallResizeCallBack(Widget w)
{
	DrawAreaWidget	daw = (DrawAreaWidget)w;
	OlDrawAreaCallbackStruct cbdata;

	cbdata.reason = OL_REASON_RESIZE;
	cbdata.event = (XEvent *)NULL;
	cbdata.x = daw->core.x;
	cbdata.y = daw->core.y;
	cbdata.width = daw->core.width;
	cbdata.height = daw->core.height;
	cbdata.region = (Region)NULL;

	/* set flag indicating you're in a resize proc */
	daw->draw_area.resizeflag = True;

	XtCallCallbackList(w,
		daw->draw_area.resizeCallback, &cbdata);

	daw->draw_area.resizeflag = False;
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
 * ClassPartInitialize - this function calls
 *	_OlFixResourceList which reorders the widget's
 *	 resource list so the visual resource is evaluated
 *	 ahead of the colormap resource.
 *
 ****************************procedure*header*****************************
 */
static void 
ClassPartInitialize(WidgetClass wc)
{
	_OlFixResourceList(wc);
}

/*
 *************************************************************************
 *
 * Initialize -
 *
 ****************************procedure*header*****************************
 */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	register DrawAreaWidget nw = (DrawAreaWidget)new;

	nw->draw_area.resizeflag = False;
}

/*
 *************************************************************************
 *
 * Realize -
 *
 ****************************procedure*header*****************************
 */
static void 
Realize(Widget widget, Mask *value_mask, XSetWindowAttributes *attributes)
{
	XtCreateWindow(widget, (unsigned int) InputOutput,
		((DrawAreaWidget)widget)->draw_area.visual,
		*value_mask, attributes);

	_OlAddWMColormapWindows(widget);
}

/*
 *************************************************************************
 *
 * Destroy -
 *
 ****************************procedure*header*****************************
 */
static void 
Destroy(Widget widget)
{
	_OlDelWMColormapWindows(widget);
}

/*
 *************************************************************************
 *
 * Resize -
 *
 ****************************procedure*header*****************************
 */
static void 
Resize(Widget w)
{
	XtWidgetProc super_resize;
	DrawAreaWidget daw = (DrawAreaWidget) w;

	super_resize = drawAreaWidgetClass->core_class.superclass->core_class.resize;

	if (super_resize != (XtWidgetProc)NULL)
		(*super_resize)(w);

	if (!daw->draw_area.resizeflag)
		CallResizeCallBack(w);
}

/*
 *************************************************************************
 *
 * Redisplay -
 *
 ****************************procedure*header*****************************
 */
static void 
Redisplay(Widget widget, XEvent *event, Region region)
{
	DrawAreaWidget daw = (DrawAreaWidget) widget;
	OlDrawAreaCallbackStruct cbdata;

	switch (event->type) {
	case Expose:
		{
		cbdata.reason = OL_REASON_EXPOSE;
		cbdata.event = event;
		cbdata.x = (Position)event->xexpose.x;
		cbdata.y = (Position)event->xexpose.y;
		cbdata.width = (Dimension)
			event->xexpose.width;
		cbdata.height = (Dimension)
			event->xexpose.height;
		cbdata.region = region;
		XtCallCallbackList(widget,
			daw->draw_area.exposeCallback,
			&cbdata);
		}
		break;
	case GraphicsExpose:
		{
		cbdata.reason = OL_REASON_GRAPHICS_EXPOSE;
		cbdata.event = event;
		cbdata.x = (Position)event->xgraphicsexpose.x;
		cbdata.y = (Position)event->xgraphicsexpose.y;
		cbdata.width = (Dimension)
			event->xgraphicsexpose.width;
		cbdata.height = (Dimension)
			event->xgraphicsexpose.height;
		cbdata.region = region;
		XtCallCallbackList(widget,
			daw->draw_area.gExposeCallback,
			&cbdata);
		}
		break;
	case NoExpose:
	{
		cbdata.reason = OL_REASON_NO_EXPOSE;
		cbdata.event = event;
		cbdata.x = (Position)0;
		cbdata.y = (Position)0;
		cbdata.width = (Dimension)0;
		cbdata.height = (Dimension)0;
		cbdata.region = (Region)NULL;
		XtCallCallbackList(widget,
			daw->draw_area.noExposeCallback,
			&cbdata);
		}
		break;
	default:
		break;
	}
}

/*
 *************************************************************************
 *
 * SetValues
 *
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues(Widget current, Widget request, Widget new,
	  ArgList args, Cardinal *num_args)
{
        DrawAreaWidget new_da = (DrawAreaWidget)new;
        DrawAreaWidget current_da = (DrawAreaWidget)current;

	CorePart *cp = &(current->core);
	CorePart *newcp = &(new->core);

	if (new_da->draw_area.visual != current_da->draw_area.visual) {
		new_da->draw_area.visual =
			current_da->draw_area.visual;
		OlWarning(dgettext(OlMsgsDomain,
			"Cannot change XtNvisual.\n"));
	}

	if ((current_da->draw_area.resizeflag) &&
	    ((newcp->x != cp->x) ||
	     (newcp->y != cp->y) ||
	     (newcp->width != cp->width) ||
	     (newcp->height != cp->height) ||
	     (newcp->border_width != cp->border_width))) {
		newcp->x = cp->x;
		newcp->y = cp->y;
		newcp->width = cp->width;
		newcp->height = cp->height;
		newcp->border_width = cp->border_width;
		OlWarning(dgettext(OlMsgsDomain,
			"Cannot change XtNx, XtNy, XtNwidth, XtNheight or XtNborderWidth within a Resize Callback.\n"));
	}

	return (False);
}

/*
 *************************************************************************
 *
 * GeometryManager - Call the super classes geometry
 *	manager.  If the drawing area's size has changed
 *	call the drawing area's resize callbacks.
 *
 ****************************procedure*header*****************************
 */
static XtGeometryResult
GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply)
{
	XtGeometryHandler super_gm;
	XtGeometryResult  result = XtGeometryYes;
	Widget parent = XtParent(w);
	DrawAreaWidget daw = (DrawAreaWidget)parent;

	super_gm = ((BulletinBoardWidgetClass )drawAreaWidgetClass->core_class.superclass)->composite_class.geometry_manager;

	if (super_gm != (XtGeometryHandler)NULL) {
		Dimension old_width, old_height;

		old_width = parent->core.width;
		old_height = parent->core.height;

		_OlDnDSetDisableDSClipping(parent, True);
		daw->draw_area.resizeflag = True;
		result = (*super_gm)(w, request, reply);
		daw->draw_area.resizeflag = False;
		_OlDnDSetDisableDSClipping(parent, False);

		if ((old_width != parent->core.width) ||
		    (old_height != parent->core.height)) {
			CallResizeCallBack(parent);
			OlDnDWidgetConfiguredInHier(w);
		}
	}

	if(result == XtGeometryYes)
		OlWidgetConfigured(w);
	return(result);
}

/*
 *************************************************************************
 *
 * ChangeManaged - Call the super classes changed
 *	managed.  If the drawing area's size has changed
 *	call the drawing area's resize callbacks.
 *
 ****************************procedure*header*****************************
 */
static void
ChangeManaged(Widget w)
{
	XtWidgetProc super_cm;

	super_cm = ((BulletinBoardWidgetClass )drawAreaWidgetClass->core_class.superclass)->composite_class.change_managed;

	if (super_cm != (XtWidgetProc)NULL) {
		Dimension old_width, old_height;

		old_width = w->core.width;
		old_height = w->core.height;

		(*super_cm)(w);

		if ((old_width != w->core.width) ||
		    (old_height != w->core.height))
			CallResizeCallBack(w);
	}
}

/*
 *************************************************************************
 *
 * Code to cache colormap for each Shell Widget.
 *
 *************************************************************************
 */

typedef struct _CMShellItem {
	Widget	shell;
	Widget	*list;
	int	count;
	struct _CMShellItem *next;
} CMShellItem;

static CMShellItem *CMShellList = NULL;

/*
 * Look for the correct per shell widget list
 */
static CMShellItem *
_FindShellItem(Widget w)
{
	Widget sw = _OlGetShellOfWidget(w);
	CMShellItem *n = CMShellList;

	for (; n && (n->shell != sw); n = n->next);

	return(n);
}

/*
 * Find the correct per shell widget list,
 * add it if it doesn't exists.
 */
static CMShellItem *
_AddShellItem(Widget w)
{
	Widget sw = _OlGetShellOfWidget(w);
	CMShellItem *n;


	if (!(n = _FindShellItem(w))) {
		n = XtNew(CMShellItem);
		n->shell = sw;
		n->list = (Widget *)NULL;
		n->count = 0;
		n->next = CMShellList;
		CMShellList = n;
	}

	return(n);
}

/*
 * See if the widget already exists in
 * the shell's widget list.
 */
static int
_FindWidgetItem(CMShellItem *n, Widget w)
{
	register int i;

	for (i = 0; i < n->count; ++i)
		if (n->list[i] == w)
			break;

	if (i < n->count)
		return(True);
	else
		return(False);
}

/*
 * Add the widget to the shell's widget list
 * if it doesn't already exists.
 */
static int
_AddWidgetItem(CMShellItem *n, Widget w)
{
	Widget *tmp;

	if (_FindWidgetItem(n, w))
		return(False);

	tmp = (Widget *)XtMalloc(sizeof(Widget) *
		(n->count+1));

	*tmp = w;

	memmove((void*)(tmp+1),(const void*)n->list,
		(size_t)(n->count * sizeof(Widget)));

	++(n->count);
	XtFree((char *)n->list);
	n->list = tmp;

	return(True);
}

/*
 * Delete the widget to the shell's widget
 * list if it exists.
 */
static int
_DelWidgetItem(CMShellItem *n, Widget w)
{
	register int i, j;
	Widget *tmp;

	if (n->count < 1)
		return(False);

	if (!_FindWidgetItem(n, w))
		return(False);

	if (n->count > 1) {
		tmp = (Widget *)XtMalloc(sizeof(Widget) *
			(n->count-1));

		for (i=0,j=0; i<n->count; ++i)
			if (n->list[i] != w) {
				tmp[j] = n->list[i];
				++j;
			}
	} else
		tmp = (Widget *)NULL;

	--(n->count);
	XtFree((char *)n->list);
	n->list = tmp;
	return(True);
}


/*
 * Add a widget to it's shell's WM_COLORMAP
 * property list if it doesn't already exists.
 */
void
_OlAddWMColormapWindows (Widget w)
{
	CMShellItem *n = _AddShellItem(w);

	if (_AddWidgetItem(n, w))
		XtSetWMColormapWindows(n->shell,
			n->list, n->count);
}

/*
 * Delete a widget to it's shell's WM_COLORMAP
 * property list if it exists.
 */
void
_OlDelWMColormapWindows (Widget w)
{
	CMShellItem *n = _FindShellItem(w);

	if (n == NULL)
		return;

	if (_DelWidgetItem(n, w))
		XtSetWMColormapWindows(n->shell,
			n->list, n->count);
}
