/* Copyright (c) 1992 by Sun Microsystems, Inc. */
#ifndef _DS_TOOLTALK_H
#define _DS_TOOLTALK_H

#ident	"@(#)ds_tooltalk.h	1.4	92/11/10 SMI"

#include <xview/xview.h>
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
	char*		ds_tt_msg_name;    /* Message string 		     */
	int		ds_tt_msg_args;    /* # arguments sent with message */
	ds_tt_msgs	ds_tt_msg_type;    /* Message type		     */
} ds_tt_msg_info;

#ifdef __cplusplus
extern "C" {
#endif

/* fd used for ToolTalk */
extern int		ds_tt_fd;

/* Function declarations */
extern int		ds_tooltalk_init(char*, int, char**);
extern void		ds_tooltalk_set_argv(int, char**, char*, int, int);
extern void		ds_tooltalk_set_callback(Xv_opaque, Notify_func);
extern void		ds_tooltalk_quit();
extern void*		ds_tooltalk_get_handler(char*);
extern Tt_message	ds_tooltalk_create_message(void*, ds_tt_msgs, char*);
extern ds_tt_msg_info	ds_tooltalk_received_message(Tt_message);
extern void		ds_tooltalk_send_departing_message();
#ifdef __cplusplus
}
#endif

#endif /* !_DS_TOOLTALK_H */
