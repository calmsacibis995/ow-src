/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOCONVERT_CONVERT_H
#define	_AUDIOCONVERT_CONVERT_H

#ident	"@(#)convert.h	1.15	94/01/13 SMI"

#include "audio_i18n.h"
#include <multimedia/Audio.h>
#include <multimedia/AudioUnixfile.h>
#include <multimedia/AudioBuffer.h>
#include <multimedia/AudioTypeConvert.h>

#include "parse.h"

// for localizing strings
#ifndef SUNOS41
#define	MGET(str)	(char *) gettext(str)

#else /* 4.x */
#define	MGET(str)	(str)
#define	textdomain(d)
#define	setlocale(a, b)
#endif /* 4.x */

extern int		Statistics;		// report timing statistics
extern int		Debug;			// Debug flag

extern AudioBuffer*	create_buffer(Audio*);
extern void		get_realfile(char*&, struct stat*);
extern AudioUnixfile*	open_input_file(const char*, const AudioHdr,
			    int, int, off_t, format_type&);
extern AudioUnixfile*	create_output_file(const char*, const AudioHdr,
			    format_type, const char *infoString);
extern int		verify_conversion(AudioHdr, AudioHdr);
extern int		do_convert(AudioStream*, AudioStream*);
extern AudioError	write_output(AudioBuffer*, AudioStream*);
extern int		noop_conversion(AudioHdr, AudioHdr,
			    format_type, format_type, off_t, off_t);
extern void		Err(char*, ...);

#endif /* !_AUDIOCONVERT_CONVERT_H */
