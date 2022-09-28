#ifndef lint
static char sccsid[] = "@(#)dragdrop.c 2.2 90/10/26";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h> /* MAXPATHLEN (include types.h if removed) */
#ifdef SVR4
#include <dirent.h>
#include <netdb.h>
#else
#include <sys/dir.h>   /* MAXNAMLEN */
#endif SVR4
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#ifdef SVR4
#include <string.h>
#else
#include <strings.h>
#endif SVR4
#include <X11/X.h>
#include <xview/defaults.h>
#include <xview/font.h>
#include <xview/notice.h>
#include <xview/frame.h>
#include <xview/xview.h>
#include <xview/scrollbar.h>
#include <xview/panel.h>
#include <xview/sel_pkg.h> 
#include <xview/dragdrop.h>
#include <xview/xv_xrect.h>

#define Min(a,b) (a > b ? b : a)
#define Max(a,b) (a > b ? a : b)

#define MGET(s)    (char *)gettext(s)

/* Temp buffer size for managing INCR transfers thru 
   the selection service */

#define XFER_BUFSIZ	5000

#define TARGET_RESPONSE         0x1
#define HOST_RESPONSE           0x2
#define TRANSPORT_RESPONSE      0x4
#define FILE_NAME_RESPONSE      0x8



/* context structure for describing the current 
   drag/drop action */

typedef struct {
                Atom            *target_list;
                int             num_targets;
                Atom            *transport_list;
                int             num_transports;
                int             object_count;
                char            *source_host;
                char            *source_filename;
                int             transfer_state;
                int             processing_stream;
                int             state_mask;
                int             stop_hit;
                int             objects_transferred;
                } DND_CONTEXT;


static char		Hostname[MAXHOSTNAMELEN];
static DND_CONTEXT	*dnd_context;
static int		debug_on = 0;

static Atom incr_atom;
static Atom targets_atom;
static Atom host_atom;
static Atom file_name_atom;
static Atom atm_file_name_atom;
static Atom selection_end_atom;
static Atom alternate_transport_atom;
static Atom available_types_atom;
static Atom enumeration_count_atom;
static Atom enumeration_item_atom;
static Atom null_atom;


Xv_drop_site    	Drop_site;
Selection_requestor	Sel;
Xv_Server		My_server;

extern Frame 		Tt_frame;
extern Frame 		Tt_panel;
extern Panel_item	drop_site_item;
extern Server_image	drop_site_image;
extern Server_image	busy_drop_site_image;

atom_in_list(atom, atom_list, length)

Atom	atom;
Atom	*atom_list;
int	length;

{
	int	i;

	if (!atom_list)
		return(False);

	for (i = 0; i < length; i++)
		if (atom_list[i] == atom)
			return(True);

	return(False);
}

/* Clears the context block in preparation for 
   a new drag/drop transaction */

clear_context(dnd_ptr, all_state)

DND_CONTEXT	*dnd_ptr;

{
	if (dnd_ptr->target_list)
		free(dnd_ptr->target_list);
	dnd_ptr->target_list = NULL;

	dnd_ptr->num_targets = 0;

	if (all_state)
	{
		dnd_ptr->objects_transferred = 0;
		dnd_ptr->object_count = 0;
	}

	if (dnd_ptr->transport_list)
		free(dnd_ptr->transport_list);
	dnd_ptr->transport_list = NULL;

	dnd_ptr->num_transports = 0;

	if (dnd_ptr->source_host)
		free(dnd_ptr->source_host);
	dnd_ptr->source_host = NULL;

	if (dnd_ptr->source_filename)
		free(dnd_ptr->source_filename);
	dnd_ptr->source_filename = NULL;

	dnd_ptr->state_mask = 0;
	dnd_ptr->stop_hit = 0;
	dnd_ptr->processing_stream = 0;
}

/* Huge monolithic routine that processes all the replies 
   to the questions that I ask about the current drag/drop 
   selection.  These requests are made in groups, and the 
   routine contains a state machine whose current state is stored 
   in the dnd_context block.  This routine updates the 
   state machine as replies, or failed replies come in.  
   Changes in state may require issuing new requests for data, 
   which are processed by this same routine.  */

