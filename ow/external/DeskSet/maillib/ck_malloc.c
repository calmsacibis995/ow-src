/* @(#)ck_malloc.c	3.2 - 94/09/13 */

/* ck_malloc.c -- allocate memory; exit if we fail */

#include <stdio.h>
#include <errno.h>
#include "misc.h"

void *malloc();

void *
ck_malloc( size )
int size;
{
	void *mem;

	mem = malloc( size );

	if( ! mem ) {
		maillib_methods.ml_error (NULL, strerror(ENOMEM));
	}

	return( mem );
}
