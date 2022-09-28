#ifndef lint
static  char sccsid[] = "@(#)mail.c 3.16 93/09/09 Copyr 1991 Sun Microsystems, Inc.";
#endif
/*
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */

#include <stdio.h>
#include <errno.h>
#ifdef SVR4
#include <unistd.h>
#endif "SVR4"
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <xview/xview.h>
#include <xview/frame.h>
#include <xview/font.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/textsw.h>
#include <xview/cms.h>
#include <xview/notice.h>
#include "util.h"
#include "timeops.h"
#include "graphics.h"
#include "browser.h"
#include "editor.h"
#include "calendar.h"
#include "ds_popup.h"
#include "datefield.h"
#include "props.h"
#include "mail.h"
#include "gettext.h"
#include "common.h"
#ifdef SVR4
#include <sys/param.h>
#endif "SVR4"

static unsigned short compose_image[] = {
#include "compose.icon"
};
static unsigned short compose_mask[] = {
#include "compose.mask.icon"
};
char cm_mailfile[20]="";

static Notify_value
mail_destroy_proc(client, status)
        Notify_client   client;
        Destroy_status  status;
{
        Calendar *c = (Calendar*)xv_get((Frame)client, WIN_CLIENT_DATA);
        Mail *m;
        Browser *b;
        struct stat sb;

        if (status == DESTROY_CHECKING) {
                b = (Browser*)c->browser;
                m = (Mail*)c->mail;
                if (m != NULL) {
                        textsw_reset(m->comptextsw, 0, 0);
                        if (stat(cm_mailfile, &sb) == 0)
                                unlink(cm_mailfile);
			if (browser_showing(b))
				reset_values(c);
			free(m);
			c->mail = NULL;
                }
        }
        return(notify_next_destroy_func(client, status));
 
}
/* ARGSUSED */
static Notify_value
mail_close_proc(client, event, arg, when)
        Notify_client   client;
	Event          *event;
        Notify_arg      arg;
        Notify_event_type when;
{
        Calendar *c;
        Mail *m;
	char    	icon_label[20];
	Icon		icon;

	if (event_action(event) == ACTION_CLOSE) {
		c = (Calendar*)xv_get((Frame)client, WIN_CLIENT_DATA);
        	m = (Mail*)c->mail;
                cm_strcpy(icon_label,  LGET("To:") );
                strncat(icon_label, (char *)xv_get(m->compmailto, 
				PANEL_VALUE), sizeof(icon_label) - 4);
                icon = xv_get(m->compframe, FRAME_ICON);
                xv_set(icon, XV_LABEL, icon_label, 0);
        }
        return(notify_next_event_func(client, (Notify_event)event, arg, NOTIFY_SAFE));
 
}
/* ARGSUSED */
Panel_setting
mail_panel_notify_proc(item, event)
Panel_item      item;
Event           *event;
{
	Calendar *c = (Calendar*)xv_get(item, PANEL_CLIENT_DATA);
	Mail *m = (Mail*)c->mail;
	Boolean show = false;

	show = xv_get(m->compbcc, XV_SHOW);
	if ((item == m->compbcc && show) || (item == m->compcc && !show)) {
		xv_set(m->comptextsw, WIN_SET_FOCUS, NULL);
		return PANEL_NONE;
	}
	return(panel_text_notify(item, event));

}

extern void
mail_list(b, ml)
        Browser *b;
        char    *ml;
{
        int i, j;
	char *name, *retname=NULL;
	Calendar *c = calendar;
	Props *p = (Props*)c->properties;

        *ml = NULL;
        for (i = 0, j= xv_get(b->box, PANEL_LIST_NROWS); i < j; i++)
                if (xv_get(b->box, PANEL_LIST_SELECTED, i)) {
			name = (char*)xv_get(b->box, PANEL_LIST_STRING, i);
			if (name != NULL && *name != NULL) {
				if (strcmp(name, c->calname) == 0)
					name_in_list(c, name, 
						p->mailto_VAL, &retname);
				if (retname != NULL) {
                        		cm_strcat(ml, retname);
					retname = NULL;
				}
				else
                        		cm_strcat(ml, name);
                        	cm_strcat(ml, " ");
			}
                }
}

