/* @(#)headers.c	3.7 - 97/04/18 */

/* headers.c -- compare header lines of a file */

#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <memory.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#ifdef SVR4
#include <netdb.h>
#include <sys/utsname.h>
#endif SVR4

#include "ck_strings.h"
#include "global.h"
#include "msg.h"
#include "hash.h"
#include "bool.h"

#define	DEBUG_FLAG mt_debugging
extern int mt_debugging;
#include "debug.h"


/* local definitions */
char *hackunixfrom();
char *findheader();
char *findheaderlist();
char *findchar();
int header_length();
int matchword();
char *skipwhite();
char *skipwhite_reverse();
char *findend_of_field();
char *newname();
char *get_recipients();
char *metoo_usermap();
struct header_obj *header_create();
struct header_obj *header_set_string();
struct header_obj *header_set();
struct header_obj *header_split();
struct header_obj *header_delete();
void header_insert();
int header_enumerate();
int header_write();

extern struct link_obj *link_add();
extern int	link_free();
extern int	link_enumerate();
extern char	*make_nmlist();
extern char	*gettoken();
extern char	*skin_arpa();
extern char	*mt_value(char *);

#define	MAXARGS	64
struct _avlist
{
	char	*name;
	char	**value;
};

/*
 * hack the unix from line.  It consists of a sequence of
 * "^From name day mon date...\n".  We can depend on the line
 * starting with "From"...
 *
 * We will allocate new buffers that hold the name and the date
 */

char *
hackunixfrom( buf, bufend, fromp, datep )
char *buf;
char *bufend;
char **fromp;
char **datep;
{
	char *eol;
	char *end;

	/* only match to an end of line */
	eol = findchar( '\n', buf, bufend );
	eol = skipwhite_reverse( buf, eol );

	/* get to the name part */
	buf = findchar( ' ', buf, eol );
	buf = skipwhite( buf, eol );

	end = findchar( ' ', buf+1, eol );
	*fromp = newname( buf, end );

	/* now do the date part */
	buf = skipwhite( end, eol );
	*datep = newname( buf, eol );

	return( eol );
}

static struct _avlist *
header_in_list (buf, colon, p_arg)
char *buf;
char *colon;
register struct _avlist *p_arg;
{
	register int argc = MAXARGS;

	while ((--argc >= 0) && (p_arg->name != NULL))
	{
		if (matchword(p_arg->name, buf, colon))
			return( p_arg );
		p_arg++;
	}
	return( NULL );
}

/*
 * An slightly efficient to parse the message headers.  The caller registers
 * a list of attribute (char *) and value (char **) pairs of the message
 * header fields to be returned.  If identical headers exist, their values
 * are concatenated and separated by a single space.  Therefore, multiple "To:"
 * from MCI mail can be parsed correctly.  Note, this function can only
 * handle up to 64 attribute-value pairs.  Example:
 *
 *	char *from;
 *	char *to;
 *	header_find(buf, bufend, "from", &from, "to", &to, NULL);
 */
void
header_find(char *buf, char *bufend, ...)
{
	va_list args;
	struct _avlist arg[MAXARGS];
	register struct _avlist *p_arg;
	register int  len;
	register char *end;
	register char *tail;
	register char *colon;

	va_start(args, bufend);
	p_arg = arg;
	while ((p_arg->name = va_arg(args, char *)) != (char *) 0)
	{
		p_arg->value = va_arg(args, char **);
		*(p_arg->value) = NULL;
		if (++p_arg == &arg[MAXARGS])
			break;
	}
	va_end(args);

	while (buf < bufend)
	{
		end = findend_of_field(buf, bufend);

		colon = findchar(':', buf, end);
		if (colon && (p_arg = header_in_list(buf, colon, arg)) != NULL)
		{
			/* we have a match! */
			colon = skipwhite( colon + 1, end );
			tail = skipwhite_reverse(colon, end);

			if ((buf = *(p_arg->value)) == NULL)
			{
				buf = ck_malloc(tail - colon + 1);
				*(p_arg->value) = buf;
			}
			else
			{
				/* append the new value to the old value */
				len = strlen (buf);
				buf = ck_malloc(tail - colon + 2 + len);

				strcpy (buf, *(p_arg->value));
				ck_free( *(p_arg->value) );
				*(p_arg->value) = buf;

				buf += len;
				*buf++ = ' ';
				*buf = '\0';
			}

			while (colon < tail)
				*buf++ = *colon++;
			*buf = '\0';
		}

		buf = end;
	}
}

