#ident  "@(#)selection.c 3.11 94/07/15 Copyr 1987 Sun Micro"



/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
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

/*
 * Mailtool - selection handling
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

#include "xview/sel_pkg.h"
#include <xview/sel_svc.h>
#include <xview/sel_attrs.h>
#include <xview/textsw.h>
#include <xview/panel.h>
#include <xview/xview.h>
#include <xview/dragdrop.h>
#include <xview/font.h>
#include <xview/xv_xrect.h>
#include "attach.h"
#include "glob.h"
#include "header.h"
#include "tool.h"
#include "tool_support.h"
#include "mle.h"
#include "../maillib/obj.h"
#include "../maillib/ck_strings.h"
#include "list.h"

#define DEBUG_FLAG mt_debugging
#include "debug.h"


struct sel_data {
	Attach_list *sd_al;
	Attach_node *sd_an;
	struct attach **sd_atlist;
	struct attach *sd_at;
	struct msg *sd_msg;
	unsigned sd_atcount;
	Display *sd_display;
	Xv_object sd_frame;		/* a root frame we can tie the sd to */
	Xv_object sd_errorframe;	/* where to send frame errors */
	Xv_object sd_window;
	Selection sd_sel;
	Xv_Server sd_server;
	unsigned sd_load : 1;		/* this seln is the result of a load */
	unsigned sd_loadok : 1;		/* are loads legal at all? */
	unsigned sd_errtarget : 1;	/* the other side converted the error target */
	unsigned sd_seen_selend : 1;	/* clean up when at the end of the multiple */
	unsigned sd_itimer : 1;		/* only clean up once */
	unsigned sd_cancelled : 1;	/* this sd is no longer any good */
	void (*sd_last_doneproc)();	/* original done proc to free data */
	void (*sd_cleanup_proc)(); 	/* Used when dragging a msg from hdr list */
	void *sd_cleanup_arg;
	ListNode *sd_listnode;
	int  sd_rank;
	Drawable sd_drawable;
};


static Seln_request    *seln_buffer;
static Seln_holder	seln_holder;

static int mail_sel_convert_proc();

#define STREQ(a,b)	(!strcmp((a),(b)))
#define STRNEQ(a,b,n)	(!strncmp((a),(b),(n)))

/* there is a race condition in setting up the selection rank.
 * it is one when we call mt_attach_selection_data(); it changes
 * on the first callback...
 */
#define NO_RANK		1



static ListNode *validSelnList;

void *
find_seln_on_list(ListNode *ln, void* data, va_list va)
{
	Selection sel;
	int rank;
	struct sel_data *sd = data;

	/* the two arguments in the va_list are a selection and its rank */
	sel = va_arg(va, Selection);
	rank = va_arg(va, int);

	/* sd should never be null, but just in case... */
	if (! sd) return (NULL);

	/* if the sd has already been cancelled, don't accept it */
	if (sd->sd_cancelled) return (NULL);


	if (sd->sd_sel == sel &&
		(sd->sd_rank == NO_RANK || sd->sd_rank == rank)) 
	{
		return (sd);
	}
	return (NULL);
}




#define	ATTACH_KEY_DATA	get_attach_key_data()
static int
get_attach_key_data()
{
	static key;

	if (! key) {
		key = xv_unique_key();
	}

	return (key);
}


char *
myhostname()
{
	static char host[257];
	static int  sethost = 0;

	if (! sethost) {
		
		host[sizeof host -1] = '\0';
		gethostname(host, sizeof host -1);
		sethost = 1;
	}

	return (host);
}


static char *valid_targets[] = {
	"TARGETS",
	"HOST_NAME",
	"_SUN_DATA_LABEL",
	"TEXT",
	"_SUN_AVAILABLE_TYPES",
	"_SUN_ALTERNATE_TRANSPORT_METHODS",
	"_SUN_LENGTH_TYPE",
	"DELETE",
	"_SUN_ENUMERATION_COUNT",
	"_SUN_ENUMERATION_ITEM",
	"NAME",
	"_SUN_SELECTION_END",
	"_SUN_DRAGDROP_DONE",
	"_SUN_SELECTION_ERROR",
	"_SUN_SAMEWIN_ERROR",
	"_SUN_EXECUTABLE",
};

static int num_valid_targets = ((sizeof valid_targets)/sizeof(char *));


static char *
get_type_string(at)
struct attach *at;
{
	char *s;
	char *type;

	type = attach_methods.at_get(at, ATTACH_DATA_TYPE);

