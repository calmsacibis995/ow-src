#ifndef	_XOL_OPENLOOK_H
#define	_XOL_OPENLOOK_H

#pragma	ident	"@(#)OpenLook.h	302.31	94/04/05 include/Xol SMI"	/* oltemporary:OpenLook.h 1.52	*/

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

/**************************************************************************
 *
 *	This file contains all the things necessary to compile
 *	an OLIT application.
 *
 **************************************************************************/

#include <Xol/OlCEnv.h>	/* This file must be included first */

#include <Xol/OlClients.h>
#include <Xol/OlReasons.h>
#include <stdarg.h>

#ifdef	_OL_BUILD
	#ifdef	_XOL_OLSTRINGS_H_
	#error	"Attempted to include wrong version of strings definitions."
	#else	/* _XOL_OLSTRINGS_H_ */
	#include <Xol/OlStringsTable.h>
	#endif	/* _XOL_OLSTRINGS_H_ */
#else   /* _OL_BUILD */
#include <Xol/OlStrings.h>
#endif  /* _OL_BUILD */

#include <X11/Intrinsic.h>
#include <X11/Xlib.h>

#if	!defined(__STDC__) && !defined(__cplusplus)
#	include <limits.h>
#endif


#ifdef	__cplusplus
extern "C" {
#endif


/*************************************************************************
 *	Define True Constant Tokens
 *		Tokens that appear in this section should not have
 *		their values changed.
 *************************************************************************/

#define	OL_IGNORE		(~0)
#define	OL_NO_ITEM		(~0)

/*************************************************************************
 *	Define Tokens that have arbitary value assignments.  (Though
 *	their values must not change since it would break backwards
 *	compatibility.)
 *	New tokens should be added to the end of the list and their
 *	values must be unique.
 *	WARNING: do not remove or renumber any of the values in this
 *	list.
 *************************************************************************/

#define OleditDone		0
#define OleditError		1
#define OleditPosError		2
#define OleditReject		3

#define OL_ABSENT_PAIR		0
#define OL_ALL			1
#define OL_ALWAYS		2
#define OL_ATOM_HELP		3
#define OL_BOTH			4
#define OL_BOTTOM		5
#define OL_BUTTONSTACK		6
#define OL_CENTER		7
#define OL_CLASS_HELP		8
#define OL_COLUMNS		9
#define OL_COPY_MASK_VALUE	10
#define OL_COPY_SIZE		11
#define OL_COPY_SOURCE_VALUE	12
#define OL_CURRENT		13
#define OL_DEFAULT_PAIR		14
#define OL_DISK_SOURCE		15
#define OL_DISPLAY_FORM		16
#define OL_DOWN			17
#define OL_EXISTING_SOURCE	18
#define OL_FIXEDCOLS		19
#define OL_FIXEDHEIGHT		20
#define OL_FIXEDROWS		21
#define OL_FIXEDWIDTH		22
#define OL_FLAT_BUTTON		23
#define OL_FLAT_CHECKBOX	24
#define OL_FLAT_CONTAINER	25
#define OL_FLAT_EXCLUSIVES	26
#define OL_FLAT_HELP		27
#define OL_FLAT_NONEXCLUSIVES	28
#define OL_HALFSTACK		29
#define OL_HORIZONTAL		30
#define OL_IMAGE		31
#define OL_IN			32
#define OL_INDIRECT_SOURCE	33
#define OL_LABEL		34
#define OL_LEFT			35
#define OL_MASK_PAIR		36
#define OL_MAXIMIZE		37
#define OL_MILLIMETERS		38
#define OL_MINIMIZE		39
#define OL_NEVER		40
#define OL_NEXT			41
#define OL_NONE			42
#define OL_NONEBOTTOM		43
#define OL_NONELEFT		44
#define OL_NONERIGHT		45
#define OL_NONETOP		46
#define OL_NOTICES		47
#define OL_NO_VIRTUAL_MAPPING	48
#define OL_OBLONG		49
#define OL_OUT			50
#define OL_OVERRIDE_PAIR	51
#define OL_PIXELS		52
#define OL_POINTS		53
#define OL_POPUP		54
#define OL_PREVIOUS		55
#define OL_PROG_DEFINED_SOURCE	56
#define OL_RECTBUTTON		57
#define OL_RIGHT		58
#define OL_ROWS			59
#define OL_SOURCE_FORM		60
#define OL_SOURCE_PAIR		61
#define OL_STAYUP		62
#define OL_STRING		63
#define OL_STRING_SOURCE	64
#define OL_TEXT_APPEND		65
#define OL_TEXT_EDIT		66
#define OL_TEXT_READ		67
#define OL_TOP			68
#define OL_TRANSPARENT_SOURCE	69
#define OL_VERTICAL		70
#define OL_VIRTUAL_BUTTON	71
#define OL_VIRTUAL_KEY		72
#define OL_WIDGET_HELP		73
#define OL_WINDOW_HELP		74
#define OL_WRAP_ANY		75
#define OL_WRAP_WHITE_SPACE	76
#define OL_CONTINUOUS		77
#define OL_GRANULARITY		78
#define OL_RELEASE		79
#define OL_TICKMARK		80
#define OL_PERCENT		81
#define OL_SLIDERVALUE		82
#define OL_WT_BASE		83
#define OL_WT_CMD		84
#define OL_WT_NOTICE		85
#define OL_WT_HELP		86
#define OL_WT_OTHER		87
#define OL_SUCCESS		88
#define OL_DUPLICATE_KEY	89
#define OL_DUPLICATEKEY		89
#define OL_BAD_KEY		90
#define OL_MENU_FULL		91
#define OL_MENU_LIMITED		92
#define OL_MENU_CANCEL		93
#define OL_SELECTKEY		94
#define OL_MENUKEY		95
#define OL_MENUDEFAULT		96
#define OL_MENUDEFAULTKEY	97
#define OL_HSBMENU		98
#define OL_VSBMENU		99
#define OL_ADJUSTKEY		100
#define OL_NEXTAPP		101
#define OL_NEXTWINDOW		102
#define OL_PREVAPP		103
#define OL_PREVWINDOW		104
#define OL_WINDOWMENU		105
#define OL_WORKSPACEMENU	106
#define OL_DEFAULTACTION	108
#define OL_DRAG			109
#define OL_DROP			110
#define OL_TOGGLEPUSHPIN	111
#define OL_PAGELEFT		112
#define OL_PAGERIGHT		113
#define OL_SCROLLBOTTOM		114
#define OL_SCROLLTOP		115
#define OL_MULTIRIGHT		116
#define OL_MULTILEFT		117
#define OL_MULTIDOWN		118
#define OL_MULTIUP		119
#define OL_IMMEDIATE		120
#define OL_MOVEUP		121
#define OL_MOVEDOWN		122
#define OL_MOVERIGHT		123
#define OL_MOVELEFT		124
#define OL_CLICK_TO_TYPE	125
#define OL_REALESTATE		126
#define OL_UNDERLINE		127
#define OL_HIGHLIGHT		128
#define OL_INACTIVE		129
#define OL_DISPLAY		130
#define OL_PROC			131
#define OL_SIZE_PROC		132
#define OL_DRAW_PROC		133
#define OL_PINNED_MENU		134
#define OL_PRESS_DRAG_MENU	135
#define OL_STAYUP_MENU		136
#define OL_POINTER		137
#define OL_INPUTFOCUS		138
#define OL_QUIT			142
#define OL_DESTROY		143
#define OL_DISMISS		144
#define OL_PRE			145
#define OL_POST			146
#define OL_GROW_OFF     	147
#define OL_GROW_HORIZONTAL      148
#define OL_GROW_VERTICAL        149
#define OL_GROW_BOTH    	150
#define OL_FIND			151
#define OL_AGAIN		152
#define OL_TEXT_BUFFER_SOURCE	155
#define OL_OLTEXT_BUFFER_SOURCE	156
#define OL_EXCLUSIVE    	157
#define OL_EXCLUSIVE_NONESET    158
#define OL_NONEXCLUSIVE 	159
#define	OL_ACTIVE		160
#define	OL_ABSENT		161
#define	OL_INCR_INACTIVE	162
#define	OL_DECR_INACTIVE	163
#define	OL_SUPERCARET		164
#define	OL_INPUT_FOCUS_COLOR	165

#define	OL_OPEN			166
#define	OL_SAVE			167
#define	OL_SAVE_AS		168
#define	OL_INCLUDE		169
#define	OL_DO_COMMAND		170


/* These are bit masks used in Vendor's XtNwmProtocolMask resource.*/
#if	defined(__STDC__) || defined(__cplusplus)
#	define	_OL_CHAR_BIT	8
#else	/* __STDC__ || __cplusplus */
#	define	_OL_CHAR_BIT	CHAR_BIT
#endif

#define _OL_HI_BIT()	(unsigned long)((unsigned long)1 << \
	((unsigned long)sizeof(long) * _OL_CHAR_BIT - (unsigned long)1))

