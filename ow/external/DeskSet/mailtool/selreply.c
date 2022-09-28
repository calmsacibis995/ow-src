#ident  "@(#)selreply.c 3.13 94/10/24 Copyr 1987 Sun Micro"

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
#include "tool.h"
#include "tool_support.h"
#include "mail.h"
#include "mle.h"
#include "../maillib/obj.h"
#include "../maillib/ck_strings.h"
#include "../maillib/assert.h"

#define DEBUG_FLAG mt_debugging
#include "debug.h"


static int SEL_KEY_DATA;

#define STREQ(a,b)	(!strcmp((a),(b)))
#define STRNEQ(a,b,n)	(!strncmp((a),(b),(n)))



struct reply_data {
	char		*rd_file_host_name;
	char		*rd_file_name;
	char		*rd_data_label;
	int		rd_enumeration_count;
	int		rd_enumeration_item;
	unsigned	rd_num_added;
	unsigned	rd_bytes_added;
	Atom		rd_sel_type;
	int		rd_site_id;
	int		rd_executable;
	
	char		*rd_data_list[3];
	Attach_list	*rd_al;
	Attach_node	*rd_an;	/* only needed if an_pending is true */
	struct attach	*rd_at;
};

struct sel_state {
	void	*ss_data;
	Xv_server ss_server;

	void	*ss_incrbuffer;
	int	ss_seen_incr : 1;
	int	ss_allocated_data :1;
	int	ss_failure : 1;
	int	ss_destroying : 1;


	int	ss_numrequests;
	int	ss_thisrequest;
	void	**ss_reqarray;
	void	(*ss_replyproc)();
	void	(*ss_cleanupproc)();
	void	*ss_userdata;
	char	**ss_fail;
	Atom	ss_incr_atom;
	Atom	ss_text_atom;
	Selection_requestor ss_selreq;
	Xv_opaque ss_window;
};

static void setup_request();
static struct sel_state *alloc_selstate();
static void destroy_selstate();
static void cleanup_reply_data();
static void reply_next_object();
static void reply_setfailure();






/*
 * we got the last incr selection.  linearize it into one
 * big buffer
 */
static int
convert_incr(ss, valuep, format)
struct sel_state *ss;
void **valuep;
unsigned format;
{
	int size;
	char *ptr;
	struct incr_data *id;

	/* first figure out how large a buffer we need */
	size = mb_size(ss->ss_incrbuffer);

	/* don't bother if there is no data */
	if (size == 0) {
		mb_free(ss->ss_incrbuffer);

		*valuep = NULL;
		return (0);
	}

	DP(("convert_incr: buffer size is %d\n", size));

	/* allocate a buffer big enough for the whole thing */
	*valuep = ptr = ck_zmalloc(size);

	if (! ptr) return (-1);
	ss->ss_allocated_data = 1;

	/* now copy the data into the buffer */
	mb_read(ss->ss_incrbuffer, ptr, size);

	mb_free(ss->ss_incrbuffer);
	ss->ss_incrbuffer = NULL;

	/* now compute the number of elements */
	format /= 8;			/* turn into bytes */
	if (format == 0) format = 1;	/* sanity check */
	return (size / format);

}


static void
cleanup_proc (ss, target, type, replyValue, length, format)
struct sel_state *ss;
Xv_opaque	replyValue;
Atom		target;
Atom		type;
int		length;
int		format;
{

	DP(("selection cleanup_proc(): called, target %d\n", target));

	if (target == 0) {
		destroy_selstate(ss);
	}
}



