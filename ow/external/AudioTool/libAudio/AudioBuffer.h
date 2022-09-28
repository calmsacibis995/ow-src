/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOBUFFER_H
#define	_MULTIMEDIA_AUDIOBUFFER_H

#ident	"@(#)AudioBuffer.h	1.14	92/12/04 SMI"

#include "AudioStream.h"


// This is the class describing a mapped buffer of audio data.
// In addition to the standard Read and Write methods, the address
// of the buffer may be obtained and the data accessed directly.

class AudioBuffer : public AudioStream {
private:
	Double		buflen;			// buffer size, in seconds
	int		zflag;			// malloc'd with zmalloc?
protected:
	size_t		bufsize;		// buffer size, in bytes
	void*		bufaddr;		// buffer address

	// class AudioStream methods specialized here
	virtual Boolean opened() const;			// TRUE, if open
	virtual AudioError alloc();			// Allocate buffer

public:
	AudioBuffer(					// Constructor
	    double len = 0.,			// buffer size, in seconds
	    const char* name = "(buffer)");	// name
	~AudioBuffer();					// Destructor

	virtual void* GetAddress() const;		// Get buffer address
	virtual void* GetAddress(Double) const;		// Get address at offset
	virtual AudioError SetSize(Double len);		// Change buffer size
	virtual Double GetSize() const;			// Get buffer size
	virtual size_t GetByteCount() const;		// Get size, in bytes

	// class AudioStream methods specialized here
	virtual AudioError SetHeader(			// Set header
	    const AudioHdr& h);			// header to copy
	virtual void SetLength(				// Set data length
	    Double len);			// new length, in secs

	// class Audio methods specialized here
	virtual AudioError ReadData(			// Read from position
	    void* buf,				// buffer to fill
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)
	virtual AudioError WriteData(			// Write at position
	    void* buf,				// buffer to copy
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)
	virtual AudioError AppendData(			// Append at position
	    void* buf,				// buffer to copy
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)
	virtual AudioError AsyncCopy(		// copy to another audio obj.
	    Audio* to,				// dest audio object
	    Double& frompos,
	    Double& topos,
	    Double& limit);

	virtual Boolean isBuffer() const { return (TRUE); }
};

#endif /* !_MULTIMEDIA_AUDIOBUFFER_H */