/*
 * Find a matching header string in the specified buffer.
 *
 * The header is searched for between buf and bufend.  A 'match'
 * is specified by the null terminated string 'name' being embedded
 * between a '\n' char and a ':' char.  The matched header
 * is terminated by a '\n' char not followed by whitespace (' ' or '\t').
 *
 * findheader returns a newly allocated buffer that contains the
 * entire field, including comments and all but the trailing newline.
 *
 */

char *
findheader( name, buf, bufend )
char *name;
char *buf;
char *bufend;
{
	char *value;

	header_find (buf, bufend, name, &value, 0);
	return (value);
}

char *
findheaderlist( name, p_hdr )
char *name;
struct header_obj *p_hdr;
{
	char *v;

	while (p_hdr != NULL)
	{
		header_find(p_hdr->hdr_start, p_hdr->hdr_end, name, &v, 0);
		if (v)
			return (v);
		p_hdr = p_hdr->hdr_next;
	}
	return (NULL);
}

count_lines (buf, len)
register char *buf;
register int len;
{
	register int nlines = 0;

	if (len > 0)
	{
		while (--len >= 0)
		{
			if (*buf++ == '\n')
				nlines++;
		}
		if (buf[-1] != '\n')
			nlines++;
	}
	return (nlines);
}

struct header_obj *
header_create( buf, bufend )
char *buf;
char *bufend;
{
	struct header_obj *p_hdr;

	p_hdr = (struct header_obj *)ck_malloc (sizeof(struct header_obj));
	p_hdr->hdr_start = buf;
	p_hdr->hdr_end = bufend;
	p_hdr->hdr_next = NULL;
	p_hdr->hdr_allocated = HDR_NONE;
	p_hdr->hdr_single_val = 0;

	return (p_hdr);
}

void
header_destroy( p_hdr )
struct header_obj *p_hdr;
{
	register struct header_obj *p_next;

	while (p_hdr != NULL)
	{
		p_next = p_hdr->hdr_next;
		if (p_hdr->hdr_allocated == HDR_MALLOC)
		{
			ck_free (p_hdr->hdr_start);
			p_hdr->hdr_start = NULL;
		}
		else if (p_hdr->hdr_allocated == HDR_MMAP)
		{
			ck_zfree (p_hdr->hdr_start);
			p_hdr->hdr_start = NULL;
		}
		ck_free (p_hdr);
		p_hdr = p_next;
	}
}

struct header_obj *
header_split (buf, bufend)
register char *buf;
register char *bufend;
{
	struct header_obj *header;
	struct header_obj *p_hdr;
	register char *end;

	header = NULL;
	while (buf < bufend)
	{
		end = findend_of_field (buf, bufend);

		p_hdr = header_create(buf, end);
		if (p_hdr == NULL)
			break;
		header_insert (&header, p_hdr, 0);

		buf = end;

		/* check for end of buffer or a blank line */
		if (end >= bufend) break;
		if (*end == '\n') break;

	}
	return (header);
}

/*
 * This function only works when each header is in a separate node (i.e after
 * calling header_split(), or hdr_single_val is TRUE).
 */
struct header_obj *
header_delete (header, name, len)
struct header_obj **header;
char *name;
int len;
{
	register char *p_colon;
	register struct header_obj *p_curr;
	register struct header_obj *p_prev;

	p_prev = NULL;
	p_curr = *header;
	while (p_curr != NULL)
	{
		/* ZZZ: should we check "hdr_single_val"? */
		p_colon = &p_curr->hdr_start[len];
		if ((p_colon < p_curr->hdr_end) && (*p_colon == ':') &&
		    (strncasecmp (p_curr->hdr_start, name, len) == 0))
		{
			if (p_curr->hdr_allocated == HDR_MALLOC)
				ck_free (p_curr->hdr_start);
			else if (p_curr->hdr_allocated == HDR_MMAP)
				ck_zfree (p_curr->hdr_start);

			if (p_prev == NULL)
				*header = p_curr->hdr_next;
			else
				p_prev->hdr_next = p_curr->hdr_next;
			ck_free (p_curr);
			return (p_prev);
		}
		p_prev = p_curr;
		p_curr = p_curr->hdr_next;
	}
	return ((struct header_obj *) EOF);
}

