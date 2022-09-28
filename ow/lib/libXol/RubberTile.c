#pragma ident	"@(#)RubberTile.c	302.5	97/03/26 lib/libXol SMI"	/* rubbertile:RubberTile.c 1.8	*/

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


#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <X11/IntrinsicP.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>

#include <Xol/OlDnDVCX.h>
#include <Xol/OpenLookP.h>
#include <Xol/RubberTilP.h>
#include <Xol/memutil.h>
#include <Xol/SuperCaret.h>

/*
 * New types:
 */
typedef enum HowAsked {
	ParentQueried,
	ChildQueried,
	PleaseTry
}			HowAsked;


/*
 * Macro definitions:
 */
#define CONSTRAINT(W) (*(RubberTileConstraintRec **)&((W)->core.constraints))
#define VERTICAL(W) (((RubberTileWidget)(W))->rubber_tile.orientation == OL_VERTICAL)
#define HORIZONTAL(W) (((RubberTileWidget)(W))->rubber_tile.orientation == OL_HORIZONTAL)

#define Strlen(S)	(S && *S? strlen((S)) : 0)
#define Strdup(S)	strcpy(MALLOC((unsigned)Strlen(S) + 1), S)

/*
 * Local routines:
 */

static void		ClassInitialize (
	void
);
static void		Initialize (Widget request, Widget new,
				    ArgList args, Cardinal *num_args);
static void		Destroy (Widget w);
static void		Resize (Widget w);
static Boolean		SetValues (Widget	current,
				   Widget	request,
				   Widget	new,
				   ArgList	args,
				   Cardinal     *num_args);
static XtGeometryResult	QueryGeometry (Widget	w,
				       XtWidgetGeometry *request,
				       XtWidgetGeometry *preferred);
static XtGeometryResult	GeometryManager (Widget w,
					 XtWidgetGeometry *request,
					 XtWidgetGeometry *reply);
static void		ChangeManaged(Widget w);
static void		ConstraintInitialize (Widget request, Widget new,
					      ArgList args,
					      Cardinal *num_args);
static void		ConstraintDestroy (Widget w);
static Boolean		ConstraintSetValues (
	Widget			current,
	Widget			request,
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
);
static void		CalcChildSize (
	Widget			child,
	Dimension *		replyHard,
	Dimension *		replySimple,
	Dimension		hard,
	Dimension		simple
);
static void		Layout (
	RubberTileWidget	w,
	Boolean			resizable,
	HowAsked		query,
	Widget			who_asking,
	XtWidgetGeometry *	response,
	Boolean                 from_change_managed
);
static int		Order (const void *v1, const void *v2);
static void		ResolveReference (
	Widget *	pRefWidget,
	String *	pRefName,
	Widget		w
);
static Widget		FindWidget (
	RubberTileWidget	w,
	String			name
);
static void		Error (
	String			format,
	...
);
static void		Warning (
	String			format,
	...
);

/*
 * External data:
 */

char			XtNrefName	[] = "refName";
char			XtNrefWidget	[] = "refWidget";
char			XtNweight	[] = "weight";

char			XtCRefName	[] = "RefName";
char			XtCRefWidget	[] = "RefWidget";
char			XtCWeight	[] = "Weight";

/*
 * Instance resource list:
 */

#define offset(F) XtOffset(RubberTileWidget, F)

static XtResource resources[] = {
    {
	XtNorientation, XtCOrientation,
	XtROlDefine, sizeof(OlDefine), offset(rubber_tile.orientation),
	XtRString, (XtPointer)"vertical"
    }
};

#undef offset

/*
 * Constraint resource list:
 */

#define offset(F) XtOffset(RubberTileConstraints, F)

