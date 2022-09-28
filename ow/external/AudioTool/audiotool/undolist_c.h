/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_UNDOLIST_C_H
#define	_AUDIOTOOL_UNDOLIST_C_H

#ident	"@(#)undolist_c.h	1.32	93/02/17 SMI"

#include <multimedia/AudioError.h>

/* Define AudioLevel structure */
typedef struct AudioLevel {
	double		level;		/* 0.- 1. average level */
	int		clip;		/* TRUE: clipping detected */
} AudioLevel;


/* Compile the rest only if normal C compile */
#ifndef __cplusplus


#include <multimedia/audio_types.h>
#include <multimedia/libaudio_c.h>
#include <multimedia/AudioDetect.h>


/* Undo List manipulation routines */

typedef void*	Undolist;
typedef struct AudioDetectPts AudioDetectPts;
typedef enum AudioDetectConfig AudioDetectConfig;

EXTERN_FUNCTION( int list_init, (Undolist*) );
EXTERN_FUNCTION( void list_destroy, (Undolist) );
EXTERN_FUNCTION( int list_newfile, (Undolist, char*) );
EXTERN_FUNCTION( double list_getlength, (Undolist) );
EXTERN_FUNCTION( int list_getfilehdr, (Undolist, double, double, Audio_hdr*) );
EXTERN_FUNCTION( AudioList list_getaudio, (Undolist) );
EXTERN_FUNCTION( Audio_Object list_getrange, (Undolist, double, int, Audio_Object) );
EXTERN_FUNCTION( unsigned char *audio_getbufaddr, (Audio_Object) );
EXTERN_FUNCTION( int list_getdetect, (Undolist, AudioDetectPts**) );
EXTERN_FUNCTION( double list_getdetectconfig, (Undolist, AudioDetectConfig) );
EXTERN_FUNCTION( int list_configdetect, (Undolist, AudioDetectConfig, double) );
EXTERN_FUNCTION( int list_canundo, (Undolist) );
EXTERN_FUNCTION( int list_undo, (Undolist) );
EXTERN_FUNCTION( int list_undoall, (Undolist) );
EXTERN_FUNCTION( int list_redo, (Undolist) );
EXTERN_FUNCTION( int list_canredo, (Undolist) );
EXTERN_FUNCTION( int list_redoall, (Undolist) );
EXTERN_FUNCTION( int list_insert, (Undolist, Audio_Object, double) );
EXTERN_FUNCTION( int list_remove, (Undolist, double, double) );
EXTERN_FUNCTION( int list_prune, (Undolist, double, double) );
EXTERN_FUNCTION( int list_trimends, (Undolist, AudioDetectPts*) );
EXTERN_FUNCTION( int list_trimsilence, (Undolist, AudioDetectPts*) );
EXTERN_FUNCTION( int list_replace, (Undolist, double, double, Audio_Object) );
EXTERN_FUNCTION( int list_copy, (Undolist, double, double, Audio_Object*) );
EXTERN_FUNCTION( int list_writefile, (Undolist, char*, Audio_hdr*) );
EXTERN_FUNCTION( int list_async_writefile, (Undolist, Audio_Object, double*, double*) );
EXTERN_FUNCTION( int list_createfile, (char*, Audio_hdr*, Audio_Object*) );
EXTERN_FUNCTION( int list_playstart, (Undolist, char*, double, double, double, double) );
EXTERN_FUNCTION( int list_flush, (Undolist) );
EXTERN_FUNCTION( int list_eof, (Undolist) );
EXTERN_FUNCTION( int list_stop, (Undolist) );
EXTERN_FUNCTION( int list_async, (Undolist) );
EXTERN_FUNCTION( int list_playreset, (Undolist, double, double, int) );
EXTERN_FUNCTION( double list_playposition, (Undolist) );
EXTERN_FUNCTION( AudioLevel list_getlevel, (Undolist, double, double) );
EXTERN_FUNCTION( AudioLevel list_playlevel, (Undolist, double) );
EXTERN_FUNCTION( AudioLevel list_recordlevel, (Undolist, double) );
EXTERN_FUNCTION( int list_recordstart, (Undolist, char*, char*, Audio_hdr*, double, double) );
EXTERN_FUNCTION( double list_recordposition, (Undolist) );
EXTERN_FUNCTION( double list_recordbufposition, (Undolist) );
EXTERN_FUNCTION( int list_scrubstart, (Undolist, char*) );
EXTERN_FUNCTION( int list_scrub, (Undolist, double, double, double) );

