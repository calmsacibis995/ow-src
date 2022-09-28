#ifndef	_ERRSTK_H
#define	_ERRSTK_H

#ident "@(#)errstk.h	1.7 06/11/93 Copyright 1992 Sun Microsystems, Inc."


#include <doc/common.h>
#include <locale.h>
#include <errno.h>


// ERRSTK: a stack-oriented error handling mechanism.
//
// The model for using ERRSTK objects is as follows:
//
//	o At whatever level in your application you decide to
//	  handle the display of error messages, declare an ERRSTK
//	  object as a local stack variable.
//
//	o Pass a reference to this ERRSTK object down the call tree.
//	  In the subroutine or method where a an error is first detected,
//	  call "Init()" to set the error message
//	  describing the lowest-level details of the error.
//
//	o As the error condition propagates back up the call tree,
//	  "Push()" additional error messages onto the stack
//	  as appropriate to provide context for the initial error condition.
//
//	o Back at the top level where the error is actually handled,
//	  use "Pop()" to access the error information
//	  for presentation to the user.
//
// For example:
//
//	main(int, char **)
//	{
//		ERRSTK	error;
//		STRING		msg;
//		STATUS		foo(ERRSTK &);
//
//		if (foo(error) != STATUS_OK) {
//			while ((msg = error.Pop())  !=  NULL_STRING)
//				cout << msg << endl;
//		}
//	}
//
//	STATUS
//	foo(ERRSTK &error)
//	{
//		STATUS		bar(ERRSTK &);
//
//		if (bar(error) != STATUS_OK) {
//			error.Push("Can't foo because of bar problems");
//			return(STATUS_FAILED);
//		} else {
//			return(STATUS_OK);
//		}
//	}
//
//	STATUS
//	bar(ERRSTK &error)
//	{
//		if (1 != 1) {
//			error.Init("We're in big trouble here, folks");
//			return(STATUS_FAILED);
//		} else {
//			return(STATUS_OK);
//		}
//	}
//

#define	ERRSTK_SIZ	20

class	ERRSTK {

    private:

	STRING		msg_stack[ERRSTK_SIZ];	// stack of error messages
	int		top_of_stack;		// next avail. stack location


    public:

	ERRSTK()	{ top_of_stack = 0; }
	~ERRSTK()	{ }

	// Initialize this error stack.
	// An error has been detected at its source.
	// Clear the stack and push 'msg' onto the stack.
	// 'msg' is a format string plus arguments ala "printf".
	//
	void		Init(const char *msg ...);

	// Add an error to the stack.
	// 'msg' is a format string plus arguments ala "printf".
	// The stack must already have been initialized via "Init()".
	//
	void		Push(const char *msg ...);

	// Get the next top-most error from the stack.
	// Assumes stack is non-empty.
	// Returns a reference to the message STRING.
	//
	const STRING	&Pop();

	// Get the number of messages on the stack.
	//
	int		Depth() const	{ return(top_of_stack); }

	// Clear the stack.
	//
	void		Clear()		{ top_of_stack = 0; }

	// Is stack empty?
	//
	BOOL		IsEmpty() const	{ return((BOOL)(Depth() == 0)); }

	// Dump stack contents.
	//
	friend ostream	&operator << (ostream &out, const ERRSTK &err);
};


//
//
const char	*SysErrMsg(int error);

#endif	_ERRSTK_H
