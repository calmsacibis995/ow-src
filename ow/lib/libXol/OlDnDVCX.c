#pragma ident	"@(#)OlDnDVCX.c	302.26	97/03/26 lib/libXol SMI"	/* OLIT	*/

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

/*
 *************************************************************************
 *
 * Description:
 *      This file contains the routines necessary to manipulate the
 *	the new drag and drop protocol for OpenWindows. it is implemented
 *	as a class extension to the Vendor vendor found in Vendor.c.
 *
 ******************************file*header********************************
 */


#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>

#include <X11/IntrinsicI.h>
#include <X11/StringDefs.h>
#include <X11/EventI.h>
#include <X11/ShellP.h>
#include <X11/Xatom.h>


#ifdef	DEBUG	/* lets be able to interrogate the Regions ... */
	#include <X11/poly.h>
	#include <X11/region.h>
#endif	/* DEBUG */


#include <Xol/Error.h>
#include <Xol/OlDnDVCXP.h> /* class extension for new drag n drop stuff */
#include <Xol/OpenLookP.h>
#include <Xol/RootShell.h>
#include <Xol/VendorI.h>


/* borrowed from Vendor.c */

#define CLASSNAME(w)    (XtClass((Widget)(w))->core_class.class_name)
#define VCLASS(wc, r)   (((VendorShellWidgetClass)(wc))->vendor_shell_class.r)

#define	VECTORFULL(n, incr)	((n) == (((n) / (incr)) * (incr)))

#define NULL_EXTENSION          ((OlDnDVendorClassExtension)NULL)
#define NULL_PART_PTR           ((OlDnDVendorPartExtension*)NULL)
#define NULL_PART               ((OlDnDVendorPartExtension)NULL)

/*************************** forward decls for Drag and Drop **************/

/* public procedures */

/* class methods */

static void 	DnDVCXClassInitialize (OlDnDVendorClassExtension);
static void	DnDVCXClassPartInitialize ( WidgetClass );

static Boolean  DnDVCXSetValues (Widget, Widget, Widget,
					 ArgList, Cardinal *,
					 OlDnDVendorPartExtension,
					 OlDnDVendorPartExtension,
					 OlDnDVendorPartExtension
);

static void     DnDVCXGetValues (Widget, ArgList, Cardinal *,
					 OlDnDVendorPartExtension
);

static void     DnDVCXInitialize (Widget, Widget, ArgList, Cardinal *,
					  OlDnDVendorPartExtension,
					  OlDnDVendorPartExtension
);

static void     DnDVCXPostRealizeSetup ( Widget,
                                                 OlDnDVendorClassExtension,
                                                 OlDnDVendorPartExtension 
);

static void     DnDVCXDestroy   (Widget, OlDnDVendorPartExtension
);
static Boolean	DnDVCXTriggerMessageDispatcher ( Widget,
						 OlDnDVendorPartExtension,
						 TriggerMessagePtr
);

static Boolean	DnDVCXPreviewMessageDispatcher ( Widget,
						 OlDnDVendorPartExtension,
						 PreviewMessagePtr
);

static OlDnDDropSiteID DnDVCXRegisterDropSite ( Widget,
						OlDnDVendorPartExtension,
						Widget,
						Window,
						OlDnDSitePreviewHints,
						OlDnDSiteRectPtr,
						unsigned int,
						OlDnDTriggerMessageNotifyProc,
						OlDnDPreviewMessageNotifyProc,
						Boolean,
						XtPointer
);

static Boolean	DnDVCXUpdateDropSiteGeometry ( Widget,
						OlDnDVendorPartExtension,
						OlDnDDropSiteID,
						OlDnDSiteRectPtr,
						unsigned int
);

static void    DnDVCXDeleteDropSite (Widget, OlDnDVendorPartExtension,
					     OlDnDDropSiteID
);

static Boolean DnDVCXQueryDropSiteInfo ( Widget,
						 OlDnDVendorPartExtension,
						 OlDnDDropSiteID,
						 Widget *,
						 Window *,
						 OlDnDSitePreviewHints *,
						 OlDnDSiteRectPtr *,
						 unsigned int *,
						 Boolean *
);

static void	DnDVCXAssertDropSiteRegistry ( Widget, 
					       OlDnDVendorPartExtension);

static void	DnDVCXDeleteDropSiteRegistry ( Widget,
					       OlDnDVendorPartExtension
);

static Boolean	DnDVCXFetchDSDMInfo ( Widget, OlDnDVendorPartExtension,
					      Time );

static Boolean	DnDVCXDeliverTriggerMessage ( Widget, 
					       OlDnDVendorPartExtension,
					       Widget, Window,
					       int, int, Atom,
					       OlDnDTriggerOperation, Time
);

static Boolean	DnDVCXDeliverPreviewMessage ( Widget, 
					       OlDnDVendorPartExtension,
					       Widget, Window, int, int,
					       Time,
					       OlDnDPreviewAnimateCallbackProc,
					       XtPointer
);

static Boolean	DnDVCXInitializeDragState ( Widget,
						     OlDnDVendorPartExtension
);

static void	DnDVCXClearDragState   	    ( Widget,
						       OlDnDVendorPartExtension );

static Atom	DnDVCXAllocTransientAtom    ( Widget,
                                                       OlDnDVendorPartExtension,
						       Widget
);

static void	DnDVCXFreeTransientAtom    ( Widget,
                                                      OlDnDVendorPartExtension,
						      Widget,
						      Atom
);

static DSSelectionAtomPtr DnDVCXAssocSelectionWithWidget ( Widget,
					              OlDnDVendorPartExtension,
                                                      Widget,
                                                      Atom,
						      Time,
						      OwnerProcClosurePtr
);

static void	DnDVCXDissassocSelectionWithWidget ( Widget,
					              OlDnDVendorPartExtension,
						      Widget,
                                                      Atom,
						      Time
);

static Atom     *DnDVCXGetCurrentSelectionsForWidget
			( Widget,
				   OlDnDVendorPartExtension,
				   Widget, Cardinal *
);

static Widget DnDVCXGetWidgetForSelection
				( Widget, OlDnDVendorPartExtension,
					   Atom, OwnerProcClosurePtr *
);

static Boolean DnDVCXChangeSitePreviewHints ( Widget, 
					               OlDnDVendorPartExtension,
					               OlDnDDropSiteID,
					               OlDnDSitePreviewHints );

static Boolean DnDVCXSetDropSiteOnInterest ( Widget,
						    OlDnDVendorPartExtension,
						    OlDnDDropSiteID,
						    Boolean, Boolean );

static void DnDVCXSetInterestInWidgetHier
			( Widget, OlDnDVendorPartExtension,
				   Widget, Boolean );

static void DnDVCXClipDropSites
			( Widget, OlDnDVendorPartExtension,
				   Widget );

/* event handlers */

static void     DnDVCXEventHandler(Widget widget, XtPointer client_data, XEvent *xevent, Boolean *continue_to_dispatch);   /* drag and drop handler */

/* private functions */

static DSSelectionAtomPtr _OlDnDAssocSelectionWithWidget
					( Widget,
						  OlDnDVendorPartExtension,
						  Widget,
						  Atom,
						  Time,
						  OwnerProcClosurePtr
);

static void 		  _OlDnDDisassocSelectionWithWidget
					( Widget,
						  OlDnDVendorPartExtension,
						  Widget,
						  Atom,
						  Time
);

static Widget
_OlDnDGetWidgetForSelection ( Widget,
				      OlDnDVendorPartExtension,
				      Atom, OwnerProcClosurePtr *
);

static Boolean
_OlDnDAtomIsTransient ( Widget, OlDnDVendorPartExtension, Atom);

void
_OlResourceDependencies ( XtResourceList *, Cardinal *,
				      XrmResourceList, Cardinal, Cardinal);

static void
_SetDisableClipping ( OlDnDVendorPartExtension,
			      OlDnDVendorPartExtension, Widget, Boolean );

static void _StopDnDTxTimer  ( OwnerProcClosurePtr );
static void _StartDnDTxTimer ( OwnerProcClosurePtr );

typedef	int	(*xErrorHandler) ();

static void	EnterProtectedSection ( Display *,
						XID,
						xErrorHandler );

static void	LeaveProtectedSection ( Display * );

static int	_PSErrorHandler (Display * , XErrorEvent * );

static void	GetShellAndParentOfWindow(Display *, Window,
					  Widget *, Widget *);

/*************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************/

#undef  OFFSET
#define OFFSET(field)   XtOffsetOf(OlDnDVendorPartExtensionRec, field)

static XtResource
dnd_ext_resources[] = {
    { XtNregistryUpdateTimestamp, XtCReadOnly, XtRInt, sizeof(Time),
        OFFSET(registry_update_timestamp), XtRImmediate, (XtPointer)NULL },

    { XtNnumberOfDropSites, XtCReadOnly, XtRInt, sizeof(int),
        OFFSET(number_of_sites), XtRImmediate, (XtPointer)NULL},

    { XtNrootX, XtCReadOnly, XtRPosition, sizeof(Position),
        OFFSET(root_x), XtRImmediate, (XtPointer)NULL},

    { XtNrootY, XtCReadOnly, XtRPosition, sizeof(Position),
        OFFSET(root_y), XtRImmediate, (XtPointer)NULL},

    { XtNautoAssertDropsiteRegistry, XtCAutoAssertDropsiteRegistry, XtRBoolean,
        sizeof(Boolean), OFFSET(auto_assert_dropsite_registry),
        XtRImmediate, (XtPointer)True},

    { XtNdoDSDMFetchesAsync, XtCDoDSDMFetchesAsync, XtRBoolean,
	sizeof(Boolean), OFFSET(do_dsdm_fetches_async), XtRImmediate,
	(XtPointer)False},

    { XtNdirty, XtCReadOnly, XtRBoolean, sizeof(Boolean),
        OFFSET(dirty), XtRImmediate, (XtPointer)False},

    { XtNpendingDSDMInfo, XtCReadOnly, XtRBoolean, sizeof(Boolean),
        OFFSET(pending_dsdm_info), XtRImmediate, (XtPointer)False},

    { XtNdsdmPresent, XtCReadOnly, XtRBoolean, sizeof(Boolean),
        OFFSET(dsdm_present), XtRImmediate, (XtPointer)False},

    { XtNdndTxTimeout, XtCDndTxTimeout, XtRInt, sizeof(unsigned long),
        OFFSET(dnd_tx_timeout), XtRImmediate, (XtPointer)0},

    { XtNdoingDrag, XtCReadOnly, XtRBoolean, sizeof(Boolean),
        OFFSET(doing_drag), XtRImmediate, (XtPointer)False},

    { XtNdsdmLastLoaded, XtCReadOnly, XtRInt, sizeof(Time),
        OFFSET(dsdm_last_loaded), XtRImmediate, (XtPointer)NULL},

    { XtNdefaultDropSiteID, XtCDropSiteID, XtRPointer, 
	sizeof(OlDnDDropSiteID), OFFSET(default_drop_site), XtRImmediate,
	(XtPointer)NULL},

    { XtNdisableDSClipping, XtCDisableDSClipping, XtRBoolean, sizeof(Boolean),
        OFFSET(disable_ds_clipping), XtRImmediate, (XtPointer)False},

    { XtNconfiguringWidget, XtCConfiguringWidget, XtRPointer, sizeof(Widget),
        OFFSET(configuring_widget), XtRImmediate, (XtPointer)NULL},
};
#undef  OFFSET

/*
 *************************************************************************
 *
 * Define Class Extension Records
 *
 *************************class*extension*records*************************
 */

/*static*/ /* we cant make it so since we initialize the extension chain
	    * at compile time in Vendor.c
	    */
OlDnDVendorClassExtensionRec dnd_vendor_extension_rec = {
	{
		NULL,					/* next_extension*/
		NULLQUARK,				/* record_type	*/
		OlDnDVendorClassExtensionVersion,	/* version	*/
		sizeof(OlDnDVendorClassExtensionRec)	/* record_size	*/
	},	/* End of OlClassExtension header */

	(Cardinal)sizeof(OlDnDVendorPartExtensionRec),	/* instance size */
	(OlDnDVendorPartExtension)NULL,			/* instance list */
	dnd_ext_resources,				/* resources 	 */
	XtNumber(dnd_ext_resources),			/* num_resources */

	DnDVCXClassInitialize,				/* class init    */
	DnDVCXClassPartInitialize,			/* class part    */
	(XtEnum)NULL,					/* inited 	 */
	DnDVCXSetValues,				/* set_values    */
	DnDVCXGetValues,				/* get_values	 */
	DnDVCXInitialize,				/* initialize	 */
	DnDVCXPostRealizeSetup,				/* post realize  */
	DnDVCXDestroy,					/* destroy 	 */

	DnDVCXTriggerMessageDispatcher,			/* TM dispatcher */
	DnDVCXPreviewMessageDispatcher,			/* PM dispatcher */

	DnDVCXRegisterDropSite,				/* register DS   */
	DnDVCXUpdateDropSiteGeometry,			/* update DS     */
	DnDVCXDeleteDropSite,				/* delete DS     */
	DnDVCXQueryDropSiteInfo,			/* query DS      */

	DnDVCXAssertDropSiteRegistry,			/* assert registry */
	DnDVCXDeleteDropSiteRegistry,			/* delete registry */

	DnDVCXFetchDSDMInfo,				/* fetch dsdm info */

	DnDVCXDeliverTriggerMessage,			/* deliver trigger */
	DnDVCXDeliverPreviewMessage,			/* deliver preview */

	DnDVCXInitializeDragState,			/* init drag state */
	DnDVCXClearDragState,				/* clear drag state */

	DnDVCXAllocTransientAtom,			/* alloc atom */
	DnDVCXFreeTransientAtom,			/* free atom  */

	DnDVCXAssocSelectionWithWidget,			/* assoc */
	DnDVCXDissassocSelectionWithWidget,		/* dissasoc */
	DnDVCXGetCurrentSelectionsForWidget,		/* list selections */
	DnDVCXGetWidgetForSelection,			/* find the widget */

	DnDVCXChangeSitePreviewHints,			/* change hints */

	DnDVCXSetDropSiteOnInterest,			/* set interest */
	DnDVCXSetInterestInWidgetHier,			/* set interest */

	DnDVCXClipDropSites,				/* apply trans  */
};

OlDnDVendorClassExtension  dnd_vendor_extension = &dnd_vendor_extension_rec;

XrmQuark OlXrmDnDVendorClassExtension = (XrmQuark)NULL;


/* prior to multiple display stuff ... */

Atom	_SUN_DRAGDROP_BEGIN;
Atom	_SUN_SELECTION_END;
Atom	_SUN_SELECTION_ERROR;

Atom	_SUN_AVAILABLE_TYPES;

Atom	_SUN_LOAD;
Atom	_SUN_DATA_LABEL;
Atom	_SUN_FILE_HOST_NAME;

Atom	_SUN_ENUMERATION_COUNT;
Atom	_SUN_ENUMERATION_ITEM;

Atom	_SUN_ALTERNATE_TRANSPORT_METHODS;
Atom	_SUN_LENGTH_TYPE;
Atom	_SUN_ATM_TOOL_TALK;
Atom	_SUN_ATM_FILE_NAME;

Atom	_SUN_DRAGDROP_INTEREST;
Atom	_SUN_DRAGDROP_PREVIEW;
Atom	_SUN_DRAGDROP_TRIGGER;
Atom	_SUN_DRAGDROP_DSDM;
Atom	_SUN_DRAGDROP_SITE_RECTS;
Atom	_SUN_DRAGDROP_ACK;
Atom	_SUN_DRAGDROP_DONE;

/************************************ public functions **********************/

/* internal toolkit functions follow */

/* note side effect */
#define	FetchPartExtensionIfNull(w, p)		\
	((p) != NULL_PART ? (p) : ((p) = _OlGetDnDVendorPartExtension((w))))

/* note side effect */
#define	FetchClassExtensionIfNull(w, c)		\
	((c) != NULL_EXTENSION ? (c) : 		\
	 ((c) = _OlGetDnDVendorClassExtension((w)->core.widget_class)))

/*******************************************
 *
 * _OlDnDInitialize
 *
 * called from InitializeOpenLook
 *
 * Initializes the appropriate atoms etc
 *
 *******************************************/

void
OlDnDInitialize (Display *dpy)
{
#define	INTERN(dpy, atom)	OlInternAtom(dpy, (const char *)atom)

	if (dpy == toplevelDisplay) {
		_SUN_DRAGDROP_BEGIN	= INTERN(dpy,
						 _SUN_DRAGDROP_BEGIN_NAME);
		_SUN_SELECTION_END	= INTERN(dpy,
						 _SUN_SELECTION_END_NAME);
		_SUN_SELECTION_ERROR	= INTERN(dpy,
						 _SUN_SELECTION_ERROR_NAME);

		_SUN_AVAILABLE_TYPES	= INTERN(dpy,
						 _SUN_AVAILABLE_TYPES_NAME);

		_SUN_LOAD		= INTERN(dpy,
						 _SUN_LOAD_NAME);
		_SUN_DATA_LABEL		= INTERN(dpy,
						 _SUN_DATA_LABEL_NAME);
		_SUN_FILE_HOST_NAME	= INTERN(dpy,
						 _SUN_FILE_HOST_NAME_NAME);

		_SUN_ENUMERATION_COUNT	= INTERN(dpy,
						 _SUN_ENUMERATION_COUNT_NAME);
		_SUN_ENUMERATION_ITEM	= INTERN(dpy,
						 _SUN_ENUMERATION_ITEM_NAME);

		_SUN_LENGTH_TYPE	= INTERN(dpy,
						 _SUN_LENGTH_TYPE_NAME);
		_SUN_ATM_TOOL_TALK	= INTERN(dpy,
						 _SUN_ATM_TOOL_TALK_NAME);
		_SUN_ATM_FILE_NAME	= INTERN(dpy,
						 _SUN_ATM_FILE_NAME_NAME);

		_SUN_DRAGDROP_INTEREST	= INTERN(dpy,
						 _SUN_DRAGDROP_INTEREST_NAME);
		_SUN_DRAGDROP_PREVIEW	= INTERN(dpy,
						 _SUN_DRAGDROP_PREVIEW_NAME);
		_SUN_DRAGDROP_TRIGGER	= INTERN(dpy,
						 _SUN_DRAGDROP_TRIGGER_NAME);
		_SUN_DRAGDROP_DSDM	= INTERN(dpy,
						 _SUN_DRAGDROP_DSDM_NAME);

		_SUN_DRAGDROP_SITE_RECTS = INTERN(dpy,
						 _SUN_DRAGDROP_SITE_RECTS_NAME);
		_SUN_DRAGDROP_ACK        = INTERN(dpy,
						 _SUN_DRAGDROP_ACK_NAME);
		_SUN_DRAGDROP_DONE       = INTERN(dpy,
						 _SUN_DRAGDROP_DONE_NAME);

		_SUN_ALTERNATE_TRANSPORT_METHODS =
			INTERN(dpy, _SUN_ALTERNATE_TRANSPORT_METHODS_NAME);
	}
#undef	INTERN
}

/*******************************************
 *
 * _OlDnDDoExtensionClassInit
 *
 * called from VendorShell ClassInitialize
 *
 * calls the Class Initialize proc for ext
 *
 *******************************************/

void
_OlDnDDoExtensionClassInit (OlDnDVendorClassExtension extension)
{
	if (extension->class_inited == (XtEnum)NULL &&
	    extension->class_initialize != (OlDnDVCXClassInitializeProc)NULL) {
		(*extension->class_initialize)(extension);
		extension->class_inited = True;
	}
}

/*******************************************
 *
 * _OlDnDDoExtensionClassPartInit
 *
 * called from VendorShell ClassInitialize
 *
 * calls the Clas Part Initialize proc for ext
 *******************************************/

static void
_recurse_ext_part_init (WidgetClass wc, WidgetClass ancestor)
{
	OlDnDVendorClassExtension	ext;

	if (ancestor != (WidgetClass)NULL &&
	    ancestor != vendorShellWidgetClass)
		_recurse_ext_part_init(wc, ancestor->core_class.superclass);

	ext = _OlGetDnDVendorClassExtension(ancestor);

	if (ext != (OlDnDVendorClassExtension)NULL &&
	    ext->class_part_initialize != (OlDnDVCXClassPartInitializeProc)NULL)
		(*ext->class_part_initialize)(wc);
	
}
void
_OlDnDDoExtensionClassPartInit (WidgetClass wc)
{
	_recurse_ext_part_init(wc, wc);
}

/*******************************************
 *
 * _OlDnDCallDnDVCXPostRealizeSetup
 *
 * called from VendorShell ClassInitialize
 *
 * should be called from vendor realize
 * after window is created ....
 *
 *******************************************/

void
_OlDnDCallVCXPostRealizeSetup (Widget vendor)
{
	OlDnDVendorPartExtension	dnd_part = NULL_PART;
	OlDnDVendorClassExtension	dnd_class = NULL_EXTENSION;

	FetchPartExtensionIfNull(vendor, dnd_part);
	FetchClassExtensionIfNull(vendor, dnd_class);

	if ((dnd_class != NULL_EXTENSION && dnd_class->post_realize_setup != 
	    (OlDnDVCXPostRealizeSetupProc)NULL) && dnd_part != NULL_PART)
		(*dnd_class->post_realize_setup)(vendor, dnd_class, dnd_part);
}

/**************************************************
 *
 * _OlGetDnDVendorClassExtension
 *
 * fetch the drag and drop class extension record
 *
 **************************************************/

OlDnDVendorClassExtension
_OlGetDnDVendorClassExtension (WidgetClass wc)
{
	OlDnDVendorClassExtension	ext = NULL_EXTENSION;

	if (!wc) {
		OlWarning(dgettext(OlMsgsDomain, 
		"_OlGetDnDVendorClassExtension: NULL WidgetClass pointer"));
	} else {
		ext = GET_DND_EXT(wc);
	}
	return(ext);
} /* END OF _OlGetDnDVendorClassExtension() */

/**************************************************
 *
 * _OlGetDnDVendorPartExtension
 *
 * fetch the drag and drop part extension record
 *
 **************************************************/

OlDnDVendorPartExtension
_OlGetDnDVendorPartExtension (Widget w)
{
	OlDnDVendorPartExtension	part = NULL_PART;

	if (w == (Widget)NULL) {
		OlWarning(dgettext(OlMsgsDomain, "_OlGetDnDVendorPartExtension: NULL widget"));
	} else if (XtIsVendorShell(w) == True) {
		OlDnDVendorClassExtension	extension =
							GET_DND_EXT(XtClass(w));
		OlDnDVendorPartExtension	*part_ptr = 
					        	&extension->instance_part_list;

		for (part = *part_ptr; part != NULL_PART && w != part->owner;)
		{
			part_ptr	= &part->next_part;
			part		= *part_ptr;
		}
	}
	return(part);
} /* END OF _OlDnDGetVendorPartExtension() */

/**************************************************
 *
 * CallDnDVCXExtensionMethods
 *
 * call chained the class part extension procs ..
 *
 **************************************************/

static Boolean
_CallExtensionMethods (DnDVCXMethodType extension_method_to_call, WidgetClass wc, Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args, OlDnDVendorPartExtension cur_part, OlDnDVendorPartExtension req_part, OlDnDVendorPartExtension new_part)
{
	Boolean				ret_val = False;
	OlDnDVendorClassExtension	ext = GET_DND_EXT(wc);

	/* chain destroy sub to super ... */

	if (ext != (OlDnDVendorClassExtension)NULL        &&
	    extension_method_to_call == CallDnDVCXDestroy &&
	    ext->destroy != (OlDnDVCXDestroyProc)NULL)
	{
	    (*ext->destroy)(current, cur_part);
	}

	if (wc != vendorShellWidgetClass)
	{
	    if (_CallExtensionMethods(extension_method_to_call,
				      wc->core_class.superclass,
				      current, request, new,
				      args, num_args,
				      cur_part, req_part, 
				      new_part) == TRUE)
	    {
		ret_val = TRUE;
	    }
	}

	if (ext != NULL_EXTENSION) {
	    switch (extension_method_to_call)
	    {
	    case CallDnDVCXInitialize :
		if (ext->initialize != (OlDnDVCXInitializeProc)NULL)
		{
		    (*ext->initialize)(request, new,
				args, num_args, req_part, new_part);
		}
		break;
	
	    case CallDnDVCXDestroy :
		/* done it already .... */
		break;

	    case CallDnDVCXSetValues :
		if (ext->set_values != (OlDnDVCXSetValuesFunc)NULL)
		{
			if ((*ext->set_values)(current, request, new,
				args, num_args, cur_part, req_part, new_part)
			     == TRUE)
			{
				ret_val = TRUE;
			}
		}
		break;

	    case CallDnDVCXGetValues :
		if (ext->get_values != (OlDnDVCXGetValuesProc)NULL)
		{
		    (*ext->get_values)(current, args, num_args, cur_part);
		}
		break;

	    } /* end of switch */
	}

	return (ret_val);
}

Boolean
CallDnDVCXExtensionMethods (DnDVCXMethodType extension_method_to_call, WidgetClass wc, Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	Boolean				ret_val = False;
	WidgetClass			original_wc = wc;
	OlDnDVendorClassExtension	ext = GET_DND_EXT(wc);

	OlDnDVendorPartExtension	cur_part = NULL_PART,
					new_part = NULL_PART,
					req_part = NULL_PART,
					part;
	OlDnDVendorPartExtensionRec	cur, req;
	unsigned int			size;

	while (ext == NULL_EXTENSION && wc != vendorShellWidgetClass) {
		wc = wc->core_class.superclass;
		ext = GET_DND_EXT(wc);
	}

	if (ext == NULL_EXTENSION)
		return (False);

	switch (extension_method_to_call) {
	    case CallDnDVCXInitialize :
			
			/*
			 * create a new extension instance record
			 */ 

			size = (int)ext->instance_part_size;
			part = (OlDnDVendorPartExtension) XtCalloc(1, size);

			/* chain it */

			part->owner = new;
			part->class_extension = ext;
			part->next_part = ext->instance_part_list;
			ext->instance_part_list = part;

			/* init the resources */

			XtGetSubresources(new, (XtPointer)part, (String)NULL,
					  (String)NULL, ext->resources,
					  ext->num_resources, args, *num_args);

			new_part = part;
			req = *part;
			req_part = &req;
			break;

	    case CallDnDVCXDestroy :
			cur_part = _OlGetDnDVendorPartExtension(current);
			request = new = (Widget)NULL;
			args = (ArgList)NULL;
			num_args = (Cardinal)0;
			break;

	    case CallDnDVCXSetValues :
			new_part = _OlGetDnDVendorPartExtension(new);
			if (new_part == NULL_PART)
				return (False);	/*oops*/

			cur = *new_part;
			cur_part = &cur;

			XtSetSubvalues((XtPointer)new_part, ext->resources,
				       ext->num_resources, args, *num_args);
			req = *new_part;
			req_part = &req;
			break;

	    case CallDnDVCXGetValues :
			cur_part = _OlGetDnDVendorPartExtension(current);
			if (cur_part == NULL_PART)
				return (False);

			XtGetSubvalues((XtPointer)cur_part, ext->resources,
				       ext->num_resources, args, *num_args);

			request = new = (Widget)NULL;
			break;
	}

	ret_val = _CallExtensionMethods(extension_method_to_call, original_wc,
				       current, request, new, args, num_args,
				       cur_part, req_part, new_part);

	if (extension_method_to_call == CallDnDVCXDestroy) {
		OlDnDVendorPartExtension	*part_ptr;

		/* free the extension instance part */

		for ((part_ptr = &ext->instance_part_list), (part = *part_ptr);
		     part != NULL_PART && current != part->owner;) {
			part_ptr = &part->next_part;
			part = *part_ptr;
		}
		
		if (part != NULL_PART) {
			*part_ptr = part->next_part;
			XtFree((char *)part);
		}
	}

	return (ret_val);
}

