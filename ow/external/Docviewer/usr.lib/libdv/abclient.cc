#ident "@(#)abclient.cc	1.27 94/02/14 Copyright 1990 Sun Microsystems, Inc."

#include <doc/abclient.h>
#include <doc/book.h>
#include <doc/abinfo.h>
#include <doc/searcher.h>
#include <doc/searchdoc.h>
#include <doc/query.h>
#include "dvlocale.h"
#include <dirent.h>
#include <sys/stat.h>


ABCLIENT::ABCLIENT(const ABINFO &info, const STRING &lang) :
	abinfo		(info),
	preferred_lang	(lang),
	searcher	(NULL),
	searcher_ml	(NULL)
{
	assert(abinfo.IsValid());
	assert(preferred_lang != ""  &&  preferred_lang != NULL_STRING);
	DbgFunc("ABCLIENT::ABCLIENT" << endl);
}

ABCLIENT::~ABCLIENT(void)
{
	DbgFunc("ABCLIENT::~ABCLIENT" << endl);

	if (searcher)
	   delete searcher;
	if (searcher_ml)
	   delete searcher_ml;
}

// Open AnswerBook specified by its ABINFO database entry.
// 'lang' specifies the primary language for this AnswerBook
// (see below).  If 'lang' is "C" or NULL_STRING,
// we'll use the default language of the AnswerBook
// Returns pointer to new ABCLIENT - caller is responsible for
// deallocation.
//
ABCLIENT *
ABCLIENT::Open(const ABINFO &abinfo, const STRING &lang, ERRSTK &err)
{
	ABCLIENT	*answerbook;	// this new answerbook
	DOCNAME		top_level;
	STRING		toc_path;


	assert(abinfo.IsValid());
	DbgFunc("ABCLIENT::Open:" << abinfo.Name() << endl);


	// Create new AnswerBook object.
	//
	if ((answerbook = new ABCLIENT(abinfo, lang))  ==  NULL) {
		err.Init(DGetText("out of memory"));
		return(NULL);
	}


	answerbook->objstate.MarkGettingReady();


	// Validate AnswerBook by looking for its top-level TOC.
	//
	MakeAnswerBookRootDocName(answerbook->Name(), top_level);
	toc_path = answerbook->TOCPath(top_level);
	if ( ! BOOK::Exists(toc_path)) {
		err.Init(DGetText(
	"Verify that the card catalog entry for this AnswerBook is correct"));
		err.Push(DGetText("AnswerBook \"%s\" is not accessible"),
			~answerbook->Title());
		delete answerbook;
		return(NULL);
	}


	// Ready to roll...
	//
	answerbook->objstate.MarkReady();
	return(answerbook);
}

// Get list of languages available on this AnswerBook.
// Each language is represented by its standard ISO 639
// language/territory abbreviation.  "C" means plain ol' English.
//
STATUS
ABCLIENT::GetListOfLangs(LIST<STRING> &langs, ERRSTK &err) const
{
	assert(objstate.IsReady());
	assert(abinfo.IsValid());
	DbgFunc("ABCLIENT::GetListOfLangs" << endl);

	langs.Clear();
	return(abinfo.GetListOfLangs(langs, err));
}

// Get ids of all books in this AnswerBook that are
// available in the specified language ("C"=default=English).
//
STATUS
ABCLIENT::GetListOfBooks(	LIST<BOOKNAME>	&books,
				const STRING	&lang,
				ERRSTK		&err)
{
	assert(objstate.IsReady());
	DbgFunc("ABCLIENT::GetListOfBooks" << endl);

	books.Clear();
	return(abinfo.GetListOfBooks(books, lang, err));
}

// Initialize the searcher just to see if we can do it.
STATUS
ABCLIENT::InitSearcher(ERRSTK &err)
{
	if ((searcher = GetSearcher(err)) == NULL) {
	   delete(searcher);
	   searcher = NULL;
	   return (STATUS_FAILED);
	   }
	return (STATUS_OK);
}

void
ABCLIENT::DeleteSearcher ()
{
	if (searcher)
	   delete searcher;

	searcher = NULL;
}

