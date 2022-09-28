#ident "@(#)tt_view_driver.cc	1.44 01/14/94 Copyright 1992 Sun Microsystems, Inc."

#include <doc/tt_view_driver.h>
#include <doc/docname.h>
#include <sys/systeminfo.h>
#include "dvlocale.h"

// should be defined in sys.param.h, but it is not
#ifndef MAXHOSTNAMELEN
#define	MAXHOSTNAMELEN		256
#endif

const	int	viewer_timeout_factor = 120;

const	char	*const	DOCVIEWER_CALLBACK_IDENTIFIER("DocViewerCallback");


static	PATTERN_INFO	viewer_driver_patterns[] =
		{
		    {NAVIGATOR_TT_I_AM_YOUR_VIEWER,	NULL},
		    {DOCVIEWER_TT_DEPARTING,		NULL},
		};

static	int	number_of_vd_patterns = 
		  (sizeof( viewer_driver_patterns ) / sizeof( PATTERN_INFO ));




TT_VIEW_DRIVER::TT_VIEW_DRIVER() :
	tt_viewer_name("docviewer")
{
	DbgFunc("TT_VIEW_DRIVER::TT_VIEW_DRIVER" << endl);
}

TT_VIEW_DRIVER::TT_VIEW_DRIVER(STRING viewer_name) :
	tt_viewer_name(viewer_name)
{
	DbgFunc("TT_VIEW_DRIVER::TT_VIEW_DRIVER" << endl);
}

TT_VIEW_DRIVER::~TT_VIEW_DRIVER()
{
	ERRSTK	err;

	DbgFunc("TT_VIEW_DRIVER::~TT_VIEW_DRIVER" << endl);

	if ( IsViewerPresent() ) {
		(void) SendDeparting( err );
		viewerId = NULL_STRING;
	}

}

STATUS
TT_VIEW_DRIVER::Init(int	argc, char **argv, ERRSTK &err)
{
	STATUS	status;

	DbgFunc("TT_VIEW_DRIVER::Init" << endl);
	status = TOOLTALK::Init("navigator", argc, argv, err);

	if (status ==  STATUS_OK) {

		status = RegisterDynamicPatterns(viewer_driver_patterns,
						 number_of_vd_patterns,
						 TT_HANDLE,
						 err);

		// Set up handler for TT messages
		//
		TOOLTALK::SetEventProcs((TtMesgProc) MsgHandler,
					(TtReplyProc) ReplyHandler,
					(void *) this);
	}

	return( status );
}

// Do we current have a Viewer?
//
BOOL
TT_VIEW_DRIVER::IsViewerPresent()
{
	assert(objstate.IsReady());

	if (viewerId == NULL_STRING) {
		DbgFunc("TT_VIEW_DRIVER::IsViewerPresent: NO" << endl);
		return(BOOL_FALSE);
	} else {
		DbgFunc("TT_VIEW_DRIVER::IsViewerPresent: yes" << endl);
		return(BOOL_TRUE);
	}
}

STATUS
TT_VIEW_DRIVER::LaunchViewerWithDocname(const DOCNAME &docname,
					const STRING &preferred_language,
					const STRING &card_catalog,
					int	     x_position,
					int	     y_position,
					ERRSTK	     &err)
{
	STRING	namestr = docname.NameToString();
	STRING	start_flag = "-d";


	assert(objstate.IsReady());
	assert( ! IsViewerPresent());
	DbgFunc("TT_VIEW_DRIVER::LaunchViewerWithDocname" << endl);

	return( LaunchViewer(namestr, start_flag, preferred_language,
			     card_catalog, 0, NULL,
			     x_position, y_position, err) );
}

STATUS
TT_VIEW_DRIVER::LaunchViewerWithDocname(const DOCNAME &docname,
					const STRING &preferred_language,
					const STRING &card_catalog,
					int	     xview_argc,
					char	     **xview_argv,
					int	     x_position,
					int	     y_position,
					ERRSTK	     &err)
{
	STRING	namestr = docname.NameToString();
	STRING	start_flag = "-d";


	assert(objstate.IsReady());
	assert( ! IsViewerPresent());
	DbgFunc("TT_VIEW_DRIVER::LaunchViewerWithDocname" << endl);

	return( LaunchViewer(namestr, start_flag, preferred_language,
			     card_catalog, xview_argc, xview_argv,
			     x_position, y_position, err) );
}