	if (! type) return(NULL);

	s = malloc(strlen(type) + 11);

	if (s) {
		sprintf(s, "_SUN_TYPE_%s", type);
	}

	return (s);
}



static void
dnd_free_buf(sel, buf, target)
Selection_owner sel;
void *buf;
Atom target;
{
	struct sel_data *sd_list;

	DP(("dnd_free_buf: target %d\n"));

	/* make sure the sd is still valid */
	sd_list = enumerate_listnode(&validSelnList, find_seln_on_list,
		sel, xv_get(sel, SEL_RANK));

	if (sd_list == NULL) {
		/* it wasn't on the valid list -- give up */
		DP(("\ndnd_free_buf: sd for rank %d not on list\n\n",
			xv_get(sel, SEL_RANK)));
		return;
	}

	xv_set(sel, SEL_DONE_PROC, NULL, 0);
	ck_free(buf);
}


static Notify_func
dnd_selitimer(sd, which)
struct sel_data *sd;
int which;
{
	DP(("dnd_selitimer: sd %x\n", sd));

	notify_set_itimer_func((Notify_client)sd, NOTIFY_FUNC_NULL,
		ITIMER_REAL, 0, 0);

	xv_set(sd->sd_sel, XV_KEY_DATA, ATTACH_KEY_DATA, NULL, 0);

	/* remove us from the "valid" list of selections */
	rem_listnode(sd->sd_listnode);

	if (sd->sd_cleanup_proc) {
		/* This is used when dragging a message from the header
		 * list. In this case we dummied up a message and attachments
		 * which need to be destroyed.  Do it now.
		 */
		(*sd->sd_cleanup_proc)(sd->sd_cleanup_arg);
	}

	xv_set(sd->sd_sel, SEL_OWN, False, 0);
	xv_destroy_safe(sd->sd_sel);

	free(sd->sd_atlist);
	free(sd);
}


static void
dnd_done_last(sel, buf, target)
Selection_owner sel;
void *buf;
{
	struct sel_data *sd, *sd_list;

	DP(("dnd_done_last: target %d\n", target));

	/* make sure the sd is still valid */
	sd_list = enumerate_listnode(&validSelnList, find_seln_on_list,
		sel, xv_get(sel, SEL_RANK));

	if (sd_list == NULL) {
		/* it wasn't on the valid list -- give up */
		DP(("\ndnd_done_last: sd for rank %d not on list\n\n",
			xv_get(sel, SEL_RANK)));
		return;
	}

	sd = (struct sel_data *)xv_get(sel, XV_KEY_DATA, ATTACH_KEY_DATA);
	if (sd && sd == sd_list) {
		xv_set(sel, XV_KEY_DATA, ATTACH_KEY_DATA, NULL, 0);

		if (sd->sd_last_doneproc) {
			(*sd->sd_last_doneproc)(sel, buf, target);
		}

	}

	xv_set(sel, SEL_DONE_PROC, NULL, 0);

}


static void
dnd_setitimer(sd)
struct sel_data *sd;
{
	struct itimerval itv;
	DP(("sel_convert_proc: processing delayed sel_end\n"));

	if (sd->sd_itimer) return;
	sd->sd_itimer = 1;

	sd->sd_last_doneproc = (void (*)())
		xv_get(sd->sd_sel, SEL_DONE_PROC);

	/* start an itimer to do the real cleanup */
	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = 0;
	itv.it_value.tv_sec = 1;
	itv.it_value.tv_usec = 0;

	(void)notify_set_itimer_func((Notify_client)sd,
		(Notify_func)dnd_selitimer, ITIMER_REAL,
		&itv, 0);

	xv_set(sd->sd_sel, SEL_DONE_PROC, dnd_done_last, 0);
}


static Atom
get_null_atom(server)
Xv_server server;
{
	static atom;

	if (! atom) {
		atom = xv_get(server, SERVER_ATOM, "NULL");
		DP(("get_null_atom: returning %d\n", atom));
	}

	return (atom);
}




/*
 * go and create a temporary selection
 *
 * "template" is an sprintf template with one %d in it.  It should be
 * less than 80 chars in length.
 *
 * It will return the new selection atom, or 0 if we could not
 * intern an atom (the server is out of memory?)
 */

