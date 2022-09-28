/* Copyright (c) 1991 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIORAWPIPE_H
#define	_MULTIMEDIA_AUDIORAWPIPE_H

#ident	"@(#)AudioRawPipe.h	1.2	92/02/04 SMI"

#include "AudioPipe.h"


// This is a specialization of an AudioPipe for raw audio data
class AudioRawPipe : public AudioPipe {
private:
	Boolean 	isopened;
	off_t		offset;	/* offset to start read/write */
public:
	AudioRawPipe(				// Constructor with path
	    const int		desc,		// file descriptor
	    const FileAccess	acc,		// access mode
	    const AudioHdr&	hdr,		// header describing raw data
	    const char*		name="(raw pipe)", // name
	    const off_t		offset = 0
	);

	// class AudioPipe methods specialized here
	virtual AudioError Create();			// Create file
	virtual AudioError Open();			// Open file

	virtual Boolean opened() const;			// TRUE of opened

	AudioError SetOffset(off_t val);		// set offset
	off_t GetOffset() const;			// set offset
};

#endif /* !_MULTIMEDIA_AUDIORAWPIPE_H */
