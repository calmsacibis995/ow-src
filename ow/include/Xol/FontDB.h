#ifndef	_XOL_FONT_DB_H
#define	_XOL_FONT_DB_H

#pragma	ident	"@(#)FontDB.h	1.3	93/02/24 include/Xol SMI"	/* OLIT */

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

#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/types.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/FontChP.h>

#define FC_MAXNAMES             10000
#define FC_LISTLEN              50
#define FC_COLUMN_WIDTH         100
#define FC_MAXFONTLEN           1024


/* FontName prefix, usually '-' */

#define FC_FONTNAME_REGISTRY    0

/* Fields of FontName suffix in order */

#define FC_FOUNDRY              1
#define FC_FAMILY_NAME          2
#define FC_WEIGHT_NAME          3
#define FC_SLANT                4
#define FC_SETWIDTH_NAME        5
#define FC_ADD_STYLE_NAME       6
#define FC_PIXEL_SIZE           7
#define FC_POINT_SIZE           8
#define FC_RESOLUTION_X         9
#define FC_RESOLUTION_Y         10
#define FC_SPACING              11
#define FC_AVERAGE_WIDTH        12

/* Cannot use */
#define FC_CHARSET_REGISTRY     13
#define FC_CHARSET_ENCODING     14

#define FC_XLFD_FIRSTNAME       FC_FOUNDRY
#define FC_XLFD_LAST            FC_CHARSET_ENCODING

typedef int			FontFieldID;

typedef struct _FontFieldRec {
    char *	name;
    XrmQuark	quark;
} FontField;

extern FontDB		_OlCreateFontDatabase(
			String *	font_list,
			int		font_count,	    
			FontFieldID *	fields_interested,
			Cardinal	num_fields,
			String *	charset_registries,
			String *	charset_encodings,
			Cardinal	num_registries_and_encodings);

extern void		_OlDestroyFontDatabase(FontDB	fdb);
				    
extern FontFieldList *	_OlQueryFontDatabase(
			 	FontDB		fdb,
				FontFieldID *	req_fields,
				Cardinal	num_req_fields,		   
				ArgList		field_constraints,
				Cardinal	num_constraints,
				Cardinal *	num_values);

extern void		_OlFreeFontFieldLists(FontFieldList * field_lists,
					      Cardinal	      num_fields);

extern FontField ** 	_OlIntersectFieldLists(
				Cardinal	num_lists,
				FontField ***	lists,
				Cardinal *	list_sizes,
				Cardinal	num_fields,		   
				Cardinal *	num_values);

extern Boolean		_OlExtractFontFields(
				String		font_name,
				ArgList		args,
				Cardinal	num_args);

#endif	/* _XOL_FONT_DB_H */
