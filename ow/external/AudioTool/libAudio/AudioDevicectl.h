/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIODEVICECTL_H
#define	_MULTIMEDIA_AUDIODEVICECTL_H

#ident	"@(#)AudioDevicectl.h	1.3	96/02/20 SMI"

#include "AudioDevice.h"


// This is the audio control device class
//
// The audio control device cannot be read or written.

class AudioDevicectl : public AudioDevice {
private:
	AudioDevicectl operator=(AudioDevicectl);	// Assignment is illegal

protected:

public:
	AudioDevicectl(					// Constructor with path
	    const char*	path="");		// default device
	virtual ~AudioDevicectl() {}			// Destructor

	// class AudioDevice methods specialized here
	virtual AudioError tryopen(		// open with a given pathname
	    const char*, int);

	// class Audio methods specialized here
	virtual AudioHdr GetReadHeader();		// Get header

	// Device control and status functions
	virtual AudioError SetSignal(			// Turn SIGPOLL on/off
	    Boolean on);


	// No-op methods
	virtual AudioError Create()
	    {return (RaiseError(AUDIO_ERR_NOEFFECT));}
	virtual AudioError ReadData(void*, size_t&, Double&)
	    {return (RaiseError(AUDIO_ERR_NOEFFECT));}
	virtual AudioError WriteData(void*, size_t&, Double&)
	    {return (RaiseError(AUDIO_ERR_NOEFFECT));}
	virtual AudioError SetHeader(const AudioHdr&)
	    {return (RaiseError(AUDIO_ERR_NOEFFECT));}
	virtual AudioError SetReadHeader(AudioHdr&)
	    {return (RaiseError(AUDIO_ERR_NOEFFECT));}
	virtual AudioError SetWriteHeader(AudioHdr&)
	    {return (RaiseError(AUDIO_ERR_NOEFFECT));}
	virtual AudioError WriteEof()
	    {return (RaiseError(AUDIO_ERR_NOEFFECT));}
	virtual AudioError Flush(const FileAccess)
	    {return (RaiseError(AUDIO_ERR_NOEFFECT));}
	virtual AudioError DrainOutput()
	    {return (RaiseError(AUDIO_ERR_NOEFFECT));}

	virtual Boolean isDevice() const { return (TRUE); } // XXX ??
	virtual Boolean isDevicectl() const { return (TRUE); }

};

#endif /* !_MULTIMEDIA_AUDIODEVICECTL_H */
