#ident "@(#)reply.c 3.29 96/12/02 Copyr 1987 Sun Micro"

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
 * Mailtool - managing the message composition window
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/param.h>
#ifdef SVR4
#include <sys/kbd.h> 
#include <sys/kbio.h> 
#else 
#include <sundev/kbd.h>
#endif SVR4
#include <xview/panel.h>
#include <xview/text.h>
#include <xview/scrollbar.h>
#include <xview/xview.h>
#include <xview/font.h>
#include <xview/cms.h>
#include <xview/notice.h>
#include <xview/svrimage.h>

#include <xview/sel_svc.h>
#include <xview/sel_attrs.h>
#include <xview/dragdrop.h>
#include <X11/X.h>

#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "ds_popup.h"
#include "instrument.h"
#include "create_panels.h"
#include "mle.h"
#include "editor.h"
#include "../maillib/ck_strings.h"
#include "attach.h"
#include "destruct.h"
#include "cmds.h"
#include "mail_dstt.h"

#define DEBUG_FLAG mt_debugging
#include "debug.h"

/*
 * The 3 attachment popups' FRAME_CMD's KEY_DATA keys
 */
#define IS_POPUP_KEY    100

extern int	mt_tool_width;
struct reply_panel_data * mt_create_new_compose_frame();
static void	create_header_field_menu();
Menu		mt_gen_template_menu();
void		mt_toggle_field();
void		mt_layout_compose_window();

static Notify_value 	mt_destroy_compose_proc();
static Notify_value	mt_save_dead_letter();
static Notify_value 	mt_force_save_dead_letter();
static Notify_value 	mt_replysw_event_proc();
static void		mt_set_compose_focus();
static int 	mt_get_kbd_process(); 
static void	dup_menu_defaults();
extern int	mt_compose_frame_layout_proc();
extern Panel	mt_reply_panel;

int	REPLY_KEY_DATA = NULL;
int	TEXTSW_VIEW_KEY_DATA = NULL;
int	ATTACH_TYPE_KEY_DATA = NULL;

static char *add_string;
static char *delete_string;

/* Key data for header text fields and menu items */
struct header_field_data {
	Menu_item	hfd_mi;		/* Header Menu Item */
	Panel_item	hfd_fillin;	/* Text field in compose panel */
	char 		*hfd_defvalue;	/* Default value for text field */
	int		hfd_tmpfield;	/* TRUE to hide field on clear */
};


/* local routine definitions */
static void do_cancel(struct reply_panel_data *ptr);





#define MT_HFD_KEY hfd_key()

static int
hfd_key()
{
	static key;

	if (! key) {
		key = xv_unique_key();
	}
	return (key);
}




static struct header_field_data *
create_hfd()

{
	struct header_field_data	*hfd;

	/* Allocate and init key data for header field fillins.  Used
	 * By header menu items and header text fields
	 */
	hfd = (struct header_field_data	*)
		ck_malloc(sizeof(struct header_field_data));
	memset((char *)hfd, NULL, sizeof(struct header_field_data));

	return(hfd);
}


static
clear_menu(menu)

	Menu	menu;

{
	Menu_item	menu_item;
	register int	n;
	Panel_item      panel_item;
        struct header_field_data        *hfd;


	/*
	 * menu's CLIENT DATA points to rpd struct, so we don't want
	 * to free that or reset it to NULL, since we keep the menu.
	 */
	/*
	 * Loop through menu and remove all menu items.  
	 * Hide panel items (leave them, in case they
	 * are still visible on compose window, don't
	 * have to recreate them right away).
	 * Destroy menu_items, hfds.
	 */
	/*
	 * We ignore first two menu items (Aliases, Bcc).
	 * Bcc has special help keyword, and has no hfd.
	 *
	 * If we ever destroy panel_items in future,
	 * don't use xv_destroy_safe, because when we destroy compose
	 * fillins later they remain when PANEL_EACH_ITEM is called.
	 */
	for (n = (int)xv_get(menu, MENU_NITEMS); n > 2; n--) {
		menu_item = xv_get(menu, MENU_NTH_ITEM, n);

		/* 
		 * Instead of using key_data_remove_proc,
		 * we free the KEY DATA hfd here instead.
		 */
		panel_item = (Panel_item)xv_get(menu_item, MENU_CLIENT_DATA);
		if (panel_item) {
			xv_set(panel_item, 
				PANEL_VALUE, "", 
                		XV_KEY_DATA, MT_HFD_KEY, NULL,
				0);
		}

		/* Finished with panel items, now menuitems, hfds */

        	hfd = (struct header_field_data *)xv_get(menu_item,
                        XV_KEY_DATA, MT_HFD_KEY);
   		if (hfd) {
   			if (hfd->hfd_defvalue)
                		free(hfd->hfd_defvalue);
        		free(hfd);
		}
		xv_set(menu_item, MENU_CLIENT_DATA, NULL, 0);
		xv_set(menu, MENU_REMOVE_ITEM, menu_item, 0);
		xv_destroy(menu_item);
	}
}


struct reply_panel_data *
mt_get_compose_frame(hd)

        struct header_data *hd;
{
	int fd;
	struct reply_panel_data *ptr, *last_ptr, *value;
        static char *old_addi_fields = NULL;
        char *addi_fields = NULL;

	/*
	 * loop through all replysw's looking for one that is displayed and
	 * not in use. If none exist, take first one that is not in use and
	 * display it. If none exist, create a new one. 
	 */
	value = last_ptr = NULL;

	ptr = MT_RPD_LIST(hd);

	/* scan through the list */

	while (ptr) {
		if (!ptr->inuse) {
			if (mt_compose_window_inuse(ptr)) {
				/*
				 * without clicking compose or reply, user
				 * has loaded a file into composition window
				 * or has entered text directly. Only occurs
				 * in popup windows, since ptr->inuse is
				 * always true for split composition window,
				 * because only way to open the window is via
				 * compose or reply button. 
				 */
				ptr->inuse = TRUE;
			} else if (xv_get(ptr->replysw, XV_SHOW) &&
				xv_get(ptr->frame, XV_SHOW) &&
				!xv_get(ptr->frame, FRAME_CLOSED) )
			{
				value = ptr;/* visible, use this */
				break;
			}
			else if (value == NULL)
			/*
			 * this one not in use, but not displayed. remember
			 * it but look further.  
			 */
				value = ptr;
		}
		last_ptr = ptr;
		ptr = ptr->next_ptr;
	}

	/* did we find a reusable composition frame? */

	if (!value) {

		/* If we didn't find any composition frames that we thought 
		   we could reuse, allocate a new one. */

		value = mt_create_new_compose_frame(hd);

		if (MT_RPD_LIST(hd) != NULL) {
			/* Make sure this window has the same menu defaults
			* as the first compose window
			 */
			dup_menu_defaults(
				MT_RPD_LIST(hd)->reply_panel,
				value->reply_panel);
		}

		/* Add this window to the list hung off of the CLIENT_DATA
		   of the main control panel */

		if (last_ptr)
			last_ptr->next_ptr = value;
		else
			MT_RPD_LIST(hd) = value;
	}
	else
	{
		/*
		 * Apparently, we create a new compose window
		 * at mailtool startup, so we get here
		 * first time compose window shows.
		 * We need to force a create_header_field if
		 * first time in case user changed anything.
		 */
        	/*
         	 * We use this variable to check if additionalfields variable
         	 * changed later on
         	 */
        	if (!old_addi_fields) {
                        clear_menu(value->header_menu);
                        create_header_field_menu(value);

                	old_addi_fields = mt_value("additionalfields");
                	if (!old_addi_fields)
                    		old_addi_fields = strdup("");
                	else
                    		old_addi_fields = strdup(old_addi_fields);
        	}

                /*
                 * Check and update the Include header fields.
                 * Check and update if show attachments.
                 * Layout the compose panel and make sure that the Header
                 * menu is up to date
                 * Update lines of text (popuplines)
                 */
                addi_fields = mt_value("additionalfields");
                if (!addi_fields)
                        addi_fields = strdup("");

                if ( strcmp( old_addi_fields, addi_fields)) {
			/* 
			 * We cannot call mt_clear_menu because we
			 * need to manually reset or destroy the panel_items
			 * corresponding to each menu item.
			 * So we created our own clear_menu.
			 */
                        clear_menu(value->header_menu);
                        create_header_field_menu(value);
                        free (old_addi_fields);
                        old_addi_fields = strdup(addi_fields);
                }

                mt_show_attach_list(value->rpd_al,
				    !mt_value("hideattachments"));
		mt_layout_compose_window(value);

		/*
		 * Usually redundant, sometimes necessary (like if the
		 * compose window contains a deleted attachment)
		 */
		mt_clear_compose_window(value);
	}

	/* Record checkbox is only visible if "record" is set */
	if (mt_value("record")) {
		(void)xv_set(value->rpd_record_item, XV_SHOW, TRUE, 0);
	   	if (mt_value("dontlogmessages"))
		   (void)xv_set(value->rpd_record_item, PANEL_TOGGLE_VALUE, 0, 0, 0);
	   	else
		   (void)xv_set(value->rpd_record_item, PANEL_TOGGLE_VALUE, 0, 1, 0);
	} else {
		(void)xv_set(value->rpd_record_item, XV_SHOW, FALSE, 0);
	}

	return (value);
}


static Notify_value
mt_compose_event_proc(client, event, arg, when)
	Notify_client   client;
	Event          *event;
	Notify_arg      arg;
	Notify_event_type when;
{
	struct	reply_panel_data *ptr;
	char	icon_label[20];
	Notify_value	return_value;

	Attach_node	*an;
	extern	void	mt_iconify_attach_node(Attach_list *al, int ICONIFY);


	return_value = notify_next_event_func(client, (Notify_event)event, arg, NOTIFY_SAFE);

	/*
	 * Get the compose frame's CLIENT_DATA, so we can extract the compose
	 * panel information.
	 */
	ptr = (struct reply_panel_data *)xv_get(client, WIN_CLIENT_DATA);

	if (event_action(event) == ACTION_CLOSE)
	{
		strcpy(icon_label, "To:");
		strncat(icon_label, (char *)
			xv_get(ptr->dest_fillin, PANEL_VALUE, 0),
			sizeof(icon_label) - 4);
		xv_set(ptr->composing_icon, XV_LABEL, icon_label, 0);
		mt_iconify_attach_node(ptr->rpd_al, TRUE);

	} else if (event_action(event) == ACTION_OPEN) {
		mt_iconify_attach_node(ptr->rpd_al, FALSE);

	} else if (event_action(event) == WIN_RESIZE) {
		/* Make sure this is not a synthetic resize event.  Synthetic
		 * resize events are generate when the window is moved
		 */
		if (event_xevent(event)->xconfigure.send_event == 0) {
			mt_layout_compose_window(ptr);
		}
	} else if (event_action(event) == WIN_REPAINT) {
		/* update input focus */
		if (ptr->rpd_focus_item) {
			mt_set_compose_focus(ptr, ptr->rpd_focus_item);
			ptr->rpd_focus_item = NULL;
		}

	}

	return(return_value);
}

