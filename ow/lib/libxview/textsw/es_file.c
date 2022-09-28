#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)es_file.c 20.51 93/11/05";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Entity stream implementation for disk files.
 * 
 * A file is acting as one of three kinds of streams: 1) a read-only source of
 * characters (an "original" stream for ps_impl.c) or, 2) as a write_only
 * sink of characters (a backup or checkpoint), or 3) as a read-write edit
 * history (a "scratch" stream). A scratch stream can get VERY large, and
 * could choose to "wrap around" and re-use the bytes of the underlying file
 * to conserve file space. However, since ps_impl.c must support large
 * scratch streams for memory streams as well, it implements the wrap-around,
 * thereby changing the expected access pattern to the stream implemented in
 * this module.
 * 
 * Based on the above, the implementation uses two buffers: 1) a read buffer,
 * that contains the current insertion point and some portion of the
 * characters before and after it, and 2) a write buffer, that contains the
 * last point at which more than 4 bytes were written at once, plus some of
 * the characters around it. This peculiar requirement for more than 4 bytes
 * is because ps_impl.c keeps updating the count of the number of characters
 * in the last contiguous insertion sequence. The read buffer always matchs
 * an existing portion of the file as it exists on disk, but the write buffer
 * can be "off the end", containing characters that have not yet been sent to
 * disk.  Thus, there can be valid indices in [length_on_disk..length) that
 * are not valid positions for the underlying file! For an original stream,
 * the write buffer is NULL, and length == length_on_disk.
 * 
 * --- Misc. notes that may result in changes ... --- An empty original stream
 * need not be open except to prevent another process or some piece of client
 * code ripping it out from under this module. However, delayed opening moves
 * where certain error conditions have to be handled by clients. A original
 * stream pointed at a non-existent file could treat it as auto-creation of
 * an empty stream, and then act as appropriate to an empty stream.
 * 
 * 
 * Considerations for file consistency: Sun Unix 3.X (and BSD 4.X): For a local
 * file system, write(2) does not report success unless there is space
 * available on the disk for the data and write(2) claims that space. For a
 * NFS file system, write(2) returns as soon as it has transferred all of the
 * user data into kernel buffers.  There need not be enough space for that
 * data on the remote disk, and only a successful fsync(2) guarantees that
 * the data is on the remote disk.  If the fsync(2) fails, there is no
 * indication of which data did not make it to the disk! Sun Unix 4.X (the
 * VM-rewrite): With mapped files, the local file system may have the same
 * problem as the NFS file system. NFS replacing ND: For diskless clients,
 * the local file system has the same problem as the NFS file system unless
 * it is using the (optional) RAM-disk /tmp. AT&T System V R ? There is a
 * notion of "synchronous" files, where the write(2) does not report success
 * until the data is on the disk.  SunOS 4.X provides this feature, but
 * "synchronous" files completely bypass the kernel disk cache, and thus
 * significantly slow read(2) as well as write(2). stdio fwrite(3) is not
 * guarantee to write(2) unless the stream is unbuffered. fflush(3) forces
 * the write(2), but does not provide enough information to caller for it to
 * figure out what did not get written.  Worse yet, fseek(3) can call
 * fflush(3) as a side-effect, and does not even report an error if the
 * fflush(3) fails!
 * 
 * --- And now for some history ... --- Prior to "-r 10.12", es_file used stdio
 * and only needed to work for 3.X versions of the SunOS.  The following
 * strategy was employed. To get around the delayed nature of the calls to
 * write(2), we force ES_WRITE_BUF_LEN > BUFSIZE, forcing all full-buffer
 * fwrite(3) calls to call write(2) immediately.  (We don't just want to make
 * the stream unbuffered because we need the buffering for reading).  Since
 * the only remaining partial calls to fwrite(3) should be in es_commit and
 * es_destroy, this reduces the number of places that have to be very careful
 * about disk consistency to entity_stream shutdown and es_replace callers.
 * 
 * Other problems with using stdio: Since stdio may be looking at stdin (or some
 * other file that is being asynchronously extended), stdio does a lot of
 * lseek(2) calls to see if the file has been so extended.  For the textsw,
 * this is unnecessary and costly functionality. Stdio interacts poorly with
 * ps_impl.c, because fseek fflush's the writable scratch file a lot.
 */

#include <string.h>
#include <fcntl.h>
#ifdef SVR4
#include <stdlib.h>
#include <dirent.h>
#else
#include <sys/dir.h>
#endif /* SVR4 */
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <stdio.h>
#include <xview/pkg.h>
#include <xview/attrol.h>
#include <xview_private/primal.h>
#include <xview_private/es.h>
#ifdef OW_I18N
#include <xview/generic.h>
#include <xview/server.h>
#include <euc.h>
#endif /* OW_I18N */


extern int      sys_nerr;
extern char    *sys_errlist[];
extern long     lseek();

static Es_status es_file_commit();
static Es_handle es_file_destroy();
static Es_index es_file_get_length();
static Es_index es_file_get_position();
static Es_index es_file_set_position();
static Es_index es_file_read();
static Es_index es_file_replace();
static int      es_file_set();

