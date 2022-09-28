/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_ATOOL_DEBUG_H
#define	_AUDIOTOOL_ATOOL_DEBUG_H

#ident	"@(#)atool_debug.h	1.7	92/12/10 SMI"

#include <multimedia/audio_types.h>


/* DEBUG_PRINT defined by DEBUG.  It may also be defined at compile time. */
#if defined(DEBUG_PRINT) || defined(DEBUG)

#ifndef DEBUG_PRINT
#define DEBUG_PRINT
#endif

#define DBGOUT(args)    (void) _dbgprintf args

/* Define some different debug levels */
#define D_DND	0x01
#define D_x02	0x02
#define D_x04	0x04
#define D_x08	0x08
#define D_x10	0x10
#define D_x20	0x20
#define D_x40	0x40
#define D_x80	0x80
#define D_ALL	0xff

EXTERN_FUNCTION( void _dbgprintf, (int, char *, ...) );
EXTERN_FUNCTION( void set_debug_level, (int) );

#else /* !DEBUG_PRINT */
#define DBGOUT(args)
#endif /*!DEBUG_PRINT*/

#endif /* !_AUDIOTOOL_ATOOL_DEBUG_H */
