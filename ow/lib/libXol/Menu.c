#pragma ident	"@(#)Menu.c	302.31	97/03/26 lib/libXol SMI"	/* menu:src/Menu.c 1.100	*/

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

/*
 *************************************************************************
 *
 * Description:
 *		Source code for the OPEN LOOK (Tm - AT&T) menu system.
 *	A couple private routines are not statically defined, so that
 *	the MenuButton and the AbbreviatedMenuButton can access them.
 *
 ******************************file*header********************************
 */


#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/ControlAre.h>
#include <Xol/Error.h>
#include <Xol/Form.h>
#include <Xol/MenuButton.h>
#include <Xol/MenuP.h>
#include <Xol/OlI18nP.h>
#include <Xol/OpenLookI.h>
#include <Xol/Pushpin.h>
#include <Xol/VendorI.h>
#include <Xol/ManagerP.h>
#include <Xol/SuperCaret.h>


typedef struct {
	OlMenuPositionProc	proc;
	Widget			emanate;
	Cardinal		emanate_index;
} PopupData;

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

static void    DismissMenu(register MenuShellWidget menu);		/* brings down a window that has
					 * a window background		*/
static void    GetBackdropGC(Widget widget);		/* Get GC for Backdrop contents	*/
static void    GetBackdropPixmaps(Widget widget);	/* Get Backdrop Pixmaps		*/
static char *	GetPartAddress (Widget, Boolean, _OlDynResourceList);
static void    GetShadowGC(Widget widget);		/* Get GC for Drop Shadow	*/
static void    LockMenu(MenuShellWidget menu);		/* restricts all events to one
					   menu				*/
static void    PostMenu (MenuShellWidget, PopupData *);
static void	QueryPointer (Widget, Position *, Position *);
static OlVirtualName _OlInputEventVirtualName (Widget, XEvent *);
static void	SetFocus (MenuShellWidget, Time);
static void    SetPressDragMode (MenuShellWidget,Position,Position,
					PopupData*);
static void    PopdownRivalCascade(register MenuShellWidget menu);	/* Removes sub-portion of a
					   menu cascade			*/
static void    PositionAppMenu (Widget, Widget, Cardinal, OlDefine,
			Position *, Position *, Position *, Position *);
static void    PinPutIn (Widget, XtPointer, XtPointer);
static void    RelocateMenu (MenuShellWidget, Position,
					Position, PopupData *);
static void    RemoveFormExpose(Widget w, XtPointer client_data, XtPointer call_data);	/* removes form's event handler	*/
static void    SetStayUpMode(MenuShellWidget menu);		/* initializes application for
					 * menu stay-up mode		*/
static Pixmap  ShadowStipple(Widget w, Boolean create);		/* Creates a Gray Pixmap	*/
static void    UpdateColors (MenuShellWidget, ArgList, Cardinal);
static void	UnpostMenu (MenuShellWidget, Boolean,
				    Boolean, Boolean);

					/* class procedures		*/

static Boolean ActivateWidget (Widget,OlVirtualName,XtPointer);
static void	WMMsgHandler (Widget, OlDefine, OlWMProtocolVerify *);
static void    ChangeManaged(Widget widget);		/* Management of children	*/
static void    ClassInitialize(void);	/* One shot class initialization*/
static void    Destroy(Widget widget);		/* Class destruction		*/
static XtGeometryResult
		GeometryManager(Widget child, XtWidgetGeometry *request, XtWidgetGeometry *reply);	/* Manage Geometry Chg. requests*/
static void    GetDefault (Widget,Widget);
static void    GetValuesHook(Widget w, ArgList args, Cardinal *num_args);		/* Get values hook		*/
static void    Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);		/* Class initialization		*/
static void    InitializeHook(Widget w, ArgList args, Cardinal *num_args);	/* Class initialization hook	*/
static void    Realize(Widget w, XtValueMask *value_mask, XSetWindowAttributes *attributes);		/* realizes the menu widget	*/
static void    Redisplay(Widget widget, register XEvent *xevent, register Region region);		/* Exposure handling		*/
static void    Resize(Widget widget);		/* Resizes the menu's child	*/
static Boolean SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);		/* Set Values 			*/
static void    SetDefault (Widget,Widget);
static Widget	TraversalHandler (Widget, Widget, OlDefine, Time);

					/* action procedures		*/

static void    ApplMenuPost(register Widget w, XtPointer data, XEvent *xevent, Boolean *cont_to_dispatch);		/* What on MENU Btn Down	*/
static void    EventHandler(register Widget widget, XtPointer data, XEvent *xevent, Boolean *cont_to_dispatch);		/* For SubstructureNotify Events*/
static void	HandleKeyPress (Widget, OlVirtualEvent);
static void    FormExpose(register Widget widget, XtPointer data, XEvent *xevent, Boolean *cont_to_dispatch);		/* for drawing line under the
					   menu's title			*/
static void    MenuButtonHandler(Widget w, OlVirtualEvent ve);	/* OlEventHandler procedure	*/
static void	PopdownTimerProc (XtPointer, XtIntervalId *);

static void	PreSelect (Widget,XtPointer,XtPointer);
static void	PostSelect (Widget,XtPointer,XtPointer);


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define FG_BIT		(1<<0)

#define BYTE_OFFSET	XtOffsetOf(OlVendorPartExtensionRec, dyn_flags)
static _OlDynResource dyn_res[] = {
	{ { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel), 0,
	    XtRString, XtDefaultForeground }, BYTE_OFFSET, FG_BIT,
	    GetPartAddress }
};

#define SET_ROOT(r)	RootMenu = (MenuShellWidget)r
static MenuShellWidget	RootMenu = (MenuShellWidget)NULL;

#define IN_MENU(w, x, y) \
	(x >= w->core.x && \
	 y >= w->core.y && \
	 x <= (Position)(w->core.x + w->core.width) && \
	 y <= (Position)(w->core.y + w->core.height))

static XtIntervalId	timer_id;
typedef enum {
	eNone,		/* no timers in the system		*/
	eDormant,	/* timer in, but pointer in wrong place	*/
	eActive		/* timer in and ready to detonate	*/
} TimerState;
static TimerState	timer_state = eNone;

#define SET_TIMER(m, state)	\
		if (timer_state != eNone) {\
			XtRemoveTimeOut(timer_id);\
		}\
		if (m->core.being_destroyed == False)\
		{\
			timer_id = XtAppAddTimeOut(\
				XtWidgetToApplicationContext((Widget)m),\
				(unsigned long)100, PopdownTimerProc,\
				(XtPointer)m->menu.root);\
			timer_state = state;\
		} else\
			timer_state = eNone

#define REMOVE_TIMER \
			if (timer_state != eNone) {\
				timer_state = eNone;\
				XtRemoveTimeOut(timer_id);\
			}

				/* Define Number of Points of Decoration
				 * set margin for 12point scale.	*/

#define DECOR_H_MARGIN		4

#define MENU_EVENT_MASK		ButtonReleaseMask|ButtonPressMask

#define WORKAROUND 1			/* do not remove this line	*/

#define SHADOW_THICKNESS	6			/* In points	*/
#define TITLE_PAD		8			/* In points	*/
#define VERTICAL_PADDING	1			/* In points	*/
#define HORIZ_PADDING		5			/* In points	*/

#define CNAME(w)	(XtClass((Widget)(w))->core_class.class_name)

static Region    nullRegion = (Region) NULL;
static Region    dest_region;
static Region    hold_region;
static Region    temp_region;

static Boolean	QueryDenied = True;
static Pixmap	def_bg_pixmap = None;

#define NULL_WIDGET	((Widget)NULL)
#define OFFSET(field)	XtOffsetOf(MenuShellRec, field)

static XtCallbackRec
PreSelectCB[] = {
	{ PreSelect,	NULL },
	{ NULL,		NULL }
};

static XtCallbackRec
PostSelectCB[] = {
	{ PostSelect,	NULL },
	{ NULL,		NULL }
};

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

/* There are no Translations or Action Tables for the menu widget */
static OlEventHandlerRec MenuEvents[] = {
	{ ButtonRelease,	MenuButtonHandler	},
	{ ButtonPress,		MenuButtonHandler	},
	{ KeyPress,		HandleKeyPress		}
};

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource
resources[] = {
						/* Core Resources	*/

    { XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
	OFFSET(core.background_pixel), XtRString,
	(XtPointer) XtDefaultBackground },

    { XtNbackgroundPixmap, XtCPixmap, XtRPixmap, sizeof(Pixmap),
	OFFSET(core.background_pixmap), XtRPixmap, (XtPointer) &def_bg_pixmap },

    { XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
	OFFSET(core.border_width), XtRImmediate, (XtPointer) 0 },

						/* Shell Resources	*/

    { XtNallowShellResize, XtCAllowShellResize, XtRBoolean, sizeof(Boolean),
	OFFSET(shell.allow_shell_resize), XtRImmediate, (XtPointer) True },

    { XtNsaveUnder, XtCSaveUnder, XtRBoolean, sizeof(Boolean),
	OFFSET(shell.save_under), XtRImmediate, (XtPointer) True },

    { XtNoverrideRedirect, XtCOverrideRedirect, XtRBoolean, sizeof(Boolean),
	OFFSET(shell.override_redirect), XtRImmediate, (XtPointer) True },

						/* WMShell Resources	*/
    { XtNtransient, XtCTransient, XtRBoolean, sizeof(Boolean),
	OFFSET(wm.transient), XtRImmediate, (XtPointer)False},

						/* Menu Resources	*/
	/* XtNtextFormat should always be the first Menu resource */
    {XtNtextFormat, XtCTextFormat, XtROlStrRep,sizeof(OlStrRep),
	OFFSET(menu.text_format),XtRCallProc,(XtPointer)_OlGetDefaultTextFormat},

    { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	OFFSET(menu.foreground), XtRString, (XtPointer) XtDefaultForeground },

    { XtNmenuAttachQuery, XtCMenuAttachQuery, XtRBoolean, sizeof(Boolean),
	OFFSET(menu.attach_query), XtRBoolean, (XtPointer) &QueryDenied }, 

    { XtNmenuAugment, XtCMenuAugment, XtRBoolean, sizeof(Boolean),
	OFFSET(menu.augment_parent), XtRImmediate, (XtPointer) True },

    { XtNpushpinWidget, XtCPushpinWidget, XtRWidget, sizeof(Widget),
	OFFSET(menu.pushpin), XtRWidget, (XtPointer) NULL },

    { XtNpushpinDefault, XtCPushpinDefault, XtRBoolean, sizeof(Boolean),
	OFFSET(menu.pushpin_default), XtRImmediate, (XtPointer) False },

    { XtNrevertButton, XtCRevertButton, XtRFunction, sizeof(OlRevertButtonProc),
	OFFSET(menu.revert_button), XtRFunction, (XtPointer) NULL },

    { XtNshellBehavior, XtCShellBehavior, XtRInt, sizeof(ShellBehavior),
	OFFSET(menu.shell_behavior),
	XtRImmediate, (XtPointer) PressDragReleaseMenu }

};

#undef OFFSET
/*
 *************************************************************************
 *
 * Define Class Extension Resource List
 *
 *************************class*extension*records*************************
 */

#define OFFSET(field)	XtOffsetOf(OlVendorPartExtensionRec, field)

static XtResource
ext_resources[] = {
	{ XtNmenuButton, XtCMenuButton, XtRBoolean, sizeof(Boolean),
	  OFFSET(menu_button), XtRImmediate, (XtPointer)False },

	{ XtNpushpin, XtCPushpin, XtROlDefine, sizeof(OlDefine),
	  OFFSET(pushpin), XtRImmediate, (XtPointer)OL_NONE },

	{ XtNresizeCorners, XtCResizeCorners, XtRBoolean, sizeof(Boolean),
	  OFFSET(resize_corners), XtRImmediate, (XtPointer)False },

	{ XtNmenuType, XtCMenuType, XtROlDefine, sizeof(OlDefine),
	  OFFSET(menu_type), XtRImmediate, (XtPointer)OL_MENU_LIMITED },

	{ XtNfooterPresent, XtCFooterPresent, XtRBoolean, sizeof(Boolean),
	OFFSET(footer_present), XtRImmediate, (XtPointer)False },

	{ XtNimFooterPresent, XtCImFooterPresent, XtRBoolean, sizeof(Boolean),
	OFFSET(im_footer_present), XtRImmediate, (XtPointer)False },

	{ XtNwinType, XtCWinType, XtROlDefine, sizeof(OlDefine),
	  OFFSET(win_type), XtRImmediate, (XtPointer)OL_WT_CMD },
};

#undef OFFSET
/*
 *************************************************************************
 *
 * Define Class Extension Records
 *
 *************************class*extension*records*************************
 */

static OlVendorClassExtensionRec
vendor_extension_rec = {
    {
	NULL,					/* next_extension	*/
	NULLQUARK,				/* record_type		*/
	OlVendorClassExtensionVersion,		/* version		*/
	sizeof(OlVendorClassExtensionRec)	/* record_size		*/
    },	/* End of OlClassExtension header	*/
	ext_resources,				/* resources		*/
	XtNumber(ext_resources),		/* num_resources	*/
	NULL,					/* private		*/
	SetDefault,				/* set_default		*/
	GetDefault,				/* get_default 		*/
	NULL,					/* destroy		*/
	NULL,					/* initialize		*/
	NULL,					/* set_values		*/
	NULL,					/* get_values		*/
	TraversalHandler,			/* traversal_handler	*/
	XtInheritHighlightHandler,		/* highlight_handler	*/
	ActivateWidget,				/* activate		*/
	MenuEvents,				/* event_procs		*/
	XtNumber(MenuEvents),			/* num_event_procs	*/
	NULL,					/* part_list (private)	*/
	{ dyn_res, XtNumber(dyn_res) },		/* dyn_data		*/
	NULL,					/* transparent_proc	*/
	WMMsgHandler,				/* wm_proc		*/
	TRUE,					/* override_callback	*/
}, *vendor_extension = &vendor_extension_rec;

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

externaldef(menushellclassrec) MenuShellClassRec
menuShellClassRec = {
  {
	(WidgetClass) &transientShellClassRec,	/* superclass		*/
	"MenuShell",				/* class_name		*/
	sizeof(MenuShellRec),			/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	Initialize,				/* initialize		*/
	InitializeHook,				/* initialize_hook	*/
	Realize,				/* realize		*/
	NULL,					/* actions		*/
	NULL,					/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	TRUE,					/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	Destroy,				/* destroy		*/
	Resize,					/* resize		*/
	Redisplay,				/* expose		*/
	SetValues,				/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	GetValuesHook,				/* get_values_hook	*/
	XtInheritAcceptFocus,			/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_private	*/
	XtInheritTranslations,			/* tm_table		*/
	NULL,					/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
  },	/* End of CoreClassPart field initializations */
  {
	GeometryManager,			/* geometry_manager	*/
	ChangeManaged,				/* change_managed	*/
	XtInheritInsertChild,			/* insert_child		*/
	XtInheritDeleteChild,			/* delete_child		*/
	NULL					/* extension		*/
  },	/* End of CompositeClassPart fields	*/
  {
	NULL					/* empty field		*/
  },	/* End of ShellClassPart field */
  {
	NULL					/* empty field		*/
  },	/* End of WMShellClassPart field */
  {
	(XtPointer)&vendor_extension_rec	/* empty field		*/
  },	/* End of VendorShellClassPart field */
  {
	NULL					/* empty field		*/
  },	/* End of TranisentShellClassPart field */
  {
	NULL					/* empty field		*/
  }	/* End of MenuShellClassPart field */
}; 

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

