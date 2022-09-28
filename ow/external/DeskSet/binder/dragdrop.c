/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h> /* MAXPATHLEN (include types.h if removed) */

#ifdef SVR4
#include <dirent.h>   /* MAXNAMLEN */
#include <netdb.h>   /* MAXNAMLEN */
#else /* SVR4 */
#include <sys/dir.h>   /* MAXNAMLEN */
#endif /* SVR4 */

#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <xview/defaults.h>
#include <xview/font.h>
#include <xview/notice.h>
#include <xview/frame.h>
#include <xview/xview.h>
#include <xview/scrollbar.h>
#include <xview/text.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/dragdrop.h>
#include <xview/xv_xrect.h>
#include <xview/cursor.h>
#include <xview/cms.h>

#include "xv.h"
#include "props.h"

#define Min(a,b) (a > b ? b : a)
#define Max(a,b) (a > b ? a : b)

/* Temp buffer size for managing INCR transfers thru 
   the selection service */

#define TARGET_RESPONSE         0x1
#define HOST_RESPONSE           0x2
#define TRANSPORT_RESPONSE      0x4
#define FILE_NAME_RESPONSE      0x8
#define DATA_LABEL_RESPONSE     0x10
#define COUNT_RESPONSE          0x20
#define BYTES_REQUESTED         0x40


#define RESPONSE_LEVEL_1        0x1f
#define RESPONSE_LEVEL_2        0x3f

#define TAKES_COLOR	1
#define TAKES_ICON	2

extern Binder         *binder;
extern Display	      *dpy;

typedef struct {
		char	*data;
		int	alloc_size;
		int	bytes_used;
		} CHAR_BUF;

typedef struct {
		int		site_type;
		Panel_item	site_item;
		} Drop_data;

/* context structure for describing the current 
   drag/drop action */

typedef struct {
                Atom            *target_list;
                int             num_targets;
                Atom            *transport_list;
                int             num_transports;
                char            *data_label;
                int             num_objects;
                Atom            link_active;
                char            *source_host;
                char            *source_filename;
                Atom            chosen_target;
                Atom            chosen_transport;
                int             transfer_state;
                int             processing_stream;
                int             state_mask;
                int             stop_hit;
                CHAR_BUF        *transfer_data;
                } DND_CONTEXT;


static char		Hostname[MAXHOSTNAMELEN];
static DND_CONTEXT	*D;
static int		debug_on = 0;
static Xv_server	server;
static Xv_opaque	binder_key_data;

typedef struct {
		Atom text;
		Atom incr;
		Atom targets;
		Atom length;
		Atom host;
		Atom file_name;
		Atom atm_file_name;
		Atom delete;
		Atom selection_end;
		Atom data_label;
		Atom alternate_transport;
		Atom available_types;
		Atom enumeration_count;
		Atom color;
		Atom null;
	} Atom_list;

static Atom_list	*A;

Xv_drop_site    	Drop_site_4_icon;
Xv_drop_site    	Drop_site_4_icon_mask;
Xv_drop_site    	Drop_site_4_foreground_color;
Xv_drop_site    	Drop_site_4_background_color;
Selection_requestor	Sel;

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



dump_atom_list(atom_list, length)

Atom	*atom_list;
int	length;

{
	int	i;
	char	*atom_name;

	if (!debug_on)
		return;

	printf("dump_atom list [");
	for (i = 0; i < length; i++)
	{
		if (atom_list[i] > 0)
			atom_name = XGetAtomName( (Display *)xv_get(server, XV_DISPLAY), atom_list[i]);
		else
			atom_name = "[None]";

		printf(" %s ", atom_name);

	}
	printf("] \n");

}

/* Clears the context block in preparation for 
   a new drag/drop transaction */

clear_context(dnd_ptr)

DND_CONTEXT	*dnd_ptr;

