#ifndef lint
        static char sccsid[] = "@(#)isperm.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isperm.c
 *
 * Description:
 *	Extract permissions from mode functions.
 */

#include "isam_impl.h"


/*
 * _getopenmode(mode)
 *
 * Extract open mode (ISINPUT, ISOUTPUT, ISINOUT) from mode.
 */

enum openmode
_getopenmode(mode)
    int		mode;
{
    switch (mode & ISOPENMODE) {
    case ISINPUT:
	return (OM_INPUT);
    case ISOUTPUT:
	return (OM_OUTPUT);
    case ISINOUT:
	return (OM_INOUT);
    default:
	return (OM_BADMODE);
    }
}

/*
 * _getreadmode(mode)
 *
 * Extract read mode from mode.
 */

enum readmode
_getreadmode(mode)
    int		mode;
{
    switch (mode & ISREADMODE) {
    case ISFIRST:
	return (RM_FIRST);
    case ISLAST:
	return (RM_LAST);
    case ISNEXT:
	return (RM_NEXT);
    case ISPREV:
	return (RM_PREV);
    case ISCURR:
	return (RM_CURR);
    case ISEQUAL:
	return (RM_EQUAL);
    case ISGREAT:
	return (RM_GREAT);
    case ISGTEQ:
	return (RM_GTEQ);
    case ISLESS:
	return (RM_LESS);
    case ISLTEQ:
	return (RM_LTEQ);
    default:
	return (RM_BADMODE);
    }
}
