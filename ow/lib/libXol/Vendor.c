#pragma ident	"@(#)Vendor.c	302.40	97/03/26 lib/libXol SMI"	/* mouseless:Vendor.c 1.56	*/

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
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

/**************************************************************************
 *
 * Description:
 *	This file contains the routines necessary to manipulate the
 *	Vendor Shell class.  Rather than changing the class and instance
 *	parts of the Vendor shell and re-compiling the Intrinsics,
 *	we'll use the class extension mechanism.  At toolkit startup,
 *	the necessary class fields are loaded into the vendor shell
 *	class record.
 *
 **************************************************************************/


#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>
#include <widec.h>
#include <wctype.h>
#include <string.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>
#include <X11/Xatom.h>

#include <Xol/Converters.h>
#include <Xol/DrawAreaP.h>
#include <Xol/Error.h>
#include <Xol/EventObjP.h>
#include <Xol/ManagerP.h>
#include <Xol/OlClients.h>
#include <Xol/OlDnDVCXP.h>	/* for new drag and drop support */
#include <Xol/OlI18nP.h>
#include <Xol/OpenLookP.h>
#include <Xol/PrimitiveP.h>
#include <Xol/RootShell.h>	/* for multiple display support  */
#include <Xol/OlIm.h>
#include <Xol/Util.h>
#include <Xol/VendorI.h>
#include <Xol/FormP.h>
#include <Xol/StaticTexP.h>

#include <Xol/SuperCaret.h>

/* #ifdef _MOTIF_INTEROP */
/*
 *************************************************************************
 *
 *  Begin 10/93 Interop code:
 *  data structure PropMwmHints is defined in Xm/MwmUtil.h It is used to
 *  set Motif defined contstants in window manager property _MOTIF_WM_HINTS
 *   OLIT Decoration Atom               Motif Decoration constant
 *      _OL_DECOR_RESIZE                MWM_DECOR_RESIZEH
 *      _OL_DECOR_HEADER                MWM_DECOR_TITLE
 *      _OL_DECOR_CLOSE                 MWM_DECOR_MENU
 *
 **************************************************************************
 */

#include <X11/Xmd.h>

#define _XA_MOTIF_WM_HINTS      "_MOTIF_WM_HINTS"

/* bit definitions for MwmHints.flags */
#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)

/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)


typedef struct
{
    CARD32      flags;
    CARD32      functions;
    CARD32      decorations;
    INT32       inputMode;
    CARD32      status;
} PropMotifWmHints;

typedef PropMotifWmHints        PropMwmHints;

/* #endif  _MOTIF_INTEROP */


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

static void 
SetValuesWMShellTitle(Widget 			current, 
		      Widget 			request, 
		      Widget 			new,
		      OlVendorPartExtension   	cur_part,
		      OlVendorPartExtension   	new_part);
static void 
InitWMShellTitle(
		Widget request, 
		Widget new, 
		OlVendorPartExtension           part);
static Boolean	HasSunWMProtocols ( Widget );
static void	SetWindowState    ( Widget, OlVendorPartExtension);

static Boolean	CallExtPart (int, WidgetClass, Widget, Widget,
				Widget, ArgList, Cardinal *,
				XtPointer, XtPointer, XtPointer);
static OlVendorPartExtension GetInstanceData (Widget, int);
static void	ResortTraversalList(_OlArray array, Boolean *resort_list);
static void	SetupInputFocusProtocol(OlFocusData *fd);
static void	SetWMAttributes(Widget w, OlVendorPartExtension part, Boolean dobusy);
static char *	VendorGetBase (Widget, Boolean, _OlDynResourceList);
static XtCallbackList CreateVendorCBList(XtCallbackList in);

static void	_OlAddToShellList   ( Widget );
static void	_OlDelFromShellList ( Widget );

					/* class procedures		*/

static Boolean	AcceptFocus (Widget widget, Time *time);
static Boolean	ActivateWidget (Widget,OlVirtualName,XtPointer);
static void	ClassInitialize(void);
static void	ClassPartInitialize(WidgetClass wc);
static void	ChangeManaged(Widget w);
static void	Destroy(Widget w);
static void	ExtDestroy (Widget, XtPointer);
static Dimension ComputeOlFontHeight(OlVendorPartExtension part);
static void	Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);
static void InsertChild(Widget w);
static void Layout(Widget w);
static void CalculateSize(Widget w, Dimension *replyWidth, 
							Dimension *replyHeight);
static void CreateFooter(OlVendorPartExtension part, Widget parent);
static void	Realize(Widget w, XtValueMask *value_mask, XSetWindowAttributes *attributes);
static void	GetValues(Widget w, ArgList args, Cardinal *num_args);
static Boolean	SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);
static Widget	TraversalHandler (Widget, Widget, OlDefine, Time);
static void	WMMsgHandler (Widget, OlDefine, OlWMProtocolVerify *);

static void	Resize ( Widget );

static	XtGeometryResult GeometryManager ( Widget,
						    XtWidgetGeometry *,
						    XtWidgetGeometry *);

					/* action procedures		*/

static void	TakeFocusEventHandler(Widget w, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch);
static void	WMMessageHandler(register Widget widget, XtPointer data, XEvent *xevent, Boolean *cont_to_dispatch);


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

XrmQuark			OlXrmVendorClassExtension = NULLQUARK;
#define VENDOR_EXTENSION	"OlVendorClassExtension"

#define	CREATE	1
#define DESTROY	2

#define INIT_PROC	1
#define DESTROY_PROC	2
#define GET_PROC	3
#define SET_FUNC	4

#define NULL_EXTENSION		((OlVendorClassExtension)NULL)
#define NULL_PART_PTR		((OlVendorPartExtension*)NULL)
#define NULL_PART		((OlVendorPartExtension)NULL)
#define NULL_FOCUS_DATA		((OlFocusData *)NULL)
#define NULL_WIDGET		((Widget)NULL)

#define SHELL_LIST_STEP		16

#define CLASSNAME(w)	(XtClass((Widget)(w))->core_class.class_name)
#define VCLASS(wc, r)	(((VendorShellWidgetClass)(wc))->vendor_shell_class.r)

#define GET_EXT(wc)	(OlVendorClassExtension) _OlGetClassExtension( \
			    (OlClassExtension)VCLASS(wc, extension), \
			    OlXrmVendorClassExtension, \
			    OlVendorClassExtensionVersion)

#define CBLSIZE(cb)	((int)(cb->closure))

/*
 * This is used in SetWMAttributes, because it requires this info to know
 * whether to add or delete from the default decoration of certain window
 * type. It needs to add or delete certain decorations because Vendor has
 * allows more flexible decorations than what is allowed by the standard
 * window types.
 */
static struct WinTypeDecor {
	OlDefine	win_type;
	const char *    win_atom;
	Boolean 	window_header;
	Boolean 	menu_button;
	Boolean 	pushpin;
	Boolean 	resize_corners;
	OlDefine 	menu_type;
} wintype_decor[] = {
{ OL_WT_BASE,	_OL_WT_BASE_NAME,True,	True,	False,	True,	OL_MENU_FULL },
{ OL_WT_CMD,	_OL_WT_CMD_NAME, True,	False,	True,	True,	OL_MENU_LIMITED},
{ OL_WT_NOTICE,	_OL_WT_NOTICE_NAME,True,False,	False,	False,	OL_NONE },
{ OL_WT_HELP,	_OL_WT_HELP_NAME,  True,False,	True,	False,	OL_MENU_LIMITED },
{ OL_WT_OTHER,	_OL_WT_OTHER_NAME, False,False,	False,	False,	OL_NONE },
{ 0,		NULL,		  False,False,	False,	False,	OL_NONE },
};
#undef	AN

#define BYTE_OFFSET	XtOffsetOf(OlVendorPartExtensionRec, dyn_flags)
static _OlDynResource dyn_res[] = {
{ { XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultBackground }, BYTE_OFFSET,
	 OL_B_VENDOR_BG, VendorGetBase },
{ { XtNborderColor, XtCBorderColor, XtRPixel, sizeof(Pixel), 0,
	 XtRString, XtDefaultBackground }, BYTE_OFFSET,
	 OL_B_VENDOR_BORDERCOLOR, VendorGetBase },
};
#undef BYTE_OFFSET

extern void _OlNewAcceleratorResourceValues (Screen *, XtPointer);

/*
 * These are used to maintain a list of base window shells, so that
 * _OlDynResProc() can traverse to all the shells. The non-base window
 * shells are linked from one of the base window shells. These links are
 * maintained in the vendor shell extension.
 */
Widget *_OlShell_list = NULL;
Cardinal _OlShell_list_size = 0;
Cardinal _OlShell_list_alloc_size = 0;

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */
#define OFFSET(field)	XtOffsetOf(VendorShellRec, field)

static XtResource
resources[] = {
    { XtNborderColor, XtCBorderColor, XtRPixel, sizeof(Pixel),
	OFFSET(core.border_pixel), XtRImmediate,
	(XtPointer) XtDefaultBackground },
    { XtNallowShellResize, XtCAllowShellResize, XtRBoolean, sizeof(Boolean),
	OFFSET(shell.allow_shell_resize), XtRImmediate,
	(XtPointer) ((Boolean)True) },

    { XtNancestorSensitive, XtCSensitive, XtRBoolean, sizeof(Boolean),
	OFFSET(core.ancestor_sensitive), XtRImmediate,
	(XtPointer) ((Boolean)True) },

    { XtNinput, XtCInput, XtRLongBoolean, sizeof(Bool),
	OFFSET(wm.wm_hints.input), XtRImmediate, (XtPointer) ((Bool)FALSE) },
#ifdef	sun	/*OWV3*/
    { XtNvisual, XtCVisual, XtRVisual, sizeof(Visual*),
	OFFSET(shell.visual), XtRCallProc, (XtPointer) _OlCopyParentsVisual},
    { XtNcolormap, XtCColormap, XtRColormap, sizeof(Colormap),
	OFFSET(core.colormap), XtRCallProc, (XtPointer) _OlSetupColormap},
#endif	/*OWV3*/
};

#undef OFFSET
			/* This resource list is to initialize the
			 * extension data.				*/

#define OFFSET(field)	XtOffsetOf(OlVendorPartExtensionRec, field)

static XtResource
ext_resources[] = {

	/* NOTE: Whenever any widget is created, _XtGetResources 
	 *  is called in _XtCreate before CallInitialize (super-to-sub).
	 *  This means, if there is a subclass of Vendor shell, 
	 * for example PopupWindowShellWidgetClass, and if such a class defines
	 * a resource whose converter depends on some VendorShellWidgetClass
	 * resource, for exmaple, an XtROlStr type resource depending
	 * on a text_format resource, then such a scheme will essentially
	 * fail. This is because VendorShellWidget exists in an
	 * extesnion rec. And, extension rec resources do not get 
	 * initialized in a call to _XtGetResources. To get around this
	 * problem, each branch of subclasses extending down from
	 * Vendor Shell widget class must have a textformat resource 
	 * in their root of the subtree. For example, MenuShellWidgetClass 
	 * should have a textformat.
	 */

    {XtNtextFormat, XtCTextFormat, XtROlStrRep, sizeof(OlStrRep),
	OFFSET(text_format), XtRCallProc, (XtPointer)_OlGetDefaultTextFormat },

		/* Focus management resources	*/

    { XtNfocusWidget, XtCFocusWidget, XtRWidget, sizeof(Widget),
	OFFSET(focus_data.initial_focus_widget), XtRWidget, (XtPointer) NULL },

    { XtNfocusModel, XtCFocusModel, XtROlDefine, sizeof(OlDefine),
	OFFSET(focus_data.focus_model), XtRImmediate,
	 (XtPointer) OL_CLICK_TO_TYPE },

		/* Generic Vendor Shell resources	*/

    { XtNbusy, XtCBusy, XtRBoolean, sizeof(Boolean),
	OFFSET(busy), XtRImmediate, (XtPointer)False },

    { XtNmenuButton, XtCMenuButton, XtRBoolean, sizeof(Boolean),
	OFFSET(menu_button), XtRImmediate, (XtPointer)True },

    { XtNresizeCorners, XtCResizeCorners, XtRBoolean, sizeof(Boolean),
	OFFSET(resize_corners), XtRImmediate, (XtPointer)True },

    { XtNwindowHeader, XtCWindowHeader, XtRBoolean, sizeof(Boolean),
	OFFSET(window_header), XtRImmediate, (XtPointer)True },

    { XtNfooterPresent, XtCFooterPresent, XtRBoolean, sizeof(Boolean),
	OFFSET(footer_present), XtRImmediate, (XtPointer)False },

    { XtNimFooterPresent, XtCImFooterPresent, XtRBoolean, sizeof(Boolean),
	OFFSET(im_footer_present), XtRImmediate, (XtPointer)False },

    { XtNimRect, XtCImRect, XtRPointer, sizeof(XtPointer),
	OFFSET(im_rect), XtRImmediate, (XtPointer)NULL },

    { XtNleftFooterVisible, XtCLeftFooterVisible, XtRBoolean, sizeof(Boolean),
	OFFSET(left_footer_visible), XtRImmediate, (XtPointer)True },

    { XtNrightFooterVisible, XtCRightFooterVisible, XtRBoolean, sizeof(Boolean),
	OFFSET(right_footer_visible), XtRImmediate, (XtPointer)True },

    { XtNleftFooterString, XtCLeftFooterString, XtROlStr, sizeof(OlStr),
	OFFSET(left_footer_string), XtRImmediate, (XtPointer)NULL},

    { XtNrightFooterString, XtCRightFooterString, XtROlStr, sizeof(OlStr),
	OFFSET(right_footer_string), XtRImmediate, (XtPointer)NULL},

	{XtNimFontSet, XtCImFontSet, XtRFontSet, sizeof(XFontSet),
	OFFSET(im_font_set), XtRString, (XtPointer)XtDefaultFontSet},

	{XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	OFFSET(foreground), XtRString, (XtPointer)XtDefaultForeground},

	{XtNimStatusStyle, XtCImStatusStyle, XtROlImStatusStyle, 
	sizeof(OlImStatusStyle),
	OFFSET(im_status_style), XtRImmediate, (XtPointer)OL_NO_STATUS},

    {XtNshellTitle, XtCTitle, XtROlStr, sizeof(OlStr),
		OFFSET(title), XtRImmediate, (XtPointer)NULL},

    {XtNdefaultImName, XtCDefaultImName, XtRString, sizeof(String),
	OFFSET(default_im_name), XtRString, (XtPointer)NULL},

    { XtNpushpin, XtCPushpin, XtROlDefine, sizeof(OlDefine),
	OFFSET(pushpin), XtRImmediate, (XtPointer)OL_NONE },

    { XtNmenuType, XtCMenuType, XtROlDefine, sizeof(OlDefine),
	OFFSET(menu_type), XtRImmediate, (XtPointer)OL_MENU_FULL },

    { XtNwinType, XtCWinType, XtROlDefine, sizeof(OlDefine),
	OFFSET(win_type), XtRImmediate, (XtPointer)OL_WT_BASE },

    { XtNacceleratorsDoGrab, XtCAcceleratorsDoGrab, XtRBoolean, sizeof(Boolean),
	OFFSET(accelerators_do_grab), XtRImmediate, (XtPointer)False },

#ifdef NOT_USE
/*
   Will need this in Xt R5. But for now, vendor is maintaining its own list
   of callbacks.
*/
    { XtNconsumeEvent, XtCCallback, XtRCallback, sizeof(XtCallbackList),
	OFFSET(consume_event), XtRCallback, (XtPointer)NULL },
    { XtNwmProtocol, XtCWMProtocol, XtRCallback, sizeof(XtCallbackList),
	OFFSET(wm_protocol), XtRCallback, (XtPointer)NULL },
#endif

    { XtNuserData, XtCUserData, XtRPointer, sizeof(XtPointer),
	OFFSET(user_data), XtRPointer, (XtPointer) NULL },

    { XtNwmProtocolInterested, XtCWMProtocolInterested, XtRInt, sizeof(int),
	OFFSET(wm_protocol_mask), XtRImmediate,
	(XtPointer) (OL_WM_DELETE_WINDOW | OL_WM_TAKE_FOCUS) },

#ifdef	sun
    { XtNcomposeLED, XtCComposeLED, XtRBoolean, sizeof(Boolean),
	OFFSET(compose_led), XtRImmediate,
	(XtPointer) False },

    { XtNolWMRunning, XtCReadOnly, XtRBoolean, sizeof(Boolean),
	OFFSET(olwm_running), XtRImmediate,
	(XtPointer) False },
#endif
    { XtNsupercaret, XtCReadOnly, XtRWidget, sizeof(Widget),
        OFFSET(supercaret), XtRImmediate,
        (XtPointer) NULL },
};

/*
 *
 * Special resource list for transient shell.
 *
 */
