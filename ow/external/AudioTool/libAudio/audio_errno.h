/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIO_ERRNO_H
#define	_MULTIMEDIA_AUDIO_ERRNO_H

#ident	"@(#)audio_errno.h	1.10	97/01/14 SMI"

/*
 * libaudio error codes
 */

/* XXX - error returns and exception handling need to be worked out */
enum audioerror_t {
	AUDIO_SUCCESS = 0,		/* no error */
	AUDIO_NOERROR = -2,		/* no error, no message */
	AUDIO_UNIXERROR = -1,		/* check errno for error code */
	AUDIO_ERR_BADHDR = 1,		/* bad audio header structure */
	AUDIO_ERR_BADFILEHDR = 2,	/* bad file header format */
	AUDIO_ERR_BADARG = 3,		/* bad subroutine argument */
	AUDIO_ERR_NOEFFECT = 4,		/* device control ignored */
	AUDIO_ERR_ENCODING = 5,		/* unknown encoding format */
	AUDIO_ERR_INTERRUPTED = 6,	/* operation was interrupted */
	AUDIO_EOF = 7,			/* end-of-file */
	AUDIO_ERR_HDRINVAL = 8,		/* unsupported data format */
	AUDIO_ERR_PRECISION = 9,	/* unsupported data precision */
	AUDIO_ERR_NOTDEVICE = 10,	/* not an audio device */
	AUDIO_ERR_DEVICEBUSY = 11,	/* audio device is busy */
	AUDIO_ERR_BADFRAME = 12,	/* partial sample frame */
	AUDIO_ERR_FORMATLOCK = 13,	/* audio format cannot be changed */
	AUDIO_ERR_DEVOVERFLOW = 14	/* device overflow error */
};

#endif /* !_MULTIMEDIA_AUDIO_ERRNO_H */
