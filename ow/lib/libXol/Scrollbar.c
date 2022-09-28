#pragma ident	"@(#)Scrollbar.c	302.23	97/03/26 lib/libXol SMI"     /* scrollbar:src/Scroll.c 1.61| */

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

#include <Xol/Menu.h>
#include <Xol/OblongButt.h>
#include <Xol/OlCursors.h>
#include <Xol/OlI18nP.h>
#include <Xol/OlStrMthdsI.h>
#include <Xol/OpenLookP.h>
#include <Xol/ScrollbarP.h>
#include <Xol/Util.h>


/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *              1. Private Procedures
 *              2. Class   Procedures
 *              3. Action  Procedures
 *              4. Public  Procedures
 *
 **************************forward*declarations***************************
 */

/* private procedures           */
static void InitializeLabels(Widget request, Widget new);
static void highlight(ScrollbarWidget sw, int invert);
static void make_page_ind(ScrollbarWidget sw);
static void MakeMenu(ScrollbarWidget sw);
static void MenuCallback(Widget w, XtPointer client_data, int buttontype);
static void MoveSlider(ScrollbarWidget sw, OlScrollbarVerify *olsb, Boolean callback, Boolean more);
static void PrevButtonCallback(Widget w, XtPointer client_data, XtPointer call_data);
static void RemoveHandler(Widget w, XtPointer client_data, XtPointer call_data);
static void ShowPageInd(Widget w, XtPointer data, XEvent *xevent, Boolean *cont_to_dispatch);
static void TimerEvent(XtPointer client_data, XtIntervalId *id);
static void TopToButtonCallback(Widget w, XtPointer client_data, XtPointer call_data);
static void ToTopButtonCallback(Widget w, XtPointer client_data, XtPointer call_data);
static void Update_page_ind(ScrollbarWidget sw, int flags);
static void SBError(int idx);

static void UpdateScrollbar(ScrollbarWidget sbw, OlgxAttrs *pInfo, unsigned int flags);
static void DrawScrollbar(ScrollbarWidget sbw, OlgxAttrs *pInfo);
static void LocateElevator(ScrollbarWidget sbw, Position cablePos, Dimension cableLen);
static void SizeScrollbarAnchor(ScrollbarWidget sbw, OlgxAttrs *pInfo, 
			Dimension *pWidth, Dimension *pHeight);
static void SizeScrollbarElevator(ScrollbarWidget sbw, OlgxAttrs *pInfo,
			unsigned int type, Dimension *pWidth, Dimension *pHeight);
				

/* class procedures             */
static void              ClassInitialize(void);
static void              Destroy(Widget w);
static void              GetValuesHook(Widget w, ArgList args, Cardinal *num_args);
static void              Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);
static XtGeometryResult  QueryGeom(Widget w, XtWidgetGeometry *intended, XtWidgetGeometry *reply);
static void              Realize(Widget w, Mask *valueMask, XSetWindowAttributes *attributes);
static void              Redisplay(Widget w, XEvent *event, Region region);
static void              Resize(Widget w);
static Boolean           SetValues (Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);

/* action procedures            */
static void Menu(Widget w, XEvent *event, String *params, Cardinal *num_params);             /* What on MENU Btn Down        */
static void SelectDown(Widget w, XEvent *event, String *params, Cardinal *num_params);
static void SelectUp(Widget w, XEvent *event, String *params, Cardinal *num_params);

/* stuff for mouseless operation */
static Boolean	SBActivateWidget (Widget, OlVirtualName, XtPointer);
static void	SBButtonHandler (Widget w, OlVirtualEvent ve);

static void	ChangeDimState (Widget w, Boolean state);
static Boolean	MenuKeyPress(Widget w);
static Boolean	ScrollKeyPress(Widget w, unsigned char opcode);
static		inAnchor(ScrollbarWidget sw, int x, int y, int anchor);

static OlEventHandlerRec sb_event_procs[] =
{
	{ ButtonPress,	SBButtonHandler },
	{ ButtonRelease,SBButtonHandler },
};

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define SWB             sw->scroll
#define THEBUTTON1	SWB.here_to_lt_btn
#define THEBUTTON2	SWB.lt_to_here_btn
#define ISKBDOP		(SWB.opcode & KBD_OP)
#define HORIZ(W)        ((W)->scroll.orientation == OL_HORIZONTAL)
#define INRANGE(V,MIN,MAX) (((V) <= (MIN)) ? (MIN) : ((V) >= (MAX) ? (MAX) : (V)))
#define PIX_TO_VAL(M1,M2,Q1)	(((unsigned long)(M2)/(Q1)*(M1))+(((M2)%(Q1)*(M1)+(Q1-1))/(Q1)))
#define VAL_TO_PIX(M1,M2,Q1)	(((unsigned long)(M2)/(Q1)*(M1))+(((M2)%(Q1))*(M1)/(Q1)))
#define MAXSLIDERVAL(W)	((W)->scroll.sliderMax - (W)->scroll.proportionLength)

/* Standard menu button types */
#define HERE_TO_BEGIN           1
#define PREVIOUS                2
#define BEGIN_TO_HERE           3

/* Scroll Resources  Defaults */
#define DFLT_MIN 0
#define DFLT_MAX 100
#define DFLT_REPEATRATE        100
#define DFLT_INITIALDELAY      500
#define DFLT_SCALE             OL_DEFAULT_POINT_SIZE

/* page indicator constants */
#define PI_MOVE		1
#define PI_DRAW		2

/* scrollbar rendering states */
#define SB_POSITION     1
#define SB_BEGIN_ANCHOR 2
#define SB_END_ANCHOR   4
#define SB_PREV_ARROW   8
#define SB_NEXT_ARROW   16
#define SB_DRAG         32

/*
 *************************************************************************
 * Define Translations and Actions
 ***********************widget*translations*actions**********************
 */

/* mouseless: The Translation Table is inherited from its superclass */

/*
 *************************************************************************
 * Define Resource list associated with the Widget Instance
 ****************************widget*resources*****************************
 */

#define offset(field)  XtOffset(ScrollbarWidget, field)

static XtResource resources[] = {
        {XtNsliderMin, XtCSliderMin, XtRInt, sizeof(int),
                offset(scroll.sliderMin), XtRImmediate, (XtPointer)DFLT_MIN},
        {XtNsliderMax, XtCSliderMax, XtRInt, sizeof(int),
                offset(scroll.sliderMax), XtRImmediate, (XtPointer)DFLT_MAX },
        {XtNsliderValue, XtCSliderValue, XtRInt, sizeof(int),
                offset(scroll.sliderValue), XtRImmediate, (XtPointer)0 },
        {XtNorientation, XtCOrientation, XtROlDefine, sizeof(OlDefine),
                offset(scroll.orientation), XtRImmediate,
		(XtPointer)OL_VERTICAL },
        {XtNgranularity, XtCGranularity, XtRInt, sizeof(int),
                offset(scroll.granularity), XtRImmediate, (XtPointer)1 },
        {XtNsliderMoved, XtCSliderMoved, XtRCallback, sizeof(XtPointer),
                offset(scroll.sliderMoved), XtRCallback, (XtPointer)NULL },
        {XtNproportionLength, XtCProportionLength, XtRInt, sizeof(int),
                offset(scroll.proportionLength), XtRImmediate,
		(XtPointer)(DFLT_MAX - DFLT_MIN) },
        {XtNshowPage, XtCShowPage, XtROlDefine, sizeof(OlDefine),
                offset(scroll.showPage), XtRImmediate, (XtPointer)OL_NONE },
        {XtNcurrentPage, XtCCurrentPage, XtRInt, sizeof(int),
                offset(scroll.currentPage), XtRImmediate, (XtPointer)1 },
        {XtNrepeatRate, XtCRepeatRate, XtRInt, sizeof(int),
                offset(scroll.repeatRate), XtRImmediate,
		(XtPointer)DFLT_REPEATRATE },
        {XtNinitialDelay, XtCInitialDelay, XtRInt, sizeof(int),
                offset(scroll.initialDelay), XtRImmediate,
		(XtPointer)DFLT_INITIALDELAY },
        {XtNpointerWarping, XtCPointerWarping, XtRBoolean, sizeof(Boolean),
                offset(scroll.warp_pointer), XtRImmediate,
                (XtPointer)OL_POINTER_WARPING },

	/* resources added in OL 2.1 */
        {XtNdragCBType, XtCDragCBType, XtROlDefine, sizeof(OlDefine),
                offset(scroll.dragtype), XtRImmediate, 
		(XtPointer)OL_CONTINUOUS },
        {XtNstopPosition, XtCStopPosition, XtROlDefine, sizeof(OlDefine),
                offset(scroll.stoppos), XtRImmediate, 
		(XtPointer)OL_ALL },
        {XtNuseSetValCallback, XtCUseSetValCallback, XtRBoolean,
		sizeof(Boolean), offset(scroll.useSetValCallback), 
		XtRString, "FALSE" },

        /* resources added for Level 3 I18N */
#ifdef XGETTEXT
        {XtNmenuTitle, XtCMenuTitle, XtROlStr, sizeof(OlStr),
            offset(scroll.menuTitle), XtRString,
	(XtPointer)dgettext(OlMsgsDomain,"Scrollbar")},
 
        {XtNhereToTopLabel, XtCHereToTopLabel, XtROlStr, sizeof(OlStr),
                offset(scroll.hereToTopLabel), XtRString,
                (XtPointer)dgettext(OlMsgsDomain,"Here To Top") },
 
        {XtNtopToHereLabel, XtCTopToHereLabel, XtROlStr, sizeof(OlStr),
                offset(scroll.topToHereLabel), XtRString,
                (XtPointer) dgettext(OlMsgsDomain, "Top To Here") },
 
        {XtNhereToLeftLabel, XtCHereToLeftLabel, XtROlStr, sizeof(OlStr),
                offset(scroll.hereToLeftLabel), XtRString,
                (XtPointer)dgettext(OlMsgsDomain, "Here To Left")  },
 
        {XtNleftToHereLabel, XtCLeftToHereLabel, XtROlStr, sizeof(OlStr),
                offset(scroll.leftToHereLabel), XtRString,
                (XtPointer) dgettext(OlMsgsDomain, "Left To Here")},
 
        {XtNpreviousLabel, XtCPreviousLabel, XtROlStr, sizeof(OlStr),
                offset(scroll.previousLabel), XtRString,
                (XtPointer) dgettext(OlMsgsDomain, "Previous") },
#else
        {XtNmenuTitle, XtCMenuTitle, XtROlStr, sizeof(OlStr),
            offset(scroll.menuTitle), XtRLocaleString,
		 (XtPointer)"Scrollbar"},
 
        {XtNhereToTopLabel, XtCHereToTopLabel, XtROlStr, sizeof(OlStr),
                offset(scroll.hereToTopLabel), XtRLocaleString,
                (XtPointer)"Here To Top" },
 
        {XtNtopToHereLabel, XtCTopToHereLabel, XtROlStr, sizeof(OlStr),
                offset(scroll.topToHereLabel), XtRLocaleString,
                (XtPointer)  "Top To Here" },
 
        {XtNhereToLeftLabel, XtCHereToLeftLabel, XtROlStr, sizeof(OlStr),
                offset(scroll.hereToLeftLabel), XtRLocaleString,
                (XtPointer) "Here To Left"  },
 
        {XtNleftToHereLabel, XtCLeftToHereLabel, XtROlStr, sizeof(OlStr),
                offset(scroll.leftToHereLabel), XtRLocaleString,
                (XtPointer)  "Left To Here"},
 
        {XtNpreviousLabel, XtCPreviousLabel, XtROlStr, sizeof(OlStr),
                offset(scroll.previousLabel), XtRLocaleString,
                (XtPointer)  "Previous" },
#endif
 
        {XtNhereToTopMnemonic, XtCHereToTopMnemonic, OlRChar, sizeof(char),
                offset(scroll.hereToTopMnemonic), OlRChar,
                (XtPointer) NULL },
 
        {XtNtopToHereMnemonic, XtCTopToHereMnemonic, OlRChar, sizeof(char),
                offset(scroll.topToHereMnemonic), OlRChar,
                (XtPointer) NULL },
 
        {XtNhereToLeftMnemonic,XtCHereToLeftMnemonic,OlRChar, sizeof(char),
                offset(scroll.hereToLeftMnemonic), OlRChar,
                (XtPointer) NULL },
 
        {XtNleftToHereMnemonic,XtCLeftToHereMnemonic,OlRChar, sizeof(char),
                offset(scroll.leftToHereMnemonic), OlRChar,
                (XtPointer) NULL },
 
        {XtNpreviousMnemonic, XtCPreviousMnemonic, OlRChar, sizeof(char),
                offset(scroll.previousMnemonic), OlRChar,
                (XtPointer) NULL },

};
#undef offset

