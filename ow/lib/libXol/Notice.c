#pragma ident	"@(#)Notice.c	302.13	97/03/26 lib/libXol SMI"	/* notice:src/Notice.c 1.68	*/

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

/*******************************file*header*******************************
    Description: Notice.c - OPEN LOOK(TM) Notice Widget
*/


#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/ControlAre.h>
#include <Xol/EventObjP.h>	/* for gadgets */
#include <Xol/NoticeP.h>
#include <Xol/StaticText.h>
#include <Xol/VendorI.h>
#include <Xol/OlI18nP.h>


/**************************forward*declarations***************************

    Forward function definitions listed by category:
		1. Private functions
		2. Class   functions
		3. Action  functions
		4. Public  functions
 */
						/* private procedures */
static void	BusyState(NoticeShellWidget nw, Boolean busy);
static Widget	GetDefaultControl(CompositeWidget w);
static void	PositionNotice(NoticeShellWidget nw);
						/* class procedures */
static void	Destroy(Widget w);
static Boolean	ActivateWidget (Widget, OlVirtualName, XtPointer);
static void	WMMsgHandler (Widget, OlDefine, OlWMProtocolVerify *);
static void	ClassInitialize(void);
static void	Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);
static Boolean	SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);
						/* action procedures */
static void	LeaveEH(Widget nw, XtPointer client_data, XEvent *event, Boolean *cont_to_dispatch);
static void	MapEH(Widget nw, XtPointer client_data, XEvent *event, Boolean *cont_to_dispatch);
static void	PopdownCB(Widget nw, XtPointer closure, XtPointer call_data);
static void	warpPointer(Widget w, XtPointer client_data, XEvent *event, Boolean *cont_to_dispatch);
static void	PopupCB(Widget nw, XtPointer closure, XtPointer call_data);
static void	PostSelectCB(Widget nw, XtPointer closure, XtPointer call_data);
						/* public procedures */

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/
#define LeaveParams		LeaveWindowMask, False, LeaveEH, NULL
#define AddLeaveEH(w)		XtAddEventHandler(w, LeaveParams)
#define RemoveLeaveEH(w)	XtRemoveEventHandler(w, LeaveParams)

#define MapParams		StructureNotifyMask, False, MapEH, NULL
#define AddMapEH(w)		XtAddEventHandler(w, MapParams)
#define RemoveMapEH(w)		XtRemoveEventHandler(w, MapParams)

#define NPART(w)		( &((NoticeShellWidget)(w))->notice_shell )

#ifdef DEBUG
Boolean _NDebug = False;
#define DPRINT(x) if (_Ndebug == True) (void)fprintf x
#else
#define DPRINT(x)
#endif

/***********************widget*translations*actions***********************
    Translations and Actions
*/
/* None */

/****************************widget*resources*****************************
 *
 * Notice Resources
 */

#define OFFSET(field)  XtOffsetOf(NoticeShellRec, notice_shell.field)

static XtResource resources[] =
  {
    {XtNtextFormat, XtCTextFormat, XtROlStrRep,sizeof(OlStrRep),
	   OFFSET(text_format),XtRCallProc,(XtPointer)_OlGetDefaultTextFormat},

    { XtNcontrolArea, XtCControlArea, XtRWidget, sizeof(Widget),
	  OFFSET(control), XtRWidget, (XtPointer)NULL },

    { XtNemanateWidget, XtCEmanateWidget, XtRWidget, sizeof(Widget),
	  OFFSET(emanate), XtRWidget, (XtPointer)NULL },

    { XtNpointerWarping, XtCPointerWarping, XtRBoolean, sizeof(Boolean),
	  OFFSET(warp_pointer), XtRImmediate, (XtPointer)OL_POINTER_WARPING },

    { XtNtextArea, XtCTextArea, XtRWidget, sizeof(Widget),
	  OFFSET(text), XtRWidget, (XtPointer)NULL },

  };

#undef OFFSET

/*************************************************************************
    Define Class Extension Resource List
*/
#define OFFSET(field)	XtOffsetOf(OlVendorPartExtensionRec, field)

