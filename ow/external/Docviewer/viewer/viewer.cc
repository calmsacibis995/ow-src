#ident "@(#)viewer.cc	1.71 02/14/95 Copyright 1992 Sun Microsystems, Inc."


#include <sys/param.h>
#include <xview/defaults.h>
#include "viewer.h"
#include "docinfo.h"
#include <doc/abgroup.h>
#include <doc/abclient.h>
#include <doc/book.h>
#include <doc/cardcats.h>
#include <doc/docname.h>
#include <doc/document.h>
#include <doc/psdoc.h>
#include <xview/frame.h>
#include <xview/notice.h>
#include "dpsdebug.h"
#define	DV_DEFAULT_MAG_STR	"1.39"
#define	HV_DEFAULT_MAG_STR	"1.00"
#define	DV_DEFAULT_MAG		1.39
#define	HV_DEFAULT_MAG		1.00

#define	MAG_INCREMENT		0.10


Xv_opaque VIEWER::_dataKey	= 0;

extern DOCINFO	*docinfo;

static int	GetDocPageOffset(int view_page, const DOCUMENT *doc);

static DOCUMENT	*ResolveDocLink(const DOCNAME	&docname,
				ERRSTK		&err,
				int		recurse = 0);


static DOCUMENT *ResolveExternalLink(const DOCNAME &docname,
				      ABGROUP       *abGroup,
				      ERRSTK        &err);
// External Functions
extern "C" {
	void ds_expand_pathname(char *str, char *path);
};

VIEWER::VIEWER(const ViewerType typeArg) :
	docinfo(NULL),
#ifdef	LOG
	logger(NULL),
#endif	/* LOG */
	ttmgr(NULL),
	uimgr(NULL)
{
	DbgFunc("VIEWER::VIEWER: entered" << endl);

	vtype	= typeArg;
}

Notify_value
VIEWER::DestroyProc(const Notify_client	frame,
		   const Destroy_status	dstatus)
{
	Notify_value	nval = NOTIFY_DONE;
	VIEWER		*viewer;
	STRING		docname;
	char		*argv[12];
	int		argc;

	DbgFunc("VIEWER::DestroyProc: entered" << endl);

	viewer = (VIEWER *)xv_get(frame, XV_KEY_DATA, _dataKey);

	// If this process is really going to die, destroy the whole app.
	switch (dstatus) {

	// Process is dying - there's not much of anything we can do ...
	case DESTROY_PROCESS_DEATH:
		DbgFunc("DestroyProc: process death" << endl);
		break;

	// We've been asked to tidy up after ourselves ...
	case DESTROY_CLEANUP:
		DbgFunc("VIEWER::DestroyProc: cleaning up" << endl);
		if (viewer != NULL)
			delete viewer;
		nval = notify_next_destroy_func(frame, dstatus);
		break;

	// The user selected 'Save Workspace'
	case DESTROY_SAVE_YOURSELF:
		DbgFunc("DestroyProc: save yourself" << endl);

		// Docviewer is started by navigator, therefore
		// we don't save command state.  Helpviewer is
		// started via `helpviewer <filename>', so we
		// substitute argv[0] with `helpviewer'.
		//
		argc = 0;
		argv[0] = NULL;
		if (viewer->vtype == HELPVIEWER) {
			Rect	win_rect;
			Window	child;
			int	ixloc, iyloc;
			char	buf1[5], buf2[5], buf3[5], buf4[5];

			argv[argc++] = "helpviewer";

			// get frame position
			frame_get_rect(frame, &win_rect);
			argv[argc++] = "-Wp";
			sprintf(buf1, "%d", win_rect.r_left);
			argv[argc++] = buf1;
			sprintf(buf2, "%d", win_rect.r_top);
			argv[argc++] = buf2;

			// get icon position
			XTranslateCoordinates(
			    (Display *) xv_get(frame, XV_DISPLAY),
			    xv_get(xv_get(frame, FRAME_ICON), XV_XID),
			    xv_get(xv_get(xv_get(frame, FRAME_ICON), XV_ROOT), 
				XV_XID), 
			    0, 0, &ixloc, &iyloc, &child);
			argv[argc++] = "-WP";
			sprintf(buf3, "%d", ixloc); argv[argc++] = buf3;
			sprintf(buf4, "%d", iyloc); argv[argc++] = buf4;

			if (xv_get(frame, FRAME_CLOSED))
			    argv[argc++] = "-Wi";

			docname = viewer->GetDocName();
			if (docname != NULL_STRING) {

				// HACK!!  because "argv[i] = ~docname"
				// make compiler sick.
				//
				char filename[MAXPATHLEN];

				sprintf(filename, "%s", ~docname);
				argv[argc++] = filename;
			}

			argv[argc] = NULL;
		}
		XSetCommand((Display *)xv_get(frame, XV_DISPLAY),
			(Window)xv_get(frame, XV_XID), argv, argc);

		break;

	// Just checking - we're being given the chance to veto the destroy
	case DESTROY_CHECKING:
		DbgFunc("DestroyProc: just checking" << endl);
		break;

	default:
		assert(0);
	}

	return(nval);
}

