#ident "@(#)hist_stack.cc	1.12 03/09/93 Copyright 1990 Sun Microsystems, Inc."

#include "hist_stack.h"


class HIST_ITEM {

    public:

	HIST_ITEM(const DOCNAME &dn, const STRING &pth, int pg) :
		docname(dn), path(pth), page(pg) { }

	DOCNAME	docname;
	STRING	path;
	int	page;
};


HIST_STACK::HIST_STACK(int max_depth_arg) :
	max_depth	(max_depth_arg+1)
{
	DbgFunc("HIST_STACK::HIST_STACK" << endl);
	objstate.MarkReady();
}

HIST_STACK::~HIST_STACK()
{
	DbgFunc("HIST_STACK::~HIST_STACK" << endl);

	Clear();
}

void
HIST_STACK::Push(const DOCNAME &docname, const STRING &path, int page)
{
	assert(objstate.IsReady());
	assert(docname.IsValid() || path != NULL_STRING);
	DbgFunc("HIST_STACK::Push: " << docname << endl);


	// Add new history item to the history stack.
	//
	stack.Add(new HIST_ITEM(docname, path, page));


	// Don't let the stack exceed its maximum depth.
	//
	if (Depth() > max_depth) {
		delete stack[0];
		stack.Delete(0);
	}
}

void
HIST_STACK::Pop(DOCNAME &docname, STRING &path, int &page)
{
	HIST_ITEM      *hist;
	int		top;


	assert(objstate.IsReady());
	assert(Depth() > 0);


	top  = stack.Count() - 1;
	hist = stack[top];
	stack.Delete(top);

	docname = hist->docname;
	path    = hist->path;
	page    = hist->page;

	delete(hist);

	DbgFunc("HIST_STACK::Pop: " << docname << endl);
}

void
HIST_STACK::Clear()
{
	int	i;

	assert(objstate.IsReady());
	DbgFunc("HIST_STACK::Clear" << endl);

	for (i = 0; i < stack.Count(); i++)
		delete stack[i];

	stack.Clear();
}