static XtResource ext_resources[] =
  {
    { XtNmenuButton, XtCMenuButton, XtRBoolean, sizeof(Boolean),
	OFFSET(menu_button), XtRImmediate, (XtPointer)False },

    { XtNpushpin, XtCPushpin, XtROlDefine, sizeof(OlDefine),
	OFFSET(pushpin), XtRImmediate, (XtPointer)OL_NONE },

    { XtNresizeCorners, XtCResizeCorners, XtRBoolean, sizeof(Boolean),
	OFFSET(resize_corners), XtRImmediate, (XtPointer)False },

    { XtNmenuType, XtCMenuType, XtROlDefine, sizeof(OlDefine),
	OFFSET(menu_type), XtRImmediate, (XtPointer)OL_NONE },

    { XtNwindowHeader, XtCWindowHeader, XtRBoolean, sizeof(Boolean),
	OFFSET(window_header), XtRImmediate, (XtPointer)False },

	{ XtNfooterPresent, XtCFooterPresent, XtRBoolean, sizeof(Boolean),
	OFFSET(footer_present), XtRImmediate, (XtPointer)False },

	{ XtNimFooterPresent, XtCImFooterPresent, XtRBoolean, sizeof(Boolean),
	OFFSET(im_footer_present), XtRImmediate, (XtPointer)False },

    { XtNwinType, XtCWinType, XtROlDefine, sizeof(OlDefine),
	OFFSET(win_type), XtRImmediate, (XtPointer)OL_WT_NOTICE },
  };

/*
 *************************************************************************
 *
 * Define Class Extension Records
 *
 *************************class*extension*records*************************
 */

static OlVendorClassExtensionRec vendor_extension_rec =
  {
    {
        NULL,                                   /* next_extension       */
        NULLQUARK,                              /* record_type          */
        OlVendorClassExtensionVersion,          /* version              */
        sizeof(OlVendorClassExtensionRec)       /* record_size          */
    },  /* End of OlClassExtension header       */
        ext_resources,                          /* resources            */
        XtNumber(ext_resources),                /* num_resources        */
        NULL,                                   /* private              */
        (OlSetDefaultProc)NULL,                 /* set_default          */
	NULL,					/* get_default 		*/
	NULL,					/* destroy		*/
	NULL,					/* initialize		*/
        NULL,                                   /* set_values           */
        NULL,                                   /* get_values           */
        XtInheritTraversalHandler,              /* traversal handler    */
        XtInheritHighlightHandler,    		/* highlight handler    */
        ActivateWidget,				/* activate function    */
        NULL,              			/* event_procs          */
        0,              			/* num_event_procs      */
        NULL,					/* part_list            */
	{ NULL, 0 },				/* dyn_data		*/
	NULL,					/* transparent_proc	*/
	WMMsgHandler,				/* wm_proc		*/
	False,					/* override_callback	*/
  }, *vendor_extension = &vendor_extension_rec;

/***************************widget*class*record***************************
 *
 * Full class record constant
 */

NoticeShellClassRec noticeShellClassRec = {
  {
/* core_class fields		*/
    /* superclass		*/	(WidgetClass) &transientShellClassRec,
    /* class_name		*/	"NoticeShell",
    /* widget_size		*/	sizeof(NoticeShellRec),
    /* class_initialize		*/	ClassInitialize,
    /* class_part_init		*/	NULL,
    /* class_inited		*/	False,
    /* initialize		*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	XtInheritRealize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* num_resources		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	True,
    /* compress_exposure	*/	True,
    /* compress_enterleave	*/	True,
    /* visible_interest		*/	False,
    /* destroy			*/	Destroy,
    /* resize			*/	XtInheritResize,
    /* expose			*/	NULL,
    /* set_values		*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus		*/	XtInheritAcceptFocus,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	XtInheritTranslations,
    /* query geometry		*/	NULL
  },{
/* composite_class fields	*/
    /* geometry_manager		*/	XtInheritGeometryManager,
    /* change_managed		*/	XtInheritChangeManaged,
    /* insert_child		*/	XtInheritInsertChild,
    /* delete_child		*/	XtInheritDeleteChild,
    /* extension		*/	NULL
  },{
/* shell_class fields		*/
					NULL
  },{
/* WHShell_class fields		*/
					NULL
  },{
/* VendorShell_class fields	*/
					(XtPointer)&vendor_extension_rec
  },{
/* TransientShell_class fields	*/
					NULL
  },{
/* NoticeShell_class fields	*/
					NULL
  }
};