static XtResource	constraintResources[] = {
    {
	XtNrefName, XtCRefName,
	XtRString, sizeof(String), offset(ref_name),
	XtRString, (XtPointer)0
    }, {
	XtNrefWidget, XtCRefWidget,
	XtRPointer, sizeof(XtPointer), offset(ref_widget),
	XtRPointer, (XtPointer)0
    }, {
	XtNspace, XtCSpace,
	XtRDimension, sizeof(Dimension), offset(space),
	XtRString, (XtPointer)"0"
    }, {
	XtNweight, XtCWeight,
	XtRDimension, sizeof(Dimension), offset(weight),
	XtRString, (XtPointer)"1"
    }
};

/*
 * Full class record constant:
 */

RubberTileClassRec	rubberTileClassRec = {
    /*
     * Core class:
     */
    {
    /* superclass          */	(WidgetClass)&managerClassRec,
    /* class_name          */	"RubberTile",
    /* widget_size         */	sizeof(RubberTileRec),
    /* class_initialize    */	ClassInitialize,
    /* class_part_init     */	NULL,
    /* class_inited        */	FALSE,
    /* initialize          */	Initialize,
    /* initialize_hook     */	NULL,
    /* realize             */	XtInheritRealize,
    /* actions             */	NULL,
    /* num_actions         */	0,
    /* resources           */	resources,
    /* num_resources       */	XtNumber(resources),
    /* xrm_class           */	NULLQUARK,
    /* compress_motion     */	TRUE,
    /* compress_exposure   */	TRUE,
    /* compress_enterleave */	TRUE,
    /* visible_interest    */	FALSE,
    /* destroy             */	Destroy,
    /* resize              */	Resize,
    /* expose              */	NULL,
    /* set_values          */	SetValues,
    /* set_values_hook     */	NULL,
    /* set_values_almost   */	XtInheritSetValuesAlmost,
    /* get_values_hook     */	NULL,
    /* accept_focus        */	XtInheritAcceptFocus,
    /* version             */	XtVersion,
    /* callback_private    */	NULL,
    /* tm_table            */	XtInheritTranslations,
    /* query_geometry      */	QueryGeometry,
    },

    /*
     * Composite class:
     */
    {
    /* geometry_manager    */	GeometryManager,
    /* change_managed      */	ChangeManaged,
    /* insert_child        */	XtInheritInsertChild,
    /* delete_child        */	XtInheritDeleteChild,
    /* extension           */	NULL
    },

    /*
     * Constraint class:
     */
    {
    /* resources           */	constraintResources,
    /* num_resources       */	XtNumber(constraintResources),
    /* constraint_size     */	sizeof(RubberTileConstraintRec),
    /* initialize          */	ConstraintInitialize,
    /* destroy             */	ConstraintDestroy,
    /* set_values          */	ConstraintSetValues,
    /* extension           */	NULL
    },

    /*
     * Manager class:
     */
    {
    /* highlight_handler   */	NULL,
    /* reserved            */	NULL,
    /* reserved            */	NULL,
    /* traversal_handler   */	NULL,
    /* activate            */	NULL,
    /* event_procs         */	NULL,
    /* num_event_procs     */	0,
    /* register_focus      */	NULL,
    /* reserved            */	NULL,
    /* version             */	OlVersion,
    /* extension           */	NULL,
    /*	dyn data	   */	{NULL, 0},
    /* trnasparent proc    */	NULL,
    /* query_sc_locn_proc  */   NULL,
    },

    /*
     * RubberTile class:
     */
    {
    /* empty               */	0,
    }
};

WidgetClass		rubberTileWidgetClass = (WidgetClass)&rubberTileClassRec;

/**
 ** ClassInitialize()
 **/

static void
ClassInitialize (void)
{
	_OlAddOlDefineType ("vertical",   OL_VERTICAL);
	_OlAddOlDefineType ("horizontal", OL_HORIZONTAL);
	return;
} /* ClassInitialize() */

/**
 ** Initialize()
 **/

