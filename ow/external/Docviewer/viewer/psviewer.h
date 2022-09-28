/* RE_SID: @(%)/tmp_mnt/net/ltfu/opt3/eswarp.sandbox/dv-1093src/external/Docviewer/viewer/SCCS/s.psviewer.h 1.64 93/01/05 15:28:12 SMI */
#ifndef	_PSVIEWER_H_
#define	_PSVIEWER_H_

#ident "@(#)psviewer.h	1.64 05 Jan 1993 Copyright 1989-1992 Sun Microsystems, Inc."

#include "viewer.h"
#include <doc/psdoc.h>

// Forward class declarations.
//
class	DOCFINDER;
class	DOCINFO;
class	HIST_STACK;
class	PSDOC;

class	PSVIEWER : public VIEWER {

    private:
	static const BBox	defaultBBox;
	static const STRING	defaultName;
	static const STRING	clear_page;

	float		defaultMag;	// default mag value
	PSDOC		*psdoc;		// PS document handle
	Event		prevEvent;
	BOOL		resizeOnLoad;
	int		selected;
	BOOL		sentProlog;	// has prolog been sent?
	BOOL		showLinks;	// show or hide links?
	ViewerType	viewerType;	// Is this a helpviewer?


	// Private Functions

	// Link related methods
	int		GetSelectedLink(const int	xv_xloc,
					const int	xv_yloc);

	BOOL		LinkSelected()
	{
		return((selected >= 0) ? BOOL_TRUE : BOOL_FALSE);
	}

	void		ClearSelection()
	{
		selected = -1;
	}

	const BBox	&LinkDisplayBox(const PSLINK &link);

    public:
	
	PSVIEWER(ViewerType vtype);
       ~PSVIEWER();

	// Load a new document.
	STATUS		LoadDocument(	const DOCNAME &docname,
					const STRING &filename,
					const STRING &pathlist,
				        ERRSTK&);

	// Return current document name
	const STRING	      &GetDocName() const
	{
		return((psdoc) ? psdoc->Name() : defaultName);
	}

	// Document positioning.
	int		GetCurrPage() const
	{
		return(currPage);
	}

	int		GetNumPages() const
	{
		return((psdoc) ? psdoc->NumPages() : 0);
	}

	// Remember current document for future reference.
	void		SetCurrDoc(const DOCNAME &docname)
	{
		currDocName = docname;
	}

	// Get current document.
	const DOCNAME	&GetCurrDoc() const
	{
		return(currDocName);
	}

	// Return the DisplayBox for this document
	const BBox       &DocDisplayBox() const
	{
		return((psdoc) ? psdoc->DisplayBox() : defaultBBox);
	}

	// Describe document we are currently viewing
	void		ShowPageInfo(int);

	// Handle repaint requests
	STATUS		Repaint();

	STATUS		ExecuteSelection();
	STATUS		ExtendSelection(int, int) { return(STATUS_FAILED); }
	STATUS		SetSelection(Event *const ev);

	// Showing and hiding links
	void		HideLinks();
	void		ShowLinks();


	STATUS		GotoFirstPage();
	STATUS		GotoPage(const int pagenum);
	STATUS		GotoLastPage();

};

#endif	/* _PSVIEWER_H_ */
