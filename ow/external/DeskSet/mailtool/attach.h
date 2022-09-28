/*	@(#)attach.h 3.14 94/05/04 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * Attachment list #defines and data types
 */

#ifndef _mt_attach_h
#define _mt_attach_h

#include <sys/stat.h>
#include <xview/dragdrop.h>
#include <xview/textsw.h>
#include <desktop/tt_c.h>

 /*
  * Distance user must drag before we detect it
  */
#define DAMPING	7

#define MT_ATTACH_ICON_W	32
#define MT_ATTACH_ICON_H	32
#define MT_ATTACH_MIN_X_GAP	30

/*
 * Classing Engine types -- these have no file namespace entries.
 * MT_CE_TEXT_TYPE is used for data comming out of textsws. MT_CE_DEFAULT_TYPE
 * is used when we can't find a match in the classing engine.
 */
#define MT_CE_TEXT_TYPE		"text"
#define MT_CE_DEFAULT_TYPE	"default"
#define MT_CE_APPLICATION_TYPE	"default-app"
#define MT_CE_AUDIO_TYPE	"audio-file"
#define MT_CE_AE_TYPE		"ae-file"

/*
 * Is x,y within the rectangle described by target_ coordinates?
 */
#define mt_within(x, y, target_x, target_y, target_h, target_w) \
	(x >= target_x && x <= target_x + target_w && \
	 y >= target_y && y <= target_y + target_h)
	
/*
 * Each attachment in the attachment list is represented by a attach_node
 */
typedef struct attach_node {
	struct attach	*an_at;		/* Msg attachment handle */
	struct attach_node *an_next;	/* Next file in list */
	Server_image	an_icon;	/* Image to display */
	char	*an_open_tt;		/* TT open method if any */
	char	*an_tt_media;		/* Message Alliance Media Type */
	Tt_message an_pending_ttmsg;	/* Pending TT request if any */
	Tt_status an_ttstate;		/* State of pending request */
	char	*an_handler_id;		/* TT app currently displaying data */
	char	*an_application;	/* Objects action */
	struct msg *an_msg;		/* Tmp msg for recusive viewing */
	char	*an_buffer;		/* Buffer for tmp msg */
	short	an_pending;		/* TRUE if on pending list */
	char	*an_msgID;		/* TT message ID */
	short	an_ttnretries;		/* Number of launch retries */
	short	an_label_width;		/* Pixel width of label */
	short	an_label_length;	/* # of characters in label */
	short	an_x;			/* x,y location of icon */
	short	an_y;		
	short	an_number;		/* Position # used for display only */
	short	an_ndelete;		/* Delete number */
	char	an_selected;		/* TRUE if object is selected */
} Attach_node;


/*
 * Attachment list. 
 */
typedef struct attach_list {
	struct header_data *al_hd;	/* For recursive message body part */
	Canvas	al_canvas;		/* Canvas to paint attachments in */
	Panel	al_panel;		/* Control panel for attachments */
#ifdef FILECHOOSER
	Xv_opaque	al_saveas_fc;	/* Save As popup. Updated by selctn */
#endif
	Panel_item	al_msg_item;	/* Item for message in attach panel */
	Frame	al_frame;		/* Owner of attachment panel */
	Frame	al_errorframe;		/* where to put error footer msgs */
	Frame	al_headerframe;		/* where the message header window is */
	Display		*al_display;	/* Xlib drawing info */
	Drawable	al_drawable;
	Xv_drop_site	al_drop_site;	/* xview id for canvas drop site */
	Xv_Font	al_font;		/* Font to render attachmt labels in */
	Xv_opaque al_fontset;		/* fontset for i18n l4 */
	Font	al_fontid;		/* X font id for labels */
	GC	al_gc;			/* GC to render attachments in */
	GC	al_cleargc;
	struct msg *al_msg;
	Attach_node	*al_pending_list; /* Pending attachments */
	short	al_w;			/* Width, height of selection hot spt */
	short	al_h;
	short	al_x_gap;		/* Gaps between icons when displayed */
	short	al_y_gap;
	short	al_delete_cnt;		/* Number of deleted nodes */
	short	oversize_notice;	/* flag to make sure we don't flood user
					 * with 2 oversize notices (attachment,
					 * message)
					 */
} Attach_list;

/*
 * Pointer to a function which returns an int
 */
