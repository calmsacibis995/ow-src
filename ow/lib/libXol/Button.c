#pragma ident   "@(#)Button.c	302.25    97/03/26 lib/libXol SMI"     /* button:src/Button.c 1.33.1.64*/

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

/*
 ************************************************************
 *
 *  Description:
 *	This file contains the source for the OPEN LOOK(tm)
 *	Button widget and gadget.
 *
 ************************************************************
 */


#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>
#include <widec.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/Font.h>
#include <Xol/ButtonP.h>
#include <Xol/MenuButton.h>
#include <Xol/Menu.h>
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

typedef union {
    OlgxTextLbl		text;
    OlgxPixmapLbl	image;
    Widget		proc_widget;
} ButtonLabel;

/* private procedures */
static Boolean          IsMenuMode(ButtonWidget bw);
static int		getOblongFlags(Widget bw);
static int	        getRectFlags(Widget bw);
static void		SetDimensions(ButtonWidget bw);
static void             GetGCs(ButtonWidget w);
static void             GetInverseTextGC(ButtonWidget bw);
static void             GetNormalGC(ButtonWidget bw);
static void             OblongRedisplay(Widget w, XEvent *event, Region region);
static void             RectRedisplay(Widget w, XEvent *event, Region region);
static void             SetLabelTile(ButtonWidget bw);

static void             drawProcButton(Screen *scr, Drawable win,
	OlgxAttrs *pAttrs, Position x, Position y, Dimension buttonWidth,
	Dimension buttonHeight, XtPointer labeldata, OlDefine buttonType,
	int flags);

static void             setLabel(ButtonWidget bw, ButtonLabel *lbl, 
	OlSizeProc *sizeProc, OlDrawProc *drawProc);

static void             sizeProcLabel(Screen *scr, OlgxAttrs *pAttrs,
	XtPointer labeldata, Dimension *pWidth, Dimension *pHeight);

/* class procedures */
static void             ButtonDestroy(Widget w);

static void             ButtonInitialize(Widget request, Widget new, 
ArgList args, Cardinal *num_args);

static void             ButtonRedisplay(Widget w, XEvent *event, Region region);

static void             ButtonRealize(Widget w, XtValueMask *valueMask,
	XSetWindowAttributes *attributes);

static void             ClassInitialize(void);

static Boolean          SetValues(Widget current, Widget request, Widget new,
	ArgList args, Cardinal *num_args);

static void             HighlightHandler(Widget w, OlDefine highlight_type);



/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

static int              defShellBehavior = (int) BaseWindow;
static int              defScale = 12;
static Boolean          true = (Boolean) TRUE;
static Boolean          false = (Boolean) FALSE;


/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************
 */


/*  The Button widget does not have translations or actions  */

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

#define offset(field) XtOffset(ButtonWidget, field)

static XtResource resources[] = { 

	{XtNbuttonType,
		XtCButtonType,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.button_type),
		XtRImmediate,
		(XtPointer) ((OlDefine) OL_LABEL) },

	{XtNlabelType,
		XtCLabelType,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.label_type),
		XtRImmediate,
		(XtPointer) ((OlDefine) OL_STRING) },

	{XtNlabelJustify,
		XtCLabelJustify,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.label_justify),
		XtRImmediate,
		(XtPointer)((OlDefine) OL_LEFT) },

	{XtNmenuMark,
		XtCMenuMark,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.menumark),
		XtRImmediate,
		(XtPointer)((OlDefine) OL_DOWN) },

	{XtNlabel,
		XtCLabel,
		XtROlStr,
		sizeof(OlStr),
		offset(button.label),
		XtRImmediate,
		(XtPointer)NULL},

	{XtNlabelImage,
		XtCLabelImage,
		XtRPointer,
		sizeof(XImage *),
		offset(button.label_image),
		XtRPointer,
		(XtPointer)NULL},

	{XtNlabelTile,
		XtCLabelTile,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.label_tile),
		XtRBoolean,
		(XtPointer) &false},

	{XtNdefault,
		XtCDefault,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.is_default),
		XtRBoolean,
		(XtPointer) &false},

	{XtNselect,
		XtCCallback,
		XtRCallback,
		sizeof(XtPointer), 
		offset(button.select),
		XtRCallback,
		(XtPointer) NULL},

	{XtNunselect,
		XtCCallback,
		XtRCallback,
		sizeof(XtPointer), 
		offset(button.unselect),
		XtRCallback,
		(XtPointer) NULL},

	{XtNset,
		XtCSet,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.set),
		XtRBoolean,
		(XtPointer) &false},

	{XtNdim,
		XtCDim,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.dim),
		XtRBoolean,
		(XtPointer) &false},

	{XtNbusy,
		XtCBusy,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.busy),
		XtRBoolean,
		(XtPointer) &false},

	{XtNrecomputeSize,
		XtCRecomputeSize,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.recompute_size),
		XtRBoolean,
		(XtPointer) &true},

	{XtNbackground,
		XtCBackground,
		XtRPixel,
		sizeof(Pixel),
		offset(button.background_pixel),
		XtRString,
		(XtPointer)XtDefaultBackground },

	{XtNpreview,
		XtCPreview,
		XtRPointer,
		sizeof(Widget),
		offset(button.preview),
		XtRPointer,
		(XtPointer) NULL},

	{XtNshellBehavior,
		XtCShellBehavior,
		XtRInt,
		sizeof(int),
		offset(button.shell_behavior),
		XtRInt,
		(XtPointer) &defShellBehavior},

	{XtNpostSelect,
		XtCCallback,
		XtRCallback,
		sizeof(XtPointer), 
		offset(button.post_select),
		XtRCallback,
		(XtPointer) NULL},

	{XtNlabelProc,
		XtCCallback,
		XtRCallback,
		sizeof (XtPointer),
		offset (button.label_proc),
		XtRCallback,
		(XtPointer) NULL},

	{XtNmenuHasMeta,
		XtCMenuHasMeta,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.menu_has_meta),
		XtRBoolean,
		(XtPointer) &false},
};
#undef offset

#define offset(field) XtOffset(ButtonGadget, field)

static XtResource gadget_resources[] = {

	{XtNbuttonType,
		XtCButtonType,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.button_type),
		XtRImmediate,
		(XtPointer) ((OlDefine) OL_LABEL) },

	{XtNlabelType,
		XtCLabelType,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.label_type),
		XtRImmediate,
		(XtPointer) ((OlDefine) OL_STRING) },

	{XtNlabelJustify,
		XtCLabelJustify,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.label_justify),
		XtRImmediate,
		(XtPointer)((OlDefine) OL_LEFT) },

	{XtNmenuMark,
		XtCMenuMark,
		XtROlDefine,
		sizeof(OlDefine),
		offset(button.menumark),
		XtRImmediate,
		(XtPointer)((OlDefine) OL_DOWN) },

	{XtNlabel,
		XtCLabel,
		XtROlStr,
		sizeof(OlStr),
		offset(button.label),
		XtRImmediate,
		(XtPointer)NULL},

	{XtNlabelImage,
		XtCLabelImage,
		XtRPointer,
		sizeof(XImage *),
		offset(button.label_image),
		XtRPointer,
		(XtPointer)NULL},

	{XtNlabelTile,
		XtCLabelTile,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.label_tile),
		XtRBoolean,
		(XtPointer) &false},

	{XtNdefault,
		XtCDefault,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.is_default),
		XtRBoolean,
		(XtPointer) &false},

	{XtNselect,
		XtCCallback,
		XtRCallback,
		sizeof(XtPointer), 
		offset(button.select),
		XtRCallback,
		(XtPointer) NULL},

	{XtNunselect,
		XtCCallback,
		XtRCallback,
		sizeof(XtPointer), 
		offset(button.unselect),
		XtRCallback,
		(XtPointer) NULL},

	{XtNset,
		XtCSet,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.set),
		XtRBoolean,
		(XtPointer) &false},

	{XtNdim,
		XtCDim,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.dim),
		XtRBoolean,
		(XtPointer) &false},

	{XtNbusy,
		XtCBusy,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.busy),
		XtRBoolean,
		(XtPointer) &false},

	{XtNrecomputeSize,
		XtCRecomputeSize,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.recompute_size),
		XtRBoolean,
		(XtPointer) &true},

	{XtNbackground,
		XtCBackground,
		XtRPixel,
		sizeof(Pixel),
		offset(button.background_pixel),
		XtRString,
		(XtPointer)XtDefaultBackground },

	{XtNpreview,
		XtCPreview,
		XtRPointer,
		sizeof(Widget),
		offset(button.preview),
		XtRPointer,
		(XtPointer) NULL},

	{XtNshellBehavior,
		XtCShellBehavior,
		XtRInt,
		sizeof(int),
		offset(button.shell_behavior),
		XtRInt,
		(XtPointer) &defShellBehavior},

	{XtNpostSelect,
		XtCCallback,
		XtRCallback,
		sizeof(XtPointer), 
		offset(button.post_select),
		XtRCallback,
		(XtPointer) NULL},

	{XtNlabelProc,
		XtCCallback,
		XtRCallback,
		sizeof (XtPointer),
		offset (button.label_proc),
		XtRCallback,
		(XtPointer) NULL},

	{XtNmenuHasMeta,
		XtCMenuHasMeta,
		XtRBoolean,
		sizeof(Boolean),
		offset(button.menu_has_meta),
		XtRBoolean,
		(XtPointer) &false},
};
#undef offset


