#ifndef lint
static char sccsid[] = "@(#)dragdrop.c 3.1 92/04/03";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */


#include <sys/param.h> /* MAXPATHLEN (include types.h if removed) */
#ifdef SVR4
#include <dirent.h>
#else
#include <sys/dir.h>   /* MAXNAMLEN */
#endif
#include <sys/stat.h>

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
#include <xview/xv_xrect.h>
#include "pageview.h"

DND_CONTEXT	*dnd_context;

Atom 		 text_atom;
Atom 		 incr_atom;
Atom		 name_atom;
Atom 		 targets_atom;
Atom 		 length_atom;
Atom 		 sun_length_atom;
Atom		 host_atom;
Atom 		 sun_host_atom;
Atom 		 file_name_atom;
Atom 		 atm_file_name_atom;
Atom 		 delete_atom;
Atom 		 selection_end_atom;
Atom 		 dragdrop_done_atom;
Atom 		 data_label_atom;
Atom 		 load_atom;
Atom 		 alternate_transport_atom;
Atom 		 available_types_atom;
Atom 		 enumeration_count_atom;
Atom 		 enumeration_item_atom;
Atom 		 insert_selection_atom;
Atom		 postscript_file_atom;
Atom		 text_file_atom;
Atom 		 null_atom;
char 		 xfer_data_buf[XFER_BUFSIZ];

char 		 current_filename [MAXNAMLEN];
char 		*dnd_file;

char		*postscript_file_string = DGET ("_SUN_TYPE_postscript-file");
char		*text_file_string = DGET ("_SUN_TYPE_text");
char		*noname_file = DGET ("NoName.ps");

Selection_requestor	Current_Sel;
Dnd			Current_Dnd_object;
Selection_Op		Current_Op;

int			transfer_attempts;

extern int		tooltalk_start;
extern void		quit_pageview ();

void			stop_dragdrop ();
void			ask_for_data ();

/*
 * Check to see if a specific atom was returned in one of the
 * various atom lists from the onwer of the selection.
 */

int
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

/* 
 * Clears the context block in preparation for 
 * a new drag/drop transaction or tooltalk data transfer.
 */

void
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

        if (dnd_ptr->types_list)
	    	free (dnd_ptr->types_list);
	dnd_ptr->types_list = NULL;
	dnd_ptr->num_types = 0;

	if (dnd_ptr->data_label)
		free(dnd_ptr->data_label);
	dnd_ptr->data_label = NULL;

	dnd_ptr->num_objects = 0;
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

/*
 * Allocate a buffer for data that the owner is sending us.
 */

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

/*
 * We need more space in our buffer for the data that the onwer is
 * sending us.
 */

int
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

/*
 * Free a previously allocated buffer.
 */

void
buf_free(ptr)
CHAR_BUF	*ptr;
{
	if (!ptr)
		return;

	if (ptr->data)
		free(ptr->data);

	free(ptr);
}

/* 
 * Huge monolithic routine that processes all the replies 
 * to the questions that I ask about the current drag/drop 
 * selection.  These requests are made in groups, and the 
 * routine contains a state machine whose current state is stored 
 * in the dnd_context block.  This routine updates the 
 * state machine as replies, or failed replies come in.  
 * Changes in state may require issuing new requests for data, 
 * which are processed by this same routine.  
 */

