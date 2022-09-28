/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_AUDIOUNIXFILE_INLINE_H
#define	_MULTIMEDIA_AUDIOUNIXFILE_INLINE_H

#ident	"@(#)AudioUnixfile_inline.h	1.3	90/12/17 SMI"

// Inline routines for class AudioUnixfile


// Return the file descriptor
inline int AudioUnixfile::
getfd() const {
	return (fd);
}

// Set the file descriptor
inline void AudioUnixfile::
setfd(
	int newfd)				// new file descriptor
{
	fd = newfd;
}


// Return TRUE if fd is valid
inline Boolean AudioUnixfile::
isfdset() const {

	return (fd >= 0);
}

// Return TRUE if file hdr read/written
inline Boolean AudioUnixfile::
isfilehdrset() const {

	return (filehdrset);
}

// Return TRUE if stream is open
inline Boolean AudioUnixfile::
opened() const {

	return (isfdset() && isfilehdrset());
}

// Return the access mode
inline FileAccess AudioUnixfile::
GetAccess() const {
	return (mode);
}

// Return the blocking i/o mode
inline Boolean AudioUnixfile::
GetBlocking() const {
	return (block);
}

#endif /* !_MULTIMEDIA_AUDIOUNIXFILE_INLINE_H */
