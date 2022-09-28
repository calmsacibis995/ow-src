/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to Sun Microsystems, Inc.  The name of Sun Microsystems, Inc.
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission.  This software
 * is provided ``as is'' without express or implied warranty.
 */
#ident	"@(#)xmit.c	1.17	96/02/20 SMI"


/*
 * Radio Free Ethernet transmitter tool.
 *
 * This program builds a GUI for the RFE transmitter.
 * It execs radio_xmit to perform the audio/network operations,
 * communicating commands and status through pipes.
 *
 * The RFE multicast facilities may be used and/or extended for
 * a variety of multimedia conferencing applications. The idea
 * here is that you would probably build a new GUI for such apps,
 * but could use radio_xmit as is, or with backward-compatible
 * enhancements.
 *
 * This program is furnished both as an example of audio programming
 * and as an interesting network-based audio application.
 */

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stropts.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <group.h>
#include "xmit_ui.h"

#include <multimedia/libaudio.h>
#include <multimedia/audio_device.h>
#include "radio.h"
#include "radio_network.h"
#include "netbroadcast.h"


/* Station description structure */
typedef struct Station {
	char*		signon;				/* signon file */
	char*		input;				/* audio input file */
	char*		signoff;			/* signoff file */
	int		shutoff;			/* shutoff flag */
	int		format;				/* audio format */
	char*		address;			/* multicast address */
	unsigned	hops;				/* multicast hopcnt */
} Station_t;

/* A linked list of station structures */
typedef struct stlist {
	struct stlist*	next;				/* next in list */
	char		name[RADIO_CALLNAME_SIZE + 1];	/* callname */
	Station_t*	station;			/* station config */
	int		seen;				/* TRUE if written */
} Stationlist_t;


/* Broadcast state variables */
int			State = POWER_OFF;		/* current state */
int			XmitState = -1;			/* reported state */
int			Agc = TRUE;			/* TRUE when on */
int			Bufsiz = 0;			/* buffer length */
Station_t		CurrentStation;			/* station info */
char			Callname[RADIO_CALLNAME_SIZE + 1];
int			Audioctl_fd = -1;		/* audio control dev */
char*			Service = NULL;			/* port number */
char*			Host = NULL;			/* multicast host */
int			Resync = FALSE;			/* station resync */
int			Multicast = TRUE;		/* TRUE if ok */
int			Hopcount = RADIO_DEFAULT_RANGE;
int			Debug = FALSE;			/* debug flag */
char*			prog;				/* program name */

#define	POWER_IS_ON	(!(State == POWER_OFF))
#define	POWER_IS_OFF	(State == POWER_OFF)

/* State variables for .radiorc parsing */
Stationlist_t*		PresetList;			/* preset list head */
Stationlist_t*		CurrentPreset;			/* used in writing */
int			Reading;			/* FALSE if writing */
int			Rc_outfd = -1;			/* output file desc */

/* Connections to transmitter process */
char*			Xmit_prefix = "exec ";		/* sh prefix */
char*			Xmit_basename = "radio_xmit";	/* program name */
char*			Xmit_args = " -C Report";	/* invocation args */
char*			Xmit_command;			/* invocation cmd */
int			Xmit_childpid = -1;		/* pid of xmit proc */
int			Xmit_tofd;			/* to write to proc */
int			Xmit_fromfd;			/* to read from proc */
int			Xmit_established;		/* set on contact */

/* XView-related variables */
Attr_attribute			INSTANCE;		/* Devguide key */
xmit_XmitFrame_objects*		MFrame = NULL;
xmit_StationFrame_objects*	SFrame = NULL;
Menu				StationMenu;
Icon				CurrentIcon;
Xv_opaque			Icon_on;
Xv_opaque			Icon_onmask;
Xv_opaque			Icon_off;
Xv_opaque			Icon_offmask;
int				Volume = -1;		/* cache slider value */
int				Advanced = FALSE;	/* TRUE if expanded */
int				Height_def;		/* size of - panel */
int				Height_adv;		/* size of + panel */

static unsigned short		image_bits[] = {
#include "xmit_on.icon"
};
static unsigned short		mask_bits[] = {
#include "xmit_on.mask.icon"
};


/* Macro to translate strings */
#define	T(str)		(dgettext("radio-labels", str))
#define	MAX_GAIN	(100)				/* slider range */

/* Define error reporting routines for the frame and pin-up */
#define	MError(str)	error(MFrame->XmitFrame, str);
#define	SError(str)	error(SFrame->StationFrame, str);

/* XView routines declared below */
void			error(Xv_opaque, char*);
void			state_set();
void			icon_switch();
void			volume_enable();
void			volume_disable();
void			volume_setval(double);
void			list_resync();
void			register_reaper();
void			rc_read();
void			rc_write();

/* Global variables */
extern int		optind;
extern char*		optarg;

void
usage()
{
	FPRINTF(stderr, "%s\n\t%s ",
	    T("Radio Free Ethernet Transmitter -- usage:"), prog);
	FPRINTF(stderr, T("[-s service] [-h host] [-c]\nwhere:\n"));
	FPRINTF(stderr, T("\t-s\tSpecify service name or port number\n"));
	FPRINTF(stderr, T("\t\t\tDefault service: '%s'\n"),
	    RADIO_DEFAULT_SERVICE);
	FPRINTF(stderr, T("\t-h\tSpecify multicast hostname or ip address\n"));
	FPRINTF(stderr, T("\t\t\tDefault hostname: '%s'\n"),
	    RADIO_DEFAULT_ADDRESS);
	FPRINTF(stderr, T("\t-c\tSet short input delay for conferencing\n"));
	exit(1);
}


/* Release allocated strings for a given station structure */
void
station_clear(
	Station_t*		stp)
{
	if (stp->signon != NULL) {
		(void) free(stp->signon);
		stp->signon = NULL;
	}
	if (stp->input != NULL) {
		(void) free(stp->input);
		stp->input = NULL;
	}
	if (stp->signoff != NULL) {
		(void) free(stp->signoff);
		stp->signoff = NULL;
	}
	if (stp->address != NULL) {
		(void) free(stp->address);
		stp->address = NULL;
	}
}

/* Copy a station to another */
void
station_copy(
	Station_t*		from,
	Station_t*		to)
{
	replace_string(&to->signon, from->signon);
	replace_string(&to->input, from->input);
	replace_string(&to->signoff, from->signoff);
	to->shutoff = from->shutoff;
	to->format = from->format;
	replace_string(&to->address, from->address);
	to->hops = from->hops;
}


/* Configure the current station with new parameters */
int
station_configure(
	char*			name,
	Station_t*		stp)
{
	/* If broadcasting, cannot reconfigure */
	if (POWER_IS_ON)
		return (FALSE);

	STRNCPY(Callname, name, RADIO_CALLNAME_SIZE);
	Callname[RADIO_CALLNAME_SIZE] = '\0';

	station_copy(stp, &CurrentStation);
	return (TRUE);
}

/* Set the audio input volume to a given value */
void
audio_setvolume(
	double			gain)
{
	(void) audio_set_record_gain(Audioctl_fd, &gain);
}

/* Sync the slider to the current audio input volume */
void
audio_syncvolume()
{
	double			gain;

	if (Audioctl_fd >= 0) {
		(void) audio_get_record_gain(Audioctl_fd, &gain);
		volume_setval(gain);
	}
}

/* Close the audio control device, inactivate the slider */
void
audio_close()
{
	if (Audioctl_fd >= 0)
		(void) close(Audioctl_fd);
	Audioctl_fd = -1;
	volume_disable();
}

