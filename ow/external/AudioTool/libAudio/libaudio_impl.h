/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_LIBAUDIO_IMPL_H
#define	_MULTIMEDIA_LIBAUDIO_IMPL_H

#ident	"@(#)libaudio_impl.h	1.7	92/11/10 SMI"

#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include "libaudio.h"
#include "archdep.h"

/**
 * Useful defines
 *
 * CALLOC - allocate memory and clear it
 *		foo = CALLOC(15, char *);		-- alloc 15 ptrs
 *
 * MALLOC - allocate memory
 *		foo = MALLOC(struct foobar);		-- alloc 1 foobar
 *
 * REALLOC - re-allocate a larger amount of memory
 *		foo = REALLOC(foo, 10, struct foo);	-- extend to 10 foos
 *
 * FREE - de-allocate memory
 *
 * NOTE: These routines all operate on objects they can take the size of,
 *	 rather than byte counts.
 *
 *
 * XXX - the (long) in the following defines is used to make
 * XXX   lint shut up about pointer alignment problems.
 **/
#define MALLOC(type)	\
	(type *)(long)malloc(sizeof (type))
#define CALLOC(number, type) \
	(type *)(long)calloc((unsigned)(number), sizeof (type))
#define REALLOC(ptr, number, type) \
	(type *)(long)realloc((char *)(ptr), (unsigned)(number) * sizeof (type))
#define FREE(ptr)	\
	(void) free((char *)(ptr))

/**
 * START_C_FUNC - declare this function with C linkage
 * END_C_FUNC - put at the end of the function
 **/
#define START_C_FUNC	extern "C" {
#define END_C_FUNC	}

#endif /* !_MULTIMEDIA_LIBAUDIO_IMPL_H */