void
VIEWER::EventProc(const ViewerEvent	event,
		  const void		*obj,
		  void			*cdata)
{
	DOCNAME		*whereami;
	DVLINK		*link;
	VIEWER		*viewer = (VIEWER *) cdata;
	ERRSTK		err;

	DbgFunc("VIEWER::EventProc" << endl);
	DPSDbgFunc("Entered VIEWER::EventProc");
	assert(viewer);

	switch(event) {

	case VE__CURRENT_DOC:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__CURRENT_DOC");	  
		whereami = ((DOCNAME *) obj);
		(void) viewer->WhereAmI(*whereami, err);
		break;

	case VE__CUSTOM_MAG:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__CUSTOM_MAG");	  
		viewer->uimgr->CustomMag(viewer->DocDisplayBox(),
					 ToPercent((float) viewer->mag));
		break;

	case VE__EXECUTE_LINK:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__EXECUTE_LINK");	  
		link = (DVLINK *) obj;
		if (viewer->ExecuteLink(*link, err)  !=  STATUS_OK) {
		      notify->Warning(gettext("Can't follow hypertext link"));
		}
		break;

	case VE__FIRST_PAGE:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__FIRST_PAGE");	  
		break;

	case VE__FOLLOW_LINK:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__FOLLOW_LINK");	  
		viewer->ExecuteSelection();
		break;

	case VE__GOBACK:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__GOBACK");	  
		viewer->GoBack();
		break;

	case VE__GOTO_PAGE:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__GOTO_PAGE");	  
		viewer->GotoPage(*((int *) obj));
		break;

	case VE__HIDE_LINKS:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__HIDE_LINKS");	  
		viewer->HideLinks();
		break;

	case VE__LARGER:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__LARGER");	  
		viewer->Larger();
		break;

	case VE__LAST_PAGE:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__LAST_PAGE");	  
		viewer->HideLinks();
		break;

	case VE__MAGNIFY:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__MAGNIFY");	  
		viewer->Magnify(*((float *)obj));
		break;

	case VE__NEXT_DOC:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__NEXT_DOC");	  
		viewer->GotoNextDoc();
		break;

	case VE__NEXT_PAGE:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__NEXT_PAGE");	  
		viewer->GotoNextPage();
		break;

	case VE__PAGE_INFO:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__PAGE_INFO");	  
		viewer->ShowPageInfo(1);
		break;

	case VE__PREV_DOC:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__PREV_DOC");	  
		viewer->GotoPrevDoc();
		break;

	case VE__PREV_PAGE:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__PREV_PAGE");	  
		viewer->GotoPrevPage();
		break;

	case VE__PRINT:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__PRINT");	  
		viewer->ShowPrintInfo(1);
		break;

	case VE__REDISPLAY:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__REDISPLAY");	  
		viewer->Repaint();
		viewer->uimgr->SetResizeEvent(VE__NULL_EVENT);
		break;

	case VE__RESIZE:
		viewer->ResizeProc();
		break;

	case VE__SELECT:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__SELECT");	  
		viewer->SetSelection((Event *const) obj);
		break;

	case VE__SHOW_LINKS:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__SHOW_LINKS");	  
		viewer->ShowLinks();
		break;

	case VE__SMALLER:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__SMALLER");	  
		viewer->Smaller();
		break;

	case VE__STD_SIZE:
	        DPSDbgFunc("VIEWER::EventProc. Event is VE__STD_SIZE");	  
		viewer->StdSize();
		break;

	default:
	        DPSDbgFunc("VIEWER::EventProc. Event is default event");	  
		break;
	}
	DPSDbgFunc("Leaving VIEWER::EventProc");
}

STATUS
VIEWER::ExecuteLink(const DVLINK &dvlink, ERRSTK &err)
{
	STRING		currDocDir;
	DOCNAME		docname;
	DOCNAME		context;
	DVLINK		file_link;
	STRING		docpath;
	STRING		view_file;
	DOCUMENT	*viewdoc;
	ABCLIENT	*answerbook;
	extern STATUS	System(const STRING &, const STRING &);
	STATUS		status = STATUS_FAILED;
	int		view_docpage;
	int		view_filepage;
	int		page_offset;
	STRING		syscmd;


	assert(objstate.IsReady());

	DbgFunc("VIEWER::ExecuteLink: cookie="
		<< dvlink.GetCookie() << endl);
        DPSDbgFunc("Entered VIEWER::ExecuteLink");	  
	// Execute link.
	//
	switch (dvlink.LinkType()) {

	case DVLINK_VIEWDOCUMENT:

		// Resolve document name specified in link.
		//
		dvlink.ViewDocument(docname, view_docpage);
		context = GetCurrDoc();
		if (!docname.IsValid() && !context.IsValid()) {
			err.Init(gettext("No context for resolving link '%s'"),
				~docname);
			return(STATUS_FAILED);
		}

		docname.Resolve(GetCurrDoc());

		// Get document object specified by this document link.
		//
		if ((viewdoc = ResolveDocLink(docname, err))  ==  NULL) {
			return(STATUS_FAILED);
		}

		// Document object's view method is a DVLINK
		// which contains the actual file we want to load.
		// We assume that 'ResolveDocLink()' has already verified
		// this to be a valid link.
		//
		file_link.SetCookie(viewdoc->ViewMethod());
		file_link.ViewFile(view_file, view_filepage);
		if (view_file == NULL_STRING) {
			err.Init(gettext("Can't view document: no file"));
			delete viewdoc;
			return(STATUS_FAILED);
		}

		// Get path for PostScript directory where view file resides.
		//
		answerbook = abgroup->GetAnswerBook(viewdoc->Name());
		assert(answerbook != NULL);
		docpath = answerbook->PSPath(viewdoc->Name());
		assert(docpath != NULL_STRING);

		// Load document specified in viewdoc's "view method".
		//
		if (LoadDocument(viewdoc->Name(), view_file, docpath, err)  !=  STATUS_OK) {
			delete viewdoc;
			return(STATUS_FAILED);
		}

		// Go to the specified page.
		//
		// Perform the viewing operation specified in the link.
		//
		page_offset = GetDocPageOffset(view_docpage, viewdoc);

		switch (view_filepage) {

		case VIEWPAGE_FIRST:
			status = GotoPage(1 + page_offset);
			break;
		case VIEWPAGE_LAST:
			status = GotoLastPage();
			break;
		case VIEWPAGE_NEXT:
			status = GotoNextPage();
			break;
		case VIEWPAGE_PREV:
			status = GotoPrevPage();
			break;
		case VIEWPAGE_INVALID:
			assert(0);
			break;
		default:
			status = GotoPage(view_filepage + page_offset);
			break;
		}

		delete viewdoc;
		break;

	case DVLINK_VIEWFILE:

		DbgLow("VIEWER::ExecuteLink: view file link" << endl);


		// If a file was explicitly specified in this link,
		// load it into the viewer.
		// Otherwise, the specified operation will be performed
		// on the current file.
		//
		dvlink.ViewFile(view_file, view_filepage);

		if (vtype == HELPVIEWER) {
			docpath = helppath;
		}
		else {
			docpath = NULL_STRING;
		}

		if (view_file != NULL_STRING  &&
		    LoadDocument(docname, view_file, docpath, err) != STATUS_OK) {
			return(STATUS_FAILED);
		}


		// Perform the viewing operation specified in the link.
		//
		switch (view_filepage) {

		case VIEWPAGE_FIRST:
			status = GotoFirstPage();
			break;
		case VIEWPAGE_LAST:
			status = GotoLastPage();
			break;
		case VIEWPAGE_NEXT:
			status = GotoNextPage();
			break;
		case VIEWPAGE_PREV:
			status = GotoPrevPage();
			break;
		case VIEWPAGE_INVALID:
			assert(0);
			break;
		default:
			status = GotoPage(view_filepage);
			break;
		}
		break;

	case DVLINK_SYSTEM:
		DbgLow("VIEWER::ExecuteLink: system link" << endl);
		GetCurrDocPath(currDocDir);
		dvlink.SystemCmd(syscmd);
		status = System(syscmd, currDocDir);
		break;

	default:
		assert(0);
		break;
	}
        DPSDbgFunc("Leaving VIEWER::ExecuteLink");	  
	if (status != STATUS_OK) {
		err.Init(gettext("Can't display document"));
		return(STATUS_FAILED);
	} else {
		return(STATUS_OK);
	}

}

