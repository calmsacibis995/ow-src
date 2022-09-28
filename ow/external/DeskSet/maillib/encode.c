/* @(#)encode.c       3.1 - 92/04/03 */

/* encode.c -- provide encoding and decoding functions so we don't
 * have to call uuencode and uudecode programs
 */

#include <stdio.h>
#include <sys/types.h>
#include "buffer.h"

char DO_UUENCODE[] = "uuencode";

#define	ENC(c)	(((c) & 077) + ' ')
#define DEC(c)	(((c) - ' ') & 077)

#define	MAXLINE	45

static void
outenc (p, dd, n)
char *p;
struct dd_obj *dd;
int n;
{
	char buf[4];

	dd_methods.dd_put(dd, ENC(n));
	while (n > 0)
	{
		buf[0] = ENC(*p >> 2);
		buf[1] = ENC((*p << 4)|(p[1] >> 4) & 017);
		buf[2] = ENC((p[1] << 2)|(p[2] >> 6) & 03);
		buf[3] = ENC(p[2]);
		dd_methods.dd_puts(dd, buf, 4);
		p += 3;
		n -= 3;
	}
	dd_methods.dd_put (dd, '\n');
}

int
uuencode (dd, argv)
struct dd_obj *dd;
char *argv[];
{
	register int size;
	char buffer[MAXLINE];
	struct buffer_map *bm;

	sprintf (buffer, "begin %o %s\n", argv[0], argv[1]);
	size = strlen (buffer);
	dd_methods.dd_puts(dd, buffer, size);

	/* initialize the read buffer */
	memset (buffer, 0, sizeof(buffer));

	bm = &dd->dd_curr;
	while ((size = bm_gets (buffer, MAXLINE, bm)) > 0)
		outenc (buffer, dd, size);

	dd_methods.dd_puts(dd, " \nend\n", 6);

	return (0);
}

static int
outdec(p, dd, n)
char *p;
struct dd_obj *dd;
int n;
{
	register int i, c1, c2, c3, c4;

	/* compensate that some mailers truncate trailing white spaces */
	i = 0;
	if ((c1 = *p) != '\n')
		i++;
	else
		c1 = ' ';
	if ((c2 = p[i]) != '\n')
		i++;
	else
		c2 = ' ';
	if ((c3 = p[i]) != '\n')
		i++;
	else
		c3 = ' ';
	if ((c4 = p[i]) != '\n')
		i++;
	else
		c4 = ' ';

	if (n >= 1)
		dd_methods.dd_put(dd, DEC(c1) << 2 | DEC(c2) >> 4);
	if (n >= 2)
		dd_methods.dd_put(dd, DEC(c2) << 4 | DEC(c3) >> 2);
	if (n >= 3)
		dd_methods.dd_put(dd, DEC(c3) << 6 | DEC(c4));
	return (i);
}

char *
skip_begin(buf, bufend, mode, name)
char *buf;
char *bufend;
long *mode;
char *name;
{
	int		n;
	char		dest[256];
	register char	*bp;

	bp = buf;
	while (buf < bufend)
	{
		if (*buf++ == '\n')
		{
			if (strncmp (bp, "begin ", 6) == 0)
				break;
			bp = buf;
		}
	}
	if (buf >= bufend)
		return (NULL);

	n = buf - bp - 1;
	dest[n] = '\0';
	strncpy (dest, bp, n);
	(void) sscanf(dest, "begin %lo %s", mode, name);
	return (buf);
}

int
uudecode (dd)
struct dd_obj *dd;
{
	long mode;
	char name[128];
	register int n;
	register char *buf;
	register char *bufend;
	struct buffer_map *bm;

	/* ZZZ: Assume that the data is in contiguous memory.  If this
	 * assumption is not suitable, dd_getc() or dd_getchar() must be used.
	 */
	bm = dd->dd_curr.bm_next;
	buf = bm->bm_buffer;
	bufend = buf + bm->bm_size;

	buf = skip_begin(buf, bufend, &mode, name);
	if (buf == NULL)
		return(-1);

	while (buf < bufend)
	{
		n = DEC(*buf);
		buf++;
		if (n <= 0)
		{
			buf++;
			break;
		}
		while (n > 0)
		{
			buf += outdec (buf, dd, n);
			if (buf >= bufend)
				break;
			n -= 3;
		}
		/* skip the newline */
		while (*buf++ != '\n')
			;
	}
	if ((bufend - buf) >= 4)
	{
		if (strncmp (buf, "end\n", 4) == 0)
			return (0);
	}

	return (-1);
}
