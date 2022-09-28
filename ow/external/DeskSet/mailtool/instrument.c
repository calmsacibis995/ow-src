#if !defined(lint) && defined(sccs)
static char sccsid[] = "@(#)instrument.c 3.1 92/04/03 Copyr 1990 Sun Micro";
#endif

/*	Copyright (c) 1989, 1990, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */


/* instrument.c -- try and gather statistics about the mail system */

#ifdef INSTRUMENT

#include <stdio.h>
#include <sys/types.h>

#include <xview/xview.h>
#include <xview/text.h>
#include <xview/panel.h>

#define RINGSIZE (64*1024)

static char ringbuffer[RINGSIZE];
static char *ringptr = ringbuffer;
static char *ringend = &ringbuffer[RINGSIZE];
static int ringflushcount;
static time_t flushtime;

int mt_output_tracking = 0;


static void flushring(type)
char *type;
{
	FILE *fd;
	int count;
	int len;

	flushtime = time(0L);

	if (ringptr == ringbuffer) return;

	if (ringptr[-1] != '\n') *ringptr++ = '\n';

	/* send the contents of the ring as a mail message */
	fd = popen("/usr/lib/sendmail -t", "w");

	if (fd != NULL) {
		len = ringptr - ringbuffer;
		fprintf(fd, "To: instrument@skylark.eng.sun.com\n" );
		fprintf(fd, "Subject: count %d type %s\n",
			ringflushcount++, type );
		fprintf(fd, "\n");
		count = fwrite(ringbuffer, 1, len, fd);

		if (count != len) {
			fprintf(stderr,
"short write for flush of instrumentation buffer: tried %d, got %d\n",
				len, count);
			fprintf(fd,
"short write for flush of instrumentation buffer: tried %d, got %d\n",
				len, count);

		}

		pclose(fd);
	}

	/* reset the ring */
	ringptr = ringbuffer;

}

static void addring(string)
char *string;
{
	int len = strlen(string);

	/* for debugging... */
	if (mt_output_tracking) {
		fputs(string, stderr);
	}

	if (ringptr + len >= ringend) {
		flushring("buffer full");

		/* in theory, if string > RINGSIZE we blow up here, but
		** that won't happen in reality.
		*/
	}

	/* copy the string and incremnent the current ring offset */
	memcpy(ringptr, string, len);
	ringptr += len;
}


void
stop_instrumentation()
{
	flushring("stopping instrumentation");
}

void
checkpoint_instrumentation()
{
	flushring("checkpointing instrumentation");
}


void
track_drag_select(start, stop)
int start;
int stop;
{
	char buffer[90];


	sprintf(buffer, "drag %d %d\n", start, stop);

	addring(buffer);
}


void
track_toggle_select(msg, onoff)
int msg;
int onoff;
{
	char buffer[90];


	sprintf(buffer, "toggle %d %s\n", msg, (onoff ? "on" : "off"));

	addring(buffer);
}


void
track_message(message)
char *message;
{
	addring(message);
	addring("\n");
}



void
track_startup()
{

	/* as a side effect, this will record our starting time */
	flushring();

	addring("startup\n");
}


void
track_exit(status)
int status;
{
	char buffer[90];

	sprintf(buffer, "exiting with status %d\n", status );

	addring(buffer);
	flushring("exiting");
}

void
track_showmsg(msg, how)
int msg;
char *how;
{
	char buffer[90];

	sprintf(buffer, "showmsg %d %s\n", msg, how);
	addring(buffer);
}


void
track_button(menu, item, title)
Menu menu;
Menu_item item;
char *title;
{
	char buffer[90];

	strcpy(buffer, title);

	if (menu) {
		strcat(buffer, " menu");
		if (xv_get(menu, MENU_PIN) && xv_get(menu, MENU_PIN_WINDOW)) {
			strcat(buffer, " pinned");
		}
	} else {
		/* if menu is null, then we got here from a keystroke */
		strcat(buffer, " keystroke");
	}

	strcat(buffer, "\n");
	addring(buffer);
}


void
track_button2(menu, item, title)
Menu menu;
Menu_item item;
char *title;
{
	char buffer[90];
	char *title2;

	title2 = (char *)xv_get(item, MENU_STRING);
	sprintf(buffer, "%s_%s", title, title2);

	if (menu) {
		strcat(buffer, " menu");
		if (xv_get(menu, MENU_PIN) && xv_get(menu, MENU_PIN_WINDOW)) {
			strcat(buffer, " pinned");
		}
	} else {
		/* if menu is null, then we got here from a keystroke */
		strcat(buffer, " keystroke");
	}

	strcat(buffer, "\n");
	addring(buffer);
}


void
track_panel(item, ie, title)
Panel_item item;
Event ie;
char *title;
{
	char buffer[90];

	strcpy(buffer, title);

	if (item) {
		strcat(buffer, " panel");
	} else {
		/* if menu is null, then we got here from a keystroke */
		strcat(buffer, " keystroke");
	}

	strcat(buffer, "\n");
	addring(buffer);
}


void
track_timer()
{
	time_t now = time(0L);

	/* flush the buffer if its been over a day since the last time */
	if (now > flushtime + 24*60*60) {
		flushring("timeflush");
	}
}

#endif INSTRUMENT
