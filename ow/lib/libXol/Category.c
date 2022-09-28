#pragma ident	"@(#)Category.c	302.10	97/03/26 lib/libXol SMI"	/* category:Category.c 1.12	*/

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


#include <libintl.h>
#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/AbbrevMenu.h>
#include <Xol/CategoryP.h>
#include <Xol/ChangeBar.h>
#include <Xol/ControlAre.h>
#include <Xol/FExclusivP.h>
#include <Xol/OblongButt.h>
#include <Xol/OlI18nP.h>
#include <Xol/OpenLookP.h>
#include <Xol/Util.h>
#include <Xol/memutil.h>


extern int _OlGetScale();
/*
 * New types:
 */

typedef struct DimensionPair {
	Dimension       width;
	Dimension       height;
}               DimensionPair;

typedef struct PositionPair {
	Position        x;
	Position        y;
}               PositionPair;

typedef struct Geometry {
	Position        x;
	Position        y;
	Position        y_baseline;
	Dimension       width;
	Dimension       height;
}               Geometry;

typedef enum HowAsked {
	ParentQueried,
	ChildQueried,
	PleaseTry
}               HowAsked;


/*
 * Macros:
 */

#define AVAILABLE(W) \
	(XtIsManaged(W) || CONSTRAINT(W)->available_when_unmanaged)

#define CONSTRAINT(W) (*(CategoryConstraintRec **)&((W)->core.constraints))

#define WidthFromPoints(P) OlScreenPointToPixel(OL_HORIZONTAL,(P),screen)
#define HeightFromPoints(P) OlScreenPointToPixel(OL_VERTICAL,(P),screen)

#define ComputeChangeBarPosition(W,P) \
	ComputeTopSize((W), (DimensionPair *)0, (P), (Geometry *)0, (PositionPair *)0, (Geometry *)0)

#define ComputeCategoryLabelSize(W,P) \
	ComputeTopSize((W), (DimensionPair *)0, (Geometry *)0, (P), (PositionPair *)0, (Geometry *)0)

#define ComputePageLabelSize(W,P) \
	ComputeTopSize((W), (DimensionPair *)0, (Geometry *)0, (Geometry *)0, (PositionPair *)0, (P))

#define ISFLAG(W,F)	(((CategoryWidget)(W))->category.flags & (F))
#define SETFLAG(W,F)	((CategoryWidget)(W))->category.flags |= (F)
#define CLRFLAG(W,F)	((CategoryWidget)(W))->category.flags &= ~(F)

#define STORE(S)	(S = (S? Strdup(S) : 0))
#define UNSTORE(S)	(S? (FREE(S),0) : 0)
#define RESTORE(O,N)	(UNSTORE(O), STORE(N))

#define OBSOLETE	NULL

/*
 * Global data:
 */

char			XtNavailableWhenUnmanaged
					 [] = "availableWhenUnmanaged";
char			XtNcategoryLabel [] = "categoryLabel";
char			XtNcategoryFont  [] = "categoryFont";
char			XtNchanged       [] = "changed";
char			XtNleftFoot      [] = "leftFoot";
char			XtNnewPage       [] = "newPage";
char			XtNpageLabel     [] = "pageLabel";
char			XtNpageWidth     [] = "pageWidth";
char			XtNpageHeight    [] = "pageHeight";
char			XtNqueryChild    [] = "queryChild";
char			XtNrightFoot     [] = "rightFoot";
char			XtNshowFooter    [] = "showFooter";

char			XtCAvailableWhenUnmanaged
					 [] = "AvailableWhenUnmanaged";
char			XtCCategoryLabel [] = "CategoryLabel";
char			XtCCategoryFont  [] = "CategoryFont";
char			XtCChanged       [] = "Changed";
char			XtCLeftFoot      [] = "LeftFoot";
char			XtCNewPage       [] = "NewPage";
char			XtCPageLabel     [] = "PageLabel";
char			XtCPageWidth     [] = "PageWidth";
char			XtCPageHeight    [] = "PageHeight";
char			XtCQueryChild    [] = "QueryChild";
char			XtCRightFoot     [] = "RightFoot";
char			XtCShowFooter    [] = "ShowFooter";

/*
 * Local routines:
 */

static void		ClassInitialize (
	void
);
static void		Initialize (
	CategoryWidget		request,
	CategoryWidget		new,
	ArgList			args,
	Cardinal *		num_args
);
static void		Destroy (
	CategoryWidget	w
);
static void		Resize (
	CategoryWidget		w
);
static void		ExposeProc (
	CategoryWidget		w,
	XEvent *		pe,
	Region			region
);
static XtGeometryResult	QueryGeometry (
	CategoryWidget		widget,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	preferred
);
static XtGeometryResult	GeometryManager (
	Widget			w,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	reply
);
static void		ChangeManaged (
	CategoryWidget		w
);
static Boolean		SetValues (
	CategoryWidget		current,
	CategoryWidget		request,
	CategoryWidget		new,
	ArgList			args,
	Cardinal *		num_args
);
static void		ConstraintInitialize (
	Widget			request,
	Widget			new
);
static void		ConstraintDestroy (
	Widget			w
);
static Boolean		ConstraintSetValues (
	Widget			current,
	Widget			request,
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
);
static void		CheckForErrors (
	CategoryWidget		w
);
static void		GetGCs (
	CategoryWidget	w
);
static void		FreeGCs (
	CategoryWidget	w
);
static Cardinal		InsertPosition (
	Widget			w
);
static void		FixMenuWidth (
	CategoryWidget		w
);
static void		Layout (
	CategoryWidget		w,
	Boolean			resizable,
	HowAsked		query,
	Widget			who_asking,
	XtWidgetGeometry *	response
);
static void		AdjustForGravity (
	XtWidgetGeometry *	preferred,
	XtWidgetGeometry *	response,
	int			gravity
);
static void		SetWindowGravity (
	Widget			w
);
static Boolean		NeedChangeBar (
	CategoryWidget		w,
	Cardinal		current_page
);
static void		CheckCancelDismiss (
	CategoryWidget		w
);
static void		DrawText (
	CategoryWidget		w,
	Region			region,
	GC			gc,
	Geometry *		g,
	String			text
);
static void		DrawFoot (
	CategoryWidget		w,
	Region			region,
	OlgxAttrs *		attrs,
	Geometry *		g,
	OlgxTextLbl *		lbl
);
static void		DrawBorder (
	Widget			w,
	Region			region,
	OlgxAttrs *		attrs,
	Position		x,
	Position		y,
	Dimension		width,
	Dimension		height
);
static void		ClearChangeBar (
	CategoryWidget		w
);
static void		ClearCategoryLabel (
	CategoryWidget		w
);
static void		ClearPageLabel (
	CategoryWidget		w
);
static void		ClearFoot (
	CategoryWidget		w,
	OlDefine		which
);
static void		ComputeTopSize (
	CategoryWidget		w,
	DimensionPair *		p_top,
	Geometry *		p_change_bar,
	Geometry *		p_category_label,
	PositionPair *		p_abbrev,
	Geometry *		p_page_label
);
static void		ComputeTextSize (
	XFontStruct *		f,
	String			text,
	Dimension *		p_width,
	Dimension *		p_ascent,
	Dimension *		p_descent
);
static void		ComputeFootSize (
	CategoryWidget		w,
	OlgxAttrs *		attr,
	Geometry *		g,
	OlgxTextLbl *		lbl,
	OlDefine		which
);
static Boolean		InRegion (
	Region			region,
	Geometry *		g
);
static void		SetPageCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
);
static void		NextPageCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
);
static void		SetPage (
	CategoryWidget		w,
	Cardinal		new_page,
	Boolean			call_callbacks
);
static int		FindChild (
	CategoryWidget		w,
	Widget			child
);

/*
 * Local data:
 */

static char		Nminimize   [] = "minimize";

static String		MenuFields[]	= {
	XtNlabel,
	XtNset,
	XtNuserData
};

static MenuItem		FakeMenuItems[]	= {
	{ "(no active pages)" }
};

/*
 * Instance resource list:
 */

#define FONT_COLOR \
    {	/* SGI */							\
	XtNfontColor, XtCFontColor,					\
	XtRPixel, sizeof(Pixel), offset(category.font_color),		\
	XtRString, (XtPointer)XtDefaultForeground			\
    }

static _OlDynResource	dynamic_resources[] = {
#define offset(F) XtOffsetOf(CategoryRec, F)
#define DYNFLAG   offset(category.dynamics)

  { {
	XtNbackground, XtCBackground,
	XtRPixel, sizeof(Pixel), 0,
	XtRString, XtDefaultBackground
    },
	DYNFLAG, _CATEGORY_B_DYNAMIC_BACKGROUND, NULL
  },
  {	FONT_COLOR,
	DYNFLAG, _CATEGORY_B_DYNAMIC_FONTCOLOR, NULL
  },

#undef	offset
#undef	DYNFLAG
};

