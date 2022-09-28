#ifdef lint
#ifdef sccs
static char sccsid[]="@(#)lockf.c	3.6 02/08/93 Copyright 1987-1990 Sun Microsystems, Inc.";
#endif   
#endif

/*
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */ 

#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/file.h>
#ifdef SVR4
#include <sys/fcntl.h>
#endif SVR4
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "folder.h"

#define	DEBUG_FLAG mt_debugging
extern DEBUG_FLAG;
#include "debug.h"

#ifdef	TOOLTALK_LOCK

#include <desktop/tt_c.h>

#ifndef	DEBUG
#define	CP(fmt)
#else
#define	CP(fmt)			{if( DEBUG_FLAG ) chkrc fmt; }
#define	ERR_LAST_STRING		{ -1, NULL }
#ifdef __STDC__
#define	ERR_STRING(x)		{ (int) x, #x }
#else
#define	ERR_STRING(x)		{ (int) x, "x" }
#endif __STDC__

typedef	struct
{	int	ecode;
	char	*estring;
} TT_String;

TT_String TT_State[] = {
	ERR_STRING(TT_CREATED),
	ERR_STRING(TT_SENT),
	ERR_STRING(TT_HANDLED),
	ERR_STRING(TT_FAILED),
	ERR_STRING(TT_QUEUED),
	ERR_STRING(TT_STARTED),
	ERR_STRING(TT_REJECTED),
	ERR_LAST_STRING,
};

TT_String TT_Status[] = {
        ERR_STRING(TT_OK),
        ERR_STRING(TT_WRN_NOTFOUND),
	ERR_STRING(TT_WRN_STALE_OBJID),
        ERR_STRING(TT_WRN_STOPPED),
	ERR_STRING(TT_WRN_SAME_OBJID),
	ERR_STRING(TT_WRN_START_MESSAGE),
	ERR_STRING(TT_WRN_APPFIRST),
	ERR_STRING(TT_WRN_LAST),
        ERR_STRING(TT_ERR_CLASS),
        ERR_STRING(TT_ERR_DBAVAIL),
        ERR_STRING(TT_ERR_DBEXIST),
	ERR_STRING(TT_ERR_FILE),
	ERR_STRING(TT_ERR_INVALID),
        ERR_STRING(TT_ERR_MODE),
        ERR_STRING(TT_ERR_NOMP),
        ERR_STRING(TT_ERR_NOTHANDLER),
        ERR_STRING(TT_ERR_NUM),
        ERR_STRING(TT_ERR_OBJID),
        ERR_STRING(TT_ERR_OP),
        ERR_STRING(TT_ERR_OTYPE),
        ERR_STRING(TT_ERR_ADDRESS),
        ERR_STRING(TT_ERR_PATH),
        ERR_STRING(TT_ERR_POINTER),
        ERR_STRING(TT_ERR_PROCID),
        ERR_STRING(TT_ERR_PROPLEN),
        ERR_STRING(TT_ERR_PROPNAME),
        ERR_STRING(TT_ERR_PTYPE),
        ERR_STRING(TT_ERR_DISPOSITION),
        ERR_STRING(TT_ERR_SCOPE),
        ERR_STRING(TT_ERR_SESSION),
        ERR_STRING(TT_ERR_VTYPE),
	ERR_STRING(TT_ERR_NO_VALUE),
	ERR_STRING(TT_ERR_INTERNAL),
	ERR_STRING(TT_ERR_READONLY),
	ERR_STRING(TT_ERR_NO_MATCH),
	ERR_STRING(TT_ERR_UNIMP),
	ERR_STRING(TT_ERR_APPFIRST),
	ERR_STRING(TT_ERR_LAST),
        ERR_STRING(TT_STATUS_LAST),
	ERR_LAST_STRING,
};

static void
chkrc (sl, rc, fmt, a, b, c, d, e, f)
TT_String *sl;
Tt_status rc;
char *fmt;
{
	TT_String *p_stat;

	DP(("%d: ", getpid()));
	for (p_stat = sl; p_stat->estring != NULL; p_stat++)
	{
		if (p_stat->ecode == rc)
		{
			DP(("%s: ", p_stat->estring));
			DP((fmt, a, b, c, d, e, f));
			return;
		}
	}
	DP(("%d: Unknown value: %d\n", getpid(), rc));
}
#endif	DEBUG