static XtResource
transient_resources[] = {
					/* Focus management resources	*/

    { XtNfocusWidget, XtCFocusWidget, XtRWidget, sizeof(Widget),
	OFFSET(focus_data.initial_focus_widget), XtRWidget, (XtPointer) NULL },

				/* Generic Vendor Shell resources	*/

    { XtNbusy, XtCBusy, XtRBoolean, sizeof(Boolean),
	OFFSET(busy), XtRImmediate, (XtPointer)False },

    { XtNmenuButton, XtCMenuButton, XtRBoolean, sizeof(Boolean),
	OFFSET(menu_button), XtRImmediate, (XtPointer)False },

    { XtNresizeCorners, XtCResizeCorners, XtRBoolean, sizeof(Boolean),
	OFFSET(resize_corners), XtRImmediate, (XtPointer)False },

    { XtNwindowHeader, XtCWindowHeader, XtRBoolean, sizeof(Boolean),
	OFFSET(window_header), XtRImmediate, (XtPointer)True },

    { XtNpushpin, XtCPushpin, XtROlDefine, sizeof(OlDefine),
	OFFSET(pushpin), XtRImmediate, (XtPointer)OL_NONE },

    { XtNmenuType, XtCMenuType, XtROlDefine, sizeof(OlDefine),
	OFFSET(menu_type), XtRImmediate, (XtPointer)OL_MENU_LIMITED },

    { XtNwinType, XtCWinType, XtROlDefine, sizeof(OlDefine),
	OFFSET(win_type), XtRImmediate, (XtPointer)OL_WT_BASE },

    { XtNuserData, XtCUserData, XtRPointer, sizeof(XtPointer),
	OFFSET(user_data), XtRPointer, (XtPointer) NULL },
#ifdef	sun
    { XtNcomposeLED, XtCComposeLED, XtRBoolean, sizeof(Boolean),
	OFFSET(compose_led), XtRImmediate,
	(XtPointer) False },

    { XtNolWMRunning, XtCReadOnly, XtRBoolean, sizeof(Boolean),
	OFFSET(olwm_running), XtRImmediate,
	(XtPointer) False },
#endif
    { XtNsupercaret, XtCReadOnly, XtRWidget, sizeof(Widget),
        OFFSET(supercaret), XtRImmediate,
        (XtPointer) NULL },
};

#undef OFFSET

/* Define resource specs for Vendor resources that need to be reread
   after rewriting the resource data base */

#define WOFFSET(field)	XtOffsetOf(VendorShellRec, field)

static XtResource
work_space_widg_res[] = {
{ XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
      WOFFSET(core.background_pixel), XtRImmediate,
      (XtPointer) XtDefaultBackground },
{ XtNborderColor, XtCBorderColor, XtRPixel, sizeof(Pixel),
      WOFFSET(core.border_pixel), XtRImmediate,
      (XtPointer) XtDefaultBackground },
};

#undef WOFFSET

#define XOFFSET(field)	XtOffsetOf(OlVendorPartExtensionRec, field)

static XtResource
work_space_ext_res[] = {
{ XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
      XOFFSET(foreground), XtRString, (XtPointer)XtDefaultForeground},
};

#undef XOFFSET

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
		(XtPointer)&dnd_vendor_extension_rec,/* next_extension	*/
		NULLQUARK,			/* record_type		*/
		OlVendorClassExtensionVersion,	/* version		*/
		sizeof(OlVendorClassExtensionRec)/* record_size		*/
	},	/* End of OlClassExtension header */
	ext_resources,				/* resources		*/
	XtNumber(ext_resources),		/* num_resources	*/
	NULL,					/* private		*/
	NULL,					/* set_default		*/
	NULL,					/* get_default		*/
	ExtDestroy,				/* destroy		*/
	NULL,					/* initialize		*/
	NULL,					/* set_values		*/
	NULL,					/* get_values		*/
	TraversalHandler,			/* traversal_handler	*/
	NULL,					/* highlight_handler	*/
	ActivateWidget,				/* activate		*/
	NULL,	/* See ClassInitialize */	/* event_procs		*/
	0,	/* See ClassInitialize */	/* num_event_procs	*/
	NULL,					/* part_list		*/
	{ dyn_res, XtNumber(dyn_res) },		/* dyn_data		*/
	NULL,					/* transparent_proc	*/
	WMMsgHandler,				/* wm_proc		*/
	FALSE,					/* override_callback	*/
}, *vendor_extension = &vendor_extension_rec;

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

