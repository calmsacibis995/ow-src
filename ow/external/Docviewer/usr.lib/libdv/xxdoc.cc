#ident "@(#)xxdoc.cc	1.13 93/02/15 Copyright 1990-92 Sun Microsystems, Inc."


#include <doc/xxdoc.h>
#include "isamrec.h"	//XXX for max field lengths
#include "dvlocale.h"


XXDOC::XXDOC()
{
	DbgFunc("XXDOC::XXDOC" << endl);

	parent       = NULL;
	first_child  = NULL;
	last_child   = NULL;
	next_sibling = NULL;
	prev_sibling = NULL;

	private_data = NULL;

	objstate.MarkReady();
}

XXDOC::~XXDOC()
{
	DbgFunc("XXDOC::~XXDOC" << endl);
}

STATUS
XXDOC::SetName(const DOCNAME &docname, ERRSTK &err)
{
	assert(objstate.IsReady());
	assert(docname.IsValid());
	DbgFunc("XXDOC::SetName: " << docname << endl);
	name = docname;

	if (docname.DocId().Length() > ISAMREC_ID_LEN) {

		err.Init(DGetText("doc id exceeds %d characters: %s\n"),
			ISAMREC_ID_LEN, ~docname.DocId());
		return(STATUS_FAILED);

	} else if (docname.DocId().Length() > 16/*XXX*/) {

		fprintf(stderr, DGetText(
   "Warning: doc id longer than recommended length of 16 characters: %s\n"),
			~docname.DocId());
	}

	return(STATUS_OK);
}

STATUS
XXDOC::SetTitle(const STRING &s, ERRSTK &)
{
	assert(objstate.IsReady());
	DbgFunc("XXDOC::SetTitle: " << s << endl);
	title = STRING::CleanUp(s);

	if (title.Length() > ISAMREC_TITLE_LEN) {
		title = title.SubString(0, ISAMREC_TITLE_LEN-1);
		title = STRING::CleanUp(title);
		fprintf(stderr, DGetText(
		    "Warning: title exceeds %d characters - truncating: %s\n"),
		    ISAMREC_TITLE_LEN, ~title);
	}

	return(STATUS_OK);
}

STATUS
XXDOC::SetLabel(const STRING &s, ERRSTK &)
{
	assert(objstate.IsReady());
	DbgFunc("XXDOC::SetLabel: " << s << endl);
	label = STRING::CleanUp(s);

	if (label.Length() > ISAMREC_LABEL_LEN) {
		label = label.SubString(0, ISAMREC_LABEL_LEN-1);
		label = STRING::CleanUp(label);
		fprintf(stderr, DGetText(
		    "Warning: label exceeds %d characters - truncating: %s\n"),
		    ISAMREC_LABEL_LEN, ~label);
	}

	return(STATUS_OK);
}

STATUS
XXDOC::SetViewMethod(const STRING &s, ERRSTK &err)
{
	assert(objstate.IsReady());
	DbgFunc("XXDOC::SetViewMethod: " << s << endl);
	view_method = STRING::CleanUp(s);

	if (view_method.Length() > ISAMREC_VIEW_METHOD_LEN) {
		err.Init(DGetText("view method exceeds %d characters: %s\n"),
			ISAMREC_VIEW_METHOD_LEN, ~view_method);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
XXDOC::SetPrintMethod(const STRING &s, ERRSTK &err)
{
	assert(objstate.IsReady());
	DbgFunc("XXDOC::SetPrintMethod: " << s << endl);
	print_method = STRING::CleanUp(s);

	if (print_method.Length() > ISAMREC_PRINT_METHOD_LEN) {
		err.Init(DGetText("print method exceeds %d characters: %s\n"),
			ISAMREC_PRINT_METHOD_LEN, ~print_method);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
XXDOC::SetPageRange(const STRING &s, ERRSTK &)
{
	int	first_page, last_page;
	STRING	tmp;

	assert(objstate.IsReady());
	DbgFunc("XXDOC::SetPageRange: " << s << endl);

	tmp = STRING::CleanUp(s);
	
	first_page = -1;
	last_page  = -1;
	if (tmp != NULL_STRING)
		sscanf(tmp, "%d - %d", &first_page, &last_page);

	PGRANGE	pgrange(first_page, last_page);
	range = pgrange;

	return(STATUS_OK);
}

STATUS
XXDOC::SetFile(const STRING &s, ERRSTK &)
{
	STRING	tmp;

	assert(objstate.IsReady());
	DbgFunc("XXDOC::SetFile: " << s << endl);

	tmp = STRING::CleanUp(s);

	if (StrCaseCmp(tmp, "true")  ==  0  ||
	    StrCaseCmp(tmp, "yes")   ==  0)
		flags |= DF_FILE;

	return(STATUS_OK);
}

STATUS
XXDOC::SetNoShow(const STRING &s, ERRSTK &)
{
	STRING	tmp;

	assert(objstate.IsReady());
	DbgFunc("XXDOC::SetNoShow: " << s << endl);

	tmp = STRING::CleanUp(s);

	if (StrCaseCmp(tmp, "true")  ==  0  ||
	    StrCaseCmp(tmp, "yes")   ==  0)
		flags |= DF_NOSHOW;

	return(STATUS_OK);
}

STATUS
XXDOC::SetNoShowKids(const STRING &s, ERRSTK &)
{
	STRING	tmp;

	assert(objstate.IsReady());
	DbgFunc("XXDOC::SetNoShowKids: " << s << endl);

	tmp = STRING::CleanUp(s);

	if (StrCaseCmp(tmp, "true")  ==  0  ||
	    StrCaseCmp(tmp, "yes")   ==  0)
		flags |= DF_NOSHOWKIDS;

	return(STATUS_OK);
}

STATUS
XXDOC::SetSymLink(const STRING &s, ERRSTK &err)
{
	STRING	tmp;

	assert(objstate.IsReady());
	DbgFunc("XXDOC::SetSymLink: " << s << endl);
	assert(name.IsValid());

	tmp = STRING::CleanUp(s);

	if (sym_link.Init(tmp) != STATUS_OK) {
		err.Init(DGetText("Can't create symlink: %s"), ~s);
		return(STATUS_FAILED);
	}
	sym_link.Resolve(name);
	assert(sym_link.IsValid());

	//XXX
	//XXX Check for symlink longer than ISAMREC_SYM_LINK_LEN
	//XXX

	return(STATUS_OK);
}

void
XXDOC::AddChild(XXDOC *child)
{
	ERRSTK	notused;

	assert(objstate.IsReady());
	assert(child != NULL);
	assert(child->GetParent(notused) == NULL);
	assert(child->GetFirstChild(notused) == NULL);
	assert(child->GetLastChild(notused) == NULL);
	assert(child->GetNextSibling(notused) == NULL);
	assert(child->GetPrevSibling(notused) == NULL);
	DbgFunc("XXDOC::AddChild: add " << child << " to " << this << endl);

	if (GetFirstChild(notused) == NULL) {
		assert(last_child == NULL);
		first_child = child;
		last_child = child;

	} else { 
		assert(last_child->GetNextSibling(notused) == NULL);
		last_child->next_sibling = child;
		child->prev_sibling = last_child;
		last_child = child;
	}

	child->parent = this;
}