static XtResource resources[] = {
#define offset(F) XtOffsetOf(CategoryRec, F)

    {	/* SGI */
	XtNlayoutWidth, XtCLayoutWidth,
	XtROlDefine, sizeof(OlDefine), offset(category.layout.width),
	XtRImmediate, (XtPointer)Nminimize
    },
    {	/* SGI */
	XtNlayoutHeight, XtCLayoutHeight,
	XtROlDefine, sizeof(OlDefine), offset(category.layout.height),
	XtRImmediate, (XtPointer)Nminimize
    },
    {	/* SGI */
	XtNpageWidth, XtCPageWidth,
	XtRDimension, sizeof(Dimension), offset(category.page.width), 
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNpageHeight, XtCPageHeight,
	XtRDimension, sizeof(Dimension), offset(category.page.height), 
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNcategoryLabel, XtCCategoryLabel,
	XtRString, sizeof(String), offset(category.category_label),
	XtRImmediate, (XtPointer)"CATEGORY:"
    },
    {	/* SGI */
	XtNcategoryFont, XtCCategoryFont,
	XtRFontStruct, sizeof(XFontStruct *), offset(category.category_font),
	XtRString, (XtPointer)OlDefaultBoldFont
    },
    {	/* SGI */
	XtNfont, XtCFont,
	XtRFontStruct, sizeof(XFontStruct *), offset(category.font),
	XtRString, (XtPointer)XtDefaultFont
    },
    FONT_COLOR,
    {	/* SGI */
	XtNleftFoot, XtCLeftFoot,
	XtRString, sizeof(String), offset(category.left_foot),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNrightFoot, XtCRightFoot,
	XtRString, sizeof(String), offset(category.right_foot),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNshowFooter, XtCShowFooter,
	XtRBoolean, sizeof(Boolean), offset(category.show_footer),
	XtRImmediate, (XtPointer)False
    },
    {	/* SI */
	XtNnewPage, XtCCallback,
	XtRCallback, sizeof(XtPointer), offset(category.new_page),
	XtRCallback, (XtPointer)0
    },
    {	/* SGI */
	XtNlowerControlArea, XtCLowerControlArea,
	XtRPointer, sizeof(Widget), offset(category.lower_control_area), 
	XtRImmediate, (XtPointer)0
    },

#undef offset
};

/*
 * Constraint resource list:
 */

static XtResource	constraintResources[] = {
#define offset(F) XtOffsetOf(CategoryConstraintRec, F)

    {	/* SGI */
	XtNpageLabel, XtCPageLabel,
	XtRString, sizeof(String), offset(page_label),
	XtRImmediate, (XtPointer)0
    },
    {	/* SGI */
	XtNgravity, XtCGravity,
	XtRGravity, sizeof(int), offset(gravity),
	XtRImmediate, (XtPointer)CenterGravity
    },
    {	/* GI */
	XtNdefault, XtCDefault,
	XtRBoolean, sizeof(Boolean), offset(_default),
	XtRImmediate, (XtPointer)False
    },
    {	/* SGI */
	XtNchanged, XtCChanged,
	XtRBoolean, sizeof(Boolean), offset(changed),
	XtRImmediate, (XtPointer)False
    },
    {	/* SGI */
	XtNavailableWhenUnmanaged, XtCAvailableWhenUnmanaged,
	XtRBoolean, sizeof(Boolean), offset(available_when_unmanaged),
	XtRImmediate, (XtPointer)False
    },
    {	/* SGI */
	XtNqueryChild, XtCQueryChild,
	XtRBoolean, sizeof(Boolean), offset(query_child),
	XtRImmediate, (XtPointer)True
    },

#undef	offset
};

/*
 * Full class record constant:
 */

CategoryClassRec	categoryClassRec = {
    /*
     * Core class:
     */
    {
    /* superclass          */	(WidgetClass)&managerClassRec,
    /* class_name          */	"Category",
    /* widget_size         */	sizeof(CategoryRec),
    /* class_initialize    */	ClassInitialize,
    /* class_part_init     */	NULL,
    /* class_inited        */	FALSE,
    /* initialize          */	(XtInitProc)Initialize,
    /* initialize_hook     */	OBSOLETE,
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
    /* destroy             */	(XtWidgetProc)Destroy,
    /* resize              */	(XtWidgetProc)Resize,
    /* expose              */	(XtExposeProc)ExposeProc,
    /* set_values          */	(XtSetValuesFunc)SetValues,
    /* set_values_hook     */	OBSOLETE,
    /* set_values_almost   */	XtInheritSetValuesAlmost,
    /* get_values_hook     */	NULL,
    /* accept_focus        */	XtInheritAcceptFocus,
    /* version             */	XtVersion,
    /* callback_private    */	NULL,
    /* tm_table            */	XtInheritTranslations,
    /* query_geometry      */	(XtGeometryHandler)QueryGeometry,
    },

    /*
     * Composite class:
     */
    {
    /* geometry_manager    */	GeometryManager,
    /* change_managed      */	(XtWidgetProc)ChangeManaged,
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
    /* constraint_size     */	sizeof(CategoryConstraintRec),
    /* initialize          */	(XtInitProc)ConstraintInitialize,
    /* destroy             */	(XtWidgetProc)ConstraintDestroy,
    /* set_values          */	(XtSetValuesFunc)ConstraintSetValues,
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
    /* dyn_data            */	{ dynamic_resources, XtNumber(dynamic_resources) },
    /* transparent_proc    */	_OlDefaultTransparentProc,
    },

    /*
     * Category class:
     */
    {
    /* empty               */	0,
    }
};


WidgetClass		categoryWidgetClass = (WidgetClass)&categoryClassRec;


/*
 * OlCategorySetPage()
 */

Boolean
OlCategorySetPage(
	CategoryWidget	w,
	Widget		child
)
{
	int             i;

	/*
	 * If the category widget has not yet been realized, then its
	 * change_managed procedure won't have been called yet so there'll be
	 * no page list yet. We thus delay the setting of the page until we
	 * have the list.
	 */
	if (!XtIsRealized((Widget)w)) {
		w->category.delayed_set_page_child = child;
		return (FALSE);
	}
	
	if ((i = FindChild(w, child)) >= 0)
		SetPage(w, (Cardinal)i, False);
	else {
#define MESSAGE "OlCategorySetPage: \"%1$s\" is not the parent of \"%2$s\"\n"

		String          w_name = XtName((Widget)w);
		String          child_name = XtName(child);
		char 		*buf;

		/* Fix for 4008133 - Security risk in lib Xol   */
		if (buf = malloc(512)) {
			snprintf(buf, 512, dgettext(OlMsgsDomain, MESSAGE), 
						w_name, child_name);
			OlWarning(buf);
			free(buf);
		}
	}
	
	return (NeedChangeBar(w, w->category.current_page));
} /* OlCategorySetPage */


/**
 ** ClassInitialize()
 **/

static void
ClassInitialize (void)
{
	_OlAddOlDefineType ("maximize", OL_MAXIMIZE);
	_OlAddOlDefineType ("minimize", OL_MINIMIZE);
	_OlAddOlDefineType ("ignore",   OL_IGNORE);

	return;
} /* ClassInitialize */

/**
 ** Initialize()
 **/

static void
Initialize (
	CategoryWidget		request,
	CategoryWidget		new,
	ArgList			args,
	Cardinal *		num_args
)
{
	Widget			menu_pane;
	Widget			page_choice;
	Widget			next_page;
	Widget			child_list[2];


	CheckForErrors (new);

	new->category.page_list              = 0;
	new->category.first_page_child       = 0;
	new->category.page_list_size         = 0;
	new->category.current_page           = 0;
	new->category.delayed_set_page_child = 0;

	STORE (new->category.left_foot);
	STORE (new->category.right_foot);

	new->category.cb = _OlCreateChangeBar ((Widget)new);

	/*
	 * We want internal children to be in a particular order,
	 * thus we provide an "insert_position" procedure. We
	 * allow the client to also provide its version of the
	 * procedure.
	 */
	new->category.insert_position  = new->composite.insert_position;
	new->composite.insert_position = InsertPosition;

	if (!new->category.category_label)
		new->category.category_label = XtName((Widget)new);
	new->category.category_label = Strdup(new->category.category_label);

	GetGCs (new);

	/*
	 * Don't manage any internal widgets until we have everything
	 * ready for the "change_managed" procedure.
	 */
	SETFLAG (new, _CATEGORY_INTERNAL_CHILD);

	child_list[0] = XtVaCreateWidget(
		"abbrevMenu",
		abbrevMenuButtonWidgetClass,
		(Widget)new,
		(String)0
	);
	XtVaGetValues (
		child_list[0],
		XtNmenuPane, (XtArgVal)&menu_pane,
		(String)0
	);

	/*
	 * Unless we give a non-empty list of items, the flat exclusives
	 * widget will complain. Since we don't have any page-children
	 * yet, we'll use a fake list.
	 */
	page_choice = XtVaCreateManagedWidget(
		"pageChoice",
		flatExclusivesWidgetClass,
		menu_pane,
		XtNnumItems,      (XtArgVal)1,
		XtNitems,         (XtArgVal)FakeMenuItems,
		XtNnumItemFields, (XtArgVal)XtNumber(MenuFields),
		XtNitemFields,    (XtArgVal)MenuFields,
		XtNselectProc,    (XtArgVal)SetPageCB,
		XtNclientData,    (XtArgVal)new,
		XtNlayoutType,    (XtArgVal)OL_FIXEDCOLS,
		XtNlayoutWidth,   (XtArgVal)OL_MINIMIZE,
		XtNlabelJustify,  (XtArgVal)OL_LEFT,
		XtNfont,          (XtArgVal)new->category.font,
		(String)0
	);

	next_page = XtVaCreateManagedWidget(
		"Next page",
		oblongButtonGadgetClass,
		menu_pane,
		XtNdefault,      (XtArgVal)True,
		XtNlabelJustify, (XtArgVal)OL_LEFT,
		(String)0
	);
	XtAddCallback (next_page, XtNselect, NextPageCB, (XtPointer)new);

	child_list[1] = XtVaCreateWidget(
		"lowerControlArea",
		controlAreaWidgetClass,
		(Widget)new,
		(String)0
	);

	new->category.page_choice        = page_choice;
	new->category.next_page	         = next_page;
	new->category.lower_control_area = child_list[1];

	XtManageChildren (child_list, XtNumber(child_list));

	CLRFLAG (new, _CATEGORY_INTERNAL_CHILD);

	return;
} /* Initialize */


