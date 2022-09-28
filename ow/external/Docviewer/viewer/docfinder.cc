#ident "@(#)docfinder.cc	1.30 93/03/09 Copyright 1990 Sun Microsystems, Inc."


#include "docfinder.h"
#include <doc/abgroup.h>
#include <doc/abclient.h>
#include <doc/document.h>


extern ABGROUP	*abgroup;


DOCFINDER::DOCFINDER() :
	seed_doc	(NULL)
{
	DbgFunc("DOCFINDER::DOCFINDER" << endl);
	objstate.MarkReady();
}

DOCFINDER::~DOCFINDER()
{
	DbgFunc("DOCFINDER::~DOCFINDER" << endl);

	if (seed_doc != NULL)
		delete(seed_doc);
}

void
DOCFINDER::SeedDoc(const DOCNAME &docname)
{
	DOCUMENT	*seed;
	DOCUMENT	*parent, *child;
	ERRSTK		err;


	assert(objstate.IsReady());
	DbgFunc("DOCFINDER::SeedDoc: " << docname << endl);


	// Delete current seed document.
	//
	if (seed_doc != NULL)
		delete(seed_doc);
	seed_doc = NULL;


	// Go up the document hierarchy until we find a document
	// whose 'DocFile' field is non-empty.
	//
	for (seed  = abgroup->LookUpDoc(docname, 0, err);
	     seed != NULL &&  ! seed->IsDocFile();
	     seed  = parent) {
		parent = seed->GetParent(err);
		delete(seed);
	}


	// If we didn't find document with non-empty 'DocFile' field
	// on the way up, look for one on the way back down.
	//
	if (seed == NULL) {
		for (seed  = abgroup->LookUpDoc(docname, 0, err);
		     seed != NULL  &&  ! seed->IsDocFile();
		     seed  = child) {
			child = seed->GetFirstChild(err);
			delete(seed);
		}
	}

	if (seed == NULL) {
		DbgFunc("DOCFINDER::SeedDoc: don't know where I am" << endl);
		return;
	}

	assert(seed->IsDocFile());


	// Verify that seed document has a valid page range field.
	// Our algorithms depend on this fact.
	//
	if ( ! seed->Range().IsValid()) {
		delete(seed);
		DbgFunc("DOCFINDER::SeedDoc: bad page range" << endl);
		return;
	}

	seed_doc = seed;
	DbgMed("DOCFINDER::SeedDoc: got it: " << seed_doc << endl);
}

STATUS
DOCFINDER::GetNextFile(DOCNAME &docname, ERRSTK &err)
{
	DOCUMENT	*next;


	if ( ! IsInited()) {
		err.Init(gettext("don't know where I am"));
		return(STATUS_FAILED);
	}


	assert(objstate.IsReady());
	assert(seed_doc->IsDocFile());
	DbgFunc("DOCFINDER::GetNextFile: seed doc = " << seed_doc << endl);


	if ((next = seed_doc->GetNextSibling(err))  ==  NULL) {
		err.Init(gettext("No next file"));
		return(STATUS_FAILED);
	}

	if ( ! next->IsDocFile()) {	// must have "file=yes" attrib.
		err.Init(gettext("No next file"));
		delete(next);
		return(STATUS_FAILED);
	}

	docname = next->Name();
	delete(next);
	return(STATUS_OK);
}

STATUS
DOCFINDER::GetPrevFile(DOCNAME &docname, ERRSTK &err)
{
	DOCUMENT	*prev;


	if ( ! IsInited()) {
		err.Init(gettext("don't know where I am"));
		return(STATUS_FAILED);
	}


	assert(objstate.IsReady());
	assert(seed_doc->IsDocFile());
	DbgFunc("DOCFINDER::GetPrevFile: seed doc = " << seed_doc << endl);


	if ((prev = seed_doc->GetPrevSibling(err))  ==  NULL) {
		err.Init(gettext("No previous file"));
		return(STATUS_FAILED);
	}

	if ( ! prev->IsDocFile()) {	// must have "file=yes" attrib.
		err.Init(gettext("No previous file"));
		delete(prev);
		return(STATUS_FAILED);
	}

	docname = prev->Name();
	delete(prev);
	return(STATUS_OK);
}