static Tt_message
msg_create (op, file, class)
char *op;
char *file;
Tt_class class;
{
	Tt_message msg;
	Tt_status tt_status;

	/* Create the tooltalk message */
	if ((tt_status = tt_ptr_error(msg = tt_message_create())) != TT_OK) {
		CP((TT_Status, tt_status, "Couldn't create tooltalk message.\n"));
		return ((Tt_message) NULL);
	}

	/* Set the message class type */
	if ((tt_status = tt_message_class_set (msg, class)) != TT_OK) {
		CP((TT_Status, tt_status, "Couldn't set tooltalk message class.\n"));
		return ((Tt_message) NULL);
	}

	/* Set the message address */
	if ((tt_status = tt_message_address_set (msg, TT_PROCEDURE)) != TT_OK) {
		CP((TT_Status, tt_status, "Couldn't set tooltalk message paradigm.\n"));
		return ((Tt_message) NULL);
	}

	/* Set the disposition of the message */
	if ((tt_status = tt_message_disposition_set (msg, TT_DISCARD)) != TT_OK) {
		CP((TT_Status, tt_status, "Couldn't set tooltalk message reliability.\n"));
		return ((Tt_message) NULL);
	}

	/* Set the message operation. */
	if ((tt_status = tt_message_op_set (msg, op)) != TT_OK) {
		CP((TT_Status, tt_status, "Couldn't set tooltalk message.\n"));
		return ((Tt_message) NULL);
	}

	/* Set the message scope */
	if ((tt_status = tt_message_scope_set (msg, TT_FILE)) != TT_OK) {
		CP((TT_Status, tt_status, "Couldn't set tooltalk message scope\n"));
		return ((Tt_message) NULL);
	}

	if ((tt_status = tt_message_file_set (msg, file)) != TT_OK) {
		CP((TT_Status, tt_status, "Couldn't set tooltalk message file.\n"));
		return ((Tt_message) NULL);
	}

	return (msg);
}

/*
 * Current implementation uses Tool-Talk message to lock a folder.  It also
 * uses Tool-Talk to notify the lock holder to give up the lock.
 *
 * FM_LOCK may return:	0 for successful completion.
 *			-1 for unrecoverable error.
 *			-2 if Tooltalk server is not responsing.
 *			1 if a process holds the lock.
 */
int
ttlock_f (folder, cmd, arg)
struct folder_obj *folder;
int cmd;
{
	Tt_message msg;
	Tt_status ttrc;
	Tt_state state;
	Tt_pattern pattern;
	void	(*handle)();
	fd_set	rdmask;
	static int width;
	extern int ds_tt_fd;
	char	*ttlock_op;		/* The tooltalk op */
	int	wrong_message = 0;

	switch (cmd)
	{
	case FM_ULOCK:
		DP(("%d: Unlocking %s\n", getpid(), folder->fo_file_name));
		if ((pattern = (Tt_pattern) folder->fo_lockobj) != NULL)
			tt_pattern_destroy (pattern);
		folder->fo_lockobj = NULL;
		break;

	case FM_LOCK:
		if (folder->fo_lockobj != NULL)
			return (0);

		handle = signal (SIGIO, SIG_IGN);
		msg = msg_create ("tlock", folder->fo_file_name, TT_REQUEST);
		if (msg == NULL)
		{
			return (-2);
		}
		ttrc = tt_message_send (msg);
		tt_message_destroy (msg);
		if (ttrc != TT_OK)
		{
			CP((TT_Status, ttrc, "Can't send tlock msg\n"));
			signal (SIGIO, handle);
			if ((ttrc == TT_ERR_DBAVAIL) || 
			    (ttrc == TT_ERR_FILE)    ||
			    (ttrc == TT_ERR_DBEXIST))
				return (-2);
			else
				return (-1);
		}

		DP(("%d: waiting for reply msg of test_lock\n", getpid()));

		FD_ZERO (&rdmask);
		FD_SET (ds_tt_fd, &rdmask); 
		if (width == 0)
			width = ulimit(4, 0);
		/*
		 * Keep retrieving messages until we get a "tlock"
		 * message.
		 */
		do {
			while (select (width, &rdmask, NULL, NULL, NULL) < 0)
			{
				if (errno == EINTR) {
					FD_SET (ds_tt_fd, &rdmask);
					continue;
				}
				perror ("maillib: select");
				signal (SIGIO, handle);
				return (-1);
			}

			/* ZZZ: there is no queuing facility to handle incoming msgs.
			 * So, we "throw away" all messages until get we the 
			 * one we want (tlock).  This should be fixed later to do 
			 * the "right" thing.
			 */
			msg = tt_message_receive ();
			ttlock_op = tt_message_op(msg);
			if (strcmp(ttlock_op, "tlock") != 0) {
				tt_message_destroy(msg);
				wrong_message = 1;
			} else {
				wrong_message = 0;
			}

		} while (wrong_message);

		signal (SIGIO, handle);
		if (tt_pointer_error (msg) == TT_ERR_NOMP)
		{
			DP(("%d: ToolTalk server is dead!\n", getpid()));
			return (-1);
		}

		/* it is locked */
		state = tt_message_state (msg);
		tt_message_destroy (msg);
		if (state == TT_HANDLED)
		{
			DP(("%d: %s is locked\n",
				getpid(),folder->fo_file_name));
			return (1);
		}
		else if (state != TT_FAILED)
		{
			CP((TT_State, state, "test_lock reply msg state\n"));
		}
		else
		{
			/* state should be TT_FAILED, i.e. not locked */
			DP(("%d: %s is not locked\n",
				getpid(), folder->fo_file_name));
		}

		DP(("%d: registering pattern\n", getpid()));
		pattern = tt_pattern_create();
		ttrc = tt_pattern_category_set (pattern, TT_HANDLE);
		ttrc = tt_pattern_scope_add (pattern, TT_FILE);
		ttrc = tt_pattern_file_add (pattern, folder->fo_file_name);
		ttrc = tt_pattern_register (pattern);
		if (ttrc != TT_OK)
		{
			tt_pattern_destroy (pattern);
			CP((TT_Status, ttrc, 
				"Can't register FM_LOCK pattern\n"));
			return (-1);
		}

		/* lock successfully, save the locking handle */
		folder->fo_lockobj = (void *) pattern;
		break;

	case FM_NOTIFY:
		msg = msg_create ("rulock", folder->fo_file_name, TT_NOTICE);
		if (msg == NULL) 
		{
			return (-1);
		}
		ttrc = tt_message_send (msg);
		tt_message_destroy (msg);
		if (ttrc != TT_OK)
		{
			CP((TT_Status, ttrc, "Can't send FM_NOTIFY message\n"));
			return (-1);
		}
		break;

	default:
		return (-1);
	}

	return (0);
}

