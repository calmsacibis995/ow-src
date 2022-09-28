#ifndef lint
static char sccsid[] = "@(#)dragdrop.c 3.11 96/12/04 Copyr 1997 Sun Micro";
#endif

/*
 * Copyright (c) 1986, 1987, 1988, 1997 by Sun Microsystems, Inc.
 */

/*
 * This textedit is the envelope that binds various text display and
 *   editing facilities together.
 */

#include <sys/param.h> /* MAXPATHLEN (include types.h if removed) */
#include <ds_verbose_malloc.h>

#ifdef SVR4
#include <dirent.h>   /* MAXNAMLEN */
#include <netdb.h>   /* MAXNAMLEN */
#else
#include <sys/dir.h>   /* MAXNAMLEN */
#endif

#include <sys/stat.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <X11/X.h>
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
#include "textedit.h"

#ifdef MA_DEBUG
#define	DP(block) if(1){ printf("%s:%d ", __FILE__, __LINE__); block }
#else
#define	DP(block) if(0){ block }
#endif

Atom	current_type;

DND_CONTEXT	*dnd_context;
Atom		current_link_atom;

Atom text_atom;
Atom incr_atom;
Atom targets_atom;
Atom length_atom;
Atom host_atom;
Atom file_name_atom;
Atom atm_file_name_atom;
Atom delete_atom;
Atom selection_end_atom;
Atom dragdrop_done_atom;
Atom data_label_atom;
Atom load_atom;
Atom name_atom;
Atom alternate_transport_atom;
Atom available_types_atom;
Atom enumeration_count_atom;
Atom insert_selection_atom;
Atom null_atom;
char xfer_data_buf[XFER_BUFSIZ];


Selection_requestor	Sel;
Dnd			Dnd_object;
Xv_Server		My_server;
int			discarded = FALSE;

static int   		(*old_item_proc)();
static int   		load_on = FALSE;

void
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
			atom_name = XGetAtomName((Display *) xv_get(My_server, XV_DISPLAY), atom_list[i]);
		else
			atom_name = "[None]";

		printf(" %s ", atom_name?atom_name:"(null)");

	}
	printf("] \n");
}

atom_in_list(atom, atom_list, length)
Atom	atom;
Atom	*atom_list;
int	length;
{
	int	i;

	DP(printf("called atom_in_list\n");)
	if (!atom_list)
		return(FALSE);

	for (i = 0; i < length; i++)
		if (atom_list[i] == atom)
			return(True);

	return(False);
}

/* Clears the context block in preparation for 
   a new drag/drop transaction */

clear_context(dnd_ptr)
DND_CONTEXT	*dnd_ptr;
{
	DP(printf("called clear_context\n");)
	if (dnd_ptr->target_list)
		FREE(dnd_ptr->target_list);
	dnd_ptr->target_list = NULL;

	dnd_ptr->num_targets = 0;
	if (dnd_ptr->transport_list)
		FREE(dnd_ptr->transport_list);
	dnd_ptr->transport_list = NULL;

	dnd_ptr->num_transports = 0;
	if (dnd_ptr->data_label)
		FREE(dnd_ptr->data_label);
	dnd_ptr->data_label = NULL;

	dnd_ptr->num_objects = 0;
	if (dnd_ptr->source_host)
		FREE(dnd_ptr->source_host);
	dnd_ptr->source_host = NULL;

	if (dnd_ptr->source_name)
		FREE(dnd_ptr->source_name);
	dnd_ptr->source_name = NULL;

	if (dnd_ptr->source_filename)
		FREE(dnd_ptr->source_filename);
	dnd_ptr->source_filename = NULL;

	dnd_ptr->chosen_target = 0;
	dnd_ptr->chosen_transport = 0;

	dnd_ptr->state_mask = 0;
	dnd_ptr->stop_hit = 0;
	dnd_ptr->transfer_data = 0;
	dnd_ptr->processing_stream = 0;
	dnd_ptr->dnd_stopped = 0;
}