#define _OL_WM_GROUP(X)		(_OL_HI_BIT() >> (X))
#define _OL_WM_GROUPALL()	(_OL_WM_GROUP(0))
#define _OL_WM_BITALL()		(~(_OL_WM_GROUP(0)))
#define OL_WM_DELETE_WINDOW	(_OL_WM_GROUP(0) | ((unsigned long)1 << 0))
#define OL_WM_SAVE_YOURSELF	(_OL_WM_GROUP(0) | ((unsigned long)1 << 1))
#define OL_WM_TAKE_FOCUS	(_OL_WM_GROUP(0) | ((unsigned long)1 << 2))

#define _OL_WM_TESTBIT(X, B)	(((X & _OL_WM_GROUPALL()) & \
				 (B & _OL_WM_GROUPALL())) ? \
				 ((X & _OL_WM_BITALL()) & \
				 (B & _OL_WM_BITALL())) : 0)

/* 
 * OLIT Virtual Event (OPEN LOOK commands),
 * To maintain the backward compatiability, the following "#define"s
 * are not sorted. As a reminder, the first 59 (i.e., 0-58)
 * entries are according to the OlInputEvent stuff from Dynamic.h
 * NOTE:  DO NOT ADD ANY VALUES TO THE END OF THIS LIST -- ADD THEM TO
 * TO THE END OF THE ABOVE LIST SO THAT THE NUMBER OF COLLISIONS
 * (FOR THE #define SYMBOLS' NUMERIC VALUES IS MINIMIZED!!!!
 */
