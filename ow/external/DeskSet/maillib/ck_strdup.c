/* @(#)ck_strdup.c	3.1 - 92/04/03 */

/* Strdup.c -- copy a string into a new buffer; exit if not enough memory */


#include <stdio.h>
#include "ck_strings.h"

char *
ck_strdup( s )
char *s;
{
	char *buf;

	if (s == NULL)
		return (NULL);

	buf = ck_malloc( strlen( s ) + 1 );

	strcpy( buf, s );

	return( buf );
}
