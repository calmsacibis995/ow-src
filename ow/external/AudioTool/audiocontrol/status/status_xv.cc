/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)status_xv.cc	1.11	93/02/25 SMI"

#include <stdio.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <group.h>

#include "ds_popup.h"
#include "status_xv.h"


Attr_attribute	STATUSKEY;		// Global key for local storage


// Create and return an XView Status Panel
Statusctl*
Create_Statusctl(
	char*			name,
	ptr_t			parent)
{
	Statusctl_xv*		xvp;

	xvp = new Statusctl_xv(parent);
	if (xvp->Setdevice(name) < 0) {
		delete xvp;
		xvp = NULL;
	}
	return ((Statusctl*) xvp);
}


// Statusctl_xv implementation


// Entered any time there is a change of state of the audio device.
// The Notifier schedules this routine synchronously, so we don't
// have to worry about stomping on global data structures.
static Notify_value
sigpoll_handler(
	Notify_client		client,
	int,
	Notify_signal_mode)
{
	Statusctl_xv*		xvp;

	xvp = (Statusctl_xv*) xv_get(client, XV_KEY_DATA, STATUSKEY);
	xvp->Update();
	return (NOTIFY_DONE);
}

// Timer handler...entered periodically when continuous update is selected
// The Notifier schedules this routine synchronously, so we don't
// have to worry about stomping on global data structures.
static Notify_value
timer_handler(
	Notify_client		client,
	int)
{
	Statusctl_xv*		xvp;

	xvp = (Statusctl_xv*) xv_get(client, XV_KEY_DATA, STATUSKEY);
	xvp->Update();
	return (NOTIFY_DONE);
}

// Panel event proc is called for PROPS key and MENU events
static Notify_value
event_proc(
	Frame			frame,
	Event*			event,
	Notify_arg		arg,
	Notify_event_type	type)
{
	Statusctl_xv*		xvp;

	if ((event_action(event) == ACTION_PROPS) && event_is_up(event)) {
		Audioctl_show_playpanel();
		return (NOTIFY_DONE);
	}
	if ((event_action(event) == ACTION_MENU) && event_is_down(event)) {
		xvp = (Statusctl_xv*)
		    xv_get(frame, XV_KEY_DATA, STATUSKEY);
		menu_show(xvp->menu, frame, event, 0);
		return (NOTIFY_DONE);
	}
	return (notify_next_event_func(frame, (Notify_event)event, arg, type));
}


