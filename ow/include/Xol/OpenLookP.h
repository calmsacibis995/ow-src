#ifndef	_XOL_OPENLOOKP_H
#define	_XOL_OPENLOOKP_H

#pragma	ident	"@(#)OpenLookP.h	302.16	92/10/22 include/Xol SMI"	/* oltemporary:OpenLookP.h 1.42	*/

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

/**************************************************************************
 *
 * Description:
 * 		This is the include file for the OLIT
 *		routines which are private to the toolkit.
 * 
 **************************************************************************/


#include <Xol/OpenLook.h>

#include <X11/IntrinsicP.h>

#include <stdio.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*************************************************************************
 *	Define True Constant Tokens
 *		Tokens that appear in this section should not have
 *		their values changed.
 *************************************************************************/

/*
 *				OL_VERSION	OL_REVISION
 * OLIT 2.5			2		1
 * OpenWindow V3/OLIT 3.0	2		1		[Update omitted]
 * OpenWindows/OLIT 3.0.1	3		2
 */
#define OL_VERSION		3
#define OL_REVISION		2

#define OlVersion		(OL_VERSION * 1000 + OL_REVISION)

#define OL_DEFAULT_POINT_SIZE		12
#define OL_DEFAULT_FONT_NAME		Nlucida
#define OL_DEFAULT_BOLD_FONT_NAME	NlucidaBold
#define	OL_DEFAULT_FONTSET_NAME		"-*-*-*-R-normal--*-120-75-75-*-*-*-*"
#define	OL_DEFAULT_BOLD_FONTSET_NAME	"-*-*-bold-R-normal--*-120-75-75-*-*-*-*"
#define	OL_DEFAULT_TEXTFORMAT		OL_SB_STR_REP

#define OL_MAX_VIRTUAL_MAPPINGS	6

/* 
 * These values match the default values of analogous OpenWindows
 * Workspace Properties
 */
 
/* See OpenWindows.DragRightDistance */
#define OL_DRAG_RIGHT_DISTANCE		100		/* pixels */

/* OpenWindows.MultiClickTimeout */
#define OL_MULTI_CLICK_TIMEOUT		200		/* milliseconds */

/* OpenWindows.PopupJumpCursor, Scrollbar.JumpCursor */
#define OL_POINTER_WARPING		True

/* OpenWindows.WindowColor */
#define OL_XT_DEFAULT_BACKGROUND	"#cccccc"	/* X HEX 24-bit RGB */


/*************************************************************************
 *	Macros
 *************************************************************************/

#define _OlMax(x,y)		(((x) > (y)) ? (x) : (y))
#define _OlMin(x,y)		(((x) < (y)) ? (x) : (y))

#define _OlAssignMax(x,y)	if ((y) > (x)) x = (y)
#define _OlAssignMin(x,y)	if ((y) < (x)) x = (y)

/* String length macro	*/
#if	defined(__STDC__) || defined(__cplusplus)
#define _OlStrlen(string)	((string) ? strlen((const char*)(string)) : 0)
#else	/* __STDC__ || __cplusplus */
#define _OlStrlen(string)	((string) ? strlen((char*)(string)) : 0)
#endif	/* __STDC__ || __cplusplus */

/* geometry-related convenience macros */
#define _OlScreenWidth(w)	(WidthOfScreen(XtScreenOfObject(w)))
#define _OlScreenHeight(w)	(HeightOfScreen(XtScreenOfObject(w)))
#define _OlWidgetWidth(w)	((w)->core.width + 2 * (w)->core.border_width)
#define _OlWidgetHeight(w)	((w)->core.height + 2 * (w)->core.border_width)

#ifndef	XA_CLIPBOARD
#define XA_CLIPBOARD(d)		XInternAtom(d, "CLIPBOARD", False)
#endif

/* Define Mask Arg List structure and macro */
#if	defined(__STDC__) || defined(__cplusplus)
typedef struct {
	const char*	name;		/* resource name */
	XtArgVal	value;		/* resource value */
	int		rule;		/* comparision rule (see below) */
} MaskArg, *MaskArgList;
#else	/* __STDC__ || __cplusplus */
typedef struct {
	char*		name;
	XtArgVal	value;
	int		rule;
} MaskArg, *MaskArgList;
#endif	/* __STDC__ || __cplusplus */

/* I18N: Define ADTs for IM Handling */
typedef struct _InputMethodRec *  OlInputMethodID;
typedef struct _InputContextRec * OlInputContextID;

#define _OlSetMaskArg(m_arg, n, v, r) \
	((m_arg).name = (String)(n), (m_arg).value = (XtArgVal)(v), \
	(m_arg).rule = (int)(r))

/*
 * Convenience macro to point to desired field within one of the
 * fundamental OpenLook widget class records.
 */
#define _OlGetClassField(w, field, dest)	{ \
		WidgetClass	__ol_class = _OlClass(w); \
		\
		if (__ol_class == primitiveWidgetClass)	 \
			dest = \
		((PrimitiveWidgetClass)XtClass(w))->primitive_class.field; \
			else if (__ol_class == eventObjClass) \
		dest = ((EventObjClass)XtClass(w))->event_class.field; \
			else if (__ol_class == managerWidgetClass) \
	    	dest = ((ManagerWidgetClass)XtClass(w))->manager_class.field; \
	}

/* Obsolete MT entry points */
#define GetToken()
#define ReleaseToken()
#define InitializeToken()
#define RegisterEventThread(app)


/*************************************************************************
 *	Private strings
 *************************************************************************/

/* A cast cannot be used with this as it causes problems for xgettext */
#ifdef XGETTEXT /* for xgetext call */
#define OlMsgsDomain "SUNW_WST_LIBXOL"
#else
extern const char OlMsgsDomain[];
#endif

#if	defined(__STDC__) || defined(__cplusplus)
#define Nlucida (const char *)"-b&h-lucida-medium-r-*-*-*-*-75-75-*-*-iso8859-1"

#define OlDefaultBoldFont (const char *)"OlDefaultBoldFont"

#define BANG_NAME (const char *)"BANG"
#define DELETE_NAME (const char *)"DELETE"
#define LENGTH_NAME (const char *)"LENGTH"
#define OL_PASTE_MSG_NAME (const char *)"OL_PASTE_MSG"
#define TARGETS_NAME (const char *)"TARGETS"
#define TIMESTAMP_NAME (const char *)"TIMESTAMP"
#define COMPOUND_TEXT_NAME (const char *)"COMPOUND_TEXT"

