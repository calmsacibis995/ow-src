#ident "@(#)tocmgr.cc	1.59 94/01/31 Copyright 1990-1992 Sun Microsystems, Inc."

#include "tocmgr.h"
#include "contents.h"
#include "location.h"
#include <doc/abgroup.h>
#include <doc/bookshelf.h>
#include <doc/document.h>
#include <doc/notify.h>
#include <locale.h>


// Special name for "Bookshelf" entry in TOC.
//
//XXX const DOCNAME	BOOKSHELF_NAME("<ab=BKSHELF;bk=BKSHELF;dc=BKSHELF>");
DOCNAME			BOOKSHELF_NAME;


TOCMGR::TOCMGR(Xv_opaque frame, NOTIFY *notify, ABGROUP *abgroup_arg) :
	MODEMGR		(frame, notify),
	abgroup		(abgroup_arg)
{
	int		x, y;			// x, y coords of panel widgets


	assert(objstate.IsReady());	// should be set in MODEMGR::MODEMGR
	assert(mode_panel != NULL);	// ditto
	assert(abgroup != NULL);
	DbgFunc("TOCMGR::TOCMGR" << endl);


	xv_set(mode_panel,
			XV_HELP_DATA,		MODE_TOC_HELP,
			PANEL_LAYOUT,		PANEL_VERTICAL,
			NULL);

	// Create Location list.
	//
	x = 10;
	y = 0;
	location = new LOCATION(mode_panel, x, y, abgroup);
	xv_set(location->XvHandle(),
			PANEL_LIST_DISPLAY_ROWS, 6,
			PANEL_LAYOUT,		PANEL_VERTICAL,
			PANEL_LABEL_STRING,	gettext("Location:"),
			NULL);


	// Create Contents list below Location list.
	//
	x  = 10;
	y  = (int) xv_get(location->XvHandle(), XV_Y);
	y += (int) xv_get(location->XvHandle(), XV_HEIGHT) + 10;
	contents = new CONTENTS(mode_panel, x, y, abgroup);
	xv_set(contents->XvHandle(),
			PANEL_LAYOUT,		PANEL_VERTICAL,
			PANEL_LABEL_STRING,	gettext("Contents:"),
			NULL);


	// Register event handlers for Contents and Location lists.
	//
	contents->SetEventHandler(TOCMGR::ListEvent, (caddr_t)this);
	location->SetEventHandler(TOCMGR::ListEvent, (caddr_t)this);

	// Figure out minimum frame size now.
	// Just use total upper part and 3 rows of the contents list.

	int min_height, cur_min_height, cur_min_width, row_height;

	row_height = (int) xv_get (contents->XvHandle(), PANEL_LIST_ROW_HEIGHT);
	min_height = (int) xv_get (mode_panel, XV_Y) +
		     (int) xv_get (contents->XvHandle(), XV_Y) +
		     ( (int) xv_get (contents->XvHandle(), XV_HEIGHT) -
		  ((int) xv_get (contents->XvHandle(), PANEL_LIST_DISPLAY_ROWS)	
			 * row_height)) +
		     (row_height * 3);

	xv_get (frame, FRAME_MIN_SIZE, &cur_min_width, &cur_min_height);
	xv_set (frame, FRAME_MIN_SIZE, cur_min_width, min_height, NULL);

	// Initialize bookshelf name.
	//
	BOOKSHELF_NAME.SetABId("BKSHELF");
	BOOKSHELF_NAME.SetBookId("BKSHELF");
	BOOKSHELF_NAME.SetDocId("BKSHELF");
	assert(BOOKSHELF_NAME.IsValid());
}

TOCMGR::~TOCMGR()
{
	DbgFunc("TOCMGR::~TOCMGR\n");

	if (location != NULL)
		delete location;
	if (contents != NULL)
		delete contents;
}

STATUS
TOCMGR::Update(ERRSTK &err)
{
	assert(objstate.IsReady());
	assert(location != NULL  &&  contents != NULL);
	DbgFunc("TOCMGR::Update" << endl);


	notify->Busy(gettext("Initializing Table of Contents . . ."));

	contents->Clear();
	location->Clear();

	if (Expand(BOOKSHELF_NAME, err)  !=  STATUS_OK) {
		notify->ShowErrs(err);
		notify->Done();
		return(STATUS_FAILED);
	}

	notify->Done();
	return(STATUS_OK);
}