Atom
conjure_transient_selection(server, dpy, template)
Xv_server server;
Display *dpy;
char *template;
{
	char buf[100];
	Atom seln_atom;
	int i;


	i = 0;
	for(;;) {
		sprintf(buf, template, i);
		seln_atom = xv_get(server, SERVER_ATOM, buf);

		if (! seln_atom) return(0);

		if (XGetSelectionOwner(dpy, seln_atom) == None) {
			DP(("conjure_transient_selection: returning %d/%s\n",
				seln_atom, buf));
			return (seln_atom);
		}

		i++;
	}
}



/*
 * allocate and initialize the static data that we need for
 * an OWNER selection
 */
int
mt_attach_selection_data(sel, sd_orig, al, an, msg, loadok,
	cleanup_proc, cleanup_arg)
Selection sel;
struct sel_data *sd_orig;
Attach_list *al;
Attach_node *an;	/* If NULL use selected attachments */
struct msg *msg;
int loadok;
void	(*cleanup_proc)();
void	*cleanup_arg;
{
	struct sel_data *sd;
	Xv_object	window;
	Xv_Server	server;
	Attach_node *tmp_an;
	int count;
	struct attach *at;
	int error;

	sd = (struct sel_data *) malloc(sizeof *sd);

	if (! sd) return (0);
	memset((char *)sd, '\0', sizeof(*sd));

	/*
	 * keep the sd on a list so we can tell the valid ones
	 */
	sd->sd_listnode = add_listnode(&validSelnList, sd);

	sd->sd_loadok = loadok;
	sd->sd_cleanup_proc = cleanup_proc;
	sd->sd_cleanup_arg = cleanup_arg;

	if (sd_orig) {
		/* this is a selecton to implement insert_selection.
		 * make the seldata represent the currently selected
		 * object.
		 *
		 * ZZZ:katin if we ever get reference counts, make
		 * sure we bump it here...
		 */
		sd->sd_atcount = 1;
		sd->sd_atlist =
			(struct attach **) malloc(sizeof(struct attach *));

		if (!sd->sd_atlist)
			goto ERROR_EXIT;

		sd->sd_msg = sd_orig->sd_msg;
		sd->sd_an = sd_orig->sd_an;
		sd->sd_at = sd_orig->sd_at;
		sd->sd_atlist[0] = sd_orig->sd_at;

	} else if (an != NULL) {
		/* Caller specified which attachment to use */
		sd->sd_atlist =
			(struct attach **)malloc(sizeof(struct attach *));
		if (! sd->sd_atlist)
			goto ERROR_EXIT;

		sd->sd_msg = msg;
		sd->sd_at = an->an_at;
		sd->sd_atlist[0] = sd->sd_at;
		sd->sd_atcount = 1;
		sd->sd_an = an;

		if (attach_methods.at_decode(sd->sd_at) != 0) {
			mt_vs_warn(sd->sd_frame, gettext(
		"Could not decode attachment %s\nIgnoring that attachment"),
				mt_attach_name(sd->sd_at));
			free(sd->sd_atlist);
			goto ERROR_EXIT;
		}

	} else {
		/* allocate an anlist that represents the current list of
		 * selected attach nodes
		 */
		count = 0;

		at = attach_methods.at_first(msg);
		while (at) {
			tmp_an = (Attach_node *)
				attach_methods.at_get(at,ATTACH_CLIENT_DATA);

			if (tmp_an && tmp_an->an_selected) {
				count++;
			}

			at = attach_methods.at_next(at);
		}

		/* count had better not be zero; this should already
		 * have been checked
		 */
		sd->sd_atlist = (struct attach **)
			malloc(count * sizeof(struct attach *));

		if (! sd->sd_atlist)
			goto ERROR_EXIT;
		
		/* now initialize the an list */
		count = 0;
		at = attach_methods.at_first(msg);
		while (at) {
			tmp_an = (Attach_node *)
				attach_methods.at_get(at, ATTACH_CLIENT_DATA);

			if (tmp_an && tmp_an->an_selected) {
				error = attach_methods.at_decode(at);

				if (error) {
					mt_vs_warn(sd->sd_frame, gettext(
		"Could not decode attachment %s\nIgnoring that attachment"),
					mt_attach_name(at));
				} else {
					sd->sd_atlist[count++] = at;
				}

			}

			at = attach_methods.at_next(at);
		}

		/* this is the "real" count, which ignores any
		 * attachments that didn't work
		 */
		sd->sd_atcount = count;

		/* If there is exactly one object, then you don't
		 * have to specify which object you're interested in.
		 * Otherwise there is no default
		 */
		if (sd->sd_atcount == 1) {
			sd->sd_at = sd->sd_atlist[0];
		} else {
			/* no default at */
			sd->sd_at = NULL;
		}

		sd->sd_msg = msg;
	}


	window = xv_get(sel, XV_OWNER);
	server = XV_SERVER_FROM_WINDOW(window);

	sd->sd_al = al;
	sd->sd_load = sd_orig ? 1 : 0;
	sd->sd_sel = sel;
	sd->sd_rank = xv_get(sel, SEL_RANK);
	sd->sd_server = server;
	sd->sd_window = window;
	sd->sd_display = al->al_display;
	sd->sd_frame = al->al_headerframe;
	sd->sd_errorframe = al->al_errorframe;
	sd->sd_drawable = al->al_drawable;

	DP(("mt_attach_selection_data: call set to mail_sel_convert_proc\n"));
	xv_set(sel, SEL_CONVERT_PROC, mail_sel_convert_proc,
		XV_KEY_DATA, ATTACH_KEY_DATA, sd,
		0);

	DP(("\nmt_attach_selection_data: server %x, sel %x, rank %d\n",
		sd->sd_server, sd->sd_sel, sd->sd_rank));

	return (1);

ERROR_EXIT:
	/* Encountered an error. Free data and return */
	free(sd);
	return (0);
}


