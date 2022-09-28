/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h> /* MAXPATHLEN (include types.h if removed) */
#ifdef SVR4
#include <dirent.h>
#include <netdb.h>
#else
#include <sys/dir.h>   /* MAXNAMLEN */
#endif /* SVR4 - when will it end... */
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#ifdef SVR4
#include <string.h>
#else
#include <strings.h>
#endif /* SVR4 - three before weren't enough? */
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
#include <xview/file_chsr.h>
#include "base_gui.h"
#include "base.h"
#include "file_gui.h"
#include "xpm.h"

#define Min(a,b) (a > b ? b : a)
#define Max(a,b) (a > b ? a : b)

/* Temp buffer size for managing INCR transfers thru 
   the selection service */

#define XFER_BUFSIZ	5000

int	IconeditSelectionConvert();
extern int	canvas_dimension;
extern int	Edited;
extern File_chooser save_dialog;
extern File_chooser load_dialog;

static int	load_on = FALSE;

#define TARGET_RESPONSE         0x1
#define HOST_RESPONSE           0x2
#define TRANSPORT_RESPONSE      0x4
#define FILE_NAME_RESPONSE      0x8
#define DATA_LABEL_RESPONSE     0x10
#define COUNT_RESPONSE          0x20
#define LOAD_RESPONSE           0x40
#define BYTES_REQUESTED         0x80


#define RESPONSE_LEVEL_1        0x3f
#define RESPONSE_LEVEL_2        0x7f

#define MAX_UNDOS 8

#define COLOR 0
#define MONO 1

typedef struct {
		char	*data;
		int	alloc_size;
		int	bytes_used;
		} CHAR_BUF;

typedef struct corner_type{
  int x;
  int y;
} corner_struct;


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
static DND_CONTEXT	*dnd_context;
static Atom		current_link_atom;
static int		debug_on = 0;
static int		edited = 0;

static Atom text_atom;
static Atom incr_atom;
static Atom targets_atom;
static Atom length_atom;
static Atom host_atom;
static Atom file_name_atom;
static Atom atm_file_name_atom;
static Atom delete_atom;
static Atom selection_end_atom;
static Atom data_label_atom;
static Atom load_atom;
static Atom alternate_transport_atom;
static Atom available_types_atom;
static Atom enumeration_count_atom;
static Atom insert_selection_atom;
static Atom color_atom;
static Atom null_atom;
static Atom transfer_type_atom;
static char xfer_data_buf[XFER_BUFSIZ];
static int  iconedit_key_data;
static int  paste_position_key_data;

static CHAR_BUF	*paste_data;
static int	processing_paste_stream;
static char	*cut_data = NULL;

Xv_drop_site    	Drop_site1;
Xv_drop_site    	Drop_site2;
Selection_requestor	Sel;
Dnd			Dnd_object;

extern Xv_Server		server;
extern XColor			white;
extern Xv_singlecolor		xvwhite;
extern Xv_singlecolor		xvblack;
extern GC			moving_gc;
extern GC			preview_moving_gc;
extern GC			preview_gc;
extern GC			redraw_gc;
extern GC			select_gc;
extern GC			gc_rv;
extern GC			fill_gc;
extern GC			gc;
extern XColor                   black, current_pen;
extern base_window_objects     *base_window;
extern Colormap			cmap;
extern Display			*dpy;
extern Pixmap 			printing_pixmap;
extern Pixmap 			preview_pixmap;
extern Pixmap 			big_pixmap;
extern Window 			preview_xid, big_bit_xid;
extern int current_undo, fill_pattern, undos_allowed, redos_allowed;
extern int 			redo_possible;
extern int 			current_color_state;
extern int 			selection_top;
extern int 			selection_left;
extern int 			selection_bottom;
extern int 			selection_right;
extern int 			screen_depth;
extern int                      grid_status;
extern Xv_cursor		big_bit_cursor;

extern GC three_gc;
extern GC	gc;
extern char	*file;
extern Undo_struct undo_images[MAX_UNDOS];

/* globals for dealing withcut/copy/paste */

extern Selection_owner		Shelf_owner;
extern Selection_requestor	Shelf_requestor;
extern Selection_item		Shelf_item;

extern int boarder_upper, boarder_lower, boarder_left;
extern int boarder_right, height_block, width_block;
extern int icon_height, icon_width;
extern int  preview_boarder_upper, preview_boarder_left;
extern void grid_proc();
extern char *get_truncated_file_name();

static unsigned short source_drag_ptr[]={
#include "dupedoc_drag.icon"
};

static unsigned short source_drop_ptr[]={
#include "dupedoc_drop.icon"
};

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
			atom_name = XGetAtomName((Display *) xv_get(server, XV_DISPLAY), atom_list[i]);
		else
			atom_name = "[None]";

		printf(" %s ", atom_name);

	}
	printf("] \n");

	return(False);
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

set_pen_color(color_ptr)

char	*color_ptr;

{
	Xv_singlecolor	xvcolor;
	XColor		xcolor;
	unsigned long	new_pixel;

	XParseColor(dpy, cmap, color_ptr, &xcolor);

	new_pixel = ds_x_pixel(dpy, cmap, &xcolor);

	XSetForeground(dpy, gc, new_pixel);
	XSetForeground(dpy, fill_gc, new_pixel);
	current_pen.pixel = new_pixel;

	xvcolor.red = (u_char) (xcolor.red >> 8);
	xvcolor.green = (u_char) (xcolor.green >> 8);
	xvcolor.blue = (u_char) (xcolor.blue >> 8);

	xv_set(big_bit_cursor, CURSOR_FOREGROUND_COLOR, &xvcolor, NULL);

	if (((xvcolor.red * 0.41) + (xvcolor.green * 0.33) 
	     + (xvcolor.blue * 0.26)) < 128 )
	  xv_set(big_bit_cursor, CURSOR_BACKGROUND_COLOR, &xvwhite, NULL);
	else
	  xv_set(big_bit_cursor, CURSOR_BACKGROUND_COLOR, &xvblack, NULL);
}

/* Huge monolithic routine that processes all the replies 
   to the questions that I ask about the current drag/drop 
   selection.  These requests are made in groups, and the 
   routine contains a state machine whose current state is stored 
   in the dnd_context block.  This routine updates the 
   state machine as replies, or failed replies come in.  
   Changes in state may require issuing new requests for data, 
   which are processed by this same routine.  */

