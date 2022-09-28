/* Copyright (c) 1991 by Sun Microsystems, Inc. */
#ident	"@(#)radio_subr.c	1.11	91/07/09 SMI"

/* Support subroutines for Radio Free Ethernet */

#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/filio.h>
#include <sys/time.h>

#include "radio.h"
#include "radio_network.h"


/* Define increment in which to allocate buffers */
#define	CHAR_INCREMENT	(256)
#define	ARGV_INCREMENT	(16)


/* Make a copy of a string item */
char*
copy_string(
	char*			str)
{
	int			i;
	char*			s;

	/* Skip leading spaces, return "" if NULL string */
	if (str == NULL) {
		str = "";
	} else {
		while (isspace(*str))
			str++;
	}
	i = strlen(str);

	/* Remove trailing white space */
	while ((i > 0) && (isspace(str[i - 1]))) {
		i--;
	}

	/* Malloc space for the string, copy it, and return it */
	s = malloc(i + 1);
	(void) strncpy(s, str, i);
	s[i] = '\0';
	return (s);
}

/* Reset a string item, freeing the old value, if any */
void
replace_string(
	char**			valp,
	char*			str)
{
	if (*valp != NULL)
		(void) free(*valp);
	*valp = copy_string(str);
}


/* Return TRUE if input file descriptor has data available */
int
input_ready(
	int		infd)
{
	fd_set		fdset;
	int		nfds;
	struct timeval	tmo;

	nfds = radio_fdset(NULL, &fdset);
	FD_SET(infd, &fdset);

	tmo.tv_sec = 0;
	tmo.tv_usec = 0;
	if (select(nfds, &fdset, (fd_set*)NULL, (fd_set*)NULL, &tmo) > 0)
		return (TRUE);
	return (FALSE);
}

/* Read the given input file */
static char*
readinput(
	char*		buf,
	unsigned	len,
	int		infd)
{
	int		i;

	i = read(infd, buf, len - 1);
	if (i <= 0)
		return (NULL);
	buf[i] = '\0';
	return (buf);
}


/*
 * Parse a line of input from an input file, using the specified
 * command table, and execute commands found in the command line.
 * If nowait is TRUE, process only input that is immediately available.
 * If nowait is FALSE, hang until some input, then return.
 * The 'prog' string is used for printing error messages.
 *
 * Returns -1 if end-of-file on input.  Otherwise, returns 0.
 */
int
parse_input(
	int		infd,
	Radio_cmdtbl*	cmds,
	int		nowait,
	char*		prog)
{
	char**		arglist;
	char*		err;

	/* Loop while input data is available */
	while (!nowait || input_ready(infd)) {
		/* Lex a line of input */
		arglist = lex_line(infd);
		if (arglist == NULL)		/* end-of-file */
			return (-1);

		/* Parse commands from this line */
		err = parse_cmds(arglist, cmds);
		if (err != NULL) {
			if (err != (char*)-1)
				(void) fprintf(stderr, "%s: %s %s\n", prog,
				    "Unrecognized command:", err);
		}
		(void) free((char*)arglist);

		/* If nowait is FALSE, return in case START was processed */
		if (!nowait)
			return (0);
	}
	return (0);
}

/*
 * Read input and split it into lexical units.
 * All white space separates lexical units.
 * Embedded newlines are returned as individual lexical units ("\n").
 * Comment lines, starting with '#', are returned as single tokens.
 * Newlines can be escaped with backslash (except in comments).
 *
 * Returns an argv pointer to malloc'ed storage.
 * Returns NULL on input end-of-file.
 */
