#ifndef lint
static  char sccsid[] = "@(#)template.c 3.7 93/05/20 Copyr 1987 Sun Micro";
#endif

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
 * Mailtool - building menus for the cmdpanel
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <xview/window_hs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#ifdef SVR4
#include <unistd.h>
#include <sys/kbd.h>
#include <sys/kbio.h>
#else
#include <sundev/kbd.h>
#endif SVR4

#include <xview/panel.h>
#include <xview/text.h>
#include <xview/font.h>
#include <xview/xview.h>
#include <xview/alert.h>

#include "glob.h"
#include "tool.h"
#include "tool_support.h"
#include "mail.h"
#include "mle.h"
#include "instrument.h"

extern void	mt_toggle_field();
extern void	mt_text_insert();

extern int REPLY_KEY_DATA;

static int	layout_panel;	/* Gross global to see if we need to layout
				   the compose panel after loading a template */

void
mt_template_action_proc(menu, menu_item)

Menu		menu;
Menu_item	menu_item;

{
	struct	reply_panel_data *ptr;
	int	lower_context;
	int	old_insertion_point;
	char	new_name[MAXPATHLEN+1];
	char	new_name1[MAXPATHLEN+1];
	char	*saved_name;

	TRACK_BUTTON(menu, menu_item, "template_include");

        ptr = (struct reply_panel_data *)
		xv_get(menu, XV_KEY_DATA, REPLY_KEY_DATA);

        lower_context = (int)xv_get(ptr->replysw, TEXTSW_LOWER_CONTEXT);

	saved_name = (char *) xv_get(menu_item, MENU_CLIENT_DATA);

	ds_expand_pathname(saved_name, new_name);

	if (new_name[0] != '/') {
		strcpy(new_name1, new_name);
		sprintf(new_name, "%s/%s", Getf("HOME"), new_name1);
	}

	if (access(new_name, F_OK) == -1)
	{
                /* STRING_EXTRACTION -
                 *
                 * We could not access the specified template file.  %s is
                 * the name of the file.
                 */
		mt_vs_warn(ptr->replysw,
			gettext("Template\n%s\ndoes not exist"), new_name);
	} else {

		mt_load_template(new_name, ptr);
	}

}

Menu
mt_gen_template_menu(m, operation)

Menu		m;
Menu_generate	operation;

{
	Menu_item	item;
	int	template_added = FALSE;
	char	*template;
	char	*name;
	char	*value;

	if (operation == MENU_DISPLAY) {

		if ((template = mt_value("templates")) == NULL)
			template = "calendar:$OPENWINHOME/share/xnews/client/templates/calendar.tpl";

		mt_clear_menu(m, TRUE, FALSE);

		while (parse_external_string(&template, &name, &value)) {
			if (! *value) continue;

			name = strdup(name);
			value = strdup(value);

			if (!name || !value) {
				mt_clear_menu(m, TRUE, FALSE);
				goto no_template;
			}

			template_added = TRUE;

			item = xv_create(XV_NULL, MENUITEM,
				0);
		
			xv_set(m, MENU_ITEM,
					MENU_STRING, name,
					MENU_ACTION_PROC,
						mt_template_action_proc,
					MENU_CLIENT_DATA, value,
					0,
				0);
		}

		if (!template_added)
		{

			/* STRING_EXTRACTION -
			 *
			 * This should not happen; if you used
			 * the standard property sheet we check for
			 * this case.  But in case you have a template
			 * variable with no actual templates, we create
			 * a menu with only a "no templates" entry.
			 */
		
no_template:
			xv_set(m,
				MENU_ITEM,
					MENU_STRING,	gettext("No templates"),
					0,
				0);
		}
	}

	return(m);
}


/*
 * Given and field name make sure it exists and is visible in 
 * the compose window, and fill it in with the value.
 */
mt_fill_field(field, value, rpd)

	char	*field;
	char	*value;
	struct  reply_panel_data *rpd;
{
	Menu_item	mi, find_field_in_menu();
	Panel_item	text_item;
	u_char		*p;

	if (*field == '\n')
		return(0);

	/* Check if field is in the Header menu */
	mi = find_field_in_menu(field, rpd->header_menu);

	if (mi == NULL) {
		/* Not in menu.  May be standard field (To, Subject, Cc) */
		text_item = mt_get_header_field(rpd, field);

		if (text_item == NULL) {
			/* Field exists no where.  Create it */
			mi = mt_create_header_menuitem(rpd, rpd->header_menu,
							field, "", TRUE);
			mt_toggle_field(rpd->header_menu, mi);
			layout_panel = TRUE;
			text_item = (Panel_item)xv_get(mi, MENU_CLIENT_DATA);
		}
	} else if (mi != NULL) {
		/* Field is in menu.  Toggle it if not displayed */
		text_item = (Panel_item)xv_get(mi, MENU_CLIENT_DATA);
		if (text_item == NULL || !(int)xv_get(text_item, XV_SHOW)) {
			mt_toggle_field(rpd->header_menu, mi);
			layout_panel = TRUE;
		}
		text_item = (Panel_item)xv_get(mi, MENU_CLIENT_DATA);
	}

