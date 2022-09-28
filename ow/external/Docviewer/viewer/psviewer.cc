#ident "@(#)psviewer.cc	1.181 11/02/94 Copyright 1989-1992 Sun Microsystems, Inc."

#include "psviewer.h"
#include <xview/defaults.h>
#include <doc/list.h>
#include <doc/psdoc.h>
#include "docfinder.h"
#include "hist_stack.h"
#include <locale.h>
#include <math.h>
#include <DPS/psops.h>
#include <DPS/dpsclient.h>
#include "dpsdebug.h"
const BBox PSVIEWER::defaultBBox = { 0, 0, 612, 792 };
const STRING PSVIEWER::defaultName = "No document";
const STRING PSVIEWER::clear_page = "gsave newpath clippath 1.0 setgray fill grestore\n";
static const STRING NeWSServer = "NeWS";
static const STRING DPSServer  = "DPS";
extern "C" {
	int ds_is_double_click(Event *prev, Event *event);
};

PSVIEWER::PSVIEWER(ViewerType vtype) :
	VIEWER(vtype),
	resizeOnLoad(BOOL_FALSE)
{
	DbgFunc("PSVIEWER::PSVIEWER: entered" << endl);

	currPage	= 0;
	docfinder	= NULL;
	psdoc		= NULL;
	sentProlog	= BOOL_FALSE;
	showLinks	= BOOL_TRUE;

	memset((void *) &prevEvent, NULL, sizeof(Event));
	ClearSelection();

	if (abgroup)
		docfinder = new DOCFINDER();
}

STATUS
PSVIEWER::ExecuteSelection()
{
	STRING		cookie;
	DVLINK		dvlink;
	STATUS		status = STATUS_OK;
	ERRSTK		err;

	DbgFunc("PSVIEWER::ExecuteSelection: entered" << endl);
	assert(LinkSelected());

	cookie = psdoc->LinkList(currPage)[selected]->GetCookie();
	
	if (dvlink.SetCookie(cookie) != STATUS_OK) {
		status = STATUS_FAILED;
		notify->Warning(gettext("Can't follow hypertext link"));
	}
	else {
		VIEWER::EventProc(VE__EXECUTE_LINK, (const void *) &dvlink,
				   (void *) ((VIEWER *) this));
	}

	ClearSelection();

	uimgr->EnableFollowLinkItem(BOOL_FALSE);

	return(status);
}

int
PSVIEWER::GetSelectedLink(const int	xv_xloc,
			  const int	xv_yloc)
{
	const BBox	&bbox	= psdoc->DisplayBox();
	const int	canHght	= rint((float) mag * DocHght(bbox));

	STRING		sb_placement;
	int		xoffset = 0;

	LIST<PSLINK*>	&links	= psdoc->LinkList(currPage);

	int		ps_xloc;
	int		ps_yloc;
	int		new_xv_yloc;

	register int	cntr;

	DbgFunc("PSVIEWER::GetSelectedLink: entered" << endl);

	assert((float) mag);

	if (links.Count() == 0) {
		DbgLow("PSVIEWER::GetSelectedLink: no links on this page"
			<< endl);
		return(-1);
	}

	// Convert the event coordinates from the XView coordinate system to
	// the PostScript coordinate system.  Need to account for link
	// hit regions being offset if scrollbar placement is on the left.
	//
	sb_placement = defaults_get_string(
			"openWindows.scrollbarplacement",
			"OpenWindows.ScrollbarPlacement",
			"right");
	if (serverType == NeWSServer) {
	  if (sb_placement == "left")
	    xoffset = (int)xv_get(uimgr->vScrollbarHandle(), XV_WIDTH);
        }

	new_xv_yloc = xv_yloc;

	ps_xloc = bbox.ll_x + rint((xv_xloc - xoffset)/(float) mag);
	ps_yloc = bbox.ll_y + rint((canHght - new_xv_yloc)/(float) mag);

	DbgLow("PSVIEWER::GetSelectedLink: PS-Coords=("
	       << ps_xloc << ", " << ps_yloc << ")" << endl);

	// Do any of the link BBoxes include this point?  If so, return the
	// PSLINK that does.
	//
	for (cntr = 0;	cntr < links.Count(); cntr++) {
		if (links[cntr]->IncludesPoint(ps_xloc, ps_yloc))
			break;
	}

#ifdef	DEBUG
	if (cntr < links.Count()) {
		DbgLow("PSVIEWER::GetSelectedLink: found BBox: "
		       << links[cntr]->GetBBox().ll_x << " "
		       << links[cntr]->GetBBox().ll_y << " "
		       << links[cntr]->GetBBox().ur_x << " "
		       << links[cntr]->GetBBox().ur_y << endl);
	}
#endif	/* DEBUG */

	return((cntr < links.Count()) ? cntr : -1);
}

STATUS
PSVIEWER::GotoFirstPage()
{
	EVENTLOG("FIRST_PAGE");
	return(GotoPage(1));
}

STATUS
PSVIEWER::GotoLastPage()
{
	EVENTLOG("LAST_PAGE");
	return(GotoPage(psdoc->NumPages()));
}

