/* Copyright (c) 1992 by Sun Microsystems, Inc. */
#ident	"@(#)status.cc	1.10	92/11/13 SMI"

#include <stdio.h>
#include <unistd.h>

#include "status.h"


// Status Panel implementation


// Constructor
Statusctl::
Statusctl(
	ptr_t		p):
	Audioctl(p)
{
}

// Get the latest device state and update the panels appropriately.
void Statusctl::
Update()
{
	AudioInfo	newinfo;
	AudioInfo	change;
	Boolean		flag;

	if (ctldev == NULL) {
		info.Clear();
		return;
	}

	// Get the current device state
	(void) ctldev->GetState(newinfo);

	// If the device type may have changed, get the new type
	// and update the panel appearance to match
	if (type == (AudioDeviceType) -1) {
		type = ctldev->GetDeviceType();
	}

	// XXX - patch output buffer size??


#define	Modify(X)	if (newinfo->X != info->X)	\
			    { change->X = newinfo->X; flag = TRUE; }

	// Only change settings that changed, to avoid flashing
	Modify(play.open);
	Modify(play.pause);
	Modify(play.active);
	Modify(play.error);
	Modify(play.waiting);
	Modify(play.eof);
	Modify(play.samples);
#ifndef SUNOS41
	Modify(play.buffer_size);
#endif
	Modify(play.sample_rate);
	Modify(play.channels);
	Modify(play.encoding);
	Modify(play.precision);

	Modify(record.open);
	Modify(record.pause);
	Modify(record.active);
	Modify(record.error);
	Modify(record.waiting);
	Modify(record.samples);
#ifndef SUNOS41
	Modify(record.buffer_size);
#endif
	Modify(record.sample_rate);
	Modify(record.channels);
	Modify(record.encoding);
	Modify(record.precision);

	/* If something changed */
	if (flag) {
		info = newinfo;		/* save latest values */
		update_display(change);
	}
}
