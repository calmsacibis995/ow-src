#pragma ident	"@(#)DropTarget.c	302.9	97/03/26 lib/libXol SMI"	/* OLIT	*/

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


#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/DropTargetP.h>
#include <Xol/Dynamic.h>
#include <Xol/OlCursors.h>
#include <Xol/Util.h> /* OLGX_TODO: Remove once OLGX supports DropTarget */


/*
 * Convenient macros:
 */
#define CORE_C(wc)	(((WidgetClass)(wc))->core_class)
#define DROPTARGET_C(wc)	(((DropTargetWidgetClass)wc)->droptarget_class)
#define SUPER_C(wc)	(CORE_C(wc).superclass)

#define CORE_P(w)	((DropTargetWidget)(w))->core
#define PRIMITIVE_P(w)	((DropTargetWidget)(w))->primitive
#define PIXMAP_P(w)	((DropTargetWidget)(w))->pixmap
#define DROPTARGET_P(w)	((DropTargetWidget)(w))->drop_target

#define OBSOLETE	NULL

#define	PREVIEW_HINTS(w) \
		(DROPTARGET_P(w).preview_hints \
		| (XtIsSensitive(w) ? \
		0 : OlDnDSitePreviewInsensitive))

/* Define DropTarget Dimensions per OPEN LOOK Spec 
 * Note: Modified slightly to support scaling
 */
#define DT_WDT_TO_SCALE_DIFF         3  /* Difference between width & scale  */
#define DT_HGT_TO_SCALE_DIFF         9  /* Difference between height & scale  */
#define LG_DT_HGT_TO_SCALE_DIFF      12 /* Difference between lg height & scale */
#define DT_SCALE_TO_STROKE_RATIO     4  /* Ratio of scale to bevel width  */


/* OLGX_TODO: For now, we are just supporting scales 12 & 19 for the
 * default "full" bitmap. Need to resolve method for other scales.
 * We don't want to include the raw data like this for each scale,
 * so we'll need to develop an intelligent algorithm for creating
 * the bitmap.
 */
#define droptarget_width 18
#define droptarget_height 24
static unsigned char droptarget_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xf0, 0x3f, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 0x00,
   0x00, 0x00, 0x00, 0xf0, 0x3f, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 0x00,
   0x00, 0x00, 0x00, 0xf0, 0x3f, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 0x00,
   0x00, 0x00, 0x00, 0xf0, 0x3f, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


#define lg_droptarget_width 25
#define lg_droptarget_height 35
static unsigned char lg_droptarget_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xc0, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x07, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xc0, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x07, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xc0, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x07, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xc0, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x07, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


/*
 * Local Routines:
 */
static void		Destroy (
				Widget
);