{
	if (dnd_ptr->target_list)
		free(dnd_ptr->target_list);
	dnd_ptr->target_list = NULL;

	dnd_ptr->num_targets = 0;
	if (dnd_ptr->transport_list)
		free(dnd_ptr->transport_list);
	dnd_ptr->transport_list = NULL;

	dnd_ptr->num_transports = 0;
	if (dnd_ptr->data_label)
		free(dnd_ptr->data_label);
	dnd_ptr->data_label = NULL;

	dnd_ptr->num_objects = 0;
	dnd_ptr->link_active = 0;
	if (dnd_ptr->source_host)
		free(dnd_ptr->source_host);
	dnd_ptr->source_host = NULL;

	if (dnd_ptr->source_filename)
		free(dnd_ptr->source_filename);
	dnd_ptr->source_filename = NULL;

	dnd_ptr->chosen_target = 0;
	dnd_ptr->chosen_transport = 0;

	dnd_ptr->state_mask = 0;
	dnd_ptr->stop_hit = 0;
	dnd_ptr->transfer_data = 0;
	dnd_ptr->processing_stream = 0;
}

CHAR_BUF *
buf_alloc()

{
	CHAR_BUF	*ptr;

	if (!(ptr = (CHAR_BUF *) calloc(1, sizeof(CHAR_BUF))))
		return(NULL);

	if (!(ptr->data = (char *) malloc(10000)))
	{
		free(ptr);
		return(NULL);
	}

	ptr->alloc_size = 10000;
	ptr->bytes_used = 0;

	return(ptr);
}

buf_append(ptr, data, bytes)

CHAR_BUF	*ptr;
char		*data;
int		bytes;

{
	int	increment;
	char	*new_block;

	if ((ptr->alloc_size - ptr->bytes_used) > bytes)
	{
		/* we have enough allocated memory space to 
		   just copy the bytes in */

		strncpy(&(ptr->data[ptr->bytes_used]), data, bytes);
		ptr->bytes_used += bytes;
		return(0);
	}
	else
	{
		/* we have to allocate enough memory */

		increment = Max(10000, bytes);
		new_block = (char *) realloc(ptr->data, ptr->alloc_size + increment);
		if (!new_block)
			return(1);
		else
		{
			ptr->data = new_block;
			strncpy(&(ptr->data[ptr->bytes_used]), data, bytes);
			ptr->bytes_used += bytes;
			ptr->alloc_size += increment;

			return(0);
		}
	}
}

buf_free(ptr)

CHAR_BUF	*ptr;

{
	if (!ptr)
		return;

	if (ptr->data)
		free(ptr->data);

	free(ptr);
}

/* Huge monolithic routine that processes all the replies 
   to the questions that I ask about the current drag/drop 
   selection.  These requests are made in groups, and the 
   routine contains a state machine whose current state is stored 
   in the D block.  This routine updates the 
   state machine as replies, or failed replies come in.  
   Changes in state may require issuing new requests for data, 
   which are processed by this same routine.  */
