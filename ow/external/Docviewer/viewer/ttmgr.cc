#ident "@(#)ttmgr.cc	1.9 02/10/93 Copyright 1992 Sun Microsystems, Inc."

#include "ttmgr.h"
#include <doc/console.h>
#include <doc/token_list.h>
#include <sys/systeminfo.h>

// should be defined in sys.param.h, but it is not
#define	MAXHOSTNAMELEN		256


extern CONSOLE console;

const	STRING	TTMGR::tt_navigator_name =	"navigator";


static	PATTERN_INFO	viewer_patterns[] =
		{
		    {VIEWER_TT_VIEW_DOCUMENT,		NULL},
		    {VIEWER_TT_VIEW_FILE,		NULL},
		    {VIEWER_TT_GET_CURRENT_DOCUMENT,	NULL},
		    {VIEWER_TT_GET_CURRENT_FILE,	NULL},
		    {VIEWER_TT_SET_CARD_CATALOGS,	NULL},
		    {VIEWER_TT_SET_PREFERRED_LANGUAGE,	NULL},
		    {VIEWER_TT_NEXT_PAGE,		NULL},
		    {VIEWER_TT_PREVIOUS_PAGE,		NULL},
		    {VIEWER_TT_GO_BACK,			NULL},
		    {VIEWER_TT_CUSTOM_MAGNIFY,		NULL},
		    {VIEWER_TT_PARTIAL_PAGE,		NULL},
		    {VIEWER_TT_FULL_PAGE,		NULL},
		    {VIEWER_TT_PING,			NULL},
		    {DOCVIEWER_TT_DEPARTING,		NULL},
		};

static	int	number_of_viewer_patterns = 
		  (sizeof( viewer_patterns ) / sizeof( PATTERN_INFO ));


STATUS
TTMGR::Init(const int	argc,
	    char      **argv,
	    ERRSTK     &err)
{
	int	slash;
	STATUS	status;

	DbgFunc("TTMGR::Init: entered" << endl);

	program_name = argv[ 0 ];

	// Get the basename of argv[0]
	//
	if ((slash = program_name.RightIndex('/')) >= 0) {
		program_name = program_name.SubString(slash + 1, END_OF_STRING);
	}

	status = TOOLTALK::Init(program_name, argc, argv, err);

	if (status == STATUS_OK) {

		status = RegisterDynamicPatterns(viewer_patterns,
						 number_of_viewer_patterns,
						 TT_HANDLE,
						 err);

	}

	return( status );
}

STATUS
TTMGR::CreateWhereAmIReply(const Tt_message	msg,
			   const DOCNAME	&whereami,
			   ERRSTK		&err)
{
	Tt_status	ttStatus;
	STRING		wherestr;
	STRING		nowherestr = "viewer could not resolve the current document";
	STATUS		status	= STATUS_OK;

	if ( whereami.IsValid() ) {
		DbgFunc("TTMGR::CreateWhereAmIReply: " << whereami << endl);

		wherestr = whereami.NameToString();
		ttStatus = tt_message_arg_val_set(msg, 0, ~wherestr);
	}
	else {
		wherestr = NULL_STRING;
		ttStatus = tt_message_arg_val_set(msg, 0, NULL_STRING);
	}

	if (wherestr != NULL_STRING) {
		ttStatus = tt_message_arg_val_set(msg, 1, DV_TT_STATUS_OK);
	}
	else {
		ttStatus = tt_message_arg_val_set(msg, 1, nowherestr);
	}

	if (ttStatus != TT_OK) {
		status = STATUS_FAILED;
		DbgHigh("TTMGR::CreateWhereAmIReply: " <<
			"tt_message_arg_val_set() failed - \"" <<
			tt_status_message(ttStatus) << "\"" << endl);
		err.Init(gettext("Error setting argument in tooltalk reply - %s"),
			 tt_status_message(ttStatus));
	}

	return(status);
}

STATUS
TTMGR::CreateWhatFileReply(const Tt_message	msg,
			   const STRING		&filename,
			   const int		view_page,
			   ERRSTK		&err)
{
	STRING		nowherestr = "viewer could not find the current file";
	STATUS		status	= STATUS_OK;
	Tt_status	ttstatus;

	DbgFunc("TTMGR::CreateWhatFileReply: " << filename << endl);

	if (filename != NULL_STRING) {
		ttstatus = tt_message_arg_val_set(msg, 0, ~filename);
		ttstatus = tt_message_arg_ival_set(msg, 1, view_page);
		ttstatus = tt_message_arg_val_set(msg, 2, DV_TT_STATUS_OK);
	}

	else {
		ttstatus = tt_message_arg_val_set(msg, 0, NULL_STRING);
		ttstatus = tt_message_arg_ival_set(msg, 1, 0);
		ttstatus = tt_message_arg_val_set(msg, 2, nowherestr);
	}

	if (ttstatus != TT_OK) {
		status = STATUS_FAILED;
		DbgHigh("TTMGR::CreateWhereAmIReply: " <<
			"tt_message_arg_val_set() failed - \"" <<
			tt_status_message(ttstatus) << "\"" << endl);
		err.Init(gettext("Error setting argument in tooltalk reply - %s"),
			 tt_status_message(ttstatus));
	}

	return(status);
}