static void
pageview_reply_proc ( sel_req, target, type, replyBuf, len, format )
Selection_requestor  	sel_req;
Atom   			target;
Atom   			type;
Xv_opaque   		replyBuf;
unsigned long  		len;
int    			format;
{
    char	*atom_name;
    char	*target_name;
    int		*err_ptr = (int *) replyBuf;
    char	*char_buf = (char *) replyBuf;
    Atom	*atom_buf = (Atom *) replyBuf;
    int	 	 old_length;
    Event	 event;
    int		 alert_result;
    int		 bytes_inserted;
    int	 	 tmp_file;
    int	 	 bytes_written;
    int		 file_result;

/* 
 * Try to turn the type and target atoms into some useful text 
 * for debugging. 
 */

    if (verbose) {
       if (type != NULL)
	  atom_name = XGetAtomName(dsp, type);
       else
	  atom_name = LGET("[None]");
	  if (target > 0)
	     target_name = XGetAtomName(dsp, target);
	  else
	     target_name = LGET("[None]");

       printf(MGET("entered reply proc, type name  = %s, type atom = %d\n target_name = %s, target atom = %d\n len = %d, format = %d, buf = %d\nstate = %d\n"),
              atom_name, type, target_name, target, len, format, err_ptr, 
	      dnd_context->state_mask);
       }

/* 
 * Simply processing the return from the termination request.  No 
 * action necessary 
 */

    if (target == selection_end_atom)
       return;

/*
 * A conversion of some type failed. Mark the state variable.
 */

    if ((len == SEL_ERROR) && ((*err_ptr) == SEL_BAD_CONVERSION)) {

       if (target == enumeration_count_atom)
	  dnd_context->state_mask |= COUNT_RESPONSE;
       else if (target == sun_host_atom)
	  dnd_context->state_mask |= HOST_RESPONSE;
       else if (target == alternate_transport_atom)
	  dnd_context->state_mask |= TRANSPORT_RESPONSE;
       else if (target == file_name_atom)
	  dnd_context->state_mask |= FILE_NAME_RESPONSE;
       else if (target == data_label_atom)
	  dnd_context->state_mask |= DATA_LABEL_RESPONSE;
       else if (target == available_types_atom)
	  dnd_context->state_mask |= TYPES_RESPONSE;
       else if ((target == XA_STRING) || (target == postscript_file_atom) ||
		(target == text_file_atom) || (target == text_atom)) {

/*
 * We got an error while transferring the data. At this point,
 * let's try and start over up to 3 times. 
 */

	  transfer_attempts++;
	  if (transfer_attempts < 3) {

/*
 * Let's try and start over...
 */

	     buf_free(dnd_context->transfer_data);
	     dnd_context->transfer_data = NULL;

	     ask_for_data (sel_req);

	     }

	  else {

/*
 * We've tried 3 times to get the data, let's forget it.
 */

	     buf_free(dnd_context->transfer_data);
	     dnd_context->transfer_data = NULL;

	     if (verbose)
		fprintf (stderr, MGET("ReplyProc: Bad Conversion!\n"));
             xv_set(baseframe, FRAME_LEFT_FOOTER, 
			       MGET("Data Transfer: Failed!"), 0);

/* 
 * To complete the drag and drop or tooltalk operation,
 * we tell the source that we are all done.  
 */
 
	     stop_dragdrop(sel_req);

/*
 * If we were just started via tooltalk and the data transfer failed,
 * let's just exit since we're worthless at this point anyway...
 */

	     if ((tooltalk_start == TRUE) && (Current_Op == TOOLTALK_OP))
		quit_pageview ();

	     return;
	     }

	  }

       }


/*
 * Some internal error happened as a result of an earlier posted request.
 * Tell the user.
 */

    else if (len == SEL_ERROR) {

       switch (*err_ptr) {
 	  case SEL_BAD_PROPERTY :
	       if (verbose)
		  fprintf (stderr, MGET("ReplyProc: Bad property!\n"));
	       xv_set(baseframe, FRAME_LEFT_FOOTER, 
				 MGET("Data Transfer: Bad property!"), 0);
               break;
      	  case SEL_BAD_TIME:
	       if (verbose)
		  fprintf (stderr, MGET("ReplyProc: Bad time!\n"));
               xv_set(baseframe, FRAME_LEFT_FOOTER, 
					MGET("Data Transfer: Bad time!"), 0);
               break;
      	  case SEL_BAD_WIN_ID:
	       if (verbose)
		  fprintf (stderr, MGET("ReplyProc: Bad window id!\n"));
               xv_set(baseframe, FRAME_LEFT_FOOTER, 
				 MGET("Data Transfer: Bad window id!"), 0);
               break;
      	  case SEL_TIMEDOUT:
	       if (verbose)
		  fprintf (stderr, MGET("ReplyProc: Timed out!\n"));
               xv_set(baseframe, FRAME_LEFT_FOOTER, 
					MGET("Data Transfer: Timed out!"), 0);
               break;
      	  case SEL_PROPERTY_DELETED:
	       if (verbose)
		  fprintf (stderr, MGET("ReplyProc: Property deleted!\n"));
               xv_set(baseframe, FRAME_LEFT_FOOTER, 
				   MGET("Data Transfer: Property deleted!"), 0);
               break;
      	  case SEL_BAD_PROPERTY_EVENT:
	       if (verbose)
		  fprintf (stderr, MGET("ReplyProc: Bad property event!\n"));
               xv_set(baseframe, FRAME_LEFT_FOOTER, 
				MGET("Data Transfer: Bad property event!"), 0);
               break;
	  }

       stop_dragdrop(sel_req);

/*
 * If we were just started via tooltalk and the data transfer failed,
 * let's just exit since we're worthless at this point anyway...
 */

       if ((tooltalk_start == TRUE) && (Current_Op == TOOLTALK_OP))
	  quit_pageview ();

       return;
       }

/*
 * The owner couldn't send us all of the data at one time, so it's
 * trying to tell us that it's going to send it in chunks.
 */

    else if (type == incr_atom) 
       dnd_context->processing_stream = TRUE;

/*
 * We're getting the data from the owner.
 */

    else if ((target == XA_STRING) || (target == postscript_file_atom) ||
	     (target == text_file_atom) || (target == text_atom)) {

       if (len) {

/* 
 * The length is non-zero, so we got data, and not the end of transmission 
 */

	  if (!dnd_context->transfer_data)
	     dnd_context->transfer_data = buf_alloc();

	  if (buf_append(dnd_context->transfer_data, char_buf, len)) {

/* 
 * Memory allocation failed 
 */

	     buf_free(dnd_context->transfer_data);

	     dnd_context->transfer_data = NULL;

	     if (Current_Op == DND_OP)
	        notice_prompt(baseframe, &event, NOTICE_MESSAGE_STRINGS,
				EGET("There was not enough space to make"),
				EGET("the data transfer.  The drop operation"),
				EGET("has been cancelled."), 0,
				NOTICE_BUTTON_NO, LGET("Continue"),
				0);
	     else
	        notice_prompt(baseframe, &event, NOTICE_MESSAGE_STRINGS,
				EGET("There was not enough space to make"),
				EGET("the data transfer.  The tooltalk data"),
				EGET("transfer operation has been cancelled."),
				 0,
				NOTICE_BUTTON_NO, LGET("Continue"),
				0);

  	     stop_dragdrop(sel_req);

/*
 * If we were just started via tooltalk and the data transfer failed,
 * let's just exit since we're worthless at this point anyway...
 */

             if ((tooltalk_start == TRUE) && (Current_Op == TOOLTALK_OP))
	  	quit_pageview ();

	     return;
	     }


	  if (!dnd_context->processing_stream) {

/* 
 * We haven't seen any data before this. Therfore, create a temporary
 * file and start writing the data into it.
 */
			
	     if ((tmp_file = open (dnd_file, 
					O_WRONLY | O_CREAT | O_TRUNC, 0644 ))
						== NULL)
		fprintf (stderr, MGET("%s: Couldn't open temporary file. \n"), 
								ProgramName);
			
	     else {
	        bytes_written = write (tmp_file,
				       dnd_context->transfer_data->data, 
				       dnd_context->transfer_data->bytes_used);
					

		if (bytes_written != dnd_context->transfer_data->bytes_used)
		   fprintf (stderr, 
			  MGET("%s: Didn't write temporary file correctly!\n"), 
			    ProgramName);

		close (tmp_file);
		setbusy ();

/*
 * We may or may not have a file name at this point... let's see if we
 * do, and if so, use that as our filename.
 */

		if (dnd_context->source_filename != (char *) NULL)
		   strcpy (current_filename, 
				basename (dnd_context->source_filename));
		else if (dnd_context->data_label != (char *) NULL)
		   strcpy (current_filename, 
				basename (dnd_context->data_label));
		else
		   strcpy (current_filename, NONAME);
		if ((file_result = file_load (dnd_file)) == -1) {
		   file_not_found (current_filename);
		   }
	
		else if (file_result == 0) {
		   edit_file (dnd_file);
		   set_icon_label (current_filename);
		   xv_set (baseframe, FRAME_CLOSED, FALSE, NULL);
		   }

		setactive ();
		}

	     buf_free(dnd_context->transfer_data);
	     dnd_context->transfer_data = NULL;

/* 
 * To complete the drag and drop or tooltalk operation, we tell the 
 * source that we are all done.  
 */
 
	     stop_dragdrop(sel_req);
	     return;
	     }
	  }

/*
 * The length was zero, so we have the end of the data.
 * At this point, we can write out the data to the temporary file.
 */

       else if (dnd_context->processing_stream) {

	  if ((tmp_file = open (dnd_file, O_WRONLY | O_CREAT | O_TRUNC, 0644 ))
						== NULL)
	     fprintf (stderr, MGET("%s: Couldn't open temporary file. \n"), 
								ProgramName);
			
	  else {
	     bytes_written = write (tmp_file,
				    dnd_context->transfer_data->data, 
				    dnd_context->transfer_data->bytes_used);
					
 
	     if (bytes_written != dnd_context->transfer_data->bytes_used)
	        fprintf (stderr, 
			 MGET("%s: Didn't write temporary file correctly!\n"), 
			 ProgramName);

	     close (tmp_file);
	     setbusy ();

/*
 * We may or may not have a file name at this point... let's see if we
 * do, and if so, use that as our filename.
 */

	     if (dnd_context->source_filename != (char *) NULL)
		strcpy (current_filename, 
				basename (dnd_context->source_filename));
	     else if (dnd_context->data_label != (char *) NULL)
	        strcpy (current_filename, basename (dnd_context->data_label));
	     else
	        strcpy (current_filename, NONAME);
	     if ((file_result = file_load (dnd_file)) == -1) {
	        file_not_found (current_filename);
	        }

	     else if (file_result == 0) {
	        edit_file (dnd_file);
	        set_icon_label (current_filename);
		xv_set (baseframe, FRAME_CLOSED, FALSE, NULL);
	        }

	     setactive ();

	     }

	  buf_free(dnd_context->transfer_data);
	  dnd_context->transfer_data = NULL;

/* 
 * To complete the drag and drop or tooltalk operation, we tell the 
 * source that we are all done.  
 */
 
	  stop_dragdrop(sel_req);
	  return;
	  }

/*
 * The length was zero, and we weren't processing the data,
 * so we didn't get anything, so let's just end this dnd operation,
 * and tell the user we got an empty file.
 */

       else {
 
	  notice_prompt (baseframe, NULL,
		  NOTICE_MESSAGE_STRING, EGET("Empty file!"),
                  NOTICE_BUTTON, LGET("Ok"), 1,
                  NULL);

	  stop_dragdrop(sel_req);
	  return;

	  }
       }

/*
 * We're getting all of the targets supported by the onwer.
 */

    else if (target == targets_atom) {

       if (len) {

	  if (dnd_context->target_list && !dnd_context->processing_stream) {

	     free(dnd_context->target_list);
	     dnd_context->target_list = NULL;
   	     }

	  if (!dnd_context->target_list) {

	     dnd_context->target_list = (Atom *) malloc(len * sizeof (Atom));
	     memcpy((char *) dnd_context->target_list, char_buf, 
						len * sizeof (Atom));
	     if (!dnd_context->processing_stream)
		dnd_context->state_mask |= TARGET_RESPONSE;
	     }

	  else {
	     dnd_context->target_list = 
			(Atom *) realloc(dnd_context->target_list, 
	        		         dnd_context->num_targets * 
					 sizeof (Atom) + len * sizeof (Atom));
	     memcpy(
	      (char *) &dnd_context->target_list[dnd_context->num_targets - 1],
	      char_buf, len * sizeof (Atom));
	     }
	  }

       else {
	  dnd_context->processing_stream = FALSE;
	  dnd_context->state_mask |= TARGET_RESPONSE;
  	  }

       dnd_context->num_targets += len;
       }

/*
 * We're getting the alternate transports supported by the onwer.
 */

    else if (target == alternate_transport_atom) {

       if (len) {

	  if (dnd_context->transport_list && !dnd_context->processing_stream) {
	     free(dnd_context->transport_list);
	     dnd_context->transport_list = NULL;
	     }

	  if (!dnd_context->transport_list) {
	     dnd_context->transport_list = (Atom *) malloc(len * sizeof (Atom));
	     memcpy((char *) dnd_context->transport_list, char_buf, 
							len * sizeof (Atom));
	     if (!dnd_context->processing_stream)
		dnd_context->state_mask |= TRANSPORT_RESPONSE;
	     }

	  else {
	     dnd_context->transport_list = 
			     (Atom *) realloc(dnd_context->transport_list, 
					      dnd_context->num_transports * 
					      sizeof (Atom) + len * 
					      sizeof (Atom));
	     memcpy (
	 (char *) &dnd_context->transport_list[dnd_context->num_transports - 1],
		   char_buf, len * sizeof (Atom));
   	     }
	  }

       else {
	  dnd_context->state_mask |= TRANSPORT_RESPONSE;
	  dnd_context->processing_stream = FALSE;
	  }

       dnd_context->num_transports += len;
       }

/*
 * We're geting the types available from the owner.
 */

    else if (target == available_types_atom) {

       if (len) {

	  if (dnd_context->types_list && !dnd_context->processing_stream) {

	     free(dnd_context->types_list);
	     dnd_context->types_list = NULL;
   	     }

	  if (!dnd_context->types_list) {

	     dnd_context->types_list = (Atom *) malloc(len * sizeof (Atom));
	     memcpy((char *) dnd_context->types_list, char_buf, 
						len * sizeof (Atom));
	     if (!dnd_context->processing_stream)
		dnd_context->state_mask |= TYPES_RESPONSE;
	     }

	  else {
	     dnd_context->types_list = 
				(Atom *) realloc(dnd_context->types_list, 
					         dnd_context->num_types * 
					         sizeof (Atom) + len * 
						 sizeof (Atom));
	     memcpy(
	         (char *) &dnd_context->types_list[dnd_context->num_types - 1],
		          char_buf, len * sizeof (Atom));
	     }
	  }

       else {
	  dnd_context->state_mask |= TYPES_RESPONSE;
	  dnd_context->processing_stream = FALSE;
  	  }

       dnd_context->num_types += len;
       }

/*
 * We're getting the hostname from the owner.
 */

    else if (target == sun_host_atom) {

       if (len) {
 	  if (dnd_context->source_host && !dnd_context->processing_stream) {
 	     free(dnd_context->source_host);
	     dnd_context->source_host = NULL;
	     }

	  if (!dnd_context->source_host) {
	     dnd_context->source_host = (char *) malloc(len + 1);
	     memcpy(dnd_context->source_host, char_buf, len);
	     dnd_context->source_host[len] = NULL;
	     if (!dnd_context->processing_stream)
		dnd_context->state_mask |= HOST_RESPONSE;
	     }

	  else {
	     old_length = strlen(dnd_context->source_host);
	     dnd_context->source_host = 
			          (char *) realloc(dnd_context->source_host, 
				         	   old_length + len + 1);
  	     memcpy(&dnd_context->source_host[old_length], char_buf, len);
	     dnd_context->source_host[old_length + len] = NULL;
	     }
	  }

       else {
	  dnd_context->state_mask |= HOST_RESPONSE;
	  dnd_context->processing_stream = FALSE;
	  }
       }

/*
 * We're getting the file name from the onwer.
 */

    else if (target == file_name_atom) {

       if (len) {
	  if (dnd_context->source_filename && !dnd_context->processing_stream) {
	     free(dnd_context->source_filename);
	     dnd_context->source_filename = NULL;
	     }

	  if (!dnd_context->source_filename) {
	     dnd_context->source_filename = (char *) malloc(len + 1);
	     memcpy(dnd_context->source_filename, char_buf, len);
	     dnd_context->source_filename[len] = NULL;
	     if (!dnd_context->processing_stream)
		dnd_context->state_mask |= FILE_NAME_RESPONSE;
	     }

	  else {
	     old_length = strlen(dnd_context->source_filename);
	     dnd_context->source_filename = 
				(char *) realloc(dnd_context->source_filename,
					         old_length + len + 1);
	     memcpy(&dnd_context->source_filename[old_length], char_buf, len);
	     dnd_context->source_filename[old_length + len] = NULL;
	     }
	  }

       else {
	  dnd_context->state_mask |= FILE_NAME_RESPONSE;
	  dnd_context->processing_stream = FALSE;
	  }
       }

/*
 * We're getting the data label from the owner (could be the file name).
 */

    else if (target == data_label_atom) {

       if (len) {
	  if (dnd_context->data_label && !dnd_context->processing_stream) {
	     free(dnd_context->data_label);
	     dnd_context->data_label = NULL;
	     }

	  if (!dnd_context->data_label) {
	     dnd_context->data_label = (char *) malloc(len + 1);
	     memcpy(dnd_context->data_label, char_buf, len);
	     dnd_context->data_label[len] = NULL;
	     if (!dnd_context->processing_stream) 
		dnd_context->state_mask |= DATA_LABEL_RESPONSE;
	     }

	  else {
	     old_length = strlen(dnd_context->data_label);
	     dnd_context->data_label = 
				(char *) realloc (dnd_context->data_label, 
						  old_length + len + 1);
	     memcpy(&dnd_context->data_label[old_length], char_buf, len);
	     dnd_context->data_label[old_length + len] = NULL;
	     }
	  }

       else {
	  dnd_context->state_mask |= DATA_LABEL_RESPONSE; 
	  dnd_context->processing_stream = FALSE;
	  }
       }


/*
 * We're getting the total number of itmes being sent from the onwer.
 */

    else if (target == enumeration_count_atom) {

       dnd_context->num_objects = atom_buf[0];
       dnd_context->state_mask |= COUNT_RESPONSE;
       }

/*
 * We're getting something we didn't ask for or expect.
 */

    else
       return;


/*
 * If we just got all of the responses for our first batch of questions,
 * let's see if we can get at the file (ie did we get the file name).
 */

    if (dnd_context->state_mask == RESPONSE_LEVEL_1) {
       if (verbose)
	  printf(MGET("first batch of replies processed, asking for second\n"));

/*
 * The owner wants us to process more than one object. We aren't set up
 * to do that, so let's tell him to forget it!
 */

       if (dnd_context->num_objects > 1) {
	  notice_prompt(baseframe, &event, NOTICE_MESSAGE_STRINGS,
				EGET("Pageview cannot handle multiple"),
				EGET("files at once.  Please select one file,"),
				EGET("and try again."), 0,
				NOTICE_BUTTON_NO, LGET("Continue"),
				0);
			
       stop_dragdrop(sel_req);
       return;
       }
	
/*
 * Let's check the types available from the owner. If it's one we can
 * handle, let's proceed, otherwise, tell him we can't deal with it
 * and forget about it.
 */

       if (((!atom_in_list(XA_STRING, dnd_context->types_list, 
					dnd_context->num_types)) &&
	    (!atom_in_list(XA_STRING, dnd_context->target_list,
					dnd_context->num_targets))) && 
	   ((!atom_in_list(text_atom, dnd_context->types_list, 
					dnd_context->num_types)) &&
	    (!atom_in_list(text_atom, dnd_context->target_list,
					dnd_context->num_targets))) &&
	   (!atom_in_list(text_file_atom, dnd_context->types_list, 
					dnd_context->num_types)) && 
	   (!atom_in_list (postscript_file_atom, dnd_context->types_list, 
					dnd_context->num_types))) {
	  if (Current_Op == DND_OP)
	     notice_prompt(baseframe, &event, NOTICE_MESSAGE_STRINGS,
				EGET("The sourcing application cannot"),
				EGET("send data that pageview can operate on."),
				EGET("The drop operation has been cancelled."),
				 0,
				NOTICE_BUTTON_NO, LGET("Continue"),
				0);
	  else 
	     notice_prompt(baseframe, &event, NOTICE_MESSAGE_STRINGS,
				EGET("The sending application cannot"),
				EGET("send data that pageview can operate on."),
				EGET("The tooltalk data transfer operation has"),
				EGET("been cancelled."), 0,
				NOTICE_BUTTON_NO, LGET("Continue"),
				0);
			
          stop_dragdrop(sel_req);

/*
 * If we were just started via tooltalk and the data transfer failed,
 * let's just exit since we're worthless at this point anyway...
 */

          if ((tooltalk_start == TRUE) && (Current_Op == TOOLTALK_OP))
	     quit_pageview ();

	  return;
	  }
	
	
/* 
 * If the file name transport is supported by the owner, let's just use
 * that. See if we already have the file name... If so, let's try and use
 * it. Also check the host. If any or all of these isn't true, then let's
 * just forget it and ask for the data.
 */
	
       if ((atom_in_list(atm_file_name_atom, dnd_context->transport_list, 
					dnd_context->num_transports)) &&
	   (dnd_context->source_filename || dnd_context->data_label) &&
	   (strcmp(hostname, dnd_context->source_host) == 0)) {

/* 
 * The file is on the local machine. Let's try and get it.
 */
	
	  if (dnd_context->source_filename != (char *) NULL)
	     strcpy (current_filename, dnd_context->source_filename);
	  else
	     strcpy (current_filename, dnd_context->data_label);
	  setbusy ();
	  if ((file_result = file_load (current_filename)) == -1) {

/*
 * The file couldn't be loaded. Don't know what the problem is,
 * but let's ask the owner to send the data instead. Don't bother
 * telling the user that the file couldn't be found since we still
 * have a chance of getting the data.
 */

	     ask_for_data (sel_req);
	     }

	  else if (file_result == 0) {
	     edit_file (current_filename);
	     set_icon_label (current_filename);
	     xv_set (baseframe, FRAME_CLOSED, FALSE, NULL);
	     }

	  setactive ();
 
/* 
 * To complete the drag and drop or tooltalk operation, we tell the 
 * source that we are all done.  
 */
		
	  stop_dragdrop(sel_req);

	  return;
	  }

/* 
 * Let's ask the owner for the data instead...
 */
	
       ask_for_data (sel_req);

       xv_set(baseframe, FRAME_CLOSED, FALSE, 0);

       }

}