/*ARGSUSED*/
static void
tapetool_reply_proc ( sel_req, target, type, replyBuf, len, format )

Selection_requestor  	sel_req;
Atom   			target;
Atom   			type;
Xv_opaque   		replyBuf;
unsigned long  		len;
int    			format;
{
	char	*atom_name;
	char	*target_name;
	int	*err_ptr = (int *) replyBuf;
	char	*char_buf = (char *) replyBuf;
	int	old_length;
	Event	event;
	void	file_drop();

	/* try to turn the type and target atoms into 
	   some useful text for debugging. */

	if (debug_on)
	{
		if (type > 0)
			atom_name = XGetAtomName((Display *) xv_get(My_server, XV_DISPLAY), type);
		else
			atom_name =  "[None]" ;
		if (target > 0)
			target_name = XGetAtomName((Display *) xv_get(My_server, XV_DISPLAY), target);
		else
			target_name =  "[None]" ;

		printf( "entered reply proc, type name  = %s, type atom = %d\n target_name = %s, target atom = %d\n len = %d, format = %d, buf = %d\nstate = %d\n" ,
		atom_name, type, target_name, target, len, format, err_ptr, dnd_context->state_mask);
	}

	/* Simply processing the return from the termination 
	   request.  No action necessary */

	if ((target == selection_end_atom) || (target == enumeration_item_atom))
		return;

	if ((len == SEL_ERROR) && ((*err_ptr) == SEL_BAD_CONVERSION))
	{
		/* a conversion of some type failed.  Mark
		   the state variable */

		if (target == targets_atom)
			dnd_context->state_mask |= TARGET_RESPONSE;
		else if (target == host_atom)
			dnd_context->state_mask |= HOST_RESPONSE;
		else if (target == alternate_transport_atom)
			dnd_context->state_mask |= TRANSPORT_RESPONSE;
		else if (target == file_name_atom)
			dnd_context->state_mask |= FILE_NAME_RESPONSE;
	}
	else if (len == SEL_ERROR)
	{
		/* Some internal error happened as a result of 
		   an earlier posted request.  Tell the user. */

		switch (*err_ptr) {
		case SEL_BAD_PROPERTY :
		        xv_set(Tt_frame, FRAME_LEFT_FOOTER,  MGET("ReplyProc: Bad property!") , 0);
          		break;
      		case SEL_BAD_TIME:
          		xv_set(Tt_frame, FRAME_LEFT_FOOTER,  MGET("ReplyProc: Bad time!") , 0);
          		break;
      		case SEL_BAD_WIN_ID:
          		xv_set(Tt_frame, FRAME_LEFT_FOOTER,  MGET("ReplyProc: Bad window id!") , 0);
          		break;
      		case SEL_TIMEDOUT:
          		xv_set(Tt_frame, FRAME_LEFT_FOOTER,  MGET("ReplyProc: Timed out!") , 0);
          		break;
      		case SEL_PROPERTY_DELETED:
          		xv_set(Tt_frame, FRAME_LEFT_FOOTER,  MGET("ReplyProc: Property deleted!") , 0);
          		break;
      		case SEL_BAD_PROPERTY_EVENT:
          		xv_set(Tt_frame, FRAME_LEFT_FOOTER,  MGET("ReplyProc: Bad property event!") , 0);
          		break;
		}
		stop_dragdrop();
		return;
	}
	else if (type == incr_atom)
		dnd_context->processing_stream = TRUE;
	else if (target == targets_atom)
	{
		if (len)
		{
			if (dnd_context->target_list && !dnd_context->processing_stream)
			{
				free(dnd_context->target_list);
				dnd_context->target_list = NULL;
			}

			if (!dnd_context->target_list)
			{
				dnd_context->target_list = (Atom *) malloc(len * sizeof(Atom));
				memcpy((char *)dnd_context->target_list, char_buf, len * sizeof(Atom));
				if (!dnd_context->processing_stream)
					dnd_context->state_mask |= TARGET_RESPONSE;
			}
			else
			{
				dnd_context->target_list = (Atom *) realloc(dnd_context->target_list, dnd_context->num_targets * sizeof(Atom) + len * sizeof(Atom));
				memcpy((char *)&dnd_context->target_list[dnd_context->num_targets - 1], char_buf, len * sizeof(Atom));
			}
		}
		else
		{
			dnd_context->state_mask |= TARGET_RESPONSE;
			dnd_context->processing_stream = FALSE;
		}

		dnd_context->num_targets += len;
	}
	else if (target == alternate_transport_atom)
	{
		if (len)
		{
			if (dnd_context->transport_list && !dnd_context->processing_stream)
			{
				free(dnd_context->transport_list);
				dnd_context->transport_list = NULL;
			}

			if (!dnd_context->transport_list)
			{
				dnd_context->transport_list = (Atom *) malloc(len * sizeof(Atom));
				memcpy((char *)dnd_context->transport_list, char_buf, len * sizeof(Atom));
				if (!dnd_context->processing_stream)
					dnd_context->state_mask |= TRANSPORT_RESPONSE;
			}
			else
			{
				dnd_context->transport_list = (Atom *) realloc(dnd_context->transport_list, dnd_context->num_transports * sizeof(Atom) + len * sizeof(Atom));
				memcpy((char *)&dnd_context->transport_list[dnd_context->num_transports - 1], char_buf, len * sizeof(Atom));
			}
		}
		else
		{
			dnd_context->state_mask |= TRANSPORT_RESPONSE;
			dnd_context->processing_stream = FALSE;
		}

		dnd_context->num_transports += len;
	}
	else if (target == host_atom)
	{
		if (len)
		{
			if (dnd_context->source_host && !dnd_context->processing_stream)
			{
				free(dnd_context->source_host);
				dnd_context->source_host = NULL;
			}

			if (!dnd_context->source_host)
			{
				dnd_context->source_host = malloc(len + 1);
				memcpy(dnd_context->source_host, char_buf, len);
				dnd_context->source_host[len] = NULL;
				if (!dnd_context->processing_stream)
					dnd_context->state_mask |= HOST_RESPONSE;
			}
			else
			{
				old_length = strlen(dnd_context->source_host);
				dnd_context->source_host = (char *) realloc(dnd_context->source_host, old_length + len + 1);
				memcpy(&dnd_context->source_host[old_length], char_buf, len);
				dnd_context->source_host[old_length + len] = NULL;
			}
		}
		else
		{
			dnd_context->state_mask |= HOST_RESPONSE;
			dnd_context->processing_stream = FALSE;
		}
	}
	else if (target == file_name_atom)
	{
		if (len)
		{
			if (dnd_context->source_filename && !dnd_context->processing_stream)
			{
				free(dnd_context->source_filename);
				dnd_context->source_filename = NULL;
			}

			if (!dnd_context->source_filename)
			{
				dnd_context->source_filename = malloc(len + 1);
				memcpy(dnd_context->source_filename, char_buf, len);
				dnd_context->source_filename[len] = NULL;
				if (!dnd_context->processing_stream)
					dnd_context->state_mask |= FILE_NAME_RESPONSE;
			}
			else
			{
				old_length = strlen(dnd_context->source_filename);
				dnd_context->source_filename = (char *) realloc(dnd_context->source_filename, old_length + len + 1);
				memcpy(&dnd_context->source_filename[old_length], char_buf, len);
				dnd_context->source_filename[old_length + len] = NULL;
			}
		}
		else
		{
			dnd_context->state_mask |= FILE_NAME_RESPONSE;
			dnd_context->processing_stream = FALSE;
		}
	}
	else
		return;

	if (dnd_context->state_mask & FILE_NAME_RESPONSE)
	{
		if (debug_on)
			printf( "first batch of replies processed, asking for second\n" );

		if ((!atom_in_list(atm_file_name_atom, dnd_context->transport_list, dnd_context->num_targets)) ||
		    (strcmp(dnd_context->source_host, Hostname)))
		{
			notice_prompt(Tt_frame, &event,
				NOTICE_MESSAGE_STRINGS,
				 MGET("The sourcing application cannot\nGive Tape Tool data thru shared file names.\nThe drop operation has been cancelled.") ,
				0,
				NOTICE_BUTTON_NO,  MGET("Continue") ,
				0);
			
			stop_dragdrop();
			return;
		}

		if (debug_on)
		{
			printf( "should drop object number %d of %d, host: %s, name: %s\n" , 
			dnd_context->objects_transferred + 1, 
			dnd_context->object_count, 
			dnd_context->source_host, 
			dnd_context->source_filename);
		}

		file_drop(dnd_context->source_filename);

		dnd_context->objects_transferred++;

		dnd_context->state_mask = 0;

		begin_item_transfer();
	}

}

