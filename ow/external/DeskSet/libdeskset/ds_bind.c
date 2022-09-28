#ifndef lint
static  char sccsid[] = "%Zn%ds_bind.c 1.8 91/03/12 (c) 1988 Sun Micro";
#endif

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>		/* MAXPATHLEN */
#include <string.h>
#ifndef SVR4
#include <strings.h>
#endif  SVR4
#include <ctype.h>
#include <pwd.h>

#include <pixrect/pixrect.h>
#include "ds_bind.h"

#define DELIMITER	','	/* Field delimiter */
#define MAXMAGIC 	128	/* Maximum number of magic descriptions */
#define MAXCOLORMAPSIZE	255
#define GLYPH_WIDTH	32
#define BOOLEAN		unsigned char

#define EMAGIC		"Too many magic numbers\n"
#define EUNKNOWNMAGIC	"Unknown magic number %s\n"
#define ENOICON		"Must attach icon to rule entry \"%s\"\n"
#define EBADCOLOR	"Bad color: %s\n"
#define ENOMEMORY	"Out of memory\n"

static char *Magic[MAXMAGIC];	/* Magic Entry */
static short Moffset[MAXMAGIC];	/* Offset to description */
static int Nmagic;		/* Number of entries */

B_BIND B_bind[MAXBIND];		/* Bound list */
int B_nbind;			/* Number of bound objects */
int B_nappbind;			/* Number of sorted application icons */

extern char *sys_errlist[];
extern char *malloc();
extern char	*strchr();
extern char	*getenv();

b_read_bind(red, green, blue, ncolors)	/* Read in magic and filetype */
	u_char red[];
	u_char green[];
	u_char blue[];
	int *ncolors;
{
	register FILE *fp;	/* File pointer */
	char buf[255];		/* File buffer */
	register char *b_p;	/* Buffer pointer */
	register int i;		/* Index */
	static int compare();
#ifdef SVR4
	extern char *getenv();
#else
	extern char *sprintf(), *getenv();
#endif SVR4
	int lineno=0;


	/* Read in /etc/magic stuff */

	if (fp=fopen("/etc/magic", "r"))
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			/* Skip comments and continuation lines... */

			if (buf[0] == '#' || buf[0] == '>')
				continue;

			/* Get description; the Magic array will contain these,
			 * the Moffset array contains offsets to start of entry.
			 * Thus when a description is used we can go back to
			 * the entry and parse the rule.
			 */

			b_p = buf;      /* Skip to start of message field. */
			for (i = 0; i < 3; i++) {
				if (b_p == NULL) break;
				while (*b_p != ' ' && *b_p != '\t') b_p++;
				if (b_p == NULL) break;
				while (*b_p == ' ' || *b_p == '\t') b_p++;
			}
			if (b_p == NULL) continue;

			b_p[strlen(b_p)-1] = NULL;	/* Get rid of newline */
			Moffset[Nmagic] = b_p - buf;	/* Start of entry */
			if (Nmagic == MAXMAGIC)
			{
				(void)fprintf(stderr, EMAGIC);
				break;
			}
			if ((Magic[Nmagic] = (char *)malloc((unsigned)strlen(buf)+1))==NULL)
				break;

			(void)strcpy(Magic[Nmagic], buf);
			Magic[Nmagic] += Moffset[Nmagic];
			Nmagic++;
		}
		(void)fclose(fp);
	}


	/* Read bind entries */

	(void)sprintf(buf, "%s/.filetype", getenv("HOME"));
	if ((fp=fopen(buf, "r")) == NULL)
		fp=fopen("/etc/filetype", "r");

	if (fp)
	{
		while (fgets(buf, sizeof(buf), fp))
		{
			lineno++;
			if ((buf[0] != '#') &&		/* Skip comment */
			    !(
			      (lineno == 1) && 		/* Skip magic# on */
			      (atoi(buf) > 0)		/* 1st line       */
			     )
			   )
				if (make_entry(buf, &B_bind[B_nbind],
					       red, green, blue, ncolors)==0)
				{
					B_nbind++;
					if (B_nbind == MAXBIND)
						break;
				}
		}
		(void)fclose(fp);

		/* The bind array is in two parts; the first contains the 
		 * "pattern = icon" to provide applications for icons.  This
		 * list is sorted alphabetically.
		 * The second part contains the 
		 *	"reg expr = [icon +] [application]".  
		 * Since regular expressions are used, this list can't be 
		 * sorted.  It typically contains data file definitions.  It
		 * also contains magic numbers instead of regular expressions.
		 */
		qsort((char *)B_bind, B_nbind, sizeof(B_BIND), compare);
		for (i = 0; i<B_nbind; i++)
			if (B_bind[i].magic || 
			    b_regex_in_string(B_bind[i].pattern))
				break;
		B_nappbind = i;
	}


	/* Free up magic number array */

	for (i=0; i<Nmagic; i++)
		free(Magic[i]-Moffset[i]);
}