void
TOCMGR::EventHandler(int event, caddr_t /*event_obj*/)
{
	DOCNAME		docname;
	int		select;
	STRING		title;
	DOCUMENT	*doc;
	ERRSTK		err;


	assert(objstate.IsReady());
	DbgFunc("TOCMGR::EventHandler: " << event << endl);


	switch (event) {

	case LOCATION_SELECT_EVENT:

		// Get currently selected document in Location list.
		// If user selected current level, don't do anything.
		//
		select = location->GetSelection();
		assert(select >= 0  &&  select < location->NumDocs());
		if (select == location->NumDocs() - 1)
			break;

		notify->Busy("");

		docname = location->GetDocName(select);
		assert(docname.IsValid());

		// Display contents of current Location list selection
		// in the Contents list.
		//
		location->ClearBelow(select);
		contents->Display(docname, select+1, err);

		// Set Contents window label to document title.
		//
		title = gettext("Contents of ");
		if (docname == BOOKSHELF_NAME) {
			char *helios;
			if ((helios = getenv ("HELIOS_STARTED")) != 
							(char *) NULL)
				title += helios;
			else
				title += gettext("Library");
		} else if ((doc = abgroup->LookUpDoc(docname,0,err)) != NULL) {
			title += doc->Title();
			delete doc;
		} else {
			title += gettext("(No title)");
		}
		title += ":";
		xv_set(contents->XvHandle(), PANEL_LABEL_STRING, ~title, NULL);

		notify->Done();
		EVENTLOG1("toc", "show contents of %s", ~doc->Title());
		DbgFunc("TOCMGR::EventHandler: LOCATION_LIST_SELECT_EVENT: "
			<< docname << " (select " << select << ")" << endl);
		break;

	case CONTENTS_EXECUTE_EVENT:
	case MM_VIEW_EVENT:		// view currently selected document

		// Get currently selected document.
		//
		if ( ! HasSelection()) {
			notify->Warning(gettext("No document to display"));
			break;
		}

		// Do default "View" action.
		//
		MODEMGR::DefaultEventHandler(MM_VIEW_EVENT, (caddr_t)this);

		// If document has children, expand it.
		//
		docname = GetSelection();
		if (Expand(docname, err)  !=  STATUS_OK) {
			notify->ShowErrs(err);
		}
		break;

	case MM_PANEL_RESIZE_EVENT:	// panel resized: adjust panel widgets
		Resize();
		break;

	case CONTENTS_SELECT_EVENT:

		// Ignore these events - we're not interested.
		//
		break;

	default:
		// Pass event on to default handler.
		//
		MODEMGR::DefaultEventHandler(event, (caddr_t)this);
		break;
	}
}

void
TOCMGR::Show(BOOL flag)
{
	assert(objstate.IsReady());
	DbgFunc("TOCMGR::Show" << endl);

	xv_set(mode_panel,          XV_SHOW, flag, NULL);
	xv_set(contents->XvHandle(), XV_SHOW, flag, NULL);
	xv_set(location->XvHandle(), XV_SHOW, flag, NULL);
}

// Is there a "currently selected document" in the TOC?
// There are certain circumstances wherein the Contents list
// could be empty, e.g., all entries at this level are
// "noshow = true".
//
BOOL
TOCMGR::HasSelection() const
{
	assert(objstate.IsReady());
	assert(contents != NULL);
	DbgFunc("TOCMGR::HasSelection\n");

	if (contents->GetSelection()  <  0) {
		DbgFunc("TOCMGR::HasSelection: NO" << endl);
		return(BOOL_FALSE);
	} else {
		DbgFunc("TOCMGR::HasSelection: yes" << endl);
		return(BOOL_TRUE);
	}
}

