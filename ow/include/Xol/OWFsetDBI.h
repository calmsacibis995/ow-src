#ifndef	_XOL_OW_FSET_DB_I_H
#define	_XOL_OW_FSET_DB_I_H

#pragma	ident	"@(#)OWFsetDBI.h	1.1	92/10/06 include/Xol SMI"	/* OLIT */

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

#include <Xol/OWFsetDB.h>

#define XLFD_COUNT	15
#define INITIAL_ENTRY_COUNT	20

#define COMMENT_CHAR	'!'

#define DEFINITION_STR	"definition"
#define ALIAS_STR	"alias"

typedef struct _FsetRec {
    char *	owfset_fields[XLFD_COUNT];
    char *	fset_name;
    char *	fset_defn;
    char **	aliases;
    int		num_aliases;
} FsetRec;

struct _OWFsetDBRec {
    FsetRec *	entries;
    int		num_entries;
};

#endif	/* _XOL_OW_FSET_DB_I_H */
