#ifndef	_XOL_OLDNDVCXP_H
#define	_XOL_OLDNDVCXP_H

#pragma	ident	"@(#)OlDnDVCXP.h	302.1	92/03/26 include/Xol SMI"	/* OLIT	*/

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


#include <Xol/OpenLookP.h>
#include <Xol/OlDnDVCXI.h>

#include <X11/ShellP.h> 
#include <X11/VendorP.h>


#ifdef	__cplusplus
extern "C" {
#endif


#define GET_DND_EXT(wc) (OlDnDVendorClassExtension) _OlGetClassExtension( \
                                (OlClassExtension)VCLASS(wc, extension), \
                                OlXrmDnDVendorClassExtension, \
                                OlDnDVendorClassExtensionVersion)


/*************************** forward decls for Drag and Drop **************/

typedef	struct _OlDnDVendorPartExtensionRec*	OlDnDVendorPartExtension;
typedef struct _OlDnDVendorClassExtensionRec*	OlDnDVendorClassExtension;

typedef enum {
	CallDnDVCXInitialize,
	CallDnDVCXDestroy,
	CallDnDVCXSetValues,
	CallDnDVCXGetValues
}			DnDVCXMethodType;


/********************************************
 *
 * OLDnDVendorPartExtensionRec
 *
 ********************************************/

typedef	struct _OlDnDVendorPartExtensionRec {
	OlDnDVendorPartExtension	next_part;
	Widget				owner;
	OlDnDVendorClassExtension	class_extension;

	Time			registry_update_timestamp;
	Time			dsdm_last_loaded;

	unsigned int		number_of_sites;
	OlDnDDropSitePtr	drop_site_list;

	DropSiteGroupPtr	drop_site_groups;
	unsigned int		num_groups;

	Boolean			disable_ds_clipping;
	Widget			*pending_configures;
	unsigned int		num_pending;
	unsigned int		set_disable_sema;
	Widget			configuring_widget;
	int			doing_clipping_already;

	Position		root_x;
	Position		root_y;

	Boolean			do_dsdm_fetches_async;

	Boolean			auto_assert_dropsite_registry;
	Boolean			dirty;

	Boolean			doing_drag;

	Boolean			dsdm_present;

	OlDnDDropSitePtr	default_drop_site;

	unsigned long		dnd_tx_timeout;

	InternalDSRPtr		dropsite_rects;		/* private */
	InternalDSRPtr		current_dsr;		/* private */

	OlDnDDropSitePtr	current_dsp;		/* private */

	InternalDSDMSRPtr	*dsdm_rects;		/* private */
	InternalDSDMSRPtr	current_dsdmsr;		/* private */
	InternalDSDMSRPtr	internal_dsdm_sr_list;	/* private */
	DSDMSiteRectPtr		dsdm_sr_list;		/* private */
	unsigned long		num_dsdmsrs;		/* private */
	Boolean			pending_dsdm_info;	/* private */

	TransientAtomListPtr	transient_atoms;	/* private */
	DSSelectionAtomPtr	selection_atoms;	/* private */
} OlDnDVendorPartExtensionRec /*, *OlDnDVendorPartExtension*/;


/********************************************
 *
 * OLDnDVendorClassExtensionRec
 *
 ********************************************/

/*
 *
 * Convention: where a class extension method expects a class extension
 *	       pointer, if this is set to NULL then the method must
 *	       do a lookup for the extension. This is primarily used for
 *	       performance improvements where the caller already has the
 *	       necessary info .... and therefore passes in the extension
 *	       pointer reducing the lookup cost.
 *
 */

typedef void 		(*OlDnDVCXClassInitializeProc)(
	OlDnDVendorClassExtension);

typedef void 		(*OlDnDVCXClassPartInitializeProc)(WidgetClass);

typedef Boolean		(*OlDnDVCXSetValuesFunc)(Widget, Widget, Widget,
	ArgList, Cardinal*, OlDnDVendorPartExtension, OlDnDVendorPartExtension, 
	OlDnDVendorPartExtension);

typedef	void		(*OlDnDVCXGetValuesProc)(Widget, ArgList, Cardinal*,
 	OlDnDVendorPartExtension);

