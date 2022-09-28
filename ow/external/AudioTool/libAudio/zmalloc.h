/* Copyright (c) 1991 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_ZMALLOC_H
#define	_MULTIMEDIA_ZMALLOC_H

#ident	"@(#)zmalloc.h	1.3	92/04/16 SMI"

#include "audio_types.h"

EXTERN_FUNCTION( void* zmalloc, (size_t size) );
EXTERN_FUNCTION( void zfree, (void *) );

#endif /* !_MULTIMEDIA_ZMALLOC_H */
