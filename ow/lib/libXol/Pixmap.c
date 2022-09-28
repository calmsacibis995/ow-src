#pragma ident	"@(#)Pixmap.c	302.5	97/03/26 lib/libXol SMI"	/* OLIT	*/

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


#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/Error.h>
#include <Xol/OpenLookP.h>
#include <Xol/PixmapP.h>
#include <Xol/RootShellP.h>


/*
 * Convenient macros:
 */

#define CORE_C(wc)	(((WidgetClass)(wc))->core_class)
#define PIXMAP_C(wc)	(((PixmapWidgetClass)wc)->pixmap_class)
#define SUPER_C(wc)	(CORE_C(wc).superclass)

#define CORE_P(w)	((PixmapWidget)(w))->core
#define PRIMITIVE_P(w)	((PixmapWidget)(w))->primitive
#define PIXMAP_P(w)	((PixmapWidget)(w))->pixmap


/*
 * Local routines:
 */

static void		ClassPartInitialize (
				WidgetClass
);
static void		Initialize (
				Widget,
				Widget,
				ArgList,
				Cardinal *
);
static void		Destroy (
				Widget
);
static void		Resize (
				Widget
);
static void		Redisplay (
				Widget,
				XEvent *,
				Region
);
static Boolean		SetValues (
				Widget,
				Widget,
				Widget,
				ArgList,
				Cardinal *
);
static XtGeometryResult	QueryGeometry (
				Widget,
				XtWidgetGeometry *,
				XtWidgetGeometry *
);
static void		GetPixmapGC (
				Widget
);
static 	void		ComputePixmapGeometry (
				Widget,
				Boolean
);
static	void		PixmapComputeGeometry (
				Widget,
				Boolean
);

/*
 * Resources:
 */

static XtResource	resources[] = {
#define offset(F)       XtOffsetOf(PixmapRec,F)

    {
	XtNpixmap, XtCPixmap, XtRPixmap,
	sizeof(Pixmap), offset(pixmap.pixmap),
	XtRImmediate, (XtPointer)XtUnspecifiedPixmap,
    },{
	XtNrecomputeSize, XtCRecomputeSize, XtRBoolean,
	sizeof(Boolean), offset(pixmap.recompute_size),
	XtRString, (XtPointer)"True",
    },{
	XtNtraversalOn, XtCTraversalOn, XtRBoolean,
	sizeof(Boolean), offset(primitive.traversal_on),
	XtRImmediate, (XtPointer)False
    },
#undef	offset
};

/*
 * Class record structure:
 *
 *	(I)	XtInherit'd field
 *	(D)	Chained downward (super-class to sub-class)
 *	(U)	Chained upward (sub-class to super-class)
 */

PixmapClassRec	pixmapClassRec = {
	/*
	 * Core class:
	 */
	{
/* superclass           */ (WidgetClass)         &primitiveClassRec,
/* class_name           */                       "Pixmap",
/* widget_size          */                       sizeof(PixmapRec),
/* class_initialize     */                       NULL,
/* class_part_init   (D)*/                       ClassPartInitialize,
/* class_inited         */                       False,
/* initialize        (D)*/                       Initialize,
/* initialize_hook   (D)*/ (XtArgsProc)          NULL,
/* realize           (I)*/                       XtInheritRealize,
/* actions           (U)*/                       NULL,
/* num_actions          */                       0,
/* resources         (D)*/                       resources,
/* num_resources        */                       XtNumber(resources),
/* xrm_class            */                       NULLQUARK,
/* compress_motion      */                       True,
/* compress_exposure    */                       XtExposeCompressSeries,
/* compress_enterleave  */                       True,
/* visible_interest     */                       False,
/* destroy           (U)*/                       Destroy,
/* resize            (I)*/                       Resize,
/* expose            (I)*/                       Redisplay,
/* set_values        (D)*/                       SetValues,
/* set_values_hook   (D)*/ (XtArgsFunc)          NULL,
/* set_values_almost (I)*/                       XtInheritSetValuesAlmost,
/* get_values_hook   (D)*/                       NULL,
/* accept_focus      (I)*/                       XtInheritAcceptFocus,
/* version              */                       XtVersion,
/* callback_private     */ (XtPointer)           0,
/* tm_table          (I)*/                       XtInheritTranslations,
/* query_geometry    (I)*/                       QueryGeometry,
/* display_acceler   (I)*/ (XtStringProc)        _XtInherit,
/* extension            */ (XtPointer)           0
	},
	/*
	 * Primitive class:
	 */
	{
/* reserved             */ (XtPointer)           NULL,
/* highlight_handler (I)*/                       NULL,
/* traversal_handler (I)*/                       NULL,
/* register_focus       */                       NULL,
/* activate          (I)*/                       NULL,
/* event_procs          */                       NULL,
/* num_event_procs      */                       0,
/* version              */                       OlVersion,
/* extension            */ (XtPointer)           0,
/* dyn_data             */ { 0 , 0 },
/* transparent_proc  (I)*/			_OlDefaultTransparentProc,
	},
	/*
	 * Pixmap class:
	 */
	{
/* compute_geometry_proc  (I)*/			ComputePixmapGeometry,
	}
};

