/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)atool_sel_xv.c	1.57	93/03/03 SMI"

/* Selection service and Drag N Drop code (XView interface) */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/utsname.h>
#include <values.h>

#include <xview/xview.h>
#include <xview/xv_xrect.h>
#include <xview/cursor.h>
#include <xview/svrimage.h>
#include <xview/server.h>
#include <xview/panel.h>

#include "segment/segment_canvas.h"

#include "atool_panel.h"
#include "atool_panel_impl.h"
#include "atool_sel_impl.h"
#include "atool_i18n.h"
#include "atool_debug.h"

/* extern stuff not in .h files */
extern Audio_Object AudPanel_GetSelData(ptr_t atd, int all, int shelf);

Attr_attribute	AUD_SEL_DATA_KEY; /* global key for audio sel data storage */

/* we'll use enum names to refer to atom's */

static AtomListEntry AtomList[] = {
    /* the only time we get a reply on MULTIPLE is upon error. */
    A_MULTIPLE,			"MULTIPLE", 			0, 
    NULL,			sel_repl_multiple,

    A_TIMESTAMP,		"TIMESTAMP", 			0, NULL, NULL,

    /* some "standard" targets that we should convert */
    A_DELETE,			"DELETE", 			0, 	
    sel_cvt_delete,		sel_repl_delete,

    A_FILE_NAME,		"FILE_NAME", 			0, 
    0, /* sel_cvt_filename, */	sel_repl_filename,

    A_HOST_NAME,		"HOST_NAME", 			0,
    sel_cvt_hostname,		sel_repl_hostname,

    A_INCR,			"INCR",				0, NULL, NULL,

    /* all related to LOAD target */
    A_SUN_LOAD,			"_SUN_LOAD", 			0,
    sel_cvt_load,		sel_repl_load,

    A_SUN_COMPOSE_TYPE,		"_SUN_COMPOSE_TYPE",		0,
    0,				sel_repl_compose_type,

    A_NAME,			"NAME",				0,
    sel_cvt_name,		sel_repl_name,

    A_INSERT_SELN,		"INSERT_SELECTION", 		0, 
    sel_cvt_insert,		sel_repl_insert,

    A_SUN_SEL_END,		"_SUN_SELECTION_END",		0,
    sel_cvt_sel_end,		sel_repl_sel_end,

    A_SUN_SEL_ERROR,		"_SUN_SELECTION_ERROR",		0, 
    sel_cvt_sel_error,		sel_repl_sel_error,

    /* targets for begin/end & error of sel xfer */
    A_SUN_DND_BEGIN,		"_SUN_DRAGDROP_BEGIN",		0, 
    sel_cvt_dnd_begin,		sel_repl_dnd_begin,

    A_SUN_DND_DONE,		"_SUN_DRAGDROP_DONE",		0,
    sel_cvt_dnd_done,		sel_repl_dnd_done,
    
    /* don't cvt this 'cause it's buried in DnD code */
    A_SUN_DND_ERROR,		"_SUN_DRAGDROP_ERROR",		0,
    sel_cvt_dnd_error,		sel_repl_dnd_error,

    /* returns lists of atoms */
    A_TARGETS,			"TARGETS",			0,
    sel_cvt_targets,		sel_repl_targets,

    A_SUN_AVAIL_TYPES,		"_SUN_AVAILABLE_TYPES", 	0, 
    sel_cvt_types,		sel_repl_types,

    A_SUN_ATMS,			"_SUN_ALTERNATE_TRANSPORT_METHODS", 0, 
    sel_cvt_atms,		sel_repl_atms,

    /* our supporter alternate transport mechanisms. these don't get
     * converted, just referenced.
     */
    A_SUN_ATM_FNAME,		"_SUN_ATM_FILE_NAME",		0, NULL, NULL,

    /* recognize this but don't actually it support for V3 FCS */
    A_SUN_ATM_TCP_SOCKET,	"_SUN_ATM_TCP_SOCKET",		0,
#ifdef USE_ATM_TCP
    sel_cvt_atm_tcp_socket,	sel_repl_atm_tcp_socket,
#else
    0,				0,
#endif

    /* the type of data we support */
    A_SUN_TYPE_AUDIO,		"_SUN_TYPE_audio-file",	0, 
    sel_cvt_type_audio,		sel_repl_type_audio,

    /* convert TEXT & STRING type but treat as audio */
    A_TEXT,			"TEXT",				0,
    sel_cvt_type_audio,		sel_repl_type_audio,

    A_STRING,			"STRING", 			0,
    sel_cvt_type_audio,		sel_repl_type_audio,

    /* other types (but we don't cvt these) */
    A_INTEGER,			"INTEGER",			0, NULL, NULL,
    A_ATOM_PAIR,		"ATOM_PAIR",			0, NULL, NULL,
    
    /* for now treat this like Sun length (actually, vice versa) */
    A_LENGTH,			"LENGTH",		0,	
    sel_cvt_sun_length,		sel_repl_sun_length,

    /* other things that requestor can ask us to convert  */
    A_SUN_DATA_LABEL,		"_SUN_DATA_LABEL",		0,
    sel_cvt_label,		sel_repl_label,
    
    A_SUN_FILE_HNAME,		"_SUN_FILE_HOST_NAME",		0, 
    sel_cvt_hostname,		sel_repl_hostname,

    A_SUN_LENGTH_TYPE,		"_SUN_LENGTH_TYPE",		0,	
    sel_cvt_sun_length,		sel_repl_sun_length,

    A_SUN_ENUM_COUNT,		"_SUN_ENUMERATION_COUNT", 	0, 
    sel_cvt_enum_ct,		sel_repl_enum_ct,

    A_SUN_ENUM_ITEM,		"_SUN_ENUMERATION_ITEM",	0, 
    sel_cvt_enum_item,		sel_repl_enum_item,

    /* the name of the primary AUDIO selection */
#ifdef notdef
    A_AUDIO_PRIMARY_SEL,	"_SUN_AUDIO_PRIMARY_SELECTION", 0, NULL, NULL,
#else
    A_AUDIO_PRIMARY_SEL,	"PRIMARY",			0, NULL, NULL,
#endif

    A_CLIPBOARD,		"CLIPBOARD",			0, NULL, NULL,

    A_NONE,			NULL, 0, NULL, NULL, /* mark end of list */
};

/* list of AtomIndex's representing types we support (we'll create
 * the Atom list after we've filled in the AtomList during init time)
 */

static AtomIndex supported_atms[] =
{
#ifdef USE_ATM_TCP
	A_SUN_ATM_TCP_SOCKET, 
#endif
	NULL,
};

static int	num_supported_atms =
		    (sizeof (supported_atms)/sizeof (AtomIndex)) - 1;

static AtomIndex supported_targets[] =
{
	A_DELETE, A_FILE_NAME, A_HOST_NAME, A_NAME, A_SUN_SEL_END,
	A_TARGETS, A_SUN_DND_BEGIN, A_SUN_DND_DONE, A_SUN_DND_ERROR,
	A_SUN_TYPE_AUDIO, A_LENGTH, A_TEXT,
	A_SUN_AVAIL_TYPES, A_SUN_ATMS,
	A_SUN_DATA_LABEL, A_SUN_ENUM_COUNT, A_SUN_LENGTH_TYPE, 
	A_SUN_ENUM_ITEM, A_SUN_DATA_LABEL, A_SUN_FILE_HNAME, A_SUN_SEL_ERROR,
#ifdef USE_ATM_TCP
	A_SUN_ATM_TCP_SOCKET,
#endif
	NULL,
};
static int	num_supported_targets =
		    (sizeof (supported_targets)/sizeof (AtomIndex)) - 1;

static AtomIndex supported_types[] =
{
	A_SUN_TYPE_AUDIO, 
#ifdef notdef
	/* XXX - do we ever want to support "generic" data x-fer? */
	A_TEXT, 
	A_STRING, 
#endif
	NULL,
};

static int	num_supported_types =
		    (sizeof (supported_types)/sizeof (AtomIndex)) - 1;

/* Initialize the audiotool selection service/drag 'n drop handling code. */

/* got to look at the icon files to know these */
#define C_HEIGHT	48
#define C_WIDTH		48

