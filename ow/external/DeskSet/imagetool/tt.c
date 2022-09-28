
#ifndef lint
static char sccsid[] = "@(#)tt.c 1.17 93/02/02";
#endif

/*
 * Copyright (c) 1992, 1993 by Sun Microsystems, Inc.
 */

/*
 * tt.c - Functions used for tooltalk interface.
 */

#include <locale.h>
#include <sys/param.h>
#include "display.h"
#include "image.h"
#include "imagetool.h"
#include "ui_imagetool.h"
#include "dstt.h"
#include <xview/dragdrop.h>

extern Selection_requestor	 current_sel;
extern Dnd                       current_dnd_object;

struct itimerval		 timeout;
Tt_message			 tt_msg = NULL;  /* Message we receive 	  */

Notify_value
quit_after_sleep ()
{
    prog->tt_started = FALSE;
    xv_destroy_safe (base_window->base_window);
}

/*
 * Reply to last message so don't fail.
 */

void
last_tt_reply ()
{
    if (tt_msg != NULL) {
       dstt_set_status (tt_msg, dstt_status_user_request_cancel);
       tt_message_reply (tt_msg);
       tt_message_destroy (tt_msg);
       tt_msg = NULL;
       }
}

void
set_tt_timer ()
{
    timeout.it_value.tv_sec = 180;
    timeout.it_value.tv_usec = 0;
    timeout.it_interval.tv_sec = 0;
    timeout.it_interval.tv_usec = 0;

    notify_set_itimer_func (base_window->base_window, quit_after_sleep,
			    ITIMER_REAL, &timeout, NULL);

    if (prog->tt_sender != (char *) NULL) {
       last_tt_reply ();
       free (prog->tt_sender);
       prog->tt_sender = (char *) NULL;
       } 

   prog->tt_timer_set = TRUE;
}

void
turnoff_tt_timer ()
{
    notify_set_itimer_func (base_window->base_window, NOTIFY_FUNC_NULL,
                            ITIMER_REAL, NULL, NULL);
}

status_t
load_from_tt (ttmsg, media_type, type, tt_contents, size, messageid, title)
Tt_message 	 ttmsg;
char		*media_type;
Data_t		 type;
void 		*tt_contents;
int 		 size;
char		*messageid;
char		*title;
{
    int		 		 file_type = -1;
    int	 	 		 status;
    char			*sender;
    int		 		 i;
    int				 write_status;
    Selection_requestor		 tt_sel = NULL;
    static int			 attempt = 0;
    char			 tmpfilename [MAXPATHLEN];

/*
 * If we weren't started via tooltalk, reject the message since the
 * user may not want a new image blowing away the one he's currently
 * viewing.
 */

    if (prog->tt_started == FALSE)
       return (REJECT);

    if (prog->tt_sender == (char *) NULL) 
       prog->tt_sender = strdup (tt_message_sender (ttmsg));
    else if (strcmp (prog->tt_sender, tt_message_sender (ttmsg)) != 0)
       return (REJECT);

    for (i = 0; i < ntypes; i++)
	if ((all_types[i].media_type != (char *) NULL) &&
            (strcmp (media_type, all_types[i].media_type) == 0)) {
           file_type = i;
	   break;
	   }

    if (file_type == -1)
       return (FAIL);

    turnoff_tt_timer ();

    if (prog->verbose == TRUE)
       fprintf (stderr, "%s started with tooltalk, type is: %d\n",
			prog->name, type);

    if (type == path) 
       status = open_newfile (NULL, tt_contents, tt_contents, file_type, 
			      (char *) NULL, 0);
    else if (type == contents) {
       if (title == (char *) NULL) {
	  sprintf (tmpfilename, "%s%d", LGET ("Untitled"), attempt);
	  attempt++;
	  write_status = write_tmpfile (&(prog->datafile), tt_contents, 
					tmpfilename, size);
	  }
       else
	  write_status = write_tmpfile (&(prog->datafile), tt_contents, title,
					size);
       if (write_status == -1) {
          fprintf (stderr, MGET ("Couldn't create tmp file\n"));
          return (FAIL);
          }
	  
       status = open_newfile (NULL, title, prog->datafile, file_type, 
			      (char *) NULL, 0);
       }
    else if (type == x_selection) {
       prog->tt_load = TRUE;
       tt_sel = xv_get (base_window->base_window, XV_KEY_DATA, SELECTOR);
       xv_set (tt_sel, SEL_RANK_NAME, (char *) tt_contents, NULL);
       current_sel = tt_sel;
       current_dnd_object = NULL;
       load_from_dragdrop ();
       status = 0;
       }
    else  /* can't figure out the type */
       return (FAIL);

    if (status != 0 && prog->verbose) {
       fprintf (stderr, MGET ("Error loading image from tt message\n"));
       return (FAIL);
       }

/*
 * Everything is OK, so let's reply to the old message.
 */

    if (tt_msg != NULL) {
       tt_message_reply (tt_msg);
       tt_message_destroy (tt_msg);
       }

    tt_msg = ttmsg;
    xv_set (base_window->base_window, FRAME_CLOSED, FALSE, NULL);
    dstt_status (prog->tt_sender, 
		 (char *) dstt_set_status (NULL, dstt_status_req_rec),
		 messageid, setlocale (LC_CTYPE, NULL));
    return (HOLD);
}

/*
 * Other application is quitting. Set timer in case imagetool 
 * might be started again soon.
 */

status_t
quit_tt (ttmsg, silent, force, msgID)
Tt_message 	 ttmsg; 
int		 silent;
int 		 force; 
char 		*msgID;
{
    Frame	frame;
    int		n = 1;

    set_tt_timer ();

    xv_set (base_window->base_window, XV_SHOW, FALSE, NULL);
    while (frame = xv_get (base_window->base_window, FRAME_NTH_SUBFRAME, n++))
       xv_set (frame, FRAME_CMD_PUSHPIN_IN, FALSE,
                      XV_SHOW,              FALSE,
                      NULL);

    return (OK);
}

void
break_tt ()
{
    if (prog->tt_sender != (char *) NULL)
       free (prog->tt_sender);

    prog->tt_sender = (char *) NULL;
    prog->tt_started = FALSE;
    if (tt_msg != NULL) {
       dstt_set_status (tt_msg, dstt_status_user_request_cancel);
       tt_message_reply (tt_msg);
       tt_message_destroy (tt_msg);
       tt_msg = NULL;
       }
}

void
respond_to_tt (status)
int	status;
{

/*
 * If status is true, then we got everything OK. If false,
 * then not ok, so respond to the message with a FAIL.
 */

    if ((status == FALSE) && (tt_msg != NULL)) {
       dstt_message_fail (tt_msg, dstt_status_data_not_avail);
       tt_msg = NULL;
       }
       
}

void
save_file_tt ()
{
    dstt_saved (DGET ("File"), current_image->file);
}

