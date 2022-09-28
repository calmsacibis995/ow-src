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
#ident	"@(#)radio.c	1.21	96/02/29 SMI"


/*
 * Radio Free Ethernet receiver tool.
 *
 * This program builds a GUI for the RFE receiver.
 * It execs radio_recv to perform the audio/network operations,
 * communicating commands and status through pipes.
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
#include <sys/wait.h>

#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <group.h>
#include "radio_ui.h"

#include "radio.h"
#include "radio_network.h"
#include "netbroadcast.h"


/* A linked list of active station information */
typedef struct stlist {
	struct stlist*	next;				/* next in list */
	char		name[RADIO_CALLNAME_SIZE + 1];	/* callname */
	char*		dj;				/* dj string */
	int		active;				/* TRUE if active */
	int		seen;				/* TRUE if written */
} Stationlist_t;


/* Receiver state variables */
int			State = POWER_OFF;		/* current state */
int			RecvState = -1;			/* reported state */
char			Callname[RADIO_CALLNAME_SIZE + 1];
Stationlist_t*		ActiveList = NULL;		/* active stations */
char*			DiscJockey = NULL;		/* dj name */
char*			Service = NULL;			/* port number */
char*			Host = NULL;			/* multicast host */
int			Poweron_init = FALSE;		/* startup power-on? */
int			Autoscan = TRUE;		/* scan on signoff? */
int			Scandelay = 60;			/* delay if squelched */
int			Autorelease = TRUE;		/* release outdev */
char*			Output = NULL;			/* audio output file */
int			Debug = FALSE;			/* debug flag */
char*			prog;				/* program name */

#define	POWER_IS_ON	(!(RecvState == POWER_OFF))
#define	POWER_IS_OFF	(RecvState == POWER_OFF)

/* State variables for .radiorc parsing */
Stationlist_t*		PresetList;			/* preset list head */
int			Reading;			/* FALSE if writing */
int			Rc_outfd = -1;			/* output file desc */

/* Connections to receiver process */
char*			Shell_prefix = "exec ";		/* sh prefix */
char*			Recv_basename = "radio_recv";	/* program name */
char*			Recv_args = " -C Report";	/* invocation args */
char*			Recv_cmd1 = NULL;		/* invocation cmd */
char*			Recv_cmd2;			/* backup cmd */
int			Recv_childpid = -1;		/* pid of recv proc */
int			Recv_tofd;			/* to write to proc */
int			Recv_fromfd;			/* to read from proc */
int			Recv_established;		/* set on contact */

/* Connections to volume control process */
char*			Volume_basename = "gaintool";	/* program name */
char*			Volume_args = "";		/* invocation args */
char*			Volume_cmd1 = NULL;		/* invocation cmd */
char*			Volume_cmd2;			/* backup cmd */
int			Volume_childpid = -1;		/* pid of gaintool */

#define	ERR_NOEXEC		(10)

/* XView-related variables */
#define	PRESETS			(6)			/* preset buttons */

Attr_attribute			INSTANCE;		/* Devguide key */
radio_RadioFrame_objects*	MFrame = NULL;
radio_PropsFrame_objects*	PFrame = NULL;
Menu				StationMenu;
Menu_item			NoActiveStations;
Icon				CurrentIcon;
Xv_opaque			PresetButtons[PRESETS];
Xv_opaque			Icon_on;
Xv_opaque			Icon_onmask;
Xv_opaque			Icon_off;
Xv_opaque			Icon_offmask;

static unsigned short		image_bits[] = {
#include "radio_on.icon"
};
static unsigned short		mask_bits[] = {
#include "radio_on.mask.icon"
};

/* Macro to translate strings */
#define	T(str)		(dgettext("radio-labels", str))

/* Define error reporting routines for the frame and pin-up */
#define	MError(str)	error(MFrame->RadioFrame, str);
#define	PError(str)	error(PFrame->PropsFrame, str);

/* XView routines declared below */
void			error(Xv_opaque, char*);
void			state_set(int);
void			icon_switch();
void			station_notify(Menu, Menu_item);
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
	    T("Radio Free Ethernet Receiver -- usage:"), prog);
	FPRINTF(stderr, T("[-s service] [-h host]\nwhere:\n"));
	FPRINTF(stderr, T("\t-s\tSpecify service name or port number\n"));
	FPRINTF(stderr, T("\t\t\tDefault service: '%s'\n"),
	    RADIO_DEFAULT_SERVICE);
	FPRINTF(stderr, T("\t-h\tSpecify multicast hostname or ip address\n"));
	FPRINTF(stderr, T("\t\t\tDefault hostname: '%s'\n"),
	    RADIO_DEFAULT_ADDRESS);
	exit(1);
}