void Audio_Sel_INIT(
	ptr_t		ap,		/* audio data ptr */
	Xv_opaque	frame,		/* frame */
	Xv_opaque	canvas,		/* segment canvas */
	Xv_opaque	dropsite)	/* panel item for load drop site */
{
	AudioSelData	*asdp;		/* audio selection data ptr */
	Display		*dpy;
	Xv_server	server;
	Rect		*rect;
	Atom		*alist;
	int		i;
	struct utsname	utn;

static unsigned short drag_cursor_bits[] = {
#include "icon/dupesound_drag.icon"
};

static unsigned short drop_cursor_bits[] = {
#include "icon/dupesound_insert.icon"
};

	/* 
	 * XXX - NOTE: the only way to figure this out is to go in to
	 * iconedit and eyeball it!!!
	 */
#define DRAG_XHOT	10
#define DRAG_YHOT	10

#define DROP_XHOT	10
#define DROP_YHOT	10

	Xv_Cursor	drop_cursor,
			drag_cursor;
	Server_image	drag_image,
			drop_image;

	/* first alloc the audio selection data struct */
	if (!(asdp = (AudioSelData*) calloc(1, sizeof (AudioSelData)))) {
		DBGOUT((D_DND,
		    "Audio_Sel_INIT: can't calloc selection data\n"));
		return;
	}

	asdp->dpy = dpy = (Display *)xv_get(canvas, XV_DISPLAY);
	asdp->server = XV_SERVER_FROM_WINDOW(canvas);

	/* create various atom's (we want them cached) */
	init_atoms(asdp->server, AtomList);
	asdp->atoms = AtomList;

	/* create atom lists for responses to various queries */
	alist = (Atom *)calloc(sizeof (Atom), num_supported_types+1);
	asdp->supported_types = alist;
	for (i=0; i<num_supported_types+1; i++, alist++)
	    *alist = atom_get_atom(AtomList, supported_types[i]);
	asdp->num_sup_types = num_supported_types;

	alist = (Atom *)calloc(sizeof (Atom), num_supported_atms+1);
	asdp->supported_atms = alist;
	for (i=0; i<num_supported_atms+1; i++, alist++)
	    *alist = atom_get_atom(AtomList, supported_atms[i]);
	asdp->num_sup_atms = num_supported_atms;

	alist = (Atom *)calloc(sizeof (Atom), num_supported_targets+1);
	asdp->supported_targets = alist;
	for (i=0; i<num_supported_targets+1; i++, alist++)
	    *alist = atom_get_atom(AtomList, supported_targets[i]);
	asdp->num_sup_targets = num_supported_targets;

	/* create key and attach audio sel data to xview objects */
	AUD_SEL_DATA_KEY = xv_unique_key();

	xv_set(frame, XV_KEY_DATA, AUD_SEL_DATA_KEY, asdp, NULL);
	xv_set(canvas, XV_KEY_DATA, AUD_SEL_DATA_KEY, asdp, NULL);
	xv_set(dropsite,  XV_KEY_DATA, AUD_SEL_DATA_KEY, asdp, NULL);
	xv_set(xv_get(dropsite, XV_OWNER), 
	       XV_KEY_DATA, AUD_SEL_DATA_KEY, asdp, NULL);
	
	/* init selection data */
	asdp->ap = ap;
	asdp->canvas = canvas;
	asdp->dropsite = dropsite;

	/* get and store our host name */
#ifndef SUNOS41
	if (uname(&utn) != 0) {
		strncpy(asdp->this_host, utn.nodename, MAXHOSTNAMELEN);
	} else
#else /* 4.x */
	/* XXX - uname is SVr4 only */
	if (gethostname(asdp->this_host, MAXHOSTNAMELEN))
#endif /* 4.x */
	{
		DBGOUT((1, "Cannot initialize selection service\n"));
		return;
	}

#ifdef USE_ATM_TCP
	/* init the ATM TCP socket support */
	audio_atm_tcp_socket_init(asdp);
#endif

	/* create selection owner items for things we'll use often */
	asdp->audio_primary = (Selection_owner) 
	    xv_create(canvas_paint_window(canvas), SELECTION_OWNER, 
		      SEL_CONVERT_PROC, audio_sel_convert_proc, 
		      SEL_DONE_PROC, audio_sel_done_proc,
		      SEL_LOSE_PROC, audio_sel_lose_proc,
		      SEL_RANK, atom_get_atom(asdp->atoms,A_AUDIO_PRIMARY_SEL),
		      SEL_OWN, FALSE,
		      XV_KEY_DATA, AUD_SEL_DATA_KEY, asdp,
		      NULL);
		      
	asdp->clipboard = (Selection_owner) 
	    xv_create(canvas_paint_window(canvas), SELECTION_OWNER, 
		      SEL_CONVERT_PROC, audio_sel_convert_proc, 
		      SEL_DONE_PROC, audio_sel_done_proc,
		      SEL_LOSE_PROC, audio_sel_lose_proc,
		      SEL_RANK, atom_get_atom(asdp->atoms, A_CLIPBOARD),
		      SEL_OWN, FALSE,
		      XV_KEY_DATA, AUD_SEL_DATA_KEY, asdp,
		      NULL);

	/* create the selection, set the SEL_RANK when we need it */
	asdp->insert_sel = (Selection_owner) 
	    xv_create(canvas_paint_window(canvas), SELECTION_OWNER, 
		      SEL_CONVERT_PROC, audio_sel_convert_proc, 
		      SEL_DONE_PROC, audio_sel_done_proc,
		      SEL_LOSE_PROC, audio_sel_lose_proc,
		      SEL_OWN, FALSE,
		      XV_KEY_DATA, AUD_SEL_DATA_KEY, asdp,
		      NULL);

	/* create cursors for drop sites and dragndrop item */
	drag_image = (Server_image)xv_create(XV_NULL, SERVER_IMAGE,
				XV_WIDTH, C_WIDTH,
				XV_HEIGHT, C_HEIGHT,
				SERVER_IMAGE_BITS, drag_cursor_bits,
				NULL);

#ifdef notdef
	drag_mask_image = (Server_image)xv_create(XV_NULL, SERVER_IMAGE,
				XV_WIDTH, C_WIDTH,
				XV_HEIGHT, C_HEIGHT,
				SERVER_IMAGE_BITS, drag_cursor_mask_bits,
				NULL);
#endif

	drop_image = (Server_image)xv_create(XV_NULL, SERVER_IMAGE,
				XV_WIDTH, C_WIDTH,
				XV_HEIGHT, C_HEIGHT,
				SERVER_IMAGE_BITS, drop_cursor_bits,
				NULL);

#ifdef notdef
	drop_mask_image = (Server_image)xv_create(XV_NULL, SERVER_IMAGE,
				XV_WIDTH, C_WIDTH,
				XV_HEIGHT, C_HEIGHT,
				SERVER_IMAGE_BITS, drop_cursor_mask_bits,
				NULL);
#endif

	drag_cursor = (Xv_cursor)xv_create(XV_NULL, CURSOR,
				CURSOR_IMAGE, drag_image,
				CURSOR_XHOT, DRAG_XHOT,
				CURSOR_YHOT, DRAG_YHOT,
                                CURSOR_OP, PIX_SRC^PIX_DST,
				NULL);

	drop_cursor = (Xv_cursor)xv_create(XV_NULL, CURSOR,
				CURSOR_IMAGE, drop_image,
				CURSOR_XHOT, DROP_XHOT,
				CURSOR_YHOT, DROP_YHOT,
                                CURSOR_OP, PIX_SRC^PIX_DST,
				NULL);

	/* create drag 'n drop item */
	asdp->dnd_item = (Dnd) xv_create(canvas_paint_window(canvas),
			DRAGDROP,
			DND_TYPE,		DND_COPY, /* default */
			DND_CURSOR,		drag_cursor,
			DND_ACCEPT_CURSOR, 	drop_cursor,
			SEL_CONVERT_PROC,	audio_sel_convert_proc,
			SEL_DONE_PROC,		audio_sel_done_proc,
			SEL_LOSE_PROC,		audio_sel_lose_proc,
		        XV_KEY_DATA,		AUD_SEL_DATA_KEY, asdp,
			NULL);

	/* create the drop sites. */
	asdp->canvas_site = xv_create(canvas_paint_window(canvas),
			DROP_SITE_ITEM,
			DROP_SITE_ID,   	CANVAS_SITE,
			DROP_SITE_EVENT_MASK,
			    DND_MOTION | DND_ENTERLEAVE,
			DROP_SITE_REGION, 
			    (Rect *)xv_get(canvas, CANVAS_VIEWABLE_RECT,
				   canvas_paint_window(canvas)),
		        XV_KEY_DATA, AUD_SEL_DATA_KEY, asdp,
			0);

	/* set up drop item */

	xv_set(dropsite, 
	       PANEL_DROP_DND, asdp->dnd_item,
	       PANEL_DROP_SITE_DEFAULT, TRUE,
	       /* PANEL_DROP_SEL_REQ, asdp->sel_req, */
	       PANEL_NOTIFY_PROC, panel_dnd_event_proc,
	       NULL);

	/* use the one on the dropsite, and pray it doesn't change */
	asdp->sel_req = xv_get(dropsite, PANEL_DROP_SEL_REQ);
	xv_set(asdp->sel_req, 
	       SEL_REPLY_PROC, audio_sel_reply_proc,
	       XV_KEY_DATA, AUD_SEL_DATA_KEY, asdp,
	       NULL);
	asdp->panel_site = NULL;

	/* init the rest of the AudioSelData */
	sel_clear_data(asdp);
}