void
mt_layout_compose_window(ptr)

	struct reply_panel_data *ptr;

{
	int		frame_h;
	int		height;


	frame_h = (int)xv_get(ptr->frame, XV_HEIGHT);

	mt_set_attach_canvas_height(ptr->rpd_al);
	height = mt_attach_height(ptr->rpd_al) +
		(int)xv_get(ptr->reply_panel, XV_HEIGHT);

	if ((height = frame_h - height) < 1) {
		height = 1;
	}

	(void)xv_set(ptr->replysw,
		XV_HEIGHT, height,
		0);

	mt_layout_attach_panes(ptr->rpd_al, ptr->replysw);

	mt_resize_header_fields(ptr);
}




struct reply_panel_data *
mt_create_new_compose_frame(hd)

        struct header_data *hd;
{
	Frame           frame;
	Frame           mtframe1;
	struct reply_panel_data *ptr;
	Rect		frame_rect, compose_rect;
	Attach_list	*list;

	mtframe1 = MT_FRAME(hd);

	/* allocate the base frame for the composition window */

	frame = xv_create(XV_NULL, FRAME_BASE,
		WIN_IS_CLIENT_PANE,
		XV_SHOW, 		FALSE,
		XV_WIDTH,	20,
		FRAME_SHOW_FOOTER,	TRUE,
		FRAME_SHOW_LABEL, TRUE,
		WIN_CMD_LINE,	-1,
		WIN_CMS,	(Cms)xv_get(mtframe1, WIN_CMS),
		WIN_CONSUME_EVENTS,	WIN_REPAINT,	NULL,
		FRAME_CLOSED,	FALSE,
		0);

	/* Bugid 1065418 and 1065454 */
	ptr = (struct reply_panel_data *)
		calloc(1, sizeof(struct reply_panel_data));

	if (frame == NULL || ptr == NULL) {
                /* STRING_EXTRACTION -
                 *
                 * The error message when we could not create a new compose
                 * window.
                 */
		mt_vs_warn(frame,
			gettext("Unable to create composition window."));

		if (ptr != NULL)
			free(ptr);

		return (NULL);
	}

	/* Make each rpd node point back to header_data, 
           for easy access to all sorts of stuff */
	ptr->hd = hd; 
	ptr->inuse = FALSE; 
	ptr->next_ptr = NULL;
	ptr->frame = frame;

	ptr->reply_msg = msg_methods.mm_create(0);

	ptr->composing_icon = xv_create(0,	ICON,
				ICON_IMAGE, compose_image,
				ICON_MASK_IMAGE, compose_image_mask,
				ICON_TRANSPARENT, TRUE,
				WIN_RETAINED,   TRUE,
				0);

	xv_set(ptr->composing_icon, XV_LABEL, name_mailtool, 0);
	xv_set(frame, 
		FRAME_ICON, ptr->composing_icon, 
		0);

        /* STRING_EXTRACTION -
         *
         * Compose Message is the title of the compose pane
         */
	mt_label_frame(frame, gettext("Compose Message"));

	/* set the destroy cleanup procedure */

	notify_interpose_destroy_func(frame, mt_destroy_compose_proc);
	notify_interpose_event_func(frame, mt_compose_event_proc, NOTIFY_SAFE);

	/* create the text subwindow for the composition window 
	 * We do this first so that we can easily fit the compose frame to
	 * the proper width
	 */
	mt_create_compose_textsw(ptr);
	window_fit_width(ptr->frame);

	/* create the control panel for the compostion window */
	mt_create_compose_panel(frame, ptr);

#ifdef EDITOR
	ptr->rpd_ttysw = NULL;
	ptr->rpd_editor_inuse = FALSE;
#endif

	(void) xv_set(ptr->reply_panel, XV_SHOW, TRUE, 0);

	(void) xv_set(ptr->composing_icon,
			ICON_IMAGE, compose_image,
			ICON_MASK_IMAGE, compose_image_mask,
			XV_LABEL, name_mailtool, 
			0);

	xv_set(ptr->frame, FRAME_ICON, ptr->composing_icon, 0);

	ptr->rpd_al = mt_create_attach_list(frame, frame, ptr->reply_msg,
		mtframe1);
	mt_show_attach_list(ptr->rpd_al, !mt_value("hideattachments"));

	(void) xv_set(ptr->replysw,
		WIN_X, 0,
		WIN_BELOW, ptr->reply_panel,
		XV_WIDTH,	WIN_EXTEND_TO_EDGE,
		XV_SHOW, TRUE,
		0);

	/* Should do this with xv_set, not in xv_create */
	(void) xv_set(ptr->replysw,
        	WIN_ROWS, mt_popuplines,
        	0);

	ds_position_popup(mtframe1, frame, DS_POPUP_LOR);

	window_fit(ptr->frame);

	/* Add interpose func to allow checkpointing of dead.letter */
	if (mt_value("save")) {
		ptr->replysw_view = textsw_first(ptr->replysw);
		if (! TEXTSW_VIEW_KEY_DATA) {
			TEXTSW_VIEW_KEY_DATA = xv_unique_key();
		}
		xv_set(ptr->replysw_view, XV_KEY_DATA,
			TEXTSW_VIEW_KEY_DATA,  ptr,
			0);
		notify_interpose_event_func(ptr->replysw_view,
			mt_replysw_event_proc, NOTIFY_SAFE);
	}

	return (ptr);
}

Menu_item
mt_template_proc(mi, op)

Menu_item	mi;
Menu_generate	op;

{
	char	*c_ptr;

	/* determine all kinds of reasons why the template strings might 
	   not be legal, and refuse to display the menu if it would have 
	   no contents */

	if ((c_ptr = mt_value("templates")) == NULL)
		c_ptr = "calendar:$OPENWINHOME/share/xnews/client/templates/calendar.tpl";
	switch(op) {

		case MENU_DISPLAY:
			if (!c_ptr)
			{
				xv_set(mi, MENU_INACTIVE, TRUE, 0);
				break;
			}

			if (!*c_ptr)
			{
				xv_set(mi, MENU_INACTIVE, TRUE, 0);
				break;
			}

			if (!strchr(c_ptr, ':'))
			{
				xv_set(mi, MENU_INACTIVE, TRUE, 0);
				break;
			}

			xv_set(mi, MENU_INACTIVE, FALSE, 0);
			break;

	}

	return(mi);

}

static Menu
create_include_pulldown(ptr)
	struct reply_panel_data *ptr;

{
	Menu	menu;
	Menu	submenu;
	Menu_item	item;
	char	*s;
	int	toggle_attachments();
	Menu_item	toggle_attachments_gen_proc();

	if (! REPLY_KEY_DATA) {
		REPLY_KEY_DATA = xv_unique_key();
	}

	menu = xv_create(XV_NULL, MENU, 0);

        /* STRING_EXTRACTION -
         *
         * Bracketed, Indented, and Templates are all menus in the
         * compose/Include menu.  Bracketed puts a "Begin Included Message"
         * and "End Included Message" line before and after the included
         * mail message.  Indented puts the indent string (by default ">")
         * before each included line.  Templates is a submenu that lists
         * the user specified template files.
         */
	item = xv_create(XV_NULL, MENUITEM, 
		MENU_ACTION_ITEM, gettext("Bracketed"), mt_include_proc, 
		MENU_CLIENT_DATA, 0,
		XV_HELP_DATA,	"mailtool:IncludeBracketedItem",
		0);

	xv_set(menu, 
		MENU_APPEND_ITEM, item, 
		0);

	item = xv_create(XV_NULL, MENUITEM, 
		MENU_ACTION_ITEM, gettext("Indented"), mt_include_proc, 
		MENU_CLIENT_DATA, 1, 
		XV_HELP_DATA,	"mailtool:IncludeIndentedItem",
		0);

	xv_set(menu, 
		MENU_APPEND_ITEM, item, 
		0);

	mt_menu_append_blank_item(menu);

	submenu = xv_create(XV_NULL, MENU, 0);
	
	xv_set(submenu,
		MENU_GEN_PROC, mt_gen_template_menu,
		XV_KEY_DATA, REPLY_KEY_DATA, ptr,
		0);

	item = xv_create(XV_NULL, MENUITEM, 
		MENU_PULLRIGHT_ITEM, gettext("Templates"), submenu, 
		MENU_GEN_PROC, mt_template_proc,
		XV_HELP_DATA, "mailtool:TemplateMenu",
		0);

	xv_set(menu, 
		MENU_APPEND_ITEM, item, 
		0);

	item = xv_create(XV_NULL, MENUITEM, 
		MENU_STRING,	"",
		MENU_ACTION_PROC, toggle_attachments,
		MENU_GEN_PROC,	toggle_attachments_gen_proc,
		/* Screws up menu display
		MENU_RELEASE_IMAGE,
		*/
		XV_HELP_DATA, "mailtool:IncludeAtachments",
		0);

	xv_set(menu, 
		MENU_APPEND_ITEM, item, 
		0);

	return(menu);
}

static
toggle_attachments(menu, menu_item)

	Menu	menu;
	Menu_item	menu_item;

{
	Panel_item	item;
	struct reply_panel_data	*ptr;
	int		show;

	item = (Panel_item)xv_get(menu, MENU_CLIENT_DATA);
	ptr = (struct reply_panel_data *)xv_get(
		(Panel)xv_get(item, XV_OWNER), WIN_CLIENT_DATA);

	/* Toggle list */
	show = !mt_attach_list_visible(ptr->rpd_al);
	mt_show_attach_list(ptr->rpd_al, show);
	mt_layout_compose_window(ptr);
}

