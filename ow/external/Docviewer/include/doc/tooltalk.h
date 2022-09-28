#ifndef	_OPT_TOOLTALK_H_
#define _OPT_TOOLTALK_H_

#ident "@(#)tooltalk.h	1.18 11/15/96 Copyright 1992 Sun Microsystems, Inc."

#include <doc/common.h>
#include <desktop/tt_c.h>
#include <xview/xview.h>

// Return codes from message handling routines.
typedef enum {
	RS_NO_MATCH,	// No match on the message; keep looking or reject
	RS_REJECT,	// Reject the message
	RS_REPLY,	// The message was handled - reply to the message
	RS_FAIL,	// Fail the message
	RS_NO_REPLY	// Do not reply to the message
} TtReplyStatus;

typedef TtReplyStatus (*TtMesgProc)(void		*client_data,
				    const Tt_message	msg);

typedef void (*TtReplyProc)(void 		*client_data,
			    const Tt_message	msg);


struct PATTERN_INFO {

	char	*pattern;
	STATUS	(*callback)(Tt_message message);
};


class TOOLTALK
{
    private:

	static	int		initial_stack_mark;
	static	char		*tt_session_id;
	static	char		*tt_procid;
	static	int		tt_file_descriptor;

	static void		*clientData;
	static TtMesgProc	mesgProc;
	static TtReplyProc	replyProc;
	static int		ttStackMark;
	static BOOL		reply_received;
	static BOOL		event_received;
	static STRING		last_message_op;
	static Tt_state		last_message_state;

	static BOOL		message_alliance_enabled;
	static Xv_opaque	base_frame;


	void		Dispatch(const Tt_message	msg);

	// Receive a message from the ToolTalk file descriptor
	//
	static Notify_func	ReceiveMessage(const Notify_client ptr,
					       const int	   fd);

	// Send the reply to a message that we received
	void		SendReply(const Tt_message	msg,
				  const TtReplyStatus	rstatus);

	void	Quit();
	TtReplyStatus	DispatchMessageAllianceMessage(Tt_message message);

	STATUS		HandleMessageAlliancePattern(Tt_message message);

    protected:

	// There can only be one connection to ToolTalk. This
	// requirement can be enforced by making this static
	//
	static OBJECT_STATE	objstate;

    public:

	virtual ~TOOLTALK();
	TOOLTALK();

	// Intialize - this must be called before this object can be used
	//
	STATUS		Init(const STRING	       &appname,
			     const int			argc,
			     char		      **argv,
			     ERRSTK		       &err);

	STATUS		RegisterDynamicPatterns(PATTERN_INFO	pattern_info[],
						int		pattern_count,
						Tt_category	category,
						ERRSTK		&err);

	Tt_message	CreateMessage(const STRING	&handlerProc,
				      const STRING	&handlerId,
				      const STRING	&msgName,
				      ERRSTK		&err);

	STATUS		SendMessage(const Tt_message msg, ERRSTK &err);


	// Set call back routines for messages
	//
	void		SetEventProcs(const TtMesgProc	msgProc,
				      const TtReplyProc	replyProc,
				      void  *const	clientData);

	// Enable receipt of Message Alliance patterns
	STATUS		EnableMessageAllianceProtocol(Xv_opaque arg_base_frame,
						      ERRSTK	&err);

	// These are the functions that handle Message Alliance pattern messages.

	// These functions are declared as static in case we ever get pattern
	// callbacks to work. In that case they will have to be static, and
	// pattern user data can be used to store the "this" pointer.
	// They really should be private but they are referenced in a data
	// structure so they are declared in the public section.

	static	STATUS	GetStatus(Tt_message		message);
	static	STATUS	SetEnvironment(Tt_message	message);
	static	STATUS	GetEnvironment(Tt_message	message);
	static	STATUS	SetGeometry(Tt_message		message);
	static	STATUS	GetGeometry(Tt_message		message);
	static	STATUS	SetIconified(Tt_message		message);
	static	STATUS	GetIconified(Tt_message		message);
	static	STATUS	SetMapped(Tt_message		message);
	static	STATUS	GetMapped(Tt_message		message);
	static	STATUS	Raise(Tt_message		message);

