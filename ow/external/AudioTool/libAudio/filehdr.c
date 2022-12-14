#ident "@(#)filehdr.c 1.26 96/03/21 Copyr 1989 Sun Micro"
/* Copyright (c) 1989 by Sun Microsystems, Inc. */

/*
 * This file contains a set of Very Paranoid routines to convert
 * audio file headers to in-core audio headers and vice versa.
 *
 * They are robust enough to handle any random file input without
 * crashing miserably.  Of course, bad audio headers coming from
 * the calling program can cause significant problems.
 */

#include <stdlib.h>
#include <memory.h>
#include <fcntl.h>
#include <errno.h>         /* needed for large file error checking */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>

#include "libaudio_impl.h"	/* include other audio hdr's */

/* Round up to a double boundary */
#define	ROUND_DBL(x)	(((x) + 7) & ~7)

/*
 * Write an audio file header to an output stream.
 *
 * The file header is encoded from the supplied Audio_hdr structure.
 * If 'infop' is not NULL, it is the address of a buffer containing 'info'
 * data.  'ilen' specifies the size of this buffer.
 * The entire file header will be zero-padded to a double-word boundary.
 *
 * Note that the file header is stored on-disk in big-endian format,
 * regardless of the machine type.
 *
 * Note also that the output file descriptor must not have been set up
 * non-blocking i/o.  If non-blocking behavior is desired, set this
 * flag after writing the file header.
 */
int
audio_write_filehdr(int fd, Audio_hdr *hdrp, char *infop, unsigned int ilen)
	   		   		/* file descriptor */
	         	      		/* audio header */
	    		       		/* info buffer pointer */
	        	     		/* buffer size */
{
	int		err;
	unsigned	blen;
	unsigned char	*buf;		/* temporary buffer */

	/* create tmp buf for audio_encode_filehdr to work with */
	blen = sizeof(Audio_hdr) + (infop ? ilen : 0) + 4;
	blen = ROUND_DBL(blen);
	
	if (!(buf = (unsigned char*)calloc(1, blen))) {
		return (AUDIO_UNIXERROR);
	}

	err = audio_encode_filehdr(hdrp, infop, ilen, buf, &blen);
	if (err != AUDIO_SUCCESS) {
		return (err);
	}

	/* Write and free the holding buffer */
	err = write(fd, (char *)buf, (int)blen);
	(void) free((char *)buf);

	if (err != blen)
		return ((err < 0) ? AUDIO_UNIXERROR : AUDIO_ERR_BADFILEHDR);
	return (AUDIO_SUCCESS);
}

/*
 * Rewrite the data size field of an audio header to the output stream.
 *
 * If the output file is capable of seeking, rewrite the audio file header
 * data_size field with the supplied value.
 * Otherwise, return AUDIO_ERR_NOEFFECT.
 */
int
audio_rewrite_filesize(int fd, unsigned int size)
	   		   		/* file descriptor */
	        	     		/* new data size */
{
	int		err;
	unsigned char	*buf;
	Audio_filehdr	fhdr;
	int             fcntl_err;

	/* Can we seek back in this file and write without appending? */
	err = (char *)&fhdr.data_size - (char *)&fhdr;
	fcntl_err = fcntl(fd, F_GETFL, 0);
	if ((fcntl_err < 0) && ((errno == EOVERFLOW) || (errno == EINVAL))) {
	  /* Large file encountered (probably) */
	  perror("fcntl");
	  exit(1);
	} else if ((lseek(fd, (off_t)err, SEEK_SET) < 0) ||
		   (fcntl_err & FAPPEND)) {
	  return (AUDIO_ERR_NOEFFECT);
	}

	/* Allocate a buffer to hold the data */
	buf = (unsigned char *)malloc(sizeof (fhdr.data_size));
	if (buf == NULL)
		return (AUDIO_UNIXERROR);

	/* Encode the 32-bit integer header field */
	ENCODE_LONG(&size, buf);

	/* Write and free the holding buffer */
	err = write(fd, (char *)buf, sizeof (fhdr.data_size));
	(void) free((char *)buf);

	if (err != sizeof (fhdr.data_size))
		return ((err < 0) ? AUDIO_UNIXERROR : AUDIO_ERR_BADFILEHDR);
	return (AUDIO_SUCCESS);
}


