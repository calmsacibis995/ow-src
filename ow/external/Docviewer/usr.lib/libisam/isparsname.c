#ifndef lint
        static char sccsid[] = "@(#)isparsname.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isparsname.c
 *
 * Description:
 *	Functions that deal with NFS file names, mount points, symbolic
 *	links and other stuff that complicates the mapping from 
 *	ISAM file name to the host storing the ISAM file and a local
 *	path to the file on the host.
 *	We attempted to leverage on some functions available on SunOS.
 */

#include "isam_impl.h"
#include <sys/mnttab.h>
#include <unistd.h>
#include <netdb.h>

#define MTAB	"/etc/mnttab"

extern void _removelast();
extern char *_lastelement();


/*
 * _parse_isfname(isfname, hostname, localpath, remote, rdonly, nfsopen)
 *
 * Isfname is an input argument designating the ISAM file. It may contain
 * the full path, or a path relative to the current directory.
 * _parse_isfname() returns the name of host storing the ISAM file in
 * the hostname output parameter (must be declared char [MAXHOSTNAMELEN]),
 * the localpath (must be declared char [MAXPATHLEN]) on the host, and
 * a flag remote which is set to 1 if the file is remote and to 0 if
 * the file is local. The rdonly flag is set to 1 if the  file
 * system is mounted "ro".
 * The _parse_isfname() returns ISERROR if the ISAM file path cannot be
 * resolved to the host name and local path name. A successful call returns
 * ISOK.
 *
 * We use a trick: 
 *  (1) Use getwd() to get current directory and save it.
 *  (2) Change directory with the ISAM file; use getwd() to get
 *      absolute directory name (no symbolic links).
 *  (3) Change directory (use saved directory frojm step (1)).
 *  (4) The absolute pathname can be matched against entries in /etc/mtab.
 *
 * Modified:
 *    	nfsopen flags will cause use_mtab() to pretend that the file is local.
 */

int
_parse_isfname(isfname, hostname, localpath, remote, rdonly, nfsopen)
    char	*isfname;
    char	*hostname;
    char	*localpath;
    int		*remote;
    int		*rdonly;
    int		nfsopen;
{
    extern char *getcwd();
    char	currentdir[MAXPATHLEN];
    char	isamdir[MAXPATHLEN];
    int err;

    /* Handle pathname in form host:/localpath */

    if (strchr(isfname, ':')) {
      if (_isam_explicit_path(isfname, hostname, localpath, rdonly, remote)==ISOK) {
	if (*localpath != '/') {
	  return EFNAME;
	}
	return ISOK;
      }
      else
	return EFATAL;
    }

    /* Save current directory. */
    if (getcwd(currentdir, MAXPATHLEN) == NULL) {
      _isfatal_error("getcwd() failed");
      return EFATAL;
    }

    /* 
     * Check if isfname is a full pathname; if is is not a full pathname,
     * prefix it with the current directory.
     */
    if (isfname[0] != '/') 
	(void) sprintf(isamdir, "%s/%s", currentdir, isfname);
    else 
	(void) strcpy(isamdir, isfname);
	

    /* Remove the last element of the path. */
    _removelast(isamdir);

    /*
     * isamdir now contains full pathname of the directory with the ISAM file.
     * isamdir may contain symbolic links. Change current directory to this
     * directory and use getcwd() to get the absolute pathname of this
     * directory and override isamdir with this absolute pathname.
     */
    if (chdir(isamdir) == -1) 
	return (EFNAME);

    if (getcwd(isamdir, MAXPATHLEN) == NULL) { 
      _isfatal_error1("getcwd() failed");
      return (EFATAL);
    }

    if (chdir(currentdir) == -1) {
      _isfatal_error1("chdir() failed");
      return (EFATAL);
    }

    /*
     * Use file /etc/mtab and getmntent(3) to find the file system to
     * which the directory belongs.
     */
    err = _use_mtab(isamdir, hostname, localpath, remote, rdonly, nfsopen);
    if (err != ISOK)
        return (EFATAL);

    /*
     * Append the last element of the path to the localpath.
     */
    if (strcmp(localpath, "/") != 0)
	(void) strcat(localpath, "/");
    (void) strcat(localpath,_lastelement(isfname));

    return (ISOK);
}


/*
 * _use_mtab(isamdir, hostname, localpath, remote, rdonly)
 *
 * Use /etc/mtab to find file system storing the isamdir directory.
 */

