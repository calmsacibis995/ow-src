#ifndef	_XOL_OLTEXTBUFF_H
#define	_XOL_OLTEXTBUFF_H

#pragma	ident	"@(#)Oltextbuff.h	1.2	93/11/03 include/Xol SMI"	/* olmisc:Mltextbuff.h 302.3 */

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

/* Copyright (c) 1990 UNIX System Laboratories, Inc.	 */
/* Copyright (c) 1988 AT&T	 */
/* All Rights Reserved  	 */

/* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	 */
/* UNIX System Laboratories, Inc.                     	 */
/* The copyright notice above does not evidence any   	 */
/* actual or intended publication of such source code.	 */


#include <limits.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>

#include <Xol/OpenLook.h>
#include <Xol/txtbufCA.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct _OlTextBuffer *OlTextBufferPtr;

typedef struct _OlTextUndoItem {
	OlStr			string;
	TextLocation		start;
	TextLocation		end;
	TextUndoHint		hint;
}			OlTextUndoItem;


#ifdef	__STDC__

typedef Boolean		(*OlStrWordDefFunc)(OlStr rc);

typedef XtPointer	(*OlStrScanDefFunc)(OlStr string, OlStr curp,
	OlStr expression);

#else	/* __STDC__ */

typedef Boolean		(*OlStrWordDefFunc)();
typedef XtPointer	(*OlStrScanDefFunc)();

#endif	/* __STDC__ */


#ifdef	__STDC__

extern OlTextBufferPtr	OlAllocateTextBuffer(OlStrRep strrep, char* filename,
	TextUpdateFunction update_func, XtPointer data);

extern int		OlLinesInTextBuffer(OlTextBufferPtr text);

extern void		OlFreeTextBuffer(OlTextBufferPtr text,
	TextUpdateFunction update_func, XtPointer data);

extern OlTextBufferPtr	OlReadStringIntoTextBuffer(OlStrRep strrep, 
	OlStr string, TextUpdateFunction update_func, XtPointer data);

extern OlTextBufferPtr OlReadFileIntoTextBuffer(OlStrRep strrep, char* filename,
	TextUpdateFunction update_func, XtPointer data);

extern OlStr		OlGetTextBufferStringAtLine(OlTextBufferPtr text,
	TextLine line_number, TextLocation* location);

extern ScanResult	OlForwardScanTextBuffer(OlTextBufferPtr text, OlStr exp,
	TextLocation* location);

extern ScanResult	OlBackwardScanTextBuffer(OlTextBufferPtr text, OlStr exp,
	TextLocation* location);

extern EditResult	OlReplaceCharInTextBuffer(OlTextBufferPtr text,
	TextLocation* location, OlStr c, TextUpdateFunction update_func,
	XtPointer data);

extern EditResult	OlReplaceBlockInTextBuffer(OlTextBufferPtr text,
	TextLocation* startloc, TextLocation* endloc, OlStr string,
	TextUpdateFunction update_func, XtPointer data);

extern TextLocation*	OlLocationOfPosition(OlTextBufferPtr text,
	TextPosition position, TextLocation* location);

extern TextLine		OlLineOfPosition(OlTextBufferPtr text,
	TextPosition position);

extern TextPosition	OlPositionOfLine(OlTextBufferPtr text,
	TextLine lineindex);

extern TextLocation*	OlLastTextBufferLocation(OlTextBufferPtr text,
	TextLocation* last);

extern TextPosition	OlLastTextBufferPosition(OlTextBufferPtr text);

extern TextPosition	OlPositionOfLocation(OlTextBufferPtr text,
	TextLocation* location);

extern OlStr		OlGetTextBufferLine(OlTextBufferPtr text,
	TextLine lineindex);

extern OlStr		OlGetTextBufferCharAtLoc(OlTextBufferPtr text,
	TextLocation* location);

extern int		OlCopyTextBufferBlock(OlTextBufferPtr text,
	OlStr outbuffer, int num_bytes, TextPosition start_position,
	TextPosition end_position);

extern OlStr		OlGetTextBufferBlock(OlTextBufferPtr text,
	TextLocation* start_location, TextLocation* end_location);

extern SaveResult	OlSaveTextBuffer(OlTextBufferPtr text, char* filename);
extern Buffer*		OlGetTextBufferBuffer(OlTextBufferPtr text, TextLine line);

extern TextLocation*	OlIncrementTextBufferLocation(OlTextBufferPtr text,
	TextLocation* location, TextLine line, TextPosition offset);

extern TextLocation*	OlPreviousLocation(OlTextBufferPtr text,
	TextLocation* current);

extern TextLocation*	OlNextLocation(OlTextBufferPtr text,
	TextLocation* current);

extern TextLocation*    _OlNextLocationWithoutWrap(OlTextBufferPtr text,
	TextLocation* current);

extern TextLocation*	OlStartCurrentTextBufferWord(OlTextBufferPtr text,
	TextLocation* current);

extern TextLocation*	OlEndCurrentTextBufferWord(OlTextBufferPtr text,
	TextLocation* current);

extern TextLocation*	OlPreviousTextBufferWord(OlTextBufferPtr text,
	TextLocation* current);

extern TextLocation*	OlNextTextBufferWord(OlTextBufferPtr text,
	TextLocation* current);

extern void		OlRegisterTextBufferUpdate(OlTextBufferPtr text,
	TextUpdateFunction update_func, XtPointer data);

