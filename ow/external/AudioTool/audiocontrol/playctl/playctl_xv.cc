/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)playctl_xv.cc	1.21	97/01/14 SMI"

#include <stdio.h>
// if defined(__ppc)
#include <sunmath.h>
// endif
#include <xview/xview.h>
#include <xview/panel.h>
#include <group.h>

#include "playctl_xv.h"
#include "ds_popup.h"

Attr_attribute	INSTANCE;		// Devguide global key
Attr_attribute	PLAYCTLKEY;		// Global key for local storage

// Number of extra balance values for a center detent (must be an even number)
#define	BALANCE_MIDPTS	(8)


// Create and return an XView Play Control Panel
Playctl*
Create_Playctl(
	char*			name,
	ptr_t			parent)
{
	Playctl_xv*		xvp;

	xvp = new Playctl_xv(parent);
	if (xvp->Setdevice(name) < 0) {
		(void) fprintf(stderr, "%s: %s %s\n",
		    MGET("Audio Control"),
		    MGET("Could not open"),
		    name == NULL ? MGET("audio device") : name);
		delete xvp;
		xvp = NULL;
	}
	return ((Playctl*) xvp);
}


// Playctl_xv implementation


// Entered any time there is a change of state of the audio device.
// The Notifier schedules this routine synchronously, so we don't
// have to worry about stomping on global data structures.
static Notify_value
sigpoll_active_handler(
	Notify_client		client,
	int,
	Notify_signal_mode)
{
	Playctl_xv*		xvp;

	xvp = (Playctl_xv*) xv_get(client, XV_KEY_DATA, PLAYCTLKEY);
	xvp->Update();
	return (NOTIFY_DONE);
}

static Notify_value
sigpoll_inactive_handler(
	Notify_client,
	int,
	Notify_signal_mode)
{
	// do nothing - we're unmapped or iconified
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
	Playctl_xv*		xvp;

	if ((event_action(event) == ACTION_PROPS) && event_is_up(event)) {
		Audioctl_show_statuspanel();
		return (NOTIFY_DONE);
	}
	if ((event_action(event) == ACTION_MENU) && event_is_down(event)) {
		xvp = (Playctl_xv*)
		    xv_get(frame, XV_KEY_DATA, PLAYCTLKEY);
		menu_show(xvp->menu, frame, event, 0);
		return (NOTIFY_DONE);
	}
	return (notify_next_event_func(frame, (Notify_event)event, arg, type));
}


