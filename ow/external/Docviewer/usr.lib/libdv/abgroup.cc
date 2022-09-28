#ident "@(#)abgroup.cc	1.15 94/02/14 Copyright 1990-1992 Sun Microsystems, Inc."

#include <doc/abclient.h>
#include <doc/abgroup.h>
#include <doc/abinfo.h>
#include <doc/book.h>
#include <doc/cardcats.h>
#include <doc/document.h>
#include <doc/query.h>
#include <doc/searcher.h>
#include <doc/searchdoc.h>
#include "dvlocale.h"

// Maximum number of books in the book cache at any given time.
// This is a soft limit - it can be exceeded under certain circumstances.
//
static const int	BOOK_CACHE_MAX(20);


// ABGROUP book cache.
//
class	BOOKCACHE {

    private:

	LISTX<BOOK*>	cache;		// the cache (note autodelete list)
	ABGROUP		&abgroup;	// ABGROUP served by this cache
	int		cache_hits;	// cache performance stats
	int		cache_misses;	// cache performance stats
	OBJECT_STATE	objstate;	// current state of this object

	static int	timestamp;	// timestamp for LRU caching mechanism

	// Find unused slot in cache for a new book.
	// Returns -1 if there's no more room.
	//
	void		CleanUpLRU();


    public:

	BOOKCACHE(ABGROUP &);
	~BOOKCACHE();

	// Look up book in the cache.
	//
	BOOK		*LookUp(const BOOKNAME &name, u_long flags, ERRSTK &);
};

int	BOOKCACHE::timestamp = 0;


// ABGROUP constructor.
//
ABGROUP::ABGROUP(CARDCATS &cardcats_arg) :
	cardcats	(cardcats_arg),
	pref_lang	("C")
{
	DbgFunc("ABGROUP::ABGROUP" << endl);

	bookcache = new BOOKCACHE(*this);

	// Ready to roll...
	//
	objstate.MarkReady();
}

// ABGROUP destructor.
//
ABGROUP::~ABGROUP()
{
	DbgFunc("ABGROUP::~ABGROUP" << endl);

	delete bookcache;
}

// Add an AnswerBook to this ABGROUP.
// The AnswerBook is uniquely identified by its id and version number.
// 'AddAnswerBook()' finds the specified AnswerBook via the
// card catalog mechanism.
//
STATUS
ABGROUP::AddAnswerBook(const ABNAME &name, ERRSTK &err)
{
	ABCLIENT	*answerbook;	// this new answerbook
	ABINFO		abinfo;		// configuration info for this AB


	assert(objstate.IsReady());
	DbgFunc("ABGROUP::AddAnswerBook: " << name << endl);


	// Look up AnswerBook's configuration information
	// in the card catalogs.
	//
	if (cardcats.GetMatch(name, abinfo, err)  !=  STATUS_OK) {
		err.Init(DGetText(
			"Can't find card catalog entry for this AnswerBook"));
		err.Push(DGetText("AnswerBook \"%s\" is not accessible"),
			~name.ABId());
		return(STATUS_FAILED);
	}


	// Open the AnswerBook, then add it to our list.
	//
	answerbook = ABCLIENT::Open(abinfo, PreferredLang(), err);
	if (answerbook == NULL)
		return(STATUS_FAILED);

	answerbooks.Add(answerbook);


	return(STATUS_OK);
}

// Find named AnswerBook in this ABGROUP's AnswerBook list.
//
ABCLIENT *
ABGROUP::GetAnswerBook(const ABNAME &abname)
{
	ABCLIENT	*answerbook = NULL;
	int		i;


	assert(objstate.IsReady());
	DbgFunc("ABGROUP::GetAnswerBook: " << abname << endl);


	for (i = 0; i < answerbooks.Count(); i++) {

		assert(answerbooks[i] != NULL);

		if (answerbooks[i]->Name() == abname) {
			answerbook = answerbooks[i];
			break;
		}
	}


	if (answerbook == NULL) {
		DbgFunc("ABGROUP::GetAnswerBook: "
			<< abname << " not in list" << endl);
		return(NULL);
	}

	DbgFunc("ABGROUP::GetAnswerBook: got " << abname << endl);
	return(answerbook);
}

// Set preferred language for this ABGROUP.
//
void
ABGROUP::SetPreferredLang(const STRING &lang)
{
	pref_lang = STRING::CleanUp(lang);

	if (pref_lang == NULL_STRING)
		pref_lang = "C";

	DbgFunc("ABGROUP::SetPreferredLang: " << pref_lang << endl);
}