/* Stop the drag and drop operation.  Converts _SUN_SELECTION_END 
   on the current selection, signaling the end, and then 
   puts the rest of the application back to normal. */

stop_dragdrop()

{
	if (debug_on)
			printf( "stop_dragdrop called\n" );

	/* Signal termination of transaction */

	xv_set(Sel,
		SEL_REPLY_PROC, tapetool_reply_proc,
		SEL_TYPES, 
			selection_end_atom, 
			0,
		0 );

	sel_post_req(Sel);

	clear_context(dnd_context, TRUE);
}


begin_item_transfer()

{
	int	end_enum_context = -1;

	if (dnd_context->objects_transferred >= dnd_context->object_count)
	{
		stop_dragdrop();
		return;
	}

	xv_set(Sel,
		SEL_REPLY_PROC, tapetool_reply_proc,
		SEL_TYPES, 
			enumeration_item_atom,
			targets_atom,
			host_atom,
			alternate_transport_atom, 
			file_name_atom,
			enumeration_item_atom,
			0,

		SEL_TYPE_INDEX, 0,
			SEL_PROP_TYPE, XA_INTEGER,
			SEL_PROP_DATA, &dnd_context->objects_transferred,
			SEL_PROP_FORMAT, 32,
			SEL_PROP_LENGTH, 1,
		SEL_TYPE_INDEX, 5,
			SEL_PROP_TYPE, XA_INTEGER,
			SEL_PROP_DATA, &end_enum_context,
			SEL_PROP_FORMAT, 32,
			SEL_PROP_LENGTH, 1,
		0 );

	sel_post_req(Sel); 
}
/*ARGSUSED*/
static void
count_reply_proc ( sel_req, target, type, replyBuf, len, format )