void
insert_lose_proc(sel)
Selection_owner sel;
{
	struct sel_data *sd, *sd_list;
	DP(("insert_lose_proc: called for sel %x\n", sel));


	/* make sure the sd is still valid */
	sd_list = enumerate_listnode(&validSelnList, find_seln_on_list,
		sel, xv_get(sel, SEL_RANK));

	if (sd_list) {
		/* only schedule for deletion if on valid list */
		dnd_setitimer(sd_list);
	}
}


/*
 * create a selection, and return the atom that represents it
 */
static Atom
conjure_load(server, sd)
Xv_server server;
struct sel_data *sd;
{
	Atom sel_atom;
	int i;
	Selection_owner sel;
	int retval = 0;

	DP(("conjure_load: dpy %x\n", sd->sd_display));

	XGrabServer(sd->sd_display);

	sel_atom = conjure_transient_selection(server, sd->sd_display,
		"_SUN_MAILTOOL_SELN_%d");

	if (! sel_atom) goto end;

	/* now, get a selection */
	sel = (Selection_owner)
		xv_create(sd->sd_frame, SELECTION_OWNER,
			SEL_RANK, sel_atom,
			SEL_LOSE_PROC, insert_lose_proc,
			0);

	if (! sel) goto end;

	mt_attach_selection_data(sel, sd, sd->sd_al, NULL, NULL,
		FALSE, NULL, NULL);

	xv_set(sel, SEL_OWN, TRUE, 0);


	retval = sel_atom;
end:

	XUngrabServer(sd->sd_display);

	return (retval);

}




/*
 * convert targets from the other side of the dnd...
 */