WidgetClass	pixmapWidgetClass = (WidgetClass)&pixmapClassRec;

/**
 ** ClassPartInitialize()
 **/
static void
ClassPartInitialize (WidgetClass widget_class)
{
	PixmapWidgetClass wc = (PixmapWidgetClass)
					widget_class;
	PixmapWidgetClass super = (PixmapWidgetClass)
				wc->core_class.superclass;

	if (wc->pixmap_class.compute_geometry ==
		XtInheritOlPixmapComputeGeometry) {
		wc->pixmap_class.compute_geometry =
			super->pixmap_class.compute_geometry;
	}

} /* ClassPartInitialize */

/**
 ** Initialize()
 **/
static void
Initialize (Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	if (!CORE_P(new).width)
		CORE_P(new).width = 1;

	if (!CORE_P(new).height)
		CORE_P(new).height = 1;

        PIXMAP_P(new).fg_gc = (GC)NULL;

	PixmapComputeGeometry(new, True);

} /* Initialize */

/**
 ** Destroy()
 **/
static void
Destroy (Widget w)
{
    if (PIXMAP_P(w).fg_gc)
	XtReleaseGC(w, PIXMAP_P(w).fg_gc);

} /* Destroy */

/**
 ** Resize()
 **/
static void
Resize (Widget w)
{
    PixmapComputeGeometry(w, False);

} /* Resize */

/**
 ** Redisplay()
 **/
static void
Redisplay (Widget w, XEvent *xevent, Region region)
{
	Display	*display	= XtDisplayOfObject(w);
	Window	window		= XtWindowOfObject(w);
	PixmapWidget pw		= (PixmapWidget)w;

	/*
	 * If valid pixmap ... draw it
	 */
	if (pw->pixmap.pixmap != XtUnspecifiedPixmap) {
		if (pw->pixmap.is_bitmap) {
			XFillRectangle (
				display,
				window,
				pw->pixmap.fg_gc,
				pw->pixmap.pixmap_geometry.x,
				pw->pixmap.pixmap_geometry.y,
				pw->pixmap.pixmap_geometry.width,
				pw->pixmap.pixmap_geometry.height
			);

		} else {
			XCopyArea (
				display,
				pw->pixmap.pixmap,
				window,
				pw->pixmap.fg_gc,
				0,
				0,
				pw->pixmap.pixmap_geometry.width,
				pw->pixmap.pixmap_geometry.height,
				pw->pixmap.pixmap_geometry.x,
				pw->pixmap.pixmap_geometry.y
			);
		}
	}

} /* Redisplay */

/**
 ** SetValues()
 **/
static Boolean
SetValues (Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	Boolean	redisplay	= False;
	PixmapWidget pw = (PixmapWidget)new;

#define DIFFERENT(F) (((PixmapWidget)new)->F != ((PixmapWidget)current)->F)

	if (DIFFERENT(pixmap.pixmap) ||
	    DIFFERENT(pixmap.recompute_size) ||
	    DIFFERENT(core.width) ||
	    DIFFERENT(core.height)) {
		PixmapComputeGeometry(new, True);
		redisplay = True;
	}

	if ((DIFFERENT(pixmap.pixmap) && PIXMAP_P(new).is_bitmap) ||
	    DIFFERENT(primitive.foreground)) {
		GetPixmapGC(new);
		redisplay = True;
	}

#undef	DIFFERENT
	return (redisplay);
} /* SetValues */

/**
 ** QueryGeometry()
 **/
static XtGeometryResult
QueryGeometry (Widget w, XtWidgetGeometry *request, XtWidgetGeometry *preferred)
{
	/*
	 * We don't care about our position or border,
	 * but we will suggest a best size.
	 *
	 * The current size which is the
	 * size returned from PixmapComputeGeometry()
	 *
	 * Set the optimum size to the size of the pixmap.
	 */

	preferred->width  = CORE_P(w).width;
	preferred->height  = CORE_P(w).height;
	preferred->request_mode = CWWidth|CWHeight;

#define CHECK(BIT, F) ((request->request_mode & BIT) && \
			preferred->F == request->F)

	if (CHECK(CWWidth, width) && CHECK(CWHeight, height))
		return(XtGeometryYes);

