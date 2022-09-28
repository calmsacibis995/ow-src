#ident "@(#)folder.c	3.23 03/05/97 Copyright 1987-1993 Sun Microsystems, Inc."


/* folder.c -- implement the folder object */


#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "folder.h"
#include "msg.h"
#include "misc.h"
#include "ck_strings.h"
#include "assert.h"
#include "bool.h"
#include "buffer.h"


static int fm_add(char *path, int (*func)(), void *obj, void *arg);
static void fm_delete(struct folder_obj *);
static void fm_free(struct folder_obj *);
static int fm_write(struct folder_obj *);
static int fm_reread(struct folder_obj *);
static struct folder_obj *fm_read(char *path, int (*func)(), void *arg,
	gid_t newgid);
static unsigned long fm_enumerate(struct folder_obj *, FmEnumerateFunc, ...);
static int fm_newmail(char *pathname);
static int fm_corrupt(struct folder_obj *);
static int fm_modified(struct folder_obj *);
static int fm_lock(struct folder_obj *, int cmd, void *arg);
static struct folder_obj *fm_init(char *buf, char *bufend);

#ifdef DEBUG
void fm_dump();
#endif DEBUG

extern	char *dgettext();

/* * S_ISLNK() is not defined in 4.0 */
#ifndef S_ISLNK
#define	S_ISLNK(m)      (((m)&S_IFMT) == S_IFLNK)
#endif

#define DEBUG_FLAG mt_debugging
extern int DEBUG_FLAG;
#include "debug.h"

extern int  lock_f();
extern char *mt_value();
extern char *skin_arpa();


struct __folder_methods folder_methods = {
	fm_add,
	fm_delete,
	fm_free,
	fm_write,
	fm_reread,
	fm_read,
	fm_enumerate,
	fm_newmail,
	fm_corrupt,
	fm_modified,
	fm_lock,
	fm_init,
};


/* forward defintions for local routines */
static int fm_delete_msg(struct msg *);
static unsigned long fm_delete_msg_enumerate(struct msg *, va_list);
static void fm_append_msg();
static int lock();
static void unlock();
static int update_atime();
static void fm_add_messages();
static char *fm_find_msg();
static char *map_buffer();
static struct fm_stat *find_fmstat();

extern char *findchar();

extern char	Msg_Header[];

#define MSG_HEADER_SIZE	5
#define STREQ(a,b)	(!strcmp((a),(b)))
#define STRNEQ(a,b,c)	(!strncmp((a),(b),(c)))
#define PRIV(f,x)	(setgid((f)->fo_egid), (x), setgid((f)->fo_rgid))

/* Folder state for new mail. */
struct fm_stat {
	char	*fo_file_name;
	time_t	fo_mod_time;
	off_t	fo_size;
	struct fm_stat *fo_next;
};

/**************************************************************************
 *
 * EXTERNAL ENTRY POINTS
 *
 *************************************************************************/


/*
 * Append an object "obj" to a folder "path" using the writing "func":
 *	int
 *	func(obj, arg, fi)
 *	void *obj;
 *	void *arg;
 *	FILE *fi;
 * where "fi" is the file pointer of the folder path.  Example, "func" is
 * mm_write_msg(), "obj" is "msg", "arg" is the "suppress_hdrs".
 */
static int
fm_add(
	char *path,
	int (*func)(),
	void *obj,
	void *arg
)
{
	int	fd;
	int	ecode;
	int	error = 0;
	FILE	*fp;
	char	tmppath[MAXPATHLEN];
	struct folder_obj folder;
	static  gid_t egid = -1;
	static  gid_t rgid;

	if (egid == -1) {
	    egid = getegid();
	    rgid = getgid();
	}

	folder.fo_compressed = fm_uncompress(path, tmppath);
	if (folder.fo_compressed)
		path = tmppath;

	folder.fo_locked = 0;
	folder.fo_file_name = path;
	folder.fo_egid = egid;
	folder.fo_rgid = rgid;

	if (lock (&folder)) {
		ecode = 1;
		error = errno;
		goto lock_error;
	}

	/* Default the newly created record file not to be publicly readable. */
	fd = open (path, O_CREAT|O_RDWR, 0600);
	if (fd < 0)
	{
		ecode = 1;
		error = errno;
	}
	else
	{
		fp = fdopen (fd, "r+");
		if (fp == NULL)
		{
			ecode = 1;
			error = errno;
			close (fd);
		}
		else
		{
			/* Goto the end of file */
			fseek (fp, 0L, 2);
			ecode = (*func)(obj, arg, fp);
			if (ecode != 0)
				error = errno;
			if ((fclose (fp) != 0) && (ecode == 0))
			{
				ecode = 2;
				error = errno;
			}
		}
	}

	unlock (&folder);

lock_error:
	if (folder.fo_compressed) {
		fm_compress(&folder);
	}

	errno = error;
	return (ecode);
}





static void
fm_delete(
	struct folder_obj *folder
)
{
	if (folder != NULL)
	{
		folder_methods.fm_free(folder);

		PRIV(folder, unlink(folder->fo_file_name));
	}
}







static void
fm_free(
	struct folder_obj *folder
)
{
	if( ! folder ) return;

	DP(("fm_free (%s) %x\n", folder->fo_file_name, folder));

	/* make sure the folder is unlocked */
	unlock(folder);

	/* unlock the exclusive access */
	if (folder->fo_ttlocked) {
		(void) ttlock_f(folder, FM_ULOCK, NULL);
	} else {
		(void) PRIV(folder, lock_f(folder, FM_ULOCK, NULL));
	}

	(void) folder_methods.fm_enumerate( folder, fm_delete_msg_enumerate);

	/* make sure the folder is compressed again */
	(void) fm_compress(folder);

	if (folder->fo_file_name) {
		ck_free(folder->fo_file_name);
	}

	if (folder->fo_fd >= 0) {
		close(folder->fo_fd);
	}

	while (folder->fo_buffer) {
		struct buffer_map *next;
		struct buffer_map *bm;

		bm = folder->fo_buffer;
		next = bm->bm_next;

		munmap( bm->bm_buffer, bm->bm_size );

		ck_free(bm);
		folder->fo_buffer = next;
	}

	ck_free( folder );
}





