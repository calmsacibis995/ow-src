#ifndef lint
static char sccsid[] = "@(#)fns_lookup.c	3.1 03 Apr 1992 Copyright 1989 Sun Microsystems, Inc.";
#endif lint
/* fns_lookup.c -- the lookup routine for the file name space */

#include <stdio.h>
#include <varargs.h>
#include <string.h>
#include <memory.h>

#include "fns.h"
#include "ce.h"

/* void *ce_get_attribute(); */

#ifdef NOTYET
/* ce_get_attribute_type does not yet seem to exist */
void *ce_get_attribute_type();
#else
#define ce_get_attribute_type(a,b,c)	("no-type")
#endif

#define STREQ(a,b)	(!strcmp((a),(b)))


/*
 * initialize the library
 */
int
init_mgr(num_func, func_ptrs)
int *num_func;
void ***func_ptrs;
{
	void **funcs;
	int build();
	int match();
	int get_entry_cookie();
	int match_entry_cookie();

	*num_func = 4;
	funcs = (void **) calloc(4, sizeof (int (*)()));

	if (!funcs) return(0);

	funcs[0] = (void *)build;
	funcs[1] = (void *)match;
	funcs[2] = (void *)get_entry_cookie;
	funcs[3] = (void *)match_entry_cookie;

	*func_ptrs = funcs;

	return (1);
}


/*
 * the basic file name space lookup routine.  abstracted from
 * the file manager's fv.bind.c file.
 *
 * 0 for match, 1 for failure
 *
 * The arguments:
 *	0: file name
 *	1: contents
 *	2: content length
 *	3: contents routine callback (not currently used)
 *
 * The literals:
 *	0: file name pattern (regexp format)
 *	1: magic offset
 *	2: magic type {byte, short, long, string}
 *	3: magic mask
 *	4: magic match
 *	5: magic operation (=, <, >, &, ^)
 *	5: dir_name pattern (not currently used)
 *
 * If there is both a filename and a magic pattern then they *both* must
 * match; if either comparison fails then we tell the classing engine
 * to move onto the next pattern.
 */

/* Sats: changed the match routine so that it does a file name match
   only if arcount is 1.
   It the argcount is 3 then do a magic pattern match.
   Return success only if argcount >=1 .
*/ 

#define STRCPY(a,b)	((void) strcpy((a), (b)))
#define STRCAT(a,b)	((void) strcat((a), (b)))

char *compile();
/* XXX void *malloc(); */

static int 
match(argcount, args, litcount, lits, entry, ret_value)
int argcount;
va_list args;		/* user arguments */
int litcount;
void **lits;		/* table litererals */
void *entry;		/* ptr to ce entry */
void **ret_value;	/* ptr to our return value */
{
	char *l_filename;
	char *a_filename;
	char *a_pathname;
	char *a_contents;
	int  a_length;
	char *match;
	int  match_len;
	int  match_offset;
	unsigned long a_value;
	unsigned long l_value;

	l_filename = lits[FNSL_FILENAME];
	a_pathname = va_arg(args, char *);
	a_filename = strrchr(a_pathname, '/');
	if (a_filename) {
		/* advance past the / char */
		a_filename++;
	} else {
		/* no / char */
		a_filename = a_pathname;
	}
	/* if there is a file name pattern, then translate it */
	if (argcount >= 1 ) {
		if(l_filename) {
			if (!fns_match(a_filename, l_filename))
			{
			 	return(1);
			}
		}

        	if (argcount >= 3) {
		/* now do the magic number match */
		a_contents = va_arg(args, char *);
		a_length = va_arg(args, int);
		match_offset = (int) lits[FNSL_MAG_OFFSET];

		/* make sure the file is long enough */
		if (a_length < match_offset) return (1);

		/* reposition to the right place */
		a_length -= match_offset;
		a_contents += match_offset;

		/* do the right thing; basically we first separate strings from
		 * numbers.
		 */
		switch ((int)lits[FNSL_MAG_TYPE]) {
		case MT_NONE:
			/* no magic number match for this entry */
			break;

		case MT_STRING:
			match = lits[FNSL_MAG_MATCH];
			match_len = strlen(match);

			/* make sure the file is long enough */
			if (match_len > a_length) return (1);

			/* check the string */
			if (strncmp(match, a_contents, match_len))
			{
				return (1);
			}
			break;

		case MT_BYTE:
			if (a_length < sizeof (unsigned char)) return (1);
			a_value = *((unsigned char *) a_contents);
			goto match;

		case MT_SHORT:
			if (a_length < sizeof (unsigned short)) return (1);
			a_value = *((unsigned short *) a_contents);
			goto match;

		case MT_LONG:
			if (a_length < sizeof (unsigned long)) 
				return (1);
			a_value = *((unsigned long *) a_contents);

match:
			/* common case.  first mask off any specified mask */
			l_value = (unsigned long) lits[FNSL_MAG_MATCH];
			if (lits[FNSL_MAG_MASK]) 
				a_value &= (unsigned int)lits[FNSL_MAG_MASK];

			/* perform the proper operation */
			switch ((int)lits[FNSL_MAG_OPS]) {
			case MO_EQUAL:
				if (a_value != l_value) return (1);
				break;
				
			case MO_GT:
				if (a_value > l_value) return (1);
				break;

			case MO_LT:
				if (a_value < l_value) return (1);
				break;

			case MO_ALL:
				if ((a_value & l_value) != l_value) return (1);
				break;

			case MO_EXCLUDE:
				/* at least one bit must not match */
				if (!(a_value ^ l_value)) return (1);
				break;
			}

			/* if we got here, then we passed the magic number
			 * test
			 */
			break;
		}
	}
	if(!((argcount >= 1) || (argcount >= 3)))
		return(1);

	/* success */
	*ret_value = entry;
	return (0);
	}
}