clear_it(m)
	Mail *m;
{
	xv_set(m->compmailto, PANEL_VALUE, "", 0);
	xv_set(m->compsubj, PANEL_VALUE, "", 0);
	xv_set(m->compcc, PANEL_VALUE, "", 0);
	xv_set(m->compbcc, PANEL_VALUE, "", 0);
	xv_set(m->compframe, FRAME_LEFT_FOOTER, "", 0);
}
static void
mail_deliver_proc(menu, menu_item)
Menu            menu;
Menu_item       menu_item;
{
        int             intact = FALSE;
        int             stay_up = TRUE;
        int             make_iconic = FALSE;
	char 		*mt;
        Calendar 	*c = (Calendar*)xv_get(menu, MENU_CLIENT_DATA);
        Browser 	*b = (Browser*)c->browser;
        Mail 	 	*m = (Mail*)c->mail;
        FILE 		*f;
        char 		buf[BUFSIZ+1];
        Textsw_index 	index, new_index;
        int		nbytes;
	char    	icon_label[20];
	Icon 		icon;
	char		*val;

        stay_up = (int) xv_get(menu_item, MENU_VALUE) > 0;
        make_iconic = (int) xv_get(menu_item, MENU_VALUE) == 1;
        intact = (int) xv_get(menu_item, MENU_VALUE) > 2;
  
	mt = (char*)xv_get(m->compmailto, PANEL_VALUE);
        if (mt == NULL || *mt == '\0') {
               notice_prompt(m->compframe, (Event *)NULL,
                        NOTICE_MESSAGE_STRINGS, 
			 MGET("Please specify a recipient.") ,
			0,
                        NOTICE_BUTTON_YES,  LGET("Continue") ,
               		0);
                return;
        }

	(void) cm_strcpy(cm_mailfile, "/tmp/.calmailXXXXXX");
	(void) mktemp(cm_mailfile);
        if ((f = fopen(cm_mailfile, "w+")) == NULL) {
                (void) fprintf(stderr,  EGET("CM: error in creating tmp mail file") );
                return;
        }
	/* Decrement the reference count here so the file will not stay
	 * around if tool crashes.
	 */
	unlink(cm_mailfile);

        (void) fprintf(f, "To: %s\n",
                                (char*)xv_get(m->compmailto, PANEL_VALUE));
	(void) fprintf(f, "Subject: %s\n",
                        (char*)xv_get(m->compsubj, PANEL_VALUE));
	if ((val = (char*)xv_get(m->compcc, PANEL_VALUE)) != NULL
		 && *val != '\0')
        	(void) fprintf(f, "Cc: %s\n", val);
	if (xv_get(m->compbcc, XV_SHOW) && 
		(val = (char*)xv_get(m->compbcc, PANEL_VALUE)) != NULL
			&& *val != '\0')
        	(void) fprintf(f, "Bcc: %s\n\n", val);
                        

        index = 0;
        do {
                new_index = (Textsw_index)xv_get(m->comptextsw, TEXTSW_CONTENTS,
                                                index, buf, BUFSIZ);

                nbytes = new_index - index;
                index = new_index;
 
                if (nbytes == 0) break;
		if (fwrite(buf, nbytes, 1, f) == 0) {
                        (void) fprintf(stderr,  EGET("CM: Error in sending mail message.") );
                        fclose(f);
                        return;
                }
        } while (nbytes == BUFSIZ);
 
	rewind(f);
	if (vfork()==0) {
		dup2(fileno(f), 0);
#ifdef SVR4
                execlp(MAIL_BINARY_NAME, MAIL_BINARY_NAME, "-t", 0);
		                              /* try using $PATH first */
		if (errno == ENOENT) {
		    execlp(MAIL_FIXED_PATH, MAIL_FIXED_PATH, "-t", 0);
                                              /* failed, try hard-coded path */
                }
#else
		execlp("Mail", "Mail", "-t", 0);
#endif "SVR4"
		_exit(-1);
	}

     	if (!stay_up) {
                textsw_reset(m->comptextsw, 0, 0);
		(void) xv_set(m->compframe, XV_SHOW, FALSE, 0);
	} 

        if (!intact) {
		textsw_reset(m->comptextsw, 0, 0);
		clear_it(m);
        }
 
        if (make_iconic) {
                xv_set(m->compframe, FRAME_CLOSED, TRUE, 0);
		cm_strcpy(icon_label,  LGET("To:") );
                strncat(icon_label, (char *)xv_get(m->compmailto, PANEL_VALUE), 
				sizeof(icon_label) - 4);
		icon = xv_get(m->compframe, FRAME_ICON);
                xv_set(icon, XV_LABEL, icon_label, 0);
	}
	if (browser_exists(b))
		reset_values(c);
 
	fclose(f);
}
static void
delete_bcc_proc(menu, menu_item)
Menu            menu;
Menu_item       menu_item;
{
	Calendar *c = (Calendar*)xv_get(menu, MENU_CLIENT_DATA);
	Mail *m = (Mail*)c->mail;
	Menu_item mi;
	static void add_bcc_proc();

	(void) xv_set(menu, MENU_REMOVE_ITEM, menu_item, 0);
        mi = xv_create( XV_NULL, MENUITEM, 
			MENU_ACTION_ITEM,  LGET("Add Bcc:") , add_bcc_proc,
                        MENU_CLIENT_DATA, c,
                        XV_HELP_DATA, "cm:Addbcc",
                        NULL);
        (void)xv_set(menu, MENU_INSERT, 0, mi, NULL);       
        (void)xv_set(m->compbcc, XV_SHOW, FALSE, NULL);
	(void)xv_set(m->comppanel, XV_HEIGHT, xv_get(m->compcc, XV_Y)
			+ xv_get(m->compcc, XV_HEIGHT)+10, NULL);
	(void)xv_set(m->comptextsw, WIN_BELOW, m->comppanel, NULL);
	window_fit(m->compframe);
}
static void
add_bcc_proc(menu, menu_item)
Menu            menu;
Menu_item       menu_item;
{
	Calendar *c = (Calendar*)xv_get(menu, MENU_CLIENT_DATA);
	Mail *m = (Mail*)c->mail;
	Menu_item mi;
	static void delete_bcc_proc();

	(void) xv_set(menu, MENU_REMOVE_ITEM, menu_item, 0);
	mi = xv_create( XV_NULL, MENUITEM, 
			MENU_ACTION_ITEM,  LGET("Delete Bcc:") , delete_bcc_proc,
			MENU_CLIENT_DATA, c,
			XV_HELP_DATA, "cm:DeleteBcc",
			NULL);
	(void)xv_set(menu, MENU_INSERT, 0, mi, NULL);	

	(void)xv_set(m->comppanel, XV_HEIGHT, xv_get(m->compbcc, XV_Y)
			 + xv_get(m->compbcc, XV_HEIGHT)+10, NULL);
	xv_set(m->compbcc, XV_SHOW, TRUE, NULL);
	(void)xv_set(m->comptextsw, WIN_BELOW, m->comppanel, NULL);
}
static Menu
create_header_pulldown(c)
	Calendar *c;
{
	Menu            menu;
        Menu_item       item;

        menu = xv_create(XV_NULL, MENU, 
			MENU_CLIENT_DATA, c,
			0);

        item = xv_create(XV_NULL, MENUITEM,
                MENU_ACTION_ITEM,        LGET("Add Bcc:") , add_bcc_proc,
                MENU_VALUE,		0,
                XV_HELP_DATA,           "cm:Addbcc",
                0);

        xv_set(menu,
                MENU_APPEND_ITEM, item,
                0);

	return (menu);
}
/* ARGSUSED */
static void
include_template(item, event)
Panel_item      item;
Event           *event;
{
        struct stat sb;
	Calendar *c = (Calendar*)xv_get(item, PANEL_CLIENT_DATA);
	Mail *m = (Mail*)c->mail;
	char	cmtpl_file[BUFSIZ];

	sprintf(cmtpl_file, "%s/share/xnews/client/templates/calendar.tpl", 
			(char*)getenv("OPENWINHOME"));
	if (stat(cmtpl_file, &sb) == 0)
		xv_set(m->comptextsw, 
			TEXTSW_INSERT_FROM_FILE, cmtpl_file,
			NULL);
	else
		xv_set(m->compframe, FRAME_LEFT_FOOTER,
			 EGET("CM: Calendar Manager template does not exist.") ,
			NULL);
}
/* ARGSUSED */
static void
clear_proc(item, event)
Panel_item      item;
Event           *event;
{
        Calendar *c = (Calendar*)xv_get(item, PANEL_CLIENT_DATA);
        Mail *m = (Mail*)c->mail;
	int 	status;
	Event ie;

	if (xv_get(m->comptextsw, TEXTSW_MODIFIED)
		|| xv_get(m->compmailto, PANEL_VALUE) != NULL
		|| xv_get(m->compsubj, PANEL_VALUE) != NULL) {
		status = notice_prompt(m->compframe,
                        &ie,
                        NOTICE_MESSAGE_STRINGS, 
			 MGET("Are you sure you want to Clear window?") ,
			0,
                        NOTICE_BUTTON_YES,  LGET("Clear Window") ,
                        NOTICE_BUTTON_NO,  LGET("Do NOT Clear Window") ,
                        0);
                if (status == NOTICE_YES) { 
			textsw_reset(m->comptextsw, 0, 0);
			clear_it(m);
		}
	}
}
static Menu
create_deliver_pulldown(c)
	Calendar *c;
{
	Menu            menu;
        Menu_item       item;

        menu = xv_create(XV_NULL, MENU, 
			MENU_CLIENT_DATA, c,
			 0);

        item = xv_create(XV_NULL, MENUITEM,
                MENU_ACTION_ITEM,        LGET("Quit window") , mail_deliver_proc,
                MENU_VALUE,		0,
                XV_HELP_DATA,           "cm:DeliverTakedown",
                0);

        xv_set(menu,
                MENU_APPEND_ITEM, item,
                0);
 
        item = xv_create(XV_NULL, MENUITEM,
                MENU_ACTION_ITEM,        LGET("Close window") , mail_deliver_proc,
                MENU_VALUE,             1,
                XV_HELP_DATA,           "cm:DeliverClose",
                0);

	xv_set(menu,
                MENU_APPEND_ITEM, item,
                0);
 
        item = xv_create(XV_NULL, MENUITEM,
                MENU_ACTION_ITEM,        LGET("Clear message") , mail_deliver_proc,
                MENU_VALUE,	        2,
                XV_HELP_DATA,           "cm:DeliverClearItem",
                0);
 
        xv_set(menu,
                MENU_APPEND_ITEM, item,
                0);
 
        item = xv_create(XV_NULL, MENUITEM,
                MENU_ACTION_ITEM,        LGET("Leave message intact") , mail_deliver_proc,
                MENU_VALUE, 	        3,
                XV_HELP_DATA,           "cm:DeliverLeaveMessageItem",
                0);
 
        xv_set(menu,
                MENU_APPEND_ITEM, item,
                0);

	return(menu);
}
#define MAIL_DISP_SIZE 57 