/*
 * If we can't get at the file, then we have to ask the owner
 * of the selection to pass the data. Here we decide which type
 * we will ask for depending on the types the owner supports.
 */

void
ask_for_data (sel_req)
Selection_requestor sel_req;
{

    if (atom_in_list (postscript_file_atom, dnd_context->types_list,	
					dnd_context->num_types)) 
       xv_set(sel_req, SEL_REPLY_PROC, pageview_reply_proc,
		           SEL_TYPES, postscript_file_atom, NULL,
		           NULL );
    else if (atom_in_list(text_file_atom, dnd_context->types_list, 
					 dnd_context->num_types))
       xv_set(sel_req, SEL_REPLY_PROC, pageview_reply_proc,
		           SEL_TYPES, text_file_atom, NULL,
		           NULL );
    else if ((atom_in_list(text_atom, dnd_context->types_list,
					dnd_context->num_types)) ||
	     (atom_in_list(text_atom, dnd_context->target_list,
					dnd_context->num_targets)))
       xv_set(sel_req, SEL_REPLY_PROC, pageview_reply_proc,
		           SEL_TYPES, text_atom, NULL,
		           NULL );
    else if ((atom_in_list(XA_STRING, dnd_context->types_list, 
					dnd_context->num_types)) ||
	     (atom_in_list(XA_STRING, dnd_context->target_list,
					dnd_context->num_targets)))
        xv_set(sel_req, SEL_REPLY_PROC, pageview_reply_proc,
		            SEL_TYPES, XA_STRING, NULL,
		            NULL );
	
    dnd_context->state_mask |= BYTES_REQUESTED;

    sel_post_req(sel_req);

}