static struct header_obj *
header_construct (name, value)
unsigned char *name;
unsigned char *value;
{
	int len;
	struct header_obj *p_hdr;

	if ((name == NULL) || (value == NULL) || (*name == '\0'))
		return (NULL);

	while (*value != '\0')
	{
		if (!isspace(*value)) 
			break;
		value++;
	}
	if (*value == '\0')
		return (NULL);

	if ((p_hdr = (struct header_obj *) ck_malloc(sizeof(*p_hdr))) == NULL)
		return (NULL);

	len = strlen((char *)name) + strlen((char *)value) + 3;
	if ((p_hdr->hdr_start = (char *) ck_malloc (len + 1)) == NULL)
	{
		ck_free (p_hdr);
		return (NULL);
	}
	p_hdr->hdr_end = p_hdr->hdr_start + len;
	p_hdr->hdr_next = NULL;
	p_hdr->hdr_allocated = HDR_MALLOC;
	p_hdr->hdr_single_val = 1;
	sprintf (p_hdr->hdr_start, "%s: %s\n", name, value);

	return (p_hdr);
}

/*
 * Add a header at a position indicated by "p_curr" which can be
 * 	NULL	- means to add the beginning of the list.
 *	EOF	- means to add the end of the list.
 *	ptr	- means to add after it.
 */
struct header_obj *
header_add (header, p_curr, name, value)
struct header_obj **header;
struct header_obj *p_curr;
char	*name;
char	*value;
{
	struct header_obj *p_hdr;

	p_hdr = header_construct (name, value);
	if (p_hdr == NULL)
		return (NULL);

	if (p_curr == (struct header_obj *) EOF)
	{
		/* put at the end of the list */
		header_insert (header, p_hdr, 0);
	}
	else if (p_curr == NULL)
	{
		/* put at the beginning of the list */
		p_hdr->hdr_next = *header;
		*header = p_hdr;
	}
	else
	{
		p_hdr->hdr_next = p_curr->hdr_next;
		p_curr->hdr_next = p_hdr;
	}
	return (p_hdr);
}

void
header_insert (header, p_hdr, nodup)
struct header_obj **header;
struct header_obj *p_hdr;
bool nodup;
{
	struct header_obj *p_next;
	register int len = 0;
	register char *name = p_hdr->hdr_start;
	register struct header_obj *p_curr;
	register struct header_obj *p_prev;

	if (nodup)
	{
		char	*ptr;
		ptr = strchr (name, ':');
		if (ptr != NULL)
			len = ptr - name + 1;
		else
			len = p_hdr->hdr_end - name;
	}

	p_prev = NULL;
	p_curr = *header;
	while (p_curr != NULL)
	{
		/* if there is a duplicated header name, replace the old one */
		if ((len > 0) && (!strncasecmp(p_curr->hdr_start, name, len)))
		{
			if (p_curr->hdr_allocated == HDR_MALLOC)
				ck_free (p_curr->hdr_start);
			else if (p_curr->hdr_allocated == HDR_MMAP)
				ck_zfree (p_curr->hdr_start);

			p_next = p_curr->hdr_next;
			ck_free (p_curr);
			p_curr = p_next;
			break;
		}
		p_prev = p_curr;
		p_curr = p_curr->hdr_next;
	}

	p_hdr->hdr_next = p_curr;
	if (p_prev == NULL)
		*header = p_hdr;
	else
		p_prev->hdr_next = p_hdr;
}

struct header_obj *
header_set_string (header, buf, bufend)
struct header_obj **header;
char *buf;
char *bufend;
{
	struct header_obj *p_hdr;

	p_hdr = header_create (buf, bufend);
	if (p_hdr != NULL)
		header_insert (header, p_hdr, TRUE);

	return (p_hdr);
}

