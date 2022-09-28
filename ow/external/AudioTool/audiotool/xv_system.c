/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)xv_system.c	1.10	97/01/14 SMI"

/*
 * Fork and set up a pipe to read the IO from the forked process.
 * This implementation is for the XView Notifier.
 *
 * hacked from the notify_pipe.c example - i++;
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#ifndef SUNOS41
#include <sys/filio.h>
#include <sys/resource.h>
#endif

#include <xview/xview.h>
#include <xview/notify.h>

#include "atool_types.h"
#include "atool_i18n.h"
#include "atool_debug.h"

#ifndef SUNOS41
#define	wait_type	int
#else /* 4.x */
#define	wait_type	union wait
#endif /* 4.x */

/*
 * Linked list for relevant info about active proc's, keyed by pid of process.
 *
 *                 [0]                           [1]
 *    child reads:  |========= pipe_io[0] ========| <- parent writes
 *   pipe_io[0][0]                                     pipe_io[0][1]
 *
 *    parent reads: |========= pipe_io[1] ========| <- child writes
 *   pipe_io[1][0]                                     pipe_io[1][1]
 *
 * The parent process reads the output of the child process by reading
 * pipe_io[1][0] because the child is writing to pipe_io[1][1].
 * The child process gets its input from pipe_io[0][0] because the
 * parent writes to pipe_io[0][1].  Thus, one process is reading from
 * one end of the pipe while the other is writing at the other end.
 */

struct process_record {
	struct process_record	*next;
	int			pid;
	char			*cmd;
	Notify_client		reader_client;
	Notify_client		writer_client;
	int			read_fd;
	int			write_fd;
	int			pipe_io[2][2];
	void			(*finish_proc)();
};
static struct process_record	*process_list = NULL;

static struct process_record	*find_process(struct process_record*, int);
static struct process_record	*delete_process(struct process_record*, int); 

/*
 * Keep compiler happy, even with stricter typing
 */
typedef struct rusage	struct_rusage;

static int		do_xv_system(char*, void(*)(), caddr_t, int, int*, int);
static Notify_value	read_it(Notify_client, int);
static Notify_value	sigchldcatcher(Notify_client, int, wait_type*,
			    struct_rusage*);
static Notify_client	dummy_client = (Notify_client) 0;


/*
 * Test to see whether the specified command can be found.
 * Returns FALSE if error.  Calls callback with status == 0 if found.
 */
int
test_exec(
	char		*cmd,
	void		(*callback)(),
	caddr_t		client)
{
	char		cmdbuf[2 * MAXPATHLEN];

	/* Create a shell command that will emit 99 if 'not found' */
	sprintf(cmdbuf,
	    "for i in `type %s`; do "
	    "if [ $i = found ] ; then exit 99 ; fi ; "
	    " done ; exit 0",
	    cmd);

	if (do_xv_system(cmdbuf, callback, client, FALSE, NULL, FALSE) < 0)
		return (FALSE);
	return (TRUE);
}

/*
 * Fork and exec the specified process.
 * Returns -1 if error, else 0.
 */
int
atool_exec(
	char		*cmd,
	void		(*callback)(),
	caddr_t		client,
	int		*pid)
{
	return (do_xv_system(cmd, callback, client, FALSE, pid, TRUE));
}

/*
 * Fork and exec the specified process, with a write pipe.
 * Returns -1 if error, else pipe fd.
 */
int
atool_pipe(
	char		*cmd,
	void		(*callback)(),
	caddr_t		client,
	int		*pid)
{
	return (do_xv_system(cmd, callback, client, TRUE, pid, TRUE));
}


/*
 * Fork and exec the specified process, with or without a write pipe.
 * Returns -1 if error, else 0 (or pipe fd).
 */