/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

ButtonClassRec buttonClassRec = {
  {
    (WidgetClass) &(primitiveClassRec),	/* superclass		  */	
    "Button",				/* class_name		  */
    sizeof(ButtonRec),			/* size			  */
    ClassInitialize,			/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    ButtonInitialize,			/* initialize		  */
    NULL,				/* initialize_hook	  */
    ButtonRealize,			/* realize		  */
    NULL,				/* actions		  */
    0,					/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* resource_count	  */
    NULLQUARK,				/* xrm_class		  */
    FALSE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    FALSE,				/* visible_interest	  */
    ButtonDestroy,			/* destroy		  */
    NULL,				/* resize		  */
    ButtonRedisplay,			/* expose		  */
    SetValues,				/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    XtInheritAcceptFocus,		/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    XtInheritTranslations,		/* tm_table		  */
    NULL,				/* query_geometry	  */
  },  /* CoreClass fields initialization */
  {
    NULL,				/* reserved		*/
    HighlightHandler,			/* highlight_handler	*/
    NULL,				/* traversal_handler	*/
    NULL,				/* register_focus	*/
    NULL,				/* activate		*/
    NULL,				/* event_procs		*/
    0,					/* num_event_procs	*/
    OlVersion,				/* version		*/
    NULL,				/* extension		*/
    { NULL, 0 },			/* dyn_data		*/
    _OlDefaultTransparentProc,		/* transparent_proc	*/
    XtInheritSuperCaretQueryLocnProc,	/* query_sc_locn_proc   */
  },	/* End of Primitive field initializations */
  {
    0,                                     /* field not used    */
  },  /* ButtonClass fields initialization */
};

ButtonGadgetClassRec buttonGadgetClassRec = {
  {
    (WidgetClass) &(eventObjClassRec),  /* superclass             */
    "Button",                           /* class_name             */
    sizeof(ButtonRec),                  /* size                   */
    ClassInitialize,                    /* class_initialize       */
    NULL,                               /* class_part_initialize  */
    FALSE,                              /* class_inited           */
    ButtonInitialize,                   /* initialize             */
    NULL,                               /* initialize_hook        */
    (XtProc)ButtonRealize,              /* realize                */
    NULL,                               /* actions                */
    0,                                  /* num_actions            */
    gadget_resources,                   /* resources              */
    XtNumber(gadget_resources),         /* resource_count         */
    NULLQUARK,                          /* xrm_class              */
    FALSE,                              /* compress_motion        */
    TRUE,                               /* compress_exposure      */
    TRUE,                               /* compress_enterleave    */
    FALSE,                              /* visible_interest       */
    ButtonDestroy,                      /* destroy                */
    NULL,				/* resize                 */
    ButtonRedisplay,                    /* expose                 */
    SetValues,                          /* set_values             */
    NULL,                               /* set_values_hook        */
    XtInheritSetValuesAlmost,           /* set_values_almost      */
    NULL,                               /* get_values_hook        */
    (XtProc)XtInheritAcceptFocus,	/* accept_focus           */
    XtVersion,                          /* version                */
    NULL,                               /* callback_private       */
    NULL,                               /* tm_table               */
    NULL,                               /* query_geometry         */
  },  /* RectObjClass fields initialization */
  {
    NULL,				/* reserved		*/
    HighlightHandler,			/* highlight_handler	*/
    NULL,				/* traversal_handler	*/
    NULL,				/* register_focus	*/
    NULL,				/* activate		*/
    NULL,				/* event_procs		*/
    0,					/* num_event_procs	*/
    OlVersion,				/* version		*/
    NULL,				/* extension		*/
    {NULL, 0},				/* dyn_data		*/
    NULL,                             	/* transparent_proc     */
    XtInheritSuperCaretQueryLocnProc,	/* query_sc_locn_proc   */
  },  /* EventClass fields initialization */
  {
    0,                                     /* field not used    */
  },  /* ButtonClass fields initialization */
};


/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */


WidgetClass buttonWidgetClass = (WidgetClass) &buttonClassRec;
WidgetClass buttonGadgetClass = (WidgetClass) &buttonGadgetClassRec;


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
 *  GetGCs - This function recalculates the colors
 *	used to draw a button based on whether it has input
 *	focus.
 *
 *********************function*header************************
 */

static void
GetGCs (ButtonWidget w)
{
    Pixel	focus_color;
    Pixel	font_color;
    Pixel	foreground;
    Boolean	has_focus, isRectangular;
    ButtonPart	*bp = find_button_part(w);
    XFontStruct *font;
    XFontSet    fontset;
    OlStrRep    tf = find_text_format(w);
    int		scale = find_button_scale(w);
 
    if (tf == OL_SB_STR_REP)
        font = (XFontStruct *)(find_button_font(w));
    else
        fontset = (XFontSet)(find_button_font(w));

    if (_OlIsGadget (w))
    {
	focus_color = ((ButtonGadget) w)->event.input_focus_color;
	has_focus   = ((ButtonGadget) w)->event.has_focus;
	font_color  = ((ButtonGadget) w)->event.font_color;
	foreground  = ((ButtonGadget) w)->event.foreground;
    }
    else
    {
	focus_color = ((ButtonWidget) w)->primitive.input_focus_color;
	has_focus   = ((ButtonWidget) w)->primitive.has_focus;
	font_color  = ((ButtonWidget) w)->primitive.font_color;
	foreground  = ((ButtonWidget) w)->primitive.foreground;
    }

    if (bp->normal_GC)
	XtDestroyGC(bp->normal_GC);
    if (bp->inverse_text_GC)
	XtDestroyGC(bp->inverse_text_GC);
    if (bp->pAttrs)
	OlgxDestroyAttrs ((Widget)w, bp->pAttrs);

    GetNormalGC (w);
    GetInverseTextGC (w);

    isRectangular = (bp->button_type == OL_RECTBUTTON ||
		     bp->button_type == OL_LABEL);

    if (has_focus)
    {
	if (font_color == focus_color ||
	    bp->background_pixel == focus_color)
	{
	    GC	tmp;

	    /* reverse fg and bg for both the button and its label.
	     * If we are 2-D, then only reverse the label colors for
	     * rect buttons.  2-D oblong buttons are drawn as if they
	     * are selected.  (See getOblongFlags)
	     */
	    if (OlgIs3d (w->core.screen) || isRectangular)
	    {
		tmp = bp->normal_GC;
		bp->normal_GC = bp->inverse_text_GC;
		bp->inverse_text_GC = tmp;
		bp->pAttrs = OlgxCreateAttrs((Widget)w,
					     bp->background_pixel,
					     (OlgxBG *)&(font_color),
					     False, scale, tf,
					     (tf == OL_SB_STR_REP? 
						(OlFont)font : (OlFont)fontset));
	    }
	    else
	    {
		bp->pAttrs = OlgxCreateAttrs((Widget) w,
					     font_color,
					     (OlgxBG *)&(bp->background_pixel),
					     False, scale, tf,
                                             (tf == OL_SB_STR_REP? 
						(OlFont)font : (OlFont)fontset));
	    }
	}
	else
	    bp->pAttrs = OlgxCreateAttrs((Widget)w,
					 font_color,
					 (OlgxBG *)&(focus_color),
					 False, scale, tf,
                                         (tf == OL_SB_STR_REP?
                                             (OlFont)font : (OlFont)fontset));
    }
    else
	bp->pAttrs = OlgxCreateAttrs((Widget)w,
				     font_color,
				     (OlgxBG *)&(bp->background_pixel),
				     False, scale, tf,
                                     (tf == OL_SB_STR_REP? 
                                          (OlFont)font : (OlFont)fontset)); 
}

/*
 ************************************************************
 *
 *  GetInverseTextGC - this function sets the button's font,
 *	foreground, and background in a Graphics Context for
 *	drawing the button in its inverse state.
 *
 *********************function*header************************
 */
