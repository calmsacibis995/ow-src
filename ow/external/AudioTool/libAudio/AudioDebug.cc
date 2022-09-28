/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)AudioDebug.cc	1.8	96/09/24 SMI"

// XXX - all this either goes away or gets repackaged
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "AudioDebug.h"


// Global debugging level variable
int	Audio_debug;


// Get debug level
int
GetDebug()
{
	return (Audio_debug);
}

// Set debug level
void
SetDebug(
	int	val)			// new level
{
	Audio_debug = val;
}

// Default error printing routine
Boolean
AudioStderrMsg(
	const Audio*	cp,		// object pointer
	AudioError	code,		// error code
	AudioSeverity	sev,		// error severity
	char*		str)		// additional message string
{
	int		id;
	char*		name;

	id = cp->getid();
	switch (sev) {
	default:
		name = cp->GetName();
		break;
	case InitMessage:		// virtual function table not ready
	case InitFatal:
		name = cp->Audio::GetName();
		break;
	}

	switch (sev) {
	case InitMessage:
	case Message:
		if (Audio_debug > 1)
			(void) fprintf(stderr, _MGET_("%d: %s (%s) %s\n"),
			    id, str, name, code.msg());
		return (TRUE);
	case Warning:
		(void) fprintf(stderr, _MGET_("Warning: %s: %s %s\n"),
		    name, code.msg(), str);
		if (Audio_debug > 2)
			abort();
		return (TRUE);
	case Error:
		(void) fprintf(stderr, _MGET_("Error: %s: %s %s\n"),
		    name, code.msg(), str);
		if (Audio_debug > 1)
			abort();
		return (FALSE);
	case Consistency:
		(void) fprintf(stderr,
		    _MGET_("Audio Consistency Error: %s: %s %s\n"),
		    name, str, code.msg());
		if (Audio_debug > 0)
			abort();
		return (FALSE);
	case InitFatal:
	case Fatal:
		(void) fprintf(stderr,
		    _MGET_("Audio Internal Error: %s: %s %s\n"),
		    name, str, code.msg());
		if (Audio_debug > 0)
			abort();
		return (FALSE);
	}
	return (TRUE);
}

#ifdef DEBUG
void
AudioDebugMsg(
	int	level,
	char*	fmt,
		...)
{
	va_list ap;

	if (Audio_debug >= level) {
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}
#endif
