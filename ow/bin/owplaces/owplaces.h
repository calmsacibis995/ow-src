/*
 *      (c) Copyright 1989, 1990 Sun Microsystems, Inc. Sun design patents
 *      pending in the U.S. and foreign countries. See LEGAL_NOTICE
 *      file for terms of the license.
 */

#ident "@(#)owplaces.h	1.3 93/04/13 owplaces.h SMI"

/*
 *	owplaces.h
 */

/*
 *	Return/exit codes from owplaces
 */
#define SUCCESS		0
#define USAGE_ERROR	1
#define GENERAL_ERROR	2
#define DISPLAY_ERROR	3
#define FILE_ERROR	4
#define TIMEOUT_ERROR	5

/*
 *	Text Localization
 */
#include <locale.h>
extern	char			*gettext();
#define	LOCALIZE(msg)		gettext(msg)