// Constructor
Statusctl_xv::
Statusctl_xv(
	ptr_t			p):
	Statusctl(p)
{
	char*			t;
	unsigned long		i;
	unsigned long		max_width;
	Rect*			rect;

	// Init global keys, if necessary
	if (INSTANCE == 0)
		INSTANCE = xv_unique_key();
	if (STATUSKEY == 0)
		STATUSKEY = xv_unique_key();

	// Initialize XView record control panel
	// Save the address of the container class
	objs = new status_StatusPanel_objects;
	objs->objects_initialize((Xv_opaque) parent);
	xv_set(objs->StatusPanel, XV_KEY_DATA, STATUSKEY, this, NULL);
	xv_set(objs->PlayStatusCanvas, XV_KEY_DATA, STATUSKEY, this, NULL);
	xv_set(objs->RecordStatusCanvas, XV_KEY_DATA, STATUSKEY, this,
	    WIN_RIGHT_OF, objs->PlayStatusCanvas, NULL);
	xv_set(objs->StatusControlCanvas, XV_KEY_DATA, STATUSKEY, this,
	    WIN_BELOW, objs->PlayStatusCanvas, NULL);

	// Save the default frame label
	t = (char*) xv_get(objs->StatusPanel, FRAME_LABEL);
	default_title = new char[strlen(t) + 1];
	(void) strcpy(default_title, t);

	displayed = FALSE;
	positioned = FALSE;

	// Initialize floating menu and set notify interposer for each canvas
	menu = status_StatusPanel_menu_create((caddr_t)objs,
	    objs->StatusPanel);
	notify_interpose_event_func(objs->PlayStatusCanvas,
	    (Notify_func) event_proc, NOTIFY_SAFE);
	notify_interpose_event_func(objs->RecordStatusCanvas,
	    (Notify_func) event_proc, NOTIFY_SAFE);
	notify_interpose_event_func(objs->StatusControlCanvas,
	    (Notify_func) event_proc, NOTIFY_SAFE);

	// XXX - workaround Guide bug
	xv_set(menu, MENU_TITLE_ITEM, MGET("Audio Control"), NULL);

	// Set exclusive choice buttons to be the same width
	for (max_width = 0, i = 0; i < 2; i++) {
		rect = (Rect*)xv_get(objs->Update_switch, PANEL_CHOICE_RECT, i);
		if (rect->r_width > max_width)
			max_width = rect->r_width;
	}
	for (i = 0; i < 2; i++) {
		// XXX - setting XV_WIDTH *should* work!
		char*	oldname;
		char*	newname;

		while (rect = (Rect*)
		    xv_get(objs->Update_switch, PANEL_CHOICE_RECT, i),
		    rect->r_width < (max_width - 8)) {
			oldname = (char*)
			    xv_get(objs->Update_switch, PANEL_CHOICE_STRING, i);
			newname = new char[strlen(oldname) + 3];
			SPRINTF(newname, " %s ", oldname);
			xv_set(objs->Update_switch,
			    PANEL_CHOICE_STRING, i, newname, NULL);
			delete newname;
		}
	}

	// Save minimum width of panel
	min_width = (int) (xv_get(objs->PlayStatusCanvas, XV_WIDTH) +
	    xv_get(objs->RecordStatusCanvas, XV_WIDTH));

	resize();
}

// Destructor
Statusctl_xv::
~Statusctl_xv()
{
	// XXX - anything go here??
}

// Set device
int Statusctl_xv::
Setdevice(
	char*	name)
{
	int	i;
	char*	devname;
	char*	newname;

	i = Audioctl::Setdevice(name);
	devname = Getdevice();
	newname = new char[strlen(default_title) + strlen(devname) + 10];
	SPRINTF(newname, "%s %s Status", default_title, devname);
	xv_set(objs->StatusPanel, FRAME_LABEL, newname, NULL);
	delete newname;
	return (i);
}

// set positioned flag (return old value)
int Statusctl_xv::
Setpositioned(int flag)
{
	int oldval = positioned;
	
	positioned = flag;
	return (oldval);
}

// Return panel handle
ptr_t Statusctl_xv::
Gethandle()
{
	return ((ptr_t)objs->StatusPanel);
}

// Adjust panel width
void Statusctl_xv::
Setwidth(
	int		w)
{
	// Adjust wider, only within reason
	if ((w > min_width) && ((w - min_width) < 20)) {
		min_width = w;
		resize();
	}
}

