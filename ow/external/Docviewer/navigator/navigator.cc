#ident "@(#)navigator.cc	1.42 94/05/26 Copyright 1990-1992 Sun Microsystems, Inc."

#include "navigator.h"
#include "doc/tt_view_driver.h"
#include "uimgr.h"
#include <doc/bookshelf.h>
#include <doc/token_list.h>
#include <xview/frame.h>


// Navigator version string.
//
const STRING	navigator_version("3.6 FCS");

// Flag to determine if we are "cleaning up". Necessary in case MyExitFunc is
// invoked while processing the DestroyEvent.
//
BOOL	cleaning_up = BOOL_FALSE;


// Event handlers for XView "destroy" and "signal" events.
//
static Notify_value	DestroyEvent(	Notify_client client,
					Destroy_status status);
static Notify_value	SignalEvent(	Notify_client client,
					int sig,
					Notify_signal_mode when);

void			MyExitFunc( void );


// Unique key for associating NAVIGATOR object with frame.
// This allows "DestroyEvent()" to retrieve the NAVIGATOR object
// from the frame.
//
static int	NAVIGATOR_KEY = (int) xv_unique_key();


// NAVIGATOR constructor.
//
NAVIGATOR::NAVIGATOR() :
	uimgr(NULL),
#ifdef	LOG
	logger(NULL),
#endif	LOG
	ttmgr(NULL)
{
	DbgFunc("NAVIGATOR::NAVIGATOR" << endl);
}

// NAVIGATOR destructor.
//
NAVIGATOR::~NAVIGATOR()
{
	DbgFunc("NAVIGATOR::~NAVIGATOR" << endl);

	delete uimgr;
	delete ttmgr;
#ifdef	LOG
	delete logger;
#endif	LOG
}

// Initialize this NAVIGATOR.
//
STATUS
NAVIGATOR::Init(int *argc_ptr, char **argv, ERRSTK &err)
{
	Display	*display;
	char	*display_string;
	BOOL	end_of_list;
	int	index,
		index_2;
	int	length;
	BOOL	match_found;
	char	*putenv_string;
	int	save_argc;
	char	**save_argv;
	char	*variable = "DISPLAY";
	int	xview_argc = 0;
	char	**xview_argv;

	assert( ! objstate.IsReady());
	objstate.MarkGettingReady();

	save_argc = *argc_ptr;
	save_argv = new char *[ save_argc ];
	xview_argv = new char *[ save_argc ];

	for (index = 0;  index < save_argc;  index++) {
		save_argv[ index ] = argv[ index ];
	}

	// Initialize window system
	//
        xv_init(XV_INIT_ARGC_PTR_ARGV, argc_ptr, argv,
                XV_USE_LOCALE, TRUE,
                NULL);

	// Find all the arguments that were stripped from argv by xv_init
	//
	for (index = 1;  index < save_argc;  index++) {

		match_found = BOOL_FALSE;
		end_of_list = BOOL_FALSE;
		index_2 = 0;

		while (( !match_found ) && ( !end_of_list )) {

			if (argv[ index_2 ] == save_argv[ index ]) {
				match_found = BOOL_TRUE;
			}
			else if (index_2 < (*argc_ptr - 1)) {
				index_2++;
			}
			else {
				end_of_list = BOOL_TRUE;
			}

		}

		if ( !match_found ) {
			xview_argv[ xview_argc ] = save_argv[ index ];
			xview_argc++;
		}

	}

	// Create main window frame.
	//
	InitMainFrame(*argc_ptr, argv);

	// Make sure DISPLAY is set for viewers to be forked
	//
	display = (Display *) xv_get(main_frame, XV_DISPLAY);
	display_string = DisplayString( display );
	length = strlen( variable ) + strlen( display_string ) + 2;
	putenv_string = new char[ length ];
	sprintf(putenv_string, "%s=%s", variable, display_string);
	putenv( putenv_string );

	// Create, initialize the various components of this NAVIGATOR.
	//
	ttmgr = new TT_VIEW_DRIVER();
	uimgr = new UIMGR(main_frame, cardcats);

	if (uimgr->Init(err) != STATUS_OK) {
		err.Push(gettext("Can't initialize windows"));
		return(STATUS_FAILED);
	}

	uimgr->SaveArgs(save_argc, save_argv);
	uimgr->SaveXViewArgs(xview_argc, xview_argv);
	delete [ save_argc ] xview_argv;

	if (ttmgr->Init(*argc_ptr, argv, err) != STATUS_OK) {
		err.Push(gettext("Can't initialize tooltalk"));
		return(STATUS_FAILED);
	} else {
	    ttmgr->EnableMessageAllianceProtocol(uimgr->FrameHandle(), err);
	}


	// Stash pointer to ourselves into frame handle
	// for later use by "DestroyEvent()", etc.
	//
	xv_set(main_frame, XV_KEY_DATA, NAVIGATOR_KEY, (caddr_t)this, XV_NULL);


	// Set up handlers for common signals.
	//
	(void) notify_set_signal_func(main_frame, (Notify_func) SignalEvent,
				      SIGHUP, NOTIFY_SYNC);
	(void) notify_set_signal_func(main_frame, (Notify_func) SignalEvent,
				      SIGINT, NOTIFY_SYNC);
	(void) notify_set_signal_func(main_frame, (Notify_func) SignalEvent,
				      SIGTERM, NOTIFY_SYNC);

	// Interpose on frame destruction so we can clean up after ourselves.
	//
	notify_interpose_destroy_func(main_frame, (Notify_func)DestroyEvent);

	// If the user exits from the window system a detroy interposer is not
	// always called; establish an exit function to make sure we clean up.
	//
	(void) atexit( MyExitFunc );


	// We're ready to roll...
	//
	objstate.MarkReady();
	return(STATUS_OK);
}

