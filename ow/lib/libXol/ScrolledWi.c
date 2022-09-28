#pragma ident	"@(#)ScrolledWi.c	302.18	97/03/26 lib/libXol SMI"	/* scrollwin:src/ScrolledWi.c 1.51 */

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

/*****************************************************************************
 *                                                                           *
 *              Copyright (c) 1988 by Hewlett-Packard Company                *
 *     Copyright (c) 1988 by the Massachusetts Institute of Technology       *
 *                                                                           *
 *****************************************************************************/

/*************************************************************************
 *
 * Description: This file along with ScrolledWi.h and ScrolledWP.h implements
 *              the OPEN LOOK ScrolledWindow Widget
 *
 *******************************file*header******************************/


#include <Xol/EventObj.h>
#include <Xol/BulletinBP.h>
#include <Xol/OlDnDVCX.h>
#include <Xol/OlI18nP.h>
#include <Xol/OpenLookP.h>
#include <Xol/RootShellP.h>
#include <Xol/ScrollbarP.h>
#include <Xol/ScrolledWi.h>
#include <Xol/ScrolledWP.h>
#include <Xol/Util.h>
#include <Xol/SuperCaret.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <stdio.h>
#include <libintl.h>

typedef struct
   {
   Position     x, y;
   Dimension    width, height;
   Dimension    real_width, real_height;
   Dimension    border_width;
   } _SWGeometryInfo;

static void             ConditionalUpdate(Widget w, _SWGeometryInfo *w_info, Boolean *moved, Boolean *resized);
static void             DetermineViewSize(ScrolledWindowWidget sw, OlSWGeometries *gg, Widget bbc, int being_resized);
static void             LayoutWithoutChild();
static void             LayoutWithChild(ScrolledWindowWidget sw, _SWGeometryInfo sw_info, BulletinBoardWidget bb, _SWGeometryInfo bb_info, ScrollbarWidget vsb, _SWGeometryInfo vsb_info, ScrollbarWidget hsb, _SWGeometryInfo hsb_info, Widget bbc, int being_resized);
static Boolean          LayoutHSB(ScrolledWindowWidget sw, _SWGeometryInfo *sw_info, _SWGeometryInfo *bb_info, _SWGeometryInfo *bbc_info, ScrollbarWidget hsb, _SWGeometryInfo *hsb_info, int being_resized, int *expanded_height_for_HSB);
static Boolean          LayoutVSB(ScrolledWindowWidget sw, _SWGeometryInfo *sw_info, _SWGeometryInfo *bb_info, _SWGeometryInfo *bbc_info, ScrollbarWidget vsb, _SWGeometryInfo *vsb_info, int being_resized, int *expanded_width_for_VSB);

static void             ClassInitialize(void);
static void             Destroy(Widget w);
static XtGeometryResult GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *preferred_return);
static void             Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);
static void             InsertChild(Widget w);
static void             Realize(Widget w, Mask *value_mask, XSetWindowAttributes *attributes);
static void             Resize(Widget w);
static void		Redisplay(Widget w, XEvent *event, Region region);
static Boolean          SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);
static void             GetValuesHook(Widget w, ArgList args, Cardinal *num_args);

static void             ChildResized(Widget w, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch);
static void             ChildDestroyed(Widget w, XtPointer closure, XtPointer call_data);
static void             HSBMoved(Widget w, XtPointer closure, XtPointer call_data);
static void             VSBMoved(Widget w, XtPointer closure, XtPointer call_data);
static Boolean          CheckWidth(ScrolledWindowWidget sw, _SWGeometryInfo *sw_info, _SWGeometryInfo *bb_info, _SWGeometryInfo *bbc_info, ScrollbarWidget hsb, _SWGeometryInfo *hsb_info);
static Boolean          CheckHeight(ScrolledWindowWidget sw, _SWGeometryInfo *sw_info, _SWGeometryInfo *bb_info, _SWGeometryInfo *bbc_info, ScrollbarWidget vsb, _SWGeometryInfo *vsb_info);
static void		TransparentProc (Widget, Pixel, Pixmap);
static void		InheritBackground(Widget w, int offset, XrmValue *value);
static void		GetGC(Widget w, Pixel fg, Pixel bg);


#define SW_BB_BORDER_W(W)   (OlScreenPointToPixel(OL_HORIZONTAL,1,XtScreen(W)))
#define SW_HSB_GAP(W)       (OlScreenPointToPixel(OL_VERTICAL,2,XtScreen(W)))
#define SW_VSB_GAP(W)       (OlScreenPointToPixel(OL_HORIZONTAL,2,XtScreen(W)))
#define SW_MINIMUM_HEIGHT       200
#define SW_MINIMUM_WIDTH        200
#define SW_SB_GRANULARITY       1
#define SW_SB_SLIDER_MIN        0
#define SW_SB_SLIDER_VALUE      0

#define printinfo(N,gi) fprintf(stderr,"%s: %d,%d (%dx%d) %d (%dx%d)\n", \
   N, gi->x, gi->y, gi->width, gi->height, \
   gi->border_width, gi->real_width, gi->real_height)

#define _SWGetGeomInfo(w, gi) \
((void)( \
  (gi).x = (w)->core.x, \
  (gi).y = (w)->core.y, \
  (gi).width = (w)->core.width, \
  (gi).height = (w)->core.height, \
  (gi).border_width = 2 * (w)->core.border_width, \
  (gi).real_height = (w)->core.height + (gi).border_width, \
  (gi).real_width = (w)->core.width + (gi).border_width \
))

#define _SWSetInfoHeight(gi, h) \
((void)( \
  (gi).height = (h), \
  (gi).real_height = (gi).height + (gi).border_width \
))

#define _SWSetInfoRealHeight(gi, h) \
((void)( \
  (gi).real_height = (h), \
  (gi).height = (gi).real_height - (gi).border_width \
))

#define _SWSetInfoWidth(gi, w) \
((void)( \
  (gi).width = (w), \
  (gi).real_width = (gi).width + (gi).border_width \
))

#define _SWSetInfoRealWidth(gi, w) \
((void)( \
  (gi).real_width = (w), \
  (gi).width = (gi).real_width - (gi).border_width \
))

#define BORDER_3D(W)		(1)
#define GG_VBORDER(W, GG)	((W)->scrolled_window.vborder)
#define GG_HBORDER(W, GG)	((W)->scrolled_window.hborder)
#define BB_VBORDER(W)		((W)->scrolled_window.vborder)
#define BB_HBORDER(W)		((W)->scrolled_window.hborder)