static int
do_xv_system(
	char			*cmd,
	void			(*callback)(),
	caddr_t			dp,
	int			pflag,
	int			*chpid,
	int			execflag)
{
	char			cmdbuf[4*MAXPATHLEN];
	int			i;
	int			pid;
	int			nofile;
	FILE			*fp;
	struct process_record	*prp;

	if (!(cmd && *cmd))
		return (-1);

	/* Prepend "exec" in front of command since we're starting a shell */
	(void) sprintf(cmdbuf, "%s%s", execflag ? "exec " : "", cmd);

	if (!dummy_client)
		dummy_client = (Notify_client) xv_unique_key();

	DBGOUT((1,"xv_system: %s\n", cmd));

	/* create an exec record and insert into process list */
	prp = (struct process_record *)calloc(1, sizeof(struct process_record));
	if (prp == NULL) {
		DBGOUT((1, "Malloc failed while trying to fork %s\n", cmd));
		return (-1);
	}

	pipe(prp->pipe_io[0]);		/* set up input pipe */
	pipe(prp->pipe_io[1]);		/* set up output pipe */

	switch (pid = fork()) {
	case -1:
		close(prp->pipe_io[0][0]);
		close(prp->pipe_io[0][1]);
		close(prp->pipe_io[1][0]);
		close(prp->pipe_io[1][1]);
		DBGOUT((1, "Fork failed for: %s\n", cmd));
		(void) free((char *)prp);
		return (-1);

	case  0:	/* child */
		/* redirect child's stdin (0) & stdout (1) */
		dup2(prp->pipe_io[0][0], 0);
		dup2(prp->pipe_io[1][1], 1);

		/* XXX - What to do with stderr?  For now, eliminate it */
#ifdef notdef
		dup2(prp->pipe_io[1][1], 2);	/* merge stderr with stdout */
#else
		close(2);			/* close stderr */
#endif

		/* Get number of fds to close */
	#ifndef SUNOS41
		{
			struct rlimit rlp;

			if (getrlimit(RLIMIT_NOFILE, &rlp) < 0)
				nofile = 256;
			else
				nofile = rlp.rlim_cur;
		}
	#else
		nofile = getdtablesize();
	#endif
		for (i = nofile; i > 2; i--) {
			(void) close(i);	/* close unused fd's */
		}
		for (i = 0; i < NSIG; i++) {	/* reset signals */
			(void) signal(i, SIG_DFL);
		}

		execl("/bin/sh", "audiotool-command", "-c", cmdbuf, NULL);
		DBGOUT((1, "Exec failed for: %s", cmd));
		_exit(-1);

	default:	/* parent */
		close(prp->pipe_io[0][0]); /* close unused portions of pipes */
		close(prp->pipe_io[1][1]);
	}

	prp->pid = pid;
	prp->cmd = strdup(cmd);
	prp->reader_client = dummy_client;
	prp->writer_client = (Notify_client) dp;

	/* finish proc called when wait3_func called */
	prp->finish_proc = callback;
	prp->read_fd = prp->pipe_io[1][0]; /* output from child */

	if (pflag) {
		prp->write_fd = prp->pipe_io[0][1];	/* writing to child */
	} else {
		prp->write_fd = -1;		/* NOT writing to child */
		close(prp->pipe_io[0][1]);	/* don't need child input fd */
	}

	notify_set_input_func(prp->reader_client, read_it, prp->read_fd);
	notify_set_wait3_func(prp->reader_client, sigchldcatcher, prp->pid);

	#ifdef notdef
	/* XXX -  not reading from our stdin... */
	/* wait for user input -- then write data to pipe */
	notify_set_input_func(prp->writer_client, write_it, 0);
	/* why set a wait3 for both clients? */
	notify_set_wait3_func(prp->writer_client, sigchldcatcher, prp->pid);
	#endif

	prp->next = process_list;
	process_list = prp;

	/* if caller is interested in child PID */
	if (chpid) {
		*chpid = pid;
	}

	if (pflag) {
		return (prp->write_fd);
	} else {
		return (0);
	}
}

/*
 * callback routine for when there is data on the child's stdout to
 * read.  Read, then write the data to stdout (owned by the parent).
 */
static Notify_value
read_it(
	Notify_client	client,
	int		fd)
{
	char		buf[BUFSIZ];
	int		bytes;
	int		i;

	if (ioctl(fd, FIONREAD, &bytes) == 0) {
		while (bytes > 0) {
			if ((i = read(fd, buf, sizeof buf)) > 0) {
				DBGOUT((1,"[Read %d bytes from fd=%d]\n",
				    i, fd));
				write(1, buf, i); /* write to stdout ... */
				bytes -= i;
			}
		}
	}
	return (NOTIFY_DONE);
}