static Menu_item
toggle_attachments_gen_proc(mi, op)

	Menu_item	mi;
	Menu_generate	op;

{
	Menu	menu;
	Panel_item	button;
	struct reply_panel_data	*rpd;
	Attach_list	*al;

	switch (op) {

	case MENU_DISPLAY:
	case MENU_NOTIFY:
		menu = (Menu)xv_get(mi, MENU_PARENT);
		button = (Panel_item)xv_get(menu, MENU_CLIENT_DATA);
		rpd = (struct reply_panel_data *)xv_get(
			(Panel)xv_get(button, XV_OWNER), WIN_CLIENT_DATA);

		/* STRING_EXTRACTION -
	 	 *
	 	 * Control whether the attachment pane shows up in the
		 * compose window.
	 	 */
		if ((int)mt_attach_list_visible(rpd->rpd_al)) {
			(void)xv_set(mi, MENU_STRING,
					gettext("Hide Attachments"), 0);
		} else {
			(void)xv_set(mi, MENU_STRING,
					gettext("Show Attachments"), 0);
		}
		break;
	}

	return(mi);
}



static Menu
create_deliver_pulldown()

{
	Menu		menu;
	Menu_item	item;
#ifdef PEM
	Menu		submenu;
#endif PEM

	menu = xv_create(XV_NULL, MENU, 0);

        /* STRING_EXTRACTION -
         *
         * The deliver menu.  It has four items: quit window, close window,
         * clear message, leave message intact.  All of these deliver the
         * message.
         */

	item = xv_create(XV_NULL, MENUITEM, 
		MENU_ACTION_ITEM, 	gettext("Quit window"), mt_deliver_proc, 
		MENU_CLIENT_DATA,	0,
		XV_HELP_DATA,		"mailtool:DeliverTakedown",
		0),

	xv_set(menu, 
		MENU_APPEND_ITEM, item, 
		0);

	item = xv_create(XV_NULL, MENUITEM, 
		MENU_ACTION_ITEM, 	gettext("Close window"),
					mt_deliver_proc, 
		MENU_CLIENT_DATA,	1, 
		XV_HELP_DATA,		"mailtool:DeliverClose",
		0),

	xv_set(menu, 
		MENU_APPEND_ITEM, item, 
		0);

	item = xv_create(XV_NULL, MENUITEM, 
		MENU_ACTION_ITEM, 	gettext("Clear message"),
					mt_deliver_proc, 
		MENU_CLIENT_DATA, 	2, 
		XV_HELP_DATA,		"mailtool:DeliverClearItem",
		0),

	xv_set(menu, 
		MENU_APPEND_ITEM, item, 
		0);

   	item = xv_create(XV_NULL, MENUITEM, 
		MENU_ACTION_ITEM, 	gettext("Leave message intact"),
					mt_deliver_proc, 
		MENU_CLIENT_DATA, 	3, 
		XV_HELP_DATA,		"mailtool:DeliverLeaveMessageItem",
		0),

	xv_set(menu, 
		MENU_APPEND_ITEM, item, 
		0);

#ifdef PEM
	item = xv_create(XV_NULL, MENUITEM,
		MENU_ACTION_ITEM,	gettext("with Integrity"), 
					mt_pem_proc,
		MENU_CLIENT_DATA, 	0, 
		0);
	submenu = xv_create(XV_NULL, MENU, 0);

	xv_set(submenu,
		MENU_APPEND_ITEM,	item,
		0);
	
	item = xv_create(XV_NULL, MENUITEM,
		MENU_ACTION_ITEM,	gettext("with Confidentiality"),
					mt_pem_proc,
		MENU_CLIENT_DATA,	1,
		0);

	xv_set(submenu,
		MENU_APPEND_ITEM,	item,
		0);

	item = xv_create(XV_NULL, MENUITEM,
		MENU_ACTION_ITEM,		gettext("with Privacy"),
					mt_pem_proc,
		MENU_CLIENT_DATA,	2,
		0);

	xv_set(submenu,
		MENU_APPEND_ITEM,	item,
		0);

	item = xv_create(XV_NULL, MENUITEM,
		MENU_PULLRIGHT_ITEM,	gettext("PEM"), submenu,
		0);
	
	xv_set(menu,
		MENU_APPEND_ITEM,	item,
		0);
#endif PEM

	return(menu);
}

Panel_setting
mt_compose_text_notify_proc(item, event)

Panel_item	item;
Event		*event;

{
	Panel	last_text_field();
	Panel	parent_panel = xv_get(item, PANEL_PARENT_PANEL);
	struct reply_panel_data	*ptr = (struct reply_panel_data *)
		xv_get(parent_panel, WIN_CLIENT_DATA);

	/*
	 * If we are in the last visible text field then advance the focus
	 * to the textsw.
	 */
	if (last_text_field(parent_panel) == item &&
						!event_shift_is_down(event))
		xv_set(ptr->replysw, WIN_SET_FOCUS, 0);
	else
		return(panel_text_notify(item, event));

	return(PANEL_NONE);
}

static Panel_item
last_text_field(panel)

	Panel	panel;
{
	Panel_item	item, last_item;
	struct panel_item_data	*p;

	/*
	 * Find the last visible text item in panel
	 */
	last_item = NULL;
	PANEL_EACH_ITEM(panel, item)
	{
		if ((Panel_item_type)xv_get(item, PANEL_ITEM_CLASS) ==
							PANEL_TEXT_ITEM &&
		     (int)xv_get(item, PANEL_SHOW_ITEM))
			last_item = item;
	}
	PANEL_END_EACH;

	return(last_item);
}

#ifdef	COMPOSE_ADDR

static void
mt_append_alias_proc(menu, item)
Menu menu;
Menu_item item;
{
	int		len;
	u_char		*nl;
	u_char		*names;
	u_char		*alias;
	u_char		*value;
	int		need_comma;
	Panel_item	dest_fillin;

	dest_fillin = (Panel_item) xv_get (menu, MENU_CLIENT_DATA);
	nl = names = (char *) xv_get (dest_fillin, PANEL_VALUE);
	while (isspace(*names))
		names++;
	if (*names == '\0')
		need_comma = 0;
	else
	{
		if ((value = strrchr (names, ',')) == NULL)
			need_comma = 2;
		else
		{
			/* check if it is a trailing comma */
			do {
				value++;
			} while (isspace (*value));
			if (*value != '\0')
				need_comma = 2;		/* no trailing comma */
			else
				need_comma = 0;
		}
	}
	alias = (char *) xv_get (item, MENU_STRING);
	value = ck_malloc (strlen(nl) + need_comma + strlen(alias) + 1);
	strcpy (value, nl);
	if (need_comma)
		strcat (value, ", ");
	strcat (value, alias);
	xv_set (dest_fillin, PANEL_VALUE, value, 0);
	ck_free (value);
}

static int
add_alias_items(key, value, menu)
char *key;
char *value;
Menu menu;
{
	Menu_item	item;

	item = xv_create (XV_NULL, MENUITEM,
			MENU_STRING, 		gettext(key),
			MENU_ACTION_PROC, 	mt_append_alias_proc,
			XV_HELP_DATA,		"mailtool:AliasItem",
			0);
	xv_set (menu, MENU_APPEND_ITEM, item, 0);
	return (0);
}

Menu
mt_create_aliases_menu()
{
	Menu	menu;

	menu = xv_create(XV_NULL, MENU, 0);
	alias_enumerate(add_alias_items, menu);

	if ((int) xv_get(menu, MENU_NITEMS) > 0)
		return (menu);

	xv_destroy_safe(menu);
	return (NULL);
}

#endif	COMPOSE_ADDR


#ifdef EDITOR
mt_editor_proc(menu, menu_item)

	Menu	menu;
	Menu_item	menu_item;

{
	struct reply_panel_data	*rpd;

	rpd = (struct reply_panel_data *)xv_get(menu, MENU_CLIENT_DATA);

	if (rpd->rpd_editor_inuse) {
		mt_stop_editor(rpd);
	} else {
		mt_start_editor(rpd);
	}
}
#endif

static void
mt_mclear_proc(menu, menu_item)

	Menu	menu;
	Menu_item	menu_item;

{
	struct reply_panel_data	*rpd;

	rpd = (struct reply_panel_data *)xv_get(menu, MENU_CLIENT_DATA);

	do_cancel(rpd);
	return;
}

static void
mt_clear_proc(item, ie)

	Panel_item		item;
	Event	*ie;

{
	Panel	panel;
	struct reply_panel_data	*rpd;

	panel = (Panel)xv_get(item, XV_OWNER);
	rpd = (struct reply_panel_data *)xv_get(panel, WIN_CLIENT_DATA);

	do_cancel(rpd);
	return;
}

static struct reply_panel_data *
mt_create_compose_panel(frame, ptr)
	Frame	frame;
	struct reply_panel_data *ptr;
{
	Panel           panel;
	Panel_item      item;
	Menu		menu;
	int		mt_attach_proc();

	panel = xv_create(frame, PANEL,
		XV_X,			0,
		XV_Y,			0,
		XV_WIDTH,		WIN_EXTEND_TO_EDGE,
		XV_SHOW, 		FALSE,
		XV_FONT, 		mt_font,
		XV_NAME,		name_Mail_Tool,
		PANEL_ITEM_Y_GAP, 	3,
		0);

        /* STRING_EXTRACTION -
         *
         * We are making the compose panel. 
         */
	if (panel == NULL) {
		mt_vs_warn(frame, gettext("Unable to create reply panel,"));
		return (ptr);
	}

	/* set up this panel so that the spacing of the items isn't so yucchy */

	ptr->reply_panel = panel;


