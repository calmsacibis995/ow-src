#pragma ident	"@(#)helpfile.c	1.3	94/02/10 SMI"

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <sys/param.h>
#include <sys/types.h>
#include "helpfile.h"

strconst	DEFAULT_HELP_DIRECTORY =
			"/usr/openwin/lib/locale:/usr/openwin/lib/help";

#define MAX_MORE_HELP_CMD 128

/* 
static FILE    *helpFile;
*/
static char     helpBuffer[128];

/*
 * HelpSearchFile
 *	Search the help file for the more-help command
 *	Returns TRUE and sets moreHelp if found.
 *	Returns FALSE otherwise
 */
int
HelpSearchFile(
	FILE	 *helpFile,
	strconst  key,		/* Spot Help key */
	char	**moreHelp) 	/* OUTPUT parameter: More Help system cmd */
{
	char	 *entry = NULL;
	char	 *moreHelpCmd = NULL;
static 	char	  moreHelpCmdBuffer[MAX_MORE_HELP_CMD];

    fseek(helpFile, 0, 0);

    while (entry = fgets(helpBuffer, sizeof(helpBuffer), helpFile)) {
	if (*entry++ != ':') {
	    continue;
	}
	entry = strtok(entry, ":\n");  /* parse Spot Help key */

	if ((entry != NULL) && (strcmp(entry, key) == 0)) {
	    /* Found requested Spot Help key */
	    /* Parse More Help system * command */
	    moreHelpCmd = strtok(NULL, "\n"); 
	    if (moreHelpCmd != NULL) {
		strncpy(moreHelpCmdBuffer, moreHelpCmd,
			MAX_MORE_HELP_CMD);
		*moreHelp = &moreHelpCmdBuffer[0];
	    } else {
		*moreHelp = NULL;
	    }
	    return TRUE;
	}
    }
    return FALSE;
}


/*
 * HelpFindFile
 */
FILE *
HelpFindFile(
	strconst	 filename,
	char		*locale)
{
	FILE		*fp          = NULL;
	char		*helpDir     = NULL;
	const char	*helpPathEnv = NULL;
	char		*helpPath    = NULL;
	char		 helpFile[MAXPATHLEN];

    helpPathEnv = getenv("HELPPATH");
    if (helpPathEnv == NULL) {
	helpPathEnv = DEFAULT_HELP_DIRECTORY;
    }
    helpPath = malloc(strlen(helpPathEnv) + 1);
    strcpy(helpPath, helpPathEnv);

    if (locale == NULL) {
	locale = setlocale(LC_MESSAGES, NULL); 
    }
	
    for (helpDir = strtok(helpPath, ":");
	 helpDir != NULL;
	 helpDir = strtok(NULL, ":"))
    {
	/* Look for locale-specific help first. */
	if (locale != NULL) {
	    sprintf(helpFile, "%s/%s/help/%s", helpDir, locale, filename);
	    fp = fopen(helpFile, "r");
	    if (fp != NULL) {
		break;
	    }
	}
   
	/* Fallback on helpDir/filename. */
	sprintf(helpFile, "%s/%s", helpDir, filename);
	fp = fopen(helpFile, "r");
	if (fp != NULL) {
	    break;
	}
    }
    free(helpPath);
    return fp;
}


#ifdef notdef
/*
 * HelpGetArg
 */
int
HelpGetArg(
	char	 *filekey,		/* "file:key" */
	char	**moreHelp)		/* OUTPUT parameter */
{
	char	 *client = NULL;
	char	  data_copy[64];
	char	  filename[64];
	char	 *key = NULL;
static 	char	  last_client[64];

    if (filekey == NULL) {
	return FALSE;		/* No key/file specified */
    }
    
    strncpy(data_copy, data, sizeof(data_copy));
    data_copy[sizeof(data_copy) - 1] = '\0';

    if (!(client = strtok(data_copy, ":")) || !(key = strtok(NULL, ""))) {
	return FALSE;		/* No file specified in key */
    }
    
    if (strcmp(last_client, client)) {
	/* Last .info filename != new .info filename */
	if (helpFile != NULL) {
	    fclose(helpFile);
	    last_client[0] = '\0';
	}
	sprintf(filename, "%s.info", client);
	helpFile = HelpFindFile(filename);
	if (helpFile != NULL) {
	    strcpy(last_client, client);
	    return HelpSearchFile(key, more_help);
	} else {
	    return FALSE;  /* Specified .info file not found */
	}
    }
    return HelpSearchFile(key, more_help);
}
#endif


/*
 * HelpGetText
 *	Gets the next chunk of help text for the current key.
 *	Returns NULL when complete.
 */
char *
HelpGetText(
    FILE	*helpFile)
{
    char	*ptr = NULL;

    while (   ( ptr = fgets(helpBuffer, sizeof(helpBuffer), helpFile))
	   && (*ptr == '#'))
    {
	;	/* Empty Loop */
    }

    if ((ptr != NULL) && (*ptr != ':')) {
	return ptr;
    } else {
	return NULL;
    }
}