	// These are the functions for issuing Message Alliance commands
	STATUS	GetStatus(STRING	&process_name,
			  STRING	&procid,
			  STATUS (*callback)(STATUS	status,
					     STRING	&proc_status,
					     STRING	&vendor,
					     STRING	&name,
					     STRING	&relname),
			  ERRSTK	&err);
	STATUS	SetEnvironment(STRING	&process_name,
			       STRING	&procid,
			       STRING	&variable,
			       STRING	&value,
			       ERRSTK	&err);
	STATUS	GetEnvironment(STRING	&process_name,
			       STRING	&procid,
			       STRING	&variable,
			       STATUS	(*callback)(STATUS	status,
						    STRING	&variable,
						    STRING	&value),
			       ERRSTK	&err);
	STATUS	SetGeometry(STRING	&process_name,
			    STRING	&procid,
			    int		width,
			    int		height,
			    int		xoffset,
			    int		yoffset,
			    ERRSTK	&err);
	STATUS	GetGeometry(STRING	&process_name,
			    STRING	&procid,
			    STATUS	(*callback)(STATUS	status,
						    int		width,
						    int		height,
						    int		xoffset,
						    int		yoffset),
			    ERRSTK	&err);
	STATUS	SetIconified(STRING	&process_name,
			     STRING	&procid,
			     BOOL	close_it,
			     ERRSTK	&err);
	STATUS	GetIconified(STRING	&process_name,
			     STRING	&procid,
			     STATUS (*callback)(STATUS	status,
						BOOL	iconified),
			     ERRSTK	&err);
	STATUS	SetMapped(STRING	&process_name,
			  STRING	&procid,
			  BOOL		map_it,
			  ERRSTK	&err);
	STATUS	GetMapped(STRING	&process_name,
			  STRING	&procid,
			  STATUS (*callback)(STATUS	status,
					     BOOL	mapped),
			  ERRSTK	&err);
	STATUS	Raise(STRING	&process_name,
		      STRING	&procid,
		      ERRSTK	&err);

	STATUS	WaitForReply(const time_t	timeout,
			     STRING		&message_op,
			     Tt_state		&message_state,
			     ERRSTK		&err);

	STATUS	WaitForEvent(const time_t	timeout,
			     STRING		&message_op,
			     Tt_state		&message_state,
			     ERRSTK		&err);
};


// DocViewer-specific tooltalk messages.
// Defined here for convenience sake (we gotta do it somewhere).
//

// I used #define because they are used inside a struct initializer
//
#define	DOCVIEWER_TT_DEPARTING			"Departing"
#define	NAVIGATOR_TT_I_AM_YOUR_VIEWER		"I_Am_Your_Viewer"
#define	VIEWER_TT_VIEW_DOCUMENT			"View_Document"
#define	VIEWER_TT_VIEW_FILE			"View_File"
#define	VIEWER_TT_GET_CURRENT_DOCUMENT		"Get_Current_Document"
#define	VIEWER_TT_GET_CURRENT_FILE		"Get_Current_File"
#define	VIEWER_TT_SET_CARD_CATALOGS		"Set_Card_Catalogs"
#define	VIEWER_TT_SET_PREFERRED_LANGUAGE	"Set_Preferred_Language"
#define	VIEWER_TT_NEXT_PAGE			"Next_Page"
#define	VIEWER_TT_PREVIOUS_PAGE			"Previous_Page"
#define	VIEWER_TT_GO_BACK			"Go_Back"
#define	VIEWER_TT_CUSTOM_MAGNIFY		"Custom_Magnify"
#define	VIEWER_TT_PARTIAL_PAGE			"Partial_Page"
#define	VIEWER_TT_FULL_PAGE			"Full_Page"
#define	VIEWER_TT_PING				"Ping"

// The string value assigned to a out string status_string when there is no
// status to report (other then success)
extern const STRING	DV_TT_STATUS_OK;

// The string value assigned to an argument by ToolTalk when the original value
// is NULL_STRING
extern const STRING	TOOLTALK_NULL_STRING;


// Process information returned in Get_Status
extern const STRING	PROC_STATUS;
extern const STRING	VENDOR;
extern const STRING	RELEASE;


#ifdef	DEBUG

// Print the contents of a tt_message
//
extern void	PrintTtMessage(const STRING &string, const Tt_message msg);
extern ostream	&operator << (ostream &, const Tt_message);

#endif	/* DEBUG */

#endif	/* _OPT_TOOLTALK_H_ */