/* ARGSUSED */
static void
binder_reply_proc ( sel_req, target, type, replyBuf, len, format )

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
	Atom	*atom_buf = (Atom *) replyBuf;
	int	old_length;
	Event	event;
        Props   *p = ( Props * )binder->properties;

	/* try to turn the type and target atoms into 
	   some useful text for debugging. */

	if (debug_on)
	{
		if (type > 0)
			atom_name = XGetAtomName( (Display *)xv_get(server, XV_DISPLAY), type);
		else
			atom_name = "[None]";
		if (target > 0)
			target_name = XGetAtomName( (Display *)xv_get(server, XV_DISPLAY), target);
		else
			target_name = "[None]";

		printf("entered reply proc, type name  = %s, type atom = %d\n target_name = %s, target atom = %d\n len = %d, format = %d, buf = %d\nstate = %d\n",
		atom_name, type, target_name, target, len, format, err_ptr, D->state_mask);
	}

	/* Simply processing the return from the termination 
	   request.  No action necessary */

	if (target == A->selection_end)
		return;

	if ((len == SEL_ERROR) && ((*err_ptr) == SEL_BAD_CONVERSION))
	{
		/* a conversion of some type failed.  Mark
		   the state variable */

		if (target == A->targets)
			D->state_mask |= TARGET_RESPONSE;
		else if (target == A->host)
			D->state_mask |= HOST_RESPONSE;
		else if (target == A->alternate_transport)
			D->state_mask |= TRANSPORT_RESPONSE;
		else if (target == A->file_name)
			D->state_mask |= FILE_NAME_RESPONSE;
		else if (target == A->data_label)
			D->state_mask |= DATA_LABEL_RESPONSE;
		else if (target == A->enumeration_count)
			D->state_mask |= COUNT_RESPONSE;
	}
	else if (len == SEL_ERROR)
	{
		/* Some internal error happened as a result of 
		   an earlier posted request.  Tell the user. */

		switch (*err_ptr) {
		case SEL_BAD_PROPERTY :
		        xv_set(p->frame, FRAME_LEFT_FOOTER, "ReplyProc: Bad property!", 0);
          		break;
      		case SEL_BAD_TIME:
          		xv_set(p->frame, FRAME_LEFT_FOOTER, "ReplyProc: Bad time!", 0);
          		break;
      		case SEL_BAD_WIN_ID:
          		xv_set(p->frame, FRAME_LEFT_FOOTER, "ReplyProc: Bad window id!", 0);
          		break;
      		case SEL_TIMEDOUT:
          		xv_set(p->frame, FRAME_LEFT_FOOTER, "ReplyProc: Timed out!", 0);
          		break;
      		case SEL_PROPERTY_DELETED:
          		xv_set(p->frame, FRAME_LEFT_FOOTER, "ReplyProc: Property deleted!", 0);
          		break;
      		case SEL_BAD_PROPERTY_EVENT:
          		xv_set(p->frame, FRAME_LEFT_FOOTER, "ReplyProc: Bad property event!", 0);
          		break;
		}
		stop_dragdrop();
		return;
	}
	else if (type == A->incr)
		D->processing_stream = TRUE;
    	else if ((target == XA_STRING) || (target == A->text))
	{
		/* data stream coming thru */

		if (len && !D->stop_hit)
		{
			/* The length is non-zero, so data, and 
			   not the end of transmission */

			if (!D->transfer_data)
				D->transfer_data = buf_alloc();

			if (buf_append(D->transfer_data, char_buf, len))
			{
				/* Memory allocation failed */

				buf_free(D->transfer_data);

				D->transfer_data = NULL;

				notice_prompt(p->frame, &event,
					NOTICE_MESSAGE_STRINGS,
					MGET( "There was not enough space to make\nthe data transfer.  The drop operation\nhas been cancelled." ),
					0,
					NOTICE_BUTTON_NO, MGET( "Continue" ),
					0);

				stop_dragdrop();
				return;
			}


			if (!D->processing_stream)
			{

				buf_free(D->transfer_data);
				D->transfer_data = NULL;

	                	/* To complete the drag and drop operation,
                	 	 * we tell the source that we are all done.
                 		 */
 
				stop_dragdrop();
				return;
			}
		}
	    	else if (D->processing_stream)
		{
			/* The length was 0, so we have the 
			   end of a data transmission */

			buf_free(D->transfer_data);
				D->transfer_data = NULL;

	                /* To complete the drag and drop operation,
                 	 * we tell the source that we are all done.
                 	 */
 
			stop_dragdrop();
			return;
		}
		else
		{
 
			stop_dragdrop();
			return;
		}
	}
	else if (target == A->targets)
	{
		if (len)
		{
			if (D->target_list && !D->processing_stream)
			{
				free(D->target_list);
				D->target_list = NULL;
			}

			if (!D->target_list)
			{
				D->target_list = (Atom *) malloc(len * 4);
				memcpy( (char *)D->target_list, ( char * )replyBuf, len * 4);
				if (!D->processing_stream)
					D->state_mask |= TARGET_RESPONSE;
			}
			else
			{
				D->target_list = (Atom *) realloc(D->target_list, D->num_targets * 4 + len * 4);
				memcpy(( char * )&D->target_list[D->num_targets - 1], ( char * )replyBuf, len * 4);
			}
		}
		else
		{
			D->state_mask |= TARGET_RESPONSE;
			D->processing_stream = FALSE;
		}

		D->num_targets += len;
	}
	else if (target == A->alternate_transport)
	{
		if (len)
		{
			if (D->transport_list && !D->processing_stream)
			{
				free(D->transport_list);
				D->transport_list = NULL;
			}

			if (!D->transport_list)
			{
				D->transport_list = (Atom *) malloc(len * 4);
				memcpy(( char * )D->transport_list, ( char * )replyBuf, len * 4);
				if (!D->processing_stream)
					D->state_mask |= TRANSPORT_RESPONSE;
			}
			else
			{
				D->transport_list = (Atom *) realloc(D->transport_list, D->num_transports * 4 + len * 4);
				memcpy(( char * )&D->transport_list[D->num_transports - 1],  ( char *)replyBuf, len * 4);
			}
		}
		else
		{
			D->state_mask |= TRANSPORT_RESPONSE;
			D->processing_stream = FALSE;
		}

		D->num_transports += len;
	}
	else if (target == A->host)
	{
		if (len)
		{
			if (D->source_host && !D->processing_stream)
			{
				free(D->source_host);
				D->source_host = NULL;
			}

			if (!D->source_host)
			{
				D->source_host = (char *) malloc(len + 1);
				memcpy(D->source_host, ( char * )replyBuf, len);
				D->source_host[len] = NULL;
				if (!D->processing_stream)
					D->state_mask |= HOST_RESPONSE;
			}
			else
			{
				old_length = strlen(D->source_host);
				D->source_host = (char *) realloc(D->source_host, old_length + len + 1);
				memcpy(&D->source_host[old_length], ( char * )replyBuf, len);
				D->source_host[old_length + len] = NULL;
			}
		}
		else
		{
			D->state_mask |= HOST_RESPONSE;
			D->processing_stream = FALSE;
		}
	}
	else if (target == A->file_name)
	{
		if (len)
		{
			if (D->source_filename && !D->processing_stream)
			{
				free(D->source_filename);
				D->source_filename = NULL;
			}

			if (!D->source_filename)
			{
				D->source_filename = (char *) malloc(len + 1);
				memcpy(D->source_filename, ( char *)replyBuf, len);
				D->source_filename[len] = NULL;
				if (!D->processing_stream)
					D->state_mask |= FILE_NAME_RESPONSE;
			}
			else
			{
				old_length = strlen(D->source_filename);
				D->source_filename = (char *) realloc(D->source_filename, old_length + len + 1);
				memcpy(&D->source_filename[old_length], ( char * )replyBuf, len);
				D->source_filename[old_length + len] = NULL;
			}
		}
		else
		{
			D->state_mask |= FILE_NAME_RESPONSE;
			D->processing_stream = FALSE;
		}
	}
	else if (target == A->data_label)
	{
		if (len)
		{
			if (D->data_label && !D->processing_stream)
			{
				free(D->data_label);
				D->data_label = NULL;
			}

			if (!D->data_label)
			{
				D->data_label = (char *) malloc(len + 1);
				memcpy(D->data_label, ( char * )replyBuf, len);
				D->data_label[len] = NULL;
				if (!D->processing_stream)
					D->state_mask |= DATA_LABEL_RESPONSE;
			}
			else
			{
				old_length = strlen(D->data_label);
				D->data_label = (char *) realloc(D->data_label, old_length + len + 1);
				memcpy(&D->data_label[old_length], ( char *)replyBuf, len);
				D->data_label[old_length + len] = NULL;
			}
		}
		else
		{
			D->state_mask |= DATA_LABEL_RESPONSE; 
			D->processing_stream = FALSE;
		}
	}
	else if (target == A->enumeration_count)
	{
		D->num_objects = atom_buf[0];
		D->state_mask |= COUNT_RESPONSE;
	}
	else
		return;

	if (D->state_mask == RESPONSE_LEVEL_1)
	{
		Drop_data	*d_ptr = (Drop_data *) xv_get(Sel, XV_KEY_DATA, binder_key_data);
		if (debug_on)
			printf("first batch of replies processed, asking for second\n");

		if (D->num_objects > 1)
		{
			notice_prompt(p->frame, &event,
				NOTICE_MESSAGE_STRINGS,
				MGET( "Binder cannot handle multiple\nitems at once.  Please select one item,\nand try again." ),
				0,
				NOTICE_BUTTON_NO, MGET( "Continue" ),
				0);
			
			stop_dragdrop();
			return;
		}
	
		/* look to see if we've had a color dropped on us */

		if (debug_on)
			dump_atom_list(D->target_list, D->num_targets);

		if (atom_in_list(A->color, D->target_list, D->num_targets))
		{
			char		*color_ptr;
			int		length, format;
			Selection_requestor	color_req;

			if (d_ptr->site_type != TAKES_COLOR)
			{
				notice_prompt(p->frame, &event,
					NOTICE_MESSAGE_STRINGS,
					MGET( "This drop site requires an icon." ),
					0,
					NOTICE_BUTTON_NO, MGET( "Continue" ),
					0);
				
				stop_dragdrop();
				return;
			}


			/* we have a color, let's ask for it, and stop */

			color_req = xv_create(p->frame, SELECTION_REQUESTOR, SEL_RANK, xv_get(Sel, SEL_RANK), 0);
       			xv_set(color_req, SEL_TYPE, A->color, 0);

       			color_ptr = (char *) xv_get(color_req, SEL_DATA, &length, &format);

			if (color_ptr)
			{
				XColor	xcolor;
				char	color_string[64];

				XParseColor(dpy, xv_get( xv_get( p->frame, WIN_CMS), CMS_CMAP_ID ), color_ptr, &xcolor);
				sprintf(color_string, "%d %d %d", xcolor.red >> 8, xcolor.green >> 8, xcolor.blue >>8);

				xv_set(d_ptr->site_item, PANEL_VALUE, color_string, 0);
			}

			xv_destroy_safe(color_req);

			stop_dragdrop();
			return;

		}
		else if (d_ptr->site_type == TAKES_COLOR)
		{
			notice_prompt(p->frame, &event,
				NOTICE_MESSAGE_STRINGS,
				MGET( "This drop site requires a color." ),
				0,
				NOTICE_BUTTON_NO, MGET( "Continue" ),
				0);
			
			stop_dragdrop();
			return;
		}


	    	if (!atom_in_list(A->atm_file_name, D->transport_list, D->num_targets))
		{
			notice_prompt(p->frame, &event,
				NOTICE_MESSAGE_STRINGS,
				MGET( "The Binder can currently only handle icons by file name." ),
				0,
				NOTICE_BUTTON_NO, MGET( "Continue" ),
				0);
			
			stop_dragdrop();
			return;
		}
		

		if ((!atom_in_list(XA_STRING, D->target_list, D->num_targets)) && 
		    (!atom_in_list(A->atm_file_name, D->transport_list, D->num_targets)) && 
		    (!atom_in_list(A->text, D->target_list, D->num_targets)))
		{
			notice_prompt(p->frame, &event,
				NOTICE_MESSAGE_STRINGS,
				MGET( "The sourcing application cannot\nsend data that Binder can operate on.\nThe drop operation has been cancelled." ),
				0,
				NOTICE_BUTTON_NO, MGET( "Continue" ),
				0);
			
			stop_dragdrop();
			return;
		}

	
		if (!atom_in_list(XA_STRING, D->target_list, D->num_targets))
			D->chosen_target = A->text;
		else
			D->chosen_target = XA_STRING;
	
		/* determine what sort of data we have coming */
		/* get the host name to go with the file name */
	
		if (atom_in_list(A->atm_file_name, D->transport_list, D->num_transports) && 
		   D->source_filename)
		{
			if (!strcmp(Hostname, D->source_host))
			{
				/* life is hunky dory.  Use the 
				   file name in place. */

				xv_set(d_ptr->site_item, PANEL_VALUE, D->source_filename, 0);
 
                		/* To complete the drag and drop operation,
                 		* we tell the source that we are all done.
                 		*/
		
				stop_dragdrop();

				return;
			}
		}
		/* we have to deal with a data stream */
	

		xv_set(Sel,
			SEL_REPLY_PROC, binder_reply_proc,
			SEL_TYPES, D->chosen_target, 0,
			0 );
	
		sel_post_req(Sel);

		D->state_mask |= BYTES_REQUESTED;
	}

}