#define WM_CHANGE_STATE_NAME (const char *)"WM_CHANGE_STATE"
#define WM_DECORATION_HINTS_NAME (const char *)"WM_DECORATION_HINTS"
#define WM_DELETE_WINDOW_NAME (const char *)"WM_DELETE_WINDOW"
#define WM_DISMISS_NAME (const char *)"WM_DISMISS"
#define WM_ICON_SIZE_NAME (const char *)"WM_ICON_SIZE"
#define WM_PROTOCOLS_NAME (const char *)"WM_PROTOCOLS"
#define WM_SAVE_YOURSELF_NAME (const char *)"WM_SAVE_YOURSELF"
#define WM_STATE_NAME (const char *)"WM_STATE"
#define WM_TAKE_FOCUS_NAME (const char *)"WM_TAKE_FOCUS"
#define WM_WINDOW_MOVED_NAME (const char *)"WM_WINDOW_MOVED"

#define _OL_COPY_NAME (const char *)"_OL_COPY"
#define _OL_CUT_NAME (const char *)"_OL_CUT"
#define _OL_DECOR_ADD_NAME (const char *)"_OL_DECOR_ADD"
#define _OL_DECOR_CLOSE_NAME (const char *)"_OL_DECOR_CLOSE"
#define _OL_DECOR_DEL_NAME (const char *)"_OL_DECOR_DEL"
#define _OL_DECOR_HEADER_NAME (const char *)"_OL_DECOR_HEADER"
#define _OL_DECOR_PIN_NAME (const char *)"_OL_DECOR_PIN"
#define _OL_DECOR_RESIZE_NAME (const char *)"_OL_DECOR_RESIZE"
#define _OL_HELP_KEY_NAME (const char *)"_OL_HELP_KEY"
#define _OL_MENU_FULL_NAME (const char *)"_OL_MENU_FULL"
#define _OL_MENU_LIMITED_NAME (const char *)"_OL_MENU_LIMITED"
#define _OL_MOVE_NAME (const char *)"_OL_MOVE"
#define _OL_NONE_NAME (const char *)"_OL_NONE"
#define _OL_PIN_STATE_NAME (const char *)"_OL_PIN_STATE"
#define _OL_WIN_ATTR_NAME (const char *)"_OL_WIN_ATTR"
#define _OL_WIN_BUSY_NAME (const char *)"_OL_WIN_BUSY"
#define _OL_WIN_COLORS_NAME (const char *)"_OL_WIN_COLORS"
#define _OL_WT_BASE_NAME (const char *)"_OL_WT_BASE"
#define _OL_WT_CMD_NAME (const char *)"_OL_WT_CMD"
#define _OL_WT_HELP_NAME (const char *)"_OL_WT_HELP"
#define _OL_WT_NOTICE_NAME (const char *)"_OL_WT_NOTICE"
#define _OL_WT_OTHER_NAME (const char *)"_OL_WT_OTHER"
#define _OL_WT_PROP_NAME (const char *)"_OL_WT_PROP"

#define _SUN_ALTERNATE_TRANSPORT_METHODS_NAME (const char *)"_SUN_ALTERNATE_TRANSPORT_METHODS"
#define _SUN_ATM_FILE_NAME_NAME (const char *)"_SUN_ATM_FILE_NAME"
#define _SUN_ATM_TOOL_TALK_NAME (const char *)"_SUN_ATM_TOOL_TALK"
#define _SUN_AVAILABLE_TYPES_NAME (const char *)"_SUN_AVAILABLE_TYPES"
#define _SUN_DATA_LABEL_NAME (const char *)"_SUN_DATA_LABEL"
#define _SUN_DRAGDROP_ACK_NAME (const char *)"_SUN_DRAGDROP_ACK"
#define _SUN_DRAGDROP_BEGIN_NAME (const char *)"_SUN_DRAGDROP_BEGIN"
#define _SUN_DRAGDROP_DONE_NAME (const char *)"_SUN_DRAGDROP_DONE"
#define _SUN_DRAGDROP_DSDM_NAME (const char *)"_SUN_DRAGDROP_DSDM"
#define _SUN_DRAGDROP_INTEREST_NAME (const char *)"_SUN_DRAGDROP_INTEREST"
#define _SUN_DRAGDROP_PREVIEW_NAME (const char *)"_SUN_DRAGDROP_PREVIEW"
#define _SUN_DRAGDROP_SITE_RECTS_NAME (const char *)"_SUN_DRAGDROP_SITE_RECTS"
#define _SUN_DRAGDROP_TRIGGER_NAME (const char *)"_SUN_DRAGDROP_TRIGGER"
#define _SUN_ENUMERATION_COUNT_NAME (const char *)"_SUN_ENUMERATION_COUNT"
#define _SUN_ENUMERATION_ITEM_NAME (const char *)"_SUN_ENUMERATION_ITEM"
#define _SUN_FILE_HOST_NAME_NAME (const char *)"_SUN_FILE_HOST_NAME"
#define _SUN_LENGTH_TYPE_NAME (const char *)"_SUN_LENGTH_TYPE"
#define _SUN_LOAD_NAME (const char *)"_SUN_LOAD"
#define _SUN_SELECTION_END_NAME (const char *)"_SUN_SELECTION_END"
#define _SUN_SELECTION_ERROR_NAME (const char *)"_SUN_SELECTION_ERROR"
#define _SUN_SELN_YIELD_NAME (const char *)"_SUN_SELN_YIELD"
#define _SUN_WINDOW_STATE_NAME (const char *)"_SUN_WINDOW_STATE"
#define _SUN_WM_PROTOCOLS_NAME (const char *)"_SUN_WM_PROTOCOLS"

#else	/* __STDC__ || __cplusplus */

#define Nlucida "-b&h-lucida-medium-r-*-*-*-*-75-75-*-*-iso8859-1"

#define OlDefaultBoldFont "OlDefaultBoldFont"

#define BANG_NAME "BANG"
#define DELETE_NAME "DELETE"
#define LENGTH_NAME "LENGTH"
#define OL_PASTE_MSG_NAME "OL_PASTE_MSG"
#define TARGETS_NAME "TARGETS"
#define TIMESTAMP_NAME "TIMESTAMP"
#define COMPOUND_TEXT_NAME "COMPOUND_TEXT"

#define WM_CHANGE_STATE_NAME "WM_CHANGE_STATE"
#define WM_DECORATION_HINTS_NAME "WM_DECORATION_HINTS"
#define WM_DELETE_WINDOW_NAME "WM_DELETE_WINDOW"
#define WM_DISMISS_NAME "WM_DISMISS"
#define WM_ICON_SIZE_NAME "WM_ICON_SIZE"
#define WM_PROTOCOLS_NAME "WM_PROTOCOLS"
#define WM_SAVE_YOURSELF_NAME "WM_SAVE_YOURSELF"
#define WM_STATE_NAME "WM_STATE"
#define WM_TAKE_FOCUS_NAME "WM_TAKE_FOCUS"
#define WM_WINDOW_MOVED_NAME "WM_WINDOW_MOVED"