static void
iconedit_reply_proc ( sel_req, target, type, replyBuf, len, format )

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
    	int	alert_result;
    	int	bytes_inserted;
	char	tmpfile[20];
	FILE	*fd;

	/* try to turn the type and target atoms into 
	   some useful text for debugging. */

	if (debug_on)
	{
		if (type > 0)
			atom_name = XGetAtomName((Display *) xv_get(server, XV_DISPLAY), type);
		else
			atom_name = "[None]";
		if (target > 0)
			target_name = XGetAtomName((Display *) xv_get(server, XV_DISPLAY), target);
		else
			target_name = "[None]";

		printf("entered reply proc, type name  = %s, type atom = %d\n target_name = %s, target atom = %d\n len = %d, format = %d, buf = %d\nstate = %d\n",
		atom_name, type, target_name, target, len, format, err_ptr, dnd_context->state_mask);
	}

	/* Simply processing the return from the termination 
	   request.  No action necessary */

	if (target == selection_end_atom)
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
		else if (target == data_label_atom)
			dnd_context->state_mask |= DATA_LABEL_RESPONSE;
		else if (target == load_atom)
			dnd_context->state_mask |= LOAD_RESPONSE;
		else if (target == enumeration_count_atom)
			dnd_context->state_mask |= COUNT_RESPONSE;
	}
	else if (len == SEL_ERROR)
	{
		/* Some internal error happened as a result of 
		   an earlier posted request.  Tell the user. */

		switch (*err_ptr) {
		case SEL_BAD_PROPERTY :
		        xv_set(base_window->window, FRAME_LEFT_FOOTER, "ReplyProc: Bad property!", 0);
          		break;
      		case SEL_BAD_TIME:
          		xv_set(base_window->window, FRAME_LEFT_FOOTER, "ReplyProc: Bad time!", 0);
          		break;
      		case SEL_BAD_WIN_ID:
          		xv_set(base_window->window, FRAME_LEFT_FOOTER, "ReplyProc: Bad window id!", 0);
          		break;
      		case SEL_TIMEDOUT:
          		xv_set(base_window->window, FRAME_LEFT_FOOTER, "ReplyProc: Timed out!", 0);
          		break;
      		case SEL_PROPERTY_DELETED:
          		xv_set(base_window->window, FRAME_LEFT_FOOTER, "ReplyProc: Property deleted!", 0);
          		break;
      		case SEL_BAD_PROPERTY_EVENT:
          		xv_set(base_window->window, FRAME_LEFT_FOOTER, "ReplyProc: Bad property event!", 0);
          		break;
		}
		stop_dragdrop();
		return;
	}
	else if (type == incr_atom)
		dnd_context->processing_stream = TRUE;
    	else if ((target == XA_STRING) || (target == text_atom))
	{
		/* data stream coming thru */

		if (len && !dnd_context->stop_hit)
		{
			/* The length is non-zero, so data, and 
			   not the end of transmission */

			if (!dnd_context->transfer_data)
				dnd_context->transfer_data = buf_alloc();

			if (buf_append(dnd_context->transfer_data, char_buf, len))
			{
				/* Memory allocation failed */

				buf_free(dnd_context->transfer_data);

				dnd_context->transfer_data = NULL;

				notice_prompt(base_window->window, &event,
					NOTICE_MESSAGE_STRING,
	gettext("There was not enough space to make\nthe data transfer.  The drop operation\nhas been cancelled."),
					NOTICE_BUTTON_NO, gettext("Continue"),
					0);

				stop_dragdrop();
				return;
			}


			if (!dnd_context->processing_stream)
			{
				load_image(dnd_context->transfer_data->data);

				/* set up so there is nothing in the 
				   name field of the popup */

				if (file)
					*file = NULL;

				buf_free(dnd_context->transfer_data);
				dnd_context->transfer_data = NULL;

	                	/* To complete the drag and drop operation,
                	 	 * we tell the source that we are all done.
                 		 */
 
				stop_dragdrop();
				return;
			}
		}
	    	else if (dnd_context->processing_stream)
		{
			/* The length was 0, so we have the 
			   end of a data transmission */

			load_image(dnd_context->transfer_data->data);

			buf_free(dnd_context->transfer_data);
				dnd_context->transfer_data = NULL;

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
				dnd_context->target_list = (Atom *) malloc(len * 4);
				memcpy((char *) dnd_context->target_list, (char *) replyBuf, len * 4);
				if (!dnd_context->processing_stream)
					dnd_context->state_mask |= TARGET_RESPONSE;
			}
			else
			{
				dnd_context->target_list = (Atom *) realloc(dnd_context->target_list, dnd_context->num_targets * 4 + len * 4);
				memcpy((char *) &dnd_context->target_list[dnd_context->num_targets - 1], (char *) replyBuf, len * 4);
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
				dnd_context->transport_list = (Atom *) malloc(len * 4);
				memcpy((char *) dnd_context->transport_list, (char *) replyBuf, len * 4);
				if (!dnd_context->processing_stream)
					dnd_context->state_mask |= TRANSPORT_RESPONSE;
			}
			else
			{
				dnd_context->transport_list = (Atom *) realloc(dnd_context->transport_list, dnd_context->num_transports * 4 + len * 4);
				memcpy((char *) &dnd_context->transport_list[dnd_context->num_transports - 1], (char *) replyBuf, len * 4);
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
				dnd_context->source_host = (char *) malloc(len + 1);
				memcpy(dnd_context->source_host, (char *) replyBuf, len);
				dnd_context->source_host[len] = NULL;
				if (!dnd_context->processing_stream)
					dnd_context->state_mask |= HOST_RESPONSE;
			}
			else
			{
				old_length = strlen(dnd_context->source_host);
				dnd_context->source_host = (char *) realloc(dnd_context->source_host, old_length + len + 1);
				memcpy(&dnd_context->source_host[old_length], (char *) replyBuf, len);
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
				dnd_context->source_filename = (char *) malloc(len + 1);
				memcpy(dnd_context->source_filename, (char *) replyBuf, len);
				dnd_context->source_filename[len] = NULL;
				if (!dnd_context->processing_stream)
					dnd_context->state_mask |= FILE_NAME_RESPONSE;
			}
			else
			{
				old_length = strlen(dnd_context->source_filename);
				dnd_context->source_filename = (char *) realloc(dnd_context->source_filename, old_length + len + 1);
				memcpy(&dnd_context->source_filename[old_length], (char *) replyBuf, len);
				dnd_context->source_filename[old_length + len] = NULL;
			}
		}
		else
		{
			dnd_context->state_mask |= FILE_NAME_RESPONSE;
			dnd_context->processing_stream = FALSE;
		}
	}
	else if (target == data_label_atom)
	{
		if (len)
		{
			if (dnd_context->data_label && !dnd_context->processing_stream)
			{
				free(dnd_context->data_label);
				dnd_context->data_label = NULL;
			}

			if (!dnd_context->data_label)
			{
				dnd_context->data_label = (char *) malloc(len + 1);
				memcpy(dnd_context->data_label, (char *) replyBuf, len);
				dnd_context->data_label[len] = NULL;
				if (!dnd_context->processing_stream)
					dnd_context->state_mask |= DATA_LABEL_RESPONSE;
			}
			else
			{
				old_length = strlen(dnd_context->data_label);
				dnd_context->data_label = (char *) realloc(dnd_context->data_label, old_length + len + 1);
				memcpy(&dnd_context->data_label[old_length], (char *) replyBuf, len);
				dnd_context->data_label[old_length + len] = NULL;
			}
		}
		else
		{
			dnd_context->state_mask |= DATA_LABEL_RESPONSE; 
			dnd_context->processing_stream = FALSE;
		}
	}
	else if (target == load_atom)
	{
		dnd_context->link_active = TRUE;
		current_link_atom = atom_buf[0];
		dnd_context->state_mask |= LOAD_RESPONSE;
	}
	else if (target == enumeration_count_atom)
	{
		dnd_context->num_objects = atom_buf[0];
		dnd_context->state_mask |= COUNT_RESPONSE;
	}
	else
		return;

	if (dnd_context->state_mask == RESPONSE_LEVEL_1)
	{
		if (debug_on)
			printf("first batch of replies processed, asking for second\n");

		if (dnd_context->num_objects > 1)
		{
			notice_prompt(base_window->window, &event,
				NOTICE_MESSAGE_STRING,
	gettext("Icon Edit cannot handle multiple\nfiles at once.  Please select one file\nand try again."),
				NOTICE_BUTTON_NO, gettext("Continue"),
				0);
			
			stop_dragdrop();
			return;
		}
	
		/* look to see if we've had a color dropped on us */

		if (debug_on)
			dump_atom_list(dnd_context->target_list, dnd_context->num_targets);

		if (atom_in_list(color_atom, dnd_context->target_list, dnd_context->num_targets))
		{
			char		*color_ptr;
			int		length, format;
			Selection_requestor	color_req;
			int             red, green, blue;

			XColor		xcolor;
			unsigned long	new_pixel;
			
			/* we have a color, let's ask for it, and stop */

			color_req = xv_create(base_window->window, SELECTION_REQUESTOR, SEL_RANK, xv_get(Sel, SEL_RANK), 0);
			xv_set(color_req, SEL_TYPE, color_atom, 0);
			
			color_ptr = (char *) xv_get(color_req, SEL_DATA, &length, &format);
	
			XParseColor(dpy, cmap, color_ptr, &xcolor);

			new_pixel = ds_x_pixel(dpy, cmap, &xcolor);

			red = (u_char) (xcolor.red >> 8);
			green = (u_char) (xcolor.green >> 8);
			blue = (u_char) (xcolor.blue >> 8);

			if ((current_color_state == COLOR)  
			    || ((red == 0) && (green == 0) && (blue == 0))
			    || ((red == 255) && (green == 255) && (blue == 255)))

			{
				set_pen_color(color_ptr);

				xv_destroy_safe(color_req);
			}
			else
			{
				notice_prompt(base_window->window, &event,
					NOTICE_MESSAGE_STRINGS,
					gettext("You may not set a pen to be color for monochrome drawing"),
					0,
					NOTICE_BUTTON_NO, gettext("Continue"),
					0);
			}
			stop_dragdrop();
			return;

		}



		if ((!atom_in_list(XA_STRING, dnd_context->target_list, dnd_context->num_targets)) && 
		    (!atom_in_list(atm_file_name_atom, dnd_context->transport_list, dnd_context->num_targets)) && 
		    (!atom_in_list(text_atom, dnd_context->target_list, dnd_context->num_targets)))
		{
			notice_prompt(base_window->window, &event,
				NOTICE_MESSAGE_STRING,
      gettext("The sourcing application cannot\nsend data that Icon Editor can operate on\nThe drop operation has been cancelled."),
				NOTICE_BUTTON_NO, gettext("Continue"),
				0);
			
			stop_dragdrop();
			return;
		}

	
		if (!atom_in_list(XA_STRING, dnd_context->target_list, dnd_context->num_targets))
			dnd_context->chosen_target = text_atom;
		else
			dnd_context->chosen_target = XA_STRING;
	
		if (edited)
			alert_result = (int) notice_prompt(base_window->window, &event,
				NOTICE_MESSAGE_STRING,
			gettext("Your file has been edited\nDo you wish to discard these edits?"),
				NOTICE_BUTTON_NO, gettext("Cancel"),
				NOTICE_BUTTON_YES, gettext("Confirm"),
				0);
	
		if (edited && (alert_result == NOTICE_NO))
		{
			stop_dragdrop();
			return;
		}

		/* discard outstanding load atoms */

		discard_load();
	
		/* determine what sort of data we have coming */
		/* get the host name to go with the file name */
	
		if (atom_in_list(atm_file_name_atom, dnd_context->transport_list, dnd_context->num_transports) && 
		   dnd_context->source_filename)
		{
			if (!strcmp(Hostname, dnd_context->source_host))
			{

				char	*ptr;

				/* if necessary, set up the fiel dialog popup */

				if (!save_dialog)
				      save_dialog = file_dialog_initialize(base_window->window, FILE_CHOOSER_SAVEAS);
				if (!load_dialog)
				      load_dialog = file_dialog_initialize(base_window->window, FILE_CHOOSER_OPEN);


				/* life is hunky dory.  Use the 
				   file name in place. */

				if (!file_load_from_name(dnd_context->source_filename, dnd_context->source_filename))
				{
					/* set up the values in the file dialog popup */
	
					if (ptr = strrchr(dnd_context->source_filename, '/'))
					{
						*ptr = NULL;
	
						set_load_dir(dnd_context->source_filename);
						set_file(ptr + 1);
	
						*ptr = '/';
					}

					set_iconedit_label();
				}
	
 
                		/* To complete the drag and drop operation,
                 		* we tell the source that we are all done.
                 		*/
		
				stop_dragdrop();

				xv_set(base_window->window, FRAME_CLOSED, FALSE, 0);

				return;
			}
		}
		/* we have to deal with a data stream */
	
		/* load in the data stream */
	
		/* determine if we can forge a link to it */
	
		xv_set(Sel,
			SEL_REPLY_PROC, iconedit_reply_proc,
			SEL_TYPES, load_atom, 0,
			0 );
	
		sel_post_req(Sel);

	}
	else if (dnd_context->state_mask == RESPONSE_LEVEL_2) 
	{
		if (debug_on)
			printf("second batch of replies processed\n");

		xv_set(Sel,
			SEL_REPLY_PROC, iconedit_reply_proc,
			SEL_TYPES, dnd_context->chosen_target, 0,
			0 );

		xv_set(base_window->window, FRAME_CLOSED, FALSE, 0);
	
		sel_post_req(Sel);

		dnd_context->state_mask |= BYTES_REQUESTED;
	}

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

