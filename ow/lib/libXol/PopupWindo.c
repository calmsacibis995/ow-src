#pragma ident	"@(#)PopupWindo.c	302.20	97/03/26 lib/libXol SMI"	/* popupwindo:src/PopupWindo.c 1.36	*/

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
 * Description:	Open Look Popup Window widget.
 *		
 *******************************file*header*******************************
 */


#include <ctype.h>
#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/ButtonP.h>
#include <Xol/ControlArP.h>
#include <Xol/FooterPane.h>
#include <Xol/Menu.h>
#include <Xol/OblongButP.h>
#include <Xol/OlI18nP.h>
#include <Xol/OlStrMthdsI.h>
#include <Xol/OpenLookP.h>
#include <Xol/PopupWindP.h>
#include <Xol/VendorI.h>


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
static void	PopdownCB(Widget nw, XtPointer closure, XtPointer call_data);
static void	PopupCB(Widget nw, XtPointer closure, XtPointer call_data);
static void	CheckCallback(PopupWindowShellWidget parent,
	String callback_name, ArgList orig_args, Cardinal num_args,
	String label, char mnemonic);

					/* class procedures		*/
static Boolean	ActivateWidget (Widget,OlVirtualName,XtPointer);
static void	WMMsgHandler (Widget, OlDefine, OlWMProtocolVerify *);
static void	ClassInitialize(void);
static void	Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);
static void   	InitializeHook(Widget new, ArgList args, Cardinal *pnum_args);
static void 	Realize(Widget w, Mask *valueMask, XSetWindowAttributes *attributes);
static void 	Destroy(Widget widget);

					/* action procedures		*/

					/* public procedures		*/
void	_OlPopupWiShPopDown(Widget w, Boolean override_pushpin);


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************
 */


/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */


