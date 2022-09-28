#pragma ident	"@(#)wkspreadmenu.c	1.5	92/11/20 SMI"

/* Copyright */


/* Includes */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <X11/Intrinsic.h>	/* For Boolean */

#include "wksplist.h"
#include "wkspmenuentry.h"
#include "wkspreadmenu.h"

#ifdef DBMALLOC
	#include "dbmalloc.h"
#endif

#define TOKEN_LENGTH	 300


static char		*skipLeadingWhiteSpace(const char *);
static Boolean		 isContinued(const char *);
static char		*finishReadingLine(FILE *, char *, int *);
static LineType		 getLineType(const char *const,char *const,char *const);
static MenuEntry	*createMenuEntryAndAppendToMenuList(HeadTail *);
static MenuEntry	*getMenuEntryFromElement(const ListElement *const);


/*
 * parseMenu
 *
 *	Reads an olwm menu from the given stream and parses the it into a
 *	linked list.
 *
 * Returns
 *
 *	A pointer to the menu entries list.
 *
 *	NULL on failure.
 */

HeadTail*
parseMenu(
	char		*programsMenuPath,
	int		*lineNumber,
	MenuEntry	*menuTitle)
{
	FILE		*programsMenuFile;

	char		 buffer[ TOKEN_LENGTH];
	char		 label[  TOKEN_LENGTH];
	char		 command[TOKEN_LENGTH];
	char		*currentLine;	/* 1st non-white space in buffer. */
	
	LineType	 lineType;
    
	const char	*const UNQUOTED_FORMAT
				= "%[^ \t\n]%*[ \t]%[^\n]\n",
			*const QUOTED_FORMAT
				= "\"%[^\"]\"%*[ \t]%[^\n]\n";
	const char	*format;

	HeadTail	*menuEntryList;
	ListElement	*menuListElement;
	MenuEntry	*newMenuEntry;
	
	int		 defaultsFound = 0;


	programsMenuFile = fopen(programsMenuPath, "r");

	menuEntryList = createList();

       *lineNumber    = 0;

	while (fgets(buffer, sizeof(buffer), programsMenuFile) != NULL) {

		(void) memset(label,   '\0', TOKEN_LENGTH);
		(void) memset(command, '\0', TOKEN_LENGTH);

		(*lineNumber)++;

		currentLine = skipLeadingWhiteSpace(buffer);

		if (isContinued(currentLine)) {
			currentLine = finishReadingLine(
					programsMenuFile,
					currentLine,
					lineNumber);
		}

		if (currentLine[0] == '"') {
			format = QUOTED_FORMAT;
		} else {
			format = UNQUOTED_FORMAT;
		}

		(void) sscanf(currentLine, format, label, command);

		lineType = getLineType(currentLine, label, command);

		if (   (lineType != BLANK_LINE) /* Ignore, not written out if */
		    && (lineType != COMMENT)	/* user does a save.          */
		    && (lineType != TITLE))	/* See comment below.         */
		{
			newMenuEntry =
			    createMenuEntryAndAppendToMenuList(menuEntryList);
			newMenuEntry->type = lineType;
		}
		
		switch (lineType) {
		case BLANK_LINE:
			break;

		case COMMENT:
			break;

		case SEPARATOR:
			newMenuEntry->command = XtNewString(label);

			break;

		case TITLE:
			/*
			 * Each time a title is read the last title is over-
			 * written.  This agrees with olwm which ignores all
			 * but the last title in a menu file.
			 *
			 * NOTE:  This means that only the last title read will
			 * be preserved if the user does an apply (save).
			 */
			 
			menuTitle->label   = XtNewString(label);
			menuTitle->command = XtNewString(command);
			menuTitle->type    = lineType;
				
			break;

		case INCLUDED_MENU:
		case MENU:
		case MENU_END:
		case COMMAND:
			newMenuEntry->label   = XtNewString(label);
			newMenuEntry->command = XtNewString(command);
			
			if (strncmp("DEFAULT", command, 7) == 0) {
				newMenuEntry->isDefault = TRUE;

				defaultsFound++;
			}

			break;

		} /* switch */
	} /* while */
	
	if (defaultsFound == 0) {
		char		*tmp;
		
		MenuEntry	*firstMenuEntry;

		firstMenuEntry = getMenuEntryFromElement(menuEntryList->head);
		firstMenuEntry->isDefault = TRUE;

		tmp = XtMalloc(
			  strlen("DEFAULT")
			+ strlen(firstMenuEntry->command)
			+ 2);
		(void) sprintf(tmp, "%s%s", "DEFAULT ",firstMenuEntry->command);
		XtFree(firstMenuEntry->command);
		firstMenuEntry->command = tmp;
	
	} else if (defaultsFound > 1) {
		for (menuListElement =  menuEntryList->head;
		     menuListElement != NULL;
		     menuListElement =  menuListElement->next)
		{
			MenuEntry	*menuEntry;

			menuEntry = getMenuEntryFromElement(menuListElement);
		
			if (   (menuEntry->isDefault == TRUE)
			    && (defaultsFound         > 1))
			{
				char	*tmp;
				
				menuEntry->isDefault = FALSE;

				tmp = menuEntry->command + strlen("DEFAULT");
				tmp = skipLeadingWhiteSpace(tmp);
				tmp = XtNewString(tmp);
				XtFree(menuEntry->command);
				menuEntry->command = tmp;
				
				defaultsFound--;
			}
		}
	}

	fclose(programsMenuFile);
	
	return menuEntryList;
}


