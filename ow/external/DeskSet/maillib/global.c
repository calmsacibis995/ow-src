/* @(#)global.c	3.4 - 94/07/29 */


#include <stdio.h>
#include <pwd.h>
#include "global.h"
#include "hash.h"
#include "charset.h"
#include "ck_strings.h"

static char *init_username();

extern char *getenv();
extern char *mt_value();

struct globals glob;


init_globals()
{
	char *buffer;
	char *localpath = "/lib/locale";
	char *owhome;
	int len;

	owhome = getenv("OPENWINHOME");
	if (! owhome) {
		owhome = "/usr/openwin";
	}

	len = strlen(localpath) + strlen(owhome) + 10;
	buffer = ck_malloc(len);
	sprintf(buffer, "%s%s", owhome, localpath);
	bindtextdomain("maillib", buffer);

	mt_assign("USER", init_username());
	glob.g_myname = mt_value("USER");
	glob.g_ignore = hash_method.hm_alloc();
	glob.g_retain = hash_method.hm_alloc();
	glob.g_alias = hash_method.hm_alloc();
	glob.g_alternates = hash_method.hm_alloc();
	glob.g_iconvinfo = 0;

	cs_methods.cs_init();
}


static char *
init_username()
{
	char *s;
	struct passwd *pw;

	if ((s = getenv("USER")) != NULL) {
		return (s);
	}

	if ((pw = getpwuid(getuid())) == NULL) {
		fprintf(stderr, "Who are you?\n");
		exit( 1 );
	}

	endpwent();
	return (pw->pw_name);
}
