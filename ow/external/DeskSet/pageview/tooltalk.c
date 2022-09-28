#ifndef lint
static char *sccsid = "@(#)tooltalk.c 3.1 92/04/03";
#endif

/*
 * Copyright (c) 1990 - Sun Microsystems Inc.
 */

/*
 * tooltalk.c - tooltalk interface for pageview
 */

#include "pageview.h"
#include "ds_tooltalk.h"
#include "ds_popup.h"

#define DISPLAY_ENV	"DISPLAY="
#define LOCALE_ENV	"LOCALE="

extern char		*dnd_file;
extern int		 tooltalk_start;

Tt_message		 in_msg;
Selection_requestor	 tooltalk_sel = NULL;

char			*new_locale = NULL;
int			 new_depth = -1;
int			 new_visual = -1;

void
quit_pageview ()
{
/*
 * Sender wants us to go away.
 * First we reset the textsw so we don't get the annoying message about edits,
 * Then we unlink the dnd_file,
 * Then send a departing message to all senders,
 * Quit tooltalk gracefully,
 * Then we do an xv_destroy_safe, and we're history.
 */

    textsw_reset (edit_textsw, 0, 0);
    unlink (dnd_file);
    ds_tooltalk_send_departing_message ();
    ds_tooltalk_quit ();
    xv_destroy_safe (baseframe);
	    
    exit (0);
}