#define offset(field)   XtOffset(ScrolledWindowWidget, scrolled_window.field)
static XtResource resources[] = 
{
  {
    XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
    XtOffset(ScrolledWindowWidget, core.background_pixel),
    XtRCallProc, (XtPointer)InheritBackground
  },
  {
    XtNhScrollbar, XtCHScrollbar, XtRWidget, sizeof(Widget),
    offset(hsb), XtRImmediate, (XtPointer)NULL
  },
  {
    XtNvScrollbar, XtCVScrollbar, XtRWidget, sizeof(Widget),
    offset(vsb), XtRImmediate, (XtPointer)NULL
  },
  {
    XtNalignHorizontal, XtCAlignHorizontal, XtROlDefine, sizeof(OlDefine),
    offset(align_horizontal), XtRImmediate, (XtPointer)OL_BOTTOM
  },
  {
    XtNalignVertical, XtCAlignVertical, XtROlDefine, sizeof(OlDefine),
    offset(align_vertical), XtRImmediate, (XtPointer)OL_RIGHT
  },
  {
    XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
    XtOffset(ScrolledWindowWidget, core.border_width), XtRImmediate, (XtPointer)0
  },
  {
    XtNcurrentPage, XtCCurrentPage, XtRInt, sizeof(int),
    offset(current_page), XtRImmediate, (XtPointer)1
  },
  {
    XtNvAutoScroll, XtCVAutoScroll, XtRBoolean, sizeof(Boolean),
    offset(vAutoScroll), XtRImmediate, (XtPointer)TRUE
  },
  {
    XtNhAutoScroll, XtCHAutoScroll, XtRBoolean, sizeof(Boolean),
    offset(hAutoScroll), XtRImmediate, (XtPointer)TRUE
  },
  {
    XtNforceHorizontalSB, XtCForceHorizontalSB, XtRBoolean, sizeof(Boolean),
    offset(force_hsb), XtRImmediate, (XtPointer)FALSE
  },
  {
    XtNforceVerticalSB, XtCForceVerticalSB, XtRBoolean, sizeof(Boolean),
    offset(force_vsb), XtRImmediate, (XtPointer)FALSE
  },
  {
    XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
    offset(foreground), XtRString, XtDefaultForeground
  },
  {
    XtNhStepSize, XtCHStepSize, XtRInt, sizeof(int),
    offset(h_granularity), XtRImmediate, (XtPointer)1
  },
  {
    XtNhInitialDelay, XtCHInitialDelay, XtRInt, sizeof(int),
    offset(h_initial_delay), XtRImmediate, (XtPointer)500
  },
  {
    XtNhRepeatRate, XtCHRepeatRate, XtRInt, sizeof(int),
    offset(h_repeat_rate), XtRImmediate, (XtPointer)100
  },
  {  
    XtNhSliderMoved, XtCHSliderMoved, XtRCallback, sizeof(XtCallbackProc),
    offset(h_slider_moved), XtRCallback, (XtPointer)NULL
  },
  {
    XtNinitialX, XtCInitialX, XtRInt, sizeof(int),
    offset(init_x), XtRImmediate, (XtPointer)0
  },
  {
    XtNinitialY, XtCInitialY, XtRInt, sizeof(int),
    offset(init_y), XtRImmediate, (XtPointer)0
  },
  {
    XtNrecomputeHeight, XtCRecomputeHeight, XtRBoolean,sizeof(Boolean),
    offset(recompute_view_height), XtRImmediate, (XtPointer)TRUE
  },
  {
    XtNrecomputeWidth, XtCRecomputeWidth, XtRBoolean, sizeof(Boolean),
    offset(recompute_view_width), XtRImmediate, (XtPointer)TRUE
  },
  {
    XtNshowPage, XtCShowPage, XtROlDefine, sizeof(OlDefine),
    offset(show_page), XtRImmediate, (XtPointer)OL_NONE
  },
  {
    XtNvStepSize, XtCVStepSize, XtRInt, sizeof(int),
    offset(v_granularity), XtRImmediate, (XtPointer)1
  },
  {
    XtNvInitialDelay, XtCVInitialDelay, XtRInt, sizeof(int),
    offset(v_initial_delay), XtRImmediate, (XtPointer)500
  },
  {
    XtNvRepeatRate, XtCVRepeatRate, XtRInt, sizeof(int),
    offset(v_repeat_rate), XtRImmediate, (XtPointer)100
  },
  {  
    XtNvSliderMoved, XtCVSliderMoved, XtRCallback, sizeof(XtCallbackProc),
    offset(v_slider_moved), XtRCallback, (XtPointer)NULL
  },
  {
    XtNviewHeight, XtCViewHeight, XtRDimension, sizeof(Dimension),
    offset(view_height), XtRImmediate, (XtPointer)0
  },
  {
    XtNviewWidth, XtCViewWidth, XtRDimension, sizeof(Dimension),
    offset(view_width), XtRImmediate, (XtPointer)0
  },
  {
    XtNcomputeGeometries, XtCComputeGeometries, XtRFunction, sizeof(PFV),
    offset(compute_geometries), XtRFunction, (XtPointer)NULL
  }
};
#undef offset

/*************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record**************************/

ScrolledWindowClassRec scrolledWindowClassRec =
{
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass)&managerClassRec,
    /* class_name         */    "ScrolledWindow",
    /* widget_size        */    sizeof(ScrolledWindowRec),
    /* class_initialize   */    ClassInitialize,
    /* class_partinit     */    NULL,
    /* class_inited       */    FALSE,
    /* initialize         */    Initialize,
    /* Init hook          */    NULL,
    /* realize            */    Realize,
    /* actions            */    NULL,
    /* num_actions        */    0,
    /* resources          */    resources,
    /* num_resources      */    XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion    */    TRUE,
    /* compress_exposure  */    TRUE,
    /* compress_enterleave*/    TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    Destroy,
    /* resize             */    Resize,
    /* expose             */    Redisplay,
    /* set_values         */    SetValues,
    /* set values hook    */    NULL,
    /* set values almost  */    XtInheritSetValuesAlmost,
    /* get values hook    */    GetValuesHook,
    /* accept_focus       */    NULL,
    /* Version            */    XtVersion,
    /* PRIVATE cb list    */    NULL,
    /* TM Table           */    NULL,
    /* query_geom         */    NULL,
  },
  {
/* composite_class fields */
    /* geometry_manager   */    (XtGeometryHandler)GeometryManager,
    /* change_managed     */    NULL,
    /* insert_child       */    InsertChild,
    /* delete_child       */    NULL,
    /* extension          */    NULL
  },
  {
    /* constraint_class fields */
    /* resources      */        (XtResourceList)NULL,
    /* num_resources  */        0,
    /* constraint_size*/        0,
    /* initialize     */        (XtInitProc)NULL,
    /* destroy        */        (XtWidgetProc)NULL,
    /* set_values     */        (XtSetValuesFunc)NULL
  },{
    /* manager_class fields   */
    /* highlight_handler  */	NULL,
    /* reserved		  */	NULL,
    /* reserved		  */	NULL,
    /* traversal_handler  */    NULL,
    /* activate		  */    NULL,
    /* event_procs	  */    NULL,
    /* num_event_procs	  */	0,
    /* register_focus	  */    NULL,
    /* reserved		  */	NULL,
    /* version		  */	OlVersion,
    /* extension	  */	NULL,
    /* dyn_data		  */	{ NULL, 0 },
    /* transparent_proc	  */	TransparentProc,
  },{
/* Scrolled Window class - none */     
   /* empty */                  0
  }
};

WidgetClass scrolledWindowWidgetClass = (WidgetClass)&scrolledWindowClassRec;


/*
 * InheritBackground
 *
 */

static void
InheritBackground(Widget w, int offset, XrmValue *value)
{
    static Pixel pixel;

    pixel = XtParent(w)->core.background_pixel;
    value->addr = (caddr_t) &pixel;
}

/*
 * ConditionalUpdate
 *
 */

static void 
ConditionalUpdate(Widget w, _SWGeometryInfo *w_info, Boolean *moved, Boolean *resized)
{
int different_position;
int different_size;

if (w_info-> width <= 0) 
   w_info-> width = 1;
if (w_info-> height <= 0) 
   w_info-> height = 1;

different_position = (w-> core.x != w_info-> x || w-> core.y != w_info-> y);
different_size = (w-> core.width != w_info-> width || 
                  w-> core.height != w_info-> height);

if (different_position && different_size)
   XtConfigureWidget(w, w_info-> x, w_info-> y, 
                        w_info-> width, w_info-> height,
                        w_info-> border_width / 2);
else
   if (different_position)
      XtMoveWidget(w, w_info-> x, w_info-> y);
   else
      if (different_size)
         XtResizeWidget(w, 
                        w_info-> width, w_info-> height,
                        w_info-> border_width / 2);

if (moved   != (Boolean *)NULL) *moved   = (Boolean)different_position;
if (resized != (Boolean *)NULL) *resized = (Boolean)different_size;
} /* end of ConditionalUpdate */
/*
 * DetermineViewSize
 *
 */