/* Stop the drag and drop operation.  Converts _SUN_SELECTION_END 
   on the current selection, signaling the end, and then 
   puts the rest of the application back to normal. */

stop_dragdrop()


{
	if (debug_on)
			printf("stop_dragdrop called\n");

	/* Signal termination of transaction */

	xv_set(Sel,
		SEL_REPLY_PROC, binder_reply_proc,
		SEL_TYPES, A->selection_end, 0,
		0 );

	sel_post_req(Sel);

	/* reactivate the drop site */

	drop_site_on(TRUE);

	/* Free up any outstanding transfer data */

	if (D->transfer_data)
	{
		buf_free(D->transfer_data);
		D->transfer_data = NULL;
	}

	clear_context(D);
}

/* ARGSUSED */
load_from_dragdrop(server, event, type)

Xv_Server	server;
Event		*event;
int		type;

{
        Props *p = ( Props * )binder->properties;

	if (debug_on) {
		printf("load_from_dragdrop: called\n");
	}

	/* deactivate the drop site */

	drop_site_on(FALSE);

	/* clear the left footer for new response status */

	xv_set(p->frame, FRAME_LEFT_FOOTER, "", 0);

	/* display an alert here, asking if the user wants 
   	   to have any edits discarded.  If they don't, 
   	   then abort the load. */

	/* get target types, and see if we lika any of them */

	clear_context(D);

	xv_set(Sel,
		SEL_REPLY_PROC, binder_reply_proc,
		SEL_TYPES, 
			A->targets,
			A->host,
			A->alternate_transport, 
			A->file_name,
			A->data_label,    
			A->enumeration_count,    
			0,
		XV_KEY_DATA, binder_key_data, type,
		0 );

	sel_post_req(Sel); 

	if (debug_on) {
		printf("load_from_dragdrop: after sel_post_req\n");
	}

}

