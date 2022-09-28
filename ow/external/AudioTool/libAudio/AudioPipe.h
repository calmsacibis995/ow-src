/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOPIPE_H
#define	_MULTIMEDIA_AUDIOPIPE_H

#ident	"@(#)AudioPipe.h	1.8	90/10/11 SMI"

#include "AudioUnixfile.h"


// This is the 'base' class for pipes (such as stdin) containing audio data
class AudioPipe : public AudioUnixfile {
private:
	AudioPipe();					// Constructor w/no args
	AudioPipe operator=(AudioPipe);			// Assignment is illegal

public:
	AudioPipe(					// Constructor with path
	    const int		desc,		// file descriptor
	    const FileAccess	acc,		// access mode
	    const char*		name="(pipe)");	// name

	// class AudioUnixfile methods specialized here
	virtual AudioError Create();			// Create file
	virtual AudioError Open();			// Open file
	virtual AudioError ReadData(			// Read from position
	    void* buf,				// buffer to fill
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)
	virtual AudioError WriteData(			// Write at position
	    void* buf,				// buffer to copy
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)

	// class Audio methods specialized here
	virtual Boolean isPipe() const { return (TRUE); }
};

#endif /* !_MULTIMEDIA_AUDIOPIPE_H */