static void
reply_get_data(ss, target, type, replyValue, length, format)
struct sel_state *ss;
Xv_opaque	replyValue;
Atom		target;
Atom		type;
int		length;
unsigned	format;
{
	struct reply_data *rd = ss->ss_userdata;
	struct reply_panel_data *rpd;
	char *type_string;
	char *label;
	Attach_node *an;

#ifdef DEBUG
	char *targetname;
	if (target != NULL) {
		targetname = (char *) 
			xv_get(ss->ss_server, SERVER_ATOM_NAME, target);
	} else {
		targetname = "<null>";
	}
#endif DEBUG

	DP(("reply_get_data: called for %d/%s\n", target,
		targetname ? targetname : "<null>"));

	if (target == NULL) {
		/* do nothing... */
	} else if (ss->ss_thisrequest == 2) {

		/* this was the data request.  See if it succeeded */
		if (type) {
			type_string = (char *)
				xv_get(ss->ss_server, SERVER_ATOM_NAME, type);

			/* figure out what the icon name should be */
			if (rd->rd_data_label) {
				label = rd->rd_data_label;
			} else if (rd->rd_file_name) {
				label = strrchr(rd->rd_file_name, '/');
				if (label) label++;
				else label = rd->rd_file_name;
			} else {
				label = "";
			}

			/* convert the length to bytes */
			length /= (format / 8);

			/* if the data hasn't been copied, do so now */
			if (! ss->ss_allocated_data) {
				void *data;

				data = ck_zmalloc(length);
				if (! data) {
					reply_setfailure(ss, TRUE);
					ck_free(replyValue);
					mt_frame_msg(rd->rd_al->al_errorframe,
							TRUE,
				gettext("Data transfer failed: Out of memory"));
					return;
				}

				memcpy(data, (char *) replyValue, length);
				ck_free(replyValue);
				replyValue = (Xv_opaque) data;
			}

			/* check for leading "_SUN_TYPE_" */
			if (STRNEQ(type_string, "_SUN_TYPE_", 10)){
				type_string = &type_string[10];
			} else {
				type_string = mt_get_data_type(label,
					(void *) replyValue, length);
			}

			/*
			 * This transaction may have been the result of a
			 * synthetic drop on the textsw (ie we were sent a
			 * tooltalk message to bring up a compose window
			 * and place data in the textsw). We check for that
			 * case here.
			 */
	 		rpd = (struct reply_panel_data *)
			      xv_get(rd->rd_al->al_frame,
				     WIN_CLIENT_DATA);
			if (rpd != NULL && ss->ss_window == rpd->replysw) {
				mt_text_clear_error(rpd->replysw);
				mt_text_insert(rpd->replysw,
					(char *) replyValue, length);
			} else {
				add_attachment_memory(rd->rd_al,
					rd->rd_al->al_msg, (void *) replyValue,
					length, type_string, label,
					rd->rd_executable);
			}

			rd->rd_enumeration_item++;
			rd->rd_num_added++;
			rd->rd_bytes_added += length;

			/* Bugid 1070844: reset the flags for next object */
			rd->rd_executable = 0;
			ss->ss_allocated_data = 0;

			reply_next_object(ss);

		} else {
			/* the conversion failed.  Try it again? */
			if (target == ss->ss_text_atom) {
				rd->rd_data_list[1] = "STRING";
			} else if (target == XA_STRING) {
				/* there is nothing else to try... */
				reply_setfailure(ss, TRUE);
				mt_frame_msg(rd->rd_al->al_errorframe, TRUE,
					gettext("Data transfer failed"));
				return;
			} else {
				rd->rd_data_list[1] = "TEXT";
			}

			setup_request(ss, SEL_TYPE_NAMES, rd->rd_data_list,
				reply_get_data);
			
			xv_set(ss->ss_selreq,
				SEL_TYPE_INDEX, 0,
					SEL_PROP_TYPE, XA_INTEGER,
					SEL_PROP_DATA, &rd->rd_enumeration_item,
					SEL_PROP_FORMAT, 32,
					SEL_PROP_LENGTH, 1,
				0);

			sel_post_req(ss->ss_selreq);

		}

#ifdef DEBUG
	} else if (STREQ(targetname, "_SUN_ENUMERATION_ITEM")) {
		/* do nothing */
	} else {
		DP(("\nreply_get_data: unexpected target %s\n\n", targetname));
#endif DEBUG

	}
}



/*
 * called when we are dropping on the header window and are done with
 * the first round of questions
 */
static void
reply_enquiry_header(ss)
struct sel_state *ss;
{
	struct reply_data *rd = ss->ss_userdata;
	extern char *myhostname();
	struct header_data *hd;

	hd = mt_get_header_data(ss->ss_window);
	if (! rd->rd_file_name) {
		mt_vs_warn(rd->rd_al->al_errorframe, gettext(
"You must drop a file with a name here (for now...)"));
		reply_setfailure(ss, TRUE);
		return;
	}


	if (! rd->rd_file_host_name ||
		! STREQ(rd->rd_file_host_name, myhostname()))
	{
		mt_vs_warn(rd->rd_al->al_errorframe, gettext(
"The file you dropped doesn't seem to be reachable from here"));
		reply_setfailure(ss, TRUE);
		return;
	}


	DP(("\nreply_enquiry_header: time to get the file...\n\n"));
	mt_new_folder(hd, rd->rd_file_name, FALSE, FALSE, FALSE, TRUE);
}




