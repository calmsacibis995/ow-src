#ifndef	_XOL_TEXTEDIT_H
#define	_XOL_TEXTEDIT_H

#pragma	ident	"@(#)TextEdit.h	302.7	94/04/05 include/Xol SMI"	/* textedit:TextEdit.h 1.4 	*/

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

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <Xol/Dynamic.h>
#include <Xol/OpenLook.h>
#include <Xol/buffutil.h>
#include <Xol/textbuff.h>
#include <Xol/Oltextbuff.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef Dimension*	TabTable;

typedef struct {
	Boolean			ok;
	TextPosition		current_cursor;
	TextPosition		new_cursor;
	TextPosition		select_start;
	TextPosition		select_end;
}			 OlTextMotionCallData,* OlTextMotionCallDataPointer;

typedef struct {
	Boolean                 ok;
	TextPosition            current_cursor;
	TextPosition            select_start;
	TextPosition            select_end;
	TextPosition            new_cursor;
	TextPosition            new_select_start;
	TextPosition            new_select_end;
	char                   *text; /* a generic pointer; must cast to
					 actual type depending on text_format of
					 widget */
	int                     text_length; /* number of bytes for multi-byte
						and number of chars for wide char */

} OlTextModifyCallData, *OlTextModifyCallDataPointer;

typedef struct {
	Boolean                 requestor;
	TextPosition            new_cursor;
	TextPosition            new_select_start;
	TextPosition            new_select_end;
	char                   *inserted;/* a generic pointer; must cast to
                                         actual type depending on text_format of
                                         widget */
	char                   *deleted;/* a generic pointer; must cast to
                                         actual type depending on text_format of
                                         widget */
	TextLocation            delete_start;
	TextLocation            delete_end;
	TextLocation            insert_start;
	TextLocation            insert_end;
	TextPosition            cursor_position;
} OlTextPostModifyCallData, *OlTextPostModifyCallDataPointer;

typedef enum {
	OL_MARGIN_EXPOSED, OL_MARGIN_CALCULATED
}			 OlTextMarginHint;

typedef struct {
	OlTextMarginHint	hint;
	XRectangle*		rect;
}			OlTextMarginCallData,* OlTextMarginCallDataPointer;

#undef OL_TEXT_READ
#undef OL_TEXT_EDIT

typedef enum {
	OL_TEXT_EDIT = 66, OL_TEXT_READ = 67
} OlEditMode;

#undef OL_WRAP_OFF
#undef OL_WRAP_ANY
#undef OL_WRAP_WHITE_SPACE

typedef enum {
	OL_WRAP_OFF = 74, OL_WRAP_ANY = 75, OL_WRAP_WHITE_SPACE = 76
} OlWrapMode;

#undef OL_STRING_SOURCE
#undef OL_DISK_SOURCE
#undef OL_TEXT_BUFFER_SOURCE
#undef OL_OLTEXT_BUFFER_SOURCE

typedef enum { 
	OL_DISK_SOURCE=15, OL_STRING_SOURCE=64, OL_TEXT_BUFFER_SOURCE=155,
	OL_OLTEXT_BUFFER_SOURCE=156
} OlSourceType;

typedef XtPointer			OlTextSource;

typedef struct _TextEditClassRec*	TextEditWidgetClass;
typedef struct _TextEditRec*		TextEditWidget;

extern WidgetClass			textEditWidgetClass;


#if	defined(__STDC__) || defined(__cplusplus)

extern Boolean		OlTextEditClearBuffer(TextEditWidget ctx);

extern Boolean		OlTextEditCopyBuffer(TextEditWidget ctx, char** buffer);

extern Boolean		OlTextEditCopySelection(TextEditWidget ctx,
	int c_delete);

extern Boolean		OlTextEditGetCursorPosition(TextEditWidget ctx,
	TextPosition*  start, TextPosition*  end,
	TextPosition*  cursorPosition);

extern Boolean		OlTextEditGetLastPosition(TextEditWidget ctx, 
	TextPosition*  position);

extern Boolean		OlTextEditInsert(TextEditWidget ctx, char *buffer,
	int length);

extern void		OlTextEditMoveDisplayPosition(TextEditWidget ctx,
	OlInputEvent move_type);

extern Boolean		OlTextEditPaste(TextEditWidget ctx);

extern Boolean		OlTextEditReadSubString(TextEditWidget ctx,
	char **buffer, TextPosition start, TextPosition end);

extern Boolean		OlTextEditRedraw(TextEditWidget ctx);

extern Boolean		OlTextEditSetCursorPosition(TextEditWidget ctx,
	TextPosition start, TextPosition end, TextPosition cursorPosition);

extern TextBuffer*	OlTextEditTextBuffer(TextEditWidget ctx);

extern OlTextBufferPtr	OlTextEditOlTextBuffer(TextEditWidget ctx);

extern Boolean		OlTextEditUpdate(TextEditWidget ctx, Boolean state);

extern void		OlTextEditResize(TextEditWidget ctx,
	int request_linesVisible, int request_charsVisible);

#else	/* __STDC__ || __cplusplus */

extern Boolean		OlTextEditClearBuffer();
extern Boolean		OlTextEditCopyBuffer();
extern Boolean		OlTextEditCopySelection();
extern Boolean		OlTextEditGetCursorPosition();
extern Boolean		OlTextEditGetLastPosition();
extern Boolean		OlTextEditInsert();
extern void		OlTextEditMoveDisplayPosition();
extern Boolean		OlTextEditPaste();
extern Boolean		OlTextEditReadSubString();
extern Boolean		OlTextEditRedraw();
extern Boolean		OlTextEditSetCursorPosition();
extern TextBuffer*	OlTextEditTextBuffer();
extern OlTextBufferPtr	OlTextEditOlTextBuffer();
extern Boolean		OlTextEditUpdate();
extern void		OlTextEditResize();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_TEXTEDIT_H */