// Set the preferred language for this session.
//
void
NAVIGATOR::SetPreferredLanguage(const STRING &lang)
{
	assert(objstate.IsReady());
	DbgFunc("NAVIGATOR::SetPreferredLanguage: " << lang << endl);

	uimgr->SetPreferredLanguage(lang);
}

// Enter window system event loop.
//
void
NAVIGATOR::EnterEventLoop()
{
	assert(objstate.IsReady());
	assert(main_frame != XV_NULL);
	DbgFunc("NAVIGATOR::EnterEventLoop" << endl);

	xv_main_loop(main_frame);
}

// Interposer for frame 'destroy' events.
// Allows us to clean up after ourselves properly.
//
Notify_value
DestroyEvent(Notify_client client, Destroy_status status)
{
	Xv_opaque	frame;
	NAVIGATOR	*navigator;
	UIMGR		*uimgr;
	int		argc;
	char		**argv;
	char		*save_arg;


	frame = (Xv_opaque) client;
	assert(frame != NULL);

	navigator = (NAVIGATOR *)xv_get(frame, XV_KEY_DATA, NAVIGATOR_KEY);


	switch (status) {

	// Process is dying - there's not much of anything we can do ...
	case DESTROY_PROCESS_DEATH:
		DbgFunc("DestroyEvent: process death" << endl);
		break;

	// We've been asked to tidy up after ourselves ...
	case DESTROY_CLEANUP:
		DbgFunc("DestroyEvent: cleaning up" << endl);
		MyExitFunc();
		return(notify_next_destroy_func(client, status));

	// The user selected 'Save Workspace'
	case DESTROY_SAVE_YOURSELF:
		DbgFunc("DestroyEvent: save yourself" << endl);
		assert(navigator != NULL);

		// Get arguments used to start the Navigator.
		//
		uimgr = navigator->GetUIMgr();
		uimgr->GetArgs(&argc, &argv);

		// Navigator was started by the answerbook script,
		// so we need to make sure argv[0] is "answerbook"
		// for the sake of saving the workspace.
		//
		save_arg = argv[0];
		argv[0] = "answerbook";

		// Save the startup state with the wingow mgr.
		//
		XSetCommand((Display *)xv_get(frame, XV_DISPLAY),
			(Window)xv_get(frame, XV_XID), argv, argc);

		// Reinstall original argv[0].
		argv[0] = save_arg;

		break;

	// Just checking - we're being given the chance to veto the destroy
	case DESTROY_CHECKING:
		DbgFunc("DestroyEvent: just checking" << endl);
		break;

	default:
		assert(0);
	}

	return(NOTIFY_DONE);
}