	item = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING, gettext("Include"),
		PANEL_ITEM_MENU, menu = create_include_pulldown(ptr),
		XV_HELP_DATA, "mailtool:IncludeButton",
		0);

	/* set up the MENU_CLIENT_DATA of the menu with the panel item 
	 * handle, so we can get back at the panel handle later 
         * (in the action proc for the menu)
	 */
	xv_set(menu, MENU_CLIENT_DATA, item, 0);

	ptr->deliver_item = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING, gettext("Deliver"),
		PANEL_ITEM_MENU, menu = create_deliver_pulldown(),
		XV_HELP_DATA, "mailtool:DeliverButton",
		0);

	/* set up the MENU_CLIENT_DATA of the menu with the panel item 
	 * handle, so we can get back at the panel handle later 
         * (in the action proc for the menu)
	 */
	xv_set(menu, MENU_CLIENT_DATA, ptr->deliver_item, 0);

	/*
	 * Create the Headers button menu.
	 * The menu for Headers is created after the fillin fields are set
	 * up.
	 */
	ptr->header_menu = xv_create(XV_NULL, MENU, 0);
	ptr->address_item = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING, gettext("Header"),
		PANEL_ITEM_MENU, ptr->header_menu,
		XV_HELP_DATA, "mailtool:Headers",
		0);


	ptr->clear_item = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING, gettext("Clear"),
		PANEL_NOTIFY_PROC, mt_clear_proc,
		XV_HELP_DATA, "mailtool:ClearHelpButton",
		0);

#ifdef EDITOR
	menu = (Menu)xv_create(XV_NULL, MENU,
		MENU_CLIENT_DATA,	ptr,
		MENU_ITEM,
			MENU_ACTION_ITEM, gettext("Clear"),
						mt_mclear_proc,
			XV_HELP_DATA, "mailtool:ClearHelpButton",
			0,
		MENU_ITEM,
			MENU_ACTION_ITEM, gettext("Custom Editor"),
						mt_editor_proc,
			XV_HELP_DATA,	"mailtool:ComposeEditor",
			0,
		0);

	ptr->edit_item = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING, gettext("Edit"),
		PANEL_ITEM_MENU, menu,
		PANEL_ITEM_X, 	(int)xv_get(ptr->clear_item, PANEL_ITEM_X),
		XV_HELP_DATA, "mailtool:EditButton",
		0);
#endif

	if (! ATTACH_TYPE_KEY_DATA) {
		ATTACH_TYPE_KEY_DATA = xv_unique_key();
	}

	menu = (Menu)xv_create(XV_NULL, MENU,
		MENU_CLIENT_DATA,	ptr,
		MENU_ITEM,
 			MENU_ACTION_ITEM, gettext("Voice..."), mt_attach_proc,
			XV_HELP_DATA,	"mailtool:PullRightVoice",
			XV_KEY_DATA,	ATTACH_TYPE_KEY_DATA, MT_CE_AUDIO_TYPE,
			0,
		MENU_ITEM,
 			MENU_ACTION_ITEM, gettext("Appt..."), mt_attach_proc,
			XV_HELP_DATA,	"mailtool:PullRightAE",
			XV_KEY_DATA,	ATTACH_TYPE_KEY_DATA, MT_CE_AE_TYPE,
			0,
		0);

	item = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING, gettext("Attach"),
		PANEL_ITEM_MENU, menu,
		XV_HELP_DATA, "mailtool:AttachButton",
		0);

	mt_scoot_item_right(panel, item, 1);

	ptr->rpd_record_item = xv_create(panel, PANEL_CHECK_BOX,
		PANEL_LABEL_STRING,	"",
		PANEL_CHOICE_STRINGS, gettext("Log"), 0,
		XV_SHOW,		mt_value("record") ? TRUE : FALSE,
		XV_HELP_DATA,		"mailtool:ComposeRecord",
		0);

	mt_scoot_item_right(panel, ptr->rpd_record_item, 1);

#ifdef	COMPOSE_ADDR
	ptr->addr_menu = mt_create_aliases_menu ();
	if (ptr->addr_menu == NULL)
		ptr->to_item = NULL;
	else
	{
		ptr->to_item = xv_create(panel, PANEL_ABBREV_MENU_BUTTON,
				PANEL_ITEM_MENU,	ptr->addr_menu,
				PANEL_LABEL_STRING,	gettext("To:"),
				PANEL_LABEL_BOLD,	TRUE,
				XV_HELP_DATA,		"mailtool:ComposeAddr",
				0);
	}

	ptr->dest_fillin = xv_create(panel, PANEL_TEXT,
		PANEL_NEXT_ROW, -1,
		PANEL_LABEL_STRING, ptr->addr_menu ? "" : "To:",
		PANEL_VALUE, "",
		PANEL_VALUE_DISPLAY_LENGTH, 60,
		PANEL_VALUE_STORED_LENGTH, LINE_SIZE,
		PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
		PANEL_NOTIFY_STRING, 	"\t\r\n",
		PANEL_NOTIFY_PROC, 	mt_compose_text_notify_proc,
		XV_HELP_DATA,           "mailtool:ComposeTo",
		0);

	/* Tie the "To:" field to the menu.  Note, the menu is *not* sharable */
	if (ptr->addr_menu)
		xv_set(ptr->addr_menu, MENU_CLIENT_DATA, ptr->dest_fillin, 0);

#else
	ptr->dest_fillin = xv_create(panel, PANEL_TEXT,
		PANEL_NEXT_ROW, -1,
		PANEL_LABEL_STRING, "To:",
		PANEL_VALUE, "",
		PANEL_VALUE_DISPLAY_LENGTH, 60,
		PANEL_VALUE_STORED_LENGTH, LINE_SIZE,
		PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
		PANEL_NOTIFY_STRING, 	"\t\r\n",
		PANEL_NOTIFY_PROC, 	mt_compose_text_notify_proc,
		XV_HELP_DATA,           "mailtool:ComposeTo",
		0);
#endif	COMPOSE_ADDR

	ptr->base_fillin_y = (int) xv_get(ptr->dest_fillin, XV_Y);

	ptr->subject_fillin = xv_create(panel, PANEL_TEXT,
		PANEL_LABEL_STRING, "Subject:",
		PANEL_VALUE, "",
		PANEL_VALUE_DISPLAY_LENGTH, 60,
		PANEL_VALUE_STORED_LENGTH, LINE_SIZE / 2,
		PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
		PANEL_NOTIFY_STRING, 	"\t\r\n",
		PANEL_NOTIFY_PROC, 	mt_compose_text_notify_proc,
		XV_HELP_DATA,           "mailtool:ComposeSubject",
		0);

	ptr->incr_fillin_y = (int)
		xv_get(ptr->subject_fillin, XV_Y) - ptr->base_fillin_y;

	ptr->cc_fillin = xv_create(panel, PANEL_TEXT,
		PANEL_LABEL_STRING, "Cc:",
		PANEL_VALUE, "",
		PANEL_VALUE_DISPLAY_LENGTH, 60,
		PANEL_VALUE_STORED_LENGTH, LINE_SIZE,
		PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
		PANEL_NOTIFY_STRING, 	"\t\r\n",
		PANEL_NOTIFY_PROC, 	mt_compose_text_notify_proc,
		XV_HELP_DATA,           "mailtool:ComposeCc",
		0);

	ptr->bcc_fillin = xv_create(panel, PANEL_TEXT,
		PANEL_LABEL_STRING, strdup("Bcc:"),
		PANEL_VALUE, "",
		PANEL_VALUE_DISPLAY_LENGTH, 60,
		PANEL_VALUE_STORED_LENGTH, LINE_SIZE,
		PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
		PANEL_NOTIFY_STRING, 	"\t\r\n",
		PANEL_NOTIFY_PROC, 	mt_compose_text_notify_proc,
		PANEL_SHOW_ITEM,	FALSE,
		XV_HELP_DATA,           "mailtool:ComposeBcc",
		0);

	/* 
	 * Set the client data of the frame and panel.
	 */
	(void) xv_set(frame, WIN_CLIENT_DATA, ptr, 0);
	(void) xv_set(panel, WIN_CLIENT_DATA, ptr, 0);


	mt_layout_compose_panel(ptr);
	
	/*
	 * Set menu on Headers button
	 */
	create_header_field_menu(ptr);
	xv_set(ptr->address_item, PANEL_ITEM_MENU, ptr->header_menu,
		0);
	return (ptr);
}

static void
reply_window_notify_proc(view, attributes)
	Textsw		view;
	Attr_avlist	attributes;
{
	struct reply_panel_data *ptr;
	Textsw		textsw;
	Attr_avlist	attrs;

	/*
	 * katin: this textsw is *different* from the one
	 * that we used in textsw_insert().  We need to get
	 * the WIN_PARENT of this textsw to get back to the
	 * one that we know about
	 */
	textsw = xv_get(view, WIN_PARENT);
	ptr = (struct reply_panel_data *) xv_get(textsw, WIN_CLIENT_DATA);

	for (attrs = attributes; *attrs; attrs = attr_next(attrs)) 
	{
		switch ((Textsw_action) (*attrs)) {
		case TEXTSW_ACTION_EDITED_MEMORY:
		case TEXTSW_ACTION_EDITED_FILE:
			/* mark this window has having been changed */
			DP(( "reply_window_notify_proc/EDITED_%s\n",
				(*attrs == TEXTSW_ACTION_EDITED_MEMORY)?
				"MEMORY" : "FILE" ));
			ptr->rpd_modified = TRUE;
			ptr->rpd_checkpt = TRUE;
			break;

		case TEXTSW_ACTION_USING_MEMORY:
			/* mark this window has having been changed */
			DP(( "reply_window_notify_proc/USING_MEMORY\n"));
			ptr->rpd_modified = TRUE;
			ptr->rpd_checkpt = TRUE;
			break;

		case TEXTSW_ACTION_LOADED_FILE:
			/* mark this window has having been changed */
			DP(( "reply_window_notify_proc/LOADED_FILE\n"));
			ptr->rpd_modified = TRUE;
			ptr->rpd_checkpt = TRUE;
			break;
		}
	}

	(*ptr->rpd_old_notify_proc)(view, attributes);
}

static
mt_create_dead_letter (rpd)
	struct reply_panel_data *rpd;
{
	char	*s;
	int	fd;
	int	inuse;
	int	count = 0;
	char deadletter[MAXPATHLEN];
	struct reply_panel_data *ptr;

	s = mt_value("DEAD");
	if (s == NULL || *s == '\0')
		s = "~/dead.letter";
	ds_expand_pathname(s, deadletter);
	s = strchr(deadletter, '\0');