#define OL_UNKNOWN_INPUT	0
#define OL_UNKNOWN_BTN_INPUT	1
#define OL_SELECT		2
#define OL_ADJUST		3
#define OL_MENU			4
#define OL_CONSTRAIN		5
#define OL_DUPLICATE		6
#define OL_PAN			7
#define OL_UNKNOWN_KEY_INPUT	8
#define OL_CUT			9
#define OL_COPY			10
#define OL_PASTE		11
#define OL_HELP			12
#define OL_CANCEL		13
#define OL_PROP			14
#define OL_PROPERTY		OL_PROP
#define OL_STOP			15
#define OL_UNDO			16
#define OL_NEXTFIELD		17
#define OL_NEXT_FIELD		OL_NEXTFIELD
#define OL_PREVFIELD		18
#define OL_PREV_FIELD		OL_PREVFIELD
#define OL_CHARFWD		19
#define OL_CHARBAK		20
#define OL_ROWDOWN		21
#define OL_ROWUP		22
#define OL_WORDFWD		23
#define OL_WORDBAK		24
#define OL_LINESTART		25
#define OL_LINEEND		26
#define OL_DOCSTART		27
#define OL_DOCEND		28
#define OL_PANESTART		29
#define OL_PANEEND		30
#define OL_DELCHARFWD		31
#define OL_DELCHARBAK		32
#define OL_DELWORDFWD		33
#define OL_DELWORDBAK		34
#define OL_DELLINEFWD		35
#define OL_DELLINEBAK		36
#define OL_DELLINE		37
#define OL_SELCHARFWD		38
#define OL_SELCHARBAK		39
#define OL_SELWORDFWD		40
#define OL_SELWORDBAK		41
#define OL_SELLINEFWD		42
#define OL_SELLINEBAK		43
#define OL_SELLINE		44
#define OL_SELFLIPENDS		45
#define OL_REDRAW		46
#define OL_RETURN		47
#define OL_PAGEUP		48
#define OL_PAGEDOWN		49
#define OL_HOME			50
#define OL_END			51
#define OL_SCROLLUP		52
#define OL_SCROLLDOWN		53
#define OL_SCROLLLEFT		54
#define OL_SCROLLRIGHT		55
#define OL_SCROLLLEFTEDGE	56
#define OL_SCROLLRIGHTEDGE	57
#define OL_PGM_GOTO		58	/* LAST NUMBER IN THIS LIST */
/*
 * NOTE:  DO NOT ADD ANY VALUES TO THE END OF THE ABOVE LIST -- ADD THEM TO
 * TO THE END OF THE ABOVE THIS LIST SO THAT THE NUMBER OF COLLISIONS
 * (FOR THE #define SYMBOLS' NUMERIC VALUES IS MINIMIZED!!!!
 */

/*************************************************************************
 *	Define Bitwise Tokens
 *		This are easily identified by the naming convention
 *************************************************************************/
#define OL_B_OFF		0
#define OL_B_HORIZONTAL		(1<<0)
#define OL_B_VERTICAL		(1<<1)
#define OL_B_BOTH		(OL_B_VERTICAL|OL_B_HORIZONTAL)

/*************************************************************************
 *    Define pseudo-gravity values. Real gravity values currently
 *    stop with 10, but we'll leave some room for official growth.
 *************************************************************************/
#define Ol_EastWestGravity    1000    /* attach left and right */
#define Ol_NorthSouthGravity  1001    /* attach top and bottom */
#define Ol_AllGravity         1002    /* attach all four sides */

/*************************************************************************
 *	Define Constant tokens that are being maintained for source
 *	compatibility with older versions of the toolkit.
 *************************************************************************/
#define OL_BEEP_NEVER			OL_NEVER
#define OL_BEEP_NOTICES			OL_NOTICES
#define OL_BEEP_NOTICES_AND_FOOTERS	OL_ALWAYS
#define OL_BEEP_ALWAYS			OL_ALWAYS
#define OL_AUTO_SCROLL_OFF		OL_B_OFF
#define OL_AUTO_SCROLL_HORIZONTAL	OL_B_HORIZONTAL
#define OL_AUTO_SCROLL_VERTICAL		OL_B_VERTICAL
#define OL_AUTO_SCROLL_BOTH		OL_B_BOTH

/*************************************************************************
 *	Define public structures
 *************************************************************************/

/* Define a callback management structure for flat widgets */
typedef struct {
	Cardinal	item_index;	/* sub-object initiating callb.	*/
	XtPointer	items;		/* sub-object list */
	Cardinal	num_items;	/* number of sub-objects */
	String*		item_fields;	/* key of fields for list */
	Cardinal	num_item_fields;/* number of item fields */
	Cardinal	num_fields;	/* number of item fields */
} OlFlatCallData;

/* Define a structure for the complex flat help id */
typedef struct _OlFlatHelpId {
	Widget		widget;
	Cardinal	item_index;
} OlFlatHelpId;


/**************************************************************************
 *	Macros
 **************************************************************************/

/* 
 * The following three macros would normally appear in the private
 * header file, but we need them for the metric macros
 */
#define _OlFloor(value)		((int)(value))

#define _OlCeiling(value)	((int)(_OlFloor(value) == (value) ? \
					(value) : ((value) + 1)))

#define _OlRound(dbl)		((int)((double)(dbl) + \
				(double)((double)(dbl) < (double)0 ? -0.5 : 0.5)))

/* Metric macros */
extern Display*		toplevelDisplay;	/* in OlCommon.c */

#define OlDefaultDisplay toplevelDisplay

#define OlDefaultScreen  DefaultScreenOfDisplay(toplevelDisplay)

/* Pixels to Millimeters */
#define Ol_ScreenPixelToMM(direction, value, screen) \
	((double)(value) * (double)(direction == OL_HORIZONTAL ? \
	((double)WidthMMOfScreen(screen) / (double)WidthOfScreen(screen)) : \
	((double)HeightMMOfScreen(screen) / (double)HeightOfScreen(screen))))

