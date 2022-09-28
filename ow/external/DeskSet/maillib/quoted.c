#ident "@(#)quoted.c	3.2 05/19/93 Copyright 1987-1991 Sun Microsystems, Inc."

/*
 *  Copyright (c) 1987-1991 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */ 

/* qprintable.c -- provide an RFC-MIME Quoted Printable encoding and
 * decoding functions
 */

#include <stdio.h>
#include <ctype.h>
#include "sys/types.h"
#include "buffer.h"
#include "ck_strings.h"
#include "bool.h"

#ifdef	RFC_MIME

extern u_char QB64[256];
char DO_QUOTED_PRINTABLE[] = "quoted-printable";
static char Hex[] = "0123456789ABCDEF";

#define	MAXLINE	76
#define	_Q	0x80

#define	isquotable(c)	(QB64[c] & _Q)
#define	QP_ESC		'='		/* escape char for quoted printable */
#define	QPS_ESC		"=="		/* replace the escape char by these */

/*
 * Quoted printable encoding.
 */
qpencode (dd)
struct dd_obj *dd;
{
	int lastc;
	register int c;
	register int state = 7;
	register int count = 0;
	register struct buffer_map *bm;

	bm = &dd->dd_curr;
	for (;;)
	{
		switch (state)
		{
		case 0:
			/* state 0: read a character */
			if ((c = bm_getc(bm)) < 0)
				return (0);
			state = 1;
			break;
		case 1:
			/* default next state is to get next character */
			state = 0;

			if (c == QP_ESC)
			{
				count = line_break(dd, count, 3);
				count += 2;
				dd_methods.dd_puts(dd, QPS_ESC, 2);
			}
			else if (c == '\n')
			{
				dd_methods.dd_put(dd, c);
				count = 0;
				state = 7;
			}
			else if ((c == ' ') || (c == '\t'))
			{
				lastc = c;
				/* look ahead for LF */
				if ((c = bm_getc(bm)) < 0)
					return (0);
				state = 2;
			}
			else if (isquotable(c))
			{
				count = line_break(dd, count, 4);
				count += quote_it(dd, c);
			}
			else
			{
				count = line_break(dd, count, 2);
				count++;
				dd_methods.dd_put(dd, c);
			}
			break;
		case 2:
			/* from state 1: previous char was a space or tab */
			if (c == '\n')
			{
				/* it is a trailing blank or tab, quote it */
				count = line_break(dd, count, 4);
				count += quote_it(dd, lastc);
			}
			else
			{
				/* not a trailing blank or tab, output as is */
				count = line_break(dd, count, 2);
				count++;
				dd_methods.dd_put(dd, lastc);
			}
			state = 1;
			break;
		case 3:
			/* look ahead state for 'r' in "From " string */
			if ((c = bm_getc(bm)) != 'r')
			{
				count += flushfrom(dd, 1, FALSE);
			}
			if (c == 'r')
				state = 4;
			else
				state = 1;
			break;
		case 4:
			/* look ahead state for 'o' in "From " string */
			if ((c = bm_getc(bm)) != 'o')
			{
				count += flushfrom(dd, 2, FALSE);
			}
			if (c == 'o')
				state = 5;
			else
				state = 1;
			break;
		case 5:
			/* look ahead state for 'm' in "From " string */
			if ((c = bm_getc(bm)) != 'm')
			{
				count += flushfrom(dd, 3, FALSE);
			}
			if (c == 'm')
				state = 6;
			else
				state = 1;
			break;
		case 6:
			/* look ahead state for ' ' in "From " string */
			c = bm_getc(bm);
			count += flushfrom(dd, 4, (c == ' '));
			if (c < 0)
				return (0);
			state = 1;
			break;
		case 7:
			/* it is at the beginning of a line */
			if ((c = bm_getc(bm)) < 0)
				return (0);

			/* look ahead if it is "From " string; Sun specific. */
			if (c == 'F')
				state = 3;
			else
				state = 1;
			break;
		}
	}
}

/*
 * Flush "nchars" (1-4) characters of the "From" string with optional quoting
 * the 'F' character.
 */