#define _OL_COPY_NAME "_OL_COPY"
#define _OL_CUT_NAME "_OL_CUT"
#define _OL_DECOR_ADD_NAME "_OL_DECOR_ADD"
#define _OL_DECOR_CLOSE_NAME "_OL_DECOR_CLOSE"
#define _OL_DECOR_DEL_NAME "_OL_DECOR_DEL"
#define _OL_DECOR_HEADER_NAME "_OL_DECOR_HEADER"
#define _OL_DECOR_PIN_NAME "_OL_DECOR_PIN"
#define _OL_DECOR_RESIZE_NAME "_OL_DECOR_RESIZE"
#define _OL_HELP_KEY_NAME "_OL_HELP_KEY"
#define _OL_MENU_FULL_NAME "_OL_MENU_FULL"
#define _OL_MENU_LIMITED_NAME "_OL_MENU_LIMITED"
#define _OL_MOVE_NAME "_OL_MOVE"
#define _OL_NONE_NAME "_OL_NONE"
#define _OL_PIN_STATE_NAME "_OL_PIN_STATE"
#define _OL_WIN_ATTR_NAME "_OL_WIN_ATTR"
#define _OL_WIN_BUSY_NAME "_OL_WIN_BUSY"
#define _OL_WIN_COLORS_NAME "_OL_WIN_COLORS"
#define _OL_WT_BASE_NAME "_OL_WT_BASE"
#define _OL_WT_CMD_NAME "_OL_WT_CMD"
#define _OL_WT_HELP_NAME "_OL_WT_HELP"
#define _OL_WT_NOTICE_NAME "_OL_WT_NOTICE"
#define _OL_WT_OTHER_NAME "_OL_WT_OTHER"
#define _OL_WT_PROP_NAME "_OL_WT_PROP"

#define _SUN_ALTERNATE_TRANSPORT_METHODS_NAME "_SUN_ALTERNATE_TRANSPORT_METHODS"
#define _SUN_ATM_FILE_NAME_NAME "_SUN_ATM_FILE_NAME"
#define _SUN_ATM_TOOL_TALK_NAME "_SUN_ATM_TOOL_TALK"
#define _SUN_AVAILABLE_TYPES_NAME "_SUN_AVAILABLE_TYPES"
#define _SUN_DATA_LABEL_NAME "_SUN_DATA_LABEL"
#define _SUN_DRAGDROP_ACK_NAME "_SUN_DRAGDROP_ACK"
#define _SUN_DRAGDROP_BEGIN_NAME "_SUN_DRAGDROP_BEGIN"
#define _SUN_DRAGDROP_DONE_NAME "_SUN_DRAGDROP_DONE"
#define _SUN_DRAGDROP_DSDM_NAME "_SUN_DRAGDROP_DSDM"
#define _SUN_DRAGDROP_INTEREST_NAME "_SUN_DRAGDROP_INTEREST"
#define _SUN_DRAGDROP_PREVIEW_NAME "_SUN_DRAGDROP_PREVIEW"
#define _SUN_DRAGDROP_SITE_RECTS_NAME "_SUN_DRAGDROP_SITE_RECTS"
#define _SUN_DRAGDROP_TRIGGER_NAME "_SUN_DRAGDROP_TRIGGER"
#define _SUN_ENUMERATION_COUNT_NAME "_SUN_ENUMERATION_COUNT"
#define _SUN_ENUMERATION_ITEM_NAME "_SUN_ENUMERATION_ITEM"
#define _SUN_FILE_HOST_NAME_NAME "_SUN_FILE_HOST_NAME"
#define _SUN_LENGTH_TYPE_NAME "_SUN_LENGTH_TYPE"
#define _SUN_LOAD_NAME "_SUN_LOAD"
#define _SUN_SELECTION_END_NAME "_SUN_SELECTION_END"
#define _SUN_SELECTION_ERROR_NAME "_SUN_SELECTION_ERROR"
#define _SUN_SELN_YIELD_NAME "_SUN_SELN_YIELD"
#define _SUN_WINDOW_STATE_NAME "_SUN_WINDOW_STATE"
#define _SUN_WM_PROTOCOLS_NAME "_SUN_WM_PROTOCOLS"

#endif	/* __STDC__ || __cplusplus */


/*************************************************************************
 *	typedef's, enum's, struct's
 *************************************************************************/
