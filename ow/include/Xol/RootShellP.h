#ifndef	_XOL_ROOTSHELLP_H
#define	_XOL_ROOTSHELLP_H

#pragma	ident	"@(#)RootShellP.h	302.13	94/01/14 include/Xol SMI"	/* OLIT	*/

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


#include <Xol/OpenLookI.h>
#include <Xol/RootShell.h>

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <Xol/OlImP.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct	_RootShellRec	*RootShellWidget;

typedef void		(*RSAddShellProc)(RootShellWidget, Widget);
typedef void		(*RSDeleteShellProc)(RootShellWidget, Widget);
typedef Widget		(*RSShellRegisteredProc)(RootShellWidget, Widget);

typedef void		(*RSAddRootProc)(RootShellWidget,
	RootShellWidgetClass);
	
typedef void		(*RSDeleteRootProc)(RootShellWidget, 
	RootShellWidgetClass);

typedef Widget		(*RSRootRegisteredProc)(WidgetClass, Screen*);
typedef void		(*RSPropChangedProc)(Widget);

typedef	XtEventHandler	RSPropertyNotifyProc;


extern OlDefine		OlGetBeep(Widget w);
extern int		OlGetBeepDuration(Widget w);
extern int		OlGetBeepVolume(Widget w);
extern Boolean		OlOKToGrabPointer(Widget w);
extern Boolean		OlOKToGrabServer(Widget w);

extern void		_OlAddShellToDisplayList(Widget shell);

extern void		_OlAddShellToRootList(Widget shell,
	WidgetClass subclass);

extern void		_OlAddShellToScreenList(Widget shell);

extern void		_OlDeleteShellFromDisplayList(Widget shell);

extern void		_OlDeleteShellFromRootList(Widget shell,
	WidgetClass subclass);

extern void		_OlDeleteShellFromScreenList(Widget shell);

extern void		_OlGetListOfScreenShells(Widget* * list,
	Cardinal* num);

extern void		_OlGetRootShellApplRes(RootShellWidget rsw,
	ArgList args, Cardinal num_args);

extern int		_OlGetScale(Screen* screen);
extern char*		_OlGetScaleMap(Screen* screen);

extern Widget		_OlRootShellOfScreen(Screen* screen,
	WidgetClass subclass);

extern void		_SetDefaultAppCon(XtAppContext app_con);
extern String           _OlGetOlDefaultFont(Screen* screen);

extern ImVSInfo *       _OlGetImVSInfo(Widget w);

#define	XtInheritRSAddShell		((RSAddShellProc)_XtInherit)
#define	XtInheritRSDeleteShell		((RSDeleteShellProc)_XtInherit)
#define	XtInheritRSShellRegistered	((RSShellRegisteredProc)_XtInherit)
#define	XtInheritRSAddRoot		((RSAddRootProc)_XtInherit)
#define	XtInheritRSDeleteRoot		((RSDeleteRootProc)_XtInherit)
#define	XtInheritRSRootRegistered	((RSRootRegisteredProc)_XtInherit)
#define	XtInheritRSPropertyNotify	((RSPropertyNotifyProc)_XtInherit)

/***********************************************************************
 *
 * RootShell Widget Private Data
 *
 ***********************************************************************/

#define	VECTOR_INCR	10
#define	VECTORFULL(n, incr)	((n) == (((n) / (incr)) * (incr)))

/* New fields for the RootShell widget class record */

typedef struct {
    XtResourceList		appl_res;
    Cardinal			num_appl_res;

    RSAddShellProc		add_shell;
    RSDeleteShellProc		delete_shell;
    RSShellRegisteredProc   	shell_registered;

    RSAddRootProc		add_root;
    RSDeleteRootProc		delete_root;
    RSRootRegisteredProc	root_registered;

    RSPropertyNotifyProc	prop_notify;		
    RSPropChangedProc		prop_changed;
    
    Widget	    		*root_shells;	/* instance list */
    Cardinal	    		num_roots;	/* num instances */

    XtPointer       		extension;      /* extension */
} RootShellClassPart;

typedef struct _RootShellClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ShellClassPart		shell_class;
	OverrideShellClassPart	override_shell_class;
	RootShellClassPart	root_shell;
}			RootShellClassRec;

externalref RootShellClassRec rootShellClassRec;

typedef struct {
	Widget			appl_shell;
	Widget*			shells;			/* list of shells */
	Cardinal		num_shells;		/* number of shells */
	XtCallbackList		prop_notify_callback;
	Atom			root_property;		/* atom of monitored
							   prop */
	String			property_string;	/* value of property */
	Time			last_update;		/* last update of prop */
	Widget			vendor_instance;	/* vendor shell that caused it to be created */
	XtPointer		user_data;		/* hook for user */
}			RootShellPart;

typedef struct _RootShellRec {
	CorePart		core;
	CompositePart		composite;
	ShellPart		shell;
	OverrideShellPart	override;
	RootShellPart		root;
}			RootShellRec;

/***********************************************************************
 *
 * DisplayShell Widget Private Data
 *
 ***********************************************************************/

