#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mnttab.h>

#define BUFSIZE   128
#define VOLD_BLOCK    "/vol/dev/diskette0"
#define VOLD_RAW      "/vol/dev/rdiskette0"
#define TMPFILE   "/tmp/.removable"
#define	MAX_SLEEP_AMT	70
#define	MIN_SLEEP_AMT	20

#define DOTS(A)    (A[0] == '.' && \
                   (A[1] == 0 || (A[1] == '.' && A[2] == 0)))

struct mntlist {
    struct mnttab   *mntl_mnt;
    struct mntlist  *mntl_next;
};

static uid_t euid = -1;
static gid_t egid = -1;

static void
userprivs()
{
    if (euid == -1) {
      /* save euid & egid */
      euid = geteuid();
      egid = getegid();
    }  
    /* if swapping to user's privs fail, we must quit */
    if (seteuid(getuid()) != 0 || setegid(getgid()) != 0)
      exit(1);
}

static void
superprivs()
{
    if (euid == -1) {
      /* save euid & egid */
      euid = geteuid();
      egid = getegid();
    }
    if (seteuid(euid) != 0 || setegid(egid) != 0)
      exit(1);
}

void
run_command(path, argv, root)
    char *path;
    char **argv;
    int root;
{
	pid_t pid;
	int stat_loc;

	switch (pid = vfork()) {
		case -1:   /* Error */
			perror("vfork");
			_exit(1);
		case 0:    /* Inside child */
                        /* give up privs, if root is true we let child
                         * inherit effective uid and thus root powers */
                        if ( !root ) {
                                userprivs();
			}
			if ( root == 2 ) {
				/* Need to close tty stdin */
				close(0);
			}
			execv(path, argv);
			perror("execv");
			_exit(2);
		default:   /* Back in parent */
                        if ( waitpid(pid, &stat_loc, 0) < 0 ) {
				perror("wait");
			}
	}
}


static int
run_format(path, argv, root, fp, err_file)
    char *path;
    char **argv;
    int root;
    FILE **fp;
    char *err_file;
{
	pid_t pid;
	int stat_loc;

	userprivs();
	if ((*fp = fopen(err_file, "a")) == NULL) {
		superprivs();
		unlink(err_file);
		userprivs();
		if ((*fp = fopen(err_file, "a")) == NULL) {
			fprintf(stderr, "ff.core: error in open.\n");
			superprivs();
			return -1;
		}
	}
	superprivs();

	if (dup2(fileno(*fp), 1) == -1 || dup2(fileno(*fp), 2) == -1) {
		fprintf(stderr, "ff.core: error in dup2\n");
		fclose(*fp);
		unlink(err_file);
		return -1;
	}
	switch (pid = vfork()) {
		case -1:   /* Error */
			perror("vfork");
			_exit(1);
		case 0:    /* Inside child */
                        if ( !root )
                                userprivs();
                        execv(path, argv);
			perror("execv");
			_exit(2);
		default:   /* Back in parent */
			if ( waitpid(pid, &stat_loc, 0) < 0 )
				perror("wait");
			rewind(*fp);
			return 0;
	}
}


char *
get_block_dev(mountpt)
char *mountpt;
{
	FILE *fp;
	struct mnttab *mp;
	char *block_device = NULL;

	if ( (fp = fopen(MNTTAB, "r")) == NULL ) {
		perror("fopen");
	} else {
		mp = (struct mnttab *)malloc(sizeof(struct mnttab));
		while ( getmntent(fp, mp) != -1 ) {
			if ( strcmp(mp->mnt_mountp, mountpt) == 0 ) {
				block_device = mp->mnt_special;
				break;
			}
		}
		free(mp);
		fclose(fp);
	}
	return block_device;
}