static Atom
conjure_transient_selection(server, dpy, template)
Xv_Server server;
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
			if (debug_on)
                        	printf("conjure_transient_selection: returning %d/%s\n",
                                seln_atom, buf);
                        return (seln_atom);
                }

                i++;
        }
}




/*
 * this routine (may be? is?) needed because we cannot do a blocking
 * request and still have another request come in and be processed.
 * We are about to do an INSERT_SELECTION (which is the expected response...)
 * and we need to be able to respond to the insertsion callback.
 * The one job of this routine is to clean things up, and give an error
 * if stuff failed.
 */

static void null_proc() {}

static void
save_thru_load_reply(sel, target, type, replyValue, length, format)
Selection_requestor sel;
Atom target;
Atom type;
Xv_opaque replyValue;
int length;
unsigned format;
{
	Selection_owner owner_sel;

	if (debug_on) {
printf(
"save_thru_load_reply: target %d/%s, type %d/%s, length %d, format %d\n",
target, target ? (char *)xv_get(server, SERVER_ATOM_NAME, target) : "[None]",
type, type ? (char *)xv_get(server, SERVER_ATOM_NAME, type) : "[None]",
length, format);
	}



	if (type == None) {
		xv_set(base_window->window, FRAME_LEFT_FOOTER, "Save back failed",0);
	} else {
		xv_set(base_window->window, FRAME_LEFT_FOOTER, "Save back succeeded",0);
	}

	owner_sel = (Selection_owner)
		xv_get(sel, XV_KEY_DATA, iconedit_key_data);

	/* another bug workaround */
	xv_set(sel, SEL_REPLY_PROC, null_proc, 0);

	xv_destroy_safe(owner_sel);
	xv_destroy_safe(sel);
}

/* Return the current set of data to the original holder 
   of the data.  Only if the data does not reside in a 
   file syatem that I have access to. */

save_thru_load()

{
	Selection_requestor	Load_sel;
	int			length, format, value;
	Selection		sel;
	Atom			loadback_atom[2];


	if (debug_on)
		printf("save_thru_load called\n");

	if (!current_link_atom)
		return(FALSE);

	loadback_atom[0] = 
		conjure_transient_selection(server, xv_get(server, XV_DISPLAY), "DESKSET_SELECTION_%d");
	loadback_atom[1] = XA_STRING;

	if (debug_on)
	{
		Xv_opaque window;

		window = (Xv_opaque) XGetSelectionOwner(
			(Display *) xv_get(server, XV_DISPLAY), current_link_atom);
		printf("owner of selection window is %d\n", window);

		printf("current link atom name = %s\n", XGetAtomName((Display *) xv_get(server, XV_DISPLAY), current_link_atom));
		printf("transient selection name = %s\n", XGetAtomName((Display *) xv_get(server, XV_DISPLAY), loadback_atom[0]));
	}

	/* Ok.  The process for returning data thru the load 
	   link is to:

	   1.  Create a selection thru which the original data 
	       holder can request the data to be put back.

	   2.  Convert the INSERT_SELECTION atom.  Along with 
	       the conversion, we need to pass back a property 
	       atom.  This property contains an atom pair.  The 
	       atom pair has atoms that describe a selection to 
	       take data back thru, and the type that the requestor 
	       is supposed to convert to get that data.

	   3.  When the original holder has gotten the data back,
	       the selection should be deleted.

	*/

	/* allocate the selection to send the data back thru */

    	sel = xv_create(base_window->window, SELECTION_OWNER,
			SEL_RANK,		loadback_atom[0],
			SEL_CONVERT_PROC, 	IconeditSelectionConvert,
			0);


	/* I don't know why this is needed, but it seems to be... */
	xv_set(sel, SEL_OWN, TRUE, 0);

	Load_sel = xv_create(base_window->window, SELECTION_REQUESTOR, 
			SEL_RANK, 	current_link_atom,
			SEL_REPLY_PROC,	save_thru_load_reply,
			XV_KEY_DATA, iconedit_key_data, sel,
			0);

	/* send the conversion request */

	if (debug_on)
		printf("trying to convert INSERT SELECTION against current link atom\n");


    	value = xv_set(Load_sel, SEL_TYPE, insert_selection_atom, 
		SEL_TYPE_INDEX, 0,
			SEL_PROP_TYPE, XA_ATOM,
			SEL_PROP_DATA, loadback_atom,
			SEL_PROP_FORMAT, 32,
			SEL_PROP_LENGTH, 2,
		0);




	if (debug_on) {
		printf("save_thru_load: xv_set returned %d\n", value);
	}
	
	sel_post_req(Load_sel);

	return(TRUE);
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
		SEL_REPLY_PROC, iconedit_reply_proc,
		SEL_TYPES, selection_end_atom, 0,
		0 );

	sel_post_req(Sel);

	/* reactivate the drop site */

	drop_site_on(base_window->preview_canvas);

	/* Free up any outstanding transfer data */

	if (dnd_context->transfer_data)
	{
		buf_free(dnd_context->transfer_data);
		dnd_context->transfer_data = NULL;
	}

	clear_context(dnd_context);
}

load_from_dragdrop(server, event)

Xv_Server	server;
Event		*event;

{
	if (debug_on) {
		printf("load_from_dragdrop: called\n");
	}

	/* deactivate the drop site */

	xv_set(Drop_site1,
		DROP_SITE_DELETE_REGION, NULL,
		0);

	xv_set(Drop_site2,
		DROP_SITE_DELETE_REGION, NULL,
		0);

	/* clear the left footer for new response status */

	xv_set(base_window->window, FRAME_LEFT_FOOTER, "", 0);

	/* display an alert here, asking if the user wants 
   	   to have any edits discarded.  If they don't, 
   	   then abort the load. */

	/* get target types, and see if we lika any of them */

	clear_context(dnd_context);

	xv_set(Sel,
		SEL_REPLY_PROC, iconedit_reply_proc,
		SEL_TYPES, 
			targets_atom,
			host_atom,
			alternate_transport_atom, 
			file_name_atom,
			data_label_atom,    
			enumeration_count_atom,    
			0,
		0 );

	sel_post_req(Sel); 

	if (debug_on) {
		printf("load_from_dragdrop: after sel_post_req\n");
	}

}

drop_site_on(window)

{
	Rect	*win_rect;
	Rect	real_rect;

	win_rect = (Rect *) xv_get(base_window->preview_canvas, WIN_RECT);
	real_rect.r_left = 0;
	real_rect.r_top = 0;
	real_rect.r_width = win_rect->r_width;
	real_rect.r_height = win_rect->r_height;

	xv_set(Drop_site1,
		DROP_SITE_DELETE_REGION, NULL,
	 	DROP_SITE_REGION, &real_rect,
		0);

	win_rect = (Rect *) xv_get(base_window->big_bit_canvas, WIN_RECT);
	real_rect.r_left = 0;
	real_rect.r_top = 0;
	real_rect.r_width = win_rect->r_width;
	real_rect.r_height = win_rect->r_height;

	xv_set(Drop_site2,
		DROP_SITE_DELETE_REGION, NULL,
	 	DROP_SITE_REGION, &real_rect,
		0);
}

