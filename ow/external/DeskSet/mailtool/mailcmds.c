/* @(#)mailcmds.c	3.2 - 92/06/07 */

/* mailcmds -- implement reading of mail startup files */


#include "def.h"
#include "glob_mail.h"
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include "mle.h"
#include "../maillib/ck_strings.h"

#define DEBUG_FLAG mt_debugging
extern int DEBUG_FLAG;
#include "debug.h"

extern char *mt_value();


/*
 * Set the list of alternate names.
 */
alternates(namelist)
register char **namelist;
{
	while (*namelist != NULL)
		add_alternates(*namelist++);
	return(0);
}


/*
 * Expand file names like echo
 */

echo(argv)
register char **argv;
{
	register char *cp;
	int neednl = 0;

	while (*argv != NOSTR) {
		cp = *argv++;
		if ((cp = expand(cp)) != NOSTR) {
			neednl++;
			fprintf(stderr,"%s", cp);
			if (*argv!=NOSTR)
				putchar(' ');

			ck_free(cp);
		}
	}
	if (neednl)
		putc('\n', stderr);
	return(0);
}


/*
 * Implement 'else'.  This is pretty simple -- we just
 * flip over the conditional flag.
 */

elsecmd()
{

	switch (cond) {
	case CANY:
		fprintf(stderr, gettext("\"Else\" without matching \"if\"\n"));
		return(1);

	case CSEND:
		cond = CRCV;
		break;

	case CRCV:
		cond = CSEND;
		break;

	case CTTY:
		cond = CNOTTY;
		break;

	case CNOTTY:
		cond = CTTY;
		break;

	default:
		fprintf(stderr,gettext("invalid condition encountered\n"));
		cond = CANY;
		break;
	}
	return(0);
}

/*
 * End of if statement.  Just set cond back to anything.
 */

endifcmd()
{

	if (cond == CANY) {
		fprintf(stderr,gettext("\"Endif\" without matching \"if\"\n"));
		return(1);
	}
	cond = CANY;
	return(0);
}


/*
 * Put add users to a group.
 */

group(argv)
	char **argv;
{
	int size;
	char *buf;
	char *s;
	char **ap;

	/*
	 * Insert names from the command list into the group.
	 * Who cares if there are duplicates?  They get tossed
	 * later anyway.
	 */

	/* compute the size of the buffer */
	size = 0;
	for (ap = argv+1; *ap != NOSTR; ap++) {
		size += strlen(*ap) +1;
	}
	buf = ck_malloc(size);
	s = buf;
	for (ap = argv+1; *ap != NOSTR; ap++) {
		strcpy(s, *ap);
		s += strlen(s);
		*s++ = ' ';
	}
	*--s = '\0';

	add_alias(argv[0], buf);
	ck_free(buf);
	return(0);
}


/*
 * Conditional commands.  These allow one to parameterize one's
 * .mailrc and do some things if sending, others if receiving.
 */

ifcmd(argv)
	char **argv;
{
	register char *cp;

	if (cond != CANY) {
		fprintf(stderr,gettext("Illegal nested \"if\"\n"));
		return(1);
	}
	cond = CANY;
	cp = argv[0];
	switch (*cp) {
	case 'r': case 'R':
		cond = CRCV;
		break;

	case 's': case 'S':
		cond = CSEND;
		break;

	case 't': case 'T':
		cond = CTTY;
		break;

	default:
		fprintf(stderr,gettext("Unrecognized if-keyword: \"%s\"\n"),
			cp);
		return(1);
	}
	return(0);
}


/*
 * Add the given header fields to the ignored list.
 * If no arguments, print the current list of ignored fields.
 */
igfield(list)
	char *list[];
{
	char **ap;

	for (ap = list; *ap != 0; ap++) {
		DP(("igfield: adding %s\n", *ap));
		add_ignore(*ap);
	}
	return(0);
}


/*
 * Add the given header fields to the retained list.
 * If no arguments, print the current list of retained fields.
 */
retfield(list)
	char *list[];
{
	char **ap;

	for (ap = list; *ap != 0; ap++) {
		DP(("retfield: adding %s\n", *ap));
		add_retain(*ap);
	}
	return(0);
}


/*
 * Change user's working directory.
 */

schdir(str)
	char *str;
{
	register char *cp;
	char cwd[PATHSIZE], file[PATHSIZE];
	static char efile[PATHSIZE];

	for (cp = str; *cp == ' '; cp++)
		;
	if (*cp == '\0') {
		cp = Getf("HOME");
	} else {
		if ((cp = expand(cp)) == NOSTR)
			return(1);
	}
	if (chdir(cp) < 0) {
		perror(cp);
		ck_free(cp);
		return(1);
	}

	ck_free(cp);
	return(0);
}