externaldef(menushellwidgetclass) WidgetClass
	menuShellWidgetClass = (WidgetClass) &menuShellClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * DismissMenu - whenever the pushpin is pulled out of its hole,
 * this routine is called.
 ****************************procedure*header*****************************
 */
static void
DismissMenu(register MenuShellWidget menu)
{
	Widget 		widget_list[2];
	Cardinal	num;
	static Arg	notify[] = {
		{ XtNoverrideRedirect,	(XtArgVal) True },
		{ XtNtransient, (XtArgVal) False }
	};
	register MenuShellWidget	self;

				/* If the pushpin is in, pull it out	*/

	if (menu->menu.shell_behavior == PinnedMenu)
	{
		Arg arg[1];

		XtSetArg(arg[0], XtNpushpin, OL_OUT);
		XtSetValues(menu->menu.pushpin, arg, 1);
	}

			/* Re-establish the menu's core parent		*/

	menu->core.parent = menu->menu.parent_cache;

	_OlPropagateMenuState(menu, StayUpMenu);

	for(self = menu->menu.next; self; self = self->menu.next)
		self->menu.root = menu;

				/* Set This Menu as the root of the
				 * cascade				*/

	SET_ROOT(menu);
	menu->menu.root = menu;

				/* Make the menu and override redirect
				 * window again.			*/
	XtSetValues((Widget) menu, notify, XtNumber(notify));

				/* Unpost this menu, and
				 * reset the focus */
	UnpostMenu(menu, True, True, True);

			/* First manage the title and the pushpin
			 * if they exist.  This will cause the form
			 * to relayout so all the children of the
			 * form show.  When the form does a relayout,
			 * it causes a geometry request to the menu
			 * and hence the menu will resize.  When the
			 * menu resizes, the Resize procedure will add
			 * the menu's drop shadows.			*/

	num		= (Cardinal)0;
	if (menu->menu.title_widget != (Widget) NULL)  {
		widget_list[num] = menu->menu.title_widget;
		++num;
	}
	if (menu->menu.pushpin != (Widget) NULL) {
		widget_list[num] = menu->menu.pushpin;
		++num;
	}
	if (num != (Cardinal) 0)  {
		XtManageChildren(widget_list, num);
	}

				/* reset the form's border width	*/

	XtResizeWidget(menu->composite.children[0],
			menu->composite.children[0]->core.width,
			menu->composite.children[0]->core.height,
			(Dimension) 1);
} /* END OF DismissMenu() */

/*
 *************************************************************************
 * GetBackdropPixmaps - this routine handles the acquisition of the
 * pixmaps used to cache the menu's backdrop area.  If either of the
 * pixmaps exist, they are freed because this procedure is called when
 * the menu changes size).
 ****************************procedure*header*****************************
 */
static void
GetBackdropPixmaps(Widget widget)
{
	MenuShellWidget menu = (MenuShellWidget) widget;
	Dimension	dimension;

	if (menu->menu.backdrop_right != (Pixmap) NULL)
		XFreePixmap(XtDisplay(widget), menu->menu.backdrop_right);

	if (menu->menu.backdrop_bottom != (Pixmap) NULL)
		XFreePixmap(XtDisplay(widget), menu->menu.backdrop_bottom);

					/* Do the right drop shadow	*/

	if (menu->menu.shell_behavior == PinnedMenu)
	{

				/* Must Add the 2 border widths since
				 * this is what height this pixmap will
				 * be when the menu is transitory	*/

		dimension = menu->core.height + (Dimension)2;
	}
	else {
		dimension = menu->core.height - menu->menu.shadow_bottom;
	}

	menu->menu.backdrop_right =
		XCreatePixmap(XtDisplay(widget),
			RootWindowOfScreen(XtScreen(widget)),
			(unsigned int)menu->menu.shadow_right,
			(unsigned int)dimension,
			(unsigned int)widget->core.depth);

					/* Do the bottom drop shadow	*/

	if (menu->menu.shell_behavior == PinnedMenu)
	{
				/* Must add the 2 border widths and
				 * the width of the right shadow since
				 * this is what height this pixmap will
				 * be when the menu is transitory	*/

		dimension = menu->core.height + menu->menu.shadow_right + 2;
	}
	else {
		dimension = menu->core.width;
	}

	menu->menu.backdrop_bottom =
		XCreatePixmap(XtDisplay(widget),
			(unsigned int)RootWindowOfScreen(XtScreen(widget)),
			(unsigned int)dimension,
			(unsigned int)menu->menu.shadow_bottom,
			(unsigned int)widget->core.depth);

} /* END OF GetBackdropPixmaps() */

/*
 *************************************************************************
 * GetPartAddress - Called by the dynamic resource routines.  This routine
 * returns the base address of where the dynamic resource flags live.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static char *
GetPartAddress (Widget w, Boolean init, _OlDynResourceList res)
{
	/*
	 * Normally, you would need to check the init flag and may
	 * need to allocate the extension since this procedure is called
	 * before the subclass's Initialize routine.  But in our case,
	 * the vendor superclass allocates the extension before
	 * handling the dynamic resources.  Therefore, we're guaranteed that
	 * our extension is already allocated.
	 */

	return((char *) _OlGetVendorPartExtension(w));

} /* END OF GetPartAddress() */

/*
 *************************************************************************
 * GetShadowGC - Initializes fields that need to be initialized
 * for the Menu at run-time.
 ****************************procedure*header*****************************
 */
static void
GetShadowGC(Widget widget)
{
	XGCValues	values;
	XGCValues	values_return;
	Pixmap		pixmap;
	MenuShellWidget	menu = (MenuShellWidget) widget;

			/* If and old GC exists, save the pixmap but
			 * destroy the GC				*/

	if (menu->menu.shadow_GC != (GC)NULL &&
	    XGetGCValues(XtDisplay(widget), menu->menu.shadow_GC,
			 GCStipple, &values_return)) {
		pixmap = values_return.stipple;
		XtDestroyGC(menu->menu.shadow_GC);
	}
	else {
		pixmap = ShadowStipple(widget, True);
	}

				/* Get the GC to be used for
				 * the "drop-shadow"			*/

	values.stipple			= pixmap;
	values.fill_style		= FillStippled;
	values.foreground		= menu->menu.foreground;
	values.graphics_exposures	= False;

	menu->menu.shadow_GC = XtGetGC(widget, (XtGCMask) (GCFillStyle |
				GCForeground | GCStipple |
				GCGraphicsExposures), &values);
} /* END OF GetShadowGC() */

/*
 *************************************************************************
 * GetBackdropGC - Initializes fields that need to be initialized
 * for the Menu at run-time.
 ****************************procedure*header*****************************
 */
static void
GetBackdropGC(Widget widget)
{
	XGCValues	values;
	MenuShellWidget	menu = (MenuShellWidget) widget;

				/* If an old GC exists, destroy it	*/

	if (menu->menu.backdrop_GC != (GC) NULL)
		XtDestroyGC(menu->menu.backdrop_GC);

				/* Get the GC to be used for
				 * the "drop-shadow"			*/

	values.subwindow_mode	  = IncludeInferiors;
	values.graphics_exposures = False;
	values.foreground	  = menu->menu.foreground;
	values.background	  = menu->core.background_pixel;
	values.line_width	  = OlScreenPointToPixel(OL_VERTICAL, 1,
					XtScreen(widget));

					/* Optimize the line drawing	*/

	if (values.line_width == 1)
		values.line_width = 0;

	menu->menu.backdrop_GC = XtGetGC(widget, (XtGCMask) (GCLineWidth |
					GCForeground | GCBackground |
					GCSubwindowMode | GCGraphicsExposures),
					&values);
} /* END OF GetBackdropGC() */

/*
 *************************************************************************
 * LockMenu - "locks" menu so that all events are delivered to this menu.
 * To prevent pinned menus which may be low in the cascade from receiving
 * XEvents (because an ancestor of theirs is on the Xt GrabList), a
 * "blocking" widget is put on the grablist before the new cascade. 
 ****************************procedure*header*****************************
 */
static void
LockMenu(MenuShellWidget menu)
{
	register MenuShellWidget self;

			/* Put this menu on the Grablist next		*/

	menu->shell.grab_kind     = XtGrabExclusive;
	menu->shell.spring_loaded = True;

	_OlAddGrab((Widget) menu, True, True);

				/* Now, make every menu in the cascade
				 * prior to this one have a non-exclusive
				 * grab so that it can receive events.	*/

	for (self = menu->menu.root;
	     self && self != menu;
	     self = self->menu.next) {

		self->shell.grab_kind     = XtGrabNonexclusive;
		self->shell.spring_loaded = False;

		_OlAddGrab((Widget) self, False, False);
	}

			/* If this is the root menu, request a pointer
			 * grab to catch events off the cascade.  Since
			 * the cascade is a 'spring-loaded' cascade,
			 * the last menu in the cascade always receives
			 * any event sent to any member of the cascade.
			 * Therefore, even though the grab is on the root
			 * of the cascade, the last menu will receive
			 * the grabbed events. It is the last menu that
			 * processes the events.			*/

	if (menu == menu->menu.root)
	{
		(void)_OlGrabPointer((Widget) menu,
			True,		/* allow events to others	*/
			MENU_EVENT_MASK,
			GrabModeAsync,
			GrabModeAsync,
			None,
			None,
			CurrentTime);
	}

} /* END OF LockMenu() */

/*
 *************************************************************************
 * QueryPointer - queries the mouse location and returns the x and y
 * coordinates as Position variables.
 ****************************procedure*header*****************************
 */
static void
QueryPointer (Widget w, Position *x_ret, Position *y_ret)
{
	Window	junk;
	int	ijunk;
	int	x;
	int	y;

	(void) XQueryPointer(XtDisplay(w), RootWindowOfScreen(XtScreen(w)),
		&junk, &junk, &x, &y, &ijunk, &ijunk, (unsigned int *)&ijunk);
	*x_ret = (Position)x;
	*y_ret = (Position)y;
} /* END OF QueryPointer() */

/*
 *************************************************************************
 * PostMenu - this routine handles the posting of the menu.  It
 * sets up the necessary hooks to support the right "look and feel."
 * The does some extra processing that is too important to put into
 * Callbacks.  For instance, it makes sure that there are no rival Cascades.
 ****************************procedure*header*****************************
 */
static void
PostMenu (register MenuShellWidget menu, PopupData *popup_data)
{
    Boolean			is_descendent = False;
    register MenuShellWidget	shell;
    register Display *		dpy = XtDisplay((Widget) menu);

    if (menu->shell.popped_up == False) {

				/* Do Callbacks before anything		*/

	XtCallCallbacks((Widget) menu, XtNpopupCallback, (XtPointer) menu);


			/* Set the root of the menu.  Only do this for a
			 * Non-pinned menu				*/

	shell = (MenuShellWidget) _OlGetShellOfWidget(menu->core.parent);

	is_descendent = (shell &&
			 XtIsSubclass((Widget)shell, menuShellWidgetClass));

	if (menu->menu.shell_behavior != PinnedMenu) {
		if (is_descendent) {
			if (shell->menu.shell_behavior == PinnedMenu)
			{
				/* Since are parent is pinned and since
				 * the current toolkit does not 'clone'
				 * pinned menus, we must check to see if
				 * an ancestor of our pinned menu is in
				 * transitory mode.   This check is done
				 * by seeing if the RootMenu is non-NULL.
				 * If it is, attach this menu to its
				 * cascade.				*/

				if (RootMenu == NULL)
				{
					shell->menu.next = menu;
					menu->menu.root  = menu;
				}
				else
				{
					MenuShellWidget self = RootMenu;
					while(self->menu.next) {
						self = self->menu.next;
					}
					self->menu.next = menu;
					menu->menu.root = RootMenu;
				}
			}
			else {
				menu->menu.root = shell->menu.root;

				if (shell->menu.next)
					PopdownRivalCascade(shell);

			/* Insert this menu in the parent's hanging list */

				menu->menu.next = shell->menu.next;
				shell->menu.next = menu;
			}
		}
		else {
			menu->menu.root = menu;
		}

						/* Set a dormant timer.	*/

		SET_TIMER(menu, eDormant);
	}
	else {
		menu->menu.root = (MenuShellWidget) NULL;
	}

				/* Cache the root id for the PreSelect
				 * callback				*/

	if (menu->menu.root != (MenuShellWidget)NULL)
	{
		SET_ROOT(menu->menu.root);
	}

				/* Register that the menu cascade wants
				 * Pointer Grabs			*/

	if (menu->menu.root == menu)
	{
		(void) _OlGrabServer((Widget)menu);
	}

				/* Reset the menu's shell fields	*/

	menu->shell.popped_up     = TRUE;
	menu->shell.grab_kind     = XtGrabNone;
	menu->shell.spring_loaded = False;

	if (menu->shell.create_popup_child_proc != NULL) {
	    (*(menu->shell.create_popup_child_proc))((Widget) menu);
	}

	if (XtIsRealized((Widget) menu) == False) {
		XtRealizeWidget((Widget) menu);
		XSync(dpy, 0);
	}

			/* Position the menu after realization to insure
			 * it is placed correctly.  In here we check
			 * to see if the MENU button is already
			 * released.  If so, force stay up mode		*/

	if (popup_data->proc != (OlMenuPositionProc)NULL) {
		Position post_x = menu->menu.post_x;
		Position post_y = menu->menu.post_y;

		RelocateMenu(menu, post_x, post_y, popup_data);

		if ((post_x != menu->menu.post_x ||
		    post_y != menu->menu.post_y) &&
		    menu->core.parent != (Widget)NULL)
		{
			XEvent trash_event;

			XSync(dpy, 0);

			if (XCheckWindowEvent(dpy,
				XtWindowOfObject(XtParent(menu)),
				ButtonReleaseMask, &trash_event)) {

				SetStayUpMode(menu);
			}
		}
	}
				/* Map the menu and put it on top	*/

	XMapRaised(dpy, XtWindow((Widget) menu));
    }
    else if (menu->menu.shell_behavior == PinnedMenu)
    {
					/* Raise the window to the top	*/

	XRaiseWindow(dpy, XtWindow((Widget) menu));
    }
} /* END OF PostMenu() */

/*
 *************************************************************************
 * _OlIsMenuMouseClick - determines if an x and y position relative to the
 * menu's posting position should be interpreted as a mouse click.
 ****************************procedure*header*****************************
 */
Boolean
_OlIsMenuMouseClick(MenuShellWidget menu, int x, int y)
{
	int click_threshold;
	int dx;
	int dy;

	dx = (int)menu->menu.press_x - x;
	dy = (int)menu->menu.press_y - y;

	click_threshold = (int)_OlGetMouseDampingFactor((Widget)menu);

	return((Boolean)
		((click_threshold * click_threshold) >= ((dx * dx) + (dy * dy))
			? True : False));
} /* END OF _OlIsMenuMouseClick() */

/*
 *************************************************************************
 * _OlInputEventVirtualName - returns the virtual_name of an XEvent.
 ****************************procedure*header*****************************
 */
static OlVirtualName
_OlInputEventVirtualName (Widget w, XEvent *xevent)
{
	OlVirtualEventRec	ve;

	OlLookupInputEvent(w, xevent, &ve, OL_DEFAULT_IE);

	return (ve.virtual_name);
} /* END OF _OlInputEventVirtualName() */

