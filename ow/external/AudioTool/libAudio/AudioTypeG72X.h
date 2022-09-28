/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOTYPEG72X_H
#define	_MULTIMEDIA_AUDIOTYPEG72X_H

#ident	"@(#)AudioTypeG72X.h	1.4	92/12/04 SMI"

#include "AudioTypeConvert.h"
#include "audio_encode.h"

// This is the class for CCITT G.72X compress/decompress

class AudioTypeG72X : public AudioTypeConvert {
private:
	struct audio_g72x_state 	g72x_state;
	Boolean				initialized;

protected:

public:
	AudioTypeG72X();	// Constructor
	~AudioTypeG72X();	// Destructor

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

#endif /* !_MULTIMEDIA_AUDIOTYPEG72X_H */