#define OlScreenPixelToMM(direction, value, screen) \
		_OlRound(Ol_ScreenPixelToMM(direction, value, screen))

#define Ol_PixelToMM(direction, value) \
		Ol_ScreenPixelToMM(direction, value, OlDefaultScreen)

#define OlPixelToMM(direction, value) \
		OlScreenPixelToMM(direction, value, OlDefaultScreen)

/* Millimeters to Pixels */
#define Ol_ScreenMMToPixel(direction, value, screen) \
	((double)(value) * (double)(direction == OL_HORIZONTAL ? \
	((double)WidthOfScreen(screen) / (double)WidthMMOfScreen(screen)) : \
	((double)HeightOfScreen(screen) / (double)HeightMMOfScreen(screen))))

#define OlScreenMMToPixel(direction, value, screen) \
		_OlRound(Ol_ScreenMMToPixel(direction, value, screen))

#define Ol_MMToPixel(direction, value) \
		Ol_ScreenMMToPixel(direction, value, OlDefaultScreen)

#define OlMMToPixel(direction, value) \
		OlScreenMMToPixel(direction, value, OlDefaultScreen)

/* Pixels to Points */
#define Ol_ScreenPixelToPoint(direction, value, screen) \
	(2.834645669 * (double)(value) * (double)(direction == OL_HORIZONTAL ? \
	((double)WidthMMOfScreen(screen) / (double)WidthOfScreen(screen)) : \
	((double)HeightMMOfScreen(screen) / (double)HeightOfScreen(screen))))

#define OlScreenPixelToPoint(direction, value, screen) \
		_OlRound(Ol_ScreenPixelToPoint(direction, value, screen))

#define Ol_PixelToPoint(direction, value) \
		Ol_ScreenPixelToPoint(direction, value, OlDefaultScreen)

#define OlPixelToPoint(direction, value) \
		OlScreenPixelToPoint(direction, value, OlDefaultScreen)

/* Points to Pixels */
#define Ol_ScreenPointToPixel(direction, value, screen) \
	(0.352777777 * (double)(value) * (double)(direction == OL_HORIZONTAL ? \
	((double)WidthOfScreen(screen) / (double)WidthMMOfScreen(screen)) : \
	((double)HeightOfScreen(screen) / (double)HeightMMOfScreen(screen))))

#define OlScreenPointToPixel(direction, value, screen) \
		_OlRound(Ol_ScreenPointToPixel(direction, value, screen))

#define Ol_PointToPixel(direction, value) \
		Ol_ScreenPointToPixel(direction, value, OlDefaultScreen)

#define OlPointToPixel(direction, value) \
		OlScreenPointToPixel(direction, value, OlDefaultScreen)


/*************************************************************************
 *
 *	typedef's, enum's, struct's
 *************************************************************************/

#if	defined(__STDC__) || defined(__cplusplus)

/* Add a function Prototype for Dynamic callbacks */
typedef void		(*OlDynamicCallbackProc)(XtPointer data);

typedef short		OlDefine;	/* OLIT non-bitmask #defines */
typedef OlDefine	OlVirtualName;
typedef unsigned long	OlBitMask;	/* OLIT bitmask #defines */

/* 
 * Declare a function pointer type for the variable argument error
 * handlers.
 */
typedef void	(*OlVaDisplayErrorMsgHandler)(
	Display*	display,	/* display pointer or NULL */
	String		name,		/* message's resource name
					   concatenated with the type */
	String		type,		/* message's resource type
					   concatenated to the name */
	String		c_class,	/* class of message */
	String		message,	/* composed message created
					   from */
	va_list		vargs
);
typedef OlVaDisplayErrorMsgHandler OlVaDisplayWarningMsgHandler;

typedef void		(*OlErrorHandler)(
	String		message		/* simple error message string */
);

typedef void		(*OlWarningHandler)(
	String		message		/* simple warning message string */
);

#else	/* __STDC__ || __cplusplus */

typedef void		(*OlDynamicCallbackProc)();
typedef short		OlDefine;
typedef OlDefine	OlVirtualName;
typedef unsigned long	OlBitMask;
typedef void		(*OlVaDisplayErrorMsgHandler)();
typedef OlVaDisplayErrorMsgHandler OlVaDisplayWarningMsgHandler;
typedef void		(*OlErrorHandler)();
typedef void		(*OlWarningHandler)();

#endif	/* __STDC__ || __cplusplus */


/* 
 * Include some things that should of been in the R3 Intrinsics but
 * were not.  These must be removed for R4.
 */
#ifndef	XtSpecificationRelease
	typedef caddr_t		XtPointer;
#	define	XtRWidget	"Widget"
#endif	/* XtSpecificationRelease */

typedef struct {
	String		name;		/* XtN string */
	String		default_value;	/* `,' seperated string */
	OlVirtualName	virtual_name;	/* ol command value */
} OlKeyOrBtnRec, *OlKeyOrBtnInfo;
					/* Packed Widget Structure */
typedef struct {
	Widget          widget;		/* new id to be filled in */
	String          name;		/* name of new widget */
	WidgetClass*	class_ptr;	/* addr. of widget class */
	Widget*	parent_ptr;		/* addr. of parent id */
	String		descendant;	/* resource names used in GetValues
					 * call on parent to get real
					 * parent */
	ArgList         resources;	/* resources for new widget */
	Cardinal        num_resources;	/* number of resources */
	Boolean         managed;	/* should widget be managed? */
}			OlPackedWidget, *OlPackedWidgetList;

