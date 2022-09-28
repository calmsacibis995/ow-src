#pragma ident	"@(#)Datum.c	1.7    93/01/08 lib/libXol SMI"    /* OLIT	493	*/

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
 * Description: implementation of the OLIT generic data objects
 *
 ************************************************************************/


/************************************************************************
 *
 *      Imported interfaces 
 *
 ************************************************************************/

#ifndef	DEBUG
#define	NDEBUG
#endif

#include <assert.h>		/* assert() */
#include <errno.h>		/* error */
#include <libintl.h>		/* dgettext() */
#include <stdio.h>		/* NULL, printf() */
#include <string.h>		/* strcpy(), strcmp() */
#include <sys/stat.h>		/* struct stat */
#include <sys/types.h>		/* *_t */

#include <X11/X.h>		/* Pixmap */
#include <X11/Xlib.h>		/* XImage */
#include <X11/Intrinsic.h>	/* String */

#include <Xol/FileCh.h>		/* OlStat, OlFNavNode */
#include <Xol/OlStrMthdsI.h>
#include <Xol/OpenLook.h>	/* OlStr, OlGlyph, OlGlyphType */
#include <Xol/OpenLookP.h>	/* _OlStrlen() */
#include <Xol/diags.h>

#include <Xol/Datum.h>		/* Interface of this module */


/************************************************************************
 *
 *      Module private global storage allocation
 *
 ************************************************************************/

static OlStrRep		datum_text_format = OL_SB_STR_REP;	
					/*! OL_MB_STR_REP !*/
						/* current text_format */
static OlStrMethods	datum_sm;		/* current string methods */


/************************************************************************
 *
 *      Implementation of this module's public functions
 *
 ************************************************************************/


/************************************************************************
 *
 *      _OlDatumConstruct -- Allocate and initialize an object
 *
 ************************************************************************/

void
_OlDatumConstruct(_OlDatum* new_datump, const _OlDatum request)
{
	
	if (NULL == request) 
		*new_datump = NULL;
	else {
		_OL_MALLOC(*new_datump, _OlDatumRec);
	
		switch (request->type) {

		case _OL_DATUM_TYPE_NULL:
			(*new_datump)->type = _OL_DATUM_TYPE_NULL;
			(*new_datump)->content = (void*)NULL;
			break;

		case _OL_DATUM_TYPE_STRING:
			(*new_datump)->type = _OL_DATUM_TYPE_STRING;
			_OlStringConstruct((String*)&(*new_datump)->content,
				(const char* const)request->content);
			break;

		case _OL_DATUM_TYPE_STR:
			(*new_datump)->type = _OL_DATUM_TYPE_STR;
			_OlStrConstruct((OlStr*)&(*new_datump)->content,
				(OlStr)request->content);
			break;

		case _OL_DATUM_TYPE_FNAVNODE:
			(*new_datump)->type = _OL_DATUM_TYPE_FNAVNODE;
			_OlFNavNodeConstruct((OlFNavNode*)&(*new_datump)->content,
				(OlFNavNode)request->content);
			break;

		case _OL_DATUM_TYPE_STAT:
			(*new_datump)->type = _OL_DATUM_TYPE_STAT;
			_OlStatConstruct((OlStat*)&(*new_datump)->content,
				(OlStat)request->content);
			break;

		case _OL_DATUM_TYPE_GLYPH:
			(*new_datump)->type = _OL_DATUM_TYPE_GLYPH;
			_OlGlyphConstruct((OlGlyph*)&(*new_datump)->content,
				(OlGlyph)request->content);
			break;

		default:
			_OlAbort(NULL, dgettext(OlMsgsDomain, 
				"_OlDatumConstruct():  unsupported data type "
				"%d.\n"), &request->type);
			/*NOTREACHED*/
			break;
		}
	}
} /* end of _OlDatumConstruct() */


/************************************************************************
 *
 *      _OlDatumDestruct -- Destruct the object
 *
 ************************************************************************/