drop_site_on(on)

int	on;

{
        Props *p = ( Props * )binder->properties;

	if (on)
	{
		xv_set(Drop_site_4_icon,
			DROP_SITE_DELETE_REGION, NULL,
	 		DROP_SITE_REGION, xv_get( p->image_file_item, XV_RECT),
			0);
	
		xv_set(Drop_site_4_icon_mask,
			DROP_SITE_DELETE_REGION, NULL,
	 		DROP_SITE_REGION, xv_get(p->mask_file_item, XV_RECT),
			0);
	
		xv_set(Drop_site_4_foreground_color,
			DROP_SITE_DELETE_REGION, NULL,
	 		DROP_SITE_REGION, xv_get(p->fg_color_item, XV_RECT),
	 		DROP_SITE_REGION, xv_get(p->fg_color_chip, XV_RECT),
			0);
	
		xv_set(Drop_site_4_background_color,
			DROP_SITE_DELETE_REGION, NULL,
	 		DROP_SITE_REGION, xv_get(p->bg_color_item, XV_RECT),
	 		DROP_SITE_REGION, xv_get(p->bg_color_chip, XV_RECT),
			0);
	}
	else
	{
		xv_set(Drop_site_4_icon,
			DROP_SITE_DELETE_REGION, NULL,
			0);
	
		xv_set(Drop_site_4_icon_mask,
			DROP_SITE_DELETE_REGION, NULL,
			0);
	
		xv_set(Drop_site_4_foreground_color,
			DROP_SITE_DELETE_REGION, NULL,
			0);
	
		xv_set(Drop_site_4_background_color,
			DROP_SITE_DELETE_REGION, NULL,
			0);
	}
}