	/* Find a dead.letter that is not in-use.  ZZZ: this mechanism does
	 * not work well if multiple mailtool are running.  The later one
	 * will overwrite the previous dead.letter.
	 */
	while ((fd = open(deadletter, O_CREAT|O_EXCL|O_WRONLY, 0600)) < 0)
	{
		if (errno != EEXIST)
			return(errno);

		inuse = FALSE;
		for (ptr = MT_RPD_LIST(rpd->hd); ptr != NULL;
		     ptr = ptr->next_ptr)
		{
			if ((ptr->rpd_dead_letter != NULL) &&
			    (strcmp(ptr->rpd_dead_letter, deadletter) == 0))
			{
				/* This dead.letter is being used */
				inuse = TRUE;
				break;
			}
		}

		/* If the dead.letter exists but not inuse, then use it */
		if (!inuse)
			break;

		*s = '\0';
		sprintf(s, ".%d", ++count);
	}

	/* A new dead.letter is just created. */
	close(fd);

	/* Save the dead.letter name and the textsw log file.  Initialize the
	 * flag that not to remove the dead.letter in mt_remove_dead_letter().
	 * This flag is set to TRUE when the message is delivered successfully
	 * and the compose window is being destroyed.
	 */
	rpd->rpd_dead_letter = ck_strdup(deadletter);
	rpd->rpd_rmdead = FALSE;

	DP(("mt_create_dead_letter: rpd %x, rpd_dead_letter %x (s)\n",
		rpd, rpd->rpd_dead_letter,
		rpd->rpd_dead_letter ? rpd->rpd_dead_letter : "<NULL>"));

	return (0);
}

static void
mt_remove_dead_letter (rpd)
	struct reply_panel_data *rpd;
{
	struct stat buf;

	DP(("mt_remove_dead_letter: freeing rpd %x dead_letter %x/%s\n",
		rpd, rpd->rpd_dead_letter,
		rpd->rpd_dead_letter ? rpd->rpd_dead_letter:"<NULL>"));

	if (rpd->rpd_dead_letter)
	{
		/*
		 * Remove the dead.letter after msg is delivered successfully
		 * or the dead.letter is empty.
		 */
		if (rpd->rpd_rmdead || ((stat(rpd->rpd_dead_letter, &buf) == 0)
		    && (buf.st_size == 0)))
		{
			unlink(rpd->rpd_dead_letter);
		}
		ck_free(rpd->rpd_dead_letter);
		rpd->rpd_dead_letter = NULL;
	}

	/* Turn off the itimer */
	notify_set_itimer_func((Notify_client) rpd, NOTIFY_FUNC_NULL,
				ITIMER_REAL, NULL, NULL);
	notify_set_itimer_func((Notify_client) rpd->replysw_view,
				NOTIFY_FUNC_NULL, ITIMER_REAL, NULL, NULL);
	rpd->itimer_started = FALSE;
}

static Notify_value
mt_save_dead_letter (rpd, which)
struct reply_panel_data *rpd;
int			which;
{
	FILE	*fp;
	char	buf[BUFSIZ];

	DP(("mt_save_dead_letter: rpd %x, rpd_dead_letter %x (%s)\n",
		rpd, rpd->rpd_dead_letter,
		rpd->rpd_dead_letter ? rpd->rpd_dead_letter : "<NULL>"));

	/* The composite window is not modified (this flag won't be reset
	 * until the window is clear because bugid# 1085874), no need to
	 * save any changes.
	 */
	if (!rpd->rpd_checkpt)
		return (NOTIFY_DONE);

	/* If the dead letter is never created, create it. */
	if (rpd->rpd_dead_letter == NULL)
	{
		if (mt_create_dead_letter(rpd) != 0)
		{
			perror("mailtool: Can't create dead letter");
			return(NOTIFY_DONE);
		}
	}

	/* We have to remove the existing dead letter because if the dead
	 * letter is the loaded file in textsw; it will confuse the textsw.
	 * Unlinking the file will create problem if it is NFS mounted.
	 */
	unlink(rpd->rpd_dead_letter);

	/* Bugid 1094764: not to use textsw_store_file() because it brings
	 * up an alert when it cannot store the file.  It is not clear why
	 * it fails; it maybe a timing problem with itimer.  Note, we are not
	 * reporting any error because we are in the middle of a notify proc.
	 */
	fp = fopen(rpd->rpd_dead_letter, "w");
	if (fp != NULL)
	{
		int	len, size;
		Textsw_index start = 0;

		fchmod(fileno(fp), 0600);
		len = (int) xv_get(rpd->replysw, TEXTSW_LENGTH);
		while ((size = MIN(len, sizeof(buf))) > 0)
		{
			/* we use the undocumented attribite TEXTSW_CONTENTS_NO_COMMIT
			 * because TEXTSW_CONTENTS forces a commit of pending text
			 * from the input manager (in asian locales).
			 *
			 * Alas, there is no public way to do this, we are using
			 * a private attribute...
			 */
#if ! defined(OW_I18N)
			start = xv_get(rpd->replysw, TEXTSW_CONTENTS,
				start, buf, size);
#else
			/* this hasn't gurgled into the build yet, so we allow
			 * a special option to work around its lack...
			 */
			start = xv_get(rpd->replysw, TEXTSW_CONTENTS_NO_COMMIT,
				start, buf, size);
#endif
			if (start < 0)
				break;
			if (fwrite(buf, size, 1, fp) == 0)
				break;
			len -= size;
		}
		fclose(fp);
	}
	notify_set_itimer_func((Notify_client) rpd, NOTIFY_FUNC_NULL,
			ITIMER_REAL, NULL, NULL);
	notify_set_itimer_func((Notify_client) rpd->replysw_view,
			NOTIFY_FUNC_NULL, ITIMER_REAL, NULL, NULL);
	rpd->itimer_started = FALSE;
	return(NOTIFY_DONE);
}

/* The maximum time between saving checkpoints has been reached without
 * saving to dead.letter, so we want to force the compose window to be
 * saved.
 */

static Notify_value
mt_force_save_dead_letter(replysw, which)
Textsw			replysw;
int			which;
{
	struct reply_panel_data *ptr;

	ptr = (struct reply_panel_data *) xv_get(replysw,
			XV_KEY_DATA, TEXTSW_VIEW_KEY_DATA);
	return(mt_save_dead_letter(ptr, which));
}

static Notify_value
mt_replysw_event_proc(replysw_view, event, arg, type)
Textsw replysw_view;
Event *event;
Notify_arg arg;
Notify_event_type type;
{
	static struct itimerval itimer={ 30,0,30,0 }; /* checkpoint every 30s */
	static struct itimerval itimermax={ 0,0,1800,0 }; /* chkpt every 30m */
	struct reply_panel_data *ptr;

	switch(event_action(event)) {
                case KBD_DONE:          /* Lost keyboard focus */
                case LOC_WINENTER:      /* Pointer entered window */
                case LOC_WINEXIT:       /* Pointer exited window */
                case ACTION_CLOSE:      /* Close the window */
                case ACTION_OPEN:       /* Open the window */
                case ACTION_HELP:       /* help key */
                case SHIFT_RIGHT:       /* Shift, Ctrl, and Meta keys */
                case SHIFT_LEFT:
                case SHIFT_LEFTCTRL:
                case SHIFT_RIGHTCTRL:
                case SHIFT_META:
                case SHIFT_ALT:
                case WIN_RESIZE:        /* Resize event */
                case WIN_REPAINT:       /* Repaint event */
                case WIN_VISIBILITY_NOTIFY: /* Make window visible */
                        /* None of the above events modify the textsw */
                        break;
                default:
                        /* Assume that any other events may have modified
                         * the textsw, so we want to checkpoint dead.letter.
                         * Set the checkpoint to 30 seconds from now.
                         */
                        ptr = (struct reply_panel_data *) xv_get(replysw_view,
                                        XV_KEY_DATA, TEXTSW_VIEW_KEY_DATA);
                        notify_set_itimer_func((Notify_client) ptr,
                                (Notify_func) mt_save_dead_letter,
                                ITIMER_REAL, &itimer,
                                (struct itimerval *) NULL);
                        if (! ptr->itimer_started) {
                                notify_set_itimer_func(
                                    (Notify_client)replysw_view,
                                    (Notify_func) mt_force_save_dead_letter,
                                    ITIMER_REAL, &itimermax,
                                    (struct itimerval *) NULL);
                                ptr->itimer_started = TRUE;
                        }
        }
        return(notify_next_event_func(replysw_view,
                        (Notify_event)event, arg, type));

}


static void
mt_create_compose_textsw(ptr)
	struct reply_panel_data *ptr;
{

	ptr->replysw = xv_create(ptr->frame,
			TEXTSW,
			WIN_IS_CLIENT_PANE,
			/* ZZZ Right margins screw up WIN_COLUMNS as they are
			 * broken (1048791) -- remove this line when this bug
			 * is fixed
			 */
			XV_RIGHT_MARGIN,	0,
			WIN_COLUMNS, 	mt_tool_width,
			XV_SHOW, TRUE,
			TEXTSW_MEMORY_MAXIMUM,	 mt_memory_maximum,
			XV_HEIGHT, WIN_EXTEND_TO_EDGE,
			WIN_CLIENT_DATA, ptr,
/*
			WIN_ROWS, mt_popuplines* + 3* /,
*/
			0);

	/* If no line break action is specified, default to wrap at char.
	 * We override the standard default (wrap at word) since wrap at word
	 * encourages users to type long lines without newlines
	 */
	if (defaults_get_string("text.lineBreak",
	                        "Text.LineBreak", (char *)NULL) == NULL) {
		(void)xv_set(ptr->replysw,
			TEXTSW_LINE_BREAK_ACTION,	TEXTSW_WRAP_AT_CHAR,
			NULL);
	}

	if (ptr->replysw == NULL) {
		mt_vs_warn(ptr->frame, 
			gettext("Unable to create composition window"));
		return; 
	}

	ptr->rpd_old_notify_proc = (void (*)())xv_get(
		ptr->replysw, TEXTSW_NOTIFY_PROC);

	(void) xv_set(ptr->replysw,
		TEXTSW_NOTIFY_PROC, reply_window_notify_proc, 0);

}


mt_layout_compose_panel(ptr)

