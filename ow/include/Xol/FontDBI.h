#ifndef	_XOL_FONT_DB_I_H
#define	_XOL_FONT_DB_I_H

#pragma	ident	"@(#)FontDBI.h	1.1	92/10/06 include/Xol SMI"	/* OLIT */

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

#include <Xol/FontDB.h>

#define DEF_RET_SIZE	40


struct _FontRec {
    FontField *	font_fields;
};

struct _FontDBRec {
    struct _FontRec *	font_entries;
    Cardinal		num_fonts;
    FontFieldID	*	field_ids;
    Cardinal		num_fields;
};

#endif	/* _XOL_FONT_DB_I_H */
