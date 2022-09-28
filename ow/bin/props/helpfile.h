#pragma ident "@(#)helpfile.h	1.1 92/10/23 SMI"

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE
 *	file for terms of the license.
 */

#include <stdio.h>

#ifndef TRUE
#define TRUE 	1
#define FALSE 	0
#endif

extern FILE*
HelpFindFile(
const	char *filename,
	char *locale);

extern int
HelpSearchFile(
	FILE *helpfile,
	char *key,
	char **moreHelp);

extern char*
HelpGetText(
	FILE *helpfile);