static void
reply_enquiry(ss, target, type, replyValue, length, format)
struct sel_state *ss;
Xv_opaque	replyValue;
Atom		target;
Atom		type;
int		length;
unsigned	format;
{
	char *targetname;
	char *typename;
	struct reply_data *rd = ss->ss_userdata;

	targetname = ss->ss_reqarray[ss->ss_thisrequest -1];


	if (target == NULL) {

		if (rd->rd_site_id == 0) {
			/* this was a drop on the header window */
			reply_enquiry_header(ss);
			xv_set(ss->ss_selreq,
                		SEL_TYPES, xv_get(ss->ss_server, 
				SERVER_ATOM, "_SUN_SELECTION_END"), 0, 0);

        		sel_post_req(ss->ss_selreq);
			return;
		}


		/* OK, we're done.  Try and get the data... */

		/* ZZZ: in a perfect world, we would ask about the
		 * length here, but since we do nothing with the info
		 * yet, we don't bother.
		 */

		if (rd->rd_sel_type) {
			typename = (char *) xv_get(ss->ss_server,
				SERVER_ATOM_NAME, rd->rd_sel_type);
		} else {
			typename = "TEXT";
		}
		DP(("reply_enquiry: setting first type name to %d/%s\n",
			rd->rd_sel_type, typename));

		rd->rd_data_list[0] = "_SUN_ENUMERATION_ITEM",
		rd->rd_data_list[1] = typename;
		rd->rd_data_list[2] = 0;

		setup_request(ss, SEL_TYPE_NAMES, rd->rd_data_list,
			reply_get_data);
		
		xv_set(ss->ss_selreq,
			SEL_TYPE_INDEX, 0,
				SEL_PROP_TYPE, XA_INTEGER,
				SEL_PROP_DATA, &rd->rd_enumeration_item,
				SEL_PROP_FORMAT, 32,
				SEL_PROP_LENGTH, 1,
			0);

		sel_post_req(ss->ss_selreq);

	} else if (STREQ(targetname, "_SUN_FILE_HOST_NAME")) {
		if (type == XA_STRING) {
			rd->rd_file_host_name = (char *) replyValue;
			DP(("reply_enquiry: setting rd_file_host_name to %s\n",
				rd->rd_file_host_name));
		} else {
			DP(("reply_enquiry: FILE_HOST_NAME not a STRING\n"));
		}
	} else if (STREQ(targetname, "FILE_NAME")) {
		if (type == XA_STRING) {
			rd->rd_file_name = (char *) replyValue;
			DP(("reply_enquiry: setting rd_file_name to %s\n",
				rd->rd_file_name));
		} else {
			DP(("reply_enquiry: FILE_NAME not a STRING\n"));
		}
	} else if (STREQ(targetname, "_SUN_DATA_LABEL")) {
		if (type == XA_STRING) {
			rd->rd_data_label = (char *) replyValue;
			DP(("reply_enquiry: setting rd_data_label to %s\n",
				rd->rd_data_label));
		} else {
			DP(("reply_enquiry: _SUN_DATA_LABEL not a STRING\n"));
		}
	} else if (STREQ(targetname, "_SUN_EXECUTABLE")) {
		if (type == XA_INTEGER) {
			rd->rd_executable = *(int *) replyValue;
			DP(("reply_enquiry: setting rd_executable to %d\n",
				rd->rd_executable));
		} else {
			DP(("reply_enquiry: _SUN_EXECUTABLE not a INT\n"));
		}
	} else if (STREQ(targetname, "_SUN_AVAILABLE_TYPES")) {

		if (type == XA_ATOM || length >= 1) {
			rd->rd_sel_type = *(Atom *) replyValue;
			DP(("reply_enquiry: setting rd_sel_type to to %d\n",
				rd->rd_sel_type));
		} else {
			DP(("reply_enquiry: AVAIL_TYPES not type ATOM\n"));
		}

#ifdef DEBUG
	} else if (STREQ(targetname, "_SUN_ENUMERATION_ITEM")) {
		/* do nothing */
	} else {
		DP(("\nreply_count: unexpected target %s!!!\n\n", targetname));
#endif DEBUG

	}
}


static char *reply_next_names[] = {
	"_SUN_ENUMERATION_ITEM",
	"_SUN_FILE_HOST_NAME",
	"FILE_NAME",
	"_SUN_DATA_LABEL",
	"_SUN_AVAILABLE_TYPES",
	"_SUN_EXECUTABLE",
	0,
};

static char *reply_end_strings[] = {
	"_SUN_SELECTION_ERROR", "_SUN_SELECTION_END", "_SUN_DRAGDROP_DONE", 0
};

