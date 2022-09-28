#ident "@(#)location.cc	1.14 94/01/14 Copyright 1990-1992 Sun Microsystems, Inc."

#include "location.h"
#include "navigator.h"
#include <doc/abgroup.h>
#include <doc/document.h>


static short select_bits[16] = {
#include <images/arrow.pr>
};

static short deselect_bits[16] = {
#include <images/blank.pr>
};

static Server_image	select_glyph;
static Server_image	deselect_glyph;

// Special name for "Bookshelf" entry in TOC.
//
extern const DOCNAME	BOOKSHELF_NAME;


LOCATION::LOCATION(Xv_opaque panel, int x, int y, ABGROUP *abgroup_arg) :
	WINLIST		(panel, x, y),
	abgroup		(abgroup_arg),
	selected	(-1),
	event_handler	(NULL),
	event_arg	(NULL)
{
	DbgFunc("LOCATION::LOCATION");


	xv_set(XvHandle(),
			PANEL_CHOOSE_ONE,	FALSE,
			PANEL_CHOOSE_NONE,	TRUE,
			XV_HELP_DATA,		TOC_LOCATION_HELP,
			XV_NULL);


	select_glyph = xv_create(NULL, SERVER_IMAGE,
			XV_WIDTH,		16,
			XV_HEIGHT,		16,
			SERVER_IMAGE_BITS,	select_bits,
			NULL);

	deselect_glyph = xv_create(NULL, SERVER_IMAGE,
			XV_WIDTH,		16,
			XV_HEIGHT,		16,
			SERVER_IMAGE_BITS,	deselect_bits,
			NULL);

	if (select_glyph == NULL  ||  deselect_bits == NULL)
		OutOfMemory();


	// We want to 'interpose' on panel list selection events
	// in order to implement a custom selection UI.
	// We will treat 'select' and 'execute' (double-click) events
	// as equivalent.
	//
	WINLIST::SetEventHandler(LOCATION::WinListEvent, (caddr_t)this);


	objstate.MarkReady();
}

// Clear this Location list.
//
void
LOCATION::Clear()
{
	assert(objstate.IsReady());
	DbgFunc("LOCATION::Clear" << endl);

	doclist.Clear();
	WINLIST::Clear();
	selected = -1;
}

// Add level (document) to this Location list.
//
STATUS
LOCATION::AppendDoc(const DOCNAME &docname, ERRSTK &err)
{
	DOCUMENT	*doc;
	STRING		title;
	STRING		istring;


	assert(objstate.IsReady());
	assert(docname.IsValid());
	assert(abgroup != NULL);
	DbgFunc("LOCATION::AppendDoc: "
		<< docname << " (" << NumDocs() << ")" << endl);


	// Determine correct level of indentation for document title.
	//
	for (int i = 0; i < NumDocs(); i++)
		istring += "     ";


	// The Bookshelf entry is handled differently from
	// all other documents.
	//
	if (docname == BOOKSHELF_NAME) {
		char *helios;
		// Format title.
		//
		if ((helios = getenv ("HELIOS_STARTED")) != (char *) NULL)
		   title = helios;
		else
		   title = gettext("Library");

	} else {

		// Look up document.
		//
		if ((doc = abgroup->LookUpDoc(docname, 0, err))  ==  NULL)
			return(STATUS_FAILED);

		// Format title.
		//
		title = istring + doc->Title();
		if (doc->Label() != NULL_STRING)
			title += " (" + doc->Label() + ")";
		delete doc;
	}


	// Add document to Location list and to the list.
	//
	WINLIST::AppendEntry(title, BOOL_TRUE, deselect_glyph);
	doclist.Add(docname);
	SetSelection(NumDocs() - 1);
	WINLIST::SetViewBottom(NumDocs() - 1);


	return(STATUS_OK);
}

void
LOCATION::SetSelection(int level)
{
	int	i;


	assert(objstate.IsReady());
	assert(level >= 0  &&  level <= NumDocs());
	assert(selected < NumDocs());
	DbgFunc("LOCATION::SetSelection: " << level << endl);


	// Put list into batch update mode to avoid annoying flickering.
	//
	WINLIST::BeginBatch();


	// We're basically subverting the OpenLook selection spec here.
	// We want to indicate the selection via the "selected" glyph,
	// but *not* by highlighting the selected entry.
	//
	if (level != selected) {

		WINLIST::UpdateEntry(level, select_glyph);

		if (selected >= 0) {
			WINLIST::UpdateEntry(selected, deselect_glyph);
		} else {
			WINLIST::UpdateEntry(level, select_glyph);
		}
	}


	// Since this is a multiple-selection list,
	// sometimes we get multiple selections outstanding
	// (though I'm not exactly sure why).
	// Make sure that *all* entries are deselected.
	//
	for (i = 0; i < WINLIST::NumEntries(); i++) {
		WINLIST::Deselect(i);
	}


	// Remember new selection.
	//
	selected = level;


	// Wrap-up batch mode.
	//
	WINLIST::EndBatch();
}

// Delete all documents in Location list below specified level.
//
void
LOCATION::ClearBelow(int below)
{
	int	num_below;	// # documents in Location list below "below"
	int	i;


	assert(objstate.IsReady());
	assert(below >= 0  &&  below < NumDocs());
	DbgFunc("LOCATION::ClearBelow: " << below << endl);


	// See if there's anything below "below".
	//
	num_below = NumDocs() - below - 1;
	if (num_below == 0)
		return;

	// Delete documents below 'below' from document list.
	//
	for (i = 0; i < num_below; i++)
		doclist.Delete(below + 1);

	// Clear corresponding entries from panel list.
	//
	WINLIST::DeleteEntries(below+1, num_below);
	WINLIST::SetViewBottom( below );

	// Make sure lists are in synch.
	//
	assert(doclist.Count() == below+1);
	assert(doclist.Count() == WINLIST::NumEntries());

	// Note if we deleted 'current level'.
	//
	if (selected >= NumDocs())
		selected = -1;
}

// We want to 'interpose' on panel list selection events
// in order to implement a custom selection UI.
// We will treat 'select' and 'execute' (double-click) events
// as equivalent.
//
void
LOCATION::EventHandler(int event)
{
	int	newsel;


	assert(objstate.IsReady());


	switch (event) {

	case WINLIST_SELECT_EVENT:
	case WINLIST_EXECUTE_EVENT:

		// XXX	Sometimes it appears that there is no current
		// selection.  I don't understand this, but can imagine
		// it's some kind of race condition.  In these cases,
		// we'll simply ignore the selection event.
		//
		newsel = WINLIST::GetSelection();
		assert(newsel < NumDocs());
		if (newsel < 0)
			break;

		DbgFunc("LOCATION::EventHandler: SELECT: " << newsel << endl);
		SetSelection(newsel);

		// If someone has registered an event handler, invoke it.
		//
		event = LOCATION_SELECT_EVENT;
		if (event_handler != NULL)
			(*event_handler)(event, (caddr_t)this, event_arg);
		break;

	default:
		DbgFunc("LOCATION::EventHandler: other" << endl);
		break;
	}
}

// Handle events from Location list's WINLIST.
//
void
LOCATION::WinListEvent(int event, caddr_t /*event_obj*/, caddr_t client_data)
{
	LOCATION	*location = (LOCATION *) client_data;

	assert(location != NULL);
	DbgFunc("LOCATION::WinListEvent: " << event << endl);

	// Just pass event on to main LOCATION event handler.
	//
	location->EventHandler(event);
}
