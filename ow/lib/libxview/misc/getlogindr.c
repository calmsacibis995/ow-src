#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)getlogindr.c 20.16 91/07/07 Copyr 1984 Sun Micro";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * xv_getlogindir - Get login directory.  First try HOME environment variable.
 * Next try password file.  Print message if can't get login directory.
 */

#include <stdio.h>
#include <pwd.h>
#include <xview_private/i18n_impl.h>
#include <xview/xv_error.h>

char           *
xv_getlogindir()
{
    extern char    *getlogin(), *getenv();
    extern struct passwd *getpwnam(), *getpwuid();
    struct passwd  *passwdent;
    char           *home, *loginname;

    home = getenv("HOME");
    if (home != NULL)
	return (home);
    loginname = getlogin();
    if (loginname == NULL)
	passwdent = getpwuid(getuid());
    else
	passwdent = getpwnam(loginname);
    if (passwdent == NULL) {
	xv_error(NULL,
		 ERROR_STRING,
		     XV_MSG("xv_getlogindir: couldn't find user in password file"),
		 0);
	return (NULL);
    }
    if (passwdent->pw_dir == NULL) {
	xv_error(NULL,
		 ERROR_STRING,
		     XV_MSG("xv_getlogindir: no home directory in password file"),
		 0);
	return (NULL);
    }
    return (passwdent->pw_dir);
}
