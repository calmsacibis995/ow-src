#ifndef	_XOL_TXTBUFCA_H
#define	_XOL_TXTBUFCA_H

#pragma	ident	"@(#)txtbufCA.h	302.3	92/10/12 include/Xol SMI"	/* OLIT	*/

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


#include <X11/Intrinsic.h>

#include <Xol/buffutil.h>


#ifdef	__cplusplus
extern "C" {
#endif


/* standard types */

typedef int		TextPosition;
typedef int		UnitPosition;	/* Unit is char for SB and MB and */
					/* wchar_t for WC */
typedef int		TextLine;
typedef int		TextPage;
typedef int		TextBlock;

typedef struct _TextLocation {
	TextLine		line;
	TextPosition		offset;
	BufferElement*		buffer;
}			TextLocation;

/* status of a text buffer file */
typedef enum {
	NOTOPEN, READWRITE, READONLY, NEWFILE
}			TextFileStatus;

/* status of edit operations */
typedef enum {
	EDIT_FAILURE, EDIT_SUCCESS
}			EditResult;


/* Scan routine return values and direction definitions */
typedef enum {
	SCAN_NOTFOUND, SCAN_WRAPPED, SCAN_FOUND, SCAN_INVALID
}			ScanResult;

/* SaveTextBuffer status */
typedef enum {
	SAVE_FAILURE, SAVE_SUCCESS
}			SaveResult;

/* TextEditOperations */
#define TEXT_BUFFER_NOP                (0)
#define TEXT_BUFFER_DELETE_START_LINE  (1L<<0)
#define TEXT_BUFFER_DELETE_START_CHARS (1L<<1)
#define TEXT_BUFFER_DELETE_END_LINE    (1L<<2)
#define TEXT_BUFFER_DELETE_END_CHARS   (1L<<3)
#define TEXT_BUFFER_DELETE_JOIN_LINE   (1L<<4)
#define TEXT_BUFFER_DELETE_SIMPLE      (1L<<5)
#define TEXT_BUFFER_INSERT_SPLIT_LINE  (1L<<6)
#define TEXT_BUFFER_INSERT_LINE        (1L<<7)
#define TEXT_BUFFER_INSERT_CHARS       (1L<<8)

typedef int		TextUndoHint;

#ifdef	__STDC__
typedef void		(*TextUpdateFunction)(XtPointer datum,
					      XtPointer text,
					      EditResult status);
#else
typedef void		(*TextUpdateFunction)();
#endif


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_TXTBUFCA_H */
