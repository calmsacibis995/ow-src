/* @(#)buffer.c	3.3 - 92/12/07 */

/* buffer.c
 *
 * Manage a ring buffer of memory chunks.  This package presents
 * an abstraction of a large list linear array of memory, without
 * needing to know the size ahead of time.  It supports adding
 * bytes to the end and reading bytes from the start of a
 * memory chunk.
 *
 * I'm starting with a really simple abstraction: you can only
 * append to the end of the structure (or destroy it); you can
 * read (starting at the front) until the end.
 */


#include <memory.h>
#include <stdlib.h>
#include "../maillib/ck_strings.h"
#include "mail.h"
#include "buffer.h"


struct memhead {
	struct membuff *mh_head;
	struct membuff *mh_tail;
	struct membuff *mh_readptr;
	void (*mh_nomem)();
	int mh_current;
};

struct membuff {
	struct membuff *mb_next;
	long mb_size;
	long mb_current;
	char *mb_base;
};


#define BUFFER_INCREMENT	(1024*64 - 16)




/*
 * allocate a memhead structure (which is opaque to the application).
 *
 * if any of these routines fail (because of no memory), the nomem
 * routine will be called.  nomem should *not* return.
 */
void *
mb_alloc(
	void (*nomem)()
)
{
	struct memhead *mh;

	mh = malloc(sizeof (struct memhead));
	if (! mh) {
		(*nomem)(mh);
		exit(1);
	}

	memset(mh, '\0', sizeof (struct memhead));
	mh->mh_nomem = nomem;


	return (mh);
}


/*
 * free up a memhead structure
 */
void
mb_free(
	void *buf
)
{
	struct memhead *mh = buf;
	struct membuff *mb, *next;

	if (! mh) return;

	for (mb = mh->mh_head; mb; mb = next) {
		next = mb->mb_next;

		ck_zfree(mb->mb_base);
		ck_free(mb);
	}

	ck_free(mh);
}



/*
 * allocate a membuff structure, and add to the mh structure
 */
static struct membuff *
alloc_mb(
	struct memhead *mh
)
{
	struct membuff *mb;

	mb = malloc(sizeof (struct membuff));

	if (! mb) {
		(*mh->mh_nomem)(mh);
		exit(1);
	}
	memset(mb, '\0', sizeof (struct membuff));

	/* splice into the list */
	if (mh->mh_tail) {
		mh->mh_tail->mb_next = mb;
		mh->mh_tail = mb;
	} else {
		mh->mh_head = mb;
		mh->mh_tail = mb;
	}

	/* now allocate the buffer */
	mb->mb_size = BUFFER_INCREMENT;
	mb->mb_base = ck_zmalloc(BUFFER_INCREMENT);
	if (! mb->mb_base) {
		(*mh->mh_nomem)(mh);
		exit(1);
	}

	return (mb);
}



/*
 * append bytes to the membuff structure.  If we run out of space,
 * then call the nonmem routine
 */
void
mb_append(
	void *buf,
	char *ptr,
	int size
)
{
	struct memhead *mh;
	struct membuff *mb;
	long currsize;
	int avail;

	mh = buf;

	mb = mh->mh_tail;
	if (! mb) mb = alloc_mb(mh);

	while (size) {
		/* how many bytes can we add? */
		currsize = mb->mb_size - mb->mb_current;
		if (currsize == 0) {
			mb = alloc_mb(mh);
			continue;
		}

		if (currsize > size) currsize = size;

		/* actually add the bytes */
		memcpy(&mb->mb_base[mb->mb_current], ptr, currsize);

		/* bump the counters... */
		ptr += currsize;
		size -= currsize;
		mb->mb_current += currsize;
	}
}



/*
 * reset a memhead to start reading
 */
void
mb_reset(
	void *buf
)
{
	struct memhead *mh = buf;

	mh->mh_readptr = mh->mh_head;
	mh->mh_current = 0;
}



/*
 * read bytes from a memhead.  return the number of bytes actually put
 * into the buffer.  if "end of file", return zero.
 *
 * A questionable side effect is if you read to an end of file,
 * the buffer is implicitly reset to the start; the next read will
 * start at the beginning of the file.
 */

int
mb_read(
	void *buf,
	char *ptr,
	int size
)
{
	struct memhead *mh = buf;
	struct membuff *mb;
	long actual;
	long incr;

	if (! mh->mh_readptr) {
		/* first time reading -- set the pointers */
		mb_reset(mh);
	}

	mb = mh->mh_readptr;

	actual = 0;
	while (mb && actual < size) {

		incr = mb->mb_size - mh->mh_current;

		/* check for end of buffer */
		if (incr == 0) {
			/* advance to the next membuff */
			mb = mb->mb_next;
			mh->mh_readptr = mb;
			mh->mh_current = 0;
			continue;
		}

		/* don't copy more bytes than were requested */
		if (size - actual < incr) {
			incr = size - actual;
		}

		/* do the copy */
		memcpy(ptr, &mb->mb_base[mh->mh_current], incr);

		/* advance pointers */
		ptr += incr;
		mh->mh_current += incr;
		actual += incr;
	}

	return (actual);
}



/*
 * figure out how many bytes are in the memhead
 */
long
mb_size(
	void *buf
)
{
	struct memhead *mh = buf;
	struct membuff *mb;
	long size;

	for (size = 0, mb = mh->mh_head; mb; mb = mb->mb_next) {
		size += mb->mb_current;
	}

	return (size);
}



