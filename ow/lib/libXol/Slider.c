#pragma ident   "@(#)Slider.c	302.21    97/03/26 lib/libXol SMI"     /* slider:src/Slider.c 1.41     */

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

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <locale.h>
#include <libintl.h>
#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>

#include <Xol/GaugeP.h>
#include <Xol/OlCursors.h>
#include <Xol/OlI18nP.h>
#include <Xol/OlStrMthdsI.h>
#include <Xol/OpenLookP.h>
#include <Xol/SliderP.h>
#include <Xol/Util.h>


/*
 *************************************************************************
 *
 * Forward Procedure definitions listed	by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations***************************
 */

/* private procedures		*/
static int calc_stoppos(SliderWidget sw, int nearval);
static int calc_nextstoppos(SliderWidget sw, int val, int dir);
static void highlight(SliderWidget sw, int invert);
static void MoveSlider(SliderWidget sw, int new_location, Boolean callback, Boolean more);
static void TimerEvent(XtPointer client_data, XtIntervalId *id);
static void SLError(SliderWidget sw, int idx, int value);
static void calc_dimension(SliderWidget sw, Dimension *width, Dimension *height);
static void make_ticklist(SliderWidget sw);
static Dimension calc_leftMargin(SliderWidget sw);
static Dimension calc_rightMargin(SliderWidget sw);
static int nearest_tickval(SliderWidget sw, int val);
static int inAnchor(SliderWidget sw, int x, int y, int anchor);
static void SLButtonHandler(Widget w, OlVirtualEvent ve);
static Boolean SLActivateWidget (Widget, OlVirtualName, XtPointer);
static Boolean ScrollKeyPress (Widget, unsigned char);
static void UpdateSuperCaret(SliderWidget sw);

/* Routines used to Render Slider under OLGX */
static void SizeSlider(SliderWidget sw, OlgxAttrs* pInfo, 
			Dimension *pWidth, Dimension *pHeight);
static void SizeSliderAnchor(SliderWidget sw, OlgxAttrs* pInfo, 
			Dimension *pWidth, Dimension *pHeight);
static void DrawSlider(SliderWidget sw, OlgxAttrs *pInfo);
static void UpdateSlider(SliderWidget sw, OlgxAttrs *pInfo, 
			unsigned int flags);
static void DrawTicks(SliderWidget sw, Position to, Position from);
static void LocateElevator(SliderWidget sw, Position cablePos, 
			Dimension cableLen);

/* class procedures		*/
static void		 ClassInitialize(void);
static void		 GetValuesHook(Widget w, ArgList args, Cardinal *num_args);
static void		 Destroy(Widget w);
static void		 Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);
static XtGeometryResult  QueryGeom(Widget w, XtWidgetGeometry *intended, XtWidgetGeometry *reply);
static void		 Realize(Widget w, Mask *valueMask, XSetWindowAttributes *attributes);
static void		 Redisplay(Widget w, XEvent *event, Region region);
static void		 Resize(Widget w);
static Boolean		 SetValues (Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);
void 			SliderQuerySCLocnProc(const Widget		target,
			const Widget		supercaret,
			const Dimension		sc_width,
			const Dimension		sc_height,
			unsigned int    *const	scale,	
			SuperCaretShape *const 	shape,
			Position        *const	x_center_return,
			Position        *const	y_center_return);
static void		 HighlightHandler (Widget, OlDefine);

/* action procedures		*/
static void SelectDown(Widget w, XEvent *event, String *params, Cardinal *num_params);
static void SelectUp(Widget w, XEvent *event, String *params, Cardinal *num_params);
static void SLKeyHandler (Widget, OlVirtualEvent);


/*
 *************************************************************************
 *
 * Define global/static	variables and #defines,	and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define	SLD		sw->slider
#define	HORIZ(W)	((W)->slider.orientation == OL_HORIZONTAL)
#define	VERT(W)		((W)->slider.orientation == OL_VERTICAL)
#define IS_SLIDER(W)	(XtIsSubclass((Widget)W, sliderWidgetClass))
#define IS_GAUGE(W)	(XtIsSubclass((Widget)W, gaugeWidgetClass))
#define TICKS_ON(W)	((W)->slider.tickUnit != OL_NONE)

/* Define rendering States for OLGX */
#define SL_POSITION     1
#define SL_DRAG         2
#define SL_BEGIN_ANCHOR 4
#define SL_END_ANCHOR   8

/* Define Slider Rendering RATIOS as per OPEN LOOK Functional Spec. */
#define OL_SCALE_TO_PAD_RATIO		3.5 /* Table B-38: row g (p.458) */
#define OL_SCALE_TO_TICK_LEN_RATIO	2   /* Table B-40: row a (p.460) */
#define OL_SCALE_TO_TICK_WID_RATIO	5   /* Table B-40: row b (p.460) */


#define	INRANGE(V,MIN,MAX) (((V) <= (MIN)) ? (MIN) : ((V) >= (MAX) ? (MAX) : (V)))
#define	PIX_TO_VAL(M1,M2,Q1)	(((unsigned long)(M2)/(Q1)*(M1))+(((M2)%(Q1)*(M1)+(Q1-1))/(Q1)))
#define ISKBDOP		(SLD.opcode & KBD_OP)

/* Scroll Resources  Defaults */
#define DFLT_SLIDERVALUE	0
#define	DFLT_MIN 		0
#define	DFLT_MAX 		100
#define	DFLT_REPEATRATE	        100
#define	DFLT_INITIALDELAY       500
#define	DFLT_GRANULARITY        1
#define	DFLT_SCALE	        OL_DEFAULT_POINT_SIZE


/*
 *************************************************************************
 * Define Translations and Actions
 ***********************widget*translations*actions**********************
 */

/* The translation table is inherited from its superclass */

static OlEventHandlerRec sl_event_procs[] = {
	{ KeyPress,	SLKeyHandler	},
	{ ButtonPress,  SLButtonHandler	},
	{ ButtonRelease,SLButtonHandler },
};

/*
 *************************************************************************
 * Define Resource list	associated with	the Widget Instance
 ****************************widget*resources*****************************
 */

#define	offset(field)  XtOffset(SliderWidget, field)

static XtResource resources[] =	{
	{XtNsliderMin, XtCSliderMin, XtRInt, sizeof(int),
		offset(slider.sliderMin), XtRImmediate, (XtPointer)DFLT_MIN },
	{XtNsliderMax, XtCSliderMax, XtRInt, sizeof(int),
		offset(slider.sliderMax), XtRImmediate, (XtPointer)DFLT_MAX },
	{XtNsliderValue, XtCSliderValue, XtRInt, sizeof(int),
		offset(slider.sliderValue), XtRImmediate, (XtPointer)DFLT_SLIDERVALUE},
	{XtNorientation, XtCOrientation, XtROlDefine, sizeof(OlDefine),
		offset(slider.orientation), XtRImmediate,
		(XtPointer)OL_VERTICAL },
	{XtNsliderMoved, XtCSliderMoved, XtRCallback, sizeof(XtPointer),
		offset(slider.sliderMoved), XtRCallback, (XtPointer)NULL },
	{XtNgranularity, XtCGranularity, XtRInt, sizeof(int),
		offset(slider.granularity), XtRImmediate, (XtPointer)DFLT_GRANULARITY },
	{XtNrepeatRate,	XtCRepeatRate, XtRInt, sizeof(int),
		offset(slider.repeatRate), XtRImmediate, (XtPointer)DFLT_REPEATRATE},
	{XtNinitialDelay, XtCInitialDelay, XtRInt, sizeof(int),
		offset(slider.initialDelay), XtRImmediate, (XtPointer)DFLT_INITIALDELAY },
	{XtNpointerWarping, XtCPointerWarping, XtRBoolean, sizeof(Boolean),
		offset(slider.warp_pointer), XtRImmediate, 
		(XtPointer)OL_POINTER_WARPING },

	/* resources added in OL 2.1 */
	{XtNendBoxes, XtCEndBoxes, XtRBoolean, sizeof(Boolean),
		offset(slider.endBoxes), XtRImmediate, (XtPointer)True},
	{XtNminLabel, XtCLabel, XtROlStr, sizeof(OlStr),
		offset(slider.minLabel), XtRImmediate, (XtPointer)NULL},
	{XtNmaxLabel, XtCLabel, XtROlStr, sizeof(OlStr),
		offset(slider.maxLabel), XtRImmediate, (XtPointer)NULL},
	{XtNticks, XtCTicks, XtRInt, sizeof(int),
		offset(slider.ticks), XtRImmediate, (XtPointer)0},
	{XtNtickUnit, XtCTickUnit, XtROlDefine, sizeof(OlDefine),
		offset(slider.tickUnit), XtRImmediate, (XtPointer)OL_NONE},
        {XtNdragCBType, XtCDragCBType, XtROlDefine, sizeof(OlDefine),
                offset(slider.dragtype), XtRImmediate,
                (XtPointer)(OlDefine)OL_CONTINUOUS },
        {XtNstopPosition, XtCStopPosition, XtROlDefine, sizeof(OlDefine),
                offset(slider.stoppos), XtRImmediate,
                (XtPointer)(OlDefine)OL_ALL },
	{XtNrecomputeSize, XtCRecomputeSize, XtRBoolean, sizeof(Boolean),
		offset(slider.recompute_size), XtRImmediate, (XtPointer)False},
	{XtNspan, XtCSpan, XtRDimension, sizeof(Dimension),
		offset(slider.span), XtRImmediate, (XtPointer)OL_IGNORE },
	{XtNleftMargin, XtCMargin, XtRDimension, sizeof(Dimension),
		offset(slider.leftMargin), XtRImmediate, (XtPointer)OL_IGNORE },
	{XtNrightMargin, XtCMargin, XtRDimension, sizeof(Dimension),
		offset(slider.rightMargin), XtRImmediate,(XtPointer)OL_IGNORE },
	{XtNuseSetValCallback, XtCUseSetValCallback, XtRBoolean, 
		sizeof(Boolean), offset(slider.useSetValCallback), 
		XtRString, "FALSE"},
};

static XtResource gauge_resources[] =	{
	{XtNsliderMin, XtCSliderMin, XtRInt, sizeof(int),
		offset(slider.sliderMin), XtRImmediate, (XtPointer)DFLT_MIN },
	{XtNsliderMax, XtCSliderMax, XtRInt, sizeof(int),
		offset(slider.sliderMax), XtRImmediate, (XtPointer)DFLT_MAX },
	{XtNsliderValue, XtCSliderValue, XtRInt, sizeof(int),
		offset(slider.sliderValue), XtRImmediate, (XtPointer)DFLT_SLIDERVALUE},
	{XtNorientation, XtCOrientation, XtROlDefine, sizeof(OlDefine),
	offset(slider.orientation), XtRImmediate,(XtPointer)OL_VERTICAL },


	{XtNminLabel, XtCLabel, XtROlStr, sizeof(OlStr),
		offset(slider.minLabel), XtRImmediate, (XtPointer)NULL},
	{XtNmaxLabel, XtCLabel, XtROlStr, sizeof(OlStr),
		offset(slider.maxLabel), XtRImmediate, (XtPointer)NULL},
	{XtNticks, XtCTicks, XtRInt, sizeof(int),
		offset(slider.ticks), XtRImmediate, (XtPointer)0},
	{XtNtickUnit, XtCTickUnit, XtROlDefine, sizeof(OlDefine),
	offset(slider.tickUnit), XtRImmediate, (XtPointer)OL_NONE},

	{XtNrecomputeSize, XtCRecomputeSize, XtRBoolean, sizeof(Boolean),
	offset(slider.recompute_size), XtRImmediate, (XtPointer)False},

	{XtNspan, XtCSpan, XtRDimension, sizeof(Dimension),
		offset(slider.span), XtRImmediate, (XtPointer)OL_IGNORE },
	{XtNleftMargin, XtCMargin, XtRDimension, sizeof(Dimension),
	offset(slider.leftMargin), XtRImmediate, (XtPointer)OL_IGNORE },

	{XtNrightMargin, XtCMargin, XtRDimension, sizeof(Dimension),
	offset(slider.rightMargin), XtRImmediate,(XtPointer)OL_IGNORE },

};
#undef offset

/*
 *************************************************************************
 * Define Slider Class Record structure to be initialized at Compile time
 ***************************widget*class*record***************************
 */