/* Open the audio control device, read the input gain, activate the slider */
void
audio_open(
	char*			dev)
{
	char			name[128];
	char			msg[512];
	Audio_info		info;

	/* If already open, close it */
	if (Audioctl_fd >= 0)
		(void) close(Audioctl_fd);
	SPRINTF(name, "%sctl", dev);

	/* Open and verify the audio control device */
	Audioctl_fd = open(name, O_RDWR);
	if ((Audioctl_fd < 0) ||
	    (ioctl(Audioctl_fd, I_SETSIG, S_MSG) < 0) ||
	    (audio_getinfo(Audioctl_fd, &info) != AUDIO_SUCCESS)) {
		struct stat	st;

		/* If audio input is a device, put up an error message */
		if ((stat(dev, &st) >= 0) && S_ISCHR(st.st_mode)) {
			SPRINTF(msg, T("Cannot open %s"), name);
			MError(msg);
		}
		audio_close();
		return;
	}

	/* Activate the slider and sets its value correctly */
	volume_enable();
	audio_syncvolume();
}

/* Fork and exec the radio transmitter process */
int
fork_xmit()
{
	int			i;
	int			pipeto[2];
	int			pipefrom[2];
	char			msg[512];
	sigset_t		set;

	/* Do nothing if there's already a transmitter running */
	if (Xmit_childpid != -1)
		return (TRUE);
	Xmit_established = FALSE;

	/* Open pipes for bidirectional transfer */
	if ((pipe(pipeto) < 0) || (pipe(pipefrom) < 0)) {
failure:
		SPRINTF(msg, T("Could not execute %s process"), Xmit_basename);
		MError(msg);
		return (FALSE);
	}

	switch (Xmit_childpid = fork()) {
	case 0:					/* child */
		/* Redirect descriptors */
		dup2(pipeto[0], fileno(stdin));
		dup2(pipefrom[1], fileno(stderr));

		/* Close all other descriptors and reset signal handling */
		for (i = getdtablesize(); i > 2; i--)
			(void) close(i);
		for (i = 0; i < NSIG; i++)
			(void) signal(i, SIG_DFL);
		(void) sigfillset(&set);
		(void) sigprocmask(SIG_UNBLOCK, &set, 0);

		/* Exec the transmitter process */
		execl("/bin/sh", "sh", "-c", Xmit_command, NULL);
		exit(1);

	default:				/* parent */
		/* Close unused descriptors */
		Xmit_tofd = pipeto[1];
		(void) close(pipeto[0]);
		Xmit_fromfd = pipefrom[0];
		(void) close(pipefrom[1]);
		{
			/* Register input and wait procs */
			register_reaper();
			break;
		}
						/* fall through on error */

	case -1:				/* fork error */
		(void) close(pipeto[0]);
		(void) close(pipeto[1]);
		(void) close(pipefrom[0]);
		(void) close(pipefrom[1]);
		goto failure;
	}
	return (TRUE);
}

/*
 * Send a command line to the transmitter.
 * The string should be terminated with a newline.
 */
int
send_xmitmsg(
	char*			str)
{
	int			i;

	i = strlen(str);
	if ((Xmit_tofd < 0) ||
	    (write(Xmit_tofd, str, i) != i)) {
		return (FALSE);
	}
	if (Debug)
		FPRINTF(stderr, "%s: %s", prog, str);
	return (TRUE);
}

/* Start broadcasting the next input file */
int
set_xmitfile()
{
	char*			file;
	char			msg[512];

	/* If there is an audio control device open, close it */
	audio_close();

	switch (State) {
	case ON_THE_AIR:	file = CurrentStation.input; break;
	case SIGN_OFF:		file = CurrentStation.signoff; break;
	default:
		return (FALSE);
	}

	/* Compose a message for the transmitter */
	SPRINTF(msg, "%s=%s\n", RADIOCMD_INPUT, file);
	if (!send_xmitmsg(msg))
		return (FALSE);

	/* Open the audio control device, if any */
	audio_open(file);
	return (TRUE);
}


/*
 * Shut down transmission.
 * This is being called in response to a transmitter reporting an error or
 * power-off, or dying.
 */
void
poweroff()
{
	/* Transmitter is off or dead */
	audio_close();
	State = POWER_OFF;
	state_set(POWER_OFF);
}

/* Start transmission */
int
poweron()
{
	char*			file;
	char			msg[512];
	char			bufsiz[64];
	char			infile[256];

	/* Validate some parameters up front */
	if (net_parse_address(CurrentStation.address, NULL) < 0) {
		if (isdigit(CurrentStation.address[0])) {
			MError(T("Invalid multicast address"));
		} else {
			MError(T("Invalid multicast hostname"));
		}
		goto error;
	}
	file = CurrentStation.input;
	if (CurrentStation.input[0] == '\0') {
		MError(T("No audio input specified"));
		goto error;
	}
	if (access(CurrentStation.input, R_OK) < 0) {
		MError(T("Cannot read audio input"));
		goto error;
	}

	/* Make sure the transmitter process is running */
	if (!fork_xmit())
		goto error;

	/* If there is a signon file, get ready to sign on */
	if (CurrentStation.signon[0] != '\0') {
		file = CurrentStation.signon;
		State = SIGN_ON;
	} else {
		State = ON_THE_AIR;
	}

	/* If an input buffer size is specified, use it */
	if (Bufsiz != 0) {
		SPRINTF(bufsiz, "%s=%d", RADIOCMD_BUFSIZ, Bufsiz);
	} else {
		bufsiz[0] = '\0';
	}

	/* If there is a sign-on message, verify the input file first */
	if (State == SIGN_ON) {
		SPRINTF(infile, "%s=%s", RADIOCMD_INPUT, CurrentStation.input);
	} else {
		infile[0] = '\0';
	}

	/*
	 * Compose a message for the transmitter.
	 * If there is a signon file, set the main input first
	 * to get the transmitter to verify it before signing on.
	 */
	SPRINTF(msg,
	    "%s %s=%s %s=%s %s=%s %s=%s %s=%s %s=%s %s=%d %s %s=%s %s %s\n",
	    RADIOCMD_STOP,
	    RADIOCMD_STATION, Callname,
	    RADIOCMD_SERVICE, Service,
	    RADIOCMD_ADDRESS, CurrentStation.address,
	    RADIOCMD_FORMAT, CurrentStation.format == RADIO_TYPE_ULAW ?
	    "Uncompressed" : "Compressed",
	    RADIOCMD_AUTOSTOP, CurrentStation.shutoff ? "On" : "Off",
	    RADIOCMD_AGC, Agc ? "On" : "Off",
	    RADIOCMD_RANGE, CurrentStation.hops,
	    infile,
	    RADIOCMD_INPUT, file,
	    bufsiz,
	    RADIOCMD_START);
	if (!send_xmitmsg(msg)) {
error:
		poweroff();
		return (FALSE);
	}

	/* Toggle the power button on */
	xv_set(MFrame->PowerButton, PANEL_VALUE, 1, NULL);

	/* Busy the tool until status is sent */
	xv_set(MFrame->XmitFrame, FRAME_BUSY, TRUE, NULL);
	audio_open(file);
	return (TRUE);
}

/*
 * Broadcast signoff, if any.
 * Otherwise, tell the transmitter to stop.
 */
void
signoff()
{
	char			msg[512];

	/* Sign-off if currently broadcasting */
	if (POWER_IS_ON && (CurrentStation.signoff[0] != '\0')) {
		switch (State) {
		case SIGN_ON:
		case ON_THE_AIR:
			State = SIGN_OFF;
			if (set_xmitfile()) {
				/* Switch status msg */
				state_set(SIGN_OFF);
				return;
			}
		}
	}

	/* Send a power off message to the transmitter */
	SPRINTF(msg, "%s\n", RADIOCMD_STOP);
	if (!send_xmitmsg(msg)) {
		/* If error sending message, kill the transmitter */
		if (Xmit_childpid != -1)
			(void) kill(Xmit_childpid, SIGINT);
		else
			poweroff();
	}
}

/* Handle power off status */
/* ARGSUSED */
int
status_off(
	char*			arg)
{
	poweroff();
	return (TRUE);
}

/* Handle broadcast status */
/* ARGSUSED */
int
status_xmit(
	char*			arg)
{
	/* Update state */
	switch (State) {
	case SIGN_ON:
	case SIGN_OFF:
	case ON_THE_AIR:
		state_set(State);
		break;
	default:
		state_set(ON_THE_AIR);
		break;
	}
	return (TRUE);
}

