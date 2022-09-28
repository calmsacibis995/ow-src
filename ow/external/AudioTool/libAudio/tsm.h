/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_TSM_H
#define	_MULTIMEDIA_TSM_H

#ident	"@(#)tsm.h	1.5	92/11/11 SMI"

#include "libaudio.h"
#include "audio_encode.h"

#define u2s(x) ((int)audio_u2s(x))
#define s2u(x) ((unsigned char)audio_s2u(x))

#define BLOCK_SIZE	0.032			/* block size is 32ms */
#define OVERLAP_SIZE	0.008			/* overlap size is 8ms */

/* 
 * Structure which maintains internal state of tsm() between calls.
 * Applications use tsm_init to create and initialize
 * a state structure for each channel processed by tsm().
 */
typedef struct {
	int	block_size;	/* Operating block size in samples. 32ms */
	int	overlap_size;	/* Overlap size in samples. 8ms */
	short	*olap;		/* Trailing portion of the last output block */
	int	olap_size;
	short	*leftover;	/* Input data left over from previous call */
	int	leftover_size;
} TSM_STATE;

EXTERN_FUNCTION( TSM_STATE* tsm_create_state, (unsigned int) );
EXTERN_FUNCTION( void tsm_init_state, (TSM_STATE*) );
EXTERN_FUNCTION( void tsm_destroy_state, (TSM_STATE*) );
EXTERN_FUNCTION( unsigned int tsm, (short*, unsigned, double, TSM_STATE*,
    short*) );
EXTERN_FUNCTION( unsigned int tsm_outbuf_leng, (unsigned int, double, TSM_STATE*) );

#endif /* !_MULTIMEDIA_TSM_H */

