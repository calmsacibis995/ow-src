/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)audioctl_dstt.cc	1.12	97/01/14 SMI"

#ifndef PRE_493

#include <xview/xview.h>	// ds_popup.h depends on these
#include <xview/panel.h>

#include <desktop/tt_c.h>
#include "ds_popup.h"
#include "dstt.h"
#include "dstt_audio.h"
#include "audio_i18n.h"

#include "audiocontrol.h"
#include "audioctl.h"

static int play_mapped = FALSE;	// XXX - tmp hack
static int record_mapped = FALSE;

extern "C" {
extern char *dstt_set_status(int, int);
};

extern "C" {
// gets called after sending geom request to caller. this is used to
// to position ourself relative to the caller.
int
gtt_geometry_callback(
	Tt_message	/*ttmsg*/,
	void*		key,
	int		/*status*/, 
	int		w,
	int		h,
	int		x,
	int		y)
{
	Rect   		rect;
	Audioctl*	panel;
	Frame		frame;
   
	rect.r_width = w;
	rect.r_height = h;
	rect.r_left = x;
	rect.r_top = y;

	panel = (Audioctl*) key;
	frame = (Frame)panel->Gethandle();

	ds_position_popup_rect(&rect, frame, DS_POPUP_LOR);

	(void) panel->Setpositioned(TRUE);
	panel->Show();

	return (0);
}

status_t 
gtt_show_playctl(
	Tt_message	m,
	char*		/*prop*/,
	void*		/*data*/,
	int		/*size*/,
	char*		devname,
	char*		/*msg*/,
	char*		/*title*/)
{
	int was_sleeping; 

	if (was_sleeping = Audioctl_set_quit_timer(FALSE)) {
		play_mapped = record_mapped = FALSE;
	}

	if (Audioctl_tt_started() && (play_mapped == FALSE)) {
		// the geom callback will take care of showing the panel
		dstt_get_geometry(gtt_geometry_callback, 
				  Audioctl_get_playpanel(),
				  tt_message_sender(m),
				  NULL, NULL);
		play_mapped = TRUE;
	} else {
		Audioctl_show_playpanel();
	}

	if (devname && *devname) {
		Audioctl_set_device(devname);
	}

	// send ok status to caller
	dstt_status(tt_message_sender(m), 
		    (char*)dstt_set_status(0, dstt_status_req_rec),
		    NULL, NULL);

	return (OK);
}

status_t 
gtt_show_reclevel(
	Tt_message	m,
	char*		/*prop*/,
	void*		/*data*/,
	int		/*size*/,
	char*		devname,
	char*		/*msg*/,
	char*		/*title*/)
{
	int was_sleeping;

	if (was_sleeping = Audioctl_set_quit_timer(FALSE)) {
		play_mapped = record_mapped = FALSE;
	}

	// open the base frame (playpanel) if it is iconic.
	Audioctl_open_playpanel();

	if (Audioctl_tt_started() && (record_mapped == FALSE)) {
		dstt_get_geometry(gtt_geometry_callback, 
				  Audioctl_get_recpanel(),
				  tt_message_sender(m), 
				  NULL, NULL);
		record_mapped = TRUE;
	} else {
		Audioctl_show_recpanel();
	}

	if (devname && *devname) {
		Audioctl_set_device(devname);
	}

	// send ok status to caller
	dstt_status(tt_message_sender(m), 
		    (char*)dstt_set_status(0, dstt_status_req_rec),
		    NULL, NULL);

	return (OK);
}

status_t
gtt_Quit(
	Tt_message	/*m*/,
	int		/*silent*/,
	int		/*force*/,
	char*		/*msg*/)
{
	Audioctl_dstt_quit();

	return (OK);
}

};


// register various callbacks. 
audiocontrol_dstt_start(
	caddr_t		window)
{
	dstt_xview_desktop_callback((Xv_opaque)window,
		"Quit",	gtt_Quit,
		NULL);

	dstt_xview_desktop_register((Xv_opaque)window,
		"Quit",	TRUE,
		NULL);

	dstt_audioctl_callback("Play_Control",
			       AUDIO_CONTROL, gtt_show_playctl,
			       NULL);

	dstt_audioctl_register("Play_Control",
			       AUDIO_CONTROL, TRUE,
			       NULL);

	dstt_audioctl_callback("Record_Control",
			       AUDIO_CONTROL, gtt_show_reclevel,
			       NULL);

	dstt_audioctl_register("Record_Control",
			       AUDIO_CONTROL, TRUE,
			       NULL);

	dstt_xview_start_notifier();

	return(0);
}

