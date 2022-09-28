#ifndef _XOL_VENDORI_H
#define	_XOL_VENDORI_H

#pragma	ident	"@(#)VendorI.h	302.8	92/10/09 include/Xol SMI"	/* mouseless:VendorI.h 1.11	*/

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
 ************************************************************************
 * Description:
 *	This is the OLIT internal private header file for
 * the vendor shell.  It has information associated with the class
 * extensions.
 *
 * Why a class extension?  Because if we don't use an extension, we
 * have to add fields to the VendorShellPartRec which means we must
 * recompile the Intrinsics library and therefore, our toolkit will
 * no longer be able to run on standard Intrinsics.
 *
 * Here's a summary of the extension scheme employed:
 * 	Each shell that is a subclass of a VendorShell has an extension
 * placed in their respective VendorShellClass's extension list.
 * Subclasses that wish to provide different default resource values
 * should explicitly place an extension on the list during their
 * static initialization or in their ClassInitialization.  (Note, a
 * subclass cannot do this in its ClassPartInitialization.)
 * If the subclass doesn't provide an extension, the VendorShell's
 * ClassPartInitialization procedure will create one.
 *	Subclasses that add an extension by static initialization must
 * set the 'record_type' field in the extension header to
 * the global XrmQuark OlXrmVendorClassExtension in their
 * ClassInitialization procedure.  This field cannot be set statically
 * since the VendorShell's ClassInitialization procedure does the
 * quarking of the string.
 *	Within the extension record is a pointer to the list of instance
 * data associated with each vendor widget instance.  The routine
 * _OlGetVendorPartExtension should be called by subclasses that want
 * to read the fields that are in the instance data.  NOTE: for
 * widgets that want to access the OlFocusData, they should use
 * _OlGetFocusData directly.
 ************************************************************************
 */


#include <Xol/OpenLookP.h>
#include <Xol/array.h>		/* for traversal list */

#include <X11/ShellP.h>		/* 
				 * Must include because X11/VendorP.h 
				 * doesn't
				 */
#include <X11/VendorP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 ************************************************************************
 * Declare some externals
 ************************************************************************
 */

typedef struct {
	Widget		focus_gadget;		/* monitoring gadgets	*/
	Widget		initial_focus_widget;	/* initial widget to get focus*/
	Widget		current_focus_widget;	/* current widget w/focus */
	Widget		activate_on_focus;	/* activate when focus arrives*/
	Widget		shell;			/* obsolete		*/
	_OlArrayRec	traversal_list;
	OlDefine	focus_model;		/* current focus model	*/
	Boolean		resort_list;		/* re-sort traversal list */
} OlFocusData;

typedef struct _OlVendorPartExtensionRec {
	struct _OlVendorPartExtensionRec *next;/* next instance node	*/
	struct _OlVendorClassExtensionRec *
				class_extension; /* pointer to class ext.*/	
	Widget			vendor;		/* vendor widget's id */
	XtPointer		user_data;	/* Application data hook*/
	Boolean			busy;		/* busy window? */
	Boolean			menu_button;	/* menu button? */
	Boolean			resize_corners;	/* has resize corners? */
	Boolean			window_header;	/* has window header? */
	Boolean			accelerators_do_grab;
	unsigned char		dyn_flags;	/* dynamic resource flags */
	XtCallbackList		consume_event;	/* consumeEvent callback */
	XtCallbackList		wm_protocol;	/* WM protocol callback */
	OlDefine		menu_type;	/* WM menu type */
	OlDefine		pushpin;	/* pushpin state */
	OlDefine		win_type;	/* window type (private) */
	Widget			default_widget;	/* shell's default widget */
	OlFocusData		focus_data;	/* focus management */
	struct OlAcceleratorList *		/* list of accelerators */
				accelerator_list; /* and mnemonics */
	Boolean			olwm_running;	/* is our wm there? */
	Boolean			compose_led;	/* for compose */
	unsigned long		wm_protocol_mask;/* WM protocol mask */
	Boolean			from_setvalues; 	/* to indicate from set values */
	Boolean			footer_present; 	/* app footer present or ? */
	Boolean			left_footer_visible;
	Boolean			right_footer_visible; 	
	Widget			footer; 	/* Widget id of the footer container - Form */
	Widget			left_footer; 	/* left footer - StaticText */
	Widget			right_footer; 	/* right footer - StaticText */
	OlStr 			left_footer_string;
	OlStr			right_footer_string;
	OlStrRep		text_format; /* TextFormat - single, multi or wide-char*/
	OlFont			im_font_set; /* I18N footer's font */
	Boolean			im_footer_present; 	/* I18N footer present ?*/
	OlImStatusStyle im_status_style; /* Preferred IM Status Styles */
	String			default_im_name; /* String to identify IM Server */
	Pixel                   foreground; /* needed for XNforeground IC value */
	XRectangle *	im_rect; 	/* Geometry info of I18N footer */
	OlStr		title;   /* for shell title */

	Widget		supercaret;
} OlVendorPartExtensionRec, *OlVendorPartExtension;

	
/*
 ************************************************************************
 * Declare some new types and inheritance tokens
 ************************************************************************
 */

