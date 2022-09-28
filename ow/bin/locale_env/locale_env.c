/*
 *      (C) Copyright 1992 Sun Microsystems, Inc. Sun design patents
 *      pending in the U.S. and foreign countries. See LEGAL_NOTICE
 *      file for terms of the license.
 */

#ident "@(#)locale_env.c	1.3 93/03/19; SMI"

#include	<sys/param.h>
#include	<sys/systeminfo.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<locale.h>
#include	<libintl.h>
#include	<string.h>
#include	<X11/Xlib.h>
#include	<X11/Xatom.h>
#include	<X11/Xresource.h>


#define	HOSTNAMELEN		256		/* see manpage sysinfo(2) */
#define	RESOURCE_NAME_MAX	256
#define	DISPLAY_XRM		".display"


extern int	errno;


static XrmOptionDescRec xrmOptions[] = {
{"-display",		DISPLAY_XRM,
				XrmoptionSepArg,(caddr_t) NULL},
{"-lc_basiclocale",	".basicLocale",
				XrmoptionSepArg,(caddr_t) NULL},
{"-lc_inputlang",	".inputLang",
				XrmoptionSepArg,(caddr_t) NULL},
{"-lc_displaylang",	".displayLang",
				XrmoptionSepArg,(caddr_t) NULL},
{"-lc_numeric",		".numericFormat",
				XrmoptionSepArg,(caddr_t) NULL},
{"-lc_timeformat",	".timeFormat",
				XrmoptionSepArg,(caddr_t) NULL},
{"-xrm",		NULL,
				XrmoptionResArg,(caddr_t) NULL}
};
#define nXrmOptions (sizeof(xrmOptions)/sizeof(XrmOptionDescRec))

typedef struct _optionRec {
	const enum {
		OT_NONE,
		OT_BOOL,		/* No args */
		OT_STRINGS
	} type;
	union opt_value {
		Bool	  bool;
		char	**strings;
	} v;
	const char	*name;
} optionRec;

static optionRec option[] = {
#define	OPT_ENV		option[0].v.bool
	{OT_BOOL,	{False},	"-env",	},
#define	OPT_CSH		option[1].v.bool
	{OT_BOOL,	{False},	"-csh"},
#define	OPT_EXEC	option[2].v.strings
	{OT_STRINGS,	NULL,		"-exec"},
#define	OPT_PRINT	option[3].v.bool
	{OT_BOOL,	{False},	"-print"},
#define	OPT_HEADER	option[4].v.bool
	{OT_BOOL,	{False},	"-header"},
#define	OPT_LOCALE_OFFSET	5
	{OT_BOOL,	False,		"-get_basiclocale"},
	{OT_BOOL,	False,		"-get_displaylang"},
	{OT_BOOL,	False,		"-get_numeric"},
	{OT_BOOL,	False,		"-get_timeformat"},
	{OT_BOOL,	False,		"-get_inputlang"},
	{OT_NONE,	False,		NULL}
};
static Bool	OPT_LOCALE = False;

typedef struct _localeInfoRec {
	char		*value;
	const char	*instance;
	const char	*class;
	const char	*envName;
	const int	 posix;
} localeInfoRec;

static localeInfoRec	localeInfo[] = {
#define	OLLC_BASIC_LOCALE	0
	{NULL,	"basicLocale",	"BasicLocale",	"LC_CTYPE",	LC_CTYPE},
#define	OLLC_DISPLAY_LANG	1
	{NULL,	"displayLang",	"DisplayLang",	"LC_MESSAGES",	LC_MESSAGES},
#define	OLLC_NUMERIC		2
	{NULL,	"numericFormat","NumericFormat","LC_NUMERIC",	LC_NUMERIC},
#define	OLLC_TIME_FORMAT	3
	{NULL,	"timeFormat",	"TimeFormat",	"LC_TIME",	LC_TIME},
#define	OLLC_INPUT_LANG		4
	{NULL,	"inputLang",	"InputLang",	NULL,		-2},
#ifdef notdef
	/*
	 * Not yet, do not know what do with it.
	 */
	{NULL,	NULL,		NULL,		"LC_COLLATE",	LC_COLLATE},
	{NULL,	NULL,		NULL,		"LC_MONETARY",	LC_MONETARY},
#endif
	{NULL,	NULL,		NULL,		NULL,		-1}
};

