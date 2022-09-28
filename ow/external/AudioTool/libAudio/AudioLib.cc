/* Copyright (c) 1990 by Sun Microsystems, Inc. */
#ident	"@(#)AudioLib.cc	1.2	91/05/07 SMI"

#include "Audio.h"
#include "AudioFile.h"
#include "AudioList.h"
#include "AudioLib.h"

// Generic Audio functions


// Open an audio file readonly, and return an AudioList referencing it.
AudioError
Audio_OpenInputFile(
	const char*	path,		// input filename
	Audio*&		ap)		// returned AudioList pointer
{
	AudioFile*	inf;
	AudioList*	lp;
	AudioError	err;

	// Open file and decode the header
	inf = new AudioFile(path, ReadOnly);
	if (inf == 0)
		return (AUDIO_UNIXERROR);
	err = inf->Open();
	if (err) {
		delete inf;
		return (err);
	}

	// Create a list object and set it up to reference the file
	lp = new AudioList;
	if (lp == 0) {
		delete inf;
		return (AUDIO_UNIXERROR);
	}
	lp->Insert(inf);
	ap = lp;
	return (AUDIO_SUCCESS);
}


// Create an output file and copy an input stream to it.
// If an error occurs during output, leave a partially written file.
AudioError
Audio_WriteOutputFile(
	const char*	path,		// output filename
	const AudioHdr&	hdr,		// output data header
	Audio*		input)		// input data stream
{
	AudioFile*	outf;
	AudioError	err;

	// Create output file object
	outf = new AudioFile(path, WriteOnly);
	if (outf == 0)
		return (AUDIO_UNIXERROR);

	// Set audio file header and create file
	if ((err = outf->SetHeader(hdr)) || (err = outf->Create())) {
		delete outf;
		return (err);
	}

	// Copy data to file
	err = AudioCopy(input, outf);

	// Close output file and clean up.  If error, leave partial file.
	delete outf;
	return (err);
}