static int
fm_write_msg(
	struct msg *m,
	FILE *f
)
{
	int type;
	int (*p_func)();

	if (!m->mo_deleted) {
		type = (int) msg_methods.mm_get(m, MSG_TYPE);
#ifdef	RFC_MIME
		if (type == MSG_TEXT)
			p_func = msg_methods.mm_write_msg;
		else if (type == MSG_ATTACHMENT || type == MSG_MULTIPART)
			p_func = msg_methods.mm_write_attach;
		else
		{
			/* MSG_OTHER */
			p_func = msg_methods.mm_write_other;
		}
#else
		if (type == MSG_ATTACHMENT)
			p_func = msg_methods.mm_write_attach;
		else
		{
			/* MSG_TEXT */
			p_func = msg_methods.mm_write_msg;
		}
#endif	MIME

		return ((*p_func)(m, MSG_FULL_HDRS|MSG_DONT_TST, f));
	}
	return (0);
}


static unsigned long
fm_write_msg_enumerate(
	struct msg *m,
	va_list ap
)
{
	FILE *f = va_arg(ap, FILE *);

	return (fm_write_msg(m, f));
}




static unsigned long
fm_mark_msg(
	struct msg *m,
	va_list ap
)
{
	m->mo_new = 0;
	return (0);
}





/*
 * ZZZ: Can't handle in-memory folder (see fm_init).
 *
 * write the folder out to disk, message by message
 *
 * return 0 for success, != 0 for failure
 */