/* Got a drop event.  initiate the selection transfer */
static void
start_drop(
	AudioSelData	*asdp,
	Event		*event,
	SelXferType	stype)
{
	char		*rank;

	/* If drag to ourself, flag it and send event handler will clear */
	if (dnd_is_local(event)) {
		asdp->flags.is_local = 1;
		AudPanel_Message(asdp->ap, MGET("Data transfer cancelled"));
		return;
	}
	/* ignore if we're busy in another DnD */
	if (is_dropsite_busy(asdp)) {
		DBGOUT((D_DND, "start_drop: busy doing another DnD\n"));
		return;
	}
	if (rank = (char *) xv_get(asdp->sel_req, SEL_RANK_NAME)) {
		DBGOUT((D_DND, "start_drop: using SEL_RANK %s\n", rank));
	} else {
		DBGOUT((D_DND, "start_drop: can't get SEL_RANK_NAME\n"));
	}
	init_selection_request(asdp, stype, rank, dnd_is_forwarded(event));
}

/*
 * Event proc for DnD related events for the drop site
 */
Notify_value
panel_dnd_event_proc(
	Panel_item	obj,
	Xv_opaque	value,
	Event		*event)
{
	AudioSelData	*asdp;

	asdp = (AudioSelData*) xv_get(obj, XV_KEY_DATA, AUD_SEL_DATA_KEY);
	if (asdp == NULL) {
		DBGOUT((D_DND, "panel_dnd_event_proc: can't get asdp\n"));
		return (XV_ERROR);
	}
	switch (event_action(event)) {
	case LOC_DRAG:
		switch (value) {
		case XV_OK:
			/* snarf the data */
			if (get_audio_selection_data(asdp, 1, 0, NULL) !=
			    True) {
				/* XXX - should be footer msg? */
				DBGOUT((D_DND,
				    "no audio data, drop cancelled\n"));
				sel_cancel_xfer(asdp);
				return (XV_ERROR);
			}

			if (!asdp->flags.is_local) {
				AudPanel_Message(asdp->ap,
				    MGET("Sending data..."));
				break;
			} else {
				/* Drop on same window */
				sel_cancel_xfer(asdp);	/* fall through */
			}
		case DND_TIMEOUT:			/* Operation timeout */
			/*
			 * Drag from dropsite & drop on own segment;
			 * Drag from dropsite & drop on non-receiver
			 */
		case DND_ROOT:				/* Root window */
			/* Drag from dropsite to root window */
		case DND_ABORTED:			/* Operation aborted */
			AudPanel_Message(asdp->ap,
			    MGET("Data transfer cancelled"));
			break;

		case DND_ILLEGAL_TARGET:		/* Drop not accepted */
		case DND_SELECTION:			/* Bad selection */
		case XV_ERROR:				/* Failed */
		default:
			AudPanel_Message(asdp->ap,
			    MGET("Data transfer failed"));
			break;
		}
		if (value != XV_OK) {
			sel_cancel_xfer(asdp);
			return (value);
		}
		break;
	case ACTION_DRAG_COPY:
	case ACTION_DRAG_MOVE:
		/* Handle a Drop on the dropsite */
		/* XXX - for now, force this to be a copy */
		event->action = ACTION_DRAG_COPY;
		start_drop(asdp, event, S_Load);
		break;
	}
	return (XV_ERROR);	/* prevent item handler from doing anything! */
}

/* set drop site to busy */
void
set_dropsite_busy(
	AudioSelData	*asdp,
	int		flag)
{
	if (flag) {
		asdp->flags.is_busy = 1;
		xv_set(asdp->dropsite, PANEL_BUSY, TRUE, NULL);
		xv_set(asdp->canvas_site, 
		       DROP_SITE_DELETE_REGION, NULL, 0);
		AudPanel_INACTIVE_DONE_BUTTON((ptr_t)asdp->canvas);
	} else {
		asdp->flags.is_busy = 0;
		xv_set(asdp->dropsite, PANEL_BUSY, FALSE, NULL);
		xv_set(asdp->canvas_site, 
		       DROP_SITE_DELETE_REGION, NULL,
		       DROP_SITE_REGION, 
		       (Rect *)xv_get(asdp->canvas, CANVAS_VIEWABLE_RECT,
				      canvas_paint_window(asdp->canvas)),
		       0);
		AudPanel_RESET_DONE_BUTTON((ptr_t)asdp->canvas);
	}
}

is_dropsite_busy(AudioSelData *asdp)
{
	return (asdp->flags.is_busy);
}

/* Called when the canvas is resized */
void
Audio_INITDROPREGION(
	Xv_opaque	win)
{
	AudioSelData	*asdp;
	
	GET_ASDP(asdp, win, "Audio_INITDROPREGION");
	set_dropsite_busy(asdp, is_dropsite_busy(asdp));
}


void
Audio_SetDropsiteFull(Xv_opaque win, int flag)
{
	AudioSelData *asdp;

	GET_ASDP(asdp, win, "Audio_SetDropSiteFull");

	xv_set(asdp->dropsite, PANEL_DROP_FULL, flag, NULL);
}

/* reset everything */
void
sel_clear_data(AudioSelData *asdp)
{
	asdp->xfertype = NULL;

	if (asdp->hostname)
		free(asdp->hostname);
	asdp->hostname = NULL;

	if (asdp->filename)
		free(asdp->filename);
	asdp->filename = NULL;

	if (asdp->data_label)
		free(asdp->data_label);
	asdp->data_label = NULL;

	if (asdp->app_name)
		free(asdp->app_name);
	asdp->app_name = NULL;

	if (asdp->compose_type)
		free(asdp->compose_type);
	asdp->compose_type = NULL;

	if (asdp->holder_types)
		free(asdp->holder_types);
	asdp->holder_types = NULL;

	if (asdp->holder_atms)
		free(asdp->holder_atms);
	asdp->holder_atms = NULL;

	if (asdp->holder_targets)
		free(asdp->holder_targets);
	asdp->holder_targets = NULL;

	asdp->num_holder_types = 0;
	asdp->num_holder_atms = 0;
	asdp->num_holder_targets = 0;

	asdp->holder_fd = -1;
	asdp->holder_port = 0;
	asdp->holder_addr = 0;
	
	asdp->num_items = 0;
	asdp->cur_item = 0;

	asdp->atm = NULL;
	asdp->type = NULL;
	asdp->load_atom = NULL;
	asdp->insert_atom = NULL;

	asdp->flags.is_incr = 0;
	asdp->flags.sel_started = 0;
	asdp->flags.xfer_started = 0;
	asdp->flags.xfer_success = 0;
	asdp->flags.req_cancelled = 0;
	asdp->flags.is_incr = 0;
	asdp->flags.is_forwarded = 0;
	asdp->flags.is_local = 0;

	if (asdp->flags.is_insert) {
		asdp->flags.is_insert = 0;
		xv_set(asdp->insert_sel, SEL_OWN, FALSE, 0);
		/* xv_set(asdp->insert_sel, SEL_RANK, NULL, 0); */
	}
	sel_free_buffer(asdp);
	sel_free_buffer_list(asdp);
}	

/* free up the buffer used for transferring requested audio selection data */
void
sel_free_buffer(AudioSelData *asdp)
{
	/* this is a C++ Audio Object. we don't free it, we dereference it */

	if (asdp->audio_data)
		audio_dereference(asdp->audio_data);
	asdp->audio_data = NULL;

	if (asdp->audio_filehdr)
		free(asdp->audio_filehdr);
	asdp->audio_filehdr = NULL;

	if (asdp->xfer_buf)
		free(asdp->xfer_buf);
	asdp->xfer_buf = NULL;

	asdp->flags.is_incr = 0; /* get ready for next buf, may not be INCR */
	asdp->incr_size = 0;
	asdp->sel_length = 0;
	asdp->sel_position = 0;
}

int
sel_create_buffer_list(AudioSelData *asdp)
{
	sel_free_buffer_list(asdp);

	/* alloc enough for as many enum items holder claims to have*/
	if (!(asdp->audio_buffer_list = 
	      (Audio_Object*) calloc(asdp->num_items, sizeof (Audio_Object)))) {
		DBGOUT((D_DND, "can't calloc audio_buffer_list\n"));
		return (-1);
	}
	DBGOUT((D_DND, "created buffer for %d enum items\n", asdp->num_items));
	asdp->num_bufs = 0;
	return (0);
}

sel_add_buffer_to_list(AudioSelData *asdp)
{
	/* XXX - should check if buffer overflows (shouldn't happen) */
	if (asdp->audio_data && asdp->audio_buffer_list) {
		asdp->audio_buffer_list[asdp->num_bufs++] = asdp->audio_data;
		asdp->audio_data = NULL;
		
	}
	/* clean up the rest of the stuff */
	sel_free_buffer(asdp);
}

/* free up the buffer used for transferring requested audio selection data */
void
sel_free_buffer_list(AudioSelData *asdp)
{
	int		i;
	Audio_Object	ap;

	if (asdp->audio_buffer_list) {
		for (i=0; i<asdp->num_bufs; i++) {
			if (ap = asdp->audio_buffer_list[i]) {
				audio_dereference(ap);
				DBGOUT((D_DND,
				    "free'd buffer list item %d\n", i));
			}
		}
		free(asdp->audio_buffer_list);
		asdp->audio_buffer_list = NULL;
	}

	asdp->num_bufs = 0;
}