VendorShellClassRec
vendorShellClassRec = {
  {						/* core class		*/
	(WidgetClass) &wmShellClassRec,		/* superclass		*/
	"VendorShell",				/* class_name		*/
	sizeof(VendorShellRec),			/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	ClassPartInitialize,			/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	Initialize,				/* initialize		*/
	NULL,					/* initialize_hook	*/
	Realize,				/* realize		*/
	NULL, /* See ClassInitialize */		/* actions		*/
	NULL, /* See ClassInitialize */		/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	FALSE,					/* compress_motion	*/
	TRUE,					/* compress_exposure	*/
	FALSE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	Destroy,				/* destroy		*/
	Resize,					/* resize		*/
	XtInheritExpose,			/* expose		*/
	SetValues,				/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	GetValues,				/* get_values_hook	*/
	AcceptFocus,				/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_private	*/
	NULL, /* See ClassInitialize */		/* tm_table		*/
	XtInheritQueryGeometry,			/* query_geometry	*/
  	NULL,					/* display_accelerator  */
    	NULL					/* extension		*/
  },
  {						/* composite class	*/
	GeometryManager,			/* geometry_manager	*/
	ChangeManaged,			/* change_managed	*/
	InsertChild,			/* insert_child		*/
	XtInheritDeleteChild,			/* delete_child		*/
	NULL					/* extension         	*/
  },
  {						/* shell class		*/
	NULL					/* extension		*/
  },
  {						/* WMShell class	*/
	NULL					/* extension		*/
  },
  {						/* VendorShell class	*/
	(XtPointer)&vendor_extension_rec	/* extension		*/
  }
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass vendorShellWidgetClass = (WidgetClass) &vendorShellClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/****************************procedure*header*****************************
 * CallExtPart - this procedure forwardly-chains calls to the instance
 * extension part class procedures of the vendor shell.
 */
static Boolean
CallExtPart (int proc_type, WidgetClass wc, Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args, XtPointer cur_part, XtPointer req_part, XtPointer new_part)
{
	Boolean			ret_val = False;
	OlVendorClassExtension	ext = GET_EXT(wc);

	/* chain destroy sub to super ..... */

	if (ext != (OlVendorClassExtension)NULL &&
	    proc_type == DESTROY_PROC		&&
	    ext->destroy != (OlExtDestroyProc)NULL)
	{
	    (*ext->destroy)(current, cur_part);
	}

	if (wc != vendorShellWidgetClass)
	{
	    if (CallExtPart(proc_type, wc->core_class.superclass,
			current, request, new,
			args, num_args,
			cur_part, req_part, new_part) == TRUE)
	    {
		ret_val = TRUE;
	    }
	}

	if (ext != NULL_EXTENSION) {
	    switch (proc_type)
	    {
	    case INIT_PROC :
		if (ext->initialize != (OlExtInitializeProc)NULL)
		{
		    (*ext->initialize)(request, new,
				args, num_args, req_part, new_part);
		}
		break;
	
	    case DESTROY_PROC :
		/* dead already! */
		break;

	    case SET_FUNC :
		if (ext->set_values != (OlExtSetValuesFunc)NULL)
		{
			if ((*ext->set_values)(current, request, new,
				args, num_args, cur_part, req_part, new_part)
			     == TRUE)
			{
				ret_val = TRUE;
			}
		}
		break;

	    case GET_PROC :
		if (ext->get_values != (OlExtGetValuesProc)NULL)
		{
		    (*ext->get_values)(current, args, num_args, cur_part);
		}
		break;

	    } /* end of switch(proc_type) */
	}

	return (ret_val);
} /* END OF CallInitProc() */

/*
 *************************************************************************
 * GetInstanceData - this routine creates or destroys a vendor's instance
 * extension record (which hangs off the vendor class extension record).
 * The action field determines what to do.  The routine returns the
 * address of the newly created extension or NULL when destroying an
 * extension.
 ****************************procedure*header*****************************
 */
static OlVendorPartExtension
GetInstanceData (Widget w, int action)
{
	OlVendorPartExtension	part = NULL_PART;
	OlVendorClassExtension	extension = GET_EXT(XtClass(w));

					/* Set the widget to be itself	*/
	w = w->core.self;

	if (extension != NULL_EXTENSION)
	{

		if (action == DESTROY)
		{
			OlVendorPartExtension *	part_ptr =
						&extension->part_list;

			for (part = *part_ptr;
			     part != NULL_PART && w != part->vendor;)
			{
				part_ptr	= &part->next;
				part		= *part_ptr;
			}

			*part_ptr = part->next;
			XtFree((char *)part);
			part = NULL_PART;
		}
		else /* CREATE */
		{
			part = (OlVendorPartExtension)XtCalloc(1,
					sizeof(OlVendorPartExtensionRec));
			part->vendor		= w->core.self;
			part->next		= extension->part_list;
			part->class_extension	= extension;
			extension->part_list	= part;
		}
	}
	else
	{
					/* We should never get here	*/
	    OlWarning(dgettext(OlMsgsDomain,
		"Could not find class extension for a Vendor subclass"));
	}
	return(part);
} /* END OF GetInstanceData() */


/****************************procedure*header*****************************
 * ResortTraversalList -
 ****************************procedure*header*****************************/
static void
ResortTraversalList(array, resort_list)
	_OlArray	array;
	Boolean *	resort_list;
{
	static Widget  *list = NULL;
	static Widget  *ref_list = NULL;
	static int      list_slots_left = 0;
	static int      list_alloced = 0;

	int             i,
	                how_many,
	                pos;
	Arg             arg[1];
	String          ref_name;

		/* it's a no-op if this flag is off */
	if (*resort_list == False)
		return;

	*resort_list = False;
	how_many = 0;

	if (array != NULL && array->num_elements != 0)
	{
		XtSetArg(arg[0], XtNreferenceName, (XtArgVal)&ref_name);
		list_slots_left = list_alloced;

		for (i = 0; i < array->num_elements; i++)
		{
			ref_name = NULL; /* give ref_name "default" value */
			XtGetValues((Widget)array->array[i], arg, 1);
			if (ref_name != NULL)
			{
				pos = _OlArrayFindByName(array, ref_name);

					/* free the string */
				_OlFreeRefName((Widget)array->array[i]);

				if (pos == _OL_NULL_ARRAY_INDEX)
				{
	    				OlWarning(dgettext(OlMsgsDomain,
						"Traversal: reference name \
not found in traversal list"));
					continue;
				}
				if (list_slots_left == 0)
				{
					int	more_slots;

					more_slots = (list_alloced / 2) + 2;
					list_alloced += more_slots;
					list_slots_left += more_slots;
					list = (Widget *)XtRealloc(
							(char*)list,
							list_alloced *
							sizeof (Widget));
					ref_list = (Widget *)XtRealloc(
							(char*)ref_list,
							list_alloced *
							sizeof (Widget));
				}
				ref_list[how_many]=(Widget)array->array[pos];
				list[how_many++]=(Widget)array->array[i];
				list_slots_left--;
			}
		}
		for (i = 0; i < how_many; i++)
		{
			_OlUpdateTraversalWidget(
				list[i], NULL, ref_list[i], False);
		}
	}
} /* END OF ResortTraversalList */

/****************************procedure*header*****************************
 * SetupInputFocusProtocol - Sets WM_TAKE_FOCUS protocol on `w' and adds
 * an event handler to process the WM_TAKE_FOCUS client messages.
 */
static void
SetupInputFocusProtocol(OlFocusData *fd)
{
	Widget	shell = fd->shell;		/* VendorShell Widget	*/

		/* set WM protocol (TAKE_FOCUS property) on toplevel	*/
	_OlSetWMProtocol(XtDisplay(shell), XtWindow(shell), WM_TAKE_FOCUS);

	/* add event handler to catch (TAKE_FOCUS) Client Messages	*/

	XtAddEventHandler(shell, (EventMask)NoEventMask, TRUE,
		     TakeFocusEventHandler, (XtPointer)fd);

} /* END OF SetupInputFocusProtocol() */

/* #ifdef _MOTIF_INTEROP */

static void SetMwmAttributes( Display			*dpy, 
			      Window			win, 
			      OlVendorPartExtension	part )
{
        Atom            mwm_hint_prop;
        PropMwmHints    mwm_hints;

        mwm_hints.flags = MWM_HINTS_DECORATIONS | MWM_HINTS_FUNCTIONS;

        mwm_hints.decorations = MWM_DECOR_BORDER | MWM_DECOR_RESIZEH | MWM_DECOR_TITLE |
                                MWM_DECOR_MENU | MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE;

        mwm_hints.functions = MWM_FUNC_RESIZE | MWM_FUNC_MOVE | MWM_FUNC_MINIMIZE |
                              MWM_FUNC_MAXIMIZE | MWM_FUNC_CLOSE;

        if (part->menu_type == OL_NONE) {
                mwm_hints.functions = 0;
        }

        mwm_hint_prop = OlInternAtom ( dpy, _XA_MOTIF_WM_HINTS );

        /*  NOTE: Motif menu button is not same as OLIT close menu button.
         *  Motif has a minimize button for iconifying. This button should be absent from
         *  all the popups.
         *  Motif menubutton is used to display menus. It should be absent only
in
         *  Notices.
         */

        /* default setting according to DRAFT of 3/20/89 on
	 * Open Look Client Interface.
	 */
        switch (part->win_type) {
            case OL_WT_BASE:
                break;

            case OL_WT_CMD: {
                    /* transient shells can not be iconified */
                    mwm_hints.decorations &= ~MWM_DECOR_MINIMIZE;
                    mwm_hints.decorations &= ~MWM_DECOR_MAXIMIZE;
                    mwm_hints.functions &= ~MWM_FUNC_MINIMIZE;
                    mwm_hints.functions &= ~MWM_FUNC_MAXIMIZE;
                    break;
                }

            case OL_WT_NOTICE: {
                    /* transient shells can not be iconified */
                    mwm_hints.decorations &= ~MWM_DECOR_MINIMIZE;
                    mwm_hints.decorations &= ~MWM_DECOR_MAXIMIZE;

                    /* Notice does not have resize corners */
                    mwm_hints.decorations &= ~MWM_DECOR_RESIZEH;

		    /* Notice does not have header */
		    mwm_hints.decorations &= ~MWM_DECOR_TITLE;

                    /* Notice does not have menu */
                    mwm_hints.decorations &= ~MWM_DECOR_MENU;
                    mwm_hints.functions = 0;
                    break;
                }

            case OL_WT_HELP: {
                    /* transient shells can not be iconified */
                    mwm_hints.decorations &= ~MWM_DECOR_MINIMIZE;
                    mwm_hints.decorations &= ~MWM_DECOR_MAXIMIZE;
                    mwm_hints.functions &= ~MWM_FUNC_MINIMIZE;
                    mwm_hints.functions &= ~MWM_FUNC_MAXIMIZE;

                    /* Help does not have resize corners */
                    mwm_hints.decorations &= ~MWM_DECOR_RESIZEH;
                    mwm_hints.functions &= ~MWM_FUNC_RESIZE;


                    break;
                }

            case OL_WT_OTHER: {
                    mwm_hints.decorations = 0;
                    mwm_hints.functions = 0;
                    break;
                }

            default: {
                    mwm_hints.decorations = 0;
                    mwm_hints.functions = 0;
                    break;
                }

        }

        if (part->resize_corners == False) {
                mwm_hints.decorations &= ~MWM_DECOR_RESIZEH;
                mwm_hints.functions &= ~MWM_FUNC_RESIZE;
        }
	else {
                mwm_hints.decorations |= MWM_DECOR_RESIZEH;
                mwm_hints.functions |= MWM_FUNC_RESIZE;
        }

        if (part->menu_button == False) {
		/* if there is no menu button,
		 * window does not want to be iconified.
		 */
                mwm_hints.decorations &= ~MWM_DECOR_MINIMIZE;
                mwm_hints.decorations &= ~MWM_DECOR_MAXIMIZE;
        }
	else {
                mwm_hints.decorations |= MWM_DECOR_MINIMIZE;
                mwm_hints.decorations |= MWM_DECOR_MAXIMIZE;
        }

        if (part->window_header == False) {
		/* olwm treats that as both the title and menu button shoud
		 * not be displayed.
		 */
                mwm_hints.decorations &= ~MWM_DECOR_TITLE;
		/* mwm puts title decoration if other decorations are present */
                mwm_hints.decorations &= ~MWM_DECOR_MENU;
                mwm_hints.decorations &= ~MWM_DECOR_MINIMIZE;
                mwm_hints.decorations &= ~MWM_DECOR_MAXIMIZE;
        }
	else {
		mwm_hints.decorations |= MWM_DECOR_TITLE;
	}

        XChangeProperty ( dpy, win, mwm_hint_prop, mwm_hint_prop, 32,
                          PropModeReplace, &mwm_hints, 5 );
} /* End of SetMwmAttributes */

/* #endif  _MOTIF_INTEROP */

/*
 *************************************************************************
 * SetWMAttributes -
 ****************************procedure*header*****************************
 */
static void
SetWMAttributes(Widget w, OlVendorPartExtension part, Boolean dobusy)
{
	OLWinAttr wa;
#ifdef	sun
	NewOLWinAttr	lwa;
	int		numtowrite;
	unsigned char 	*ptr;
	Boolean		use_short_prop;
	unsigned long	flags =0L;
#endif
	struct WinTypeDecor *decor;
	Atom del_list[4];
	Atom add_list[4];
	int delcount = 0;
	int addcount = 0;
	Display *dpy= XtDisplay(w);
	Window   win= XtWindow(w);
	Atom	atom;

/* #ifdef _MOTIF_INTEROP */
	SetMwmAttributes ( dpy, win, part );
/* #endif _MOTIF_INTEROP */

	/* set up window attribute structure */

#ifndef	sun
	wa.flags = _OL_WA_WIN_TYPE;
	if (part->pushpin != OL_NONE)
		wa.flags |= _OL_WA_PIN_STATE;
#else
	if (!(use_short_prop = _OlUseShortOLWinAttr(dpy))) {
		lwa.flags = flags = _OL_WA_WIN_TYPE;
		if (part->pushpin != OL_NONE)
			lwa.flags |= _OL_WA_PIN_STATE;
		numtowrite = ULongsInNewOLWinAttr;
		ptr = (unsigned char *)&lwa;
	} else {
		numtowrite = ULongsInOLWinAttr;
		ptr = (unsigned char *)&wa;
	}
#endif
	atom = OlInternAtom(dpy, _OL_WT_OTHER_NAME);
#ifndef	sun
	wa.win_type = atom; /* default */
	wa.pin_state = (part->pushpin == OL_IN) ? 1 : 0;
#else
	if (use_short_prop) {
		wa.win_type = atom; /* default */
		wa.pin_state = (part->pushpin == OL_IN) ? 1 : 0;
	} else {
		lwa.win_type = atom; /* default */
		lwa.pin_state = (part->pushpin == OL_IN) ? 1 : 0;
		flags |= _OL_WA_PIN_STATE;
	}
#endif
	for (decor=wintype_decor; decor->win_type != NULL; decor++) {
		if (part->win_type == decor->win_type) {
			/* found it! */
#ifndef	sun
			wa.win_type = OlInternAtom(dpy,
						   decor->win_atom);
#else
			long		menu_type;
			long		cancel = 0;

			if (use_short_prop)
				wa.win_type = OlInternAtom(dpy,
							   decor->win_atom);
			else
				lwa.win_type = OlInternAtom(dpy,
							    decor->win_atom);
#endif
			if (part->window_header != decor->window_header) {
				atom = OlInternAtom(dpy, _OL_DECOR_HEADER_NAME);
				if (decor->window_header == True)
					del_list[delcount++] = atom;
				else
					add_list[addcount++] = atom;
			}
			if (part->menu_button != decor->menu_button) {
				atom = OlInternAtom(dpy, _OL_DECOR_CLOSE_NAME);
				if (decor->menu_button == True)
					del_list[delcount++] = atom;
				else
					add_list[addcount++] = atom;
			}
			if (part->resize_corners != decor->resize_corners) {
				atom = OlInternAtom(dpy, _OL_DECOR_RESIZE_NAME);
				if (decor->resize_corners == True)
					del_list[delcount++] = atom;
				else
					add_list[addcount++] = atom;
			}
			if (((part->pushpin != OL_NONE) != 
				(decor->pushpin != False)) && 
				((part->win_type != OL_WT_BASE))) {
				atom = OlInternAtom(dpy, _OL_DECOR_PIN_NAME);
				if (decor->pushpin != False)
					del_list[delcount++] = atom;
				else
					add_list[addcount++] = atom;
			}
#ifndef	sun
			if (part->menu_type != decor->menu_type) {
				wa.flags |= _OL_WA_MENU_TYPE;
#else
			if (use_short_prop || part->menu_type != decor->menu_type) {
				flags |= _OL_WA_MENU_TYPE;
#endif
				switch(part->menu_type) {
				case OL_MENU_FULL:
#ifndef	sun
					wa.menu_type = OlInternAtom(dpy,
								    _OL_MENU_FULL_NAME);
#else
					menu_type = OlInternAtom(dpy,
                                                                 _OL_MENU_FULL_NAME);
#endif
					break;
				case OL_MENU_LIMITED:
#ifndef	sun
					wa.menu_type = OlInternAtom(dpy,
                                                                    _OL_MENU_LIMITED_NAME);
					wa.flags |= _OL_WA_CANCEL;
					wa.cancel = 0;
#else
					menu_type = OlInternAtom(dpy,
							         _OL_MENU_LIMITED_NAME);
					flags |= _OL_WA_CANCEL;
					cancel = 0;
#endif
					break;
				case OL_MENU_CANCEL:
#ifndef	sun
					wa.menu_type = OlInternAtom(dpy,
								    _OL_MENU_LIMITED_NAME);
					wa.flags |= _OL_WA_CANCEL;
					wa.cancel = 1;
#else
					menu_type = OlInternAtom(dpy,
								 _OL_MENU_LIMITED_NAME);
					flags |= _OL_WA_CANCEL;
					cancel = 1;
#endif
					break;
				case OL_NONE:
#ifndef	sun
					wa.flags &= ~_OL_WA_MENU_TYPE;
					wa.menu_type = OlInternAtom(dpy,
								    _OL_NONE_NAME);
#else
					menu_type = OlInternAtom(dpy,
							         _OL_NONE_NAME);
#endif
					break;
				}
			}
#ifdef	sun
			if (use_short_prop) {
				wa.menu_type = menu_type;
			} else {
				lwa.flags = flags;
				lwa.menu_type = menu_type;
				if (flags & _OL_WA_CANCEL)
					lwa.cancel = cancel;
			}
#endif
			break;
		} /* if */
	} /* for */

	atom = OlInternAtom(dpy, _OL_WIN_ATTR_NAME);
	XChangeProperty(dpy, win, atom,
#ifndef	sun
		atom, 32, PropModeReplace, (unsigned char *) &wa, 5);
#else
		atom, 32, PropModeReplace, ptr, numtowrite);
#endif

	atom = OlInternAtom(dpy, _OL_DECOR_DEL_NAME);
	if (delcount)
		XChangeProperty(dpy, win, atom,
#ifndef	sun
			atom, 32, PropModeReplace,
#else
			XA_ATOM, 32, PropModeReplace,
#endif
		 	(unsigned char *) del_list, delcount);

	atom = OlInternAtom(dpy, _OL_DECOR_ADD_NAME);
	if (addcount)
		XChangeProperty(dpy, win, atom,
#ifndef	sun
			atom, 32, PropModeReplace,
#else
			XA_ATOM, 32, PropModeReplace,
#endif
		 	(unsigned char *) add_list, addcount);

	if (dobusy)
		SetWMWindowBusy(dpy, win, (part->busy == True ?
				WMWindowIsBusy : WMWindowNotBusy));
}

/* ARGSUSED */
static char *
VendorGetBase (Widget w, Boolean init, _OlDynResourceList res)
{
	OlVendorPartExtension part;

	/*
	 * Normally, you would need to check the init flag and may
	 * need to allocate the extension. But here, vendor is the one
	 * calling this function. Thus the extension must have been
	 * allocated previously.
	 */
	/*
	if (init == TRUE) {
		part = GetInstanceData(w, CREATE);
	}
	else
	*/
		part = _OlGetVendorPartExtension(w);

	return((char *)part);
}

static XtCallbackList
CreateVendorCBList(XtCallbackList in)
{
	XtCallbackList cb;
	int i = 0;

	if (in)
		for (cb=in; cb->callback; i++, cb++) ;

	if (i && (cb = (XtCallbackList)XtMalloc(sizeof(XtCallbackRec) * ++i))) {
		(void)memcpy((char *)cb, (char *)in, sizeof(XtCallbackRec) * i);
		return(cb);
	}
	return(NULL);
}

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 * AcceptFocus - If this widget can accept focus then it is set here
 *		 FALSE is returned if focus could not be set
 ****************************procedure*header*****************************
 */
static Boolean
AcceptFocus (Widget w, Time *time)
{
	if (OlCanAcceptFocus(w, *time)) {
		OlFocusData    *fd = _OlGetFocusData(w, NULL);
		_OlArray        list = &(fd->traversal_list);
		Widget          the_default;

		/*
		 * Try to move focus to the widget that was set to receive it.
		 * If that fails, try the next one.
		 */
		if (fd->initial_focus_widget != NULL_WIDGET &&
			fd->initial_focus_widget != w &&
			OlCallAcceptFocus(fd->initial_focus_widget, 
				*time) == TRUE) {
			return (True);
		}
		
		if ((the_default = _OlGetDefault(w)) != NULL_WIDGET &&
			the_default != w &&
			OlCallAcceptFocus(the_default, *time) == TRUE) {
			return (True);
		}
		
		if (list->num_elements > (Cardinal)0 &&
			TraversalHandler(w, (Widget)list->array[0], 
				OL_IMMEDIATE, *time) != NULL_WIDGET) {
			return (True);
		}

		/*
		 * No initial_focus_widget, no default widget and no descendent
		 * wants focus, so set it to the shell.
		 */
		(void) OlSetInputFocus(w, RevertToNone, *time);

		return (True);
	}

	return (False);
} /* END OF AcceptFocus() */


/****************************procedure*header*****************************
 * ActivateWidget - this procedure is a default activate proc for vendor
 *			this routine allows the help window worked properly
 *			on OL_CANCEL.
 */
/* ARGSUSED */
static Boolean
ActivateWidget (Widget w, OlVirtualName type, XtPointer call_data)
{
	Boolean			consumed = False;
	OlVendorPartExtension part = _OlGetVendorPartExtension(w);

	switch (type)
	{
		case OL_CANCEL:
			consumed = True;
				/* means it's not base/applicWindow */
			if (XtIsSubclass(w, topLevelShellWidgetClass) == False)
			{
				XtPopdown(w);
			}
			else if (part->pushpin != OL_NONE)
			{
				XtUnmapWidget(w);
			}
			break;
		case OL_DEFAULTACTION:
			consumed = True;
			if ((w 	= _OlGetDefault(w)) != (Widget)NULL) {
				(void) OlActivateWidget(w,OL_SELECTKEY,
						(XtPointer)NULL);
			}
			break;
		case OL_TOGGLEPUSHPIN:
			{
				if (part->pushpin != OL_NONE) {
					consumed = True;
					if (part->pushpin == OL_OUT)
						_OlSetPinState(w, OL_IN);
					else {
						_OlSetPinState(w, OL_OUT);
						if (XtIsSubclass(w,
						    topLevelShellWidgetClass)
						    == False)
						  XtPopdown(w);
						else
						  XtUnmapWidget(w);
					}
				}
			}
			break;
		default:
			break;
	}

	return (consumed);
} /* END OF ActivateWidget() */

/*
 *************************************************************************
 * ClassInitialize -
 ****************************procedure*header*****************************
 */
static void
ClassInitialize(void)
{
	VendorShellWidgetClass vwc =
				(VendorShellWidgetClass)vendorShellWidgetClass;
	Cardinal size = sizeof(XtResource) * vendor_extension->num_resources;

	/* multiple display support - must happen first! */

	XtInitializeWidgetClass(displayShellWidgetClass);
	XtInitializeWidgetClass(screenShellWidgetClass);
	XtInitializeWidgetClass(superCaretShellWidgetClass);

			/* Quarkify our record_type name so that other
			 * subclasses can use it.			*/

	OlXrmVendorClassExtension = XrmStringToQuark(VENDOR_EXTENSION);

	vendor_extension->header.record_type = OlXrmVendorClassExtension;

			/* Make a copy of the uncompiled resource
			 * list and stick it in the private slot.  This
			 * way, we can guarantee that we always have
			 * an uncompiled list around.			*/

	vendor_extension->c_private = (XtResourceList) XtMalloc(size);
	
	(void)memcpy((char *)vendor_extension->c_private,
			(const char *)vendor_extension->resources, (int)size);

	 vwc->core_class.actions	= (XtActionList)_OlGenericActionTable;
	 vwc->core_class.num_actions	= (Cardinal)_OlGenericActionTableSize;
	 vwc->core_class.tm_table	= (String)_OlGenericTranslationTable;
	 vendor_extension->event_procs	= (OlEventHandlerList)
						_OlGenericEventHandlerList;
	 vendor_extension->num_event_procs = (Cardinal)
						_OlGenericEventHandlerListSize;

	_OlAddOlDefineType ("click_to_type", OL_CLICK_TO_TYPE);
	_OlAddOlDefineType ("click-to-type", OL_CLICK_TO_TYPE);
	_OlAddOlDefineType ("real_estate",   OL_REALESTATE);
	_OlAddOlDefineType ("real-estate",   OL_REALESTATE);
	_OlAddOlDefineType ("none",          OL_NONE);
	_OlAddOlDefineType ("out",           OL_OUT);
	_OlAddOlDefineType ("in",            OL_IN);
	_OlAddOlDefineType ("full",          OL_MENU_FULL);
	_OlAddOlDefineType ("limited",       OL_MENU_LIMITED);
	_OlAddOlDefineType ("cancel",        OL_MENU_CANCEL);
	_OlAddOlDefineType ("base",          OL_WT_BASE);
	_OlAddOlDefineType ("cmd",           OL_WT_CMD);
	_OlAddOlDefineType ("command",       OL_WT_CMD);
	_OlAddOlDefineType ("notice",        OL_WT_NOTICE);
	_OlAddOlDefineType ("help",          OL_WT_HELP);
	_OlAddOlDefineType ("other",         OL_WT_OTHER);

	_OlDnDDoExtensionClassInit(&dnd_vendor_extension_rec);
			/* initialize the dnd class extension */
} /* END OF ClassInitialize() */

/*
 *************************************************************************
 * ClassPartInitialize - this routine initializes the vendor shell
 * extension.  It's main responsibility is to incorporate the default
 * resources for subclasses.
 ****************************procedure*header*****************************
 */
static void
ClassPartInitialize(WidgetClass wc)
{
	OlVendorClassExtension	super_ext;
	OlVendorClassExtension	ext;
	Cardinal		size;
	Boolean			examine_resources = True;
#ifdef SHARELIB
   void **__libXol__XtInherit = _libXol__XtInherit;
#undef _XtInherit
#define _XtInherit      (*__libXol__XtInherit)
#endif

#ifdef	sun	/*OWV3*/
	_OlFixResourceList(wc);
#endif

	_OlDnDDoExtensionClassPartInit(wc);	/* initialize the dnd class
						 * part extension
						 */

	/* If this widget class is the VendorShellWidgetClass, return. */
	if (wc == vendorShellWidgetClass) {
		return;
	}

			/* Get the superclass's vendor extension.  We
			 * don't have to check to see if we found the
			 * superclass extension, since we always create
			 * one when the superclass came through this
			 * routine earlier.				*/

	super_ext	= GET_EXT(wc->core_class.superclass);
	ext		= GET_EXT(wc);

	if (ext == NULL_EXTENSION)
	{
			/* this class has no extension so create one
			 * for it using the superclass as the template	*/

		size	= sizeof(OlVendorClassExtensionRec);
		ext	= (OlVendorClassExtension) XtMalloc(size);

		(void)memcpy((char *)ext, (const char *)super_ext, (int)size);

		ext->header.next_extension	= VCLASS(wc, extension);
		VCLASS(wc, extension)		= (XtPointer)ext;

		/* do not copy chained extension methods */

		ext->initialize = (OlExtInitializeProc)NULL;
		ext->destroy    = (OlExtDestroyProc)NULL;
		ext->set_values = (OlExtSetValuesFunc)NULL;
		ext->get_values = (OlExtGetValuesProc)NULL;

			/* if transient shell, put in special resource list,
			 * else set a flag not to examine the resources
			 * since they were copied from the superclass.	*/

		if (wc == transientShellWidgetClass)
		{
			ext->resources		= transient_resources;
			ext->num_resources	= XtNumber(transient_resources);
			ext->c_private		= (XtResourceList)NULL;
		}
		else
		{
			examine_resources = False;
		}
	}

			/* Now fill in the private resources list	*/

	if (ext->num_resources == (Cardinal)0)
	{
				/* give the super class's resource
				 * lists to this class.			*/

		ext->resources		= super_ext->resources;
		ext->num_resources	= super_ext->num_resources;
		ext->c_private		= super_ext->c_private;
	}
	else if (examine_resources == True)
	{
		Cardinal	i;
		Cardinal	j;
		XtResourceList	rsc;

		size = sizeof(XtResource) * super_ext->num_resources;

		rsc = (XtResourceList) XtMalloc(size);
	
			/* Loop over the resources copying superclass
			 * resources that are not specified in this
			 * class.					*/

		for (i=0; i < super_ext->num_resources; ++i)
		{
			for (j=0; j < ext->num_resources; ++j)
			{
				if (!strcmp(ext->resources[j].resource_name,
				    super_ext->c_private[i].resource_name))
				{
					/* Take this subclass's resource
					 * structure			*/

					rsc[i] = ext->resources[j];
					break;
				}
			}

				/* If j equals the number of resources
				 * in this subclass, then the subclass
				 * did not override the superclass
				 * value so, use the superclass's	*/

			if (j == ext->num_resources) {
				rsc[i] = super_ext->c_private[i];
			}
		}

				/* cache the results in this extension	*/

		ext->resources		= rsc;
		ext->num_resources	= super_ext->num_resources;

				/* Now make a copy of the uncompiled
				 * resource list in this extension	*/
				
		ext->c_private = (XtResourceList) XtMalloc(size);

		(void)memcpy((char *)ext->c_private,
				(const char *)ext->resources, (int)size);
	}

			/* Always initialize the part_list field	*/

	ext->part_list = NULL_PART;

				/* Inherit the SetDefault Procedure	*/

	if (ext->set_default == XtInheritSetDefault) {
		ext->set_default = super_ext->set_default;
	}

				/* Inherit the Traversal Procedure	*/

	if (ext->traversal_handler == XtInheritTraversalHandler) {
		ext->traversal_handler = super_ext->traversal_handler;
	}

				/* Inherit the highlight Procedure	*/

	if (ext->highlight_handler == XtInheritHighlightHandler) {
		ext->highlight_handler = super_ext->highlight_handler;
	}

				/* Inherit the wm_msg Procedure		*/

	if (ext->wm_proc == XtInheritWMProtocolProc) {
		ext->wm_proc = super_ext->wm_proc;
	}

				/* Inherit the transparent Procedure	*/

	if (ext->transparent_proc == XtInheritTransparentProc) {
		ext->transparent_proc = super_ext->transparent_proc;
	}

	if (ext->dyn_data.num_resources == 0) {
		/* give the superclass's resource list to this class */
		ext->dyn_data = super_ext->dyn_data;
	}
	else {
		/* merge the two lists */
		_OlMergeDynResources(&(ext->dyn_data), &(super_ext->dyn_data));
	}

} /* END OF ClassPartInitialize() */

/*
 *************************************************************************
 * Destroy - the new vendor shell class destroy procedure.
 ****************************procedure*header*****************************
 */
static void
Destroy(Widget w)
	      	  		/* vendor widget being destroyed	*/
{
	OlVendorPartExtension	part_ext;
	OlFocusData *		fd = _OlGetFocusData(w, &part_ext);

				/* Remove the Help Event handler	*/

	XtRemoveEventHandler(w, (EventMask)NoEventMask, True,
		(XtEventHandler) _OlPopupHelpTree, (XtPointer) NULL);

				/* Remove the TakeFocus Handler		*/
	XtRemoveEventHandler(w, (EventMask)NoEventMask, TRUE,
		     TakeFocusEventHandler, (XtPointer)fd);

				/* Remove the WMMessage Handler		*/
	XtRemoveEventHandler(w, (EventMask)PropertyChangeMask, TRUE,
		     WMMessageHandler, (XtPointer)NULL);

	/* Destroy its im info rec in the display shell */
	_OlDestroyImVSInfo(w);

	_OlDestroyKeyboardHooks(w);

	_OlDelFromShellList(w);

	/* Call the class-extension-part destroy procedure. */

	(void) CallExtPart(DESTROY_PROC, XtClass(w), 
			w,			/* current widget */
			NULL, NULL, NULL, 0,
			(XtPointer)part_ext,	/* current ext part */
			(XtPointer)NULL, (XtPointer)NULL);

	/* call the dnd class extension destroy method */

	CallDnDVCXExtensionMethods(CallDnDVCXDestroy, XtClass(w), w, 
				   NULL_WIDGET, NULL_WIDGET, (ArgList)NULL,
				   (Cardinal *)NULL);

				/* Destroy the extension record		*/

	(void)GetInstanceData(w, DESTROY);

#ifdef	sun	/*OWV3*/
	_OlDelWMColormapWindows(w);
#endif
#if	0
	OlDestroyScreenShell(w, False);	/* maybe */
	OlDestroyDisplayShell(w,False);	/* ditto */
#endif
} /* END OF Destroy() */

/****************************procedure*header*****************************
 * ExtDestroy - Class Extension Destroy procedure
 */
/* ARGSUSED */
static void
ExtDestroy (Widget w, XtPointer part_ext)
{
    OlVendorPartExtension part = (OlVendorPartExtension)part_ext;

    if (part == NULL)
	return;

    if (part->focus_data.traversal_list.array != NULL) {
	XtFree((char*)part->focus_data.traversal_list.array);
	part->focus_data.traversal_list.array = NULL;
    }

    if (part->consume_event)
	XtFree((char*)part->consume_event);

    if (part->wm_protocol)
	XtFree((char*)part->wm_protocol);

    if (part->im_rect)
	XtFree((XtPointer) part->im_rect);
    if (part->left_footer_string != (OlStr) NULL)
	XtFree((XtPointer) part->left_footer_string);
    if (part->right_footer_string != (OlStr) NULL)
	XtFree((XtPointer) part->right_footer_string);
    if (part->title != (OlStr) NULL)
	XtFree((XtPointer) part->title);
}

/*
 *************************************************************************
 *
 * Resize()
 *
 ****************************procedure*header*****************************
 */

static void
Resize (Widget widget)
{
	WidgetClass	super = vendorShellWidgetClass->core_class.superclass;

	_OlDnDSetDisableDSClipping(widget, True);

	Layout(widget);	

	_OlDnDSetDisableDSClipping(widget, False);

	OlDnDWidgetConfiguredInHier(widget);
}

/*
 *************************************************************************
 *
 * GeometryManager()
 *
 ****************************procedure*header*****************************
 */

static XtGeometryResult 
GeometryManager( widget, request, reply )
Widget widget;
XtWidgetGeometry *request;
XtWidgetGeometry *reply;
{
VendorShellWidget vendor = (VendorShellWidget)(widget->core.parent);
OlVendorPartExtension	part = _OlGetVendorPartExtension(widget->core.parent);
XtWidgetGeometry my_request;
XtGeometryResult	result;
Dimension new_height;
int i;
Widget kid = (Widget) NULL;
Widget footer = part && part->footer_present &&
        vendor->composite.num_children > 1 ?
        vendor->composite.children[vendor->composite.num_children - 1] :
        (Widget) NULL;

        for (i = 0; i < vendor->composite.num_children; i++) {
                if (XtIsManaged(vendor->composite.children[i])) {
                        kid = vendor->composite.children[i];
                        break;
                }
        }

	/*
	 * if widget != applications composite (i.e its your footer)
	 * then allow only height changes and set the width to be same
	 * as the shell. If the request Width is not the same as that
	 * of the Shell's then return XtGeometryAlmost - thereby forcing
	 * the footer to be the same width as the shell...
	 */
	
	if (vendor->composite.num_children > 1){
		if ( widget != kid) {
			if (request->request_mode & (CWX | CWY | CWBorderWidth))
				return (XtGeometryNo);

			if ((request->request_mode & CWWidth) || 
					(request->request_mode & CWHeight)) {

				Dimension shell_width, shell_height;
				XtGeometryResult ftr_geom_result;

				if (request->request_mode & CWHeight)
					widget->core.height = request->height;

				if (request->width == vendor->core.width)
					ftr_geom_result = XtGeometryYes; 
				else {
					widget->core.width = vendor->core.width;
					reply->width = vendor->core.width;
					reply->height = request->height;
					reply->request_mode = request->request_mode;
					ftr_geom_result = XtGeometryAlmost; 
				}
				CalculateSize((Widget)vendor, &shell_width, &shell_height);
				if (part->from_setvalues) {
					vendor->core.width = shell_width;
					vendor->core.height = shell_height;
					return (ftr_geom_result);
				}
				my_request.request_mode = 0;
				if (shell_width != vendor->core.width || 
							shell_height != vendor->core.height){
					my_request.request_mode |= CWWidth;
					my_request.width = shell_width;
					my_request.request_mode |= CWHeight;
					my_request.height = shell_height;
					result = XtMakeGeometryRequest((Widget)vendor, 
										&my_request, NULL);
					if ((result == XtGeometryYes) || 
										(result == XtGeometryDone))
						return (ftr_geom_result);
					else
						return (XtGeometryNo);
				}
				else if (shell_width == vendor->core.width || 
								shell_height == vendor->core.height)
					return (ftr_geom_result);
				else 
					return (XtGeometryNo);
			}
		}
	}

	if(vendor->shell.allow_shell_resize == FALSE && XtIsRealized(widget))
		return (XtGeometryNo);

	_OlDnDSetDisableDSClipping(widget, True);

	if(!XtIsRealized((Widget)vendor)){
		if (request->request_mode & (CWX | CWY)) {
			return (XtGeometryNo);
		}
	}

        new_height = request->height;
        if (footer && XtIsManaged(footer))
                new_height += _OlWidgetHeight(footer);
        if (part && part->im_footer_present)
                new_height += ComputeOlFontHeight(part);

	if(!XtIsRealized((Widget)vendor)){
	/* 
	 * add height and width of footer and IMStatus into 
	 * vendor geometry
	 */

		*reply = *request;
		if(request->request_mode & CWWidth)
		   widget->core.width = vendor->core.width = request->width;
		if(request->request_mode & CWHeight) 
		   widget->core.height = vendor->core.height = new_height;
		if(request->request_mode & CWBorderWidth)
		   widget->core.border_width = vendor->core.border_width =
		   	request->border_width;
		result = XtGeometryYes;
	}
	else {
		/* %%% worry about XtCWQueryOnly */
		my_request.request_mode = 0;
		if (request->request_mode & CWWidth) {
			/* also here ... */
			my_request.width = request->width;
			my_request.request_mode |= CWWidth;
		}
		if (request->request_mode & CWHeight) {
			/* also here ... */
			my_request.height = new_height;
			my_request.request_mode |= CWHeight;
		}
		if (request->request_mode & CWBorderWidth) {
			my_request.border_width = request->border_width;
			my_request.request_mode |= CWBorderWidth;
		}

		/*
		**	Ensure that there are no outstanding events
		**	before making the Geometry Request.
		**	The problem is that RootGeometryManager
		**	in Shell.c will issue a ConfigureRequest
		**	and wait for a ConfigureNotify event.
		**	It will discard older ConfigureNotify events
		**	which may be expected elsewhere,
		**	leading the tool to believe that the
		**	Window Manager is not responding.
		**
		**	Bug 4023131 - fixed by DPT 22-Jan-97
		*/

		XSync(XtDisplay((Widget)vendor), False);

		result = XtMakeGeometryRequest((Widget)vendor, &my_request, NULL);
		if ((result == XtGeometryYes) || (result == XtGeometryDone)) {
			if (request->request_mode & CWWidth) {
				widget->core.width = request->width;
			}
			/* subtract footer & IM status height */
			if (request->request_mode & CWHeight) {
                                widget->core.height = new_height;
                                if (footer && XtIsManaged(footer))
                                        widget->core.height -=
                                                _OlWidgetHeight(footer);
                                if (part && part->im_footer_present)
                                        widget->core.height -=
                                                ComputeOlFontHeight(part);
			}
			if (request->request_mode & CWBorderWidth) {
				widget->core.x = widget->core.y = -request->border_width;
			}
			result = XtGeometryYes;
		} 
		else 
			result = XtGeometryNo;
	}

	_OlDnDSetDisableDSClipping(widget, False);

	if (result == XtGeometryYes || result == XtGeometryDone)
		OlDnDWidgetConfiguredInHier(widget);
	
        if(result == XtGeometryYes)  {
                OlWidgetConfigured(widget);

                /* Now update the geometry for IM Footer Area if necessary */

                if (part && part->im_footer_present && kid)  {
                        part->im_rect->x = 0;
                        if (part->im_rect->y != _OlWidgetHeight(kid) ||
                                part->im_rect->width != vendor->core.width ||
                                part->im_rect->height != ComputeOlFontHeight(part)) {

                                Arg args[1];
                                part->im_rect->y = _OlWidgetHeight(kid);
                                part->im_rect->width = vendor->core.width;
                                part->im_rect->height = ComputeOlFontHeight(part);
                                XtSetArg(args[0], XNArea, (XtPointer)part->im_rect);
                                OlSetIMStatusOnAllICs((Widget) vendor, args, 1);
                        }
                }

	}
	return (result);
}

/*
 *************************************************************************
 * Realize - Sets up the input focus protocol with the window manager if
 * there is a focus widget.
 * This can only be done at realize time, since we need a window to put
 * the take focus property on.
 ****************************procedure*header*****************************
 */
static void
Realize(Widget w, XtValueMask *value_mask, XSetWindowAttributes *attributes)
{
	XtRealizeProc		super_realize;
	OlVendorPartExtension	part_ext;
	OlFocusData *		fd = _OlGetFocusData(w, &part_ext);


			/* Use the superclass to create the window	*/

	super_realize = vendorShellWidgetClass->core_class.
				superclass->core_class.realize;

	if (super_realize != (XtRealizeProc)NULL) {
		(*super_realize)(w, value_mask, attributes);
		if (part_ext->footer_present)
			CreateFooter(part_ext, w);
	} else {
		OlError(dgettext(OlMsgsDomain,
			"VendorShell's superclass has no Realize Proc."));
	}

	if (fd != NULL_FOCUS_DATA) {
		SetupInputFocusProtocol(fd);
	}

	/* always interested in these WM messages */
	if (_OL_WM_TESTBIT(part_ext->wm_protocol_mask,
			OL_WM_DELETE_WINDOW))
		_OlSetWMProtocol(XtDisplay(w), XtWindow(w),
			OlInternAtom(XtDisplay(w), WM_DELETE_WINDOW_NAME));
	if (_OL_WM_TESTBIT(part_ext->wm_protocol_mask,
					OL_WM_SAVE_YOURSELF))
		_OlSetWMProtocol(XtDisplay(w), XtWindow(w),
			OlInternAtom(XtDisplay(w), WM_SAVE_YOURSELF_NAME));

	SetWMAttributes(w, part_ext, (part_ext->busy) ? True : False);

	/* do some post-realisation setup for the dnd class extension */

	_OlDnDCallVCXPostRealizeSetup(w);

#ifdef	sun	/*OWV3*/
	_OlAddWMColormapWindows(w);
	SetWindowState(w, part_ext);
#endif

	/* Maybe realize the first IC if none realized yet */
	OlRealizeFirstIC(w);
} /* END OF Realize() */

static void
copy_olstring(OlStrRep tf, OlStr * foot_str)
{
    if(tf != OL_WC_STR_REP)
	*foot_str = (*foot_str != (OlStr) NULL) ?
	    XtNewString((XtPointer)(*foot_str)) : (OlStr) NULL;
    else {
	wchar_t *ws;
	if (*foot_str != (OlStr) NULL) {
	    ws = (wchar_t *)XtMalloc((wslen((wchar_t *)*foot_str)
				      +1)*sizeof(wchar_t));
	    wscpy(ws, (wchar_t *) *foot_str); 
	    *foot_str = (OlStr)ws;
	}
    }
}

/*
 *************************************************************************
 * Initialize - this routine is the new vendor shell class initialize
 * procedure.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
Initialize (Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	OlVendorPartExtensionRec	req_part;
	OlVendorPartExtension		part = GetInstanceData(new, CREATE);
	OlFocusData *			fd = &(part->focus_data);
	MaskArg				mask_args[4];
	XtCallbackList			cb = NULL;
	XtCallbackList			wm_cb = NULL;
	VendorShellWidget		vsw = (VendorShellWidget)new;

	OlCreateDisplayShell(new, args, *num_args);	/* maybe */
	OlCreateScreenShell(new, args, *num_args);	/* ditto */

			/* Make sure this shell does not redirect
			 * keypress events				*/

	XtSetKeyboardFocus(new, NULL_WIDGET);

			/* Add an event handler to trap the help client
			 * messages from the window manager.		*/

	XtAddEventHandler(new, (EventMask)NoEventMask, True,
				_OlPopupHelpTree, (XtPointer) NULL);

			/* Add an event handler to catch client messages
			 * from the window manager concerning
			 * WM protocol.					*/
	XtAddEventHandler(new, (EventMask)PropertyChangeMask,
			True, WMMessageHandler, (XtPointer) NULL);


			/* Now set the subvalue data.  We'll use the 
			 * resource database to do this.
			 * NOTE: that I've hacked the XtGetSubresources
			 * call to use a NULL name and class string.
			 * Hopefully, the Instrinsics will publicly
			 * allow this.					*/

	XtGetSubresources(new, (XtPointer)part, (String)NULL, (String)NULL,
			part->class_extension->resources,
			part->class_extension->num_resources, args, *num_args);


			/* Check XtNconsumeEvent */
	_OlSetMaskArg(mask_args[0], XtNconsumeEvent, &cb, OL_COPY_SOURCE_VALUE);
	_OlSetMaskArg(mask_args[1], NULL, sizeof(XtCallbackList), OL_COPY_SIZE);
	_OlSetMaskArg(mask_args[2], XtNwmProtocol, &wm_cb, OL_COPY_SOURCE_VALUE);
	_OlSetMaskArg(mask_args[3], NULL, sizeof(XtCallbackList), OL_COPY_SIZE);
	_OlComposeArgList(args, *num_args, mask_args, 4, NULL, NULL);
	part->consume_event = CreateVendorCBList(cb);
	part->wm_protocol   = CreateVendorCBList(wm_cb);
	
	if(_OlInputFocusFeedback(new) == OL_SUPERCARET)
		part->supercaret = _OlCreateSuperCaret(new, args, *num_args);

			/* Set the request part equal to the new part	*/

	req_part = *part;

			/* Initialize the OlFocusData structure.	*/

	fd->shell			= new;
	fd->focus_gadget		= NULL_WIDGET;
	fd->activate_on_focus		= NULL_WIDGET;
	fd->current_focus_widget	= NULL_WIDGET;
	fd->resort_list			= False;
	fd->traversal_list.array	= _OL_NULL_ARRAY;
	fd->traversal_list.num_elements	= 0;
	fd->traversal_list.num_slots	= 0;

	part->default_widget		= NULL_WIDGET;
	part->accelerator_list		= 0;
	part->from_setvalues        = False;
	part->im_rect               = (XRectangle *)
					XtCalloc(1, sizeof(XRectangle));
	copy_olstring(part->text_format, &(part->left_footer_string));
	copy_olstring(part->text_format, &(part->right_footer_string));

	/* 
	 * Keep the XtNshellTitle resource value in sync with
	 * the XtNtitle resource. XtNtitle is defined in WMShell.
	 */
	InitWMShellTitle(request, new, part);
			 
	/* must always express interests on these message types */
	part->wm_protocol_mask	       |= OL_WM_DELETE_WINDOW | OL_WM_TAKE_FOCUS;

		/* add shell to traversal list, since shell doesn't have */
		/* reference_name and reference_widget resources, so     */
		/* use NULLs instead                                     */
	_OlUpdateTraversalWidget(new, NULL, NULL_WIDGET, True);


	/* Add this shell to the list for tracking
	   dynamic changes to accelerators
	 */
	_OlAddToShellList(new);

	/* Call the class extension part initialize procedure. */

	(void) CallExtPart(INIT_PROC, XtClass(new), NULL_WIDGET,
			request, new, args, num_args, (XtPointer)NULL,
			(XtPointer)&req_part, (XtPointer)part);

#ifdef	sun
	part->olwm_running = HasSunWMProtocols(new);
#endif

	_OlInitDynResources(new, &(part->class_extension->dyn_data));
	_OlCheckDynResources(new, &(part->class_extension->dyn_data),
			 args, *num_args);

	/* now initialize the dnd class ext part */

	CallDnDVCXExtensionMethods(CallDnDVCXInitialize, XtClass(new),
				   NULL_WIDGET, request, new, args, num_args);

	/* initialize the VS Info List in the DisplayShell */
	_OlCreateImVSInfo(new);
} /* END OF Initialize() */