/* 
 * Stop the drag and drop or tooltalk operation.  Converts 
 * _SUN_SELECTION_END or _SUN_DRAGDROP_DONE on the current selection, 
 * signaling the end, and then puts the rest of the application back to 
 * normal.  
 */

void
stop_dragdrop(sel_req)
Selection_requestor sel_req;
{
	if (verbose)
			printf(MGET("stop_dragdrop called\n"));

/* 
 * Signal termination of transaction.
 */

	xv_set(sel_req,
		SEL_REPLY_PROC, pageview_reply_proc,
		SEL_TYPES, selection_end_atom, 0,
		0 );


/* 
 * Free up any outstanding transfer data.
 */ 

	if (dnd_context->transfer_data)
	{
		buf_free(dnd_context->transfer_data);
		dnd_context->transfer_data = NULL;
	}

	sel_post_req(sel_req);

}

/*
 * We're starting a dnd operation. Determine which replies we want
 * from the owner of the selection, and start the entire process.
 * Note that this function is also used when we're starting at
 * tooltalk `dispatch_data' operation.
 */

void 
load_from_dragdrop()
{

	if (verbose) {
		printf(MGET("load_from_dragdrop: called\n"));
	}

/* 
 * Set the number of transfer attempts to zero.
 */
	
	transfer_attempts = 0;

/* 
 * Clear the left footer for new response status.
 */

	xv_set(baseframe, FRAME_LEFT_FOOTER, "", 0);

/* 
 * Get target types, and see if we lika any of them.
 */

	clear_context(dnd_context);

	xv_set(Current_Sel,
		SEL_REPLY_PROC, pageview_reply_proc,
		SEL_TYPES, 
			enumeration_count_atom,    
			sun_host_atom,
			alternate_transport_atom, 
			file_name_atom,
			data_label_atom,    
			available_types_atom,
			targets_atom,
			0,
		0 );

	sel_post_req(Current_Sel); 


	if (verbose) {
		printf(MGET("load_from_dragdrop: after sel_post_req\n"));
	}

}


