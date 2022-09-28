#ifndef	_DS_TOOLTALK_H_
#define	_DS_TOOLTALK_H_

#ident "@(#)ds_tooltalk.h	1.1 07/08/92 Copyright 1991,1992 Sun Microsystems, Inc."

/* This file is derived from libdeskset's ds_tooltalk.h */

/*
 * "@(#)ds_tooltalk.h	1.6 $25 Jun 1991"
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <desktop/tt_c.h>
#include <xview/xview.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* Deskset Message types */
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
} ds_tt_msgs;

/* Structure used to store tooltalk message information */
typedef struct {
	char		*ds_tt_msg_name;    /* Message string 		     */
	int	 	 ds_tt_msg_args;    /* # arguments sent with message */
	ds_tt_msgs	 ds_tt_msg_type;    /* Message type		     */
} ds_tt_msg_info;
	
/* fd used for ToolTalk */
extern	int		 ds_tt_fd;

#if	defined(__STDC__)

/*  Declarations for ANSI C & C++ */

/* Function declarations */
extern 	Tt_status	ds_tooltalk_init(const char	       *appname,
					 const int		argc,
					 char		      **argv);

extern	void		ds_tooltalk_set_callback(Xv_opaque	base_frame,
						 Notify_func	ttHandler);

extern	void		ds_tooltalk_quit();

extern 	void	       *ds_tooltalk_get_handler(char   	       *ce_value);

extern 	Tt_message	ds_tooltalk_create_message(const void  *handler,
						   const ds_tt_msgs dsMsgType,
						   const char  *msgName);

extern	ds_tt_msg_info	ds_tooltalk_received_message(Tt_message	ttMsg);

extern  void		ds_tooltalk_send_departing_message();

extern  void		ds_tooltalk_set_argv(const int		argc,
					     char	      **argv,
					     const char	       *locale,
					     const int		depth,
					     const int		visual);

#else	/* !defined(__STDC__) */

/* Function declarations */
extern 	int 	        ds_tooltalk_init ();
extern	void		ds_tooltalk_set_callback ();
extern	void		ds_tooltalk_quit ();
extern 	void	       *ds_tooltalk_get_handler ();
extern 	Tt_message	ds_tooltalk_create_message ();
extern	ds_tt_msg_info	ds_tooltalk_received_message ();
extern  void		ds_tooltalk_send_departing_message ();
extern  void		ds_tooltalk_set_argv ();

#endif	/* defined(__STDC__) */

#ifdef	__cplusplus
}
#endif

#endif	/* _DS_TOOLTALK_H_ */
