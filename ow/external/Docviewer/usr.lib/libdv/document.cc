#ident "@(#)document.cc	1.34 06/11/93 Copyright 1992 Sun Microsystems, Inc."



#include <doc/document.h>


DOCUMENT::DOCUMENT()
{
	DbgFunc("DOCUMENT::DOCUMENT" << endl);

	flags       = 0;

	// NOTE that we don't mark this object "Ready" here.
	// This is only done in "IsValid()" below, which
	// forces users of this object to call IsValid()
	// before using it.
	//
}

DOCUMENT::DOCUMENT(const DOCUMENT &doc)
{
	DbgFunc("DOCUMENT::DOCUMENT(DOCUMENT &)" << endl);

	name         = doc.name;
	title        = doc.title;
	view_method  = doc.view_method;
	print_method = doc.print_method;
	label        = doc.label;
	sym_link     = doc.sym_link;
	range        = doc.range;
	flags        = doc.flags;

	// NOTE that we don't mark this object "Ready" here.
	// This is only done in "IsValid()" below, which
	// forces users of this object to call IsValid()
	// before using it.
	//
}

DOCUMENT::~DOCUMENT()
{
	DbgFunc("DOCUMENT::~DOCUMENT: " << this << endl);
}

// See if document is valid.
//
BOOL
DOCUMENT::IsValid()
{
	DbgFunc("DOCUMENT::IsValid: " << this << endl);

	if ( ! name.IsValid())
		return(BOOL_FALSE);

	//XXX need to be more thorough.

	objstate.MarkReady();
	return(BOOL_TRUE);
}

// Set offset for document:
void
DOCUMENT::SetPageOffset(int offsetvalue)
{
  name.SetOffset(offsetvalue);
}

// See if two documents are equivalent.
//
BOOL
DOCUMENT::Equal(const DOCUMENT *doc1, const DOCUMENT *doc2)
{
	return((BOOL)	(doc1->Name()          == doc2->Name()		&&
			 doc1->Title()         == doc2->Title()		&&
			 doc1->ViewMethod()    == doc2->ViewMethod()	&&
			 doc1->PrintMethod()   == doc2->PrintMethod()	&&
			 doc1->range           == doc2->range)
		);
}

// Print document title.
//
ostream &
operator << (ostream &out, const DOCUMENT *doc)
{
	if (doc != NULL)
		out << doc->Name() << " (" << doc->Title() << ")";

	return(out);
}