/* tell selection holder we're all done with this transfer */
void
sel_request_finished(AudioSelData *asdp)
{
	/* XXX - not sure if this really goes here */

	/* post request that selection transfer finished */
	xv_set(asdp->sel_req,
	    SEL_TYPES, atom_get_atom(asdp->atoms, A_SUN_SEL_END), 0, 0);
	(void) sel_post_req(asdp->sel_req);

	DBGOUT((D_DND, "selection request: Posted request to finish\n"));

	set_dropsite_busy(asdp, False);

	/* if we did a load, create a load context then clean up */
	if (asdp->load_atom)
		sel_create_load_context(asdp);

	sel_clear_data(asdp);

	AudPanel_Message(asdp->ap, NULL);
}

/* done doing sel transfer (seln holder) */
void
sel_xfer_finished(AudioSelData *asdp)
{
	DBGOUT((D_DND, "selection transfer: Finished\n"));
	set_dropsite_busy(asdp, False);
	sel_clear_data(asdp);
}

/* cancel an in progress seln request */
void
sel_cancel_req(AudioSelData *asdp, char *msg)
{
	/*
	 * set the req_cancelled flag. callers that don't expect more 
	 * replies will call sel_process_next_request() to send out
	 * SUN_SELECTION_ERROR and clean up. otherwise, the master
	 * reply proc will ignore requests until it's appropriate to
	 * call sel_process_next_request()
	 */
	asdp->flags.req_cancelled = 1; /* flag it here, deal with it later */
	DBGOUT((D_DND, "selection request: cancelling because %s\n", msg));
	AudPanel_Message(asdp->ap, msg);
}

/* requestor has cancelled transfer */
void
sel_cancel_xfer(AudioSelData *asdp)
{
	set_dropsite_busy(asdp, False);
	sel_clear_data(asdp);
	DBGOUT((D_DND, "selection transfer: Cancelled\n"));
}

/* assert ownership of PRIMARY seln */
void
Audio_OWN_PRIMARY_SELECTION(Xv_opaque win)
{
	AudioSelData *asdp;

	GET_ASDP(asdp, win, "Audio_OWN_PRIMARY_SELECTION");
	xv_set(asdp->audio_primary, SEL_OWN, TRUE, NULL);
	DBGOUT((D_DND, "grabbed ownership of primary AUDIO selection\n"));
}

/* assert ownership of CLIPBOARD selection */
void
Audio_OWN_CLIPBOARD(Xv_opaque win)
{
	AudioSelData *asdp;

	GET_ASDP(asdp, win, "Audio_OWN_CLIPBOARD");
	if ((int)AudPanel_Getshelf(asdp->ap) != NULL) {
		xv_set(asdp->clipboard, SEL_OWN, TRUE, NULL);
		DBGOUT((D_DND, "grabbed ownership of clipboard\n"));
	}
}

/* release ownership of CLIPBOARD selection */
void
Audio_RELEASE_CLIPBOARD(Xv_opaque win)
{
	AudioSelData *asdp;

	GET_ASDP(asdp, win, "Audio_RELEASE_CLIPBOARD");

	xv_set(asdp->clipboard, SEL_OWN, FALSE, NULL);
	AudPanel_Releaseshelf(asdp->ap);
}

/* animate drop site when there's a drag preview */
void
Audio_DRAG_PREVIEW(Xv_opaque window)
{
	AudioSelData *asdp;

	GET_ASDP(asdp, window, "Audio_DRAG_PREVIEW");

	/* XXX - need some real code here! */
	DBGOUT((D_DND, "got a DRAG_PREVIEW over %s\n",
	    (asdp->canvas == window) ? "canvas" : "dropsite"));
}

/*
 * Start sourcing of data for the drop.
 * Called from segment canvas event handler and
 * panel event handler (for the dropsite). if audio is dragged from
 * the dropsite, all audio data is sent, otherwise just
 * the current audio selection.
 */
int
Audio_SENDDROP(
	Xv_opaque	window)
{
	AudioSelData *asdp;
	int status;
	int sendall;		/* send all data, or just seln? */

	GET_ASDP(asdp, window, "Audio_SENDDROP");

	if (is_dropsite_busy(asdp)) {
		DBGOUT((D_DND, "Audio_SENDDROP: busy doing another DnD\n"));
		return;
	}

	set_dropsite_busy(asdp, True);

	/* get ourselves a copy of the selection (or all audio data) */
	sendall = (window == xv_get(asdp->dropsite, XV_OWNER));
		
	if (!get_audio_selection_data(asdp, sendall, 0, NULL)) {
		DBGOUT((D_DND, "no audio data, drag sourcing cancelled\n"));
		sel_cancel_xfer(asdp);
		return (XV_ERROR);
	}

	switch (status = dnd_send_drop(asdp->dnd_item)) {
	case XV_OK:
		if (!asdp->flags.is_local) {
			AudPanel_Message(asdp->ap,
				MGET("Sending data..."));
			break;
		} else {
			sel_cancel_xfer(asdp);	/* fall through */
		}
	case DND_TIMEOUT:
	case DND_ROOT:
	case DND_ABORTED:
		AudPanel_Message(asdp->ap,
		    MGET("Data transfer cancelled"));
		break;

	case DND_ILLEGAL_TARGET:
	case DND_SELECTION:
	case XV_ERROR:
	default:
		AudPanel_Message(asdp->ap,
		    MGET("Data transfer failed"));
		break;
	}
	/* restore drop site if it failed */
	if (status != XV_OK) {
		sel_cancel_xfer(asdp);
	}
	return (status);
}

/* Here's where we receive the drop on the canvas */
void
Audio_GETDROP(
	Xv_opaque	win,
	Event		*event)
{
	AudioSelData	*asdp;

	GET_ASDP(asdp, win, "Audio_GETDROP");
	if (dnd_decode_drop(asdp->sel_req, event) == XV_ERROR) {
		AudPanel_Message(asdp->ap, MGET("Data transfer error"));
		return;
	}
	start_drop(asdp, event, S_Drop);
}

/*
 * This routine starts a selection data receive.
 * First, post a request asking the holder for the type of data,
 * size, num items, type, etc.
 */
void
init_selection_request(
	AudioSelData	*asdp,
	SelXferType	stype,
	char		*selrankname,	/* NULL for predefined */
	int		fflag)		/* drop forwarded? */
{
static int		reset_item = SELN_RESET_ITEM;
	Atom 		selrank;
	int 		last_index;

	/* first thing to do is discard any pending loads if doing a new load */
	if (asdp->load_context) {
		if ((stype == S_Load) && (AudPanel_Getlink(asdp->ap) != NULL)) {
			char	msg[BUFSIZ];

			sprintf(msg, MGET(
			    "This operation will break the link to %s.\n"),
			    AudPanel_Getlink(asdp->ap));
			if (AudPanel_Choicenotice(asdp->ap, msg,
			    MGET("Continue"), MGET("Cancel"), NULL) == 2)
				return;
		}
		if ((stype == S_TTLoad) || (stype == S_Load)) {
			AudPanel_Breaklink(asdp->ap);
		}
	}
	/*
	 * setup stuff we want to ask all sel holders about.
	 * after some of those questions are answered, we'll snarf the data.
	 */
	xv_set(asdp->sel_req,
	       SEL_TYPES,
	           atom_get_atom(asdp->atoms, A_SUN_ENUM_COUNT),
	       	   atom_get_atom(asdp->atoms, A_SUN_ENUM_ITEM),
	           atom_get_atom(asdp->atoms, A_TARGETS),
	           atom_get_atom(asdp->atoms, A_SUN_FILE_HNAME),
	           atom_get_atom(asdp->atoms, A_FILE_NAME),
	           atom_get_atom(asdp->atoms, A_SUN_DATA_LABEL),
		   atom_get_atom(asdp->atoms, A_NAME),
	           atom_get_atom(asdp->atoms, A_SUN_COMPOSE_TYPE),
	       	   atom_get_atom(asdp->atoms, A_SUN_AVAIL_TYPES),
	           0,
	       SEL_TYPE_INDEX, 1, 		/* enum item */
		   SEL_PROP_TYPE, XA_INTEGER,
		   SEL_PROP_DATA, &(asdp->cur_item),
		   SEL_PROP_FORMAT, 32,
		   SEL_PROP_LENGTH, 1,
	       SEL_RANK, XA_PRIMARY, /* by default */
	       NULL);

	/* NOTE: set to index of LAST item in SEL_TYPES list. increment as
	 * we add new stuff to list. yes this looks bogus, but there's
	 * no better way (really ;-).
	 */
	last_index = 8;

	/* if this is a drop, a selection atom will already be
	 * associated with this
	 */
	if (stype == S_Paste) {
		DBGOUT((D_DND, "about to post request for clipboard paste\n"));
		xv_set(asdp->sel_req, 
		       SEL_RANK, atom_get_atom(asdp->atoms, A_CLIPBOARD),
		       NULL);
	} else {
		switch (stype) {
		case S_TTLoad:
		case S_Load:
			/* append LOAD item to request */
			xv_set(asdp->sel_req, SEL_APPEND_TYPES,
			    atom_get_atom(asdp->atoms, A_SUN_LOAD), 0, NULL);
			last_index++;

			DBGOUT((D_DND, "posting request for Load\n"));
			break;
		case S_Drop:
			DBGOUT((D_DND, "posting request for Drop\n"));
			break;
		default:
			DBGOUT((D_DND, "posting request for unknown type\n"));
			break;
		}
		if (selrankname && *selrankname &&
		    (strcmp(selrankname, DEF_SELN_RANK_NAME) != 0)) {
			if (!(selrank=(Atom)xv_get(asdp->server, SERVER_ATOM,
						   selrankname))) {
				DBGOUT((D_DND, "can't get selection rank %s\n",
					selrankname));
				sel_cancel_req(asdp,
				    MGET("Data transfer failed"));
				return;
			}
			DBGOUT((D_DND, "post_init: setting seln rank to %s\n",
				 selrankname));
			xv_set(asdp->sel_req, SEL_RANK, selrank, NULL);
		}
	}
	/* append last enum item to selection request */
	xv_set(asdp->sel_req, 
	       SEL_APPEND_TYPES, 
	           atom_get_atom(asdp->atoms, A_SUN_ENUM_ITEM), 0,
	       SEL_TYPE_INDEX, last_index+1, 
		   SEL_PROP_TYPE, XA_INTEGER,
		   SEL_PROP_DATA, (unsigned int *) &reset_item,
		   SEL_PROP_FORMAT, 32,
		   SEL_PROP_LENGTH, 1,
	       NULL);

	sel_clear_data(asdp);

	/* XXX - store selection rank on LOAD??? */
	asdp->xfertype = stype;
	asdp->flags.is_forwarded = fflag ? 1 : 0;
	asdp->flags.sel_started = 1;
	asdp->last_item = SELN_RESET_ITEM;

	if (fflag) {
		DBGOUT((D_DND, "init_req: forwarded Drop\n"));
	}
	/* XXX - set tool busy for drop/paste! */
	set_dropsite_busy(asdp, True);
	AudPanel_Message(asdp->ap, MGET("Receiving data..."));

	(void) sel_post_req(asdp->sel_req);
	DBGOUT((D_DND, "posted initial multiple request\n"));
	return;
}

