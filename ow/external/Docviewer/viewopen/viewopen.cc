#ident "@(#)viewopen.cc	1.18 93/04/14 Copyright (c) 1990-1993 Sun Microsystems, Inc."

#include <doc/tt_view_driver.h>
#include <doc/console.h>
#include <doc/dvlink.h>
#include <doc/docname.h>
#include <doc/pathname.h>
#include <doc/tt_view_driver.h>
#include <doc/cardcats.h>
#include <doc/token_list.h>
#include <doc/utils.h>
#include <xview/xview.h> 
#include <locale.h>
#include <poll.h>


// viewopen: display a document in the Viewer
//
// Sends a message to the docviewer via ToolTalk. Syntax is "dvopen
// <cookie>", where "<cookie>" is typically a docviewer link cookie.
// Here are some examples:
// 
// view::first
// view:Guided_Tour.psx:3
// view::last
// view::next
// view:<Object Id>:1
// view::prev
// 
// or
// 
// <Object Id>
// <filename>
//

STATUS
ParseArgs(const int		argc,
	  char *const		*argv,
	  STRING		&progname,
	  STRING		&viewerTtId,
	  STRING		&docname,
	  STRING		&filename,
	  BOOL			&start_new_viewer,
	  STRING		&command_to_send,
	  int			&int_value,
	  STRING		&string_value,
	  int			&timeout,
	  int			&debug);

STATUS	SendCommand(STRING		&command_to_send,
		    int			int_value,
		    STRING		string_value,
		    TT_VIEW_DRIVER	*ttmgr,
		    int			timeout,
		    BOOL		&waiting_for_event,
		    ERRSTK		&err);

void	EventHandler(int event, caddr_t event_object, caddr_t client_data);

STATUS	GetFileCallback(STATUS	status,
			STRING	&path_to_file,
			int	page_number);
STATUS	GetStatusCallback(STATUS status,
			  STRING &proc_status,
			  STRING &vendor,
			  STRING &title,
			  STRING &release);
STATUS	GetEnvironmentCallback(STATUS status, STRING &variable, STRING &value);
STATUS	GetGeometryCallback(STATUS status,
			    int width, int height, int xoffset, int yoffset);
STATUS	GetIconifiedCallback(STATUS status, BOOL iconified);
STATUS	GetMappedCallback(STATUS status, BOOL mapped);

void	Usage();


const	STRING	helpopen			= "helpopen";
const	STRING	viewopen			= "viewopen";
const	STRING	helpviewer			= "helpviewer";
const	STRING	docviewer			= "docviewer";

const	STRING	next_page_command		= "NextPage";
const	STRING	previous_page_command		= "PrevPage";
const	STRING	go_back_command			= "GoBack";
const	STRING	custom_magnify_command		= "Magnify";
const	STRING	partial_page_command		= "Partial";
const	STRING	full_page_command		= "Full";
const	STRING	kill_command			= "Kill";

const	STRING	get_current_doc_command		= "Docname";
const	STRING	get_current_file_command	= "File";

const	STRING	info_command			= "Info";
const	STRING	setenv_command			= "Setenv";
const	STRING	getenv_command			= "Getenv";
const	STRING	locate_command			= "Locate";
const	STRING	position_command		= "Position";
const	STRING	open_command			= "Open";
const	STRING	close_command			= "Close";
const	STRING	is_open_command			= "IsOpen";
const	STRING	map_command			= "Map";
const	STRING	unmap_command			= "Unmap";
const	STRING	is_mapped_command		= "IsMapped";
const	STRING	raise_command			= "Raise";

// Text domain names for localization of error messages, etc.
// Do not change these names - they are registered with the text domain name
// registry (textdomain@Sun.COM).
//
static const STRING	HELP_DOMAINNAME("SUNW_DESKSET_ANSWERBOOK_HELPOPEN");
static const STRING	VIEW_DOMAINNAME("SUNW_DESKSET_ANSWERBOOK_VIEWOPEN");


static	STRING		progname;
static	CONSOLE		console;
static	BOOL		event_received = BOOL_FALSE;
static	BOOL		error_in_event = BOOL_FALSE;