STATUS
TT_VIEW_DRIVER::LaunchViewerWithFile(const STRING &path_to_file,
				     const STRING &preferred_language,
				     const STRING &card_catalog,
				     int	  x_position,
				     int	  y_position,
				     ERRSTK	  &err)
{
	STRING	start_flag = "-f";


	assert(objstate.IsReady());
	assert( ! IsViewerPresent());
	DbgFunc("TT_VIEW_DRIVER::LaunchViewerWithFile" << endl);

	return( LaunchViewer(path_to_file, start_flag, preferred_language,
			     card_catalog, 0, NULL,
			     x_position, y_position, err) );
}


Notify_value
TT_VIEW_DRIVER::ViewerWait3Event(caddr_t client_data,
			int /*pid*/,
			int *status,
			struct rusage *)
{
	TT_VIEW_DRIVER		*ttmgr;
	TT_VIEW_DRIVER_STATUS	ttstatus;


	ttmgr = (TT_VIEW_DRIVER *) client_data;
	assert(ttmgr->objstate.IsReady());
	DbgFunc("TT_VIEW_DRIVER::ViewerWait3Event\n");

	ttmgr->viewerId = NULL_STRING;

	if (ttmgr->event_handler) {
		(*ttmgr->event_handler)(TT_VIEW_DRIVER_VIEWER_EXIT_MSG,
					(caddr_t)&ttstatus,
					ttmgr->event_client_data);
	}

	return (NOTIFY_DONE);
}


STATUS
TT_VIEW_DRIVER::LaunchViewer(const STRING &docname_or_file,
			     const STRING &start_flag,
			     const STRING &preferred_language,
			     const STRING &card_catalog,
			     int	  xview_argc,
			     char	  **xview_argv,
			     int	  x_position,
			     int	  y_position,
			     ERRSTK &err)
{
	int	index;
	STRING	path_to_viewer;
	int	pid;
	char	*procid;
	STATUS	status = STATUS_OK;
	int	viewer_argc;
	char	**viewer_argv;
	int	viewer_argv_size;
	STRING	viewer_name = tt_viewer_name;
	char	x_buffer[ 20 ];
	char	y_buffer[ 20 ];
	char	*icon_path;


	assert(objstate.IsReady());
	assert( ! IsViewerPresent());
	DbgFunc("TT_VIEW_DRIVER::LaunchViewer" << endl);

	procid = tt_default_procid();

	// Start off with 20 for arguments set in here (more then enough)
	//
	viewer_argv_size = 20;

	if ((xview_argc > 0) && (xview_argv != NULL)) {
		viewer_argv_size += xview_argc;
	}

	viewer_argv = new char *[ viewer_argv_size ];

	viewer_argc = 0;
	viewer_argv[viewer_argc++] = (char *) ~viewer_name;

	if ((xview_argc > 0) && (xview_argv != NULL)) {

		for (index = 0;  index < xview_argc;  index++) {
			viewer_argv[viewer_argc++] = xview_argv[ index ];
		}

	}

	if ((x_position >= 0) && (y_position >= 0)) {
		viewer_argv[viewer_argc++] = "-Wp";
		sprintf(x_buffer, "%d", x_position);
		viewer_argv[viewer_argc++] = x_buffer;
		sprintf(y_buffer, "%d", y_position);
		viewer_argv[viewer_argc++] = y_buffer;
	}

	if ((icon_path = getenv ("DOCVIEWERICON")) != (char *) NULL) {
		viewer_argv[viewer_argc++] = "-WI";
		viewer_argv[viewer_argc++] = icon_path;
	}

	viewer_argv[viewer_argc++] = "-p";
	viewer_argv[viewer_argc++] = procid;
	viewer_argv[viewer_argc++] = (char *) ~start_flag;
	viewer_argv[viewer_argc++] = (char *) ~docname_or_file;

	if (preferred_language != NULL_STRING) {
		viewer_argv[viewer_argc++] = "-l";
		viewer_argv[viewer_argc++] = (char *) ~preferred_language;
	}

	if (card_catalog != NULL_STRING) {
		viewer_argv[viewer_argc++] = "-c";
		viewer_argv[viewer_argc++] = (char *) ~card_catalog;
	}

	viewer_argv[viewer_argc++] = NULL;

	pid = (int) fork();

	if (pid == 0) {
#ifndef DEBUG
		path_to_viewer = getenv("DOCVIEWER");
		if (path_to_viewer == NULL_STRING) { 
			path_to_viewer = getenv("OPENWINHOME");

			if ((path_to_viewer != NULL_STRING) &&
		    	    (path_to_viewer.Length() > 0)) {
				path_to_viewer += "/bin/";
				path_to_viewer += viewer_name;
			}
			else {
				path_to_viewer = viewer_name;
			}
		}
#else
		path_to_viewer = getenv("DOCVIEWER");
		if (path_to_viewer == NULL_STRING) 
			path_to_viewer = viewer_name;
#endif
		(void) execvp(path_to_viewer, viewer_argv);
		perror(viewer_name);
		_exit(1);	// don't call 'exit()' - see man pages for
	}
	else if (pid < 0) {
		err.Push(DGetText("could not exec '%s'"), ~viewer_name);
		status = STATUS_FAILED;
	}
	else {
		// Set up timer to handler Viewer failed to respond with
		// I_Am_Your_Viewer ToolTalk message
		timer.TimeOut(viewer_timeout_factor,
			      (TimerCallBack) TT_VIEW_DRIVER::ViewerTimeoutEvent,
			      (caddr_t) this);

		// Set up wait3() handler to detect Viewer demise.
		(void) notify_set_wait3_func((Notify_client) this,
					     (Notify_func)
					     &TT_VIEW_DRIVER::ViewerWait3Event,
					     pid);
	}

	if ((viewer_argc > 0) && (viewer_argv != NULL)) {
		delete [viewer_argv_size] viewer_argv;
	}

	return( status );
}