Notify_value 
tooltalk_handler (client, fd)
Notify_client client;
int fd;
{
    Tt_status		 tt_status;
    Tt_state		 msg_state;
    int			 msg_args;
    ds_tt_msg_info	 tt_message;
    char		*sel_name;
    char		*status_msg;
    char		*locale_string;
    int			 sender_xpos;
    int			 sender_ypos;
    int			 sender_width;
    int			 sender_height;
    int			 location;
    Rect		 sender_rect;
    struct timeval	 timeout;
    fd_set		 readfdmask;
    int			 select_return;
    struct rlimit 	 rlp;

/*
 * If both arguments are NULL, then we're calling this function
 * because we are getting started via tooltalk. In this case, let's
 * make sure that there is a message waiting for us. 
 */

    if ((client == (Notify_client) NULL) && (fd == NULL)) {

       timeout.tv_sec = 60L;
       timeout.tv_usec = 0L;

       getrlimit(RLIMIT_NOFILE, &rlp);
       FD_ZERO (&readfdmask);
       FD_SET (ds_tt_fd, &readfdmask);

       select_return = select (rlp.rlim_cur, &readfdmask, NULL, NULL, &timeout);

/*
 * If select_return is 0 or negative then either we timed out, or some
 * other error occurred and we can't get the message. In this case,
 * it's useless for pageview to start since we won't be able to do much
 * so just print a message on stderr and exit.
 */

       if (select_return < 1) {
	  fprintf (stderr, 
		   MGET("%s: Couldn't be started via Tooltalk Message.\n"),
		   ProgramName);
	  exit (1);
	  }
       } 


/*
 * Retrieve the tooltalk message. If we can't get it, print a message
 * and return.
 */

    if ((tt_status = tt_ptr_error (in_msg = tt_message_receive ())) != TT_OK) {
       fprintf (stderr, MGET("%s: Couldn't receive Tooltalk message.\n"), 
						ProgramName); 
       return (NOTIFY_DONE);
       }

/*
 * Determine the state of the message...
 */

    msg_state = tt_message_state (in_msg);

    if (verbose)
       fprintf (stderr, MGET("%s: tooltalk msg state: %d\n"), 
			ProgramName, msg_state);

    switch (msg_state) {

/*
 * Pageview should only get TT_SENT messages since it's currently only
 * a tooltalk message handler.
 */

      case TT_SENT:

/*
 * Get # of arguments received.
 */

	msg_args = tt_message_args_count (in_msg);

/*
 * Determine message received.
 */

	tt_message = ds_tooltalk_received_message (in_msg);

        if (verbose)
           fprintf (stderr, MGET("%s: tooltalk msg name: %s\n"), 
			    ProgramName, tt_message.ds_tt_msg_name);

	switch (tt_message.ds_tt_msg_type) {
	  
	  case DS_TT_LAUNCH_MSG:

/* 
 * We are being launched via tooltalk. But, if we didn't
 * get the correct number of arguments then exit.
 */
 
	    if (msg_args != tt_message.ds_tt_msg_args) {
	       fprintf (stderr,
			MGET("%s: Incorrect number of arguments received for Tooltalk message %s.\n"),
			ProgramName,tt_message.ds_tt_msg_name);
	       exit (1);
	       }

/*
 * Retrieve the arguments. First is the locale, next the depth, last the
 * visual.
 */

            locale_string = (char *) tt_message_arg_val (in_msg, 0);
            new_locale = (char *) malloc (strlen (locale_string) + 1);
            strcpy (new_locale, locale_string);
            tt_message_arg_ival (in_msg, 1, &new_depth);
            tt_message_arg_ival (in_msg, 2, &new_visual);

/*
 * Reply to the LAUNCH message...
 */
	 
	    tt_message_reply (in_msg);
	    break;

	  case DS_TT_DISPATCH_DATA_MSG:

/*
 * First, check if the number of arguments received is correct. If
 * not, return.
 */

	    if (msg_args != tt_message.ds_tt_msg_args) {
	       fprintf (stderr,
			MGET("%s: Incorrect number of arguments received for Tooltalk message %s.\n"),
			ProgramName,tt_message.ds_tt_msg_name);
	       return (NOTIFY_DONE);
	       }

/*
 * If we haven't created a selection requestor yet, do it now.
 */

	    if (tooltalk_sel == (Selection_requestor) NULL)
	       tooltalk_sel = xv_create (baseframe, SELECTION_REQUESTOR, NULL);


/*
 * Retrieve the argument - which is the SEL_RANK_NAME for the selection
 * that will be used to send the data from the sender
 */

	    sel_name = (char *) tt_message_arg_val (in_msg, 0);

	    xv_set (tooltalk_sel, SEL_RANK_NAME, sel_name, NULL);

/*
 * Get the data from the sender.
 */

	    
	    Current_Sel = tooltalk_sel;
	    Current_Op = TOOLTALK_OP;
	
/*
 * We use the 'load_from_dragdrop' function to get the data - even
 * though this isn't a drag and drop operation, it does what we want
 * so what the heck.
 */

	    load_from_dragdrop ();
	
	    break;

	  case DS_TT_STATUS_MSG:
 
/*
 * First, check if the number of arguments received is correct. If
 * not, return.
 */

	    if (msg_args != tt_message.ds_tt_msg_args) {
	       fprintf (stderr,
			MGET("%s: Incorrect number of arguments received for Tooltalk message %s.\n"),
			ProgramName,tt_message.ds_tt_msg_name);
	       return (NOTIFY_DONE);
	       }

/*
 * Get the status message and display it in the left footer. 
 */

	    status_msg = (char *) tt_message_arg_val (in_msg, 0);

	    xv_set (baseframe, FRAME_LEFT_FOOTER, status_msg, NULL);

	    break;

	  case DS_TT_MOVE_MSG:
 
/*
 * First, check if the number of arguments received is correct. If
 * not, return.
 */

	    if (msg_args != tt_message.ds_tt_msg_args) {
	       fprintf (stderr,
			MGET("%s: Incorrect number of arguments received for Tooltalk message %s.\n"),
			ProgramName,tt_message.ds_tt_msg_name);
	       return (NOTIFY_DONE);
	       }

/*
 * Get arguments: x, y, width, height of senders baseframe and
 * location of where the handlers (us) baseframe should go relative
 * to the senders
 */

	    tt_message_arg_ival (in_msg, 0, &sender_xpos);
	    tt_message_arg_ival (in_msg, 1, &sender_ypos);
	    tt_message_arg_ival (in_msg, 2, &sender_width);
	    tt_message_arg_ival (in_msg, 3, &sender_height);
	    tt_message_arg_ival (in_msg, 4, &location);

	    sender_rect.r_left = sender_xpos;
	    sender_rect.r_top = sender_ypos;
	    sender_rect.r_width = sender_width;
	    sender_rect.r_height = sender_height;

	    xv_set (baseframe, XV_SHOW, FALSE, NULL);

/*
 * Use ds_position_popup_rect to position our baseframe.
 */

	    ds_position_popup_rect (&sender_rect, baseframe, location);

/*
 * Now make the window appear again..
 */

	    xv_set (baseframe, XV_SHOW, TRUE, NULL);

/*
 * If the window is now being displayed, then we can assume that
 * the tooltalk init worked a-ok. So, reset tooltalk_start back to
 * false, and let pageview work as it should.
 */

	    tooltalk_start = FALSE;

	    break;

	  case DS_TT_QUIT_MSG:

	    quit_pageview ();

	  case DS_TT_HIDE_MSG:

/*
 * Sender wants us to hide...
 * Set baseframe's XV_SHOW to false.
 */

	    xv_set (baseframe, XV_SHOW, FALSE, NULL);
	    break;

	  case DS_TT_EXPOSE_MSG:

/*
 * Sender wants us to reappear...
 * Set baseframe's XV_SHOW to true.
 */

	    xv_set (baseframe, XV_SHOW, TRUE, NULL);

/*
 * The sending app may have not sent us a MOVE message, so let's
 * set tooltalk_start to false since we are now up and running.
 */

	    tooltalk_start = FALSE;

	    break;

/*
 * DS_TT_RETRIEVE_DATA_MSG - Sender wants data back. Not applicable
 *   for pageview.
 * DS_TT_DEPARTING_MSG - A handler is departing - since pageview isn't
 *   a tooltalk sender, we shouldn't receive this message either.
 * DS_TT_NO_STD_MSG - We don't have any specific messages yet, so we
 *   shouldn't receive this either.
 */

	  case DS_TT_RETRIEVE_DATA_MSG:
	  case DS_TT_DEPARTING_MSG:
	  case DS_TT_NO_STD_MSG:

	    if (verbose)
	       fprintf (stderr, 
			MGET("%s doesn't handle this tooltalk message %s.\n"),
		 	ProgramName, tt_message.ds_tt_msg_name); 
	    break;

	  }

        break; 

      case TT_STARTED:
/*
 * Get # of arguments received.
 */
 
        msg_args = tt_message_args_count (in_msg);
 
fprintf (stderr, "tooltalk_handler:  got #args: %d\n", msg_args);
/*
 * Determine message received.
 */
 
        tt_message = ds_tooltalk_received_message (in_msg);
 
        if (verbose)
           fprintf (stderr, MGET("%s: tooltalk msg name: %s\n"),
                            ProgramName, tt_message.ds_tt_msg_name);
 
fprintf (stderr, "tooltalk_handler: msg_type: %d\n", tt_message.ds_tt_msg_type);        switch (tt_message.ds_tt_msg_type) {
          
          case DS_TT_LAUNCH_MSG:
 
/*
 * We are being launched via tooltalk. But, if we didn't
 * get the correct number of arguments then exit.
 */

            if (msg_args != tt_message.ds_tt_msg_args) {
               fprintf (stderr,
                        MGET("%s: Incorrect number of arguments received for Tooltalk message %s.\n"),
                        ProgramName, tt_message.ds_tt_msg_name);
               exit (1);
               }
 
/*
 * Retrieve the arguments. First is the DISPLAY, second is the LOCALE.
 */
 
            locale_string = (char *) tt_message_arg_val (in_msg, 0);
            new_locale = (char *) malloc (strlen (locale_string) + 1);
            strcpy (new_locale, locale_string);
            tt_message_arg_ival (in_msg, 1, &new_depth);
            tt_message_arg_ival (in_msg, 2, &new_visual);
 
/*
 * Reply to the LAUNCH message...
 */

            tt_message_reply (in_msg);
            break;
          }
 
        break;
 
      case TT_HANDLED:
      case TT_REJECTED:
      case TT_FAILED:
      case TT_CREATED:
      case TT_QUEUED:

	if (verbose)
	   fprintf (stderr, MGET("%s got unwanted tooltalk message state %d\n"),
				  ProgramName, msg_state);

      }

      tt_message_destroy (in_msg);
  
      return (NOTIFY_DONE);

}
