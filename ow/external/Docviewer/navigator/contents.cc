#ident "@(#)contents.cc	1.13 01/10/94 Copyright 1990 Sun Microsystems, Inc."


#include "contents.h"
#include <doc/abclient.h>
#include <doc/abgroup.h>
#include <doc/document.h>


static Server_image	blank_glyph = XV_NULL;
static short		blank_bits[16] = {
#include <images/blank.pr>
};

// Special name for "Bookshelf" entry in TOC.
//
extern const DOCNAME	BOOKSHELF_NAME;


CONTENTS::CONTENTS(Xv_opaque panel, int x, int y, ABGROUP *abgroup_arg) :
	WINLIST		(panel, x, y),
	abgroup		(abgroup_arg)
{
	assert(panel != NULL);
	assert(abgroup != NULL);
	DbgFunc("CONTENTS::CONTENTS" << endl);

	xv_set(XvHandle(), XV_HELP_DATA, TOC_CONTENTS_HELP, XV_NULL);


	if (blank_glyph == XV_NULL) {

		blank_glyph = xv_create(NULL, SERVER_IMAGE,
				XV_WIDTH,		16,
				XV_HEIGHT,		16,
				SERVER_IMAGE_BITS,	blank_bits,
				NULL);

		if (blank_glyph == XV_NULL)
			OutOfMemory();
	}

	WINLIST::SetEventHandler(CONTENTS::WinListEvent, (caddr_t)this);

	objstate.MarkReady();
}

// Clear this Contents list.
//
void
CONTENTS::Clear()
{
	assert(objstate.IsReady());
	DbgFunc("CONTENTS::Clear" << endl);

	doclist.Clear();
	WINLIST::Clear();
}

// Display the contents (children) of the specified document.
//
STATUS
CONTENTS::Display(const DOCNAME &docname, int indent, ERRSTK &err)
{
	DOCUMENT	*doc;		// document we're displaying
	DOCUMENT	*child;		// child document
	DOCUMENT	*next;		// next child document
	ABNAME		abname;		// name of AB in contents list
	DOCNAME		rootname;	// name of AB root document
	STRING		istring;	// indentation string
	STRING		title;		// entry title
	BOOL		bold;		// display entry in bold font?
	u_long		flags;
	int		i;


	assert(objstate.IsReady());
	assert(docname.IsValid());
	DbgFunc("CONTENTS::Display: " << docname << endl);


	// Clear the current list.
	//
	CONTENTS::Clear();


	// Determine correct level of indentation.
	//
	//   (removed 1/6/94 .... Eric told me to!)
	// for (i = 0; i < indent; i++)
	//	istring += "     ";


	// Batch updates to "winlist" to prevent screen flicker.
	//
	WINLIST::BeginBatch();


	if (docname == BOOKSHELF_NAME) {

		for (i = 0; i < abgroup->answerbooks.Count(); i++) {

			// Look for root document of this AnswerBook.
			// Tell the lookup mechanism to get the "Preferred
			// Language" translation of the book if it's available.
			// Otherwise it'll get the English version.
			//
			abname = abgroup->answerbooks[i]->Name();
			MakeAnswerBookRootDocName(abname, rootname);
			flags = LU_PREFERRED_LANG;
			doc = abgroup->LookUpDoc(rootname, flags, err);
			if (doc == NULL) {
				cerr << "Can't get doc: " << abname << endl;
				continue;
			}

			// Format title.
			//
			//title = istring + doc->Title();
			title = doc->Title();
	

			// Then add it to the list.
			//
			bold = BOOL_TRUE;
			WINLIST::AppendEntry(title, bold, blank_glyph);
			doclist.Add(doc->Name());
			delete doc;
		}

	} else {

		// Find document record for the named document.
		//
		if ((doc = abgroup->LookUpDoc(docname, 0, err))  ==  NULL) {
			WINLIST::EndBatch();
			return(STATUS_FAILED);
		}
		assert( ! doc->IsLeaf());
		assert( ! doc->IsNoShowKids());
		assert( ! doc->IsSymLink());


		// Add each of parent's children to list
		// (unless document's "noshow" attribute is set).
		//
		for (child=doc->GetFirstChild(err); child!=NULL; child=next) {

			if ( ! child->IsNoShow()) {

				// Format title.
				//
				//title = istring + child->Title();
				title = child->Title();

				// Then add it to the list.
				// Make it boldface if it has children
				// and some of those children are displayable.
				//
				if (child->IsLeaf() || child->IsNoShowKids())
					bold = BOOL_FALSE;
				else
					bold = BOOL_TRUE;
				WINLIST::AppendEntry(title, bold, blank_glyph);
				doclist.Add(child->Name());
			}

			next = child->GetNextSibling(err);
			delete(child);
		}

		delete (doc) ; 	/* for purify */
	}


	WINLIST::EndBatch();


	// Make sure contents list is visible.
	//
	WINLIST::SetViewTop(0);


	return(STATUS_OK);
}

const DOCNAME &
CONTENTS::GetDocName(int entry)
{
	assert(objstate.IsReady());
	assert(doclist[entry].IsValid());
	DbgFunc("CONTENTS::GetDocName: " << doclist[entry] << endl);

	return(doclist[entry]);
}

void
CONTENTS::EventHandler(int event)
{
	assert(objstate.IsReady());


	switch (event) {

	case WINLIST_SELECT_EVENT:

		DbgFunc("CONTENTS::EventHandler: SELECT: " << endl);
		event = CONTENTS_SELECT_EVENT;
		if (event_handler != NULL)
		    (*event_handler)(event, (caddr_t)this, event_client_data);
		break;

	case WINLIST_EXECUTE_EVENT:

		DbgFunc("CONTENTS::EventHandler: EXECUTE: " << endl);
		event = CONTENTS_EXECUTE_EVENT;
		if (event_handler != NULL)
		    (*event_handler)(event, (caddr_t)this, event_client_data);
		break;

	default:
		DbgFunc("CONTENTS::EventHandler: other" << endl);
		break;
	}
}

// Handle events from Contents list's WINLIST.
//
void
CONTENTS::WinListEvent(int event, caddr_t /*event_obj*/, caddr_t client_data)
{
	CONTENTS	*contents = (CONTENTS *) client_data;

	assert(contents != NULL);
	DbgFunc("CONTENTS::WinListEvent: " << event << endl);

	// Just pass event on to main CONTENTS event handler.
	//
	contents->EventHandler(event);
}