struct header_obj *
header_set (header, name, value)
struct header_obj **header;
char *name;
char *value;
{
	struct header_obj *p_hdr;

	p_hdr = header_construct (name, value);
	if (p_hdr != NULL)
		header_insert (header, p_hdr, TRUE);

	return (p_hdr);
}

int
header_length (p_hdr)
struct header_obj *p_hdr;
{
	register int len = 0;

	while (p_hdr != NULL)
	{
		len += p_hdr->hdr_end - p_hdr->hdr_start;
		p_hdr = p_hdr->hdr_next;
	}
	return (len);
}

#ifdef	LINE_COUNT
int
header_lines (p_hdr)
struct header_obj *p_hdr;
{
	register int num_lines = 0;

	while (p_hdr != NULL)
	{
		num_lines += count_lines (p_hdr->hdr_start,
					  p_hdr->hdr_end - p_hdr->hdr_start);
		p_hdr = p_hdr->hdr_next;
	}
	return (num_lines);
}
#endif	LINE_COUNT

struct header_obj *
header_dup (p_hdr)
struct header_obj *p_hdr;
{
	register int	len;
	register char 	*buf;
	struct header_obj *newhdr = NULL;
	register struct header_obj *p_nhdr;

	while (p_hdr != NULL)
	{
		len = p_hdr->hdr_end - p_hdr->hdr_start;
		buf = ck_malloc (len+1);
		memcpy (buf, p_hdr->hdr_start, len);
		buf[len] = '\0';

		p_nhdr = header_create (buf, buf+len);
		if (p_nhdr != NULL)
		{
			p_nhdr->hdr_allocated = HDR_MALLOC;
			header_insert (&newhdr, p_nhdr, 0);
		}
		p_hdr = p_hdr->hdr_next;
	}

	return (newhdr);
}

header_enumerate (p_hdr,p_func,arg)
struct header_obj *p_hdr;
int (*p_func)();
{
	int retval;
	register int len;
	register char *buf;
	register char *end;
	register char *colon;
	register char *name;
	register char *bufend;

	while (p_hdr != NULL)
	{
		end = p_hdr->hdr_start;
		bufend = p_hdr->hdr_end;
		do
		{
			buf = end;
			end = findend_of_field (buf, bufend);

			len = end - buf;
			name = ck_malloc (len + 1);
			strncpy (name, buf, len);
			name[len] = '\0';

			colon = findchar (':', name, name + len);
			if (*colon == ':')
				*colon++ = '\0';

			retval = (*p_func) (name, colon, arg);

			ck_free (name);

			if (retval != 0)
				return (retval);

		} while (end < bufend);

		p_hdr = p_hdr->hdr_next;
	}
}

header_write (ofp, p_hdr, end_of_hdr)
FILE *ofp;
struct header_obj *p_hdr;
int end_of_hdr;
{
	while (p_hdr != NULL)
	{
		if (fwrite (p_hdr->hdr_start, p_hdr->hdr_end-p_hdr->hdr_start,
				1, ofp) != 1)
		{
			return (1);
		}

		p_hdr = p_hdr->hdr_next;
	}

	/* option to write out the end-of-header separator */
	if (end_of_hdr) {
		if (fputs("\n", ofp) != 1) {
			return (1);
		}
	}

	return (0);
}

void
getheadername(name, namelen, buf, bufend)
char *name;
int namelen;
char *buf;
char *bufend;
{
	char *colon;

	name[0] = '\0';

	buf = skipwhite(buf, bufend);

	colon = findchar(':', buf, bufend);

	/* check for no match */
	if (colon == bufend) return;

	colon = skipwhite_reverse(buf, colon);

	/* handle name overflow, leave space for null termination */
	namelen--;
	if (colon - buf < namelen) namelen = colon - buf;

	memcpy(name, buf, namelen);
	name[namelen] = '\0';
}


void
getheadervalue(value, valuelen, buf, bufend)
char *value;
int valuelen;
char *buf;
char *bufend;
{
	char *colon;

	value[0] = '\0';

	colon = findchar(':', buf, bufend);

	/* check for no match */
	if (colon++ == bufend) return;

	--valuelen;
	if (bufend - colon < valuelen)
		valuelen = bufend - colon;
	if ((buf = memccpy (value, colon, '\n', valuelen)) != NULL)
		*--buf = '\0';
	else
		value[valuelen] = '\0';
}