/* 
 * The convert proc is called whenever someone makes a request to the dnd
 * selection.  
 */


int
PageviewSelectionConvert(seln, type, data, length, format)
    Selection_owner	 seln;
    Atom		*type;
    Xv_opaque		*data;
    unsigned long	*length;
    int			*format;
{
    static int		Current_point;
    static int		count = 1;
    static int		item = 0;
    int			length_buf;
    Xv_Server 		server = XV_SERVER_FROM_WINDOW(xv_get(seln, XV_OWNER));
    char		*atom_name;
    char		atm_file [MAXPATHLEN+MAXNAMLEN];
    static Atom		target_list[19];
    static Atom		types_list[2];
    static Atom		transport_atoms [1];
    static int		transfer_in_progress = FALSE;


/* 
 * Try to turn the type and target atoms into some useful text for 
 * debugging. 
 */

	if (verbose)
	{
		printf(MGET("PageviewSelectionConvert conversion called\n"));

		if (type != NULL)
			atom_name = XGetAtomName(dsp, *type);
		else
			atom_name = LGET("[None]");

		printf(MGET("PageviewSelectionConvert, being asked to convert %s\n"), 
			    atom_name);
	}

/* 
 * Interesting sidelight here.  You cannot simply set the type 
 * in the reply to the type requested.  It must be the actual 
 * type of the data returned.  HOST_NAME, for example would 
 * be returnd as type STRING. 
 */

/* 
 * Destination has told us it has completed the drag and drop transaction.  
 * We should respond with a zero-length NULL reply.
 */

    if ((*type == selection_end_atom) || (*type == dragdrop_done_atom))
    {

/* 
 * Reset 'transfer_in_progres' and Current_point just in  case we screwed 
 * up somewhere. 
 */

	transfer_in_progress = FALSE;
	Current_point = 0;

/* 
 * Yield ownership of the selection.
 */

	xv_set(Current_Dnd_object, SEL_OWN, False, 0);

	xv_set(baseframe, FRAME_LEFT_FOOTER, 
				MGET("Data Transfer: Completed"), 0);
	*format = 32;
	*length = 0;
	*data = NULL;
	*type = null_atom;
	return(True);
    } 

/* 
 * In our case, we chose not to listen to delete 
 * commands on the drag/drop item.  We never mean 
 * to do a move, always a copy. 
 */

    else if (*type == delete_atom) 
    {
	*format = 32;
	*length = 0;
	*data = NULL;
	*type = null_atom;
	return(True);
    } 

/*
 * Destination wants the length of the data. Send the length of the file.
 */
 
    else if ((*type == length_atom) || (*type == sun_length_atom))
    {
	length_buf = xv_get(edit_textsw, TEXTSW_LENGTH);
	*format = 32;
	*length = 1;
	*type = XA_INTEGER;
	*data = (Xv_opaque) &length_buf;
	return(True);
    } 

/*
 * Destination wants the transport mechanisms we support. For now, it's
 * just the _SUN_ATM_FILE_NAME.
 */

    else if (*type == alternate_transport_atom)
    {
	*format = 32;
	*type = XA_ATOM;
	if ((strcmp (iconname, NONAME) == 0) ||
	    (strncmp (basename (pathname), DGET ("PV"), 2) == 0)) {
	   *length = 0;
	   *data = (Xv_opaque) NULL;
	   return (False);
	   }
	*length = 1;
        transport_atoms [0] = atm_file_name_atom;
	*data = (Xv_opaque) transport_atoms;
	return (True);
    }

/*
 * Destination wants the file name with host name prepended.
 */
 
    else if (*type == atm_file_name_atom)
    {

/*
 * If we don't have a file name, then just send a NULL
 */

	*format = 8;
	*type = XA_STRING;
	if ((strcmp (iconname, NONAME) == 0) ||
	    (strncmp (basename (pathname), DGET ("PV"), 2) == 0)) {
	   *length = 0;
	   *data = (Xv_opaque) NULL;
	   return (False);
	   }
 	else {
	   sprintf (atm_file, "%s:%s", hostname, pathname);
	   *length = strlen (atm_file);
	   *data = (Xv_opaque) atm_file;
	}
	return (True);
    }

/*
 * Desination wants to know the number of items it will get.
 */

    else if (*type == enumeration_count_atom)
    {
	*format = 32;
	*length = 1;
	*type = XA_INTEGER;
	*data = (Xv_opaque) &count;
	return(True);
    } 

/* 
 * Desination wants to know which item is the current one.
 */

    else if (*type == enumeration_item_atom)
    {
	*format = 32;
	*length = 0;
	*type = null_atom;
	*data = (Xv_opaque) &item;
	return(True);
    }

/*
 * Destination wants to know the hostname of the owner.
 */

    else if ((*type == sun_host_atom) || (*type == host_atom))
    {
	/* Return the hostname that the application 
	   is running on */

	*format = 8;
	*length = strlen(hostname);
	*data = (Xv_opaque) hostname;
	*type = XA_STRING;
	return(True);
    } 

/*
 * Destination wants to know the file name (entire pathname).
 */

    else if (*type == file_name_atom) 
    {
	/* Return the file name */


/* 
 * Determine if we have a file name, if not, send NULL.
 */

	*format = 8;
	*type = XA_STRING;
	if ((strcmp (iconname, NONAME) == 0) ||
	    (strncmp (basename (pathname), DGET ("PV"), 2) == 0)) {
	   *length = 0;
	   *data = (Xv_opaque) NULL;
	   return (False);
	   }
	else {	
	   *length = strlen (pathname);
	   *data = (Xv_opaque) pathname;
	   }
	return (True);
    }

/* 
 * Destination wants to know the data label of the file... in this
 * case, we just give them the file name, not the complete path.
 */

    else if (*type == data_label_atom)
    {
	/* Return the last part of the file name */

/* 
 * Determine if we have a file name, if not, use NoName.ps
 */

	*format = 8;
	*type = XA_STRING;
	if (strcmp (iconname, NONAME) == 0) {
	   *length = strlen (noname_file);
	   *data = (Xv_opaque) noname_file;
	   }
	else {
	   *length = strlen (iconname);
	   *data = (Xv_opaque) iconname;
	   }
	return (True);
    }

/*
 * Destination wants to know our applications name...
 */

    else if (*type == name_atom)
    {
	/* Return the name of this application */

	*format = 8;
	*length = strlen (PV_Name);
	*data = (Xv_opaque) PV_Name;
	*type = XA_STRING;
	return (True);
    }

/*
 * Destination wants to know all of the target atoms we support.
 */

    else if (*type == targets_atom)
    {

	*format = 32;
	*length = 16;
	*type = XA_ATOM;
	target_list[0] = XA_STRING;
	target_list[1] = delete_atom;
	target_list[2] = targets_atom;
	target_list[3] = host_atom;
	target_list[4] = sun_host_atom;
	target_list[5] = length_atom;
	target_list[6] = selection_end_atom;
	target_list[7] = available_types_atom;
	target_list[8] = data_label_atom;
        target_list[9] = file_name_atom;
  	target_list[10] = dragdrop_done_atom;
  	target_list[11] = sun_length_atom;
  	target_list[12] = alternate_transport_atom;
  	target_list[13] = atm_file_name_atom;
	target_list[14] = enumeration_count_atom;
	target_list[15] = enumeration_item_atom;
   	target_list[16] = text_atom;
   	target_list[17] = name_atom;
 	target_list[18] = postscript_file_atom;

	*data = (Xv_opaque) target_list;
	return(True);
    } 

/*
 * Destination wants to know all of the data types we support.
 */

    else if (*type == available_types_atom)
    {

	*format = 32;
	*length = 2;
	*type = XA_ATOM;
	types_list[0] = postscript_file_atom;
	types_list[1] = XA_STRING;

	*data = (Xv_opaque) types_list;
	return(True);
    } 

/*
 * Destination wants the data!
 */

    else if ((*type == XA_STRING) || (*type == postscript_file_atom) 
			|| (*type == text_atom)) 
    {
	int	bytes_2_copy;

/*
 * Get the length of the data...
 */

	*length = (int) xv_get(edit_textsw, TEXTSW_LENGTH);

/* 
 * If the number of bytes will fit into one buffer, then we just
 * ship the whole thing at once.  If it is greater that the
 * size of a buffer, we need to go into sending INCR messages. 
 */

	if (!transfer_in_progress)
	{
		if (xv_get(edit_textsw, TEXTSW_LENGTH) <=
		    Min(XFER_BUFSIZ - 1, *length))
		{

/* 
 * It all fits, ship without using INCR.
 */

			xv_get(edit_textsw, TEXTSW_CONTENTS, 0, xfer_data_buf, 
				      xv_get(edit_textsw, TEXTSW_LENGTH));
			*format = 8;
			*data = (Xv_opaque) xfer_data_buf;
			*type = XA_STRING;
			xfer_data_buf[*length] = NULL;

			return(True);
		}

/*
 * Too big. Set up for shipping the stream as chunks of data.
 */

		else
		{
			Current_point = 0;
			length_buf = xv_get(edit_textsw, TEXTSW_LENGTH);
			*type = incr_atom;
			*length = 1;
			*format = 32;
			*data = (Xv_opaque) &length_buf;
			transfer_in_progress = TRUE;
			return(True);
		}
	}

/*
 * Destination wants more data.
 */

	else
	{

		bytes_2_copy = Min ( Min(XFER_BUFSIZ - 1, *length), 
			   	     xv_get(edit_textsw, TEXTSW_LENGTH) - 
				     Current_point);
		xv_get(edit_textsw, TEXTSW_CONTENTS, Current_point, 
		       xfer_data_buf, bytes_2_copy);
		*format = 8;
		*length = bytes_2_copy;
		*type = XA_STRING;
		*data = (Xv_opaque) xfer_data_buf;
		xfer_data_buf[*length] = NULL;

		Current_point += bytes_2_copy;
		if (*length == 0)
			transfer_in_progress = FALSE;
		return(True);
	}
    } 

/* 
 * Let the default convert procedure deal with the request.
 */

    else
    {

	return(sel_convert_proc(seln, type, data, length, format));
    }
}