static char *reply_samewin_strings[] = {
	"_SUN_SAMEWIN_ERROR", "_SUN_DRAGDROP_DONE", 0
};



/*
 * the common code that launches the set of enquiry ops
 * to get the next object
 */
static void
reply_next_object(ss)
struct sel_state *ss;
{
	struct reply_data *rd = ss->ss_userdata;

	if (rd->rd_enumeration_item >= rd->rd_enumeration_count) {
		char buffer[128];
		int kbytes;
		int numbytes;

		DP(("reply_next_object: we're done now...\n"));

		/* we've done all the objects */
		setup_request(ss, SEL_TYPE_NAMES, &reply_end_strings[1],
			cleanup_proc);

		/* STRING_EXTRACTION -
		 *
		 * the frame message for how much we added.  The first
		 * %d is the number of attachments; the second is
		 * the number of kilobytes in those attachments.
		 *
		 * the entire message, including the expanded sizes,
		 * cannot be more than 128 bytes.
		 */
		sprintf(buffer, gettext(
			"Added %d attachments for a total of %dk bytes"),
			rd->rd_num_added,
			(rd->rd_bytes_added + 1023) / 1024  );
		mt_frame_msg(rd->rd_al->al_errorframe, FALSE, buffer);

	} else {
		char **next_list;

		if (rd->rd_site_id == 0) {
			/* this is a drop on the header window.  Insist
			 * that it be a single object
			 */

			if (rd->rd_enumeration_count != 1) {
				mt_vs_warn(rd->rd_al->al_errorframe,
					gettext(
"Mailtool can only deal with the drop of a single\
mail file on this header pane"));
				reply_setfailure(ss, TRUE);
				return;

			}
		}

		DP(("reply_next_object: now working on item # %d\n",
			rd->rd_enumeration_item));

		setup_request(ss, SEL_TYPE_NAMES, reply_next_names,
			reply_enquiry);
		
		xv_set(ss->ss_selreq,
			SEL_TYPE_INDEX, 0,
				SEL_PROP_TYPE, XA_INTEGER,
				SEL_PROP_DATA, &rd->rd_enumeration_item,
				SEL_PROP_FORMAT, 32,
				SEL_PROP_LENGTH, 1,
			0);
	}

	sel_post_req(ss->ss_selreq);
}


/*
 * this is the first reply proc; it gathers up the enumeration
 * count and the XID of a drawable to detect drops in the same window
 */
static void
reply_count(ss, target, type, replyValue, length, format)
struct sel_state *ss;
Xv_opaque	replyValue;
Atom		target;
Atom		type;
int		length;
unsigned	format;
{
	char *targetname;
	struct reply_data *rd = ss->ss_userdata;

	targetname = ss->ss_reqarray[ss->ss_thisrequest -1];


	if (target == NULL) {
		/* called after all the targets have been processed */
		if (! ss->ss_fail) {
			reply_next_object(ss);
		}
	} else if (STREQ(targetname, "_SUN_ENUMERATION_COUNT")) {

		/* set the default */
		rd->rd_enumeration_count = 1;
		rd->rd_enumeration_item = 0;

		if (type == XA_INTEGER && length >= 1) {
			rd->rd_enumeration_count = *(int *) replyValue;
			DP(("reply_count: setting ENUMERATION_COUNT to %d\n",
				rd->rd_enumeration_count));
		} else {
			DP(("reply_count: no ENUMERATION_COUNT returned\n"));
		}

	} else if (STREQ(targetname, "DRAWABLE")) {

		/* get the drawable information */
		ASSERT(rd->rd_al);

		if (type == target && length == 1 && format == 32) {
			if (rd->rd_al->al_drawable == *(Drawable *) replyValue){
				DP(("reply_count: same window\n"));
				mt_frame_msg(rd->rd_al->al_errorframe, FALSE,
					gettext(
					"Ignoring drag and drop on same window"
					));
				reply_setfailure(ss, FALSE);
			} else {
				DP(("reply_count: different window\n"));
			}
		} else {
			DP(("reply_count: no drawable returned\n"));
		}

	} else {
		DP(("\nreply_count: unexpected target %s!!!\n\n", targetname));
	}
}



char *mt_received_drop_strings[] = {
	"_SUN_ENUMERATION_COUNT", "DRAWABLE", 0
};