/*
 * typedef for OLIT font
 */
typedef	XtPointer	OlFont;

/*
 * String Representaion types for I18N
 */
typedef XtPointer OlStr;

typedef enum _OlStrRep {
	OL_SB_STR_REP,
	OL_MB_STR_REP,
	OL_WC_STR_REP
} OlStrRep;

/*
 * Enumerated data types for specifying im mode
 */
typedef enum _OlImPreeditStyle {
	OL_ON_THE_SPOT,
	OL_OVER_THE_SPOT,
	OL_ROOT_WINDOW,
	OL_NO_PREEDIT
} OlImPreeditStyle;

typedef enum _OlImStatusStyle{
	OL_IM_DISPLAYS_IN_CLIENT,
	OL_IM_DISPLAYS_IN_ROOT,
	OL_NO_STATUS
} OlImStatusStyle;


/*
 * The Generic Callback structs' definition
 */
typedef struct _OlAnyCallbackStruct {
	int			reason;
} OlAnyCallbackStruct;

typedef struct _OlAnyEventCallbackStruct {
	int			reason;
	XEvent*			event;
} OlAnyEventCallbackStruct;


typedef enum _OGlyphType {
	OL_GLYPH_TYPE_XIMAGE,
	OL_GLYPH_TYPE_PIXMAP
} OlGlyphType;

typedef union _OlGlyphRep {
	XImage			ximage;
	Pixmap			pixmap;
} *OlGlyphRep, OlGlyphRepRec;

typedef struct _OlGlyph {
	OlGlyphType		type;
	OlGlyphRep		rep;
} *OlGlyph, OlGlyphRec;


/*
 * Support for Text Verification Callbacks
 */
typedef enum { motionVerify, modVerify, leaveVerify } OlVerifyOpType;
typedef enum { OlsdLeft, OlsdRight } OlScanDirection;

typedef enum {
    OlstPositions, OlstWhiteSpace, OlstEOL, OlstParagraph, OlstLast
}			OlScanType;

typedef long		OlTextPosition;
typedef short		 TextFit;


typedef struct {
	int			firstPos;
	int			length;
	unsigned char*		ptr;
}			OlTextBlock, *OlTextBlockPtr;

typedef struct {
	XEvent*			xevent;
	OlVerifyOpType		operation;
	Boolean			doit;
	OlTextPosition		currInsert,
				newInsert;
	OlTextPosition		startPos,
				endPos;
	OlTextBlock*		text;
}			OlTextVerifyCD, *OlTextVerifyPtr;

typedef char		OlMnemonic;

typedef struct {
	Boolean			consumed;
	XEvent*			xevent;
	Modifiers		dont_care;
	OlVirtualName		virtual_name;
	KeySym			keysym;
	String			buffer;
	Cardinal		length;
	Cardinal		item_index;
}			OlVirtualEventRec,* OlVirtualEvent;

typedef struct _OlVirtualEventInfo *OlVirtualEventTable; /* an opque defn */

#if	defined(__STDC__) || defined(__cplusplus)
typedef void		(*OlEventHandlerProc)(Widget w, OlVirtualEvent event);
#else	/* __STDC__ || __cplusplus */
typedef void		(*OlEventHandlerProc)();
#endif	/* __STDC__ || __cplusplus */

typedef struct {
	int			type;	/* XEvent type */
	OlEventHandlerProc	handler;/* handling routine */
} OlEventHandlerRec, *OlEventHandlerList;

typedef struct {
	unsigned long	msgtype;	 /* type of WM msg */
	XEvent		*xevent;
} OlWMProtocolVerify;

/*
 * For the 3D coloration:
 */

typedef struct _OlColorTuple {
	Pixel		bg0;	/* ``white'' */
	Pixel		bg1;	/* XtNbackground */
	Pixel		bg2;	/* bg1 + 10-15% black */
	Pixel		bg3;	/* bg1 + 50% black */
}		OlColorTuple;

typedef struct _OlColorTupleList {
	OlColorTuple*	list;
	Cardinal	size;
}		OlColorTupleList;

/*************************************************************************
 *	Public extern declarations
 *************************************************************************/


#define OL_CORE_IE		(XtPointer) 1
#define OL_TEXT_IE		(XtPointer) 2
#define OL_DEFAULT_IE		(XtPointer) 3

#if	defined(__STDC__) || defined(__cplusplus)
typedef void		(*OlMenuPositionProc)(
	Widget		menu,		/* menu shell widget id */
	Widget		emanate,	/* the emanate widget */
	Cardinal	emanate_index,	/* index to flat item */
	OlDefine	state,		/* menu's state */
	Position*	mx,     	/* moving position */
	Position*	my,     	/* moving position */
	Position*	px,		/* pointer position */
	Position*	py		/* pointer position */
);
#else	/* __STDC__ || __cplusplus */
typedef void		(*OlMenuPositionProc)();
#endif	/* __STDC__ || __cplusplus */


/*
 * Action module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		OlAction(Widget w, XEvent* xevent, String* params,
	Cardinal* num_params);

extern Boolean		OlActivateWidget(
	Widget		w,	/* widget to activate */
	OlVirtualName	type,	/* type of activation */
	XtPointer	data	/* widget specific data	*/
);
	
extern Boolean		OlAssociateWidget(
	Widget		leader,			/* leader widget */
	Widget		follower,		/* follower widget */
	Boolean		disable_traversal	/* disable traversal */
);
	
extern void		OlUnassociateWidget(Widget follower);