/* Fork and exec the radio receiver process */
int
fork_recv()
{
	int			i;
	int			pipeto[2];
	int			pipefrom[2];
	char			msg[512];
	sigset_t		set;

	/* Do nothing if there's already a receiver running */
	if (Recv_childpid != -1)
		return (TRUE);
	Recv_established = FALSE;

	/* Open pipes for bidirectional transfer */
	if ((pipe(pipeto) < 0) || (pipe(pipefrom) < 0)) {
failure:
		SPRINTF(msg, T("Could not start the %s process"),
		    Recv_basename);
		MError(msg);
		return (FALSE);
	}

	switch (Recv_childpid = fork()) {
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

		/* Exec the receiver process */
		if (Recv_cmd1 != NULL)
			execl(Recv_cmd1, Recv_basename, "-C", "Report", NULL);
		execl("/bin/sh", "sh", "-c", Recv_cmd2, NULL);
		exit(ERR_NOEXEC);

	default:				/* parent */
		/* Close unused descriptors */
		Recv_tofd = pipeto[1];
		(void) close(pipeto[0]);
		Recv_fromfd = pipefrom[0];
		(void) close(pipefrom[1]);

		/* Register input and wait procs */
		register_reaper();
		break;

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
 * Send a command line to the receiver.
 * The string should be terminated with a newline.
 */
int
send_recvmsg(
	char*			str)
{
	int			i;

	i = strlen(str);
	if ((Recv_tofd < 0) ||
	    (write(Recv_tofd, str, i) != i)) {
		return (FALSE);
	}
	if (Debug)
		FPRINTF(stderr, "%s: %s", prog, str);
	return (TRUE);
}

/*
 * Shut down reception.
 * This is being called in response to a receiver reporting an error or
 * power-off, or dying.
 */
void
poweroff()
{
	/* Receiver is off or dead */
	State = POWER_OFF;
	state_set(POWER_OFF);
}

/* Start scanning for a new station */
int
start_scan()
{
	char			msg[512];

	Callname[0] = '\0';
	xv_set(MFrame->StationButtonText, PANEL_VALUE, Callname, NULL);
	xv_set(MFrame->DJText, XV_SHOW, FALSE, NULL);
	if (POWER_IS_ON) {
		SPRINTF(msg, "%s\n", RADIOCMD_SCAN);
		if (!send_recvmsg(msg)) {
			poweroff();
			return (FALSE);
		}
	}
	return (TRUE);
}

/* Start transmission */
int
poweron()
{
	char			msg[512];

	/* Make sure the receiver process is running */
	if (!fork_recv()) {
		poweroff();
		return (FALSE);
	}

	/* If there is no station set, start scanning */
	if (*Callname == '\0') {
		State = SCANNING;
		if (!start_scan())
			return (FALSE);
	} else {
		State = QUIET;
	}

	/*
	 * Compose a message for the receiver.
	 */
	SPRINTF(msg, "%s %s=%s %s=%s %s=%s %s=%s %s=%s %s %s\n",
	    RADIOCMD_STOP,
	    RADIOCMD_STATION, Callname,
	    RADIOCMD_SERVICE, Service,
	    RADIOCMD_ADDRESS, Host,
	    RADIOCMD_OUTPUT, Output,
	    RADIOCMD_RELEASE, Autorelease ? "On" : "Off",
	    State == SCANNING ? RADIOCMD_SCAN : "",
	    RADIOCMD_START);
	if (!send_recvmsg(msg)) {
		poweroff();
		return (FALSE);
	}

	/* Toggle the power button on */
	xv_set(MFrame->PowerButton, PANEL_VALUE, 1, NULL);

	/* Busy the tool until status is sent */
	xv_set(MFrame->RadioFrame, FRAME_BUSY, TRUE, NULL);
	return (TRUE);
}

/* Tell the receiver to stop */
void
signoff()
{
	char			msg[512];

	/* Send a power off message to the receiver */
	SPRINTF(msg, "%s\n", RADIOCMD_STOP);
	if (!send_recvmsg(msg)) {
		/* If error sending message, kill the receiver */
		if (Recv_childpid != -1)
			(void) kill(Recv_childpid, SIGINT);
		else
			poweroff();
	}
	State = POWER_OFF;
}

/* Timer handler, called when the autoscan delay has been reached */
/* ARGSUSED */
void
timer_handler(
	Notify_client		client,
	int			which)
{
	if (POWER_IS_ON)
		start_scan();
}

/* Handle a change of the autoscan counter status */
void
autoscan_reset()
{
	struct itimerval	t;

	/* Cancel the old timer, if any */
	(void) notify_set_itimer_func(QUIET, NOTIFY_FUNC_NULL,
	    ITIMER_REAL, NULL, NULL);

	if (Autoscan && (RecvState == QUIET)) {
		if (Scandelay <= 0) {
			/* Simulate a timeout */
			timer_handler(QUIET, ITIMER_REAL);
		} else {
			/* Restart the timer */
			t.it_value.tv_sec = Scandelay;
			t.it_value.tv_usec = 0;
			t.it_interval.tv_sec = 0;
			t.it_interval.tv_usec = 0;
			(void) notify_set_itimer_func(QUIET,
			    (Notify_func)timer_handler, ITIMER_REAL, &t, NULL);
		}
	}
}

/* Handle power off status */
/* ARGSUSED */
int
status_off(
	char*			arg)
{
	state_set(POWER_OFF);
	return (TRUE);
}

/* Handle receiver status */
/* ARGSUSED */
int
status_recv(
	char*			arg)
{
	state_set(ON_THE_AIR);
	return (TRUE);
}

/* Handle squelched data */
/* ARGSUSED */
int
status_squelch(
	char*			arg)
{
	state_set(QUIET);
	return (TRUE);
}

/* Handle audio device released */
/* ARGSUSED */
int
status_release(
	char*			arg)
{
	state_set(BACKOFF);
	return (TRUE);
}

/* Handle start of station scan */
/* ARGSUSED */
int
status_scan(
	char*			arg)
{
	state_set(SCANNING);
	return (TRUE);
}

/* Handle station sign-off */
/* ARGSUSED */
int
status_signoff(
	char*			arg)
{
	state_set(OFF_THE_AIR);

	/* If auto-scan is set, scan for another station */
	if (Autoscan) {
		start_scan();
	}
	return (TRUE);
}

/* Receiver status parse table */
Radio_cmdtbl	statuslist[] = {
	{RADIOCMD_POWEROFF,	status_off},
	{RADIOCMD_BROADCAST,	status_recv},
	{RADIOCMD_SQUELCH,	status_squelch},
	{RADIOCMD_RELEASE,	status_release},
	{RADIOCMD_SCANNING,	status_scan},
	{RADIOCMD_SIGNOFF,	status_signoff},
	{NULL,			(int(*)()) NULL},
};


/* Cleanup on exit; kill the receiver program */
void
exit_cleanup()
{
	if (Recv_childpid != -1) {
		(void) kill(Recv_childpid, SIGINT);
	}
	(void) fflush(stderr);
}


/* XView interface section: derived from radio_stubs.c */

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
	xv_set(MFrame->StationButton, PANEL_INACTIVE, POWER_IS_OFF, NULL);
	if (POWER_IS_OFF) {
		xv_set(MFrame->PowerButton, PANEL_VALUE, 0, NULL);
		xv_set(MFrame->DJText, XV_SHOW, FALSE, NULL);
	}
}

/* Set the status message according to current state */
void
state_set(
	int			newstate)
{
	char*			str;

	if ((newstate == RecvState) && POWER_IS_ON)
		return;			/* no change in state */
	RecvState = newstate;

	switch (RecvState) {
	case POWER_OFF:		str = "Power Off"; break;
	case SCANNING:		str = "Scanning"; break;
	case QUIET:		str = "No Broadcast"; break;
	case ON_THE_AIR:	str = "On-The-Air"; break;
	case OFF_THE_AIR:	str = "Off-The-Air"; break;
	case WAITING:		str = "Audio Busy"; break;
	case BACKOFF:		str = "Audio Busy"; break;
	}
	if (Debug)
		FPRINTF(stderr, "%s: State message set to: %s\n", prog, str);

	xv_set(MFrame->StatusMessage, PANEL_LABEL_STRING, T(str), NULL);
	xv_set(MFrame->StatusGroup, GROUP_LAYOUT, TRUE, NULL);

	/* Reset or cancel the autoscan timer */
	autoscan_reset();

	/* Make sure various things are set properly */
	toggle_power();
	icon_switch();

	if (RecvState == SCANNING) {
		Callname[0] = '\0';
		xv_set(MFrame->StationButtonText, PANEL_VALUE, Callname, NULL);
		xv_set(MFrame->DJText, XV_SHOW, FALSE, NULL);
	}
}

/* Active Station menu maintenance routines */

/* Release allocated strings for a given station structure */
void
active_clear(
	Stationlist_t*		stlp)
{
	if (stlp->dj != NULL) {
		(void) free(stlp->dj);
		stlp->dj = NULL;
	}
}

/* Returns the matching station item (or NULL, if none) */
Stationlist_t*
active_find(
	char*			name)
{
	Stationlist_t*		stlp;

	for (stlp = ActiveList; stlp != NULL; stlp = stlp->next) {
		if (strcmp(name, stlp->name) == 0)
			return (stlp);
	}
	return (NULL);
}

/* Look for a particular station name in the active menu */
Menu_item
active_findmenu(
	char*			name)
{
	int			i;
	Menu_item		mi;
	Stationlist_t*		stlp;

	/* Find the matching active station menu item and return it */
	for (i = (int) xv_get(StationMenu, MENU_NITEMS); i > 1; i--) {
		mi = xv_get(StationMenu, MENU_NTH_ITEM, i);
		stlp = (Stationlist_t*) xv_get(mi, MENU_VALUE);
		if (strcmp(name, stlp->name) == 0)
			return (mi);
	}
	return (NULL);
}

/* Add an entry to the active station list entry */
void
active_add(
	Stationlist_t*		stlp)
{
	char*			itemname;

	/* If this is the first menu entry, remove the placeholder */
	if (ActiveList == NULL)
		xv_set(StationMenu, MENU_REMOVE_ITEM, NoActiveStations, NULL);

	/* Link in this new entry */
	stlp->next = ActiveList;
	ActiveList = stlp;

	/*
	 * Add an entry into the station menu corresponding to this station.
	 * Store the station data pointer.
	 */
	itemname = malloc(RADIO_CALLNAME_SIZE + 3);
	SPRINTF(itemname, "%s%s%s",
	    stlp->active ? " " : "[", stlp->name, stlp->active ? " " : "]");
	xv_set(StationMenu,
	    MENU_APPEND_ITEM, (Menu_item)xv_create(NULL, MENUITEM,
	    MENU_STRING, itemname,
	    MENU_VALUE, stlp,
	    MENU_NOTIFY_PROC, station_notify,
	    MENU_RELEASE,
	    MENU_RELEASE_IMAGE,
	    NULL),
	    NULL);
}

/* Change the active station menu label for the named station */
void
active_change(
	Stationlist_t*		stlp)
{
	char*			itemname;
	Menu_item		mi;

	/* Find the associated active station menu item and replace it */
	if ((mi = active_findmenu(stlp->name)) == NULL)
		return;

	itemname = malloc(RADIO_CALLNAME_SIZE + 3);
	SPRINTF(itemname, "%s%s%s",
	    stlp->active ? " " : "[", stlp->name, stlp->active ? " " : "]");
	xv_set(StationMenu,
	    MENU_REPLACE_ITEM, mi, (Menu_item)xv_create(NULL, MENUITEM,
	    MENU_STRING, itemname,
	    MENU_VALUE, stlp,
	    MENU_NOTIFY_PROC, station_notify,
	    MENU_RELEASE,
	    MENU_RELEASE_IMAGE,
	    NULL),
	    NULL);
}

/* Delete the specified item */
void
active_delete(
	Stationlist_t*		stlp)
{
	Stationlist_t**		lp;
	Menu_item		mi;

	/* Find the associated active station menu item and remove it */
	mi = active_findmenu(stlp->name);
	if (mi != NULL) {
		xv_set(StationMenu, MENU_REMOVE_ITEM, mi, NULL);
		xv_destroy(mi);
	}

	/* Unlink and remove this station list entry */
	for (lp = &ActiveList; *lp != stlp; lp = &((*lp)->next));
	*lp = stlp->next;
	active_clear(stlp);
	(void) free((char *)stlp);

	/* If this was the last menu entry, insert the placeholder */
	if (ActiveList == NULL)
		xv_set(StationMenu, MENU_APPEND_ITEM, NoActiveStations, NULL);
}

/* Set the current station and dj fields from status message */
void
tune_station(
	char*			name,
	char*			dj)
{
	Stationlist_t*		stlp;

	stlp = active_find(name);
	if (stlp == NULL) {
		/* This is a new station */
		stlp = (Stationlist_t*) calloc(1, sizeof (*stlp));
		stlp->active = TRUE;
		STRCPY(stlp->name, name);
		active_add(stlp);
	}

	if (POWER_IS_ON) {
		/* Set the station name */
		STRNCPY(Callname, name, RADIO_CALLNAME_SIZE);
		Callname[RADIO_CALLNAME_SIZE] = '\0';
		xv_set(MFrame->StationButtonText, PANEL_VALUE, Callname, NULL);

		/* Set the dj, if known */
		if (dj != NULL) {
			replace_string(&stlp->dj, dj);
			replace_string(&DiscJockey, dj);
			xv_set(MFrame->DJText,
			    PANEL_VALUE, DiscJockey,
			    XV_SHOW, TRUE,
			    NULL);
		}
	}
}


/* Rebuild the active station menu from a station list */
void
rebuild_stationmenu(
	char**			ptr)
{
	Stationlist_t*		stlp;
	Stationlist_t*		newp;
	char*			id;
	int			active;

	/* Clear all the 'seen' flags in the active station list */
	for (stlp = ActiveList; stlp != NULL; stlp = stlp->next)
		stlp->seen = FALSE;

	/* Loop through each station name token */
	while ((id = *ptr) != NULL) {
		/* Strip off [] indicating a quiet station */
		if ((id[0] == '[') && (id[strlen(id) - 1] == ']')) {
			id = copy_string(&id[1]);
			id[strlen(id) - 1] = '\0';
			active = FALSE;
		} else {
			id = copy_string(id);
			active = TRUE;
		}

		/* Look for this station in the active list */
		stlp = active_find(id);
		if (stlp != NULL) {
			/* Update station menu item if status changed */
			stlp->seen = TRUE;
			if ((stlp->active && !active) ||
			    (!stlp->active && active)) {
				stlp->active = active;
				active_change(stlp);
			}
		} else {
			/* This is a new station */
			stlp = (Stationlist_t*) calloc(1, sizeof (*stlp));
			stlp->active = active;
			STRCPY(stlp->name, id);
			stlp->seen = TRUE;
			active_add(stlp);
		}
		(void) free(id);
		ptr++;
	}

	/* Now loop through the active list, removing inactive stations */
	for (stlp = ActiveList; stlp != NULL; ) {
		newp = stlp->next;
		if (!stlp->seen) {
			active_delete(stlp);
		}
		stlp = newp;
	}
}


/*
 * Notify callback function for `PropsButton'.
 */
/* ARGSUSED */
void
radio_RadioFrame_PropsButton_notify_callback(
	Panel_item		item,
	Event*			event)
{
	xv_set(PFrame->PropsFrame, XV_SHOW, TRUE, NULL);
}

/*
 * Notify callback function for `PowerButton'.
 */
/* ARGSUSED */
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
			return;
		}
	} else {
		xv_set(MFrame->RadioFrame, FRAME_BUSY, TRUE, NULL);
		signoff();
	}
}