/*
 * Decode an audio file header from an input stream.
 *
 * The file header is decoded into the supplied Audio_hdr structure.
 * If 'infop' is not NULL, it is the address of a buffer to which the
 * 'info' portion of the file header will be copied.  'ilen' specifies
 * the maximum number of bytes to copy.  The buffer will be NULL-terminated,
 * even if it means over-writing the last byte.
 *
 * Note that the file header is stored on-disk in big-endian format,
 * regardless of the machine type.  This may not have been true if
 * the file was written on a non-Sun machine.  For now, such
 * files will appear invalid.
 *
 * Note also that the input file descriptor must not have been set up
 * non-blocking i/o.  If non-blocking behavior is desired, set this
 * flag after reading the file header.
 */
int
audio_read_filehdr(int fd, Audio_hdr *hdrp, char *infop, unsigned int ilen)
	   		   		/* input file descriptor */
	         	      		/* output audio header */
	    		       		/* info buffer pointer */
	        	     		/* buffer size */
{
	int		err;
	unsigned	isize;
	unsigned	resid;
	unsigned char	buf[sizeof (Audio_filehdr)];
	struct stat	st;

	/* Read the header (but not the text info). */
	err = read(fd, (char *)buf, sizeof (buf));
	if ((err != sizeof (buf)) ||
	    (audio_decode_filehdr(buf, hdrp, &isize) != AUDIO_SUCCESS)) {
		goto checkerror;
	}

	/* Stat the file, to determine if it is a regular file. */
	err = fstat(fd, &st);
	if (err < 0)
		return (AUDIO_UNIXERROR);

	/*
	 * If data_size is not indeterminate (i.e., this isn't a pipe),
	 * try to validate the hdr_size and data_size.
	 */
	if (hdrp->data_size != AUDIO_UNKNOWN_SIZE) {
		/* Only trust the size for regular files */
		if (S_ISREG(st.st_mode)) {
			if (st.st_size !=
			    (isize + hdrp->data_size + sizeof (Audio_filehdr)))
				return (AUDIO_ERR_BADFILEHDR);
		}
	}

	/*
	 * If infop is non-NULL, try to read in the info data.
	 */
	resid = isize;
	if ((infop != NULL) && (ilen != 0)) {
		if (isize > ilen)
			isize = ilen;
		err = read(fd, infop, (int)isize);
		if (err != isize)
			goto checkerror;

		/* Zero any residual bytes in the text buffer */
		if (isize < ilen)
			(void) memset(&infop[isize], '\0',
				      (int)(ilen - isize));
		else
			infop[ilen - 1] = '\0';	/* zero-terminate */

		resid -= err;			/* subtract the amount read */
	}

	/*
	 * If we truncated the info, seek or read data until info size is
	 * satisfied.  If regular file, seek nearly to end and check for eof.
	 */
	if (resid != 0) {
		if (S_ISREG(st.st_mode)) {
			err = lseek(fd, (off_t)(resid - 1), SEEK_CUR);
			if ((err < 0) ||
			    ((err = read(fd, (char *)buf, 1)) != 1))
				goto checkerror;
		} else while (resid != 0) {
			char	junk[8192];	/* temporary buffer */

			isize = (resid > sizeof (junk)) ? sizeof (junk) : resid;
			err = read(fd, junk, (int)isize);
			if (err != isize)
				goto checkerror;
			resid -= err;
		}
	}
	return (AUDIO_SUCCESS);

checkerror:
	if ((err < 0) && (errno == EOVERFLOW)) {
	  perror("read");
	  exit(1);
	} else {
	  return ((err < 0) ? AUDIO_UNIXERROR : AUDIO_ERR_BADFILEHDR);
	}
}

/*
 * Return TRUE if the named file is an audio file.  Else, return FALSE.
 */
