/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)recordctl_xv.cc	1.26	97/01/14 SMI"

#include <stdio.h>
#include <math.h>
// if defined(__ppc)
#include <sunmath.h>
// endif
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/cms.h>
#include <group.h>

#include "meter/meter.h"
#include "ds_popup.h"
#include "ds_colors.h"
#include "recordctl_xv.h"


Attr_attribute	RECORDCTLKEY;		// Global key for local storage

// Number of extra balance values for a center detent (must be an even number)
#define	BALANCE_MIDPTS	(8)


// Create and return an XView Record Control Panel
Recordctl*
Create_Recordctl(
	char*			name,
	ptr_t			parent)
{
	Recordctl_xv*		xvp;

	xvp = new Recordctl_xv(parent);
	if (xvp->Setdevice(name) < 0) {
		delete xvp;
		xvp = NULL;
	}
	return ((Recordctl*) xvp);
}


// Recordctl_xv implementation


// Entered any time there is a change of state of the audio device.
// The Notifier schedules this routine synchronously, so we don't
// have to worry about stomping on global data structures.
static Notify_value
sigpoll_handler(
	Notify_client		client,
	int,
	Notify_signal_mode)
{
	Recordctl_xv*		xvp;

	xvp = (Recordctl_xv*) xv_get(client, XV_KEY_DATA, RECORDCTLKEY);
	xvp->Update();
	return (NOTIFY_DONE);
}

// Timer handler is used to drive animation during Auto-Adjust
static Notify_value
timer_handler(
	Notify_client		client,
	int)
{
	Recordctl_xv*		xvp;
	struct itimerval	timer;

	xvp = (Recordctl_xv*) xv_get(client, XV_KEY_DATA, RECORDCTLKEY);
	xvp->Update();

	// If still adjusting volume, reschedule the timer
	if (xvp->adjusting) {
		timer.it_value.tv_usec = (1000000 / xvp->update_frequency);
		timer.it_value.tv_sec = 0;
		timer.it_interval.tv_usec = 0;
		timer.it_interval.tv_sec = 0;
		(void) notify_set_itimer_func((Xv_opaque)client,
		    (Notify_func) timer_handler, ITIMER_REAL, &timer, NULL);
	} else {
		(void) notify_set_itimer_func((Xv_opaque)client,
		    NOTIFY_FUNC_NULL, ITIMER_REAL, NULL, NULL);
	}
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
	Recordctl_xv*		xvp;

	xvp = (Recordctl_xv*) xv_get(frame, XV_KEY_DATA, RECORDCTLKEY);
	if ((event_action(event) == ACTION_PROPS) && event_is_up(event)) {
		Audioctl_show_statuspanel();
		return (NOTIFY_DONE);
	}
	if ((event_action(event) == ACTION_MENU) && event_is_down(event)) {
		menu_show(xvp->menu, frame, event, 0);
		return (NOTIFY_DONE);
	}
	if (event_action(event) == ACTION_STOP) {
		// Stop adjusting.  Don't take down the frame if unpinned.
		xvp->StopAdjust(FALSE);
	}
	return (notify_next_event_func(frame, (Notify_event)event, arg, type));
}