// Find logical document begins on physical page 'page'.
//
// If there are several such documents, select the document
// at the lowest level in the document hierarchy.
// E.g., if a chapter and the first section within that chapter
// both begin on that page, select the section rather than the chapter.
//
// If no documents actually begin on 'page', select the lowest-level
// document whose page range includes 'page', and that begins
// closest to 'page'.
//
STATUS
DOCFINDER::GetDocOnPage(int page, DOCNAME &docname, ERRSTK &err)
{
	DOCUMENT	*doc;
	int		page_offset;


	assert(objstate.IsReady());
	DbgFunc("DOCFINDER::GetDocOnPage: " << page << endl);
	
	if (abgroup == NULL  ||  ! IsInited()) {
		err.Init(gettext("don't know where I am"));
		return(STATUS_FAILED);
	}


	// The page number we were given is the physical page number
	// from the beginning of the physical file.
	// Translate that to the logical page number.
	//
	page = CvtToLogicalPage(page);
	assert(page >= 0);


	// Find the document on this page that meets our criteria.
	//
	if ((doc = DocOnPage(page, seed_doc, err))  ==  NULL)
		return(STATUS_FAILED);

	docname = doc->Name();


	// Determine offset of page from beginning of document.
	//
	assert(doc->Range().IsValid());
	assert(page >= doc->Range().FirstPage());
	assert(page <= doc->Range().LastPage());

	page_offset = page - doc->Range().FirstPage();
	docname.SetOffset(page_offset);
	DbgMed("DOCFINDER::GetDocOnPage:"
		<< "  doc='"	<< doc	<< "'"
		<< ", page="	<< page
		<< endl);


	// Clean up after ourselves, being careful not to delete
	// the Seed, since it could possibly be the doc we found.
	//
	if (doc != seed_doc)
		delete(doc);

	return(STATUS_OK);
}

// Implement 'GetDocOnPage()' algorithm.
// Returns new document (caller is responsible for deallocation)
// or NULL on failure.
//
DOCUMENT *
DOCFINDER::DocOnPage(int page, DOCUMENT *start, ERRSTK &err)
{
	DOCUMENT	*close, *closer;
	DOCUMENT	*doc;
	DOCUMENT	*next;


	assert(objstate.IsReady());
	assert(abgroup != NULL);
	assert(start != NULL);
	assert(IsInited());
	DbgFunc("DOCFINDER::DocOnPage: " << page << " (" << start << ")\n");



	// Get page range of starting document.
	// Make sure it encompasses the page we're looking for.
	//
	if ( ! start->Range().IsValid()  ||  ! start->Range().Includes(page)) {
		err.Init(gettext("bad page range for '%s'"), ~start->Title());
		return(NULL);
	}


	// If this document is a terminal (lowest level in the
	// document hierarchy, declare victory.
	//
	if (start->IsLeaf()) {
		DbgMed("DOCFINDER::DocOnPage: found it: " << start << endl);
		return(start);
	}


	// Otherwise, go down to the next level in the doc hierarchy
	// (search list of 'start's children) looking for the
	// child that includes 'page' in its page range.
	//
	for (close = start->GetFirstChild(err); close != NULL; close = next) {

		if ( ! close->Range().IsValid()) {
			err.Init(gettext("bad page range for '%s'"),
				~close->Title());
			delete(close);
			return(NULL);
		}

		if (close->Range().Includes(page)) {
			DbgHigh("DOCFINDER::DocOnPage: page in range: "
				<< close << endl);
			break;
		}

		next = close->GetNextSibling(err);
		delete(close);
	}


	// In certain cases, we won't have found any docs at this level
	// that include 'page' in their page range.  This happens,
	// e.g., when we're on the second page of a new chapter,
	// but the first section in that chapter doesn't start until
	// the third page.
	// In this case, just return the chapter.
	//
	if (close == NULL)
		return(start);


	DbgLow("DOCFINDER::DocOnPage: close: " << close << endl);


	// In the above loop, we found the first document at this level
	// in the hierarchy whose page range includes 'page'.
	// If 'page' is the last page of this document,
	// it's possible that the next document actually begins on 'page'.
	// In that case, we'd rather have that next document.
	//
	if (close->Range().LastPage() == page				&&
	    close->Range().LastPage() != close->Range().FirstPage()	&&
	    (closer = close->GetNextSibling(err))  !=  NULL) {

		if (closer->Range().IsValid()   &&
		    closer->Range().FirstPage() == page) {
			delete(close);		// clean up
			close = closer;		// switch
			DbgLow("DOCFINDER::DocOnPage: closer: "
				<< closer << endl);
		} else {
			delete(closer);
		}
	}


	assert(close != NULL);
	assert(close->Range().Includes(page));
	DbgLow("DOCFINDER::DocOnPage: got it (intermediate): "
			<< close << endl);


	// Recursively descend the hierarchy looking
	// for the document closest to 'page'.
	//
	doc = DocOnPage(page, close, err);

	if (doc == NULL  ||  doc == close) {
		return(close);
	} else {
		delete(close);
		return(doc);
	}
}

int
DOCFINDER::CvtToLogicalPage(int page)
{
	assert(objstate.IsReady());
	assert(IsInited());
	assert(seed_doc != NULL);
	assert(seed_doc->Range().IsValid());
	assert(page >= 0);
	DbgFunc("DOCFINDER::CvtToLogicalPage: "
		<< "page="	<< page
		<< ", range="	<< seed_doc->Range()
		<< endl);

	return(page + seed_doc->Range().FirstPage() - 1);
}