/************************************************************
 *
 *	_OlDnDSetDisableDSClipping()
 *
 ************************************************************/

void
_OlDnDSetDisableDSClipping (Widget widget, Boolean value)
{
	Widget		 	 vendor;
	OlDnDVendorPartExtension dnd_part;

	vendor = _OlGetShellOfWidget(widget);

	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part)
		_SetDisableClipping(dnd_part, dnd_part, widget, value);
}
/******************************** USER CALLS ********************************/

/************************************************************
 *
 *	OlDnDGetWidgetOfDropSite
 *
 * return widget associated with drop site .. this is either
 * the widget associated with the drop site on registration
 * or the widget parent of the window registered.
 *
 ************************************************************/

/*FTNPROTOB*/
Widget
OlDnDGetWidgetOfDropSite (OlDnDDropSiteID dropsiteid)
/*FTNPROTOE*/
{
        Widget retval;

	GetToken();
	retval = OlDnDDropSitePtrOwner((OlDnDDropSitePtr)dropsiteid);
	ReleaseToken();
	return retval;
}

/************************************************************
 *
 *	OlDnDGetWindowOfDropSite
 *
 * return the window associated with the dropsite ... this
 * is either the window registered or the window of the
 * "owning" widget ...
 *
 * if the 'object' registered was a gadget then the window
 * ID is that of its windowed ancestor
 *
 ************************************************************/

/*FTNPROTOB*/
Window
OlDnDGetWindowOfDropSite (OlDnDDropSiteID dropsiteid)
/*FTNPROTOE*/
{
        Window retval;

	GetToken();
	retval =  OlDnDDropSitePtrWindow((OlDnDDropSitePtr)dropsiteid);
	ReleaseToken();
	return retval;
}

/************************************************************
 *
 *	OlDnDGetDropSitesOfWidget
 *
 * return a list of dropsites registered for this widget
 *
 * note: the client must free the store returned
 *
 ************************************************************/

/*FTNPROTOB*/
OlDnDDropSiteID *
OlDnDGetDropSitesOfWidget (Widget widget, Cardinal *num_sites_return)
/*FTNPROTOE*/
{
	OlDnDDropSitePtr		dsp;
	Widget				vendor;
	OlDnDVendorPartExtension	dnd_part;
	unsigned int			num;
	OlDnDDropSiteID			*dsids, *p;

	GetToken();
	vendor = _OlGetShellOfWidget(widget);
	*num_sites_return = (Cardinal)0;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL ||
	    dnd_part->drop_site_list == (OlDnDDropSitePtr)NULL) {
	        ReleaseToken();
		return (OlDnDDropSiteID *)NULL;
	}

	for (num = 0, dsp = dnd_part->drop_site_list;
	     dsp != (OlDnDDropSitePtr)NULL;
	     dsp = OlDnDDropSitePtrNextSite(dsp))
		if (widget == OlDnDDropSitePtrOwner(dsp)) num++;

	if (!num) { ReleaseToken(); return (OlDnDDropSiteID *)NULL; }

	p = dsids = (OlDnDDropSiteID *)XtCalloc(num, sizeof(OlDnDDropSiteID));

	for (dsp = dnd_part->drop_site_list;
	     dsp != (OlDnDDropSitePtr)NULL;
             dsp = OlDnDDropSitePtrNextSite(dsp))
		if (widget == OlDnDDropSitePtrOwner(dsp)) 
			*p++ = (OlDnDDropSiteID)dsp;

	*num_sites_return = num;

	ReleaseToken();
	return dsids;
}

/************************************************************
 *
 *	OlDnDGetDropSitesOfWindow
 *
 * return a list of dropsites registered for this window
 *
 * note: the client must free the store returned
 *
 ************************************************************/

/*FTNPROTOB*/
OlDnDDropSiteID *
OlDnDGetDropSitesOfWindow (Display *dpy, Window window, Cardinal *num_sites_return)
/*FTNPROTOE*/
{
	OlDnDDropSitePtr		dsp;
	Widget				vendor;
	OlDnDVendorPartExtension	dnd_part;
	unsigned int			num;
	OlDnDDropSiteID			*dsids, *p;

	GetToken();
	*num_sites_return = (Cardinal)0;

	GetShellAndParentOfWindow(dpy, window, &vendor, (Widget *)NULL);

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL ||
	    dnd_part->drop_site_list == (OlDnDDropSitePtr)NULL) {
	        ReleaseToken();
		return (OlDnDDropSiteID *)NULL;
	}

	for (num = 0, dsp = dnd_part->drop_site_list;
	     dsp != (OlDnDDropSitePtr)NULL;
	     dsp = OlDnDDropSitePtrNextSite(dsp))
		if (window == OlDnDDropSitePtrWindow(dsp)) num++;

	if (!num) { ReleaseToken(); return (OlDnDDropSiteID *)NULL; }

	p = dsids = (OlDnDDropSiteID *)XtCalloc(num, sizeof(OlDnDDropSiteID));

	for (dsp = dnd_part->drop_site_list;
	     dsp != (OlDnDDropSitePtr)NULL;
             dsp = OlDnDDropSitePtrNextSite(dsp))
		if (window == OlDnDDropSitePtrWindow(dsp)) 
			*p++ = (OlDnDDropSiteID)dsp;

	ReleaseToken();
	return dsids;
}
	

/************************************************************
 *
 *	OlDnDOwnSelection
 *
 * this is the function used to assert ownership of a
 * selection suitable for drag and drop operations.
 * it is identical in function to the Intrinisics rountines
 * for handling selections except that instead of specifying
 * a widget the caller specifies a dropsite ID ...
 *
 * 
 ************************************************************/

/*
 * this handler is used to catch the death of the requestors window
 * during the drag and drop operation.
 *
 * if the property notify is for the requestor window, with our transient
 * atom specified and the state being deleted then we can assume that
 * the window has been destroyed so we call the Protocol state callback
 * with a detail of OlDnDTransactionRequestorWindowDeath.
 *
 */

static void
_OlDnDRequestorWindowDiedHandler (Widget widget, XtPointer client_data, XEvent *xevent, Boolean *continue_to_dispatch)
{
	OwnerProcClosurePtr		opc = (OwnerProcClosurePtr)client_data;
        XDestroyWindowEvent             *dnev = (XDestroyWindowEvent *)xevent;
        Atom                            selection;
 
 
        if (dnev->type != DestroyNotify) return;

	if (dnev->window != XtWindowOfObject(widget) &&
            dnev->window ==
	    DSSelectionAtomPtrRequestorWindow(OwnerProcClosurePtrAssoc(opc))) {
		selection = DSSelectionAtomPtrSelectionAtom(
				OwnerProcClosurePtrAssoc(opc));

                if (OwnerProcClosurePtrStateProc(opc) !=
                    (OlDnDTransactionStateCallback)NULL)
                        (*OwnerProcClosurePtrStateProc(opc))
                                (widget, selection,
                                 OlDnDTransactionRequestorWindowDeath,
                                 OwnerProcClosurePtrTimestamp(opc),
                                 OwnerProcClosurePtrClientData(opc));

                if (OwnerProcClosurePtrCleanupProc(opc) !=
                        (OlDnDTransactionCleanupProc)NULL)
                                (*OwnerProcClosurePtrCleanupProc(opc))(opc);
	}
}

static void
_LocalRequestorDied (Widget w, XtPointer client_data, XtPointer call_data)
{
	OwnerProcClosurePtr	opc = (OwnerProcClosurePtr)client_data;

	if (OwnerProcClosurePtrStateProc(opc) !=
	    (OlDnDTransactionStateCallback)NULL)
		(*OwnerProcClosurePtrStateProc(opc))
			(w, DSSelectionAtomPtrSelectionAtom(
				OwnerProcClosurePtrAssoc(opc)),
			 OlDnDTransactionRequestorWindowDeath,
			 OwnerProcClosurePtrTimestamp(opc),
			 OwnerProcClosurePtrClientData(opc));

	if (OwnerProcClosurePtrCleanupProc(opc) !=
	    (OlDnDTransactionCleanupProc)NULL)
		(*OwnerProcClosurePtrCleanupProc(opc))(opc);
}

/*
 * tidy up the requestor window death handler 
 */

#define	RemoteRequestorEVM	(StructureNotifyMask)

static void
_RemoveRequestorWindowDeathHandler (OwnerProcClosurePtr closure)
{
	Widget			widget;
	DSSelectionAtomPtr	sap = OwnerProcClosurePtrAssoc(closure);
	Window			window;
	Display			*dpy;

	window = DSSelectionAtomPtrRequestorWindow(sap);
	dpy    = DSSelectionAtomPtrRequestorDisplay(sap);

	/* was this the death of a non-widget window ?? */

	widget = XtWindowToWidget(dpy, window);

	if (widget == (Widget)NULL) return;	/* oops ! */

	if (XtWindowOfObject(widget) != window) {	/* non-widget */
		XWindowAttributes	wattrs;
		unsigned long		evm;

		_XtUnregisterWindow(window, widget);

		XtRemoveRawEventHandler(widget, RemoteRequestorEVM, False,
				        _OlDnDRequestorWindowDiedHandler,
				        (XtPointer)closure);

		EnterProtectedSection(dpy, (XID)window, _PSErrorHandler);

		XGetWindowAttributes(dpy, window, &wattrs);

		evm = wattrs.your_event_mask;

		evm &= ~(RemoteRequestorEVM & 
			 ~(RemoteRequestorEVM &
			   OwnerProcClosurePtrWEVM(closure))
		        );

		XSelectInput(dpy, window, evm);
		LeaveProtectedSection(dpy);

	} else XtRemoveCallback(widget, XtNdestroyCallback,
				_LocalRequestorDied, closure);

	_StopDnDTxTimer(closure);	/* done */

	OwnerProcClosurePtrCleanupProc(closure) = 
		(OlDnDTransactionCleanupProc)NULL;
}

static void
_DnDTxTimeout (XtPointer client_data, XtIntervalId *timer_id)
{
	OwnerProcClosurePtr	opc = (OwnerProcClosurePtr)client_data;
	Widget			owner;
	Atom			selection;
	Time			time;

	if (OwnerProcClosurePtrTimerId(opc) != *timer_id) return;

	OwnerProcClosurePtrTimerId(opc) = (XtIntervalId)NULL;

	owner     = DSSelectionAtomPtrOwner(OwnerProcClosurePtrAssoc(opc));
	selection = DSSelectionAtomPtrSelectionAtom(
			OwnerProcClosurePtrAssoc(opc));
	time      = OwnerProcClosurePtrTimestamp(opc);

	if (OwnerProcClosurePtrStateProc(opc) !=
		(OlDnDTransactionStateCallback)NULL) {
                        (*OwnerProcClosurePtrStateProc(opc))
                                (owner,
                                 (int)selection,
                                 OlDnDTransactionTimeout,
				 time,
                                 OwnerProcClosurePtrClientData(opc));
 
		if (OwnerProcClosurePtrCleanupProc(opc) !=
			(OlDnDTransactionCleanupProc)NULL) 
				(*OwnerProcClosurePtrCleanupProc(opc))(opc);

	} else {	/* disown the selection */

		/*
		 * a little anti-social but at least they'll get a 
		 * notification if they have a lose proc .... if not
		 * then some clean-up will occur in the DnD code.
		 */

		OlDnDDisownSelection(owner, selection, time);
	}

}
	
/*
 * filter out Selection conversion requests that are part of the
 * drag and drop protocol, and not part of the data transfer.
 */

static Boolean
_OlDnDConvertSelectionFilter (Widget vendor, OlDnDVendorPartExtension dnd_part, OwnerProcClosurePtr opc, Widget widget, Atom *selection, Atom *target, Atom *type, XtPointer *value, long unsigned int *length, int *format, XtRequestId req_id, Boolean *dispatch_convert)
{
	XSelectionRequestEvent	*sev;
	OlDnDTransactionState	state;

	FetchPartExtensionIfNull(vendor, dnd_part);

	*dispatch_convert = False;

	sev = XtGetSelectionRequest(widget, *selection, req_id);

	if (sev == (XSelectionRequestEvent *)NULL) {

		/* something is SERIOUSLY wrong */

                if (OwnerProcClosurePtrStateProc(opc) !=
                    (OlDnDTransactionStateCallback)NULL)
                        (*OwnerProcClosurePtrStateProc(opc))
                                (widget,
                                 (int)*selection,
				 OlDnDTransactionRequestorError,
                                 OwnerProcClosurePtrTimestamp(opc),
                                 OwnerProcClosurePtrClientData(opc));
 
		_StartDnDTxTimer(opc);	/* reset */

		return (False);
	}

	OwnerProcClosurePtrTimestamp(opc) = sev->time;

	if (*target == OlInternAtom( XtDisplayOfObject(widget),
				     _SUN_DRAGDROP_ACK_NAME)) {
		Widget			w;

		*value = (XtPointer)NULL;
		*length = (unsigned long)0;
		*format = 32;
		*type = XA_ATOM;

		DSSelectionAtomPtrRequestorWindow(
			OwnerProcClosurePtrAssoc(opc)) = sev->requestor;
		DSSelectionAtomPtrRequestorDisplay(
			OwnerProcClosurePtrAssoc(opc)) = sev->display;

		if ((w = XtWindowToWidget(sev->display, sev->requestor)) !=
		     (Widget)NULL) {
			/* this is a window owned by this app ..... */

			XtAddCallback(w, XtNdestroyCallback,
				      _LocalRequestorDied, (XtPointer)opc);
		} else {	/* non local window */
			XWindowAttributes	wattrs;

			EnterProtectedSection(sev->display,
					      (XID)(sev->requestor),
					      _PSErrorHandler);


			XGetWindowAttributes(sev->display,
					     sev->requestor, &wattrs);

			OwnerProcClosurePtrWEVM(opc) = wattrs.your_event_mask;
			wattrs.your_event_mask |= RemoteRequestorEVM;

			XSelectInput(sev->display, sev->requestor, 
				     wattrs.your_event_mask);

                        _XtRegisterWindow(sev->requestor, widget);

			XtAddRawEventHandler(widget, RemoteRequestorEVM, False,
					     _OlDnDRequestorWindowDiedHandler,
					     (XtPointer)opc);

			LeaveProtectedSection(sev->display);
		}

		_StartDnDTxTimer(opc);	/* reset */

		return (True);
	}

	if (*target == OlInternAtom( XtDisplayOfObject(widget),
				     _SUN_DRAGDROP_BEGIN_NAME)) {

		*value = (XtPointer)NULL;
		*length = (unsigned long)0;
		*format = 32;
		*type = XA_ATOM;

		if (OwnerProcClosurePtrStateProc(opc) !=
		    (OlDnDTransactionStateCallback)NULL)
			(*OwnerProcClosurePtrStateProc(opc))
				(widget,
				 (int)*selection,
				 OlDnDTransactionBegins,
				 sev->time,
				 OwnerProcClosurePtrClientData(opc));

		_StartDnDTxTimer(opc);	/* reset */

		return (True);
	}

	if (((state = OlDnDTransactionEnds),
	      *target == OlInternAtom( XtDisplayOfObject(widget),
				       _SUN_SELECTION_END_NAME))   ||
	    ((state = OlDnDTransactionRequestorError),
	      *target == OlInternAtom( XtDisplayOfObject(widget),
				       _SUN_SELECTION_ERROR_NAME)) ||
	    ((state = OlDnDTransactionDone),
	      *target == OlInternAtom( XtDisplayOfObject(widget),
				       _SUN_DRAGDROP_DONE_NAME))) {

		_StopDnDTxTimer(opc);

		*value = (XtPointer)NULL;
		*length = (unsigned long)0;
		*format = 32;
		*type = XA_ATOM;

		/*
		 * call the state callback to indicate
		 * that the transaction failed.
		 */

		if (OwnerProcClosurePtrStateProc(opc) != 
		    (OlDnDTransactionStateCallback)NULL)
			(*OwnerProcClosurePtrStateProc(opc))
				(widget,
				 (int)*selection,
				 state,
				 sev->time,
				 OwnerProcClosurePtrClientData(opc));

		if (OwnerProcClosurePtrCleanupProc(opc) !=
			(OlDnDTransactionCleanupProc)NULL) 
				(*OwnerProcClosurePtrCleanupProc(opc))(opc);

		return (True);
	}

	/*
	 * if we got to here then its part of the selection
	 * transfer and not part of the drag and drop protocol
	 * so call the users registered Convert Proc.
	 */

	*dispatch_convert = True;

	_StartDnDTxTimer(opc);	/* reset */

	return (True);
}


/*
 * the Selection Converter ..... filters out protocol conversions
 * but calls the users ConvertSelection Proc whern appropriate
 */

static Boolean
_OlDnDConvertSelectionFunc (Widget widget, Atom *selection, Atom *target, Atom *type, XtPointer *value, long unsigned int *length, int *format)
{
	Widget				vendor = _OlGetShellOfWidget(widget);
	Widget				sw;
	OlDnDVendorPartExtension	dnd_part;
	OwnerProcClosurePtr		opc;
	Boolean				ret_val, dispatch_convert = False;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL)
		return False; /* oops! */

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, *selection, &opc);

	if (sw != widget)
		return (False);

	ret_val = _OlDnDConvertSelectionFilter(vendor, dnd_part, opc,
					     widget, selection, target,
					     type, value, length, format,
					     (XtRequestId)NULL,
					     &dispatch_convert);

	if (dispatch_convert && OwnerProcClosurePtrConvertProc(opc) !=
	    (XtConvertSelectionProc)NULL)
		ret_val = (*OwnerProcClosurePtrConvertProc(opc))
				(widget, selection, target, type, value,
				 length, format);

	return (ret_val);
}

/*
 * tidy up its all over .....
 */

static void
_OlDnDSelectionDoneProc (Widget widget, Atom *selection, Atom *target)
{
	Widget				vendor = _OlGetShellOfWidget(widget);
	Widget				sw;
	OlDnDVendorPartExtension	dnd_part;
	OwnerProcClosurePtr		opc;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL)
		return; /* oops! */

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, *selection, &opc);

	if (sw != widget)
		return;

	if (OwnerProcClosurePtrDoneProc(opc) != 
	    (XtSelectionDoneProc)NULL)
		(*OwnerProcClosurePtrDoneProc(opc))
			(widget, selection, target); 
}

/*
 * tidy up we lost the selection ..... 
 */

static void
_OlDnDLoseSelectionProc (Widget widget, Atom *selection)
{
	Widget				vendor = _OlGetShellOfWidget(widget);
	Widget				sw;
	OlDnDVendorPartExtension	dnd_part;
	OwnerProcClosurePtr		opc;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL)
		return; /* oops! */

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, *selection, &opc);

	if (sw != widget)
		return;

	if (OwnerProcClosurePtrLoseProc(opc) != 
	    (XtLoseSelectionProc)NULL)
		(*OwnerProcClosurePtrLoseProc(opc))
			(widget, selection); 

	if (OwnerProcClosurePtrCleanupProc(opc) !=
		(OlDnDTransactionCleanupProc)NULL)
			(*OwnerProcClosurePtrCleanupProc(opc))(opc);

	_OlDnDDisassocSelectionWithWidget(vendor, dnd_part, widget,
				          *selection, 
					  OwnerProcClosurePtrTimestamp(opc));

	XtFree((char *)opc);
}


/* OlDnDOwnSelection */

/*FTNPROTOB*/
Boolean
OlDnDOwnSelection (Widget widget, Atom selection, Time timestamp, XtConvertSelectionProc convert_proc, XtLoseSelectionProc lose_selection_proc, XtSelectionDoneProc done_proc, OlDnDTransactionStateCallback state_proc, XtPointer closure)
/*FTNPROTOE*/
{
	Widget				vendor;
	OwnerProcClosurePtr		opc;
	OlDnDVendorPartExtension	dnd_part;

	GetToken();
	vendor = _OlGetShellOfWidget(widget);
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part == (OlDnDVendorPartExtension)NULL) {
		ReleaseToken();
		return (False);
        }

	/*
	 * create our own private closure of the users procs and then
	 * assert ownership of the selection, using our private 
	 * filter functions.
	 */
	  
	opc = (OwnerProcClosurePtr)XtCalloc(1, sizeof(OwnerProcClosure));

	OwnerProcClosurePtrConvertProc(opc) = convert_proc;
	OwnerProcClosurePtrLoseProc(opc) = lose_selection_proc;
	OwnerProcClosurePtrDoneProc(opc) = done_proc;
	OwnerProcClosurePtrDoneIncrProc(opc) =
		(XtSelectionDoneIncrProc)NULL;
	OwnerProcClosurePtrConvertIncrProc(opc) =
		(XtConvertSelectionIncrProc)NULL;
	OwnerProcClosurePtrLoseIncrProc(opc) =
		(XtLoseSelectionIncrProc)NULL;
	 OwnerProcClosurePtrCancelIncrProc(opc) =
		(XtCancelConvertSelectionProc)NULL;
	OwnerProcClosurePtrStateProc(opc) = state_proc;
	OwnerProcClosurePtrClientData(opc) = closure;
	OwnerProcClosurePtrCleanupProc(opc) =
			_RemoveRequestorWindowDeathHandler;
	OwnerProcClosurePtrSelectionTransient(opc) = 
			_OlDnDAtomIsTransient(vendor, dnd_part, selection);
	OwnerProcClosurePtrTimestamp(opc) = timestamp;
	OwnerProcClosurePtrTimeoutProc(opc) = _DnDTxTimeout;
	OwnerProcClosurePtrTimerId(opc) = (XtIntervalId)NULL;
 
	if (!XtOwnSelection(widget,
			    selection, timestamp,
			    _OlDnDConvertSelectionFunc,
			    _OlDnDLoseSelectionProc,
			    _OlDnDSelectionDoneProc)) {

		XtFree((char *)opc);
		ReleaseToken();
		return (False);
	}

	OwnerProcClosurePtrAssoc(opc) = _OlDnDAssocSelectionWithWidget(
						vendor, dnd_part, widget,
						selection, timestamp, opc);

	ReleaseToken();
	return (True);
}

/************************************************************
 *
 *	OlDnDOwnSelectionIncremental
 *
 ************************************************************/


static Boolean
_OlDnDConvertSelectionIncrFunc (Widget widget, Atom *selection, Atom *target, Atom *type, XtPointer *value, long unsigned int *length, int *format, long unsigned int *max_length, XtPointer client_data, XtRequestId *request_id)
{
	Widget				vendor = _OlGetShellOfWidget(widget);
	Widget				sw;
	OlDnDVendorPartExtension	dnd_part;
	OwnerProcClosurePtr		opc;
	Boolean				ret_val, dispatch_convert;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL)
		return (False); /* oops! */

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, *selection, &opc);

	if (sw != widget)
		return (False);

	ret_val = _OlDnDConvertSelectionFilter(vendor, dnd_part, opc,
					     widget, selection, target,
					     type, value, length, format,
					     *request_id, &dispatch_convert);

	if (dispatch_convert && OwnerProcClosurePtrConvertIncrProc(opc) !=
	    (XtConvertSelectionIncrProc)NULL) 
		return (*OwnerProcClosurePtrConvertIncrProc(opc))
				(widget, selection, target, type,
				 value, length, format, max_length,
				 client_data, request_id);
	else
	    return (ret_val);
}


static void
_OlDnDSelectionDoneIncrProc (Widget widget, Atom *selection, Atom *target, XtRequestId *request_id, XtPointer client_data)
{
	Widget				vendor = _OlGetShellOfWidget(widget);
	Widget				sw;
	OlDnDVendorPartExtension	dnd_part;
	OwnerProcClosurePtr		opc;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL)
		return; /* oops! */

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, *selection, &opc);

	if (sw != widget)
		return;

	if (OwnerProcClosurePtrDoneIncrProc(opc) != 
	    (XtSelectionDoneIncrProc)NULL)
		(*OwnerProcClosurePtrDoneIncrProc(opc))
			(widget, selection, target, client_data, request_id); 
}

static void
_OlDnDLoseSelectionIncrProc (Widget widget, Atom *selection, XtPointer client_data)
{
	Widget				vendor = _OlGetShellOfWidget(widget);
	Widget				sw;
	OlDnDVendorPartExtension	dnd_part;
	OwnerProcClosurePtr		opc;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL)
		return; /* oops! */

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, *selection, &opc);

	if (sw != widget)
		return;

	if (OwnerProcClosurePtrLoseIncrProc(opc) != 
	    (XtLoseSelectionIncrProc)NULL)
		(*OwnerProcClosurePtrLoseIncrProc(opc))
			(widget, selection, client_data); 

	if (OwnerProcClosurePtrCleanupProc(opc) !=
		(OlDnDTransactionCleanupProc)NULL)
			(*OwnerProcClosurePtrCleanupProc(opc))(opc);

	_OlDnDDisassocSelectionWithWidget(vendor, dnd_part, widget,
					  *selection, 
					  OwnerProcClosurePtrTimestamp(opc));

	XtFree((char *)opc);
}

static void
_OlDnDCancelConvertSelectionProc (Widget widget, Atom *selection, Atom *target, XtRequestId *requestid, XtPointer client_data)
{
	Widget				vendor = _OlGetShellOfWidget(widget);
	Widget				sw;
	OlDnDVendorPartExtension	dnd_part;
	OwnerProcClosurePtr		opc;

	if ((dnd_part = _OlGetDnDVendorPartExtension(vendor)) ==
	    (OlDnDVendorPartExtension)NULL)
		return; /* oops! */

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, *selection, &opc);

	if (sw != widget)
		return;

	if (OwnerProcClosurePtrCancelIncrProc(opc) != 
	    (XtCancelConvertSelectionProc)NULL)
		(*OwnerProcClosurePtrCancelIncrProc(opc))
			(widget, selection, target, requestid, client_data); 
}

/*FTNPROTOB*/
Boolean
OlDnDOwnSelectionIncremental (Widget widget, Atom selection, Time timestamp, XtConvertSelectionIncrProc convert_incr_proc, XtLoseSelectionIncrProc lose_incr_selection_proc, XtSelectionDoneIncrProc incr_done_proc, XtCancelConvertSelectionProc incr_cancel_proc, XtPointer client_data, OlDnDTransactionStateCallback state_proc)
/*FTNPROTOE*/
{
	Widget				vendor;
	OwnerProcClosurePtr		opc;
	OlDnDVendorPartExtension	dnd_part;

	GetToken();
	vendor = _OlGetShellOfWidget(widget);
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part == (OlDnDVendorPartExtension)NULL) {
		ReleaseToken();
		return (False);
	}

	opc = (OwnerProcClosurePtr)XtCalloc(1, sizeof(OwnerProcClosure));

	OwnerProcClosurePtrConvertIncrProc(opc) = convert_incr_proc;
	OwnerProcClosurePtrLoseIncrProc(opc) = lose_incr_selection_proc;
	OwnerProcClosurePtrCancelIncrProc(opc) = incr_cancel_proc;
	OwnerProcClosurePtrDoneIncrProc(opc) = incr_done_proc;
	OwnerProcClosurePtrDoneProc(opc) = (XtSelectionDoneProc)NULL;
	OwnerProcClosurePtrConvertProc(opc) = (XtConvertSelectionProc)NULL;
	OwnerProcClosurePtrLoseProc(opc) = (XtLoseSelectionProc)NULL;
	OwnerProcClosurePtrStateProc(opc) = state_proc;
	OwnerProcClosurePtrClientData(opc) = client_data;
	OwnerProcClosurePtrCleanupProc(opc) =
			_RemoveRequestorWindowDeathHandler;
	OwnerProcClosurePtrSelectionTransient(opc) = 
			_OlDnDAtomIsTransient(vendor, dnd_part, selection);
	OwnerProcClosurePtrTimeoutProc(opc) = _DnDTxTimeout;
	OwnerProcClosurePtrTimerId(opc) = (XtIntervalId)NULL;
	OwnerProcClosurePtrWEVM(opc)    = NoEventMask;
 
	if (!XtOwnSelectionIncremental(widget,
			    selection, timestamp,
			    _OlDnDConvertSelectionIncrFunc,
			    _OlDnDLoseSelectionIncrProc,
			    _OlDnDSelectionDoneIncrProc,
			    _OlDnDCancelConvertSelectionProc,
			    client_data)) {
		XtFree((char *)opc);
		ReleaseToken();
		return (False);
	}

	OwnerProcClosurePtrAssoc(opc) = _OlDnDAssocSelectionWithWidget(
						vendor, dnd_part, widget,
						selection, timestamp, opc);

	ReleaseToken();
	return (True);
}