static
canvas_event_proc(window, event, arg, type)

Xv_opaque               window;
Event                   *event;
Notify_arg              arg;
Notify_event_type       type;

{
	Rect	*find_item_rect;
	Rect	*source_item_rect;
	int	find_left, find_width, source_left, source_width;
	int	gap = xv_get(window, PANEL_ITEM_X_GAP);
       	Xv_Server       server = XV_SERVER_FROM_WINDOW(event_window(event));
	int	status;
	int	panel_width;
	static int	possible_drag;
	static int	start_x;
	static int	start_y;


	switch (event_action(event)) {

	case ACTION_SELECT:

		/* A selection action within the canvas might be 
		   the beginning of a drag/drop action, but not 
		   if one is still in progress */

		if (event_is_down(event))
		{
			possible_drag = TRUE;
			start_x = event_x(event);
			start_y = event_y(event);
		}
		else
			possible_drag = FALSE;

		break;

	case LOC_DRAG:

		/* if we've drug more than 5 pixels away 
		   from the source item, and drag/drop is legal, 
		   then we can start a drag/drop action. */

		if (possible_drag && 
		   (abs(start_x - event_x(event) > 5) || 
		   (abs(start_y - event_y(event)) > 5)))
		{
			/* deactivate the drop site */

			xv_set(Drop_site1,
				DROP_SITE_DELETE_REGION, NULL,
				0);

			xv_set(Drop_site2,
				DROP_SITE_DELETE_REGION, NULL,
				0);

			if (debug_on)
				printf("initiating drag/drop operation\n");

			switch(status = dnd_send_drop(Dnd_object)) {

			    case XV_OK:
				xv_set(base_window->window, FRAME_LEFT_FOOTER, 
				       gettext("Drag and Drop: Began"), 0);
				break;
			    case DND_TIMEOUT:
				xv_set(base_window->window, FRAME_LEFT_FOOTER, 
				       gettext("Drag and Drop: Timed Out"),0);
				break;
			    case DND_ILLEGAL_TARGET:
				xv_set(base_window->window, FRAME_LEFT_FOOTER,
				       gettext("Drag and Drop: Illegal Target"),0);
				break;
			    case DND_SELECTION:
				xv_set(base_window->window, FRAME_LEFT_FOOTER,
				       gettext("Drag and Drop: Bad Selection"),0);
				break;
			    case DND_ROOT:
				xv_set(base_window->window, FRAME_LEFT_FOOTER,
				       gettext("Drag and Drop: Root Window"),0);
				break;
			    case XV_ERROR:
				xv_set(base_window->window, FRAME_LEFT_FOOTER, 
				       gettext("Drag and Drop: Failed"),0);
				break;
			}


			if (status != XV_OK)
			{
				/* reactivate the drop site */

				drop_site_on(base_window->preview_canvas);

			}

			possible_drag = FALSE;

		}
		break;

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

            		if (dnd_site_id(event) == 1)
                		load_from_dragdrop(server, event);

                        /* If this is a move operation, we must ask
                         * the source to delete the selection object.
                         * We should only do this if the transfer of
                         * data was successful.
                         */
/*
            		if (event_action(event) == ACTION_DRAG_MOVE) 
			{
                		int length, format;
	
                		xv_set(Sel, SEL_TYPE_NAME, "DELETE", 0);
                		(void)xv_get(Sel, SEL_DATA, &length, &format);
            		}
 */
        	} 
		else
		{
			if (debug_on)
            			printf ("drop error\n");
		}
	}
	return(notify_next_event_func(window, (Notify_event) event, arg, type));
}


/* The convert proc is called whenever someone makes a request to the dnd
 * selection.  Two cases we handle within the convert proc: DELETE and
 * _SUN_SELECTION_END.  Everything else we pass on to the default convert
 * proc which knows about our selection items.
 */


int
IconeditSelectionConvert(seln, type, data, length, format)
    Selection_owner	 seln;
    Atom		*type;
    Xv_opaque		*data;
    unsigned long	*length;
    int			*format;
{
    static int		Current_point;
    static int		length_buf;
    Xv_Server 		server = XV_SERVER_FROM_WINDOW(xv_get(seln, XV_OWNER));
    char		*atom_name;
    static Atom		target_list[10];
    static Atom		types_list[2];
    static int		transfer_in_progress = FALSE;
    static CHAR_BUF	*image_buf = NULL;
    char		tmp_filename[20];
    FILE		*fp;
    struct stat		statbuf;
    Pixmap		drag_pixmap;
    char    *filename = (char *) get_truncated_file_name();


	/* try to turn the type and target atoms into 
	   some useful text for debugging. */

	if (debug_on)
	{
		printf("IconeditSelectionConvert conversion called\n");

		if ((int) type > 0)
			atom_name = XGetAtomName((Display *) xv_get(server, XV_DISPLAY), *type);
		else
			atom_name = "[None]";

		printf("IconeditSelectionConvert, being asked to convert %s\n", atom_name);
	}

    /* Interesting sidelight here.  You cannot simply set the type 
       in the reply to the type requested.  It must be the actual 
       type of the data returned.  HOST_NAME, for example would 
       be returnd as type STRING. */

    if (*type == selection_end_atom)
    {
			/* Destination has told us it has completed the drag
			 * and drop transaction.  We should respond with a
			 * zero-length NULL reply.
			 */

	/* Yield ownership of the selection */

	xv_set(Dnd_object, SEL_OWN, False, 0);

	/* reactivate the drop site */

	drop_site_on(base_window->preview_canvas);

	xv_set(base_window->window, FRAME_LEFT_FOOTER, 
	       gettext("Drag and Drop: Completed"),0);
	*format = 32;
	*length = 0;
	*data = NULL;
	*type = null_atom;
	return(True);
    } 
    else if (*type == delete_atom) 
    {
	/* In our case, we chose not to listen to delete 
	   commands on the drag/drop item.  We never mean 
	   to do a move, always a copy. */

	*format = 32;
	*length = 0;
	*data = NULL;
	*type = null_atom;
	return(True);
    } 
    else if (*type == enumeration_count_atom) 
    {
	length_buf = 1;
	*format = 32;
	*length = 1;
	*data = (Xv_opaque) &length_buf;
	*type = XA_INTEGER;
	return(True);
    } 
    else if (*type == host_atom)
    {
	/* Return the hostname that the application 
	   is running on */

	*format = 8;
	*length = strlen(Hostname);
	*data = (Xv_opaque) Hostname;
	*type = XA_STRING;
	return(True);
    } 
    else if (*type == data_label_atom && filename && *filename)
    {
	/* Return the hostname that the application 
	   is running on */

	*format = 8;
	*length = strlen(filename);
	*data = (Xv_opaque) filename;
	*type = XA_STRING;
	return(True);
    } 
    else if (*type == targets_atom)
    {

	/* This request should return all of the targets 
	   that can be converted on this selection.  This 
	   includes the types, as well as the queries that 
	   can be issued. */

	*format = 32;
	*length = 0;
	*type = XA_ATOM;
	target_list[(*length)++] = XA_STRING;
	target_list[(*length)++] = text_atom;
	target_list[(*length)++] = delete_atom;
	target_list[(*length)++] = targets_atom;
	target_list[(*length)++] = host_atom;
	target_list[(*length)++] = length_atom;
	target_list[(*length)++] = selection_end_atom;
	target_list[(*length)++] = available_types_atom;
	target_list[(*length)++] = transfer_type_atom;
        if (filename && *filename)
	  target_list[(*length)++] = data_label_atom;
	*data = (Xv_opaque) target_list;
	return(True);
    } 
    else if (*type == available_types_atom)
    {

	/* This target returns all of the data types that 
	   the holder can convert on the selection. */

	*format = 32;
	*length = 0;
	*type = XA_ATOM;
	types_list[(*length)++] = transfer_type_atom;
	types_list[(*length)++] = XA_STRING;
	*data = (Xv_opaque) types_list;
	return(True);
    } 
    else if ((*type == XA_STRING) || (*type == text_atom) || (*type == transfer_type_atom)) 
    {
	int	bytes_2_copy;

	/* if the number of bytes will fit into one buffer, then we just
	   ship the whole thing at once.  If it is greater that the
	   size of a buffer, we need to go into sending INCR messages. */

	if (!transfer_in_progress && !image_buf)
	{
		image_buf = (CHAR_BUF *) calloc(sizeof(CHAR_BUF), 1);


		drag_pixmap = XCreatePixmap(dpy, preview_xid, icon_width, icon_height, screen_depth);

		XCopyArea(dpy, preview_pixmap, drag_pixmap, 
			gc, preview_boarder_left, preview_boarder_upper, 
			icon_width, icon_height, 0, 0);

     		XWritePixmapData(dpy, cmap, 
				"", &(image_buf->data),
				drag_pixmap, icon_width, 
				icon_height, NULL, 1, NULL);

  		XFreePixmap(dpy, drag_pixmap);

		image_buf->alloc_size = strlen(image_buf->data);
		image_buf->bytes_used = strlen(image_buf->data);

	}

	if (!transfer_in_progress)
	{
		if (image_buf->bytes_used <=
		    Min(XFER_BUFSIZ - 1, *length))
		{

			/* It all fits, ship without using INCR */

			memcpy(xfer_data_buf, image_buf->data, image_buf->bytes_used);
			*format = 8;
			*length = image_buf->bytes_used;
			*data = (Xv_opaque) xfer_data_buf;
			xfer_data_buf[*length] = NULL;
			*type = transfer_type_atom;

			buf_free(image_buf);
			image_buf = NULL;

			return(True);
		}
		else
		{
			/* Too big.  Set up for shipping the stream 
			   as chunks of data */

			Current_point = 0;
			length_buf = image_buf->bytes_used;
			*type = incr_atom;
			*length = 1;
			*format = 32;
			*data = (Xv_opaque) &length_buf;
			transfer_in_progress = TRUE;
			return(True);
		}
	}
	else
	{
		/* Auxilliary request for more data */

		bytes_2_copy = Min(Min(XFER_BUFSIZ - 1, *length), 
			   	image_buf->bytes_used - Current_point);
		memcpy(xfer_data_buf, &image_buf->data[Current_point], bytes_2_copy);
		*format = 8;
		*length = bytes_2_copy;
		*data = (Xv_opaque) xfer_data_buf;
		xfer_data_buf[*length] = NULL;

		*type = transfer_type_atom;

		Current_point += bytes_2_copy;
		if (*length == 0)
		{
			transfer_in_progress = FALSE;
			buf_free(image_buf);
			image_buf = NULL;
		}
		return(True);
	}
    } 
    else
    {
	/* Let the default convert procedure deal with the
	 * request.
	 */

	return(sel_convert_proc(seln, type, data, length, format));
    }
}