static struct es_ops es_file_ops = {
    es_file_commit,
    es_file_destroy,
    es_file_get,
    es_file_get_length,
    es_file_get_position,
    es_file_set_position,
    es_file_read,
    es_file_replace,
    es_file_set
};

typedef struct _es_file_buf {
    Es_index        start;	/* Disk position, valid iff used > 0 */
    unsigned        used;	/* # valid chars in buf */
    void           *chars;
}               es_file_buf;
typedef es_file_buf *Es_file_buf;

#define	BUF_INVALIDATE(_buf)	(_buf)->used = 0
#define	BUF_LAST_PLUS_ONE(_buf)	((_buf)->start + (_buf)->used)
#define	BUF_CONTAINS_POS(_buf, _pos)					\
	((_buf)->used > 0 &&						\
	(_buf)->start <= (_pos) && (_pos) < BUF_LAST_PLUS_ONE(_buf))

struct private_data {
    Es_status       status;
    char           *name;
#ifndef BACKUP_AT_HEAD_OF_LINK
    char           *true_name;	/* Non-null iff name was sym link */
#endif
    unsigned        flags, options;
    Xv_opaque       client_data;
    Es_index        length, length_on_disk, pos;
    int             fd;
#ifdef obsolete
    FILE           *file;
#endif
    es_file_buf     read_buf;	/* cache for read's */
    es_file_buf     write_buf;	/* cache for replace's */
#ifdef OW_I18N
    int             mb_fd;	/* Original multi-bytes file */
    int             skipped;
    char           *wc_filename;	/* Name of the wchar backup file */
#endif    
};
typedef struct private_data *Es_file_data;

/* Bits for flags */
#define COMMIT_DONE	0x00000001

#define	ABS_TO_REP(esh)	(Es_file_data)esh->data


/*
 * Some invariants for read_buf and write_buf: 1) The buffers are allowed to
 * overlap, but read's must retrieve from the write_buf first, to ensure that
 * the client reads what it wrote.
 */

static char    *file_name_only_msgs[] = {
     /* 0 */ 0,
     /* 1 */ 0,
     /* 2 */ 0,
     /* 3 */ 0,
     /* 4 */ 0,
     /* 5 */ 0
};

/*
   added a static routine for checking if the start to end overlaps with
   the read buffer.  If it does, then invalidate the read buffer.
   XXX: Should actually update the read buffer to reflect the change in
   the buf. 
*/

static void
update_read_buf(private,start,end,buf)
    void           *buf;
    Es_file_data    private;
    Es_index        start;
    Es_index        end;
{
    if (private->read_buf.used > 0 &&
	start < BUF_LAST_PLUS_ONE(&private->read_buf) &&
	end > private->read_buf.start) {
	/*
	 * There is overlap: in future, might be better to update
	 * read_buf, but for now, just discard it.
	 */
	BUF_INVALIDATE(&private->read_buf);
    }
}
    
Pkg_private int
es_file_append_error(error_buf, file_name, status)
    char           *error_buf;
    char           *file_name;
    Es_status       status;
/* Messages appended to error_buf have no trailing newline */
{
    register char  *first_free_in_buf;
    register int    msg_index = 0;
    static int init_msg = 0;

    if (error_buf == 0)
	return;	

    if (! init_msg)  {
        file_name_only_msgs[0] = XV_MSG("cannot read file '%s'");
        file_name_only_msgs[1] = XV_MSG("'%s' does not exist");
        file_name_only_msgs[2] = XV_MSG("not permitted to access '%s'");
        file_name_only_msgs[3] = XV_MSG("'%s' is not a file of ASCII text");
        file_name_only_msgs[4] = XV_MSG("too many symbolic links from '%s'");
        file_name_only_msgs[5] = XV_MSG("out of space for file '%s'");
	init_msg = 1;
    }

    first_free_in_buf = error_buf + strlen(error_buf);
    if (status & ES_CLIENT_STATUS(0)) {
	(void) sprintf(first_free_in_buf,
		       XV_MSG("INTERNAL error for file '%s', status is %ld"),
		       file_name, status);
	return;
    }
    switch (ES_BASE_STATUS(status)) {
      case ES_SUCCESS:
	break;			/* Caller is REALLY lazy! */
      case ES_CHECK_ERRNO:
	switch (errno) {
	  case ENOENT:
	    msg_index = 1;
	    goto Default;
	  case EACCES:
	    msg_index = 2;
	    goto Default;
	  case EISDIR:
	    msg_index = 3;
	    goto Default;
#ifndef SVR4
	  case ELOOP:
	    msg_index = 4;
	    goto Default;
#endif /* SVR4 */
	  case ENOMEM:
	    (void) strcat(error_buf, XV_MSG("alloc failure"));
	    break;
	  default:
	    if (errno <= 0 || errno >= sys_nerr)
		goto Default;
	    (void) sprintf(first_free_in_buf, 
			XV_MSG("file '%s': %s"),
			   file_name, sys_errlist[errno]);
	    break;
	}
	break;
      case ES_INVALID_HANDLE:
	(void) strcat(error_buf, XV_MSG("invalid es_handle"));
	break;
      case ES_SEEK_FAILED:
	(void) strcat(error_buf, XV_MSG("seek failed"));
	break;
      case ES_FLUSH_FAILED:
      case ES_FSYNC_FAILED:
      case ES_SHORT_WRITE:
	msg_index = 5;
	goto Default;
      default:
Default:
	(void) sprintf(first_free_in_buf, file_name_only_msgs[msg_index],
		       file_name);
    }
}