/*
 * Notify callback function for `ScanButton'.
 */
void
scan_notify(
	Panel_item		item,
	Event*			event)
{
	MError("");
	if (POWER_IS_OFF) {
		Callname[0] = '\0';
		power_notify(item, 1, event);
	} else {
		start_scan();
	}
}

/* Set a station up as the current station */
void
station_setup(
	char*			name)
{
	char			msg[512];

	/* Quit now if no change */
	if ((*name == '\0') || (strcmp(name, Callname) == 0))
		return;

	/* Set the station name */
	STRNCPY(Callname, name, RADIO_CALLNAME_SIZE);
	Callname[RADIO_CALLNAME_SIZE] = '\0';
	xv_set(MFrame->StationButtonText, PANEL_VALUE, Callname, NULL);

	/* Clear the dj field */
	xv_set(MFrame->DJText, XV_SHOW, FALSE, NULL);

	/* If active, reset the station name */
	if (POWER_IS_ON) {
		SPRINTF(msg, "%s=%s\n", RADIOCMD_STATION, Callname);
		if (!send_recvmsg(msg))
			poweroff();
	}
}

/*
 * Notify routine for `StationMenu' items.
 */
void
station_notify(
	Menu			menu,
	Menu_item		mi)
{
	Stationlist_t*		stlp;

	MError("");
	/* Get the station structure and fill in current station */
	stlp = (Stationlist_t*) xv_get(mi, MENU_VALUE);
	(void) station_setup(stlp->name);
}