/*
 *************************************************************************
 * _OlPropagateMenuState - this routine propagates the menu's state to the
 * children of the menu pane, so that they can modify their behavior or
 * appearance as necessary.
 ****************************procedure*header*****************************
 */
void
_OlPropagateMenuState(MenuShellWidget menu, ShellBehavior shell_behavior)
	                     
	             	               		/* New shell behavior	*/
{
	CompositeWidget pane = menu->menu.pane;
	int		i;
	int		ac = 0;		/* args count			*/
	Arg		args[2];	/* maximum number of args	*/


				/* Set the first arg to be the
				 * shell behavior			*/

	XtSetArg(args[ac], XtNshellBehavior, shell_behavior);
	++ac;

				/* Update the menu's shell behavior	*/
	menu->menu.shell_behavior = shell_behavior;

			/* Inform the menu's pushpin if it has one	*/

	if (menu->menu.pushpin != (Widget) NULL)
		XtSetValues(menu->menu.pushpin, args, ac);

			/* Inform pane children of new menu state	*/

	for(i = 0; i < pane->composite.num_children; ++i)
		XtSetValues(pane->composite.children[i], args, 1);

} /* END OF _OlPropagateMenuState() */

/*
 *************************************************************************
 * SetFocus - This function sets focus to the menu when it maps
 ****************************procedure*header*****************************
 */
static void
SetFocus (MenuShellWidget menu, Time time)
{
	Widget			w = (Widget)menu;
	OlVendorPartExtension	part;
	OlFocusData *		fd = _OlGetFocusData(w, &part);
	_OlArray		list = &(fd->traversal_list);
	Widget			the_default;
	OlTraversalFunc		func;
	Widget			parent;

	/* Get the current focus widget from the parent shell */
        parent = (Widget)menu;
        while( ((parent = XtParent((Widget)parent)) != (Widget)NULL) &&
               ((menu->menu.revert_focus = OlGetCurrentFocusWidget(parent))
                                        == (Widget)NULL) );

	if ((the_default = _OlGetDefault(w)) != NULL_WIDGET &&
	    OlCallAcceptFocus(the_default, time) == TRUE)
	{
		return;
	}

	if (list->num_elements > (Cardinal)0 &&
	    (func = part->class_extension->traversal_handler) &&
	    (*func)(w,(Widget)list->array[0],OL_IMMEDIATE,time) != NULL_WIDGET)
	{
		return;
	}

		/* No default widget and no descendent wants
		 * focus, so set it to the menu.		*/

	(void)OlCallAcceptFocus(w, time);
} /* END OF SetFocus() */

/*
 *************************************************************************
 * SetPressDragMode - initializes the application for a
 * "press-drag-release" menu.
 ****************************procedure*header*****************************
 */
static void
SetPressDragMode (MenuShellWidget menu,
		  Position x, Position y, PopupData *popup_data)
{
				/* Set the state of the Menu		*/

	menu->menu.post_x         = (Position)x;
	menu->menu.post_y         = (Position)y;
	menu->menu.prevent_stayup = False;

				/* Inform menu items of the new state	*/

	_OlPropagateMenuState(menu, PressDragReleaseMenu);

				/* Popup the Menu			*/

	PostMenu(menu, popup_data);

				/* "lock" the menu to
				 * direct all XEvents to this menu.	*/

	LockMenu(menu);

} /* END OF SetPressDragMode() */

/*
 *************************************************************************
 * PopdownRivalCascade - this routine is called when only a portion of
 * an existing menu cascade must be removed to prepare for a different
 * cascade branch.  The argument "menu" is the point at which every
 * menu below it must be removed.
 ****************************procedure*header*****************************
 */
static void
PopdownRivalCascade(register MenuShellWidget menu)
{
	XEvent            xevent;
	register Display *dpy;
	register Window   win;
	extern Region     nullRegion;
	extern Region     temp_region;
	extern Region     hold_region;

				/* Remove the submenus, but do not
				 * attempt to reset the input focus	*/

	UnpostMenu(menu->menu.next, False,True, True);

				/* Refresh this menu before doing
				 * anything else			*/

	dpy = XtDisplay((Widget) menu);
	win = XtWindow((Widget) menu);

				/* Insure exposures caused by bringing
				 * down the cascade get on queue	*/

	XSync(dpy, FALSE);

				/* Peel Off Exposures			*/

	if (XCheckWindowEvent(dpy, win, ExposureMask, &xevent)) {

					/* initialize the region	*/

		XIntersectRegion(nullRegion, temp_region, hold_region);

					/* Accummulate exposures	*/

		do {
			XtAddExposureToRegion(&xevent, hold_region);

		} while (xevent.xexpose.count > 0 &&
			 XCheckWindowEvent(dpy, win, ExposureMask, &xevent));

					/* Do Redisplay Now		*/

		if (XtClass((Widget) menu)->core_class.expose)
			(*(XtClass((Widget) menu)->core_class.expose))
				((Widget) menu, &xevent, hold_region);
	}

				/* Set the "next" field to NULL		*/

	menu->menu.next = (MenuShellWidget) NULL;

} /* END OF PopdownRivalCascade() */

/*
 *************************************************************************
 * PositionAppMenu - This routine has the task of positioning the
 * Menu.  This routine positions the menu so that the pointer is a few
 * points to the left of the default menu item.  If there's no default,
 * position the menu so that the pointer is to the left of the first
 * menu item.  If there are no menu items, put the pointer in the upper
 * left-hand corner.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
PositionAppMenu (Widget w, Widget emanate, Cardinal emanate_index,
		 OlDefine state, Position *mx, Position *my,
		 Position *px, Position *py)
{
	register MenuShellWidget menu = (MenuShellWidget) w;
	register Widget	self;
	Widget		default_widget = (Widget) NULL;
	Position	horiz_buf;	/* distance from right edge of
					 * menu to right screen	edge	*/
	Position	vert_buf;	/* distance from bottom edge of
					 * menu to bottom screen edge	*/
	Position	rx;	/* menu position relative to RootWindow	*/
	Position	ry;	/* menu position relative to RootWindow	*/
	Position	x;	/* position within menu to put pointer	*/
	Position	y;	/* position within menu to put pointer	*/

	if (state == OL_PINNED_MENU) {
		return;
	}

				/* Determine pointer position within the
				 * menu.				*/

	default_widget = _OlGetDefault((Widget)menu);

				/* Locate the upper left hand corner
				 * of the default widget relative to 
				 * the menu widget.  Start the x location
				 * at four points to the left of
				 * the default item.			*/

	x = (Position) -OlScreenPointToPixel(OL_HORIZONTAL, 4, XtScreen(w));
	for (self = default_widget, y=0;
	     self != (Widget) NULL && self != (Widget) menu;
	     self = self->core.parent)
	{
		x += self->core.x + (Position)self->core.border_width;
		y += self->core.y + (Position)self->core.border_width;
	}

				/* If "the_default" widget does not
				 * equal the menu shell's default, then
				 * the pushpin was the true default.
				 * So, put the pointer at the top of the
				 * menu and 4 points to the left	*/

	if (default_widget != (Widget) NULL)
	{
		y += (Position) ((default_widget->core.height /(Dimension)2) +
				default_widget->core.border_width);
	}

				/* Calculate the menu's position relative
				 * to the pointer.  The new position will
				 * be relative to the RootWindow.	*/

	rx = *px - x;
	ry = *py - y;

				/* Calculate the buffer between the new
				 * menu's position and the right and
				 * bottom edge of the screen.		*/

	horiz_buf = (Position)WidthOfScreen(XtScreen(w)) - (rx + 
					(Position)_OlWidgetWidth(w));
	vert_buf = (Position)HeightOfScreen(XtScreen(w)) - (ry +
					(Position)_OlWidgetHeight(w));

			/* If the horizontal buffer is less than zero,
			 * we've gone off the right edge; so, move the
			 * root position to the left and move the pointer
			 * to the left by necessary amount.		*/

	if (horiz_buf < (Position)0)
	{
		rx += horiz_buf;
		*px += horiz_buf;
	}

			/* If the vertical buffer is less than zero,
			 * we've gone off the bottom edge; so, move the
			 * root position up and the pointer up by the
			 * necessary amount.				*/

	if (vert_buf < (Position)0)
	{
		ry += vert_buf;
		*py += vert_buf;
	}

			/* Check to see if the menu extends off the top
			 * or left screen edge.  Take appropriate action
			 * if so.					*/

	if (rx < (Position)0)
	{
		*px -= rx;
		rx -= rx;
	}

	if (ry < (Position)0)
	{
		*py -= ry;
		ry -= ry;
	}

				/* Set the new desired menu location	*/
	*mx = rx;
	*my = ry;

} /* END OF PositionAppMenu() */

/*
 *************************************************************************
 * PinPutIn - whenever the pushpin is put into its hole, this routine
 * is called.
 *	The pushpin we are talking about is the pushpin that the menu
 * owns, not the pushpin that the window manager owns.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
PinPutIn(Widget w, XtPointer client_data, XtPointer call_data)
	      		  		/* Pushpin widget id		*/
	         	            	/* The PopupData		*/
	         	          	/* pushpin's call data		*/
{
	static Arg notify[] = {
		{ XtNoverrideRedirect,	(XtArgVal) False }
	};
	Display	*		dpy;
	Window			win;
	Cardinal		num;
	MenuShellWidget		menu = (MenuShellWidget) _OlGetShellOfWidget(w);
	Widget			base_shell = XtParent((Widget)menu);
	Widget 			widget_list[2];
	XSizeHints 		sh;
	XWMHints *		wmh;
	Boolean			move_window = False;

	if (menu == (MenuShellWidget) NULL)
		return;

			/* Realize the menu now since we know it will
			 * be done down below anyway -- so we can
			 * code assuming we have a window.		*/

	if (XtIsRealized((Widget)menu) == False)
	{
		XtRealizeWidget((Widget)menu);
	}

	dpy		= XtDisplay((Widget) menu);
	win		= XtWindow((Widget) menu);

	if (menu->shell.popped_up == True)
	{
		if (menu->menu.shell_behavior == PinnedMenu)
		{
			SetWMPushpinState(dpy, win, WMPushpinIsIn);
			return;
		}
		else
		{
				/* If the menu is popped up, unmap it
				 * so we can reset some of the window
				 * hints (i.e., the window manager can
				 * only see the new hints when the
				 * window maps).			*/

			XUnmapWindow(dpy, win);
			XSync(dpy, (Bool)0);
			move_window = True;
		}
	}

	wmh	= &(menu->wm.wm_hints);

				/* Set the Window Group of the menu
				 * if it's not already set		*/

	if (base_shell != (Widget)NULL &&
	     XtWindow(base_shell) != RootWindowOfScreen(XtScreen(base_shell)))
	{
				/* Find the base window.  It can't be
				 * a notice, popup or another menu.	*/

		base_shell = _OlGetShellOfWidget(base_shell);

		while(base_shell != (Widget)NULL &&
		    XtIsSubclass(base_shell, menuShellWidgetClass) == True)
		{
			base_shell = XtParent(base_shell);

			if (base_shell != (Widget)NULL)
			{
				base_shell = _OlGetShellOfWidget(base_shell);
			}
		}

		if (base_shell != (Widget)NULL)
		{
		    Arg args[2];
		    Cardinal ac;
			ac = 0;
			XtSetArg(args[ac], XtNtransient, (XtArgVal)True); ++ac;
			XtSetArg(args[ac], XtNtransientFor,
					(XtArgVal) base_shell); 	  ++ac;
			XtSetValues((Widget)menu, args, ac);

			wmh->window_group	= (XID) XtWindow(base_shell);
			wmh->flags		|= WindowGroupHint;
		}
	} /* end of setting window group */

				/* Always set the input flag to false
				 * since we somehow keep dropping this	*/

	wmh->flags	|= InputHint;
	wmh->input	= (Bool)False;

						/* Apply the hints	*/
	XSetWMHints(dpy, win, wmh);

				/* Set the menu to be pinned		*/
 
	_OlPropagateMenuState(menu, PinnedMenu);

	if (RootMenu != (MenuShellWidget)NULL)
	{
		/* Remove all menus in the cascade except for this
		 * one.  Also, don't attempt to reset the input focus	*/

		UnpostMenu(RootMenu, False, True, True);
	}

			/* Change the Override Redirect flag before
			 * doing before unmanaging the form's children
			 * since in R3, the shell's core geometry fields
			 * are not updated until the WindowManager gives
			 * us a ConfigureNotify Event.  We can't wait
			 * that long, so turn off the override.		*/

	XtSetValues((Widget) menu, notify, XtNumber(notify));

			/* put pin in */
	_OlSetPinState((Widget)menu, OL_IN);

			/* Unmanage the title and the pushpin
			 * if they exist.  This will cause the form
			 * to relayout so the pane only shows.  When
			 * the form does a relayout, it will make
			 * geometry request to the menu and hence the
			 * menu will resize.  When the menu resizes,
			 * the menu's Resize procedure will remove
			 * the menu's drop shadows.			*/

	num = (Cardinal)0;
	if (menu->menu.title_widget != (Widget) NULL)  {
		widget_list[num] = menu->menu.title_widget;
		++num;
	}
	if (menu->menu.pushpin != (Widget) NULL) {
		widget_list[num] = menu->menu.pushpin;
		++num;
	}
	if (num != (Cardinal)0)  {
		XtUnmanageChildren(widget_list, num);
	}

					/* Remove the form's border	*/

	XtResizeWidget(menu->composite.children[0],
			menu->composite.children[0]->core.width,
			menu->composite.children[0]->core.height,
			(Dimension)0);

				/* Move the menu to the left to
				 * compensate for the decoration set.
				 * Don't do anything to the height
				 * since the above resize request
				 * already handled this.		*/

	if (move_window == (Boolean)True) {
		Position margin = OlScreenPointToPixel(OL_HORIZONTAL,
				DECOR_H_MARGIN, XtScreen((Widget)menu));
		XtMoveWidget((Widget)menu, (menu->core.x - margin),
				menu->core.y);
	}

				/* Set the size hints for the window
				 * manager.				*/

	*((struct _OldXSizeHints *)&sh) = menu->wm.size_hints;
	sh.base_width = menu->wm.base_width;
	sh.base_height = menu->wm.base_height;
	sh.win_gravity = menu->wm.win_gravity;
	sh.flags	&= ~PPosition;			/* Turn this off*/
	sh.flags	|= USPosition;			/* Turn this on	*/
	sh.x		= (int)menu->core.x;
	sh.y		= (int)menu->core.y;

	XSetNormalHints(dpy, win, &sh);

				/* Now determine what to do with the
				 * menu on screen			*/

	if (menu->shell.popped_up == False) {
		if (client_data == NULL)
		{
			PopupData	popup_data;

			/* In the future, we have to add code here to
			 * resolve the popup_data parameters.  This has to
			 * be done in the procedure AppMenuPost also.
			 */
			popup_data.proc		= (OlMenuPositionProc)NULL;
			popup_data.emanate	= (Widget)NULL;
			popup_data.emanate_index = (Cardinal)OL_NO_ITEM;

			PostMenu(menu, &popup_data);
		} 
		else
		{
			PostMenu(menu, (PopupData *)client_data);
		}
	}
	else {			/* Menu is already up	*/

		XMapWindow(dpy, win);

		/* Remove the Menu's parent widget pointer to prevent
		 * this menu or any of its children from being in the
		 * same cascade as any ancestry cascade.
		 * This kludge should go away when the pinned menu
		 * becomes a duplicate of the unpinned menu		*/

	/*	Comment out next line since it causes problems
		in the R3 Intrinsics
			
		menu->core.parent = (Widget) NULL;
	*/
	}

	/* Revert focus to the widget that had the
                focus before this shell was popped */

	if(menu->menu.revert_focus != (Widget)NULL) { 
                OlMoveFocus(menu->menu.revert_focus,
                                        OL_IMMEDIATE,CurrentTime);
                menu->menu.revert_focus = (Widget)NULL;
	} else /* This means the focus was originally outside this application */
		OlMoveFocus(XtParent(menu), OL_IMMEDIATE,
						CurrentTime);
} /* END OF 	PinPutIn() */

