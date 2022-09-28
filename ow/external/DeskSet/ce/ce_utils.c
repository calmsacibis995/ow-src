#ifndef lint
static char sccsid[] = "@(#)ce_utils.c	1.15 1/30/91 Copyright 1989 Sun Microsystems, Inc.";
#endif lint
/*
 * Utility functions used throughout the CE
 * Change record:
 *	1/30/91 - CRT id 58 - put back fcntl() locking
 */
#include <errno.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <rpc/types.h>
#include <rpc/xdr.h>

#include "ce_int.h"

/* define FLOCK for SVR4 or 4.1 compile environments */
#ifdef 	SVR4
#define	FLOCK	flock_t
#else
#define	FLOCK	struct flock
#endif	SVR4

/* Hash function
 * Given a string s, return an integer hash value for s
 * This routine is an exact copy of hashpjw(), a hash function listed
 * in page 436 of Aho, Sethi and Ullman's "Compilers, Principles, Techniques
 * and Tools" - Addison - Wesley (1986 Reprinted edition)
 */
#define	PRIME 211
#define	EOS '\0'
int hashpjw(s)
char *s;
{
	char *p;
	unsigned h = 0, g;
	for (p = s; *p != EOS; p = p+1)	{
		h = (h << 4) + (*p);
		if (g = h&0xf0000000)	{
			h = h ^ (g >> 24);
			h = h ^ g;
		}
	}
	return (h % PRIME);
}

/* XDR routine for NS_MGR_INFO
 */
bool_t
xdr_NS_MGR_INFO(xdrs, objp)
XDR		*xdrs;
NS_MGR_INFO	**objp;
{
	/* set the ptr to NULL */
	*objp = NULL;
	return (TRUE);
}

/* XDR routine for OLD_INFO
 */
bool_t
xdr_OLD_INFO(xdrs, objp)
XDR		*xdrs;
OLD_INFO	**objp;
{
	/* set the ptr to NULL */
	*objp = NULL;
	return (TRUE);
}


/* returns a database mtime */
time_t
get_db_mtime (dd)
int *dd;		/* database (file) descriptor */
{
	struct stat stat_info;
	
	if (fstat (*dd, &stat_info) == 0)
		return (stat_info.st_mtime);
	else
		return ((time_t) -1);
}


char *
get_next_path(s1, s2, si)
register char *s1, *s2;
char	*si;
{
	register char	*s;
	char	*end;

	s = si;
	end = s + MAXPATHLEN;
	while(*s1 && *s1 != ':' && s < end)
		*s++ = *s1++;
	if(si != s && s < end)
		*s++ = '/';
	while(*s2 && s < end)
		*s++ = *s2++;
	*s = '\0';
	return(*s1? ++s1: 0);
}	

/* return the default database path for a db type
 */
char
*get_default_db_path (db_type)
int	db_type;
{
	static char	*user_db_path = "~/.cetables/cetables";
	static char	*system_db_path = "/etc/cetables/cetables";
	static char     *network_db_path;

	if(getenv("DESKSETHOME"))
	   network_db_path = "$DESKSETHOME/lib/cetables/cetables";
	else
           network_db_path = "$OPENWINHOME/lib/cetables/cetables";


	switch (db_type)	{
	case USER_DB:
		return (user_db_path);
	case SYSTEM_DB:
		return (system_db_path);
	case NETWORK_DB:
		return (network_db_path);
	}
}

/* XXX kludge - a version of perform_db_op() used by ce_test_ok_to_write
 * because perform_db_op() uses statics that are tied to the currently
 * open CE database
 */

/* opens, locks, reads, writes and unlocks a CE database */
int
XXX_perform_db_op (db, db_type, op, dd)
CE_DB	*db;
int	db_type;
int	op;
int	*dd;		/* really just a file descriptor - a database descriptor */