/*
 * Notify callback function for Preset Buttons.
 */
/* ARGSUSED */
void
preset_notify(
	Panel_item		item,
	Event*			event)
{
	MError("");
	/* Get the preset name and set the current station */
	(void) station_setup((char*)xv_get(item, PANEL_LABEL_STRING));
}


/*
 * Notify callback function for `AutoScanItem'.
 */
/* ARGSUSED */
void
autoscan_notify(
	Panel_item		item,
	int			value,
	Event*			event)
{
	PError("");

	/* Enable/disable the scan-delay item */
	xv_set(PFrame->ScanDelayGroup,
	    PANEL_INACTIVE, value ? FALSE : TRUE, NULL);
}

/* Return a copy of the current preset name */
char*
preset_currentname()
{
	char*			name;

	/* Get the current value of the type-in text field */
	name = copy_string((char*)
	    xv_get(MFrame->PresetItem, PANEL_VALUE));

	/* If no name there, get the current station name, if any */
	if (*name == '\0') {
		replace_string(&name, (char*)
		    xv_get(MFrame->StationButtonText, PANEL_VALUE));
	}
	/* If still no name, give up */
	if (*name == '\0') {
		(void) free(name);
		return (NULL);
	}
	return (name);
}

/* Return the preset button index number for the current preset name */
int
preset_selected()
{
	char*			name;
	int			i;

	name = preset_currentname();
	if (name != NULL) {
		for (i = 0; i < PRESETS; i++) {
			if (strcmp(name, (char*)
			    xv_get(PresetButtons[i], PANEL_LABEL_STRING)) == 0)
				break;
		}
		(void) free(name);
		if (i < PRESETS)		/* if a match was found */
			return (i);
	}
	return (-1);				/* no current name */
}