// Reposition everything
void Statusctl_xv::
resize()
{
	unsigned long		i;
	unsigned long		j;
	unsigned long		max_width;
	unsigned long		bottom;
	Rect*			rect;

	// Relocate everything along the Y axis
	bottom = xv_get(objs->PlayTitleGroup, XV_Y) +
	    xv_get(objs->PlayTitleGroup, XV_HEIGHT) + 10;
	xv_set(objs->PlayS3Group,
	    XV_Y, bottom, GROUP_VERTICAL_OFFSET, bottom, NULL);
	xv_set(objs->RecS3Group,
	    XV_Y, bottom, GROUP_VERTICAL_OFFSET, bottom, NULL);
	i = xv_get(objs->PlayM3Group, XV_HEIGHT) + 10;
	bottom += xv_get(objs->PlayS3Group, XV_HEIGHT) + 16 + i;
	xv_set(objs->PlayStatusCanvas, XV_HEIGHT, bottom, NULL);
	xv_set(objs->PlayM3Group, GROUP_LAYOUT, TRUE, NULL);
	xv_set(objs->RecordStatusCanvas, XV_HEIGHT, bottom, NULL);
	xv_set(objs->RecM3Group, GROUP_LAYOUT, TRUE, NULL);

	// Resize the window to make everything fit properly
	max_width = xv_get(objs->PlayStatusCanvas, XV_WIDTH) +
	    xv_get(objs->RecordStatusCanvas, XV_WIDTH);
	i = xv_get(objs->StatuscontrolGroup, XV_WIDTH) +
	    (4 * xv_get(objs->Popen_flag, XV_X));
	if (i > max_width)
		max_width = i;
	if (min_width > max_width)
		max_width = min_width;
	xv_set(objs->PlayStatusCanvas, XV_WIDTH, max_width / 2, NULL);
	xv_set(objs->RecordStatusCanvas, XV_WIDTH, max_width / 2, NULL);
	xv_set(objs->StatusControlCanvas, XV_WIDTH, max_width, NULL);
	min_width = (int) max_width;

	// Re-center the titles
	xv_set(objs->PlayTitleGroup, GROUP_LAYOUT, TRUE, NULL);
	xv_set(objs->RecordTitleGroup, GROUP_LAYOUT, TRUE, NULL);
	xv_set(objs->StatuscontrolGroup, GROUP_LAYOUT, TRUE, NULL);

	// Move over the update control, if there is room
	i = xv_get(objs->StatuscontrolGroup, XV_X);
	j = 3 * xv_get(objs->Popen_flag, XV_X);
	rect = (Rect*) xv_get(objs->Update_switch, PANEL_CHOICE_RECT, 1);
	if (i > j) {
		i = xv_get(objs->StatuscontrolGroup, XV_WIDTH) - rect->r_width;
		if (i < ((max_width / 2) - j))
			j = (max_width / 2) - i;
		xv_set(objs->StatuscontrolGroup, XV_X, j, NULL);
	}

	// Re-tile the panels and fit the frame
	xv_set(objs->RecordStatusCanvas,
	    XV_X, xv_get(objs->PlayStatusCanvas, XV_WIDTH), NULL);
	xv_set(objs->StatusControlCanvas, XV_X, 0,
	    XV_Y, xv_get(objs->PlayStatusCanvas, XV_HEIGHT), NULL);
	xv_set(objs->StatusPanel,
	    XV_WIDTH, max_width + 2,
	    XV_HEIGHT, xv_get(objs->StatusControlCanvas, XV_Y) +
	    xv_get(objs->StatusControlCanvas, XV_HEIGHT) + 2,
	    NULL);
}

// Display/open the panel
void Statusctl_xv::
Show()
{
	// Make sure the panel info is current
	Update();

	if (!displayed) {
		// Catch SIGPOLL to detect record control changes
		notify_set_signal_func(objs->StatusPanel,
		    (Notify_func) sigpoll_handler, SIGPOLL, NOTIFY_SYNC);
		displayed = TRUE;

		// Set timer, if in continuous update mode
		set_timer(continuous);

		if ((parent != NULL) && (positioned == FALSE)) {
			ds_position_popup((Frame)parent,
			    (Frame)objs->StatusPanel, DS_POPUP_AOB);
			positioned = TRUE;
		}
	}
	// XXX - workaround a bug that shrinks the top panels
	xv_set(objs->PlayStatusCanvas, XV_HEIGHT,
	    xv_get(objs->StatusControlCanvas, XV_Y), NULL);
	xv_set(objs->RecordStatusCanvas, XV_HEIGHT,
	    xv_get(objs->StatusControlCanvas, XV_Y), NULL);

	xv_set(objs->StatusPanel, XV_SHOW, TRUE, NULL);
}

// Done callback function for `StatusPanel'.
void
Status_done_proc(
	Frame			frame)
{
	Statusctl_xv*			xvp;

	// Unmap the frame (it better not be pinned!)
	xv_set(frame, XV_SHOW, FALSE, NULL);

	// Cleanup
	xvp = (Statusctl_xv*) xv_get(frame, XV_KEY_DATA, STATUSKEY);
	xvp->Unshow(0);
}

