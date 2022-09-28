#ifndef	_BOOKSHELF_H
#define	_BOOKSHELF_H

#ident "@(#)bookshelf.h	1.14 11/15/96 Copyright 1992 Sun Microsystems, Inc."


#include <doc/common.h>
#include <doc/list.h>
#include <doc/listx.h>
#include <doc/abname.h>
#include <doc/notify.h>


// Forward references.
//
class	BOOKMARK;

// BOOKSHELF::Open() flags (analogous to open(2))
//
enum	BS_OPEN_FLAGS {
	BS_RDONLY	= 0x01,
	BS_RDWR		= 0x02,
	BS_CREAT	= 0x04,
	BS_SET_LOCK	= 0x08,	// set advisory lock on this bookshelf file
	BS_RESET_LOCK	= 0x10	// reset lock, e.g., reset a stale lock
};


//
class	BOOKSHELF {

    private:

	// This bookshelf's path.
	//
	STRING		path;

	// File handle for this bookshelf.
	//
	FILE		*bsfp;

	// Is bookshelf read-only?
	//
	BOOL		read_only;

	// Is bookshelf locked?
	//
	BOOL		locked;

	// Is bookshelf in need of saving?
	//
	BOOL		dirty;

	// Current state of this object.
	//
	OBJECT_STATE	objstate;

	// Utility functions for dealing with lock files.
	//
	static void	MakeLockPath(const STRING &path, STRING &lock_path);
	static STATUS	CreateLockFile(const STRING &path, BOOL &flag_change, NOTIFY &notify);
	static STATUS	ClearLockFile(const STRING &path, ERRSTK &err);

	// Methods for processing the various bookmark file attributes.
	//
	void		AnswerBookAttr(	const STRING &name, const STRING &val);
	void		BookmarkAttr(	const STRING &name, const STRING &val);
	void		SearchAttr(	const STRING &name, const STRING &val);
	void		NavigatorAttr(	const STRING &name, const STRING &val);

	// Initialize bookshelf file.
	//
	STATUS		WriteHeader(ERRSTK &err);

	// BOOKSHELF constructor.
	//
	BOOKSHELF(const STRING &path, FILE *fp, BOOL rdonly, BOOL locked);


    public:


	// Determine if specified file is a bookshelf file.
	//
	static BOOL	IsBookshelfFile(const STRING &path);

	// Get path name of default bookshelf file for the current user.
	//
	static void	GetDefaultPath(STRING &path);

	// See if the specified bookshelf file exists.
	//
	static BOOL	FileExists(const STRING &path);

	// Is the specified bookshelf currently locked by
	// this or another process?
	//
	static BOOL	FileIsLocked(const STRING &path);

	// Open bookshelf file.
	// Returns pointer to new BOOKSHELF, or NULL on error.
	// Caller is responsible for deallocation.
	//
	// "flags" semantics are as follows:
	//
	//	BS_RDONLY	Open file read-only.
	//			Ignore locking-related flags (BS_*_LOCK)
	//	BS_RDWR		Open file read/write.  Assumes one of
	//			BS_*_LOCK is also specified
	//	BS_CREAT	Create file if it doesn't already exist.
	//			Assumes BS_RDWR is also specified.
	//	BS_SET_LOCK	Set advisory lock for this file.
	//			Operation will fail if lock is already set
	//			by this or another process.
	//	BS_RESET_LOCK	Delete existing lock, if any, then set lock
	//			as with BS_SET_LOCK
	//
	static BOOKSHELF *OpenFile(const STRING &path, int &flags, ERRSTK &err, NOTIFY &notify);

	// BOOKSHELF destructor.
	//
	~BOOKSHELF();

	// Read current bookshelf file.
	//
	STATUS		ReadFile(ERRSTK &err);

	// Save current bookshelf contents to current file.
	// Assumes bookshelf is writable.
	//
	STATUS		SaveToFile(ERRSTK &err);

	// Is this bookshelf read-only?
	//
	BOOL		IsReadOnly() const		{ return(read_only); }

	// Has this bookshelf been modified but not saved?
	//
	BOOL		IsDirty() const			{ return(dirty); }

	// Mark this bookshelf as having been modified.
	//
//XXX	void		MarkDirty()			{ dirty = BOOL_TRUE; }
	void		MarkDirty();

	// Get Bookshelf full path name.
	//
	const STRING	&GetPath() const		{ return(path); }

	// List of AnswerBooks contained in this bookshelf.
	//
	LIST<ABNAME>	answerbooks;

	// List of Bookmarks contained in this bookshelf.
	// Note that this is an "autodelete" list (LISTX).
	//
	LISTX<BOOKMARK*> bookmarks;
};

#endif	_BOOKSHELF_H
