/*
 *
 * deskset_tt.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)ds_tooltalk.c 3.4 94/03/03 Copyr 1990 Sun Micro";
#endif

#include "ds_tooltalk.h"
#include <xview/xview.h>
#include <string.h>
#include <stdlib.h>

/* Number of DeskSet messages */
#define TOTAL_DESKSET_MSGS	DS_TT_NO_STD_MSG

/* Number of command line args that can be used to set the DISPLAY */
#define CMD_LINE_ARGS		5

/* Number of visuals currently supported - wish this were in
   X.h, but that just the declarations */
#define MAXVISUALS		6

/* All of our messages start with this string */
#define SUN_DESKSET		"Sun_DeskSet_"

/* If we're using otypes, then this is prepended to the otype */
#define SUN_DESKSET_OBJECT	"Sun_DeskSet_Object_"

static ds_tt_msg_info Deskset_Msgs [TOTAL_DESKSET_MSGS + 1] = {
	"launch", 	 3, 	DS_TT_LAUNCH_MSG,
	"status", 	 1, 	DS_TT_STATUS_MSG, 
	"dispatch_data", 1,	DS_TT_DISPATCH_DATA_MSG, 
	"move",		 5,	DS_TT_MOVE_MSG,
	"quit",		 0,	DS_TT_QUIT_MSG,
	"hide",		 0,	DS_TT_HIDE_MSG,
	"expose",	 0,	DS_TT_EXPOSE_MSG,
	"retrieve_data", 1,	DS_TT_RETRIEVE_DATA_MSG,
	"departing",	 0,	DS_TT_DEPARTING_MSG, 
	NULL,		 0,	DS_TT_NO_STD_MSG
 };

static char 	**sender_procids = NULL;

static int	  stackmark;
static char      *tt_sessid;
int	  	  ds_tt_fd;

static char	 *visual_names [MAXVISUALS] = { "StaticGray", "GrayScale", 
				 		"StaticColor", "PseudoColor", 
						"TrueColor", "DirectColor" };


/* Comment out C++ declaration for now */
/* int ds_tooltalk_init (char *appname) */

/* 
 * ds_tooltalk_init
 * Initializes tooltalk environment. Arguments are argv[0] (application
 * name) and the base frame handle (needed to set up the notify handler).
 * This function returns the application name - note that this may be
 * different from argv[0] since argv[0] may be a full pathname
 * (ie. /usr/local/bin/openwin/xview/filemgr).
 *
 * Parameters: appname:  The application name (argv[0]).
 *			 Note that full path name is OK (ie /bin/xxx).
 *	       argc:	 Number of command line arguments.
 *	       argv:	 Array of command line arguments.
 *
 * Returns: 0 if successful.
 *	    tooltalk error code if not successful
 */