/*
 * this routine (may be? is?) needed because we cannot do a blocking
 * request and still have another request come in and be processed.
 * We are about to do an INSERT_SELECTION (which is the expected response...)
 * and we need to be able to respond to the insertsion callback.
 * The one job of this routine is to clean things up, and give an error
 * if stuff failed.
 */
 

/*ARGSUSED*/
static void
discard_load_reply_proc(sel, target, type, replyValue, length, format)
Selection_requestor sel;
Atom target;
Atom type;
Xv_opaque replyValue;
int length;
unsigned format;
{
        Selection_owner owner_sel;
 
        if (debug_on) {
printf(
"discard_load_reply_proc: target %d/%s, type %d/%s, length %d, format %d\n",
target, target ? (char *)xv_get(server, SERVER_ATOM_NAME, target) : "[None]",
type, type ? (char *)xv_get(server, SERVER_ATOM_NAME, type) : "[None]",
length, format);
        }
 
        /* another bug workaround */
        xv_set(sel, SEL_REPLY_PROC, null_proc, 0);
 
        owner_sel = (Selection_owner)
                        xv_get(sel, XV_KEY_DATA, iconedit_key_data);
 
        xv_destroy_safe(owner_sel);
        xv_destroy_safe(sel);
}
 
discard_load()
 
{
        Selection_requestor     Discard_sel;
        Selection               sel;
 
        if (debug_on)
                printf("discard_load called\n");
 
        if (!current_link_atom)
                return(FALSE);
 
        /* allocate the selection to send the data back thru */
 
        sel = xv_create(base_window->window, SELECTION_OWNER,
                        SEL_RANK,               current_link_atom,
                        SEL_CONVERT_PROC,       IconeditSelectionConvert,
                        0);
 
        Discard_sel = xv_create(base_window->window, SELECTION_REQUESTOR,
                        SEL_REPLY_PROC, discard_load_reply_proc,
                        XV_KEY_DATA, iconedit_key_data, sel,
                        0);
 
        /* send the conversion request */
 
        if (debug_on)
                printf("trying to convert INSERT SELECTION against current link at om\n");
 
 
        xv_set(Discard_sel,
                        SEL_TYPE, selection_end_atom,
                        0);
 
        sel_post_req(Discard_sel);
        current_link_atom = NULL;
 
        return(TRUE);
}



init_dragdrop()

{
	Server_image	source_drag_image;
	Server_image	source_drop_image;

	server = XV_SERVER_FROM_WINDOW(base_window->window);
	gethostname(Hostname, MAXHOSTNAMELEN);

	text_atom = xv_get(server, SERVER_ATOM, "TEXT");
	incr_atom = xv_get(server, SERVER_ATOM, "INCR");
	targets_atom = xv_get(server, SERVER_ATOM, "TARGETS");
	length_atom = xv_get(server, SERVER_ATOM, "LENGTH");
	host_atom = xv_get(server, SERVER_ATOM, "_SUN_FILE_HOST_NAME");
	file_name_atom = xv_get(server, SERVER_ATOM, "FILE_NAME");
	atm_file_name_atom = xv_get(server, SERVER_ATOM, "_SUN_ATM_FILE_NAME");
	delete_atom = xv_get(server, SERVER_ATOM, "DELETE");
	selection_end_atom = xv_get(server, SERVER_ATOM, "_SUN_SELECTION_END");
	data_label_atom = xv_get(server, SERVER_ATOM, "_SUN_DATA_LABEL");
	load_atom = xv_get(server, SERVER_ATOM, "_SUN_LOAD");
	insert_selection_atom = xv_get(server, SERVER_ATOM, "INSERT_SELECTION");
	alternate_transport_atom = xv_get(server, SERVER_ATOM, "_SUN_ALTERNATE_TRANSPORT_METHODS");
	available_types_atom = xv_get(server, SERVER_ATOM, "_SUN_AVAILABLE_TYPES");

	enumeration_count_atom = xv_get(server, SERVER_ATOM, "_SUN_ENUMERATION_COUNT");
	color_atom = xv_get(server, SERVER_ATOM, "_SUN_TYPE_color");
	null_atom = xv_get(server, SERVER_ATOM, "NULL");
	transfer_type_atom = xv_get(server, SERVER_ATOM, "_SUN_TYPE_xpm-file");

	iconedit_key_data = xv_unique_key();
	paste_position_key_data = xv_unique_key();

	dnd_context = (DND_CONTEXT *) malloc(sizeof(DND_CONTEXT));
	memset((char *) dnd_context, NULL, sizeof(DND_CONTEXT));

     	source_drag_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, source_drag_ptr,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

     	source_drop_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, source_drop_ptr,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);
	
    	Drop_site1 = xv_create(base_window->preview_canvas, DROP_SITE_ITEM,
		DROP_SITE_DEFAULT,	TRUE,
                DROP_SITE_ID,           1,
                DROP_SITE_EVENT_MASK,
                                DND_MOTION | DND_ENTERLEAVE,
                0);
	
    	Drop_site2 = xv_create(base_window->big_bit_canvas, DROP_SITE_ITEM,
                DROP_SITE_ID,           1,
                DROP_SITE_EVENT_MASK,
                                DND_MOTION | DND_ENTERLEAVE,
                0);

	Dnd_object = xv_create(base_window->preview_canvas, DRAGDROP,
				SEL_CONVERT_PROC, IconeditSelectionConvert,
				DND_TYPE, DND_COPY,
				DND_CURSOR, xv_create(NULL, CURSOR,
					CURSOR_IMAGE, source_drag_image,
					CURSOR_XHOT, 17,
					CURSOR_YHOT, 24,
					CURSOR_OP, PIX_SRC^PIX_DST,
					0),
				DND_ACCEPT_CURSOR, xv_create(NULL, CURSOR,
					CURSOR_IMAGE, source_drop_image,
					CURSOR_XHOT, 17,
					CURSOR_YHOT, 24,
					CURSOR_OP, PIX_SRC^PIX_DST,
					0),
				0);

	Sel = xv_create(base_window->preview_canvas, SELECTION_REQUESTOR, 0);


	drop_site_on(base_window->preview_canvas);

	xv_set((Xv_opaque) canvas_pixwin(base_window->preview_canvas), 
			WIN_CONSUME_EVENTS, 
				LOC_DRAG, ACTION_DRAG_MOVE, ACTION_DRAG_COPY, 0, 
			0);

	notify_interpose_event_func((Notify_client) canvas_pixwin(base_window->preview_canvas), (Notify_func) canvas_event_proc, NOTIFY_SAFE);
	notify_interpose_event_func(base_window->preview_canvas, (Notify_func) canvas_event_proc, NOTIFY_SAFE);
	notify_interpose_event_func(base_window->big_bit_canvas, (Notify_func) canvas_event_proc, NOTIFY_SAFE);
}

lose_clipboard()

{
	if (debug_on)
		printf("lose clipboard called\n");

	if (cut_data)
	{
		free(cut_data);
		cut_data = NULL;
	}
}

copy_to_shelf()

