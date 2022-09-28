#ifndef lint
static  char sccsid[] = "@(#)rtable_main.c 3.12 96/09/13 Copyr 1991 Sun Microsystems, Inc.";
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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#ifdef SVR4
#ifndef _NETINET_IN_H
#include <netinet/in.h>
#endif /* _NETINET_IN_H */
#include <netconfig.h>
#include <netdir.h>
#include <sys/stropts.h>
#include <tiuser.h>
#include <netdb.h>   /* define MAXHOSTNAMELEN */
#include <sys/systeminfo.h>
#endif "SVR4"
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <rpc/rpc.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <pwd.h>
#include <grp.h>
#include "util.h"
#include "log.h"
#include "garbage.h"
#include "rpcextras.h"

#define strlen cm_strlen

#ifndef	S_IRWXU
#define	S_IRWXU		(S_IRUSR|S_IWUSR|S_IXUSR)
#endif
#ifndef	S_IRWXG
#define	S_IRWXG		(S_IRGRP|S_IWGRP|S_IXGRP)
#endif
#ifndef	S_IRWXO
#define	S_IRWXO		(S_IROTH|S_IWOTH|S_IXOTH)
#endif

#define S_MASK  (S_INPUT|S_HIPRI|S_ERROR|S_HANGUP)

extern int debug, debug1;
static int standalone;			/* default is 0 */

extern void tableprog_2();
extern void init_alarm();

SVCXPRT *transp;
char	*pgname;
static char *dir = DEFAULT_DIR;
int daemon_gid, daemon_uid;

static void
parse_args(argc, argv)
	char **argv;
{
	int	opt;

	if (pgname = strrchr (argv[0], '/'))
		pgname++;
	else
		pgname = argv[0];
	while ((opt = getopt (argc, argv, "dsx")) != -1)
	{
		switch (opt)
		{
		case 'd':
			debug = 1;
			break;
		case 'x':
			debug  = 1;
			debug1= 1;
			break;
		case 's':
			standalone = 1;
			break;
		case '?':
			fprintf (stderr, "Usage: %s [-ds]\n", pgname);
			exit (-1);
		}
	}
}

static void
init_dir()
{
	char msgbuf[BUFSIZ];
	int create_dir;
	struct stat info;
	mode_t mode;

	if (geteuid() != 0)
	{
		fprintf (stderr,
			"%s: must be run in super-user mode!  Exited.\n",
			pgname);
		exit (-1);
	}

	create_dir = 0;
	if (stat(dir, &info))
	{
		/* if directory does not exist, create the directory */
		if ((errno != ENOENT) || mkdir(dir, S_IRWXU|S_IRWXG|S_IRWXO))
		{
			if (errno == ENOENT)
				sprintf(msgbuf, "%s: cannot create %s.\n%s: %s",
					pgname, dir, pgname, "System error");
			else
				sprintf(msgbuf, "%s: cannot access %s.\n%s: %s",
					pgname, dir, pgname, "System error");
			perror (msgbuf);
			exit (-1);
		}
		create_dir = 1;
	}

	/* if dir is just created, we need to do chmod and chown.
	 * Otherwise, only do chmod and/or chown if permssion and/or
	 * ownership is wrong.
	 */
	mode = S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO;

	if (create_dir || info.st_mode != (mode | S_IFDIR)) {

		/* set directory permission to be "rwxrwsrwt" */
		if (chmod(dir, mode)) {
			sprintf(msgbuf, "%s: Permission on %s%s\n%s%s\n%s%s",
				pgname, dir,
				" is wrong but cannot be corrected.", pgname,
				": This might happen if you are mounting the directory.",
				pgname, ": System error");
			perror(msgbuf);
			if (create_dir)
				rmdir(dir);
			exit(-1);
		}
	}

	if (create_dir || info.st_uid!=daemon_uid || info.st_gid!=daemon_gid) {
		/* set directory ownership to: owner = 1, group = 1 */
		if (chown(dir, daemon_uid, daemon_gid)) {
			sprintf(msgbuf, "%s: Ownership on %s%s\n%s%s\n%s%s",
				pgname, dir,
				" is wrong but cannot be corrected.", pgname,
				": This might happen if you are mounting the directory.",
				pgname, ": System error");
			perror(msgbuf);
			if (create_dir)
				rmdir(dir);
			exit(-1);
		}
	}

	/* Change current directory, so core file can be dumped. */
	chdir (dir);
}

