#ifndef	_CONTENTS_H
#define	_CONTENTS_H

#ident "@(#)contents.h	1.5 06/11/93 Copyright 1990-1992 Sun Microsystems, Inc."


#include "winlist.h"
#include <doc/list.h>
#include <doc/docname.h>


// Events generated by CONTENTS.
//
typedef enum {
	CONTENTS_SELECT_EVENT	= 23456,
	CONTENTS_EXECUTE_EVENT	= 23457
} CONTENTS_EVENT;


// Forward class references.
//
class	ABGROUP;


class	CONTENTS : public WINLIST {

    private:

	// List of DOCNAMEs in this Contents list.
	//
	LIST<DOCNAME>	doclist;

	// Event handler and accompanying callback argument for
	// CONTENTS events.
	//
	EVENT_HANDLER	event_handler;
	caddr_t		event_client_data;

	// AnswerBooks with which we're currently dealing.
	//
	ABGROUP		*abgroup;

	// Current state of this object.
	//
	OBJECT_STATE	objstate;

	// Main event handler.
	//
	void		EventHandler(int event);

	// Event handler for Contents list events ("select", "execute").
	// This method is declared "static" because it is used
	// as a callback in an environment where its class is not known.  
	//
	static void	WinListEvent(	int	event_type,
					caddr_t	event_obj,
					caddr_t	client_data);


    public:

	// CONTENTS constructor/destructor and initialization routines.
	// Note that 'Init()' must be called after a CONTENTS object
	// is instantiated but before it is used.
	//
	CONTENTS(Xv_opaque panel, int x, int y, ABGROUP *abgroup_arg);
	~CONTENTS()				{ }

	// Clear this Contents list.
	//
	void		Clear();

	// Display the contents (children) of a particular
	// TOC entry (document).
	// Assumes "docname" is *not* a leaf document and
	// is *not* a symbolic link.
	//
	STATUS		Display(const DOCNAME &docname, int indent, ERRSTK &);

	// Get the DOCNAME of specified entry.
	//
	const DOCNAME	&GetDocName(int);

	// Get the number of entries in this list.
	//
	int		NumDocs() const		{ return(doclist.Count()); }

	// Register event handler for CONTENTS events.
	//
	void	SetEventHandler(EVENT_HANDLER func, caddr_t clnt)
			{ event_handler = func; event_client_data = clnt; }
};

#endif	_CONTENTS_H
