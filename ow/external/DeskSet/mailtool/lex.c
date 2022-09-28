#ifndef lint
static	char *sccsid = "@(#)lex.c 3.2 92/12/07 SMI";
#endif
/* from Mail/lex.c @(#)lex.c 1.25 89/09/04 SMI */
/* from Mail S5R2 1.3 */

#include "def.h"
#include "glob_mail.h"
#include "main.h"
#include "mle.h"
#include <sys/stat.h>

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Lexical processing of commands.
 */

FILE *input;
int sourcing;
int cond;


/*
 * Interpret user commands one by one.  If standard input is not a tty,
 * print no prompt.
 */

commands()
{
	void stop();
	register int n;
	char linebuf[LINESIZE];
	char line[LINESIZE];

	for (;;) {
top:
		/*
		 * Read a line of commands from the current input
		 * and handle end of file specially.
		 */

		n = 0;
		linebuf[0] = '\0';
		for (;;) {
			if (readline(input, line) <= 0) {
				if (n != 0)
					break;
				if (sourcing) {
					unstack();
					goto more;
				}
				return;
			}
			if ((n = strlen(line)) == 0)
				break;
			n--;
			if (line[n] != '\\')
				break;
			line[n++] = ' ';
			if (n > LINESIZE - strlen(linebuf) - 1)
				break;
			strcat(linebuf, line);
		}
		n = LINESIZE - strlen(linebuf) - 1;
		if (strlen(line) > n) {
			fprintf(stderr,
		gettext("Line plus continuation line too long:\n\t%s\n\nplus\n\t%s\n"),
			    linebuf, line);
			if (sourcing) {
				unstack();
				goto more;
			}
			return;
		}
		strncat(linebuf, line, n);
		if (execute(linebuf, 0))
			return;
more:		;
	}
}

/*
 * Execute a single command.  If the command executed
 * is "quit," then return non-zero so that the caller
 * will know to return back to main, if he cares.
 * Contxt is non-zero if called while composing mail.
 */

execute(linebuf, contxt)
	char linebuf[];
{
	char word[LINESIZE];
	char *arglist[MAXARGC];
	struct cmd *com;
	register char *cp, *cp2;
	register int c;
	int muvec[2];
	int e;
	int newcmd = 0;

	/*
	 * Strip the white space away from the beginning
	 * of the command, then scan out a word, which
	 * consists of anything except digits and white space.
	 *
	 * Handle ! escapes differently to get the correct
	 * lexical conventions.
	 */

	cp = linebuf;
	while (*cp && strchr(" \t", *cp))
		cp++;
	/*
	 * Throw away comment lines here.
	 */
	if (*cp == '#') {
		if (*++cp != '-') {
			return(0);
		}
		/* the special newcmd header -- "#-" */
		newcmd = 1;

		/* strip whitespace again */
		while (*++cp && strchr(" \t", *cp));
	}
	cp2 = word;
	while (*cp && !strchr(" \t0123456789$^.:/-+*'\"", *cp))
		*cp2++ = *cp++;
	*cp2 = '\0';

	/*
	 * Look up the command; if not found, complain.
	 * We ignore blank lines to eliminate confusion.
	 */

	if (equal(word, ""))
		return(0);

	com = lex(word);
	if (com == NONE) {
		if (newcmd) {
			/* this command is OK not to be found; that way
			 * we can extend the .mailrc file with new
			 * commands and not kill old mail and mailtool
			 * programs.
			 */
			return(0);
		}
		fprintf(stderr,"Unknown command: \"%s\"\n", word);
		if (sourcing)
			unstack();
		return(1);
	}

	/*
	 * See if we should execute the command -- if a conditional
	 * we always execute it, otherwise, check the state of cond.
	 */

	if ((com->c_argtype & F) == 0) {
		if (cond == CSEND || cond == CTTY )
		{
			return(0);
		}

#ifdef undef
		if (cond == CRCV && !rcvmode || cond == CSEND && rcvmode ||
		    cond == CTTY && !intty || cond == CNOTTY && intty)
		{
			return(0);
		}
#endif undef
	}

	/*
	 * Process the arguments to the command, depending
	 * on the type he expects.  Default to an error.
	 * If we are sourcing an interactive command, it's
	 * an error.
	 */

	e = 1;
	switch (com->c_argtype & ~(F)) {
	case STRLIST:
		/*
		 * Just the straight string, with
		 * leading blanks removed.
		 */
		while (strchr(" \t", *cp))
			cp++;
		e = (*com->c_func)(cp);
		break;

	case RAWLIST:
		/*
		 * A vector of strings, in shell style.
		 */
		if ((c = getrawlist(cp, arglist,
				sizeof arglist / sizeof *arglist)) < 0)
			break;
		if (c < com->c_minargs) {
			fprintf(stderr,gettext("%s requires at least %d arg(s)\n"),
				com->c_name, com->c_minargs);
			break;
		}
		if (c > com->c_maxargs) {
			fprintf(stderr,gettext("%s takes no more than %d arg(s)\n"),
				com->c_name, com->c_maxargs);
			break;
		}
		e = (*com->c_func)(arglist);
		freerawlist(arglist);
		break;

	case NOLIST:
		/*
		 * Just the constant zero, for exiting,
		 * eg.
		 */
		e = (*com->c_func)(0);
		break;

	default:
		fprintf(stderr,gettext("Unknown argtype %#x"), com->c_argtype);
		mt_done(1);
	}

	/*
	 * Exit the current source file on
	 * error.
	 */

	if (e && sourcing)
		unstack();

	/* ZZZ:katin: should we return an error here? */
	return(e);
}


/*
 * Find the correct command in the command table corresponding
 * to the passed command "word"
 */

struct cmd *
lex(word)
	char word[];
{
	register struct cmd *cp;
	extern struct cmd cmdtab[];

	for (cp = &cmdtab[0]; cp->c_name != NOSTR; cp++)
		if (isprefix(word, cp->c_name))
			return(cp);
	return(NONE);
}

/*
 * Determine if as1 is a valid prefix of as2.
 * Return true if yep.
 */

isprefix(as1, as2)
	char *as1, *as2;
{
	register char *s1, *s2;

	s1 = as1;
	s2 = as2;
	while (*s1++ == *s2)
		if (*s2++ == '\0')
			return(1);
	return(*--s1 == '\0');
}

/*
 * Load a file of user definitions.
 */
load(name)
	char *name;
{
	register FILE *in, *oldin;

	if ((in = fopen(name, "r")) == NULL)
		return(-1);
	oldin = input;
	input = in;
	sourcing = 0;
	commands();
	input = oldin;
	fclose(in);
	return(0);
}