static
panel_event_proc(window, event, arg, type)

Xv_opaque               window;
Event                   *event;
Notify_arg              arg;
Notify_event_type       type;

{
	switch (event_action(event)) {

	case ACTION_DRAG_COPY:
	case ACTION_DRAG_MOVE:

                /* If the user dropped over an acceptable
                 * drop site, the owner of the drop site will
                 * be sent an ACTION_DROP_{COPY, MOVE} event.
                 */

                /* To acknowledge the drop and to associate the                                 * rank of the source's selection to our
                 * requestor selection object, we call
                 * dnd_decode_drop().
                 */

        	if (dnd_decode_drop(Sel, event) != XV_ERROR) 
		{
 			/*We can use the macro dnd_site_id() to access
                         * the site id of the drop site that was
                         * dropped on.
                         */

                	load_from_dragdrop(server, event, dnd_site_id(event));

                        /* If this is a move operation, we must ask
                         * the source to delete the selection object.
                         * We should only do this if the transfer of
                         * data was successful.
                         */
        	} 
		else
		{
			if (debug_on)
            			printf ("drop error\n");
		}
	}
	return(notify_next_event_func(window, ( Notify_event )event, arg, type));
}

init_dragdrop()

{
	Drop_data	*data_ptr;
        Props           *p = ( Props * )binder->properties;

	server = XV_SERVER_FROM_WINDOW(p->frame);
	gethostname(Hostname, MAXHOSTNAMELEN);

	A = (Atom_list *) calloc(sizeof(Atom_list), 1);

	A->text = xv_get(server, SERVER_ATOM, "TEXT");
	A->incr = xv_get(server, SERVER_ATOM, "INCR");
	A->targets = xv_get(server, SERVER_ATOM, "TARGETS");
	A->length = xv_get(server, SERVER_ATOM, "LENGTH");
	A->host = xv_get(server, SERVER_ATOM, "_SUN_FILE_HOST_NAME");
	A->file_name = xv_get(server, SERVER_ATOM, "FILE_NAME");
	A->atm_file_name = xv_get(server, SERVER_ATOM, "_SUN_ATM_FILE_NAME");
	A->delete = xv_get(server, SERVER_ATOM, "DELETE");
	A->selection_end = xv_get(server, SERVER_ATOM, "_SUN_SELECTION_END");
	A->data_label = xv_get(server, SERVER_ATOM, "_SUN_DATA_LABEL");
	A->alternate_transport = xv_get(server, SERVER_ATOM, "_SUN_ALTERNATE_TRANSPORT_METHODS");
	A->available_types = xv_get(server, SERVER_ATOM, "_SUN_AVAILABLE_TYPES");

	A->enumeration_count = xv_get(server, SERVER_ATOM, "_SUN_ENUMERATION_COUNT");
	A->color = xv_get(server, SERVER_ATOM, "_SUN_TYPE_color");
	A->null = xv_get(server, SERVER_ATOM, "NULL");

	D = (DND_CONTEXT *) malloc(sizeof(DND_CONTEXT));
	memset(( char * )D, NULL, sizeof(DND_CONTEXT));

	data_ptr = (Drop_data *) calloc(sizeof(Drop_data), 1);

	data_ptr->site_type = TAKES_ICON;
	data_ptr->site_item = p->image_file_item;

    	Drop_site_4_icon = xv_create(p->panel, DROP_SITE_ITEM,
                DROP_SITE_ID,           data_ptr,
                DROP_SITE_EVENT_MASK,
                                DND_MOTION | DND_ENTERLEAVE,
                0);
	
	data_ptr = (Drop_data *) calloc(sizeof(Drop_data), 1);

	data_ptr->site_type = TAKES_ICON;
	data_ptr->site_item = p->mask_file_item;

    	Drop_site_4_icon_mask = xv_create(p->panel, DROP_SITE_ITEM,
                DROP_SITE_ID,           data_ptr,
                DROP_SITE_EVENT_MASK,
                                DND_MOTION | DND_ENTERLEAVE,
                0);
	
	data_ptr = (Drop_data *) calloc(sizeof(Drop_data), 1);

	data_ptr->site_type = TAKES_COLOR;
	data_ptr->site_item = p->fg_color_item;
	
    	Drop_site_4_foreground_color = xv_create(p->panel, DROP_SITE_ITEM,
                DROP_SITE_ID,           data_ptr,
                DROP_SITE_EVENT_MASK,
                                DND_MOTION | DND_ENTERLEAVE,
                0);
	
	data_ptr = (Drop_data *) calloc(sizeof(Drop_data), 1);

	data_ptr->site_type = TAKES_COLOR;
	data_ptr->site_item = p->bg_color_item;
	
    	Drop_site_4_background_color = xv_create(p->panel, DROP_SITE_ITEM,
                DROP_SITE_ID,           data_ptr,
                DROP_SITE_EVENT_MASK,
                                DND_MOTION | DND_ENTERLEAVE,
                0);

	binder_key_data = xv_unique_key();

	Sel = xv_create(p->panel, SELECTION_REQUESTOR, 0);


	drop_site_on(TRUE);

 	notify_interpose_event_func(( Notify_client )canvas_pixwin(p->panel),
				    ( Notify_func )panel_event_proc, NOTIFY_SAFE);
}
