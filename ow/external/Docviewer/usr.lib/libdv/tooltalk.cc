#ident "@(#)tooltalk.cc	1.25 02/15/93 Copyright 1992 Sun Microsystems, Inc."

#include <doc/tooltalk.h>
#include "dvlocale.h"


// Message Alliance tooltalk messages.
//
// I used #define because they are used inside a struct initializer
//
#define	MA_TT_GET_STATUS		"Get_Status"
#define	MA_TT_SET_ENVIRONMENT		"Set_Environment"
#define	MA_TT_GET_ENVIRONMENT		"Get_Environment"
#define	MA_TT_SET_GEOMETRY		"Set_Geometry"
#define	MA_TT_GET_GEOMETRY		"Get_Geometry"
#define	MA_TT_SET_ICONIFIED		"Set_Iconified"
#define	MA_TT_GET_ICONIFIED		"Get_Iconified"
#define	MA_TT_SET_MAPPED		"Set_Mapped"
#define	MA_TT_GET_MAPPED		"Get_Mapped"
#define	MA_TT_RAISE			"Raise"

const STRING	TOOLTALK_NULL_STRING("(null)");
const STRING	DV_TT_STATUS_OK("DV_OK");

const STRING	PROC_STATUS("Running");
const STRING	VENDOR("SunSoft");
const STRING	RELEASE("PreBeta 2.0");

const char	*	const	MESSAGE_ALLIANCE_IDENTIFIER("MessageAlliance");


int		TOOLTALK::initial_stack_mark = NULL;
char		*TOOLTALK::tt_session_id = NULL;
int		TOOLTALK::tt_file_descriptor = NULL;
char		*TOOLTALK::tt_procid = NULL;

void		*TOOLTALK::clientData	= NULL;
TtMesgProc	TOOLTALK::mesgProc	= NULL;
OBJECT_STATE	TOOLTALK::objstate;
TtReplyProc	TOOLTALK::replyProc	= NULL;
int		TOOLTALK::ttStackMark	= NULL;

BOOL		TOOLTALK::reply_received = BOOL_FALSE;
BOOL		TOOLTALK::event_received = BOOL_FALSE;
STRING		TOOLTALK::last_message_op = NULL_STRING;
Tt_state	TOOLTALK::last_message_state = TT_CREATED;

BOOL		TOOLTALK::message_alliance_enabled = BOOL_FALSE;
Xv_opaque	TOOLTALK::base_frame	= XV_NULL;


static	PATTERN_INFO	message_alliance_patterns[] =
		{
		    {MA_TT_GET_STATUS,		TOOLTALK::GetStatus},
		    {MA_TT_SET_ENVIRONMENT,	TOOLTALK::SetEnvironment},
		    {MA_TT_GET_ENVIRONMENT,	TOOLTALK::GetEnvironment},
		    {MA_TT_SET_GEOMETRY,	TOOLTALK::SetGeometry},
		    {MA_TT_GET_GEOMETRY,	TOOLTALK::GetGeometry},
		    {MA_TT_SET_ICONIFIED,	TOOLTALK::SetIconified},
		    {MA_TT_GET_ICONIFIED,	TOOLTALK::GetIconified},
		    {MA_TT_SET_MAPPED,		TOOLTALK::SetMapped},
		    {MA_TT_GET_MAPPED,		TOOLTALK::GetMapped},
		    {MA_TT_RAISE,		TOOLTALK::Raise},
		};

static	int	number_of_ma_patterns = 
		  (sizeof( message_alliance_patterns ) / sizeof( PATTERN_INFO ));



TOOLTALK::TOOLTALK()
{
	DbgFunc("TOOLTALK::TOOLTALK: entered" << endl);
	assert(objstate.IsNotReady());
}

TOOLTALK::~TOOLTALK()
{
	DbgFunc("TOOLTALK::~TOOLTALK: entered" << endl);

	Quit();
}

void
TOOLTALK::Quit()
{
	DbgFunc("TOOLTALK:Quit: entered" << endl);
	assert(objstate.IsReady());

	(void) tt_session_quit( tt_session_id );
	tt_release( initial_stack_mark );
	(void) tt_close();
	
	clientData	= NULL;
	mesgProc	= NULL;
	replyProc	= NULL;
	ttStackMark	= NULL;

	objstate.MarkUnusable();
}