/************************************************************
 *
 * OlDnDDisownSelection
 *
 * disown the selection ....
 *
 ************************************************************/

/*FTNPROTOB*/
void
OlDnDDisownSelection (Widget widget, Atom selection, Time time)
/*FTNPROTOE*/
{
	Widget				vendor, sw;
	OwnerProcClosurePtr		opc;
	OlDnDVendorPartExtension	dnd_part;

	GetToken();
	vendor = _OlGetShellOfWidget(widget);
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part == (OlDnDVendorPartExtension)NULL) {
		ReleaseToken();
		return;
        }

	sw = _OlDnDGetWidgetForSelection(vendor, dnd_part, selection, &opc);

	if (widget != sw) {
		ReleaseToken();
		return;
        }
	if (OwnerProcClosurePtrCleanupProc(opc) !=
		(OlDnDTransactionCleanupProc)NULL)
			(*OwnerProcClosurePtrCleanupProc(opc))(opc);

	_OlDnDDisassocSelectionWithWidget(vendor, dnd_part, widget,
					  selection, time);

	XtDisownSelection(widget, selection, time);

	XtFree((char *)opc);
	ReleaseToken();
}

/************************************************************
 *
 *	OlDnDBeginSelectionTransaction
 *
 ************************************************************/

static void
_DNDVCXHandleProtocolActionCB (Widget vendor, XtPointer client_data, Atom *selection, Atom *type, XtPointer value, long unsigned int *length, int *format)
{
	OlDnDProtocolAction	action;
	OlDnDDropSitePtr	dsp;
	ReqProcClosurePtr	pc = (ReqProcClosurePtr)client_data;

	if (ReqProcClosurePtrAction(pc) == 
	    OlInternAtom( XtDisplay(vendor), _SUN_DRAGDROP_BEGIN_NAME)) {
		action = OlDnDSelectionTransactionBegins;
	} else if (ReqProcClosurePtrAction(pc) == 
		   OlInternAtom( XtDisplay(vendor), _SUN_SELECTION_END_NAME))
		action = OlDnDSelectionTransactionEnds;
	else if (ReqProcClosurePtrAction(pc) == 
		 OlInternAtom( XtDisplay(vendor), _SUN_DRAGDROP_DONE_NAME))
			action = OlDnDDragNDropTransactionDone;
	     else
			action = OlDnDSelectionTransactionError;

		
	if (ReqProcClosurePtrCallback(pc) != 
	    (OlDnDProtocolActionCallbackProc)NULL)
		(*ReqProcClosurePtrCallback(pc))
				(ReqProcClosurePtrWidget(pc),
				 *selection, action,
				 (*type != XT_CONVERT_FAIL),
				 ReqProcClosurePtrClosure(pc));
	XtFree((char *)pc);
}

static void
DnDVCXGetProtocolActionSelection (Widget widget, Atom selection, Atom protocol, Time timestamp, OlDnDProtocolActionCallbackProc proc, XtPointer closure)
{
	Widget			vendor = _OlGetShellOfWidget(widget);
	ReqProcClosurePtr	pc = (ReqProcClosurePtr)
					XtCalloc(1, sizeof(ReqProcClosure));

	ReqProcClosurePtrWidget(pc) = widget;
	ReqProcClosurePtrCallback(pc) = proc;
	ReqProcClosurePtrClosure(pc) = closure;
	ReqProcClosurePtrAction(pc) = protocol;

	XtGetSelectionValue(vendor, selection, protocol,
			    _DNDVCXHandleProtocolActionCB,
			    (XtPointer)pc, timestamp);
}

/*FTNPROTOB*/
void
OlDnDBeginSelectionTransaction (Widget widget, Atom selection, Time timestamp, OlDnDProtocolActionCallbackProc proc, XtPointer closure)
/*FTNPROTOE*/
{
        GetToken();
	DnDVCXGetProtocolActionSelection(widget, selection,
					OlInternAtom(XtDisplayOfObject(widget),
						     _SUN_DRAGDROP_BEGIN_NAME),
					 timestamp, proc, closure);
	ReleaseToken();
}

/************************************************************
 *
 *	OlDnDEndSelectionTransaction
 *
 ************************************************************/

/*FTNPROTOB*/
void
OlDnDEndSelectionTransaction (Widget widget, Atom selection, Time timestamp, OlDnDProtocolActionCallbackProc proc, XtPointer closure)
/*FTNPROTOE*/
{
        GetToken();
	DnDVCXGetProtocolActionSelection(widget, selection,
					 OlInternAtom(XtDisplayOfObject(widget),
						      _SUN_SELECTION_END_NAME),
					 timestamp,
					 proc, closure);
	ReleaseToken();
}

/************************************************************
 *
 *	OlDnDDragNDropDone
 *
 ************************************************************/

/*FTNPROTOB*/
void
OlDnDDragNDropDone (Widget widget, Atom selection, Time timestamp, OlDnDProtocolActionCallbackProc proc, XtPointer closure)
/*FTNPROTOE*/
{
        GetToken();
	DnDVCXGetProtocolActionSelection(widget, selection,
					 OlInternAtom(XtDisplayOfObject(widget),
						      _SUN_DRAGDROP_DONE_NAME),
					 timestamp,
					 proc, closure);
	ReleaseToken();
}

/************************************************************
 *
 *	OlDnDErrorDuringSelectionTransaction
 *
 ************************************************************/

/*FTNPROTOB*/
void
OlDnDErrorDuringSelectionTransaction (Widget widget, Atom selection, Time timestamp, OlDnDProtocolActionCallbackProc proc, XtPointer closure)
/*FTNPROTOE*/
{
        GetToken();
	DnDVCXGetProtocolActionSelection(widget, selection,
					 OlInternAtom(XtDisplayOfObject(widget),
						      _SUN_SELECTION_ERROR_NAME),
					 timestamp, 
					 proc, closure);
	ReleaseToken();
}

/************************************************************
 *
 *	OlDnDAllocTransientAtom
 *
 ************************************************************/

/*FTNPROTOB*/
Atom
OlDnDAllocTransientAtom (Widget widget)
       	        
/*FTNPROTOE*/
{
	Widget				vendor;
	OlDnDVendorPartExtension	dnd_part;
	Atom				retval;

	GetToken();
	vendor = _OlGetShellOfWidget(widget);
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part->class_extension->alloc_transient_atom != 
	    (OlDnDVCXAllocTransientAtomFunc)NULL)
		retval = ((*dnd_part->class_extension->alloc_transient_atom)
			  (vendor, dnd_part, widget));
	else
		retval = (Atom)NULL;
	ReleaseToken();
	return retval;
}

/************************************************************
 *
 *	OlDnDFreeTransientAtom
 *
 ************************************************************/

/*FTNPROTOB*/
void
OlDnDFreeTransientAtom (Widget widget, Atom atom)
/*FTNPROTOE*/
{
	Widget				vendor;
	OlDnDVendorPartExtension	dnd_part;

	GetToken();
	vendor = _OlGetShellOfWidget(widget);
	dnd_part = _OlGetDnDVendorPartExtension(vendor);
	if (dnd_part->class_extension->free_transient_atom !=
	    (OlDnDVCXFreeTransientAtomProc)NULL)
		(*dnd_part->class_extension->free_transient_atom)
		  (vendor, dnd_part, widget, atom);
	ReleaseToken();
}

/************************************************************
 *
 *	OlDnDRegisterWidgetDropSite()
 *
 * 	Add a new ObjectDropSite
 *
 ************************************************************/

/*FTNPROTOB*/
OlDnDDropSiteID
OlDnDRegisterWidgetDropSite (Widget  widget,
			     OlDnDSitePreviewHints preview_hints,
			     OlDnDSiteRectPtr site_rects,
			     unsigned int num_sites,
			     OlDnDTMNotifyProc tmnotify,
			     OlDnDPMNotifyProc pmnotify,
			     Boolean on_interest, XtPointer closure)
/*FTNPROTOE*/
{
	Widget				vendor;
	Window				window;
	OlDnDVendorPartExtension	dnd_part;
	OlDnDDropSiteID			retval;

	GetToken();
	if (widget == (Widget)NULL) {
		OlWarning(dgettext(OlMsgsDomain, "OlDnDRegisterWidgetDropSite: NULL widget"));
		ReleaseToken();
		return (OlDnDDropSiteID)NULL;
	}

	vendor = _OlGetShellOfWidget(widget);
	window = XtWindowOfObject(widget);

	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->register_drop_site !=
	    (OlDnDVCXRegisterDSFunc)NULL) {
		retval = (*dnd_part->class_extension->register_drop_site)
				(vendor, dnd_part, widget, window,
				 preview_hints, site_rects, num_sites,
				 tmnotify, pmnotify, on_interest, closure);
		ReleaseToken();
		return retval;
	} else {
		OlWarning(dgettext(OlMsgsDomain, "OlDnDRegisterWidgetDropSite: NULL class extension or proc"));
		ReleaseToken();
		return (OlDnDDropSiteID)NULL;
	}
}


/************************************************************
 *
 *	OlDnDRegisterWindowDropSite()
 *
 * 	Add a new WindowDropSite
 *
 ************************************************************/

/*FTNPROTOB*/
OlDnDDropSiteID
OlDnDRegisterWindowDropSite (
			     Display *dpy, Window window,
			     OlDnDSitePreviewHints preview_hints,
			     OlDnDSiteRectPtr site_rects,
			     unsigned int num_sites,
			     OlDnDTMNotifyProc tmnotify,
			     OlDnDPMNotifyProc pmnotify,
			     Boolean on_interest, XtPointer closure)
/*FTNPROTOE*/
{
	Widget				vendor, widget;
	OlDnDVendorPartExtension	dnd_part;
	OlDnDDropSiteID			retval;

	GetToken();
	if (dpy == (Display*)NULL || window == (Window)NULL) {
		OlWarning(dgettext(OlMsgsDomain, "OlDnDRegisterWindowDropSite: NULL Display or Window"));
		ReleaseToken();
		return (OlDnDDropSiteID)NULL;
	}

	GetShellAndParentOfWindow(dpy, window, &vendor, &widget);

	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->register_drop_site !=
	    (OlDnDVCXRegisterDSFunc)NULL) {
		retval = (*dnd_part->class_extension->register_drop_site)
				(vendor, dnd_part, widget, window,
				 preview_hints, site_rects, num_sites,
				 tmnotify, pmnotify, on_interest, closure);
		ReleaseToken();
		return retval;
	} else {
		OlWarning(dgettext(OlMsgsDomain, "OlDnDRegisterWindowDropSite: NULL class extension or proc"));
		ReleaseToken();
		return (OlDnDDropSiteID)NULL;
	}
}

/********************************************
 *
 * OlDnDDestroyDropSite
 *
 ********************************************/


/*FTNPROTOB*/
void
OlDnDDestroyDropSite (OlDnDDropSiteID dropsiteid)
/*FTNPROTOE*/
{
	OlDnDDropSitePtr		dsp  = (OlDnDDropSitePtr)dropsiteid;
	OlDnDVendorPartExtension	dnd_part;
	Widget				vendor;

	GetToken();
	vendor = _OlGetShellOfWidget(OlDnDDropSitePtrOwner(dsp));
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->delete_drop_site != 
	    (OlDnDVCXDeleteDSProc)NULL)
		(*dnd_part->class_extension->delete_drop_site)
			(vendor, dnd_part, dropsiteid);
	ReleaseToken();
}

/********************************************
 *
 * OlDnDUpdateDropSiteGeometry
 *
 ********************************************/

/*FTNPROTOB*/
Boolean
OlDnDUpdateDropSiteGeometry (OlDnDDropSiteID dropsiteid, OlDnDSiteRectPtr site_rects, unsigned int num_rects)
/*FTNPROTOE*/
{
	OlDnDDropSitePtr		dsp  = (OlDnDDropSitePtr)dropsiteid;
	OlDnDVendorPartExtension	dnd_part;
	Widget				vendor;
	Boolean				retval;

	GetToken();
	vendor = _OlGetShellOfWidget(OlDnDDropSitePtrOwner(dsp));
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->update_drop_site_geometry != 
	    (OlDnDVCXUpdateDSGeometryProc)NULL) {
		retval = (*dnd_part->class_extension->update_drop_site_geometry)
				(vendor, dnd_part, dropsiteid, site_rects, num_rects);
		ReleaseToken();
		return retval;
	} else {
	    ReleaseToken();
	    return (False);
	}
}

/********************************************
 *
 * OlDnDQueryDropSiteInfo
 *
 ********************************************/

/*FTNPROTOB*/
Boolean
OlDnDQueryDropSiteInfo (
			OlDnDDropSiteID dropsiteid,
			Widget *widget, Window *window,
			OlDnDSitePreviewHints *preview_hints,
			OlDnDSiteRectPtr *site_rects,
			unsigned int *num_rects, Boolean *on_interest)
/*FTNPROTOE*/
{
	OlDnDDropSitePtr		dsp = (OlDnDDropSitePtr)dropsiteid;
	OlDnDVendorPartExtension	dnd_part;
	Widget				vendor;
	Boolean				retval;

	GetToken();
	vendor = _OlGetShellOfWidget(OlDnDDropSitePtrOwner(dsp));
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->query_drop_site_info !=
	    (OlDnDVCXQueryDSInfoFunc)NULL) {
		retval = (*dnd_part->class_extension->query_drop_site_info)
				( vendor, dnd_part, dropsiteid, widget, window,
				  preview_hints, site_rects, num_rects,
				  on_interest );
			
	} else retval = False;
	ReleaseToken();
	return retval;
}

/********************************************
 *
 * OlDnDDeliverTriggerMessage
 *
 ********************************************/

static OlDnDVendorPartExtension	horrible_hack = 
					(OlDnDVendorPartExtension)NULL;

/*FTNPROTOB*/
Boolean
OlDnDDeliverTriggerMessage (
			    Widget widget, Window root,
			    Position rootx, Position rooty,
			    Atom selection,
			    OlDnDTriggerOperation operation,
			    Time timestamp)
/*FTNPROTOE*/
{
	OlDnDVendorPartExtension	dnd_part;
	Widget				vendor = _OlGetShellOfWidget(widget);
	Boolean				retval;

	GetToken();
	if (horrible_hack != (OlDnDVendorPartExtension)NULL &&
	    horrible_hack->owner == vendor) {
		dnd_part = horrible_hack;
	} else {
		horrible_hack = dnd_part = _OlGetDnDVendorPartExtension(vendor);
	}

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->deliver_trigger_message !=
	    (OlDnDVCXDeliverTMFunc)NULL)
		retval = (*dnd_part->class_extension->deliver_trigger_message)
			(vendor, dnd_part, widget, root, rootx,
			 rooty, selection, operation, timestamp);
	else retval = False;
	ReleaseToken();
	return retval;
}

/********************************************
 *
 * OlDnDDeliverPreviewMessage
 *
 ********************************************/

/*FTNPROTOB*/
Boolean
OlDnDDeliverPreviewMessage (
			    Widget widget, Window root,
			    Position rootx, Position rooty,
			    Time timestamp)
/*FTNPROTOE*/
{
	OlDnDVendorPartExtension	dnd_part;
	Widget				vendor = _OlGetShellOfWidget(widget);
	Boolean				retval;

	GetToken();
	if (horrible_hack != (OlDnDVendorPartExtension)NULL &&
	    horrible_hack->owner == vendor) {
		dnd_part = horrible_hack;
	} else {
		horrible_hack = dnd_part = _OlGetDnDVendorPartExtension(vendor);
	}

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->deliver_preview_message !=
	    (OlDnDVCXDeliverPMFunc)NULL)
		retval = (*dnd_part->class_extension->deliver_preview_message)
			(vendor, dnd_part, widget, root, rootx,
			 rooty, timestamp, 
			(OlDnDPreviewAnimateCallbackProc)NULL, (XtPointer)NULL);
	else retval = False;
	ReleaseToken();
	return retval;
}

/********************************************
 *
 * OlDnDPreviewAndAnimate
 *
 ********************************************/

/*FTNPROTOB*/
Boolean
OlDnDPreviewAndAnimate (
			Widget widget, Window root,
			Position rootx, Position rooty,
			Time timestamp,
			OlDnDPreviewAnimateCbP animate_proc,
			XtPointer closure)
/*FTNPROTOE*/
{
	OlDnDVendorPartExtension	dnd_part;
	Widget				vendor = _OlGetShellOfWidget(widget);
	Boolean				retval;

	GetToken();
	vendor = _OlGetShellOfWidget(widget);
	if (horrible_hack != (OlDnDVendorPartExtension)NULL &&
	    horrible_hack->owner == vendor) {
		dnd_part = horrible_hack;
	} else {
		horrible_hack = dnd_part = _OlGetDnDVendorPartExtension(vendor);
	}

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->deliver_preview_message !=
	    (OlDnDVCXDeliverPMFunc)NULL)
		retval = (*dnd_part->class_extension->deliver_preview_message)
			(vendor, dnd_part, widget, root, rootx,
			 rooty, timestamp, animate_proc, closure );
	else retval = False;
	ReleaseToken();
	return retval;
}

/********************************************
 *
 * OlDnDInitializeDragState
 *
 ********************************************/

/*FTNPROTOB*/
Boolean
OlDnDInitializeDragState (Widget widget)
/*FTNPROTOE*/
{
	Widget				vendor;
	OlDnDVendorPartExtension	dnd_part;

	GetToken();
	vendor = _OlGetShellOfWidget(widget);

	horrible_hack = dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->initialize_drag_state !=
	    (OlDnDVCXInitializeDragStateFunc)NULL) {
	        ReleaseToken();
		return(*dnd_part->class_extension->initialize_drag_state)
			(vendor, dnd_part);
	}

	ReleaseToken();
	return (False);
}

/********************************************
 *
 * OlDnDClearDragState
 *
 ********************************************/

/*FTNPROTOB*/
void
OlDnDClearDragState (Widget widget)
/*FTNPROTOE*/
{
	OlDnDVendorPartExtension	dnd_part;

	GetToken();
	if (!XtIsVendorShell(widget))
		widget = _OlGetShellOfWidget(widget);

	dnd_part = _OlGetDnDVendorPartExtension(widget);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->clear_drag_state !=
	    (OlDnDVCXClearDragStateProc)NULL) 
		(*dnd_part->class_extension->clear_drag_state)
					    (widget, dnd_part);
	ReleaseToken();
}

/********************************************
 *
 * OlDnDGetCurrentSelectionsForWidget
 *
 ********************************************/

/*FTNPROTOB*/
Boolean     
OlDnDGetCurrentSelectionsForWidget (Widget widget, Atom **atoms_return, Cardinal *num_sites_return)
/*FTNPROTOE*/
{
	Widget			 vendor;
	OlDnDVendorPartExtension dnd_part;

	GetToken();
	vendor = _OlGetShellOfWidget(widget);
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->get_w_current_selections !=
	    (OlDnDVCXGetCurrentSelectionsFunc)NULL)
		*atoms_return = (*dnd_part->class_extension->get_w_current_selections)
					(vendor, dnd_part, widget, num_sites_return);
	else {
		*num_sites_return = (Cardinal)NULL;
		*atoms_return = (Atom *)NULL;
	}
	ReleaseToken();
	return (*atoms_return != (Atom *)NULL);
}

/********************************************
 *
 * OlDnDChangeDropSitePreviewHints
 *
 ********************************************/

/*FTNPROTOB*/
Boolean     
OlDnDChangeDropSitePreviewHints (OlDnDDropSiteID dropsiteid, OlDnDSitePreviewHints hints)
/*FTNPROTOE*/
{
	OlDnDDropSitePtr	 dsp = (OlDnDDropSitePtr)dropsiteid;
	Widget			 vendor;
	OlDnDVendorPartExtension dnd_part;
	Boolean			 retval;

	GetToken();
	vendor = _OlGetShellOfWidget(OlDnDDropSitePtrOwner(dsp));
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->change_site_hints !=
	    (OlDnDVCXChangeSitePreviewHintsFunc)NULL) {
		retval = (*dnd_part->class_extension->change_site_hints)
		    (vendor, dnd_part, dropsiteid, hints);
	} else
		retval = False;
	ReleaseToken();
	return retval;
}

/********************************************
 *
 * OlDnDSetDropSiteInterest
 *
 ********************************************/

/*FTNPROTOB*/
Boolean     
OlDnDSetDropSiteInterest (
			  OlDnDDropSiteID dropsiteid,
			  Boolean on_interest)
/*FTNPROTOE*/
{

	OlDnDDropSitePtr	 dsp = (OlDnDDropSitePtr)dropsiteid;
	Widget			 vendor;
	OlDnDVendorPartExtension dnd_part;
	Boolean			 retval;

	GetToken();
	vendor = _OlGetShellOfWidget(OlDnDDropSitePtrOwner(dsp));
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->set_ds_on_interest !=
	    (OlDnDVCXSetDSOnInterestFunc)NULL) {
		retval = (*dnd_part->class_extension->set_ds_on_interest)
				(vendor, dnd_part, dropsiteid,
				 on_interest, True);
		ReleaseToken();
		return retval;
        } else {
		ReleaseToken();    
		return (False);
	}
}

/********************************************
 *
 * OlDnDSetInterestInWidgetHier
 *
 ********************************************/

/*FTNPROTOB*/
void
OlDnDSetInterestInWidgetHier (Widget widget, Boolean on_interest)
/*FTNPROTOE*/
{

	Widget			 vendor;
	OlDnDVendorPartExtension dnd_part;

	GetToken();
	vendor = _OlGetShellOfWidget(widget);
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->set_interest_in_widget_hier !=
	    (OlDnDVCXSetInterestWidgetHierFunc)NULL) {
		(*dnd_part->class_extension->set_interest_in_widget_hier)
				(vendor, dnd_part, widget, on_interest);
	}
	ReleaseToken();
}

/********************************************
 *
 * OlDnDWidgetConfiguredInHier
 *
 ********************************************/

/*FTNPROTOB*/
void
OlDnDWidgetConfiguredInHier (Widget widget)
       		       
/*FTNPROTOE*/
{

	Widget			 vendor;
	OlDnDVendorPartExtension dnd_part;

	vendor = _OlGetShellOfWidget(widget);
	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL &&
	    dnd_part->class_extension->clip_drop_sites !=
	    (OlDnDVCXClipDropSitesProc)NULL) {
		(*dnd_part->class_extension->clip_drop_sites)
				(vendor, dnd_part, widget);
	}
}

/************************** private functions *****************************/


/************************************************************
 *
 *	_PSErrorHandler
 *
 ************************************************************/

static struct {
		XID		resource_id;
		unsigned long	first_request_protected;
		xErrorHandler	prev_handler;
} _psclosure = { (Window)NULL, (unsigned long)0, (XErrorHandler)NULL };

static int
_PSErrorHandler (Display *dpy, XErrorEvent *xerror)
{
	if (xerror->error_code == BadWindow		&&
	    xerror->resourceid == _psclosure.resource_id &&
	    xerror->serial >= _psclosure.first_request_protected)
		return 0;

	if (_psclosure.prev_handler == (xErrorHandler)NULL) return 0;

	return (*_psclosure.prev_handler)(dpy, xerror);
}

/************************************************************
 *
 *	EnterProtectedSection
 *
 ************************************************************/

static void
EnterProtectedSection (Display *dpy, XID resid, xErrorHandler prev_handler)
{
	if (_psclosure.prev_handler != (xErrorHandler)NULL) return;

	_psclosure.resource_id 		   = resid;
	_psclosure.first_request_protected = NextRequest(dpy);

	_psclosure.prev_handler = XSetErrorHandler(prev_handler);
}

/************************************************************
 *
 *	LeaveProtectedSection
 *
 ************************************************************/

static void
LeaveProtectedSection (Display *dpy)
{
	XSync(dpy, False);

	XSetErrorHandler(_psclosure.prev_handler);

	_psclosure.prev_handler  = (xErrorHandler)NULL;
	_psclosure.resource_id = (XID)NULL;

	_psclosure.first_request_protected = (unsigned long)0;
}

/************************************************************
 *
 *	_StopDnDTxTimer
 *
 ************************************************************/

static void
_StopDnDTxTimer (OwnerProcClosurePtr opc)
{
	if (OwnerProcClosurePtrTimerId(opc) != (XtIntervalId)NULL) {
		XtRemoveTimeOut(OwnerProcClosurePtrTimerId(opc));
		OwnerProcClosurePtrTimerId(opc) = (XtIntervalId)NULL;
	}
}

/************************************************************
 *
 *	_StartDnDTxTimer
 *
 ************************************************************/

static void
_StartDnDTxTimer (OwnerProcClosurePtr opc)
{
	OlDnDVendorPartExtension	dnd_part;
	Widget				vendor;
	
	vendor  = _OlGetShellOfWidget(
			DSSelectionAtomPtrOwner(OwnerProcClosurePtrAssoc(opc)));

	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part == (OlDnDVendorPartExtension)NULL) return;
		
	_StopDnDTxTimer(opc);	/* just in case */
	
	if (OwnerProcClosurePtrTimeoutProc(opc) != (XtTimerCallbackProc)NULL &&
	    dnd_part->dnd_tx_timeout > (unsigned long)0) {

		OwnerProcClosurePtrTimerId(opc) =
			XtAppAddTimeOut(XtWidgetToApplicationContext(vendor),
					dnd_part->dnd_tx_timeout,
					OwnerProcClosurePtrTimeoutProc(opc),
					(XtPointer)opc);
	}
}

/********************************************
 *
 *	_CalculateWidgetDepth
 *
 ********************************************/

static unsigned int
_CalculateWidgetDepth (register Widget w)
{
	unsigned int	depth = 0;

	for (; w != (Widget)NULL && !XtIsSubclass(w, shellWidgetClass);
	     depth++, (w = w->core.parent))
	;

	return (depth);
}

/********************************************
 *
 *	_CheckAncestorAtDepth
 *
 ********************************************/

static Boolean
_CheckAncestorsToDepth (register Widget w, unsigned int w_depth, Widget a, register unsigned int a_depth)
{
	register unsigned int	d;

	if ((int)w_depth - (int)a_depth < 0) return (False);

	for (d = w_depth;
	     d > a_depth && w != (Widget)NULL && w != a;
	     d--, (w = w->core.parent))
	;

	return (a == w && w != (Widget)NULL);
}

/********************************************
 *
 *	_InsertDSInGroup
 *
 ********************************************/

#define	GROUP_DEPTH	3	/* controls group granularity */