CHAR_BUF *
buf_alloc()
{
	CHAR_BUF	*ptr;

	DP(printf("called buf_alloc\n");)
	if (!(ptr = (CHAR_BUF *) CALLOC(1, sizeof(CHAR_BUF))))
		return(NULL);

	if (!(ptr->data = (char *) MALLOC(10000)))
	{
		FREE(ptr);
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

	DP(printf("called buf_append\n");)
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
		new_block = (char *) REALLOC(ptr->data, ptr->alloc_size + increment);
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
	DP(printf("called buf_free\n");)
	if (!ptr)
		return;

	if (ptr->data)
		FREE(ptr->data);

	FREE(ptr);
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
textedit_reply_proc ( sel_req, target, type, replyBuf, len, format )

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

	/* try to turn the type and target atoms into 
	   some useful text for debugging. */

	DP(printf("called textedit_reply_proc\n");)
	if (debug_on)
	{
		if (type > 0)
			atom_name = XGetAtomName((Display *) xv_get(My_server, XV_DISPLAY), type);
		else
			atom_name = "[None]";
		if (target > 0)
			target_name = XGetAtomName((Display *) xv_get(My_server, XV_DISPLAY), target);
		else
			target_name = "[None]";

		printf("entered reply proc, type name  = %s, type atom = %d\n target_name = %s, target atom = %d\n len = %d, format = %d, buf = %d\nstate = %d\n",
		atom_name, type, target_name, target, len, format, err_ptr, dnd_context->state_mask);
	}

	/* Simply processing the return from the termination 
	   request.  No action necessary */

	if ((target == selection_end_atom) || (target == dragdrop_done_atom))
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
		else if ((target == XA_STRING) || (target == text_atom))
		{
		        xv_set(base_frame, FRAME_LEFT_FOOTER, MGET("Data transfer failed"), 0);
			stop_dragdrop();
			return;
		}
	}
	else if (len == SEL_ERROR)
	{
		/* Some internal error happened as a result of 
		   an earlier posted request.  Tell the user. */

		/* STRING_EXTRACTION
		 * The several lines below are internal error messages.  Do not need to
		 * translate the word "ReplyProc", just translate the phrase after the
		 * colon.
		 */

		switch (*err_ptr) {
		case SEL_BAD_PROPERTY :
		        xv_set(base_frame, FRAME_LEFT_FOOTER, MGET("ReplyProc: Bad property!"), 0);
          		break;
      		case SEL_BAD_TIME:
          		xv_set(base_frame, FRAME_LEFT_FOOTER, MGET("ReplyProc: Bad time!"), 0);
          		break;
      		case SEL_BAD_WIN_ID:
          		xv_set(base_frame, FRAME_LEFT_FOOTER, MGET("ReplyProc: Bad window id!"), 0);
          		break;
      		case SEL_TIMEDOUT:
          		xv_set(base_frame, FRAME_LEFT_FOOTER, MGET("ReplyProc: Timed out!"), 0);
          		break;
      		case SEL_PROPERTY_DELETED:
          		xv_set(base_frame, FRAME_LEFT_FOOTER, MGET("ReplyProc: Property deleted!"), 0);
          		break;
      		case SEL_BAD_PROPERTY_EVENT:
          		xv_set(base_frame, FRAME_LEFT_FOOTER, MGET("ReplyProc: Bad property event!"), 0);
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

				notice_prompt(base_frame, &event,
					NOTICE_MESSAGE_STRINGS,
					MGET("There was not enough space to make\nthe data transfer.  The drop operation\nhas been cancelled."),
					0,
					NOTICE_BUTTON_NO, MGET("Continue"),
					0);

				stop_dragdrop();
				return;
			}


			if (!dnd_context->processing_stream)
			{
				int	old_context;
				/* very unlikey that one buffer will blow 
				   the system out.  Will not bother to check 
				   for memory limit on the textsw */

				xv_set(textsw, TEXTSW_CONTENTS, "", 0);

				old_context = xv_get(textsw, 
						TEXTSW_LOWER_CONTEXT);
				xv_set(textsw, TEXTSW_LOWER_CONTEXT, -1, 0);

				text_insert(textsw, 
					dnd_context->transfer_data->data, 
					dnd_context->transfer_data->bytes_used);
				xv_set(textsw,
					TEXTSW_LOWER_CONTEXT, old_context, 0);

				buf_free(dnd_context->transfer_data);
				dnd_context->transfer_data = NULL;

				/* set textsw to top */

				xv_set(textsw, 
					TEXTSW_INSERTION_POINT, 0, 
					TEXTSW_FIRST, 0,
					TEXTSW_FILE_CONTENTS, 0,
					0);

				edited = 0;

				set_tool_label(dnd_context->data_label, read_only);


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

			xv_set(textsw, TEXTSW_CONTENTS, "", 0);

			text_insert(textsw, 
				dnd_context->transfer_data->data, 
				dnd_context->transfer_data->bytes_used);

			buf_free(dnd_context->transfer_data);
				dnd_context->transfer_data = NULL;

			/* set textsw to top */

			xv_set(textsw, 
				TEXTSW_INSERTION_POINT, 0, 
				TEXTSW_FIRST, 0,
#if 0
				TEXTSW_FILE_CONTENTS, 0,
#endif
				0);

			edited = 0;

			textsw_normalize_view(textsw, 0);


			set_tool_label(dnd_context->data_label, read_only);


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
				FREE(dnd_context->target_list);
				dnd_context->target_list = NULL;
			}

			if (!dnd_context->target_list)
			{
				dnd_context->target_list = (Atom *) MALLOC(len * sizeof(Atom));
				memcpy((char *) dnd_context->target_list, char_buf, len * sizeof(Atom));
				if (!dnd_context->processing_stream)
					dnd_context->state_mask |= TARGET_RESPONSE;
			}
			else
			{
				dnd_context->target_list = (Atom *) REALLOC(dnd_context->target_list, dnd_context->num_targets * sizeof(Atom) + len * sizeof(Atom));
				memcpy((char *) &dnd_context->target_list[dnd_context->num_targets - 1], char_buf, len * sizeof(Atom));
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
				FREE(dnd_context->transport_list);
				dnd_context->transport_list = NULL;
			}

			if (!dnd_context->transport_list)
			{
				dnd_context->transport_list = (Atom *) MALLOC(len * sizeof(Atom));
				memcpy((char *) dnd_context->transport_list, char_buf, len * sizeof(Atom));
				if (!dnd_context->processing_stream)
					dnd_context->state_mask |= TRANSPORT_RESPONSE;
			}
			else
			{
				dnd_context->transport_list = (Atom *) REALLOC(dnd_context->transport_list, dnd_context->num_transports * sizeof(Atom) + len * sizeof(Atom));
				memcpy((char *) &dnd_context->transport_list[dnd_context->num_transports - 1], char_buf, len * sizeof(Atom));
			}
		}
		else
		{
			dnd_context->state_mask |= TRANSPORT_RESPONSE;
			dnd_context->processing_stream = FALSE;
		}

		dnd_context->num_transports += len;
	}
	else if (target == name_atom)
	{
		if (len)
		{
			if (dnd_context->source_name && !dnd_context->processing_stream)
			{
				FREE(dnd_context->source_name);
				dnd_context->source_name = NULL;
			}

			if (!dnd_context->source_name)
			{
				dnd_context->source_name = (char *)
					MALLOC(len + 1);
				memcpy(dnd_context->source_name, char_buf, len);
				dnd_context->source_name[len] = NULL;
				if (!dnd_context->processing_stream)
					dnd_context->state_mask |= HOST_RESPONSE;
			}
			else
			{
				old_length = strlen(dnd_context->source_name);
				dnd_context->source_name = (char *) REALLOC(dnd_context->source_name, old_length + len + 1);
				memcpy(&dnd_context->source_name[old_length], char_buf, len);
				dnd_context->source_name[old_length + len] = NULL;
			}
		}
		else
		{
			dnd_context->state_mask |= HOST_RESPONSE;
			dnd_context->processing_stream = FALSE;
		}
	}
	else if (target == host_atom)
	{
		if (len)
		{
			if (dnd_context->source_host && !dnd_context->processing_stream)
			{
				FREE(dnd_context->source_host);
				dnd_context->source_host = NULL;
			}

			if (!dnd_context->source_host)
			{
				dnd_context->source_host = (char *)
					MALLOC(len + 1);
				memcpy(dnd_context->source_host, char_buf, len);
				dnd_context->source_host[len] = NULL;
				if (!dnd_context->processing_stream)
					dnd_context->state_mask |= HOST_RESPONSE;
			}
			else
			{
				old_length = strlen(dnd_context->source_host);
				dnd_context->source_host = (char *) REALLOC(dnd_context->source_host, old_length + len + 1);
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
				FREE(dnd_context->source_filename);
				dnd_context->source_filename = NULL;
			}

			if (!dnd_context->source_filename)
			{
				dnd_context->source_filename = (char *)
					MALLOC(len + 1);
				memcpy(dnd_context->source_filename, char_buf, len);
				dnd_context->source_filename[len] = NULL;
				if (!dnd_context->processing_stream)
					dnd_context->state_mask |= FILE_NAME_RESPONSE;
			}
			else
			{
				old_length = strlen(dnd_context->source_filename);
				dnd_context->source_filename = (char *) REALLOC(dnd_context->source_filename, old_length + len + 1);
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
	else if (target == data_label_atom)
	{
		if (len)
		{
			if (dnd_context->data_label && !dnd_context->processing_stream)
			{
				FREE(dnd_context->data_label);
				dnd_context->data_label = NULL;
			}

			if (!dnd_context->data_label)
			{
				dnd_context->data_label = (char *)
					MALLOC(len + 1);
				memcpy(dnd_context->data_label, char_buf, len);
				dnd_context->data_label[len] = NULL;
				if (!dnd_context->processing_stream)
					dnd_context->state_mask |= DATA_LABEL_RESPONSE;
			}
			else
			{
				old_length = strlen(dnd_context->data_label);
				dnd_context->data_label = (char *) REALLOC(dnd_context->data_label, old_length + len + 1);
				memcpy(&dnd_context->data_label[old_length], char_buf, len);
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
		current_link_atom = atom_buf[0];
		set_load_state(TRUE);
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
			notice_prompt(base_frame, &event,
				NOTICE_MESSAGE_STRINGS,
				MGET("Text Edit cannot handle multiple\nfiles at once.  Please select one file,\nand try again."),
				0,
				NOTICE_BUTTON_NO, MGET("Continue"),
				0);
			
			stop_dragdrop();
			return;
		}

		if (debug_on)
		{
			dump_atom_list(dnd_context->target_list, dnd_context->num_targets);
			if (!atom_in_list(XA_STRING, dnd_context->target_list, dnd_context->num_targets))
				printf("source does not handle XA_STRING\n");

			if (!atom_in_list(text_atom, dnd_context->target_list, dnd_context->num_targets))
				printf("source does not handle text_atom\n");
		}
	
		if ((!atom_in_list(XA_STRING, dnd_context->target_list, dnd_context->num_targets)) && 
		    (!atom_in_list(atm_file_name_atom, dnd_context->transport_list, dnd_context->num_targets)) && 
		    (!atom_in_list(text_atom, dnd_context->target_list, dnd_context->num_targets)))
		{
			notice_prompt(base_frame, &event,
				NOTICE_MESSAGE_STRINGS,
				MGET("The sourcing application cannot\nsend data that textedit can operate on.\nThe drop operation has been cancelled."),
				0,
				NOTICE_BUTTON_NO, MGET("Continue"),
				0);
			
			stop_dragdrop();
			return;
		}
	
		if (!atom_in_list(XA_STRING, dnd_context->target_list, dnd_context->num_targets))
			dnd_context->chosen_target = text_atom;
		else
			dnd_context->chosen_target = XA_STRING;
	
		if (edited)
			alert_result = (int) notice_prompt(base_frame, &event,
				NOTICE_MESSAGE_STRINGS,
				MGET("Your file has been edited.\nDo you wish to cancel the drag/drop operation,\nsave the current data, or discard these edits?"),
				0,
				NOTICE_BUTTON, MGET("Cancel"), 1,
				NOTICE_BUTTON, MGET("Save"), 2,
				NOTICE_BUTTON, MGET("Discard"), 3,
				0);
	
		if (edited && (alert_result == 1))
		{
			stop_dragdrop();
			return;
		}
		else if (edited && (alert_result == 2))
		{
			if (current_link_atom)
				save_thru_load();
			else if (strcmp(current_filename, MGET("(NONE)")))
				textsw_save(textsw, 0, 0);
			else {
				notice_prompt(base_frame, &event,
					NOTICE_MESSAGE_STRINGS,
					MGET("There is no place to save your data to.\nYou must save the data, and reload the new data."),
					0,
					NOTICE_BUTTON_YES, MGET("Cancel"),
					0);
				stop_dragdrop();
				return;
			     }

			discard_load();
		}
		/* textedit_dstt_breaklink(); */

		/* discard any outstanding load atoms */

		discard_load();
	
		/* determine what sort of data we have coming */
		/* get the host name to go with the file name */
	
		if (atom_in_list(atm_file_name_atom, dnd_context->transport_list, dnd_context->num_transports) && 
		   dnd_context->source_filename)
		{
			if (!strcmp(Hostname, dnd_context->source_host))
			{

                                struct stat stat_buf;
 
                                if (stat (dnd_context->source_filename,
                                                &stat_buf) == 0) {
 
                                   if (S_ISDIR (stat_buf.st_mode))
                                      notice_prompt (base_frame, &event,
                                           NOTICE_MESSAGE_STRINGS,
                                                MGET("Can't load directory"),
                                                dnd_context->source_filename,
                                                NULL,
                                           NOTICE_BUTTON_NO,
                                                MGET ("Cancel drag and drop"),
                                           NULL);
                                   else if (!S_ISREG (stat_buf.st_mode))
                                      notice_prompt (base_frame, &event,
                                           NOTICE_MESSAGE_STRINGS,
                                                dnd_context->source_filename,
                                                MGET("is not a regular file."),
                                                NULL,
                                           NOTICE_BUTTON_NO,
                                                MGET ("Cancel drag and drop"),
                                           NULL);
                                   else if (access (dnd_context->source_filename, R_OK))
                                      notice_prompt (base_frame, &event,
                                           NOTICE_MESSAGE_STRINGS,
                                                dnd_context->source_filename,
                                                MGET("is not a readable file."),
                                                NULL,
                                           NOTICE_BUTTON_NO,
                                                MGET ("Cancel drag and drop"),
                                           NULL);
 
				/* life is hunky dory.  Use the 
				   file name in place. */
	
	               		   else {
				      xv_set(textsw, 
					TEXTSW_FILE, dnd_context->source_filename, 
	               			TEXTSW_INSERTION_POINT, 0,
					0);

					xv_set(textsw, 
						TEXTSW_INSERTION_POINT, 0, 
						TEXTSW_FIRST, 0,
#if 0
						TEXTSW_FILE_CONTENTS, 0,
#endif
						0);
				      xv_set(base_frame, FRAME_CLOSED, FALSE, 0);
				      }
				   }
	
 
                		/* To complete the drag and drop operation,
                 		* we tell the source that we are all done.
                 		*/
		
				stop_dragdrop();

				return;
			}
		}
		/* we have to deal with a data stream */
	
		/* load in the data stream */
	
		/* determine if we can forge a link to it */
	
		xv_set(Sel,
			SEL_REPLY_PROC, textedit_reply_proc,
			SEL_TYPES, load_atom, 0,
			0 );
	
		edited = read_only = 0;

		sel_post_req(Sel);

		xv_set(base_frame, FRAME_CLOSED, FALSE, 0);
	}
	else if (dnd_context->state_mask == RESPONSE_LEVEL_2) 
	{
		if (debug_on)
			printf("second batch of replies processed\n");

		if (current_link_atom)
		{
			(void)sprintf(current_filename, MGET("Link to %s:%s"), 
				dnd_context->source_name?
					dnd_context->source_name:
					"(null)",
				dnd_context->data_label?
					dnd_context->data_label:
					"(null)");
		}
		else
		{
			/* the data is just a copy.  Set the name 
		   	to [No File]. */
	
			(void)strcpy(current_filename, "(NONE)");
	
		}
		/* read the data stream into the textsw */
	
		xv_set(Sel,
			SEL_REPLY_PROC, textedit_reply_proc,
			SEL_TYPES, dnd_context->chosen_target, 0,
			0 );

		xv_set(base_frame, FRAME_CLOSED, FALSE, 0);
	
		dnd_context->state_mask |= BYTES_REQUESTED;

		sel_post_req(Sel);

	}

}

/* Stop the drag and drop operation.  Converts _SUN_SELECTION_END 
   on the current selection, signaling the end, and then 
   puts the rest of the application back to normal. */

stop_dragdrop()
{
	DP(printf("called stop_dragdrop\n");)
	if (debug_on)
			printf("stop_dragdrop called\n");

	/* check to see if we already stopped this transaction; if
	 * so then give up
	 */
	if (dnd_context->dnd_stopped) return;

	/* keep us from ever stopping twice */
	dnd_context->dnd_stopped = 1;

	/* Signal termination of transaction */
	xv_set(Sel,
		SEL_REPLY_PROC, textedit_reply_proc,
		SEL_TYPES, dragdrop_done_atom,
			   selection_end_atom, 
			   0,
		0 );

	/* Free up any outstanding transfer data */

	if (dnd_context->transfer_data)
	{
		buf_free(dnd_context->transfer_data);
		dnd_context->transfer_data = NULL;
	}

	sel_post_req(Sel);

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

	DP(printf("called discard_load_reply_proc\n");)

	if (debug_on) {
printf(
"discard_load_reply_proc: target %d/%s, type %d/%s, length %d, format %d\n",
target, target ? (char *)xv_get(My_server, SERVER_ATOM_NAME, target) : "[None]",
type, type ? (char *)xv_get(My_server, SERVER_ATOM_NAME, type) : "[None]",
length, format);
	}

	/* another bug workaround */
	xv_set(sel, SEL_REPLY_PROC, null_proc, 0);

	owner_sel = (Selection_owner)
			xv_get(sel, XV_KEY_DATA, textedit_key_data);

	xv_destroy_safe(owner_sel);
	xv_destroy_safe(sel);
	if (exiting == TRUE)
	   xv_destroy (base_frame);
	exiting = FALSE;
}

discard_load()
{
	Selection_requestor	Discard_sel;
	Selection		sel;

	DP(printf("called discard_load\n");)
	discarded = FALSE;
	if (debug_on)
		printf("discard_load called\n");

	if (!current_link_atom)
		return(FALSE);

	set_load_state(FALSE);

	/* allocate the selection to send the data back thru */

	sel = xv_create(base_frame, SELECTION_OWNER,
			SEL_RANK,		current_link_atom,
			SEL_CONVERT_PROC, 	TexteditSelectionConvert,
			0);

	Discard_sel = xv_create(base_frame, SELECTION_REQUESTOR, 
			SEL_REPLY_PROC,	discard_load_reply_proc,
			XV_KEY_DATA, textedit_key_data, sel,
			0);

	/* send the conversion request */

	if (debug_on)
		printf("trying to convert INSERT SELECTION against current link atom\n");


    	xv_set(Discard_sel, 
			SEL_TYPE, selection_end_atom, 
			0);

	current_link_atom = NULL;
	discarded = TRUE;
	
	sel_post_req(Discard_sel);

	return(TRUE);
}

load_from_dragdrop()
{
	DP(printf("called load_from_dragdrop\n");)

	if (debug_on) {
		printf("load_from_dragdrop: called\n");
	}

	/* clear the left footer for new response status */

	xv_set(base_frame, FRAME_LEFT_FOOTER, "", 0);

	/* display an alert here, asking if the user wants 
   	   to have any edits discarded.  If they don't, 
   	   then abort the load. */

/*	if(!ma_done_proc(base_frame))
	{
		turnoff_timer();
		xv_set(base_frame, XV_SHOW, TRUE, NULL);
	}
*/
	/* get target types, and see if we lika any of them */

	clear_context(dnd_context);

	xv_set(Sel,
		SEL_REPLY_PROC, textedit_reply_proc,
		SEL_TYPES, 
			targets_atom,
			host_atom,
			name_atom,
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
Xv_server server;
Display *dpy;
char *template;
{
        char buf[100];
        Atom seln_atom;
        int i;


 	DP(printf("called conjure_transient_selection\n");)
       i = 0;
        for(;;) {
                sprintf(buf, template, i);
                seln_atom = xv_get(server, SERVER_ATOM, buf);

                if (! seln_atom) return(0);

                if (XGetSelectionOwner(dpy, seln_atom) == None) {
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

/*ARGSUSED*/
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

	DP(printf("called save_thru_load_reply\n");)

	if (debug_on) {
printf(
"save_thru_load_reply: target %d/%s, type %d/%s, length %d, format %d\n",
target, target ? (char *)xv_get(My_server, SERVER_ATOM_NAME, target) : "[None]",
type, type ? (char *)xv_get(My_server, SERVER_ATOM_NAME, type) : "[None]",
length, format);
	}



	if (type == None) {
		xv_set(base_frame, FRAME_LEFT_FOOTER, MGET("Save back failed"),0);
	} else {
		xv_set(base_frame, FRAME_LEFT_FOOTER, MGET("Save back succeeded"),0);

		xv_set(textsw, 
			TEXTSW_INSERTION_POINT, 0, 
			TEXTSW_FIRST, 0,
#if 0
			TEXTSW_FILE_CONTENTS, 0,
#endif
			0);

		edited = 0;
		set_tool_label(dnd_context->data_label, read_only);
	}

	owner_sel = (Selection_owner)
			xv_get(sel, XV_KEY_DATA, textedit_key_data);

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
	int			value;
	Selection		sel;
	Atom			loadback_atom[2];

	DP(printf("called save_thru_load\n");)
	loadback_atom[0] = 
		conjure_transient_selection(My_server, (Display *) xv_get(My_server, XV_DISPLAY), "DESKSET_SELECTION_%d");
	loadback_atom[1] = XA_STRING;

	if (debug_on)
		printf("save_thru_load called\n");

	if (!current_link_atom)
		return;


	if (debug_on)
	{
		Xv_opaque window;

		window = (Xv_opaque) XGetSelectionOwner(
			(Display *) xv_get(My_server, XV_DISPLAY), current_link_atom);
		printf("owner of selection window is %d\n", window);

		printf("current link atom name = %s\n", XGetAtomName((Display *) xv_get(My_server, XV_DISPLAY), current_link_atom));
		printf("transient selection name = %s\n", XGetAtomName((Display *) xv_get(My_server, XV_DISPLAY), loadback_atom[0]));
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

	sel = xv_create(base_frame, SELECTION_OWNER,
			SEL_RANK,		loadback_atom[0],
			SEL_CONVERT_PROC, 	TexteditSelectionConvert,
			0);


	/* I don't know why this is needed, but it seems to be... */
	xv_set(sel, SEL_OWN, TRUE, 0);

	Load_sel = xv_create(base_frame, SELECTION_REQUESTOR, 
			SEL_RANK, 	current_link_atom,
			SEL_REPLY_PROC,	save_thru_load_reply,
			XV_KEY_DATA, textedit_key_data, sel,
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
	save_data(0,0);
}


/* This routine sets up the external appearance of the tool 
   to deal with the state of owning a link back into some 
   data holders space.  Thus when the user tries to save 
   the data, we know to ship the bytes back to the original 
   data holder. */

set_load_state(on)
int	on;
{
	Menu_item	load_item;

	DP(printf("called set_load_state(%s)\n", on?"True":"False");)
	if (!textsw)
		return;

	load_item = xv_get(xv_get(textsw, TEXTSW_SUBMENU_FILE), MENU_NTH_ITEM, 2);
	if (!load_item)
		return;

	if (on && !load_on)
	{
		old_item_proc = (int (*)()) xv_get(load_item, MENU_NOTIFY_PROC);
		xv_set(load_item, MENU_NOTIFY_PROC, save_thru_load, 0);
		load_on = TRUE;
	}
	else if (!on && load_on)
	{
		xv_set(load_item, MENU_NOTIFY_PROC, old_item_proc, 0);
		load_on = FALSE;
	}
}


/* The convert proc is called whenever someone makes a request to the dnd
 * selection.  Two cases we handle within the convert proc: DELETE and
 * _SUN_SELECTION_END.  Everything else we pass on to the default convert
 * proc which knows about our selection items.
 */


/*ARGSUSED*/
int
TexteditSelectionConvert(seln, type, data, length, format)
    Selection_owner	 seln;
    Atom		*type;
    Xv_opaque		*data;
    unsigned long	*length;
    int			*format;
{
    static int		Current_point;
    static int		length_buf;
    char		*atom_name;
    static Atom		target_list[12];
    static Atom		types_list[2];
    static int		transfer_in_progress = FALSE;
    char		tmpfile [MAXPATHLEN];


	/* try to turn the type and target atoms into 
	   some useful text for debugging. */

	DP(printf("called TexteditSelectionConvert\n");)
	if (debug_on)
	{
		printf("TexteditSelectionConvert conversion called\n");

		if (type != NULL)
			atom_name = XGetAtomName((Display *)xv_get(My_server, XV_DISPLAY), *type);
		else
			atom_name = "[None]";

		printf("TexteditSelectionConvert, being asked to convert %s\n", atom_name);
	}

    /* Interesting sidelight here.  You cannot simply set the type 
       in the reply to the type requested.  It must be the actual 
       type of the data returned.  HOST_NAME, for example would 
       be returnd as type STRING. */

    if ((*type == selection_end_atom) || (*type == dragdrop_done_atom))
    {
			/* Destination has told us it has completed the drag
			 * and drop transaction.  We should respond with a
			 * zero-length NULL reply.
			 */

	/* Yield ownership of the selection */

	xv_set(Dnd_object, SEL_OWN, False, 0);

	xv_set(base_frame, FRAME_LEFT_FOOTER, MGET("Drag and Drop: Completed"),0);
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
    else if (*type == length_atom)
    {
	length_buf = xv_get(textsw, TEXTSW_LENGTH);
	*format = 32;
	*length = 1;
	*type = XA_INTEGER;
	*data = (Xv_opaque) &length_buf;
	return(True);
    } 
    else if (*type == enumeration_count_atom)
    {
	length_buf = 1;
	*format = 32;
	*length = 1;
	*type = XA_INTEGER;
	*data = (Xv_opaque) &length_buf;
	return(True);
    } 
    else if (*type == name_atom)
    {
	/* Return the name of our application (textedit). */

	*format = 8;
	strcpy (tmpfile, "textedit");
	*length = strlen (tmpfile);
	*type = XA_STRING;
	*data = (Xv_opaque) tmpfile;
	return (True);
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
    else if (*type == alternate_transport_atom)
    {

	/* This request should return all of the targets 
	   that can be converted on this selection.  This 
	   includes the types, as well as the queries that 
	   can be issued. */

	if (!edited && strcmp(current_filename, MGET("(NONE)"))
		&& (current_link_atom == (Atom) NULL) )
	{
		*format = 32;
		*length = 1;
		*type = XA_ATOM;
		target_list[0] = atm_file_name_atom;
		*data = (Xv_opaque) target_list;
		return(True);
	}
	else
		return(False);
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
	target_list[(*length)++] = dragdrop_done_atom;
	target_list[(*length)++] = available_types_atom;
	target_list[(*length)++] = name_atom;
	if (current_type)
	{
		target_list[*length] = current_type;
		(*length)++;
	}
	if (!edited && strcmp(current_filename, MGET("(NONE)"))
		&& (current_link_atom == (Atom) NULL) )
	{
		target_list[*length] = alternate_transport_atom;
		(*length)++;
	}
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
	types_list[(*length)++] = XA_STRING;
	if (current_type)
	{
		types_list[(*length)++] = current_type;
	}
	*data = (Xv_opaque) types_list;
	return(True);
    } 
    else if (*type == file_name_atom)
    {
	/* Return the hostname that the application 
	   is running on */

	if (!edited && strcmp(current_filename, MGET("(NONE)")))
	{
		if (strncmp (current_filename, MGET("Link"), 4) == 0)
	 	   sprintf (tmpfile, "%s", 
					(strrchr (current_filename, ':') + 1));
		else	
	 	   sprintf (tmpfile, "%s/%s", current_directory, 
							current_filename);

		*format = 8;
		*length = strlen(tmpfile);
		*data = (Xv_opaque) tmpfile;
		*type = XA_STRING;
		return(True);
	}
	else
		return(False);
    } 
    else if ((*type == XA_STRING) || (*type == text_atom) || (*type == current_type)) 
    {
	int	bytes_2_copy;

	/* if the number of bytes will fit into one buffer, then we just
	   ship the whole thing at once.  If it is greater that the
	   size of a buffer, we need to go into sending INCR messages. */

	if (!transfer_in_progress)
	{
		if (xv_get(textsw, TEXTSW_LENGTH) <=
		    Min(XFER_BUFSIZ - 1, *length))
		{

			/* It all fits, ship without using INCR */

			xv_get(textsw, TEXTSW_CONTENTS, 0, xfer_data_buf, xv_get(textsw, TEXTSW_LENGTH));
			*format = 8;
			*length = (int) xv_get(textsw, TEXTSW_LENGTH);
			*data = (Xv_opaque) xfer_data_buf;
			xfer_data_buf[*length] = NULL;

			if ((*type == text_atom) && current_type)
				*type = current_type;

			return(True);
		}
		else
		{
			/* Too big.  Set up for shipping the stream 
			   as chunks of data */

			Current_point = 0;
			length_buf = xv_get(textsw, TEXTSW_LENGTH);
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
	        int bytes_copied;
		Textsw_index next_pos;

		bytes_2_copy = Min(Min(XFER_BUFSIZ - 1, *length), 
			   	xv_get(textsw, TEXTSW_LENGTH) - Current_point);
		next_pos = xv_get(textsw, TEXTSW_CONTENTS, Current_point, xfer_data_buf, bytes_2_copy);
		bytes_copied = (int)next_pos - Current_point;
		*format = 8;
		*length = bytes_copied;
		*data = (Xv_opaque) xfer_data_buf;
		xfer_data_buf[*length] = NULL;

		if ((*type == text_atom) && current_type)
			*type = current_type;

		Current_point += bytes_copied;
		if (*length == 0)
			transfer_in_progress = FALSE;
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


dragdrop_init()
{
	DP(printf("called dragdrop_init\n");)

	text_atom = xv_get(My_server, SERVER_ATOM, "TEXT");
	incr_atom = xv_get(My_server, SERVER_ATOM, "INCR");
	targets_atom = xv_get(My_server, SERVER_ATOM, "TARGETS");
	length_atom = xv_get(My_server, SERVER_ATOM, "LENGTH");
	host_atom = xv_get(My_server, SERVER_ATOM, "_SUN_FILE_HOST_NAME");
	file_name_atom = xv_get(My_server, SERVER_ATOM, "FILE_NAME");
	name_atom = xv_get(My_server, SERVER_ATOM, "NAME");
	atm_file_name_atom = xv_get(My_server, SERVER_ATOM, "_SUN_ATM_FILE_NAME");
	delete_atom = xv_get(My_server, SERVER_ATOM, "DELETE");
	selection_end_atom = xv_get(My_server, SERVER_ATOM, "_SUN_SELECTION_END");
	dragdrop_done_atom = xv_get(My_server, SERVER_ATOM, "_SUN_DRAGDROP_DONE");
	data_label_atom = xv_get(My_server, SERVER_ATOM, "_SUN_DATA_LABEL");
	load_atom = xv_get(My_server, SERVER_ATOM, "_SUN_LOAD");
	alternate_transport_atom = xv_get(My_server, SERVER_ATOM, "_SUN_ALTERNATE_TRANSPORT_METHODS");
	available_types_atom = xv_get(My_server, SERVER_ATOM, "_SUN_AVAILABLE_TYPES");

	insert_selection_atom = xv_get(My_server, SERVER_ATOM, "INSERT_SELECTION");
	enumeration_count_atom = xv_get(My_server, SERVER_ATOM, "_SUN_ENUMERATION_COUNT");
	null_atom = xv_get(My_server, SERVER_ATOM, "NULL");
	current_type = NULL;

	dnd_context = (DND_CONTEXT *) MALLOC(sizeof(DND_CONTEXT));
	memset((char *) dnd_context, NULL, sizeof(DND_CONTEXT));

	Dnd_object = xv_create(panel, DRAGDROP,
				SEL_CONVERT_PROC, TexteditSelectionConvert,
				DND_TYPE, DND_COPY,
				DND_CURSOR, xv_create(NULL, CURSOR,
					CURSOR_IMAGE, source_drag_ptr_image,
					CURSOR_XHOT, 17,
					CURSOR_YHOT, 24,
					CURSOR_OP, PIX_SRC^PIX_DST,
					0),
				DND_ACCEPT_CURSOR, xv_create(NULL, CURSOR,
					CURSOR_IMAGE, source_drop_ptr_image,
					CURSOR_XHOT, 17,
					CURSOR_YHOT, 24,
					CURSOR_OP, PIX_SRC^PIX_DST,
					0),
				0);
	
	xv_set (source_panel_item, PANEL_DROP_DND, Dnd_object, NULL);

	Sel = xv_get (source_panel_item, PANEL_DROP_SEL_REQ);
}