// Get DOCNAME of currently selected document.
//
const DOCNAME &
TOCMGR::GetSelection() const
{
	DOCNAME		docname;
	int		selection;


	assert(objstate.IsReady());
	assert(contents != NULL);
	DbgFunc("TOCMGR::GetSelection\n");


	// Get index of current selection.
	//
	selection = contents->GetSelection();
	assert(selection >= 0);

	// Return DOCNAME of current selection.
	//
	return(contents->GetDocName(selection));
}

STATUS
TOCMGR::Expand(const DOCNAME &docname, ERRSTK &err)
{
	DOCUMENT	*doc;
	DOCNAME		tmpname;
	STRING		title;		// title of document we're expanding
	STRING		label;		// label of contents window
	int		indent;		// indentation level in Contents list
	u_long		flags;


	assert(objstate.IsReady());
	assert(contents != NULL  &&  location != NULL);
	assert(abgroup  != NULL);
	DbgFunc("TOCMGR::Expand: " << docname << endl);


	// The "Bookshelf" document must be handled differently
	// since it is not a real "DOCUMENT".
	//
	if (docname == BOOKSHELF_NAME) {
		char *helios;
		if ((helios = getenv ("HELIOS_STARTED")) != (char *) NULL)
			title   = helios;
		else
			title   = gettext("Library");
		tmpname = docname;
	} else {

		// Look up document.
		//
		flags = LU_RESOLVE_SYMLINK|LU_PREFERRED_LANG;
		if ((doc = abgroup->LookUpDoc(docname, flags, err))  ==  NULL)
			return(STATUS_FAILED);

		title = doc->Title();

		// If this is a leaf document (doc has no children),
		// or if all its children are "no show" (not displayable),
		// we obviously don't need to expand it.
		//
		if (doc->IsLeaf()  ||  doc->IsNoShowKids()) {
			if (doc->ViewMethod() == NULL_STRING) {
				notify->Warning(
					gettext("Document not viewable: %s"),
					~title);
			}
			delete doc;
			return(STATUS_OK);
		}

		tmpname = doc->Name();
		delete doc;
	}

	notify->Busy(gettext("Displaying contents of '%s'"), ~title);
	EVENTLOG1("toc", "expanding %s", ~title);
	

	// Append document to bottom of Location list.
	//
	if (location->AppendDoc(tmpname, err)  !=  STATUS_OK) {
		notify->Done();
		return(STATUS_FAILED);
	}


	// Display document's contents in Contents list.
	//
	indent = location->NumDocs();
	if (contents->Display(tmpname, indent, err)  !=  STATUS_OK) {
		notify->Done();
		return(STATUS_FAILED);
	}


	// Label Contents list appropriately.
	//
	label  = gettext("Contents of ");
	label += title;
	label += ":";
	xv_set(contents->XvHandle(), PANEL_LABEL_STRING, ~label, NULL);

	notify->Done();
	return(STATUS_OK);
}

void
TOCMGR::ListEvent(int event, caddr_t event_obj, caddr_t client_data)
{
	TOCMGR	*tocmgr = (TOCMGR *) client_data;

	assert(tocmgr != NULL);
	DbgFunc("TOCMGR::ListEvent: " << event << endl);

	// Just pass event on to main TOC event handler.
	//
	tocmgr->EventHandler(event, event_obj);
}

void
TOCMGR::Resize()
{
	int	panel_width;
	int	panel_height;
	int	location_width;
	int	contents_y;
	int	contents_width;
	int	contents_height;


	assert(objstate.IsReady());
	assert(location != NULL  &&  contents != NULL);
	DbgFunc("TOCMGR::Resize" << endl);


	panel_width   = (int) xv_get(mode_panel, XV_WIDTH);
	panel_height  = (int) xv_get(mode_panel, XV_HEIGHT);


	// Make Location list as wide as the panel.
	//
	location_width = panel_width - 10;
	location->FitWidth(location_width);


	// Make Contents list as wide as the panel
	// and extend it to the bottom of the panel.
	//
	contents_y      = (int) xv_get(contents->XvHandle(), XV_Y);
	contents_width  = panel_width - 10;
	contents_height = panel_height - contents_y - 10;
	contents->Fit(contents_width, contents_height);
}