int
audio_isaudiofile(char *name)
{
	int		fd;
	int		err;
	Audio_hdr	hdr;
	unsigned char	buf[sizeof (Audio_filehdr)];
	unsigned	isize;

	/* Open the file (set O_NONBLOCK in case the name refers to a device) */
	fd = open(name, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
	  if (errno == EOVERFLOW) {
	    perror("open");
	    exit(1);
	  } else {
	    return (FALSE);
	  }
	}

	/* Read the header (but not the text info). */
	err = read(fd, (char *)buf, sizeof (buf));
	if (err < 0) {
	  if (errno == EOVERFLOW) {
	    perror("open");
	    exit(1);
	  } else {
	    return (FALSE);
	  }
	}
	(void) close(fd);

	if ((err == sizeof (buf)) &&
	    (audio_decode_filehdr(buf, &hdr, &isize) == AUDIO_SUCCESS)) {
		return (hdr.encoding);
	} else {
		return (FALSE);
	}
}

/*
 * audio_endian()
 *
 * This routine tests the magic number at the head of a buffer
 * containing the file header.  The first thing in the header
 * should be the magic number written in big endian format.
 * If the magic number when accessed as its native data
 * type isn't equal to the defined value AUDIO_FILE_MAGIC,
 * then the current CPU doesn't use big endian format.  It
 * is currently assumed that if a process doesn't use big
 * endian format it uses little endian format, but that
 * may not always be true in the future.
 * 
 * Date: 1/29/93
 * Author: czei@sunpix.east.sun.com
 *
 */
unsigned audio_endian(unsigned char *buf)
{
    u_32 localMagic; /* the archdep.h and audio_filehdr.h define u_32 */
		     /* as the type of the magic number.              */

    localMagic = buf[0];  /* this should cast the first u32 value in
			     buf into the localMagic variable */
    
    if(localMagic == AUDIO_FILE_MAGIC)
	return AUDIO_ENDIAN_BIG;
    else
	return AUDIO_ENDIAN_SMALL;
}



/*
 * Try to decode buffer containing an audio file header into an audio header.
 */