Pkg_private     Es_handle
es_file_create(name, options, status)
    char           *name;
    int             options;
    Es_status      *status;
{
    extern          fstat();
    Es_handle       esh = NEW(Es_object);
    register Es_file_data private;
    int             open_option;
    struct stat     buf;
    Es_status       dummy_status;
#ifndef BACKUP_AT_HEAD_OF_LINK
    char           *temp_name, true_name[MAXNAMLEN];
    int             link_count, true_name_len;
#endif

    if (status == 0)
	status = &dummy_status;
    *status = ES_CHECK_ERRNO;
    errno = 0;

    /* (1) Try to allocate all necessary memory */
    if (esh == NULL)
	goto AllocFailed;
    if ((private = NEW(struct private_data)) == NULL)
	goto AllocFailed;
    private->fd = -1;		/* In case of later AllocFailed */
#ifdef OW_I18N
     private->mb_fd = -1;	/* In case of later AllocFailed */
#endif

    BUF_INVALIDATE(&private->read_buf);
    if (!multibyte)         
        private->read_buf.chars = xv_malloc(ES_READ_BUF_LEN);
    else
        private->read_buf.chars = xv_malloc(ES_READ_BUF_LEN*sizeof(wchar_t));

    if (private->read_buf.chars == NULL)
	    goto AllocFailed;
    
    BUF_INVALIDATE(&private->write_buf);
    if (options & ES_OPT_APPEND) {
        if (!multibyte)         
            private->write_buf.chars = xv_malloc(ES_READ_BUF_LEN);
        else
            private->write_buf.chars = 
                          xv_malloc(ES_READ_BUF_LEN*sizeof(wchar_t));

        if (private->write_buf.chars == NULL)
	    goto AllocFailed;
    } else
	private->write_buf.chars = NULL;

    if ((private->name = strdup(name)) == NULL)
	goto AllocFailed;

#ifndef BACKUP_AT_HEAD_OF_LINK
    /* (2) Chase the symbolic link if 'name' is one. */
    for (temp_name = name, link_count = 0;
	 (link_count < MAXSYMLINKS) &&
	 (-1 != (true_name_len =
		 readlink(temp_name, true_name, sizeof(true_name))));
	 temp_name = true_name, link_count++) {
	true_name[true_name_len] = '\0';
    }
    if (link_count == MAXSYMLINKS) {
	errno = ELOOP;
	goto Error_Return;
    }
    if (temp_name == name) {
	private->true_name = NULL;
    } else
	private->true_name = strdup(true_name);
#endif /* BACKUP_AT_HEAD_OF_LINK */

    /* (3) Open up the file and check to see it is not directory. */
    open_option = (options & ES_OPT_APPEND)
	? (O_RDWR | O_TRUNC | O_CREAT)
	: (O_RDONLY);
    private->fd = open(name, open_option, 0666);
    if (private->fd < 0) {
	goto Error_Return;
    }
    private->flags = 0;
    private->options = options;
    if ((private->options & ES_OPT_APPEND) == 0) {
	if (fstat(private->fd, &buf) == -1)
	    goto Error_Return;
	if ((buf.st_mode & S_IFMT) != S_IFREG) {
	    errno = EISDIR;
	    goto Error_Return;
	}

    if (!multibyte)
        private->length = buf.st_size;
    }
    /* (4) Final fix ups. */
    
    if (!multibyte)
        private->length_on_disk = private->length;

    esh->ops = &es_file_ops;
    esh->data = (caddr_t) private;

#ifdef OW_I18N		/* Now create the wchar file */
    if (multibyte && ((private->options & ES_OPT_APPEND) == 0)) {
	private->mb_fd = private->fd;

	if (options == 0) {/* For other options, ther file should be in wchar */
	    private->fd = es_file_make_wchar_file(esh, open_option);

	    if (private->fd < 0) {
		if (private->status == ES_FLUSH_FAILED)
		    *status = ES_FLUSH_FAILED;
		goto Error_Return;
	    }
	    if (fstat(private->fd, &buf) == -1)
		goto Error_Return;
	    if ((buf.st_mode & S_IFMT) != S_IFREG) {
		errno = EISDIR;
		goto Error_Return;
	    }
	}
	private->length = buf.st_size / sizeof(wchar_t);
	private->length_on_disk = private->length;
    }
#endif /* OW_I18N */

    *status = private->status = ES_SUCCESS;
    return (esh);

AllocFailed:
    errno = ENOMEM;
Error_Return:
    if (esh) {
	free((char *) esh);
	esh = ES_NULL;
    }
    if (private) {
	if (private->read_buf.chars)
	    free(private->read_buf.chars);
	if (private->write_buf.chars)
	    free(private->write_buf.chars);
	if (private->fd >= 0)
	    (void) close(private->fd);
	free((char *) private);
	private = (Es_file_data) 0;
    }
    return (esh);
}

