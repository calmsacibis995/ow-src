#ident "@(#)errstk.cc	1.8 93/02/15 Copyright 1992 Sun Microsystems, Inc."


#include <doc/errstk.h>
#include <stdarg.h>


#define MAXMSG_LEN	200


// Initialize this error stack.
// An error has been detected at its source.
// Clear the stack, set the error code to 'error',
// and push 'msg' onto the stack.
// 'msg' is a format string plus arguments ala "printf".
//
void
ERRSTK::Init(const char *msg ...)
{
	va_list		args;
	char		msgbuf[2*MAXMSG_LEN];


	Clear();

	va_start(args, msg);
	vsprintf(msgbuf, msg, args);
	va_end(args);

	msgbuf[MAXMSG_LEN] = '\0';
	msg_stack[top_of_stack++] = msgbuf;
	DbgFunc("ERRSTK::Init: " << msgbuf << endl);
}

// Add an error to the stack.
// 'msg' is a format string plus arguments ala "printf".
// The stack must already have been initialized via "Init()".
//
void
ERRSTK::Push(const char *msg ...)
{
	va_list		args;
	char		msgbuf[2*MAXMSG_LEN];


	assert(Depth() > 0  &&  Depth() < ERRSTK_SIZ);


	va_start(args, msg);
	vsprintf(msgbuf, msg, args);
	va_end(args);

	msgbuf[MAXMSG_LEN] = '\0';
	msg_stack[top_of_stack++] = msgbuf;
	DbgFunc("ERRSTK::Push: " << msgbuf << endl);
}

// Get the next top-most error from the stack.
// Returns a reference to the message STRING, or NULL_STRING
// if there are no more messages on the stack.
//
const STRING &
ERRSTK::Pop()
{
	assert( ! IsEmpty());

	return(msg_stack[--top_of_stack]);
}

// Dump stack contents.
//
ostream &
operator << (ostream &out, const ERRSTK &err)
{
	STRING	indent;
	int	i;

	for (i = err.top_of_stack-1; i >= 0; i--) {
		out << indent << err.msg_stack[i] << endl;
		indent += "   ";
	}

	return(out);
}

const char *
SysErrMsg(int error)
{
	static STRING	msg;

	if ((msg = strerror(error))  ==  NULL_STRING) {
		return("(no error)");
	} else {
		return(msg);
	}
}