typedef	void		(*OlDnDVCXInitializeProc)(Widget, Widget, ArgList,
	Cardinal*, OlDnDVendorPartExtension, OlDnDVendorPartExtension);

typedef	void		(*OlDnDVCXPostRealizeSetupProc)(Widget,
	OlDnDVendorClassExtension, OlDnDVendorPartExtension);

typedef	void		(*OlDnDVCXDestroyProc)(Widget, OlDnDVendorPartExtension);

typedef	Boolean		(*OlDnDVCXTriggerMessageDispatcherFunc)(Widget, 
	OlDnDVendorPartExtension, TriggerMessagePtr);

typedef	OlDnDVCXTriggerMessageDispatcherFunc OlDnDVCXTMDispatcherFunc;

typedef	Boolean 	(*OlDnDVCXPreviewMessageDispatcherFunc)(Widget, 
	OlDnDVendorPartExtension, PreviewMessagePtr);

typedef	OlDnDVCXPreviewMessageDispatcherFunc OlDnDVCXPMDispatcherFunc;

typedef	OlDnDDropSiteID	(*OlDnDVCXRegisterDSFunc)(Widget, 
	OlDnDVendorPartExtension, Widget, Window, OlDnDSitePreviewHints,
	OlDnDSiteRectPtr, unsigned int,  OlDnDTriggerMessageNotifyProc,
	OlDnDPreviewMessageNotifyProc, Boolean, XtPointer);

typedef Boolean		(*OlDnDVCXUpdateDSGeometryProc)(Widget, 
	OlDnDVendorPartExtension, OlDnDDropSiteID, OlDnDSiteRectPtr,
	unsigned int);

typedef void		(*OlDnDVCXDeleteDSProc)(Widget,
	OlDnDVendorPartExtension, OlDnDDropSiteID);

typedef Boolean		(*OlDnDVCXQueryDSInfoFunc)(Widget, 
	OlDnDVendorPartExtension, OlDnDDropSiteID, Widget*, Window*, 
	OlDnDSitePreviewHints*, OlDnDSiteRectPtr*, unsigned int*, Boolean*);

typedef	void		(*OlDnDVCXAssertRegistryProc)(Widget,
	OlDnDVendorPartExtension);

typedef	void		(*OlDnDVCXDeleteRegistryProc)(Widget,
	OlDnDVendorPartExtension);

typedef	Boolean		(*OlDnDVCXFetchDSDMInfoFunc)(Widget, 
	OlDnDVendorPartExtension,  Time);

typedef	Boolean		(*OlDnDVCXDeliverTriggerMessageFunc)(Widget,
	OlDnDVendorPartExtension, Widget, Window, int, int, Atom,
	OlDnDTriggerOperation, Time);

typedef	OlDnDVCXDeliverTriggerMessageFunc	OlDnDVCXDeliverTMFunc;

typedef	Boolean		(*OlDnDVCXDeliverPreviewMessageFunc)(Widget,
	OlDnDVendorPartExtension, Widget, Window, int, int, Time,
	OlDnDPreviewAnimateCallbackProc, XtPointer);

typedef	OlDnDVCXDeliverPreviewMessageFunc	OlDnDVCXDeliverPMFunc;

typedef	Boolean		(*OlDnDVCXInitializeDragStateFunc)(Widget, 
	OlDnDVendorPartExtension);

typedef	void		(*OlDnDVCXClearDragStateProc)(Widget,
	OlDnDVendorPartExtension);

typedef Atom		(*OlDnDVCXAllocTransientAtomFunc)(Widget,
	OlDnDVendorPartExtension, Widget);

typedef void		(*OlDnDVCXFreeTransientAtomProc)(Widget,
	OlDnDVendorPartExtension, Widget, Atom);

typedef DSSelectionAtomPtr (*OlDnDVCXAssocSelectionFunc)(Widget,
	OlDnDVendorPartExtension, Widget, Atom, Time, OwnerProcClosurePtr);

typedef void		(*OlDnDVCXDisassocSelectionProc)(Widget, 
	OlDnDVendorPartExtension, Widget, Atom, Time);

typedef Atom*		(*OlDnDVCXGetCurrentSelectionsFunc)(Widget,
	OlDnDVendorPartExtension, Widget, Cardinal*);