void
_OlDatumDestruct(_OlDatum* datump)
{

	if (NULL != datump && NULL != *datump) {
		switch ((*datump)->type) {

		case _OL_DATUM_TYPE_NULL:
			/*EMPTY*/;
			break;

		case _OL_DATUM_TYPE_STRING:
			_OlStringDestruct((String*)&(*datump)->content);
			break;

		case _OL_DATUM_TYPE_STR:
			_OlStrDestruct((OlStr*)&(*datump)->content);
			break;

		case _OL_DATUM_TYPE_FNAVNODE:
			_OlFNavNodeDestruct((OlFNavNode*)&(*datump)->content);
			break;

		case _OL_DATUM_TYPE_STAT:
			_OlStatDestruct((OlStat*)&(*datump)->content);
			break;

		case _OL_DATUM_TYPE_GLYPH:
			_OlGlyphDestruct((OlGlyph*)&(*datump)->content);
			break;

		default:
			_OlAbort(NULL, "Unsupported data type %d\n",
				&(*datump)->type);
			/*NOTREACHED*/
			break;
		}
		
		_OL_FREE(*datump);
		*datump = (_OlDatum)NULL;
	}
} /* end of _OlDatumDestruct() */


/************************************************************************
 *
 *      _OlDatumCompare -- Compare two objects:
 *		returns + for left_datum > right_datum
 *			- for left_datum < right_datum
 *			0 for left_datum = right_datum
 *
 ************************************************************************/

int
_OlDatumCompare(const _OlDatum left_datum, const _OlDatum right_datum,
	_OlDatumComparisonFunc datum_cmp)
{
	int	return_value;
	
	if (NULL == left_datum) 
		if (NULL == right_datum)
			return_value = 0;
		else
			return_value = -1;	/* By convention */
	else if (NULL == right_datum) 
		return_value = +1;		/* By convention */
	else if (left_datum->type != right_datum->type)
		_OlAbort(NULL, "Data type mismatch in _OlDatumCompare: "
			"%s left, %s right", &left_datum->type, 
			&right_datum->type);
	else 
		switch (left_datum->type) {
	
		case _OL_DATUM_TYPE_NULL:
			(void) printf("content = (void*)NULL");
			break;
	
		case _OL_DATUM_TYPE_STRING:
			return_value = _OlStringCompare(
				(const char*const)left_datum->content, 
				(const char*const)right_datum->content,
				(StringComparisonFunc)datum_cmp);
			break;
	
		case _OL_DATUM_TYPE_STR:
			return_value = _OlStrCompare(
				(const OlStr)left_datum->content, 
				(const OlStr)right_datum->content,
				(OlStrComparisonFunc)datum_cmp);
			break;
	
		case _OL_DATUM_TYPE_FNAVNODE:
			return_value = _OlFNavNodeCompare(
				(OlFNavNode)left_datum->content, 
				(OlFNavNode)right_datum->content,
				(StringComparisonFunc)datum_cmp);
			break;
	
		default:
			_OlAbort(NULL, "Unsupported data type %d\n",
				&left_datum->type);
			/*NOTREACHED*/
			break;
		}
	
	return return_value;
} /* end of _OlDatumCompare() */


#ifdef	_OLDATUMMERGE
/************************************************************************
 *
 *      _OlDatumMerge -- Merge two objects if semantically feasibly
 *
 ************************************************************************/

void
_OlDatumMerge(_OlDatum datum, _OlDatum* add_datump);
{
	
	if (NULL != add_datump)
		if (NULL == datum) 
			if (NULL == *add_datump)
				return;
			else
				_OlAbort(NULL,
					"Attempt to _OlDatumMerge with NULL");
	
		else if (NULL == *add_datump) 
			return;
		else if (datum->type != *add_datump->type)
			_OlAbort(NULL, "Data type mismatch in _OlDatumMerge: "
				"%s target, %s source", &datum->type, 
				&(*add_datump)->type);
		else {
			_OL_MALLOC(*new_datump, _OlDatumRec);
		
			switch (add_datump->type) {
	
			case _OL_DATUM_TYPE_NULL:
				break;
	
			case _OL_DATUM_TYPE_STRING:
				/*!
				_OlStringDestruct();
				_OlStringConstruct(
					(String*)&(*new_datump)->content,
					(const char* const)add_datump->content);
				string_merge()
				strcat?
				!*/
				break;
	
			default:
				_OlAbort(NULL, "Unsupported data type %d.\n",
					&datum->type);
				/*NOTREACHED*/
				break;
			}
		}
} /* end of _OlDatumMerge() */
#endif	/* _OLDATUMMERGE */

/************************************************************************
 *
 *      Implementation of String object functions
 *
 ************************************************************************/


/************************************************************************
 *
 *      _OlStringConstruct -- Allocate and initialize a String
 *
 ************************************************************************/