SliderClassRec sliderClassRec =	{
	{
	/* core_class fields	  */
	/* superclass	      */    (WidgetClass) &primitiveClassRec,
	/* class_name	      */    "Slider",
	/* widget_size	      */    sizeof(SliderRec),
	/* class_initialize   */    ClassInitialize,
	/* class_part_init    */    NULL,
	/* class_inited	      */    FALSE,
	/* initialize	      */    Initialize,
	/* initialize_hook    */    NULL,
	/* realize	      */    Realize,
	/* actions	      */    NULL,
	/* num_actions	      */    0,
	/* resources	      */    resources,
	/* num_resources      */    XtNumber(resources),
	/* xrm_class	      */    NULLQUARK,
	/* compress_motion    */    TRUE,
	/* compress_exposure  */    TRUE,
	/* compress_enterleave*/    TRUE,
	/* visible_interest   */    FALSE,
	/* destroy	      */    Destroy,
	/* resize	      */    Resize,
	/* expose	      */    Redisplay,
	/* set_values	      */    SetValues,
	/* set_values_hook    */    NULL,
	/* set_values_almost  */    XtInheritSetValuesAlmost,
	/* get_values_hook    */    GetValuesHook,
	/* accept_focus	      */    XtInheritAcceptFocus,
	/* version	      */    XtVersion,
	/* callback_private   */    NULL,
	/* tm_table	      */    XtInheritTranslations,
	/* query_geometry     */    QueryGeom,
	/* display_accelerator*/    NULL,
	/* extension	      */    NULL,
	},
  {					/* primitive class	*/
      NULL,				/* reserved            */
      HighlightHandler,			/* highlight_handler   */
      NULL,				/* traversal_handler   */
      NULL,				/* register_focus      */
      SLActivateWidget,			/* activate            */
      sl_event_procs,			/* event_procs         */
      XtNumber(sl_event_procs),		/* num_event_procs     */
      OlVersion,			/* version             */
      NULL,				/* extension           */
      { NULL, 0 },			/* dyn_data	       */
      _OlDefaultTransparentProc,	/* transparent_proc    */
      SliderQuerySCLocnProc,		/* query_sc_locn_proc  */
  },
	{
	/* Slider class	fields */
	/* empty		  */	0,
	}
};

/*
 *************************************************************************
 * Public Widget Class Definition of the Widget	Class Record
 *************************public*class*definition*************************
 */
WidgetClass sliderWidgetClass =	(WidgetClass)&sliderClassRec;

/*
 *************************************************************************
 * Define Gauge Class Record structure to be initialized at Compile time
 ***************************widget*class*record***************************
 */
SliderClassRec gaugeClassRec =	{
	{
	/* core_class fields	  */
	/* superclass	      */    (WidgetClass) &primitiveClassRec,
	/* class_name	      */    "Gauge",
	/* widget_size	      */    sizeof(SliderRec),
	/* class_initialize   */    ClassInitialize,
	/* class_part_init    */    NULL,
	/* class_inited	      */    FALSE,
	/* initialize	      */    Initialize,
	/* initialize_hook    */    NULL,
	/* realize	      */    Realize,
	/* actions	      */    NULL,
	/* num_actions	      */    (Cardinal)0,
	/* resources	      */    gauge_resources,
	/* num_resources      */    XtNumber(gauge_resources),
	/* xrm_class	      */    NULLQUARK,
	/* compress_motion    */    FALSE,
	/* compress_exposure  */    TRUE,
	/* compress_enterleave*/    FALSE,
	/* visible_interest   */    FALSE,
	/* destroy	      */    Destroy,
	/* resize	      */    Resize,
	/* expose	      */    Redisplay,
	/* set_values	      */    SetValues,
	/* set_values_hook    */    NULL,
	/* set_values_almost  */    XtInheritSetValuesAlmost,
	/* get_values_hook    */    GetValuesHook,
	/* accept_focus	      */    NULL,
	/* version	      */    XtVersion,
	/* callback_private   */    NULL,
	/* tm_table	      */    NULL,
	/* query_geometry     */    QueryGeom,
	},
  {					/* primitive class	*/
      NULL,				/* reserved            */
      NULL,				/* highlight_handler   */
      NULL,				/* traversal_handler   */
      NULL,				/* register_focus      */
      NULL,				/* activate            */
      NULL,				/* event_procs         */
      0,				/* num_event_procs     */
      OlVersion,			/* version             */
      NULL,				/* extension           */
      { NULL, 0 },			/* dyn_data	       */
      _OlDefaultTransparentProc,	/* transparent_proc    */
      NULL,				/* query_sc_locn_proc   */
  },
	{
	/* Gauge class	fields */
	/* empty		  */	0,
	}
};

/*
 *************************************************************************
 * Public Widget Class Definition of the Widget	Class Record
 *************************public*class*definition*************************
 */
WidgetClass gaugeWidgetClass =	(WidgetClass)&gaugeClassRec;

/*
 *************************************************************************
 * Private Procedures
 ***************************private*procedures****************************
 */

static void
UpdateSuperCaret(SliderWidget sw)
{
	PrimitivePart	*pp = &(sw->primitive);
	OlVendorPartExtension ext_part = pp->ext_part;

	if(!IS_SLIDER(sw) || pp->has_focus == FALSE)
		return;
	if(ext_part == (OlVendorPartExtension)NULL ||
            ext_part->supercaret == (Widget)NULL    ||
            pp->input_focus_feedback != OL_SUPERCARET)
		return;

	_OlCallUpdateSuperCaret(ext_part->supercaret, (Widget)sw);
}

/*
 * calculates the nearest stoppable value for a drag operation.
 */
static int
calc_stoppos(SliderWidget sw, int nearval)
{
	switch(sw->slider.stoppos) {
	case OL_TICKMARK:
		nearval = nearest_tickval(sw, nearval);
		break;
	case OL_GRANULARITY:
		nearval = SLD.sliderMin + (nearval -
			  SLD.sliderMin + SLD.granularity / 2) /
			  SLD.granularity * SLD.granularity;
		break;
	}

	nearval = MIN(SLD.sliderMax, nearval);
	nearval = MAX(SLD.sliderMin, nearval);
	return(nearval);
}

static int
calc_nextstoppos(SliderWidget sw, int val, int dir)
{
	int nearval;

	nearval = calc_stoppos(sw, val);

	if (dir == 1) {
		/* next */
		if (nearval > val)
			return(nearval);

		switch(SLD.stoppos) {
		case OL_TICKMARK:
			switch(SLD.tickUnit) {
			case OL_PERCENT:
				nearval = SLD.sliderMin + (((nearval -
					  SLD.sliderMin) * 100 /
					  (SLD.sliderMax - SLD.sliderMin) +
					  SLD.ticks / 2) /
					  SLD.ticks + 1) * SLD.ticks *
					  (SLD.sliderMax - SLD.sliderMin) / 100;
				break;
			case OL_SLIDERVALUE:
				nearval = SLD.sliderMin + (nearval -
					  SLD.sliderMin + SLD.ticks / 2) /
					  SLD.ticks * SLD.ticks + SLD.ticks;
				break;
			}
			break;
		case OL_GRANULARITY:
			nearval += SLD.granularity;
			break;
		default:
			nearval += 1;
		}
	}
	else {
		/* previous */
		if (nearval < val)
			return(nearval);

		switch(SLD.stoppos) {
		case OL_TICKMARK:
			switch(SLD.tickUnit) {
			case OL_PERCENT:
				nearval = SLD.sliderMin + (((nearval -
					  SLD.sliderMin) * 100 /
					  (SLD.sliderMax - SLD.sliderMin) +
					  SLD.ticks / 2) /
					  SLD.ticks - 1) * SLD.ticks *
					  (SLD.sliderMax - SLD.sliderMin) / 100;
				break;
			case OL_SLIDERVALUE:
				nearval = SLD.sliderMin + (nearval -
					  SLD.sliderMin + SLD.ticks / 2) /
					  SLD.ticks * SLD.ticks - SLD.ticks;
				break;
			}
			break;
		case OL_GRANULARITY:
			nearval -= SLD.granularity;
			break;
		default:
			nearval -= 1;
		}
	}

	nearval = MIN(SLD.sliderMax, nearval);
	nearval = MAX(SLD.sliderMin, nearval);
	return(nearval);
}

static void
GetGCs(SliderWidget sw)
{
	XGCValues values;
	Pixel	focus_color;
	Boolean	has_focus;

       /* destroy old GCs */
	if (sw->slider.labelGC	!= NULL)
	   XtDestroyGC(sw->slider.labelGC);

	if (sw->slider.pAttrs != (OlgxAttrs *) NULL) {
		OlgxDestroyAttrs ((Widget)sw, sw->slider.pAttrs);
	}

       /* get label GC	*/
	if (sw->primitive.font_color == -1)
		values.foreground = sw->primitive.foreground;
	else
		values.foreground = sw->primitive.font_color;
	values.background = sw->core.background_pixel;
	if(sw->primitive.text_format == OL_SB_STR_REP) {
		values.font	  = ((XFontStruct *)sw->primitive.font)
						->fid;
		sw->slider.labelGC= XtGetGC((Widget)sw,
				      (unsigned) (GCForeground | 
						  GCBackground | 
						  	GCFont),
				      			&values);
	} else 
		sw->slider.labelGC= XtGetGC((Widget)sw,
				      (unsigned) (GCForeground | GCBackground),
				      &values);

	/* get other GCs.  Worry about input focus color conflicts */
	focus_color = sw->primitive.input_focus_color;
	has_focus = sw->primitive.has_focus;

	if (has_focus)
	{
	    if (sw->primitive.foreground == focus_color ||
		sw->core.background_pixel == focus_color)
	    {
		/* input focus color conflicts with either the foreground
		 * or background color.  Reverse fg and bg for 3-D.  2-D
		 * is handled specially by making the buttons appear pressed,
		 * so use normal colors.
		 */
		if (OlgIs3d (sw->core.screen))
		    sw->slider.pAttrs =
			OlgxCreateAttrs ((Widget)sw,
					sw->core.background_pixel,
					(OlgxBG *)&(sw->primitive.foreground),
					False, sw->primitive.scale,
					(OlStrRep)0, (OlFont)NULL);
		else
		    sw->slider.pAttrs =
			OlgxCreateAttrs ((Widget)sw,
					sw->primitive.foreground,
					(OlgxBG *)&(sw->core.background_pixel),
					False, sw->primitive.scale,
					(OlStrRep)0, (OlFont)NULL);
	    }
	    else
		/* no color conflict */
		sw->slider.pAttrs =
		    OlgxCreateAttrs ((Widget)sw,
				    sw->primitive.foreground,
				    (OlgxBG *)&(focus_color),
				    False, sw->primitive.scale,
                                    (OlStrRep)0, (OlFont)NULL);
	}
	else
	    /* normal coloration */
	    sw->slider.pAttrs = OlgxCreateAttrs ((Widget)sw,
						sw->primitive.foreground,
						(OlgxBG *)
						&(sw->core.background_pixel),
						False, sw->primitive.scale,
                                    		(OlStrRep)0, (OlFont)NULL); 
} /* GetGCs */

/*
 *************************************************************************
 * TimerEvent -	a function registered to trigger every time the
 *		repeatRate time	interval has elapsed. At the end of
 *		this functions it registers itself.
 *		Thus, elev is moved, ptr is warped and function	is
 *		reregistered.
 ****************************procedure*header*****************************
 */
