/* @(#)base64.c       3.1 - 92/04/03 */

/* b64.c -- provide a RFC1113 Base64 encoding and decoding functions */

#include <stdio.h>
#include <sys/types.h>
#include "buffer.h"
#include "ck_strings.h"

#ifdef	RFC_MIME

#define	MAXLINE	72
#define	_Q	0x80

char	DO_BASE64[] = "base64";
u_char	QB64[256] = {
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 	/* nul - bel */
	_Q,  0,  0, _Q, _Q,  0, _Q, _Q,		/* bs - si */
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 	/* dle - etb */
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 	/* can - us */
	 0,  0,  0,  0,  0,  0,  0,  0,		/* sp - ' */
	 0,  0,  0, 62,  0,  0,  0, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 	/* 0 - 7 */
	60, 61,  0,  0,  0,  0,  0,  0, 	/* 8 - ? */
	 0,  0,  1,  2,  3,  4,  5,  6,		/* @ - G */ 
	 7,  8,  9, 10, 11, 12, 13, 14, 	/* H - O */
	15, 16, 17, 18, 19, 20, 21, 22, 	/* P - W */
	23, 24, 25,  0,  0,  0,  0,  0, 	/* X - _ */
	 0, 26, 27, 28, 29, 30, 31, 32, 	/* ` - g */
	33, 34, 35, 36, 37, 38, 39, 40, 	/* h - o */
	41, 42, 43, 44, 45, 46, 47, 48, 	/* p - w */
	49, 50, 51,  0,  0,  0,  0, _Q, 	/* x - del */
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q,		/* 0200 - 0207 */
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 
	_Q, _Q, _Q, _Q, _Q, _Q, _Q, _Q, 	/* 0370 - 0377 */
};
char	B64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define	ENC(c)	(B64[(c) & 077])
#define	DEC(c)	((int)((QB64[(c) & 0177]) & ~(_Q)))

#define	OPTEOL	0		/* define 1 to allow optional EOL */

b64encode (dd)
register struct dd_obj *dd;
{
	register int n;
	register int c1, c2, c3;
	register int count = MAXLINE;
#if (OPTEOL != 0)
	register int eol = 0;
#else
	char buf[4];
#endif

	while ((c1 = dd_getchar(dd)) >= 0)
	{
#if (OPTEOL != 0)
		eol = 0;
		if (OPTEOL && c1 == '\n')
		{
			dd_methods.dd_put(dd, ',');
			if (--count <= 0)
			{
				dd_methods.dd_put(dd, '\n');
				count = MAXLINE;
			}
			continue;
		}
#endif

		if (((c2 = dd_getchar(dd)) < 0) || (OPTEOL && c2 == '\n'))
		{
#if (OPTEOL != 0)
			if (c2 == '\n')
				eol = 1;
#endif
			c2 = 0;
			c3 = 0;
			n = 1;
		}
		else if (((c3 = dd_getchar(dd)) < 0) || (OPTEOL && c3 == '\n'))
		{
#if (OPTEOL != 0)
			if (c3 == '\n')
				eol = 1;
#endif
			c3 = 0;
			n = 2;
		}
		else
			n = 3;
	
#if (OPTEOL != 0)
		dd_methods.dd_put(dd, ENC(c1 >> 2));
		if (--count <= 0)
		{
			dd_methods.dd_put(dd, '\n');
			count = MAXLINE;
		}
		dd_methods.dd_put(dd, ENC((c1 << 4)|((c2 >> 4) & 017)));
		if (--count <= 0)
		{
			dd_methods.dd_put(dd, '\n');
			count = MAXLINE;
		}
		dd_methods.dd_put(dd,
			((n < 2) ? '=' : ENC((c2 << 2)|((c3 >> 6) & 03))));
		if (--count <= 0)
		{
			dd_methods.dd_put(dd, '\n');
			count = MAXLINE;
		}
		dd_methods.dd_put(dd, ((n < 3) ? '=' : ENC(c3)));
		if (--count <= 0)
		{
			dd_methods.dd_put(dd, '\n');
			count = MAXLINE;
		}

		if (eol)
		{
			dd_methods.dd_put(dd, ',');
			if (--count <= 0)
			{
				dd_methods.dd_put(dd, '\n');
				count = MAXLINE;
			}
		}
#else
		buf[0] = ENC(c1 >> 2);
		buf[1] = ENC((c1 << 4)|((c2 >> 4) & 017));
		buf[2] = ((n < 2) ? '=' : ENC((c2 << 2)|((c3 >> 6) & 03)));
		buf[3] = ((n < 3) ? '=' : ENC(c3));
		dd_methods.dd_puts(dd, buf, 4);

		if ((count -= 4) <= 0)
		{
			dd_methods.dd_put(dd, '\n');
			count = MAXLINE;
		}
#endif
	}

	if (count != MAXLINE)
		dd_methods.dd_put(dd, '\n');
	return (0);
}

