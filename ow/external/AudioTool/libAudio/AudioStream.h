/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOSTREAM_H
#define	_MULTIMEDIA_AUDIOSTREAM_H

#ident	"@(#)AudioStream.h	1.9	93/03/11 SMI"

#include "Audio.h"
#include "AudioHdr.h"
#include <stdlib.h>
#include "AudioDebug.h"

// This is the abstract base class for all audio data sources/sinks.
// It is invalid to create an object of type AudioStream.

class AudioStream : public Audio {
private:
	AudioHdr	hdr;			// data encoding info
	Double		length;			// length of data, in secs

	AudioStream operator=(AudioStream);		// Assignment is illegal

protected:

	Boolean hdrset() const;				// TRUE if header valid
	AudioError updateheader(			// Set header (always)
	    const AudioHdr& h);			// header to copy
	void setlength(					// Set data length
	    Double len);			// new length, in secs

	virtual Boolean opened() const = 0;		// TRUE if stream 'open'

public:
	AudioStream(const char* path="");		// Constructor

	virtual AudioError SetHeader(			// Set header
	    const AudioHdr& h);			// header to copy
	virtual void SetLength(				// Set data length
	    Double len);			// new length, in secs

	// XXX - is this needed?  do we need time->sample frames?
	virtual size_t GetByteCount() const;		// Get length, in bytes

	// class Audio methods specialized here
	virtual AudioHdr GetHeader();			// Get header
	virtual Double GetLength() const;		// Get length, in secs

	//Make sure endian of the data matches the current processor.
        AudioError coerceEndian(unsigned char* buf,size_t len, AudioEndian en);

	virtual Boolean isEndianSensitive() const;
        AudioEndian localByteOrder() const
        {
		return hdr.localByteOrder();
	}
};

#include "AudioStream_inline.h"

#endif /* !_MULTIMEDIA_AUDIOSTREAM_H */
