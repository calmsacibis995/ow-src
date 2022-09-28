/*      @(#)glob.h 3.14 IEI SMI      */

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

/*
 * Mailtool - global constants
 */

extern char *name_Mail_Tool;    /* localized "Mail Tool" */
extern char *name_mailtool;     /* localized "mailtool" */
extern char *name_none;         /* localized "[None]" */

#define	DEFMAXFILES	15	/* default max # of file names in file menu */
#define	LINE_SIZE	1024
#define MT_SHORT_LINE	80

#define	MT_MAX_VIEWS		10	/* maximum number of viewing windows */

#define	MT_LOAD_OP	0
#define MT_MOVE_OP	1
#define MT_COPY_OP	2
#define MT_OPEN_OP	3
#define MT_LOAD1_OP	4


/*
 * Mailtool - global variables
 */

extern int	mt_memory_maximum;	/* for text subwindows */
extern int	mt_aborting;	/* aborting, don't save messages */
extern char    *mt_cmdname;	/* our name */
extern struct view_window_list	*mt_view_window_list; /* List of view wins */

/* replacement for mt_curmsg, number of current message */
#define MT_CURMSG(hd)	(hd)->hd_curmsg
/* replacement for mt_folder, string of current folder */
#define MT_FOLDER(hd)	(hd)->hd_folder_name
/* replacement for mt_frame */
#define MT_FRAME(hd)	(hd)->hd_frame
/* 
 * replacement for mt_composition_window_list 
 * RPD stands for reply_panel_data struct, 
 * not the xview compose window itself 
 */
#define MT_RPD_LIST(hd)	(hd)->hd_rpd_list

extern struct msg *mt_prevmsg;	/* number of previous message */
extern int	mt_scandir;	/* scan direction */
extern int	mt_num_show_windows;	/* number of viewing windows up */
extern char	mt_multibyte;		/* TRUE if in multibyte locale */


extern struct msg *mt_delp;	/* pointer to linked list of deleted messages */
extern int	mt_new_count;	/* number of new messages */

extern int	mt_command_parameter_key;
extern int	mt_command_panel_layout_key;
extern int	mt_composition_panel_layout_key;
extern int	mt_composition_window_list_key;
extern int	mt_composition_frame_key;
extern int	mt_viewing_window_list_key;
extern int	mt_menu_parent_button_key;
extern int	mt_file_fillin_key;
extern int	mt_file_panel_key;

extern	Server_image    compose_image, compose_image_mask; 
extern	Server_image    dead_image, dead_image_mask;
extern	Server_image    mail_image, mail_image_mask;
extern	Server_image    nomail_image, nomail_image_mask;
extern	Server_image    emptymail_image, emptymail_image_mask;
extern	Server_image    reply_image, reply_image_mask;

char	       *getenv();
char	       *index();
char	       *mt_savestr();
char	       *mt_savetabbedstr();
char	       *mt_value();
char	       *mt_get_header();
u_long		mt_current_time();


int		mt_copy_msgs();
int		mt_compose_msg();
void		mt_load_msg();
void		mt_print_msg();
void		mt_preserve_msg();
void		mt_mail_cmd();
void		mt_mail_start_cmd();
void		mt_assign();
void		mt_view_reuse_proc();
void		mt_view_new_proc();
int		mt_deassign();
int		mt_mail_seln_exists();
struct msg     *mt_get_curselmsg();
int		mt_is_prefix();
int		mt_full_path_name();
char	       *mt_mail_get_line();
