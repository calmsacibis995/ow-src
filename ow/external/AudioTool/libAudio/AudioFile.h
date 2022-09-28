/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOFILE_H
#define	_MULTIMEDIA_AUDIOFILE_H

#ident	"@(#)AudioFile.h	1.16	97/01/14 SMI"

#include "AudioUnixfile.h"

#include <sys/types.h>
#include <sys/mman.h>

// A 'primitive type' for memory mapped file access types
enum vmaccess_t {
    NormalAccess = 0, RandomAccess = 1, SequentialAccess = 2
};

class VMAccess {
private:
	vmaccess_t	type;		// combined mode
public:
	VMAccess (vmaccess_t x = NormalAccess): type(x) { }	// Constructor
	inline operator vmaccess_t()			// Cast to enum
	    { return (type); }
	inline operator int() {				// Cast to integer
	    switch (type) {
	    case NormalAccess: return (MADV_NORMAL);
	    case RandomAccess: return (MADV_RANDOM);
	    case SequentialAccess: return (MADV_SEQUENTIAL);
	    }
	}
};


// This is the 'base' class for regular files containing audio data
class AudioFile : public AudioUnixfile {
private:
	static const FileAccess	defmode;	// Default access mode
	static const char*	AUDIO_PATH;	// Default path env name

	off_t			hdrsize;	// length of file header
	off_t			seekpos;	// current system file pointer
	Double			origlen;	// initial length of file

	caddr_t			mapaddr;	// for mmaping RO files
	off_t			maplen;		// length of mmaped region
	VMAccess		vmaccess;	// vm (mmap) access type

	AudioFile operator=(AudioFile);			// Assignment is illegal

protected:
	virtual AudioError tryopen(			// Open named file
	    const char*, int);
	virtual AudioError createfile(			// Create named file
	    const char* path);			// filename

	// class AudioUnixfile methods specialized here
	virtual AudioError seekread(			// Seek in input stream
	    Double pos,				// position to seek to
	    off_t& offset);			// returned byte offset
	virtual AudioError seekwrite(		// Seek in output stream
	    Double pos,				// position to seek to
	    off_t& offset);			// returned byte offset

public:
	AudioFile();				// Constructor w/no args
	AudioFile(				// Constructor with path
	    const char* path,			// filename
	    const FileAccess acc=defmode);	// access mode
	virtual ~AudioFile();			// Destructor

	static AudioError SetTempPath(		// Set tmpfile location
	    const char* path);			// directory path

	// class AudioUnixfile methods specialized here
	virtual void SetBlocking(Boolean) { }	// No-op for files

	AudioError SetAccessType(		// front end to madvise 
	    VMAccess vmacc);			// (normal, random, seq access)

	inline VMAccess GetAccessType()	const {	// get vm access type
		return(vmaccess);
	}

	virtual AudioError Create();		// Create file
	virtual AudioError Open();		// Open file
	virtual AudioError OpenPath(		// ... with search path
	    const char* path=AUDIO_PATH);
	virtual AudioError Close();		// Close file

	virtual AudioError ReadData(		// Read from position
	    void* buf,				// buffer to fill
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)
	virtual AudioError WriteData(		// Write at position
	    void* buf,				// buffer to copy
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)

	virtual AudioError AsyncCopy(		// copy to another audio obj.
	    Audio* ap,				// dest audio object
	    Double& frompos,
	    Double& topos,
	    Double& limit);

	// class Audio methods specialized here
	virtual Boolean isFile() const { return (TRUE); }
};

#endif /* !_MULTIMEDIA_AUDIOFILE_H */