/*
 *Destroy()
 */

static void
Destroy (
	CategoryWidget	w
)
{
	UNSTORE (w->category.category_label);
	UNSTORE ((char *)w->category.page_list);
	UNSTORE (w->category.left_foot);
	UNSTORE (w->category.right_foot);

	FreeGCs (w);

	if (w->category.cb)
		_OlDestroyChangeBar ((Widget)w, w->category.cb);

	return;
} /* Destroy */


/*
 *Resize()
 */

static void
Resize(
	CategoryWidget w
)
{

	Layout(w, False, PleaseTry, (Widget)0, (XtWidgetGeometry *)0);
	
	return;
} /* Resize */


/*
 * ExposeProc()
 */

static void
ExposeProc(
	CategoryWidget	w,
	XEvent	       *pe,
	Region		region
)
{
	Geometry        change_bar;
	Geometry        category_label;
	Geometry        page_label;
	Geometry        foot;
	DimensionPair   top;
	OlgxTextLbl      lbl;
	Boolean         full_width;


	/*
	 * CATEGORY label, page label:
	 */
	ComputeTopSize(
		w,
		&top,
		&change_bar,
		&category_label,
		(PositionPair *) 0,
		&page_label
		);

	if (NeedChangeBar(w, w->category.current_page))
		_OlDrawChangeBar(
			(Widget) w,
			w->category.cb,
			OL_NORMAL,
			False,
			change_bar.x,
			change_bar.y,
			region
			);

	DrawText(
		w,
		region,
		w->category.category_font_GC,
		&category_label,
		w->category.category_label
		);

	if (w->category.page_list)
		DrawText(
			w,
			region,
			w->category.font_GC,
			&page_label,
			w->category.page_list[w->category.current_page].label
			);

	/*
	 * Footers:
	 */
	foot.height = 0;	/* initialize, in case no footers */
	
	if (w->category.show_footer) {

#		define SIZEIT(LBL,WHICH) \
		    ComputeFootSize(w, w->category.attrs, &foot, (LBL), (WHICH))
	
#		define DRAWIT(LBL) \
			DrawFoot(w, region, w->category.attrs, &foot, (LBL))

		if (w->category.left_foot) {
			SIZEIT(&lbl, OL_LEFT);
			DRAWIT(&lbl);
		}
		
		if (w->category.right_foot) {
			SIZEIT(&lbl, OL_RIGHT);
			DRAWIT(&lbl);
		}
		
		if (!w->category.left_foot && !w->category.right_foot)
			SIZEIT(&lbl, OL_BOTH);

#		undef	SIZEIT
#		undef	DRAWIT
	}

	/*
	 * Border:
	 */
	DrawBorder(
		(Widget)w,
		region,
		w->category.attrs,
		0,		/* x */
		top.height,	/* y */
		w->core.width,	/* width */
		w->core.height - top.height - foot.height	/* height */
		);

	return;
} /* ExposeProc */

/*
 * SetValues()
 */
 
static Boolean
SetValues (
	CategoryWidget		current,
	CategoryWidget		request,
	CategoryWidget		new,
	ArgList			args,
	Cardinal *		num_args
)
{
	CategoryPart *		newPart	= (CategoryPart *)&(new->category);
	CategoryPart *		curPart	= (CategoryPart *)&(current->category);

	Boolean			do_layout	= False;
	Boolean			redisplay	= False;
	Boolean			get_GCs		= False;


	CheckForErrors (new);

#define DIFFERENT(FIELD) (curPart->FIELD != newPart->FIELD)

	/*
	 * If the client wants OL_MAXIMIZE or OL_MINIMIZE, then
	 * it can't try to set the size directly.
	 */
	if (curPart->layout.width != OL_IGNORE)
		new->core.width = current->core.width;
	if (curPart->layout.height != OL_IGNORE)
		new->core.height = current->core.height;

	/*
	 * We play a subtle trick here: We aren't supposed to do any
	 * (re)displaying in this routine, but should just return a
	 * Boolean that indicates whether the Intrinsics should force
	 * a redisplay. But the Intrinsics does this by ``calling the
	 * Xlib XClearArea() function on the [parent] widget's window.''
	 * This would cause the entire widget to need a redisplay!
	 * Thus, instead of returning True here, we instead call
	 * XClearArea ourselves on just the area of the label and return
	 * False. The XClearArea() (done inside ClearCategoryLabel()) will
	 * generate an expose event for a much smaller region.
	 */
	if (DIFFERENT(category_label)) {
		ClearCategoryLabel (current);
		UNSTORE (curPart->category_label);
		if (!newPart->category_label)
			newPart->category_label = XtName((Widget)new);
		newPart->category_label = Strdup(newPart->category_label);
		do_layout = True;
	}

	if (current->core.background_pixel != new->core.background_pixel){
		if (new->category.cb)
			_OlFreeChangeBarGCs ((Widget)new, new->category.cb);
		get_GCs = True;
	}
	if (
		DIFFERENT(font_color)
	     || DIFFERENT(category_font->fid)
	     || DIFFERENT(font->fid)
	)
		/*
		 * MORE: If just the font or font color changed, use
		 * the same trick as above to avoid full exposure.
		 */
		get_GCs = True;

	if (get_GCs) {
		FreeGCs (new);
		GetGCs (new);
		redisplay = True;
	}

	/*
	 * Use the same trick described above to avoid refreshing
	 * the entire window when just a footer changes.
	 */
	if (DIFFERENT(left_foot) || DIFFERENT(right_foot)) {
		/*
		 * If we go from having a footer to not have a footer,
		 * or vice versa, we have to recompute our layout.
		 */
		if (
			!newPart->left_foot && !newPart->right_foot
		     || !curPart->left_foot && !curPart->right_foot
		)
			do_layout = True;

		/*
		 * If both the old and new footer are empty (but not null)
		 * then don't bother with the trick, because nothing will
		 * be erased and nothing needs to be drawn. But do re-
		 * allocate, to keep our copy freeable.
		 */
#define EMPTY(P)	  ((P) && !*(P))
#define BOTH_EMPTY(FIELD) (EMPTY(newPart->FIELD) && EMPTY(curPart->FIELD))
		if (DIFFERENT(left_foot)) {
			if (!BOTH_EMPTY(left_foot))
				ClearFoot (current, OL_LEFT);
			RESTORE (curPart->left_foot, newPart->left_foot);
		}
		if (DIFFERENT(right_foot)) {
			if (!BOTH_EMPTY(right_foot))
				ClearFoot (current, OL_RIGHT);
			RESTORE (curPart->right_foot, newPart->right_foot);
		}
#undef	BOTH_EMPTY
#undef	EMPTY
	}

	if (DIFFERENT(page.width) || DIFFERENT(page.height))
		do_layout = True;

	/*
	 * We always use a special "insert_position" procedure,
	 * so replace what the client gave (but save it so we can
	 * call it).
	 */
	if (current->composite.insert_position != new->composite.insert_position) {
		new->category.insert_position = new->composite.insert_position;
		new->composite.insert_position = InsertPosition;
	}

	if (do_layout)
		Layout (new, True, PleaseTry, (Widget)0, (XtWidgetGeometry *)0);

#undef DIFFERENT

	return (redisplay);
} /* SetValues */

/*
 * QueryGeometry()
 */