typedef struct {
	XtPointer		extension;
} DisplayShellClassPart;

typedef struct _DisplayShellClassRec {
  	CoreClassPart      	core_class;
	CompositeClassPart 	composite_class;
	ShellClassPart  	shell_class;
	OverrideShellClassPart  override_shell_class;
	RootShellClassPart	root_shell;
	DisplayShellClassPart	display_shell;
} DisplayShellClassRec;

externalref	DisplayShellClassRec	displayShellClassRec;

typedef	struct  _DisplayShellRec {

        /* for supercaret */
 
        Boolean         shape_extension_present;
        int             shape_event_base;
        int             shape_error_base;
 
	/* per display application attributes */
	Cardinal	multi_click_timeout;	/* in milliseconds */
	int		beep_duration;		/* in msec */
	int		beep_volume;		/* Beep volumn percentage */
	OlDefine	beep;			/* Beep for which levels */
	Boolean		select_does_preview;	/* Does select preview? */
	Boolean		grab_pointer;		/* can we grab the pointer */
	Boolean		grab_server;		/* can we grab the server */
	Boolean		menu_accelerators;	/* accelerators operation on? */
	Boolean		mouseless;		/* mouseless operation on? */
	OlDefine	input_focus_feedback;	/* color or supercaret */
	Cardinal	multi_object_count;
	OlBitMask	dont_care;		/*
						 * should use Modifiers,
						 * but can't find a
						 * repesentation type
						 */

	/*
	 * Resources that control accelerators
	 * and mnemonics:
	 */

	Modifiers	mnemonic_modifiers;
	OlDefine	show_mnemonics;
	OlDefine	show_accelerators;
	String		shift_name;
	String		lock_name;
	String		control_name;
	String		mod1_name;
	String		mod2_name;
	String		mod3_name;
	String		mod4_name;
	String		mod5_name;

	OlDefine	help_model;
	Boolean		mouse_status;
	Cardinal	key_remap_timeout;	/* in seconds */
	String      ol_default_font;
	Boolean		ctrl_alt_meta_key;
	Boolean		use_short_OlWinAttr;

	/*
	 * list of im related attributes per vendor shell on this display
	 */
	ImVSInfoList	im_vs_info_list;
	/* per display drag state information */
	Boolean		doing_drag;

} DisplayShellPart;

typedef struct {
	CorePart		core;
	CompositePart		composite;
	ShellPart		shell;
	OverrideShellPart	override;
	RootShellPart		root;
	DisplayShellPart	display;
}			DisplayShellRec, *DisplayShellWidget;

/***********************************************************************
 *
 * ScreenShell Widget Private Data
 *
 ***********************************************************************/

#define	XtInheritRSPropChangedProc	((RSPropChangedProc)_XtInherit)

typedef struct {
	RSPropChangedProc	rm_prop_changed;
	XtPointer		extension;
}			ScreenShellClassPart;

typedef struct _ScreenShellClassRec {
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ShellClassPart		shell_class;
	OverrideShellClassPart	override_shell_class;
	RootShellClassPart	root_shell;
	ScreenShellClassPart	screen_shell;
}			ScreenShellClassRec;

externalref	ScreenShellClassRec	screenShellClassRec;

typedef	struct {
	OlDynamicScreenCallback	proc;
	XtPointer		closure;
} OlDSCBRec, *OlDynamicScreenCallbacks;

typedef struct _ScreenShellRec {
	/* per screen application attributes */
	Cardinal		mouse_damping_factor;	/* in points */
	Boolean			three_d;		/* use 3-D visuals? */
	int			scale;
	char*			scale_map_file;		/* name of scale to
							   screen resolution map
							   file */
	Dimension		drag_right_distance;
	Dimension		menu_mark_region;
	#define	UpdatePrevAppAttrs(ssw)	\
	if ((ssw)->screen.prev_app_attrs != (_OlAppAttributes*)NULL) \
	      XtFree((char*)(ssw)->screen.prev_app_attrs); \
	(ssw)->screen.prev_app_attrs = (ssw)->screen.app_attrs;	\
	(ssw)->screen.app_attrs	    = (_OlAppAttributes*)NULL

	Boolean			app_attrs_need_update; /* dynamic db load
							  occurred */
	_OlAppAttributes*	prev_app_attrs;		/* last app attrs */
	_OlAppAttributes*	app_attrs;	/* only used if someone calls:
						   _OlGetAppAttributesRef() */
	OlDynamicScreenCallbacks dyn_cbs;
	Cardinal		num_dyn_cbs;
	String			ol_cursor_font_name;
	Font			ol_cursor_font_id;
	XFontStruct*		ol_cursor_font_data;
	Boolean			doing_dynamic_res_processing;
}			ScreenShellPart;

typedef struct {
	CorePart		core;
	CompositePart		composite;
	ShellPart		shell;
	OverrideShellPart	override;
	RootShellPart		root;
	ScreenShellPart		screen;
}			ScreenShellRec, *ScreenShellWidget;


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_ROOTSHELLP_H */
