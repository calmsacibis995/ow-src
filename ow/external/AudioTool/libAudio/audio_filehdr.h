/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIO_FILEHDR_H
#define	_MULTIMEDIA_AUDIO_FILEHDR_H

#ident	"@(#)audio_filehdr.h	1.10	92/11/10 SMI"

#include "archdep.h"

/*
 * Define an on-disk audio file header.
 *
 * This structure should not be arbitrarily imposed over a stream of bytes,
 * since the byte orders could be wrong.
 *
 * Note that there is an 'info' field that immediately follows this
 * structure in the file.
 *
 * The hdr_size field is problematic in the general case because the
 * field is really "data location", which does not ensure that all
 * the bytes between the header and the data are really 'info'.
 * Further, there are no absolute guarantees that the 'info' is ASCII text,
 * (non-ASCII info may eventually be far more useful anyway).
 *
 * When audio files are passed through pipes, the 'data_size' field may
 * not be known in advance.  In such cases, the 'data_size' should be
 * set to AUDIO_UNKNOWN_SIZE.
 */
typedef struct {
	u_32		magic;		/* magic number */
	u_32		hdr_size;	/* size of this header */
	u_32		data_size;	/* length of data (optional) */
	u_32		encoding;	/* data encoding format */
	u_32		sample_rate;	/* samples per second */
	u_32		channels;	/* number of interleaved channels */
} Audio_filehdr;


/* Define the magic number */
#define	AUDIO_FILE_MAGIC		((u_32)0x2e736e64)

/* Define the encoding fields */
#define	AUDIO_FILE_ENCODING_MULAW_8	(1)	/* 8-bit ISDN u-law */
#define	AUDIO_FILE_ENCODING_LINEAR_8	(2)	/* 8-bit linear PCM */
#define	AUDIO_FILE_ENCODING_LINEAR_16	(3)	/* 16-bit linear PCM */
#define	AUDIO_FILE_ENCODING_LINEAR_24	(4)	/* 24-bit linear PCM */
#define	AUDIO_FILE_ENCODING_LINEAR_32	(5)	/* 32-bit linear PCM */
#define	AUDIO_FILE_ENCODING_FLOAT	(6)	/* 32-bit IEEE floating point */
#define	AUDIO_FILE_ENCODING_DOUBLE	(7)	/* 64-bit IEEE floating point */
#define	AUDIO_FILE_ENCODING_ADPCM_G721	(23)	/* 4-bit CCITT g.721 ADPCM */
#define	AUDIO_FILE_ENCODING_ADPCM_G722	(24)	/* CCITT g.722 ADPCM */
#define	AUDIO_FILE_ENCODING_ADPCM_G723_3 (25)	/* CCITT g.723 3-bit ADPCM */
#define	AUDIO_FILE_ENCODING_ADPCM_G723_5 (26)	/* CCITT g.723 5-bit ADPCM */
#define AUDIO_FILE_ENCODING_ALAW_8	(27) 	/* 8-bit ISDN A-law */

#endif /* !_MULTIMEDIA_AUDIO_FILEHDR_H */