Selection_requestor  	sel_req;
Atom   			target;
Atom   			type;
Xv_opaque   		replyBuf;
unsigned long  		len;
int    			format;
{
	int *count_ptr = (int *) replyBuf;

	if ((len == SEL_ERROR) && ((*count_ptr) == SEL_BAD_CONVERSION))
	{
		/* a conversion of some type failed.  Mark
		   the state variable */

		if (target == enumeration_count_atom)
			dnd_context->object_count = 1;
	}
	else if (len == SEL_ERROR)
	{
		/* Some internal error happened as a result of 
		   an earlier posted request.  Tell the user. */

		switch (*count_ptr) {
		case SEL_BAD_PROPERTY :
		        xv_set(Tt_frame, FRAME_LEFT_FOOTER,  MGET("ReplyProc: Bad property!") , 0);
          		break;
      		case SEL_BAD_TIME:
          		xv_set(Tt_frame, FRAME_LEFT_FOOTER,  MGET("ReplyProc: Bad time!") , 0);
          		break;
      		case SEL_BAD_WIN_ID:
          		xv_set(Tt_frame, FRAME_LEFT_FOOTER,  MGET("ReplyProc: Bad window id!") , 0);
          		break;
      		case SEL_TIMEDOUT:
          		xv_set(Tt_frame, FRAME_LEFT_FOOTER,  MGET("ReplyProc: Timed out!") , 0);
          		break;
      		case SEL_PROPERTY_DELETED:
          		xv_set(Tt_frame, FRAME_LEFT_FOOTER,  MGET("ReplyProc: Property deleted!") , 0);
          		break;
      		case SEL_BAD_PROPERTY_EVENT:
          		xv_set(Tt_frame, FRAME_LEFT_FOOTER,  MGET("ReplyProc: Bad property event!") , 0);
          		break;
		}
		stop_dragdrop();
		return;
	}
	else if (target == enumeration_count_atom)
		dnd_context->object_count = *count_ptr;


	if (debug_on)
		printf( "holder had %d items\n" , dnd_context->object_count);

	begin_item_transfer();
}