static          XtGeometryResult
QueryGeometry(
	CategoryWidget		w,
	XtWidgetGeometry       *request,
	XtWidgetGeometry       *preferred
)
{
	XtWidgetGeometry response;
	XtGeometryResult result = XtGeometryYes;

	/*
	 * We don't care about our position, we will suggest a best size, and
	 * we really don't want a border.
	 */
	*preferred = *request;
	preferred->request_mode &= CWWidth | CWHeight | CWBorderWidth;

	Layout(w, True, ParentQueried, (Widget) 0, &response);
	
	preferred->width = response.width;
	preferred->height = response.height;
	preferred->border_width = 0;

#define CHECK(BIT,F) \
	if (request->request_mode & BIT) {				\
		if (preferred->F != request->F) {			\
			result = XtGeometryAlmost;			\
			if (preferred->F == w->core.F)			\
				result = XtGeometryNo;			\
		}							\
	} else

	CHECK(CWWidth, width);
	CHECK(CWHeight, height);
	CHECK(CWBorderWidth, border_width);
	
#undef	CHECK

	/*
	 * If the caller didn't ask about a border width, tell them we care.
	 * This means returning XtGeometryAlmost instead of XtGeometryYes.
	 */
	if (!(request->request_mode & CWBorderWidth))
		if (result == XtGeometryYes)
			result = XtGeometryAlmost;

	return (result);
} /* QueryGeometry */


/**
 ** GeometryManager()
 **/

static XtGeometryResult
GeometryManager (
	Widget			w,
	XtWidgetGeometry *	_request,
	XtWidgetGeometry *	reply
)
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
		(CategoryWidget)XtParent(w),
		True,
		(request.request_mode & XtCWQueryOnly?
			  ChildQueried
			: PleaseTry
		),
		w,
		&response
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
	 * When returning XtGeometryYes, we have to update the
	 * widgets geometry to reflect the requested values.
	 * This includes the x and y position, as returned to
	 * this routine from "Layout()" in the "response" structure.
	 * Note: If the child had wanted different x,y values,
	 * we wouldn't be returning XtGeometryYes.
	 */
	if (result == XtGeometryYes) {
		w->core.x = response.x;
		w->core.y = response.y;
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

	return (result);
} /* GeometryManager */


/*
 **ChangeManaged()
 */

static void
ChangeManaged (
	CategoryWidget		w
)
{
	Cardinal		first		= w->category.first_page_child;
	Cardinal		nchildren	= w->composite.num_children;
	Cardinal		size		= 0;
	Cardinal		i;
	Cardinal		n;

	Boolean			have_current_page	= False;
	Boolean			current_page_has_changed= False;

	Widget			current		= 0;

	Widget *		child		= w->composite.children;

	CategoryConstraintRec *	constraint;

	MenuItem *		list		= w->category.page_list;


	/*
	 * Don't waste time with special (internal) children.
	 */
	if (ISFLAG(w, _CATEGORY_INTERNAL_CHILD))
		return;


	/*
	 * Save the current page's widget for later. If we have
	 * a delayed OlCategorySetPage() call pending, use the
	 * page widget we set aside, instead.
	 */
	if (w->category.delayed_set_page_child) {
		current = w->category.delayed_set_page_child;
		w->category.delayed_set_page_child = 0;
	} else if (list)
		current = (Widget)list[w->category.current_page].user_data;

	/*
	 * Count the number of pages.
	 * While we're here, set the window gravity if it hasn't
	 * been set yet.
	 */
	for (n = first; n < nchildren; n++)
		if (AVAILABLE(child[n])) {
			size++;
			SetWindowGravity (child[n]);
		}

	/*
	 * (Re)allocate space for the menu list.
	 */
	if (size != w->category.page_list_size) {
		w->category.page_list =
		    list = (MenuItem *) Array((char *)list, MenuItem, size);
		w->category.page_list_size = size;
	}

	/*
	 * Fill the (new) array, and find the new index of the
	 * current page. Clear all mapped-when-managed flags here.
	 * Once we have the current page we'll mark it for mapping.
	 */
	w->category.current_page = 0;
	for (n = first, i = 0; n < nchildren; n++) {
		if (AVAILABLE(child[n])) {
			list[i].label  = CONSTRAINT(child[n])->page_label;
			list[i].set    = False;
			list[i].user_data = (XtPointer)child[n];
			if (child[n] == current) {
				have_current_page = True;
				w->category.current_page = i;
			}
			child[n]->core.mapped_when_managed = False;
			i++;
		}
	}

	/*
	 * If we don't have a current page (any more), find a page
	 * that has XtNdefault TRUE.
	 *
	 * Checking to see if more than one child is added with
	 * XtNdefault TRUE is not a good idea. As it is now, an
	 * application can change the default to a widget being
	 * added in one shot. Insisting that only one child
	 * has XtNdefault TRUE would force an application to
	 * clear the flag on an old widget before setting it
	 * for a new widget. We just use reasonable defaults:
	 *
	 *	- if no children have XtNdefault TRUE, use the first
	 *	  child
	 *
	 *	- if more then one child has XtNdefault TRUE, use the
	 *	  last child added (or set)
	 */
	if (!have_current_page && size) {
		current_page_has_changed = True;
		for (i = 0; i < size; i++) {
			constraint = CONSTRAINT((Widget)list[i].user_data);
			if (constraint->_default) {
				have_current_page = True;
				w->category.current_page = i;
			}
		}
		if (!have_current_page)
			w->category.current_page = 0;
	}

	/*
	 * If we have a non-empty list we now have a current page.
	 * Set the mapped-when-managed flag for that page, so that
	 * it gets displayed automatically by the Intrinsics, and
	 * mark the corresponding menu item as current.
	 */
	if (size) {
		list[w->category.current_page].set = True;
		((Widget)(list[w->category.current_page].user_data))
					->core.mapped_when_managed = True;
	}

	/*
	 * If the page-list is (now) empty, we may have to
	 * replace the flat exclusives menu with a fake menu,
	 * to avoid problems with an empty list.
	 */
	SETFLAG(w, _CATEGORY_INTERNAL_CHILD);
	if (size) {
		XtVaSetValues (
			w->category.page_choice,
			XtNnumItems,    (XtArgVal)size,
			XtNitems,       (XtArgVal)list,
			XtNlayoutWidth, (XtArgVal)OL_MINIMIZE,
			(String)0
		);
	} else {
		XtVaSetValues (
			w->category.page_choice,
			XtNnumItems,    (XtArgVal)1,
			XtNitems,       (XtArgVal)FakeMenuItems,
			(String)0
		);
	}
	CLRFLAG(w, _CATEGORY_INTERNAL_CHILD);
	FixMenuWidth(w);

	/*
	 * Being here means the number of potentially displayable pages
	 * has changed--thus we must recalculate our layout.
	 */
 	Layout(w, True, PleaseTry, (Widget)0, (XtWidgetGeometry *)0);

	if (current_page_has_changed)
		ClearPageLabel(w);

	return;
} /* ChangeManaged */

/**
 ** ConstraintInitialize()
 **/

static void
ConstraintInitialize (
	Widget			request,
	Widget			new
)
{
	CategoryConstraintRec *	constraint = CONSTRAINT(new);

	CategoryWidget		parent	   = (CategoryWidget)XtParent(new);


	if (ISFLAG(parent, _CATEGORY_INTERNAL_CHILD))
		return;

	if (!constraint->page_label)
		constraint->page_label = XtName(new);
	constraint->page_label = Strdup(constraint->page_label);

	constraint->window_gravity_set = False;

	/*
	 * The XtNdefault resource is checked in the "change_managed"
	 * procedure.
	 */

	return;
} /* ConstraintInitialize */

/**
 ** ConstraintDestroy()
 **/

static void
ConstraintDestroy (
	Widget			w
)
{
	CategoryConstraintRec *	constraint = CONSTRAINT(w);


	if (constraint->page_label)
		FREE (constraint->page_label);

	/*
	 * The "change_managed" procedure has already been called
	 * and this page-child has been removed from the menu.
	 */

	return;
} /* ConstraintDestroy */

/**
 ** ConstraintSetValues()
 **/

static Boolean
ConstraintSetValues (
	Widget			current,
	Widget			request,
	Widget			new,
	ArgList			args,
	Cardinal *		num_args
)
{
	CategoryConstraintRec * curConstraint	= CONSTRAINT(current);
	CategoryConstraintRec * newConstraint	= CONSTRAINT(new);

	CategoryWidget		parent		= (CategoryWidget)XtParent(new);

	Boolean			do_layout	= False;
	Boolean			redisplay	= False;


	if (ISFLAG(parent, _CATEGORY_INTERNAL_CHILD))
		return (False);

#define DIFFERENT(FIELD) (curConstraint->FIELD != newConstraint->FIELD)

	if (DIFFERENT(page_label)) {
		int			i	= FindChild(parent, new);


		FREE (curConstraint->page_label);
		if (!newConstraint->page_label)
			newConstraint->page_label = XtName(new);
		newConstraint->page_label = Strdup(newConstraint->page_label);

		if (i >= 0) {
			parent->category.page_list[i].label
					= newConstraint->page_label;
			SETFLAG (parent, _CATEGORY_INTERNAL_CHILD);
			XtVaSetValues (
				parent->category.page_choice,
				XtNitemsTouched, (XtArgVal)True,
				XtNlayoutWidth,  (XtArgVal)OL_MINIMIZE,
				(String)0
			);
			CLRFLAG (parent, _CATEGORY_INTERNAL_CHILD);
			FixMenuWidth (parent);
		}

		/*
		 * We play a subtle trick here: We aren't supposed to do
		 * any (re)displaying in this routine, but should just
		 * return a Boolean that indicates whether the Intrinsics
		 * should force a redisplay. But the Intrinsics does this
		 * by ``calling the Xlib XClearArea() function on the
		 * [parent] widget's window.'' This would cause the entire
		 * widget to need a redisplay! Thus, instead of returning
		 * True here, we instead call XClearArea ourselves on just
		 * the area of the label and return False.
		 *
		 * The XClearArea() (done inside ClearPageLabel()) will
		 * generate a much smaller expose event.
		 */
		if (parent->category.current_page == i) {
			ClearPageLabel (parent);
			do_layout = True;
		}
	}

	if (DIFFERENT(changed)) {
		/*
		 * Same trick as above.
		 */
		ClearChangeBar (parent);
		CheckCancelDismiss (parent);
	}

	if (DIFFERENT(gravity)) {
		do_layout = True;
		newConstraint->window_gravity_set = False;
		SetWindowGravity (new);
	}

	/*
	 * XtNdefault is GI, so there's nothing to do with it here.
	 */


	if (do_layout)
		Layout (parent, True, PleaseTry, (Widget)0, (XtWidgetGeometry *)0);

#undef	DIFFERENT
	return (redisplay);
} /* ConstraintSetValues */

