/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOLIST_H
#define	_MULTIMEDIA_AUDIOLIST_H

#ident	"@(#)AudioList.h	1.10	96/02/20 SMI"

#include "Audio.h"


// This is the 'base' class for a list of extents of audio objects
class AudioList : public Audio {

	// Define a linked list nested class
	class AudioListEntry {
	private:
		void operator=(AudioListEntry);		// Assignment is illegal
	public:
		Audio*		aptr;		// pointer to audio object
		AudioListEntry*	next;		// pointer to next in list
		AudioListEntry*	prev;		// pointer to previous

		AudioListEntry(				// Constructor w/obj
		    Audio* obj);		// referenced audio object
		~AudioListEntry();			// Destructor

		void newptr(Audio* newa);		// Reset extent
		void link(				// Link in new extent
		    AudioListEntry* after);	// link after this one
		void split(				// Split an extent
		    Double pos);		// split at offset
	};

private:
	AudioListEntry		head;		// list head

	AudioListEntry* first() const;		// Return first extent
	virtual Boolean getposition(		// Locate extent/offset
	    Double& pos,			// target position (updated)
	    AudioListEntry*& ep) const;		// returned entry pointer

	AudioList operator=(AudioList);		// Assignment is illegal

public:
	AudioList(const char* name="[list]");	// Constructor
	virtual ~AudioList();			// Destructor

	// class Audio methods specialized here
	virtual Double GetLength() const;	// Get length, in secs
	virtual char* GetName() const;		// Get name string
	virtual AudioHdr GetHeader();		// Get header
	virtual AudioHdr GetHeader(Double pos);	// Get header at pos

	virtual AudioError ReadData(		// Read from position
	    void* buf,				// buffer to fill
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)

	virtual AudioError WriteData(			// Write is prohibited
	    void* buf,				// buffer to copy
	    size_t& len,			// buffer length (updated)
	    Double& pos);			// start position (updated)

	virtual Boolean isList() const { return (TRUE); }

	// list manipulation methods
	virtual AudioError Insert(			// Insert an entry
	    Audio* obj);			// object to insert
	virtual AudioError Insert(			// Insert an entry
	    Audio* obj,				// object to insert
	    Double pos);			// insertion offset, in seconds
	virtual AudioError Append(			// Append an entry
	    Audio* obj);			// object to append

	virtual AudioError AsyncCopy(		// copy to another audio obj.
	    Audio* ap,				// dest audio object
	    Double& frompos,
	    Double& topos,
	    Double& limit);
};

// XXX - C++ 2.1 can't handle nested classes properly (C++ 4.0 does)
//typedef AudioList::AudioListEntry AudioList_AudioListEntry;

#endif /* !_MULTIMEDIA_AUDIOLIST_H */