static
make_entry(buf, bind, red, green, blue, ncolors) /* Create a bind entry */
	char *buf;			/* Pattern, magic, app, icon */
	register B_BIND *bind;		/* Bind structure */
	u_char red[], green[], blue[];
	int *ncolors;
{
	register int i;			/* Index */
	register char *b_p, *s_p;	/* Temp pointers */
	char errmsg[80];		/* load icon error message */
	char iconpath[MAXPATHLEN];	/* Path to the bind entries icon. */
	int r, g, b;			/* Icon colors (if any) */
	BOOLEAN more;			/* Another field? */
	static B_MAGIC *make_magic();


	if ((bind->buf=(char *)malloc((unsigned)strlen(buf)+1))==NULL)
		return(-1);

	(void)strcpy(bind->buf, buf);

	/* Pattern */
	b_p = bind->buf;
	s_p = b_p;
	while (*b_p && *b_p != DELIMITER)
		b_p++;
	*b_p = NULL;
	bind->pattern = s_p;

	/* Magic number */
	s_p = ++b_p;
	while (*b_p && *b_p != DELIMITER)
		b_p++;
	*b_p = NULL;

	bind->magic = NULL;
	if (*s_p)
	{
		/* Match magic description with magic definition */

		for (i=0; i<Nmagic; i++)
		{
			if (strcmp(Magic[i], s_p) == 0)
			{
				if ((bind->magic = (B_MAGIC *)make_magic(Magic[i]-Moffset[i]))==NULL)
					return(-1);
				bind->magic->str = s_p;
				break;
			}
		}
		if (i==Nmagic)
		{
			(void)fprintf(stderr, EUNKNOWNMAGIC, s_p);
			return(-1);
		}
	}

	/* Application */
	s_p = ++b_p;
	while (*b_p && *b_p != DELIMITER)
		b_p++;
	*b_p = NULL;
	bind->application = s_p;


	/* Icon */
	s_p = ++b_p;
	while (*b_p && *b_p != DELIMITER && *b_p != '\n')
		b_p++;
	*b_p = NULL;
	if (*(bind->iconfile = s_p) == NULL)
	{
		(void)fprintf(stderr, ENOICON, bind->buf);
		return(-1);
	}
	if (*s_p)
	{
		ds_expand_pathname(s_p, iconpath);
		if ((bind->icon = (Pixrect *)icon_load_mpr(iconpath, errmsg)) == NULL)
		{
			(void)fprintf(stderr, "%s", errmsg);
			if (bind->magic) free((char *)bind->magic);
			return(-1);
		}
	}
	else
		bind->icon = NULL;
	

	/* Foreground Color */

	s_p = ++b_p;
	while (*b_p && *b_p != DELIMITER && *b_p != '\n')
		b_p++;
	more = *b_p == DELIMITER;
	*b_p = NULL;

	if (*s_p == NULL)
		bind->color = 0;
	else
	{
		if ((sscanf(s_p, "%d %d %d", &r, &g, &b) != 3) || 
		     *ncolors == MAXCOLORMAPSIZE-1)
		{
			(void)fprintf(stderr, EBADCOLOR, (int)s_p);
			return(0);
		}

		/* Check color map; have we used these color before? */
		for (i=0; i < *ncolors; i++)
			if (r == red[i] && b == blue[i] && g == green[i])
				break;

		bind->color = i;
		if (i==*ncolors)
		{
			/* New color; add them to our colormap */
			red[i] = (u_char)r;
			green[i] = (u_char)g;
			blue[i] = (u_char)b;
			(*ncolors)++;
		}
	}


	/* Get print method (if any) */

	if (!more)
		return(0);

	s_p = ++b_p;
	while (*b_p && *b_p != DELIMITER && *b_p != '\n')
		b_p++;
	more = *b_p == DELIMITER;
	*b_p = NULL;
	bind->print_method = s_p;


	/* Get image method */
	if (!more)
		return(0);
	s_p = ++b_p;
	while (*b_p && *b_p != DELIMITER && *b_p != '\n')
		b_p++;
	more = *b_p == DELIMITER;
	*b_p = NULL;
	bind->image = (u_char)s_p;

	if (!more)
		return(0);
	s_p = ++b_p;
	while (*b_p && *b_p != DELIMITER && *b_p != '\n')
		b_p++;
	more = *b_p == DELIMITER;
	*b_p = NULL;
	bind->doc_id = s_p;

	if (!more)
		return(0);
	s_p = ++b_p;
	while (*b_p && *b_p != DELIMITER && *b_p != '\n')
		b_p++;
	more = *b_p == DELIMITER;
	*b_p = NULL;
	bind->filter = s_p;

	return(0);
}