void
_OlStringConstruct(String* new_stringp, const char* const string)
{
	
	if (NULL == string)
		*new_stringp = NULL;
	else {
		_OL_MALLOCN(*new_stringp, _OlStrlen(string) + 1);
		(void) strcpy(*new_stringp, string);
	}
}


/************************************************************************
 *
 *      _OlStringDestruct -- Free a String
 *
 ************************************************************************/

void
_OlStringDestruct(String* stringp)
{

	if (NULL != stringp && NULL != *stringp)
		_OL_FREE(*stringp);
}


/************************************************************************
 *
 *      _OlStringCompare -- Compare two objects
 *
 ************************************************************************/

int
_OlStringCompare(const char*const left_string, const char*const right_string,
	StringComparisonFunc string_cmp)
{

	if (NULL == string_cmp)
		return strcmp(left_string, right_string);
		/*NOTREACHED*/
	else 
		return (*string_cmp)(left_string, right_string);
		/*NOTREACHED*/
}


/************************************************************************
 *
 *      Implementation of OlStr object functions
 *
 ************************************************************************/


/************************************************************************
 *
 *      _OlStrSetMode -- 
 *
 ************************************************************************/

void
_OlStrSetMode(OlStrRep text_format)
{

	datum_sm = str_methods[text_format];
}


/************************************************************************
 *
 *      _OlStrConstruct -- Allocate and initialize a OlStr
 *
 ************************************************************************/

void
_OlStrConstruct(OlStr* new_strp, const OlStr str)
{

	if (NULL == str)
		*new_strp = NULL;
	else {
		_OL_MALLOCN(*new_strp, datum_sm.StrNumBytes(str));
		(void) datum_sm.StrCpy(*new_strp, str);
	}
}


/************************************************************************
 *
 *      _OlStrDestruct -- Free a OlStr
 *
 ************************************************************************/

void
_OlStrDestruct(OlStr* strp)
{

	if (NULL != strp && NULL != *strp)
		switch (datum_text_format) {
		case OL_SB_STR_REP:
		/*FALLTHROUGH*/
		case OL_MB_STR_REP: {
			String		str;
			
			str = (String)*strp;
			_OL_FREE(str);
			}
			break;
		case OL_WC_STR_REP: {
			wchar_t		wc;

			wc = (wchar_t)*strp;
			_OL_FREE(wc);
			}
			break;
		default:
			_OlAbort(NULL, dgettext(OlMsgsDomain, 
				"_OlStrDestruct():  unsupported text "
				"format %d.\n"), datum_text_format);
			/*NOTREACHED*/
			break;
		}
}


/************************************************************************
 *
 *      _OlStrCompare -- Compare two objects
 *
 ************************************************************************/

int
_OlStrCompare(const OlStr left_str, const OlStr right_str, OlStrComparisonFunc str_cmp)
{

	if (NULL == str_cmp)
		return datum_sm.StrCmp(left_str, right_str);
		/*NOTREACHED*/
	else 
		return (*str_cmp)(left_str, right_str);
		/*NOTREACHED*/
}


/************************************************************************
 *
 *      Implementation of OlFNavNode object functions
 *
 ************************************************************************/


/************************************************************************
 *
 *      _OlFNavNodeConstruct -- Allocate and initialize an OlFNavNode
 *
 ************************************************************************/

void
_OlFNavNodeConstruct(OlFNavNode* newp, const OlFNavNode request)
{

	if (NULL == request)
		*newp = NULL;
	else {
		OlFNavNode	c_new;
		
		_OL_MALLOC(c_new, OlFNavNodeRec);
		*newp = c_new;

		_OlStringConstruct(&c_new->name,
			(const char*const)request->name);

		c_new->is_folder = request->is_folder;
		c_new->operational = request->operational;
		c_new->filtered = request->filtered;
		c_new->active = request->active;
		
		_OlGlyphConstruct(&c_new->glyph, request->glyph);
		_OlStatConstruct(&c_new->sbufp, request->sbufp);
	}
}


/************************************************************************
 *
 *      _OlFNavNodeDestruct -- Free a OlFNavNode
 *
 ************************************************************************/