static char		 openWindows[] = "openWindows";
static char		 OpenWindows[] = "OpenWindows";
static char		*myName;
static const char	 myTextDomain[] = "SUNW_WST_LOCALE_ENV";

static Display		*dpy;		/*
					 * This command will not
					 * connect to multiple server
					 * ever!
					 */
static XrmDatabase	 xrmDB = NULL;


static void	constructLocaleInfo();
static void	doEnv();
static void	doExec();
static void	doLocale();
static void	doPrint();
static void	getUserDefaults();
static void	openDisplay(XrmDatabase xrdb);
static void	setupLocale();


main(ac, av)
    int		  ac;
    char	**av;
{
    XrmDatabase		 cmdLineDB = NULL;
    optionRec		*o;
    int			 i;
    int			 j;


    if ((myName = strrchr(av[0], '/')) == NULL)
	myName = av[0];
    else
	myName++;

    setupLocale();

    XrmInitialize();

    XrmParseCommand(&cmdLineDB, xrmOptions, nXrmOptions, openWindows, &ac, av);

    /*
     * Process options which not belong to resources.
     */
    for (i = 1; i < ac; i++) {
	for (j = 0, o = option; o->type != OT_NONE; j++, o++)
	    if (strcmp(o->name, av[i]) == 0) {
		switch (o->type) {
		    case OT_BOOL:
		        o->v.bool = True;
			if (j >= OPT_LOCALE_OFFSET) {
			    if (OPT_LOCALE)
				goto usage;
			    OPT_LOCALE = True;
			}
			break;

		    case OT_STRINGS:
			if (++i >= ac)
			    goto usage;
			o->v.strings = &av[i];
			break;
		}
		break;
	    }
    }
    /*
     * This command only does one thing at a time.
     */
    if (((OPT_ENV == True) + (OPT_LOCALE == True)
         + (OPT_EXEC != NULL) + (OPT_PRINT == True)) != 1)
        goto usage;

    openDisplay(cmdLineDB);

    getUserDefaults();

    XrmMergeDatabases(cmdLineDB, &xrmDB);

    constructLocaleInfo();

    if (OPT_ENV)
	doEnv();
    else if (OPT_LOCALE)
	doLocale();
    else if (OPT_EXEC != NULL) {
	doEnv();
	doExec();
    } else if (OPT_PRINT == True)
	doPrint();
    else
	goto usage;

    return 0;

usage:
    (void) fprintf(stderr, gettext(
"usage: %s [common opt] -env [-csh]\n"
" or    %s [common opt] -exec cmd args...\n"
" or    %s [common opt] {-get_basiclocale, -get_displaylang,\n"
"                                -get_inputlang, -get_numeric, \n"
"                                -get_timeformat}\n"
" or    %s [common opt] -print [-header]\n"
" where common opt is : [-display dpy] [-lc_basiclocale locale]\n"
"                       [-lc_displaylang locale] [-lc_inputlang locale]\n"
"                       [-lc_numeric locale] [-lc_timeformat locale]\n"),
	    myName, myName, myName, myName);

    return 2;
}


static void
setupLocale()
{
    char		 *openwinhome;
    char		  domainPath[MAXPATHLEN + 1];

    (void) setlocale(LC_ALL, "");
    if ((openwinhome = getenv("OPENWINHOME")) == NULL)
	openwinhome = "/usr/openwin";
    (void) sprintf(domainPath, "%s/lib/locale", openwinhome);
    bindtextdomain(myTextDomain, domainPath);
    textdomain(myTextDomain);
}


#define	DISPLAY_ENV	"DISPLAY="