int ds_tooltalk_init (appname, argc, argv)
char  *appname; 
int    argc;
char **argv;
{
   char		*tt_procid;
   char		*tt_appname;
   char		*tt_ptype;
   Tt_status	 tt_status;
   int		 i, j;
   Tt_message	 in_msg;
   char		*server = NULL;

   stackmark = tt_mark();

/*
 * Get correct display. Look through command line args and see
 * if any of the following were used:
 *    -Wr -display -xrm -Wd or -default
 * If so, get the value of the DISPLAY.
 */

   for ( i = 1; i < argc && server == (char *) NULL; i++ ) {
       if ((strcmp (argv[i], "-Wr") == 0) || 
	   (strcmp (argv[i], "-display") == 0)) {
	  if (argv[i+1] != (char *) NULL) {
	     server = (char *) malloc (strlen (argv[i+1]) + 1);
	     strcpy (server, argv[i+1]);
	     i++;
	     }
	  }
       else if ((strcmp (argv[i], "-Wd") == 0) ||
		(strcmp (argv[i], "-default") == 0)) {
	  if (argv[i+1] != (char *) NULL) {
	     if (strcmp (argv[i+1], "server.name") == 0) {
	        if (argv[i+2] != (char *) NULL) {
	           server = (char *) malloc (strlen (argv[i+2]) + 1);
	           strcpy (server, argv[i+2]);
	           i += 2;
	           }
		}
	     }
	  }
       else if (strcmp (argv[i], "-xrm") == 0) {
	  if (argv[i+1] != (char *) NULL) {
	     char *p = strchr (argv[i+1], ':');
	     if (p != (char *) NULL) {
		int length = p - argv[i+1];
		if (strncmp (argv[i+1], "server.name", length) == 0) {
		   p++;
		   server = (char *) malloc (strlen (p) + 1);
		   strcpy (server, p);
		   i++;
		   }
	        }
	     }
	  }
       }	

   if (server == (char *) NULL) {
      server = getenv ("DISPLAY");

      /* this is a hack, but it seems to emulate what XView and Xt does */
      if (! server) {
	 server = ":0";
      }
   }

   tt_sessid = tt_X_session (server);
   if((tt_status = tt_ptr_error(tt_sessid)) != TT_OK)
	return ( tt_status );

   tt_default_session_set (tt_sessid);

   tt_procid = tt_open();
 
   if ((tt_status = tt_ptr_error (tt_procid)) != TT_OK) 
      return ( tt_status );

/* establish fd to wait on for messages */

   ds_tt_fd = tt_fd();

/*
 * If the appname is NULL, then this application does not want to
 * be a handler.  So, in this case, just initialize, but do not
 * declare ptype and return.
 */
 
   if (appname == (char *) NULL) {

/*  just join session */

      if ((tt_status = tt_session_join(tt_sessid)) != TT_OK) 
         return (tt_status);

      return (0);
      }

   tt_ptype = (char *) malloc (strlen (SUN_DESKSET) + strlen (appname) + 1);

   sprintf (tt_ptype,"%s%s", SUN_DESKSET, appname);
		
   if ((tt_status = tt_ptype_declare (tt_ptype)) != TT_OK) 
      return (tt_status);

/* join session */

   if ((tt_status = tt_session_join(tt_sessid)) != TT_OK) 
      return (tt_status);

/*
 * Return 0 - everything went OK
 */

   return (0);
}

/*
 * ds_tooltalk_set_argv
 * Sets command line arg values if application is being started via
 * tooltalk. Inserts appropriate values for depth, visual and locale.
 *
 * Parameters:  argc - Number of command line args.
 * 		argv - Array of command line args.
 * 		locale - Locale to use.
 *		depth - Depth to use.
 * 		visual - Visual to use.
 * 
 * No Return Value
 */


void ds_tooltalk_set_argv (argc, argv, locale, depth, visual)
int	  argc;
char	**argv;
char	 *locale;
int	  depth;
int	  visual;
{
    int 	 i;
    char	 arg_value [100];

/*
 * Go through argv, and find the various command line args:
 *  -display, -visual, and -lc_basiclocale.
 * When they are found, substitute in the various values passed
 * to this function.
 */

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-depth") == 0) {
	   sprintf (arg_value, "%d", depth);
	   argv [i+1] = (char *) malloc (strlen (arg_value) + 1);
	   strcpy (argv [i+1], arg_value);
	   i++;
	   }
	if (strcmp(argv[i], "-visual") == 0) {
	   argv [i+1] = (char *) malloc (strlen (visual_names [visual]) + 1);
	   strcpy (argv [i+1], visual_names [visual]);
	   i++;
	   }
	if (strcmp(argv[i], "-lc_basiclocale") == 0) {
	   argv [i+1] = (char *) malloc (strlen (locale) + 1);
	   strcpy (argv [i+1], locale);
	   i++;
	   }
	}

}


/* 
 * ds_tooltalk_set_callback
 * Sets callback function which should be called whenever a message
 * is received via ToolTalk.
 *
 * Parameters:  frame_handle - The application's base frame handle.
 *		tooltalk_handler_func - Pointer to function that should
 *					run when a message is received via
 *					ToolTalk.
 *
 * No Return value
 */

void ds_tooltalk_set_callback (frame_handle, tooltalk_handler_func)
Xv_opaque frame_handle;
Notify_func tooltalk_handler_func;
{

/* 
 * Set function which should execute when any incoming messages 
 * are found. 
 */

   notify_set_input_func (frame_handle, tooltalk_handler_func, ds_tt_fd);

}
		