int
audio_decode_filehdr(unsigned char *buf, Audio_hdr *hdrp, unsigned int *isize)
	             	     		/* buffer address */
	         	      		/* output audio header */
	        	       		/* output size of info */
{
	Audio_filehdr	fhdr;		/* temporary file header storage */

	/* test the magic number to determine the endian */ 
	hdrp->endian = audio_endian(buf);

	/* Decode the 32-bit integer header fields. */
	DECODE_LONG(buf, &fhdr.magic); buf += 4;
	DECODE_LONG(buf, &fhdr.hdr_size); buf += 4;
	DECODE_LONG(buf, &fhdr.data_size); buf += 4;
	DECODE_LONG(buf, &fhdr.encoding); buf += 4;
	DECODE_LONG(buf, &fhdr.sample_rate); buf += 4;
	DECODE_LONG(buf, &fhdr.channels);

	/* Check the magic number. */
	if (fhdr.magic != AUDIO_FILE_MAGIC)
		return (AUDIO_ERR_BADFILEHDR);

	/* Decode into the audio header structure. */
	hdrp->data_size = fhdr.data_size;
	hdrp->sample_rate = fhdr.sample_rate;
	hdrp->channels = fhdr.channels;

	/* Set the info field size (ie, number of bytes left before data). */
	*isize = fhdr.hdr_size - sizeof (Audio_filehdr);

	/* Check the data encoding. */
	switch (fhdr.encoding) {
	case AUDIO_FILE_ENCODING_MULAW_8:
		hdrp->encoding = AUDIO_ENCODING_ULAW;
		hdrp->bytes_per_unit = 1;
		hdrp->samples_per_unit = 1;
		break;
	case AUDIO_FILE_ENCODING_ALAW_8:
		hdrp->encoding = AUDIO_ENCODING_ALAW;
		hdrp->bytes_per_unit = 1;
		hdrp->samples_per_unit = 1;
		break;
	case AUDIO_FILE_ENCODING_LINEAR_8:
		hdrp->encoding = AUDIO_ENCODING_LINEAR;
		hdrp->bytes_per_unit = 1;
		hdrp->samples_per_unit = 1;
		break;
	case AUDIO_FILE_ENCODING_LINEAR_16:
		hdrp->encoding = AUDIO_ENCODING_LINEAR;
		hdrp->bytes_per_unit = 2;
		hdrp->samples_per_unit = 1;
		break;
	case AUDIO_FILE_ENCODING_LINEAR_24:
		hdrp->encoding = AUDIO_ENCODING_LINEAR;
		hdrp->bytes_per_unit = 3;
		hdrp->samples_per_unit = 1;
		break;
	case AUDIO_FILE_ENCODING_LINEAR_32:
		hdrp->encoding = AUDIO_ENCODING_LINEAR;
		hdrp->bytes_per_unit = 4;
		hdrp->samples_per_unit = 1;
		break;
	case AUDIO_FILE_ENCODING_FLOAT:
		hdrp->encoding = AUDIO_ENCODING_FLOAT;
		hdrp->bytes_per_unit = 4;
		hdrp->samples_per_unit = 1;
		break;
	case AUDIO_FILE_ENCODING_DOUBLE:
		hdrp->encoding = AUDIO_ENCODING_FLOAT;
		hdrp->bytes_per_unit = 8;
		hdrp->samples_per_unit = 1;
		break;
	case AUDIO_FILE_ENCODING_ADPCM_G721:
		hdrp->encoding = AUDIO_ENCODING_G721;
		hdrp->bytes_per_unit = 1;
		hdrp->samples_per_unit = 2;
		break;
	case AUDIO_FILE_ENCODING_ADPCM_G723_3:
		hdrp->encoding = AUDIO_ENCODING_G723;
		hdrp->bytes_per_unit = 3;
		hdrp->samples_per_unit = 8;
		break;
	case AUDIO_FILE_ENCODING_ADPCM_G723_5:
		hdrp->encoding = AUDIO_ENCODING_G723;
		hdrp->bytes_per_unit = 5;
		hdrp->samples_per_unit = 8;
		break;

	default:
		return (AUDIO_ERR_BADFILEHDR);
	}
	return (AUDIO_SUCCESS);
}

/*
 * Encode a file header from the supplied Audio_hdr structure and
 * store in the supplied char* buffer. blen is the size of the
 * buffer to store the header in.
 * If 'infop' is not NULL, it is the address of a buffer containing 'info'
 * data.  'ilen' specifies the size of this buffer.
 * The entire file header will be zero-padded to a double-word boundary.
 */