/*
 *************************************************************************
 * RelocateMenu - this routine moves the menu according to the position
 * routine.  It also warps the pointer if the positioning routine
 * wants it warped.
 ****************************procedure*header*****************************
 */
static void
RelocateMenu (MenuShellWidget menu, Position px, Position py,
	      PopupData *popup_data)
{
	Position	mx = menu->core.x;
	Position	my = menu->core.y;
	OlDefine	state;

				/* Store the pointer location at Button Press 
					to use in ButtonRelease	*/
	menu->menu.press_x = px;
	menu->menu.press_y = py;
					/* Store the pointer location	*/
	menu->menu.post_x = px;
	menu->menu.post_y = py;

					/* Call the positioning procedure
					 * if there is one.		*/

	if (popup_data->proc != (OlMenuPositionProc)NULL)
	{
		if (menu->menu.root != NULL &&
			menu->menu.root->menu.shell_behavior
					== PressDragReleaseMenu)
			state = OL_PRESS_DRAG_MENU;
		else if (menu->menu.root != NULL &&
				menu->menu.root->menu.shell_behavior
							== StayUpMenu)
			state = OL_STAYUP_MENU;
		else
			state = OL_PINNED_MENU;

		(*(popup_data->proc))((Widget)menu, popup_data->emanate,
					popup_data->emanate_index,
					state, &mx, &my, &px, &py);

		if (menu->menu.post_x != px || menu->menu.post_y != py)
		{
			menu->menu.post_x = px;
			menu->menu.post_y = py;

			XWarpPointer(XtDisplay((Widget)menu), (Window) NULL,
				RootWindowOfScreen(XtScreen((Widget)menu)),
				0, 0, (unsigned int)0, (unsigned int)0,
				(int)px, (int)py);
		}
	}

	XtMoveWidget((Widget)menu, mx, my);
} /* END OF RelocateMenu() */
	
/*
 *************************************************************************
 * RemoveFormExpose - this routine removes the Event handler that was
 * added to the form.  It is called via the form's DestroyCallback list.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
RemoveFormExpose(Widget w, XtPointer client_data, XtPointer call_data)
	      		  			/* The form widget	*/
	         	            
	         	          
{
	Widget menu = w->core.parent;

	XtRemoveEventHandler(w, ExposureMask, False, FormExpose,
			(XtPointer) menu);
} /* END OF RemoveFormExpose() */

/*
 *************************************************************************
 * SetStayUpMode - initializes the application for a "stay-up" menu.  It
 * makes every button on the current menu behave for a stay-up mode.
 ****************************procedure*header*****************************
 */
static void
SetStayUpMode(MenuShellWidget menu)
{
	REMOVE_TIMER;

			/* If this menu is not the root of the cascade
			 * and the root of the cascade is not in StayUp
			 * mode, we must set it to StayUp mode.		*/

	if (menu != menu->menu.root && 
	    menu->menu.root != NULL &&
	    menu->menu.root->menu.shell_behavior == PressDragReleaseMenu)
	{
		MenuShellWidget self = menu->menu.root;

		for( ; self != (MenuShellWidget)NULL; self = self->menu.next)
		{
			_OlPropagateMenuState(self, StayUpMenu);
		}
	}

				/* Set the menu state, and inform menu
				 * items of the new state		*/

	_OlPropagateMenuState(menu, StayUpMenu);

				/* Revert the button's background	*/

	if (menu->menu.revert_button != (OlRevertButtonProc) NULL)
		(* menu->menu.revert_button)((Widget)menu,True);

} /* END OF SetStayUpMode() */

/*
 *************************************************************************
 * ShadowStipple - creates a stipple Bitmap.  This routine caches the
 * bitmaps based on the screen.
 ****************************procedure*header*****************************
 */
static Pixmap
ShadowStipple(Widget w, Boolean create)
	      	  			/* menu widget id		*/
	       	       			/* create or destroy flag	*/
{
	static unsigned char stipple_bits[] = {
	   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
	   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
	   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
	   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa
	};
	typedef struct _PixCache {
		struct _PixCache *	next;
		struct _PixCache *	prev;
		Screen *		screen;
		Pixmap			stipple;
		int			ref_count;
	} PixCache;
	unsigned int		stipple_width = 16;
	unsigned int		stipple_height = 16;
	static PixCache *	list_head = (PixCache *)NULL;
	PixCache *		self;
	Screen *		screen = XtScreen(w);

					/* Find the cached screen	*/

	for (self = list_head; self != (PixCache *)NULL; self = self->next) {
		if (self->screen == screen)
			break;
	}

	if (create == True) {
		if (self == (PixCache *) NULL) {
			self = (PixCache *) XtMalloc(sizeof(PixCache));
			self->prev	= (PixCache *) NULL;
			self->screen	= screen;
			self->ref_count	= 0;
			self->stipple	= XCreateBitmapFromData(XtDisplay(w),
						RootWindowOfScreen(screen),
						(char *)stipple_bits,
						stipple_width, stipple_height);
			self->next	= list_head;

			if (list_head != (PixCache *) NULL) {
				list_head->prev	= self;
			}

					/* Put the new node at the head
					 * of the current list		*/

			list_head = self;
		}

		++self->ref_count;
		return(self->stipple);
	}
	else {
		if (--self->ref_count == 0) {

						/* Reset trailing node	*/

			if (self->next != (PixCache *) NULL)
				self->next->prev = self->prev;

						/* Reset preceding node	*/

			if (list_head == self) {
				list_head = self->next;
			}
			else {
				self->prev->next = self->next;
			}

					/* Free the pixmap and the node	*/

			XFreePixmap(XtDisplay(w), self->stipple);
			XtFree((char *) self);
		}

		return((Pixmap) NULL);
	}
} /* END OF ShadowStipple() */

/*
 *************************************************************************
 * UpdateColors - this routine updates the backgrounds of the menu's
 * children.  It also insures that the background of the menu has
 * not changed.  This routine assumes a check has been made to see if
 * the menu's form and pane haven't been destroyed.  The SetValues()
 * procedure made this check and warned the application programmer of
 * any potential problems.
 *************************************************************************
 */
static void
UpdateColors (MenuShellWidget menu, ArgList args, Cardinal num_args)
{
	ArgList		composed_args;
	Cardinal	num_composed_args;
	MaskArg		mask_args[4];
	Boolean		bg_changed = False;

	if (num_args == (Cardinal)0)
	{
		return;
	}

	_OlSetMaskArg(mask_args[0], XtNforeground, NULL, OL_SOURCE_PAIR);
	_OlSetMaskArg(mask_args[1], XtNbackground, NULL, OL_SOURCE_PAIR);
	_OlSetMaskArg(mask_args[2], XtNbackgroundPixmap, NULL, OL_SOURCE_PAIR);
	_OlSetMaskArg(mask_args[3], XtNfont, NULL, OL_SOURCE_PAIR);

	_OlComposeArgList(args, num_args, mask_args, 4,
				&composed_args, &num_composed_args);

	if (num_composed_args == (Cardinal)0)
	{
		return;
	}

#define SAME_NAME(n1, n2)  (!strcmp((const char *)n1,(const char*)n2))
	for (num_args = 0; num_args < num_composed_args; ++num_args)
	{
		if (SAME_NAME(composed_args[num_args].name, XtNbackground) ||
		    SAME_NAME(composed_args[num_args].name,
					XtNbackgroundPixmap))
		{
			bg_changed = True;
		}
		else if (SAME_NAME(composed_args[num_args].name,
					XtNforeground)	&&
			 menu->menu.title_widget != (Widget) NULL)
		{
				/* If the foreground changed, overrided
				 * the title's fontColor
				 */
			XtVaSetValues(menu->menu.title_widget,
				XtNfontColor, composed_args[num_args].value,
				NULL);
		}
	}
#undef SAME_NAME

			/* Do the form widget
			 */

	XtSetValues(menu->composite.children[0], composed_args,
			num_composed_args);

			/* do the menu pane
			 */

	XtSetValues((Widget)menu->menu.pane, composed_args, num_composed_args);

	if (menu->menu.title_widget != (Widget) NULL) {
		XtSetValues(menu->menu.title_widget, composed_args,
				num_composed_args);
	}

	if (menu->menu.pushpin != (Widget) NULL) {
		XtSetValues(menu->menu.pushpin, composed_args,
				num_composed_args);
	}

			/* Always reset the background of the
			 * the menu if the bg pixel or bg pixmap
			 * changed.					*/

	if (bg_changed == True) {
		menu->core.background_pixmap = def_bg_pixmap;

		if (XtIsRealized((Widget)menu) == True) {
			XSetWindowBackgroundPixmap(XtDisplay(menu),
				XtWindow(menu), menu->core.background_pixmap);
		}
	}

	XtFree((XtPointer) composed_args);
} /* END OF UpdateColors() */

/*
 *************************************************************************
 * UnpostMenu - this routine pops down a menu.  If there are any 
 * non-pinned menus hanging off of this one, pop them down first.  This
 * sequence occurs recursively.
 * The second parameter indicates whether or not this routine should
 * attempt to reset input focus.  When this routine calls itself
 * recursively, the 'reset_focus' flag is always set to False.
 ****************************procedure*header*****************************
 */

static void
ClearMenuGrab (register MenuShellWidget menu)
{
	if (menu->shell.grab_kind != XtGrabNone) {
	    _OlRemoveGrab((Widget) menu);

	    menu->shell.grab_kind     = XtGrabNone;
	    menu->shell.spring_loaded = False;
	}
}

static void
UnpostMenu (MenuShellWidget menu, Boolean reset_focus,
	    Boolean revert_menu_button, Boolean clear_grab)
{
	MenuShellWidget	self;

	if (menu->shell.popped_up == False) {
		return;
	}
					/* Remove sub-cascades first	*/

	if (menu->menu.next != (MenuShellWidget)NULL)
	{
		UnpostMenu(menu->menu.next, True, True, True);
	}

				/* If this menu is the root of the 
				 * cascade, unregister it's grabs	*/

	if (menu->menu.root == menu)
	{
		REMOVE_TIMER;
		SET_ROOT(NULL);
		_OlUngrabPointer((Widget) menu);
		_OlUngrabServer((Widget) menu);
	}

		/* If the reset_focus flag is true and this menu is
		 * emanating from an object which is on a transitory
		 * menu, set the focus to the emanate object.		*/

	if (reset_focus == True)
    		if((menu->menu.shell_behavior == PressDragReleaseMenu ||
     			menu->menu.shell_behavior == StayUpMenu)) {
			if(menu->menu.revert_focus  != (Widget)NULL)
				(void)OlCallAcceptFocus(
					menu->menu.revert_focus,
					CurrentTime);
			else /* This means the focus was originally 
				outside this application */
				OlMoveFocus(XtParent(menu),
						OL_IMMEDIATE,
						CurrentTime);
			menu->menu.revert_focus = (Widget)NULL;
		}  

					/* Now do the stuff for this
					 * menu				*/

	if (menu->menu.shell_behavior != PinnedMenu)
	{
		XtUnmapWidget((Widget) menu);
	}

	if (revert_menu_button &&
	    menu->menu.revert_button != (OlRevertButtonProc) NULL) {
		(* menu->menu.revert_button)((Widget)menu,False);
	}

				/* reset grab_kind field and the
				 * spring_loaded field if necessary	*/

	if (clear_grab) ClearMenuGrab(menu);

			/* Remove the "next" field from the parent of
			 * this menu and set "root", and "next" to NULL	*/


	for(self = menu->menu.root;
	    self != (MenuShellWidget)NULL; self = self->menu.next)
	{
		if (self->menu.next == menu)
		{
			self->menu.next = (MenuShellWidget)NULL;
		}
	}
	menu->menu.root = (MenuShellWidget) NULL;
	menu->menu.next = (MenuShellWidget) NULL;

	if (menu->menu.shell_behavior != PinnedMenu)
	{
		menu->shell.popped_up = FALSE;
		XtCallCallbacks((Widget) menu, XtNpopdownCallback,	
				(XtPointer) menu);
	}
} /* END OF UnpostMenu() */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * ActivateWidget - this procedure allows external forces to activate this
 * widget indirectly.
 ****************************procedure*header*****************************
*/
/* ARGSUSED */
static Boolean
ActivateWidget (Widget w, OlVirtualName type, XtPointer data)
{
	Boolean		ret_val = FALSE;
	MenuShellWidget	menu = (MenuShellWidget)w;

	switch(type) {
	case OL_DEFAULTACTION:
		ret_val = TRUE;

		if ((w = _OlGetDefault(w)) != (Widget)NULL) {
			(void)OlActivateWidget(w, OL_SELECTKEY,
						(XtPointer)NULL);
		}
		break;
	case OL_CANCEL:
		ret_val = TRUE;
		if (menu->menu.shell_behavior == PinnedMenu) {
			DismissMenu(menu);
		} else {
			if(menu->menu.root != NULL)
				UnpostMenu(menu->menu.root, True, True, True);
		}
		break;
	case OL_TOGGLEPUSHPIN:
		ret_val = TRUE;
		if (menu->menu.pushpin != NULL) {
			OlDefine	pin_state;
			Arg		arg[1];

			XtSetArg (arg[0], XtNpushpin, &pin_state);
			XtGetValues(menu->menu.pushpin, arg, 1);
			if (pin_state == OL_OUT)
				(void)OlActivateWidget(menu->menu.pushpin,
					OL_SELECTKEY, (XtPointer)NULL);
			else
				DismissMenu(menu);
		}
		break;
	}
	return(ret_val);
} /* END OF ActivateWidget() */

static void
WMMsgHandler (Widget w, OlDefine action, OlWMProtocolVerify *wmpv)
{
	MenuShellWidget		menu = (MenuShellWidget)w ;
	ShellBehavior		msb = menu->menu.shell_behavior;

	if ((wmpv->msgtype == OL_WM_DELETE_WINDOW) &&
	    (msb == PinnedMenu)) {
		switch(action) {
		case OL_DEFAULTACTION:
		case OL_DISMISS:
			DismissMenu(menu);
			break;
		case OL_QUIT:
			XtUnmapWidget(w);
			exit(EXIT_SUCCESS);
			break;
		case OL_DESTROY:
			XtDestroyWidget(w);
			break;
		}
	}
}


/*
 *************************************************************************
 * ChangedManaged - This procedure handles the management concerns when
 * the Menu Shell takes on children.
 * Since the menus are supposed to have "drop shadows," the menu's window
 * should be slightly larger than the child's window.
 ****************************procedure*header*****************************
 */