int		debug = 0;




main(int argc, char **argv)
{
	STRING		command_to_send		= NULL_STRING;
	STRING		docname			= NULL_STRING;
	ERRSTK		err;
	int		exit_status		= 0;
	STRING		filename		= NULL_STRING;
	int		int_value		= -1;
	STRING		message_op;
	Tt_state	message_state;
	int		slash;
	TT_VIEW_DRIVER	*ttmgr			= NULL;
	BOOL		start_new_viewer	= BOOL_FALSE;
	STATUS		status;
	STRING		string_value		= NULL_STRING;
	int		timeout			= 30;
	STRING		viewerTtId		= NULL_STRING;
	BOOL		waiting_for_event	= BOOL_FALSE;


	progname = argv[0];

	// Get the basename of argv[0]
	//
	if ((slash = progname.RightIndex('/')) >= 0) {
		progname = progname.SubString(slash + 1, END_OF_STRING);
	}

	console.Init(progname);

	if (progname == helpopen) {
		ttmgr = new TT_VIEW_DRIVER(helpviewer);
	}
	else {
		ttmgr = new TT_VIEW_DRIVER(docviewer);
	}

	if ((status = ttmgr->Init(argc, argv, err)) != STATUS_OK) {
		console.Message(gettext("Can't initialize tooltalk"));
		(cerr << err);
		exit(1);
	}

	ttmgr->SetEventHandler(EventHandler, (caddr_t) ttmgr);

	// Initialize XView. RPC_CLNT needs this.
	//
	xv_init(XV_INIT_ARGC_PTR_ARGV,	&argc, argv,
		XV_USE_LOCALE,	TRUE,
		NULL);


	// Set up for localization.
  	//
	if (progname == helpopen)
		InitTextDomain(HELP_DOMAINNAME);
	else
		InitTextDomain(VIEW_DOMAINNAME);


	status = ParseArgs(argc, argv, progname,
			   viewerTtId, docname, filename,
			   start_new_viewer, command_to_send,
			   int_value, string_value, timeout, debug);
	if (status == STATUS_OK) {

	    if ((docname == NULL_STRING) &&
		(filename == NULL_STRING) && (command_to_send == NULL_STRING)) {
		status = STATUS_FAILED;
		Usage();
	    }
	
	    else {

		if ( start_new_viewer ) {

		    if (docname != NULL_STRING) {
			status = ttmgr->LaunchViewerWithDocname(docname,
								NULL_STRING,
								NULL_STRING,
								-1, -1,
								err);
		    }
		    else {
			status = ttmgr->LaunchViewerWithFile(filename,
							     NULL_STRING,
							     NULL_STRING,
							     -1, -1,
							     err);
		    }

		    waiting_for_event = BOOL_TRUE;
		    message_state = TT_HANDLED;

		    if (status != STATUS_OK) {
			(cerr << ~progname << ": " << err << endl);
		    }

		}

		else {

		    if (viewerTtId == NULL_STRING) {
			status = ttmgr->Ping( err );
			status = ttmgr->WaitForReply(timeout, message_op,
						     message_state, err);

			if (message_state != TT_HANDLED) {

			    if (docname != NULL_STRING) {
				status = ttmgr->LaunchViewerWithDocname(
							docname, NULL_STRING,
							NULL_STRING, -1, -1,
							err);
				waiting_for_event = BOOL_TRUE;
			    }
			    else if (filename != NULL_STRING) {
				status = ttmgr->LaunchViewerWithFile(
							filename, NULL_STRING,
							NULL_STRING, -1, -1,
							err);
				waiting_for_event = BOOL_TRUE;
			    }
			    else {
				status = STATUS_FAILED;
				cerr << "There must be a running docviewer to perform this operation" << endl;
			    }


			    if (status != STATUS_OK) {
				cerr << ~progname << ": " << err << endl;
			    }

			}

		    }

		    else {
		        ttmgr->IKnowMyViewer( viewerTtId );
		    }

		    if (status == STATUS_OK) {

			if (( !waiting_for_event ) && (docname != NULL_STRING)) {
			    ttmgr->SendViewDocumentMsg(docname, err);
			    status = ttmgr->WaitForReply(timeout, message_op,
							 message_state, err);
			}

			else if (( !waiting_for_event ) &&
				 (filename != NULL_STRING)) {
			    ttmgr->SendViewFileMsg(filename, err);
			    status = ttmgr->WaitForReply(timeout, message_op,
							 message_state, err);

			    if ((status == STATUS_OK) &&
				(message_state == TT_HANDLED)) {
				status = ttmgr->SetIconified(BOOL_FALSE, err);
			    }

			}

			else if (command_to_send != NULL_STRING) {
			    status = SendCommand(command_to_send,
						 int_value, string_value,
						 ttmgr, timeout,
						 waiting_for_event, err);
			}

		    }

		}

	    }

	}

	if ( waiting_for_event ) {

		ttmgr->WaitForEvent(timeout, message_op, message_state, err);

		if ( !event_received ) {
			cout << err << endl;
		}

		if (( !event_received ) || ( error_in_event )) {
			status = STATUS_FAILED;
		}

	}

	if ((status != STATUS_OK) || ( error_in_event )) {
	    exit_status = 1;
	}

	exit( exit_status );
}

