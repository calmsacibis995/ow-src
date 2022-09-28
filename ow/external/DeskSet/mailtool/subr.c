#ifndef lint
static  char sccsid[] = "@(#)subr.c 3.4 94/04/13 Copyr 1987 Sun Micro";
#endif

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

/*
 * Mailtool - miscellaneous subroutines
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#ifdef SVR4
#include <signal.h>
#endif SVR4
#include <sys/stat.h>
#include "mail.h"
#include "mle.h"

char	*strcpy();

/*
 * Save a copy of a string.
 */
char *
mt_savestr(s)
	register char  *s;
{
	register char  *t;
	extern char    *mt_cmdname;

	t = (char *) strdup(s);
	if (t == NULL) {
                /*
                 * Memory allocation failed.  We don't attempt to handle
                 * this gracefully; we just bail out.  The "%s" is the name
                 * of the mail program (as passed into argv[0]); we print
                 * this message to stderr.
                 */
		nomem();
	}
	return (t);
}

u_long
mt_current_time()
{
	struct timeval  tv;

	gettimeofday(&tv, NULL);
	return (tv.tv_sec);
}


/* is s1 a proper prefix of s2 */
int
mt_is_prefix(s1, s2)
	register char  *s1, *s2;
{
	while (*s1 != '\0' && *s2 != '\0')
		if (*s1++ != *s2++)
			return (0);
	return (1);
}



mt_system(s)
char	*s;
{
	int	status, pid, w;
	register void (*istat)(), (*qstat)();

	/*
	 * Same as system(3) but returns -1 immediately if fork failed
	 */
	if((pid = vfork()) == 0) {
		(void) execl("/bin/sh", "sh", "-c", s, (char *)0);
		_exit(127);
	} else if (pid < 0) {
		return(pid);
	}

	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	while((w = wait(&status)) != pid && w != -1)
		;
	(void) signal(SIGINT, istat);
	(void) signal(SIGQUIT, qstat);
	return((w == -1)? w: status);
}