#if	defined(__STDC__) || defined(__cplusplus)

	#define	XtInheritTransparentProc	((OlTransparentProc)_XtInherit)
	#define	XtInheritWMProtocolProc		((OlWMProtocolProc)_XtInherit)

        /* This is used for getting bitmap data into the read-only text
	   segment */
        typedef const unsigned char _OlReadOnlyBitmapType;

	/* Temporary structure */
	typedef struct {
		int unused;		/* keeps compiler happy */
	}	OlUnitType, *OlUnitTypeList,
		OlHelpInfo, *OlHelpInfoList,
		OlDynamicResource, *OlDynamicResourceList;
	
	typedef Boolean		(*OlActivateFunc)(
		Widget		widget,
		OlVirtualName	virtual_name,
		XtPointer	closure
	);
	#define	XtInheritActivateFunc	((OlActivateFunc)_XtInherit)
	
	/* Define a common structure used for all class extensions */
	typedef struct {
	    XtPointer	next_extension;	/* pointer to next in list */
	    XrmQuark	record_type;	/* NULLQUARK */
	    long	version;	/* version particular to extension
	    				   record */
	    Cardinal	record_size;	/* sizeof() particular extension
	    				   Record */
	}	OlClassExtensionRec, *OlClassExtension;
	
	typedef void		(*OlHighlightProc)(
		Widget		widget,
		OlDefine	highlight_type
	);
	#define	XtInheritHighlightHandler	((OlHighlightProc)_XtInherit)
	
	typedef Widget		(*OlRegisterFocusFunc)(Widget w);
	#define	XtInheritRegisterFocus	((OlRegisterFocusFunc)_XtInherit)
	
	typedef Widget		(*OlTraversalFunc)(
		Widget		traversal_manager,	/* widget id */
		Widget		w,			/* starting widget */
		OlVirtualName	direction,		/* direction */
		Time		time			/* request time */
	);
	#define	XtInheritTraversalHandler	((OlTraversalFunc)_XtInherit)
	
	/* Virtual mapping structure */
	typedef struct {
		OlDefine	type;		/* type flag */
		unsigned int	modifiers;	/* modifier mask */
		unsigned int	detail;		/* mapping detail, 
						   e.g., keycode */
		String		composed;	/* Name of virtual key that was
						   used to compose this one */
	} _OlVirtualMapping;
	
	/* Define some prototypes for class-instance-extension-part procedures */
	typedef void    	(*OlExtDestroyProc)(
	    Widget		shell,		/* shell's widget id */
	    XtPointer		cur_part	/* current ext part */
	);
	
	typedef void    	(*OlExtGetValuesProc)(
		Widget		shell,		/* shell widget's id */
		ArgList		args,		/* arg list */
		Cardinal*	num_args,	/* # of args */
		XtPointer	ext_part	/* current extension part */
	);
	
	typedef void		(*OlExtInitializeProc)(
		Widget		request,	/* request widget */
		Widget		c_new,		/* new widget */
		ArgList		args,		/* arg list */
		Cardinal*	num_args,	/* # of args */
		XtPointer	req_part,	/* request extension part */
		XtPointer	new_part	/* new extension part */
	);
	
	typedef Boolean		(*OlExtSetValuesFunc)(
		Widget		current,	/* current widget */
		Widget		request,	/* request widget */
		Widget		c_new,		/* new widget */
		ArgList		args,		/* arg list */
		Cardinal*	num_args,	/* # of args */
		XtPointer	cur_part,	/* current extension part */
		XtPointer	req_part,	/* request extension part */
		XtPointer	new_part	/* new extension part */
	);
	
	typedef int	ShellBehavior;	/* argument to XtNshellBehavior
					   resource */
	
	#define OtherBehavior		0/* Application-defined shell behavior */
	#define BaseWindow		1 /* Shell is a plain base window */
	#define PopupWindow		2 /* Shell is a plain, unpinned popup */
	#define PinnedWindow		3 /* Shell is pinned up */
	#define PinnedMenu		4 /* Shell is pinned menu window */
	#define PressDragReleaseMenu	5 /* Menu shell in press-drag-release
					     mode */
	#define StayUpMenu		6 /* Menu shell in stay-up mode */
	#define UnpinnedMenu		7 /* Shell is unpinned menu window */

	/* dynamic resource structure */
	typedef struct _OlDynResource	_OlDynResource, *_OlDynResourceList;

	typedef char*		(*_OlBaseProc)(Widget, Boolean,
		_OlDynResourceList);

	struct _OlDynResource {
		XtResource	res;		/* resource structure */
		int		offset;		/* byte offset */
		int		bit_offset;	/* bit offset into a byte */
		_OlBaseProc	proc;		/* base proc */
	};

	typedef struct {
		_OlDynResourceList	resources;
		int			num_resources;
	}	_OlDynData;
	
	typedef void		(*OlTransparentProc)(Widget, Pixel, Pixmap);
	typedef void		(*OlWMProtocolProc)(Widget, OlDefine action,
		OlWMProtocolVerify*);

	typedef	enum {
		SuperCaretNone,
		SuperCaretLeft,
		SuperCaretBottom
	} SuperCaretShape;

	typedef void    	(*SuperCaretQueryLocnProc)(
					   const Widget           target,
                                           const Widget           supercaret,
                                           const Dimension        width,
                                           const Dimension        height,
                                           unsigned int    *const sc_scale,
                                           SuperCaretShape *const sc_shape,
                                           Position        *const x_center_ret,
                                           Position        *const y_centre_ret
				 );
#define XtInheritSuperCaretQueryLocnProc        \
                                ((SuperCaretQueryLocnProc)_XtInherit)

#else	/* __STDC__ || __cplusplus */

#	define	XtInheritTransparentProc	((OlTransparentProc)_XtInherit)
#	define	XtInheritWMProtocolProc		((OlWMProtocolProc)_XtInherit)

	typedef struct {
		int unused;
	}	OlUnitType, *OlUnitTypeList,
		OlHelpInfo, *OlHelpInfoList,
		OlDynamicResource, *OlDynamicResourceList;
	
	
	typedef Boolean		(*OlActivateFunc)();
#	define	XtInheritActivateFunc	((OlActivateFunc)_XtInherit)
	
	typedef struct {
	    XtPointer	next_extension;
	    XrmQuark	record_type;
	    long	version;
	    Cardinal	record_size;
	} OlClassExtensionRec, *OlClassExtension;
	
	typedef void		(*OlHighlightProc)();
#	define	XtInheritHighlightHandler	((OlHighlightProc)_XtInherit)
	
	typedef Widget		(*OlRegisterFocusFunc)();
#	define	XtInheritRegisterFocus	((OlRegisterFocusFunc)_XtInherit)
	
	typedef Widget		(*OlTraversalFunc)();
#	define	XtInheritTraversalHandler	((OlTraversalFunc)_XtInherit)
	
	typedef struct {
		OlDefine	type;
		unsigned int	modifiers;
		unsigned int	detail;
		String		composed;
	} _OlVirtualMapping;
	
	typedef void		(*OlExtDestroyProc)();
	typedef void		(*OlExtGetValuesProc)();
	typedef void		(*OlExtInitializeProc)();
	typedef Boolean		(*OlExtSetValuesFunc)();
	typedef int		ShellBehavior;
	
#	define	OtherBehavior		0
#	define	BaseWindow		1
#	define	PopupWindow		2
#	define	PinnedWindow		3
#	define	PinnedMenu		4
#	define	PressDragReleaseMenu	5
#	define	StayUpMenu		6
#	define	UnpinnedMenu		7
	
	typedef struct __OlDynResource	_OlDynResource, *_OlDynResourceList;

	typedef char*		(*_OlBaseProc)();

	struct __OlDynResource {
		XtResource	res;		/* resource structure */
		int		offset;		/* byte offset */
		int		bit_offset;	/* bit offset into a byte */
		_OlBaseProc	proc;		/* base proc */
	};
	
	
	typedef struct {
		_OlDynResourceList resources;
		int		num_resources;
	} _OlDynData;
	
	typedef void		(*OlTransparentProc)();
	typedef void		(*OlWMProtocolProc)();

	typedef	enum {
		SuperCaretNone,
		SuperCaretLeft,
		SuperCaretBottom
	} SuperCaretShape;

	typedef void		(*SuperCaretQueryLocnProc)();

#define XtInheritSuperCaretQueryLocnProc        \
                                ((SuperCaretQueryLocnProc)_XtInherit)
 
#endif	/* __STDC__ || __cplusplus */