	if (text_item != NULL) {
		/* Convert all whitespace to spaces */
		for (p = (u_char *) value; *p != '\0'; p++) {
			if (isspace(*p)) {
				*p = ' ';
			}
		}
		p--;

		/* Strip trailing whitespace */
		while (isspace(*p)) {
			*p-- = '\0';
		}

		/* Strip leading whitespace */
		while (isspace(*value))
			value++;

		(void)xv_set(text_item, PANEL_VALUE, value, 0);
	}

	return(0);
}

static Menu_item
find_field_in_menu(name, menu)

	char	*name;
	Menu	menu;

{
	int	nitems;
	int	n;
	int	name_length;
	Menu_item	mi;
	Panel_item	text_field;
	char		*label;
	char		*p;
	

	/* Search throught the specified Header menu looking for a field */
	name_length = strlen(name);
	for (n = (int)xv_get(menu, MENU_NITEMS); n > 0; n--) {
		mi = (Menu_item)xv_get(menu, MENU_NTH_ITEM, n);
		p = (char *)xv_get(mi, MENU_STRING);

		if ((label = (char *)strrchr(p, ' ')) == NULL)
			label = p;
		else
			label++;

		/* Compare field names.
		 */
		if (strncmp(name, label, name_length) == 0 && 
			strlen(label) - 1 == name_length) {
			return(mi);
		}
	}

	return(NULL);
}

/*
 * Load a template file.  Parse any header fields it may have
 */
mt_load_template(file, rpd)

	char		*file;
	struct	reply_panel_data *rpd;

{
	char		*file_buf;
	int		file_size;

	/* Mmap in file */
	if ((file_buf = (char *)ck_mmap(file, &file_size)) == NULL) {
		return -1 ;
	};

	/* Load template into the compose window */
	mt_load_template_memory(file_buf, file_size, rpd);

	/* Unmap file */
	ck_unmap(file_buf, file_size);

	return 0;
}

/*
 * Load a template from memory
 */
mt_load_template_memory(buffer, size, rpd)

	char		*buffer;
	int		size;
	struct reply_panel_data	*rpd;

{
	u_char		*body;
	u_char		*mt_skip_header();
	int		len;
	int		lower_context;
	struct header_obj	hdr;

	/* Skip over the message header to find the start of the body.
	 * If there is no header than body == buffer
	 */
	body = mt_skip_header(buffer, size);

	if ((char *) body != buffer) {
		/* File has a header.  Parse it.  "body" sits at the blank line
		 * between the msg headers and contents, skip the blank line.
		 */
		hdr.hdr_start = buffer;
		hdr.hdr_end = (char *) body++;
		hdr.hdr_next = NULL;
		hdr.hdr_allocated = 0;
		hdr.hdr_single_val = 0;

		layout_panel = FALSE;
		/* Have mt_fill_field() called for each header field */
		header_enumerate(&hdr, mt_fill_field, rpd);

		if (layout_panel) {
			/* Layout panel */
			mt_compose_frame_layout_proc(rpd->frame);
		}
	}

	lower_context = (int)xv_get(rpd->replysw, TEXTSW_LOWER_CONTEXT);
	(void)xv_set(rpd->replysw, TEXTSW_LOWER_CONTEXT, -1, 0);
	/* Insert body of template into compose textsw */
	len = size - ((char *) body - buffer);
	if (len > 0) {
		mt_text_clear_error(rpd->replysw);
		mt_text_insert(rpd->replysw, (char *) body, len);
	}
	(void)xv_set(rpd->replysw, TEXTSW_LOWER_CONTEXT, lower_context, 0);

	return;

}

u_char *
mt_skip_header(buf, bufsiz)

	u_char	*buf;
	int	bufsiz;


{
	u_char	*end;
	u_char	*bufend;
	u_char	*colon;

	/* 
	 * Check if buf has a message header on it.  If it does
	 * skip over it
	 */
	bufend = buf + bufsiz;
	while (!isspace(*buf)) {

		end = (u_char *)findend_of_field(buf, bufend);
		colon = (u_char *)findchar(':', buf, end);
		if (*colon != ':')
			break;

		if (!valid_field(buf, colon))
			break;

		buf = end;
	}

	return(buf);
}

static int
valid_field(start_p, end_p)

	register u_char	*start_p;
	register u_char	*end_p;

{
	/* Make sure field has no embedded white space
	 * Technically white space is permitted by RFC-822, but if we 
	 * allow it in template fields, than we frequently end up
	 * adding something as a field which the user did not intend
	 * to be a field.
	 */
	while (start_p != end_p) {
		if (isspace(*start_p++))
			return(FALSE);
	}

	return(TRUE);
}