typedef Widget		(*OlDnDVCXGetSelectionFunc)(Widget, 
	OlDnDVendorPartExtension, Atom, OwnerProcClosurePtr*);

typedef	Boolean		(*OlDnDVCXChangeSitePreviewHintsFunc)(Widget, 
	OlDnDVendorPartExtension, OlDnDDropSiteID, OlDnDSitePreviewHints);

typedef	Boolean		(*OlDnDVCXSetDSOnInterestFunc)(Widget, 
	OlDnDVendorPartExtension, OlDnDDropSiteID, Boolean, Boolean);

typedef	void		(*OlDnDVCXSetInterestWidgetHierFunc)(Widget,
	OlDnDVendorPartExtension, Widget, Boolean);

typedef	void		(*OlDnDVCXClipDropSitesProc)(Widget,
	OlDnDVendorPartExtension, 
	Widget);


/* inheritance tokens */

#define XtInheritOlDnDVCXPostRealizeSetupProc \
			((OlDnDVCXPostRealizeSetupProc)_XtInherit)
			
#define XtInheritOlDnDVCXRegisterDSFunc \
			((OlDnDVCXRegisterDSFunc)_XtInherit)
			
#define XtInheritOlDnDVCXUpdateDSGeometryProc \
			((OlDnDVCXUpdateDSGeometryProc)_XtInherit)
			
#define XtInheritOlDnDVCXDeleteDSProc \
			((OlDnDVCXDeleteDSProc)_XtInherit)
			
#define XtInheritOlDnDVCXQueryDSInfoFunc \
			((OlDnDVCXQueryDSInfoFunc)_XtInherit)
			
#define XtInheritOlDnDVCXAssertRegistryProc \
			((OlDnDVCXAssertRegistryProc)_XtInherit)
			
#define XtInheritOlDnDVCXDeleteRegistryProc \
			((OlDnDVCXDeleteRegistryProc)_XtInherit)
			
#define XtInheritOlDnDVCXFetchDSDMInfoFunc \
			((OlDnDVCXFetchDSDMInfoFunc)_XtInherit)
			
#define XtInheritOlDnDVCXTMDispatcherFunc \
			((OlDnDVCXTMDispatcherFunc)_XtInherit)
			
#define XtInheritOlDnDVCXPMDispatcherFunc \
			((OlDnDVCXPMDispatcherFunc)_XtInherit)
			
#define XtInheritOlDnDVCXDeliverTMFunc \
			((OlDnDVCXDeliverTMFunc)_XtInherit)
			
#define XtInheritOlDnDVCXDeliverPMFunc \
			((OlDnDVCXDeliverPMFunc)_XtInherit)
			
#define XtInheritOlDnDVCXInitializeDragStateFunc \
			((OlDnDVCXInitializeDragStateFunc)_XtInherit)
			
#define XtInheritOlDnDVCXClearDragStateProc \
			((OlDnDVCXClearDragStateProc)_XtInherit)
			
#define XtInheritOlDnDVCXAllocTransientAtomFunc \
			((OlDnDVCXAllocTransientAtomFunc)_XtInherit)
			
#define XtInheritOlDnDVCXFreeTransientAtomProc \
			((OlDnDVCXFreeTransientAtomProc)_XtInherit)
			
#define XtInheritOlDnDVCXAssocSelectionFunc \
			((OlDnDVCXAssocSelectionFunc)_XtInherit)
			
#define XtInheritOlDnDVCXDisassocSelectionProc \
			((OlDnDVCXDisassocSelectionProc)_XtInherit)
			
#define XtInheritOlDnDVCXGetCurrentSelectionsFunc \
			((OlDnDVCXGetCurrentSelectionsFunc)_XtInherit)
			
#define XtInheritOlDnDVCXGetSelectionFunc \
			((OlDnDVCXGetSelectionFunc)_XtInherit)
			
#define XtInheritOlDnDVCXChangeSitePreviewHintsFunc \
			((OlDnDVCXChangeSitePreviewHintsFunc)_XtInherit)
			
#define XtInheritOlDnDVCXSetDSOnInterestFunc \
			((OlDnDVCXSetDSOnInterestFunc)_XtInherit)
			