WidgetClass noticeShellWidgetClass = (WidgetClass)&noticeShellClassRec;


/***************************private*procedures****************************
 *
 * Private Functions
 *
 */

/******************************function*header****************************
 * BusyState-  toggle busy state
 */

static void
BusyState(NoticeShellWidget nw, Boolean busy)
{
    Arg		state[1];
    Widget	base;

    XtSetArg(state[0], XtNbusy, busy);

    /* [un]busy ([un]freeze) application's base window
     * 1st, get base window (child of root)
     */
    for (base = (Widget)nw->notice_shell.emanate;
	 XtParent(base) != NULL; base = XtParent(base))
	;

    if (XtIsRealized(base))
	XtSetValues(base, state, 1);

    /* [un]busy the control */
    XtSetValues(nw->notice_shell.emanate, state, XtNumber(state));
}

/******************************function*header****************************
 * GetDefaultControl():  look for control in control area with default set.
 *		If none found, make the 1st control the default control.
 */

static Widget
GetDefaultControl(CompositeWidget w)
{
    Cardinal	childCnt = w->composite.num_children;
    WidgetList	kid = w->composite.children;
    Arg		defaultArg[1];
    Cardinal	i;
    Boolean	is_default;

    if (childCnt == 0)
	return (NULL);

    XtSetArg(defaultArg[0], XtNdefault, &is_default);

    for (i = 0; i < childCnt; i++)
    {
	/* Here, don't look for oblongButton's just look for
	   anything that understands XtNdefault
	 */
	XtGetValues(kid[i], defaultArg, XtNumber(defaultArg));
	if (is_default)
	    return (kid[i]);
    }

    /* Falling thru means no default is set.  Use 1st child without
	regard for whether it understands the resource 'XtNdefault'
    */
    XtSetArg(defaultArg[0], XtNdefault, True);
    XtSetValues(kid[0], defaultArg, XtNumber(defaultArg));
    return (kid[0]);
}

/******************************function*header****************************
 * PositionNotice()
 */