/*
 * handle the death of the child.  If the process dies, the child
 * dies and generates a SIGCHLD signal.  Capture it and disable the
 * functions that talk to the pipes.
 *
 * in the case of the writer client, call the finish_proc with the
 * status and the client....
 *
 */
static Notify_value
sigchldcatcher(
	Notify_client	client, 
	int		pid, 
	wait_type	*status,
	struct_rusage	*rusage)
{
	struct process_record *prp;

	/* find process in proc list */
	if (!(prp = find_process(process_list, pid))) {
		DBGOUT((1, "sigchild: can't find process %d in list\n", pid));
		return (NOTIFY_IGNORED);
	}

	if (WIFEXITED(*status)) {
		if (WEXITSTATUS(*status) != 0) {
			DBGOUT((1,
			    "'%s' (pid %d) exited w/error status %d\n",
			    prp->cmd, pid, WEXITSTATUS(*status)));
		} else {
			DBGOUT((1, "'%s' (pid %d) completed\n", prp->cmd, pid));
			/*
			 * call read_it directly in case there's something left
			 * to read on the pipe
			 */
			read_it(prp->reader_client, prp->read_fd);
		  
		}
	} else if (WIFSIGNALED(*status)) {
		DBGOUT((1,
		    "'%s' (pid %d) terminated w/signal %d (status %d)\n",
		    prp->cmd, pid, WTERMSIG(*status), WEXITSTATUS(*status)));
	}

	if (WIFEXITED(*status) || WIFSIGNALED(*status)) {
		/* unregister input func with appropriate file descriptor */
		notify_set_input_func(prp->reader_client, 
		    NOTIFY_FUNC_NULL, prp->read_fd);

		close(prp->read_fd);

		/*
		 * call finish proc. writer_client is client data.
		 * if died on a signal, exit status is the negative signum
		 */
		if (prp->finish_proc) {
			(*prp->finish_proc)(prp->writer_client,
			    WIFSIGNALED(*status) ? (0 - WTERMSIG(*status)) :
			    WEXITSTATUS(*status));
		}

		/* remove from proc list */
		process_list = delete_process(process_list, pid);

		return (NOTIFY_DONE);
	}
	DBGOUT((1,"SIGCHLD not handled for pid %d\n", pid));
	return (NOTIFY_IGNORED);
}

static struct process_record *
find_process(
	struct process_record	*prp,
	int			pid)
{
	for (; prp; prp = prp->next) {
		if (prp->pid == pid) {
			return (prp);
		}
	}
	return (NULL);
}

static struct process_record *
delete_process(
	struct process_record	*prp,
	int			pid)
{
	struct process_record	*pprp;
	struct process_record	*head;
    
	for (pprp = NULL, head = prp; prp; pprp = prp, prp = prp->next) {
		if (prp->pid == pid) {
			if (pprp) {
				pprp->next = prp->next;
			} else {
				head = prp->next;
			}
			if (prp->cmd) {
				free(prp->cmd);
			}
			free(prp);
			DBGOUT((1,"removed pid %d from proc list\n", pid));
			break;
		}
	}
	return (head);
}

#ifdef notdef
/*
 * callback routine for when there is data on the parent's stdin to
 * read.  Read it and then write the data to the child process via
 * the pipe.
 */
static Notify_value
write_it(
	Notify_client	client,
	int		fd)
{
	char		buf[BUFSIZ];
	int		bytes;
	int		i;

	/* only write to pipe (child's stdin) if user typed anything */
	if (ioctl(fd, FIONREAD, &bytes) == -1 || bytes == 0) {
		notify_set_input_func(client, NOTIFY_FUNC_NULL,
		    prp->pipe_io[0][1]);
		close(prp->pipe_io[0][1]);
	} else {
		while (bytes > 0) {
			if ((i = read(fd, buf, sizeof buf)) > 0) {
				DBGOUT((1,
				    "[Sending %d bytes to pipe (fd=%d)]\n",
				    i, prp->pipe_io[0][1]));
				write(prp->pipe_io[0][1], buf, i);
			} else if (i == -1) {
				break;
			}
		bytes -= i;
		}
	}
	return (NOTIFY_DONE);
}
#endif
