/* Copyright */

#ifndef WKSPMENUENTRY_H
#define WKSPMENUENTRY_H

#pragma ident	"@(#)wkspmenuentry.h	1.3	92/11/20 SMI"


#include <Xol/ScrollingL.h>	/* For OlListToken */
#include <X11/Intrinsic.h>	/* For Bolean      */


typedef enum {
	UNKNOWN,
	BLANK_LINE,
	COMMENT,
	SEPARATOR,
	TITLE,
	INCLUDED_MENU,
	MENU,
	MENU_END,
	COMMAND
} LineType;

typedef struct menuEntry {
	char		*label;
	char		*keyword;
	char		*command;
	Boolean		 isDefault;
	LineType	 type;
	OlListToken	 token;
} MenuEntry;


extern MenuEntry	*createMenuEntry(void);

#endif /* WKSPMENUENTRY_H */