/*
 * convert ascii to long, watching for leading 0 and 0x for octal
 * and hex numbers
 */
static unsigned long
atolo(s)
char *s;
{
	unsigned long j = 0L;

	(void)sscanf(s, "%li", &j);
	return(j);
}



/* convert a string from external readable form into internal binary
 * form
 */
static void
convert_str(orig, new)
char *orig;
char *new;
{
	register c;
	int i;
	int val;

	while ((c = *orig++) != 0) {
		if (c != '\\') {
			/* just a normal character */
			*new++ = c;
			continue;
		}

		/* process the escape sequence */
		switch (c = *orig++) {
		case '\0':
			/* unexpected end of string! */
			break;

		default:	*new++ = c;	 break;
		case 'n':	*new++ = '\n';	break;
		case 'r':	*new++ = '\r';	break;
		case 'b':	*new++ = '\b';	break;
		case 't':	*new++ = '\t';	break;
		case 'f':	*new++ = '\f';	break;
		case 'v':	*new++ = '\v';	break;

		case 'x':
			/* hex code escape -- up to 3 digits */
			for (i = 0, val = 0; i < 3; i++) {
				c = *orig++;
				if (c >= '0' && 'c' <= '9') {
					val = (val << 4) + c - '0';
				} else if (c >= 'a' && c <= 'f') {
					val = (val << 4) + c - 'a' + 10;
				} else if (c >= 'A' && c <= 'F') {
					val = (val << 4) + c - 'A' + 10;
				} else {
					/* not a valid char */
					orig--;
					break;
				}
			}
			*new++ = val;
			break;

		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':

			/* octal escape, up to 3 digits */
			for (i = 1, val = c - '0'; i < 3; i++) {
				c = *orig++;
				if (c >= '0' && c <= '7') {
					val = c - '0';
				} else {
					orig--;
					break;
				}
			}
			*new++ = val;
			break;
		}

	}
	*new = '\0';
}



/*
 * build the file name space.  We watch for four attributes: 
 *	FNS_FILENAME -- the regexp(3) compatible file specification
 *	FNS_MAGIC_OFFSET -- all 3 of these are /etc/magic format.  see the
 *	FNS_MAGIC_TYPE		man page for more details.
 *	FNS_MAGIC_MATCH
 *
 * We end up building a file name space entry that we then allocate and
 * return.
 *
 * we return 0 on success and 1 for failure.
 */