static void
TimerEvent (XtPointer client_data, XtIntervalId *id)
{
	SliderWidget sw	= (SliderWidget)client_data;
	int warppoint =	0;
	int sx = 0,sy =	0,width	= 0,height = 0;
	int new_location;
	XEvent event;
	Boolean morepending = FALSE;
	Boolean docallback = TRUE;

	if (sw->slider.opcode == NOOP) {
		sw->slider.timerid = (XtIntervalId)NULL;
		return;
	}

       /* determine where to move to */
       if (sw->slider.opcode & ANCHOR)
		new_location = (SLD.opcode & DIR_INC) ?
				(HORIZ(sw) ? SLD.sliderMax : SLD.sliderMin) :
				(HORIZ(sw) ? SLD.sliderMin : SLD.sliderMax);
	else if	(sw->slider.opcode == DRAG_ELEV) {
		Window wjunk;
		int junk, winx,	winy;
		static int min;
		static int max;
		static int valueRange;
		static int pixelRange;
		int pixel;

		if (sw->slider.timerid == (XtIntervalId)NULL) {
			/* first time */
			min = sw->slider.sliderMin;
			max = SLD.sliderMax;
			valueRange = max - min;
			if (HORIZ (sw))
			    pixelRange = sw->core.width -
				sw->slider.leftPad - sw->slider.rightPad -
				2*sw->slider.anchwidth - sw->slider.elevwidth;
			else
			    pixelRange = sw->core.height -
				2*sw->slider.anchlen - sw->slider.elevheight;
		}

		XQueryPointer(XtDisplay(sw), XtWindow(sw), &wjunk, &wjunk,
				&junk, &junk, &winx, &winy,
				(unsigned int *)&junk);

		if (HORIZ(sw))
		    pixel = winx + sw->slider.dragbase;
		else
		    pixel = pixelRange - (winy + sw->slider.dragbase);
		pixel =	PIX_TO_VAL(pixel,valueRange,pixelRange)
		    + SLD.sliderMin;
		new_location = INRANGE(pixel, min, max);

		/*
		 * Handle DragCBType here.
		 */
		if (sw->slider.dragtype == OL_GRANULARITY) {
			if ((new_location != sw->slider.sliderMin) &&
			    (new_location != sw->slider.sliderMax) &&
			    (((new_location - SLD.sliderMin) %
			       SLD.granularity) != 0))
				docallback = FALSE;
		} /* OL_GRANULARITY */
		else if (sw->slider.dragtype == OL_RELEASE)
			docallback = FALSE;

		morepending = TRUE;
	} /* DRAG_TYPE */
       else {
		/* move by a unit of granularity */
	       int step;

		if (SLD.opcode & PAGE)
			step = SLD.granularity;
		else {
			/*
			 * GRAN_DEC or GRAN_INC !!
			 * This can only happen with mouseless operations.
			 * They are used to simulate drag operation. Thus,
			 * they should honor stoppos. See below.
			 */
			step = 1;
		}

		if (!(SLD.opcode & DIR_INC))
			step = - step;
			
		if (!VERT(sw))
			step = - step;

		warppoint = 1;

		if ((SLD.opcode == (GRAN_DEC | KBD_OP)) ||
		    (SLD.opcode == (GRAN_INC | KBD_OP))) {
			new_location = calc_nextstoppos(sw, SLD.sliderValue, step);
		}
		else
			new_location = SLD.sliderValue + step;
			
		new_location = MIN(SLD.sliderMax, new_location);
		new_location = MAX(SLD.sliderMin, new_location);
       }

	if (new_location != sw->slider.sliderValue) {
		MoveSlider(sw, new_location, docallback, morepending);
		if ((ISKBDOP == False) && warppoint &&
			 (SLD.warp_pointer == True)) {
			int wx,	wy;

			if (sw->slider.opcode &	DIR_INC) {
				warppoint = -3;
				if (HORIZ(sw))
					sx = SLD.sliderPValue;
				else
					sy = SLD.sliderPValue;
			}
			else {
				if (HORIZ(sw)) {
					width =	SLD.sliderPValue +
					    SLD.elevwidth;
					warppoint = SLD.elevwidth + 3;
				}
				else {
					height = SLD.sliderPValue +
					    SLD.elevheight;
					warppoint = SLD.elevheight + 3;
				}
			}

			if (HORIZ(sw)) {
				wx = SLD.sliderPValue +	warppoint;
				wy = SLD.elev_offset + SLD.elevheight / 2;
			}
			else {
				wx = SLD.elev_offset + SLD.elevwidth / 2;
				wy = SLD.sliderPValue +	warppoint;
			}
			XWarpPointer(XtDisplay(sw), XtWindow(sw),
				     XtWindow(sw), sx, sy,
				     width, height, wx,	wy);
		}
	}
	else if	(sw->slider.opcode != DRAG_ELEV) {
		SelectUp((Widget)sw,NULL,NULL,NULL);
		return;
	}

	if ((ISKBDOP == False) && !(sw->slider.opcode & ANCHOR)) {
		XtAppContext	ac = XtWidgetToApplicationContext((Widget)sw);
#ifdef NOT_USE
		/*
		 * check for button release here so it seems
		 * more	responsive to the user.
		 */
		if (XCheckWindowEvent(XtDisplay(sw), XtWindow(sw),
				      ButtonReleaseMask, &event)) {
			SelectUp((Widget)sw,NULL,NULL,NULL);
		}
		else {
#endif
			if (sw->slider.opcode == DRAG_ELEV) {
				SLD.timerid = XtAppAddTimeOut(ac, 0, TimerEvent,
							  (XtPointer)sw);
			}
			else if	(!sw->slider.timerid) {
				SLD.timerid = XtAppAddTimeOut(ac,
							      SLD.initialDelay,
							      TimerEvent,
							      (XtPointer)sw);
			}
			else {
				if (sw->slider.repeatRate > 0) {
					SLD.timerid = XtAppAddTimeOut(ac,
							   SLD.repeatRate,
							   TimerEvent,
							   (XtPointer)sw);
				}
			}
#ifdef NOT_USE
		}
#endif
	}
} /* TimerEvent	*/

/*
 ******************************************************
 * DrawTicks -	Function called to draw the Slider/Gauge
 *		tick marks. This was integrated into
 *		Slider.c for the OLGX migration (OLGX
 *		does not draw the ticks for us).
 ******************************************************
 */
static void
DrawTicks (SliderWidget sw, Position from, Position to)
{
    Screen	*scr = XtScreen((Widget)sw);
    Dimension	length, thickness;
    Position	pos;
    Boolean	horizontal;
    Position	*pTick;
    register	i;
    int         state = OlgIs3d(sw->core.screen)? OLGX_NORMAL : OLGX_INVOKED;

    if (!sw->slider.ticklist)
	return;

    if (!XtIsSensitive((Widget)sw))
	state |= OLGX_INACTIVE;

    if (HORIZ(sw)) {
	horizontal = True;
	length = (Dimension)((sw->primitive.scale / OL_SCALE_TO_TICK_LEN_RATIO) * 
		OlgxScreenPointToPixel(OL_VERTICAL, 1, scr));
	thickness = (Dimension)((sw->primitive.scale / OL_SCALE_TO_TICK_WID_RATIO) *
                OlgxScreenPointToPixel(OL_HORIZONTAL, 1, scr));
	if (IS_SLIDER(sw))
	    pos = sw->slider.elev_offset + (Position)sw->slider.elevheight  -
	      	(Position)(2*OlgxScreenPointToPixel (OL_VERTICAL, 1, scr));
	else
	    pos = (Position)Gauge_EndCapHeight(sw->slider.pAttrs->ginfo);

    } else {
	horizontal = False;
        length = (Dimension)((sw->primitive.scale / OL_SCALE_TO_TICK_LEN_RATIO) *
                OlgxScreenPointToPixel(OL_HORIZONTAL, 1, scr));
        thickness  = (Dimension)((sw->primitive.scale / OL_SCALE_TO_TICK_WID_RATIO) * 
                OlgxScreenPointToPixel(OL_VERTICAL, 1, scr)); 
	if (IS_SLIDER(sw))
	    pos = sw->slider.elev_offset + (Position)sw->slider.elevwidth -
                (Position)(2*OlgxScreenPointToPixel (OL_HORIZONTAL, 1, scr));
        else
            pos = (Position)Gauge_EndCapHeight(sw->slider.pAttrs->ginfo);
    }

    pTick = sw->slider.ticklist;
    for (i=sw->slider.numticks; i>0; i--) {
	if (to < (Position) (*pTick + thickness))
	    break;

	if (from < (Position) (*pTick + thickness)) {
	    if (horizontal) {
		olgx_draw_box(sw->slider.pAttrs->ginfo, XtWindow(sw),
				*pTick, pos, thickness, length, state, 1);

	    } else /* Vertical */
                olgx_draw_box(sw->slider.pAttrs->ginfo, XtWindow(sw), 
                                pos, *pTick, length, thickness, state, 1); 
	}
	pTick++;
    }
} /* DrawTicks */

/* 
 * muldiv - Multiply two numbers and divide by a third.
 * Calculate m1*m2/q where m1, m2, and q are integers.  Be careful of
 * overflow. Integrated in for OLGX migration.
 */
#define muldiv(m1, m2, q)	((m2)/(q) * (m1) + (((m2)%(q))*(m1))/(q))

/*
 **************************************************************
 * LocateElevator: Function which determines where the new
 * 	position of the elevator(for Gauge: indicator) should be -- 
 * 	calibrating this value based on the cable length and the
 * 	sliderMin & sliderMax values.  This function was
 *	integrated into Slider for the OLGX migration.
 *
 * OLGX_TODO: this algorithm is designed around using Olg
 * for rendering -- sliderPValue is irrelevent for OLGX.  We
 * should look at redesigning this around OLGX's needs if we
 * have time.
 **************************************************************
 */
static void
LocateElevator(SliderWidget sw, Position cablePos, Dimension cableLen)
{
    Position	elevPos;
    Dimension	elevLen;
    int		userRange;	/* size of user coordinate space */

    if (HORIZ(sw))
	elevLen = sw->slider.elevwidth;
    else
	elevLen = sw->slider.elevheight;

    /* Find the position of the elevator */
    userRange = sw->slider.sliderMax - sw->slider.sliderMin;
    if (userRange > 0)
	elevPos = muldiv (sw->slider.sliderValue - sw->slider.sliderMin,
			  (int) cableLen, userRange);
    else
	elevPos = 0;

    if (VERT(sw))
	elevPos = cableLen - elevPos;

    sw->slider.sliderPValue = cablePos + elevPos;

} /* LocateElevator */

/*
 ****************************************************************
 * UpdateSlider: Function which re-draws only the part of the
 * 	Slider/Gauge indicated by the "flags" argument.  This
 *	routine was integrated for the OLGX migration.
 ****************************************************************
 */