static void
_InsertDSInGroup (Widget vendor, OlDnDVendorPartExtension dnd_part, OlDnDDropSitePtr dsp)
{
	register DropSiteGroupPtr	grp = dnd_part->drop_site_groups;
	register unsigned int		n   = dnd_part->num_groups,
					i;
	register OlDnDDropSitePtr	*p;

	for (i = 0; i < n; i++) {
		int deltad = (int)OlDnDDropSitePtrOwnerDepth(dsp) -
		    	     (int)DropSiteGroupLCADepth(grp[i]);

		if (deltad > 0 && deltad <= GROUP_DEPTH &&
		    _CheckAncestorsToDepth(OlDnDDropSitePtrOwner(dsp),
					   OlDnDDropSitePtrOwnerDepth(dsp),
					   DropSiteGroupLCA(grp[i]),
					   DropSiteGroupLCADepth(grp[i])))
			break;	/* found one */
	}

#undef	VECTOR_INCR
#define	VECTOR_INCR	10

	if (i == n) {
		if (VECTORFULL(n, VECTOR_INCR) || n == 0) {
			grp = dnd_part->drop_site_groups =
				(DropSiteGroupPtr) XtRealloc((char *)grp,
							(n + VECTOR_INCR) *
							sizeof(DropSiteGroup));
		}
		dnd_part->num_groups++;

		DropSiteGroupCount(grp[i]) = 0;

		if (OlDnDDropSitePtrOwner(dsp)->core.parent != (Widget)NULL) {
			DropSiteGroupLCA(grp[i])   =
				 OlDnDDropSitePtrOwner(dsp)->core.parent;

			DropSiteGroupLCADepth(grp[i])  =
				OlDnDDropSitePtrOwnerDepth(dsp) - 1;
		} else {
			DropSiteGroupLCA(grp[i]) = OlDnDDropSitePtrOwner(dsp);
			DropSiteGroupLCADepth(grp[i]) = 
				OlDnDDropSitePtrOwnerDepth(dsp);
		}

		DropSiteGroupDropSites(grp[i]) = (OlDnDDropSitePtr)NULL;
	}

	for (p = &DropSiteGroupDropSites(grp[i]);
	     *p != (OlDnDDropSitePtr)NULL &&
	     OlDnDDropSitePtrOwnerDepth(dsp) <= OlDnDDropSitePtrOwnerDepth(*p);
	     p = &OlDnDDropSitePtrNextInGroup(*p))
	; /* insert in ascending order of depth in the tree */

	OlDnDDropSitePtrNextInGroup(dsp) = *p;
	*p = dsp;
	DropSiteGroupCount(grp[i])++;
}

/********************************************
 *
 *	_DeleteDSFromGroup
 *
 ********************************************/

static void
_DeleteDSFromGroup (Widget vendor, OlDnDVendorPartExtension dnd_part, OlDnDDropSitePtr dsp)
{
	register OlDnDDropSitePtr	*p;
	register DropSiteGroupPtr	grp = dnd_part->drop_site_groups;
	register int			i;

	for (i = 0; i < dnd_part->num_groups; i++) {
		int deltad = (int)OlDnDDropSitePtrOwnerDepth(dsp) -
			     (int)DropSiteGroupLCADepth(grp[i]);

		if (deltad > 0 && deltad <= GROUP_DEPTH &&
                    _CheckAncestorsToDepth(OlDnDDropSitePtrOwner(dsp),
                                           OlDnDDropSitePtrOwnerDepth(dsp),
                                           DropSiteGroupLCA(grp[i]),
                                           DropSiteGroupLCADepth(grp[i])))
                        break;  /* found one */
	}

	if (i == dnd_part->num_groups) 
		return; /* something wrong! */

	grp += i;

	for (p = &DropSiteGroupPtrDropSites(grp);
	     *p != (OlDnDDropSitePtr)NULL && *p != dsp;
	     p = &OlDnDDropSitePtrNextInGroup(*p))
	;

	if (*p != (OlDnDDropSitePtr)NULL) {
		*p = OlDnDDropSitePtrNextInGroup(*p);
		DropSiteGroupPtrCount(grp)--;
		OlDnDDropSitePtrNextInGroup(dsp) = (OlDnDDropSitePtr)NULL;

		if (DropSiteGroupPtrCount(grp) == 0) {
			if (i < --dnd_part->num_groups) {
				memcpy((char *)(dnd_part->drop_site_groups + i),
				       (char *)(dnd_part->drop_site_groups +
					       i + 1),
				       sizeof(DropSiteGroup) *
						(dnd_part->num_groups - i));
				grp = dnd_part->drop_site_groups +
				      dnd_part->num_groups;
			}

			if (dnd_part->num_groups) {
				DropSiteGroupPtrCount(grp)     = 0;
				DropSiteGroupPtrLCA(grp)       = (Widget)NULL;
				DropSiteGroupPtrLCADepth(grp)  = 0;
				DropSiteGroupPtrDropSites(grp) =
					(OlDnDDropSitePtr)NULL;
			}
		}
	}
}

/********************************************
 *
 *	_AddWidgetToConfigurePending
 *
 ********************************************/

static void
_RemoveDeadWidgetPendingConfigure(const Widget	  w,
				  const XtPointer clientd,
				  const XtPointer calld)
{
	unsigned int		 i = (unsigned int)clientd;
	Widget			 vendor;
	OlDnDVendorPartExtension dnd_part;

	vendor = _OlGetShellOfWidget(w);

	dnd_part = _OlGetDnDVendorPartExtension(vendor);

	if (dnd_part != (OlDnDVendorPartExtension)NULL	   &&
	    dnd_part->pending_configures != (Widget *)NULL &&
	    dnd_part->num_pending > i			   &&
	    dnd_part->pending_configures[i] == w) {
		dnd_part->pending_configures[i] = (Widget)NULL;
		XtRemoveCallback(w,
				 XtNdestroyCallback,
				 _RemoveDeadWidgetPendingConfigure,
				 (XtPointer)i
		);
	}
}

static void
_AddWidgetToConfigurePending (OlDnDVendorPartExtension dnd_part, Widget widget)
{
	register Widget	*p   = dnd_part->pending_configures;
	register int	num  = dnd_part->num_pending;

	if (widget->core.being_destroyed)
		return;

	if (num == 0 || VECTORFULL(num, VECTOR_INCR)) {
		dnd_part->pending_configures = 
		p =  (Widget *)XtRealloc((char *)p,
				 (num + VECTOR_INCR) * sizeof(Widget));
	}

	for (; num > 0; p++, num--) if (*p == widget) return;

	dnd_part->pending_configures[dnd_part->num_pending] = widget;

	XtAddCallback(widget,
		      XtNdestroyCallback, 
		      _RemoveDeadWidgetPendingConfigure,
		      (XtPointer)dnd_part->num_pending++
	);
}
#undef	VECTOR_INCR

/********************************************
 *
 *	_DoPendingConfigures
 *
 ********************************************/

typedef struct _wad {
	Widget*		w;
	unsigned int*	d;
} WAD, *WADPtr;

static int
_comparDepths(const void *i, const void *j)
{
	WADPtr	ip = (WADPtr)i,
		jp = (WADPtr)j;

	return ((int)(*(ip->d)) - (int)(*(jp->d)));
}

static void
_DoPendingConfigures (Widget vendor, OlDnDVendorPartExtension dnd_part)
{
	register int			i;
	WAD				wad_cache[20];
	WADPtr				arry = wad_cache,
					wadp;
	OlDnDVCXClipDropSitesProc	proc;
	unsigned int			cw_depth;
	unsigned int			d_cache[XtNumber(wad_cache)],
					*darry = d_cache,
					*dp;

	/* avoid recursion through call to clip_drop_sites */

	if (++dnd_part->doing_clipping_already) {
	     --dnd_part->doing_clipping_already;
	     return;
	}

	if (dnd_part->num_pending == 0 ||
	    dnd_part->disable_ds_clipping) {
		--dnd_part->doing_clipping_already;
		return;
	}

	proc = dnd_part->class_extension->clip_drop_sites;

	if (proc == (OlDnDVCXClipDropSitesProc)NULL) {

		XtFree((char *)dnd_part->pending_configures);
		dnd_part->pending_configures = (Widget *)NULL;
		dnd_part->num_pending        = 0;
		dnd_part->configuring_widget = (Widget)NULL;

		--dnd_part->doing_clipping_already;
		return;
	}

	if (dnd_part->num_pending > XtNumber(wad_cache)) {
		arry = (struct _wad *)XtCalloc(dnd_part->num_pending,
					       sizeof(struct _wad) + 
					       sizeof(unsigned int));
		darry = (unsigned int *)(arry +
					 dnd_part->num_pending *
					 sizeof(struct _wad)
					);
	}

	wadp = arry;
	dp   = darry;

	for (i = 0; i < dnd_part->num_pending; i++, wadp++, dp++) {
		wadp->w		= &(dnd_part->pending_configures[i]);
		*(wadp->d = dp) = _CalculateWidgetDepth(*(wadp->w));
	}
		
	if (dnd_part->num_pending > 1) {
		qsort((char *)arry, dnd_part->num_pending,
		      sizeof(WAD), _comparDepths);
	}

	for (i = 0; i < dnd_part->num_pending; i++) {
		register int	j;

		if (*(arry[i].w) == (Widget)NULL) continue;

		XtRemoveCallback(*(arry[i].w),
				 XtNdestroyCallback,
				 _RemoveDeadWidgetPendingConfigure,
				 (XtPointer)i
		);

		for (j = i + 1; j < dnd_part->num_pending; j++) {
			if (arry[j].w != (Widget*)NULL &&
			    _CheckAncestorsToDepth(*(arry[j].w), *(arry[j].d),
						   *(arry[i].w), *(arry[i].d)))

			    XtRemoveCallback(*(arry[j].w),
					     XtNdestroyCallback,
					     _RemoveDeadWidgetPendingConfigure,
					     (XtPointer)i
			    );

			    *(arry[j].w) = (Widget)NULL;
		}
	}

	cw_depth = _CalculateWidgetDepth(dnd_part->configuring_widget);

	for (i = 0; i < dnd_part->num_pending; i--) {
		if (dnd_part->pending_configures[i] != (Widget)NULL &&
		    !_CheckAncestorsToDepth(dnd_part->pending_configures[i],
					    darry[i],
					    dnd_part->configuring_widget,
					    cw_depth)) {
			(*proc)(vendor, dnd_part,
				dnd_part->pending_configures[i]);
		}
	}
		
	XtFree((char *)dnd_part->pending_configures);

	if (arry != wad_cache) XtFree((char *)arry);

	dnd_part->pending_configures = (Widget *)NULL;
	dnd_part->num_pending        = 0;
	dnd_part->configuring_widget = (Widget)NULL;

	--dnd_part->doing_clipping_already;
}
/********************************************
 *
 *	_CalculateClipAndTransform
 *
 ********************************************/

#define	MAYBEVISIBLE(w)							  \
		(XtIsRealized(w)				       && \
		((XtIsWidget((w))				       && \
		 (((w)->core.widget_class->core_class.visible_interest && \
		  (w)->core.visible)				       || \
		  (XtIsShell(w)					       || \
		  (XtIsManaged(w) && (w)->core.mapped_when_managed)))) || \
		XtIsManaged(w)))

static Region
_CalculateClipAndTransform (register Widget root, unsigned int r_depth, register Widget w, unsigned int w_depth, Position *x_trans_ret, Position *y_trans_ret, Boolean *visibility_hint)
{
	Region			reg = XCreateRegion();
	Widget			stack_cache[100],
				*arry = stack_cache,
				*siblings;
	register Widget		*p = arry,
				*s;
	int			num = (int)w_depth - (int)r_depth + 1;
	register Position	x_trans = (Position)0,
				y_trans = (Position)0;
	Boolean	 		probably_not_visible = False;
	XRectangle		rect;

	if (num <= 0 || root == (Widget)NULL || w == (Widget)NULL) {
		*visibility_hint = False;
		return (reg);
	}

	if (num > XtNumber(stack_cache))
		p = arry = (Widget *)XtCalloc(num, sizeof(Widget));

	for (; w != (Widget)NULL; w = w->core.parent) {
		*p++ = w;

		probably_not_visible = !MAYBEVISIBLE(w);

		if (w == root || XtIsShell(w) || probably_not_visible) break;
	}
	w = *(--p);

	if (probably_not_visible) {
		if (x_trans_ret != (Position *)NULL) *x_trans_ret = x_trans;
		if (y_trans_ret != (Position *)NULL) *y_trans_ret = y_trans;
	
		*visibility_hint = False;
		return (reg);
	}



	for (;;) {
		Region		rr,
				rs     = (Region)NULL;
		Dimension	wbw    = (Dimension)0;
		Widget		parent = XtParent(w);
		Position	wx,
				wy;

		if (w != root) {
			rect.x = (wx = w->core.x) + x_trans;
			rect.y = (wy = w->core.y) + y_trans;
		} else
			wx = wy = rect.x = rect.y = (Position)0;

		rect.width  = w->core.width;
		rect.height = w->core.height;

		rr = XCreateRegion();

		XUnionRectWithRegion(&rect, rr, rr);
		if (w != root)
			XIntersectRegion(reg, rr, reg);
		else
			XUnionRegion(reg, rr, reg);

		XDestroyRegion(rr);

		wbw 	  = w->core.border_width;
		x_trans  += wx + wbw;
		y_trans  += wy + wbw;

		if (p == arry) break;

		w = *(--p);
	}

	if (p != stack_cache) XtFree((char *)arry);

	if (x_trans_ret != (Position *)NULL) *x_trans_ret = x_trans;
	if (y_trans_ret != (Position *)NULL) *y_trans_ret = y_trans;

	if (XEmptyRegion(reg))
		*visibility_hint = False;
	else
		*visibility_hint = True;

	return (reg);
}
#undef	MAYBEVISIBLE

/********************************************
 *
 *	GetShellAndParentOfWindow
 *
 ********************************************/

static void
GetShellAndParentOfWindow (Display *dpy, Window window,
			   Widget *vendor, Widget *owner)
{
	Widget				widget;
	Window				root, parent, w, *children;
	unsigned int			nchildren;

	for (w = window;
	     w != (Window)NULL &&
	     (widget = XtWindowToWidget(dpy, w)) != (Widget)NULL;
	     w = parent) {
		int	ret;

		ret = XQueryTree(dpy, w, &root, &parent,
				 &children, &nchildren);

		if (children != (Window *)NULL) XtFree((char *)children);

		if (!ret) break;
	} 

	if (owner != (Widget *)NULL) *owner = widget;

	if (vendor != (Widget *)NULL) {
		*vendor = (widget != (Widget)NULL ? _OlGetShellOfWidget(widget)
						  : widget);
	}
}

/********************************************
 *
 * InsertSiteRectsInDropSiteList
 *
 * insert the list of drop sites into the
 * dispatcher list
 *
 ********************************************/

#define	EQSITE(s1, s2)						\
	((InternalDSRPtrTopLevelRectX(s1) ==			\
	  InternalDSRPtrTopLevelRectX(s2))		&&	\
	 (InternalDSRPtrTopLevelRectY(s1) ==			\
	  InternalDSRPtrTopLevelRectY(s2))		&&	\
	 (InternalDSRPtrTopLevelRectWidth(s1) ==		\
	  InternalDSRPtrTopLevelRectWidth(s2))	&&		\
	 (InternalDSRPtrTopLevelRectHeight(s1) ==		\
	  InternalDSRPtrTopLevelRectHeight(s2)))

#define	GESITE(s1, s2)						\
	((InternalDSRPtrTopLevelRectX(s1) >=			\
	  InternalDSRPtrTopLevelRectX(s2))		&&	\
	 (InternalDSRPtrTopLevelRectY(s1) >=			\
	  InternalDSRPtrTopLevelRectY(s2))		&&	\
	 (InternalDSRPtrTopLevelRectWidth(s1) >=		\
	  InternalDSRPtrTopLevelRectWidth(s2))	&&		\
	 (InternalDSRPtrTopLevelRectHeight(s1) >=		\
	  InternalDSRPtrTopLevelRectHeight(s2)))


static	void
InsertSiteRectsInDropSiteList (OlDnDVendorPartExtension dnd_part, InternalDSRPtr sites, unsigned int num_sites)
{
	for (; num_sites--; sites++) {
		InternalDSRPtr	*rect = &dnd_part->dropsite_rects, next;

		for (; *rect; rect = &InternalDSRPtrNext(*rect))
			if (GESITE(sites, *rect)) break;

		if (*rect && EQSITE(sites, *rect)) continue;
		
		next = *rect;
		*rect = sites;
		InternalDSRPtrNext(sites) = next;

		if (next) {
			InternalDSRPtrPrev(sites) = InternalDSRPtrPrev(next);
			InternalDSRPtrPrev(next)  = sites;
		} else 
			InternalDSRPtrPrev(sites) = (InternalDSRPtr)NULL;
	}
}

#undef	EQSITE
#undef	GESITE

/********************************************
 *
 * LookupXYInDropSiteList
 *
 * find an InternalDSR which contains
 * the (x,y) pair.
 *
 ********************************************/

static InternalDSRPtr
LookupXYInDropSiteList (OlDnDVendorPartExtension dnd_part, int x, int y)
{
	InternalDSRPtr	*site = &dnd_part->dropsite_rects;

	if (dnd_part->current_dsr != (InternalDSRPtr)NULL &&
	    XYInInternalDSR(dnd_part->current_dsr, x, y))
			return (dnd_part->current_dsr);
		
	while (*site) {
		if (XYInInternalDSR(*site, x, y))
			break;

		site = &InternalDSRPtrNext(*site);
	}
	return (*site);
}

/********************************************
 *
 * DeleteSiteRectsInDropSiteLIst
 *
 * need we say more ?
 *
 ********************************************/

static	void
DeleteSiteRectsInDropSiteList (OlDnDVendorPartExtension dnd_part, InternalDSRPtr sites, unsigned int num_sites)
{
	for (; num_sites--; sites++) {
		InternalDSRPtr	*rect = &dnd_part->dropsite_rects, next;

		for (; *rect && *rect != sites;
		     rect = &InternalDSRPtrNext(*rect))
		;

		if (*rect) {
			next = InternalDSRPtrNext(sites);
			*rect = next;
			if (next)
				InternalDSRPtrPrev(next) =
					InternalDSRPtrPrev(sites);

			InternalDSRPtrNext(sites) = 
				InternalDSRPtrNext(sites) =
					(InternalDSRPtr)NULL;

			/* invalidate dsr cache */

			if (dnd_part->current_dsr == sites)
				dnd_part->current_dsr = (InternalDSRPtr)NULL;

		}
	}
}

/********************************************
 *
 * InsertSiteRectsInDropSiteList
 *
 * insert the list of drop sites into the
 * DSDM list
 *
 ********************************************/

#define	EQSITE(s1, s2)					\
	((InternalDSDMSRPtrRectX(s1) ==			\
	  InternalDSDMSRPtrRectX(s2))		 &&	\
	 (InternalDSDMSRPtrRectY(s1) ==			\
	  InternalDSDMSRPtrRectY(s2))		 &&	\
	 (InternalDSDMSRPtrRectWidth(s1) ==		\
	  InternalDSDMSRPtrRectWidth(s2))	 &&	\
	 (InternalDSDMSRPtrRectHeight(s1) ==		\
	  InternalDSDMSRPtrRectHeight(s2)))

#define	GESITE(s1, s2)					\
	((InternalDSDMSRPtrRectX(s1) >=			\
	  InternalDSDMSRPtrRectX(s2))		&&	\
	 (InternalDSDMSRPtrRectY(s1) >=			\
	  InternalDSDMSRPtrRectY(s2))		&&	\
	 (InternalDSDMSRPtrRectWidth(s1) >=		\
	  InternalDSDMSRPtrRectWidth(s2))	&&	\
	 (InternalDSDMSRPtrRectHeight(s1) >=		\
	  InternalDSDMSRPtrRectHeight(s2)))


static	void
InsertDSDMSRsIntoDSDMSRList (OlDnDVendorPartExtension dnd_part, InternalDSDMSRPtr dsdmsrs, unsigned int num_dsdmsrs)
{
	unsigned int		noofscreens, screen;

	noofscreens = ScreenCount(XtDisplayOfObject(dnd_part->owner));

	if (dnd_part->dsdm_rects == (InternalDSDMSRPtr *)NULL) {
		dnd_part->dsdm_rects = 
				(InternalDSDMSRPtr *)XtCalloc(noofscreens,
						sizeof(InternalDSDMSRPtr));
	}

	for (; num_dsdmsrs--; dsdmsrs++) {
		InternalDSDMSRPtr	*rect = dnd_part->dsdm_rects, next;

		screen = InternalDSDMSRPtrRectScreenNumber(dsdmsrs);
		if (screen >= 0 && screen < noofscreens) {
			rect += screen;
		} else continue;

		for (; *rect; rect = &InternalDSDMSRPtrNext(*rect))
			if (GESITE(dsdmsrs, *rect)) break;

		if (*rect && EQSITE(dsdmsrs, *rect)) continue;
		
		next = *rect;
		*rect = dsdmsrs;
		InternalDSDMSRPtrNext(dsdmsrs) = next;

		if (next) {
			InternalDSDMSRPtrPrev(dsdmsrs) =
					InternalDSDMSRPtrPrev(next);
			InternalDSDMSRPtrPrev(next)  = dsdmsrs;
		} else 
			InternalDSDMSRPtrPrev(dsdmsrs) =
					(InternalDSDMSRPtr)NULL;
	}
}

#undef	EQSITE
#undef	GESITE

/********************************************
 *
 * LookupXYInDSDMSRList
 *
 * find an InternalDSDMSR which contains
 * the (x,y) pair.
 *
 ********************************************/

static InternalDSDMSRPtr
LookupXYInDSDMSRList (OlDnDVendorPartExtension dnd_part, Screen *screen, int x, int y)
{
	InternalDSDMSRPtr	*site = dnd_part->dsdm_rects;
	unsigned int		noofscreens, scrn;

	noofscreens = ScreenCount(XtDisplayOfObject(dnd_part->owner));
	scrn = XScreenNumberOfScreen(screen);

	if (dnd_part->current_dsdmsr != (InternalDSDMSRPtr)NULL &&
	    XYInInternalDSDMSR(dnd_part->current_dsdmsr, scrn, x, y))
		return (dnd_part->current_dsdmsr);

	if (dnd_part->dsdm_rects == (InternalDSDMSRPtr *)NULL)
                return (InternalDSDMSRPtr)NULL;

	if (scrn >= 0 && scrn < noofscreens) {
		site += scrn;
	} else  return (InternalDSDMSRPtr)NULL;

	if (*site == (InternalDSDMSRPtr)NULL)
                return (InternalDSDMSRPtr)NULL;

	while (*site) {
		if (XYInInternalDSDMSR(*site, scrn, x, y))
			break;

		site = &InternalDSDMSRPtrNext(*site);
	}
	return (*site);
}

/********************************************
 *
 * DeleteDSDMSRList
 *
 * need we say more ?
 *
 ********************************************/

static	void
DeleteDSDMSRList (OlDnDVendorPartExtension dnd_part)
{
	InternalDSDMSRPtr	*site = dnd_part->dsdm_rects;
	unsigned int		i, noofscreens;
   
	if (dnd_part->dsdm_rects == (InternalDSDMSRPtr *)NULL) return;
   
	noofscreens = ScreenCount(XtDisplayOfObject(dnd_part->owner));

	for (; site < dnd_part->dsdm_rects + noofscreens; site++) {
		if (*site != (InternalDSDMSRPtr)NULL) {
			InternalDSDMSRPtr	ptr = *site, next;
		  
			while (ptr) {
				next = InternalDSDMSRPtrNext(ptr);
				InternalDSDMSRPtrPrev(ptr) =
					InternalDSDMSRPtrNext(ptr) =
						(InternalDSDMSRPtr)NULL;
				ptr = next;
			  }
		}
	}

	XtFree((char *)dnd_part->dsdm_rects);
	dnd_part->dsdm_rects = (InternalDSDMSRPtr *)NULL;
	dnd_part->current_dsdmsr = (InternalDSDMSRPtr)NULL;
}

/************************************************************
 *
 *	DnDFetchRootCoords()
 *
 *	update the extension instances root_x and root_y 
 * 	resources.
 *
 ************************************************************/

static void
DnDFetchRootCoords (Widget vendor, OlDnDVendorPartExtension dnd_part)
{
	int	rx, ry;
	Screen	*screen = vendor->core.screen;
	Window	child;

	FetchPartExtensionIfNull(vendor, dnd_part);

	if (XTranslateCoordinates(DisplayOfScreen(screen), XtWindow(vendor), 
				   RootWindowOfScreen(screen), 0, 0,
				   &rx, &ry, &child)) {
		dnd_part->root_x = (Position)rx;
		dnd_part->root_y = (Position)ry;
	}
}

/************************************************************
 *
 *	MapLocalCoordsToTopLevel()
 *
 *	Map the site rect coords in the local coordinate
 *	space to theat of the vendor vendor
 *
 *	NOTE: widgets should be realised before calling this
 *	      function ....
 *
 *
 *	ALSO NOTE THIS FUNCTION IS NOW SEMI-REDUNDANT 
 *	WITH THE INCLUSION OF DS CLIPPING
 *
 ************************************************************/

static void
MapLocalCoordsToTopLevel (Widget vendor, OlDnDVendorPartExtension dnd_part, Widget widget, Window window, OlDnDSiteRectPtr site_rect, OlDnDSiteRectPtr return_rect)
{
	int	x, y;

	FetchPartExtensionIfNull(vendor, dnd_part);

	x = (int)SiteRectPtrX(site_rect);
	y = (int)SiteRectPtrY(site_rect);

	if (window != XtWindowOfObject(widget)) {	/* client window */
		Window		dummy;

		XTranslateCoordinates(XtDisplay(vendor), window, 
				      XtWindow(vendor), x, y, &x, &y, 
				      &dummy);
	} else { /* widget or gadget */
		register RectObj rect = (RectObj)widget;

#define		Adjust(g)	\
		(g += (rect->rectangle.g + rect->rectangle.border_width))

		for (rect = (RectObj)widget;
		     rect != (RectObj)vendor;
		     rect = (RectObj)XtParent((Widget)rect)) {
			Adjust(x);
			Adjust(y);
		}
	}

	SiteRectPtrX(return_rect)      = (Position)x;
	SiteRectPtrY(return_rect)      = (Position)y;
	SiteRectPtrWidth(return_rect)  = SiteRectPtrWidth(site_rect);
	SiteRectPtrHeight(return_rect) = SiteRectPtrHeight(site_rect);
}

/***********************************************************************
 *
 * DnDVCXEventHandler
 *
 * This is the event handler which dispatches incoming events on Vendor 
 * Shell widgets for the purposes of Drag and Drop.
 *
 *************************************************************************/

#define	VCXEventHandlerMask	(PropertyChangeMask | \
				 FocusChangeMask    | \
				 StructureNotifyMask)

static void
DnDVCXEventHandler (Widget widget, XtPointer client_data, XEvent *xevent, Boolean *continue_to_dispatch)
{
	Widget				vendor;
	OlDnDVendorPartExtension	dnd_part;
	OlDnDVendorClassExtension	dnd_class;

	if ((vendor = _OlGetShellOfWidget(widget)) == widget) {    
		dnd_part = (OlDnDVendorPartExtension)client_data;
	} else {
		dnd_part = _OlGetDnDVendorPartExtension(vendor);
	}

	dnd_class = _OlGetDnDVendorClassExtension(vendor->core.widget_class);
	
	if (dnd_class == (OlDnDVendorClassExtension)NULL || 
	    dnd_part  == (OlDnDVendorPartExtension)NULL) {
			return; /* its all gone horribly wrong! */
	}

	switch (xevent->xany.type) {
		case ClientMessage:

			if (xevent->xclient.message_type == 
			    OlInternAtom(xevent->xany.display,
					 _SUN_DRAGDROP_PREVIEW_NAME)) {
				PreviewMessage	preview;

				CopyPreviewMessageFromClientMessage(&preview,
							    &(xevent->xclient));

				if (dnd_class->preview_message_dispatcher !=
				    (OlDnDVCXPMDispatcherFunc)NULL) {
					(*dnd_class->preview_message_dispatcher)
						(vendor, dnd_part, &preview);
				}
				return;
			} /* preview message */

			if (xevent->xclient.message_type ==
			    OlInternAtom(xevent->xany.display,
					 _SUN_DRAGDROP_TRIGGER_NAME)) {
				TriggerMessage	trigger;

				CopyTriggerMessageFromClientMessage(&trigger,
							    &(xevent->xclient));

				if (dnd_class->trigger_message_dispatcher !=
				    (OlDnDVCXTMDispatcherFunc)NULL) {
					(*dnd_class->trigger_message_dispatcher)
						(vendor, dnd_part, &trigger);
				}

				return;
			} /* trigger message */

			return;	/* must be something else .... */

		case PropertyNotify:
		    if (xevent->xproperty.atom ==
			OlInternAtom(xevent->xany.display,
				     _SUN_DRAGDROP_INTEREST_NAME) &&
		        xevent->xproperty.state == PropertyNewValue) {
			dnd_part->registry_update_timestamp =
							 xevent->xproperty.time;

			if (dnd_part->dirty &&
			    dnd_part->auto_assert_dropsite_registry &&
			    dnd_class->assert_drop_site_registry !=
			    (OlDnDVCXAssertRegistryProc)NULL) {
				(*dnd_class->assert_drop_site_registry)
					(vendor, dnd_part);
			} /* its changed since we last updated it!!! phew ... */
		    } /* PropertyNotify */
                    return;
 
#if     0
                case FocusIn:
                        if (dnd_part->class_extension->fetch_dsdm_info !=
                            (OlDnDVCXFetchDSDMInfoFunc)NULL) {
                                (*dnd_part->class_extension->fetch_dsdm_info)
                                                (vendor, dnd_part,
                                                 XtLastTimestampProcessed(
                                                 XtDisplay(vendor)));
                        }
                        return;
#endif

	}
}

