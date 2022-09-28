#pragma ident	"@(#)FlatCvt.c	302.4	97/03/26 lib/libXol SMI"	/* flat:FlatCvt.c 1.12	*/

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

/*
 *************************************************************************
 *
 * Description:
 *	This file contains resource converters for the flattened widget.
 *
 ******************************file*header********************************
 */


#include <ctype.h>
#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/Error.h>
#include <Xol/FlatP.h>
#include <Xol/OpenLookP.h>


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define FLAT(w)		(((FlatWidget)w)->flat)
#define FCLASS(w)	(((FlatWidgetClass)XtClass(w))->flat_class)
#define CNAME(w)	((XtClass((Widget)w))->core_class.class_name)

typedef struct {
	char *	start;
	int	length;
} StringNode, *StringArray;

#define ALLOC_STRING_ARRAY(f,stk,def_size,size)\
	auto StringNode	stk[def_size];\
	auto StringNode *	f = ((size+1) > XtNumber(stk) ?\
		(StringNode *)XtMalloc((size+1)*sizeof(StringNode)) : stk)
#define FREE_MEMORY(ptr,stk)	if(ptr!=stk)XtFree((char *)ptr);

#define SAME_STRING(s1,s2)  (!strcmp((const char *)(s1),(const char *)(s2)))

#define Done(type, value) \
	if (to->addr != (caddr_t)NULL) {			\
		if (to->size < (unsigned int)sizeof(type)) {	\
			to->size = (unsigned int) sizeof(type);	\
			return (False);				\
		}						\
		*(type*)(to->addr) = value;			\
	} else {						\
		to->addr = (caddr_t)&value;			\
	}							\
	to->size = (unsigned int)sizeof(type);			\
	return(True)

				/* Define parse-aiding macros		*/

#define STACK_SIZE		256
#define AT_EOL(c)		(c == '\0')
#define LITERAL			('\\')
#define FIELD_SEPARATOR		(',')
#define RECORD_LEADER		('{')
#define RECORD_TRAILER		('}')
#define IS_WHITESPACE(c)	(c == ' '|| c == '\t'|| c == '\n')
#define SCAN_WHITESPACE(s)	while(IS_WHITESPACE(*s)) ++s
#define SCAN_FOR_CHAR(s, c)	while(!AT_EOL(*s) && *s != c) ++s

	/* Scan a keyword.  The scanning ends when a field separator or
	 * a record separator is encounterd.  However, the keyword is
	 * the string starting at the first character.  It ends just
	 * prior to any whitespace preceding the two terminators above.
	 */
#define SCAN_KEYWORD(s,sn,tc)	\
    {								\
	register char *	whitespace = (char *)NULL;		\
	(sn).start  = s;					\
	while(!AT_EOL(*s)) {					\
		if (*s == LITERAL) {				\
			whitespace = (char *)NULL;		\
			++s;					\
			if (AT_EOL(*s)) {			\
				break;				\
			}					\
		} else if (*s == FIELD_SEPARATOR ||		\
			   *s == RECORD_TRAILER) {		\
			break;					\
		} else if (IS_WHITESPACE(*s)) {			\
			if (whitespace == (char *)NULL)		\
				whitespace = s;			\
		} else {					\
			whitespace = (char *)NULL;		\
		}						\
		++s;						\
	}							\
	(sn).length = (int)((whitespace!=(char*)NULL ?		\
				whitespace : s) - (sn).start);	\
    }								\
    (tc) += (sn).length + 1

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private  Procedures 
 *		2. Public  Procedures 
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static int	CopyKeyword (char *, char *, int);
static int	CountMaxFields (char *);
static Boolean	CvtStringToFlatItemFields (Display *, XrmValue *,
				Cardinal *, XrmValue *, XrmValue *,
				XtPointer *);
static Boolean	CvtStringToFlatItems (Display *, XrmValue *,
				Cardinal *, XrmValue *, XrmValue *,
				XtPointer *);
static XtArgVal	GetArgVal (FlatWidget, XtResource *, StringNode *);
static FlatWidget GetFlatId (Display *, const char *, XrmValue *);
static XtResource *
		GetResource (FlatWidget, String);