/*
 *************************************************************************
 * Define Class Record structure to be initialized at Compile time
 ***************************widget*class*record***************************
 */
ScrollbarClassRec scrollbarClassRec = {
        {
        /* core_class fields      */
        /* superclass         */    (WidgetClass) &primitiveClassRec,
        /* class_name         */    "Scrollbar",
        /* widget_size        */    sizeof(ScrollbarRec),
        /* class_initialize   */    ClassInitialize,
        /* class_part_init    */    NULL,
        /* class_inited       */    FALSE,
        /* initialize         */    Initialize,
        /* initialize_hook    */    NULL,
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
        /* set_values_hook    */    NULL,
        /* set_values_almost  */    XtInheritSetValuesAlmost,
        /* get_values_hook    */    GetValuesHook,
        /* accept_focus       */    NULL,
        /* version            */    XtVersion,
        /* callback_private   */    NULL,
        /* tm_table           */    XtInheritTranslations,
        /* query_geometry     */    (XtGeometryHandler)QueryGeom,
	/* display_accelerator*/    NULL,
	/* extension	      */    NULL,
        },
/* changed for mouseless operation */
  {					/* primitive class	*/
      NULL,				/* reserved		*/
      NULL,				/* highlight_handler	*/
      NULL,				/* traversal_handler	*/
      NULL,				/* register_focus	*/
      SBActivateWidget,			/* activate		*/ 
      sb_event_procs,			/* event_procs		*/
      XtNumber(sb_event_procs),		/* num_event_procs	*/
      OlVersion,			/* version		*/
      NULL,				/* extension		*/
      { NULL, 0 },			/* dyn_data		*/
      _OlDefaultTransparentProc,	/* transparent_proc	*/
      NULL,				/* query_sc_locn_proc   */
  },
        {
        /* Scrollbar class fields */
        /* empty                  */    0,
        }
};

/*
 *************************************************************************
 * Public Widget Class Definition of the Widget Class Record
 *************************public*class*definition*************************
 */
WidgetClass scrollbarWidgetClass = (WidgetClass)&scrollbarClassRec;

/*
 *************************************************************************
 * Private Procedures
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * GetGCs - this routine gets the normal GC for the Scrollbar
 ****************************procedure*header*****************************
 */
static void
GetGCs(ScrollbarWidget sw)
{
        XGCValues values;

       /* destroy old GCs */
        if (sw->scroll.textGC != NULL)
           XtDestroyGC(sw->scroll.textGC);

	if (sw->scroll.pAttrs != (OlgxAttrs *) NULL) {
		OlgxDestroyAttrs ((Widget)sw, sw->scroll.pAttrs);
	}

	/* get new GCs. */
        values.foreground = sw->primitive.foreground;
	if(sw->primitive.text_format == OL_SB_STR_REP){
		values.font	  = ((XFontStruct *)
					sw->primitive.font)->fid;
		sw->scroll.textGC = XtGetGC((Widget)sw,
				    (unsigned) (GCForeground | GCFont),
				    &values);
	}
	else{
		sw->scroll.textGC = XtGetGC((Widget)sw,
				    (unsigned) (GCForeground),
				    &values);
	}

	sw->scroll.pAttrs = OlgxCreateAttrs ((Widget)sw,
						sw->primitive.foreground,
						(OlgxBG *)&(sw->core.background_pixel),
						False, sw->primitive.scale,
						(OlStrRep)0, (OlFont)NULL);

} /* END OF GetGCs() */

static void
MakeMenu(ScrollbarWidget sw)
{
        Arg args[10];
        Widget * pane = &sw-> scroll.pane;
	OlStr label;
	int i;

        /* Create the menu pane associated with the Scrollbar. */
	i = 0;
        XtSetArg(args[i], XtNmenuAugment, FALSE); i++;
        XtSetArg(args[i], XtNshellTitle, sw->scroll.menuTitle); i++;
        XtSetArg(args[i], XtNtextFormat, sw->primitive.text_format); i++;
        sw->scroll.popup = XtCreatePopupShell("ScrollMenu",
                menuShellWidgetClass, (Widget)sw, args, 2);
        XtSetArg(args[0], XtNmenuPane, pane);
        XtGetValues(sw-> scroll.popup, args, 1);

/* to support the mouseless operation, scrollbar needs to cache
   the first two button's ids */
	i = 0;
        XtSetArg(args[i], XtNmnemonic,
                HORIZ(sw) ? sw->scroll.hereToLeftMnemonic :
                sw->scroll.hereToTopMnemonic); i++;
	XtSetArg(args[i],XtNtextFormat,sw->primitive.text_format); i++;
	label = (HORIZ(sw) ? sw->scroll.hereToLeftLabel : 
					sw->scroll.hereToTopLabel);
	XtSetArg(args[i],XtNlabel,label); i++;
        XtAddCallback( (THEBUTTON1 = XtCreateManagedWidget("but1", 
			oblongButtonGadgetClass, *pane, args, i)),
            XtNselect, ToTopButtonCallback, (XtPointer)sw);

	i = 0;
        XtSetArg(args[i], XtNmnemonic,
                HORIZ(sw) ? sw->scroll.leftToHereMnemonic :
                sw->scroll.topToHereMnemonic); i++;
	XtSetArg(args[i],XtNtextFormat,sw->primitive.text_format); i++;
	label = (HORIZ(sw) ? sw->scroll.leftToHereLabel : 
					sw->scroll.topToHereLabel);
	XtSetArg(args[i],XtNlabel,label); i++;
        XtAddCallback( (THEBUTTON2 = XtCreateManagedWidget("but2",
            oblongButtonGadgetClass, *pane, args, i)),
            XtNselect, TopToButtonCallback, (XtPointer)sw);

	i = 0;
        XtSetArg(args[i], XtNmnemonic, sw->scroll.previousMnemonic); i++;
	XtSetArg(args[i],XtNlabel,sw->scroll.previousLabel); i++;
	XtSetArg(args[i],XtNtextFormat,sw->primitive.text_format); i++;
        XtAddCallback( XtCreateManagedWidget("but3",
            oblongButtonGadgetClass, *pane, args, i),
            XtNselect, PrevButtonCallback, (XtPointer)sw);
} /* end of MakeMenu */

static void
ToTopButtonCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuCallback(w, client_data, HERE_TO_BEGIN);
}

static void
TopToButtonCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuCallback(w, client_data, BEGIN_TO_HERE);
}

static void
PrevButtonCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuCallback(w, client_data, PREVIOUS);
}

static void
MenuCallback(Widget w, XtPointer client_data, int buttontype)
{
       ScrollbarWidget sw = (ScrollbarWidget)client_data;
       OlScrollbarVerify olsb;

	/* save current position */
       if (buttontype == PREVIOUS) {
                olsb.new_location = INRANGE(SWB.previous,SWB.sliderMin,
                                        SWB.sliderMax - SWB.proportionLength);
		sw->scroll.previous = sw->scroll.sliderValue;
       }
       else {
                int val;
		int cableLength;

		if (HORIZ (sw))
		    cableLength = sw->core.width - sw->scroll.anchwidth * 2;
		else
		    cableLength = sw->core.height - sw->scroll.anchlen * 2;

                val = VAL_TO_PIX(SWB.XorY,SWB.proportionLength, cableLength);
		sw->scroll.previous = sw->scroll.sliderValue;
                if (buttontype == BEGIN_TO_HERE)
                        olsb.new_location = MAX(sw->scroll.sliderValue - val,
                                                sw->scroll.sliderMin);
                else
                        olsb.new_location = MIN(sw->scroll.sliderValue + val,
                                                SWB.sliderMax - SWB.proportionLength);
       }

	if (olsb.new_location != sw->scroll.sliderValue);
        	MoveSlider(sw,&olsb,TRUE,FALSE);
} /* MenuCallback */

/*
 *************************************************************************
 * ReturnPaneId - this procedure checks the Arg list to set if someone
 * is requesting the id of the menu pane.  If they are, the pane id
 * is written to the location specified in the Arg list.
 ****************************procedure*header*****************************
 */

