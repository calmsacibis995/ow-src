#pragma ident "@(#)helpfile.c	1.2 93/05/04 SMI"

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

#define DEFAULT_HELP_DIRECTORY "/usr/openwin/lib/locale:/usr/openwin/lib/help"
#define MAX_MORE_HELP_CMD 128

static char     helpBuffer[128];

/*
 * HelpSearchFile
 *	Search the help file for the more-help command
 *	Returns TRUE and sets moreHelp if found.
 *	Returns FALSE otherwise
 */
int
HelpSearchFile(
	FILE	*helpFile,
	char	*key,		/* Spot Help key */
	char	**moreHelp) 	/* OUTPUT parameter: More Help system cmd */
{
	char	*entry;
	char	*moreHelpCmd;
static 	char	moreHelpCmdBuffer[MAX_MORE_HELP_CMD];

	fseek(helpFile, 0, 0);

	while (entry = fgets(helpBuffer, sizeof(helpBuffer), helpFile)) {
		if (*entry++ != ':')
			continue;

		entry = strtok(entry, ":\n");  /* parse Spot Help key */

		if (entry && 0 == strcmp(entry, key)) {
			/* Found requested Spot Help key */
			/* Parse More Help system * command */
			moreHelpCmd = strtok(NULL, "\n"); 
			if (moreHelpCmd) {
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
const	char	*filename,
	char	*locale)
{
	FILE	*fp = NULL;
	char	*helpDir;
	char	*helpPathEnv;
	char	*helpPath;
	char	helpFile[MAXPATHLEN];

	if ((helpPathEnv = getenv("HELPPATH")) == NULL) {
		helpPathEnv = DEFAULT_HELP_DIRECTORY;
	}
	helpPath = (char *) malloc(strlen(helpPathEnv) + 1);
	strcpy(helpPath, helpPathEnv);

	if (!locale)
    		locale = setlocale(LC_MESSAGES, NULL); 

	for (helpDir = strtok(helpPath, ":");
	     helpDir != NULL;
	     helpDir = strtok(NULL, ":")) {

		/*
		 * Look for locale-specific help first 
		 */
		if (locale) {
			sprintf(helpFile, "%s/%s/help/%s", 
				helpDir, locale, filename);
			if ((fp = fopen(helpFile, "r")) != NULL)
				break;
			}   
		/*
		 * Fallback on helpDir/filename.
		 */
		sprintf(helpFile, "%s/%s", helpDir, filename);
		if ((fp = fopen(helpFile, "r")) != NULL) {
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
	char	*filekey,		/* "file:key" */
	char	**moreHelp)		/* OUTPUT parameter */
{
	char	*client;
	char	data_copy[64];
	char	filename[64];
	char	*key;
static 	char	last_client[64];

	if (filekey == NULL)
		return FALSE;		/* No key/file specified */

	strncpy(data_copy, data, sizeof(data_copy));
	data_copy[sizeof(data_copy) - 1] = '\0';

	if (!(client = strtok(data_copy, ":")) || !(key = strtok(NULL, "")))
		return FALSE;		/* No file specified in key */

	if (strcmp(last_client, client)) {
		/* Last .info filename != new .info filename */
		if (helpFile) {
			fclose(helpFile);
			last_client[0] = '\0';
		}
		sprintf(filename, "%s.info", client);
		helpFile = HelpFindFile(filename);
		if (helpFile) {
			strcpy(last_client, client);
			return HelpSearchFile(key, more_help);
		} else {
			return FALSE;  /* Specified .info file not found */
		}
	}
	return (HelpSearchFile(key, more_help));
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
	char	*ptr;

	while ((ptr = fgets(helpBuffer, sizeof(helpBuffer), helpFile)) &&
		(*ptr == '#'))
			;

	return (ptr && *ptr != ':' ? ptr : NULL);
}