static int	ParseString (Widget, const char *, char *,
					StringNode *, int *, int *, int *);

					/* public procedures		*/

void	_OlFlatAddConverters (void);

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ****************************private*procedures***************************
 */

/*
 *************************************************************************
 * CopyKeyword - copies a keyword into a buffer and returns the actual
 * characters in the word.  Literal characters are not included in the
 * count and are not copied into the destination.
 ****************************procedure*header*****************************
 */
static int
CopyKeyword (char *dest, char *src, int raw_length)
{
	char *	old_dest = dest;

	if (raw_length) {
		for ( ; raw_length; --raw_length, ++src)
		{
			if (*src == LITERAL) {
				--raw_length;
				++src;
				*dest++ = (*src == 'n' ? '\n' :
					   *src == 't' ? '\t' :
					   *src == 'b' ? '\b' : *src);
			}
			else {
				*dest++ = *src;
			}
		}
		*dest++ = '\0';
	}

	return((int)(dest - old_dest));
} /* END OF CopyKeyword() */

/*
 *************************************************************************
 * CountMaxFields - counts the number of field separators to determine
 * the maximum number of fields within the string.
 ****************************procedure*header*****************************
 */
static int
CountMaxFields (register char *s)
{
	int	hits;

	for (hits=0; *s; ++s)
		if (*s == FIELD_SEPARATOR || *s == RECORD_TRAILER)
			++hits;
	return(hits);
} /* END OF CountMaxFields() */

/*
 *************************************************************************
 * CvtStringToFlatItemFields -
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
CvtStringToFlatItemFields (Display *dpy, XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to, XtPointer *data)
{
	static String *	item_fields;
	int		i = CountMaxFields((char *)(from->addr));
	ALLOC_STRING_ARRAY(fields,stack,20,i);
	int		elements;
	int		num_recs;
	int		total_chars;
	char *		name;
	FlatWidget	flat;
	char *		s;
	static const char *	proc_name = "CvtStringToFlatItemFields";
	Cardinal	size;

	if ((flat = GetFlatId(dpy, proc_name, args)) == (FlatWidget)NULL ||
	    !ParseString((Widget)flat, proc_name, (char *)from->addr,
			fields, &elements, &num_recs, &total_chars))
	{
		FREE_MEMORY(fields,stack);
		return(False);
	}
	else if (num_recs != 1)
	{
		OlVaDisplayWarningMsg(dpy, "tooManyRecords",
			"Convert", OleCOlToolkitWarning,
			"%s: Widget \"%s\" (class \"%s\") database\
 XtNnumItemFields value has more than 1 record specification, %d { }'s",
			proc_name, XtName((Widget)flat), CNAME(flat), num_recs);
		FREE_MEMORY(fields,stack);
		return(False);
	}
	else if (flat->flat.num_item_fields != OL_IGNORE &&
		 flat->flat.num_item_fields != (Cardinal)elements)
	{
		OlVaDisplayWarningMsg(dpy, "inconsistentNumItemFields",
			"Convert", OleCOlToolkitWarning,
			"%s: Widget \"%s\" (class \"%s\") programmatic\
 XtNnumItemFields (%d) conflicts with fields read from the database (%d)",
			proc_name, XtName((Widget)flat), CNAME(flat),
			flat->flat.num_item_fields, elements);
	}

			/* If no fields were specified, return NULL.	*/

	if (num_recs == 0)
	{
		FREE_MEMORY(fields,stack);
		item_fields = (String *)NULL;

		Done(String *, item_fields);
	}

	size		= (Cardinal)((elements * sizeof(String)) + total_chars);
	item_fields	= (String *)XtMalloc(size);

			/* Set the starting location where we can write
			 * the strings.					*/

	s = (char *)item_fields + (sizeof(String) * elements);

	for (i = 0; i < elements; ++i, ++fields)
	{
		if (fields->length == 0)
		{
			OlVaDisplayWarningMsg(dpy, "nullItemField",
				"Converter", OleCOlToolkitWarning,
				"%s: widget \"%s\" (class \"%s\") cannot\
 have NULL itemField name", proc_name, XtName((Widget)flat), CNAME(flat));
			break;
		}

		name = s;
		s += CopyKeyword(name, fields->start, fields->length);

		if (GetResource(flat, name) != (XtResource *)NULL)
		{
			item_fields[i] = name;
		}
		else
		{
			OlVaDisplayWarningMsg(dpy, "badItemResource",
				"Converter", OleCOlToolkitWarning,
				"%s: widget \"%s\" (class \"%s\") does\
 have resource \"%s\"", (char *)proc_name, XtName((Widget)flat),
				CNAME(flat), name);
			break;
		}
	}
	
			/* if 'i' is less than elements, an error
			 * occurred, so free memory and set the
			 * size to indicate the conversion failed.	*/

	if (i < elements)
	{
		XtFree((char *)item_fields);
		return(False);
	}

	flat->flat.num_item_fields = (Cardinal)elements;

	if (size > (Cardinal)(s - (char *)item_fields))
	{
		item_fields = (String *)XtRealloc((XtPointer)item_fields,
				(s-(char *)item_fields));
	}

	FREE_MEMORY(fields, stack);
	Done(String *, item_fields);
} /* END OF CvtStringToFlatItemFields() */