static int
fm_write(
	struct folder_obj *folder
)
{
	char buf[BUFSIZ];
	char tmpname[MAXPATHLEN];
	char fname[MAXPATHLEN];
	int len;
	int fd;
#ifdef SVR4
	sigset_t mask;
	sigset_t oldmask;
#else
	int mask;
#endif "SVR4"
	int newmail;
	int status;
	struct stat statbuf;
	FILE *f;

	if (folder == NULL)
		return (0);

	if (folder->fo_file_name == NULL)
		return (0);

	DP(("fm_write('%s')\n", folder->fo_file_name));

	if (fm_modified(folder) == 0)
	{
		if (folder->fo_compressed)
			(void) fm_compress(folder);
		return (0);
	}

	/* figure out the mode of the old file */
	statbuf.st_mode = 0600;	/* in case stat fails */
	strcpy (fname, folder->fo_file_name);
	lstat(fname, &statbuf);

	/* get the physical folder name (i.e. not the symbolic link name) */
	while (S_ISLNK (statbuf.st_mode))
	{
		/* folder is symbolic linked, get the actual file name */
		len = readlink (fname, tmpname, sizeof(tmpname)-1);
		if (len < 0)
		{
			status = errno;
			sprintf (buf, dgettext("SUNW_DESKSET_MAILLIB",
"The mail file %s\n\
is a symbolic link which can not be followed.\n\
Reason: %s\n\
\n\
If you continue, changes you have made to it will be discarded."),
				fname, strerror(status));
			maillib_methods.ml_set(ML_ERRMSG, buf);
			return (status);
		}
		tmpname[len] = '\0';

		/* symbolic link to an absolute path or relative path */
		if (tmpname[0] == '/')
			strcpy (fname, tmpname);
		else
		{
			char *ptr;
			if (ptr = strrchr (fname, '/'))
				strcpy (ptr+1, tmpname);
			else
			{
				strcpy (fname, "./");
				strcat (fname, tmpname);
			}
		}

		/* get the mode of the actual file */
		if (lstat (fname, &statbuf) < 0)
			statbuf.st_mode = 0600;
	}

	/* test if the file is writable */
	if (access (fname, W_OK) < 0)
	{
		status = errno;
		sprintf (buf, dgettext("SUNW_DESKSET_MAILLIB",
"Could not write mail file\n%s\n\
Reason: %s\n\
\n\
If you continue, changes you have made to it will be discarded."),
			fname, strerror(status)),
		maillib_methods.ml_set(ML_ERRMSG, buf);
		return (status);
	}

	/* lock the file */
	if (lock(folder)) return(1);

	/* detect any corruption */
	if (fm_corrupt(folder))
	{
		unlock(folder);
		sprintf (buf, dgettext("SUNW_DESKSET_MAILLIB",
"Mail Tool is confused about the state of your mail file\n%s\n\
and cannot determine how to incorporate the changes to\n\
this mail file. Continue discards all changes to the mail\n\
file, restoring the mail file to its original state.\n\
Cancel will cancel this request to save your changes\n\
to the mail file."),
			folder->fo_file_name);
		maillib_methods.ml_set(ML_ERRMSG, buf);
		return (-1);
	}
		
	/* construct a temporary file name */
	sprintf(tmpname, "%s.%d.tmp", fname, getpid());

#ifdef SVR4
	/* This is POSIX */
	(void) sigemptyset(&mask);	/* Initialize mask */
	(void) sigaddset(&mask, SIGHUP);
	(void) sigaddset(&mask, SIGABRT);
	(void) sigaddset(&mask, SIGUSR1);
	(void) sigaddset(&mask, SIGINT);
	(void) sigaddset(&mask, SIGTERM);
	(void) sigaddset(&mask, SIGUSR2);
	(void) sigaddset(&mask, SIGQUIT);
	(void) sigaddset(&mask, SIGTSTP);
	(void) sigaddset(&mask, SIGALRM);

	(void) sigprocmask(SIG_BLOCK, &mask, &oldmask);
#else
	/* blocking these signals */
	mask = (sigmask(SIGHUP) | sigmask(SIGINT) | sigmask(SIGQUIT) |
	       sigmask(SIGABRT) | sigmask(SIGTERM) | sigmask(SIGTSTP) |
	       sigmask(SIGALRM) | sigmask(SIGUSR1) | sigmask(SIGUSR2));
	mask = sigblock(mask);
#endif "SVR4"

	/* open up a temporary file */
	PRIV(folder, fd = open(tmpname, O_WRONLY | O_CREAT | O_TRUNC, 0600));

	if(fd == -1) {
		status = errno;
		sprintf(buf, dgettext("SUNW_DESKSET_MAILLIB",
"Cannot create\n%s\nReason: %s\n\
Please check the directory permissions and try again.\n\
\n\
If you continue, changes you have made will be discarded."),
			tmpname, strerror(status));
		maillib_methods.ml_set(ML_ERRMSG, buf);
		goto RETURN;
	}
	/* since umask(2) affects the permission on open(2), we have to enforce
	 * the original permission.  Fix 1st half of Bugid 1051581.
	 */

	fchmod (fd, statbuf.st_mode & 06666);

	/* since we can't write directly to the original file, we try to
	 * keep the original uid and gid as much as we can.
	 */
	fchown (fd, fpathconf(fd, _PC_CHOWN_RESTRICTED) ?
		-1 : statbuf.st_uid, statbuf.st_gid);

	f = fdopen(fd, "w");

	if (! f) {
		status = -1;
		PRIV(folder, unlink(tmpname));
		close(fd);

		sprintf(buf, dgettext("SUNW_DESKSET_MAILLIB",
"Internal error: can't create descriptor for\n\
%s\n\
\n\
If you continue, changes you have made will be discarded."),
			tmpname);
		maillib_methods.ml_set(ML_ERRMSG, buf);
		goto RETURN;
	}

	/* mark everything as "old" before writing out msg */
	(void) folder_methods.fm_enumerate(folder, fm_mark_msg);

	/* incorporate new msg */
	newmail = fm_incorporate_msg(folder);
	if (newmail == -1) {
		status = -1;
		PRIV(folder, unlink(tmpname));
		fclose (f);
		sprintf(buf, dgettext("SUNW_DESKSET_MAILLIB",
"Internal error: can't retrieve new mail while updating mail file\n\
%s\n\
\n\
If you continue, changes you have made will be discarded."),
			folder->fo_file_name);
		maillib_methods.ml_set(ML_ERRMSG, buf);
		goto RETURN;
	}

	errno = 0;

	/* write out msg */
	status = folder_methods.fm_enumerate(folder, fm_write_msg_enumerate, f);

	/* Fix for 4029337 - write returns success even if over quota */
	if (errno == EDQUOT) {
		status = 1;
	}

	if (status != 0) {
		PRIV(folder, unlink (tmpname));
		fclose(f);

		/* the write failed.  go see why */
		sprintf(buf, ((status == 1) ?
			/* write error */
			dgettext("SUNW_DESKSET_MAILLIB",
"Cannot save your changes.  (Possibly your disk is full.)\n\
\n\
If you continue, changes you have made will be discarded.") :
			/* unexpeced EOF */
			dgettext("SUNW_DESKSET_MAILLIB",
"Unexpected end of file while writing mail file\n%s\n\
\n\
If you continue, changes you have made to it will be discarded.")),
			folder->fo_file_name);

		maillib_methods.ml_set(ML_ERRMSG, buf);
		status = ENOSPC;
		goto RETURN;
	}

	/* flush out the last write before updating the mod and access time */
	if (fflush (f) != 0) {
		status = ferror(f);
		PRIV(folder, unlink (tmpname));
		fclose(f);
		sprintf(buf, dgettext("SUNW_DESKSET_MAILLIB",
"Cannot save your changes.  (Possibly your disk is full.)\n\
\n\
If you continue, changes you have made will be discarded."));
		maillib_methods.ml_set(ML_ERRMSG, buf);
		goto RETURN;
	}

	/* update the access time to be after the mod time, so we "know"
	 * that we have read the mail
	 */
	if (fstat(fd, &statbuf) != 0)
		folder->fo_size = -1;
	else {
		time_t time_p[2];
		long time();
		struct fm_stat *fmstat;

		/* if there is no new mail from incorporation, update the
		 * access time to be at least one sec after mod time.
		 */
		if (!newmail)
		{
			time_p[0] = time((long *) 0) + 1;
			time_p[1] = statbuf.st_mtime;

			if (time_p[0] <= time_p[1]) {
				/* ususally due to NFS time warps */
				time_p[0] = time_p[1] + 1;
			}

			DP(("fm_write: setting access = %d, modify = %d\n",
				time_p[0], time_p[1]));

			/* access time is at least one sec after mod time */
			utime(tmpname, time_p);
		}

		folder->fo_size = statbuf.st_size;
		folder->fo_mod_time = statbuf.st_mtime;

		/* update the folder status for new mail */
		fmstat = find_fmstat(folder->fo_file_name, FALSE);
		if (fmstat != NULL)
		{
			fmstat->fo_size = statbuf.st_size;
			fmstat->fo_mod_time = statbuf.st_mtime;
		}
	}

	/* Make sure the folder is sync-ed to the disk */
	if (fsync (fd) < 0) {
		status = errno; 
		PRIV(folder, unlink (tmpname));
		fclose(f);

		sprintf(buf, dgettext("SUNW_DESKSET_MAILLIB",
"Writing the mail file %s to disk failed\n\
Reason: %s\n\
\n\
If you continue, changes you have made to it will be discarded."),
			folder->fo_file_name, strerror(status));
		maillib_methods.ml_set(ML_ERRMSG, buf);
		goto RETURN;
	}

	fclose(f);

	/* we succeeded! rename the mail folder */
	PRIV(folder, status = rename(tmpname, fname));
	if (status < 0) {
		status = errno;
		PRIV(folder, unlink(tmpname));

		sprintf(buf, dgettext("SUNW_DESKSET_MAILLIB",
"Internal error: can't replace the mail file\n%s\n\
Reason: %s\n\
\n\
If you continue, changes you have made to it will be discarded."),
			folder->fo_file_name, strerror(status));
		maillib_methods.ml_set(ML_ERRMSG, buf);
		goto RETURN;
	}


	/* mark the folder as "clean" */
	folder->fo_changed = 0;
	status = 0;

RETURN:
	/* compress the folder if it was uncompressed in fm_read() */
	if (folder->fo_compressed)
		(void) fm_compress(folder);

	unlock(folder);

#ifdef SVR4
	sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
#else
	sigsetmask (mask);
#endif SVR4

	return (status);
}






