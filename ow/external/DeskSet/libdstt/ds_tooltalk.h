

/*	
 * "@(#)ds_tooltalk.h	3.1 $04/03/92"
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ifndef DS_TOOLTALK_H
#define DS_TOOLTALK_H

#include <desktop/tt_c.h>

/* Message types */
typedef enum  { 
	DS_TT_LAUNCH_MSG, 		
	DS_TT_STATUS_MSG, 
	DS_TT_DISPATCH_DATA_MSG, 
	DS_TT_MOVE_MSG, 
	DS_TT_QUIT_MSG, 
	DS_TT_HIDE_MSG, 
	DS_TT_EXPOSE_MSG, 
	DS_TT_RETRIEVE_DATA_MSG, 
	DS_TT_DEPARTING_MSG, 
	DS_TT_NO_STD_MSG 
} ds_tt_msgs ;

/* Structure used to store tooltalk message information */
typedef struct {
	char		*ds_tt_msg_name;    /* Message string 		     */
	int	 	 ds_tt_msg_args;    /* # arguments sent with message */
	ds_tt_msgs	 ds_tt_msg_type;    /* Message type		     */
} ds_tt_msg_info;
	
/* fd used for ToolTalk */
extern	int		 ds_tt_fd;

/*  Declarations for C++ - commented out for now 		   	     */
/* extern 	char 	       *ds_tooltalk_init (char *); 		     */
/* extern	void		ds_tooltalk_set_callback (Xv_opaque, Notify_func */
/* extern 	void	       *ds_tooltalk_get_handler (char *);	     */
/* extern 	Tt_message	ds_tooltalk_create_message (void *, char *); */
/* extern	ds_tt_msg_info	ds_tooltalk_received_message (Tt_message);   */

/* Function declarations */
extern 	int 	        ds_tooltalk_init ();
extern	void		ds_tooltalk_set_callback ();
extern	void		ds_tooltalk_quit ();
extern 	void	       *ds_tooltalk_get_handler ();
extern 	Tt_message	ds_tooltalk_create_message ();
extern	ds_tt_msg_info	ds_tooltalk_received_message ();
extern  void		ds_tooltalk_send_departing_message ();
extern  void		ds_tooltalk_set_argv ();

#endif