char *
findchar( c, buf, bufend )
char c;
char *buf;
char *bufend;
{
	while( buf < bufend ) {
		if( *buf == c ) break;
		buf++;
	}

	return( buf );
}



int
matchword( name, buf, bufend )
char *name;
char *buf;
char *bufend;
{
	int namelen;
	register unsigned int c;
	register unsigned int n;
	register int i;

	/* skip leading white space */
	buf = skipwhite( buf, bufend );

	namelen = strlen( name );

	/* check that the field is long enough */
	if( bufend - buf < namelen ) return( 0 );

	/* make sure the name matches; ignore case */
	for( i = namelen; i > 0; i-- ) {
		c = *buf++;
		n = *name++;

		if( isupper( c ) ) c = tolower( c );
		if( isupper( n ) ) n = tolower( n );

		if( c != n ) return( 0 );
	}

	/* make sure the rest is white space... */
	buf = skipwhite( buf, bufend );

	/* if the rest of the buffer was white space, then we had a match! */
	return( buf == bufend );
}



char *
skipwhite( buf, bufend )
char *buf;
char *bufend;
{
	while( buf < bufend ) {
		switch( *buf ) {
		case ' ':
		case '\t':
			buf++;
			continue;

		case '\n':
			/* skip line folding */
			if( &buf[1] < bufend &&
				(buf[1] == ' ' || buf[1] == '\t') )
			{
				buf += 2;
				continue;
			}
			buf++;
			break;

		default:
			break;
		}

		break;
	}

	return( buf );
}

char *
skipwhite2( buf )
unsigned char *buf;
{
	while ((*buf != '\0') && isspace(*buf))
		buf++;
	return((char *)buf);
}

char *
skipwhite_reverse( buf, bufend )
char *buf;
char *bufend;
{
	while( buf < bufend ) {
		switch( *--bufend ) {
		case ' ':
		case '\t':
		case '\n':
			continue;

		default:
			return( bufend + 1);
		}
	}

	return( buf );
}


/*
 * find the end of an RFC822 field.  This is any eol that is not
 * followed by a continuation field (space or tab)
 */

char *
findend_of_field( buf, bufend )
char *buf;
char *bufend;
{
	char *eol;
	while( buf < bufend ) {
		/* start at beginning of next line */
		eol = findchar( '\n', buf, bufend ) + 1;

		/* check for the end of the buffer */
		if( eol >= bufend ) {
			buf = bufend;
			break;
		}

		/* advance to the next line */
		buf = eol;

		/* check for the lack of a line fold */
		if( (*eol != ' ') && (*eol != '\t') ) {
			break;
		}
	}

	return( buf );
}



/*
 * allocate space for a new name, initialize it, and return a pointer to it
 */

char *
newname( buf, bufend )
char *buf;
char *bufend;
{
	char *name;
	char *namebase;
	char *end;
	char *eol;

	namebase = name = ck_malloc( bufend - buf + 1 );
	
	/*
	 * copy characters in, deleteting extra white space at the
	 * edges.
	 */
	while (buf < bufend) {
		buf = skipwhite(buf, bufend);
		eol = findchar('\n', buf, bufend);
		end = skipwhite_reverse(buf, eol);

		while (buf < end) {
			*name++ = *buf++;
		}
		
	}
	*name++ = '\0';

	return (namebase);
}

/*
 * Strip an address of all host names, both UUCP and Internet-style.
 * Assumes that all UUCP names pile up on the left, and all Internet-style
 * names pile up on the right.
 * Modifies the string that it is passed.
 */
char *
striphosts(addr,new)
	char *addr;
{
	register char *cp, *cp2;

	if ((cp = strrchr(addr,'!')) != NULL)
		cp++;
	else
		cp = addr;
	/*
	 * Now strip off all Internet-type
	 * hosts.
	 */
	if ((cp2 = strchr(cp, '%')) == NULL)
		cp2 = strchr(cp, '@');
	if (cp2 != NULL)
		*cp2 = '\0';
	return(cp);
}