/* ARGSUSED */
static          caddr_t
#ifdef ANSI_FUNC_PROTO
es_file_get(Es_handle esh, Es_attribute attribute, ...)
#else
es_file_get(esh, attribute, va_alist)
    Es_handle       esh;
    Es_attribute    attribute;
va_dcl
#endif
{
    register Es_file_data private = ABS_TO_REP(esh);
#ifndef lint
    va_list         args;
#endif
    switch (attribute) {
      case ES_CLIENT_DATA:
	return ((caddr_t) (private->client_data));
      case ES_NAME:
	return ((caddr_t) (private->name));
      case ES_STATUS:
	return ((caddr_t) (private->status));
      case ES_SIZE_OF_ENTITY:
        if (!multibyte)
	    return ((caddr_t) sizeof(char));
        else
	    return ((caddr_t) sizeof(wchar_t));
      case ES_TYPE:
	return ((caddr_t) ES_TYPE_FILE);
#ifdef OW_I18N
      case ES_SKIPPED:
	return ((caddr_t) private->skipped);
#endif
      default:
	return (0);
    }
}

static int
es_file_set(esh, attrs)
    Es_handle       esh;
    Attr_avlist     attrs;
{
    register Es_file_data private = ABS_TO_REP(esh);
    Es_status       status_dummy = ES_SUCCESS;
    register Es_status *status = &status_dummy;

    for (; *attrs && (*status == ES_SUCCESS); attrs = attr_next(attrs)) {
	switch ((Es_attribute) * attrs) {
	  case ES_CLIENT_DATA:
	    private->client_data = attrs[1];
	    break;
	  case ES_FILE_MODE:
	    if (fchmod(private->fd, (int) attrs[1]) == -1)
		*status = private->status = ES_CHECK_ERRNO;
	    break;
	  case ES_STATUS:
	    private->status = (Es_status) attrs[1];
	    break;
	  case ES_STATUS_PTR:
	    status = (Es_status *) attrs[1];
	    *status = status_dummy;
	    break;
	  default:
	    *status = ES_INVALID_ATTRIBUTE;
	    break;
	}
    }
    return ((*status == ES_SUCCESS));
}

/* ARGSUSED */
static int
es_file_seek(private, pos, caller)
    register Es_file_data private;
    Es_index        pos;
    char           *caller;
{

#ifdef DEBUG
    if (private->length_on_disk < pos) {
	private->status = ES_SEEK_FAILED;
	(void) fprintf(stderr,
		       "%s: lseek to position %d > length_on_disk %d!!\n",
		       caller, pos, private->length_on_disk);
	return (1);
    }
#endif

    /* Backup file or multibyte locale, file is wchar */
    if (multibyte)
        pos *= sizeof(wchar_t);

    if (lseek(private->fd, pos, L_SET) == -1) {
	private->status = ES_SEEK_FAILED;

#ifdef DEBUG
	(void) fprintf(stderr, "Bad lseek in %s to position %d\n",
		       caller, pos);
#endif

	return (1);
    } else {
	return (0);
    }
}

static int
es_file_fill_buf(private, buf, first, last_plus_one)
    register Es_file_data private;
    register Es_file_buf buf;
    register Es_index first, last_plus_one;
{
    register int    read_in;

    if (first < last_plus_one) {
	if (es_file_seek(private, first, "es_file_fill_buf")) {
	    read_in = -1;
	    return (read_in);
	}

        if (multibyte) {
            read_in = read(private->fd, buf->chars,
	       (last_plus_one - first) * sizeof(wchar_t)) / sizeof(wchar_t);
        } else
	    read_in = read(private->fd, buf->chars, last_plus_one - first);

	if (read_in == -1 ||
	    read_in != last_plus_one - first /* paranoia */ ) {
	    private->status = ES_CHECK_ERRNO;
	    read_in = -2;
#ifdef DEBUG
	    (void) fprintf(stderr,
			   "Failed read in %s of %d chars\n",
			   "es_file_fill_buf", last_plus_one - first);
#endif
	    return (read_in);
	}
    } else {
	read_in = 0;
#ifdef DEBUG
	if (first != private->length)
	    (void) fprintf(stderr,
			   "Null read in %s at %d with length %d\n",
			   "es_file_fill_buf", first, private->length);
#endif
    }
    buf->start = first;
    buf->used = read_in;
    return (read_in);
}

static int
es_file_flush_write_buf(private, buf)
    register Es_file_data private;
    register Es_file_buf buf;
