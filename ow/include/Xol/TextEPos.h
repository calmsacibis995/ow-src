#ifndef	_XOL_TEXTEPOS_H
#define	_XOL_TEXTEPOS_H

#pragma	ident	"@(#)TextEPos.h	302.3	92/11/06 include/Xol SMI" /*textedit:TextEPos.h 1.2*/

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


#include <Xol/TextEditP.h>
#include <Xol/textbuff.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef enum {OL_CARET, OL_CHAR} PositionType;

extern void		_SetDisplayLocation(TextEditWidget ctx,
	TextEditPart*  text, int diff, int setSB, TextLocation c_new);

extern void		_SetTextXOffset(TextEditWidget ctx,
	TextEditPart*  text, int diff, int setSB);

extern void		_CalculateCursorRowAndXOffset(TextEditWidget ctx,
	int* row, int* xoffset, TextLocation currentDP, TextLocation currentIP);

extern int		_MoveDisplayPosition(TextEditWidget ctx, XEvent* event,
	OlInputEvent direction_amount, TextPosition newpos);

extern void		_MoveDisplay(TextEditWidget ctx, TextEditPart*  text,
	int fontht, int page, TextLocation c_new, int diff, int setSB);

extern void		_MoveDisplayLaterally(TextEditWidget ctx,
	TextEditPart*  text, int diff, int setSB);

extern void		_MoveCursorPosition(TextEditWidget ctx, XEvent* event,
	OlInputEvent direction_amount, TextPosition newpos);

extern void		_MoveCursorPositionGlyph(TextEditWidget ctx,
	TextLocation c_new);

extern void		_ExtendSelection(TextEditWidget ctx,
	OlInputEvent ol_event);

extern Boolean		_MoveSelection(TextEditWidget ctx,
	TextPosition position, TextPosition selectStart,
	TextPosition selectEnd, int selectMode);

extern int		_TextEditOwnPrimary(TextEditWidget ctx, Time time);

extern TextPosition	_PositionFromXY(TextEditWidget ctx, int x, int y,
							PositionType type);

extern XRectangle	_RectFromPositions(TextEditWidget ctx,
	TextPosition start, TextPosition end);

extern TextPosition	_ScrollDisplayByTextPosDelta(TextEditWidget ctx,
 						     int	    text_delta);

#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_TEXTEPOS_H */