static void
UpdateSlider(SliderWidget sw, OlgxAttrs *pInfo, unsigned int flags)
{
    unsigned	horizontal = (sw->slider.orientation == OL_HORIZONTAL);
    unsigned    isGauge = IS_GAUGE((Widget)sw);
    Display	*dpy = XtDisplay (sw);
    Window	win = XtWindow (sw);
    Screen	*scr = XtScreen (sw);
    Position	cablePos;	/* screen coordinate of top/left of cable */
    Dimension	cableLen;	/* length of cable area */
    Dimension	elevLen;	/* length of elevator */
    Dimension	padLen;		/* length of gap between elements */
    int		colorConflict;	/* input focus color clashes with fg or bg */
    Pixel	focus_color;
    Boolean	has_focus;
    int		state = OLGX_ERASE; /* OLGX rendering state */

    if (!XtIsSensitive((Widget)sw)) 
	state |= OLGX_INACTIVE;

    focus_color = sw->primitive.input_focus_color;
    has_focus = sw->primitive.has_focus;

    /* If the input focus color is the same as bg or fg, then something
     * has to change.  If we are in 3-D mode, then fg and bg have already
     * been reversed, and we don't need to worry about it here.  If we are
     * in 2-D mode, then draw the elevator and anchors as if they are pressed
     * whenever there is a conflict.
     */
    if (!OlgIs3d (sw->core.screen) && has_focus &&
	(sw->primitive.foreground == focus_color ||
	 sw->core.background_pixel == focus_color))
	colorConflict = 1;
    else
	colorConflict = 0;

    if (horizontal) {
	cablePos = sw->slider.leftPad;
	cableLen = sw->core.width - cablePos - sw->slider.rightPad;
	padLen = OlgxScreenPointToPixel(OL_HORIZONTAL, 
		       (sw->primitive.scale / OL_SCALE_TO_PAD_RATIO), scr);
	elevLen = sw->slider.elevwidth;
    
    } else {
	cablePos = 0;
	cableLen = sw->core.height;
	padLen = OlgxScreenPointToPixel(OL_VERTICAL, 
			(sw->primitive.scale / OL_SCALE_TO_PAD_RATIO), scr);
	elevLen = sw->slider.elevheight;
    }

    /* Update anchors */
    /* Draw anchors with olgx_draw_choice_item() so that we can easily make
     * them inactive.
     */
    if (sw->slider.endBoxes) {
	Position	anchOffset;
	Position	elevAnchDiff;

	if (horizontal) {
            elevAnchDiff = sw->slider.elevheight - sw->slider.anchlen;  
            anchOffset = sw->slider.elev_offset + elevAnchDiff / 2 + 
                               elevAnchDiff % 2; 

	    if (flags & SL_BEGIN_ANCHOR)
    		olgx_draw_choice_item(pInfo->ginfo, win, cablePos, anchOffset, 
				sw->slider.anchwidth, sw->slider.anchlen, (long)NULL, 
			        state | ((sw->slider.opcode == ANCHOR_TOP)^colorConflict?
					OLGX_INVOKED : OLGX_NORMAL));

	    if (flags & SL_END_ANCHOR)
                olgx_draw_choice_item(pInfo->ginfo, win, cablePos+cableLen-sw->slider.anchwidth, 
                                anchOffset, sw->slider.anchwidth, sw->slider.anchlen, (long)NULL, 
                                state | ((sw->slider.opcode == ANCHOR_BOT)^colorConflict?
                                        OLGX_INVOKED : OLGX_NORMAL));
	    cableLen -= (sw->slider.anchwidth * 2 + padLen * 2);
	    cablePos += (sw->slider.anchwidth + padLen);
	
	} else {
            elevAnchDiff = sw->slider.elevwidth - sw->slider.anchwidth;  
            anchOffset = sw->slider.elev_offset + elevAnchDiff / 2 + 
                               elevAnchDiff % 2; 

	    if (flags & SL_BEGIN_ANCHOR)
                olgx_draw_choice_item(pInfo->ginfo, win, anchOffset, cablePos,
                               sw->slider.anchwidth, sw->slider.anchlen, (long)NULL,
                               state | ((sw->slider.opcode == ANCHOR_TOP)^colorConflict?
                                        OLGX_INVOKED : OLGX_NORMAL));

	    if (flags & SL_END_ANCHOR)
                olgx_draw_choice_item(pInfo->ginfo, win, anchOffset, 
			       sw->core.height - sw->slider.anchlen, 
                               sw->slider.anchwidth, sw->slider.anchlen, (long)NULL, 
                               state | ((sw->slider.opcode == ANCHOR_BOT)^colorConflict?
                                        OLGX_INVOKED : OLGX_NORMAL));
	    cableLen -= (sw->slider.anchlen * 2 + padLen * 2);
	    cablePos += (sw->slider.anchlen + padLen);
	}
    }

    /* If we are only updating an anchor, we're done */
    if (!(flags & ~(SL_BEGIN_ANCHOR | SL_END_ANCHOR)))
	return;

    /* Update the position of the elevator(Slider) or indicator(Gauge) */
    if (flags & SL_POSITION || flags & SL_DRAG) {
	Position	oldElevPos; 
	Position	x, y; /* Position of Slider/Gauge */
	int		adjustedValue, adjustedOldValue;

	if (flags & SL_DRAG)
	    state |= OLGX_INVOKED;

	/* Save the old elevator/indicator position */
	oldElevPos = sw->slider.sliderPValue;

	if (!isGauge)
	    LocateElevator (sw, cablePos, cableLen - elevLen);
	else {
	    if (horizontal)
		LocateElevator (sw, sw->slider.minTickPos,
				sw->slider.maxTickPos - sw->slider.minTickPos);
	    else
		LocateElevator (sw, sw->slider.maxTickPos,
				sw->slider.minTickPos - sw->slider.maxTickPos);
	    elevLen = 0;
	}

	if (horizontal) {
	    state |= OLGX_HORIZONTAL;
	    x = cablePos;
	    y = sw->slider.elev_offset; /* if Gauge, elev_offset = 0 */
	    adjustedValue = sw->slider.sliderPValue - cablePos;
	    adjustedOldValue = oldElevPos - cablePos;

	} else {
	    state |= OLGX_VERTICAL;
	    x = sw->slider.elev_offset; /* if Gauge, elev_offset = 0 */
	    y = cablePos;
	    adjustedValue = cableLen - (sw->slider.sliderPValue - cablePos) -
		(isGauge ? 0 : sw->slider.elevheight);
	    adjustedOldValue = cableLen - (oldElevPos - cablePos) -
                (isGauge ? 0 : sw->slider.elevheight);
	}


   	if (!isGauge) { /* Update Slider */

	    olgx_draw_slider(pInfo->ginfo, win, x, y, cableLen,
                	adjustedOldValue, adjustedValue, state | OLGX_UPDATE); 

	    /* Repaint damaged tick marks damaged by moving elevator */
            if (oldElevPos < sw->slider.sliderPValue) 
            	DrawTicks(sw, cablePos, sw->slider.sliderPValue);
            else
              	DrawTicks(sw, sw->slider.sliderPValue + 
		     (horizontal? sw->slider.elevwidth : sw->slider.elevheight), 
			 cablePos + cableLen);

    	} else  /* Update Gauge */
	    olgx_draw_gauge(pInfo->ginfo, win, x, y, cableLen,
			adjustedOldValue, adjustedValue, state | OLGX_UPDATE);
    }

} /* UpdateSlider */

/*
 **************************************************************
 * DrawSlider: Function which completely draws the Slider/Gauge:
 *	End boxes, cable/elevator, tickmarks, min/max labels.
 *	This was integrated into Slider.c for OLGX migration.
 **************************************************************
 */
static void
DrawSlider(register SliderWidget sw, OlgxAttrs *pInfo)
{
    Display	*dpy = XtDisplay (sw);
    Screen	*scr = XtScreen (sw);
    Drawable	win = XtWindow (sw);
    unsigned	horizontal = (sw->slider.orientation == OL_HORIZONTAL);
    unsigned    isGauge    = IS_GAUGE(sw);
    Position	cablePos;	/* screen coordinate of top/left of cable */
    Dimension	cableLen;	/* length of cable area */
    Dimension	elevLen;	/* length of elevator */
    Dimension	padLen;		/* length of gap between elements */
    int		colorConflict;	/* input focus color clashes with fg or bg */
    Pixel	focus_color;
    Boolean	has_focus;
    int		state = OLGX_ERASE;		/* for OLGX flag */

    if (!XtIsSensitive((Widget)sw))
	state |= OLGX_INACTIVE;

    focus_color = sw->primitive.input_focus_color;
    has_focus = sw->primitive.has_focus;

    /* If the input focus color is the same as bg or fg, then something
     * has to change.  If we are in 3-D mode, then fg and bg have already
     * been reversed, and we don't need to worry about it here.  If we are
     * in 2-D mode, then draw the elevator and anchors as if they are pressed
     * whenever there is a conflict.
     */
    if (!OlgIs3d (sw->core.screen) && has_focus &&
	(sw->primitive.foreground == focus_color ||
	 sw->core.background_pixel == focus_color))
	colorConflict = 1;
    else
	colorConflict = 0;

    if (horizontal) {
	state |= OLGX_HORIZONTAL;
	cablePos = sw->slider.leftPad;
	cableLen = sw->core.width - cablePos - sw->slider.rightPad;
	padLen = OlgxScreenPointToPixel(OL_HORIZONTAL, 
			(sw->primitive.scale / OL_SCALE_TO_PAD_RATIO), scr);
	elevLen = sw->slider.elevwidth;
    
    } else {
	state |= OLGX_VERTICAL;
	cablePos = 0;
	cableLen = sw->core.height;  
	padLen = OlgxScreenPointToPixel(OL_VERTICAL, 
			(sw->primitive.scale / OL_SCALE_TO_PAD_RATIO), scr);
	elevLen = sw->slider.elevheight;
    }

    /* If the slider has anchors, draw them. 
     * Use olgx_draw_choice_item() so we can easily render them 
     * insensitive.
     */
    if (sw->slider.endBoxes) {
	Position	anchOffset;
	Position	elevAnchDiff;

	if (horizontal) {
	    elevAnchDiff = sw->slider.elevheight - sw->slider.anchlen;
	    anchOffset = sw->slider.elev_offset + elevAnchDiff / 2 +
			       elevAnchDiff % 2;

            olgx_draw_choice_item(pInfo->ginfo, win, cablePos, anchOffset,
                           sw->slider.anchwidth, sw->slider.anchlen, (long)NULL,
                           state | ((sw->slider.opcode == ANCHOR_TOP)^colorConflict?
                                        OLGX_INVOKED : OLGX_NORMAL));

            olgx_draw_choice_item(pInfo->ginfo, win, 
			   cablePos + cableLen - sw->slider.anchwidth, anchOffset,
                           sw->slider.anchwidth, sw->slider.anchlen, (long)NULL,
                           state | ((sw->slider.opcode == ANCHOR_BOT)^colorConflict?
                                        OLGX_INVOKED : OLGX_NORMAL));
	    cableLen -= (sw->slider.anchwidth * 2 + padLen * 2);
	    cablePos += (sw->slider.anchwidth + padLen);
	
	} else {
            elevAnchDiff = sw->slider.elevwidth - sw->slider.anchwidth; 
            anchOffset = sw->slider.elev_offset + elevAnchDiff / 2 +
                               elevAnchDiff % 2;

            olgx_draw_choice_item(pInfo->ginfo, win, anchOffset, cablePos, 
                           sw->slider.anchwidth, sw->slider.anchlen, (long)NULL,
                           state | ((sw->slider.opcode == ANCHOR_TOP)^colorConflict?
                                        OLGX_INVOKED : OLGX_NORMAL));

            olgx_draw_choice_item(pInfo->ginfo, win, anchOffset, 
			   sw->core.height - sw->slider.anchlen, 
                           sw->slider.anchwidth, sw->slider.anchlen, (long)NULL, 
                           state | ((sw->slider.opcode == ANCHOR_BOT)^colorConflict?
                                        OLGX_INVOKED : OLGX_NORMAL));
	    cableLen -= (sw->slider.anchlen * 2 + padLen * 2);
	    cablePos += (sw->slider.anchlen + padLen);
	}
    }

    if (!isGauge)
	LocateElevator (sw, cablePos, cableLen - elevLen);
    else {
	if (horizontal)
	    LocateElevator (sw, sw->slider.minTickPos,
			    sw->slider.maxTickPos - sw->slider.minTickPos);
	else
	    LocateElevator (sw, sw->slider.maxTickPos,
			    sw->slider.minTickPos - sw->slider.maxTickPos);
	elevLen = padLen = 0;
    }

    if (sw->slider.type == SB_REGULAR) {
	Position	x, y;
	int		adjustedValue; /* OLGX needs a calibrated slider value;
					*(sliderPValue is the calibrated pixel 
					* offset of the elevator)
					*/
	if (horizontal) {
	    x = cablePos;
	    y = sw->slider.elev_offset;
	    adjustedValue = sw->slider.sliderPValue - cablePos;
	
	} else {
	    x = sw->slider.elev_offset;
	    y = cablePos;
	    adjustedValue = cableLen - (sw->slider.sliderPValue - cablePos) -
		(isGauge? 0 : sw->slider.elevheight); 

	}
        DrawTicks(sw, cablePos, cablePos + cableLen);

	if (!isGauge)  /* Draw Slider */
            olgx_draw_slider(pInfo->ginfo, win, x, y, cableLen, 
                	0 /*ignored*/, adjustedValue, state); 

	else /* Draw Gauge */
	    olgx_draw_gauge(pInfo->ginfo, win, x, y, cableLen,
			0 /*ignored*/, adjustedValue, state);

    }

    /* Draw the labels, if any */
    if (sw->slider.minLabel || sw->slider.maxLabel) {
	Position	x, y;
	Dimension	minWidth, minHeight;
	int		len;
	OlStrRep	tf = sw->primitive.text_format;
	OlFont font = sw->primitive.font;
	XRectangle overall_ink, overall_logical;
	GC labelGC = sw->slider.labelGC;

	SizeSlider (sw, sw->slider.pAttrs, &minWidth, &minHeight);

	if (sw->slider.minLabel) {
	    (*str_methods[tf].StrExtents)(font,sw->slider.minLabel,
		(*str_methods[tf].StrNumUnits)(sw->slider.minLabel),
				&overall_ink, &overall_logical);

	    if (horizontal) {
		y = sw->slider.elev_offset + minHeight + 
					(-overall_logical.y);
		x = sw->slider.minTickPos - overall_logical.width / 2;

		if (x < 0)
		    x = 0;
	    
	    } else {
		x = sw->slider.elev_offset + minWidth;
		y = MIN(sw->slider.minTickPos + (-overall_logical.y)/2,
			(int) (sw->core.height - (overall_logical.height +
				overall_logical.y)));
	    }

	    (*str_methods[tf].StrDraw)(dpy,win,font,labelGC,x,y,
					sw->slider.minLabel,
			(*str_methods[tf].StrNumUnits)
			(sw->slider.minLabel) );
	}

	if (sw->slider.maxLabel) {
	    (*str_methods[tf].StrExtents)(font,sw->slider.maxLabel,
		(*str_methods[tf].StrNumUnits)(sw->slider.maxLabel),
				&overall_ink, &overall_logical);

	    if (horizontal) {
		y = sw->slider.elev_offset + minHeight + 
					(-overall_logical.y);
		x = sw->slider.maxTickPos - overall_logical.width / 2;
		if (x < 0)
		    x = 0;
	    
	    } else {
		x = sw->slider.elev_offset + minWidth;
		y = MAX(sw->slider.maxTickPos + (-overall_logical.y) / 2,
			-overall_logical.y);
	    }
	    (*str_methods[tf].StrDraw)(dpy,win,font,labelGC,x,y,
					sw->slider.maxLabel,
			(*str_methods[tf].StrNumUnits)
			(sw->slider.maxLabel) );
	}
    }

} /* DrawSlider */

