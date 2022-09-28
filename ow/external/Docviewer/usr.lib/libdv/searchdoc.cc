#ident "@(#)searchdoc.cc	1.5 06/11/93 Copyright 1992 Sun Microsystems, Inc."


#include <doc/searchdoc.h>


SEARCHDOC::SEARCHDOC(const DOCNAME &name, int wght) :
	docname	(name),
	weight	(wght)
{
	DbgFunc("SEARCHDOC::SEARCHDOC: " << this << endl);
}

SEARCHDOC::SEARCHDOC(const SEARCHDOC &searchdoc) :
	docname	(searchdoc.docname),
	weight	(searchdoc.weight)
{
	DbgFunc("SEARCHDOC::SEARCHDOC: " << this << endl);
}

ostream & 
operator << (ostream &ostr, const SEARCHDOC *doc) 
{
	if (doc != NULL)
	  	ostr << doc->DocName() << "\t(" << doc->weight << ")";

	return ostr ;
}