void
MyExitFunc()
{

	if ( !cleaning_up ) {
		cleaning_up = BOOL_TRUE;

		if (bookshelf != NULL) {
			delete bookshelf;
			bookshelf = NULL;
		}

		if (navigator != NULL) {
			delete navigator;
			navigator = NULL;
		}

	}

}

Notify_value
SignalEvent(Notify_client client, int sig, Notify_signal_mode /* when */)
{
	DbgFunc("SignalEvent: " << sig << endl);
	xv_destroy_safe((Frame) client);
	return(NOTIFY_DONE);
}

TT_VIEW_DRIVER *
NAVIGATOR::GetTTMgr() const
{
	assert(objstate.IsReady());
	assert(ttmgr != NULL);
	return(ttmgr);
}

UIMGR *
NAVIGATOR::GetUIMgr() const
{
	assert(objstate.IsReady());
	assert(uimgr != NULL);
	return(uimgr);
}

Xv_opaque
NAVIGATOR::GetMainFrame() const
{
	assert(objstate.IsReady());
	assert(main_frame != NULL);
	return(main_frame);
}

#ifdef	LOG
LOGGER *
NAVIGATOR::GetLogger() const
{
	STRING	logfile;


	assert(objstate.IsReady());


	// If $DVLOGFILE is set, initialize event logging mechanism.
	//
	if (logger==NULL  &&  (logfile=getenv("DVLOGFILE")) != NULL_STRING) {

		logger = new LOGGER(logfile);
		if (logger->Init() != STATUS_OK) {
			delete logger;
			logger = NULL;
			fprintf(stderr,	gettext("Can't init event logger"));
		}
	}

	return(logger);
}
#endif	LOG

static u_short		icon_bits[] =
{
#include <images/gator.icon>
};

static u_short		mask_bits[] =
{
#include <images/gatormask.icon>
};

void
NAVIGATOR::InitMainFrame(int argc, char **argv)
{
	Icon		icon;
	Server_image	icon_image;
	Server_image	mask_image;


	DbgFunc("NAVIGATOR::InitMainFrame" << endl);


	// Create Navigator icon.
	//
	icon_image = xv_create(XV_NULL, SERVER_IMAGE,
				XV_WIDTH,		64,
				XV_HEIGHT,		64,
				SERVER_IMAGE_BITS,	icon_bits,
				XV_NULL);

	mask_image = xv_create(XV_NULL, SERVER_IMAGE,
				XV_WIDTH,		64,
				XV_HEIGHT,		64,
				SERVER_IMAGE_BITS,	mask_bits,
				XV_NULL);

	if (icon_image == XV_NULL ||  mask_image == XV_NULL)
		OutOfMemory();


	icon = xv_create(XV_NULL, ICON,
				ICON_IMAGE,		icon_image,
				ICON_MASK_IMAGE,	mask_image,
				ICON_LABEL,		gettext("Navigator"),
				ICON_TRANSPARENT,	TRUE,
				XV_NULL);

	if (icon == XV_NULL)
		OutOfMemory();


	// Create main window frame.
	//
	main_frame = xv_create(XV_NULL, FRAME,
				XV_X,			0,
				XV_Y,			0,
				FRAME_ICON,		icon,
				FRAME_WM_COMMAND_ARGC_ARGV, argc, argv,
				FRAME_SHOW_LABEL,	TRUE,
				FRAME_NO_CONFIRM,	TRUE,
				FRAME_SHOW_FOOTER,	TRUE,
				XV_NULL);

	if (main_frame == XV_NULL)
		OutOfMemory();

}

void
NAVIGATOR::ViewFile (const STRING &filename, ERRSTK &err)
{
        STATUS status = uimgr->ViewFile (filename, err);
}