void
TT_VIEW_DRIVER::ViewerTimeoutEvent(caddr_t client_data)
{
	TT_VIEW_DRIVER		*ttmgr;
	TT_VIEW_DRIVER_STATUS	ttstatus;


	ttmgr = (TT_VIEW_DRIVER *) client_data;
	assert(ttmgr->objstate.IsReady());
	DbgFunc("TT_VIEW_DRIVER::ViewerTimeoutEvent\n");

	if (ttmgr->event_handler) {
		ttstatus.status = STATUS_FAILED;
		ttstatus.err.Init(DGetText("Could not launch new Viewer"));
		(*ttmgr->event_handler)(TT_VIEW_DRIVER_VIEWER_STARTUP_REPLY,
					(caddr_t)&ttstatus,
					ttmgr->event_client_data);
	}

}

STATUS
TT_VIEW_DRIVER::SendViewDocumentMsg(const DOCNAME &docname, ERRSTK &err)
{
	Tt_message	ttmsg;
	Tt_status	status;
	STRING		namestr = docname.NameToString();

	assert(objstate.IsReady());
	DbgFunc("TT_VIEW_DRIVER::SendViewDocumentMsg: " << namestr << endl);

	ttmsg = CreateMessage(	tt_viewer_name,
				viewerId,
				VIEWER_TT_VIEW_DOCUMENT,
				err);
	if (ttmsg == NULL) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 VIEWER_TT_VIEW_DOCUMENT);
		return(STATUS_FAILED);
	}

	status = tt_message_arg_add(ttmsg, TT_IN, "string", ~namestr);
	status = tt_message_arg_add(ttmsg, TT_OUT, "string", NULL);

	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send %s message to %s"),
			 VIEWER_TT_VIEW_DOCUMENT, ~tt_viewer_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TT_VIEW_DRIVER::SendViewFileMsg(const STRING &path_to_file, ERRSTK &err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	assert(objstate.IsReady());
	DbgFunc("TT_VIEW_DRIVER::SendViewFileMsg: " << path_to_file << endl);

	ttmsg = CreateMessage(	tt_viewer_name,
				viewerId,
				VIEWER_TT_VIEW_FILE,
				err);
	if (ttmsg == NULL) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 VIEWER_TT_VIEW_FILE);
		return(STATUS_FAILED);
	}

	status = tt_message_arg_add(ttmsg, TT_IN, "string", ~path_to_file);
	status = tt_message_arg_add(ttmsg, TT_OUT, "string", NULL);

	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send %s message to %s"),
			 VIEWER_TT_VIEW_FILE, ~tt_viewer_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TT_VIEW_DRIVER::SendWhereAmIMsg(ERRSTK &err)
{
	Tt_message	ttmsg;

	assert(objstate.IsReady());
	assert(IsViewerPresent());
	DbgFunc("TT_VIEW_DRIVER::SendWhereAmIMsg" << endl);

	ttmsg = CreateMessage(	tt_viewer_name,
				viewerId,
				VIEWER_TT_GET_CURRENT_DOCUMENT,
				err);
	if (ttmsg == NULL) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 VIEWER_TT_GET_CURRENT_DOCUMENT);
		return(STATUS_FAILED);
	}

	tt_message_arg_add(ttmsg, TT_OUT, "string", NULL);
	tt_message_arg_add(ttmsg, TT_OUT, "string", NULL);

	if (SendMessage(ttmsg, err) == STATUS_FAILED) {
		err.Push(DGetText("Could not send %s message to %s"),
			 VIEWER_TT_GET_CURRENT_DOCUMENT, ~tt_viewer_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TT_VIEW_DRIVER::SendWhatFileMsg(STATUS	(*callback)(STATUS	status,
						    STRING	&path_to_file,
						    int		page_number),
				ERRSTK	&err)
{
	Tt_status	status;
	Tt_message	ttmsg;

	assert(objstate.IsReady());
	assert(IsViewerPresent());
	DbgFunc("TT_VIEW_DRIVER::SendWhatFileMsg" << endl);

	ttmsg = CreateMessage(	tt_viewer_name,
				viewerId,
				VIEWER_TT_GET_CURRENT_FILE,
				err);
	if (ttmsg == NULL) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 VIEWER_TT_GET_CURRENT_FILE);
		return(STATUS_FAILED);
	}

	tt_message_arg_add(ttmsg, TT_OUT, "string", NULL);
	tt_message_arg_add(ttmsg, TT_OUT, "int", 0);
	tt_message_arg_add(ttmsg, TT_OUT, "string", NULL);
	status = tt_message_user_set(ttmsg, 0,
				     (void *) DOCVIEWER_CALLBACK_IDENTIFIER);
	status = tt_message_user_set(ttmsg, 1,
				     (void *) callback);

	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err) == STATUS_FAILED) {
		err.Push(DGetText("Could not send %s message to %s"),
			 VIEWER_TT_GET_CURRENT_FILE, ~tt_viewer_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TT_VIEW_DRIVER::SendDeparting(ERRSTK &err)
{
	Tt_message	ttmsg;

	assert(objstate.IsReady());
	assert(IsViewerPresent());
	DbgFunc("TT_VIEW_DRIVER::SendDeparting" << endl);

	ttmsg = CreateMessage(	tt_viewer_name,
				viewerId,
				DOCVIEWER_TT_DEPARTING,
				err);

	if (ttmsg == NULL) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 DOCVIEWER_TT_DEPARTING);
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err) == STATUS_FAILED) {
		err.Push(DGetText("Could not send departing message to %s"),
			 ~tt_viewer_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TT_VIEW_DRIVER::SetPreferredLanguage(const STRING &language, ERRSTK &err)
{
	Tt_message	ttmsg;
	Tt_status	status;


	assert(objstate.IsReady());
	DbgFunc("TT_VIEW_DRIVER::SetPreferredLanguage: " << language << endl);


	ttmsg = CreateMessage(	tt_viewer_name,
				viewerId,
				VIEWER_TT_SET_PREFERRED_LANGUAGE,
				err);
	if (ttmsg == NULL) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 VIEWER_TT_SET_PREFERRED_LANGUAGE);
		return(STATUS_FAILED);
	}

	status = tt_message_arg_add(ttmsg, TT_IN, "string", ~language);
	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err) == STATUS_FAILED) {
		err.Push(DGetText("Could not set Viewer preferred language"));
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TT_VIEW_DRIVER::SetCardCatalogs(const STRING &cclist, ERRSTK &err)
{
	Tt_message	ttmsg;
	Tt_status	status;


	assert(objstate.IsReady());
	DbgFunc("TT_VIEW_DRIVER::SetCardCatalogs: " << cclist << endl);


	ttmsg = CreateMessage(	tt_viewer_name,
				viewerId,
				VIEWER_TT_SET_CARD_CATALOGS,
				err);
	if (ttmsg == NULL) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 VIEWER_TT_SET_CARD_CATALOGS);
		return(STATUS_FAILED);
	}

	status = tt_message_arg_add(ttmsg, TT_IN, "string", ~cclist);
	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err) == STATUS_FAILED) {
		err.Push(DGetText("Could not set card catalogs in Viewer"));
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TT_VIEW_DRIVER::NextPage(ERRSTK &err)
{
	return( NextPage(1, err) );
}

STATUS
TT_VIEW_DRIVER::NextPage(int number_of_pages, ERRSTK &err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	assert(objstate.IsReady());
	DbgFunc("TT_VIEW_DRIVER::NextPage: " << endl);

	ttmsg = CreateMessage(	tt_viewer_name,
				viewerId,
				VIEWER_TT_NEXT_PAGE,
				err);
	if (ttmsg == NULL) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 VIEWER_TT_NEXT_PAGE);
		return(STATUS_FAILED);
	}

	status = tt_message_iarg_add(ttmsg, TT_IN, "int", number_of_pages);
	status = tt_message_arg_add(ttmsg, TT_OUT, "string", NULL);
	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send next page message to %s"),
			 tt_viewer_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}