/*
 * this is really resoponsible for determining the next action.
 * all seln req's are bracketed by enum item req's. the first
 * enum item req sets the item to xfer. the second one is a reset.
 * if we haven't started data xfer, this is where we start the xfer.
 * if we have, we check if there are more items to get, otherwise
 * we end the request.
 */
void
sel_process_next_request(AudioSelData *asdp)
{
	/* check various flags to figure out what to do next */

	/* request was cancelled, post req to convert ERROR and
	 * clean up.
	 */

	if (asdp->flags.req_cancelled) {
		DBGOUT((D_DND, "proc_next_req: seln req cancelled!\n"));
		/* 
		 * XXX - ask holder to convert _SUN_SELECTION_ERROR so
		 * he will know to abort transfer(?)
		 */
		xv_set(asdp->sel_req, 
		       SEL_TYPE, atom_get_atom(asdp->atoms, A_SUN_SEL_ERROR),
		       0);

		(void) sel_post_req(asdp->sel_req);

		set_dropsite_busy(asdp, False);
		sel_clear_data(asdp);
		return;
	}

	/* beginning of a request, just set last_item and return */
	if (asdp->last_item == SELN_RESET_ITEM) {
		DBGOUT((D_DND, "proc_next_req: starting req for item %d\n",
			asdp->cur_item));
		asdp->last_item = asdp->cur_item;
		return;
	}

	if (asdp->flags.xfer_started == FALSE) {
		/* 
		 * This is the end of the initial request -- start data
		 * xfer here.
		 * As a shortcut, if we got a file name in the
		 * initial request, and if there's only 1 item,
		 * try to load the file and be done with it
		 */

		if ((asdp->num_items == 1) &&  ((asdp->xfertype == S_Load) ||
						(asdp->xfertype == S_TTLoad))
		    && sel_load_file(asdp)) {
			/* success, we're done! */
			sel_request_finished(asdp);
			return;
		}

		/* that didn't work, go for the bits! */
		asdp->flags.xfer_started = 1;

		/* create buffer list to receive enum items */
		sel_create_buffer_list(asdp);

		/* post request for data, will set last_item to -1 */
		start_request_for_data(asdp);
	} else {		/* data x-fer already in progress */

		/*
		 * If last_item is cur_item, we know to start a request for the
		 * next item. here's where we check if there are more
		 * items to request or if we're done.
		 */

		DBGOUT((D_DND, "finished request for enum item %d\n",
			asdp->cur_item));
		asdp->cur_item++;

		if (asdp->cur_item >= asdp->num_items) {
			DBGOUT((D_DND, "no more items to request\n"));

			/* insert buffer list into audiotool */
			do_insert_buffer_list(asdp);

			/* clean up */
			sel_request_finished(asdp);
		} else {
			DBGOUT((D_DND, "requesting next enum item %d\n",
				asdp->cur_item));

			/* ask for next enum item */
			asdp->sel_length = 0; /* clear length */
			start_request_for_data(asdp);
		}
	}
}

/*
 * try to load current item as a file. return TRUE on success, FALSE
 * otherwise.
 */
int
sel_load_file(AudioSelData *asdp)
{
	int 	pflag;		/* play on load? */

	/* first check if a file name is even set! */
	if (!asdp->filename) {
		return (FALSE);
	}

	/* XXX - don't know if this is a reasonable check */
#ifdef notdef
	/* now check if hostname == our host. if not, bail. */
	if (asdp->hostname && 
	    (strncmp(asdp->hostname, asdp->this_host, MAXHOSTNAMELEN) != 0)) {
		return (FALSE);
	}
#endif

	/* first make sure it exists before attempting to load */
	if (access(asdp->filename, R_OK) < 0) {
		return (FALSE);
	}

	/*
	 * see if we're iconic & if this was a forwarded drop. if so,
	 * play after load
	 */
	pflag = (AudPanel_Isicon(asdp->ap) && asdp->flags.is_forwarded);

	DBGOUT((D_DND, "doing DND file load, file=%s, pflag=%s\n",
		asdp->filename, pflag ? "TRUE" : "FALSE"));

	return (AudPanel_Loadfile(asdp->ap, asdp->filename, pflag));
}

/* try to request clipboard. return TRUE if we can get the clipboard */
int
Audio_GET_CLIPBOARD(Xv_opaque win)
{
	AudioSelData *asdp;

	GET_ASDP(asdp, win, "Audio_GET_CLIPBOARD");

	/* if we own it, return false and we'll paste from the local shelf */
	if (xv_get(asdp->clipboard, SEL_OWN)) {
		DBGOUT((D_DND, "Audio_GET_CLIPBOARD: we own it already!\n"));
		return (FALSE);
	}

	DBGOUT((D_DND, "Audio_GET_CLIPBOARD: requesting clipboard\n"));
	init_selection_request(asdp, S_Paste, NULL, FALSE);
	return (TRUE);
}

/*
 * done proc - called after each cvt proc is called for seln holder.
 * since we don't alloc things in the fly for single conversions,
 * nothing is done here.
 */
void
audio_sel_done_proc(Selection_owner seln, Xv_opaque val, Atom target)
{
	AudioSelData *asdp;

	GET_ASDP(asdp, seln, "audio_sel_done_proc");

	DBGOUT((D_DND, "selection transfer for target %s completed\n",
		 (char*)xv_get(asdp->server, SERVER_ATOM_NAME, target)));
}

/*
 * snarf a copy (extent) of the current selection (or all audio data) in to 
 * an audio object. 
 */
int
get_audio_selection_data(
	AudioSelData	*asdp,
	int		all,
	int		shelf,
	Audio_Object	aobj)
{
	/* this is more then we'll need, but just in case */
	unsigned char buf[BUFSIZ];
	unsigned int hlen = BUFSIZ;
	int err;

	asdp->audio_data = (aobj != NULL) ? aobj :
	    AudPanel_GetSelData(asdp->ap, all, shelf);
	
	if (!asdp->audio_data) {
		DBGOUT((D_DND, "get_audio_sel_data: can't get audio data\n"));
		sel_cancel_xfer(asdp);
		return (False);
	}

	if (!(asdp->audio_filehdr = (Audio_hdr*)calloc(1,sizeof (Audio_hdr)))) {
		DBGOUT((D_DND, "get_audio_sel_data: can't calloc audio hdr\n"));
		sel_cancel_xfer(asdp);
		return (False);
	}
	/* 
	 * NOTE: the name of this call is misleading. we're just getting
	 * the Audio_hdr C struct from the audio object. we never actually
	 * store a copy of the Audio_filehdr.
	 */
	if (audio_getfilehdr(asdp->audio_data, asdp->audio_filehdr)
	    != AUDIO_SUCCESS) {
		DBGOUT((D_DND, "get_audio_sel_data: error getting filehdr\n"));
		sel_cancel_xfer(asdp);
		return (False);
	}

	/* 
	 * to properly calculate the actual length in bytes of the
	 * audio data, we call audio_encode_filehdr and discard the
	 * data. this is done because it'll do some padding
	 */
	if ((err = audio_encode_filehdr(asdp->audio_filehdr, NULL, 0,
				 buf, &hlen)) != AUDIO_SUCCESS) {
		DBGOUT((D_DND,
		    "get_audio_sel_data: error encoding filehdr (%d)\n", err));
		sel_cancel_xfer(asdp);
		return (False);
	}
		
	asdp->sel_length = asdp->audio_filehdr->data_size + hlen;
	asdp->sel_position = 0;

	DBGOUT((D_DND, "snarfed audio data (%s), length = %d\n",
		 all ? "all" : "selection", asdp->sel_length));
	return (True);
}

