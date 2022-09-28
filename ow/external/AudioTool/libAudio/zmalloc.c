/* Copyright (c) 1991 by Sun Microsystems, Inc. */
#ident	"@(#)zmalloc.c	1.2	96/02/20 SMI"

/*
 * zmalloc	- use mmap(2) to allocate memory from /dev/zero.
 * zfree	- use munmap(2) to unmap (free) memory.
 *
 * These functions should be better than malloc(3) for large memory allocation.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "zmalloc.h"

/*
 * a utility structure to keep track of the (possibly) multiple mmaps
 * that we have done...
 */
struct buffer_map {
	struct buffer_map *bm_next;
	char *bm_buffer;
	int bm_size;
};

static  void *bm_empty = (void *) "";		/* special buffer */
static	struct buffer_map *bm_list;		/* NULL by default */

static struct buffer_map *
insert_bm (char *buf, size_t size)
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
delete_bm (char *buf)
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
zmalloc( size_t size )
{
	int	fd;
	caddr_t	mbuf;

	/* XXX - Special case: never allocate 0 bytes, use a special buffer */
	if (size == 0)
	    return ((void *)NULL); /* return (bm_empty); */

	if ((fd = open ("/dev/zero", O_RDWR)) < 0) {
		perror ("/dev/zero");
		return ((void *) NULL);
	}
	
	mbuf = mmap (0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	close (fd);

	if (mbuf == (caddr_t) -1) {
		perror ("zmalloc: mmap");
		return ((void *) NULL);
	}

	(void) insert_bm (mbuf, size);

	/* DP(("zmalloc(%d) = %#x\n", size, mbuf)); */

	return ((void *) mbuf);
}

void
zfree(void* mbuf)
{
	size_t size;

	if (mbuf == bm_empty)
	    return;

	if (mbuf != NULL) {
		if (size = delete_bm ((caddr_t)mbuf)) {
			if (munmap ((char *)mbuf, size) < 0)
			    perror ("zfree: munmap");
			/* DP(("zfree = %#x\n", mbuf)); */
		}
	}

}