{
	static char	*user_db_path = "~/.cetables/cetables";
	static char	*system_db_path = "/etc/cetables/cetables";
	static char 	*network_db_path;
	static char	*path;
	static char	db_path[MAXPATHLEN];
	char		*mkdir_path;
	char		*sp, *tp, *lp;
	int		lock_sts, i;
	static int	path_set = 0;
	static int 	lock_fd;
	int		read_sts;
	FLOCK	 	fl;

	if(getenv("DESKSETHOME"))
	   network_db_path = "$DESKSETHOME/lib/cetables/cetables";
	else
           network_db_path = "$OPENWINHOME/lib/cetables/cetables";
	
	
	switch (op)	{
	case SET_DB_FILENAME:

		path = (char *)dd;
		path_set = 1;
		return (0);

	case OPEN_DB_FOR_WRITE:
		if (path_set)	{
			ce_expand_pathname (path, db_path, sizeof db_path);
			path = NULL;
			path_set = 0;
		}
		else	{
			
			switch (db_type)	{
			case USER_DB: 
				ce_expand_pathname (user_db_path, db_path, sizeof db_path);
				break;
			case SYSTEM_DB:
				(void)strcpy (db_path, system_db_path);
				break;
			case NETWORK_DB:
				ce_expand_pathname (network_db_path, db_path, sizeof db_path);
				break;
			}
		}

		while (1)	{
			
			*dd = open (db_path, O_WRONLY|O_CREAT, 0666);
			if (*dd == -1)	{
				*dd = errno;
				if (errno == ENOENT)	{
					mkdir_path = (char *) malloc (strlen(db_path)
								      + 1);
					sp = (char *)strrchr (db_path, '/');
					lp = db_path;
					tp = mkdir_path;
					for (i = 0; lp != sp; i++)	{
						*tp++ = *lp++;
					}
					*tp = '\0';
					if ((mkdir (mkdir_path, S_IRWXU|
						    S_IRGRP|S_IXGRP|
						    S_IROTH|S_IXOTH)) == -1)	{
						*dd = errno;
						return (CE_ERR_OPENING_DB);
					}
				}						
				else	
					return (CE_ERR_OPENING_DB);
			}
			else
				break;
		}
		
		
		return (0);

	case TEST_OK_TO_WRITE:

#ifdef 	USE_LOCKING		
		memset (&fl, 0, sizeof (FLOCK));		
		fl.l_type = F_WRLCK;
		fl.l_whence = SEEK_SET;
		fl.l_start = 0;
		fl.l_len = 0;
		if ((lock_sts = fcntl (*dd, F_SETLK, &fl)) == 0)
			return (0);
		else	{
				lock_sts = errno;
				switch (lock_sts)	{
				case EACCES:
				case EAGAIN:
				default:
					return (CE_ERR_DB_LOCKED);
				}
			}
#else	USE_LOCKING
		return (0);
#endif	USE_LOCKING		
		
	case UNLOCK_DB:
	
#ifdef	USE_LOCKING
		fl.l_type = F_UNLCK;
		fl.l_whence = SEEK_SET;
		fl.l_start = 0;
		fl.l_len = 0;
		(void) fcntl (*dd, F_SETLK, &fl);
		return (0);
#else	USE_LOCKING
		return (0);
#endif	USE_LOCKING		
		
	case CLOSE_DB:
		(void)close (*dd);
		return (0);
		
	}
}

/* opens, locks, reads, writes and unlocks a CE database */
int
perform_db_op (db, db_type, op, dd)
CE_DB	*db;
int	db_type;
int	op;
int	*dd;		/* really just a file descriptor - a database descriptor */