/*
 * write audio data using XView selection service. this is called from 
 * the cvt proc for audio-file data. the first time we're called, we check 
 * the size of the selection and compare it to the max size the server 
 * will handle. if it's too big we need to do an INCR transfer, otherwise
 * just send the whole bloody thing at once. either way, we need to get
 * the file header into a char* buffer on the first call.
 * then, copy the audio selection into and AudioBuffer object, and 
 * append/move that to the char* buffer and send it off.
 */
int
audio_sel_write_data(
	AudioSelData	*asdp,
	Atom		*type,
	Xv_opaque	*data,
	long		*length,
	int		*format)
{
	Audio_hdr	*hdr;
	Audio_Object	audio_data;
	unsigned int	blen;		/* buf len */
	unsigned int	hlen;		/* hdr len */
	unsigned int	slen;		/* save buf len */
	unsigned int	nb;		/* number bytes sent */
	unsigned char	*buf;
	int		status;
	int		frame_size;

	/* check if we've started the xfer yet. if not, lots o work to do */
	if (!asdp->flags.xfer_started) {
		if (!asdp->audio_data) {
			DBGOUT((D_DND, "write_data: no audio data\n"));
			sel_cancel_xfer(asdp);
			return (False);
		}
		set_dropsite_busy(asdp, True);

		/* decide if we need to do INCR xfer. *length is the
		 * max size seln the server can handle.
		 */
		asdp->flags.xfer_started = 1;
		asdp->flags.is_incr = 0;

		if (asdp->xfer_buf)	/* double check */
		    free(asdp->xfer_buf);
		asdp->xfer_buf = NULL;

		/* XXX- not really the right check for length?? */
		if (asdp->sel_length >= *length) {
			/* too big, init INCR xfer */
			asdp->flags.is_incr = 1;

			/* size of each INCR chunk */
			asdp->incr_size = (DEF_INCR_DATA_SIZE < *length ?
					 DEF_INCR_DATA_SIZE : *length);
			/* 
			 * adjust it such that it's an exact multiple of
			 * a sample frame.
			 */
			frame_size = asdp->audio_filehdr->bytes_per_unit *
				asdp->audio_filehdr->channels;

			asdp->incr_size = (asdp->incr_size / frame_size) *
				frame_size;

			DBGOUT((D_DND,
				"starting INCR xfer of audio data:\n"));
			DBGOUT((D_DND,
				"\t%d bytes in %d incr's (server max=%d)\n",
				asdp->sel_length, asdp->incr_size,
				*length));

			*type = atom_get_atom(asdp->atoms, A_INCR);
			*length = 1;
			*format = 32;
			*data = (Xv_opaque) &(asdp->sel_length);
			return (True);
		}
	}

	/*
	 * if we've gotten this far we're either sending the whole
	 * kit n' kaboodle, or sending a single INCR chunk.
	 */
	/* amount we're sending in this shot */
	/* XXX - length calc depends on knowing what audio_encode_filehdr does. 
	 */
	nb = blen = asdp->flags.is_incr ? asdp->incr_size : asdp->sel_length;

	/* first alloc the xfer buffer */

	/* XXX - this is kinda lame. we can just get a ptr to the audio
	 * buffer object and avoid a copy
	 */
	if (!asdp->xfer_buf) {
		if (!(asdp->xfer_buf = (unsigned char*)calloc(1, blen))) {
			DBGOUT((D_DND, "write_data: can't alloc xfer buf\n"));
			sel_cancel_req(asdp, 
			    MGET("Data transfer failed"));
			return (False);
		}
	}		
	buf = asdp->xfer_buf;

	/* if we haven't started the xfer yet, encode the header */
	if (asdp->sel_position == 0) {
		/* If we're at position 0, first scribble the
		 * filehdr into the buf
		 */
		hlen = blen;
		if (audio_encode_filehdr(asdp->audio_filehdr, NULL, 0,
					 buf, &hlen) != AUDIO_SUCCESS) {
			DBGOUT((D_DND, "can't encode filehdr\n"));
			sel_cancel_req(asdp,
			    MGET("Data transfer failed"));
			return (False);
		}
		/* adjust buf and buf len accordingly */
		buf += hlen;
		blen -= hlen;
	}

	/*
	 * if we're at the end of the buffer, just return a NULL
	 * response to signal end of data to the requestor
	 */
	if (asdp->sel_position >= asdp->audio_filehdr->data_size) {
		*type = atom_get_atom(asdp->atoms, A_SUN_TYPE_AUDIO);
		*format = 8;
		*length = 0;
		*data = NULL;

		/* XXX - do cleanup here?? */
		/* sel_clear_data(asdp); */

		DBGOUT((D_DND, "INCR xfer complete!!!\n"));
		return (True);
	}

	/*
	 * OK, now xfer data starting at sel_position, for blen
	 * bytes. the position and length are relative to the
	 * start of the selection.
	 */
	slen = blen;		/* save it */

	/* XXX - should get audio_getbuffer to return ptr instead! */
	if (audio_getbuffer(asdp->audio_data, buf,
			  asdp->sel_position, &blen) != AUDIO_SUCCESS) {
		DBGOUT((D_DND, "error reading audio buffer pos=%d, len=%d\n",
		    asdp->sel_position, blen));
		sel_cancel_xfer(asdp);
		return (False);
	}

	DBGOUT((D_DND, "sending INCR msg with %d bytes at pos=%d\n",
		 blen, asdp->sel_position));

	asdp->sel_position += blen;
	nb -= (slen - blen);
	
	*type = atom_get_atom(asdp->atoms, A_SUN_TYPE_AUDIO);
	*format = 8;
	*length = nb;
	*data = (Xv_opaque) asdp->xfer_buf;

	return (True);
}


/*
 * Read audio data using XView selection package
 */
void
audio_sel_read_data(
	AudioSelData	*asdp,
	Atom		type,
	Xv_opaque	val,
	unsigned long	length,
	int		format)
{
	int		rflag;
	unsigned	ilen;		/* header info len (really padding) */
	unsigned long	blen;
	unsigned long	audbuflen;
	unsigned char	*buf;

	buf = (unsigned char *) val;
	blen = length * (format / 8);
	DBGOUT((D_DND, "read_data called, format=%d, length=%d (blen=%d)\n",
		 format, length, blen));

	/* check to see if we have an audio_hdr. if not, we need to decode it */
	/* XXX - should make other checks here */
	if (!asdp->audio_filehdr) {
		if (!(asdp->audio_filehdr = (Audio_hdr*) 
		    calloc(1, sizeof (Audio_hdr)))) {
			DBGOUT((D_DND, "read_data: can't calloc audio hdr\n"));
			sel_cancel_req(asdp,
			    MGET("Data transfer failed"));
			return;
		}

		if (audio_decode_filehdr(buf, asdp->audio_filehdr, &ilen) !=
		    AUDIO_SUCCESS) {
#ifdef notdef
			AudPanel_Message(asdp->ap,	/* XXX - need this? */
			    MGET("Data transfer cancelled"));
			/* XXX - don't cancel all, just cancel this enum item */
			sel_cancel_req(asdp, MGET("Data transfer failed"));
#endif
			return;
		}

		/* check if it's ok to insert this data */
		rflag = ((asdp->xfertype == S_Load) ||
		    (asdp->xfertype == S_TTLoad));
		if (!rflag &&
		    !AudPanel_Caninsert(asdp->ap, asdp->audio_filehdr)) {
			AudPanel_Alert(asdp->ap, MGET(
			    "Can't insert selection:\n"
			    "The new data does not match the current "
			    "audio format.\n"));
			sel_cancel_req(asdp, MGET("Data transfer cancelled"));
			return;
		}

		/* we have the header and some data, adjust blen and buf */
		blen -= (sizeof (Audio_hdr) + ilen);
		buf += (sizeof (Audio_hdr) + ilen);

		/*
		 * Create the AudioBuffer object to store the data in.
		 * set it to the length specified in the Audio_hdr.
		 * If file hdr doesn't have size (not req'd), use size
		 * gotten from LENGTH request.
		 */
		if (asdp->audio_filehdr->data_size == AUDIO_UNKNOWN_SIZE) {
			DBGOUT((D_DND,
			    "data_size not set, using file size for buflen\n"));
			audbuflen =
			    asdp->sel_length - sizeof (Audio_hdr) + ilen;
		} else {
			audbuflen = asdp->audio_filehdr->data_size;
		}

		asdp->audio_data =
		    audio_createbuffer(asdp->audio_filehdr, audbuflen);
		DBGOUT((D_DND, "got audio header, created %d byte buffer\n", 
		    audbuflen));
	}

	/* now we just stuff the data into the AudioBuffer */
	DBGOUT((D_DND, "writing %d bytes starting at %d to audio buffer\n",
	    blen, asdp->sel_position));
	if (audio_putbuffer(asdp->audio_data, buf, 
	    &blen, &(asdp->sel_position)) != AUDIO_SUCCESS) {
		DBGOUT((D_DND, "error writing to audio buffer\n"));
		sel_cancel_req(asdp, MGET("Data transfer failed"));
		return;
	}
	DBGOUT((D_DND, "wrote %d bytes, next buffer starts at %d\n",
		 blen, asdp->sel_position));
}