/*
 *************************************************************************
 * CvtStringToFlatItems -
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
static Boolean
CvtStringToFlatItems (Display *dpy, XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to, XtPointer *data)
{
	static XtPointer	items;
	XtArgVal *	argvals;
	char *		s;
	int		elements;
	int		num_recs;
	int		total_chars;
	int		i;
	int		j = CountMaxFields((char *)(from->addr));
	ALLOC_STRING_ARRAY(fields,stack,50,j);
	FlatWidget	flat;
	static const char *	proc_name = "CvtStringToFlatItems";
	XtResource *	rsc_stack[20];
	XtResource **	rscs;
	Cardinal	size;

	if ((flat = GetFlatId(dpy, proc_name, args)) == (FlatWidget)NULL ||
	    !ParseString((Widget)flat, proc_name, (char *)from->addr,
			fields, &elements, &num_recs, &total_chars))
	{
		FREE_MEMORY(fields,stack);
		return(False);
	}
	else if (flat->flat.num_item_fields != (Cardinal)OL_IGNORE &&
		 flat->flat.num_item_fields != (Cardinal)0 &&
		 flat->flat.num_item_fields != elements)
	{
		OlVaDisplayWarningMsg(dpy, "itemsAndNumItemFieldsConflict",
			"Convert", OleCOlToolkitWarning,
			"%s: Widget \"%s\" (class \"%s\") number of\
 XtNnumItemFields (%d) conflicts with XtNitems' fields per record (%d)",
			proc_name, XtName((Widget)flat), CNAME(flat),
			flat->flat.num_item_fields, elements);
		FREE_MEMORY(fields, stack);
		return(False);
	}
	else if (flat->flat.num_items != OL_IGNORE &&
		 flat->flat.num_items != (Cardinal)num_recs)
	{
		OlVaDisplayWarningMsg(dpy, "inconsistentNumItemFields",
			"Convert", OleCOlToolkitWarning,
			"%s: Widget \"%s\" (class \"%s\") programmatic\
 XtNnumItems (%d) conflicts with fields read from the database (%d)",
			proc_name, XtName((Widget)flat), CNAME(flat),
			flat->flat.num_items, num_recs);
	}

		/* If there are no items or no itemFields, return NULL.	*/

	if (num_recs == 0 ||
	    (flat->flat.num_item_fields != (Cardinal)OL_IGNORE &&
	     flat->flat.num_item_fields == (Cardinal)0) ||
	    flat->flat.item_fields == (String *)NULL)
	{
		FREE_MEMORY(fields,stack);
		return(False);
	}

	size	= (Cardinal) ((elements * num_recs * sizeof(XtArgVal)) +
				total_chars);
	items	= (XtPointer)XtMalloc(size);

			/* Populate an array with the addresses of
			 * the itemField resource structures.		*/

	rscs = (elements <= XtNumber(rsc_stack) ? rsc_stack :
		(XtResource **)XtMalloc((Cardinal)
				(elements*sizeof(XtResource*))));

	for (i=0; i < FLAT(flat).num_item_fields; ++i)
	{
		rscs[i] = GetResource(flat, FLAT(flat).item_fields[i]);
	}

			/* Set the starting location where we can write
			 * the strings.					*/

	s = (char *)items + (sizeof(XtArgVal) * num_recs * elements);

	for (i=0, argvals=(XtArgVal *)items; i < num_recs; ++i)
	{
		for (j = 0; j < elements; ++j, ++fields)
		{
			if (rscs[j] != (XtResource *)NULL)
			{
				/* If this resource is a String, make
				 * a copy of it; else, convert it.	*/

				if (!strcmp((const char *)XtRString,
					(const char *)rscs[j]->resource_type))
				{
					*argvals = (XtArgVal)s;
					s += CopyKeyword(s, fields->start,
							fields->length);
				}
				else
				{
					*argvals = GetArgVal(flat, rscs[j],
								fields);
				}
			}
			else
			{
				*argvals = (XtArgVal)0;
			}
			++argvals;
		}
	}
	
	flat->flat.num_items = (Cardinal)num_recs;

	if (size > (Cardinal)(s - (char *)items))
	{
		items = (XtPointer)XtRealloc(items, (s-(char *)items));
	}

	FREE_MEMORY(rscs, rsc_stack);
	FREE_MEMORY(fields, stack);
	Done(XtPointer, items);
} /* END OF CvtStringToFlatItems() */

