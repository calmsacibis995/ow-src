#pragma ident	"@(#)oldials.c	1.7    93/02/04 examples/olit/oldials SMI"    /* OLIT */

/*
 *	Copyright (C) 1991, 1993  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 * This file is a product of Sun Microsystems, Inc. and is
 * provided for unrestricted use provided that this legend is
 * included on all media and as a part of the software
 * program in whole or part.  Users may copy or modify this
 * file without charge, but are not authorized to license or
 * distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND
 * INCLUDING THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE, OR ARISING FROM A COURSE
 * OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * This file is provided with no support and without any
 * obligation on the part of Sun Microsystems, Inc. to assist
 * in its use, correction, modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT
 * TO THE INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY
 * PATENTS BY THIS FILE OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any
 * lost revenue or profits or other special, indirect and
 * consequential damages, even if Sun has been advised of the
 * possibility of such damages.
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>

#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/FNonexclus.h>
#include <Xol/Gauge.h>
#include <Xol/OblongButt.h>
#include <Xol/OpenLook.h>
#include <Xol/StaticText.h>

#define GAUGE_MAX	65536
#define GAUGE_HALF	GAUGE_MAX / 2

static void extension_handler();

typedef struct {
    Boolean verbose;
} ApplicationData, *ApplicationDataPtr;

static String captions[] = { "60 Hz :",
			   "125 Hz :",
			   "250 Hz :",
			   "500 Hz :",
			   "1 KHz :",
			   "4 KHz :",
			   "8 KHz :",
			   "16 KHz :" };

Display *dpy;
Widget gauge[8];
int gauge_values[8] = { GAUGE_HALF, GAUGE_HALF, GAUGE_HALF, GAUGE_HALF,
			GAUGE_HALF, GAUGE_HALF, GAUGE_HALF, GAUGE_HALF };
int motion_notify_type;
XEventClass motion_notify_class;
XID device_id = 0;
ApplicationData app_data;

/*
 * New options on the command line
 */
static XrmOptionDescRec options[] = {
    { "-verbose",	".verbose",	XrmoptionNoArg,		"true" }
};

#define offset(F)	XtOffsetOf(ApplicationData, F)
static XtResource resources[] = {
    { "verbose", "Verbose", XtRBoolean, sizeof(Boolean),
      offset(verbose), XtRImmediate, (XtPointer) False}
};
#undef offset