// Cleanup on takedown of the Status Panel
void Statusctl_xv::
Unshow(int)
{
	if (!displayed)
		return;
	displayed = FALSE;
	
	// Init state so next Update() will set everything
	info.Clear();

	// Quit catching SIGPOLL
	notify_set_signal_func(objs->StatusPanel, NOTIFY_FUNC_NULL, SIGPOLL,
	    NOTIFY_SYNC);

	// Cancel timer
	set_timer(FALSE);
}


// Force a dismissal of the Status Panel
void Statusctl_xv::
Unshow()
{
	if (!displayed)
		return;

	// This will take the frame down if it is pinned
	xv_set(objs->StatusPanel, 
	       FRAME_CMD_PIN_STATE, FRAME_CMD_PIN_OUT, 
	       XV_SHOW, FALSE,
	       NULL);

	// Finish cleaning up
	Unshow(0);
}

// Update the Status Panel according to the given values
void Statusctl_xv::
update_display(
	AudioInfo&	nip)
{
	char		buf[32];
	char*		p;
	AudioHdr	hdr;

// XXX - Assumes audio state init value of ~0 is correct
#define	UC_INIT			((unsigned char)(~0))
#define	ENABLE(I)		xv_set(objs->I,				\
				    XV_SHOW, TRUE, NULL);
#define	DISABLE(I)		xv_set(objs->I,				\
				    XV_SHOW, FALSE, NULL);
#define	SET_STRING(I, S)	xv_set(objs->I, XV_SHOW, TRUE,		\
				    PANEL_LABEL_STRING, (S), NULL);
#define	SET_VAL(I, X)		SPRINTF(buf, "%u", nip->X);		\
				SET_STRING(I, buf);
#define	NEWVAL(I, X)		if (nip->X != ~0) {			\
					SET_VAL(I, X);			\
				}
#define	SET_SECS(I, F)		SPRINTF(buf, "%.3f secs", F);		\
				SET_STRING(I, buf);

#define	Set_Statuslight(I, on)	xv_set(objs->I,				\
				    PANEL_VALUE, ((on) ? 1 : 0), NULL);

#define	NEWSTATE(I, X)		if (nip->X != UC_INIT) {		\
					Set_Statuslight(I, nip->X);	\
				}

	// Set play state
	NEWSTATE(Popen_flag, play.open);
	NEWSTATE(Ppaused_flag, play.pause);
	NEWSTATE(Pactive_flag, play.active);
	NEWSTATE(Perror_flag, play.error);
	NEWSTATE(Pwaiting_flag, play.waiting);
	NEWVAL(Peof_value, play.eof);
	NEWVAL(Psam_value, play.samples);

#ifdef SUNOS41
	DISABLE(Pbuf_label);
	DISABLE(Pbuf_value);
#else
	if (nip->play.buffer_size != ~0) {
		if (nip->play.buffer_size == 0) {
			DISABLE(Pbuf_label);
			DISABLE(Pbuf_value);
		} else {
			ENABLE(Pbuf_label);
			SET_SECS(Pbuf_value,
			    (double) nip->play.buffer_size /
			    ((double) (info->play.sample_rate *
			    info->play.channels * info->play.precision) / 8.));
		}
	}
#endif
	// If any format change, update the whole thing
	if ((nip->play.sample_rate != ~0) || (nip->play.channels != ~0) ||
	    (nip->play.encoding != ~0) || (nip->play.precision != ~0)) {
		hdr = ctldev->GetWriteHeader();
		SET_STRING(Pencode_value, p = hdr.EncodingString()); delete p;
		SET_STRING(Prate_value, p = hdr.RateString()); delete p;
		SET_STRING(Pchan_value, p = hdr.ChannelString()); delete p;
	}

	// Set record state
	NEWSTATE(Ropen_flag, record.open);
	NEWSTATE(Rpaused_flag, record.pause);
	NEWSTATE(Ractive_flag, record.active);
	NEWSTATE(Rerror_flag, record.error);
	NEWSTATE(Rwaiting_flag, record.waiting);
	NEWVAL(Rsam_value, record.samples);
#ifdef SUNOS41
	DISABLE(Rbuf_label);
	DISABLE(Rbuf_value);
