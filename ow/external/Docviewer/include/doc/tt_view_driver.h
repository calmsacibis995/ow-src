/* RE_SID: @(%)/export/build0/source/SOURCE+SCCS_S297_FCS/external/Docviewer/include/doc/SCCS/s.tt_view_driver.h 1.27 96/11/15 15:24:45 SMI */
#ifndef	_TT_VIEW_DRIVER_H_
#define	_TT_VIEW_DRIVER_H_

#ident "@(#)tt_view_driver.h	1.27 11/15/96 Copyright 1992 Sun Microsystems, Inc."

#include <doc/docname.h>
#include <doc/tooltalk.h>
#include <doc/itimer.h>


// Events generated by TT_VIEW_DRIVER.
//
typedef enum {
	TT_VIEW_DRIVER_VIEW_DOCUMENT_REPLY	= 1300,
	TT_VIEW_DRIVER_WHEREAMI_REPLY		= 1301,
	TT_VIEW_DRIVER_VIEWER_STARTUP_REPLY	= 1302,
	TT_VIEW_DRIVER_VIEWER_EXIT_MSG		= 1303
} TT_VIEW_DRIVER_EVENT;


// This structure contains the status and return value(s) (if any)
// of the current tooltalk message, and is passed to the registered
// tooltalk message handler.
//
class	TT_VIEW_DRIVER_STATUS {

    public:

	STATUS	status;
	ERRSTK	err;
	void	*tt_arg;

	TT_VIEW_DRIVER_STATUS() : status(STATUS_OK), tt_arg(NULL)	{ }
};


class TT_VIEW_DRIVER : public TOOLTALK {

	// DocViewer's ToolTalk name.
	//
	const STRING	tt_viewer_name;

	// Our viewer's handler id
	//
	STRING		viewerId;

	ITIMER		timer;

	//
	//
	void	ViewDocumentReplyProc(const Tt_message, const Tt_state);
	void	CurrentDocReplyProc(const Tt_message, const Tt_state);
	void	CurrentFileReplyProc(const Tt_message, const Tt_state);

	// Event handler and accompanying callback argument for
	// TT_VIEW_DRIVER events.
	//
	EVENT_HANDLER	event_handler;
	caddr_t		event_client_data;
	//
	STATUS	LaunchViewer(const STRING &docname_or_file,
			     const STRING &start_flag,
			     const STRING &preferred_language,
			     const STRING &card_catalog,
			     int	  xview_argc,
			     char	  **xview_argv,
			     int	  x_position,
			     int	  y_position,
			     ERRSTK	  &err);

	static TtReplyStatus	MsgHandler(	void		*clientData,
					   	Tt_message	msg);

	static void		ReplyHandler(	void		*clientData,
						Tt_message	msg);

	/*
	 * Handler for timeout event - when viewer does not reply with an
	 * I_Am_Your_Viewer ToolTalk message (see the ITIMER class)
	 */
	static	void	ViewerTimeoutEvent(caddr_t client_data);

	/*
	 * Handler for docviewer 'wait3()' events. Lets us know when
	 * docviewer exits, etc.
	 */
	static	Notify_value	ViewerWait3Event(caddr_t client_data,
						 int pid,
						 int *status,
						 struct rusage *);

    public:

	// TT_VIEW_DRIVER constructor, destructor.
	//
	TT_VIEW_DRIVER();
	TT_VIEW_DRIVER(STRING viewer_name);
	~TT_VIEW_DRIVER();

	// Initialize TT_VIEW_DRIVER.
	// Set up tooltalk session, etc.
	//
	STATUS	Init(int argc, char **argv, ERRSTK &err);

	// Do we current have a Viewer?
	//
	BOOL	IsViewerPresent();

