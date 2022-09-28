/* Copyright (c) 1991 by Sun Microsystems, Inc. */
#ident	"@(#)radio_rcfile.c	1.3	92/06/24 SMI"

/* .radiorc file access subroutines for Radio Free Ethernet */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <malloc.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <unistd.h>
#include <sys/fcntl.h>

#include "radio.h"

/* The following flag is used to compress multiple blank lines on output */
static int	lastblank;


/*
 * Open the rc file with the given mode: O_RDONLY or O_WRONLY.
 *
 * If mode is (O_WRONLY | O_EXCL), then the file (if it exists) is
 * unlinked first.  This is used for updating as follows:
 *
 *	in = radiorc_open(O_RDONLY);
 *	out = radiorc_open(O_WRONLY | O_EXCL);
 *	if (out < 0) ...error...
 *	if (in >= 0) {
 *		...read input lines...[modify]...write output...
 *	}
 *	...write any parameters not already modified...
 *	radiorc_close(in); radiorc_close(out);
 */
int
radiorc_open(
	int		mode)
{
	char*		env;
	char		name[MAXPATHLEN];

	/* Open $HOME/.radiorc */
	env = getenv("HOME");
	SPRINTF(name, "%s%s%s",
	     env != NULL ? env : "", env != NULL ? "/" : "", RADIO_RCFILE);

	/* If special flag, unlink and create a new one */
	if (mode == (O_WRONLY | O_EXCL)) {
		/* Unlink the file, if possible */
		if ((access(name, W_OK) < 0) || (unlink(name) < 0)) {
			if (errno != ENOENT)
				return (-1);
		}
		return (open(name, O_WRONLY | O_CREAT | O_EXCL, 0666));
	}

	lastblank = TRUE;	/* set flag to remove leading blank lines */

	/* Otherwise, honor the specified open mode */
	return (open(name, mode, 0666));
}

/* Close the rc file */
void
radiorc_close(
	int		fd)
{
	if (fd >= 0)
		(void) close(fd);
}


/* Write out an rc file entry for given item */
void
radiorc_putstr(
	int			fd,
	Radio_cmdtbl*		tbl,
	char*			str)
{
	char			msg[512];

	/* Only write each item out once (except for preset station values) */
	if (tbl->seen++ == 0) {
		SPRINTF(msg, "%s = %s\n", tbl->keyword, str);
		(void) write(fd, msg, strlen(msg));
		lastblank = FALSE;
	}
}

/* Write out a numeric rc file entry */
void
radiorc_putval(
	int			fd,
	Radio_cmdtbl*		tbl,
	int			val)
{
	char			value[32];

	SPRINTF(value, "%d", val);
	radiorc_putstr(fd, tbl, value);
}

/* Write out a parsed rc file entry (used for comments and unknown flags) */
void
radiorc_putcomment(
	int			fd,
	char**			ptr)
{
	int			i;
	char			msg[1024];

	msg[0] = '\0';
	if (*ptr != NULL) {
		(void) strcpy(msg, *ptr);
		while (*++ptr != NULL) {
			if (**ptr != '\n')
				(void) strcat(msg, " ");
			(void) strcat(msg, *ptr);
		}
	}

	/* Make sure there's a newline at the end of this line */
	i = strlen(msg);
	if (msg[i - 1] != '\n') {
		(void) strcat(msg, "\n");
		i++;
	}
	if (i > 1) {
		lastblank = FALSE;
	} else {
		if (lastblank)
			return;		/* don't write multiple newlines */
		lastblank = TRUE;
	}
	(void) write(fd, msg, i);
}

/* Write out a blank line */
void
radiorc_putblank(
	int			fd)
{
	if (lastblank)
		return;			/* don't write multiple newlines */
	lastblank = TRUE;
	(void) write(fd, "\n", 1);
}
