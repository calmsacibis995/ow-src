#ident "@(#)console.cc	1.4 06/11/93 Copyright 1991 Sun Microsystems, Inc."



#include <doc/console.h>
#include <stdio.h>
#include <errno.h>
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


STATUS
CONSOLE::Init(const STRING &prog_name)
{
	DbgFunc("CONSOLE::Init: program=" << prog_name << endl);

	program_name = prog_name;
	is_inited = BOOL_TRUE;

	return(STATUS_OK);
}

// Print a "printf-style" (varargs) message on the console.
//
void
CONSOLE::Message(const char *message ...)
{
	va_list	args;

	assert(IsInited());
	DbgFunc("CONSOLE::Message: " << message << endl);

	if (program_name != NULL_STRING)
		fprintf(stderr, "%s: ", (const char *)program_name);

	va_start(args, message);
	vfprintf(stderr, message, args);
	va_end(args);

	fputc('\n', stderr);
}

// Print a "perror-style" message on the console.
//
void
CONSOLE::Perror(const char *message ...)
{
	va_list		args;
	extern int	errno;
#ifndef SVR4
	extern char	*sys_errlist[];
#endif	SVR4

	assert(IsInited());
	DbgFunc("CONSOLE::Perror: " << message << endl);

	if (program_name != NULL_STRING)
		fprintf(stderr, "%s: ", (const char *)program_name);

	va_start(args, message);
	vfprintf(stderr, message, args);
	va_end(args);

	fprintf(stderr, ": %s\n",
#ifdef SVR4
		strerror(errno));
#else
		(errno <= sys_nerr) ? sys_errlist[errno] : "Bad errno");
#endif SVR4

	fputc('\n', stderr);
}
