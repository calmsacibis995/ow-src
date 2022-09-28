#ifndef	_NOTIFY_H
#define	_NOTIFY_H

#ident "@(#)notify.h	1.18 06/11/93 Copyright 1990-1992 Sun Microsystems, Inc."

#include "common.h"
#include <xview/xview.h>

#define	MSGLEN	1024	// max total length of message, including arguments
#define	FMTLEN	256	// max length of message, excluding arguments


class	ITIMER;		// foward reference


class	NOTIFY {

    private:

	STRING		mode_message;
	Xv_opaque	frame;
	char		msgbuf[MSGLEN];

	// Keep track of how "busy" we are.
	//
	int		busy_count;

	// Handler for timeout events
	// (e.g., resetting mode message after a few seconds).
	//
	ITIMER		*itimer;
	static void	MsgTimeOutEvent(caddr_t client_data);

	// How long to display warning messages.
	//
	int		msg_timeout;


    public:

	NOTIFY(Xv_opaque frame);
	~NOTIFY();

	// Display title.
	//
	void	Title(const char *message ...);

	// Display semi-permanent message describing
	// current operational state or context.
	//
	void	Mode(const char *message ...);

	// Display status message.
	//
	void	StatusLine(const char *message ...);

	// Display warning message.  Beep.
	//
	void	Warning(const char *message ...);

	// Alert the user to a condition outside of their control
	// that affects the operation of their application.
	//
	void	Alert(const char *message ...);

	// Alert the user to a condition outside of their control
	// that affects the operation of their application.
	// Prompt them for a yes or no answer.
	// Returns BOOL_TRUE if user answers 'yes', else returns BOOL_FALSE.
	//
	BOOL	AlertPrompt(const char *y, const char *n, const char *msg ...);

	// Display contents of error stack.
	//
	void	ShowErrs(ERRSTK &err);

	// Notify the user that something may take a while ...
	//
	void	Busy(const char *message ...);
	void	Done();
};

#endif	_NOTIFY_H