static void
MoveSlider(SliderWidget sw, int new_location, Boolean callback, Boolean more)
{

	if (callback) {
		OlSliderVerify olsl;

		olsl.new_location = new_location;
		olsl.more_cb_pending = more;
		XtCallCallbacks((Widget)sw, XtNsliderMoved, (XtPointer)&olsl);

		/* copy newloc back, to maintain 2.0 behavior */
		new_location = olsl.new_location;
	}

	/*
	 * Note	that even if delta is zero, you	still need to do the stuffs
	 * below. Maybe	max or min has changed.
	 */
	sw->slider.sliderValue = new_location;
	if  (sw->slider.type ==	SB_REGULAR) 
		UpdateSlider(sw, sw->slider.pAttrs, 
			(more? SL_DRAG : 0) | SL_POSITION);

} /* MoveSlider	*/

/*
 ***********************************************************
 * Recalc: 
 * This	function calculates all	the dimensions for a slider.
 ***********************************************************
 */
static void
Recalc(SliderWidget sw)
{
	Dimension elevWidth, elevHeight;
	Dimension anchorWidth, anchorHeight;

	/* calculate dimensions of anchors and elevator */

	if (sw->slider.endBoxes) 
	    SizeSliderAnchor(sw, sw->slider.pAttrs,
		&anchorWidth, &anchorHeight);
	else 
	    anchorWidth = anchorHeight = 0;
	    
	sw->slider.anchwidth = anchorWidth;
        sw->slider.anchlen = anchorHeight;

	if (HORIZ(sw)) {
	    sw->slider.elevwidth = (Dimension)HorizSliderControl_Width(sw->slider.pAttrs->ginfo);
	    sw->slider.elevheight = (Dimension)HorizSliderControl_Height(sw->slider.pAttrs->ginfo);
	} else {
            sw->slider.elevwidth = (Dimension)HorizSliderControl_Height(sw->slider.pAttrs->ginfo); 
            sw->slider.elevheight = (Dimension)HorizSliderControl_Width(sw->slider.pAttrs->ginfo);
	}

	/* calculate left padding */
	SLD.leftPad = calc_leftMargin(sw);	

	/* calculate right padding */
	SLD.rightPad = calc_rightMargin(sw);	

	sw->slider.type	= SB_REGULAR;
	if (HORIZ (sw)) {
	    Dimension	length;

	    length = sw->core.width - SLD.leftPad - SLD.rightPad;

	    if ((Dimension) (SLD.anchwidth * 2 + SLD.elevwidth) >= length)
		sw->slider.type	= SB_MINREG;

	    SLD.elev_offset = 0;

	    /* calculate min and max tick positons */
	    if (IS_SLIDER(sw)) {
	    	SLD.minTickPos = SLD.leftPad + sw->slider.anchwidth +
			(Position) sw->slider.elevwidth/2;
	        SLD.maxTickPos = sw->core.width - sw->slider.anchwidth -
			(Position) (sw->slider.elevwidth + 1)/2 - SLD.rightPad;

	    } else { /* Gauge uses different Metrics for OLGX */
		SLD.minTickPos = SLD.leftPad + 
			(Position)Gauge_EndCapOffset(sw->slider.pAttrs->ginfo);
	        SLD.maxTickPos = sw->core.width - SLD.rightPad -
			(Position)Gauge_EndCapOffset(sw->slider.pAttrs->ginfo);
	    }
	} else {
	    if ((Dimension)(SLD.anchlen*2 + SLD.elevheight) >= sw->core.height)
		sw->slider.type	= SB_MINREG;

	    SLD.elev_offset = SLD.leftPad;

	    /* calculate min and max tick positons */
            if (IS_SLIDER(sw)) { 
	    	SLD.maxTickPos = sw->slider.anchlen +
			(Position) sw->slider.elevheight/2;
	    	SLD.minTickPos = sw->core.height - sw->slider.anchlen -
			(Position) (sw->slider.elevheight + 1)/2;

	    } else { /* Gauge uses different metrics for OLGX */
		SLD.maxTickPos = (Position)Gauge_EndCapOffset(sw->slider.pAttrs->ginfo);
		SLD.minTickPos = (Position)sw->core.height -
			(Position)Gauge_EndCapOffset(sw->slider.pAttrs->ginfo);
	    }
	}

	make_ticklist(sw);
} /* Recalc */

static void
CheckValues(SliderWidget sw)
{
       int range;
	int save;

	if (sw->slider.orientation != OL_HORIZONTAL) {
		if (sw->slider.orientation != OL_VERTICAL)
			SLError(sw, 0, (int)sw->slider.orientation);
		sw->slider.orientation = OL_VERTICAL;
	}

       if (sw->slider.sliderMin	>= sw->slider.sliderMax) {
		SLError(sw, 1, sw->slider.sliderMin);
	       sw->slider.sliderMin = DFLT_MIN;
	       sw->slider.sliderMax = DFLT_MAX;
       }

       range = sw->slider.sliderMax - sw->slider.sliderMin;

	save = SLD.sliderValue;
       SLD.sliderValue = INRANGE(SLD.sliderValue,SLD.sliderMin,SLD.sliderMax);
	if (save != SLD.sliderValue)
		SLError(sw, 3, save);

       if (sw->primitive.scale < 1)
	       sw->primitive.scale	= 10;

	if (IS_SLIDER(sw)) {
		save = SLD.granularity;
       		SLD.granularity = INRANGE(SLD.granularity,1,range);
		if (save != SLD.granularity)
			SLError(sw, 4, save);

       		if (sw->slider.repeatRate < 1) {
			SLError(sw, 6, sw->slider.repeatRate);
	       		sw->slider.repeatRate = DFLT_REPEATRATE;
		}
       		if (sw->slider.initialDelay < 1) {
			SLError(sw, 5, sw->slider.initialDelay);
	       		sw->slider.initialDelay = DFLT_INITIALDELAY;
		}

		if ((SLD.dragtype != OL_GRANULARITY) &&
	    	(SLD.dragtype != OL_CONTINUOUS) &&
	    	(SLD.dragtype != OL_RELEASE)) {
			SLError(sw, 12, (int)SLD.dragtype);
			SLD.dragtype = OL_CONTINUOUS;
		}

		if ((SLD.stoppos != OL_GRANULARITY) &&
	    	(SLD.stoppos != OL_TICKMARK) &&
	    	(SLD.stoppos != OL_ALL)) {
			SLError(sw, 13, (int)SLD.stoppos);
			SLD.stoppos = OL_ALL;
		}
	}
	else {
		/* force behavior for gauge widgets */
		SLD.endBoxes = FALSE;
	}

       if (SLD.ticks < 0) {
		SLError(sw, 7, SLD.ticks);
	       sw->slider.ticks = 0;
	}

	if ((SLD.span != (Dimension)OL_IGNORE) &&
	    ((int)SLD.span < 1)) {
		SLError(sw, 8, (int)sw->slider.span);
		sw->slider.span = OL_IGNORE;
	}

	if (((SLD.tickUnit != OL_PERCENT) && (SLD.tickUnit != OL_SLIDERVALUE) &&
	    (SLD.tickUnit != OL_NONE)) ||
	    ((SLD.tickUnit != OL_NONE) && (SLD.ticks == 0))) {
		SLError(sw, 9, (int)sw->slider.tickUnit);
		sw->slider.tickUnit = OL_NONE;
	}

	if (SLD.leftMargin < 0) {
		SLError(sw, 10, (int)SLD.leftMargin);
		SLD.leftMargin = OL_IGNORE;
	}

	if (SLD.rightMargin < 0) {
		SLError(sw, 11, (int)SLD.rightMargin);
		SLD.rightMargin = OL_IGNORE;
	}

} /* CheckValues */

static Dimension 
calc_leftMargin(SliderWidget sw)
{
	Dimension labelwidth;
	Dimension tiplen;
	OlStrRep tf = sw->primitive.text_format;
	XRectangle overall_ink,overall_logical;

	if ((Dimension)SLD.leftMargin != (Dimension)OL_IGNORE)
		return(SLD.leftMargin);

	if (VERT(sw) || (SLD.minLabel == NULL))
		return((Dimension)0);

	(*str_methods[tf].StrExtents)(sw->primitive.font,
			 SLD.minLabel,
			(*str_methods[tf].StrNumUnits)(SLD.minLabel),
					&overall_ink,&overall_logical);
	labelwidth = (Dimension)(overall_logical.width / 2);

	tiplen = sw->slider.anchwidth + sw->slider.elevwidth/2;
	if (labelwidth < tiplen)
		return((Dimension)0);

	else
		return(labelwidth - tiplen);
} /* calc_leftMargin */

static Dimension 
calc_rightMargin(SliderWidget sw)
{
	Dimension labelwidth;
	Dimension tiplen;
	XRectangle overall_ink,overall_logical;
	OlStrRep tf = sw->primitive.text_format;

	if ((Dimension)SLD.rightMargin != (Dimension)OL_IGNORE)
		return(SLD.rightMargin);

        if (VERT(sw)) {
                Dimension minwidth;

		if (SLD.minLabel != NULL) {
			(*str_methods[tf].StrExtents)(sw->primitive.font,
			 SLD.minLabel,
			(*str_methods[tf].StrNumUnits)(SLD.minLabel),
					&overall_ink,&overall_logical);
                	minwidth = (Dimension)overall_logical.width;
		}
		else
			minwidth = 0;

		if (SLD.maxLabel != NULL) {
			(*str_methods[tf].StrExtents)(sw->primitive.font,
				 SLD.maxLabel,
			(*str_methods[tf].StrNumUnits)(SLD.maxLabel),
					&overall_ink,&overall_logical);
			labelwidth = (Dimension)overall_logical.width;
		}
		else
			labelwidth = 0;

                labelwidth = MAX(minwidth, labelwidth);
		return(labelwidth);
        }

	if (SLD.maxLabel != NULL) {
		(*str_methods[tf].StrExtents)(sw->primitive.font,
			 SLD.maxLabel,
			(*str_methods[tf].StrNumUnits)(SLD.maxLabel),
					&overall_ink,&overall_logical);
        	labelwidth = (Dimension)(overall_logical.width/2);
		tiplen = sw->slider.anchwidth +
		    (Dimension) (sw->slider.elevwidth+1)/2;
		if (labelwidth < tiplen)
			return((Dimension)0);
		else
			return(labelwidth - tiplen);
	}
	else
		return((Dimension)0);
} /* calc_rightMargin */


/* Define Slider Rendering RATIOS as per OPEN LOOK Functional Spec. */
#define OL_SCALE_TO_H_ENDBOX_WDT_RATIO  2   /* Table B-38: row e (p.458) */
#define OL_SCALE_TO_H_ENDBOX_HGT_DIFF   1   /* Table B-38: row d (p.458) */

 /*
  * SizeSliderAnchor: determine the width & height of the
  * slider anchor (endbox) according to scale and orientation.
  */
void
SizeSliderAnchor(SliderWidget sw, OlgxAttrs *pInfo, 
                     Dimension *pWidth, Dimension *pHeight)
{
    Screen *scr = XtScreenOfObject((Widget)sw);
    int scale = sw->primitive.scale;

    if (HORIZ(sw)) {
        *pWidth=OlgxScreenPointToPixel(OL_HORIZONTAL,
                (scale / OL_SCALE_TO_H_ENDBOX_WDT_RATIO), scr);
        *pHeight=OlgxScreenPointToPixel(OL_VERTICAL,
                (scale - OL_SCALE_TO_H_ENDBOX_HGT_DIFF), scr);

    } else {  /* Vertical */
        *pHeight=OlgxScreenPointToPixel(OL_VERTICAL,
                (scale / OL_SCALE_TO_H_ENDBOX_WDT_RATIO), scr);
        *pWidth=OlgxScreenPointToPixel(OL_HORIZONTAL,
                (scale - OL_SCALE_TO_H_ENDBOX_HGT_DIFF), scr);
    } 
 
} /* SizeSliderAnchor */