/*
 * ds_tooltalk_quit
 * Ends a tooltalk session.
 * 
 * No parameters.
 * 
 * No return value.
 */

void ds_tooltalk_quit () 
{
    int id_index;

    tt_session_quit (tt_sessid);
    tt_release (stackmark);
    tt_close ();

/*
 * delete all sender procid references
 */

   if (sender_procids != (char **) NULL)
      for (id_index = 0; sender_procids [id_index] != (char *) NULL; id_index++)
	  free (sender_procids [id_index]);

   cfree (sender_procids);
}


static 	char   *tt_tokens = ": ";

/* Comment out C++ declaration for now */
/* void *ds_tooltalk_get_handler (char *ce_value) */

/*
 * ds_tooltalk_get_handler 
 * Given a value from the classing engine (for the attribute TYPE_OPEN_TT),
 * returns the handler from this string.
 *
 * Parameter:	ce_value - Value retrived from classing engine for the
 *			   attribute TYPE_OPEN_TT.
 *
 * Returns:	(void *) - The handling application.
 *
 */

void *ds_tooltalk_get_handler (ce_value) 
char *ce_value;
{

   return ((void *) strtok (ce_value, tt_tokens));

}


/* Comment out C++ declaration for now... */
/* Tt_message ds_tooltalk_create_message (void *handler,        */
/*					  ds_tt_msgs signature, */
/*					  char *msg_name);      */

/*
 * ds_tooltalk_create_message
 * Given the handler of the handling application and the signature, creates
 * a tooltalk message and returns it to the application. Note that the 
 * application must then complete the message with any required arguments
 * and send the message.
 *
 * Parameters:	handler	- The handling application (value returned from
 *			  ds_tooltalk_get_handler). A NULL value means that
 *			  the caller of this function will set the handler
 *			  of the tooltalk message itself.
 *		signature - The message type of the message, i.e. 
 *			    DS_TT_LAUNCH_MSG, DS_TT_DISPATCH_DATA_MSG, etc.
 *		msg_name - If the signature is DS_TT_NO_STD_MSG (not one
 *			   of the nine standard msgs), then this is the
 *			   message operation name.
 *
 * Returns: (Tt_message) - A tooltalk message structure.
 *
 */