static void
ReturnPaneId(Widget w, ArgList args, Cardinal *num_args)
                                /* scrollbar widget id          */
                                /* Arg List for Scrollbar       */
                                /* number of Args               */
{
        ScrollbarWidget sw = (ScrollbarWidget) w;
        MaskArg         mask_args[2];
        Widget *        pane = NULL;

        if (*num_args != 0) {
               _OlSetMaskArg(mask_args[0], XtNmenuPane, &pane, OL_COPY_SOURCE_VALUE);
               _OlSetMaskArg(mask_args[1], NULL, sizeof(Widget *),OL_COPY_SIZE);
               _OlComposeArgList(args, *num_args, mask_args, 2, NULL, NULL);

               if (pane != NULL) {
                       if (sw->scroll.pane == NULL)
                               MakeMenu(sw);
                       *pane = sw->scroll.pane;
               }
        }
} /* END OF ReturnPaneId() */

/*
 *************************************************************************
 * TimerEvent - a function registered to trigger every time the
 *              repeatRate time interval has elapsed. At the end of
 *              this functions it registers itself.
 *              Thus, elev is moved, ptr is warped and function is
 *              reregistered.
 ****************************procedure*header*****************************
 */
static void
TimerEvent(XtPointer client_data, XtIntervalId *id)
{
        ScrollbarWidget sw = (ScrollbarWidget)client_data;
        OlScrollbarVerify olsb;
        int warppoint = 0;
	int sx = 0,sy = 0,width = 0,height = 0;
	Boolean morepending = FALSE;
	Boolean docallback = TRUE;

	if (sw->scroll.opcode == NOOP) {
		sw->scroll.timerid = (XtIntervalId)NULL;
		return;
	}

       /* determine where to move to */
       if (sw->scroll.opcode & ANCHOR)
                olsb.new_location = (sw->scroll.opcode & DIR_INC) ?
				    SWB.sliderMax - SWB.proportionLength :
                                    sw->scroll.sliderMin;
	else if (sw->scroll.opcode == DRAG_ELEV) {
		Window wjunk;
		int junk, winx, winy;
		static int minVal;
		static int maxVal;
		static int valueRange;
		static int pixelRange;
		int pixel;	

		if (sw->scroll.timerid == (XtIntervalId)NULL) {
			/* first time */
			minVal = sw->scroll.sliderMin;
			maxVal = SWB.sliderMax - SWB.proportionLength;
			valueRange = maxVal - minVal;
			if (HORIZ (sw))
			    pixelRange = sw->core.width -
				sw->scroll.anchwidth*2 - sw->scroll.elevwidth;
			else
			    pixelRange = sw->core.height -
				sw->scroll.anchlen*2 - sw->scroll.elevheight;
		}
		XQueryPointer(XtDisplay(sw), XtWindow(sw), &wjunk, &wjunk,
			      &junk, &junk, &winx, &winy, (unsigned *)&junk);

		{
		    int point      = (HORIZ(sw) ? winx : winy);
		    int elevEndPos = sw->scroll.elevheight;
		    int partSize   = (int) (elevEndPos+1)/
			(int) (sw->scroll.type&EPARTMASK);
		    int offset     = point - sw->scroll.sliderPValue;

		    if (sw->scroll.opcode == DRAG_ELEV &&
			offset >= partSize             &&
			offset < (elevEndPos - partSize))
			    olsb.new_location = sw->scroll.sliderValue;
		    else {
			pixel = point + sw->scroll.dragbase;
			pixel = PIX_TO_VAL(pixel,valueRange,pixelRange);
			olsb.new_location = INRANGE(pixel + SWB.sliderMin,
						    minVal, maxVal);
		    }
		}
		/*
		 * Handle DragCBType here.
		 */
		if (sw->scroll.dragtype == OL_GRANULARITY) {
		    	if ((olsb.new_location != sw->scroll.sliderMin) &&
		            (olsb.new_location != MAXSLIDERVAL(sw)) &&
			    (((olsb.new_location - SWB.sliderMin) %
			       SWB.granularity) != 0))
				docallback = FALSE;
		} /* OL_GRANULARITY */
		else if (sw->scroll.dragtype == OL_RELEASE)
			docallback = FALSE;

		morepending = TRUE;
	} /* DRAG_TYPE */
       else {
               int step;

		/*
		 * warppoint has dual roles here.
		 * For page operation, warppoint is the relative amount
		 * in pixels to move. For other operations, warppoint is
		 * a boolean to indicate warping is needed.
		 */
		if (sw->scroll.opcode & PAGE) {
               		step = SWB.proportionLength;
			warppoint = -3;
		}
		else {
               		step = SWB.granularity;
			warppoint = 1;
		}
               if (sw->scroll.opcode & DIR_INC) {
                       warppoint = (HORIZ (sw) ? sw->scroll.elevwidth :
			   sw->scroll.elevheight) + 2;
                       olsb.new_location = MIN(SWB.sliderMax - SWB.proportionLength,
                                               SWB.sliderValue + step);
               }
               else {
                       olsb.new_location = MAX(SWB.sliderMin,
                                               SWB.sliderValue - step);
               }
       }

        if (olsb.new_location != sw->scroll.sliderValue) {
		int save;


		save = sw->scroll.sliderPValue;
                MoveSlider(sw, &olsb, docallback, morepending);
/* make sure it's not a mouseless operation */
                if ((ISKBDOP == False) && olsb.ok && warppoint &&
			(SWB.warp_pointer == True)) {
                        int wx, wy;

			if (sw->scroll.opcode & DIR_INC) {
				if (HORIZ(sw))
					width = SWB.sliderPValue+SWB.elevwidth;
				else
					height = SWB.sliderPValue +
					    SWB.elevheight;
			}
			else {
				if (HORIZ(sw))
					sx = SWB.sliderPValue;
				else
					sy = SWB.sliderPValue;
			}

			if (sw->scroll.opcode & PAGE) {
				if (HORIZ(sw)) {
					wx = SWB.sliderPValue + warppoint;
					wy = sw->core.height / 2;
				}
				else {
					wx = sw->core.width / 2;
					wy = SWB.sliderPValue + warppoint;
				}
                        	XWarpPointer(XtDisplay(sw), XtWindow(sw),
					     XtWindow(sw), sx, sy,
					     width, height, wx, wy);
			}
			else {
			    wx = HORIZ(sw) ? SWB.sliderPValue - save : None;
			    wy = HORIZ(sw) ? None : SWB.sliderPValue - save;
			    XWarpPointer(XtDisplay(sw), XtWindow(sw), None,
					 0, 0, 0, 0, wx, wy);
			}
                }
        }
        else if (sw->scroll.opcode != DRAG_ELEV &&
			!(sw->scroll.opcode & ANCHOR)) {
		SelectUp((Widget)sw,NULL,NULL,NULL);
                return;
	}

/* make sure it's not a mouseless operation */
        if ((ISKBDOP == False) && !(sw->scroll.opcode & ANCHOR)) {
		XtAppContext	ac = XtWidgetToApplicationContext((Widget)sw);
		/*
		 * check for button release here so it seems
		 * more responsive to the user.
		 */
		if (sw->scroll.opcode == DRAG_ELEV) {
			SWB.timerid = XtAppAddTimeOut(ac, 0, TimerEvent,
						     (XtPointer)sw);
		}
		else if (sw->scroll.timerid == (XtIntervalId)NULL) {
                            SWB.timerid = XtAppAddTimeOut(ac,
							  SWB.initialDelay,
                                                          TimerEvent,
							  (XtPointer)sw);
                }
                else {
                        if (sw->scroll.repeatRate > 0) {
                                SWB.timerid = XtAppAddTimeOut(ac, 
							      SWB.repeatRate,
                                                              TimerEvent,
							      (XtPointer)sw);
                        }
                }
        }
} /* TimerEvent */


/* muldiv - Multiply two numbers and divide by a third.
 *
 * Calculate m1*m2/q where m1, m2, and q are integers.  Be careful of
 * overflow.
 */
#define muldiv(m1, m2, q)	((m2)/(q) * (m1) + (((m2)%(q))*(m1))/(q));

/*
 ********************************************************
 * LocateElevator: determine new position of elevator
 * and indicator.  If possible, adjust indicator so that
 * at least a minimum indicator length will display on
 * each side of the elevator. (NOTE: this code was
 * borrowed from OlgScrollbar.c).
 ********************************************************
 */
static void
LocateElevator(ScrollbarWidget sbw, Position cablePos, Dimension cableLen)
{
    Screen *	scr = XtScreenOfObject((Widget)sbw);
    Position	elevPos, indPos;
    Dimension	elevLen, indLen, minIndLen;
    int		userRange;	/* size of user coordinate space */

    if (HORIZ(sbw)) {
	elevLen = sbw->scroll.elevwidth;
	minIndLen = (Dimension)OlgxScreenPointToPixel(OL_HORIZONTAL, 1, scr)*3;

    } else { /* Vertical */
	elevLen = sbw->scroll.elevheight;
	minIndLen = (Dimension)OlgxScreenPointToPixel(OL_VERTICAL, 1, scr)*3;
    }

    /* Find the position of the elevator */
    userRange = sbw->scroll.sliderMax - sbw->scroll.sliderMin;
    if (sbw->scroll.type != SB_REGULAR) {
	/* center the elevator */
	elevPos = (Position) (cableLen - elevLen) / 2;
    
    } else {
	if (userRange != sbw->scroll.proportionLength) {
	    elevPos = muldiv (sbw->scroll.sliderValue - sbw->scroll.sliderMin,
			      (int) (cableLen - elevLen),
			      userRange - sbw->scroll.proportionLength);
	} else
	    elevPos = 0;
    }

    sbw->scroll.sliderPValue = cablePos + elevPos;

    /* Find the proportion indicator positions */
    if (sbw->scroll.type != SB_REGULAR) {
	indLen = 0;
	indPos = 0;
    } else {
	Position	elevEndPos;

	elevEndPos = elevPos + elevLen;

	/* Find the position of the proportion indicator */
	indLen = muldiv (sbw->scroll.proportionLength, (int) cableLen,
			 userRange);

	if (indLen < (Dimension) (elevLen + minIndLen * 2)) {
	    /* Proportion indicator is hidden by the elevator.
	     * Draw a short one anyway.
	     */
	    indLen = elevLen + minIndLen * 2;
	    indPos = elevPos - minIndLen;
	} else {
	    /* Full sized proportion indicator.  Adjust it's position such
	     * that at least 3 points show on each side of the elevator.
	     */
	    if (userRange != sbw->scroll.proportionLength) {
		indPos = muldiv (sbw->scroll.sliderValue-sbw->scroll.sliderMin,
				 (int) (cableLen - indLen),
				 userRange - sbw->scroll.proportionLength);
	    
	    } else
		indPos = 0;
	    if ((Dimension) (elevPos - indPos) < minIndLen)
		indPos = elevPos - minIndLen;
	    else
		if ((Position) (indPos + indLen - elevEndPos) <
		    (Position) minIndLen)  
		    indPos = elevEndPos + minIndLen - 1 - indLen;
	}

	/* clip the indicator */
	if (indPos < 0)
	    indPos = 0;
	if ((Position) (indPos + indLen) > (Position) cableLen - 1) {
	    /* We subtract 1 to get around a bug in OLGX */
	    indPos = cableLen - indLen - 1;
	    if (indPos < 0) {
		indPos = 0;
		indLen = cableLen;
	    }
	}
    }
    sbw->scroll.indPos = cablePos + indPos;
    sbw->scroll.indLen = indLen;

} /* LocateElevator */