{
	static char	*user_db_path = "~/.cetables/cetables";
	static char	*system_db_path = "/etc/cetables/cetables";
	static char 	*network_db_path;
	static char	*lock_suffix = ".lck";
	static char	*path;
	static char	db_path[MAXPATHLEN];
	char		*mkdir_path;
	char		*sp, *tp, *lp;
	int		lock_sts, i;
	static int	path_set = 0;
	static int 	lock_fd;
	int		read_sts, write_sts;
	FLOCK		fl;

	/* To accomodate DESKSETHOME; further kludge */
	if(getenv("DESKSETHOME"))
		network_db_path = "$DESKSETHOME/lib/cetables/cetables";
	else
		network_db_path = "$OPENWINHOME/lib/cetables/cetables";	
	
	switch (op)	{
	case SET_DB_FILENAME:

		path = (char *)dd;
		path_set = 1;
		return (0);

	case OPEN_DB_FOR_READ:
		if (path_set)	{
			ce_expand_pathname (path, db_path, sizeof db_path);
			path = NULL;
			path_set = 0;
		}
		else	{
			
			switch (db_type)	{
			case USER_DB: 
				ce_expand_pathname (user_db_path, db_path, sizeof db_path);
				break;
			case SYSTEM_DB:
				(void)strcpy (db_path, system_db_path);
				break;
			case NETWORK_DB:
				ce_expand_pathname (network_db_path, db_path, sizeof db_path);
				break;
			}
		}
	
		*dd = open (db_path, O_RDONLY);
		if (*dd == -1)
			return (CE_ERR_OPENING_DB);
		return (0);
		
	case OPEN_DB_FOR_WRITE:
		if (path_set)	{
			ce_expand_pathname (path, db_path, sizeof db_path);
			path = NULL;
			path_set = 0;
		}
		else	{
			
			switch (db_type)	{
			case USER_DB: 
				ce_expand_pathname (user_db_path, db_path, sizeof db_path);
				break;
			case SYSTEM_DB:
				(void)strcpy (db_path, system_db_path);
				break;
			case NETWORK_DB:
				ce_expand_pathname (network_db_path, db_path, sizeof db_path);
				break;
			}
		}

		while (1)	{
			
			*dd = open (db_path, O_WRONLY|O_CREAT, 0666);
			if (*dd == -1)	{
				*dd = errno;
				if (errno == ENOENT)	{
					mkdir_path = (char *) malloc (strlen(db_path)
								      + 1);
					sp = (char *)strrchr (db_path, '/');
					lp = db_path;
					tp = mkdir_path;
					for (i = 0; lp != sp; i++)	{
						*tp++ = *lp++;
					}
					*tp = '\0';
					if ((mkdir (mkdir_path, S_IRWXU|
						    S_IRGRP|S_IXGRP|
						    S_IROTH|S_IXOTH)) == -1)	{
						*dd = errno;
						return (CE_ERR_OPENING_DB);
					}
				}						
				else	
					return (CE_ERR_OPENING_DB);
			}
			else
				break;
		}		

		
		return (0);

		
	case TEST_OK_TO_READ:
		return (0);
		
	case TEST_OK_TO_WRITE:

#ifdef	USE_LOCKING	
		memset (&fl, 0, sizeof (FLOCK));	
		fl.l_type = F_WRLCK;
		fl.l_whence = 0;
		fl.l_start = 0;
		fl.l_len = 0;
		if ((lock_sts = fcntl (*dd, F_SETLK, &fl)) != -1)
			return (0);
		else	{
				lock_sts = errno;
				switch (lock_sts)	{
				case EACCES:
				case EAGAIN:
				default:
					return (CE_ERR_DB_LOCKED);
				}
			}
#else	USE_LOCKING
		return (0);
#endif	USE_LOCKING
		
	case UNLOCK_DB:
		
#ifdef	USE_LOCKING
		fl.l_type = F_UNLCK;
		fl.l_whence = SEEK_SET;
		fl.l_start = 0;
		fl.l_len = 0;
		(void) fcntl (*dd, F_SETLK, &fl);
		return (0);
#else	USE_LOCKING
		return (0);
#endif	USE_LOCKING		

		
	case CLOSE_DB:
		(void)close (*dd);
		return (0);
		
	case READ_DB:
		read_sts = read_ce_db(db, *dd);
		if (read_sts == 0)
			return(0);
		else {
			if (read_sts == CE_ERR_BAD_DATABASE_FILE)	{
				/* just clobber the bad database file & lock file */
				(void)close (*dd);
				(void)unlink (db_path);
			}
			return(read_sts);
		}			
			
	case WRITE_DB:
		if ((write_sts = write_ce_db(db, *dd)) != 0)	{
			fl.l_type = F_UNLCK;
			fl.l_whence = SEEK_SET;
			fl.l_start = 0;
			fl.l_len = 0;
			(void) fcntl (*dd, F_SETLK, &fl);
			(void)close (*dd);
			return (write_sts);
		}
		else
			return (0);
		
	}
}