char *
get_recipients(m,name)
struct msg *m;
char *name;
{
	char *value;

	value = findheaderlist(name, m->mo_hdr);
	if (value == NULL)
		return (NULL);

	return (metoo_usermap (value));
}

/*
 * Put together the address list, making sure there are no duplicates.  If
 * "metoo" is not set, make sure the user isn't added to the address list.
 * If "allnet" is set, ignore hostnames so that "foo@bar1" and "foo@bar2"
 * are equivalent.
 */
char *
metoo_usermap (value)
char *value;
{
	char *new;
	char *name;
	char token[BUFSIZ];
	char *namehost = NULL;
	struct link_obj *group = NULL;
	register int metoo;
	register int allnet;
	register char *p_token;
#ifdef	SVR4
	struct utsname unamebuf;
#endif	SVR4

	/* remove all RFC 822 information: () comments, <>, etc */
	name = skin_arpa (value);
	ck_free (value);
	if ((value = name) == NULL)
		return (value);

	metoo = (mt_value ("metoo") != NULL);
	allnet = (mt_value ("allnet") != NULL);
	/*
	 * If allnet is set, we want to compare just "user".  If allnet
	 * isn't set, we want to compare "user@hostname".
	 */
	if (!allnet)
	{
		/* append "@hostname" to get "user@hostname" */
		strcpy (token, glob.g_myname);
		strcat (token, "@");
#ifdef	SVR4
		uname(&unamebuf);
		strcat(token, unamebuf.nodename);
		namehost = ck_strdup (token);
#else
		gethostname (strchr(token,'\0'), MAXHOSTNAMELEN);
		namehost = ck_strdup (token);
#endif	SVR4
	}
	while (name = gettoken (name, token, sizeof(token)))
	{
		p_token = token;
		if (*p_token == '\0')
			continue;

		new = NULL;
		if (!metoo)
		{
			if (allnet)
			{
				/* strip off the host such that foo@bar,
				 * foo@bar1, and bar2!foo  are equivalent.
				 */
				new = ck_strdup (p_token);
				p_token = striphosts (new);
			}
		}

		DP(("name=%s\n", p_token));

		/*
		 * This is a complicated "if" condition, but basically, it is
		 * split into two parts.  If "metoo" is set, then it adds the
		 * address to the address list (group).  Otherwise, if allnet
		 * is set, it adds the address if the address without the host
		 * doesn't match the user name or any of the alternate names; if 
		 * allnet isn't set, it adds the address if the address with
		 * the host doesn't match the user name or any alternate names.
		 * p_token contains something like "foo" if allnet is set and
		 * "foo@bar" if allnet is not set.
		 */ 
		if (metoo || (strcasecmp (glob.g_myname, p_token) &&
		    (!hash_method.hm_test (glob.g_alternates, p_token)) &&
		    ((namehost == NULL) || strcasecmp (namehost, p_token))))
		{
			/* eliminate any duplicated names */
			p_token = ck_strdup (token); 
			if (link_add (&group, p_token, strcmp) == NULL)
				ck_free (p_token);
		}

		ck_free (new);
	}

	ck_free (namehost);
	ck_free (value);

	if (group == NULL)
		return (NULL);

	/* build the final name list which each name is separated by ", " */
	value = make_nmlist (group);

	/* free up the list of names */
	link_enumerate (group, link_free, ck_free);

	return (value);
}

int
is_sender(m)
struct msg *m;
{
	char *new = NULL;
	int result;
	char *name;

	if ((name = m->mo_from) == NULL)
		return (0);

	if (mt_value("allnet")) {
		new = ck_strdup(name);
		name = striphosts(new);
	}

	result = (strcasecmp(glob.g_myname, name) == 0);

	ck_free(new);

	return (result);
}


#ifdef	never
#ifdef	RFC_MIME
encode_header (s1)
char *s1;
{
	char *encode;
	char encoding;
	char *charset;

	encode = check_encode(s1, len, FALSE, FALSE, &charset);
	if (strcmp (encode, DO_QUOTED_PRINTABLE) == 0)
		encoding = 'Q';
	else if (strcmp (encode, DO_BASE64) == 0)
		encoding = 'B';
	else
		return;

	sprintf (buf, "=?%s?%c?%s?=", charset, encoding, qpenc_string(s1));
}

