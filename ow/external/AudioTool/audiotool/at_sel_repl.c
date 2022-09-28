/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)at_sel_repl.c	1.28	93/03/03 SMI"

/*
 * Selection service and Drag N Drop code (XView interface)
 * Selection Reply Proc's
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

/*
 * Here's the primary reply proc.
 * It calls the reply proc's for the atoms that are sent to holder to convert.
 */
void
audio_sel_reply_proc(
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	AudioSelData		*asdp;
	AtomListEntry		*alep;

	GET_ASDP(asdp, sel_req, "audio_sel_reply_proc");

	/* first check for selection error and report it */
	if (length == SEL_ERROR) {
		DBGOUT((D_DND, "reply: selection error for target %s, ",
		       (char*)xv_get(asdp->server, SERVER_ATOM_NAME, target)));
		switch (*(int*)val) {
		case SEL_TIMEDOUT:
			DBGOUT((D_DND, "Selection Timeout\n"));
			break;
		case SEL_BAD_WIN_ID:
			DBGOUT((D_DND, "Bad Window ID\n"));
			break;
		case SEL_BAD_TIME:
			DBGOUT((D_DND, "Bad Time\n"));
			break;
		case SEL_BAD_CONVERSION:
			DBGOUT((D_DND, "Bad Conversion\n"));
			break;
		default:
			DBGOUT((D_DND, "Unknown error (%d)\n", *(int*)val));
		}
	}
	DBGOUT((D_DND, "audio_sel_reply_proc: processing %s\n",
		 (char*)xv_get(asdp->server, SERVER_ATOM_NAME, target)));

	/*
	 * check if the req's been cancelled. if so, just ignore
	 * all replies except for ENUM_ITEM, SUN_SEL_END, SUN_SEL_ERROR, etc. 
	 */
	if (asdp->flags.req_cancelled &&
	    (target != atom_get_atom(asdp->atoms, A_SUN_ENUM_ITEM)) &&
	    (target != atom_get_atom(asdp->atoms, A_SUN_SEL_END)) &&
	    (target != atom_get_atom(asdp->atoms, A_MULTIPLE)) &&
	    (target != atom_get_atom(asdp->atoms, A_SUN_SEL_ERROR))) {
		DBGOUT((D_DND, "reply_proc: request cancelled, ignoring\n"));
		return;
	}

	/* let the handlers for the individual items deal with this further */
	/* see if there's an entry in our list for this guy */
	if (alep = atom_lookup_atom(asdp->atoms, target)) {
		if (alep->repl_proc) {
			(*alep->repl_proc)(asdp, sel_req, target, type,
			    val, length, format);
		} else {
			DBGOUT((D_DND, "warning: no repl proc for target %s\n",
			    alep->name));
			return;
		}
	} else {
		DBGOUT((D_DND, "?never heard of target %s?\n",
		    (char*)xv_get(asdp->server, SERVER_ATOM_NAME, target)));
	}
}

/* This should NEVER happen (but it does). */
void
sel_repl_multiple(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length == SEL_ERROR) {
		DBGOUT((D_DND, "error sending Multiple request\n"));
		/* 
		 * clear any load atom. if a MULTIPLE fails, ALL
		 * data is invalidated. this should only happen under
		 * extremely bogus conditions (i.e. other client is
		 * really hosed or goes away).
		 */
		sel_cancel_req(asdp, MGET("Data transfer failed"));

		/*
		 * force processing of next item in case there
		 * are no more pending requests. this will eliminate
		 * possibility of hanging in a busy state.
		 */
		sel_process_next_request(asdp);
	}
}

void
sel_repl_targets(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length == SEL_ERROR) {
		/* XXX - need to really handle this */
		return;
	}
	if (asdp->holder_targets)
	   DBGOUT((D_DND, "sel_repl_targets: already have holder targets?\n"));

	COPY_ATOM_LIST(asdp->holder_targets, val, length);

