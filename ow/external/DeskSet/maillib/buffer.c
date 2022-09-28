#ident "@(#)buffer.c	3.5 06/28/95 Copyright 1987-1991 Sun Microsystems, Inc."

/* buffer.c -- handle fragmented buffer block for runtime compression and
 * encoding.
 */

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef	SVR4
#include <ulimit.h>
#else
#ifndef	UL_GDESLIM
#define	UL_GDESLIM	4		/* Compatible with SVR4 */
#endif
#endif	SVR4

#include "buffer.h"
#include "ck_strings.h"

#define DEBUG_FLAG mt_debugging
extern int mt_debugging;
#include "debug.h"

static struct dd_obj	*dd_init();
static int		dd_read();
static int		dd_write();
static int		dd_loop();
static int		dd_getc();
static struct buffer_map *dd_put();
static struct buffer_map *dd_puts();
static struct buffer_map *dd_done();
static struct buffer_map *dd_alloc();
static struct buffer_map *dd_conv();
static void		dd_delete();

struct buffer_map	*bm_alloc();
struct buffer_map	*bm_last();
void			bm_free();
void			bm_init();
int			bm_gets();
int			bm_getc();
int			bm_copy();
int			bm_write();
int			bm_size();
static int		dpopen();

struct __dd_methods dd_methods = {
	dd_init,
	dd_read,
	dd_write,
	dd_loop,
	dd_done,
	dd_put,
	dd_puts,
	dd_getc,
	dd_conv,
};

#define	SSIZE	(8 * BUFSIZ)		/* small mmap block size */
#define	BSIZE	(16 * BUFSIZ)		/* big mmap block size */

typedef	int	bool;

static struct buffer_map *
dd_conv (progfunc, isfunc, bmap, argv)
void	*progfunc;
bool	isfunc;
struct buffer_map *bmap;
char	**argv;
{
	struct dd_obj *dd;
	struct buffer_map *bm;

	if (bmap == NULL)
		return (NULL);

	if (isfunc)
	{
		dd = dd_init (NULL, bmap);
		if (dd == NULL)
			return (NULL);
		(*((void (*)()) progfunc)) (dd, argv);
		bm = dd_done (dd);
	}
	else
	{
		dd = dd_init ((char *) progfunc, bmap);
		if (dd == NULL)
			return (NULL);
		dd_loop (dd);
		bm = dd_done (dd);
	}

	return (bm);
}

static struct dd_obj *
dd_init (command, bm)
char *command;
struct buffer_map *bm;
{
	int fd[2];
	int pid = -1;
	struct dd_obj *dd;

	if ((command != NULL) && ((pid = dpopen (command, fd)) < 0))
		return (NULL);

	dd = (struct dd_obj *) ck_malloc (sizeof (struct dd_obj));
	dd->dd_pid = pid;
	dd->dd_fd[0] = command ? fd[0] : -1;
	dd->dd_fd[1] = command ? fd[1] : -1;
	dd->dd_size = bm->bm_size < SSIZE ? SSIZE : BSIZE;
	dd->dd_curr.bm_next = bm;
	dd->dd_curr.bm_buffer = bm->bm_buffer;
	dd->dd_curr.bm_size = 0;
	dd->dd_head = NULL;
	dd->dd_tail = NULL;

	return (dd);
}

static struct buffer_map *
dd_done (dd)
struct dd_obj *dd;
{
	int status;
	struct buffer_map *bm;

	if (dd->dd_fd[0] >= 0)
		close (dd->dd_fd[0]);
	if (dd->dd_fd[1] >= 0)
		close (dd->dd_fd[1]);

	if (dd->dd_pid > 0)
	{
		/* Clean up the process */
		while (waitpid(dd->dd_pid, &status, 0) >= 0)
			;
	}

	/* return the allocated buffer list to user */
	bm = dd->dd_head;

	dd->dd_tail = NULL;
	dd->dd_head = NULL;
	ck_free (dd);

	return (bm);
}

static int
dd_read (dd)
struct dd_obj *dd;
{
	int	n;
	register struct buffer_map *bm = dd->dd_tail;