static int
build(namespace, entry, new_buffer, new_size)
CE_NAMESPACE namespace;
CE_ENTRY entry;
void **new_buffer;
int *new_size;
{
	void *lits[FNSL_SIZE];
	int i;

	CE_ATTRIBUTE at_filename;
	CE_ATTRIBUTE at_mag_offset;
	CE_ATTRIBUTE at_mag_type;
	CE_ATTRIBUTE at_mag_match;
	CE_ATTRIBUTE at_mag_mask;

	char *e_filename;
	char *e_mag_offset;
	char *e_mag_type;
	char *e_mag_match;
	char *e_mag_mask;
	char *l_compile;

	enum magic_types type;
	enum magic_ops  op;

	void *newlits;
	void *newlitptr;

	int alloclength = sizeof lits;

	memset(lits, 0, sizeof lits);

	/* translate the attributes into internal id's */
	at_filename   = (CE_ATTRIBUTE)
		ce_get_attribute_id(namespace, "FNS_FILENAME");
	at_mag_offset = (CE_ATTRIBUTE)
		ce_get_attribute_id(namespace, "FNS_MAGIC_OFFSET");
	at_mag_match  = (CE_ATTRIBUTE)
		ce_get_attribute_id(namespace, "FNS_MAGIC_MATCH");
	at_mag_type   = (CE_ATTRIBUTE)
		ce_get_attribute_id(namespace, "FNS_MAGIC_TYPE");
	at_mag_mask   = (CE_ATTRIBUTE)
		ce_get_attribute_id(namespace, "FNS_MAGIC_MASK");

	/* get the filename attribute */
	e_filename = (char *) ce_get_attribute(namespace, entry, at_filename);


	l_compile = NULL;
	if (e_filename) {
		int len;
		int status;

#ifdef undef
		/* the compilation code is on hold for now... */

		/* there is a file name.  compile it. */

		len = strlen(e_filename) * 2 + 1000;
		l_compile = (void *) malloc(len);
		status = compile(e_filename, l_compile,
			&l_compile[len], '\0');

		/* check for errors */
		if (! status) {
			return(1);
		}
#endif undef
		l_compile = e_filename;

		alloclength += strlen(l_compile) + 1;
	}

	e_mag_offset = (char *) ce_get_attribute(namespace, entry, at_mag_offset);
	e_mag_match  = (char *) ce_get_attribute(namespace, entry, at_mag_match);
	e_mag_type   = (char *) ce_get_attribute(namespace, entry, at_mag_type);
	e_mag_mask   = (char *) ce_get_attribute(namespace, entry, at_mag_mask);

	type = MT_NONE;

	/* we insist on all 3 of offset, match, and type existing */
	if (e_mag_offset && e_mag_match && e_mag_type) {
		char buffer[30];
		char *amper;
		char *bp;

		lits[FNSL_MAG_OFFSET] = (void *) atolo(e_mag_offset);

		/* check for mask */
		amper = strchr(e_mag_type, '&');
		if (amper) {
			if (amper - e_mag_type + 1 >= sizeof buffer) {
				/* the type field was too long; it can't
				 * be legal
				 */
				fprintf(stderr, "Unknown magic type %s\n",
					e_mag_type);
				return(1);
			}

			/* make a local copy up to the & */
			strncpy(buffer, e_mag_type, amper-e_mag_type);
			buffer[amper - e_mag_type] = '\0';
			bp = buffer;

			/* process the mask */
			amper++;
			lits[FNSL_MAG_MASK] = (void *) atolo(amper);

		} else {
			if (e_mag_mask)
				lits[FNSL_MAG_MASK] = (void *) atolo(e_mag_mask);
			bp = e_mag_type;
		}

		if (strncmp(bp, "string", sizeof "string") == 0) {
			char *tmp;
			type = MT_STRING;

			/* allocate a temporary buffer from the stack
			 * and delete any special character processing.
			 * Then figure out the size of the converted
			 * string.
			 */
			tmp = (char *) malloc(strlen(e_mag_match) + 1);
			convert_str(e_mag_match, tmp);
			e_mag_match = tmp;

			alloclength += strlen(e_mag_match) + 1;

		} else if (strncmp(bp, "byte", sizeof "byte") == 0) {
			type = MT_BYTE;
		} else if (strncmp(bp, "short", sizeof "short") == 0) {
			type = MT_SHORT;
		} else if (strncmp(bp, "long", sizeof "long") == 0) {
			type = MT_LONG;
		} else {
			fprintf(stderr, "Unknown magic type %s\n",
				e_mag_type);
			return (1);
		}

		if (type != MT_STRING) {
			/* parse the match value */
			switch(*e_mag_match) {
			case '=':
				op = MO_EQUAL;
				e_mag_match++;
				break;
			case '>':
				op = MO_GT;
				e_mag_match++;
				break;
			case '<':
				op = MO_LT;
				e_mag_match++;
				break;
			case '&':
				op = MO_ALL;
				e_mag_match++;
				break;
			case '^':
				op = MO_EXCLUDE;
				e_mag_match++;
				break;
			default:
				op = MO_EQUAL;
				break;
			}

			lits[FNSL_MAG_OPS] = (void *) op;
			lits[FNSL_MAG_MATCH] = (void *) atolo(e_mag_match);

		}
	}
	lits[FNSL_MAG_TYPE] = (void *) type;


	/* now go back and allocate buffers */
	newlits = (void *)malloc(alloclength);

	if (! newlits) return(1);

	newlitptr = ((char *)newlits) + sizeof lits;

	if (l_compile) {
		/* copy and record the compiled match pointer */
		strcpy(newlitptr, l_compile);
		lits[FNSL_FILENAME] = newlitptr;

		/* advance the pointer */
		newlitptr = ((char *)newlits) + strlen(l_compile) + 1;
	}

	if (type == MT_STRING) {
		/* copy and record the string literal */
		strcpy(newlitptr, e_mag_match);
		lits[FNSL_MAG_MATCH] = newlitptr;
	}

	/* copy in the lit pointers */
	memcpy(newlits, lits, sizeof lits);

	/* bookkeeping: record the return value and length */
	*new_buffer = newlits;
	*new_size = alloclength;
#ifdef undef
	free (l_compile);
#endif
	return(0);
}