/* Handle squelch status */
/* ARGSUSED */
int
status_squelch(
	char*			arg)
{
	/* Update state only if in normal broadcast */
	switch (State) {
	case SIGN_ON:
	case SIGN_OFF:
		break;
	default:
		state_set(QUIET);
		break;
	}
	return (TRUE);
}

/* Handle end-of-file status */
/* ARGSUSED */
int
status_eof(
	char*			arg)
{
	switch (State) {
	case SIGN_ON:
		State = ON_THE_AIR;
		if (set_xmitfile())
			break;
					/* if error, fall through */
	case SIGN_OFF:
	default:
		/* Main broadcast complete: sign off */
		signoff();
		break;
	}
	return (TRUE);
}

/* Transmitter status parse table */
Radio_cmdtbl	statuslist[] = {
	{RADIOCMD_POWEROFF,	status_off},
	{RADIOCMD_BROADCAST,	status_xmit},
	{RADIOCMD_SQUELCH,	status_squelch},
	{RADIOCMD_EOF,		status_eof},
	{NULL,			(int(*)()) NULL},
};


/* Cleanup on exit; kill the transmitter program */
void
exit_cleanup()
{
	if (Xmit_childpid != -1) {
		(void) kill(Xmit_childpid, SIGINT);
	}
	(void) fflush(stderr);
}


/* XView interface section: derived from xmit_stubs.c */

/* Display an error message in the footer of the specified frame */
void
error(
	Xv_opaque		frame,
	char*			str)
{
	xv_set(frame, FRAME_LEFT_FOOTER, str, NULL);
}

/* Toggle items that change state when Power is turned on or off */
toggle_power()
{
	int			nth;

	/* Get the number of interesting items (skip title, blank, and Edit) */
	nth = (int) xv_get(StationMenu, MENU_NITEMS) - 3;

	/* Toggle the active state of the station menu and agc items */
	while (nth > 0) {
		xv_set((Menu_item)xv_get(StationMenu, MENU_NTH_ITEM, nth + 1),
		    MENU_INACTIVE, POWER_IS_ON,
		    NULL);
		nth--;
	}
	xv_set(MFrame->AgcControl, PANEL_INACTIVE, POWER_IS_OFF, NULL);

	if (POWER_IS_OFF) {
		xv_set(MFrame->PowerButton, PANEL_VALUE, 0, NULL);
	}
}

/* Set the status message according to current state */
void
state_set(
	int			newstate)
{
	char*			str;

	if ((newstate == XmitState) && POWER_IS_ON)
		return;			/* no change in state */
	XmitState = newstate;

	switch (XmitState) {
	case POWER_OFF:		str = "Power Off"; break;
	case QUIET:		str = "No Broadcast"; break;
	case ON_THE_AIR:	str = "Broadcasting"; break;
	case SIGN_ON:		str = "Signing On"; break;
	case SIGN_OFF:		str = "Signing Off"; break;
	}
	if (Debug)
		FPRINTF(stderr, "%s: State message set to: %s\n", prog, str);

	xv_set(MFrame->StatusMessage, PANEL_LABEL_STRING, T(str), NULL);
	xv_set(MFrame->StatusGroup, GROUP_LAYOUT, TRUE, NULL);

	/* Make sure various things are set properly */
	toggle_power();
	icon_switch();

	if (XmitState == POWER_OFF) {
		/* If the current station was reconfigured, update it now */
		if (Resync)
			list_resync();
	}
}

/*
 * Notify callback function for `PowerButton'.
 */
void
power_notify(
	Panel_item		item,
	unsigned int		value,
	Event*			event)
{
	MError("");
	if (POWER_IS_OFF) {
		/* Turn power on */
		if (!poweron()) {
			poweroff();
			return;
		}
	} else {
		/* Sign the station off */
		signoff();
		if (POWER_IS_ON)
			xv_set(MFrame->XmitFrame, FRAME_BUSY, TRUE, NULL);
	}
}

/* Activate the volume slider */
void
volume_enable()
{
	xv_set(MFrame->VolumeControl, PANEL_INACTIVE, FALSE, NULL);
}

/* Deactivate the volume slider */
void
volume_disable()
{
	xv_set(MFrame->VolumeControl, PANEL_INACTIVE, TRUE, NULL);
}

/* Set a new value on the volume slider */
void
volume_setval(
	double			gain)
{
	int			value;

	/* Scale to slider range */
	value = irint((double)MAX_GAIN * gain);

	/* Only change the slider if the value has changed */
	if (value != Volume) {
		Volume = value;
		xv_set(MFrame->VolumeControl, PANEL_VALUE, Volume, NULL);
	}
}

/*
 * Notify callback function for `VolumeControl'.
 */
void
volume_notify(
	Panel_item		item,
	int			value,
	Event*			event)
{
	double			gain;

	/* Get the new slider value and rescale it */
	Volume = value;
	gain = ((double)Volume / (double)MAX_GAIN);
	audio_setvolume(gain);
}

/*
 * Notify callback function for `AgcControl'.
 */
void
agc_notify(
	Panel_item		item,
	int			value,
	Event*			event)
{
	char			msg[512];

	MError("");
	if (Agc == value)
		return;			/* no change */
	Agc = value;
	/* Compose a message for the transmitter */
	SPRINTF(msg, "%s=%s\n", RADIOCMD_AGC, Agc ? "On" : "Off");
	(void) send_xmitmsg(msg);
}


/* Set broadcast station parameters from a preset; enable the controls */
int
station_setup(
	char*			name,
	Station_t*		stp)
{
	/* Copy the station data into the current station structure */
	if (station_configure(name, stp)) {
		/* Set the station name; activate the power button */
		xv_set(MFrame->StationButtonText, PANEL_VALUE, Callname, NULL);
		xv_set(MFrame->PowerButton, PANEL_INACTIVE, FALSE, NULL);
		return (TRUE);
	}
	return (FALSE);
}

/*
 * Notify routine for `StationMenu' items.
 */
void
station_notify(
	Menu			menu,
	Menu_item		mi)
{
	Station_t*		stp;

	MError("");
	/* Get the station structure and fill in current station */
	stp = (Station_t*) xv_get(mi, MENU_VALUE);
	(void) station_setup((char*)xv_get(mi, MENU_STRING), stp);
}

/*
 * Menu handler for `StationMenu (Edit...)'.
 */
Menu_item
xmit_StationMenu_item1_callback(
	Menu_item		item,
	Menu_generate		op)
{
	switch (op) {
	case MENU_NOTIFY:
		MError("");
		xv_set(SFrame->StationFrame, XV_SHOW, TRUE, NULL);
		break;

	case MENU_DISPLAY:
	case MENU_DISPLAY_DONE:
	case MENU_NOTIFY_DONE:
		break;
	}
	return item;
}


/* Scrolling list maintenance routines */

/* Returns the item number of the selected item (or, -1 if none) */
int
list_selected()
{
	int			i;
	int			nrows;

	nrows = (int) xv_get(SFrame->StationList, PANEL_LIST_NROWS);
	for (i = 0; i < nrows; i++) {
		if (xv_get(SFrame->StationList, PANEL_LIST_SELECTED, i))
			return (i);
	}
	return (-1);
}

/* Returns the matching station item (or NULL, if none) */
Station_t*
list_find(
	char*			name)
{
	int			i;
	int			nrows;

	nrows = (int) xv_get(SFrame->StationList, PANEL_LIST_NROWS);
	for (i = 0; i < nrows; i++) {
		if (strcmp(name, (char*) xv_get(SFrame->StationList,
		    PANEL_LIST_STRING, i)) == 0) {
			/*
			 * Here is a station with the same name.
			 * Return its station information structure.
			 */
			return ((Station_t*) xv_get(SFrame->StationList,
			    PANEL_LIST_CLIENT_DATA, i));
		}
	}
	return (NULL);
}