Tt_message ds_tooltalk_create_message (handler, signature, msg_name)
void *handler;
ds_tt_msgs signature;
char *msg_name;
{

   char		*tt_message;
   char		*tt_otype;
   Tt_status	 tt_status;
   Tt_message	 tt_msg;


/* Put handler and signature together to form message  - if using ptypes only

   tt_message = (char *) malloc (sizeof (SUN_DESKSET) + 
				 sizeof ( (char *) handler) +
				 sizeof (signature) + 1);

   sprintf (tt_message, "%s%s_%s", SUN_DESKSET, (char *) handler, signature);
*/

/*
 * Using otypes, just need the signature 
 */


/* Create the tooltalk message */

   if ((tt_status = tt_ptr_error(tt_msg =  tt_message_create())) != TT_OK) {

#ifdef DEBUG
      fprintf (stderr, "Couldn't create tooltalk message. Status: %d\n",
							tt_status);
#endif
      return ( (Tt_message) NULL);
      }

/* 
 * Set message address. If the handler was NULL, then the calling
 * app is going to set the handler explicitly so set the address type
 * to TT_HANDLER. Otherwise, it's TT_OBJECT. 
 */

   if (handler == (char *) NULL) 
      tt_status = tt_message_address_set (tt_msg, TT_HANDLER);
   else
      tt_status = tt_message_address_set (tt_msg, TT_OBJECT);

/* If using ptypes...
   tt_status = tt_message_address_set(tt_msg, TT_PROCEDURE);
*/

   if (tt_status != TT_OK) {

#ifdef DEBUG
      fprintf (stderr, "Couldn't set tooltalk message paradigm. Status: %d\n",
							tt_status);
#endif

      return ( (Tt_message) NULL);
      }

/* Set the message class type */

   if ((tt_status = tt_message_class_set(tt_msg, TT_REQUEST)) != TT_OK) {

#ifdef DEBUG
      fprintf (stderr, "Couldn't set tooltalk message class. Status: %d\n",
							tt_status);
#endif

      return ( (Tt_message) NULL);
      }

/* Set the message scope */

   if ((tt_status = tt_message_scope_set(tt_msg, TT_SESSION)) != TT_OK) {

#ifdef DEBUG
      fprintf (stderr, "Couldn't set tooltalk message scope. Status: %d\n",
							tt_status);
#endif

      return ( (Tt_message) NULL);
      }

/* Set the message's session */

   if ((tt_status = tt_message_session_set(tt_msg, tt_sessid)) != TT_OK) {

#ifdef DEBUG
      fprintf (stderr, "Couldn't set tooltalk message session. Status: %d\n",
							tt_status);
#endif

      return ( (Tt_message) NULL);
      }

/* Set the disposition of the message */

   if ((tt_status = tt_message_disposition_set (tt_msg,TT_START)) != TT_OK) {

#ifdef DEBUG
      fprintf (stderr, "Couldn't set tooltalk message reliability. Status: %d\n",
							tt_status);
#endif

      return ( (Tt_message) NULL);
      }

/* 
 * Set the message operation. If not one of the 9 DeskSet msgs, 
 * the the message operation name was passed to the function.
 */
 
   if (signature != DS_TT_NO_STD_MSG)
      tt_status = tt_message_op_set (tt_msg,
				Deskset_Msgs [(int) signature].ds_tt_msg_name);
   else
      tt_status = tt_message_op_set (tt_msg, msg_name);
       
   if (tt_status != TT_OK) {
 
#ifdef DEBUG
      fprintf (stderr, "Couldn't set tooltalk message. Status: %d\n",
						tt_status);
#endif

      return ( (Tt_message) NULL);
      }

/*
 * Set the otype of the message - unless the message is the departing msg
 * or the handler was NULL. 
 */

   if ((signature != DS_TT_DEPARTING_MSG) && (handler != (char *) NULL)) {
      tt_otype = (char *) malloc (strlen (SUN_DESKSET_OBJECT) + 
				  strlen ( (char *) handler) + 1);

      sprintf (tt_otype, "%s%s",SUN_DESKSET_OBJECT, (char *) handler);

      if ((tt_status = tt_message_otype_set (tt_msg, tt_otype)) != TT_OK) {

#ifdef DEBUG
         fprintf (stderr, "Couldn't set tooltalk message otype. Status: %d\n",
						   tt_status);
#endif

         return ( (Tt_message) NULL);
         }
      }

/* Return the tooltalk message */

   return (tt_msg);
}


static 	char	message_token = '_';

/* Comment out C++ declaration for now... */
/* ds_tt_msg_info ds_tooltalk_received_message (Tt_message tt_msg) */

/*
 * ds_tooltalk_received_message
 * Given a tooltalk message, returns a structure defining that message.
 * For example, if a tooltalk message was received with op = "dispatch_data",
 * then this function would return
 *		{ "dispatch_data", 1, DS_TT_DISPATCH_DATA_MSG }.
 *
 * Parameters:	tt_msg - The ToolTalk message.
 *
 * Returns: (ds_tt_msg_info) - A structure containing the message name received,
 *			       the message type, and the number of arguments
 *			       received along with the message.
 * 
 */