STATUS
TTMGR::CreatePingReply(const Tt_message	msg,
		       const STRING	&hostname,
		       const STRING	&display,
		       const STRING	&viewer_name,
		       ERRSTK		&err)
{
	STRING		not_this_host =
			"this viewer is running on a different host";
	STRING		not_this_display =
			"this viewer is running on a different display";
	STRING		not_this_viewer =
			"this is the wrong viewer";
	STRING		real_display;
	STRING		our_display;
	char		our_hostname[ MAXHOSTNAMELEN ];
	STATUS		status	= STATUS_OK;
	Tt_status	ttStatus;

	DbgFunc("TTMGR::CreatePingReply: " << hostname << display << endl);
	(void) sysinfo(SI_HOSTNAME, our_hostname, MAXHOSTNAMELEN);
	
	if (hostname == our_hostname) {
		real_display = GetRealDisplay(display, hostname);
		our_display = getenv( "DISPLAY" );
		our_display = GetRealDisplay(our_display, our_hostname);

		if (real_display == our_display) {

		    if (viewer_name == program_name) {
			ttStatus = tt_message_arg_val_set(msg, 3,
							  tt_default_procid());
		    }
		    else {
			ttStatus = tt_message_arg_val_set(msg, 3,
							not_this_viewer);
			status = STATUS_FAILED;
		    }

		}

		else {
			ttStatus = tt_message_arg_val_set(msg, 3,
							  not_this_display);
			status = STATUS_FAILED;
		}

	}

	else {
		ttStatus = tt_message_arg_val_set(msg, 3, not_this_host);
		status = STATUS_FAILED;
	}

	return(status);
}

// This function takes all the permutations of how a DISPLAY might be specified,
// and converts it into a "canonical" form.
// The syntax for DISPLAY is [hostname[.domain]]:server[.screen]
// The canonical form is hostname:server.screen where hostname defaults to
// the local host, and screen defaults to 0.
STRING
TTMGR::GetRealDisplay(const	STRING	&display,
		      const	STRING	&hostname)
{
	STRING		colon(":");
	STRING		default_screen(".0");
	STRING		display_prefix;
	TOKEN_LIST	*display_tokens;
	STRING		real_display;
	STRING		server;
	TOKEN_LIST	*server_tokens;
	TOKEN_LIST	token_list(display, ':');

	DbgFunc("TTMSG::GetRealDisplay" << endl);

	if (token_list.Count() < 2) {
		server = token_list[ 0 ];
	}
	else {
		server = token_list[ 1 ];
		display_tokens = new TOKEN_LIST(token_list[ 0 ], '.');
		display_prefix = (*display_tokens)[ 0 ];
		delete display_tokens;
	}

	server_tokens = new TOKEN_LIST(server, '.');

	if (server_tokens->Count() < 2) {
		server = server + default_screen;
	}

	if (token_list.Count() < 2) {
		real_display = hostname + colon + server;
	}
	else {
		real_display = display_prefix + colon + server;
	}

	delete( server_tokens );

	return( real_display );
}


STATUS
TTMGR::SendIAmYourViewerMsg(STRING	destination_procid,
			    STRING	status_string,
			    STRING	our_procid,
			    ERRSTK	&err)
{
	STATUS		return_status = STATUS_OK;
	Tt_status	tt_status;
	Tt_message	ttmsg;


	assert(objstate.IsReady());
	DbgFunc("TTMSG::SendIAmYourViewerMsg: destination_procid = " << destination_procid << endl);
	DbgFunc("TTMSG::SendIAmYourViewerMsg: our_procid =         " << our_procid << endl);

	navigator_procid = destination_procid;

	ttmsg = CreateMessage(	tt_navigator_name,
				destination_procid,
				NAVIGATOR_TT_I_AM_YOUR_VIEWER,
				err);

	if (ttmsg == NULL) {
		return(STATUS_FAILED);
	}

	tt_status = tt_message_arg_add(ttmsg, TT_IN, "string", ~our_procid);

	if ((status_string == NULL_STRING) ||
	    (status_string.Length() <= 0)) {

		status_string = DV_TT_STATUS_OK;
	}

	tt_status = tt_message_arg_add(ttmsg, TT_IN, "string", ~status_string);

	if (tt_status != TT_OK) {
		err.Init(gettext("Can't create ToolTalk message: %s"),
			tt_status_message(tt_status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(gettext("Could not send view document message to %s"),
			 tt_navigator_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TTMGR::SendDepartingMsg(STRING	destination_procid,
		        ERRSTK	&err)
{
	Tt_message	ttmsg;

	assert(objstate.IsReady());
	DbgFunc("TTMGR::SendDepartingMsg" << endl);

	ttmsg = CreateMessage(	tt_navigator_name,
				navigator_procid,
				DOCVIEWER_TT_DEPARTING,
				err);

	if (ttmsg == NULL) {
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err) == STATUS_FAILED) {
		err.Push(gettext("Could not send departing message to %s"),
			 ~tt_navigator_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

TTMGR::~TTMGR()
{
	ERRSTK	err;

	DbgFunc("TTMGR::~TTMGER: entered" << endl);

	if (navigator_procid != NULL_STRING) {

		SendDepartingMsg(navigator_procid, err);

	}

}
