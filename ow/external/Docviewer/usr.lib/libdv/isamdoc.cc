#ident "@(#)isamdoc.cc	1.14 93/02/15 Copyright 1990 Sun Microsystems, Inc."


#include <doc/isamdoc.h>
#include <doc/xxdoc.h>
#include "dvlocale.h"
#include "isamrec.h"


// Invalid ISAM record number - useful for initialization.
//
const long	ISAM_RECNUM_INVALID = 0;


ISAMDOC::ISAMDOC(const ISAMREC *isamrec, ISAMBOOK *isambook)
{
	STRING	sym_link_str;

	assert(isamrec != NULL);
	assert(isambook != NULL);
	DbgFunc("ISAMDOC::ISAMDOC(ISAMREC, ISAMBOOK)" << endl);


	if (name.SetDocId(isamrec->id)  !=  STATUS_OK)
		return;
	name.Resolve(isambook->Name());

	title            = STRING::CleanUp(isamrec->title);
	label            = STRING::CleanUp(isamrec->label);
	parent_rec       = isamrec->parent_rec;
	first_child_rec  = isamrec->first_child_rec;
	last_child_rec   = isamrec->last_child_rec;
	next_sibling_rec = isamrec->next_sibling_rec;
	prev_sibling_rec = isamrec->prev_sibling_rec;

	PGRANGE	pgrange(isamrec->first_page, isamrec->last_page);
	range = pgrange;

	flags = 0;

	if (isamrec->flags & IF_NOSHOW)
		flags |= DF_NOSHOW;

	if (isamrec->flags & IF_NOSHOWKIDS)
		flags |= DF_NOSHOWKIDS;

	if (isamrec->flags & IF_DOCFILE)
		flags |= DF_FILE;

	if (isamrec->flags & IF_ISSYMLINK) {
		sym_link_str  = STRING::CleanUp(isamrec->view_method);
		sym_link_str += STRING::CleanUp(isamrec->print_method);
		sym_link.Init(sym_link_str);
	} else {
		view_method  = STRING::CleanUp(isamrec->view_method);
		print_method = STRING::CleanUp(isamrec->print_method);
	}

	// Remember the book database from whence we came.
	// Needed in subsequent "Get*()" operations.
	// Increment book's reference count.
	//
	book = isambook;
	book->AddRef();
	DbgMed("ISAMDOC::ISAMDOC:"
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

// Create ISAMDOC from an XXDOC.
//
ISAMDOC::ISAMDOC(const XXDOC &doc) : DOCUMENT(doc)
{
	XXDOC	*parent;
	XXDOC	*first_child, *last_child;
	XXDOC	*next_sibling, *prev_sibling;
	ERRSTK	notused;


	DbgFunc("ISAMDOC::ISAMDOC(XXDOC): " << &doc << endl);


	parent_rec       = ISAM_RECNUM_INVALID;
	first_child_rec  = ISAM_RECNUM_INVALID;
	last_child_rec   = ISAM_RECNUM_INVALID;
	next_sibling_rec = ISAM_RECNUM_INVALID;
	prev_sibling_rec = ISAM_RECNUM_INVALID;


	// XXX Assume someone has properly set record numbers
	// XXX in private data field.
	//
	if ((parent = (XXDOC *)doc.GetParent(notused)) != NULL)
		parent_rec = (long) parent->GetPrivateData();

	if ((first_child = (XXDOC *)doc.GetFirstChild(notused)) != NULL)
		first_child_rec = (long) first_child->GetPrivateData();

	if ((last_child = (XXDOC *)doc.GetLastChild(notused)) != NULL)
		last_child_rec = (long) last_child->GetPrivateData();

	if ((next_sibling = (XXDOC *)doc.GetNextSibling(notused)) != NULL)
		next_sibling_rec = (long) next_sibling->GetPrivateData();

	if ((prev_sibling = (XXDOC *)doc.GetPrevSibling(notused)) != NULL)
		prev_sibling_rec = (long) prev_sibling->GetPrivateData();


	book = NULL;	// assume we don't need the book


	// NOTE that we don't mark this object "Ready" here.
	// This is only done in "IsValid()" below, which
	// forces users of this object to call IsValid()
	// before using it.
	//
}

ISAMDOC::~ISAMDOC()
{
	DbgFunc("ISAMDOC::~ISAMDOC" << endl);

	// Decrement book's reference count.
	//
	if (book != NULL) {
		book->DeleteRef();
		DbgMed("ISAMDOC::~ISAMDOC:"
			<< " refcount="	<< book->RefCount()
			<< " for "	<< book
			<< ", doc: "	<< this
			<< endl);
	}
}

BOOL
ISAMDOC::IsValid()
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
ISAMDOC::IsLeaf() const
{
	assert(objstate.IsReady());
	DbgFunc("ISAMDOC::IsLeaf: " << this << endl);

	if (IsValidRecNum(first_child_rec)  ||  IsSymLink())
		return(BOOL_FALSE);
	else
		return(BOOL_TRUE);
}

DOCUMENT *
ISAMDOC::GetFirstChild(ERRSTK &err) const
{
	assert(objstate.IsReady());
	assert(book != NULL);
	DbgFunc("ISAMDOC::GetFirstChild: " << this << endl);

	if ( ! IsValidRecNum(first_child_rec)) {
		return(NULL);
	} else {
		return(book->GetDocByRecNum(first_child_rec, err));
	}
}

DOCUMENT *
ISAMDOC::GetLastChild(ERRSTK &err) const
{
	assert(objstate.IsReady());
	assert(book != NULL);
	DbgFunc("ISAMDOC::GetLastChild: " << this << endl);

	if ( ! IsValidRecNum(last_child_rec)) {
		return(NULL);
	} else {
		return(book->GetDocByRecNum(last_child_rec, err));
	}
}

DOCUMENT *
ISAMDOC::GetNextSibling(ERRSTK &err) const
{
	assert(objstate.IsReady());
	assert(book != NULL);
	DbgFunc("ISAMDOC::GetNextSibling: " << this << endl);

	if ( ! IsValidRecNum(next_sibling_rec)) {
		return(NULL);
	} else {
		return(book->GetDocByRecNum(next_sibling_rec, err));
	}
}

DOCUMENT *
ISAMDOC::GetPrevSibling(ERRSTK &err) const
{
	assert(objstate.IsReady());
	assert(book != NULL);
	DbgFunc("ISAMDOC::GetPrevSibling: " << this << endl);

	if ( ! IsValidRecNum(prev_sibling_rec)) {
		return(NULL);
	} else {
		return(book->GetDocByRecNum(prev_sibling_rec, err));
	}
}

DOCUMENT *
ISAMDOC::GetParent(ERRSTK &err) const
{
	assert(objstate.IsReady());
	assert(book != NULL);
	DbgFunc("ISAMDOC::GetParent: " << this << endl);

	if ( ! IsValidRecNum(parent_rec)) {
		return(NULL);
	} else {
		return(book->GetDocByRecNum(parent_rec, err));
	}
}

// Convert generic document object into ISAM record.
// Returns STATUS_OK if document object is valid and conversion succeeds,
// otherwise returns STATUS_FAILED.
//
STATUS
CvtDocToRec(const XXDOC *doc, ISAMREC *isamrec, ERRSTK &err)
{
	XXDOC	*tmp;


	assert(doc != NULL  &&  isamrec != NULL);
	DbgFunc("CvtDocToRec: " << doc << endl);


	// Verify that document values do not exceed
	// field lengths in ISAM record.
	//
	if (doc->Id().Length()          > ISAMREC_ID_LEN	    ||
	    doc->Title().Length()       > ISAMREC_TITLE_LEN	    ||
	    doc->Label().Length()       > ISAMREC_LABEL_LEN	    ||
	    doc->ViewMethod().Length()  > ISAMREC_VIEW_METHOD_LEN   ||
	    doc->PrintMethod().Length() > ISAMREC_PRINT_METHOD_LEN) {
		err.Init(DGetText("one or more fields is too long"));
		return(STATUS_FAILED);
	}


	// Copy values into fields of ISAM record.
	//
	StrCpy(isamrec->id,    doc->Id());
	StrCpy(isamrec->title, doc->Title());
	StrCpy(isamrec->label, doc->Label());

	isamrec->first_page = doc->Range().FirstPage();
	isamrec->last_page  = doc->Range().LastPage();


	// Set flags appropriately in ISAM record.
	//
	isamrec->flags = 0;

	if (doc->IsDocFile())
		isamrec->flags |= IF_DOCFILE;
	if (doc->IsNoShow())
		isamrec->flags |= IF_NOSHOW;
	if (doc->IsNoShowKids())
		isamrec->flags |= IF_NOSHOWKIDS;


	if (doc->IsSymLink()) {

		// This document is a symbolic link (to another document).
		// We store the value of the symbolic link in the
		// ISAM record's view_method and print_method fields
		// in order to save space - symbolic link documents
		// never use these fields.  Note that the max length
		// of a symbolic link is defined as the sum of the
		// max lengths of the print_method and view_method fields.
		//
		STRING	symlink_str = doc->SymLink().NameToString();
		STRING	first_half, second_half;
		int	n = ISAMREC_VIEW_METHOD_LEN;

		if (symlink_str.Length() > ISAMREC_SYM_LINK_LEN) {
			err.Init(DGetText("symbolic link value is too long"));
			return(STATUS_FAILED);
		}

		first_half  = symlink_str.SubString(0, n-1);
		second_half = symlink_str.SubString(n, END_OF_STRING);
		
		StrCpy(isamrec->view_method,  first_half);
		StrCpy(isamrec->print_method, second_half);
		isamrec->flags |= IF_ISSYMLINK;

	} else {
		// This document is NOT a symbolic link.
		// Just store the view/print method values in the
		// respective fields in the ISAM record.
		//
		StrCpy(isamrec->view_method,  doc->ViewMethod());
		StrCpy(isamrec->print_method, doc->PrintMethod());
	}


	// Store the record numbers of this document's relatives
	// (parent, children, siblings).
	// Two assumptions are being made here:
	//	1) Someone has already set up the relationships
	//	   via calls to XXDOC::AddChild()
	//	2) Someone has stashed a valid ISAM record number
	//	   in each of the relatives' private data fields.
	//
	isamrec->parent_rec       = ISAM_RECNUM_INVALID;
	isamrec->first_child_rec  = ISAM_RECNUM_INVALID;
	isamrec->last_child_rec   = ISAM_RECNUM_INVALID;
	isamrec->next_sibling_rec = ISAM_RECNUM_INVALID;
	isamrec->prev_sibling_rec = ISAM_RECNUM_INVALID;

	if ((tmp = (XXDOC *)doc->GetParent(err))  !=  NULL)
		isamrec->parent_rec = (long) tmp->GetPrivateData();

	if ((tmp = (XXDOC *)doc->GetFirstChild(err))  !=  NULL)
		isamrec->first_child_rec = (long) tmp->GetPrivateData();

	if ((tmp = (XXDOC *)doc->GetLastChild(err))  !=  NULL)
		isamrec->last_child_rec = (long) tmp->GetPrivateData();

	if ((tmp = (XXDOC *)doc->GetNextSibling(err))  !=  NULL)
		isamrec->next_sibling_rec = (long) tmp->GetPrivateData();

	if ((tmp = (XXDOC *)doc->GetPrevSibling(err))  !=  NULL)
		isamrec->prev_sibling_rec = (long) tmp->GetPrivateData();


	// Initialize "spare" fields in record to reasonable values.
	//
	isamrec->spare_long_1 = 0;
	isamrec->spare_long_2 = 0;
	StrCpy(isamrec->spare_char63_1, "");
	StrCpy(isamrec->spare_char63_2, "");

	return(STATUS_OK);
}
