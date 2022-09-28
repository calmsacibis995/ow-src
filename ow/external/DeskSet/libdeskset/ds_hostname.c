
#ident 	"@(#)ds_hostname.c	1.2 91/01/09 Copyr 1990 Sun Micro"

#include <sys/param.h> /* MAXPATHLEN (include types.h if removed) */
#ifdef SVR4
#include <dirent.h>    /* MAXNAMLEN */
#include <netdb.h>     /* MAXHOSTNAMELEN */
#else
#include <sys/dir.h>   /* MAXNAMLEN */
#endif SVR4
#include <sys/stat.h>
#include <X11/X.h> 
#include <X11/Xlib.h> 

#define	TRUE	1
#define	FALSE	0

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/* ds_hostname.c */

static char	client_hostname[MAXHOSTNAMELEN + 4];
static int	ds_hostname_set = NULL;

char *
ds_hostname(dpy)

Display	*dpy;

{
	char		hostname[MAXHOSTNAMELEN];
	char		*display = DisplayString(dpy);
	char		*scanner = display;
	char		savechar;

	if (ds_hostname_set)
		return(client_hostname);

	gethostname(hostname, MAXHOSTNAMELEN);

	while (*scanner)
		scanner++;

	while (*scanner != ':')
		scanner--;

	*scanner = NULL;

	if (strcmp(display, hostname) && 
	    strcmp(display, "localhost") &&
	    strcmp(display, "unix") &&
	    strcmp(display, "")
	   )
		sprintf(client_hostname, " [%s] ", hostname);
	else
		strcpy(client_hostname, "");

	*scanner = ':';

	ds_hostname_set = TRUE;
	return(client_hostname);
}
