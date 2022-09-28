#ifndef lint
static char sccsid[] = "@(#)dragdrop.c 1.20 93/06/16";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */


#include <dirent.h>
#include <fcntl.h>
#include <sys/param.h> /* MAXPATHLEN (include types.h if removed) */
#include <sys/stat.h>
#include "display.h"
#include "image.h"
#include "imagetool.h"
#include "ui_imagetool.h"

#include <xview/dragdrop.h>
#include <xview/notice.h>

#define MAXALLOC		10000
#define	XFER_BUFSIZE		65535
#define COUNT_RESPONSE          0x1
#define HOST_RESPONSE           0x2
#define TRANSPORT_RESPONSE      0x4
#define FILE_NAME_RESPONSE      0x8
#define DATA_LABEL_RESPONSE     0x10
#define TYPES_RESPONSE          0x20
#define TARGET_RESPONSE         0x40
#define BYTES_REQUESTED         0x80

#define RESPONSE_LEVEL_1        0x7f

typedef struct {
        Atom            *target_list;
        int              num_targets;
        Atom            *transport_list;
        int              num_transports;
        Atom            *types_list;
        int              num_types;
        char            *data_label;
        int              num_objects;
        char            *source_host;
        char            *source_filename;
        Atom             chosen_target;
        Atom             chosen_transport;
        int              processing_stream;
        int              state_mask;
        int              stop_hit;
} DND_CONTEXT;

DND_CONTEXT	*dnd_context;

FILE		*dnd_file = NULL;

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
Atom 		 null_atom;

Atom		*types_atom_list;
int		 num_type_atoms;

char 		 xfer_data_buf [XFER_BUFSIZE + 1];
char 	 	 current_filename [MAXNAMLEN];

char		*text_file_string = DGET ("_SUN_TYPE_text");
char		*compress_file_string = DGET ("_SUN_TYPE_compress");
char		*sun_type_string = DGET ("_SUN_TYPE_");

Selection_requestor	current_sel;
Dnd			current_dnd_object;

int			transfer_attempts;

void			stop_dragdrop ();
void			ask_for_data ();

/*
 * Check to see if a specific atom was returned in one of the
 * various atom lists from the onwer of the selection.
 */

int
atom_in_list (atom, atom_list, length)
Atom	 atom;
Atom	*atom_list;
int	 length;
{
    int		i;

    if (!atom_list)
       return(False);

    for (i = 0; i < length; i++)
	if (atom_list[i] == atom)
	   return (True);

    return(False);
}

Atom
check_types ()
{
    int 	i;

    if (dnd_context->num_types > 0) 
       for (i = 0; i < num_type_atoms; i++)
	   if (atom_in_list (types_atom_list [i], dnd_context->types_list, 
					dnd_context->num_types) == True)
	      return (types_atom_list [i]);
 
    return ((Atom) NULL);
}

int
target_in_list (atom)
Atom	atom;
{
    int 	i;

    for (i = 0; i < num_type_atoms; i++)
	if (atom == types_atom_list [i])
	   return (True);

    return (False);
}

int
type_from_target (target)
{
    char 	*atom_name = XGetAtomName (current_display->xdisplay, target);
    char	*type_name = strtok (atom_name, sun_type_string);
    int		 i;

    if (type_name == (char *) NULL)
       return (-1);

    for (i = 0; i < ntypes; i++)
        if (strcmp (type_name, all_types[i].type_name) == 0) 
           return (i);

    return (-1);
}


/* 
 * Clears the context block in preparation for 
 * a new drag/drop transaction or tooltalk data transfer.
 */

void
clear_context (dnd_ptr)
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
    dnd_ptr->processing_stream = 0;
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
imagetool_reply_proc ( sel_req, target, type, replyBuf, len, format )
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
    static Atom	 tmp_atom = NULL;
    int	 	 old_length;
    Event	 event;
    FILE	*tmp_file;
    int	 	 bytes_written;
    int		 file_result;
    int		 file_type;