/*
 * this corresponds to "incorporate" in the old mail structure
 * of things.  basically, we try and find all the "new" messages
 * that might have come in since the last time we looked at the
 * folder.
 */
static int
fm_reread(
	struct folder_obj *folder
)
{
	int status;

	if ((folder == NULL) || (folder->fo_file_name == NULL))
		return (0);

	if (lock(folder)) return(1);

	status = fm_incorporate_msg(folder);

	/* It is really no new mail or there is a new mail, we need to update
	 * the access time.
	 */
	if (status >= 0)
		(void) update_atime (folder);

	unlock(folder);

	/* Treat any error as no new msg coming in.  Backward compatible */
	if (status < 0)
		status = 0;

	return (status);
}





static char *
map_buffer(
	struct folder_obj *folder,
	long offset,
	int size
)
{
	struct buffer_map *bm;
#ifdef SVR4
	int pagesize = (int) sysconf(_SC_PAGESIZE);
#else
	int pagesize = getpagesize();
#endif "SVR4"
	int pageoff = (offset & (pagesize-1));

	size += pageoff;
	if (size == 0) {
		return ((char *) EOF);
	}

	bm = ck_malloc(sizeof (struct buffer_map));

	bm->bm_size = size;
	bm->bm_buffer = mmap(0, size,
		PROT_READ, MAP_PRIVATE, folder->fo_fd, offset - pageoff);

	if (bm->bm_buffer == (void *) -1) {
		int fd;
		int numbytes;
		char *buffer;

		/* fix for 1089208: if there is a lock on an nfs mounted file,
		 * mmap will fail.  In this case, we will mmap from /dev/zero,
		 * and then read() the data into the buffer.  From then on,
		 * things should look the same to mailtool
		 */
		fd = open("/dev/zero", O_RDWR);
		if (fd < 0) {
err_exit:
			ck_free(bm);
			return(NULL);
		}

		bm->bm_buffer = mmap(0, size , PROT_READ|PROT_WRITE,
			MAP_SHARED, fd, offset - pageoff);
		
		close(fd);

		if (bm->bm_buffer == (void *) -1) goto err_exit;

		/* see to the right place in the source file */
		lseek(folder->fo_fd, offset - pageoff, SEEK_SET);

		/* initialize the loop variables and read in the data */
		numbytes = size;
		buffer = bm->bm_buffer;
		while (numbytes) {
			int i;

			/* read in the data */
			i = read(folder->fo_fd, buffer, numbytes);

			/* we had a read error.  Clean up */
			if (i <= 0) {
				munmap(bm->bm_buffer, bm->bm_size);
				goto err_exit;
			}

			/* update the pointers */
			numbytes -= i;
			buffer += i;
		}

		/* set the buffer to read-only to catch possible bugs */
		mprotect(bm->bm_buffer, bm->bm_size, PROT_READ);
	}

	bm->bm_next = folder->fo_buffer;
	folder->fo_buffer = bm;

	return(bm->bm_buffer + pageoff);
}





static
update_atime(
	struct folder_obj *folder
)
{
	struct stat statbuf;

	if (stat(folder->fo_file_name, &statbuf) != 0)
		return (-1);

	if (statbuf.st_atime <= statbuf.st_mtime)
	{
		time_t time_p[2];
		long time();

		time_p[0] = time((long *) 0) + 1;
		time_p[1] = statbuf.st_mtime;

		if (time_p[0] <= time_p[1]) {
			/* ususally due to NFS time warps */
			time_p[0] = time_p[1] + 1;
		}

		DP(("update_atime: atime=%d, mtime=%d\n",time_p[0],time_p[1]));

		utime (folder->fo_file_name, time_p);
	}
	return (0);
}





/*
 * open a folder from memory.  No locking or incorporating new message
 * will be available.  ZZZ: fm_write() will be rewritten to deal with
 * this special folder.
 */
