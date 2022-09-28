#ifndef	_SEARCHDOC_H
#define	_SEARCHDOC_H

#ident "@(#)searchdoc.h	1.5 93/01/05 Copyright 1990-1992 Sun Microsystems, Inc."
#ident "@(#)searchdoc.h	1.8 11/15/96 Copyright 1990-1992 Sun Microsystems, Inc."

#include <doc/docname.h>


class SEARCHDOC {

    private:

	DOCNAME	docname;	// document's object name
	long	weight;		// document's relevance rank weight


    public:

	SEARCHDOC(const DOCNAME &name, int wght);
	SEARCHDOC(const SEARCHDOC &searchdoc);
	~SEARCHDOC()				{ }

	const DOCNAME	&DocName() const 	{ return(docname); }
	int		Weight() const		{ return(weight); }

        friend ostream	&operator << (ostream &, const SEARCHDOC *);

	// ABCLIENT needs access to "docname" in order to resolve it
	// properly.
	//
	friend class ABCLIENT;
};

#endif	_SEARCHDOC_H
