#ifndef	_BOOKMARK_H
#define	_BOOKMARK_H

#ident "@(#)bookmark.h	1.6 06/11/93 Copyright 1990-1992 Sun Microsystems, Inc."

#include <doc/common.h>
#include <doc/docname.h>

#define	BM_MAXLINE	512		// longest allowed bookmark line
#define	BM_MAXCOMMENT	(8*1024)	// longest allowed bookmark comment


class	BOOKMARK {

    private:

	DOCNAME	docname;	// object name of document we're bookmarking
	STRING	title;		// bookmark title
	STRING	annotation;	// bookmark annotation
	STRING	abtitle;	// title of AnswerBook to which bookmrk belongs


    public:

	BOOKMARK(const DOCNAME &name) : docname(name)	{ }
	~BOOKMARK()				{ }

	// Get/Set values of various bookmark fields.
	//
	const DOCNAME	&DocName() const	{ return(docname); }
	const STRING	&Title() const		{ return(title); }
	const STRING	&Annotation() const	{ return(annotation); }
	const STRING	&AnswerBookTitle() const { return(abtitle); }

	void		SetAnnotation(const STRING &);
	void		SetTitle(const STRING &);
	void		SetAnswerBookTitle(const STRING &);

	// Print contents of bookmark to ostream.
	//
	friend ostream	&operator << (ostream &, const BOOKMARK *);
};

#endif	_BOOKMARK_H