STATUS
TOOLTALK::Init(const STRING    &name,
	       const int	argc,
	       char	      **argv,
	       ERRSTK	       &err)
{
	Tt_status	ttStatus;
	STATUS		status = STATUS_OK;

	DbgFunc("TOOLTALK::Init: entered" << endl);

	// This routine should be called only once
	assert(objstate.IsNotReady());

	objstate.MarkGettingReady();

	initial_stack_mark = tt_mark();
	tt_procid = tt_open();

	if ((ttStatus = tt_ptr_error( tt_procid )) == TT_OK) {
	    tt_file_descriptor = tt_fd();
	    tt_session_id = tt_default_session();

	    if ((ttStatus = tt_ptr_error( tt_session_id )) == TT_OK) {

		if ((ttStatus = tt_session_join( tt_session_id )) != TT_OK) {
		    status = STATUS_FAILED;
		    err.Init(DGetText("Could not initialize tooltalk (tt_session_join): %s"),
			     tt_status_message(ttStatus));
		}

	    }
	    else {
		status = STATUS_FAILED;
		err.Init(DGetText("Could not initialize tooltalk (tt_default_session): %s"),
			 tt_status_message(ttStatus));
	    }

	}
	else {
	    status = STATUS_FAILED;
	    err.Init(DGetText("Could not initialize tooltalk (tt_open): %s"),
		     tt_status_message(ttStatus));
	}

	if (status == STATUS_OK) {
		(void) notify_set_input_func((Notify_client) this,
					     (Notify_func) ReceiveMessage,
					     tt_file_descriptor);
	}
	else {
	    objstate.MarkUnusable();
	}

	return( status );
}

void
TOOLTALK::SetEventProcs(const TtMesgProc	argMesgProc,
			const TtReplyProc	argReplyProc,
			void *const		data)
{
	DbgFunc("TOOLTALK::SetEventProcs: entered" << endl);

	assert(objstate.IsReady());

	mesgProc = argMesgProc;
	replyProc = argReplyProc;
	clientData = data;

	return;
}

STATUS
TOOLTALK::EnableMessageAllianceProtocol(Xv_opaque arg_base_frame,
					ERRSTK	  &err)
{
	STATUS	status = STATUS_OK;

	DbgFunc("TOOLTALK::EnableMessageAllianceProtocol: entered" << endl);
	assert(objstate.IsReady());

	base_frame = arg_base_frame;
	status = RegisterDynamicPatterns(message_alliance_patterns,
					 number_of_ma_patterns,
					 TT_OBSERVE,
					 err);

	if (status == STATUS_OK) {
		message_alliance_enabled = BOOL_TRUE;
	}

	return( status );
}

STATUS
TOOLTALK::RegisterDynamicPatterns(PATTERN_INFO	pattern_info[],
				  int		pattern_count,
				  Tt_category	category,
				  ERRSTK	&err)
{
	int		index;
	Tt_pattern	pattern;
	STATUS		status = STATUS_OK;
	Tt_status	ttstatus;

	DbgFunc("TOOLTALK::RegisterDynamicPatterns: entered" << endl);

	for (index = 0;  index < pattern_count;  index++) {

	    pattern = tt_pattern_create();
	    ttstatus = tt_ptr_error( pattern );

	    if (ttstatus == TT_OK) {
		tt_pattern_scope_add(pattern, TT_SESSION);
		tt_pattern_session_add(pattern, tt_default_session());
		tt_pattern_category_set(pattern, category);
		tt_pattern_address_add(pattern, TT_PROCEDURE);
		tt_pattern_op_add(pattern,
				  pattern_info[index].pattern);

		ttstatus = tt_pattern_register( pattern );

		if (ttstatus != TT_OK) {
		    status = STATUS_FAILED;
		    err.Init(DGetText("Could not register tooltalk pattern: %s"),
						tt_status_message( ttstatus ));
		}

	    }

	    else {
		err.Init(DGetText("Could not create tooltalk pattern: %s"),
			 tt_status_message( ttstatus ));
	    }

	}

	return( status );
}

Notify_func
TOOLTALK::ReceiveMessage(const Notify_client	ptr,
			 const int		/* fd */)
{
	const Tt_message	message = tt_message_receive();

	DbgFunc("TOOLTALK::RecieveMessage: entered" << endl << message);

	// Once in a while we receive a bad message after Quiting, so this
	// assertion is problematic; do an if statement instead
//	assert(objstate.IsReady());
	if ( objstate.IsReady() ) {

		if (message) {
			TOOLTALK  *tooltalk = (TOOLTALK *) ptr;
			assert(tooltalk != NULL);

			tooltalk->Dispatch(message);
		}
	}

	return(NOTIFY_DONE);
}