static void
SizeSlider(SliderWidget sw, OlgxAttrs *pInfo, Dimension *pWidth, Dimension *pHeight)
{
    Screen *scr = XtScreenOfObject((Widget)sw);

    if (IS_SLIDER(sw)) {
    	Dimension   anchorWidth, anchorHeight;
    	Dimension   elevWidth, elevHeight;
    	Dimension   extent;

	if (sw->slider.endBoxes)
    	    SizeSliderAnchor (sw, pInfo, &anchorWidth, &anchorHeight);
	else
	    anchorWidth = anchorHeight = 0;


   	if (HORIZ(sw)) {
            elevWidth = (Dimension)HorizSliderControl_Width(pInfo->ginfo);
    	    elevHeight = (Dimension)HorizSliderControl_Height(pInfo->ginfo);
            extent = elevWidth + anchorWidth * 2;
 
            *pWidth = MAX(sw->core.width, extent);
            *pHeight = elevHeight;

    	} else {
            elevWidth = (Dimension)HorizSliderControl_Height(pInfo->ginfo); 
            elevHeight = (Dimension)HorizSliderControl_Width(pInfo->ginfo); 
            extent = elevHeight + anchorHeight * 2;
 
            *pWidth = elevWidth;
            *pHeight = MAX(sw->core.height, extent);
    	}
    } else { /* is Gauge */
        if (HORIZ(sw)) {
	    *pWidth =  sw->core.width;
	    *pHeight = (Dimension)Gauge_EndCapHeight(pInfo->ginfo);

	} else {
	    *pWidth = (Dimension)Gauge_EndCapHeight(pInfo->ginfo);
	    *pHeight = sw->core.height;
	}
   }
   if (HORIZ(sw)) {
        if (sw->slider.tickUnit != OL_NONE) { /* Add tick mark length */
             *pHeight += (Dimension)(sw->primitive.scale/OL_SCALE_TO_TICK_LEN_RATIO);

             if (IS_SLIDER(sw))     /* slider tick marks are set back */  
		*pHeight -= (Dimension)(2*OlgxScreenPointToPixel(OL_VERTICAL, 1, scr));
	}
 
	/* Add offset for labels */
        if (sw->slider.minLabel || sw->slider.maxLabel)
             *pHeight += (Dimension)(2*OlgxScreenPointToPixel(OL_VERTICAL, 1, scr));
 
    } else {
        if (sw->slider.tickUnit != OL_NONE) { /* Add tick mark length */
            *pWidth += (Dimension)(sw->primitive.scale/OL_SCALE_TO_TICK_LEN_RATIO);

	    if (IS_SLIDER(sw))     /* slider tick marks are set back */  
                 *pWidth -= (Dimension)(2*OlgxScreenPointToPixel(OL_HORIZONTAL, 1, scr));
	}
 
	/* Add offset for labels */
        if (sw->slider.minLabel || sw->slider.maxLabel)
             *pWidth += (Dimension)(2*OlgxScreenPointToPixel(OL_HORIZONTAL, 1, scr));
    }

} /* SizeSlider */


static void 
calc_dimension(SliderWidget sw, Dimension *width, Dimension *height)
{
	Dimension minlabelw;
	Dimension maxlabelw;
	int max;
	Dimension lpad;
	Dimension rpad;
	Dimension minWidth, minHeight, tempHeight;
	XRectangle overall_ink,overall_logical;
	OlStrRep tf = sw->primitive.text_format;


	lpad = calc_leftMargin(sw);
	rpad = calc_rightMargin(sw);

	SizeSlider(sw, sw->slider.pAttrs, &minWidth, &minHeight);
	
	if (VERT(sw)) {
		*width = lpad + minWidth + rpad;
		if (SLD.span != (Dimension)OL_IGNORE)
			*height = SLD.span;
		else
			*height = MAX(*height, minHeight);
	}
	else {
		/* horizontal */
		if (SLD.span != (Dimension)OL_IGNORE)
			*width = lpad + SLD.span + rpad;
		else
			*width = MAX(*width, minWidth);

		*height = minHeight;
		if (SLD.minLabel || SLD.maxLabel) {
			
		if(SLD.maxLabel) {
			(*str_methods[tf].StrExtents)(sw->primitive.font,
				 SLD.maxLabel,
			(*str_methods[tf].StrNumUnits)(SLD.maxLabel),
					&overall_ink,&overall_logical);
		  	tempHeight = (Dimension)overall_logical.height;
			maxlabelw = (Dimension)overall_logical.width;
		}	

		if(SLD.minLabel) {
			(*str_methods[tf].StrExtents)(sw->primitive.font,
				 SLD.minLabel,
			(*str_methods[tf].StrNumUnits)(SLD.minLabel),
					&overall_ink,&overall_logical);
		  	tempHeight = ((Dimension)overall_logical.height
				> tempHeight ? (Dimension)
				overall_logical.height : tempHeight);
			minlabelw = (Dimension)overall_logical.width;
		}	
		*height += tempHeight;

		if (SLD.span == (Dimension)OL_IGNORE)  {
			*width = MAX ((Dimension) *width,
				      (Dimension) (lpad + minWidth + rpad));
			if (SLD.minLabel && SLD.maxLabel) {
			    OlStr space = (tf == OL_WC_STR_REP) ?
						(OlStr)L" " : (OlStr)" ";

			    (*str_methods[tf].StrExtents)(
				sw->primitive.font,space,
				(*str_methods[tf].StrNumUnits)(space),
					&overall_ink,&overall_logical);
			    max = minlabelw + (Dimension)
				overall_logical.width + maxlabelw -
				ABS(SLD.maxTickPos - SLD.minTickPos);

			    if (max > 0)
				/*
				 * needs more space between minTick and
				 * maxTicks to show both labels clearly.
				 */
				*width += max;
			}
		}

		}
	}
} /* calc_dimension */

static void
make_ticklist(SliderWidget sw)
{
	int i;
	int urange;
	Position range;
	Position *pp;

	/* free the old list */
	if (sw->slider.ticklist) {
		XtFree((char*)sw->slider.ticklist);
		sw->slider.ticklist = NULL;
	}

	if (!TICKS_ON(sw)) 
		return;

	switch(sw->slider.tickUnit) {
	case OL_PERCENT:
		urange = 100;
		sw->slider.numticks = 100 / sw->slider.ticks + 1;
		break;
	case OL_SLIDERVALUE:
		urange = SLD.sliderMax - SLD.sliderMin;
		sw->slider.numticks = urange / sw->slider.ticks + 1;
		break;
	}
	range = SLD.maxTickPos - SLD.minTickPos;
	if (range < 0)
		range = - range;
	
	/* allocate space */
	pp = SLD.ticklist = (Position *)XtMalloc(SLD.numticks * sizeof(Position));

	if (VERT(sw))
	    for (i=SLD.numticks - 1; i >= 0; i--) {
		*pp++ = SLD.minTickPos - ((range*(i*SLD.ticks))/urange);
	    }
	else 
	    for (i=0; i < SLD.numticks; i++) {
		*pp++ = SLD.minTickPos + ((range*(i*SLD.ticks))/urange);
	    }
}

static void
SLError(SliderWidget sw, int idx, int value)
{
	static char *resources[] = {
		"orientation",
		"sliderMin",
		"sliderMax",
		"sliderValue",
		"granularity",
		"initialDelay",
		"repeatRate",
		"ticks",
		"span",
		"tickUnit",
		"leftMargin",
		"rightMargin",
		"dragCBType",
		"stopPosition",
	};

	char *error;

	if (error = malloc(128)) {
		 snprintf(error, 128, dgettext(OlMsgsDomain,
			"Slider/Gauge \"%1$s\": Bad %2$s value (%3$d), set to default"),
			XtName((Widget)sw), resources[idx], value);
		OlWarning(error);
		free(error);
	}
}


static void
highlight(SliderWidget sw, int invert)
{
        unsigned char save_opcode = SLD.opcode; /* save */
        int flag;


	SLD.opcode &= (~KBD_OP);
	switch(sw->slider.opcode) {
	case ANCHOR_TOP:
		/* highlight anchor */
	        flag = SL_BEGIN_ANCHOR;
		break;
	case ANCHOR_BOT:
		/* highlight anchor */
	        flag = SL_END_ANCHOR;
		break;
	case PAGE_DEC:
	case PAGE_INC:
	default:
		flag = 0;
		/* do nothing */
		break;
	case DRAG_ELEV:
		/* highlight dragbox */
		flag = SL_DRAG;
		break;
	}

	if (!invert)
	    sw->slider.opcode = NOOP;

	if (flag)
	    UpdateSlider(sw, sw->slider.pAttrs, flag);

	SLD.opcode = save_opcode; /* restore */
} /* highlight */

static Boolean
ScrollKeyPress(Widget w,unsigned char opcode)
{
	SliderWidget sw = (SliderWidget)w;

	SLD.opcode = opcode | KBD_OP;
	highlight(sw, True);
	TimerEvent((XtPointer)sw, (XtIntervalId*)NULL);
	highlight(sw, False);
	SLD.opcode = NOOP;
	return(True);
} /* ScrollKeyPress */

static Boolean
SLActivateWidget (Widget w, OlVirtualName activation_type, XtPointer call_data)
{
	SliderWidget sw = (SliderWidget)w;
	Boolean consumed = False;

	if (SLD.orientation == (OlDefine)OL_HORIZONTAL) {
		switch(activation_type) {
		case OL_PAGELEFT:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_INC);
			break;
		case OL_PAGERIGHT:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_DEC);
			break;
		case OL_SCROLLLEFT:
			consumed = ScrollKeyPress(w, (unsigned char)GRAN_INC);
			break;
		case OL_SCROLLRIGHT:
			consumed = ScrollKeyPress(w, (unsigned char)GRAN_DEC);
			break;
		case OL_SCROLLLEFTEDGE:
			consumed = ScrollKeyPress(w, (unsigned char)ANCHOR_TOP);
			break;
		case OL_SCROLLRIGHTEDGE:
			consumed = ScrollKeyPress(w, (unsigned char)ANCHOR_BOT);
			break;
		}
	}
	else { /* OL_VERTICAL */
		switch(activation_type) {
		case OL_PAGEDOWN:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_DEC);
			break;
		case OL_PAGEUP:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_INC);
			break;
		case OL_SCROLLDOWN:
			consumed = ScrollKeyPress(w, (unsigned char)GRAN_DEC);
			break;
		case OL_SCROLLUP:
			consumed = ScrollKeyPress(w, (unsigned char)GRAN_INC);
			break;
		case OL_SCROLLTOP:
			consumed = ScrollKeyPress(w, (unsigned char)ANCHOR_TOP);
			break;
		case OL_SCROLLBOTTOM:
			consumed = ScrollKeyPress(w, (unsigned char)ANCHOR_BOT);
			break;
		}
	}
	UpdateSuperCaret(sw);
	return(consumed);
} /* SLActivateWidget */

static void
SLButtonHandler(Widget w, OlVirtualEvent ve)
{
	SliderWidget sw = (SliderWidget)w;
	
	switch(ve->virtual_name) {
	case OL_SELECT:
		ve->consumed = True;
		if (ve->xevent->type == ButtonPress) {
			sw->slider.moving = TRUE;
			UpdateSuperCaret(sw);
			SelectDown(w, ve->xevent, NULL, NULL);
		} else {
			SelectUp(w, ve->xevent, NULL, NULL);
                        sw->slider.moving = FALSE; 
                        UpdateSuperCaret(sw); 
		}
	}
} /* SLButtonHandler */
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
	_OlAddOlDefineType ("horizontal",  OL_HORIZONTAL);
	_OlAddOlDefineType ("vertical",    OL_VERTICAL);
	_OlAddOlDefineType ("percent",     OL_PERCENT);
	_OlAddOlDefineType ("slidervalue", OL_SLIDERVALUE);
	_OlAddOlDefineType ("continuous",  OL_CONTINUOUS);
	_OlAddOlDefineType ("granularity", OL_GRANULARITY);
	_OlAddOlDefineType ("tickmark",    OL_TICKMARK);
	_OlAddOlDefineType ("release",     OL_RELEASE);
	_OlAddOlDefineType ("all",         OL_ALL);
	_OlAddOlDefineType ("none",        OL_NONE);
}