char**
lex_line(
	int		infd)
{
	char*		cbuf;
	char**		pbuf;
	unsigned	cctr;
	unsigned	pctr;
	char**		retbuf;
	unsigned	retsiz;
	char		buf[513];
	char*		bp;
	int		i;
	int		newline;
	int		addnewline;
	int		incomment;

	/* Read a line of input */
	bp = readinput(buf, sizeof (buf), infd);
	if (bp == NULL)
		return (NULL);			/* end-of-file */

	/* Allocate initial buffers */
	cbuf = (char*)malloc(CHAR_INCREMENT);
	*cbuf = '\0';
	cctr = 0;
	pbuf = (char**) malloc(ARGV_INCREMENT * sizeof (char*));
	*pbuf = 0;
	pctr = 0;
	if ((cbuf == NULL) || (pbuf == NULL))
		return (NULL);

	/* Lex input lines until a newline terminator or eof */
	newline = TRUE;
	incomment = FALSE;
	addnewline = FALSE;
	for (;
	    (*bp != '\0') ||
	    ((bp != buf) && !newline &&
	    ((bp = readinput(buf, sizeof (buf), infd)) != NULL));
	    bp++) {
		/* If processing a comment line, preserve its format */
		if (incomment && (*bp != '\n'))
			goto append;

		/* Check for an escaped newline */
		if ((*bp == '\\') && (bp[1] == '\n')) {
			bp++;		/* skip newline on next increment */
			continue;
		}

		/* If printable character, get ready to append it */
		if (addnewline || !isspace(*bp)) {

			/* If this is the first char, set a string offset */
			if ((cctr == 0) || (cbuf[cctr - 1] == '\0')) {
				/* Copy lines starting with '#' unchanged */
				if (newline && (*bp == '#')) {
					incomment = TRUE;
				}
				pbuf[pctr++] = (char*)cctr;
				if ((pctr % ARGV_INCREMENT) == 0) {
					/* Extend the pointer array */
					pbuf = (char**) realloc((char*)pbuf,
					    (int)((pctr + ARGV_INCREMENT) *
					    sizeof (char*)));
					if (pbuf == NULL) {
						(void) free((char*)cbuf);
						return (NULL);
					}
				}
			}
			newline = FALSE;
		} else {
			/* If end of line, set flag for next loop */
			if (*bp == '\n') {
				addnewline = TRUE;
				incomment = FALSE;
			}
			newline = FALSE;
			*bp = '\0';		/* Set to append a NULL */

			/* Whitespace is ignored, except it ends a string */
			if ((cctr == 0) || (cbuf[cctr - 1] == '\0'))
				goto checkadd;
		}

append:
		/* Append current char, extend array when necessary */
		cbuf[cctr++] = *bp;
		if ((cctr % CHAR_INCREMENT) == 0) {
			cbuf = (char*)realloc((char*)cbuf,
			    (int)(cctr + CHAR_INCREMENT));
			if (cbuf == NULL) {
				(void) free((char*)pbuf);
				return (NULL);
			}
		}
		cbuf[cctr] = '\0';	/* terminate partial strings */

checkadd:
		/* If newline, make it a token by itself */
		if (addnewline) {
			if (*bp == '\0') {
				/* First time around, scan newline again */
				*bp-- = '\n';		/* append \n next */
			} else {
				/* Second time, terminate "\n" string */
				*bp = '\0';		/* terminate token */
				addnewline = FALSE;	/* then continue */
				newline = TRUE;
				goto append;
			}
		}
	}

	/* Now recombine the parsed data into a single malloc */
	retsiz = ++cctr + ((pctr + 2) * sizeof (char*));
	retbuf = (char**) malloc((int)retsiz);
	if (retbuf != NULL) {
		/* Copy strings to end of buffer */
		bp = (char*)(retbuf + pctr + 2);
		(void) memmove(bp, cbuf, cctr);

		/* Copy pointers, adding the buffer offset */
		for (i = 0; i < pctr; i++)
			retbuf[i] = (unsigned)pbuf[i] + bp;

		/* Write two consecutive NULL pointers at the end */
		retbuf[i++] = NULL;
		retbuf[i] = NULL;	/* just in case no final \n */
	}

	(void) free((char*)cbuf);
	(void) free((char*)pbuf);
	return (retbuf);
}

/*
 * Read multiple lines of input and split it into lexical units
 * with line boundaries.  Uses lex_line() to parse input, then
 * marches through removing newline tokens.  The resulting argv
 * pointer may have multiple NULL-terminated lists of tokens.
 * Two NULL pointers in a row signifies the end of input.
 *
 * Returns an argv pointer to malloc'ed storage.
 * Returns NULL on input end-of-file.
 */