/*
 * Lexical definitions.
 *
 * All lexical space is allocated dynamically.
 * The eighth bit of characters is used to prevent recognition,
 * and eventually stripped.
 */

static
fns_match(s, p)
	register unsigned char *s, *p;
{
	register unsigned scc;
	int ok, lc;
	unsigned c, cc;


	/* globbed = 1; */
	for (;;) {
		scc = *s++;
		switch (c = *p++) {

		case '{':
			return (execbrc(p - 1, s - 1));
		case '[':
			ok = 0;
			lc = 077777;
			while (cc = *p++) {
				if (cc == ']') {
					if (ok)
						break;
					return (0);
				}
				if (cc == '-') {
					if (lc <= scc && scc <= *p++)
						ok++;
				} else
					if (scc == (lc = cc))
						ok++;
			}
			if (cc == 0) {
				/*fm_msg(TRUE, Missing, ']');*/
				return(-1);
			}
			continue;

		case '*':
			if (!*p)
				return (1);
			/* if (*p == '/') {
				p++;
				goto slash;
			} */
			for (s--; *s; s++) 
				if (fns_match(s, p))
					return (1);
			return (0);

		case 0:
			return (scc == 0);

		default:
			if (c != scc)
				return (0);
			continue; 

		case '?':
			if (scc == 0)
				return (0);
			continue;
		}
	}
}


static
execbrc(p, s)
	char *p, *s;
{
	char restbuf[BUFSIZ + 2];
	register char *pe, *pm, *pl;
	int brclev = 0;
	char *lm, savec;

	for (lm = restbuf; *p != '{'; *lm++ = *p++)
		continue;
	for (pe = ++p; *pe; pe++)
	switch (*pe) {

	case '{':
		brclev++;
		continue;

	case '}':
		if (brclev == 0)
			goto pend;
		brclev--;
		continue;

	case '[':
		for (pe++; *pe && *pe != ']'; pe++)
			continue;
		if (!*pe) {
			/*fm_msg(TRUE, Missing, ']');*/
			return(-1);
		}
		continue;
	}
pend:
	if (brclev || !*pe) {
		/*fm_msg(TRUE, Missing, '}');*/
		return(-1);
	}
	for (pl = pm = p; pm <= pe; pm++)
	switch (*pm) {

	case '{':
		brclev++;
		continue;

	case '}':
		if (brclev) {
			brclev--;
			continue;
		}
		goto doit;

	case ',':
		if (brclev)
			continue;
doit:
		savec = *pm;
		*pm = 0;
		STRCPY(lm, pl);
		STRCAT(restbuf, pe + 1);
		*pm = savec;

#ifdef undef
		if (s == 0) {
			sgpathp = gpathp;
			expand(restbuf);
			gpathp = sgpathp;
			*gpathp = 0;
		} else
#endif undef
		if (fns_match(s, restbuf))
			return (1);

		/* sort(); */
		pl = pm + 1;
		continue;

	case '[':
		for (pm++; *pm && *pm != ']'; pm++)
			continue;
		if (!*pm) {
			/*fm_msg(TRUE, Missing, ']');*/
			return(-1);
		}
		continue;
	}
	return (0);
}



struct name_tab {
	char *nt_name;
	char *nt_type;
	char *nt_value;
} cookie_name_tab[] = {
	{ "FNS_MAGIC_OFFSET" },
	{ "FNS_MAGIC_TYPE" },
	{ "FNS_MAGIC_MATCH" },
	{ "FNS_FILENAME" },
};
#define NUMCOOKIES ((sizeof cookie_name_tab)/(sizeof (struct name_tab)))