decode_header (s1)
{
}
#endif	RFC_MIME
#endif	never

/* manage the list of fields whose values are frequently updated */
int
save_field (name)
char *name;
{
	void *result;
	static void *g_save;
	extern char Msg_From[];	
	extern char Msg_Content_Len[];
	extern char Msg_Content_Type[];
	extern char Msg_Status[];
	extern char *header_name();

	if (g_save == NULL)
	{
		g_save = hash_method.hm_alloc ();
		hash_method.hm_add(g_save, Msg_From, NULL, 0);
		hash_method.hm_add(g_save, Msg_Content_Len, NULL, 0);
		hash_method.hm_add(g_save, Msg_Content_Type, NULL, 0);
		hash_method.hm_add(g_save, Msg_Status, NULL, 0);
		hash_method.hm_add(g_save, header_name(NULL,AT_LEN), NULL, 0);
#ifndef	RFC_MIME
		hash_method.hm_add(g_save, header_name(NULL,AT_LINES), NULL, 0);
#endif	RFC_MIME
	}

	result = hash_method.hm_test(g_save, name);
	return (result != NULL);
}

/* manage the alternate name list */
void
add_alternates(name)
char *name;
{
	if(! hash_method.hm_test(glob.g_alternates, name)) {
		/* name is not already there... */
		hash_method.hm_add(glob.g_alternates, name, NULL, 0);
	}
}

/* manage the alias list */
void
add_alias(name, value)
char *name;
char *value;
{
	char *old;

	/* aliases to the same name get appended to the list */
	old = hash_method.hm_test(glob.g_alias, name);
	if (old) {
		char *new;
		int size;

		size = strlen(value) + strlen(old) + 2;
		new = ck_malloc(size);
		sprintf(new, "%s %s", value, old);

		/* delete any old bindings for the name */
		hash_method.hm_delete(glob.g_alias, name);

		/* add the new binding */
		hash_method.hm_add(glob.g_alias, name, new, size);

		/* free up the temporary space */
		ck_free(new);
	} else {
		/* add the new binding */
		hash_method.hm_add(glob.g_alias, name, value, strlen(value) +1);
	}
}


/* manage the retain/ignore list */
void
add_ignore(name)
char *name;
{
	if(! hash_method.hm_test(glob.g_ignore, name)) {
		/* name is not already there... */
		hash_method.hm_add(glob.g_ignore, name, NULL, 0);
	}
}

void
add_retain(name)
char *name;
{
	if(! hash_method.hm_test(glob.g_retain, name)) {
		hash_method.hm_add(glob.g_retain, name, NULL, 0);
	}
	glob.g_nretained++;
}


int
ignore_field(name)
char *name;
{
	void *ptr;
	int invert;
	void *result;

	if (glob.g_nretained) {
		ptr = glob.g_retain;
		invert = 1;
	} else {
		ptr = glob.g_ignore;
		invert = 0;
	}

	result = hash_method.hm_test(ptr, name);

	if (invert) return (result == NULL);
	return (result != NULL);
}

int
alternates_enumerate (func, param)
int (*func)();
caddr_t param;
{
	return (hash_method.hm_enumerate (glob.g_alternates, func, param));
}

int
alias_enumerate (func, param)
int (*func)();
caddr_t param;
{
	return (hash_method.hm_enumerate (glob.g_alias, func, param));
}

int
retain_enumerate (func, param)
int (*func)();
caddr_t param;
{
	return (hash_method.hm_enumerate (glob.g_retain, func, param));
}

int
ignore_enumerate(func, param)
int (*func)();
caddr_t param;
{
	return (hash_method.hm_enumerate (glob.g_ignore, func, param));
}

void
alternates_free(void)
{
	hash_method.hm_free (glob.g_alternates);
}

void
alias_free(void)
{
	hash_method.hm_free(glob.g_alias);
}

void
retain_free(free)
{
	hash_method.hm_free(glob.g_retain);
}

void
ignore_free(void)
{
	hash_method.hm_free(glob.g_ignore);
}
