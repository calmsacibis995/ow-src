#ident "@(#)xvutils.cc	1.2 93/02/15 Copyright 1992 Sun Microsystems, Inc."


#include <doc/utils.h>
#include <xview/xview.h>
#include "dvlocale.h"
#include <poll.h>


// XView-safe version of "sleep(3)".
//
STATUS
XvSleep(int secs, ERRSTK &err)
{
	sigset_t	oldmask;
	sigset_t	mask;
	struct pollfd	fds;


	assert(secs >= 0);
	DbgFunc("XvSleep: " << secs << " seconds" << endl);


	// Effectively shut down the XView notifier by
	// disabling (almost) all signals.
	//
	(void) sigemptyset(&mask);
	(void) sigaddset(&mask, SIGPOLL);
	(void) sigaddset(&mask, SIGALRM);

	if (sigprocmask(SIG_BLOCK, &mask, &oldmask) != 0) {
		err.Init(DGetText("XView sleep failed (sig_block): %s"),
			SysErrMsg(errno));
		return(STATUS_FAILED);

	}


	// Polling on an inactive file descriptor (stdin) will
	// cause our process to block for the desired length of time.
	//
	fds.fd = 0;
	fds.events = POLLRDNORM;
	fds.revents = 0;

	if (poll(&fds, 1, secs*1000) != 0) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, NULL);
		err.Init(DGetText("XView sleep failed (poll): %s"),
			SysErrMsg(errno));
		return(STATUS_FAILED);
	}


	// Restore old signal mask, reactivating the notifier.
	//
	if (sigprocmask(SIG_SETMASK, &oldmask, NULL) != 0) {
		err.Init(DGetText("XView sleep failed (sig_setmask): %s"),
			SysErrMsg(errno));
		return(STATUS_FAILED);
	}


	return(STATUS_OK);
}
