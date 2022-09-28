#ident "@(#)notify.cc	1.30 93/10/12 Copyright 1992 Sun Microsystems, Inc."

#include <stdarg.h>

// XXX - hack hack hack!
// This is a trick to prevent cpp from including <varargs.h>.
// The problem is that <stdarg.h> and <varargs.h>
// have different definitions for the same varargs
// macros.  We need the <stdargs.h> versions here,
// but xview #includes the <varargs.h> versions.
// So here we are.
//
#define	_sys_varargs_h	//XXX hack, hack, hack!

#include <doc/notify.h>
#include <doc/itimer.h>
#include <xview/frame.h>
#include <xview/notice.h>
#ifdef SVR4
#include "dvlocale.h"
#endif


NOTIFY::NOTIFY(Xv_opaque parent) :
	frame		(parent),
	msg_timeout	(5),
	busy_count	(0),
	itimer		(NULL),
	mode_message	("")
{
	assert(frame != NULL);
	DbgFunc("NOTIFY constructor" << endl);

	xv_set(frame,
		FRAME_SHOW_HEADER,	TRUE,
		FRAME_SHOW_FOOTER,	TRUE,
		NULL);
}

NOTIFY::~NOTIFY()
{
	DbgFunc("NOTIFY destructor" << endl);
	xv_destroy_safe((Xv_opaque)frame);
	if (itimer)
		delete itimer;
}

void
NOTIFY::Title(const char *message ...)
{
	va_list	args;

	assert(message != NULL);
	DbgFunc("NOTIFY::Title: " << message << endl);

	va_start(args, message);
	vsprintf(msgbuf, message, args);
	va_end(args);

	xv_set(frame,
		FRAME_LABEL,	msgbuf,
		NULL);
}

void
NOTIFY::Mode(const char *message ...)
{
	va_list	args;

	assert(message != NULL);
	DbgFunc("NOTIFY::Mode: " << message << endl);

	va_start(args, message);
	vsprintf(msgbuf, message, args);
	va_end(args);

	xv_set(frame,
		FRAME_LEFT_FOOTER, msgbuf,
		NULL);

	mode_message = msgbuf;
}

void
NOTIFY::StatusLine(const char *message ...)
{
	va_list	args;

	assert(message != NULL);
	DbgFunc("NOTIFY::StatusLine: " << message << endl);

	va_start(args, message);
	vsprintf(msgbuf, message, args);
	va_end(args);

	xv_set(frame,
		FRAME_LEFT_FOOTER, msgbuf,
		NULL);

	// Set a timer that will come back and replace the mode message
	// after a few seconds.
	//
	if (itimer == NULL  &&  (itimer = new ITIMER()) == NULL)
		return;

	itimer->TimeOut(msg_timeout,
			(TimerCallBack) NOTIFY::MsgTimeOutEvent,
			(caddr_t) this);
}

void
NOTIFY::Warning(const char *message ...)
{
	va_list	args;

	assert(message != NULL);
	DbgFunc("NOTIFY::Warning: " << message << endl);

	va_start(args, message);
	vsprintf(msgbuf, message, args);
	va_end(args);

	xv_set(frame,
		FRAME_LEFT_FOOTER, msgbuf,
		WIN_ALARM,
		NULL);

	// Set a timer that will come back and replace the mode message
	// after a few seconds.
	//
	if (itimer == NULL  &&  (itimer = new ITIMER()) == NULL)
		return;

	itimer->TimeOut(msg_timeout,
			(TimerCallBack) NOTIFY::MsgTimeOutEvent,
			(caddr_t) this);
}

void
NOTIFY::Alert(const char *message ...)
{
	va_list	args;
	Event	event;

	assert(message != NULL);
	DbgFunc("NOTIFY::Alert: " << message << endl);

	va_start(args, message);
	vsprintf(msgbuf, message, args);
	va_end(args);

	notice_prompt(frame, &event,
			NOTICE_MESSAGE_STRING,	msgbuf,
			NOTICE_BUTTON_YES,	DGetText("Continue"),
			NULL);
}

BOOL
NOTIFY::AlertPrompt(const char *yes, const char *no, const char *message ...)
{
	va_list	args;
	Event	event;
	int	answer;

	assert(message != NULL);
	assert(yes != NULL  &&  no != NULL);
	DbgFunc("NOTIFY::AlertPrompt: " << message << endl);

	va_start(args, message);
	vsprintf(msgbuf, message, args);
	va_end(args);

	answer = notice_prompt(frame, &event,
			NOTICE_MESSAGE_STRING,	msgbuf,
			NOTICE_BUTTON_YES,	yes,
			NOTICE_BUTTON_NO,	no,
			NULL);

	return(answer == NOTICE_YES ? BOOL_TRUE : BOOL_FALSE);
}

void
NOTIFY::ShowErrs(ERRSTK &err)
{
	const char	*errlist[20];
	Event		event;
	int		i;

	DbgFunc("NOTIFY::ShowErrs" << endl);

	for (i = 0; i < 19  &&  ! err.IsEmpty(); i++)
		errlist[i] = err.Pop();
	errlist[i] = NULL;

	if ( i != 0)
		notice_prompt(	frame, &event,
			NOTICE_MESSAGE_STRINGS_ARRAY_PTR,	errlist,
			NOTICE_BUTTON_YES,	DGetText("Continue"),
			NULL);
}

void
NOTIFY::Busy(const char *message ...)
{
	va_list	args;

	assert(message != NULL);
	DbgFunc("NOTIFY::Busy: " << message << endl);

	va_start(args, message);
	vsprintf(msgbuf, message, args);
	va_end(args);

	xv_set(frame,
		FRAME_BUSY,		TRUE,
		FRAME_LEFT_FOOTER,	msgbuf,
		NULL);

	++busy_count;
}

void
NOTIFY::Done()
{
	DbgFunc("NOTIFY::Done" << endl);

	if (busy_count > 0) {

		if (--busy_count == 0) {
			xv_set(frame,
				FRAME_BUSY,		FALSE,
				FRAME_LEFT_FOOTER,	~mode_message,
				NULL);
		}

	}
}

void
NOTIFY::MsgTimeOutEvent(caddr_t client_data)
{
	NOTIFY	*notify;


	DbgFunc("NOTIFY::MsgTimeOutEvent" << endl);

	notify = (NOTIFY *) client_data;
	xv_set(	notify->frame,
		FRAME_LEFT_FOOTER,	~notify->mode_message,
		NULL);
}
