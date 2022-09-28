#ifndef	_XOL_TEXTEDITP_H
#define	_XOL_TEXTEDITP_H

#pragma	ident	"@(#)TextEditP.h	302.15	92/11/13 include/Xol SMI"	/* textedit:TextEditP.h 1.9 	*/

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


#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>

#include <Xol/OpenLookP.h>
#include <Xol/DynamicP.h>
#include <Xol/OlDnDVCX.h>
#include <Xol/PrimitiveP.h>
#include <Xol/TextEdit.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct {
	int			 foo;
}			TextEditClassPart;

typedef struct _TextEditClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	TextEditClassPart	textedit_class;
}			TextEditClassRec;

typedef enum {
	OlCursorOn, OlCursorOff, OlCursorBlinkOff, OlCursorBlinkOn
}			OlCursorState;

typedef struct _TextCursor {
	int			limit;
	int			width;
	int			height;
	int			xoffset;
	int			baseline;
	unsigned char*		inbits;
	unsigned char*		outbits;
}			TextCursor;

typedef Bufferof(int)		WrapLocation;
typedef Bufferof(WrapLocation*)	WrapContents;
typedef Bufferof(XIMFeedback)	XIMFeedbackBuffer;

typedef struct _WrapTableObject {
    TextEditWidget	ctx;
    Boolean		build_wrap;
    OlStrRep		text_format;
    TextLine		cur_start; /* WrapTable extents */
    TextLine		cur_end;
    int			proportion;
    WrapContents*	contents;
}  WrapTable;


typedef struct _DisplayTable {
	OlStr			p;
	int			used;
	int			size; /* number of units in wrap line */
	char			flag;
	TextLocation		wraploc;
	int			nchars; /* num of chars in wrap line */
	XIMFeedback*		pre_edit_feedback; /* feedback info */
	int			feedback_start_offset;  
	int			num_feedback_chars;
}			DisplayTable;

typedef struct {
	TextPosition		displayPosition;
	TextPosition		cursorPosition;
	TextPosition		selectStart;
	TextPosition		selectEnd;
	TextPosition		anchor;
	Dimension		leftMargin;
	Dimension		rightMargin;
	Dimension		topMargin;
	Dimension		bottomMargin;
	OlEditMode		editMode;
	OlWrapMode		wrapMode;
	OlDefine		growMode;
	OlSourceType		sourceType;
	XtPointer		source;		/* OlStr */
	XtCallbackList		motionVerification;
	XtCallbackList		modifyVerification;
	XtCallbackList		postModifyNotification;
	XtCallbackList		margin;
	XtCallbackList		buttons;
	XtCallbackList		keys;
	long			blinkRate;
	Boolean			updateState;
	Boolean			shouldBlink;
	Boolean			insertTab;
	Boolean			insertReturn;
	TabTable		tabs;
	OlRegisterFocusFunc	register_focus;

	/* not in resource table */
	TextLocation		displayLocation;
	TextLocation		cursorLocation;

	/* private information */
	XtPointer		clip_contents;	/* OlStr */
	OlStr			dnd_contents;
	DisplayTable*		DT;
	int			DTsize;
	XtIntervalId		blink_timer;
	int			lineCount;
	int			linesVisible;
	int			charsVisible;
	int			lineHeight;
	int			charWidth;
	int			xOffset;
	int			maxX;
	Widget			vsb;
	Widget			hsb;
	TextBuffer*		textBuffer;
	GC			gc;
	GC			invgc;
	GC			insgc;
	int			save_offset;
	char			selectMode;
	unsigned char		dyn_flags;
	WrapTable*		wrapTable;
	unsigned int		mask;
	unsigned long		dynamic;
	Pixmap			CursorIn;
	Pixmap			CursorOut;
	TextCursor*		CursorP;
	OlCursorState		cursor_state;
	Boolean			hadFocus;
	Position		cursor_x;
	Position		cursor_y;
	char			need_vsb;
	char			need_hsb;
	Dimension		prev_width;
	Dimension		prev_height;
	Boolean			cursor_visible;
	OlDnDDropSiteID		dropsiteid;	/* new - for drag and drop
						   support */
	Atom			transient;
	OlStrRep		text_format;

	/* pre-edit call back related data */
	Boolean			pre_edit;
	TextPosition		pre_edit_start;
	TextPosition		pre_edit_end;
	TextPosition		pre_edit_caret;
	XIMFeedbackBuffer*	pre_edit_feedback;
	GC*			feedbackGCs;
	OlInputContextID	ic_id;
	OlImPreeditStyle        pre_edit_style;

	/* more resources */
	OlStr		menuTitle;
	OlStr		undoLabel;
	OlStr		cutLabel;
	OlStr		copyLabel;
	OlStr		pasteLabel;
	OlStr		deleteLabel;
	char			undoMnemonic;
	char			cutMnemonic;
	char			copyMnemonic;
	char			pasteMnemonic;
	char			deleteMnemonic;

	/* XtWorkProcID for wrap computation */
	XtWorkProcId	wrap_work_proc_id;
}			TextEditPart;

