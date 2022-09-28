#ifndef	_TOCMGR_H
#define	_TOCMGR_H

#ident "@(#)tocmgr.h	1.19 06/11/93 Copyright 1990 Sun Microsystems, Inc."


#include "modemgr.h"

// Forward class declarations.
//
class	ABGROUP;
class	CONTENTS;
class	DOCUMENT;
class	LOCATION;
class	NOTIFY;


// TOCMGR manages all aspects of table of contents navigation.
//
class	TOCMGR : public MODEMGR {

    private:

	// TOC Location list
	// Displays current location within the TOC hierarchy.
	//
	LOCATION	*location;

	// TOC contents list.
	// Displays the contents of the currently selected entry
	// in the Location list.
	//
	CONTENTS	*contents;

	// AnswerBooks with which we're currently dealing.
	//
	ABGROUP		*abgroup;

	// Add a document to the TOC Location list and
	// display its contents in the Contents list.
	//
	STATUS		Expand(const DOCNAME &docname, ERRSTK &err);

	// Main event handler.
	//
	void		EventHandler(int event, caddr_t event_obj);

	// Event handler for single- and double-clicks in Location and
	// Contents lists.
	// This method is declared "static" because it is used
	// as a callback in an environment where its class is not known.  
	//
	static void	ListEvent(	int	event_type,
					caddr_t	event_obj,
					caddr_t	client_data);

	// Resize TOC panel - relayout widgets as needed.
	//
	void		Resize();


    public:

	// TOC Manager constructor, destructor.
	//
	TOCMGR(Xv_opaque frame, NOTIFY *notify, ABGROUP *abgroup_arg);
	~TOCMGR();

	// Update table of contents.
	//
	STATUS		Update(ERRSTK &err);

	// Make the toc panel and widgets (in)visible.
	//
	void		Show(BOOL show);

	// Get the currently selected document in the Contents list.
	// Assumes there is a current selection (see "HasSelection()").
	//
	const DOCNAME	&GetSelection() const;

	// Is there a currently selected document in this Contents list?
	//
	BOOL		HasSelection() const;
};


// Special name for "Bookshelf" entry in TOC.
//
extern /*const*/ DOCNAME	BOOKSHELF_NAME;

#endif	_TOCMGR_H