void
mt_received_drop(canvas, event, al)
Canvas canvas;
Event *event;
Attach_list *al;
{
	Xv_server		server;
	struct sel_state	*ss;
	Xv_drop_site		drop_site;
	struct reply_data	*rd;


	server = XV_SERVER_FROM_WINDOW(event_window(event));

	ss = alloc_selstate(canvas, server, sizeof (struct reply_data),
		cleanup_reply_data);

	if (! ss) {
		return;
	}

	rd = ss->ss_userdata;
	rd->rd_al = al;

	drop_site = dnd_decode_drop(ss->ss_selreq, event);

	if (drop_site == XV_ERROR) {
		DP(("mt_received_drop: coun't decode drop site\n"));
		destroy_selstate(ss);
		return;
	}

	rd->rd_site_id = xv_get(drop_site, DROP_SITE_ID);

	DP(("mt_received_drop: converting first type. Site ID %d\n",
		rd->rd_site_id));

	setup_request(ss, SEL_TYPE_NAMES, mt_received_drop_strings,
		reply_count);

	DP(("mt_received_drop: rank %s/%d; about to post the request.\n",
		xv_get(ss->ss_selreq, SEL_RANK_NAME),
		xv_get(ss->ss_selreq, SEL_RANK)));
	
	sel_post_req(ss->ss_selreq);
}

void
mt_received_synthetic_drop(window, al, transient_selection)
Xv_opaque window;
Attach_list *al;
Atom	transient_selection;
{
	Xv_server		server;
	struct sel_state	*ss;
	Xv_drop_site		drop_site;
	struct reply_data	*rd;

	server = XV_SERVER_FROM_WINDOW(al->al_frame);

	ss = alloc_selstate(window, server, sizeof (struct reply_data),
		cleanup_reply_data);

	if (! ss) {
		return;
	}

	rd = ss->ss_userdata;
	rd->rd_al = al;

	drop_site = al->al_drop_site;
	rd->rd_site_id = xv_get(drop_site, DROP_SITE_ID);

	xv_set(ss->ss_selreq, SEL_RANK, transient_selection, 0);

	DP(("mt_received_drop: converting first type. Site ID %d\n",
		rd->rd_site_id));

	setup_request(ss, SEL_TYPE_NAMES, mt_received_drop_strings,
		reply_count);

	DP(("mt_received_drop: rank %s/%d; about to post the request.\n",
		xv_get(ss->ss_selreq, SEL_RANK_NAME),
		xv_get(ss->ss_selreq, SEL_RANK)));
	
	sel_post_req(ss->ss_selreq);
}



void
insert_reply_proc(ss, target, type, replyValue, length, format)
struct sel_state *ss;
Xv_opaque	replyValue;
Atom		target;
Atom		type;
int		length;
int		format;
{
	char *target_name;
	char *label;
	char *type_string;
	struct attach *at;
	struct reply_data *rd;
	Attach_list	*al;
	int	rcode;


	DP(("insert_reply_proc: called\n"));
	
	/* there should only be one thing to check against; do
	 * we really need to check state?  For now, say "no"
	 */

	/* we now have a length & a value.  store it back into
	 * the sd structure...
	 */

	if (! format) {
		/* there was an error */
		reply_setfailure(ss, TRUE);
		return;

	}

	rd = ss->ss_userdata;
	at = rd->rd_at;

	/* convert the length to bytes */
	length /= (format / 8);	/* convert to bytes */

	if (!ss->ss_allocated_data) {
		void *data;

		/* we haven't copied the data yet.  Do so... */
		format = 8;

		data = ck_zmalloc(length);

		if (! data) {
			reply_setfailure(ss, TRUE);

			/* put a message up... */
			xv_set(ss->ss_window,
				gettext(
"Save back to attachment %s\nfailed due to not enough memory"),
				 mt_attach_name(at), 0);

			return;
		}

		memcpy(data, (char *) replyValue, length);
		ck_free(replyValue);
		replyValue = (Xv_opaque) data;
	}

	DP(("insert_reply_proc: updating attachment\n"));
	attach_methods.at_set(at, ATTACH_MMAP_BODY, replyValue);
	attach_methods.at_set(at, ATTACH_CONTENT_LEN, length);

	al = rd->rd_al;
	if (rd->rd_an && rd->rd_an->an_pending) {
		/* Insert request was on an "attachment in progress".
		 * Add it to the attachment list.
		 */
		DP(("insert_reply_proc: adding pending attachment to list\n"));
		mt_add_attachment(al, rd->rd_an, al->al_msg);
		rd->rd_an->an_pending = FALSE;
	}

	return;
}


/*
 * try and read in a selection into the current object
 *
 * return 0 for failure of any type.
 */

