#ifndef	_XOL_EVENTOBJP_H
#define	_XOL_EVENTOBJP_H

#pragma	ident	"@(#)EventObjP.h	302.4	92/10/09 include/Xol SMI"	/* eventobj:EventObjP.h 1.12 	*/

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


#include <Xol/EventObj.h>
#include <Xol/PrimitiveP.h>

#include <X11/RectObjP.h>


#ifdef	__cplusplus
extern "C" {
#endif


/* Macros */

#define	_OlXTrans(w, x_val) \
	((_OlIsGadget(w)) ? ((RectObj)(w))->rectangle.x + (x_val) : (x_val))

#define	_OlYTrans(w, y_val) \
	((_OlIsGadget(w)) ? ((RectObj)(w))->rectangle.y + (y_val) : (y_val))

/* dynamic resources bit masks */
#define OL_B_EVENT_BG		(1 << 0)
#define OL_B_EVENT_FG		(1 << 1)
#define OL_B_EVENT_FONTCOLOR	(1 << 2)
#define OL_B_EVENT_FOCUSCOLOR	(1 << 3)


/*
 *  Event Object Class Data Structures
 */
typedef struct _EventObjClassPart {
	/* fields for Primitive Class equivalence */
	XtPointer		reserved1;	/* obsolete can_accept_focus */
	OlHighlightProc		highlight_handler;
	OlTraversalFunc		traversal_handler;
	OlRegisterFocusFunc	register_focus;
	OlActivateFunc		activate;
	OlEventHandlerList	event_procs;
	Cardinal		num_event_procs;
	XtVersionType		version;
	XtPointer		extension;
	_OlDynData		dyn_data;
	OlTransparentProc	transparent_proc;
	SuperCaretQueryLocnProc	query_sc_locn_proc;
} EventObjClassPart;

typedef struct _EventObjClassRec {
	RectObjClassPart	rect_class;
	EventObjClassPart	event_class;
} EventObjClassRec;


/*
 *  eventObjClassRec is defined in EventObj.c
 */
extern EventObjClassRec	eventObjClassRec;


/*
 *  Event Object Instance Data Structures
 */
typedef struct {
	/* Position dependent: the following fields coincide with CorePart */
	XtEventTable		event_table;

	/* Fields for Primitive Instance equivalence */
	XtPointer		user_data;
	Boolean			traversal_on;
	OlMnemonic		mnemonic;
	String			accelerator;
	String			qualifier_text;
	Boolean			meta_key;
	String			accelerator_text;
	String			reference_name;
	Widget			reference_widget;
	XtCallbackList		consume_event;
	OlStrRep		text_format;
	OlFont			font;
	Pixel			input_focus_color;
	Pixel			font_color;
	Pixel			foreground;
	int			scale;

	/* non-resource related fields */
	Boolean			has_focus;
	unsigned char		dyn_flags;
} EventObjPart;

typedef struct _EventObjRec {
	ObjectPart		object;
	RectObjPart		rectangle;
	EventObjPart		event;
} EventObjRec;


#if	defined(__STDC__) || defined(__cplusplus)

extern void		_OlAddEventHandler(Widget widget, EventMask eventMask,
	Boolean other, XtEventHandler proc, XtPointer closure);

extern void		_OlRemoveEventHandler(Widget widget,
	EventMask eventMask, Boolean other, XtEventHandler proc,
	XtPointer closure);

#else	/* __STDC__ || __cplusplus */

extern void		_OlAddEventHandler();
extern void		_OlRemoveEventHandler();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_EVENTOBJP_H */
