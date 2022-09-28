/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIO_HDR_H
#define	_MULTIMEDIA_AUDIO_HDR_H

#ident	"@(#)audio_hdr.h	1.16	95/06/28 SMI"

/*
 * Define an in-core audio data header.
 *
 * This is different that the on-disk file header.
 * The fields defined here are preliminary at best.
 */

/*
 * The audio header contains the following fields:
 *
 *      endian                  Byte order of 16-bit or greater PCM,
 *                                and possibly floating point data.
 *
 *	sample_rate		Number of samples per second (per channel).
 *
 *	samples_per_unit	This field describes the number of samples
 *				  represented by each sample unit (which, by
 *				  definition, are aligned on byte boundaries).
 *				  Audio samples may be stored individually
 *				  or, in the case of compressed formats
 *				  (e.g., ADPCM), grouped in algorithm-
 *				  specific ways.  If the data is bit-packed,
 *				  this field tells the number of samples
 *				  needed to get to a byte boundary.
 *
 *	bytes_per_unit		Number of bytes stored for each sample unit
 *
 *	channels		Number of interleaved sample units.
 *				   For any given time quantum, the set
 *				   consisting of 'channels' sample units
 *				   is called a sample frame.  Seeks in
 *				   the data should be aligned to the start
 *				   of the nearest sample frame.
 *
 *	encoding		Data encoding format.
 *
 *	data_size		Number of bytes in the data.
 *				   This value is advisory only, and may
 *				   be set to the value AUDIO_UNKNOWN_SIZE
 *				   if the data size is unknown (for
 *				   instance, if the data is being
 *				   recorded or generated and piped
 *				   to another process).
 *
 * The first four values are used to compute the byte offset given a
 * particular time, and vice versa.  Specifically:
 *
 *	seconds = offset / C
 *	offset = seconds * C
 * where:
 *	C = (channels * bytes_per_unit * sample_rate) / samples_per_unit
 *
 *
 */
typedef struct {
	unsigned	sample_rate;		/* samples per second */
	unsigned	samples_per_unit;	/* samples per unit */
	unsigned	bytes_per_unit;		/* bytes per sample unit */
	unsigned	channels;		/* # of interleaved channels */
	unsigned	encoding;		/* data encoding format */
        unsigned        endian;                 /* byte order */
	unsigned	data_size;		/* length of data (optional) */
} Audio_hdr;

/*
 * Define the possible encoding types.
 * Note that the names that overlap the encodings in <sun/audioio.h>
 * must have the same values.
 */
#define AUDIO_ENCODING_NONE	(0)	/* No encoding specified ... */
#define	AUDIO_ENCODING_ULAW	(1)	/* ISDN u-law */
#define	AUDIO_ENCODING_ALAW	(2)	/* ISDN A-law */
#define	AUDIO_ENCODING_LINEAR	(3)	/* PCM 2's-complement (0-center) */
#define	AUDIO_ENCODING_FLOAT	(100)	/* IEEE float (-1. <-> +1.) */
#define	AUDIO_ENCODING_G721	(101)	/* CCITT g.721 ADPCM */
#define	AUDIO_ENCODING_G722	(102)	/* CCITT g.722 ADPCM */
#define	AUDIO_ENCODING_G723	(103)	/* CCITT g.723 ADPCM */
#define	AUDIO_ENCODING_DVI	(104)	/* DVI ADPCM */

/*
 * Define the possible endian types.
 */
#define AUDIO_ENDIAN_BIG 0    /* SPARC, 68000, etc. */
#define AUDIO_ENDIAN_SMALL 1  /* Intel              */

/* Value used for indeterminate size (e.g., data passed through a pipe) */
#define	AUDIO_UNKNOWN_SIZE	((unsigned)(~0))


/* Define conversion macros for integer<->floating-point conversions */

/* double to 8,16,32-bit linear */
#define	audio_d2c(X)		((X) >= 1. ? 127 : (X) <= -1. ? -127 :	\
				    (char)(rint((X) * 127.)))
#define	audio_d2s(X)		((X) >= 1. ? 32767 : (X) <= -1. ? -32767 :\
				    (short)(rint((X) * 32767.)))
#define	audio_d2l(X)		((X) >= 1. ? 2147483647 : (X) <= -1. ?	\
				    -2147483647 :			\
				    (long)(rint((X) * 2147483647.)))
				    

/* 8,16,32-bit linear to double */
#define	audio_c2d(X)		(((unsigned char)(X)) == 0x80 ? -1. :	\
				    ((double)((char)(X))) / 127.)
#define	audio_s2d(X)		(((unsigned short)(X)) == 0x8000 ? -1. :\
				    ((double)((short)(X))) / 32767.)
#define	audio_l2d(X)		(((unsigned long)(X)) == 0x80000000 ? -1. :\
				    ((double)((long)(X))) / 2147483647.)


#endif /*!_MULTIMEDIA_AUDIO_HDR_H*/