/*
 ***************************************************************
 * UpdateScrollbar:  Redraw ONLY the part of the scrollbar
 * which needs updating using OLGX:  EndBoxes or
 * elevator/indicator position.
 **************************************************************
 */
static void
UpdateScrollbar(ScrollbarWidget sbw, OlgxAttrs *pInfo, unsigned int flags)
{
    Screen *	scr = XtScreenOfObject((Widget)sbw);
    Window	win = XtWindow((Widget)sbw);
    Dimension	padLen;
    unsigned	horizontal = (sbw->scroll.orientation == OL_HORIZONTAL);
    int		state = OLGX_UPDATE | OLGX_ERASE;

    padLen = (horizontal) ? (Dimension)OlgxScreenPointToPixel(OL_HORIZONTAL, 2, scr):
		(Dimension)OlgxScreenPointToPixel(OL_VERTICAL, 2, scr);

    /* Update anchors */
    if (sbw->scroll.type != SB_MINIMUM) {

	if (flags & SB_BEGIN_ANCHOR) {
	    if (horizontal) {
		olgx_draw_choice_item(pInfo->ginfo, win, 0, sbw->scroll.offset,
					sbw->scroll.anchwidth, sbw->scroll.anchlen,
					(long)NULL, state | 
                            		((sbw->scroll.opcode==ANCHOR_TOP)?
						OLGX_INVOKED : OLGX_NORMAL));

	    } else { /* Vertical */
                olgx_draw_choice_item(pInfo->ginfo, win, sbw->scroll.offset, 0,
                        		sbw->scroll.anchwidth, sbw->scroll.anchlen, 
                        		(long)NULL, state |  
                            		((sbw->scroll.opcode==ANCHOR_TOP)?
                                		OLGX_INVOKED : OLGX_NORMAL));
	    }
	    return; 
	}

	if (flags & SB_END_ANCHOR) {
	    if (horizontal) {
		olgx_draw_choice_item(pInfo->ginfo, win, 
				sbw->core.width - sbw->scroll.anchwidth,
				sbw->scroll.offset, sbw->scroll.anchwidth,
                        	sbw->scroll.anchlen, (long)NULL, 
                        	state | ((sbw->scroll.opcode==ANCHOR_BOT)?
                                	OLGX_INVOKED : OLGX_NORMAL)); 

	    } else { /* Vertical */
                olgx_draw_choice_item(pInfo->ginfo, win, sbw->scroll.offset,
                        	sbw->core.height - sbw->scroll.anchlen,
                        	sbw->scroll.anchwidth, sbw->scroll.anchlen, (long)NULL,
                        	state | ((sbw->scroll.opcode==ANCHOR_BOT)?
                                	OLGX_INVOKED : OLGX_NORMAL));
	    }
	return;
	}
    }

    if (flags & (SB_POSITION | SB_PREV_ARROW | SB_NEXT_ARROW | SB_DRAG)) {
	Position	oldElevPos, oldIndPos;
	Position	elevPos, indPos;
	Position	scrollbarX, scrollbarY;
	Position	cablePos;
	Dimension	elevLen, cableLen, scrollbarLen, indLen;

	switch (sbw->scroll.opcode) {
	    case DRAG_ELEV:  state |= OLGX_SCROLL_ABSOLUTE;
			break;
	    case GRAN_DEC:   state |= OLGX_SCROLL_BACKWARD;
			break;
	    case GRAN_INC:   state |= OLGX_SCROLL_FORWARD;
			break;
	    case ANCHOR_BOT: state |= OLGX_SCROLL_NO_FORWARD;
			break;
	    case ANCHOR_TOP: state |= OLGX_SCROLL_NO_BACKWARD;
			break;
	    default:	state |= OLGX_NORMAL;
	}

	/* Calculate the length and offset of the cable area */
	if (horizontal) {
	    state |= OLGX_HORIZONTAL;
	    cablePos = sbw->scroll.anchwidth + padLen;
	    scrollbarX = (Position)0;
	    scrollbarY = (Position)sbw->scroll.offset; 
	    scrollbarLen = sbw->core.width;
	    cableLen = sbw->core.width - ((sbw->scroll.anchwidth+padLen)*2);
	    elevLen = sbw->scroll.elevwidth;
	
	} else { /* Vertical */
	    state |= OLGX_VERTICAL;
	    cablePos = sbw->scroll.anchlen + padLen;
	    scrollbarY = (Position)0;
            scrollbarX = (Position)sbw->scroll.offset;   
	    scrollbarLen = sbw->core.height;
	    cableLen = sbw->core.height - ((sbw->scroll.anchlen+padLen)*2);
	    elevLen = sbw->scroll.elevheight;
	}

	/* Save the old indicator and elevator positions */
	oldElevPos = sbw->scroll.sliderPValue;
	oldIndPos = sbw->scroll.indPos;

	/* Get the new position of the elevator and indicator */
	LocateElevator (sbw, cablePos, cableLen);

	elevPos = sbw->scroll.sliderPValue;
	indLen = sbw->scroll.indLen;
	indPos = sbw->scroll.indPos;

        if (sbw->scroll.type == SB_ABBREVIATED ||
                sbw->scroll.type == SB_MINIMUM) 
            state |= OLGX_ABBREV;

/* OLGX_TODO: This doesn't seem to have any effect . ??OLGX bug? */
	if (sbw->scroll.sliderValue == sbw->scroll.sliderMin)
	    state |= OLGX_SCROLL_NO_BACKWARD;
  	else if (sbw->scroll.sliderValue == sbw->scroll.sliderMax)
	    state |= OLGX_SCROLL_NO_FORWARD;

        olgx_draw_scrollbar(pInfo->ginfo, win, scrollbarX, scrollbarY,
                scrollbarLen, elevPos, oldElevPos, indPos, indLen, state);

    }
} /* UpdateScrollbar */

/*
 ************************************************************
 * DrawScrollbar:  render a complete scrollbar using OLGX:
 * endBoxes, cable, elevator & indicator.
 ************************************************************
*/
static void
DrawScrollbar(register ScrollbarWidget sbw, OlgxAttrs *pInfo)
{
    Display	*dpy = XtDisplay (sbw);
    Screen	*scr = XtScreenOfObject ((Widget)sbw);
    Drawable	win = XtWindow (sbw);
    unsigned	horizontal = (sbw->scroll.orientation == OL_HORIZONTAL);
    Position	scrollbarX, scrollbarY; /* Position of scrollbar */
    Dimension   scrollbarLen;   /* Overall Length of scrollbar */
    Position    cablePos;       /* offset where cable begins */
    Dimension	cableLen;	/* length of cable area */
    Dimension   padLen;         /* length of gap between endBox and cable */
    Position	elevPos;	/* offset of elevator in scrollbar */
    Dimension	elevLen;	/* length of elevator */
    Position	indPos;		/* offset of proportion indicator */
    Dimension   indLen;		/* length of proportion indicator */
    int		state = 0;

    if (!XtIsSensitive((Widget)sbw))
	state |= OLGX_INACTIVE;

    if (horizontal) {
	state |= OLGX_HORIZONTAL;
	cableLen = scrollbarLen = sbw->core.width;
	padLen = (Dimension)OlgxScreenPointToPixel(OL_HORIZONTAL,2,scr);
	elevLen = sbw->scroll.elevwidth;
    
    } else { /* Vertical */
	state |= OLGX_VERTICAL;
	cableLen = scrollbarLen = sbw->core.height;
	padLen = (Dimension)OlgxScreenPointToPixel(OL_VERTICAL,2,scr);
	elevLen = sbw->scroll.elevheight;
    }
    cablePos = 0;

    /* If the scrollbar has anchors, draw them. */
    if (sbw->scroll.type != SB_MINIMUM) {

	if (horizontal) {
            olgx_draw_choice_item(pInfo->ginfo, win, 0, sbw->scroll.offset,
                        	sbw->scroll.anchwidth, sbw->scroll.anchlen,
                        	(long)NULL, state |
                            	((sbw->scroll.opcode==ANCHOR_TOP)?
                                	OLGX_INVOKED : OLGX_NORMAL));

            olgx_draw_choice_item(pInfo->ginfo, win, 
				sbw->core.width - sbw->scroll.anchwidth,
				sbw->scroll.offset, 
                        	sbw->scroll.anchwidth, sbw->scroll.anchlen, 
                        	(long)NULL, state | 
                            	((sbw->scroll.opcode==ANCHOR_BOT)?
                                	OLGX_INVOKED : OLGX_NORMAL)); 

	    cableLen -= ((sbw->scroll.anchwidth + padLen) * 2);
	    cablePos = sbw->scroll.anchwidth + padLen;
	
	} else { /* Vertical */
            olgx_draw_choice_item(pInfo->ginfo, win, sbw->scroll.offset, 0,
                        	sbw->scroll.anchwidth, sbw->scroll.anchlen, 
                        	(long)NULL, state | 
                            	((sbw->scroll.opcode==ANCHOR_TOP)?
                                	OLGX_INVOKED : OLGX_NORMAL)); 
 
            olgx_draw_choice_item(pInfo->ginfo, win, sbw->scroll.offset,
                        	sbw->core.height - sbw->scroll.anchlen,
                        	sbw->scroll.anchwidth, sbw->scroll.anchlen,  
                        	(long)NULL, state |  
                            	((sbw->scroll.opcode==ANCHOR_BOT)? 
                                	OLGX_INVOKED : OLGX_NORMAL));
	    cableLen -= ((sbw->scroll.anchlen + padLen) * 2);
	    cablePos = sbw->scroll.anchlen + padLen;
	}
    }
    if (horizontal) {
	scrollbarX = 0;
	scrollbarY = (Position)sbw->scroll.offset;

    } else { /* Vertical */
	scrollbarX = (Position)sbw->scroll.offset;
   	scrollbarY = 0;
    }

    /* Draw the elevator */
    LocateElevator (sbw, cablePos, cableLen);

    /* Draw the cable and proportion indicator */
    elevPos = sbw->scroll.sliderPValue;
    indPos = sbw->scroll.indPos;
    indLen = sbw->scroll.indLen;

    if (sbw->scroll.type == SB_ABBREVIATED ||
                sbw->scroll.type == SB_MINIMUM) 
        state |= OLGX_ABBREV;

    if (sbw->scroll.sliderValue == sbw->scroll.sliderMin)
	state |= OLGX_SCROLL_NO_BACKWARD;
    else if (sbw->scroll.sliderValue == sbw->scroll.sliderMax)
	state |= OLGX_SCROLL_NO_FORWARD;
    
    olgx_draw_scrollbar(pInfo->ginfo, win, scrollbarX, scrollbarY, 
		scrollbarLen, elevPos, 0, indPos, indLen, state);

} /* DrawScrollbar */