static void DetermineViewSize(ScrolledWindowWidget sw, OlSWGeometries *gg, Widget bbc, int being_resized)
{
if ((sw-> scrolled_window.compute_geometries != NULL) && (bbc)) {
   Boolean need_vsb = TRUE;
   (*sw-> scrolled_window.compute_geometries)(bbc, gg);
   sw-> scrolled_window.force_vsb = gg-> force_vsb;
   sw-> scrolled_window.force_hsb = gg-> force_hsb;
   if (sw-> scrolled_window.force_vsb ||
         gg-> sw_view_height < gg-> bbc_real_height)
      gg-> sw_view_width -= gg-> vsb_width;
   else
      need_vsb = FALSE;

   if (sw-> scrolled_window.force_hsb ||
         gg-> sw_view_width < gg-> bbc_real_width)
      {
      gg-> sw_view_height -= gg-> hsb_height;
      if (!need_vsb && gg-> sw_view_height < gg-> bbc_real_height)
         gg-> sw_view_width -= gg-> vsb_width;
      }
}
else {
        Dimension min_width;
        Dimension min_height;

        if (sw->scrolled_window.view_width) {
                if (being_resized && ((int)sw->core.width <
                        (int)(gg->sw_view_width + 2 * GG_HBORDER(sw, gg))))
                        min_width = sw->core.width - 2 * GG_HBORDER(sw, gg);
                else
                        min_width = sw->scrolled_window.view_width;
        }
        else
                min_width = sw->core.width - 2 * GG_HBORDER(sw, gg);

        if (sw->scrolled_window.view_height) {
                if (being_resized && ((int)sw->core.height <
                        (int)(gg->sw_view_height + 2 * GG_VBORDER(sw, gg))))
                        min_height = sw->core.height - 2 * GG_VBORDER(sw, gg);
                else
                        min_height = sw->scrolled_window.view_height;
        }
        else
                min_height = sw->core.height - 2 * GG_VBORDER(sw, gg);

        /* can't be smaller than 1 */
        min_width  = MAX(min_width,  1);
        min_height = MAX(min_height, 1);

        /* recompute view size based on child size */
        if (bbc != (Widget)NULL) {
                if (sw->scrolled_window.recompute_view_width == TRUE)
                        min_width = MIN((Dimension)(gg->bbc_real_width +
				 2 * bbc->core.border_width), min_width);
                if (sw->scrolled_window.recompute_view_height == TRUE)
                        min_height = MIN((Dimension)(gg->bbc_real_height +
				 2 * bbc->core.border_width), min_height);
        }

        gg->sw_view_width  = min_width;
        gg->sw_view_height = min_height;
}
} /* end of DetermineViewSize */

static Boolean
CheckHeight(ScrolledWindowWidget sw, _SWGeometryInfo *sw_info, _SWGeometryInfo *bb_info, _SWGeometryInfo *bbc_info, ScrollbarWidget vsb, _SWGeometryInfo *vsb_info)
{
        XtWidgetGeometry        intended;
        XtWidgetGeometry        preferred;

        if ((sw->scrolled_window.force_vsb == TRUE) ||
            bb_info->height < bbc_info->real_height) {
                intended.request_mode = CWHeight;
                intended.height = bb_info->real_height - vsb_info->border_width;
                switch(XtQueryGeometry((Widget)vsb, &intended, &preferred)) {
                case XtGeometryYes:
                        break;
                case XtGeometryAlmost:
                case XtGeometryNo:
			if (preferred.height < intended.height)
			    preferred.height = intended.height;

                        else if (preferred.height > intended.height) {
                          for (--intended.height;intended.height != 0;
                                --intended.height)
                                if (XtQueryGeometry((Widget)vsb,
						    &intended,&preferred)
                                         == XtGeometryYes
                                    || preferred.height <= intended.height)
                                                break;
                        }
                        break;
                }
		bb_info->real_height = preferred.height + vsb_info->border_width;
		bb_info->height = bb_info->real_height - 2 * BB_VBORDER(sw);
                return TRUE;
        }
        return FALSE;
} /* CheckHeight() */

static Boolean
CheckWidth(ScrolledWindowWidget sw, _SWGeometryInfo *sw_info, _SWGeometryInfo *bb_info, _SWGeometryInfo *bbc_info, ScrollbarWidget hsb, _SWGeometryInfo *hsb_info)
{
        XtWidgetGeometry        intended;
        XtWidgetGeometry        preferred;

        if ((sw->scrolled_window.force_hsb == TRUE) ||
            bb_info->width < bbc_info->real_width) {
                intended.request_mode = CWWidth;
                intended.width = bb_info->real_width - hsb_info->border_width;
                switch(XtQueryGeometry((Widget)hsb, &intended, &preferred)) {
                case XtGeometryYes:
                        break;
                case XtGeometryAlmost:
                case XtGeometryNo:
			if (preferred.width < intended.width)
			    preferred.width = intended.width;

                        else if (preferred.width > intended.width) {
                           for (--intended.width;intended.width != 0;
                                --intended.width)
                                if (XtQueryGeometry((Widget)hsb,
						    &intended,&preferred)
                                         == XtGeometryYes
                                        || preferred.width <= intended.width)
                                                break;
                        }
                        break;
                }
		bb_info->real_width = preferred.width + hsb_info->border_width;
		bb_info->width = bb_info->real_width - 2 * BB_HBORDER(sw);
                return TRUE;
        }
        else
                return FALSE;
} /* CheckWidth() */

static void
TransparentProc (Widget w, Pixel pixel, Pixmap pixmap)
{
	ScrolledWindowWidget sw = (ScrolledWindowWidget)w;

	GetGC(w, sw->core.border_pixel, pixel);
	/* do the default also */
	_OlDefaultTransparentProc(w, pixel, pixmap);
}

#ifdef OLD
/*
 * LayoutWithoutChild
 *
 */

static void LayoutWithoutChild(sw, sw_info, bb, bb_info, vsb, vsb_info, hsb, hsb_info, being_resized)
ScrolledWindowWidget sw;
_SWGeometryInfo      sw_info;
BulletinBoardWidget  bb;
_SWGeometryInfo      bb_info;
ScrollbarWidget      hsb;
_SWGeometryInfo      hsb_info;
ScrollbarWidget      vsb;
_SWGeometryInfo      vsb_info;
int being_resized;
{
Dimension       min_width;
Dimension       min_height;
_SWGeometryInfo bbc_info;
int                  expanded_height_for_HSB = 0;
int                  expanded_width_for_VSB  = 0;

bbc_info.x = 
bbc_info.y = 
bbc_info.width = 
bbc_info.height =
bbc_info.real_width = 
bbc_info.real_height = 
bbc_info.border_width = 0;

min_width = (sw-> scrolled_window.view_width > 0) ?
             sw-> scrolled_window.view_width : 
             sw-> core.width - 2 * BB_HBORDER(sw);

min_height = (sw-> scrolled_window.view_height > 0) ?
              sw-> scrolled_window.view_height :
              sw-> core.height - 2 * BB_HBORDER(sw);

bb_info.width = min_width;
bb_info.real_width = min_width + 2 * BB_HBORDER(sw);
bb_info.height = min_height;
bb_info.real_height = min_height + 2 * BB_VBORDER(sw);

(void)LayoutHSB(sw, &sw_info, &bb_info, &bbc_info, hsb, &hsb_info,
	being_resized, &expanded_height_for_HSB);
if (LayoutVSB(sw, &sw_info, &bb_info, &bbc_info, vsb, &vsb_info,
	being_resized, &expanded_height_for_HSB))
   if (LayoutHSB(sw, &sw_info, &bb_info, &bbc_info, hsb, &hsb_info,
	being_resized, &expanded_height_for_HSB))
      (void)CheckHeight(sw, &sw_info, &bb_info, &bbc_info, vsb, &vsb_info);

if (BORDER_3D(sw))
	bb_info.border_width = 0;
ConditionalUpdate(bb, &bb_info, (Boolean *)NULL, (Boolean *)NULL);
ConditionalUpdate(hsb, &hsb_info, (Boolean *)NULL, (Boolean *)NULL);
ConditionalUpdate(vsb, &vsb_info, (Boolean *)NULL, (Boolean *)NULL);

XtSetSensitive((Widget)hsb, FALSE);
XtSetSensitive((Widget)vsb, FALSE);

} /* end of LayoutWithoutChild */
#endif
/*
 * LayoutWithChild
 *
 */