	if ((bm == NULL) || ((n = (dd->dd_size - bm->bm_size)) <= 0))
	{
		/* allocate a new buffer */
		bm = dd_alloc (dd);
		n = dd->dd_size;
	}

	while (1)
	{
		n = read (dd->dd_fd[0], bm->bm_buffer + bm->bm_size, n);
		if (n > 0)
		{
			bm->bm_size += n;
			break;
		}
		else if ((n == 0) || ((n < 0) && (errno != EINTR)))
		{
			/* the last buffer was never used, free it */
			if (bm->bm_size == 0)
				dd_delete (dd, bm);
			return (1);
		}

		/* ignore EINTR */
	}
	return (0);
}

static int
dd_write (dd)
struct dd_obj *dd;
{
	register int	n;
	register struct buffer_map *bm;

	bm = dd->dd_curr.bm_next;
	n = bm->bm_size - dd->dd_curr.bm_size;

	/* Current node is exhausted, get next buffer */
	if (n <= 0)
	{
		bm = bm->bm_next;
		/* no more data to be written out */
		if (bm == NULL)
			return (1);
		dd->dd_curr.bm_buffer = bm->bm_buffer;
		dd->dd_curr.bm_size = 0;
		dd->dd_curr.bm_next = bm;
		n = bm->bm_size;
	}

	if (n > BUFSIZ)
		n = BUFSIZ;
	else if (n == 0)
		return (1);

	n = write (dd->dd_fd[1], dd->dd_curr.bm_buffer, n);
	if (n > 0)
	{
		dd->dd_curr.bm_size += n;
		dd->dd_curr.bm_buffer += n;
	}
	else if ((n < 0) && (errno != EINTR) &&
		 (errno != EAGAIN) && (errno != EWOULDBLOCK))
	{
		return (1);
	}

	/* if n == 0, we will try again later */

	return (0);
}

#ifdef	DEBUG
static
dd_copy (dd, func, param)
struct dd_obj *dd;
int (*func)();
void *param;
{
	return (bm_write (dd->dd_head, func, param));
}
#endif	DEBUG

static struct buffer_map *
dd_alloc (dd)
struct dd_obj *dd;
{
	struct buffer_map *bm;
	char *buf;

	buf = (char *) ck_zmalloc (dd->dd_size);
	if (buf == NULL)
		return (NULL);

	bm = bm_alloc (buf, 0);
	if (bm == NULL)
	{
		ck_zfree (buf);
		return (NULL);
	}

	if (dd->dd_head == NULL)
		dd->dd_head = bm;
	else
		dd->dd_tail->bm_next = bm;
	dd->dd_tail = bm;

	return (bm);
}

static void
dd_delete (dd, bm)
struct dd_obj *dd;
struct buffer_map *bm;
{
	register struct buffer_map *p_prev;
	register struct buffer_map *p_curr;
	register struct buffer_map *p_next;

	p_prev = NULL;
	p_curr = dd->dd_head;
	while (p_curr != NULL)
	{
		p_next = p_curr->bm_next;
		if ((bm != NULL) && (p_curr != bm))
			p_prev = p_curr;
		else
		{
			/* found the buffer from linked list, remove it */
			if (p_prev == NULL)
				dd->dd_head = p_next;
			else
				p_prev->bm_next = p_next;

			/* update the last buffer ptr */
			if (dd->dd_tail == p_curr)
				dd->dd_tail = p_next ? p_next : p_prev;

			/* free all allocated buffers */
			ck_zfree (p_curr->bm_buffer);
			ck_free (p_curr);

			/* delete one mmap block only */
			if (bm != NULL)
				return;
		}
		p_curr = p_next;
	}
	dd->dd_head = NULL;
	dd->dd_tail = NULL;
}


static struct buffer_map *
dd_put (dd, c)
struct dd_obj *dd;
char c;
{
	register struct buffer_map *bm = dd->dd_tail;

	if ((bm == NULL) || (bm->bm_size >= dd->dd_size))
	{
		bm = dd_alloc (dd);
		if (bm == NULL)
			return (NULL);
	}
	bm->bm_buffer[bm->bm_size] = c;
	bm->bm_size++;
	return (bm);
}