STATUS
VIEWER::GoBack()
{
	DOCNAME	docname;
	STRING	path;
	int	page;
	ERRSTK	err;


        DPSDbgFunc("Entered VIEWER::GoBack");	  
	assert(goback != NULL);

	if ( !goback->IsEmpty() ) {
		EVENTLOG("GOBACK");

		// Pop top document off the history stack.
		//
		goback->Pop(docname, path, page);

		// Load document
		//
		if (LoadDocument(docname, path, NULL_STRING, err) == STATUS_OK
		    &&  GotoPage(page) == STATUS_OK) {

			// Pop off the stuff that was pushed on by LoadDocument()
			// or GotoPage() above
			//
			goback->Pop(docname, path, page);

			if (goback->IsEmpty())
				uimgr->DisableGoBack(BOOL_TRUE);

			return(STATUS_OK);

		} else {
			DbgHigh("VIEWER::GoBack: error executing goback item"
				<< endl);
			return(STATUS_FAILED);
		}
	}

}

STATUS
VIEWER::GotoNextPage()
{
	DOCNAME		docname;
	DVLINK		next;
	STRING		link;
	STATUS		status = STATUS_FAILED;
	ERRSTK		err;

        DPSDbgFunc("Entered VIEWER::GotoNextPage");	  
	EVENTLOG("NEXT_PAGE");


	if (currPage < GetNumPages())
		return(GotoPage(currPage + 1));


	/*
	 * This is the last page in the file.
	 * Load the next file by creating and executing
	 * a "document" hypertext link.
	 */
	if (docfinder != NULL  &&
	    docfinder->GetNextFile(docname, err)  ==  STATUS_OK) {

		link = MakeViewDocumentLink(docname);
		if (next.SetCookie(link)  ==  STATUS_OK)
			status = ExecuteLink(next, err);
	}
		
	if (status != STATUS_OK)
		notify->Warning(gettext("End of book, cannot turn page"));
	else {
		ShowPageInfo(0);
		ShowPrintInfo(0);
	}
        DPSDbgFunc("Leaving VIEWER::GotoNextPage");	  
	return(status);
}

STATUS
VIEWER::GotoPrevPage()
{
	DOCNAME		docname;
	DVLINK		prev;
	STRING		link;
	STATUS		status = STATUS_FAILED;
	ERRSTK		err;

        DPSDbgFunc("Entered VIEWER::GotoPrevPage");	  
	EVENTLOG("PREV_PAGE");

	if (currPage > 1)
		return(GotoPage(currPage - 1));

	/*
	 * This is the last page in the file.
	 * Load the prev file by creating and executing
	 * a "document" hypertext link.
	 */
	if (docfinder != NULL  &&
	    docfinder->GetPrevFile(docname, err)  ==  STATUS_OK) {

		link = MakeViewDocumentLink(docname, VIEWPAGE_LAST);
		if (prev.SetCookie(link)  ==  STATUS_OK)
			status = ExecuteLink(prev, err);
	}
		
	if (status != STATUS_OK)
		notify->Warning(gettext("End of book, cannot turn page"));
	else {
		ShowPageInfo(0);
		ShowPrintInfo(0);
	}
        DPSDbgFunc("Leaving VIEWER::GotoPrevPage");	  
	return(status);
}

STATUS
VIEWER::GotoNextDoc()
{
	DOCNAME		docname;
	DVLINK		next;
	STRING		link;
	STATUS		status = STATUS_FAILED;
	ERRSTK		err;

        DPSDbgFunc("Entered VIEWER::GotoNextDoc");	  
	EVENTLOG("NEXT_DOC");


	/*
	 * If there is no next file,
	 * just go to the last page of the current file.
	 */
	if (!docfinder ||
	    (docfinder->GetNextFile(docname, err))  !=  STATUS_OK) {
		if (currPage < GetNumPages())
			return(GotoLastPage());
		return(STATUS_OK);
	}

	/*
	 * This is the last page in the file.
	 * Load the next file by creating and executing
	 * a "document" hypertext link.
	 */
	link = MakeViewDocumentLink(docname);

	if (next.SetCookie(link) != STATUS_OK  ||
	    ExecuteLink(next, err)    !=  STATUS_OK) {
		notify->Warning(gettext("Unable to go to next document"));
		return(STATUS_FAILED);
	}
        DPSDbgFunc("Leaving VIEWER::GotoNextDoc");	  
	return(STATUS_OK);
}

STATUS
VIEWER::GotoPrevDoc()
{
	DOCNAME		docname;
	DVLINK		prev;
	STRING		link;
	ERRSTK		err;

        DPSDbgFunc("Entered VIEWER::GotoPrevDoc");	  
	EVENTLOG("PREV_DOC");


	/*
	 * If we're not on the first page of this document,
	 * just go there.
	 */
	if (currPage > 1)
		return(GotoFirstPage());


	/*
	 * Get the preceding file.
	 */
	if (docfinder == NULL  ||
	    docfinder->GetPrevFile(docname, err)  !=  STATUS_OK) {
		notify->Warning(gettext("Beginning of document"));
		return(STATUS_FAILED);
	}


	/*
	 * Load the preceding file by creating and executing
	 * a "document" hypertext link.
	 */
	link = MakeViewDocumentLink(docname);

	if (prev.SetCookie(link) != STATUS_OK  ||
	    ExecuteLink(prev, err)    !=  STATUS_OK) {
		notify->Warning(gettext("Unable to go to previous document"));
		return(STATUS_FAILED);
	}
        DPSDbgFunc("Leaving VIEWER::GotoPrevDoc");	  
	return(STATUS_OK);
}