static caddr_t
make_compose(c)
        Calendar *c;
{
        Mail *m = (Mail*)c->mail;
	Server_image server_image;
	Server_image mask_image;
	Icon	tool_icon;
	int row = 0, gap = 10;
	char buf[MAXNAMELEN];
	char *t;
	struct pr_size size, size1, size2, size3, size4;
	Xv_Font pf;
	int wd;
	Font_string_dims dims;
	
	pf =  c->fonts->fixed12;
	c->mail = (caddr_t) ckalloc(sizeof(Mail));
	m = (Mail*)c->mail;
	buf[0] = '\0';
	server_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, compose_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);
        mask_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, compose_mask,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);
	tool_icon = (Icon) xv_create(0, ICON,
		ICON_IMAGE, server_image,
		ICON_MASK_IMAGE, mask_image,
		ICON_TRANSPARENT, TRUE,
		XV_LABEL,  LGET("To:") ,
		0);

        m->compframe = xv_create(XV_NULL, FRAME_BASE,
                XV_WIDTH, 550,
                XV_HEIGHT, 415,
                WIN_ROW_GAP, 4,
                FRAME_SHOW_FOOTER,      TRUE,
                FRAME_SHOW_RESIZE_CORNER,       TRUE,
                WIN_BIT_GRAVITY, ForgetGravity,
                FRAME_SHOW_LABEL, TRUE,
		WIN_CMS,     (Cms)xv_get(c->frame, WIN_CMS),
		FRAME_ICON, tool_icon,
                WIN_CLIENT_DATA, c,
		FRAME_NO_CONFIRM, TRUE,
                XV_HELP_DATA, "cm:MailHelp",
		FRAME_CLOSED, FALSE,
		FRAME_WM_COMMAND_ARGC_ARGV, NULL, -1,
                0);
        m->comppanel = xv_create(m->compframe, PANEL,
                XV_X, 0,
                XV_Y, 0,
                PANEL_CLIENT_DATA, c,
                OPENWIN_SHOW_BORDERS, FALSE,
                XV_HELP_DATA, "cm:MailHelp",
                0);
	
	row = xv_row(m->comppanel,0);
	m->include_button = xv_create(m->comppanel, PANEL_BUTTON,
                PANEL_LABEL_STRING,  LGET("Include") ,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, 10,
                XV_Y, row,
		PANEL_NOTIFY_PROC, include_template,
                PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:IncludeButton",
                0);
	m->deliver_button = xv_create(m->comppanel, PANEL_BUTTON,
                PANEL_LABEL_STRING,  LGET("Deliver") ,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, xv_get(m->include_button, XV_X) +
			 xv_get(m->include_button, XV_WIDTH) + gap,
                XV_Y, row,
		PANEL_ITEM_MENU, create_deliver_pulldown(c),
                PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:DeliverButton",
                0);
	m->header_button = xv_create(m->comppanel, PANEL_BUTTON,
                PANEL_LABEL_STRING,  LGET("Header") ,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, xv_get(m->deliver_button, XV_X) +
			 xv_get(m->deliver_button, XV_WIDTH) + gap,
                XV_Y, row,
		PANEL_ITEM_MENU, create_header_pulldown(c),
                PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:HeaderButton",
                0);
         m->clear_button = xv_create(m->comppanel, PANEL_BUTTON,
                PANEL_LABEL_STRING,  LGET("Clear") ,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, xv_get(m->header_button, XV_X) +
			 xv_get(m->header_button, XV_WIDTH) + gap,
                XV_Y, row,
		PANEL_NOTIFY_PROC, clear_proc,
                PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:ClearButton",
                0);

	t =  MGET("To:") ;
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size1.x = dims.width;
        size1.y = dims.height;

	t =  MGET("Subject:") ;
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size2.x = dims.width;
        size2.y = dims.height;

	
	t =  MGET("Cc:") ;
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size3.x = dims.width;
        size3.y = dims.height;

	t =  MGET("Bcc:") ;
        (void)xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size4.x = dims.width;
        size4.y = dims.height;

	wd = max(max(max(size1.x, size2.x), size3.x), size4.x) +
		c->view->outside_margin;
        m->compmailto = xv_create(m->comppanel, PANEL_TEXT,
                PANEL_LABEL_STRING,  LGET("To:") , PANEL_LABEL_BOLD, TRUE,
                PANEL_VALUE_DISPLAY_LENGTH, MAIL_DISP_SIZE,
                PANEL_VALUE_STORED_LENGTH, BUFSIZ,
                XV_X, wd - size1.x,
                XV_Y, xv_get(m->deliver_button, XV_HEIGHT) +
                        xv_get(m->deliver_button, XV_Y) + 2,
                XV_HELP_DATA, "cm:ToField",
                0);
        m->compsubj = xv_create(m->comppanel, PANEL_TEXT,
                PANEL_LABEL_STRING,  LGET("Subject:") ,
                PANEL_LABEL_BOLD, TRUE,
		PANEL_VALUE_DISPLAY_LENGTH, MAIL_DISP_SIZE,
                XV_X, wd - size2.x,
                XV_Y, xv_get(m->compmailto, XV_HEIGHT) +
                        xv_get(m->compmailto, XV_Y) + 2,
		PANEL_NOTIFY_LEVEL,     PANEL_SPECIFIED,
		PANEL_NOTIFY_STRING,    "\t\r\n",
		PANEL_NOTIFY_PROC,  mail_panel_notify_proc,
		PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:CompSubj",
                0);
	 m->compcc = xv_create(m->comppanel, PANEL_TEXT,
		PANEL_LABEL_STRING,  LGET("Cc:") ,
		PANEL_VALUE_DISPLAY_LENGTH, MAIL_DISP_SIZE,
                PANEL_VALUE_STORED_LENGTH, BUFSIZ,
		XV_X, wd - size3.x,
		XV_Y, xv_get(m->compsubj, XV_Y) +
                                xv_get(m->compsubj, XV_HEIGHT) + 2,
		PANEL_NOTIFY_LEVEL,     PANEL_SPECIFIED,
		PANEL_NOTIFY_STRING,    "\t\r\n",
		PANEL_NOTIFY_PROC,      mail_panel_notify_proc,
		XV_SHOW,   TRUE,
		PANEL_CLIENT_DATA, c,
		XV_HELP_DATA,           "cm:Composecc",
		0);
	 m->compbcc = xv_create(m->comppanel, PANEL_TEXT,
		PANEL_LABEL_STRING,  LGET("Bcc:") ,
		PANEL_VALUE_DISPLAY_LENGTH, MAIL_DISP_SIZE,
		XV_X, wd - size4.x,
		XV_Y, xv_get(m->compcc, XV_Y) +
			 xv_get(m->compcc, XV_HEIGHT) + 2,
		PANEL_NOTIFY_LEVEL,     PANEL_SPECIFIED,
		PANEL_NOTIFY_STRING,    "\t\r\n",
		PANEL_NOTIFY_PROC,      mail_panel_notify_proc,
		XV_SHOW,   FALSE,
		PANEL_CLIENT_DATA, c,
		XV_HELP_DATA,           "cm:ComposeBcc",
		0);
        m->comptextsw = xv_create(m->compframe, TEXTSW,
                XV_X, 0,
                NULL);

	(void)sprintf(buf, "%s: %s", 
		 (char*)cm_get_relname(), MGET("Compose Message"));
        xv_set(m->compframe, XV_LABEL, buf, NULL);
        xv_set(m->comppanel, XV_HEIGHT, xv_get(m->compcc, XV_Y) +
		xv_get(m->compcc, XV_HEIGHT)+10, NULL);

        ds_position_popup(c->frame, m->compframe, DS_POPUP_LOR);
        notify_interpose_destroy_func(m->compframe, mail_destroy_proc);
	notify_interpose_event_func(m->compframe, mail_close_proc, NOTIFY_SAFE);
	xv_set(m->comptextsw, WIN_BELOW, m->comppanel, NULL);

	return (caddr_t)m;
}