/**
 ** CheckForErrors()
 **/

static void
CheckForErrors (
	CategoryWidget		w
)
{
#define CHECK(DIR) \
	switch (w->category.layout.DIR) {				\
									\
	case OL_MINIMIZE:						\
	case OL_MAXIMIZE:						\
	case OL_IGNORE:							\
		break;							\
									\
	default:							\
		OlWarning (dgettext(OlMsgsDomain,			\
			"Category: Invalid layout, using \"minimize\"")); \
		w->category.layout.DIR = OL_MINIMIZE;			\
		break;							\
									\
	}

	CHECK (width);
	CHECK (height);
#undef	CHECK

	return;
} /* CheckForErrors */

/**
 ** GetGCs()
 ** FreeGCs()
 **/

static void
GetGCs (
	CategoryWidget	w
)
{
	XGCValues		v;


	/*
	 * WARNING:
	 * Never call this routine if the GC is already in the widget
	 * structure. Call FreeGCs() first if a new GC is needed,
	 * so that the old GC is released.
	 */

	v.foreground = w->category.font_color;
	v.font       = w->category.category_font->fid;
	v.background = w->core.background_pixel;
	w->category.category_font_GC = XtGetGC(
		(Widget)w,
		GCForeground | GCFont | GCBackground,
		&v
	);

	v.foreground = w->category.font_color;
	v.font       = w->category.font->fid;
	v.background = w->core.background_pixel;
	w->category.font_GC = XtGetGC(
		(Widget)w,
		GCForeground | GCFont | GCBackground,
		&v
	);

	w->category.attrs = OlgxCreateAttrs(
		(Widget)w,
		w->category.font_color,
		(OlgxBG *)&(w->core.background_pixel),
		False,
		(unsigned)_OlGetScale(XtScreen(w)),
		OL_SB_STR_REP, (OlFont)w->category.font
	);

	return;
} /* GetGCs */

static void
FreeGCs(CategoryWidget w)
{

	if (w->category.category_font_GC) {
		XtReleaseGC((Widget)w, w->category.category_font_GC);
		w->category.category_font_GC = 0;
	}
	
	if (w->category.font_GC) {
		XtReleaseGC((Widget)w, w->category.font_GC);
		w->category.font_GC = 0;
	}
	
	if (w->category.attrs)
		OlgxDestroyAttrs((Widget)w, w->category.attrs);

	return;
} /* FreeGCs */

/**
 ** InsertPosition()
 **/

static Cardinal
InsertPosition (Widget w)
{
	CategoryWidget		parent	= (CategoryWidget)XtParent(w);

	Cardinal		pos;


	/*
	 * If we are inserting an internal widget, put it up front
	 * in the list and update the first-page-child index.
	 * If we are inserting a page-child, call the client's
	 * insert position procedure (if any), but bump the
	 * client's position to keep the widget after our internal
	 * widgets.
	 */
	if (ISFLAG(parent, _CATEGORY_INTERNAL_CHILD))
		pos = parent->category.first_page_child++;
	else
		if (parent->category.insert_position)
			pos = (*parent->category.insert_position)(w)
			    + parent->category.first_page_child;
		else
			pos = parent->composite.num_children;

	return (pos);
} /* InsertPosition */

/**
 ** FixMenuWidth()
 **/

static void
FixMenuWidth (CategoryWidget w)
{
	Dimension		flat_width;
	Dimension		next_width;


	/*
	 * We want to force the flat exclusives and the ``next''
	 * button to be the same width, so that the menu looks nice.
	 */

	XtVaGetValues (
		w->category.page_choice,
		XtNwidth, (XtArgVal)&flat_width,
		(String)0
	);
	XtVaGetValues (
		w->category.next_page,
		XtNwidth, (XtArgVal)&next_width,
		(String)0
	);

	SETFLAG (w, _CATEGORY_INTERNAL_CHILD);
	if (flat_width < next_width) {
		XtVaSetValues (
			w->category.page_choice,
			XtNwidth,       (XtArgVal)next_width,
			XtNlayoutWidth, (XtArgVal)OL_IGNORE,
			(String)0
		);
	} else {
		XtVaSetValues (
			w->category.next_page,
			XtNwidth, (XtArgVal)flat_width,
			(String)0
		);
	}
	CLRFLAG (w, _CATEGORY_INTERNAL_CHILD);

	return;
} /* FixMenuWidth */


/*
 * Layout()
 */