STATUS
TT_VIEW_DRIVER::PreviousPage(ERRSTK &err)
{
	return( PreviousPage(1, err) );
}

STATUS
TT_VIEW_DRIVER::PreviousPage(int number_of_pages, ERRSTK &err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	assert(objstate.IsReady());
	DbgFunc("TT_VIEW_DRIVER::PreviousPage: " << endl);

	ttmsg = CreateMessage(	tt_viewer_name,
				viewerId,
				VIEWER_TT_PREVIOUS_PAGE,
				err);
	if (ttmsg == NULL) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 VIEWER_TT_PREVIOUS_PAGE);
		return(STATUS_FAILED);
	}

	status = tt_message_iarg_add(ttmsg, TT_IN, "int", number_of_pages);
	status = tt_message_arg_add(ttmsg, TT_OUT, "string", NULL);
	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send previous page message to %s"),
			 tt_viewer_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TT_VIEW_DRIVER::GoBack(ERRSTK &err)
{
	return( GoBack(1, err ) );
}

STATUS
TT_VIEW_DRIVER::GoBack(int number_of_pages, ERRSTK &err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	assert(objstate.IsReady());
	DbgFunc("TT_VIEW_DRIVER::GoBack: " << endl);

	ttmsg = CreateMessage(	tt_viewer_name,
				viewerId,
				VIEWER_TT_GO_BACK,
				err);
	if (ttmsg == NULL) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 VIEWER_TT_GO_BACK);
		return(STATUS_FAILED);
	}

	status = tt_message_iarg_add(ttmsg, TT_IN, "int", number_of_pages);
	status = tt_message_arg_add(ttmsg, TT_OUT, "string", NULL);
	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send go back message to %s"),
			 tt_viewer_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TT_VIEW_DRIVER::CustomMagnify(int level,
			      ERRSTK &err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	assert(objstate.IsReady());
	DbgFunc("TT_VIEW_DRIVER::CustomMagnify: " << endl);

	ttmsg = CreateMessage(	tt_viewer_name,
				viewerId,
				VIEWER_TT_CUSTOM_MAGNIFY,
				err);
	if (ttmsg == NULL) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 VIEWER_TT_CUSTOM_MAGNIFY);
		return(STATUS_FAILED);
	}

	status = tt_message_iarg_add(ttmsg, TT_IN, "int", level);
	status = tt_message_arg_add(ttmsg, TT_OUT, "string", NULL);
	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send custom magnify message to %s"),
			 tt_viewer_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TT_VIEW_DRIVER::PartialPage(ERRSTK &err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	assert(objstate.IsReady());
	DbgFunc("TT_VIEW_DRIVER::PartialPage: " << endl);

	ttmsg = CreateMessage(	tt_viewer_name,
				viewerId,
				VIEWER_TT_PARTIAL_PAGE,
				err);
	if (ttmsg == NULL) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 VIEWER_TT_PARTIAL_PAGE);
		return(STATUS_FAILED);
	}

	status = tt_message_arg_add(ttmsg, TT_OUT, "string", NULL);
	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send partial page message to %s"),
			 tt_viewer_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TT_VIEW_DRIVER::FullPage(ERRSTK &err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	assert(objstate.IsReady());
	DbgFunc("TT_VIEW_DRIVER::FullPage: " << endl);

	ttmsg = CreateMessage(	tt_viewer_name,
				viewerId,
				VIEWER_TT_FULL_PAGE,
				err);
	if (ttmsg == NULL) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 VIEWER_TT_FULL_PAGE);
		return(STATUS_FAILED);
	}

	status = tt_message_arg_add(ttmsg, TT_OUT, "string", NULL);
	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send full page message to %s"),
			 tt_viewer_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TT_VIEW_DRIVER::Ping(ERRSTK &err)
{
	Tt_message	ttmsg;
	Tt_status	status;
	STRING		display;
	char		hostname[ MAXHOSTNAMELEN ];

	assert(objstate.IsReady());
	DbgFunc("TT_VIEW_DRIVER::Ping: " << endl);

					// Note handler is NOT specified
	ttmsg = CreateMessage(	tt_viewer_name,
				NULL_STRING,
				VIEWER_TT_PING,
				err);
	if (ttmsg == NULL) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 VIEWER_TT_PING);
		return(STATUS_FAILED);
	}

	status = tt_message_address_set(ttmsg, TT_PROCEDURE);

	(void) sysinfo(SI_HOSTNAME, hostname, MAXHOSTNAMELEN);
	display = getenv( "DISPLAY" );
	status = tt_message_arg_add(ttmsg, TT_IN, "string", hostname);
	status = tt_message_arg_add(ttmsg, TT_IN, "string", display);
	status = tt_message_arg_add(ttmsg, TT_IN, "string", tt_viewer_name);
	status = tt_message_arg_add(ttmsg, TT_OUT, "string", NULL);

	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			 tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send ping message to %s"),
			 tt_viewer_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TT_VIEW_DRIVER::GetStatus(STATUS	(*callback)(STATUS	status,
						    STRING	&proc_status,
						    STRING	&vendor,
						    STRING	&progname,
						    STRING	&relname),
			  ERRSTK	&err)
{
	STRING	process_name = tt_viewer_name;

	return( TOOLTALK::GetStatus(process_name, viewerId, callback, err) );
}

