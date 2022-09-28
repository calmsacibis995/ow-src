/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIODEVICE_H
#define	_MULTIMEDIA_AUDIODEVICE_H

#ident	"@(#)AudioDevice.h	1.30	97/01/14 SMI"

#ifdef SUNOS41
// Get the local copy of the system file before the C++ private version
#include "../include/sun/audioio.h"
#else
#include <sys/ioccom.h>
#include <sys/audioio.h>
#endif

#include "AudioUnixfile.h"

// Enumerate known device types
enum AudioDeviceType {
	AudioDeviceUnknown,		// unknown device type
	AudioDeviceAMD,			// AM79C30
	AudioDeviceDBRI,		// DBRI & MMCodec (SpeakerBox)
	AudioDeviceSBPRO,		// SoundBlaster Pro
	AudioDeviceSB16,		// SoundBlaster 16
	AudioDeviceSPECTRUM,		// MediaVision Audio Spectrum 16
	AudioDeviceMULTISOUND,		// Turtle Beach MultiSound
	AudioDeviceCODEC		// MMCodec with no DBRI
};

// The audio information structure is defined as a class so that
// it is automatically initialized whenever allocated.
class AudioInfo {
private:
	audio_info_t	info;			// device info structure
public:
	AudioInfo();					// Constructor
	audio_info_t* operator ->()			// Cast to info ptr
		{ return (&info); }
	void	Clear();				// Reset
};

// Audio device encoding structure (play/record state)
typedef audio_prinfo_t	AudioPRinfo;


// This is the 'base' class for audio devices
//
// Since audio devices can theoretically have separate input and output
// encoding formats, there are separate methods for input and output headers.
// For compatibility with other AudioStream classes, GetHeader() gets the
// input data encoding and SetHeader() sets the output data encoding.

class AudioDevice : public AudioUnixfile {
private:
	static const FileAccess	defmode;	// Default access mode
	static const char*	AUDIO_ENV;	// Default device env name
	static const char*	AUDIO_DEV;	// Default device name

	AudioHdr		readhdr;	// input data encoding
	AudioDeviceType		devtype;	// device type

	AudioDevice operator=(AudioDevice);		// Assignment is illegal

protected:
	virtual AudioError tryopen(		// open with a given pathname
	    const char*, int);
	virtual void decode_devtype();		// figure out the device type
	virtual void clearhdr();		// clear cached readhdr
	virtual AudioError info_to_hdr(
	    const AudioPRinfo& prinfo,		// device encoding info
	    AudioHdr& h) const;			// header to set
	virtual AudioError hdr_to_info(
	    const AudioHdr& h,			// header to encode
	    AudioPRinfo& prinfo) const;		// output device encoding info

	virtual Double scale_gain(unsigned int);	// gain -> float
	virtual unsigned int unscale_gain(Double);	// float -> gain
	virtual Double scale_balance(unsigned int);	// balance -> float
	virtual unsigned int unscale_balance(Double);	// float -> balance
	virtual AudioError incr_volume(			// change volume a notch
	    Boolean,				// true to raise, false to lower
	    AudioInfo&,				// info structure
	    unsigned int*);			// ptr to gain field in info
	virtual Boolean rate_match(			// Check rate tolerance
	    unsigned int, unsigned int);

	// class AudioUnixfile methods specialized here
	virtual AudioError decode_filehdr()		// No-op for devices
	    { return (AUDIO_ERR_BADARG); }
	virtual AudioError encode_filehdr()		// No-op for devices
	    { return (AUDIO_ERR_BADARG); }

	// class AudioStream methods specialized here
	virtual Boolean opened() const;			// TRUE, if open

public:
	AudioDevice(					// Constructor with path
	    const char*	path="",		// device name
	    const FileAccess acc=defmode);	// access mode
	virtual ~AudioDevice();				// Destructor

	// class AudioUnixfile methods specialized here
	virtual AudioError Create();			// Create file
	virtual AudioError Open();			// Open file
	virtual AudioError OpenPath(			// ... with search path
	    const char* path=AUDIO_ENV);

