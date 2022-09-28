/* @(#)folder.h       3.8 - 94/05/04 */

/* defines externally visible objects to mail system */

#ifndef __folder_h__
#define __folder_h__

#include <sys/types.h>
#include <stdarg.h>

/*
 * the structure that holds all info about a folder (as far as the
 * support library is concerned...
 */
struct folder_obj {
	char *fo_file_name;
	int fo_fd;
	struct buffer_map *fo_buffer;
	long fo_size;
	struct msg *fo_first_msg;
	struct msg *fo_last_msg;
	char *fo_last_from;
	void *fo_lockobj;
	time_t fo_mod_time;
	int fo_num_msgs;
	gid_t fo_rgid;	/* real gid */
	gid_t fo_egid;	/* effective gid */
	char fo_changed;
	char fo_compressed;
	char fo_locked;
	char fo_garbage;
	char fo_read_only;
	char fo_ttlocked;
};



/*
 * A folder contains multiple messages.  Key folders
 * are the system "inbox", and files within the "folders"
 * directory
 */

typedef unsigned long (*FmEnumerateFunc)(struct msg *, va_list);

struct __folder_methods {
	int (*fm_add)(char *path, int (*func)(), void *obj, void *arg);
	void (*fm_delete)(struct folder_obj *);
	void (*fm_free)(struct folder_obj *);
	int (*fm_write)(struct folder_obj *);
	int (*fm_reread)(struct folder_obj *);

	struct folder_obj *(*fm_read)(char *path, int (*func)(), void *arg,
		gid_t effective_gid);

	unsigned long (*fm_enumerate)(struct folder_obj *,
		FmEnumerateFunc, ...);
	int (*fm_newmail)(char *pathname);
	int (*fm_corrupt)(struct folder_obj *);

	int (*fm_modified)(struct folder_obj *);
	int (*fm_lock)(struct folder_obj *, int cmd, void *arg);
	struct folder_obj *(*fm_init)(char *buf, char *bufend);
};

/*
 * return value for fm_newmail
 */
#define	FM_NO_NEW_MAIL	0		/* Folder has no new mail */
#define	FM_NEW_MAIL	1		/* Folder has new mail */
#define	FM_NEW_ARRIVAL	2		/* Folder has new arrival mail */
#define	FM_NO_MAIL	3		/* Folder is empty */

/*
 * cmd used in fm_lock
 */
#define	FM_ULOCK	0		/* Unlock folder */
#define	FM_LOCK		1		/* Lock folder */
#define	FM_NOTIFY	2		/* Request other proc to give up */

/*
 * return value for the locking callback in fm_read
 */
#define	FM_AGAIN	0		/* Try to lock again */
#define	FM_NOLOCK	1		/* Not to lock the folder */
#define	FM_ABORT	-1		/* Give up; don't open the folder */

extern struct __folder_methods folder_methods;

#endif	!__folder_h__