STATUS
TT_VIEW_DRIVER::SetEnvironment(STRING	&variable,
			       STRING	&value,
			       ERRSTK	&err)
{
	STRING	process_name = tt_viewer_name;

	return( TOOLTALK::SetEnvironment(process_name, viewerId,
					 variable, value, err) );
}

STATUS
TT_VIEW_DRIVER::GetEnvironment(STRING	&variable,
			       STATUS	(*callback)(STATUS	status,
						    STRING	&variable,
						    STRING	&value),
			       ERRSTK &err)
{
	STRING	process_name = tt_viewer_name;

	return( TOOLTALK::GetEnvironment(process_name, viewerId,
					 variable, callback, err) );
}

STATUS
TT_VIEW_DRIVER::SetGeometry(int		width,
			    int		height,
			    int		xoffset,
			    int		yoffset,
			    ERRSTK	&err)
{
	STRING	process_name = tt_viewer_name;

	return( TOOLTALK::SetGeometry(process_name, viewerId,
				      width, height, xoffset, yoffset, err) );
}

STATUS
TT_VIEW_DRIVER::GetGeometry(STATUS	(*callback)(STATUS	status,
						    int		width,
						    int		height,
						    int		xoffset,
						    int		yoffset),
			    ERRSTK	&err)
{
	STRING	process_name = tt_viewer_name;

	return( TOOLTALK::GetGeometry(process_name, viewerId,
				      callback, err) );
}