/* Delete the specified item */
void
list_delete(
	int			nth)
{
	Station_t*		stp;
	Menu_item		mi;

	/* Get the station structure and deallocate it */
	stp = (Station_t*) xv_get(SFrame->StationList,
	    PANEL_LIST_CLIENT_DATA, nth);
	if (stp != NULL)
		station_clear(stp);
	(void) free((char*)stp);

	xv_set(SFrame->StationList,
	    PANEL_LIST_SELECT, nth, FALSE,
	    PANEL_LIST_DELETE, nth,
	    NULL);

	/*
	 * Delete the corresponding station menu item.
	 * Increment nth twice since menu items count from 1
	 * and the menu title is 1.
	 */
	nth += 2;
	mi = (Menu_item) xv_get(StationMenu, MENU_NTH_ITEM, nth);
	xv_set(StationMenu,
	    MENU_REMOVE_ITEM, mi,
	    MENU_DEFAULT, (int)xv_get(StationMenu, MENU_DEFAULT) - 1,
	    NULL);
	xv_destroy(mi);
	SError("");
}

/*
 * Get the current panel values for a station configuration.
 * Return them in an allocated station structure suitable for list_add().
 */
Station_t*
list_getconfig()
{
	Station_t*		stp;

	/* Create and fill in a new station structure */
	stp = (Station_t*) calloc(1, sizeof (*stp));
	stp->signon = copy_string((char*)
	    xv_get(SFrame->SignonItem, PANEL_VALUE));
	stp->input = copy_string((char*)
	    xv_get(SFrame->InputItem, PANEL_VALUE));
	stp->signoff = copy_string((char*)
	    xv_get(SFrame->SignoffItem, PANEL_VALUE));

	stp->shutoff = (int)xv_get(SFrame->ShutoffControl, PANEL_VALUE);
	switch ((int)xv_get(SFrame->FormatControl, PANEL_VALUE)) {
	case 0:
	default:
		stp->format = RADIO_TYPE_ULAW;
		break;
	case 1:
		stp->format = RADIO_TYPE_G721;
		break;
	}

	stp->address = copy_string((char*)
	    xv_get(SFrame->AddressItem, PANEL_VALUE));
	stp->hops = (int)xv_get(SFrame->HopcountItem, PANEL_VALUE);
	return (stp);
}


/* Add a scrolling list entry.  'item' is used to keep the panel up. */
void
list_add(
	Panel_item		item,
	Station_t*		stp,
	char*			name,
	int			nth)
{
	int			i;
	int			nrows;

	/* Replace or insert an entry */
	nrows = (int) xv_get(SFrame->StationList, PANEL_LIST_NROWS);
	for (i = 0; i < nrows; i++) {
		if (strcmp(name, (char*) xv_get(SFrame->StationList,
		    PANEL_LIST_STRING, i)) == 0) {
			/*
			 * Here is a station with the same name.
			 * Delete it and insert a new entry at the same place.
			 */
			list_delete(i);
			nth = i;
			break;
		}
	}

	/*
	 * If this is the current station, or if there is no current station,
	 * try to reset the parameters.
	 */
	SError("");
	if ((Callname[0] == '\0') || (strcmp(name, Callname) == 0)) {
		if (!station_setup(name, stp)) {
			SError(T("Current station parameters unchanged"));
			if (item != NULL)
				xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, 0);
			Resync = TRUE;		/* flag to resync later */
		}
	}

	/* Insert a new entry, append if item number not specified */
	if (nth < 0)
		nth = nrows;
	xv_set(SFrame->StationList,
	    PANEL_LIST_INSERT, nth,
	    PANEL_LIST_STRING, nth, name,
	    PANEL_LIST_CLIENT_DATA, nth, stp,
	    PANEL_LIST_SELECT, nth, TRUE,
	    NULL);

	/*
	 * Add an entry into the station menu corresponding to this station
	 * Increment nth since menu items count from 1 & insert goes after nth.
	 * Store the station data (but don't destroy it when deleting the item).
	 * Create the item inactive if already broadcasting.
	 */
	xv_set(StationMenu,
	    MENU_INSERT, nth + 1, (Menu_item)xv_create(NULL, MENUITEM,
	    MENU_STRING, copy_string(name),
	    MENU_VALUE, stp,
	    MENU_INACTIVE, POWER_IS_ON,
	    MENU_NOTIFY_PROC, station_notify,
	    MENU_RELEASE,
	    MENU_RELEASE_IMAGE,
	    NULL),
	    MENU_DEFAULT, (int)xv_get(StationMenu, MENU_DEFAULT) + 1,
	    NULL);
}

/*
 * Update the current station to match configuration.
 * This is called if the configuration changed while broadcasting.
 */
void
list_resync()
{
	Station_t*		stp;
	int			i;
	int			nrows;

	/* Replace or insert an entry */
	stp = list_find(Callname);
	if (stp != NULL) {
		/*
		 * Here is a station with the same name.
		 * Reset it to match the updated configuration.
		 */
		if (station_setup(Callname, stp))
			SError(T("Current station parameters updated"));
	}
	Resync = FALSE;
}

/*
 * Notify callback function for `StationList'.
 */
int
list_notify(
	Panel_item		item,
	char*			string,
	Xv_opaque		client_data,
	Panel_list_op		op,
	Event*			event,
	int			row)
{
	Station_t*		stp;

	SError("");
	stp = (Station_t*) client_data;
	switch (op) {
	case PANEL_LIST_OP_DESELECT:
		break;

	case PANEL_LIST_OP_SELECT:
		xv_set(SFrame->StationItem, PANEL_VALUE, string, NULL);
		xv_set(SFrame->SignonItem, PANEL_VALUE, stp->signon, NULL);
		xv_set(SFrame->InputItem, PANEL_VALUE, stp->input, NULL);
		xv_set(SFrame->SignoffItem, PANEL_VALUE, stp->signoff, NULL);

		xv_set(SFrame->ShutoffControl, PANEL_VALUE, stp->shutoff, NULL);
		switch (stp->format) {
		case RADIO_TYPE_ULAW:
		default:
			xv_set(SFrame->FormatControl, PANEL_VALUE, 0, NULL);
			break;
		case RADIO_TYPE_G721:
			xv_set(SFrame->FormatControl, PANEL_VALUE, 1, NULL);
			break;
		}

		xv_set(SFrame->AddressItem, PANEL_VALUE, stp->address, NULL);
		xv_set(SFrame->HopcountItem, PANEL_VALUE, stp->hops, NULL);
		break;

	case PANEL_LIST_OP_VALIDATE:
		break;

	case PANEL_LIST_OP_DELETE:
		list_delete(row);
		break;
	}
	return XV_OK;
}

/*
 * Notify callback function for `AddButton'.
 */
void
add_notify(
	Panel_item		item,
	Event*			event)
{
	char*			name;

	/* Get the station name and validate it */
	name = copy_string((char*) xv_get(SFrame->StationItem, PANEL_VALUE));
	if (name[0] == '\0') {
		SError(T("Station name must be specified"));
		xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, 0);
	} else {
		/* Append it to the station list and rewrite rc file */
		list_add(item, list_getconfig(), name, -1);
		rc_write();
	}
	(void) free(name);
}

/*
 * Notify callback function for `DeleteButton'.
 */
void
delete_notify(
	Panel_item		item,
	Event*			event)
{
	int			i;

	/* Find the selected item */
	i = list_selected();
	if (i < 0) {
		SError(T("Nothing to delete"));
		xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, 0);
		return;
	}
	list_delete(i);
	rc_write();		/* Rewrite rc file */
}

/*
 * Notify callback function for `ChangeButton'.
 */
void
change_notify(
	Panel_item		item,
	Event*			event)
{
	char*			name;
	int			i;

	/* Find the selected item */
	i = list_selected();
	if (i < 0) {
		/* If no selection, treat this is an add event */
		add_notify(item, event);
		return;
	}

	/* Get the station name and validate it */
	name = copy_string((char*) xv_get(SFrame->StationItem, PANEL_VALUE));
	if (name[0] == '\0') {
		SError(T("Station name must be specified"));
		xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, 0);
	} else {
		/* Change is a delete, followed by an add */
		list_delete(i);
		list_add(item, list_getconfig(), name, i);
		rc_write();		/* Rewrite rc file */
	}
	(void) free(name);
}