static XtResource resources[] =
{
	/* XtNtextFormat should always be the first resource */
        {XtNtextFormat, XtCTextFormat, XtROlStrRep,sizeof(OlStrRep),
	  XtOffset(PopupWindowShellWidget, popupwindow.text_format),
	  XtRCallProc,(XtPointer)_OlGetDefaultTextFormat},

	{ XtNupperControlArea, XtCUpperControlArea, XtRPointer, sizeof(Widget), 
		XtOffset(PopupWindowShellWidget, popupwindow.upperControlArea), 
		XtRPointer, NULL },

	{ XtNlowerControlArea, XtCLowerControlArea, XtRPointer, sizeof(Widget),
		XtOffset(PopupWindowShellWidget, popupwindow.lowerControlArea), 
		XtRPointer, NULL },

	{ XtNfooterPanel, XtCFooterPanel, XtRPointer, sizeof(Widget),
		XtOffset(PopupWindowShellWidget, popupwindow.footerPanel), 
		XtRPointer, NULL },

	{ XtNapply, XtCCallback, XtRCallback, sizeof(XtPointer),
		XtOffset(PopupWindowShellWidget, popupwindow.apply),
		XtRCallback, (XtPointer)NULL},

	{ XtNsetDefaults, XtCCallback, XtRCallback, sizeof(XtPointer),
		XtOffset(PopupWindowShellWidget, popupwindow.setDefaults),
		XtRCallback, (XtPointer)NULL},

	{ XtNreset, XtCCallback, XtRCallback, sizeof(XtPointer),
		XtOffset(PopupWindowShellWidget, popupwindow.reset),
		XtRCallback, (XtPointer)NULL},

	{ XtNresetFactory, XtCCallback, XtRCallback, sizeof(XtPointer),
		XtOffset(PopupWindowShellWidget, popupwindow.resetFactory),
		XtRCallback, (XtPointer)NULL},

	{ XtNverify, XtCCallback, XtRCallback, sizeof(XtPointer),
		XtOffset(PopupWindowShellWidget, popupwindow.verify), 
		XtRCallback, NULL },

	{ XtNpropertyChange, XtCPropertyChange, XtRBoolean, sizeof(Boolean),
		XtOffset(PopupWindowShellWidget, popupwindow.propchange), 
		XtRBoolean, False },

#ifdef XGETTEXT
	{ XtNapplyLabel, XtCApplyLabel, XtROlStr, sizeof(OlStr),
		XtOffset(PopupWindowShellWidget, popupwindow.applyLabel),
		XtRString, 
		(XtPointer) dgettext(OlMsgsDomain, "Apply") },

	{ XtNsetDefaultsLabel, XtCSetDefaultsLabel, 
		XtROlStr, sizeof(OlStr),
		XtOffset(PopupWindowShellWidget, 
		popupwindow.setDefaultsLabel),
		XtRString, 
		(XtPointer) dgettext(OlMsgsDomain,"Set Defaults") },

	{ XtNresetLabel, XtCResetLabel, XtROlStr, sizeof(OlStr),
		XtOffset(PopupWindowShellWidget, popupwindow.resetLabel),
		XtRString, (XtPointer) dgettext(OlMsgsDomain,"Reset") },

	{ XtNresetFactoryLabel, XtCResetFactoryLabel, 
		XtROlStr, sizeof(OlStr),
		XtOffset(PopupWindowShellWidget, 
		popupwindow.resetFactoryLabel),
		XtRString, 
		(XtPointer)dgettext(OlMsgsDomain, "Reset To Factory") },

	{ XtNmenuTitle, XtCMenuTitle, XtROlStr, 
		sizeof(OlStr), XtOffset(PopupWindowShellWidget, 
		popupwindow.menuTitle),
		XtRString, 
		(XtPointer) dgettext(OlMsgsDomain,"Settings") },
#else
	{ XtNapplyLabel, XtCApplyLabel, XtROlStr, sizeof(OlStr),
		XtOffset(PopupWindowShellWidget, popupwindow.applyLabel),
		XtRLocaleString, (XtPointer) "Apply" },

	{ XtNsetDefaultsLabel, XtCSetDefaultsLabel, 
		XtROlStr, sizeof(OlStr),
		XtOffset(PopupWindowShellWidget, 
		popupwindow.setDefaultsLabel), XtRLocaleString, 
		(XtPointer) "Set Defaults" },

	{ XtNresetLabel, XtCResetLabel, XtROlStr, sizeof(OlStr),
		XtOffset(PopupWindowShellWidget, popupwindow.resetLabel),
		XtRLocaleString, (XtPointer) "Reset" },

	{ XtNresetFactoryLabel, XtCResetFactoryLabel, 
		XtROlStr, sizeof(OlStr),
		XtOffset(PopupWindowShellWidget, 
		popupwindow.resetFactoryLabel),
		XtRLocaleString, (XtPointer) "Reset To Factory" },

	{ XtNmenuTitle, XtCMenuTitle, XtROlStr, 
		sizeof(OlStr), XtOffset(PopupWindowShellWidget, 
		popupwindow.menuTitle),
		XtRLocaleString, (XtPointer) "Settings" },
#endif

	{ XtNapplyMnemonic, XtCApplyMnemonic, OlRChar, 
		sizeof(char), XtOffset(PopupWindowShellWidget, 
		popupwindow.applyMnemonic),
		OlRChar, (XtPointer) "A" },

	{ XtNsetDefaultsMnemonic, XtCSetDefaultsMnemonic, OlRChar, 
		sizeof(char), XtOffset(PopupWindowShellWidget, 
		popupwindow.setDefaultsMnemonic),
		OlRChar, (XtPointer) "S" },

	{ XtNresetMnemonic, XtCResetMnemonic, OlRChar, 
		sizeof(char), XtOffset(PopupWindowShellWidget, 
		popupwindow.resetMnemonic),
		OlRChar, (XtPointer) "R" },

	{ XtNresetFactoryMnemonic, XtCResetFactoryMnemonic, OlRChar, 
		sizeof(char), XtOffset(PopupWindowShellWidget, 
		popupwindow.resetFactoryMnemonic),
		OlRChar, (XtPointer) "F" },
    	{ XtNpointerWarping, XtCPointerWarping, XtRBoolean, 
		sizeof(Boolean), XtOffset(PopupWindowShellWidget,
		popupwindow.warp_pointer),XtRImmediate, (XtPointer)OL_POINTER_WARPING }
};
#undef OFFSET