void
TOOLTALK::Dispatch(const Tt_message	msg)
{
	TtReplyStatus	rstatus = RS_REJECT;
	const Tt_state	msgState = tt_message_state(msg);
	STATUS		status;

	DbgFunc("TOOLTALK::Dispatch: entered - msg = " <<
		(caddr_t) msg << ")" << endl);

	assert(objstate.IsReady());

	if (msgState == TT_SENT) {

		DbgLow("TOOLTALK::Dispatch: calling mesgProc" << endl);

		rstatus = mesgProc(clientData, msg);

		if ((rstatus == RS_NO_MATCH) && ( message_alliance_enabled )) {
			rstatus = DispatchMessageAllianceMessage( msg );
		}

		if (rstatus != RS_NO_REPLY) {
			SendReply(msg, rstatus);
		}

		event_received = BOOL_TRUE;
		last_message_op = tt_message_op( msg );;
		last_message_state = msgState;
	}
	else {
		DbgLow("TOOLTALK::Dispatch: got a reply" << endl);

		switch (msgState) {

		case TT_HANDLED:	// A reply to a msg. we sent
		case TT_FAILED:
		case TT_REJECTED:
			status = HandleMessageAlliancePattern( msg );

			if ((status == STATUS_FAILED) && (replyProc)) {
				replyProc(clientData, msg);
			}
			else {
				DbgHigh("TOOLTALK::Dispatch: replyProc " <<
					"not registered!?");
			}

			reply_received = BOOL_TRUE;
			last_message_op = tt_message_op( msg );;
			last_message_state = msgState;

			break;

		case TT_STARTED:
		case TT_CREATED:
		case TT_QUEUED:
			break;
		default:
			DbgLow("UNKNOWN STATE!" << endl);
			// we could add code like this if we wanted to recognize
			// the message received when ttsession dies.
			//ttstatus = tt_ptr_error( msg );

			//if (ttstatus == TT_ERR_NOMP) {
			//	fake_message = tt_message_create();
			//	(void) tt_message_op_set(fake_message,
			//			 TTSESSION_APPARENTLY_DIED);
			//	mesgProc(clientData, fake_message);
			//}
			//else {
			//	DbgLow("UNKNOWN STATE!" << endl);
			//	assert(0);
			//}
			break;
		}
	}

	if (msg != NULL) {
		tt_message_destroy(msg);
	}
	return;
}

TtReplyStatus
TOOLTALK::DispatchMessageAllianceMessage(Tt_message message)
{
	int		index = 0;
	BOOL		match_found = BOOL_FALSE;
	STRING		message_op;
	TtReplyStatus	reply_status = RS_NO_MATCH;
	STATUS		status;

	DbgFunc("TOOLTALK::DispatchMessageAllianceMessage: " << endl);

	message_op = tt_message_op( message );

	while ((index < number_of_ma_patterns) && ( !match_found )) {

		if (message_op == message_alliance_patterns[ index ].pattern) {
			match_found = BOOL_TRUE;
		}
		else {
			index++;
		}

	};

	if ( match_found ) {

		status =
		      (*message_alliance_patterns[ index ].callback)( message );

		if (status == STATUS_OK) {
			reply_status = RS_REPLY;
		}
		else {
			reply_status = RS_FAIL;
		}

	}

	return( reply_status );
}

STATUS
TOOLTALK::HandleMessageAlliancePattern(Tt_message message)
{
	STATUS		(*callback)(...);
	STATUS		callback_status = STATUS_OK;
	char		*identifier;
	STRING		message_op;
	Tt_state	message_state;
	STATUS		status = STATUS_FAILED;
	STRING		string_arg0;
	STRING		string_arg1;
	STRING		string_arg2;
	STRING		string_arg3;
	Tt_status	ttstatus;
	int		value0;
	int		value1;
	int		value2;
	int		value3;

	identifier = (char *) tt_message_user(message, 0);
	callback = (STATUS (*)(...))tt_message_user(message, 1);

	if ((identifier == MESSAGE_ALLIANCE_IDENTIFIER) &&
	    (callback != (void *) NULL)) {

		message_op = tt_message_op( message );
		message_state = tt_message_state( message );

		if (message_state != TT_HANDLED) {
			callback_status = STATUS_FAILED;
		}

		if (message_op == MA_TT_GET_STATUS) {
			string_arg0 = tt_message_arg_val(message, 0);
			string_arg1 = tt_message_arg_val(message, 1);
			string_arg2 = tt_message_arg_val(message, 2);
			string_arg3 = tt_message_arg_val(message, 3);
			status = (*callback)(callback_status,
						     string_arg0, string_arg1,
						     string_arg2, string_arg3);
		}

		else if (message_op == MA_TT_GET_ENVIRONMENT) {
			string_arg0 = tt_message_arg_val(message, 0);
			string_arg1 = tt_message_arg_val(message, 1);
			status = (*callback)(callback_status,
					     string_arg0, string_arg1);
		}

		else if (message_op == MA_TT_GET_GEOMETRY) {
			ttstatus = tt_message_arg_ival(message, 0, &value0);
			ttstatus = tt_message_arg_ival(message, 1, &value1);
			ttstatus = tt_message_arg_ival(message, 2, &value2);
			ttstatus = tt_message_arg_ival(message, 3, &value3);
			status = (*callback)(callback_status,
					     value0, value1,
					     value2, value3);
		}
		
		else if ((message_op == MA_TT_GET_ICONIFIED) ||
			 (message_op == MA_TT_GET_MAPPED)) {
			ttstatus = tt_message_arg_ival(message, 0, &value0);
			status = (*callback)(callback_status, (BOOL) value0);
		}

	}

	return( status );
}