static struct buffer_map *
dd_puts (dd, s, n)
register struct dd_obj *dd;
register char *s;
register int n;
{
	register struct buffer_map *bm = dd->dd_tail;

	while (--n >= 0)
	{
		if ((bm == NULL) || (bm->bm_size >= dd->dd_size))
		{
			bm = dd_alloc (dd);
			if (bm == NULL)
				return (NULL);
		}
		bm->bm_buffer[bm->bm_size] = *s++;
		bm->bm_size++;
	}
	return (bm);
}

static int
dd_getc (dd)
struct dd_obj *dd;
{
	return (bm_getc (&dd->dd_curr));
}

static int
dd_loop (dd)
struct dd_obj *dd;
{
	fd_set fd_in;
	fd_set fd_out;
	register int n;
	register int in, out;
	register long width;
	register fd_set *p_fd_in;
	register fd_set *p_fd_out;

	if (dd == NULL)
		return (-1);

	p_fd_in = &fd_in;
	p_fd_out = &fd_out;

	FD_ZERO (p_fd_in);
	FD_ZERO (p_fd_out);

	in = dd->dd_fd[0];
	out = dd->dd_fd[1];
	FD_SET (in, p_fd_in);
	FD_SET (out, p_fd_out);

	width = ulimit(UL_GDESLIM);
	while ((n = select (width, p_fd_in, p_fd_out, NULL, 0)) != 0)
	{
		if (n < 0)
		{
			if (errno != EINTR)
				break;
			if (p_fd_in)
				FD_SET (in, p_fd_in);
			if (p_fd_out)
				FD_SET (out, p_fd_out);
			continue;
		}

		if (p_fd_out && FD_ISSET (out, p_fd_out))
		{
			if (dd_write (dd))
			{
				p_fd_out = NULL;
				close (out);
				dd->dd_fd[1] = -1;
			}
		}

		if (p_fd_in && FD_ISSET (in, p_fd_in))
		{
			if (dd_read (dd))
			{
				p_fd_in = NULL;
				close (in);
				dd->dd_fd[0] = -1;
			}
		}

		if (p_fd_in)
			FD_SET (in, p_fd_in);

		if (p_fd_out)
			FD_SET (out, p_fd_out);

		if ((p_fd_in == NULL) && (p_fd_out == NULL))
			return (0);
	}
	return (-1);
}

#ifdef	DEBUG
void
bm_dump (bm)
struct buffer_map *bm;
{
	while (bm != NULL)
	{
		DP(("bm=%x, buffer=%x, used=%d, next=%x\n",
			bm, bm->bm_buffer, bm->bm_size, bm->bm_next));
		write (1, bm->bm_buffer, bm->bm_size);
		bm = bm->bm_next;
	}
}
#endif	DEBUG

struct buffer_map *
bm_alloc (buf, size)
char *buf;
int size;
{
	register struct buffer_map *bm;

	bm = (struct buffer_map *) ck_malloc (sizeof (struct buffer_map));
	bm->bm_size = size;
	bm->bm_buffer = buf;
	bm->bm_next = NULL;

	return (bm);
}

struct buffer_map *
bm_last (bm)
struct buffer_map *bm;
{
	if (bm != NULL)
	{
		while (bm->bm_next != NULL)
			bm = bm->bm_next;
	}
	return (bm);
}

void
bm_free (bm, func)
struct buffer_map *bm;
void (*func)();
{
	struct buffer_map *next;

	while (bm != NULL)
	{
		next = bm->bm_next;

		if (func != NULL)
			(*func) (bm->bm_buffer);

		bm->bm_size = 0;
		bm->bm_buffer = NULL;
		bm->bm_next = NULL;
		ck_free (bm);

		bm = next;
	}
}


unsigned long
bm_enumerate(
	struct buffer_map *bm,
	BM_FUNC func,
	...
)
{
	va_list ap;
	register unsigned long ecode;
	struct buffer_map *next;

	while (bm != NULL)
	{
		next = bm->bm_next;
		va_start(ap, func);
		ecode = (*func)(bm->bm_buffer, bm->bm_size, ap);
		va_end(ap);
		if (ecode != 0) {
			return (ecode);
		}
		bm = next;
	}

	return (0);
}