static void
Initialize (Widget request, Widget new, ArgList args, Cardinal *num_args)
{
        RubberTileWidget new_rtw = (RubberTileWidget)new;
	static Arg arg = { XtNallowShellResize, TRUE };


	/*
	 * Make it easy on the poor slob who insists on creating
	 * us with no children (or no initial size).
	 */
	if (!new_rtw->core.width)
		new_rtw->core.width = 1;
	if (!new_rtw->core.height)
		new_rtw->core.height = 1;

	new_rtw->rubber_tile.managed_children = 0;
	new_rtw->rubber_tile.num_managed_children = 0;

	/*
	 * If we're a child of a shell, tell it to allow resizes.
	 */
	if (XtIsSubclass(XtParent(new_rtw), shellWidgetClass))
		XtSetValues (XtParent(new_rtw), &arg, 1);

	return;
} /* Initialize */

/**
 ** Destroy()
 **/

static void
Destroy (Widget w)
{
    RubberTileWidget rtw = (RubberTileWidget)w;

    if (rtw->rubber_tile.managed_children)
	FREE ((char *)rtw->rubber_tile.managed_children);
    return;
} /* Destroy */

/**
 ** Resize()
 **/

static void
Resize (Widget w)
{

	RubberTileWidget rtw = (RubberTileWidget)w;
	Cardinal		nchildren = rtw->composite.num_children;
	Widget *		pchild	  = rtw->composite.children;
	RubberTileConstraintRec * constraint;

	Layout (rtw, False, PleaseTry, (Widget)0, (XtWidgetGeometry *)0, False);

	/*
	 * This is a good time to store the ``set-values'' size
	 * for each child. Can't wait any longer....
	 */
	while (nchildren--) {
		if (XtIsManaged(*pchild)) {
			constraint = CONSTRAINT(*pchild);
			if (VERTICAL(rtw))
				constraint->set_size = (*pchild)->core.height;
			else
				constraint->set_size = (*pchild)->core.width;
		}
		pchild++;
	}
	return;
} /* Resize */

/**
 ** SetValues()
 **/

static Boolean
SetValues (Widget	current,
	   Widget	request,
	   Widget	new,
	   ArgList	args,
	   Cardinal	*num_args)
{
        RubberTileWidget current_rtw = (RubberTileWidget)current;
        RubberTileWidget new_rtw = (RubberTileWidget)new;
    
	Boolean			ret	= False;

#define DIFFERENT(F) (new_rtw->rubber_tile.F != current_rtw->rubber_tile.F)

	if (DIFFERENT(orientation))
		Layout (new_rtw, True, PleaseTry, (Widget)0, 
				(XtWidgetGeometry *)0, False);

#undef	DIFFERENT
	return (ret);
} /* SetValues */

/**
 ** QueryGeometry()
 **/

static XtGeometryResult
QueryGeometry (Widget w, XtWidgetGeometry *request,
	       XtWidgetGeometry *preferred)
{
	XtWidgetGeometry	response;
	XtGeometryResult	result	= XtGeometryYes;

	/*
	 * We don't care about our position or border, but we will
	 * suggest a best size.
	 */

	*preferred = *request;
	preferred->request_mode &= CWWidth|CWHeight;

	Layout ((RubberTileWidget)w, True, ParentQueried,
		(Widget)0, &response, False);
	preferred->width  = response.width;
	preferred->height = response.height;

#define CHECK(BIT,F) \
	if (request->request_mode & BIT) {				\
		if (preferred->F != request->F) {			\
			result = XtGeometryAlmost;			\
			if (preferred->F == w->core.F)			\
				result = XtGeometryNo;			\
		}							\
	} else

	CHECK (CWWidth, width);
	CHECK (CWHeight, height);
#undef	CHECK

	return (result);
}

/**
 ** GeometryManager()
 **/

