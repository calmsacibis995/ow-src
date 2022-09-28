/* Copyright (c) 1992 by Sun Microsystems, Inc. */
#ident	"@(#)AudioTypeMux.cc	1.2	92/12/04 SMI"

#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "AudioTypeMux.h"

// This is a conversion class for channel multiplex/demultiplex

// class AudioTypeMux methods

// Constructor
AudioTypeMux::
AudioTypeMux()
{
}

// Destructor
AudioTypeMux::
~AudioTypeMux()
{
}

// Test conversion possibilities.
// Return TRUE if conversion to/from the specified type is possible.
Boolean AudioTypeMux::
CanConvert(
	AudioHdr	/*h*/) const		// target header
{
	// XXX - The test is whether we're converting 1->many or many->1
	//       This routine needs a to/from argument.
	// XXX - What if the format doesn't have fixed-size sample units?
	return (TRUE);
}

// Multiplex or demultiplex.
// The buffer pointer should be a NULL-terminated array of buffers if 1-channel
AudioError AudioTypeMux::
Convert(
	AudioBuffer*&	inbuf,			// data buffer to process
	AudioHdr	outhdr)			// target header
{
	AudioBuffer*	outbuf;
	AudioBuffer**	multibuf;
	AudioHdr	inhdr;
	Double		length;
	unsigned int	channels;
	size_t		nsamps;
	size_t		nbytes;
	size_t		unitsz;
	unsigned char**	inptrs;
	unsigned char*	in;
	unsigned char*	out;
	int		i;
	int		j;
	int		k;
	AudioError	err;

	channels = outhdr.channels;
	if (channels == 1) {
		inhdr = inbuf->GetHeader();	// Demux multi-channel data
		length = inbuf->GetLength();
	} else {
		multibuf = (AudioBuffer**) inbuf;	// Mux multiple buffers
		inhdr = multibuf[0]->GetHeader();
		length = multibuf[0]->GetLength();
	}

	// Make sure we're not being asked to do the impossible or trivial
	if ((err = inhdr.Validate()))
		return (err);
	if ((inhdr.sample_rate != outhdr.sample_rate) ||
	    (inhdr.encoding != outhdr.encoding) ||
	    (inhdr.samples_per_unit != outhdr.samples_per_unit) ||
	    (inhdr.bytes_per_unit != outhdr.bytes_per_unit))
		return (AUDIO_ERR_HDRINVAL);
	if (inhdr.channels == outhdr.channels)
		return (AUDIO_SUCCESS);
	if ((inhdr.channels != 1) && (outhdr.channels != 1))
		return (AUDIO_ERR_HDRINVAL);
	if (Undefined(length))
		return (AUDIO_ERR_BADARG);

	// Get the number of sample frames and the size of each
	nsamps = (size_t) inhdr.Time_to_Samples(length);
	nbytes = (size_t) inhdr.FrameLength();
	unitsz = (size_t) inhdr.bytes_per_unit;

	// Figure out if we're multiplexing or demultiplexing
	if (channels == 1) {
		// Demultiplex multi-channel data into several mono channels

		// Allocate buffer pointer array and each buffer
		channels = inhdr.channels;
		multibuf = (AudioBuffer**)
		    calloc((channels + 1), sizeof (AudioBuffer*));
		for (i = 0; i < channels; i++) {
			multibuf[i] = new AudioBuffer(length,
			    "(Demultiplex conversion buffer)");
			if (multibuf[i] == 0) {
				err = AUDIO_UNIXERROR;
				goto cleanup;
			}
			if (err = multibuf[i]->SetHeader(outhdr)) {
				delete multibuf[i];
cleanup:			while (--i >= 0) {
					delete multibuf[i];
				}
				delete multibuf;
				return (err);
			}
		}
		multibuf[i] = NULL;

		for (i = 0; i < channels; i++) {
			// Get output pointer and input channel pointer
			out = (unsigned char*)multibuf[i]->GetAddress();
			in = (unsigned char*)inbuf->GetAddress();
			in += (i * unitsz);

			// Copy a sample unit and bump the input pointer
			for (j = 0; j < nsamps; j++) {
				for (k = 0; k < unitsz; k++) {
					*out++ = *in++;
				}
				in += ((channels - 1) * unitsz);
			}

			// Set the valid data length
			multibuf[i]->SetLength(length);
		}
		// Release the input buffer
		inbuf->Reference();
		inbuf->Dereference();

		// Return the array pointer (callers beware!)
		inbuf = (AudioBuffer*) multibuf;

	} else {
		// Multiplex several mono channels into multi-channel data

		// Allocate an output buffer
		outbuf = new AudioBuffer(length,
		    "(Multiplex conversion buffer)");
		if (outbuf == 0)
			return (AUDIO_UNIXERROR);
		if (err = outbuf->SetHeader(outhdr)) {
			delete outbuf;
			return (err);
		}

		// Verify the input pointer is an array of buffer pointers
		multibuf = (AudioBuffer**) inbuf;
		for (channels = 0; ; channels++) {
			// Look for NULL termination
			if (multibuf[channels] == NULL)
				break;
			if (!multibuf[channels]->isBuffer())
				return (AUDIO_ERR_BADARG);
		}
		if (channels != outhdr.channels)
			return (AUDIO_ERR_BADARG);

		// Allocate a bunch of input pointers
		inptrs = (unsigned char**)
		    calloc(channels, sizeof (unsigned char*));
		for (i = 0; i < channels; i++) {
			inptrs[i] = (unsigned char*) multibuf[i]->GetAddress();
		}

		// Get output pointer
		out = (unsigned char*)outbuf->GetAddress();

		for (i = 0; i < nsamps; i++) {
			// Copy a sample frame from each input buffer
			for (j = 0; j < channels; j++) {
				in = inptrs[j];
				for (k = 0; k < nbytes; k++) {
					*out++ = *in++;
				}
				inptrs[j] = in;
			}
		}
		// Set the valid data length
		outbuf->SetLength(length);

		// Release the input buffers and pointer arrays
		for (i = 0; i < channels; i++) {
			multibuf[i]->Reference();
			multibuf[i]->Dereference();
			multibuf[i] = NULL;
		}
		delete multibuf;
		delete inptrs;

		// Set the valid data length and return the new pointer
		outbuf->SetLength(length);
		inbuf = outbuf;
	}
	return (AUDIO_SUCCESS);
}

AudioError AudioTypeMux::
Flush(
	AudioBuffer*&	/*buf*/)
{
	return (AUDIO_SUCCESS);
}