/* Set a preset button */
int
preset_set(
	char*			name)
{
	int			i;

	for (i = 0; i < PRESETS; i++) {
		if ((int) xv_get(PresetButtons[i], PANEL_INACTIVE)) {
			xv_set(PresetButtons[i],
			    PANEL_LABEL_STRING, name,
			    PANEL_INACTIVE, FALSE,
			    NULL);

			/* If that was the first preset, activate 'clr' */
			if (i == 0)
				xv_set(MFrame->ClearPresetButton,
				    PANEL_INACTIVE, FALSE, NULL);

			/* If that was the last preset, inactivate 'set' */
			if ((i + 1) == PRESETS)
				xv_set(MFrame->SetPresetButton,
				    PANEL_INACTIVE, TRUE, NULL);
			break;
		} else {
			/* If the preset is already set, do nothing */
			if (strcmp(name, (char*)
			    xv_get(PresetButtons[i], PANEL_LABEL_STRING)) == 0)
				return (FALSE);
		}
	}
	return (TRUE);
}

/* Clear a preset button */
void
preset_clear(
	int			p)
{
	int			i;
	int			active;
	int			allgone;

	/* Shuffle the rest of the buttons down */
	allgone = (p == 0);
	for (i = p++; p < PRESETS; p++) {
		/* Test to see if this button is active */
		active = FALSE;
		if (!xv_get(PresetButtons[p], PANEL_INACTIVE)) {
			allgone = FALSE;
			active = TRUE;
		}

		/* Copy this button down */
		xv_set(PresetButtons[i],
		    PANEL_LABEL_STRING,
		    xv_get(PresetButtons[p], PANEL_LABEL_STRING),
		    PANEL_INACTIVE, !active,
		    NULL);
		i = p;
	}

	/* Clear the last button */
	xv_set(PresetButtons[i],
	    PANEL_LABEL_STRING, " ",
	    PANEL_INACTIVE, TRUE,
	    NULL);

	/* If cleared all the presets, deactivate 'clr' */
	if (allgone)
		xv_set(MFrame->ClearPresetButton,
		    PANEL_INACTIVE, TRUE, NULL);

	/* Activate 'set' */
	xv_set(MFrame->SetPresetButton, PANEL_INACTIVE, FALSE, NULL);
}

/*
 * Notify callback function for `SetPresetButton'.
 */
/* ARGSUSED */
void
set_notify(
	Panel_item		item,
	Event*			event)
{
	char*			name;

	MError("");

	/* Find the item from the station name */
	name = preset_currentname();
	if (name == NULL) {
		return;
	}
	if (preset_set(name))
		rc_write();		/* Rewrite rc file */
	(void) free(name);
}

/*
 * Notify callback function for `ClearPresetButton'.
 */
/* ARGSUSED */
void
clear_notify(
	Panel_item		item,
	Event*			event)
{
	int			i;

	/* Find the item from the station name */
	MError("");
	i = preset_selected();
	if (i < 0) {
		return;
	}
	preset_clear(i);
	rc_write();		/* Rewrite rc file */
}

/*
 * Notify callback function for `ApplyButton'.
 */
/* ARGSUSED */
void
apply_notify(
	Panel_item		item,
	Event*			event)
{
	char			msg[512];

	PError("");

	/* Apply all property sheet values */
	if ((Autoscan != (int)xv_get(PFrame->AutoScanItem, PANEL_VALUE)) ||
	    (Scandelay != (int)xv_get(PFrame->ScanDelayItem, PANEL_VALUE))) {
		Autoscan = (int)xv_get(PFrame->AutoScanItem, PANEL_VALUE);
		Scandelay = (int)xv_get(PFrame->ScanDelayItem, PANEL_VALUE);
		autoscan_reset();
	}
	if (Autorelease != (int) xv_get(PFrame->AutoReleaseItem, PANEL_VALUE)) {
		Autorelease = (int)xv_get(PFrame->AutoReleaseItem, PANEL_VALUE);
		if (POWER_IS_ON) {
			SPRINTF(msg, "%s=%s\n", RADIOCMD_RELEASE,
			    (Autorelease ? "On" : "Off"));
			if (!send_recvmsg(msg))
				poweroff();
		}
	}

	if (strcmp(Output, (char*)xv_get(PFrame->OutputItem, PANEL_VALUE))
	    != 0) {
		replace_string(&Output,
		    (char*)xv_get(PFrame->OutputItem, PANEL_VALUE));
		if (POWER_IS_ON) {
			SPRINTF(msg, "%s=%s\n", RADIOCMD_OUTPUT, Output);
			if (!send_recvmsg(msg))
				poweroff();
		}
	}
	rc_write();		/* Rewrite rc file */
}

/*
 * Notify callback function for `ResetButton'.
 */
/* ARGSUSED */
void
reset_notify(
	Panel_item		item,
	Event*			event)
{
	PError("");

	/* Reset all property sheet values to current values */
	xv_set(PFrame->AutoScanItem, PANEL_VALUE, Autoscan ? 1 : 0, NULL);
	xv_set(PFrame->ScanDelayItem, PANEL_VALUE, Scandelay, NULL);
	xv_set(PFrame->ScanDelayGroup,
	    PANEL_INACTIVE, Autoscan ? FALSE : TRUE, NULL);
	xv_set(PFrame->AutoReleaseItem, PANEL_VALUE, Autorelease ? 1 : 0, NULL);
	xv_set(PFrame->OutputItem, PANEL_VALUE, Output, NULL);

	/* Don't dismiss the panel on reset */
	if (item != NULL)
		xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, 0);
}


/* Set the icon according to current state */
void
icon_switch()
{
	Xv_opaque		oldi;
	Xv_opaque		newi;
	Xv_opaque		mask;

	/* Change the icon only if it is different */
	oldi = xv_get(CurrentIcon, ICON_IMAGE);
	newi = (RecvState == ON_THE_AIR) ? Icon_on : Icon_off;
	mask = (RecvState == ON_THE_AIR) ? Icon_onmask : Icon_offmask;
	if (oldi != newi) {
		/* XXX - note that simply changing FRAME_ICON does not work */
		xv_set(CurrentIcon,
		    ICON_IMAGE, newi,
		    ICON_MASK_IMAGE, mask,
		    NULL);
	}
}