/*
 * This routine detects errors in attempted write, etc. but does not allow
 * for successful retry in all cases (e.g., short writes or failed fsynch).
 */
{
    register int    written;
    int		    num_of_bytes;

    if (buf->used == 0) {
	written = 0;
	return (written);
    }

    if (multibyte) {
        if (private->options & ES_OPT_BACKUPFILE) {
	    if (es_file_seek(private, buf->start, "es_file_flush_write_buf")) {
	        return (-1);
	    }
            num_of_bytes = buf->used * sizeof(wchar_t);
            written = write(private->fd, buf->chars, num_of_bytes);       
        } else {
            /* 
               the temp_buf is for protection wrt requests that are bigger
               than expected. This should never happen...
            */
            char dummy_write_mb_buf[(ES_WRITE_BUF_LEN + 1) * sizeof(wchar_t) ];
            char *temp_buf_ptr = dummy_write_mb_buf;

            if (buf->used > ES_WRITE_BUF_LEN)
                temp_buf_ptr = 
                     (char *)malloc((buf->used + 1) * sizeof(wchar_t));
	
	    ((wchar_t *)buf->chars)[buf->used] = NULL;
	    num_of_bytes = wcstombs(temp_buf_ptr, buf->chars,
				(buf->used +1) * sizeof(wchar_t));
	    written = write(private->fd, temp_buf_ptr, num_of_bytes);

            if (temp_buf_ptr && (temp_buf_ptr != dummy_write_mb_buf))
                free(temp_buf_ptr);
        }
    } else {
        if (es_file_seek(private, buf->start, "es_file_flush_write_buf"))
	    return (-1);
        written = write(private->fd, buf->chars, buf->used);
        num_of_bytes = buf->used;
    }

    if (written == -1 ||
	written != num_of_bytes) {
	private->status = ES_SHORT_WRITE; /* ES_FLUSH_FAILED instead? */
	written = -2;

#ifdef DEBUG
	(void) fprintf(stderr,
		       "Failed write in %s of %d chars\n",
		       "es_file_flush_write_buf", buf->used);
#endif

	return (written);
    }
    written = buf->used;
    if (buf->start + written > private->length_on_disk)
        private->length_on_disk = buf->start + written;
    BUF_INVALIDATE(buf);
    return (written);
}

static int
es_file_move_write_buf(private, include, also_include, include_offset)
    register Es_file_data private;
    register Es_index include, also_include;
    void          **include_offset;
/*
 * Caller ensures: include <= also_include < include+ES_WRITE_BUF_LEN <
 * ES_INFINITY, include <= private->length Return values: < 0 indicate
 * various errors, = 0 implies write_buf already correctly positioned, > 0
 * implies write_buf written then moved. Assumptions: if you have to write
 * any bytes, you might as well write them all. if you have to read any
 * bytes, you might as well read them all.
 */
{
    register Es_file_buf buf = &private->write_buf;
    register int    written = 0;
    Es_index        buf_last_plus_one;

    if (buf->used == 0)
	goto Fill_Buffer;
    buf_last_plus_one = BUF_LAST_PLUS_ONE(buf);
#ifdef XV_DEBUG
    if (buf_last_plus_one > private->length)
	take_breakpoint();
#endif
    /*
     * Buffer must contain include and also_include. There are two possible
     * problems, even if include is contained: 1) include is too far into the
     * buffer for also_include to also be contained, or 2) there is space
     * available in the buffer for also_include, but the buffer needs to fill
     * in bytes from the disk between its current end and also_include. If
     * include is at buffer end, or also_include is at or beyond buffer end,
     * it may be possible to simply extend the buffer iff buffer is at the
     * end of the stream and it has room left.
     */
    if (include < buf->start || include > buf_last_plus_one ||
	(include == buf_last_plus_one &&
	 include >= buf->start + ES_WRITE_BUF_LEN) ||
	(also_include >= buf_last_plus_one &&
	 (buf_last_plus_one < private->length ||
	  also_include >= buf->start + ES_WRITE_BUF_LEN))
	) {
	written = es_file_flush_write_buf(private, &private->write_buf);
	if (written < 0)
	    return (written);
	/*
	 * Possible future optimizations: 1) Deal with buf_last_plus_one <=
	 * also_include < buf->start+ES_WRITE_BUF_LEN 2) Slide buffer around to
	 * avoid full read when possible 3) Copy from read_buf to avoid full
	 * read when possible
	 */
Fill_Buffer:
	if (es_file_fill_buf(private, buf, include,
			   include + ES_WRITE_BUF_LEN > private->length_on_disk
		 ? private->length_on_disk : include + ES_WRITE_BUF_LEN) < 0) {
	    written = -4;
	    return (written);
	}
    }
    if (!multibyte)
        *include_offset = (char *)(buf->chars) + (include - buf->start);
    else
        *include_offset = (wchar_t *)buf->chars + (include - buf->start);
    return (written);
}

static void
es_file_maybe_truncate_buf(buf, new_last_plus_one)
    register Es_file_buf buf;
    register Es_index new_last_plus_one;
{
    if (buf->used > 0 && new_last_plus_one < BUF_LAST_PLUS_ONE(buf)) {
	buf->used = (new_last_plus_one < buf->start)
	    ? 0 : new_last_plus_one - buf->start;
    }
}