/* if everything works fine, return 0; otherwise, return -1 */
int
check_mode (file,uid,gid,mode)
char	*file;
int	uid;
int	gid;
mode_t	mode;
{
	char msgbuf[BUFSIZ];
	struct stat info;
	char	*newf;
	int	ret = 0;

	if (stat(file, &info) == 0)
	{
		seteuid (0);

		/* If file ownership is incorrect, it may be a Trojan horse. */
		if (info.st_uid != uid)
		{
			newf = ckalloc (cm_strlen(file)+2);
			cm_strcpy (newf, file);
			cm_strcat (newf, "-");
			if (rename (file, newf)) {
				sprintf(msgbuf, "%s: Ownership of %s is wrong (owner=%d) and renaming %s to %s failed.\n%s: System error",
					pgname, file, info.st_uid, file, newf,
					pgname); 
				perror(msgbuf);
				ret = -1;
			} else {
				fprintf (stderr,
				"%s: %s (owner=%d) has been renamed to %s.\n",
				pgname, file, info.st_uid, newf);
			}
			free (newf);
		}
		else
		{
			if ((info.st_mode & mode) != mode)
			{
				/* self correct the permission */
				if (chmod (file, mode) < 0) {
					sprintf(msgbuf, "%s: %s%s%s.\n%s%s",
					    pgname,
					    "Ownership of ", file,
					    " is wrong but cannot be corrected",
					    pgname, ": System error");
					perror(msgbuf);
					ret = -1;
				}
			}
			if (ret == 0 && info.st_gid != gid)
			{
				/* self correct the group id */
				if (chown (file, uid, gid) < 0) {
					sprintf(msgbuf, "%s: %s%s%s.\n%s%s",
					    pgname,
					    "Group ownership on ", file,
					    " is wrong but cannot be corrected",
					    pgname, ": System error");
					perror(msgbuf);
					ret = -1;
				}
			}
		}

		seteuid (daemon_uid);
	}

	return(ret);
}

/* if everything works fine, return 0; otherwise, return -1 */
int
set_mode (file,uid,gid,mode)
char	*file;
int	uid;
int	gid;
mode_t	mode;
{
	int	error;
	char	buff[BUFSIZ];

	seteuid (0);
	if (chmod (file, mode) < 0)
	{
		error = errno;
		sprintf (buff, "%s: chmod %s to %o failed.\n%s: System error",
			pgname, file, mode, pgname);
		errno = error;
		perror (buff);
		return(-1);
	}
	if (chown (file, uid, gid) < 0)
	{
		error = errno;
		sprintf (buff, "%s: chown %s to (uid=%d,gid=%d) failed.\n%s%s",
			pgname, file, uid, gid, pgname, ": System error");
		errno = error;
		perror (buff);
		return(-1);
	}
	if (seteuid (daemon_uid) < 0)
	{
		error = errno;
		sprintf (buff, "%s: Can't switch process uid back to daemon.\n%s%s",
			pgname, pgname, ": System error");
		errno = error;
		perror (buff);
	}
	return(0);
}