/* insert the buffer's we've collected from the selection transfer
 * in to the audiotool segment canvas. check if we need to unload
 * the loaded data and if we need to select the inserted data.
 */
void
do_insert_buffer_list(AudioSelData *asdp)
{
	int rflag;		/* flag to replace cur audio data */
	int sflag;		/* flag to select cur audio data */
	int pflag;		/* flag to auto play */

	/* make sure there's something to insert */
	if (asdp->num_bufs && asdp->audio_buffer_list) {
		/* only replace if doing a load (otherwise will insert) */
		rflag = ((asdp->xfertype == S_Load) 
			 || (asdp->xfertype == S_TTLoad));
		/* only select if item is a drop on the canvas */
		sflag = (asdp->xfertype == S_Drop);

		/* only set if drop on icon (forwarded) || ToolTalk load */
		pflag = ((asdp->xfertype == S_TTLoad) || 
			 asdp->flags.is_forwarded);

		DBGOUT((D_DND, "inserting buffer list of %d items\n", 
			 asdp->num_bufs));

		AudPanel_InsertFromBufferList(asdp->ap, 
					      asdp->audio_buffer_list,
					      asdp->num_bufs,
					      rflag, sflag, pflag);
	}
	/* deal with making link, etc. */
	finish_insert_data(asdp);
}

/* new data has been loaded/inserted into audiotool. if it's
 * loaded we need to determine if a link should be established.
 * if so, create the insert-back button on the main panel and
 * put the name of the app we're linked to in the title bar.
 */
void
finish_insert_data(AudioSelData *asdp)
{
	asdp->flags.xfer_success = 1;

	/* set up link info if transfer was a load (only if this
	 * was a load and the holder converted the LOAD target, and
	 * if it was a single file transfer).
	 */
	if (((asdp->xfertype == S_Load) || (asdp->xfertype == S_TTLoad))
	    && asdp->load_atom && (asdp->num_items == 1)) {

		/* set the basename of the file (DATA_LABEL from app) */
		AudPanel_Setpath(asdp->ap, asdp->data_label);

		/* 
		 * put the app name and (maybe?) the host name
		 * in the title bar to show link. File menu will change
		 * to "Save to app-name".
		 */
		AudPanel_Setlink(asdp->ap,
		    asdp->app_name ? asdp->app_name : MGET("???"),
		    ((asdp->xfertype == S_TTLoad) && asdp->compose_type));

		if ((asdp->xfertype == S_TTLoad) && (asdp->compose_type))
			AudPanel_CREATE_DONE_BUTTON((ptr_t)asdp->canvas,
			    asdp->compose_type);

		/* update the header */
		AudPanel_Namestripe(asdp->ap);
	}
}

/*
 * set up load context, after a successful round of requests. examines
 * the seln context and extracts necessary info for doing save back
 */
void
sel_create_load_context(AudioSelData *asdp)
{
	DBGOUT((D_DND, "creating LOAD context for saveback\n"));

	if (asdp->load_context) {
		DBGOUT((D_DND, "warning: already have load context\n"));
		return;
	}
	asdp->load_context = (struct sel_load_context *)
		calloc(sizeof (struct sel_load_context), 1);
	if (!asdp->load_context) {
		DBGOUT((D_DND, "warning: can't calloc load context!\n"));
		return;
	}

	asdp->load_context->xfertype = asdp->xfertype;
	asdp->load_context->load_atom = asdp->load_atom;
	asdp->load_context->insert_atom = conjure_insert(asdp);
	if (!asdp->load_context->insert_atom) {
		/* XXX - do something? */
		DBGOUT((D_DND, "can't create an INSERT selection\n"));
	}
	asdp->load_context->compose = (asdp->compose_type ? TRUE : FALSE);
	asdp->load_context->type = asdp->type;
	asdp->load_context->done = FALSE;
}

/*
 * clear load context.
 */
void
sel_clear_load_context(AudioSelData *asdp)
{
	DBGOUT((D_DND, "clearing LOAD context\n"));

	if (!asdp->load_context) {
		DBGOUT((D_DND, "warning: no load context\n"));
		return;
	}

	free(asdp->load_context);
	asdp->load_context = NULL;
}


/* warning: gnarly cryptic code to follow!
 * here's where we post a request to the app we're linked to and ask
 * to convert INSERT_SELECTION. first we create a transient selection
 * of our own and become the owner of it. we attach the atom for selection
 * rank and the type of data we're holding (audio) to as prop info
 * to the selection request. we then use the load_atom as the selection
 * rank to post the request on. upon a successful reply we expect
 * the client to start asking us the standard stuff
 */
void
Audio_SAVEBACK(
	Xv_opaque	win,
	Audio_Object	aobj,
	int		done)
{
static Atom		atom_pair[2];	/* pair 'o atom's  */
	AudioSelData	*asdp;
	
	GET_ASDP(asdp, win, "Audio_SAVEBACK");

	/* do all sorts of sanity checks */
	if (!asdp->load_context) {
		DBGOUT((D_DND, "Audio_SAVEBACK: could not get context\n"));
		cancel_insert_seln(asdp, MGET("Saveback failed"));
		return;
	}

	if (!asdp->load_context->load_atom) {
		DBGOUT((D_DND, "Audio_SAVEBACK: no LOAD atom\n"));
		cancel_insert_seln(asdp, MGET("Saveback failed"));
		return;
	}

	if (!asdp->load_context->insert_atom && 
	    (!(asdp->load_context->insert_atom = conjure_insert(asdp)))) {
		DBGOUT((D_DND, "Audio_SAVEBACK: no INSET atom\n"));
		cancel_insert_seln(asdp, MGET("Saveback failed"));
		return;
	}

	/* 0=seln rank, 1=target to cvt */
	atom_pair[0] = asdp->load_context->insert_atom;

	/* the same type we received */
	atom_pair[1] = asdp->load_context->type ? asdp->load_context->type :
		atom_get_atom(asdp->atoms, A_SUN_TYPE_AUDIO);

	/* flag if we break link after saveback */
	asdp->load_context->done = done;


	/* we post a request to convert INSERT_SELECTION, with
	 * the SEL_RANK being the LOAD atom and other goodies
	 * set as property data. yech!
	 */
	xv_set(asdp->sel_req, SEL_TYPES, 
	       atom_get_atom(asdp->atoms, A_INSERT_SELN),
	       0,
	       SEL_TYPE_INDEX, 0, 
		SEL_PROP_TYPE, atom_get_atom(asdp->atoms, A_ATOM_PAIR),
		SEL_PROP_DATA, atom_pair,
		SEL_PROP_FORMAT, 32,
		SEL_PROP_LENGTH, 2,
	       SEL_RANK, asdp->load_context->load_atom,
	       NULL);

	/* we're sending the whole file back! */
	if (get_audio_selection_data(asdp, 1, 0, aobj) != True) {
		DBGOUT((D_DND, "Audio_SAVEBACK: no selection data\n"));
		cancel_insert_seln(asdp, MGET("Saveback failed"));
		return;
	}

	/* do the set up */

	asdp->flags.is_insert = 1;
	asdp->flags.is_incr = 0;
	asdp->flags.xfer_success = 0;
	asdp->flags.xfer_started = 0;
	asdp->flags.sel_started = 1;

	DBGOUT((D_DND, "about to post INSERT_SELECTION\n"));

	set_dropsite_busy(asdp, True);

	/* post it! */
	(void) sel_post_req(asdp->sel_req);

}

/* cancel insert selection (because of error, etc.). calls AudPanel_Breaklink
 * to reset tool label and discard load atom. in addition, clears current
 * selection data.
 */
void
cancel_insert_seln(AudioSelData *asdp, char* msg)
{
	AudPanel_Message(asdp->ap, msg);

	/* clear title bar, etc. */
	AudPanel_Breaklink(asdp->ap);

	/* AudPanel_Breaklink will call this *only* if a previous link
	 * was successfully established. we need to force the issue here
	 * to insure we're not holding any LOAD atoms from an incomplete
	 * link.
	 */
	discard_load(asdp->canvas);

	set_dropsite_busy(asdp, False);
	sel_clear_data(asdp);
}

/* called by AudPanel_Breaklink to discard load selection (breaks the link).
 * canvas object is passed in here and we retrieve asdp since caller
 * doesn't know about our data struct's.
 */
