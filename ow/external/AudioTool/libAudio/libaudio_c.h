/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_LIBAUDIO_C_H
#define	_MULTIMEDIA_LIBAUDIO_C_H

#ident	"@(#)libaudio_c.h	1.5	92/12/14 SMI"


#ifndef __cplusplus

#include <math.h>
#include <float.h>		/* DBL_MAX is defined here */
#include "libaudio.h"
#include "audio_hdr.h"

#define	AUDIO_UNKNOWN_TIME	(DBL_MAX)

typedef void*		Audio_Object;
typedef Audio_Object	AudioBuffer;
typedef Audio_Object	AudioFile;
typedef Audio_Object	AudioPipe;
typedef Audio_Object	AudioDevice;
typedef Audio_Object	AudioExtent;
typedef Audio_Object	AudioList;

typedef int		AudioError;

extern char*		Audio_errmsg(AudioError);

extern AudioFile	Audio_new_File(char* path, int mode);
extern AudioDevice	Audio_new_Device(char* path, int mode);

extern void		AudioDevice_SetBlocking(AudioDevice dev, int on);
extern void		AudioDevice_SetSignal(AudioDevice dev, int on);

extern int		Audio_Copy(Audio_Object from, Audio_Object to);
extern double		Audio_GetLength(Audio_Object obj);

extern void		Audio_Init();	/* XXX - not needed in SVr4 */


#else	/* if C++ compile... */
extern "C" {
extern void		Audio_Init();	/* XXX - not needed in SVr4 */
}
#endif

#endif /* !_MULTIMEDIA_LIBAUDIO_C_H */