// Retrieve document from this ABGROUP.
// Document is specified by its fully-qualified Document Object Name
// (e.g. <abid=FooAB,abvers=1.0,bkid=FOOBAR,bklang=C,docid=1001>).
//
// This method is the heart of the "multilingual AnswerBook" mechanism -
// If the "LU_PREFERRED_LANG" bit in "flags" is set, LookUpDoc()
// searches first for the "preferred language" translation of that book,
// then if not found looks for the English translation.
//
// If the "LU_RESOLVE_SYMLINK" bit in "flags" is set and the document is a
// symbolic link, resolve it to another book or document.
// Handles multiple levels of indirection (up to MAX_INDIRECT).
//
// Returns new DOCUMENT (caller is responsible for deallocation),
// or NULL if object id doesn't match.
//
DOCUMENT *
ABGROUP::LookUpDoc(const DOCNAME &docname, u_long flags, ERRSTK &err)
{
	DOCUMENT	*doc;		// document we're looking for
	DOCNAME		sym_link;	// symbolic link document's object name
	int		indirect = 0;	// levels of symlink indirection
#define	MAX_INDIRECT	10		// allow 10 levels of link indirection


	assert(objstate.IsReady());
	DbgFunc("ABGROUP::LookUpDoc: " << docname << endl);


	doc = DoLookUp(docname, flags, err);

	if (flags & LU_RESOLVE_SYMLINK) {
		while (doc != NULL  &&  doc->IsSymLink()) {

			if (++indirect > MAX_INDIRECT) {
				err.Init(DGetText("possible link loop"));
				err.Push(DGetText("too many levels of indirection in symbolic link '%s'"),
					~docname);
				return(NULL);
			}

			sym_link = doc->SymLink();
			delete(doc);
			doc = DoLookUp(sym_link, flags, err);
		}
	}

	// Set offset for the document:

	if (doc != NULL)
        {
	  doc->SetPageOffset(docname.Offset());
	}

	return(doc);
}

// Retrieve document from this ABGROUP.
// Document is specified by its fully-qualified Document Object Name
// (e.g. <abid=FooAB,abvers=1.0,bkid=FOOBAR,bklang=C,docid=1001>).
//
// This method is the heart of the "multilingual AnswerBook" mechanism -
// if a document's book language is "preferred language", LookUpDoc()
// searches first for the "preferred language" translation of that book,
// then if not found looks for the English translation.
//
// Returns new DOCUMENT (caller is responsible for deallocation),
// or NULL if object id doesn't match.
//
DOCUMENT *
ABGROUP::DoLookUp(const DOCNAME &docname, u_long flags, ERRSTK &err)
{
	DOCNAME		mlname;	// temp doc name used for multilingual stuff
	DOCUMENT	*mldoc;	// temp doc used for multilingual stuff
	BOOK		*book;	// book containing document we're looking for


	assert(objstate.IsReady());
	DbgFunc("ABGROUP::LookUpDoc: " << docname << endl);


	// If the "LU_PREFERRED_LANG" bit in "flags" is set,
	// search first for the "preferred language" translation of the book,
	// then if not found look for the English translation.
	//
	if ((flags & LU_PREFERRED_LANG)  &&  PreferredLang() != "C") {

		mlname = docname;
		mlname.SetBookLang(PreferredLang());
		flags &= ~LU_PREFERRED_LANG;

		if ((mldoc = DoLookUp(mlname, flags, err))  ==  NULL) {
			mlname.SetBookLang("C");	// English
			mldoc = DoLookUp(mlname, flags, err);
		}

		return(mldoc);
	}


	// Make sure document name is valid and fully-qualified.
	//
	if ( ! docname.IsValid()) {
		err.Init(DGetText("Document name is invalid: '%s'"), ~docname);
		return(NULL);
	}


	// Get document's book from the book cache.
	//
	if ((book = bookcache->LookUp(docname.BookName(), flags, err)) == NULL)
		return(NULL);

	return(book->GetDocById(docname.DocId(), err));
}

// BOOKCACHE constructor.
//
BOOKCACHE::BOOKCACHE(ABGROUP &abgroup_arg) :
	abgroup		(abgroup_arg),
	cache_hits	(0),
	cache_misses	(0)
{
	DbgFunc("BOOKCACHE::BOOKCACHE" << endl);

	objstate.MarkReady();
}

// BOOKCACHE destructor.
//
BOOKCACHE::~BOOKCACHE()
{
	DbgFunc("BOOKCACHE::~BOOKCACHE" << endl);
}