static          Es_status
es_file_commit(esh)
    Es_handle       esh;
{
    register Es_file_data private = ABS_TO_REP(esh);

    if (es_file_flush_write_buf(private, &private->write_buf) < 0) {
	return (private->status);
    }
    if (fsync(private->fd) == -1) {
#ifdef DEBUG
    (void) fprintf(stderr,
       "Failed fsynch in es_file_commit!\n");
#endif
       /* BUG ALERT!  Who knows what state the file is in? */
       return (ES_FSYNC_FAILED);
    }
    private->flags |= COMMIT_DONE;
    return (ES_SUCCESS);
}


static          Es_handle
es_file_destroy(esh)
    Es_handle       esh;
{
    register Es_file_data private = ABS_TO_REP(esh);

    if (private->write_buf.chars) {
#ifdef XV_DEBUG
	if ((private->write_buf.used > 0) &&
	    (private->flags & COMMIT_DONE)) {
	    /*
	     * Caller should have called es_commit in order to guarantee
	     * appropriate recovery in case of errors.
	     */
	    take_breakpoint();
	}
#endif
	free(private->write_buf.chars);
    }
    (void) close(private->fd);
    private->fd = -1;

    if (private->mb_fd > 0) {
	(void)close(private->mb_fd);
	private->mb_fd = -1;
    }

    if ((private->options & ES_OPT_APPEND) &&
	(private->flags & COMMIT_DONE) == 0) {
	(void) unlink(private->name);
    }

    if (private->wc_filename) {
	(void) unlink(private->wc_filename);
	xv_free(private->wc_filename);
	private->wc_filename = NULL;
    }

    free((char *) esh);
    free(private->read_buf.chars);
#ifndef BACKUP_AT_HEAD_OF_LINK
    free((char *)private->true_name);
#endif
    free(private->name);
    free((char *) private);
    return (NULL);
}

static          Es_index
es_file_get_length(esh)
    Es_handle       esh;
{
    register Es_file_data private = ABS_TO_REP(esh);
    return (private->length);
}

static          Es_index
es_file_get_position(esh)
    Es_handle       esh;
{
    register Es_file_data private = ABS_TO_REP(esh);
    return (private->pos);
}

static          Es_index
es_file_set_position(esh, pos)
    Es_handle       esh;
    register Es_index pos;
{
    register Es_file_data private = ABS_TO_REP(esh);
    if (pos > private->length)
	pos = private->length;
    private->pos = pos;
    return (private->pos);
}

static          Es_index
es_file_read(esh, count, buf, count_read)
    Es_handle       esh;
    int             count;
    register int   *count_read;
    void           *buf;
/*
 * Needed characters may be in the read_buf, the write_buf, or on disk, or
 * some combination of all three.
 */
{
    register Es_file_data private = ABS_TO_REP(esh);
    register Es_index pos = private->pos, lpo;
    register int    to_read, still_needed;
    es_file_buf     dummy_read_buf;

    /*
     * Client may request more bytes than are available, so count cannot be
     * trusted in the following code.
     */
    *count_read = (count > private->length - pos)
	? (private->length - pos) : count;
    for (still_needed = *count_read;
	 still_needed > 0;
	 still_needed -= to_read, pos += to_read) {
	/*
	 * Figure out where the next set of characters is coming from. The
	 * write_buf has precedence over the read_buf in the tests so that
	 * overlap range reads from the write_buf!
	 */
	if (BUF_CONTAINS_POS(&private->write_buf, pos)) {
	    to_read = BUF_LAST_PLUS_ONE(&private->write_buf) - pos;
	    if (to_read > still_needed)
		to_read = still_needed;

            if (!multibyte)
	        XV_BCOPY((char *)(private->write_buf.chars) + 
                      (pos - private->write_buf.start),
		      (char *)buf + (*count_read - still_needed), to_read);
            else
	        XV_BCOPY((wchar_t *)(private->write_buf.chars) + 
                      (pos - private->write_buf.start),
		      (wchar_t *)buf + (*count_read - still_needed), 
                        to_read * sizeof(wchar_t));

	} else if (BUF_CONTAINS_POS(&private->read_buf, pos)) {
	    to_read = BUF_LAST_PLUS_ONE(&private->read_buf) - pos;
	    if (to_read > still_needed)
		to_read = still_needed;

            if (!multibyte)
	        XV_BCOPY((char *)private->read_buf.chars + 
                      (pos - private->read_buf.start),
		      (char *)buf + (*count_read - still_needed), to_read);
            else
	        XV_BCOPY((wchar_t *)private->read_buf.chars + 
                      (pos - private->read_buf.start),
		      (wchar_t *)buf + (*count_read - still_needed), 
                          to_read * sizeof(wchar_t));

	} else if (still_needed <= ES_READ_BUF_LEN) {
	    /*
	     * Since we have to read from disk, might as well get as many
	     * characters as possible.
	     */
	    lpo = pos + ES_READ_BUF_LEN;
	    if (lpo > private->length_on_disk)
		lpo = private->length_on_disk;
	    /* Overlap with write_buf is avoided for good hygiene. */
	    if (private->write_buf.used > 0 &&
		pos < private->write_buf.start &&
		lpo > private->write_buf.start)
		lpo = private->write_buf.start;
	    if (es_file_fill_buf(private, &private->read_buf, pos, lpo)
		< 0) {
		*count_read = 0;
		pos = private->pos;
		goto Return;
	    }
	    /*
	     * Go around again and characters will get copied from read_buf
	     * in earlier part of loop.
	     */
	    to_read = 0;
	} else {
	    /* Read directly from the disk */
            if (!multibyte)
	        dummy_read_buf.chars = 
                   (char *)buf + (*count_read - still_needed);
            else 
	        dummy_read_buf.chars = 
                   (wchar_t *)buf + (*count_read - still_needed);

	    lpo = pos + still_needed;
	    if (lpo > private->length_on_disk)
		lpo = private->length_on_disk;
	    /* Overlap with write_buf is forbidden. */
	    if (private->write_buf.used > 0 &&
		lpo > private->write_buf.start)
		lpo = private->write_buf.start;
	    if (es_file_fill_buf(private, &dummy_read_buf, pos, lpo)
		< 0) {
		*count_read = 0;
		pos = private->pos;
		goto Return;
	    }
	    to_read = dummy_read_buf.used;
	}
    }
Return:
    private->pos = pos;
    return (pos);
}