static int
get_entry_cookie(ns, entry, cookie_ptr)
CE_NAMESPACE ns;
CE_ENTRY entry;
void **cookie_ptr;
{
	CE_ATTRIBUTE attribute;
	char *value;
	char *type;
	char *result;
	int i;
	int len;
	struct name_tab ntab[NUMCOOKIES];
	struct name_tab *nt;


	/* there are four significant attributes:
	 * FNS_MAGIC_OFFSET, FNS_MAGIC_TYPE, FNS_MAGIC_MATCH, and FNS_FILENAME
	 *
	 * Record the state of these four attributes, and build a cookie
	 * from it.  The cookie looks like a sequence of
	 * "(ATTR,type,<value>)" tokens
	 * sequences.
	 */

	memcpy(ntab, cookie_name_tab, sizeof(ntab));

	/* get the values of the attribute strings */
	for (i = NUMCOOKIES, len = 0, nt = ntab; i > 0; i--, nt++) {
		nt->nt_value = NULL;
		nt->nt_type = NULL;

		attribute = ce_get_attribute_id(ns, nt->nt_name);
		if (! attribute) continue;

		value = ce_get_attribute(ns, entry, attribute);
		if (! value) continue;

		type = ce_get_attribute_type(ns, entry, attribute);
		if (! type) type = "";

		len += strlen(nt->nt_name) + strlen(value) + strlen(type) + 6;
		nt->nt_value = value;
		nt->nt_type = type;

		/* adjust the length for embedded '>' chars in the cookie */
		while (value = strpbrk(value, ">\\")) {
			len++;
		}
	}

	result = (char *)malloc(len +1);
	*cookie_ptr = result;

	if (! result) return(0);

	/* now initialize the result */
	for (i = NUMCOOKIES, nt = ntab; i > 0; i--, nt++) {
		char *s;

		sprintf(result, "(%s,%s,<", nt->nt_name, nt->nt_type);
		result += strlen(result);

		if (s = strpbrk(value, ">\\")) {
			/* we need to escape these two characters */
			do {
				memcpy(result, value, s-value);
				result += s-value;
				*result++ = '\\';
				*result++ = *s++;
				value = s;
			} while(s = strpbrk(value, ">\\"));

			/* now do the trailing part of the string */
			strcpy(result, value);
		} else {
			/* the easy case: no embedded '>' chars */
			sprintf(result, "(%s,%s,<%s>)",
				nt->nt_name, nt->nt_type, nt->nt_value);
		}
		
		result += strlen(result);
		*result++ = '>';
		*result++ = ')';
	}
	*result = '\0';

	return(1);
}


/* this routine splits up a cookie into successive chunks.
 *
 * A cookie is defined as "(NAME,type,<value>)"
 * \> can be used to escape the trailing >
 * This routine assumes that the cookie can be written into.
 *
 * The routine updates the cookiep, namep, valuep, and typep
 * routines with pointers into the cookie string.  cookiep
 * will be updated to point to the beginning of the next cookie.
 */
static int
cookie_split (cookiep, namep, valuep, typep)
char **cookiep;
char **namep;
char **valuep;
char **typep;
{
	char *cookie = *cookiep;
	char *value;
	char *s;

	if (*cookie++ != '(') return (0);

	/* look for the name */
	*namep = cookie;
	cookie = strchr(cookie, ',');
	if (!cookie) return (0);
	*cookie++ = '\0';

	/* look for the type */
	*typep = cookie;
	cookie = strchr(cookie, ',');
	if (!cookie) return (0);
	*cookie++ = '\0';

	/* look for the value */
	if (*cookie++ != '<') return (0);
	*valuep = cookie;

	/* walk down the cookie undoing any escape sequences \< and \\ */
	s = cookie;
	while (*cookie != '>') {
		switch (*cookie) {
		case '\0':
			/* we ran off the end of the string without a
			 * terminator
			 */
			return (0);
		
		case '\\':
			cookie++;

			/* be conservative: these are the only two chars
			 * we currently check...
			 */
			if (*cookie != '>' || *cookie != '\\') return (0);
			*s++ = *cookie++;
			break;

		default:
			*s++ = *cookie++;
			break;
		}
	}

	if (*cookie++ != ')') return (0);

	*cookiep = cookie;
}




static int
match_entry_cookie(ns, orig_cookie, entry)
CE_NAMESPACE ns;
char *orig_cookie;
CE_ENTRY entry;
{
	CE_ATTRIBUTE attribute;
	char *a_type;
	char *a_value;
	struct name_tab ntab[NUMCOOKIES];
	struct name_tab *nt;
	char *name;
	char *value;
	char *type;
	char *cookie;
	char *cookie_base;
	int i;
	int retval = 0;


	memcpy(ntab, cookie_name_tab, sizeof(ntab));

	/* sanity check */
	if (!orig_cookie || !*orig_cookie) return(0);


	/* make a cookie that we can write into */
	cookie_base = cookie = (char *)malloc(strlen(orig_cookie) +1);
	if (!cookie) goto end;

	strcpy(cookie, orig_cookie);

	/* try and split the cookie into strings */
	while (*cookie) {
		/* cookie_split returns zero when invalid syntax */
		if( !cookie_split(&cookie, &name, &value, &type)) goto end;

		for (i = NUMCOOKIES, nt = ntab; i > 0; i--, nt++) {
			if (STREQ(nt->nt_name, name)) break;
		}
		if (i == 0) {
			/* a name we were not expecting; it was not in ntab */
			goto end;
		}
		/* remember the info */
		nt->nt_value = value;
		nt->nt_type = type;
	}

	/* now see if we have a match */
	for (i = NUMCOOKIES, nt = ntab; i > 0; i--, nt++) {
		attribute = ce_get_attribute_id(ns, nt->nt_name);

		/* if there is no attribute & one was specified, give up */
		if (!attribute && nt->nt_value) goto end;

		a_value = ce_get_attribute(ns, entry, attribute);
		a_type = ce_get_attribute_type(ns, entry, attribute);

		if (!a_value && nt->nt_value) goto end;
		if (a_value && !nt->nt_value) goto end;

		/* if the attributes or types are specified but don't match,
		 * give up
		 */
		if (a_value) {
			if (!a_type) a_type = "";
			if (! STREQ(a_value, nt->nt_value)) goto end;
			if (! STREQ(a_type, nt->nt_type)) goto end;
		}
	}

	/* success! */
	retval = 1;

end:
	if (cookie_base) free(cookie_base);
	return(retval);

}