/**************************************************************************
 *
 *	Private external declarations
 **************************************************************************/

#if	defined(__STDC__) || defined(__cplusplus)
	extern XrmName			_OlApplicationName;
	
	#if	!defined(XtSpecificationRelease) || XtSpecificationRelease <= 4
		extern XtActionsRec	_OlGenericActionTable[];
		extern const Cardinal	_OlGenericActionTableSize;
	#else
		/*
		 * a rather unpleasant hack follows to simulate a
		 * single shared action table by a local copy of that
		 * action table since by default in R5 action tables
		 * are compiled in situ and therefore a single copy
		 * cannot be shared since CoreClassPartInitialize
		 * attempts to recompile this shared copy multiple
		 * times, causing a core dump.  So for every object
		 * that had knowledge of this symbol by including this
		 * file, that code is maintained by decalring a static
		 * copy, of course this now means that each object
		 * shall have its own copy and anyone previously
		 * assuming that they can modify this table and have its
		 * effects propagate across all those widgets that
		 * share this table will no longer work.  Since it
		 * seems that this is a highly unlikely semantic anyway
		 * it doesnt matter too much!
		 */
		static XtActionsRec _OlGenericActionTable[] = {
			{ "OlAction",   OlAction }
		};
		
		static const Cardinal	_OlGenericActionTableSize =
						XtNumber(_OlGenericActionTable);
	#endif
	
	extern OlEventHandlerRec	_OlGenericEventHandlerList[];
	extern const Cardinal		_OlGenericEventHandlerListSize;
	extern const char		_OlGenericTranslationTable[];
	extern Boolean			_OlDynResProcessing;
	extern Widget*			_OlShell_list;
	extern Cardinal			_OlShell_list_size;

#else	/* __STDC__ || __cplusplus */

	extern XrmName			_OlApplicationName;
	
#	if	!defined(XtSpecificationRelease) || XtSpecificationRelease <= 4
		extern XtActionsRec	_OlGenericActionTable[];
		extern Cardinal		_OlGenericActionTableSize;
#	else
		/*
		 * a rather unpleasant hack follows to simulate a
		 * single shared action table by a local copy of that
		 * action table since by default in R5 action tables
		 * are compiled in situ and therefore a single copy
		 * cannot be shared since CoreClassPartInitialize
		 * attempts to recompile this shared copy multiple
		 * times, causing a core dump.  So for every object
		 * that had knowledge of this symbol by including this
		 * file, that code is maintained by decalring a static
		 * copy, of course this now means that each object
		 * shall have its own copy and anyone previously
		 * assuming that they can modify this table and have
		 * its effects propagate across all those widgets that
		 * share this table will no longer work.  Since it
		 * seems that this is a highly unlikely semantic anyway
		 * it doesnt matter too much!
		 */
		static XtActionsRec	_OlGenericActionTable[] = {
			{ "OlAction",   OlAction }
		};
		
		static Cardinal		_OlGenericActionTableSize =
						XtNumber(_OlGenericActionTable);
#	endif
	
	extern OlEventHandlerRec	_OlGenericEventHandlerList[];
	extern Cardinal			_OlGenericEventHandlerListSize;
	extern char			_OlGenericTranslationTable[];
	extern Boolean			_OlDynResProcessing;
	extern Widget*			_OlShell_list;
	extern Cardinal			_OlShell_list_size;

#endif	/* __STDC__ || __cplusplus */


/*
 * function prototype section
 */

/*
 * Accelerate module
 */
 
#if	defined(__STDC__) || defined(__cplusplus)

extern void		_OlDestroyKeyboardHooks(Widget w);

extern void		_OlMakeAcceleratorText(Widget w, String str,
	String *qualifier, Boolean *meta_key, String *accelerator_text);

extern OlDefine		_OlAddMnemonic(Widget w, XtPointer data, char mnemonic);

extern void		_OlRemoveMnemonic(Widget w, XtPointer data,
	Boolean ignore_data, char mnemonic);
	
extern Widget		_OlFetchMnemonicOwner(Widget w, XtPointer* p_data,
	OlVirtualEvent virtual_event);

extern OlDefine		_OlAddAccelerator(Widget w, XtPointer data,
	String accelerator);
	
extern void		_OlRemoveAccelerator(Widget w, XtPointer data,
	Boolean ignore_data, String accelerator);

extern Widget		_OlFetchAcceleratorOwner(Widget w, XtPointer* p_data,
	OlVirtualEvent virtual_event);

extern void		_OlNewAcceleratorResourceValues(Screen* screen,
	XtPointer client_data);

#else	/* __STDC__ || __cplusplus */

extern void		_OlDestroyKeyboardHooks();
extern void		_OlMakeAcceleratorText();
extern OlDefine		_OlAddMnemonic();
extern void		_OlRemoveMnemonic();
extern Widget		_OlFetchMnemonicOwner();
extern OlDefine		_OlAddAccelerator();
extern void		_OlRemoveAccelerator();
extern Widget		_OlFetchAcceleratorOwner();
extern void		_OlNewAcceleratorResourceValues();

#endif	/* __STDC__ || __cplusplus */


/*
 * Applic module
 */

#if	defined(__STDC__) || defined(__cplusplus)

/* Beep the display only if Settings in Workspace permit it */
extern void		_OlBeepDisplay(
	Widget		widget,		/* widget wanting beep */
	Cardinal	count		/* number of beeps */
);

/* Checks to see if a widget can grab the pointer */ 
extern Boolean		_OlGrabPointer(
	Widget		widget,		/* widget requesting grab */
	int		owner_events,
	unsigned int	event_mask,
	int		pointer_mode, 
	int		keyboard_mode,
	Window		confine_to,
	Cursor		cursor,
	Time		time
);

/* Grab the server */
extern Boolean		_OlGrabServer(
	Widget		widget		/* widget requesting grab */
);

/* Used to ungrab the pointer */
extern void		_OlUngrabPointer(
	Widget		widget		/* widget wishing pointer ungrab */
);

/* Used to ungrab the server */
extern void		_OlUngrabServer(
	Widget		widget		/* widget wishing server ungrab */
);

#else	/* __STDC__ || __cplusplus */

extern void		_OlBeepDisplay();
extern Boolean		_OlGrabPointer();
extern Boolean		_OlGrabServer();
extern void		_OlUngrabPointer();
extern void		_OlUngrabServer();

#endif	/* __STDC__ || __cplusplus */


/*
 * Converters module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void	_OlAddOlDefineType(
	String		name,	/* OlDefine's name */
	OlDefine	value	/* OlDefine's value */
);

/* 
 * Add an OlDefine type to it's converter's database
 * Obsolete, use _OlAddOlDefine instead
 */