/*
 *************************************************************************
 *
 * Define Class Extension Resource List
 *
 *************************************************************************
 */
#define OFFSET(field)	XtOffsetOf(OlVendorPartExtensionRec, field)

static XtResource
ext_resources[] = {
	{ XtNmenuButton, XtCMenuButton, XtRBoolean, sizeof(Boolean),
	  OFFSET(menu_button), XtRImmediate, (XtPointer)False },

	{ XtNpushpin, XtCPushpin, XtROlDefine, sizeof(OlDefine),
	  OFFSET(pushpin), XtRImmediate, (XtPointer)OL_OUT },

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

/*
 *************************************************************************
 *
 * Define Class Extension Records
 *
 *************************************************************************
 */
static OlVendorClassExtensionRec
vendor_extension_rec = {
    {
        NULL,                                   /* next_extension       */
        NULLQUARK,                              /* record_type          */
        OlVendorClassExtensionVersion,          /* version              */
        sizeof(OlVendorClassExtensionRec)       /* record_size          */
    },  /* End of OlClassExtension header       */
        ext_resources,                          /* resources            */
        XtNumber(ext_resources),                /* num_resources        */
        NULL,                                   /* private              */
        NULL,                                   /* set_default          */
	NULL,					/* get_default 		*/
	NULL,					/* destroy		*/
        NULL,                                   /* initialize           */
        NULL,                                   /* set_values           */
        NULL,                                   /* get_values           */
        XtInheritTraversalHandler,              /* traversal_handler    */
        XtInheritHighlightHandler,  		/* highlight_handler    */
        ActivateWidget, 			/* activate function    */
        NULL,              			/* event_procs          */
        0,              			/* num_event_procs      */
        NULL,					/* part_list            */
	{ NULL, 0 },				/* dyn_data		*/
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

PopupWindowShellClassRec popupWindowShellClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &transientShellClassRec,
    /* class_name         */    "PopupWindowShell",
    /* size               */    sizeof(PopupWindowShellRec),
    /* Class Initializer  */    ClassInitialize,
    /* class_part_initialize*/	NULL,
    /* Class init'ed ?    */	FALSE,
    /* initialize         */    (XtInitProc) Initialize,
    /* initialize_hook    */	InitializeHook,
    /* realize            */    Realize,
    /* actions            */    NULL,
    /* num_actions        */    0,
    /* resources          */    resources,
    /* resource_count     */	XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion    */    FALSE,
    /* compress_exposure  */    TRUE,
    /* compress_enterleave*/	FALSE,
    /* visible_interest   */    FALSE,
    /* destroy            */    Destroy,
    /* resize             */    XtInheritResize,
    /* expose             */    NULL,
    /* set_values         */    NULL,
    /* set_values_hook    */	NULL,			
    /* set_values_almost  */	XtInheritSetValuesAlmost,  
    /* get_values_hook    */	NULL,			
    /* accept_focus       */    XtInheritAcceptFocus,
    /* intrinsics version */	XtVersion,
    /* callback offsets   */    NULL,
    /* tm_table           */    XtInheritTranslations,
    /* query_geometry     */    NULL,
  },{
/* composite_class fields */
    /* geometry_manager   */    XtInheritGeometryManager,
    /* change_managed     */    XtInheritChangeManaged,
    /* insert_child	  */	XtInheritInsertChild,
    /* delete_child	  */	XtInheritDeleteChild,
    /* extension	  */    NULL
  },{
/* shell_class fields 	*/
    /* grumble		*/	0
  },{
/* wm_shell_class fields*/
    /* mumble		*/	0
  },{
/* vendor_shell_class fields*/
    /* tumble           */	(XtPointer)&vendor_extension_rec
  },{
/* tranisent_shell_class fields*/
    /* stumble		*/	0
  },{
/* popup_shell_class fields*/
    /* fumble		*/	0
  }
};