static struct folder_obj *
fm_init(
	char *buf,
	char *bufend
)
{
	struct folder_obj *folder;
	struct buffer_map *bm;

	if( buf == NULL )
		return( NULL );

	folder = ck_malloc( sizeof( struct folder_obj ) );
	memset( (char*) folder, 0, sizeof( struct folder_obj ) );

	folder->fo_read_only = 1;
	folder->fo_fd = -1;
	folder->fo_changed = 0;

	folder->fo_size = bufend - buf;
	folder->fo_mod_time = time ((time_t *) NULL);

	bm = bm_alloc(buf, folder->fo_size);
	bm->bm_next = folder->fo_buffer;
	folder->fo_buffer = bm;

	/* look for first msg which starts with "From " ... */
	if( ( buf = fm_find_msg (buf, bufend) ) == NULL)
	{
		fm_free( folder );
		return( NULL );
	}

	fm_add_messages(folder, buf, bufend);

	DP(("fm_init (%x, %x) = %x\n", buf, bufend, folder));

	/* return the folder to the user */
	return( folder );
}






static int
fm_compress(
	struct folder_obj *folder
)
{
#ifdef	SVR4
	pid_t	pid;
#else
	int	pid;
#endif	SVR4
	int	status;

	if ((folder->fo_file_name == NULL) || (folder->fo_compressed == 0))
		return(0);
	
	if ((pid = vfork()) == 0)
	{
		PRIV(folder, execlp("compress", "compress", "-f",
			folder->fo_file_name, 0));
		perror("compress");
		exit(1);
	}

	while (waitpid(pid, &status, 0) != pid)
		;
		
	/*
	if (WIFEXITED(status) != 0)
		return (0);
	*/

	folder->fo_compressed = 0;
	return(1);
}





static int
fm_uncompress(
	char *path,
	char *tmppath
)
{
#ifndef	COMPRESS_FOLDER
	/* No-op */
	return (0);
#else
	int	len;
	int	status;
	struct stat statbuf;
#ifdef	SVR4
	pid_t	pid;
#else
	int	pid;
#endif	SVR4

	/* ZZZ: we should look up the classing engine to determine if the
	 * folder is compressed.  But it makes maillib depends on CE.
	 */
	len = strlen(path);
	if ((len <= 2) || (path[len-2] != '.') || (path[len-1] != 'Z') ||
	    stat(path, &statbuf) || ((statbuf.st_mode & S_IFMT) != S_IFREG))
		return (0);

	if ((pid = vfork()) == 0)
	{
		PRIV(folder, execlp("uncompress", "uncompress", "-f", path, 0));
		perror("uncompress");
		exit (1);
	}

	while (waitpid(pid, &status, 0) != pid)
		;
		
	/*
	if (WIFEXITED(status) != 0)
		return (0);
	*/

	strcpy (tmppath, path);
	tmppath[len-2] = '\0';
	return(1);
#endif	COMPRESS_FOLDER
}





/*
 * open a folder given a pathname.  Return the pointer to
 * the initialized folder, or NULL if something went wrong.
 * The "callback" will be called if locking the folder fails.
 * If no "callback" is specified, the folder will not be locked.
 */
static struct folder_obj *
fm_read(
	char *path,
	int (*callback)(),
	void *arg,
	gid_t effective_gid
)
{
	int fd;
	int ecode;
	bool compressed;
	char tmppath[MAXPATHLEN];
	struct folder_obj *folder;
	struct stat statbuf;
	char *buf;
	char *bufend;
	int relock;		/* fd and file are obsoleted, need relock? */

	if (!strcmp(path, "%")) {
		path = mt_value("MAIL");
	}

	/* uncompress the .Z folder first */
	compressed = fm_uncompress(path, tmppath);
	if (compressed)
		path = tmppath;

	/* allocate a folder structure */
	folder = ck_malloc( sizeof( struct folder_obj ) );
	memset( (char*) folder, 0, sizeof( struct folder_obj ) );
	folder->fo_file_name = ck_strdup( path );
	folder->fo_read_only = 0;
	folder->fo_changed = 0;
	folder->fo_compressed = compressed;
	folder->fo_egid = effective_gid;
	folder->fo_rgid = getgid();
	folder->fo_ttlocked = (mt_value("ttlock") != NULL);

	/* use lockf() to lock the folder, it must be opened with O_RDWR.
	 * Note, this fd will be obsoleted if the unlock notification is done
	 * because fm_write() creates a new file instead overwrites an existing.
	 */
	if (!folder->fo_ttlocked) { /* if not using tooltalk locking */
		DP(("fm_read(): real gid %d, eff gid %d\n",
			folder->fo_rgid, folder->fo_egid));
Lock_Again:
		relock = 0;
		fd = open(path, (callback == NULL) ? O_RDONLY : O_RDWR);
		if (fd == -1) {
			if ((errno != EACCES) || (callback == NULL))
			{
				fm_free( folder );
				return( NULL );
			}
			/* try to open the file with read-only and no locking */
			fd = open( path, O_RDONLY);
			if (fd == -1)
			{
				fm_free( folder );
				return( NULL );
			}
			callback = NULL;
		}
		folder->fo_fd = fd;
	} /* if not using tooltalk locking */

	/* wants to lock the folder exclusively? */
	if (callback != NULL)
	{
		/* Lock failed because another process may have exclusive
		 * access to it or internal error.  Do callback to handle it.
		 */
		if (folder->fo_ttlocked) {
			while ((ecode = ttlock_f(folder, FM_LOCK, NULL)) != 0)
			{
				ecode = (*callback) (folder, ecode, arg);
				if (ecode == FM_NOLOCK) {
					break;
				}
				else if (ecode == FM_AGAIN)
				{
					continue;
				}
				else
				{
					fm_free(folder);
					return( NULL );
				}
			}
		} else {
			for (;;) {
				PRIV(folder, ecode = lock_f(folder, FM_LOCK,
					NULL));
				if (ecode == 0) break;

				ecode = (*callback) (folder, ecode, arg);
				if (ecode == FM_NOLOCK) {
					break;
				}
				else if (ecode == FM_AGAIN)
				{
					relock = 1;
					continue;
				}
				else
				{
					fm_free(folder);
					return (NULL);
				}
			}
		}
	}

#ifdef	TOOLTALK_LOCK
	if (folder->fo_ttlocked) { /* if using tooltalk locking */
		PRIV(folder, fd = open( path, O_RDONLY ));
		if( fd == -1 ) {
			fm_free( folder );
			return( NULL );
		}
		folder->fo_fd = fd;
	} else { /* if not using tooltalk locking */
#endif TOOLTALK_LOCK
		if (relock && (ecode != FM_NOLOCK))
		{
			/* Unlock notification was requested; the folder had 
			 * been changed.  The fd points to the old file and 
			 * lockf() locks the old file, so it must be re-locked
			 * and re-opened.
			 */
			PRIV(folder, lock_f(folder, FM_ULOCK, NULL));
			close(fd);
			goto Lock_Again;
		}
	}

	/* lock the folder */
	if (lock(folder)) {
		fm_free(folder);
		return (NULL);
	}

	/* figure out the size of the folder */
	if( fstat( fd, &statbuf ) == -1 ) {
		/* fstat failed? */
		fm_free( folder );
		return( NULL );
	}
	folder->fo_size = statbuf.st_size;
	folder->fo_mod_time = statbuf.st_mtime;

	/* mmap the folder into memory */
	buf = map_buffer(folder, 0, folder->fo_size);

	if( buf == NULL )
	{
		fm_free( folder );
		return( NULL );
	}

	if ( buf == (char *) EOF )
		bufend = buf = NULL;
	else {
		bufend = buf + folder->fo_size;

		/* look for first msg which starts with "From " ... */
		if( ( buf = fm_find_msg (buf, bufend) ) == NULL)
		{
			fm_free( folder );
			return( NULL );
		}

		fm_add_messages(folder, buf, bufend);
	}

	/* since we mmap the file, the access time may not be updated */
	(void) update_atime (folder);

	unlock(folder);

	DP(("fm_read (%s) = %x\n", path, folder));

	/* return the folder to the user */
	return( folder );

	/*NOTREACHED*/
}






