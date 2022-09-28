/* Copyright (c) 1992 by Sun Microsystems, Inc. */
#ident	"@(#)AudioDevicectl.cc	1.6	96/02/20 SMI"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "AudioDevicectl.h"

// class AudioDevicectl methods


// Constructor with optional path argument
AudioDevicectl::
AudioDevicectl(
	const char*		path):		// filename
	AudioDevice(path, ReadWrite)
{
}


// Enable/disable SIGPOLL on state changes
AudioError AudioDevicectl::
SetSignal(
	Boolean		on)			// True to enable
{
	// Set the streams flag
	if (ioctl(getfd(), I_SETSIG, (on ? S_MSG : 0)) < 0)
		return (RaiseError(AUDIO_UNIXERROR));
	return (AUDIO_SUCCESS);
}

// Get input encoding
AudioHdr AudioDevicectl::
GetReadHeader()
{
	clearhdr();
	return (AudioDevice::GetReadHeader());
}


// Open an audio control device with the given name and mode
AudioError AudioDevicectl::
tryopen(
	const char*	devname,
	int)
{
	struct stat	st;
	char*		ctlname;
	int		desc;
	AudioInfo	info;
	AudioError	err;

	// If the name is changing, set the new one
	if (devname != GetName())
		SetName(devname);

	// XXX - convert name to device name, using audio config file
	// For now, append "ctl" to the device name
	ctlname = new char[strlen(devname) + 4];
	(void) strcpy(ctlname, devname);
	(void) strcat(ctlname, "ctl");

	// Check and open the control device.
	err = AUDIO_SUCCESS;
	if (stat(ctlname, &st) < 0) {
		err = AUDIO_UNIXERROR;
	} else if (!S_ISCHR(st.st_mode)) {
		err = AUDIO_ERR_NOTDEVICE;
	} else if ((desc = open(ctlname, O_RDWR)) < 0) {
		err = AUDIO_UNIXERROR;
	}
	delete ctlname;
	if (err)
		return (err);

	// Set the file descriptor (this marks the file open)
	setfd(desc);

	err = GetState(info);
	if (err != AUDIO_SUCCESS) {
		(void) close(desc);
		setfd(-1);
		return (err);
	}
	// Get the device type
	decode_devtype();

	return (AUDIO_SUCCESS);
}