/*
 *************************************************************************
 * ComputeOlFontHeight - Figures out the height of the i18N status area...
 * This requires a valid vendor extension part...
 ****************************procedure*header*****************************
 */
static Dimension 
ComputeOlFontHeight(OlVendorPartExtension part)
{
	return (Dimension)((XExtentsOfFontSet(
			(XFontSet)(part->im_font_set)))->max_logical_extent.height);
}

/*
 *************************************************************************
 * _OlGetVendorTextFormat - Retrieves the text_format from the ext record.
 ****************************procedure*header*****************************
 */
extern void 
_OlGetVendorTextFormat(Widget w, Cardinal *size, XrmValue *ret_val)
{
	OlVendorPartExtension	part = _OlGetVendorPartExtension(w);
	char *msg;

	if(part == NULL) {
		if (msg = malloc(256)) {
			snprintf(msg, 256, "Vendor extension Part missing for: %s widget\n",
									XtName(w)); 
			OlError(dgettext(OlMsgsDomain,msg));
			free(msg);
		}
	}

	if (ret_val != (XrmValue *) NULL) {
		ret_val->size = sizeof(OlStrRep);
		ret_val->addr = (XtPointer)&(part->text_format);
	}
}

/*
 *************************************************************************
 * CreateFooter - this routine is used to create the necessary Application
 * footer area in the shell.
 ****************************procedure*header*****************************
 */
