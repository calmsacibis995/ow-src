#ifndef	_XXDOC_H
#define	_XXDOC_H

#ident "@(#)xxdoc.h	1.9 06/11/93 Copyright 1989-92 Sun Microsystems, Inc."


#include <doc/document.h>


class	XXDOC : public DOCUMENT {

    private:

	// XX-specific fields.
	//
	XXDOC		*parent;
	XXDOC		*first_child;
	XXDOC		*last_child;
	XXDOC		*next_sibling;
	XXDOC		*prev_sibling;

	caddr_t		private_data;

	OBJECT_STATE	objstate;


    public:

	XXDOC();
	~XXDOC();

	// Set document attributes.
	//
	STATUS		SetName(const DOCNAME &, ERRSTK &);
	STATUS		SetTitle(const STRING &, ERRSTK &);
	STATUS		SetLabel(const STRING &, ERRSTK &);
	STATUS		SetViewMethod(const STRING &, ERRSTK &);
	STATUS		SetPageRange(const STRING &, ERRSTK &);
	STATUS		SetPrintMethod(const STRING &, ERRSTK &);
	STATUS		SetFile(const STRING &, ERRSTK &);
	STATUS		SetNoShow(const STRING &, ERRSTK &);
	STATUS		SetNoShowKids(const STRING &, ERRSTK &);
	STATUS		SetSymLink(const STRING &, ERRSTK &);

	// Get document relatives.
	//
	DOCUMENT	*GetParent(ERRSTK &) const	{ return parent;}
	DOCUMENT	*GetNextSibling(ERRSTK &) const	{ return next_sibling;}
	DOCUMENT	*GetPrevSibling(ERRSTK &) const	{ return prev_sibling;}
	DOCUMENT	*GetFirstChild(ERRSTK &) const	{ return first_child;}
	DOCUMENT	*GetLastChild(ERRSTK &) const	{ return last_child;}

	// Add child document to this document.
	//
	void		AddChild(XXDOC *child);

	//
	//
	caddr_t		GetPrivateData() const	{ return(private_data); }
	void		SetPrivateData(caddr_t data) { private_data = data; }
};

#endif	_XXDOC_H
