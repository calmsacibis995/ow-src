#ifndef lint
        static char sccsid[] = "@(#)isamhooks.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isamhooks.c
 *
 * Description: 
 *	Entry and exit hooks that are called upon every entry (and exit)
 *	to any Access Module Function.
 *	
 *
 */

#include "isam_impl.h"
extern struct dlink  *pavail;
/*
 * _isam_entryhook()
 *
 */

void
_isam_entryhook()
{
    isdupl = 0;				     /* Reset duplicate indicator */
    _setiserrno2(0, '0', '0');
}

/*
 * _isam_exithook()
 *
 */

void
_isam_exithook()
{
    _isdisk_inval();			     /* Invalidate all buffers */
}
