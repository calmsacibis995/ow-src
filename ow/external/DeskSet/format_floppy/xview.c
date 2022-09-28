#include <stdio.h>
#include <sys/param.h>
#include <xview/xview.h>
#include <xview/frame.h>
#include <xview/panel.h>
#include "format_floppy_ui.h"
#include "format_floppy.h"

/*  NOTE on DevGuide naming:
 *  The "Format Disk" and "Cancel" buttons, the choice settings and "Disk Name"
 *  are the same on all 3 popups and that is why they all have the same
 *  Panel_item name.
 */

Attr_attribute INSTANCE;
format_floppy_base_frame_objects *Format_floppy_base_frame;
format_floppy_format_popup_objects *Format_floppy_format_popup;
format_floppy_unformatted_popup_objects *Format_floppy_unformatted_popup;
format_floppy_unlabeled_popup_objects *Format_floppy_unlabeled_popup;

/* Exit the program when the user pushes the pushpin in.  Otherwise this process
 * will never terminate.
 */
my_frame_done_proc(frame)
Frame frame;
{
	xv_set(frame, XV_SHOW, FALSE, NULL);
	exit(0);
}

int
main(argc, argv)
int argc;
char *argv[];
{
	char bind_home[MAXPATHLEN];
	Frame main_frame;
	Panel main_panel;
	Panel_item main_format_button;
	Panel_item main_cancel_button;
	Panel_item main_eject_button;
	Display *dpy;
	int screen_no;
	int dpy_size = 0;
	int frame_size = 0;
	int three_button = 0;

	ds_expand_pathname("$OPENWINHOME/lib/locale", bind_home);
	bindtextdomain(MSGFILE, bind_home);

	init_program(argc, argv);

	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
		XV_USE_LOCALE, TRUE,
		NULL);
	INSTANCE = xv_unique_key();

	Format_floppy_base_frame = format_floppy_base_frame_objects_initialize(NULL, NULL);
	xv_set(Format_floppy_base_frame->base_frame, XV_SHOW, FALSE, NULL);
	switch (popup_type) {
		case Format:
			Format_floppy_format_popup = format_floppy_format_popup_objects_initialize(NULL, Format_floppy_base_frame->base_frame);
			main_frame = (Frame)Format_floppy_format_popup->format_popup;	
			main_panel = (Panel)Format_floppy_format_popup->format_panel;
			xv_set(Format_floppy_format_popup->format_diskname, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
			main_format_button = (Panel_item)Format_floppy_format_popup->format_format;
			main_cancel_button = (Panel_item)Format_floppy_format_popup->format_cancel;
			break;
		case Unformatted:
			Format_floppy_unformatted_popup = format_floppy_unformatted_popup_objects_initialize(NULL, Format_floppy_base_frame->base_frame);
			main_frame = (Frame)Format_floppy_unformatted_popup->unformatted_popup;	
			main_panel = (Panel)Format_floppy_unformatted_popup->unformatted_panel;
			xv_set(Format_floppy_unformatted_popup->format_diskname, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
			main_format_button = (Panel_item)Format_floppy_unformatted_popup->format_format;
			main_cancel_button = (Panel_item)Format_floppy_unformatted_popup->format_cancel;
			main_eject_button = (Panel_item)Format_floppy_unformatted_popup->format_eject;
			three_button = 1;
			break;
		case Unlabeled:
			Format_floppy_unlabeled_popup = format_floppy_unlabeled_popup_objects_initialize(NULL, Format_floppy_base_frame->base_frame);
			main_frame = (Frame)Format_floppy_unlabeled_popup->unlabeled_popup;	
			main_panel = (Panel)Format_floppy_unlabeled_popup->unlabeled_panel;
			xv_set(Format_floppy_unlabeled_popup->format_diskname, PANEL_NOTIFY_LEVEL, PANEL_ALL, NULL);
			main_format_button = (Panel_item)Format_floppy_unlabeled_popup->format_format;
			main_cancel_button = (Panel_item)Format_floppy_unlabeled_popup->format_cancel;
			main_eject_button = (Panel_item)Format_floppy_unlabeled_popup->format_eject;
			three_button = 1;
			break;
	}

	if ( three_button ) {
		ds_center_items(main_panel, -1,
			main_format_button,
			main_cancel_button,
			main_eject_button,
			0);
	} else {
		ds_center_items(main_panel, -1,
			main_format_button,
			main_cancel_button,
			0);
	}
	screen_no = (int)xv_get((Xv_Screen)xv_get(main_frame, XV_SCREEN), SCREEN_NUMBER);
	dpy = (Display *)xv_get(main_frame, XV_DISPLAY);
	if ( x_position ) {
		dpy_size = DisplayWidth(dpy, screen_no);
		frame_size = (int)xv_get(main_frame, XV_WIDTH);
		dpy_size -= frame_size;
		if ( x_position >  dpy_size ) {
			xv_set(main_frame, XV_X, dpy_size, NULL);
		} else {
			xv_set(main_frame, XV_X, x_position, NULL);
		}
	}
	if ( y_position ) {
		dpy_size = DisplayHeight(dpy, screen_no);
		frame_size = (int)xv_get(main_frame, XV_HEIGHT);
		dpy_size -= frame_size;
		if ( y_position > dpy_size ) {
			xv_set(main_frame, XV_Y, dpy_size, NULL);
		} else {
			xv_set(main_frame, XV_Y, y_position, NULL);
		}
	}
	xv_set(main_frame, FRAME_DONE_PROC, my_frame_done_proc, XV_SHOW, TRUE, NULL);
	xv_main_loop(main_frame);
	exit(0);
}

Panel_setting
store_label(item, event)
Panel_item item;
Event *event;
{
	label = (char *)strdup((char *)xv_get(item, PANEL_VALUE));

	return panel_text_notify(item, event);
}

void
format(item, event)
Panel_item item;
Event *event;
{
	format_floppy_format_popup_objects *ip = (format_floppy_format_popup_objects *)xv_get(item, XV_KEY_DATA, INSTANCE);

	/* Provide feedback to user when formatting */
	xv_set(ip->format_popup, FRAME_BUSY, TRUE, NULL);
	xv_set(ip->format_popup, FRAME_SHOW_FOOTER, TRUE,
		FRAME_LEFT_FOOTER, dgettext(MSGFILE, "Formatting takes a couple of minutes.  Please wait."),
		NULL);
	switch ((int)xv_get(ip->format_choice, PANEL_VALUE)) {
		case 0:    /* UNIX format */
			format_floppy(0);
			break;
		case 1:    /* DOS high density */
			format_floppy(1);
			break;
		case 2:    /* NEC-DOS medium density */
			format_floppy(2);
			break;
	}
	xv_set(ip->format_popup, FRAME_BUSY, FALSE, NULL);
	exit(0);
}

void
cancel(item, event)
Panel_item item;
Event *event;
{
	exit(0);
}

void
eject(item, event)
Panel_item item;
Event *event;
{
	system("/bin/eject floppy");
	exit(0);
}

void
unformatted_format(item, event)
Panel_item item;
Event *event;
{
	format_floppy_unformatted_popup_objects *ip = (format_floppy_unformatted_popup_objects *)xv_get(item, XV_KEY_DATA, INSTANCE);

	xv_set(ip->unformatted_popup, FRAME_BUSY, TRUE, NULL);
	xv_set(ip->unformatted_popup, FRAME_SHOW_FOOTER, TRUE,
		FRAME_LEFT_FOOTER, dgettext(MSGFILE, "Formatting takes a couple of minutes.  Please wait."),
		NULL);
	switch ((int)xv_get(ip->format_choice, PANEL_VALUE)) {
		case 0:    /* UNIX format */
			format_floppy(0);
			break;
		case 1:    /* DOS high density */
			format_floppy(1);
			break;
		case 2:    /* NEC-DOS medium density */
			format_floppy(2);
			break;
	}
	xv_set(ip->unformatted_popup, FRAME_BUSY, FALSE, NULL);
	exit(0);
}

void
unlabeled_format(item, event)
Panel_item item;
Event *event;
{
	format_floppy_unlabeled_popup_objects *ip = (format_floppy_unlabeled_popup_objects *)xv_get(item, XV_KEY_DATA, INSTANCE);

	xv_set(ip->unlabeled_popup, FRAME_BUSY, TRUE, NULL);
	xv_set(ip->unlabeled_popup, FRAME_SHOW_FOOTER, TRUE,
		FRAME_LEFT_FOOTER, dgettext(MSGFILE, "Formatting takes a couple of minutes.  Please wait."),
		NULL);
	switch ((int)xv_get(ip->format_choice, PANEL_VALUE)) {
		case 0:    /* UNIX format */
			format_floppy(0);
			break;
		case 1:    /* DOS high density */
			format_floppy(1);
			break;
		case 2:    /* NEC-DOS medium density */
			format_floppy(2);
			break;
	}
	xv_set(ip->unlabeled_popup, FRAME_BUSY, FALSE, NULL);
	exit(0);
}