void
TOOLTALK::SendReply(const Tt_message	message,
		    const TtReplyStatus	rstatus)
{
	Tt_status	ttStatus;

	DbgFunc("TOOLTALK::SendReply: entered" << endl);
	assert(objstate.IsReady());

	switch (rstatus) {

	case RS_REPLY:
		// Reply to message
		if ((ttStatus = tt_message_reply(message)) != TT_OK) {
			DbgHigh("Could not reply to message " <<
				tt_status_message(ttStatus) << endl);
		};

		break;

	case RS_REJECT:
	case RS_NO_MATCH:
		// Reject message
		if ((ttStatus = tt_message_reject(message)) != TT_OK) {
			DbgHigh("Could not reject message " << 
				tt_status_message(ttStatus) << endl);
		};
		break;

	case RS_FAIL:
		// Fail message
		if ((ttStatus = tt_message_fail(message)) != TT_OK) {
			DbgHigh("Could not fail message " <<
				tt_status_message(ttStatus) << endl);
		};

		break;

	case RS_NO_REPLY:
		// Do nothing
		break;
	}

	return;
}


// These are the functions that handle receipt of Message Alliance messages

STATUS
TOOLTALK::GetStatus(Tt_message message)
{
	char	*label;
	STATUS	status = STATUS_OK;

	DbgFunc("TOOLTALK::GetStatus: " << endl);
	tt_message_arg_val_set(message, 0, PROC_STATUS);
	tt_message_arg_val_set(message, 1, VENDOR);
	label = (char *) xv_get(base_frame, FRAME_LABEL);
	tt_message_arg_val_set(message, 2, label);
	tt_message_arg_val_set(message, 3, RELEASE);

	return( status );
}

STATUS
TOOLTALK::SetEnvironment(Tt_message message)
{
	int	length;
	char	*putenv_string;
	STATUS	status = STATUS_OK;
	char	*value;
	char	*variable;

	DbgFunc("TOOLTALK::SetEnvironment: " << endl);
	variable = tt_message_arg_val(message, 0);
	value = tt_message_arg_val(message, 1);

	length = strlen( variable ) + strlen( value ) + 2;
	putenv_string = new char[ length ];

	sprintf(putenv_string, "%s=%s", variable, value);
	putenv( putenv_string );

	return( status );
}

STATUS
TOOLTALK::GetEnvironment(Tt_message message)
{
	STATUS	status = STATUS_OK;
	STRING	value;
	STRING	variable;

	DbgFunc("TOOLTALK::GetEnvironment: " << endl);
	variable = tt_message_arg_val(message, 0);
	value = getenv( ~variable );
	tt_message_arg_val_set(message, 1, value);

	return( status );
}

STATUS
TOOLTALK::SetGeometry(Tt_message message)
{
	char		argument_char;
	STRING		argument_type;
	int		index;
	int		new_value;
	int		number_of_args;
	int		old_value;
	Rect		rect;
	STATUS		status = STATUS_OK;
	Tt_status	ttstatus;

	DbgFunc("TOOLTALK::SetGeometry: " << endl);

	frame_get_rect(base_frame, &rect);
	number_of_args = tt_message_args_count( message );

	for (index = 0;  index < number_of_args;  index++) {
	    argument_type = tt_message_arg_type(message, index);
	    argument_char = argument_type[ 0 ];

	    switch ( argument_char ) {

		case 'w' :
		    old_value = rect.r_width;
		    ttstatus = tt_message_arg_ival(message, index, &new_value);

		    if (ttstatus == TT_OK) {
			rect.r_width = new_value;
		    }
		    break;

		case 'h' :
		    old_value = rect.r_height;
		    ttstatus = tt_message_arg_ival(message, index, &new_value);

		    if (ttstatus == TT_OK) {
			rect.r_height = new_value;
		    }
		    break;

		case 'x' :
		    old_value = rect.r_left;
		    ttstatus = tt_message_arg_ival(message, index, &new_value);

		    if (ttstatus == TT_OK) {
			rect.r_left = new_value;
		    }
		    break;

		case 'y' :
		    old_value = rect.r_top;
		    ttstatus = tt_message_arg_ival(message, index, &new_value);

		    if (ttstatus == TT_OK) {
			rect.r_top = new_value;
		    }
		    break;

		default  :
		    old_value = -1;
		    break;
	    }

	    if (old_value >= 0) {
		tt_message_arg_ival_set(message, index, old_value);
	    }

	}

	frame_set_rect(base_frame, &rect);

	return( status );
}