STATUS
ParseArgs(const int		argc,
	  char *const		*argv,
	  STRING		&progname,
	  STRING		&viewerTtId,
	  STRING		&docname,
	  STRING		&filename,
	  BOOL			&start_new_viewer,
	  STRING		&command_to_send,
	  int			&int_value,
	  STRING		&string_value,
	  int			&timeout,
	  int			&debug)
{
	int		opt;
	STATUS		status = STATUS_OK;

#ifdef	DEBUG
	while ((opt = getopt(argc, argv, "d:f:i:st:o:v:x:")) != EOF) {
		switch (opt) {
		case 'x':
			debug = atoi(optarg);
			break;
#else

	while ((opt = getopt(argc, argv, "d:f:i:st:o:v:")) != EOF) {
		switch (opt) {
#endif				/* DEBUG */

		case 'd':
			if (command_to_send != NULL_STRING) {
				console.Message(gettext("-d and -o options are mutually exclusive."));
				status = STATUS_FAILED;
			}
			else if (filename != NULL_STRING) {
				console.Message(gettext("-d and -f options are mutually exclusive."));
				status = STATUS_FAILED;
			}
			else if (progname == helpopen) {
				console.Message(gettext("-d is incompatible with helpopen."));
				status = STATUS_FAILED;
			}
			else {
				docname = optarg;
			}
			break;

		case 'f':
			if (command_to_send != NULL_STRING) {
				console.Message(gettext("-d and -f options are mutually exclusive."));
				status = STATUS_FAILED;
			}
			else if (docname != NULL_STRING) {
				console.Message(gettext("-f and -d options are mutually exclusive."));
				status = STATUS_FAILED;
			}
			else {
				filename = optarg;
			}
			break;

		case 'i':
			if (progname == helpopen) {
				status = STATUS_FAILED;
				console.Message(gettext("\"-i\" not valid"));
			}
			else if ( start_new_viewer ) {
				console.Message(gettext("-i and -s options are mutually exclusive"));
				status = STATUS_FAILED;
			}

			viewerTtId = optarg;
			break;

		case 's':
			if (progname == helpopen) {
				status = STATUS_FAILED;
				console.Message(gettext("\"-s\" not valid"));
			}
			if (viewerTtId != NULL_STRING) {
				console.Message(gettext("-i and -s options are mutually exclusive."));
				status = STATUS_FAILED;
			}
			else if (command_to_send != NULL_STRING) {
				console.Message(gettext("-s and -o options are mutually exclusive."));
				status = STATUS_FAILED;
			}
			else {
				start_new_viewer = BOOL_TRUE;
			}
			break;

		case 't':
			if ((timeout = atoi(optarg)) < 0 || timeout > 60) {
				console.Message(gettext("error - %s %s\"%d\""),
						gettext("invalid timeout:"),
						gettext("range is 0-60"),
						timeout);
				status = STATUS_FAILED;
			}
			break;

		case 'o':
			if ( start_new_viewer ) {
				console.Message(gettext("-s and -o options are mutually exclusive."));
				status = STATUS_FAILED;
			}
			else if (docname != NULL_STRING) {
				console.Message(gettext("-d and -o options are mutually exclusive."));
				status = STATUS_FAILED;
			}
			else if (filename != NULL_STRING) {
				console.Message(gettext("-f and -o options are mutually exclusive."));
				status = STATUS_FAILED;
			}
			else {
				command_to_send = optarg;
			}
			break;

		case 'v':
			string_value = optarg;
			int_value = atoi( optarg );
			break;

		default:
			status = STATUS_FAILED;
		}
	}

	if (optind == argc - 1) {

		if (progname == viewopen) {
			docname	= argv[optind];
		}
		else {
			filename = argv[optind];
		}

	}

	if (status == STATUS_FAILED) {
		Usage();
	}

	return (status);
}

STATUS	SendCommand(STRING		&command_to_send,
		    int			int_value,
		    STRING		string_value,
		    TT_VIEW_DRIVER	*ttmgr,
		    int			timeout,
		    BOOL		&waiting_for_event,
		    ERRSTK		&err)
{
	TOKEN_LIST	env_tokens(string_value, '=');
	int		height;
	STRING		message_op;
	Tt_state	message_state;
	TOKEN_LIST	pos_tokens(string_value, ':');
	STATUS		status = STATUS_FAILED;
	STRING		value;
	STRING		variable;
	int		width;
	int		xoffset;
	int		yoffset;

	if (command_to_send == next_page_command) {

		if (int_value > 1) {
			status = ttmgr->NextPage(int_value, err);
		}
		else {
			status = ttmgr->NextPage( err );
		}
	}
	else if (command_to_send == previous_page_command) {

		if (int_value > 1) {
			status = ttmgr->PreviousPage(int_value, err);
		}
		else {
			status = ttmgr->PreviousPage( err );
		}
	}
	else if (command_to_send == go_back_command) {

		if (int_value > 1) {
			status = ttmgr->GoBack(int_value, err);
		}
		else {
			status = ttmgr->GoBack( err );
		}
	}
	else if (command_to_send == custom_magnify_command) {

		if (int_value == -1) {
			int_value = 100;
		}

		status = ttmgr->CustomMagnify(int_value, err);
	}
	else if (command_to_send == partial_page_command) {
		status = ttmgr->PartialPage( err );
	}
	else if (command_to_send == full_page_command) {
		status = ttmgr->FullPage( err );
	}
	else if (command_to_send == kill_command) {
		status = ttmgr->SendDeparting( err );
	}
	else if (command_to_send == get_current_doc_command) {
		status = ttmgr->SendWhereAmIMsg( err );
	}
	else if (command_to_send == get_current_file_command) {
		status = ttmgr->SendWhatFileMsg(GetFileCallback, err);
	}

// Message Alliance commands
	else if (command_to_send == info_command) {
		status = ttmgr->GetStatus(GetStatusCallback, err);
	}
	else if (command_to_send == setenv_command) {
		variable = env_tokens[ 0 ];
		value = env_tokens[ 1 ];
		status = ttmgr->SetEnvironment(variable, value, err);
	}
	else if (command_to_send == getenv_command) {
		status = ttmgr->GetEnvironment(string_value,
					       GetEnvironmentCallback, err);
	}
	else if (command_to_send == position_command) {
		width = atoi( pos_tokens[ 0 ] );
		height = atoi( pos_tokens[ 1 ] );
		xoffset = atoi( pos_tokens[ 2 ] );
		yoffset = atoi( pos_tokens[ 3 ] );
		status = ttmgr->SetGeometry(width, height, xoffset, yoffset,err);
	}
	else if (command_to_send == locate_command) {
		status = ttmgr->GetGeometry(GetGeometryCallback, err);
	}
	else if (command_to_send == open_command) {
		status = ttmgr->SetIconified(BOOL_FALSE, err);
	}
	else if (command_to_send == close_command) {
		status = ttmgr->SetIconified(BOOL_TRUE, err);
	}
	else if (command_to_send == is_open_command) {
		status = ttmgr->GetIconified(GetIconifiedCallback, err);
	}
	else if (command_to_send == map_command) {
		status = ttmgr->SetMapped(BOOL_TRUE, err);
	}
	else if (command_to_send == unmap_command) {
		status = ttmgr->SetMapped(BOOL_FALSE, err);
	}
	else if (command_to_send == is_mapped_command) {
		status = ttmgr->GetMapped(GetMappedCallback, err);
	}
	else if (command_to_send == raise_command) {
		status = ttmgr->Raise( err );
	}

	status = ttmgr->WaitForReply(timeout, message_op,
				     message_state, err);

	if (message_state == TT_FAILED) {
		status = STATUS_FAILED;
	}

	return( status );
}

void
EventHandler(int event, caddr_t event_object, caddr_t client_data)
{
	DOCNAME			*docname;
	STRING			docname_string;
	STRING			*filename_string;
	TT_VIEW_DRIVER		*ttmgr;
	TT_VIEW_DRIVER_STATUS	*ttstatus;
	STRING			viewer_id;

	ttstatus = (TT_VIEW_DRIVER_STATUS *) event_object;

	if (event == TT_VIEW_DRIVER_VIEWER_STARTUP_REPLY) {

		if (ttstatus->status == STATUS_OK) {
			ttmgr = (TT_VIEW_DRIVER *) client_data;
			viewer_id = ttmgr->GetViewerId();

			if (progname == viewopen) {
				cout << viewer_id << endl;
			}

		}
		else {
			cout << ttstatus->err << endl;
			error_in_event = BOOL_TRUE;
		}

		event_received = BOOL_TRUE;
	}

	else if (event == TT_VIEW_DRIVER_WHEREAMI_REPLY) {

		if (ttstatus->status == STATUS_OK) {
			docname = (DOCNAME *) ttstatus->tt_arg;
			docname_string = docname->NameToString();
			cout << docname_string << endl;
		}
		else {
			cout << ttstatus->err << endl;
			error_in_event = BOOL_TRUE;
		}

		event_received = BOOL_TRUE;
	}

}

STATUS
GetFileCallback(STATUS	status,
		STRING	&path_to_file,
		int	page_number)
{
	STATUS	return_status = STATUS_OK;

	cout << "path_to_file=" << path_to_file << endl;
	cout << "page_number=" << page_number << endl;

	return( return_status );
}

STATUS
GetStatusCallback(STATUS status,
		  STRING &proc_status,
		  STRING &vendor,
		  STRING &title,
		  STRING &release)
{
	STATUS	return_status = STATUS_OK;

	cout << "proc_status=" << proc_status << endl;
	cout << "vendor=" << vendor << endl;
	cout << "title=" << title << endl;
	cout << "release=" << release << endl;

	return( return_status );
}

STATUS
GetEnvironmentCallback(STATUS status, STRING &variable, STRING &value)
{
	STATUS	return_status = STATUS_OK;

	cout << variable << "=" << value << endl;

	return( return_status );
}

STATUS
GetGeometryCallback(STATUS status,
		    int width, int height, int xoffset, int yoffset)
{
	STATUS	return_status = STATUS_OK;

	cout << width << ":" << height << ":"
					<< xoffset << ":" << yoffset << endl;

	return( return_status );
}

STATUS
GetIconifiedCallback(STATUS status, BOOL iconified)
{
	STATUS	return_status = STATUS_OK;

	cout << !iconified << endl;

	return( return_status );
}

STATUS
GetMappedCallback(STATUS status, BOOL mapped)
{
	STATUS	return_status = STATUS_OK;

	cout << mapped << endl;

	return( return_status );
}

void
Usage()
{
	cerr << gettext("usage: ") << ~progname << " ";
#ifdef	DEBUG
	cerr << gettext("[-x debug] ");
#endif
	if (progname == helpopen) {
		cerr << gettext("-f filename [-t timeout]");
	}
	else {
		cerr <<  gettext("[-d docname] [-f filename] [-s] [-i procid] [-o command [-v value]] [-t timeout]");
	}
	cerr << endl;
}