/* Notify routine for input from receiver process */
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
	xv_set(MFrame->RadioFrame, FRAME_BUSY, FALSE, NULL);

	while (input_ready(Recv_fromfd)) {
		Recv_established = TRUE;
		sptr = lex_multiline(Recv_fromfd);
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
			 * If station report, update active station menu.
			 * If station id, update dj field.
			 * If status message, parse and dispatch it.
			 * Otherwise, display the message in the footer and
			 * power down.
			 */
			ptr++;
			if (strcmp(*ptr, RADIOCMD_STATIONREPORT) == 0) {
				/* Build an active station list */
				rebuild_stationmenu(&ptr[1]);

			} else if (strcmp(*ptr, RADIOCMD_IDREPORT) == 0) {
				/* Set current station and callname */
				tune_station(ptr[1], ptr[2]);

			} else if ((strcmp(*ptr, RADIOCMD_STATUS) != 0) ||
			    (parse_cmds(&ptr[1], statuslist) != NULL)) {
				/* Display message in footer */
				msg[0] = '\0';
				while (*ptr != NULL) {
					STRCAT(msg, *ptr++);
					STRCAT(msg, " ");
				}
				MError(msg);

				/* If error, the receiver is powered down */
				poweroff();
			}
			ptr = ++eptr;
		}
		/* Free the whole input message */
		(void) free((char*)sptr);
	}
	return (NOTIFY_DONE);
}