{
        int             status;
        char            notice_buf[MAXPATHLEN];
        Event           event;
        struct stat     statbuf;
	Pixmap		cut_pixmap;

	int             x, y, w, h, preview_x, preview_y, preview_w, preview_h;
	int             erase_all;

	if (((selection_left == 0) && (selection_right == 0)) && ((selection_top == 0) && (selection_bottom == 0)))
	{
	  erase_all = 1;
	  selection_left = boarder_left; selection_top = boarder_upper; selection_right = (canvas_dimension - boarder_right); selection_bottom = (canvas_dimension - boarder_lower);
	}
	else 
	  erase_all = 0;
	
	x = selection_left < selection_right ? selection_left : selection_right;
	y = selection_top < selection_bottom ? selection_top : selection_bottom;
	
	w = selection_left < selection_right ? selection_right - selection_left : selection_left - selection_right; 
	h = selection_top < selection_bottom ? selection_bottom - selection_top : selection_top - selection_bottom;
	
	x = x - boarder_left; y = y - boarder_upper;
	
	preview_x = x/width_block; preview_y = y/height_block; preview_w = w/width_block; preview_h = h/height_block;
	
	if (erase_all == 1)
	  {
	    selection_left = 0; selection_top = 0; selection_right = 0; selection_bottom = 0;
	  }
	

	if (!(cut_pixmap = XCreatePixmap(dpy, preview_xid, preview_w, preview_h, screen_depth)))
		return;


	XCopyArea(dpy, preview_pixmap, cut_pixmap, 
		gc, preview_x + preview_boarder_left, preview_y + preview_boarder_upper, 
		preview_w, preview_h, 0, 0);

	cut_data = NULL;

        if (status = XWritePixmapData(dpy, cmap, 
					"", &cut_data,
					cut_pixmap, preview_w, 
					preview_h, NULL, 1, NULL))
	  {
	    switch (status) {
	        case PixmapNoMemory:
		      sprintf(notice_buf, "Did not have enough memory to write out.");
	              pop_notice(notice_buf);
		      break;
	    }
	  }

/*	XWritePixmapFile(dpy, cmap, "/tmp/sean.debug",  
			 cut_pixmap,
			 preview_w, preview_h,
			 NULL, 1, NULL);
*/
  	XFreePixmap(dpy, cut_pixmap);

        xv_set(Shelf_owner, SEL_OWN, TRUE, 0);
	xv_set(Shelf_item, SEL_DATA, cut_data, 0);

}

cut_to_shelf()

{
	copy_to_shelf();
	clear_area();
}

load_image(xpm_buffer)

char	*xpm_buffer;

{
  char		*errmsg;
  unsigned int 	height, width;
  unsigned long 	*pixels_return;
  unsigned int 	npixles_return;
  ColorSymbol 	*colorsymbols;
  unsigned int 	numsymbols;
  Menu		edit_menu;
  Event		event;
  Visual          *visual;
  int		answer;
  
  
  if (Edited)
    {
      answer = notice_prompt(base_window->window, NULL,
			     NOTICE_MESSAGE_STRINGS,
			     gettext("Your image has been edited.  You may continue,\ndiscarding your edits, or you may cancel your load.\nIf you continue your load, your old image may be restored using Undo."),
			     0,
			     NOTICE_BUTTON_YES, gettext("Cancel"),
			     NOTICE_BUTTON_NO, gettext("Continue"),
			     0);
      
      
      if (answer == NOTICE_YES)
	{
	  return(NULL);
	}
    }
  
  visual = ( Visual * )xv_get( base_window->window, XV_VISUAL );
  
  if (!(errmsg = (char *)XReadPixmapData(dpy, 
					 visual,
					 preview_pixmap, cmap, xpm_buffer, 
					 screen_depth, 
					 &printing_pixmap,  &width, &height,
					 &pixels_return, &npixles_return,
					 colorsymbols, &numsymbols, NULL)))
    {
      icon_height = height;
      icon_width = width;
      
      XFillRectangle( dpy, preview_pixmap, gc_rv, 0 , 0, 138, 138);
      XFillRectangle( dpy, big_pixmap, gc_rv, 0, 0, canvas_dimension, canvas_dimension); 
      XFillRectangle( dpy, preview_xid, gc_rv, 0 , 0, 138, 138);
      XFillRectangle( dpy, big_bit_xid, gc_rv, 0, 0, canvas_dimension, canvas_dimension); 
      
      set_the_size_info();	  
      
      XCopyArea(dpy, printing_pixmap, preview_pixmap, gc, 0, 0, icon_width, icon_height, preview_boarder_left, preview_boarder_upper);  
      
      undo_images[(current_undo+1)%MAX_UNDOS].image = XGetImage(dpy, preview_pixmap, preview_boarder_left, preview_boarder_upper, icon_width, icon_height, -1, ZPixmap); 
      
      undo_images[current_undo%MAX_UNDOS].state = current_color_state;
      undo_images[current_undo%MAX_UNDOS].height = icon_height;
      undo_images[current_undo%MAX_UNDOS].width = icon_width;
      current_undo++;
      
      if (undos_allowed < (MAX_UNDOS -1))
	undos_allowed++;
      
      Edited = 0;
      undo_images[current_undo%MAX_UNDOS].edited = 0;
      
      redos_allowed = 0;
      
      if (undos_allowed == 1)
	{
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), MENU_INACTIVE, FALSE, NULL);
	}
      
      set_iconedit_footer();
      
      undo_images[current_undo%MAX_UNDOS].state = COLOR;
      current_color_state = COLOR;
      if (undo_images[(current_undo-1)%MAX_UNDOS].state == MONO)
	change_stuff_to_color();
      
      icon_height = height;
      icon_width = width;
      
      reset_those_pesky_clip_masks();
      selection_off();
      
      do_that_base_repaint_thing(undo_images[(current_undo)%MAX_UNDOS].image);
      XCopyArea(dpy, preview_pixmap, preview_xid, gc, 
		0, 0, 138, 138, 0, 0); 
    }
  else
    {
      
      char        filename[30];
      char        notice_buf[MAXPATHLEN];
      char        other_error_message[256];
      Server_image temp_server_image;
      GC          bitmap_gc;
      int         this_didnt_work = 1;
      FILE        *file_handle;
      int         status, x, y, border_width, depth;
      extern int  save_format;
      Window      root;
      Pixmap      bitplane_pixmap;
      
      strcpy(filename,"/tmp/icprXXXXXX");
      mktemp(filename);
      file_handle = fopen(filename, "w");
      if (!file_handle)
	{
	  sprintf(notice_buf,  
		  gettext("The drop could not be made."));
	  pop_notice(notice_buf);
	  xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
	  return(FALSE);
	}
      else
	fwrite(xpm_buffer, strlen(xpm_buffer), 1, file_handle);
      fclose(file_handle);
      
      other_error_message[0] = NULL;
      temp_server_image = 
	(Server_image) icon_load_svrim(filename, other_error_message);

      bitmap_gc = XCreateGC(dpy, big_bit_xid, 0, 0);
      bitplane_pixmap = XCreatePixmap(dpy, preview_xid, 
				      icon_width, icon_height, 1);
      
      xv_set(base_window->window, FRAME_BUSY, TRUE, 0);      

      if (!other_error_message[0])
	{
	  status = XGetGeometry(dpy, 
				(Pixmap)xv_get(temp_server_image, XV_XID), 
				&root, &x, &y, &width, &height, 
				&border_width, &depth);
	  
	  if (status == 1) 
	    {
	      icon_height = height;
	      icon_width = width;
	      set_the_size_info();	  
	      XFillRectangle( dpy, preview_pixmap, gc_rv, 
			     0 , 0, 138, 138);
	      XFillRectangle( dpy, big_pixmap, gc_rv, 
			     0, 0, canvas_dimension, canvas_dimension); 
	      XFillRectangle( dpy, preview_xid, gc_rv, 
			     0 , 0, 138, 138);
	      XFillRectangle( dpy, big_bit_xid, gc_rv, 
			     0, 0, canvas_dimension, canvas_dimension); 
	      XSetClipOrigin( dpy, bitmap_gc, 
			     preview_boarder_left, preview_boarder_upper);
	      XSetClipMask( dpy, bitmap_gc, 
			   (Pixmap)xv_get(temp_server_image, XV_XID));
	      XSetForeground(dpy, bitmap_gc, black.pixel);
	      XSetBackground(dpy, bitmap_gc, white.pixel);	  
	      XFillRectangle(dpy, preview_pixmap, bitmap_gc, 
			     0, 0, 138, 138);  
	      undo_images[(current_undo+1)%MAX_UNDOS].image = 
		XGetImage(dpy, preview_pixmap, 
			  preview_boarder_left, preview_boarder_upper, 
			  icon_width, icon_height, -1, ZPixmap); 
	      undo_images[current_undo%MAX_UNDOS].state = 
		current_color_state;
	      undo_images[current_undo%MAX_UNDOS].height = icon_height;
	      undo_images[current_undo%MAX_UNDOS].width = icon_width;
	      
	      current_undo++;
	      if (undos_allowed < (MAX_UNDOS -1))
		undos_allowed++;
	      
	      Edited = 0;
	      undo_images[current_undo%MAX_UNDOS].edited = 0;
	      redos_allowed = 0;
	      
	      if (undos_allowed == 1)
		{
		  edit_menu  = 
		    xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
		  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
			 MENU_INACTIVE, FALSE, NULL);
		}
	      
	      set_iconedit_footer();
	      undo_images[current_undo%MAX_UNDOS].state = MONO;
	      current_color_state = MONO;	      
	      save_format = 1;
	      if (undo_images[(current_undo-1)%MAX_UNDOS].state == COLOR)
		{
		  change_stuff_to_mono();
		}
	      load_file_misc();
	      
	      xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
	      return(FALSE);
	    }
	}   
      
      status = XReadBitmapFile( dpy, preview_pixmap, filename, 
			       &width, &height, &bitplane_pixmap, 0, 0);
      
      if (status != BitmapSuccess)
	{
	  if (status == BitmapNoMemory)
	    sprintf(notice_buf,  
		    gettext("There was not enough memory to read in the drop"));
	  else
	    if (status == BitmapOpenFailed)
	      sprintf(notice_buf,  
		      gettext("'%s' could not be opened") , filename); 
	    else
	      sprintf(notice_buf,  
		      gettext("The drop data was not in a format that iconedit can read."));
	  pop_notice(notice_buf);
	}
      else
	if ((width > 128) || (height > 128))
	  {
	    sprintf(notice_buf,  
		    gettext("The drop is too big for iconedit to read."));
	    pop_notice(notice_buf);
	    xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
	    return(TRUE);
	    
	  }
	else
	  {
	    icon_height = height;
	    icon_width = width;
	    set_the_size_info();	  
	    save_format = 2;
	    XFillRectangle( dpy, preview_pixmap, gc_rv, 
			   0 , 0, 138, 138);
	    XFillRectangle( dpy, big_pixmap, gc_rv, 
			   0, 0, canvas_dimension, canvas_dimension); 
	    XFillRectangle( dpy, preview_xid, gc_rv, 
			   0 , 0, 138, 138);
	    XFillRectangle( dpy, big_bit_xid, gc_rv, 
			   0, 0, canvas_dimension, canvas_dimension); 
	    XSetClipOrigin( dpy, bitmap_gc, 
			   preview_boarder_left, preview_boarder_upper);
	    XSetClipMask( dpy, bitmap_gc, bitplane_pixmap);
	    XSetForeground(dpy, bitmap_gc, black.pixel);
	    XSetBackground(dpy, bitmap_gc, white.pixel);	  
	    XFillRectangle(dpy, preview_pixmap, bitmap_gc, 
			   0, 0, 138, 138);
	    undo_images[(current_undo+1)%MAX_UNDOS].image = 
	      XGetImage(dpy, preview_pixmap, 
			preview_boarder_left, preview_boarder_upper, 
			icon_width, icon_height, -1, ZPixmap); 
	    undo_images[current_undo%MAX_UNDOS].state = 
	      current_color_state;
	    undo_images[current_undo%MAX_UNDOS].height = icon_height;
	    undo_images[current_undo%MAX_UNDOS].width = icon_width;
	    current_undo++;
	    if (undos_allowed < (MAX_UNDOS -1))
	      undos_allowed++;
	    
	    Edited = 0;
	    undo_images[current_undo%MAX_UNDOS].edited = 0;
	    redos_allowed = 0;
	    
	    if (undos_allowed == 1)
	      {
		edit_menu  = 
		  xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
		xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		       MENU_INACTIVE, FALSE, NULL);
	      }
	    
	    set_iconedit_footer();
	    
	    undo_images[current_undo%MAX_UNDOS].state = MONO;
	    current_color_state = MONO;	      
	    
	    if (undo_images[(current_undo-1)%MAX_UNDOS].state == COLOR)
	      {
		change_stuff_to_mono();
	      }
	    
	    load_file_misc();
	    
	    XCopyArea(dpy, preview_pixmap, preview_xid, 
		      bitmap_gc, 0, 0, 138, 138, 0, 0); 
	    XFreePixmap(dpy, bitplane_pixmap);
	  }
      xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
      
      return(FALSE);
    }
}