static void
fm_add_messages(
	struct folder_obj *folder,
	char *buf,
	char *bufend
)
{
	int msg_number;
	struct msg *msg;


	if (folder == NULL)
		return;

	/* we can't use last msg number to determine the next available msg
	 * number because of sorting.
	 */
	if (folder->fo_last_msg) {
		msg_number = folder->fo_num_msgs;
	} else {
		msg_number = 0;
	}

	/* scan the folder looking for messages */
	while( buf < bufend ) {
		/* start of a message ... */

		/* allocate a new message structure */
		msg = (struct msg *) ck_malloc( sizeof( struct msg ) );
		memset( (char *) msg, 0, sizeof( struct msg ) );

		/* initialized the message sturcture */
		msg->mo_msg_number = ++msg_number;
		buf = msg_methods.mm_init (msg, buf, bufend);

		/* link the msg into the folder */
		fm_append_msg( folder, msg );

		/* ZZZ: rewrite the cache file */
	}

	/* save the last msg number for the incoming messages */
	folder->fo_num_msgs = msg_number;
}






/*
 * call each msg in a folder in turn; if any returns non-zero then
 * return that value;
 */
static unsigned long
fm_enumerate(
	struct folder_obj *folder,
	FmEnumerateFunc call,
	...
)
{
	struct msg *msg, *next;
	unsigned long retval = 0;
	va_list ap;

	if (folder == NULL) {
		return (0);
	}

	for( msg = folder->fo_first_msg; ! retval && msg; msg = next ) {
		next = msg->mo_next;

		va_start(ap, call);
		retval = (*call)(msg, ap);
		va_end(ap);
	}

	return( retval );
}






/*
 * Register a path for new mail checking.  ZZZ: sometime later we may want
 * to have a deregistration function.
 */
static struct fm_stat *
find_fmstat(
	char *path,
	bool to_add
)
{
	static struct fm_stat *fm_head;
	register struct fm_stat *p_curr = fm_head;

	while (p_curr != NULL)
	{
		if (strcmp(p_curr->fo_file_name, path) == 0)
			return (p_curr);
		p_curr = p_curr->fo_next;
	}
 
	if (to_add)
	{
		p_curr = (struct fm_stat *) ck_malloc (sizeof(struct fm_stat));
		memset ((char *) p_curr, 0, sizeof(struct fm_stat));
		p_curr->fo_file_name = ck_strdup (path);
		p_curr->fo_next = fm_head;
		fm_head = p_curr;
	}
	return (p_curr);
}







/*
 * Check new arrival mail by registering the mail file.  Returns:
 *	FM_NO_NEW_MAIL if there is no new mail in the folder.
 *	FM_NEW_MAIL if there is a new mail in the folder.
 *	FM_NEW_ARRIVAL if there is a newly arrival mail in the folder.
 *	FM_NO_MAIL if the folder is empty or doesn't exist.
 */
static int
fm_newmail(
	char *path
)
{
	register off_t	fsize;
	register time_t	ftime;
	struct fm_stat	*folder;
	struct stat	statbuf;

	/*
	 * The folder no longer exists, then we don't have any mail.
	 */
	if (stat (path, &statbuf) < 0)
		return (FM_NO_MAIL);

	/* find the folder status.  if not exist, add one. */
	folder = find_fmstat(path, TRUE);
	if (folder == NULL)
		return (FM_NO_MAIL);
	
	fsize = folder->fo_size;
	ftime = folder->fo_mod_time;

	/* Update the folder size and last modified time */
	folder->fo_size = statbuf.st_size;
	folder->fo_mod_time = statbuf.st_mtime;

	if (((statbuf.st_atime <= statbuf.st_mtime) && (statbuf.st_size != 0))
	    || ((fsize != 0) && (fsize < statbuf.st_size)))
	{
		/*
		 * If the file modified time is later than the last checking
		 * time, we know there is a new arrival of mail.  Or, there is
		 * a change in the folder size (fsize==0 is the initial state).
		 */
		if ((ftime < statbuf.st_mtime) || ((fsize != 0) &&
						   (fsize < statbuf.st_size)))
			return (FM_NEW_ARRIVAL);
		else
			return (FM_NEW_MAIL);
	}

	if (statbuf.st_size == 0)
		return (FM_NO_MAIL);
	else
		return (FM_NO_NEW_MAIL);
}