STATUS
PSVIEWER::GotoPage(const int arg)
{
	const int	prevPage	= currPage;

	BOOL		disableGoback	= BOOL_FALSE;
	int		pageNum		= arg;

	DbgFunc("VIEWER::GotoPage: page=" << pageNum << endl);

	assert(psdoc->IsValid());

	if (pageNum < 1) {
		pageNum = 1;
	}
	else if (pageNum > GetNumPages()) {
		pageNum = GetNumPages();
	}

	// Reset the selected link index
	ClearSelection();

	uimgr->EnableFollowLinkItem(BOOL_FALSE);

	// If currPage == 0, the viewer has just loaded in a document.
	// LoadDocument() has already saved the current viewer state -
	// don't save it again
	//
	if (currPage > 0) {
		goback->Push(currDocName, GetDocName(), currPage);
	}

	if (goback->IsEmpty()) {
		disableGoback = BOOL_TRUE;
	}

	uimgr->DisableGoBack(disableGoback);

	ClearSelection();

	// If this is the first page of a new document, (prevPage ==
	// 0), the window might need to be resized, and XView will
	// automatically cause a repaint. Since repaints are
	// expensive, do not repaint twice.
	//
	currPage = pageNum;

	if (prevPage == 0 && resizeOnLoad) {
		uimgr->FitFrame((float) mag, DocDisplayBox());
		resizeOnLoad = BOOL_FALSE;
		int dochght = DocHght(DocDisplayBox());
		int docwdth = DocWdth(DocDisplayBox());
		uimgr->AdjustPixmap(rint(mag * docwdth), rint(mag * dochght));
		Repaint();
	}
	else {
		Repaint();
	}
	VIEWER::ShowPageInfo(0);
	VIEWER::ShowPrintInfo(0);
		
	DbgFunc("VIEWER::GotoPage: finished" << endl);

	return (STATUS_OK);
}

// Hide Links
void
PSVIEWER::HideLinks()
{
	DbgFunc("PSVIEWER::HideLinks: entered" << endl);
	if (psdoc->LinkList(currPage).Count() > 0) {
		uimgr->DrawLinkBoxes(psdoc->LinkList(currPage),
				     selected);
	}
	showLinks = BOOL_FALSE;
}

//Prepare to view a new document "name".
STATUS
PSVIEWER::LoadDocument(const DOCNAME	&docname,
		       const STRING	&filename,
		       const STRING	&pathlist,
		       ERRSTK		&err)
{
	// local variables
	PSDOC	*newdoc;
	STRING	footer_msg;

	DbgFunc("PSVIEWER::LoadDocument: entered" << endl);

	// Inform user
	notify->Busy(gettext("Loading document ..."));

	EVENTLOG2("LOAD_DOCUMENT", "doc=%s filename=%s", ~docname, ~filename);

	// Create, open new document object
	newdoc = new PSDOC(filename, pathlist);

	if (newdoc->Open(err) != STATUS_OK) {
		delete(newdoc);
		EVENTLOG("LOAD_DOCUMENT_FAILED");
		err.Push(gettext("Viewer can't display document"));
		notify->Done();
 		return (STATUS_FAILED);
	}

	resizeOnLoad	= (((DocHght(newdoc->DisplayBox()) !=
			     DocHght(DocDisplayBox())) ||
			    (DocWdth(newdoc->DisplayBox()) !=
			     DocWdth(DocDisplayBox()))) ?
			   BOOL_TRUE : BOOL_FALSE);

	// Save the current state of the viewer before deleting the
	// previous document
	//
	if (psdoc != NULL) {
		// Push the current document on the goback stack
		goback->Push(currDocName, GetDocName(), currPage);
		uimgr->GetInputFocus();
		delete(psdoc);
		uimgr->ResetPostScript();
	}

	// During program startup, we always want to resizeOnLoad so
	// that we don't attempt to directly repaint at a time when
	// the NeWS server might not be ready to handle it.
	//
	if ((resizeOnLoad == BOOL_FALSE) &&
	    ((int)xv_get(uimgr->FrameHandle(), XV_SHOW) == 0))
		resizeOnLoad = BOOL_TRUE;

	psdoc		= newdoc;
	currPage	= 0;
	sentProlog	= BOOL_FALSE;
	SetCurrDoc(docname);

	if (docfinder != NULL)
		docfinder->SeedDoc(docname);

	notify->Done();
	uimgr->SetInputFocus();

	// Set the mode line
	footer_msg = MakeModeLine();
	if (footer_msg == NULL_STRING)
		footer_msg = psdoc->GetDocTitle();
	notify->Mode(gettext("%s"), ~footer_msg);

	DbgFunc("PSVIEWER::LoadDocument: finished" << endl);
	return (STATUS_OK);
}