STATUS
VIEWER::GetDocOnPage(int page, DOCNAME &docname, ERRSTK &err)
{

	DbgFunc("VIEWER::GetDocOnPage: " << page << endl);

	if (docfinder)
		return(docfinder->GetDocOnPage(page, docname, err));
	else
		return(STATUS_FAILED);
}

STATUS
VIEWER::Init(int *argcPtr, char **argv, ERRSTK &err)
{
	STRING		card_catalog = NULL_STRING;
	STRING		linkText = NULL_STRING;
	STRING		preferred_language = NULL_STRING;
	Xv_opaque	frame;

	DbgFunc("VIEWER::Init: entered" << endl);
	assert(objstate.IsNotReady());

	objstate.MarkGettingReady();

	argv_procid = NULL_STRING;
	argv_docname = NULL_STRING;
	argv_viewfile = NULL_STRING;

#ifdef	LOG
	// If $DVLOGFILE is set, initialize event logging mechanism.
	//

	STRING		logfile = getenv("DVLOGFILE");

	if (logfile != NULL) {
		logger = new LOGGER(logfile);

		if (logger->Init() != STATUS_OK) {
			delete		logger;

			logger = NULL;
		}
	}
#endif

	// NOTE - the order of calls is important!
	//

	// Intialize the deskset tooltalk interface
	//
	ttmgr = new TTMGR();

	if (InitUI(argcPtr, argv, vtype, preferred_language, card_catalog, err) != STATUS_OK) {
		return(STATUS_FAILED);
	}

	frame = uimgr->FrameHandle();
	if (ttmgr->Init(*argcPtr, argv, err)  !=  STATUS_OK  ||
	    ttmgr->EnableMessageAllianceProtocol(frame, err)  !=  STATUS_OK) {
		err.Push(gettext("Could not start Viewer"));
		notify->ShowErrs(err);
		return(STATUS_FAILED);
	}


	objstate.MarkReady();
	
	if (argv_docname != NULL_STRING)
		linkText = argv_docname;
	else if (argv_viewfile != NULL_STRING)
		linkText = argv_viewfile;

	if (linkText != NULL_STRING  &&
	    Start(linkText, preferred_language, card_catalog, err)!=STATUS_OK){
		err.Push(gettext("Could not start Viewer"));
		notify->ShowErrs(err);
		return(STATUS_FAILED);
	}


	return(STATUS_OK);
}

// This function is used to display error messages that occur
// prior to the base frame being created.
// In this case a dummy base frame is created for the sake of
// displaying a popup with an error message.
//
void
VIEWER::DisplayError(ERRSTK &err)
{
    Event event;
    Frame base_frame;
    const char* errlist[20];
    int i;

    for (i = 0; i < 19 && ! err.IsEmpty(); i++) {
	errlist[i] = err.Pop();
    }
    errlist[i] = NULL;
    
    if (i != 0) {
    	base_frame = uimgr->FrameHandle();
	if (base_frame == XV_NULL) {
	    base_frame = xv_create( XV_NULL,FRAME,
		FRAME_LABEL,		gettext("docviewer"),
		FRAME_NO_CONFIRM,	TRUE,
	    );
	}

    	notice_prompt(base_frame, &event,
	    NOTICE_MESSAGE_STRINGS_ARRAY_PTR,       errlist,
            NOTICE_BUTTON_YES,      gettext("Continue"),
            NULL);
  }

  return;
}


STATUS
VIEWER::InitUI(int		*argcPtr,
	       char		**argv,
	       const ViewerType	vtype,
	       STRING		&preferred_language,
	       STRING		&card_catalog,
	       ERRSTK		&err)
{
	STRING		newshost;
	extern void	Usage();

	DbgFunc("VIEWER:InitUI:" << endl);
	// Initialize the UI

	uimgr = new UIMGR();

	uimgr->ParseArgs(argcPtr, argv);

	if (ParseArgs(*argcPtr, argv,
				newshost, preferred_language, card_catalog,
				err) != STATUS_OK) {
		DbgHigh("VIEWER::InitUI: error parsing options" << endl);
		Usage();
		*argcPtr = -1;
		return(STATUS_FAILED);
	}

	if (uimgr->Init(vtype, newshost, err) != STATUS_OK) {
	    err.Push(gettext("Could not start docviewer."));
	    DisplayError(err);
	    return(STATUS_FAILED);
	}

	const Frame	frame = uimgr->FrameHandle();

	uimgr->SetEventProc((const ViewerEventProc) EventProc,
			    (const void *) this);

	_dataKey = xv_unique_key();

	(void) xv_set(frame,
		      XV_KEY_DATA, _dataKey, (caddr_t) this,
		      XV_NULL);

	// Set the command line arguments
	//
	if (newshost != NULL_STRING) {
		(void) xv_set(frame,
			      FRAME_WM_COMMAND_STRINGS,
			      "-r", ~newshost, XV_NULL,
			      XV_NULL);
	}

	// Initialize the help path for the appropriate locale
	//
	SetHelpPath();

	notify = new NOTIFY(frame);

	goback = new HIST_STACK(42);

	if (vtype == DOCVIEWER) {
		docfinder = new DOCFINDER();
	}

	// Install destruction interposer
	notify_interpose_destroy_func(frame, (Notify_func)DestroyProc);

	// Handle common signals
	//
	(void) notify_set_signal_func(frame, (Notify_func) SignalProc,
				      SIGHUP, NOTIFY_SYNC);
	(void) notify_set_signal_func(frame, (Notify_func) SignalProc,
				      SIGINT, NOTIFY_SYNC);
	(void) notify_set_signal_func(frame, (Notify_func) SignalProc,
				      SIGTERM, NOTIFY_SYNC);

	return(STATUS_OK);
}

STATUS
VIEWER::Magnify(const float magArg)
{
	const int hght = (int) rint(magArg * DocHght(DocDisplayBox()));
	const int wdth = (int) rint(magArg * DocWdth(DocDisplayBox()));

	DbgFunc("VIEWER::Magnify: magVal:\t" << magArg << endl);

	assert(objstate.IsReady());

	if (wdth >= uimgr->MinWidth()) {
		mag = magArg;
		uimgr->SetResizeEvent(VE__MAGNIFY);
		ResizeProc();
	}
	else {
		notify->Warning(gettext("Document at minimum size; cannot shrink viewer window"));
	}
	
	return(STATUS_OK);
}