/* call callback routine only if rel > granularity */
static void
MoveSlider(ScrollbarWidget sw, OlScrollbarVerify *olsb, Boolean callback, Boolean more)
{
        olsb->delta = olsb->new_location - sw->scroll.sliderValue;
        olsb->ok = TRUE;
	olsb->new_page = sw->scroll.currentPage;
	if (callback) {
               	olsb->slidermin = sw->scroll.sliderMin;
               	olsb->slidermax = sw->scroll.sliderMax;
               	olsb->more_cb_pending = more;
               	XtCallCallbacks((Widget)sw, XtNsliderMoved, (XtPointer) olsb);
	}

	/*
	 * Note that even if delta is zero, you still need to do the stuffs
	 * below. Maybe max, min, proportion length has changed.
	 */
        if (olsb->ok) {
		sw->scroll.currentPage = olsb->new_page;
		sw->scroll.sliderValue = olsb->new_location;
		if  (sw->scroll.type == SB_REGULAR)
			UpdateScrollbar(sw, sw->scroll.pAttrs,
					    SB_POSITION);

		/* update page indicator? */
		if ((sw->scroll.showPage != OL_NONE) &&
		    (sw->scroll.opcode == DRAG_ELEV))
			Update_page_ind(sw,PI_MOVE | PI_DRAW);
	}
} /* MoveSlider */

static void
Update_page_ind(ScrollbarWidget sw, int flags)
{
	char buff[16];
	wchar_t wbuff[16];
	int width, height, x, y;
	int textHeight;
	int w; /* width of page_ind window */
	int pad;
	XRectangle overall_ink, overall_logical;
	OlStrRep	tf = sw->primitive.text_format;
	XrmValue 	src,dest;
	OlStr		string;
	Screen		*scr = XtScreenOfObject((Widget)sw);

	/* page indicator is currently mapped */
	(void) snprintf(buff, 16, "%d", sw->scroll.currentPage);
	src.size = strlen(buff) + 1;
	src.addr = buff;
	dest.size = sizeof(OlStr);
	dest.addr = (XtPointer)&string;
	XtConvertAndStore((Widget)sw,XtRString,&src,XtROlStr,&dest);
	
	(*str_methods[tf].StrExtents)(sw->primitive.font,
				string, (*str_methods[tf].StrNumUnits)(
						string),&overall_ink,
						&overall_logical);
				
		width = overall_logical.width;
	        textHeight = overall_logical.height; 

	pad = (Dimension)OlgxScreenPointToPixel(OL_HORIZONTAL, 1, scr);
	w = width + pad * 12;
	height = textHeight + 
			(Dimension)OlgxScreenPointToPixel(OL_VERTICAL, 1, scr)*4;

	if ((flags & PI_MOVE)  || (SWB.page_ind->core.width != w)) {
		/* The next few lines assume page indicator only
		   works in vertical scrollbars */
		x = SWB.absx + SWB.offset + ((SWB.showPage == OL_LEFT) ? 
			-(w + pad*3 + 1) :
			(SWB.anchwidth + pad*3));
		y = SWB.absy + SWB.sliderPValue +
		    (int) (SWB.elevheight-height) / 2;

		XtConfigureWidget(sw->scroll.page_ind, x, y, w, height, 1);
	}
	if (flags & PI_DRAW) {
		XClearArea(XtDisplay(sw),
			       	XtWindow(sw->scroll.page_ind),
			       	0, 0,
			       	sw->scroll.page_ind->core.width,
			       	sw->scroll.page_ind->core.height,
				False);

		(*str_methods[tf].StrDraw)(XtDisplay(sw),
					XtWindow(sw->scroll.page_ind),
					sw->primitive.font,
					sw->scroll.textGC,
					(int)(SWB.page_ind->core.width 
								- width)/2,
					(int)(sw->scroll.page_ind->core.height
					 - textHeight)/2 +
						(-overall_logical.y),
					string,
					(*str_methods[tf].StrNumUnits)
								(string));
	}
} /* Update_page_ind */

/* ARGSUSED */
static void
ShowPageInd(Widget w, XtPointer data, XEvent *xevent, Boolean *cont_to_dispatch)
{
	ScrollbarWidget sw = (ScrollbarWidget)data;

	Update_page_ind(sw,PI_DRAW);
} /* ShowPageInd */


/* Define Scrollbar Rendering RATIOS as per OPEN LOOK Functional Spec. */
#define OL_SCALE_TO_H_ANCHOR_WDT_RATIO  2   /* Table B-20: row e (p.437) */

/*
 ************************************************************
 * SizeScrollbarAnchor: Return the width & height of the
 * scrollbar anchor (endbox).
 ************************************************************
 */
static void
SizeScrollbarAnchor(ScrollbarWidget sbw, OlgxAttrs *pInfo, 
                    Dimension *pWidth, Dimension *pHeight)
{
    Screen *scr = XtScreenOfObject((Widget)sbw);

    if (HORIZ(sbw)) {
        *pWidth=(Dimension)OlgxScreenPointToPixel(OL_HORIZONTAL,
                (sbw->primitive.scale / OL_SCALE_TO_H_ANCHOR_WDT_RATIO), scr);
        *pHeight= (Dimension)ScrollbarElevator_Width(pInfo->ginfo);

    } else  { /* Vertical */
        *pHeight=(Dimension)OlgxScreenPointToPixel(OL_VERTICAL,
                (sbw->primitive.scale / OL_SCALE_TO_H_ANCHOR_WDT_RATIO), scr);
        *pWidth= (Dimension)ScrollbarElevator_Width(pInfo->ginfo);

    } 

} /* SizeScrollbarAnchor */

/*
 **********************************************************
 * SizeScrollbarElevator: return the width & height of
 * the scrollbar elevator.
 **********************************************************
 */
static void
SizeScrollbarElevator (ScrollbarWidget sbw, OlgxAttrs *pInfo, 
		       unsigned int type, Dimension *pWidth,
		       Dimension *pHeight)
{
    Screen *scr = XtScreenOfObject((Widget)sbw);

    if (HORIZ(sbw)) {
	*pHeight = (Dimension)ScrollbarElevator_Width(pInfo->ginfo);

	if (type == SB_REGULAR)
	    *pWidth = (Dimension)ScrollbarElevator_Height(pInfo->ginfo);
	else
	    *pWidth = (Dimension)AbbScrollbar_Height(pInfo->ginfo);
    
    } else { /* Vertical */
	*pWidth = (Dimension)ScrollbarElevator_Width(pInfo->ginfo);

	if (type == SB_REGULAR)
	    *pHeight = (Dimension)ScrollbarElevator_Height(pInfo->ginfo);
	else
	    *pHeight = (Dimension)AbbScrollbar_Height(pInfo->ginfo);

    }
} /* SizeScrollbarElevator */


/*
 * This function calculates all the dimensions for a scrollbar.
 */
static void
Recalc(ScrollbarWidget sw)
{
       Dimension	elevWidth, elevHeight;
       Dimension	*elevLength;
       Dimension	anchorWidth, anchorHeight;
       int		length;
       int		anchorSize;

        /* calculates all dimensions */
        SizeScrollbarAnchor (sw, sw->scroll.pAttrs,
				&anchorWidth, &anchorHeight);
        sw->scroll.anchwidth = anchorWidth;
        sw->scroll.anchlen = anchorHeight;
	if (HORIZ (sw))
	{
	    length = sw->core.width;
	    anchorSize = sw->scroll.anchwidth * 2;
	    sw->scroll.offset = (Position) (sw->core.height -
					    sw->scroll.anchlen) / 2;
	    elevLength = &elevWidth;
	}
	else
	{
	    length = sw->core.height;
	    anchorSize = sw->scroll.anchlen * 2;
	    sw->scroll.offset = (Position) (sw->core.width -
					    sw->scroll.anchwidth) / 2;
	    elevLength = &elevHeight;
	}

	/*
	 * determines the type of scrollbar *
	 * regular, abbreviated, or minimum *
	 *
	 * abbreviated scrollbar has no dragbox.
	 *
	 * minimal scrollbar has no dragbox and anchors.
	 */
	sw->scroll.type = SB_REGULAR;
	SizeScrollbarElevator (sw, sw->scroll.pAttrs, SB_REGULAR,
			  &elevWidth, &elevHeight);

	if ((int)(anchorSize + *elevLength) <= (int) length) {
		/* regular elevator */
		if ((int) (anchorSize + *elevLength) == (int) length)
			/* minimum regular */
			sw->scroll.type = SB_MINREG;
	}
	else {
		sw->scroll.type = SB_ABBREVIATED;
		SizeScrollbarElevator(sw, sw->scroll.pAttrs,
				 SB_ABBREVIATED, &elevWidth, &elevHeight);
		if ((int)(anchorSize + *elevLength) > (int) length)
		{
			sw->scroll.type = SB_MINIMUM;
			SizeScrollbarElevator (sw, sw->scroll.pAttrs,
			      SB_MINIMUM, &elevWidth, &elevHeight);
		}
	}
       sw->scroll.elevwidth = elevWidth;
       sw->scroll.elevheight = elevHeight;
} /* Recalc */