typedef void	(*OlSetDefaultProc)(
    Widget	shell,		/* shell widget's id */
    Widget	new_default	/* new default widget */
);
#define XtInheritSetDefault		((OlSetDefaultProc)_XtInherit)

typedef void	(*OlGetDefaultProc)(
    Widget	shell,		/* shell widget's id */
    Widget	the_default	/* current default */
);
#define XtInheritGetDefault		((OlGetDefaultProc)_XtInherit)

typedef struct _OlVendorClassExtensionRec {
	OlClassExtensionRec	header;		/* required header	*/
	XtResourceList		resources;	/* extension resources	*/
	Cardinal		num_resources;	/* number of resources	*/
	XtResourceList		c_private;	/* private resources	*/
	OlSetDefaultProc	set_default;	/* set shell's default	*/
	OlGetDefaultProc	get_default;	/* ping subclass	*/
	OlExtDestroyProc	destroy;	/* extension destroy	*/
	OlExtInitializeProc	initialize;	/* extension initialize */
	OlExtSetValuesFunc	set_values;	/* extension setvalues  */
	OlExtGetValuesProc	get_values;	/* extension getvalues  */
	OlTraversalFunc		traversal_handler;
	OlHighlightProc		highlight_handler;
	OlActivateFunc		activate;
	OlEventHandlerList	event_procs;
	Cardinal		num_event_procs;
	OlVendorPartExtension	part_list;	/* instance data list	*/
	_OlDynData		dyn_data;	/* dyn_data		*/
	OlTransparentProc	transparent_proc;
	OlWMProtocolProc	wm_proc;	/* wm msg handler	*/
	Boolean			override_callback; 
} OlVendorClassExtensionRec, *OlVendorClassExtension;

	/* Define a version field number that reflects the version of
	 * this release's vendor shell extension.  Newer versions
	 * should distinguish themselves from older ones by using a
	 * different version number.					*/

#define OlVendorClassExtensionVersion	1L

#define OL_B_VENDOR_BG	(1 << 0)
#define OL_B_VENDOR_BORDERCOLOR	(1 << 1)


/*
 ************************************************************************
 * Declare some externals
 ************************************************************************
 */

extern XrmQuark	OlXrmVendorClassExtension;


/*
 * function prototype section
 */

extern void		_OlCopyParentsVisual(Widget widget, int closure, 
	XrmValue* value);

extern void		_OlFixResourceList(WidgetClass wc);

extern OlFocusData*	_OlGetFocusData(
	Widget			widget,		/* start search here */
	OlVendorPartExtension*	part_ptr	/* pointer or NULL */
);

extern OlVendorClassExtension	_OlGetVendorClassExtension(
	WidgetClass	vendor_class	/* subclass */
);

extern OlVendorPartExtension	_OlGetVendorPartExtension(
	Widget		vendor		/* subclass instance */
);

extern void		_OlSetPinState(
	Widget		widget,		/* widget in question */
	OlDefine	pinstate	/* new pin state */
);

extern void		_OlSetupColormap(Widget widget, int closure,
	XrmValue* value);

extern void		_OlVendorSetSuperCaretFocus(Widget vendor,
						    Widget target
			);
#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_VENDORI_H */
