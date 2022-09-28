#ifndef	_XOL_TEXTBUFF_H
#define	_XOL_TEXTBUFF_H

#pragma	ident	"@(#)textbuff.h	302.3	93/11/03 include/Xol SMI"	/* olmisc:textbuff.h 1.7 	*/

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


#include <stdio.h>

#include <X11/Intrinsic.h>

#include <Xol/OpenLook.h>
#include <Xol/buffutil.h>
#include <Xol/txtbufCA.h>
#include <Xol/txtbufCNA.h>


#ifdef	__cplusplus
extern "C" {
#endif


/* 
 * Public definitions
 */

typedef struct {
	TextPosition		bytes;
	TextLine		lines;
	TextPage		qpos;
	BlockTable*		dpos;
}			Page;

typedef struct {
	TextPage		pageindex;
	Buffer*			buffer;
	unsigned long		userData;
}			Line;

typedef Bufferof(Page) PageTable;

typedef	Bufferof(Line) LineTable;

typedef struct _TextUndoItem {
	char*			string;
	TextLocation		start;
	TextLocation		end;
	TextUndoHint		hint;
}			TextUndoItem;

typedef struct _TextBuffer TextBuffer;


#define TextBufferUserData(text,line)	(text->lines.p[line].userData)
#define TextBufferName(text)		(text->filename)
#define TextBufferModified(text)	(text->dirty)

#define TextBufferEmpty(text)		(text->lines.used == 1 && \
	text->lines.p[0].buffer-> used == 1)
                                        
#define TextBufferNamed(text)		(text->filename != NULL)
#define LinesInTextBuffer(text)		(text->lines.used)
#define LastTextBufferLine(text)	(text->lines.used - 1)

#define LastCharacterInTextBufferLine(text, line) \
	(text->lines.p[line].buffer->used - 1)
   
#define LengthOfTextBufferLine(text, line) (text->lines.p[line].buffer->used)

#define SameTextLocation(x,y)		(x.line == y.line && x.offset == y.offset)


#ifdef	__STDC__

typedef Boolean		(*OlStrWordDefProc)(int rc);

typedef char *		(*OlStrScanDefProc)(char* string, char* curp,
			 char* expression);

#else	/* __STDC__ */

typedef Boolean		(*OlStrWordDefProc)();
typedef char *		(*OlStrScanDefProc)();

#endif	/* __STDC__ */


#ifdef	__STDC__

extern TextBuffer*	AllocateTextBuffer(char* filename,
	TextUpdateFunction update_func, caddr_t data);

extern void		FreeTextBuffer(TextBuffer*  text,
	TextUpdateFunction update_func, caddr_t data);

extern TextBuffer*	ReadStringIntoTextBuffer(char* string,
	TextUpdateFunction update_func, caddr_t data);

extern TextBuffer*	ReadFileIntoTextBuffer(char* filename,
	TextUpdateFunction update_func, caddr_t data);

extern char*		GetTextBufferLocation(TextBuffer* text,
	TextLine line_number, TextLocation*  location);

extern ScanResult	ForwardScanTextBuffer(TextBuffer* text,
	TextLocation*  location, char* exp);

extern ScanResult	BackwardScanTextBuffer(TextBuffer* text, char* exp,
	TextLocation*  location);

extern EditResult	ReplaceCharInTextBuffer(TextBuffer*  text,
	TextLocation*  location, int c, TextUpdateFunction update_func,
	caddr_t data);

extern EditResult	ReplaceBlockInTextBuffer(TextBuffer*  text,
	TextLocation*  startloc, TextLocation*  endloc, char* string,
	TextUpdateFunction update_func, caddr_t data);

extern TextLocation	LocationOfPosition(TextBuffer*  text,
	TextPosition position);

extern int		LineOfPosition(TextBuffer*  text,
	TextPosition position);

extern TextPosition	PositionOfLine(TextBuffer*  text, TextLine lineindex);
extern TextLocation	LastTextBufferLocation(TextBuffer*  text);
extern TextPosition	LastTextBufferPosition(TextBuffer*  text);

extern TextPosition	PositionOfLocation(TextBuffer*  text,
	TextLocation location);

extern char*		GetTextBufferLine(TextBuffer*  text,
	TextLine lineindex);

extern int		GetTextBufferChar(TextBuffer*  text,
	TextLocation location);