bm_write (bm, func, param)
struct buffer_map *bm;
int (*func)();
int param;
{
	register int ecode;

	while (bm != NULL)
	{
		ecode = (*func) (bm->bm_buffer, bm->bm_size, param);
		if (ecode != 0)
			return (ecode);
		bm = bm->bm_next;
	}
	return (0);
}

int
bm_copy (buf, bm)
char *buf;
struct buffer_map *bm;
{
	register int size = 0;

	while (bm != NULL)
	{
		memcpy (buf + size, bm->bm_buffer, bm->bm_size);
		size += bm->bm_size;
		bm = bm->bm_next;
	}
	return (size);
}

int
bm_size (bm)
struct buffer_map *bm;
{
	register int size = 0;

	while (bm != NULL)
	{
		size += bm->bm_size;
		bm = bm->bm_next;
	}
	return (size);
}

void
bm_init (bm, ptr)
struct buffer_map *bm;
struct buffer_map *ptr;
{
	ptr->bm_buffer = bm->bm_buffer;
	ptr->bm_size = 0;
	ptr->bm_next = bm;
}

int
bm_gets (buf, n, ptr)
char *buf;
int n;
struct buffer_map *ptr;
{
	register int len;
	register int size = 0;
	register struct buffer_map *bm;

	/* Get current buffer */
	bm = ptr->bm_next;

	while (n > 0)
	{
		/* Current node is exhausted, get next buffer */
		if (ptr->bm_size >= bm->bm_size)
		{
			bm = bm->bm_next;
			if (bm == NULL)
				return (size);
			ptr->bm_buffer = bm->bm_buffer;
			ptr->bm_size = 0;
			ptr->bm_next = bm;
		}

		len = bm->bm_size - ptr->bm_size;
		if (n < len)
			len = n;
		memcpy (buf, &(ptr->bm_buffer[ptr->bm_size]), len);
		ptr->bm_size += len;
		size += len;
		buf += len;
		n -= len;
	}

	/* Return the actual size */
	return (size);
}

int
bm_getc (ptr)
struct buffer_map *ptr;
{
	struct buffer_map *bm;

	/* Get current buffer */
	bm = ptr->bm_next;

	/* Current node is exhausted, get next buffer */
	if (ptr->bm_size >= bm->bm_size)
	{
		bm = bm->bm_next;
		if (bm == NULL)
			return (EOF);
		ptr->bm_buffer = bm->bm_buffer;
		ptr->bm_size = 0;
		ptr->bm_next = bm;
	}

	/* Return the character */
	return ((u_char) ptr->bm_buffer[ptr->bm_size++]);
}

static int
dpopen (command, fd)
char *command;
int fd[2];
{
	int pid;
	int ifd[2];
	int ofd[2];

	if (pipe (ifd) < 0)
		return (-1);
	if (pipe (ofd) < 0)
		return (-1);

	if ((pid = vfork ()) == 0)
	{
		if (dup2 (ifd[0], 0) < 0)
			_exit (1);
		if (dup2 (ofd[1], 1) < 0)
			_exit (1);

		close (ifd[0]);
		close (ifd[1]);
		close (ofd[0]);
		close (ofd[1]);

		(void) execl("/bin/sh", "sh", "-c", command, (char *)0);
		perror (command);
		_exit (1);
	}
	else
	{
		close (ifd[0]);
		close (ofd[1]);

		if (pid < 0)
		{
			close (ifd[1]);
			close (ofd[0]);
			return (-1);
		}
		else
		{
			fd[0] = ofd[0];
			fd[1] = ifd[1];
			/* the output pipe is non-blocking I/O */
#ifdef	SVR4
			fcntl (fd[1], F_SETFL, O_NONBLOCK);
#else
			fcntl (fd[1], F_SETFL, FNBIO);
#endif	SVR4
			return (pid);
		}
	}
}