void
print_tt_emsg(
	char*		msg,
	Tt_status	rcode)
{
	char*		s;

	/*
	 * Print out a tool talk error message.  Currently this is used
	 * mostly for debugging.  We need to come up with a formal way to
	 * handle this.
	 */
	switch (rcode) {
	case TT_OK:
		s = MGET("TT_OK (%d): Call successful");
		break;
	case TT_ERR_CLASS:
		s = MGET("TT_ERR_CLASS (%d): Tt_class value is invalid");
		break;
	case TT_ERR_DBAVAIL:
		s = MGET("TT_ERR_DBAVAIL (%d): Cannot access database");
		break;
	case TT_ERR_DBEXIST:
		s = MGET("TT_ERR_DBEXIST (%d): Database does not exist");
		break;
	case TT_ERR_FILE:
		s = MGET("TT_ERR_FILE (%d): Cannot create or access file");
		break;
	case TT_ERR_INVALID:
		s = MGET("TT_ERR_INVALID (%d): Invalid link");
		break;
	case TT_ERR_MODE:
		s = MGET("TT_ERR_MODE (%d): Invalid Tt_mode");
		break;
	case TT_ERR_NOMP:
		s = MGET("TT_ERR_NOMP (%d): ttsession is not running -- is Tool Talk installed?");
		break;
	case TT_ERR_NOTHANDLER:
		s = MGET("TT_ERR_NOTHANDLER (%d): Action is allowed for handler only ");
		break;
	case TT_ERR_NUM:
		s = MGET("TT_ERR_NUM (%d): Integer value is wildly out of range");
		break;
	/*
	case TT_ERR_OBJID:
		s = MGET("TT_ERR_OBJID (%d): Object ID does not refer to an existing object ");
		break;
	*/
	case TT_ERR_OP:
		s = MGET("TT_ERR_OP (%d): Syntactically invalid operation name");
		break;
	case TT_ERR_OTYPE:
		s = MGET("TT_ERR_OTYPE (%d): Undefined object type");
		break;
	/*
	case TT_ERR_ADDRESS:
		s = MGET("TT_ERR_ADDRESS (%d): Invalid Tt_address");
		break;
	*/
	case TT_ERR_PATH:
		s = MGET("TT_ERR_PATH (%d): Invalid path");
		break;
	case TT_ERR_POINTER:
		s = MGET("TT_ERR_POINTER (%d): Invalid pointer");
		break;
	/*
	case TT_ERR_PROCID:
		s = MGET("TT_ERR_PROCID (%d): Invalid process id");
		break;
	*/
	case TT_ERR_PROPLEN:
		s = MGET("TT_ERR_PROPLEN (%d): Property value too long");
		break;
	case TT_ERR_PROPNAME:
		s = MGET("TT_ERR_PROPNAME (%d): Syntactically invalid property name");
		break;
	case TT_ERR_PTYPE:
		s = MGET("TT_ERR_PTYPE (%d): Undefined process type");
		break;
	case TT_ERR_READONLY:
		s = MGET("TT_ERR_READONLY (%d): Attribute is read-only");
		break;
	/*
	case TT_ERR_DISPOSITION:
		s = MGET("TT_ERR_DISPOSITION (%d): Invalid Tt_disposition");
		break;
	*/
	case TT_ERR_SCOPE:
		s = MGET("TT_ERR_SCOPE (%d): Invalid Tt_scope");
		break;
	case TT_ERR_SESSION:
		s = MGET("TT_ERR_SESSION (%d): Session ID is not the name of an active session");
		break;
	case TT_ERR_VTYPE:
		s = MGET("TT_ERR_VTYPE (%d): Invalid value type name");
		break;
	case TT_ERR_NO_VALUE:
		s = MGET("TT_ERR_NO_VALUE (%d): No such property value");
		break;
	/*
	case TT_ERR_NO_MATCH:
		s = MGET("TT_ERR_NO_MATCH (%d): Could not find handler for message");
		break;
	*/
	case TT_WRN_NOTFOUND:
		s = MGET("TT_WRN_NOTFOUND (%d): Object does not exist");
		break;
	case TT_WRN_STOPPED:
		s = MGET("TT_WRN_STOPPED (%d): Query halted by fileter procedure");
		break;
	/*
	case TT_WRN_STALE_OBJID:
		s = MGET("TT_WRN_STALE_OBJID (%d): Stale object id");
		break;
	case TT_WRN_SAME_OBJID:
		s = MGET("TT_WRN_SAME_OBJID (%d): Same object id");
		break;
	*/
	default:
		s = MGET("Unknown error code %d");
		break;

	}

    	fprintf(stderr, "%s: %s: ", MGET("Audio Control"), msg);
	fprintf(stderr, s, rcode);
    	fprintf(stderr, "\n");
}

#endif /* !PRE_493 */
