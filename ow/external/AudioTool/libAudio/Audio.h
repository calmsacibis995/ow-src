/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIO_H
#define	_MULTIMEDIA_AUDIO_H

#ident	"@(#)Audio.h	1.22	97/01/14 SMI"

#include "AudioTypes.h"
#include "AudioError.h"
#include "AudioHdr.h"

// Error-handling function declaration
class Audio;
typedef Boolean	(*AudioErrfunc)(const Audio*, AudioError, AudioSeverity, char*);


// Data transfer subcodes.
// Returned from ReadData(), WriteData(), AsyncCopy(), Copy() in err.sys
typedef enum AudioCopyFlag {
    AUDIO_COPY_SHORT_INPUT = 100,	// AUDIO_SUCCESS: input would block
    AUDIO_COPY_ZERO_LIMIT = 101,	// AUDIO_SUCCESS: length was zero
    AUDIO_COPY_SHORT_OUTPUT = 102,	// AUDIO_SUCCESS: only partial output
    AUDIO_COPY_INPUT_EOF = 103,		// AUDIO_EOF: eof on input
    AUDIO_COPY_OUTPUT_EOF = 104		// AUDIO_EOF: eof on output
};



// This is the abstract base class from which all audio data types derive.
// It is invalid to create an object of type Audio.

class Audio {
private:
	static int	idctr;			// id seed value

	int		id;			// object id number
	int		refcnt;			// reference count
	char*		name;			// name
	Double		readpos;		// current read position ptr
	Double		writepos;		// current write position ptr
	AudioErrfunc	errorfunc;		// address of error function

	Audio operator=(Audio);		       	// Assignment is illegal

protected:
	void SetName(const char* str);	       	// Set name string
	Double setpos(		        	// Set position
	    Double& pos,			// position field to update
	    Double newpos,			// new position
	    Whence w=Absolute);			// Absolute || Relative

// XXX - should these be protected?
public:
	int getid() const;		        // Get id value

	virtual AudioError RaiseError(		// Raise error code
	    AudioError code,			// error code
	    AudioSeverity sev= Error,		// error severity
	    char* msg="") const;		// error message
	virtual void PrintMsg(			// Raise error msg
	    char* msg,				// error code
	    AudioSeverity sev=Message) const;	// error severity

public:
	Audio(const char* str="");		// Constructor
	virtual ~Audio();			// Destructor

	void Reference();			// Increment ref count
	void Dereference();			// Decrement ref count
	Boolean isReferenced() const;		// TRUE if referenced

	virtual char* GetName() const;		// Get name string
	virtual void SetErrorFunction(		// Set user error func
	    AudioErrfunc func);			// return TRUE if non-fatal

	virtual Double ReadPosition() const;	// Get read position
	virtual Double WritePosition() const;	// Get write position
	virtual Double SetReadPosition(		// Set read position
	    Double pos,				// new position
	    Whence w=Absolute);			// Absolute || Relative
	virtual Double SetWritePosition(	// Set write position
	    Double pos,				// new position
	    Whence w=Absolute);			// Absolute || Relative

	virtual AudioError Read(		// Read from current pos
	    void* buf,				// buffer to fill
	    size_t& len);			// buffer length (updated)
	virtual AudioError Write(		// Write to current pos
	    void* buf,				// buffer to copy
	    size_t& len);			// buffer length (updated)

	// XXX - no Append() method for now because of name clashes

	// methods specialized by inherited classes
	virtual AudioHdr GetHeader() = 0;	// Get header
	virtual AudioHdr GetHeader(Double);	// Get header at pos
	virtual Double GetLength() const = 0;		// Get length, in secs

	virtual AudioError ReadData(			// Read from position
	    void* buf,				// buffer to fill
	    size_t& len,			// buffer length (updated)
	    Double& pos) = 0;			// start position (updated)
	virtual AudioError WriteData(			// Write at position
	    void* buf,				// buffer to copy
	    size_t& len,			// buffer length (updated)
	    Double& pos) = 0;			// start position (updated)
	virtual AudioError AppendData(			// Write and extend
	    void* buf,				// buffer to copy
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)

	virtual AudioError Copy(		// copy to another audio obj.
	    Audio* ap);				// dest audio object

	virtual AudioError Copy(		// copy to another audio obj.
	    Audio* ap,				// dest audio object
	    Double& frompos,
	    Double& topos,
	    Double& limit);

	virtual AudioError AsyncCopy(		// copy to another audio obj.
	    Audio* ap,				// dest audio object
	    Double& frompos,
	    Double& topos,
	    Double& limit);

	// Define default classification routines
	// The appropriate routine should be specialized by each leaf class.
	virtual Boolean isFile() const { return (FALSE); }
	virtual Boolean isDevice() const { return (FALSE); }
	virtual Boolean isDevicectl() const { return (FALSE); }
	virtual Boolean isPipe() const { return (FALSE); }
	virtual Boolean isBuffer() const { return (FALSE); }
	virtual Boolean isExtent() const { return (FALSE); }
	virtual Boolean isList() const { return (FALSE); }
};

#include "Audio_inline.h"

#endif /* !_MULTIMEDIA_AUDIO_H */