static void
GetInverseTextGC(ButtonWidget bw)
{
	XGCValues	values;
	ButtonPart	*bp;
	OlStrRep	tf = find_text_format(bw);

	bp = find_button_part(bw);

	values.foreground = bp->background_pixel;
	values.background = find_button_font_color(bw);
	if(tf == OL_SB_STR_REP){
                values.font = ((XFontStruct *)find_button_font(bw))->fid;
		bp->inverse_text_GC = XtGetGC(XtParent(bw),
			GCForeground | GCFont | GCBackground,
			&values);
        }
        else{
	/* In MB or WC case , GCFont will be set by DrawString routine */
		bp->inverse_text_GC = XtGetGC(XtParent(bw),
			GCForeground | GCBackground,
			&values);
        }

}	/* GetInverseTextGC */

/*
 ************************************************************
 *
 *  GetNormalGC - this function sets the button's font,
 *	foreground, and background in a Graphics Context for
 *	drawing the button in its normal state.
 *
 *********************function*header************************
 */

static void
GetNormalGC(ButtonWidget bw)
{
	XGCValues	values;
	ButtonPart *bp;
	OlStrRep  tf = find_text_format(bw);

	bp = find_button_part(bw);


	values.foreground = find_button_font_color(bw);
	values.background = bp->background_pixel;
	if(tf == OL_SB_STR_REP){
                values.font     = 
			((XFontStruct *)find_button_font(bw))->fid;
		bp->normal_GC = XtGetGC(XtParent(bw),
			GCForeground | GCFont | GCBackground,
			&values);
        }
        else{
	/* In MB or WC case , GCFont will be set by DrawString routine */
		bp->normal_GC = XtGetGC(XtParent(bw),
			GCForeground | GCBackground,
			&values);
        }

}	/* GetNormalGC */


/*
 ************************************************************
 *
 *  IsMenuMode - this function checks if button is in a menu
 *	context since visual (and size) is different.
 *
 *********************function*header************************
 */

static Boolean
IsMenuMode(ButtonWidget bw)
{
	Boolean retval;
	ButtonPart *bp;

	bp = find_button_part(bw);

	switch (bp->shell_behavior) {

		case PinnedMenu:
		case PressDragReleaseMenu:
		case StayUpMenu:
		case UnpinnedMenu:
			retval = TRUE;
			break;

		case BaseWindow:
		case PopupWindow:
		case PinnedWindow:
		case OtherBehavior:
			retval = FALSE;
			break;
	}
	return retval;

}	/* IsMenuMode */


/*
 ************************************************************
 *
 *  OblongRedisplay - This function redisplays the oblong
 *	button in its current state.
 *
 *********************function*header************************
 */
/*ARGSUSED1*/
static void
OblongRedisplay(Widget w, XEvent *event, Region region)
         
              		/* unused */
              		/* unused */
{
	ButtonWidget	bw = (ButtonWidget) w;
	ButtonPart *	bp = find_button_part(bw);

	if (bp->set)
		_OlDrawHighlightButton(bw);
	else
		_OlDrawNormalButton(bw);

}	/* OblongRedisplay */

/*
 ***********************************************************
 *
 *  getOblongFlags - Determine the flags needed for
 *	olgx_draw_accel_button according to the state of the
 *	button.
 *
 *********************function*header************************
 */
static int
getOblongFlags (register Widget bw)
{
	Boolean	has_focus;
	register ButtonPart *	bp = find_button_part(bw);
	register int		flags = 0;

	has_focus = (_OlIsGadget(bw)) ?
	    ((ButtonGadget)bw)->event.has_focus :
	    ((ButtonWidget)bw)->primitive.has_focus;
        if (bp->set)
	    flags |= OLGX_INVOKED;
	else
	{
	    /* If the button is 2-D and has input focus, the coloration is
	     * as if the button is set if the input focus color conflicts with
	     * either the label color or background.  If the input focus color
	     * is different from either of these, than normal button coloration
	     * is used.
	     */
	    if (!OlgIs3d (bw->core.screen))
	    {
		Pixel	focus_color;
		Pixel	font_color;

		if (_OlIsGadget (bw))
		{
		    focus_color = ((ButtonGadget) bw)->event.input_focus_color;
		    font_color = ((ButtonGadget) bw)->event.font_color;
		}
		else
		{
		    focus_color = 
			((ButtonWidget) bw)->primitive.input_focus_color;
		    font_color = ((ButtonWidget) bw)->primitive.font_color;
		}

		if (has_focus && 
		    _OlInputFocusFeedback(bw) == OL_INPUT_FOCUS_COLOR && 
		        (focus_color == font_color ||
			 focus_color == bp->background_pixel))
		    flags |= OLGX_INVOKED;
	    }
	}
	if ((bp->busy) || (bp->internal_busy))
	{
	    flags |= OLGX_BUSY;
	    flags &= ~OLGX_INVOKED;
	}
	if (!flags)
	    flags |= OLGX_ERASE;

	if (bp->is_default)
	    flags |= OLGX_DEFAULT;
	if (!XtIsSensitive (bw))
	    flags |= OLGX_INACTIVE;
	if (IsMenuMode((ButtonWidget)bw))
	    /* Need to fake out OLGX here; it wants to clear the button
	     * background using XFillRectangle, which is most undesirable
	     * if the button background colour is different than that of
	     * its container. So don't tell OLGX this is a menu item
	     * if this button has the inputFocusColor; this will draw
	     * it as a 3D raised button on the menu, but that is better
	     * than a rectangle (not too noticeable, depending on the
	     * inputFocusColor).
	     * There is no workaround for the 2D colour case.
	     */
	    if (_OlInputFocusFeedback(bw) == OL_SUPERCARET || 
		!has_focus || bp->is_default)
		    flags |= OLGX_MENU_ITEM;
	if (bp->button_type == OL_BUTTONSTACK)
	    flags |= (bp->menumark == OL_RIGHT) ? 
		OLGX_HORIZ_MENU_MARK : OLGX_VERT_MENU_MARK;
	if (bp->label_type == OL_IMAGE)
	    flags |= OLGX_LABEL_IS_XIMAGE;
	else if (find_text_format(bw) == OL_WC_STR_REP)
	    flags |= OLGX_LABEL_IS_WCS;

	return flags;
}

/*
 ************************************************************
 *
 *  sizeProcLabel - Determine the size of a procedurally defined
 *  label.  Package up some button attributes and call the user's
 *  function.
 ************************************************************
 */

static void
sizeProcLabel (Screen *scr, OlgxAttrs *pAttrs, XtPointer labeldata, Dimension *pWidth, Dimension *pHeight)
{
    Widget		w = *((Widget *) labeldata);
    ButtonProcLbl	lbl;
    ButtonPart		*bp = find_button_part ((ButtonWidget) w);

    lbl.callbackType = OL_SIZE_PROC;
    lbl.ginfo = pAttrs->ginfo;
    lbl.justification = bp->label_justify;
    lbl.set = bp->set;
    lbl.width = 0;
    lbl.height = 0;

    XtCallCallbacks (w, XtNlabelProc, (caddr_t) &lbl);

    *pWidth = lbl.width;
    *pHeight = lbl.height;
}

/*
 ************************************************************
 *
 *  drawProcButton - Draw a button & procedurally defined label.  Package
 *  up some button attributes and call the user's function.
 */

static void
drawProcButton (
    Screen	*scr,
    Drawable	win,
    OlgxAttrs	*pAttrs,
    Position	x,
    Position	y,
    Dimension	buttonWidth,
    Dimension	buttonHeight,
    XtPointer	labeldata,
    OlDefine	buttonType,
    int		flags)
{
    Widget		w = *((Widget *)labeldata);
    ButtonProcLbl	lbl;
    ButtonPart		*bp = find_button_part((ButtonWidget)w);
    void		(*olgxDrawProc)();

    switch (buttonType) {
	case OL_LABEL:
	    olgxDrawProc = olgx_draw_accel_label;
	    break;
	case OL_RECTBUTTON:
	    olgxDrawProc = olgx_draw_accel_choice_item;
	    break;
	case OL_OBLONG:
	case OL_BUTTONSTACK:
	default:
	    olgxDrawProc = olgx_draw_accel_button;
	    break;
    }
    (*olgxDrawProc)(pAttrs->ginfo, win, x, y, buttonWidth, buttonHeight,
	NULL, 0, NULL, 0, OLGX_NORMAL, 0, NULL, 0, NULL, flags);

    /* Stuff relevant data into a structure to pass to the user function
     * to draw the label.
     * Note that the screen and window to draw in might be different than
     * those in the widget.  Previewing does make life difficult.
     */
    lbl.callbackType = OL_DRAW_PROC;
    lbl.win = win;
    lbl.ginfo = pAttrs->ginfo;
    lbl.x = x;
    lbl.y = y;
    lbl.width = buttonWidth;
    lbl.height = buttonHeight;
    lbl.justification = bp->label_justify;
    lbl.set = bp->set;
    lbl.sensitive = XtIsSensitive(w);

    XtCallCallbacks (w, XtNlabelProc, (caddr_t)&lbl);
}

