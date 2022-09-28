#ifndef	_XOL_TEXTWRAP_H
#define	_XOL_TEXTWRAP_H

#pragma	ident	"@(#)TextWrap.h	302.4	92/08/11 include/Xol SMI"	/* textedit:TextWrap.h 1.1 	*/

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


#include <X11/Intrinsic.h>

#include <Xol/OlStrMthdsI.h>
#include <Xol/OpenLook.h>
#include <Xol/TextEditP.h>
#include <Xol/textbuff.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef int		(*_OlNonPrintableCharFunc)(int rc);

extern void		OlTextEditSetNonPrintableCharFunc(
	_OlNonPrintableCharFunc non_printable_char_func);

extern TextLocation	_FirstWrapLine(WrapTable* wrap_tbl);

extern TextLocation	_LastDisplayedWrapLine(WrapTable* wrap_tbl, int page);

extern TextLocation	_LastWrapLine(WrapTable* wrap_tbl);

extern int		_LineNumberOfWrapLocation(WrapTable* wrap_tbl, 
	TextLocation location);

extern int		_NumberOfWrapLines(WrapTable* wrap_tbl);

extern void		_UpdateWrapTable(TextEditWidget ctx, TextLine start, 
	TextLine end);

extern void		_BuildWrapTable(TextEditWidget ctx);

extern OlStr _GetNextWrappedLine(TextBuffer* textBuffer, WrapTable* wrap_tbl, 
	TextLocation* location);

extern TextLocation	_IncrementWrapLocation(WrapTable* wrap_tbl,
	TextLocation current, int n, TextLocation limit, int* reallines);

extern TextLocation	_MoveToWrapLocation(WrapTable* wrap_tbl,
	TextLocation current, TextLocation limit, int* reallines);

extern TextPosition	_PositionOfWrapLocation(TextBuffer* textBuffer,
	WrapTable* wrap_tbl, TextLocation location);

extern TextLocation	_LocationOfWrapLocation(WrapTable* wrap_tbl,
	TextLocation location);

extern TextLocation	_WrapLocationOfPosition(TextBuffer* textBuffer,
	WrapTable* wrap_tbl, TextPosition position);

extern TextLocation	_WrapLocationOfLocation(WrapTable* wrap_tbl,
	TextLocation location);

extern TextPosition	_WrapLineOffset(WrapTable* wrap_tbl,
	TextLocation location);

extern UnitPosition	_WrapLineUnitOffset(TextBuffer*, WrapTable*,
	TextLocation);

extern TextPosition	_WrapLineLength(TextBuffer* textBuffer,
	WrapTable* wrap_tbl, TextLocation location);

extern UnitPosition	_WrapLineNumUnits(TextBuffer* place_holder1,
	WrapTable* place_holder2, TextLocation place_holder3);

extern TextPosition	_WrapLineEnd(TextBuffer* textBuffer,
	WrapTable* wrap_tbl, TextLocation location);

extern void		_StringOffsetAndPosition(OlStr s, TextPosition start,
	TextPosition end, OlFont fs, OlStrRep place_holder, TabTable tabs,
	int minx, int* retx, TextPosition* reti);

extern int		_StringWidth(int x, OlStr s, TextPosition start, 
	TextPosition end, OlFont fs, OlStrRep place_holder, TabTable tabs);

extern int		_CharWidth(int ic, XFontStruct* fs,
	XCharStruct* per_char, int min, int max, int maxwidth);

extern int          	_CharWidthMB(char* place_holder1, 
	XFontSet place_holder2);

extern int          	_CharWidthWC(wchar_t place_holder1, 
	XFontSet place_holder2);

extern int		_NextTabFrom(int x, TabTable tabs, int maxwidth);

extern void		_DrawWrapLine(int x, int startx, int maxx, OlStr p,
	UnitPosition offset, UnitPosition len, OlFont fs, TabTable tabs,
	OlWrapMode mode, Display* dpy, Window win, GC normal, GC select, int y,
	int ascent, int fontht, UnitPosition current, UnitPosition start,
 	UnitPosition end, Pixel bg, Pixel fg, int xoffset, Boolean sensitive,
	PreEditRec *);

extern void		_DrawWrapLineMB(int x, int startx, int maxx, OlStr p,
	UnitPosition offset, UnitPosition len, OlFont fs, TabTable tabs,
	OlWrapMode mode, Display* dpy, Window win, GC normal, GC select, int y,
	int ascent, int fontht, UnitPosition current, UnitPosition start,
 	UnitPosition end, Pixel bg, Pixel fg, int xoffset, Boolean sensitive,
	PreEditRec* );

extern void		_DrawWrapLineWC(int x, int startx, int maxx, OlStr p,
	UnitPosition offset, UnitPosition len, OlFont fs, TabTable tabs,
	OlWrapMode mode, Display* dpy, Window win, GC normal, GC select, int y,
	int ascent, int fontht, UnitPosition current, UnitPosition start,
 	UnitPosition end, Pixel bg, Pixel fg, int xoffset, Boolean sensitive,
	PreEditRec* );

extern TextPosition	_WrapLine(int x, int maxx, OlStr p, TextPosition offset,
	UnitPosition* place_holder, OlFont fs, TabTable tabs, OlWrapMode mode);

extern TextPosition	_WrapLineMB(int x, int maxx, OlStr p, TextPosition offset,
	UnitPosition* place_holder, OlFont fs, TabTable tabs, OlWrapMode mode);

extern TextPosition	_WrapLineWC(int x, int maxx, OlStr p, TextPosition offset,
	UnitPosition* place_holder, OlFont fs, TabTable tabs, OlWrapMode mode);

extern Boolean		_UpdateWrapTableAbove(TextEditWidget 	ctx,
					      Boolean		do_double);

extern Boolean		_UpdateWrapTableBelow(TextEditWidget	ctx,
					      Boolean		do_double);

extern TextLocation	_IncrementWrapLocByTextDelta(TextEditWidget	ctx,
						     TextLocation	current,
						     int *		text_delta,
						     int *		reallines,
						     Boolean		atleast_1line);

#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_TEXTWRAP_H */