/******************************************************************************
 *
 * FindDropSiteInfoFromDestination
 *
 ******************************************************************************/

static	Boolean
FindDropSiteInfoFromDestination (Screen *screen, Window root_window, int root_x, int root_y, SiteDescriptionPtr *site_found)
{
	Window			w, child,
				toplevel = (Window)NULL;
	Window			*children;
	unsigned int		nchildren;
	Display			*dpy = DisplayOfScreen(screen);
	int			x, y;
	int			top_x, top_y;
	unsigned int		i;

	Atom			actual_type;
	int			actual_format;
	unsigned long		nitems;
	unsigned long		bytes_remaining;
	InterestPropertyPtr	interest = (InterestPropertyPtr)NULL;
	SiteDescriptionPtr	site = (SiteDescriptionPtr)NULL;
	unsigned int		sizeof_site = 0;
	Atom			interest_atom;

	/*
	 * welcome to an expensive function ... so whats a few round trips
	 * between clients and servers?
	 */

	children = (Window*)XtMalloc(sizeof(Window));
	children[0] = root_window;
	nchildren = 1;

	interest_atom = OlInternAtom(dpy, _SUN_DRAGDROP_INTEREST_NAME);

	for (;;) {
		Atom		*props;
		unsigned int	num_props, propsfound = 0;

		for (i = 0; i < nchildren; i++) {
			w = children[i];
			if (!XTranslateCoordinates(dpy, root_window,
						   w, root_x, root_y,
						   &x, &y, &child))
				continue;

			if (child != None) break;
		}

		if(children) {
			XFree((char *)children);
			children = NULL;
		}

		if(child != None)
			props =	XListProperties(dpy, w, (int *)&num_props);
		else {
		 	num_props = 0; 
                        props = NULL; 
                }

		for (i = 0; i < num_props; i++) {
			if (props[i] == interest_atom) {
				if (propsfound++) 
					break; 
				else continue;
			}
			if (props[i] == 
			    OlInternAtom(dpy, WM_STATE_NAME)) {
				if (propsfound++) 
					break; 
				else continue;
			}
		}

		if (props) {
			XFree((char *)props);
			props = NULL;
		}

		if (propsfound == 2) {
			toplevel = w;
			break;
		}

		if (child == None ||
		    !XQueryTree(dpy, child, &root_window, &w,
				&children, &nchildren) ||
				!children ||
				!nchildren)
			break; /* bale out */
	}

	if (toplevel == (Window)NULL) 
		toplevel = root_window;

	if (XGetWindowProperty(dpy, toplevel, interest_atom, 0L, 1L,
			      False, interest_atom, &actual_type,
			      &actual_format, &nitems, &bytes_remaining, 
			      (unsigned char **)&interest))
		goto failed;

	if (XGetWindowProperty(dpy, toplevel, interest_atom,0L,
			      1 + bytes_remaining / 4, False, actual_type,
			      &actual_type, &actual_format, &nitems,
			      &bytes_remaining,
			      (unsigned char **)&interest))
		goto failed;

	if (interest == (InterestPropertyPtr)NULL || 
	    InterestPropertyPtrVersionNumber(interest) != 0)
		goto failed; /* bad version */

	if (!XTranslateCoordinates(dpy, toplevel, root_window,
				   0, 0, &top_x, &top_y, &w)) {
	}

	x = root_x - top_x; /* adjust co-ordinates */
	y = root_y - top_y;

	for (i = 0, site = InterestPropertyPtrSiteDescriptions(interest);
	     i < InterestPropertyPtrSiteCount(interest); i++) {
		unsigned int		p, offset;
		AreaListPtr		area = &SiteDescriptionPtrAreas(site);

		if (SiteDescriptionPtrAreaIsRectList(site)) {
			unsigned int	nrects = NumOfRectsInAreaListPtr(area);
			RectListPtr	rect = &AreaListPtrRectList(area);

			offset = SizeOfSiteDescriptionForNRects(nrects);

			for (p = 0; p < nrects; p++) {
			    if ((x >= SiteRectX(RectListPtrRects(rect)[p]) &&
			         x <= (SiteRectX(RectListPtrRects(rect)[p]) +
				      SiteRectWidth(RectListPtrRects(rect)[p]))) &&
			       (y >= SiteRectY(RectListPtrRects(rect)[p]) &&
			        y <= (SiteRectY(RectListPtrRects(rect)[p]) +
				      SiteRectHeight(RectListPtrRects(rect)[p])))) {
					sizeof_site = offset;
					goto found;
			     }
			}
		} else if (SiteDescriptionPtrAreaIsWindowList(site)) {
			XWindowAttributes	wa;
			unsigned int		nwindows =
						NumOfWindowsInAreaListPtr(area);
			WindowListPtr		windows =
						&AreaListPtrWindowList(area);	

			for (p = 0; p < nwindows; p++ ) {
				int	wx, wy;

				if (XGetWindowAttributes(dpy, 
					WindowListPtrWindows(windows)[p], &wa)
				    != Success)
					continue;

				if (!XTranslateCoordinates(dpy, 
					WindowListPtrWindows(windows)[p],
					root_window, 0, 0, &wx, &wy, &w))
					continue;

				if ((x >= wx && x <= (wx + wa.width)) &&
				    (y >= wy && y <= (wy + wa.height))) {
					sizeof_site = offset;
					goto found;
				}
			}

			offset = SizeOfSiteDescriptionForNWindows(nwindows);
		} else {
			site = (SiteDescriptionPtr)NULL;
			break; 
		}

		site = (SiteDescriptionPtr)((unsigned char *)site + offset);
	}

found:	
	if (interest != (InterestPropertyPtr)NULL &&
	    i < InterestPropertyPtrSiteCount(interest)) {
		*site_found = (SiteDescriptionPtr)XtCalloc(1, sizeof_site);
		memcpy((char *)*site_found, (char *)site, sizeof_site);
	} else 
		failed: site = *site_found = (SiteDescriptionPtr)NULL;

	if (interest != (InterestPropertyPtr)NULL)
		XFree((char *)interest);

	return (site != (SiteDescriptionPtr)NULL);
}


/********************************************
 *
 * _OlDnDAssocSelectionWithWidget
 *
 ********************************************/

static DSSelectionAtomPtr
_OlDnDAssocSelectionWithWidget (Widget vendor, OlDnDVendorPartExtension dnd_part, Widget widget, Atom selection, Time timestamp, OwnerProcClosurePtr closure)
{
	DSSelectionAtomPtr	sel;

	for (;;) {
		if (dnd_part->class_extension->associate_selection_and_w !=
		    (OlDnDVCXAssocSelectionFunc)NULL)
			sel = (*dnd_part->class_extension->associate_selection_and_w)
					(vendor, dnd_part, widget, selection, timestamp, closure);

		if (sel != (DSSelectionAtomPtr)NULL &&
		    DSSelectionAtomPtrOwner(sel) != widget)
			OlDnDDisownSelection(DSSelectionAtomPtrOwner(sel),
					     selection, timestamp);
		else
			return (sel);
	}
}
	
/********************************************
 *
 * _OlDnDDisassocSelectionWithWidget
 *
 ********************************************/

static void
_OlDnDDisassocSelectionWithWidget (Widget vendor, OlDnDVendorPartExtension dnd_part, Widget widget, Atom selection, Time timestamp)
{
	if (dnd_part->class_extension->disassociate_selection_and_w !=
		    (OlDnDVCXDisassocSelectionProc)NULL)
		(*dnd_part->class_extension->disassociate_selection_and_w)
			(vendor, dnd_part, widget, selection, timestamp);

}

/********************************************
 *
 * _OlDnDGetWidgetForSelection
 *
 ********************************************/

static Widget
_OlDnDGetWidgetForSelection (Widget vendor, OlDnDVendorPartExtension dnd_part, Atom selection, OwnerProcClosurePtr *closure)
{
	if (dnd_part->class_extension->get_w_for_selection !=
	    (OlDnDVCXGetSelectionFunc)NULL)
		return (*dnd_part->class_extension->get_w_for_selection)
				(vendor, dnd_part, selection, closure);
	else {
		*closure = (OwnerProcClosurePtr)NULL;
		return (Widget)NULL;
	}
}

/********************************************
 *
 * _OlDnDAtomIsTransient
 *
 ********************************************/

static Boolean
_OlDnDAtomIsTransient (Widget vendor, OlDnDVendorPartExtension dnd_part, Atom atom)
{
	TransientAtomListPtr	transients;
	unsigned int		i;

	if ((transients = dnd_part->transient_atoms) !=
	    (TransientAtomListPtr)NULL) {
		for (i = 0; i < TransientAtomListPtrAlloc(transients); i++)
			if (atom == TransientAtomListPtrAtom(transients, i)) {
				return (True);
			}
	}
	return (False);
}

/********************************************
 *
 * _OlResourceDependencies
 *
 * this function bears some resemblance
 * to _XtDependencies() except that it
 * doesnt create the indirection table
 * since XtGetSubresources does this for 
 * you ...... blah ....
 *
 ********************************************/

void
_OlResourceDependencies (XtResourceList *class_res, Cardinal *class_num_res, XrmResourceList super_res, Cardinal super_num_res, Cardinal super_ext_size)
{
	register Cardinal	i,j;
	Cardinal		new_next;
	Cardinal		new_num_res;
	XrmResourceList		new_res, class_xrm_res;

	if (*class_res != (XtResourceList)NULL &&
	    (int)(*class_res)->resource_offset > 0) {
		_XtCompileResourceList(*class_res, *class_num_res);
	}
	class_xrm_res = (XrmResourceList)*class_res;

	new_num_res = super_num_res + *class_num_res;
	if (new_num_res > 0) {
		new_res = (XrmResourceList)XtCalloc(new_num_res, 
					            sizeof(XrmResource));
	} else
		new_res = (XrmResourceList)NULL;

	if (super_res != (XrmResourceList)NULL) {
		memcpy((char *)new_res, (char *)super_res,
		       super_num_res * sizeof(XrmResource));
	}

	if (*class_res == (XtResourceList)NULL) {
		*class_res = (XtResourceList)new_res;
		*class_num_res = new_num_res;
		return;
	}

	new_next = super_num_res;
	for (i = 0; i < *class_num_res; i++) {
		if (-class_xrm_res[i].xrm_offset-1 < super_ext_size) {
			for (j = 0; j < super_num_res; j++) {
				if (class_xrm_res[i].xrm_offset ==
				    new_res[j].xrm_offset) {
					class_xrm_res[i].xrm_size =
						new_res[j].xrm_size;

					new_res[j] = class_xrm_res[i];
					new_num_res--;
					goto NextResource;
				}
			} /* for j */
		} /* if */
		new_res[new_next++] = class_xrm_res[i];
NextResource:;
	} /* for i */

	*class_res = (XtResourceList)new_res;
	*class_num_res = new_num_res;
}

/********************************* Class Methods *****************************/

/*******************************************
 *
 * DnDVCXClassInitialize
 *
 *******************************************/

static void
DnDVCXClassInitialize (OlDnDVendorClassExtension extension)
{
	OlXrmDnDVendorClassExtension =
                        XrmStringToQuark(OlDnDVendorClassExtensionName);

        extension->header.record_type = OlXrmDnDVendorClassExtension;
}

/*******************************************
 *
 * DnDVCXClassPartInitialize
 *
 *******************************************/