static void
ChangeManaged(Widget widget)
{
    register MenuShellWidget	m = (MenuShellWidget) widget;
    register int		i;
    Widget			child;
    Boolean			needresize = FALSE;
    Dimension			width_add = 0;
    Dimension			height_add = 0;


    if (m->menu.shell_behavior != PinnedMenu)
    {
	width_add = m->menu.shadow_right;
	height_add = m->menu.shadow_bottom;
    }

				/* Loop Over managed Children	*/

    for (i = 0; i < m->composite.num_children; i++)
    {
	if (m->composite.children[i]->core.managed)
	{
		child = m->composite.children[i];

				/* Set the Width and Height of the
				 * Menu if either dimension is zero	*/

		if (m->core.width == 0 || m->core.height == 0)
		{
		    if (child->core.height == 0)
		    {
			child->core.height = 1;

			OlVaDisplayWarningMsg(XtDisplay((Widget)m), "","",
				OleCOlToolkitWarning,
				"Menu \"%s\" has child \"%s\" with\
 zero height, forcing to 1", m->core.name, child->core.name);
		    }

		    if (child->core.width == 0)
		    {
			child->core.width = 1;

			OlVaDisplayWarningMsg(XtDisplay((Widget)m), "","",
				OleCOlToolkitWarning,
				"Menu \"%s\" has child \"%s\" with\
 zero width, forcing to 1", m->core.name, child->core.name);
		    }

				/* Remember we have a border width of 1 on
				 * the child, so add 2 to width/height	*/

		    if (m->core.width == 0)
		    {
			m->core.width = child->core.width + width_add + 2;
		    }

		    if (m->core.height == 0)
		    {
			m->core.height = child->core.height + height_add + 2;
		    }

		} else needresize = TRUE;

				/* Make the Menu's child have a border
				 * width of 1 pixel			*/

		if (child->core.border_width != 1) {
			child->core.border_width = 1;
			needresize = TRUE;
		}

				/* Make resize changes if needed	*/

		if (needresize == TRUE)
		{
			XtResizeWidget(child, m->core.width - width_add,
				m->core.height - height_add,
				child->core.border_width);
		}

		if (child->core.x != 0 || child->core.y != 0)
		{
			XtMoveWidget(child, (Position) 0, (Position) 0);
		}

		break;		/* only one managed child allowed	*/

	}			/* End of changing sole managed child	*/
    }				/* End of looping over children		*/
} /* END OF ChangeManaged() */

/*
 *************************************************************************
 * ClassInitialize - one shot initialization for the
 * MenuShellWidgetClass
 ****************************procedure*header*****************************
 */
static void
ClassInitialize(void)
{
	extern Region nullRegion;
	extern Region dest_region;
	extern Region hold_region;
	extern Region temp_region;

				/* Add the record type to the extension	*/

	vendor_extension->header.record_type = OlXrmVendorClassExtension;

				/* One Time Region Initialization	*/

	nullRegion  = XCreateRegion();
	dest_region = XCreateRegion();
	hold_region = XCreateRegion();
	temp_region = XCreateRegion();

	_OlAddOlDefineType ("none", OL_NONE);
	_OlAddOlDefineType ("out",  OL_OUT);
} /* END OF ClassInitialize() */

/*
 *************************************************************************
 * Destroy - handles the destruction of fields associated with the 
 * Menu Shell Widget.  For example, we must destroy the GC's that
 * we created.
 * 	We must also remove the Event Handlers that we gave the parent,
 * if we originally augmented the parent's event handler list.
 ****************************procedure*header*****************************
 */
static void
Destroy(Widget widget)
{
	Widget		parent = widget->core.parent;
	MenuShellWidget	menu = (MenuShellWidget) widget;

				/* If this widget is the root of a
				 * menu cascade, we better ungrab
				 * the Server				*/

	if (menu->menu.root == menu)
	{
		REMOVE_TIMER;
		SET_ROOT(NULL);
		_OlUngrabServer((Widget) menu);
	}

	if (menu->menu.backdrop_right != (Pixmap) NULL)
		XFreePixmap(XtDisplay(widget), menu->menu.backdrop_right);
	if (menu->menu.backdrop_bottom != (Pixmap) NULL)
		XFreePixmap(XtDisplay(widget), menu->menu.backdrop_bottom);


				/* Destroy the GC's, destroy the stipple
				 * pixmap in the shadow GC		*/

	(void) ShadowStipple(widget, False);
	XtDestroyGC(menu->menu.backdrop_GC);
	XtDestroyGC(menu->menu.shadow_GC);

					/* Remove our Event Handlers	*/

	XtRemoveEventHandler((Widget)menu, (EventMask) StructureNotifyMask,
			     False, EventHandler, NULL);

			/* If we augmented the parent's event handlers,
			 * remove them before we destroy ourself	*/

	if (menu->menu.augment_parent == True && parent != NULL) {

		XtRemoveEventHandler(parent, ButtonPressMask,
			False, ApplMenuPost, (XtPointer) widget);
	}

#ifdef WORKAROUND
			/* If we're being destroyed, then make
			 * sure to replace our parent's pointer
			 * so that our superclasses can do what they
			 * have to.  In the future when a pinned menu
			 * is not the same as a transitory one, we
			 * won't have to worry about the parent pointer
			 * and hence, the following lines of code
			 * will be removed.				*/

	if (menu->menu.shell_behavior == PinnedMenu)
	{
		menu->core.parent = menu->menu.parent_cache;
	}
#endif

} /* END OF Destroy() */

/*
 *************************************************************************
 * Redisplay - handles the writing of the drop shadow.  Since we like
 * to compress all exposure events, the "region" will not be NULL.
 * Therefore, to optomize the refresh, we will use the information in the
 * region to our advantage.  To extract the information in the region
 * that we want, we must call XClipBox().  But, since we know that the
 * Xt Toolkit already did this and updated the expose components of the
 * XEvent, we just have to look at the XEvent to get the necessary
 * information.
 ****************************procedure*header*****************************
 */
static void
Redisplay(Widget widget, register XEvent *xevent, register Region region)
	                        	/* The Menu ShellWidget	*/
	                        	/* Exposure XEvent 		*/
	                        	/* Region			*/
{
	register MenuShellWidget menu = (MenuShellWidget) widget;
	Pixmap            src;
	int               query;
	int               i;
	int               x_offset;
	int               y_offset;
	static XRectangle rect;
	extern Region     nullRegion;
	extern Region     dest_region;
	extern Region     temp_region;

					/* Make sure this is necessary	*/
	if (menu->shell.popped_up == False ||
	    menu->menu.shell_behavior == PinnedMenu)
	{
		return;
	}

	for(i = 0; i < 2; ++i) {
#ifdef WORKAROUND
				/* below calculations assume that
				 * the width and height of the menu
				 * always is greater than
				 * the shadow's short dimension		*/
#endif
	    if (i == 0) {	/* Right edge rectangle			*/

		rect.width  = (unsigned short)menu->menu.shadow_right;
		rect.height = (unsigned short)
				(menu->core.height - menu->menu.shadow_bottom);
		rect.x      = (short)menu->core.width - rect.width;
		rect.y      = 0;
		x_offset    = (int)rect.x;
		y_offset    = (int)rect.y;
		src         = menu->menu.backdrop_right;
	    }
	    else {		/* Bottom edge rectangle		*/

		rect.width  = (unsigned short)menu->core.width;
		rect.height = (unsigned short)menu->menu.shadow_bottom;
		rect.x      = 0;
		rect.y      = (short)(menu->core.height - rect.height);
		x_offset    = (int)rect.x;
		y_offset    = (int)rect.y;
		src         = menu->menu.backdrop_bottom;
	    }

	    if (region != (Region) NULL)
		query = XRectInRegion(region, rect.x, rect.y,
				rect.width, rect.height);
	    else
		query = RectangleIn;

	    if (query != RectangleOut) {
		if (query == RectanglePart) {
		    XUnionRectWithRegion(&rect, nullRegion, temp_region);
		    XIntersectRegion(region, temp_region, dest_region);
		    XClipBox(dest_region, &rect);
		}

		XCopyArea(xevent->xany.display,
			src,
			xevent->xany.window,
			menu->menu.backdrop_GC,
			(int)((int)rect.x - x_offset),
			(int)((int)rect.y - y_offset),
			(unsigned int)rect.width,
			(unsigned int)rect.height,
			(int)rect.x, (int)rect.y);
	    }
	} /* END OF FORLOOP */

} /* END OF Redisplay() */

/*
 *************************************************************************
 * GeometryManager - this manages geometry management requests.  It's a
 * bit tricky since this version of the menu must be used for pinned
 * and transistory menus.  Therefore, we must take into account the
 * menu's dropshadow when considering requests.
 *
 * Also, since the Xt Intrinsics do not update the shell's core fields
 * in a request, we'll update the widget's width an height and hope the
 * window manager honors these.  (From Intrinsic release R3, the Shell's
 * geometry manager does an XConfigureWindow and waits for the 
 * ConfigureNotify response before updating the core fields.
 ****************************procedure*header*****************************
 */
static XtGeometryResult
GeometryManager(Widget child, XtWidgetGeometry *request, XtWidgetGeometry *reply)
	      			      		/* Requesting child id	*/
	                  	        	/* The request		*/
	                  	      		/* The reply		*/
{
	MenuShellWidget		menu = (MenuShellWidget) XtParent(child);
	XtGeometryResult	(*geom)();
	XtGeometryResult	result;
	XtWidgetGeometry	new_request;

	new_request = *request;

	geom = (XtGeometryResult (*)()) ((CompositeWidgetClass)
			menuShellWidgetClass->core_class.superclass)->
					composite_class.geometry_manager;

	if (menu->menu.shell_behavior == PinnedMenu)
	{
		result = (* geom)(child, request, reply);
	}
	else
	{
		
				/* Must account for the dropshadow */

		if (request->request_mode & CWWidth)
			new_request.width += menu->menu.shadow_right;

		if (request->request_mode & CWHeight)
			new_request.height += menu->menu.shadow_bottom;

		result = (* geom)(child, &new_request, reply);


				/* Remove the dropshadow from the reply	*/

		if (result != XtGeometryNo)
		{
			if (request->request_mode & CWWidth)
			{
				if (result == XtGeometryYes)
				{
					child->core.width = request->width;
				}
				else
				{
		 			reply->width -= menu->menu.shadow_right;
				}
			}

			if (request->request_mode & CWHeight)
			{

				if (result == XtGeometryYes)
				{
					child->core.height = request->height;
				}
				else
				{
					reply->height -=
						menu->menu.shadow_bottom;
				}
			}
		}
	}

			/* Get New pixmaps for the backdrop shadow	*/

	if (XtIsRealized((Widget) menu) == True)
	{
		GetBackdropPixmaps((Widget)menu);
	}

	if (result == XtGeometryYes)
	{
		if ((request->request_mode & CWWidth) &&
		     menu->core.width != new_request.width)
		{
			menu->core.width = new_request.width;
		}

		if ((request->request_mode & CWHeight) &&
		     menu->core.height != new_request.height)
		{
			menu->core.height = new_request.height;
		}
		OlWidgetConfigured(child);
	}

	return(result);

} /* END OF GeometryManager() */

/*
 *************************************************************************
 * GetDefault - This routine is called by the convenience routine
 * _OlGetDefault.  If the menu has no default this routine attempts to
 * pick one.
 ****************************procedure*header*****************************
 */
static void
GetDefault(Widget w, Widget the_default)
	      	  			/* the menu widget		*/
	      	            		/* Current default widget	*/
{
	if (the_default == (Widget)NULL)
	{
		SetDefault(w, (Widget)NULL);
	}
} /* END OF GetDefault() */

/*
 *************************************************************************
 * GetValuesHook - gets subresource data
 ****************************procedure*header*****************************
 */
static void
GetValuesHook(Widget w, ArgList args, Cardinal *num_args)
	            			/* menu shell widget id		*/
	               			/* Arg List for Menu		*/
	                   		/* number of Args		*/
{
	MaskArg		mask_args[2];

	if (*num_args == (Cardinal)0)
		return;

	_OlSetMaskArg(mask_args[0], XtNmenuPane,
		((MenuShellWidget) w)->menu.pane, OL_COPY_MASK_VALUE);
	_OlSetMaskArg(mask_args[1], NULL, sizeof(Widget), OL_COPY_SIZE);

			/* Use the compose routine to fill in the
			 * value.  We don't need to have any return
			 * pointers since the rule we're using doesn't
			 * generate a return list.			*/

	_OlComposeArgList(args, *num_args, mask_args, XtNumber(mask_args),
			(ArgList *)NULL, (Cardinal *) NULL);

} /* END OF GetValuesHook() */

/*
 *************************************************************************
 * Initialize - Initializes the Menu Instance.  Any conflicts 
 * between the "request" and "new" widgets should be resolved here.
 * This routine makes sure that the parent is allowed to take a menu
 * popup.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
	               			/* What user wants		*/
	           			/* What user gets, so far....	*/
	       		     
	          	         
{
	register MenuShellWidget	nw = (MenuShellWidget) new;
	static Boolean			prevent_attachment;
	Widget				parent = XtParent(new);
	static Arg			query[] =
	{
		{ XtNmenuAttachQuery,    (XtArgVal) &prevent_attachment }
	};
	OlVendorPartExtension part =
			_OlGetVendorPartExtension(_OlGetShellOfWidget(new));

			/* Ask original parent if a menu is allowed to
			 * to be attached to it.  If a non-NULL response
			 * is returned, a warning results because
			 * the parent denied attachment.		*/

	prevent_attachment = False;

	XtGetValues(parent, query, XtNumber(query));

	if (prevent_attachment == (Boolean)True) {
		OlVaDisplayWarningMsg(XtDisplay(new), "","",
			OleCOlToolkitWarning,
			"Widget \"%s\" (class name \"%s\")\
 refused Menu attachment !!", (parent ? parent->core.name : ""),
			(parent ? CNAME(parent) : ""));
	}

			/* If the parent of the menu is a MenuButton,
			 * then the revert_button proc is set and we
			 * know that this is not an application menu.   */

	nw->menu.application_menu =
	    (nw->menu.revert_button == (OlRevertButtonProc) NULL) ? True :
		False;
	nw->menu.emanate = XtParent(new);


			/* If the parent wishes us to add Event handlers
			 * for it, do so.				*/

	if (((MenuShellWidget) new)->menu.augment_parent == True) {
		XtAddEventHandler(nw->core.parent, ButtonPressMask | KeyPressMask,
			False, ApplMenuPost, (XtPointer) nw);
	}


			/* Add an event handler to catch client messages
			 * from the window manager concerning
			 * decorations.					*/

			/* Add an Event Handler	to catch StructureNotify
			 * XEvents					*/

	XtAddEventHandler(new, (EventMask) StructureNotifyMask, False, 
			EventHandler, NULL);



			/* Get the GC's for the Menu			*/

	nw->menu.shadow_GC   = (GC) NULL;
	nw->menu.backdrop_GC = (GC) NULL;

	GetShadowGC(new);
	GetBackdropGC(new);

			/* Initialize the backdrop pixmaps to NULL, and
			 * set the shadow thicknesses			*/

	nw->menu.backdrop_right  = (Pixmap) NULL;
	nw->menu.backdrop_bottom = (Pixmap) NULL;
	nw->menu.shadow_right    = OlScreenPointToPixel(OL_HORIZONTAL,
					SHADOW_THICKNESS, XtScreen(new));
	nw->menu.shadow_bottom   = OlScreenPointToPixel(OL_VERTICAL,
					SHADOW_THICKNESS, XtScreen(new));

				/* Initialize the Cascade Pointers	*/

	nw->menu.root = (MenuShellWidget) NULL;
	nw->menu.next = (MenuShellWidget) NULL;

				/* Check the validity of the pushpin
				 * state resource.			*/
	if (part->pushpin != OL_NONE && 
			part->pushpin != OL_OUT) {
		if(part->pushpin == OL_IN) {
			part->pushpin = OL_OUT;
			OlWarning( dgettext(OlMsgsDomain,
				"Invalid XtNpushpin (MenuShell): set to OL_OUT\n"));
		} else {
			part->pushpin = OL_NONE;
		 	OlWarning( dgettext(OlMsgsDomain,
				"Invalid XtNpushpin (MenuShell): set to OL_NONE\n"));
		}

	}

	nw->core.border_width	= (Dimension)0;
	nw->menu.pushpin        = (Widget) NULL;
	nw->menu.title_widget	= (Widget) NULL;
	nw->menu.pane		= (CompositeWidget) NULL;
	nw->menu.prevent_stayup	= (Boolean)False;
	nw->menu.post_x		= (Position)0;
	nw->menu.post_y		= (Position)0;
	nw->menu.press_x	= (Position)0;
	nw->menu.press_y	= (Position)0;
	nw->menu.revert_focus = (Widget)NULL;


				/* Cache the parent id of this menu	*/

	nw->menu.parent_cache = nw->core.parent;

				/* Prevent other menus from attaching to
				 * this shell				*/

	if (nw->menu.attach_query != QueryDenied) {
		OlWarning(dgettext(OlMsgsDomain,
			"Menu Initialize: cannot modify menu's attach flag"));
		nw->menu.attach_query = QueryDenied;
	}
	if (part && part->footer_present)
		part->footer_present = False;
	if (part && part->im_footer_present)
		part->im_footer_present = False;


} /* END OF Initialize() */