/*
 ************************************************************
 *
 *  setLabel - populate the label structure and select the
 *	proper sizing and drawing functions.
 *
 *********************function*header************************
 */
static void
setLabel(
ButtonWidget	bw,
ButtonLabel	*lbl,
OlSizeProc	*sizeProc,
OlDrawProc      *drawProc)
{
    ButtonPart	*bp = find_button_part(bw);
    OlStrRep  tf = find_text_format(bw);

    /*
     * WARNING: This temporary measure for showing accelerators/mnemonics
     * assumes that the "lbl->text.label" value is only needed for a short
     * time. If this routine can be reentered before the "lbl->text.label"
     * from a previous call is used, havoc will ensue.
     */

    switch (bp->label_type)  {
    case OL_STRING:
    case OL_POPUP:
	lbl->text.label = bp->label;
	lbl->text.font = find_button_font(bw);
	lbl->text.text_format = tf;
	lbl->text.flags = (bp->label_type == OL_POPUP) ? TL_POPUP : 0;
	if (bp->menu_has_meta)
	    lbl->text.flags |= TL_LAYOUT_WITH_META;

	switch (bp->label_justify) {
	case OL_LEFT:
	    lbl->text.justification = TL_LEFT_JUSTIFY;
	    break;

	case OL_CENTER:
	    lbl->text.justification = TL_CENTER_JUSTIFY;
	    break;

	default:
	    lbl->text.justification = TL_RIGHT_JUSTIFY;
	    break;
	}
	
	if (_OlIsGadget(bw)) {
	    lbl->text.mnemonic = ((ButtonGadget)(bw))->event.mnemonic;
	    lbl->text.qualifier = 
		(unsigned char *)((ButtonGadget)(bw))->event.qualifier_text;
	    lbl->text.meta = ((ButtonGadget)(bw))->event.meta_key;
	    lbl->text.accelerator = 
		(unsigned char *)((ButtonGadget)(bw))->event.accelerator_text;
	} else {
	    lbl->text.mnemonic = bw->primitive.mnemonic;
	    lbl->text.qualifier = (unsigned char *)bw->primitive.qualifier_text;
	    lbl->text.meta = bw->primitive.meta_key;
	    lbl->text.accelerator = 
		(unsigned char *)bw->primitive.accelerator_text;
	}

	*sizeProc = OlgxSizeTextLabel;
	*drawProc = OlgxDrawTextButton;
	break;

    case OL_IMAGE:
	lbl->image.label.image = bp->label_image;
	lbl->image.type = PL_IMAGE;
	lbl->image.flags = 0;

	switch (bp->label_justify) {
	case OL_LEFT:
	    lbl->image.justification = TL_LEFT_JUSTIFY;
	    break;

	case OL_CENTER:
	    lbl->image.justification = TL_CENTER_JUSTIFY;
	    break;

	default:
	    lbl->image.justification = TL_RIGHT_JUSTIFY;
	    break;
	}
	*sizeProc = OlgxSizePixmapLabel;
	*drawProc = OlgxDrawImageButton;
	break;

    case OL_PROC:
	lbl->proc_widget = (Widget)bw;
	*sizeProc = sizeProcLabel;
	*drawProc = drawProcButton;
	break;
    }
}

/*
 ************************************************************
 *
 *  RectRedisplay - The Redisplay function must clear the
 *	entire window before drawing the button from scratch;
 *	there is no attempt to repair damage on the button.
 *	Note: this function also used for OL_LABEL buttontype.
 *
 *********************function*header************************
 */
/*ARGSUSED1*/
static void
RectRedisplay(Widget w, XEvent *event, Region region)
         
              		/* unused */
              		/* unused */
{
	ButtonPart *	bp = find_button_part(w);
	ButtonLabel	lbl;
	OlSizeProc	sizeProc;
	OlDrawProc	drawProc;
	int		flags;

	if(!XtIsRealized(w))
		return;

	setLabel((ButtonWidget)w, &lbl, &sizeProc, &drawProc);
	flags = getRectFlags(w);
	(*drawProc)(XtScreenOfObject(w), XtWindowOfObject(w), bp->pAttrs, 
	    _OlXTrans(w, 0), _OlYTrans(w, 0), w->core.width, w->core.height,
	    (XtPointer)&lbl, bp->button_type, flags);

}	/* RectRedisplay */

/*
 ***********************************************************
 *
 *  getRectFlags - Determine the flags needed for
 *	olgx_draw_accel_choice_item according to the state of the
 *	button.
 *
 *********************function*header************************
 */
static int
getRectFlags (register Widget bw)
{
	register ButtonPart *	bp = find_button_part(bw);
	register int		flags = 0;

        if (bp->set)
	    flags |= OLGX_INVOKED;
	if (bp->is_default)
	    flags |= OLGX_DEFAULT;
	if (bp->dim || !XtIsSensitive(bw))
	    flags |= OLGX_INACTIVE;

	if (bp->label_type == OL_IMAGE)
	    flags |= OLGX_LABEL_IS_XIMAGE;
	else if (find_text_format(bw) == OL_WC_STR_REP)
	    flags |= OLGX_LABEL_IS_WCS;

	return flags;
}

/*
 ************************************************************
 *
 *  SetLabelTile - this function creates a pixmap of the
 *	current image and sets it as the tile to the necessary
 *	GCs.
 *
 *********************function*header************************
 */
static void
SetLabelTile(ButtonWidget bw)
{
	XGCValues	values;
	Pixel		font_color;
	Pixel		foreground;
	XFontStruct	*font;
	XFontSet	fontset;
	ButtonPart *	bp = find_button_part(bw);
	OlStrRep	tf = find_text_format(bw);
	int		scale = find_button_scale(bw);
	Display *	bw_display = XtDisplayOfObject((Widget)bw);

	if(tf == OL_SB_STR_REP)
		font       = (XFontStruct *)(find_button_font(bw));
	else
		fontset		= (XFontSet)(find_button_font(bw));
	font_color = find_button_font_color(bw);
	foreground = find_button_foreground(bw);
	if (bp->label_tile)  {
		Pixmap tile_pixmap;
		Pixmap highlight_tile_pixmap;

		if (bp->label_image == (XImage *) NULL)
			OlError(dgettext(OlMsgsDomain,
				"XtNlabelImage must be set when XtNlabelTile is True.\n"));

		tile_pixmap = XCreatePixmap(bw_display,
			RootWindowOfScreen(XtScreenOfObject((Widget)bw)),
			(unsigned int) bp->label_image->width,
			(unsigned int) bp->label_image->height,
			(unsigned int) OlDepthOfObject((Widget)bw)
			);

		highlight_tile_pixmap = XCreatePixmap(bw_display,
			RootWindowOfScreen(XtScreenOfObject((Widget)bw)),
			(unsigned int) bp->label_image->width,
			(unsigned int) bp->label_image->height,
			(unsigned int) OlDepthOfObject((Widget)bw)
			);

		XPutImage(bw_display,
			tile_pixmap,
			bp->inverse_text_GC,
			bp->label_image,
			0,
			0,
			(int) _OlXTrans(bw, 0),
			(int) _OlYTrans(bw, 0),
			(unsigned int) bp->label_image->width,
			(unsigned int) bp->label_image->height);

		XPutImage(bw_display,
			highlight_tile_pixmap,
			bp->normal_GC,
			bp->label_image,
			0,
			0,
			(int) _OlXTrans(bw, 0),
			(int) _OlYTrans(bw, 0),
			(unsigned int) bp->label_image->width,
			(unsigned int) bp->label_image->height);
		/*
		 *  Recreate the GCs using the appropriate tile
		 */

		if (bp->pAttrs)
		    OlgxDestroyAttrs ((Widget)bw, bp->pAttrs);
		bp->pAttrs = OlgxCreateAttrs ((Widget)bw,
					     font_color,
					     (OlgxBG *)&(tile_pixmap),
					     True, scale, tf,
					     (tf == OL_SB_STR_REP? 
						(OlFont)font : (OlFont)fontset));

		if (bp->pHighlightAttrs)
		    OlgxDestroyAttrs ((Widget)bw, bp->pHighlightAttrs);
		bp->pHighlightAttrs = OlgxCreateAttrs ((Widget)bw,
				      font_color,
				      (OlgxBG *)&(highlight_tile_pixmap),
				      True, scale, tf,
				      (tf == OL_SB_STR_REP? 
					  (OlFont)font : (OlFont)fontset));

		XtDestroyGC(bp->inverse_text_GC);
		values.foreground = bp->background_pixel;
		values.background = font_color;
		values.tile = highlight_tile_pixmap;

		if(tf == OL_SB_STR_REP){
			values.font = font->fid;
			bp->inverse_text_GC = XtGetGC(XtParent(bw),
				GCForeground | GCFont | GCBackground | GCTile,
				&values);
		}
		else{
			bp->inverse_text_GC = XtGetGC(XtParent(bw),
				GCForeground | GCBackground | GCTile,
				&values);
		}

		XtDestroyGC(bp->normal_GC);
		values.foreground = font_color;
		values.background = bp->background_pixel;
		values.tile = tile_pixmap;

		if(tf == OL_SB_STR_REP){
			values.font = font->fid;
			bp->normal_GC = XtGetGC(XtParent(bw),
				GCForeground | GCFont | GCBackground | GCTile,
				&values);
		}
		else{
			bp->normal_GC = XtGetGC(XtParent(bw),
				GCForeground | GCBackground | GCTile,
				&values);
		}
	
		XFreePixmap(bw_display, tile_pixmap);
		XFreePixmap(bw_display, highlight_tile_pixmap);
		}

}	/*  SetLabelTile  */