EXTERN_FUNCTION( double list_rewind, (Undolist) );
EXTERN_FUNCTION( double list_readpos, (Undolist) );

/* device control, file open/close, etc. */
EXTERN_FUNCTION( char *audio_getname, (Audio_Object) );
EXTERN_FUNCTION( int audio_openctldev, (char*, Audio_Object*) );
EXTERN_FUNCTION( int audio_checkformat, (Audio_Object, Audio_hdr*) );
EXTERN_FUNCTION( int audio_setformat, (Audio_Object, Audio_hdr*) );
EXTERN_FUNCTION( void audio_setplaywaiting, (Audio_Object) );
EXTERN_FUNCTION( void audio_setrecordwaiting, (Audio_Object) );
EXTERN_FUNCTION( int audio_getplayopen, (Audio_Object) );
EXTERN_FUNCTION( int audio_openfile, (char*, Audio_Object*) );
EXTERN_FUNCTION( void audio_reference, (Audio_Object) );
EXTERN_FUNCTION( void audio_dereference, (Audio_Object) );
EXTERN_FUNCTION( char* audio_errmsg, (int) );

/* file length, file system free space */
EXTERN_FUNCTION( long getfreespace, (char*) );
EXTERN_FUNCTION( long list_getfilesize, (Undolist*) );
EXTERN_FUNCTION( long audio_getsize, (double, Audio_hdr*) );
EXTERN_FUNCTION( double audio_getlength, (Audio_Object) );
EXTERN_FUNCTION( double audio_bytes_to_time, (long, Audio_hdr*) );

/* for selection transfer, etc. */
EXTERN_FUNCTION( int list_read_pipe, (Undolist, int, Audio_Object*) );
EXTERN_FUNCTION( int list_write_pipe, (Undolist, int, double, double) );
EXTERN_FUNCTION( int audio_write_pipe, (Audio_Object, int) );
EXTERN_FUNCTION( int audio_getfilehdr, (Audio_Object, Audio_hdr*) );
EXTERN_FUNCTION( int audio_getbuffer, (Audio_Object, unsigned char*, 
				       unsigned, unsigned*) );
EXTERN_FUNCTION( Audio_Object audio_createbuffer, (Audio_hdr*, unsigned) );
EXTERN_FUNCTION( int audio_putbuffer, (Audio_Object, unsigned char*,
				       unsigned long*, unsigned long*) );

/* parsing routines */
EXTERN_FUNCTION( char* audio_printkbytes, (long) );
EXTERN_FUNCTION( char* audio_printhdr, (Audio_hdr*) );
EXTERN_FUNCTION( char* audio_print_encoding, (Audio_hdr*) );
EXTERN_FUNCTION( char* audio_print_rate, (Audio_hdr*) );
EXTERN_FUNCTION( char* audio_print_channels, (Audio_hdr*) );
EXTERN_FUNCTION( int audio_parsehdr, (char*, Audio_hdr*) );
EXTERN_FUNCTION( int audio_parse_rate, (char*, Audio_hdr*) );
EXTERN_FUNCTION( int audio_parse_encoding, (char*, Audio_hdr*) );
EXTERN_FUNCTION( int audio_parse_channels, (char*, Audio_hdr*) );

EXTERN_FUNCTION( void audio_debug, (int) );
EXTERN_FUNCTION( void audio_perror, (int, char*) );
EXTERN_FUNCTION( void print_detect, (AudioDetectPts*) );

#endif /*!__cplusplus*/

#endif /* !_AUDIOTOOL_UNDOLIST_C_H */
