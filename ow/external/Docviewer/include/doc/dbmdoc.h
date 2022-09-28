#ifndef	_DBMDOC_H
#define	_DBMDOC_H

#ident "@(#)dbmdoc.h	1.8 06/11/93 Copyright 1989 Sun Microsystems, Inc."


#include <doc/document.h>
#include <doc/dbmbook.h>


// Forward references.
//
class	DBMBOOK;
class	ATTRLIST;
class	XXDOC;


class	DBMDOC : public DOCUMENT {

    private:

	// ndbm-specific fields.
	//
	STRING		parent_id;
	STRING		first_child_id;
	STRING		next_sibling_id;

	// The book from whence we came.
	//
	DBMBOOK		*book;

	// The document constructor is private.
	// The only way to get a new document is via "MakeDoc()".
	//
	DBMDOC();

	OBJECT_STATE	objstate;


    public:

	// Construct DBM document from DBM database record.
	// Must check object's validity (see "IsValid()") before use.
	//
	DBMDOC(ATTRLIST &attrs, DBMBOOK *book);

	// Construct DBM document from XXDOC.
	// Must check object's validity (see "IsValid()") before use.
	//
	DBMDOC(const XXDOC &doc);

	~DBMDOC();

	// See if this document object is valid.
	//
	BOOL		IsValid();

	BOOL		IsLeaf() const;

	DOCUMENT	*GetFirstChild(ERRSTK &) const;
	DOCUMENT	*GetLastChild(ERRSTK &) const;
	DOCUMENT	*GetNextSibling(ERRSTK &) const;
	DOCUMENT	*GetPrevSibling(ERRSTK &) const;
	DOCUMENT	*GetParent(ERRSTK &) const;

	// Book to which this document belongs.
	//
	const BOOK	*GetBook() const	{ return(book); }
};

// Convert generic XXDOC into attribute list.
//
STATUS	CvtDocToAttrList(const XXDOC *, ATTRLIST &, ERRSTK &);


#endif	_DBMDOC_H