b64decode (dd)
register struct dd_obj *dd;
{
	register int c1, c2, c3, c4;

	/* use macro dd_getchar() instead of dd_getc() for performance */
	while ((c1 = dd_getchar(dd)) != EOF)
	{
		if (c1 == '\n')
			continue;
#if (OPTEOL != 0)
		if (c1 == ',')
		{
			dd_methods.dd_put(dd, '\n');
			continue;
		}
#endif
		while ((c2 = dd_getchar(dd)) == '\n')
			;
		if (c2 == EOF)
			break;
		dd_methods.dd_put(dd, DEC(c1) << 2 | DEC(c2) >> 4);

		while ((c3 = dd_getchar(dd)) == '\n')
			;
		if (c3 == EOF)
			break;
		if (c3 != '=')
			dd_methods.dd_put(dd, DEC(c2) << 4 | DEC(c3) >> 2);

		while ((c4 = dd_getchar(dd)) == '\n')
			;
		if (c4 == EOF)
			break;
		if (c4 != '=')
			dd_methods.dd_put(dd, DEC(c3) << 6 | DEC(c4));
	}
	return (0);
}

/*
 * Base64 string encoding which returns a NULL-terminated encoded string.
 * The encoded string does not have any newline.
 */
char *
b64senc (s, len)
register char *s;
register int len;
{
	register int n;
	register char *ns;
	register int c1, c2, c3;
	char *encoded;

	/* base64 is a 3-4 byte encoding: allocate space for encoded string. */
	n = ((len + 2) / 3) * 4;
	encoded = ns = ck_malloc(n + 1);

	while (--len >= 0)
	{
		c1 = *s++;
		if (--len < 0)
		{
			c2 = 0;
			c3 = 0;
			n = 1;
		}
		else
		{
			c2 = *s++;
			if (--len < 0)
			{
				c3 = 0;
				n = 2;
			}
			else
			{
				c3 = *s++;
				n = 3;
			}
		}
	
		*ns++ = ENC(c1 >> 2);
		*ns++ = ENC((c1 << 4)|((c2 >> 4) & 017));
		*ns++ = ((n < 2) ? '=' : ENC((c2 << 2)|((c3 >> 6) & 03)));
		*ns++ = ((n < 3) ? '=' : ENC(c3));
	}

	*ns++ = '\0';
	return (encoded);
}

char *
b64sdec (s, p_len)
register char *s;
register int *p_len;
{
	char *decoded;
	register char *ns;
	register int c1, c2, c3, c4;

	*p_len = ((strlen(s) + 3) / 4) * 3;
	decoded = ns = ck_malloc(*p_len);

	while ((c1 = *s++) != '\0')
	{
		if (c1 == '\n')
			continue;
		while ((c2 = *s++) == '\n')
			;
		if (c2 == '\0')
			break;
		*ns++ = (DEC(c1) << 2 | DEC(c2) >> 4);

		while ((c3 = *s++) == '\n')
			;
		if (c3 == '\0')
			break;
		if (c3 != '=')
			*ns++ = (DEC(c2) << 4 | DEC(c3) >> 2);

		while ((c4 = *s++) == '\n')
			;
		if (c4 == '\0')
			break;
		if (c4 != '=')
			*ns++ = (DEC(c3) << 6 | DEC(c4));
	}

	/* get the actual size of decoded data */
	*p_len = (int) (ns - decoded);

	return (decoded);
}

#endif	RFC_MIME
