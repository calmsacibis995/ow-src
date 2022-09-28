#ifndef	_ABGROUP_H
#define	_ABGROUP_H

#ident "@(#)abgroup.h	1.12 11 Jun 1993 Copyright 1990 Sun Microsystems, Inc."


#include <doc/common.h>
#include <doc/cardcats.h>
#include <doc/docname.h>
#include <doc/list.h>
#include <doc/listx.h>

// Forward references.
//
class	ABCLIENT;
class	ABINFO;
class	BOOK;
class	BOOKCACHE;
class	DOCUMENT;
class	QUERY;
class	SEARCHDOC;


// An ABGROUP is a collection of AnswerBooks that can be
// browsed, searched, and viewed as a single entity.
//
class	ABGROUP {

    private:

	// Cache for accessing AnswerBook book databases.
	//
	BOOKCACHE	*bookcache;

	// Current "preferred language" for multilingual AnswerBooks.
	// If a given AnswerBook document is available in the preferred
	// language, the system will always use that document.
	// Otherwise, the system will use the English version of the document.
	//
	STRING		pref_lang;

	// Reference to the list of card catalogs we should use to
	// locate AnswerBooks.  Note that this is passed in to us -
	// we don't manage it, we just use it.
	//
	CARDCATS	&cardcats;

	// Current state of this object.
	//
	OBJECT_STATE	objstate;

	// Workhorse method used by "LookUpDoc()" below.
	//
	DOCUMENT	*DoLookUp(	const DOCNAME	&docname,
					u_long		flags,
					ERRSTK		&err);


    public:

	// List of AnswerBooks contained in this ABGROUP.
	// Note that this is an "auto delete" list (see <doc/listx.h>).
	//
	LISTX<ABCLIENT*>	answerbooks;

	// ABGROUP constructor, destructor.
	//
	ABGROUP(CARDCATS &cardcats);
	~ABGROUP();

	// Add an AnswerBook to this ABGROUP (method #1).
	// The AnswerBook is uniquely identified by its ABNAME.
	// 'AddAnswerBook()' finds the specified AnswerBook via the
	// ABINFO mechanism.
	// Use this method if you *don't* already have an ABINFO entry
	// for the AnswerBook.
	//
	STATUS		AddAnswerBook(const ABNAME &name, ERRSTK &);

	// Add an AnswerBook to this ABGROUP (method #2).
	// The AnswerBook is specified by 'abinfo'.
	// Use this method if you *do* already have an ABINFO entry
	// for the AnswerBook.
	//
	STATUS		AddAnswerBook(ABINFO &abinfo, ERRSTK &);

	// Find named AnswerBook in this ABGROUP's AnswerBook list.
	//
	ABCLIENT	*GetAnswerBook(const ABNAME &abname);

	// Destroy the card catalogs...  docviewer needs this, 
	// but navigator doesn't.
	void		DestroyCardCatalogs ()
	{
			delete &cardcats;
	}

	// Get list of card catalogs associated with this ABGROUP.
	//
	CARDCATS	&GetCardCatalogs()	{ return(cardcats); }

	// Get preferred language used by this ABGROUP to locate
	// foreign language books.  'lang' should be an ISO 639
	// language/territory abbreviation.
	//
	const STRING	&PreferredLang() const	{ return(pref_lang); }

	// Set preferred language for this ABGROUP.
	//
	void		SetPreferredLang(const STRING &lang);

	// Retrieve document from this ABGROUP.
	// Document is specified by its fully-qualified object name
	// (e.g. <abid=FooAB,abvers=1.0,bkid=FOOBAR,bklang=C,docid=1001>).
	//
	// This method is the heart of the "multilingual AnswerBook"
	// mechanism - if the "LU_PREFERRED_LANG" bit in "flags" is set,
	// LookUpDoc() searches first for the "preferred language"
	// translation of the specified document, then if not found looks
	// for the English translation.
	//
	// If the "LU_RESOLVE_SYMLINK" bit in "flags" is set and the document
	// is a symbolic link, resolve the link to another book or document.
	// Document can reside either in the same book, in a different book
	// in the same AnswerBook, or in a different AnswerBook in the current
	// ABGROUP.  References to documents outside of the current ABGROUP
	// are not resolvable.  Handles up to 10 levels of indirection.
	//
	// If the "LU_AUTO_ADD" bit in "flags" is set, we'll automatically
	// add AnswerBooks to this ABGROUP as they are accessed.
	// Otherwise, lookups to AnswerBooks not in the ABGROUP will fail.
	// This is useful, e.g., for the Viewer which frequently must access
	// AnswerBooks it can't, a priori, know to load.
	//
	// Returns new DOCUMENT (caller is responsible for deallocation),
	// or NULL if document is not found.
	//
#define	LU_PREFERRED_LANG	0x01
#define	LU_RESOLVE_SYMLINK	0x02
#define	LU_AUTO_ADD		0x04
	DOCUMENT	*LookUpDoc(	const DOCNAME	&docname,
					u_long		flags,
					ERRSTK		&err);

	// Perform full-text search across all AnswerBooks in
	// this ABGROUP.
	//
	STATUS		Search(	const QUERY		&query,
				LISTX<SEARCHDOC*>	&hitlist,
				ERRSTK			&err);
};

#endif	_ABGROUP_H
