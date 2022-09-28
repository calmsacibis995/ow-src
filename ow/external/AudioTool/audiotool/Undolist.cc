/* Copyright (c) 1990 by Sun Microsystems, Inc. */
#ident	"@(#)Undolist.cc	1.6	96/02/20 SMI"

#include "Undolist.h"

// Undo List manipulation routines


// Constructor takes audio object ptr as argument
Undolist::UndolistEntry::	// UndolistEntry::
UndolistEntry(
	Audio* obj):			// audio object
	dptr(0), next(0), prev(0)
{
	// Reference audio object, to enable garbage collection if nothing else
	aptr = obj;
	if (obj != 0)
		obj->Reference();
}

// Destructor
Undolist::UndolistEntry::	// UndolistEntry::
~UndolistEntry()
{
	// Dereference audio object before eliminating pointer to it
	if (aptr != 0)
		aptr->Dereference();

	// Delete detect array, if present
	setdetect(0);
}

// Set detection point array
void Undolist::UndolistEntry::	// UndolistEntry::
setdetect(
	AudioDetectPts*	array)		// new detection array, or NULL
{
	// Delete old array, if present
	if (dptr != 0)
		delete dptr;
	dptr = array;
}

// Link in a new UndolistEntry
// Entries beyond the current one are removed
void Undolist::UndolistEntry::	// UndolistEntry::
link(
	UndolistEntry*	after)		// link after this one
{
	// First, unlink subsequent entries, if any
	after->unlink();

	// Link object as end of list
	prev = after;
	next = 0;
	after->next = this;
}

// Unlink the tail of an UndolistEntry list beyond 'this'
void Undolist::UndolistEntry::	// UndolistEntry::
unlink()
{
	UndolistEntry*	from;
	UndolistEntry*	savp;

	from = this->next;

	// unlink to end of list
	while (from != 0) {
		savp = from->next;
		from->prev->next = savp;	// maintain list consistency
		if (savp != 0)
			savp->prev = from->prev;
		delete from;
		from = savp;
	}
}


// Undolist Constructor
Undolist::
Undolist():
	current(0), head(0)
{
	dstate = new AudioDetect;
	pstate = new PlayRecord;
}

// Undloist Destructor
Undolist::
~Undolist()
{
	// Unlink any residual list
	clearlist();

	// Discard detection state and player/recorder
	delete dstate;
	delete pstate;
}

// Get the first entry in the list
UndolistEntry* Undolist::
first() const
{
	return (head.next);
}

// Clear entire undo list
void Undolist::
clearlist()
{
	head.unlink();
	current = 0;
}

// Clear all the cached detection arrays (called when detection params change)
void Undolist::
cleardetect()
{
	UndolistEntry*	ptr;

	ptr = first();
	while (ptr != 0) {
		ptr->setdetect(0);
		ptr = ptr->next;
	}
}

// Return current audio object
Audio* Undolist::
currentaudio()
{
	if (current == 0)
		return (0);
	return (current->aptr);
}

// Add an entry after the current point (delete the list beyond)
void Undolist::
addentry(
	Audio*		obj,		// audio object
	AudioDetectPts*	array)		// initial detection array
{
	UndolistEntry	*ep;

	// Create new entry, attach given detection array
	ep = new UndolistEntry(obj);
	ep->setdetect(array);

	if (current == 0) {
		// This is the first entry
		ep->link(&head);
	} else {
		ep->link(current);
	}
	// New entry is now current
	current = ep;
}

// Returns TRUE if there is something to undo; else returns FALSE.
Boolean Undolist::
canundo()
{
	if ((current == 0) || (current == first()))
		return (FALSE);
	return (TRUE);
}

// Returns TRUE if there is something to redo; else returns FALSE.
Boolean Undolist::
canredo()
{
	if ((current == 0) || (current->next == 0))
		return (FALSE);
	return (TRUE);
}

// Backup the list pointer one position
// Returns TRUE if no-op; else returns FALSE.
Boolean Undolist::
undo()
{
	if ((current == 0) || (current == first()))
		return (TRUE);
	current = current->prev;
	if (current == first())
		return (TRUE);
	return (FALSE);
}

// Advance the list pointer one position
// Returns TRUE if no-op; else returns FALSE.
Boolean Undolist::
redo()
{
	if ((current == 0) || (current->next == 0))
		return (TRUE);
	current = current->next;
	return (FALSE);
}

// Backup the list pointer to start position
// Returns TRUE if no-op; else returns FALSE.
Boolean Undolist::
undoall()
{
	if ((current == 0) || (current == first()))
		return (TRUE);
	current = first();
	return (FALSE);
}

// Advance the list pointer to last position
// Returns TRUE if no-op; else returns FALSE.
Boolean Undolist::
redoall()
{
	// If already at end of list, return TRUE
	if (redo())
		return (TRUE);

	// Loop until at end of list
	while (!redo());
	return (FALSE);
}

// Return current detect array
AudioDetectPts* Undolist::
currentdetect()
{
	if (current == 0)
		return (0);
	return (current->dptr);
}

// Set current detect array
void Undolist::
setcurrentdetect(
	AudioDetectPts*	val)		// new detect points
{
	if (current != 0)
		current->setdetect(val);
}

// Return detect state struct
AudioDetect* Undolist::
detectstate()
{
	return (dstate);
}

// Return play/record state structure
PlayRecord* Undolist::
getplayrec()
{
	return (pstate);
}