static void
DnDVCXClassPartInitialize (WidgetClass wc)
{
        Cardinal                	size;
        OlDnDVendorClassExtension       super_ext;
        OlDnDVendorClassExtension       ext;
	Cardinal			super_num_res;
	Cardinal			super_ext_size;
	XrmResourceList			super_res;
	Boolean				super_null;
	Boolean				copied_super;

	if (wc != vendorShellWidgetClass)
		super_ext = GET_DND_EXT(wc->core_class.superclass);
	else
		super_ext = (OlDnDVendorClassExtension)NULL;

	ext =       GET_DND_EXT(wc);

	if ((super_null = (super_ext == (OlDnDVendorClassExtension)NULL)) &&
	    ext == (OlDnDVendorClassExtension)NULL) {
		return;	/*oops*/
	}

	if ((copied_super = (ext == (OlDnDVendorClassExtension)NULL))) {
                size = super_ext->header.record_size;
                ext = (OlDnDVendorClassExtension)XtMalloc(size);

                (void)memcpy((char *)ext, (const char *)super_ext, 
			     (int)size);

                ext->header.next_extension = VCLASS(wc, extension);
                VCLASS(wc, extension) = (XtPointer)ext;

		/* if this is a copy zero out class functions to 
	         * stop multiple calls .... due to chaining.
		 */

		ext->class_initialize = (OlDnDVCXClassInitializeProc)NULL;
		ext->class_part_initialize = 
					(OlDnDVCXClassPartInitializeProc)NULL;
		ext->get_values = (OlDnDVCXGetValuesProc)NULL;
		ext->set_values = (OlDnDVCXSetValuesFunc)NULL;
		ext->destroy = (OlDnDVCXDestroyProc)NULL;

		ext->resources = (XtResourceList)NULL;
		ext->num_resources = (Cardinal)NULL;
	}

	ext->instance_part_list = (OlDnDVendorPartExtension)NULL;

	if (super_null) {
		super_ext_size = super_num_res = (Cardinal)0;
		super_res = (XrmResourceList)NULL;
	} else {
		super_ext_size = super_ext->instance_part_size;
		super_num_res = super_ext->num_resources;
		super_res = (XrmResourceList)super_ext->resources;
	}

	_OlResourceDependencies(&ext->resources, &ext->num_resources,
				super_res, super_num_res, super_ext_size);

	if (super_null || copied_super)
		return;

#ifndef	__STDC__
#define	INHERITPROC(ext, super, field, PROCNAME)	\
	if (ext->field == XtInherit/**/PROCNAME)	\
		ext->field = super->field
#else
#define	INHERITPROC(ext, super, field, PROCNAME)	\
	if (ext->field == XtInherit##PROCNAME)		\
		ext->field = super->field
#endif

	/* note that no spaces must appear in the actual param PROCNAME
	   when invoking this macro otherwise the correct concatentation
	   will not take place
         */

	INHERITPROC(ext, super_ext, 
		    post_realize_setup,OlDnDVCXPostRealizeSetupProc);
	INHERITPROC(ext, super_ext,
		    register_drop_site,OlDnDVCXRegisterDSFunc);
	INHERITPROC(ext, super_ext,
		    update_drop_site_geometry,OlDnDVCXUpdateDSGeometryProc);
	INHERITPROC(ext, super_ext, delete_drop_site,OlDnDVCXDeleteDSProc);
	INHERITPROC(ext, super_ext,
		    query_drop_site_info,OlDnDVCXQueryDSInfoFunc);
	INHERITPROC(ext, super_ext,
		    assert_drop_site_registry,OlDnDVCXAssertRegistryProc);
	INHERITPROC(ext, super_ext,
		    delete_drop_site_registry,OlDnDVCXDeleteRegistryProc);
	INHERITPROC(ext, super_ext,
		    fetch_dsdm_info,OlDnDVCXFetchDSDMInfoFunc);
	INHERITPROC(ext, super_ext,
		    trigger_message_dispatcher,OlDnDVCXTMDispatcherFunc);
	INHERITPROC(ext, super_ext,
		    preview_message_dispatcher,OlDnDVCXPMDispatcherFunc);
	INHERITPROC(ext, super_ext,
		    deliver_trigger_message,OlDnDVCXDeliverTMFunc);
	INHERITPROC(ext, super_ext,
		    deliver_preview_message,OlDnDVCXDeliverPMFunc);
	INHERITPROC(ext, super_ext,
		    initialize_drag_state,OlDnDVCXInitializeDragStateFunc);
	INHERITPROC(ext, super_ext,
		    clear_drag_state,OlDnDVCXClearDragStateProc);
	INHERITPROC(ext, super_ext,
		    alloc_transient_atom,OlDnDVCXAllocTransientAtomFunc);
	INHERITPROC(ext, super_ext,
		    free_transient_atom,OlDnDVCXFreeTransientAtomProc);
	INHERITPROC(ext, super_ext,
		    associate_selection_and_w,OlDnDVCXAssocSelectionFunc);
	INHERITPROC(ext, super_ext, disassociate_selection_and_w
		    ,OlDnDVCXDisassocSelectionProc);
	INHERITPROC(ext, super_ext, get_w_current_selections
		    ,OlDnDVCXGetCurrentSelectionsFunc);
	INHERITPROC(ext, super_ext,
		    get_w_for_selection,OlDnDVCXGetSelectionFunc);
	INHERITPROC(ext, super_ext,
		    change_site_hints,OlDnDVCXChangeSitePreviewHintsFunc);
	INHERITPROC(ext, super_ext,
		    set_ds_on_interest,OlDnDVCXSetDSOnInterestFunc);
	INHERITPROC(ext, super_ext,
		    set_interest_in_widget_hier,OlDnDVCXSetInterestWidgetHierFunc);
	INHERITPROC(ext, super_ext,
		    clip_drop_sites,OlDnDVCXClipDropSitesProc);
#undef	INHERITPROC
}

/******************************************************************************
 *
 * DnDVCXSetValues
 *
 ******************************************************************************/


static void
_SetDisableClipping (
		     OlDnDVendorPartExtension new_part,
		     OlDnDVendorPartExtension old_part,
		     Widget widget, Boolean new_value)
{
	Boolean	existing_value = ((new_part == old_part)
					? new_part->disable_ds_clipping
					: old_part->disable_ds_clipping);
		
	if (new_value == existing_value) {
		if (new_value)
			new_part->set_disable_sema++;
		else
			new_part->set_disable_sema--;
	} else	if (!existing_value &&
		     new_part->set_disable_sema == 0) {
				new_part->set_disable_sema = 1;
				new_part->disable_ds_clipping = True;
				new_part->configuring_widget  = widget;
		} else if (new_part->set_disable_sema == 1) {
				new_part->set_disable_sema = 0;
				new_part->disable_ds_clipping = False;
				new_part->configuring_widget  = widget;
			} else {
				new_part->disable_ds_clipping = existing_value;
				new_part->set_disable_sema--;
			}

	/* note: new_part->configuring_widget is cleared by
	 *	 _DoPendingConfigures()
	 */
}

static	Boolean
DnDVCXSetValues (Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args, OlDnDVendorPartExtension cur_part, OlDnDVendorPartExtension req_part, OlDnDVendorPartExtension new_part)
{
	register Cardinal	i;

#define	Changed(field)	(new_part->field != cur_part->field)
#define	ReadOnly(field)	 new_part->field =  cur_part->field

	/* enforce read-only resources */

	ReadOnly(root_x);
	ReadOnly(root_y);
	ReadOnly(dirty);

	ReadOnly(dsdm_present);
	ReadOnly(doing_drag);

	ReadOnly(number_of_sites);
	ReadOnly(registry_update_timestamp);
	ReadOnly(dsdm_last_loaded);

	if (Changed(default_drop_site)) {
		OlDnDDropSitePtr			odsp, ndsp, p;
		OlDnDVCXChangeSitePreviewHintsFunc	cspftn;
		Boolean					save;

		cspftn = cur_part->class_extension->change_site_hints;

		if (cspftn == (OlDnDVCXChangeSitePreviewHintsFunc)NULL) {
			new_part->default_drop_site =
				cur_part->default_drop_site;
			/* cant effect the change */
			goto foo;
		}

		odsp = ndsp = (OlDnDDropSitePtr)NULL;

		for (p = new_part->drop_site_list;
		     p != (OlDnDDropSitePtr)NULL;
		     p = OlDnDDropSitePtrNextSite(p)) {
			if (p == cur_part->default_drop_site)
				odsp = p;
			if (p == new_part->default_drop_site)
				ndsp = p;

		}

		save = new_part->auto_assert_dropsite_registry;
		new_part->auto_assert_dropsite_registry = False;

		if (odsp != (OlDnDDropSitePtr)NULL &&
		    ndsp == new_part->default_drop_site) {
			OlDnDSitePreviewHints 	hints;

			hints = OlDnDDropSitePtrPreviewHints(odsp) &
				~OlDnDSitePreviewDefaultSite;
			(*cspftn)(new, new_part, (OlDnDDropSiteID)odsp,
				  hints);
		}

		if (ndsp != (OlDnDDropSitePtr)NULL) {
			OlDnDSitePreviewHints 	hints;

			hints = OlDnDDropSitePtrPreviewHints(ndsp) |
				OlDnDSitePreviewDefaultSite;
			(*cspftn)(new, new_part, (OlDnDDropSiteID)ndsp,
				  hints);
		}

		new_part->auto_assert_dropsite_registry = save;

		foo:;
	}

	for (i = 0; i < *num_args; i++ ) {
		if (strcmp(args[i].name, XtNdisableDSClipping) == 0) {
			_SetDisableClipping(new_part, cur_part,
					    new_part->configuring_widget,
					    new_part->disable_ds_clipping);
			break;
		}
	}

	if (!new_part->disable_ds_clipping && cur_part->disable_ds_clipping &&
	     new_part->num_pending) {
		Boolean	save = new_part->auto_assert_dropsite_registry;

		_DoPendingConfigures(new, new_part);

		new_part->auto_assert_dropsite_registry = save;
		new_part->dirty = True;
	}
	if (new_part->auto_assert_dropsite_registry &&
	    new_part->dirty)
		(*cur_part->class_extension->assert_drop_site_registry)
			(new, new_part);
		

	return (False);
}

/******************************************************************************
 *
 * DnDVCXGetValues
 *
 ******************************************************************************/

static	void
DnDVCXGetValues (Widget current, ArgList args, Cardinal *num_args, OlDnDVendorPartExtension cur_part)
{
	Boolean		fetch = False;
	Position	*x = (Position*)NULL, *y = (Position *)NULL;
	unsigned int	i;

	for (i = 0; i < *num_args; i++) {
		if (!strcmp(args[i].name,XtNrootX)) {
			x = (Position *)args[i].value;
			fetch = True;
			continue;
		}
		if (!strcmp(args[i].name,XtNrootY)) {
			y = (Position *)args[i].value;
			fetch = True;
			continue;
		}

		if (!strcmp(args[i].name, XtNdsdmPresent)) {
			Boolean *ret = (Boolean *)args[i].value;

			if (cur_part->class_extension->fetch_dsdm_info !=
			    (OlDnDVCXFetchDSDMInfoFunc)NULL)
				(*cur_part->class_extension->fetch_dsdm_info)
					(current, cur_part, 
					 XtLastTimestampProcessed(
						XtDisplay(current)));

			*ret = cur_part->dsdm_present;
		}
	}

	if (fetch) {
		DnDFetchRootCoords(current, cur_part);
		if (x) *x = cur_part->root_x;
		if (y) *y = cur_part->root_y;
	}
}

/******************************************************************************
 *
 * DnDVCXInitialize
 *
 ******************************************************************************/

static	void
DnDVCXInitialize (Widget request, Widget new, ArgList args, Cardinal *num_args, OlDnDVendorPartExtension req_part, OlDnDVendorPartExtension new_part)
{
	new_part->drop_site_list = (OlDnDDropSitePtr)NULL;

	new_part->dsdm_rects = (InternalDSDMSRPtr*)NULL;
	new_part->dropsite_rects = (InternalDSRPtr)NULL;

	new_part->current_dsr = (InternalDSRPtr)NULL;
	new_part->current_dsp = (OlDnDDropSitePtr)NULL;
	new_part->current_dsdmsr = (InternalDSDMSRPtr)NULL;

	new_part->dsdm_sr_list = (DSDMSiteRectPtr)NULL;
	new_part->internal_dsdm_sr_list = (InternalDSDMSRPtr)NULL;
	new_part->num_dsdmsrs = 0;

	new_part->transient_atoms = (TransientAtomListPtr)NULL;
	new_part->selection_atoms = (DSSelectionAtomPtr)NULL;

	new_part->drop_site_groups = (DropSiteGroupPtr)NULL;
	new_part->num_groups       = (unsigned int)0;

	new_part->pending_configures = (Widget *)NULL;
	new_part->num_pending        = 0;
	new_part->set_disable_sema   = 0;

	new_part->doing_clipping_already = -1;

	XtAddEventHandler(new, VCXEventHandlerMask, True,
			  DnDVCXEventHandler, (XtPointer) new_part);
}

/******************************************************************************
 *
 * DnDVCXPostRealizeSetup
 *
 ******************************************************************************/

static void
DnDVCXPostRealizeSetup (Widget vendor, OlDnDVendorClassExtension dnd_class, OlDnDVendorPartExtension dnd_part)
{
	FetchPartExtensionIfNull(vendor, dnd_part);
	FetchClassExtensionIfNull(vendor, dnd_class);

	if (dnd_part == (OlDnDVendorPartExtension)NULL || 
	    dnd_class == (OlDnDVendorClassExtension)NULL) /* null pointer neurosis */
		return;


	if (dnd_part->dirty && dnd_part->auto_assert_dropsite_registry &&
	    dnd_class->assert_drop_site_registry != (OlDnDVCXAssertRegistryProc)NULL) {
		(*dnd_class->assert_drop_site_registry)(vendor, dnd_part);
	}
}

/******************************************************************************
 *
 * DnDVCXDestroy
 *
 ******************************************************************************/

static void
DnDVCXDestroy (Widget vendor, OlDnDVendorPartExtension dnd_part)
{
	OlDnDDropSitePtr dsp; 

	FetchPartExtensionIfNull(vendor, dnd_part);

	XtRemoveEventHandler(vendor, VCXEventHandlerMask, True,
			     DnDVCXEventHandler, (XtPointer)dnd_part);

	dsp = dnd_part->drop_site_list;
	dnd_part->drop_site_list = (OlDnDDropSitePtr)NULL;
	dnd_part->number_of_sites = 0;

	for (; dsp != (OlDnDDropSitePtr)NULL;
	       dsp = OlDnDDropSitePtrNextSite(dsp)) {
		DeleteSiteRectsInDropSiteList(dnd_part,
					OlDnDDropSitePtrTopLevelRects(dsp),
					OlDnDDropSitePtrNumRects(dsp));
		XtFree((char *)OlDnDDropSitePtrTopLevelRects(dsp));
		XtFree((char *)OlDnDDropSitePtrRects(dsp));
		XtFree((char *)dsp);
	}

	DeleteDSDMSRList(dnd_part);

	if (dnd_part->drop_site_groups)
		XtFree((char *)dnd_part->drop_site_groups);
	if (dnd_part->internal_dsdm_sr_list)
		XtFree((char *)dnd_part->internal_dsdm_sr_list);
	if (dnd_part->dsdm_sr_list)
		XtFree((char *)dnd_part->dsdm_sr_list);
	if (dnd_part->transient_atoms)
		XtFree((char *)dnd_part->transient_atoms);
	if (dnd_part->pending_configures != (Widget *)NULL)
		XtFree((char *)dnd_part->pending_configures);

}

/****************************************************************************
 *
 * DnDVCXTriggerMessageDispatcher
 *
 ****************************************************************************/



static void
_DnDVCXTriggerACKCB (Widget widget, XtPointer client_data, Atom *selection, Atom *type, XtPointer value, long unsigned int *length, int *format)
{
	OlDnDDropSitePtr		dsp = (OlDnDDropSitePtr)client_data;

	if ((OlDnDDropSitePtrGotAck(dsp) = (*type == XT_CONVERT_FAIL))) {
		OlDnDDropSitePtrState(dsp) = DropSiteError;
	} else  OlDnDDropSitePtrState(dsp) = DropSiteTx;
}

static Boolean
DnDVCXTriggerMessageDispatcher (Widget vendor, OlDnDVendorPartExtension dnd_part, TriggerMessagePtr trigger_message)
{
	Position		topx, topy;
	InternalDSRPtr		idsr;
	OlDnDDropSitePtr	dsp;
	OlDnDTriggerOperation	operation;
	Display			*dpy     = XtDisplay(vendor);
	unsigned int		tryagain = 2; /* twice, 2nd time update root
					       * (x,y) first ...
					       */

	if (!vendor->core.visible) return (False);

	FetchPartExtensionIfNull(vendor, dnd_part);

#if	!defined(USE_GEOMETRY_LOOKUP_ONLY)
	dsp = (OlDnDDropSitePtr)TriggerMessagePtrSiteID(trigger_message);
	{
		OlDnDDropSitePtr	p;


		for (p = dnd_part->drop_site_list;
		     p != (OlDnDDropSitePtr)NULL && p != dsp;
		     p = OlDnDDropSitePtrNextSite(p))
		;

#if	defined(USE_SITE_ID_LOOKUP_ONLY)
		if (p == (OlDnDDropSitePtr)NULL)
			return (False);
#else
		if (p == dsp && dsp != (OlDnDDropSitePtr)NULL)
			goto	gotsite;
#endif
	}
#endif
#if	defined(USE_GEOMETRY_LOOKUP_ONLY) || !defined(USE_SITE_ID_LOOKUP_ONLY)
	while (tryagain--) {
		topx = TriggerMessagePtrX(trigger_message) - dnd_part->root_x;
		topy = TriggerMessagePtrY(trigger_message) - dnd_part->root_y;

		if ((idsr = LookupXYInDropSiteList(dnd_part, topx, topy)) != 
		    (InternalDSRPtr)NULL) {
			break;
		}
		
		if (tryagain) DnDFetchRootCoords(vendor, dnd_part);
		/* try again with new root (x,y) */
	}

	if (idsr == (InternalDSRPtr)NULL) {
		if (dnd_part->default_drop_site == (OlDnDDropSitePtr)NULL ||
		    !dnd_part->dsdm_present)
			return (False);
		else {	/* handle forwarding to default sites */
			InternalDSDMSRPtr	idsdmsr = 
						       (InternalDSDMSRPtr)NULL;

			idsdmsr = LookupXYInDSDMSRList(dnd_part,
				       XtScreen(vendor),
				       TriggerMessagePtrX(trigger_message),
				       TriggerMessagePtrY(trigger_message));


			if (idsdmsr == (InternalDSDMSRPtr)NULL ||
			    !InternalDSDMSRPtrRectIsDefaultSite(idsdmsr))
				return (False);

			/* we have a match - its a default site .... */

			if (XtWindow(vendor) ==
			    InternalDSDMSRPtrRectWindowID(idsdmsr))
				dsp = dnd_part->default_drop_site;
			else
				return (False); /* how did we get here */
		}
	} else
		dsp = (OlDnDDropSitePtr)InternalDSRPtrDropSite(idsr);
#endif

#ifndef	USE_GEOMETRY_LOOKUP_ONLY
gotsite:;
#endif

	dnd_part->current_dsr = (InternalDSRPtr)NULL; /* zero cache */
	dnd_part->current_dsp = (OlDnDDropSitePtr)NULL;


	if (OlDnDDropSitePtrTriggerNotify(dsp) ==
	    (OlDnDTriggerMessageNotifyProc)NULL) {
		return (False);
	}

	if (TriggerMessagePtrFlags(trigger_message) & OlDnDTriggerAck) {
		XtGetSelectionValue(OlDnDDropSitePtrOwner(dsp),
				 TriggerMessagePtrSelection(trigger_message),
				 OlInternAtom(XtDisplay(vendor),
					      _SUN_DRAGDROP_ACK_NAME),
				 _DnDVCXTriggerACKCB,
				 (XtPointer)dsp,
				 TriggerMessagePtrTimestamp(trigger_message));
		OlDnDDropSitePtrState(dsp) = DropSiteAck;
	} else OlDnDDropSitePtrState(dsp) = DropSiteTrigger;

	OlDnDDropSitePtrIncomingTransient(dsp) =
		(Boolean)((TriggerMessagePtrFlags(trigger_message) & 
		 OlDnDTriggerTransient) ? True : False);

	/* call the notifier */

	OlDnDDropSitePtrTimestamp(dsp) =
		TriggerMessagePtrTimestamp(trigger_message);

	operation = (TriggerMessagePtrFlags(trigger_message) &
		     OlDnDTriggerMove) ? OlDnDTriggerMoveOp
				       : OlDnDTriggerCopyOp;

	/* hack - hack - hack - hack */

	/* 
	 *
	 * I am grossly violating the Intrinsics here however making
	 * sure that XtLastTimestampProcessed() will return the
	 * timestamp of the trigger notify event will ensure that
	 * calls to XtGetSelectionValue(s) using XtLastTimestampProcessed()
	 * will work ....
	 * 
	 * I could have cheated and dispatched a synthetic  PropertyNotify 
	 * event on the vendor here but its an awful lot of code to 
	 * traverse to achieve an assignment ..... 
	 *
	 * mea culpa!
	 *
	 */

	if (OlDnDDropSitePtrTimestamp(dsp) >
	    XtLastTimestampProcessed(dpy)) {
		_XtGetPerDisplay(dpy)->last_timestamp = 
			OlDnDDropSitePtrTimestamp(dsp);
	}

	(*OlDnDDropSitePtrTriggerNotify(dsp))
			(OlDnDDropSitePtrOwner(dsp),
			 OlDnDDropSitePtrWindow(dsp),
			 TriggerMessagePtrX(trigger_message),
			 TriggerMessagePtrY(trigger_message),
			 TriggerMessagePtrSelection(trigger_message),
			 TriggerMessagePtrTimestamp(trigger_message),
			 (OlDnDDropSiteID)dsp,
			 operation,
			 (Boolean)OlDnDDropSitePtrIncomingTransient(dsp),
			 (Boolean)((TriggerMessagePtrFlags(trigger_message) &
				    OlDnDTriggerForwarded) ==
					    OlDnDTriggerForwarded),
			 OlDnDDropSitePtrClosure(dsp));

	return ((Boolean)True);
}

/******************************************************************************
 *
 * DnDVCXPreviewMessageDispatcher
 *
 ******************************************************************************/

static Boolean
DnDVCXPreviewMessageDispatcher (Widget vendor, OlDnDVendorPartExtension dnd_part, PreviewMessagePtr preview_message)
{
	Position		topx, topy;
	InternalDSRPtr		idsr = (InternalDSRPtr)NULL, prev_cache;
	InternalDSDMSRPtr	idsdmsr = (InternalDSDMSRPtr)NULL;
	OlDnDDropSitePtr	dsp, prev_dsp;
	Boolean			forwarded;
	unsigned int		tryagain = 2; /* twice, 2nd time update root
					       * (x,y) first ...
					       */

	FetchPartExtensionIfNull(vendor, dnd_part);

	if (((prev_cache = dnd_part->current_dsr) != (InternalDSRPtr)NULL ||
	    ((prev_dsp = dnd_part->current_dsp) != (OlDnDDropSitePtr)NULL)) &&
	    PreviewMessagePtrEventcode(preview_message) == LeaveNotify) {
		
		if (prev_cache)
			dsp = (OlDnDDropSitePtr)
				InternalDSRPtrDropSite(prev_cache);
		else
			dsp = prev_dsp;

		if (OlDnDDropSitePtrWantsPreviewEnterLeave(dsp)) {
			forwarded = ((PreviewMessagePtrFlags(preview_message)
				     & OlDnDPreviewForwarded) ==
					OlDnDPreviewForwarded);

			(*OlDnDDropSitePtrPreviewNotify(dsp))
					(OlDnDDropSitePtrOwner(dsp),
					 OlDnDDropSitePtrWindow(dsp),
					 PreviewMessagePtrX(preview_message),
					 PreviewMessagePtrY(preview_message),
					 PreviewMessagePtrEventcode(
							preview_message),
					 PreviewMessagePtrTimestamp(
							preview_message),
					 (OlDnDDropSiteID)dsp,
					 forwarded,
					 OlDnDDropSitePtrClosure(dsp));
		}

		dnd_part->current_dsr = (InternalDSRPtr)NULL;
		dnd_part->current_dsp = (OlDnDDropSitePtr)NULL;

		return (True);
	}

#if	!defined(USE_GEOMETRY_LOOKUP_ONLY)
	dsp = (OlDnDDropSitePtr)PreviewMessagePtrSiteID(preview_message);
	{
		OlDnDDropSitePtr	p;

		for (p = dnd_part->drop_site_list;
		     p != (OlDnDDropSitePtr)NULL && p != dsp;
		     p = OlDnDDropSitePtrNextSite(p))
		;

#if	defined(USE_SITE_ID_LOOKUP_ONLY)
		if (p == (OlDnDDropSitePtr)NULL)
			return (False);
#else
		if (p == dsp && dsp != (OlDnDDropSitePtr)NULL)
			goto	gotsite;
#endif
	}
#endif
#if	defined(USE_GEOMETRY_LOOKUP_ONLY) || !defined(USE_SITE_ID_LOOKUP_ONLY)
	while (tryagain--) { 
		topx = PreviewMessagePtrX(preview_message) - dnd_part->root_x;
		topy = PreviewMessagePtrY(preview_message) - dnd_part->root_y;

		if ((idsr = LookupXYInDropSiteList(dnd_part, topx, topy)) != 
		    (InternalDSRPtr)NULL) {
			break;
		}
		
		if (tryagain) DnDFetchRootCoords(vendor, dnd_part);
		/* try again with new root (x,y) */
	}

	if (idsr == (InternalDSRPtr)NULL) {
		if (dnd_part->default_drop_site == (OlDnDDropSitePtr)NULL ||
		    !dnd_part->dsdm_present)
			return (False);
		else {	/* handle forwarding to default sites */

			idsdmsr = LookupXYInDSDMSRList(dnd_part,
				       XtScreen(vendor),
				       PreviewMessagePtrX(preview_message),
				       PreviewMessagePtrY(preview_message));


			if (idsdmsr == (InternalDSDMSRPtr)NULL ||
			    !InternalDSDMSRPtrRectIsDefaultSite(idsdmsr))
				return (False);

			/* we have a match - its a default site .... */

			if (XtWindow(vendor) ==
			    InternalDSDMSRPtrRectWindowID(idsdmsr))
				dsp = dnd_part->default_drop_site;
			else
				return (False); /* how did we get here ? */
		}
	} else
		dsp = (OlDnDDropSitePtr)InternalDSRPtrDropSite(idsr);
#endif

#ifndef	USE_GEOMETRY_LOOKUP_ONLY
gotsite:;
#endif

	if (OlDnDDropSitePtrPreviewNotify(dsp) ==
	    (OlDnDPreviewMessageNotifyProc)NULL ||
	    !OlDnDDropSitePtrIsSensitive(dsp)) {
		return (False);
	}

	if (idsr == (InternalDSRPtr)NULL) {
		InternalDSRPtr	rects = OlDnDDropSitePtrTopLevelRects(dsp);
		unsigned int	num   = OlDnDDropSitePtrNumRects(dsp);

		dnd_part->current_dsp = dsp;
		dnd_part->current_dsr = idsr; /* update cache */

		tryagain = 2;

		while (tryagain--) {
			register InternalDSRPtr  r;

			topx = PreviewMessagePtrX(preview_message) -
				dnd_part->root_x;
			topy = PreviewMessagePtrY(preview_message) -
				dnd_part->root_y;

			for (r = rects; r < rects + num; r++) {
				if (XYInInternalDSR(r, topx, topy))
					goto preview;
			}

			if (tryagain) DnDFetchRootCoords(vendor, dnd_part);
		} 

		return (False);
	}

preview:;
	if (PreviewMessagePtrEventcode(preview_message) == EnterNotify) {
		dnd_part->current_dsr = idsr; /* update cache */
		dnd_part->current_dsp = dsp;
	}

	if (PreviewMessagePtrEventcode(preview_message) == LeaveNotify) 
		OlDnDDropSitePtrState(dsp) = DropSiteState0;

	OlDnDDropSitePtrState(dsp) = DropSitePreviewing;
	OlDnDDropSitePtrTimestamp(dsp) =
		PreviewMessagePtrTimestamp(preview_message);

	forwarded = ((PreviewMessagePtrFlags(preview_message)
		      & OlDnDPreviewForwarded) == OlDnDPreviewForwarded);

	/* call the notifier */

	if ((OlDnDDropSitePtrWantsPreviewMotion(dsp) &&
	     PreviewMessagePtrEventcode(preview_message) == MotionNotify) ||
	    (OlDnDDropSitePtrWantsPreviewEnterLeave(dsp) &&
	     (PreviewMessagePtrEventcode(preview_message) == EnterNotify)))
		(*OlDnDDropSitePtrPreviewNotify(dsp))
				(OlDnDDropSitePtrOwner(dsp),
				 OlDnDDropSitePtrWindow(dsp),
				 PreviewMessagePtrX(preview_message),
				 PreviewMessagePtrY(preview_message),
				 PreviewMessagePtrEventcode(preview_message),
				 PreviewMessagePtrTimestamp(preview_message),
				 (OlDnDDropSiteID)dsp,
				 forwarded,
				 OlDnDDropSitePtrClosure(dsp));

	return (True);
}

/******************************************************************************
 *
 * DnDVCXRegisterDropSite
 *
 * register a drop site.
 *
 ******************************************************************************/

static void _calldsdestroy(Widget w, OlDnDDropSitePtr dsp)
{
	Widget			 vendor  = _OlGetShellOfWidget(w);
	OlDnDVendorPartExtension dnd_part= _OlGetDnDVendorPartExtension(vendor);


	if (dnd_part == (OlDnDVendorPartExtension)NULL)
		return;

	if (dnd_part->class_extension->delete_drop_site !=
	    (OlDnDVCXDeleteDSProc)NULL)
		(*dnd_part->class_extension->delete_drop_site)
			(vendor, dnd_part, (OlDnDDropSiteID)dsp);
}

static void
unregisterondestroycb(Widget w, XtPointer clientd, XtPointer calld)
{
	OlDnDDropSitePtr 	 dsp = (OlDnDDropSitePtr)(clientd);

	_calldsdestroy(w, dsp);
}

#define	WindowDSOwnerEVM	(StructureNotifyMask)

static void
_windowevh(Widget w, XtPointer clientd, XEvent *event, Boolean *continue_to_dispatch)
{
	OlDnDDropSitePtr 	 dsp = (OlDnDDropSitePtr)(clientd);

	if (OlDnDDropSitePtrWindow(dsp) != event->xany.window)
		return;

	switch (event->xany.type) {
		case	DestroyNotify:
			_XtUnregisterWindow(event->xany.window, w);
			_calldsdestroy(w, dsp);
	}
}

static OlDnDDropSiteID
DnDVCXRegisterDropSite (
			Widget vendor,
			OlDnDVendorPartExtension dnd_part,
			Widget widget, Window window,
			OlDnDSitePreviewHints preview_hints,
			OlDnDSiteRectPtr site_rects,
			unsigned int num_sites,
			OlDnDTriggerMessageNotifyProc tmnotify,
			OlDnDPreviewMessageNotifyProc pmnotify,
			Boolean on_interest, XtPointer closure)
{
	OlDnDDropSitePtr		dsp;
	register InternalDSRPtr		isr;
	register OlDnDSiteRectPtr	sr = site_rects,
					dssr;
	Boolean	 			is_window_ds;
	OlDnDVCXClipDropSitesProc	proc;

	FetchPartExtensionIfNull(vendor, dnd_part);
	
	if (widget == (Widget)NULL) {
		OlWarning(dgettext(OlMsgsDomain, "DnDVCXRegisterDropSite: Widget NULL"));
		return ((OlDnDDropSiteID)NULL);
	}

	is_window_ds = (XtWindowOfObject(widget) != window);

	if (!XtIsRealized(widget)) {
		OlWarning(dgettext(OlMsgsDomain, "DnDVCXRegisterDropSite: Widget not Realized"));
		return ((OlDnDDropSiteID)NULL);
	}

	dsp  = (OlDnDDropSitePtr)XtCalloc(1, sizeof(OlDnDDropSite));

	isr  = (InternalDSRPtr) XtCalloc(num_sites, sizeof(InternalDSR));
	dssr = (OlDnDSiteRectPtr)XtCalloc(num_sites, sizeof(OlDnDSiteRect));

	OlDnDDropSitePtrTopLevelRects(dsp) = isr;
	OlDnDDropSitePtrRects(dsp)         = dssr;

	for (; sr < site_rects + num_sites; isr++, sr++, dssr++) {
		*dssr = *sr;

		InternalDSRPtrNext(isr) = (InternalDSRPtr)NULL;
		InternalDSRPtrPrev(isr) = (InternalDSRPtr)NULL;
		InternalDSRPtrDropSite(isr) = (OlDnDDropSiteID)dsp;

		InternalDSRPtrRectVisible(isr) = 
				ClippedSiteVisibilityIndeterminate; 

	}

	preview_hints &= (OlDnDSitePreviewNone	      | /* redundant */
			  OlDnDSitePreviewEnterLeave  |
			  OlDnDSitePreviewMotion      |
			  OlDnDSitePreviewBoth	      | /* redundant */
			  OlDnDSitePreviewDefaultSite |
			  OlDnDSitePreviewForwarded   |
			  OlDnDSitePreviewInsensitive);


	if (preview_hints & OlDnDSitePreviewDefaultSite) {
		dnd_part->default_drop_site = dsp;
	}

	OlDnDDropSitePtrOwner(dsp) 	       = widget;
	OlDnDDropSitePtrWindow(dsp)	       = window;
	OlDnDDropSitePtrEventWindow(dsp)       = XtWindow(vendor);
	OlDnDDropSitePtrPreviewHints(dsp)      = preview_hints;
	OlDnDDropSitePtrNumRects(dsp)	       = num_sites;
	OlDnDDropSitePtrTimestamp(dsp)         = XtLastTimestampProcessed(
							XtDisplay(vendor));
	OlDnDDropSitePtrState(dsp)             = DropSiteState0;
	OlDnDDropSitePtrTriggerNotify(dsp)     = tmnotify;
	OlDnDDropSitePtrPreviewNotify(dsp)     = pmnotify;
	OlDnDDropSitePtrClosure(dsp) 	       = closure;
	OlDnDDropSitePtrOnInterest(dsp)        = on_interest;
	OlDnDDropSitePtrGotAck(dsp)            = False;
	OlDnDDropSitePtrIncomingTransient(dsp) = False;

	OlDnDDropSitePtrOwnerDepth(dsp) = _CalculateWidgetDepth(widget);

	OlDnDDropSitePtrVisibility(dsp) = ClippedSiteVisibilityIndeterminate;

	OlDnDDropSitePtrWEVM(dsp)       = NoEventMask;

	dnd_part->number_of_sites++;
	OlDnDDropSitePtrNextSite(dsp) = dnd_part->drop_site_list;
	dnd_part->drop_site_list = dsp;


	_InsertDSInGroup(vendor, dnd_part, dsp);

	proc = dnd_part->class_extension->clip_drop_sites;

	if (proc != (OlDnDVCXClipDropSitesProc)NULL)
		(*proc)(vendor, dnd_part, OlDnDDropSitePtrOwner(dsp));
	else {
		sr  = site_rects;
		isr = OlDnDDropSitePtrTopLevelRects(dsp);

		for (; sr < site_rects + num_sites; isr++, sr++) {
			MapLocalCoordsToTopLevel(
				vendor, dnd_part, 
				OlDnDDropSitePtrOwner(dsp),
				OlDnDDropSitePtrWindow(dsp), sr,
				&InternalDSRPtrTopLevelRect(isr)
				);
		}
	}

	if (widget != vendor && !is_window_ds) {
		XtAddCallback(widget, XtNdestroyCallback, unregisterondestroycb,
			      (XtPointer)dsp);

		if (XtHasCallbacks(widget, XtNunrealizeCallback) 
		    != XtCallbackNoList) {
			XtAddCallback(widget, XtNunrealizeCallback,
				      unregisterondestroycb, (XtPointer)dsp);
		}
	} else if (is_window_ds) {
		XWindowAttributes	wattrs;

		_XtRegisterWindow(window, widget);

		XGetWindowAttributes(XtDisplay(widget), window, &wattrs);

		OlDnDDropSitePtrWEVM(dsp) = wattrs.your_event_mask;

		wattrs.your_event_mask |= WindowDSOwnerEVM;

		XSelectInput(XtDisplayOfObject(widget), window,
			     wattrs.your_event_mask);

		XtInsertRawEventHandler(widget, WindowDSOwnerEVM, True,
				        _windowevh, dsp, XtListHead);
	}

	InsertSiteRectsInDropSiteList(dnd_part,
				      OlDnDDropSitePtrTopLevelRects(dsp),
				      num_sites); 

	if (on_interest) {
		dnd_part->dirty = True;

		if (dnd_part->auto_assert_dropsite_registry &&
		    dnd_part->class_extension->assert_drop_site_registry !=
		    (OlDnDVCXAssertRegistryProc)NULL) {
			(*dnd_part->class_extension->assert_drop_site_registry)
				(vendor, dnd_part);

			if (dnd_part->class_extension->fetch_dsdm_info !=
			    (OlDnDVCXFetchDSDMInfoFunc)NULL) {
				(*dnd_part->class_extension->fetch_dsdm_info)
						(vendor, dnd_part, 
						 XtLastTimestampProcessed(
						 XtDisplay(vendor)));
			}
		}
	}
	return ((OlDnDDropSiteID)dsp);
}

/******************************************************************************
 *
 * DnDVCXUpdateDropSiteGeometry
 *
 * update the geometry of a drop site.
 *
 ******************************************************************************/

static void
InvalidateCurrentDSR (OlDnDVendorPartExtension dnd_part, InternalDSRPtr isr, unsigned int num_isrs)
{
	register InternalDSRPtr	srp = isr;

	if (dnd_part->current_dsr != (InternalDSRPtr)NULL) {
		for (; srp != isr + num_isrs; srp = InternalDSRPtrNext(srp))
			if (srp == dnd_part->current_dsr) {
				dnd_part->current_dsr = (InternalDSRPtr)NULL;
				break;
			}
	}
}

static Boolean
DnDVCXUpdateDropSiteGeometry (Widget vendor, OlDnDVendorPartExtension dnd_part, OlDnDDropSiteID drop_site, OlDnDSiteRectPtr site_rects, unsigned int num_rects)
{
	OlDnDDropSitePtr		dsp;
	OlDnDSiteRectPtr		dssr;
	Boolean				ret_val = False;
	OlDnDVCXClipDropSitesProc	proc;

	FetchPartExtensionIfNull(vendor, dnd_part);

        for (dsp = dnd_part->drop_site_list;
             dsp != (OlDnDDropSitePtr)NULL &&
             dsp != (OlDnDDropSitePtr)drop_site;
                dsp = OlDnDDropSitePtrNextSite(dsp))
	;

	if (dsp != (OlDnDDropSitePtr)NULL) {
		InternalDSRPtr		isr;
		OlDnDSiteRectPtr	sr;

		if (!XtIsRealized(OlDnDDropSitePtrOwner(dsp))) {
			OlWarning(dgettext(OlMsgsDomain, "DnDVCXUpdateDropSiteGeometry: Widget not Realized"));
			return (ret_val);
		}

		InvalidateCurrentDSR(dnd_part, 
				     OlDnDDropSitePtrTopLevelRects(dsp),
				     OlDnDDropSitePtrNumRects(dsp)); 

		DeleteSiteRectsInDropSiteList(dnd_part,
				      OlDnDDropSitePtrTopLevelRects(dsp),
				      OlDnDDropSitePtrNumRects(dsp)); 

		if (OlDnDDropSitePtrTopLevelRects(dsp) != (InternalDSRPtr)NULL)
			XtFree((char *)OlDnDDropSitePtrTopLevelRects(dsp));

		if (OlDnDDropSitePtrRects(dsp) != (OlDnDSiteRectPtr)NULL)
			XtFree((char *)OlDnDDropSitePtrRects(dsp));

		OlDnDDropSitePtrNumRects(dsp) = num_rects;

		isr = (InternalDSRPtr) XtCalloc(num_rects, sizeof(InternalDSR));
		OlDnDDropSitePtrTopLevelRects(dsp) = isr;

		dssr = (OlDnDSiteRectPtr) XtCalloc(num_rects,
						  sizeof(OlDnDSiteRect));

		OlDnDDropSitePtrRects(dsp) = dssr;

		for (sr = site_rects;
		     sr < site_rects+num_rects;
		     dssr++, isr++, sr++) {
			*dssr = *sr;

			InternalDSRPtrNext(isr) = (InternalDSRPtr)NULL;
			InternalDSRPtrPrev(isr) = (InternalDSRPtr)NULL;
			InternalDSRPtrDropSite(isr) = (OlDnDDropSiteID)dsp;
		}

		proc = dnd_part->class_extension->clip_drop_sites;

		if (proc != (OlDnDVCXClipDropSitesProc)NULL)
			(*proc)(vendor, dnd_part, OlDnDDropSitePtrOwner(dsp));
		else {
			sr  = site_rects;
			isr = OlDnDDropSitePtrTopLevelRects(dsp);

			for (; sr < site_rects + num_rects;
			       isr++, sr++) {
				MapLocalCoordsToTopLevel(
					vendor, dnd_part, 
					OlDnDDropSitePtrOwner(dsp),
					OlDnDDropSitePtrWindow(dsp), sr,
					&InternalDSRPtrTopLevelRect(isr)
					);
			}
		}

		InsertSiteRectsInDropSiteList(dnd_part,
				      OlDnDDropSitePtrTopLevelRects(dsp),
				      OlDnDDropSitePtrNumRects(dsp)); 

		if (!OlDnDDropSitePtrOnInterest(dsp))
			return (True);

		dnd_part->dirty = True;

		if (dnd_part->auto_assert_dropsite_registry &&
		    dnd_part->class_extension->assert_drop_site_registry !=
		    (OlDnDVCXAssertRegistryProc)NULL) {
			(*dnd_part->class_extension->assert_drop_site_registry)
				(vendor, dnd_part);

			if (dnd_part->class_extension->fetch_dsdm_info !=
			    (OlDnDVCXFetchDSDMInfoFunc)NULL) {
				(*dnd_part->class_extension->fetch_dsdm_info)
						(vendor, dnd_part, 
						 XtLastTimestampProcessed(
							XtDisplay(vendor)));
			}
		}
		ret_val = True;
	}

	return (ret_val);
}

/******************************************************************************
 *
 * DnDVCXDeleteDropSite
 *
 * delete a drop site.
 *
 ******************************************************************************/

static void
DnDVCXDeleteDropSite (Widget vendor, OlDnDVendorPartExtension dnd_part, OlDnDDropSiteID drop_site)
{
	OlDnDDropSitePtr	dsp, *dspp;
	unsigned int		i;
	Boolean			is_window_ds;

	FetchPartExtensionIfNull(vendor, dnd_part);

	for (dspp = &dnd_part->drop_site_list;
	     *dspp != (OlDnDDropSitePtr)NULL &&
	     *dspp != (OlDnDDropSitePtr)drop_site;
		dspp = &OlDnDDropSitePtrNextSite(*dspp))
	;

	if (*dspp == (OlDnDDropSitePtr)NULL)
		return;	/* nae such luck */

	dsp = *dspp;	/* remove it from bondage */
	*dspp = OlDnDDropSitePtrNextSite(dsp);
	OlDnDDropSitePtrNextSite(dsp) = (OlDnDDropSitePtr)NULL;

	dnd_part->dirty = True;

	InvalidateCurrentDSR(dnd_part, 
			     OlDnDDropSitePtrTopLevelRects(dsp),
			     OlDnDDropSitePtrNumRects(dsp)); 

	if (dnd_part->current_dsp == dsp)
		dnd_part->current_dsp = (OlDnDDropSitePtr)NULL;

	DeleteSiteRectsInDropSiteList(dnd_part, 
			      OlDnDDropSitePtrTopLevelRects(dsp),
			      OlDnDDropSitePtrNumRects(dsp)); 

	is_window_ds = XtWindowOfObject(OlDnDDropSitePtrOwner(dsp)) == 
		       OlDnDDropSitePtrWindow(dsp);
		       
	if (OlDnDDropSitePtrOwner(dsp) != vendor && !is_window_ds) {
		XtRemoveCallback(OlDnDDropSitePtrOwner(dsp),
				 XtNdestroyCallback, 
				 unregisterondestroycb, (XtPointer)dsp);

		if (XtHasCallbacks(OlDnDDropSitePtrOwner(dsp),
				   XtNunrealizeCallback) != XtCallbackNoList) {
			XtRemoveCallback(OlDnDDropSitePtrOwner(dsp),
					 XtNunrealizeCallback, 
					 unregisterondestroycb, (XtPointer)dsp);
		}
	} else if (is_window_ds) {
		XWindowAttributes	wattrs;
		unsigned long		evm;

		_XtUnregisterWindow(OlDnDDropSitePtrWindow(dsp),
				    OlDnDDropSitePtrOwner(dsp));

		XGetWindowAttributes(
			XtDisplayOfObject(OlDnDDropSitePtrOwner(dsp)),
			OlDnDDropSitePtrWindow(dsp), 
			&wattrs
		);

 		evm = wattrs.your_event_mask;

		evm &= ~(WindowDSOwnerEVM &
			 ~(WindowDSOwnerEVM &
			   OlDnDDropSitePtrWEVM(dsp))
			);

		XSelectInput(XtDisplayOfObject(OlDnDDropSitePtrOwner(dsp)),
			     OlDnDDropSitePtrWindow(dsp), evm);

		XtRemoveRawEventHandler(OlDnDDropSitePtrOwner(dsp),
				        WindowDSOwnerEVM,
				        True, _windowevh, (XtPointer)dsp);
	}


	if (dnd_part->class_extension->disassociate_selection_and_w !=
	    (OlDnDVCXDisassocSelectionProc)NULL)
		(*dnd_part->class_extension->disassociate_selection_and_w)
			(vendor, dnd_part, OlDnDDropSitePtrOwner(dsp),
			 FreeAllSelectionAtoms, 
			 XtLastTimestampProcessed(XtDisplay(vendor)));

        if (dnd_part->class_extension->free_transient_atom !=
            (OlDnDVCXFreeTransientAtomProc)NULL)
                (*dnd_part->class_extension->free_transient_atom)
                        (vendor, dnd_part, OlDnDDropSitePtrOwner(dsp),
			 FreeAllTransientAtoms);

	_DeleteDSFromGroup(vendor, dnd_part, dsp);

	if (OlDnDDropSitePtrNumRects(dsp)) {
		XtFree((char *)OlDnDDropSitePtrTopLevelRects(dsp));
		XtFree((char *)OlDnDDropSitePtrRects(dsp));
	}
	XtFree((char *)dsp);

	if (dsp == dnd_part->default_drop_site) {
		dnd_part->default_drop_site = (OlDnDDropSitePtr)NULL;
	}

	if (--dnd_part->number_of_sites == 0) {
		if (dnd_part->class_extension->delete_drop_site_registry !=
		    (OlDnDVCXDeleteRegistryProc)NULL)
			(*dnd_part->class_extension->delete_drop_site_registry)
				(vendor, dnd_part);
		dnd_part->number_of_sites = 0;
	} else if (dnd_part->auto_assert_dropsite_registry &&
	    	   dnd_part->class_extension->assert_drop_site_registry !=
	    	   (OlDnDVCXAssertRegistryProc)NULL) {
			(*dnd_part->class_extension->assert_drop_site_registry)
							     (vendor, dnd_part);
	}
}

/******************************************************************************
 *
 * DnDVCXQueryDropSiteInfo
 *
 * get the info for a registered drop site.
 *
 ******************************************************************************/

static	Boolean
DnDVCXQueryDropSiteInfo (Widget vendor, OlDnDVendorPartExtension dnd_part, OlDnDDropSiteID drop_site, Widget *widget, Window *window, OlDnDSitePreviewHints *preview_hints, OlDnDSiteRectPtr *site_rects, unsigned int *num_rects, Boolean *on_interest)
{
	OlDnDDropSitePtr	dsp;

	FetchPartExtensionIfNull(vendor, dnd_part);

        for (dsp = dnd_part->drop_site_list;
             dsp != (OlDnDDropSitePtr)NULL &&
             dsp != (OlDnDDropSitePtr)drop_site;
                dsp = OlDnDDropSitePtrNextSite(dsp))
	;

	if (dsp != (OlDnDDropSitePtr)NULL) {
		if (widget != (Widget *)NULL)
			*widget = OlDnDDropSitePtrOwner(dsp);

		if (window != (Window *)NULL) 
			*window = OlDnDDropSitePtrWindow(dsp);

		if (preview_hints != (OlDnDSitePreviewHints *)NULL)
			*preview_hints = OlDnDDropSitePtrPreviewHints(dsp);

		if (num_rects != (unsigned int *)NULL)
			*num_rects = OlDnDDropSitePtrNumRects(dsp);

		if (on_interest != (Boolean *)NULL)
			*on_interest = OlDnDDropSitePtrOnInterest(dsp);

		if (num_rects != (unsigned int *)NULL &&
		    site_rects != (OlDnDSiteRectPtr *)NULL) {
			unsigned int 		num = *num_rects;
			OlDnDSiteRectPtr	site;
			InternalDSRPtr		isr;

			*site_rects = site = (OlDnDSiteRectPtr)
				XtCalloc(num, sizeof(OlDnDSiteRect));

			for (isr = OlDnDDropSitePtrTopLevelRects(dsp);
			     num--;
			     site++, isr++)
				*site = InternalDSRPtrTopLevelRect(isr);

		} else {
				if (num_rects)
					*num_rects = 0;
				if (site_rects)
					*site_rects = (OlDnDSiteRectPtr)NULL;

				return (False);
		}
		return (True);
	}
	return (False);
}

/******************************************************************************
 *
 * DnDVCXAssertDropSiteRegistry
 *
 * assert the registry of our drop sites ....
 *
 * include format of property later .....
 *
 ******************************************************************************/

static void
DnDVCXAssertDropSiteRegistry (Widget vendor, OlDnDVendorPartExtension dnd_part)
{
	FetchPartExtensionIfNull(vendor, dnd_part);

	if (dnd_part->dirty && dnd_part->auto_assert_dropsite_registry) {
		InterestPropertyPtr	interest;
		SiteDescriptionPtr	sites;
		OlDnDDropSitePtr	dsp;
		Atom			interest_property;
		unsigned int		sizeofinterestprop =
						SizeOfInterestPropertyHead;
		unsigned int		site_count   = 0,
					actual_size  = sizeofinterestprop;

		if (!XtIsRealized(vendor))
			return;

		for (dsp = dnd_part->drop_site_list;
		     dsp != (OlDnDDropSitePtr)NULL;
		     dsp = OlDnDDropSitePtrNextSite(dsp)) {
			if (OlDnDDropSitePtrOnInterest(dsp)) {
				sizeofinterestprop +=
				SizeOfSiteDescriptionForNRects(
					OlDnDDropSitePtrNumRects(dsp));
			}
		}	/* calc max size */
	
		interest = (InterestPropertyPtr)XtCalloc(1, sizeofinterestprop);
		InterestPropertyPtrVersionNumber(interest) = 0;
		
		for ((dsp = dnd_part->drop_site_list), 
		     (sites = InterestPropertyPtrSiteDescriptions(interest));
		     dsp != (OlDnDDropSitePtr)NULL;
		     dsp = OlDnDDropSitePtrNextSite(dsp)) { /* for */
			InternalDSRPtr	 isr;
			SiteRectPtr 	 rects;
			unsigned int	 nr = OlDnDDropSitePtrNumRects(dsp);
			unsigned int	 size;
			unsigned int	 nrects = 0;

			if (!OlDnDDropSitePtrOnInterest(dsp) ||
			    OlDnDDropSitePtrVisibility(dsp) ==
				 ClippedSiteNotVisible) continue;
			
			SiteDescriptionPtrEventWindow(sites) = 
				OlDnDDropSitePtrEventWindow(dsp);
			SiteDescriptionPtrSiteID(sites) = (unsigned int)dsp;
			SiteDescriptionPtrPreviewHints(sites) =
				OlDnDDropSitePtrPreviewHints(dsp);
			SiteDescriptionPtrAreasType(sites) = IsRectList;

			for ((isr = OlDnDDropSitePtrTopLevelRects(dsp)),
			     (rects = RectListRects(
				        SiteDescriptionPtrAreaRectList(sites)));
			     nr--;
			     isr++, rects++) {
				if (!InternalDSRPtrRectVisible(isr)) continue;

				SiteRectPtrX(rects) =
					InternalDSRPtrTopLevelRectX(isr);
				SiteRectPtrY(rects) =
					InternalDSRPtrTopLevelRectY(isr);
				SiteRectPtrWidth(rects) =
					InternalDSRPtrTopLevelRectWidth(isr);
				SiteRectPtrHeight(rects) =
					InternalDSRPtrTopLevelRectHeight(isr);
				nrects++;
			}

			if (nrects > 0) {
				RectListRectCount(SiteDescriptionPtrAreaRectList(sites)) = nrects;
				size = SizeOfSiteDescriptionForNRects(nrects);
				actual_size += size;
				sites = (SiteDescriptionPtr)
						((char *)sites + size);
				site_count++;
			}
		}

		interest_property = OlInternAtom(XtDisplay(vendor),
						 _SUN_DRAGDROP_INTEREST_NAME);

		if (site_count > 0) {
			InterestPropertyPtrSiteCount(interest) = site_count;


			XChangeProperty(XtDisplay(vendor), XtWindow(vendor), 
					interest_property, interest_property,
					32, PropModeReplace,
					(unsigned char *)interest,
					actual_size >> 2);
		} else
			XDeleteProperty(XtDisplay(vendor), XtWindow(vendor),
					interest_property);

		XtFree((char *)interest);
		dnd_part->dirty = False;
	}
}

/******************************************************************************
 *
 * DnDVCXDeleteDropSiteRegistry
 *
 * blow dat suckah away!
 *
 ******************************************************************************/

static void
DnDVCXDeleteDropSiteRegistry (Widget vendor, OlDnDVendorPartExtension dnd_part)
{
	FetchPartExtensionIfNull(vendor, dnd_part);
	
	dnd_part->dirty = False;
	if (XtIsRealized(vendor)) {
		XDeleteProperty(XtDisplay(vendor), XtWindow(vendor), 
				OlInternAtom(XtDisplay(vendor),
					     _SUN_DRAGDROP_INTEREST_NAME));
	}
}

/****************************************************************************
 *
 * DnDVCXFetchDSDMInfo
 *
 * fetch the _SUN_DRAGDROP_SITE_RECTS target from the _SUN_DRAGDROP_DSDM 
 * selection.
 *
 ****************************************************************************/

static void
_DnDVCXDSDMSiteRectSelectionCB (Widget vendor, XtPointer client_data, Atom *selection, Atom *type, XtPointer value, long unsigned int *length, int *format)
{
	unsigned int			i;
	OlDnDVendorPartExtension	dnd_part;

	dnd_part = (OlDnDVendorPartExtension)client_data;

	if (!dnd_part->pending_dsdm_info) {
		return;
	}

	/* did we get the info ???? */

	if ((*selection != OlInternAtom(XtDisplay(vendor),
				        _SUN_DRAGDROP_DSDM_NAME) &&
	    *type != XA_INTEGER) ||
	    *length == 0 || value == (XtPointer)NULL) {
		dnd_part->pending_dsdm_info = False;
		dnd_part->dsdm_present = False;
		return;	/* oops */
	}

	if (dnd_part->dsdm_rects != (InternalDSDMSRPtr *)NULL) {
		DeleteDSDMSRList(dnd_part);
		XtFree((char *)dnd_part->internal_dsdm_sr_list);
		XtFree((char *)dnd_part->dsdm_sr_list);
	}

	/* yes .. put it in the cache */

	dnd_part->dsdm_last_loaded =
		XtLastTimestampProcessed(XtDisplay(vendor));

	dnd_part->dsdm_sr_list = (DSDMSiteRectPtr)value;
	dnd_part->num_dsdmsrs = *length / (sizeof (DSDMSiteRect) / sizeof(int));
	dnd_part->internal_dsdm_sr_list =
			(InternalDSDMSRPtr) XtCalloc(dnd_part->num_dsdmsrs,
						     sizeof(InternalDSDMSR));

	for (i = 0; i < dnd_part->num_dsdmsrs; i++) {
		InternalDSDMSRRect(dnd_part->internal_dsdm_sr_list[i]) =
			dnd_part->dsdm_sr_list + i;
	}

	InsertDSDMSRsIntoDSDMSRList(dnd_part, dnd_part->internal_dsdm_sr_list,
				    dnd_part->num_dsdmsrs);

	dnd_part->pending_dsdm_info = False;
	dnd_part->dsdm_present = True;
}

/*
 * request the list of drop sites from the dsdm ....
 */

static Boolean
DnDVCXFetchDSDMInfo (Widget vendor, OlDnDVendorPartExtension dnd_part, Time timestamp)
{
	XtAppContext			app;

	app = XtDisplayToApplicationContext(XtDisplay(vendor));

	if (!XtIsRealized(vendor) || dnd_part->pending_dsdm_info)
		return (False);

        FetchPartExtensionIfNull(vendor, dnd_part);

	if (timestamp == CurrentTime &&
	    (timestamp = XtLastTimestampProcessed(XtDisplay(vendor))) == CurrentTime)
		return (False);

	dnd_part->pending_dsdm_info = True;

	XtGetSelectionValue(vendor,
			    OlInternAtom(XtDisplay(vendor),
					 _SUN_DRAGDROP_DSDM_NAME), 
			    OlInternAtom(XtDisplay(vendor),
			    		 _SUN_DRAGDROP_SITE_RECTS_NAME), 
			    _DnDVCXDSDMSiteRectSelectionCB,
			    (XtPointer)dnd_part, timestamp);

	if (!dnd_part->do_dsdm_fetches_async) {
		while (dnd_part->pending_dsdm_info) {
			XtAppProcessEvent(app, XtIMAll);
		}
	}

	/*
	 * we have to wait a while .....
	 */

	return (!dnd_part->do_dsdm_fetches_async &&
		dnd_part->dsdm_sr_list != (DSDMSiteRectPtr)NULL);
}

/******************************************************************************
 *
 * DnDVCXDeliverTriggerMessage
 *
 * deliver a trigger message ....... if the dsdm is present lookup the
 * internal cache of site rects .... otherwise hunt the window heirarchy
 * for another shell with the drop site info .... if the drop site is 
 * local then call the dispatcher otherwise send the event.
 *
 ******************************************************************************/

static Boolean
DnDVCXDeliverTriggerMessage (Widget vendor, OlDnDVendorPartExtension dnd_part, Widget widget, Window root, int rootx, int rooty, Atom selection, OlDnDTriggerOperation operation, Time timestamp)
{
	TriggerMessage			trigger_message;
	XClientMessageEvent		client_message;
	InternalDSDMSRPtr		idsdmsr = (InternalDSDMSRPtr)NULL;
	unsigned int			scrn, noofscreens;
	Boolean				ret = False;
	Window				window_id = (Window)NULL;
	OlDnDDropSiteID			dropsiteid = (OlDnDDropSiteID)NULL;
	OlDnDTriggerFlags		flags = 0;
	SiteDescriptionPtr		site = (SiteDescriptionPtr)NULL;
	Widget				local_vendor = (Widget)NULL;
	OlDnDVendorPartExtension	part;
	Display				*dpy = XtDisplayOfObject(widget);


        FetchPartExtensionIfNull(vendor, dnd_part);

	noofscreens = ScreenCount(dpy);

	for (scrn = 0;
	     scrn < noofscreens && root != RootWindow(dpy, scrn);
	     scrn++)
	;

	if (dnd_part->dsdm_present) {
		idsdmsr = LookupXYInDSDMSRList(dnd_part,
					       ScreenOfDisplay(dpy, scrn),
				       	       rootx, rooty);

		if (idsdmsr != (InternalDSDMSRPtr)NULL) {
			window_id = InternalDSDMSRPtrRectWindowID(idsdmsr);
			dropsiteid = (OlDnDDropSiteID)
					InternalDSDMSRPtrRectSiteID(idsdmsr);	
		}
	}

	if (!dnd_part->dsdm_present || idsdmsr == (InternalDSDMSRPtr)NULL)
	     if (FindDropSiteInfoFromDestination(ScreenOfDisplay(dpy, scrn),
				                 root, rootx, rooty, &site)) {
		window_id = SiteDescriptionPtrEventWindow(site);
		dropsiteid = (OlDnDDropSiteID)SiteDescriptionPtrSiteID(site);
		XtFree((char *)site);
	     }
					     
	if (idsdmsr == (InternalDSDMSRPtr)NULL &&
	    site == (SiteDescriptionPtr)NULL) {
		return (False);
	}

	if (dnd_part->current_dsdmsr != (InternalDSDMSRPtr)NULL)
		InternalDSDMSRPtrInSite(dnd_part->current_dsdmsr) = False;

	/* we've got the drop site ..... lets do it! */

	
	TriggerMessageType(trigger_message) = 
		OlInternAtom(XtDisplay(vendor), _SUN_DRAGDROP_TRIGGER_NAME);

	TriggerMessageWindow(trigger_message) = window_id;

	TriggerMessageSiteID(trigger_message) = (unsigned long)dropsiteid;

	flags = OlDnDTriggerAck; /* well, we might as well! */

	if (idsdmsr != (InternalDSDMSRPtr)NULL) {
		if (InternalDSDMSRPtrRectIsForwarded(idsdmsr))
			flags |= OlDnDTriggerForwarded;
	} else {
		if (SiteDescriptionPtrIsForwarded(site))
			flags |= OlDnDTriggerForwarded;
	}

	switch (operation) {
		case OlDnDTriggerCopyOp:
			flags |= OlDnDTriggerCopy;
			break;
		default:
		case OlDnDTriggerMoveOp:
			flags |= OlDnDTriggerMove;
			break;
	}

	/* check if transient */

	if (_OlDnDAtomIsTransient(vendor, dnd_part, selection))
			flags |= OlDnDTriggerTransient;

	TriggerMessageFlags(trigger_message) = flags;

	TriggerMessageX(trigger_message) = rootx;
	TriggerMessageY(trigger_message) = rooty;

	TriggerMessageSelection(trigger_message) = selection;
	TriggerMessageTimestamp(trigger_message) = timestamp;

	/* try locally first */

	local_vendor = XtWindowToWidget(XtDisplay(vendor), window_id);

	if (local_vendor != (Widget)NULL &&
	    XtIsSubclass(local_vendor, vendorShellWidgetClass) &&
	    (part = _OlGetDnDVendorPartExtension(local_vendor)) !=
					(OlDnDVendorPartExtension)NULL) {
		OlDnDVCXTMDispatcherFunc	tm_dispatcher;

		tm_dispatcher =
			part->class_extension->trigger_message_dispatcher;

		if (tm_dispatcher != (OlDnDVCXTMDispatcherFunc)NULL) {
			ret = (*tm_dispatcher)
				    (local_vendor, part, &trigger_message);
		}
	}

	if (!ret) {
		CopyTriggerMessageToClientMessage(dpy,
						  &trigger_message,
						  &client_message);

		ret = XSendEvent(dpy, client_message.window, False,
				 NoEventMask, (XEvent *)&client_message);
	}
 
	{	/* start the timer for the transaction */

		OwnerProcClosurePtr	opc;
		Widget			sw;
	
		sw = _OlDnDGetWidgetForSelection(vendor, dnd_part,
						 selection, &opc);
		if (sw != (Widget)NULL)
			_StartDnDTxTimer(opc);
	}

	return (ret);
}


/******************************************************************************
 *
 * DnDVCXDeliverPreviewMessage
 *
 * Send a preview message .... first find the drop site its intended for,
 * deliver any LeaveNotifies that are required (if we have left a drop
 * site we were previously in ..... if the drop site is local call the
 * dispatcher directly .... otherwise send the event ......
 *
 ******************************************************************************/

static Boolean
DnDVCXDeliverPreviewMessage (Widget vendor, OlDnDVendorPartExtension dnd_part, Widget widget, Window root, int rootx, int rooty, Time timestamp, OlDnDPreviewAnimateCallbackProc animate_proc, XtPointer closure)
{
	PreviewMessage			preview_message;
	XClientMessageEvent		client_message;
	InternalDSDMSRPtr		idsdmsr, prev_cache;
	unsigned int			scrn, noofscreens;
	Screen				*screen;
	Boolean				ret_val = True;
	Boolean				deliver_preview;
	unsigned long			eventcode;
	Widget				local_vendor = (Widget)NULL;
	OlDnDVendorPartExtension	part;
	Display				*dpy = XtDisplayOfObject(widget);
	OlDnDSitePreviewHints		hints;
	Boolean				insensitive;

        FetchPartExtensionIfNull(vendor, dnd_part);

	noofscreens = ScreenCount(dpy);

	for (scrn = 0;
	     scrn < noofscreens && root != RootWindow(dpy, scrn);
	     scrn++)
	;

	screen = ScreenOfDisplay(dpy, scrn);
	prev_cache = dnd_part->current_dsdmsr;

	idsdmsr = LookupXYInDSDMSRList(dnd_part, screen, rootx, rooty);

	/* deliver any leave messages first! */

	PreviewMessageType(preview_message) = 
		OlInternAtom(dpy, _SUN_DRAGDROP_PREVIEW_NAME);

	if (prev_cache != (InternalDSDMSRPtr)NULL) { /* we were in a site */
		Boolean	ret = False;
		Boolean	prev_forwarded,
			current_forwarded;

		/* if we are not still in it ... deliver a Leave event if
		 * appropriate
		 */

		prev_forwarded  =
			InternalDSDMSRPtrRectIsForwarded(prev_cache);
		current_forwarded = 
			(idsdmsr != (InternalDSDMSRPtr)NULL &&
			 InternalDSDMSRPtrRectIsForwarded(idsdmsr));

		if (!XYInInternalDSDMSR(prev_cache, scrn, rootx, rooty) &&
		    (idsdmsr == (InternalDSDMSRPtr)NULL                 ||
		     ((prev_forwarded && !current_forwarded)            ||
		      (!prev_forwarded && current_forwarded))	        ||
		     InternalDSDMSRPtrRectSiteID(idsdmsr) !=
		     InternalDSDMSRPtrRectSiteID(prev_cache))) {
			InternalDSDMSRPtrInSite(prev_cache) = False;
			deliver_preview =
				InternalDSDMSRPtrRectWantsPreviewEnterLeave(
							    prev_cache);
			eventcode = LeaveNotify;
			insensitive = InternalDSDMSRPtrRectIsInsensitive(prev_cache);

			if (animate_proc !=
			    (OlDnDPreviewAnimateCallbackProc)NULL) {
				(*animate_proc)(widget,
						(int)eventcode,
						(Time)timestamp,
						(int)insensitive,
						closure);
			}

		} else { /* we are still inside it !!!! */
			deliver_preview = False;

			if (idsdmsr != (InternalDSDMSRPtr)NULL &&
			    InternalDSDMSRPtrRectSiteID(idsdmsr) ==
			    InternalDSDMSRPtrRectSiteID(prev_cache))
				InternalDSDMSRPtrInSite(idsdmsr) = True;
		}

		if (deliver_preview) {
			hints = InternalDSDMSRPtrRectPreviewHints(prev_cache);

			PreviewMessageWindow(preview_message) = 
			     InternalDSDMSRPtrRectWindowID(prev_cache);	

			PreviewMessageSiteID(preview_message) =
			     InternalDSDMSRPtrRectSiteID(prev_cache);

			PreviewMessageEventcode(preview_message) = eventcode;

			if (InternalDSDMSRPtrRectIsForwarded(prev_cache))
				PreviewMessageFlags(preview_message) = 
						OlDnDPreviewForwarded;

			PreviewMessageX(preview_message) = rootx;
			PreviewMessageY(preview_message) = rooty;

			PreviewMessageTimestamp(preview_message) = timestamp;

#define	PCE_PMD(part)	(part)->class_extension->preview_message_dispatcher

			/* try locally */

			local_vendor = XtWindowToWidget(XtDisplay(vendor),
					PreviewMessageWindow(preview_message));

			if (local_vendor != (Widget)NULL &&
			    XtIsSubclass(local_vendor, vendorShellWidgetClass)
			    && (part = _OlGetDnDVendorPartExtension(local_vendor))
			             != (OlDnDVendorPartExtension)NULL) {
				OlDnDVCXPMDispatcherFunc	pm_dispatcher;

				pm_dispatcher = PCE_PMD(part);
				if (pm_dispatcher != (OlDnDVCXPMDispatcherFunc)NULL)
					ret = (*pm_dispatcher) 
							(local_vendor,
							 part,
							 &preview_message);
			}

			if (!ret) { /* remote site */
				CopyPreviewMessageToClientMessage(dpy,
					&preview_message, &client_message);

				ret = XSendEvent(dpy, client_message.window,
						 False, NoEventMask,
						 (XEvent *)&client_message);
			} 
		} else ret = True;

		ret_val = ret;
	}

	dnd_part->current_dsdmsr = idsdmsr; /* update the cache */

	if (idsdmsr == (InternalDSDMSRPtr)NULL) {
		return (False); /* oops no sites */
	}

	if (InternalDSDMSRPtrInSite(idsdmsr)) {
		eventcode =  MotionNotify;
		deliver_preview = 
			InternalDSDMSRPtrRectWantsPreviewMotion(idsdmsr);
	} else {
		eventcode = EnterNotify;
		InternalDSDMSRPtrInSite(idsdmsr) = True;
		deliver_preview =
			InternalDSDMSRPtrRectWantsPreviewEnterLeave(idsdmsr);

		insensitive = InternalDSDMSRPtrRectIsInsensitive(idsdmsr);
		if (animate_proc !=
		    (OlDnDPreviewAnimateCallbackProc)NULL) {
			(*animate_proc)(widget,
					(int)eventcode,
					(Time)timestamp,
					(int)insensitive,
					closure);
		}
	}


	if (deliver_preview) {
		Boolean	ret = False;

		hints = InternalDSDMSRPtrRectPreviewHints(idsdmsr);

		PreviewMessageWindow(preview_message) = 
			InternalDSDMSRPtrRectWindowID(idsdmsr);	

		PreviewMessageSiteID(preview_message) =
			InternalDSDMSRPtrRectSiteID(idsdmsr);

		PreviewMessageEventcode(preview_message) = eventcode;

		PreviewMessageX(preview_message) = rootx;
		PreviewMessageY(preview_message) = rooty;

		PreviewMessageTimestamp(preview_message) = timestamp;

		if (InternalDSDMSRPtrRectIsForwarded(idsdmsr))
			PreviewMessageFlags(preview_message) = 
					OlDnDPreviewForwarded;

		/* try locally */

		local_vendor = XtWindowToWidget(XtDisplay(vendor),
					PreviewMessageWindow(preview_message));

		if (local_vendor != (Widget)NULL &&
		    XtIsSubclass(local_vendor, vendorShellWidgetClass) &&
		    (part = _OlGetDnDVendorPartExtension(local_vendor)) !=
					(OlDnDVendorPartExtension)NULL) {
			OlDnDVCXPMDispatcherFunc	pm_dispatcher;

			pm_dispatcher = PCE_PMD(part);

			if (pm_dispatcher != (OlDnDVCXPMDispatcherFunc)NULL) {
			    ret = (*pm_dispatcher) (local_vendor,
						    part,
						    &preview_message);
			}
		}

		if (!ret) {
			CopyPreviewMessageToClientMessage(dpy,
							  &preview_message,
							  &client_message);

			ret = XSendEvent(dpy, client_message.window,
					      False, NoEventMask,
					      (XEvent *)&client_message);
		}
		ret_val &= ret;
	}

	return (ret_val);
}


/******************************************************************************
 *
 * DnDVCXInitializeDragState
 *
 * enter drag state.
 *
 ******************************************************************************/

static Boolean
DnDVCXInitializeDragState (Widget vendor, OlDnDVendorPartExtension dnd_part)
{
	FetchPartExtensionIfNull(vendor, dnd_part);

	if (dnd_part->doing_drag)
		return (True);

	dnd_part->doing_drag = True;

	/* just in case */
	if (dnd_part->current_dsdmsr != (InternalDSDMSRPtr)NULL)
		InternalDSDMSRPtrInSite(dnd_part->current_dsdmsr) = False;

	dnd_part->current_dsdmsr = (InternalDSDMSRPtr)NULL;

	if (dnd_part->class_extension->fetch_dsdm_info !=
	    (OlDnDVCXFetchDSDMInfoFunc)NULL) {
		return (*dnd_part->class_extension->fetch_dsdm_info)
				(vendor, dnd_part, 
				 XtLastTimestampProcessed(XtDisplay(vendor)));
	}

	return (False);
}

/******************************************************************************
 *
 * DnDVCXClearDragState
 *
 * clear down the drag state.
 *
 ******************************************************************************/

static void
DnDVCXClearDragState (Widget vendor, OlDnDVendorPartExtension dnd_part)
{
	FetchPartExtensionIfNull(vendor, dnd_part);

	dnd_part->doing_drag = False;

	if (dnd_part->current_dsdmsr != (InternalDSDMSRPtr)NULL)
		InternalDSDMSRPtrInSite(dnd_part->current_dsdmsr) = False;

	dnd_part->current_dsdmsr = (InternalDSDMSRPtr)NULL;
}

/******************************************************************************
 *
 * DnDVCXAllocTransientAtom
 *
 * allocate a "transient" atom for use as a temporary selection atom ????
 *
 ******************************************************************************/

#define	INCRLIST	5

static Atom
DnDVCXAllocTransientAtom (Widget vendor, OlDnDVendorPartExtension dnd_part, Widget widget)
{
	TransientAtomListPtr	transients;
	unsigned int		i;
	Atom			atom;

	FetchPartExtensionIfNull(vendor, dnd_part);

	transients = dnd_part->transient_atoms;
	if (transients  == (TransientAtomListPtr)NULL ||
	     NeedsLargerTransientAtomList(transients)) {
		unsigned int	size, prev = 0, used = 0;

		if (transients) {
			prev = TransientAtomListPtrAlloc(transients);
			used = TransientAtomListPtrUsed(transients);
		}

		size = SizeOfTransientAtomListForNAtoms(prev + INCRLIST);
					
		dnd_part->transient_atoms = transients =
			(TransientAtomListPtr)XtRealloc((char *)transients,
							 size);

		TransientAtomListPtrAlloc(transients) = prev + INCRLIST;
		TransientAtomListPtrUsed(transients) = used;

		for (i = prev; i < TransientAtomListPtrAlloc(transients);
		     i++) {	
			TransientAtomListPtrAtom(transients, i) = (Atom)NULL;
			TransientAtomListPtrOwner(transients, i) =
				(Widget)NULL;
		}
	}

	for (i = 0;
	     i < TransientAtomListPtrAlloc(transients) &&
	     TransientAtomListPtrOwner(transients, i) != (Widget)NULL; i++)
	;
	if ((atom = TransientAtomListPtrAtom(transients, i)) == (Atom)NULL) {
		char *str;

		if (str = malloc(64)) {
			snprintf(str, 64, "_SUN_DRAGDROP_TRANSIENT_%08x_%03d",
					XtWindow(vendor), i);
			TransientAtomListPtrAtom(transients, i) = atom =
			OlInternAtom(XtDisplay(vendor), str);
			free(str);
		}

	}

	TransientAtomListPtrOwner(transients, i) = widget;
	TransientAtomListPtrUsed(transients)++;

	return (atom);
}

/******************************************************************************
 *
 * DnDVCXFreeTransientAtom
 *
 * free a transient atom for re-use.
 *
 ******************************************************************************/

static void
DnDVCXFreeTransientAtom (Widget vendor, OlDnDVendorPartExtension dnd_part, Widget widget, Atom atom)
{
	TransientAtomListPtr	transients;
	unsigned int		i;

	if ((transients = dnd_part->transient_atoms) !=
	    (TransientAtomListPtr)NULL) {
		for (i = 0; i < TransientAtomListPtrAlloc(transients); i++)
			if (TransientAtomListPtrOwner(transients, i) == widget
			    && (atom == FreeAllTransientAtoms ||
			    atom == TransientAtomListPtrAtom(transients, i))) {
				TransientAtomListPtrOwner(transients, i) =
					(Widget)NULL;
				TransientAtomListPtrUsed(transients)--;
			}
	}
}

/******************************************************************************
 *
 * DnDVCXAssocSelectionWithWidget
 *
 * bind a selection atom to a widget.
 *
 ******************************************************************************/

static DSSelectionAtomPtr
DnDVCXAssocSelectionWithWidget (Widget vendor, OlDnDVendorPartExtension dnd_part, Widget widget, Atom selection, Time timestamp, OwnerProcClosurePtr closure)
{
	DSSelectionAtomPtr	part_list, p;

	FetchPartExtensionIfNull(vendor, dnd_part);

	part_list = dnd_part->selection_atoms;

	for (p = part_list; p != (DSSelectionAtomPtr)NULL &&
	     selection != DSSelectionAtomPtrSelectionAtom(p);
	     p = DSSelectionAtomPtrNext(p))
	;

	if (p != (DSSelectionAtomPtr)NULL) { /* someone else still has it */
		return p;
	}

	p = (DSSelectionAtomPtr)XtCalloc(1, sizeof(DSSelectionAtom));

	DSSelectionAtomPtrSelectionAtom(p)   = selection;
	DSSelectionAtomPtrOwner(p)   	     = widget;
	DSSelectionAtomPtrTimestamp(p) 	     = timestamp;
	DSSelectionAtomPtrClosure(p)         = closure;
	DSSelectionAtomPtrRequestorWindow(p) = (Window)NULL;
	DSSelectionAtomPtrRequestorDisplay(p) = (Display *)NULL;

	DSSelectionAtomPtrNext(p) = part_list;
	dnd_part->selection_atoms = p;

	return p;
}

/******************************************************************************
 *
 * DnDVCXDissassocSelectionWithWidget
 *
 * free a selection from its associated widget.
 *
 ******************************************************************************/


static void	
DnDVCXDissassocSelectionWithWidget (Widget vendor, OlDnDVendorPartExtension dnd_part, Widget widget, Atom selection, Time timestamp)
{
	DSSelectionAtomPtr	*p, q;

	FetchPartExtensionIfNull(vendor, dnd_part);

	for (;;) {

		for (p = &dnd_part->selection_atoms;
		     *p != (DSSelectionAtomPtr)NULL;
		     p = &DSSelectionAtomPtrNext(*p)) {
		     if (DSSelectionAtomPtrOwner(*p) == widget &&
		         (selection == FreeAllSelectionAtoms ||
		          selection == DSSelectionAtomPtrSelectionAtom(*p)))
				break;
		}

		if (*p == (DSSelectionAtomPtr)NULL)
			return; /* its not there */

		q = *p;
		*p = DSSelectionAtomPtrNext(*p); /* unlink from ds */

		if (DSSelectionAtomPtrClosure(q) != 
		    (OwnerProcClosurePtr)NULL && 
		    OwnerProcClosurePtrCleanupProc(
				DSSelectionAtomPtrClosure(q))
		    != (OlDnDTransactionCleanupProc)NULL)
			(*OwnerProcClosurePtrCleanupProc(
				DSSelectionAtomPtrClosure(q)))
					(DSSelectionAtomPtrClosure(q));
	
		XtFree((char *)q);

		if (selection != FreeAllSelectionAtoms)
			return;	/* all done */
	}
}

/******************************************************************************
 *
 * DnDVCXGetCurrentSelectionsForWidget
 *
 * find the list of selections active for the widget.
 *
 ******************************************************************************/

static Atom     
*DnDVCXGetCurrentSelectionsForWidget (Widget vendor, OlDnDVendorPartExtension dnd_part, Widget widget, Cardinal *num_sites_return)
{
	Atom			*atoms, *a;
	DSSelectionAtomPtr	p;

	*num_sites_return = 0;

	FetchPartExtensionIfNull(vendor, dnd_part);

	for (p = dnd_part->selection_atoms;
	     p != (DSSelectionAtomPtr)NULL; 
	     p = DSSelectionAtomPtrNext(p))
		if (DSSelectionAtomPtrOwner(p) == widget)
			*num_sites_return++;

	if (!*num_sites_return)
		return (Atom *)NULL;

	a = atoms = (Atom *)XtCalloc(*num_sites_return, sizeof(Atom));

	for (p = dnd_part->selection_atoms;
	     p != (DSSelectionAtomPtr)NULL; 
	     p = DSSelectionAtomPtrNext(p))
		if (DSSelectionAtomPtrOwner(p) == widget)
			*a++ = DSSelectionAtomPtrSelectionAtom(p);

	return atoms;
}

/******************************************************************************
 *
 * DnDVCXGetDropSiteForSelection
 *
 * find the widget associated with a particular selection atom.
 *
 ******************************************************************************/

static Widget
DnDVCXGetWidgetForSelection (Widget vendor, OlDnDVendorPartExtension dnd_part, Atom selection, OwnerProcClosurePtr *closure)
{
	DSSelectionAtomPtr	p;
	Widget			w;

	FetchPartExtensionIfNull(vendor, dnd_part);

	for (p = dnd_part->selection_atoms;
	     p != (DSSelectionAtomPtr)NULL &&
	     selection != DSSelectionAtomPtrSelectionAtom(p);
	     p = DSSelectionAtomPtrNext(p))
	;

	if (p != (DSSelectionAtomPtr)NULL) {
		w = DSSelectionAtomPtrOwner(p);
		*closure = DSSelectionAtomPtrClosure(p);
	} else {
		w = (Widget)NULL;
		*closure = (OwnerProcClosurePtr)NULL;
	}
	
	return (w);
}

/******************************************************************************
 *
 * DnDVCXChangeSitePreviewHints
 *
 * update the site peview hints ....
 *
 ******************************************************************************/

static Boolean
DnDVCXChangeSitePreviewHints (Widget vendor, OlDnDVendorPartExtension dnd_part, OlDnDDropSiteID dropsiteid, OlDnDSitePreviewHints new_hints)
{
	OlDnDDropSitePtr	dsp;

	FetchPartExtensionIfNull(vendor, dnd_part);

        for (dsp = dnd_part->drop_site_list;
             dsp != (OlDnDDropSitePtr)NULL &&
             dsp != (OlDnDDropSitePtr)dropsiteid;
                dsp = OlDnDDropSitePtrNextSite(dsp))
        ;

	if (dsp == (OlDnDDropSitePtr)NULL) {
		return (False);
	}

	new_hints &= (OlDnDSitePreviewNone	 | /* redundant */
		      OlDnDSitePreviewEnterLeave |
		      OlDnDSitePreviewMotion     |
		      OlDnDSitePreviewBoth       | /* redundant */
		      OlDnDSitePreviewDefaultSite|
		      OlDnDSitePreviewInsensitive);

	if (OlDnDDropSitePtrPreviewHints(dsp) == new_hints) {
		return (True);
	}
	
	if ((OlDnDDropSitePtrPreviewHints(dsp) & OlDnDSitePreviewDefaultSite)
	    && !(new_hints & OlDnDSitePreviewDefaultSite)) {
		dnd_part->default_drop_site = (OlDnDDropSitePtr)NULL;
	}

	OlDnDDropSitePtrPreviewHints(dsp) = new_hints;

	if ((OlDnDDropSitePtrPreviewHints(dsp) & OlDnDSitePreviewDefaultSite)
	    && dnd_part->default_drop_site != dsp) {
		dnd_part->default_drop_site = dsp;
	}

	dnd_part->dirty = True;

	if (dnd_part->auto_assert_dropsite_registry &&
	    dnd_part->class_extension->assert_drop_site_registry !=
	    (OlDnDVCXAssertRegistryProc)NULL) {
		(*dnd_part->class_extension->assert_drop_site_registry)
			(vendor, dnd_part);
	}
	
	return (True);
}

/******************************************************************************
 *
 * DnDVCXSetDropSiteOnInterest
 *
 ******************************************************************************/

static Boolean
DnDVCXSetDropSiteOnInterest (
			     Widget vendor,
			     OlDnDVendorPartExtension dnd_part,
			     OlDnDDropSiteID dropsiteid,
			     Boolean on_interest,
			     Boolean validate)
{
	OlDnDDropSitePtr	dsp;
	Boolean			save;

	FetchPartExtensionIfNull(vendor, dnd_part);

	if (validate)
		for (dsp = dnd_part->drop_site_list;
		     dsp != (OlDnDDropSitePtr)NULL &&
		     dsp != (OlDnDDropSitePtr)dropsiteid;
			dsp = OlDnDDropSitePtrNextSite(dsp))
		;

	if (dsp == (OlDnDDropSitePtr)NULL) {
		return (False);
	}

	save = OlDnDDropSitePtrOnInterest(dsp);
	OlDnDDropSitePtrOnInterest(dsp) = on_interest;

	if ((!save && on_interest) || (save && !on_interest)) {
		dnd_part->dirty = True;

		if (dnd_part->auto_assert_dropsite_registry &&
		    dnd_part->class_extension->assert_drop_site_registry !=
		    (OlDnDVCXAssertRegistryProc)NULL) {
			(*dnd_part->class_extension->assert_drop_site_registry)
				(vendor, dnd_part);

			if (dnd_part->class_extension->fetch_dsdm_info !=
			    (OlDnDVCXFetchDSDMInfoFunc)NULL) {
				(*dnd_part->class_extension->fetch_dsdm_info)
					(vendor, dnd_part, 
					 XtLastTimestampProcessed(
						XtDisplay(vendor)));
			}
		}
	}
	return (True);
}

/******************************************************************************
 *
 * DnDVCXSetInterestInWidgetHier
 *
 ******************************************************************************/

static OlDnDDropSitePtr	*_site_list = (OlDnDDropSitePtr *)NULL;

static OlDnDVendorPartExtension	_dnd_part = (OlDnDVendorPartExtension)NULL;
static Boolean			_interest;
static Widget			_vendor;


static int
_comparWidgets(const void *i, const void *j)
{
	OlDnDDropSitePtr	ip = (OlDnDDropSitePtr)i,
				jp = (OlDnDDropSitePtr)j;

	return	((int)OlDnDDropSitePtrOwner(jp) -
		 (int)OlDnDDropSitePtrOwner(ip));
}

static void
_recurshier(Widget w)
{
	register OlDnDDropSitePtr	dsp;
	register CompositeWidget	comp = (CompositeWidget)w;

	dsp = (OlDnDDropSitePtr)bsearch((char *)&w, (char *)_site_list,
					_dnd_part->number_of_sites, 
					sizeof(Widget), _comparWidgets);

	if (dsp != (OlDnDDropSitePtr)NULL && 
	    _dnd_part->class_extension->set_ds_on_interest !=
	    (OlDnDVCXSetDSOnInterestFunc)NULL) {
		(*_dnd_part->class_extension->set_ds_on_interest)
				(_vendor, _dnd_part, (OlDnDDropSiteID)dsp,
				 _interest && w->core.mapped_when_managed,
				 False);
	}

	if (XtIsComposite(w) && comp->composite.num_children) {
		register Cardinal 	n;
		register WidgetList	child = comp->composite.children;

		for (n = 0; n < comp->composite.num_children; n++) {
			_recurshier(child[n]);
		}
	}
}

static void
DnDVCXSetInterestInWidgetHier (
			       Widget vendor,
			       OlDnDVendorPartExtension dnd_part,
			       Widget widget,
			       Boolean on_interest)
{
	OlDnDDropSitePtr	dsp, *p;
	Boolean			save;

	FetchPartExtensionIfNull(vendor, dnd_part);
	
	if (dnd_part->number_of_sites == 0) return;

	p = _site_list = (OlDnDDropSitePtr *)XtCalloc(dnd_part->number_of_sites,
						      sizeof(OlDnDDropSitePtr));

	for (dsp = dnd_part->drop_site_list;
	     dsp != (OlDnDDropSitePtr)NULL;
	     (dsp = OlDnDDropSitePtrNextSite(dsp)), p++)
		*p = dsp;
	     
	qsort((char *)_site_list, dnd_part->number_of_sites, 
	      sizeof(OlDnDDropSitePtr), _comparWidgets);

	save = dnd_part->auto_assert_dropsite_registry;
	dnd_part->auto_assert_dropsite_registry = False;

	_interest = on_interest;
	_vendor = vendor;
	_dnd_part = dnd_part;

	_recurshier(widget);

	dnd_part->auto_assert_dropsite_registry = save;

	if (dnd_part->auto_assert_dropsite_registry &&
	    dnd_part->class_extension->assert_drop_site_registry !=
	    (OlDnDVCXAssertRegistryProc)NULL) {
		(*dnd_part->class_extension->assert_drop_site_registry)
			(vendor, dnd_part);

		if (dnd_part->class_extension->fetch_dsdm_info !=
		    (OlDnDVCXFetchDSDMInfoFunc)NULL) {
			(*dnd_part->class_extension->fetch_dsdm_info)
					(vendor, dnd_part, 
					 XtLastTimestampProcessed(
						XtDisplay(vendor)));
		}
	}
	
	XtFree((char *)_site_list);
	_site_list = (OlDnDDropSitePtr *)NULL;
}

/******************************************************************************
 *
 * DnDVCXClipDropSites
 *
 ******************************************************************************/

/*
 * clip the site according to the region
 */

static void
_ClipDropSite (register OlDnDDropSitePtr dsp, Widget widget, unsigned int depth, register Region region, Position xt, Position yt, Boolean visible)
{
	Region				r, save;
	Position			lxt,
					lyt;
	Boolean				visible_hint;
	register OlDnDSiteRectPtr	sr;
	register InternalDSRPtr		isr;
	unsigned int			i;

	if (!visible) {
		OlDnDDropSitePtrVisibility(dsp) = ClippedSiteNotVisible;
		return;
	}

	r = _CalculateClipAndTransform(widget, depth,
				       OlDnDDropSitePtrOwner(dsp),
				       OlDnDDropSitePtrOwnerDepth(dsp),
				       &lxt, &lyt, &visible_hint);

	if (XEmptyRegion(r) || !visible_hint) {
		OlDnDDropSitePtrVisibility(dsp) = ClippedSiteNotVisible;
		XDestroyRegion(r);
		return;
	}

	isr = OlDnDDropSitePtrTopLevelRects(dsp);
	sr  = OlDnDDropSitePtrRects(dsp);

	if (OlDnDDropSitePtrWindow(dsp) !=
	    XtWindowOfObject(OlDnDDropSitePtrOwner(dsp))) {
		int	x,
			y;
		Window	child;

		/* its a window - find the transform to map window -> widget */

		XTranslateCoordinates(XtDisplayOfObject(widget),
				      OlDnDDropSitePtrWindow(dsp),
				      XtWindowOfObject(
						OlDnDDropSitePtrOwner(dsp)),
				      0, 0, &x, &y, &child);

		if (child != OlDnDDropSitePtrWindow(dsp)) {
			OlDnDDropSitePtrVisibility(dsp) = ClippedSiteNotVisible;
			XDestroyRegion(r);
			return;
		}

		xt += x;
		yt += y;

		/*
		 * NOTE:
		 * 
		 * if we were to do this properly we would walk the window
		 * tree from the ds window back up to the owning widget
		 * calculating the visible clip region ... then we would
		 * intersect that with the widgets clip region here ...
		 * but we dont right now ... it would be a pretty expensive
		 * operation anyway ...
		 *
		 */
	}

	XOffsetRegion(r, xt, yt);

	XIntersectRegion(region, r, r);	/* clip it */

	save = r;

	for (i = 0; i < OlDnDDropSitePtrNumRects(dsp);sr++, isr++, i++) {
		XRectangle	rect;
		Region		dsr;
		int		rect_in_region;

		rect.x      = SiteRectPtrX(sr) + xt + lxt;
		rect.y      = SiteRectPtrY(sr) + yt + lyt;
		rect.width  = SiteRectPtrWidth(sr);
		rect.height = SiteRectPtrHeight(sr);

		rect_in_region = XRectInRegion(r, rect.x, rect.y,
					       rect.width, rect.height);

		if (rect_in_region == RectangleOut) {
			InternalDSRPtrRectVisible(isr) = ClippedSiteNotVisible;
			visible_hint = (Boolean)False;
			continue;
		}

		r = save;
		save = XCreateRegion();
		XUnionRegion(r, save, save);

		dsr = XCreateRegion();
		XUnionRectWithRegion(&rect, dsr, dsr);
		
		XIntersectRegion(r, dsr, dsr);
		XClipBox(dsr, &rect);

		InternalDSRPtrTopLevelRectX(isr)      = rect.x;
		InternalDSRPtrTopLevelRectY(isr)      = rect.y;
		InternalDSRPtrTopLevelRectWidth(isr)  = rect.width;
		InternalDSRPtrTopLevelRectHeight(isr) = rect.height;
		InternalDSRPtrRectVisible(isr)        = ClippedSiteVisible;

		XDestroyRegion(r);
		XDestroyRegion(dsr);
		visible_hint = (Boolean)True;
	}

	OlDnDDropSitePtrVisibility(dsp) = ( visible_hint
						? ClippedSiteVisible
						: ClippedSiteNotVisible);

	XDestroyRegion(save);
}

/*
 * assumes that the lkist of groups are those effected.
 */

static void
_ClipDropSiteGroups (Widget vendor, Widget w, unsigned int depth, DropSiteGroupPtr *groups, int num_groups)
{
	register Region	wr;
	Position	xt = (Position)0,
			yt = (Position)0;
	int		i;
	Boolean		visible_hint;

	/* get the clip and transform in TLW co-ords for the config widget */

	wr = _CalculateClipAndTransform(vendor, 0, w, depth, &xt, &yt, 
					&visible_hint
	     );

	if (!visible_hint) return;

	for (i = 0; i < num_groups; i++, groups++) {
		OlDnDDropSitePtr dsp;
		int		 j;
		Boolean		 group_visible = visible_hint,
				 lca_subw_conf = (Boolean)False;
		Region		 lr    = (Region)NULL,
				 save  = (Region)NULL;
		Position	 lxt   = (Position)0,
				 lyt   = (Position)0;
		Widget		 confw = DropSiteGroupPtrLCA(*groups);
		unsigned int	 confd = DropSiteGroupPtrLCADepth(*groups);
	
		if (confd > depth) { /* widget above group configured */
			lr = _CalculateClipAndTransform(w, depth,
							confw, confd,
							&lxt, &lyt,
							&group_visible
			     );

			if (group_visible &= !XEmptyRegion(lr)) {
				XOffsetRegion(lr, xt, yt);
				XIntersectRegion(wr, lr, lr);
			}

			if (confd - (int)depth > 1) {
				lxt += xt;
				lyt += yt;
			 } else {
				lxt = xt;
				lyt = yt;
			}
		} else { /* group lca or subwidget configured */
			XUnionRegion(wr, lr = XCreateRegion(), lr);

			lxt = xt;
			lyt = yt;

			if ((lca_subw_conf = (w != confw))) { 
				confw = w;
				confd = depth;
			}
		}

		save = lr;

		dsp = DropSiteGroupPtrDropSites(*groups);

		for (j = 0; j < DropSiteGroupPtrCount(*groups);
		     j++, (dsp = OlDnDDropSitePtrNextInGroup(dsp))) {
			if (!lca_subw_conf ||
			    _CheckAncestorsToDepth(
					OlDnDDropSitePtrOwner(dsp),
					OlDnDDropSitePtrOwnerDepth(dsp),
					confw, confd
			    )) {
				lr   = save;
				save = XCreateRegion();

				XUnionRegion(lr, save, save);

				_ClipDropSite(dsp,
					      confw,
					      confd,
					      lr, lxt, lyt, 
					      (Boolean)group_visible);

				XDestroyRegion(lr);
			}
		}

		XDestroyRegion(save);
	}

	XDestroyRegion(wr);
}

/*****
 *
 * WARNING: this clipping code does *NOT* account for stacking order
 *	    issues.
 *****/

static void
DnDVCXClipDropSites (Widget vendor, OlDnDVendorPartExtension dnd_part, Widget widget)
{
	DropSiteGroupPtr		stack_cache[50], *arry = stack_cache;
	register DropSiteGroupPtr	grp, *p = arry;
	unsigned int			n,
					depth;
	

	FetchPartExtensionIfNull(vendor, dnd_part);


	if (dnd_part->disable_ds_clipping) {
		_AddWidgetToConfigurePending(dnd_part, widget);
		return;
	}

	depth = _CalculateWidgetDepth(widget);

	if (dnd_part->num_groups > XtNumber(stack_cache))
		p = arry = (DropSiteGroupPtr *)XtCalloc(dnd_part->num_groups,
							sizeof(DropSiteGroupPtr));

	for ((n = dnd_part->num_groups), (grp = dnd_part->drop_site_groups);
	     n > 0; n--, grp++) {
			Widget		ancestor = widget,
					child    = DropSiteGroupPtrLCA(grp);

			int		ad = (int)depth,
					cd = (int)DropSiteGroupPtrLCADepth(grp);

	    if (ad > cd && (ad - cd) <= GROUP_DEPTH) {
		ancestor = child;	/* lca */
		ad       = cd;
		child    = widget;
		cd       = depth;
	    }

	    if (_CheckAncestorsToDepth(child, cd, ancestor, ad)) *p++ = grp;
	}

	if (p != arry)
		_ClipDropSiteGroups(vendor, widget, depth, arry, p - arry);

	if (arry != stack_cache) XtFree((char *)arry);

	_DoPendingConfigures(vendor, dnd_part);

	dnd_part->dirty = True;

	if (dnd_part->auto_assert_dropsite_registry &&
	    dnd_part->class_extension->assert_drop_site_registry !=
	    (OlDnDVCXAssertRegistryProc)NULL) {
		(*dnd_part->class_extension->assert_drop_site_registry)
			(vendor, dnd_part);
	}

}

