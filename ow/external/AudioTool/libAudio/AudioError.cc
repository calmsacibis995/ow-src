/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)AudioError.cc	1.10	93/02/04 SMI"

#include <string.h>
#include "AudioError.h"


// class Audio methods

// Convert error code to string
char* AudioError::
msg()
{
	if (code == AUDIO_NOERROR)
		return ("");
	if (code == AUDIO_UNIXERROR) {
		if (sys == 0)
			sys = errno;
#ifndef SUNOS41
		// XXX - strerror crashes on error numbers out of range!
		if (sys >= 0)
			return (strerror(sys));
#else /* 4.x */
		if ((sys < sys_nerr) && (sys >= 0))
			return (sys_errlist[sys]);
#endif
		else
			return (_MGET_("Unknown UNIX error"));
	}

	// XXX - these must jive with what's in audio_errno.h
	switch (code) {
	case 0:				/* AUDIO_SUCCESS = 0 */
		return (_MGET_("Audio operation successful"));
	case 1:				/* AUDIO_ERR_BADHDR = 1 */
		return (_MGET_("Invalid audio header"));
	case 2:				/* AUDIO_ERR_BADFILEHDR = 2 */
		return (_MGET_("Invalid audio file header"));
	case 3:				/* AUDIO_ERR_BADARG = 3 */
		return (_MGET_("Invalid argument or value"));
	case 4:				/* AUDIO_ERR_NOEFFECT = 4 */
		return (_MGET_("Audio operation not performed"));
	case 5:				/* AUDIO_ERR_ENCODING = 5 */
		return (_MGET_("Unknown audio encoding format"));
	case 6:				/* AUDIO_ERR_INTERRUPTED = 6 */
		return (_MGET_("Audio operation interrupted"));
	case 7:				/* AUDIO_EOF = 7 */
		return (_MGET_("Audio end-of-file"));
	case 8:				/* AUDIO_ERR_HDRINVAL = 8 */
		return (_MGET_("Unsupported audio data format"));
	case 9:				/*AUDIO_ERR_PRECISION = 9 */
		return (_MGET_("Unsupported audio data precision"));
	case 10:			/* AUDIO_ERR_NOTDEVICE = 10 */
		return (_MGET_("Not an audio device"));
	case 11:			/* AUDIO_ERR_DEVICEBUSY = 11 */
		return (_MGET_("Audio device is busy"));
	case 12:			/* AUDIO_ERR_BADFRAME = 12 */
		return (_MGET_("Partial sample frame"));
	case 13:			/* AUDIO_ERR_FORMATLOCK = 13 */
		return (_MGET_("Audio format cannot be changed"));
	case 14:			/* AUDIO_ERR_DEVOVERFLOW = 14 */
		return (_MGET_("Audio device overrun"));
	default:
		return (_MGET_("Unknown audio error"));
	}
}
