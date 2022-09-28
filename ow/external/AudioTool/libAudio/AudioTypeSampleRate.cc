/* Copyright (c) 1992 by Sun Microsystems, Inc. */
#ident	"@(#)AudioTypeSampleRate.cc	1.5	92/12/04 SMI"

#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "AudioDebug.h"

#include "AudioTypeSampleRate.h"

// This is the first stab at a conversion class for Sample Rate conversions

// class AudioTypeSampleRate methods

// Constructor
AudioTypeSampleRate::
AudioTypeSampleRate(int inrate, int outrate) : 
	resampler(inrate, outrate), input_rate(inrate), output_rate(outrate)
{
}

// Destructor
AudioTypeSampleRate::
~AudioTypeSampleRate()
{
}

// Test conversion possibilities.
// Return TRUE if conversion to/from the specified type is possible.
Boolean AudioTypeSampleRate::
CanConvert(
	AudioHdr	h) const		// target header
{
	if ((input_rate <= 0) || (output_rate <= 0))
		return (FALSE);
	if ((h.encoding != LINEAR) ||
	    ((h.sample_rate != output_rate) && (h.sample_rate != input_rate)) ||
	    (h.bytes_per_unit != 2) ||
	    (h.channels != 1)) {
		return (FALSE);
	}
	return (TRUE);
}


// Convert buffer to the specified type
// May replace the buffer with a new one, if necessary
AudioError AudioTypeSampleRate::
Convert(
	AudioBuffer*&	inbuf,			// data buffer to process
	AudioHdr	outhdr)			// target header
{
	AudioBuffer*	outbuf;
	AudioHdr	inhdr;
	Double		length;
	int		i;
	size_t		nsamps;
	size_t		insamps;
	AudioError	err;

	inhdr = inbuf->GetHeader();
	length = inbuf->GetLength();

	if (Undefined(length)) {
		return (AUDIO_ERR_BADARG);
	}

	// Make sure we're not being asked to do the impossible
	// XXX - need a better error code
	if ((err = inhdr.Validate()) || (err = outhdr.Validate())) {
		return (err);
	}

	// If the requested conversion is different than what was initially
	// established, then return an error.
	// XXX - Maybe one day flush and re-init the filter
	if ((inhdr.sample_rate != input_rate) ||
	    (outhdr.sample_rate != output_rate)) {
		return (AUDIO_ERR_BADARG);
	}

	// If conversion is a no-op, just return success
	if (inhdr.sample_rate == outhdr.sample_rate) {
		return (AUDIO_SUCCESS);
	}

	// If nothing in the buffer, do the simple thing
	if (length == 0.) {
		inbuf->SetHeader(outhdr);
		return (AUDIO_SUCCESS);
	}

	// Add some padding to the output buffer
	i = 4 * ((input_rate / output_rate) + (output_rate / input_rate));
	length += outhdr.Samples_to_Time(i);

	// Allocate a new buffer
	outbuf = new AudioBuffer(length, "(SampleRate conversion buffer)");
	if (outbuf == 0)
		return (AUDIO_UNIXERROR);
	if (err = outbuf->SetHeader(outhdr)) {
		delete outbuf;
		return (err);
	}

	// here's where the guts go ...
	nsamps = resampler.filter((short*)inbuf->GetAddress(),
		   (int)inbuf->GetHeader().Time_to_Samples(inbuf->GetLength()),
		   (short*)outbuf->GetAddress());

	// do a sanity check. did we write more bytes then we had
	// available in the output buffer?
	insamps = (unsigned int) 
		outbuf->GetHeader().Time_to_Samples(outbuf->GetSize());

	AUDIO_DEBUG((2, "TypeResample: after filter, insamps=%d, outsamps=%d\n",
		     insamps, nsamps));
	
	if (nsamps > outbuf->GetHeader().Time_to_Samples(outbuf->GetSize())) {
		AudioStderrMsg(outbuf, AUDIO_NOERROR, Fatal,
		   "resample filter corrupted the heap");
	}

	// set output size appropriately
	outbuf->SetLength(outbuf->GetHeader().Samples_to_Time(nsamps));

#ifdef notdef
	// XXX - this is probably wrong: zero return means that there
	//       was not enough input data to generate an output sample
	// error
	if (nsamps == 0) {
		if (outbuf != inbuf)
			delete outbuf;
		return (AUDIO_ERR_BADARG); // is this appropriate?
	}
#endif

	// This will delete the buffer
	inbuf->Reference();
	inbuf->Dereference();

	inbuf = outbuf;
	return (AUDIO_SUCCESS);
}

AudioError AudioTypeSampleRate::
Flush(
	AudioBuffer*&	outbuf)
{
	AudioHdr	h;
	Double		pos;
	int		nsamp;
	size_t		cnt;
	AudioError	err;
	unsigned char*	tmpbuf;

	if (outbuf == NULL)
		return (AUDIO_SUCCESS);
	h = outbuf->GetHeader();

	nsamp = resampler.getFlushSize();
	if (nsamp > 0) {
		cnt = (size_t) nsamp * h.bytes_per_unit;
		tmpbuf = new unsigned char[cnt];

		// this does a flush
		nsamp = resampler.filter(NULL, 0, (short*)tmpbuf);

		// Copy to the supplied buffer
		if (nsamp > 0) {
			cnt = (size_t) nsamp * h.bytes_per_unit;
			pos = outbuf->GetLength();
			err = outbuf->AppendData(tmpbuf, cnt, pos);
			if (err)
				return (err);
		}
		delete tmpbuf;
	}
	return (AUDIO_SUCCESS);
}

#ifdef notdef
void AudioTypeSampleRate::
ReInit(AudioHdr inhdr, AudioHdr outhdr);
{
	if (resampler) {
		delete (resampler);
	}

	resampler = new ResampleFilter(inhdr.sample_rate, outhdr.sample_rate);
}
#endif