/* On station panel resize events, re-layout the panel */
void
station_resize(
	Xv_window		win,
	Event*			event,
	Notify_arg		arg,
	Notify_event_type	type)
{
	xv_set(SFrame->ChangeButtonGroup, GROUP_LAYOUT, TRUE, NULL);
	xv_set(SFrame->PlusMinusGroup, GROUP_LAYOUT, TRUE, NULL);
	panel_paint(SFrame->StationControlPanel, PANEL_CLEAR);
}

/* Event callback function for `StationFrame' */
Notify_value
xmit_StationFrame_event_callback(
	Xv_window		win,
	Event*			event,
	Notify_arg		arg,
	Notify_event_type	type)
{
	if (event_action(event) == WIN_RESIZE) {
		station_resize(win, event, arg, type);
	}
	return (notify_next_event_func(win, (Notify_event)event, arg, type));
}


/* Toggle the station config panel between default and advanced mode */
void
toggle_plusminus()
{
	Rect			cr;

	frame_get_rect(SFrame->StationFrame, &cr);
	if (Advanced) {
		xv_set(SFrame->AdvancedGroup, XV_SHOW, TRUE, NULL);
		xv_set(SFrame->PlusMinusButton, PANEL_LABEL_STRING, T("-"),
		    NULL);
		cr.r_height = Height_adv;
	} else {
		xv_set(SFrame->AdvancedGroup, XV_SHOW, FALSE, NULL);
		xv_set(SFrame->PlusMinusButton, PANEL_LABEL_STRING, T("+"),
		    NULL);
		cr.r_height = Height_def;
	}

	/* Change height; a resize event will trigger station_resize() */
	frame_set_rect(SFrame->StationFrame, &cr);
}

/*
 * Notify callback function for `PlusMinusButton'.
 */
void
plusminus_notify(
	Panel_item		item,
	Event*			event)
{
	/* Toggle between default and advanced station config panel */
	Advanced = !Advanced;
	toggle_plusminus();

	/* Don't take down the panel if unpinned */
	xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, 0);
}

/* Set the icon according to current state */
void
icon_switch()
{
	Xv_opaque		old;
	Xv_opaque		new;
	Xv_opaque		mask;

	/* Change the icon only if it is different */
	old = xv_get(CurrentIcon, ICON_IMAGE);
	new = POWER_IS_ON ? Icon_on : Icon_off;
	mask = POWER_IS_ON ? Icon_onmask : Icon_offmask;
	if (old != new) {
		/* XXX - note that simply changing FRAME_ICON does not work */
		xv_set(CurrentIcon,
		    ICON_IMAGE, new,
		    ICON_MASK_IMAGE, mask,
		    NULL);
	}
}


/* Notify routine for input from transmitter process */
Notify_value
childinput_handler(
	Notify_client		client,
	int			fd)
{
	char**			ptr;
	char**			sptr;
	char**			eptr;
	int			cnt;
	char			msg[512];

	/* If the frame was set busy, free it up now */
	xv_set(MFrame->XmitFrame, FRAME_BUSY, FALSE, NULL);

	while (input_ready(Xmit_fromfd)) {
		Xmit_established = TRUE;
		sptr = lex_multiline(Xmit_fromfd);
		if (sptr == NULL)
			break;			/* end-of-file */

		ptr = sptr;			/* duplicate ptr */

		/* Parse multiple lines of input */
		while (*ptr != NULL) {
			/* Find the end of this line */
			for (eptr = ptr; *eptr != NULL; eptr++) {
				if (Debug)
					FPRINTF(stderr, "%s ", *eptr);
			}
			if (Debug)
				FPRINTF(stderr, "\n");

			/*
			 * First field is program name: skip that.
			 * If status message, parse and dispatch it.
			 * Otherwise, display the message in the footer and
			 * power down.
			 */
			ptr++;
			if ((strcmp(*ptr, RADIOCMD_STATUS) != 0) ||
			    (parse_cmds((ptr + 1), statuslist) != NULL)) {
				/* Display message in footer */
				msg[0] = '\0';
				while (*ptr != NULL) {
					STRCAT(msg, *ptr++);
					STRCAT(msg, " ");
				}
				MError(msg);

				/* If error, the transmitter is powered down */
				/* if (XmitState != POWER_OFF) */
				poweroff();
			}
			ptr = ++eptr;
		}
		/* Free the whole input message */
		(void) free((char*)sptr);
	}
	return (NOTIFY_DONE);
}

/* Reset state when the transmitter process appears to have died */
void
clear_child()
{
	char			msg[512];

	xv_set(MFrame->XmitFrame, FRAME_BUSY, FALSE, NULL);
	if (Xmit_childpid != -1) {
		/* Clear out input buffer */
		(void) childinput_handler((Notify_client)MFrame, Xmit_fromfd);

		/* Print a relevant error message */
		if (Xmit_established) {
			MError(T("Transmitter process terminated"));
		} else {
			SPRINTF(msg,
			    T("Could not execute %s process"), Xmit_basename);
			MError(msg);
		}

		/* Close down connections */
		Xmit_childpid = -1;
		(void) notify_set_input_func((Notify_client)MFrame,
		    NOTIFY_FUNC_NULL, Xmit_fromfd);
		(void) close(Xmit_tofd);
		(void) close(Xmit_fromfd);
		Xmit_tofd = -1;
		Xmit_fromfd = -1;

		poweroff();
	}
}

/* Signal handler for SIGPIPE */
Notify_value
sigpipe_handler(
	Notify_client		client,
	int			sig,
	Notify_signal_mode	when)
{
	clear_child();
	return (NOTIFY_DONE);
}

/* Signal handler for dead child process */
Notify_value
sigchild_handler(
	Notify_client		client,
	int			pid,
	int*			status,
	struct rusage*		rusage)
{
	if (WIFEXITED(*status) || WIFSIGNALED(*status)) {
		clear_child();
		return (NOTIFY_DONE);
	}
	return (NOTIFY_IGNORED);
}


/* Signal handler for audio device state changes */
Notify_value
sigio_handler(
	Notify_client		client,
	int			sig,
	Notify_signal_mode	when)
{
	audio_syncvolume();
	return (NOTIFY_DONE);
}

/* Signal handler for interrupt */
Notify_value
sigint_handler(
	Notify_client		client,
	int			sig,
	Notify_signal_mode	when)
{
	exit_cleanup();
	exit(0);
}

/* Set up XView callbacks for events concerning the transmitter process */
void
register_reaper()
{
	(void) notify_set_signal_func((Notify_client)MFrame,
	    sigpipe_handler, SIGPIPE, NOTIFY_SYNC); /* used to be NOTIFY_SAFE */
	(void) notify_set_wait3_func((Notify_client)MFrame,
	    sigchild_handler, Xmit_childpid);
	(void) notify_set_input_func((Notify_client)MFrame,
	    childinput_handler, Xmit_fromfd);
}