/*
 *************************************************************************
 *  Initialize
 ****************************procedure*header*****************************
 */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
               		     /*	what the client	asked for */
           		     /*	what we're going to give him */
             
                   
{
	SliderWidget sw	= (SliderWidget) new;
	Dimension width;
	Dimension height;
	Dimension elevWidth, elevHeight;
	Dimension anchorWidth, anchorHeight;
	OlStr temp;
	OlStrRep tf = sw->primitive.text_format;
	OlStr null_string = (*str_methods[tf].StrEmptyString)();
	

	sw->slider.labelGC  = NULL;
	sw->slider.ticklist = NULL;
	sw->slider.opcode   = NOOP;
	sw->slider.pAttrs   = (OlgxAttrs *) NULL;
	sw->slider.moving   = FALSE;

	/* check for valid values */
	CheckValues(sw);

	sw->slider.timerid   = (XtIntervalId)NULL;

	/* replicate label strings */
	if (sw->slider.minLabel) {
		if(!((*str_methods[tf].StrCmp)(sw->slider.minLabel,
						null_string)) ) 
			sw->slider.minLabel = (OlStr)NULL; 
					/* empty string */
		temp = (OlStr)XtMalloc(
				(*str_methods[tf].StrNumBytes)
					(sw->slider.minLabel));
		(*str_methods[tf].StrCpy)(temp,
						sw->slider.minLabel);
		sw->slider.minLabel = temp;
	}
	if (sw->slider.maxLabel) {
		if(!((*str_methods[tf].StrCmp)(sw->slider.maxLabel,
						null_string)) ) 
			sw->slider.maxLabel = (OlStr)NULL; 
					/* empty string */
		temp = (OlStr)XtMalloc(
				(*str_methods[tf].StrNumBytes)
					(sw->slider.maxLabel));
		(*str_methods[tf].StrCpy)(temp,
						sw->slider.maxLabel);
		sw->slider.maxLabel = temp;
	}

	GetGCs(sw);

	/* calculate dimensions of anchors and elevator */
	if (sw->slider.endBoxes)
            SizeSliderAnchor(sw, sw->slider.pAttrs, 
			&anchorWidth, &anchorHeight);
	else
	    anchorWidth = anchorHeight = 0;

        sw->slider.anchwidth = anchorWidth;
        sw->slider.anchlen = anchorHeight;
 
	if (HORIZ(sw)) {
             sw->slider.elevwidth = 
		   (Dimension)HorizSliderControl_Width(sw->slider.pAttrs->ginfo);
             sw->slider.elevheight = 
		   (Dimension)HorizSliderControl_Height(sw->slider.pAttrs->ginfo);
	} else {
	     sw->slider.elevwidth = 
		   (Dimension)HorizSliderControl_Height(sw->slider.pAttrs->ginfo);
	     sw->slider.elevheight = 
		   (Dimension)HorizSliderControl_Width(sw->slider.pAttrs->ginfo);
	}

	SLD.leftPad = calc_leftMargin(sw);	
	SLD.rightPad = calc_rightMargin(sw);

	if ((sw->core.height == 0) || (sw->core.width == 0)) {
		width = sw->core.width;
		height = sw->core.height;
		calc_dimension(sw, &width, &height);
		if (sw->core.height == 0)
			sw->core.height	= height;
		if (sw->core.width == 0)
			sw->core.width = width;
	}


	/* override Core widget	and set	borderWidth to 0 and inherit
	 * parent's background.
	 */
	sw->core.border_width =	0;
	sw->core.background_pixmap = ParentRelative;
}   /* Initialize */

static void
Destroy(Widget w)
{
	SliderWidget sw = (SliderWidget)w;

	XtDestroyGC(sw->slider.labelGC);
	if (sw->slider.pAttrs)
	    OlgxDestroyAttrs (w, sw->slider.pAttrs);

	if (sw->slider.minLabel)
		XtFree(sw->slider.minLabel);
	if (sw->slider.maxLabel)
		XtFree(sw->slider.maxLabel);
	if (sw->slider.ticklist)
		XtFree((char*)sw->slider.ticklist);

	if (sw->slider.timerid)	{
		XtRemoveTimeOut(sw->slider.timerid);
		sw->slider.timerid = (XtIntervalId)NULL;
	}
	if (IS_SLIDER(sw))
        	XtRemoveAllCallbacks((Widget)sw, XtNsliderMoved);

} /* Destroy */

/*
 * Redisplay
 */
static void
Redisplay(Widget w, XEvent *event, Region region)
{
	SliderWidget sw = (SliderWidget)w;
	

	if (!event || !event->xexpose.count)
	    DrawSlider(sw, sw->slider.pAttrs);
}

/*
 *************************************************************************
 * Resize - Reconfigures all the subwidgets to a new size.
 *	    Must also recalulate size and position of indicator.
 ****************************procedure*header*****************************
 */
static void
Resize(Widget w)
{
	SliderWidget sw = (SliderWidget)w;

	Recalc(sw);

} /* Resize */

/*
 *************************************************************************
 *  Realize - Creates the WIndow
 ****************************procedure*header*****************************
 */
static void
Realize(Widget w, Mask *valueMask, XSetWindowAttributes *attributes)
{
	SliderWidget sw = (SliderWidget)w;

	XtCreateWindow((Widget)sw, InputOutput,	(Visual	*)CopyFromParent,
	    *valueMask,	attributes );

	XDefineCursor(XtDisplay(sw), XtWindow(sw),
		      OlGetStandardCursor((Widget)sw));

	Recalc(sw);

} /* Realize */

/*
 *************************************************************************
 * GetValuesHook - get resource data. If asking for leftMargin or
 *		   rightMargin and they happen to be OL_IGNORE, then
 *		   return actual left padding and right padding,
 *		   respectively.
 *************************************************************************
 */