static void
Layout (
	CategoryWidget		w,
	Boolean			resizable,
	HowAsked		query,
	Widget			who_asking,
	XtWidgetGeometry *	response
)
{
	Screen *		screen		= XtScreenOfObject((Widget)w);

	Cardinal		first		= w->category.first_page_child;
	Cardinal		nchildren	= w->composite.num_children;
	Cardinal		i;

	Widget *		child		= w->composite.children;

	Dimension		width;
	Dimension		height;
	Dimension		hThickness;	/* horizontal line */
	Dimension		vThickness;	/* vertical line   */

	DimensionPair		top;
	DimensionPair		largest_page;
	DimensionPair		best_fit;
	DimensionPair		request;

	int			largest_width;	/* must be int      */
	int			largest_height;	/* must be int, too */

	Geometry		lca;
	Geometry		foot;

	OlgxTextLbl		lbl;

	PositionPair		abbrev;


	ComputeTopSize (w, &top, (Geometry *)0, (Geometry *)0, &abbrev, (Geometry *)0);

	lca.width  = child[1]->core.width;
	lca.height = child[1]->core.height;

	foot.height = 0;	/* initialize, in case no footers */
	if (w->category.show_footer)
		ComputeFootSize (w, w->category.attrs, &foot, &lbl, OL_BOTH);

	/*
	 * Find size of largest page.
	 */
	largest_page.width = largest_page.height = 0;
	for (i = first; i < nchildren; i++)
		if (XtIsManaged(child[i])) {

			width = child[i]->core.width
			      + (2 * child[i]->core.border_width);
			if (width > largest_page.width)
				largest_page.width = width;

			height = child[i]->core.height
			       + (2 * child[i]->core.border_width);
			if (height > largest_page.height)
				largest_page.height = height;

		}

	/*
	 * If the layout is OL_IGNORE and we were given a page
	 * size, use it in place of the largest page dimension.
	 */
	if (
		w->category.layout.width == OL_IGNORE
	     && w->category.page.width
	)
		largest_page.width = w->category.page.width;
	if (
		w->category.layout.height == OL_IGNORE
	     && w->category.page.height
	)
		largest_page.height = w->category.page.height;

	/*
	 * Every page ``includes'' the button box across the bottom.
	 * (Not really, but we draw it to appear so.)
	 */
	if (largest_page.width < lca.width)
		largest_page.width = lca.width;
	largest_page.height += lca.height;

	/*
	 * Add space for the border around the page.
	 */
	hThickness = (Dimension)OlgxScreenPointToPixel(OL_VERTICAL,
				    2, screen);
	vThickness = (Dimension)OlgxScreenPointToPixel(OL_HORIZONTAL,
				    2, screen);
	largest_page.width  += 2 * vThickness;
	largest_page.height += 2 * hThickness;

	/*
	 * This is the ``just fits'' size. Note that the footer
	 * width doesn't count (it is truncated to fit, if too wide).
	 */
	best_fit.width  = _OlMax(top.width, largest_page.width);
	best_fit.height = top.height + largest_page.height + foot.height;

	/*
	 * If the "resizable" flag allows:
	 *
	 *	 If OL_MINIMIZE, request the size that fits.
	 *
	 *	 If OL_MAXIMIZE, request the larger of the current size
	 *	 or the size that fits.
	 *
	 *	 If OL_IGNORE but the client hasn't given a size yet,
	 *	 request the size that fits.
	 */

	request.width	= w->core.width;
	request.height	= w->core.height;

#define DECISION(DIR) \
	switch (w->category.layout.DIR) {				\
									\
	case OL_MINIMIZE:						\
		request.DIR = best_fit.DIR;				\
		break;							\
									\
	case OL_MAXIMIZE:						\
		if (best_fit.DIR > request.DIR)				\
			request.DIR = best_fit.DIR;			\
		break;							\
									\
	case OL_IGNORE:							\
		if (!request.DIR)					\
			request.DIR = best_fit.DIR;			\
		break;							\
									\
	}

	if (resizable) {

	    DECISION (width);
	    DECISION (height);

	    if (
		query != ParentQueried
	     && (	request.width != w->core.width
		     || request.height != w->core.height
		)
	    ) {
		XtWidgetGeometry	ask;
		XtWidgetGeometry	get;
		XtGeometryResult	ans;

		ask.width  = request.width;
		ask.height = request.height;
		ask.request_mode = (CWWidth|CWHeight);
		if (query == ChildQueried)
			ask.request_mode |= XtCWQueryOnly;
		ans = XtMakeGeometryRequest((Widget)w, &ask, &get);
		if (query != ChildQueried && ans == XtGeometryAlmost)
			XtMakeGeometryRequest((Widget)w, &get, (XtWidgetGeometry *)0);
	    }
	}

	/*
	 * Skip the rest of this if our parent is just asking.
	 */
	if (query == ParentQueried) {
		if (response) {
			response->x            = w->core.x;
			response->y            = w->core.y;
			response->width        = request.width;
			response->height       = request.height;
			response->border_width = w->core.border_width;
		}
		return;
	}

	if (query == PleaseTry) {
		Position		x;
		Position		y;

		/*
		 * Place the abbreviated menu button in its correct
		 * place; the category label and the page label will
		 * be taken care of when we get an expose event.
		 */
		if (who_asking != child[0])
			XtMoveWidget (child[0], abbrev.x, abbrev.y);
		else if (response) {
			response->x            = abbrev.x;
			response->y            = abbrev.y;
			response->width        = child[0]->core.width;
			response->height       = child[0]->core.height;
			response->border_width = child[0]->core.border_width;
		}

		/*
		 * Try to center the lower control area above the footer,
		 * but if there's not enough space, maintain a NorthWest
		 * gravity.
		 */

		x = ((int)w->core.width - (int)lca.width) / 2;
		y = (int)w->core.height
		  - (int)foot.height
		  - (int)hThickness
		  - (int)lca.height;

		if (who_asking != child[1])
			XtMoveWidget (child[1], (x<0? 0:x), (y<0? 0:y));
		else if (response) {
			response->x            = (x<0? 0:x);
			response->y            = (y<0? 0:y);
			response->width        = child[1]->core.width;
			response->height       = child[1]->core.height;
			response->border_width = child[1]->core.border_width;
		}
	}

	/*
	 * This is the largest any child can be. Don't worry about
	 * negative values (!), as a check inside the following
	 * loop prevents us trying to set child sizes <= 0.
	 */
	largest_width	= request.width
			- 2 * vThickness;
	largest_height	= request.height
			- top.height
			- 2 * hThickness
			- lca.height
			- foot.height;

	/*
	 * Place all of the page-children, whether visible or not
	 * (but managed). This allows us to quickly switch pages
	 * by just unmapping the current page and mapping the new
	 * page.
	 */
	for (i = first; i < nchildren; i++)
	    if (
		XtIsManaged(child[i])
	     &&	(query != ChildQueried || who_asking == child[i])
	    ) {
		Dimension		bw = child[i]->core.border_width;

		XtWidgetGeometry	pre;
		XtWidgetGeometry	new;

		CategoryConstraintRec * constraint = CONSTRAINT(child[i]);


		/*
		 * Set up the maximum preferred geometry.
		 */
		new.x = vThickness;
		new.y = top.height + hThickness;
		if (largest_width > (int)(2 * bw))
			new.width = largest_width - (int)(2 * bw);
		else
			new.width = 1;
		if (largest_height > (int)(2 * bw))
			new.height = largest_height - (int)(2 * bw);
		else
			new.height = 1;
		new.border_width = bw;

		/*
		 * Before deriving the gravity-preferred geometry, see
		 * if the child has a better idea of its size. Use that
		 * size instead of the current (core) size, but ignore
		 * any other child-preferred geometry.
		 *
		 * Note: The ".query_child" resource is a hack used to
		 * overcome deficient widgets that (1) don't have a
		 * query_geometry procedure, but (2) don't really resize
		 * when told to. The Intrinsics treat these widgets as
		 * properly responsive to resize requests, and we'll get
		 * an XtGeometryYes if we try to query the child.
		 * But since they don't resize, we'll usually get the
		 * gravity wrong. Thus we let the client control when we
		 * should query.
		 */
		if (constraint->query_child && who_asking != child[i]) {
			new.request_mode = (CWWidth|CWHeight);
			switch (XtQueryGeometry(child[i], &new, &pre)) {

			case XtGeometryYes:
				pre.width  = new.width;
				pre.height = new.height;
				break;

			case XtGeometryAlmost:
#define CHECK(BIT,F) \
	if (!(pre.request_mode & BIT) || pre.F > new.F)			\
		pre.F = new.F;
				CHECK (CWWidth, width);
				CHECK (CWHeight, height);
#undef	CHECK
				break;

			case XtGeometryNo:
				pre.width  = child[i]->core.width;
				pre.height = child[i]->core.height;
				break;
			}

		} else {
			pre.width  = child[i]->core.width;
			pre.height = child[i]->core.height;
		}

		AdjustForGravity (&pre, &new, constraint->gravity);

		if (who_asking != child[i])
			XtConfigureWidget (
				child[i],
				new.x, new.y, new.width, new.height,
				bw
			);
		else if (response) {
			response->x            = new.x;
			response->y            = new.y;
			response->width        = new.width;
			response->height       = new.height;
			response->border_width = bw;
		}
	    }

	return;
} /* Layout */

/**
 ** AdjustForGravity()
 **/

static void
AdjustForGravity (
	XtWidgetGeometry *	preferred,
	XtWidgetGeometry *	response,
	int			gravity
)
{
	/*
	 * Start off with values for AllGravity:
	 */

				/*
				 * Can't start any further left or up
				 * than this:
				 */
	Position		x	= response->x;
	Position		y	= response->y;

				/*
				 * Can't be any bigger than this:
				 */
	Dimension		width	= response->width;
	Dimension		height	= response->height;


	/*
	 * CAUTION: To simplify this routine, we make the assumption
	 * that the "preferred" geometry is already constrained to the
	 * maximum size available.
	 */

#define CENTER_LEFTRIGHT \
	x = response->x + (response->width - preferred->width)/2U;	\
	width = preferred->width

#define CENTER_TOPBOTTOM \
	y = response->y + (response->height - preferred->height)/2U;	\
	height = preferred->height

#define ATTACH_TOP \
	/*EMPTY*/

#define ATTACH_LEFT \
	/*EMPTY*/

#define ATTACH_BOTTOM \
	y      = response->y + (height - preferred->height);		\
	height = preferred->height

#define ATTACH_RIGHT \
	x     = response->x + (width - preferred->width);		\
	width = preferred->width


	switch (gravity) {
	case Ol_AllGravity:
	default:
		break;
	case Ol_EastWestGravity:
		CENTER_TOPBOTTOM;
		break;
	case Ol_NorthSouthGravity:
		CENTER_LEFTRIGHT;
		break;
	case CenterGravity:
		CENTER_LEFTRIGHT;
		CENTER_TOPBOTTOM;
		break;
	case NorthGravity:
		ATTACH_TOP;
		CENTER_LEFTRIGHT;
		break;
	case SouthGravity:
		ATTACH_BOTTOM;
		CENTER_LEFTRIGHT;
		break;
	case EastGravity:
		ATTACH_RIGHT;
		CENTER_TOPBOTTOM;
		break;
	case WestGravity:
		ATTACH_LEFT;
		CENTER_TOPBOTTOM;
		break;
	case NorthWestGravity:
		ATTACH_TOP;
		ATTACH_LEFT;
		break;
	case NorthEastGravity:
		ATTACH_TOP;
		ATTACH_RIGHT;
		break;
	case SouthWestGravity:
		ATTACH_BOTTOM;
		ATTACH_LEFT;
		break;
	case SouthEastGravity:
		ATTACH_BOTTOM;
		ATTACH_RIGHT;
		break;
	}

	response->x      = x;
	response->y      = y;
	response->width  = width;
	response->height = height;

	return;
} /* AdjustForGravity */

/**
 ** SetWindowGravity
 **/

static void
SetWindowGravity (Widget w)
{
	CategoryConstraintRec * constraint	= CONSTRAINT(w);

	XSetWindowAttributes	attributes;


	if (constraint->window_gravity_set || !XtIsRealized(w))
		return;

	attributes.bit_gravity = constraint->gravity;
	XChangeWindowAttributes (
		XtDisplayOfObject(w),
		XtWindowOfObject(w),
		CWBitGravity,
		&attributes
	);

	constraint->window_gravity_set = True;
	return;
} /* SetWindowGravity */

/**
 ** NeedChangeBar()
 **/

