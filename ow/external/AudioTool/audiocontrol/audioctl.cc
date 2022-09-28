/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)audioctl.cc	1.5	93/02/25 SMI"

#include <stdio.h>
#include <unistd.h>

#include "audioctl.h"


// Audio Control Panel virtual base class


// Constructor
Audioctl::
Audioctl(
	ptr_t		p):
	ctldev(NULL), parent(p), type((AudioDeviceType)-1)
{
}

// Destructor
Audioctl::
~Audioctl()
{
	if (ctldev != NULL)
		delete ctldev;
}


// Open a new audio control device for the panel
// returns 0 if success; else -1
int Audioctl::
newdevice(
	char*		device)
{
	AudioDevicectl*	newdev;

	if (device == NULL)
		device = "";
	newdev = new AudioDevicectl(device);
	if (newdev->Open() != AUDIO_SUCCESS) {
		delete newdev;
		return (-1);
	}

	// New device is ok, close the old device
	if (ctldev != NULL)
		delete ctldev;
	ctldev = newdev;
	ctldev->SetSignal(TRUE);

	// Init the device type and state so Update() will set everything
	info.Clear();
	type = (AudioDeviceType) -1;
	return (0);
}

// Set the device field of the play control panel
// returns 0 if success; else -1
int Audioctl::
Setdevice(
	char*		device)
{
	int		err;

	err = newdevice(device);
	Update();

	// Subclass this to display error messages
	return (err);
}

// Get the real name of the device
char* Audioctl::
Getdevice()
{
	if (ctldev == NULL)
		return (MGET("no audio device"));
	return (ctldev->GetName());
}

// Return the width of the panel
int Audioctl::
Getwidth()
{
	return (min_width);
}