static void
GetValuesHook(Widget w, ArgList args, Cardinal *num_args)
{
	SliderWidget sw = (SliderWidget)w;
	int n = 0;
	MaskArg mask_args[6];

	if (*num_args != 0) {
		Dimension pad;
		OlDefine save;
		
		save = SLD.leftMargin;
		SLD.leftMargin = (Dimension)OL_IGNORE;
		pad = calc_leftMargin(sw);
		SLD.leftMargin = save;
		_OlSetMaskArg(mask_args[n], XtNleftMargin, 
				pad, OL_COPY_MASK_VALUE); n++;
		_OlSetMaskArg(mask_args[n], NULL, sizeof(Dimension),
				OL_COPY_SIZE); n++;

		save = SLD.rightMargin;
		SLD.rightMargin = (Dimension)OL_IGNORE;
		pad = calc_rightMargin((SliderWidget)w);
		SLD.rightMargin = save;
		_OlSetMaskArg(mask_args[n], XtNrightMargin, 
				pad, OL_COPY_MASK_VALUE); n++;
		_OlSetMaskArg(mask_args[n], NULL, sizeof(Dimension),
				OL_COPY_SIZE); n++;

		if (SLD.span == (Dimension)OL_IGNORE) {
			if (VERT(sw))
				_OlSetMaskArg(mask_args[n], XtNspan, 
					sw->core.height, OL_COPY_MASK_VALUE);
			else
				_OlSetMaskArg(mask_args[n], XtNspan, 
					sw->core.width - SLD.leftPad -
					SLD.rightPad, OL_COPY_MASK_VALUE);
			n++;
			_OlSetMaskArg(mask_args[n], NULL, sizeof(Dimension),
					OL_COPY_SIZE); n++;
		}

		if (n > 0)
			_OlComposeArgList(args, *num_args, mask_args, n,
					 NULL, NULL);
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
	if(_OlInputFocusFeedback(w) == OL_SUPERCARET)
		return;
	GetGCs((SliderWidget)w);
	Redisplay(w, (XEvent*)NULL, (Region)NULL);
} /* END OF HighlightHandler() */

/*
 *************************************************************************
 * QueryGeom - Important routine for parents that want to know how
 *		large the slider wants to be. the thickness  (width
 *		for ver. slider) and length (height for	ver. slider)
 *		should be honored by the parent, otherwise an ugly
 *		visual will appear.
 ****************************procedure*header*****************************
 */
static XtGeometryResult
QueryGeom(Widget w, XtWidgetGeometry *intended, XtWidgetGeometry *reply)
         
                            /* parent's changes; may be NULL */
                            /* child's preferred geometry; never NULL */
{
	SliderWidget sw = (SliderWidget)w;
	XtGeometryResult result;

	/* assume ok */
	result = XtGeometryYes;
	*reply = *intended;

	/* border width	has to be zero */
	if (intended->request_mode & CWBorderWidth &&
	    intended->border_width != 0)
	{
		reply->border_width = 0;
		result = XtGeometryAlmost;
	}

	/* X, Y, Sibling, and StackMode	are always ok */

	calc_dimension(sw, &reply->width, &reply->height);
	if ((intended->request_mode & CWHeight)	&&
	    (reply->height != intended->height)) {
		result = XtGeometryAlmost;
	}

	if ((intended->request_mode & CWWidth) &&
	    (reply->width != intended->width)) {
		result = XtGeometryAlmost;
	}

	return (result);
}	/* QueryGeom */

/*
 ************************************************************
 *
 *  SetValues -	This function compares the requested values
 *	to the current values, and sets	them in	the new
 *	widget.	 It returns TRUE when the widget must be
 *	redisplayed.
 *
 *********************procedure*header************************
 */
static Boolean
SetValues (Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	SliderWidget sw	= (SliderWidget) current;
	SliderWidget newsw = (SliderWidget) new;
	int moved = 0;
	int resize = 0;
	int recalc = 0;
	int new_location;
	Boolean	needs_redisplay	= FALSE;
	OlStrRep tf = sw->primitive.text_format;
	OlStr null_string = (*str_methods[tf].StrEmptyString)();
	OlStr temp;

	/* cannot change orientation on	the fly	*/
	newsw->slider.orientation = sw->slider.orientation;

	CheckValues(newsw);

	if (XtIsSensitive(new) != XtIsSensitive(current))
		needs_redisplay = True;

        if ((newsw->slider.sliderMin != sw->slider.sliderMin) ||
	    (newsw->slider.sliderMax != sw->slider.sliderMax)) {
		recalc = 1;
		moved = 1;
	}
	else if (newsw->slider.sliderValue != sw->slider.sliderValue) {
		moved = 1;
	}

	if ((newsw->slider.leftMargin != sw->slider.leftMargin) ||
	    (newsw->slider.rightMargin != sw->slider.rightMargin)) {
		recalc = 1;
		resize = 1;
	}

	if (newsw->slider.recompute_size != sw->slider.recompute_size)
		resize = 1;

	if ((newsw->slider.recompute_size == True) &&
	    (newsw->slider.span != sw->slider.span)) {
		recalc = 1;
		resize = 1;
	}


	if (newsw->slider.minLabel != sw->slider.minLabel) {
		if (newsw->slider.minLabel) 
			if(!((*str_methods[tf].StrCmp)
				(newsw->slider.minLabel, null_string)) ) 
					/* empty string */
				newsw->slider.minLabel = (OlStr)NULL; 
			else {
				temp = (OlStr)XtMalloc((*str_methods[tf].StrNumBytes)
					(newsw->slider.minLabel));
				(*str_methods[tf].StrCpy)(temp,newsw->slider.minLabel);
				newsw->slider.minLabel = temp;
			}
		if (((newsw->slider.minLabel != NULL) ^
		    (sw->slider.minLabel != NULL)) ||
		    (newsw->slider.minLabel && sw->slider.minLabel &&
		     (*str_methods[tf].StrCmp)
			(newsw->slider.minLabel, SLD.minLabel))) {
			resize = 1;
			recalc = 1;
		}
		if (sw->slider.minLabel)  
			XtFree(sw->slider.minLabel);
	}

	if (newsw->slider.maxLabel != sw->slider.maxLabel) {
		if (newsw->slider.maxLabel) 
			if(!((*str_methods[tf].StrCmp)
				(newsw->slider.maxLabel, null_string)) ) 
					/* empty string */
				newsw->slider.maxLabel = (OlStr)NULL; 
			else {
				temp = (OlStr)XtMalloc((*str_methods[tf].StrNumBytes)
					(newsw->slider.maxLabel));
				(*str_methods[tf].StrCpy)(temp,newsw->slider.maxLabel);
				newsw->slider.maxLabel = temp;
			}
		if (((newsw->slider.maxLabel != NULL) ^
		    (sw->slider.maxLabel != NULL)) ||
		    (newsw->slider.maxLabel && sw->slider.maxLabel &&
		     (*str_methods[tf].StrCmp)
			(newsw->slider.maxLabel, SLD.maxLabel))) {
			resize = 1;
			recalc = 1;
		}
		if (sw->slider.maxLabel) 
			XtFree(sw->slider.maxLabel);
	}

	if (newsw->slider.ticks != sw->slider.ticks) {
		recalc = 1;
		if ((newsw->slider.tickUnit != OL_NONE) &&
		    (sw->slider.tickUnit != OL_NONE)) {
			needs_redisplay = TRUE;
			resize = 1;
		}
	}

	if (newsw->slider.tickUnit != sw->slider.tickUnit) {
		recalc = 1;
		resize = 1;
	}

	if ((newsw->primitive.font != sw->primitive.font) ||
	    (newsw->primitive.scale != sw->primitive.scale)) {
		GetGCs(newsw);
		resize = 1;
		recalc = 1;
	}
	else {
		if (newsw->slider.endBoxes != sw->slider.endBoxes)
			recalc = 1;

		if (newsw->primitive.font_color != sw->primitive.font_color) {
			GetGCs(newsw);
			needs_redisplay = TRUE;
		}
	}

	if (resize) {
		if (newsw->slider.recompute_size &&
		      (newsw->core.width  == sw->core.width) &&
		      (newsw->core.height == sw->core.height)) {
		/* figure out new size and make a resize request */
		Dimension request_width;
		Dimension request_height;

		request_width = newsw->core.width;
		request_height = newsw->core.height;
		calc_dimension(newsw, &request_width, &request_height);
		if ((request_width  != newsw->core.width) ||
		    (request_height != newsw->core.height)) {
			newsw->core.width = request_width;
			newsw->core.height = request_height;
			recalc = 0;
			needs_redisplay = FALSE;
		}
	} /* recompute_size */
	} /* resize */

	if (recalc) {
		Recalc(newsw);
		needs_redisplay = TRUE;
	}


	if ((newsw->primitive.foreground != sw->primitive.foreground) ||
	    (newsw->core.background_pixel != sw->core.background_pixel)) {
	       GetGCs(newsw);
	       needs_redisplay = TRUE;
       }

	/* The slider should always inherit its parents background for
	 * the window.  If anyone has played with this, set it back.
	 */
	if (XtIsRealized((Widget)sw) &&
	    newsw->core.background_pixmap != ParentRelative)
	{
	    newsw->core.background_pixmap = ParentRelative;
	    XSetWindowBackgroundPixmap (XtDisplay (sw), XtWindow (sw),
					ParentRelative);
	    needs_redisplay = TRUE;
	}

	/*
	 * Call the XtNsliderMoved callback if
	 * the widget is a slider,
	 * useSetValCallback is True,
	 * the slider's value has changed,
	 * and the widget is realized
	 */
	if (IS_SLIDER(current) &&
		(sw->slider.useSetValCallback == TRUE) &&
		(newsw->slider.sliderValue != sw->slider.sliderValue) &&
		XtIsRealized((Widget)sw)) {
		new_location = newsw->slider.sliderValue;
		newsw->slider.sliderValue = sw->slider.sliderValue;
		MoveSlider(newsw, new_location, TRUE, FALSE);
	} else if (moved && (needs_redisplay == FALSE) && 
			XtIsRealized((Widget)sw)) {
		new_location = newsw->slider.sliderValue;
		newsw->slider.sliderValue = sw->slider.sliderValue;
		MoveSlider(newsw, new_location, FALSE, FALSE);
	}

	return (needs_redisplay);
}	/* SetValues */

/* query_sc_locn_proc	*/  
void
SliderQuerySCLocnProc(const Widget		target,
			const Widget		supercaret,
			const Dimension		sc_width,
			const Dimension		sc_height,
			unsigned int    *const	scale,	
			SuperCaretShape *const 	shape,
			Position        *const	x_center_return,
			Position        *const	y_center_return)
{
	SliderWidget sw	= (SliderWidget)target;
	SuperCaretShape rs = *shape;

	if(sw->slider.moving == TRUE) {
		*shape = SuperCaretNone;
		return;
	}

	*x_center_return = (Position)0;
	*y_center_return = (Position)0;

	*shape = (sw->slider.orientation == OL_VERTICAL ? 
				SuperCaretLeft:
				SuperCaretBottom);

        if (sw->primitive.scale != *scale || rs != *shape) {
                *scale = sw->primitive.scale;
                return; /* try again */
        }

	switch(*shape)	{
		case SuperCaretLeft:
			*x_center_return += (sw->slider.elev_offset);
			*y_center_return += sw->slider.sliderPValue +
				(Dimension)sw->slider.elevheight/(Dimension)2;
			break;
		case SuperCaretBottom:
			*x_center_return += sw->slider.sliderPValue + 
				(Dimension)sw->slider.elevwidth/(Dimension)2;
			*y_center_return += (sw->slider.elev_offset +
					sw->slider.elevheight + 
					sc_height/(Dimension)6); 
			break;
	}

		
} /*query_sc_locn_proc */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * SelectDown -	Callback for Btn Select	Down inside Slider window, but
 *		not in any children widgets.
 ****************************procedure*header*****************************
 */
static void
SelectDown(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	SliderWidget sw = (SliderWidget)w;
        int point;
	int bottomAnchorPos, topAnchorPos, elevEndPos;
        unsigned char opcode = NOOP;

	/* discard events not over the slider proper */
	if (VERT(sw)) {
		if (event->xbutton.x < sw->slider.elev_offset ||
		    event->xbutton.x >= (Position) (sw->slider.elev_offset +
						    sw->slider.elevwidth))
		    return;

        	point = event->xbutton.y;
		topAnchorPos = sw->slider.anchlen;
		bottomAnchorPos = sw->core.height - sw->slider.anchlen;
		elevEndPos = sw->slider.elevheight;
	}
	else {
		if (event->xbutton.y < sw->slider.elev_offset ||
		    event->xbutton.y >= (Position) (sw->slider.elev_offset +
						    sw->slider.elevheight))
		    return;

        	point = event->xbutton.x;
		topAnchorPos = sw->slider.leftPad + sw->slider.anchwidth;
		bottomAnchorPos = sw->core.width - sw->slider.rightPad -
		    sw->slider.anchwidth;
		elevEndPos = sw->slider.elevwidth;

		if (point < (Position) SLD.leftPad ||
		    point >= (Position) (sw->core.width - SLD.rightPad))
		    return;
	}

	if (sw->slider.endBoxes == True) {
       		if (point < (int) topAnchorPos)
	       		opcode = ANCHOR_TOP;
       		else if (point >= (int) bottomAnchorPos)
	       		opcode = ANCHOR_BOT;
	}

   	/* all subsequent checks are relative to elevator */
	point -= sw->slider.sliderPValue;

	if (opcode == NOOP) {
	       if (point < 0)
		       opcode =	PAGE_INC;
	       else if (point >= elevEndPos)
		       opcode =	PAGE_DEC;
		else
		       opcode =	DRAG_ELEV;
       }

	sw->slider.opcode = opcode;
	highlight(sw,TRUE);

       if (opcode == DRAG_ELEV)	{
		if (sw->slider.type != SB_REGULAR)
			/* if not regular slider, cannot drag */
			return;
		else {
			/* record pointer based	pos. for dragging */
			sw->slider.dragbase = SLD.sliderPValue -
			    (HORIZ(sw) ?
			     event->xbutton.x : event->xbutton.y) -
				 topAnchorPos;
		}
	}

	/* Use Timer for repetetive motions - for "drag" & "page"
	 * operations. BTW, the "page" operation is supposed to
	 * be non-repetetive as per OL spec ?? -if so, include it
	 * too, in below conditional -JMK
	 */
	if ((opcode != ANCHOR_TOP) && (opcode != ANCHOR_BOT))
		TimerEvent((XtPointer)sw, (XtIntervalId*)NULL);
}	/* SelectDown */

/*
 *************************************************************************
 * SelectUp - Callback when Select Button is released.
 ****************************procedure*header*****************************
 */
static void
SelectUp(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	SliderWidget sw = (SliderWidget)w;
	int nearval;

	/* unhighlight */
	highlight(sw,FALSE);

	if (sw->slider.timerid)	{
		XtRemoveTimeOut(sw->slider.timerid);
		sw->slider.timerid = (XtIntervalId)NULL;
	}

	if ((sw->slider.opcode == ANCHOR_TOP) ||
	    (sw->slider.opcode == ANCHOR_BOT)) {
		 int new_location;
		 int x = event->xbutton.x; 
		 int y = event->xbutton.y; 

		 new_location = (SLD.opcode & DIR_INC) ?
				 (HORIZ(sw) ? SLD.sliderMax : SLD.sliderMin) :
				 (HORIZ(sw) ? SLD.sliderMin : SLD.sliderMax);
		if ((new_location != sw->slider.sliderValue) &&
		    (inAnchor(sw,x,y,sw->slider.opcode))) {
			 MoveSlider(sw, new_location, True, False);
		}
	}

	if (sw->slider.opcode == DRAG_ELEV) {
		sw->slider.opcode = NOOP;
		nearval = calc_stoppos(sw, SLD.sliderValue);
		MoveSlider(sw, nearval, TRUE, FALSE);
	}
	else
		sw->slider.opcode = NOOP;
}	/* SelectUp */

static void
SLKeyHandler (Widget w, OlVirtualEvent ve)
{
	typedef struct {
		OlVirtualName	orig_name;
		OlVirtualName	new_name;
		OlDefine	orientation;
		Boolean		get_count;
	} RemapCmd;
	static const RemapCmd	remapCmds[] = {
		{ OL_MOVERIGHT,	OL_SCROLLRIGHT,	OL_HORIZONTAL,	False	},
		{ OL_MULTIRIGHT,OL_SCROLLRIGHT,	OL_HORIZONTAL,	True	},
		{ OL_MOVELEFT,	OL_SCROLLLEFT,	OL_HORIZONTAL,	False	},
		{ OL_MULTILEFT,	OL_SCROLLLEFT,	OL_HORIZONTAL,	True	},
		{ OL_MOVEUP,	OL_SCROLLUP,	OL_VERTICAL,	False	},
		{ OL_MULTIUP,	OL_SCROLLUP,	OL_VERTICAL,	True	},
		{ OL_MOVEDOWN,	OL_SCROLLDOWN,	OL_VERTICAL,	False	},
		{ OL_MULTIDOWN,	OL_SCROLLDOWN,	OL_VERTICAL,	True	}
	};
	const RemapCmd *	cmds = remapCmds;
	Cardinal		i;

	for (i=0; i < XtNumber(remapCmds); ++cmds, ++i)
	{
		if (ve->virtual_name == cmds->orig_name &&
		    ((SliderWidget)w)->slider.orientation == cmds->orientation)
		{
			Cardinal count;

			ve->consumed = True;

			count = (cmds->get_count == True ?
					_OlGetMultiObjectCount(w) : 1);
			while(count--)
			{
				OlActivateWidget(w, cmds->new_name,
							(XtPointer)NULL);
			}
			break;
		}
	}
} /* SLKeyHandler() */

static int
nearest_tickval(SliderWidget sw, int val)
{
	int new_location;

	switch(sw->slider.tickUnit) {
	case OL_PERCENT:
		new_location = SLD.sliderMin + ((val - SLD.sliderMin) * 100 /
				 (SLD.sliderMax - SLD.sliderMin) +
				 SLD.ticks/2) / SLD.ticks * SLD.ticks *
				 (SLD.sliderMax - SLD.sliderMin) / 100;
		break;
	case OL_SLIDERVALUE:
		new_location = SLD.sliderMin + (val - SLD.sliderMin +
				 SLD.ticks/2) / SLD.ticks * SLD.ticks;
		break;
	default:
		new_location = val;
		break;
	}
	return(new_location);
}

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************class*procedures*****************************
 */

void
OlSetGaugeValue(Widget w, int value)
{
	SliderWidget sw = (SliderWidget)w;
	
	if (w == (Widget)NULL) {
		OlWarning(dgettext(OlMsgsDomain,
			"OlSetGaugeValue: NULL gauge widget id"));
		return;
	}

	if (!IS_GAUGE(w)) {
		OlWarning(dgettext(OlMsgsDomain,
			"OlSetGaugeValue: widget id is not a gauge id"));
		return;
	}

	/* check value */
       if (INRANGE(value,SLD.sliderMin,SLD.sliderMax) != value) {
		OlWarning(dgettext(OlMsgsDomain,
			"OlSetGaugeValue: slider value out of range"));
		return;
	}

	MoveSlider(sw, value, FALSE, FALSE);
}


static int
inAnchor(SliderWidget sw, int x, int y, int anchor)
                
         
           	/* ANCHOR_TOP or ANCHOR_BOT ? */
{
	if (VERT(sw)) {
		if (x < sw->slider.elev_offset || 
		    x >= (Position) 
			(sw->slider.elev_offset + sw->slider.elevwidth))
			return False;
		if (y < 0 || y >= (int)(sw->core.height))
			return False;
		if ((anchor == ANCHOR_TOP) && 
		    (y < (int)(sw->slider.anchlen)))
			return True;
		if ((anchor == ANCHOR_BOT) &&
		    (y >= (int)(sw->core.height - sw->slider.anchlen)))
			return True;
		return False;
	}
	else {
		if (y < sw->slider.elev_offset ||
		    y >= (Position)
			(sw->slider.elev_offset + sw->slider.elevheight))
			return False;
		if (x < (Position)(sw->slider.leftPad) ||
		    x >= (Position)(sw->core.width - sw->slider.rightPad))
			return False;
		if ((anchor == ANCHOR_TOP) &&
		    (x < (int)(sw->slider.leftPad + sw->slider.anchwidth)))
			return True;
		if ((anchor == ANCHOR_BOT) &&
		    (x >= (int)(sw->core.width - sw->slider.rightPad -
		    				sw->slider.anchwidth)))
			return True;
		return False;
	}
}