static void
PositionNotice(NoticeShellWidget nw)
{
    Widget	emanateW = nw->notice_shell.emanate;
    int		emanate_x, emanate_y;	/* must be int's */
    Dimension	emanate_w, emanate_h;
    Position	notice_x, notice_y;
    Dimension	notice_w, notice_h;
    Dimension	screen_w, screen_h;
    int		delta;			/* must be signed */
    Window	junkWin;
    Boolean	shifted_left;
    Boolean	use_emanate;
    Position	current_x, current_y;


    /* Notice must be Realized (geometry is needed ) */

    if (!XtIsRealized((Widget)nw))
	XtRealizeWidget((Widget)nw);

    /* Get origin of emanate widget relative to root window.  If emanate
	widget has no window (not realized) or its (window) parent is
	root, don't even try to translate coordinates
     */
    if (XtIsRealized(emanateW) &&
	(XtWindowOfObject(emanateW) != RootWindowOfScreen(XtScreen(nw))))
    {
	XTranslateCoordinates(XtDisplay(nw), XtWindowOfObject(emanateW),
			      RootWindowOfScreen(XtScreen(nw)), 0, 0,
			      &emanate_x, &emanate_y, &junkWin);
	use_emanate = True;

	/* If 'emamante' is a gadget, coord's are actually parent's coord's.
	   Must adjust to gadget coord's.
	*/
	if (_OlIsGadget(emanateW))
	{
	    emanate_x += emanateW->core.x;
	    emanate_y += emanateW->core.y;
	}

    } else {
	use_emanate = False;
    }

    screen_w	= _OlScreenWidth((Widget)nw);
    screen_h	= _OlScreenHeight((Widget)nw);
    notice_w	= _OlWidgetWidth(nw);
    notice_h	= _OlWidgetHeight(nw);

    if (use_emanate)
    {
	emanate_w = _OlWidgetWidth(emanateW);
	emanate_h = _OlWidgetHeight(emanateW);

	if (emanate_x < ((int)screen_w / 2))
		notice_x = (Position)(emanate_x + emanate_w);	/* right of control */
	else 
		notice_x = (Position)(emanate_x - notice_w);	/* left of control */

	if (emanate_y < ((int)screen_h / 2))
		notice_y = (Position)(emanate_y + emanate_h); /* below */
	else if (emanate_y > ((int)screen_h / 2))
		notice_y = (Position)(emanate_y - notice_h); /* above */
	else
		notice_y = (Position)emanate_y;		/* level w/control */

	/* consider horizontal axis (x) first */
	delta = screen_w - notice_w;
	if (notice_x > delta)			/* too wide */
	{
	    notice_x = (Position)delta;		/* shift left */
	    shifted_left = True;
	} else {
	    shifted_left = False;
	}

	/* consider vertical axis (y) */
	delta = screen_h - notice_h;
	if (notice_y > delta)			/* too tall */
	{
	    if (shifted_left)
	    {
		notice_y = emanate_y - notice_h; /* above control */
		if (notice_y < 0)		/* still no good */
		{
		    notice_y = (Position)delta; /* bottom of screen */
		    delta = emanate_x - notice_w;
		    if (delta > 0)		/* left of control */
			notice_x = (Position)delta;
		}

	    } else {
		notice_y = (Position)delta; /* shift up */
	    }
	}

    } else {			/* don't use emanate: center notice */
	notice_x = (Position)(screen_w - notice_w) / (Position)2;
	notice_y = (Position)(screen_h - notice_h) / (Position)2;
    }

/* Code Modification: Earlier version used XtMoveWidget to set the
   Notice's position. But this caused probs in Pointer warping, due
   to a race condition between XWarpPointer and the actual move&map of
   the Notice after suitable WM interactions. And this would result,
   in the pointer warping to the previous position of the Notice - if 
   the Notice had been moved by the user to a new position.
	Hence I used XtSetVals on the widgets' XtNx & XtNy resources.
   Since the RootShellGeoMgr would then wait for the ConfigureNotify
   from the WM, to ensure that the "move" has completed. 
	Why the comparison before SetVals ? Because if the notice is
   being mapped/popped at the same position as in its previous mapped
   state, then I would'nt get a ConfigureNotify. And the RootGM would be
   waiting indefintely for it ..
	I can't use GetValues to get the x & y position of the widget,
   as the Core's x & y resources may not reflect the actual state of
   the window, during our present Unmapped state. Hence rely on 
   XtTranslateCoords ...
	Use XtTranslateCoords to convert (0,0) which is the upper
   left corner INSIDE THE BORDER ,of our window. But we actually
   want the (x,y) position of our window, which is the upper left
   corner OUTSIDE the border. Hence ..
*/
    XtTranslateCoords((Widget)nw, 0,0,&current_x,&current_y);
    if (((current_x - nw->core.border_width) != notice_x) ||
    	((current_y - nw->core.border_width) != notice_y)) {
		Arg args[2];
		XtSetArg(args[0], XtNx, notice_x);
		XtSetArg(args[1], XtNy, notice_y);
		XtSetValues((Widget)nw, args, 2);
	}
}

/*************************************************************************
 *
 * Class Procedures
 *
 */

/******************************function*header****************************
 * Destroy-
 */
static void
Destroy(Widget w)
{
    RemoveLeaveEH(w);
    RemoveMapEH(w);
}

/****************************procedure*header*****************************
    ActivateWidget - this procedure allows external forces to activate this
    widget indirectly.
*/
/* ARGSUSED */
static Boolean
ActivateWidget (Widget w, OlVirtualName type, XtPointer data)
{
    Boolean	ret_val;

    if (type == OL_DEFAULTACTION)
    {
	ret_val = True;
	w = _OlGetDefault(w);

	if (w != (Widget)NULL)
	    OlActivateWidget(w, OL_SELECTKEY, (XtPointer)NULL);

    } else if (type == OL_CANCEL) {
	ret_val = True;

	_OlBeepDisplay(w, 1);

    } else {
	ret_val = False;
    }

    return(ret_val);
} /* END OF ActivateWidget() */