static void
openDisplay(xrdb)
    XrmDatabase		xrdb;
{
    char	  instanceName[MAXNAMELEN + sizeof (DISPLAY_XRM) + 1];
    char	 *display = NULL;
    char	 *display_env;
    XrmValue	  value;
    char	 *type;


    (void) sprintf(instanceName, "%s%s", openWindows, DISPLAY_XRM);

    if (XrmGetResource(xrdb, instanceName, instanceName, &type, &value)) {
	display = (char *) value.addr;
	display_env = malloc(strlen(display) + sizeof (DISPLAY_ENV) + 1);
	(void) sprintf(display_env, "%s%s", DISPLAY_ENV, display);
        (void) putenv(display_env);
    }

    if ((dpy = XOpenDisplay(display)) == NULL) {
	(void) fprintf(stderr, gettext("%s: cann't connect to %s"),
		myName, display == NULL ? "(NULL DISPLAY)" : display);
	exit(1);
    }
}


static void
getUserDefaults()
{
    XrmDatabase		 userDB = NULL;
    XrmDatabase		 envDB = NULL;
    char		*xrdbStr;
    char		*homeDir;
    char		*xenvFile;
    char		 fileName[MAXPATHLEN];
    char		 hostName[HOSTNAMELEN + 1];


    homeDir = getenv("HOME");

    if ((xrdbStr = XResourceManagerString(dpy)) != NULL) {
	userDB = XrmGetStringDatabase((char *) xrdbStr);
    } else {
	if (homeDir != NULL) {
	    (void) sprintf(fileName, "%s/.Xdefaults", homeDir);
	    userDB = XrmGetFileDatabase(fileName);
	}
    }

    /*
     * I guess, we have to try XENVIRONMENT or ~/.Xdefaults-hostname.
     */
    if ((xenvFile = getenv("XENVIRONMENT")) == NULL) {
	if (homeDir != NULL
	 && sysinfo(SI_HOSTNAME, hostName, HOSTNAMELEN) > 0) {
	    (void) sprintf(fileName, "%s/.Xdefaults-%s", homeDir, hostName);
	    envDB = XrmGetFileDatabase(fileName);
	}
    } else
	envDB = XrmGetFileDatabase(xenvFile);

    if (envDB != NULL)
	XrmMergeDatabases(envDB, &userDB);

    if (userDB != NULL)
	XrmMergeDatabases(userDB, &xrmDB);
}


static void
constructLocaleInfo()
{
    localeInfoRec	*li;
    XrmValue		 value;
    char		*type;
    char		 instance[RESOURCE_NAME_MAX + 1];
    char		*instance_name;
    char		 class[RESOURCE_NAME_MAX + 1];
    char		*class_name;
    char		*locale;


    /*
     * Looking for Xrm first.
     */
    (void) sprintf(instance, "%s.", openWindows);
    instance_name = &instance[strlen(openWindows) + 1];
    (void) sprintf(class, "%s.", OpenWindows);
    class_name = &class[strlen(OpenWindows) + 1];

    for (li = localeInfo; li->posix != -1; li++) {
	if (li->instance == NULL)
	    continue;
	(void) strcpy(instance_name, li->instance);
	(void) strcpy(class_name, li->class);
	if (XrmGetResource(xrmDB, instance, class, &type, &value)) {
	    li->value = (char *) value.addr;
	    if (li->posix >= 0)
		(void) setlocale(li->posix, li->value);
	}
    }
    /*
     * For backward compatibility, look for "OpenWindows.numric", if
     * "OpenWidows.numericFormat" is not found.
     */
    li = &localeInfo[OLLC_NUMERIC];
    if (li->value == NULL) {
	(void) strcpy(instance_name, "numeric");
	if (XrmGetResource(xrmDB, instance, class, &type, &value)) {
	    li->value = (char *) value.addr;
	    if (li->posix >= 0)
	        (void) setlocale(li->posix, li->value);
	}
    }

    /*
     * Now look for environment via setlocale to fill rest.
     */
    for (li = localeInfo; li->posix != -1; li++) {
        if (li->value == NULL && li->posix >= 0) {
	    /*
	     * What heck, strdup is not part of the ANSI C nor POSIX (;_;)
	     */
	    locale = setlocale(li->posix, NULL);
	    if ((li->value = malloc(strlen(locale) + 1)) == NULL) {
		(void) fprintf(stderr,
			       gettext("%s: no sufficient memory available\n"),
			       myName);
		exit(2);
	    }
	    (void) strcpy(li->value, locale);
	}
    }

    /*
     * Finaly, take care the input language.
     */
    if (localeInfo[OLLC_INPUT_LANG].value == NULL)
        localeInfo[OLLC_INPUT_LANG].value
			= localeInfo[OLLC_BASIC_LOCALE].value;
}


