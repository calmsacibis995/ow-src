/* Copyright (c) 1990 by Sun Microsystems, Inc. */
#ident	"@(#)libaudio_c.cc	1.11	96/02/20 SMI"

// #include <iostream.h>
// #include <stdiostream.h>

#include "AudioFile.h"
#include "AudioDevice.h"
#include "AudioBuffer.h"
#include "AudioExtent.h"
#include "AudioList.h"
#include "AudioLib.h"

// XXX - tmp hack
#ifdef SUNOS41
// this is to link in __head from libC.so for patch version of cfront
extern struct __linkl *__head;
struct __linkl **__LinkInHead = (struct __linkl **)(& __head );
extern "C" { void _main(); };
#endif

// C wrappers for C++ object methods


extern "C" {

// XXX - Initialize C++ runtime, in case linked with cc
void
Audio_Init()
{
//	cin.sync_with_stdio();
//	cout.sync_with_stdio();
//	cerr.sync_with_stdio();
#ifdef SUNOS41
	_main(); // to init static classes, etc.
#endif
}


// Convert audio error code to message
char*
Audio_errmsg(
	int		code)
{
	AudioError	err;

	err = code;
	return (err.msg());
}

// Convert open mode to FileAccess
FileAccess
mode_to_access(
	int		mode)
{
	if ((mode & O_ACCMODE) == O_RDONLY)
		return ((FileAccess)ReadOnly);
	if ((mode & O_ACCMODE) == O_WRONLY)
		return ((FileAccess)WriteOnly);
	if ((mode & O_ACCMODE) == O_RDWR)
		return ((FileAccess)ReadWrite);
}

// Set up a file and open it
AudioFile*
Audio_new_File(
	char*		path,
	int		mode)
{
	AudioFile*	f;

	f = new AudioFile(path, mode_to_access(mode));
	if (f->Open() != AUDIO_SUCCESS) {
		delete f;
		f = 0;
	}
	return (f);
}

AudioDevice*
Audio_new_Device(
	char*		path,
	int		mode)
{
	AudioDevice*	f;

	f = new AudioDevice(path, mode_to_access(mode));
	if ((mode & O_NDELAY) || (mode & O_NONBLOCK))
		f->SetBlocking(FALSE);
	if (f->Open() != AUDIO_SUCCESS) {
		delete f;
		f = 0;
	}
	return (f);
}

// Set blocking/non-blocking mode
void
AudioDevice_SetBlocking(
	AudioDevice*	dev,
	int		on)
{
	dev->SetBlocking((Boolean)on);
}

// Set signal mode
void
AudioDevice_SetSignal(
	AudioDevice*	dev,
	int		on)
{
	dev->SetSignal((Boolean)on);
}

// Copy an entire audio object
int
Audio_Copy(
	Audio*		from,
	Audio*		to)
{
	Double frompos = 0. ;
        Double topos   = 0. ;
	Double length  = AUDIO_UNKNOWN_SIZE;

	return (AudioCopy(from, to, frompos, topos, length));
}



double
Audio_GetLength(Audio* obj)
{
	return (obj->GetLength());
}

#ifdef notyet
Audio_hdr *
Audio_GetHeader(AudioHeader *obj)
{
	return (obj->GetHeader());
}

void
Audio_SetHeader(AudioHeader *obj, Audio_hdr *h)
{
	obj->SetHeader(h);
}

void
Audio_SetMode(AudioStream *obj, int m)
{
	obj->SetMode(m);
}

void
Audio_SetPosition(AudioStream *obj, double p)
{
	obj->SetPosition(p);
}

int
Audio_SetPathname(AudioUnixFile *obj, char *path)
{
	return (obj->SetPathname(path));
}

int
Audio_Close(AudioStream *obj)
{
	return (obj->Close());
}

int
Audio_DupList_Fast(AudioList *obj, AudioList **newl)
{
	AudioList	*nl;
	int		err;

	nl = new AudioList();
	err = obj->Dup(nl);
	*newl = nl;
	return (err);
}

int
Audio_DupList(AudioList *obj, double in, double out, AudioList **newl)
{
	AudioList	*nl;
	int		err;

	nl = new AudioList();
	err = obj->Dup(in, out, nl);
	*newl = nl;
	return (err);
}

int
Audio_InsertAfter(AudioList *obj, AudioExtent *newe, AudioExtent *olde)
{
	return (obj->InsertAfter(newe, olde));
}

void
Audio_PrintList(AudioList *obj)
{
	// XXX - print methods dump core
	// obj->Print();
}

void
Audio_SetExtent(AudioExtent *obj, AudioStream *strm, double start, double end)
{
	obj->Set(strm, start, end);
}

#endif /*notyet*/

}