#ifdef DEBUG
    {
	int		i;
	AtomListEntry	*alep;
	Atom		*alist;
	for (i = 0, alist = asdp->holder_targets; i<length; i++, alist++) {
		if (alep = atom_lookup_atom(asdp->atoms, *alist)) {
			DBGOUT((D_DND, "target %s supported\n", alep->name));
		} else {
			DBGOUT((D_DND, "target atom %s not in list\n",
			    (char*)xv_get(asdp->server, 
			    SERVER_ATOM_NAME, *alist)));
		}
	}
    }
#endif
}

void
sel_repl_types(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	int			i;
	AtomListEntry		*alep;
	Atom			*alist;

	if (length == SEL_ERROR) {
		/* XXX - should fall back and use TEXT type? */
		DBGOUT((D_DND, "error getting avail types\n"));
		sel_cancel_req(asdp, MGET("Data transfer failed"));
		return;
	}
	if (asdp->holder_types)
	    DBGOUT((D_DND, "sel_repl_types: already have holder types?\n"));

	COPY_ATOM_LIST(asdp->holder_types, val, length);

	for (i=0, alist = asdp->holder_types; i<length && alist && *alist;
	    i++, alist++) {
		if (alep = atom_lookup_atom(asdp->atoms, *alist)) {
			DBGOUT((D_DND, "type %s supported\n", alep->name));
		} else {
			DBGOUT((D_DND, "type atom %s supported (not in list)\n", 
				 (char*)xv_get(asdp->server, 
					       SERVER_ATOM_NAME, *alist)));
		}
		if (is_atom_in_list(asdp->supported_types, *alist)) {
			asdp->type = *alist;
			DBGOUT((D_DND, "holder can convert %s\n",
				 (char*)xv_get(asdp->server, 
					       SERVER_ATOM_NAME, *alist)));
			break;
		}
	}
	if (!asdp->type) {
		DBGOUT((D_DND, "holder can't send us our type of data\n"));
		/* XXX - should be able to convert TEXT... */
		sel_cancel_req(asdp, MGET("Data transfer failed"));
		return;
	} 
}

/* collect holder's supported ATM's */
void
sel_repl_atms(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	int			i;
	AtomListEntry		*alep;
	Atom			*alist;

	/* next state shouldn't depend on this */
	if (length == SEL_ERROR) {
		DBGOUT((D_DND, "warning: can't get avail ATMs\n"));
		return;
	}
	if (asdp->holder_atms)
	    DBGOUT((D_DND, "sel_repl_atms: already have holder atms?\n"));

	COPY_ATOM_LIST(asdp->holder_atms, val, length);

	/* go through list and see if any of the holders ATM's is in
	 * our list of supported ATM's. if so, use one!
	 */
	for (i = 0, alist = asdp->holder_atms;
	    i < length && alist && *alist;
	    i++, alist++) {
		if (alep = atom_lookup_atom(asdp->atoms, *alist)) {
			DBGOUT((D_DND, "atm %s supported\n", alep->name));
		} else {
			DBGOUT((D_DND, "atm atom %s not in atom list\n", 
				 (char*)xv_get(asdp->server, 
					       SERVER_ATOM_NAME, *alist)));
		}
		if (is_atom_in_list(asdp->supported_atms, *alist)) {
			asdp->atm = *alist;
			DBGOUT((D_DND, "holder can use atm %s\n",
				 (char*)xv_get(asdp->server, 
					       SERVER_ATOM_NAME, *alist)));
			break;
		}
	}
	/* if no atm's supported, we'll have to use INCR transfer.... */
	if (!asdp->atm) {
		DBGOUT((D_DND, "holder doesn't support any of our ATMs\n"));
		/* XXX - need to do std X server xfer */
	}
}

/*
 * We've asked the holder to cvt type audio-file. we should
 * now have all the data if the reply for this has been received.
 */
