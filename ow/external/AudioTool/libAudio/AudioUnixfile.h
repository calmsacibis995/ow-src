/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOUNIXFILE_H
#define	_MULTIMEDIA_AUDIOUNIXFILE_H

#ident	"@(#)AudioUnixfile.h	1.11	92/10/22 SMI"

#include "AudioStream.h"


// This is the abstract base class for all file descriptor based audio i/o.
// It is invalid to create an object of type AudioUnixfile.

class AudioUnixfile : public AudioStream {
private:
	FileAccess	mode;			// access mode
	Boolean		block;			// FALSE if fd set non-blocking
	Boolean		filehdrset;		// TRUE if file hdr read/written
	int		fd;			// file descriptor
	char*		infostring;		// Info string from header
	unsigned int	infolength;		// Info string length

	AudioUnixfile() {}				// Constructor w/no args
	AudioUnixfile operator=(AudioUnixfile);		// Assignment is illegal

protected:
	AudioUnixfile(					// Constructor
	    const char* path,			// pathname
	    const FileAccess acc);		// access mode

	int getfd() const;				// Return descriptor
	void setfd(int d);				// Set descriptor

	virtual AudioError decode_filehdr();		// Get header from file
	virtual AudioError encode_filehdr();		// Write file header
	virtual AudioError seekread(			// Seek in input stream
	    Double pos,				// position to seek to
	    off_t& offset);			// returned byte offset
	virtual AudioError seekwrite(			// Seek in output stream
	    Double pos,				// position to seek to
	    off_t& offset);			// returned byte offset

	virtual Boolean isfdset() const;		// TRUE if fd is valid
	virtual Boolean isfilehdrset() const;		// TRUE if file hdr r/w

	// class AudioStream methods specialized here
	virtual Boolean opened() const;			// TRUE, if open

public:
	virtual ~AudioUnixfile();			// Destructor

	virtual FileAccess GetAccess() const;		// Get mode
	virtual Boolean GetBlocking() const;		// TRUE, if blocking i/o
	virtual void SetBlocking(Boolean b);		// Set block/non-block

	virtual AudioError Create() = 0;		// Create file
	virtual AudioError Open() = 0;			// Open file
	virtual AudioError OpenPath(			// ... with search path
	    const char* path=0);
	virtual AudioError Close();			// Close file

	// Methods specific to the audio file format
	virtual char* const GetInfostring(		// Get info string
	    int& len) const;			// return length
	virtual void SetInfostring(			// Set info string
	    const char*	str,			// ptr to info data
	    int		len = -1);		// optional length

	// class Audio methods specialized here
	virtual AudioError ReadData(			// Read from position
	    void* buf,				// buffer to fill
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)
	virtual AudioError WriteData(			// Write at position
	    void* buf,				// buffer to copy
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)
};

#include "AudioUnixfile_inline.h"

#endif /* !_MULTIMEDIA_AUDIOUNIXFILE_H */