int
audio_encode_filehdr(Audio_hdr *hdrp,
                     char *infop,
                     unsigned int ilen,
                     unsigned char *buf,
                     unsigned int *blen)
	         	      		/* audio header */
	    		       		/* info buffer pointer */
	        	     		/* info buffer size */
	    		     		/* output buffer */
	        	      		/* output buffer size */
{
	int		err;
	int		hdrsize;
	Audio_filehdr	fhdr;		/* temporary file header storage */
	/* unsigned char	*buf;		/* temporary buffer */
	unsigned char	*bp;

	fhdr.magic = AUDIO_FILE_MAGIC;	/* set the magic number */

	/*
	 * Set the size of the real header (hdr size + info size).
	 * If no supplied info, make sure a minimum size is accounted for.
	 * Also, round the whole thing up to double-word alignment.
	 */
	if ((infop == NULL) || (ilen == 0)) {
		infop = NULL;
		ilen = 4;
	}
	hdrsize = sizeof (Audio_filehdr) + ilen;
	fhdr.hdr_size = ROUND_DBL(hdrsize);

	/* Decode the audio header structure. */
	fhdr.data_size = hdrp->data_size;
	fhdr.sample_rate = hdrp->sample_rate;
	fhdr.channels = hdrp->channels;

	/* Check the data encoding. */
	switch (hdrp->encoding) {
	case AUDIO_ENCODING_ULAW:
		if (hdrp->samples_per_unit != 1)
			return (AUDIO_ERR_BADHDR);

		switch (hdrp->bytes_per_unit) {
		case 1:
			fhdr.encoding = AUDIO_FILE_ENCODING_MULAW_8; break;
		default:
			return (AUDIO_ERR_BADHDR);
		}
		break;
	case AUDIO_ENCODING_ALAW:
		if (hdrp->samples_per_unit != 1)
			return (AUDIO_ERR_BADHDR);

		switch (hdrp->bytes_per_unit) {
		case 1:
			fhdr.encoding = AUDIO_FILE_ENCODING_ALAW_8; break;
		default:
			return (AUDIO_ERR_BADHDR);
		}
		break;
	case AUDIO_ENCODING_LINEAR:
		if (hdrp->samples_per_unit != 1)
			return (AUDIO_ERR_BADHDR);

		switch (hdrp->bytes_per_unit) {
		case 1:
			fhdr.encoding = AUDIO_FILE_ENCODING_LINEAR_8; break;
		case 2:
			fhdr.encoding = AUDIO_FILE_ENCODING_LINEAR_16; break;
		case 3:
			fhdr.encoding = AUDIO_FILE_ENCODING_LINEAR_24; break;
		case 4:
			fhdr.encoding = AUDIO_FILE_ENCODING_LINEAR_32; break;
		default:
			return (AUDIO_ERR_BADHDR);
		}
		break;
	case AUDIO_ENCODING_FLOAT:
		if (hdrp->samples_per_unit != 1)
			return (AUDIO_ERR_BADHDR);

		switch (hdrp->bytes_per_unit) {
		case 4:
			fhdr.encoding = AUDIO_FILE_ENCODING_FLOAT; break;
		case 8:
			fhdr.encoding = AUDIO_FILE_ENCODING_DOUBLE; break;
		default:
			return (AUDIO_ERR_BADHDR);
		}
		break;
	case AUDIO_ENCODING_G721:
		if (hdrp->bytes_per_unit != 1)
			return (AUDIO_ERR_BADHDR);
		else if (hdrp->samples_per_unit != 2)
			return (AUDIO_ERR_BADHDR);
		else
			fhdr.encoding = AUDIO_FILE_ENCODING_ADPCM_G721;
		break;
	case AUDIO_ENCODING_G723:
		if (hdrp->samples_per_unit != 8)
			return (AUDIO_ERR_BADHDR);
		else if (hdrp->bytes_per_unit == 3)
			fhdr.encoding = AUDIO_FILE_ENCODING_ADPCM_G723_3;
		else if (hdrp->bytes_per_unit == 5)
			fhdr.encoding = AUDIO_FILE_ENCODING_ADPCM_G723_5;
		else
			return (AUDIO_ERR_BADHDR);
		break;
	default:
		return (AUDIO_ERR_BADHDR);
	}

	/* copy the fhdr into the supplied buffer - make sure it'll fit */
	if (*blen < (unsigned)fhdr.hdr_size) {
		/* XXX - is this apropriate? */
		return (AUDIO_EOF);
	}

	/* reset blen to actual size of hdr data */
	*blen = (unsigned)fhdr.hdr_size;

	bp = (unsigned char*) buf;

	/* Encode the 32-bit integer header fields. */
	ENCODE_LONG(&fhdr.magic, buf); buf += 4;
	ENCODE_LONG(&fhdr.hdr_size, buf); buf += 4;
	ENCODE_LONG(&fhdr.data_size, buf); buf += 4;
	ENCODE_LONG(&fhdr.encoding, buf); buf += 4;
	ENCODE_LONG(&fhdr.sample_rate, buf); buf += 4;
	ENCODE_LONG(&fhdr.channels, buf); buf += 4;

	/* Copy the info data */
	if (infop != NULL) {
		memcpy((char *)buf, infop, (int)ilen);
		buf += ilen;
	}

	if (fhdr.hdr_size > hdrsize)
		memset((char *)buf, '\0', (int)(fhdr.hdr_size - hdrsize));

	/* buf now has the data, just return ... */

	return (AUDIO_SUCCESS);
}