static XtGeometryResult
GeometryManager (Widget			w,
		 XtWidgetGeometry 	*_request,
		 XtWidgetGeometry 	*reply)
{
	XtGeometryResult	result	= XtGeometryYes;

	XtWidgetGeometry	save;
	XtWidgetGeometry	request;
	XtWidgetGeometry	response;


	/*
	 * Make a copy so that we can fiddle with the values.
	 */
	request = *_request;

	/*
	 * Children cannot ask to be repositioned.
	 */
#define CHECK(BIT,F) \
	if (request.request_mode & BIT) {				\
		if (request.F != w->core.F) {				\
			result = XtGeometryAlmost;			\
			request.F = w->core.F;				\
		}							\
	} else

	CHECK (CWX, x);
	CHECK (CWY, y);
#undef	CHECK

	/*
	 * We don't care about any other geometry except size.
	 */
	if (!(request.request_mode & (CWWidth|CWHeight|CWBorderWidth)))
		goto Return;

	/*
	 * For our convenience, make all size fields in "request" valid.
	 */
	if (!(request.request_mode & CWWidth))
		request.width = w->core.width;
	if (!(request.request_mode & CWHeight))
		request.height = w->core.height;
	if (!(request.request_mode & CWBorderWidth))
		request.border_width = w->core.border_width;

	/*
	 * Save the current core geometry, and replace it with the
	 * request geometry. We use a layout routine used by other
	 * procedures (e.g. Resize, ChangeManaged), and it uses
	 * just the core fields for figuring layout. We do have to
	 * tell the layout routine that this widget is special
	 * (and possibly that we are only inquiring about a change);
	 * the layout procedure will store the actual geometry in
	 * "response" instead of moving/resizing this widget directly.
	 *
	 * On return from the layout routine, we update our copy of
	 * the request structure to reflect geometry that can't be
	 * given to this widget (this will cause us to return an
	 * ``almost''). We set or restore the core geometry fields,
	 * and then leave.
	 */

#define SAVE(A,B) \
	(A).width        = (B).width,					\
	(A).height       = (B).height,					\
	(A).border_width = (B).border_width

	SAVE (save, w->core);
	SAVE (w->core, request);

	Layout (
		(RubberTileWidget)XtParent(w),
		True,
		(request.request_mode & XtCWQueryOnly?
			  ChildQueried
			: PleaseTry
		),
		w,
		&response,
		False
	);

#define CHECK(BIT,F) \
	if (request.request_mode & BIT) {				\
		if (request.F != response.F) {				\
			result = XtGeometryAlmost;			\
			request.F = response.F;				\
		}							\
	} else

	CHECK (CWWidth, width);
	CHECK (CWHeight, height);
	CHECK (CWBorderWidth, border_width);
#undef	CHECK

	/*
	 * When returning XtGeometryYes we have to update the
	 * widgets geometry to reflect the requested values.
	 * This includes the x and y position, as returned to
	 * this routine from "Layout()" in the "response" structure.
	 * Note: If the child had wanted different x,y values,
	 * we wouldn't be returning XtGeometryYes.
	 *
	 * Also, we update the ``set size'' at this point, so that
	 * we can track the ``natural'' size of this child.
	 */
	if (result == XtGeometryYes) {
		w->core.x = response.x;
		w->core.y = response.y;
		if (VERTICAL(XtParent(w)))
			CONSTRAINT(w)->set_size = w->core.height;
		else
			CONSTRAINT(w)->set_size = w->core.width;
	} else
		SAVE (w->core, save);
#undef	SAVE

	/*
	 * If the best we can do is the current size of the widget,
	 * returning anything but XtGeometryNo would be a waste of
	 * time.
	 */
#define SAME(F)	(request.F == w->core.F)
	if (
		result == XtGeometryAlmost
	     && SAME(width) && SAME(height) && SAME(border_width)
	) {
		request.request_mode = 0;
		result = XtGeometryNo;
	}
#undef	SAME

Return:
	if (reply)
		/*
		 * NOTE:
		 * Xt doesn't require that we return anything in "reply"
		 * if we return XtGeomtryNo, but the description of the
		 * set_values_almost procedure might cause some to think
		 * otherwise. Thus we do the following regardless of
		 * the return value, to help out.
		 */
		*reply = request;

	if(result == XtGeometryYes)
		OlWidgetConfigured(w);
	return (result);
} /* GeometryManager */