static void		Initialize (
				Widget,
				Widget,
				ArgList,
				Cardinal *
);
static void		Realize (
				Widget,
				XtValueMask *,
				XSetWindowAttributes *
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
static void		ButtonHandler (
				Widget,
				OlVirtualEvent
);
static void		SetBusy (
				Widget,
				Boolean
);
static Boolean		PreviewNotify (
				Widget,
				Window,
				Position,
				Position,
				int,
				Time,
				OlDnDDropSiteID,
				Boolean,
				XtPointer
);
static void		TriggerNotify (
				Widget,
				Window,
				Position,
				Position,
				Atom,
				Time,
				OlDnDDropSiteID,
				OlDnDTriggerOperation,
				Boolean,
				Boolean,
				XtPointer
);
static void		AnimateNotify (
				Widget,
				int,
				Time,
				Boolean,
				XtPointer
);

static void		GetDropTargetBitmap (
				Widget,
				OlgxAttrs *,
				Pixmap *
);

/* OLGX_TODO: These 2 routines should be removed once
 * OLGX supports droptarget rendering.
 */
static void		DrawVariableBorderBox (
				Widget,
				Screen *,
                                Drawable,
                                OlgxAttrs *,
                                Position,
                                Position,
                                Dimension,
                                Dimension,
                                unsigned,
				unsigned,
				unsigned
);

static void		DrawDropTargetBox (
				Widget,
				Screen *,
				Drawable,
				OlgxAttrs *,
				Position,
				Position,
				Dimension,
				Dimension,
				unsigned
);

static void		SizeDropTarget (
				Widget,
				OlgxAttrs *,
				Dimension *,
				Dimension *
);

static void		GetGCs (
				Widget
);


static void		_SetDefaultMoveCursor (
				Widget,
				int,
				XrmValue *
);
static void		_SetDefaultCopyCursor (
				Widget,
				int,
				XrmValue *
);
static void		PixmapComputeGeometry (
				Widget,
				Boolean
);

/*
 * Event Procs:
 */

static OlEventHandlerRec dt_event_procs[] =
{
        { ButtonPress,  ButtonHandler }
};

/*
 * Resources:
 */

static XtResource	resources[] = {
#define offset(F)       XtOffsetOf(DropTargetRec,F)

    {
	XtNrecomputeSize, XtCRecomputeSize, XtRBoolean,
	sizeof(Boolean), offset(pixmap.recompute_size),
	XtRString, (XtPointer)"False",
    },{
	XtNfull, XtCFull, XtRBoolean,
	sizeof(Boolean), offset(drop_target.full),
	XtRString, (XtPointer)"False",
    },{
	XtNbusyPixmap, XtCBusyPixmap, XtRPixmap,
	sizeof(Pixmap), offset(drop_target.busy_pixmap),
	XtRImmediate, (XtPointer)XtUnspecifiedPixmap,
    },{

/*
 * need to add a String to XtROlDnDSitePreviewHints converter
 */
	XtNdndPreviewHints, XtCDndPreviewHints,
	XtROlDnDSitePreviewHints,
	sizeof(OlDnDSitePreviewHints),
	offset(drop_target.preview_hints),
	XtRImmediate, (XtPointer)OlDnDSitePreviewNone,
    },{
	XtNdndPreviewCallback, XtCCallback, XtRCallback,
	sizeof(XtPointer), offset(drop_target.previewCallback),
	XtRCallback, (XtPointer)NULL,
    },{
	XtNdndTriggerCallback, XtCCallback, XtRCallback,
	sizeof(XtPointer), offset(drop_target.triggerCallback),
	XtRCallback, (XtPointer)NULL,
    },{
	XtNownSelectionCallback, XtCCallback, XtRCallback,
	sizeof(XtPointer), offset(drop_target.ownSelectionCallback),
	XtRCallback, (XtPointer)NULL,
    },{
	XtNselectionAtom, XtCSelectionAtom, XtRAtom,
	sizeof(Atom), offset(drop_target.selectionAtom),
	XtRImmediate, (XtPointer)NULL,
    },{
	XtNdndMoveCursor, XtCCursor, XtRCursor,
	sizeof(Cursor), offset(drop_target.moveCursor),
	XtRCallProc, (XtPointer)_SetDefaultMoveCursor,
    },{
	XtNdndCopyCursor, XtCCursor, XtRCursor,
	sizeof(Cursor), offset(drop_target.copyCursor),
	XtRCallProc, (XtPointer)_SetDefaultCopyCursor,
    },{
	XtNdndAcceptCursor, XtCCursor, XtRCursor,
	sizeof(Cursor), offset(drop_target.acceptCursor),
	XtRImmediate, (XtPointer)NULL,
    },{
	XtNdndRejectCursor, XtCCursor, XtRCursor,
	sizeof(Cursor), offset(drop_target.rejectCursor),
	XtRImmediate, (XtPointer)NULL,
    },{
	XtNdndAnimateCallback, XtCCallback, XtRCallback,
	sizeof(XtPointer), offset(drop_target.animateCallback),
	XtRCallback, (XtPointer)NULL,
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

DropTargetClassRec	dropTargetClassRec = {
	/*
	 * Core class:
	 */
	{
/* superclass           */ (WidgetClass)         &pixmapClassRec,
/* class_name           */                       "DropTarget",
/* widget_size          */                       sizeof(DropTargetRec),
/* class_initialize     */                       NULL,
/* class_part_init   (D)*/                       NULL,
/* class_inited         */                       False,
/* initialize        (D)*/                       Initialize,
/* initialize_hook   (D)*/ (XtArgsProc)          OBSOLETE,
/* realize           (I)*/                       Realize,
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
/* set_values_hook   (D)*/ (XtArgsFunc)          OBSOLETE,
/* set_values_almost (I)*/                       XtInheritSetValuesAlmost,
/* get_values_hook   (D)*/                       NULL,
/* accept_focus      (I)*/                       XtInheritAcceptFocus,
/* version              */                       XtVersion,
/* callback_private     */ (XtPointer)           0,
/* tm_table          (I)*/                       XtInheritTranslations,
/* query_geometry    (I)*/                       XtInheritQueryGeometry,
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
/* event_procs          */                       dt_event_procs,
/* num_event_procs      */                       XtNumber(dt_event_procs),
/* version              */                       OlVersion,
/* extension            */ (XtPointer)           0,
/* dyn_data             */ { 0 , 0 },
/* transparent_proc  (I)*/                       _OlDefaultTransparentProc,
	},
	/*
	 * Pixmap class:
	 */
	{
/* compute_pixmap_geometry_proc  (I)*/		XtInheritOlPixmapComputeGeometry,
	},
	/*
	 * DropTarget class:
	 */
	{
/* unused               */                       0
	}
};

WidgetClass dropTargetWidgetClass = (WidgetClass)&dropTargetClassRec;

	
/**
 ** Initialize()
 **/
static void
Initialize (Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	Boolean recalculate = False;

        /*
         * Get GCs for Rendering
         */
        DROPTARGET_P(new).pAttrs    = (OlgxAttrs *)NULL;
        DROPTARGET_P(new).bg_gc     = (GC)NULL;
        DROPTARGET_P(new).fg_gc     = (GC)NULL;
        DROPTARGET_P(new).hilite_gc = (GC)NULL;
 
        GetGCs(new);

	/*
	 * get the default pixmap
	 */
	if (PIXMAP_P(new).pixmap == XtUnspecifiedPixmap) {
		GetDropTargetBitmap(new,
			DROPTARGET_P(new).pAttrs,
			&(PIXMAP_P(new).pixmap));
		recalculate = True;
	}

	/*
	 * save the normal pixmap
	 */
	DROPTARGET_P(new).normal_pixmap =
		PIXMAP_P(new).pixmap;

	/*
	 * if not recompute_size (resize to size of pixmap)
	 * and no dimensions were specified use the defaults.
	 */
	if ((PIXMAP_P(new).recompute_size == False) &&
	    (CORE_P(request).width == 0 ||
	     CORE_P(request).height == 0)) {
		SizeDropTarget(new,
			DROPTARGET_P(new).pAttrs,
			&(CORE_P(new).width),
			&(CORE_P(new).height));

		recalculate = True;
	}

	/*
	 * Compute the widget's size and placement of
	 * the pixmap in the widget.
	 */
	if (recalculate)
		PixmapComputeGeometry(new, True);

	/*
	 * Initial the Drop Site ID
	 */
	DROPTARGET_P(new).dropsite_id =
		(OlDnDDropSiteID)NULL;

} /* Initialize */

/**
 ** Realize()
 **/
static void
Realize (Widget w, XtValueMask *value_mask, XSetWindowAttributes *attributes)
{
	XtRealizeProc	super_realize;
	OlDnDSiteRect	rect;

	/*
	 * Call super classes realize proc
	 */
	super_realize = dropTargetWidgetClass->core_class.superclass->core_class.realize;

	if (super_realize != (XtRealizeProc)NULL)
		(*super_realize)(w, value_mask, attributes);

	/*
	 * Register the dropsite
	 */
	rect.x = rect.y = 0;
	rect.width = (unsigned int)CORE_P(w).width;
	rect.height = (unsigned int)CORE_P(w).height;

	DROPTARGET_P(w).dropsite_id =
		OlDnDRegisterWidgetDropSite(w,
			PREVIEW_HINTS(w),
			&rect, 1,
			(OlDnDTMNotifyProc)TriggerNotify,
			(OlDnDPMNotifyProc)PreviewNotify,
			True,
			NULL);

} /* Realize */

/**
 ** Destroy 
 **/
static void
Destroy (Widget w)
{
    /* Destroy GCs */

    OlgxDestroyAttrs(w, DROPTARGET_P(w).pAttrs);
    XtReleaseGC(w, DROPTARGET_P(w).bg_gc);
    XFreeGC(XtDisplay(w), DROPTARGET_P(w).fg_gc);

    if (DROPTARGET_P(w).hilite_gc) /* no GC in 2D case */
        XFreeGC(XtDisplay(w), DROPTARGET_P(w).hilite_gc);

} /* Destroy */


/**
 ** Resize()
 **/
static void
Resize (Widget w)
{
	XtWidgetProc	super_resize;

	/*
	 * Call super classes resize proc
	 */
	super_resize = dropTargetWidgetClass->core_class.superclass->core_class.resize;

	if (super_resize != (XtWidgetProc)NULL)
		(*super_resize)(w);

	if (!XtIsRealized(w))
		return;

	/*
	 * Update the Dropsite's Geometry
	 */
	if (DROPTARGET_P(w).dropsite_id) {
		OlDnDSiteRect	rect;

		rect.x = rect.y = 0;
		rect.width = (unsigned int)CORE_P(w).width;
		rect.height = (unsigned int)CORE_P(w).height;

		OlDnDUpdateDropSiteGeometry(
			DROPTARGET_P(w).dropsite_id,
			&rect, 1);
	}
} /* Resize */

/**
 ** Redisplay()
 **/
static void
Redisplay (Widget w, XEvent *xevent, Region region)
{
	Screen		*screen = XtScreen(w);
	Window		window = XtWindowOfObject(w);
	XtExposeProc	super_expose;
	/*
	 * Draw the Drop Target's Box
	 */
	DrawDropTargetBox(w, screen, window,
		DROPTARGET_P(w).pAttrs, 0, 0,
		CORE_P(w).width, CORE_P(w).height,
		(XtIsSensitive(w) ? 0 : DT_INSENSITIVE));

	/*
	 * If the Drop Target is full and senstive,
	 * call the super class's expose proc to draw
	 * the pixmap.
	 */
	if (DROPTARGET_P(w).full && XtIsSensitive(w)) {
		super_expose = dropTargetWidgetClass->core_class.superclass->core_class.expose;

		if (super_expose != (XtExposeProc)NULL)
			(*super_expose)(w, xevent, region);

		/*
		 * If the Drop Target is busy display
		 * the busy pixmap.  If busy pixmap
		 * exists stipple the entire Drop Target.
		 */
		if (DROPTARGET_P(w).busy &&
		    DROPTARGET_P(w).busy_pixmap ==
		    XtUnspecifiedPixmap) {
			unsigned hStroke, vStroke;
			Display *dpy = XtDisplay(w);

			hStroke = (unsigned)OlScreenPointToPixel(
				OL_HORIZONTAL, 1, screen);
			vStroke = (unsigned)OlScreenPointToPixel(
				OL_VERTICAL, 1, screen);

		 	XSetStipple(dpy, DROPTARGET_P(w).fg_gc,
				DROPTARGET_P(w).pAttrs->pDev->inactiveStipple);
			XSetFillStyle(dpy, DROPTARGET_P(w).fg_gc,
				FillStippled);

			XFillRectangle(dpy, window, DROPTARGET_P(w).fg_gc,
				hStroke, vStroke, 
				CORE_P(w).width - (2*hStroke),
				CORE_P(w).height - (2*vStroke));

			/* Reset GC */
			XSetFillStyle(dpy, DROPTARGET_P(w).fg_gc, FillSolid);
		}
				
	}

} /* Redisplay */

/**
 ** SetValues()
 **/
static Boolean
SetValues (Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	Boolean		redisplay = False;
	Boolean		recalculate = False;

#define	DIFFERENT(F) (((DropTargetWidget)new)->F != ((DropTargetWidget)current)->F)

	if (DIFFERENT(primitive.foreground) || 
		DIFFERENT(core.background_pixel) ||
		    DIFFERENT(primitive.scale)) {
	    /* OLGX_TODO: This isn't complete - need to also get
	     * new size default bitmap if necessary.
	     */
	    if (!PIXMAP_P(new).recompute_size)
		SizeDropTarget(new, 
                        DROPTARGET_P(new).pAttrs,
                        &(CORE_P(new).width),
                        &(CORE_P(new).height));
	    GetGCs(new);
	    redisplay = True;
	}

	if (DIFFERENT(pixmap.pixmap))
		DROPTARGET_P(new).normal_pixmap =
			PIXMAP_P(new).pixmap;

	if (DIFFERENT(drop_target.full))
		redisplay = True;

	if (DROPTARGET_P(new).dropsite_id &&
	    (DIFFERENT(drop_target.preview_hints) ||
	     (XtIsSensitive(new) != XtIsSensitive(current)))) {
		OlDnDChangeDropSitePreviewHints(
			DROPTARGET_P(new).dropsite_id,
			PREVIEW_HINTS(new));

		if (XtIsSensitive(new) !=
		    XtIsSensitive(current))
			redisplay = True;
	}

#undef	DIFFERENT

        return(redisplay);

} /* SetValues */

/**
 ** DragIt()
 **/
static void
DragIt (Widget w, XEvent *event, OlDnDTriggerOperation operation)
{
	Cursor				cursor;
	Window				drop_window;
	Position			x, y;
	OlDnDDragDropInfo		rootinfo;
	XtPointer			closure;
	OlDropTargetCallbackStruct	cbdata;

	/*
	 * Continue only if the user
	 * starts dragging the pointer.
	 */
	if (OlDetermineMouseAction(w, event) != MOUSE_MOVE)
		return;

	/*
	 * Make drop target look busy
	 */
	SetBusy(w, True);

	/*
	 * Change the cursor
	 */
	if (operation == OlDnDTriggerMoveOp) {
		cursor = DROPTARGET_P(w).moveCursor;
	} else {	/* OlDnDTriggerCopyOp */
		cursor = DROPTARGET_P(w).copyCursor;
	}

	DROPTARGET_P(w).dragCursor = cursor;
	OlGrabDragPointer(w, cursor, None);

	if (OlDnDDragAndDrop(w, &drop_window, &x, &y,
		&rootinfo,
		(OlDnDPreviewAnimateCbP)AnimateNotify,
		(XtPointer)operation)) {

		cbdata.reason = OL_REASON_DND_OWNSELECTION;
		cbdata.widget = w;
		cbdata.time = rootinfo.drop_timestamp,

		XtCallCallbackList(w,
			DROPTARGET_P(w).ownSelectionCallback,
			&cbdata);

		/*
		 * If you got the selection
		 * deliver the trigger msg
		 */
		if (DROPTARGET_P(w).selectionAtom &&
		    (XGetSelectionOwner(XtDisplay(w),
		    DROPTARGET_P(w).selectionAtom) ==
		    XtWindow(w)))
			OlDnDDeliverTriggerMessage(w,
				rootinfo.root_window,
				rootinfo.root_x,
				rootinfo.root_y,
				DROPTARGET_P(w).selectionAtom,
				operation,
				rootinfo.drop_timestamp);
	}

	OlUngrabDragPointer(w);

	SetBusy(w, False);

} /* DragIt */

/**
 ** ButtonHandler()
 **/
static void
ButtonHandler (Widget w, OlVirtualEvent ve)
{
	ve->consumed = False;

	if (DROPTARGET_P(w).full && XtIsSensitive(w)) {
		switch(ve->virtual_name) {
		case OL_SELECT:
			DragIt(w, ve->xevent,
				OlDnDTriggerMoveOp);
			ve->consumed = True;
			break;
		case OL_DUPLICATE:
			DragIt(w, ve->xevent,
				OlDnDTriggerCopyOp);
			ve->consumed = True;
			break;
		default:
			break;
		}
	}

} /* ButtonHandler */


/**
 ** SetBusy()
 **/
static void
SetBusy (Widget w, Boolean state)
{
	/*
	 * if busy set the pixmap to the busy pixmap
	 * else set the pixmap to the normal pixmap.
	 *
	 * if no busy pixmap specified set the pixmap
	 * to the normal pixmap and it will be stippled.
	 */

	if (state) {
		DROPTARGET_P(w).busy = True;
		if (DROPTARGET_P(w).busy_pixmap !=
		    XtUnspecifiedPixmap)
			PIXMAP_P(w).pixmap =
				DROPTARGET_P(w).busy_pixmap;
	} else {
		DROPTARGET_P(w).busy = False;
		PIXMAP_P(w).pixmap =
			DROPTARGET_P(w).normal_pixmap;
	}

	Redisplay(w, (XEvent *)NULL, (Region)NULL);

} /* SetBusy */

/**
 ** PreviewNotify()
 **/
static Boolean
PreviewNotify (Widget widget, Window window, Position root_x,
	       Position root_y, int detail, Time time,
	       OlDnDDropSiteID dropsiteid, Boolean forwarded,
	       XtPointer closure)
{
	OlDropTargetCallbackStruct cbdata;

	cbdata.reason = OL_REASON_DND_PREVIEW;
	cbdata.widget = widget;
	cbdata.window = window;
	cbdata.root_x = root_x;
	cbdata.root_y = root_y;
	cbdata.eventcode = detail;
	cbdata.time = time;
	cbdata.dropsiteid = dropsiteid;
	cbdata.forwarded = forwarded;

	XtCallCallbackList(widget,
		DROPTARGET_P(widget).previewCallback,
		&cbdata);

	return(True);
} /* PreviewNotify */

/**
 ** TriggerNotify()
 **/
static void
TriggerNotify (Widget widget, Window window, Position root_x,
	       Position root_y, Atom selection, Time time,
	       OlDnDDropSiteID dropsiteid,
	       OlDnDTriggerOperation operation, Boolean send_done,
	       Boolean forwarded, XtPointer closure)
{
	OlDropTargetCallbackStruct cbdata;

	cbdata.reason = OL_REASON_DND_TRIGGER;
	cbdata.widget = widget;
	cbdata.window = window;
	cbdata.root_x = root_x;
	cbdata.root_y = root_y;
	cbdata.selection = selection;
	cbdata.time = time;
	cbdata.dropsiteid = dropsiteid;
	cbdata.operation = operation;
	cbdata.send_done = send_done;
	cbdata.forwarded = forwarded;

	XtCallCallbackList(widget,
		DROPTARGET_P(widget).triggerCallback,
		&cbdata);

} /* TriggerNotify */

/**
 ** AnimateNotify()
 **/
static void
AnimateNotify (Widget widget, int eventcode, Time time,
	       Boolean insensitivity, XtPointer closure)
{
	OlDropTargetCallbackStruct cbdata;
	Cursor cursor;

	cbdata.reason = OL_REASON_DND_ANIMATE;
	cbdata.widget = widget;
	cbdata.eventcode = eventcode;
	cbdata.time = time;
	cbdata.operation = (OlDnDTriggerOperation)closure;
	cbdata.sensitivity = (insensitivity ? False : True);

	/*
	 * If you enter a drop site which is sensitive
	 * change the cursor to an accept cursor.
	 *
	 * If you enter a drop site which is insensitive
	 * change the cursor to reject cursor.
	 */
	if (eventcode == EnterNotify) {
		cursor = (insensitivity ?
			DROPTARGET_P(widget).rejectCursor :
			DROPTARGET_P(widget).acceptCursor);
	} else if (eventcode == LeaveNotify)
		cursor = DROPTARGET_P(widget).dragCursor;

	if (cursor)
		OlChangeGrabbedDragCursor(widget, cursor);

	XtCallCallbackList(widget,
		DROPTARGET_P(widget).animateCallback,
		&cbdata);

} /* AnimateNotify */

/*
 * Get the Default Drop Target Bitmap
 */
static void
GetDropTargetBitmap(Widget w, OlgxAttrs *pInfo, Pixmap *pixmap)
{
    Screen		*scr = XtScreenOfObject(w);
    register _OlgxDevice      *pDev = pInfo->pDev;
    Pixmap		*pBitmap;
    unsigned int 	width, height;
    unsigned char	*defaultData;
 
    /*
     * Load the bitmap, if we haven't already.
     */
    if (!pDev->dropTarget) {
        pBitmap = &pDev->dropTarget;

	/* OLGX_TODO: Need to resolve handling other scales.
	 */
	if (PRIMITIVE_P(w).scale <= 16) {
            width = droptarget_width;
            height = droptarget_height;
            defaultData = droptarget_bits;

        } else { /* 19,20 or 24 */
	    width = lg_droptarget_width;
	    height = lg_droptarget_height;
	    defaultData = lg_droptarget_bits;
	}

        *pBitmap = XCreateBitmapFromData (DisplayOfScreen (scr),
                                          RootWindowOfScreen (scr),
                                          (char *) defaultData,
					  width, height);

    }
    *pixmap = pDev->dropTarget;

} /* GetDropTargetBitmap */

/* 
 * OLGX_TODO: Once OLGX supports a drop-target rendering routine,
 * this should be removed.
 */
static void
DrawVariableBorderBox(Widget w, Screen*  scr, Drawable win,
	OlgxAttrs*  pInfo, Position x, Position y, Dimension width,
	Dimension height, unsigned hStroke, unsigned vStroke,
	unsigned isPressed)
{
    Position	topX, topY, rightX, bottomY;
    Dimension	currentHeight, currentWidth;
    register	offset;
    register	numRects = 0, sizeRectsArray=0;

    if (OlgIs3d(scr)) {
	XRectangle *topRects, *bottomRects;

	/* Generate boxes for the top and left, being careful to mitre
	 * the top right and bottom left.
	 */
	topX = x;
	topY = y;
	rightX = x + width - 1;
	bottomY = y + height - 1;
	currentWidth = width - 1;
	currentHeight = height - 1;

	sizeRectsArray = (MIN(vStroke, hStroke)*2 + 1) * sizeof(XRectangle);
	topRects = (XRectangle *)XtMalloc(sizeRectsArray);
	bottomRects = (XRectangle *)XtMalloc(sizeRectsArray);

	for (offset=MIN(vStroke, hStroke); offset-->0; ) {
	    bottomRects [numRects].x = topX;
	    bottomRects [numRects].y = bottomY--;
	    bottomRects [numRects].width = currentWidth;
	    bottomRects [numRects].height = 1;
	    numRects++;

	    bottomRects [numRects].x = rightX--;
	    bottomRects [numRects].y = topY + 1;
	    bottomRects [numRects].width = 1;
	    bottomRects [numRects].height = currentHeight;
	    numRects--;

	    topRects [numRects].x = topX++;
	    topRects [numRects].y = topY;
	    topRects [numRects].width = 1;
	    topRects [numRects].height = currentHeight;
	    numRects++;

	    topRects [numRects].x = topX;
	    topRects [numRects].y = topY++;
	    topRects [numRects].width = currentWidth;
	    topRects [numRects].height = 1;
	    numRects++;

	    currentWidth -= 2;
	    currentHeight -= 2;
	}

	if (vStroke > hStroke) {
	    /* Must draw additional lines at top and bottom */
	    topRects [numRects].x = topX;
	    topRects [numRects].y = topY;
	    topRects [numRects].width = currentWidth;
	    topRects [numRects].height = vStroke - hStroke;

	    bottomRects [numRects].x = topX;
	    bottomRects [numRects].y = bottomY;
	    bottomRects [numRects].width = currentWidth;
	    bottomRects [numRects].height = vStroke - hStroke;
	    numRects++;

	} else if (vStroke < hStroke) {	/* but not equal */

	    /* Must draw additional lines at left and right */
	    topRects [numRects].x = topX;
	    topRects [numRects].y = topY;
	    topRects [numRects].width = hStroke - vStroke;
	    topRects [numRects].height = currentHeight;

	    bottomRects [numRects].x = rightX - (hStroke - vStroke) + 1;
	    bottomRects [numRects].y = topY;
	    bottomRects [numRects].width = hStroke - vStroke;
	    bottomRects [numRects].height = currentHeight;
	    numRects++;
	}

	/* Draw the rectangles generated */
	XFillRectangles (DisplayOfScreen (scr), win, isPressed ?
			     DROPTARGET_P(w).fg_gc : DROPTARGET_P(w).hilite_gc,
			 topRects, numRects);
	XFillRectangles (DisplayOfScreen (scr), win, isPressed ?
			     DROPTARGET_P(w).hilite_gc : DROPTARGET_P(w).fg_gc,
			 bottomRects, numRects);
	XtFree((char *)topRects);
	XtFree((char *)bottomRects);

    } else {
	XRectangle topRects[4];

	/* The 2-D case is easy--fill 4 rectangles */
	topRects [0].x = x;
	topRects [0].y = y;
	topRects [0].width = width;
	topRects [0].height = vStroke;

	topRects [1].x = x + width - hStroke;
	topRects [1].y = y + vStroke;
	topRects [1].width = hStroke;
	topRects [1].height = height - vStroke;

	topRects [2].x = x;
	topRects [2].y = y + height - vStroke;
	topRects [2].width = width - hStroke;
	topRects [2].height = vStroke;

	topRects [3].x = x;
	topRects [3].y = y + vStroke;
	topRects [3].width = hStroke;
	topRects [3].height = height - vStroke * 2;

	XFillRectangles (DisplayOfScreen (scr), win, DROPTARGET_P(w).fg_gc,
			 topRects, 4);
    }

} /* DrawVariableBorderBox */


/* 
 * OLGX_TODO: Need this until OLGX supports droptarget
 * rendering.
 */
void
DrawDropTargetBox(Widget w, Screen*  scr, Drawable win,
	OlgxAttrs*  pInfo, Position x, Position y, Dimension width,
	Dimension height, unsigned flags)
{
	Display			*dpy = XtDisplay(w);
	Dimension		ignoreWidth, ignoreHeight;
	register _OlgxDevice	*pDev = pInfo->pDev;
	int     		stroke = PRIMITIVE_P(w).scale /
					DT_SCALE_TO_STROKE_RATIO;

	SizeDropTarget(w,  pInfo,
		&ignoreWidth, &ignoreHeight);

	if (OlgIs3d(scr)) {
		/*
		 * Draw the box's background
		 */
		XFillRectangle (dpy, win, DROPTARGET_P(w).bg_gc,
			x, y, width, height);

		/*
		 * If insensitive, set the GCs
		 */
		if (flags & DT_INSENSITIVE) {
			XSetStipple (dpy, DROPTARGET_P(w).hilite_gc,
				pDev->inactiveStipple);
			XSetFillStyle (dpy, DROPTARGET_P(w).hilite_gc,
				FillStippled);
			XSetStipple (dpy, DROPTARGET_P(w).fg_gc,
				pDev->inactiveStipple);
			XSetFillStyle (dpy, DROPTARGET_P(w).fg_gc,
				FillStippled);
		}

		/*
		 * Draw a variable size box
		 */
		DrawVariableBorderBox (w, scr, win, pInfo,
			x, y, width, height,
			OlScreenPointToPixel(OL_HORIZONTAL,
				stroke, scr),
			OlScreenPointToPixel(OL_VERTICAL,
				stroke, scr),
			True);

		/*
		 * If insensitive, restore the GCs
		 */
		if (flags & DT_INSENSITIVE) {
			XSetFillStyle (dpy, DROPTARGET_P(w).hilite_gc,
				FillSolid);
			XSetFillStyle (dpy, DROPTARGET_P(w).fg_gc,
				FillSolid);
		}
	} else {
		/*
		 * Draw the box's background
		 */
		XFillRectangle (dpy, win, DROPTARGET_P(w).bg_gc,
			x, y, width, height);

		/*
		 * If insensitive, set the GCs
		 */
		if (flags & DT_INSENSITIVE) {
			XSetStipple (dpy, DROPTARGET_P(w).fg_gc,
				pDev->inactiveStipple);
			XSetFillStyle (dpy, DROPTARGET_P(w).fg_gc,
				FillStippled);
		}

		/*
		 * Draw a variable size box
		 */
		DrawVariableBorderBox (w, scr, win, pInfo,
			x, y, width, height,
			OlScreenPointToPixel(OL_HORIZONTAL,
				stroke, scr),
			OlScreenPointToPixel(OL_VERTICAL,
				stroke, scr),
			True);

		/*
		 * If insensitive, restore the GCs
		 */
		if (flags & DT_INSENSITIVE) {
			XSetFillStyle (dpy, DROPTARGET_P(w).fg_gc,
				FillSolid);
		}
	}
} /* DrawDropTargetBox */

/**
 ** SizeDropTarget(): Determine dimensions of outline
 **/
void
SizeDropTarget(Widget w, OlgxAttrs *pInfo, 
	       Dimension *pWidth, Dimension *pHeight)
{
    Screen *scr = XtScreenOfObject(w);
    int scale = PRIMITIVE_P(w).scale;

    *pWidth = (Dimension)OlScreenPointToPixel(
                OL_HORIZONTAL, scale + DT_WDT_TO_SCALE_DIFF,
                scr);

    *pHeight = (Dimension)OlScreenPointToPixel(
               	OL_VERTICAL,  scale + 
		    (scale <= 14 ?  
			DT_HGT_TO_SCALE_DIFF :
		    	    LG_DT_HGT_TO_SCALE_DIFF),
               	scr);

} /* SizeDropTarget */

/**
 ** GetGCs: Get Drawing GCs and pAttr pointer for rendering.
 ** OLGX_TODO: once OLGX supports droptarget rendering, we
 ** need to remove the regular GC stuff.
 */
static void
GetGCs(Widget w)
{
    XGCValues 	values;
    Drawable	drawable;
    Screen	*scr = XtScreenOfObject(w);
    Display	*dpy = XtDisplay(w);

    /* If we've created them already, destroy them */

    if (DROPTARGET_P(w).pAttrs != (OlgxAttrs *)NULL)
	OlgxDestroyAttrs(w, DROPTARGET_P(w).pAttrs);

    if (DROPTARGET_P(w).bg_gc)
	XtReleaseGC((Widget)w, DROPTARGET_P(w).bg_gc);
    if (DROPTARGET_P(w).fg_gc)
	XFreeGC(dpy, DROPTARGET_P(w).fg_gc);
    if (DROPTARGET_P(w).hilite_gc)
	XFreeGC(dpy, DROPTARGET_P(w).hilite_gc);

    DROPTARGET_P(w).pAttrs = OlgxCreateAttrs(w,
		PRIMITIVE_P(w).foreground,
                (OlgxBG *)&(CORE_P(w).background_pixel),
		False, PRIMITIVE_P(w).scale,
		(OlStrRep)0, (OlFont)NULL);

    /* Get Drawable to Create GCs on */
    if (XtIsRealized((Widget)w))
	drawable = (Drawable)XtWindow((Widget)w);
    else
	drawable = XCreatePixmap(dpy, RootWindowOfScreen(scr),
                           1, 1, OlDepthOfObject((Widget)w));

    if (OlgIs3d(scr)) {
	unsigned long colorBG2, colorBG3, colorHilite;
	Graphics_info *pInfo = DROPTARGET_P(w).pAttrs->ginfo;

	colorBG2    = olgx_get_single_color(pInfo, OLGX_BG2);
	colorBG3    = olgx_get_single_color(pInfo, OLGX_BG3);
	colorHilite = olgx_get_single_color(pInfo, OLGX_WHITE);

	/* Create ReadOnly shared GC for background */
	values.foreground = colorBG2;
	DROPTARGET_P(w).bg_gc = XtGetGC((Widget)w, GCForeground, &values);

	/* Create Writable GCs (so we can set an inactive stipple) */
	values.foreground = colorBG3;
	DROPTARGET_P(w).fg_gc = XCreateGC(dpy, drawable, 
					GCForeground, &values);

	values.foreground = colorHilite;
	DROPTARGET_P(w).hilite_gc = XCreateGC(dpy, drawable, 
					GCForeground, &values);
    } else { /* 2D only needs 2 GCs */

	/* Create ReadOnly shared GC for background */
	values.foreground = CORE_P(w).background_pixel;
	DROPTARGET_P(w).bg_gc = XtGetGC((Widget)w, GCForeground, &values);

	/* Create Writable GC (so we can set inactive stipple) */
	values.foreground = PRIMITIVE_P(w).foreground;
        DROPTARGET_P(w).fg_gc = XCreateGC(dpy, drawable,  
                                        GCForeground, &values);
	DROPTARGET_P(w).hilite_gc = (GC)NULL;
    }

} /* GetGCs */


/**
 ** _SetDefaultMoveCursor()
 **/
static void
_SetDefaultMoveCursor (Widget widget, int closure, XrmValue *value)
{
	static	Cursor	cursor;

	cursor = OlGetMoveCursor(widget);

	value->addr = (caddr_t) &cursor;

} /* _SetDefaultMoveCursor */

/**
 ** _SetDefaultCopyCursor()
 **/
static void
_SetDefaultCopyCursor (Widget widget, int closure, XrmValue *value)
{
	static	Cursor	cursor;

	cursor = OlGetDuplicateCursor(widget);

	value->addr = (caddr_t) &cursor;

} /* _SetDefaultCopyCursor */

/**
 ** PixmapComputeGeometry()
 **/
static void
PixmapComputeGeometry (Widget w, Boolean resizable)
{
	XtCheckSubclass(w, pixmapWidgetClass, NULL);
 
	(*((PixmapWidgetClass) XtClass(w))->pixmap_class.compute_geometry)(w, resizable);
}