char *
reconstruct_mntpt(mountpt, volname)
char *mountpt;
char *volname;
{
	static char *new_mntpt = NULL;
	int slash_count = 0;
	int k = 0;

	if (!new_mntpt)
		if (!(new_mntpt = (char *) malloc(BUFSIZE)))
			exit(1);

	while ( slash_count < 2 ) {
		if ( mountpt[k] == '/' ) {
			slash_count++;
		}
		new_mntpt[k] = mountpt[k];
		k++;
	}
	strncat(new_mntpt, volname, BUFSIZE - strlen(BUFSIZE));
	return new_mntpt;
}

char *
get_vol(device)
char *device;
{
	char *dev = NULL;
	char *tmp = NULL;
	static char *name = NULL;
	int k;

	if (!name) 
		if (!(name = (char *) malloc(BUFSIZE)))
			exit(1);

	if ( (tmp = (char *)strdup((char *)strstr(device, "diskette0"))) != NULL ) {
		dev = (char *)strtok(tmp, "/");
		dev = (char *)strtok((char *)NULL, "\0");
		if( strlen(dev) > BUFSIZE - 1)
			exit(1);
		strncpy(name, dev, BUFSIZE);
		free(tmp);
		return name;
	} else {
		return NULL;
	}
}

char *
get_newpath(device, oldvol, newvol)
char *device;
char *oldvol;
char *newvol;
{
	static char *newpath;
	int k = 0;

	if (!newpath) 
		if (!(newpath = (char *) malloc(BUFSIZE)))
			exit(1);

	if ( (k = strlen(oldvol)) > 0 ) {
		if(strlen(device) - k + strlen(newvol) > BUFSIZE - 1)
			exit(1);
		strncpy(newpath, device, (strlen(device) - k));
		strncat(newpath, newvol, BUFSIZE - strlen(newpath));
		return newpath;
	} else {
		return NULL;
	}
}

void
update_tmpfile(popup, newvol)
int popup;
char *newvol;
{
	FILE *fp;
	char *buf;

	if (buf = (char *) malloc(BUFSIZE)) {
		snprintf(buf, BUFSIZE, "%s/floppy0", TMPFILE);
		unlink(buf);
			userprivs();
		if ( (fp = fopen(buf, "w")) == NULL ) {
			perror("fopen");
		} else {
			fprintf(fp, "/floppy/%s %s/%s", newvol, VOLD_RAW, newvol);
			fclose(fp);
		}
		/* Update /tmp/.removable's mtime */
		if ( (utime(TMPFILE, (struct utimbuf *)NULL)) == -1 ) {
			perror("utime");
		}
		superprivs();
		free(buf);
	}
}

static struct mnttab *
dupmnttab(struct mnttab *mnt)
{
	struct mnttab *new;

	new = (struct mnttab *)malloc(sizeof(*new));
	if (new == NULL)
		goto alloc_failed;
	memset((char *)new, 0, sizeof(*new));
	new->mnt_special = (char *)strdup(mnt->mnt_special);
	if (new->mnt_special == NULL)
		goto alloc_failed;
	new->mnt_mountp = (char *)strdup(mnt->mnt_mountp);
	if (new->mnt_mountp == NULL)
		goto alloc_failed;
	new->mnt_fstype = (char *)strdup(mnt->mnt_fstype);
	if (new->mnt_fstype == NULL)
		goto alloc_failed;
	new->mnt_mntopts = (char *)strdup(mnt->mnt_mntopts);
	if (new->mnt_mntopts == NULL)
		goto alloc_failed;
	new->mnt_time = (char *)strdup(mnt->mnt_time);
	if (new->mnt_time == NULL)
		goto alloc_failed;

	return (new);

alloc_failed:
	fprintf(stderr, "dupmnttab: no memory\n");
	return (NULL);
}

static void
freemnttab(struct mnttab *mnt)
{
	free(mnt->mnt_special);
	free(mnt->mnt_mountp);
	free(mnt->mnt_fstype);
	free(mnt->mnt_mntopts);
	free(mnt->mnt_time);
	free(mnt);
}

