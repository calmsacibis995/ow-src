#ifndef	_CONSOLE_H
#define	_CONSOLE_H

#ident "@(#)console.h	1.4 06/11/93 Copyright 1990 Sun Microsystems, Inc."


#include <doc/common.h>

// Console device for printing error messages that can't be
// displayed in a window or by some other more appropriate means.
// This should be considered the method of last resort for displaying
// error messages!
//
class	CONSOLE {

    private:

	// Name of current program/application.
	//
	STRING	program_name;

	// Has 'Init()' been called successfully?
	//
	BOOL	is_inited;


    public:

	CONSOLE()	{ }
	~CONSOLE()	{ }

	// Initialize console.
	// This must be done before invoking any other CONSOLE methods.
	//
	STATUS		Init(const STRING &prog_name=NULL_STRING);
	BOOL		IsInited()		{ return(is_inited); }

	// Print a "printf-style" (varargs) message on the console.
	//
	void		Message(const char *message ...);

	// Print a "perror-style" message on the console.
	//
	void		Perror(const char *message ...);
};

#endif	_CONSOLE_H
