/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOTYPESAMPLERATE_H
#define	_MULTIMEDIA_AUDIOTYPESAMPLERATE_H

#ident	"@(#)AudioTypeSampleRate.h	1.1	92/06/05 SMI"

#include "AudioTypeConvert.h"
#include "Resample.h"

// This is the class doing Sample Rate conversion

class AudioTypeSampleRate : public AudioTypeConvert {
private:
	ResampleFilter	resampler;
	int 		input_rate;
	int		output_rate;

protected:

public:
	AudioTypeSampleRate(int inrate, int outrate); // Constructor
	~AudioTypeSampleRate();				// Destructor

	// Class AudioTypeConvert methods specialized here
	virtual Boolean CanConvert(		// TRUE if conversion ok
	    AudioHdr h) const;			// type to check against

	// Convert buffer to the specified type
	// Either the input or output type must be handled by this class
	virtual AudioError Convert(		// Convert to new type
	    AudioBuffer*& inbuf,		// data buffer to process
	    AudioHdr outhdr);			// target header

	virtual AudioError Flush(AudioBuffer*& buf);	// flush remains
#ifdef notdef
	virtual void ReInit(AudioHdr inhdr, AudioHdr outhdr);	// reset state
#endif
};

#endif /* !_MULTIMEDIA_AUDIOTYPESAMPLERATE_H */