/* 
 * Try to turn the type and target atoms into some useful text 
 * for debugging. 
 */

    if (prog->verbose) {
       if (type != NULL)
	  atom_name = XGetAtomName(current_display->xdisplay, type);
       else
	  atom_name = LGET("[None]");
	  if (target > 0)
	     target_name = XGetAtomName(current_display->xdisplay, target);
	  else
	     target_name = LGET("[None]");

       fprintf (stderr, MGET ("entered reply proc, type name  = %s, type atom = %d\n target_name = %s, target atom = %d\n len = %d, format = %d, buf = %d\nstate = %d\n"),
		atom_name, type, target_name, target, len, format, err_ptr, 
		dnd_context->state_mask);
       }

/* 
 * If get selection_end_atom, just return.
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
       else if (target_in_list (target) == True) {

/*
 * We got an error while transferring the data. At this point,
 * let's try and start over up to 3 times. 
 */

	  transfer_attempts++;
	  if (transfer_attempts < 3) {

/*
 * Let's try and start over...
 */

	     fclose (dnd_file);
	     dnd_file = (FILE *) NULL;
	     ask_for_data (tmp_atom, sel_req);

	     }

	  else {

/*
 * We've tried 3 times to get the data, let's forget it.
 */

	     fclose (dnd_file);
	     dnd_file = (FILE *) NULL;

	     if (prog->verbose)
		fprintf (stderr, MGET("ReplyProc: Bad Conversion!\n"));
	     display_error (base_window->base_window,
			    EGET("Drag & Drop failed."));

/* 
 * To complete the drag and drop or tooltalk operation,
 * we tell the source that we are all done.  
 */
 
	     stop_dragdrop (sel_req, FALSE);

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
	       if (prog->verbose)
		  fprintf (stderr, MGET ("ReplyProc: Bad property!\n"));
	       display_error (base_window->base_window,  
			      EGET ("Data Transfer: Bad property!"));
               break;
      	  case SEL_BAD_TIME:
	       if (prog->verbose)
		  fprintf (stderr, MGET ("ReplyProc: Bad time!\n"));
               display_error (base_window->base_window,
			      EGET ("Data Transfer: Bad time!"));
               break;
      	  case SEL_BAD_WIN_ID:
	       if (prog->verbose)
		  fprintf (stderr, MGET ("ReplyProc: Bad window id!\n"));
               display_error (base_window->base_window,
			      EGET ("Data Transfer: Bad window id!"));
               break;
      	  case SEL_TIMEDOUT:
	       if (prog->verbose)
		  fprintf (stderr, MGET ("ReplyProc: Timed out!\n"));
               display_error (base_window->base_window,
			      EGET("Data Transfer: Timed out!"));
               break;
      	  case SEL_PROPERTY_DELETED:
	       if (prog->verbose)
		  fprintf (stderr, MGET ("ReplyProc: Property deleted!\n"));
               display_error (base_window->base_window,
			      EGET ("Data Transfer: Property deleted!"));
               break;
      	  case SEL_BAD_PROPERTY_EVENT:
	       if (prog->verbose)
		  fprintf (stderr, MGET ("ReplyProc: Bad property event!\n"));
               display_error (base_window->base_window,
			      EGET ("Data Transfer: Bad property event!"));
               break;	  }

       stop_dragdrop (sel_req, FALSE);

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

    else  if (target_in_list (target) == True) {

       if (len) {

/* 
 * The length is non-zero, so we got data, and not the end of transmission 
 * If dnd_file is NULL, then we haven't opened the tmp file for writing.
 */

	  if (dnd_file == (FILE *) NULL) {
             if (dnd_context->source_filename != (char *) NULL)
                strcpy (current_filename,
                        basename (dnd_context->source_filename));
             else if (dnd_context->data_label != (char *) NULL)
                strcpy (current_filename, basename (dnd_context->data_label));
             else
                strcpy (current_filename, LGET ("Untitled"));

	     make_tmpfile (&(prog->datafile), current_filename);
	     dnd_file = fopen (prog->datafile, "w");
	     if (dnd_file == (FILE *) NULL) {
		stop_dragdrop (sel_req, FALSE);
    		display_error (base_window->base_window,  
			       EGET ("Drag & Drop: Could not open temporary file."));
		return;
		}
   	     }	

	  if (fwrite (char_buf, sizeof (char), len, dnd_file) != len) {

/* 
 * Writing out file failed.
 */

    	     display_error (base_window->base_window, 
			    EGET ("Drag & Drop: Could not write to temporary file."));

	     fclose (dnd_file);
	     dnd_file = (FILE *) NULL;
  	     stop_dragdrop (sel_req, FALSE);

	     return;
	     }


	  if (!dnd_context->processing_stream) {
	     fclose (dnd_file);
	     dnd_file = (FILE *) NULL;
	
	     file_type = type_from_target (target);

	     file_result = open_newfile (NULL, current_filename, prog->datafile,
					 file_type, (char *) NULL, 0);

	     if (file_result == 0) 
		xv_set (base_window->base_window, FRAME_CLOSED, FALSE, NULL);

/* 
 * To complete the drag and drop or tooltalk operation, we tell the 
 * source that we are all done.  
 */
 
	     stop_dragdrop (sel_req, TRUE);
	     return;
	     }
	  }

/*
 * The length was zero, so we have the end of the data.
 * At this point, we can write out the data to the temporary file.
 */

       else if (dnd_context->processing_stream) {
	  fclose (dnd_file);
	  dnd_file = (FILE *) NULL;

	  file_type = type_from_target (target);

	  file_result = open_newfile (NULL, current_filename, prog->datafile,
				      file_type,  (char *) NULL, 0);

	  if (file_result == 0) 
	     xv_set (base_window->base_window, FRAME_CLOSED, FALSE, NULL);

/* 
 * To complete the drag and drop or tooltalk operation, we tell the 
 * source that we are all done.  
 */
 
	  stop_dragdrop (sel_req, TRUE);
	  return;
	  }

/*
 * The length was zero, and we weren't processing the data,
 * so we didn't get anything, so let's just end this dnd operation,
 * and tell the user we got an empty file.
 */

       else {
 
	  display_error (base_window->base_window, EGET ("Drag & Drop: An empty file has been dropped."));
	  stop_dragdrop (sel_req, FALSE);
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
       if (prog->verbose)
	  fprintf (stderr, MGET ("first batch of replies processed, asking for second\n"));

/*
 * The owner wants us to process more than one object. We aren't set up
 * to do that, so let's tell him to forget it!
 */

       if (dnd_context->num_objects > 1) {
	 display_error (base_window->base_window, 
			EGET ("Drag & Drop failed.  Drag one file at a time."));
         stop_dragdrop (sel_req, FALSE);
         return;
       }
	
/*
 * Let's check the types available from the owner. If it's one we can
 * handle, let's proceed, otherwise, tell him we can't deal with it
 * and forget about it.
 */

       if ((tmp_atom = check_types ()) == (Atom) NULL) {
	  display_error (base_window->base_window, 
			 EGET ("Drag & Drop halted.  Image Tool cannot read file format."));
          stop_dragdrop (sel_req, FALSE);
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
	   (dnd_context->source_host != NULL) &&
	   (strcmp(prog->hostname, dnd_context->source_host) == 0)) {

/* 
 * The file is on the local machine. Let's try and get it.
 */
	
	  if (dnd_context->source_filename != (char *) NULL)
	     strcpy (current_filename, dnd_context->source_filename);
	  else
	     strcpy (current_filename, dnd_context->data_label);

/*
 * If we got the file type from the dnd op, use it.
 */

 	  file_type = -1;
	  if (tmp_atom != (Atom) NULL)
	     file_type = type_from_target (tmp_atom);

	  file_result = open_newfile (NULL, current_filename, current_filename,
				      file_type, (char *) NULL, 0);
	  if (file_result == -1) {
             stop_dragdrop (sel_req, FALSE);
	     return;
	     }

	  else if (file_result == 0) {
	     /*set_labels (current_filename);*/
	     xv_set (base_window->base_window, 
					FRAME_CLOSED, FALSE, NULL);
	     }

 
/* 
 * To complete the drag and drop or tooltalk operation, we tell the 
 * source that we are all done.  
 */
		
	  stop_dragdrop (sel_req, TRUE);

	  return;
	  }

/* 
 * Let's ask the owner for the data instead...
 */
	
       ask_for_data (tmp_atom, sel_req);

       xv_set(base_window->base_window, FRAME_CLOSED, FALSE, 0);

       }

}