static char *
skipLeadingWhiteSpace(
	const char	*line)
{
	char		*currentCharacter = (char *)line;
	

	while (isspace((int)(*currentCharacter))) {
		currentCharacter++;
	}
	
	return currentCharacter;
}


/*
 * Assumes the line was read by fgets.  fgets preserves newlines and appends
 * a NULL to the end of the line.
 */
 
static Boolean
isContinued(
	const char	*line)
{
	const char	 CONTINUATION = '\\';
	
	Boolean		 isContinuation;


	/*
	 * We're looking for a line that looks like:
	 *
	 *	[...\\\n\0]
	 */

	/* Remember strlen ignores the NULL at the end of the line. */

	if (line[strlen(line) - 2] == CONTINUATION) {
		isContinuation = TRUE;
	} else {
		isContinuation = FALSE;
	}
	
	return isContinuation;
}


static char*
finishReadingLine(
	FILE	*stream,
	char	*line,
	int	*lineNumber)
{
	Boolean	 Done = FALSE;
	
	char	 buffer[TOKEN_LENGTH];
	
	
	/* Remove backslash and newline from the continuation's first line. */
	line[strlen(line) - 2] = '\0';

	while (!Done) {
		if (fgets(buffer, sizeof(buffer), stream) == NULL) {
			return line;
		}

		(*lineNumber)++;
		
		if (isContinued(buffer)) {
			/* Get rid of backslash. */
			buffer[strlen(buffer) - 2] = '\0';
		} else {
			Done = TRUE;
		}
		
		(void) strcat(line, " ");
		(void) strcat(line, skipLeadingWhiteSpace(buffer));
	}

	return line;
}


/* Assumes that leading white space has been removed */

static LineType
getLineType(
	const char	*const line,
	char		*const label,
	char		*const command)
{
	LineType	 lineType;
	

	if (line[0] == '#') {		/* Olwm comment lines begin with '#'. */
		lineType = COMMENT;
		
	} else if (line[0] == '\0') {
		lineType = BLANK_LINE;
		
	} else if (strcmp(label, "SEPARATOR") == 0) {
		lineType = SEPARATOR;
		
	} else if (strncmp(command, "TITLE",   5) == 0) {
		lineType = TITLE;
		
	} else if (strncmp(command, "INCLUDE", 7) == 0) {
		lineType = INCLUDED_MENU;
		
	} else if (strncmp(command, "MENU",    4) == 0) {
		lineType = MENU;
		
	} else if (strncmp(command, "END",     3) == 0) {
		lineType = MENU_END;
		
	} else {
		lineType = COMMAND;

	}
	
	return lineType;
}


static MenuEntry*
createMenuEntryAndAppendToMenuList(
	HeadTail	*menuEntryList)
{
	ListElement	*listElement;


	listElement       = createListElement();
	listElement->data = createMenuEntry();

	appendAfter(menuEntryList, menuEntryList->tail, listElement);
	
	return (MenuEntry *)(listElement->data);
}


static MenuEntry*
getMenuEntryFromElement(
	const ListElement	*const	menuListElement)
{
	assert(menuListElement != NULL);
	
	return (MenuEntry *)(menuListElement->data);
}