/* Reset state when the receiver process appears to have died */
void
clear_child()
{
	char			msg[512];

	xv_set(MFrame->RadioFrame, FRAME_BUSY, FALSE, NULL);
	if (Recv_childpid != -1) {
		/* Clear out input buffer */
		(void) childinput_handler((Notify_client)MFrame, Recv_fromfd);

		/* Print a relevant error message */
		if (!Recv_established) {
			SPRINTF(msg,
			    T("Could not start the %s process"), Recv_basename);
			MError(msg);
		}

		/* Close down connections */
		Recv_childpid = -1;
		(void) notify_set_input_func((Notify_client)MFrame,
		    NOTIFY_FUNC_NULL, Recv_fromfd);
		(void) close(Recv_tofd);
		(void) close(Recv_fromfd);
		Recv_tofd = -1;
		Recv_fromfd = -1;

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
	char			msg[512];

	if (WIFEXITED(*status) || WIFSIGNALED(*status)) {
		if (pid == Recv_childpid) {
			clear_child();
		} else if (pid == Volume_childpid) {
			Volume_childpid = -1;
			if (WEXITSTATUS(*status) > 0) {
				SPRINTF(msg,
				    T("Could not start the %s process"),
				    Volume_basename);
				MError(msg);
			}
		}
		return (NOTIFY_DONE);
	}
	return (NOTIFY_IGNORED);
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

/* Set up XView callbacks for events concerning the receiver process */
void
register_reaper()
{
	(void) notify_set_signal_func((Notify_client)MFrame,
	    sigpipe_handler, SIGPIPE, NOTIFY_SYNC); /* used to be NOTIFY_SAFE */
	(void) notify_set_wait3_func((Notify_client)MFrame,
	    sigchild_handler, Recv_childpid);
	(void) notify_set_input_func((Notify_client)MFrame,
	    childinput_handler, Recv_fromfd);
}

/*
 * Notify callback function for `VolumeButton'.
 */
void
volume_notify(
	Panel_item		item,
	Event*			event)
{
	int			i;
	sigset_t		set;
	char			msg[512];

	if (Volume_childpid > 0)
		return;

	if ((Volume_childpid = fork()) == 0) {		/* if child... */
		/* Close all descriptors and reset signal handling */
		for (i = getdtablesize(); i >= 2; i--)
			(void) close(i);
		for (i = 0; i < NSIG; i++)
			(void) signal(i, SIG_DFL);
		(void) sigfillset(&set);
		(void) sigprocmask(SIG_UNBLOCK, &set, 0);

		/* Exec the volume control process */
		if (Volume_cmd1 != NULL)
			execl(Volume_cmd1, Volume_basename, NULL);
		execl("/bin/sh", "sh", "-c", Volume_cmd2, NULL);
		exit(ERR_NOEXEC);
	} else {
		(void) notify_set_wait3_func((Notify_client)MFrame,
		    sigchild_handler, Volume_childpid);
		SPRINTF(msg, "%s %s", T("Starting"), Volume_basename);
		MError(msg);
	}
}

Xv_opaque
XmitPanel_Init()
{
	/* Initialize panel and property sheet */
	MFrame = radio_RadioFrame_objects_initialize(NULL, NULL);
	PFrame = radio_PropsFrame_objects_initialize(NULL, MFrame->RadioFrame);
	if ((MFrame == NULL) || (PFrame == NULL)) {
		FPRINTF(stderr, "%s: %s\n", prog, T("Cannot initialize XView"));
		exit(1);
	}

	(void) notify_set_signal_func((Notify_client)MFrame,
	    sigint_handler, SIGINT, NOTIFY_SYNC); /* used to be NOTIFY_SAFE */

	/* XXX - work around guide bug */
	xv_set(MFrame->PresetItem,
	    XV_USE_DB, PANEL_VALUE_STORED_LENGTH, 4, NULL,
	    NULL);

	/* Set up an array of button item handles */
	PresetButtons[0] = MFrame->PresetButton1;
	PresetButtons[1] = MFrame->PresetButton2;
	PresetButtons[2] = MFrame->PresetButton3;
	PresetButtons[3] = MFrame->PresetButton4;
	PresetButtons[4] = MFrame->PresetButton5;
	PresetButtons[5] = MFrame->PresetButton6;

	StationMenu = (Menu) xv_get(MFrame->StationButton, PANEL_ITEM_MENU);
	NoActiveStations = (Menu_item) xv_create(XV_NULL, MENUITEM,
	    MENU_STRING, T("no active stations"),
	    MENU_VALUE, NULL,
	    NULL);
	xv_set(StationMenu,
	    MENU_INSERT, 1, NoActiveStations,
	    MENU_DEFAULT, 1,
	    NULL);

	CurrentIcon = (Icon) xv_get(MFrame->RadioFrame, FRAME_ICON);
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
	xv_set(MFrame->StationButtonText, PANEL_VALUE, Callname, NULL);
	xv_set(PFrame->ScanDelayItem, PANEL_VALUE, 60, NULL);
	xv_set(PFrame->OutputItem, PANEL_VALUE, Output, NULL);
	return (MFrame->RadioFrame);
}


/* Routines to support reading and writing the .radiorc file */

/* Parser routine declarations */
int	rc_service(char*);
int	rc_host(char*);
int	rc_power(char*);
int	rc_station(char*);
int	rc_autoscan(char*);
int	rc_scandelay(char*);
int	rc_autorelease(char*);
int	rc_output(char*);
int	rc_preset(char*);

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
	{"Radio_Power",			rc_power},
#define	RC_POWER	2
	{"Radio_Station",		rc_station},
#define	RC_STATION	3
	{"Radio_Auto_Scan",		rc_autoscan},
#define	RC_AUTOSCAN	4
	{"Radio_Scan_Delay",		rc_scandelay},
#define	RC_SCANDELAY	5
	{"Radio_Auto_Release",		rc_autorelease},
#define	RC_AUTORELEASE	6
	{"Radio_Audio_Output",		rc_output},
#define	RC_OUTPUT	7
						/* Station preset info */
	{"Radio_Preset",		rc_preset},
#define	RC_PRESET	8
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
rc_power(char* arg)
{
	if (Reading) {
		Poweron_init = is_on(arg);
	} else {
		radiorc_putstr(Rc_outfd, &Rc_tbl[RC_POWER],
		    POWER_IS_ON ? "On" : "Off");
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
rc_autoscan(char* arg)
{
	if (Reading) {
		Autoscan = is_on(arg);
	} else {
		radiorc_putstr(Rc_outfd, &Rc_tbl[RC_AUTOSCAN],
		    Autoscan ? "On" : "Off");
	}
	return (TRUE);
}

int
rc_scandelay(char* arg)
{
	if (Reading) {
		Scandelay = atoi(arg);
	} else {
		radiorc_putval(Rc_outfd, &Rc_tbl[RC_SCANDELAY], Scandelay);
	}
	return (TRUE);
}

int
rc_autorelease(char* arg)
{
	if (Reading) {
		Autorelease = is_on(arg);
	} else {
		radiorc_putstr(Rc_outfd, &Rc_tbl[RC_AUTORELEASE],
		    Autorelease ? "On" : "Off");
	}
	return (TRUE);
}

int
rc_output(char* arg)
{
	if (Reading) {
		replace_string(&Output, arg);
	} else {
		radiorc_putstr(Rc_outfd, &Rc_tbl[RC_OUTPUT], Output);
	}
	return (TRUE);
}

/* When a new station preset entry is encountered, reset station ptrs */
int
rc_preset(char* arg)
{
	Stationlist_t*		stlp;
	Stationlist_t**		oldp;

	if (Reading) {
		/* Ignore this entry if the station name is null */
		if (arg[0] == '\0') {
			return (TRUE);
		}

		/* Enter this preset into a new structure and append it */
		stlp = (Stationlist_t*) calloc(1, sizeof (*stlp));
		stlp->next = NULL;
		STRNCPY(stlp->name, arg, RADIO_CALLNAME_SIZE);
		stlp->name[RADIO_CALLNAME_SIZE] = '\0';
		oldp = &PresetList;
		while (*oldp != NULL) {
			oldp = &((*oldp)->next);
		}
		*oldp = stlp;
	} else {
		/*  When a preset entry is found, write the whole preset list */
		for (stlp = PresetList; stlp != NULL; stlp = stlp->next) {
			/* Make sure this preset is written only once */
			if (stlp->seen++ == 0) {
				/* Clear the 'seen' flag and write it */
				Rc_tbl[RC_PRESET].seen = FALSE;
				radiorc_putstr(Rc_outfd,
				    &Rc_tbl[RC_PRESET], stlp->name);
			}
		}
	}
	return (TRUE);
}


/* Read the .radiorc file to load state; can be called without XView panels */
void
rc_load()
{
	int			oldfd;
	char**			sptr;
	char**			ptr;

	/* Get a handle to the .radiorc file, if any */
	oldfd = radiorc_open(O_RDONLY);
	if (oldfd < 0)
		goto error;

	/* Initialize the preset list */
	PresetList = NULL;
	Reading = TRUE;

	/* Read in and parse the file */
	while ((sptr = lex_multiline(oldfd)) != NULL) {

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
	radiorc_close(oldfd);
	return;
}

/* Walk the station preset list, creating presets */
void
rc_makelist()
{
	Stationlist_t*		stlp;
	Stationlist_t*		oldp;

	/* XXX - should destroy old list first */

	stlp = PresetList;
	while (stlp != NULL) {
		/* Add this station */
		(void) preset_set(stlp->name);
		oldp = stlp;
		stlp = oldp->next;
		(void) free((char*)oldp);
	}
	PresetList = NULL;
}

/* Read the .radiorc file to load state */
void
rc_read()
{
	/* This is going to take a while */
	xv_set(MFrame->RadioFrame, FRAME_BUSY, TRUE, NULL);

	/* Parse the rc file */
	rc_load();
	rc_makelist();

	xv_set(MFrame->RadioFrame, FRAME_BUSY, FALSE, NULL);
}

/* Update the .radiorc file with current values */
void
rc_write()
{
	int			i;
	int			nrows;
	int			oldfd;
	char**			sptr;
	char**			ptr;
	char**			eptr;
	int			lastblank;
	Stationlist_t*		stlp;
	Stationlist_t*		oldp;
	char			msg[512];

	/* This is going to take a while */
	xv_set(MFrame->RadioFrame, FRAME_BUSY, TRUE, NULL);

	/* Get a handle to the old file, if any; then unlink and rewrite it */
	oldfd = radiorc_open(O_RDONLY);
	Rc_outfd = radiorc_open(O_WRONLY | O_EXCL);
	if (Rc_outfd < 0)
		goto error;

	/* Mark all items that must be written out */
	Rc_tbl[RC_SERVICE].seen = FALSE;
	Rc_tbl[RC_HOST].seen = FALSE;
	Rc_tbl[RC_POWER].seen = FALSE;
	Rc_tbl[RC_STATION].seen = FALSE;
	Rc_tbl[RC_AUTOSCAN].seen = FALSE;
	Rc_tbl[RC_SCANDELAY].seen = FALSE;
	Rc_tbl[RC_AUTORELEASE].seen = FALSE;
	Rc_tbl[RC_OUTPUT].seen = FALSE;
	Rc_tbl[RC_PRESET].seen = FALSE;

	/* Initialize the preset list */
	PresetList = NULL;
	Reading = FALSE;

	/* Build the preset list (back to front) */
	for (i = PRESETS; --i >= 0; ) {
		stlp = (Stationlist_t*) calloc(1, sizeof (*stlp));
		stlp->next = PresetList;
		PresetList = stlp;
		STRCPY(stlp->name, (char*) xv_get(PresetButtons[i],
		    PANEL_LABEL_STRING));
	}

	/* If there was an old file, rewrite it now */
	if (oldfd >= 0) {
		lastblank = TRUE;
		while ((sptr = lex_line(oldfd)) != NULL) {
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
		radiorc_close(oldfd);
	}

	/* Write out all parameters that were not handled above */
	(void) (*Rc_tbl[RC_SERVICE].func)("");
	(void) (*Rc_tbl[RC_HOST].func)("");
	radiorc_putblank(Rc_outfd);
	(void) (*Rc_tbl[RC_POWER].func)("");
	(void) (*Rc_tbl[RC_STATION].func)("");
	(void) (*Rc_tbl[RC_AUTOSCAN].func)("");
	(void) (*Rc_tbl[RC_SCANDELAY].func)("");
	(void) (*Rc_tbl[RC_AUTORELEASE].func)("");
	(void) (*Rc_tbl[RC_OUTPUT].func)("");

	/* Write out all presets that were not handled above */
	for (stlp = PresetList; stlp != NULL; ) {
		/* Make sure this preset is written only once */
		if (stlp->seen++ == 0) {
			/* Clear the 'seen' flag and write it out */
			Rc_tbl[RC_PRESET].seen = FALSE;
			radiorc_putstr(Rc_outfd,
			    &Rc_tbl[RC_PRESET], stlp->name);
		}
		/* Free this entry */
		oldp = stlp;
		stlp = stlp->next;
		(void) free((char*) oldp);
	}

	radiorc_close(Rc_outfd);
	Rc_outfd = -1;
	xv_set(MFrame->RadioFrame, FRAME_BUSY, FALSE, NULL);
	return;

error:
	radiorc_close(oldfd);
	radiorc_close(Rc_outfd);
	Rc_outfd = -1;
	SPRINTF(msg, T("Could not write defaults to ~/%s"), RADIO_RCFILE);
	PError(msg);
	xv_set(MFrame->RadioFrame, FRAME_BUSY, FALSE, NULL);
}



/* radio entry point */
main(
	int			argc,
	char*			argv[])
{
	char*			path;
	char*			s;
	int			i;
	Xv_opaque		panel;

	/* malloc_debug(2); */

	/* Get the program name */
	prog = strrchr(argv[0], '/');
	if (prog == NULL)
		prog = argv[0];
	else
		prog++;

	/*
	 * Construct a pathname to initiate the receiver program.
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

	/* Look for the receiver in the both this directory in $PATH */
	if (*path != '\0') {
		Recv_cmd1 = malloc(strlen(path) +
		    strlen(Recv_basename) + 1);
		SPRINTF(Recv_cmd1, "%s%s", path, Recv_basename);
	}
	Recv_cmd2 = malloc(strlen(Shell_prefix) + 
	    strlen(Recv_basename) + strlen(Recv_args) + 1);
	SPRINTF(Recv_cmd2, "%s%s%s",
	    Shell_prefix, Recv_basename, Recv_args);

	/* Look for gaintool in both this directory and anywhere in $PATH */
	if (*path != '\0') {
		Volume_cmd1 = malloc(strlen(path) +
		    strlen(Volume_basename) + 1);
		SPRINTF(Volume_cmd1, "%s%s", path, Volume_basename);
	}
	Volume_cmd2 = malloc(strlen(Shell_prefix) +
	    strlen(Volume_basename) + 1);
	SPRINTF(Volume_cmd2, "%s%s", Shell_prefix, Volume_basename);

	(void) free(path);

	/* Initialize I18N and XView */
	INSTANCE = xv_unique_key();
	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
	    XV_USE_LOCALE, TRUE,
	    0);

	/* Set defaults and then load more defaults from .radiorc file */
	replace_string(&DiscJockey, "");
	replace_string(&Output, AUDIO_DEVICE);
	replace_string(&Service, RADIO_DEFAULT_SERVICE);
	replace_string(&Host, RADIO_DEFAULT_ADDRESS);
	rc_load();

	/* Parse start-up options */
	while ((i = getopt(argc, argv, "s:h:D")) != EOF) switch (i) {
	case 's':			/* specify port number/service name */
		replace_string(&Service, optarg);
		break;
	case 'h':			/* specify multicast host/address */
		replace_string(&Host, optarg);
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

			/* Try the NIC-supplied address */
			} else if (net_parse_address(RADIO_NIC_ADDRESS, NULL)
			    >= 0) {
				replace_string(&Host, RADIO_NIC_ADDRESS);
				FPRINTF(stderr,
			    T("\tUsing default multicast address '%s'\n"), Host);
			}
		} else {
			/* If no multicast OS support, just use UDP */
			replace_string(&Host, NET_BROADCAST_NAME);
			FPRINTF(stderr,
	    T("%s:\tDefaulting to UDP broadcast (local subnet only)\n"), prog);
		}
	}

	panel = XmitPanel_Init();
	state_set(POWER_OFF);

	/* Set up the station presets, init the property sheet */
	rc_makelist();
	reset_notify((Panel_item)NULL, (Event*)NULL);
	PError("");

	/* Start up with power on? */
	if (Poweron_init)
		(void) poweron();

	xv_main_loop(panel);

	exit_cleanup();
	exit(0);
}