extern int		CopyTextBufferBlock(TextBuffer*  text, char* buffer,
	TextPosition start_position, TextPosition end_position);

extern char*		GetTextBufferBlock(TextBuffer*  text,
	TextLocation start_location, TextLocation end_location);

extern SaveResult	SaveTextBuffer(TextBuffer*  text, char* filename);
extern Buffer*		GetTextBufferBuffer(TextBuffer*  text, TextLine line);

extern TextLocation	IncrementTextBufferLocation(TextBuffer*  text,
	TextLocation location, TextLine line, TextPosition offset);

extern TextLocation	PreviousLocation(TextBuffer*  textBuffer,
	TextLocation current);

extern TextLocation	NextLocation(TextBuffer*  textBuffer,
	TextLocation current);

extern TextLocation _NextLocationWithoutWrap(TextBuffer*  textBuffer,
	TextLocation current);

extern TextLocation	StartCurrentTextBufferWord(TextBuffer*  textBuffer,
	TextLocation current);

extern TextLocation	EndCurrentTextBufferWord(TextBuffer*  textBuffer,
	TextLocation current);

extern TextLocation	PreviousTextBufferWord(TextBuffer*  textBuffer,
	TextLocation current);

extern TextLocation	NextTextBufferWord(TextBuffer*  textBuffer,
	TextLocation current);

extern void		RegisterTextBufferUpdate(TextBuffer*  text,
	TextUpdateFunction update_func, caddr_t data);

extern int		UnregisterTextBufferUpdate(TextBuffer*  text,
	TextUpdateFunction update_func, caddr_t data);

extern void		RegisterTextBufferScanFunctions(
	OlStrScanDefProc forward_scan_func, OlStrScanDefProc backward_scan_func);

extern void		RegisterTextBufferWordDefinition(
	OlStrWordDefProc word_definition_func);

#else	/* __STDC__ */

extern TextBuffer*	AllocateTextBuffer();
extern void		FreeTextBuffer();
extern TextBuffer*	ReadStringIntoTextBuffer();
extern TextBuffer*	ReadFileIntoTextBuffer();
extern char*		GetTextBufferLocation();
extern ScanResult	ForwardScanTextBuffer();
extern ScanResult	BackwardScanTextBuffer();
extern EditResult	ReplaceCharInTextBuffer();
extern EditResult	ReplaceBlockInTextBuffer();
extern TextLocation	LocationOfPosition();
extern int		LineOfPosition();
extern TextPosition	PositionOfLine();
extern TextLocation	LastTextBufferLocation();
extern TextPosition	LastTextBufferPosition();
extern TextPosition	PositionOfLocation();
extern char*		GetTextBufferLine();
extern int		GetTextBufferChar();
extern int		CopyTextBufferBlock();
extern char*		GetTextBufferBlock();
extern SaveResult	SaveTextBuffer();
extern Buffer*		GetTextBufferBuffer();
extern TextLocation	IncrementTextBufferLocation();
extern TextLocation	PreviousLocation();
extern TextLocation	NextLocation();
extern TextLocation _NextLocationWithoutWrap();
extern TextLocation	StartCurrentTextBufferWord();
extern TextLocation	EndCurrentTextBufferWord();
extern TextLocation	PreviousTextBufferWord();
extern TextLocation	NextTextBufferWord();
extern void		RegisterTextBufferUpdate();
extern int		UnregisterTextBufferUpdate();
extern void		RegisterTextBufferScanFunctions();
extern void		RegisterTextBufferWordDefinition();

#endif	/* __STDC__ */


/*
 * Private definitions
 */
 
/* TextBuffer table sizes and increments */
#define PQLIMIT        16

struct _TextBuffer {
	char*			filename;
	FILE*			tempfile;
	TextBlock		blockcnt;
	TextBlock		blocksize;
	LineTable		lines;
	PageTable		pages;
	BlockTable*		free_list;
	PageQueue		pqueue[PQLIMIT];
	TextPage		pagecount;
	TextPage		pageref;
	TextPage		curpageno;
	Buffer*			buffer;
	char			dirty;
	TextFileStatus		status;
	int			refcount;
	TextUpdateCallback*	update;
	TextUndoItem		deleted;
	TextUndoItem		insert;
};


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_TEXTBUFF_H */
