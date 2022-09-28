/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOTYPECONVERT_H
#define	_MULTIMEDIA_AUDIOTYPECONVERT_H

#ident	"@(#)AudioTypeConvert.h	1.3	92/06/08 SMI"

#include "AudioBuffer.h"


// This is the abstract base class for an audio type conversion module

class AudioTypeConvert {
protected:
	AudioHdr	hdr;		// contains type information

public:
	AudioTypeConvert() {};				// Constructor
	virtual ~AudioTypeConvert() {};			// Destructor
	virtual AudioHdr DataType() const		// Return type
	    { return (hdr); }

	// class methods specialized by subclasses
	virtual Boolean CanConvert(			// TRUE if conversion ok
	    AudioHdr h) const = 0;		// type to check against

	// Convert buffer to the specified type
	// Either the input or output type must be handled by this class
	virtual AudioError Convert(			// Convert to new type
	    AudioBuffer*& inbuf,		// data buffer to process
	    AudioHdr outhdr) = 0;		// target header

	virtual AudioError Flush(AudioBuffer*& buf) = 0; // flush any remaining
						         // data that may exist

#ifdef notdef
	virtual void ReInit(AudioHdr inhdr, AudioHdr outhdr) {}; // reset state
#endif
};

#endif /* !_MULTIMEDIA_AUDIOTYPECONVERT_H */