/*
 *************************************************************************
 * GetArgVal - converts a string into an XtArgVal and returns it.
 * Note: this procedure is not called the the resource type is XtRString.
 ****************************procedure*header*****************************
 */
static XtArgVal
GetArgVal (FlatWidget w, XtResource *rsc, StringNode *sn)
{
	XtArgVal	val;
	String		type = rsc->resource_type;
	char		stack[256];
	char *		name = ((sn->length + 1) > sizeof(stack) ?
				XtMalloc((Cardinal)(sn->length+1)) : stack);
	XrmValue	from_val;
	XrmValue	to_val;
	Boolean		use_to_val = True;

	if (sn->length == 0)
	{
		name = "";
	}
	else
	{
		(void) CopyKeyword(name, sn->start, sn->length);
	}

	from_val.size	= (unsigned int)sizeof(String);
	from_val.addr	= (caddr_t)name;

	XtConvert((Widget)w, XtRString, &from_val, type, &to_val);

		/* If the conversion failed, use the default value.	*/

	if (to_val.size == (unsigned int)0)
	{
		if (SAME_STRING(XtRImmediate, type))
		{
			use_to_val	= False;
			val		= (XtArgVal)rsc->default_addr;
		}
		else if (SAME_STRING(XtRCallProc, type))
		{
			(* ((XtResourceDefaultProc)rsc->default_addr))
				((Widget)w, rsc->resource_offset, &to_val);
		}
		else
		{
					/* Use the default address	*/

			from_val.addr = (caddr_t)rsc->default_addr;

			XtConvert((Widget)w, XtRString, &from_val,
					type, &to_val);
		}
	}
	else
	{
		_OlConvertToXtArgVal((XtPointer)to_val.addr,
					&val, (Cardinal)to_val.size);
	}

	if (use_to_val == True)
	{
		if (to_val.size == (unsigned int)0)
		{
			val = (XtArgVal)0;
		}
		else
		{
			_OlConvertToXtArgVal((XtPointer)to_val.addr,
				&val, (Cardinal)to_val.size);
		}
	}

	FREE_MEMORY(name, stack);
	return(val);
} /* END OF GetArgVal() */

/*
 *************************************************************************
 * GetFlatId - extracts a widget id from an XrmValue and checks its
 * validity.
 ****************************procedure*header*****************************
 */
static FlatWidget
GetFlatId (Display *dpy, const char *proc, XrmValue *args)
{
	FlatWidget	flat = *((FlatWidget *)args[0].addr);

	if (flat == (FlatWidget)NULL)
	{
		flat = (FlatWidget)NULL;
		OlVaDisplayWarningMsg(dpy, OleNnullWidget, (String)proc,
			OleCOlToolkitWarning, "%s: NULL widget id", proc);
	}
	else if (XtIsSubclass((Widget)flat, flatWidgetClass) == False)
	{
		flat = (FlatWidget)NULL;
		OlVaDisplayWarningMsg(dpy, "wrongClass", (String)proc,
			OleCOlToolkitWarning,
			"%s: widget \"%s\" (class \"%s\") is not a\
 FlatWidget Subclass", proc, XtName((Widget)flat), CNAME(flat));
	}
	return (flat);
} /* END OF GetFlatId() */

