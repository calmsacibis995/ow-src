/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)atool_dstt.c	1.16	93/03/03 SMI"

#ifndef PRE_493

#include <xview/xview.h>
#include <desktop/tt_c.h>

#include "dstt.h"
#include "atool_types.h"
#include "atool_i18n.h"
#include "atool_debug.h"
#include "atool_panel.h"
#include "atool_panel_impl.h"
#include "atool_sel_impl.h"
#include "undolist_c.h"

extern void init_tt_keys();
extern void set_tt_quit_timer(int);
extern char *dstt_set_status(int, int);

#define	TOOLTALK_NAME	"Sun_Audio"

/* XXX - Store static data until there's a key data facility */

/* Requestor base frame */
static Frame	frame;

/* Flag bits for which applications to launch */
#define	Launch_Play	1
#define	Launch_Record	2

/* set when a launch is started */
static int	launching = FALSE;

/* set when another launch request came in while doing this one */
static int	pending = 0;
static ptr_t	Play_dp;
static char	*Play_devname;
static char	*Play_label;
static ptr_t	Record_dp;
static char	*Record_devname;
static char	*Record_label;

static char *toolid = NULL;	/* toolid of audiocontrol */

status_t
gtt_Quit(Tt_message m, int silent, int force, char *msg)
{
	DBGOUT((1, "dstt: got quit message\n"));

	if (do_handle_tt_quit(frame) == FALSE) {
		return (REJECT);
	}

	return (OK);	
}

/* create an audio buffer from TT buffer */
Audio_Object
gtt_create_buffer(ptr_t dp, void *buf, int size)
{
	Audio_hdr hdr;
	int unsigned ilen;
	Audio_Object aobj;
	unsigned long blen;
	unsigned long pos = 0;

	if (audio_decode_filehdr(buf, &hdr, &ilen) != AUDIO_SUCCESS) {
		DBGOUT((2, "error decoding header from tooltalk buffer\n"));
		return (NULL);
	}

#ifdef notdef
	/* check if it's ok to insert this data */
	if (AudPanel_Caninsert(dp, &hdr) != TRUE) {
		return (NULL);
	}
#endif

	if (hdr.data_size == AUDIO_UNKNOWN_SIZE) {
		blen = size - sizeof(Audio_hdr)+ilen;
	} else {
		blen = hdr.data_size;
	}

	DBGOUT((2, "creating an audio buffer from TT contents (blen=%d)\n",
		blen));

	aobj = audio_createbuffer(&hdr, blen);

	/* copy to audio object */
	if (audio_putbuffer(aobj, buf, &blen, &pos) != AUDIO_SUCCESS) {
		audio_dereference(aobj);
		aobj = NULL;
	}

	return (aobj);
}

status_t
gtt_Display(Tt_message m, char * key, Data_t type, void *data, int size, 
	    char *msg, char *title)
{
	char		*host, *file;
	int		len;
	Menu_item	load_item;
	char		*status_msg;
	status_t	rc = HOLD;
	AudioSelData	*asdp;
	Audio_Object	aobj;
	struct atool_panel_data	*ap;

	DBGOUT((1,"dstt: executing callback Display \n"));

	/* Guard against bogus calls */
	if (strcmp(key, TOOLTALK_NAME) != 0)
		return (REJECT);

	/* first turn of quit timer (if running) */
	set_tt_quit_timer(FALSE);

	/* XXX - when do we return FAIL or REJECT? */

	asdp = (AudioSelData*) xv_get(frame, XV_KEY_DATA, AUD_SEL_DATA_KEY);
	if (asdp == NULL) {
		DBGOUT((D_DND, "dstt: can't get seln data\n"));
		return (FAIL);
	}
	ap = (struct atool_panel_data *) asdp->ap;

	if (ap->tstate != Unloaded) {
		DBGOUT((D_DND, "dstt: rejected display, contains data.\n"));
		return (REJECT);
	}

	if (AudPanel_Getlink(ap) != NULL) {
		DBGOUT((D_DND, "dstt: rejected display, already linked.\n"));
		return (REJECT);
	}
	AudPanel_Show(ap);
	set_tt_quit_timer(FALSE);

	switch (type) {
	case contents:
		DBGOUT((2, "dstt: got data in contents buffer\n"));

		if (title) {
		}

		if (aobj = gtt_create_buffer(ap, data, size)) {
			if (AudPanel_InsertFromBufferList(ap,
				  &aobj, 1, TRUE, FALSE, TRUE) == TRUE) {
				break;
			}
		}

		/* failed if we got this far */
		rc = FAIL;
		break;
	case path:
		DBGOUT((2,"dstt: got data in file %s\n", data));
		if (!AudPanel_Loadfile(ap, data, TRUE)) {
			return (FAIL);
		}
		break;
	case x_selection:
		DBGOUT((2,"dstt: load from x_selection %s\n", data));
		init_selection_request(asdp, S_TTLoad /* S_TTDisplay */,
		    data, FALSE);
		break;
	}

	/* send ok status to caller */
	if (rc == HOLD) {
		dstt_status(tt_message_sender(m),
		    (char*)dstt_set_status(0, dstt_status_req_rec), msg,
		    setlocale(LC_CTYPE, NULL));
		ap->link_m = m;
		ap->link_msg = strdup(msg);
		ap->link_data = type;
	}
	return (rc);
}


