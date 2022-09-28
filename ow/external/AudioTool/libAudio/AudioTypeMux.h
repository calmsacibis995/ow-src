/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOTYPEMUX_H
#define	_MULTIMEDIA_AUDIOTYPEMUX_H

#ident	"@(#)AudioTypeMux.h	1.1	92/12/03 SMI"

#include "AudioTypeConvert.h"

// This is the class doing channel multiplex/demultiplex

class AudioTypeMux : public AudioTypeConvert {

protected:

public:
	AudioTypeMux();				// Constructor
	~AudioTypeMux();			// Destructor

	// Class AudioTypeConvert methods specialized here
	virtual Boolean CanConvert(		// TRUE if conversion ok
	    AudioHdr h) const;			// type to check against

	// Convert buffer to the specified type
	// Either the input or output type must be handled by this class
	virtual AudioError Convert(		// Convert to new type
	    AudioBuffer*& inbuf,		// data buffer to process
	    AudioHdr outhdr);			// target header

	virtual AudioError Flush(AudioBuffer*& buf);
};

#endif /* !_MULTIMEDIA_AUDIOTYPEMUX_H */