/* stolen from regexp.h... */
#if !defined(_regexp_h) && 0
#define _regexp_h


/* I made one change to regexp: I added CCIRCF as an initial token
 * to the match string to anchor the match to the beginning of the
 * line; this eliminates a global variable that had to be separately
 * tracked.
 */
#define INIT	register char *reginstr = instring;
#define GETC()		*reginstr++
#define PEEKC()		*reginstr
#define UNGETC()	reginstr--
#define RETURN(n)	return(n)
#define ERROR(n)	{ fprintf(stderr, "regexp error %d\n", n); return(0);}


/*	@(#)regexp.h 1.9 88/08/19 SMI; from S5R3.1 1.4.1.2	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include <ctype.h>

#define	CBRA	2
#define	CCHR	4
#define	CDOT	8
#define	CCL	12
#define	CXCL	16
#define	CDOL	20
#define	CCEOF	22
#define	CKET	24
#define	CBRC	28
#define	CLET	30
#define	CBACK	36
#define NCCL	40
#define CCIRCF	44

#define	STAR	01
#define RNGE	03

#define	NBRA	9

#define PLACE(c)	ep[c >> 3] |= bittab[c & 07]
#define ISTHERE(c)	(ep[c >> 3] & bittab[c & 07])
#define ecmp(s1, s2, n)	(!strncmp(s1, s2, n))

static char	*braslist[NBRA];
static char	*braelist[NBRA];
int	sed, nbra;
char	*loc1, *loc2, *locs;
static int	nodelim;

static int	low;
static int	size;

static char	bittab[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

/*ARGSUSED*/
static char *
compile(instring, ep, endbuf, seof)
register char *ep;
char *instring, *endbuf;
{
	INIT	/* Dependent declarations and initializations */
	register c;
	register eof = seof;
	char *lastep;
	int cclcnt;
	char bracket[NBRA], *bracketp;
	int closed;
	int neg;
	int lc;
	int i, cflg;
	int iflag; /* used for non-ascii characters in brackets */

	if((c = GETC()) == eof || c == '\n') {
		if(c == '\n') {
			UNGETC(c);
			nodelim = 1;
		}
		if(*ep == 0 && !sed)
			ERROR(41);
		RETURN(ep);
	}
	bracketp = bracket;
	closed = nbra = 0;
	lastep = 0;
	if(c == '^')
		*ep++ = CCIRCF;
	else
		UNGETC(c);
	while(1) {
		if(ep >= endbuf)
			ERROR(50);
		c = GETC();
		if(c == eof) {
			*ep++ = CCEOF;
			*ep++ = '\0';
			if (bracketp != bracket)
				ERROR(42);
			RETURN(ep);
		}
		if(c != '*' && ((c != '\\') || (PEEKC() != '{')))
			lastep = ep;
		switch(c) {

		case '.':
			*ep++ = CDOT;
			continue;

		case '\0':
		case '\n':
			if(!sed) {
				UNGETC(c);
				*ep++ = CCEOF;
				*ep++ = '\0';
				nodelim = 1;
				if(bracketp != bracket)
					ERROR(42);
				RETURN(ep);
			}
			else ERROR(36);
		case '*':
			if(lastep == 0 || *lastep == CBRA || *lastep == CKET
			    || *lastep == CBRC || *lastep == CLET)
				goto defchar;
			*lastep |= STAR;
			continue;

		case '$':
			if(PEEKC() != eof && PEEKC() != '\n' && PEEKC() != '\0')
				goto defchar;
			*ep++ = CDOL;
			continue;

		case '[':
			if(&ep[17] >= endbuf)
				ERROR(50);

			*ep++ = CCL;
			lc = 0;
			for(i = 0; i < 16; i++)
				ep[i] = 0;

			neg = 0;
			if((c = GETC()) == '^') {
				neg = 1;
				c = GETC();
			}
			iflag = 1;
			do {
				c &= 0377;
				if(c == '\0' || c == '\n')
					ERROR(49);
				if((c & 0200) && iflag) {
					iflag = 0;
					if(&ep[32] >= endbuf)
						ERROR(50);
					ep[-1] = CXCL;
					for(i = 16; i < 32; i++)
						ep[i] = 0;
				}
				if(c == '-' && lc != 0) {
					if((c = GETC()) == ']') {
						PLACE('-');
						break;
					}
					if(sed && c == '\\') {
						switch(PEEKC()) {

						case 'n':
							(void) GETC();
							c = '\n';
							break;

						case '\\':
							(void) GETC();
							c = '\\';
							break;
						}
					}
					if((c & 0200) && iflag) {
						iflag = 0;
						if(&ep[32] >= endbuf)
							ERROR(50);
						ep[-1] = CXCL;
						for(i = 16; i < 32; i++)
							ep[i] = 0;
					}
					while(lc < c ) {
						PLACE(lc);
						lc++;
					}
				} else if(sed && c == '\\') {
					switch(PEEKC()) {

					case 'n':
						(void) GETC();
						c = '\n';
						break;

					case '\\':
						(void) GETC();
						c = '\\';
						break;
					}
				}
				lc = c;
				PLACE(c);
			} while((c = GETC()) != ']');
			
			if(iflag)
				iflag = 16;
			else
				iflag = 32;
			
			if(neg) {
				if(iflag == 32) {
					for(cclcnt = 0; cclcnt < iflag; cclcnt++)
						ep[cclcnt] ^= 0377;
					ep[0] &= 0376;
				} else {
					ep[-1] = NCCL;
					/* make nulls match so test fails */
					ep[0] |= 01;
				}
			}

			ep += iflag;

			continue;

		case '\\':
			switch(c = GETC()) {

			case '<':
				*ep++ = CBRC;
				continue;

			case '>':
				*ep++ = CLET;
				continue;

			case '(':
				if(nbra >= NBRA)
					ERROR(43);
				*bracketp++ = nbra;
				*ep++ = CBRA;
				*ep++ = nbra++;
				continue;

			case ')':
				if(bracketp <= bracket) 
					ERROR(42);
				*ep++ = CKET;
				*ep++ = *--bracketp;
				closed++;
				continue;

			case '{':
				if(lastep == (char *) 0)
					goto defchar;
				*lastep |= RNGE;
				cflg = 0;
			nlim:
				c = GETC();
				i = 0;
				do {
					if('0' <= c && c <= '9')
						i = 10 * i + c - '0';
					else
						ERROR(16);
				} while(((c = GETC()) != '\\') && (c != ','));
				if(i > 255)
					ERROR(11);
				*ep++ = i;
				if(c == ',') {
					if(cflg++)
						ERROR(44);
					if((c = GETC()) == '\\')
						*ep++ = 255;
					else {
						UNGETC(c);
						goto nlim;
						/* get 2'nd number */
					}
				}
				if(GETC() != '}')
					ERROR(45);
				if(!cflg)	/* one number */
					*ep++ = i;
				else if((ep[-1] & 0377) < (ep[-2] & 0377))
					ERROR(46);
				continue;

			case '\n':
				ERROR(36);

			case 'n':
				if(sed)
					c = '\n';
				goto defchar;

			default:
				if(c >= '1' && c <= '9') {
					if((c -= '1') >= closed)
						ERROR(25);
					*ep++ = CBACK;
					*ep++ = c;
					continue;
				}
			}
	/* Drop through to default to use \ to turn off special chars */

		defchar:
		default:
			lastep = ep;
			*ep++ = CCHR;
			*ep++ = c;
		}
	}
	/*NOTREACHED*/
}