static Boolean
NeedChangeBar(
	CategoryWidget		w,
	Cardinal		current_page
)
{
	Cardinal		first		= w->category.first_page_child;
	Cardinal		nchildren	= w->composite.num_children;
	Cardinal		i;
	Widget *		child		= w->composite.children;

	for (i = first; i < nchildren; i++)
		if (AVAILABLE(child[i]) &&
				CONSTRAINT(child[i])->changed  &&
				FindChild(w, child[i]) != current_page)
			return (True);

	return (False);
} /* NeedChangeBar */

/**
 ** CheckCancelDismiss()
 **/

static void
CheckCancelDismiss (CategoryWidget w)
{
	Cardinal		first		= w->category.first_page_child;
	Cardinal		nchildren	= w->composite.num_children;
	Cardinal		i;

	Widget *		child		= w->composite.children;
	Widget			shell		= XtParent(w);

	OlDefine		new_menu_type	= OL_MENU_LIMITED;
	OlDefine		menu_type;


	if (!XtIsShell(shell))
		return;

	XtVaGetValues (shell, XtNmenuType, (XtArgVal)&menu_type, (String)0);
	for (i = first; i < nchildren; i++)
		if (AVAILABLE(child[i]) && CONSTRAINT(child[i])->changed)
			new_menu_type = OL_MENU_CANCEL;
	if (new_menu_type != menu_type)
		XtVaSetValues (shell, XtNmenuType, (XtArgVal)new_menu_type, (String)0);

	return;
} /* CheckCancelDismiss */

/**
 ** DrawText()
 **/

static void
DrawText (
	CategoryWidget		w,
	Region			region,
	GC			gc,
	Geometry *		g,
	String			text
)
{
	Cardinal        len = _OlStrlen(text);

	if (!XtIsRealized((Widget)w) || !len)
		return;

	if (InRegion(region, g)) {
		XDrawImageString(
			XtDisplayOfObject((Widget)w),
			XtWindowOfObject((Widget)w),
			gc,
			g->x,
			g->y_baseline,
			text,
			len
			);
		XFlush(XtDisplayOfObject((Widget)w));
	}

	return;
} /* DrawText */

/**
 ** DrawFoot()
 **/

static void
DrawFoot (
	CategoryWidget		w,
	Region			region,
	OlgxAttrs *		attrs,
	Geometry *		g,
	OlgxTextLbl *		lbl
)
{
	if (!XtIsRealized((Widget)w))
		return;

	if (InRegion(region, g))
		olgx_draw_text(attrs->ginfo, XtWindow((Widget)w),
			(XtPointer)lbl->label, g->x, g->y, g->width,
			OLGX_NORMAL);

	return;
} /* DrawFoot */

/**
 ** DrawBorder()
 **/

static void
DrawBorder (
	Widget			w,
	Region			region,
	OlgxAttrs *		attrs,
	Position		x,
	Position		y,
	Dimension		width,
	Dimension		height
)
{
	Screen *		screen = XtScreenOfObject(w);

	Window			window = XtWindowOfObject(w);

	Dimension		hThickness;	/* horizontal line */
	Dimension		vThickness;	/* vertical line   */

	Geometry		g;


	hThickness = (Dimension)OlgxScreenPointToPixel(OL_VERTICAL,
					2, screen);
	vThickness = (Dimension)OlgxScreenPointToPixel(OL_HORIZONTAL,
					2, screen);

	/*
	 * Top edge in region?
	 */
	g.x      = x;
	g.y      = y;
	g.width  = width;
	g.height = hThickness;
	if (InRegion(region, &g))
		goto DrawIt;
	/*
	 * Right edge in region?
	 */
	g.x      = x + (width - vThickness);
	g.y      = y;
	g.width  = vThickness;
	g.height = height;
	if (InRegion(region, &g))
		goto DrawIt;
	/*
	 * Bottom edge in region?
	 */
	g.x      = x;
	g.y      = y + (height - hThickness);
	g.width  = width;
	g.height = hThickness;
	if (InRegion(region, &g))
		goto DrawIt;
	/*
	 * Left edge in region?
	 */
	g.x      = x;
	g.y      = y;
	g.width  = vThickness;
	g.height = height;
	if (!InRegion(region, &g))
		return;

/*
 * Draw Chiseled Box
*/
DrawIt:	olgx_draw_box(attrs->ginfo, window, x, y, width, height,
			OLGX_INVOKED, FALSE);
        olgx_draw_box(attrs->ginfo, window, x+1, y+1, width-2, height-2,
			OLGX_NORMAL, FALSE);
			
	return;
} /* DrawBorder */

/**
 ** ClearChangeBar()
 **/

static void
ClearChangeBar (CategoryWidget w)
{
	Geometry		change_bar;


	if (!XtIsRealized((Widget)w))
		return;

	/*
	 * Note: We don't check to see if we need a change bar,
	 * because we rely on the expose event that the server
	 * generates after this. On getting the expose event,
	 * we will see if we need to draw a change bar.
	 */
	ComputeChangeBarPosition(w, &change_bar);
	_OlDrawChangeBar(
		(Widget)w,
		w->category.cb,
		OL_NONE,
		True,
		change_bar.x,
		change_bar.y,
		(Region)0
	);

	return;
} /* ClearChangeBar */


/*
 * ClearCategoryLabel()
 */

static void
ClearCategoryLabel (CategoryWidget w)
{
	Geometry        category_label;
	DimensionPair   top;

	if (!XtIsRealized((Widget) w))
		return;

	ComputeCategoryLabelSize(w, &category_label);
	XClearArea(
		XtDisplayOfObject((Widget) w),
		XtWindowOfObject((Widget) w),
		category_label.x,
		category_label.y,
		category_label.width,
		category_label.height,
		True
		);

	return;
} /* ClearCategoryLabel */


/*
 * ClearPageLabel()
 */

static void
ClearPageLabel(CategoryWidget w)
{
	Geometry        page_label;

	if (!XtIsRealized((Widget)w))
		return;

	if (w->category.page_list) {
		ComputePageLabelSize(w, &page_label);
		XClearArea(
			XtDisplayOfObject((Widget)w),
			XtWindowOfObject((Widget)w),
			page_label.x,
			page_label.y,
			page_label.width,
			page_label.height,
			True
			);
	}

	return;
} /* ClearPageLabel */


/*
 * ClearFoot()
 */

static void
ClearFoot(
	CategoryWidget	w,
	OlDefine	which
)
{
	Display        *display = XtDisplayOfObject((Widget)w);
	Window          window = XtWindowOfObject((Widget)w);
	Geometry        g;
	OlgxTextLbl      lbl;
	Boolean         full_width;

	if (!XtIsRealized((Widget)w))
		return;

	ComputeFootSize(w, w->category.attrs, &g, &lbl, which);
	XClearArea(display, window, g.x, g.y, g.width, g.height, True);

	return;
} /* ClearFoot */


/*
 * ComputeTopSize()
 */