STATUS
PSVIEWER::Repaint()
{
	// Local variables
	int		nbytes;
	char	       *start;
	STATUS		status = STATUS_OK;
	
	DPSDbgFunc("Entered PSVIEWER::Repaint");
	DbgFunc("PSVIEWER::Repaint: entered ... page:\t" << currPage << endl);

	EVENTLOG("BEGIN_REPAINT_WINDOW");


	if (psdoc) {
		assert(psdoc->IsValid());
		assert(currPage > 0 && currPage <= GetNumPages());

		// If the prolog for this document has not been sent, send it
		if (sentProlog == BOOL_FALSE) {
		        DPSDbgFunc("PSVIEWER::Repaint. Prolog Not sent");

			if (psdoc->GetProlog(&start, &nbytes) != STATUS_OK) {
				DbgHigh("PSVIEWER::Repaint: no prolog" <<
					endl);

			}
			else {
			        DPSDbgFunc("PSVIEWER::Repaint. Prolog sent");
				DbgLow("PSVIEWER::Repaint: sending prolog" <<
				       endl);
				DPSDbgFunc("PSVIEWER::Repaint. Sending data");

				uimgr->SendData(start, nbytes);
			}

			sentProlog = BOOL_TRUE;
		}
	        DPSDbgFunc("PSVIEWER::Repaint. Clearing Page");
		uimgr->ClearPage();

		if (psdoc->GetPage(currPage,
				      &start, &nbytes) == STATUS_OK) {

			DbgMed("PSVIEWER::Repaint: sending page:\t" << currPage
			       << endl);
			DPSDbgFunc("PSVIEWER::Repaint. Displaying Page");
			status = uimgr->DisplayPage(mag, DocDisplayBox(),
						    start, nbytes);

			if (showLinks &&
			    psdoc->LinkList(currPage).Count() > 0) {
			        DPSDbgFunc("PSVIEWER::Repaint. Drawing Links");
				uimgr->DrawLinkBoxes(psdoc->LinkList(currPage),
						     selected);
			}

			// Draw selected link
			//
			if (LinkSelected()) {
				const PSLINK	&link =
					*psdoc->LinkList(currPage)[selected];
				DPSDbgFunc("PSVIEWER::Repaint. Drawing Selected link");

				uimgr->DrawSelectedLink(link,
							DocDisplayBox(),
							mag );
			}
			uimgr->UpdateCanvas();
		}
		else {
			DbgHigh("PSVIEWER::Repaint: \"" << currPage
				<< "\" no such page" << endl);
			status = STATUS_FAILED;
		}
	}
	else {	// Show a clear page
	        DPSDbgFunc("PSVIEWER::Repaint. Showing a clear page");
		const caddr_t clear_page_ps =
			(const caddr_t) ((const char *) clear_page);

		status = uimgr->DisplayPage(mag,
					    DocDisplayBox(),
					    clear_page_ps,
					    clear_page.Length());
		uimgr->UpdateCanvas();
	}
	
	EVENTLOG("END_REPAINT_WINDOW");
	DPSDbgFunc("Leaving PSVIEWER::Repaint");
	return (status);
}

STATUS
PSVIEWER::SetSelection(Event *const ev)
{
	DbgFunc("PSVIEWER::SetSelection at:\t(" << event_x(ev) << ", "
		<< event_y(ev) << ")" << endl);

	// If there is a selected link unhighlight it
	if (LinkSelected()) {
		const PSLINK	&link = *psdoc->LinkList(currPage)[selected];

		DbgLow("PSVIEWER::SetSelection: Found previous selected link"
		       << endl);
		// Remove the filled background behind the selected link
		uimgr->DrawSelectedLink(link, DocDisplayBox(), mag);

		// Put the outline back, if necessary
		if (showLinks) {
			uimgr->DrawLink(link, DocDisplayBox(), mag);
		}
		uimgr->EnableFollowLinkItem(BOOL_FALSE);
	}

	selected = GetSelectedLink(event_x(ev), event_y(ev));

	if (LinkSelected()) {
		const PSLINK	&link = *psdoc->LinkList(currPage)[selected];

		DbgLow("PSVIEWER::SetSelection: found selection" << endl);

		if (link.IsSingleClick()) {
			EVENTLOG("SINGLE_CLICK_SELECTION");
			ExecuteSelection();
		}
		else if (ds_is_double_click(&prevEvent, ev)) {
			EVENTLOG("DOUBLE_CLICK_SELECTION");
			ExecuteSelection();
		}
		else {
			EVENTLOG("SINGLE_CLICK_SELECTION");

			// Remove the outlined rect if necessary
			if (showLinks)
				uimgr->DrawLink(link, DocDisplayBox(), mag);

			// Highlight the selected link
			uimgr->DrawSelectedLink(link, DocDisplayBox(), mag);


			// Allow user to follow link from view menu
			uimgr->EnableFollowLinkItem(BOOL_TRUE);
		}

		prevEvent = *ev;
	}

	return(STATUS_OK);
}

// Show Links
void
PSVIEWER::ShowLinks()
{
	DbgFunc("PSVIEWER::ShowLinks: entered" << endl);
	showLinks = BOOL_TRUE;

	if (psdoc->LinkList(currPage).Count() > 0) {
		uimgr->DrawLinkBoxes(psdoc->LinkList(currPage),
				     selected);
	}
}

PSVIEWER::~PSVIEWER()
{
	DbgFunc("PSVIEWER::~PSVIEWER: entered" << endl);

	if (psdoc)
		delete(psdoc);
}
