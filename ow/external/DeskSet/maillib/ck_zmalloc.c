/* @(#)ck_zmalloc.c	3.9 12/18/92 */

/*
 * ck_zmalloc	- use mmap(2) to allocate memory from /dev/zero.
 * ck_zfree	- use munmap(2) to unmap (free) memory.
 * ck_mmap	- use mmap(2) to map file for read-only.
 * ck_wmmap	- use mmap(2) to map file for read-write.
 * ck_unmap	- identical to ck_zfree().
 *
 * These functions should be better than malloc(3) for large memory allocation.
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "buffer.h"
#include "bool.h"

#define	DEBUG_FLAG mt_debugging
extern DEBUG_FLAG;
#include "debug.h"


static  void *bm_empty = (void *) "";		/* special buffer */
static	struct buffer_map *bm_list;		/* NULL by default */

static struct buffer_map *
insert_bm (buf, size)
char *buf;
size_t size;
{
	struct buffer_map *bm;

	bm = (struct buffer_map *) malloc (sizeof (struct buffer_map));
	bm->bm_buffer = buf;
	bm->bm_size = size;
	bm->bm_next = bm_list;

	bm_list = bm;

	return (bm_list);
}

static size_t
delete_bm (buf)
char *buf;
{
	size_t size;
	register struct buffer_map *p_curr;
	register struct buffer_map *p_prev;

	p_prev = NULL;
	p_curr = bm_list;
	while (p_curr != NULL)
	{
		if (p_curr->bm_buffer == buf)
		{
			if (p_prev == NULL)
				bm_list = p_curr->bm_next;
			else
				p_prev->bm_next = p_curr->bm_next;
			size = p_curr->bm_size;
			free (p_curr);
			return (size);
		}

		p_prev = p_curr;
		p_curr = p_curr->bm_next;
	}
	return (0);
}

void *
ck_zmalloc( size )
size_t size;
{
	static int fd = -1;
	static ino_t inum;
	static dev_t devno;
	struct stat statbuf;
	caddr_t	mbuf;

	/* Special case: never allocate 0 bytes, use a special buffer */
	if (size == 0)
		return (bm_empty);
	else if (size < 0)
	{
		errno = EINVAL;
		perror ("ck_zmalloc: size < 0");
		return (NULL);
	}

	/* Make sure that the cached /dev/zero fd is still valid by
	 * comparing its inode.
	 */
	if ((fd < 0) || (fstat(fd, &statbuf) != 0) || (statbuf.st_ino != inum)
	    || (statbuf.st_dev != devno))
	{
		/* The fd is obsoleted: never opened, closed, or changed */
		if ((fd = open ("/dev/zero", O_RDWR)) < 0)
		{
			perror ("/dev/zero");
			return (NULL);
		}

		if (inum == 0)
		{
			/* Cache the /dev/zero inode information. */
			fstat(fd, &statbuf);
			inum = statbuf.st_ino;
			devno = statbuf.st_dev;
		}
	}
	
	mbuf = mmap (0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (mbuf != (caddr_t) -1)
		(void) insert_bm (mbuf, size);
	else
	{
		mbuf = NULL;
		perror ("ck_zmalloc: mmap");
	}

	DP(("ck_zmalloc(%d) = %#x\n", size, mbuf));

	return ((void *) mbuf);
}

static void *
mmap_file( p_file, p_size, read_only)
char *p_file;
size_t *p_size;
bool read_only;
{
	int	fd;
	caddr_t	buf;
	struct stat sb;

	if ((fd = open (p_file, read_only ? O_RDONLY : O_RDWR)) < 0)
		return ((void *) NULL);
	fstat (fd, &sb);
	if ((*p_size = sb.st_size) == 0)
	{
		close (fd);
		return (bm_empty);
	}

	buf = mmap (0, sb.st_size, read_only ? PROT_READ : PROT_READ|PROT_WRITE,
			MAP_SHARED, fd, 0);
	if (buf != (caddr_t) -1)
		(void) insert_bm (buf, sb.st_size);
	else
	{
		buf = NULL;
		perror ("mmap");
	}

	close (fd);

	DP(("%#x\n", buf));

	return ((void *) buf);
}

/*
 * mmap a file with read-only
 */
void *
ck_mmap( p_file, p_size )
char *p_file;
size_t *p_size;
{
	DP(("ck_mmap = "));
	return (mmap_file (p_file, p_size, TRUE));
}

/*
 * mmap a file of writable 
 */
void *
ck_wmmap( p_file, p_size )
char *p_file;
size_t *p_size;
{
	DP(("ck_wmmap = "));
	return (mmap_file (p_file, p_size, FALSE));
}

void *
ck_zfree( mbuf )
caddr_t mbuf;
{
	size_t size;

	if (mbuf == bm_empty)
		return (NULL);

	if (mbuf != NULL)
	{
		/* Bugid 1070844: don't unmap if size is 0; mbuf is bogus,
		 * but don't know why!
		 */
		if ((size = delete_bm (mbuf)) > 0)
		{
			if (munmap (mbuf, size) < 0)
				perror ("ck_zfree: munmap");

			DP(("ck_zfree = %#x\n", mbuf));

			return (NULL);
		}
	}

	DP(("ck_zfree (not found) = %#x\n", mbuf));
	return (NULL);
}

void *
ck_unmap( buf )
caddr_t buf;
{
	DP(("ck_unmap: "));

	return (ck_zfree (buf));
}
