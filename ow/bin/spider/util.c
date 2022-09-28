#pragma indent "@(#)util.c	1.4 92/06/07	SMI"

/*
 *	Spider
 *
 *	(c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *	(c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *
 *	See copyright.h for the terms of the copyright.
 *
 */

/*
 * misc utility funcs
 */

#include	"defs.h"
#include	"globals.h"
#ifndef KITLESS
#include	<sys/file.h>
#endif /* KITLESS */
#ifdef XAW
#include	"xaw_ui.h"
#endif /* XAW */
#include	<stdlib.h>
#include	<ctype.h>
#include	<string.h>
#include	<pwd.h>

#define	NUM_RETRIES	5

int	replayTime = 200;

#ifndef XVIEW
/*
 * Get the current PRIMARY selection.
 *
 * This is a pretty gross hack, but it works...
 */

char *
get_selection(void)
{
	static Atom	selection = (Atom) 0;
	static Atom	target = (Atom) 0;
	Window		win;
	unsigned char  *prop;
	XEvent		ev;
	XSelectionEvent	*sev;
	Atom		type;
	int		format;
	unsigned long	elmts,
			left;
	int		retry = 0;

	if (!selection)	{
		selection = XInternAtom(dpy, "PRIMARY", False);
		target = XInternAtom(dpy, "STRING", False);
	}

	win = XGetSelectionOwner(dpy, selection);

	if (win == None) {	/* nobody owns it */
		return NULL;
	}

#ifdef XAW
	{
	String		str;
	XawTextPosition	start,
			end;
	Arg		args[1];
	XawTextBlock	text;

	XtSetArg(args[0], XtNstring, &str);
	if (helptext && win == XtWindow(helptext)) {
		XawTextGetSelectionPos(helptext, &start, &end);
		XawTextSourceRead(XawTextGetSource(helptext),
			start, &text, end - start);
	} else if (win == XtWindow(file)) {
		XawTextGetSelectionPos(file, &start, &end);
		XawTextSourceRead(XawTextGetSource(file),
			start, &text, end - start);
	} else {
		goto skip;
	}
	prop = (unsigned char *)malloc(end - start + 1);
	(void)strncpy(prop, text.ptr, end - start);
	prop[end - start] = '\0';
	return (char *)prop;
	}

	skip:
#endif /* XAW */

	XConvertSelection(dpy, selection, target, None, table, CurrentTime);

	XSync(dpy, 0);

	/* wait for notification */
	while(XCheckTypedEvent(dpy, SelectionNotify, &ev) == False) {
		XSync(dpy, 0);
		if (retry++ == NUM_RETRIES) {
			return NULL;
		}
		sleep(1);
	}

	sev = (XSelectionEvent *)&ev;

	if (sev->property == None) {	/* nothing to get */
		return NULL;
	}

	(void)XGetWindowProperty(dpy, table, sev->property,
		0L, 1024L,
		False, AnyPropertyType, &type, &format,
		&elmts, &left, &prop);
	
	assert(type == target);

	if (format != 8) {	/* only want chars */
		return NULL;
	}

	return (char *)prop;
}
#endif /* XVIEW */


#ifdef XAW
char	*helpDir;

/*
 * See if all the help files are there.
 */
 
Bool
can_get_help_files(char helpfiles[6][256]) {
	int	i;

	(void)sprintf(helpfiles[0], "%s/doc.intro",    helpDir);
	(void)sprintf(helpfiles[1], "%s/doc.rules",    helpDir);
	(void)sprintf(helpfiles[2], "%s/doc.controls", helpDir);
	(void)sprintf(helpfiles[3], "%s/doc.examples", helpDir);
	(void)sprintf(helpfiles[4], "%s/doc.misc",     helpDir);
	(void)sprintf(helpfiles[5], "%s/doc.summary",  helpDir);

	for (i = 0; i < 6; i++)	{
		if (access(helpfiles[i], R_OK) == -1) {
			return False;
		}
	}
	return True;
}
#endif /* XAW */


char *
remove_newlines(char *str) {
	char	*newstr;
	char	*n;

	/* pad it generously to provide for tilde expansion */
	n = newstr = (char *)calloc((unsigned)(strlen(str) + 256), 1);

	/* remove leading whitespace */
	while (isspace(*str)) {
		str++;
	}

	/* tilde expansion */
	if (*str == '~') {
		/* user */
		if (*(str + 1) == '/') {
			(void)strcpy(newstr, getenv("HOME"));
		} else {
			char		 uname[20],
					*t;
			struct passwd	*pwd;
			int		 len;

			t = strchr(str + 1, '/');
			if (t) {
				len = t - str - 1;
			} else {
				len = strlen(str);
			}
			(void)strncpy(uname, str + 1, len);
			uname[len] = '\0';
			if (pwd = getpwnam(uname)) {
				(void)strcpy(newstr, pwd->pw_dir);
				str += len;
			}
		}
		n += strlen(newstr);
		str++;
	}

	/* strip newlines in selection */
	while (*str) {
		if (*str != '\n') {
			*n++ = *str;
		}
		str++;
	}
	*n = '\0';
	return newstr;
}

#ifndef XVIEW
void
delay(void) {
	if (replayTime) {
		usleep((unsigned)replayTime);
	}
}


#ifdef	LOCAL_USLEEP

#include <signal.h>
#include <X11/Xos.h>		/* for (sys/)time.h */

usleep(long value) {
	void			stopme();
	struct itimerval	ntval,
				otval;

	ntval.it_interval.tv_sec  = 0;
	ntval.it_interval.tv_usec = 0;
	ntval.it_value.tv_sec     = 0;
	ntval.it_value.tv_usec    = value;
	signal(SIGALRM, stopme);
	setitimer(ITIMER_REAL, &ntval, &otval);
	pause();
}


void
stopme(void) {
	signal(SIGALRM, SIG_DFL);
}
#endif	/* LOCAL_USLEEP */

#endif /* XVIEW */


#ifdef	LOCAL_STRDUP
char *
strdup(char *s) {
	return strcpy(malloc((unsigned) strlen(s) + 1), s);
}
#endif	/* LOCAL_STRDUP */