Static
int
_use_mtab(isamdir, hostname, localpath, remote, rdonly, nfs)
    char		*isamdir;
    char		*hostname;
    char		*localpath;
    int			*remote;
    int			*rdonly;
    int			nfs;
{
  extern FILE	*fopen();
  FILE		*fp;
  struct mnttab	ment;
  char		mntfsname[MAXPATHLEN];
  char		mntdir[MAXPATHLEN];
  int 	 	ret;

  if ((fp = fopen(MTAB, "r")) == NULL) {
    _isfatal_error1("Cannot open /etc/mnttab");
    return (ISERROR);
  }
  /*
   * Scan the /etc/mnttab file and find the longest prefix of
   * isamdir in mnt_dir. The corresponding mnt_fsname is then the
   * file system to which the isamdir belongs.
   * Keep the current candidate for mnt_mountp and mnt_special in
   * mntdir and mntfsname respectively. *remote and *rdonly are
   * updated at each time the candidate is changed. The candidate
   * is changed whenever a longer matching mnt_mountp is found.
   */

  *mntfsname = NULL;	
  *mntdir = NULL;

  while ((ret = getmntent (fp, &ment)) == 0) {
    if (_isprefixdir(ment.mnt_mountp, isamdir) &&
	(int)strlen(ment.mnt_mountp) > (int) strlen(mntdir)) {

      /* A new candidate found. */
      (void) strcpy(mntdir, ment.mnt_mountp);
      (void) strcpy(mntfsname, ment.mnt_special);
      if (strcmp(ment.mnt_fstype, "nfs") == 0) 
	*remote = REMOTE_FILE;
      else
	*remote = (strcmp(ment.mnt_fstype, "rfs") == 0) ?
	  REMOTE_FILE : LOCAL_FILE;
      *rdonly = (strstr(ment.mnt_mntopts, "ro") != NULL);
    }
  }

  /* The file must be found under normal circumstances. */
  if (*mntdir == NULL) {
    _isfatal_error1("Cannot find file system in /etc/mnttab");
    return (ISERROR);
  }
    
  /* 
   * NFS mode is specified, pretend that this is not remote file.
   */
  if (*remote == REMOTE_FILE && nfs)
    *remote = NFS_FILE;

  if (*remote == REMOTE_FILE) {
    int	l;
    char	*p;

    if ((p = strchr(mntfsname, ':')) == NULL) {
      _isfatal_error1("No ':' in mntfsname for remote file system");
      return (ISERROR);
    }
    l = p - mntfsname;

    (void) strncpy(hostname,mntfsname, l);
    hostname[l] = NULL;

    /* 
     * Handle root mounted as a aspecial case, since otherwise we
     *   would lose '/' 
     */
    if (strcmp(mntdir, "/") == 0)
      (void)sprintf(localpath, "%s/%s", mntfsname + l + 1,
		    isamdir + 1);
    else
      (void) sprintf(localpath, "%s%s", mntfsname + l + 1,
		     isamdir + strlen(mntdir));

    /* 
     * A special case when the ISAM is located directly in
     * the root directory: remove the trailing '/'
     */
    if (localpath[strlen(localpath) -1] == '/')
      localpath[strlen(localpath) -1] = '\0';
  }
  else {
    /*
     * Set hostname to an empty string, 
     * localpath is isamdir.
     */
    *hostname = NULL;
    (void) strcpy (localpath, isamdir);
  }

  (void) fclose(fp);		/* Close file */

  return (ISOK);
}

/*
 * _isprefixdir(dir, prefix)
 *
 * Return 1 if prefix is prefix directory of dir. Otherwise return 0.
 */

int
_isprefixdir(prefix, dir)
    char	*dir;
    char	*prefix;
{
    int		prefixlen;

    prefixlen = strlen(prefix);

    return (strcmp(prefix, "/") == 0 ||	     /* Root is prefix to any dir */
	    strncmp(prefix, dir, prefixlen) == 0 && dir[prefixlen] == '/' ||
					     /* Proper prefix directory */
	    strcmp(prefix, dir) == 0);	     /* Prefix and dir are identical */
}

static _isam_explicit_path(isfname, hostname, localpath, rdonly, remote)
	char	*isfname, *hostname, *localpath;
	int	*rdonly, *remote;
{
  char		*p;
  int		hostnamelen;
  char		namebuf[MAXHOSTNAMELEN];
  int		force_remote;	/* host::/localpat will force
				   application to go through server
				   always */
					
  *rdonly = 0;			/* we don't know permissions yet */
  if (p = strchr(isfname, ':')) {
    hostnamelen = p - isfname;
    force_remote = (isfname[hostnamelen + 1] == ':');
    strncpy(hostname, isfname, hostnamelen);
    hostname[hostnamelen] = NULL;
    strcpy(localpath, p+1 + force_remote);

    if (gethostname(namebuf, sizeof(namebuf)) == -1) {
      _isfatal_error1("gethostname() failed");
      return ISERROR;
    }
		
    *remote = strcmp(hostname, namebuf) != 0 || force_remote;
    return ISOK;
  }
  else
    return ISERROR;
}

/*
 * Check permission for open
 *
 * input : isfname - the full path of ISAM file
 *         mode    - openmode ready for access()
 *
 * output: ISOK    - permision is OK
 *         errors  - depending on the cases
 */

int 
_check_open_permissions(isfname, mode, netisamd)
    char	 	*isfname;
    int			mode;
    int                 netisamd; /* 1 if called from daemon */
{
    char	datfilepath[MAXPATHLEN];
    int		fd,accessmode;
    int         ret;

    (void) strcpy(datfilepath, isfname);
    _makedat_isfname(datfilepath);	   /* Convert into path to .rec file*/
   
   if (netisamd){ 

    /* Set the appropriate mode parameter to access(). */
    switch (mode) {
    case OM_INPUT:
        accessmode = R_OK;
        break;
    case OM_OUTPUT:
    case OM_INOUT:
        accessmode = R_OK | W_OK;
        break;
    default:
        return (EBADARG);
    }

    if (access(datfilepath, accessmode) == -1) {
	switch (errno) {
	case EROFS:
	case EACCES:
	case EPERM:
	    return EACCES;
	case ENOENT:
	    return ENOENT;
	default:		     /* Map other errors to EFNAME */
	    return EFNAME;
	}
      }
   }
   else {                      /* check effective uid in local case */
         if ((fd = open(datfilepath,mode)) == -1) {
	     switch (errno) { 
	     case EROFS:    
	     case EACCES:
	     case EPERM:
		 return EACCES;
    	     case ENOENT:   
	          return ENOENT;
	     default:                /* Map other errors to EFNAME */
		 return EFNAME;
	     }
           }
         else
	    close(fd);
     }    

    return (ISOK);
}