/* Following enumeration details the three possible types of replace. */
typedef enum {
    esfr_truncate, esfr_overwrite, esfr_insert
}               Esfr_mode;

static          Es_index
es_file_replace(esh, last_plus_one, count, buf, count_used)
    Es_handle       esh;
    register int    count;
    int            *count_used, last_plus_one;
    void           *buf;
{
    register Es_file_data private = ABS_TO_REP(esh);
    register Esfr_mode mode;
    void           *offset;
    es_file_buf     dummy_write_buf;

    /* Ensure that the operation is consistent with the options. */
    if ((private->options & ES_OPT_APPEND) == 0) {
	private->status = ES_INCONSISTENT_POS;

#ifdef DEBUG
	(void) fprintf(stderr, "es_file_replace: read-only stream\n");
#endif

	/* Error */
	return (ES_CANNOT_SET);
    }
    if (private->pos < private->length) {
	if (last_plus_one <= private->length) {
	    mode = esfr_overwrite;
	} else {
	    mode = esfr_truncate;
	    if (count != 0) {
		private->status = ES_INVALID_ARGUMENTS;

#ifdef DEBUG
		(void) fprintf(stderr,
			       "%s: non-zero (%d) count in truncate\n",
			       "es_file_replace", count);
#endif

		/* Error */
		return (ES_CANNOT_SET);
	    }
	    if (private->pos < private->length_on_disk) {
		private->status = ES_INVALID_ARGUMENTS;

#ifdef DEBUG
		(void) fprintf(stderr,
			       "%s: truncate @ %d when length_on_disk %d\n",
			       "es_file_replace", private->pos,
			       private->length_on_disk);
#endif

		/* Error */
		return (ES_CANNOT_SET);
	    }
	}
	if ((private->options & ES_OPT_OVERWRITE) == 0 ||
	    ((mode == esfr_overwrite) &&
	     (count != last_plus_one - private->pos))) {
	    private->status = ES_INVALID_ARGUMENTS;

#ifdef DEBUG
	    (void) fprintf(stderr, "%s last_plus_one is %d, len is %d\n",
			   "es_file_replace position error:",
			   last_plus_one, private->length);
#endif

	    /* Error */
	    return (ES_CANNOT_SET);
	}
    } else {
	mode = esfr_insert;
    }

    /* Do the replace */
    if (mode == esfr_truncate) {
	es_file_maybe_truncate_buf(&private->read_buf, 0);	/* ??? */
	es_file_maybe_truncate_buf(&private->write_buf, 0);	/* ??? */
	*count_used = 0;
    } else if (mode == esfr_overwrite) {
	/*
	 * If new bytes will fit in the write_buf, position the write_buf to
	 * accomodate them (unless there are 4 or fewer bytes) and overwrite.
	 * Otherwise, flush out the write_buf and write the new bytes
	 * directly.
	 */
	if (count <= ES_WRITE_BUF_LEN) {
	    if (count < 5 &&
		(last_plus_one < private->write_buf.start ||
		 private->pos >= BUF_LAST_PLUS_ONE(&private->write_buf))) {
		goto Write_Direct;
	    }
	    if (es_file_move_write_buf(private, private->pos,
				       last_plus_one, &offset) < 0) {
		/* Error */
		return (ES_CANNOT_SET);
	    }
            if (!multibyte)
	        XV_BCOPY(buf, offset, count);
            else
	        XV_BCOPY(buf, offset, count*sizeof(wchar_t));
	    *count_used = count;
	    update_read_buf(private,private->pos,private->pos+count,buf);
	} else {
	    goto Flush_And_Write_Direct;
	}
    } else {			/* mode == es_insert */
	/*
	 * Insert only happens at end-of-stream, thus it always increases
	 * private->write_buf.used by the size of the insertion. If new bytes
	 * will fit in the write_buf, position the write_buf to accomodate
	 * them and add them.  Otherwise, flush out the write_buf and write
	 * the new bytes directly.
	 */
	if (count <= ES_WRITE_BUF_LEN) {
	    if (es_file_move_write_buf(private, private->pos,
				       private->pos + count, &offset) < 0) {
		/* Error */
		return (ES_CANNOT_SET);
	    }
            if (!multibyte)
	        XV_BCOPY(buf, offset, count);
            else
            	XV_BCOPY(buf, offset, count*sizeof(wchar_t));
	    private->write_buf.used += count;
	    update_read_buf(private,private->pos,private->pos+count,buf);
	} else {
    Flush_And_Write_Direct:
	    if (es_file_flush_write_buf(private,
					&private->write_buf) < 0) {
		/* Error */
		return (ES_CANNOT_SET);
	    }
    Write_Direct:
	    /* Fake up a write buffer */
	    dummy_write_buf.used = count;
	    dummy_write_buf.start = private->pos;
	    dummy_write_buf.chars = buf;
	    if (es_file_flush_write_buf(private,
					&dummy_write_buf) <= 0) {
		/* Error */
		return (ES_CANNOT_SET);
	    }
	    /*
	     * Correct overlap with the read_buf that is not masked by
	     * overlap with the write_buf.
	     */
	    if (private->read_buf.used > 0 &&
		private->pos < BUF_LAST_PLUS_ONE(&private->read_buf) &&
		private->read_buf.start < private->pos + count) {
		/*
		 * There is overlap: in future, might be better to update
		 * read_buf, but for now, just discard it.
		 */
		BUF_INVALIDATE(&private->read_buf);
	    }
	}
	*count_used = count;
    }

    private->pos += *count_used;
    if (mode != esfr_overwrite)
	private->length = private->pos;
    return (private->pos);
}