int
do_insert_selection(insert_sel, insert_target, window, server, al, an, at)
Atom insert_sel;
Atom insert_target;
Xv_object window;
Xv_Server server;
Attach_list *al;
Attach_node *an;
struct attach *at;
{
        struct sel_state *ss;
        int length;
        int format;
        int rcode = 1;
        Atom insert_target_array[2];
	struct reply_data *rd;

        DP(("do_insert_selection: sel %d/%s, target %d/%s\n",
                insert_sel,
                xv_get(server, SERVER_ATOM_NAME, insert_sel),
                insert_target,
                xv_get(server, SERVER_ATOM_NAME, insert_target)));
 
        ss = alloc_selstate(window, server,
		sizeof (struct reply_data), cleanup_reply_data);

        if (!ss) return(0);
 
	rd = ss->ss_userdata;
	rd->rd_al = al;
	rd->rd_an = an;
	rd->rd_at = at;
 
        xv_set(ss->ss_selreq,
                SEL_RANK, insert_sel,
                0);
 
        DP(("do_insert_selection: about to get type\n"));
 
        insert_target_array[0] = 
		xv_get(server, SERVER_ATOM_NAME, insert_target);
        insert_target_array[1] = 0;
        setup_request(ss, SEL_TYPE_NAMES, insert_target_array,
		insert_reply_proc);

        /* now get the data */
        (void) xv_get(ss->ss_selreq, SEL_DATA, &length, &format);

        DP(("do_insert_selection: xv_get length %d, format %d\n",
                length, format));

	/* Check if there was an error processing the data */
	if (ss->ss_failure)
		rcode = 0;

        /* we ignore the actual returned data: instead we
         * stash all the usefull stuff via the reply proc in
         * the sel_state structure
         */
	destroy_selstate(ss);

	return(rcode);
}





/*
 * this procedure handles the "boilerplate" code needed to interface
 * with xview.  It compacts incr replies into a single larger data
 * packet, then calls the custom reply proc.
 */