/*
 ************************************************************
 *
 *  SetDimensions - This function determines the 
 *	dimensions of the core width and height of the 
 *	button window.
 *
 *********************function*header************************
 */
static void
SetDimensions(ButtonWidget bw)
{
	ButtonPart	*bp;
	OlSizeProc	lblSizeProc;
	OlDrawProc	lblDrawProc;
	ButtonLabel	lbl;
	int		flags;

	bp = find_button_part(bw);

	setLabel(bw, &lbl, &lblSizeProc, &lblDrawProc);

	switch (bp->button_type) {
	default:
	case OL_RECTBUTTON:
	case OL_LABEL:
	    flags = getRectFlags((Widget)bw);
	    OlgxSizeRectButton(XtScreenOfObject((Widget)bw), bp->pAttrs, 
			       (XtPointer)&lbl, lblSizeProc, flags, 
			       &bp->normal_width, &bp->normal_height);
	    break;

	case OL_OBLONG:
	case OL_BUTTONSTACK:
	    flags = getOblongFlags((Widget)bw);
	    OlgxSizeOblongButton(XtScreenOfObject((Widget)bw), bp->pAttrs, 
				 (XtPointer)&lbl, lblSizeProc, flags, 
				 &bp->normal_width, &bp->normal_height);
	    break;
	}

	if (bp->recompute_size)  {
		bw->core.width = bp->normal_width;
		bw->core.height = bp->normal_height;
		return;
		}

	if (bw->core.height == (Dimension) 0)
		bw->core.height = bp->normal_height;

	if (bw->core.width == (Dimension) 0)
		bw->core.width = bp->normal_width;

}	/*  SetDimensions  */

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
 *  ButtonDestroy - this function frees the GCs and the space
 *	allocated for the copy of the label.
 *
 *********************function*header************************
 */
static void
ButtonDestroy(Widget w)
{
	ButtonWidget bw = (ButtonWidget)w;
	ButtonPart *bp;

	bp = find_button_part(bw);


	/*  Get rid of callbacks and eventhandlers */

	XtRemoveAllCallbacks(w, XtNselect);
	XtRemoveAllCallbacks(w, XtNunselect);
	XtRemoveAllCallbacks(w, XtNpostSelect);

	/*
	 *  Free the GCs
	 */
	OlgxDestroyAttrs (w, bp->pAttrs);
	if (bp->pHighlightAttrs)
	    OlgxDestroyAttrs (w, bp->pHighlightAttrs);
	XtDestroyGC(bp->normal_GC);
	XtDestroyGC(bp->inverse_text_GC);

	/*
	 *  Free the label, if it is not the widget name
	 */
       	if (bp->label)
		XtFree(bp->label);

}	/* ButtonDestroy */


/*
 ************************************************************
 *
 *  ButtonInitialize - this function is called when the 
 *	widget is created.  It copies the label, creates the
 *	GCs, and initializes the button state to NORMAL.
 *
 *********************function*header************************
 */
/* ARGSUSED */
static void
ButtonInitialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	ButtonWidget	bw = (ButtonWidget) new;
	ButtonPart *	bp = find_button_part(bw);
	Widget		shell=(Widget)NULL;
	OlStrRep	tf = find_text_format(bw);
    	XFontStruct *font;
    	XFontSet    fontset;

        if (tf == OL_SB_STR_REP)
            font = (XFontStruct *)(find_button_font(bw));
        else
            fontset = (XFontSet)(find_button_font(bw));


				/* initialize button fields as needed */

	if(!bp->recompute_size) {
		if(bw->core.width<=(Dimension)3) { /* calculation assumption */
			bw->core.width=(Dimension)4;
		}
		if(bw->core.height==(Dimension)0) 
			bw->core.height=(Dimension)1;
	}

	bw->core.border_width = (Dimension) 0;

	/*
	 *  Do a check on the boolean resource values.  They must
	 *  be boolean values.
	 */
	if (bp->is_default) {
		bp->is_default = (Boolean) TRUE;

			/* Since our default is TRUE, tell the shell	*/
		_OlSetDefault(new, bp->is_default);
	}

	if (bp->set)
		bp->set = (Boolean) TRUE;

	if (bp->dim)
		bp->dim = (Boolean) TRUE;

	if (bp->busy)
		bp->busy = (Boolean) TRUE;

	bp->internal_busy = (Boolean) FALSE;

	if (bp->label_tile)
	    /* OLGX_TODO: OLGX doesn't support label tiles yet; just say no */
	    /*bp->label_tile = (Boolean)TRUE;*/
	    bp->label_tile = False;

				/* check for valid button type */

	if(bp->button_type!=OL_LABEL
		&& bp->button_type!=OL_OBLONG
		&& bp->button_type!=OL_RECTBUTTON
		&& bp->button_type!=OL_BUTTONSTACK) {

		bp->button_type = OL_LABEL;
		OlWarning(dgettext(OlMsgsDomain,
			"Invalid XtNbuttonType: OL_LABEL used as default.\n"));
	}

				/* check for valid label type */

	if (bp->label_type != OL_STRING 
		&& bp->label_type != OL_IMAGE
		&& bp->label_type != OL_POPUP
		&& bp->label_type != OL_PROC)  {

		bp->label_type = OL_STRING;
		OlWarning(dgettext(OlMsgsDomain,
			"Invalid XtNlabelType: OL_STRING used as default.\n"));
	}

				/* check for valid combinations */

	if(bp->button_type!=OL_OBLONG && bp->label_type==OL_POPUP) {
		bp->label_type=OL_STRING;
		OlWarning(dgettext(OlMsgsDomain,
		   "Invalid XtNbuttonType/XtNlabelType: using OL_STRING\n"));
	}
	if(bp->label_type==OL_IMAGE &&  bp->label_image==(XImage *)NULL) {
		bp->label_type=OL_STRING;
		OlWarning(dgettext(OlMsgsDomain,
			"No XtNlabelImage: XtNlabelType set to OL_STRING\n"));
	}

	shell = _OlGetShellOfWidget(new);

	if(shell && XtIsSubclass(shell, menuShellWidgetClass)) {
		Arg	arg; 	
		XtSetArg(arg,XtNshellBehavior,
			(XtArgVal)&(bp->shell_behavior));
		XtGetValues(shell,&arg,1);
	}

	if (IsMenuMode(bw) && bp->menumark!=OL_RIGHT) {
		bp->menumark = OL_RIGHT;
	}
	else if (bp->menumark != OL_DOWN && bp->menumark != OL_RIGHT) {
		bp->menumark = OL_DOWN;
	}

	/*
	 *  First initialize the resources that are common to
	 *  all buttons.
	 */

	/* Set the new label -- always copy & never allow a NULL pointer */
	if(tf != OL_WC_STR_REP)
		bp->label = XtNewString((XtPointer)(bp->label ? 
	    			bp->label : XtName((Widget)bw)));
	else {
		wchar_t *ws;
		if(bp->label) {
		ws = (wchar_t *)XtMalloc((wslen((wchar_t *)bp->label)
						+1)*sizeof(wchar_t));
		wscpy(ws, (wchar_t *)bp->label); 
		bp->label = (OlStr)ws;
		} else {
		char *widget_name = XtName((Widget)bw);
		ws = (wchar_t *)XtMalloc((strlen(widget_name) +1)
						*sizeof(wchar_t));
		(void)mbstowcs(ws, widget_name, strlen(widget_name)+1);
		bp->label = (OlStr)ws;
		}
	}

	if (XtIsSubclass(new, menuButtonWidgetClass) || 
	    XtIsSubclass(new, menuButtonGadgetClass)) {
	    if (_OlIsGadget(bw)) {
		if (((ButtonGadget)bw)->event.accelerator_text) {
		    ((ButtonGadget)bw)->event.accelerator = NULL;
		    ((ButtonGadget)bw)->event.accelerator_text = NULL;
		    ((ButtonGadget)bw)->event.meta_key = False;
		    ((ButtonGadget)bw)->event.qualifier_text = NULL;
		    OlWarning(dgettext(OlMsgsDomain,
		       "MenuButton: cannot specify accelerator for submenu\n"));
		}
		if (((ButtonGadget)bw)->event.mnemonic) {
		    ((ButtonGadget)bw)->event.mnemonic = NULL;
		    OlWarning(dgettext(OlMsgsDomain,
		       "MenuButton: cannot specify mnemonic for submenu\n"));
		}
	    } else {
		if (bw->primitive.accelerator_text) {
		    bw->primitive.accelerator = NULL;
		    bw->primitive.accelerator_text = NULL;
		    bw->primitive.meta_key = NULL;
		    bw->primitive.qualifier_text = NULL;
		    OlWarning(dgettext(OlMsgsDomain,
		       "MenuButton: cannot specify accelerator for submenu\n"));
		}
		if (bw->primitive.mnemonic) {
		    bw->primitive.mnemonic = NULL;
		    OlWarning(dgettext(OlMsgsDomain,
		       "MenuButton: cannot specify mnemonic for submenu\n"));
		}
	    }
	}

	if (IsMenuMode(bw))
	    bp->menu_has_meta = (_OlIsGadget(bw)) ? 
		((ButtonGadget)bw)->event.meta_key : bw->primitive.meta_key;

	if (!_OlIsGadget(bw))
		bp->background_pixel = bw->core.background_pixel;

	/* check on resources initialized by user */

	if (bp->label_justify != OL_LEFT 
			&& bp->label_justify!=OL_CENTER)  {
		bp->label_justify = OL_LEFT;
		OlWarning(dgettext(OlMsgsDomain,
	         "Invalid XtNlabelJustify value: Using OL_LEFT as default.\n"));
	}

	GetNormalGC(bw);
	GetInverseTextGC(bw);

	bp->pAttrs = bp->pHighlightAttrs = (OlgxAttrs *) 0;
	SetLabelTile(bw);

	/* LATER -- must support background pixmaps */
	if (!bp->label_tile)
	    bp->pAttrs = OlgxCreateAttrs((Widget)bw,
					 find_button_font_color(bw),
					 (OlgxBG *)&(bp->background_pixel),
					 False, find_button_scale(bw), tf,
					 (tf == OL_SB_STR_REP? 
					     (OlFont)font : (OlFont)fontset));

	/*
	 *  Now initialize resources which are specific to each type
	 *  of button.
	 */

	SetDimensions(bw);

} 	/* ButtonInitialize */