Pkg_private int
es_file_copy_status(esh, to)
    Es_handle       esh;
    char           *to;
{
    Es_file_data    private = ABS_TO_REP(esh);
    int             dummy;
    return (es_copy_status(to, private->fd, &dummy));
}

Pkg_private     Es_handle
es_file_make_backup(esh, backup_pattern, status)
    register Es_handle esh;
    char           *backup_pattern;
    Es_status      *status;
/* Currently backup_pattern must be of the form "%s<suffix>" */
{
    register Es_file_data private;
    char            backup_name[MAXNAMLEN];  
    int             fd, len, retrying = FALSE;
    Es_status       dummy_status;
    Es_handle       result;

    if (status == 0)
	status = &dummy_status;
    if ((esh == NULL) || (esh->ops != &es_file_ops)) {
	*status = ES_INVALID_HANDLE;
	return (NULL);
    }
    *status = ES_CHECK_ERRNO;
    errno = 0;
    private = ABS_TO_REP(esh);
#ifdef BACKUP_AT_HEAD_OF_LINK
    (void) sprintf(backup_name, backup_pattern, private->name);
#else
    (void) sprintf(backup_name, backup_pattern,
		   (private->true_name) ? private->true_name
		   : private->name);
#endif

    fd = (!multibyte) ? private->fd : private->mb_fd;
    len = lseek(fd, 0L, 1);
    if (lseek(fd, 0L, 0) != 0)
	goto Lseek_Failed;
Retry:
    if (es_copy_fd(private->name, backup_name, fd) != 0) {
	if ((!retrying) && (errno == EACCES)) {
	    /*
	     * It may be that the backup_name is already taken by a file that
	     * cannot be overwritten, so try to remove it first.
	     */
	    if (unlink(backup_name) == 0) {
		retrying = TRUE;
		goto Retry;
	    }
	    if (errno == ENOENT)
		/*
		 * backup_name does not exist, so problem with es_copy_fd
		 * really is unfixable access error, which that needs to be
		 * reported to caller, so set errno back!
		 */
		errno = EACCES;
	}
	return (NULL);
    }
    if (lseek(fd, (long) len, 0) != len)
	goto Lseek_Failed;
    result = es_file_create(backup_name, 0, status);
    return (result);

Lseek_Failed:
    *status = ES_SEEK_FAILED;
    return (NULL);
}

static int
es_file_make_wchar_file(esh, open_option)
    register Es_handle 	esh;
    int     		open_option;
{
    register Es_file_data private;
    char            *filename;
    int             fd, new_fd = -1, len, result;
    extern int	    es_mb_to_wc_fd();

    if ((esh == NULL) || (esh->ops != &es_file_ops)) {
	return(-1);
    }
    private = ABS_TO_REP(esh);

    fd = private->mb_fd;
    len = lseek(fd, 0L, 1);
    if (lseek(fd, 0L, 0) != 0)
	return(-1);

    filename = tempnam(NULL,NULL);
    private->skipped = 0;
    result = es_mb_to_wc_fd(private->name, filename, fd, &private->skipped);
    if (!result) {
	if (lseek(fd, (long)len, 0) == len) {
	    new_fd = open(filename, open_option, 0666);
	    private->wc_filename = strdup(filename);
	}
	(void) unlink(filename);
    }
    else if (result == 2)
	private->status = ES_FLUSH_FAILED;

    free(filename);   
    return(new_fd);
}