struct 	reply_panel_data	*ptr;

{
	register Panel_item	item, last_item;
	register int		current_y = ptr->base_fillin_y;
	register int		current_x;
	register int		value_x;
	register int		length;
	Rect			*item_rect;
	Xv_Font			pf = NULL;
	Font_string_dims	font_size;
	Panel_item_type		class;
	int			last_x;
	int			longest = 0;
	char			*string;


#ifdef EDITOR
	if (mt_value("enable_edit")) {
		(void)xv_set(ptr->clear_item, PANEL_SHOW_ITEM, FALSE, 0);
		(void)xv_set(ptr->edit_item, PANEL_SHOW_ITEM, TRUE, 0);
	} else {
		(void)xv_set(ptr->edit_item, PANEL_SHOW_ITEM, FALSE, 0);
		(void)xv_set(ptr->clear_item, PANEL_SHOW_ITEM, TRUE, 0);
	}
#endif

	/*
	 * Compute the longest label
	 */
	PANEL_EACH_ITEM(ptr->reply_panel, item)
	{
		if (xv_get(item, PANEL_SHOW_ITEM) &&
		    (((class = (Panel_item_type) xv_get(item,
				PANEL_ITEM_CLASS)) == PANEL_TEXT_ITEM) ||
		     (class == PANEL_ABBREV_MENU_BUTTON_ITEM)))
		{
			pf = (Xv_Font)xv_get(item, PANEL_LABEL_FONT);
			string = (char *)xv_get(item, PANEL_LABEL_STRING);
			(void)xv_get(pf, FONT_STRING_DIMS, string, &font_size);
			if (font_size.width > longest)  {
				longest = font_size.width;
			}
		}
	}
	PANEL_END_EACH;

	value_x = longest + 2 * (int)xv_get(pf, FONT_DEFAULT_CHAR_WIDTH);
	last_x = value_x;

	/* layout each item on the panel */
	PANEL_EACH_ITEM(ptr->reply_panel, item)
	{
		if (xv_get(item, PANEL_SHOW_ITEM))
		{
			class = (Panel_item_type)xv_get(item, PANEL_ITEM_CLASS);
			if ((class == PANEL_TEXT_ITEM) ||
			    (class == PANEL_ABBREV_MENU_BUTTON_ITEM))
			{
				string = (char *)xv_get(item,
						PANEL_LABEL_STRING);
				if (string == NULL || *string == '\0')
					current_x = last_x;
				else
					current_x = value_x;

				xv_set(item, 
					PANEL_VALUE_X, current_x,
					PANEL_ITEM_Y, current_y,
					0);

				if (class == PANEL_TEXT_ITEM)
					current_y += ptr->incr_fillin_y;
				else
				{
					item_rect = (Rect *)xv_get(item,
							PANEL_ITEM_RECT);
					last_x = item_rect->r_left +
					 	 item_rect->r_width + 4;
				}
				last_item = item;
			}
		}
	}
	PANEL_END_EACH;

	item_rect = (Rect *) xv_get(last_item, PANEL_ITEM_RECT);

	xv_set(ptr->reply_panel,
		XV_WIDTH, WIN_EXTEND_TO_EDGE,
		XV_HEIGHT, item_rect->r_top + item_rect->r_height + 5,
		0);

	mt_resize_header_fields(ptr);
}

mt_resize_header_fields(ptr)

struct 	reply_panel_data	*ptr;

{
	Panel_item	item;

	/*
	 * Set the display width of the Header fillin fields. 
	 */
	PANEL_EACH_ITEM(ptr->reply_panel, item) {
		if (xv_get(item, PANEL_SHOW_ITEM) &&
			(Panel_item_type)xv_get(item, PANEL_ITEM_CLASS) == 
							PANEL_TEXT_ITEM) {
			mt_resize_fillin(ptr->reply_panel, item);
		}
	}
	PANEL_END_EACH;
}

mt_compose_frame_layout_proc(frame)

Frame	frame;

{
	struct reply_panel_data *ptr;

	ptr = (struct reply_panel_data *) xv_get(frame, WIN_CLIENT_DATA);

	mt_layout_compose_panel(ptr);

	xv_set(ptr->reply_panel, WIN_Y, 0, 0);
	xv_set(ptr->replysw, 
		WIN_BELOW, 	ptr->reply_panel, 
		XV_WIDTH, 	WIN_EXTEND_TO_EDGE, 
		0);

	/* Will cause textsw to grow/shrink to absorb free space */
	mt_layout_compose_window(ptr);
}

/*
 * user is quitting a popup reply window. This takes care of removing the
 * window from the tools data structures 
 */
static Notify_value 
mt_destroy_compose_proc(client, status)
	Notify_client   client;
	Destroy_status  status;
{
	struct reply_panel_data *ptr, *prev;
	struct header_data	*hd;

	if (status == DESTROY_CHECKING) {
		
		ptr = (struct reply_panel_data *)xv_get(client,
			WIN_CLIENT_DATA);

		hd = ptr->hd;

                /* STRING_EXTRACTION -
                 *
                 * A generic confirm message when the user tries to quit
                 * the compose window...
                 */
		if (!mt_destroying && mt_compose_window_inuse(ptr) &&
			!mt_vs_confirm(client, TRUE,
			gettext("Quit Window"),
			gettext("Cancel"),
			gettext(
			"Are you sure you want to Quit composition window?")))
		{
			notify_veto_destroy(client);
			return (NOTIFY_DONE);
		} else {

			/*
			 * Destroy the compose window and remove it from the
			 * list. If it is the only compose window then this
			 * routine just hides it and returns false. 
			 * mt_destroying is TRUE when quitting tool and will
			 * force destruction of all windows.
			 */
			if (!destroy_compose_window(ptr->hd,
				(Frame)client, mt_destroying))
			{
				(void)notify_veto_destroy(client);
				return(NOTIFY_DONE);
			}
		}

	} 


	(void) xv_set(client, FRAME_NO_CONFIRM, TRUE, 0);
	return (notify_next_destroy_func(client, status));
}


static
destroy_compose_window(hd, frame, force_destruction)

	struct header_data 	*hd;
	Frame	frame;
	int	force_destruction;
{
	register struct reply_panel_data *ptr, *prev, *rpd_list;
	Frame popup;
	int i, rcode;


	/*
	 * Search list for window and destroy it.  Spare it if it's the
	 * last window.
	 */
	prev = NULL;
	ptr = rpd_list = MT_RPD_LIST(hd);
	while (ptr) {
		if (frame == ptr->frame) {

			(void)xv_set(frame, WIN_SHOW, FALSE, 0);

			/*
			 * Stop check pointing and remove dead letter file
			 */
			if (mt_value("save")) {
				mt_remove_dead_letter(ptr);
			}

			mt_clear_compose_window(ptr);

			if (!force_destruction && ptr->next_ptr == NULL &&
					ptr == rpd_list) {
				/*
				 * Only window in the list.  Spare it
				 */
				ptr->inuse = FALSE;
				i = 1;
				for (;;) {
					popup = xv_get(frame,
						FRAME_NTH_SUBFRAME, i++);

					if (! popup) break;
					/* See if it's a popup */
                                        if (!(int)xv_get(popup, XV_KEY_DATA, IS_POPUP_KEY))
                                           continue;
					/* If already unmapped, no need to touch it */
                                        if (!(int)xv_get(popup, XV_SHOW))
                                           continue;

					if ((int)xv_get(popup, FRAME_CMD_PUSHPIN_IN))
					   xv_set(popup, FRAME_CMD_PUSHPIN_IN, FALSE, NULL);
					(void)xv_set(popup, WIN_SHOW, FALSE, 0);
				}

				rcode = FALSE;
				goto RETURN;
			}

			/* found it.  splice it out of chain */
			if (prev) {
				prev->next_ptr = ptr->next_ptr;
			} else {
				MT_RPD_LIST(hd) = ptr->next_ptr;
			}

			/* free up the message and its attachments if any */
			if (ptr->reply_msg) {
				mt_attach_clear_msg(ptr->reply_msg);
				msg_methods.mm_free_msg (ptr->reply_msg);
				ptr->reply_msg = NULL;
				ptr->rpd_al->al_msg = NULL;
			}

			if (ptr->rpd_bufferid) {
				free(ptr->rpd_bufferid);
			}
			/* set the al_errorframe to NULL if the errorframe
			   is the same as the frame being destroyed.
			   This prevents the reply_proc() from accessing
			   this frame. */
			if (ptr->rpd_al && 
			    (ptr->frame == ptr->rpd_al->al_errorframe)) {
			    ptr->rpd_al->al_errorframe = NULL;
			}
			free(ptr);
			break;
		}
		prev = ptr;
		ptr = ptr->next_ptr;
	}

	rcode = TRUE;

RETURN:
	/*
 	 * If mailtool is completely unmapped (ie it is a tooltalk
	 * client and is not doing anything) start the self destruct
	 * sequence...
 	 */
	if (!force_destruction && mt_mailtool_hidden(hd)) {
		(void)mt_start_self_destruct(atoi(mt_value("selfdestruct")));
	}

	return(rcode);
}

static void
mt_set_compose_focus(rpd, item)

	struct reply_panel_data	*rpd;
	Xv_opaque	item;

{
	/*
	 * Set the input focus to the appropiate place.  For now that's either
	 * in the textsw or one of the text items in the reply_panel
	 */
	if (item == rpd->replysw) {
		(void)xv_set(item, WIN_SET_FOCUS, 0);
	} else {
		(void)xv_set(rpd->reply_panel,
				PANEL_CARET_ITEM, item,
				WIN_SET_FOCUS, 
				0);
	}

	return;
}

void
mt_display_reply(ptr, focus_item)
	struct reply_panel_data *ptr;
	Xv_opaque	focus_item;

