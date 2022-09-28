#ident "@(#)dbmdoc.cc	1.15 06/11/93 Copyright 1990 Sun Microsystems, Inc."



#include <doc/dbmdoc.h>
#include <doc/xxdoc.h>
#include <doc/attrlist.h>
#include <doc/errstk.h>


static const STRING	DOC_ID("ID");
static const STRING	DOC_TITLE("TI");
static const STRING	DOC_LABEL("LB");
static const STRING	DOC_PARENT("PD");
static const STRING	DOC_FIRST_CHILD("FC");
static const STRING	DOC_NEXT_SIBLING("NC");
static const STRING	DOC_VIEW_METHOD("VI");
static const STRING	DOC_PRINT_METHOD("PR");
static const STRING	DOC_PAGE_RANGE("RG");
static const STRING	DOC_FILE("DF");
static const STRING	DOC_NOSHOW("ND");
static const STRING	DOC_NOSHOWKIDS("NK");


// XXX Need to handle book links properly.
// XXX Watch out for confusion between "book link" and "first child".
//
DBMDOC::DBMDOC(ATTRLIST &attrs, DBMBOOK *book_arg) : book(book_arg)
{
	int	first_page, last_page;
	STRING	range_str;
	DOCNAME	tmpname;

	assert(book != NULL);
	assert(book->Name().IsValid());
	DbgFunc("DBMDOC::DBMDOC" << endl);


	// Get document name.
	//
	name.Strict(BOOL_FALSE);	// relax strict DOCNAME type checking
	if (name.Init(attrs[DOC_ID])  !=  STATUS_OK  ||
	    name.BookId() != book->Id())
		return;
	name.Resolve(book->Name());

	// Get ids of relatives (parent, siblings, children).
	// Note that the 'first child' attribute semantics are overloaded:
	// it can either represent the id of the document's first child
	// (within the current book), or the id of a document in
	// another book (in which case it's a "symbolic link" to that book).
	//
	tmpname.Strict(BOOL_FALSE);	// relax strict DOCNAME type checking
	if (attrs[DOC_PARENT] != NULL_STRING) {
		if (tmpname.Init(attrs[DOC_PARENT])  ==  STATUS_OK)
			parent_id = tmpname.DocId();
	}
	if (attrs[DOC_NEXT_SIBLING] != NULL_STRING) {
		if (tmpname.Init(attrs[DOC_NEXT_SIBLING])  ==  STATUS_OK)
			next_sibling_id = tmpname.DocId();
	}

	// Determine if 'first child' represents a symbolic link.
	//
	if (attrs[DOC_FIRST_CHILD] != NULL_STRING) {
		if (tmpname.Init(attrs[DOC_FIRST_CHILD])  ==  STATUS_OK) {
			tmpname.Resolve(book->Name());
			if (tmpname.BookId() != book->Id()) {
				sym_link = tmpname;
			} else {
				first_child_id = tmpname.DocId();
			}
		}
	}

	// Get other document attributes.
	//
	title           = attrs[DOC_TITLE];
	view_method     = attrs[DOC_VIEW_METHOD];
	print_method    = attrs[DOC_PRINT_METHOD];
	label           = attrs[DOC_LABEL];

	// Parse "page range" string.
	// Format is "first - last".
	//
	first_page = -1;
	last_page  = -1;
	range_str = attrs[DOC_PAGE_RANGE];
	if (range_str != NULL_STRING)
		sscanf(range_str, "%d - %d", &first_page, &last_page);
	PGRANGE	pgrange(first_page, last_page);
	range = pgrange;

	if (StrCaseCmp(attrs[DOC_NOSHOW], "true")  ==  0  ||
	    StrCaseCmp(attrs[DOC_NOSHOW], "yes")   ==  0) {
		flags |= DF_NOSHOW;
	}

	if (StrCaseCmp(attrs[DOC_NOSHOWKIDS], "true")  ==  0  ||
	    StrCaseCmp(attrs[DOC_NOSHOWKIDS], "yes")   ==  0) {
		flags |= DF_NOSHOWKIDS;
	}

	if (StrCaseCmp(attrs[DOC_FILE], "true")  ==  0  ||
	    StrCaseCmp(attrs[DOC_FILE], "yes")   ==  0) {
		flags |= DF_FILE;
	}

	// Remember the book database from whence we came.
	// Needed in subsequent "Get*()" operations.
	// Increment book's reference count.
	//
	book->AddRef();
	DbgMed("DBMDOC::DBMDOC:"
		<< " refcount="	<< book->RefCount()
		<< " for "	<< book
		<< ", doc: "	<< this
		<< endl);


	// NOTE that we don't mark this object "Ready" here.
	// This is only done in "IsValid()" below, which
	// forces users of this object to call IsValid()
	// before using it.
	//
}

// Create DBMDOC from an XXDOC.
//
DBMDOC::DBMDOC(const XXDOC &doc) : DOCUMENT(doc)
{
	DOCUMENT	*parent, *first_child, *next_sibling;
	ERRSTK		notused;

	DbgFunc("DBMDOC::DBMDOC(XXDOC)" << endl);

	if ((parent = doc.GetParent(notused)) != NULL)
		parent_id = parent->Id();

	if ((first_child = doc.GetFirstChild(notused)) != NULL)
		first_child_id = first_child->Id();

	if ((next_sibling = doc.GetNextSibling(notused)) != NULL)
		next_sibling_id = next_sibling->Id();


	book = NULL;	// assume we don't need the book


	// NOTE that we don't mark this object "Ready" here.
	// This is only done in "IsValid()" below, which
	// forces users of this object to call IsValid()
	// before using it.
	//
}