STATUS
TT_VIEW_DRIVER::SetIconified(BOOL	close_it,
			     ERRSTK	&err)
{
	STRING	process_name = tt_viewer_name;

	return( TOOLTALK::SetIconified(process_name, viewerId, close_it, err) );
}

STATUS
TT_VIEW_DRIVER::GetIconified(STATUS	(*callback)(STATUS	status,
						    BOOL	iconified),
			     ERRSTK	&err)
{
	STRING	process_name = tt_viewer_name;

	return( TOOLTALK::GetIconified(process_name, viewerId, callback, err) );
}

STATUS
TT_VIEW_DRIVER::SetMapped(BOOL		mapped,
			  ERRSTK	&err)
{
	STRING	process_name = tt_viewer_name;

	return( TOOLTALK::SetMapped(process_name, viewerId, mapped, err) );
}

STATUS
TT_VIEW_DRIVER::GetMapped(STATUS	(*callback)(STATUS	status,
						    BOOL	mapped),
			  ERRSTK	&err)
{
	STRING	process_name = tt_viewer_name;

	return( TOOLTALK::GetMapped(process_name, viewerId, callback, err) );
}

STATUS
TT_VIEW_DRIVER::Raise(ERRSTK	&err)
{
	STRING	process_name = tt_viewer_name;

	return( TOOLTALK::Raise(process_name, viewerId, err) );
}