/*
 ************************************************************
 *
 *  ButtonRedisplay - This function displays the button depending
 *	upon the button type.  If the event is null, then the button
 *	is not being redisplayed in response to an expose event,
 *	and the window must be cleared before drawing the button.
 *
 *********************function*header************************
 */
static void
ButtonRedisplay(Widget w, XEvent *event, Region region)
         
              		/* unused */
              		/* unused */
{
	ButtonWidget bw = (ButtonWidget) w;
	ButtonPart *bp;

	if(XtIsRealized(w) == FALSE) return;

	/* OLGX_TODO: need this, when getOblongFlags is setting OLGX_ERASE
	 * for OLGX_NORMAL? (transparent button)
	 */
	if (event == (XEvent *) 0)
	    XClearArea(XtDisplayOfObject((Widget)bw), 
	    	XtWindowOfObject((Widget)bw),
		    _OlXTrans (bw, 0), _OlYTrans (bw, 0),
		    bw->core.width, bw->core.height, False);

	bp = find_button_part(bw);

	switch (bp->button_type)  {
		case OL_RECTBUTTON:
		case OL_LABEL:
			RectRedisplay(w, event, region);
			break;
		case OL_OBLONG:
		case OL_BUTTONSTACK:
		default:
			OblongRedisplay(w, event, region);
			break;
		}
}	/* ButtonRedisplay */

/*
 ************************************************************
 *
 *  ButtonRealize - realize the button widget.
 *
 *********************function*header************************
 */
static void
ButtonRealize(Widget w, XtValueMask *valueMask,
	      XSetWindowAttributes *attributes)
{

	/* The window background is always inherited from the parent. */
	if (XtClass(w) != buttonWidgetClass) {
		attributes->background_pixmap = ParentRelative;
		*valueMask |= CWBackPixmap;
		*valueMask &= ~CWBackPixel;
	}
	XtCreateWindow(w, (unsigned int)InputOutput, (Visual *)CopyFromParent,
		    *valueMask, attributes );

}	/* ButtonRealize */


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
						/* XtNbuttonType */
	_OlAddOlDefineType ("label",       OL_LABEL);
	_OlAddOlDefineType ("oblong",      OL_OBLONG);
	_OlAddOlDefineType ("rectbutton",  OL_RECTBUTTON);
	_OlAddOlDefineType ("buttonstack", OL_BUTTONSTACK);

						/* XtNlabelType */
	_OlAddOlDefineType ("string",      OL_STRING);
	_OlAddOlDefineType ("image",       OL_IMAGE);
	_OlAddOlDefineType ("popup",       OL_POPUP);
	_OlAddOlDefineType ("proc",        OL_PROC);

						/* XtNlabelJustify */
	_OlAddOlDefineType ("left",        OL_LEFT);
	_OlAddOlDefineType ("center",      OL_CENTER);

						/* XtNmenuMark */
	_OlAddOlDefineType ("down",        OL_DOWN);
	_OlAddOlDefineType ("right",       OL_RIGHT);
} /* ClassInitialize */

/*
 ************************************************************
 *
 *  SetValues - This function compares the requested values
 *	to the current values, and sets them in the new
 *	widget.  It returns TRUE when the widget must be
 *	redisplayed.
 *
 *********************function*header************************
 */