	virtual AudioError ReadData(			// Read from position
	    void* buf,				// buffer to fill
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)
	virtual AudioError WriteData(			// Write at position
	    void* buf,				// buffer to copy
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)

	// class AudioStream methods specialized here
	virtual AudioError SetHeader(			// Set header
	    const AudioHdr& h);			// header to copy

	// class Audio methods specialized here
	virtual AudioHdr GetHeader();		// Get header

	virtual Boolean isDevice() const { return (TRUE); }

	// Device control and status functions
	virtual AudioDeviceType GetDeviceType() const;	// Get device type
	virtual AudioError GetState(			// Get device state
	    AudioInfo& info) const;
	virtual AudioError SetState(			// Set device state
	    AudioInfo& info);
	virtual AudioError SetSignal(			// Turn SIGPOLL on/off
	    Boolean on);

	virtual Boolean CanSetHeader(		// TRUE if device-compatible hdr
	    AudioHdr& h);			// header to check
	virtual AudioError SetReadHeader(		// Set input encoding
	    AudioHdr& h);			// header to copy (updated)
	virtual AudioError SetWriteHeader(		// Set output encoding
	    AudioHdr& h);			// header to copy (updated)
	virtual AudioHdr GetReadHeader();		// Get input encoding
	virtual AudioHdr GetWriteHeader();		// Get output encoding

	virtual AudioError WriteEof();			// Write eof sync flag
	virtual AudioError Flush(const FileAccess);	// Flush data
	virtual AudioError DrainOutput();		// Wait for output

	virtual AudioError Pause(const FileAccess);	// Set pause flags
	virtual AudioError Resume(const FileAccess);	// Clear pause flags

	virtual AudioError SetPlayEof(unsigned&);	// Play eof counter
	virtual AudioError SetPlaySamples(unsigned&);	// Play sample count
	virtual AudioError SetRecSamples(unsigned&);	// Record sample count
	virtual AudioError SetPlayError(Boolean&);	// Play error flag
	virtual AudioError SetRecError(Boolean&);	// Record error flag
	virtual AudioError SetPlayWaiting();		// Set Play waiting
	virtual AudioError SetRecWaiting();		// Set Record waiting
	virtual AudioError SetRecDelay(Double&);	// Set Record delay

	virtual AudioError SetPlayVolume(Double&);	// Play volume
	virtual AudioError PlayVolumeUp();		// Raise volume a notch
	virtual AudioError PlayVolumeDown();		// Lower volume a notch
	virtual AudioError SetRecVolume(Double&);	// Record volume
	virtual AudioError RecVolumeUp();		// Raise volume a notch
	virtual AudioError RecVolumeDown();		// Lower volume a notch
	virtual AudioError SetMonVolume(Double&);	// Monitor volume
	virtual AudioError MonVolumeUp();		// Raise volume a notch
	virtual AudioError MonVolumeDown();		// Lower volume a notch
	virtual AudioError SetPlayBalance(Double&);	// Set balance
	virtual AudioError SetRecBalance(Double&);	// Set balance

	virtual Double GetPlayVolume(AudioInfo* = 0);	// Play volume
	virtual Double GetRecVolume(AudioInfo* = 0);	// Record volume
	virtual Double GetMonVolume(AudioInfo* = 0);	// Monitor volume
	virtual Double GetPlayBalance(AudioInfo* = 0);	// Play balance
	virtual Double GetRecBalance(AudioInfo* = 0);	// Record balance
	virtual unsigned GetPlaySamples(AudioInfo* = 0);// Play sample count
	virtual unsigned GetRecSamples(AudioInfo* = 0);	// Record sample count
	virtual Boolean GetPlayOpen(AudioInfo* = 0);	// Get Play open flag
	virtual Boolean GetRecOpen(AudioInfo* = 0);	// Get Record open flag
	virtual Boolean GetPlayWaiting(AudioInfo* = 0);	// Get Play waiting
	virtual Boolean GetRecWaiting(AudioInfo* = 0);	// Get Record waiting
};

#endif /* !_MULTIMEDIA_AUDIODEVICE_H */
