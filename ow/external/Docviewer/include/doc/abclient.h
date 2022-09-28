#ifndef	_ABCLIENT_H
#define	_ABCLIENT_H

#ident "@(#)abclient.h	1.21 02/14/94 Copyright 1990-1992 Sun Microsystems, Inc."

#include <doc/common.h>
#include <doc/abname.h>
#include <doc/abinfo.h>
#include <doc/bookname.h>
#include <doc/listx.h>


// Forward references.
//
class	QUERY;
class	SEARCHDOC;
class	SEARCHER;


// In theory, all access to AnswerBook data goes through this interface,
// including document database lookups, PostScript file access,
// search & retrieval operations, etc.
// Currently only a subset of the above go through this interface.
//
class	ABCLIENT {

    private:

	// AnswerBook configuration info.
	//
	ABINFO		abinfo;

	// Search engines for doing full-text searches.
	// "searcher_ml" is used if we're searching a multilingual AnswerBook.
	//
	SEARCHER	*searcher;
	SEARCHER	*searcher_ml;

	// Preferred language.
	//
	STRING		preferred_lang;

	// ABCLIENT constructor is private - "ABCLIENT::Open()" is the
	// only way to get a new ABCLIENT object.
	//
	ABCLIENT(const ABINFO &info, const STRING &lang);

	OBJECT_STATE	objstate;


    public:

	~ABCLIENT(void);

	// Open AnswerBook specified by its ABINFO database entry.
	// Returns pointer to new ABCLIENT - caller is responsible for
	// deallocation.
	//
	static ABCLIENT	*Open(	const ABINFO	&info,
				const STRING	&preferred_lang,
				ERRSTK		&err);

	// Get AnswerBook title, id, other general info.
	//
	const STRING	&Title() const	{ return(abinfo.Title()); }
	const ABNAME	&Name() const	{ return(abinfo.Name()); }

	// Get location information for various AnswerBook compoents:
	// TOC databases, PostScript directories, search index.
	// Each of these components can be available in different languages;
	// 'lang' or 'name.BookLang()' specifies which language to look for.
	//
	const STRING	TOCPath(const BOOKNAME &name) const
				{ return(abinfo.TOCPath(name)); }
	const STRING	PSPath(const BOOKNAME &name) const
				{ return(abinfo.PSPath(name)); }
	const STRING	IndexPath(const STRING &lang) const
				{ return(abinfo.IndexPath(lang)); }

	// Get list of languages available on this AnswerBook.
	// Each language is represented by its standard ISO 639
	// language/territory abbreviation.  "C" means plain ol' English.
	//
	STATUS		GetListOfLangs(LIST<STRING> &langs, ERRSTK &err) const;

	// Get names of all books in this AnswerBook that are
	// available in the specified language ("C"=default=English).
	//
	STATUS		GetListOfBooks(	LIST<BOOKNAME>	&books,
					const STRING	&lang,
					ERRSTK		&err);

	// Test to see if we can open the collections for this answerbook
	STATUS		InitSearcher(ERRSTK &err);

	// Delete searcher if we created it but then find that we
	// can't open all of the collections
	void		DeleteSearcher();

	// Get Searchers for this AnswerBook.
	//
	SEARCHER	*GetSearcher(ERRSTK &err);
	SEARCHER	*GetSearcherML(ERRSTK &err);

	// Perform full-text search across all books in this AnswerBook.
	//
	STATUS		Search(	const QUERY		&query,
				LISTX<SEARCHDOC*>	&hitlist,
				BOOL			delete_searcher,
				ERRSTK			&err);
};

#endif	_ABCLIENT_H
