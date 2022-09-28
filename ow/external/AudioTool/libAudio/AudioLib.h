/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOLIB_H
#define	_MULTIMEDIA_AUDIOLIB_H

#ident	"@(#)AudioLib.h	1.5	90/10/11 SMI"

#include "Audio.h"


// Declarations for global functions
AudioError AudioCopy(					// Copy entire stream
    Audio*	from,				// input source
    Audio*	to);				// output sink

AudioError AudioCopy(					// Copy data
    Audio*	from,				// input source
    Audio*	to,				// output sink
    Double&	frompos,			// input position (updated)
    Double&	topos,				// output position (updated)
    Double&	limit);				// amount to copy (updated)

AudioError AudioAsyncCopy(				// Copy one data segment
    Audio*	from,				// input source
    Audio*	to,				// output sink
    Double&	frompos,			// input position (updated)
    Double&	topos,				// output position (updated)
    Double&	limit);				// amount to copy (updated)

AudioError Audio_OpenInputFile(				// Filename->AudioList
    const char*	path,				// input filename
    Audio*&	ap);				// returned AudioList ptr

AudioError Audio_WriteOutputFile(			// Copy to output file
    const char*	path,				// output filename
    const AudioHdr&	hdr,			// output data header
    Audio*	input);				// input data stream

#endif /* !_MULTIMEDIA_AUDIOLIB_H */