void
mnt_rename(char *from, char *to, char *tospec)
{
	FILE		*fp;
	struct mnttab	mnt, mntrename, mntref;
	int		ret;
	struct mntlist	*mntl, *mntl1, *mntl_head = NULL;
	
	/*
	 * open mnttab
	 */
	if ((fp = fopen(MNTTAB, "r+")) == NULL) {
		perror(MNTTAB);
		return;
	}

	/*
	 * lock /etc/mnttab
	 */
	if (lockf(fileno(fp), F_LOCK, 0L) < 0) {
		fprintf(stderr, "Cannot lock %s: ", MNTTAB);
		perror("");
		goto out;
	}

	/*
	 * Read the mount table into memory.
	 */
	while (getmntent(fp, &mnt) == 0) {
		if ((mntl = (struct mntlist *)malloc(sizeof(*mntl))) == NULL) {
			perror(MNTTAB);
			goto out;
		}
		if (mntl_head == NULL)
			mntl_head = mntl;
		else
			mntl1->mntl_next = mntl;
		mntl1 = mntl;
		mntl->mntl_next = NULL;
		mntl->mntl_mnt = dupmnttab(&mnt);
		if (mntl->mntl_mnt == NULL) {
			fprintf(stderr, "Out of memory\n");
			goto out;
		}
		/*
		 * rename the mount point.
		 */
		if (strcmp(mntl->mntl_mnt->mnt_mountp, from) == 0) {
			free(mntl->mntl_mnt->mnt_mountp);
			mntl->mntl_mnt->mnt_mountp = (char *)strdup(to);
			free(mntl->mntl_mnt->mnt_special);
			mntl->mntl_mnt->mnt_special = (char *)strdup(tospec);
		}
	}

	/*
	 * rewind our file pointer and truncate /etc/mnttab.
	 * we do the ftruncate to avoid garbage at the end
	 * of the file if it shrinks.
	 */
	rewind(fp);
	if (ftruncate(fileno(fp), 0) < 0) {
		fprintf(stderr, "Cannot truncate %s: ", MNTTAB);
		perror("");
		goto out;
	}

	/*
	 * Write mnttab back out.
	 */
	for (mntl = mntl_head; mntl; mntl = mntl->mntl_next) {
		if (putmntent(fp, mntl->mntl_mnt) <= 0) {
			perror("putmntent");
			goto out;
		}
	}

out:
	(void) fclose(fp);

	/*
	 * Free any memory we may have allocated.
	 */
	for (mntl = mntl_head; mntl; mntl = mntl1) {
		mntl1 = mntl->mntl_next;
		freemnttab(mntl->mntl_mnt);
		free(mntl);
	}
}

void
ff_rename(oldraw, newvol, mnt_point)
char *oldraw;
char *newvol;
char *mnt_point;
{
	char *buf = NULL;
	char *tospec = NULL;
	char *oldvol;

	if ((buf = (char *) malloc(BUFSIZE)) &&
		(tospec = (char *) malloc(BUFSIZE))) {

		/* Rename the volume */
		if(strncmp(oldraw, "/vol/", 5) || strstr(oldraw, "/..")
			   || strchr(newvol, '/') || strncmp(mnt_point, "/floppy/", 8)
			   || strchr(mnt_point + 8, '/'))
			exit(1);
		oldvol = get_vol(oldraw);

		rename(oldraw, get_newpath(oldraw, oldvol, newvol));
		/* Protect against buffer overflow */
		if(strlen(newvol) > BUFSIZE - 1 - strlen("/vol/dev/diskette0/"))
			exit(1);
		/* Rename the mount point */
		snprintf(buf, BUFSIZE, "/floppy/%s", newvol);

		rename(mnt_point, buf);
		sync();
		/* Update mnttab */
		snprintf(tospec, BUFSIZE, "/vol/dev/diskette0/%s", newvol);
		mnt_rename(mnt_point, buf, tospec);

		/*	Fix for 1214529 - fake an eject and insert	*/
		/*	This removes the old /floppy/floppy0 link	*/
		/*	and ensures that the disk icon is not lost	*/

		{
			char *argv_tmp[3];
			argv_tmp[0] = "volrmmount";
			argv_tmp[1] = "-e";
			argv_tmp[2] = NULL;
			run_command("/usr/bin/volrmmount", argv_tmp, 1);
			argv_tmp[1] = "-i";
			run_command("/usr/bin/volrmmount", argv_tmp, 1);
		}

		/* Update /tmp/.removable/ file */
		update_tmpfile(0, newvol);
	}
	if (buf)
		free(buf);
	if (tospec)
		free(tospec);
}

