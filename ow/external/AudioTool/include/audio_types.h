/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _AUDIO_TYPES_H
#define	_AUDIO_TYPES_H

#ident	"@(#)audio_types.h	1.6	92/11/13 SMI"

/* typedefs & #defines used throughout audio sources */

#include <sys/types.h>

/* XXX - The audio interface moved in SVr4 */
#ifdef SUNOS41
#include <sun/audioio.h>
#else /* SVR4 */
#include <sys/audioio.h>
#endif /* SVR4 */

/*
 * Include our local copy of c_varieties.h.
 * This would be so much easier if c_varieties was "standard".
 */
#include <multimedia/audio_types.h>

/* this should be in sys/types.h. sigh.... */
typedef void *ptr_t;

#endif /* !_AUDIO_TYPES_H */
