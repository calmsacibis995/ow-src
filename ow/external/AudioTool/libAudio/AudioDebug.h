/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIODEBUG_H
#define	_MULTIMEDIA_AUDIODEBUG_H

#ident	"@(#)AudioDebug.h	1.6	92/11/17 SMI"

#include "audio_types.h"
#include "Audio.h"


// Declare default message printing routine
EXTERN_FUNCTION( Boolean AudioStderrMsg,
    (const Audio*, AudioError, AudioSeverity, char*) );

#ifdef DEBUG
EXTERN_FUNCTION( void AudioDebugMsg, (int, char* fmt, DOTDOTDOT) );
#endif

EXTERN_FUNCTION( void SetDebug, (int) );
EXTERN_FUNCTION( int GetDebug, () );

#ifdef DEBUG
#define AUDIO_DEBUG(args)    AudioDebugMsg args
#else
#define AUDIO_DEBUG(args)
#endif /* !DEBUG */


#endif /* !_MULTIMEDIA_AUDIODEBUG_H */
