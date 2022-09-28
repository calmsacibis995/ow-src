#ifndef lint
        static char sccsid[] = "@(#)isalloc.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isalloc.c
 *
 * Description:
 *	Functions that allocate and deallocate memory.
 *	All errors are treated as fatal.
 */

#include "isam_impl.h"

extern char *malloc(), *realloc();

/*
 * _ismalloc(nbytes)
 *
 * Allocate nbytes.
 */

char *_ismalloc(nbytes)
    unsigned int	nbytes;
{
    register char	*p;
    
    if ((p = malloc (nbytes)) == NULL)
	_isfatal_error("malloc() failed");

    return (p);
}

char *_isrealloc(oldaddr, nbytes)
    char		*oldaddr;
    unsigned int	nbytes;
{
    register char	*p;
    
    if ((p = realloc (oldaddr, nbytes)) == NULL)
	_isfatal_error("realloc() failed");

    return (p);
}


/*
 * _isallocstring(str)
 *
 * Create a duplicate of string in dynamic memory.
 */

char *
_isallocstring(str)
    char	*str;
{
    register char	*p;

    if ((p = strdup(str)) == NULL) 
	_isfatal_error("strdup() failed");

    return (p);
}

/*
 * _isfreestring(str)
 *
 * Free dynamically allocated string.
 */

void
_isfreestring(str)
    char	*str;
{
    assert(str != NULL);
    free(str);
}