static void
CreateFooter(OlVendorPartExtension part, Widget parent)
{

	part->footer = XtVaCreateWidget("footer",
			formWidgetClass, parent, 
			XtNheight, 1,
			XtNwidth, 1,
			XtNborderWidth, 0,
			XtNbackground, parent->core.background_pixel,
			NULL);

	part->left_footer = XtVaCreateManagedWidget("left_footer",
			staticTextWidgetClass, part->footer, 
			XtNborderWidth, 0, 
			XtNstring, part->left_footer_string,
			XtNtextFormat, part->text_format,
			XtNgravity, WestGravity,
			XtNforeground, part->foreground,
			XtNbackground, parent->core.background_pixel,
			XtNmappedWhenManaged, part->left_footer_visible,
			NULL);

	part->right_footer = XtVaCreateManagedWidget( "right_footer", 
			staticTextWidgetClass, part->footer, 
			XtNborderWidth, 0, 
			XtNstring, part->right_footer_string,
			XtNtextFormat, part->text_format,
			XtNgravity, EastGravity,
			XtNxAddWidth, True,
			XtNxRefWidget, part->left_footer,
			XtNxAttachRight, True,
			XtNxVaryOffset, True,
			XtNforeground, part->foreground,
			XtNbackground, parent->core.background_pixel,
			XtNmappedWhenManaged, part->right_footer_visible,
			NULL);
	XtManageChild(part->footer);
}

/*
 *************************************************************************
 * CalculateSize - this routine is used to calculate the necessary height
 * and width taking into consideration application and I18N footer and 
 * status areas...
 ****************************procedure*header*****************************
 */
static void
CalculateSize(Widget w, Dimension *replyWidth, Dimension *replyHeight)
{
CompositeWidget cw = (CompositeWidget)w;
OlVendorPartExtension	part = _OlGetVendorPartExtension(w);
Widget kid, footer;
int i;


	kid = footer = (Widget) NULL;
	for (i = 0; i < cw->composite.num_children; i++) {
		if (XtIsManaged(cw->composite.children[i])) {
			kid = cw->composite.children[i];
			break;
		}
	}

        /* check if kid null or kid is footer */
        if (kid == (Widget) NULL ||
                (part && part->footer_present && part->footer == kid))  {
		*replyHeight = w->core.height;
		*replyWidth = w->core.width;
		return;
	}

	*replyWidth = _OlWidgetWidth(kid);
        *replyHeight = _OlWidgetHeight(kid);

        footer = cw->composite.children[cw->composite.num_children - 1];
        if (part && part->footer_present && footer && XtIsManaged(footer))
                *replyHeight += _OlWidgetHeight(footer);
        if (part && part->im_footer_present)
                *replyHeight += ComputeOlFontHeight(part);

}

/*
 *************************************************************************
 * Layout - this routine is used to layout the shell's composite and the 
 * hidden footers(app's and I18N). It also updates the geometry of the 
 * I18N status area.
 ****************************procedure*header*****************************
 */
static void
Layout(Widget w)
{
CompositeWidget cw = (CompositeWidget)w;
OlVendorPartExtension	part = _OlGetVendorPartExtension(w);
int kid_height, ftr_height, im_ftr_height;
Widget kid, footer;
int i;

	kid = footer = (Widget)NULL;
	for (i = 0; i < cw->composite.num_children; i++) {
		if (XtIsManaged(((CompositeWidget)w)->composite.children[i])) {
			kid = cw->composite.children[i];
			break;
		}
	}

        if (kid == (Widget)NULL ||
                (part && part->footer_present && part->footer == kid))
                return;         /* return if no children or just the footer */

	ftr_height = im_ftr_height = 0;

	if (part && part->footer_present) {
		footer = cw->composite.children[cw->composite.num_children - 1];
		if (footer != (Widget)NULL && XtIsManaged(footer))
			ftr_height = _OlWidgetHeight(footer);
	}

	if (part && part->im_footer_present)
		im_ftr_height = ComputeOlFontHeight(part);

	kid_height = w->core.height - 2*kid->core.border_width -
					ftr_height - im_ftr_height;

	if (footer != (Widget)NULL 	&& 
		XtIsManaged(footer))	{	
		int y = w->core.height - _OlWidgetHeight(footer);
		int width = w->core.width - 2*footer->core.border_width;

		if(y > 0 && width > 0)
			XtConfigureWidget( footer, 0, (Position)y, 
						(Dimension)width, 
						footer->core.height,
						footer->core.border_width);
	}

	/* Now check to see if we can fit everything else resize the shell's 
		first child - application's container */


	if ((Dimension) (ftr_height + im_ftr_height + kid_height) > 
				(Dimension) w->core.height) {
		kid_height = w->core.height - ftr_height - im_ftr_height -
						2*kid->core.border_width;
	}

	/* Now configure the kid */
	{
	int kid_width = w->core.width - 2*kid->core.border_width;
	if(kid_height > 0 && kid_width > 0)
		XtConfigureWidget(kid, 0, 0, 
				(Dimension)kid_width,
				(Dimension)kid_height, 
				kid->core.border_width);
	}

	/* Now update the geometry for IM Footer Area if necessary */

	if (part && part->im_footer_present) {
		part->im_rect->x = 0;
		if (part->im_rect->y != _OlWidgetHeight(kid) || 
				part->im_rect->width != w->core.width ||
				part->im_rect->height != ComputeOlFontHeight(part)) {

			Arg args[1];

			part->im_rect->y = _OlWidgetHeight(kid);
			part->im_rect->width = w->core.width;
			part->im_rect->height = ComputeOlFontHeight(part);

			XtSetArg(args[0], XNArea, (XtPointer)part->im_rect);
			OlSetIMStatusOnAllICs(w, args, 1);
		}
	}
}

/*
 *************************************************************************
 * ChangeManaged - this routine first calculated the size of the shell
 * and makes a request for resize...
 * If we get called from setvalues then we just update the 
 * necessary fields about a geometry change and let the Intrinsics 
 * do the necessary geometry negotiations.
 ****************************procedure*header*****************************
 */
static void
ChangeManaged(Widget w)
{
Dimension width, height;
Dimension reply_width, reply_height;
OlVendorPartExtension	part = _OlGetVendorPartExtension(w);
	    
	if (!XtIsRealized(w)) {
       XtWidgetProc super_cm;

       super_cm = ((CompositeWidgetClass)vendorShellWidgetClass->
            core_class.superclass)->composite_class.change_managed;
       if (super_cm != (XtWidgetProc)NULL)
           (*super_cm)(w);
    }

	CalculateSize(w, &width, &height);		 

	if (part->from_setvalues) {
		w->core.width = width;
		w->core.height = height;
		Layout(w);
	}
	else {
		switch (XtMakeResizeRequest(w, width, height, 
								&reply_width, &reply_height)) {
			case XtGeometryYes:
				Layout(w);
				break;

			case XtGeometryNo:
				break;

			case XtGeometryAlmost:
				if (XtMakeResizeRequest(w, reply_width, reply_height, 
										NULL, NULL) == XtGeometryYes)
					Layout(w);
				break;
		}
	}
			      
} /* ChangeManaged */

/*
 *************************************************************************
 * InsertChild - this routine does the insertion of the shell's children
 * It maintains the footer as the last child at all times.
 ****************************procedure*header*****************************
 */
static void
InsertChild(Widget w)
{
OlVendorPartExtension	part = _OlGetVendorPartExtension(w->core.parent);
CompositeWidget cw; /* shell widget */
XtWidgetProc super_ic;
int i, j;

	super_ic = ((CompositeWidgetClass)vendorShellWidgetClass->
			core_class.superclass)->composite_class.insert_child;

	if (super_ic != (XtWidgetProc)NULL) 
		(*super_ic)(w);

	cw = (CompositeWidget) w->core.parent;

	if (part && part->footer_present && (cw->composite.num_children > 1)) {
		if (part->footer != /* make sure that the footer is the last child */
			(cw->composite.children[cw->composite.num_children - 1])) {

			for (i = 0; i < cw->composite.num_children; i++) { 
												/* find the footer */
				Widget footer;
				
				if (cw->composite.children[i] == part->footer) {
					footer = cw->composite.children[i];

					for (j = i; j < cw->composite.num_children - 1; j++) 
								/* move the rest up one slot */
						cw->composite.children[j] = cw->composite.children[j+1];

					cw->composite.children[cw->composite.num_children - 1] = footer;
					break; /* we're done */
				}
			}
		}
	}
}

/*
 *************************************************************************
 * GetValues - this routine is the new vendor shell class GetValues
 * procedure.
 ****************************procedure*header*****************************
 */
static void
GetValues(Widget w, ArgList args, Cardinal *num_args)
	      		  		/* VendorShellWidget subclass	*/
	       		     
	          	         
{
	OlVendorPartExtension	part = _OlGetVendorPartExtension(w);

					/* Get the subvalue data.	*/

	if (part != NULL_PART) {
#ifdef	sun
		int	i;
#endif
		MaskArg mask_args[4];


		XtGetSubvalues((XtPointer)part,
			part->class_extension->resources,
			part->class_extension->num_resources, args, *num_args);


		/* XtNconsumeEvent */
		_OlSetMaskArg(mask_args[0], XtNconsumeEvent,
			      part->consume_event, OL_COPY_MASK_VALUE);
		_OlSetMaskArg(mask_args[1], NULL, sizeof(XtCallbackList),
			      OL_COPY_SIZE);
		_OlSetMaskArg(mask_args[2], XtNwmProtocol,
			      part->wm_protocol, OL_COPY_MASK_VALUE);
		_OlSetMaskArg(mask_args[3], NULL, sizeof(XtCallbackList),
			      OL_COPY_SIZE);
		_OlComposeArgList(args, *num_args, mask_args, 4, NULL, NULL);

		(void) CallExtPart(GET_PROC, XtClass(w),
				w, NULL_WIDGET, NULL_WIDGET, args, num_args,
				(XtPointer)part, (XtPointer)NULL,
				(XtPointer)NULL);
#ifdef	sun
		for (i = 0; i < *num_args; i++)
			if (strcmp(XtNolWMRunning, args[i].name) == 0) break;

		if (i < *num_args) {
			part->olwm_running          = HasSunWMProtocols(w);
			*((Boolean *)args[i].value) = part->olwm_running;
		}
		for (i = 0; i < *num_args; i++) {
			if (strcmp(XtNleftFooterString, args[i].name) == 0) {
				*((String *)args[i].value) = part->left_footer_string;
			}
			if (strcmp(XtNrightFooterString, args[i].name) == 0) {
				*((String *)args[i].value) = part->right_footer_string;
			}
			if (strcmp(XtNfooterPresent, args[i].name) == 0) {
				*((Boolean *)args[i].value) = part->footer_present;
			}
			if (strcmp(XtNleftFooterVisible, args[i].name) == 0) {
				*((Boolean *)args[i].value) = part->left_footer_visible;
			}
			if (strcmp(XtNrightFooterVisible, args[i].name) == 0) {
				*((Boolean *)args[i].value) = part->right_footer_visible;
			}
			if (strcmp(XtNimFooterPresent, args[i].name) == 0) {
				*((Boolean *)args[i].value) = part->im_footer_present;
			}
			if (strcmp(XtNimFontSet, args[i].name) == 0) {
				*((OlFont *)args[i].value) = part->im_font_set;
			}
			if (strcmp(XtNimStatusStyle, args[i].name) == 0) {
				*((OlImStatusStyle *)args[i].value) = part->im_status_style;
			}
			if (strcmp(XtNtextFormat, args[i].name) == 0) {
				*((OlStrRep *)args[i].value) = part->text_format;
			}
			if (strcmp(XtNdefaultImName, args[i].name) == 0) {
				*((String *)args[i].value) = part->default_im_name;
			}
			if (strcmp(XtNforeground, args[i].name) == 0) {
				*((Pixel *)args[i].value) = part->foreground;
			}
			if (strcmp(XtNimRect, args[i].name) == 0) {
				if (part->im_footer_present)
					*((XRectangle **)args[i].value) = part->im_rect;
				else 
					*((XRectangle **)args[i].value) = (XRectangle *)NULL;
			}
		}
#endif
	}

	/* now do a get for the dnd class extension */

	CallDnDVCXExtensionMethods(CallDnDVCXGetValues, XtClass(w), w,
				   NULL_WIDGET, NULL_WIDGET, args, num_args);
} /* END OF GetValues() */

/*
 *************************************************************************
 * SetValues - this routine is the new vendor shell class SetValues
 * procedure.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	VendorShellWidget		nvsw = (VendorShellWidget)new;
	VendorShellWidget		cvsw = (VendorShellWidget)current;
	OlVendorClassExtension		ext;
	OlVendorPartExtensionRec	cur_part;
	OlVendorPartExtensionRec	req_part;
	OlVendorPartExtension		new_part =
					_OlGetVendorPartExtension(new);
	Boolean				ext_return;
	Atom				del_list[4];
	Atom 				add_list[4];
	int 				delcount = 0;
	int 				addcount = 0;
	XtCallbackList			cb = NULL;
	XtCallbackList			wm_cb = NULL;
	MaskArg				mask_args[4];
	Atom				atom;
	Display				*dpy = XtDisplay(new);
	Arg im_arg[5];
	int im_i;
	Arg generic_footer_args[3];
	int generic_footer_i;
	Arg left_footer_args[1];
	int left_footer_i;
	Arg right_footer_args[1];
	int right_footer_i;

	if (new_part == NULL_PART) {
		return(False);
	}

	ext = new_part->class_extension;

				/* make a copy of the record, then do
				 * a set values on it.			*/

	cur_part	= *new_part;


				/* Fill in the application's requests.
				 * (Do this in the new part and then
				 * copy the results into the request
				 * part.				*/

	XtSetSubvalues((XtPointer)new_part, ext->resources,
			ext->num_resources, args, *num_args);

	req_part = *new_part;
	new_part->text_format = cur_part.text_format; /* disallow text_format
													changes */

	if(new_part->title != cur_part.title ||
		nvsw->wm.title != cvsw->wm.title)
		SetValuesWMShellTitle(current, request, new, &cur_part, new_part);
