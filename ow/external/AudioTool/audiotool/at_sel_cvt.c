/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)at_sel_cvt.c	1.19	93/03/03 SMI"

/*
 * Selection service and Drag N Drop code (XView interface)
 *
 * Selection Convert Proc's
 */

#ifndef OWV2
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <values.h>
#include <xview/xview.h>
#include <xview/server.h>

#include "atool_panel.h"
#include "atool_sel_impl.h"
#include "atool_i18n.h"
#include "atool_debug.h"

/* here's where the selection owner converts the selection request to the
 * requested type. we'll do a lookup in the atom list to avoid a huge
 * switch statement. for atoms that we really know how to convert,
 * a convert proc will be attached to the atom list entry which will
 * get called with all arg's as this in addition to the asdp.
 */
int
audio_sel_convert_proc(Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{
	AudioSelData *asdp;
	AtomListEntry *alep;

	asdp = (AudioSelData*) xv_get(seln, XV_KEY_DATA, AUD_SEL_DATA_KEY);
	if (asdp == NULL) {
		DBGOUT((1, "sel_cvt_proc: can't get audio seln data\n"));
		goto err;
	}

	/* see if there's an entry in our list for this guy */
	if (alep = atom_lookup_atom(asdp->atoms, *type)) {
		if (alep->cvt_proc) {
			return (*alep->cvt_proc)(asdp, seln, type, data,
						 length, format);
		} else {
			DBGOUT((D_DND, "warning: no cvt proc for tgt %s\n",
				 alep->name));
			goto err;
		}

	}
	DBGOUT((D_DND, "never heard of target %s, can't cvt\n",
		 (char*)xv_get(asdp->server, SERVER_ATOM_NAME, *type)));

err:
	*type = XA_INTEGER;
	*format = 32;
	*length = 0;
	*data = NULL;
	return (False);
}


/* cvt target for end of dnd xfer */
int
sel_cvt_dnd_done(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{
	AudPanel_Message(asdp->ap, NULL);
	sel_xfer_finished(asdp);
	*format = 32;
	*type = XA_INTEGER;
	*length = 0;
	*data = NULL;
	return (True);
}

/* cvt target for end of dnd xfer */
int
sel_cvt_dnd_error(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{
	DBGOUT((D_DND, "Drag N Drop: Error\n"));

	AudPanel_Message(asdp->ap, NULL);
	sel_cancel_xfer(asdp);
	*format = 32;
	*type = XA_INTEGER;
	*length = 0;
	*data = NULL;
	return (True);
}

/* cvt target for end of sel xfer */
int
sel_cvt_sel_end(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{
	DBGOUT((D_DND, "Selection Transfer: finished\n"));

	AudPanel_Message(asdp->ap, NULL);
	sel_xfer_finished(asdp);
	*type = XA_INTEGER;
	*format = 32;
	*length = 0;
	*data = NULL;
	return (True);
}

/* cvt target for delete selection */
int
sel_cvt_delete(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{

	DBGOUT((D_DND, "Selection: Delete requested (& ignored)\n"));

	*length = 0;
	*data = NULL;
	*type = XA_INTEGER;
	*format = 32;

	/* XXX - audiotool doesn't support this (yet?) */
	return (False);
}

/* cvt target for load selection */
int
sel_cvt_load(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{
	static Atom tmp_atom;

	DBGOUT((D_DND, "Selection: Load requested (ignored)\n"));

	*length = 1;
	*format = 32;
	*type = XA_ATOM;

#ifdef notdef
	/* 
	 * We got a load.  Give back a selection on the
	 * original object.
	 */
	if (!(asdp->load_atom = conjure_load(asdp))) {
		DBGOUT((D_DND, "sel_cvt_load: can't create seln\n"));
		*data =  NULL;
		return (False);
	}
	DBGOUT((D_DND, "sel_cvt_load: sending off a _SUN_LOAD...\n"));
	*data = (Xv_opaque) &(asdp->load_atom);
	return (True);
#else
	DBGOUT((D_DND, "sel_cvt_load: we don't support _SUN_LOAD...\n"));
	*data = NULL;
	return (False);
#endif
}


/* cvt target for name */
int
sel_cvt_name(
	AudioSelData	*asdp,
	Selection_owner	seln,
	Atom		*type,
	Xv_opaque	*data,
	long		*length,
	int		*format)
{
static char		tool_name[MAXPATHLEN+1];

	*format = 8;
	*type = XA_STRING;

	if (Progname && *Progname) {
		strncpy(tool_name, Progname, MAXPATHLEN);
		*length = strlen(tool_name);
		*data = (Xv_opaque)tool_name;
		DBGOUT((D_DND, "Selection: Name requested, sending %s\n",
			tool_name));

		return (True);
	} else {
		*length = 0;
		*data = NULL;

		DBGOUT((D_DND, "Selection: Name requested, not set!!\n"));

		return (False);
	}
}

/* cvt target for data label */
int
sel_cvt_label(
	AudioSelData	*asdp,
	Selection_owner	seln,
	Atom		*type,
	Xv_opaque	*data,
	long		*length,
	int		*format)
{
static char		*def_data_label = DEF_DATA_LABEL;
static char		fname[MAXPATHLEN+1];
	char		*cp;
	
	*type = XA_STRING;
	*format = 8;

	AudPanel_Getpath(asdp->ap, fname, MAXPATHLEN);
	if (fname[0]) {
		if (cp = strrchr(fname, '/')) {
			*data = (Xv_opaque)++cp;
			*length = strlen(cp);
		} else {
			*data = (Xv_opaque)fname;
			*length = strlen(fname);
		}
	} else {		/* user default label ... */
		*data = (Xv_opaque)def_data_label;
		*length = strlen(def_data_label);
	}
	DBGOUT((D_DND, "Selection: Data Label requested, sending %s\n",
		 (char*)*data));
	return (True);
}

/* cvt target for hostname */
int
sel_cvt_hostname(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{
	DBGOUT((D_DND, "Selection: Hostname requested\n"));
	*type = XA_STRING;
	*format = 8;
	*data = (Xv_opaque)asdp->this_host;
	*length = strlen(asdp->this_host);
	return (True);
}

/* cvt target for hostname */
int
sel_cvt_filename(AudioSelData *asdp,
	Selection_owner	seln,
	Atom		*type,
	Xv_opaque	*data,
	long		*length,
	int		*format)
{
static char		fname[MAXPATHLEN+1];

	*type = XA_STRING;
	*format = 8;

	/* XXX - it doesn't make sense to convert this target ... */
#ifdef notdef
	AudPanel_Getpath(asdp->ap, fname, MAXPATHLEN);
	if (fname[0]) {
		*data = (Xv_opaque)fname;
		*length = strlen(fname);
		DBGOUT((D_DND, "Selection: Filename requested, sending %s\n",
			 fname));
		return (True);
	} else {
		*data = NULL;
		*length = 0;
		DBGOUT((D_DND, "Selection: Filename requested, NO FILE\n"));
		return (False);
	}
#else
	*data = NULL;
	*length = 0;
	return (False);
#endif
}

/* cvt target for length */
int
sel_cvt_sun_length(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{

	Sel_prop_info *selprop;
	static int ibuf;
	int status;

	*format = 32;
	*type = XA_INTEGER;
	*length = 1;

	/* XXX - should get prop info and return length for specific type */
	if (!asdp->sel_length || !asdp->audio_data) {
		DBGOUT((D_DND, "cvt_sun_length: no audio sel data?!\n"));
		*data = NULL;
		return (False);
	}

	*data = (Xv_opaque)&(asdp->sel_length);

	DBGOUT((D_DND, "Selection: Length request, returning %d\n",
		 asdp->sel_length));
	return (True);
}

/* cvt target for enum count */
int
sel_cvt_enum_ct(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{
	static int ibuf;

	/* XXX - always send 1 for now. in the future we may support
	 * sending an audio selection w/several types of audio data
	 * as seperate items
	 */
	*format = 32;
	*length = 1;

	/* XXX - this should probably be alloc'd not static space  */
	ibuf = 1;
	*data = (Xv_opaque)&ibuf;

	DBGOUT((D_DND, "Selection: Enum count requested, returing %d\n", ibuf));

	return (True);
}

/* cvt target for enum item */
int
sel_cvt_enum_item(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{

	Sel_prop_info	*selprop;
	Atom		rank = (Atom)NULL;

	*length = 0;
	*format = 32;
	*data = NULL;
	*type = XA_INTEGER;	/* arbitrary */

	if (!(selprop = (Sel_prop_info*)xv_get(seln, SEL_PROP_INFO))) {
		DBGOUT((D_DND, "cvt_enum_item: error getting prop info\n"));
		return (False);
	}

	if (!selprop->data) {
		DBGOUT((D_DND, "cvt_enum_item: prop->data is nil\n"));
		free(selprop);
		return (False);
	}

	if ((rank = (Atom)xv_get(seln, SEL_RANK)) == NULL) {
		DBGOUT((D_DND, "cvt_enum_item: can't get seln rank\n"));
	}

	if ((asdp->cur_item = *(int*)selprop->data) == SELN_RESET_ITEM) {
		DBGOUT((D_DND, "requestor asked to RESET enum count\n"));
		asdp->cur_item = 0;	/* reset enum to beginning ??? */
	} else {
		DBGOUT((D_DND, "Selection: Enum item %d requested.\n",
			 asdp->cur_item));
		/*
		 * if we haven't grabbed the seln data, grab it here.
		 * for the case of DnD, since we init the transaction
		 * (by sourcing the drag), we'll grab the data then.
		 * for a Paste (when someone requests the shelf), we
		 * grab it here. only do so if the seln RANK for this
		 * request is CLIPBOARD (to be sure).
		 */

		if ((asdp->audio_data==NULL) &&
		    (rank == atom_get_atom(asdp->atoms, A_CLIPBOARD))) {
			    get_audio_selection_data(asdp, FALSE, TRUE, NULL);
			    DBGOUT((D_DND, "grabbed copy of shelf\n"));
		    }
	}

	free(selprop);

	/* XXX - for now we only suppory 1 item. we should support
	 * multiple items in the future.
	 */
	if (asdp->cur_item > 0) {
		DBGOUT((D_DND, "requestor asked for item we don't have (%d)\n",
			asdp->cur_item));
		sel_cancel_xfer(asdp);
		return (False);
	}

	return (True);
}

/* cvt target for insert selection */
int
sel_cvt_insert(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{
	Sel_prop_info *selprop;

	*length = 0;
	*data = NULL;
	*format = 32;
	*type = XA_INTEGER;

	/* XXX - don't support this yet (if ever) ... */
#ifdef notdef
	if (!(selprop = (Sel_prop_info*)xv_get(seln, SEL_PROP_INFO))) {
		DBGOUT((1, "cvt_insert: can't get prop info\n"));
		sel_cancel_xfer(asdp);
		return (False);
	}
	

	asdp->insert_sel = *(Atom *) selprop->data;
	asdp->insert_target = *(((Atom *) selprop->data) + 1);

	free(selprop);

	/* ask the holder to give us the bits by asking to convert
	 * the target he sent us...
	 */

	sel_clear_data(asdp);
	asdp->xfertype = S_Insert;
	xv_set(asdp->sel_req,
	       SEL_RANK, asdp->insert_sel,
	       SEL_TYPE, asdp->insert_target,
	       NULL);

	(void) sel_post_req(asdp->sel_req);

	DBGOUT((D_DND, "Selection: Insert request, posted response\n"));

	return (True);
#else
	DBGOUT((D_DND, "Selection: ignoring request for INSERT\n"));
	return (False);
#endif
}

/* cvt target for begin of selection xfer */
int
sel_cvt_dnd_begin(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{

	DBGOUT((D_DND, "Drag and Drop: Begun\n"));
	*length = 0;
	*data = NULL;
	*type = XA_INTEGER;
	*format = 32;

	return (True);
}

/* cvt target for selection xfer error */
int
sel_cvt_sel_error(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{
	DBGOUT((D_DND, "Selection Transfer: ERROR.\n"));

	AudPanel_Message(asdp->ap, NULL);
	sel_cancel_xfer(asdp);
	*length = 0;
	*data = NULL;
	*type = XA_INTEGER;
	*format = 32;
	return (True);
}

/* cvt targets */
int
sel_cvt_targets(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{
	DBGOUT((D_DND, "Selection Xfer: Sending targets\n"));

	*format = 32;
	*type = XA_ATOM;
	*length = asdp->num_sup_targets;
	*data = (Xv_opaque) asdp->supported_targets;

	return (True);
}

/* cvt types */
int
sel_cvt_types(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{
	DBGOUT((D_DND, "Selection Xfer: Sending types\n"));

	*format = 32;
	*type = XA_ATOM;
	*length = asdp->num_sup_types;
	*data = (Xv_opaque) asdp->supported_types;

	return (True);
}

/* cvt targets */
int
sel_cvt_atms(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{
	DBGOUT((D_DND, "Selection Xfer: Sending ATMS\n"));

	*format = 32;
	*type = XA_ATOM;
	*length = asdp->num_sup_atms;
	*data = (Xv_opaque) asdp->supported_atms;

	return (True);
}

/* Finally!!!! send audio data. */

int
sel_cvt_type_audio(AudioSelData *asdp,
	Selection_owner seln,
	Atom *type,
	Xv_opaque *data,
	long int *length,
	int *format)
{
	int status;
	DBGOUT((D_DND, "Selection Xfer: Requestor has asked for AUDIO!\n"));

#ifdef USE_ATM_TCP
	/* check if we've got an ATM and we've started a TCP xfer,
	 * if so, send the selection over the pipe
	 */
	if (asdp->flags.xfer_started && asdp->holder_fd && 
	    (asdp->atm == atom_get_atom(asdp->atoms, A_SUN_ATM_TCP_SOCKET)))
	    {
		    DBGOUT((D_DND, "transferring audio data via open socket\n"));
		    if (AudPanel_SendSelection(asdp->ap, asdp->audio_data,
				      asdp->holder_fd) != AUDIO_SUCCESS) {
			    DBGOUT((1, "error sending audio data\n"));
			    status = False;
		    } else {
			    DBGOUT((D_DND, "audio data transferred!\n"));
			    status = True;
		    }

		    /* clean up */

		    close(asdp->holder_fd);

		    /* XXX - clean up here??? */
		    /* sel_clear_data(asdp); */

		    /* set the reply values and tell 'em we're done */
		    *type = XA_INTEGER;
		    *format = 32;
		    *length = 0;
		    *data = NULL;
	
		    return status;
	    }
#endif /* !USE_ATM_TCP */

	/* if we got this far, we're doing xfer via X server.
	 * check size of selection to see if we want to
	 * do INCR xfer or just send it all at once.
	 */

	/* XXX - init the INCR x-fer.... */
	DBGOUT((D_DND, "requested audio, using X server transfer\n"));
	
	return audio_sel_write_data(asdp, type, data, length, format);
}

#ifdef USE_ATM_TCP
/* cvt target for _SUN_ATM_TCP_SOCKET. we'll be getting
 * the addr/port to use as a pair of atoms. try to initiate
 * connection at that point (just do the connect and save
 * the descriptor). if that succeeds, send back True, otherwise False.
 *
 */
int
sel_cvt_atm_tcp_socket(
	AudioSelData	*asdp,
	Selection_owner	seln,
	Atom		*type,
	Xv_opaque	*data,
	long		*length,
	int		*format)
{
	unsigned long	*ilist;
	unsigned long	addr;
	unsigned short	port;
	Sel_prop_info	*selprop;
	int		fd;
	
	DBGOUT((D_DND,
	    "Selection Xfer: Requestor has asked for ATM_TCP_SOCKET\n"));

	/* this stuff doesn't really matter. a succesfull reply is all
	 * that's needed
	 */
	*type = XA_INTEGER;
	*format = 32;
	*length = 0;
	*data = NULL;

	if (!(selprop = (Sel_prop_info*)xv_get(seln, SEL_PROP_INFO))) {
		DBGOUT((1, "can't get SEL_PROP_INFO for ATM_TCP_SOCKET\n"));
		return (False);
	}
	if (selprop->length != 2) {
		DBGOUT((1, "can't cvt on ATM_TCP_SOCKET, wrong length\n"));
		/* XXX - do some clean up */
		return (False);
	}
	if (selprop->format != 32) {
		DBGOUT((1, "can't cvt on ATM_TCP_SOCKET, wrong format\n"));
		/* XXX - do some clean up */
		return (False);
	}

	ilist = (unsigned long *)selprop->data;
	addr = ilist[0];
	port = (unsigned short) ilist[1];

	/* now, attempt to connect. if the connect succeeds, set the
	 * the values in the AudioSelData so that when we get the
	 * request to convert type, we can go ahead
	 */

	if ((fd = audio_tcp_send_start(addr, port)) == -1) {
		DBGOUT((1, MGET("ATM_TCP_SOCKET, can't connect\n"));
		/* XXX - clean up our act here */
		return (False);
	}

	asdp->flags.xfer_started = 1;
	asdp->holder_fd = fd;
	asdp->holder_port = port;
	asdp->holder_addr = addr;
	asdp->atm = atom_get_atom(asdp->atoms, A_SUN_ATM_TCP_SOCKET);

	return (True);
}
#endif /* !USE_ATM_TCP */

#endif /* OWV2 */