extern void	_OlRegisterOlDefineType(
	XtAppContext	app_context,	/* Application Context or NULL */
	String		name,		/* OlDefine's name */
	OlDefine	value		/* OlDefine's value */
);

extern void	_OlCvtStringToGravity(
	XrmValue*	args,		/* arguments needed for conversion */
	Cardinal*	num_args,	/* the number of converion args */
	XrmValue*	from,		/* value to convert */
	XrmValue*	to		/* returned converted value */
);

/* Converts string to OlDefine resource	*/
extern void	_OlCvtStringToOlDefine(
	XrmValue*	args,		/* arguments needed for conversion */
	Cardinal*	num_args,	/* the number of converion args */
	XrmValue*	from,		/* value to convert */
	XrmValue*	to		/* returned converted value */
);

#else	/* __STDC__ || __cplusplus */

extern void	_OlAddOlDefineType();
extern void	_OlRegisterOlDefineType();
extern void	_OlCvtStringToGravity();
extern void	_OlCvtStringToOlDefine();

#endif	/* __STDC__ || __cplusplus */


/*
 * Copy module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		_OlConvertToXtArgVal(
	const void*	src,	/* arbitrary source location */
	XtArgVal*	dst,	/* address of XtArgVal to hold data */
	size_t		size	/* size of data */
);

/* Copy an XtArgVal value into a destination of an arbitrary size. */
extern void		_OlCopyFromXtArgVal(
	const XtArgVal	src,	/* XtArgVal holding the data */
	void*		dst,	/* data's destination location */
	size_t		size	/* size of data */
);

/* Copy information located at some address into an XtArgVal. */
extern void		_OlCopyToXtArgVal(
	const void*	src, 	/* arbitrary source location */
	XtArgVal*	dst,	/* address of XtArgVal holding */
				/* the address of data's destinatio */
	size_t		size	/* size of data */
);


#else	/* __STDC__ || __cplusplus */

extern void		_OlConvertToXtArgVal();
extern void		_OlCopyFromXtArgVal();
extern void		_OlCopyToXtArgVal();

#endif	/* __STDC__ || __cplusplus */


/*
 * CvtColor module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		OlRegisterColorTupleListConverter(void);

#else	/* __STDC__ || __cplusplus */

extern void		OlRegisterColorTupleListConverter();

#endif	/* __STDC__ || __cplusplus */


/*
 * Dynamic module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern KeySym		_OlCanonicalKeysym(Display* display, KeySym keysym,
	KeyCode* p_keycode, Modifiers* p_modifiers);

extern void		_OlGetModifierMapping(Display* dpy);
extern void		_OlInitDynamicHandler(Widget w);
extern int		_OlKeysymToSingleChar(KeySym keysym);
extern KeySym		_OlSingleCharToKeysym(int chr);

extern Boolean		_OlStringToOlBtnDef(Display* display, XrmValue* args,
	Cardinal* num_args, XrmValue* from, XrmValue* to,
	XtPointer* converter_data);

extern Boolean		_OlStringToOlKeyDef(Display* display, XrmValue* args,
	Cardinal* num_args, XrmValue* from, XrmValue* to,
	XtPointer* converter_data);

extern OlVirtualName	_OlStringToVirtualKeyName(String str);

#else	/* __STDC__ || __cplusplus */

extern KeySym		_OlCanonicalKeysym();
extern void		_OlGetModifierMapping();
extern void		_OlInitDynamicHandler();
extern int		_OlKeysymToSingleChar();
extern KeySym		_OlSingleCharToKeysym();
extern Boolean		_OlStringToOlBtnDef();
extern Boolean		_OlStringToOlKeyDef();
extern OlVirtualName	_OlStringToVirtualKeyName();

#endif	/* __STDC__ || __cplusplus */


/*
 * DynResProc module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		_OlDynResProc(Screen* screen);

/* Get transparent_proc given a widget */
extern OlTransparentProc _OlGetTransProc(Widget w);

/* get dirty bit for a dynamic resource */
extern int		_OlGetDynBit(Widget w, _OlDynResourceList res);

/* set dirty bit for a dynamic resource */
extern void		_OlSetDynBit(Widget w, _OlDynResourceList res);

/* unset dirty bit for a dynamic resource */
extern void		_OlUnsetDynBit(Widget w, _OlDynResourceList res);

extern void		_OlInitDynResources(Widget w, _OlDynData* data);

extern void		_OlCheckDynResources(Widget w, _OlDynData* data,
	ArgList args, Cardinal num_args);

extern void		_OlMergeDynResources(
	_OlDynData*	c_new,		/* new  */
	_OlDynData*	old		/* old */
);

extern void		_OlDefaultTransparentProc(Widget w, Pixel pixel,
	Pixmap pixmap);

extern Pixmap		_OlGetRealPixmap(Widget w);

#else	/* __STDC__ || __cplusplus */

extern void		_OlDynResProc();
extern OlTransparentProc _OlGetTransProc();
extern int		_OlGetDynBit();
extern void		_OlSetDynBit();
extern void		_OlUnsetDynBit();
extern void		_OlInitDynResources();
extern void		_OlCheckDynResources();
extern void		_OlMergeDynResources();
extern void		_OlDefaultTransparentProc();
extern Pixmap		_OlGetRealPixmap();

#endif	/* __STDC__ || __cplusplus */


/*
 * Extension module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern OlClassExtension	_OlGetClassExtension(
	OlClassExtension	extension,	/* start with this one */
	XrmQuark		record_type,	/* type to look for */
	long			version		/* if non-zero, look for it */
);

#else	/* __STDC__ || __cplusplus */

extern OlClassExtension	_OlGetClassExtension();

#endif	/* __STDC__ || __cplusplus */


/*
 * Extension module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern XFontStruct*	_OlGetFont(Screen* screen, int point_size,
	String font_name);

extern XImage*		_OlGetImage(Screen* screen, int point_size, char* name);
extern void		_OlFreeFont(Display* display, XFontStruct* font);
extern void		_OlFreeImage(XImage* image);
extern void		_OlDisplayFontCache(void);

#else	/* __STDC__ || __cplusplus */

extern XFontStruct*	_OlGetFont();
extern XImage*		_OlGetImage();
extern void		_OlFreeFont();
extern void		_OlFreeImage();
extern void		_OlDisplayFontCache();

#endif	/* __STDC__ || __cplusplus */


/*
 * EventObj module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern Widget		_OlWidgetToGadget(
	Widget		w,			/* Composite */
	Position	x,			/* Position over Composite */
	Position	y			/* Position over Composite */
);