const STRING &
VIEWER::MakeModeLine()
{
	DOCUMENT       *bookObj = NULL;
	DOCUMENT       *docObj = NULL;
	DOCNAME		docname;
	BOOKNAME	bookname;
	ABNAME		abname;
	ABCLIENT	*answerbook;
	STRING		bookTitle;
	STRING		docTitle;
	STRING		abTitle;
	ERRSTK		err;

	/*
	 * Try to generate the title of the current document. This will be
	 * used in the mode line of the frame.
	 */

	// Get the title of the LDA object via the docname
	if (currDocName.IsValid()  &&  abgroup != NULL) {
		docname = currDocName;
		docname.SetDocId(currDocName.BookId());

		bookname = docname.BookName();
		abname = bookname.ABName();
		answerbook = abgroup->GetAnswerBook(abname);
		if (answerbook != NULL)
			abTitle = answerbook->Title();

		if ((bookObj = abgroup->LookUpDoc(docname,0,err))  !=  NULL) {
			bookTitle = bookObj->Title();
			delete(bookObj);
		}

		if ((docObj = abgroup->LookUpDoc(currDocName,0,err)) != NULL) {
			docTitle = docObj->Title();
			delete(docObj);
		}

		if (bookTitle != NULL_STRING) {
			modeLineStr = bookTitle;
			if (abTitle != "")
				modeLineStr += " (" + abTitle + ")";
			return (modeLineStr);
		}
	}

	// If LDA didn't have it return NULL_STRING
	modeLineStr = NULL_STRING;

	return (modeLineStr);
}

TtReplyStatus
VIEWER::MessageProc(void *clientData, const Tt_message msg)
{
	ERRSTK		err;
	int		loop;
	int		message_value;
	STRING		msgOp   = tt_message_op(msg);
	VIEWER		*viewer	= (VIEWER *) clientData;
	TtReplyStatus	rstatus	= RS_NO_MATCH;
	TTMGR		*ttmgr	= viewer->ttmgr;
	Frame		frame	= viewer->uimgr->FrameHandle();
	STRING		failed_string = gettext("Can't display document");
	STATUS		status;
	Tt_status	ttstatus;

	assert(viewer->objstate.IsReady());
	DbgFunc("VIEWER::MessageProc: entered - msg = " << msg << ")" << endl);

	if (msgOp == VIEWER_TT_GET_CURRENT_DOCUMENT) {
		DOCNAME whereami;

		status = viewer->WhereAmI(whereami, err);

		if (status != STATUS_OK) {
		    whereami = NULL_STRING;
		}

		status = ttmgr->CreateWhereAmIReply(msg, whereami, err);

		if (status == STATUS_OK) {
			rstatus = RS_REPLY;
		}
		else {
			rstatus = RS_REJECT;
			DbgHigh("VIEWER::MessageProc: Failing reply "
				"to whereami message - \"" <<
				err << "\"" << endl);
		}
	} 

	else if (msgOp == VIEWER_TT_GET_CURRENT_FILE) {

		status = ttmgr->CreateWhatFileReply(msg, viewer->GetDocName(),
						    viewer->GetCurrPage(), err);

		if (status == STATUS_OK) {
			rstatus = RS_REPLY;
		}
		else {
			rstatus = RS_REJECT;
			DbgHigh("VIEWER::MessageProc: Failing reply "
				"to get_current_file message - \"" <<
				err << "\"" << endl);
		}
	} 

	else if (msgOp == VIEWER_TT_PING) {
		STRING	hostname = tt_message_arg_val(msg, 0);
		STRING	display = tt_message_arg_val(msg, 1);
		STRING	viewer_name = tt_message_arg_val(msg, 2);

		status = ttmgr->CreatePingReply(msg, hostname, display,
						viewer_name, err);

		if (status == STATUS_OK) {
			rstatus = RS_REPLY;
		}
		else {
			rstatus = RS_REJECT;
			DbgHigh("VIEWER::MessageProc: Failing reply "
				"to Ping message - \"" <<
				err << "\"" << endl);
		}
	} 
	else if (msgOp == VIEWER_TT_VIEW_DOCUMENT) {
		STRING	msgval = tt_message_arg_val(msg, 0);
		DOCNAME	docname;
		DVLINK	link;

		if (docname.Init(msgval)           == STATUS_OK  &&
		    link.SetCookie(msgval)         == STATUS_OK  &&
		    viewer->ExecuteLink(link, err) == STATUS_OK) {

			(void) tt_message_arg_val_set(msg, 1, DV_TT_STATUS_OK);
			rstatus = RS_REPLY;
		}
		else {
			(void) tt_message_arg_val_set(msg, 1, failed_string);
			rstatus = RS_FAIL;
			DbgHigh("VIEWER::MessageProc: " <<
				"Failing reply to view_document " <<
				"message - \"" << err << "\"" <<
				endl);
		}
	}
	else if (msgOp == VIEWER_TT_VIEW_FILE) {
		STRING	msgval = tt_message_arg_val(msg, 0);
		DVLINK	link;

		if (link.SetCookie(msgval)         == STATUS_OK  &&
		    viewer->ExecuteLink(link, err) == STATUS_OK) {

			rstatus = RS_REPLY;
		}
		else {
			rstatus = RS_FAIL;
			DbgHigh("VIEWER::MessageProc: " <<
				"Failing reply to view_file " <<
				"message - \"" << err << "\"" <<
				endl);
		}
	}
	else if (msgOp == VIEWER_TT_SET_PREFERRED_LANGUAGE) {
		STRING	preferred_language = tt_message_arg_val(msg, 0);

		assert(abgroup != NULL);
		abgroup->SetPreferredLang(preferred_language);
		rstatus = RS_REPLY;
	}
	else if (msgOp == VIEWER_TT_SET_CARD_CATALOGS) {
		STRING	card_catalog = tt_message_arg_val(msg, 0);

		assert(abgroup != NULL);
		abgroup->GetCardCatalogs().Clear();
		abgroup->GetCardCatalogs().Append(card_catalog, err);
		rstatus = RS_REPLY;
	}
	else if (msgOp == VIEWER_TT_NEXT_PAGE) {
		ttstatus = tt_message_arg_ival(msg, 0, &message_value);

		if (ttstatus != TT_OK) {
			message_value = 1;
		}

		for (loop = 1;  loop <= message_value;  loop++) {
			status = viewer->GotoNextPage();
		}

		if (status == STATUS_OK) {
			rstatus = RS_REPLY;
		}
		else {
			rstatus = RS_FAIL;
		}
	}
	else if (msgOp == VIEWER_TT_PREVIOUS_PAGE) {
		ttstatus = tt_message_arg_ival(msg, 0, &message_value);

		if (ttstatus != TT_OK) {
			message_value = 1;
		}

		for (loop = 1;  loop <= message_value;  loop++) {
			status = viewer->GotoPrevPage();
		}

		if (status == STATUS_OK) {
			rstatus = RS_REPLY;
		}
		else {
			rstatus = RS_FAIL;
		}
	}
	else if (msgOp == VIEWER_TT_GO_BACK) {
		ttstatus = tt_message_arg_ival(msg, 0, &message_value);

		if (ttstatus != TT_OK) {
			message_value = 1;
		}

		for (loop = 1;  loop <= message_value;  loop++) {
			status = viewer->GoBack();
		}

		if (status == STATUS_OK) {
			rstatus = RS_REPLY;
		}
		else {
			rstatus = RS_FAIL;
		}
	}
	else if (msgOp == VIEWER_TT_CUSTOM_MAGNIFY) {
		ttstatus = tt_message_arg_ival(msg, 0, &message_value);

		if (ttstatus == TT_OK) {
			viewer->Magnify((float) message_value/100.0);
			rstatus = RS_REPLY;
		}
		else {
			rstatus = RS_FAIL;
		}
	}
	else if (msgOp == DOCVIEWER_TT_DEPARTING) {
		DbgHigh("VIEWER::MessageProc: received " <<
			"DS_TT_DEPARTING message from our owner " <<
			"DIVE! DIVE! DIVE!"
			<< endl);
		xv_destroy_safe(frame);
	}

	return (rstatus);
}