/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass popupWindowShellWidgetClass = (WidgetClass)&popupWindowShellClassRec;

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 *
 * _OlPopupWiShPopDown(w, override_pushpin)
 *
 * Given a widget, find its shell and try the shell's verify callbacks.
 * If the verify succeeds, call the dismiss callbacks.  Then check the 
 * shell's pushpin state, and pop down the shell if pin is out.  
 *
 * If override_pushpin is true, pull the pin out, call dismiss callbacks,
 * and pop down the shell without verifying.
 *
 ****************************procedure*header*****************************
 */

void
_OlPopupWiShPopDown(Widget w, Boolean override_pushpin)
{
	Widget 			shellw;
	Boolean			verifyok = True;
	OlVendorPartExtension	part;

	for(shellw = w; shellw; shellw = shellw->core.parent) {
		if (XtIsSubclass(shellw, popupWindowShellWidgetClass))
			break;
	}

	if (!shellw) {
		OlWarning (dgettext(OlMsgsDomain,
				"Popup window can't pop down!"));
		return;
	}

	if (override_pushpin == True) {
		Arg args[1];
		XtSetArg(args[0], XtNpushpin, OL_OUT);
		XtSetValues(shellw, args, 1);
/* Need to call verify callbacks next */
/*
		XtPopdown(shellw);
		return;
*/
	}

	if (((PopupWindowShellWidget) shellw)->popupwindow.verify) {
		XtCallCallbacks(shellw, XtNverify, &verifyok);
		if (verifyok == False)
			return;
	}

 	part = _OlGetVendorPartExtension(shellw);

	switch (part->pushpin) {
		case OL_IN:
			break;
		case OL_OUT:
		default:
			XtPopdown(shellw);
			break;
	}
}

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/******************************function*header****************************
 * PopdownCB():  called when PopupWindo is popped down.
 */

static void
PopdownCB(Widget w, XtPointer closure, XtPointer call_data)
{
    PopupWindowShellWidget pwsw = (PopupWindowShellWidget)w;
    PopupWindowShellPart * pwswPart = &(pwsw->popupwindow);

    if (pwswPart->warp_pointer && pwswPart->do_unwarp)
	/* restore position of pointer */
	XWarpPointer(XtDisplay(pwsw), None,
		     RootWindowOfScreen(XtScreen(pwsw)), 0, 0, 0, 0,
		     pwswPart->root_x, pwswPart->root_y);

}

static void
warpPointer(
		Widget w, 
		XtPointer client_data, 
		XEvent *event, 
		Boolean *cont_to_dispatch) 
{
	int     	dest_x, dest_y;
	Dimension	height, width;
	Widget		fw = (Widget)client_data;
	Boolean		busy;

	if(event->type 	!= MapNotify ||	
		event->xmap.window != XtWindowOfObject(w))
		return;

	XtVaGetValues(fw, XtNwidth, &width, XtNheight, &height, NULL);
	dest_x = (int)width/2;
	dest_y = ((int)height >= 1 ? (int)height/2: 0);

	if(_OlIsGadget(fw))	{ /* Transform to parents coordinates
				as a gadget does not have its own window. */
		Position x,y;

		XtVaGetValues(fw,XtNx,&x, XtNy, &y, NULL);
		dest_x += (int)x;
		dest_y += (int)y;
	}

	XWarpPointer(XtDisplay(w), None, 
			XtWindowOfObject(fw), 0, 0, 0, 0, dest_x, dest_y);
	OlCallAcceptFocus(fw,CurrentTime);
	/* StructureNotifyMask must be selected for by 
	 *	PopupWindowSehll widget: so use Raw. */
	XtRemoveRawEventHandler(w, StructureNotifyMask, False, 
					warpPointer, client_data);

	/* If busy then show it */
	XtVaGetValues( w, XtNbusy, &busy, NULL );
	SetWMWindowBusy(XtDisplay(w), XtWindow(w), (busy == True ?
					WMWindowIsBusy : WMWindowNotBusy));

}
	