/*
 *************************************************************************
 * InitializeHook - Initializes the Menu Instance.  This routine
 * creates the children of the Menu Shell Widget.
 * Right now, a form widget is used as the child of the Menu shell.
 * The pane and the pushpin are children of the form.
 ****************************procedure*header*****************************
 */
static void
InitializeHook(Widget w, ArgList args, Cardinal *num_args)
	            			/* What user gets, so far....	*/
	               			/* Original Arg List for Menu	*/
	                   		/* Original number of Args	*/
{
	MenuShellWidget	menu = (MenuShellWidget) w;
	Widget		reference;
	Cardinal	m;
	Cardinal	count;
	int		vpad;			/* Vertical padding	*/
	int		hpad;			/* horizontal padding	*/
	int		offset;
	Widget		form;
	char *		pane_name = "pane";
	Arg		tmp_args[36];
	MaskArg		mask_args[36];
	ArgList		comp_args = (ArgList)NULL;	/* composed Args*/
	OlVendorPartExtension part = 
			_OlGetVendorPartExtension(_OlGetShellOfWidget(w));

				/* Now set up the args for the form	*/

	count = 0;
	XtSetArg(tmp_args[count], XtNborderWidth, 0);		++count;

	form = XtCreateManagedWidget("menu_form", formWidgetClass,
			w, tmp_args, count);



				/* Define padding for title & pushpin	*/

	hpad = OlScreenPointToPixel(OL_HORIZONTAL, TITLE_PAD, XtScreen(w));
	if (hpad == 0)
		hpad = 1;

	vpad = OlScreenPointToPixel(OL_VERTICAL, TITLE_PAD, XtScreen(w));
	if (vpad == 0)
		vpad = 1;

						/* Add the pushpin	*/
	if (part->pushpin != (OlDefine)OL_NONE)
	{
	    count = 0;

	    XtSetArg(tmp_args[count], XtNdefault, menu->menu.pushpin_default);
	    ++count;
            XtSetArg(tmp_args[count], XtNpushpin, part->pushpin);
            ++count;
	    XtSetArg(tmp_args[count], XtNxAttachRight, False);
	    ++count;
	    XtSetArg(tmp_args[count], XtNyOffset, vpad);
	    ++count;
	    XtSetArg(tmp_args[count], XtNxAttachOffset, hpad);
	    ++count;
	    XtSetArg(tmp_args[count], XtNxOffset, hpad);
	    ++count;
	    XtSetArg(tmp_args[count], XtNxVaryOffset, False);
	    ++count;

					/* Do a little self checking	*/

		if (count > XtNumber(tmp_args))
			OlError(dgettext(OlMsgsDomain,
				"Menu: pushpin args exceed allocated size"));

		menu->menu.pushpin = XtCreateManagedWidget("pushpin",
				pushpinWidgetClass, form, tmp_args, count);

		_OlDeleteDescendant(menu->menu.pushpin);

				/* Add callbacks to the pushpin		*/

		XtAddCallback(menu->menu.pushpin, XtNpushpinIn, PinPutIn,
			(XtPointer) NULL);
	}
	else {
		menu->menu.pushpin = (Widget) NULL;
	}

			 /* add a widget to display the title.		*/

	if (menu->menu.application_menu == True)
	{
			/* Add an Event handler to draw the line under
			 * the menu's title				*/

		XtAddEventHandler(form, ExposureMask, False, FormExpose,
			(XtPointer)menu);

			/* Now Add a destroy callback to the form to
			 * remove the event handler			*/

		XtAddCallback(form, XtNdestroyCallback, (XtCallbackProc)
			RemoveFormExpose, (XtPointer) NULL);

				/* Determine attributes of the title	*/

		count = 0;
		XtSetArg(tmp_args[count], XtNborderWidth, 0);
		++count;
		XtSetArg(tmp_args[count], XtNlabelJustify, OL_CENTER);
		++count;
		XtSetArg(tmp_args[count], XtNlabel, part->title);
		++count;
		XtSetArg(tmp_args[count],XtNtextFormat,menu->menu.text_format);
		++count;
		XtSetArg(tmp_args[count], XtNbuttonType, OL_LABEL);
		++count;
		XtSetArg(tmp_args[count], XtNrecomputeSize, True);
		++count;
		XtSetArg(tmp_args[count], XtNyOffset, vpad);
		++count;
		XtSetArg(tmp_args[count], XtNxResizable, True);
		++count;
		XtSetArg(tmp_args[count], XtNxVaryOffset, False);
		++count;
		XtSetArg(tmp_args[count], XtNxAttachRight, True);
		++count;
		XtSetArg(tmp_args[count], XtNxAttachOffset, 0);
		++count;
		XtSetArg(tmp_args[count], XtNxOffset, 0);
		++count;
		if (menu->menu.pushpin != NULL) {
			XtSetArg(tmp_args[count], XtNxRefWidget,
					menu->menu.pushpin);
			++count;
			XtSetArg(tmp_args[count], XtNxAddWidth, True);
			++count;
		}
		else
		{
			XtSetArg(tmp_args[count], XtNxRefWidget, form);
			++count;
			XtSetArg(tmp_args[count], XtNxAddWidth, False);
			++count;
		}

					/* Do a little self checking	*/

		if (count > XtNumber(tmp_args))
		   OlError(dgettext(OlMsgsDomain,
		   	"Menu: Title Widget args exceed allocated size"));

		menu->menu.title_widget = XtCreateManagedWidget("title",
					buttonWidgetClass, form,
					tmp_args, count);

		_OlDeleteDescendant(menu->menu.title_widget);
	}
	else {
		menu->menu.title_widget = (Widget) NULL;
	}

				/* Set up the mask arg list for the
				 * menu pane.				*/

	m = 0;

	offset = OlScreenPointToPixel(OL_HORIZONTAL, HORIZ_PADDING,
			XtScreen(w));

	_OlSetMaskArg(mask_args[m], XtNhPad, offset, OL_DEFAULT_PAIR);
		++m;

	_OlSetMaskArg(mask_args[m], XtNhSpace, offset, OL_DEFAULT_PAIR);
		++m;

	offset = OlScreenPointToPixel(OL_VERTICAL, (SHADOW_THICKNESS >>1),
			XtScreen(w));
	_OlSetMaskArg(mask_args[m], XtNvPad, offset, OL_DEFAULT_PAIR);
		++m;

	offset = OlScreenPointToPixel(OL_VERTICAL, VERTICAL_PADDING,
			XtScreen(w));
	if (offset == 0)
	{
		offset = 1;
	}
	_OlSetMaskArg(mask_args[m], XtNvSpace, offset, OL_DEFAULT_PAIR);
		++m;

	_OlSetMaskArg(mask_args[m],XtNlayoutType,OL_FIXEDCOLS,OL_DEFAULT_PAIR);
		++m;
	_OlSetMaskArg(mask_args[m], XtNmeasure, NULL, OL_SOURCE_PAIR);
		++m;
	_OlSetMaskArg(mask_args[m], XtNsameSize, OL_COLUMNS, OL_DEFAULT_PAIR);
		++m;
	_OlSetMaskArg(mask_args[m], XtNcenter, True, OL_ABSENT_PAIR);
		++m;
	_OlSetMaskArg(mask_args[m], XtNpreSelect, PreSelectCB,
			OL_DEFAULT_PAIR); ++m;

	_OlSetMaskArg(mask_args[m], XtNpostSelect, PostSelectCB,
			OL_DEFAULT_PAIR); ++m;

					/* Set core-related resources	*/

	_OlSetMaskArg(mask_args[m], XtNx, 0, OL_OVERRIDE_PAIR);		++m;
	_OlSetMaskArg(mask_args[m], XtNy, 0, OL_OVERRIDE_PAIR);		++m;
	_OlSetMaskArg(mask_args[m], XtNwidth, 0, OL_OVERRIDE_PAIR);	++m;
	_OlSetMaskArg(mask_args[m], XtNheight, 0, OL_OVERRIDE_PAIR);	++m;
	_OlSetMaskArg(mask_args[m], XtNborderWidth, 0, OL_OVERRIDE_PAIR);++m;

					/* Set Form's constraints	*/

	if (menu->menu.application_menu == True) {
		offset = vpad;

		if (menu->menu.pushpin != (Widget) NULL &&
		    menu->menu.pushpin->core.height >
					menu->menu.title_widget->core.height) {
			reference = menu->menu.pushpin;
		}
		else {
			reference = menu->menu.title_widget;
		}
	}
	else if (menu->menu.pushpin != (Widget) NULL)  {
		reference = menu->menu.pushpin;
		offset = vpad;
	}
	else {
		offset = 0;
		reference = form;
	}

	_OlSetMaskArg(mask_args[m], XtNyRefWidget, reference, OL_OVERRIDE_PAIR);
		++m;
	_OlSetMaskArg(mask_args[m], XtNyAddHeight, True, OL_OVERRIDE_PAIR);
		++m;
	_OlSetMaskArg(mask_args[m], XtNyVaryOffset, False, OL_OVERRIDE_PAIR);
		++m;
	_OlSetMaskArg(mask_args[m], XtNyOffset, offset, OL_OVERRIDE_PAIR);
		++m;
	_OlSetMaskArg(mask_args[m], XtNxResizable, True, OL_OVERRIDE_PAIR);
		++m;
	_OlSetMaskArg(mask_args[m], XtNyAttachBottom, True, OL_OVERRIDE_PAIR);
		++m;
	_OlSetMaskArg(mask_args[m], XtNxAttachRight, True, OL_OVERRIDE_PAIR);
		++m;
	_OlSetMaskArg(mask_args[m], XtNpaneName, &pane_name,
				OL_COPY_SOURCE_VALUE);
		++m;
	_OlSetMaskArg(mask_args[m], NULL, sizeof(String), OL_COPY_SIZE);
		++m;

			/* Do a little self checking			*/

	if (m > XtNumber(mask_args))
		OlError(dgettext(OlMsgsDomain,
				"Menu: pane args exceed allocated size"));

			/* Build the Resource list for the pane		*/

	_OlComposeArgList(args, *num_args, mask_args, m, &comp_args, &count);


				/* Create the Pane			*/

	menu->menu.pane = (CompositeWidget) XtCreateManagedWidget(
				pane_name, controlAreaWidgetClass,
				form, comp_args, count);

	XtFree((char *)comp_args);

					/* Set the menu Shell behavior	*/

	_OlPropagateMenuState(menu, PressDragReleaseMenu);

	UpdateColors(menu, args, *num_args);

} /* END OF InitializeHook() */

/*
 *************************************************************************
 * Realize - this routine realizes the menu shell widget and sets the
 * window's decoration hints.
 ****************************procedure*header*****************************
 */
static void
Realize(Widget w, XtValueMask *value_mask, XSetWindowAttributes *attributes)
{
	MenuShellWidget	menu = (MenuShellWidget) w;
	Widget		form = menu->composite.children[0];

				/* Make Sure there's a default widget	*/

	(void) _OlGetDefault((Widget)menu);

					/* First, realize the menu	*/

	(* (menuShellClassRec.core_class.superclass->core_class.realize))
		(w, value_mask, attributes);

			/* Realize the descendents of the menu and 
			 * uninstall translations of the title, form,
			 * and menupane.				*/

	/* Set the cursor as the standard cursor for the menu shell, since
	   its parent is root (1126352). */
	XDefineCursor (XtDisplay (w), XtWindow (w), OlGetStandardCursor (w) );

	XtRealizeWidget(form);
	XtUninstallTranslations(form);
	if (menu->menu.title_widget != (Widget)NULL) {
		XtUninstallTranslations(menu->menu.title_widget);
	}


} /* END OF Realize() */

/*
 *************************************************************************
 * Resize - resizes the child of the Menu shell widget.  It makes sure
 * that there is room for the dropshadow.
 ****************************procedure*header*****************************
 */
static void
Resize(Widget widget)
{
	register MenuShellWidget m = (MenuShellWidget) widget;
	register Widget          child;
	register int		 i;
	register Dimension	 width = m->core.width;
	register Dimension	 height = m->core.height;

			/* Get New pixmaps for the backdrop shadow	*/

	if (XtIsRealized(widget) == True)
		GetBackdropPixmaps((Widget)m);

			/* If we're not in some pinned situation, remove
			 * the width and height of the drop shadow since
			 * we've made the shell that much larger	*/

	if (m->menu.shell_behavior != PinnedMenu)
	{
		width -= (Dimension) m->menu.shadow_right;
		height -= (Dimension) m->menu.shadow_bottom;
	}

	for(i=0; i < m->composite.num_children; ++i) {
		child = m->composite.children[i];
		if (child->core.managed)
			XtResizeWidget(child, width, height, 
				child->core.border_width);
	}

} /* END OF Resize() */
	
