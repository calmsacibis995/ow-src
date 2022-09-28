/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOTYPECHANNEL_H
#define	_MULTIMEDIA_AUDIOTYPECHANNEL_H

#ident	"@(#)AudioTypeChannel.h	1.1	92/06/05 SMI"

#include "AudioTypeConvert.h"
#include "audio_encode.h"

// This is the class doing channel (mono->stereo) conversion

class AudioTypeChannel : public AudioTypeConvert {

protected:

public:
	AudioTypeChannel();	// Constructor
	~AudioTypeChannel();	// Destructor

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

#endif /* !_MULTIMEDIA_AUDIOTYPECHANNEL_H */