TtReplyStatus
TT_VIEW_DRIVER::MsgHandler(void *client_data, Tt_message ttmsg)
{
	STRING			msgOp;
	Tt_status		status;
	TT_VIEW_DRIVER		*ttmgr = (TT_VIEW_DRIVER *) client_data;
	TtReplyStatus		rstatus	= RS_NO_MATCH;
	TT_VIEW_DRIVER_STATUS	ttstatus;
	ERRSTK			err;
	unsigned char		*buffer_pointer;
	unsigned char		*status_pointer;
	int			length;
	STRING			status_string;
	Tt_state		message_state;


	assert(ttmgr != NULL);
	assert(ttmgr->objstate.IsReady());
	DbgFunc("TT_VIEW_DRIVER::MsgHandler: " << endl);

	msgOp = tt_message_op(ttmsg);

	if (msgOp == NAVIGATOR_TT_I_AM_YOUR_VIEWER) {
		ttmgr->timer.Cancel();
		status = tt_message_arg_bval(ttmsg, 0,
					     &buffer_pointer, &length);
		ttmgr->viewerId = (char *) buffer_pointer;
		status = tt_message_arg_bval(ttmsg, 1,
					     &status_pointer, &length);
		status_string = (char *) status_pointer;
		message_state = tt_message_state( ttmsg );

		if ((status_string != NULL_STRING) &&
		    (status_string != DV_TT_STATUS_OK) &&
		    (status_string.Length() > 0)) {
			ttstatus.status = STATUS_FAILED;
			ttstatus.err.Init( status_string );
			ttmgr->viewerId = NULL_STRING;
		}

		if (ttmgr->event_handler) {
		    (*ttmgr->event_handler)(TT_VIEW_DRIVER_VIEWER_STARTUP_REPLY,
					    (caddr_t)&ttstatus,
					    ttmgr->event_client_data);
		}

		rstatus = RS_NO_REPLY;
	}

	else if (msgOp == DOCVIEWER_TT_DEPARTING) {

		if (ttmgr->viewerId != tt_message_sender(ttmsg)) {

			ttmgr->viewerId = NULL_STRING;

			if (ttmgr->event_handler) {
				(*ttmgr->event_handler)(
						TT_VIEW_DRIVER_VIEWER_EXIT_MSG,
						(caddr_t)&ttstatus,
						ttmgr->event_client_data);
			}

		}

		rstatus = RS_NO_REPLY;
	}

	return(rstatus);
}

void
TT_VIEW_DRIVER::ReplyHandler(void *client_data, Tt_message msg)
{
	TT_VIEW_DRIVER		*ttmgr = (TT_VIEW_DRIVER *) client_data;
	STRING			msgOp;
	Tt_state		msgState = tt_message_state(msg);

	assert(ttmgr != NULL);
	assert(ttmgr->objstate.IsReady());
	DbgFunc("TT_VIEW_DRIVER::ReplyHandler: " << endl);

	msgOp = tt_message_op(msg);

	if (msgOp == VIEWER_TT_VIEW_DOCUMENT) {

		ttmgr->ViewDocumentReplyProc(msg, msgState);

	} else if (msgOp == VIEWER_TT_VIEW_FILE) {

		ttmgr->ViewDocumentReplyProc(msg, msgState);

	} else if (msgOp == VIEWER_TT_GET_CURRENT_DOCUMENT) {

		ttmgr->CurrentDocReplyProc(msg, msgState);

	} else if (msgOp == VIEWER_TT_GET_CURRENT_FILE) {

		ttmgr->CurrentFileReplyProc(msg, msgState);

	} else if (msgOp == VIEWER_TT_PING) {

		if (msgState == TT_HANDLED) {
			ttmgr->viewerId = tt_message_handler(msg);
		} else {
			ttmgr->viewerId = NULL_STRING;
		}

	} else if (msgOp == VIEWER_TT_SET_CARD_CATALOGS) {

	} else if (msgOp == VIEWER_TT_SET_PREFERRED_LANGUAGE) {

	} else if (msgOp == VIEWER_TT_NEXT_PAGE) {

	} else if (msgOp == VIEWER_TT_PREVIOUS_PAGE) {

	} else if (msgOp == VIEWER_TT_GO_BACK) {

	} else if (msgOp == VIEWER_TT_CUSTOM_MAGNIFY) {

	} else if (msgOp == VIEWER_TT_PARTIAL_PAGE) {

	} else if (msgOp == VIEWER_TT_FULL_PAGE) {

	}

}