/******************************function*header****************************
 * PopupCB():  called when PopupWindo is popped up.
 */

static void
PopupCB(Widget w, XtPointer closure, XtPointer call_data)
{
    PopupWindowShellWidget   pwsw = (PopupWindowShellWidget)w;
    PopupWindowShellPart *	pwswPart = &(pwsw->popupwindow);
    Widget		focus_widget = NULL;

    XtVaGetValues(w, XtNfocusWidget, &focus_widget, NULL);
    if(focus_widget == (Widget)NULL)
	focus_widget = w;

    if (pwswPart->warp_pointer)
    {
	Window		junkWin;
	int		junkPos;
	unsigned int	junkMask;

	if(!XtIsRealized(w))
		XtRealizeWidget(w);
	/* save pointer position (to be restored later) */
	XQueryPointer(XtDisplay(pwsw), XtWindow(pwsw), &junkWin, &junkWin,
		      &(pwswPart->root_x), &(pwswPart->root_y),
		      &junkPos, &junkPos, &junkMask);

	/* StructureNotifyMask must be selected for by 
	 *	PopupWindowSehll widget: so use Raw. */
	XtAddRawEventHandler(w, StructureNotifyMask, 
				False, warpPointer, (XtPointer)focus_widget);

	pwswPart->do_unwarp = True;
    }
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
 *
 *  Initialize
 *
 *	Add an event handler to intercept client messages.
 * 
 ****************************procedure*header*****************************
 */

/* ARGSUSED */
static void 
Initialize (Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    PopupWindowShellWidget new_pwsw = (PopupWindowShellWidget)new;
    PopupWindowShellPart	*pwp = &(new_pwsw->popupwindow);
    OlStrRep	tf 	= pwp->text_format;
    OlStr (*StrCpy)(OlStr s1, OlStr s2) = 
			str_methods[tf].StrCpy;
    int (*StrNumBytes)(OlStr s) =
			str_methods[tf].StrNumBytes;

	/* make a personal copy */
   pwp->menuTitle = StrCpy((OlStr)XtMalloc(StrNumBytes(
					pwp->menuTitle) ),
					pwp->menuTitle);
   pwp->applyLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					pwp->applyLabel) ),
					pwp->applyLabel);
   pwp->setDefaultsLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					pwp->setDefaultsLabel) ),
					pwp->setDefaultsLabel);
   pwp->resetLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					pwp->resetLabel) ),
					pwp->resetLabel);
   pwp->resetFactoryLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					pwp->resetFactoryLabel) ),
					pwp->resetFactoryLabel);
    new_pwsw->popupwindow.menu = NULL;
    XtAddCallback((Widget)new_pwsw, XtNpopupCallback, PopupCB, NULL);
    XtAddCallback((Widget)new_pwsw, XtNpopdownCallback, PopdownCB, NULL);
} /* Initialize */

/*
 *************************************************************************
 *
 * InitializeHook:
 *
 *	Create upper and lower control areas.
 *	Set up "post-select" routine to check pushpin.
 * 
 ****************************procedure*header*****************************
 */