/* ARGSUSED */
static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	ButtonWidget bw = (ButtonWidget) current;
	ButtonWidget newbw = (ButtonWidget) new;
	Boolean needs_redisplay = (Boolean) FALSE;
	Boolean was_resized = (Boolean) FALSE;
	OlStrRep	tf = find_text_format(newbw);
	ButtonPart *bp;
	ButtonPart *newbp;

	bp = find_button_part(bw);
	newbp = find_button_part(newbw);



	if ((find_button_font(newbw) != find_button_font(bw)) ||
    		(find_button_scale(newbw) != find_button_scale(bw))) {
		was_resized = (Boolean)True;
	}
	

	/* Note: cannot change button type since semantics won't match */

	if(newbp->button_type!=bp->button_type) {

		newbp->button_type=bp->button_type;
		OlWarning(dgettext(OlMsgsDomain,
			"Cannot change XtNbuttonType.\n"));
	}

				/* check for valid label type */

	if (newbp->label_type != OL_STRING 
		&& newbp->label_type != OL_IMAGE
		&& newbp->label_type != OL_POPUP
		&& newbp->label_type != OL_PROC)  {

		newbp->label_type = OL_STRING;
		OlWarning(dgettext(OlMsgsDomain,
			"Invalid XtNlabelType: OL_STRING used as default.\n"));
	}

				/* check for valid combinations */

	if(newbp->button_type!=OL_OBLONG && newbp->label_type==OL_POPUP) {
		newbp->label_type=OL_STRING;
		OlWarning(dgettext(OlMsgsDomain,
		    "Invalid XtNbuttonType/XtNlabelType: using OL_STRING\n"));
	}
	if(newbp->label_type==OL_IMAGE && newbp->label_image==(XImage *)NULL) {
		newbp->label_type=OL_STRING;
		OlWarning(dgettext(OlMsgsDomain,
		     "No XtNlabelImage: XtNlabelType set to OL_STRING\n"));
	}
	
	/*
	 *  Has the menumark resource changed?
	 */

	if (IsMenuMode(newbw) && newbp->menumark != OL_RIGHT) {
		newbp->menumark = OL_RIGHT;
	}
	else if (newbp->menumark != OL_DOWN && newbp->menumark != OL_RIGHT) {
		newbp->menumark = OL_DOWN;
	}

	/*
	 *  Has the shell_behavior resource changed?
	 */

	if (bp->shell_behavior!=newbp->shell_behavior 
		&& ( (IsMenuMode(bw) && !IsMenuMode(newbw)
			|| (!IsMenuMode(bw) && IsMenuMode(newbw)))))
	{
		was_resized=(Boolean) TRUE;
	}

	/*
	 *  Has the label_type resource changed?
	 */
	
	/* OLGX_TODO: OLGX doesn't support label tiles yet; just say no */
	if (newbp->label_tile)
	    newbp->label_tile = False;

	if(newbp->label_type!=bp->label_type) {	

		switch (newbp->label_type)  {

		case OL_STRING:
		case OL_POPUP:
		case OL_PROC:
			if (bp->label_tile)  {
				/*
				 *  Turn off tile in the GC's
				 */
				XtDestroyGC(bp->inverse_text_GC);
				XtDestroyGC(bp->normal_GC);
				GetInverseTextGC(newbw);
				GetNormalGC(newbw);
				}
			break;

		case OL_IMAGE:
			if (newbp->label_image != (XImage *)NULL)  {
				if (newbp->label_tile)
					SetLabelTile(newbw);
				break;
				}

		}
		was_resized = (Boolean) TRUE;
	}

	/*
	 *  Has the label resource changed?  
	 *
	 *  MORE: Check the show-mnemonic and show-accelerator resources.
	 */

	if (bp->label != newbp->label)  {
		if(tf != OL_WC_STR_REP)
			newbp->label = 
				XtNewString((XtPointer)(newbp->label ? 
	    			newbp->label : XtName((Widget)bw)));
		else {
			wchar_t *ws;
			if(newbp->label) {
			ws = (wchar_t *)XtMalloc((wslen((wchar_t *)
					newbp->label) +1)*sizeof(wchar_t));
			wscpy(ws, (wchar_t *)newbp->label); 
			newbp->label = (OlStr)ws;
			} else {
			char *widget_name = XtName((Widget)bw);
			ws = (wchar_t *)XtMalloc((strlen(widget_name) +1)
						*sizeof(wchar_t));
			mbstowcs(ws, widget_name, strlen(widget_name)+1);
			newbp->label = (OlStr)ws;
			}
		}
		if (bp->label)
			XtFree((char*)bp->label);

		was_resized = (Boolean)TRUE;
	}

	/*
	 *  Has the mnemonic changed?  
	 *  
	 */

	if (_OlIsGadget (newbw)) {
	    if (XtIsSubclass(new, menuButtonGadgetClass) &&
		((ButtonGadget)newbw)->event.mnemonic) {
		    ((ButtonGadget)newbw)->event.mnemonic = NULL;
		    OlWarning(dgettext(OlMsgsDomain,
		       "MenuButton: cannot specify mnemonic for submenu\n"));
	    } else if (((ButtonGadget) bw)->event.mnemonic !=
		       ((ButtonGadget) newbw)->event.mnemonic)
		was_resized = (Boolean) TRUE;
	} else {
	    if (XtIsSubclass(new, menuButtonWidgetClass) &&
		newbw->primitive.mnemonic) {
		    newbw->primitive.mnemonic = NULL;
		    OlWarning(dgettext(OlMsgsDomain,
		       "MenuButton: cannot specify mnemonic for submenu\n"));
	    } else if (bw->primitive.mnemonic != newbw->primitive.mnemonic)
		was_resized = (Boolean) TRUE;
	}

	/*
	 *  Has the accelerator changed?  -JMK
	 *  
	 */

	if (_OlIsGadget (newbw)) {
	    if (XtIsSubclass(new, menuButtonGadgetClass) &&
		((ButtonGadget)newbw)->event.accelerator_text) {
		    ((ButtonGadget)newbw)->event.accelerator = NULL;
		    ((ButtonGadget)newbw)->event.accelerator_text = NULL;
		    ((ButtonGadget)newbw)->event.meta_key = False;
		    ((ButtonGadget)newbw)->event.qualifier_text = NULL;
		    OlWarning(dgettext(OlMsgsDomain,
		       "MenuButton: cannot specify accelerator for submenu\n"));
	    } else if ((((ButtonGadget) bw)->event.accelerator !=
		((ButtonGadget) newbw)->event.accelerator) ||
	    	((ButtonGadget) bw)->event.accelerator_text !=
		((ButtonGadget) newbw)->event.accelerator_text) {

		if (IsMenuMode(newbw))
		    newbp->menu_has_meta =((ButtonGadget)newbw)->event.meta_key;
		was_resized = (Boolean) TRUE;
	    }
	} else {
	    if (XtIsSubclass(new, menuButtonWidgetClass) &&
		newbw->primitive.accelerator_text) {
		    newbw->primitive.accelerator = NULL;
		    newbw->primitive.accelerator_text = NULL;
		    newbw->primitive.meta_key = NULL;
		    newbw->primitive.qualifier_text = NULL;
		    OlWarning(dgettext(OlMsgsDomain,
		       "MenuButton: cannot specify accelerator for submenu\n"));
	    } else if ((bw->primitive.accelerator != 
			newbw->primitive.accelerator) ||
	    	(bw->primitive.accelerator_text != 
			newbw->primitive.accelerator_text)) {

		if (IsMenuMode(newbw))
		    newbp->menu_has_meta = newbw->primitive.meta_key;
		was_resized = (Boolean) TRUE;
	    }
	}
	if (bp->menu_has_meta != newbp->menu_has_meta)
	    was_resized = True;

	/*
	 *  Has the label_image resource changed?
	 */
	if (bp->label_image != newbp->label_image) {
		if (newbp->label_type == OL_IMAGE)  {
			SetLabelTile(newbw);
			was_resized = (Boolean) TRUE;
			}
	}

	/*
	 *  Has the label_tile resource changed?
	 */
	if (bp->label_tile != newbp->label_tile) {
		if (newbp->label_tile &&
			newbp->label_type == OL_IMAGE)  {
			SetLabelTile(newbw);
			needs_redisplay = (Boolean) TRUE;
			}
	}

	/*
	 *  If the recompute_size resource has changed, then the size
	 *  of the button may change.
	 */
	if (bp->recompute_size != newbp->recompute_size)  {
		was_resized = (Boolean) TRUE;
	}

	/*
	 *  If the foreground or background have changed,
	 *  then the button's GCs must be destroyed and recreated.
	 *  Note that changing the values of the foreground and
	 *  background in the current GCs does not work
	 *  because the GCs are cached and the changes would
	 *  affect other widgets which share the GCs.
	 */

	if (!_OlIsGadget(newbw))  {
		if (newbw->core.background_pixel != bw->core.background_pixel) 
			newbp->background_pixel = newbw->core.background_pixel;
	}

	if (find_button_font_color(bw) != find_button_font_color(newbw) ||
		bp->background_pixel != newbp->background_pixel ||
		find_button_font(bw) != find_button_font(newbw) ||
		find_button_scale(bw) != find_button_scale(newbw))  {

		if (newbp->label_type == OL_IMAGE && newbp->label_tile)
		{
			SetLabelTile((ButtonWidget)newbp);
		}
		else
		{
			GetGCs(newbw);
		}
		
		needs_redisplay = TRUE;
	}

	newbw->core.border_width= (Dimension) 0; 

	/*
	 *  Now we have dealt with all of the resources which could
	 *  possibly change the size of the button, so it is time
	 *  to do the geometry request.
	 */
	if (bw->core.width != newbw->core.width ||
		bw->core.height != newbw->core.height ||
		was_resized) {

		/*
		 *  First do any calculations necessary to determine
		 *  the new size of the button.
		 */
	        SetDimensions(newbw);

		if(newbp->normal_height == (Dimension) 0) 	/* no 0x0 */
		    newbp->normal_height = (Dimension) 1;
		if (newbp->normal_width <= (Dimension) 3) {/* Xlib calc. assumption */ 
		    newbp->normal_height = (Dimension) 4;
		}

		if(newbw->core.height == (Dimension) 0) 	/* no 0x0 */
				newbw->core.height = (Dimension) 1;
		if (newbw->core.width <= (Dimension) 3) /* X assumption */ 
				newbw->core.height = (Dimension) 4;

		/*
		 *  Always return true from SetValues if the core.width
		 *  or core.height have changed.
		 */
		needs_redisplay = (Boolean) TRUE;
		}

	/*
	 *  Has the label_justify resource changed?
	 */
	if (newbp->label_justify != bp->label_justify)  {
		if (newbp->label_justify != OL_LEFT
			&& newbp->label_justify != OL_CENTER) {
			newbp->label_justify=bp->label_justify;
			OlWarning(dgettext(OlMsgsDomain,
			  "Invalid XtNlabelJustify value: Not changed\n"));
		}
		else
			needs_redisplay = (Boolean) TRUE;
	}

	/*
	 *  Has the is_default resource changed?
	 */
	if (bp->is_default != newbp->is_default)  {
		if (bp->is_default)
			bp->is_default = True;	/* make it boolean */

		_OlSetDefault(new, newbp->is_default);
		needs_redisplay = (Boolean) TRUE;
	}

	/*
	 *  Has the sensitive resource changed?
	 */
	if (XtIsSensitive((Widget)newbw) != XtIsSensitive((Widget)bw))  {
		needs_redisplay = (Boolean) TRUE;
		GetGCs((ButtonWidget)new);
	}

	/*
	 *  Has the set resource changed?
	 */
	if (newbp->set != bp->set)  {
		needs_redisplay = (Boolean) TRUE;

		if (newbp->set)
			newbp->set = (Boolean) TRUE;
	}

	/*
	 *  Has the dim resource changed?
	 */
	if (newbp->dim != bp->dim)  {
		needs_redisplay = (Boolean) TRUE;

		if (newbp->dim)
			newbp->dim = (Boolean) TRUE;
	}
	
	/*
	 *  Has the busy resource changed?
	 */
	if (newbp->busy != bp->busy)  {
		needs_redisplay = (Boolean) TRUE;

		if (newbp->busy)
			newbp->busy = (Boolean) TRUE;
		else
			newbp->internal_busy=(Boolean) FALSE;
	}

	/*
	 *  Has the button preview resource changed?
	 */
	if (newbp->preview != (Widget)NULL)  {
		_OlButtonPreview((ButtonWidget)newbp->preview, 
			(ButtonWidget)new);
		newbp->preview = (Widget)NULL;
	}

	/*
	 *  The window always uses the parent's background.  (Widgets only)
	 */
	if (XtIsRealized (new) && !_OlIsGadget (new) &&
	    (XtClass(new) != buttonWidgetClass) &&
	    new->core.background_pixmap != ParentRelative)
	{
	    XSetWindowBackgroundPixmap (XtDisplay (new), XtWindow (new),
					ParentRelative);
	    new->core.background_pixmap = ParentRelative;
	    needs_redisplay = (Boolean) TRUE;
	}

	return (needs_redisplay || was_resized);

}	/* SetValues */