static int
fm_corrupt(
	struct folder_obj *folder
)
{
	struct msg *msg;
	struct stat sb;
	struct stat stat_buf;

	if (folder == NULL)
		return (0);

	if (folder->fo_garbage)
		return (1);

	folder->fo_garbage = 1;

	if (fstat(folder->fo_fd, &stat_buf) != 0)
		return (1);

	/* 
	 * When another mailtool saves a mail file, it removes the old file
	 * and creates the new file.  If the file is NFS mounted (NFS is
	 * stateless), the mmapped memory becomes invalid and causes SIGBUS
	 * when it is being accessed.  ZZZ: Here we make sure that it is still
	 * the same file.  There is no need to apply this to the mail file in
	 * local system, but we still treat it as corrupted.  Bugid #1082580.
	 */
	if ((stat(folder->fo_file_name, &sb) != 0) ||
	    (stat_buf.st_dev != sb.st_dev) || (stat_buf.st_ino != sb.st_ino))
		return (1);

	/* If the folder is smaller than the mmapped file, it is corrupted. */
	if (stat_buf.st_size < folder->fo_size)
		return (1);

	/* If the header of the last mmapped msg is not started with "From ",
	 * the folder is corrupted (we assume that one of the previous msgs
	 * grows).  Note, if the msg is modified, its buffer is not mmapped
	 * from folder anymore (see mm_replace).
	 */
	if (!STRNEQ( folder->fo_last_from, Msg_Header, MSG_HEADER_SIZE ))
		return (1);

	folder->fo_garbage = 0;
	return (0);
}






static int
fm_modified(
	struct folder_obj *folder
)
{
	struct msg *m;

	if (folder == NULL)
		return (0);

	if (folder->fo_changed)
		return (1);

	for (m = folder->fo_first_msg; m != NULL; m = m->mo_next)
	{
		if (msg_methods.mm_modified (m))
		{
			folder->fo_changed = 1;
			return (1);
		}
	}
	return (0);
}






/*
 * FM_LOCK returns
 *	 0 if locking is success
 *	-1 if unable to lock due to system limitation or internal error
 *	>0 if locking fails because other process has it
 */
static
fm_lock(
	struct folder_obj *folder,
	int cmd,
	void *arg
)
{
	int status;

	DP(("fm_lock (fd=%d,file=%s,cmd=%s)\n", folder->fo_fd,
		folder->fo_file_name, (cmd == FM_LOCK) ? "FM_LOCK" :
		((cmd == FM_ULOCK) ? "FM_ULOCK" : "FM_NOTIFY")));

	if (folder->fo_file_name == NULL)
		return (0);
		
	if (folder->fo_ttlocked) {
		return (ttlock_f(folder, cmd, (int) arg));
	} else {
		PRIV(folder, status = lock_f(folder, cmd, (int) arg));
		return (status);
	}
}



/**************************************************************************
 *
 * LOCAL CALLS
 *
 *************************************************************************/

static char *
fm_find_msg(
	char *buf,
	char *bufend
)
{
	/* find the beginning of a message which starts with "From " */
	while ((bufend - buf) >= MSG_HEADER_SIZE)
	{
		if( STRNEQ( buf, Msg_Header, MSG_HEADER_SIZE ) )
			return (buf);

		/* check for end of buffer; start after \n */
		buf = findchar( '\n', buf, bufend ) + 1;
	}
	return (NULL);
}





static int
fm_incorporate_msg(
	struct folder_obj *folder
)
{
	struct fm_stat *fmstat;
	struct stat statbuf;
	char *begin;
	char *buf, *bufend;
	int status;


	status = fstat(folder->fo_fd, &statbuf);

	if (status == 0) {
		/* update the mod time */
		folder->fo_mod_time = statbuf.st_mtime;
	}

	if (status < 0 || statbuf.st_size == 0 ||
	    statbuf.st_size == folder->fo_size ||
	    (statbuf.st_mode & S_IFMT) != S_IFREG)
	{
		/* we really don't have new mail... */
		return(0);
	}

	/* ZZZ:katin: extend the locks on our new region */

	/* map in the new portion of the file */
	buf = map_buffer(folder, folder->fo_size,
		statbuf.st_size - folder->fo_size);

	if (buf == NULL) {
		return(-1);
	}

	if (buf == (char *) EOF)
		buf = NULL;

	bufend = buf + (statbuf.st_size - folder->fo_size);
	if ((folder->fo_size = statbuf.st_size) != 0)
	{
		/* look for "From " at the start...  if not, search
		 * forward for the beginning of a message
		 */
		begin = fm_find_msg (buf, bufend);
		if ((begin != buf) || (begin == NULL))
		{
			folder->fo_garbage = 1;
			if (begin == NULL) {
				buf = bufend;
			} else {
				buf = begin;
			}
		}
	}

	/* actually read in the messages */
	if (buf < bufend) {
		fm_add_messages(folder, buf, bufend);
		/* indicate that the folder is needed to be written out */
		folder->fo_changed = 1;
	}

	/* update the folder status for new mail */
	fmstat = find_fmstat(folder->fo_file_name, FALSE);
	if (fmstat != NULL)
	{
		fmstat->fo_size = statbuf.st_size;
		fmstat->fo_mod_time = statbuf.st_mtime;
	}

	return(1);
}