void
TT_VIEW_DRIVER::ViewDocumentReplyProc(const Tt_message msg, const Tt_state msgState)
{
	BOOL			did_viewer_respond = BOOL_TRUE;
	STRING			status_string;
	TT_VIEW_DRIVER_STATUS	ttstatus;

	DbgFunc("TT_VIEW_DRIVER:ViewDocumentReplyProc:" << endl);

	// This is a reply to our request to the Viewer
	// display a document.  Notify interested
	// parties of the outcome of this event.
	//

	switch (msgState) {

	case TT_CREATED:
	case TT_QUEUED:
	case TT_SENT:
		// We don't need to to anything here.
		// We're waiting for a "TT_FAILED" or "TT_HANDLED".
		return;

	case TT_HANDLED:
		// Viewer successfully displayed the document.
		assert(viewerId == tt_message_handler(msg));
		break;

	case TT_FAILED:
	case TT_REJECTED:
// There is a tooltalk bug (1113909) where on 4.X systems ttsession will
// mark a reply with state set to TT_FAILED even though the message succeeded.
// In the ttsession trace scope is set to TT_SCOPE_NONE, but in the actual reply
// received here that is NOT the case (it is set to TT_SESSION).
// This code ignores the TT_FAILED state if the status string arg indicates
// success.
		status_string = tt_message_arg_val(msg, 1);

		if (status_string != DV_TT_STATUS_OK) {
			ttstatus.status = STATUS_FAILED;
			ttstatus.tt_arg = (void *) &did_viewer_respond;

			if ((status_string != NULL_STRING) &&
			    (status_string.Length() > 0)) {
				ttstatus.err.Init( status_string );
			}
			else {
				status_string =DGetText("Viewer did not respond");
				ttstatus.err.Init( status_string );
				did_viewer_respond = BOOL_FALSE;
				viewerId = NULL_STRING;
			}

		}
		else {
			assert(viewerId == tt_message_handler(msg));
		}

		break;

	case TT_STARTED:
		// Better not happen - Viewer should *only* get started
		// via an explicit "launch" message.
		assert(0);
		break;

	default:
		// unexpected tooltalk state
		assert(0);
		break;
	}

	if (event_handler) {
		(*event_handler)(TT_VIEW_DRIVER_VIEW_DOCUMENT_REPLY,
				(caddr_t)&ttstatus,
				event_client_data);
	}
}

void
TT_VIEW_DRIVER::CurrentDocReplyProc(const Tt_message msg, const Tt_state msgState)
{
	DOCNAME			whereami;// Viewer's response to "WhereAmI" msg
	STRING			tmpstr;
	const TT_VIEW_DRIVER_EVENT	event	= TT_VIEW_DRIVER_WHEREAMI_REPLY;
	TT_VIEW_DRIVER_STATUS		ttstatus;

	DbgFunc("TT_VIEW_DRIVER:CurrentDocReplyProc:" << endl);

	// This is a reply to our request to the Viewer to get the
	// current document. The Viewer's response is in the form of a
	// DOCNAME. Pass the status and the DOCNAME on to interested
	// parties.
	//

	if (msgState == TT_HANDLED) {
		// Get whereami return value from msg
		//
		tmpstr = tt_message_arg_val(msg, 0);
		if (whereami.Init(tmpstr) != STATUS_OK) {
			ttstatus.status = STATUS_FAILED;
			tmpstr = tt_message_arg_val(msg, 1);
			ttstatus.err.Init(DGetText("Location reported by Viewer is bad"));
		}
		else {
			ttstatus.tt_arg = (caddr_t) &whereami;
		}
	}
	else {
		ttstatus.status = STATUS_FAILED;
		tmpstr = tt_message_arg_val(msg, 1);

		if ((tmpstr == NULL_STRING) ||
		    (tmpstr.Length() <= 0)) {
			tmpstr = "Viewer is not responding; restart the application!";
		}

		ttstatus.err.Init( tmpstr );
	}

	if (event_handler) {
		(*event_handler)(event, (caddr_t)&ttstatus, event_client_data);
	}
}

void
TT_VIEW_DRIVER::CurrentFileReplyProc(const Tt_message	message,
				     const Tt_state	message_state)
{
	STATUS	(*callback)(...);
	STATUS	callback_status;
	STRING	error;
	char	*identifier;
	STRING	path_to_file;
	int	page_number;

	DbgFunc("TT_VIEW_DRIVER:CurrentFileReplyProc:" << endl);

	// This is a reply to our request to the Viewer to get the
	// current document. The Viewer's response is in the form of a
	// DOCNAME. Pass the status and the DOCNAME on to interested
	// parties.
	//

	identifier = (char *) tt_message_user(message, 0);
	callback = (STATUS (*)(...))tt_message_user(message, 1);

	if ((identifier == DOCVIEWER_CALLBACK_IDENTIFIER) &&
	    (callback != (void *) NULL)) {

		if (message_state != TT_HANDLED) {
			callback_status = STATUS_FAILED;
		}

		// Get whatfile return value from msg
		//
		path_to_file = tt_message_arg_val(message, 0);
		(void) tt_message_arg_ival(message, 1, &page_number);

		(void) (*callback)(callback_status,
				   path_to_file, page_number);
	}
}