extern int		OlUnregisterTextBufferUpdate(OlTextBufferPtr text,
	TextUpdateFunction update_func, XtPointer data);

extern void		OlRegisterAllTextBufferScanFunctions(OlStrRep strrep,
	OlStrScanDefFunc forward_scan_func, OlStrScanDefFunc backward_scan_func);

extern void		OlRegisterPerTextBufferScanFunctions(OlTextBufferPtr text,
	OlStrScanDefFunc forward_scan_func, OlStrScanDefFunc backward_scan_func);

extern void		OlRegisterAllTextBufferWordDefinition(OlStrRep strrep,
	OlStrWordDefFunc word_definition_func);

extern void		OlRegisterPerTextBufferWordDefinition(
	OlTextBufferPtr text, OlStrWordDefFunc word_definition_func);

extern OlTextUndoItem	OlGetTextUndoInsertItem(OlTextBufferPtr text);

extern void		OlSetTextUndoInsertItem(OlTextBufferPtr text,
	OlTextUndoItem text_undo_insert);

extern OlTextUndoItem	OlGetTextUndoDeleteItem(OlTextBufferPtr text);

extern void		OlSetTextUndoDeleteItem(OlTextBufferPtr text,
	OlTextUndoItem text_undo_deleted);

extern UnitPosition	OlUnitPositionOfTextPosition(OlTextBufferPtr text,
	TextPosition pos);

extern UnitPosition	OlUnitOffsetOfLocation(OlTextBufferPtr text,
	TextLocation* loc);

extern TextPosition OlCharOffsetOfUnitLocation(OlTextBufferPtr text,
				TextLocation *loc); /* loc->offset is a unit offset */

extern String		OlGetTextBufferFileName(OlTextBufferPtr text);
extern Boolean		OlIsTextBufferModified(OlTextBufferPtr text);
extern Boolean		OlIsTextBufferEmpty(OlTextBufferPtr text);
extern TextLine		OlLastTextBufferLine(OlTextBufferPtr text);

extern int		OlLastCharInTextBufferLine(OlTextBufferPtr text,
	TextLine line);

extern int		OlNumCharsInTextBufferLine(OlTextBufferPtr text,
	TextLine line);

extern int		OlNumBytesInTextBufferLine(OlTextBufferPtr text,
	TextLine line);
extern int		OlNumUnitsInTextBufferLine(OlTextBufferPtr text,
	TextLine line);

#else	/* __STDC__ */

extern OlTextBufferPtr	OlAllocateTextBuffer();
extern int		OlLinesInTextBuffer();
extern void		OlFreeTextBuffer();
extern OlTextBufferPtr	OlReadStringIntoTextBuffer();
extern OlTextBufferPtr	OlReadFileIntoTextBuffer();
extern OlStr		OlGetTextBufferStringAtLine();
extern ScanResult	OlForwardScanTextBuffer();
extern ScanResult	OlBackwardScanTextBuffer();
extern EditResult	OlReplaceCharInTextBuffer();
extern EditResult	OlReplaceBlockInTextBuffer();
extern TextLocation*	OlLocationOfPosition();
extern TextLine		OlLineOfPosition();
extern TextPosition	OlPositionOfLine();
extern TextLocation*	OlLastTextBufferLocation();
extern TextPosition	OlLastTextBufferPosition();
extern TextPosition	OlPositionOfLocation();
extern OlStr		OlGetTextBufferLine();
extern OlStr		OlGetTextBufferCharAtLoc();
extern int		OlCopyTextBufferBlock();
extern OlStr		OlGetTextBufferBlock();
extern SaveResult	OlSaveTextBuffer();
extern Buffer*		OlGetTextBufferBuffer();
extern TextLocation*	OlIncrementTextBufferLocation();
extern TextLocation*	OlPreviousLocation();
extern TextLocation*	OlNextLocation();
extern TextLocation*    _OlNextLocationWithoutWrap();
extern TextLocation*	OlStartCurrentTextBufferWord();
extern TextLocation*	OlEndCurrentTextBufferWord();
extern TextLocation*	OlPreviousTextBufferWord();
extern TextLocation*	OlNextTextBufferWord();
extern void		OlRegisterTextBufferUpdate();
extern int		OlUnregisterTextBufferUpdate();
extern void		OlRegisterAllTextBufferScanFunctions();
extern void		OlRegisterPerTextBufferScanFunctions();
extern void		OlRegisterAllTextBufferWordDefinition();
extern void		OlRegisterPerTextBufferWordDefinition();
extern OlTextUndoItem	OlGetTextUndoInsertItem();
extern void		OlSetTextUndoInsertItem();
extern OlTextUndoItem	OlGetTextUndoDeleteItem();
extern void		OlSetTextUndoDeleteItem();
extern UnitPosition	OlUnitPositionOfTextPosition();
extern UnitPosition	OlUnitOffsetOfLocation();
extern TextPosition     OlCharOffsetOfUnitLocation();
extern String		OlGetTextBufferFileName();
extern Boolean		OlIsTextBufferModified();
extern Boolean		OlIsTextBufferEmpty();
extern TextLine		OlLastTextBufferLine();
extern int		OlLastCharInTextBufferLine();
extern int		OlNumCharsInTextBufferLine();
extern int		OlNumBytesInTextBufferLine();
extern int		OlNumUnitsInTextBufferLine();

#endif	/* __STDC__ */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_OLTEXTBUFF_H */