DBMDOC::~DBMDOC()
{
	DbgFunc("DBMDOC::~DBMDOC" << endl);

	// Decrement book's reference count.
	//
	if (book != NULL) {
		book->DeleteRef();
		DbgMed("DBMDOC::~DBMDOC:"
			<< " refcount="	<< book->RefCount()
			<< " for "	<< book
			<< ", doc: "	<< this
			<< endl);
	}
}

BOOL
DBMDOC::IsValid()
{
	if ( ! DOCUMENT::IsValid())
		return(BOOL_FALSE);

	//XXX Do something here.
	//

	// This doc is ready to roll...
	//
	objstate.MarkReady();

	return(BOOL_TRUE);
}

BOOL
DBMDOC::IsLeaf() const
{
	assert(objstate.IsReady());
	DbgFunc("DBMDOC::IsLeaf: " << this << endl);

	if (first_child_id == NULL_STRING  &&  ! IsSymLink())
		return(BOOL_TRUE);
	else
		return(BOOL_FALSE);
}

DOCUMENT *
DBMDOC::GetFirstChild(ERRSTK &err) const
{
	assert(objstate.IsReady());
	assert(book != NULL);
	DbgFunc("DBMDOC::GetFirstChild: " << this << endl);

	if (IsLeaf()) {
		return(NULL);
	} else {
		return(book->GetDocById(first_child_id, err));
	}
}

DOCUMENT *
DBMDOC::GetLastChild(ERRSTK &/*err*/) const
{
	assert(objstate.IsReady());
	assert(book != NULL);
	DbgFunc("DBMDOC::GetLastChild: " << this << endl);

	//XXX
	//
	cerr << "DBMDOC::GetLastChild: not yet implemented" << endl;

	return(NULL);
}

DOCUMENT *
DBMDOC::GetNextSibling(ERRSTK &err) const
{
	assert(objstate.IsReady());
	assert(book != NULL);
	DbgFunc("DBMDOC::GetNextSibling: " << this << endl);

	if (next_sibling_id == NULL_STRING) {
		return(NULL);
	} else {
		return(book->GetDocById(next_sibling_id, err));
	}
}

DOCUMENT *
DBMDOC::GetPrevSibling(ERRSTK &err) const
{
	DOCUMENT	*parent, *sibling, *nextsib;

	assert(objstate.IsReady());
	assert(book != NULL);
	DbgFunc("DBMDOC::GetPrevSibling: " << this << endl);

	if ((parent = this->GetParent(err))  ==  NULL) {
		return(NULL);
	}

	if ((sibling = parent->GetFirstChild(err))  ==  NULL) {
		delete(parent);
		return(NULL);
	}

	delete(parent);


	// If doc is first child of its parent, it has no previous sibling.
	//
	if (sibling->Name().DocId() == this->Name().DocId()) {
		delete(sibling);
		return(NULL);
	}


	// Scan list of siblings from first to last looking for the one
	// whose next sibling is 'this'.
	//
	while (((DBMDOC *)sibling)->next_sibling_id != this->Name().DocId()) {

		if ((nextsib = sibling->GetNextSibling(err))  ==  NULL) {
			delete(sibling);
			return(NULL);
		}

		delete(sibling);
		sibling = nextsib;
	}

	return(sibling);
}

DOCUMENT *
DBMDOC::GetParent(ERRSTK &err) const
{
	assert(objstate.IsReady());
	assert(book != NULL);
	DbgFunc("DBMDOC::GetParent: " << this << endl);

	if (parent_id == NULL_STRING) {
		return(NULL);
	} else {
		return(book->GetDocById(parent_id, err));
	}
}

STATUS
CvtDocToAttrList(const XXDOC *doc, ATTRLIST &attrs, ERRSTK &/*err*/)
{
	XXDOC	*parent, *first_child, *next_sibling;
	ERRSTK	notused;
	char	buf[50];

	assert(doc != NULL);
	DbgFunc("CvtDocToAttrList: " << doc << endl);

	attrs.Clear();

	attrs[DOC_ID]           = doc->Name().ShortName();
	attrs[DOC_TITLE]        = doc->Title();
	attrs[DOC_LABEL]        = doc->Label();
	attrs[DOC_VIEW_METHOD]  = doc->ViewMethod();
	attrs[DOC_PRINT_METHOD] = doc->PrintMethod();

	if ((parent = (XXDOC *)doc->GetParent(notused))  !=  NULL)
		attrs[DOC_PARENT] = parent->Name().ShortName();

	if ((first_child = (XXDOC *)doc->GetFirstChild(notused))  !=  NULL)
		attrs[DOC_FIRST_CHILD] = first_child->Name().ShortName();

	if ((next_sibling = (XXDOC *)doc->GetNextSibling(notused))  !=  NULL)
		attrs[DOC_NEXT_SIBLING] = next_sibling->Name().ShortName();


	if (doc->Range().IsValid()) {
		sprintf(buf, "%d - %d",
			doc->Range().FirstPage(),
			doc->Range().LastPage());
		attrs[DOC_PAGE_RANGE] = buf;
	}

	if (doc->IsDocFile())
		attrs[DOC_FILE] = "yes";

	if (doc->IsNoShow())
		attrs[DOC_NOSHOW] = "true";

	if (doc->IsNoShowKids())
		attrs[DOC_NOSHOWKIDS] = "true";

	if (doc->IsSymLink())
		attrs[DOC_FIRST_CHILD] = doc->SymLink().ShortName();

	return(STATUS_OK);
}