// Constructor
Playctl_xv::
Playctl_xv(ptr_t parent_local):
	Playctl(parent_local)
{
	// Init global keys, if necessary
	if (INSTANCE == 0)
		INSTANCE = xv_unique_key();
	if (PLAYCTLKEY == 0)
		PLAYCTLKEY = xv_unique_key();

	// Initialize XView play control panel
	// Save the address of the container class
	objs = new playctl_PlayctlPanel_objects;
	objs->objects_initialize((Xv_opaque)parent_local);
	xv_set(objs->PlayctlPanel, XV_KEY_DATA, PLAYCTLKEY, this, NULL);
	xv_set(objs->PlayctlCanvas, XV_KEY_DATA, PLAYCTLKEY, this, NULL);

	// Catch, but ignore, SIGPOLL until panel is mapped
	notify_set_signal_func(objs->PlayctlPanel, 
			       (Notify_func) sigpoll_inactive_handler,
			       SIGPOLL, NOTIFY_SYNC);

	iconic = FALSE;
	displayed = FALSE;
	positioned = FALSE;
	balance = TRUE;

	// Set the sliders to be fully interactive
	xv_set(objs->PlayVolumeSlider, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
	xv_set(objs->PlayBalanceSlider, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);

	// Get the maximum gain from the interface
	max_gain = (double)((unsigned)
	    xv_get(objs->PlayVolumeSlider, PANEL_MAX_VALUE));

	// Get the maximum balance, then add some pixels for a center detent
	max_balance = (unsigned)
	    xv_get(objs->PlayBalanceSlider, PANEL_MAX_VALUE);
	xv_set(objs->PlayBalanceSlider,
	    PANEL_MAX_VALUE, max_balance + BALANCE_MIDPTS, NULL);

	// Save the handles to all the outport items
	outports[0] = objs->Outport0;
	outports[1] = objs->Outport1;
	outports[2] = objs->Outport2;

	// Initialize floating menu and set up a notify interposer
	menu = playctl_PlayctlPanel_menu_create((caddr_t)objs,
	    objs->PlayctlPanel);
	notify_interpose_event_func(objs->PlayctlCanvas,
	    (Notify_func) event_proc, NOTIFY_SAFE);

	// XXX - workaround Guide bug
	xv_set(menu, MENU_TITLE_ITEM, MGET("Audio Control"), NULL);

	// Save minimum width of panel
	min_width = (int) xv_get(objs->PlayctlCanvas, XV_WIDTH);

	// Do an inital layout based on balance being displayed
	resize();
}

// Destructor
Playctl_xv::
~Playctl_xv()
{
	// XXX - anything go here??
}

// Return panel handle
ptr_t Playctl_xv::
Gethandle()
{
	return ((ptr_t)objs->PlayctlPanel);
}

// set positioned flag (return old value)
int Playctl_xv::
Setpositioned(int flag)
{
	int oldval = positioned;
	
	positioned = flag;
	return (oldval);
}

// Display/open the panel
void Playctl_xv::
Show()
{
	// Make sure the panel info is current
	Update();

	if ((iconic == TRUE) || (displayed == FALSE)) {
		// Catch SIGPOLL to detect play control changes
		notify_set_signal_func(objs->PlayctlPanel,
		    (Notify_func) sigpoll_active_handler, SIGPOLL, NOTIFY_SYNC);

		xv_set(objs->PlayctlPanel, FRAME_CLOSED, FALSE, NULL);
		iconic = FALSE;
		displayed = TRUE;

		if ((parent != NULL) && (positioned == FALSE)) {
			ds_position_popup((Frame)parent,
			    (Frame)objs->PlayctlPanel, DS_POPUP_LOR);
			positioned = TRUE;
		}
	}
	xv_set(objs->PlayctlPanel, XV_SHOW, TRUE, NULL);
}

// Dismiss the Play Control Panel
void Playctl_xv::
Unshow()
{
	if (displayed = FALSE) {
		return;
	}

	// Init state so next Update() will set everything
	info.Clear();

	displayed = FALSE;

	xv_set(objs->PlayctlPanel, XV_SHOW, FALSE, NULL);

	// Quit catching SIGPOLL
	notify_set_signal_func(objs->PlayctlPanel, 
			       (Notify_func) sigpoll_inactive_handler,
			       SIGPOLL, NOTIFY_SYNC);
}

// Dismiss the Play Control Panel
void Playctl_xv::
Iconify()
{
	if (iconic || (displayed == FALSE))
		return;

	// Init state so next Update() will set everything
	info.Clear();

	xv_set(objs->PlayctlPanel, FRAME_CLOSED, TRUE, NULL);
	iconic = TRUE;

	// Quit catching SIGPOLL
	notify_set_signal_func(objs->PlayctlPanel, 
			       (Notify_func) sigpoll_inactive_handler,
			       SIGPOLL, NOTIFY_SYNC);
}

// Enable/disable the balance control
void Playctl_xv::
setbalance(
	Boolean			enable)
{
	Boolean			change;

	// Call the enable/disable routines only if necessary
	if (enable && !balance) {
		create_balance();
		change = TRUE;
	} else if (!enable && balance) {
		destroy_balance();
		change = TRUE;
	}
	balance = enable;
	if (change)
		resize();
}


// Destroy a control and clear out its pointer
void Playctl_xv::
destroy_object(
	Xv_opaque&		obj)
{
	xv_destroy_safe(obj);
	obj = NULL;
}

// Destroy balance control
void Playctl_xv::
destroy_balance()
{
	destroy_object(objs->PlayBalanceGroup);
	destroy_object(objs->PlayBalanceSliderGroup);
	destroy_object(objs->PlayBalanceLeftSliderGroup);
	destroy_object(objs->PlayBalanceMsg);
	destroy_object(objs->PlayBalanceLeftMsg);
	destroy_object(objs->PlayBalanceRightMsg);
	destroy_object(objs->PlayBalanceSlider);
}

// Restore balance control
void Playctl_xv::
create_balance()
{
	objs->PlayBalanceMsg =
	    objs->PlayBalanceMsg_create(objs->PlayctlCanvas);
	objs->PlayBalanceLeftMsg =
	    objs->PlayBalanceLeftMsg_create(objs->PlayctlCanvas);
	objs->PlayBalanceSlider =
	    objs->PlayBalanceSlider_create(objs->PlayctlCanvas);
	objs->PlayBalanceLeftSliderGroup =
	    objs->PlayBalanceLeftSliderGroup_create(objs->PlayctlCanvas);
	objs->PlayBalanceRightMsg =
	    objs->PlayBalanceRightMsg_create(objs->PlayctlCanvas);
	objs->PlayBalanceSliderGroup =
	    objs->PlayBalanceSliderGroup_create(objs->PlayctlCanvas);
	objs->PlayBalanceGroup =
	    objs->PlayBalanceGroup_create(objs->PlayctlCanvas);
	xv_set(objs->PlayBalanceSlider,
	    PANEL_MAX_VALUE, max_balance + BALANCE_MIDPTS,
	    PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
}

// Adjust panel width
void Playctl_xv::
Setwidth(
	int		w)
{
	// Adjust wider, only within reason
	if ((w > min_width) && ((w - min_width) < 20)) {
		min_width = w;
		resize();
	}
}

// Re-layout the panel based on current sizes, etc
void Playctl_xv::
resize()
{
	unsigned long	max_right;
	unsigned long	max_width;
	unsigned long	bottom;
	unsigned long	i;

	// Move the messages to the left edge to start out
	i = xv_get(objs->PlayOutputGroup, XV_X) + 4;
	xv_set(objs->PlaySliderGroup, XV_X, i, NULL);
	if (balance)
		xv_set(objs->PlayBalanceGroup, XV_X, i, NULL);

	// Calculate the rightmost & bottom point of ports & sliders
	max_right = xv_get(objs->PlayOutputGroup, XV_X) +
	    xv_get(objs->PlayOutputGroup, XV_WIDTH) + 18;

	bottom = xv_get(objs->PlaySliderGroup, XV_Y) +
	    xv_get(objs->PlaySliderGroup, XV_HEIGHT);

	if (balance) {
		unsigned long	rt;
		unsigned long	balx;

		bottom = xv_get(objs->PlayBalanceGroup, XV_Y) +
		    xv_get(objs->PlayBalanceGroup, XV_HEIGHT);

		// Line up the slider labels to the colons
		rt = xv_get(objs->PlayVolumeMsg, XV_X) +
		    xv_get(objs->PlayVolumeMsg, XV_WIDTH);
		i = xv_get(objs->PlayBalanceMsg, XV_X) +
		    xv_get(objs->PlayBalanceMsg, XV_WIDTH);
		if (i > rt)
			rt = i;
		xv_set(objs->PlaySliderGroup, XV_X,
		    rt - xv_get(objs->PlayVolumeMsg, XV_WIDTH),
		    GROUP_LAYOUT, TRUE, NULL);
		balx = rt - xv_get(objs->PlayBalanceMsg, XV_WIDTH);
		xv_set(objs->PlayBalanceGroup, XV_X, balx,
		    GROUP_LAYOUT, TRUE, NULL);

		// Line up the sliders to the endpoints
		rt = xv_get(objs->PlayVolumeSlider, XV_X) +
		    xv_get(objs->PlayVolumeSlider, XV_WIDTH);
		i = xv_get(objs->PlayBalanceLeftSliderGroup, XV_X) +
		    xv_get(objs->PlayBalanceLeftSliderGroup, XV_WIDTH);
		if (i > rt)
			rt = i;
		xv_set(objs->PlayVolumeSlider, XV_X,
		    rt - xv_get(objs->PlayVolumeSlider, XV_WIDTH), NULL);
		xv_set(objs->PlayBalanceSliderGroup, XV_X,
		    rt - xv_get(objs->PlayBalanceLeftSliderGroup, XV_WIDTH),
		    GROUP_LAYOUT, TRUE, NULL);

		// Get the rightmost point of the balance control
		i = xv_get(objs->PlayBalanceSliderGroup, XV_X) +
		    xv_get(objs->PlayBalanceSliderGroup, XV_WIDTH);
		if (i > max_right)
			max_right = i;

		// Line up the balance messages
		i = xv_get(objs->PlayBalanceMsg, XV_Y) +
		    xv_get(objs->PlayBalanceMsg, XV_HEIGHT);
		rt = i - xv_get(objs->PlayBalanceRightMsg, XV_HEIGHT);
		xv_set(objs->PlayBalanceRightMsg, XV_Y, rt, NULL);
		xv_set(objs->PlayBalanceLeftMsg, XV_Y, rt, NULL);

		// Restore label position in case group code moved it
		xv_set(objs->PlayBalanceMsg, XV_X, balx, NULL);
	}

	// Get the rightmost point of the play volume control
	i = xv_get(objs->PlaySliderGroup, XV_X) +
	    xv_get(objs->PlaySliderGroup, XV_WIDTH) + 5;
	if (i > max_right)
		max_right = i;

	// Figure out which button is wider
	max_width = xv_get(objs->RecpanelButton, XV_WIDTH);
	i = xv_get(objs->MuteButton, XV_WIDTH);
	if (i > max_width)
		max_width = i;

	// Calculate rightmost point for buttons, obeying minimum constraint
	max_right += max_width;
	i = xv_get(objs->PlayOutputGroup, XV_X);
	if (max_right < ((unsigned long) min_width - i)) {
		max_right = (unsigned long) min_width - i;
	}

	// Now set both buttons to line up on the right
	xv_set(objs->RecpanelButton,
	    XV_X, max_right - xv_get(objs->RecpanelButton, XV_WIDTH), NULL);
	xv_set(objs->MuteButton,
	    XV_X, max_right - xv_get(objs->MuteButton, XV_WIDTH), NULL);

	// Resize panel and frame to fit, save new size as future minimum width
	max_right += i;
	min_width = (int) max_right;
	xv_set(objs->PlayctlCanvas,
	    XV_WIDTH, max_right,
	    XV_HEIGHT, bottom + xv_get(objs->PlayOutputGroup, XV_Y),
	    NULL);
	xv_set(objs->PlayctlPanel,
	    XV_WIDTH, xv_get(objs->PlayctlCanvas, XV_WIDTH) + 2,
	    XV_HEIGHT, xv_get(objs->PlayctlCanvas, XV_HEIGHT) + 2,
	    NULL);
}


// Set output switch control
void Playctl_xv::
setoutports(
	DevicePort*	ports)
{
	unsigned long	rt;
	unsigned long	i;
	unsigned long	j;
	unsigned long	maxw = 0;

	// Detect rightmost point to see if a resize is necessary
	rt = xv_get(objs->PlayOutputGroup, XV_X) +
	    xv_get(objs->PlayOutputGroup, XV_WIDTH);

	// Set output strings and port values
	for (i = 0; i < MAX_OUTPUTS; i++) {
		xv_set(outports[i], PANEL_CHOICE_STRING, 0,
		    ports[i].name == NULL ? " " : ports[i].name,
		    PANEL_CLIENT_DATA, ports[i].port,
		    XV_SHOW, (ports[i].name == NULL ? FALSE : TRUE), NULL);
		if (ports[i].name != NULL) {
			j = xv_get(outports[i], XV_WIDTH);
			if (j > maxw)
				maxw = j;
		}
	}

	// Resize all buttons to be the same size
	for (i = 0; i < MAX_OUTPUTS; i++) {
		// XXX - setting XV_WIDTH *should* work!
		char*	oldname;
		char*	newname;

		while ((ports[i].name != NULL) &&
		    (xv_get(outports[i], XV_WIDTH) < (maxw - 8))) {
			oldname = (char*)
			    xv_get(outports[i], PANEL_CHOICE_STRING, 0);
			newname = new char[strlen(oldname) + 3];
			SPRINTF(newname, " %s ", oldname);
			xv_set(outports[i],
			    PANEL_CHOICE_STRING, 0, newname, NULL);
			delete newname;
		}
	}
	if (output_exclusive()) {
		xv_set(objs->PlayOutputGroup,
		    GROUP_HORIZONTAL_SPACING, 0, NULL);
	} else {
		xv_set(objs->PlayOutputGroup,
		    GROUP_HORIZONTAL_SPACING, 8, NULL);
	}

	// Check to see if things moved
	i = xv_get(objs->PlayOutputGroup, XV_X) +
	    xv_get(objs->PlayOutputGroup, XV_WIDTH);
	if (i != rt)
		resize();
}

// Convert device gain into the local scaling factor
int Playctl_xv::
unscale_gain(
	unsigned int	g)
{
	return (irint(max_gain *
	    (((double)(g - AUDIO_MIN_GAIN)) /
	    (double)(AUDIO_MAX_GAIN - AUDIO_MIN_GAIN))));
}

// Update the Play Control Panel according to the given values
void Playctl_xv::
update_display(
	AudioInfo&	nip)
{
	int		i;
	unsigned	p;

// XXX - Assumes audio state init value of ~0 is correct
#define	UC_INIT			((unsigned char)(~0))
#define	SET_VAL(I, X)		if ((unsigned)				\
				    xv_get(objs->I, PANEL_VALUE) !=	\
				    (unsigned) (X)) {			\
					xv_set(objs->I, PANEL_VALUE, X, 0); \
				}
#define	NEWVAL(I, X)		if (nip->X != UC_INIT) {		\
				    SET_VAL(I, nip->X);			\
				}

	// Update sliders
	if (nip->play.gain != ~0) {
		last_volume = unscale_gain(nip->play.gain);
		SET_VAL(PlayVolumeSlider, last_volume);
	}

	// Recalculate balance to compensate for center detent
	// XXX - balance slider range must match device range
	if (balance && (nip->play.balance != UC_INIT)) {
		last_balance = nip->play.balance;
		if (last_balance == (max_balance / 2)) {
			last_balance += BALANCE_MIDPTS / 2;
		} else if (last_balance > (max_balance / 2)) {
			last_balance += BALANCE_MIDPTS;
		}
		SET_VAL(PlayBalanceSlider, last_balance);
	}

	// Update mute button
	NEWVAL(MuteButton, output_muted);

	// Update output ports
	if (nip->play.port != ~0) {
		for (i = 0; i < MAX_OUTPUTS; i++) {
			p = (unsigned)xv_get(outports[i], PANEL_CLIENT_DATA);
			xv_set(outports[i],
			    PANEL_CHOOSE_ONE, (output_exclusive() ?
			    ((p & nip->play.port) ? TRUE : FALSE) : FALSE),
			    PANEL_VALUE, (p & nip->play.port) ? 1 : 0, NULL);
		}
	}
	// Update available output ports
	if (nip->play.avail_ports != ~0) {
		for (i = 0; i < MAX_OUTPUTS; i++) {
			p = (unsigned)xv_get(outports[i], PANEL_CLIENT_DATA);
			xv_set(outports[i], PANEL_INACTIVE,
			    (p & nip->play.avail_ports) ? FALSE : TRUE, NULL);
		}
	}
}