Xv_opaque
XmitPanel_Init()
{
	Rect		cr;
	int		ha;
	int		hd;

	/* Initialize panel and property sheet */
	MFrame = xmit_XmitFrame_objects_initialize(NULL, NULL);
	SFrame = xmit_StationFrame_objects_initialize(NULL, MFrame->XmitFrame);
	if ((MFrame == NULL) || (SFrame == NULL)) {
		FPRINTF(stderr, "%s: %s\n", prog, T("Cannot initialize XView"));
		exit(1);
	}

	/* Calculate the sizes for default and advanced station panels */
	frame_get_rect(SFrame->StationFrame, &cr);
	ha = (int)xv_get(SFrame->StationControlPanel, XV_HEIGHT);
	hd = ha - (int)xv_get(SFrame->AdvancedGroup, XV_HEIGHT);
	Height_adv = cr.r_height;
	Height_def = hd + (Height_adv - ha);
	toggle_plusminus();

	/* Make the slider control return continuous status */
	xv_set(MFrame->VolumeControl, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
	(void) notify_set_signal_func((Notify_client)MFrame,
	    sigio_handler, SIGIO, NOTIFY_SYNC); /* used to be NOTIFY_SAFE */

	(void) notify_set_signal_func((Notify_client)MFrame,
	    sigint_handler, SIGINT, NOTIFY_SYNC); /* used to be NOTIFY_SAFE */

	/* XXX - work around guide bug */
	xv_set(SFrame->StationItem,
	    XV_USE_DB, PANEL_VALUE_STORED_LENGTH, 4, NULL,
	    NULL);

	StationMenu = (Menu) xv_get(MFrame->StationButton, PANEL_ITEM_MENU);
	CurrentIcon = (Icon) xv_get(MFrame->XmitFrame, FRAME_ICON);
	xv_set(CurrentIcon, ICON_TRANSPARENT, TRUE, NULL);
	Icon_off = xv_get(CurrentIcon, ICON_IMAGE);
	Icon_offmask = xv_get(CurrentIcon, ICON_MASK_IMAGE);

	Icon_on = xv_create(XV_NULL, SERVER_IMAGE,
	    SERVER_IMAGE_DEPTH, 1,
	    SERVER_IMAGE_BITS, image_bits,
	    XV_WIDTH, 64,
	    XV_HEIGHT, 64,
	    NULL);
	Icon_onmask = xv_create(XV_NULL, SERVER_IMAGE,
	    SERVER_IMAGE_DEPTH, 1,
	    SERVER_IMAGE_BITS, mask_bits,
	    XV_WIDTH, 64,
	    XV_HEIGHT, 64,
	    NULL);

	/* Set up defaults */
	xv_set(SFrame->InputItem, PANEL_VALUE, AUDIO_DEVICE, NULL);
	xv_set(SFrame->ShutoffControl, PANEL_VALUE, TRUE, NULL);
	xv_set(SFrame->FormatControl, PANEL_VALUE, 0, NULL);
	xv_set(SFrame->AddressItem, PANEL_VALUE, Host, NULL);
	xv_set(SFrame->HopcountItem, PANEL_VALUE, Hopcount, NULL);
	return (MFrame->XmitFrame);
}


/* Routines to support reading and writing the .radiorc file */

/* Parser routine declarations */
int	rc_station(char*);
int	rc_agc(char*);
int	rc_service(char*);
int	rc_host(char*);
int	rc_bufsiz(char*);
int	rc_config(char*);
int	rc_preset(char*);
int	rc_signon(char*);
int	rc_input(char*);
int	rc_signoff(char*);
int	rc_shutoff(char*);
int	rc_format(char*);
int	rc_address(char*);
int	rc_hops(char*);

/*
 * Parsing table for .radiorc handling
 * The defined values are table indeces, so they must be kept up to date.
 */
Radio_cmdtbl	Rc_tbl[] = {
						/* General tool information */
	{"Radio_Service",		rc_service},
#define	RC_SERVICE	0
	{"Radio_Multicast_Host",	rc_host},
#define	RC_HOST		1
	{"Xmit_Station",		rc_station},
#define	RC_STATION	2
	{"Xmit_Auto_Volume_Adjust",	rc_agc},
#define	RC_AGC		3
	{"Xmit_Input_Buffer",		rc_bufsiz},
#define	RC_BUFSIZ	4
	{"Xmit_Config",			rc_config},
#define	RC_CONFIG	5
						/* Station preset info */
	{"Xmit_Preset_Station",		rc_preset},
#define	RC_PRESET	6
	{"Xmit_Sign_On_File",		rc_signon},
#define	RC_SIGNON	7
	{"Xmit_Audio_Input",		rc_input},
#define	RC_INPUT	8
	{"Xmit_Sign_Off_File",		rc_signoff},
#define	RC_SIGNOFF	9
	{"Xmit_Auto_Shutoff",		rc_shutoff},
#define	RC_SHUTOFF	10
	{"Xmit_Audio_Format",		rc_format},
#define	RC_FORMAT	11
	{"Xmit_Multicast_Addr",		rc_address},
#define	RC_ADDRESS	12
	{"Xmit_Multicast_Hops",		rc_hops},
#define	RC_HOPS		13
	{NULL,				(int(*)(char*)) NULL},
};

/*
 * Parsing routines for each rc file option.
 * If Reading is TRUE, 'arg' contains the value from the rc file.
 * If Reading is FALSE, write the corresponding entry out.
 * The radiorc_putstr() and radiorc_putval() routines guard against duplication.
 */

int
rc_service(char* arg)
{
	if (Reading) {
		replace_string(&Service, arg);
	} else {
		radiorc_putstr(Rc_outfd, &Rc_tbl[RC_SERVICE], Service);
	}
	return (TRUE);
}

int
rc_host(char* arg)
{
	if (Reading) {
		replace_string(&Host, arg);
	} else {
		radiorc_putstr(Rc_outfd, &Rc_tbl[RC_HOST], Host);
	}
	return (TRUE);
}

int
rc_station(char* arg)
{
	if (Reading) {
		STRNCPY(Callname, arg, RADIO_CALLNAME_SIZE);
		Callname[RADIO_CALLNAME_SIZE] = '\0';
	} else {
		radiorc_putstr(Rc_outfd, &Rc_tbl[RC_STATION], Callname);
	}
	return (TRUE);
}

int
rc_agc(char* arg)
{
	if (Reading) {
		Agc = is_on(arg);
	} else {
		radiorc_putstr(Rc_outfd, &Rc_tbl[RC_AGC], Agc ? "On" : "Off");
	}
	return (TRUE);
}

int
rc_bufsiz(char* arg)
{
	if (Reading) {
		Bufsiz = atoi(arg);
	} else {
		/* Only rewrite if it was already there (XXX - for now) */
		if ((*arg != '\0') && (Bufsiz != 0))
			radiorc_putval(Rc_outfd, &Rc_tbl[RC_BUFSIZ], Bufsiz);
	}
	return (TRUE);
}

int
rc_config(char* arg)
{
	if (Reading) {
		Advanced = (strcmp(arg, "Advanced") == 0);
	} else {
		radiorc_putstr(Rc_outfd, &Rc_tbl[RC_CONFIG],
		    Advanced ? "Advanced" : "Default");
	}
	return (TRUE);
}

/* Clear the 'seen' flags for preset parameters */
void
rc_resetpreset()
{
	Rc_tbl[RC_PRESET].seen = FALSE;
	Rc_tbl[RC_SIGNON].seen = FALSE;
	Rc_tbl[RC_INPUT].seen = FALSE;
	Rc_tbl[RC_SIGNOFF].seen = FALSE;
	Rc_tbl[RC_SHUTOFF].seen = FALSE;
	Rc_tbl[RC_FORMAT].seen = FALSE;
	Rc_tbl[RC_ADDRESS].seen = FALSE;
	Rc_tbl[RC_HOPS].seen = FALSE;
}

/* Write out all previously unwritten parameters for the current preset */
void
rc_writepreset(
	Station_t*	stp)
{
	(void) (*Rc_tbl[RC_SIGNON].func)("");
	(void) (*Rc_tbl[RC_INPUT].func)("");
	(void) (*Rc_tbl[RC_SIGNOFF].func)("");
	(void) (*Rc_tbl[RC_SHUTOFF].func)("");
	(void) (*Rc_tbl[RC_FORMAT].func)("");
	(void) (*Rc_tbl[RC_ADDRESS].func)("");
	(void) (*Rc_tbl[RC_HOPS].func)("");
	radiorc_putblank(Rc_outfd);
}

/* When a new station preset entry is encountered, reset station ptrs */
int
rc_preset(char* arg)
{
	Stationlist_t*		stlp;
	Station_t*		stp;

	if (Reading) {
		/* Ignore this entry if the station name is null */
		if (arg[0] == '\0') {
			CurrentPreset = NULL;
			return (TRUE);
		}

		/* Start entering data into a new station structure */
		stlp = (Stationlist_t*) calloc(1, sizeof (*stlp));
		stlp->station = (Station_t*) calloc(1, sizeof (*stlp->station));
		stlp->next = PresetList;
		PresetList = stlp;
		CurrentPreset = stlp;
		stlp->station->input = copy_string(AUDIO_DEVICE);
		stlp->station->shutoff = TRUE;
		stlp->station->format = RADIO_TYPE_ULAW;
		stlp->station->hops = Hopcount;
		stlp->station->address = copy_string(Host);
		STRNCPY(stlp->name, arg, RADIO_CALLNAME_SIZE);
		stlp->name[RADIO_CALLNAME_SIZE] = '\0';

		/* If no default station name, use the first preset */
		if (Callname[0] == '\0')
			STRCPY(Callname, stlp->name);
	} else {
		/* Make sure the previous preset was entirely written out */
		if (CurrentPreset != NULL)
			rc_writepreset(CurrentPreset->station);
		CurrentPreset = NULL;

		/* Find this station in the list */
		for (stlp = PresetList; stlp != NULL; stlp = stlp->next) {
			if (strcmp(arg, stlp->name) == 0) {
				/* Make sure this preset is written only once */
				if (stlp->seen++ == 0) {
					/* Clear the 'seen' flags */
					rc_resetpreset();

					/* Start new preset output */
					radiorc_putblank(Rc_outfd);
					radiorc_putstr(Rc_outfd,
					    &Rc_tbl[RC_PRESET], stlp->name);
					CurrentPreset = stlp;
				}
				break;
			}
		}
	}
	return (TRUE);
}

int
rc_signon(char* arg)
{
	if (CurrentPreset == NULL)	/* Skip if not a valid preset */
		return (TRUE);

	if (Reading) {
		replace_string(&CurrentPreset->station->signon, arg);
	} else {
		radiorc_putstr(Rc_outfd, &Rc_tbl[RC_SIGNON],
		    CurrentPreset->station->signon);
	}
	return (TRUE);
}

int
rc_input(char* arg)
{
	if (CurrentPreset == NULL)	/* Skip if not a valid preset */
		return (TRUE);

	if (Reading) {
		replace_string(&CurrentPreset->station->input, arg);
	} else {
		radiorc_putstr(Rc_outfd, &Rc_tbl[RC_INPUT],
		    CurrentPreset->station->input);
	}
	return (TRUE);
}

int
rc_signoff(char* arg)
{
	if (CurrentPreset == NULL)	/* Skip if not a valid preset */
		return (TRUE);

	if (Reading) {
		replace_string(&CurrentPreset->station->signoff, arg);
	} else {
		radiorc_putstr(Rc_outfd, &Rc_tbl[RC_SIGNOFF],
		    CurrentPreset->station->signoff);
	}
	return (TRUE);
}

int
rc_shutoff(char* arg)
{
	if (CurrentPreset == NULL)	/* Skip if not a valid preset */
		return (TRUE);

	if (Reading) {
		CurrentPreset->station->shutoff = is_on(arg);
	} else {
		radiorc_putstr(Rc_outfd, &Rc_tbl[RC_SHUTOFF],
		    CurrentPreset->station->shutoff ? "On" : "Off");
	}
	return (TRUE);
}

int
rc_format(char* arg)
{
	if (CurrentPreset == NULL)	/* Skip if not a valid preset */
		return (TRUE);

	if (Reading) {
		if (strcmp(arg, "Compressed") == 0)
			CurrentPreset->station->format = RADIO_TYPE_G721;
		else
			CurrentPreset->station->format = RADIO_TYPE_ULAW;
	} else {
		switch (CurrentPreset->station->format) {
		default:
		case RADIO_TYPE_ULAW: arg = "Uncompressed"; break;
		case RADIO_TYPE_G721: arg = "Compressed"; break;
		}
		radiorc_putstr(Rc_outfd, &Rc_tbl[RC_FORMAT], arg);
	}
	return (TRUE);
}

int
rc_address(char* arg)
{
	if (CurrentPreset == NULL)	/* Skip if not a valid preset */
		return (TRUE);

	if (Reading) {
		if (arg[0] != '\0')
			replace_string(&CurrentPreset->station->address, arg);
	} else {
		radiorc_putstr(Rc_outfd, &Rc_tbl[RC_ADDRESS],
		    CurrentPreset->station->address);
	}
	return (TRUE);
}

int
rc_hops(char* arg)
{
	if (CurrentPreset == NULL)	/* Skip if not a valid preset */
		return (TRUE);

	if (Reading) {
		CurrentPreset->station->hops = atoi(arg);
	} else {
		radiorc_putval(Rc_outfd, &Rc_tbl[RC_HOPS],
		    CurrentPreset->station->hops);
	}
	return (TRUE);
}


/* Read the .radiorc file to load state; can be called without XView panels */
void
rc_load()
{
	int			old;
	char			msg[512];
	char**			sptr;
	char**			ptr;

	/* Get a handle to the .radiorc file, if any */
	old = radiorc_open(O_RDONLY);
	if (old < 0)
		goto error;

	/* Initialize the preset list */
	PresetList = NULL;
	CurrentPreset = NULL;
	Reading = TRUE;

	/* Read in and parse the file */
	while ((sptr = lex_multiline(old)) != NULL) {

		/* For each input line, parse a command */
		for (ptr = sptr; *ptr != NULL; ) {
			(void) parse_cmds(ptr, Rc_tbl);

			/* Skip to next command */
			while (*ptr++ != NULL);
		}

		/* Free the whole input message */
		(void) free((char*)sptr);
	}

error:
	radiorc_close(old);
	return;
}

/* Walk the station list, creating stations */
void
rc_makelist()
{
	Stationlist_t*		stlp;
	Stationlist_t*		oldp;
	Station_t*		stp;

	/* XXX - should destroy old list first */

	stlp = PresetList;
	while (stlp != NULL) {
		/* Add this station, handing its station struct off */
		list_add(NULL, stlp->station, stlp->name, 0);

		oldp = stlp;
		stlp = oldp->next;
		(void) free((char*)oldp);
	}
	PresetList = NULL;

	/* If a callname was set, propagate the station configuration */
	if (Callname[0] != '\0')
		list_resync();
}

/*
 * Read the .radiorc file to load state
 * XXX - this is never called for now (that's why the station list
 *       doesn't have to be destroyed in rc_makelist() yet.
 */
void
rc_read()
{
	/* This is going to take a while */
	xv_set(MFrame->XmitFrame, FRAME_BUSY, TRUE, NULL);

	/* Parse the rc file */
	rc_load();
	rc_makelist();

	xv_set(MFrame->XmitFrame, FRAME_BUSY, FALSE, NULL);
}

/* Update the .radiorc file with current values */
void
rc_write()
{
	int			i;
	int			nrows;
	int			old;
	char**			sptr;
	char**			ptr;
	char**			eptr;
	int			lastblank;
	Stationlist_t*		stlp;
	Stationlist_t*		oldp;
	char			msg[512];

	/* This is going to take a while */
	xv_set(MFrame->XmitFrame, FRAME_BUSY, TRUE, NULL);

	/* Get a handle to the old file, if any; then unlink and rewrite it */
	old = radiorc_open(O_RDONLY);
	Rc_outfd = radiorc_open(O_WRONLY | O_EXCL);
	if (Rc_outfd < 0)
		goto error;

	/* Mark all items that must be written out */
	Rc_tbl[RC_SERVICE].seen = FALSE;
	Rc_tbl[RC_HOST].seen = FALSE;
	Rc_tbl[RC_STATION].seen = FALSE;
	Rc_tbl[RC_AGC].seen = FALSE;
	Rc_tbl[RC_BUFSIZ].seen = FALSE;
	Rc_tbl[RC_CONFIG].seen = FALSE;
	rc_resetpreset();

	/* Initialize the preset list */
	PresetList = NULL;
	CurrentPreset = NULL;
	Reading = FALSE;

	/* Build the preset list (back to front) */
	nrows = (int) xv_get(SFrame->StationList, PANEL_LIST_NROWS);
	for (i = nrows; --i >= 0; ) {
		stlp = (Stationlist_t*) calloc(1, sizeof (*stlp));
		stlp->next = PresetList;
		PresetList = stlp;
		STRCPY(stlp->name, (char*) xv_get(SFrame->StationList,
		    PANEL_LIST_STRING, i));
		stlp->station = (Station_t*) xv_get(SFrame->StationList,
		    PANEL_LIST_CLIENT_DATA, i);
	}

	/* If there was an old file, rewrite it now */
	if (old >= 0) {
		lastblank = TRUE;
		while ((sptr = lex_line(old)) != NULL) {
			/*
			 * For each input line, parse a command.
			 * If unknown command, rewrite the line.
			 * Write blank lines and comments out, too.
			 */
			ptr = sptr;
			while (*ptr != NULL) {
				/* Delineate a single input line */
				for (eptr = ptr; *eptr != NULL; eptr++) {
					if (**eptr == '\n') {
						*eptr++ = NULL;
						break;
					}
				}

				if ((*ptr == NULL) || (**ptr == '#') ||
				    (parse_cmds(ptr, Rc_tbl) != NULL)) {
					radiorc_putcomment(Rc_outfd, ptr);
				}
				/* Point to next token */
				ptr = eptr;
			}

			/* Free the whole input message */
			(void) free((char*)sptr);
		}
		radiorc_close(old);

		/* If a preset was partially written, finish it now */
		if (Rc_tbl[RC_PRESET].seen) {
			if (CurrentPreset != NULL)
				rc_writepreset(CurrentPreset->station);
			CurrentPreset = NULL;
		}
	}

	/* Write out all parameters that were not handled above */
	(void) (*Rc_tbl[RC_SERVICE].func)("");
	(void) (*Rc_tbl[RC_HOST].func)("");
	radiorc_putblank(Rc_outfd);
	(void) (*Rc_tbl[RC_STATION].func)("");
	(void) (*Rc_tbl[RC_AGC].func)("");
	(void) (*Rc_tbl[RC_BUFSIZ].func)("");
	(void) (*Rc_tbl[RC_CONFIG].func)("");

	/* Write out all presets that were not handled above */
	for (stlp = PresetList; stlp != NULL; ) {
		/* Make sure this preset is written only once */
		if (stlp->seen++ == 0) {
			/* Clear the 'seen' flags */
			rc_resetpreset();

			/* Start new preset output */
			radiorc_putblank(Rc_outfd);
			radiorc_putstr(Rc_outfd,
			    &Rc_tbl[RC_PRESET], stlp->name);
			CurrentPreset = stlp;
			rc_writepreset(CurrentPreset->station);
		}
		/* Free this entry */
		oldp = stlp;
		stlp = stlp->next;
		(void) free((char*) oldp);
	}

	radiorc_close(Rc_outfd);
	Rc_outfd = -1;
	xv_set(MFrame->XmitFrame, FRAME_BUSY, FALSE, NULL);
	return;

error:
	radiorc_close(old);
	radiorc_close(Rc_outfd);
	Rc_outfd = -1;
	SPRINTF(msg, T("Could not write defaults to ~/%s"), RADIO_RCFILE);
	SError(msg);
	xv_set(MFrame->XmitFrame, FRAME_BUSY, FALSE, NULL);
}



/* xmit entry point */
main(
	int			argc,
	char*			argv[])
{
	char*			path;
	char*			s;
	int			i;
	Xv_opaque		panel;

	/* Get the program name */
	prog = strrchr(argv[0], '/');
	if (prog == NULL)
		prog = argv[0];
	else
		prog++;

	/*
	 * Construct a pathname to initiate the transmitter program.
	 * If there was an explicit path, use it as a path prefix.
	 * Otherwise, hopefully, it will be found in the current $PATH.
	 * Prepend the shell 'exec' command and append program args.
	 */
	path = copy_string(argv[0]);
	s = strrchr(path, '/');
	if (s == NULL) {
		path[0] = '\0';
	} else {
		s[1] = '\0';
	}
	Xmit_command = malloc(strlen(Xmit_prefix) +
	    strlen(path) + strlen(Xmit_basename) + strlen(Xmit_args) + 1);
	SPRINTF(Xmit_command, "%s%s%s%s",
	    Xmit_prefix, path, Xmit_basename, Xmit_args);
	(void) free(path);

	/* Initialize I18N and XView */
	INSTANCE = xv_unique_key();
	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
	    XV_USE_LOCALE, TRUE,
	    0);

	/* Set defaults from .radiorc file */
	replace_string(&Service, RADIO_DEFAULT_SERVICE);
	replace_string(&Host, RADIO_DEFAULT_ADDRESS);
	rc_load();

	/* Parse start-up options */
	while ((i = getopt(argc, argv, "s:h:cD")) != EOF) switch (i) {
	case 's':			/* specify port number/service name */
		replace_string(&Service, optarg);
		break;
	case 'h':			/* specify multicast host/address */
		replace_string(&Host, optarg);
		break;
	case 'c':			/* specify conferencing */
		Bufsiz = 256;
		break;
	case 'D':			/* debug flag */
		Debug = TRUE;
		break;
	default:
		usage();
	}

	/* Verify the service/port number */
	if (radio_set_service(Service) < 0) {
		if (isdigit(Service[0])) {
			FPRINTF(stderr,
			    T("%s:\tInvalid service port number\n"), prog);
		} else {
			FPRINTF(stderr,
			    T("%s:\tCannot locate '%s' in services map\n"),
			    prog, Service);
		}
		/*
		 * Try the default service name or, if that fails,
		 * the NIC assigned port number.
		 */
		replace_string(&Service, RADIO_DEFAULT_SERVICE);
		if ((radio_set_service(Service) < 0) &&
		    (replace_string(&Service, RADIO_NIC_PORT),
		    (radio_set_service(Service) < 0))) {
			FPRINTF(stderr,
    T("\tUse the -s option to specify a service name or port number\n"));
			exit(1);
		} else {
			FPRINTF(stderr,
			    T("\tUsing the '%s' service port\n"), Service);
		}
	}

	/* Verify the multicast host/address */
	if (net_parse_address(Host, NULL) < 0) {
		Multicast = FALSE;

		/* If multicast is supported in the OS, keep trying */
		if (net_multicast_supported()) {
			/* If there was a default override, print msg */
			if (strcmp(Host, RADIO_DEFAULT_ADDRESS) != 0) {
				FPRINTF(stderr,
	    T("%s:\tCannot locate multicast host '%s'\n"), prog, Host);
			}

			/* Try the default address */
			if (net_parse_address(RADIO_DEFAULT_ADDRESS, NULL)
			    >= 0) {
				replace_string(&Host, RADIO_DEFAULT_ADDRESS);
				FPRINTF(stderr,
				    T("\tUsing multicast host '%s'\n"), Host);
				Multicast = TRUE;
			} else if (net_parse_address(RADIO_NIC_ADDRESS, NULL)
			    >= 0) {
				replace_string(&Host, RADIO_NIC_ADDRESS);
				FPRINTF(stderr,
	    T("\tUsing default multicast address '%s'\n"), Host);
				Multicast = TRUE;
			}
		} else {
			/* If no multicast OS support, just use UDP */
			replace_string(&Host, NET_BROADCAST_NAME);
			Hopcount = 1;
			FPRINTF(stderr,
	    T("%s:\tDefaulting to UDP broadcast (local subnet only)\n"), prog);
		}
	}

	panel = XmitPanel_Init();
	state_set(POWER_OFF);

	/* Set up the station list, deselect the last entry */
	rc_makelist();
	SError("");
	if ((i = list_selected()) >= 0)
		xv_set(SFrame->StationList, PANEL_LIST_SELECT, i, FALSE, NULL);

	if (Callname[0] == '\0')
		MError(T("Press the Station button to enter a station name"));

	xv_main_loop(panel);

	exit_cleanup();
	exit(0);
}