void
VIEWER::Larger()
{
	DbgFunc("VIEWER::Larger: entered" << endl);

	EVENTLOG2("LARGER",
		  "Current Magnification = %7.4f, MAG_INCREMENT = %f",
		  (float) mag, MAG_INCREMENT);
	Magnify((float) mag + MAG_INCREMENT);
	DbgFunc("VIEWER::Larger: finished" << endl);
}

STATUS
VIEWER::ParseArgs(const int	argc,
		  char *const	*argv,
		  STRING	&newshost,
	          STRING	&preferred_language,
	          STRING	&card_catalog,
		  ERRSTK	&err)
{
	int		opt;
	STRING		progname;
	int		slash;
	STATUS		status = STATUS_FAILED;

	DbgFunc("VIEWER::ParseArgs:" << endl);
	progname = argv[0];

	// Get the basename of argv[0]
	//
	if ((slash = progname.RightIndex('/')) >= 0) {
		progname = progname.SubString(slash + 1, END_OF_STRING);
	}

	while ((opt = getopt(argc, argv, "p:d:f:l:c:r:t:x:")) != EOF) {

		switch (opt) {

		case 'p':
			argv_procid = optarg;
			break;

		case 'd':
			argv_docname = optarg;
			status = STATUS_OK;
			break;

		case 'f':
			argv_viewfile = optarg;
			status = STATUS_OK;
			break;

		case 'l':
			preferred_language = optarg;
			break;

		case 'c':
			card_catalog = optarg;
			break;

		case 'r':
			newshost = optarg;
			break;
#ifdef	DEBUG
		case 'x':
			debug = atoi(optarg);
			break;
#endif	DEBUG
		default:
			err.Init(gettext("Unknown option\"-%c\""),
				 (char) opt);
			return(STATUS_FAILED);
		}
	}

	if (optind == argc - 1) {

		if (progname == "helpviewer") {
			argv_viewfile = argv[ optind ];
		}
		else {
			argv_docname = argv[ optind ];
		}

		status = STATUS_OK;
	}

	if (!(const char *) newshost) {
		newshost = defaults_get_string("viewer.newshost",
					       "Viewer.NeWSHost",
					       NULL);
	}

	return( status );
}

void
VIEWER::ResizeProc()
{
        DPSDbgFunc("Entered VIEWER::ResizeProc");	  
	DbgFunc("VIEWER::ResizeProc: entered" << endl);
	const dochght		= DocHght(DocDisplayBox());
	const docwdth		= DocWdth(DocDisplayBox());
	static int firsttime	= 1;

	switch (uimgr->GetResizeEvent()) {

	case VE__MAGNIFY:	// Magnify
        DPSDbgFunc("Leaving VIEWER::ResizeProc. Event is VE__MAGNIFY");	  

		uimgr->ResetScrollbars();
		uimgr->SetSize((int) rint(docwdth * mag), 
		               (int) rint(dochght * mag));
		uimgr->AdjustPixmap((int) rint(docwdth * mag), 
		                    (int) rint(dochght * mag));
		Repaint();
		uimgr->SetResizeEvent(VE__RESIZE);
		break;

	case VE__RESIZE:	// window stretch
        DPSDbgFunc("Leaving VIEWER::ResizeProc. Event is VE__RESIZE");	  
		uimgr->SetSize((int) rint(docwdth * mag),
			       (int) rint(dochght * mag));
		uimgr->CheckScrollbars ();
		if (firsttime){ 
			Repaint();
			firsttime = 0;
		}
		else
			uimgr->UpdateCanvas();
		break;

	default:
                DPSDbgFunc("Leaving VIEWER::ResizeProc. Event is default event");	  
		break;
	}

        DPSDbgFunc("Leaving VIEWER::ResizeProc");	  
	return;
}


void
VIEWER::ShowPrintInfo(int force)
{
	Xv_opaque	print_frame;
	DOCNAME		whereami;
	ERRSTK		err;

	DbgFunc("VIEWMGR::ShowPrintInfo: entered\n");

	// ShowPrintInfo may be called even though the popup window
	// isn't already displayed.  If it isn't visible and the
	// user didn't request it, then don't do anything.
	//
	print_frame = uimgr->PrintwinHandle();
	if (!force && ((print_frame == NULL) ||
		       (!(int)xv_get(print_frame, XV_SHOW))))
		return;

	if (print_frame == NULL)
	   uimgr->CreatePrintWin ();

	if (GetDocOnPage(currPage, whereami, err) != STATUS_OK) {
		notify->Warning(gettext("Can't find page info"));
		return;
	}
	assert(whereami.IsValid());

	if (uimgr->PrintDocument(whereami, currPage, err) != STATUS_OK)
		notify->Warning(gettext("Can't find document info"));
}