/*
 *************************************************************************
 * SetValues - used to set resources associated with the MenuPart.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
	               		/* The original Widget			*/
	               		/* Widget user wants			*/
	           		/* Widget user gets, so far ....	*/
	       		     
	          	         
{
	Boolean		redisplay = False;
	unsigned int	mask = (unsigned int)0;
	XWindowChanges	xwc;
	MenuShellWidget	cw = (MenuShellWidget) current;
	MenuShellWidget	nw = (MenuShellWidget) new;
	MenuShellWidget	rw = (MenuShellWidget) request;

				/* Because of a bug in WMShell, compare
				 * the requested title to the current.  If
				 * they diff, do a string compare between
				 * request and new.  If the strings are the
				 * same, assume a new title has been
				 * accepted.				*/

	if (rw->wm.title != cw->wm.title &&
	    nw->menu.application_menu == (Boolean) True &&
	    nw->menu.title_widget != (Widget) NULL &&
	    !strcmp((const char *)nw->wm.title, (const char *)rw->wm.title))
	{
		Arg	args[1];

		XtSetArg(args[0], XtNlabel, nw->wm.title);
		XtSetValues(nw->menu.title_widget, args, 1);
	}

	if (nw->core.border_width != cw->core.border_width)
	{
		nw->core.border_width = 0;
	}

	if (nw->menu.pane != cw->menu.pane)
	{
		OlWarning(dgettext(OlMsgsDomain,
			"Menu SetValues: cannot modify XtNmenuPane"));
		nw->menu.pane = cw->menu.pane;
	}

	if (nw->menu.attach_query != cw->menu.attach_query)
	{
		OlWarning(dgettext(OlMsgsDomain,
			"Menu SetValues: cannot modify menu's attach flag"));
		nw->menu.attach_query = cw->menu.attach_query;
	}

	if (nw->menu.pushpin_default != cw->menu.pushpin_default &&
	    nw->menu.pushpin != (Widget) NULL)
	{
		_OlSetDefault(nw->menu.pushpin, nw->menu.pushpin_default);
	}

						/* Update the colors	*/

	if (nw->menu.foreground != cw->menu.foreground)
	{
					/* Update the GC's	*/

		GetShadowGC((Widget) nw);
		GetBackdropGC((Widget) nw);
	}
	UpdateColors(nw, args, *num_args);

				/* Since the menu is subclass of a shell
				 * widget, we have to take care of 
				 * geometry changes before we leave this
				 * function.				*/

#define XWCHANGE(field,flag)	if (nw->core.field != cw->core.field) {\
		mask |= (unsigned int) flag;\
		cw->core.field = nw->core.field;\
		xwc.field = (int) nw->core.field;}

	XWCHANGE(x,CWX)
	XWCHANGE(y,CWY)
	XWCHANGE(width,CWWidth)
	XWCHANGE(height,CWHeight)
	XWCHANGE(border_width,CWBorderWidth)
#undef XWCHANGE

	if (mask != (unsigned int)0 &&
	    XtIsRealized(new) == True)
	{
		XConfigureWindow(XtDisplay(new),XtWindow(new), mask, &xwc);
	}

	return(	redisplay );
} /* END OF SetValues() */

/*
 *************************************************************************
 * SetDefault - handles the changing of the menu's default.  This
 * routine is called indirectly by the convenience routine _OlSetDefault.
 * If there's no new default, this routine attempts to set a new one.
 ****************************procedure*header*****************************
 */
static void
SetDefault(Widget menu_widget, Widget new_default)
{
	MenuShellWidget	menu = (MenuShellWidget) menu_widget;
	Arg		args[1];

	if (new_default != NULL_WIDGET)
	{
		if (menu->menu.pushpin == (Widget) NULL ||
		    menu->menu.pushpin != new_default)
		{
			menu->menu.pushpin_default = False;
		}

		return;
	}

	XtSetArg(args[0], XtNdefault, True);

		/* If there's a pushpin on this menu, make it
		 * the default; else, search the pane for the
		 * first eligible child.			*/

	if (menu->menu.pushpin != (Widget) NULL &&
	    menu->menu.pushpin->core.managed == True &&
	    menu->menu.pushpin->core.mapped_when_managed == True)
	{
		XtSetValues(menu->menu.pushpin, args, 1);
		menu->menu.pushpin_default = True;
	}
	else
	{
		Cardinal	num_kids = ((CompositeWidget)
				(menu->menu.pane))->composite.num_children;
		WidgetList	kids = ((CompositeWidget)
					(menu->menu.pane))->composite.children;

		menu->menu.pushpin_default = False;

		for (; num_kids != 0; --num_kids, ++kids) {
			if (XtIsManaged(*kids))
			{
				XtSetValues((*kids), args, 1);
				break;
			}
		}
	}
} /* END OF SetDefault() */

/*
 *************************************************************************
 * TraversalHandler - this routine handles the traversal for the menu
 * widget.  If the current widget with focus is on the left side of the
 * menu's pane, and the user presses an OL_MOVELEFT or OL_MULTILEFT, the
 * menu will unpost itself.
 ****************************procedure*header*****************************
 */
static Widget
TraversalHandler (Widget shell, Widget w, OlDefine direction, Time time)
{
	MenuShellWidget	menu = (MenuShellWidget) shell;
	Widget		w_return;

	if ((direction == OL_MOVELEFT || direction == OL_MULTILEFT) &&
	    menu->menu.shell_behavior == StayUpMenu)
	{
		UnpostMenu(menu, True, True, True);
		w_return = NULL;
	}
	else			/* Let the vendor class handle the key	*/
	{
		OlVendorClassExtension	ext =
			_OlGetVendorClassExtension(vendorShellWidgetClass);

		w_return = (*ext->traversal_handler)(shell, w, direction, time);
	}
	return(w_return);
} /* END OF TraversalHandler() */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 * ApplMenuPost - this procedure handles what happens when a menu
 * button or a menu key is pressed.  This routine is for application menu parents.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
ApplMenuPost(register Widget w, XtPointer data, XEvent *xevent, Boolean *cont_to_dispatch)
	               	  		/* parent widget of Menu	*/
	         	     		/* Handler data: the menu id	*/
	        	       		/* Menu widget's XEvent		*/
	         	                 
{
static Position dummy;
MenuShellWidget menu = (MenuShellWidget) data;
OlVirtualName vn;

	if (w == (Widget) NULL)
	{
		OlWarning(dgettext(OlMsgsDomain,
				"ApplMenuPost: Null parent Widget"));
		return;
	}

					/* Release Pointer so others
					 * get events			*/

	if (menu == (MenuShellWidget) NULL)
	{
		OlWarning(dgettext(OlMsgsDomain,
				"ApplMenuPost: Null menu id"));
		return;
	}

	if ( ((vn = _OlInputEventVirtualName((Widget) menu, xevent)) == OL_MENU) || 
				vn  == OL_MENUKEY)
	{

		_OlUngrabPointer(w);

		if (menu->menu.shell_behavior == PinnedMenu)
		{
			XRaiseWindow(XtDisplay((Widget)menu),
						XtWindow((Widget)menu));
		}
		else
		{
			/* In the future, we have to add code here to
			 * resolve the emanate widget and the emanate_index
			 * parameters that we'll pass to OlMenuPopup.  This
			 * same problem exists in the routine PinPutIn.
			 */
			OlMenuPopup((Widget) menu, (Widget)NULL, 
				(Cardinal)OL_NO_ITEM, 
				(OlDefine)OL_PRESS_DRAG_MENU, 
				(Boolean)TRUE,
				(Position)xevent->xbutton.x_root,
				(Position)xevent->xbutton.y_root,
				(OlMenuPositionProc)PositionAppMenu);

			dummy = 0; /* fix for optimizer bug */
		}
	}
} /* END OF ApplMenuPost() */

/*
 *************************************************************************
 * EventHandler - This routine catches Substructure XEvents
 * and performs special operations, such as caching the
 * background pixmaps.
 * 
 * When the menu is mapped, this routine copies the two drop shadow
 * regions into pixmaps, so that they can be used later for redisplaying
 * the menu's backdrop region.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
EventHandler(register Widget widget, XtPointer data, XEvent *xevent, Boolean *cont_to_dispatch)
{
	MenuShellWidget	menu = (MenuShellWidget) widget;
	Pixmap *	dest;
	int		i;
	short		shadow_x;		/* Drop shadow offset	*/
	short		shadow_y;		/* Drop shadow offset	*/
	XRectangle	rect;

	if (menu->menu.shell_behavior == PinnedMenu ||
	    xevent->type != MapNotify)
	{
		return;
	}

			/* Set Focus with the current time since
			 * XMapEvents don't have a time field.		*/

	SetFocus(menu, CurrentTime);

			/* do two loops, one for the right edge and 
			 * one for the bottom edge			*/

	for(i = 0; i < 2; ++i) {

		if (i == 0) {				/* right edge	*/

			dest        = &(menu->menu.backdrop_right);
			shadow_x    = 0;
			shadow_y    = (short)menu->menu.shadow_bottom;
			rect.width  = (unsigned short)menu->menu.shadow_right;
			rect.height = (unsigned short)(widget->core.height - 
					menu->menu.shadow_bottom);
			rect.x      = (short)widget->core.width -
					(short) rect.width;
			rect.y      = 0;

				/* If no pixmaps, get them		*/

			if (*dest == (Pixmap) NULL)
				GetBackdropPixmaps(widget);

		}
		else {					/* Bottom edge	*/

			dest        = &(menu->menu.backdrop_bottom);
			shadow_x    = (short)menu->menu.shadow_right;
			shadow_y    = 0;
			rect.width  = (unsigned short)widget->core.width;
			rect.height = (unsigned short)menu->menu.shadow_bottom;
			rect.x      = 0;
			rect.y      = (short)widget->core.height -
						(short)rect.height;
		}

			/* First put drop shadow on background.		*/

		XFillRectangle(xevent->xany.display, xevent->xany.window,
			menu->menu.shadow_GC, 
			(int)(rect.x + shadow_x),
			(int)(rect.y + shadow_y),
			(unsigned int)(rect.width - (unsigned short)shadow_x),
			(unsigned int)(rect.height - (unsigned short)shadow_y));

			/* Copy background into Pixmap			*/

		XCopyArea(xevent->xany.display, xevent->xany.window,
			*dest, menu->menu.backdrop_GC,
			(int)rect.x, (int)rect.y,
			(unsigned int)rect.width, (unsigned int)rect.height,
			0, 0);
	} /* End of FORLOOP */
} /* END OF EventHandler() */

/*
 *************************************************************************
 * HandleKeyPress - handles keypress events for the menu
 ****************************procedure*header*****************************
 */
static void
HandleKeyPress (Widget w, OlVirtualEvent ve)
{
	Widget		cfw = (Widget)NULL;
	MenuShellWidget	menu = (MenuShellWidget)w;

				/* Always consume the event	*/

	ve->consumed = TRUE;

				/* If this menu is not the tail of the
				 * cascade, ingore the keypress since
				 * menu cascades are spring loaded and the
				 * tail menu will handle the keypress.
				 * We check to see if there's a root menu and
				 * if there is, we know there's an active
				 * menu cascade.			*/

	if  (menu->menu.root != (MenuShellWidget)NULL &&
	     menu->menu.next != (MenuShellWidget)NULL)
	{
		return;
	}

		/* Get the current focus widget within this menu.
		 * If this menu (which is the tail of the cascade)
		 * isn't the focus widget, then we will return.
		 * BUT, if the keypress event wasn't for any window
		 * within the cascade, we'll do an ActivateWidget
		 * on the menu.  This scenario ocurrs if the user
		 * managed to get focus to another window within
		 * the application, e.g., popping up the help
		 * window or moving to another application and
		 * back again.
		 */
	if ((cfw = OlGetCurrentFocusWidget(w)) == (Widget)NULL ||
	    cfw != (Widget)menu)
	{
		cfw = XtWindowToWidget(XtDisplay(w), ve->xevent->xany.window);

		if (cfw != (Widget)NULL &&
		    (cfw = _OlGetShellOfWidget(cfw)) != (Widget)NULL)
		{
			for(menu = menu->menu.root; ; menu = menu->menu.next)
			{
				/* If menu is NULL, the event was off the
				 * cascade, so do an ActivateWidget.
				 */
				if (menu == (MenuShellWidget)NULL)
				{
					OlActivateWidget(w, ve->virtual_name,
							(XtPointer)NULL);
					break;
				}
			}
		}
		return;
	}

	if (_OlFetchMnemonicOwner(w, (XtPointer *)NULL, ve) != (Widget)NULL ||
	    _OlFetchAcceleratorOwner(w, (XtPointer *)NULL, ve) != (Widget)NULL)
	{
			/* If we've matched an accelerator or a mnemonic,
			 * let the superclass handle it.		*/

		ve->consumed = False;
	}
	else
	{
		switch(ve->virtual_name) {
		case OL_PREVFIELD:
		case OL_NEXTFIELD:
		case OL_MOVERIGHT:
		case OL_MOVELEFT:
		case OL_MOVEUP:
		case OL_MOVEDOWN:
		case OL_MULTIRIGHT:
		case OL_MULTILEFT:
		case OL_MULTIUP:
		case OL_MULTIDOWN:
			OlMoveFocus(cfw, ve->virtual_name,
						ve->xevent->xkey.time);
			break;
		case OL_TOGGLEPUSHPIN:
		case OL_CANCEL:
		case OL_DEFAULTACTION:
			(void) OlActivateWidget(w, ve->virtual_name,
							(XtPointer)NULL);
			break;
		default:
			if (ve->virtual_name != OL_UNKNOWN_KEY_INPUT)
			{
				(void) OlActivateWidget(cfw, ve->virtual_name,
							(XtPointer)NULL);
			}
			break;
		}
	}
} /* END OF HandleKeyPress() */

/*
 *************************************************************************
 * FormExpose - this routine is responsible for drawing the line under
 * the menu title.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
FormExpose(register Widget widget, XtPointer data, XEvent *xevent, Boolean *cont_to_dispatch)
	               	       		/* The form widget		*/
	         	     		/* The menu widget		*/
	        	       		/* The XEvent 			*/
	         	                 
{
	MenuShellWidget menu = (MenuShellWidget) data;
	Widget		title = menu->menu.title_widget;
	int		x;
	int		y;

	if (menu->menu.shell_behavior == PinnedMenu)
		return;

	x = OlScreenPointToPixel(OL_HORIZONTAL, TITLE_PAD, XtScreen(widget));
	y = OlScreenPointToPixel(OL_VERTICAL, TITLE_PAD, XtScreen(widget));

	y = ((unsigned int) y >> 1) + y + (int)title->core.height;

	XDrawLine(XtDisplay(widget), XtWindow(widget), menu->menu.backdrop_GC,
		x, y, ((int)widget->core.width - x), y);

} /* END OF FormExpose() */

/*
 *************************************************************************
 * MenuButtonHandler -
 ****************************procedure*header*****************************
 */