#endif TOOLTALK_LOCK

static int
folder_unlock (fd)
int fd;
{
	int error = 0;

#ifdef	FLOCK
	/* Unlock kernel lock */
	if (flock (fd, LOCK_UN) < 0)
		error = 1;
#endif	FLOCK

	/* Unlock network lock */
	if (lockf (fd, F_ULOCK, 0) < 0)
		error = 1;

	return (error);
}


/*
 * This function locks a file through 2 locks: network lock and kernel lock.
 * It returns 0 for success, -1 for unsuccess (remote process or lock error),
 * or the local process id which owns the lock.
 */
static int
folder_lock (fd)
int	fd;
{
	struct flock lb;


	lb.l_whence = 1;
	lb.l_start = 0;
	lb.l_len = 0;
	lb.l_pid = -1;

	/* locking it network-wide */
	while (lockf (fd, F_TLOCK, 0) < 0)
	{
		if ((errno != EACCES) && (errno != EAGAIN))
			return (-1);

		/* find out who has the exclusive lock */
		lb.l_type = F_WRLCK;
	    	if (fcntl (fd, F_GETLK, &lb) < 0)
		{
			/* Unable to find the owner of the lock */
			return (-1);
		}
		/* the lock has just been preempted.  Hurry, go get it! */
		if (lb.l_type == F_UNLCK)
			continue;

#ifdef	FLOCK
		/* use kernel lock to determine if it is a remote process */
		if (flock (fd, LOCK_NB|LOCK_EX) == 0)
		{
			/* lock successfully, so it must be a remote proc */
			flock (fd, LOCK_UN);
			return (-1);
		}

		/* kernel lock failed, it must be a local process */

		/* ask the local process to preempt the lock,  Note, l_pid
		 * should not be same as current pid.
		 */
#endif	FLOCK

		/* since we are not using flock() to determine if it is
		 * a local process, we always assume that it is a local
		 * process until Tool-Talk is ready.
		 */

		return (lb.l_pid);
	}

#ifdef	FLOCK
	/* locking it locally, so we can test if it is a local process */
	if (flock (fd, LOCK_NB|LOCK_EX) < 0)
	{
		lockf (fd, F_ULOCK, 0);
		return (-1);
	}
#endif	FLOCK

	return (0);
}

/*
 * Current implementation uses Unix lockf(3) to lock a folder.  It uses
 * signal to notify the lock holder to give up the lock.
 *
 * FM_LOCK may return:	0 for successful completion.
 *			-1 for unrecoverable error.
 *			pid for the owner who holds the lock.
 */
int
lock_f (folder, cmd, pid)
struct folder_obj *folder;
int cmd;
int pid;
{
	int	fd;

	switch (cmd)
	{
	case FM_ULOCK:
		/* locked? */
		if ((fd = (int) folder->fo_lockobj) > 0)
		{
			folder->fo_lockobj = NULL;
			DP(("folder_unlock (%d)\n", fd));
			return (folder_unlock (fd));
		}
		return (0);

	case FM_LOCK:
		/* locked by myself already? */
		if (((int) folder->fo_lockobj) > 0)
			return (0);
		DP(("folder_lock (%d)\n", folder->fo_fd));
		if ((pid = folder_lock (folder->fo_fd)) == 0)
		{
			/* lock successfully, save the locking handle */
                        folder->fo_lockobj = (caddr_t) folder->fo_fd;
		}
		return (pid);

	case FM_NOTIFY:
		kill (pid, SIGUSR1);
		return (0);

	default:
		return (-1);
	}
}

#ifdef	FLOCK

/* If we use flock() to determine if the locking process is a local process,
 * we can't lock the folder directly because sendmail/Mail both use flock()
 * to lock the folder.  It will cause dead-lock.
 */

char *
lock_file (path)
char *path;
{
	char *lpath;
	char *lfile;

	lpath = (char *) malloc (strlen(path) + 7);
	if (lfile = strrchr (path, '/'))
	{
		strcpy (lpath, path);
		lfile++;
		lpath[(int) (lfile - path)] = '\0';
	}
	else
	{
		lpath[0] = '\0';
		lfile = path;
	}
	strcat (lpath, ".lock.");
	strcat (lpath, lfile);

	return (lpath);
}
#endif	FLOCK
