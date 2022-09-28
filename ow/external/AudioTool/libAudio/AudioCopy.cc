/* Copyright (c) 1990 by Sun Microsystems, Inc. */
#ident	"@(#)AudioCopy.cc	1.13	96/02/20 SMI"

#include <stdlib.h>
#include <errno.h>
#include "Audio.h"
#include "AudioBuffer.h"
#include "AudioLib.h"

// Generic Audio functions


// Data format translation occurs transparently
AudioError
AudioCopy(
	Audio*		from,		// input source
	Audio*		to)		// output sink
{
	Double		frompos = 0. ;
	Double		topos   = 0. ;
	Double		limit   = AUDIO_UNKNOWN_TIME;

	return (AudioCopy(from, to, frompos, topos, limit));
}


// Copy a data stream
// Data format translation occurs transparently
AudioError
AudioCopy(
	Audio*		from,		// input source
	Audio*		to,		// output sink
	Double&		frompos,	// input position (updated)
	Double&		topos,		// output position (updated)
	Double&		limit)		// amount to copy (updated)
{
	return (from->Copy(to, frompos, topos, limit));
}

// Copy one segment of a data stream
// Data format translation occurs transparently
AudioError
AudioAsyncCopy(
	Audio*		from,		// input source
	Audio*		to,		// output sink
	Double&		frompos,	// input position (updated)
	Double&		topos,		// output position (updated)
	Double&		limit)		// amount to copy (updated)
{
	return (from->AsyncCopy(to, frompos, topos, limit));
}

#ifdef notyet
// State structure for copy operations
class AudioCopyState {
private:
	void operator=(AudioCopyState);			// Assignment is illegal
public:
	Audio*		from;		// input stream
	Audio*		to;		// output stream
	Double		frompos;	// input position
	Double		topos;		// output position
	Double		limit;		// transfer limit

	AudioBuffer*	ibuf;		// input buffer
	AudioBuffer*	obuf;		// output buffer
	Boolean		ilocal;		// TRUE, if ibuf allocated locally
	Boolean		olocal;		// TRUE, if obuf allocated locally
	size_t		iresid;		// residual data in ibuf
	size_t		oresid;		// residual data in obuf
	void*		ioff;		// offset to ibuf residual data
	void*		ooff;		// offset to obuf residual data

	AudioCopyState();				// Constructor
	~AudioCopyState();				// Destructor

	AudioError input_fill();			// Copy input to ibuf
	AudioError input_translate();			// Copy ibuf to obuf
	AudioError output_residual();			// Copy obuf to output
};

// Copy an entire data stream
// Copy input data to the input buffer.
AudioError AudioCopyState::
input_fill()
{
	size_t		cnt;
	size_t		frame;
	Double		svpos;
	AudioError	err;

	// XXX - Check consistency
	if (!ilocal) {
		from->Error(AUDIO_NOERROR, Fatal,
		    "input_fill called for input AudioBuffer");
	}

	// If an incomplete frame was read, try to complete it now
	frame = (size_t) from->GetHeader().FrameLength();
	cnt = resid % frame; 
	if (cnt != 0) {
		cnt = frame - cnt;
		err = from->ReadData((char*)ioff + iresid, cnt, frompos);
		resid += cnt;
		return (err);
	}

	// Return if nothing to do
	if (iresid > 0)
		return (AUDIO_SUCCESS);
	if (limit == 0.)
		return (AUDIO_EOF);

	// Read input stream and update residual count
	cnt = buf->GetByteCount();
	svpos = frompos;
	err = from->ReadData(ibuf->GetAddress(), cnt, frompos);
	limit -= frompos - svpos;
	iresid = cnt;
	ioff = ibuf->GetAddress();

	return (err);
}

// Copy residual input data to the output buffer, translating as necessary.
AudioError AudioCopyState::
input_translate()
{
	Double		cnt;
	AudioError	err;

	// Return if nothing to do
	if (iresid == 0.)
		return (AUDIO_SUCCESS);

	return (AUDIO_SUCCESS);
}

// Copy residual output data to the output stream.
AudioError AudioCopyState::
output_residual()
{
	size_t		cnt;
	AudioError	err;

	// Return if nothing to do
	if (oresid == 0)
		return (AUDIO_SUCCESS);

	// XXX - Check consistency
	if (!olocal) {
		to->Error(AUDIO_NOERROR, Fatal,
		    "output_residual called for output AudioBuffer");
	}

	// Copy residual output data to output stream and update residual count
	cnt = oresid;
	err = to->WriteData(ooff, cnt, topos);
	ooff += cnt;
	oresid -= cnt;
	if (oresid == 0.)
		ooff = 0;

	return (err);
}

#endif
