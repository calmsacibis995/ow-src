/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIO_INLINE_H
#define	_MULTIMEDIA_AUDIO_INLINE_H

#ident	"@(#)Audio_inline.h	1.6	91/08/16 SMI"

// Inline routines for class Audio


// Return object id
inline int Audio::
getid() const
{
	return (id);
}

// Return TRUE if the object is referenced
inline Boolean Audio::
isReferenced() const
{
	return (refcnt > 0);
}

// Access routine for retrieving the current read position pointer
inline Double Audio::
ReadPosition() const
{
	return (readpos);
}

// Access routine for retrieving the current write position pointer
inline Double Audio::
WritePosition() const
{
	return (writepos);
}

// Return the name of an audio object
inline char* Audio::
GetName() const
{
	return (name);
}

// Set the error function callback address
inline void Audio::
SetErrorFunction(
	AudioErrfunc	func)			// function address
{
	errorfunc = func;
}

// Default get header at position routine does a normal GetHeader
inline AudioHdr Audio::
GetHeader(
	Double)
{
	return (GetHeader());
}

#endif /* !_MULTIMEDIA_AUDIO_INLINE_H */
