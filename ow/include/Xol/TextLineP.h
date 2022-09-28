#ifndef _XOL_TEXTLINEP_H
#define _XOL_TEXTLINEP_H
#pragma ident	"@(#)TextLineP.h	1.6	92/11/13 lib/libXol SMI"	/* OLIT	*/

/*
 *        Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                    All rights reserved.
 *          Notice of copyright on this source code 
 *          product does not indicate publication. 
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 *
 *    Sun Microsystems, Inc., 2550 Garcia Avenue,
 *    Mountain View, California 94043.
 *
 */

#include 	<Xol/OpenLookP.h>
#include	<Xol/PrimitiveP.h>
#include 	<Xol/OlDnDVCX.h>
#include	<Xol/OlgxP.h>
#include	<Xol/TextLine.h>

#ifdef  __cplusplus
extern "C" {
#endif


/***********************************************************************
 *
 *	Definition Of The Instance Structure 
 *
 ***********************************************************************/

typedef enum { TLCaretInvisible, TLCaretDiamond, TLCaretTriangle } TLCaretMode;
typedef struct _CaretInfo {
        TLCaretMode     caret_state;
	Dimension	caret_width;
	Dimension	caret_height;
	short		caret_ascent;
	short		caret_descent;
	Pixmap		saved_pixmap;
        GC              caret_gc;
} CaretInfo;

typedef struct _TLBuffer {
        int             size;
        int             used;
        char            *p;
} TLBuffer;
typedef TLBuffer PositionTable;

typedef struct _UndoBuffer {
        OlStr           string;    /* deleted string */
        int             start;     /* start pos of insert */
        int             end;      /* end pos AFTER insert */
} UndoBuffer;

typedef enum 	{  OlselectNull, OlselectPosition, OlselectChar, OlselectWord,
    		   OlselectLine, OlselectParagraph, OlselectAll
		} OlSelectType;

typedef struct {
	/* 	Public resources 	*/
	int			cursor_position; 
	OlDefine		edit_type;	
	long			blink_rate;	
	OlStr			caption_label;	
	OlFont			caption_font;
	int			chars_visible;	
	int			initial_delay;
	int			repeat_rate;
	Boolean			insert_tab;
	int			maximum_chars;	
	OlStr 			string;	
	Boolean			update_display;	
	Boolean			underline;
	OlImPreeditStyle 	pre_edit_style;
	Widget			menu;
	XtCallbackList		pre_modify_callback;
	XtCallbackList		post_modify_callback;
	XtCallbackList		motion_callback;
	XtCallbackList		commit_callback;

	/* 	Caption  related resources */
	OlDefine		caption_position;
	Dimension		caption_width;	
	Dimension		caption_height;
	OlDefine		caption_alignment;
	Dimension		caption_space;

	/*	Private resources	*/
	Boolean			caret_visible;	/* sets cursor-visibility */
	int			caret_pos;	/* Position of the insert-caret */
	int 			caret_x;	/* x-coord of caret */
	int			caret_y;	/* y-coord of caret */
	CaretInfo		caret_info;
	Boolean			blink_on;	/* True, when blinker makes caret ON */
	XtIntervalId		blink_timer;

	int			max_char_width; /* Max char_width in our fontset */
	int			last_pos_width;	/* Ref DrawText() */
	int			gap;
	int			num_chars;		

	int			select_start;	/* Start of selected text */
	int 			select_end;	/* End of selected text */
	int			anchor;		/* Anchor for wipethru selection */
	OlSelectType		select_mode;	/* char/word/line mode */
	XtIntervalId		wipethru_timer;	
	unsigned int		mask;

	TLBuffer          	*buffer;	/* text buffer */
	PositionTable   	*pos_table;	/* if format == MB */
	UndoBuffer      	undo_buffer;

	Boolean			preed_on;
	int			preed_start;	/* insert point for preedited text */
	int			preed_caret;	/* preedit caret -controlled by IM */
	int			num_preed_chars; 
	TLBuffer		*preed_buffer;	/* preedit buffer */
	PositionTable		*preed_pos_table; /* if format == MB */
	TLBuffer		*feedback_table; /* feedback info */
	OlInputContextID 	ic_id;

	GC			GCs[3];		/* Normal, Inverse, Caption GCs */
	OlgxAttrs		*pAttrs;

	Boolean 		leftarrow_present;	
	Boolean			rightarrow_present;
	int			char_offset;	/* Character @ left-edge of widget */

	Boolean			my_menu;

	int			scroll_direction;	
	XtIntervalId		scroll_timer;	

	OlStr			clip_contents;	/* Contents of Clipboard */
	Atom			transient;	/* for DnD  ... */
	OlStr			dnd_contents;	/* Dragged data -during DnD */
	OlDnDDropSiteID         dropsite_id;

	unsigned long		redraw;		/* redraw info if !updateDisplay */

	Dimension		real_width;
	Dimension		real_height;
} TextLinePart;


typedef struct _TextLineRec {
	CorePart	core;
	PrimitivePart	primitive;
	TextLinePart	textLine;
} TextLineRec;


/*************************************************************************
 *
 *	Definition Of The Class structure
 *
 *************************************************************************/

typedef enum { TLSetVal, TLInit, NFSetVal, NFInit, NFOther }  OlTLSetStringHints;

typedef Boolean	(*OlTLSetStringProc)(TextLineWidget w, 
				     OlStr string, 
				     OlTLSetStringHints hints,
				     Boolean cursor_set
				    );
#define XtInheritOlTLSetStringProc ((OlTLSetStringProc)_XtInherit)

typedef OlStr 	(*OlTLGetStringProc)(TextLineWidget w);
#define XtInheritOlTLGetStringProc ((OlTLGetStringProc)_XtInherit)


typedef struct _TextLineClassPart {
	OlTLSetStringProc	set_string;
	OlTLGetStringProc	get_string;
	XtPointer		extension;
} TextLineClassPart;

typedef struct _TextLineClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart      primitive_class;
	TextLineClassPart	textLine_class;
} TextLineClassRec;

extern TextLineClassRec textLineClassRec;



#ifdef  __cplusplus
}
#endif

#endif  /* _XOL_TEXTLINEP_H */
