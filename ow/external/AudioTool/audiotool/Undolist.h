/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_UNDOLIST_H
#define	_AUDIOTOOL_UNDOLIST_H

#ident	"@(#)Undolist.h	1.5	96/02/20 SMI"

#include <multimedia/Audio.h>
#include <multimedia/AudioDetect.h>
#include "PlayRecord.h"


// Define Undolist class
class Undolist {

	// Define a linked list nested class
	class UndolistEntry {
	private:
		void operator=(UndolistEntry);		// Assignment is illegal
	public:
		Audio*		aptr;		// audio object
		AudioDetectPts*	dptr;		// cached detection array
		UndolistEntry*	next;		// next object in list
		UndolistEntry*	prev;		// previous object in list

		UndolistEntry(Audio* obj);		// Constructor
		~UndolistEntry();			// Destructor
		void setdetect(				// Set the detect array
		    AudioDetectPts* array);
		void link(				// Link in new entry
		    UndolistEntry* after);
		void unlink();				// Unlink tail
	};

private:
	UndolistEntry	head;			// list head
	UndolistEntry*	current;		// ptr to current entry
	AudioDetect*	dstate;			// detect state class
	PlayRecord*	pstate;			// play.record object

	void operator=(Undolist);			// Assignment is illegal
	UndolistEntry* first() const;			// Return first entry
public:
	Undolist();					// Constructor
	~Undolist();					// Destructor
	void clearlist();				// Clear entire list
	Audio* currentaudio();				// Get current audio obj

	void addentry(					// Link new entry
	    Audio* obj,				// audio object
	    AudioDetectPts* array=0);		// initial detection array
	Boolean canundo();				// TRUE, if undoable
	Boolean canredo();				// TRUE, if redoable
	Boolean undo();					// Backup position
	Boolean redo();					// Advance position
	Boolean undoall();				// Backup to start
	Boolean redoall();				// Advance to end

	void cleardetect();				// Clear all detect ptrs
	AudioDetectPts* currentdetect();		// Get current detect
	void setcurrentdetect(AudioDetectPts* val);	// Set current detect
	AudioDetect* detectstate();			// Get detect state

	PlayRecord* getplayrec();			// Get play object
};

// XXX - C++ 2.1 can't handle nested classes properly (C++ 4.0 does)
//typedef Undolist::UndolistEntry Undolist_UndolistEntry;

#endif /* !_AUDIOTOOL_UNDOLIST_H */