static void
reply_proc(sel_req, target, type, replyValue, length, format)
Selection_requestor sel_req;
Xv_opaque	replyValue;
Atom		target;
Atom		type;
int		length;
unsigned	format;
{
	struct sel_state *ss;
	char *type_name;
	char *target_name;
	char *label;
	char *type_string;
	Atom tmp_atom;
	int sel_error = 0;

	ss = (struct sel_state *) xv_get(sel_req, XV_KEY_DATA, SEL_KEY_DATA);

#ifdef DEBUG
	if (type && ss) {
		type_name = (char *)
			xv_get(ss->ss_server, SERVER_ATOM_NAME, type);
	} else {
		type_name = "[None]";
	}

	if (target && ss) {
		target_name = (char *)
			xv_get(ss->ss_server, SERVER_ATOM_NAME, target);
	} else {
		target_name = "[None]";
	}
		


	DP(("reply_proc: target %s, type %s, val %x, len %d, fnt %d\n",
		target_name, type_name, replyValue, length, format));
#endif DEBUG
	
	/* sanity check... */
	if (! ss) return;

	if (target == xv_get(ss->ss_server, SERVER_ATOM, "_SUN_SELECTION_END") ||
		target == xv_get(ss->ss_server, SERVER_ATOM, "_SUN_DRAGDROP_DONE"))
		return;

	/* this is how xview tells us about errors */
	if (length == SEL_ERROR && type == NULL) {
		struct reply_data *rd = ss->ss_userdata;
		char *string;
		char buf[128];
		int fatalerror = 1;

		sel_error = 1;

		/* just a note -- it is probably a bad idea to use
		 * rd here, because technically ss_userdata may not
		 * point to it.  But at this stage I'm feeling
		 * pretty pragmatic about all this.
		 */

		/* OK, something went wrong.  Lets figure out what */
		switch (*(int *)replyValue) {
		case SEL_BAD_TIME:
			string = gettext(
"Data transfer failed.\n\
The error was a selection\n\
protocol error -- bad time");
			break;

		case SEL_BAD_WIN_ID:
			string = gettext(
"Data transfer failed.\n\
The error was a selection\n\
protocol error -- bad time");
			break;

		case SEL_BAD_PROPERTY:
			string = gettext(
"Data transfer failed.\n\
The error was a selection\n\
protocol error -- bad time");
			break;

		case SEL_BAD_CONVERSION:
			/* this is "OK", and is a normal part of the
			 * protocol
			 */
			fatalerror = 0;
			break;

		case SEL_TIMEDOUT:
			string = gettext(
"Data transfer failed.\n\
\n\
The selection transaction timed out.\n\
If this happens too much, try setting\n\
your selection timeout value to a\n\
larger number of seconds.\n\
\n\
You can do this by adding a line like\n\
\n\
	Selection.Timeout: 10\n\
\n\
to the .Xdefaults file in your home\n\
directory.  The default value is to have\n\
a timeout of three seconds.");
			break;

		case SEL_PROPERTY_DELETED:
			string = gettext(
"Data transfer failed.\n\
The error was an xview selection\n\
protocol error -- property deleted\n\
(This should never happen...)");
			break;

		case SEL_BAD_PROPERTY_EVENT:
			string = gettext(
"Data transfer failed.\n\
The error was an xview selection\n\
protocol error -- bad property event\n\
(This should never happen...)");
			break;

		default:
			string = gettext(
"Data transfer failed.\n\
The error was an unknown xview\n\
selection error (internal xview\n\
error number %d)");
			break;
		}



		if (fatalerror) {
			if (rd) {
				mt_vs_warn(rd->rd_al->al_errorframe, string,
					*(int *)replyValue);
			}

			reply_setfailure(ss, TRUE);
		}
	}


	if (type == ss->ss_incr_atom) {
		extern void nomem();
		extern void *mb_alloc();

		ss->ss_seen_incr = 1;
		ss->ss_incrbuffer = mb_alloc(nomem);
		DP(("reply_proc: setting ss_seen_incr\n"));
		return;
	}

	if (ss->ss_seen_incr) {

		if (! ss->ss_fail) {
			if (length == 0) {
				/* we are at the end of the incr list */
				length = convert_incr(ss, &replyValue, format);

				DP(("reply_proc: clearing seen_incr: len %d\n",
					length));
				ss->ss_seen_incr = 0;
				if (length == -1) {
					reply_setfailure(ss, TRUE);
				}
			} else {
				DP(("reply_proc: incr len %d, format %d\n",
					length, format));
				mb_append(ss->ss_incrbuffer, replyValue,
					length * (format/8));
				return;
			}
		} else if (length == 0) {
			/* we're done with the incr... */
			ss->ss_seen_incr = 0;
		}
	}

	/* sanity check -- make sure the target is what we think it is */
	ASSERT(sel_error ||
		STREQ(target_name, ss->ss_reqarray[ss->ss_thisrequest]));

	/* mark off the fact that we're processing another request... */
	ss->ss_thisrequest++;

	if (! ss->ss_fail) {

		if (ss->ss_replyproc) {
			/* call the subsidiary reply proc */
			(*ss->ss_replyproc)(ss, target, type, replyValue,
				length, format);

		}
	}

	if (ss->ss_thisrequest >= ss->ss_numrequests) {

		/* we need to cache these values in case the
		 * reply proc free's up the ss structure
		 */
		void (*replyproc)() = ss->ss_replyproc;
		char **fail = ss->ss_fail;

		/* we have done the last target; flag it... */
		if (ss->ss_replyproc) {
			(*ss->ss_replyproc)(ss, NULL, NULL, NULL, 0, 0);
		}


		/* if we've failed then cleanup, but make sure we
		 * have a recursion breaker...
		 */
		if (fail && replyproc != cleanup_proc) {
			/* mark us as "dead" */
			setup_request(ss, SEL_TYPE_NAMES, fail, cleanup_proc);
			sel_post_req(ss->ss_selreq);
		}
	}

}




/*
 * set things up for the next set of requests.  We are called with
 * the the sel_state information, an array of the
 * request strings, and the procedure to call back when the
 * reply comes back
 */
static void
setup_request(ss, insert_type, requests, replyproc)
struct sel_state *ss;
unsigned insert_type;
void **requests;
void (*replyproc)();
{

	int count;
	void *tmp[20];
	int i;


	/* count the number of requests */
	for (count = 0; requests[count]; count++);

	ss->ss_numrequests = count;
	ss->ss_reqarray = requests;
	ss->ss_thisrequest = 0;
	ss->ss_replyproc = replyproc;

	/* don't bother if the count is zero */
	if (count == 0) return;


	/* hack, hack -- there is no efficent way to set requests
	 * without doing it inline.  So we have an arbitrary maximum
	 * and rely on the ASSERT to pick this up during debugging.
	 */

	/* make sure we have enought space */
	ASSERT((count+2) < (sizeof(tmp)/sizeof(int)));
	
	for (i = 0; i < count; i++) {
		tmp[i] = requests[i];
	}

	tmp[i] = 0;	/* terminate the SEL_TYPE_NAMES */
	tmp[i+1] = 0;	/* terminate the xv_set */

	xv_set(ss->ss_selreq, insert_type,
			tmp[0], tmp[1], tmp[2], tmp[3], tmp[4],
			tmp[5], tmp[6], tmp[7], tmp[8], tmp[9],
			tmp[10], tmp[11], tmp[12], tmp[13], tmp[14],
			tmp[15], tmp[16], tmp[17], tmp[18], tmp[19],
			0,
		0);

	/* we don't post the request because the caller may
	 * want to set some of the prop info...
	 */
	return;

}