static void
ComputeTopSize (
	CategoryWidget		w,
	DimensionPair *		p_top,
	Geometry *		p_change_bar,
	Geometry *		p_category_label,
	PositionPair *		p_abbrev,
	Geometry *		p_page_label
)
{
	Screen *		screen		= XtScreenOfObject((Widget)w);
	Widget			child0		= w->composite.children[0];
	DimensionPair		top;
	PositionPair		abbrev;
	Geometry		change_bar;
	Geometry		category_label;
	Geometry		page_label;
	
	Dimension		top_margin	= HeightFromPoints(CATEGORY_TOP_MARGIN);
	
	Dimension		width;
	Dimension		ascent;
	Dimension		descent;
	Dimension		max_ascent	= 0;
	Dimension		max_descent	= 0;


	/*
	 * We don't have good guidelines for the vertical spacing of an
	 * abbreviated button and the labels to either side. Close
	 * inspection of the pictures in the OPEN LOOK specification
	 * suggest that the label to the right should be centered
	 * vertically in its ascent with the button, and the label
	 * to the left should have the same baseline as the label
	 * to the right. Not all pictures show this, but it looks OK.
	 */
	top.width = top.height = 0;

	/*
	 * Change bar:
	 */
	width = w->category.cb->width + w->category.cb->pad;
	top.width += WidthFromPoints(CATEGORY_CHANGE_BAR_SPACE) + width;
	change_bar.x      = top.width - width;
/*	change_bar.y	  = see BELOW */
	change_bar.width  = w->category.cb->width;
	change_bar.height = w->category.cb->height;

	/*
	 * CATEGORY label:
	 */
	ComputeTextSize (
		w->category.category_font,
		w->category.category_label,
		&width,
		&ascent,
		&descent
	);
	top.width += WidthFromPoints(CHANGE_BAR_PAD) + width;
	category_label.x      = top.width - width;
	category_label.y      = -ascent; /* see BELOW */
	category_label.width  = width;
	category_label.height = ascent + descent;

	if (ascent > max_ascent)
		max_ascent = ascent;
	if (descent > max_descent)
		max_descent = descent;

	/*
	 * Abbreviated menu button (Part I):
	 */
	top.width += WidthFromPoints(CATEGORY_SPACE1) + child0->core.width;
	abbrev.x = top.width - child0->core.width;

	/*
	 * Page label:
	 */
	if (w->category.page_list) {
		ComputeTextSize (
			w->category.font,
			w->category.page_list[w->category.current_page].label,
			&width,
			&ascent,
			&descent
		);

		top.width += WidthFromPoints(CATEGORY_SPACE2) + width;
		page_label.x      = top.width - width;
		page_label.y      = -ascent; /* see BELOW */
		page_label.width  = width;
		page_label.height = ascent + descent;

		if (ascent > max_ascent)
			max_ascent = ascent;
		if (descent > max_descent)
			max_descent = descent;
	}

	/*
	 * Abbreviated menu button (Part II):
	 *
	 * At this point, "ascent" is the ascent for the label to the
	 * right (page label), if we have one, otherwise, it is the
	 * ascent for the label to the left (category label). Center
	 * the abbreviated menu button vertically with this distance.
	 */
	ascent = child0->core.height - (child0->core.height - ascent) / 2U;
#if !defined(__ppc)
	descent = _OlMin(child0->core.height - ascent, 0U);
#else
	descent = _OlMin(child0->core.height - ascent, 0);
#endif /* __ppc */
	if (ascent > max_ascent)
		max_ascent = ascent;
	if (descent > max_descent)
		max_descent = descent;

	top.height = top_margin
		   + max_ascent
		   + max_descent
		   + HeightFromPoints(CATEGORY_BOTTOM_MARGIN);

	/*
	 * [If you're looking for ``BELOW'', here it is!]
	 *
	 * Note: ".y" already has the negative of the ascent for the
	 * corresponding font, so adding the ".y_baseline" value
	 * gives the coordinate of the top edge of the bounding box.
	 */
	category_label.y_baseline  = top_margin + max_ascent;
	category_label.y          += category_label.y_baseline;
	page_label.y_baseline      = top_margin + max_ascent;
	page_label.y              += page_label.y_baseline;
	change_bar.y               = category_label.y;

	/*
	 * At this point, "ascent" is the ``ascent'' for the abbreviated
	 * menu button.
	 */
	abbrev.y = top_margin + max_ascent - ascent;


	if (p_top)		*p_top            = top;
	if (p_change_bar)	*p_change_bar     = change_bar;
	if (p_category_label)	*p_category_label = category_label;
	if (p_abbrev)		*p_abbrev         = abbrev;
	if (p_page_label)	*p_page_label     = page_label;

	return;
} /* ComputeTopSize */

/**
 ** ComputeTextSize()
 **/

static void
ComputeTextSize (
	XFontStruct *		f,
	String			text,
	Dimension *		p_width,
	Dimension *		p_ascent,
	Dimension *		p_descent
)
{
	if (!text || !*text)
		return;

	*p_width	= XTextWidth(f, text, _OlStrlen(text));
	*p_ascent	= f->max_bounds.ascent;
	*p_descent	= f->max_bounds.descent;

	return;
} /* ComputeTextSize */

/**
 ** ComputeFootSize()
 **/

static void
ComputeFootSize (
	CategoryWidget		w,
	OlgxAttrs *		attr,
	Geometry *		g,
	OlgxTextLbl *		lbl,
	OlDefine		which
)
{
	Screen *		screen = XtScreenOfObject((Widget)w);

	Window			window = XtWindowOfObject((Widget)w);

	Boolean			full_width;

	Dimension		spec_height;


	switch (which) {
	case OL_LEFT:
		if (!(lbl->label = w->category.left_foot))
			return;
		lbl->justification = TL_LEFT_JUSTIFY;
		full_width = (w->category.right_foot == 0);
		break;

	case OL_RIGHT:
		if (!(lbl->label = w->category.right_foot))
			return;
		lbl->justification = TL_RIGHT_JUSTIFY;
		full_width = (w->category.left_foot == 0);
		break;

	case OL_BOTH:
		lbl->label = "xxx";
		lbl->justification = TL_RIGHT_JUSTIFY;
		full_width = True;
		break;
	}	
	lbl->mnemonic    = 0;
	lbl->qualifier	 = 0;
	lbl->meta	 = False;
	lbl->accelerator = 0;
	lbl->flags       = 0;
	lbl->font        = (OlStr)w->category.font;

	OlgxSizeTextLabel (screen, w->category.attrs, (XtPointer)lbl, 
	    &(g->width), &(g->height));

	/*
	 * Slight deviation here from OPEN LOOK specification:
	 * The spec. includes the window decorations in the 3/4 and 1/4
	 * proportions, we do not.
	 */
	if (lbl->justification == TL_LEFT_JUSTIFY) {
		g->x     = WidthFromPoints(CATEGORY_FOOTER_EDGE);
		g->width = (full_width? w->core.width : (3*w->core.width)/4U)
			 - g->x
			 - WidthFromPoints(CATEGORY_INTER_FOOTER);
	} else {
		if (full_width)
			g->x = WidthFromPoints(CATEGORY_FOOTER_EDGE);
		else
			g->x = (3*w->core.width)/4U
			     + WidthFromPoints(CATEGORY_INTER_FOOTER);
		g->width = w->core.width
			 - g->x
			 - WidthFromPoints(CATEGORY_FOOTER_EDGE);
	}

	spec_height = HeightFromPoints(CATEGORY_FOOTER_HEIGHT);
	if (g->height < spec_height)
		g->height = spec_height;
	g->y = w->core.height - g->height;
	if (g->y < 0) {
		/*
		 * If the CategoryWidget is newly created,
		 * and we're being called from the Layout routine,
		 * then the CategoryWidget doesn't have a size. On
		 * the other hand, if the widget has a size but it's
		 * just too small, we need to clip the footer height.
		 * We check for zero as a cheap way of seeing which
		 * case is at hand.
		 */
		if (w->core.height)
			g->height = w->core.height;
		g->y = 0;
	}

	return;
} /* ComputeFootSize */

/**
 ** InRegion()
 **/

static Boolean
InRegion (
	Region			region,
	Geometry *		g
)
{
	switch (XRectInRegion(region, g->x, g->y, g->width, g->height)) {
	case RectangleOut:
		return (False);
	case RectanglePart:
	case RectangleIn:
		return (True);
	}
} /* InRegion */

/**
 ** SetPageCB()
 **/

static void
SetPageCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
{
	CategoryWidget		cw	= (CategoryWidget)client_data;

	OlFlatCallData *	cd	= (OlFlatCallData *)call_data;


	if (!cw->category.page_list)
		return;
	SetPage (cw, cd->item_index, True);

	return;

} /* SetPageCB */

/**
 ** NextPageCB()
 **/

static void
NextPageCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
{
	CategoryWidget		cw	= (CategoryWidget)client_data;

	Cardinal		new_page;


	if (!cw->category.page_list)
		return;

	new_page = cw->category.current_page + 1;
	if (new_page >= cw->category.page_list_size)
		new_page = 0;
	SetPage (cw, new_page, True);

	return;
} /* NextPageCB */


/*
 * SetPage()
 */
 
static void
SetPage(
	CategoryWidget	w,
	Cardinal	new_page,
	Boolean		call_callbacks
)
{
	MenuItem       *list = w->category.page_list;
	MenuItem       *new;
	MenuItem       *old;

	Widget          new_w;
	Widget          old_w;


	if (!list || w->category.current_page == new_page)
		return;

#define CP(W) (W)->category.current_page

	old = &(list[CP(w)]);
	new = &(list[new_page]);

	old_w = (Widget) old->user_data;
	new_w = (Widget) new->user_data;

	if (call_callbacks && XtHasCallbacks((Widget)w, XtNnewPage) == 
			XtCallbackHasSome) {
		OlCategoryNewPage np;

		np.old = old_w;
		np.c_new = new_w;
		np.apply_all = NeedChangeBar(w, new_page);
		XtCallCallbacks((Widget)w, XtNnewPage, &np);
	}

	/*
	 * Erase the current page label; this will cause an exposure event
	 * later which will in turn cause us to draw the new page label.
	 * Likewise, erase the change bar. Unmap the current page and clear the
	 * corresponding menu item. Then map the new page, set the
	 * corresponding menu item, and tell the exclusives about the new menu
	 * settings.
	 */

	ClearPageLabel(w);
	ClearChangeBar(w);

	XtUnmapWidget(old_w);
	old->set = False;

	CP(w) = new_page;

	XtMapWidget(new_w);
	new->set = True;

	SETFLAG(w, _CATEGORY_INTERNAL_CHILD);
	XtVaSetValues(
		w->category.page_choice,
		XtNitemsTouched, (XtArgVal) True,
		(String) 0
		);
	CLRFLAG(w, _CATEGORY_INTERNAL_CHILD);

	return;
} /* SetPage */


/*
 * FindChild()
 */

static int
FindChild(
	CategoryWidget	w,
	Widget		child
)
{
	int             i;

	for (i = 0; i < w->category.page_list_size; i++)
		if (child == (Widget) (w->category.page_list[i].user_data))
			return (i);

	return (-1);
} /* FindChild */