void
main(const unsigned int argc, const char*const argv[])
{
    Widget toplevel, control_area, text, caption, non_exclusives;
    XtAppContext app;
    Arg args[12];
    Cardinal ac;
    char **extensions;
    XDeviceInfo *input_devices;
    XDevice *knob_device;
    Status status;
    int nextensions, ndevices, j, k, num_axes;
    Atom knob_atom;

    OlToolkitInitialize(NULL);
    toplevel = XtAppInitialize(&app, "Equalizer", options, XtNumber(options),
			       (int*)&argc, (char**)argv, NULL, NULL, 0);

    XtGetApplicationResources(toplevel, &app_data, resources,
				XtNumber(resources), NULL, 0);

    dpy = XtDisplay(toplevel);

    ac = 0;
    XtSetArg(args[ac], XtNlayoutType, (XtArgVal) OL_FIXEDCOLS);		++ac;
    XtSetArg(args[ac], XtNalignCaptions, (XtArgVal) True);		++ac;
    control_area = XtCreateManagedWidget("control_area",	/* Name */
				controlAreaWidgetClass,		/* Class */
    				toplevel,			/* Parent */
				args, ac);			/* Args */

    ac = 0;
    XtSetArg(args[ac], XtNstring, (XtArgVal) "Graphic Equalizer");	++ac;
    text = XtCreateManagedWidget("staticText",			/* Name */
				staticTextWidgetClass,		/* Class */
    		 		control_area,			/* Parent */
				args, ac);			/* Args */

    /* List all available server extensions */
    extensions = XListExtensions(dpy, &nextensions);
    if (app_data.verbose) {
	printf("Extensions:\n");
        for (j = 0; j < nextensions; j++)
	    printf("  %s\n", extensions[j]);
    }

    /* Get the supported input devices */
    input_devices = XListInputDevices(dpy, &ndevices);

    knob_atom = XInternAtom(dpy, "KNOB_BOX", True);
    if (knob_atom == (Atom)None) {
	(void) fprintf(stderr, "KNOB_BOX device not supported by server\n");
	exit(EXIT_SUCCESS);
    }

    for (j = 0; j < ndevices && input_devices[j].type != knob_atom; j++)
	;

    /*
     * If num_classes is 0, this probably means the dials box is not attached.
     */
    if (j == ndevices || input_devices[j].num_classes == 0) {
	fprintf(stderr, "KNOB_BOX device not available\n");
	exit(1);
    }

    device_id = input_devices[j].id;
    for (k = 0; k < input_devices[j].num_classes;
				k++, input_devices[j].inputclassinfo++) {
	if (input_devices[j].inputclassinfo->class == ValuatorClass) {
	    num_axes = ((XValuatorInfo *)
				(input_devices[j].inputclassinfo))->num_axes;
	    if (app_data.verbose)
	        printf("Number of axes is: %d\n", num_axes);
	}
    }

    /* Free up the input device information */
    XFreeDeviceList(input_devices);

    ac = 0;
    XtSetArg(args[ac], XtNwidth, (XtArgVal) 500);			++ac;
    XtSetArg(args[ac], XtNorientation, (XtArgVal) OL_HORIZONTAL);	++ac;
    XtSetArg(args[ac], XtNticks, (XtArgVal) 10);			++ac;
    XtSetArg(args[ac], XtNtickUnit, (XtArgVal) OL_PERCENT);		++ac;
    XtSetArg(args[ac], XtNsliderMin, (XtArgVal) 0);			++ac;
    XtSetArg(args[ac], XtNsliderMax, (XtArgVal) GAUGE_MAX);		++ac;
    XtSetArg(args[ac], XtNsliderValue, (XtArgVal) GAUGE_HALF);		++ac;
    for (j=0; j < num_axes; j++) {
	caption = XtCreateManagedWidget(captions[j],		/* Name */
				captionWidgetClass,		/* Class */
				control_area,			/* Parent */
				args, ac);			/* Args */

        gauge[j] = XtCreateManagedWidget("gauge",		/* Name */
				gaugeWidgetClass,		/* Class */
				caption,			/* Parent */
				args, ac);			/* Args */
    }

    { /* keep flat stuff in local block */
        typedef struct {
	    XtArgVal	label;
	    XtArgVal	set;
        } FlatData;

        static FlatData *items;
        static String item_fields[] = { XtNlabel, XtNset };

        items = (FlatData *) XtMalloc(2 * sizeof(FlatData));
        items[0].label = (XtArgVal) "Loudness";
        items[1].label = (XtArgVal) "Equalizer";
        items[0].set = (XtArgVal) False;
        items[1].set = (XtArgVal) False;

        ac = 0;
        XtSetArg(args[ac], XtNitems, (XtArgVal) items);			++ac;
        XtSetArg(args[ac], XtNnumItems, (XtArgVal) 2);			++ac;
        XtSetArg(args[ac], XtNitemFields, (XtArgVal) item_fields);	++ac;
        XtSetArg(args[ac], XtNnumItemFields, (XtArgVal) XtNumber(item_fields));
									++ac;

        non_exclusives = XtCreateManagedWidget("nonExclusives",	/* Name */
				flatNonexclusivesWidgetClass,	/* Class */
				control_area,			/* Parent */
				args, ac);			/* Args */
    } /* end local block */

    /*
     * Request the server to open the device
     */
    knob_device = XOpenDevice(dpy, device_id);

    /*
     * Extension event types and classes (masks) are not constants, but
     * instead are allocated dynamically when the extension is initialized.
     * The class is needed to indicate which events we wish to receive.
     * The type is needed to determine precisely what event we have received.
     * The following macro retrieves the type and class
     */
    DeviceMotionNotify(knob_device, motion_notify_type, motion_notify_class);

    /*
     * The Intrinsics know nothing about extension events. In order to
     * receive these events, we add an event handler for NoEventMask with
     * the 'nonmaskable' argument True. This normally means we simply receive
     * the non-maskable events, but the Intrinsics have been bug-fixed so that
     * extension events are treated in the same way as non-maskable events.
     */
    XtAddEventHandler(non_exclusives, NoEventMask, True,
				extension_handler, NULL);

    /*
     * Realize the widget hierarchy - this creates our widgets' windows
     * and we need a valid window for the XSelectExtensionEvent()
     */
    XtRealizeWidget(toplevel);

    /*
     * By default the non-exclusive widget is the only widget in this
     * application that will take keyboard focus (gauges and static text
     * have *traversalOn set to False). Therefore setting the focus argument
     * in the following call to FollowKeyboard means that the dial events
     * are delivered to the non_exclusives widget. It is also possible to
     * explicitly set a Window as the focus argument; however, this window
     * must have the map_state IsViewable.
     */
    XSetDeviceFocus(dpy, knob_device, FollowKeyboard,
			RevertToPointerRoot, CurrentTime);
    XSelectExtensionEvent(dpy, XtWindow(non_exclusives),
				&motion_notify_class, 1);
    XtMainLoop();
}


/*
 * This event handler will be called for any nonmaskable events AND any
 * extension events.
 */
static void
extension_handler(w, data, event, continue_to_dispatch)
Widget w;
XtPointer data;
XEvent *event;
Boolean continue_to_dispatch;
{
    unsigned char	i;

    if (event->type == motion_notify_type) {
	XDeviceMotionEvent *motionEvent = (XDeviceMotionEvent *)event;

	/*
	 * The axis_data we receive is an array of integers in the range
	 * 0 => 65536 in multiples of 90.
	 * 0 => 32768 	    represent increasing clockwise motion.
	 * 65536 => 32768    represent increasing anti-clockwise motion.
	 * ... therefore 65446 is an anti-clockwise motion equal to 90
	 * 'units' of clockwise motion.
	 */

	if (device_id == motionEvent->deviceid) {
	    for (i = 0; i < motionEvent->axes_count; i++) {
		int this_dial = i + motionEvent->first_axis;
		int this_value = motionEvent->axis_data[i];

		if (app_data.verbose)
		    printf("Axis %d:    %d\n", this_dial, this_value);

		if (this_value > GAUGE_MAX)
		    this_value -= 65536;
		assert(this_value <= GAUGE_MAX && this_value >= -GAUGE_MAX);
		gauge_values[this_dial] += this_value;

		if (gauge_values[this_dial] > GAUGE_MAX)
		    gauge_values[this_dial] -= GAUGE_MAX;
		if (gauge_values[this_dial] < 0)
		    gauge_values[this_dial] += GAUGE_MAX;

		OlSetGaugeValue(gauge[this_dial], gauge_values[this_dial]);
	    }
	}
    }
}