extern void		_OlAddGrab(
	Widget		w,			/* widget wanting grab */
	Boolean		exclusive,		/* exclusives mode? */
	Boolean		spring_loaded		/* spring loaded? */
);
	
extern void		_OlRemoveGrab(
	Widget		w		/* Widget with prior grab */
);

#else	/* __STDC__ || __cplusplus */

extern Widget		_OlWidgetToGadget();
extern void		_OlAddGrab();
extern void		_OlRemoveGrab();

#endif	/* __STDC__ || __cplusplus */


/*
 * Help module
 */

#if	defined(__STDC__) || defined(__cplusplus)

/* Expose the event handler that pops up the help widget */
extern void		_OlPopupHelpTree(
	Widget		w,			/* shell widget id */
	XtPointer	client_data,		/* unused */
	XEvent*		xevent,			/* help XEvent message */
	Boolean*	continue_to_dispatch	/* unused */
);

extern void		_OlPopdownHelpTree(
	Widget		w			/* shell widget id */		
);

extern void		_OlProcessHelpKey(
	Widget		w,		/* widget receiving keypress event */
	XEvent*		xevent		/* KeyPress XEvent pointer */
);

#else	/* __STDC__ || __cplusplus */

extern void		_OlPopupHelpTree();
extern void		_OlPopdownHelpTree();
extern void		_OlProcessHelpKey();

#endif	/* __STDC__ || __cplusplus */


/*
 * MaskArgs module
 */

#if	defined(__STDC__) || defined(__cplusplus)

/* Composes an Arg list from a source Arg list and a source MaskArg list. */
extern void		_OlComposeArgList(
	ArgList		args,		/* source Arg List */
	Cardinal	num_args,	/* number of source Args */
	MaskArgList	mask_args,	/* mask Arg List */
	Cardinal	mask_num_args,	/* number of mask Args */
	ArgList*	comp_args,	/* destination Arg List */
	Cardinal*	comp_num_args	/* number of dest Args */
);

#else	/* __STDC__ || __cplusplus */

extern void		_OlComposeArgList();

#endif	/* __STDC__ || __cplusplus */


/*
 * Manager module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void			_OlManagerCallPreSelectCBs(Widget selectee,
	XEvent* initiator);

extern void			_OlManagerCallPostSelectCBs(Widget selectee,
	XEvent* initiator);

#else	/* __STDC__ || __cplusplus */

extern void			_OlManagerCallPreSelectCBs();
extern void			_OlManagerCallPostSelectCBs();

#endif	/* __STDC__ || __cplusplus */


/*
 * Menu module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		_OlPopdownTrailingCascade(
	Widget		w,			/* shell widget id */
	Boolean		skip_first		/* first cascading menu */
);

#else	/* __STDC__ || __cplusplus */

extern void		_OlPopdownTrailingCascade();

#endif	/* __STDC__ || __cplusplus */


/*
 * OlCommon module
 */

#if	defined(__STDC__) || defined(__cplusplus)

/* Toolkit pre-initialization */
extern void		OlPreInitialize(
	char*		classname,	/* application class */
	XrmOptionDescRec* urlist,	/* options list */
	Cardinal	num_urs,	/* number of options */
	int*		argc,		/* pointer to argc */
	char*		argv[]);

/* Toolkit post-initialization */
extern void		OlPostInitialize(
	char*		classname,	/* application class */
	XrmOptionDescRec* urlist,	/* options list */
	Cardinal	num_urs,	/* number of options */
	int*		argc,		/* pointer to argc */
	char*		argv[]
);

/* Returns one of OLIT fundamental class types */
extern WidgetClass	_OlClass(Widget w);

/* Clear the background of a widget or a gadget	*/
extern void		_OlClearWidget(
	Widget		w,		/* widget/gadget id to clear */
	Boolean		exposures	/* generate exposure event? */
);

/* Complete name of widget, including ancestors. */
extern String		_OlFullName(
	Widget		w		/* the widget id */
);

/* To retrieve application title */
extern String		_OlGetApplicationTitle(
	Widget		w		/* any widget in application */
);

extern void		_OlGetRefNameOrWidget(Widget w, ArgList args,
	Cardinal* num_args);

/* Search up a widget tree until the shell is found */
extern Widget		_OlGetShellOfWidget(
	Widget		w		/* start search here */
);

/* to store application title */
extern void		_OlSetApplicationTitle(
	String		title		/* set title to this */
);

/* append Atom to WM_PROTOCOLS property of toplevel window */
extern void		_OlSetWMProtocol(
	Display*	display,
	Window		window,		/* window (toplevel) */
	Atom		property	/* atom to append */
);

/* To retrieve default text format. NOTE - use as a XtResourceDefaultProc */
extern void		_OlGetDefaultTextFormat(
	Widget		w,
	int		offset,
	XrmValue*	value
);

/* To retrieve default focus color for text input widgets -
 * or widgets containing a text input widget -
 * NOTE - use as an XtResourceDefaultProc
 */
extern void		_OlGetDefaultFocusColor(
	Widget		w,
	int		offset,
	XrmValue*	value
);

#else	/* __STDC__ || __cplusplus */

extern void		OlPreInitialize();
extern void		OlPostInitialize();

extern WidgetClass	_OlClass();
extern void		_OlClearWidget();
extern String		_OlFullName();
extern String		_OlGetApplicationTitle();
extern void		_OlGetRefNameOrWidget();
extern Widget		_OlGetShellOfWidget();
extern void		_OlSetApplicationTitle();
extern void		_OlSetWMProtocol();
extern void		_OlGetDefaultTextFormat();
extern void		_OlGetDefaultFocusColor();

#endif	/* __STDC__ || __cplusplus */


/*
 * OlDnDVCX module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		_OlDnDSetDisableDSClipping(Widget, Boolean);

#else	/* __STDC__ || __cplusplus */

extern void		_OlDnDSetDisableDSClipping();

#endif	/* __STDC__ || __cplusplus */


/*
 * OpenLook module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern int		GetLongProperty(Display* dpy, Window w, Atom property, 
	long* result);

extern int		GetWMDecorationHints(Display* dpy, Window w, 
	WMDecorationHints* wmdh);

extern void		SetWMIconSize(Display* dpy, Window w, WMIconSize* wmis);
extern int		GetWMPushpinState(Display* dpy, Window w, long* state);
extern int		GetWMState(Display* dpy, Window w);
extern int		GetWMWindowBusy(Display* dpy, Window w, long* state);

#else	/* __STDC__ || __cplusplus */