/*
 * Set or display a variable value.  Syntax is similar to that
 * of csh.
 */

set(arglist)
	char **arglist;
{
	register struct var *vp;
	register char *cp, *cp2;
	char varbuf[LINESIZE], **ap, **p;
	int errs, h, s;

	errs = 0;
	for (ap = arglist; *ap != NOSTR; ap++) {
		cp = *ap;
		cp2 = varbuf;
		while (*cp != '=' && *cp != '\0')
			*cp2++ = *cp++;
		*cp2 = '\0';
		if (*cp == '\0')
			cp = "";
		else
			cp++;
		if (equal(varbuf, "")) {
			fprintf(stderr,gettext("Non-null variable name required\n"));
			errs++;
			continue;
		}
		mt_assign(varbuf, cp);
	}
	return(errs);
}

/*
 * Unset a bunch of variable values.
 */

unset(arglist)
	char **arglist;
{
	register char **ap;

	for (ap = arglist; *ap != NOSTR; ap++)
		(void) mt_deassign(*ap);
	return(0);
}


static	int	ssp = -1;		/* Top of file stack */
struct sstack {
	FILE	*s_file;		/* File we were in. */
	int	s_cond;			/* Saved state of conditionals */
} sstack[NOFILE];

/*
 * Pushdown current input file and switch to a new one.
 * Set the global flag "sourcing" so that others will realize
 * that they are no longer reading from a tty (in all probability).
 */

source(name)
	char name[];
{
	register FILE *fi;
	register char *cp;

	if ((cp = expand(name)) == NOSTR)
		return(1);
	if ((fi = fopen(cp, "r")) == NULL) {
		perror(cp);
		ck_free(cp);
		return(1);
	}
	ck_free(cp);
	if (ssp >= NOFILE - 2) {
		fprintf(stderr,gettext("Too much \"sourcing\" going on.\n"));
		fclose(fi);
		return(1);
	}
	sstack[++ssp].s_file = input;
	sstack[ssp].s_cond = cond;
	cond = CANY;
	input = fi;
	sourcing++;
	return(0);
}

/*
 * Pop the current input back to the previous level.
 * Update the "sourcing" flag as appropriate.
 */

unstack()
{
	if (ssp < 0) {
		fprintf(stderr,gettext("\"Source\" stack over-pop.\n"));
		sourcing = 0;
		return;
	}
	fclose(input);
	if (cond != CANY)
		fprintf(stderr,gettext("Unmatched \"if\"\n"));
	cond = sstack[ssp].s_cond;
	input = sstack[ssp--].s_file;
	if (ssp < 0)
		sourcing = 0;
}

/*
 * Take a file name, possibly with shell meta characters
 * in it and expand it by using "sh -c echo filename"
 * Return the file name as a dynamic string.
 */

char *
expand(name)
	char name[];
{
	char xname[LINESIZE];
	char cmdbuf[LINESIZE];
	register int pid, l;
	register char *cp, *Shell;
	int s, pivec[2];
	struct stat sbuf;

	DP(("expand(%s)=", name));
	if (name[0] == '+' && getfolderdir(cmdbuf) >= 0) {
		sprintf(xname, "%s/%s", cmdbuf, name + 1);
		return(expand(xname));
	}
	if (!strpbrk(name, "~{[*?$`'\"\\")) {
		DP(("%s\n", name));
		return(ck_strdup(name));
	}
	if (pipe(pivec) < 0) {
		perror("pipe");
		return(ck_strdup(name));
	}
	sprintf(cmdbuf, "echo %s", name);
	if ((pid = vfork()) == 0) {
		Shell = mt_value("SHELL");
		if (Shell == NOSTR || *Shell=='\0')
			Shell = SHELL;
		close(pivec[0]);
		close(1);
		dup(pivec[1]);
		close(pivec[1]);
		close(2);
		execlp(Shell, Shell, "-c", cmdbuf, (char *)0);
		_exit(1);
	}
	if (pid == -1) {
		perror("fork");
		close(pivec[0]);
		close(pivec[1]);
		return(NOSTR);
	}
	close(pivec[1]);
	l = read(pivec[0], xname, LINESIZE);
	close(pivec[0]);
	while (wait(&s) != pid);
		;
	s &= 0377;
	if (s != 0 && s != SIGPIPE) {
		fprintf(stderr, gettext("\"Echo\" failed\n"));
		goto err;
	}
	if (l < 0) {
		perror(gettext("read"));
		goto err;
	}
	if (l == 0) {
		fprintf(stderr, gettext("\"%s\": No match\n"), name);
		goto err;
	}
	if (l == LINESIZE) {
		fprintf(stderr, gettext("Buffer overflow expanding \"%s\"\n"),
			name);
		goto err;
	}
	xname[l] = 0;
	for (cp = &xname[l-1]; *cp == '\n' && cp > xname; cp--)
		;
	*++cp = '\0';
	if (strchr(xname, ' ') && stat(xname, &sbuf) < 0) {
		fprintf(stderr, gettext("\"%s\": Ambiguous\n"), name);
		goto err;
	}
	DP(("%s\n", xname));
	return(ck_strdup(xname));

err:
	fflush(stderr);
	return(NOSTR);
}