typedef struct _PreEditRec {
	UnitPosition		start;
	UnitPosition		end;
	XIMFeedback*		pre_edit_feedback;
	GC*			feedbackGCs;
}			PreEditRec;

typedef struct _TextEditRec {
	CorePart		core;
	PrimitivePart		primitive;
	TextEditPart		textedit;
}			TextEditRec;


#define ASCENT(widget)        (_OlFontAscent((Widget)widget))
#define DESCENT(widget)       (_OlFontDescent((Widget)widget))
#define	FONTHT(widget)        (ASCENT(widget) + DESCENT(widget))
#define FONTWID(widget)       (_OlFontWidth((Widget)widget))
#define	PAGE_T_GAP(widget)    ((int)(widget-> textedit.topMargin))
#define PAGE_B_GAP(widget)    ((int)(widget-> textedit.bottomMargin))
#define PAGE_R_GAP(widget)    ((int)(widget-> textedit.rightMargin))
#define PAGE_L_GAP(widget)    ((int)(widget-> textedit.leftMargin))
#define PAGE_T_MARGIN(widget) ((int)(widget-> textedit.topMargin))
#define PAGE_L_MARGIN(widget) ((int)(widget-> textedit.leftMargin))
#define PAGE_R_MARGIN(widget) (((int)(widget-> core.width) - PAGE_R_GAP(widget)) \
			<  0 ? 0 : ((int)(widget-> core.width) - PAGE_R_GAP(widget))) 
#define PAGE_B_MARGIN(widget) (((int)(widget-> core.height) - PAGE_B_GAP(widget)) \
			<  0 ? 0 : ((int)(widget-> core.height) - PAGE_B_GAP(widget)))
#define PAGEHT(widget)        ((PAGE_B_MARGIN(widget) - PAGE_T_MARGIN(widget)) \
			< 0  ? 0 : ((int)(widget-> core.height) - PAGE_B_GAP(widget)) - \
				PAGE_T_MARGIN(widget))
#define PAGEWID(widget)       ((PAGE_R_MARGIN(widget) - PAGE_L_MARGIN(widget)) \
			< 0  ? 0 : ((int)(widget-> core.width) - PAGE_R_GAP(widget)) - \
				PAGE_L_MARGIN(widget))
#define PAGE_LINE_HT(widget)  (PAGEHT(widget) / FONTHT(widget))
#define LINES_VISIBLE(widget) (widget-> textedit.linesVisible)
#define HGAP(widget)          (0)
#define HORIZONTAL_SHIFT(widget) (8 * ENSPACE(widget))
#define ENSPACE(widget)       (_FontSetEnspace(widget))

#define PRINTLOC(L,T)   \
		(void)fprintf(stderr,"%s:(%5d, %5d)\n", T, L.line, L.offset)

#define PRINTPOS(P,T)   \
		(void)fprintf(stderr,"%s:(%5d)\n", T, P)

#define PRINTRECT(rect, T) \
(void)fprintf(stderr,"%s: %d,%d %d,%d\n",T,rect.x,rect.y,rect.width,rect.height)

#define PRINTSELECT(text, T) \
(void)fprintf(stderr,"%s: %d to %d\n",T,text-> selectStart, text-> selectEnd)

/* dynamics resources bit masks */
#define OL_B_TEXTEDIT_BG		(1 << 0)
#define OL_B_TEXTEDIT_FONTCOLOR		(1 << 1)

/* Borrowed from DynamicP.h */
#define ABS_DELTA(x1, x2)    (x1 < x2 ? x2 - x1 : x1 - x2)

extern TextEditClassRec		textEditClassRec;


#if	defined(__STDC__) || defined(__cplusplus)

extern void		UpdateDisplay(TextEditWidget ctx, 
				XtPointer genericBuffer, int requestor);

extern void		_OlSetClickMode(int mode);
extern int		_FontSetAscent(TextEditWidget widget);
extern int		_FontSetDescent(TextEditWidget widget);
extern int		_FontSetWidth(TextEditWidget widget);
extern int		_FontSetEnspace(TextEditWidget widget);
extern Boolean		_BGDisplayChange(TextEditWidget ctx);
extern Boolean		_CheckForScrollbarStateChange(TextEditWidget ctx, TextEditPart *text);
	
#else	/* __STDC__ || __cplusplus */

extern void		UpdateDisplay();
extern void		_OlSetClickMode();
extern int		_FontSetAscent();
extern int		_FontSetDescent();
extern int		_FontSetWidth();
extern int		_FontSetEnspace();
extern Boolean		_BGDisplayChange();
extern Boolean		_CheckForScrollbarStateChange();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_TEXTEDITP_H */