#ifdef	sun
	if (new_part->compose_led != cur_part.compose_led)
		SetWindowState(new, new_part);
#endif
	/* XtNconsumeEvent */
	_OlSetMaskArg(mask_args[0], XtNconsumeEvent, &cb, OL_COPY_SOURCE_VALUE);
	_OlSetMaskArg(mask_args[1], NULL, sizeof(XtCallbackList), OL_COPY_SIZE);
	_OlSetMaskArg(mask_args[2], XtNwmProtocol, &wm_cb, OL_COPY_SOURCE_VALUE);
	_OlSetMaskArg(mask_args[3], NULL, sizeof(XtCallbackList), OL_COPY_SIZE);
	_OlComposeArgList(args, *num_args, mask_args, 4, NULL, NULL);

	if (cb) {
		if (new_part->consume_event)
			XtFree((char*)new_part->consume_event);
		new_part->consume_event = CreateVendorCBList(cb);
	}
	if (wm_cb) {
		if (new_part->wm_protocol)
			XtFree((char*)new_part->wm_protocol);
		new_part->wm_protocol = CreateVendorCBList(wm_cb);
	}

	if (XtIsRealized(new)) {
		/* read-only resource after initialize	*/
		new_part->window_header = cur_part.window_header;
		new_part->from_setvalues = True;

		if ((new_part->menu_type != cur_part.menu_type) ||
		    (new_part->busy != cur_part.busy) ||
		    (new_part->pushpin != cur_part.pushpin))
			SetWMAttributes(new, new_part, 
				(new_part->busy != cur_part.busy) ? True : False);
		else {
			if ((new_part->menu_button != cur_part.menu_button) &&
			    (new_part->window_header == True)) {
				atom = OlInternAtom(dpy, _OL_DECOR_CLOSE_NAME);
				if (new_part->menu_button == True)
					add_list[addcount++] = atom;
				else
					del_list[delcount++] = atom;
			}
	
			if (new_part->resize_corners != cur_part.resize_corners) {
				atom = OlInternAtom(dpy, _OL_DECOR_RESIZE_NAME);
				if (new_part->resize_corners == True)
					add_list[addcount++] = atom;
				else
					del_list[delcount++] = atom;
			}

			if (delcount) {
				atom = OlInternAtom(dpy, _OL_DECOR_DEL_NAME);
				XChangeProperty(XtDisplay(new), XtWindow(new), atom,
#ifndef	sun
					atom, 32, PropModeReplace,
#else
					XA_ATOM, 32, PropModeReplace,
#endif
					(unsigned char *) del_list, delcount);
			}

			if (addcount) {
				atom = OlInternAtom(dpy, _OL_DECOR_ADD_NAME);
				XChangeProperty(XtDisplay(new), XtWindow(new), atom,
#ifndef	sun
					atom, 32, PropModeReplace,
#else
					XA_ATOM, 32, PropModeReplace,
#endif
					(unsigned char *) add_list, addcount);
			}
		}

/* #ifdef _MOTIF_INTEROP */
		SetMwmAttributes( dpy, XtWindow(new), new_part );
/* #endif _MOTIF_INTEROP */

	} /* realize */

	if (new_part->footer_present != cur_part.footer_present) {
		if (new_part->footer_present) {
			if (new_part->footer)
				XtManageChild(new_part->footer);
			else 
				CreateFooter(new_part, new);
		} else if (new_part->footer)
			XtUnmanageChild(new_part->footer);
	}

	if (new_part->im_footer_present != cur_part.im_footer_present) {
		if(XtIsRealized(new))
			if (new_part->im_footer_present) 
				new->core.height += ComputeOlFontHeight(new_part);
			else
				new->core.height -= ComputeOlFontHeight(new_part);
	}

	if ((new_part->left_footer_visible != cur_part.left_footer_visible) &&
	     new_part->left_footer)
		XtSetMappedWhenManaged(new_part->left_footer,
			new_part->left_footer_visible);

	if ((new_part->right_footer_visible != cur_part.right_footer_visible) &&
	     new_part->right_footer)
		XtSetMappedWhenManaged(new_part->right_footer,
			new_part->right_footer_visible);

	generic_footer_i = 0;
	left_footer_i = 0;
	right_footer_i = 0;
	im_i = 0;

	if (new_part->left_footer_string != cur_part.left_footer_string) {
		XtFree((XtPointer)cur_part.left_footer_string);
		copy_olstring(new_part->text_format, &(new_part->left_footer_string));
		XtSetArg(left_footer_args[left_footer_i], XtNstring,
			new_part->left_footer_string);
		left_footer_i++;
	}

	if (new_part->right_footer_string != cur_part.right_footer_string) {
		XtFree((XtPointer)cur_part.right_footer_string);
		copy_olstring(new_part->text_format, &(new_part->right_footer_string));
		XtSetArg(right_footer_args[right_footer_i], XtNstring,
			new_part->right_footer_string);
		right_footer_i++;
	}

	/* NOTE: foreground comes from Vendor, bg and bgp from Core */
	if (new_part->foreground != cur_part.foreground) {
		XtSetArg(im_arg[im_i], XNForeground, (XtPointer)new_part->foreground);
		im_i++;
		XtSetArg(generic_footer_args[generic_footer_i], XtNforeground, 
			(XtPointer)new_part->foreground);
		generic_footer_i++;
	}

	if (new->core.background_pixel != current->core.background_pixel) {
		XtSetArg(im_arg[im_i], XNBackground, 
			(XtPointer)new->core.background_pixel);
		im_i++;
		XtSetArg(generic_footer_args[generic_footer_i], XtNbackground, 
			(XtPointer)new->core.background_pixel);
		generic_footer_i++;
	}

	if (new->core.background_pixmap != current->core.background_pixmap) {
		XtSetArg(im_arg[im_i], XNBackgroundPixmap, 
			(XtPointer)new->core.background_pixmap);
		im_i++;
		XtSetArg(generic_footer_args[generic_footer_i], XtNbackgroundPixmap, 
			(XtPointer)new->core.background_pixmap);
		generic_footer_i++;
	}

	if (new_part->im_footer_present) {
		if (new_part->im_font_set != cur_part.im_font_set && 
		    (new_part->text_format != OL_SB_STR_REP)) {
			int new_im_height_diff;

			XtSetArg(im_arg[im_i], XNFontSet, (XtPointer)new_part->im_font_set);
			im_i++;

			new_im_height_diff = (int) ComputeOlFontHeight(new_part) - 
				new_part->im_rect->height;
			if (new_im_height_diff) {
				int new_h = (int) new->core.height;
				new_h += new_im_height_diff;
				new->core.height = (Dimension) new_h;
			}
		}
		if (im_i)
			OlSetIMStatusOnAllICs(new, im_arg, im_i);
	}

	/*
	 * set footer resources on the form
	 */
        if (new_part->footer && generic_footer_i)
		XtSetValues(new_part->footer, generic_footer_args, generic_footer_i);

	/*
	 * set left footer resources on the left static text
	 */
        if (new_part->left_footer) {
        	if (generic_footer_i)
			XtSetValues(new_part->left_footer, generic_footer_args,
				generic_footer_i);
        	if (left_footer_i)
			XtSetValues(new_part->left_footer, left_footer_args,
				left_footer_i);
	}

	/*
	 * set right footer resources on the right static text
	 */
        if (new_part->right_footer) {
        	if (generic_footer_i)
			XtSetValues(new_part->right_footer, generic_footer_args,
				generic_footer_i);
        	if (right_footer_i)
			XtSetValues(new_part->right_footer, right_footer_args,
				right_footer_i);
	}

	new_part->from_setvalues = False;

	/*
	 * This line assumes that THIS SetValues always return False,
	 * thus saving the OR operation with the extension return value.
	 */
	if (ext->set_values) {
		/* 
		 * special set_values proc for subclasses that wants to
		 * override Vendor's setvalues work.  This is done after
		 * everything else in this procedure since subclasses
		 * normally get their set_values procedure calling only.
		 * after the superclass has completed it's work.
		 */
		ext_return = CallExtPart(SET_FUNC, XtClass(new), current,
				request, new, args, num_args,
				(XtPointer)&cur_part, (XtPointer)&req_part,
				(XtPointer)new_part);
	}
	else {
		ext_return   = False;
	}
			
	_OlCheckDynResources(new, &(ext->dyn_data), args, *num_args);

	/* handle background transparency */
    	if ((_OlDynResProcessing == FALSE) &&
	    (new->core.background_pixel != current->core.background_pixel) ||
            (new->core.background_pixmap != current->core.background_pixmap)) {
		int i;
		WidgetList child = ((CompositeWidget)new)->composite.children;
		OlTransparentProc proc;
		
		for (i=((CompositeWidget)new)->composite.num_children; i > 0;
		 	i--,child++) {
			if (proc = _OlGetTransProc(*child))
				(*proc)(*child, new->core.background_pixel,
					new->core.background_pixmap);
		} /* for */
	} /* if */

	ext_return |= CallDnDVCXExtensionMethods(CallDnDVCXSetValues,
						 XtClass(new), current,
						 request, new, args, num_args);

	return(ext_return);
} /* END OF SetValues() */

/****************************procedure*header*****************************
 *
 * TraversalHandler - traverses to next object
 *
 ****************************procedure*header*****************************
 */

/* ARGSUSED */
static Widget
TraversalHandler (Widget shell, Widget w, OlDefine direction, Time time)
{
    OlFocusData * focus_data = _OlGetFocusData(shell, NULL_PART_PTR);
    _OlArray	list;			/* traversal list */
    int		start_pos;
    int		pos;

    if (focus_data == NULL)
    {
	OlWarning(dgettext(OlMsgsDomain,
		"Traversal: failed to get traversal list from shell"));
	return (NULL_WIDGET);
    }


    /*
     * If shell is not sensitive, don't bother switching focus, just set
     * focus to the shell.
     */
    if (XtIsSensitive(shell) == False) {
        Boolean flag1 = shell->core.sensitive;
	Boolean flag2 = shell->core.ancestor_sensitive;

        shell->core.sensitive = True;
	shell->core.ancestor_sensitive = True;
	if (OlCanAcceptFocus(shell, time) == False)
		w = NULL_WIDGET;
	else
	{
		(void)OlSetInputFocus(shell, RevertToNone, time);
		w = shell;
	}
        shell->core.sensitive = flag1;
	shell->core.ancestor_sensitive = flag2;
	return(w);
    }

    /*	re-map direction to inter-object movement (ones understood by
	shell traversal handler.
     */
    switch(direction) {

    case OL_MOVELEFT:
    case OL_MULTILEFT:
    case OL_MOVEUP:
    case OL_MULTIUP:
	direction = OL_PREVFIELD;
	break;

    case OL_MOVERIGHT:
    case OL_MULTIRIGHT:
    case OL_MOVEDOWN:
    case OL_MULTIDOWN:
	direction = OL_NEXTFIELD;
	break;
    }

    list = &(focus_data->traversal_list);	/* get list */

		/* resort the list if necessary */
    ResortTraversalList(list, &focus_data->resort_list);

    /*	Get position in list. */
    if ((start_pos = _OlArrayFind(list, w)) == _OL_NULL_ARRAY_INDEX)
    {
	Widget ancestor = w;

	/* widget not found in list.  Walk up the widget tree looking
	   for an ancestor which could be considered its "managing"
	   ancestor.  This works in particular for controls inside a
	   [non]exclusives, for instance.  The control receives the
	   traversal action key but it's the ancestor that is on the
	   list.

	   It's okay to make ancestor = XtParent(w) right away since
	   ancestor != shell; shell would have been found on list.
	 */
	do
	{
	    ancestor = XtParent(ancestor);
	    if ((start_pos = _OlArrayFind(list, ancestor))
	      != _OL_NULL_ARRAY_INDEX)
		break;
	} while (ancestor != shell);	/* do this last so that shell
					   can be looked for as well
					 */
    }

    /*	if start_pos is *still* NULL, make it zero (start from beginning and
	issue a warning, otherwise, adjust start_pos according to direction.
	(It's unlikely no ancestor was found on list since Shell is put
	on list.  It *is* possible that shell is not Vendor shell).
     */

    if (start_pos == _OL_NULL_ARRAY_INDEX)
    {
	start_pos = 0;

	OlVaDisplayWarningMsg(XtDisplay(shell), "", "",
		OleCOlToolkitWarning,
"VendorShell \"%s\": widget \"%s\" (class \"%s\") not found in traversal list",
		XtName(shell), XtName(w), CLASSNAME(w));

    } else {
	switch (direction)
	{
	case OL_NEXTFIELD :
	    start_pos = (start_pos + 1) % list->num_elements;
	    break;

	case OL_PREVFIELD :
	    start_pos = (start_pos == 0) ?
				list->num_elements - 1 : start_pos - 1;
	    break;

	case OL_IMMEDIATE :
	    direction = OL_NEXTFIELD;	/* for processing at bottom of loop */
	    break;			/* but start_pos remains the same */

	default :
	    OlWarning(dgettext(OlMsgsDomain,
	    	"VendorShell: TraversalHandler - invalid direction. \
			Using OL_NEXTFIELD instead"));
	    direction = OL_NEXTFIELD;	/* fixup direction */
	    break;
	}
    }

    /*	enter main loop to find widget to traverse to.
	Any intra-object traversal direction has been remapped to
	inter-object traversal and IMMEDIATE has been changed to
	NEXTFIELD for processing at bottom of loop.  'start_pos' has
	been established based on direction.

	The shell is skipped in the list since it is not intuitive to
	traverse to it.  It must be in the list, however, to be able
	to traverse *from* it to some descendant.
     */
    pos = start_pos;		/* initial index */

    do
    {
	register Widget trav_widget = (Widget)list->array[pos];	/* get widget */

	/* Check for widget being destroyed */
	if (trav_widget->core.being_destroyed == False)
	{
	    WidgetClass	wc_special = _OlClass(trav_widget);
	    Boolean	traversable;

	    if (wc_special == primitiveWidgetClass)
	    {
		traversable = ((PrimitiveWidget)trav_widget)->primitive.traversal_on;

	    } else if (wc_special == eventObjClass) {
		traversable = ((EventObj)trav_widget)->event.traversal_on;

	    } else if (wc_special == managerWidgetClass) {
		traversable = ((ManagerWidget)trav_widget)->manager.traversal_on;

	    } else if (wc_special == vendorShellWidgetClass) {
		traversable = False;	/* skip the shell */

	    } else {
		traversable = False;
	    }

	    /* Check for traversable widget that can take focus */
	    if (traversable && OlCallAcceptFocus(trav_widget, time))
		return(trav_widget);
	}

	/* get "next" index depending on direction */
	if (direction == OL_NEXTFIELD)
	{
	    pos = (pos + 1) % list->num_elements;	/* next index */
	
	} else if (pos != 0) {
	    pos--;				/* previous index (simple) */

	} else {
	    pos = list->num_elements - 1;	/* previous (ring around) */
	}

    } while (pos != start_pos);

		/* there is a timing problem here if we don't do this	*/
		/* check. A BadMatch error will happen if a user pops	*/
		/* up/down a menu really quick (e.g., press-drag)	*/
    if (OlCanAcceptFocus(shell, time))
    {
    	(void)OlSetInputFocus(shell, RevertToNone, time);
	return(shell);
    }
    return(NULL_WIDGET);

} /* END OF TraversalHandler() */