extern Boolean		OlSetAppEventProc(
	Widget			w,		/* widget of the application */
	OlDefine		listtype,	/* type: OL_PRE or OL_POST */
	OlEventHandlerList	list,		/* list */
	Cardinal		count		/* length of list */
);

#else	/* __STDC__ || __cplusplus */

extern void		OlAction();
extern Boolean		OlActivateWidget();
extern Boolean		OlAssociateWidget();
extern void		OlUnassociateWidget();
extern Boolean		OlSetAppEventProc();

#endif	/* __STDC__ || __cplusplus */


/*
 * Dynamic module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		OlCallDynamicCallbacks(void);

extern void		OlClassSearchIEDB(WidgetClass wc,
	OlVirtualEventTable db);

extern void		OlClassSearchTextDB(
	WidgetClass	wc		/* the target */
);

extern OlVirtualEventTable OlCreateInputEventDB(Widget w,
	OlKeyOrBtnInfo key_info, int num_key_info, OlKeyOrBtnInfo btn_info,
	int num_btn_info);

extern void		OlLookupInputEvent(
	Widget		w,			/* widget getting xevent */
	XEvent*		xevent,			/* xevent to look at */
	OlVirtualEvent	virtual_event_ret,	/* returned virtual event */
	XtPointer	db_flag			/* search flag */
);

extern void		OlRegisterDynamicCallback(OlDynamicCallbackProc CB,
	XtPointer data);

extern int		OlUnregisterDynamicCallback(OlDynamicCallbackProc CB,
	XtPointer data);

extern void		OlWidgetSearchIEDB(Widget w, OlVirtualEventTable db);

extern void		OlWidgetSearchTextDB(Widget w);

#else	/* __STDC__ || __cplusplus */

extern void		OlCallDynamicCallbacks();
extern void		OlClassSearchIEDB();
extern void		OlClassSearchTextDB();
extern OlVirtualEventTable OlCreateInputEventDB();
extern void		OlLookupInputEvent();
extern void		OlRegisterDynamicCallback();
extern int		OlUnregisterDynamicCallback();
extern void		OlWidgetSearchIEDB();
extern void		OlWidgetSearchTextDB();

#endif	/* __STDC__ || __cplusplus */


/*
 * Error module
 */
 
#if	defined(__STDC__) || defined(__cplusplus)

/*
 * The OLIT toolkit's fatal error routine.
 * This is a variable argument list routine where the first 5 parameters are
 * mandatory.
 */
extern void
OlVaDisplayErrorMsg(
	Display*	display,	/* Display pointer or NULL */
	String		name,		/* message's resource name - 
					   concatenated with the type */
	String		type,		/* message's resource type - 
				   	   concatenated to the name */
	String		c_class,	/* class of message */
	String		message,	/* the message string */
	...				/* parameters; variable arguments to
					   satisfy the message parameters */
);
	
/*
 * The OLIT toolkit's non-fatal error routine.
 * This is a variable argument list routine where the first 5 parameters are
 * mandatory.
 */
extern void		OlVaDisplayWarningMsg(
	Display*	display,	/* Display pointer or NULL */
	String		name,		/* message's resource name - 
				   	   concatenated with the type */
	String		type,		/* message's resource type - 
				   	   concatenated to the name */
	String		c_class,	/* class of message */
	String		message,	/* the message string */
	...				/* parameters;	variable arguments to
					   satisfy the message parameters */
);
	
/* OLIT toolkit error */
extern void		 OlError(
String error_msg	/* message string */
);

/* 
 * This procedure is used to override the default fatal error
 * message procedure.
 */
extern OlVaDisplayErrorMsgHandler	OlSetVaDisplayErrorMsgHandler(
	OlVaDisplayErrorMsgHandler handler	/* new handler or NULL	*/
);
	
/*
 * This procedure is used to override the default non-fatal error message
 * procedure.
 */
extern OlVaDisplayWarningMsgHandler	OlSetVaDisplayWarningMsgHandler(
	OlVaDisplayWarningMsgHandler handler	/* new handler or NULL	*/
);
	
/* To register a procedure to be called on fatal errors: */
extern OlErrorHandler	 OlSetErrorHandler(
	OlErrorHandler handler	/* handler to be called	*/
);

/* To register a procedure to be called on non-fatal errors: */
extern OlWarningHandler	 OlSetWarningHandler(
	OlWarningHandler handler	/* handler to be called */
);

/* OLIT toolkit warning	*/
extern void		 OlWarning(
	String s	/* message string */
);

#else	/* __STDC__ || __cplusplus */

extern void		 OlVaDisplayErrorMsg();
extern void		 OlVaDisplayWarningMsg();

extern void		 OlError();
extern void		 OlWarning();

extern OlVaDisplayErrorMsgHandler	OlSetVaDisplayErrorMsgHandler();
extern OlVaDisplayWarningMsgHandler	OlSetVaDisplayWarningMsgHandler();

extern OlErrorHandler	 OlSetErrorHandler();
extern OlWarningHandler	 OlSetWarningHandler();

#endif	/* __STDC__ || __cplusplus */


/*
 * Help module
 */
 
#if	defined(__STDC__) || defined(__cplusplus)

extern void		OlRegisterHelp(
	OlDefine	id_type,	/* type of help being registered */
	XtPointer	id,		/* id of object registering help */
	OlStr		tag,		/* string tag for the help msg*/
	OlDefine	source_type,	/* type of help message source */
	XtPointer	source		/* pointer to the message source */
);