static void
WMMsgHandler (Widget w, OlDefine action, OlWMProtocolVerify *wmpv)
{
    if (wmpv->msgtype == OL_WM_DELETE_WINDOW)
    {
	switch(action)
	{
	case OL_DEFAULTACTION:
	case OL_DISMISS:
	    XtPopdown(w);
	    break;

	case OL_QUIT:
	    XtUnmapWidget(w);
	    exit(EXIT_SUCCESS);

	case OL_DESTROY:
	    XtDestroyWidget(w);
	    break;
	}
    }
}


static void
ClassInitialize(void)
{
    vendor_extension->header.record_type = OlXrmVendorClassExtension;
}

/******************************function*header****************************
 * Initialize():  override any superclass resources of interest and
 *		establish own resource defaults.
 */

/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    NoticeShellWidget   new_ns = (NoticeShellWidget)new;
    NoticeShellPart *	nPart = &(new_ns->notice_shell);
    Cardinal		m;
    MaskArg		mArgs[10];
    ArgList		mergedArgs;
    Cardinal		mergedCnt;
    Widget		pane;
	OlVendorPartExtension part =
			_OlGetVendorPartExtension(_OlGetShellOfWidget(new));
    static Arg		pane_args[] = {
				{ XtNcenter, (XtArgVal) True },
				{ XtNlayoutType, (XtArgVal) OL_FIXEDCOLS },
				{ XtNsameSize, (XtArgVal) OL_NONE },
				{ XtNvSpace, (XtArgVal) 0 },
				};

    /* postSelect callbacks must be added at Create time. */
    static XtCallbackRec popdown_cb[] = {
	{ NULL, NULL },		/* filled in below */
        { NULL, NULL }		/* delimiter */
    };

    /* Add event handler to get window-leave notification.  This is used
	to decide if pointer should be unwarped.  Add event handler to
	get map notification.  This is to beep the display.  Beeping the
	display when Popped Up is too early when system is loaded so
	defer till when window is mapped.
	
     */
    AddLeaveEH((Widget)new_ns);
    AddMapEH((Widget)new_ns);

    /****************************************************************
     * CREATE FIXED CHILDREN
     */

    /* create the sole child widget of this notice/shell widget */
    pane = XtCreateManagedWidget("pane", controlAreaWidgetClass,
				 (Widget)new_ns,	/* parent (self) */
				 pane_args, XtNumber(pane_args));

    /* create the component parts: text area & control area.
     * some resources can be specified/overidden by the appl:
     * OL_OVERRIDE_PAIR: full control of these resources
     * OL_DEFAULT_PAIR: the appl has some say about these resources
     * OL_SOURCE_PAIR: don't care, include them on appl's behalf
     */
    /*
     * create text area: will contain appl's message
     */
    m = 0;
    _OlSetMaskArg(mArgs[m], XtNalignment, 0, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNfont, NULL, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNfontColor, 0, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNlineSpace, 0, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNstring, NULL, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNstrip, True, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNwrap, True, OL_SOURCE_PAIR); m++;