paste_image(xpm_buffer, x, y)

char	*xpm_buffer;
int      x;
int      y;
{
	char		*errmsg;
  	unsigned int 	height, width;
  	unsigned long 	*pixels_return;
  	unsigned int 	npixles_return;
  	ColorSymbol 	*colorsymbols;
  	unsigned int 	numsymbols;
	Menu		edit_menu;
	Event		event;
	int             corner_x, corner_y;
	long            pixel, temp_pixel;
	int             lower_corner, right_corner;
	int             color_paste;
        Visual          *visual;
	XColor one, two;

	if (!*xpm_buffer)
	{

		notice_prompt(base_window->window, &event,
			NOTICE_MESSAGE_STRINGS,
			gettext("There was nothing on the shelf"),
			0,
			NOTICE_BUTTON_NO, gettext("Continue"),
			0);
		return;
	}

        visual = ( Visual * )xv_get( base_window->window, XV_VISUAL );

	if (!(errmsg = (char *)XReadPixmapData(dpy, 
					visual,
					preview_pixmap, cmap, xpm_buffer, 
					screen_depth, 
					&printing_pixmap, &width, &height, 
					&pixels_return, &npixles_return,
					colorsymbols, &numsymbols, NULL)))
	{

	  if (npixles_return == 1) {
	    one.pixel = pixels_return[0];
	    one.flags = DoRed | DoGreen | DoBlue;
	    XQueryColor(dpy, cmap, &one);
	    if ((one.red == one.blue == one.green == 0) ||
		(one.red == one.blue == one.green == 65535))
	      color_paste = 0;
	    else 
	      color_paste = 1;
	  } else if (npixles_return == 2) {
	    one.pixel = pixels_return[0];
	    one.flags = DoRed | DoGreen | DoBlue;
	    XQueryColor(dpy, cmap, &one);
	 
	    two.pixel = pixels_return[1];
	    two.flags = DoRed | DoGreen | DoBlue;
	    XQueryColor(dpy, cmap, &two);
	    
	    if (((one.red == one.blue == one.green == 0) ||
		(one.red == one.blue == one.green == 65535)) &&
		((two.red == two.blue == two.green == 0) ||
		 (two.red == two.blue == two.green == 65535)))
	      color_paste = 0;
	    else 
	      color_paste = 1;
	  }
	  else
	    color_paste = 1;

	  corner_x = ((x-boarder_left)/width_block) + preview_boarder_left;
	  corner_y = ((y-boarder_upper)/height_block)+ preview_boarder_upper;

	  XCopyArea(dpy, printing_pixmap, preview_pixmap, preview_gc, 0, 0, width, height, corner_x, corner_y);  
	  
	  undo_images[(current_undo+1)%MAX_UNDOS].image = XGetImage(dpy, preview_pixmap, preview_boarder_left, preview_boarder_upper, icon_width, icon_height, -1, ZPixmap); 
	  
	  undo_images[current_undo%MAX_UNDOS].state = current_color_state;
	  undo_images[current_undo%MAX_UNDOS].height = icon_height;
	  undo_images[current_undo%MAX_UNDOS].width = icon_width;
	  current_undo++;
	
	  if (undos_allowed < (MAX_UNDOS -1))
	    undos_allowed++;

          Edited++;
	  undo_images[current_undo%MAX_UNDOS].edited++;
	  
	  redos_allowed = 0;
	  
	  if (undos_allowed == 1)
	    {
	      edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	      xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), MENU_INACTIVE, FALSE, NULL);
	    }
	  
          set_iconedit_footer();
	
	  if (color_paste == 1) 
	    {
	      undo_images[current_undo%MAX_UNDOS].state = COLOR;
	      current_color_state = COLOR;
	      if (undo_images[(current_undo-1)%MAX_UNDOS].state == MONO)
		{
		  /* Do that color from mono stuff - 
		     reset the pallette, make color pixmap a 
		     valid save format, etc... */
		  
		  change_stuff_to_color();
		  
		}
	    }

	  selection_off();
	  
	  temp_pixel = current_pen.pixel;

	  right_corner = (corner_x - preview_boarder_left);
	  lower_corner = (corner_y - preview_boarder_upper);
	    
	  for (x = right_corner; x < (right_corner + width); x++)
	    {
	      corner_x = (x * width_block) + boarder_left +1;
	      for (y = lower_corner; y < (lower_corner + height); y++)
		{
		  if ( (x < icon_width) && ( y < icon_height) )
		    {
		      corner_y = (y * height_block) + boarder_upper +1;
		      pixel = XGetPixel(undo_images[(current_undo)%MAX_UNDOS].image, x, y);
		      current_pen.pixel = pixel;
		      XSetForeground(dpy, redraw_gc, current_pen.pixel);
		      XFillRectangle(dpy, big_bit_xid, redraw_gc, corner_x, corner_y, width_block-1, height_block-1);	    
		      XFillRectangle(dpy, big_pixmap, redraw_gc, corner_x, corner_y, width_block-1, height_block-1);	    
		      XDrawPoint(dpy, preview_pixmap, redraw_gc, x+preview_boarder_left, y+preview_boarder_upper);
		      XDrawPoint(dpy, preview_xid, redraw_gc, x+preview_boarder_left, y+preview_boarder_upper);
		    }
		}
	    }
	  XSetForeground(dpy, redraw_gc, black.pixel);
	  current_pen.pixel = temp_pixel;