void
sel_repl_type_audio(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length == SEL_ERROR) {
		DBGOUT((D_DND, "holder can't cvt type %s\n",
			atom_get_name(asdp->atoms, type)));
		/* clean up */
		sel_cancel_req(asdp, MGET("Data transfer failed"));
		return;
	}
	/* if the length is 0 & it's an INCR xfer, we're done with this
	 * item, insert the AudioBuffer.
	 */
	if ((length == 0) && (asdp->flags.is_incr)) {
		DBGOUT((D_DND, "done with INCR, inserting data\n"));
		sel_add_buffer_to_list(asdp);
		return;
	}

	/* check if we're starting and INCR transfer. if so, get the length
	 * of the data and create the audio buffer. then we start filling
	 * it in on successive calls to this reply proc.
	 */
	if (type == atom_get_atom(asdp->atoms,A_INCR)) {
		if (!asdp->sel_length) /* if not set already ... */
		    asdp->sel_length = *(int*)val;
		asdp->flags.is_incr = 1;
		asdp->flags.xfer_started = 1;
		/* can't create buffer 'till we have the header ... */
		DBGOUT((D_DND, "started INCR recv of audio data (len=%d)\n",
			 asdp->sel_length));
		return;
	}
	/* 
	 * if we get this far we must be xfering audio data
	 * NOTE: if it's a 0 length buffer (but not an INCR),
	 * don't collect any data. we're just establishing a
	 * "link" (i.e. for mailtool compose).
	 */
	if (asdp->sel_length != 0) {
		audio_sel_read_data(asdp, type, val, length, format);
		(void) free((char*)val);	/* free malloc'ed data */

		/* if this NOT an INCR xfer, we can finish up here */
		if (!asdp->flags.is_incr) {
			sel_add_buffer_to_list(asdp);
		}
	}
}

void
sel_repl_delete(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length == SEL_ERROR) {
		DBGOUT((D_DND, "holder can't delete selection\n"));
	} else {
		DBGOUT((D_DND, "holder has deleted selection\n"));
	}
	return;
}

void
sel_repl_load(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length != SEL_ERROR) {
		asdp->load_atom = *(Atom*)val;
		DBGOUT((D_DND, "got Load response, atom name = %s\n",
			 (char*)xv_get(asdp->server, SERVER_ATOM_NAME,
				       asdp->load_atom)));
	} else {
		DBGOUT((D_DND, "holder can't convert Load, no saveback ...\n"));
		asdp->load_atom = NULL;
	}
	return;
}

void
sel_repl_insert(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length != SEL_ERROR) {
		DBGOUT((D_DND, "Insert_Selection successful\n"));
		set_dropsite_busy(asdp, False);

		/* mark the file as unmodified */
		/* XXX - is this the right place? */
		if (!asdp->load_context) {
			DBGOUT((D_DND, "repl_insert: no load_context?!?!\n"));
			AudPanel_Finishsaveback(asdp->ap, FALSE);
		} else {
			AudPanel_Finishsaveback(asdp->ap,
			    asdp->load_context->done);
		}
		sel_xfer_finished(asdp);
	} else {
		DBGOUT((D_DND, "holder can't convert Insert_Selection\n"));
		sel_cancel_req(asdp, MGET("Data transfer failed"));
		sel_process_next_request(asdp);
	}
	return;
}

void
sel_repl_name(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length != SEL_ERROR) {
		COPY_STRING(asdp->app_name, val, length);
		DBGOUT((D_DND, "got Name = %s\n", asdp->app_name));
	}
	return;
}

void
sel_repl_dnd_begin(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	return;
}

void
sel_repl_dnd_done(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	return;
}

void
sel_repl_dnd_error(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	return;
}

void
sel_repl_sel_error(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length == SEL_ERROR) {
		DBGOUT((D_DND, "holder can't convert SUN_SEL_ERROR\n"));
	}
	DBGOUT((D_DND, "selection reply: SUN_SEL_ERROR -- clearing\n"));
	set_dropsite_busy(asdp, False);
	sel_clear_data(asdp);
	return;
}