// Find book in book cache.
//
BOOK *
BOOKCACHE::LookUp(const BOOKNAME &book_name, u_long flags, ERRSTK &err)
{
	BOOK		*book;		// book for which we're looking
	ABCLIENT	*answerbook;	// AnswerBook containing this book
	ABNAME		ab_name;	// answerbook name portion of book_name
	STRING		book_path;	// path for book database
	int		i;


	assert(objstate.IsReady());
	assert(book_name.IsValid());
	DbgFunc("BOOKCACHE::LookUp: " << book_name << endl);


	// See if book is already in cache.
	//
	for (i = 0; i < cache.Count(); i++) {

		if (cache[i]->Name() == book_name) {

			// We scored a hit.
			// Update the book's timestamp for the benefit
			// of our LRU caching mechanism.
			//
			book = cache[i];
			book->timestamp = BOOKCACHE::timestamp++;
			++cache_hits;
			DbgMed("BOOKCACHE::LookUp: found " << book << endl);
			return(book);
		}
	}


	DbgMed("BOOKCACHE::LookUp: miss" << endl);
	++cache_misses;


	// Check the ABGROUP for this book's AnswerBook.
	// If we don't find it, and the "Auto Add" flag is set,
	// attempt to add the AnswerBook to this ABGROUP.
	//
	ab_name = book_name.ABName();

	if ((answerbook = abgroup.GetAnswerBook(ab_name))  ==  NULL) {

		if ((flags & LU_AUTO_ADD) == 0) {

			err.Init(DGetText(
			   "AnswerBook '%s' is not in the current Library"),
			   ~ab_name);
			return(NULL);
		}

		if (abgroup.AddAnswerBook(ab_name, err)  !=  STATUS_OK)
			return(NULL);
		answerbook = abgroup.GetAnswerBook(ab_name);
		assert(answerbook != NULL);
		DbgMed("BOOKCACHE::LookUp: auto_added " << ab_name << endl);
	}


	// Locate the book database that defines this book.
	//
	book_path = answerbook->TOCPath(book_name);
	if ( ! BOOK::Exists(book_path)) {
		err.Init(DGetText("can't find book '%s': not part of '%s'"),
			~book_name.BookId(), ~answerbook->Title());
		return(NULL);
	}


	// Cleanup LRU books in cache.
	//
	CleanUpLRU();


	// Open the book.
	// Set its time stamp for the benefit of our LRU caching mechanism.
	//
	if ((book = BOOK::Open(book_name, book_path, err))  ==  NULL)
		return(NULL);
	book->timestamp = BOOKCACHE::timestamp++;


	// Add book to cache.
	//
	cache.Add(book);

	return(book);
}

void
BOOKCACHE::CleanUpLRU()
{
	int	lru_book;	// index of least recently used book in cache
	int	i;


	assert(objstate.IsReady());
	DbgFunc("BOOKCACHE::CleanUpLRU" << endl);


	// If cache is getting too big, try to pare it down
	// by tossing out currently unreferenced books on an LRU basis.
	//
	while (cache.Count() > BOOK_CACHE_MAX) {

		lru_book = -1;

		for (i = 0; i < cache.Count(); i++) {

			if (cache[i]->RefCount() > 0)
				continue;

			if (lru_book < 0  ||
			    cache[i]->timestamp < cache[lru_book]->timestamp)
				lru_book = i;
		}

		// If there are no more unreferenced books in the cache,
		// break out of the loop even if the cache still has
		// too many books in it.
		//
		if (lru_book < 0) {
			DbgMed("BOOKCACHE::CleanUpLRU: exceeding soft limit: "
				<< cache.Count()
				<< " (" << BOOK_CACHE_MAX << ")"
				<< endl);
			break;
		}

		// Otherwise, delete the LRU unreferenced book.
		//
		DbgMed("BOOKCACHE::CleanUpLRU: deleting LRU book: "
			<< cache[lru_book] << endl);
		cache.Delete(lru_book);
	}
}

// Perform full-text search across all AnswerBooks in
// this ABGROUP.
//
STATUS
ABGROUP::Search(const QUERY &query, LISTX<SEARCHDOC*> &hitlist, ERRSTK &err)
{
	int	i;
	int	j;
	BOOL	delete_searcher = BOOL_FALSE;


	assert(objstate.IsReady());
	DbgFunc("ABGROUP::Search: " << query << endl);
	

	// Clear residual hits from hit list.
	//
	hitlist.Clear();

	// Figure out if we can leave collections open or if we
	// need to open/search/close.

	// Attempt to open all the answerbook collections
	// If they are already open, no big deal.
        for (i = 0; i < answerbooks.Count() && delete_searcher == BOOL_FALSE;
			      i++) {
	    if (answerbooks[i]->InitSearcher(err) != STATUS_OK) {
	       delete_searcher = BOOL_TRUE;
	       // Delete any searchers we've already created
	       for (j = 0; j < i; j++)
	           answerbooks[j]->DeleteSearcher();
	       }
	    }

	// Search each AnswerBook in turn.
	//
	for (i = 0; i < answerbooks.Count(); i++) {
		answerbooks[i]->Search(query, hitlist, delete_searcher, err);
	}


	// XXX need to determine how to handle search error
	// XXX occurring in individual AnswerBooks.
	//
	return(STATUS_OK);
}