STATUS
TOOLTALK::GetGeometry(Tt_message message)
{
	char		argument_char;
	STRING		argument_type;
	int		index;
	int		number_of_args;
	Rect		rect;
	STATUS		status = STATUS_OK;
	int		value;

	DbgFunc("TOOLTALK::GetGeometry: " << endl);

	frame_get_rect(base_frame, &rect);
	number_of_args = tt_message_args_count( message );

	for (index = 0;  index < number_of_args;  index++) {
		argument_type = tt_message_arg_type(message, index);
		argument_char = argument_type[ 0 ];

		switch ( argument_char ) {

			case 'w' : value = rect.r_width;	break;
			case 'h' : value = rect.r_height;	break;
			case 'x' : value = rect.r_left;		break;
			case 'y' : value = rect.r_top;		break;

			default  : value = -1;			break;
		}

		if (value >= 0) {
			tt_message_arg_ival_set(message, index, value);
		}

	}

	return( status );
}

STATUS
TOOLTALK::SetIconified(Tt_message message)
{
	int	close_it;
	STATUS	status = STATUS_OK;

	DbgFunc("TOOLTALK::SetIconified: " << endl);
	tt_message_arg_ival(message, 0, &close_it);
	xv_set(base_frame, FRAME_CLOSED, close_it, NULL);

	return( status );
}

STATUS
TOOLTALK::GetIconified(Tt_message message)
{
	int	closed;
	STATUS	status = STATUS_OK;

	DbgFunc("TOOLTALK::GetIconified: " << endl);
	closed = (int) xv_get(base_frame, FRAME_CLOSED);
	tt_message_arg_ival_set(message, 0, closed);

	return( status );
}

STATUS
TOOLTALK::SetMapped(Tt_message message)
{
	int	mapped;
	STATUS	status = STATUS_OK;

	DbgFunc("TOOLTALK::SetMapped: " << endl);
	tt_message_arg_ival(message, 0, &mapped);
	xv_set(base_frame, XV_SHOW, mapped, NULL);

	return( status );
}

STATUS
TOOLTALK::GetMapped(Tt_message message)
{
	int	mapped;
	STATUS	status = STATUS_OK;

	DbgFunc("TOOLTALK::GetMapped: " << endl);
	mapped = (int) xv_get(base_frame, XV_SHOW);
	tt_message_arg_ival_set(message, 0, mapped);

	return( status );
}

STATUS
TOOLTALK::Raise(Tt_message message)
{
	int	mapped;
	STATUS	status = STATUS_OK;

	DbgFunc("TOOLTALK::Raise: " << endl);
	xv_set(base_frame, WIN_FRONT, NULL);

	return( status );
}


// These are the functions for sending Message Alliance messages