/* gets a cookie for an entry 
 * if it's a namespace entry, return the namespace entry cookie "NS_ENTRY"
 * if it's not, call the namespace manager's get_entry_cookie routine
 */
int
get_cookie (namespace, entry, cookie_ptr)
CE_NAMESPACE namespace;
CE_ENTRY entry;
char **cookie_ptr;
{
	if (is_ns_entry (entry->flags))	{
		*cookie_ptr = ns_entry_cookie;
		return (1);
	}
	
	return ((*namespace->ns_mgr_ptr->get_entry_cookie) (namespace, entry, cookie_ptr));
}

/* matches a cookie for an entry
 * if it is a namespace entry, sees if cookie is "NS_ENTRY"
 * if it's not, calls the namespace manager's match_entry_cookie routine
 */
int
match_cookie (namespace, entry, cookie)
CE_NAMESPACE namespace;
CE_ENTRY entry;
char *cookie;
{
	if (is_ns_entry (entry->flags))	{
		return (strcmp (cookie, ns_entry_cookie));
	}
	return ((*namespace->ns_mgr_ptr->match_entry_cookie)(namespace, cookie, entry));
}

/******************************************************************************
*
*       Function:       ce_expand_pathname (stolen from libdeskset)
*
*       Description:    Expand ~'s and environment variables in a path.
*		
*			This routine was stolen from filemgr. I've made a
*			couple of small changes to better handle "~user"
*			at the start of a path.
*
*       Parameters:     path	Unexpanded path
*                       bug    	Returned expanded path.  Caller is responsible
*				allocating enough space.
*
*       Returns:        Nothing
*
******************************************************************************/
ce_expand_pathname(path, buf, buflen)
	char *path;			/* User's unexpanded path */
	register char *buf;		/* Return expanded path */
	register int buflen;		/* length of returned path */
{
	register char *p, *b_p, *e_p;	/* String pointers */
	char *save_p;			/* Point in path before env var */
	char env[255];			/* Environment variable expansion */
	struct passwd *pw;		/* Password file entry */

	/* sanity check */
	if (buflen <= 0) return;

	/* ensure that buffer is null terminated */
	buf[--buflen] = '\0';

	p = path;
	if (*p == '~')
	{
		p++;
		if (*p && *p != '/')
		{
			/* Somebody else's home directory? */
			if (b_p = (char *)strchr(p, '/'))
				*b_p = '\0';
			if (pw = getpwnam(p)) {
				(void) strncpy(buf, pw->pw_dir, buflen);
			} else {
				*buf = '~';
				(void) strncpy(buf + 1, p, buflen -1);
			}

			if (b_p)  {
				*b_p = '/';
				p = b_p;
			} else
				return;

		} else {
			pw = (struct passwd*)getpwuid(getuid());
			if (pw)
				(void) strncpy(buf, pw->pw_dir, buflen);
			else 
				(void) strncpy(buf, getenv("HOME"), buflen);
		}
		buflen -= strlen(buf);
		buf += strlen(buf);
	}

	while (*p && buflen > 0) {
		if (*p == '$')
		{
			/* Expand environment variable */
			save_p = p;
			e_p = env;
			p++;

			/* hack alert: 255 limit on env vars? */
			while ((isalnum(*p) || *p == '_') && e_p - env < 255) {
				*e_p++ = *p++;
			}
			*e_p = NULL;
			if (e_p = (char *)getenv(env)) {
					strncpy(buf, e_p, buflen);
					buflen -= strlen(buf);
					buf += strlen(buf);

			} else {
				p = save_p;
				*buf++ = *p++;
				buflen--;
			}
		} else  {
			*buf++ = *p++;
			buflen--;
		}
	}
	*buf = NULL;
}

void
*entry_compare (namespace, entry, arg_entry)
CE_NAMESPACE namespace;
CE_ENTRY entry;
CE_ENTRY arg_entry;
{
	if (entry == arg_entry)
		return ((void *)&namespace->db_id);
	return (NULL);
}