static void
CheckValues(ScrollbarWidget sw)
{
	int range;
	int err = -1;
	int save;

        if (sw->scroll.orientation != OL_HORIZONTAL) {
		if (sw->scroll.orientation != OL_VERTICAL)
			SBError(0);
                sw->scroll.orientation = OL_VERTICAL;
		if ((sw->scroll.showPage != OL_NONE) &&
		    (sw->scroll.showPage != OL_LEFT) &&
		    (sw->scroll.showPage != OL_RIGHT)) {
			SBError(1);
			sw->scroll.showPage = OL_NONE;
		}
	}
	else {
		if (sw->scroll.showPage != OL_NONE) {
			SBError( 1);
			sw->scroll.showPage = OL_NONE;
		}
	}

       if (sw->scroll.sliderMin >= sw->scroll.sliderMax) {
		SBError( 2);
               sw->scroll.sliderMin = DFLT_MIN;
               sw->scroll.sliderMax = DFLT_MAX;
       }

	
       range = sw->scroll.sliderMax - sw->scroll.sliderMin;

	save = sw->scroll.proportionLength;
       sw->scroll.proportionLength = INRANGE(sw->scroll.proportionLength,1,range);
	if (save != sw->scroll.proportionLength)
		SBError( 5);
	
	save = sw->scroll.granularity;
       sw->scroll.granularity = INRANGE(sw->scroll.granularity,1,range);
	if (save != sw->scroll.granularity)
		SBError( 6);

	save = sw->scroll.sliderValue;
       sw->scroll.sliderValue = INRANGE(sw->scroll.sliderValue,
                                        sw->scroll.sliderMin,
                                        sw->scroll.sliderMax - sw->scroll.proportionLength);
	if (save != sw->scroll.sliderValue)
		SBError( 4);

       if (sw->primitive.scale < 1)
               sw->primitive.scale = 1;

       if (sw->scroll.repeatRate < 1) {
		SBError( 8);
               sw->scroll.repeatRate = DFLT_REPEATRATE;
	}
       if (sw->scroll.initialDelay < 1) {
		SBError( 7);
               sw->scroll.initialDelay = DFLT_INITIALDELAY;
	}

	if ((sw->scroll.dragtype != OL_GRANULARITY) &&
	    (sw->scroll.dragtype != OL_CONTINUOUS)  &&
	    (sw->scroll.dragtype != OL_RELEASE)) {
		SBError(9);
		sw->scroll.dragtype = OL_CONTINUOUS;
	}

	if ((sw->scroll.stoppos != OL_GRANULARITY) &&
	    (sw->scroll.stoppos != OL_ALL)) {
		SBError(10);
		sw->scroll.stoppos = OL_ALL;
	}
} /* CheckValues */

static void
SBError(int idx)
{
	static char *resources[] = {
		"orientation",
		"showPage",
		"sliderMin",
		"sliderMax",
		"sliderValue",
		"proportionLength",
		"granularity",
		"initialDelay",
		"repeatRate",
		"dragCBType",
		"stopPosition"
	};
	char *error;

	if (error = malloc(64)) {
		snprintf(error, 64, dgettext(OlMsgsDomain,
			"Scrollbar - Bad %1$s resource value, set to default"),
			resources[idx]);
		OlWarning(error);
		free(error);
	}
}

static void
highlight(ScrollbarWidget sw, int invert)
{
        int x,y;
	unsigned flag;
	unsigned char	save_opcode;

/* mouseless: save opcode, and then turn the KBD_OP bit off,
   recover it before return */
	save_opcode = SWB.opcode;
	SWB.opcode &= (~KBD_OP);

        switch(sw->scroll.opcode) {
        case ANCHOR_TOP:
                /* highlight top/left anchor */
	        flag = SB_BEGIN_ANCHOR | SB_PREV_ARROW;
		break;

        case ANCHOR_BOT:
                /* highlight bottom/right anchor */
		flag = SB_END_ANCHOR | SB_NEXT_ARROW;
                break;

        case PAGE_DEC:
        case PAGE_INC:
        case NOOP:
                /* do nothing */
		flag = 0;
                break;

        case GRAN_DEC:
                /* highlight top/left arrow */
		flag = SB_PREV_ARROW;
		break;

        case GRAN_INC:
                /* highlight bottom/right arrow */
		flag = SB_NEXT_ARROW;
		break;

        case DRAG_ELEV:
                /* highlight dragbox */
		flag = SB_DRAG;
		break;
        }

	if (!invert)
	    sw->scroll.opcode = NOOP;

	if (flag)
	    UpdateScrollbar (sw, sw->scroll.pAttrs, flag);

	/* show/unshow page indicator */
	if ((SWB.showPage != OL_NONE) && (flag == SB_DRAG)) {
		if (invert) {
			/* show page indicator */
			Update_page_ind(sw,PI_MOVE);
			XtPopup(sw->scroll.page_ind,XtGrabNone);
			Update_page_ind(sw,PI_DRAW);
		}
		else {
			/* unmap page indicator */
			XtPopdown(sw->scroll.page_ind);
		}
	}
	SWB.opcode = save_opcode;
}

static void
make_page_ind(ScrollbarWidget sw)
{
	static Arg args[3];

	XtSetArg(args[0], XtNwidth, 1);
	XtSetArg(args[1], XtNheight, 1);
	XtSetArg(args[2], XtNbackground, sw->core.background_pixel);
	if ((sw->scroll.page_ind = XtCreatePopupShell("PageInd",
		overrideShellWidgetClass, (Widget)sw, (ArgList)args, 3)) == 
		NULL) {
		OlWarning(dgettext(OlMsgsDomain,
			"Scrollbar: unable to create page indicator popup shell, set showPage to OL_NONE"));
		sw->scroll.showPage = OL_NONE;
		return;
	}

	/* add event handler */
	XtAddEventHandler(SWB.page_ind,ExposureMask, FALSE,
			  ShowPageInd, (XtPointer) sw);
	XtAddCallback(SWB.page_ind, XtNdestroyCallback,
		      (XtCallbackProc)RemoveHandler, (XtPointer)sw);
}

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */
/*
************************************************************
 *
 *  ClassInitialize - Register OlDefine string values.
 *********************procedure*header************************
 */
static void
ClassInitialize(void)
{
	_OlAddOlDefineType ("horizontal",  OL_HORIZONTAL);
	_OlAddOlDefineType ("vertical",    OL_VERTICAL);
	_OlAddOlDefineType ("left",        OL_LEFT);
	_OlAddOlDefineType ("right",       OL_RIGHT);
	_OlAddOlDefineType ("continuous",  OL_CONTINUOUS);
	_OlAddOlDefineType ("granularity", OL_GRANULARITY);
	_OlAddOlDefineType ("release",     OL_RELEASE);
	_OlAddOlDefineType ("all",         OL_ALL);
	_OlAddOlDefineType ("none",        OL_NONE);
} /* END OF ClassInitialize */

/*
 *************************************************************************
 * GetValuesHook - gets subresource data
 ****************************procedure*header*****************************
 */
static void
GetValuesHook(Widget w, ArgList args, Cardinal *num_args)
                                /* menu shell widget id         */
                                /* Arg List for Menu            */
                                /* number of Args               */
{
        ReturnPaneId(w, args, num_args);
} /* END OF GetValuesHook() */

/*
 *************************************************************************
 *  Initialize
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
                             /* what the client asked for */
                             /* what we're going to give him */
       		     
          	         
{
        ScrollbarWidget sw = (ScrollbarWidget) new;
	ScrollbarPart *swp = &(sw->scroll);
	Dimension	dtmp1, dtmp2;
	XrmValue src,dest;
	int i;

        sw->scroll.opcode   = NOOP;
/* new fields for mouseless operation */
	THEBUTTON1	    = NULL;
	THEBUTTON2	    = NULL;


	InitializeLabels(request, new);
        /* check for valid values */
        CheckValues(sw);

        sw->scroll.timerid   = (XtIntervalId)NULL;
        sw->scroll.popup     = NULL;
        sw->scroll.pane      = NULL;
        sw->scroll.page_ind  = NULL;
        sw->scroll.textGC  = NULL;
        sw->scroll.pAttrs  = NULL;

        sw->scroll.previous = sw->scroll.sliderValue;

	GetGCs (sw);
        SizeScrollbarAnchor (sw, sw->scroll.pAttrs, &dtmp1, &dtmp2);
	sw->scroll.anchwidth = dtmp1;
	sw->scroll.anchlen   = dtmp2;

	if (sw->core.height == 0 || sw->core.width == 0)
	{
	    sw->scroll.type = SB_ABBREVIATED;
	    SizeScrollbarElevator (sw, sw->scroll.pAttrs, SB_ABBREVIATED,
				&dtmp1, &dtmp2);
	    sw->scroll.elevwidth  = dtmp1;
	    sw->scroll.elevheight = dtmp2;
	    if (sw->core.height == 0)
	    {
		if (HORIZ(sw))
		    sw->core.height = sw->scroll.anchlen +
			(Dimension)OlgxScreenPointToPixel(OL_VERTICAL,1,sw->core.screen)*2;
		else
		    sw->core.height = sw->scroll.anchlen * 2 +
			sw->scroll.elevheight;
	    }

	    if (sw->core.width == 0)
	    {
		if (HORIZ (sw))
		    sw->core.width = sw->scroll.anchwidth * 2 +
			sw->scroll.elevheight;
		else
		    sw->core.width = SWB.anchwidth +
			(Dimension)OlgxScreenPointToPixel(OL_HORIZONTAL, 1, sw->core.screen)*2;
	    }
	}

	/* override Core widget and set  borderWidth to 0 and inherit
	 * parent's background.
	 */
	sw->core.border_width = 0;
	sw->core.background_pixmap = ParentRelative;

	/* don't bother checking for XtNfontColor as a dynamic resource */
	sw->primitive.dyn_flags |= OL_B_PRIMITIVE_FONTCOLOR;

}   /* Initialize */