	// Start up a new Viewer.
	//
	STATUS	LaunchViewerWithDocname(const DOCNAME	&docname,
					const STRING	&preferred_language,
					const STRING	&card_catalog,
					int		x_position,
					int		y_position,
					ERRSTK		&err);
	STATUS	LaunchViewerWithDocname(const DOCNAME	&docname,
					const STRING	&preferred_language,
					const STRING	&card_catalog,
					int		xview_argc,
					char		**xview_argv,
					int		x_position,
					int		y_position,
					ERRSTK		&err);
	STATUS	LaunchViewerWithFile(const STRING &path_to_file,
				     const STRING &preferred_language,
				     const STRING &card_catalog,
				     int	  x_position,
				     int	  y_position,
				     ERRSTK	  &err);


	// Register event handler for TT_VIEW_DRIVER events.
	//
	void	SetEventHandler(EVENT_HANDLER func, caddr_t clnt)
			{ event_handler = func; event_client_data = clnt; }


// Sending messages that use the Event Handler scheme
	// Send a message to set the Viewer's preferred language.
	//
	STATUS	SetPreferredLanguage(const STRING &language, ERRSTK &err);

	// Send a message to set the Viewer's card catalogs.
	// "cclist" is a colon-separated list of card catalog path names.
	//
	STATUS	SetCardCatalogs(const STRING &cclist, ERRSTK &err);

	// Send "view document" message to the Viewer.
	//
	STATUS	SendViewDocumentMsg(const DOCNAME &docname, ERRSTK &err);

	// Send "view file" message to the Viewer.
	//
	STATUS	SendViewFileMsg(const STRING &path_to_file, ERRSTK &err);

	// Send "where am I" message to the Viewer.
	//
	STATUS	SendWhereAmIMsg(ERRSTK &err);

	// Send "departing" message to the Viewer.
	//
	STATUS	SendDeparting(ERRSTK &err);

	STATUS	NextPage(ERRSTK &err);
	STATUS	NextPage(int number_of_pages, ERRSTK &err);
	STATUS	PreviousPage(ERRSTK &err);
	STATUS	PreviousPage(int number_of_pages, ERRSTK &err);
	STATUS	GoBack(ERRSTK &err);
	STATUS	GoBack(int number_of_pages, ERRSTK &err);
	STATUS	CustomMagnify(int level, ERRSTK &err);
	STATUS	PartialPage(ERRSTK &err);
	STATUS	FullPage(ERRSTK &err);
	STATUS	Ping(ERRSTK &err);


// Sending messages that use the Callback scheme

	// Send "where am I" message to the Viewer.
	//
	STATUS	SendWhatFileMsg(STATUS (*callback)(STATUS	status,
						   STRING	&path_to_file,
						   int		page_number),
				ERRSTK &err);

	// These are the functions for issuing Message Alliance commands.
	STATUS	GetStatus(STATUS (*callback)(STATUS	status,
					     STRING	&proc_status,
					     STRING	&vendor,
					     STRING	&progname,
					     STRING	&relname),
			  ERRSTK &err);
	STATUS	SetEnvironment(STRING	&variable,
			       STRING	&value,
			       ERRSTK	&err);
	STATUS	GetEnvironment(STRING	&variable,
			       STATUS	(*callback)(STATUS	status,
						    STRING	&variable,
						    STRING	&value),
			       ERRSTK &err);
	STATUS	SetGeometry(int		width,
			    int		height,
			    int		xoffset,
			    int		yoffset,
			    ERRSTK	&err);
	STATUS	GetGeometry(STATUS	(*callback)(STATUS	status,
						    int		width,
						    int		height,
						    int		xoffset,
						    int		yoffset),
			    ERRSTK	&err);
	STATUS	SetIconified(BOOL	close_it,
			     ERRSTK	&err);
	STATUS	GetIconified(STATUS (*callback)(STATUS	status,
						BOOL	iconified),
			     ERRSTK &err);
	STATUS	SetMapped(BOOL		close_it,
			  ERRSTK	&err);
	STATUS	GetMapped(STATUS (*callback)(STATUS	status,
						BOOL	iconified),
			  ERRSTK &err);
	STATUS	Raise(ERRSTK &err);


// When we need to know who we are talking to
	STRING	GetViewerId( void )
				{ return( viewerId ); }

	void	IKnowMyViewer(STRING my_viewer)
				{ viewerId = my_viewer; }
};

#endif	_TT_VIEW_DRIVER_H_