ds_tt_msg_info ds_tooltalk_received_message (tt_msg)
Tt_message tt_msg;
{
   char  	       *tt_signature;
   char		      **sender_ids;
   int	  	 	msg_index = 0;
   int		 	id_index ;
   ds_tt_msg_info	deskset_msg ;
   char		       *sender = NULL;
   Tt_state		msg_state;

/* Get the message operation */

   char  *tt_message_name = tt_message_op (tt_msg);

/*
 * Determine if this message was SENT or HANDLED. If HANDLED,
 * then we need to get the handlers procid (since that is really
 * the 'sender' of this message). If the message is SENT, then
 * we get the senders procid.
 */

   msg_state = tt_message_state (tt_msg);

   if (msg_state == TT_SENT)

/*
 * Get the sender of this message and store the procid in case
 * we want to send the 'departing' message to everyone we've ever
 * heard from.
 */

      sender = tt_message_sender (tt_msg);

   else if ((msg_state == TT_HANDLED) || (msg_state == TT_REJECTED))

      sender = tt_message_handler (tt_msg);

   if (sender != (char *) NULL) {

      char *this_sender = (char *) malloc (strlen (sender) + 1);

      strcpy (this_sender, sender);

/*
 * Add to our sender_procids
 */

      if (sender_procids == (char **) NULL) {
         sender_procids = (char **) calloc (2, sizeof (char *));
         sender_procids [0] = this_sender;
         sender_procids [1] = (char *) NULL;
         }
   
      else {
         for (id_index = 0 ; sender_procids [id_index] != (char *) NULL ;
		   id_index++) 
	     if (strcmp (sender_procids [id_index], sender) == 0)
	        break ;

         if (sender_procids [id_index] == (char *) NULL) {
	    sender_procids = (char **) realloc (sender_procids,
			(id_index + 2) * sizeof (char *));
            sender_procids [id_index] = this_sender;
            sender_procids [id_index + 1] = (char *) NULL;
            }
         }    
      }
   
/*
 * Determine signature from this message. - ptypes only


   tt_signature = strrchr (tt_message_name, message_token);
 
   if (tt_signature == (char *) NULL) {
      return (Deskset_Msgs [(int) DS_TT_NO_STD_MSG]);


 * Since we're pointing to the '_' advance the pointer by one to
 * point to the string.

   tt_signature++;

*/

/*
 * Find the message number.
 */

   for ( ; (msg_index < TOTAL_DESKSET_MSGS) &&
	   (strcmp (tt_message_name, 
		    Deskset_Msgs [msg_index].ds_tt_msg_name) != 0) 
	 ; msg_index++);

/*
 * Check if the message isn't one of the nine 'standard' ones.
 * If not, then fill in the ds_tt_msg_name, and the ds_tt_msg_args
 * of this message.
 */
 
   if (msg_index == DS_TT_NO_STD_MSG) {
      if (Deskset_Msgs [msg_index].ds_tt_msg_name != (char *) NULL)
	 free (Deskset_Msgs [msg_index].ds_tt_msg_name);

      Deskset_Msgs [msg_index].ds_tt_msg_name =
				(char *) malloc (strlen (tt_message_name) + 1);
      strcpy (Deskset_Msgs [msg_index].ds_tt_msg_name, tt_message_name);
 
      Deskset_Msgs [msg_index].ds_tt_msg_args = tt_message_args_count (tt_msg);
      }

/* Return the message number */

   deskset_msg = Deskset_Msgs [msg_index];

   return ( deskset_msg ) ;
}

/*
 * ds_tooltalk_send_departing_message
 * Used by handling applications to send the 'departing' message
 * to any other process that has ever sent this application a message.
 * It should be used to tell all the senders that this handling application 
 * is going away...
 *
 * No parameters.
 * 
 * No return value.
 */

void ds_tooltalk_send_departing_message ()
{

    Tt_message	quit_msg;
    Tt_status	tt_status;
    int	 	id_index;

/*
 * Go through sender_procids and send all processes the departing
 * message
 */

   if (sender_procids != (char **) NULL)
      for (id_index = 0; sender_procids [id_index] != (char *) NULL; 
			id_index++) {
	  quit_msg = ds_tooltalk_create_message ((char *) NULL,
	 	     Deskset_Msgs [(int) DS_TT_DEPARTING_MSG].ds_tt_msg_type);
          
/*
 * Set the process id of the message
 */

       	  if (tt_message_handler_set (quit_msg, sender_procids [id_index]) 
						== TT_OK)

             if (tt_message_address_set (quit_msg, TT_HANDLER) == TT_OK) {
	        tt_status = tt_message_send (quit_msg); 

#ifdef DEBUG
		if (tt_status != TT_OK)
		   fprintf (stderr,"Couldn't send message.  Status: %d\n",
						tt_status);
#endif
		}

 	  }

}