static int
mail_sel_convert_proc(seln, type, data, length, format)
	Selection_owner	seln;
	Atom		*type;
	Xv_opaque	*data;
	unsigned long	*length;
	int		*format;
{
	Xv_Server	server;
	char		*atom_name;
	struct attach	*at;
	int		retval = True;
	int		multiple_type = *format;
	struct sel_data *sd;
	struct sel_data *sd_list;


	/* make sure the sd is still valid */
	sd_list = enumerate_listnode(&validSelnList, find_seln_on_list,
		seln, xv_get(seln, SEL_RANK));

	if (sd_list == NULL) {
		/* it wasn't on the valid list -- give up */
		DP(("\nsel_convert_proc: sd for rank %d not on list\n\n",
			xv_get(seln, SEL_RANK)));
		goto sel_fail;
	}


	sd = (struct sel_data *)xv_get(seln, XV_KEY_DATA, ATTACH_KEY_DATA);

	if (sd == NULL || sd != sd_list) {
		/* this should never happen, but sometimes we get messed
		 * up in the protocol...
		 */
		DP(("sel_convert_proc: NULL sd!\n"));
sel_fail:
		*type = 0;
		*data = 0;
		*length = 0;
		*format = 0;
		return (False);
	}

	at = sd->sd_at;
	server = sd->sd_server;

	/* special case for dnd targets -- this is the first time
	 * we get to see the "real" rank
	 */
	if (sd->sd_rank == NO_RANK) {
		sd->sd_rank = xv_get(seln, SEL_RANK);
	}

DP(("sel_convert_proc: seln %x/%d, atom %d, al %x, at %x, load %x%s%s%s\n",
		seln, sd->sd_rank,
		*type, sd->sd_al, at, sd->sd_load,
		*format & SEL_BEGIN_MULTIPLE ? " BEGIN" : "",
		*format & SEL_END_MULTIPLE ? " END" : "",
		*format & SEL_MULTIPLE ? " MUL" : ""));

	atom_name = (char *) xv_get(server, SERVER_ATOM_NAME, *type);
	DP(("                  converting %s\n", atom_name));

	if (atom_name == NULL) return (False);

	if (multiple_type & SEL_END_MULTIPLE) {
		/* reset the an to the default state -- we're at the end
		 * of a multiple here...
		 */
		if (sd->sd_atcount == 1) {
			sd->sd_at = sd->sd_atlist[0];
		} else {
			sd->sd_at = NULL;
		}
	}

	/* ZZZ:dnd fix me to be more efficient */
	if (STREQ(atom_name, "_SUN_SELECTION_END") ||
		STREQ(atom_name, "_SUN_DRAGDROP_DONE"))
	{

		if (! sd->sd_errtarget && ! sd->sd_seen_selend) {
			mt_frame_msg(sd->sd_errorframe, FALSE,
				gettext("Data Transfer: Completed"));
		}

		sd->sd_seen_selend = 1;

		*format = 32;
		*length = 0;
		*data = NULL;
		*type = get_null_atom(server);

	} else if (STREQ(atom_name, "TARGETS")) {
		Atom *atom_list;
		char *type_string = "";
		int i;

		/* we need to return a list of the targets that
		 * we accept
		 */

		DP(("TARGETS: num_valid_targets = %d\n", num_valid_targets));

		/* we need to free this later! */
		atom_list = (Atom *)
			malloc((num_valid_targets + 6) * sizeof(Atom));

		if (!atom_list) goto error;

		xv_set(seln, SEL_DONE_PROC, dnd_free_buf, 0);

		for (i = 0; i < num_valid_targets; i++) {
			atom_list[i] = xv_get(server,
				SERVER_ATOM, valid_targets[i]);
		}

		if (at) {
			type_string = get_type_string(at);
			if (type_string) {
				atom_list[i++] = xv_get(server,
					SERVER_ATOM, type_string);
			}

			if (at->at_file != NULL) {
				atom_list[i++] = xv_get(server,
					SERVER_ATOM, "_SUN_FILE_HOST_NAME");
				atom_list[i++] = xv_get(server,
					SERVER_ATOM, "FILE_NAME");
			}

			if (sd->sd_an && sd->sd_an->an_pending) {
				/* Attchment is "in progress".  Set compose
				 * type.  This makes the "done" button
				 * appear in audiotool
				 */
				atom_list[i++] = xv_get(server,
					SERVER_ATOM, "_SUN_COMPOSE_TYPE");
			}
		}

		if (sd->sd_loadok) {
			atom_list[i++] = xv_get(server,
					SERVER_ATOM, "_SUN_LOAD");
		}

		/* finally if the type is _SUN_TYPE_text then add string too */
		/* 1115450: make sure type_string is valid */
		if (type_string != NULL && STREQ(type_string, "_SUN_TYPE_text"))
		{
			atom_list[i++] = XA_STRING;
		}

		*format = 32;
		*length = i;
		*data = (Xv_opaque) atom_list;
		*type = XA_ATOM;

	} else if (STREQ(atom_name, "_SUN_COMPOSE_TYPE") && sd->sd_an &&
		sd->sd_an->an_pending)
	{
		
		*format = 8;
		*length = 9;
		*data = (Xv_opaque) "mail-file";
		*type = XA_STRING;

	} else if (STREQ(atom_name, "_SUN_EXECUTABLE")) {
		static executable;


		if (at) {
			executable = (int) attach_methods.at_get(at,
				ATTACH_EXECUTABLE);
		} else {
			executable = 0;
		}

		DP(("sel_convert_proc: _SUN_EXECUTABLE %d\n", executable));

		*format = 32;
		*length = 1;
		*data = (Xv_opaque) &executable;
		*type = XA_INTEGER;

	} else if (STREQ(atom_name, "HOST_NAME")) {
		char *hostname = myhostname();;

		DP(("sel_convert_proc: HOST_NAME %s\n", hostname));

		*format = NBBY;
		*length = strlen(hostname);
		*data = (Xv_opaque) hostname;
		*type = XA_STRING;

	} else if (STREQ(atom_name, "_SUN_FILE_HOST_NAME")) {
		char *hostname = myhostname();;

		DP(("sel_convert_proc: _SUN_FILE_HOST_NAME %s\n", hostname));

		*format = NBBY;
		*length = strlen(hostname);
		*data = (Xv_opaque) hostname;
		*type = XA_STRING;

	} else if (STREQ(atom_name, "_SUN_DATA_LABEL")) {
		char *label;


		if (! at) goto error;
 		label = attach_methods.at_get(at, ATTACH_DATA_NAME);

		if (! label)
		label = attach_methods.at_get(at, ATTACH_DATA_TYPE);

		if (! label) goto error;

		DP(("sel_convert_proc: _SUN_DATA_LABEL %s\n", label));

		*format = NBBY;
		*length = strlen(label);
		*data = (Xv_opaque) label;
		*type = XA_STRING;

	} else if (STREQ(atom_name, "FILE_NAME")) {
		char *file;


		/* you must have a current object for this atom */
		if (! at) goto error;

		if ((file = at->at_file) == NULL) {
			/* don't already have a file name */
			goto error;
		}

		DP(("sel_convert_proc: FILE_NAME %s\n", file));

		*format = NBBY;
		*length = strlen(file);
		*data = (Xv_opaque) file;
		*type = XA_STRING;

	} else if (STREQ(atom_name, "_SUN_AVAILABLE_TYPES")) {
		static Atom type_atom;
		char *attach_type;
		char *atom_string;

		/* you must have a current object for this atom */
		if (! at) goto error;

		atom_string = get_type_string(at);
		if (! atom_string) {
			/* sigh... this should not really ever happen --
			 * the type string is too large
			 */
			goto error;
		}

		DP(("sel_convert_proc: _SUN_AVAILABLE_TYPES atom_string = %s\n",
			atom_string));


		type_atom = (Atom) xv_get(server, SERVER_ATOM, atom_string);
		free(atom_string);

		*format = 32;
		*length = 1;
		*data = (Xv_opaque) &type_atom;
		*type = XA_ATOM;

	} else if (STREQ(atom_name, "_SUN_LENGTH_TYPE")) {
		static data_length;
		Sel_prop_info *propInfo;

		/* you must have a current object for this atom */
		if (! at) goto error;

		propInfo = (Sel_prop_info *) xv_get(seln,SEL_PROP_INFO);

		DP((
	"sel_convert_proc: type %d/%s, data %d <%s>, length %d, format %d\n",
			propInfo->type,
			propInfo->typeName ? propInfo->typeName : "<null>",
			propInfo->data,
			propInfo->data,
			propInfo->length,
			propInfo->format));

		if (! propInfo) goto error;

		data_length = (int)
			attach_methods.at_get(at, ATTACH_CONTENT_LEN);

		DP(("sel_convert_proc: _SUN_LENGTH_TYPE, length = %d\n",
			data_length));

		*format = 32;
		*length = 1;
		*data = (Xv_opaque) &data_length;
		*type = XA_INTEGER;

		free(propInfo);

	} else if (STREQ(atom_name, "STRING")) {
		char *attach_type;

		/* you must have a current object for this atom */
		if (! at) goto error;

		/* we used to insist that you had a text type.  But
		 * fuck it -- xview doesn't know about TEXT and refuses
		 * to use it, so we'll just treat STRING like we treat
		 * TEXT
		 */
		goto do_type_convert;

	} else if (STREQ(atom_name, "TEXT")) {
		char *type_string;

		/* you must have a current object for this atom */
		if (! at) goto error;

		/* text is "polymorphic" -- it means that its OK to give
		 * anything
		 */

		type_string = get_type_string(at);

		*type = xv_get(server, SERVER_ATOM, type_string);
		free(type_string);

		goto do_type_convert;

	} else if (STRNEQ(atom_name, "_SUN_TYPE_", 10)) {
		char *attach_type;

		/* you must have a current object for this atom */
		if (! at) goto error;

		attach_type = (char *)
			attach_methods.at_get(at, ATTACH_DATA_TYPE);

		if (!STREQ(&atom_name[10], attach_type)) {

			/* they asked for a type that we can't convert */
		DP(("sel_convert_proc: failed attempt to convert type %s\n",
				atom_name));

			goto error;
		}
do_type_convert:

		/* go ahead and return it */
		*length = (int)
			attach_methods.at_get(at, ATTACH_CONTENT_LEN);
		*data = (Xv_opaque)
			attach_methods.at_get(at, ATTACH_BODY);
		*format = 8;
		/* don't change the type */

		DP(("sel_convert_proc: converting type %s, length %d\n",
			atom_name, *length));
	} else if (STREQ(atom_name, "_SUN_LOAD")) {
		static tmp_atom;

		/* We got a load.  Give back a selection on the
		 * original object.
		 */

		/* you must have a current object for this atom */
		if (! at) goto error;

		if (! sd->sd_loadok) goto error;

		tmp_atom = conjure_load(server, sd);

		if (! tmp_atom) goto error;

		DP(("sel_convert_proc: sending off a _SUN_LOAD...\n"));

		*length = 1;
		*format = 32;
		*data = (Xv_opaque) &tmp_atom;
		*type = XA_ATOM;

	} else if (STREQ(atom_name, "_SUN_ENUMERATION_COUNT")) {

		DP(("sel_convert_proc: returning enumeration count of %d\n",
			sd->sd_atcount));

		*length = 1;
		*format = 32;
		*type = XA_INTEGER;
		*data = (Xv_opaque) &sd->sd_atcount;

	} else if (STREQ(atom_name, "_SUN_ENUMERATION_ITEM")) {
		Sel_prop_info *propInfo;
		unsigned item;

		propInfo = (Sel_prop_info *) xv_get(seln,SEL_PROP_INFO);

		if (! propInfo) goto error;

		DP((
	"sel_convert_proc: type %d/%s, data %d, length %d, format %d\n",
			propInfo->type,
			propInfo->typeName ? propInfo->typeName : "<null>",
			propInfo->data ? *(unsigned *)propInfo->data : -1,
			propInfo->length,
			propInfo->format));

		/* if the length is not at least 2, format 32 then give up */
		if (propInfo->length != 1 || propInfo->format != 32 ||
			propInfo->type != XA_INTEGER || ! propInfo->data)
		{
			free(propInfo);
			goto error;
		}

		item = *(unsigned *)propInfo->data;

		free(propInfo);

		/* can't set to an item that's too large... */
		if (item > sd->sd_atcount && item != ~0) goto error;

		DP(("sel_convert_proc: setting the current enumeration to %d\n",
			item));

		/* actually set the current item */
		if (item == ~0) {
			/* reset the at to the default state */
			if (sd->sd_atcount == 1) {
				sd->sd_at = sd->sd_atlist[0];
			} else {
				sd->sd_at = NULL;
			}
		} else {
			sd->sd_at = sd->sd_atlist[item];
		}

		/* flag our success */
		*length = 0;
		*format = 32;
		*data = 0;
		*type = get_null_atom(server);

	} else if (STREQ(atom_name, "_SUN_SELECTION_ERROR") ||
		STREQ(atom_name, "_SUN_SAMEWIN_ERROR"))
	{

		if (STREQ(atom_name, "_SUN_SELECTION_ERROR")) {
			mt_frame_msg(sd->sd_errorframe, TRUE,
				gettext("Data Transfer: Failed"));
		}

		/* mark the fact that there was an error */
		sd->sd_errtarget = 1;

		*length = 0;
		*format = 32;
		*data = 0;
		*type = get_null_atom(server);

	} else if (STREQ(atom_name, "DRAWABLE")) {

		*data = (Xv_opaque) &sd->sd_drawable;
		*length = 1;
		*format = 32;
		*type = XA_DRAWABLE;

	} else if (STREQ(atom_name, "NAME")) {

		/* return the name of the application */
		*length = strlen(mt_cmdname);
		*format = 8;
		*data = (Xv_opaque) mt_cmdname;
		*type = XA_STRING;

	} else if (STREQ(atom_name, "INSERT_SELECTION")) {
		Sel_prop_info *propInfo;
		Atom insert_sel;
		Atom insert_target;

		/* we are being called to insert the contents
		 * of the specified selection in order to
		 * overwrite the current selection.
		 *
		 * We insist that this is only legal if the
		 * current selection is the result of a "load".
		 * Is this reasonable?  Probably, but I'm not
		 * sure.
		 */

		if (! sd->sd_load) goto error;

		DP(("sel_convert_proc: INSERT_SELECTION after a load\n"));

		/* go and get the selection and target to use */

		propInfo = (Sel_prop_info *) xv_get(seln,SEL_PROP_INFO);

		if (! propInfo) goto error;

		DP((
	"sel_convert_proc: type %d/%s, data %d <%s>, length %d, format %d\n",
			propInfo->type,
			propInfo->typeName ? propInfo->typeName : "<null>",
			*(unsigned *)propInfo->data,
			xv_get(server, SERVER_ATOM_NAME,
				*(unsigned *)propInfo->data),
			propInfo->length,
			propInfo->format));


		/* if the length is not at least 2, format 32 then give up */
		if (propInfo->length < 2 || propInfo->format != 32) {
			free(propInfo);
			goto error;
		}

		insert_sel = *(Atom *) propInfo->data;
		insert_target = *(((Atom *) propInfo->data) + 1);

		free(propInfo);

		/* we now have to conjure up an xview selection,
		 * convert the target, grab the data, and then
		 * replace the type with the new type
		 */
		if (! do_insert_selection(insert_sel, insert_target,
			sd->sd_window, sd->sd_server, sd->sd_al, sd->sd_an,
			sd->sd_at))
		{
			goto error;
		}

		/* flag our success */
		*length = 0;
		*format = 32;
		*data = 0;
		*type = get_null_atom(server);

	} else {
		DP(("sel_convert_proc: calling the default convert proc\n"));
		retval = sel_convert_proc(seln, type, data, length, format);
	}

end:
	if ((multiple_type & SEL_END_MULTIPLE) || multiple_type == 0) {

		DP(("sel_convert_proc: resetting ancount\n"));

		/* reset the an to the default state -- we're at the end
		 * of a multiple here...
		 */
		if (sd->sd_atcount == 1) {
			sd->sd_at = sd->sd_atlist[0];
		} else {
			sd->sd_at = NULL;
		}

		if (sd->sd_seen_selend) {
			dnd_setitimer(sd);
		}
	}

	return (retval);

error:
	*length = 0;
	*format = 0;
	*data = NULL;
	*type = get_null_atom(server);
	retval = False;
	goto end;
}

