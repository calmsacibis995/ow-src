#ident "@(#)contype.c	3.6 10/16/96 Copyright 1987-1991 Sun Microsystems, Inc."

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

/* Handle top-level RFC-MIME Content-Type header field */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include "headers.h"
#include "ck_strings.h"
#include "bool.h"

#define DEBUG_FLAG mt_debugging
extern int mt_debugging;
#include "debug.h"

static int co_read();
static int co_write();
static int co_copy();
static int co_concat();
static int co_size();
static void co_destroy();
static char *co_get();
static void co_set();
static int co_enumerate();

struct __co_methods co_methods = {
	co_read,
	co_write,
	co_copy,
	co_concat,
	co_size,
	co_destroy,
	co_get,
	co_set,
	co_enumerate,
};

struct coparam **co_last_param;

extern char *skipwhite();
extern char *findchar();

char *
mempbrk(buf, bufend, brkset)
register char *buf;
register char *bufend;
register char *brkset;
{
	register char *p;

	while (buf < bufend)
	{
		for (p=brkset; *p != '\0' && *p != *buf; ++p)
			;
		if (*p != '\0')
			return(buf);
		buf++;
	}
	return(buf);
}

static char *
get_token(buf, bufend, brkset, quoted, token)
register u_char *buf;
register u_char *bufend;
char *brkset;
bool *quoted;
char **token;
{
	register int len;
	register u_char *end;

	buf = (u_char *) skipwhite(buf, bufend);
	if (buf >= bufend) {
		*token = NULL;	/* Fix for 4006442 */
		return((char *)buf);
	}

	if (*buf != '"')
		*quoted = FALSE;
	else
	{
		buf++;
		*quoted = TRUE;
		brkset = "\"";
	}

	end = (u_char *) mempbrk(buf, bufend, brkset);
	if ((len = end - buf) <= 0)
		*token = NULL;
	else
	{
		*token = ck_malloc (len+1);
		strncpy(*token, (char *) buf, len);
		(*token)[len] = '\0';
	}
	
	if (end < bufend)
	{
		if (*end == '"')
			end++;
		if (isspace(*end))
			end = (u_char *) skipwhite(end, bufend);
	}
	return((char *)end);
}

static struct coparam *
create_param(attr, value)
char *attr;
char *value;
{
	struct coparam *p;

	p = (struct coparam *)ck_malloc(sizeof(struct coparam));
	p->co_attr = ck_strdup(attr);
	p->co_value = ck_strdup(value);
	p->co_next = NULL;
	return (p);
}

static void
destroy_param(p)
struct coparam *p;
{
	struct coparam *next;

	if (p != NULL)
	{
		ck_free(p->co_attr);	p->co_attr = NULL;
		ck_free(p->co_value);	p->co_value = NULL;
		next = p->co_next;	p->co_next = NULL;
		ck_free(p);

		/* recusively destroy all parameters */
		destroy_param(next);
	}
}

static char *
get_param(buf, bufend, p)
register char *buf;
register char *bufend;
struct coparam **p;
{
	char	*attr;
	char	*value;
	bool	quoted;
	static char *brkset = "\t =(";
	static char *delmtr = "\t ;(";

	if (*buf == ';')
	{
		buf = get_token(++buf, bufend, brkset, &quoted, &attr);

		if (attr == NULL) {
			return (buf);
		}

		if ((buf >= bufend) || (*buf != '='))
		{
			if (attr) {
				ck_free(attr);
			}
			return (buf);
		}

		buf = get_token(++buf, bufend, delmtr, &quoted, &value);

		if (value != NULL) {
			*p = create_param(attr, value);
		}

		ck_free(attr);
		ck_free(value);

		/* recursively get next parameter */
		if (buf < bufend) {
			buf = get_param(buf, bufend, &((*p)->co_next));
		}
	}

	return (buf);
}

static int
co_read (ct, buf, bufend)
struct cotype *ct;
register char *buf;
register char *bufend;
{
	int len;
	int quoted;
	static char *brkset = "/;\t (";		/* don't change the order */
#ifdef	NEVER
	static char *brkset = "/;\t ()<>@,:\\[]"; /* don't change the order */
#endif	NEVER

	/* Initialize all fields in content-type */
	memset ((char *) ct, 0, sizeof (*ct));

	/* Get the type field which is required field! */
	buf = get_token(buf, bufend, brkset, &quoted, &ct->co_type);
	ct->co_tquoted = quoted;
	if (buf >= bufend)
		return(0);
	if (ct->co_type == NULL)
		return (-1);
	
	/* Get the subtype field */
	if (*buf == '/')
	{
		buf = get_token(++buf, bufend, &brkset[1], &quoted,
				&ct->co_subtype);
		ct->co_squoted = quoted;
		if (buf >= bufend)
			return(0);
	}
	
	/* Get the optional parameters which must a preceding semi-colon. */
	buf = get_param(buf, bufend, &ct->co_parameter);
	if (buf >= bufend)
		return(0);

	/* Get the comment field */
	if (*buf++ == '(')
	{
		/* Exclude the '(' and ')' from the comment field */
		bufend = findchar (')', buf, bufend);
		if ((len = bufend - buf) > 0)
		{
			ct->co_comment = ck_malloc (len+1);
			strncpy (ct->co_comment, buf, len);
			ct->co_comment[len] = '\0';
		}
	}
	return (0);
}