// XView callbacks, adapted from playctl_stubs.cc


// Event callback function for `PlayctlPanel'.
Notify_value
playctl_PlayctlPanel_event_callback(
	Xv_window		win,
	Event*			event,
	Notify_arg		arg,
	Notify_event_type	type)
{
	Playctl_xv*		xvp;

	switch (event_action(event)) {
	case WIN_RESIZE:
	case ACTION_OPEN:
	case ACTION_CLOSE:
		xvp = (Playctl_xv*) xv_get(win, XV_KEY_DATA, PLAYCTLKEY);
		switch (event_action(event)) {
		case WIN_RESIZE:
			break;
		case ACTION_OPEN:
			xvp->Show();
			break;
		case ACTION_CLOSE:
			xvp->Iconify();
			break;
		}
		break;
	}
	return (notify_next_event_func(win, (Notify_event) event, arg, type));
}

// Convert local gain to double value 0.-1.
static double
scale_gain(
	int		g,
	double		max)
{
	return ((double)g / max);
}

// Convert local balance to double value -1. to 1.
static double
scale_balance(
	int		g,
	double		max)
{
	return (((double)(2 * g) / max) - 1.);
}

// Notify callback function for `PlayVolumeSlider'.
void
Playctl_volume_proc(
	Panel_item			item,
	int				value,
	Event*				event)
{
	playctl_PlayctlPanel_objects*	ip;
	Playctl_xv*			xvp;

	ip = (playctl_PlayctlPanel_objects*)xv_get(item, XV_KEY_DATA, INSTANCE);
	xvp = (Playctl_xv*) xv_get(ip->PlayctlPanel, XV_KEY_DATA, PLAYCTLKEY);

	// If no change, don't sweat it
	if (value == xvp->last_volume)
		return;

	// Check for a single click increment/decrement
	if (event_is_up(event)) {
		if ((value - xvp->last_volume) == 1) {
			xvp->VolumeIncr(TRUE);			// increment
		} else if ((xvp->last_volume - value) == 1) {
			xvp->VolumeIncr(FALSE);			// decrement
		} else {
			xvp->SetVolume(scale_gain(value, xvp->max_gain));
		}
	} else {
		xvp->SetVolume(scale_gain(value, xvp->max_gain));
	}
}