static void
InitializeHook(Widget new, ArgList args, Cardinal *pnum_args)
{
        PopupWindowShellWidget new_pwsw = (PopupWindowShellWidget)new;
	Widget		mastercontrol, panel;
	Widget		uppercontrol, lowercontrol;
	MaskArg		maskargs[20];
	Arg		localargs[20];
	ArgList		mergedargs;
	Cardinal	i,j,mergedcnt;
	static XtCallbackRec cb[] = {
		{ (XtCallbackProc) _OlPopupWiShPopDown, (XtPointer) False },
		{ NULL, NULL}
	};
	PopupWindowShellPart *pw = &(new_pwsw->popupwindow);


/*
**  Make footer panel
*/

	i = 0;
	XtSetArg(localargs[i], XtNtextFormat, pw->text_format); i++;
	panel = XtCreateManagedWidget(	"panel",
					footerPanelWidgetClass, 
					(Widget) new_pwsw, localargs, i);
/*
**  Make main control area
*/

	i = 0;
	XtSetArg(localargs[i], XtNlayoutType, OL_FIXEDCOLS);	i++;
	XtSetArg(localargs[i], XtNcenter, True);		i++;
	XtSetArg(localargs[i], XtNsameSize, OL_NONE);		i++;
	XtSetArg(localargs[i], XtNmeasure, 1);			i++;
	XtSetArg(localargs[i], XtNhPad, 0);			i++;
	XtSetArg(localargs[i], XtNvPad, 0);			i++;
	XtSetArg(localargs[i], XtNhSpace, 0);			i++;
	XtSetArg(localargs[i], XtNvSpace, 0);			i++;
	XtSetArg(localargs[i], XtNborderWidth, 0);		i++;

	mastercontrol = XtCreateManagedWidget(	"control",
						controlAreaWidgetClass, 
						panel,
						localargs,
						i);
/*
**  Make child control areas.  Inherit application resources from parent's
**  arg list, if specified.  Note that some default values are different
**  for the two control areas.
*/
	i = 0;
	_OlSetMaskArg(	maskargs[i], XtNalignCaptions, 
			True, OL_DEFAULT_PAIR);				i++;
	_OlSetMaskArg(maskargs[i], XtNcenter, FALSE, OL_DEFAULT_PAIR);	i++;
	_OlSetMaskArg(maskargs[i], XtNhPad, 0, OL_SOURCE_PAIR);		i++;
	_OlSetMaskArg(maskargs[i], XtNhSpace, 0, OL_SOURCE_PAIR);	i++;
	_OlSetMaskArg(	maskargs[i], XtNlayoutType, 
			OL_FIXEDCOLS, OL_DEFAULT_PAIR);			i++;
	_OlSetMaskArg(maskargs[i], XtNmeasure, 1, OL_DEFAULT_PAIR);	i++;
	_OlSetMaskArg(maskargs[i], XtNsameSize, 0, OL_SOURCE_PAIR);	i++;
	_OlSetMaskArg(maskargs[i], XtNvPad, 0, OL_SOURCE_PAIR);		i++;
	_OlSetMaskArg(maskargs[i], XtNvSpace, 0, OL_SOURCE_PAIR);	i++;
	_OlSetMaskArg(	maskargs[i], XtNborderWidth, 
			0, OL_OVERRIDE_PAIR);				i++;

	_OlComposeArgList(	args, *pnum_args, maskargs, 
				i, &mergedargs, &mergedcnt);

	uppercontrol = XtCreateManagedWidget(	"upper",
						controlAreaWidgetClass, 
						mastercontrol,
						mergedargs,
						mergedcnt);

	XtFree( (char *) mergedargs);

	i = 0;
	_OlSetMaskArg(	maskargs[i], XtNalignCaptions, 
			0, OL_SOURCE_PAIR);				i++;
	_OlSetMaskArg(maskargs[i], XtNcenter, 0, OL_SOURCE_PAIR);	i++;
	_OlSetMaskArg(maskargs[i], XtNhPad, 0, OL_SOURCE_PAIR);		i++;
	_OlSetMaskArg(maskargs[i], XtNhSpace, 0, OL_SOURCE_PAIR);	i++;
	_OlSetMaskArg(	maskargs[i], XtNlayoutType, 
			OL_FIXEDROWS, OL_DEFAULT_PAIR);			i++;
	_OlSetMaskArg(maskargs[i], XtNmeasure, 1, OL_DEFAULT_PAIR);	i++;
	_OlSetMaskArg(maskargs[i], XtNsameSize, 0, OL_SOURCE_PAIR);	i++;
	_OlSetMaskArg(maskargs[i], XtNvPad, 0, OL_SOURCE_PAIR);		i++;
	_OlSetMaskArg(maskargs[i], XtNvSpace, 0, OL_SOURCE_PAIR);	i++;
	_OlSetMaskArg(maskargs[i], XtNborderWidth, 0, OL_OVERRIDE_PAIR);i++;
	_OlSetMaskArg(maskargs[i], XtNpostSelect, cb, OL_OVERRIDE_PAIR);i++;

	_OlComposeArgList(args, *pnum_args, maskargs, 
				i, &mergedargs, &mergedcnt);

	lowercontrol = XtCreateManagedWidget(	"lower",
						controlAreaWidgetClass, 
						mastercontrol,
						mergedargs,
						mergedcnt);

	XtFree( (char *) mergedargs);

	pw->upperControlArea = uppercontrol;
	pw->lowerControlArea = lowercontrol;
	pw->footerPanel = panel;

/*
** check for callbacks which need automatic buttons, and add the
** buttons if necessary.
*/

	CheckCallback(new_pwsw, XtNapply, args, *pnum_args, 
		pw->applyLabel, pw->applyMnemonic);
	CheckCallback(new_pwsw, XtNsetDefaults, args, *pnum_args, 
		pw->setDefaultsLabel, pw->setDefaultsMnemonic);
	CheckCallback(new_pwsw, XtNreset, args, *pnum_args, 
		pw->resetLabel, pw->resetMnemonic);
	CheckCallback(new_pwsw, XtNresetFactory, args, *pnum_args, 
		pw->resetFactoryLabel, pw->resetFactoryMnemonic);
}