void
_OlFNavNodeDestruct(OlFNavNode* nodep)
{

	if (NULL != nodep && NULL != *nodep) {
		OlFNavNode	node = *nodep;

		_OlStringDestruct(&node->name);
		_OlGlyphDestruct(&node->glyph);
		_OlStatDestruct(&node->sbufp);

		_OL_FREE(*nodep);
	}
}


/************************************************************************
 *
 *      _OlFNavNodeCompare -- Compare two OlFNavNode
 *
 ************************************************************************/

int
_OlFNavNodeCompare(const OlFNavNode left,
	const OlFNavNode right, StringComparisonFunc string_cmp)
{

	if (NULL == string_cmp)
		return strcmp(left->name, right->name);
		/*NOTREACHED*/
	else 
		return (*string_cmp)(left->name, right->name);
		/*NOTREACHED*/
}


/************************************************************************
 *
 *      Implementation of OlStat (struct stat*) object functions
 *
 ************************************************************************/


/************************************************************************
 *
 *      _OlStatConstruct -- Allocate and initialize an OlStat
 *
 ************************************************************************/

void
_OlStatConstruct(OlStat* newp, const OlStat request)
{

	if (NULL == request)
		*newp = NULL;
	else {
		_OL_MALLOC(*newp, OlStatRec);
		memcpy((void*)*newp, (void*)request, sizeof (OlStat));
	}
}


/************************************************************************
 *
 *      _OlStatDestruct -- Free an OlStat
 *
 ************************************************************************/

void
_OlStatDestruct(OlStat* sbufpp)
{

	if (NULL != sbufpp && NULL != *sbufpp)
		_OL_FREE(*sbufpp);
}


/************************************************************************
 *
 *      Implementation of OlGlyph object functions
 *
 ************************************************************************/


/************************************************************************
 *
 *      _OlGlyphConstruct -- Allocate and initialize an OlGlyph
 *		shallow copy
 *
 ************************************************************************/

void
_OlGlyphConstruct(OlGlyph* newp, const OlGlyph request)
{
	
	if (NULL == newp)
		*newp = NULL;
	else {
		OlGlyph		c_new;

		_OL_MALLOC(c_new, OlGlyphRec);
		*newp = c_new;
		c_new->type  = request->type;
		
		switch (request->type) {
		case OL_GLYPH_TYPE_XIMAGE:
			if ((OlGlyphRep)NULL != request->rep) {
				_OL_MALLOC(c_new->rep, OlGlyphRepRec);
				memcpy((void*)c_new->rep, 
					(const void*)request->rep,
					sizeof (XImage));
			} else
				c_new->rep = (OlGlyphRep)NULL;
			break;
		case OL_GLYPH_TYPE_PIXMAP:
		/*FALLTHROUGH*/
		default:
			_OlAbort(NULL, "Unsupported data type %d.\n",
				&request->type);
			/*NOTREACHED*/
			break;
		}
	}
}


/************************************************************************
 *
 *      _OlGlyphDestruct -- Free an OlGlyph
 *
 ************************************************************************/

void
_OlGlyphDestruct(OlGlyph* glyphp)
{

	if (NULL != glyphp && NULL != *glyphp) {
		OlGlyph		glyph = *glyphp;

		_OL_FREE(glyph->rep);
		_OL_FREE(*glyphp);
	}
}


#ifdef	DEBUG
/************************************************************************
 *
 *      self-test
 *
 ************************************************************************/


/************************************************************************
 *
 *      _OlDatumPrint -- Dump the object to standard output
 *
 ************************************************************************/

void
_OlDatumPrint(const _OlDatum datum)
{
	
	(void) printf("\n_OlDatum:\t");

	if (NULL == datum)
		(void) printf("(_OlDatum)NULL\n");
	else {
		(void) printf("\n\ttype = ");

		switch (datum->type) {

		case _OL_DATUM_TYPE_NULL:
			(void) printf("_OL_DATUM_TYPE_NULL\n");
			break;

		case _OL_DATUM_TYPE_STRING:
			(void) printf("_OL_DATUM_TYPE_STRING\n\tcontent = ");
			_OlStringPrint((const char* const)datum->content);
			break;

		case _OL_DATUM_TYPE_FNAVNODE:
			(void) printf("_OL_DATUM_TYPE_FNAVNODE\n\tcontent "
				"(%x) :\n", datum->content);
			_OlFNavNodePrint((OlFNavNode)datum->content);
			break;

		default:
			_OlAbort(NULL, "Unsupported data type %d\n",
				&datum->type);
			/*NOTREACHED*/
			break;
		}
	}
} /* end of _OlDatumPrint() */