static int
put_token (quoted, token, func, arg)
int quoted;
char *token;
int (*func)();
void *arg;
{
	int	err;
	static char QUOTATION[] = { '"' };

	if (quoted)
	{
		if (err = (*func)(QUOTATION, 1, arg))
			return (err);
	}
	if (err = (*func)(token, strlen(token), arg))
		return (err);
	if (quoted)
	{
		if (err = (*func)(QUOTATION, 1, arg))
			return (err);
	}
	return (0);
}

static int
put_param (p, func, arg)
struct coparam *p;
int (*func)();
void *arg;
{
	if (p == NULL)
		return (0);
	if (p->co_attr && p->co_value)
	{
		int	err;
		char	*buf;

		buf = (char*)malloc(strlen(p->co_attr) + strlen(p->co_value) + 10);
		sprintf (buf, "; %s=\"%s\"", p->co_attr, p->co_value);
		if (err = (*func)(buf, strlen(buf), arg)) {
			free (buf);
			return (err);
		}
		free(buf);

		/* recursively write out the next parameter */
		return (put_param (p->co_next, func, arg));
	}
	return (0);
}

static int
co_write (ct, func, arg)
struct cotype *ct;
int (*func)();
void *arg;
{
	int	err;

	/* The type field is mandatory according RFC-MIME. */
	if (ct->co_type == NULL)
		return (-1);
	if (err = put_token(ct->co_tquoted, ct->co_type, func, arg))
		return (err);

	if (ct->co_subtype)
	{
		if (err = (*func)("/", 1, arg))
			return (err);
		if (err = put_token(ct->co_squoted, ct->co_subtype, func, arg))
			return (err);
	}

	if (ct->co_parameter)
	{
		if (err = put_param(ct->co_parameter, func, arg))
			return (err);
	}

	/* the comment field should not include (), they will be added. */
	if (ct->co_comment)
	{
		/* ZZZ: if the previous token is not a quoted-string, an extra
		 * space character (from " (") is added.  But it does not hurt.
		 */
		if (err = (*func)(" (", 2, arg))
			return (err);
		if (err = (*func)(ct->co_comment, strlen(ct->co_comment), arg))
			return (err);
		if (err = (*func)(")", 1, arg))
			return (err);
	}

	return (err);
}

static struct coparam *
dup_param (p)
struct coparam *p;
{
	struct coparam *np;

	if (p == NULL)
		return (NULL);
	else
	{
		/* recusively duplicate parameters */
		np = create_param(p->co_attr, p->co_value);
		np->co_next = dup_param(p->co_next);
		return (np);
	}

}

static int
co_copy (c1, c2)
struct cotype *c1;
struct cotype *c2;
{
	c1->co_type = ck_strdup (c2->co_type);
	c1->co_subtype = ck_strdup (c2->co_subtype);
	c1->co_parameter = dup_param (c2->co_parameter);
	c1->co_comment = ck_strdup (c2->co_comment);
	c1->co_tquoted = c2->co_tquoted;
	c1->co_squoted = c2->co_squoted;

	return (0);
}

static int
co_concat(src, len, dst)
char *src;
int len;
char *dst;
{
	strncat (dst, src, len);
	return (0);
}

static int
co_size(src, len, total)
char *src;
int len;
int *total;
{
	*total += len;
	return (0);
}

static void
co_destroy (ct)
struct cotype *ct;
{
	ck_free (ct->co_type);		ct->co_type = NULL;
	ck_free (ct->co_subtype);	ct->co_subtype = NULL;
	destroy_param(ct->co_parameter); ct->co_parameter = NULL;
	ck_free (ct->co_comment);	ct->co_comment = NULL;
}

static
co_enumerate (ct, func, arg)
struct cotype *ct;
int (*func)();
void *arg;
{
	register int	err;
	register struct coparam *p;

	p = ct->co_parameter;
	while (p != NULL)
	{
		err = (*func)(p->co_attr, p->co_value, arg);
		if (err != 0)
			return (err);

		p = p->co_next;
	}
	return (0);
}

static struct coparam *
find_param (ct, attr)
struct cotype *ct;
char *attr;
{
	register struct coparam *p;

	/* remember the last parameter */
	co_last_param = &ct->co_parameter;
	p = ct->co_parameter;
	while (p != NULL)
	{
		if (strcasecmp(p->co_attr, attr) == 0)
			return (p);
		
		/* remember the last parameter */
		co_last_param = &(p->co_next);

		p = p->co_next;
	}
	return (NULL);
}

/* Get a value of an attribute in the content-type parameter.
 */
static char *
co_get (ct, attr)
struct cotype *ct;
char *attr;
{
	struct coparam *param;

	param = find_param(ct, attr);
	if (param == NULL)
		return (NULL);
	return (ck_strdup(param->co_value));
}

/*
 * This function appends the attribute-value pair to the content type
 * haeader as parameter.  The value is always quoted.
 */
static void
co_set (ct, attr, value)
struct cotype *ct;
char *attr;
char *value;
{
	struct coparam *param;

	if (value == NULL || *value == '\0')
		return;

	param = find_param(ct, attr);
	if (param == NULL)
	{
		/* append to the end */
		param = create_param(attr, value);
		*co_last_param = param;
	}
	else
	{
		/* overwrite the duplicate */
		ck_free(param->co_value);
		param->co_value = ck_strdup(value);
	}
}
