#ifndef lint
        static char sccsid[] = "@(#)issignals.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * issignals.c
 *
 * Description:
 *	Signal masking functions
 */


#include "isam_impl.h"
#include <signal.h>

/*
 * _issignal_mask() is called at the beginning of each ISAM update operation
 * to mask signals for the duration of the operation. _issignals_unmask()
 * is called at the end of the operation to restore the original signal
 * mask.
 *
 * _issignals_cntl() enables/disables this signal masking facility.
 * (the default is "mask the signals").
 *
 * The variable already_masked is used to provide more robustness: it
 * will prevent permanent signal masking due to bugs in the ISAM package.
 * The permanent masking of signals would happen if _issignals_mask()
 * were called twice in a row.
 */


static int      do_mask = 1;		    /* default value */
static int      already_masked;
static sigset_t oldmask;
static sigset_t allsignals;


_issignals_cntl(opt)
    int		opt;			     /* 1 will enable masking */
					     /* 0 will disable masking */
{
    int		oldmask = do_mask;

    do_mask = opt ? 1 : 0;

    return (oldmask);
}

_issignals_mask()
{
    if (do_mask && !already_masked) {
        (void) sigfillset(&allsignals);
        (void) sigdelset(&allsignals, SIGTRAP);
        (void) sigdelset(&allsignals, SIGSEGV);
        (void) sigdelset(&allsignals, SIGILL);
        (void) sigdelset(&allsignals, SIGBUS);
        (void) sigprocmask(SIG_SETMASK, &allsignals, &oldmask);
	already_masked = 1;
    }
}

_issignals_unmask()
{
    if (do_mask) {
	(void)sigprocmask(SIG_SETMASK, &oldmask, NULL);
	already_masked = 0;
    }
}
