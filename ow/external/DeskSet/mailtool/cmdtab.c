#ifndef lint
static	char *sccsid = "@(#)cmdtab.c 3.2 92/12/14 SMI"; /* from S5R2 1.1 */
#endif

#include "def.h"

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Define all of the command names and bindings.
 */


extern int alternates();
extern int echo();
extern int elsecmd();
extern int endifcmd();
extern int group();
extern int ifcmd();
extern int igfield();
extern int retfield();
extern int schdir();
extern int set();
extern int source();
extern int unset();
extern int user_button();

clearaliases()
{
	alias_free();
	return (0);
}

struct cmd cmdtab[] = {
	"alias",	group,		RAWLIST,	2,	1000,
	"alternates",	alternates,	RAWLIST,	1,	1000,
	"cd",		schdir,		STRLIST,	0,	0,
	"chdir",	schdir,		STRLIST,	0,	0,
	"discard",	igfield,	RAWLIST,	1,	1000,
	"echo",		echo,		RAWLIST,	0,	1000,
	"else",		elsecmd,	F|RAWLIST,	0,	0,
	"endif",	endifcmd,	F|RAWLIST,	0,	0,
	"group",	group,		RAWLIST,	2,	1000,
	"if",		ifcmd,		F|RAWLIST,	1,	1,
	"ignore",	igfield,	RAWLIST,	1,	1000,
	"retain",	retfield,	RAWLIST,	1,	1000,
	"set",		set,		RAWLIST,	1,	1000,
	"source",	source,		STRLIST,	0,	0,
	"unset",	unset,		RAWLIST,	1,	1000,
	"button",	user_button,	RAWLIST,	4,	4,
        "clearaliases", clearaliases,   RAWLIST,      	0,      0,
	0,		0,		0,		0,	0
};