static void LayoutWithChild(ScrolledWindowWidget sw, _SWGeometryInfo sw_info, BulletinBoardWidget bb, _SWGeometryInfo bb_info, ScrollbarWidget vsb, _SWGeometryInfo vsb_info, ScrollbarWidget hsb, _SWGeometryInfo hsb_info, Widget bbc, int being_resized)
{
_SWGeometryInfo      bbc_info;
Arg                  arg[10];
Position             gap;
OlSWGeometries       geometries;
int                  expanded_height_for_HSB = 0;
int                  expanded_width_for_VSB  = 0;
int		     n;
Boolean save_hsb = sw-> scrolled_window.force_hsb;
Boolean save_vsb = sw-> scrolled_window.force_vsb;

Boolean child_moved   = False;
Boolean child_resized = False;

Dimension           pref_height;
Dimension           pref_width;

if (bbc)
	_SWGetGeomInfo(bbc, bbc_info);
else
	memset((char *)&bbc_info, 0, sizeof(bbc_info));
geometries = GetOlSWGeometries(sw);
DetermineViewSize(sw, &geometries, bbc, being_resized);

bb_info.width = geometries.sw_view_width;
bb_info.real_width = geometries.sw_view_width + 2 * BB_HBORDER(sw);
bb_info.height = geometries.sw_view_height;
bb_info.real_height = geometries.sw_view_height + 2 * BB_VBORDER(sw);
_SWSetInfoWidth(bbc_info, geometries.bbc_width);
_SWSetInfoHeight(bbc_info, geometries.bbc_height);
  
if (!being_resized) {
        _SWSetInfoWidth(sw_info, bb_info.real_width);
        _SWSetInfoHeight(sw_info, bb_info.real_height);
}

(void)LayoutHSB(sw, &sw_info, &bb_info, &bbc_info, hsb, &hsb_info,
        being_resized, &expanded_height_for_HSB);
if (LayoutVSB(sw, &sw_info, &bb_info, &bbc_info, vsb, &vsb_info,
        being_resized, &expanded_width_for_VSB))
   if (LayoutHSB(sw, &sw_info, &bb_info, &bbc_info, hsb, &hsb_info,
        being_resized, &expanded_height_for_HSB))
      (void)CheckHeight(sw, &sw_info, &bb_info, &bbc_info, vsb, &vsb_info);
  
if (!being_resized && ((bb_info.width > sw->core.width) ||
      (bb_info.height > sw->core.height) ||
      (sw_info.width != sw->core.width) ||
      (sw_info.height != sw->core.height))) {
          int ret;
          if ((ret = XtMakeResizeRequest((Widget)sw, sw_info.width,
                  sw_info.height, &pref_width,
                  &pref_height)) == XtGeometryAlmost) {
                  _SWSetInfoWidth(sw_info, pref_width);
                  _SWSetInfoHeight(sw_info, pref_height);
                  (void)XtMakeResizeRequest((Widget)sw,
                          sw_info.width, sw_info.height,
                          (Dimension *)NULL, (Dimension *)NULL);
          }

          if (ret == XtGeometryNo) {
                  /*
                   * Resize request rejected!!!
                   * If resize request was due to scrollbars,
                   * then shrink the bulletin board size to allocate
                   * space for scrollbars. Also don't forget to reset
                   * size of SW to original.
                   */
                  _SWGetGeomInfo((Widget)sw, sw_info);
                  if (expanded_height_for_HSB) {
                          Dimension needed_space;
                          Dimension extra_space;
                          Dimension diff;

                          needed_space = hsb_info.real_height + SW_HSB_GAP(sw);
                          extra_space  = sw_info.height - bb_info.real_height;
                          diff         = needed_space - extra_space;
			  bb_info.height = bb_info.height - diff;
			  bb_info.real_height = bb_info.height + 2 * BB_VBORDER(sw);
                  }
                  if (expanded_width_for_VSB) {
                          Dimension needed_space;
                          Dimension extra_space;
                          Dimension diff;

                          needed_space = vsb_info.real_width + SW_VSB_GAP(sw);
                          extra_space  = sw_info.height - bb_info.real_height;
                          diff         = needed_space - extra_space;
			  bb_info.width = bb_info.width - diff;
			  bb_info.real_width = bb_info.width + 2 * BB_HBORDER(sw);
                  }
          }
  }

  if (being_resized)
          _SWGetGeomInfo((Widget)sw, sw_info);

if ((gap = bb_info.width - (bbc_info.x + bbc_info.real_width)) > 0)
   bbc_info.x = MIN(bbc_info.x + gap, 0);
if ((gap = bb_info.height - (bbc_info.y + bbc_info.real_height)) > 0)
   bbc_info.y = MIN(bbc_info.y + gap, 0);

/* adjust positions for alignment */
if (sw->scrolled_window.align_horizontal == OL_BOTTOM) {
        hsb_info.y = bb_info.real_height + SW_HSB_GAP(sw);
	bb_info.y = 0;
}
else {
        hsb_info.y = 0;
        bb_info.y = hsb_info.real_height + SW_HSB_GAP(sw);
}

if (sw->scrolled_window.align_vertical == OL_RIGHT) {
	bb_info.x = 0;
        vsb_info.x = bb_info.real_width + SW_VSB_GAP(sw);
}
else {
        vsb_info.x = 0;
        bb_info.x = vsb_info.real_width + SW_VSB_GAP(sw);
}

vsb_info.y = bb_info.y;
_SWSetInfoRealHeight(vsb_info, bb_info.real_height);

hsb_info.x = bb_info.x;
_SWSetInfoRealWidth(hsb_info, bb_info.real_width);

if (BORDER_3D(sw)) {
	bb_info.x += sw->scrolled_window.hborder;
	bb_info.y += sw->scrolled_window.vborder;
}

ConditionalUpdate((Widget)bb, &bb_info, (Boolean *)NULL, (Boolean *)NULL);
if (bbc) {
	_OlDnDSetDisableDSClipping(bbc, True);
	ConditionalUpdate(bbc, &bbc_info, &child_moved, &child_resized);
	_OlDnDSetDisableDSClipping(bbc, False);
	if (child_moved || child_resized)
		OlDnDWidgetConfiguredInHier(bbc);
}
ConditionalUpdate((Widget)hsb, &hsb_info, (Boolean *)NULL, (Boolean *)NULL);
ConditionalUpdate((Widget)vsb, &vsb_info, (Boolean *)NULL, (Boolean *)NULL);

if (sw-> scrolled_window.hAutoScroll)
   {
   XtSetArg(arg[0], XtNproportionLength, 
        MAX(1, (Dimension)(MIN(bbc_info.real_width, bb_info.width))));
   XtSetArg(arg[1], XtNsliderMax,    MAX(1, bbc_info.real_width));
   XtSetArg(arg[2], XtNsliderValue,  -bbc_info.x);
   XtSetArg(arg[3], XtNgranularity,  MIN((int)(MAX(1, bbc_info.real_width)),
                                sw->scrolled_window.h_granularity));
   XtSetValues((Widget)hsb, arg, 4);
    
   XtSetSensitive((Widget)hsb, (bbc_info.real_width > bb_info.width));
   }
/*
**	Removed as a fix for bug 1255521 - forced scrollbars should not be sensitive.
**	Dave Tong, 16-Oct-96
else
   XtSetSensitive((Widget)hsb, TRUE);
*/
      
if (sw-> scrolled_window.vAutoScroll)
   {
   XtSetArg(arg[0], XtNproportionLength, 
        MAX(1, (Dimension)(MIN(bbc_info.real_height, bb_info.height))));
   XtSetArg(arg[1], XtNsliderMax,     MAX(1, bbc_info.real_height));
   XtSetArg(arg[2], XtNsliderValue,   -bbc_info.y);
   XtSetArg(arg[3], XtNgranularity,   MIN((int)(MAX(1, bbc_info.real_height)),
					  sw->scrolled_window.v_granularity));
   XtSetValues((Widget)vsb, arg, 4);
 
   XtSetSensitive((Widget)vsb, (bbc_info.real_height > bb_info.height));
   }
/*
**	Removed as a fix for bug 1255521 - forced scrollbars should not be sensitive.
**	Dave Tong, 16-Oct-96
else
   XtSetSensitive((Widget)vsb, TRUE);
*/

/* n is used here temporarily */
n = ((bb_info.width < bbc_info.real_width) ||
                 (sw->scrolled_window.force_hsb == TRUE));
if (XtIsRealized((Widget)hsb)) {
        if (n)
                XtMapWidget((Widget)hsb);
        else
                XtUnmapWidget((Widget)hsb);
}
else
        XtSetMappedWhenManaged((Widget)hsb, (n) ? TRUE : FALSE);

n = ((bb_info.height < bbc_info.real_height) ||
                 (sw->scrolled_window.force_vsb == TRUE));
if (XtIsRealized((Widget)vsb)) {
        if (n)
                XtMapWidget((Widget)vsb);
        else
                XtUnmapWidget((Widget)vsb);
}
else 
        XtSetMappedWhenManaged((Widget)vsb, (n) ? TRUE : FALSE);

sw-> scrolled_window.force_hsb = save_hsb;
sw-> scrolled_window.force_vsb = save_vsb;
} /* LayoutWithChild() */
/*
 * OlLayoutScrolledWindow
 *
 * The \fIOlLayoutScrolledWindow\fR procedure is used to programatically
 * force a layout of the components of the scrolled window widget \fIsw\fR.
 *
 * Synopsis:
 *
 *#include <ScrolledWi.h>
 * ...
 */