/*
 * Returns a pointer to the latest entry in the VOLD_{RAW,BLOCK} directory.
 * XXX Since I am putting it to sleep for a second, there should be an entry.
 * This function returns NULL if there is no entry in the directory.
 * NOTE that the caller of this routine is responsible for freeing
 * the return value of this routine.
 */
char *
get_new_raw_block(raw)
int raw;
{
	DIR *dirp;
	struct dirent *cur;
	struct stat stbuf;
	char *fname;
	char *last_entry = NULL;
	char *curdir;
	int last_mtime = 0;

	if (fname = malloc(MAXPATHLEN)) {
		if ( raw ) {
			curdir = (char *)VOLD_RAW;
		} else {    /* Get block device path */
			curdir = (char *)VOLD_BLOCK;
		}
		if ( (dirp = opendir(curdir)) != NULL ) {
			while ( (cur = readdir(dirp)) != NULL ) {
				if ( DOTS(cur->d_name) )
					continue;
				snprintf(fname, MAXPATHLEN, "%s/%s", curdir, cur->d_name);
				stat(fname, &stbuf);
				if ( (stbuf.st_mode & S_IFMT) != S_IFDIR ) {
					if ( stbuf.st_mtime >= last_mtime ) {
						last_mtime = stbuf.st_mtime;
						if ( last_entry != NULL ) {
							free(last_entry);
						}
						last_entry = (char *)strdup(fname);
					}
				}
			}
			(void)closedir(dirp);
		}
		free(fname);
	}
	return last_entry;
}

static void
format_it(path, argv, fp)
char *path;
char **argv;
FILE *fp;
{
	char *err_file = NULL;
	char *giant_buf = NULL;
	char *ptr = NULL;

	if ((err_file = (char *) malloc(40)) &&
		(giant_buf = (char *) malloc(MAXPATHLEN))) {
		snprintf(err_file, 40, "/tmp/.ff.core%d", getpid());
		if (run_format(path, argv, 0, &fp, err_file) == -1)
			exit(1);
		while (fgets(giant_buf, MAXPATHLEN, fp) != NULL) {
			if ((ptr = (char*)strstr(giant_buf, "busy")) != NULL) 
				fprintf(stderr, "ff.core: Device is busy\n");
			else if ((ptr = (char*)strstr(giant_buf, "diskette is write protected")) != NULL ) 
				fprintf(stderr, "ff.core: Diskette is write protected\n");
		}
		fclose(fp);
		unlink(err_file);
		if (ptr != NULL)
			exit(1);
	}
	if (err_file)
		free(err_file);
	if (giant_buf)
		free(giant_buf);
}

static int
name_will_change(char *old_nm, char *new_label)
{
	if (strncmp(old_nm, "unlabeled", 9) == 0) {
		/* anytime you start with "unlabeled" it'll change */
		return (1);
	}
	if ((new_label != NULL) &&
	    (strcmp(new_label, old_nm) != 0)) {
		/* new label specified, and different from current label */
		return (1);
	}
	/* name shouldn't change */
	return (0);
}

static void
sleep_until_gone(char *raw_path, unsigned max_sleep_amt)
{
	struct stat	sb;
	unsigned	cnt = 0;	/* how many secs we've slept */

	while (cnt++ < max_sleep_amt) {
		if (stat(raw_path, &sb) < 0) {
			/* must be gone if stat fails */
			sleep(3);
			return;
		}
		(void) sleep(1);
	}
	/* give up and return */
}


