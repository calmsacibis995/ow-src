/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOERROR_H
#define	_MULTIMEDIA_AUDIOERROR_H

#ident	"@(#)AudioError.h	1.13	96/09/24 SMI"

#include <locale.h>
#include <errno.h>
#include "audio_errno.h"	/* to get enum for error codes */

#ifndef SUNOS41
#define	_MGET_(str)	(char *) dgettext(I18N_DOMAIN, str)
#define	_GETTEXT_(str)	(char *) gettext(str)
#else /* 4.x */
#define	_MGET_(str)	str
#define	_GETTEXT_(str)	str
#endif /* 4.x */


/* If compiling C++ code... */
#ifdef __cplusplus

// The AudioError class allows various interesting automatic conversions
class AudioError {
private:
	audioerror_t	code;			// error code

public:
	int		sys;			// system error code

	inline AudioError (audioerror_t val = AUDIO_SUCCESS):	// Constructor
	    code(val), sys(0)
	    { if (code == AUDIO_UNIXERROR) sys = errno; }
	inline AudioError (int val):			// Constructor from int
	    code((audioerror_t)val), sys(0)
	    { if (code == AUDIO_UNIXERROR) sys = errno; }

	inline operator= (AudioError val)		// Assignment
	    { code = val.code; sys = val.sys; return (*this); }
	inline operator int()				// Cast to integer
	    { return (code); }
	inline operator== (audioerror_t e)		// Compare
	    { return (code == e); }
	inline operator!= (audioerror_t e)		// Compare
	    { return (code != e); }
	inline operator== (AudioError e)		// Compare
	    { return ((code == e.code) && (sys == e.sys)); }
	inline operator!= (AudioError e)		// Compare
	    { return ((code != e.code) || (sys |= e.sys)); }

	char*	msg();					// Return error string
};

#endif /* __cplusplus */

#endif /* !_MULTIMEDIA_AUDIOERROR_H */