/*
 * If we can't get at the file, then we have to ask the owner
 * of the selection to pass the data. Here we decide which type
 * we will ask for depending on the types the owner supports.
 */

void
ask_for_data (tmp_atom, sel_req)
Atom		    tmp_atom;
Selection_requestor sel_req;
{
    
    xv_set(sel_req, SEL_REPLY_PROC, imagetool_reply_proc,
		    SEL_TYPES, tmp_atom, NULL,
		    NULL );
	
    dnd_context->state_mask |= BYTES_REQUESTED;

    sel_post_req (sel_req);

}

/* 
 * Stop the drag and drop or tooltalk operation.  Converts 
 * _SUN_SELECTION_END or _SUN_DRAGDROP_DONE on the current selection, 
 * signaling the end, and then puts the rest of the application back to 
 * normal.  
 */

void
stop_dragdrop (sel_req, success)
Selection_requestor sel_req;
int		    success;
{
    if (prog->verbose)
       fprintf (stderr, MGET ("stop_dragdrop called\n"));

/* 
 * Signal termination of transaction.
 */

    xv_set(sel_req, SEL_REPLY_PROC, imagetool_reply_proc,
		    SEL_TYPES, selection_end_atom, 0,
		    NULL );

    sel_post_req (sel_req);

/*
 * If we got the data from tooltalk, check the success
 * to determine what to do now.
 */

    if (prog->tt_load == TRUE) {
       prog->tt_load = FALSE;
       respond_to_tt (success);
       }

/*
 * If we didn't get the data from tooltalk, but were started from
 * tooltalk, and success == TRUE, then break the connection now.
 */

    else if ((prog->tt_started == TRUE) && (success == TRUE))
       break_tt ();

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

    if (prog->verbose) 
       fprintf (stderr, MGET ("load_from_dragdrop: called\n"));

/* 
 * Set the number of transfer attempts to zero.
 */
	
    transfer_attempts = 0;

/* 
 * Clear the left footer for new response status.
 */

    xv_set (base_window->base_window, FRAME_LEFT_FOOTER, DGET (""), 0);

/* 
 * Get target types, and see if we lika any of them.
 */

    clear_context (dnd_context);

    xv_set (current_sel, SEL_REPLY_PROC, imagetool_reply_proc,
			 SEL_TYPES, enumeration_count_atom,    
				    sun_host_atom,
				    alternate_transport_atom, 
				    file_name_atom,
				    data_label_atom,    
				    available_types_atom,
				    targets_atom,
				    NULL,
			 NULL );

    sel_post_req (current_sel); 


    if (prog->verbose) 
       fprintf (stderr, MGET ("load_from_dragdrop: after sel_post_req\n"));

}


