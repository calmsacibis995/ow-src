#ident  "@(#)defaults.c	26.22    96/10/31 SMI"

/*
 *      (c) Copyright 1989 Sun Microsystems, Inc.
 */

/*
 *      Sun design patents pending in the U.S. and foreign countries. See
 *      LEGAL_NOTICE file for terms of the license.
 */

#ifdef SYSV
#include <sys/types.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#ifdef OW_I18N_L4
#include <sys/param.h>
#endif

#include "i18n.h"
#include "ollocale.h"
#include "olwm.h"
#include "defaults.h"
#include "globals.h"
#include "resources.h"

#define FALLBACK_FONT	"fixed"

/* Global Data */
extern Bool FallbackMode;		/* Declared in olwm.c */

/*
 * GetUserDefaults
 *
 * Get RESOURCE_MANAGER string from server; if none, then load from
 * $HOME/.Xdefaults.  If XENVIRONMENT names a file, load and merge it.
 * Otherwise, load $HOME/.Xdefaults-hostname and merge it.  See
 * Xlib/XGetDflt.c.  We could use that code if it weren't Xlib-private.
 */
XrmDatabase
GetUserDefaults(dpy)
    Display	*dpy;
{
    XrmDatabase serverDB = NULL;
    XrmDatabase fileDB = NULL;
    char filename[1024];
    unsigned long nitems, remain;
    char *rsrcstr;
    char *homedir = getenv("HOME");
    char *envfile = getenv("XENVIRONMENT");
    char hostname[100];
/*	
**	Not used
    int namelen;
*/

    rsrcstr = GetWindowProperty(dpy, RootWindow(dpy, 0), XA_RESOURCE_MANAGER,
	0L, 100000000L, /* REMIND: use ENTIRE_CONTENTS */
	XA_STRING, 0L, &nitems, &remain);

    if (rsrcstr == NULL) {
	if (homedir != NULL) {
	    (void) strcpy(filename, homedir);
	    (void) strcat(filename, "/.Xdefaults");
	    serverDB = XrmGetFileDatabase(filename);
	}
    } else {
	serverDB = XrmGetStringDatabase(rsrcstr);
	XFree(rsrcstr);
    }

    /* Now try XENVIRONMENT or $HOME/.Xdefaults-hostname. */

    if (envfile == NULL) {
	if (homedir != NULL) {
	    (void) strcpy(filename, homedir);
	    (void) strcat(filename, "/.Xdefaults-");
/*
**	gethostname only takes two parameters. New compiler is more fussy.
	    if (0 == gethostname(hostname, sizeof(hostname), &namelen)) {
*/
	    if (0 == gethostname(hostname, sizeof(hostname))) {
		(void) strcat(filename, hostname);
		fileDB = XrmGetFileDatabase(filename);
	    }
	}
    } else {
	fileDB = XrmGetFileDatabase(envfile);
    }

    if (fileDB != NULL)
	XrmMergeDatabases(fileDB, &serverDB);

    /*
     * Special case for dealing with the "FallbackMode". If this flag is
     * turned on, then we're forced into a C locale, and we'll have
     * to force to a font we know we've got ("fixed").
     */
    if(FallbackMode) {
	XrmPutStringResource(&serverDB, "OpenWindows.TitleFont", FALLBACK_FONT);
	XrmPutStringResource(&serverDB, "OpenWindows.TextFont", FALLBACK_FONT);
	XrmPutStringResource(&serverDB, "OpenWindows.ButtonFont", FALLBACK_FONT);
	XrmPutStringResource(&serverDB, "OpenWindows.IconFont", FALLBACK_FONT);

	XrmPutStringResource(&serverDB, "OpenWindows.BasicLocale", "C");
	XrmPutStringResource(&serverDB, "OpenWindows.DisplayLang", "C");
	XrmPutStringResource(&serverDB, "OpenWindows.InputLang", "C");
	XrmPutStringResource(&serverDB, "OpenWindows.NumericFormat", "C");
	XrmPutStringResource(&serverDB, "OpenWindows.TimeFormat", "C");

	XrmPutStringResource(&serverDB, "OpenWindows.CharacterSet", 
				ISO_LATIN_1);	
    }

    return serverDB;
}


/* 
 * GetAppDefaults
 *
 * Gets the app-defaults file and return a database of its contents.  If we 
 * are running internationalized, looks in the following places
 *
 *	$OPENWINHOME/lib/locale/<locale>/app-defaults
 *	/usr/lib/X11/app-defaults/<locale>
 *	$OPENWINHOME/lib/app-defaults
 *	/usr/lib/X11/app-defaults
 *
 * If we are not running internationalized, the entries with <locale> are 
 * ignored.  Returns NULL if no app-defaults file is found.
 *
 * REMIND: this should use XFILESEARCHPATH.
 */
 
XrmDatabase
GetAppDefaults()
{
    XrmDatabase appDB = NULL;
    char filename[1024];
    char *openwinhome = getenv("OPENWINHOME");

#ifdef OW_I18N_L3
    char *locale;

    locale = GRV.lc_basic.locale;
    if (locale != NULL) {
	if (openwinhome != NULL) {
	    (void) sprintf(filename, "%s/lib/locale/%s/app-defaults/Olwm",
			   openwinhome, locale);
	    appDB = XrmGetFileDatabase(filename);
	    if (appDB != NULL)
		return appDB;
	}

	(void) sprintf(filename, "/usr/lib/X11/app-defaults/%s/Olwm", locale);
	appDB = XrmGetFileDatabase(filename);
	if (appDB != NULL)
	    return appDB;
    }
#endif

    if (openwinhome != NULL) {
	(void) strcpy(filename, openwinhome);
	(void) strcat(filename, "/lib/app-defaults/Olwm");
	appDB = XrmGetFileDatabase(filename);
	if (appDB != NULL)
	    return appDB;
    }

    appDB = XrmGetFileDatabase("/usr/lib/X11/app-defaults/Olwm");
    return appDB;
}


/* ===== global functions ================================================= */

/* 
 * GetDefaults
 *
 * XXX - this has been turned into just a call to InitGlobals().  Does it 
 * still need to exist?
 */
void
GetDefaults(dpy, commandlineDB)
    Display *dpy;
    XrmDatabase commandlineDB;
{
    InitGlobals(dpy, commandlineDB);
}
