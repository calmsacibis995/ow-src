#pragma ident	"@(#)wkspwritemenu.c	1.2	92/11/11 SMI"

/* Copyright */


/* Includes */

#include <stdio.h>
#include <string.h>

#include "wksplist.h"
#include "wkspmenuentry.h"
#include "wkspwritemenu.h"

#ifdef DBMALLOC
	#include "dbmalloc.h"
#endif


static MenuEntry*	getMenuEntryFromElement(const ListElement *const);


int
writeMenu(
	const FILE	*const menuFile,
	const HeadTail	*const menuList,
	const MenuEntry	*const menuTitle)
{
	ListElement	*menuListElement;
	MenuEntry	*menuEntry;


	(void) fputs("\"",               (FILE *)menuFile);
	(void) fputs(menuTitle->label,   (FILE *)menuFile);
	(void) fputs("\"",               (FILE *)menuFile);
	
	(void) fputs("\t",               (FILE *)menuFile);
	
	(void) fputs(menuTitle->command, (FILE *)menuFile);

	(void) fputs("\n",               (FILE *)menuFile);
	
	for (menuListElement = menuList->head;
	     menuListElement != NULL;
	     menuListElement = menuListElement->next)
	{
		menuEntry = getMenuEntryFromElement(menuListElement);

		if (strcmp(menuEntry->label, "") != 0) {
			(void) fputs("\"",               (FILE *)menuFile);
			(void) fputs(menuEntry->label,   (FILE *)menuFile);
			(void) fputs("\"",               (FILE *)menuFile);

		}			

		(void) fputs("\t",               (FILE *)menuFile);
		
		(void) fputs(menuEntry->command,         (FILE *)menuFile);

		(void) fputs("\n",                       (FILE *)menuFile);
	}

	return 0;
}


static MenuEntry*
getMenuEntryFromElement(
        const ListElement     *const menuListElement)
{
        return ((MenuEntry *)(menuListElement->data));
}