typedef int (*Function)(Attach_list *, Attach_node *, long arg);


/* functions defined in attach_canvas.c */
void		mt_create_attach_canvas(Frame, Panel, Attach_list *);
void		mt_set_attach_canvas_height(Attach_list *);
void		mt_repaint_attach_canvas(Attach_list *, int clear);
void		mt_layout_attach_display(Attach_list *);
char *		mt_attach_name(struct attach *);
int		add_attachment_memory(Attach_list *, struct msg *, void *data,
			int length, char *type, char *label, int executable);
int		mt_set_attach_selection(Attach_list *, Attach_node *);
int		mt_delete_attach_selection(Attach_list *, Attach_node *, long);
int		mt_undelete_attach(Attach_list *);
void		mt_tt1_invoke_application(Attach_list *, Attach_node *,
			int use_tooltalk);
void		mt_invoke_application(Attach_list *, Attach_node *,
			int use_tooltalk);
Attach_node	*mt_create_attachment(Attach_list *, void *data,
			int length, char *type, char *label, int executable);
Attach_node	*mt_fcreate_attachment(Attach_list *, char *path, char *label,	
			int tmpfile);
void		mt_add_attachment(Attach_list *, Attach_node *, struct msg *);
void		mt_drag_attachment(Attach_list *, struct msg *, int copy,
			int loadok, void (*cleanup_proc)(), void *cleanup_arg);
void		mt_clear_attachments(Attach_list *);
void		mt_load_attachments(Textsw, Attach_list *, struct attach *,
			int skip_deleted);
struct attach *	mt_get_next_attach(struct attach *);
int		mt_is_text_attachment(struct attach *);
void		mt_rename_attachment(Attach_list *, Attach_node *, char *label);
Attach_list	*mt_get_attach_list(Xv_opaque win);
void		mt_set_attach_list(Xv_opaque win, Attach_list *al);


/* functions defined in attach_list.c */
Attach_list	*mt_create_attach_list(Frame frame, Frame error_Frame,
			struct msg *, Frame headerframe);
void		mt_set_attach_msg(Attach_list *, struct msg *);
Attach_node	*mt_get_attach_node(struct attach *);
void		mt_init_attach_list(Attach_list *, Frame, Canvas, Panel);
void		mt_destroy_attach_nodes(Attach_list *);
void		mt_destroy_attach_list(Attach_list *);
void		mt_destroy_attach_node(Attach_node *);
Attach_node	*mt_find_attach_node(Attach_list *al, short x, short y);
void		mt_set_display_xgap(Attach_list *);
long		mt_set_attach_display_xys(Attach_list *, long width);
void		mt_traverse_attach_list(Attach_list *, Function call_back,
			long arg);
Attach_node	*mt_find_last_deleted(Attach_list *);
Attach_node	*mt_get_next_selected_node(Attach_list *, Attach_node *);
long		mt_attach_list_size(Attach_list *, int selected_only);
int		mt_nattachments(Attach_list *, int selected_only);
int		mt_set_attach_display_xgap(Attach_list *);
void		mt_add_pending_attachment(Attach_list *, Attach_node *);
Attach_node	*mt_delete_pending_attachment(Attach_list *, Attach_node *);
void		mt_destroy_pending_attachments(Attach_list *);



/* functions defined in attach_panel.c */
Panel_item	mt_get_attach_msg_item(Panel);
void		mt_attach_footer_message(Attach_list *);
void		mt_attach_panel_message(Attach_list *);
void		mt_show_attach_list(Attach_list *, int show);
int		mt_attach_list_visible(Attach_list *);
void		mt_layout_attach_panes(Attach_list *, Xv_Window);
void		mt_update_attach_popups(Attach_list *);
long		mt_attach_height(Attach_list *);




/* functions defined in attach_ce.c */
int		mt_init_ce(void);
char		*mt_get_encode(char *compression_token, struct attach *);
char		*mt_get_decode(char *compression_token, struct attach *);
char		*mt_get_data_type(char *label, void *buf, int bufsize);
char		*mt_get_compression_type(char *type);
char		*mt_get_attachment_name(char *type);
struct attach	*mt_create_at(u_char *file, u_char *label, int tmpfile);
Attach_node	*mt_init_attach_node(Attach_list *, Attach_node *,
			struct attach *, struct stat *);



#endif /* !_mt_attach_h */