// Notify callback function for `PlayBalanceSlider'.
void
Playctl_balance_proc(
	Panel_item			item,
	int				value,
	Event*				event)
{
	playctl_PlayctlPanel_objects*	ip;
	Playctl_xv*			xvp;
	unsigned int			mid;
	int				delta;

#define	nearmid(X)	(((X) >= (xvp->max_balance / 2)) &&		\
			 ((X) <= ((xvp->max_balance / 2) + BALANCE_MIDPTS)))

	ip = (playctl_PlayctlPanel_objects*)xv_get(item, XV_KEY_DATA, INSTANCE);
	xvp = (Playctl_xv*) xv_get(ip->PlayctlPanel, XV_KEY_DATA, PLAYCTLKEY);

	// Implement center detent
	mid = (xvp->max_balance + BALANCE_MIDPTS) / 2;
	if (event_is_up(event) && !xvp->dragbalance) {
		// Single click
		if (nearmid(value)) {
			if (xvp->last_balance != mid)
				value = mid;
			value = (xvp->max_balance / 2) +
			    ((value < mid) ? -1 : ((value > mid) ? 1 : 0));

		} else if (value > ((xvp->max_balance / 2) + BALANCE_MIDPTS)) {
			// Value is beyond center detent, so adjust it down
			value -= BALANCE_MIDPTS;
		}
	} else if (!event_is_up(event) && !xvp->dragbalance) {
		// Start drag
		xvp->dragbalance = TRUE;
		xvp->dragvalue = xvp->last_balance;
	}

	if (xvp->dragbalance) {
		if (!event_is_up(event)) {
			// Continue drag
			delta = value - (int) xvp->last_balance;
			value = xvp->dragvalue + delta;
			xvp->dragvalue = value;
		} else {
			// Finish drag
			xvp->dragbalance = FALSE;
			value = xvp->dragvalue;
		}
		// Correct the new value for the device scale
		if (nearmid(value)) {
			value = xvp->max_balance / 2;
		} else if (value > ((xvp->max_balance / 2) + BALANCE_MIDPTS)) {
			// Value is beyond center detent, so adjust it down
			value -= BALANCE_MIDPTS;
		}
	}

	xvp->SetBalance(scale_balance(value, (double)xvp->max_balance));
}

