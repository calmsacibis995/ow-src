/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_LIBAUDIO_H
#define	_MULTIMEDIA_LIBAUDIO_H

#ident	"@(#)libaudio.h	1.17	92/11/10 SMI"

#include "audio_types.h"
#include "audio_hdr.h"
#include "audio_filehdr.h"
#include "audio_device.h"
#include "audio_errno.h"
#include "audio_encode.h"

/* define various constants for general use */

/* Theoretical maximum length of hh:mm:ss.dd string */
#define	AUDIO_MAX_TIMEVAL	(32)

/* Theoretical maximum length of encoding information string */
#define	AUDIO_MAX_ENCODE_INFO	(80)


/* Why aren't these stupid values defined in a standard place?! */
#ifndef TRUE
#define	TRUE	(1)
#endif
#ifndef FALSE
#define	FALSE	(0)
#endif
#ifndef NULL
#define	NULL	0
#endif


/* Declare libaudio C routines */

/* File Header routines */
EXTERN_FUNCTION( int audio_write_filehdr, (int, Audio_hdr*, char*, unsigned) );
EXTERN_FUNCTION( int audio_rewrite_filesize, (int, unsigned) );
EXTERN_FUNCTION( int audio_read_filehdr, (int, Audio_hdr*, char*, unsigned) );
EXTERN_FUNCTION( int audio_isaudiofile, (char*) );
EXTERN_FUNCTION( int audio_decode_filehdr,
		    (unsigned char*, Audio_hdr*, unsigned*) );
EXTERN_FUNCTION( int audio_encode_filehdr, (Audio_hdr*, char*, unsigned,
		    unsigned char*, unsigned*) );

/* Audio Header routines */
EXTERN_FUNCTION( double audio_bytes_to_secs, (Audio_hdr*, unsigned) );
EXTERN_FUNCTION( unsigned audio_secs_to_bytes, (Audio_hdr*, double) );
EXTERN_FUNCTION( double audio_str_to_secs, (char*) );
EXTERN_FUNCTION( char *audio_secs_to_str, (double, char*, int) );
EXTERN_FUNCTION( int audio_cmp_hdr, (Audio_hdr*, Audio_hdr*) );
EXTERN_FUNCTION( int audio_enc_to_str, (Audio_hdr*, char*) );


/* Device Control routines */
EXTERN_FUNCTION( int audio_getinfo, (int, Audio_info*) );
EXTERN_FUNCTION( int audio_setinfo, (int, Audio_info*) );
EXTERN_FUNCTION( int audio__setplayhdr, (int, Audio_hdr*, unsigned) );
EXTERN_FUNCTION( int audio__setval, (int, unsigned*, unsigned) );
EXTERN_FUNCTION( int audio__setgain, (int, double*, unsigned) );
EXTERN_FUNCTION( int audio__setpause, (int, unsigned) );
EXTERN_FUNCTION( int audio__flush, (int, unsigned int) );
EXTERN_FUNCTION( int audio_drain, (int, int) );
EXTERN_FUNCTION( int audio_play_eof, (int) );

#endif /* !_MULTIMEDIA_LIBAUDIO_H */