_OlSetMaskArg(mArgs[m], XtNtextFormat, nPart->text_format, OL_SOURCE_PAIR); m++;

    _OlComposeArgList(args, *num_args, mArgs, m, &mergedArgs, &mergedCnt);
	
    nPart->text = XtCreateManagedWidget("textarea",
					staticTextWidgetClass, pane,
					mergedArgs, mergedCnt);
    XtFree((char*)mergedArgs);

    /*
     * create control area: will contain application's controls
     */

    popdown_cb[0].callback	= PostSelectCB;
    popdown_cb[0].closure	= (XtPointer)new_ns;

    m = 0;
    _OlSetMaskArg(mArgs[m], XtNhPad, 0, OL_DEFAULT_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNhSpace, 0, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNlayoutType, OL_FIXEDROWS, OL_DEFAULT_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNmeasure, 0, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNsameSize, 0, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNvPad, 0, OL_DEFAULT_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNvSpace, 0, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNpostSelect, popdown_cb, OL_OVERRIDE_PAIR); m++;

    _OlComposeArgList(args, *num_args, mArgs, m, &mergedArgs, &mergedCnt);
	
    nPart->control = XtCreateManagedWidget("controlarea",
					   controlAreaWidgetClass,
					   pane, mergedArgs, mergedCnt);
    XtFree((char*)mergedArgs);

    /* add callbacks so we know when we're being popped up and down */
    XtAddCallback((Widget)new_ns, XtNpopupCallback, 
					OlCallbackPopdownMenu, NULL);
    XtAddCallback((Widget)new_ns, XtNpopupCallback, PopupCB, NULL);
    XtAddCallback((Widget)new_ns, XtNpopdownCallback, PopdownCB, NULL);

	if (part && part->footer_present)
		part->footer_present = False;
	if (part && part->im_footer_present)
		part->im_footer_present = False;
}

/******************************function*header****************************
    SetValues- check for attempts to set read-only resources.  Pass on
    certain resource changes to Text.
*/

/* ARGSUSED */
static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    NoticeShellWidget current_ns = (NoticeShellWidget)current;
    NoticeShellWidget new_ns = (NoticeShellWidget)new;
    Cardinal	m;
    MaskArg	mArgs[2];
    ArgList	mergedArgs;
    Cardinal	mergedCnt;
    Boolean	redisplay = False;

	 /* always reset text format */
	NPART(new_ns)->text_format = NPART(current_ns)->text_format;

    if ((NPART(new_ns)->text != NPART(current_ns)->text) ||
      (NPART(new_ns)->control != NPART(current_ns)->control))
	OlWarning(dgettext(OlMsgsDomain,
		"Notice: SetValues- cannot set read-only resources"));

    /* "forward" font & fontColor changes to the Static Text */
    m = 0;
    _OlSetMaskArg(mArgs[m], XtNfont, NULL, OL_SOURCE_PAIR); m++;
    _OlSetMaskArg(mArgs[m], XtNfontColor, 0, OL_SOURCE_PAIR); m++;
    _OlComposeArgList(args, *num_args, mArgs, m, &mergedArgs, &mergedCnt);
	
    if (mergedCnt != 0)
    {
	XtSetValues(NPART(new_ns)->text, mergedArgs, mergedCnt);
	XtFree((char*)mergedArgs);

	redisplay = True;
    }

    return (redisplay);
}

/*************************************************************************
 *
 * Action Procedures
 *
 */

/******************************function*header****************************
 * LeaveEH- 
 */

/* ARGSUSED */
static void
LeaveEH(Widget w, XtPointer client_data, XEvent *event, Boolean *cont_to_dispatch)
{
    NoticeShellWidget nw = (NoticeShellWidget)w;
    nw->notice_shell.do_unwarp = False;
}

/******************************function*header****************************
 * MapEH- 
 */

/* ARGSUSED */
static void
MapEH(Widget w, XtPointer client_data, XEvent *event, Boolean *cont_to_dispatch)
{
    if (event->type == MapNotify)
	_OlBeepDisplay(w, 1);	/* figures out if beep is needed */
	
}

/******************************function*header****************************
 * PopdownCB():  Notice pops down automatically like other OPEN LOOK
 *	popup windows.  This is done by using button postSelect callback
 * (specified when control area is created (see Initialize)).
 * The Xt-supplied callback procedure, XtCallbackPopdown, is used to
 * simply pop the Notice down.  PopdownCB is called when the Notice pops
 * down.  Here is where the pointer should be unwarped and the control &
 * application window un-busied.
 */