void
discard_load(Xv_opaque win)
{
	AudioSelData *asdp;
	Selection_requestor sreq; /* tmp selection requestor object */
	unsigned int len, fmt;

	GET_ASDP(asdp, win, "discard_load");

	/* post a _SUN_SELECTION_END request to the holder of the load
	 * so he knows to release his data.
	 */

	if (asdp->load_context && asdp->load_context->load_atom) {
		DBGOUT((D_DND, "discarding LOAD, breaking link\n"));

		sreq = (Selection_requestor) 
		    xv_create(canvas_paint_window(asdp->canvas),
			      SELECTION_REQUESTOR, 
			      SEL_TYPE, atom_get_atom(asdp->atoms, 
						      A_SUN_SEL_END),
			      SEL_RANK, asdp->load_context->load_atom,
			      SEL_REPLY_PROC, audio_sel_reply_proc,
			      XV_KEY_DATA, AUD_SEL_DATA_KEY, asdp,
			      0);
		(void) sel_post_req(sreq);
		sel_clear_load_context(asdp);
	}
}

/* make up a transient selection, return the Atom */
Atom
conjure_transient_selection(AudioSelData *asdp)
{
	char buf[100];		/* more then enough space */
	Atom seln_atom;
	int i;

	for (i=0; ; i++) {
		sprintf(buf, AUDIO_SELN_TMPL, i);

		if (!(seln_atom = xv_get(asdp->server, SERVER_ATOM, buf)))
		    return (0);

		if (XGetSelectionOwner(asdp->dpy, seln_atom) == None) {
			DBGOUT((D_DND, "conj_trans_seln: returning %d/%s\n",
				seln_atom, buf));
			return (seln_atom);
		}
	}
}


/*
 * create a selection, and return the atom that represents it
 */
Atom
conjure_insert(AudioSelData *asdp)
{
static Selection_owner	sel = NULL;
	Atom		sel_atom = NULL;
	int		i;

	/* check if we already have a transient seln */
	if ((sel_atom = (Atom)xv_get(asdp->insert_sel, SEL_RANK))
	    && xv_get(asdp->insert_sel, SEL_OWN))
	    {
		    DBGOUT((D_DND, "already have tansient seln %s\n",
			     (char*)xv_get(asdp->server, SERVER_ATOM_NAME,
					   sel_atom)));
		    return (sel_atom);
	    }

	XGrabServer(asdp->dpy);

	sel_atom = conjure_transient_selection(asdp);

	if (sel_atom) {
		/* now set the selection */
		xv_set(asdp->insert_sel, SEL_RANK, sel_atom, NULL);
		xv_set(asdp->insert_sel, SEL_OWN, TRUE, 0);
	}

	XUngrabServer(asdp->dpy);

	return (sel_atom);
}

/* When we lose the primary selection, this procedure is called.  We dehigh-
 * light our selection.
 */
void
audio_sel_lose_proc(Selection_owner seln)
{
	AudioSelData		*asdp;

	GET_ASDP(asdp, seln, "audio_sel_lose_proc");

	/* clear the selection */
	if (seln == asdp->audio_primary) {
		DBGOUT((D_DND, "lost ownership of AUDIO PRIMARY selection\n"));
		AudPanel_Clearselect(asdp->ap);
	} else if (seln == asdp->clipboard) {
		AudPanel_Releaseshelf(asdp->ap);
		DBGOUT((D_DND, "lost ownership of CLIPBOARD\n"));
	} else if (asdp->load_context && asdp->load_context->insert_atom &&
	    (asdp->load_context->insert_atom == (Atom)xv_get(seln,SEL_RANK))) {
		DBGOUT((D_DND, "lost ownership of INSERT_SELECTION atom %s\n",
			 (char*)xv_get(asdp->server, SERVER_ATOM_NAME,
				       xv_get(seln,SEL_RANK))));
		asdp->load_context->insert_atom = NULL;
	} else {
		DBGOUT((D_DND, "lost selection ownership of seln rank %s\n",
			 (char*)xv_get(asdp->server, SERVER_ATOM_NAME, 
				       xv_get(seln,SEL_RANK))));
	}
}

/*
 * send a request for the data. here we package up a request to cvt
 * an ATM and and a particular type.
 */
void
start_request_for_data(AudioSelData *asdp)
{
static int	reset_item = SELN_RESET_ITEM;

	/* check our current state so we know what to do */
	if (!asdp->flags.sel_started) {
		DBGOUT((D_DND, "can't req data, no sel xfer started\n"));
		sel_cancel_req(asdp, MGET("Data transfer failed"));
		return;
	}

	/* XXX - we should *probably* request TEXT as a fallback */
	if (!asdp->type) {
		DBGOUT((D_DND, "can't req data, type not set\n"));
		sel_cancel_req(asdp, MGET("Data transfer failed"));
		return;
	}

	/* set last_item to -1 (reset). we'll look at this in the
	 * reply proc to know wether we're at the beginning or
	 * end of a multiple. 
	 */
	asdp->last_item = SELN_RESET_ITEM;

#ifdef USE_ATM_TCP
	if (asdp->atm &&
	    (asdp->atm == atom_get_atom(asdp->atoms, A_SUN_ATM_TCP_SOCKET))) {
		start_atm_tcp_request(asdp);
		return;
	}
#endif /* !USE_ATM_TCP */

	/* if we get this far, doing X server transfer */

	DBGOUT((D_DND, "using X server to transfer data\n"));
	xv_set(asdp->sel_req,
	       SEL_TYPES, 
		   atom_get_atom(asdp->atoms, A_SUN_ENUM_ITEM),
		   atom_get_atom(asdp->atoms, A_LENGTH),
		   atom_get_atom(asdp->atoms, A_SUN_LENGTH_TYPE),
		   asdp->type,
		   atom_get_atom(asdp->atoms, A_SUN_ENUM_ITEM),
	           0,
	       SEL_TYPE_INDEX, 0, 
	           SEL_PROP_TYPE, XA_INTEGER,
	           SEL_PROP_DATA, &(asdp->cur_item),
		   SEL_PROP_FORMAT, 32,
		   SEL_PROP_LENGTH, 1,
	       SEL_TYPE_INDEX, 2, 
		   SEL_PROP_FORMAT, 8,
		   SEL_PROP_LENGTH, 0,
		   SEL_PROP_TYPE, asdp->type,
		   SEL_PROP_DATA, "", 	/* this stuff irrelevant */
	       SEL_TYPE_INDEX, 4, 
		   SEL_PROP_TYPE, XA_INTEGER,
		   SEL_PROP_DATA, (unsigned int*) &(reset_item),
		   SEL_PROP_FORMAT, 32,
		   SEL_PROP_LENGTH, 1,
	       0);

	/* go to it!! */
	(void) sel_post_req(asdp->sel_req);

}

#ifdef USE_ATM_TCP
/* post request to cvt ATM_TCP_SOCKET for TCP transfer */

void
start_atm_tcp_request(AudioSelData *asdp)
{
static int	addr_port_nums[2]; /* send addr/port info to sel holder */

	DBGOUT((D_DND, "using SUN_ATM_TCP_SOCKET\n"));
	
	/* ok. we're cool. let's get it going. first ask about type
	 * length, then ATM, then cvt the type
	 */
	/* setup request for TCP_SOCKET (append to types list) */

	addr_port_nums[0] = asdp->req_addr;
	addr_port_nums[1] = asdp->req_port;
	
	/*
	 * register socket w/notifier 'cause we're expecting
	 * the holder to connect to us when he gets the ATM request.
	 */
	audio_tcp_req_start(asdp);

	xv_set(asdp->sel_req,
	       SEL_TYPES, 
		   atom_get_atom(asdp->atoms, A_SUN_ENUM_ITEM),
		   atom_get_atom(asdp->atoms, A_LENGTH),
		   atom_get_atom(asdp->atoms, A_SUN_LENGTH_TYPE),
		   asdp->atm,
		   asdp->type,
		   atom_get_atom(asdp->atoms, A_SUN_ENUM_ITEM),
		   0,
	       SEL_TYPE_INDEX, 0, 
		   SEL_PROP_TYPE, XA_INTEGER,
		   SEL_PROP_DATA, &(asdp->cur_item),
		   SEL_PROP_FORMAT, 32,
		   SEL_PROP_LENGTH, 1,
	       SEL_TYPE_INDEX, 2, 
		   SEL_PROP_FORMAT, 8,
		   SEL_PROP_LENGTH, 0,
		   SEL_PROP_TYPE, asdp->type,
		   SEL_PROP_DATA, "", /* this stuff irrelevant */
	       SEL_TYPE_INDEX, 3,
		   SEL_PROP_FORMAT, 32,
		   SEL_PROP_LENGTH, 2,
		   SEL_PROP_TYPE, XA_INTEGER,
		   SEL_PROP_DATA, (Xv_opaque) addr_port_nums,
	       SEL_TYPE_INDEX, 5, 
		   SEL_PROP_TYPE, XA_INTEGER,
		   SEL_PROP_DATA, (unsigned int*) &(reset_item),
		   SEL_PROP_FORMAT, 32,
		   SEL_PROP_LENGTH, 1,
	       0);

	/* set flags */
	asdp->flags.xfer_started = 1;
	DBGOUT((D_DND, "posting multiple request for LENGTH/ATM/TYPE\n"));

	/* go to it!! */
	(void) sel_post_req(asdp->sel_req);
}
#endif /* !USE_ATM_TCP */