void
sel_repl_sel_end(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	Atom			rank;

	/* already did cleanup ... */
	DBGOUT((D_DND, "Selection Request: FINISHED!\n"));

	if (rank = (Atom) xv_get(sel_req, SEL_RANK)) {
		DBGOUT((D_DND, "got SELN_END on atom %s\n",
			(char*)xv_get(asdp->server, SERVER_ATOM_NAME, rank)));
		if (sel_req != asdp->sel_req) { /* don't need */
			DBGOUT((D_DND, "destroying seln requestor object\n"));
			xv_destroy(sel_req);
		}
	} else {
		DBGOUT((D_DND, "SELN_END reply: can't get seln rank\n"));
		return;
	}
	return;
}

void
sel_repl_label(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length != SEL_ERROR) {
		COPY_STRING(asdp->data_label, val, length);
		DBGOUT((D_DND, "got DATA_LABEL = %s\n", asdp->data_label));
	}
}

void
sel_repl_filename(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length != SEL_ERROR) {
		COPY_STRING(asdp->filename, val, length);
		DBGOUT((D_DND, "got Filename = %s\n", asdp->filename));
	}
}

void
sel_repl_hostname(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length != SEL_ERROR) {
		COPY_STRING(asdp->hostname, val, length);
		DBGOUT((D_DND, "got Hostname = %s\n", asdp->hostname));
	}
}

void
sel_repl_compose_type(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length != SEL_ERROR) {
		COPY_STRING(asdp->compose_type, val, length);
		DBGOUT((D_DND, "got COMPOSE_TYPE = %s\n", asdp->compose_type));
	}
}

void
sel_repl_sun_length(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length == SEL_ERROR) {
		/* ignore this one */
		DBGOUT((D_DND, "holder can't convert LENGTH\n"));
		return;
	}
	DBGOUT((D_DND, "LENGTH for type %s is %d\n",
	    (char*)xv_get(asdp->server, SERVER_ATOM_NAME, type), *(int*)val));

	if (asdp->sel_length && (asdp->sel_length != *(int*)val)) {
		DBGOUT((D_DND,
		    "warning: override previous length of %d with %d\n",
		    asdp->sel_length, *(int*)val));
	}
	asdp->sel_length = *(int*)val;
}

void
sel_repl_enum_ct(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length == SEL_ERROR) {
		/* ignore this one */
		DBGOUT((D_DND, "holder can't convert SUN_TYPE_LENGTH\n"));
		return;
	}
	asdp->num_items = *(int*)val;
	DBGOUT((D_DND, "holder has %d items for us\n",
		 asdp->num_items));
	return;
}

/* reply proc for ENUM_ITEM. this is responsible for state transition */
void
sel_repl_enum_item(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length == SEL_ERROR) {
		DBGOUT((D_DND, "holder can't give next enum item req'd?\n"));
		sel_cancel_req(asdp, MGET("Data transfer failed"));
	}
	/* this does the state transition */
	sel_process_next_request(asdp);
}

#ifdef USE_ATM_TCP
/* we've asked the holder to cvt ATM_TCP_SOCKET and we gave him
 * the addr/port numbers. if the reply is successful the connection
 * should be established. if not, we should unregister the notify_input_func
 * for the socket which we registered when we asked to convert this target
 */
void
sel_repl_atm_tcp_socket(
	AudioSelData		*asdp,
	Selection_requestor	sel_req,
	Atom			target,
	Atom			type,
	Xv_opaque		val,
	unsigned long		length,
	int			format)
{
	if (length == SEL_ERROR) {
		DBGOUT((D_DND, "holder can't use ATM_TCP_SOCKET\n"));

		/* clean up */
		if (asdp->atm == atom_get_atom(asdp->atoms,
		    A_SUN_ATM_TCP_SOCKET))
			asdp->atm = NULL;
		audio_tcp_req_done(asdp);
		return;
	}
	/* XXX - CLEANUP! */
	if (asdp->holder_fd < 0) {
		DBGOUT((D_DND, "didn't get a connection from holder\n"));
		return;
	}
}
#endif /* !USE_ATM_TCP */

#endif /* OWV2 */