{
	Frame           frame;
	static struct itimerval itimer={ 30,0,30,0 }; /* checkpoint every 30s */

	(void)mt_stop_self_destruct();

	frame = ptr->frame;

	/* Start checkpointing to dead.letter */
	if (mt_value("save")) {
		/* Set the checkpoint frequency to every 30 seconds after
		 * an event in the compose textsw.  Don't checkpoint until
		 * there has been no activity for 30 seconds.  If we haven't
		 * checkpointed for 30 minutes, then check point anyway.
		 */
		notify_set_itimer_func((Notify_client) ptr,
			(Notify_func) mt_save_dead_letter, ITIMER_REAL, &itimer,
			(struct itimerval *) NULL);
	}

	/* By default focus goes to the To field */
	if (focus_item == NULL)
		focus_item = ptr->dest_fillin;

	if (!xv_get(frame, XV_SHOW)) {
		/* Since the window is unmapped we can't set the focus 
		 * (see bug 1049288), so we stash the item away and set
		 * it when the window is mapped 
		 */
		ptr->rpd_focus_item = focus_item;
		(void) xv_set(frame, XV_SHOW, TRUE, 0);
	} else {
		/* Window is unmapped.  We can set focus now */
		mt_set_compose_focus(ptr, focus_item);
		ptr->rpd_focus_item = NULL;
	}
	
	if (xv_get(frame, FRAME_CLOSED)) {
		(void) xv_set(frame, FRAME_CLOSED, FALSE, 0); 
	}

	(void)xv_set(frame, WIN_FRONT, 0);
}


mt_compose_window_inuse(ptr)

	struct reply_panel_data *ptr;
{
	Panel_item	item;
	struct msg	*msg;
	char 		*value, *defvalue;
	struct header_field_data	*hfd;


	/*
	 * Check if the compose window has been modified
	 */
	if (ptr->rpd_modified)
		return(TRUE);
	/* 
	 * Check if any fillin fields have been modified
	 */
	PANEL_EACH_ITEM(ptr->reply_panel, item) {
		if ((Panel_item_type) xv_get(item, PANEL_ITEM_CLASS) ==
		     PANEL_TEXT_ITEM && (int)xv_get(item, XV_SHOW)) {

		     	value = (char *)xv_get(item, PANEL_VALUE);
			hfd = (struct header_field_data *)xv_get(item,
						XV_KEY_DATA, MT_HFD_KEY);

			if (hfd == NULL || hfd->hfd_defvalue == NULL)
				defvalue = "";
			else
			 	defvalue = hfd->hfd_defvalue;

			if (strcmp(value, defvalue) != 0)
				return(TRUE);
		}
	} PANEL_END_EACH;

	if (mt_attachment_in_msg(ptr->reply_msg)) {
		return(TRUE);
	}

	return(FALSE);
}

mt_delete_tmpfields(rpd)

	struct reply_panel_data	*rpd;

{
	register int	n;
	int	field_deleted;
	Menu_item	menu_item;
	struct header_field_data *hfd;
	Menu	menu;

	/* Delete all header fields which were added by templates */
	menu = rpd->header_menu;
	field_deleted = FALSE;

	for (n = (int)xv_get(menu, MENU_NITEMS); n > 0; n--) {

		menu_item = xv_get(menu, MENU_NTH_ITEM, n);

		if ((hfd = (struct header_field_data *)xv_get(menu_item,
		     XV_KEY_DATA, MT_HFD_KEY)) && hfd->hfd_tmpfield) {
			if (hfd->hfd_fillin)
				(void)xv_destroy_safe(hfd->hfd_fillin);
			(void)xv_set(menu, MENU_REMOVE_ITEM, menu_item, 0);
			(void)xv_destroy_safe(menu_item);
			field_deleted = TRUE;
		}
	}

	return(field_deleted);
}

mt_clear_compose_window(rpd)

	struct reply_panel_data *rpd;

{
#ifdef EDITOR
	/* If we have an edit sessions going terminate it */
	if (rpd->rpd_editor_inuse)
		mt_stop_editor(rpd);
#endif

	textsw_reset(rpd->replysw, 0, 0);
	mt_clear_compose_fillins(rpd->reply_panel);
	if (mt_delete_tmpfields(rpd))
		mt_compose_frame_layout_proc(rpd->frame);
	(void)xv_set(rpd->frame, FRAME_LEFT_FOOTER, "", 0);

	/*
	 * Make sure that the caret is on the To line
	 */
	mt_set_compose_focus(rpd, rpd->dest_fillin);

	/* If the Record item is visible, make sure it is checked */
	if (xv_get(rpd->rpd_record_item, XV_SHOW)){
	   if (mt_value("dontlogmessages"))
		(void)xv_set(rpd->rpd_record_item, PANEL_TOGGLE_VALUE, 0, 0, 0);
	   else
		(void)xv_set(rpd->rpd_record_item, PANEL_TOGGLE_VALUE, 0, 1, 0);
	}

	/*
	 * Reset modified flag
	 */
	rpd->rpd_modified = FALSE;
	rpd->rpd_checkpt = FALSE;
	rpd->inuse = FALSE;
	rpd->itimer_started = FALSE;

	/* Empty the attachment list */
	mt_clear_attachments(rpd->rpd_al);

	/*
	 * Free the old message and create a new one
	 */
	if (rpd->reply_msg) {
		mt_attach_clear_msg(rpd->reply_msg);
		msg_methods.mm_free_msg(rpd->reply_msg);
	}
	rpd->reply_msg = msg_methods.mm_create (0);

	mt_set_attach_msg(rpd->rpd_al, rpd->reply_msg);
}


static
mt_clear_compose_fillins(panel)

	Panel	panel;
{
	Panel_item	item;
	char		*defvalue;
	struct header_field_data *hfd;

	/* 
	 * Clear out the contents of all fillin fields in the reply panel
	 */
	PANEL_EACH_ITEM(panel, item) {

		if ((Panel_item_type)xv_get(item, PANEL_ITEM_CLASS) ==
							PANEL_TEXT_ITEM) {

			hfd = (struct header_field_data *)
				xv_get(item, XV_KEY_DATA, MT_HFD_KEY);

			if (hfd == NULL || hfd->hfd_defvalue == NULL)
				defvalue = "";
			else
			 	defvalue = hfd->hfd_defvalue;

			MP(("mt_clear_compose_fillins: setting %s to '%s'\n",
				xv_get(item, PANEL_LABEL_STRING), defvalue));

			xv_set(item, PANEL_VALUE, defvalue, 0);
		}
	}
	PANEL_END_EACH;
}


Menu_item
mt_create_header_menuitem(ptr, menu, name, defvalue, tmpfield)
	struct reply_panel_data	*ptr;
	Menu	menu;
	char	*name;
	char	*defvalue;
	int	tmpfield;
{
	char	*buf;
	int	len;
	Menu_item	item;
	void		mt_toggle_field_proc();
	Panel_item	field_item;
	Panel_item	mt_get_header_field();
	struct header_field_data	*hfd;

	/* find the end of the string */
	len = strlen(name);

	/* make a local copy of the name */
	buf = ck_malloc(len +2);	/* one for '\0', one for ':' */
	strcpy(buf, name);
	buf[len] = '\0';
	buf[len + 1] = '\0';

	field_item = mt_get_header_field(ptr, buf);

	buf[len] = ':';

	hfd = create_hfd();

	if (*defvalue) {
		hfd->hfd_defvalue = strdup(defvalue);
	} else {
		hfd->hfd_defvalue = NULL;
	}

	hfd->hfd_tmpfield = tmpfield;	/* TRUE to hide field on a clear */

	item = xv_create(XV_NULL, MENUITEM,
		MENU_ACTION_PROC,	mt_toggle_field_proc,
		MENU_CLIENT_DATA,	field_item,
		XV_KEY_DATA,		MT_HFD_KEY, hfd,
		MENU_RELEASE,
		XV_HELP_DATA,		"mailtool:ToggleFieldItem",
		0);

	hfd->hfd_mi = item;
	hfd->hfd_fillin = field_item;

	set_header_menu_label(item, 0, buf);
	free(buf);

	xv_set(menu, MENU_APPEND_ITEM, item, 0);

	/*
	 * If old panel_item, reset key data
	 * to new hfd
	 */
	if (field_item) {
		xv_set(field_item, 
			XV_KEY_DATA, MT_HFD_KEY, hfd,
			0);
	}
	return(item);
}



static void
create_header_field_menu(ptr)

	struct reply_panel_data	*ptr;

{
	Menu		menu;
	char		*s;
	char		*name;
	char		*value;
	Panel_item      item;
	extern  int	mt_alias_proc();
	int		n;


	menu = ptr->header_menu;

	(void)xv_set(menu, MENU_CLIENT_DATA, ptr, 0);

        /* STRING_EXTRACTION -
         *
         * We want to be able to access the alias property sheet
	 * from the Compose Window.
         */

	/*
	 * See if first two menuitems are created
	 * (Aliases, Bcc). Create them once.
	 */
	n = (int)xv_get(menu, MENU_NITEMS);
	if (n == 0) {
	   item = xv_create(XV_NULL, MENUITEM,
		MENU_ACTION_PROC,	mt_alias_proc,
		MENU_STRING, gettext("Aliases..."),
		XV_HELP_DATA, "mailtool:PropsAliasList",
		0);

	   xv_set(menu, MENU_APPEND_ITEM, item, 0);
	}


        /* STRING_EXTRACTION -
         *
         * We automatically prepend either "Add" or "Delete" to each
         * currently displayed standard header line ("Subject", "To", "Cc",
         * and "Bcc") depending on whether or not the item is currently
         * being displayed on the header.
         *
         * If the corresponding panel header is visible, the menu
         * should allow a delete, else an add.
         */
	add_string = gettext("Add");
	delete_string = gettext("Delete");

	/*
	 * Hack in Bcc for now. Bcc should be included as a default in the
	 * prop sheet
	 */
	if (n == 0) {
	   (void)mt_create_header_menuitem(ptr, menu, strdup("Bcc"), "", FALSE);
	   (void)xv_set(menu, 
		MENU_DEFAULT_ITEM, 
		(Panel_item) xv_get(menu, MENU_NTH_ITEM, 2),
		0);
	}

	/*
	 * Fill in the fields menu
	 */
	s = mt_value("additionalfields");

	while(parse_external_string(&s, &name, &value)) {
		(void)mt_create_header_menuitem(ptr, menu, name, value, FALSE);
	}
}


Panel_item
mt_get_header_field(ptr, label)

	struct reply_panel_data	*ptr;
	char	*label;