static struct sel_state *
alloc_selstate(window, server, userdata_size, cleanup)
Xv_opaque window;
Xv_server server;
int userdata_size;
void (*cleanup)();

{
	struct sel_state *ss;

	if (! SEL_KEY_DATA) {
		SEL_KEY_DATA = xv_unique_key();
	}

	ss = ck_malloc(sizeof *ss);
	memset((char *)ss, '\0', sizeof *ss);

	ss->ss_selreq = xv_create(window, SELECTION_REQUESTOR,
		SEL_REPLY_PROC, reply_proc,
		XV_KEY_DATA, SEL_KEY_DATA, ss,
		0);

	if (! ss->ss_selreq) {
		ck_free(ss);
		return(NULL);
	}

	ss->ss_server = server;
	ss->ss_window = window;
	ss->ss_cleanupproc = cleanup;
	ss->ss_failure = 0;

	if (userdata_size) {
		ss->ss_userdata = ck_malloc(userdata_size);
		memset(ss->ss_userdata, '\0', userdata_size);
	}

	ss->ss_incr_atom = xv_get(server, SERVER_ATOM, "INCR");
	ss->ss_text_atom = xv_get(server, SERVER_ATOM, "TEXT");

	return (ss);
}



static void
cleanup_reply_data(ss)
struct sel_state *ss;
{
	struct reply_data *rd = ss->ss_userdata;

	if (rd) {
		ck_free(rd->rd_file_host_name);
		ck_free(rd->rd_file_name);
		ck_free(rd->rd_data_label);
		ck_free(rd);
	}

}



/*
 * function called a second after the dnd is done so xview can
 * unwind it's stack
 */
static Notify_func
destroy_selitimer(ss, which)
struct sel_state *ss;
int which;
{
	DP(("destroy_selitimer: called with ss %x\n", ss));

	notify_set_itimer_func((Notify_client)ss, NOTIFY_FUNC_NULL, ITIMER_REAL,
		0, 0);

	if (ss->ss_cleanupproc) {
		(*ss->ss_cleanupproc)(ss);
	}

	if (ss->ss_incrbuffer) {
		mb_free(ss->ss_incrbuffer);
	}

	/* we shouldn't need to do this, but destoyed objects
	 * are still used...
	 */
	xv_set(ss->ss_selreq,
		XV_KEY_DATA, SEL_KEY_DATA, NULL,
		SEL_REPLY_PROC, NULL,
		0);

	xv_destroy_safe(ss->ss_selreq);

	ck_free(ss);
	return (NOTIFY_DONE);
}



static void
destroy_selstate(ss)
struct sel_state *ss;
{
	struct itimerval itv;

	DP(("destroy_selstate(%x)\n", ss));

	/* two sanity checks to avoid race conditions with xview */
	if (! ss) return;

	/* schedule an itimer in a second to do the actual cleanup */
	if (!ss->ss_destroying) {
		itv.it_interval.tv_sec = 0;
		itv.it_interval.tv_usec = 0;
		itv.it_value.tv_sec = 1;
		itv.it_value.tv_usec = 0;
		(void)notify_set_itimer_func((Notify_client)ss,
			(Notify_func)destroy_selitimer, ITIMER_REAL, &itv, 0);

		ss->ss_destroying = 1;
	}

}


static void
reply_setfailure(ss, send_error)
struct sel_state *ss;
int send_error;
{

	DP(("reply_setfailure: ss = %d, send_error = %d\n", ss, send_error));

	if (send_error) {
		ss->ss_fail = reply_end_strings;
	} else {
		ss->ss_fail = reply_samewin_strings;
	}
	
}




#ifdef DEBUG
print_atom_list(title, server, list, length)
char *title;
Xv_server server;
Atom *list;
int length;
{
	char *name;


	if (length <= 0) {
		DP(("%s: empty list\n", title));
		return;
	}

	if (title) DP(("%s:\n", title));

	while (length--) {
		if (*list) {
			name = (char *) xv_get(server, SERVER_ATOM_NAME, *list);
		} else {
			name = "[None]";
		}
		list++;
		DP(("    %s\n", name));
	}
}
#endif DEBUG

