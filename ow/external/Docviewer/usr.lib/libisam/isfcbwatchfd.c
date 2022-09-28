#ifndef lint
        static char sccsid[] = "@(#)isfcbwatchfd.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isfcbwatchfd.c
 *
 * Description:
 *	Watch the limit on UNIX file descriptors used by the FCB module.
 */

#include "isam_impl.h"
#include <sys/time.h>
#include <sys/resource.h>

static int _limit = MAXFCB_UNIXFD;	     /* Imposed limit */
static int _in_use = 0;			     /* Current number of 
					      * open file descriptors
					      */

/*
 * _watchfd_incr(n)
 *
 * Register that n additional UNIX file descriptors were open.
 *
 * Return the new number of open file descriptors.
 */

int
_watchfd_incr(n)
{
    _in_use += n;
    assert(_in_use <= _limit);
    return (_in_use);
}


/*
 * _watch_decr(n)
 *
 * Register that n open file descriptors were closed.
 *
 * Return the new number of open file descriptors.
 */

int
_watchfd_decr(n)
{
    _in_use -= n;
    assert(_in_use >= 0);
    return (_in_use);
}


/*
 * _watch_check()
 *
 * Return number of fd that are still available.
 */

int
_watchfd_check()
{
    return (_limit - _in_use);
}


/*
 * _watchfd_max_set(n)
 *
 * Set the maximum number of UNIX fds that may be comsumed by ISAM files.
 */

int
_watchfd_max_set(n)
    int		n;
{
    int		oldlimit = _limit;
    int dtab_size = 0;
    struct rlimit rl;

    if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
      dtab_size = rl.rlim_cur;
      

    if (n < 3 || n > dtab_size) {
	_setiserrno2(EBADARG, '9', '0');
	return (ISERROR);
    }

    _limit = n;
    return (oldlimit);
}

/*
 * _watchfd_max_get()
 *
 * Get the maximum number of UNIX fds that may be comsumed by ISAM files.
 */

int
_watchfd_max_get()
{
    return (_limit);
}