#define XtInheritOlDnDVCXSetInterestWidgetHierFunc \
			((OlDnDVCXSetInterestWidgetHierFunc)_XtInherit)
			
#define XtInheritOlDnDVCXClipDropSitesProc \
			((OlDnDVCXClipDropSitesProc)_XtInherit)


/*
 * the class extension itself.
 */

typedef	struct _OlDnDVendorClassExtensionRec {
	OlClassExtensionRec		header;

	Cardinal			instance_part_size;
	OlDnDVendorPartExtension	instance_part_list;
	XtResourceList			resources;
	Cardinal			num_resources;

	OlDnDVCXClassInitializeProc	class_initialize;
	OlDnDVCXClassPartInitializeProc	class_part_initialize;
	XtEnum				class_inited;

	OlDnDVCXSetValuesFunc		set_values;
	OlDnDVCXGetValuesProc		get_values;
	OlDnDVCXInitializeProc		initialize;
	OlDnDVCXPostRealizeSetupProc	post_realize_setup;
	OlDnDVCXDestroyProc		destroy;

	OlDnDVCXTMDispatcherFunc	trigger_message_dispatcher;
	OlDnDVCXPMDispatcherFunc	preview_message_dispatcher;

	OlDnDVCXRegisterDSFunc		register_drop_site;
	OlDnDVCXUpdateDSGeometryProc	update_drop_site_geometry;
	OlDnDVCXDeleteDSProc		delete_drop_site;
	OlDnDVCXQueryDSInfoFunc		query_drop_site_info;

	OlDnDVCXAssertRegistryProc	assert_drop_site_registry;
	OlDnDVCXDeleteRegistryProc	delete_drop_site_registry;

	OlDnDVCXFetchDSDMInfoFunc	fetch_dsdm_info;

	OlDnDVCXDeliverTMFunc		deliver_trigger_message;
	OlDnDVCXDeliverPMFunc		deliver_preview_message;

	OlDnDVCXInitializeDragStateFunc	initialize_drag_state;
	OlDnDVCXClearDragStateProc	clear_drag_state;

	OlDnDVCXAllocTransientAtomFunc	alloc_transient_atom;
	OlDnDVCXFreeTransientAtomProc	free_transient_atom;

	OlDnDVCXAssocSelectionFunc		associate_selection_and_w;
	OlDnDVCXDisassocSelectionProc 		disassociate_selection_and_w;
	OlDnDVCXGetCurrentSelectionsFunc	get_w_current_selections;
	OlDnDVCXGetSelectionFunc		get_w_for_selection;

	OlDnDVCXChangeSitePreviewHintsFunc	change_site_hints;
	OlDnDVCXSetDSOnInterestFunc		set_ds_on_interest;
	OlDnDVCXSetInterestWidgetHierFunc	set_interest_in_widget_hier;

	OlDnDVCXClipDropSitesProc		clip_drop_sites;
} OlDnDVendorClassExtensionRec;

extern OlDnDVendorClassExtensionRec dnd_vendor_extension_rec;
extern OlDnDVendorClassExtension    dnd_vendor_extension;

extern	XrmQuark	OlXrmDnDVendorClassExtension;

#define	OlDnDVendorClassExtensionVersion	1L
#define	OlDnDVendorClassExtensionName		"OlDnDVendorClassExtension"


extern Boolean		CallDnDVCXExtensionMethods(
	DnDVCXMethodType extension_method_to_call, WidgetClass wc,
	Widget current, Widget request, Widget c_new, ArgList args,
	Cardinal* num_args);

extern Widget		GetShellParentOfWindow(Display* dpy, Window window);

extern void		_OlDnDCallVCXPostRealizeSetup(Widget vendor);

extern void		_OlDnDDoExtensionClassInit(
	OlDnDVendorClassExtension extension);

extern void		_OlDnDDoExtensionClassPartInit(WidgetClass wc);
extern void		_OlDnDSetDisableDSClipping(Widget widget, Boolean value);

extern OlDnDVendorClassExtension _OlGetDnDVendorClassExtension(WidgetClass wc);
extern OlDnDVendorPartExtension _OlGetDnDVendorPartExtension(Widget w);


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_OLDNDVCXP_H */