/*
 * drag_headermsg() creates a dummy message and attaches to it copies of the
 * mail messages being dragged by the user out of the header window.  It
 * then uses the normal attachment DnD code to handle the DnD. This routine
 * destroys this dummy message an its attachments.
 */
void
mt_drag_headermsg_cleanup(al)

	Attach_list	*al;

{
	struct msg	*m;

	DP(("mt_drag_headermsg_cleanup\n"));

	/*
	 * Destroy the dummy message and attachment list we created in
	 * drag_headermsg().  Destroy al first since it uses the message.
	 */
	if (al != NULL) {
		m = al->al_msg;
		mt_destroy_attach_list(al);
		al = NULL;
	}

	if (m != NULL) {
		msg_methods.mm_free_msg(m);
	}

	return;
}



static void *
clear_sd_by_message(ListNode *ln, void* data, va_list va)
{
	struct sel_data *sd = data;
	struct msg *msg;

	/* the two arguments in the va_list are a selection and its rank */
	msg = va_arg(va, struct msg *);

	if (msg == sd->sd_msg) {
		/* get rid of this transaction */
		DP(("clear_sd_by_message: clearing sd %x\n", sd));
		sd->sd_cancelled = 1;
		dnd_setitimer(sd);
	}

	return (NULL);
}