#else	/* __STDC__ || __cplusplus */

extern void		OlRegisterHelp();

#endif	/* __STDC__ || __cplusplus */


/*
 * Menu module
 */
 
#if	defined(__STDC__) || defined(__cplusplus)

extern void		OlMenuPost(
	Widget	w	/* Menu's Widget id */
);

extern void		OlMenuUnpost(
	Widget	w	/* menu's widget id */
);

extern void		OlMenuPopup(
	Widget			w,		/* menu's widget id */
	Widget			emanate,	/* emanate widget */
	Cardinal		emanate_index,	/* index to flat item */
	OlDefine		state,		/* menu's state */
	Boolean			setpos,		/* set position */
	Position		x,		/* pointer position */
	Position		y,		/* pointer position */
	OlMenuPositionProc	pos_proc	/* procedure or NULL */
);

extern void		OlMenuPopdown(
	Widget			w,		/* menu's widget id */
	Boolean			pinned_also	/* pinned or not */
);


extern	void 		OlCallbackPopdownMenu(
				Widget w, 
				XtPointer client_data, 
				XtPointer call_data);

#else	/* __STDC__ || __cplusplus */

extern void		OlMenuPost();
extern void		OlMenuUnpost();
extern void		OlMenuPopdown();
extern void		OlMenuPopup();
extern void 		OlCallbackPopdownMenu();

#endif	/* __STDC__ || __cplusplus */


/*
 * DragNDrop module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern Boolean		OlDragAndDrop(
	Widget		w,		/* widget which started drag operation */
	Window*		window,		/* window where drop occurred */
	Position*	xPosition,	/* x location in window of drop */
	Position*	yPosition	/* y location in window of drop */
);

extern void		OlGrabDragPointer(
	Widget		w,	/* widget wanting to begin draa operation */
	Cursor		cursor,	/* cursor used during drag operation */
	Window		window	/* confine to window or None */
);

extern void		OlUngrabDragPointer(
	Widget		w	/* widget which started the drag operation */
);

extern void		OlChangeGrabbedDragCursor(Widget w, Cursor cursor);

#else	/* __STDC__ || __cplusplus */

extern Boolean		OlDragAndDrop();
extern void		OlGrabDragPointer();
extern void		OlUngrabDragPointer();
extern void		OlChangeGrabbedDragCursor();

#endif	/* __STDC__ || __cplusplus */


/*
 * FlatState module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		OlFlatGetValues(
	Widget		w,		/* flat widget id */
	Cardinal	item_index,	/* item to query */
	ArgList		args,		/* querying args */
	Cardinal	num_args	/* number of args */
);

extern void		OlFlatSetValues(
	Widget		w,		/* flat widget id */
	Cardinal	item_index,	/* item to modify */
	ArgList		args,		/* modifying args */
	Cardinal	num_args	/* number of args */
);

extern void		OlVaFlatGetValues(
	Widget		w,		/* flat widget id */
	Cardinal	item_index,	/* item to query */
	...				/* NULL terminated name/value pairs */
);

extern void		OlVaFlatSetValues(
	Widget		w,		/* flat widget id */
	Cardinal	item_index,	/* item to modify */
	...				/* NULL terminated name/value pairs */
);

#else	/* __STDC__ || __cplusplus */

extern void		OlFlatGetValues();
extern void		OlFlatSetValues();
extern void		OlVaFlatGetValues();
extern void		OlVaFlatSetValues();

#endif	/* __STDC__ || __cplusplus */


/*
 * FlatPublic module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern Boolean		OlFlatCallAcceptFocus(
	Widget		w,		/* Flat widget id */
	Cardinal	i,		/* Item index */
	Time		time		/* time */
);

extern Cardinal		OlFlatGetFocusItem(
	Widget		w		/* Flat widget id */
);

extern void		OlFlatGetItemGeometry(
	Widget		w,		/* Flat widget id */
	Cardinal	i,		/* item index */
	Position*	x_ret,		/* x return */
	Position*	y_ret,		/* y return */
	Dimension*	w_ret,		/* width return */
	Dimension*	h_ret		/* height return */
);
	
extern Cardinal		OlFlatGetItemIndex(
	Widget		w,		/* Flat widget id */
	Position	x,		/* x location */
	Position	y		/* y location */
);

#else	/* __STDC__ || __cplusplus */

extern Boolean		OlFlatCallAcceptFocus();
extern Cardinal		OlFlatGetFocusItem();
extern void		OlFlatGetItemGeometry();
extern Cardinal		OlFlatGetItemIndex();

#endif	/* __STDC__ || __cplusplus */


/*
 * OlCommon module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern Pixel		OlBlackPixel(Widget w);
extern Colormap		OlColormapOfObject(Widget object);
extern int		OlDepthOfObject(Widget object);

extern void		OlGetApplicationResources(Widget w, XtPointer base,
	XtResource* resources, int num_resources, ArgList args,
	Cardinal num_args);
extern void		OlSetApplicationResources(Widget w, XtPointer base,
	XtResource* resources, int num_resources, ArgList args,
	Cardinal num_args);

extern OlFont 	OlGetDefaultFont(Widget w);

/* Toolkit initialization */
extern Widget		OlInitialize(
	const char*	shell_name,	/* initial shell instance name */
	const char*	classname,	/* application class */
	XrmOptionDescRec* urlist,	/* options list	*/
	Cardinal	num_urs,	/* number of options */
	Cardinal*	argc,		/* pointer to argc */
	char*		argv[]
);