/*
 * Read up a line from the specified input into the line
 * buffer.  Return the number of characters read.  Do not
 * include the newline at the end.
 */

readline(ibuf, linebuf)
	FILE *ibuf;
	char *linebuf;
{
	register char *cp;
	register int c;
	int seennulls = 0;

	clearerr(ibuf);
	c = getc(ibuf);
	for (cp = linebuf; c != '\n' && c != EOF; c = getc(ibuf)) {
		if (c == 0) {
			if (!seennulls) {
				fprintf(stderr,
			gettext("Mail: ignoring NULL characters in mail\n"));
				seennulls++;
			}
			continue;
		}
		if (cp - linebuf < LINESIZE-2)
			*cp++ = c;
	}
	*cp = 0;
	if (c == EOF && cp == linebuf)
		return(0);
	return(cp - linebuf + 1);
}


/*
 * return the filename associated with "s".  This function always
 * returns a non-null string (no error checking is done on the receiving end)
 */
char *
Getf(s)
register char *s;
{
	register char *cp;
	static char defbuf[PATHSIZE];

	if ((cp = mt_value(s)) && *cp) {
		return(cp);
	} else if (strcmp(s, "MBOX")==0) {
		strcpy(defbuf, Getf("HOME"));
		strcat(defbuf, "/");
		strcat(defbuf, "mbox");
		return(defbuf);
	} else if (strcmp(s, "DEAD")==0) {
		strcpy(defbuf, Getf("HOME"));
		strcat(defbuf, "/");
		strcat(defbuf, "dead.letter");
		return(defbuf);
	} else if (strcmp(s, "MAILRC")==0) {
		strcpy(defbuf, Getf("HOME"));
		strcat(defbuf, "/");
		strcat(defbuf, ".mailrc");
		return(defbuf);
	} else if (strcmp(s, "HOME")==0) {
		/* no recursion allowed! */
		return(".");
	}
	return("DEAD");	/* "cannot happen" */
}


char *
libpath(string)
char *string;
{
	static char buf[100];

#ifdef SVR4
	strcpy(buf, "/etc/mail");
#else
	strcpy(buf, "/usr/lib");
#endif SVR4
	strcat(buf, "/");
	strcat(buf, string);
	return(buf);
}


/*
 * Scan out the list of string arguments, shell style
 * for a RAWLIST.
 */

getrawlist(line, argv, argc)
	char line[];
	char **argv;
	int  argc;
{
	register char **ap, *cp, *cp2;
	char linebuf[LINESIZE], quotec;
	register char **last;

	ap = argv;
	cp = line;
	last = argv + argc - 1;
	while (*cp != '\0') {
		while (*cp && strchr(" \t", *cp))
			cp++;
		cp2 = linebuf;
		quotec = 0;
		while (*cp != '\0') {
			if (quotec) {
				if (*cp == quotec) {
					quotec=0;
					cp++;
				} else
					*cp2++ = *cp++;
			} else {
				if (*cp && strchr(" \t", *cp))
					break;
				if (*cp && strchr("'\"", *cp))
					quotec = *cp++;
				else
					*cp2++ = *cp++;
			}
		}
		*cp2 = '\0';
		if (cp2 == linebuf)
			break;
		if (ap >= last) {
			fprintf(stderr,
			"Too many elements in the list; excess discarded\n");
			break;
		}
		*ap++ = ck_strdup(linebuf);
	}
	*ap = NOSTR;
	return(ap-argv);
}


freerawlist(argv)
char **argv;
{
	while(*argv) {
		ck_free(*argv);
		argv++;
	}
}

/*
 * Count the number of arguments in the given string raw list.
 */

argcount(argv)
	char **argv;
{
	register char **ap;

	for (ap = argv; *ap != NOSTR; ap++)
		;	
	return(ap-argv);
}


/*
 * Determine the current folder directory name.
 */
getfolderdir(name)
	char *name;
{
	char *folder;

	if ((folder = mt_value("folder")) == NOSTR)
		return(-1);
	if (*folder == '/')
		strcpy(name, folder);
	else
		sprintf(name, "%s/%s", mt_value("HOME"), folder);
	return(0);
}