// Constructor
Recordctl_xv::
Recordctl_xv(
	ptr_t			p):
	Recordctl(p)
{
	Xv_opaque		vumeter;
	int			steps;
	Cms			cms;
	Xv_singlecolor		color;
	unsigned long		green;
	unsigned long		red;
	unsigned long		i;
	unsigned long		j;
	unsigned long		k;

	// Init global keys, if necessary
	if (INSTANCE == 0)
		INSTANCE = xv_unique_key();
	if (RECORDCTLKEY == 0)
		RECORDCTLKEY = xv_unique_key();

	// Initialize XView record control panel
	// Save the address of the container class
	objs = new recordctl_RecordctlPanel_objects;
	objs->objects_initialize((Xv_opaque) parent);
	xv_set(objs->RecordctlPanel, XV_KEY_DATA, RECORDCTLKEY, this, NULL);
	xv_set(objs->RecordctlCanvas, XV_KEY_DATA, RECORDCTLKEY, this, NULL);
	xv_set(objs->MeterLowCanvas, XV_KEY_DATA, RECORDCTLKEY, this, NULL);
	xv_set(objs->MeterCanvas, XV_KEY_DATA, RECORDCTLKEY, this, NULL);
	xv_set(objs->MeterHighCanvas, XV_KEY_DATA, RECORDCTLKEY, this, NULL);

	// XXX - Workaround for Bug 1122838
	// Unmap the overlapping canvasses so XView doesn't get confused
	xv_set(objs->MeterLowCanvas, XV_SHOW, FALSE, NULL);
	xv_set(objs->MeterCanvas, XV_SHOW, FALSE, NULL);
	xv_set(objs->MeterHighCanvas, XV_SHOW, FALSE, NULL);
	window_fit(objs->RecordctlPanel);

	displayed = FALSE;
	positioned = FALSE;
	balance = TRUE;

	// Set the sliders to be fully interactive
	xv_set(objs->RecordVolumeSlider, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
	xv_set(objs->RecordBalanceSlider, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
	xv_set(objs->MonitorVolumeSlider, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);

	// Get the maximum gain from the interface
	max_gain = (double)((unsigned)
	    xv_get(objs->RecordVolumeSlider, PANEL_MAX_VALUE));

	// Get the maximum balance, then add some pixels for a center detent
	max_balance = (unsigned)
	    xv_get(objs->RecordBalanceSlider, PANEL_MAX_VALUE);
	xv_set(objs->RecordBalanceSlider,
	    PANEL_MAX_VALUE, max_balance + BALANCE_MIDPTS, NULL);

	// Save the handles to all the inport items
	inports[0] = objs->Inport0;
	inports[1] = objs->Inport1;
	inports[2] = objs->Inport2;

	// Initialize floating menu and set notify interposer for each canvas
	menu = recordctl_RecordctlPanel_menu_create((caddr_t)objs,
	    objs->RecordctlPanel);
	notify_interpose_event_func(objs->RecordctlCanvas,
	    (Notify_func) event_proc, NOTIFY_SAFE);
	notify_interpose_event_func(objs->MeterLowCanvas,
	    (Notify_func) event_proc, NOTIFY_SAFE);
	notify_interpose_event_func(objs->MeterCanvas,
	    (Notify_func) event_proc, NOTIFY_SAFE);
	notify_interpose_event_func(objs->MeterHighCanvas,
	    (Notify_func) event_proc, NOTIFY_SAFE);

	// XXX - workaround Guide bug
	xv_set(menu, MENU_TITLE_ITEM, MGET("Audio Control"), NULL);

	// Resize the meter message canvases
	j = xv_get(objs->LowsignalMsg, XV_WIDTH);
	i = xv_get(objs->NosignalMsg, XV_WIDTH);
	if (i > j)
		j = i;
	i = xv_get(objs->PeakMsg, XV_WIDTH);
	if (i > j)
		j = i;
	k = xv_get(objs->MeterCanvas, XV_HEIGHT);
	i = xv_get(objs->PeakMsg, XV_HEIGHT) + 10;
	if (i > k)
		k = i;

	xv_set(objs->MeterCanvas, XV_WIDTH, 9 * k / 2, XV_HEIGHT, k, NULL);
	xv_set(objs->MeterLowCanvas, XV_WIDTH, j + 10, XV_HEIGHT, k, NULL);
	xv_set(objs->MeterHighCanvas, XV_WIDTH, j + 10, XV_HEIGHT, k, NULL);

	// Position and disappear the auto-adjust messages
	i = (xv_get(objs->MeterCanvas, XV_HEIGHT) -
	    xv_get(objs->PeakMsg, XV_HEIGHT)) / 2;
	xv_set(objs->PeakMsg, XV_X, 5, XV_Y, i, PANEL_SHOW_ITEM, FALSE, NULL);
	xv_set(objs->LowsignalMsg,
	    XV_X, j - xv_get(objs->LowsignalMsg, XV_WIDTH) - 5,
	    XV_Y, i, PANEL_SHOW_ITEM, FALSE, NULL);
	xv_set(objs->NosignalMsg,
	    XV_X, j - xv_get(objs->NosignalMsg, XV_WIDTH) - 5,
	    XV_Y, i, PANEL_SHOW_ITEM, FALSE, NULL);

	// Initialize the level meter
	steps = (int)xv_get(objs->MeterGauge, PANEL_MAX_VALUE);

	// Get the color indeces
	cms = ds_cms_create(objs->RecordctlPanel);
	if (cms != NULL) {
		color.red = 0;
		color.green = 255;
		color.blue = 0;
		green = ds_cms_index(cms, &color);
		color.red = 255;
		color.green = 0;
		red = ds_cms_index(cms, &color);

		xv_set(objs->MeterCanvas, WIN_CMS, cms, NULL);
		xv_set(objs->MeterLowCanvas, WIN_CMS, cms, NULL);
		xv_set(objs->MeterHighCanvas, WIN_CMS, cms, NULL);
		ds_set_colormap(objs->RecordctlPanel, cms,
		    DS_NULL_CMS_INDEX, DS_NULL_CMS_INDEX);
	}

	vumeter = (Xv_opaque) xv_create(objs->MeterCanvas, PANEL_METER_ITEM,
	    XV_KEY_DATA, INSTANCE, objs,
	    XV_HELP_DATA, (char*)xv_get(objs->MeterGauge, XV_HELP_DATA),
	    PANEL_DIRECTION, (int)xv_get(objs->MeterGauge, PANEL_DIRECTION),
	    PANEL_METER_WIDTH, (int)xv_get(objs->MeterCanvas, XV_HEIGHT) - 4,
	    PANEL_METER_HEIGHT, (int)xv_get(objs->MeterCanvas, XV_WIDTH) - 4,
	    PANEL_METER_LEDS, steps,
	    NULL);

	if (cms != NULL) {
		xv_set(vumeter,
		    PANEL_METER_LED_COLOR, green,
		    PANEL_METER_OVERLD_COLOR, red,
		    NULL);
		xv_set(objs->PeakMsg,
		    PANEL_ITEM_COLOR, red, PANEL_SHOW_ITEM, FALSE, NULL);
	}

	xv_set(objs->MeterGroup,
	    GROUP_MEMBERS, vumeter, NULL,
	    GROUP_ANCHOR_OBJ, objs->MeterCanvas,
	    GROUP_ANCHOR_POINT, GROUP_CENTER,
	    GROUP_REFERENCE_POINT, GROUP_CENTER,
	    GROUP_HORIZONTAL_OFFSET, 0,
	    GROUP_VERTICAL_OFFSET, 0,
	    GROUP_LAYOUT, TRUE,
	    NULL);

	destroy_object(objs->MeterGauge);
	objs->MeterGauge = vumeter;
	meter_steps = (double) ((int)xv_get(vumeter, PANEL_MAX_VALUE));
	panel_paint(objs->MeterCanvas, PANEL_NO_CLEAR);

	// Save minimum width of panel
	min_width = (int) xv_get(objs->RecordctlCanvas, XV_WIDTH);

	// Do an inital layout based on balance being displayed
	resize();
}

// Destructor
Recordctl_xv::
~Recordctl_xv()
{
	// XXX - anything go here??
}

// set positioned flag (return old value)
int Recordctl_xv::
Setpositioned(int flag)
{
	int oldval = positioned;
	
	positioned = flag;
	return (oldval);
}

// Return panel handle
ptr_t Recordctl_xv::
Gethandle()
{
	return ((ptr_t)objs->RecordctlPanel);
}

// Display/open the panel
void Recordctl_xv::
Show()
{
	// Make sure the panel info is current
	Update();

	if (!displayed) {
		// Catch SIGPOLL to detect record control changes
		notify_set_signal_func(objs->RecordctlPanel,
		    (Notify_func) sigpoll_handler, SIGPOLL, NOTIFY_SYNC);
		displayed = TRUE;

		if ((parent != NULL) && (positioned == FALSE)) {
			ds_position_popup((Frame)parent,
			    (Frame)objs->RecordctlPanel, DS_POPUP_AOB);
			positioned = TRUE;
		}
	}

	// XXX - Workaround for Bug 1122838
	// Map the overlapping canvasses
	xv_set(objs->MeterLowCanvas, XV_SHOW, TRUE, NULL);
	xv_set(objs->MeterCanvas, XV_SHOW, TRUE, NULL);
	xv_set(objs->MeterHighCanvas, XV_SHOW, TRUE, NULL);

	xv_set(objs->RecordctlPanel, XV_SHOW, TRUE, NULL);
}

// Done callback function for `RecordctlPanel'.
void
Recordctl_done_proc(
	Frame			frame)
{
	Recordctl_xv*			xvp;

	// Unmap the frame (it better not be pinned!)
	xv_set(frame, XV_SHOW, FALSE, NULL);

	// Cleanup
	xvp = (Recordctl_xv*) xv_get(frame, XV_KEY_DATA, RECORDCTLKEY);
	xvp->Unshow(0);
}

// Cleanup on takedown of the Record Control Panel
void Recordctl_xv::
Unshow(int)
{
	if (!displayed)
		return;
	displayed = FALSE;

	if (adjusting)
		StopAdjust();

	// Init state so next Update() will set everything
	info.Clear();

	// Quit catching SIGPOLL
	notify_set_signal_func(objs->RecordctlPanel, NOTIFY_FUNC_NULL, SIGPOLL,
	    NOTIFY_SYNC);

	// If the main panel is not up, the program may exit
	Audioctl_recpanel_exit();
}


// Force a dismissal of the Record Control Panel
void Recordctl_xv::
Unshow()
{
	if (!displayed)
		return;

	// This will take the frame down if it is pinned
	xv_set(objs->RecordctlPanel, 
	       FRAME_CMD_PIN_STATE, FRAME_CMD_PIN_OUT, 
	       XV_SHOW, FALSE,
	       NULL);

	// Finish cleaning up
	Unshow(0);
}

// Enable/disable the balance control
void Recordctl_xv::
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
void Recordctl_xv::
destroy_object(
	Xv_opaque&		obj)
{
	xv_set(obj, XV_SHOW, FALSE, NULL);
	xv_destroy_safe(obj);
	obj = NULL;
}

// Destroy balance control
void Recordctl_xv::
destroy_balance()
{
	destroy_object(objs->RecordBalanceGroup);
	destroy_object(objs->RecordBalanceSliderGroup);
	destroy_object(objs->RecordBalanceLeftSliderGroup);
	destroy_object(objs->RecordBalanceMsg);
	destroy_object(objs->RecordBalanceLeftMsg);
	destroy_object(objs->RecordBalanceRightMsg);
	destroy_object(objs->RecordBalanceSlider);
}

// Restore balance control
void Recordctl_xv::
create_balance()
{
	objs->RecordBalanceMsg =
	    objs->RecordBalanceMsg_create(objs->RecordctlCanvas);
	objs->RecordBalanceLeftMsg =
	    objs->RecordBalanceLeftMsg_create(objs->RecordctlCanvas);
	objs->RecordBalanceSlider =
	    objs->RecordBalanceSlider_create(objs->RecordctlCanvas);
	objs->RecordBalanceLeftSliderGroup =
	    objs->RecordBalanceLeftSliderGroup_create(objs->RecordctlCanvas);
	objs->RecordBalanceRightMsg =
	    objs->RecordBalanceRightMsg_create(objs->RecordctlCanvas);
	objs->RecordBalanceSliderGroup =
	    objs->RecordBalanceSliderGroup_create(objs->RecordctlCanvas);
	objs->RecordBalanceGroup =
	    objs->RecordBalanceGroup_create(objs->RecordctlCanvas);
	xv_set(objs->RecordBalanceSlider,
	    PANEL_MAX_VALUE, max_balance + BALANCE_MIDPTS,
	    PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
}

// Adjust panel width
void Recordctl_xv::
Setwidth(
	int		w)
{
	// Adjust wider
	if (w > min_width) {
		min_width = w;
		resize();
	}
}

// Re-layout the panel based on current sizes, etc
void Recordctl_xv::
resize()
{
	unsigned long	max_right;
	unsigned long	bottom;
	unsigned long	i;
	unsigned long	rt;
	unsigned long	balx;

	// Re-init the messages
	SetAdjustmsg(
	    MGET("Press the Auto-Adjust button to set the record volume"));
	SetErrormsg();

	// Adjust the Y positions of the top items
	i = xv_get(objs->RecordInputGroup, XV_Y);
	bottom = (2 * i) + xv_get(objs->RecordInputGroup, XV_HEIGHT);
	xv_set(objs->AutoAdjustMsgGroup,
	    XV_Y, bottom, GROUP_VERTICAL_OFFSET, bottom, NULL);
	bottom += i + xv_get(objs->AutoAdjustMsgGroup, XV_HEIGHT);
	xv_set(objs->MeterLowCanvas, XV_Y, bottom, NULL);
	xv_set(objs->MeterCanvas, XV_Y, bottom, NULL);
	xv_set(objs->MeterHighCanvas, XV_Y, bottom, NULL);
	bottom += i + xv_get(objs->MeterCanvas, XV_HEIGHT);
	xv_set(objs->RecordSliderGroup, XV_Y, bottom, NULL);
	i /= 2;
	bottom += i + xv_get(objs->RecordSliderGroup, XV_HEIGHT);
	if (balance) {
		xv_set(objs->RecordBalanceGroup, XV_Y, bottom, NULL);
		bottom += i + xv_get(objs->RecordBalanceGroup, XV_HEIGHT);
	}
	xv_set(objs->MonitorSliderGroup, XV_Y, bottom, NULL);
	bottom += i + xv_get(objs->MonitorSliderGroup, XV_HEIGHT);

	// Move the messages to the left edge to start out
	i = xv_get(objs->RecordInputGroup, XV_X);
	xv_set(objs->RecordSliderGroup, XV_X, i, NULL);
	xv_set(objs->MonitorSliderGroup, XV_X, i, NULL);
	if (balance)
		xv_set(objs->RecordBalanceGroup, XV_X, i, NULL);

	// Calculate the rightmost point of all items
	max_right = xv_get(objs->AutoAdjustMsgGroup, XV_WIDTH) + (3 * i);

	rt = xv_get(objs->RecordInputGroup, XV_X) +
	    xv_get(objs->RecordInputGroup, XV_WIDTH) + (2 * i);
	if (rt > max_right)
		max_right = rt;

	// Line up the slider labels to the colons
	rt = xv_get(objs->RecordVolumeMsg, XV_X) +
	    xv_get(objs->RecordVolumeMsg, XV_WIDTH);
	i = xv_get(objs->RecordMonitorMsg, XV_X) +
	    xv_get(objs->RecordMonitorMsg, XV_WIDTH);
	if (i > rt)
		rt = i;
	if (balance) {
		i = xv_get(objs->RecordBalanceMsg, XV_X) +
		    xv_get(objs->RecordBalanceMsg, XV_WIDTH);
		if (i > rt)
			rt = i;
	}
	xv_set(objs->RecordSliderGroup, XV_X,
	    rt - xv_get(objs->RecordVolumeMsg, XV_WIDTH),
	    GROUP_LAYOUT, TRUE, NULL);
	xv_set(objs->MonitorSliderGroup, XV_X,
	    rt - xv_get(objs->RecordMonitorMsg, XV_WIDTH),
	    GROUP_LAYOUT, TRUE, NULL);
	if (balance) {
		balx = rt - xv_get(objs->RecordBalanceMsg, XV_WIDTH);
		xv_set(objs->RecordBalanceGroup, XV_X, balx,
		    GROUP_LAYOUT, TRUE, NULL);
	}

	// Line up the sliders to the endpoints
	rt = xv_get(objs->RecordVolumeSlider, XV_X) +
	    xv_get(objs->RecordVolumeSlider, XV_WIDTH);
	i = xv_get(objs->MonitorVolumeSlider, XV_X) +
	    xv_get(objs->MonitorVolumeSlider, XV_WIDTH);
	if (i > rt)
		rt = i;
	if (balance) {
		i = xv_get(objs->RecordBalanceLeftSliderGroup, XV_X) +
		    xv_get(objs->RecordBalanceLeftSliderGroup, XV_WIDTH);
		if (i > rt)
			rt = i;
		xv_set(objs->RecordBalanceSliderGroup, XV_X,
		    rt - xv_get(objs->RecordBalanceLeftSliderGroup, XV_WIDTH),
		    GROUP_LAYOUT, TRUE, NULL);
	}
	xv_set(objs->RecordVolumeSlider, XV_X,
	    rt - xv_get(objs->RecordVolumeSlider, XV_WIDTH), NULL);
	xv_set(objs->MonitorVolumeSlider, XV_X,
	    rt - xv_get(objs->MonitorVolumeSlider, XV_WIDTH), NULL);

	if (balance) {
		// Get the rightmost point of the balance control
		i = xv_get(objs->RecordBalanceSliderGroup, XV_X) +
		    xv_get(objs->RecordBalanceSliderGroup, XV_WIDTH) + 10;
		if (i > max_right)
			max_right = i;

		// Line up the balance messages
		i = xv_get(objs->RecordBalanceMsg, XV_Y) +
		    xv_get(objs->RecordBalanceMsg, XV_HEIGHT);
		rt = i - xv_get(objs->RecordBalanceRightMsg, XV_HEIGHT);
		xv_set(objs->RecordBalanceRightMsg, XV_Y, rt, NULL);
		xv_set(objs->RecordBalanceLeftMsg, XV_Y, rt, NULL);

		// Restore label position in case group code moved it
		xv_set(objs->RecordBalanceMsg, XV_X, balx, NULL);
	}

	// Get the rightmost point of the auto-adjust button
	i = (2 * xv_get(objs->RecordSliderGroup, XV_X)) +
	    xv_get(objs->RecordSliderGroup, XV_WIDTH) + 5 +
	    // fix for bugid 1129109
	    //xv_get(objs->AutoAdjustGroup, XV_WIDTH);
	    maxbuttonwidth ();
	if (i > max_right)
		max_right = i;

	// Calculate window width, obeying constraint, and save new minimum
	if (max_right < (unsigned long) min_width) {
		max_right = (unsigned long) min_width;
	}
	min_width = (int) max_right;

	// Now position the Auto-Adjust button properly
	rt = max_right - xv_get(objs->RecordSliderGroup, XV_X) -
	    // fix for bugid 1129109
	    // xv_get(objs->AutoAdjustGroup, XV_WIDTH);
	    // Moving over the entire width seems to cause problems
	    // in the 'sv' locale.  Let's just say that this is
	    // an 'empirically derived' number....
		(0.93 * maxbuttonwidth ());
	i = (xv_get(objs->RecordSliderGroup, XV_Y) +
	    (xv_get(objs->RecordSliderGroup, XV_HEIGHT) -
	    xv_get(objs->AutoAdjustGroup, XV_HEIGHT)) / 2);
	xv_set(objs->AutoAdjustGroup, XV_X, rt, XV_Y, i, NULL);

	// Resize panel and frame to fit
	xv_set(objs->RecordctlCanvas,
	    XV_WIDTH, max_right,
	    XV_HEIGHT, bottom,
	    NULL);
	xv_set(objs->RecordctlPanel,
	    XV_WIDTH, xv_get(objs->RecordctlCanvas, XV_WIDTH) + 2,
	    XV_HEIGHT, xv_get(objs->RecordctlCanvas, XV_HEIGHT) + 2,
	    NULL);

	// Adjust things that should be centered
	xv_set(objs->AutoAdjustMsgGroup, GROUP_LAYOUT, TRUE, NULL);
	rt = (max_right - xv_get(objs->MeterCanvas, XV_WIDTH)) / 2;
	xv_set(objs->MeterCanvas, XV_X, rt, NULL);
	xv_set(objs->MeterLowCanvas, XV_X, xv_get(objs->MeterCanvas, XV_X) -
	    xv_get(objs->MeterLowCanvas, XV_WIDTH), NULL);
	xv_set(objs->MeterHighCanvas, XV_X, xv_get(objs->MeterCanvas, XV_X) +
	    xv_get(objs->MeterCanvas, XV_WIDTH), NULL);
}


// fix for bugid 1129190:
// Calculate the maximum width of the button by 
// setting it to both states and checking the width.
int Recordctl_xv::
maxbuttonwidth()
{
	int	x;
	int	auto_button_width;

	auto_button_width = (int) xv_get(objs->AutoAdjustGroup, XV_WIDTH);
	adjusting = TRUE;

	setadjustbutton();
	x = (int) xv_get(objs->AutoAdjustGroup, XV_WIDTH);
	if (x > auto_button_width)
		auto_button_width = x;
	adjusting = FALSE;
	setadjustbutton();

	return auto_button_width;
}


// Set input switch control
void Recordctl_xv::
setinports(
	DevicePort*	ports)
{
	unsigned long	rt;
	unsigned long	i;
	unsigned long	j;
	unsigned long	maxw = 0;

	// Detect rightmost point to see if a resize is necessary
	rt = xv_get(objs->RecordInputGroup, XV_X) +
	    xv_get(objs->RecordInputGroup, XV_WIDTH);

	// Set input strings and port values
	for (i = 0; i < MAX_INPUTS; i++) {
		xv_set(inports[i], PANEL_CHOICE_STRING, 0,
		    ports[i].name == NULL ? " " : ports[i].name,
		    PANEL_CLIENT_DATA, ports[i].port,
		    XV_SHOW, (ports[i].name == NULL ? FALSE : TRUE), NULL);
		if (ports[i].name != NULL) {
			j = xv_get(inports[i], XV_WIDTH);
			if (j > maxw)
				maxw = j;
		}
	}
	// Resize all buttons to be the same size
	for (i = 0; i < MAX_INPUTS; i++) {
		// XXX - setting XV_WIDTH *should* work!
		char*	oldname;
		char*	newname;

		while ((ports[i].name != NULL) &&
		    (xv_get(inports[i], XV_WIDTH) < (maxw - 8))) {
			oldname = (char*)
			    xv_get(inports[i], PANEL_CHOICE_STRING, 0);
			newname = new char[strlen(oldname) + 3];
			SPRINTF(newname, " %s ", oldname);
			xv_set(inports[i],
			    PANEL_CHOICE_STRING, 0, newname, NULL);
			delete newname;
		}
	}
	xv_set(objs->RecordInputGroup, GROUP_HORIZONTAL_SPACING, 0,
	    XV_X, 10, XV_Y, 8, NULL);

	// Check to see if things moved
	i = xv_get(objs->RecordInputGroup, XV_X) +
	    xv_get(objs->RecordInputGroup, XV_WIDTH);
	if (i != rt)
		resize();
}

// Set error message string in the left footer
void Recordctl_xv::
SetErrormsg(
	char*		msg)
{
	if (msg == NULL)
		msg = "";
	xv_set(objs->RecordctlPanel, FRAME_LEFT_FOOTER, msg, NULL);
}

// Convert device gain into the local scaling factor
// Set Auto-Adjust message string
void Recordctl_xv::
SetAdjustmsg(
	char*		msg)
{
	if ((msg == NULL) || (*msg == '\0')) {
		xv_set(objs->AutoAdjustMsg, PANEL_SHOW_ITEM, FALSE,
		    PANEL_LABEL_STRING, "", NULL);
	} else {
		xv_set(objs->AutoAdjustMsg, PANEL_SHOW_ITEM, TRUE,
		    PANEL_LABEL_STRING, msg, NULL);
		xv_set(objs->AutoAdjustMsgGroup, GROUP_LAYOUT, TRUE, NULL);
	}
}

// Convert device gain into the local scaling factor
int Recordctl_xv::
unscale_gain(
	unsigned int	g)
{
	return (irint(max_gain *
	    (((double)(g - AUDIO_MIN_GAIN)) /
	    (double)(AUDIO_MAX_GAIN - AUDIO_MIN_GAIN))));
}

// Update the Record Control Panel according to the given values
void Recordctl_xv::
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
	if (nip->record.gain != ~0) {
		last_volume = unscale_gain(nip->record.gain);
		SET_VAL(RecordVolumeSlider, last_volume);
	}
	if (nip->monitor_gain != ~0) {
		last_monitor = unscale_gain(nip->monitor_gain);
		SET_VAL(MonitorVolumeSlider, last_monitor);
	}

	// Recalculate balance to compensate for center detent
	// XXX - balance slider range must match device range
	if (balance && (nip->record.balance != UC_INIT)) {
		last_balance = nip->record.balance;
		if (last_balance == (max_balance / 2)) {
			last_balance += BALANCE_MIDPTS / 2;
		} else if (last_balance > (max_balance / 2)) {
			last_balance += BALANCE_MIDPTS;
		}
		SET_VAL(RecordBalanceSlider, last_balance);
	}

	// Update input ports
	if (nip->record.port != ~0) {
		for (i = 0; i < MAX_INPUTS; i++) {
			p = (unsigned)xv_get(inports[i], PANEL_CLIENT_DATA);
			xv_set(inports[i],
			    PANEL_CHOOSE_ONE,
			    (p & nip->record.port) ? TRUE : FALSE,
			    PANEL_VALUE, (p & nip->record.port) ? 1 : 0, NULL);
		}
	}
	// Update available input ports
	if (nip->record.avail_ports != ~0) {
		for (i = 0; i < MAX_INPUTS; i++) {
			p = (unsigned)xv_get(inports[i], PANEL_CLIENT_DATA);
			xv_set(inports[i], PANEL_INACTIVE,
			    (p & nip->record.avail_ports) ? FALSE : TRUE, NULL);
		}
	}
}

void Recordctl_xv::
SetLevelmeter(
	double		level,		// 0. to 1.
	int		flag)		// values defined in recordctl.h
{
	xv_set(objs->MeterGauge,
	    PANEL_VALUE, (int) aint(meter_steps * level), NULL);
	xv_set(objs->NosignalMsg,
	    PANEL_SHOW_ITEM, (flag & LEVEL_NO_SIGNAL), NULL);
	xv_set(objs->LowsignalMsg,
	    PANEL_SHOW_ITEM, (flag & LEVEL_LO_SIGNAL), NULL);
	xv_set(objs->PeakMsg,
	    PANEL_SHOW_ITEM, (flag & LEVEL_PEAK_SIGNAL), NULL);
}


// Auto-Adjust implementation methods

// Set the button label to start or stop
void Recordctl_xv::
setadjustbutton()
{
	// If adjusting, set button to Stop, else reset to Auto-Adjust
	if (adjusting) {
		xv_set(objs->AutoAdjustButton,
		    PANEL_LABEL_STRING, MGET("Stop Adjust"), NULL);
	} else {
		xv_set(objs->AutoAdjustButton,
		    PANEL_LABEL_STRING, MGET("Auto-Adjust"), NULL);
	}
	// Re-layout
	xv_set(objs->AutoAdjustGroup, GROUP_LAYOUT, TRUE, NULL);
}

// When the Auto-Adjust button is pressed, take appropriate action
void Recordctl_xv::
startstop_button()
{
	// Keep the frame up for now, in case the pin is out
	xv_set(objs->AutoAdjustButton, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);

	// Start Auto-Adjust if not running
	// Stop if already running
	if (!adjusting) {
		StartAdjust();
	} else {
		// Stop adjusting.  Don't take down the frame if unpinned.
		StopAdjust(FALSE);
	}
}

// Start an Auto-Adjust
void Recordctl_xv::
StartAdjust()
{
	// Invoke the parent class method to access the device and initialize
	Recordctl::StartAdjust();

	// Change the adjust button message, if successfully started
	setadjustbutton();

#ifdef notdef
	// Kick the timer handler to start timer events rolling
	(void) timer_handler((Notify_client) objs->RecordctlPanel, 0);
#else
	Update();
#endif
}

// Stop an Auto-Adjust.  If 'takedown' is TRUE, unmap the frame if not pinned.
void Recordctl_xv::
StopAdjust(
	int		takedown)
{
	// Invoke the parent class method to stop auto-adjust
	Recordctl::StopAdjust(takedown);

	// Reset the adjust button, clear message, and level meter
	setadjustbutton();
	SetAdjustmsg();
	SetLevelmeter(0.);

	// If takedown is TRUE, unmap the frame if it is unpinned
	if (takedown) {
		SetErrormsg(MGET("Auto-adjust finished: Level OK"));
		if (xv_get(objs->RecordctlPanel, FRAME_CMD_PIN_STATE) ==
		    FRAME_CMD_PIN_OUT)
			Unshow();
	} else {
		SetErrormsg(MGET("Auto-adjust terminated"));
	}
}


// XView callbacks, adapted from recordctl_stubs.cc

// Event callback function for `RecordctlPanel'.
Notify_value
recordctl_RecordctlPanel_event_callback(
	Xv_window		win,
	Event*			event,
	Notify_arg		arg,
	Notify_event_type	type)
{
	Recordctl_xv*		xvp;

	switch (event_action(event)) {
	case WIN_RESIZE:
	case ACTION_OPEN:
	case ACTION_CLOSE:
		xvp = (Recordctl_xv*) xv_get(win, XV_KEY_DATA, RECORDCTLKEY);
		switch (event_action(event)) {
		case WIN_RESIZE:
			break;
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

// Notify callback function for `RecordVolumeSlider'.
void
Recordctl_volume_proc(
	Panel_item			item,
	int				value,
	Event*				event)
{
	recordctl_RecordctlPanel_objects*	ip;
	Recordctl_xv*			xvp;

	ip = (recordctl_RecordctlPanel_objects*)
	    xv_get(item, XV_KEY_DATA, INSTANCE);
	xvp = (Recordctl_xv*)
	    xv_get(ip->RecordctlPanel, XV_KEY_DATA, RECORDCTLKEY);

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

// Notify callback function for `MonitorVolumeSlider'.
void
Recordctl_monitor_proc(
	Panel_item			item,
	int				value,
	Event*				event)
{
	recordctl_RecordctlPanel_objects*	ip;
	Recordctl_xv*			xvp;

	ip = (recordctl_RecordctlPanel_objects*)
	    xv_get(item, XV_KEY_DATA, INSTANCE);
	xvp = (Recordctl_xv*)
	    xv_get(ip->RecordctlPanel, XV_KEY_DATA, RECORDCTLKEY);

	// If no change, don't sweat it
	if (value == xvp->last_monitor)
		return;

	// Check for a single click increment/decrement
	if (event_is_up(event)) {
		if ((value - xvp->last_monitor) == 1) {
			xvp->MonitorIncr(TRUE);			// increment
		} else if ((xvp->last_monitor - value) == 1) {
			xvp->MonitorIncr(FALSE);		// decrement
		} else {
			xvp->SetMonitor(scale_gain(value, xvp->max_gain));
		}
	} else {
		xvp->SetMonitor(scale_gain(value, xvp->max_gain));
	}
}

// Notify callback function for `RecordBalanceSlider'.
void
Recordctl_balance_proc(
	Panel_item			item,
	int				value,
	Event*				event)
{
	recordctl_RecordctlPanel_objects*	ip;
	Recordctl_xv*			xvp;
	unsigned int			mid;
	int				delta;

#define	nearmid(X)	(((X) >= (xvp->max_balance / 2)) &&		\
			 ((X) <= ((xvp->max_balance / 2) + BALANCE_MIDPTS)))

	ip = (recordctl_RecordctlPanel_objects*)
	    xv_get(item, XV_KEY_DATA, INSTANCE);
	xvp = (Recordctl_xv*)
	    xv_get(ip->RecordctlPanel, XV_KEY_DATA, RECORDCTLKEY);

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

// Notify callback function for Inport switches
void
Recordctl_inport_proc(
	Panel_item			item,
	unsigned int			value,
	Event*)
{
	recordctl_RecordctlPanel_objects*	ip;
	Recordctl_xv*			xvp;
	unsigned			port;

	ip = (recordctl_RecordctlPanel_objects*)
	    xv_get(item, XV_KEY_DATA, INSTANCE);
	xvp = (Recordctl_xv*)
	    xv_get(ip->RecordctlPanel, XV_KEY_DATA, RECORDCTLKEY);
	port = (unsigned) xv_get(item, PANEL_CLIENT_DATA);
	xvp->SetPort(port, (value != 0));
}

// Notify callback function for `AutoAdjustButton'.
void
Recordctl_autoadjust_proc(
	Panel_item			item,
	Event*)
{
	recordctl_RecordctlPanel_objects*	ip;
	Recordctl_xv*			xvp;

	ip = (recordctl_RecordctlPanel_objects*)
	    xv_get(item, XV_KEY_DATA, INSTANCE);
	xvp = (Recordctl_xv*)
	    xv_get(ip->RecordctlPanel, XV_KEY_DATA, RECORDCTLKEY);

	// Call the class code to handle start and stop events
	xvp->startstop_button();
}

// Menu handler for `RecordctlPanel_menu (Play...)'.
Menu_item
recordctl_RecordctlPanel_menu_item0_callback(
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

// Menu handler for `RecordctlPanel_menu (Status...)'.
Menu_item
recordctl_RecordctlPanel_menu_item1_callback(
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