static void
MenuButtonHandler (Widget w, OlVirtualEvent ve)
{
	MenuShellWidget menu = (MenuShellWidget)w;

			/* Prevent degenerate state or redundant
			 * processing.  Also, since the last menu in a
			 * cascade should process the events, return
			 * if we're not the last menu.			*/

	if (menu == (MenuShellWidget) NULL ||
	    menu->shell.popped_up == False ||
	    menu->menu.next != (MenuShellWidget)NULL)
	{
		return;
	}
	else if (ve->xevent->type == ButtonPress)
	{
		if (menu->menu.root != (MenuShellWidget)NULL &&
		    menu->menu.root->menu.shell_behavior == StayUpMenu)
		{
			SET_TIMER(menu, eDormant);
		}
		return;
	}

					/* Else... ButtonRelease	*/

	switch(ve->virtual_name) {
	case OL_SELECT:	/* FALLTHROUGH */
	case OL_MENU:
		switch(menu->menu.shell_behavior) {
		case PressDragReleaseMenu:

			/* if the distance traveled by the mouse 
			 * between the ButtonPress and the ButtonRelease
			 * is less than the damping factor, the menu
			 * is meant to behave as a "Stay-Up" menu; else,
			 * it behaves as a spring-loaded "Popup" menu.	*/

			if (menu->menu.prevent_stayup == False &&
			    _OlIsMenuMouseClick(menu,
						ve->xevent->xbutton.x_root,
						ve->xevent->xbutton.y_root))
			{
				SetStayUpMode(menu);
			}
			else
			{
				if(menu->menu.root != NULL)
					UnpostMenu(menu->menu.root, True, True, True);
			}
			break;

		case StayUpMenu:

			if(menu->menu.root != NULL)
				UnpostMenu(menu->menu.root, True, True, True);
			break;

		default:
			break;
		}
		break;
	default:
		if (menu->menu.shell_behavior != PinnedMenu &&
		    menu->menu.root != (MenuShellWidget)NULL)
		{
			UnpostMenu(menu->menu.root, True, True, True);
		}
		break;
	}
} /* END OF MenuButtonHandler */

/*
 *************************************************************************
 * PopdownTimerProc - routine called when a timer expires on a menu
 * cascade.  If the pointer is to the left of the last menu in the
 * cascade, the last menu pane is dismissed.  If the pointer is to the
 * right of the last menu's left edge, reset the timer.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
PopdownTimerProc (XtPointer data, XtIntervalId *id_ptr)
{
	MenuShellWidget	menu = (MenuShellWidget)data;
	MenuShellWidget	tail;
	Position	x;
	Position	y;
	TimerState	prior_state = timer_state;
	TimerState	new_state = eNone;

					/* Erase the old timer_state	*/
	timer_state = eNone;

					/* Find the tail of the cascade	*/
	for (tail = menu; tail->menu.next != (MenuShellWidget)NULL; )
	{
		menu = tail;
		tail = tail->menu.next;
	}

		/* If the tail menu is the root menu or if the tail menu
		 * is being destroyed, return.  Also, check for
		 * degenerate case (when the menu is not popped up).	*/

	if (menu == tail ||
	    tail->core.being_destroyed == True ||
	    tail->shell.popped_up == FALSE)
	{
		return;
	}

			/* Query the pointer relative to the RootWindow	*/

	QueryPointer((Widget)menu, &x, &y);

	if (prior_state == eActive)
	{
		if ((x + 2 + (Position)
		     (_OlGetAppAttributesRef((Widget)menu)->menu_mark_region))
		     < tail->core.x)
		{
			OlMenuPopdown((Widget)tail, FALSE);

				/* If 'menu' is not the root of the cascade,
				 * then it's ok to reset a timer, since 
				 * 'menu' is now the new tail.		*/

			if (menu != menu->menu.root)
			{
				new_state = (IN_MENU(menu, x, y) ?
						eActive : eDormant);
			}
		}
		else
		{
			new_state = (x < tail->core.x || IN_MENU(tail, x, y)
						? eActive : eDormant);
		}
	}
	else	/* prior_state == eDormant */
	{
		new_state = (IN_MENU(tail, x, y) ? eActive : eDormant);
	}

	if (new_state != eNone)
	{
		SET_TIMER(menu, new_state);
	}
} /* END OF PopdownTimerProc() */

/*
 *************************************************************************
 * PreSelect - this routine is called by menu Items that are selected
 *
 * This code contains a fix to remove the long standing problem of having
 * toolkit grabs active during the selected items callback.
 *
 * the Manager widget now has pre and post select callbacks that all
 * selectable primitives should call before and after calling their
 * select callbacks.
 *
 * the preselect callback will unpost the menu thereby clearing the
 * toollkit grab, and it turns any associated MenuButton busy for
 * the duration of the select callback.
 *
 * the postselect callback then restores any associated MenuButton
 * back to its normal state.
 *
 * Note that UnpostMenu required a new parameter to selectably revert
 * the MenuButton State.
 *
 * plus one that isnt used currently to maintain old semantic by  maintaining
 * a grab active while the select callback is being invoked!
 *
 * lpgc.
 ****************************procedure*header*****************************
 */
extern WidgetClass abbrevMenuButtonWidgetClass;

/* ARGSUSED */
static void
PreSelect (Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget		parent = (Widget)NULL,
			mb     = w,
			menuShell = NULL;
	Display		*dpy   = XtDisplay(mb);

	/*
	 * traverse back up the cascade setting all the parents
	 * buttons busy .... if they are not already
	 */

        /*
         * get the container of the item that selected.
         */
 
	if (call_data == (XtPointer)NULL ||
	    ((OlManagerSelectCallbackStruct*)call_data)->reason !=
		OL_REASON_PRE_SELECT)
		return;

        w = XtParent(((OlManagerSelectCallbackStruct*)call_data)->selectee);
 
	while (w != (Widget)NULL) {
		Widget	t = XtParent(w);

		if (XtIsSubclass(w, menuShellWidgetClass)) {

	    	/* save top-level not pinned menuShell */
			if(((ShellWidget)w)->shell.popped_up &&
				((MenuShellWidget)w)->menu.shell_behavior != PinnedMenu)
	    			menuShell = w;	

		    if ( ((MenuShellWidget)w)->menu.shell_behavior != 
			    PinnedMenu &&
			t != (Widget)NULL			  &&
			(XtIsSubclass(t, buttonWidgetClass)   ||
			 XtIsSubclass(t, buttonGadgetClass)   ||
			 XtIsSubclass(t, abbrevMenuButtonWidgetClass))) {
			    Arg	ba[1];
			    Boolean	button_busy = (Boolean)False;

			    parent = t;

			    XtSetArg(ba[0], XtNbusy, (XtArgVal)&button_busy);
			    XtGetValues(parent, ba, XtNumber(ba));

			    if (!button_busy) {
				XtSetArg(ba[0], XtNbusy, (XtArgVal)True);
				XtSetValues(parent, ba, XtNumber(ba));
			    }
		    }
 		}
		w = t;
	}

	/*
	 * wait for the button at the root of the menu to expose itself
	 */

	if (parent != (Widget)NULL) {
		XEvent	event;
		Boolean	done;
		Window	win = XtWindowOfObject(parent);

		/*
		 * setting the MenuButton busy causes SetValues to 
		 * request redisplay which clears the MenuButton
		 * window/area .... causing an expose to be generated
		 * by the server .... we need to process this to have
		 * the MenuButton redraw itself `busy'.
		 *
		 * however there is always a chance however slim that
		 * the menu button is somehow not exposed so we send
		 * ourselves a dummy event to guarantee unblocking
		 */

		event.xexpose.type    = Expose;
		event.xexpose.window  = win;
		event.xexpose.display = dpy;
		event.xexpose.x       = 0;
		event.xexpose.y       = 0;
		event.xexpose.width   = 0;
		event.xexpose.height  = 0;
		event.xexpose.count   = 0;

		XSendEvent(dpy, win, True, ExposureMask, &event);

		do {
			XMaskEvent(dpy, ExposureMask, &event);

			done = (event.xexpose.window  == win &&
				event.xexpose.count   == 0   &&
				event.xexpose.x == 0 &&
				event.xexpose.y == 0 &&
				event.xexpose.width == 0 &&
				event.xexpose.height == 0);

			if (!done)
				XtDispatchEvent(&event);
		} while (!done);
	}
	
	/*
	 * unpost the menu 
	 */

	if (RootMenu != (MenuShellWidget)NULL &&
	    XtIsSubclass(mb, menuButtonWidgetClass) == False &&
	    XtIsSubclass(mb, menuButtonGadgetClass) == False &&
	    (MenuShellWidget)menuShell == RootMenu)
	{
		UnpostMenu(RootMenu, True, False, True);
	}

	XFlush(dpy);	/* flush pending draws/umaps etc ... */
} /* END OF PreSelect() */

/* ARGSUSED */
static void
PostSelect (Widget w, XtPointer client_data, XtPointer call_data)
{
	MenuShellWidget	menu =(MenuShellWidget)NULL;

        /*
         * get the container of the item that selected.
         */
 
	if (call_data == (XtPointer)NULL ||
	    ((OlManagerSelectCallbackStruct*)call_data)->reason !=
		OL_REASON_POST_SELECT)
		return;

        w = XtParent(((OlManagerSelectCallbackStruct*)call_data)->selectee);

	/*
	 * find the menu shell
	 */

	while (w != (Widget)NULL) {
		Widget	t = XtParent(w);

		menu = (MenuShellWidget)w;

		if (XtIsSubclass(w, menuShellWidgetClass) &&
		    menu->menu.shell_behavior != PinnedMenu) {
			ClearMenuGrab(menu); /* clear the grab */

			if (menu->menu.revert_button != (OlRevertButtonProc)NULL)
					(*menu->menu.revert_button)(w, False);
			if(XtIsSubclass(t, buttonWidgetClass)   ||
				XtIsSubclass(t, buttonGadgetClass)   ||
	        		XtIsSubclass(t, abbrevMenuButtonWidgetClass)) {
				Arg	ba[1];
				Boolean	button_busy = 	TRUE;

				XtSetArg(ba[0], XtNbusy, (XtArgVal)&button_busy);
				XtGetValues(t, ba, XtNumber(ba));

				if (button_busy == TRUE) {
					XtSetArg(ba[0], XtNbusy, (XtArgVal)FALSE);
					XtSetValues(t, ba, XtNumber(ba));
				}
			}
		}
		w = t;
	}

}

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

void
OlCallbackPopdownMenu(Widget w, XtPointer client_data, XtPointer call_data)
{
	if(RootMenu != (MenuShellWidget)NULL)
		OlMenuPopdown((Widget)RootMenu, False);
}
		
/*
 *************************************************************************
 * _OlPopdownTrailingCascade - pops down a menu cascade that
 * is a descendent of this object's shell.
 ****************************procedure*header*****************************
 */
void
_OlPopdownTrailingCascade (Widget w, Boolean skip_first)
{
	MenuShellWidget	self;

	if (RootMenu == (MenuShellWidget)NULL			||
	    w == (Widget)NULL)
	{
		return;
	}

			/* Set w to be its shell
			 */

	w = _OlGetShellOfWidget(w);
	if (((ShellWidget)w)->shell.popped_up == False)
		return;

	for (self = RootMenu;
	     self != (MenuShellWidget)NULL;
	     self = self->menu.next)
	{
		if (w == (Widget)self)
		{
			/* MenuButtons typically use the skip_first
			 * flag since the next menu in the cascade
			 * belongs to it.
			 */

			self = self->menu.next;

			if (skip_first == True)
			{
				self = (self != (MenuShellWidget)NULL ?
					self->menu.next : NULL);
			}

			if (self != (MenuShellWidget)NULL)
			{
				OlMenuPopdown((Widget)self, False);
			}
			return;
		}
	}

		/* If we've reached here, the original object is
		 * not within the existing menu cascade.  So, popdown
		 * the whole cascade.
		 */

	OlMenuPopdown((Widget)RootMenu, False);

} /* END OF _OlPopdownTrailingCascade() */

/*
 *************************************************************************
 * OlMenuPost - this procedure allows an application to post a menu.
 * It is merely a convenience routine for OlMenuPopup.
 ****************************procedure*header*****************************
 */
void
OlMenuPost (Widget w)
{
static Position dummy;

	if (w == (Widget)NULL ||
	    XtIsSubclass(w, menuShellWidgetClass) == False)
	{
		OlWarning(dgettext(OlMsgsDomain,
				"OlMenuPost: null widget or widget not menu"));
		return;
	}
	OlMenuPopup(w, (Widget)NULL, OL_NO_ITEM, OL_PRESS_DRAG_MENU, False,
			(Position)0, (Position)0, (OlMenuPositionProc)NULL);

	dummy = 0; /* need this to fix optimizer bug */

} /* END OF OlMenuPost() */

/*
 *************************************************************************
 * OlMenuPopup -
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
void
OlMenuPopup (Widget w, Widget emanate, Cardinal emanate_index,
	     OlDefine state, Boolean setpos, Position x, Position y,
	     OlMenuPositionProc pos_proc)
{
	MenuShellWidget menu = (MenuShellWidget) w;
	PopupData	popup_data;
	OlVendorPartExtension part =
			_OlGetVendorPartExtension(_OlGetShellOfWidget(w));

	if (w == (Widget)NULL ||
	    XtIsSubclass(w, menuShellWidgetClass) == False)
	{
		OlWarning(dgettext(OlMsgsDomain,
				"OlMenuPopup: null widget or widget not menu"));
		return;
	}

	GetToken();
		if(part->pushpin == OL_NONE &&
			state == OL_PINNED_MENU) {
			OlWarning(dgettext(OlMsgsDomain,
			"OlMenuPopup: cannot pin MenuShell with XtNpushpin OL_NONE\n"));
		ReleaseToken();
		return;
	}

	if (pos_proc == (OlMenuPositionProc)NULL) {
		pos_proc = PositionAppMenu;
	}

	popup_data.proc			= pos_proc;
	popup_data.emanate		= emanate;
	popup_data.emanate_index	= emanate_index;

	if (state == OL_PINNED_MENU) {
		if (!setpos) {
 			Position x_ret, y_ret;
 
 			QueryPointer(w,&x_ret, &y_ret);
 			x = (int)x_ret;
 			y = (int)y_ret;
		}

		XtMoveWidget(w, x, y);

		PinPutIn(menu->menu.pushpin, (XtPointer)&popup_data,
				(XtPointer)NULL);
		ReleaseToken();
		return;
	}

	if (menu->shell.popped_up == False) {
		if (!setpos) {
			QueryPointer(w, &x, &y);
		}

		SetPressDragMode(menu, x, y, &popup_data);

		if (state == OL_STAYUP_MENU) {
			SetStayUpMode(menu);
		}
	}
	ReleaseToken();
} /* END OF OlMenuPopup() */

/*
 *************************************************************************
 * OlMenuPopdown - this routine pops down decorated menus.
 * non-pinned menus hanging off of this one, pop them down first.  This
 * sequence occurs recursively.
 ****************************procedure*header*****************************
 */
void
OlMenuPopdown (Widget widget, Boolean pinned_also)
{
	register MenuShellWidget	menu = (MenuShellWidget)widget;

	if (!XtIsSubclass((Widget) menu, menuShellWidgetClass)) {
		OlError(dgettext(OlMsgsDomain,
		"OlMenuPopdown requires a subclass of menushellWidgetClass"));
	}

	GetToken();
	if (menu->menu.shell_behavior == PinnedMenu)
	{
		if (pinned_also == TRUE) {
			DismissMenu(menu);
		}
	}
	else		/* transitory menu	*/
	{
		UnpostMenu(menu, True, True, True);
	}
	ReleaseToken();
} /* END OF OlMenuPopdown() */

/*
 *************************************************************************
 * OlMenuUnpost - this routine pops down non-pinned menus.  This routine
 * is obsolete and is retained for compatibility reasons only.
 ****************************procedure*header*****************************
 */
void
OlMenuUnpost (Widget w)
{
    if (!XtIsSubclass(w, menuShellWidgetClass)) {
	OlError(dgettext(OlMsgsDomain,
		"OlMenuUnpost requires a subclass of menushellWidgetClass"));
    }

    GetToken();
    UnpostMenu((MenuShellWidget)w, True, True, True);
    ReleaseToken();
} /* END OF OlMenuUnpost() */