/**
 ** ChangeManaged()
 **/

static void
ChangeManaged (Widget w)
{
	Cardinal nchildren = ((RubberTileWidget)w)->composite.num_children;
	Widget *pchild = ((RubberTileWidget)w)->composite.children;
	RubberTileConstraintRec * constraint;

	/*
	 * This is a good time to store the ``set-values'' size
	 * for each child. Can't wait any longer....
	 */
	while (nchildren--) {
		if (XtIsManaged(*pchild)) {
			constraint = CONSTRAINT(*pchild);
			if (!constraint->set_size)
				if (VERTICAL(w))
					constraint->set_size = (*pchild)->core.height;
				else
					constraint->set_size = (*pchild)->core.width;
		}
		pchild++;
	}

	/*
	 * Layout the children, do geometry negotiation, etc.
	 */
 	Layout ((RubberTileWidget)w, True, PleaseTry,
		(Widget)0, (XtWidgetGeometry *)0, True);

	return;
} /* ChangeManaged */

/**
 ** ConstraintInitialize()
 **/

static void
ConstraintInitialize (Widget request, Widget new,
		      ArgList args, Cardinal *num_args)
{
	RubberTileConstraintRec *	constraint = CONSTRAINT(new);


	if (constraint->ref_name)
		constraint->ref_name = Strdup(constraint->ref_name);

	/*
	 * Don't save the ``set-value'' size here, wait until the
	 * child gets managed.
	 */
	constraint->set_size = 0;

	return;
} /* ConstraintInitialize */

/**
 ** ConstraintDestroy()
 **/

static void
ConstraintDestroy (Widget w)
{
	RubberTileConstraintRec *	constraint = CONSTRAINT(w);


	if (constraint->ref_name)
		FREE (constraint->ref_name);

	return;
} /* ConstraintDestroy */

/**
 ** ConstraintSetValues()
 **/

static Boolean
ConstraintSetValues (Widget current, Widget request, Widget new,
		     ArgList args, Cardinal * num_args)
{
	RubberTileConstraintRec * curConstraint	= CONSTRAINT(current);
	RubberTileConstraintRec * newConstraint = CONSTRAINT(new);

	RubberTileWidget	parent		= (RubberTileWidget)XtParent(new);

	Boolean			ret		= False;


#define DIFFERENT(F)	(newConstraint->F != curConstraint->F)

	/*
	 * If the reference widget or name has changed, clear the
	 * opposing member in order to get the proper referencing.
	 * For names, the string space will be freed out of "current"
	 * later.
	 *
	 * Note: If both the reference widget and the reference name
	 * have changed, leave both alone so that mismatches can
	 * be caught.
	 */
	if (DIFFERENT(ref_widget) && DIFFERENT(ref_name))
		/*EMPTY*/;
	else if (DIFFERENT(ref_widget))
		newConstraint->ref_name = 0;
	else if (DIFFERENT(ref_name))
		newConstraint->ref_widget = 0;

	if (DIFFERENT(ref_name)) {
		FREE (curConstraint->ref_name);
		if (newConstraint->ref_name)
			newConstraint->ref_name = Strdup(newConstraint->ref_name);
	}

	/*
	 * Save the set-values size if it has changed.
	 */
	if (VERTICAL(parent)) {
		if (new->core.height != current->core.height)
			newConstraint->set_size = new->core.height;
	} else {
		if (new->core.width != current->core.width)
			newConstraint->set_size = new->core.width;
	}

	/*
	 * If the constraints have changed, rearrange the children
	 * to fit the constraints.
	 */
	if (DIFFERENT(ref_widget) || DIFFERENT(weight)) {
		if (XtIsRealized(current))
			Layout (
				(RubberTileWidget)new->core.parent,
				True,
				PleaseTry,
				(Widget)0,
				(XtWidgetGeometry *)0,
				False
			);
	}

#undef	DIFFERENT
	return (ret);
} /* ConstraintSetValues */

/**
 ** CalcChildSize()
 **/