#undef	CHECK

	return(XtGeometryNo);
} /* QueryGeometry */

/**
 ** PixmapComputeGeometry()
 **/
static void
PixmapComputeGeometry (Widget w, Boolean resizable)
{
	XtCheckSubclass(w, pixmapWidgetClass, NULL);

	(*((PixmapWidgetClass) XtClass(w))->pixmap_class.compute_geometry)(w, resizable);
}

/**
 ** ComputePixmapGeometry()
 **/
static void
ComputePixmapGeometry (Widget w, Boolean resizable)
{
	Dimension	original_width	= CORE_P(w).width;
	Dimension	original_height	= CORE_P(w).height;
	PixmapWidget	pw = (PixmapWidget)w;

	/*
	 * Be vewy vewy careful about types here, wascally wabbit!
	 */
	Window		ignore_root;
	int		ignore_x, ignore_y;
	unsigned int	width;
	unsigned int	height;
	unsigned int	ignore_border_width;
	unsigned int	depth;

	Display *	display	= XtDisplayOfObject(w);
	Screen *	screen	= XtScreenOfObject(w);

	/*
	 * If it's not a legal pixmap set the pixmap size to 1 
	 */
	if (PIXMAP_P(w).pixmap == XtUnspecifiedPixmap)
		width = height = 1U;
	else {
		/*
		 * Get the depth of the pixmap
		 */
		XGetGeometry (
			display,
			PIXMAP_P(w).pixmap,
			&ignore_root,
			&ignore_x, &ignore_y,
			&width, &height,
			&ignore_border_width,
			&depth
		);

		if (depth == 1)
			PIXMAP_P(w).is_bitmap = True;
		else if (depth == CORE_P(w).depth)
			PIXMAP_P(w).is_bitmap = False;
		else {
			OlVaDisplayWarningMsg (
				XtDisplayOfObject(w),
				"invalidPixmap",
				"illegalDepth",
				OleCOlToolkitWarning,
"Widget %s: Depth of the pixmap (%d) doesn't match widget's depth (%d)",
				XtName(w),
				depth,
				CORE_P(w).depth
			);
			PIXMAP_P(w).pixmap = XtUnspecifiedPixmap;
			width = height = 1U;
		}
	}

	/*
	 * If recompute flag is on resize widget to size of pixmap.
	 *
	 * If recompute flag is off and the size of the widget is
	 * larger than the pixmap center the pixmap within the widget,
	 */

#define COMPUTE(POS,DIM) \
	if (PIXMAP_P(w).recompute_size && resizable) {			\
		CORE_P(w).DIM = (Dimension)DIM;				\
		PIXMAP_P(w).pixmap_geometry.POS = 0;			\
	} else {							\
		if ((Dimension)DIM > CORE_P(w).DIM)			\
			DIM = (unsigned int)CORE_P(w).DIM;		\
		PIXMAP_P(w).pixmap_geometry.POS = (CORE_P(w).DIM - DIM) / 2U;	\
	}								\
	PIXMAP_P(w).pixmap_geometry.DIM = (Dimension)DIM

	COMPUTE (x, width);
	COMPUTE (y, height);
#undef	COMPUTE
         if (!PIXMAP_P(w).fg_gc)
            GetPixmapGC(w);

} /* ComputePixmapGeometry */

/**
 ** GetPixmapGC()
 **/
static void
GetPixmapGC (Widget w)
{
    PixmapWidget pw = (PixmapWidget)w; 
    /*
     * Destroy existing GC
     */
    if (pw->pixmap.fg_gc)
	XtReleaseGC(w, pw->pixmap.fg_gc);

    /*
     * If valid Pixmap exists, Alloc new GC
     */
    if (pw->pixmap.pixmap != XtUnspecifiedPixmap) {
	XGCValues values;
	unsigned long mask;

    	values.foreground = pw->primitive.foreground;
	mask = GCForeground;

	if (pw->pixmap.is_bitmap) {
	    values.fill_style = FillStippled;
	    values.stipple = pw->pixmap.pixmap;
	    values.ts_x_origin = pw->pixmap.pixmap_geometry.x;
	    values.ts_y_origin = pw->pixmap.pixmap_geometry.y;

	    mask |= (GCFillStyle | GCStipple | 
			GCTileStipXOrigin | GCTileStipYOrigin);
	}
    	pw->pixmap.fg_gc = XtGetGC(w, mask, &values);

    } else
	pw->pixmap.fg_gc = (GC)NULL;

} /* GetPixmapGC */