static
lock_it()
{
	char	buff [MAXPATHLEN];
	int	fd;
#ifdef SVR4
	char host[MAXHOSTNAMELEN];
	struct flock locker;
	locker.l_type = F_WRLCK;
	locker.l_whence = 0;
	locker.l_start = 0;
	locker.l_len = 0;
#endif /* SVR4 */

	cm_strcpy (buff, dir);
#ifdef SVR4
	/* 
	   /var/spool might be mounted.  Use .lock.hostname to
	   prevent more than one cms running in each host.
	*/
	(void)sysinfo(SI_HOSTNAME, host, sizeof(host));
	cm_strcat (buff, "/.lock.");
	cm_strcat(buff, host);
#else
	cm_strcat (buff, "/.lock");
#endif /* SVR4 */

/*
** Bug 1265008: Potential for race condition enabling user to create
** root-owned lock file in arbitrary directory.
*/

	fd = open(buff, O_WRONLY|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
	if (fd < 0)
	{
		perror (buff);
		exit (-1);
	}

	/* Note, we have to use flock() instead of lockf() because cms process
	 * is run in each host.
	 */
#ifdef SVR4
	if (fcntl (fd, F_SETLK, &locker) != 0)
#else
	if (flock (fd, LOCK_EX|LOCK_NB) != 0)
#endif "SVR4"
	{
		if (errno != EWOULDBLOCK)
		{
			perror ("rpc.cmsd: flock");
			exit (-1);
		} else	fprintf(stderr, "cm: rpc.cmsd is already running.\n");
		/* cms has been running.... */
		exit(0);
	}

	return (fd);
}

static void
program(rqstp, transp)
        struct svc_req *rqstp;
        register SVCXPRT *transp;
{
        char *result;
        char *argument = NULL;
        program_handle ph = getph();
        struct rpcgen_table *proc;

        /* first do some bounds checking: */
        if (rqstp->rq_vers >= ph->nvers) {
                svcerr_noproc(transp);
                return;
        }
        if (ph->prog[rqstp->rq_vers].nproc == 0) {
                svcerr_noproc(transp);
                return;
        }
        if (rqstp->rq_proc >= ph->prog[rqstp->rq_vers].nproc) {
                svcerr_noproc(transp);
                return;
        }
        /* assert - the program number, version and proc numbers are valid */
        proc = &(ph->prog[rqstp->rq_vers].vers[rqstp->rq_proc]);
        argument = (char*)calloc(proc->len_arg, sizeof(char));
        if (!svc_getargs(transp, proc->xdr_arg, argument)) {
                svcerr_decode(transp);
                return;
        }
        result = (*proc->proc)(argument, rqstp);
        if (result != NULL && !svc_sendreply(transp, proc->xdr_res, result)) {
                svcerr_systemerr(transp);
        }

        if (!svc_freeargs(transp, proc->xdr_arg, argument)) {
                (void)fprintf(stderr, "unable to free arguments");
                exit(1);
        }
        free(argument);
        return;
}

main(argc, argv)
	char **argv;
{
	CLIENT *cl;
        u_long version;
        program_handle ph = newph();
	struct passwd *pw;
	struct group *gr;
	struct rlimit rl;
	struct sockaddr_in saddr;
	int asize = sizeof (saddr);
	SVCXPRT *tcp_transp = (SVCXPRT *)-1;
        SVCXPRT *udp_transp = (SVCXPRT *)-1;
#ifdef SVR4
	struct netconfig *nconf_udp;
	struct netconfig *nconf_tcp;
	static int clntio_fd;
	extern int t_errno;
#endif "SVR4"

	pw = (struct passwd *)getpwnam("daemon");
	gr = (struct group *)getgrnam("daemon");
	if (pw != NULL) 
		daemon_uid = pw->pw_uid;
	else
		daemon_uid = 1;
	if (gr != NULL)
		daemon_gid = gr->gr_gid;
	else 
		daemon_gid = 1;

	parse_args(argc, argv);

	/* check to see if we are started by inetd */
	if (getsockname(0, (struct sockaddr *)&saddr, &asize) == 0) {
		/*
		 * If it is started by inetd, make stderr to be
		 * output to console.
		 */

		int	fd;

		standalone = 0;

		if ((fd = open ("/dev/console", O_WRONLY)) >= 0) {
			close(2);
			if (fd != 2)
				dup2(fd, 2);
			close (fd);
		}
	} else
		standalone = 1;

	/* Set up private directory and switch euid/egid to daemon. */
	umask (S_IWOTH);
	init_dir();

	/* do this after we get the transport 
	setegid (daemon_gid);
	seteuid (daemon_uid);
	*/

	/* Don't allow multiple cms processes running in the same host. */
	(void)lock_it();

	/* raise the soft limit of number of file descriptor */
	/* this is to prevent the backend from running out of open file des */
	getrlimit(RLIMIT_NOFILE, &rl);
	if (rl.rlim_cur < rl.rlim_max) {
		rl.rlim_cur = rl.rlim_max;
		setrlimit(RLIMIT_NOFILE, &rl);
	}

/* -----------------------------------------------------------------
   **** This part has not been ported to SVR4 - [vmh] ****

	transp = svcudp_create(standalone ? RPC_ANYSOCK : 0, 0, 0);
	if (transp == NULL) {
		(void)fprintf(stderr, "cannot create udp service.\n");
		exit(1);
	}

	if (standalone)
		(void)pmap_unset(TABLEPROG, TABLEVERS);

	if (!svc_register(transp, TABLEPROG, TABLEVERS,
		tableprog_2, standalone ? IPPROTO_UDP : 0)) {
		(void)fprintf(stderr,
			"unable to register (TABLEPROG, TABLEVERS, udp).\n");
		exit(1);
	}
--------------------------------------------------------------------*/

#ifdef SVR4
	nconf_udp = getnetconfigent("udp");
	nconf_tcp = getnetconfigent("tcp");

	if (!standalone)
	{
		char	mname[FMNAMESZ+1];

		/* Brought up by inetd, make sure fd 0 is TLI fd, not socket */
		if (ioctl(0, I_LOOK, mname) != 0) {
			perror("rtable_main.c: ioctl gets module name");
			exit(1);
		}
		if (strcmp(mname, "sockmod") == 0) {
			/* Change socket fd to TLI fd */
			if (ioctl(0, I_POP, 0) || ioctl(0, I_PUSH, "timod")) {
				perror("rtable_main.c: ioctl I_POP/I_PUSH");
				exit(1);
			}
		} else if (strcmp(mname, "timod") != 0) {
			fprintf(stderr, "rtable_main.c: fd 0 is not timod\n");
			exit(1);
		}
	}

	for (version = 0; version < ph->nvers; version++) {
		/* don't register unsupported versions: */
		if (ph->prog[version].nproc == 0) continue;

		if (standalone)
			rpcb_unset(ph->program_num, version, NULL);

		/* brought up by inetd, use fd 0 which must be a TLI fd */
		if (udp_transp == (SVCXPRT *)-1) {
                        udp_transp = svc_tli_create(standalone ? RPC_ANYFD : 0,
                                nconf_udp, (struct t_bind*) NULL, 0, 0);

                        if (udp_transp == NULL) {
                                t_error("rtable_main.c: svc_tli_create(udp)");
                                exit(2);
                        }
                }


		if (svc_reg(udp_transp, ph->program_num, version, program,
				standalone ? nconf_udp : NULL) == 0) {
			t_error("rtable_main.c: svc_reg");
			exit(3);
		}

		/* Set up tcp for calls that potentially return */
		/* large amount of data.  This transport is not */
		/* registered with inetd so need to register it */
		/* with rpcbind ourselves.			*/

		rpcb_unset(ph->program_num, version, nconf_tcp);

		if (tcp_transp == (SVCXPRT *)-1) {
                        tcp_transp = svc_tli_create(RPC_ANYFD, nconf_tcp,
                                        (struct t_bind *)NULL, 0, 0);
                        if (tcp_transp == NULL) {
                                t_error("rtable_main.c: svc_til_create(tcp)");
                                exit(2);
                        }
                }

		if (svc_reg(tcp_transp, ph->program_num, version, program,
				nconf_tcp) == 0) {
			t_error("rtable_main.c: svc_reg(tcp)");
			exit(3);
		}
	}/*for*/

	if (nconf_udp)
		freenetconfigent(nconf_udp);
	if (nconf_tcp)
		freenetconfigent(nconf_tcp);

#else
	for (version = 0; version < ph->nvers; version++) {
		/* don't register unsupported versions: */
		if (ph->prog[version].nproc == 0) continue;

		if (standalone)
			(void) pmap_unset(ph->program_num, version);

		transp = svcudp_create(standalone ? RPC_ANYSOCK : 0,0,0);
		if (transp == NULL) {
			(void)fprintf(stderr,
				"rtable_main.c: cannot create udp service.\n");
			exit(1);
                }

		if (!svc_register(transp, ph->program_num, version, program,
				standalone ? IPPROTO_UDP : 0)) {
			(void)fprintf(stderr, "rtable_main.c: unable to register");
			exit(1);
		}

		/* Set up tcp for calls that potentially return */
		/* large amount of data.  This transport is not */
		/* registered with inetd so need to register it */
		/* with rpcbind ourselves.			*/

		transp = svctcp_create(RPC_ANYSOCK, 0, 0);
		if (transp == NULL) {
			(void)fprintf(stderr,
				"rtable_main.c: cannot create tcp service.\n");
			exit(1);
		}

		if (!svc_register(transp, ph->program_num, version, program,
				IPPROTO_TCP)) {
			(void)fprintf(stderr, "rtable_main.c: unable to register(tcp)");
			exit(1);
		}
	}

#endif "SVR4"

	setegid (daemon_gid);
	seteuid (daemon_uid);

	/* my custom stuff */
	init_time();
	init_alarm();
	svc_run();
	(void)fprintf(stderr, "svc_run returned\n");
	exit(1);
}