extern void
get_cm_compose()
{
	Calendar *c = calendar;
	Browser *b = (Browser*)c->browser;
        Mail *m = (Mail*)c->mail;
        char mailing_list[BUFSIZ];
	char *appt;

        if (m == NULL) {
                c->mail = (caddr_t)make_compose(c);
		m = (Mail*)c->mail;
	}

	xv_set(m->compframe, XV_SHOW, TRUE, NULL);

        xv_set(m->compsubj, PANEL_VALUE,  MGET("Meeting") , NULL);

	mail_list(b, mailing_list);
	xv_set(m->compmailto, PANEL_VALUE, mailing_list, NULL);

	appt = get_appt_str();
	xv_set(m->comptextsw, TEXTSW_CONTENTS, appt, NULL);
	if (appt)
		free(appt);

	xv_set(m->comptextsw, TEXTSW_INSERTION_POINT, 0,
		TEXTSW_FIRST, 0,  NULL);
	textsw_possibly_normalize(m->comptextsw, 0);

	xv_set(b->frame, FRAME_BUSY, FALSE, NULL);
}

/* ARGSUSED */
extern Notify_value
mail_proc(item, event)
        Panel_item item;
        Event *event;
{
	char header[BUFSIZ], *hptr;
	char to_list[BUFSIZ];
	Browser *b = (Browser*)calendar->browser;

	/* FRAME_BUSY will be reset to FALSE in close_cb or get_cm_compose */
	xv_set(b->frame, FRAME_BUSY, TRUE, NULL);

	/* compose message header */
	mail_list(b, to_list);
	sprintf(header, "To: %s\nSubject: Meeting\n", to_list);
	hptr = cm_strdup(header);

	if (cm_tt_compose(hptr, calendar->frame, b->frame))
		get_cm_compose();

	if (hptr)
		free(hptr);
 
        return (NOTIFY_DONE);
}