/*
 * stop any pending transactions from ocurring on the specified message --
 * it is about to go away
 */
void
mt_attach_clear_msg(struct msg *msg)
{
	DP(("mt_attach_clear_msg: called for msg %x\n", msg));
	if (msg == NULL) return;
	(void) enumerate_listnode(&validSelnList, clear_sd_by_message, msg);
}


/* stop pending transactions on a folder */
static void *
clear_sd_by_hd(ListNode *ln, void* data, va_list va)
{
	struct sel_data *sd = data;
	HeaderDataPtr hd;

	/* the two arguments in the va_list are a selection and its rank */
	hd = va_arg(va, HeaderDataPtr);

	if (hd->hd_frame == sd->sd_errorframe) {
		/* get rid of this transaction */
		DP(("clear_sd_by_hd: clearing sd %x\n", sd));
		sd->sd_cancelled = 1;
		dnd_setitimer(sd);
	}

	return (NULL);
}

void
mt_attach_clear_folder(HeaderDataPtr hd)
{
	DP(("mt_attach_clear_folder: called for hd %x/%s\n",
		hd, hd->hd_folder_name));
	(void) enumerate_listnode(&validSelnList, clear_sd_by_hd,
		hd);
}

void
mt_attach_remove_cleanup_proc(Selection sel)
{
        struct sel_data *sd = (struct sel_data *)xv_get(sel, XV_KEY_DATA, 
							ATTACH_KEY_DATA);
	sd->sd_cleanup_proc = NULL;
	sd->sd_cleanup_arg = NULL;
}
