#ifdef lint
static  char sccsid[] = "@(#)cm_tt.c 1.7 93/01/20 Copyr 1991 Sun Microsystems, Inc.";
#endif

/*
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */ 

/*
 * ToolTalk interface functions for cm.
 */

#include <stdio.h>
#include <sys/param.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/cms.h>
#include <xview/font.h>
#include <xview/sel_pkg.h>
#include <desktop/tt_c.h>
#include <ds_popup.h>
#include <ds_tooltalk.h>
#include <dstt.h>

#define AE_MEDIA		"Sun_CM_Appointment"
#define MAIL_MEDIA		"RFC_822_Message"

extern	int debug;
extern	int gargc;
extern	char **gargv;

static Xv_opaque	frame = NULL;

/*
 * used by dstt
 */
void
get_version(char **vender, char **name, char **version)
{
	extern char *ds_relname();

	if (debug)
		fprintf(stderr, "get_version called\n");

	*vender = "Sunsoft, Inc.";
	*name = "Calendar Manager";
	*version = ds_relname();
}

/*
 * CLOSE callback routine.
 * If the CLOSE message is,
 * TT_HANDLED: set FRAME_BUSY of the multi-browser window to FALSE
 * TT_FAILED: get cm's compose window.
 */
int
close_cb(Tt_message	m,
	void		*key,
	bufftype_t	bufftype,
	char		*id,
	int		inquisitive,
	int		force)
{
	switch (tt_message_state(m)) {
	case TT_HANDLED:
		if (debug)
			fprintf(stderr, "close_cb: TT_HANDLED\n");

		if (frame != NULL)
			xv_set(frame, FRAME_BUSY, FALSE, NULL);
		break;

	case TT_FAILED:
		if (debug)
			fprintf(stderr, "close_cb: TT_FAILED\n");

		/* get_cm_compose will set FRAME_BUSY to FALSE */
		get_cm_compose();
		break;

	default:
		if (debug)
			fprintf(stderr, "close_cb: msg state = %d\n",
				tt_message_state(m));
	}
	return(OK);
}

/*
 * PASTE callback routine.
 * If the PASTE message is,
 * TT_HANDLED: send a CLOSE message to complete the hand shaking.
 * TT_FAILED: send a CLOSE message to abort the hand shaking and
 *		get cm's compose window.
 */
int
paste_cb(Tt_message	m,
	void		*key,
	bufftype_t	bufftype,
	char		*id,
	char		*media,
	Data_t		type,
	void		*data,
	int		size,
	int		offset,
	char		*locator)
{
	switch (tt_message_state(m)) {
	case TT_HANDLED:
		if (debug)
			fprintf(stderr, "%s: TT_HANDLED; dstt_close called\n",
				"paste_cb");

		/* send close message: inquisitive = 1, force = 0 */
		dstt_close(close_cb, NULL, buffer, id, 1, 0);
		break;

	case TT_FAILED:
		if (debug)
			fprintf(stderr, "%s: TT_FAILED; dstt_close called\n",
				"paste_cb");

		/* inquisitive = 0, force = 1 */
		dstt_close(close_cb, NULL, buffer, id, 0, 1);

		/* get_cm_compose will set FRAME_BUSY to FALSE */
		get_cm_compose();
		break;

	default:
		if (debug)
			fprintf(stderr, "paste_cb: msg state = %d\n",
				tt_message_state(m));
	}
	return(OK);
}

/*
 * OPEN callback routine.
 * If the OPEN message is,
 * TT_HANDLED: send the appointment template using a PASTE messsage.
 * TT_FAILED: get cm's compose window.
 * TT_STARTED: don't need to do anything.
 */
int
open_cb(Tt_message	m,
	void		*key,
	char		*media,
	Data_t		type,
	void		*data,
	int		size,
	bufftype_t	bufftype,
	char		*id,
	dstt_bool_t	readonly,
	dstt_bool_t	mapped,
	int		shareLevel,
	char		*locator)
{
	extern char *get_appt_str();
	char *appt = NULL;

	switch (tt_message_state(m)) {
	case TT_HANDLED:
		if (debug)
			fprintf(stderr, "%s: TT_HANDLED; dstt_paste called\n",
				"open_cb");

		appt = get_appt_str();

		/* send attachment */
		dstt_paste(paste_cb, NULL, buffer, id, AE_MEDIA, contents,
			appt, (appt ? cm_strlen(appt) : 0), 0, NULL);

		if (appt)
			free(appt);
		break;

	case TT_FAILED:
		if (debug)
			fprintf(stderr, "open_cb: TT_FAILED\n");

		/* get_cm_compose will set FRAME_BUSY to FALSE */
		get_cm_compose();
		break;

	default:
		if (debug)
			fprintf(stderr, "open_cb: msg state = %d\n",
				tt_message_state(m));
	}
	return(OK);
}

/*
 * Get compose window from mailtool through tooltalk.
 * Start the OPEN-PASTE-CLOSE hand shaking by sending an OPEN message
 * which sends the message header (to:, subject:, etc) to mailtool.
 */
extern int
cm_tt_compose(header, cframe, bframe)
	char *header;
	Xv_opaque cframe;	/* main window of cm */
	Xv_opaque bframe;	/* multi-browse window */
{
	static int init = FALSE;

	if (header == NULL)
		return 1;

	if (init == FALSE) {
		frame = bframe;

		/* initialize dstt */
		if (dstt_check_startup(get_version, &gargc, &gargv)) {
			if (debug)
				fprintf(stderr, "dstt_check_startup failed\n");
			return 1;
		} else {
			init = TRUE;

			dstt_xview_desktop_callback(cframe, NULL);
			dstt_xview_desktop_register(cframe, NULL);
			dstt_xview_start_notifier();
		}
	}

	if (dstt_open(open_cb, NULL, MAIL_MEDIA, contents, header,
			cm_strlen(header), buffer, NULL, FALSE, 0, FALSE,
			"Charactor:0"))
	{
		if (debug)
			fprintf(stderr, "%s: dstt_open called successfully\n",
				"cm_tt_compose");
		return 0;
	} else {
		if (debug)
			fprintf(stderr, "%s: dstt_open called but failed\n",
				"cm_tt_compose");
		return 1;
	}
}

/*
 * Send a broadcast message through tooltalk to announce that
 * cm properties are changed.
 */
extern void
cm_tt_update_props(filename)
	char *filename;
{
	if (debug)
		fprintf(stderr, "cm_tt_update_props: dstt_modified called\n");

	dstt_modified("File", filename);
}