{
	register char	*p;
	register char	*item_label;
	Panel_item	item;	
	int		n;

	/*
	 * Search the reply panel for a header field which
	 * matches the specified label
	 */
	PANEL_EACH_ITEM(ptr->reply_panel, item)
	{
		if ((Panel_item_type)xv_get(item, PANEL_ITEM_CLASS) !=
			PANEL_TEXT_ITEM)
		{
			continue;
		}
		/* 
		 * Get the item's label and compute its length without
		 * the ':'
		 */
		item_label = (char *)xv_get(item, PANEL_LABEL_STRING);
		n = strlen(item_label) - 1;

		/* 
		 * Compare strings
		 */
		if (strncmp(label, item_label, n) == 0 && strlen(label) == n)
		{
			return(item);
		}
	}
	PANEL_END_EACH;

	return(NULL);
}

void
mt_toggle_field_proc(menu, menu_item)

	Menu		menu;
	Menu_item	menu_item;
{
	struct reply_panel_data	*rpd;

	TRACK_BUTTON2(menu, menu_item, "add_del_header");

	mt_toggle_field(menu, menu_item);

	/*
	 * Layout the compose frame
	 */
	rpd = (struct reply_panel_data *)xv_get(menu, MENU_CLIENT_DATA);
	mt_compose_frame_layout_proc(rpd->frame);

#ifdef NEVER /* Doesn't seem to be needed anymore */
	/*
	 * Make sure everything is visible.  If we don't do this, and
	 * the user does an Add Bcc then an Add Cc, the Cc line exists
	 * but is not visible.  For some reason you need the bloody
	 * CLEAR to force the paint
	 */
	panel_paint(rpd->reply_panel, PANEL_CLEAR);
#endif
}


void
mt_toggle_field(menu, menu_item)

	Menu		menu;
	Menu_item	menu_item;
{
	Panel_item	panel_item;
	int		item_shown;
	struct reply_panel_data	*ptr;
	struct header_field_data	*hfd;
	char		*label;
	char		*p;
	char		*defvalue;

	TRACK_BUTTON2(menu, menu_item, "add_del_header");

	/*
	 * Get the panel item which corresponds to this menu item
	 */
	panel_item = (Panel_item)xv_get(menu_item, MENU_CLIENT_DATA);
	ptr = (struct reply_panel_data *)xv_get(menu, MENU_CLIENT_DATA);

	p = (char *)xv_get(menu_item, MENU_STRING);
	if ((label = (char *)strrchr(p, ' ')) == NULL)
		label = p;
	else
		label++;

	hfd = (struct header_field_data *)xv_get(menu_item, XV_KEY_DATA,
						 MT_HFD_KEY);
	if (hfd == NULL || hfd->hfd_defvalue == NULL)
		defvalue = "";
	else
	 	defvalue = hfd->hfd_defvalue;

	if (panel_item == NULL) {
		/* Field isn't created yet.  Create it now */
		panel_item = (Panel_item)xv_create(ptr->reply_panel, PANEL_TEXT,
			PANEL_LABEL_STRING, 	label,
			PANEL_LABEL_BOLD,	TRUE,
			PANEL_VALUE_DISPLAY_LENGTH, 60,
			PANEL_VALUE_STORED_LENGTH, LINE_SIZE,
			PANEL_NOTIFY_LEVEL,	PANEL_SPECIFIED,
			PANEL_NOTIFY_STRING, 	"\t\r\n",
			PANEL_NOTIFY_PROC, 	mt_compose_text_notify_proc,
			PANEL_SHOW_ITEM,	FALSE,
			XV_HELP_DATA,		"mailtool:ComposeCustom",
			XV_KEY_DATA, MT_HFD_KEY, hfd,
			0);
		(void)xv_set(menu_item, MENU_CLIENT_DATA, panel_item, NULL);
		/* This was 0 before */
		hfd->hfd_fillin = panel_item;
	}


	/*
	 * Toggle the item on the panel
	 */
	item_shown = (int)xv_get(panel_item, PANEL_SHOW_ITEM);
	xv_set(panel_item, PANEL_SHOW_ITEM, !item_shown, 0);


	/*
	 * Set the menu item's string
	 */
	set_header_menu_label(menu_item,
		xv_get(panel_item, PANEL_SHOW_ITEM) == TRUE, label);

	/*
	 * Initialize the value of the header.
	 * We set it even if "", since destroy compose fillins proc
	 * resets PANEL_VALUE back to hfd->defvalue
	 */
	if (defvalue) { 
		xv_set(panel_item, PANEL_VALUE, defvalue, 0);
	}

}



static
set_header_menu_label(menu_item, on_panel, panel_string)

	Menu_item	menu_item;
	int	on_panel;
	char	*panel_string;

{
	char	*menu_string;
	char	*prefix;

	/*
	 * If the corresponding panel header is visible, the menu
	 * should allow a delete, else an add.
	 */
	if (on_panel) {
		prefix = delete_string;
	} else {
		prefix = add_string;
	}

	/* STRING_EXTRACTION -
	 *
	 * Put the text item's label into the menu item's string.  The
	 * first argument is the add or delete string, and the second
	 * is the actual name of the header.
	 */
	menu_string = ck_malloc(strlen(prefix) + strlen(panel_string) + 2);
	sprintf(menu_string, gettext("%s %s"), prefix, panel_string);
	xv_set(menu_item, MENU_STRING, menu_string, MENU_RELEASE_IMAGE, 0);
	return;
}

mt_scoot_item_right(panel, item, n_chars)

	Panel	panel;
	Panel_item	item;
	int	n_chars;

{
	Xv_Font	font;
	int	n;

	/* Move item 1 character to the right */
	font = (Xv_Font)xv_get(panel, XV_FONT);

	n = (int)xv_get(item, PANEL_ITEM_X);
	n += n_chars * (int)xv_get(font, FONT_DEFAULT_CHAR_WIDTH);

	(void)xv_set(item, PANEL_ITEM_X, n, 0);

	return(n);
}


mt_attach_proc(me, mi)

	Menu		me;
	Menu_item	mi;

{
	struct reply_panel_data	*rpd;
	Frame			frame;
	Frame			headerframe;
	char			*attach_data_type;
	Attach_node		*new_an;

	rpd = (struct reply_panel_data *)xv_get(me, MENU_CLIENT_DATA);
	headerframe = MT_FRAME(rpd->hd);
	mt_busy(headerframe, TRUE, NULL, TRUE);

	/* Make sure attachment list is visible */
	mt_show_attach_list(rpd->rpd_al, TRUE);
	mt_layout_compose_window(rpd);

	/* Generate the pending attachment */
	attach_data_type = (char *)xv_get(mi,
					XV_KEY_DATA, ATTACH_TYPE_KEY_DATA); 
	new_an = mt_create_attachment(rpd->rpd_al, NULL, NULL,
					attach_data_type, NULL, FALSE);
	new_an->an_pending = TRUE;
	mt_invoke_application(rpd->rpd_al, new_an, TRUE);

	mt_busy(headerframe, FALSE, NULL, TRUE);

}

static void
dup_menu_defaults(source_panel, target_panel)

	Panel	source_panel, target_panel;


{
	Panel_item	src, tgt;
	Menu		source_menu, target_menu;

	/*
	 * Sets all top level menus in the target_panel to have the same
	 * defaults as the source_panel.  Both panels *must* be
	 * identical in termis of menu buttons for this to work.
	 */

	for (src = (Panel_item)xv_get(source_panel, PANEL_FIRST_ITEM),
	     tgt = (Panel_item)xv_get(target_panel, PANEL_FIRST_ITEM);
	     (src != NULL) && (tgt != NULL);
	     src = (Panel_item)xv_get(src, PANEL_NEXT_ITEM),
	     tgt = (Panel_item)xv_get(tgt, PANEL_NEXT_ITEM)) {
		
		if (xv_get(src, PANEL_ITEM_CLASS) != PANEL_BUTTON_ITEM)
			continue;	/* Not a button */

		if ((source_menu = xv_get(src, PANEL_ITEM_MENU)) == NULL)
			continue;	/* Not a menu button */
		
		target_menu = xv_get(tgt, PANEL_ITEM_MENU);

		(void)xv_set(target_menu,
			MENU_DEFAULT, (int)xv_get(source_menu, MENU_DEFAULT),
			0);
	}

	return;
}



/*
 * Cancel the current reply (or send, or forward).
 */
static void
do_cancel(
	struct reply_panel_data *ptr
)
{
	Panel           panel;
	Textsw		next_split;

        /* STRING_EXTRACTION -
         *
         * "Clear Window" and "Do Not Clear Window" are button labels.
         * "Are you sure you want to Clear window?" is a notice prompt.
         * This notice comes up when you hit the Clear button in the
         * compose window
         */

	if (mt_compose_window_inuse(ptr) &&
		!mt_vs_confirm(ptr->replysw, TRUE, 
		gettext("Clear Window"),
		gettext("Do NOT Clear Window"),
		gettext("Are you sure you want to Clear window?")))
	{
		return;
	}

	while (next_split = (Textsw)textsw_next(
		(Textsw)textsw_first(ptr->replysw)))
		(void)notify_post_destroy(next_split, DESTROY_CLEANUP,
					NOTIFY_IMMEDIATE);
	mt_clear_compose_window(ptr);
	ptr->inuse = FALSE;

}

/*
 * Actually send the current reply-type message.
 */
/* ARGSUSED */
void
mt_deliver_proc(
	Menu		menu,
	Menu_item	menu_item
)
{

#ifdef INSTRUMENT
	{
		char *msg;
		char buffer[90];

		switch(xv_get(menu_item, MENU_CLIENT_DATA)) {
		case 0: msg = "quit_win"; break;
		case 1: msg = "close_win"; break;
		case 2: msg = "clear_msg"; break;
		case 3: msg = "leave_msg"; break;
		}
		sprintf(buffer, "deliver_%s", msg);
		TRACK_BUTTON(menu, menu_item, buffer);
	}
#endif INSTRUMENT

	mt_do_deliver(menu, menu_item);
}


#ifdef PEM

/*
 * Send the current reply-type message using PEM integrity and/or confidentiality.
 */
/* ARGSUSED */
void
mt_pem_proc(menu, menu_item)

Menu		menu;
Menu_item	menu_item;

)
{

	mt_do_deliver_pem(menu, menu_item);
}

#endif PEM

