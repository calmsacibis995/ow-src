#ifndef _XOL_DATUM_H
#define _XOL_DATUM_H

#pragma	ident	"@(#)Datum.h	1.5	92/12/10 include/Xol SMI"	/* OLIT	493 */

/*
 *	Copyright (C) 1992  Sun Microsystems, Inc
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


/************************************************************************
 *
 *      Interface of the Datum module
 *
 ************************************************************************/


/************************************************************************
 *
 *      Imported Interfaces
 *
 ************************************************************************/

#include <Xol/OpenLook.h>	/* OlStr, OlGlyph */
#include <Xol/FileCh.h>		/* OlFileNavNode, OlStat */


#ifdef	__cplusplus
extern "C" {
#endif


/************************************************************************
 *
 *      Module Public Type Definitions
 *
 ************************************************************************/

typedef enum UUOlDatumType {
	_OL_DATUM_TYPE_NULL,
	_OL_DATUM_TYPE_STRING,
	_OL_DATUM_TYPE_STR,
	_OL_DATUM_TYPE_FNAVNODE,
	_OL_DATUM_TYPE_STAT,
	_OL_DATUM_TYPE_GLYPH
} _OlDatumType;

typedef struct UUOlDatumRec {
	_OlDatumType	type;
	void*		content;
} *_OlDatum, _OlDatumRec;

typedef int	(*StringComparisonFunc)(const char*const left_string,
	const char*const right_string);


/************************************************************************
 *
 *      Module Public Type Definitions
 *
 ************************************************************************/

typedef int	(*_OlDatumComparisonFunc)(const _OlDatum left_datum, 
	const _OlDatum right_datum);

typedef void	(*_OlDatumTraversalFunc)(const _OlDatum datum, void* arg1, 
	void* arg2, void* arg3, void* arg4);


/************************************************************************
 *
 *      External Module Interface Declarations
 *
 ************************************************************************/

extern void	_OlDatumConstruct(_OlDatum* new_datump, const _OlDatum bufferp);

extern void	_OlDatumDestruct(_OlDatum* datump);

extern int	_OlDatumCompare(const _OlDatum left_datum,
	const _OlDatum right_datum, _OlDatumComparisonFunc datum_cmp);

#ifdef	DATUM_MERGE
extern void	_OlDatumMerge(_OlDatum datum, _OlDatum* datump);
#endif

/************************************************************************
 *
 *      String type
 *
 ************************************************************************/

extern void	_OlStringConstruct(String* new_stringp, const char*const string);

extern void	_OlStringDestruct(String* stringp);

extern int	_OlStringCompare(const char*const left_string,
	const char*const right_string, StringComparisonFunc string_cmp);


/************************************************************************
 *
 *      OlStr type
 *
 ************************************************************************/

extern void	_OlStrSetMode(OlStrRep text_format);

extern void	_OlStrConstruct(OlStr* new_strp, const OlStr str);

extern void	_OlStrDestruct(OlStr* strp);

extern int	_OlStrCompare(const OlStr left_str, const OlStr right_str, 
	OlStrComparisonFunc str_cmp);


/************************************************************************
 *
 *      OlFNavNode type
 *
 ************************************************************************/

extern void	_OlFNavNodeConstruct(OlFNavNode* newp, const OlFNavNode request);

extern void	_OlFNavNodeDestruct(OlFNavNode* node);

extern int	_OlFNavNodeCompare(const OlFNavNode left,
	const OlFNavNode right, StringComparisonFunc string_cmp);


/************************************************************************
 *
 *      OlStat type
 *
 ************************************************************************/

extern void	_OlStatConstruct(OlStat* c_new, const OlStat request);

extern void	_OlStatDestruct(OlStat* sbufpp);


/************************************************************************
 *
 *      OlGlyph type
 *
 ************************************************************************/

extern void	_OlGlyphConstruct(OlGlyph* glyphp, const OlGlyph request);

extern void	_OlGlyphDestruct(OlGlyph* glyphp);


/************************************************************************
 *
 *      debugging support
 *
 ************************************************************************/

#ifdef	DEBUG
extern void	_OlDatumPrint(const _OlDatum datum);

extern void	_OlStringPrint(const char* const string);

extern void	_OlFNavNodePrint(const OlFNavNode node);

extern void	_OlStatPrint(const OlStat sbufp);

extern void	_OlGlyphPrint(const OlGlyph glyph);

extern void	_OlDatumTest(void);
#endif	/* DEBUG */


#ifdef	__cplusplus
}
#endif


/* end of Datum.h */
#endif	/* _XOL_DATUM_H */