static void
Destroy(Widget w)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;
	ScrollbarPart *swp = &(sw->scroll);

	if (sw->scroll.textGC)
	    XtDestroyGC(sw->scroll.textGC);

	if (sw->scroll.pAttrs)
	    OlgxDestroyAttrs (w, sw->scroll.pAttrs);
        if (sw->scroll.timerid) {
                XtRemoveTimeOut(sw->scroll.timerid);
                sw->scroll.timerid = (XtIntervalId)NULL;
        }

	if (swp->hereToTopLabel)
		XtFree(swp->hereToTopLabel);
	if (swp->topToHereLabel)
		XtFree(swp->topToHereLabel);
	if (swp->hereToLeftLabel)
		XtFree(swp->hereToLeftLabel);
	if (swp->leftToHereLabel)
		XtFree(swp->leftToHereLabel);
	if (swp->previousLabel)
		XtFree(swp->previousLabel);

} /* Destroy */

static void
RemoveHandler(Widget w, XtPointer client_data, XtPointer call_data)
{
	XtRemoveEventHandler(w, ExposureMask, FALSE, ShowPageInd, call_data);
}

/*
 * Redisplay
 */
static void
Redisplay(Widget w, XEvent *event, Region region)
{
    ScrollbarWidget	sw = (ScrollbarWidget) w;

    DrawScrollbar(sw, sw->scroll.pAttrs);
}

/*
 *************************************************************************
 * Resize - Reconfigures all the subwidgets to a new size.
 *          Must also recalulate size and position of indicator.
 ****************************procedure*header*****************************
 */
static void
Resize(Widget w)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;

        Recalc(sw);
} /* Resize */

/*
 *************************************************************************
 *  Realize - Creates the WIndow, and Creates the racing stripe.
 *      The racing stripe cannot be created any earlier, because the
 *      pixmap is attached to the window's background, thus window must
 *      have been created first.
 ****************************procedure*header*****************************
 */
static void
Realize(Widget w, Mask *valueMask, XSetWindowAttributes *attributes)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;

        XtCreateWindow((Widget)sw, InputOutput, (Visual *)CopyFromParent,
            *valueMask, attributes );

	XDefineCursor(XtDisplay(sw), XtWindow(sw),
		      OlGetStandardCursor((Widget)sw));

        /* draw the scrollbar stuffs in window */
        Recalc(sw);

	if (sw->scroll.showPage != OL_NONE)
		make_page_ind(sw);
} /* Realize */

/*
 *************************************************************************
 * QueryGeom - Important routine for parents that want to know how
 *              large the scrollbar wants to be. the thickness  (width
 *              for ver. scrollbar) and length (height for ver. scrollbar)
 *              should be honored by the parent, otherwise an ugly
 *              visual will occur.
 ****************************procedure*header*****************************
 */
static XtGeometryResult
QueryGeom(Widget w, XtWidgetGeometry *intended, XtWidgetGeometry *reply)
         
                            /* parent's changes; may be NULL */
                            /* child's preferred geometry; never NULL */
{
	ScrollbarWidget sw = (ScrollbarWidget)w;
        XtGeometryResult result;
	Dimension height;
	Dimension width;
	Dimension replylen;

	/* start with the same values */
	*reply = *intended;

	/* assume ok */
	result = XtGeometryYes;

	/* border width has to be zero */
	reply->border_width = 0;
	if (intended->request_mode & CWBorderWidth &&
	    intended->border_width != 0)
		result = XtGeometryAlmost;

	/* X, Y, Sibling, and StackMode are always ok */

	if (intended->request_mode & (CWWidth | CWHeight))
	{
	    SizeScrollbarElevator (sw, sw->scroll.pAttrs, SB_REGULAR,
				      &width, &height);

	    /* here checks the width */
	    if (intended->request_mode & CWWidth)
	    {
		if (HORIZ(sw))
		{
		    width += sw->scroll.anchwidth * 2;
		    if (intended->width < width)
		    {
			SizeScrollbarElevator (sw, sw->scroll.pAttrs,
				SB_ABBREVIATED, &width, &height);
			width += sw->scroll.anchwidth * 2;
			if (intended->width < width)
			    SizeScrollbarElevator (sw, sw->scroll.pAttrs,
				    SB_MINIMUM, &width, &height);
			if (intended->width != width)
			{
			    result = XtGeometryAlmost;
			    reply->width = width;
			}
		    }
		}
		else
		{
		    width += (Dimension)OlgxScreenPointToPixel(OL_HORIZONTAL,1,
				XtScreenOfObject((Widget)sbw)) * 2;
		    if (intended->width != width)
		    {
			result = XtGeometryAlmost;
			reply->width = width;
		    }
		}
	    }

	    /* here checks the height */
	    if (intended->request_mode & CWHeight)
	    {
		if (HORIZ(sw))
		{
		    height += (Dimension)OlgxScreenPointToPixel(OL_VERTICAL, 1,
                                XtScreenOfObject((Widget)sbw)) * 2;
		    if (intended->height != height)
		    {
			result = XtGeometryAlmost;
			reply->height = height;
		    }
		}
		else
		{
		    height += sw->scroll.anchlen * 2;
		    if (intended->height < height)
		    {
			SizeScrollbarElevator (sw, sw->scroll.pAttrs,
				SB_ABBREVIATED, &width, &height);
			height += sw->scroll.anchlen * 2;
			if (intended->height < height)
			    SizeScrollbarElevator (sw, sw->scroll.pAttrs,
				    SB_MINIMUM, &width, &height);
			if (intended->height != height)
			{
			    result = XtGeometryAlmost;
			    reply->height = height;
			}
		    }
		}
	    }
	}
	return (result);
}			/* QueryGeom */

/*
 ************************************************************
 *
 *  SetValues - This function compares the requested values
 *      to the current values, and sets them in the new
 *      widget.  It returns TRUE when the widget must be
 *      redisplayed.
 *
 *********************procedure*header************************
 */
/* ARGSUSED */
static Boolean
SetValues (Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
        ScrollbarWidget sw = (ScrollbarWidget)current;
        ScrollbarWidget newsw = (ScrollbarWidget)new;
        OlScrollbarVerify olsb;
        int moved;
        Boolean needs_redisplay = FALSE;

        /* cannot change orientation on the fly */
        newsw->scroll.orientation = sw->scroll.orientation;

        CheckValues(newsw);

	if (XtIsSensitive(new) != XtIsSensitive(current))
		needs_redisplay = True;

	/* Cannot set currentPage except in the next case */
	newsw->scroll.currentPage = sw->scroll.currentPage;

       if ((newsw->scroll.sliderMin != sw->scroll.sliderMin) ||
           (newsw->scroll.sliderMax != sw->scroll.sliderMax) ||
           (newsw->scroll.proportionLength != sw->scroll.proportionLength) ||
           (newsw->scroll.sliderValue != sw->scroll.sliderValue)) {
               olsb.new_location = newsw->scroll.sliderValue;
               olsb.new_page	 = newsw->scroll.currentPage;
               moved = 1;
       }
	else
		moved = 0;

	if (newsw->scroll.showPage != sw->scroll.showPage) {
		if (newsw->scroll.showPage == OL_NONE) {
			if (newsw->scroll.page_ind)
				XtDestroyWidget((Widget)newsw->scroll.page_ind);
		}
		else {
			if (!(newsw->scroll.page_ind) && 
					XtIsRealized((Widget)sw))
				make_page_ind(newsw);
		}
	}

	if ((newsw->primitive.foreground != sw->primitive.foreground) ||
	    (newsw->core.background_pixel != sw->core.background_pixel)) {
	       GetGCs (newsw);
               needs_redisplay = TRUE;
       }

	/* The scrollbar should always inherit its parents background for
	 * the window.  If anyone has played with this, set it back.
	 */
	if (XtIsRealized((Widget)sw) &&
	    newsw->core.background_pixmap != ParentRelative)
	{
	    newsw->core.background_pixmap = ParentRelative;
	    XSetWindowBackgroundPixmap(XtDisplay (sw), XtWindow (sw),
					ParentRelative);
	    needs_redisplay = TRUE;
	}

	/*
	 * Call the XtNsliderMoved callback if
	 * useSetValCallback is True,
	 * the slider's value or current page has changed,
	 * and the widget is realized
	 */
	if ((sw->scroll.useSetValCallback == TRUE) &&
	    ((newsw->scroll.sliderValue != sw->scroll.sliderValue) ||
	    (newsw->scroll.currentPage != sw->scroll.currentPage)) &&
	    XtIsRealized((Widget)sw)) {
		newsw->scroll.sliderValue = sw->scroll.sliderValue;
       		MoveSlider(newsw,&olsb,TRUE,FALSE);
	} else
       if (moved && (needs_redisplay == FALSE) && XtIsRealized((Widget)sw)) {
		newsw->scroll.sliderValue = sw->scroll.sliderValue;
       		MoveSlider(newsw,&olsb,FALSE,FALSE);
	}
        return (needs_redisplay);
}       /* SetValues */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * SelectDown - Callback for Btn Select Down inside Scrollbar window, but
 *              not in any children widgets.
 ****************************procedure*header*****************************
 */
static void
SelectDown(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;
       int point;
       int bottomAnchorPos, topAnchorPos, elevEndPos;
       int opcode = NOOP;
	int	partSize;


	/* Throw out events not directly over the scrollbar */
	if (HORIZ (sw))
	{
	    if (event->xbutton.y < sw->scroll.offset ||
		event->xbutton.y >= sw->scroll.offset +
		    (Position) sw->scroll.anchlen)
		return;
	    point = event->xbutton.x;
	    topAnchorPos = sw->scroll.anchwidth;
	    bottomAnchorPos = sw->core.width - sw->scroll.anchwidth;
	    elevEndPos = sw->scroll.elevwidth;
	    partSize = (int) (sw->scroll.elevwidth+1) /
		(int) (sw->scroll.type&EPARTMASK);
	}
	else
	{
	    if (event->xbutton.x < sw->scroll.offset ||
		event->xbutton.x >= sw->scroll.offset +
		    (Position) sw->scroll.anchwidth)
		return;
	    point = event->xbutton.y;
	    topAnchorPos = sw->scroll.anchlen;
	    bottomAnchorPos = sw->core.height - sw->scroll.anchlen;
	    elevEndPos = sw->scroll.elevheight;
	    partSize = (int) (sw->scroll.elevheight+1) /
		(int) (sw->scroll.type&EPARTMASK);
	}

	if (sw->scroll.type != SB_MINIMUM) {
       		if (point < topAnchorPos)
               		opcode = ANCHOR_TOP;
       		else if (point >= bottomAnchorPos)
               		opcode = ANCHOR_BOT;
		else
               		/* all subsequent checks are relative to elevator */
               		point -= sw->scroll.sliderPValue;
	}

	if (opcode == NOOP) {
               if (point < 0)
                       opcode = PAGE_DEC;
               else if (point < partSize)
                       opcode = GRAN_DEC;
               else if (point >= elevEndPos)
                       opcode = PAGE_INC;
	       else if (point < elevEndPos - partSize)
               	       opcode = DRAG_ELEV;
	       else
		       opcode = GRAN_INC;
       }

	/* if need to show page indicator, get the abs coord of the scrollbar
	   to be used by Update_page_ind().  */
	if ((sw->scroll.showPage != OL_NONE) && (opcode == DRAG_ELEV)) {
		sw->scroll.absx = event->xbutton.x_root - event->xbutton.x;
		sw->scroll.absy = event->xbutton.y_root - event->xbutton.y;
	}

	sw->scroll.opcode = (unsigned char)opcode;
        highlight(sw,TRUE);

	/* save current position */
	sw->scroll.previous = sw->scroll.sliderValue;

       if (opcode == DRAG_ELEV) {
		if (sw->scroll.type != SB_REGULAR) {
			/* if not regular scrollbar, cannot drag */
			highlight(sw,FALSE);
			return;
		}
		else {
			/* record pointer based pos. for dragging */
			sw->scroll.dragbase = SWB.sliderPValue - (HORIZ(sw) ?
				   event->xbutton.x : event->xbutton.y) -
				   topAnchorPos;
		}
	}

	if ((opcode != ANCHOR_TOP) && (opcode != ANCHOR_BOT))
        	TimerEvent((XtPointer)sw, (XtIntervalId*)NULL);
}       /* SelectDown */