static void
WMMsgHandler (Widget w, OlDefine action, OlWMProtocolVerify *wmpv)
{
	if (wmpv->msgtype == OL_WM_DELETE_WINDOW) {
		switch(action) {
		case OL_QUIT:
		case OL_DEFAULTACTION:
			if (XtIsSubclass(w, topLevelShellWidgetClass) == False)
				XtPopdown(w);
			else {
				OlVendorPartExtension	part = 
					_OlGetVendorPartExtension(
					 _OlGetShellOfWidget(w));

				XtUnmapWidget(w);
			    	if (part->pushpin == OL_NONE)
					exit(EXIT_SUCCESS);
			}
			break;
		case OL_DESTROY:
			XtDestroyWidget(w);
			break;
		case OL_DISMISS:
			if (XtIsSubclass(w, topLevelShellWidgetClass) == False)
				XtPopdown(w);
			else
				XtUnmapWidget(w);
			break;
		}
	}
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
 * TakeFocusEventHandler - This function responds to the window manager's
 * WM_TAKE_FOCUS message by giving focus to the widget registered by the
 * application or to the last widget that had focus.   OlMoveFocus()
 * is used to set the focus so that if the registered widget can not
 * accept focus, the traversal list will be searched for one that can.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static void
TakeFocusEventHandler(Widget w, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch)
	      		  			/* VendorShell Widget	*/
	         	            		/* OlFocusData		*/
	        	      
	         	                     
{
    if (event->type == ClientMessage &&
        event->xclient.message_type == OlInternAtom(event->xany.display,
						    WM_PROTOCOLS_NAME) &&
        (Atom)event->xclient.data.l[0] == OlInternAtom(event->xany.display,
							WM_TAKE_FOCUS_NAME))
    {
	Time		time = (Time)event->xclient.data.l[1];

	(void)OlCallAcceptFocus(w, time);
    }
} /* END OF TakeFocusEventHandler() */

/****************************procedure*header*****************************
 *
 * WMMessageHandler - handles message from the window manager which
 * come in the form of Property or Client Messages, and property changes.
 * Vendor needs to track property changes to keep the resource values in
 * vendor in sync with the window property values, because some old clients
 * may be doing XChangeProperty or calling convenience routines to update
 * decorations directly.
 */

/*
 * pulled out code from WMMessageHandler to simplify it!!!
 */

static void
_doWinAttrs (Widget w, OlVendorPartExtension part, XEvent *xevent)
{
#ifndef	sun	/* DO THE OLD CODE */
OLWinAttr wa;

if (GetOLWinAttr(xevent->xany.display, XtWindow(widget), &wa) == Success) {
	if (wa.flags & _OL_WA_WIN_TYPE) {
		struct WinTypeDecor *decor;

		for (decor=wintype_decor;
		     decor->win_type != NULL; decor++)
			if (OlInternAtom(xevent->xany.display,
					 decor->win_atom) == wa.win_type) {
#define COPY(X)		part->X = decor->X
				COPY(win_type);
				COPY(window_header);
				COPY(menu_button);
				COPY(resize_corners);
				COPY(menu_type);

				if (decor->pushpin == True)
					part->pushpin = OL_OUT;
				else
					part->pushpin = OL_NONE;
				break;
			}
#undef COPY

		if (wa.flags & _OL_WA_MENU_TYPE) {
			if (wa.menu_type == 
			    OlInternAtom(xevent->xany.display,
					 _OL_MENU_FULLNAME))
				part->menu_type = OL_MENU_FULL;
			else if (wa.menu_type == 
				 OlInternAtom(xevent->xany.display,
                                              _OL_NONE_NAME))
				part->menu_type = OL_NONE;
			else if (wa.menu_type == 
				 OlInternAtom(xevent->xany.display,
                                              _OL_MENU_LIMITED_NAME)) {
				if ((wa.flags & _OL_WA_CANCEL) &&
				    (wa.cancel))
				part->menu_type = OL_MENU_CANCEL;
			}
		}
		if (wa.flags & _OL_WA_PIN_STATE) {
			if (wa.pin_state)
				part->pushpin = OL_IN;
			else
				part->pushpin = OL_OUT;
		}
} /*if*/
#else /* DO SUN CODE */
	Atom            atr;
	int             afr;
	unsigned long   nir,
			bar;
	unsigned long	win_type;
	int             numtofetch;
	Boolean		use_short_olwinattr;
	Display		*dpy = xevent->xany.display;
	union {	
		NewOLWinAttr   *lprop;
		OLWinAttr     	*prop;
	} olwinattrs;
	Atom		atom;

	use_short_olwinattr = _OlUseShortOLWinAttr(dpy);
	numtofetch = (use_short_olwinattr ? ULongsInOLWinAttr :
					    ULongsInNewOLWinAttr);

	atom = OlInternAtom(dpy, _OL_WIN_ATTR_NAME);

	if (XGetWindowProperty(dpy, XtWindow(w),
			       atom, 0L, numtofetch, False,
			       atom, &atr, &afr, &nir, &bar,
			       (unsigned char **)&olwinattrs) == Success) {

		if (atr != atom || afr != 32 ||
		    (nir != ULongsInOLWinAttr &&
		     nir != ULongsInNewOLWinAttr)) {
			if (olwinattrs.prop != (OLWinAttr *)NULL)
				XtFree((char *)olwinattrs.prop);
			return;
		}
			
		use_short_olwinattr = (nir == ULongsInOLWinAttr);

		if (use_short_olwinattr ||
		    (olwinattrs.lprop->flags & _OL_WA_WIN_TYPE)) {
			struct WinTypeDecor *decor;

			for (decor=wintype_decor;
			     decor->win_type != NULL; decor++) {
				win_type = (use_short_olwinattr ?
						 olwinattrs.prop->win_type :
						 olwinattrs.lprop->win_type); 
				if (OlInternAtom(dpy, decor->win_atom) ==
					win_type) {
					/* copy std values */
#define COPY(X)		part->X = decor->X
					COPY(win_type);
					COPY(window_header);
					COPY(menu_button);
					COPY(resize_corners);
					COPY(menu_type);

					if (decor->pushpin == True)
						part->pushpin = OL_OUT;
					else
						part->pushpin = OL_NONE;
					break;
#undef COPY
				}
			}
		}

#define	CheckField(f, v)	\
		((use_short_olwinattr && olwinattrs.prop->f == v) ||	\
		  olwinattrs.lprop->f == v)

		if (use_short_olwinattr ||
		    olwinattrs.lprop->flags & _OL_WA_MENU_TYPE) {
			if (CheckField(menu_type, 
				       OlInternAtom(dpy, _OL_MENU_FULL_NAME)))
				part->menu_type = OL_MENU_FULL;
			else if (CheckField(menu_type,
					   OlInternAtom(dpy, _OL_NONE_NAME)))
				part->menu_type = OL_NONE;
			else if (!use_short_olwinattr &&
				 olwinattrs.lprop->menu_type ==
				OlInternAtom(dpy, _OL_MENU_LIMITED_NAME)) {
				if ((olwinattrs.lprop->flags & _OL_WA_CANCEL) &&
				    (olwinattrs.lprop->cancel))
				part->menu_type = OL_MENU_CANCEL;
			} else  part->menu_type = 
					OlInternAtom(dpy,_OL_MENU_LIMITED_NAME);
		}

		if (part->pushpin != OL_NONE && 
		    (use_short_olwinattr ||
		    (olwinattrs.lprop->flags & _OL_WA_PIN_STATE))) {
			if ((use_short_olwinattr && olwinattrs.prop->pin_state)
			    || olwinattrs.lprop->pin_state)
				part->pushpin = OL_IN;
			else
				part->pushpin = OL_OUT;
		}

#undef	CheckField
	}
	if (olwinattrs.prop != (OLWinAttr *)NULL)
		XtFree((char *)olwinattrs.prop);
#endif	/* ALL DONE */
}

static void
WMMessageHandler(register Widget widget, XtPointer data, XEvent *xevent, Boolean *cont_to_dispatch)
{
	Atom	atom;
	Display	*dpy = xevent->xany.display;

	OlVendorPartExtension	part = 
			_OlGetVendorPartExtension(_OlGetShellOfWidget(widget));


        if (xevent->xany.type == PropertyNotify) {
		if (xevent->xproperty.atom ==
		    OlInternAtom(dpy, _OL_PIN_STATE_NAME)) {
        		long pushpin_state;

                	GetWMPushpinState(dpy,
                                XtWindow(widget), &pushpin_state);

                	if (pushpin_state == WMPushpinIsIn)
                	{
				part->pushpin = OL_IN;
                	}
                	else if (pushpin_state == WMPushpinIsOut)
                	{
				part->pushpin = OL_OUT;
                	}
		}
		else if (xevent->xproperty.atom == 
			 OlInternAtom(dpy, _OL_WIN_BUSY_NAME)) {
			long busy_state;

			GetWMWindowBusy(xevent->xany.display, XtWindow(widget),
					&busy_state);

			part->busy = (busy_state == WMWindowIsBusy ?
						TRUE : FALSE);
		}
		else if ((xevent->xproperty.atom == OlInternAtom(dpy,
							_OL_DECOR_ADD_NAME)) ||
		         (xevent->xproperty.atom == OlInternAtom(dpy,
                                                        _OL_DECOR_DEL_NAME))) {
			Atom	*atoms;
			Atom	*save_atoms;
			int	n;
			Boolean state;

			save_atoms = atoms = GetAtomList(dpy,
							 XtWindow(widget),
							 xevent->xproperty.atom,
							 &n, False);

			if (atoms) {
				if (xevent->xproperty.atom ==
				    OlInternAtom(dpy,
						 _OL_DECOR_ADD_NAME))
					state = True;
				else
					state = False;

				while (n--) {
					if (*atoms == 
					    OlInternAtom(dpy,
							 _OL_DECOR_CLOSE_NAME))
						part->menu_button = state;
					if (*atoms == 
					    OlInternAtom(dpy,
							 _OL_DECOR_RESIZE_NAME))
						part->resize_corners = state;
					if (*atoms == 
					    OlInternAtom(dpy,
							 _OL_DECOR_HEADER_NAME))
						part->window_header = state;
					if (*atoms == 
					    OlInternAtom(dpy,
							 _OL_DECOR_PIN_NAME)) {
						if (state == False)
							part->pushpin = OL_NONE;
						else if (part->pushpin == OL_NONE)
							part->pushpin = OL_OUT;
					}
					atoms++;
				}
				XtFree((char*)save_atoms);
			}
		} else if (xevent->xproperty.atom == 
			 (atom = OlInternAtom(dpy,
					      _OL_WIN_ATTR_NAME))) {
				_doWinAttrs(widget, part, xevent);
		       }
        } else if (xevent->xclient.message_type == 
		 OlInternAtom(dpy,
			      WM_PROTOCOLS_NAME)) {
		unsigned long msgtype = 0;

		if (xevent->xclient.data.l[0] == 
		    OlInternAtom(dpy,
				 WM_SAVE_YOURSELF_NAME)) {
			msgtype = OL_WM_SAVE_YOURSELF;
		}
		else if (xevent->xclient.data.l[0] == 
			 OlInternAtom(dpy,
				      WM_DELETE_WINDOW_NAME)) {
			msgtype = OL_WM_DELETE_WINDOW;
		}
		else if (xevent->xclient.data.l[0] == 
		         OlInternAtom(dpy,
				      WM_TAKE_FOCUS_NAME)) {
			msgtype = OL_WM_TAKE_FOCUS;
		}

		if (msgtype) {
			OlWMProtocolVerify st;
			OlVendorClassExtension	ext = part->class_extension;

			st.msgtype = msgtype;
			st.xevent  = xevent;

			
 			if ((ext->override_callback == FALSE) &&
			    (OlHasCallbacks(widget, XtNwmProtocol) ==
				 XtCallbackHasSome))
				OlCallCallbacks(widget, XtNwmProtocol,
					 (XtPointer)&st);
			else if (ext->wm_proc)
				(*(ext->wm_proc))(widget,OL_DEFAULTACTION,&st);
		}
	}
} /* END OF WMMessageHandler() */


/****************************public*procedures****************************
 *
 * Public Procedures
 *
 */

/*
 *************************************************************************
 * _OlGetDefault - this routine gets the default associated with a shell.
 ****************************procedure*header*****************************
 */
Widget
_OlGetDefault (Widget w)
{
	OlVendorPartExtension	part =
			_OlGetVendorPartExtension(_OlGetShellOfWidget(w));
	Widget		default_widget;
	
	if (part != NULL_PART) {
		OlGetDefaultProc proc = part->class_extension->get_default;

		if (proc != (OlGetDefaultProc)NULL)
		{
			(*proc)(part->vendor, part->default_widget);
		}
		default_widget = part->default_widget;
	} else {
		default_widget = NULL_WIDGET;
	}

	return (default_widget);
} /* END OF _OlGetDefault() */

/*
 *************************************************************************
 * _OlGetFocusData - this routine routines the focus data associated with
 * the supplied widget.
 * If the a vendor extension part pointer is supplied, the part pointer
 * is initialized with the vendor extension part's address.
 ****************************procedure*header*****************************
 */
OlFocusData *
_OlGetFocusData (Widget w, OlVendorPartExtension *part_ptr)
{
	OlFocusData *	fd = NULL_FOCUS_DATA;

	if (part_ptr != NULL_PART_PTR)
		*part_ptr = NULL_PART;

	if (w == NULL_WIDGET)
	    return(NULL_FOCUS_DATA);

	w = _OlGetShellOfWidget(w);

	if ((w != NULL_WIDGET) && (XtIsVendorShell(w) == True))
	{
		OlVendorPartExtension part = _OlGetVendorPartExtension(w);

		if (part_ptr != NULL_PART_PTR)
			*part_ptr = part;

		if (part != NULL_PART)
			fd = &(part->focus_data);
	}
	return (fd);
} /* END OF _OlGetFocusData() */

/*
 *************************************************************************
 * _OlGetVendorClassExtension - this routine returns the class extension
 * associated with a vendor subclass.  Subclasses of the vendor shell
 * should use this routine to access the vendor's class part.
 ****************************procedure*header*****************************
 */
OlVendorClassExtension
_OlGetVendorClassExtension (WidgetClass wc)
{
	OlVendorClassExtension	ext = NULL_EXTENSION;

	if (!wc) {
	    	OlWarning(dgettext(OlMsgsDomain,
	              "_OlGetVendorClassExtension: NULL WidgetClass pointer"));
	} else {
		ext = GET_EXT(wc);
	}
	return(ext);
} /* END OF _OlGetVendorClassExtension() */

/*
 *************************************************************************
 * _OlGetVendorPartExtension - this routine returns the part extension
 * data associated with a vendor subclass widget instance.  Subclasses
 * of the vendor shell should use this routine to access the vendor's
 * part.
 ****************************procedure*header*****************************
 */
OlVendorPartExtension
_OlGetVendorPartExtension (Widget w)
{
	OlVendorPartExtension	part = NULL_PART;

	if (w == NULL_WIDGET) {
	    	OlWarning(dgettext(OlMsgsDomain,
				"_OlGetVendorPartExtension: NULL widget"));
	} else if (XtIsVendorShell(w) == True) {
		OlVendorClassExtension	extension = GET_EXT(XtClass(w));
		OlVendorPartExtension *	part_ptr = &extension->part_list;

		for (part = *part_ptr; part != NULL_PART && w != part->vendor;)
		{
			part_ptr	= &part->next;
			part		= *part_ptr;
		}
	}
	return(part);
} /* END OF _OlGetVendorPartExtension() */

/****************************procedure*header*****************************
 * _OlLoadVendorShell - this routine is called during the toolkit
 * initialization to force a reference to this file's object file.
 * If at some point in the future loading an entire new vendor class
 * record causes a problem (e.g., masks new routines placed into
 * the Intrinsic's vendor class record by MIT), then we can simply
 * remove our class record and insert our procedures in there
 * corresponding fields.  If we do this, then our procedures will be
 * responsible for calling the replacing Intrinsic's routine.
 *
   Take this opportunity to fix up shell inheritance.  Transient and its
   superclasses should inherit fields from Core but this is not the way
   it is shipped from MIT.
 */
void
_OlLoadVendorShell(void)
{
#ifdef SHARELIB
   void **__libXol__XtInherit = _libXol__XtInherit;
#undef _XtInherit
#define _XtInherit      (*__libXol__XtInherit)
#endif

#define ForceInheritance(wc) \
	wc->core_class.accept_focus = XtInheritAcceptFocus; \
	wc->core_class.tm_table = XtInheritTranslations;

    ForceInheritance(transientShellWidgetClass);
    ForceInheritance(topLevelShellWidgetClass);
    ForceInheritance(applicationShellWidgetClass);

#undef ForceInheritance

} /* END OF _OlLoadVendorShell() */

/*
 *************************************************************************
 * _OlSetDefault - this routine sets the default widget on a vendor
 * shell.  Then if the particular shell has a SetDefault procedure it
 * is called with the new default widget id.  If a widget wants tobe the
 * default and there is already a default widget, the old default widget
 * is unset by calling XtSetValues on it.
 * Note: this routine does not check to see if the widget which
 * called this routine has actually set it's default value.
 ****************************procedure*header*****************************
 */
void
_OlSetDefault (Widget w, Boolean wants_to_be_default)
{
	Widget			old_default;
	OlVendorClassExtension	ext = NULL_EXTENSION;
	OlVendorPartExtension	part;

	part = _OlGetVendorPartExtension(_OlGetShellOfWidget(w));

	if (part == NULL_PART) {
		return;
	}

	ext		= part->class_extension;
	old_default	= part->default_widget;

	if (wants_to_be_default == False) {

				/* If this widget no longer wants to
				 * be the default and it's currently,
				 * the default, remove it; else, do
				 * nothing.				*/

		if (part->default_widget == w) {
			part->default_widget = NULL_WIDGET;
		}
	} else {

				/* If this widget wants to be the default
				 * and it's not the current default,
				 * set it to be so, else, noop.		*/

		if (part->default_widget != w) {
			part->default_widget = w;

			if (old_default != NULL_WIDGET) {
				Arg	args[1];

				XtSetArg(args[0], XtNdefault, False);
				XtSetValues(old_default, args, 1);
			}
		}
	}

			/* If the shell's default changed, call the class
			 * procedure with the old and new defaults.	*/

	if (part->vendor->core.being_destroyed == False &&
	    part->default_widget != old_default &&
	    ext->set_default != (OlSetDefaultProc)NULL)
	{
		(* ext->set_default) (part->vendor, part->default_widget);
	}
} /* END OF _OlSetDefault() */

/*************************************************************************
 * _OlSetPinState -
 */
void
_OlSetPinState (Widget w, OlDefine pin_state)
{
	OlVendorPartExtension	part =
			_OlGetVendorPartExtension(_OlGetShellOfWidget(w));
	
	if (part->pushpin != pin_state) {
		part->pushpin = pin_state;
		SetWMAttributes(w, part, False);
	}
}

static void
_OlAddToShellList (Widget w)
{
	Widget			vsw	= _OlFindVendorShell(w, False);

	if (vsw) OlAddDynamicScreenCB(XtScreen(vsw),
				      _OlNewAcceleratorResourceValues,
			 	      (XtPointer)vsw);

	_OlRegisterShell(w);
}

static void
_OlDelFromShellList (Widget w)
{
	Widget			vsw	= _OlFindVendorShell(w, False);

	if (vsw) OlRemoveDynamicScreenCB(XtScreen(vsw),
					 _OlNewAcceleratorResourceValues,
			 		 (XtPointer)vsw);

	_OlUnregisterShell(w);
}

void
OlWMProtocolAction (Widget w, OlWMProtocolVerify *st, OlDefine action)
{
	OlVendorPartExtension	part;

	GetToken();
	if (XtIsSubclass(w, vendorShellWidgetClass) && 
	    (part = _OlGetVendorPartExtension(_OlGetShellOfWidget(w))) &&
	    (part->class_extension->wm_proc)) {
		(*(part->class_extension->wm_proc))(w, action, st);
	}
	ReleaseToken();
}

/**
 ** _OlFindVendorShell()
 **/

Widget
_OlFindVendorShell (Widget w, Boolean is_mnemonic)
{
	do {
		while (w && !XtIsVendorShell(w))
			w = XtParent(w);
		if (is_mnemonic || !w || !XtParent(w))
			break;
		w = XtParent(w);
	} while (w);

	return (w);
}

void 
OlAddCallback (Widget widget, String name, XtCallbackProc callback, XtPointer closure)
{
	int wm;

	GetToken();

	if (XtIsSubclass(widget, vendorShellWidgetClass) &&
	    (((wm = !strcmp(name, XtNwmProtocol)) != 0) ||
	     !strcmp(name, XtNconsumeEvent))) {
		OlVendorPartExtension pe = _OlGetVendorPartExtension(widget);
		XtCallbackList cb;
		XtCallbackList *first;

		if (!pe) {
			ReleaseToken();
			return;
		}

		if (wm)
			first = &(pe->wm_protocol);
		else
			first = &(pe->consume_event);

		if ((cb = *first) == NULL) {
			/* first time */
			cb = (XtCallbackList)XtMalloc(sizeof(XtCallbackRec)*2);
			if (cb == NULL) {
	    			OlWarning(dgettext(OlMsgsDomain,
					"OlAddCallback: out of memory"));
				ReleaseToken();
				return;
			}

			*first = cb;
			cb->callback = callback;
			cb->closure  = closure;
			(cb+1)->callback = NULL;
		}
		else {
			int i;

			/* count the # of entries */
			for (i=0, cb=*first; cb->callback; i++,cb++);

			cb = (XtCallbackList)XtMalloc(sizeof(XtCallbackRec) *
				(i+2));
			if (cb == NULL) {
	    			OlWarning(dgettext(OlMsgsDomain,
					"OlAddCallback: out of memory"));
				ReleaseToken();
				return;
			}
			cb->callback = callback;
			cb->closure  = closure;
			(void) memcpy((char *)(cb+1), (char *)(*first),
			       sizeof(XtCallbackRec) * (i+1));
			XtFree((char *)*first); /* free the old list */
			*first = cb;
		}
	}
	else
		XtAddCallback(widget, name, callback, closure);

	ReleaseToken();
}

void 
OlRemoveCallback (Widget widget, String name, XtCallbackProc callback, XtPointer closure)
{
	int wm;

	GetToken();

	if (XtIsSubclass(widget, vendorShellWidgetClass) &&
	    (((wm = !strcmp(name, XtNwmProtocol)) != 0) ||
	     !strcmp(name, XtNconsumeEvent))) {
		OlVendorPartExtension pe = _OlGetVendorPartExtension(widget);
		XtCallbackList cb;
		XtCallbackList *first;

		if (!pe) {
			ReleaseToken();
			return;
		}

		if (wm) {
			cb = pe->wm_protocol;
			first = &(pe->wm_protocol);
		}
		else {
			cb = pe->consume_event;
			first = &(pe->consume_event);
		}
		if (!cb) {
			ReleaseToken();
			return;
		}

		if ((callback == NULL) || (cb == NULL)) {
			ReleaseToken();
			return;
		}

		for (; cb->callback; cb++)
			if ((cb->callback == callback) &&
			    (cb->closure  == closure)) {
				/*
				 * Found it!
				 * just shift up the rest of the list.
				 * don't bother shrinking the space.
				 */
				do {
					*cb = *(cb+1);
					cb++;
				} while (cb->callback);

				/* if an empty list */
				if (cb == *first) {
					XtFree((char *)*first);
					*first = NULL;
				}
				break;
			}
	}
	else
		XtRemoveCallback(widget, name, callback, closure);

	ReleaseToken();
}

XtCallbackStatus 
OlHasCallbacks (Widget widget, String callback_name)
{
	int wm;
	XtCallbackStatus retval;

	GetToken();

	if (XtIsSubclass(widget, vendorShellWidgetClass) &&
	    (((wm = !strcmp(callback_name, XtNwmProtocol)) != 0) ||
	     !strcmp(callback_name, XtNconsumeEvent))) {
		OlVendorPartExtension pe = _OlGetVendorPartExtension(widget);
		XtCallbackList cb;

		if (!pe) {
			ReleaseToken();
			return(XtCallbackNoList);
		}
		
		if (wm)
			cb = pe->wm_protocol;
		else
			cb = pe->consume_event;
		if (cb) {
			ReleaseToken();
			return(XtCallbackHasSome);
		}
		
		else {
			ReleaseToken();
			return(XtCallbackHasNone);
		}
	}
	else {
		retval = XtHasCallbacks(widget, callback_name);
		ReleaseToken();
		return retval;
	}
}

void 
OlCallCallbacks (Widget widget, String name, XtPointer call_data)
{
	int wm;

	GetToken();

	if (XtIsSubclass(widget, vendorShellWidgetClass) &&
	    (((wm = !strcmp(name, XtNwmProtocol)) != 0) ||
	     !strcmp(name, XtNconsumeEvent))) {
		OlVendorPartExtension pe = _OlGetVendorPartExtension(widget);
		XtCallbackList cb;

		if (!pe) {
			ReleaseToken();
			return;
		}
		
		if (wm)
			cb = pe->wm_protocol;
		else
			cb = pe->consume_event;
		if (!cb) {
			ReleaseToken();
			return;
		}

		for (; cb->callback; cb++)
			(*(cb->callback))(widget, cb->closure, call_data);
		
	}
	else
		XtCallCallbacks(widget, name, call_data);

	ReleaseToken();
}

#ifdef	sun	/*OWV3*/

/*
 *************************************************************************
 *
 * HasSunWMProtocols()
 *
 * SetWindowState()
 *
 *************************************************************************
 */

static Boolean
HasSunWMProtocols (Widget w)
{
	int		i,
			status;
	Atom		actType;
	int		actFormat;
	unsigned long 	nitems,
			bytes_after;
	Atom 		*prop   = (Atom *)NULL;
	Bool		result  = False;
	Display		*dpy    = XtDisplayOfObject(w);
	Screen		*screen = XtScreenOfObject(w);

	status = XGetWindowProperty(dpy,
				    RootWindowOfScreen(screen),
				    OlInternAtom(dpy, _SUN_WM_PROTOCOLS_NAME),
				    0,100L,False,XA_ATOM, &actType,&actFormat,
				    &nitems,&bytes_after,
				    (unsigned char **)&prop);

	if (status != Success)
		return False;
	
	for (i = 0; i < nitems ; i++) {
		if (prop[i] == OlInternAtom(dpy, _SUN_WINDOW_STATE_NAME))
			result = True;
	}

	if (prop != (Atom *)NULL && nitems > 0)
		XFree((char *)prop);

	return result;
}


static void 
SetValuesWMShellTitle(Widget 			current, 
		      Widget 			request, 
		      Widget 			new,
		      OlVendorPartExtension   	cur_part,
		      OlVendorPartExtension   	new_part)
{
	VendorShellWidget 	nvsw = (VendorShellWidget)new;
	VendorShellWidget	cvsw = (VendorShellWidget)current;

	if(new_part->title != cur_part->title && 
				new_part->title != NULL) { 
		if(cur_part->title != NULL)
			XtFree((char *)cur_part->title);
		InitWMShellTitle(request, new, new_part);
	} else
		if(nvsw->wm.title != cvsw->wm.title &&
						nvsw->wm.title != NULL) {
			if(cur_part->title != NULL)
				XtFree((char *)cur_part->title);
			new_part->title = cur_part->title = NULL;
			InitWMShellTitle(request, new, new_part);
		}
}

static void 
InitWMShellTitle(
		Widget request, 
		Widget new, 
		OlVendorPartExtension           part)
{ 
	VendorShellWidget		vsw = (VendorShellWidget)new;

	switch(part->text_format) {
		case OL_MB_STR_REP:
		case OL_SB_STR_REP:
			if(part->title != (OlStr)NULL)  { 
				part->title = (OlStr)XtNewString((char *)part->title);
				if(vsw->wm.title != NULL)
					XtFree(vsw->wm.title);
				vsw->wm.title = XtNewString((char *)part->title);
			} else
				part->title = (OlStr)XtNewString(vsw->wm.title);
			break;
		case OL_WC_STR_REP:
			if(part->title != (OlStr)NULL) {
				size_t num;
				size_t n = wslen((wchar_t *)part->title)*
							MB_CUR_MAX + 1;
				char *mbs = (char *)XtMalloc(n);

				part->title = (OlStr)wsdup((wchar_t *)part->title);
				
				if((num = wcstombs(mbs,part->title,n)) 
								!= (size_t)-1) { 
					if(vsw->wm.title != (char *)NULL)
						XtFree(vsw->wm.title);
					vsw->wm.title = XtNewString(mbs);
				}

				if(mbs == (char *)NULL)
					XtFree(mbs);

			} else {
				wchar_t *title;
				size_t num = strlen(vsw->wm.title) + 1;

				title = (wchar_t *)XtMalloc(num*sizeof(wchar_t));  
				if(mbstowcs(title, vsw->wm.title, num) != (size_t)-1)
					part->title = (OlStr)title;
			}
				
			break;
		default:	
			break;
	}
}

static void
SetWindowState (Widget w, OlVendorPartExtension part)
{
	Display	*dpy;

	struct _windowstate {
		unsigned long    flags;
		unsigned long    state;
	} WindowState;

#define WSSemanticState         (1L<<0)	/* value for WindowState flags */
#define WSSemanticCompose       (1L<<0) /* value for WindowState state */

	if (!XtIsSubclass(w, vendorShellWidgetClass)) {
		w    = _OlGetShellOfWidget(w);
		part = _OlGetVendorPartExtension(w);
	}

	if (!XtIsRealized(w) || !part->olwm_running) return;
	dpy = XtDisplay(w);

	WindowState.flags = WSSemanticState;
	WindowState.state = (part->compose_led ? WSSemanticCompose : 0L);

	XChangeProperty(dpy, w->core.window,
			OlInternAtom(dpy, _SUN_WINDOW_STATE_NAME),
			XA_INTEGER, 32, PropModeReplace, 
			(unsigned char *)&WindowState,
			sizeof(WindowState) / 4);
}


/*
 *************************************************************************
 * _OlFixResourceList - this function moves the visual 
 * resource in front of the colormap resource in the 
 * widget's resource list.  This is done so the visual 
 * resource get resolved before the colormap resource.
 ****************************procedure*header*****************************
 */
void
_OlFixResourceList (WidgetClass wc)
{
	Cardinal num_res = wc->core_class.num_resources;
	XrmQuark cname, vname;
	XrmResourceList	*new_res, *nres, *visual_pos,
		*res = (XrmResourceList *)
		wc->core_class.resources;
	int i = 0, gotit = 0;

	if (num_res > 0) {
		/* allocate new resource list */
		new_res = nres = (XrmResourceList *)
			XtMalloc(num_res *
			sizeof(XrmResourceList));

		cname = XrmStringToName(XtNcolormap);
		vname = XrmStringToName(XtNvisual);

		/*
		 * find colormap resource
		 * and move up one position
		 */
		while (i < num_res) {
			if (vname == (*res)->xrm_name) {
				/* list is already in order */
				XtFree((char*)nres);
				return;
			} else if (cname == (*res)->xrm_name) {
				visual_pos = new_res++;
				*new_res++ = *res++;
				i++;
				break;
			}
			*new_res++ = *res++;
			i++;
		}

		/*
		 * find visual resource and
		 * move up in front of colormap
		 */
		while (i < num_res) {
			if (vname == (*res)->xrm_name) {
				*visual_pos = *res++;
				i++;
				gotit = 1;
				break;
			}
			*new_res++ = *res++;
			i++;
		}

		if (gotit) {
			/* copy the remaining */
			memmove((void*)new_res, (void*)res,
				(size_t)((num_res - i)
				 * sizeof(XrmResourceList)));

			/* free the old list */
			XtFree((char  *)
				(wc->core_class.resources));

			/* replace with the new list */
			wc->core_class.resources = 
				(XtResourceList)nres;
		} else
			XtFree((char*)nres);
	}
}

/*
 *************************************************************************
 * _OlCopyParentsVisual - 
 ****************************procedure*header*****************************
 */
void
_OlCopyParentsVisual (Widget widget, int closure, XrmValue *value)
{
	static Visual *visual;

	if (XtParent(widget) == NULL)
		visual = DefaultVisualOfScreen (widget->core.screen);
	else
		visual = OlVisualOfObject(XtParent(widget));

	value->addr = (caddr_t) &visual;
}

/*
 *************************************************************************
 * _OlSetupColormap - 
 ****************************procedure*header*****************************
 */
void
_OlSetupColormap (Widget widget, int closure, XrmValue *value)
{
	Screen *screen = XtScreenOfObject(widget);
	Visual *visual;
	static Colormap cm;
	XrmValue from, to;
	Widget	parent = XtParent(widget);
	Visual *parents_visual = (parent != (Widget)NULL ?
					OlVisualOfObject(parent):
					(Visual *)NULL);

	if (XtIsShell(widget))
		 visual=((ShellWidget)widget)->shell.visual;
	else if (XtIsSubclass(widget, drawAreaWidgetClass))
		 visual=((DrawAreaWidget)widget)->draw_area.visual;
	else
		OlError(dgettext(OlMsgsDomain,
			"_OlSetupColormap: no visual associated with widget"));

	if (visual == CopyFromParent)
		OlError(dgettext(OlMsgsDomain,
		   "_OlSetupColormap: visual should never be CopyFromParent"));

	
	/* 
	 * if the parents visual is the same as ours,
	 * inherit the colormap.
	 */

	if(parents_visual != (Visual *)NULL &&
			(XVisualIDFromVisual(visual) ==
			XVisualIDFromVisual(parents_visual)) ) {
		Arg arg[1];
		int i = 0;

		XtSetArg(arg[i], XtNcolormap, &cm); i++;
		XtGetValues(parent, arg, i);
		value->addr = (XtPointer)&cm;
	} else {
		/*
	 	 * Call the Visual to Colormap converter
	 	 * to create a colormap
		 */
		from.size = sizeof(Visual **);
		from.addr = (caddr_t) &visual;
	
		to.size = sizeof(Colormap);
		to.addr = (caddr_t) &cm;
	
		if (XtConvertAndStore(widget, XtRVisual, &from,
			XtRColormap, &to) == False)
			OlError(dgettext(OlMsgsDomain,
		      	"_OlSetupColormap: visual to colormap converter failed"));

		value->addr = (caddr_t) &cm;
	}

}
#endif	/*OWV3*/

extern void
_OlVendorRereadSomeResources(Widget     vw,
			     Arg *      args,
			     Cardinal * num_args)
{
    OlVendorPartExtension part = _OlGetVendorPartExtension(vw);

    XtGetSubresources(vw, (XtPointer) vw,
		      (String) NULL, (String) NULL,
		      work_space_widg_res,
		      XtNumber(work_space_widg_res),
		      args, *num_args
		      );
    XtGetSubresources(vw, (XtPointer) part,
		      (String) NULL, (String) NULL,
		      work_space_ext_res,
		      XtNumber(work_space_ext_res),
		      args, *num_args
		      );
}

void
_OlVendorSetSuperCaretFocus(const Widget vw,
                            const Widget focus_widget)
{
        OlVendorPartExtension ext_part = _OlGetVendorPartExtension(vw);

        if (ext_part == (OlVendorPartExtension)NULL ||
            ext_part->supercaret == (Widget)NULL    ||
            _OlInputFocusFeedback(vw) != OL_SUPERCARET)
                return;


        _OlCallUpdateSuperCaret(ext_part->supercaret, focus_widget);
}
