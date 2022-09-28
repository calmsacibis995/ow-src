#ifndef	_XOL_OW_FSET_DB_H
#define	_XOL_OW_FSET_DB_H

#pragma	ident	"@(#)OWFsetDB.h	1.2	93/02/24 include/Xol SMI"	/* OLIT */

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

#define OWFS_MAXNAMES             10000
#define OWFS_MAXFSETLEN           1024

extern OWFsetDB		CreateOWFsetDB(char *		path_name);

extern void		DestroyOWFsetDB(OWFsetDB	fdb);
				    
extern char **		ListOWFsets(
				OWFsetDB	fdb,
				int *		return_fset_count);

extern char *		GetFsetForOWFset(
				OWFsetDB	fdb,
				char *		fset_name);

extern char *		GetOWFsetForAlias(
				  OWFsetDB	fdb,
				  char *	fset_alias);

extern char *		GetOWFsetForSpec(
				OWFsetDB	fdb,
				char *		fset_spec);

extern void		FreeOWFsetList(char ** list);

#endif	/* _XOL_OW_FSET_DB_H */
