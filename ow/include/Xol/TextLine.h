#ifndef _XOL_TEXTLINE_H
#define _XOL_TEXTLINE_H
#pragma ident	"@(#)TextLine.h	1.3	92/10/08 lib/libXol SMI"	/* OLIT	*/

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

#include	<X11/Intrinsic.h>
#include	<Xol/Primitive.h>

#ifdef  __cplusplus
extern "C" {
#endif

/* Public #defines for positions used by OlTLGetPosition */
#define		OL_CURSORPOS		(-1)
#define		OL_BEGIN_CURRENT_WORD	(-2)
#define		OL_END_CURRENT_WORD	(-3)
#define		OL_END_LINE		(-4)

typedef struct _TextLineRec           *TextLineWidget;
typedef struct _TextLineClassRec      *TextLineWidgetClass;

extern WidgetClass	textLineWidgetClass;

/* TextLine Calldata structures */

/* for PreModify verification */
typedef struct
{
	int             reason;
	XEvent          *event;
	Boolean         valid;
	int             current_cursor;
	int             new_cursor;
	OlStr		buffer;
	int             start;
	int             replace_length;
	OlStr           insert_buffer;
	int             insert_length;
} OlTLPreModifyCallbackStruct;

/* for PostModify verification */
typedef struct
{
	int             reason;
	XEvent          *event;
	int 		cursor;
	OlStr		buffer;
} OlTLPostModifyCallbackStruct;

/* for Motion Verification */
typedef struct
{
	int             reason;
	XEvent          *event;
	Boolean         valid;
	int             current_cursor;
	int             new_cursor;
} OlTLMotionCallbackStruct;

/* for Commit Verification */
typedef struct
{
	int             reason;
	XEvent          *event;
	Boolean         valid;
	OlStr           buffer;
	int		length;
} OlTLCommitCallbackStruct;

/* Public Function prototypes */

#if defined(__STDC__) || defined(__cplusplus)

extern int OlTLGetPosition(Widget w, int position);
extern OlStr OlTLGetSubString(Widget w, int start, int length);
extern Boolean OlTLSetSubString(Widget w, int start, 
				int length, OlStr string);
extern Boolean OlTLSetSelection (Widget w, int start, int length);
extern OlStr OlTLGetSelection (Widget w, int *start, int *length);
extern Boolean OlTLOperateOnSelection(Widget w, int mode);

#else   /* __STDC__ || __cplusplus */

extern int OlTLGetPosition();
extern OlStr OlTLGetSubString();
extern Boolean OlTLSetSubString();
extern Boolean OlTLSetSelection ();
extern OlStr OlTLGetSelection ();
extern Boolean OlTLOperateOnSelection();

#endif  /* __STDC__ || __cplusplus */

#ifdef  __cplusplus
}
#endif

#endif  /* _XOL_TEXTLINE_H */
