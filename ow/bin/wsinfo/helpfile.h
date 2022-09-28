#pragma ident	"@(#)helpfile.h	1.2	94/02/10 SMI"

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE
 *	file for terms of the license.
 */

#include <stdio.h>

typedef const char *const	strconst;

#ifndef TRUE
#define TRUE 	1
#define FALSE 	0
#endif

extern FILE*
HelpFindFile(
	strconst	  filename,
	char		 *locale);

extern int
HelpSearchFile(
	FILE		 *helpfile,
	strconst	  key,
	char		**moreHelp);

extern char*
HelpGetText(
	FILE		  *helpfile);