void
VIEWER::ShowPageInfo(int force)
{
	DOCNAME	whereami;
	ERRSTK	err;


	DbgFunc("VIEWMGR::ShowPageInfo: entered\n");

	if (docinfo == NULL) {

		// ShowPageInfo may be called anytime programatically,
		// without user request.  If so, then don't do anything.
		//
		if (!force)
			return;

		if (abgroup) {
			docinfo = new DOCINFO(uimgr->FrameHandle());
		}
		else {
			notify->Warning(gettext("Can't display page info"));
			return;
		}
	} else {
		// ShowPageInfo may be called even though the popup window
		// isn't already displayed.  If it isn't visible and the
		// user didn't request it, then don't do anything.
		//
		if (!force && !(int)xv_get(docinfo->FrameHandle(), XV_SHOW))
			return;
	}

	if (GetDocOnPage(currPage, whereami, err) != STATUS_OK) {
		notify->Warning(gettext("Can't find page info"));
		return;
	}
	assert(whereami.IsValid());


	DbgMed("VIEWMGR::ShowPageInfo:"
			<< " currpage="	<< currPage
			<< " whereami="	<< whereami
			<< endl);

	if (docinfo->Show(whereami)  !=  STATUS_OK)
		notify->Warning(gettext("Can't display page info"));
}

Notify_value
VIEWER::SignalProc(const Notify_client	client,
#ifdef	DEBUG
		   const int		sig,
#else
		   const int		/* sig */,
#endif	/* DEBUG */
		   const Notify_signal_mode /* when */)
{
	DbgFunc("VIEWER::SignalProc: signal = " << sig << endl);
	xv_destroy_safe((Frame) client);
	return(NOTIFY_DONE);
}

void
VIEWER::Smaller()
{
	DbgFunc("VIEWER::Smaller: entered" << endl);

	EVENTLOG2("SMALLER",
		  "Current Magnification = %7.4f, MAG_INCREMENT = %f",
		  (float) mag, MAG_INCREMENT);

	Magnify((float) mag - MAG_INCREMENT);

	DbgFunc("VIEWER::Smaller: finished" << endl);
}

STATUS
VIEWER::Start(const STRING &linktext,
	      const STRING &preferred_language,
	      const STRING &card_catalog,
	      ERRSTK &err)
{
	DVLINK		link;
	STATUS		status = STATUS_OK;
	STRING		our_procid;
	STRING		status_string;

	// Externals
	//
	extern ABGROUP	*abgroup;

	DbgFunc("VIEWER::Start: entered" << endl);

	// Initialize AnswerBook client-side interface.
	//
	if (vtype == DOCVIEWER) {

		// Initialize card catalog list.
		// This list is used to locate AnswerBooks.
		//
		CARDCATS *cardcats = new CARDCATS();

		(void) cardcats->Append(card_catalog, err);
		(void) cardcats->AppendDefaults(err);

		abgroup = new ABGROUP(*cardcats);

		if (preferred_language != NULL_STRING) {
			abgroup->SetPreferredLang(preferred_language);
		}
	}

	if (linktext != NULL_STRING) {

		(void) xv_set(uimgr->FrameHandle(),
			      FRAME_WM_COMMAND_STRINGS,
			      ~linktext, XV_NULL,
			      XV_NULL);

		if ((status = link.SetCookie(linktext)) != STATUS_OK) {
			err.Init(gettext("Could not parse link"));
			status_string = gettext("Could not parse link");

		} else if ((status = ExecuteLink(link, err)) != STATUS_OK) {
		       status_string = gettext("Can't display document");
		}
	}

	if (status == STATUS_OK) {
		ttmgr->SetEventProcs((TtMesgProc) VIEWER::MessageProc,
				     (TtReplyProc) NULL,
				     (void *) this);
	}

	// If started with a -p procid we must send the navigator a ToolTalk
	// message, identifying ourselves as its' viewer
	if (argv_procid != NULL_STRING) {
		our_procid = tt_default_procid();
		(void) ttmgr->SendIAmYourViewerMsg(argv_procid,
						   status_string,
						   our_procid,
						   err);
	}

	if (status == STATUS_OK) {
		mag.Reset(vtype);

		defaultMag = (float) mag;

		// If the user specified the window size via -Ws,
		// don't do anything. Otherwise, fit the frame to the
		// canvas
		//
		if (!UserSetSize()) {
			uimgr->FitFrame(mag, DocDisplayBox());
		}

		xv_main_loop(uimgr->FrameHandle());
	}

	return(status);
}

void
VIEWER::StdSize()
{
	DbgFunc("VIEWER::StdSize: entered" << endl);

	EVENTLOG("RESET_TO_STD_SIZE");
	Magnify(defaultMag);
}

STATUS
VIEWER::WhereAmI(DOCNAME &whereami, ERRSTK &err)
{
	DbgFunc("VIEWER::WhereAmI: entered" << endl);

	return(GetDocOnPage(currPage, whereami, err));
}

VIEWER::~VIEWER()
{
	DbgFunc("VIEWER::~VIEWER: entered" << endl);

	if (ttmgr != NULL) {
		delete ttmgr;
	}
#ifdef	LOG
	if (logger != NULL) {
		delete logger;
	}
#endif	LOG

	if (uimgr != NULL) {
		delete uimgr;
	}
	if (docfinder)
		delete(docfinder);

	if (docinfo)
		delete(docinfo);
	
	if (goback)
		delete (goback);
}


DOCUMENT *
ResolveExternalLink(const DOCNAME &docname,
		    ABGROUP       *abGroup,
	            ERRSTK        &err)
{
  DOCUMENT* externalDoc = NULL;
  LISTX<ABINFO*> abInfoList;

  CARDCATS& cardCats  = abGroup->GetCardCatalogs();

  cardCats.GetAll(abInfoList, err);
  int numAnswerbooks = abInfoList.Count();
  
  for (int i=0; i< numAnswerbooks; i++) {

	    DOCNAME tmpDocName = docname;
    
	    tmpDocName.SetABId(abInfoList[i]->Name().ABId());
	    tmpDocName.SetABVersion(abInfoList[i]->Name().ABVersion());

	    externalDoc = abGroup->LookUpDoc(tmpDocName, LU_PREFERRED_LANG|LU_AUTO_ADD , err);

	    if (externalDoc != NULL) {
	      break;
	    }
  }

  return externalDoc;
}