/************************************************************************
 *
 *      _OlStringPrint -- Dump a String to standard output
 *
 ************************************************************************/

void
_OlStringPrint(const char*const string)
{

	if (NULL == string)
		(void) printf("(const char*const)NULL\n");
	else
		(void) printf("\"%s\"\n", string);
}


/************************************************************************
 *
 *      _OlFNavNodePrint -- Dump an OlFNavNode to standard output
 *
 ************************************************************************/

void
_OlFNavNodePrint(const OlFNavNode node)
{

	#define	OUTPUT(flag)	((flag) ? "B_TRUE" : "B_FALSE")
	
	if (NULL == node)
		(void) printf("(const OlFNavNode)NULL\n");
	else {
		(void) printf("\n\tname: ");
		_OlStringPrint(node->name);
		
		(void) printf("\tis_folder: %s\n", OUTPUT(node->is_folder));
		(void) printf("\toperational: %s\n", OUTPUT(node->operational));
		(void) printf("\tfiltered: %s\n", OUTPUT(node->filtered));
		(void) printf("\tactive: %s\n", OUTPUT(node->active));
		
		_OlGlyphPrint(node->glyph);
		_OlStatPrint(node->sbufp);
	}

	#undef	OUTPUT
}


/************************************************************************
 *
 *      _OlStatPrint -- Dump an OlStat to standard output
 *
 ************************************************************************/

void
_OlStatPrint(const OlStat sbufp)
{

	if (NULL == sbufp)
		(void) printf("(struct stat*)NULL\n");
	else {
		(void) printf("\tOlStat: %x ...\n", sbufp);
	}
}


/************************************************************************
 *
 *      _OlGlyphPrint -- Dump an OlGlyph to standard output
 *
 ************************************************************************/

void
_OlGlyphPrint(const OlGlyph glyph)
{

	if (NULL == glyph)
		(void) printf("(OlGlyph)NULL\n");
	else
		(void) printf("\tOlGlyph: %x ...\n", glyph);
}


void
_OlDatumTest(void)
{
	_OlDatum	string;
	_OlDatum	string1;
	_OlDatum	string2;
	_OlDatum	string3;
	_OlDatumRec	value_buffer;
	int		cmp_val;
	
	errno = 0;

	_OlDatumConstruct(&string, (_OlDatum)NULL);
	_OlDatumPrint(string);
	_OlDatumDestruct(&string);
	
	value_buffer.type = _OL_DATUM_TYPE_NULL;
	_OlDatumConstruct(&string, &value_buffer);
	_OlDatumPrint(string);
	_OlDatumDestruct(&string);
	
	value_buffer.type = _OL_DATUM_TYPE_STRING;
	value_buffer.content = NULL;
	_OlDatumConstruct(&string, &value_buffer);
	_OlDatumPrint(string);
	_OlDatumDestruct(&string);
	
	value_buffer.type = _OL_DATUM_TYPE_STRING;
	value_buffer.content = "The quick brown fox jumped over the lazy dog.";
	_OlDatumConstruct(&string, &value_buffer);
	_OlDatumPrint(string);
	_OlDatumDestruct(&string);
	
	value_buffer.type = _OL_DATUM_TYPE_STRING;
	value_buffer.content = "one";
	_OlDatumConstruct(&string1, &value_buffer);
	_OlDatumPrint(string1);
	value_buffer.content = "two";
	_OlDatumConstruct(&string2, &value_buffer);
	_OlDatumPrint(string2);
	cmp_val = _OlDatumCompare(string1, string2, NULL);
	(void) printf("The first string is %s than the second one.\n",
		cmp_val ? (cmp_val > 0 ? "greater" : "less") : "equal");

	_OlDatumConstruct(&string3, string2);
	_OlDatumPrint(string2);
	_OlDatumPrint(string3);
	cmp_val = _OlDatumCompare(string2, string3, NULL);
	(void) printf("The first string is %s than the second one.\n",
		cmp_val ? (cmp_val > 0 ? "greater" : "less") : "equal");

	_OlDatumDestruct(&string1);
	_OlDatumDestruct(&string2);

	_OlDatumPrint(string3);
	_OlDatumDestruct(&string3);
	
	_OlDatumPrint(string);
}
#endif	/* DEBUG */


/* end of Datum.c */