static void
CalcChildSize (Widget child, Dimension *replyHard, Dimension *replySimple,
	       Dimension hard, Dimension simple)
{
	RubberTileConstraintRec * constraint = CONSTRAINT(child);


	if (*replySimple < simple)
		*replySimple = simple;

	*replyHard += constraint->set_size
		   + 2 * child->core.border_width
		   + constraint->space;

	return;
} /* CalcChildSize */

/**
 ** Layout()
 **
 ** Layout the managed children.
 **/

static void
Layout (RubberTileWidget w, Boolean resizable, HowAsked query,
	Widget who_asking, XtWidgetGeometry *response,
	Boolean from_change_managed)
{
	int			configured   = 0;
	Cardinal		nchildren    = w->composite.num_children;
	Cardinal		nmanaged;
	Cardinal		n;

	Widget *		pchild	     = w->composite.children;
	Widget *		list;
	Widget *		p;

	RubberTileConstraintRec * constraint;

	Dimension		width;
	Dimension		height;

	Position		pos;

	int			dsize;
	int			total_weight;


	/*
	 * Construct an alternate list of children, that can be
	 * sorted into reverse order of the reference constraints
	 * We keep this list with the parent, to avoid needless
	 * hits on the memory allocation subsystem.
	 *
	 * Compute the total weight while doing this.
	 */

	for (nmanaged = 0, n=0; n < nchildren; n++)
		if (XtIsManaged(pchild[n]))
			 nmanaged++;
	if (!nmanaged)
		return;

	if (w->rubber_tile.num_managed_children == nmanaged)
		list = w->rubber_tile.managed_children;
	else {
		if ((list = w->rubber_tile.managed_children))
			list = (Widget *)REALLOC((char *)list,
						 nmanaged * sizeof(Widget));
		else
			list = (Widget *)MALLOC(nmanaged * sizeof(Widget));
		w->rubber_tile.managed_children = list;
		w->rubber_tile.num_managed_children = nmanaged;
	}

	p = list;
	total_weight = 0;
	for (n = 0; n < nchildren; n++) {
		if (!XtIsManaged(pchild[n]))
			continue;

		constraint = CONSTRAINT(pchild[n]);
		ResolveReference (
			&constraint->ref_widget,
			&constraint->ref_name,
			pchild[n]
		);

		total_weight += constraint->weight;

		*p++ = pchild[n];
	}
	if (!total_weight)
		total_weight = 1;

	/*
	 * Sort the list.
	 *
	 * In the absence of a constraint to order two widgets,
	 * their order is their creation order. Likewise, of two
	 * widgets that reference the same widget, their order
	 * is their creation order. (We hope "qsort()" maintains
	 * original order for equals!)
	 */
	qsort ((char *)list, nmanaged, sizeof(Widget), Order);


	/*
	 * Calculate our ``base'' size:
	 *
	 * If we can resize, this is the size we need to be to just fit
	 * the children at their current sizes. The difference between
	 * this base size and our current size we'll try to get from our
	 * parent. If we don't get it all from our parent, we'll grow
	 * or shrink our children to fill in.
	 *
	 * If we can't resize, the base size is the what we need to just
	 * fit the children at their ORIGINAL (``set-values'') size.
	 * We'll spread the difference between that base size and our
	 * current size among the children.
	 */

	width = 0;
	height = 0;

	if (resizable && !from_change_managed) {
		for (p = list, n = 0; n < nmanaged; n++, p++) {
			Dimension		W = _OlWidgetWidth(*p);
			Dimension		H = _OlWidgetHeight(*p);
			Dimension		space;

			space = CONSTRAINT(*p)->space;
			if (VERTICAL(w)) {
				if (width < W)
					width = W;
				height += H + space;
			} else {
				width += W + space;
				if (height < H)
					height = H;
			}
		}

	} else {
		for (p = list, n = 0; n < nmanaged; n++, p++) {
			Dimension		W = _OlWidgetWidth(*p);
			Dimension		H = _OlWidgetHeight(*p);

			if (VERTICAL(w))
				CalcChildSize (*p, &height, &width, H, W);
			else
				CalcChildSize (*p, &width, &height, W, H);
		}
	}

	/*
	 * Do what we have to to survive having no managed children.
	 */
	if (!width)
		width	= 1;
	if (!height)
		height	= 1;

	/*
	 * If we're resizable, ask our parent.
	 */
	if (
		resizable
	     && query != ParentQueried
	     && (width != w->core.width || height != w->core.height)
	) {
		XtWidgetGeometry	ask;
		XtWidgetGeometry	get;
		XtGeometryResult	ans;

		ask.width  = width;
		ask.height = height;

		ask.request_mode = (CWWidth|CWHeight);
		if (query == ChildQueried)
			ask.request_mode |= XtCWQueryOnly;
		ans = XtMakeGeometryRequest((Widget)w, &ask, &get);
		if (query != ChildQueried && ans == XtGeometryAlmost)
			XtMakeGeometryRequest((Widget)w, &get, (XtWidgetGeometry *)0);
	}

	/*
	 * Skip the rest of this if our parent is just asking.
	 */
	if (query == ParentQueried) {
		if (response) {
			response->x            = w->core.x;
			response->y            = w->core.y;
			response->width        = width;
			response->height       = height;
			response->border_width = w->core.border_width;
		}
		return;
	}

	/*
	 * "dsize" is how much we still have left to go. We'll
	 * try to grow/shrink our children to take up that slack.
	 */
	if (VERTICAL(w))
		dsize = w->core.height - height;
	else
		dsize = w->core.width - width;

	/*
	 * Adjust the size and/or position of each child.
	 *
	 * You may be wondering why we bother with the following
	 * loop if "dsize" is zero (this happens, for instance,
	 * when this routine is called from the ChangeManaged proc.)
	 * The reason is that we may still need to set the children's
	 * position.
	 */
	pos = 0;
	_OlDnDSetDisableDSClipping((Widget)w, True);
	for (p = list, n = 0; n < nmanaged; n++, p++) {
		Dimension		bw = (*p)->core.border_width;

					/*
					 * Must be int, to catch <0!
					 */
		int			size;


		constraint = CONSTRAINT(*p);

		/*
		 * The weight constraints dictate how much of the
		 * extra space to give to each child (or how much
		 * of the needed space to steal from each child).
		 * The set-values size of each child is the starting
		 * point for each calculation, regardless of whether
		 * we've resized the children before.
		 */
		if (*p != who_asking) {
			size = constraint->set_size
			+ (dsize * (int)constraint->weight) / total_weight;
			if (size < 0)
				size = 1;
		} else
			size = (VERTICAL(w)? (*p)->core.height : (*p)->core.width);

		if (query == PleaseTry && *p != who_asking) {
			configured++;
			if (VERTICAL(w))
				OlConfigureWidget (
					*p,
					0,
					pos + constraint->space,
					w->core.width - 2 * bw,
					size,
					bw
				);
			else
				OlConfigureWidget (
					*p,
					pos + constraint->space,
					0,
					size,
					w->core.height - 2 * bw,
					bw
				);

		} else if (*p == who_asking && response) {
			configured++;
			if (VERTICAL(w)) {
				response->x = 0;
				response->y = pos + constraint->space;
				response->width  = w->core.width - 2 * bw;
				response->height = size;
				response->border_width = bw;
			} else {
				response->x = pos + constraint->space;
				response->y = 0;
				response->width  = size;
				response->height = w->core.height - 2 * bw;
				response->border_width = bw;
			}
		}

		pos += constraint->space + size + 2 * bw;
	}

	_OlDnDSetDisableDSClipping((Widget)w, False);
	if (configured)
		OlDnDWidgetConfiguredInHier((Widget)w);
	return;
} /* Layout */

