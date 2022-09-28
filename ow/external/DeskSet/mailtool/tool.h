/*      @(#)tool.h 3.20 IEI SMI      */

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
 * Mailtool - tool global variables
 */

#ifndef mailtool_tool_h
#define mailtool_tool_h

#include <xview/xv_c_types.h>
#include <xview/tty.h>
#include "header.h"

extern struct header_data    *mt_header_data_list;	/* ptr to list of global header_data structs, to support multiple base frames */
extern Frame    mt_frame;	/* the mailtool frame */

extern Icon mt_current_icon;
extern Icon mt_unknown_icon;

extern Xv_Font	mt_font;	/* the default font */
extern struct header_data *mt_header_data_list;

extern int	charheight, charwidth;
extern int	mt_popuplines;
extern int	mt_idle;		/* closed, not processing mail */
extern int      mt_iconic;	/* specifies whether tool is to be created
				 * out iconic or not */
extern int      mt_destroying;	/* true if tool is in process of being
				 * destroyed */
extern int	mt_debugging;
extern int	mt_use_alerts;
extern int	mt_bells;	/* number of bells */
extern int	mt_flashes;	/* number of flashes */
extern int	mt_nomail;		/* no mail in current folder */
#define	CURRENT_FOLDER(hd)	(hd)->hd_folder /* replaces current_folder global */
extern int      mt_system_mail_box;	/* true if current folder is system
					 * mail box */
extern char    *mt_load_from_folder;
extern char    *mt_namestripe_right;


struct reply_panel_data {
	struct header_data *	hd;
	Frame		frame;
	Panel		reply_panel;
	Textsw		replysw;
	Textsw		replysw_view;	/* first view of replysw */
	Tty		rpd_ttysw;
	struct attach_list	*rpd_al;
	Menu		header_menu;
	Panel_item	deliver_item;
#ifdef EDITOR
	Panel_item	edit_item;
#endif
	Panel_item	clear_item;
	Panel_item	address_item;
#ifdef	COMPOSE_ADDR
	Menu		addr_menu;
	Panel_item	to_item;
#endif	COMPOSE_ADDR
	Panel_item	dest_fillin;
	Panel_item	subject_fillin;
	Panel_item	cc_fillin;
	Panel_item	bcc_fillin;
	Panel_item	rpd_record_item;
	Xv_opaque	rpd_focus_item;
	int		base_fillin_y;
	int		incr_fillin_y;
	Icon		composing_icon;
	struct reply_panel_data	*next_ptr;
	void		(*rpd_old_notify_proc)();
	struct msg	*reply_msg;	/* the sending message */
	char		*rpd_bufferid;	/* For ToolTalk Open/Paste/Close msg */
	char		*rpd_dead_letter;
	char		inuse;		/* 0 = available, 1 = inuse */
	char		rpd_modified;
	char		rpd_checkpt;	/* set to TRUE if rpd_modified is set */
	char		rpd_rmdead;	/* rm dead letter if delivery success */
	char		itimer_started;	/* set to TRUE when itimermax started */
	int		deliver_flags;	/* the four deliver options */
#ifdef EDITOR
	char		rpd_editor_inuse;
	char		*rpd_editor_file;
#endif
	int		rpd_deliver_error;	/* true if there was an
						 * early delivery error that
						 * needs to be remembered
						 */
};


struct view_window_list {
	Frame		vwl_frame;
	Frame		vwl_headerframe;
	Textsw		vwl_textsw;
	struct view_window_list	*vwl_next;
	struct attach_list	*vwl_al;
	struct msg     *vwl_msg;	/* the message */
	char		vwl_garbage;	/* 0 = good, 1 = garbage */
	char		vwl_modified;	/* TRUE if window was changed */
	char		vwl_only_attach_modified; /* TRUE if only attachment
						   * part was changed
						   */
};

struct inactivation_list {
	Xv_opaque	handle;
	int		panel_item;
	struct	inactivation_list	*next_ptr;
};

/*
 * a pointer to a struct of this type is the value returned by the menus
 * behind the buttons on the  mailtool control panel. The generic panel event
 * proc then calls the appropriate procedure after setting the shift mask. 
 */


/* Mailtool notify procs and event procs */

EXTERN_FUNCTION(void mt_menu_done_proc, (Menu menu, Menu_item menu_item, Event *ie));
EXTERN_FUNCTION(void mt_deliver_proc, (Menu menu, Menu_item menu_item));
#ifdef PEM
EXTERN_FUNCTION(void mt_pem_proc, (Menu menu, Menu_item menu_item));
#endif PEM
EXTERN_FUNCTION(void mt_done_proc, (Panel_item item, Event *ie));


/* generate procs */

EXTERN_FUNCTION(Menu mt_get_folder_menu,
	(Menu m, char *path_string, char *item_string));

EXTERN_FUNCTION(Menu mt_folder_menu_gen_proc,(Menu m, Menu_generate operation));


/* miscellaneous routines shared by several modules */


void	mt_create_compose_textsw();

struct reply_panel_data		*mt_create_compose_panel();
struct reply_panel_data		*mt_get_compose_frame();

void mt_init_tool_storage(void);
void mt_draw_selection_box(HeaderDataPtr hd, struct msg *msg);
void mt_repaint_headers(Canvas canvas, Pixwin *pw, Rectlist *area, int clear);
void mt_clear_header_canvas(struct header_data *hd);
void scroll_set_view_len(struct header_data *hd, int pos,
	unsigned long *objlen_ptr);
void mt_cleanup_proc(int pid);
void mt_finish_initializing(Frame frame);
#ifdef MULTIPLE_FOLDERS
struct header_data *mt_finish_initializing1(Frame frame);
#endif
void mt_start_tool(int argc, char **argv);
void mt_done_signal_proc(void);
void mt_init_mailtool_defaults(void);
void mt_start_timer(void);
int mt_get_scrollbar_width(Canvas canvas);
struct header_data * mt_get_header_data(Xv_opaque win);
void mt_window_header_data(struct header_data *hd, Xv_opaque win);
void mt_set_header_data(struct header_data *hd);
void mt_copy_header_data(Xv_opaque win1, Xv_opaque win2);



void	mt_start_tool();
void	mt_add_menu_item();
void	mt_restore_namestripe();
void	mt_stop_reply();
void	mt_display_reply();
void	mt_undel_list_init();

struct reply_panel_data *mt_create_new_compose_frame(struct header_data *);


#endif /* mailtool_tool_h */