/* 
 * Initialize atoms needed for dnd.
 */

void
dragdrop_init()
{
 	dnd_file = (char *) mktemp (DGET("/tmp/PVDnd-XXXXXX"));
	text_atom = xv_get(My_server, SERVER_ATOM, DGET("TEXT"));
	incr_atom = xv_get(My_server, SERVER_ATOM, DGET("INCR"));
   	name_atom = xv_get (My_server, SERVER_ATOM, DGET("NAME"));
	targets_atom = xv_get(My_server, SERVER_ATOM, DGET("TARGETS"));
	length_atom = xv_get(My_server, SERVER_ATOM, DGET("LENGTH"));
	sun_length_atom = xv_get(My_server, SERVER_ATOM, 
						DGET("_SUN_LENGTH_TYPE"));
	host_atom = xv_get (My_server, SERVER_ATOM, DGET("HOST_NAME"));
	sun_host_atom = xv_get(My_server, SERVER_ATOM, 
					DGET("_SUN_FILE_HOST_NAME"));
	file_name_atom = xv_get(My_server, SERVER_ATOM, DGET("FILE_NAME"));
	atm_file_name_atom = xv_get(My_server, SERVER_ATOM, 
						DGET("_SUN_ATM_FILE_NAME"));
	delete_atom = xv_get(My_server, SERVER_ATOM, DGET("DELETE"));
	selection_end_atom = xv_get(My_server, SERVER_ATOM, 
						DGET("_SUN_SELECTION_END"));
	dragdrop_done_atom = xv_get (My_server, SERVER_ATOM, 
						DGET("_SUN_DRAGDROP_DONE"));
	data_label_atom = xv_get(My_server, SERVER_ATOM, 
						DGET("_SUN_DATA_LABEL"));
	load_atom = xv_get(My_server, SERVER_ATOM, DGET("_SUN_LOAD"));
	alternate_transport_atom = xv_get(My_server, SERVER_ATOM, 
				   DGET("_SUN_ALTERNATE_TRANSPORT_METHODS"));
	available_types_atom = xv_get(My_server, SERVER_ATOM, 
					DGET("_SUN_AVAILABLE_TYPES"));

	insert_selection_atom = xv_get(My_server, SERVER_ATOM, 
						DGET("INSERT_SELECTION"));
	enumeration_count_atom = xv_get(My_server, SERVER_ATOM, 
						DGET("_SUN_ENUMERATION_COUNT"));
	enumeration_item_atom = xv_get(My_server, SERVER_ATOM, 
						DGET("_SUN_ENUMERATION_ITEM"));
	postscript_file_atom = xv_get (My_server, SERVER_ATOM,
					   	postscript_file_string);
	text_file_atom = xv_get (My_server, SERVER_ATOM, text_file_string);
	null_atom = xv_get(My_server, SERVER_ATOM, DGET("NULL"));

	dnd_context = (DND_CONTEXT *) malloc(sizeof(DND_CONTEXT));
	memset((char *) dnd_context, NULL, sizeof(DND_CONTEXT));

}