char**
lex_multiline(
	int		infd)
{
	char**		ptr;
	char**		eptr;
	char**		wptr;

	/* Read input and divide up into tokens */
	ptr = lex_line(infd);
	if (ptr == NULL)
		return (NULL);

	/* Convert newline tokens to NULL; collapse adjacent newlines */
	wptr = ptr;
	for (eptr = ptr; *eptr != NULL; eptr++) {
		if (strcmp(*eptr, "\n") == 0) {
			/*
			 * If this is the first token, skip it.
			 * If the previous token was a newline, skip it.
			 * Otherwise, set a terminating NULL pointer.
			 */
			if ((wptr == ptr) || (wptr[-1] == NULL)) {
				continue;
			}
			*wptr++ = NULL;
		} else {
			*wptr++ = *eptr;
		}
	}
	if (wptr[-1] != NULL)
		*wptr++ = NULL;		/* terminate the previous line */
	*wptr = NULL;			/* terminate the whole mess */
	return (ptr);
}


/* Return FALSE if 'off', TRUE if 'on' */
int
is_on(
	char*		str)
{
	/* If neither on nor off, default to on */
	if ((str[0] != 'o') && (str[0] != 'O'))
		return (TRUE);
	if ((str[1] == 'f') || (str[1] == 'F'))
		return (FALSE);
	return (TRUE);
}

/* Case insensitive string compare.  Returns 0 if match */
static int
strncmp_case(
	char*		str1,
	char*		str2,
	int		limit)
{
	int		i;
	char		c1;
	char		c2;

	for (i = 0; i < limit; i++) {
		c1 = *str1++;
		c2 = *str2++;
		if ((c1 == '\0') || (c2 == '\0')) {
			return (0);	/* strings matched */
		}
		if (islower(c1))
			c1 = toupper(c1);
		if (islower(c2))
			c2 = toupper(c2);
		if (c1 != c2)
			return (1);
	}
	return (0);
}

/*
 * Match commands from a command list and execute the handler function
 * Command syntax is:
 *	key=arg
 * Spaces are allowed anywhere between key, equal sign, and arg.
 * Commands that start with '#' are ignored.
 *
 * If parse error, a pointer to the token in error is returned.
 * If the action routine returns an error, -1 is returned.
 * Otherwise, NULL is returned.
 */
char*
parse_cmds(
	char**		argv,
	Radio_cmdtbl*	cmds)
{
	char*		str;
	char*		s;
	char*		equal;
	int		i;

	/* Cycle through each command string */
	equal = NULL;
	while ((str = *argv++) != NULL) {
		/* Ignore comments and newline tokens */
		if ((str[0] == '#') || (strcmp(str, "\n") == 0))
			continue;

		/* Get ptr to arg */
		s = str;
		while (*s != '\0') {
			if (*s == '=') {
				equal = s;
				*s++ = '\0';
				/* If no arg in this token, try the next */
				if ((*s == '\0') && (*argv != NULL) &&
				    (**argv != '\n')) {
					s = *argv++;
				}
				break;		/* s now points to arg */
			}
			s++;

			/* If no equal sign, scan ahead for it */
			if ((*s == '\0') &&
			    (*argv != NULL) && (**argv == '=')) {
				s = *argv++;	/* continue scanning */
			}
		}
		/* str is NULL-terminated, s points to argument, if any */

		/* Match string against command list */
		for (i = 0; cmds[i].keyword != NULL; i++) {
			if (strncmp_case(cmds[i].keyword, str, strlen(str))
			    != 0)
				continue;

			/* If match, call function.  Returns FALSE if error. */
			if (!(*cmds[i].func)(s)) {
				if (equal != NULL)
					*equal = '=';	/* Restore equal sign */
				return ((char*)-1);
			}
			break;
		}
		/* Restore equal sign, if deleted */
		if (equal != NULL)
			*equal = '=';

		/* If no match, return keyword string */
		if (cmds[i].keyword == NULL) {
			return (str);
		}
	}
	return (NULL);
}