static void
doEnv()
{
    localeInfoRec	*li;
    char		*env;
    char		*fmt;
    char		 buffer[256];


    /*
     * Try to effecient as possible, there will be only necessary
     * shell environment variables to be set/export.  However, this
     * routine will not do the unset (or unsetenv in csh), to elminate
     * unnecessary environment variables.  This is becasue some shell
     * script which written by the user might be depend on the
     * existence of the those variables.
     */

    if (OPT_CSH)
	fmt = "setenv %s '%s' ;\n";
    else
	fmt = "%s='%s' ;\n";

    buffer[0] = '\0';

    if ((env = getenv("LANG")) == NULL
     || strcmp(env, localeInfo[OLLC_BASIC_LOCALE].value) != 0) {

	if (OPT_EXEC) {
	    (void) sprintf(buffer, "LANG=%s", localeInfo[OLLC_BASIC_LOCALE].value);
	    (void) putenv(buffer);
	} else {
	    (void) printf(fmt, "LANG", localeInfo[OLLC_BASIC_LOCALE].value);
	    if (env == NULL)
	        (void) strcat(buffer, "LANG ");
	}
    }

    for (li = &localeInfo[1]; li->posix != -1; li++) {
        if (li->envName == NULL)
	    continue;
        env = getenv(li->envName);
	if (env == NULL
	 && strcmp(localeInfo[OLLC_BASIC_LOCALE].value, li->value) == 0)
	    continue;
	if (env != NULL && strcmp(env, li->value) == 0)
	    continue;

	if (OPT_EXEC) {
	    (void) sprintf(buffer, "%s=%s", li->envName, li->value);
	    (void) putenv(buffer);
	} else {
	    (void) printf(fmt, li->envName, li->value);
	    if (env == NULL) {
	        (void) strcat(buffer, li->envName);
	        (void) strcat(buffer, " ");
	    }
	}
    }
    if (OPT_ENV == True && OPT_CSH == False && buffer[0] != '\0')
	(void) printf("export %s;\n", buffer);
}


static void
doLocale()
{
    optionRec		*o;
    int			 i;


    i = OPT_LOCALE_OFFSET;
    for (o = &option[i]; o->name != NULL; i++, o++) {
	if (o->v.bool == True) {
	    (void) printf("%s\n", localeInfo[i - OPT_LOCALE_OFFSET].value);
	    break;
	}
    }
}


static void
doExec()
{
    (void) execvp(OPT_EXEC[0], OPT_EXEC);
    (void) fprintf(stderr, gettext("%s: can't execute %s: %s\n"),
		   myName, OPT_EXEC[0], strerror(errno));
}


static void
doPrint()
{
    localeInfoRec	*li;

    if (OPT_HEADER == False) {
	                  /* 01234567890123456789012345678901234567890123456 */
	(void) puts(gettext("Basic     Display   Numeric   Time      Input"));
    }
    for (li = localeInfo; li->posix != -1; li++) {
	if (li->instance == NULL)
	    continue;
        (void) printf("%-9s ", li->value);
    }
    (void) putchar('\n');
}