/*
 *************************************************************************
 * ActivateWidget - this procedure allows external forces to activate this
 * widget indirectly.
 ****************************procedure*header*****************************
*/
/* ARGSUSED */
static Boolean
ActivateWidget (Widget w, OlVirtualName type, XtPointer call_data)
{
	Boolean			consumed = False;
	OlVendorPartExtension	part;

	part = _OlGetVendorPartExtension(w);
	switch (type)
	{
		case OL_DEFAULTACTION:
			consumed = TRUE;
			if ((w = _OlGetDefault(w)) != (Widget)NULL)
				(void) OlActivateWidget(w, OL_SELECTKEY,
						(XtPointer)NULL);
			break;
		case OL_CANCEL:
			consumed = TRUE;
			if (part->pushpin == OL_IN)
				_OlSetPinState(w, OL_OUT);
			XtPopdown(w);
			break;
		case OL_TOGGLEPUSHPIN:
			consumed = TRUE;
			if (part->pushpin == OL_OUT)
				_OlSetPinState(w, OL_IN);
			else
			{
				_OlSetPinState(w, OL_OUT);
				XtPopdown(w);
			}
			break;
		default:
			break;
	}
	return(consumed);
} /* END OF ActivateWidget() */

static void
WMMsgHandler (Widget w, OlDefine action, OlWMProtocolVerify *wmpv)
{
	if (wmpv->msgtype == OL_WM_DELETE_WINDOW) {
		switch(action) {
		case OL_DEFAULTACTION:
		case OL_DISMISS:
			_OlPopupWiShPopDown(w,True);
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

static void
ClassInitialize(void)
{
	vendor_extension->header.record_type = OlXrmVendorClassExtension;
}

static void 
Realize(Widget w, Mask *valueMask, XSetWindowAttributes *attributes)
{
        PopupWindowShellWidget pwsw = (PopupWindowShellWidget)w;
	Window		win;
	Display *	dpy;
	XWMHints *	hintp = &pwsw->wm.wm_hints;

	w->core.border_width = 0;

	(*popupWindowShellClassRec.core_class.superclass->core_class.realize)
		(w, valueMask, attributes);

	if(w->core.parent) {
		Widget	ptr;

		win = XtWindow(w);
		dpy = XtDisplay(w);
		for (	ptr = w->core.parent; 
			ptr->core.parent;
			ptr = ptr->core.parent) {
			}
		hintp->window_group = XtWindow(ptr);
		hintp->flags |=  WindowGroupHint;
		XSetWMHints(dpy, win, hintp);
	}
}

static void
Destroy(Widget widget)
{
PopupWindowShellPart   *pwp = &(((PopupWindowShellWidget)widget)->popupwindow);

	XtRemoveAllCallbacks(widget,XtNverify);
	XtRemoveAllCallbacks(widget,XtNapply);
	XtRemoveAllCallbacks(widget,XtNsetDefaults);
	XtRemoveAllCallbacks(widget,XtNreset);
	XtRemoveAllCallbacks(widget,XtNresetFactory);
	if (pwp->menuTitle)
		XtFree(pwp->menuTitle);
	if (pwp->applyLabel)
		XtFree(pwp->applyLabel);
	if (pwp->setDefaultsLabel)
		XtFree(pwp->setDefaultsLabel);
	if (pwp->resetLabel)
		XtFree(pwp->resetLabel);
	if (pwp->resetFactoryLabel)
		XtFree(pwp->resetFactoryLabel);
}

static void
CheckCallback(PopupWindowShellWidget parent,
	String callback_name, ArgList orig_args, Cardinal num_args,
	String label, char mnemonic)
{
	Arg args[10];
	int i, j;
	static Widget menupane;
	static Arg queryMenuPane[] = {
		{ XtNmenuPane,	(XtArgVal) &menupane}
	};
	Widget	child;
	OblongButtonGadget yabba;
	ControlAreaWidget lca;
	static XtCallbackRec cb[] = {
		{ (XtCallbackProc) _OlPopupWiShPopDown, (XtPointer) False },
		{ NULL, NULL}
	};

	lca = (ControlAreaWidget) parent->popupwindow.lowerControlArea;

/*
**  Does the named callback exist in the arg list?
*/

	for (i = 0; i < num_args; i++) {
		if (!strcmp (orig_args[i].name, callback_name))
			break;
	}

	if (i == num_args)
		return;

/*
** Add the appropriate button to the lower control area
*/
	j = 0;
	XtSetArg(args[j], XtNmnemonic, mnemonic);		j++;
	XtSetArg(args[j], XtNtextFormat, parent->popupwindow.text_format); j++;
	XtSetArg(args[j], XtNlabel, label); j++;
	yabba = (OblongButtonGadget) 
		XtCreateManagedWidget( label, oblongButtonGadgetClass, 
					(Widget) lca, args, j);

	XtAddCallbacks( (Widget) yabba, XtNselect,
		       (XtPointer)orig_args[i].value);

/*
** Add an entry to the popup menu, first creating the menu if it
** doesn't already exist.
*/

	if (!parent->popupwindow.menu) {
		j = 0;
		XtSetArg(args[j], XtNpushpin, OL_NONE);		j++;
		XtSetArg(args[j], XtNshellTitle, parent->popupwindow.menuTitle); j++;
		XtSetArg(args[j], XtNpostSelect, cb);		j++;
	XtSetArg(args[j], XtNtextFormat, parent->popupwindow.text_format); j++;

		parent->popupwindow.menu = 
			XtCreatePopupShell(	"First Menu", 
						menuShellWidgetClass,
						(Widget) parent, args, j);

	}

	XtGetValues(	parent->popupwindow.menu, queryMenuPane, 
			XtNumber(queryMenuPane));

	j = 0;
	XtSetArg(args[j], XtNmnemonic, mnemonic);		j++;
	XtSetArg(args[j], XtNtextFormat, parent->popupwindow.text_format); j++;
	XtSetArg(args[j], XtNlabel, label); j++;
	yabba = (OblongButtonGadget) 
		XtCreateManagedWidget( label, oblongButtonGadgetClass,
					menupane, args, j);

	XtAddCallbacks( (Widget) yabba, XtNselect, 
		(XtCallbackList) orig_args[i].value);
}

/*
**  SetValues() -- Should look for changes to the callback lists and add
**  buttons as necessary.
*/