/*
 *************************************************************************
 * GetResource - returns the address of the item resource associated
 * with the supplied name.
 ****************************procedure*header*****************************
 */
static XtResource *
GetResource (FlatWidget fw, String name)
{
	XrmQuarkList	qlist = FCLASS(fw).quarked_items;
	XrmQuark	quark = XrmStringToQuark(name);
	Cardinal	max = FCLASS(fw).num_item_resources;
	Cardinal	i;

	for (i=0; i < max; ++i, ++qlist)
	{
		if (quark == *qlist)
		{
			return(FCLASS(fw).item_resources + i);
		}
	}

	return((XtResource *)NULL);
} /* END OF GetResource() */

/*
 *************************************************************************
 * ParseString - Parses a string and pushes locations and lengths of
 * embedded strings into a supplied array.  The return the number elements
 * in each record and the number of records found.
 * If an error is encountered, a 0 is returned.
 ****************************procedure*header*****************************
 */
static int
ParseString (Widget w, const char *proc_name, register char *s, StringArray kw_array, int *elements, int *num_recs, int *total)
{
	int		num;
	char *		error = (char *)NULL;
	int		e;

	*elements	= 0;
	*num_recs	= 0;
	*total		= 0;
	num		= 0;

	while (!AT_EOL(*s))
	{
		SCAN_WHITESPACE(s);

		if (RECORD_LEADER != *s)
		{
			error = "Missing record leader '{'";
			break;
		}

		++s;		/* skip record leader	*/
		e = 0;

		while(!AT_EOL(*s))
		{
			SCAN_WHITESPACE(s);

			SCAN_KEYWORD(s, kw_array[num], *total);

			++e;
			++num;

			if (*s == FIELD_SEPARATOR) {
				++s;
			} else if (*s == RECORD_TRAILER) {
				break;
			} else {
				error = "Missing field separator ',' or \
 record trailer '}'";
				break;
			}
		}

		if (error != (char *)NULL) {
			break;
		} else {
			++(*num_recs);
		}

		if (*s != RECORD_TRAILER) {
			error = "Missing Trailer";
			break;
		}

		if (*elements == 0) {
			*elements = e;
		} else if (*elements != e) {
			error = "records within list cannot have different\
 number of elements";
			break;
		}

				/* Skip the record trailer and search for
				 * the next leader.			*/
		++s;
		SCAN_FOR_CHAR(s, RECORD_LEADER);
	}

	if (error != (char *)NULL)
	{
		OlVaDisplayWarningMsg(XtDisplay(w), "parseError", "Converter",
			OleCOlToolkitWarning, "Converter %s: flat widget\
\"%s\" (class \"%s\"): %s", proc_name, XtName(w), CNAME(w), error);
	}
	return(error ? 0 : 1);
} /* END OF ParseString() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * _OlFlatAddConverters - adds converters for the XtNitems and
 * XtNitemFields resources.
 ****************************procedure*header*****************************
 */
void
_OlFlatAddConverters (void)
{
	static XtConvertArgRec	args[] = {
		{ XtBaseOffset, (XtPointer) 0, sizeof(FlatWidget) }
	};

		/* Note, we cannot have a request that the converter
		 * type be anything other than XtCacheNone since
		 * the converters will update more than one widget
		 * instance field.					*/

	XtSetTypeConverter(XtRString, OlRFlatItemFields,
			CvtStringToFlatItemFields, args, XtNumber(args),
			XtCacheNone, (XtDestructor)NULL);

	XtSetTypeConverter(XtRString, OlRFlatItems,
			CvtStringToFlatItems, args, XtNumber(args),
			XtCacheNone, (XtDestructor)NULL);

} /* END OF _OlFlatAddConverters() */