extern void OlLayoutScrolledWindow(ScrolledWindowWidget sw, int being_resized)
{
_SWGeometryInfo     sw_info;
BulletinBoardWidget bb;
_SWGeometryInfo     bb_info;
ScrollbarWidget     hsb;
_SWGeometryInfo     hsb_info;
ScrollbarWidget     vsb;
_SWGeometryInfo     vsb_info;
Widget              bbc;


bb  = sw-> scrolled_window.bboard;
bbc = sw-> scrolled_window.bb_child;
hsb = sw-> scrolled_window.hsb;
vsb = sw-> scrolled_window.vsb;

_SWGetGeomInfo((Widget)bb, bb_info);
_SWGetGeomInfo((Widget)hsb, hsb_info);
_SWGetGeomInfo((Widget)sw, sw_info);
_SWGetGeomInfo((Widget)vsb, vsb_info);

   LayoutWithChild(sw, sw_info, bb, bb_info, vsb, vsb_info, hsb, 
                hsb_info, bbc, being_resized);

} /* end of OlLayoutScrolledWindow */
/*
 * LayoutHSB
 *
 */

static Boolean LayoutHSB(ScrolledWindowWidget sw, _SWGeometryInfo *sw_info, _SWGeometryInfo *bb_info, _SWGeometryInfo *bbc_info, ScrollbarWidget hsb, _SWGeometryInfo *hsb_info, int being_resized, int *expanded_height_for_HSB)
{
int needed_space;
int extra_space;
int real_extra_space;
int diff;

if (CheckWidth(sw, sw_info, bb_info, bbc_info, hsb, hsb_info))
   {
   needed_space = hsb_info->real_height + SW_HSB_GAP(sw);
   extra_space  = sw_info->height - bb_info->real_height;
   real_extra_space  = sw->core.height - bb_info->real_height;
   diff         = needed_space - extra_space;
   if (diff > 0) {
           if (being_resized ||
               ((sw->scrolled_window.view_height == 0) &&
               (real_extra_space < needed_space))) {
		   bb_info->real_height = bb_info->real_height - (Dimension)diff;
		   bb_info->height = bb_info->real_height - 2 * BB_VBORDER(sw);
                   *expanded_height_for_HSB = 0;
           }
           else {
                   _SWSetInfoHeight(*sw_info, sw_info->height +
                                   (Dimension)diff);
                   *expanded_height_for_HSB = 1;
           }
           return TRUE;
   }
   }
return FALSE;

} /* end of LayoutHSB */
/*
 * LayoutVSB
 *
 */

static Boolean LayoutVSB(ScrolledWindowWidget sw, _SWGeometryInfo *sw_info, _SWGeometryInfo *bb_info, _SWGeometryInfo *bbc_info, ScrollbarWidget vsb, _SWGeometryInfo *vsb_info, int being_resized, int *expanded_width_for_VSB)
{
int needed_space;
int extra_space;
int real_extra_space;
int diff;

if (CheckHeight(sw, sw_info, bb_info, bbc_info, vsb, vsb_info))
   {
   needed_space = vsb_info->real_width + SW_VSB_GAP(sw);
   extra_space  = sw_info->width - bb_info->real_width;
   real_extra_space  = sw->core.width - bb_info->real_width;
   diff         = needed_space - extra_space;
   if (diff > 0) {
           if (being_resized ||
               ((sw->scrolled_window.view_width == 0) &&
                (real_extra_space < needed_space))) {
		   bb_info->real_width = bb_info->real_width - (Dimension)diff;
		   bb_info->width = bb_info->real_width - 2 * BB_HBORDER(sw);
                   *expanded_width_for_VSB = 0;
           }
           else {
                   _SWSetInfoWidth(*sw_info, sw_info->width +
                                   (Dimension)diff);
                   *expanded_width_for_VSB = 1;
           }
           return TRUE;
   }
   }
return FALSE;

} /* end of LayoutVSB */
/*
 * ClassInitialize
 *
 */

static void ClassInitialize(void)
{
ScrolledWindowWidgetClass myclass;
CompositeWidgetClass      superclass;

myclass = (ScrolledWindowWidgetClass)scrolledWindowWidgetClass;
superclass = (CompositeWidgetClass)compositeWidgetClass;

myclass-> composite_class.delete_child =
   superclass-> composite_class.delete_child;
myclass-> composite_class.change_managed =
   superclass-> composite_class.change_managed;

_OlAddOlDefineType ("bottom", OL_BOTTOM);
_OlAddOlDefineType ("top",    OL_TOP);
_OlAddOlDefineType ("right",  OL_RIGHT);
_OlAddOlDefineType ("left",   OL_LEFT);
} /* end of ClassInitialize */
/*
 * Destroy
 *
 */

static void Destroy(Widget w)
{

XtRemoveAllCallbacks(w, XtNhSliderMoved);
XtRemoveAllCallbacks(w, XtNvSliderMoved);
if (((ScrolledWindowWidget)(w))->scrolled_window.gc)
    XtReleaseGC(w, ((ScrolledWindowWidget)(w))->scrolled_window.gc);

} /* end of Destroy */
/*
 * GeometryManager
 *
 */

static XtGeometryResult GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *preferred_return)
{

OlLayoutScrolledWindow((ScrolledWindowWidget)w-> core.parent, 0);

OlWidgetConfigured(w);
return XtGeometryYes;

} /* end of GeometryManager */

/*
 * Initialize
 *
 */