status_t
gtt_Edit(Tt_message m, char * key, Data_t type, void *data, int size, 
	 char *msg, char *title)
{
	char		*host, *file;
	int		len;
	Menu_item	load_item;
	char		*status_msg;
	status_t	rc = HOLD;
	AudioSelData	*asdp;
	Audio_Object	aobj;
	struct atool_panel_data	*ap;

	DBGOUT((1,"dstt: executing callback Edit \n"));

#ifdef	NOTDEF
	/* Guard against bogus calls */
	if (strcmp(key, TOOLTALK_NAME) != 0)
		return (REJECT);
#endif

	/* turn off quit timer if running */
	set_tt_quit_timer(FALSE);

	/* XXX - do we return FAIL or REJECT? */

	asdp = (AudioSelData*) xv_get(frame, XV_KEY_DATA, AUD_SEL_DATA_KEY);
	if (asdp == NULL) {
		DBGOUT((D_DND, "dstt: can't get seln data\n"));
		return (FAIL);
	}
	ap = (struct atool_panel_data *) asdp->ap;

	if (ap->tstate != Unloaded) {
		DBGOUT((D_DND, "dstt: rejected edit, contains data.\n"));
		return (REJECT);
	}

	if (AudPanel_Getlink(ap) != NULL) {
		DBGOUT((D_DND, "dstt: rejected edit, already linked.\n"));
		return (REJECT);
	}

	AudPanel_Show(ap);
	set_tt_quit_timer(FALSE);

	switch (type) {
	case contents:
		DBGOUT((2, "dstt: got data in contents buffer\n"));

		if (title) {
		}

		if (aobj = gtt_create_buffer(ap, data, size)) {
			if (AudPanel_InsertFromBufferList(ap,
				  &aobj, 1, TRUE, FALSE, TRUE) == TRUE) {
				break;
			}
		}
		/* failed if we got this far */
		rc = FAIL;
		break;
	case path:
		DBGOUT((2,"dstt: got data in file %s\n", data));
		if (!AudPanel_Loadfile(ap, data, FALSE)) {
			return (FAIL);
		}
		break;
	case x_selection:
		DBGOUT((2,"dstt: load from x_selection %s\n", data));
		init_selection_request(asdp, S_TTLoad, data, FALSE);
		break;
	}

	/* send ok status to caller */
	if (rc == HOLD) {
		dstt_status(tt_message_sender(m),
		    (char*)dstt_set_status(0, dstt_status_req_rec), msg,
		    setlocale(LC_CTYPE, NULL));
		ap->link_m = m;
		ap->link_msg = strdup(msg);
		ap->link_data = type;
	}
	return (rc);
}

/* Called when a link is broken */
void
gtt_Breaklink(
	struct atool_panel_data	*ap)
{
	/* If there's an outstanding link, tell the sender we're done with it */
	if (ap->link_msg != NULL) {
		dstt_message_return(ap->link_m, NULL, NULL);
		(void) free(ap->link_msg);
		ap->link_msg = NULL;
	}
}

/* Called to save data back to invoking application */
void
gtt_Saveback(
	struct atool_panel_data	*ap,
	Audio_Object		aobj,
	int			done)
{
	switch (ap->link_data) {
	case contents:
		/* XXX - not handled yet!! */
	case path:
		/* XXX - not handled yet!! */
		AudPanel_Alert(ap, MGET(
	    "Saveback of this ToolTalk data type is not implemented yet."));
		break;

	case x_selection:
		Audio_SAVEBACK((Xv_opaque)ap->panel, aobj, done);
		return;
	default:
		DBGOUT((2, "tooltalk saveback unknown type: %d\n",
		    ap->link_data));
		break;
	}
	if (done) {
		AudPanel_Breaklink(ap);
	}
}


