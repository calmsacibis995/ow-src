#pragma ident	"@(#)wkspmenuentry.c	1.3	92/11/13 SMI"

/* Copyright */


/* Includes */

#include <stdio.h>
#include <stdlib.h>

#include "wkspmenuentry.h"

#ifdef DBMALLOC
	#include "dbmalloc.h"
#endif

MenuEntry *
createMenuEntry(
	void)
{
	MenuEntry *newMenuEntry;
	
	
	newMenuEntry = XtNew(MenuEntry);
	
	newMenuEntry->label	= "";
	newMenuEntry->keyword	= "";
	newMenuEntry->command	= "";
	newMenuEntry->isDefault	= FALSE;
	newMenuEntry->type	= UNKNOWN;
	newMenuEntry->token	= NULL;
 
	return newMenuEntry;
}