load_from_dragdrop()

{
	if (debug_on) {
		printf( "load_from_dragdrop: called\n" );
	}


	/* clear the left footer for new response status */

	xv_set(Tt_frame, FRAME_LEFT_FOOTER, "", 0);

	clear_context(dnd_context, TRUE);

	xv_set(Sel, 
		SEL_TYPES, enumeration_count_atom, 0, 
		SEL_REPLY_PROC, count_reply_proc,
		0);

	sel_post_req(Sel);

	if (debug_on) {
		printf( "load_from_dragdrop: after sel_post_req\n" );
	}

}

int
load_event_proc(item, value, event)
Panel_item		item;
unsigned int		value;
Event                   *event;

{
	switch (event_action(event)) {

	case ACTION_DRAG_COPY:
	case ACTION_DRAG_MOVE:

                /* If the user dropped over an acceptable
                 * drop site, the owner of the drop site will
                 * be sent an ACTION_DROP_{COPY, MOVE} event.
                 */

		event->action = ACTION_DRAG_COPY;

                /* To acknowledge the drop and to associate the                                 * rank of the source's selection to our
                 * requestor selection object, we call
                 * dnd_decode_drop().
                 */

        	if (value != XV_ERROR) 
		{
 			/*We can use the macro dnd_site_id() to access
                         * the site id of the drop site that was
                         * dropped on.
                         */

               	  	load_from_dragdrop();

        	} 
		else
		{
			if (debug_on)
            			printf ( "drop error\n" );
			return (XV_OK);
		}
	}
	return(XV_ERROR);
}


init_dragdrop()

{
	My_server = XV_SERVER_FROM_WINDOW(Tt_frame);
	gethostname(Hostname, MAXHOSTNAMELEN);

	incr_atom = xv_get(My_server, SERVER_ATOM,  "INCR" );
	targets_atom = xv_get(My_server, SERVER_ATOM,  "TARGETS" );
	host_atom = xv_get(My_server, SERVER_ATOM,  "_SUN_FILE_HOST_NAME" );
	file_name_atom = xv_get(My_server, SERVER_ATOM,  "FILE_NAME" );
	atm_file_name_atom = xv_get(My_server, SERVER_ATOM,  "_SUN_ATM_FILE_NAME" );
	selection_end_atom = xv_get(My_server, SERVER_ATOM,  "_SUN_SELECTION_END" );
	alternate_transport_atom = xv_get(My_server, SERVER_ATOM,  "_SUN_ALTERNATE_TRANSPORT_METHODS" );
	available_types_atom = xv_get(My_server, SERVER_ATOM,  "_SUN_AVAILABLE_TYPES" );

	enumeration_count_atom = xv_get(My_server, SERVER_ATOM,  "_SUN_ENUMERATION_COUNT" );
	enumeration_item_atom = xv_get(My_server, SERVER_ATOM,  "_SUN_ENUMERATION_ITEM" );
	null_atom = xv_get(My_server, SERVER_ATOM,  "NULL" );

	dnd_context = (DND_CONTEXT *) malloc(sizeof(DND_CONTEXT));
	memset((char *) dnd_context, NULL, sizeof(DND_CONTEXT));

	Sel = (Selection_requestor) 
		    xv_get (drop_site_item, PANEL_DROP_SEL_REQ);

}