/* ARGSUSED */
static void 
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
BulletinBoardWidget     bb;
ScrolledWindowWidget    nsw = (ScrolledWindowWidget)new;
Arg                     arg[20];
Dimension               view_width  = nsw-> scrolled_window.view_width;
Dimension               view_height = nsw-> scrolled_window.view_height;
Pixel                   save_pixel;

	nsw->scrolled_window.gc = NULL;
	GetGC(new, nsw->core.border_pixel, nsw->core.background_pixel);

	nsw->scrolled_window.vborder = (Dimension)(2 *
		OlgxScreenPointToPixel(OL_VERTICAL, 1, XtScreen(new)));
	nsw->scrolled_window.hborder = (Dimension)(2 *
                OlgxScreenPointToPixel(OL_HORIZONTAL, 1, XtScreen(new)));
	XtSetArg(arg[0], XtNborderWidth, 0);

if (view_width == 0)
   {
   view_width = nsw-> core.width == 0 ? SW_MINIMUM_WIDTH : 
      nsw-> core.width - 2 * nsw->scrolled_window.hborder;
   }

if (view_height == 0)
   {
   view_height = nsw-> core.height == 0 ? SW_MINIMUM_HEIGHT :
      nsw-> core.height - 2 * nsw->scrolled_window.vborder;
   }

if (view_width <= 0)
   view_width = SW_MINIMUM_WIDTH;

if (view_height <= 0)
   view_height = SW_MINIMUM_HEIGHT;

if (nsw-> scrolled_window.init_x > 0)
   {
   OlWarning(dgettext(OlMsgsDomain,
		"ScrolledWindow: XtNinitialX must be <= 0 - using 0."));
   nsw-> scrolled_window.init_x = 0;
   }

if (nsw-> scrolled_window.init_y > 0)
   {
   OlWarning(dgettext(OlMsgsDomain,
   		"ScrolledWindow: XtNinitialY must be <= 0 - using 0."));
   nsw-> scrolled_window.init_y = 0;
   }

/* save the SW's background to pass on to the bulletin board */
save_pixel = nsw-> core.background_pixel;

nsw-> core.border_width      = 0;

XtSetArg(arg[1], XtNborderColor, nsw-> core.border_pixel);
XtSetArg(arg[2], XtNheight,      view_height);
XtSetArg(arg[3], XtNwidth,       view_width);
XtSetArg(arg[4], XtNlayout,      OL_IGNORE);
XtSetArg(arg[5], XtNbackground,  save_pixel);
bb = nsw-> scrolled_window.bboard = (BulletinBoardWidget)XtCreateManagedWidget
   ("BulletinBoard", bulletinBoardWidgetClass, (Widget)nsw, arg, 6);

XtSetArg(arg[0], XtNwidth,        view_width);
XtSetArg(arg[1], XtNinitialDelay, nsw-> scrolled_window.h_initial_delay);
XtSetArg(arg[2], XtNrepeatRate,   nsw-> scrolled_window.h_repeat_rate);
XtSetArg(arg[3], XtNorientation,  OL_HORIZONTAL);
XtSetArg(arg[4], XtNsliderMin,    SW_SB_SLIDER_MIN);
XtSetArg(arg[5], XtNsliderValue,  SW_SB_SLIDER_VALUE);
XtSetArg(arg[6], XtNbackground,  save_pixel);
XtSetArg(arg[7], XtNheight, 0);
nsw-> scrolled_window.hsb = (ScrollbarWidget)XtCreateManagedWidget
   ("HScrollbar", scrollbarWidgetClass, (Widget)nsw, arg, 8);

XtSetArg(arg[0], XtNheight,      view_height);
XtSetArg(arg[1], XtNinitialDelay,nsw-> scrolled_window.v_initial_delay);
XtSetArg(arg[2], XtNrepeatRate,  nsw-> scrolled_window.v_repeat_rate);
XtSetArg(arg[3], XtNorientation, OL_VERTICAL);
XtSetArg(arg[4], XtNsliderMin,   SW_SB_SLIDER_MIN);
XtSetArg(arg[5], XtNsliderValue, SW_SB_SLIDER_VALUE);
XtSetArg(arg[6],XtNshowPage,    nsw-> scrolled_window.show_page);
XtSetArg(arg[7], XtNbackground,  save_pixel);
XtSetArg(arg[8], XtNwidth, 0);
nsw-> scrolled_window.vsb = (ScrollbarWidget)XtCreateWidget
   ("VScrollbar", scrollbarWidgetClass, (Widget)nsw, arg, 9);

OlAssociateWidget(new, (Widget)(nsw->scrolled_window.vsb), TRUE);
OlAssociateWidget(new, (Widget)(nsw->scrolled_window.hsb), TRUE);

XtAddCallback((Widget)nsw-> scrolled_window.vsb, XtNsliderMoved,
	      VSBMoved, NULL);
XtAddCallback((Widget)nsw-> scrolled_window.hsb, XtNsliderMoved,
	      HSBMoved, NULL);

if (nsw-> scrolled_window.force_vsb || nsw-> scrolled_window.force_hsb)
   {
   if (nsw-> scrolled_window.force_vsb && 
       !nsw-> scrolled_window.recompute_view_width)
      view_width  -= (nsw-> scrolled_window.vsb-> core.width + SW_VSB_GAP(new));
   if (nsw-> scrolled_window.force_hsb && 
       !nsw-> scrolled_window.recompute_view_height)
      view_height -= (nsw-> scrolled_window.hsb-> core.height + SW_HSB_GAP(new));

   XtSetArg(arg[0], XtNwidth,  view_width);
   XtSetArg(arg[1], XtNheight, view_height);

   XtSetValues((Widget)nsw-> scrolled_window.bboard, arg, 2);
   XtSetValues((Widget)nsw-> scrolled_window.hsb, arg, 1);
   XtSetValues((Widget)nsw-> scrolled_window.vsb, &arg[1], 1);
   }

nsw-> scrolled_window.bb_child = NULL;

nsw-> core.width = bb-> core.width + 2 * BB_HBORDER(nsw) + 
   ((nsw-> scrolled_window.force_vsb) ?
    (nsw-> scrolled_window.vsb-> core.width + SW_VSB_GAP(new)) : 0);

nsw-> core.height = bb-> core.height + 2 * BB_VBORDER(nsw) + 
   ((nsw-> scrolled_window.force_hsb) ?
    (nsw-> scrolled_window.hsb-> core.height + SW_HSB_GAP(new)) : 0);

/*OlLayoutScrolledWindow(nsw, 0);*/

} /* end of Initialize */
/*
 * InsertChild
 *
 */

static void InsertChild(Widget w)
{
Arg                  arg[2];
CompositeWidgetClass superclass;
ScrolledWindowWidget sw = (ScrolledWindowWidget)w-> core.parent;

superclass = (CompositeWidgetClass)compositeWidgetClass;
    
/* assumption: vsb is the last autochild to be added */
if (sw-> scrolled_window.vsb == NULL)
   {
   (*superclass-> composite_class.insert_child)(w);
   XtManageChild(w);
   }
else
   {
   if (sw-> scrolled_window.bb_child == NULL ||
       sw-> scrolled_window.bb_child-> core.being_destroyed == TRUE)
      {
	if (sw->scrolled_window.bb_child &&
	    sw->scrolled_window.bb_child->core.being_destroyed == TRUE) {
		OlUnassociateWidget((Widget)sw);
	}
			/* Associate the Scrolled window with the
			 * inserted child
			 */
      OlAssociateWidget(w, (Widget)sw, TRUE);

      sw-> scrolled_window.bb_child = w;
      w-> core.parent = (Widget)sw-> scrolled_window.bboard;
      XtAddCallback(w, XtNdestroyCallback, ChildDestroyed, (XtPointer)NULL);
      if (!_OlIsGadget(w))
                XtAddEventHandler(w, StructureNotifyMask, FALSE, ChildResized,
                        (XtPointer)sw);

      /* save child configure info */
      sw-> scrolled_window.child_width  = w-> core.width;
      sw-> scrolled_window.child_height = w-> core.height;
      sw-> scrolled_window.child_bwidth = w-> core.border_width;

      XtSetArg(arg[0], XtNx, sw-> scrolled_window.init_x);
      XtSetArg(arg[1], XtNy, sw-> scrolled_window.init_y);
      XtSetValues(w, arg, 2);

      (*superclass-> composite_class.insert_child)(w);

      if (XtIsRealized((Widget)sw))
      	OlLayoutScrolledWindow(sw, 0);
      }
   else
      OlError(dgettext(OlMsgsDomain,
		"ScrolledWindow: No more than one widget in window\n"));
   }
} /* end of InsertChild */
/*
 * Realize
 *
 */

