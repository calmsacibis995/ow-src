#ifndef	_DOCFINDER_H
#define	_DOCFINDER_H

#ident "@(#)docfinder.h	1.10 93/03/09 Copyright 1990 Sun Microsystems, Inc."

#include "common.h"
#include <doc/docname.h>


// Forward references.
//
class	DOCUMENT;


class	DOCFINDER {

    private:

	// The seed document is some document whose pages lie
	// entirely within the currently displayed file.
	// Given such a document, we can traverse the local
	// document hierarchy around the seed document to find
	// the document(s) corresponding to any other page
	// in the current file.
	//
	DOCUMENT	*seed_doc;

	// Get the document that resides on the specified page.
	// 'start' is a starter document that is "close" to
	// the document we're seeking.
	//
	DOCUMENT	*DocOnPage(int page, DOCUMENT *start, ERRSTK &err);

	// Convert a logical page number within a a *book*
	// to a physical page number within the current file.
	//
	int		CvtToLogicalPage(int page);

	BOOL		IsInited()	{ return((BOOL)(seed_doc != NULL)); }

	OBJECT_STATE	objstate;	// current state of this object


    public:

	DOCFINDER();
	~DOCFINDER();

	// Seed DOCFINDER with an appropriate seed document.
	//
	void	SeedDoc(const DOCNAME &);

	// Determine which document contains 'page'.
	// Returns document name of that document, including logical page
	// within the document.
	// 'page' is a physical page number within a file,
	// rather than a logical page number within a document object.
	//
	STATUS	GetDocOnPage(int page, DOCNAME &docname, ERRSTK &err);

	// Find the first document that resides in the PostScript file
	// immediately succeeding/preceding the current file.
	//
	STATUS	GetNextFile(DOCNAME &docname, ERRSTK &err);
	STATUS	GetPrevFile(DOCNAME &docname, ERRSTK &err);
};

#endif	_DOCFINDER_H