/*	  XCopyArea(dpy, preview_pixmap, preview_xid, gc, 0, 0, 138, 138, 0, 0);  */
	}
	else
	  {
	    
	    if (debug_on)
	      printf("shelf = '%s'\n", xpm_buffer);
	    notice_prompt(base_window->window, &event,
			  NOTICE_MESSAGE_STRINGS,
			  gettext("The data on the shelf was not XPM format"),
			  0,
			  NOTICE_BUTTON_NO, gettext("Continue"),
			  0);
	    return;
	  }
	
      }


void
paste_reply_proc(sel_req, target, type, replyBuf, len, format )

Selection_requestor     sel_req;
Atom                    target;
Atom                    type;
Xv_opaque               replyBuf;
unsigned long           len;
int                     format;

{
	char	*atom_name;
	char	*target_name;
	int	*err_ptr = (int *) replyBuf;
	char	*char_buf = (char *) replyBuf;
	Event	event;
    	int	alert_result;
	int     x, y;

	corner_struct   *server_hidden_data;

	server_hidden_data = (corner_struct *)xv_get(sel_req, XV_KEY_DATA, paste_position_key_data);

	x =  server_hidden_data->x;
	y =  server_hidden_data->y;
	
	/* try to turn the type and target atoms into 
	   some useful text for debugging. */

	if (debug_on)
	{
		if (type > 0)
			atom_name = XGetAtomName((Display *) xv_get(server, XV_DISPLAY), type);
		else
			atom_name = "[None]";
		if (target > 0)
			target_name = XGetAtomName((Display *) xv_get(server, XV_DISPLAY), target);
		else
			target_name = "[None]";

		printf("entered paste reply proc, type name  = %s, type atom = %d\n target_name = %s, target atom = %d\n len = %d, format = %d, buf = %d\nstate = %d\n",
		atom_name, type, target_name, target, len, format, err_ptr, dnd_context->state_mask);
	}

	/* Simply processing the return from the termination 
	   request.  No action necessary */

	if (target == selection_end_atom)
		return;

	if ((len == SEL_ERROR) && ((*err_ptr) == SEL_BAD_CONVERSION))
	{
		/* our request failed.  Must be no holder, or no data */

		notice_prompt(base_window->window, &event,
			NOTICE_MESSAGE_STRINGS,
			gettext("There was nothing on the shelf"),
			0,
			NOTICE_BUTTON_NO, gettext("Continue"),
			0);
		return;

	}
	else if (len == SEL_ERROR)
	{
		/* Some internal error happened as a result of 
		   an earlier posted request.  Tell the user. */

		switch (*err_ptr) {
		case SEL_BAD_PROPERTY :
		        xv_set(base_window->window, FRAME_LEFT_FOOTER, 
			       gettext("ReplyProc: Bad property!"), 0);
          		break;
      		case SEL_BAD_TIME:
          		xv_set(base_window->window, FRAME_LEFT_FOOTER, 
			       gettext("ReplyProc: Bad time!"), 0);
          		break;
      		case SEL_BAD_WIN_ID:
          		xv_set(base_window->window, FRAME_LEFT_FOOTER, 
			       gettext("ReplyProc: Bad window id!"), 0);
          		break;
      		case SEL_TIMEDOUT:
          		xv_set(base_window->window, FRAME_LEFT_FOOTER, 
			       gettext("ReplyProc: Timed out!"), 0);
          		break;
      		case SEL_PROPERTY_DELETED:
          		xv_set(base_window->window, FRAME_LEFT_FOOTER, 
			       gettext("ReplyProc: Property deleted!"), 0);
          		break;
      		case SEL_BAD_PROPERTY_EVENT:
          		xv_set(base_window->window, FRAME_LEFT_FOOTER, 
			       gettext("ReplyProc: Bad property event!"), 0);
          		break;
		}
		return;
	}
	else if (type == incr_atom)
		processing_paste_stream = TRUE;
    	else if (target == XA_STRING)
	{
		/* data stream coming thru */

		if (len)
		{
			/* The length is non-zero, so data, and 
			   not the end of transmission */

			if (!paste_data)
				paste_data = buf_alloc();

			if (buf_append(paste_data, char_buf, len))
			{
				/* Memory allocation failed */

				buf_free(paste_data);

				paste_data = NULL;

				notice_prompt(base_window->window, &event,
					NOTICE_MESSAGE_STRING,
	gettext("There was not enough space to make\nthe data transfer.  The paste operation\nhas been cancelled."),
					NOTICE_BUTTON_NO, gettext("Continue"),
					0);
				return;
			}


			if (!processing_paste_stream)
			{
				paste_image(paste_data->data, x, y);

				buf_free(paste_data);
				paste_data = NULL;
	                	/* To complete the drag and drop operation,
                	 	 * we tell the source that we are all done.
                 		 */
 
				return;
			}
		}
	    	else if (processing_paste_stream)
		{
			/* The length was 0, so we have the 
			   end of a data transmission */

			paste_image(paste_data->data, x, y);

			buf_free(paste_data);
			paste_data = NULL;

	                /* To complete the drag and drop operation,
                 	 * we tell the source that we are all done.
                 	 */
			return;
		}
		else
			return;
	}
}

paste_from_shelf(x, y)
     int x;
     int y;
{
  corner_struct *corner;

  corner = (corner_struct *) malloc(sizeof(corner_struct));

  corner->x = x;
  corner->y = y;

  xv_set(Shelf_requestor, 
	 XV_KEY_DATA, paste_position_key_data, corner,
	 SEL_REPLY_PROC, paste_reply_proc,
	 0);
  sel_post_req(Shelf_requestor);

}

clear_area()
{
        int x, y, w, h, corner_x, corner_y, preview_x, preview_y, preview_w, preview_h, erase_all;
	long int lastpixel, pixel;

	Menu edit_menu;

	xv_set(base_window->window, FRAME_BUSY, TRUE, 0);
	    
	XDrawRectangle(dpy, big_bit_xid, select_gc, selection_left, 
	selection_top, selection_right - selection_left, 
	selection_bottom - selection_top); 

	if (((selection_left == 0) && (selection_right == 0)) && ((selection_top == 0) && (selection_bottom == 0)))
	  {
	    erase_all = 1;
	    selection_left = boarder_left; selection_top = boarder_upper; selection_right = (canvas_dimension - boarder_right); selection_bottom = (canvas_dimension - boarder_lower);
	  }
	else 
	  erase_all = 0;
	
	x = selection_left < selection_right ? selection_left : selection_right;
	y = selection_top < selection_bottom ? selection_top : selection_bottom;
	
	w = selection_left < selection_right ? selection_right - selection_left : selection_left - selection_right; 
	h = selection_top < selection_bottom ? selection_bottom - selection_top : selection_top - selection_bottom;
	
	x = x - boarder_left; y = y - boarder_upper;
	
	preview_x = x/width_block; preview_y = y/height_block; preview_w = w/width_block; preview_h = h/height_block;
	
	XSetForeground(dpy, moving_gc, white.pixel);
	XSetForeground(dpy, preview_moving_gc, white.pixel);
	
	XFillRectangle(dpy, big_pixmap, moving_gc, x+boarder_left, y+boarder_upper, w+1, h+1);
	XFillRectangle(dpy, big_bit_xid, moving_gc, x+boarder_left, y+boarder_upper, w+1, h+1);
	XFillRectangle(dpy, preview_xid, preview_moving_gc, preview_x+preview_boarder_left, preview_y+preview_boarder_upper, preview_w, preview_h);
	XFillRectangle(dpy, preview_pixmap, preview_moving_gc, preview_x+preview_boarder_left, preview_y+preview_boarder_upper, preview_w, preview_h);

	undo_images[current_undo%MAX_UNDOS].state = current_color_state;
	undo_images[current_undo%MAX_UNDOS].height = icon_height;
	undo_images[current_undo%MAX_UNDOS].width = icon_width;
	current_undo++;
	if (undos_allowed < (MAX_UNDOS -1))
	  undos_allowed++;
	
	Edited++;
	undo_images[current_undo%MAX_UNDOS].edited++;

	if (undos_allowed == 1)
	  {
	    edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	    xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), MENU_INACTIVE, FALSE, NULL);
	  }
	
	undo_images[current_undo%MAX_UNDOS].image = XGetImage(dpy, preview_pixmap, preview_boarder_left, preview_boarder_upper, icon_width, icon_height, -1, ZPixmap); 
	grid_proc(grid_status);
	    
	if (redo_possible == TRUE)
	  {
	    redo_possible = FALSE;
	    redos_allowed = 0;
	    edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	    xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), MENU_INACTIVE, TRUE, NULL);
	  }
	if (erase_all == 1)
	  {
	    selection_left = 0; selection_top = 0; selection_right = 0; selection_bottom = 0;
	  }
	xv_set(base_window->window, FRAME_BUSY, FALSE, 0);

	set_iconedit_footer();
}