static void Realize(Widget w, Mask *value_mask, XSetWindowAttributes *attributes)
{
ScrolledWindowWidget sw = (ScrolledWindowWidget)w;

OlLayoutScrolledWindow(sw, 0);

XtCreateWindow((Widget)sw, InputOutput, (Visual *)CopyFromParent,
               *value_mask, attributes);
} /* end of Realize */

/*
 * Resize
 *
 */

static void Resize(Widget w)
{
ScrolledWindowWidget sw  = (ScrolledWindowWidget)w;
  
OlLayoutScrolledWindow(sw, 1);

} /* end of Resize */

static void
Redisplay(Widget w, XEvent *event, Region region)
{
    ScrolledWindowWidget sw  = (ScrolledWindowWidget)w;
    Widget bb = (Widget)(sw->scrolled_window.bboard);
    Window win = XtWindow(w);

    XDrawRectangle(XtDisplay(w), XtWindow(w), 
		sw->scrolled_window.gc,
		bb->core.x - sw->scrolled_window.hborder,
		bb->core.y - sw->scrolled_window.vborder,
		bb->core.width + (2 * sw->scrolled_window.hborder) - 1,
		bb->core.height + (2 * sw->scrolled_window.vborder) - 1);
}

/*
 * GetValuesHook
 *
 */

static void GetValuesHook(Widget w, ArgList args, Cardinal *num_args)
{
ScrolledWindowWidget sw;
BulletinBoardWidget  bb;
MaskArg              mask_args[8];
Arg                  sv_args[1];
Widget *             vpane = NULL;
Widget *             hpane = NULL;

if (*num_args != 0) 
   {
   sw = (ScrolledWindowWidget)w;
   bb = sw-> scrolled_window.bboard;

   _OlSetMaskArg(mask_args[0], XtNviewWidth, bb-> core.width, OL_COPY_MASK_VALUE);
   _OlSetMaskArg(mask_args[1], NULL, sizeof(Dimension), OL_COPY_SIZE);
   _OlSetMaskArg(mask_args[2], XtNviewHeight, bb-> core.height, OL_COPY_MASK_VALUE);
   _OlSetMaskArg(mask_args[3], NULL, sizeof(Dimension), OL_COPY_SIZE);
   _OlSetMaskArg(mask_args[4], XtNvMenuPane, &vpane, OL_COPY_SOURCE_VALUE);
   _OlSetMaskArg(mask_args[5], NULL, sizeof(Widget *), OL_COPY_SIZE);
   _OlSetMaskArg(mask_args[6], XtNhMenuPane, &hpane, OL_COPY_SOURCE_VALUE);
   _OlSetMaskArg(mask_args[7], NULL, sizeof(Widget *), OL_COPY_SIZE);

   _OlComposeArgList(args, *num_args, mask_args, 8, NULL, NULL);

   if (vpane) 
      {
      XtSetArg(sv_args[0], XtNmenuPane, vpane);
      XtGetValues((Widget)sw-> scrolled_window.vsb, sv_args, 1);
      }

   if (hpane) 
      {
      XtSetArg(sv_args[0], XtNmenuPane, hpane);
      XtGetValues((Widget)sw-> scrolled_window.hsb, sv_args, 1);
      }
   }

} /* end of GetValuesHook */
/*
 * SetValues
 *
 */

/* ARGSUSED */
static Boolean SetValues (Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
ScrolledWindowWidget cur_sw      = (ScrolledWindowWidget)current;
ScrolledWindowPart * cur_part    = &cur_sw-> scrolled_window;
ScrolledWindowWidget new_sw      = (ScrolledWindowWidget)new;
ScrolledWindowPart * new_part    =  &new_sw-> scrolled_window;
Boolean              do_layout   = FALSE;
Boolean              flag        = FALSE;
int                  b           = 0;
int                  h           = 0;
int                  v           = 0;
Arg                  bb_arg[10];
Arg                  hsb_arg[20];
Arg                  vsb_arg[20];

if (new_part-> align_horizontal      != cur_part-> align_horizontal      ||
    new_part-> align_vertical        != cur_part-> align_vertical        ||
    new_part-> force_hsb             != cur_part-> force_hsb             ||
    new_part-> force_vsb             != cur_part-> force_vsb             ||
    new_part-> recompute_view_height != cur_part-> recompute_view_height ||
    new_part-> recompute_view_width  != cur_part-> recompute_view_width  ||
    new_sw-> core.height             != cur_sw-> core.height             ||
    new_sw-> core.width              != cur_sw-> core.width              ||
    new_part-> view_height           != cur_part-> view_height           ||
    new_part-> view_width            != cur_part-> view_width)
   do_layout = TRUE;

if (new_part-> current_page != cur_part-> current_page)
   {
   XtSetArg(vsb_arg[v], XtNcurrentPage, new_part-> current_page);     v++;
   }

if (new_part-> foreground != cur_part-> foreground)
   {
   XtSetArg(hsb_arg[h], XtNborderColor, new_part-> foreground);       h++;
   XtSetArg(hsb_arg[h], XtNforeground,  new_part-> foreground);       h++;
   XtSetArg(vsb_arg[v], XtNborderColor, new_part-> foreground);       v++;
   XtSetArg(vsb_arg[v], XtNforeground,  new_part-> foreground);       v++;
   }

/* now check border color */
if (cur_sw->core.border_pixel != new_sw->core.border_pixel) {
        XtSetArg(bb_arg[b], XtNborderColor,new_sw->core.border_pixel); b++;
}

/* check background, so that we can update the attrs struct */
if (cur_sw->core.background_pixel != new_sw->core.background_pixel) {
	GetGC(new, new_sw->core.border_pixel, new_sw->core.background_pixel);
	flag = TRUE;
}
if (new_part-> h_granularity != cur_part-> h_granularity)
   {
   XtSetArg(hsb_arg[h], XtNgranularity, new_part-> h_granularity);    h++;
   }

if (new_part-> h_initial_delay != cur_part-> h_initial_delay)
   {
   XtSetArg(hsb_arg[h], XtNinitialDelay, new_part-> h_initial_delay); h++;
   }

if (new_part-> h_repeat_rate != cur_part-> h_repeat_rate)
   {
   XtSetArg(hsb_arg[h], XtNrepeatRate, new_part-> h_repeat_rate);     h++;
   }

if (new_part-> show_page != cur_part-> show_page)
   {
   XtSetArg(vsb_arg[v], XtNshowPage, new_part-> show_page);           v++;
   }

if (new_part-> v_granularity != cur_part-> v_granularity)
   {
   XtSetArg(vsb_arg[v], XtNgranularity, new_part-> v_granularity);    v++;
   }

if (new_part-> v_initial_delay != cur_part-> v_initial_delay)
   {
   XtSetArg(vsb_arg[v], XtNinitialDelay, new_part-> v_initial_delay); v++;
   }

if (new_part-> v_repeat_rate != cur_part-> v_repeat_rate)
   {
   XtSetArg(vsb_arg[v], XtNrepeatRate, new_part-> v_repeat_rate);     v++;
   }

if (do_layout)
   {
   if (new_sw-> core.width  == cur_sw-> core.width &&
       new_sw-> core.height == cur_sw-> core.height)
      OlLayoutScrolledWindow(new_sw, 0);
      flag = TRUE;
   }

if (h != 0 || v != 0 || b != 0)
   {
   if (h != 0) 
      XtSetValues((Widget)cur_part-> hsb, hsb_arg, h);
   if (v != 0) 
      XtSetValues((Widget)cur_part-> vsb, vsb_arg, v);
   if (b != 0) 
      XtSetValues((Widget)cur_part-> bboard, bb_arg, b);
   flag = TRUE;
   }

return (flag);

} /* end of SetValues */
/*
 * ChildResized
 *
 */