static B_MAGIC *
make_magic(buf)			/* Build magic number parser */
	char *buf;		/* Raw magic number description */
{
	static char *types[NTYPES] = { "long", "string", "byte", "short" };
	static char ops[NOPS] = { '=', '>', '<', '=', 'x' };
	register char *p, *p2, *p3;/* Temp pointer */
	int i;			/* Index */
	B_MAGIC *magic;	/* Magic number description */
	char *e_p;		/* Error message pointer */
	char *getstr();
	long atolo();
	

	if ((magic=(B_MAGIC *)malloc(sizeof(B_MAGIC)))==NULL)
		return(NULL);

	p = buf;

	p2 = strchr(p, '\t');	/* OFFSET... */
	if (!p2)
	{
		e_p = "No tab after offset";
		goto bad;
	}
	magic->off = atoi(p);

	while(*p2 == '\t')	/* TYPE... */
		p2++;
	p = p2;
	p2 = strchr(p, '\t');
	if(p2 == NULL)
	{
		e_p = "No tab after type";
		goto bad;
	}
	*p2++ = NULL;
	p3 = strchr(p, '&');	/* MASK... */
	if (p3)
	{
		*p3++ = NULL;
		magic->mask = atoi(p3);
	} else
		magic->mask = 0L;

	for (i = 0; i < NTYPES; i++)
		if (strcmp(p, types[i]) == 0)
			break;
	if (i==NTYPES)
	{
		e_p = "Illegal type";
		goto bad;
	}
	magic->type = i;

	*(p2-1) = '\t';		/* Put tab back */
	while(*p2 == '\t')	/* TYPE... */
		p2++;
	p = p2;			/* OP VALUE... */
	p2 = strchr(p, '\t');
	if (!p2)
	{
		e_p = "No tab after value";
		goto bad;
	}
	*p2 = NULL;
	if (magic->type != STR)
	{
		/* Get operator; missing op is assumed to be '=' */

		for (i = 0; i < NOPS; i++)
			if (*p == ops[i])
			{
				magic->opcode = i;
				p++;
				break;
			}
		if (i==NOPS)
			magic->opcode = EQ;
	}

	if (magic->opcode != ANY) {
		if (magic->type == STR)
		{
			if ((magic->value.str = getstr(p))==NULL)
			{
				e_p = ENOMEMORY;
				goto bad;
			}
		}
		else
			magic->value.num = atolo(p);
	}
	*p2 = '\t';


	return(magic);

bad:
	(void)fprintf(stderr, e_p);
	if (magic)
		free((char *)magic);
	return(NULL);
}


static char *
getstr(s)				/* Interpret special chars in string */
	register char *s;
{
	static char *store;
	register char *p;
	register char c;
	register int val;

	if ((store = (char *)malloc((unsigned)strlen(s) + 1)) == NULL)
		return(NULL);

	p = store;
	while((c = *s++) != '\0') {
		if(c == '\\') {
			switch(c = *s++) {

			case '\0':
				goto out;

			default:
				*p++ = c;
				break;

			case 'n':
				*p++ = '\n';
				break;

			case 'r':
				*p++ = '\r';
				break;

			case 'b':
				*p++ = '\b';
				break;

			case 't':
				*p++ = '\t';
				break;

			case 'f':
				*p++ = '\f';
				break;

			case 'v':
				*p++ = '\v';
				break;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				val = c - '0';
				c = *s++;  /* try for 2 */
				if(c >= '0' && c <= '7') {
					val = (val<<3) | (c - '0');
					c = *s++;  /* try for 3 */
					if(c >= '0' && c <= '7')
						val = (val<<3) | (c-'0');
					else
						--s;
				}
				else
					--s;
				*p++ = val;
				break;
			}
		} else
			*p++ = c;
	}
out:
	*p = '\0';
	return(store);
}


static long
atolo(s)				/* Convert ASCII to long */
	register char *s;
{
	register char *fmt = "%ld";
	auto long j = 0L;

	if(*s == '0') {
		s++;
		if(*s == 'x') {
			s++;
			fmt = "%lx";
		} else
			fmt = "%lo";
	}
	(void)sscanf(s, fmt, &j);
	return(j);
}