// Perform full-text search across all books in this AnswerBook.
//
STATUS
ABCLIENT::Search(	const QUERY		&query,
			LISTX<SEARCHDOC*>	&main_hitlist,
			BOOL			delete_searcher,
			ERRSTK			&err)
{
	DOCNAME			tmpname;
	LISTX<SEARCHDOC*>	hitlist;
	LISTX<SEARCHDOC*>	hitlist_ml;
	LIST<BOOKNAME>		books_ml;
	int			maxdocs;
	int			i, j;


	assert(objstate.IsReady());
	DbgFunc("ABCLIENT::Search: " << query << endl);


	// Initialize search engine the first time around.
	// Note that it's ok to not find the searcher for a
	// particular language (other than English).
	//
	if ((searcher = GetSearcher(err))  ==  NULL)
		return(STATUS_FAILED);
	searcher_ml = GetSearcherML(err);


	// Perform the search.
	//
	maxdocs = query.MaxDocuments();
	if (searcher->Search(query, maxdocs, hitlist, err)  !=  STATUS_OK) {
		if (delete_searcher == BOOL_TRUE) {
		   delete searcher;
		   searcher = NULL;
		   if (searcher_ml != NULL) {
		      delete searcher_ml;
		      searcher_ml = NULL;
		      }
		   }
		return(STATUS_FAILED);
		}
	if (searcher_ml  &&
	    searcher_ml->Search(query, maxdocs, hitlist_ml, err)  != STATUS_OK) {
		if (delete_searcher == BOOL_TRUE) {
		   delete searcher;
		   searcher = NULL;
		   if (searcher_ml != NULL) {
		      delete searcher_ml;
		      searcher_ml = NULL;
		      }
		   }
		return(STATUS_FAILED);
		}


	// The document names in the hit list are not fully resolved
	// (they're missing the "AnswerBook id" and "AnswerBook version" parts,
	// as well as the book language.
	// So we'll resolve them here.
	//
	MakeAnswerBookRootDocName(Name(), tmpname);
	tmpname.SetBookLang("C");
	for (i = 0; i < hitlist.Count(); i++)
		hitlist[i]->docname.Resolve(tmpname);
	tmpname.SetBookLang(preferred_lang);
	for (i = 0; i < hitlist_ml.Count(); i++)
		hitlist_ml[i]->docname.Resolve(tmpname);


	// If we're searching a multilingual AnswerBook,
	// we may have gotten hits in an English book that is also
	// available in the "preferred language" translation.
	// It doesn't make sense for these hits to show up in the hit list
	// since the English book is not visible in the Navigator TOC.
	// So we'll cull them out here.
	//
	if (searcher_ml          &&
	    hitlist.Count() > 0  &&
	    GetListOfBooks(books_ml, preferred_lang, err)  ==  STATUS_OK) {

		for (i = 0; i < hitlist.Count(); i++) {

		    const STRING &bookid = hitlist[i]->DocName().BookId();

		    for (j = 0; j < books_ml.Count(); j++) {

			if (bookid == books_ml[j].BookId()) {

			    DbgMed("ABCLIENT::Search: culling "
						<< hitlist[i]->DocName()
						<< endl);
			    hitlist.Delete(i);
			    break;
			}
		    }
		}
	}


	// Move hits into main hitlist.
	// Note that we have to create new SEARCHDOC objects to add to
	// the main hitlist since the objects in "hitlist" and "hitlist_ml"
	// will automatically get deallocated once we return().
	// Note also that whoever owns the main hitlist is responsible for
	// deleting these objects.
	//
	for (i = 0; i < hitlist.Count(); i++)
		main_hitlist.Add(new SEARCHDOC(*hitlist[i]));
	for (i = 0; i < hitlist_ml.Count(); i++)
		main_hitlist.Add(new SEARCHDOC(*hitlist_ml[i]));

	if (delete_searcher == BOOL_TRUE) {
	   delete searcher;
	   searcher = NULL;
	   if (searcher_ml != NULL) {
	      delete searcher_ml;
	      searcher_ml = NULL;
	      }
	   }

	return(STATUS_OK);
}

// Get searcher for this AnswerBook.
// Returns pointer to SEARCHER if it could be initialized,
// otherwise returns NULL.
//
SEARCHER *
ABCLIENT::GetSearcher(ERRSTK &err)
{
	STRING	index_path;	// where this search index resides

	assert(objstate.IsReady());
	DbgFunc("ABCLIENT::GetSearcher" << endl);


	// Initialize search engine the first time around.
	//
	if (searcher == NULL) {

		index_path = IndexPath("C");

		searcher = new SEARCHER(index_path);
		if (searcher->Init(err) != STATUS_OK) {
			delete(searcher);
			searcher = NULL;
		}
	}

	return(searcher);
}

// Get searcher for this AnswerBook.
// Returns pointer to SEARCHER if it could be initialized,
// otherwise returns NULL.
//
SEARCHER *
ABCLIENT::GetSearcherML(ERRSTK &err)
{
	STRING	index_path;	// where this search index resides

	assert(objstate.IsReady());
	DbgFunc("ABCLIENT::GetSearcherML: " << preferred_lang << endl);


	// Initialize search engine the first time around,
	// but don't bother if preferred language is English.
	//
	if (searcher_ml == NULL  &&  preferred_lang != "C") {

		index_path = IndexPath(preferred_lang);

		searcher_ml = new SEARCHER(index_path);
		if (searcher_ml->Init(err) != STATUS_OK) {
			delete(searcher_ml);
			searcher_ml = NULL;
		}
	}

	return(searcher_ml);
}