extern void		OlUpdateDisplay(Widget w);
extern Visual*		OlVisualOfObject(Widget object);
extern Pixel		OlWhitePixel(Widget w);
extern void		OlSetDefaultTextFormat(OlStrRep format);

#else	/* __STDC__ || __cplusplus */

extern Pixel		OlBlackPixel();
extern Colormap		OlColormapOfObject();
extern int		OlDepthOfObject();
extern void		OlGetApplicationResources();
extern void		OlSetApplicationResources();
extern XFontStruct*	OlGetDefaultFont();
extern Widget		OlInitialize();
extern void		OlUpdateDisplay();
extern Visual*		OlVisualOfObject();
extern Pixel		OlWhitePixel();
extern void		OlSetDefaultTextFormat();

#endif	/* __STDC__ || __cplusplus */


/*
 * OlGetRes module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern char		OlGetResolution(Screen* screen);

#else	/* __STDC__ || __cplusplus */

extern char		OlGetResolution();

#endif	/* __STDC__ || __cplusplus */


/*
 * OpenLook module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern int		OlIsFMRunning(Display* dpy, Screen* scr);
extern int		OlIsWMRunning(Display* dpy, Screen* scr);
extern int		OlIsWSMRunning(Display* dpy, Screen* scr);

#else	/* __STDC__ || __cplusplus */

extern int		OlIsFMRunning();
extern int		OlIsWMRunning();
extern int		OlIsWSMRunning();

#endif	/* __STDC__ || __cplusplus */


/*
 * Packed module
 */

#if	defined(__STDC__) || defined(__cplusplus)

/* Create a single Packed Widget */
extern Widget		OlCreatePackedWidget(OlPackedWidget* packed_widget);

/* Create multiple packed widgets */
extern Widget		OlCreatePackedWidgetList(
	OlPackedWidgetList packed_widget_list,  Cardinal num_of_elements);

#else	/* __STDC__ || __cplusplus */

extern Widget		OlCreatePackedWidget();
extern Widget		OlCreatePackedWidgetList();

#endif	/* __STDC__ || __cplusplus */


/*
 * RootShell module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		OlGetApplicationValues(
	Widget		w,		/* widget to get display */
	ArgList		args,		/* args to be fetched */
	Cardinal	num_args	/* number of args */
);

extern void		OlSetApplicationValues(
	Widget		w,		/* widget to get display */
	ArgList		args,		/* args to be set */
	Cardinal	num_args	/* number of args */
);

extern OlDefine		OlQueryAcceleratorDisplay(
	Widget		w		/* any widget */
);

extern OlDefine		OlQueryMnemonicDisplay(
	Widget		w		/* any widget */
);

extern void		OlToolkitInitialize(XtPointer param);

#else	/* __STDC__ || __cplusplus */

extern void		OlGetApplicationValues();
extern void		OlSetApplicationValues();
extern OlDefine		OlQueryAcceleratorDisplay();
extern OlDefine		OlQueryMnemonicDisplay();
extern void		OlToolkitInitialize();

#endif	/* __STDC__ || __cplusplus */


/*
 * Traversal module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern Boolean		OlCallAcceptFocus(Widget w, Time time);
extern Boolean		OlCanAcceptFocus(Widget w, Time time);

extern Widget		OlGetCurrentFocusWidget(
	Widget		w	/* widget of the Base window for this widget */
);

/* Note: this function is obsolete.  Use OlGetCurrentFocusWidget() */
extern Boolean		OlHasFocus(
	Widget		w	/* widget to query */
);

extern Widget		OlMoveFocus(
	Widget		w,		/* Starting point */
	OlVirtualName	direction, 	/* direction of movement */
	Time		time		/* time of request */
);

extern void		OlSetInputFocus(
	Widget		w,		/* widget to get focus */
	int		revert_to,	/* See XLib documentation */
	Time		time		/* time or CurrentTime */
);

#else	/* __STDC__ || __cplusplus */

extern Boolean		OlCallAcceptFocus();
extern Boolean		OlCanAcceptFocus();
extern Widget		OlGetCurrentFocusWidget();
extern Boolean		OlHasFocus();
extern Widget		OlMoveFocus();
extern void		OlSetInputFocus();

#endif	/* __STDC__ || __cplusplus */


/*
 * Vendor module
 */

#if	defined(__STDC__) || defined(__cplusplus)

extern void		OlAddCallback(Widget widget, String name,
	XtCallbackProc callback, XtPointer closure);

extern void		OlCallCallbacks(Widget widget, String name,
	XtPointer call_data);

extern XtCallbackStatus	OlHasCallbacks(Widget widget, String callback_name);

extern void		OlRemoveCallback(Widget widget, String name,
	XtCallbackProc callback, XtPointer closure);

extern void		OlWMProtocolAction(Widget w, OlWMProtocolVerify* st, 
	OlDefine action);

#else	/* __STDC__ || __cplusplus */

extern void		OlAddCallback();
extern void		OlCallCallbacks();
extern XtCallbackStatus	OlHasCallbacks();
extern void		OlRemoveCallback();
extern void		OlWMProtocolAction();

#endif	/* __STDC__ || __cplusplus */


/*
 * Virtual module
 */

#if	defined(__STDC__) || defined(__cplusplus)

/* 
 * Convert a string containing virtual expressions into into a standard
 * Xt translation string
 */
extern char*		OlConvertVirtualTranslation(char* translation);

#else	/* __STDC__ || __cplusplus */

extern char*		OlConvertVirtualTranslation();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_OPENLOOK_H */