/*
 *************************************************************************
 * SelectUp - Callback when Select Button is released.
 ****************************procedure*header*****************************
 */
static void
SelectUp(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;

	if (sw->scroll.opcode != NOOP) {
        	/* unhighlight */
        	highlight(sw,FALSE);

        	if (sw->scroll.timerid) {
                	XtRemoveTimeOut (sw->scroll.timerid);
                	sw->scroll.timerid = (XtIntervalId)NULL;
        	}

		if (sw->scroll.opcode & ANCHOR) {
			 OlScrollbarVerify olsb;
			 int x = event->xbutton.x; 
			 int y = event->xbutton.y; 
	
			 olsb.new_location = (SWB.opcode & DIR_INC) ?
			  	SWB.sliderMax - SWB.proportionLength :
			  	sw->scroll.sliderMin;
			if ((olsb.new_location != sw->scroll.sliderValue)&&
			    (inAnchor(sw,x,y,sw->scroll.opcode))) {
				 MoveSlider(sw, &olsb, True, False);
			}
		}

		if (SWB.opcode == DRAG_ELEV) {
        		OlScrollbarVerify olsb;

        		sw->scroll.opcode = NOOP;
			if (SWB.stoppos == OL_GRANULARITY) {
			
				olsb.new_location = SWB.sliderMin +
					 (SWB.sliderValue - SWB.sliderMin +
					 SWB.granularity / 2) /
					 SWB.granularity * SWB.granularity;
				olsb.new_location = INRANGE(olsb.new_location,
                                       	SWB.sliderMin,
                                       	SWB.sliderMax - SWB.proportionLength);
			}
			else
				olsb.new_location = sw->scroll.sliderValue;
			MoveSlider(sw, &olsb, TRUE, FALSE);
		}
       		sw->scroll.opcode = NOOP;
	}
} /* SelectUp */

static void
Menu(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;
       if ((SWB.timerid == NULL) && 
		((SWB.opcode == NOOP) || !(SWB.opcode & ANCHOR))) {
               sw->scroll.XorY = HORIZ(sw)? event->xbutton.x : event->xbutton.y;
               if (sw->scroll.popup == NULL)
                       MakeMenu(sw);
		else
			ChangeDimState (w, True);
               OlMenuPost(sw->scroll.popup);
       }
} /* Menu */

/* this routine is only called by Menu() and MenuKeyPress().
   in the mouseless mode, the first two menu buttons on the scrollbar
   menu should be dimed. As a result, when it operates on the mouse
   operation, the dim state needs to be reversed
 */

static void
ChangeDimState(Widget w, Boolean state)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;

	XtSetSensitive(THEBUTTON1, state);
	XtSetSensitive(THEBUTTON2, state);

	return;
} /* ChangeDimState */

static Boolean
MenuKeyPress(Widget w)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;

	if ((SWB.timerid == NULL) && 
		((SWB.opcode == NOOP) || !(SWB.opcode & ANCHOR)))
	{
		Position x,y;

		if (SWB.popup == NULL)
			MakeMenu(sw);
		ChangeDimState(w, False);

		x = SWB.offset + SWB.anchwidth;
		y = SWB.sliderPValue;
		if (HORIZ(sw)) {
			Position tmp;

			tmp = x; x = y; y = tmp;
		}
		XtTranslateCoords(w, x, y, &x, &y);
		OlMenuPopup((Widget)SWB.popup, w, (Cardinal)OL_NO_ITEM, 
				(OlDefine)OL_STAYUP_MENU, (Boolean)TRUE, 
				(Position)x, (Position)y, 
				(OlMenuPositionProc)NULL);
	}
	return (True);
} /* MenuKeyPress */

static Boolean
ScrollKeyPress(Widget w, unsigned char opcode)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;

	SWB.opcode = opcode | KBD_OP;
	highlight (sw, True);
	SWB.previous = SWB.sliderValue;

	TimerEvent((XtPointer)sw, (XtIntervalId*)NULL);

	highlight (sw, False);
	SWB.opcode = NOOP;
	return (True);
} /* ScrollKeyPress */

static Boolean
SBActivateWidget (
Widget		w,
OlVirtualName	activation_type,
XtPointer	call_data)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;
	Boolean		consumed = False;

	if (SWB.orientation == (OlDefine)OL_HORIZONTAL)
	{
	  switch (activation_type)
	  {
		case OL_MENUKEY:
		case OL_HSBMENU:
			consumed = MenuKeyPress(w);
			break;
		case OL_PAGELEFT:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_DEC);
			break;
		case OL_PAGERIGHT:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_INC);
			break;
		case OL_SCROLLLEFT:
			consumed = ScrollKeyPress(w, (unsigned char)GRAN_DEC);
			break;
		case OL_SCROLLRIGHT:
			consumed = ScrollKeyPress(w, (unsigned char)GRAN_INC);
			break;
		case OL_SCROLLLEFTEDGE:
			consumed = ScrollKeyPress(w, (unsigned char)ANCHOR_TOP);
			break;
		case OL_SCROLLRIGHTEDGE:
			consumed = ScrollKeyPress(w, (unsigned char)ANCHOR_BOT);
			break;
	  }
	}
	else			/* OL_VERTICAL */
	{
	  switch (activation_type)
	  {
		case OL_MENUKEY:
		case OL_VSBMENU:
			consumed = MenuKeyPress(w);
			break;
		case OL_PAGEUP:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_DEC);
			break;
		case OL_PAGEDOWN:
			consumed = ScrollKeyPress(w, (unsigned char)PAGE_INC);
			break;
		case OL_SCROLLUP:
			consumed = ScrollKeyPress(w, (unsigned char)GRAN_DEC);
			break;
		case OL_SCROLLDOWN:
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
	return (consumed);
} /* SBActivateWidget */

static void
SBButtonHandler(Widget w, OlVirtualEvent ve)
{
	switch (ve->virtual_name)
	{
		case OL_SELECT:
			ve->consumed = True;
			if (ve->xevent->type == ButtonPress)
				SelectDown(w, ve->xevent, (String*)NULL, 
					(Cardinal*)NULL);
			else
				SelectUp(w, ve->xevent, (String*)NULL, 
					(Cardinal*)NULL);
			break;
		case OL_MENU:
			ve->consumed = True;
			Menu(w, ve->xevent, (String*)NULL, (Cardinal*)NULL);
			break;
		default:
			if (ve->xevent->type == ButtonRelease)
				SelectUp(w, ve->xevent, (String*)NULL, 
					(Cardinal*)NULL);
	}
} /* SBButtonHandler */

/*
 * InAnchor() - says whether the current (x,y) position in within the
 *		the specified anchor-boxes
 */
static 
inAnchor(ScrollbarWidget sw, int x, int y, int anchor)
                   
         
           	/* ANCHOR_TOP or ANCHOR_BOT ? */
{
	/* Minimum scrollbar does'nt have anchors ... */
	if (sw->scroll.type == SB_MINIMUM)
		return False;
	if (HORIZ(sw)) {
		if (y < sw->scroll.offset ||
		    y >= (Position)
			(sw->scroll.offset + sw->scroll.anchlen))
			return False;
		if (x < 0 || x >= (int)sw->core.width)
			return False;
		if ((anchor == ANCHOR_TOP) &&
		    (x < (int)sw->scroll.anchwidth))
			return True;
		if ((anchor == ANCHOR_BOT) &&
		    (x >= (int)(sw->core.width - sw->scroll.anchwidth)))
			return True;
		return False;
	}
	else {
		if (x < sw->scroll.offset || 
		    x >= (Position) 
			(sw->scroll.offset + sw->scroll.anchwidth))
			return False;
		if (y < 0 || y >= (int)(sw->core.height))
			return False;
		if ((anchor == ANCHOR_TOP) && 
		    (y < (int)sw->scroll.anchlen))
			return True;
		if ((anchor == ANCHOR_BOT) &&
		    (y >= (int)(sw->core.height - sw->scroll.anchlen)))
			return True;
		return False;
	}
}


static void
InitializeLabels(Widget request, Widget new)
{
	ScrollbarWidget sbw = (ScrollbarWidget)new;
	OlStrRep tf = sbw->primitive.text_format;
	OlStr (*StrCpy)(OlStr s1, OlStr s2) = 
					str_methods[tf].StrCpy;
	int (*StrNumBytes)(OlStr s) =
					str_methods[tf].StrNumBytes;
	ScrollbarPart *swp = &(sbw->scroll);

	swp->hereToTopLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					swp->hereToTopLabel) ),
					swp->hereToTopLabel);
	swp->topToHereLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					swp->topToHereLabel) ),
					swp->topToHereLabel);
	swp->hereToLeftLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					swp->hereToLeftLabel) ),
					swp->hereToLeftLabel);
	swp->leftToHereLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					swp->leftToHereLabel) ),
					swp->leftToHereLabel);
	swp->previousLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					swp->previousLabel) ),
					swp->previousLabel);
}