static int
fm_delete_msg(
	struct msg *msg
)
{
	struct folder_obj *folder = msg->mo_folder;

	/* detach the message from the list */
	if( msg->mo_next ) {
		msg->mo_next->mo_prev = msg->mo_prev;
	} else {
		/* this is the last message */
		folder->fo_last_msg = msg->mo_prev;
	}

	if( msg->mo_prev ) {
		msg->mo_prev->mo_next = msg->mo_next;
	} else {
		/* this is the first message */
		folder->fo_first_msg = msg->mo_next;
	}

	msg_methods.mm_free_msg(msg);

	return( 0 );
}


unsigned long
fm_delete_msg_enumerate(
	struct msg *m,
	va_list ap
)
{
	return (fm_delete_msg(m));
}





/*
 * append a message to the end of the folder
 */
static void
fm_append_msg(
	struct folder_obj *folder,
	struct msg *msg
)
{

	msg->mo_folder = folder;

	if( folder->fo_last_msg ) {
		ASSERT( ! folder->fo_last_msg->mo_next );

		folder->fo_last_msg->mo_next = msg;
		msg->mo_next = NULL;

		msg->mo_prev = folder->fo_last_msg;
		folder->fo_last_msg = msg;
	} else {
		/* empty folder */
		ASSERT( ! folder->fo_first_msg );

		folder->fo_first_msg = folder->fo_last_msg = msg;
		msg->mo_next = msg->mo_prev = NULL;
	}

	/* all messages must start with "From ", save the last offset
	 * to detect folder corruption.
	 */
	folder->fo_last_from = msg->mo_hdr->hdr_start;
}



/*
 * get a lock on a mail folder.
 *
 * ZZZ:katin: we should put up some messages while sleeping to keep
 * the user from wondering...
 */
static int
lock(
	struct folder_obj *folder
)
{
	char curlock[MAXPATHLEN];
	char tmplock[MAXPATHLEN];
	time_t now;
	int fd;
	struct stat sbuf;
	struct stat tmpbuf;
	int statfailed;

	/* don't lock twice or don't lock when file name isn't specified */
	if (folder->fo_locked || (folder->fo_file_name == NULL))
		return (0);

	strcpy(curlock, folder->fo_file_name);
	strcat(curlock, ".lock");
	strcpy(tmplock, folder->fo_file_name);
	strcat(tmplock, ".XXXXXX");
	mktemp(tmplock);

	statfailed = 0;

	DP(("locking %s\n", folder->fo_file_name));

	for (;;) {
		/* try and create the tmp file */
		PRIV(folder, fd = open(curlock, O_RDWR | O_CREAT | O_EXCL, 0));

		if (fd >= 0) {
			close(fd);
			folder->fo_locked = 1;
			return (0);
		}

		DP(("lock: already locked (errno %d)\n", errno));

		/* there already is a lock file there.  Try and
		 * figure out if it is over 5 minutes old.  First try
		 * and figure out the current time on the file server
		 */

		while (stat(curlock, &sbuf) >= 0) {
			statfailed = 0;

			PRIV(folder, unlink(tmplock));
			PRIV(folder, fd = creat(tmplock, 0));
			if (fd < 0 || fstat(fd, &tmpbuf) < 0) {
				now = time((time_t *) 0);
			} else {
				now = tmpbuf.st_ctime;
			}
			close(fd);
			PRIV(folder, unlink(tmplock));

			DP(("lock: %d secs old\n", now - sbuf.st_ctime));

			/* see if it is over 5 minutes old */
			if (sbuf.st_ctime + 300 < now) {
				int unlinkstatus;
				DP(("lock: too old; forcing removal\n"));
				PRIV(folder, unlinkstatus = unlink(curlock));

				if (unlinkstatus == -1) {
					int confirm;

nolock:
	/* clear the error message build into maillib */
	maillib_methods.ml_set(ML_CLRERR);

	/* STRING_EXTRACTION SUNW_DESKSET_MAILLIB
	 *
	 * This string is printed when we cannot remove the lock
	 * file of a folder.  At this point we can either fail
	 * the open, and not do the operation, or ignore locking
	 * and take our chances.  We ask the user what to do...
	 *
	 */
	confirm = maillib_methods.ml_confirm( NULL, 0,
	dgettext("SUNW_DESKSET_MAILLIB", "Abort locking"),
	dgettext("SUNW_DESKSET_MAILLIB", "Don't bother locking"),
	dgettext("SUNW_DESKSET_MAILLIB",
"Cannot get the lock file for folder\n\
%s.\n\
At this point you can either abort the operation,\n\
or attempt to perform the operation without a lock.\n\
\n\
WARNING: attempting to proceed without the lock may\n\
result in damage to the mail folder.\n"),
	folder->fo_file_name, strerror(errno));

	if (confirm) {
		/* they aborted */
		return (1);
	} else {
		/* continue without the lock */
		return (0);
	}
				}
				break;
			}

			sleep(5);
		}

		if (statfailed++ > 5) {
			goto nolock;
		}
	}
}


static void
unlock(
	struct folder_obj *folder
)
{
	char curlock[MAXPATHLEN];
	
	if (folder->fo_locked) {
		DP(("unlocking %s\n", folder->fo_file_name));
		strcpy(curlock, folder->fo_file_name);
		strcat(curlock, ".lock");

		PRIV(folder, unlink(curlock));
		folder->fo_locked = 0;
	}
}