/*********************function*header************************
 *  HighlightHandler - This function get new GCs to change the
 *	button and label colors as required for the mode and
 *	forces a redraw of the button.
 */
/*ARGSUSED1*/
static void
HighlightHandler (Widget w, OlDefine highlight_type)
{
    if(_OlInputFocusFeedback(w) == OL_SUPERCARET)
	return;
    GetGCs((ButtonWidget)w);
    ButtonRedisplay(w, (XEvent*)NULL, (Region)NULL);
}


/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */


/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */


/*
 ************************************************************
 *
 *  _OlButtonPreview - This function displays the source widget
 *	in the destination widget.  It is used to preview
 *	the default choice in a menu.
 *
 *********************function*header************************
 */
void
_OlButtonPreview(ButtonWidget dest, ButtonWidget src)
{
	ButtonPart	*button_part = find_button_part(src);
        unsigned	flags;
	ButtonLabel	lbl;
	OlSizeProc      sizeProc;
	OlDrawProc      drawProc;
	Boolean		wasSet;
	OlgxAttrs	*pAttrs;

	if(!XtIsRealized((Widget)dest))
		return;

	/* We must clear the destination window before drawing */
	/* OLGX_TODO: may only need this in the OL_LABEL case, if so move
	 * it to be within that case.
	 */
	/*******************
	XClearArea(XtDisplayOfObject((Widget)dest), 
		XtWindowOfObject((Widget)dest),
		_OlXTrans(dest, 0), _OlYTrans(dest, 0),
		dest->core.width, dest->core.height, False);
	*******************/

	/* Populate the label structure from the new button part.  The
	 * button is drawn selected.
	 */
	wasSet = button_part->set;
	button_part->set = True;
	setLabel(src, &lbl, &sizeProc, &drawProc);
	button_part->set = wasSet;

	/* Use only a subset of the flags from the button part */
	switch (button_part->button_type)  {
	case OL_LABEL:
	    /* OLGX_TODO: set flags...any needed values:? */
	    flags = 0;
	    (*drawProc)(XtScreenOfObject((Widget)dest),
			XtWindowOfObject((Widget)dest),
			button_part->pAttrs,
			_OlXTrans(dest, 0), _OlYTrans(dest, 0),
			dest->core.width, dest->core.height, (XtPointer)&lbl,
			button_part->button_type, flags);
	    break;

	case OL_RECTBUTTON:
	    flags = OLGX_INVOKED;
	    if (button_part->is_default)
		flags |= OLGX_DEFAULT;
	    if (button_part->dim)
		flags |= OLGX_INACTIVE;
	    if (button_part->label_type == OL_IMAGE) {
		flags |= OLGX_LABEL_IS_XIMAGE;
	    } else if (button_part->label_type != OL_PROC) {
		if (lbl.text.text_format == OL_WC_STR_REP)
		    flags |= OLGX_LABEL_IS_WCS;
	    }

	    (*drawProc)(XtScreenOfObject((Widget)dest),
		XtWindowOfObject((Widget)dest), button_part->pAttrs, 
		_OlXTrans(dest, 0), _OlYTrans(dest, 0),
		dest->core.width, dest->core.height, 
		(XtPointer)&lbl, button_part->button_type, flags);
	    break;

	case OL_OBLONG:
	case OL_BUTTONSTACK:
	default:
	    flags = OLGX_INVOKED;
	    if (button_part->is_default)
		flags |= OLGX_DEFAULT;
	    if ((button_part->busy) || (button_part->internal_busy))
		flags |= OLGX_BUSY;

	    switch (button_part->shell_behavior) {
	    case PinnedMenu:
	    case PressDragReleaseMenu:
	    case StayUpMenu:
	    case UnpinnedMenu:
		flags |= OLGX_MENU_ITEM;
		break;
	    }

	    if (button_part->button_type == OL_BUTTONSTACK)
		flags |= (button_part->menumark == OL_RIGHT) ?
		    OLGX_HORIZ_MENU_MARK : OLGX_VERT_MENU_MARK;

	    if (button_part->label_type == OL_IMAGE)
		flags |= OLGX_LABEL_IS_XIMAGE;
	    else if (button_part->label_type != OL_PROC) {
		if (lbl.text.text_format == OL_WC_STR_REP)
		    flags |= OLGX_LABEL_IS_WCS;
	    }

	    if (button_part->label_tile) 
		pAttrs = button_part->pHighlightAttrs;
	    else
		pAttrs = button_part->pAttrs;

	    (*drawProc)(XtScreenOfObject((Widget)dest),
		XtWindowOfObject((Widget)dest), pAttrs, _OlXTrans(dest, 0),
		_OlYTrans(dest, 0), dest->core.width, dest->core.height,
		(XtPointer)&lbl, button_part->button_type, flags);
	    break;
	}

	XFlush(XtDisplayOfObject((Widget)dest));
}	/*  _OlButtonPreview  */


/*
 ************************************************************
 *
 *  _OlDrawHighlightButton - this function draws the oblong
 *	button in its highlighted state.
 *
 *********************function*header************************
 */
void
_OlDrawHighlightButton(ButtonWidget bw)
{
	ButtonPart *	bp = find_button_part(bw);
	OlgxAttrs *	pAttrs;
	ButtonLabel	lbl;
	OlSizeProc      sizeProc;
	OlDrawProc      drawProc;
	int		flags;
	Boolean		wasSet;

	if(!XtIsRealized((Widget)bw))
		return;

	wasSet = bp->set;
	bp->set = True;
	flags = getOblongFlags((Widget)bw);
	setLabel(bw, &lbl, &sizeProc, &drawProc);

	if (bp->label_tile) 
	    pAttrs = bp->pHighlightAttrs;
	else
	    pAttrs = bp->pAttrs;

        (*drawProc)(XtScreenOfObject((Widget)bw), XtWindowOfObject((Widget)bw),
            pAttrs, _OlXTrans(bw, 0), _OlYTrans(bw, 0), bw->core.width,
	    bw->core.height, (XtPointer)&lbl, bp->button_type, flags);
	bp->set = wasSet;

	XFlush(XtDisplayOfObject((Widget)bw));

}  /* _OlDrawHighlightButton  */


/*
 ************************************************************
 *
 *  _OlDrawNormalButton - this function draws the oblong
 *	button in its normal state.
 *
 *********************function*header************************
 */
void
_OlDrawNormalButton(ButtonWidget bw)
{
	ButtonPart	*bp = find_button_part(bw);
	ButtonLabel	lbl;
	OlSizeProc	sizeProc;
	OlDrawProc	drawProc;
	int		flags;
	Boolean		wasSet;

	if(!XtIsRealized((Widget)bw))
		return;

	wasSet = bp->set;

	/* 2-D oblong buttons that have input focus are drawn as if they
	 * they are set if the input focus color conflicts with either the
	 * foreground or background color.  In this case, the flags returned
	 * from getOblongFlags will contain OLGX_INVOKED.  If this flag is
	 * set, then the bp->set must also be True to get the correct label
	 * colors.  Yuck.
	 */
	bp->set = False;
	flags = getOblongFlags((Widget)bw);
	if (flags & OLGX_INVOKED)
	    bp->set = True;
	setLabel(bw, &lbl, &sizeProc, &drawProc);

	(*drawProc)(XtScreenOfObject((Widget)bw), XtWindowOfObject((Widget)bw), 
	    bp->pAttrs, _OlXTrans(bw, 0), _OlYTrans(bw, 0), bw->core.width,
	    bw->core.height, (XtPointer)&lbl, bp->button_type, flags);
	bp->set = wasSet;
}  /* _OlDrawNormalButton  */
