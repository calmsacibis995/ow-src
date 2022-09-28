/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOSTREAM_INLINE_H
#define	_MULTIMEDIA_AUDIOSTREAM_INLINE_H

#ident	"@(#)AudioStream_inline.h	1.5	96/09/24 SMI"

// Inline routines for class AudioStream


// Return TRUE if the current AudioHdr is valid
inline Boolean AudioStream::
hdrset() const
{
	return (hdr.Validate() == AUDIO_SUCCESS);
}

// Return the current AudioHdr
inline AudioHdr AudioStream::
GetHeader()
{
	return (hdr);
}

// Set the length parameter
inline void AudioStream::
setlength(
	Double len)		// new length, in secs
{
	length = len;
}

// Set the length parameter, if possible
inline void AudioStream::
SetLength(
	Double len)		// new length, in secs
{
	// This may be used to set the expected length of a write-only stream
	if (!opened())
		length = len;
}

inline Double AudioStream::
GetLength() const
{
	return (length);
}

inline size_t AudioStream::
GetByteCount() const
{
	return (hdr.Time_to_Bytes(length));
}

#endif /* !_MULTIMEDIA_AUDIOSTREAM_INLINE_H */