static void
PopdownCB(Widget w, XtPointer closure, XtPointer call_data)
{
    NoticeShellWidget nw = (NoticeShellWidget)w;
    NoticeShellPart * nPart = &(nw->notice_shell);

    if (nPart->warp_pointer && nPart->do_unwarp)
	/* restore position of pointer */
	XWarpPointer(XtDisplay(nw), None,
		     RootWindowOfScreen(XtScreen(nw)), 0, 0, 0, 0,
		     nPart->root_x, nPart->root_y);

    /* "unbusy" the appropriate things */
    BusyState(nw, False);
}

static void
warpPointer(Widget w, XtPointer client_data,
	XEvent *event, Boolean *cont_to_dispatch) 
{
	int             dest_x, dest_y;
	Dimension       height, width;
	Widget          fw = (Widget)client_data;

	if (event->type != MapNotify || 
	    event->xmap.window != XtWindowOfObject(w))
		return;

	XtVaGetValues(fw,
		XtNwidth,	&width,
		XtNheight,	&height,
		NULL);

	dest_x = (int)width/2;
	dest_y = ((int)height >= 1 ? (int)height/2: 0);

	/*
	 * Transform to parents coordinates
	 * as a gadget does not have its own window.
	 */
	if (_OlIsGadget(fw)) {
		Position x,y;

		XtVaGetValues(fw,
			XtNx,	&x,
			XtNy,	&y,
			NULL);
		dest_x += (int)x;
		dest_y += (int)y;
	}

	XWarpPointer(XtDisplay(w), None, 
		XtWindowOfObject(fw), 0, 0, 0, 0,
		dest_x, dest_y);
	OlCallAcceptFocus(fw,CurrentTime);

	/*
	 * StructureNotifyMask must be selected for by 
	 * PopupWindowShell widget: so use Raw.
	 */
	XtRemoveRawEventHandler(w, StructureNotifyMask,
		False, warpPointer, client_data);
}

/******************************function*header****************************
 * PopupCB():  called when Notice is popped up.
 */

static void
PopupCB(Widget w, XtPointer closure, XtPointer call_data)
{
    NoticeShellWidget   nw = (NoticeShellWidget)w;
    NoticeShellPart *	nPart = &(nw->notice_shell);
    Widget		def;

    if (nPart->emanate == NULL)
	nPart->emanate = XtParent(nw);

    /* "busy" the appropriate things */
    BusyState(nw, True);

    PositionNotice(nw);

    /* Get (and Set) default control */
    def = GetDefaultControl((CompositeWidget)(nw->notice_shell.control));

    if (nPart->warp_pointer)
    {
	Window		junkWin;
	int		junkPos;
	unsigned int	junkMask;

	/*
	 * If the notice widget doesn't have any children
	 * in it's control area to warp the pointer
	 * to, warp the pointer to the control area.
	 */
	if (def == NULL)
		def = nw->notice_shell.control;

	/* save pointer position (to be restored later) */
	XQueryPointer(XtDisplay(nw), XtWindow(nw), &junkWin, &junkWin,
		      &(nPart->root_x), &(nPart->root_y),
		      &junkPos, &junkPos, &junkMask);

	/*
	 * StructureNotifyMask must be selected for by 
	 * PopupWindowSehll widget: so use Raw.
	 */
	XtAddRawEventHandler(w, StructureNotifyMask, 
		False, warpPointer, (XtPointer)def);

	nPart->do_unwarp = True;
    }

    /* Set windowGroup to group Notice with its "application" window.  Use
	SetValues on XtNwindowGroup since this is a resource of the shell.
	Rely on Shell to use proper protocol.
    */
    if (nPart->emanate != NULL &&
	XtWindowOfObject(nPart->emanate) !=
	RootWindowOfScreen(XtScreenOfObject(nPart->emanate)))
    {
	Arg wm_arg[1];
	XtSetArg(wm_arg[0], XtNwindowGroup,
		 XtWindow(_OlGetShellOfWidget(nPart->emanate)));
	XtSetValues((Widget)nw, wm_arg, XtNumber(wm_arg));
    }
}

/******************************function*header****************************
 * PostSelectCB():  called when button pressed in Notice
 */

static void
PostSelectCB(Widget nw, XtPointer closure, XtPointer call_data)
{
    XtPopdown((Widget)closure);
}