static int step(p1, p2)
register char *p1, *p2; 
{
	register c;


	/*
	 * Save the beginning of the string in "loc1", so that "advance"
	 * knows when it's looking at the first character of the string;
	 * it needs this to implement \<.
	 */
	loc1 = p1;
	if(*p2 == CCIRCF) {
		return(advance(p1, &p2[1]));
	}
	/* fast check for first character */
	if(*p2 == CCHR) {
		c = p2[1];
		do {
			if(*p1 != c)
				continue;
			if(advance(p1, p2)) {
				loc1 = p1;
				return(1);
			}
		} while(*p1++);
		return(0);
	}
		/* regular algorithm */
	do {
		if(advance(p1, p2)) {
			loc1 = p1;
			return(1);
		}
	} while(*p1++);
	return(0);
}

static advance(lp, ep)
register char *lp, *ep;
{
	register char *curlp;
	register int c;
	char *bbeg; 
	register char neg;
	int ct;

	while(1) {
		neg = 0;
		switch(*ep++) {

		case CCHR:
			if(*ep++ == *lp++)
				continue;
			return(0);
	
		case CDOT:
			if(*lp++)
				continue;
			return(0);
	
		case CDOL:
			if(*lp == 0)
				continue;
			return(0);
	
		case CCEOF:
			loc2 = lp;
			return(1);
	
		case CXCL: 
			c = (unsigned char)*lp++;
			if(ISTHERE(c)) {
				ep += 32;
				continue;
			}
			return(0);
		
		case NCCL:	
			neg = 1;

		case CCL: 
			c = *lp++;
			if(((c & 0200) == 0 && ISTHERE(c)) ^ neg) {
				ep += 16;
				continue;
			}
			return(0);
		
		case CBRA:
			braslist[*ep++] = lp;
			continue;
	
		case CKET:
			braelist[*ep++] = lp;
			continue;
	
		case CCHR | RNGE:
			c = *ep++;
			getrnge(ep);
			while(low--)
				if(*lp++ != c)
					return(0);
			curlp = lp;
			while(size--) 
				if(*lp++ != c)
					break;
			if(size < 0)
				lp++;
			ep += 2;
			goto star;
	
		case CDOT | RNGE:
			getrnge(ep);
			while(low--)
				if(*lp++ == '\0')
					return(0);
			curlp = lp;
			while(size--)
				if(*lp++ == '\0')
					break;
			if(size < 0)
				lp++;
			ep += 2;
			goto star;
	
		case CXCL | RNGE:
			getrnge(ep + 32);
			while(low--) {
				c = (unsigned char)*lp++;
				if(!ISTHERE(c))
					return(0);
			}
			curlp = lp;
			while(size--) {
				c = (unsigned char)*lp++;
				if(!ISTHERE(c))
					break;
			}
			if(size < 0)
				lp++;
			ep += 34;		/* 32 + 2 */
			goto star;
		
		case NCCL | RNGE:
			neg = 1;
		
		case CCL | RNGE:
			getrnge(ep + 16);
			while(low--) {
				c = *lp++;
				if(((c & 0200) || !ISTHERE(c)) ^ neg)
					return(0);
			}
			curlp = lp;
			while(size--) {
				c = *lp++;
				if(((c & 0200) || !ISTHERE(c)) ^ neg)
					break;
			}
			if(size < 0)
				lp++;
			ep += 18; 		/* 16 + 2 */
			goto star;
	
		case CBACK:
			bbeg = braslist[*ep];
			ct = braelist[*ep++] - bbeg;
	
			if(ecmp(bbeg, lp, ct)) {
				lp += ct;
				continue;
			}
			return(0);
	
		case CBACK | STAR:
			bbeg = braslist[*ep];
			ct = braelist[*ep++] - bbeg;
			curlp = lp;
			while(ecmp(bbeg, lp, ct))
				lp += ct;
	
			while(lp >= curlp) {
				if(advance(lp, ep))	return(1);
				lp -= ct;
			}
			return(0);
	
	
		case CDOT | STAR:
			curlp = lp;
			while(*lp++);
			goto star;
	
		case CCHR | STAR:
			curlp = lp;
			while(*lp++ == *ep);
			ep++;
			goto star;
	
		case CXCL | STAR:
			curlp = lp;
			do {
				c = (unsigned char)*lp++;
			} while(ISTHERE(c));
			ep += 32;
			goto star;
		
		case NCCL | STAR:
			neg = 1;

		case CCL | STAR:
			curlp = lp;
			do {
				c = *lp++;
			} while(((c & 0200) == 0 && ISTHERE(c)) ^ neg);
			ep += 16;
			goto star;
	
		star:
			if(--lp == curlp) {
				continue;
			}

			if(*ep == CCHR) {
				c = ep[1];
				do {
					if(*lp != c)
						continue;
					if(advance(lp, ep))
						return(1);
				} while(lp-- > curlp);
				return(0);
			}

			if(*ep == CBACK) {
				c = *(braslist[ep[1]]);
				do {
					if(*lp != c)
						continue;
					if(advance(lp, ep))
						return(1);
				} while(lp-- > curlp);
				return(0);
			}

			do {
				if(lp == locs)
					break;
				if(advance(lp, ep))
					return(1);
			} while(lp-- > curlp);
			return(0);

#define	uletter(c)	(isalpha(c) || (c) == '_')
		case CBRC:
			if(lp == loc1)
				continue;
			c = (unsigned char)*lp;
			if(uletter(c) || isdigit(c)) {
				c = (unsigned char)lp[-1];
				if(!uletter(c) && !isdigit(c))
					continue;
			}
			return(0);

		case CLET:
			c = (unsigned char)*lp;
			if(!uletter(c) && !isdigit(c))
				continue;
			return(0);
#undef uletter
		}
	}
	/*NOTREACHED*/
}

static
getrnge(str)
register char *str;
{
	register int sizecode;

	low = *str++ & 0377;
	sizecode = *str & 0377;
	if (sizecode == 255)
		size = 20000;
	else
		size = sizecode - low;
}

#endif /*!_regexp_h*/