/* ARGSUSED */
static void ChildResized(Widget w, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch)
{
ScrolledWindowWidget    sw = (ScrolledWindowWidget)client_data;

if ((sw-> scrolled_window.child_width  != w-> core.width)  ||
    (sw-> scrolled_window.child_height != w-> core.height) ||
    (sw-> scrolled_window.child_bwidth != w-> core.border_width))
   {
   OlLayoutScrolledWindow(sw, 0);
   sw-> scrolled_window.child_width  = w-> core.width;
   sw-> scrolled_window.child_height = w-> core.height;
   sw-> scrolled_window.child_bwidth = w-> core.border_width;
   }

} /* end of ChildResized */
/*
 * ChildDestroyed
 *
 */

static void ChildDestroyed(Widget w, XtPointer closure, XtPointer call_data)
{
ScrolledWindowWidget sw = (ScrolledWindowWidget)(w-> core.parent)-> core.parent;

if (sw-> scrolled_window.bb_child-> core.being_destroyed == TRUE) {
   sw-> scrolled_window.bb_child = NULL;
   OlUnassociateWidget((Widget)sw);
   if (sw->core.being_destroyed == FALSE)
      OlLayoutScrolledWindow(sw, 0);
}

} /* end of ChildDestroyed */
/*
 * HSBMoved
 *
 */

static void HSBMoved(Widget w, XtPointer closure, XtPointer call_data)
{
OlScrollbarVerify *  olsbv = (OlScrollbarVerify *)call_data;
ScrollbarWidget      sb    = (ScrollbarWidget)w;
ScrolledWindowWidget sw    = (ScrolledWindowWidget)w-> core.parent;

if (sw-> scrolled_window.bb_child == (Widget)NULL)
   olsbv-> ok = FALSE;

XtCallCallbacks((Widget)sw, XtNhSliderMoved, call_data);

if (olsbv-> ok)
   if (sw-> scrolled_window.hAutoScroll) {
      if (!olsbv->more_cb_pending)
         _OlDnDSetDisableDSClipping(sw->scrolled_window.bb_child, True);

      XtMoveWidget(sw-> scrolled_window.bb_child,
                  (Position) (-olsbv-> new_location),
                  (Position)(sw-> scrolled_window.bb_child-> core.y));

      if (!olsbv->more_cb_pending) {
         _OlDnDSetDisableDSClipping(sw->scrolled_window.bb_child, False);
         OlDnDWidgetConfiguredInHier(sw-> scrolled_window.bb_child);
      } 
   }
} /* end of HSBMoved */
/*
 * VSBMoved
 *
 */

static void VSBMoved(Widget w, XtPointer closure, XtPointer call_data)
{
OlScrollbarVerify *  olsbv = (OlScrollbarVerify *)call_data;
ScrollbarWidget      sb    = (ScrollbarWidget)w;
ScrolledWindowWidget sw    = (ScrolledWindowWidget)w->core.parent;
int                  page;

if (sw-> scrolled_window.bb_child == (Widget)NULL)
   olsbv-> ok = FALSE;

XtCallCallbacks((Widget)sw, XtNvSliderMoved, call_data);

if (olsbv-> ok)
   if (sw-> scrolled_window.vAutoScroll) {
      if (!olsbv->more_cb_pending)
	 _OlDnDSetDisableDSClipping(sw->scrolled_window.bb_child, True);

      XtMoveWidget(sw-> scrolled_window.bb_child,
                   (Position)(sw-> scrolled_window.bb_child-> core.x),
                   (Position) -(olsbv-> new_location));

      if (!olsbv->more_cb_pending) {
	 _OlDnDSetDisableDSClipping(sw->scrolled_window.bb_child, False);
	 OlDnDWidgetConfiguredInHier(sw-> scrolled_window.bb_child);
      }
   }
} /* end of VSBMoved */
/*
 * GetOlSWGeometries
 *
 * The \fIGetOlSWGeometries\fR function is used to retrieve the 
 * \fIOlSWGeometries\fR for the scrolled window widget \fIsw\fR.
 *
 * Synopsis:
 * 
 *#include <ScrolledWi.h>
 * ...
 */

extern OlSWGeometries GetOlSWGeometries(ScrolledWindowWidget sw)
{
BulletinBoardWidget  bb;
Widget               bbc;
ScrollbarWidget      hsb;
ScrollbarWidget      vsb;
OlSWGeometries geometries;

bb  = sw->scrolled_window.bboard;
bbc = sw->scrolled_window.bb_child;
hsb = sw->scrolled_window.hsb;
vsb = sw->scrolled_window.vsb;

geometries.sw              = (Widget)sw;
geometries.vsb             = (Widget)vsb;
geometries.hsb             = (Widget)hsb;
geometries.bb_border_width = BORDER_3D(sw) ?
				MAX(BB_HBORDER(sw), BB_VBORDER(sw)) :
				bb-> core.border_width;
geometries.vsb_width       = vsb-> core.width + SW_VSB_GAP(sw);
geometries.vsb_min_height  = 36;
geometries.hsb_height      = hsb-> core.height + SW_HSB_GAP(sw);
geometries.hsb_min_width   = 36;
geometries.sw_view_width   = sw-> core.width  - 2 * BB_HBORDER(sw);
geometries.sw_view_height  = sw-> core.height - 2 * BB_VBORDER(sw);
if (bbc) {
	geometries.bbc_width       = bbc-> core.width;
	geometries.bbc_height      = bbc-> core.height;
	geometries.bbc_real_width  = bbc-> core.width;
	geometries.bbc_real_height = bbc-> core.height;
}
else {
	geometries.bbc_width       = 
	geometries.bbc_height      = 
	geometries.bbc_real_width  =
	geometries.bbc_real_height = 0;
}
geometries.force_vsb       = sw-> scrolled_window.force_vsb;
geometries.force_hsb       = sw-> scrolled_window.force_hsb;

return geometries;

} /* end of GetOlSWGeometries */

static void
GetGC(Widget w, Pixel fg, Pixel bg)
{
	ScrolledWindowWidget	sw = (ScrolledWindowWidget)w;
	XrmValue		src, dst;
	XColor			colorFG, colorBG1, colorBG2, colorBG3,
				colorHighlight;
	XGCValues		values;

	if (sw->scrolled_window.gc)
	    XtReleaseGC(w, sw->scrolled_window.gc);

	if (OlgIs3d(XtScreenOfObject(w))) {
	    src.size = sizeof(Pixel);
	    src.addr = (XtPointer)&fg;
	    dst.size = sizeof(XColor);
	    dst.addr = (XtPointer)&colorFG;
	    XtConvertAndStore(w, XtRPixel, &src, XtRColor, &dst);

	    src.addr = (XtPointer)&bg;
	    dst.addr = (XtPointer)&colorBG1;
	    XtConvertAndStore(w, XtRPixel, &src, XtRColor, &dst);

	    olgx_calculate_3Dcolors(&colorFG, &colorBG1, &colorBG2, &colorBG3,
		&colorHighlight);
	    XAllocColor(XtDisplay(w), OlColormapOfObject(w), &colorBG3);
	    values.foreground = colorBG3.pixel;
	    sw->scrolled_window.gc = XtGetGC(w, GCForeground, &values);
	} else {
	    values.foreground = fg;
	    sw->scrolled_window.gc = XtGetGC(w, GCForeground, &values);
	}
}
