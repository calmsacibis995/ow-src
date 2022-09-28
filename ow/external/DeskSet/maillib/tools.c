/* @(#)tools.c	3.3 - 93/11/23 */

/* tools.c -- auxiliary functions */

#include <stdio.h>
#include <string.h>
#include "ck_strings.h"

static char *phrase(char *name, int token, int comma);

/*
 * delete arpa RFC822 style information - () comments, <>, etc.
 *
 * implicitly assumes that processing makes the name get smaller,
 * never larger.
 */
char *
phrase(
	char *name,
	int token,	/* true if we do only one token, 0 for whole string */
	int comma	/* true means insist on terminating comma && token */
) {

	register char c;
	register char *cp, *cp2;
	char *bufend, *nbufp;
	int gotlt, lastsp, didq;
	int nesting;
	int extra;

	if (name == (char *) 0) {
		return((char *) 0);
	}

	/* count comma's; we may need up to one extra space per comma... */
	extra = 1;
	cp = name;
	for (;;) {
		cp = strchr(cp, ',');

		if (!cp) break;

		if (*++cp != ' ' && *cp != '\t') extra++;
	}

	nbufp = ck_malloc(strlen(name) + extra);

	gotlt = 0;
	lastsp = 0;
	bufend = nbufp;
	for (cp = name, cp2 = bufend; (c = *cp++) != 0;) {
		switch (c) {
		case '(':
			/*
				Start of a comment, ignore it.
			*/
			nesting = 1;
			while ((c = *cp) != 0) {
				cp++;
				switch(c) {
				case '\\':
					if (*cp == 0) goto outcm;
					cp++;
					break;
				case '(':
					nesting++;
					break;
				case ')':
					--nesting;
					break;
				}
				if (nesting <= 0) break;
			}
		outcm:
			lastsp = 0;
			break;

		case '"':
			/*
				Start a quoted string.
				Copy it in its entirety.
			*/
			didq = 0;
			while ((c = *cp) != 0) {
				cp++;
				switch (c) {
				case '\\':
					if ((c = *cp) == 0) goto outqs;
					cp++;
					break;
				case '"':
					goto outqs;
				}
				if (gotlt == 0 || gotlt == '<') {
					if (lastsp) {
						lastsp = 0;
						*cp2++ = ' ';
					}
					if (!didq) {
						*cp2++ = '"';
						didq++;
					}
					*cp2++ = c;
				}
			}
		outqs:
			if (didq)
				*cp2++ = '"';
			lastsp = 0;
			break;

		case ' ':
		case '\t':
		case '\n':
			if (token && (!comma || c == '\n')) {
			done:
				cp[-1] = 0;
				return cp;
			}
			lastsp = 1;
			break;

		case ',':
			*cp2++ = c;
			if (gotlt != '<') {
				if (token)
					goto done;
				bufend = cp2 + 1;
				gotlt = 0;
			}
			break;

		case '<':
			cp2 = bufend;
			gotlt = c;
			lastsp = 0;
			break;

		case '>':
			if (gotlt == '<') {
				gotlt = c;
				break;
			}

			/* FALLTHROUGH . . . */

		default:
			if (gotlt == 0 || gotlt == '<') {
				if (lastsp) {
					lastsp = 0;
					*cp2++ = ' ';
				}
				*cp2++ = c;
			}
			break;
		}
	}
	*cp2 = 0;

#ifdef DEBUG
	if ( (unsigned)(cp2 - nbufp) >= strlen(name) + extra) {
		printf( "skin_arpa: blown buffer\n" );
		abort();
	}
#endif DEBUG

	return(nbufp);
}



char *
skin_arpa(
	char *name
) {
	return(phrase(name, 0, 0));


}




