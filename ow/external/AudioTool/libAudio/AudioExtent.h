/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOEXTENT_H
#define	_MULTIMEDIA_AUDIOEXTENT_H

#ident	"@(#)AudioExtent.h	1.8	91/08/16 SMI"

#include <values.h>
#include "Audio.h"


// This class defines an extent of a referenced audio class
class AudioExtent : public Audio {
private:
	Audio*			ref;		// reference to audio object
	Double			start;		// start time
	Double			end;		// end time

	AudioExtent operator=(AudioExtent);		// Assignment is illegal

public:
	AudioExtent(					// Constructor
	    Audio* obj,				// audio object
	    double s=0.,			// start time
	    double e=AUDIO_UNKNOWN_TIME);	// end time
	virtual ~AudioExtent();				// Destructor

	Audio* GetRef() const;				// Get audio obj
	void SetRef(Audio* r);				// Set audio obj
	Double GetStart() const;			// Get start time
	void SetStart(Double s);			// Set start time
	Double GetEnd() const;				// Get end time
	void SetEnd(Double e);				// Set end time

	// class Audio methods specialized here
	virtual Double GetLength() const;		// Get length, in secs
	virtual char* GetName() const;			// Get name string
	virtual AudioHdr GetHeader();			// Get header
	virtual AudioHdr GetHeader(Double pos);		// Get header at pos

	virtual AudioError ReadData(			// Read from position
	    void* buf,				// buffer to fill
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)

	virtual AudioError WriteData(			// Write is prohibited
	    void* buf,				// buffer to copy
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)

	virtual Boolean isExtent() const { return (TRUE); }
};

#endif /* !_MULTIMEDIA_AUDIOEXTENT_H */
