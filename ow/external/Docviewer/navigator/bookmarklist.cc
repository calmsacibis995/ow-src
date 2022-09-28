#ident "@(#)bookmarklist.cc	1.49 93/06/22 Copyright 1990-1992 Sun Microsystems, Inc."


#include "bookmarklist.h"
#include "navigator.h"
#include <doc/bookmark.h>
#include <doc/bookshelf.h>
#include <locale.h>
#include <doc/notify.h>


// Comparison routine used in sorting bookmark lists.
//
int	CompareBookmarks(const void *arg1, const void *arg2);


BOOKMARKLIST::BOOKMARKLIST(Xv_opaque panel, int x, int y) :
	WINLIST(panel, x, y)
{ 
	DbgFunc("BOOKMARKLIST::BOOKMARKLIST" << endl);

	objstate.MarkReady();
}

BOOKMARKLIST::~BOOKMARKLIST()
{
	DbgFunc("BOOKMARKLIST::~BOOKMARKLIST" << endl);
}

STATUS
BOOKMARKLIST::Load(ERRSTK &err)
{
	LIST<BOOKMARK*>	&bookmarks = bookshelf->bookmarks;
	STRING		title;
	STRING		indent("      ");
	int		i, n;


	assert(objstate.IsReady());
	assert(bookshelf != NULL);
	DbgFunc("BOOKMARKLIST::Load" << endl);


	// Before we display the list, sort it by AnswerBook.
	//
	SortList(bookmarks, CompareBookmarks);


	// If a bookmark list is already loaded,
	// get rid of it.
	//
	WINLIST::Clear();


	// Add each bookmark to the display list.
	//
	WINLIST::BeginBatch();
	for (i = n = 0; i < bookmarks.Count(); i++) {

		if (i == 0  ||  bookmarks[i]->AnswerBookTitle() !=
				bookmarks[i-1]->AnswerBookTitle()) {

			title = bookmarks[i]->AnswerBookTitle();
			WINLIST::AppendHeaderEntry(title);
			WINLIST::SetClientData(n++, NULL);
		}

		title = indent + bookmarks[i]->Title();
		WINLIST::AppendEntry(title);
		WINLIST::SetClientData(n++, (caddr_t)bookmarks[i]);
	}
	WINLIST::EndBatch();


	return(STATUS_OK);
}

// Add bookmark to the list.
//
void
BOOKMARKLIST::Add(BOOKMARK *bookmark)
{
	LIST<BOOKMARK*>	&bookmarks = bookshelf->bookmarks;
	ERRSTK		err;


	assert(objstate.IsReady());
	assert(bookshelf != NULL);
	assert(bookmark != NULL);
	DbgFunc("BOOKMARKLIST::Add: " << bookmark << endl);


	// Add bookmark to the internal list.
	//
	bookmarks.Add(bookmark);


	// Resort the list.
	//
	Load(err);


	// Mark bookshelf as having been modified.
	//
	bookshelf->MarkDirty();
}

// Is bookmark list empty?
//
BOOL
BOOKMARKLIST::IsEmpty(void)
{
	LIST<BOOKMARK*>	&bookmarks = bookshelf->bookmarks;

	assert(objstate.IsReady());
	assert(bookshelf != NULL);
	DbgFunc("BOOKMARKLIST::IsEmpty" << endl);

	return(bookmarks.IsEmpty());
}

BOOKMARK *
BOOKMARKLIST::GetBookmark(int entry) const
{
	BOOKMARK	*bookmark;


	assert(objstate.IsReady());
	assert(entry >= 0  &&  entry < NumEntries());

	bookmark = (BOOKMARK *) GetClientData(entry);

	if (bookmark == NULL)
		return(NULL);
	DbgFunc("BOOKMARKLIST::GetBookmark[" << entry << "]: "
		<< bookmark->Title() << endl);

	return(bookmark);

}

void
BOOKMARKLIST::UpdateEntry(int entry)
{
	BOOKMARK	*bookmark = GetBookmark(entry);
	STRING		indent("      ");

	assert(objstate.IsReady());
	assert(bookshelf != NULL);
	assert(bookmark != NULL);
	DbgFunc("BOOKMARKLIST::UpdateEntry[" << entry << "]: "
		<< bookmark->Title() << endl);

	WINLIST::UpdateEntry(entry, indent + bookmark->Title());


	// Mark bookshelf as having been modified.
	//
	bookshelf->MarkDirty();
}

void
BOOKMARKLIST::DeleteEntry(int entry)
{
	LIST<BOOKMARK*>	&bookmarks = bookshelf->bookmarks;
	BOOKMARK	*bookmark = GetBookmark(entry);
	int		i;


	assert(objstate.IsReady());
	assert(bookshelf != NULL);
	assert(bookmark != NULL);
	DbgFunc("BOOKMARKLIST::DeleteEntry[" << entry << "]: "
		<< bookmark->Title() << endl);


	// Find and delete bookmark object.
	//
	bookmark = (BOOKMARK *) GetClientData(entry);
	assert(bookmark != NULL);

	for (i = 0; i < bookmarks.Count(); i++) {
		if (bookmarks[i] == bookmark)
			break;
	}
	assert(i < bookmarks.Count());

	bookmarks.Delete(i);


	// Delete bookmark entry from bookmark list.
	//
	WINLIST::DeleteEntry(entry);


	// Mark bookshelf as having been modified.
	//
	bookshelf->MarkDirty();
}

int
CompareBookmarks(const void *arg1, const void *arg2)
{
	BOOKMARK	*bm1 = *((BOOKMARK **)arg1);
	BOOKMARK	*bm2 = *((BOOKMARK **)arg2);

	if (bm1->AnswerBookTitle() == bm2->AnswerBookTitle()) {
		return(0);
	} else if (bm1->AnswerBookTitle() > bm2->AnswerBookTitle()) {
		return(1);
	} else {
		return(-1);
	}
}