static
flushfrom (dd, nchars, quotef)
struct dd_obj *dd;
int nchars;
bool quotef;
{
	int count = nchars;

	if (--nchars >= 0)
	{
		if (quotef)
			count += quote_it(dd, 'F');
		else
			dd_methods.dd_put(dd, 'F');
	}
	if (--nchars >= 0)
		dd_methods.dd_put(dd, 'r');
	if (--nchars >= 0)
		dd_methods.dd_put(dd, 'o');
	if (--nchars >= 0)
		dd_methods.dd_put(dd, 'm');
	return (count);
}

/*
 * If it needs soft line break, write out the soft line break.
 */
static
line_break (dd, count, pads)
struct dd_obj *dd;
int	count;
int	pads;
{
	if ((count + pads) > MAXLINE)
	{
		dd_methods.dd_puts(dd, "=\n", 2);
		return (0);
	}
	return (count);
		
}

/*
 * Quote a single character.
 */
static
quote_it(dd, c)
struct dd_obj *dd;
int c;
{
	char buf[3];

	buf[0] = QP_ESC;
	buf[1] = Hex[(c >> 4) & 0xf];
	buf[2] = Hex[(c & 0xf)];
	dd_methods.dd_puts(dd, buf, 3);

	return (3);
}

/*
 * Quoted printable decoding.
 */
qpdecode (dd)
register struct dd_obj *dd;
{
	char hex[3];
	register unsigned int c;

	/* use macro dd_getchar() instead of dd_getc() for performance */
	hex[2] = 0;
	while ((c = dd_getchar(dd)) != EOF)
	{
		if (c == QP_ESC)
		{
			if ((c = dd_getchar(dd)) == EOF)
				break;

			/* soft line break */
			if (c == '\n')
				continue;

			/* map the hex value to a byte */
			if (isxdigit(c))
			{
				hex[0] = (char) c;
				if ((hex[1] = dd_getchar(dd)) == EOF)
					break;
				c = (int) strtol(hex, NULL, 16);
			}
			else if (c != QP_ESC)
			{
				/* invalid situation, output liternally */
				dd_methods.dd_put(dd, QP_ESC);
			}
		}
		dd_methods.dd_put(dd, c);
	}
	return (0);
}

/*
 * Quoted-Printable string encoding which returns a NULL-terminated encoded
 * string .  The encoded string does not have any newline and space character
 * is encoded.
 */
char *
qpsenc (s, len)
register char *s;
register int len;
{
	char	*encoded;
	register char *ns;
	register unsigned char c; 

	ns = encoded = ck_malloc((len * 3) + 1);
	while (--len >= 0)
	{
		c = *s++;
		if (c == QP_ESC)
		{
			*ns++ = QP_ESC;
			*ns++ = QP_ESC;
		}
		else if (isquotable(c) || (c == ' '))
		{
			*ns++ = QP_ESC;
			*ns++ = Hex[(c >> 4) & 0xf];
			*ns++ = Hex[(c & 0xf)];
		}
		else
		{
			*ns++ = c;
		}
	}
	*ns++ = '\0';
	ns = ck_strdup(encoded);
	ck_free(encoded);
	return (ns);
}

char *
qpsdec (s, p_len)
register char *s;
int	*p_len;
{
	char hex[3];
	char *decoded;
	register int len;
	register int l = 0;
	register char *ns;
	register unsigned char c;

	hex[2] = '\0';
	len = strlen(s);
	ns = decoded = ck_strdup(s);
	while (--len >= 0)
	{
		if ((c = *s++) == QP_ESC)
		{
			if (--len < 0) 
				break;
			if ((c = *s++) == '\n')
				continue;
			if (isxdigit(c))
			{
				hex[0] = (char) c;
				if (--len < 0)
					break;
				hex[1] = *s++;
				c = (unsigned int) strtol(hex, NULL, 16);
			}
			else if (c != QP_ESC)
			{
				/* invalid situation, output liternally */
				*ns++ = QP_ESC;
				l++;
			}
		}
		*ns++ = c;
		l++;
	}
	*ns++ = '\0';
	ns = ck_strdup(decoded);
	ck_free(decoded);
	*p_len = l;
	return (ns);
}

#endif	RFC_MIME