DOCUMENT *
ResolveDocLink(const DOCNAME &docname, ERRSTK &err, int recurse)
{
	DVLINK		dvlink;
	DOCUMENT	*doc;
	DOCNAME		tmpname;
	u_long		flags;
	int		junk;


	assert(docname.IsValid());
	DbgFunc("ResolveDocLink: " << docname << endl);


	if (recurse > 7) {
		err.Init(gettext("Hypertext link loop detected: '%s'"),
			~docname);
		return(NULL);
	}


	if (abgroup == NULL) {
		err.Init(gettext("No current AnswerBooks"));
		return(NULL);
	}


	// Retrieve named document from AnswerBook database.
	//
	flags = LU_PREFERRED_LANG|LU_AUTO_ADD;
	if ((doc = abgroup->LookUpDoc(docname, flags, err))  ==  NULL) {

	        // May be the docname is a hypertext link to another answerbook:

	        doc = ResolveExternalLink(docname,abgroup, err);
		
		if (doc == NULL) {
		  err.Init(gettext("Can't find document"));
		  return(NULL);
		}
	}


	// Get document's view method, which should be a
	// 'view file' link.
	//
	if (doc->ViewMethod() == NULL_STRING) {
		err.Init(gettext("Can't view document '%s' (no view method)"),
				~doc->Title());
		delete doc;
		return(NULL);
	}


	// Make link object's view method the new link cookie.
	//
	if (dvlink.SetCookie(doc->ViewMethod())  !=  STATUS_OK) {
		err.Init(gettext("Can't view document '%s' (bad view method)"),
				~doc->Title());
		delete doc;
		return(NULL);
	}

	// If view method is another "view document" link,
	// call ourselves recursively to resolve it.
	//
	if (dvlink.LinkType() == DVLINK_VIEWDOCUMENT) {
		dvlink.ViewDocument(tmpname, junk);
		tmpname.Resolve(doc->Name());
		delete doc;
		doc = ResolveDocLink(tmpname, err, ++recurse);
	}

	return(doc);
}


int
GetDocPageOffset(int view_page, const DOCUMENT *doc)
{
	int	page_offset;
	int	page_count;


	assert(doc != NULL);
	DbgFunc("GetDocPageOffset" << endl);


	// Determine how many pages there are in this object.
	// We'll use this info in just a minute.
	//
	if (doc->Range().IsValid())
		page_count = doc->Range().NumPages();
	else
		page_count = 1;


	// Convert the link argument into an offset from the
	// first page of the object.
	// Only certain arguments make sense:
	//	last
	//	<page number>
	// We ignore the rest:
	//	first		(it's redundant)
	//	next		(next object maybe?  Is this useful?)
	//	prev		(previous object maybe?  Is this useful?)
	//
	switch (view_page) {

	case VIEWPAGE_INVALID:
	case VIEWPAGE_FIRST:
	case VIEWPAGE_NEXT:
	case VIEWPAGE_PREV:
		page_offset = 0;
		break;

	case VIEWPAGE_LAST:
		page_offset = page_count - 1;
		break;

	default:
		page_offset = doc->Name().Offset();
		if (page_offset < 0)
			page_offset = 0;
		else if (page_offset >= page_count)
			page_offset = page_count - 1;
		break;
	}


	return(page_offset);
}


void
VIEWER::GetCurrDocPath(STRING &currDocDir)
{
	STRING	path;
	int	slash;

	path = GetDocName();

	slash = path.RightIndex('/');

	if (slash < 0)
		currDocDir = path;
	else if (slash == 0)
		currDocDir = NULL_STRING;
	else
		currDocDir = path.SubString(0, slash-1);
}

void
VIEWER::SetHelpPath()
{
	const STRING	env = getenv("HELPPATH");
	char		localeDir[MAXPATHLEN];

	DbgFunc("VIEWER::SetHelpPath:" << endl);

	// Get the locale directory associated with this
	// language (specified by the attribute XV_LC_DISPLAY_LANG),
	// and prepend it to helppath
	//
	ds_expand_pathname("$OPENWINHOME/lib/locale/", localeDir);
	helppath = localeDir;
	helppath += (const char *) xv_get(xv_default_server,
					  XV_LC_DISPLAY_LANG);
	helppath += "/help:";

	if ((const char *) env) {
		helppath += env;
	}

	return;
}

BOOL
VIEWER::UserSetSize()
{
	const Frame	frame	= uimgr->FrameHandle();
	const STRING	sizeOpt	= "-Ws";

	const int	argc	= (int) xv_get(frame, FRAME_WM_COMMAND_ARGC);
	const char	**argv	= (char **)xv_get(frame,FRAME_WM_COMMAND_ARGV);

	volatile int	cntr;

	DbgFunc("UIMGR::UserSetSize:" << endl);
	assert(frame);

	for (cntr = 1; cntr < argc; cntr++) {
		if (argv[cntr] == sizeOpt)
			return(BOOL_TRUE);
	}

	return(BOOL_FALSE);
}

void
MAGVAL::Reset(const ViewerType vtype)
{
	char	*str;
	float	defaultMag;

	DbgFunc("MAGVAL::Init: entered" << endl);

	if (vtype == HELPVIEWER) {
		str = defaults_get_string("helpviewer.defaultmag",
					  "HelpViewer.DefaultMag",
					  HV_DEFAULT_MAG_STR);
	}
	else {
		str = defaults_get_string("docviewer.defaultmag",
					  "DocViewer.DefaultMag",
					  DV_DEFAULT_MAG_STR);
	}

	// Retrieve the language we're using for numbers.
	char *tmp_numeric = setlocale (LC_NUMERIC, NULL);

	// Set locale for numbers to C - this will allow the atof
	//   to work properly.
	setlocale (LC_NUMERIC, "C");

	DbgLow("MAGVAL::Init: defaults_get_string() returned\t= " <<
	       str << endl);

	if ((defaultMag = atof(str)) < 0.5) {
		if (vtype == HELPVIEWER)
			defaultMag = HV_DEFAULT_MAG;
		else
			defaultMag = DV_DEFAULT_MAG;
	}

	// Reset LC_NUMERIC back to what is was.
	if (tmp_numeric != (char *) NULL)
	   setlocale (LC_NUMERIC, tmp_numeric);

	Set(defaultMag);

	DbgLow("MAGVAL::Init: defaultMag\t= " << (const float) *this << endl);

	return;
}


void
MAGVAL::Set(const float arg)
{
	char buffer[8];

	if (arg > 0.0) {
		(void) sprintf(buffer, "%-d%%" , rint((val = arg) * 100.0));
		valStr = buffer;
	}
}