extern int		GetLongProperty();
extern int		GetWMDecorationHints();
extern void		SetWMIconSize();
extern int		GetWMPushpinState();
extern int		GetWMState();
extern int		GetWMWindowBusy();

#endif	/* __STDC__ || __cplusplus */


/*
 * RootShell module
 */

#if	defined(__STDC__) || defined(__cplusplus)

/* single click damping factor	*/
extern Cardinal		_OlGetMouseDampingFactor(
	Widget		w		/* widget id or NULL */
);

extern Cardinal		_OlGetMultiObjectCount(
	Widget		w		/* widget id or NULL */
);

extern void		_OlRegisterShell(Widget w);

extern Boolean		_OlSelectDoesPreview(
	Widget		w		/* widget id or NULL */
);

extern Boolean		_OlMenuAccelerators(
	Widget		w		/* widget id or NULL */
);

extern Boolean		_OlMouseless(
	Widget		w		/* widget id or NULL */
);

extern	OlDefine	_OlInputFocusFeedback(Widget w);
extern void		_OlUnregisterShell(Widget w);

#else	/* __STDC__ || __cplusplus */

extern Cardinal		_OlGetMouseDampingFactor();
extern Cardinal		_OlGetMultiObjectCount();
extern void		_OlRegisterShell();
extern Boolean		_OlSelectDoesPreview();
extern OlDefine		_OlInputFocusFeedback();
extern void		_OlUnregisterShell();

#endif	/* __STDC__ || __cplusplus */


/*
 * Traversal module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		_OlCallHighlightHandler(Widget w,
	OlDefine highlight_type);

extern void		_OlDeleteDescendant(Widget w);
extern void		_OlFreeRefName(Widget w);
extern void		_OlInsertDescendant(Widget w);

extern void		_OlRegisterFocusWidget(
	Widget		w,
	Boolean		override_current /* Override current focus widget? */
);

extern Widget		_OlRemapDestinationWidget(Widget w, XEvent* xevent);

extern void		_OlSetCurrentFocusWidget(
	Widget		w,
	OlDefine	state			/* OL_IN or OL_OUT */
);

extern void		_OlUpdateTraversalWidget(
	Widget		w,
	String		ref_name,
	Widget		ref_widget,
	Boolean		insert_update	/* True: insert, False: update */
);

#else	/* __STDC__ || __cplusplus */

extern void		_OlCallHighlightHandler();
extern void		_OlDeleteDescendant();
extern void		_OlFreeRefName();
extern void		_OlInsertDescendant();
extern void		_OlRegisterFocusWidget();
extern Widget		_OlRemapDestinationWidget();
extern void		_OlSetCurrentFocusWidget();
extern void		_OlUpdateTraversalWidget();

#endif	/* __STDC__ || __cplusplus */


/*
 * Vendor module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern Widget		_OlFindVendorShell(Widget w, Boolean is_mnemonic);

extern Widget		_OlGetDefault(
	Widget		w			/* any widget or shell */
);

extern void		_OlSetDefault(
	Widget		w,			/* widget in question */
	Boolean		wants_to_be_default	/* is default or not? */
);
extern void		_OlVendorRereadSomeResources(Widget     vendor_widget,
						     Arg *      args,
						     Cardinal * num_args);

#else	/* __STDC__ || __cplusplus */

extern Widget		_OlFindVendorShell();
extern Widget		_OlGetDefault();
extern void		_OlVendorRereadSomeResources();

#endif	/* __STDC__ || __cplusplus */


/*
 * Virtual module
 */

#if	defined(__STDC__) || defined(__cplusplus)

/* adds virtual btn/key mappings to the virtual btn/key lists */
extern void		_OlAddVirtualMappings(
	String		string		/* virtual token mappings */
);

/* adds virtual btn/key mappings to the virtual btn/key lists */
extern void		_OlAppAddVirtualMappings(
	Widget		widget,		/* widget id or NULL */
	const char*	string		/* virtual token mappings */
);

/* Dump the virtual buttons/keys to a file */
extern void		_OlDumpVirtualMappings(
	FILE*		fp,		/* destination file pointer */
	Boolean		long_form	/* True for long listing */
);

/* Get the mappings for a virtual button or virtual key	*/
extern Cardinal		_OlGetVirtualMappings(
	String		name,		/* virtual button name */
	_OlVirtualMapping list[],	/* application-supplied array to
					   fill in */
	Cardinal	list_length	/* number of array elements */
);

/* Initialize the virtual button/key code */
extern void		_OlInitVirtualMappings(
	Widget		widget		/* application widget */
);

/* Sees if a modifier mask and button number is a virtual button mapping */
extern Boolean		_OlIsVirtualButton(
	String		name,		/* virtual button name */
	unsigned int	state,		/* modifier mask */
	unsigned int	button,		/* button number or zero */
	Boolean		exclusive	/* True for perfect match */
);

/* Sees if a modifier mask and a keycode is a virtual key mapping */
extern Boolean		_OlIsVirtualKey(
	String		name, 		/* virtual key name */
	unsigned int	state,		/* modifier mask */
	KeyCode		keycode,	/* button from XEvent or NULL */
	Boolean		exclusive	/* True for perfect match */
);

/* Sees if an XEvent is either a virtual button or virtual Key event. */
extern Boolean		_OlIsVirtualEvent(
	String name,		/* Virtual Button/key name */
	XEvent* xevent,		/* XEvent to be checked */
	Boolean exclusive	/* True for perfect match */
);

extern void		_OlLoadVendorShell(void);

#else	/* __STDC__ || __cplusplus */

extern void		_OlAddVirtualMappings();
extern void		_OlAppAddVirtualMappings();
extern void		_OlDumpVirtualMappings();
extern Cardinal		_OlGetVirtualMappings();
extern void		_OlInitVirtualMappings();
extern Boolean		_OlIsVirtualButton();
extern Boolean		_OlIsVirtualKey();
extern Boolean		_OlIsVirtualEvent();
extern void		_OlLoadVendorShell();

#endif	/* __STDC__ || __cplusplus */

/*
 * SuperCaret
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern  Widget  _OlCreateSuperCaret(Widget      vendor,
                                   ArgList      args,
                                   Cardinal     num_args
                );
 
extern  Widget  _OlGetSuperCaret(Widget vendor);
 
extern  void    _OlCallUpdateSuperCaret(Widget   scsw, Widget target);

#else	/* __STDC__ || __cpplusplus */

extern  Widget  _OlCreateSuperCaret();
 
extern  Widget  _OlGetSuperCaret();
 
extern  void    _OlCallUpdateSuperCaret();

#endif	/* __STDC__ || __cpplusplus */

#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_OPENLOOKP_H */