char *newenv[] = { "PATH=/usr/bin:/usr/sbin" , 0};
extern char **environ;

void
main(argc, argv)
int argc;
char *argv[];
{
	char *buf;
	char *tmp;
	char *block_device = NULL;
	char *raw_device = NULL;
	char *mnt_point = NULL;
	char *label = NULL;
	char *oldvol = NULL;
	int format_type = 0;
	int popup_type = 1;
	int user = 0;
	int i;
	struct mnttab *mp;
	FILE *fp;
	struct stat stbuf;
	char *argv_tmp[10];

	if (!(buf = malloc(BUFSIZE)))
		exit(0);
	if (!(tmp = malloc(80)))
		exit(0);

	environ = newenv;
	umask(022);
	/* RENAME */
	if ( strcmp(argv[1], "-r") == 0 ) {
		ff_rename(argv[2], argv[3], argv[4]);
		exit(0);
	}

	popup_type = (int)atoi(argv[1]);
	format_type = (int)atoi(argv[2]);
	raw_device = (char *)strdup(argv[3]);
	/* make sure raw_device exists; security hole fix */
	if (stat(raw_device, &stbuf) == -1) 
		exit(1);
	mnt_point = (char *)strdup(argv[4]);
	if ( argc == 6 )
		label = (char *)strdup(argv[5]);

	/* Need to unmount floppy before formatting */
	if ( !popup_type ) {
		/* Get block device path first */
		if ( (fp = fopen(MNTTAB, "r")) == NULL ) {
			perror("fopen");
		} else {
			mp = (struct mnttab *)malloc(sizeof(struct mnttab));
			while ( getmntent(fp, mp) != -1 ) {
				if ( strcmp(mp->mnt_mountp, mnt_point) == 0 ) {
					block_device = (char *)strdup(mp->mnt_special);
					break;
				}
			}
			free(mp);
			fclose(fp);
		}
		if ( block_device == NULL ) {
			/* Assume raw device path is valid. 
			 * Derive block device path from raw device path.
			 */
			strncpy(tmp, get_vol(raw_device), 80);
			snprintf(buf, BUFSIZE, "%s/%s", VOLD_BLOCK, tmp);
			block_device = (char *)strdup(buf);
			free(mnt_point);
			snprintf(buf, BUFSIZE, "/floppy/%s", tmp);
			mnt_point = (char *)strdup(buf);
			mkdir(mnt_point, 777);
		} else {
			/* Check if mount point exist */
			if ( (stat(mnt_point, &stbuf)) == ENOENT ) {
				return;
			} else {
				argv_tmp[0] = "umount";
				argv_tmp[1] = mnt_point;
				argv_tmp[2] = NULL;
				run_command("/usr/sbin/umount", argv_tmp, 1);
			}
		}
	}

	fp = 0;
	/* Format and mount floppy */
	switch (format_type) {
		case 0:    /* UNIX */
			/* Put UNIX file system on diskette */
			if ( popup_type ) {
				/* Only fdformat if unformatted or unlabeled */
				/* redirect stderr to stdout so we can check errors */
				argv_tmp[0] = "fdformat";
				argv_tmp[1] = "-q";
				argv_tmp[2] = "-f";
				argv_tmp[3] = raw_device;
				argv_tmp[4] = NULL;
				format_it("/usr/bin/fdformat", argv_tmp, fp);
                                /* vold assigns a volume name after fdformat */
				if (name_will_change(mnt_point, label)) {
				    sleep_until_gone(raw_device, MAX_SLEEP_AMT);
				} else {
				    /*in case name_will_change() is wrong*/
				    sleep(MIN_SLEEP_AMT);
				}

				free(raw_device);
				raw_device = get_new_raw_block(1);
				free(block_device);
				block_device = get_new_raw_block(0);
				free(mnt_point);

				if (stat("/floppy", &stbuf) == -1) {
				   mkdir("/floppy", 777);
				   chmod("/floppy", S_IRWXU | S_IRGRP | S_IXGRP
					 | S_IROTH | S_IXOTH);
				}
				snprintf(buf, BUFSIZE, "/floppy/%s", get_vol(block_device));
				mnt_point = (char *)strdup(buf);
				mkdir(mnt_point, 777);
			}
			user = getuid();
                        argv_tmp[0] = "newfs";
                        argv_tmp[1] = raw_device;
                        argv_tmp[2] = NULL;
                        run_command("/usr/sbin/newfs", argv_tmp, 2);

                        argv_tmp[0] = "mount";
                        argv_tmp[1] = "-F";
                        argv_tmp[2] = "ufs";
                        argv_tmp[3] = "-o";
                        argv_tmp[4] = "nosuid";
                        argv_tmp[5] = block_device;
                        argv_tmp[6] = mnt_point;
                        argv_tmp[7] = NULL;
			run_command("/usr/sbin/mount", argv_tmp, 1);
			chmod(mnt_point, S_IRWXU | S_IRWXG | S_IRWXO);
			chown(mnt_point, user, getgid());
			if ( label ) {
				ff_rename(raw_device, label, mnt_point);
			} else {
				update_tmpfile(popup_type, get_vol(block_device));
			}
			break;
		case 1:    /* DOS high density */
			i=0;
			argv_tmp[i++] = "fdformat";
			argv_tmp[i++] = "-q";
			argv_tmp[i++] = "-f";
			argv_tmp[i++] = "-d";
			if ( (label != NULL) && (strlen(label) > 0) ) {
				argv_tmp[i++] = "-b";
				argv_tmp[i++] = label;
			}
			argv_tmp[i++] = raw_device;
			argv_tmp[i++] = NULL;
		case 2:    /* NEC-DOS medium density */
			if ( format_type == 2 ) {
				i=0;
				argv_tmp[i++] = "fdformat";
				argv_tmp[i++] = "-q";
				argv_tmp[i++] = "-f";
				argv_tmp[i++] = "-t";
				argv_tmp[i++] = "nec";
				argv_tmp[i++] = "-m";
				if ( (label != NULL) && (strlen(label) > 0) ) {
				    argv_tmp[i++] = "-b";
				    argv_tmp[i++] = label;
				}
				argv_tmp[i++] = raw_device;
				argv_tmp[i++] = NULL;
			}
			format_it("/usr/bin/fdformat", argv_tmp, fp);
			if ( popup_type ) {
                                /* vold assigns a volume name after fdformat */
                                if (name_will_change(mnt_point, label)) {
                                    sleep_until_gone(raw_device, MAX_SLEEP_AMT);                                } else {
                                    /*in case name_will_change() is wrong*/
                                    sleep(MIN_SLEEP_AMT);
                                }

				free(block_device);
				block_device = get_new_raw_block(0);
				free(mnt_point);

                                if (stat("/floppy", &stbuf) == -1) {
                                   mkdir("/floppy", 777);
                                   chmod("/floppy", S_IRWXU | S_IRGRP | S_IXGRP
                                         | S_IROTH | S_IXOTH);
                                }
				snprintf(buf, BUFSIZE, "/floppy/%s", get_vol(block_device));
				mnt_point = (char *)strdup(buf);
				mkdir(mnt_point, 777);
			}

			argv_tmp[0] = "mount";
			argv_tmp[1] = "-F";
			argv_tmp[2] = "pcfs";
			argv_tmp[3] = block_device;
			argv_tmp[4] = mnt_point;
			argv_tmp[5] = NULL;
			run_command("/usr/sbin/mount", argv_tmp, 1);
			if ( label ) 
				ff_rename(raw_device, label, mnt_point);
			else 
				update_tmpfile(popup_type, get_vol(block_device));
			break;
	}
}