/**
 ** Order()
 **/

static int
Order (const void *v1, const void *v2)
{
        Widget *w1 = (Widget *)v1;
        Widget *w2 = (Widget *)v2;
    
	RubberTileConstraintRec * w1constraint = CONSTRAINT(*w1);
	RubberTileConstraintRec * w2constraint = CONSTRAINT(*w2);


	if (w1constraint->ref_widget == *w2)
		return (1);	/* w1 comes after w2 */

	else if (w2constraint->ref_widget == *w1)
		return (-1);	/* w2 comes after w1 */

	else
		return (0);	/* essentially equal */
} /* Order */

/**
 ** ResolveReference()
 **/

static void
ResolveReference (Widget *pRefWidget, String *pRefName, Widget w)
{   
#define	NAME_OF_PARENT	XtName(w->core.parent)

	if (*pRefWidget && *pRefName) {
		XrmQuark xrm_name = XrmStringToQuark(*pRefName);

		if (xrm_name != (*pRefWidget)->core.xrm_name) {
			String		new_name = XtName(*pRefWidget);

			Warning (
				"\
RubberTile \"%s\": refWidget (%s) and refName (%s) don't match.\
",
				NAME_OF_PARENT,
				new_name,
				*pRefName
			);
			FREE (*pRefName);
			*pRefName = Strdup(new_name);
		}

	} else if (*pRefWidget) {
		*pRefName = Strdup(XtName(*pRefWidget));

	} else if (*pRefName) {
		*pRefWidget = FindWidget((RubberTileWidget)w->core.parent, *pRefName);
		if (!*pRefWidget) {
			Warning (
				"\
RubberTile \"%s\": refName \"%s\" is not a known widget.\
",
				NAME_OF_PARENT,
				*pRefName
			);

			FREE (*pRefName);
			*pRefName = Strdup(NAME_OF_PARENT);
			*pRefWidget = w->core.parent;
		}

	} else {
		*pRefName = Strdup(NAME_OF_PARENT);
		*pRefWidget = w->core.parent;
	}

	if (
		(*pRefWidget) != w->core.parent
	     && (*pRefWidget)->core.parent != w->core.parent
	) {
		Warning (
			"\
RubberTile \"%s\": refWidget \"%s\" is neither a sibling nor the parent.\
",
			NAME_OF_PARENT,
			*pRefName
		);
		FREE (*pRefName);
		*pRefName = Strdup(NAME_OF_PARENT);
		*pRefWidget = w->core.parent;
	}

#undef	NAME_OF_PARENT
	return;
} /* ResolveReference */