/* This is the entry called when tooltalk finds the requested process */
void
status_handler(Tt_message m, char *status, char *vendor, char *toolname,
		char *version, char *msgid, char *domain)
{
	char *cp;

	DBGOUT((1, "atool_dstt: status_handler status=%s, toolname=%s\n",
		status ? status : "(NULL)",
		toolname ? toolname : "(NULL)"));

	/* if we get this far - we're successful */
	launching = FALSE;

	if (cp = tt_message_sender(m)) {
		if (toolid != NULL)
			(void) free(toolid);
		toolid = strdup(cp);
		DBGOUT((2, "toolid of audiocontrol process: %s\n", toolid));
	}

	/* If any pending launches, start them now */
	if (pending & Launch_Play) {
		(void) ttstart_play(Play_dp, Play_devname, Play_label);
	} else if (pending & Launch_Record) {
		(void) ttstart_record(Record_dp, Record_devname, Record_label);
	} else {
		/* XXX - there has to be a better way */
		AudioSelData	*asdp = (AudioSelData*)
		    xv_get(frame, XV_KEY_DATA, AUD_SEL_DATA_KEY);

		if (asdp == NULL) {
			DBGOUT((D_DND, "status_handler: can't get seln\n"));
			return;
		}
		AudPanel_Message(asdp->ap, NULL);
	}
}

atool_dstt_start(Xv_opaque window)
{
	DBGOUT((1, "atool_dstt_start called\n"));

	/* store for later use */
	frame = window;

	init_tt_keys(window);	/* for quit timer */

	dstt_xview_desktop_callback(window,
		"Quit",		gtt_Quit,
		NULL);

	dstt_xview_desktop_register(window,
		"Quit",		TRUE,
		NULL);

	dstt_editor_callback(TOOLTALK_NAME,
		"Display",	gtt_Display,
		"Edit",		gtt_Edit,
		NULL);

	dstt_editor_register(TOOLTALK_NAME,
		"Display",	TRUE,
		"Edit",		TRUE,
		NULL);

	dstt_notice_callback((ptr_t)window, STATUS, status_handler, NULL);
	dstt_notice_register((ptr_t)window, STATUS, TRUE, NULL);
	dstt_xview_start_notifier();
	return (0);
}

int
audioctl_callback(Tt_message m, void *key, int rcode, char *prop, 
		  void *data, int size, char *msg, char *title)
{
	int state;

	state = tt_message_state (m);

	DBGOUT((1, "audioctl_callback called, state = %d\n", state));

	switch (state) {
	case TT_STARTED:
		AudPanel_Message(key, MGET("Launching Audio Control..."));
		launching = TRUE;
		/* Busy frame and display waiting message */
		/* XXX - should start a timer in case of tooltalk errors */
		/* AudPanel_Busy(key); */
		break;
	case TT_HANDLED:
		/* unbusy frame */
		/* AudPanel_Unbusy(key); */
		launching = FALSE;
		AudPanel_Message(key, NULL);
		break;
	case TT_FAILED:
	case TT_REJECTED:
	default:
		/* unbusy frame and put msg in footer */
		/* AudPanel_Unbusy(key); */
		if (launching) {
			AudPanel_Errormessage(key,
			    MGET("Error starting Audio Control"));
			launching = FALSE;
		}
		/* just in case ... */
		/* AudPanel_Unbusy(key); */
		break;
	}
	return (OK);
}

/* Start play volume panel.  If busy, set a flag to start when ready. */
int
ttstart_play(ptr_t dp, char *devname, char *label)
{
	if (!launching) {
		dstt_audioctl(audioctl_callback,
		    dp, "Play_Control", NULL, 0, devname, NULL, label);
		pending &= ~Launch_Play;
	} else {
		pending |= Launch_Play;
		Play_dp = dp;
		Play_devname = devname;
		Play_label = label;
	}
	return (0);
}

/* Start record volume panel.  If busy, set a flag to start when ready. */
int
ttstart_record(ptr_t dp, char *devname, char *label)
{
	if (launching == FALSE) {
		dstt_audioctl(audioctl_callback,
		    dp, "Record_Control", NULL, 0, devname, NULL, label);
		pending &= ~Launch_Record;
	} else {
		pending |= Launch_Record;
		Record_dp = dp;
		Record_devname = devname;
		Record_label = label;
	}
	return (0);
}

quit_audioctl_callback(Tt_message m, void *key, int status, int silent, 
	       int force, char *msgid)
{

	DBGOUT((2, "quit_audioctl_callback, status = %d\n", status));
} 

dstt_send_quit()
{
	if (toolid) {
		DBGOUT((2, "sending quit msg to audiocontol, id = %s\n", 
			toolid));
		dstt_quit(quit_audioctl_callback, "audiotool", toolid,
		    TRUE, FALSE, NULL);
	}
}

#endif /* !PRE_493 */