// Notify callback function for `MuteButton'.
void
Playctl_mute_proc(
	Panel_item			item,
	unsigned int			value,
	Event*)
{
	playctl_PlayctlPanel_objects*	ip;
	Playctl_xv*			xvp;

	ip = (playctl_PlayctlPanel_objects*)xv_get(item, XV_KEY_DATA, INSTANCE);
	xvp = (Playctl_xv*) xv_get(ip->PlayctlPanel, XV_KEY_DATA, PLAYCTLKEY);
	xvp->SetMute(value != 0);
}

// Notify callback function for Outport switches
void
Playctl_outport_proc(
	Panel_item			item,
	unsigned int			value,
	Event*)
{
	playctl_PlayctlPanel_objects*	ip;
	Playctl_xv*			xvp;
	unsigned			port;

	ip = (playctl_PlayctlPanel_objects*)xv_get(item, XV_KEY_DATA, INSTANCE);
	xvp = (Playctl_xv*) xv_get(ip->PlayctlPanel, XV_KEY_DATA, PLAYCTLKEY);
	port = (unsigned) xv_get(item, PANEL_CLIENT_DATA);
	xvp->SetPort(port, (value != 0));
}

// Notify callback function for `RecpanelButton'.
void
playctl_PlayctlPanel_RecpanelButton_notify_callback(
	Panel_item,
	Event*)
{
	Audioctl_show_recpanel();
}

// Menu handler for `PlayctlPanel_menu (Record...)'.
Menu_item
playctl_PlayctlPanel_menu_item0_callback(
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

// Menu handler for `PlayctlPanel_menu (Status...)'.
Menu_item
playctl_PlayctlPanel_menu_item1_callback(
	Menu_item	item,
	Menu_generate	op)
{
	switch (op) {
	case MENU_NOTIFY:
		Audioctl_show_statuspanel();
		break;
	}
	return (item);
}