/**
 ** FindWidget()
 **/

static Widget
FindWidget (RubberTileWidget w, String name)
{
	int			n;

	Widget *		list;

	/*
	 * Compare quarks, not strings. First, it should prove a little
	 * faster, second, it works for gadget children (the string version
	 * of the name isn't in core for gadgets).
	 */
	XrmQuark		xrm_name = XrmStringToQuark(name);


	if (xrm_name == w->core.xrm_name)
		return ((Widget)w);

	list = w->composite.children;
	n = w->composite.num_children;

	while (n--) {
		if (xrm_name == (*list)->core.xrm_name)
			return (*list);
		list++;
	}

	return (0);
} /* FindWidget */

/**
 ** Error()
 ** Warning()
 **/

/*PRINTFLIKE1*/
/*VARARGS1*/
static void
Error (String format, ...)
{
	static char	 *buf;
	va_list			ap;

	if (buf = malloc(512)) {
		va_start (ap, format);
		vsnprintf (buf, 512, format, ap);
		va_end (ap);

		OlError(buf);
		free(buf);
	}
	exit(EXIT_FAILURE);
} /* Error */

/*PRINTFLIKE1*/
/*VARARGS1*/
static void
Warning(String format, ...)
{
	static char  *buf;

	va_list			ap;

	if (buf = malloc(512)) {
		va_start (ap, format);
		vsnprintf (buf, 512, format, ap);
		va_end (ap);

		OlWarning (buf);
		free(buf);
	}
	return;
} /* Warning */
