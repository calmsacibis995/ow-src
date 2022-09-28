#ident	"@(#)environ.c	1.10	93/07/29 SMI"

/*
 *      (c) Copyright 1989 Sun Microsystems, Inc.
 */

/*
 *      Sun design patents pending in the U.S. and foreign countries. See
 *      LEGAL_NOTICE file for terms of the license.
 */

#include <stdio.h>
#ifdef SYSV
#include <string.h>
#else
#include <strings.h>
extern char *strrchr();
extern char *strchr();
#endif
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "mem.h"
#include "ollocale.h"

extern	char **environ;

/* -----------------------------------------------------------------------
 *	Local Data Structures
 * -----------------------------------------------------------------------*/

/*
 * 	Env - environment object
 */
typedef struct _Env {
	char	**environ;	/* array of environment strings */
	int	length;		/* length of environ array */
	int	used;		/* number of entries actually used */
} Env;

/* -----------------------------------------------------------------------
 *	Local Functions
 * -----------------------------------------------------------------------*/

/*
 *	createEnv - Creates a new environment array that is the length of
 *		    of the current environment plus the number of additions.
 */
static void
createEnv(
	Env	*env,
	int	nadditions)
{
	int	i = 0;

	/* find the number of items in the current environ */
	while (environ[i] != (char *)NULL) {
		i++;
	}

	/* create space for the environ strings */
	env->used = i;
	env->length = env->used + nadditions + 1;
	env->environ = MemAlloc(env->length*sizeof(char *));

	/* copy the current environ into the new one */
	for (i=0; i<env->used; i++) {
		env->environ[i] = MemNewString(environ[i]);
	}
	env->environ[i] = (char *)NULL;
}

/*
 *	putEnv - Puts the name,value pair into the specified environment
 *	         replacing any existing values.
 *		 Assumes there is space for the new setting.
 */
static void
putEnv(
	Env	*env,
	char	*name,
	char	*value)
{
	int	nameLen = strlen(name);
	char	*envVar;
	int	count;

	/* create new env string with space for '=' and null */
	envVar = (char *)MemAlloc(nameLen + strlen(value) +2);

	(void)sprintf(envVar,"%s=%s",name,value);

	/* search through, checking for variable in question */
	for (count=0 ; count<env->used; count++) {
		if (!strncmp(env->environ[count],name,nameLen))
			break;
	}
	

	if (count == env->used)		/* finished loop without match */
		env->used++;		/* added 1 more var to the env */
	else
		MemFree(env->environ[count]);	/* don't need */

	env->environ[count] = envVar;

	/* make sure the last entry in the vector is NULL */
	env->environ[env->used] = (char *)NULL;

}

/*
 *	putDisplayEnv - sets the DISPLAY env to the appropriate screen
 */
static void
putDisplayEnv(
	Env	*env,
	Display	*dpy,
	int	screen)
{
	char	*display = DisplayString(dpy);
	char	*colon,*dot;
	char	value[128];
	int	len;

	if ((colon = strrchr(display,':')) == (char *)NULL) {
		return;
	}
	if ((dot = strchr(colon,'.')) != (char *)NULL) {
		len = dot - display;
	} else {
		len = colon - display;
	}

	(void)sprintf(value,"%.*s.%d",len,display,screen);

	putEnv(env,"DISPLAY",value);
}

#ifndef NOSVENV
/*
 *	putSunViewEnv - sets the various SV environment variables
 */
static void
putSunViewEnv(
	Env	*env,
	Display *dpy,
	int	screen)
{
	static char	*svEnv[] = { "WINDOW_PARENT", 
				     "WMGR_ENV_PLACEHOLDER", 
				     "WINDOW_TTYPARMS" };
	int		i, svEnvLen = sizeof(svEnv)/sizeof(char *);
	char		*result,*curpos;
	unsigned long	nitems,remainder;
	extern void	*GetWindowProperty();
	extern Atom	AtomSunViewEnv;

	result = (char *)GetWindowProperty(dpy,RootWindow(dpy,screen),
			AtomSunViewEnv,0L,100000L,
			XA_STRING,8,&nitems,&remainder);

	if (result == NULL)
		return;

	curpos = result;
	for (i=0; i<svEnvLen; i++) {
		putEnv(env,svEnv[i],curpos);
		curpos += strlen(curpos) + 1;
	}
	XFree((char *)result);
}
#endif /* NOSVENV */

/*
 *	putLocaleEnv - Sets the various LC_* locale environment variables.
 *		       LANG is set as the default from basiclocale.
 *		       The other LC_* variables are set from their value
 *		       in the OLLCItem list if different than LANG.
 */
static void
putLocaleEnv(
	Env		*env,
	OLLCItem	*lc)
{
	int		i;
	char		*lang = lc[OLLC_LC_BASIC_LOCALE].locale;

	putEnv(env,"LANG",lang);

	for (i = 0; i < OLLC_LC_MAX; i++) {
		if (lc[i].envName && 0 != strcmp(lc[i].locale,lang))
			putEnv(env,lc[i].envName,lc[i].locale);
	}
}

/* -----------------------------------------------------------------------
 *	Global Functions
 * -----------------------------------------------------------------------*/

/*
 *	MakeEnviron - returns a new environment array that contains the
 *		      current environ plus a modified DISPLAY and
 *		      SunView environment variables.
 */
char **
MakeEnviron(
	Display		*dpy,
	int		screen,
	OLLCItem	*lc)
{
	Env	newEnv;
	int	nadditions;

	nadditions = 1;			/* for DISPLAY */

#ifndef NOSVENV
	nadditions += 3;		/* for SunView environment */
#endif /* NOSVENV */

	nadditions += OLLC_LC_MAX + 1;	/* for LC_* locale environment */

	createEnv(&newEnv,nadditions);

	putDisplayEnv(&newEnv,dpy,screen);

#ifndef NOSVENV
	putSunViewEnv(&newEnv,dpy,screen);
#endif /* NOSVENV */

	putLocaleEnv(&newEnv,lc);

	return newEnv.environ;
}

/*
 *	UpdateEnviron
 */
char **
UpdateEnviron(
	Display		*dpy,
	int		screen,
	char		**oldenv,
	OLLCItem	*lc)
{
	Env		newEnv;
	int		i = 0;
	
	createEnv(&newEnv,OLLC_LC_MAX+1);

	/* Make sure the display is set correctly */
	putDisplayEnv(&newEnv,dpy,screen);

	putLocaleEnv(&newEnv,lc);

        /* Next, nuke the "old" environment */
	while(oldenv[i] != NULL)
	    MemFree(oldenv[i++]);
	MemFree((char *)oldenv);

	return newEnv.environ;
}