#else
	if (nip->record.buffer_size != ~0) {
		if (nip->record.buffer_size == 0) {
			DISABLE(Rbuf_label);
			DISABLE(Rbuf_value);
		} else {
			ENABLE(Rbuf_label);
			SET_SECS(Rbuf_value,
			    (double) nip->record.buffer_size /
			    ((double) (info->record.sample_rate *
			    info->record.channels * info->record.precision) /
			    8.));
		}
	}
#endif
	// If any format change, update the whole thing
	if ((nip->record.sample_rate != ~0) || (nip->record.channels != ~0) ||
	    (nip->record.encoding != ~0) || (nip->record.precision != ~0)) {
		hdr = ctldev->GetReadHeader();
		SET_STRING(Rencode_value, p = hdr.EncodingString()); delete p;
		SET_STRING(Rrate_value, p = hdr.RateString()); delete p;
		SET_STRING(Rchan_value, p = hdr.ChannelString()); delete p;
	}
}

// Enable/Disable timer
void Statusctl_xv::
set_timer(
	Boolean	 		on)
{
	struct itimerval	timer;

	if (on) {			// Update Mode: Continuous
		// Set up interval timer
		timer.it_value.tv_usec = 1000000 / 4;		// 1/4 second
		timer.it_value.tv_sec = 0;
		timer.it_interval.tv_usec = 1000000 / 4;	// 1/4 second
		timer.it_interval.tv_sec = 0;

		(void) notify_set_itimer_func(objs->StatusPanel,
		    (Notify_func)timer_handler, ITIMER_REAL,
		    &timer, (struct itimerval *)0);

	} else {			/* Update Mode: Status Change */
		/* Cancel timer */
		(void) notify_set_itimer_func(objs->StatusPanel,
		    (Notify_func)timer_handler, ITIMER_REAL,
		    (struct itimerval *)0, (struct itimerval *)0);
	}
}


// XView callbacks, adapted from status_stubs.cc


/* Null event callback for read-only status items */
/* ARGSUSED */
void
Status_null_event_proc(
	Panel_item,
	Event*)
{
}

// Event callback function for `StatusPanel'.
Notify_value
status_StatusPanel_event_callback(
	Xv_window		win,
	Event*			event,
	Notify_arg		arg,
	Notify_event_type	type)
{
	Statusctl_xv*		xvp;

	switch (event_action(event)) {
	case WIN_RESIZE:
	case ACTION_OPEN:
	case ACTION_CLOSE:
		xvp = (Statusctl_xv*) xv_get(win, XV_KEY_DATA, STATUSKEY);
		switch (event_action(event)) {
		case ACTION_OPEN:
			xvp->Show();
			break;
		case ACTION_CLOSE:
			xvp->Unshow(0);
			break;
		}
		break;
	}
	return (notify_next_event_func(win, (Notify_event) event, arg, type));
}

// Notify callback function for `Update_switch'.
void
Status_update_proc(
	Panel_item	item,
	int	 	value,
	Event*)
{
	status_StatusPanel_objects*	ip;
	Statusctl_xv*		xvp;

	ip = (status_StatusPanel_objects*) xv_get(item, XV_KEY_DATA, INSTANCE);
	xvp = (Statusctl_xv*) xv_get(ip->StatusPanel, XV_KEY_DATA, STATUSKEY);
	xvp->continuous = (value != 0);
	xvp->set_timer(xvp->continuous);
}


// Menu handler for `StatusPanel_menu (Play...)'.
Menu_item
status_StatusPanel_menu_item0_callback(
	Menu_item	item,
	Menu_generate	op)
{
	switch (op) {
	case MENU_NOTIFY:
		Audioctl_show_playpanel();
		break;
	}
	return (item);
}

// Menu handler for `StatusPanel_menu (Record...)'.
Menu_item
status_StatusPanel_menu_item1_callback(
	Menu_item	item,
	Menu_generate	op)
{
	switch (op) {
	case MENU_NOTIFY:
		Audioctl_show_recpanel();
		break;
	}
	return (item);
}
