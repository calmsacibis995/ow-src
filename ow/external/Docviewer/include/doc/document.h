#ifndef	_DOCUMENT_H
#define	_DOCUMENT_H

#ident "@(#)document.h	1.47 06/11/93 Copyright 1989-1992 Sun Microsystems, Inc."

#include <doc/common.h>
#include <doc/docname.h>
#include <doc/pgrange.h>


// Define forward references
//
class	BOOK;


class	DOCUMENT {

    private:

	OBJECT_STATE	objstate;


    protected:

	DOCNAME		name;		// document's object name
	STRING		title;		// title
	STRING		view_method;	//
	STRING		print_method;	//
	STRING		label;		//
	DOCNAME		sym_link;	//
	PGRANGE		range;		// document's page range
	u_long		flags;		//
#define			DF_FILE		0x0001
#define			DF_NOSHOW	0x0002
#define			DF_NOSHOWKIDS	0x0004
	BOOL		IsSet(u_long f) const	{ return((BOOL)(flags & f)); }


	// Is this document valid?
	// This method is private because it's only used when
	// creating new DOCUMENT objects.
	BOOL		IsValid();

	// Document constructors are private.
	// Only derived classes can actually create DOCUMENT objects.
	//
	DOCUMENT();
	DOCUMENT(const DOCUMENT &);


    public:

	// Document destructor.
	// Note that this destructor must be virtual in order for objects
	// derived from DOCUMENT (e.g., ISAMDOC, DBMDOC) to be
	// properly cleaned up.
	// Note that this implies that DOCUMENT objects mustn't require
	// any cleanup of its own, since there is no real DOCUMENT destructor.
	//
	virtual		~DOCUMENT();

	// Access document attributes.
	//
	const DOCNAME	&Name() const		{ return(name); }
	const STRING	&Id() const		{ return(name.DocId()); }
	const STRING	&Title() const		{ return(title); }
	const STRING	&ViewMethod() const	{ return(view_method); }
	const STRING	&PrintMethod() const	{ return(print_method); }
	const STRING	&Label() const		{ return(label); }
	const DOCNAME	&SymLink() const	{ return(sym_link); }
	const PGRANGE	&Range() const		{ return(range); }
	BOOL		IsDocFile() const	{ return(IsSet(DF_FILE)); }
	BOOL		IsNoShow() const	{ return(IsSet(DF_NOSHOW));}
	BOOL		IsNoShowKids() const	{ return(IsSet(DF_NOSHOWKIDS));}
	BOOL		IsSymLink() const
				{ return(sym_link.IsValid()); }
	BOOL		IsRoot() const		{ return(name.IsRoot()); }
	virtual BOOL	IsLeaf() const		{ return(BOOL_FALSE); }

	// Set offset in the docname attribute:

	void            SetPageOffset(int offsetvalue);

	// Get related documents.
	//
	virtual DOCUMENT	*GetFirstChild(ERRSTK &) const	{ return NULL;}
	virtual DOCUMENT	*GetLastChild(ERRSTK &) const	{ return NULL;}
	virtual DOCUMENT	*GetNextSibling(ERRSTK &) const	{ return NULL;}
	virtual DOCUMENT	*GetPrevSibling(ERRSTK &) const	{ return NULL;}
	virtual DOCUMENT	*GetParent(ERRSTK &) const	{ return NULL;}

	// Are two document equivalent?
	// Returns 'BOOL_TRUE' if they have the same attributes and values,
	// otherwise returns 'BOOL_FALSE'.
	//
	static BOOL		Equal(const DOCUMENT *d1, const DOCUMENT *d2);

	// Book to which this document belongs.
	//
	virtual const BOOK	*GetBook() const	{ return NULL; }

	// Print document title.
	//
	friend ostream	&operator << (ostream &out, const DOCUMENT *doc);
};

#endif	_DOCUMENT_H