b_check_bind(name, icon)
	register char *name;		/* File name */
	int *icon;			/* Return index into Bind array */
{
	register int j;			/* Index */
	int fd = 0;			/* File descriptor */
	char buf[64];			/* First n bytes of file */
	char *fname;			/* Filename less path (if any) */

	*icon = -1;			/* Nothing found */
	if (fname = strrchr(name, '/'))
		fname++;
	else
		fname = name;

	for (j=B_nappbind; j<B_nbind; j++)
		if (*B_bind[j].pattern)
		{
			/* Name regular expression */

			if (b_match(fname, B_bind[j].pattern)==1)
			{
				*icon = j;
				break;
			}
		}
		else
		{
			/* Magic number; open and read once! */

			if (fd == 0)
			{
				/* If it fails to open or read then quit.
				 * By opening, we have updated the file's
				 * access time; we could reset this by
				 * remembering the access time and restoring
				 * it with utime().  But I would like to avoid
				 * two extra syscalls on each file.
				 */
				if ((fd=open(name, 0))==-1)
					return;
				if (read(fd, buf, sizeof(buf))==0) {
					(void)close(fd);
					return;
				}
			}

			if (check_magic(buf, B_bind[j].magic))
			{
				/* It matches! */
				*icon = j;
				break;
			}
		}

	if (fd)
		(void)close(fd);
}


static
check_magic(buf, magic)			/* Return TRUE if same magic number */
	char *buf;			/* First n bytes of file */
	register B_MAGIC *magic;	/* Magic number description */
{
	register char *p;		/* Temp pointer */
	long val;			/* Numeric value */

	p = &buf[magic->off];

	switch(magic->type)
	{
	case LONG:
		val = (*(long *) p);
		break;

	case STR:
		return(strncmp(p, magic->value.str, strlen(magic->value.str))==0);

	case ABYTE:
		val = (long)(*(unsigned char *) p);
		break;

	case SHORT:
		val = (long)(*(unsigned short *) p);
		break;
	}

	if (magic->mask)
		val &= magic->mask;

	switch (magic->opcode & ~SUB)
	{
	case EQ:
		return(val == magic->value.num);

	case GT:
		return(val <= magic->value.num);

	case LT:
		return(val >= magic->value.num);
	}

	return(0);
}

/*
 * Lexical definitions.
 *
 * All lexical space is allocated dynamically.
 * The eighth bit of characters is used to prevent recognition,
 * and eventually stripped.
 */
#define	QUOTE 	0200		/* Eighth char bit used internally for 'ing */
#define	TRIM	0177		/* Mask to strip quote bit */

static char *Missing = "Missing %c";

b_match(s, p)
	register char *s, *p;
{
	register int scc;
	int ok, lc;
	int c, cc;

	/* globbed = 1; */
	for (;;) {
		scc = *s++ & TRIM;
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
				/*fv_putmsg(1, Missing, ']', 0);*/
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
				if (b_match(s, p))
					return (1);
			return (0);

		case 0:
			return (scc == 0);

		default:
			if ((c & TRIM) != scc)
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
			/*fv_putmsg(1, Missing, ']', 0);*/
			return(-1);
		}
		continue;
	}
pend:
	if (brclev || !*pe) {
		/*fv_putmsg(1, Missing, '}', 0);*/
		return(-1);
	}
	for (pl = pm = p; pm <= pe; pm++)
	switch (*pm & (QUOTE|TRIM)) {

	case '{':
		brclev++;
		continue;

	case '}':
		if (brclev) {
			brclev--;
			continue;
		}
		goto doit;

	case ','|QUOTE:
	case ',':
		if (brclev)
			continue;
doit:
		savec = *pm;
		*pm = 0;
		(void) strcpy(lm, pl);
		(void) strcat(restbuf, pe + 1);
		*pm = savec;
		/*if (s == 0) {
			sgpathp = gpathp;
			expand(restbuf);
			gpathp = sgpathp;
			*gpathp = 0;
		} else */if (b_match(s, restbuf))
			return (1);
		/* sort(); */
		pl = pm + 1;
		continue;

	case '[':
		for (pm++; *pm && *pm != ']'; pm++)
			continue;
		if (!*pm) {
			/*fv_putmsg(1, Missing, ']', 0);*/
			return(-1);
		}
		continue;
	}
	return (0);
}


b_regex_in_string(s_p)                 /* Regular expression in string? */
	register char *s_p;
{
	while (*s_p)
	{
		if (*s_p == '*' || *s_p == '[' || *s_p == ']' ||
		    *s_p == '{' || *s_p == '}' || *s_p == '?')
			break;
		s_p++;
	}
	return(*s_p);
}


static int
compare(b1, b2)
	B_BIND *b1, *b2;
{
	int r1, r2;			/* Regular expressions? */

	/* Magic numbers and regular expressions have the lowest sort order */

	if (b1->magic && b2->magic)
		return(0);
	else if (b1->magic)
		return(1);
	else if (b2->magic)
		return(-1);
	r1 = b_regex_in_string(b1->pattern);
	r2 = b_regex_in_string(b2->pattern);
	if (r1 && r2)
		return(0);
	else if (r1)
		return(1);
	else if (r2)
		return(-1);

	return(strcmp(b1->pattern, b2->pattern));
}