STATUS
TOOLTALK::GetStatus(STRING	&process_name,
		    STRING	&procid,
		    STATUS	(*callback)(STATUS	status,
					    STRING	&proc_status,
					    STRING	&vendor,
					    STRING	&progname,
					    STRING	&release),
		    ERRSTK	&err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	DbgFunc("TOOLTALK::GetStatus: " << endl);
	ttmsg = CreateMessage(	process_name,
				procid,
				MA_TT_GET_STATUS,
				err);
	if (ttmsg == NULL) {
		return(STATUS_FAILED);
	}

	status = tt_message_iarg_add(ttmsg, TT_OUT, "string", NULL);
	status = tt_message_iarg_add(ttmsg, TT_OUT, "string", NULL);
	status = tt_message_iarg_add(ttmsg, TT_OUT, "string", NULL);
	status = tt_message_iarg_add(ttmsg, TT_OUT, "string", NULL);
	status = tt_message_user_set(ttmsg, 0,
				     (void *) MESSAGE_ALLIANCE_IDENTIFIER);
	status = tt_message_user_set(ttmsg, 1,
				     (void *) callback);

	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send %s message to %s"),
			 MA_TT_GET_STATUS, ~process_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TOOLTALK::SetEnvironment(STRING	&process_name,
			 STRING	&procid,
			 STRING	&variable,
			 STRING	&value,
			 ERRSTK	&err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	ttmsg = CreateMessage(	process_name,
				procid,
				MA_TT_SET_ENVIRONMENT,
				err);
	if (ttmsg == NULL) {
		return(STATUS_FAILED);
	}

	status = tt_message_arg_add(ttmsg, TT_IN, "string", variable);
	status = tt_message_arg_add(ttmsg, TT_IN, "string", value);
	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send %s message to %s"),
			 MA_TT_SET_ENVIRONMENT, ~process_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TOOLTALK::GetEnvironment(STRING	&process_name,
			 STRING	&procid,
			 STRING	&variable,
			 STATUS	(*callback)(STATUS	status,
					    STRING	&variable,
					    STRING	&value),
			 ERRSTK	&err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	DbgFunc("TOOLTALK::GetEnvironment: " << endl);
	ttmsg = CreateMessage(	process_name,
				procid,
				MA_TT_GET_ENVIRONMENT,
				err);
	if (ttmsg == NULL) {
		return(STATUS_FAILED);
	}

	status = tt_message_arg_add(ttmsg, TT_IN, "string", variable);
	status = tt_message_arg_add(ttmsg, TT_OUT, "string", NULL);
	status = tt_message_user_set(ttmsg, 0,
				     (void *) MESSAGE_ALLIANCE_IDENTIFIER);
	status = tt_message_user_set(ttmsg, 1,
				     (void *) callback);

	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send %s message to %s"),
			 MA_TT_GET_ENVIRONMENT, ~process_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TOOLTALK::SetGeometry(STRING	&process_name,
	 	      STRING	&procid,
		      int	width,
		      int	height,
		      int	xoffset,
		      int	yoffset,
		      ERRSTK	&err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	ttmsg = CreateMessage(	process_name,
				procid,
				MA_TT_SET_GEOMETRY,
				err);
	if (ttmsg == NULL) {
		return(STATUS_FAILED);
	}

	status = tt_message_iarg_add(ttmsg, TT_INOUT, "width", width);
	status = tt_message_iarg_add(ttmsg, TT_INOUT, "height", height);
	status = tt_message_iarg_add(ttmsg, TT_INOUT, "xOffset", xoffset);
	status = tt_message_iarg_add(ttmsg, TT_INOUT, "yOffset", yoffset);
	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send %s message to %s"),
			 MA_TT_SET_GEOMETRY, ~process_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TOOLTALK::GetGeometry(STRING	&process_name,
		      STRING	&procid,
		      STATUS	(*callback)(STATUS	status,
			  		    int		width,
				  	    int		height,
					    int		xoffset,
					    int		yoffset),
		      ERRSTK	&err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	DbgFunc("TOOLTALK::GetGeometry: " << endl);
	ttmsg = CreateMessage(	process_name,
				procid,
				MA_TT_GET_GEOMETRY,
				err);
	if (ttmsg == NULL) {
		return(STATUS_FAILED);
	}

	status = tt_message_iarg_add(ttmsg, TT_OUT, "width", NULL);
	status = tt_message_iarg_add(ttmsg, TT_OUT, "height", NULL);
	status = tt_message_iarg_add(ttmsg, TT_OUT, "xOffset", NULL);
	status = tt_message_iarg_add(ttmsg, TT_OUT, "yOffset", NULL);
	status = tt_message_user_set(ttmsg, 0,
				     (void *) MESSAGE_ALLIANCE_IDENTIFIER);
	status = tt_message_user_set(ttmsg, 1,
				     (void *) callback);

	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send %s message to %s"),
			 MA_TT_GET_GEOMETRY, ~process_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TOOLTALK::SetIconified(STRING	&process_name,
		       STRING	&procid,
		       BOOL	close_it,
		       ERRSTK	&err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	ttmsg = CreateMessage(	process_name,
				procid,
				MA_TT_SET_ICONIFIED,
				err);
	if (ttmsg == NULL) {
		return(STATUS_FAILED);
	}

	status = tt_message_iarg_add(ttmsg, TT_INOUT, "boolean", close_it);
	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send %s message to %s"),
			 MA_TT_SET_ICONIFIED, ~process_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TOOLTALK::GetIconified(STRING	&process_name,
		       STRING	&procid,
		       STATUS	(*callback)(STATUS	status,
					    BOOL	iconified),
		       ERRSTK	&err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	DbgFunc("TOOLTALK::GetIconified: " << endl);
	ttmsg = CreateMessage(	process_name,
				procid,
				MA_TT_GET_ICONIFIED,
				err);
	if (ttmsg == NULL) {
		return(STATUS_FAILED);
	}

	status = tt_message_iarg_add(ttmsg, TT_OUT, "boolean", NULL);
	status = tt_message_user_set(ttmsg, 0,
				     (void *) MESSAGE_ALLIANCE_IDENTIFIER);
	status = tt_message_user_set(ttmsg, 1,
				     (void *) callback);

	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send %s message to %s"),
			 MA_TT_GET_ICONIFIED, ~process_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TOOLTALK::SetMapped(STRING	&process_name,
		    STRING	&procid,
		    BOOL	mapped,
		    ERRSTK	&err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	ttmsg = CreateMessage(	process_name,
				procid,
				MA_TT_SET_MAPPED,
				err);
	if (ttmsg == NULL) {
		return(STATUS_FAILED);
	}

	status = tt_message_iarg_add(ttmsg, TT_INOUT, "boolean", mapped);
	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send %s message to %s"),
			 MA_TT_SET_MAPPED, ~process_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TOOLTALK::GetMapped(STRING	&process_name,
		    STRING	&procid,
		    STATUS	(*callback)(STATUS	status,
					    BOOL	mapped),
		    ERRSTK	&err)
{
	Tt_message	ttmsg;
	Tt_status	status;

	DbgFunc("TOOLTALK::GetMapped: " << endl);
	ttmsg = CreateMessage(	process_name,
				procid,
				MA_TT_GET_MAPPED,
				err);
	if (ttmsg == NULL) {
		return(STATUS_FAILED);
	}

	status = tt_message_iarg_add(ttmsg, TT_OUT, "boolean", NULL);
	status = tt_message_user_set(ttmsg, 0,
				     (void *) MESSAGE_ALLIANCE_IDENTIFIER);
	status = tt_message_user_set(ttmsg, 1,
				     (void *) callback);

	if (status != TT_OK) {
		err.Init(DGetText("Can't create ToolTalk message: %s"),
			tt_status_message(status));
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err)  !=  STATUS_OK) {
		err.Push(DGetText("Could not send %s message to %s"),
			 MA_TT_GET_MAPPED, ~process_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TOOLTALK::Raise(STRING	&process_name,
		STRING	&procid,
		ERRSTK	&err)
{
	Tt_message	ttmsg;

	ttmsg = CreateMessage(	process_name,
				procid,
				MA_TT_RAISE,
				err);
	if (ttmsg == NULL) {
		return(STATUS_FAILED);
	}

	if (SendMessage(ttmsg, err) != STATUS_OK) {
		err.Push(DGetText("Could not send %s message to %s"),
			 MA_TT_RAISE, ~process_name);
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

STATUS
TOOLTALK::WaitForReply(const time_t	timeout,
		       STRING		&message_op,
		       Tt_state		&message_state,
		       ERRSTK		&err)
{
	const time_t	start	= time(NULL);
	STATUS		status	= STATUS_OK;

	extern STATUS	XvSleep(int secs, ERRSTK &err);

	DbgFunc("TOOLTALK::WaitForReply" << endl);

	// This logic assumes the reply has not been received yet, we assume
	// it can not be until notify_dispatch allows the notify function
	// to be invoked with an incoming reply (see Init() and ReplyHandler())
	reply_received = BOOL_FALSE;

	if (start < 0) {
		status = STATUS_FAILED;
		err.Init(SysErrMsg(errno));
	}
	else {
		do {
			(void) XvSleep(1, err);
			(void) notify_dispatch();
		} while (!reply_received && (time(NULL) - start) <= timeout);

		if (!reply_received) {
			err.Init(DGetText("request timed out %d seconds"),
				 timeout);
			status = STATUS_FAILED;
		}
	}

	message_op = last_message_op;
	message_state = last_message_state;
	return (status);
}

STATUS
TOOLTALK::WaitForEvent(const time_t	timeout,
		       STRING		&message_op,
		       Tt_state		&message_state,
		       ERRSTK		&err)
{
	const time_t	start	= time(NULL);
	STATUS		status	= STATUS_OK;

	extern STATUS	XvSleep(int secs, ERRSTK &err);

	DbgFunc("TOOLTALK::WaitForEvent" << endl);

	// This logic assumes the reply has not been received yet, we assume
	// it can not be until notify_dispatch allows the notify function
	// to be invoked with an incoming reply (see Init() and ReplyHandler())
	event_received = BOOL_FALSE;

	if (start < 0) {
		status = STATUS_FAILED;
		err.Init(SysErrMsg(errno));
	}
	else {
		do {
			(void) XvSleep(1, err);
			(void) notify_dispatch();
		} while (!event_received && (time(NULL) - start) <= timeout);

		if (!event_received) {
			err.Init(DGetText("request timed out %d seconds"),
				 timeout);
			status = STATUS_FAILED;
		}
	}

	message_op = last_message_op;
	message_state = last_message_state;
	return (status);
}

Tt_message
TOOLTALK::CreateMessage(const STRING		&application,
			const STRING		&handlerId,
			const STRING		&msgName,
			ERRSTK			&err)
{
	Tt_status	ttStatus;
	Tt_message	msg;

	DbgFunc("TOOLTALK::CreateMessage: entered" << endl);

	// Get a mark on the tooltalk stack so that we can release it
	// all once with tt_release() in SendMessage()
	//
	if (ttStackMark) {
		tt_release(ttStackMark);
	}

	ttStackMark = tt_mark();

	msg = tt_message_create();

	if ((ttStatus = tt_ptr_error( msg )) == TT_OK) {
	    tt_message_address_set(msg, TT_HANDLER);
	    tt_message_class_set(msg, TT_REQUEST);
	    tt_message_scope_set(msg, TT_SESSION);
	    tt_message_session_set(msg, tt_session_id);
	    tt_message_op_set(msg, msgName);
	    tt_message_disposition_set(msg, TT_DISCARD);

	    // Fill in the handler with the receiving application's
	    // tooltalk handler id
	    //
	    if (handlerId != NULL_STRING) {
		(void) tt_message_handler_set(msg, handlerId);
		ttStatus = tt_message_address_set(msg, TT_HANDLER);
	    }
	    else {
		ttStatus = tt_message_address_set(msg, TT_PROCEDURE);
	    }

	    // Check that the message is valid
	    //
	    if (ttStatus != TT_OK) {
		msg = NULL;
		err.Init(DGetText("Could not create tooltalk message: %s"),
			 tt_status_message(ttStatus));
	    }

	}
	else {
	    err.Init(DGetText("Could not create tooltalk message: %s"),
		     tt_status_message(ttStatus));

	}

	return(msg);
}

STATUS
TOOLTALK::SendMessage(const Tt_message msg, ERRSTK &err)
{
	STATUS		status = STATUS_OK;
	STRING		status_message;
	Tt_status	ttStatus;

	// Try to send the message and see if it worked
	if ((ttStatus = tt_message_send(msg)) != TT_OK) {
		DbgHigh("TOOLTALK::SendMessage: " <<
			"tt_message_send() failed: ttStatus = " <<
			ttStatus << endl);

		if (ttStatus == TT_ERR_NOMP) {
			status_message = DGetText("Could not send tooltalk message, no ttsession is running.\n");
			status_message += DGetText("Please consider re-starting the application.");
			err.Init( status_message );
		}
		else {
			err.Init(DGetText("tooltalk error: %s"),
						 tt_status_message(ttStatus));
		}

		status = STATUS_FAILED;
	}

	// Release all memory we allocated since we last called
	// CreateMessage()
	//
	tt_release(ttStackMark);
	ttStackMark = NULL;

	return (status);
}


#ifdef	DEBUG
void
PrintTtMessage(const STRING &string, const Tt_message msg)
{
	int	mstatus;
	STRING	state;
	STRING	errMsg;

	switch (tt_message_state(msg)) {
	case TT_SENT:
		state = "TT_SENT";
		break;
	case TT_FAILED:
		state = "TT_FAILED";
		break;
	case TT_REJECTED:
		state = "TT_REJECTED";
		break;
	case TT_CREATED:
		state = "TT_CREATED";
		break;
	case TT_HANDLED:
		state = "TT_HANDLED";
		break;
	case TT_QUEUED:
		state = "TT_QUEUED";
		break;
	case TT_STARTED:
		state = "TT_STARTED";
		break;
	default:
		state = "Unknown State";
		break;
	}

	if (tt_int_error((mstatus = tt_message_status(msg))) == TT_OK)
		errMsg = NULL_STRING;
	else
		errMsg = tt_message_status_string(msg);

	if (string && string != NULL_STRING) {
		fprintf(stderr, "%-20s: ", string);
	}
	
	fprintf(stderr, "%#8x %-15s %s %s (%d): %s\n",
		(caddr_t) msg, tt_message_op(msg),
		~state, ~errMsg,
		mstatus, tt_message_handler(msg));

	return;
}

ostream &
operator << (ostream &ostr, const Tt_message msg)
{
	STRING		state;
	Tt_status	status;

	switch (tt_message_state(msg)) {
	case TT_SENT:
		state = "TT_SENT";
		break;
	case TT_FAILED:
		state = "TT_FAILED";
		break;
	case TT_REJECTED:
		state = "TT_REJECTED";
		break;
	case TT_CREATED:
		state = "TT_CREATED";
		break;
	case TT_HANDLED:
		state = "TT_HANDLED";
		break;
	case TT_QUEUED:
		state = "TT_QUEUED";
		break;
	case TT_STARTED:
		state = "TT_STARTED";
		break;
	default:
		state = "Unknown State";
		break;
	}


	ostr	<< tt_message_op(msg)
		<< ": state="	<< state
		<< ", handler="	<< tt_message_handler(msg);

	if ((status = tt_int_error(tt_message_status(msg))) != TT_OK) {
		ostr	<< ", status="	<< status
			<< ", errmsg="	<< tt_message_status_string(msg);
	} else {
		ostr	<< ", status=TT_OK";
	}

	return(ostr << endl);
}
#endif	DEBUG