/* 
 * The convert proc is called whenever someone makes a request to the dnd
 * selection.  
 */


int
ImagetoolConvertProc(seln, type, data, length, format)
Selection_owner	 seln;
Atom		*type;
Xv_opaque	*data;
unsigned long	*length;
int		*format;
{
    static int		 current_point;
    static int		 count = 1;
    static int		 item = 0;
    Xv_Server 		 server = XV_SERVER_FROM_WINDOW(xv_get(seln, XV_OWNER));
    char		*atom_name;
    char		*data_ptr;
    char		 atm_file [MAXPATHLEN+MAXNAMLEN];
    static Atom		 target_list[19];
    static Atom		 types_list[2];
    static Atom		 transport_atoms [1];
    static int		 transfer_in_progress = FALSE;
    static FILE		*dnd_fp;
    int			 i;


/* 
 * Try to turn the type and target atoms into some useful text for 
 * debugging. 
 */

    if (prog->verbose) {
       fprintf (stderr, MGET ("ImagetoolConvertProc conversion called\n"));

       if (type != NULL)
   	  atom_name = XGetAtomName (current_display->xdisplay, *type);
       else
	  atom_name = LGET ("[None]");

       fprintf (stderr, 
		MGET ("ImagetoolConvertProc, being asked to convert %s\n"), 
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

    if ((*type == selection_end_atom) || (*type == dragdrop_done_atom)) {

/* 
 * Reset 'transfer_in_progres' and current_point just in  case we screwed 
 * up somewhere. 
 */

       transfer_in_progress = FALSE;
       current_point = 0;

/* 
 * Yield ownership of the selection.
 */

       xv_set (current_dnd_object, SEL_OWN, False, NULL);

       xv_set (base_window->base_window, FRAME_LEFT_FOOTER, 
				EGET("Drag & Drop: Completed"), NULL);
       *format = 32;
       *length = 0;
       *data = NULL;
       *type = null_atom;
       return (True);
       } 

/* 
 * In our case, we chose not to listen to delete 
 * commands on the drag/drop item.  We never mean 
 * to do a move, always a copy. 
 */

    else if (*type == delete_atom) {
       *format = 32;
       *length = 0;
       *data = NULL;
       *type = null_atom;
       return (True);
       } 

/*
 * Destination wants the length of the data. Send the length of the file.
 */
 
    else if ((*type == length_atom) || (*type == sun_length_atom)) {
       *format = 32;
       *length = 1;
       *type = XA_INTEGER;
       *data = (Xv_opaque) current_image->file_size;
       return (True);
       } 

/*
 * Destination wants the transport mechanisms we support. For now, it's
 * just the _SUN_ATM_FILE_NAME.
 */

    else if (*type == alternate_transport_atom) {
       *format = 32;
       *type = XA_ATOM;
       if ((current_image == (ImageInfo *) NULL) ||
	   (current_image->data != (char *) NULL)) {
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
 
    else if (*type == atm_file_name_atom) {

/*
 * If we don't have a file name, then just send a NULL
 */

       *format = 8;
       *type = XA_STRING;
       if ((current_image == (ImageInfo *) NULL) ||
	   (current_image->data != (char *) NULL)) {
	  *length = 0;
	  *data = (Xv_opaque) NULL;
	  return (False);
	  }

       sprintf (atm_file, "%s:%s", prog->hostname, current_image->realfile);
       *length = strlen (atm_file);
       *data = (Xv_opaque) atm_file;
       return (True);
       }

/*
 * Desination wants to know the number of items it will get.
 */

    else if (*type == enumeration_count_atom) {
       *format = 32;
       *length = 1;
       *type = XA_INTEGER;
       *data = (Xv_opaque) &count;
       return (True);
       } 

/* 
 * Desination wants to know which item is the current one.
 */

    else if (*type == enumeration_item_atom) {
       *format = 32;
       *length = 0;
       *type = null_atom;
       *data = (Xv_opaque) &item;
       return (True);
       }

/*
 * Destination wants to know the hostname of the owner.
 */

    else if ((*type == sun_host_atom) || (*type == host_atom)) {
       *format = 8;
       *length = strlen (prog->hostname);
       *data = (Xv_opaque) prog->hostname;
       *type = XA_STRING;
       return (True);
       } 

/*
 * Destination wants to know the file name (entire pathname).
 */

    else if (*type == file_name_atom) {

/* 
 * Determine if we have a file name, if not, send NULL.
 * Also, if title is Untitiled, then send NULL.
 */

       *format = 8;
       *type = XA_STRING;
       if ((current_image == (ImageInfo *) NULL) ||
	   (strcmp (current_image->file, LGET ("Untitled")) == 0) ||
	   (current_image->data != (char *) NULL)) {
	  *length = 0;
	  *data = (Xv_opaque) NULL;
	  return (False);
	  }

       *length = strlen (current_image->realfile);
       *data = (Xv_opaque) current_image->realfile;
       return (True);
       }

/* 
 * Destination wants to know the data label of the file... in this
 * case, we just give them the file name, not the complete path.
 */

    else if (*type == data_label_atom) {

/* 
 * Determine if we have a file name, if not, return False.
 */

       *format = 8;
       *type = XA_STRING;
       if ((current_image == (ImageInfo *) NULL) ||
	   (strcmp (current_image->file, LGET ("Untitled")) == 0)) {
	  *length = 0;
	  *data = (Xv_opaque) NULL;
	  return (False);
	  }

       *length = strlen (basename (current_image->file));
       *data = (Xv_opaque) basename (current_image->file);
       return (True);
       }

/*
 * Destination wants to know our applications name...
 */

    else if (*type == name_atom) {
       *format = 8;
       *length = strlen (prog->name);
       *data = (Xv_opaque) prog->name;
       *type = XA_STRING;
       return (True);
       }

/*
 * Destination wants to know all of the target atoms we support.
 */

    else if (*type == targets_atom) {
       *format = 32;
       *length = 19;
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
       target_list[16] = name_atom;
       target_list[17] = text_atom;
       for (i = 0; i < ntypes; i++)
	   if (current_image->type_info == &all_types [i])
	      target_list [18] = types_atom_list [i];
	       
       *data = (Xv_opaque) target_list;
       return (True);
       } 

/*
 * Destination wants to know all of the data types we support.
 */

    else if (*type == available_types_atom) {
       *format = 32;
       *length = 2;
       *type = XA_ATOM;
       for (i = 0; i < ntypes; i++)
	   if (current_image->type_info == &all_types [i])
	      types_list [0] = types_atom_list [i];

       types_list[1] = XA_STRING;
       *data = (Xv_opaque) types_list;
       return (True);
       } 

/*
 * Destination wants the data!
 */

    else if (target_in_list (*type) == True) {
       int bytes_2_copy;

/*
 * If the length is zero for some reason, forget it.
 */
 
       if (current_image->file_size == 0) {
          *format = 8;
          *type = XA_STRING;
	  *length = 0;
          *data = (Xv_opaque) NULL;
	  return (False);
	  }
	  
       *length = current_image->file_size;

/* 
 * If the number of bytes will fit into one buffer, then we just
 * ship the whole thing at once.  If it is greater that the
 * size of a buffer, we need to go into sending INCR messages. 
 */

       if (transfer_in_progress == FALSE) {

/* 
 * Open the file if necessary.
 */

	  if (current_image->data == (char *) NULL) {
	     if ((dnd_fp = fopen (current_image->realfile, "r")) == 
							(FILE *) NULL) {
	        fprintf (stderr, MGET ("couldn't open file for dnd\n"));
	        *format = 8;
	   	*type = XA_STRING;
	   	*length = 0;
	   	*data = (Xv_opaque) NULL;
	   	return (False);
		}
	     }
/*
 * Determine if we can ship it all, or have to do it in chunks.
 */

	  if (*length > XFER_BUFSIZE) {

/*
 * If we have the file, then make sure we can read from it before
 * we continue.
 */

	     current_point = 0;
	     *type = incr_atom;
	     *length = 1;
	     *format = 32;
	     *data = (Xv_opaque) current_image->file_size;
	     transfer_in_progress = TRUE;
	     return (True);
	     }

	  else {
/*
 * Can send all the data now.
 */

	     if (current_image->data != (char *) NULL)
	        memcpy (xfer_data_buf, current_image->data, *length);
	     else 
	        fread (xfer_data_buf, 1, *length, dnd_fp);

	     *format = 8;
	     *data = (Xv_opaque) xfer_data_buf;
	     *type = XA_STRING;
	     xfer_data_buf [*length] = NULL;
	     return (True);
	     }
	  }

/*
 * Destination wants more data.
 */

       else {
	  bytes_2_copy = Min (Min (XFER_BUFSIZE, *length), 
				     *length - current_point);
	  
	  *length = bytes_2_copy;
   	  if (*length > 0) {
	     if (current_image->data != (char *) NULL) {
	 	data_ptr = current_image->data + current_point;
	        memcpy (xfer_data_buf, data_ptr, bytes_2_copy);
		}
	     else 
	        fread (xfer_data_buf, 1, bytes_2_copy, dnd_fp);
	     }
	  else {
	     transfer_in_progress = FALSE;
	     if (current_image->data == (char *) NULL)
	        fclose (dnd_fp);
	     }
	  *format = 8;
	  *type = XA_STRING;
	  *data = (Xv_opaque) xfer_data_buf;
	  xfer_data_buf [*length] = NULL;

	  current_point += bytes_2_copy;
	  return (True);
	  }
       } 

/* 
 * Let the default convert procedure deal with the request.
 */

    else 
	return (sel_convert_proc (seln, type, data, length, format));
    
}


/* 
 * Initialize atoms needed for dnd.
 */

void
dragdrop_init (server)
Xv_opaque	server;
{
    int		 i;
    char	 tmp_name [256];

    incr_atom = xv_get (server, SERVER_ATOM, DGET("INCR"));
    name_atom = xv_get (server, SERVER_ATOM, DGET("NAME"));
    targets_atom = xv_get (server, SERVER_ATOM, DGET("TARGETS"));
    length_atom = xv_get (server, SERVER_ATOM, DGET("LENGTH"));
    sun_length_atom = xv_get (server, SERVER_ATOM, DGET("_SUN_LENGTH_TYPE"));
    host_atom = xv_get (server, SERVER_ATOM, DGET("HOST_NAME"));
    sun_host_atom = xv_get (server, SERVER_ATOM, DGET("_SUN_FILE_HOST_NAME"));
    file_name_atom = xv_get (server, SERVER_ATOM, DGET("FILE_NAME"));
    atm_file_name_atom = xv_get (server, SERVER_ATOM, 
				 DGET("_SUN_ATM_FILE_NAME"));
    delete_atom = xv_get (server, SERVER_ATOM, DGET("DELETE"));
    selection_end_atom = xv_get (server, SERVER_ATOM, 
				 DGET("_SUN_SELECTION_END"));
    dragdrop_done_atom = xv_get (server, SERVER_ATOM, 
				 DGET("_SUN_DRAGDROP_DONE"));
    data_label_atom = xv_get (server, SERVER_ATOM, 
			      DGET("_SUN_DATA_LABEL"));
    load_atom = xv_get (server, SERVER_ATOM, DGET("_SUN_LOAD"));
    alternate_transport_atom = xv_get (server, SERVER_ATOM, 
			            DGET("_SUN_ALTERNATE_TRANSPORT_METHODS"));
    available_types_atom = xv_get (server, SERVER_ATOM, 
				  DGET("_SUN_AVAILABLE_TYPES"));
    insert_selection_atom = xv_get (server, SERVER_ATOM, 
				    DGET("INSERT_SELECTION"));
    enumeration_count_atom = xv_get (server, SERVER_ATOM, 						 	     DGET("_SUN_ENUMERATION_COUNT"));
    enumeration_item_atom = xv_get(server, SERVER_ATOM, 
				   DGET("_SUN_ENUMERATION_ITEM"));
    null_atom = xv_get(server, SERVER_ATOM, DGET("NULL"));

    types_atom_list = (Atom *) malloc ((ntypes + 6)* sizeof (Atom));

    for (i = 0; i < ntypes; i++) {
	sprintf (tmp_name, "%s%s", sun_type_string, all_types[i].type_name);
	types_atom_list [i] = xv_get (server, SERVER_ATOM, tmp_name);
        }
      
    types_atom_list [ntypes] = xv_get (server, SERVER_ATOM, text_file_string);
    num_type_atoms = ntypes + 1;

    types_atom_list [num_type_atoms] = xv_get (server, SERVER_ATOM, 
					       DGET ("TEXT"));
    text_atom = types_atom_list [num_type_atoms];
    num_type_atoms++;

/*
 * Add compressed files to our list that we'll accept.
 */

    types_atom_list [num_type_atoms] = xv_get (server, SERVER_ATOM, 
						compress_file_string);
    num_type_atoms++;

    types_atom_list [num_type_atoms] = XA_STRING;
    num_type_atoms++;

/*
 * Also add _SUN_TYPE_default and _SUN_TYPE_default-doc
 */

    sprintf (tmp_name, "%s%s", sun_type_string, DGET ("default"));
    types_atom_list [num_type_atoms] = xv_get (server, SERVER_ATOM, tmp_name);
    num_type_atoms++;

    sprintf (tmp_name, "%s%s", sun_type_string, DGET ("default-doc"));
    types_atom_list [num_type_atoms] = xv_get (server, SERVER_ATOM, tmp_name);
    num_type_atoms++;

    dnd_context = (DND_CONTEXT *) malloc (sizeof(DND_CONTEXT));
    memset ((char *) dnd_context, NULL, sizeof(DND_CONTEXT));

}
