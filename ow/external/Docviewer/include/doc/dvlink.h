#ifndef _DVLINK_H
#define	_DVLINK_H

#ident "@(#)dvlink.h	1.7 11/15/96 Copyright 1989 Sun Microsystems, Inc."

#include <doc/common.h>
#include <doc/docname.h>
#include <doc/list.h>


// Link types.
//
typedef enum {
	DVLINK_INVALID,		// invalid link (syntax error, etc.)
	DVLINK_VIEWFILE,	// view (file) link
	DVLINK_VIEWDOCUMENT,	// view document link
	DVLINK_PRINT,		// print link
	DVLINK_PRINTFILE,	// printfile link
	DVLINK_SYSTEM		// system link
} DVLINKTYPE;


// View link argument types.
//
#define	VIEWPAGE_INVALID	0		// not valid
#define	VIEWPAGE_FIRST		(-1)		// goto first page of document
#define	VIEWPAGE_LAST		(-2)		// goto last page of document
#define	VIEWPAGE_NEXT		(-3)		// goto next page
#define	VIEWPAGE_PREV		(-4)		// goto preceding page


// Link construction tools.
// Each of these routines returns a pointer to a static STRING
// that is overwritten on subsequent calls.
//
const STRING	MakeViewFileLink(const STRING &file, int page=1);
const STRING	MakeViewDocumentLink(const DOCNAME &docname, int page=1);
const STRING	MakeSystemLink(const STRING &command);


class DVLINK {

    private:

	// Original link text.
	//
	STRING		cookie;

	// Link type.
	//
	DVLINKTYPE	link_type;

	// Link body (parameters).
	//
	STRING		link_body;

	// Page number of file or document object if this is a
	// DVLINK_VIEWFILE or DVLINK_VIEWDOCUMENT link.
	//
	int		view_page;

	// Parse link cookie.
	//
	STATUS		ParseIt();

	// Current state of this object.
	//
	OBJECT_STATE	objstate;


    public:

	// DVLINK constructor, destructor.
	//
	DVLINK();
	~DVLINK()	{ }

	// Set link cookie; initialize link.
	// Returns STATUS_OK if link is valid and was parsed successfully,
	// otherwise returns STATUS_FAILED.
	//
	STATUS		SetCookie(const STRING &);

	// Get link cookie.
	//
	const STRING	&GetCookie() const	{ return(cookie); }

	// Is this a valid link?
	//
	BOOL		IsValid() const;

	// Get link type.
	//
	DVLINKTYPE	LinkType() const	{ return(link_type); }

	// Get "view file" parameters (file and page number).
	// Assumes this is a valid DVLINK_VIEWFILE link.
	//
	void		ViewFile(STRING &file, int &page) const;

	// Get "view document" parameters (document name and page number).
	// Assumes this is a valid DVLINK_VIEWDOCUMENT link.
	//
	void		ViewDocument(DOCNAME &docname, int &page) const;

	// Get "print" parameters (file or list of files to print).
	// Assumes this is a valid DVLINK_PRINT link.
	//
	void		Print(LIST<STRING> &files_to_print) const;

	// Get "print file" parameters (name of file containing list of
	// files to print).
	// Assumes this is a valid DVLINK_PRINTFILE link.
	//
	void		PrintFile(STRING &printfile) const;

	// Get shell command parameters.
	// Assumes this is a valid DVLINK_SYSTEM link.
	//
	void		SystemCmd(STRING &syscmd) const;

	// Print link to "ostream".
	//
	friend ostream	&operator <<  (ostream &ostr, const DVLINK &link)
		{ return(ostr << link.GetCookie()); }
};

#endif _DVLINK_H
