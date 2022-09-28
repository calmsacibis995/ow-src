#ifndef lint
static  char sccsid[] = "@(#)props.c 3.81 94/09/16 Copyr 1991 Sun Microsystems, Inc.";
#endif
/* props.c */

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/textsw.h>
#include <xview/panel.h>
#include <xview/scrollbar.h>
#include <xview/font.h>
#include <xview/notice.h>
#include <sys/stat.h>
#include "ds_listbx.h"
#include "util.h"
#include "timeops.h"
#include "datefield.h"
#include "props.h"
#include "editor.h"
#include "appt.h"
#include "graphics.h"
#include "browser.h"
#include "common.h"
#include "blist.h"
#include "calendar.h"
#include "table.h"
#include "weekglance.h"
#include "dayglance.h"
#include "select.h"
#include "ds_popup.h"
#include "gettext.h"
#include "alist.h"
#include "todo.h"
#include "defnamesvc.h"
#include "ds_xlib.h"
#if 0
#include "namesvc.h"
#endif
#include <limits.h>
#include <sys/param.h>

#define COMMENT		'#'
#define CONTINUATION	'\\'
#define ADD_OP		0
#define APPLY_OP        1	

#define NAMELEN             23 
#define TIMEBUF             20 
#define PL_WIDTH            250

int entrylen;

static char  *b_str, *i_str, *d_str;

extern int debug;
static 	void	add_entry_to_box();

static Props_entry      *p_entries = NULL;

char *unitstr[3];
static char *unitstrASCII[3] = {"Mins", "Hrs", "Days"};
char *privacystr[3];
static char *privacystrASCII[3] = {"Show Time and Text", "Show Nothing", "Show Time Only"};
static XrmDatabase     dsdb, rdb;    /* handle to resource db's */

Xv_Font box_font;

/* if you add to this you must also add a define to props.h */
char	*property_names[] = {
	 "Calendar.BeepOn",
	 "Calendar.BeepAdvance",
	 "Calendar.BeepUnit",
	 "Calendar.FlashOn",
	 "Calendar.FlashAdvance",
	 "Calendar.FlashUnit",
	 "Calendar.OpenOn",
	 "Calendar.OpenAdvance",
	 "Calendar.OpenUnit",
	 "Calendar.MailOn",
	 "Calendar.MailAdvance",
	 "Calendar.MailUnit",
	 "Calendar.MailTo",
	 "Calendar.UnixOn",
	 "Calendar.UnixAdvance",
	 "Calendar.UnixCommand",
	 "Calendar.DayBegin",
	 "Calendar.DayEnd",
	 "Calendar.CalendarList",
	 "Calendar.DefaultView",
	 "Calendar.DefaultDisplay",
	 "Calendar.PrintDest",
	 "Calendar.PrintPrivacy",
	 "Calendar.PrinterName",
	 "Calendar.PrintOptions",
	 "Calendar.PrintDirName",
	 "Calendar.PrintFileName",
	 "Calendar.PrintWidth",
	 "Calendar.PrintHeight",
	 "Calendar.PrintPosXOff",
	 "Calendar.PrintPosYOff",
	 "Calendar.PrintUnit",
	 "Calendar.PrintCopies",
	 "Calendar.DefaultCal",
	 "Calendar.Location",
	 "Calendar.DateOrdering",
	 "Calendar.DateSeparator",
	 "Calendar.Privacy",
	NULL};

static Boolean props_changed; /* for switching props panels with changes */
static void p_create_edefs(), p_create_dispset();
static void p_create_access(), p_create_printer(), p_category_notify();
static void p_create_dateformat();
static void p_show_edefs(), p_show_dispset();
static void p_show_access(), p_show_printer();
static void p_show_dateformat();
static void p_turnoff_last_pane();
static Notify_value beginslider_notify(), endslider_notify(), changed_notify();
static Panel_setting changed_text_notify();
static void p_reset_proc(), p_apply_proc();
static void accesslist_proc(), change_accesslist();

extern Boolean
props_showing(p)
        Props *p;
{
        if (p != NULL && p->frame != NULL)
                return true;
        return false;
}

extern char *
str_to_entry(s, cal, perm)
        char *s;
        char **cal;
        char **perm;
{
	char *tmp_ptr, *blank_ptr;

	*cal = NULL; *perm = NULL;

	*cal = cm_strdup(s);
	blank_ptr = strchr(*cal, ' ');
	if (blank_ptr != NULL)
		*blank_ptr = NULL;

	tmp_ptr = strrchr(s, ' ');
	if (tmp_ptr != NULL)
		*perm = cm_strdup(++tmp_ptr);
	
	
}
static Notify_value
p_frame_done(frame)
	Frame frame;
{
	Calendar *c = (Calendar*)xv_get(frame, WIN_CLIENT_DATA);
	Props *p = (Props*)c->properties;
	int not_val, view;
	extern void p_display_vals();

	if (!props_changed) {
		xv_set(frame, XV_SHOW, FALSE, NULL);
		return NOTIFY_DONE;
	}
	not_val = notice_prompt(frame, NULL,
		NOTICE_MESSAGE_STRINGS,
		MGET("You Have Made Changes That Have\nNot Been APPLIED. Do You Still\nWant To Dismiss Window?"),
		0,
		NOTICE_BUTTON_YES, LGET("Dismiss"),
		NOTICE_BUTTON_NO, LGET("Cancel"),
		0);

	if (not_val == NOTICE_YES) {
		xv_set(frame, XV_SHOW, FALSE, NULL);
		view = xv_get(p->category_item, PANEL_VALUE);
		p_display_vals(p, view);
	}
	else {
		xv_set(frame, FRAME_CMD_PUSHPIN_IN, TRUE, 
			 XV_SHOW, TRUE, NULL);
		props_changed = false;
	}
	return NOTIFY_DONE;
}
static void
free_props_entries(base)
	Props_entry	**base;
{

	Props_entry	*p_ptr, *s_ptr;

	p_ptr = *base;

	while (p_ptr) {
		if (p_ptr->property_name)
			free(p_ptr->property_name);
		if (p_ptr->property_value)
			free(p_ptr->property_value);
		s_ptr = p_ptr;
		p_ptr = p_ptr->next;
		free(s_ptr);
	}
	*base = NULL;
}

static Boolean
propentry_in_list(property_name, entry_ptr)
	char *property_name;
	Props_entry **entry_ptr;
{
	Props_entry	*p_ptr;

	p_ptr = p_entries;
	while (p_ptr) {
		if (strcmp(p_ptr->property_name, property_name) == 0) {
			*entry_ptr = p_ptr;
			return true;
		}
		p_ptr = p_ptr->next;
	}
	return false;
}

static int
cm_read_cmrc(file_name)
char	*file_name;
{
	FILE	*rc_file;
	char	buffer[BUFSIZ];
	char 	*c_ptr, *d_ptr;
	int	content_gotten;
	Props_entry	*new_prop;
	char	cal_default[2*BUFSIZ];

	/* look for any occurrence of that property */

	if (p_entries)
		free_props_entries(&p_entries);
	if ((rc_file = fopen(file_name, "r")) == NULL)
		return(FALSE);
	while ((int)fgets(buffer, BUFSIZ-1, rc_file)) {
	
		/* find the colon (:) within the line.  This
		delimits the name of the property */

		c_ptr = strchr(buffer, ':');

		if (c_ptr) {

			/* there was a colon.  Otherwise, junk the
			line. */

			*c_ptr = NULL;
	
			new_prop = (Props_entry *) ckalloc(sizeof(Props_entry));
			new_prop->property_name = cm_strdup(buffer);
			new_prop->update = 1;
			new_prop->next = p_entries;
			p_entries = new_prop;
		}
		else 
			continue;


		/* Initialize the buffer.  After
		that,  get string text out of the buffer
		until the last line without a newline,
		or until there is a comment sign on one
		of the lines in the property.  Lines
		are continued with backslash characters.
		Comments are in # signs. */

		d_ptr = cal_default;
		*d_ptr = NULL;
		
		c_ptr++;


		/* compress out white space after a
		   property name */

		while (c_ptr != NULL && *c_ptr != NULL
			&& (*c_ptr == ' ' || *c_ptr == '\t'))
			c_ptr++;

		/* if nothing left, bail. */

		if (c_ptr == NULL || *c_ptr == NULL) {
			new_prop->property_value = cm_strdup("");
			continue;
		}
	
		content_gotten = FALSE;
		while (!content_gotten) {
			/* grab characters until a comment, or the
			end of the last line without a continuation
			mark */

			while (*c_ptr && (*c_ptr != COMMENT))
				*d_ptr++ = *c_ptr++;

			if (*c_ptr == COMMENT) {
			/* comment terminated the line.  Bail */

				*d_ptr++ = NULL;
				new_prop->property_value = cm_strdup(cal_default);
				content_gotten = TRUE;
			}
			else if (*(c_ptr - 2) != CONTINUATION) {
			/* was there a continuation mark in the
			   last column of the line? */
				/* If not,  bail */

				*d_ptr++ = NULL;

				/* rip out any terminating newlines */

				if (cal_default[cm_strlen(cal_default) -1] == '\n')
					cal_default[cm_strlen(cal_default) - 1] = NULL;
				new_prop->property_value = cm_strdup(cal_default);
					content_gotten = TRUE;
			}
			else if (!fgets(buffer, BUFSIZ, rc_file)) {

			/* A continued line.  Reset the buffer,
			   insert a space, and continue until a
			   line without continuation,  or a comment. */
			
			/* If no more, bail */

				*d_ptr++ = NULL;
				new_prop->property_value = cm_strdup(cal_default);
				content_gotten = TRUE;
			}

			d_ptr -= 2;
			c_ptr = buffer;
		}
	}
	(void) fclose(rc_file);
	return true;
}

static int
cm_read_dsfile(file_name)
char	*file_name;
{
	char 	*resource = NULL, *p_ptr;
	Props_entry	*entry_ptr, *new_prop;
	int	ret_value=false, i;

	/* .desksetdefaults file */
	for (i = 0; i <= NUM_PROPS; i++) {
		p_ptr = strrchr(property_names[i], '.');
		resource = ds_get_resource(rdb, "calendar", ++p_ptr);
		if (resource != NULL) { 
			if (!propentry_in_list(property_names[i], &entry_ptr)) {
				new_prop = (Props_entry*)ckalloc(sizeof(Props_entry));
				new_prop->property_name = cm_strdup(property_names[i]);
				new_prop->property_value = cm_strdup(resource);
				new_prop->update = 0;
				new_prop->next = p_entries;
				p_entries = new_prop;
			}
			else {
				free(entry_ptr->property_value);
				entry_ptr->property_value = cm_strdup(resource);
				entry_ptr->update = 0;
			}
			ret_value = true;
		}
	}
	return ret_value;
}

extern char *
cm_get_property(property_name)
	char *property_name;
{
	Props_entry	*p_ptr;

	/* search through the property structure to
	   determine the value of the requested property */

	p_ptr = p_entries;
	while (p_ptr) {
		if (!strcmp(p_ptr->property_name, property_name))
			if (*p_ptr->property_value != NULL)
				return(p_ptr->property_value);
			else
				return(NULL);
		p_ptr = p_ptr->next;
	}
	return(NULL);
}

extern int
cm_get_boolean_property(property_name)
	char	*property_name;
{
	char	*p_p = cm_get_property(property_name);

	if (p_p)
	{
		if ((*p_p == 'Y') || (*p_p == 'T') || (*p_p == '1'))
			return(1);
		else
			return(0);
	}
	else
		return(-1);
}

static int
cal_new_prop(property_name, value)
	char	*property_name;
	char	*value;
{
	Props_entry	*new_prop;

	new_prop = (Props_entry *) ckalloc(sizeof(Props_entry));
        new_prop->property_name = cm_strdup(property_name);
        new_prop->property_value = cm_strdup(value);
        new_prop->update = 1;
	new_prop->next = p_entries;
	p_entries = new_prop;
}
	

extern void
cal_set_property(property_name, value)
	char	*property_name;
	char	*value;
{
	Props_entry *p_entry = p_entries;

/*	if (*value == NULL)
		return;
		                         /* allow null options to clear */
					 /* existing value in file */
					 /* bug 1143863 (scavin 22Sep93) */
	while (p_entry) {
		if (!strcmp(p_entry->property_name, property_name)) {
			if (p_entry->property_value != NULL)
				free(p_entry->property_value);
			p_entry->property_value = cm_strdup(value);
			p_entry->update = 1;
			return;
		}
		p_entry = p_entry->next;
	}

	if (!p_entry)
		cal_new_prop(property_name, value);
}
extern Xv_opaque
cal_set_properties()
{
	Calendar *c = calendar;
	Props *p = (Props*)c->properties;
	Props_entry *p_ptr;
	char *c_ptr;
	Xv_opaque stat;

	for (p_ptr = p_entries; p_ptr != NULL; p_ptr = p_ptr->next) {
		if (p_ptr->update) {
			c_ptr = strrchr(p_ptr->property_name, '.');
			if (c_ptr != NULL)
				ds_put_resource(&dsdb, "calendar", 
					++c_ptr, p_ptr->property_value);
		}
	}
	stat = ds_save_resources(dsdb);
	if (stat != XV_OK)
		notice_prompt(c->frame, (Event *)NULL,
                        NOTICE_MESSAGE_STRING,
			EGET("Cannot save .desksetdefaults database.\nMake sure you have access to this file."),
                        0,
                        NOTICE_BUTTON_YES,  LGET("Continue"),
                0);
	return stat;
}

static bool_t
cal_propunchanged(buffer)
	char	buffer[];
{
	Props_entry *p_entry = p_entries;

	for (p_entry = p_entries; p_entry != NULL; p_entry = p_entry->next) {
		if ((!strncmp(buffer, p_entry->property_name, 
			cm_strlen(p_entry->property_name))) 
			&& (p_entry->update == 1)) {
			if (p_entry->update == 0)
				return(TRUE);
			return(FALSE);
		}
	}
	return(TRUE);
}

/* moving .cm.rc props to .desksetdefaults */
extern void
cal_convert_cmrc()
{
	struct	stat	statbuf;
	char	*resource = NULL, file_name[MAXNAMELEN];

	dsdb = ds_load_deskset_defs();
	rdb = ds_load_resources(
			(Display*)xv_get(calendar->frame, XV_DISPLAY));
	if (rdb != NULL)
		resource = ds_get_resource(rdb, "calendar", "Upgraded");
	/* moved .cm.rc data to .desksetdefaults */
	if (resource == NULL) {
		sprintf(file_name, "%s/%s", getenv("HOME"), RC_FILENAME);
		/* .cm.rc file exists; read it and write to .desksetdefaults */
		if (stat(file_name, &statbuf) != -1) {
			if (cm_read_cmrc(file_name)) {
				ds_put_resource(&dsdb, "calendar", 
					"Upgraded", "true");
				cal_set_properties();
				return;	
			}
		}
		else
			ds_put_resource(&dsdb, "calendar", 
				"Upgraded", "true");
		ds_save_resources(dsdb);
	}
}
extern Boolean
cal_update_props()
{
	struct	stat	statbuf;
	char	*ds, file_name[MAXNAMELEN];
	Calendar *c = calendar;
	int status=-1;
	Boolean ret_value = false;

	/* read .desksetdefaults file */
	if ((ds = getenv("DESKSETDEFAULTS")) == NULL) {
		sprintf(file_name, "%s/%s", getenv("HOME"), 
			DS_FILENAME);
		status = stat(file_name, &statbuf);
	}
	else
		status = stat(ds, &statbuf);
	if (status != -1 && statbuf.st_mtime != c->general->rc_ts) {
		rdb = ds_load_resources(
			(Display*)xv_get(c->frame, XV_DISPLAY));
		cm_read_dsfile(file_name);
		c->general->rc_ts = statbuf.st_mtime;
		ret_value = true;
	}
	return ret_value;
}

static void
cal_set_boolean_property(property_name, on)
	char	*property_name;
	int	on;
{
	if (on)
		cal_set_property(property_name, "True");
	else
		cal_set_property(property_name, "False");
}

static void
format_time(time, buf, display)
	int time; 
	char *buf;
	DisplayType display;
{
	if(time > 24*60) return;

	if (display == hour12) {
        	if(time == 0 || time == 24*60)
                	(void) cm_strcpy(buf,  MGET("midnight") );
        	else if(time == 12*60)
                	(void) cm_strcpy(buf,  MGET("noon") );
        	else if(time < 1*60)
               		 (void) sprintf(buf, "12:%02d a.m.", time%60);
        	else if(time < 12*60)
                	(void) sprintf(buf, "%2d:%02d a.m.", time/60, 
					time%60);
        	else if(time < 13*60)
                	(void) sprintf(buf, "12:%02d p.m.", time%60);
        	else
                	(void) sprintf(buf, "%2d:%02d p.m.",
					(time-12*60)/60, time%60);	
	}
	else { /* hour24 */
        	if(time == 0 || time == 24*60)
                	(void) cm_strcpy(buf,  MGET("0000") );
        	else if(time == 12*60)
                	(void) cm_strcpy(buf,  MGET("1200") );
        	else if(time < 1*60)
               		 (void) sprintf(buf, "00%02d", time%60);
        	else if(time < 24*60)
                	(void) sprintf(buf, "%02d%02d", time/60, 
					time%60);
	}

}
static char*
perms_int_to_str(perms_int)
	int 	perms_int;
{
	char *perms_str;

	/* Permission strings are not translated so don't worry about allocating
	 * enough to fit the translated multibyte string.
	 */
	perms_str = (char *)ckalloc(cm_strlen(b_str)+cm_strlen(i_str)+cm_strlen(d_str) + 5);

	if (perms_int & access_read) 
		cm_strcpy(perms_str, b_str);
	if (perms_int & access_write) 
		cm_strcat(perms_str, i_str);
	if (perms_int & access_delete) 
		cm_strcat(perms_str, d_str);

	return	perms_str;
}

static int
perms_str_to_int(perms_str)
	char 	*perms_str;
{
	int 	perms_int = 0, b_len, i_len, d_len;

	b_len = cm_strlen(b_str);
	i_len = cm_strlen(i_str);
	d_len = cm_strlen(d_str);
	while (perms_str != NULL && *perms_str != '\0') {
		if (strncmp(perms_str, b_str, b_len) == 0) {
			perms_str += b_len;
			perms_int |= access_read;
		}
		if (strncmp(perms_str, i_str, i_len) == 0) {
			perms_str += i_len;
			perms_int |= access_write;
		}
		if (strncmp(perms_str, d_str, d_len) == 0) {
			perms_str += d_len;
			perms_int |= access_delete;
		}
	}

	return perms_int;
}
/* ARGSUSED */
static void
dest_choice_proc(item, value, event)
	Panel_item      item;
	int             value;
	Event           *event;

{
        Calendar *c = (Calendar *) xv_get(item, PANEL_CLIENT_DATA);
        Props *p = (Props *) c->properties;

        if (value)
        {
                xv_set(p->printer_name, XV_SHOW, FALSE, 0);
                xv_set(p->printerstr, XV_SHOW, FALSE, 0);
                xv_set(p->options, XV_SHOW, FALSE, 0);
                xv_set(p->optionstr, XV_SHOW, FALSE, 0);
                xv_set(p->file_name, XV_SHOW, TRUE, 0);
                xv_set(p->filestr, XV_SHOW, TRUE, 0);
                xv_set(p->dir_name, XV_SHOW, TRUE, 0);
                xv_set(p->dirstr, XV_SHOW, TRUE, 0);
        }
        else
        {   
                xv_set(p->file_name, XV_SHOW, FALSE, 0);
                xv_set(p->filestr, XV_SHOW, FALSE, 0);
                xv_set(p->dir_name, XV_SHOW, FALSE, 0);
                xv_set(p->dirstr, XV_SHOW, FALSE, 0);
                xv_set(p->printer_name, XV_SHOW, TRUE, 0);
                xv_set(p->printerstr, XV_SHOW, TRUE, 0);
                xv_set(p->options, XV_SHOW, TRUE, 0);
                xv_set(p->optionstr, XV_SHOW, TRUE, 0);
        }
	if (event)
		props_changed = true;
}

extern void
p_display_vals(p, view)
	Props   *p;
	int 	view;
{
	char *pstr, buf[TIMEBUF];
	Access_Entry *list = NULL;

	switch (view)
	{
		case EDITOR_DEFAULTS:
			(void) xv_set(p->reminder, PANEL_VALUE, p->reminder_VAL,     0);
			(void) xv_set(p->beepadvance, PANEL_VALUE, p->beepadvance_VAL,  0);
			(void) xv_set(p->beepunit, PANEL_LABEL_STRING,
				 unitstr[p->beepunit_VAL],  0);
			(void) xv_set(p->flashadvance,PANEL_VALUE, p->flashadvance_VAL, 0);
			(void) xv_set(p->flashunit, PANEL_LABEL_STRING, 
					unitstr[p->flashunit_VAL], 0);
			(void) xv_set(p->openadvance, PANEL_VALUE, p->openadvance_VAL,  0);
			(void) xv_set(p->openunit, PANEL_LABEL_STRING,
					 unitstr[p->openunit_VAL],  0);
			(void) xv_set(p->mailadvance, PANEL_VALUE, p->mailadvance_VAL,  0);
			(void) xv_set(p->mailunit, PANEL_LABEL_STRING, unitstr[p->mailunit_VAL],  0);
			(void) xv_set(p->mailto, PANEL_VALUE, p->mailto_VAL,  0);
			(void) xv_set(p->privacyunit, PANEL_LABEL_STRING, privacystr[p->privacyunit_VAL],  0);
			break;

		case  DISPLAY_SETTINGS:
			(void) xv_set(p->beginslider, PANEL_VALUE, p->begin_slider_VAL, 0);
			(void) xv_set(p->endslider,   PANEL_VALUE, p->end_slider_VAL,   0);	
			if (p->default_disp_VAL == hour12)
				(void) xv_set(p->default_disp, PANEL_VALUE, 0, 0);	
			else
				(void) xv_set(p->default_disp, PANEL_VALUE, 1, 0);	
			(void) xv_set(p->default_view,PANEL_VALUE, p->default_view_VAL, 0);	
			(void) xv_set(p->defcal, PANEL_VALUE, p->defcal_VAL, 0);	
			(void) xv_set(p->cloc, PANEL_VALUE, p->cloc_VAL, 0);	

			format_time(p->begin_slider_VAL*60, buf, p->default_disp_VAL);
			(void) xv_set(p->beginslider_str, PANEL_LABEL_STRING, buf, 0);

			format_time(p->end_slider_VAL*60, buf, p->default_disp_VAL);
			(void) xv_set(p->endslider_str, PANEL_LABEL_STRING, buf, 0);
			break;
		case GROUP_ACCESS_LISTS:
			(void) xv_set(p->perms, PANEL_VALUE, p->perms_VAL, 0);
			(void) xv_set(p->username, PANEL_VALUE, "", 0);
			list_cd_flush(p->access_list);
			if (table_get_access(calendar->calname, &list) == status_ok) {
				for (; list != NULL; list=list->next) {
					pstr = (char*)perms_int_to_str(list->access_type);
                                	add_entry_to_box(p->access_list,
					 	box_font, list->who, 
						cm_strdup(list->who), pstr,
						(int)xv_get(p->access_list, 
						PANEL_LIST_NROWS));
                                	free(pstr);
				}
			}
                        break;
		case PRINTER_OPTS:
			(void) xv_set(p->dest_choice, PANEL_VALUE, p->dest_choiceVAL, 0);
			(void) xv_set(p->meo, PANEL_VALUE, p->meoVAL, 0);
			(void) xv_set(p->printer_name, PANEL_VALUE, p->printer_nameVAL, 0);
			(void) xv_set(p->dir_name, PANEL_VALUE, p->dir_nameVAL, 0);
			(void) xv_set(p->file_name, PANEL_VALUE, p->file_nameVAL, 0);
			(void) xv_set(p->options, PANEL_VALUE, p->optionVAL, 0);
			(void) xv_set(p->scale_height, PANEL_VALUE, p->heightVAL, 0);
			(void) xv_set(p->scale_width, PANEL_VALUE, p->widthVAL, 0);
			(void) xv_set(p->offset_width, PANEL_VALUE, p->xoffsetVAL, 0);
			(void) xv_set(p->offset_height, PANEL_VALUE, p->yoffsetVAL, 0);
			(void) xv_set(p->repeat_times, PANEL_VALUE, atoi(p->repeatVAL), 0);
			(void) xv_set(p->copies, PANEL_VALUE, atoi(p->copiesVAL), 0);
			dest_choice_proc(p->dest_choice, p->dest_choiceVAL, 0);
                        break;
		case DATE_FORMAT:
			(void) xv_set(p->ordering_list, PANEL_VALUE, p->ordering_VAL, NULL);
			(void) xv_set(p->separator_list, PANEL_VALUE, p->separator_VAL, NULL);
			break;
	}
}
extern void
set_tempdef_vals(p, view)
	Props	*p;
	int view;
{
	char *pathname=NULL, *def_pr=NULL;
	char path[MAXPATHLEN];
	char buf[TIMEBUF];

	switch (view) {
                case EDITOR_DEFAULTS:
			(void) xv_set(p->reminder, PANEL_VALUE, 
					REMINDER_DEF, NULL); 
			(void) xv_set(p->beepadvance, PANEL_VALUE, 
					BEEPADV_DEF,  0);
			(void) xv_set(p->beepunit, PANEL_LABEL_STRING,
					 unitstr[0],  NULL);
                        (void) xv_set(p->flashadvance,PANEL_VALUE, 
					FLASHADV_DEF, 0);
			(void) xv_set(p->flashunit, PANEL_LABEL_STRING,
					unitstr[0], NULL);
		        (void) xv_set(p->openadvance, PANEL_VALUE, 
					OPENADV_DEF,  0);
                        (void) xv_set(p->openunit, PANEL_LABEL_STRING,
					 unitstr[0],  NULL);
			(void) xv_set(p->mailadvance, PANEL_VALUE, 
					MAILADV_DEF,  0);
			(void) xv_set(p->mailunit, PANEL_LABEL_STRING,
					 unitstr[1],  NULL);
                        (void) xv_set(p->mailto, PANEL_VALUE, 
					calendar->user, NULL);
                        (void) xv_set(p->privacyunit, PANEL_LABEL_STRING, 
					privacystr[0], NULL);
                        break;
 
                case  DISPLAY_SETTINGS:
                        (void) xv_set(p->beginslider, PANEL_VALUE, 
					(int)BEGINS_DEF, 0);
                        (void) xv_set(p->endslider, PANEL_VALUE, 
					(int)ENDS_DEF, 0);
			(void) xv_set(p->default_disp, PANEL_VALUE, 
					DISP_DEF, 0);	
			format_time(BEGINS_DEF*60, buf, 
					p->default_disp_VAL);
                        (void) xv_set(p->beginslider_str, 
					PANEL_LABEL_STRING, buf, 0);
                        format_time(ENDS_DEF*60, buf, 
					p->default_disp_VAL);
			(void) xv_set(p->endslider_str, 
					PANEL_LABEL_STRING, buf, 0);
			(void) xv_set(p->default_view, PANEL_VALUE, 
					VIEW_DEF, 0);	
			(void) xv_set(p->defcal, PANEL_VALUE, 
					calendar->calname, 0);	
			(void) xv_set(p->cloc, PANEL_VALUE, 
					cm_get_local_host(), 0);	
			break;
		case GROUP_ACCESS_LISTS:
			list_cd_flush(p->access_list);
			(void)xv_set(p->username, PANEL_VALUE, "", 
					NULL);
			(void)xv_set(p->perms, PANEL_VALUE, 1, NULL);
			add_entry_to_box(p->access_list,
                                box_font, WORLD,
                                cm_strdup(WORLD), b_str, 0);
                        break;
		case PRINTER_OPTS:
			(void) xv_set(p->dest_choice, PANEL_VALUE, 
					CHOICE_DEF, 0);
			(void) xv_set(p->meo, PANEL_VALUE, (int)MEO_DEF, 0);
			def_pr = (char*)ds_def_printer();
			(void) xv_set(p->printer_name, PANEL_VALUE,
                                         def_pr, 0);
			free(def_pr);
#ifdef SVR4
			if ((pathname = getcwd(NULL, BUFSIZ)) == NULL)
				perror("getcwd");
			(void) xv_set(p->dir_name, PANEL_VALUE, 
					pathname, 0);
			free(pathname);
#else
			getwd(path);
			(void) xv_set(p->dir_name, PANEL_VALUE, 
					path, 0);
#endif "SVR4"
			(void) xv_set(p->file_name, PANEL_VALUE, 
					PSCAL_DEF, 0);
			(void) xv_set(p->options, PANEL_VALUE, 
					OPTIONS_DEF, 0);
			(void) xv_set(p->scale_height, PANEL_VALUE, 
					SCALEH_DEF, 0);
			(void) xv_set(p->scale_width, PANEL_VALUE, 
					SCALEW_DEF, 0);
			(void) xv_set(p->offset_width, PANEL_VALUE, 
					XWID_DEF, 0);
			(void) xv_set(p->offset_height, PANEL_VALUE, 
					XHGT_DEF, 0);
			(void) xv_set(p->repeat_times, PANEL_VALUE, 
					REP_DEF, 0);
			(void) xv_set(p->copies, PANEL_VALUE, 
					COPIES_DEF, 0);
			dest_choice_proc(p->dest_choice, CHOICE_DEF, 0);
                        break;
		case DATE_FORMAT:
			(void) xv_set(p->ordering_list, PANEL_VALUE, 
					ORDER_DEF, NULL);
			(void) xv_set(p->separator_list, PANEL_VALUE,
					SEP_DEF, NULL);
			break;
	}
}

extern void
set_default_vals(p, view)
	Props	*p;
	int view;
{
	static int already_set;
	char *pathname=NULL;
	char *def_pr=NULL;

	if ( !already_set ) {
		unitstr[0] = (char *)cm_strdup(MGET("Mins"));
		unitstr[1] = (char *)cm_strdup(MGET("Hrs")); 
		unitstr[2] = (char *)cm_strdup(MGET("Days"));

		privacystr[0] = (char *)cm_strdup(MGET("Show Time and Text")); 
		privacystr[1] = (char *)cm_strdup(MGET("Show Nothing")); 
		privacystr[2] = (char *)cm_strdup(MGET("Show Time Only")); 
		/* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL :
	 	 * If the following strings are translated,
		 * the message string below the access list should
		 * also be translated.
		 * Asian locales do not need to translate them
	 	 */

		b_str = MGET("B");
		i_str = MGET("I");
		d_str = MGET("D");
		entrylen = (int)(NAMELEN + cm_strlen(b_str) + cm_strlen(i_str) + cm_strlen(d_str) + MB_LEN_MAX + 1);
		already_set = 1;
	}

	switch (view)
	{
	  case EDITOR_DEFAULTS:
		p->reminder_VAL	= REMINDER_DEF;
		cm_strcpy(p->beepadvance_VAL, BEEPADV_DEF);
		p->beepunit_VAL  = 0;  /* mins */

		cm_strcpy(p->flashadvance_VAL, FLASHADV_DEF);
		p->flashunit_VAL = 0;   /* mins */

		cm_strcpy(p->mailadvance_VAL, MAILADV_DEF);
		p->mailunit_VAL = 1;  /* hrs */

		cm_strcpy(p->openadvance_VAL, OPENADV_DEF);
		p->openunit_VAL = 0;  /* mins */
		cm_strcpy(p->mailto_VAL, calendar->user);
		p->privacyunit_VAL = 0;
		break;

	   case DISPLAY_SETTINGS:
		p->begin_slider_VAL	= BEGINS_DEF;
		p->end_slider_VAL	= ENDS_DEF;
		p->default_view_VAL	= VIEW_DEF;
		p->default_disp_VAL	= DISP_DEF;
		cm_strcpy(p->defcal_VAL, calendar->calname);
		cm_strcpy(p->cloc_VAL, cm_get_local_host());
		break;

	   case GROUP_ACCESS_LISTS:
		p->perms_VAL = 1;
		break;
	   case PRINTER_OPTS:
        	p->dest_choiceVAL = CHOICE_DEF;
        	p->meoVAL = (int)MEO_DEF;
		def_pr = (char*)ds_def_printer();
		cm_strcpy(p->printer_nameVAL, def_pr);
		free(def_pr);
        	cm_strcpy(p->optionVAL, OPTIONS_DEF);
#ifdef SVR4
		if ((pathname = getcwd(NULL, BUFSIZ)) == NULL)
			perror("cm: getcwd");
        	cm_strcpy(p->dir_nameVAL, pathname);
		free(pathname);
#else
		getwd(p->dir_nameVAL);
#endif

        	cm_strcpy(p->file_nameVAL, PSCAL_DEF);
        	cm_strcpy(p->heightVAL, SCALEH_DEF);
        	cm_strcpy(p->widthVAL, SCALEW_DEF);
        	cm_strcpy(p->xoffsetVAL, XWID_DEF);
        	cm_strcpy(p->yoffsetVAL, XHGT_DEF);
        	sprintf(p->repeatVAL, "%d", REP_DEF);
        	sprintf(p->copiesVAL, "%d", COPIES_DEF);
		break;
	   case DATE_FORMAT:
		p->ordering_VAL = (Ordering_Type)ORDER_DEF;
		p->separator_VAL = (Separator_Type)SEP_DEF;
		break;
	}
}
static int
unitstr_to_int(str)
	char *str;
{
	int ret = 0;

	if (str == NULL) 
		ret = 0;
	else if (strcmp(str,  MGET("Mins") ) == 0)
		ret = 0;
	else if (strcmp(str,  MGET("Hrs") ) == 0)
		ret = 1;
	else if (strcmp(str,  MGET("Days") ) == 0)
		ret = 2;

	return ret;
}

/* convert ascii label from cm resource */
static int
unitstrASCII_to_int(str)
char *str;
{
	int ret = 0;

	if (str == NULL)
		ret = 0;
	else if (strcmp(str, "Mins") == 0)
		ret = 0;
	else if (strcmp(str, "Hrs") == 0)
		ret = 1;
	else if (strcmp(str, "Days") == 0)
		ret = 2;

	return ret;
}
extern int
privacystr_to_int(ps)
char *ps;
{
        int pr_val = public;
 
        if (strcmp(ps, MGET("Show Time and Text")) == 0)
                pr_val = public;
        else if (strcmp(ps, MGET("Show Time Only")) == 0)
                pr_val = semiprivate;
        else if (strcmp(ps, MGET("Show Nothing")) == 0)
                pr_val = private;
 
        return pr_val;
}

/* convert ascii label from cm resource */
static int
privacystrASCII_to_int(str)
char *str;
{
	int ret = 0;

	if (str == NULL)
		ret = 0;
	else if (strcmp(str, "Show Time and Text") == 0)
		ret = 0;
	else if (strcmp(str, "Show Nothing") == 0)
		ret = 1;
	else if (strcmp(str, "Show Time Only") == 0)
		ret = 2;

	return ret;
}

extern void
set_rc_vals(p, view)
	Props	*p;
	int 	view;
{
	char	*value_p, *loc;
	int	boolean, reminder_buf = 0;
	Access_Entry	*list = NULL;
	Calendar *c = calendar;

	switch (view)
	{
	  case EDITOR_DEFAULTS:
		boolean = cm_get_boolean_property(property_names[CP_BEEPON]);
		if (boolean == 1)
			reminder_buf |= 0x1;

		boolean = cm_get_boolean_property(property_names[CP_FLASHON]);
		if (boolean == 1)
			reminder_buf |= 0x2;

		boolean = cm_get_boolean_property(property_names[CP_OPENON]);
		if (boolean == 1)
			reminder_buf |= 0x4;

		boolean = cm_get_boolean_property(property_names[CP_MAILON]);
		if (boolean == 1)
			reminder_buf |= 0x8;

		boolean = cm_get_boolean_property(property_names[CP_UNIXON]);
		if (boolean == 1)
			reminder_buf |= 0x10;

		p->reminder_VAL	= reminder_buf;
		value_p = cm_get_property(property_names[CP_BEEPADV]);
		if (value_p)
			(void) cm_strcpy(p->beepadvance_VAL,  value_p);
		value_p = cm_get_property(property_names[CP_BEEPUNIT]);
		if (value_p)
			p->beepunit_VAL =  unitstrASCII_to_int(value_p);
		value_p = cm_get_property(property_names[CP_FLASHADV]);
		if (value_p)
			(void) cm_strcpy(p->flashadvance_VAL, value_p);
		value_p = cm_get_property(property_names[CP_FLASHUNIT]);
		if (value_p)
			p->flashunit_VAL = unitstrASCII_to_int(value_p);
		value_p = cm_get_property(property_names[CP_MAILADV]);
		if (value_p)
			(void) cm_strcpy(p->mailadvance_VAL,  value_p);
		value_p = cm_get_property(property_names[CP_MAILUNIT]);
		if (value_p)
			p->mailunit_VAL = unitstrASCII_to_int(value_p);
		value_p = cm_get_property(property_names[CP_OPENADV]);
		if (value_p)
			(void) cm_strcpy(p->openadvance_VAL,  value_p);
		value_p = cm_get_property(property_names[CP_OPENUNIT]);
		if (value_p)
			p->openunit_VAL = unitstrASCII_to_int(value_p);
		value_p = cm_get_property(property_names[CP_MAILTO]);
		if (value_p)
			(void) cm_strcpy(p->mailto_VAL, value_p);
		value_p = cm_get_property(property_names[CP_PRIVACY]);
		if (value_p)
			p->privacyunit_VAL = privacystrASCII_to_int(value_p);

		break;

	  case DISPLAY_SETTINGS:
		value_p = cm_get_property(property_names[CP_DAYBEGIN]);
		if (value_p)
			p->begin_slider_VAL = atoi(value_p);			

		value_p = cm_get_property(property_names[CP_DAYEND]);
		if (value_p)
			p->end_slider_VAL = atoi(value_p);			

		value_p = cm_get_property(property_names[CP_DEFAULTDISP]);
		if (value_p) {
			if (!atoi(value_p))
				p->default_disp_VAL = hour12;
			else
				p->default_disp_VAL = hour24;
		}

		value_p = cm_get_property(property_names[CP_DEFAULTVIEW]);
		if (value_p)
			p->default_view_VAL = atoi(value_p);			

		value_p = cm_get_property(property_names[CP_DEFAULTCAL]);
		if (value_p) 
			(void)cm_strcpy(p->defcal_VAL, value_p);

		loc = cm_target2location(c->calname);
		cm_strcpy(p->cloc_VAL, loc);
		free(loc); 

		break;
	case GROUP_ACCESS_LISTS:
	   	break;
	case PRINTER_OPTS:
		value_p = cm_get_property(property_names[CP_PRINTDEST]);
		if (value_p)
			p->dest_choiceVAL = atoi(value_p);
		value_p = cm_get_property(property_names[CP_PRINTPRIVACY]);
		if (value_p)
			p->meoVAL = atoi(value_p);
		value_p = cm_get_property(property_names[CP_PRINTOPTIONS]);
		if (value_p)
			(void) cm_strcpy(p->optionVAL, value_p);
		value_p = cm_get_property(property_names[CP_PRINTERNAME]);
		if (value_p)
			(void) cm_strcpy(p->printer_nameVAL, value_p);
		value_p = cm_get_property(property_names[CP_PRINTDIRNAME]);
		if (value_p)
			(void) cm_strcpy(p->dir_nameVAL, value_p);
		value_p = cm_get_property(property_names[CP_PRINTFILENAME]);
		if (value_p)
			(void) cm_strcpy(p->file_nameVAL, value_p);
		value_p = cm_get_property(property_names[CP_PRINTWIDTH]);
		if (value_p)
			(void) cm_strcpy(p->widthVAL, value_p);
		value_p = cm_get_property(property_names[CP_PRINTHEIGHT]);
		if (value_p)
			(void) cm_strcpy(p->heightVAL, value_p);
		value_p = cm_get_property(property_names[CP_PRINTPOSXOFF]);
		if (value_p)
			(void) cm_strcpy(p->xoffsetVAL, value_p);
		value_p = cm_get_property(property_names[CP_PRINTPOSYOFF]);
		if (value_p)
			(void) cm_strcpy(p->yoffsetVAL, value_p);
		value_p = cm_get_property(property_names[CP_PRINTMONTHS]);	
		if (value_p)
			(void) cm_strcpy(p->repeatVAL, value_p);
		value_p = cm_get_property(property_names[CP_PRINTCOPIES]);
		if (value_p)
			(void) cm_strcpy(p->copiesVAL, value_p);
		break;
	case DATE_FORMAT:
		value_p = cm_get_property(property_names[CP_DATEORDERING]);
		if (value_p)
			p->ordering_VAL = (Ordering_Type)atoi(value_p);
		value_p = cm_get_property(property_names[CP_DATESEPARATOR]);
		if (value_p)
			p->separator_VAL = (Separator_Type)atoi(value_p);
		break;
	}
}
static Boolean
dup_name_with_nohost(l, s, num_entries)
        Panel_item l; char *s;
	int *num_entries;
{
	char *shead, *lhead, *name;
	int i, ret = false;

        if (l==NULL || s==NULL || *s == NULL) 
		return(false);

	*num_entries=0;
	shead = get_head(s, '@');
        for (i = xv_get(l, PANEL_LIST_NROWS)-1; i >=0; i--) {
		name = (char*)xv_get(l, PANEL_LIST_CLIENT_DATA, i);
		lhead = get_head(name, '@');
                if (strcmp(lhead, shead) == 0) {
			(*num_entries)++;
			if (strchr(name, '@') == NULL || 
				strchr(s, '@') == NULL) {
                        	ret = true;
			}
		}
		free(lhead);
        }
	free(shead); 
        return ret; 
}
static void
write_rc_vals(p, view)
	Props	*p;
	int 	view;
{
	int	i, j;
	char	int_buf[15];
	Access_Entry *list=NULL;
	char *tmpbuf;
	Calendar *c = calendar;
#if 0
	Lookup_stat status;
#endif

	cal_update_props();
        switch (view)
        {
	   case EDITOR_DEFAULTS:
		cal_set_boolean_property(property_names[CP_BEEPON],
			(int) xv_get(p->reminder, PANEL_VALUE) & 0x1);
	 	cal_set_boolean_property(property_names[CP_FLASHON],
		 	(int) xv_get(p->reminder, PANEL_VALUE) & 0x2);
		cal_set_boolean_property(property_names[CP_OPENON],
			(int) xv_get(p->reminder, PANEL_VALUE) & 0x4);
		cal_set_boolean_property(property_names[CP_MAILON],
			(int) xv_get(p->reminder, PANEL_VALUE) & 0x8);
		cal_set_boolean_property(property_names[CP_UNIXON],
			(int) xv_get(p->reminder, PANEL_VALUE) & 0x10);
		cal_set_property(property_names[CP_BEEPADV],
			(char*)xv_get(p->beepadvance, PANEL_VALUE));
		cal_set_property(property_names[CP_BEEPUNIT],
			(char *)unitstrASCII[unitstr_to_int((char *)xv_get(p->beepunit, PANEL_LABEL_STRING))]);
		cal_set_property(property_names[CP_FLASHADV],
			(char*)xv_get(p->flashadvance, PANEL_VALUE));
		cal_set_property(property_names[CP_FLASHUNIT],
			(char *)unitstrASCII[unitstr_to_int((char *)xv_get(p->flashunit, PANEL_LABEL_STRING))]);
		cal_set_property(property_names[CP_MAILADV],
			(char*)xv_get(p->mailadvance, PANEL_VALUE));
		cal_set_property(property_names[CP_MAILUNIT],
			(char *)unitstrASCII[unitstr_to_int((char *)xv_get(p->mailunit, PANEL_LABEL_STRING))]);
		cal_set_property(property_names[CP_OPENADV],
			(char*)xv_get(p->openadvance, PANEL_VALUE));
		cal_set_property(property_names[CP_OPENUNIT],
			(char *)unitstrASCII[unitstr_to_int((char *)xv_get(p->openunit, PANEL_LABEL_STRING))]);
		cal_set_property(property_names[CP_MAILTO],
			(char*)xv_get(p->mailto, PANEL_VALUE));
		cal_set_property(property_names[CP_PRIVACY],
			(char *)privacystrASCII[privacystr_to_int((char *)xv_get(p->privacyunit, PANEL_LABEL_STRING))]);

		cal_set_properties();
                break;

    	    case DISPLAY_SETTINGS:
		(void) sprintf(int_buf, "%d", xv_get(p->beginslider, PANEL_VALUE));
		cal_set_property(property_names[CP_DAYBEGIN], int_buf);
	
		(void) sprintf(int_buf, "%d", xv_get(p->endslider, PANEL_VALUE));
		cal_set_property(property_names[CP_DAYEND], int_buf);

		(void) sprintf(int_buf, "%d", xv_get(p->default_view, PANEL_VALUE));
		cal_set_property(property_names[CP_DEFAULTVIEW], int_buf);
		(void) sprintf(int_buf, "%d", xv_get(p->default_disp, PANEL_VALUE));
		cal_set_property(property_names[CP_DEFAULTDISP], int_buf);

		tmpbuf = (char*)xv_get(p->defcal, PANEL_VALUE);
		cal_set_property(property_names[CP_DEFAULTCAL], tmpbuf);

		tmpbuf = (char*)xv_get(p->cloc, PANEL_VALUE);
		cal_set_property(property_names[CP_CALLOC], tmpbuf);
#if 0
#ifdef SVR4
		status = cm_set_nis_location(cm_get_uname(), DEFAULT_CALNAME, 
						tmpbuf);
#endif "SVR4"
#endif
		cal_set_properties();
                break;

            case GROUP_ACCESS_LISTS:

#if 0
		if (xv_get(p->access_list, PANEL_LIST_FIRST_SELECTED) != -1)
			change_accesslist(calendar);
		else 
			if (accesslist_proc(p->username, APPLY_OP))
				xv_set(p->username, PANEL_VALUE, "", NULL);
#endif

		j = xv_get(p->access_list, PANEL_LIST_NROWS);
		for (i = --j; i>=0; i--)
		{
			Access_Entry	*accessentry;
			char		*list_entry;
			char		*name_entry, *real_entry, *perms_str;

			/* get the ith entry */

			list_entry = (char*)xv_get(p->access_list,
					PANEL_LIST_STRING, i);
			real_entry = (char*)xv_get(p->access_list,
					PANEL_LIST_CLIENT_DATA, i);
			str_to_entry(list_entry, &name_entry, &perms_str);
			accessentry = make_access_entry(real_entry, 
				(int)perms_str_to_int(perms_str));
			free(name_entry); free(perms_str);
			if (list==NULL) 
				list = accessentry;
			else {
				accessentry->next = list;
				list = accessentry;
			}
		}
		if (table_set_access(calendar->calname, list) == status_denied) {
			notice_prompt(p->frame, (Event *)NULL,
                                NOTICE_MESSAGE_STRINGS,
                                MGET("You Do Not Have Access to Calendar:"),
                                calendar->calname,
                                0,   
                                NOTICE_BUTTON_YES,  LGET("Continue") ,
                                NULL);
			return;
		}
		/* dont cal_set_properties(). access gets set in rpc.cmsd not
			resource file */
		break;
	   case PRINTER_OPTS:
		(void) sprintf(int_buf, "%d", xv_get(p->dest_choice, 
				PANEL_VALUE));
		cal_set_property(property_names[CP_PRINTDEST], int_buf);
		(void) sprintf(int_buf, "%d", (int)xv_get(p->meo, PANEL_VALUE));
		cal_set_property(property_names[CP_PRINTPRIVACY], int_buf);
		cal_set_property(property_names[CP_PRINTOPTIONS],
			(char*)xv_get(p->options, PANEL_VALUE));
		cal_set_property(property_names[CP_PRINTERNAME],
			(char*)xv_get(p->printer_name, PANEL_VALUE));
		cal_set_property(property_names[CP_PRINTDIRNAME],
			(char*)xv_get(p->dir_name, PANEL_VALUE));
		cal_set_property(property_names[CP_PRINTFILENAME],
			(char*)xv_get(p->file_name, PANEL_VALUE));
		cal_set_property(property_names[CP_PRINTWIDTH],
			(char*)xv_get(p->scale_width, PANEL_VALUE));
		cal_set_property(property_names[CP_PRINTHEIGHT],
			(char*)xv_get(p->scale_height, PANEL_VALUE));
		cal_set_property(property_names[CP_PRINTPOSXOFF],
			(char*)xv_get(p->offset_width, PANEL_VALUE));
		cal_set_property(property_names[CP_PRINTPOSYOFF],
			(char*)xv_get(p->offset_height, PANEL_VALUE));
		(void) sprintf(int_buf, "%d", (char*)xv_get(p->repeat_times, 
				PANEL_VALUE));
		cal_set_property(property_names[CP_PRINTMONTHS], int_buf);
		(void) sprintf(int_buf, "%d", xv_get(p->copies, 
				PANEL_VALUE));
		cal_set_property(property_names[CP_PRINTCOPIES], int_buf);
		cal_set_properties();
		break;
	   case DATE_FORMAT:
		memset(int_buf, 0, sizeof(int_buf));
		(void) sprintf(int_buf, "%d", xv_get(p->ordering_list, PANEL_VALUE));
		cal_set_property(property_names[CP_DATEORDERING], int_buf);
		memset(int_buf, 0, sizeof(int_buf));
		(void) sprintf(int_buf, "%d", xv_get(p->separator_list, PANEL_VALUE));
		cal_set_property(property_names[CP_DATESEPARATOR], int_buf);
		cal_set_properties();
         	break; 
        default:
		break;
	}
}
	
int temp_begin_slider_VAL=0;
int temp_end_slider_VAL=0;
DisplayType temp_default_display=0;
	
/* ARGSUSED */
extern void
p_show_proc(frame)
	Frame frame;
{
	Calendar *c;
	Props *p;
	int view;

	if (frame == NULL) {
		make_props(calendar);
		c = calendar;
	}
	else 
		c = (Calendar *) xv_get(frame, WIN_CLIENT_DATA);

	if(c==NULL) return;
	p = (Props *) c->properties;
	if(p==NULL) return;

	/* save time bounds slider vals because they may
	   be needed for future comparisons in apply_proc */

	temp_begin_slider_VAL =(int)xv_get(p->beginslider, PANEL_VALUE);
	temp_end_slider_VAL =(int)xv_get(p->endslider, PANEL_VALUE);
	if ((int)xv_get(p->default_disp, PANEL_VALUE) == 0)
		temp_default_display = hour12;
	else
		temp_default_display = hour24;

        (void)xv_set(p->frame, XV_SHOW, TRUE, NULL);

	cal_update_props();
	view = xv_get(p->category_item, PANEL_VALUE);
	p_display_vals(p, view);

	props_changed = false;
}

/* ARGSUSED */
extern void
cm_show_props(m, mi)
	Menu m;
	Menu_item mi;
{
	Calendar *c;
	Props *p;

	c = (Calendar *) xv_get(m, MENU_CLIENT_DATA);
	p = (Props *) c->properties;
	if (!p->frame)
		p = (Props*)make_props(c);

	(void)p_show_proc(p->frame);
}
static Boolean
display_changed(p)
	Props *p;
{
	if (p==NULL) return(0);
	if (p->default_disp_VAL != temp_default_display) 
		return true;
	return false;
}

static Boolean
timebounds_changed(p)
	Props *p;
{
	Boolean ret_value = false;

	if (p==NULL) 
		return(ret_value);
	if ((p->begin_slider_VAL!=temp_begin_slider_VAL) 
		|| (p->end_slider_VAL!=temp_end_slider_VAL))
		ret_value =  true;
	temp_begin_slider_VAL =(int)xv_get(p->beginslider, PANEL_VALUE);
	temp_end_slider_VAL =(int)xv_get(p->endslider, PANEL_VALUE);

	return ret_value;
}
static void
readd_entry(c, name, fromrow, torow)
	Calendar *c;
	char *name;
	int fromrow, torow;
{
	Boolean selected = false;
	Pixrect *glyph=0;
	Browser *b = (Browser*)c->browser;

	glyph = (Pixrect*)xv_get(b->box, PANEL_LIST_GLYPH, fromrow);
	selected = xv_get(b->box, PANEL_LIST_SELECTED, fromrow); 
	xv_set(b->box, PANEL_LIST_DELETE, fromrow, NULL);
	list_add_entry(b->box, name, glyph, NULL, torow, FALSE);
	xv_set(b->box, PANEL_LIST_SELECT, torow, selected, NULL);
}
static void
readd_blist_entry(c, name, fromrow, torow)
	Calendar *c;
	char *name;
	int fromrow, torow;
{
	Browselist *bl = (Browselist*)c->browselist;

	xv_set(bl->list, PANEL_LIST_DELETE, fromrow, NULL);
	list_add_entry(bl->list, name, NULL, NULL, torow, FALSE);
}

static void
reset_blist(c)
	Calendar *c;
{
	Browselist *bl = (Browselist*)c->browselist;
	Props *p = (Props*)c->properties;
	int row=-1, nrows;

        if (browselist_exists(bl)) {
                if (list_in_list(bl->list, c->calname, &row)) {
                        if (row != 0)
                                readd_blist_entry(c, c->calname, row, 0);
                }
                else
                        list_add_entry(bl->list, c->calname,
                                        NULL, NULL, 0, FALSE);
                if (strcmp(c->calname, p->defcal_VAL) != 0) {
                        if (list_in_list(bl->list, p->defcal_VAL, &row)) {
                                if (row != 1)
                                        readd_blist_entry(c, p->defcal_VAL, row,
1);
                        }
                        else
                                list_add_entry(bl->list, p->defcal_VAL,
                                        NULL, NULL, 1, FALSE);
                }
	}
}

static void
reset_browser(c)
	Calendar *c;
{

	Browser *b = (Browser*)c->browser;
	Props *p = (Props*)c->properties;
	int row=-1;

	/* update browser scrolling list */
	if (list_in_list(b->box, c->calname, &row)) {
		if (row != 0) 
			readd_entry(c, c->calname, row, 0);
	}
	else 
		list_add_entry(b->box, c->calname, NULL, NULL, 0, FALSE);
	if (strcmp(c->calname, p->defcal_VAL) != 0) {
		if (list_in_list(b->box, p->defcal_VAL, &row)) {
			if (row != 1) 
				readd_entry(c, p->defcal_VAL, row, 1);
		}
		else 
			list_add_entry(b->box, p->defcal_VAL, NULL, 
					NULL, 1, FALSE);
	}
}
static void
p_reset_value(frame, item, value)
	Frame		frame;
	Panel_item	item;
	char		value[];
{
	xv_set(item, PANEL_VALUE, value, NULL);
	(void) notice_prompt(frame, (Event*)NULL,
		NOTICE_MESSAGE_STRING,
		MGET("Text field must have a value"),
		NULL);
}

static void 
p_apply_values(c, view)
    	Calendar *c;
	int view;
{
	Props *p = NULL;
    	Editor *e = NULL;
    	Browser *b = NULL;
    	Browselist *bl = NULL;
	Event *event=NULL;
	Boolean something_changed = false;
	char *tmpbuf;
	Ordering_Type save_oval;
	Separator_Type save_sval;
	int tick, y,     char_height;
	char buf[160], *save_cloc, *save_defcal;
	Week    *w;
	char *value;

	p = (Props*) c->properties;
	e = (Editor *) c->editor;
	b = (Browser *) c->browser;
	bl = (Browselist *) c->browselist;
	char_height = xv_get(c->fonts->lucida12b, FONT_DEFAULT_CHAR_HEIGHT); 

	switch (view)
	{
	 	case EDITOR_DEFAULTS:
			p->reminder_VAL = (int)xv_get(p->reminder, 
				PANEL_VALUE);
			(void) cm_strcpy(p->beepadvance_VAL, 
				(char *)xv_get(p->beepadvance, 
				PANEL_VALUE));

			p->beepunit_VAL = (int) unitstr_to_int(
				(char *)xv_get(p->beepunit, 
				PANEL_LABEL_STRING));
			(void) cm_strcpy(p->flashadvance_VAL,
				(char *)xv_get(p->flashadvance, 
				PANEL_VALUE));

			p->flashunit_VAL = (int)unitstr_to_int(
				(char *)xv_get(p->flashunit, 
				PANEL_LABEL_STRING));
			(void) cm_strcpy(p->openadvance_VAL, 
				(char *)xv_get(p->openadvance, 
				PANEL_VALUE));

			p->openunit_VAL = (int)unitstr_to_int(
				(char *)xv_get(p->openunit, 
				PANEL_LABEL_STRING));
			(void) cm_strcpy(p->mailadvance_VAL,
				(char *)xv_get(p->mailadvance, 
				PANEL_VALUE));
			(void) cm_strcpy(p->mailto_VAL,
				(char *)xv_get(p->mailto, PANEL_VALUE));

			p->mailunit_VAL = (int)unitstr_to_int(
				(char *)xv_get(p->mailunit, 
				PANEL_LABEL_STRING));

			p->privacyunit_VAL = (int)privacystr_to_int(
				(char *)xv_get(p->privacyunit, 
				PANEL_LABEL_STRING));
			if (editor_exists(e)) {
				set_default_reminders(c);
				set_default_privacy(e);
			}

			break;

		case DISPLAY_SETTINGS:
			p->begin_slider_VAL = (int) xv_get(p->beginslider, 
						PANEL_VALUE);
			p->end_slider_VAL =  (int) xv_get(p->endslider,  
						PANEL_VALUE);
			save_oval = p->default_view_VAL;
			p->default_view_VAL = (int) xv_get(p->default_view,
					   PANEL_VALUE);
			if (save_oval != p->default_view_VAL)
 				set_default_view(c, p->default_view_VAL);
			save_oval = p->default_disp_VAL;
			if ((int)xv_get(p->default_disp, PANEL_VALUE) == 1) {
				if (save_oval != hour24) {
					p->default_disp_VAL = hour24;
					common_update_lists(c);
				}
			}
			else {
				if (save_oval != hour12) {
					p->default_disp_VAL = hour12;
					common_update_lists(c);
				}
			}

			tmpbuf = (char*)xv_get(p->defcal, PANEL_VALUE);
			save_defcal = (char*)cm_strdup(p->defcal_VAL);
			if (tmpbuf && *tmpbuf) {
				if (strcmp(tmpbuf, p->defcal_VAL) != 0) {
					something_changed = true;
					if (bl != NULL)
						list_delete_entry(bl->list, p->defcal_VAL);
					if (b != NULL)
						list_delete_entry(b->box, p->defcal_VAL);
					cm_strcpy(p->defcal_VAL, tmpbuf);
				}
			}
			else
				p_reset_value(p->frame, p->defcal, save_defcal);
			tmpbuf = (char*)xv_get(p->cloc, PANEL_VALUE);
			save_cloc = (char*)cm_strdup(p->cloc_VAL);
			if (tmpbuf && *tmpbuf) {
				if (strcmp(tmpbuf, p->cloc_VAL) != 0) {
					something_changed = true;
					if (bl != NULL)
						list_delete_entry(bl->list, c->calname);
					if (b != NULL)
						list_delete_entry(b->box, c->calname);
					cm_strcpy(p->cloc_VAL, tmpbuf);
					cm_set_deftarget();
					tmpbuf = ckalloc(cm_strlen(LGET("CM Properties")) 
						+ cm_strlen(c->calname) + 4);   
					sprintf(tmpbuf, "%s: %s", LGET("CM Properties"),
						c->calname);
					xv_set(p->frame, XV_LABEL, tmpbuf, NULL);
					free(tmpbuf);
					if (editor_exists(e)) {
						tmpbuf = ckalloc(cm_strlen(LGET(
							"CM Appointment Editor: ")) + 
						cm_strlen(c->calname) + 4);   
						sprintf(tmpbuf, "%s %s", 
							LGET("CM Appointment Editor: "),
							c->calname);
						
						xv_set(e->frame, XV_LABEL, tmpbuf, NULL);
						free(tmpbuf);
					}
					table_create(c->calname);
					if (p->frame != NULL) {
						set_rc_vals(p, GROUP_ACCESS_LISTS);
						p_display_vals(p, GROUP_ACCESS_LISTS);
					}
				}
			}
			else
				p_reset_value(p->frame, p->cloc, save_cloc);
			if (something_changed) {
				if (browser_exists(b)) {
					reset_browser(c);
					blist_write_names(c, b->box);
				}
				if (browselist_exists(bl)) {
					reset_blist(c);
					if (!browser_exists(b))
						blist_write_names(c, bl->list);
				}
				/* must write list from browse menu */
				if (!browser_exists(b) && !browselist_exists(bl)) 
					reset_blist_menu(c, save_cloc, save_defcal);
				something_changed = false;
			}
			free(save_cloc); free(save_defcal);

			/* reset the pulldown in the appointment editor, since it
	   			triggers off the property sheet	*/
			if (editor_exists(e)) {
				(void)xv_set(e->timestack, 
				PANEL_ITEM_MENU, e_make_time_menu(e), 0);
        			(void)xv_set(e->durationstack, 
				PANEL_ITEM_MENU, e_make_duration_menu(e), 0);
			}
	
			/* check to see if viewing day glance or weekglance which are
				sensitive to the time bounds values	*/ 
			if (timebounds_changed(p)) {
				if (c->view->glance ==dayGlance) { 
					calendar_deselect(c);
					init_mo(c);
					init_dayview(c);
					paint_dayview(c, true, NULL);
					calendar_select(c, hourSelect,(caddr_t)NULL);
				}
				else if (c->view->glance == weekGlance) {
						calendar_deselect(c);
						paint_weekview(c, NULL);
						calendar_select(c, weekdaySelect, (caddr_t)NULL);
				}
				something_changed = true;
			}
			if (display_changed(p)) {
				beginslider_notify(p->beginslider, 
					p->begin_slider_VAL, event);
				endslider_notify(p->endslider, 
					p->end_slider_VAL, event);
				temp_default_display = p->default_disp_VAL;
				paint_canvas(c, NULL);
				if (editor_exists(e)) {
					e_hide_ampm(c);
					if (p->default_disp_VAL == hour12) {
						xv_set(e->time, PANEL_VALUE, "9:00",  NULL);
						xv_set(e->duration, PANEL_VALUE, "10:00", NULL);
					}
					else {
						xv_set(e->time, PANEL_VALUE, "0900", NULL);
						xv_set(e->duration, PANEL_VALUE, "1000", NULL);
					}
				}
				something_changed = true;
				
			}
			if (something_changed && browser_exists(b))
				update_browser_display(b, p);
			break;
		case PRINTER_OPTS:
			p->dest_choiceVAL = (int) xv_get(p->dest_choice,
                                                PANEL_VALUE);
			p->meoVAL = (int) xv_get(p->meo, PANEL_VALUE);

			value = (char*) xv_get(p->printer_name, PANEL_VALUE);
			if (value && *value)
				cm_strcpy(p->printer_nameVAL, value);
			else
				p_reset_value(p->frame, p->printer_name, p->printer_nameVAL);

			cm_strcpy(p->optionVAL, (char*) xv_get(p->options,
                                                PANEL_VALUE));
			cm_strcpy(p->dir_nameVAL, (char*) xv_get(p->dir_name,
                                                PANEL_VALUE));
			cm_strcpy(p->file_nameVAL, (char*) xv_get(p->file_name,
                                                PANEL_VALUE));

			value = (char*) xv_get(p->scale_height, PANEL_VALUE);
			if (value && *value)
				cm_strcpy(p->heightVAL, value);
			else
				p_reset_value(p->frame, p->scale_height, p->heightVAL);
			value = (char*) xv_get(p->scale_width, PANEL_VALUE);
			if (value && *value)
				cm_strcpy(p->widthVAL, value);
			else
				p_reset_value(p->frame, p->scale_width, p->widthVAL);

			value = (char*) xv_get(p->offset_width, PANEL_VALUE);
			if (value && *value)
				cm_strcpy(p->xoffsetVAL, value); 
			else
				p_reset_value(p->frame, p->offset_width, p->xoffsetVAL);

			value = (char*) xv_get(p->offset_height, PANEL_VALUE);
			if (value && *value)
				cm_strcpy(p->yoffsetVAL, value);
			else
				p_reset_value(p->frame, p->offset_height, p->yoffsetVAL);

			sprintf(p->repeatVAL, "%d", (int)xv_get(p->repeat_times, PANEL_VALUE));
			sprintf(p->copiesVAL, "%d", (int) xv_get(p->copies, PANEL_VALUE));
			break;
		case DATE_FORMAT:
			save_oval = p->ordering_VAL;
			save_sval = p->separator_VAL;
			p->ordering_VAL = (Ordering_Type) xv_get(p->ordering_list, PANEL_VALUE);
			p->separator_VAL = (Separator_Type) xv_get(p->separator_list, PANEL_VALUE);
			if (editor_exists(e) && 
				(save_oval != p->ordering_VAL ||
				save_sval != p->separator_VAL)) {
				
				tmpbuf = (char*)get_datemsg(p->ordering_VAL,
                                        p->separator_VAL);
				xv_set(e->datefield,
					PANEL_LABEL_STRING, 
					tmpbuf, NULL);
				free(tmpbuf);
				if (editor_showing(e)) {
					p->ordering_VAL = save_oval;
					p->separator_VAL = save_sval;
					tick = cm_getdate(get_date_str(p, 
						e->datetext), NULL);
					p->ordering_VAL = (Ordering_Type) xv_get(p->ordering_list, PANEL_VALUE);
					p->separator_VAL = (Separator_Type) xv_get(p->separator_list, PANEL_VALUE);
					set_date_on_panel(tick, e->datetext,
                                        	p->ordering_VAL, p->separator_VAL);
				}
			}
			common_update_lists(c);
			/* update date labels and date on browser */
			if (browser_exists(b)) {
				set_date_on_panel(b->date, b->datetext, 
					p->ordering_VAL, p->separator_VAL);
				y =  b->chart_y-(2*char_height);
				gr_clear_area(b->xcontext, 0, 0, 
					b->canvas_w, y-4);
				format_date(b->begin_week_tick, 
					p->ordering_VAL, buf, 0);
        			gr_text(b->xcontext, 
					c->view->outside_margin+4, 
					y-8, c->fonts->lucida12b, buf, NULL);
			}
			/* update date headers on current view */
			if (!xv_get(c->frame, FRAME_CLOSED)) {
        			switch(c->view->glance) {
        			case monthGlance:
					gr_clear_area(c->xcontext, 0, 0, c->view->winw, 
					c->view->topoffset-2);
					format_date(c->view->date, p->ordering_VAL, buf,0);
					gr_text(c->xcontext, c->view->outside_margin, 
						c->view->topoffset/2+5, c->fonts->lucida12b, buf, NULL);
					break;
				case weekGlance:
					w = (Week *)c->view->week_info;
					gr_clear_area(c->xcontext, 0, 0, c->view->winw, 
						c->view->topoffset);
					format_week_header(w->start_date, p->ordering_VAL, buf);
					gr_text(c->xcontext, w->x, w->y -
						xv_get(w->font,FONT_DEFAULT_CHAR_HEIGHT)/2,
					w->font, buf, NULL);
					break;
				case dayGlance:
					if (save_oval != p->ordering_VAL ||
                                		save_sval != p->separator_VAL) {
						gr_clear_area(c->xcontext, (int)MOBOX_AREA_WIDTH+4, 0,
							c->view->winw-(int)MOBOX_AREA_WIDTH+4, 
							c->view->topoffset);
						paint_day_header(c, c->view->date, NULL);
					}
					break;
				}
			}
			break;
	}
}

/* ARGSUSED */
static void
p_apply_proc(item, event)
	Panel_item item;
	Event *event;
{
	Calendar *c;
	Props *p;
	int view;
	char file_name[MAXNAMELEN];

	c = (Calendar *) xv_get(item, PANEL_CLIENT_DATA);
	p = (Props *)c->properties;
	
	view = xv_get(p->category_item, PANEL_VALUE);
	write_rc_vals(p, view);
	p_apply_values(c, view);

	if (!xv_get(p->frame, FRAME_CMD_PUSHPIN_IN)) 
		(void) xv_set(p->frame, XV_SHOW, FALSE, NULL); 
	else
		xv_set(p->frame, XV_SHOW, TRUE, NULL);

	props_changed = false;

	/* tell others that props changed */
	(void) sprintf(file_name, "%s/%s", getenv("HOME"), DS_FILENAME);
	cm_tt_update_props(file_name);
}

/* ARGSUSED */
static void
p_reset_proc(item, event)
	Panel_item item;
	Event *event;
{
	Calendar *c;
	Props *p;
	int view;

	c = (Calendar *) xv_get(item, PANEL_CLIENT_DATA);
	p = (Props *)c->properties;

	cal_update_props();
	view = xv_get(p->category_item, PANEL_VALUE);
	p_display_vals(p, view);

	if (!xv_get(p->frame, FRAME_CMD_PUSHPIN_IN)) 
		(void) xv_set(p->frame, XV_SHOW, FALSE, 0);

	props_changed = false;
}

/* ARGSUSED */
static void
p_defaults_proc(item, event)
	Panel_item item;
	Event *event;
{
	Calendar *c;
	Props *p;
	int view;

	c = (Calendar *) xv_get(item, PANEL_CLIENT_DATA);
	p = (Props *) c->properties;

	view = xv_get(p->category_item, PANEL_VALUE);
	set_tempdef_vals(p, view);

	if (!xv_get(p->frame, FRAME_CMD_PUSHPIN_IN))
		(void) xv_set(p->frame, XV_SHOW, TRUE, 0); 

	props_changed = true;

}
static void
add_entry_to_box(box, font, name, client_data, str, index)
	Panel_item	box;
	Xv_Font		font;
	char 	*name, *client_data, *str;
	int	 index;
{
    	char    *access_entry;
	int	i;

	access_entry = (char *)malloc(sizeof(char) * entrylen);
	access_entry[0] = '\0';
 	strncpy (access_entry, name, NAMELEN);
 	access_entry[NAMELEN] = '\0';
	for (i = cm_strlen(access_entry); i <= NAMELEN; i++)
		access_entry[i] = ' ';
	access_entry[i] = '\0';
	cm_strcat (access_entry, str);
	list_add_entry_font(box, access_entry, client_data, index, font, FALSE);
	free(access_entry);
}

/* ARGSUSED */
static void
menu_add_calendar(menu, menu_item)
	Menu		menu; 
	Menu_item	menu_item; 
{
	Calendar *c = (Calendar *)xv_get(menu, MENU_CLIENT_DATA); 
	Props	 *p = (Props *) c->properties;

	/* something is selected, so assume they're doing a change */
	if (xv_get(p->access_list, PANEL_LIST_FIRST_SELECTED) != -1)
		change_accesslist(c);
	else {
		accesslist_proc(p->username, ADD_OP);
		props_changed = true;
	}
} 

/* ARGSUSED */
static void
button_add_calendar(item, event)
	Panel		item; 
	Event		*event; 
{
	Calendar *c = (Calendar *)xv_get(item, PANEL_CLIENT_DATA); 
	Props	 *p = (Props *) c->properties;

	/* something is selected, so assume they're doing a change */
	if (xv_get(p->access_list, PANEL_LIST_FIRST_SELECTED) != -1)
		change_accesslist(c);
	else {
		accesslist_proc(p->username, ADD_OP);
		props_changed = true;
	}
} 

/* ARGSUSED */
static void
access_items_callback(listbox, list_entry, index)
	Panel_item	listbox;
	char		*list_entry;
	int		index;
{
	list_delete_entry_n(listbox, index);
}

/* ARGSUSED */
static void
cal_remove_from_accesslist(p)
	Props	*p;
{
	int 	not_val;

	if (list_num_selected (p->access_list)) {
		not_val = notice_prompt(p->frame, NULL,
                    NOTICE_MESSAGE_STRINGS,
                    MGET("Do You Really Want To Delete\nAll Selected Items in List?"),
                    0,
                    NOTICE_BUTTON_YES, LGET("Delete Selected") ,
                    NOTICE_BUTTON_NO, LGET("Cancel") ,
                    0);
		if (not_val == NOTICE_YES) {
			list_items_selected(p->access_list, access_items_callback);
			(void)xv_set(p->username, PANEL_VALUE, "", NULL);
			(void)xv_set(p->perms, PANEL_VALUE, 1, NULL);
			props_changed = true;
		}
	}
	else 
		(void) notice_prompt(p->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
			MGET("Select the Names in the List\nThat You Want to Delete and\nChoose the DELETE Menu Item or the Remove Button Again."),
			0,
			NOTICE_BUTTON_YES,  LGET("Continue") ,
			NULL);
		
} 

/*ARGSUSED */
static void
cal_remove_from_button(item, event)
	Panel_item 	item;
	Event		*event;
{
        Calendar *c = (Calendar *)xv_get(item, PANEL_CLIENT_DATA);
        Props    *p = (Props *) c->properties;
	cal_remove_from_accesslist(p);
}

/*ARGSUSED */
static void
cal_remove_from_menu(item, event)
	Menu_item 	item;
	Event		*event;
{
	Calendar *c = (Calendar *)xv_get(item, MENU_CLIENT_DATA); 
        Props    *p = (Props *) c->properties;
	cal_remove_from_accesslist(p);
}

/* this gets called when one or more item is selected in access list */
/* ARGSUSED */
static void
change_accesslist(c)
	Calendar *c; 
{
	Props	 *p = (Props *) c->properties;
        char     *name_text_entry = (char*)xv_get(p->username, PANEL_VALUE);
	register int     i, num;
	int	 nent, perms_int = (int)xv_get(p->perms, PANEL_VALUE);
	char	*uname, *name2, *perms_str, *name, *cd;

	/* cannot change name on more than one selected item */
	if ((num = list_num_selected(p->access_list)) > 1) 
		if (name_text_entry != NULL && *name_text_entry != NULL) {
			(void) notice_prompt(p->frame, (Event *)NULL,
                                NOTICE_MESSAGE_STRINGS,
                                MGET("Cannot Change Name On More Than One\nSelected Name. Deselect All But One Name."),
                                0,
                                NOTICE_BUTTON_YES,  LGET("Continue") ,
                                NULL);
			return;
		}
	if (perms_int == 0) {
		(void)notice_prompt(p->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
			MGET("Permissions Must Be Set To Change."),
			0,
			NOTICE_BUTTON_YES,  LGET("Continue") ,
			NULL);
		return;
        }
	if (dup_name_with_nohost(p->access_list, name_text_entry, &nent)) {
		if (nent > 1) {
               		notice_prompt(p->frame, (Event *)NULL,
                        NOTICE_MESSAGE_STRINGS,
                        MGET("You Cannot Add the Same Name Twice When\nOne Name Appears Without a Hostname."),
                        0,
                        NOTICE_BUTTON_YES,  LGET("Continue") ,
               	 	NULL);   
            	    return;
		}
        }
	/* changing entries */
	i = (int)xv_get(p->access_list, PANEL_LIST_FIRST_SELECTED);
	while (i != -1) {
		/* changing permissions */
		if (name_text_entry == NULL || *name_text_entry == NULL) {
			/* get displayed name from list */
			name_text_entry = (char*)xv_get(p->access_list, 
					PANEL_LIST_STRING, i);
			str_to_entry(name_text_entry, &name, &perms_str);
			free(perms_str);
		}
		else 
			 name = (char*)cm_strdup(name_text_entry);
		/* real name stored as client data */
		cd = (char*)xv_get(p->access_list, PANEL_LIST_CLIENT_DATA, i);
		perms_str = (char*)perms_int_to_str(perms_int);
		uname = get_head(c->user, '@');
		name2 = get_head(cd, '@');
                if (strcmp(uname, name2) == 0 && strlen(perms_str) != 3) {
                        notice_prompt(p->frame, (Event *)NULL,
                        NOTICE_MESSAGE_STRINGS,
                        MGET("You Must Give Yourself Full Permissions!"),
                        0,
                        NOTICE_BUTTON_YES,  LGET("Continue"),
                        NULL);
                        free(name2);free(uname); free(perms_str);
                        return;
                }
		free(name2);free(uname);
		free(cd);
		xv_set(p->access_list, PANEL_LIST_DELETE, i, NULL);
		add_entry_to_box(p->access_list, box_font, name,
				 name, perms_str, i);
		free(perms_str);
		name_text_entry = NULL;
		props_changed = true;
		i = (int)xv_get(p->access_list, PANEL_LIST_NEXT_SELECTED, i);
	}
} 

static Menu
make_access_menu()
{
	Menu namemenu;

	namemenu = menu_create(
                MENU_CLIENT_DATA, calendar,
                MENU_TITLE_ITEM, MGET("Name"),
                MENU_NOTIFY_STATUS, XV_ERROR,
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_STRING, LGET("Add Name"),
                        MENU_ACTION_PROC, menu_add_calendar,
			XV_HELP_DATA, "cm:DefaultAccessAdd",
                        0),
                MENU_APPEND_ITEM, menu_create_item(
                        MENU_STRING, LGET("Delete Selected"),
                        MENU_ACTION_PROC, cal_remove_from_menu,
			XV_HELP_DATA, "cm:DefaultAccessDelete",
                        0),
		0);
	return(namemenu);
}
/* ARGSUSED */
static void
panel_add_calendar(item, event)
        Panel_item item;
        Event *event;
{
	Calendar *c = (Calendar *)xv_get(item, PANEL_CLIENT_DATA); 
	Props *p = (Props *)c->properties; 

	/* something is selected, so assume they're doing a change */
	if (xv_get(p->access_list, PANEL_LIST_FIRST_SELECTED) != -1)
		change_accesslist(c);
	else {
		accesslist_proc(item, ADD_OP);
		props_changed = true;
	}
}

static void
accesslist_proc(item, op)
	Panel_item item;
	int op;
{
	Calendar *c = (Calendar *)xv_get(item, PANEL_CLIENT_DATA);
	Props    *p = (Props *) c->properties;
	char    *name_entry = (char*)xv_get(item, PANEL_VALUE);
	int     perms_int = (int)xv_get(p->perms, PANEL_VALUE);
	char	*perms_str, *list_entry;
	char    name_to_add[NAMELEN+1];
	int	i = 0, num;
	char *name, *uname;
 
	if (strchr(name_entry, ' ') != NULL) {
		(void) notice_prompt(p->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
			MGET("Remove blanks from Name\nand Add Again."),
			0,
			NOTICE_BUTTON_YES,  LGET("Continue") ,
			NULL);
		return;
	}
	if (dup_name_with_nohost(p->access_list, name_entry, &num)) {
		notice_prompt(p->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
			MGET("You Cannot Add the Same Name Twice When\nOne Name Appears Without a Hostname."),
			0,
			NOTICE_BUTTON_YES,  LGET("Continue") ,
		NULL);
		return;
	}

	if (name_entry != NULL && name_entry[0] != '\0' && perms_int != 0) {
		uname = get_head(c->user, '@');
		name = get_head(name_entry, '@');
		perms_str = (char*)perms_int_to_str(perms_int);
		if (strcmp(uname, name) == 0 &&  strlen(perms_str) != 3) {
			notice_prompt(p->frame, (Event *)NULL,
			NOTICE_MESSAGE_STRINGS,
			MGET("You Must Give Yourself Full Permissions!"),
			0,   
			NOTICE_BUTTON_YES,  LGET("Continue"),
			NULL);
			free(name);free(uname);free(perms_str);
			return;
		}
		free(name);free(uname);
		if (!duplicate_cd(p->access_list, name_entry, &num)) {
			/* if world, make it lowercase */
			if (strcasecmp(name_entry, WORLD) == 0)
				cm_strcpy(name_to_add, WORLD);
			else {
				strncpy(name_to_add, name_entry, NAMELEN);
				name_to_add[NAMELEN] = '\0';
				/* add name after world setting */
				if (xv_get(p->access_list, PANEL_LIST_NROWS) != 0) {
					list_entry = (char*)xv_get(p->access_list,
                                        	PANEL_LIST_CLIENT_DATA, 0);
					if (list_entry != NULL && 
						*list_entry != NULL &&
						strcasecmp(list_entry, WORLD) == 0)  
							i = 1;
				}
			}
			perms_str = (char*)perms_int_to_str(perms_int);
			add_entry_to_box(p->access_list, box_font, 
				name_to_add, cm_strdup(name_entry), perms_str, i);
			free(perms_str);
		}
		else if (op == ADD_OP)
			(void) notice_prompt(p->frame, (Event *)NULL,
                        NOTICE_MESSAGE_STRINGS,
                        MGET("Name is Already in List"),
                        0,
                        NOTICE_BUTTON_YES, LGET("Continue"),
                        NULL);
		return;
	}
        if (op == ADD_OP) { 
                (void) notice_prompt(p->frame, (Event *)NULL,
                        NOTICE_MESSAGE_STRINGS,
			MGET("Name and Permissions Must\nBe Set to Add Item to List."),
                        0,
                        NOTICE_BUTTON_YES, LGET("Continue"),
                        NULL);
                return;
        }
}

/* ARGSUSED */
access_notify_proc(item, string, cd, op, event, row)
	Panel_item item;
	char *string;       /* string of row being operated on */
        Xv_opaque cd;  /* Client data of row */
        Panel_list_op op;   /* operation */
        Event *event;
        int row;
{
	Props 	*p = (Props *) calendar->properties;
	char 	*entry,	*name_str, *perms_str, *realname;
	int	perms = 1;

	if (list_num_selected(item) > 1) {
		xv_set(p->username, PANEL_VALUE, "", NULL);
		xv_set(p->perms, PANEL_VALUE, 1, NULL);
		return;
	}	

	if (op == PANEL_LIST_OP_SELECT) {
		entry = (char*)list_get_entry(item, row);
		str_to_entry(entry, &name_str, &perms_str);
		perms = (int)perms_str_to_int(perms_str);
		realname = (char*)xv_get(item, PANEL_LIST_CLIENT_DATA, row);
		xv_set(p->username, PANEL_VALUE, realname, NULL);
		xv_set(p->perms, PANEL_VALUE, perms, NULL);
		free(name_str); free(perms_str);
	}
}
/* ARGSUSED */
static Panel_setting
validate_number(item, event)
        Panel_item      item;
        Event           *event;
{
        char    *input_str = (char*)xv_get(item, PANEL_VALUE);
        Props   *p;
        Boolean  error = false;
        char 	save_str[81];

        /* event proc gets called for down and up event */
        if (event_is_down(event)
                || input_str == NULL || input_str[0] == '\0')
                return(panel_text_notify(item, event));

        cm_strcpy(save_str, input_str);
        while (input_str != NULL && input_str[0] != '\0') {
                if (!iscntrl(*input_str) && !isdigit(*input_str)) {
                        p = (Props *) xv_get(item, PANEL_CLIENT_DATA);
				notice_prompt(p->frame, (Event *)NULL,
				NOTICE_MESSAGE_STRINGS,  MGET("Invalid Entry."),
				0,
				NOTICE_BUTTON_YES,  LGET("Continue") ,
                        	0);
                        /* overwrite input error */
                        save_str[cm_strlen(save_str)-1] = '\0';
                        xv_set(item, PANEL_VALUE, save_str, NULL);
                        return(PANEL_NONE);
                }
		input_str++;
        }
        props_changed = true;
        return(panel_text_notify(item, event));
}


extern caddr_t
make_props(c)
	Calendar *c;
{
	int tmp, i;
	Xv_Font pf;
	Props	*p = (Props *) c->properties;
	char *buf;
	Menu choicemenu;

#ifdef OW_I18N
        box_font = (Xv_Font) xv_find(calendar->frame, FONT,
                FONT_FAMILY,    FONT_FAMILY_DEFAULT_FIXEDWIDTH,
                FONT_SIZE,      12,
                0);
#else
	box_font = c->fonts->fixed12;
#endif
	p->longest_str = 0;
	p->last_props_pane = EDITOR_DEFAULTS;
	buf = ckalloc(cm_strlen(LGET("CM Properties")) + cm_strlen(c->calname) + 3);
	sprintf(buf, "%s: %s", LGET("CM Properties"), c->calname); 
	p->frame = xv_create(c->frame, FRAME_CMD, 
		WIN_USE_IM, TRUE,
		XV_LABEL,  buf,
		WIN_ROW_GAP, 9,
		XV_SHOW, FALSE,
		FRAME_CMD_PUSHPIN_IN, TRUE,
		WIN_CLIENT_DATA, c,
                FRAME_SHOW_FOOTER,      false,
                FRAME_DONE_PROC,      p_frame_done,
		XV_HELP_DATA, "cm:PropsPanel",
		0);
	free(buf);

        p->panel = xv_get(p->frame, FRAME_CMD_PANEL);
	xv_set(p->panel, WIN_CLIENT_DATA, c,
		XV_HELP_DATA, "cm:PropsPanel",
		NULL);

	p->category_item = xv_create(p->panel, PANEL_CHOICE_STACK,
		     	 	XV_X, xv_col(p->panel, 1),
		     		XV_Y, xv_row(p->panel, 0),
				PANEL_LABEL_STRING,  LGET("Category:"),
				PANEL_CHOICE_STRINGS,
                			LGET("Editor Defaults"),
					LGET("Display Settings"),
					LGET("Access List and Permissions "),
					LGET("Printer Settings"),
					LGET("Date Format"),
					0,
	 	        	PANEL_NOTIFY_PROC, p_category_notify,
				PANEL_CLIENT_DATA, c,
				XV_HELP_DATA, "cm:Display",
		        	NULL);
	/* attach help keyword to individual choice strings */
	/* this is a workaround since it's bug in xview that
	 * PANEL_CHOICE_STRINGS does not inherit the help text
	 * from the containing PANEL_CHOICE_STACK
	 */
	if (choicemenu = xv_get(p->category_item, PANEL_ITEM_MENU))
		xv_set(choicemenu, XV_HELP_DATA, "cm:Display", NULL);

	pf = (Xv_Font ) xv_get(p->category_item, PANEL_LABEL_FONT);

        p_create_edefs(p, pf);
        p_create_dispset(p, pf);
        p_create_access(p, pf);
        p_create_printer(p, pf);
	p_create_dateformat(p, pf);

        /* create these three buttons next because they are used 
           in p_create_bcal() */
	p->apply_button = xv_create(p->panel, PANEL_BUTTON,
			PANEL_LABEL_STRING, LGET(" Apply "),
			XV_Y, xv_row(p->panel, 9),
			PANEL_NOTIFY_PROC, p_apply_proc,
			PANEL_CLIENT_DATA, c,
			XV_HELP_DATA, "cm:DefaultApply",
			0);
	p->reset_button = xv_create(p->panel, PANEL_BUTTON,
			PANEL_LABEL_STRING, LGET(" Reset "),
			XV_Y, xv_row(p->panel, 9),
			PANEL_NOTIFY_PROC, p_reset_proc,
			PANEL_CLIENT_DATA, c,
			XV_HELP_DATA, "cm:DefaultReset",
			0);
	p->defaults_button = xv_create(p->panel, PANEL_BUTTON,
			PANEL_LABEL_STRING, LGET(" Defaults "),
			XV_Y, xv_row(p->panel, 9),
			PANEL_NOTIFY_PROC, p_defaults_proc,
			PANEL_CLIENT_DATA, c,
			XV_HELP_DATA, "cm:DefaultDefaults",
			0);

        (void)xv_set(p->panel, PANEL_DEFAULT_ITEM, p->apply_button, 0);

	for (i = 0; i < NO_OF_PANES; i++) 
            p_display_vals(p, i);

	tmp = xv_get(p->beginslider_str, XV_X) +
		 xv_get(p->beginslider_str, XV_WIDTH);
	if (p->longest_str < tmp)
		p->longest_str = tmp;
	tmp = xv_get(p->endslider_str, XV_X) + 
		xv_get(p->endslider_str, XV_WIDTH);

	if (p->longest_str < tmp)
		p->longest_str = tmp;

	p_show_dispset(p, FALSE);
	p_show_access(p, FALSE);
	p_show_printer(p, FALSE);
	p_show_dateformat(p, FALSE);
	p_show_edefs(p, TRUE);

	xv_set(p->panel, XV_WIDTH, p->longest_str + 20,
		XV_HEIGHT, xv_get(p->apply_button, XV_Y) +
		xv_get(p->apply_button, XV_HEIGHT) + 15, NULL);

	xv_set(p->frame, WIN_FIT_WIDTH, 0,
		WIN_FIT_HEIGHT, 0, NULL);

	ds_center_items(p->panel, 9, p->apply_button, p->reset_button,
		p->defaults_button, NULL);

	ds_position_popup(c->frame, p->frame, DS_POPUP_LOR);

	return((caddr_t)p);
}
static void
p_set_adv_unit(m, mi)
        Menu m;
        Menu_item mi;
{
        Panel_item pi;

        pi = (Panel_item)xv_get(m, MENU_CLIENT_DATA);
        xv_set(pi, PANEL_LABEL_STRING, 
		(char*)xv_get(mi, MENU_STRING), NULL);
	props_changed = true;
}
static void
p_set_priv(m, mi)
        Menu m;
        Menu_item mi;
{
        Props *p;

        p = (Props*)xv_get(m, MENU_CLIENT_DATA);
        xv_set(p->privacyunit, PANEL_LABEL_STRING,
                (char*)xv_get(mi, MENU_STRING), NULL);
	props_changed = true;
}
extern Menu
p_make_priv_menu(p)
        Props *p;
{
        static Menu pmenu;

        if (p==NULL) return(NULL);

        pmenu = menu_create(
                MENU_CLIENT_DATA, p,
                MENU_NOTIFY_PROC, p_set_priv,
                MENU_NOTIFY_STATUS, XV_ERROR,
                MENU_ITEM,
                        MENU_STRING,  privacystr[0],
                        MENU_VALUE, 0,
                        0,
                MENU_ITEM,
                        MENU_STRING,  privacystr[2],
                        MENU_VALUE, 2,
                        0,
                MENU_ITEM,
                        MENU_STRING,  privacystr[1],
                        MENU_VALUE, 1,
                        0,
                XV_HELP_DATA, "cm:PrivacyMenu",
                0);

        return(pmenu);
}

static Menu
p_make_unit_menu(pi)
        Panel_item pi;
{
        static Menu unitmenu;
        
        if (pi==NULL) return(NULL);

	/* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL :
	 * 
	 * "mins" is abbreviation for the time unit "minutes".  
	 *
	 * "hrs" is abbreviation for the time unit "hours"
	 */
        unitmenu = menu_create(
                MENU_CLIENT_DATA, pi,
                MENU_NOTIFY_PROC, p_set_adv_unit,
                MENU_NOTIFY_STATUS, XV_ERROR,
                MENU_ITEM,
                        MENU_STRING, MGET("Mins"),
                        MENU_VALUE, 0,
                        0,
                MENU_ITEM,
                        MENU_STRING, MGET("Hrs"),
                        MENU_VALUE, 1,
                        0,
                MENU_ITEM,
                        MENU_STRING, MGET("Days"),
                        MENU_VALUE, 2,
                        0,
                XV_HELP_DATA, "cm:UnitMenu",
                0);
        
        return(unitmenu);
}
/* ARGSUSED */
static Notify_value
props_access_sort_alpha(item, event)
        Panel_item item;
        Event *event;
{
        Calendar *c;
        Props *p;
	char *entry, *ptr1, *ptr2;
	Boolean world = false;
	int i, selected=0;
 
	c = (Calendar *)xv_get(item, PANEL_CLIENT_DATA);
        p = (Props *)c->properties;

	i = (int) xv_get(p->access_list, PANEL_LIST_NROWS);
	if (i < 1) {
		notice_prompt(c->frame, (Event *)NULL,
		NOTICE_MESSAGE_STRINGS,
		EGET("There are no items to sort"),
		0,
		NOTICE_BUTTON_YES,  LGET("Continue"),
		0);
		return;
	}

        entry = cm_strdup((char *)list_get_entry(p->access_list, 0));
	if (strncmp(entry, "world", 5) == 0) {
		world = TRUE;
                selected = (int)xv_get(p->access_list, PANEL_LIST_SELECTED, 0, NULL);
		ptr1 = strchr(entry, ' ');
		ptr2 = strrchr(entry, ' ');
		*ptr1 = '\0';
		xv_set(p->access_list, PANEL_LIST_DELETE, 0, NULL);
	}
        xv_set(p->access_list, PANEL_LIST_SORT, PANEL_FORWARD, NULL);

	if (world) {
                add_entry_to_box(p->access_list, box_font, entry,
                     		entry, ++ptr2, 0);
                if (selected)
                        xv_set(p->access_list, PANEL_LIST_SELECT, 0, TRUE,
NULL);
	}
	else
		free(entry);
	props_changed = true;
}


static void 
p_create_edefs(p, pf)
	Props *p;
	Xv_Font pf;
{
	int y_gap=28, x_gap = 20, wd;
	int tmp, longest_str;
	char *t=NULL;
	struct pr_size size, size1, size2;
	Font_string_dims dims;

	t = LGET("Alarm:");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size.x = dims.width;
	size.y = dims.height;

	t = LGET("Mail To:");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size1.x = dims.width;
	size1.y = dims.height;

	t = LGET("Privacy:");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size2.x = dims.width;
	size2.y = dims.height;

	wd = max(max(size.x, size1.x), size2.x) + 40;

	t = LGET("Alarm:");
	p->reminderstr = xv_create(p->panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, t,
		PANEL_LABEL_BOLD, TRUE,
		XV_X, wd - size.x,
		XV_Y, xv_row(p->panel, 1) + y_gap,
		XV_HELP_DATA, "cm:DefaultReminder",
		0);
	/* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL :
	 *
	 * Beep = A sound that calendar manager window makes.
	 *
	 * Flash = Screen blinks or flicker
	 *
	 * PopUp = A small window is displayed to remind the user of an 
	 * appointment.
	 *
	 * Mail = Send mail as a reminder. 
	 */
	p->reminder = xv_create(p->panel, PANEL_TOGGLE,
		PANEL_CHOICE_STRINGS, LGET("Beep"), LGET("Flash"), LGET("PopUp"), LGET("Mail"),  0,
		XV_X, xv_get(p->reminderstr, XV_X) + 
			xv_get(p->reminderstr, XV_WIDTH) + x_gap ,
		XV_Y, xv_get(p->reminderstr, XV_Y),
		PANEL_LAYOUT, PANEL_VERTICAL,
		PANEL_FEEDBACK, PANEL_INVERTED,
		PANEL_NOTIFY_PROC, changed_notify,
		XV_HELP_DATA, "cm:ReminderPropsToggle",
		0);

	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size.x = dims.width;
	size.y = dims.height;
        p->beepadvance = xv_create(p->panel, PANEL_TEXT,
                PANEL_VALUE_DISPLAY_LENGTH, 2,
                XV_X, xv_get(p->reminder, XV_X) + 
			xv_get(p->reminder, XV_WIDTH) + x_gap,
                XV_Y, xv_get(p->reminder, XV_Y) + 10,
		PANEL_NOTIFY_PROC, validate_number,
		PANEL_NOTIFY_LEVEL, PANEL_ALL, 
		PANEL_CLIENT_DATA, p, 
		XV_HELP_DATA, "cm:BeepAdvance",
                0);
        p->beepunitstack = xv_create(p->panel, PANEL_ABBREV_MENU_BUTTON,
                XV_X, xv_get(p->beepadvance, XV_X) +
                        xv_get(p->beepadvance, XV_WIDTH) + x_gap,
		XV_Y, xv_get(p->beepadvance, XV_Y),
                XV_HELP_DATA, "cm:BeepAdvance",
                0);
        p->beepunit = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_VALUE_DISPLAY_LENGTH, 4,
                XV_X, xv_get(p->beepunitstack, XV_X) +
                        xv_get(p->beepunitstack, XV_WIDTH) + x_gap,
		XV_Y, xv_get(p->beepadvance, XV_Y), 
                XV_HELP_DATA, "cm:BeepUnit",
                0);
	xv_set(p->beepunitstack, PANEL_ITEM_MENU, 
		p_make_unit_menu(p->beepunit), NULL);

	p->flashadvance = xv_create(p->panel, PANEL_TEXT,
                PANEL_VALUE_DISPLAY_LENGTH, 2,
                XV_X, xv_get(p->beepadvance, XV_X),
		XV_Y, xv_get(p->beepadvance, XV_Y) + y_gap,
		PANEL_NOTIFY_PROC, validate_number,
		PANEL_NOTIFY_LEVEL, PANEL_ALL, 
		PANEL_CLIENT_DATA, p, 
                XV_HELP_DATA, "cm:FlashAdvance",
                0);
        p->flashunitstack = xv_create(p->panel, PANEL_ABBREV_MENU_BUTTON,
                XV_X, xv_get(p->beepunitstack, XV_X),
		XV_Y, xv_get(p->flashadvance, XV_Y),
                XV_HELP_DATA, "cm:FlashUnit",
                0);
        p->flashunit = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_VALUE_DISPLAY_LENGTH, 4,
                XV_X, xv_get(p->beepunit, XV_X),
		XV_Y, xv_get(p->flashadvance, XV_Y),
                XV_HELP_DATA, "cm:FlashUnit",
                0);
	xv_set(p->flashunitstack, PANEL_ITEM_MENU, 
		p_make_unit_menu(p->flashunit), NULL); 

	p->openadvance = xv_create(p->panel, PANEL_TEXT,
                PANEL_VALUE_DISPLAY_LENGTH, 2,
                XV_X, xv_get(p->beepadvance, XV_X),
		XV_Y, xv_get(p->flashadvance, XV_Y) + y_gap,
		PANEL_NOTIFY_PROC, validate_number,
		PANEL_NOTIFY_LEVEL, PANEL_ALL, 
		PANEL_CLIENT_DATA, p, 
                XV_HELP_DATA, "cm:OpenAdvance",
                0);
        p->openunitstack = xv_create(p->panel, PANEL_ABBREV_MENU_BUTTON,
                XV_X, xv_get(p->beepunitstack, XV_X),
		XV_Y, xv_get(p->openadvance, XV_Y),
                XV_HELP_DATA, "cm:OpenUnit",
                0);
        p->openunit = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_VALUE_DISPLAY_LENGTH, 4,
                XV_X, xv_get(p->beepunit, XV_X),
		XV_Y, xv_get(p->openadvance, XV_Y),
                XV_HELP_DATA, "cm:OpenUnit",
                0);
	xv_set(p->openunitstack, PANEL_ITEM_MENU, 
		p_make_unit_menu(p->openunit), NULL);  

	p->mailadvance = xv_create(p->panel, PANEL_TEXT,
                PANEL_VALUE_DISPLAY_LENGTH, 2,
                XV_X, xv_get(p->beepadvance, XV_X),
		XV_Y, xv_get(p->openadvance, XV_Y) + y_gap,
		PANEL_NOTIFY_PROC, validate_number,
		PANEL_NOTIFY_LEVEL, PANEL_ALL, 
		PANEL_CLIENT_DATA, p, 
                XV_HELP_DATA, "cm:MailAdvance",
                0);
        p->mailunitstack = xv_create(p->panel, PANEL_ABBREV_MENU_BUTTON,
                XV_X, xv_get(p->beepunitstack, XV_X),
		XV_Y, xv_get(p->mailadvance, XV_Y),
                XV_HELP_DATA, "cm:MailUnit",
                0);
        p->mailunit = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_VALUE_DISPLAY_LENGTH, 4,
                XV_X, xv_get(p->beepunit, XV_X),
		XV_Y, xv_get(p->mailadvance, XV_Y),
                XV_HELP_DATA, "cm:MailUnit",
                0);

	t = LGET("Mail To:");
	p->mailtostr = xv_create(p->panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING,  t,
		PANEL_LABEL_BOLD, TRUE,
		XV_X, wd - size1.x,
		XV_Y, xv_get(p->mailunit, XV_Y) + 40,
		XV_HELP_DATA, "cm:Mailto",
		0);
	p->mailto = xv_create(p->panel, PANEL_TEXT,
		PANEL_VALUE_DISPLAY_LENGTH, 26,
		PANEL_VALUE_STORED_LENGTH, BUFSIZ,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		XV_X, xv_get(p->reminder, XV_X),
		XV_Y, xv_get(p->mailtostr, XV_Y),
		PANEL_NOTIFY_PROC, changed_text_notify,
		PANEL_NOTIFY_LEVEL, PANEL_ALL, 
                XV_HELP_DATA, "cm:Mailto",
                0);

	xv_set(p->mailunitstack, PANEL_ITEM_MENU, 
		p_make_unit_menu(p->mailunit), NULL);  
        t = LGET("Privacy:");
        p->privacystr = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, t,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, wd - size1.x,
                XV_Y, xv_get(p->mailto, XV_Y) +
                         2*xv_get(p->mailto, XV_HEIGHT) + 20,
                XV_HELP_DATA, "cm:PrivacyMenu",
                0);
        p->privacystack = xv_create(p->panel, PANEL_ABBREV_MENU_BUTTON,
                PANEL_ITEM_MENU, p_make_priv_menu(p),
                PANEL_VALUE, privacystr[p->privacyunit_VAL],
                XV_X, xv_get(p->privacystr, XV_X)
                        + xv_get(p->privacystr, XV_WIDTH) + 10,
                XV_Y, xv_get(p->privacystr, XV_Y),
                PANEL_CLIENT_DATA, p,
                XV_HELP_DATA, "cm:PrivacyMenu",
                0);
        p->privacyunit = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_VALUE_DISPLAY_LENGTH, 4,
                XV_X, xv_get(p->privacystack, XV_X)
                        + xv_get(p->privacystack, XV_WIDTH) + 10,
                XV_Y, xv_get(p->privacystr, XV_Y),
                PANEL_CLIENT_DATA, p,
                PANEL_LABEL_STRING, privacystr[0],
                XV_HELP_DATA, "cm:PrivacyMenu",
                0);

	t = MGET("Mins");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size.x = dims.width;
	size.y = dims.height;

	t = MGET("Hrs");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size1.x = dims.width;
	size1.y = dims.height;

	t = MGET("Days");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size2.x = dims.width;
	size2.y = dims.height;

	longest_str = max(max(size.x, size1.x), size2.x);

	tmp = xv_get(p->mailunit, XV_X) + longest_str;
	if (tmp > p->longest_str)
		p->longest_str = tmp;

	t = MGET("Show Time and Text");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size.x = dims.width;
	size.y = dims.height;

	t = MGET("Show Time Only");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size1.x = dims.width;
	size1.y = dims.height;

	t = MGET("Show Nothing");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size2.x = dims.width;
	size2.y = dims.height;

	longest_str = max(max(size.x, size1.x), size2.x);

	tmp = xv_get(p->privacyunit, XV_X) + longest_str;
	if (tmp > p->longest_str)
		p->longest_str = tmp;
}

static void
p_create_dispset(p, pf)
	Props *p;
	Xv_Font pf;
{
	int row, tmp, wd, gap=10, y_gap = 10;
	char *t=NULL;
	struct pr_size size1, size2, size3, size4, size5, size6, size7;
	Font_string_dims dims;

	t = LGET("Day Boundaries:");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size1.x = dims.width;
	size1.y = dims.height;

	t = LGET("  Begin:");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size2.x = dims.width;
	size2.y = dims.height;

	t = LGET("  End:");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size3.x = dims.width;
	size3.y = dims.height;

	/* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL :
	 *
	 * Hour Display = Choice between a 12 hour time display or
	 * a 24 hour time display.
	 */
	t = LGET("Hour Display:");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size4.x = dims.width;
	size4.y = dims.height;

	/* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL :
	 *
	 * Default View = The view that is displayed on the calendar when
	 * the calendar manager application is invoked. 
	 */
	t = LGET("Default View:");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size5.x = dims.width;
	size5.y = dims.height;

	/* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL :
	 *
	 * Initial Browse = Whose calendar is browsed when the calendar
	 * application is invoked.
	 */
	t = LGET("Initial Calendar View:");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size6.x = dims.width;
	size6.y = dims.height;

	/* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL :
	 *
	 * Where the Calendar Data lives, on what hostname
	 */
	t = LGET("User Calendar Location:");
	(void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
	size7.x = dims.width;
	size7.y = dims.height;

	wd = max(max(max(max(max(max(size1.x, size2.x), size3.x), size4.x), 
				size5.x), size6.x), size7.x) + 23;

	row = xv_row(p->panel, 1);
	t = LGET("User Calendar Location:");
	p->clocstr = xv_create(p->panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, t,
		PANEL_LABEL_BOLD, TRUE,
		XV_X, wd - size7.x,
		XV_Y, row+7,
		XV_HELP_DATA, "cm:Location",
		0);
	p->cloc = xv_create(p->panel, PANEL_TEXT,
		XV_X, xv_get(p->clocstr, XV_X) + size7.x + gap,
		XV_Y, row+7,
		PANEL_VALUE_DISPLAY_LENGTH, 23,
		PANEL_NOTIFY_PROC, changed_text_notify,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		XV_HELP_DATA, "cm:Location",
		0);

	t = LGET("Day Boundaries:");
	p->dayboundstr = xv_create(p->panel, PANEL_MESSAGE,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_LABEL_STRING, t,
		XV_X, wd - size1.x,
		XV_Y, xv_row(p->panel,2)+7,
		XV_HELP_DATA, "cm:DefaultDay",
		0);

	t = LGET("  Begin:");
	p->beginstr = xv_create(p->panel, PANEL_MESSAGE,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_LABEL_STRING, t,
		XV_X, wd - size2.x,
		XV_Y, xv_row(p->panel, 3),
		XV_HELP_DATA, "cm:DefaultDayBegin",
		0);
	p->beginslider = xv_create(p->panel, PANEL_SLIDER,
		XV_X, xv_get(p->beginstr, XV_X) + size2.x + gap,
		XV_Y, xv_get(p->beginstr, XV_Y),
		PANEL_MIN_VALUE, 0,
		PANEL_MAX_VALUE, 23,
		PANEL_SLIDER_WIDTH, 92,
		PANEL_SHOW_VALUE, FALSE,
		PANEL_SHOW_RANGE, FALSE,
		PANEL_VALUE, BEGINS_DEF,
		PANEL_NOTIFY_PROC, beginslider_notify,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_CLIENT_DATA, p,
		XV_HELP_DATA, "cm:DefaultDayBegin",
		0);
	p->beginslider_str = xv_create(p->panel, PANEL_MESSAGE,
		PANEL_LABEL_BOLD, TRUE,
		XV_X, (int)xv_get(p->beginslider, XV_X) +
			(int)xv_get(p->beginslider, 
			PANEL_SLIDER_WIDTH) + 2*gap,
		XV_Y, xv_get(p->beginslider, XV_Y),
		PANEL_VALUE, "",
		XV_HELP_DATA, "cm:DefaultDayBegin",
		0);

	t = LGET("  End:");
	p->endstr = xv_create(p->panel, PANEL_MESSAGE,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_LABEL_STRING, t,
		XV_X, wd - size3.x,
		XV_Y, xv_row(p->panel, 4),
		XV_HELP_DATA, "cm:DefaultDayEnd",
		0);
	p->endslider = xv_create(p->panel, PANEL_SLIDER,
		XV_X, xv_get(p->endstr, XV_X) + size3.x + gap,
		XV_Y, xv_get(p->endstr, XV_Y),
		PANEL_MIN_VALUE, 1,
		PANEL_MAX_VALUE, 24,
		PANEL_SLIDER_WIDTH, 92,
		PANEL_SHOW_VALUE, FALSE,
		PANEL_SHOW_RANGE, FALSE,
		PANEL_VALUE, ENDS_DEF,
		PANEL_NOTIFY_PROC, endslider_notify,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_CLIENT_DATA, p,
		XV_HELP_DATA, "cm:DefaultDayEnd",
		0);
	p->endslider_str = xv_create(p->panel, PANEL_MESSAGE,
		PANEL_LABEL_BOLD, TRUE,
		XV_X, (int)xv_get(p->endslider, XV_X) +
			(int)xv_get(p->endslider, 
			PANEL_SLIDER_WIDTH) + 2*gap,
		XV_Y, xv_get(p->endslider, XV_Y),
		PANEL_VALUE, "",
		XV_HELP_DATA, "cm:DefaultDayEnd",
		0);

	row = xv_row(p->panel, 5);
	t = LGET("Hour Display:");
	p->defdispstr = xv_create(p->panel, PANEL_MESSAGE,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_LABEL_STRING, t,
		XV_X, wd - size4.x,
		XV_Y, row,
		XV_HELP_DATA, "cm:DefaultView",
		0);
	p->default_disp = xv_create(p->panel, PANEL_CHOICE,
		PANEL_CHOICE_STRINGS, LGET(" 12 Hour "), 
					LGET(" 24 Hour "), 0,
		XV_X, xv_get(p->endslider, XV_X),
		XV_Y, row,
		PANEL_FEEDBACK, PANEL_INVERTED,
		PANEL_NOTIFY_PROC, changed_notify,
		XV_HELP_DATA, "cm:DefaultView",
		0);
	row = xv_row(p->panel, 6);
	t = LGET("Default View:");
	p->defviewstr = xv_create(p->panel, PANEL_MESSAGE,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_LABEL_STRING, t,
		XV_X, wd - size5.x,
		XV_Y, row,
		XV_HELP_DATA, "cm:DefaultDisp",
		0);
	p->default_view = xv_create(p->panel, PANEL_CHOICE,
		PANEL_CHOICE_STRINGS, LGET("Year"), LGET("Month"), LGET("Week"), LGET("Day"), 0,
		XV_X, xv_get(p->endslider, XV_X),
		XV_Y, row,
		PANEL_FEEDBACK, PANEL_INVERTED,
		PANEL_NOTIFY_PROC, changed_notify,
		XV_HELP_DATA, "cm:DefaultDisp",
		0);
	row = xv_row(p->panel, 7);
	t = LGET("Initial Calendar View:");
	p->defcalstr = xv_create(p->panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, t,
		PANEL_LABEL_BOLD, TRUE,
		XV_X, wd - size6.x,
		XV_Y, row+y_gap,
		XV_HELP_DATA, "cm:DefaultCal",
		0);
	p->defcal = xv_create(p->panel, PANEL_TEXT,
		XV_X, xv_get(p->endslider, XV_X),
		XV_Y, row+y_gap,
		PANEL_VALUE_DISPLAY_LENGTH, 23,
		PANEL_NOTIFY_PROC, changed_text_notify,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		XV_HELP_DATA, "cm:DefaultCal",
		0);

	tmp = xv_get(p->default_view, XV_X) + 
		xv_get(p->default_view, XV_WIDTH);
	if (p->longest_str < tmp) p->longest_str = tmp; 
}

static void
p_create_access(p, pf)
	Props *p;
	Xv_font pf;
{
	int y_gap=10, x_gap=15, gap = 10, tmp, tmp2, max;
	char *t=NULL;
	struct pr_size size;
	Font_string_dims dims;
	Calendar *c;
        Access_Entry    *list = NULL;
	int pvdl;
	Rect	*rect;

	c = (Calendar*)xv_get(p->panel, WIN_CLIENT_DATA);

	xv_get(pf, FONT_STRING_DIMS, LGET("User Name: "), &dims);
	pvdl = (double)(PL_WIDTH+30-dims.width)/
		(double)xv_get(pf, FONT_COLUMN_WIDTH);
        p->username = xv_create(p->panel, PANEL_TEXT,
                XV_X, xv_col(p->panel, 1) + gap,
                XV_Y, xv_row(p->panel, 1),
                PANEL_CLIENT_DATA,       c,
                PANEL_NOTIFY_PROC, button_add_calendar,
                PANEL_VALUE_DISPLAY_LENGTH, pvdl,
                PANEL_LABEL_STRING, LGET("User Name:"),
                PANEL_LAYOUT, PANEL_HORIZONTAL,
                PANEL_READ_ONLY, FALSE,
		XV_HELP_DATA, "cm:GAUserName",
                0);

	t = MGET("User Name             Permissions");
	/* Need to make sure there is room to display the entire panel title */
	xv_get(pf, FONT_STRING_DIMS, t, &dims);
	dims.width += 30;    /* 30 is approximately the width of the scroll bar */
	if ( dims.width < PL_WIDTH ) {
		dims.width = PL_WIDTH;
	}
	p->access_list = xv_create(p->panel, PANEL_LIST,
		PANEL_LIST_DISPLAY_ROWS,	5, 
                PANEL_LIST_WIDTH,       dims.width,
		PANEL_CHOOSE_ONE, 	FALSE,     
		PANEL_CHOOSE_NONE,      TRUE,
		PANEL_CLIENT_DATA, 	 c, 
        XV_X, xv_get(p->username, XV_X),
		XV_Y,  		xv_row(p->panel,2),
		PANEL_NOTIFY_PROC,     	access_notify_proc,
		PANEL_ITEM_MENU, 	make_access_menu(),
		PANEL_LIST_TITLE, t,
                XV_HELP_DATA, "cm:GroupAccessList",
		0);

	tmp = xv_get(p->access_list, XV_WIDTH);
	tmp2 = xv_get(p->username, XV_WIDTH);
	if ( tmp2 > tmp ) {
		tmp = tmp2;
	}

	/* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL :
	 * If the following message is translated,  the BID permissions
	 * in the access list should also be translated.
	 * Asian locales do not need to translate this message
	 */
        p->access_msg = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, MGET("       B = Browse  I = Insert  D = Delete"),
                XV_X, xv_get(p->access_list, XV_X) + 10,
                XV_Y, xv_get(p->access_list, XV_Y) +
                         xv_get(p->access_list, XV_HEIGHT)+2,
                XV_HELP_DATA, "cm:DefaultAccessLevel",
                NULL);

        p->addbutton = xv_create(p->panel, PANEL_BUTTON,
                XV_X, xv_get(p->access_list, XV_X) + tmp + x_gap,
                XV_Y, xv_row(p->panel, 1) - 3,
                PANEL_NOTIFY_PROC, button_add_calendar,
                PANEL_CLIENT_DATA, c,
                PANEL_LABEL_STRING, LGET("Add Name"),
		XV_HELP_DATA, "cm:GAAddName",
                0);

	tmp = xv_get(p->addbutton, XV_X) +
		 xv_get(p->addbutton, XV_WIDTH); 
	if (p->longest_str < tmp)
		p->longest_str = tmp;

	p->removebutton = xv_create(p->panel, PANEL_BUTTON,
                XV_X, xv_get(p->addbutton, XV_X),
                XV_Y, xv_row(p->panel, 3)-10,
                PANEL_CLIENT_DATA, c,
                PANEL_LABEL_STRING, LGET("Remove"),
                PANEL_NOTIFY_PROC, cal_remove_from_button,
		XV_HELP_DATA, "cm:GARemove",
                0);

	tmp = xv_get(p->removebutton, XV_X) +
		 xv_get(p->removebutton, XV_WIDTH); 
	if (p->longest_str < tmp)
		p->longest_str = tmp;

        p->sortbutton = xv_create(p->panel, PANEL_BUTTON,
                XV_X, xv_get(p->addbutton, XV_X),
                XV_Y, xv_row(p->panel, 4)-10,
                PANEL_NOTIFY_PROC, props_access_sort_alpha,
                PANEL_CLIENT_DATA, c,
                PANEL_LABEL_STRING, LGET("Sort List"),
		XV_HELP_DATA, "cm:GASortList",
                0);

	tmp = xv_get(p->sortbutton, XV_X) +
		 xv_get(p->sortbutton, XV_WIDTH); 
	if (p->longest_str < tmp)
		p->longest_str = tmp;

	rect = (Rect *) xv_get(p->access_msg, PANEL_ITEM_RECT);

        p->perms = xv_create(p->panel, PANEL_TOGGLE,
                PANEL_CHOICE_STRINGS, LGET("Browse"), LGET("Insert"),
				  LGET("Delete"), NULL,
                PANEL_FEEDBACK, PANEL_INVERTED,
		PANEL_LABEL_STRING, LGET("Permissions:"),
                PANEL_CLIENT_DATA, c,
                XV_X, xv_get(p->username, XV_X),
		XV_Y, xv_get(p->access_msg, XV_Y) + rect->r_height + 10,
                XV_HELP_DATA, "cm:DefaultAccessLevel",
                0);

        max = xv_get(p->addbutton, XV_WIDTH);
        if (xv_get(p->removebutton, XV_WIDTH) > max)
                max = xv_get(p->removebutton, XV_WIDTH);
        if (xv_get(p->sortbutton, XV_WIDTH) > max)
                max = xv_get(p->sortbutton, XV_WIDTH);

        xv_set(p->addbutton, PANEL_LABEL_WIDTH,        
                (int)xv_get(p->addbutton, PANEL_LABEL_WIDTH) +
                max - (int)xv_get(p->addbutton, XV_WIDTH),
                NULL);
        xv_set(p->removebutton, PANEL_LABEL_WIDTH,
                (int)xv_get(p->removebutton, PANEL_LABEL_WIDTH) +
                max - (int)xv_get(p->removebutton, XV_WIDTH),
                NULL);
        xv_set(p->sortbutton, PANEL_LABEL_WIDTH,
                (int)xv_get(p->sortbutton, PANEL_LABEL_WIDTH) +
                max - (int)xv_get(p->sortbutton, XV_WIDTH),
                NULL);
}


static void
p_create_dateformat(p, pf)
	Props *p;
	Xv_font pf;
{
	int longest = 0;
	char *str;
	Font_string_dims dims;
	int order_str_width = 0, separator_str_width = 0;
	int row_height = xv_get(p->panel, WIN_ROW_HEIGHT);

	/* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL :
	 *
	 * Date Ordering = The order of month, day, and year in your local
	 * country or place.
	 * 
	 * Date Separator = The separator that is used in your local country
	 * or place.  For example, in the United States, 7/4/91 means
	 * July 4th, 1991, and the date separator is the slash '/'.
	 */
	str = LGET("Date Ordering:");
	(void)xv_get(pf, FONT_STRING_DIMS, str, &dims);
	order_str_width = longest = dims.width;
	p->ordering_str = xv_create(p->panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, str,
		PANEL_LABEL_BOLD, TRUE,
		XV_Y, xv_get(p->category_item, XV_Y) + 2 * row_height,
                XV_HELP_DATA, "cm:DateOrder",
		NULL);
	p->ordering_list = xv_create(p->panel, PANEL_CHOICE,
		PANEL_CHOICE_STRINGS, LGET("MM | DD | YY"), 
						  LGET("DD | MM | YY"),
						  LGET("YY | MM | DD"), 
						  NULL,
		PANEL_NOTIFY_PROC, changed_notify,
		PANEL_LAYOUT, PANEL_VERTICAL,
		XV_Y, xv_get(p->ordering_str, XV_Y) + row_height,
                XV_HELP_DATA, "cm:DateOrder",
		NULL);
	str = LGET("Date Separator:");
	(void)xv_get(pf, FONT_STRING_DIMS, str, &dims);
	separator_str_width = dims.width;
	if ( dims.width > longest ) {
		longest = dims.width;
	}
	p->separator_str = xv_create(p->panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, str,
		PANEL_LABEL_BOLD, TRUE,
		XV_Y, xv_get(p->ordering_list, XV_Y) + xv_get(p->ordering_list, XV_HEIGHT) + 2*row_height,
                XV_HELP_DATA, "cm:DateSep",
		NULL);
	p->separator_list = xv_create(p->panel, PANEL_CHOICE,
		PANEL_CHOICE_STRINGS, LGET("Blank"),
						  LGET("/"),
						  LGET("."),
						  LGET("-"),
						  NULL,
		PANEL_NOTIFY_PROC, changed_notify,
		PANEL_LAYOUT, PANEL_HORIZONTAL,
		XV_Y, xv_get(p->separator_str, XV_Y) + row_height,
                XV_HELP_DATA, "cm:DateSep",
		NULL);

	xv_set(p->ordering_str, XV_X, xv_col(p->panel, OFFSET) + longest - order_str_width+20, NULL);
	xv_set(p->ordering_list, XV_X, xv_col(p->panel, 7 * OFFSET), NULL);
	xv_set(p->separator_str, XV_X, xv_col(p->panel, OFFSET) + longest - separator_str_width+20, NULL);
	xv_set(p->separator_list, XV_X, xv_col(p->panel, 7 * OFFSET), NULL);
}

/* ARGSUSED */
static Panel_setting
validate_decnum(item, event)
        Panel_item      item;
        Event           *event;
{
        char    *input_str = (char*)xv_get(item, PANEL_VALUE);
        int     num_decs = 0;
        Boolean error = false;
	char save_str[81];
        Props   *p;

	/* event proc gets called for down and up event */
	if (event_is_down(event) || input_str == NULL || (*input_str == NULL))
        	return(panel_text_notify(item, event));

	cm_strcpy(save_str, input_str);
	while (input_str != NULL && *input_str != '\0') {
		if (!iscntrl(*input_str) && !isdigit(*input_str)) {
			if (*input_str == '.')
				num_decs++;
			else {
				error = true;
				break;
			}
		}
		input_str++;
	}
	if (num_decs > 1) 	
		error = true;

	if (num_decs == 1) {
		if ( ((strncmp(save_str, "0.", 2) == 0) && 
			(save_str[2] == '0' || save_str[2] == '1' || 
			save_str[2] == '2') ) ||
			(save_str[0] ==  '.' &&
			(save_str[1] == '0' || save_str[1] == '1' || 
			save_str[1] == '2')) ) {
                	p = (Props *) xv_get(item, PANEL_CLIENT_DATA);
                	notice_prompt(p->frame, (Event *)NULL,
                        	NOTICE_MESSAGE_STRINGS,  
				MGET("Invalid Entry."),
				MGET("Entry Must be 0.3 or Greater."),
                        	0,
                        	NOTICE_BUTTON_YES,  LGET("Continue") ,
				0);
			save_str[cm_strlen(save_str)-1] = '\0';
			xv_set(item, PANEL_VALUE, save_str, NULL);
        		return(PANEL_NONE);
		}
	}

        if (error) {
                p = (Props *) xv_get(item, PANEL_CLIENT_DATA);
                notice_prompt(p->frame, (Event *)NULL,
                        NOTICE_MESSAGE_STRINGS,  MGET("Invalid Entry"),
                        0,
                        NOTICE_BUTTON_YES,  LGET("Continue") ,
                        0);
		save_str[cm_strlen(save_str)-1] = '\0';
		xv_set(item, PANEL_VALUE, save_str, NULL);
        	return(PANEL_NONE);
        }
        props_changed = true;
        return(panel_text_notify(item, event));
}
static void
p_create_printer(p, pf)
	Props *p;
	Xv_font pf;
{
	int alen, tmp, wd, gap=10, row;
        char *buf, *buf2, *buf3, *abuf, *t=NULL;
        struct pr_size size1, size2, size3, size4, size5;
	struct pr_size size6, size7, size8, size9;
        Font_string_dims dims;
	Rect *rect;
	Calendar *c = (Calendar*)xv_get(p->panel, WIN_CLIENT_DATA);

	row = 1;
	t = LGET("Destination:");
        (void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size1.x = dims.width;
        size1.y = dims.height;

        t = LGET("Printer:");
        (void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size2.x = dims.width;
        size2.y = dims.height;

	t = LGET("Options:");
        (void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size3.x = dims.width;
        size3.y = dims.height;

        t = LGET("Directory:");
        (void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size4.x = dims.width;
        size4.y = dims.height;

        t = LGET("File:");
        (void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size5.x = dims.width;
        size5.y = dims.height;

        t = LGET("Width:");
        (void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size6.x = dims.width;
        size6.y = dims.height;

        t = LGET("Position:");
        (void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size7.x = dims.width;
        size7.y = dims.height;

	   /* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL :
	    *
	    * Units in this context means several things depending on the
	    * displayed calendar's view.
	    * If view is day, then "Units" means the number of days in advance.
	    * If view is week, then "Units" means the number of weeks.
	    * Likewise for month and year view.
	    */
        t = LGET("Units:");
        (void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size8.x = dims.width;
        size8.y = dims.height;

	   /* STRING_EXTRACTION SUNW_DESKSET_CM_LABEL :
	    *
	    * Privacy Type; prinpe of appts to print, may be public, 
            * private, or semiprivate. 
	    *
	    */
        t = LGET("Include:");
        (void) xv_get(pf, FONT_STRING_DIMS, t, &dims);
        size9.x = dims.width;
        size9.y = dims.height;


	wd = max(max(max(max(max(max(max(max(size1.x, size2.x), size3.x), 
		size4.x), size5.x), size6.x), size7.x), size8.x), size9.x)
                        + c->view->outside_margin;
	t = LGET("Destination:");
        p->dest_item = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, t,
                PANEL_LABEL_BOLD, TRUE,
        	XV_X, wd - size1.x,
                XV_Y, xv_row(p->panel, row),
                XV_HELP_DATA, "cm:DestChoice",
                0);
 
        p->dest_choice = xv_create(p->panel, PANEL_CHOICE,
                PANEL_CHOICE_STRINGS, LGET("Printer"), LGET("File"), 0,
                XV_X, wd + gap,
                XV_Y, xv_row(p->panel, row),
                PANEL_LAYOUT, PANEL_HORIZONTAL,
                PANEL_FEEDBACK, PANEL_INVERTED,
                PANEL_NOTIFY_PROC, dest_choice_proc,
		PANEL_CLIENT_DATA, c,
                XV_HELP_DATA, "cm:DestChoice",
                0);
 
	row++;
        t = LGET("Printer:");
        p->printerstr = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, t,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, wd - size2.x,
                XV_Y, xv_row(p->panel,row),
                XV_SHOW, !CHOICE_DEF,
                XV_HELP_DATA, "cm:PrinterName",
                0);
        p->printer_name = xv_create(p->panel, PANEL_TEXT,
                PANEL_VALUE_DISPLAY_LENGTH, 20,
                PANEL_VALUE_STORED_LENGTH, 80,
                XV_X, wd + gap,
                XV_Y, xv_row(p->panel, row),
                PANEL_NOTIFY_PROC, changed_text_notify,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
                XV_HELP_DATA, "cm:PrinterName",
                0);
	t = LGET("Options:");
        p->optionstr = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, t,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, wd - size3.x,
                XV_Y, xv_row(p->panel,row+1),
                XV_SHOW, !CHOICE_DEF,
                PANEL_NOTIFY_PROC, changed_text_notify,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
                XV_HELP_DATA, "cm:Option",
                0);
        p->options = xv_create(p->panel, PANEL_TEXT,
                PANEL_VALUE_DISPLAY_LENGTH, 20,
                XV_X, wd + gap,
                XV_Y, xv_row(p->panel, row+1),
                PANEL_NOTIFY_PROC, changed_text_notify,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
                XV_HELP_DATA, "cm:Option",
                0);
 
        t = LGET("Directory:");
        p->dirstr = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, t,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, wd - size4.x,
                XV_SHOW, CHOICE_DEF,
                XV_Y, xv_row(p->panel, row),
                XV_HELP_DATA, "cm:FileDir",
                0);
        p->dir_name = xv_create(p->panel, PANEL_TEXT,
                PANEL_VALUE_DISPLAY_LENGTH, 20,
                XV_X, wd + gap,
                XV_Y, xv_row(p->panel, row),
                XV_SHOW, FALSE,
                PANEL_NOTIFY_PROC, changed_text_notify,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
                XV_HELP_DATA, "cm:FileDir",
                0);
 
	row++;
        t = LGET("File:");
        p->filestr = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, t,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, wd - size5.x,
                XV_SHOW, CHOICE_DEF,
                XV_Y, xv_row(p->panel, row),
                XV_HELP_DATA, "cm:FileName",
                0);
        p->file_name = xv_create(p->panel, PANEL_TEXT,
                PANEL_VALUE_DISPLAY_LENGTH, 20,
                XV_X, wd + gap,
                XV_Y, xv_row(p->panel, row),
                XV_SHOW, FALSE,
                PANEL_NOTIFY_PROC, changed_text_notify,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
                XV_HELP_DATA, "cm:FileName",
                0);
 
	row++;
        t = LGET("Width:");
        p->out_width = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, t,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, wd - size6.x,
                XV_Y, xv_row(p->panel, row),
                XV_HELP_DATA, "cm:OutputWidth",
                0);
	p->scale_width = xv_create(p->panel, PANEL_TEXT,
                PANEL_VALUE_DISPLAY_LENGTH, 6,
                PANEL_VALUE_STORED_LENGTH, 6,
                PANEL_NOTIFY_PROC, validate_decnum,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
                XV_X, wd + gap,
                XV_Y, xv_row(p->panel, row),
		PANEL_CLIENT_DATA, p,
                XV_HELP_DATA, "cm:OutputWidth",
                0);
        
        t = LGET("Height:");
        p->height = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, LGET("Height:"),
                PANEL_LABEL_BOLD, TRUE,
                XV_X, xv_get(p->scale_width, XV_X) +
			 xv_get(p->scale_width, XV_WIDTH) + gap,
                XV_Y, xv_row(p->panel, row),
                XV_HELP_DATA, "cm:OutputHeight",
                0);
        rect = (Rect *) xv_get(p->height, PANEL_ITEM_RECT);
        p->scale_height = xv_create(p->panel, PANEL_TEXT,
                PANEL_VALUE_DISPLAY_LENGTH, 6,
                PANEL_VALUE_STORED_LENGTH, 6,
                PANEL_NOTIFY_PROC, validate_decnum,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
                XV_X, rect->r_left + rect->r_width + gap,
                XV_Y, xv_row(p->panel, row),
		PANEL_CLIENT_DATA, p,
                XV_HELP_DATA, "cm:OutputHeight",
                0);
 
	row++;
        t = LGET("Position:");
        p->position = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, t,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, wd - size7.x,
                XV_Y, xv_row(p->panel, row),
                XV_HELP_DATA, "cm:PositionGroup",
                0);
        p->offset_width = xv_create(p->panel, PANEL_TEXT,
                PANEL_VALUE_DISPLAY_LENGTH, 6,
                PANEL_VALUE_STORED_LENGTH, 6,
                PANEL_NOTIFY_PROC, validate_decnum,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
                XV_X, wd + gap,
                XV_Y, xv_row(p->panel, row),
		PANEL_CLIENT_DATA, p,
                XV_HELP_DATA, "cm:LeftOffset",
                0);
        rect = (Rect *) xv_get(p->offset_width, PANEL_ITEM_RECT);
        p->inches_left = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, LGET("Inches from left"),
                PANEL_LABEL_BOLD, TRUE,
                XV_X, rect->r_left + rect->r_width + gap,
                XV_Y, xv_row(p->panel, row),
                XV_HELP_DATA, "cm:LeftOffset",
                0);

	row++;
        p->offset_height = xv_create(p->panel, PANEL_TEXT,
                PANEL_VALUE_DISPLAY_LENGTH, 6,
                PANEL_VALUE_STORED_LENGTH, 6,
                PANEL_NOTIFY_PROC, validate_decnum,
                PANEL_NOTIFY_LEVEL, PANEL_ALL,
                XV_X, xv_get(p->offset_width, XV_X),
                XV_Y, xv_row(p->panel, row),
		PANEL_CLIENT_DATA, p,
                XV_HELP_DATA, "cm:BottomOffset",
                0);
        rect = (Rect *) xv_get(p->offset_height, PANEL_ITEM_RECT);
        p->inches_bottom = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, LGET("Inches from bottom"),
                PANEL_LABEL_BOLD, TRUE,
                XV_X, rect->r_left + rect->r_width + gap,
                XV_Y, xv_row(p->panel, row),
                XV_HELP_DATA, "cm:BottomOffset",
                0);

	row++;
        t = LGET("Units:");
        p->repeatstr = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, t,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, wd - size8.x,
                XV_Y, xv_row(p->panel, row),
                XV_HELP_DATA, "cm:NumberOfItems",
                0);
        p->repeat_times = xv_create(p->panel, PANEL_NUMERIC_TEXT,
		PANEL_MIN_VALUE, 1,
                PANEL_VALUE_DISPLAY_LENGTH, 2,
                PANEL_VALUE_STORED_LENGTH, 2,
                XV_X, wd + gap,
                XV_Y, xv_row(p->panel, row),
                PANEL_NOTIFY_PROC, changed_text_notify,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
                XV_HELP_DATA, "cm:NumberOfItems",
                0);

        t = LGET("Copies:");
        p->copiesstr = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, t,
                PANEL_LABEL_BOLD, TRUE,
                XV_X, xv_get(p->repeat_times, XV_X) +
			xv_get(p->repeat_times, XV_WIDTH) + gap,
                XV_Y, xv_row(p->panel, row),
                XV_HELP_DATA, "cm:NumberOfPages",
                0);
        p->copies = xv_create(p->panel, PANEL_NUMERIC_TEXT,
		PANEL_MIN_VALUE, 1,
                PANEL_VALUE_DISPLAY_LENGTH, 2,
                PANEL_VALUE_STORED_LENGTH, 2,
                XV_X, xv_get(p->copiesstr, XV_X) +
                        xv_get(p->copiesstr, XV_WIDTH) + gap,
                XV_Y, xv_row(p->panel, row),
                PANEL_NOTIFY_PROC, changed_text_notify,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
                XV_HELP_DATA, "cm:NumberOfPages",
                0);

	row++;
	t = LGET("Include:");
        p->meo_str = xv_create(p->panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, t,
                PANEL_LABEL_BOLD, TRUE,
        	XV_X, wd - size9.x,
                XV_Y, xv_row(p->panel, row),
                XV_HELP_DATA, "cm:meo",
                0);
	abuf = MGET("Appts"); 
	alen = cm_strlen(abuf);
	buf  = (char*)ckalloc(cm_strlen(privacystr[0])+alen+5);
	buf2  = (char*)ckalloc(cm_strlen(privacystr[2])+alen+5);
	buf3  = (char*)ckalloc(cm_strlen(privacystr[1])+alen+5);
	sprintf(buf, "\"%s\" %s", privacystr[0], abuf);
	sprintf(buf2, "\"%s\" %s", privacystr[2], abuf);
	sprintf(buf3, "\"%s\" %s", privacystr[1], abuf);
        p->meo = xv_create(p->panel, PANEL_CHOICE,
                PANEL_CHOICE_STRINGS, buf, buf2, buf3, 0,
		PANEL_CHOOSE_ONE, FALSE,
		XV_X, xv_get(p->meo_str, XV_X) + 
			xv_get(p->meo_str, XV_WIDTH) + gap,
		 XV_Y, xv_row(p->panel, row),
		PANEL_CLIENT_DATA, c,
                PANEL_NOTIFY_PROC, changed_notify,
		PANEL_VALUE, (int)MEO_DEF,
		PANEL_LAYOUT, PANEL_VERTICAL, 
                XV_HELP_DATA, "cm:meo",
                0);
	free(buf); free(buf2); free(buf3);
	tmp = xv_get(p->meo, XV_X) + xv_get(p->meo, XV_WIDTH);
	if (p->longest_str < tmp)
		p->longest_str = tmp;
	tmp = xv_get(p->copies, XV_X) + xv_get(p->copies, XV_WIDTH);
	if (p->longest_str < tmp)
		p->longest_str = tmp;
}

/* ARGSUSED */
static void
p_category_notify(item, value, event)
	Panel_item item;
	int value;
	Event *event;
{
	Calendar *c;
	Props *p;
	int not_val;
	static int old_value;
	Boolean switch_it = true;

	c = (Calendar*)xv_get(item, PANEL_CLIENT_DATA);
	p = (Props*) c->properties;

	if (props_changed) {
		xv_set(item, PANEL_VALUE, old_value, NULL);
		not_val = notice_prompt(p->panel, event,
                    NOTICE_MESSAGE_STRINGS,
                    MGET("Changes to this category have not been applied."),
                    0,
                    NOTICE_BUTTON_YES, LGET("Apply Changes"),
                    NOTICE_BUTTON_NO, LGET("Discard Changes"),
                    NOTICE_BUTTON, LGET("Cancel"), 2,
                    0);
		switch (not_val) {
        		case NOTICE_YES:
				(void)p_apply_proc(item, event);
				xv_set(item, PANEL_VALUE, value, NULL);
          			break;
        		case NOTICE_NO:
				(void)p_reset_proc(item, event);
				xv_set(item, PANEL_VALUE, value, NULL);
          			break;
        		default:
				switch_it = false;
				break;	
		}
	}

	if (switch_it) {
        	p_turnoff_last_pane(p);
		/* set current view pane*/
		switch ((int)xv_get(item, PANEL_VALUE)) {
		case EDITOR_DEFAULTS:
    			p_show_edefs(p, TRUE);
			break;
		case DISPLAY_SETTINGS:
    			p_show_dispset(p, TRUE);
			break;
          	case GROUP_ACCESS_LISTS:
    			p_show_access(p, TRUE);
			break;
          	case PRINTER_OPTS:
    			p_show_printer(p, TRUE);
			break;
		case DATE_FORMAT:
			p_show_dateformat(p, TRUE);
			break;
          	default:
    			p_show_edefs(p, TRUE);
			break;
		}
	}
	old_value = value;
	props_changed = false;
}

static void
p_turnoff_last_pane(p)
	Props *p;
{
	switch (p->last_props_pane)
        {
		case EDITOR_DEFAULTS:
          		p_show_edefs(p, FALSE);
          		break;
		case DISPLAY_SETTINGS:
          		p_show_dispset(p, FALSE);
          		break;
		case GROUP_ACCESS_LISTS:
          		p_show_access(p, FALSE);
          		break;
		case PRINTER_OPTS:
          		p_show_printer(p, FALSE);
			break;
		case DATE_FORMAT:
			p_show_dateformat(p, FALSE);
			break;
          	default:  
          		p_show_edefs(p, FALSE);
          		break;
        }
}

static void
p_reset_panel(relative_item, p)
	Panel_item relative_item;
	Props *p;
{
	Rect *rect;

	rect = (Rect *) xv_get(relative_item, PANEL_ITEM_RECT);
	xv_set(p->apply_button, XV_Y, xv_get(relative_item, XV_Y) + rect->r_height + 35, NULL);
	xv_set(p->reset_button, XV_Y, xv_get(p->apply_button, XV_Y), NULL);
	xv_set(p->defaults_button, XV_Y, xv_get(p->apply_button, XV_Y), NULL);

	xv_set(p->panel, XV_HEIGHT, xv_get(p->apply_button, XV_Y) +
		xv_get(p->apply_button, XV_HEIGHT) + 25, NULL);
	xv_set(p->frame, WIN_FIT_HEIGHT, 0, NULL);
}

static void
p_show_edefs(p, show)
	Props *p;
	int show;
{

	p_reset_panel(p->privacystr, p);
	xv_set(p->reminderstr, XV_SHOW, show, NULL);
	xv_set(p->reminder, XV_SHOW, show, NULL);
	xv_set(p->beepadvance, XV_SHOW, show, NULL);
	xv_set(p->beepunitstack, XV_SHOW, show, NULL);
	xv_set(p->beepunit, XV_SHOW, show, NULL);
	xv_set(p->flashadvance, XV_SHOW, show, NULL);
	xv_set(p->flashunitstack, XV_SHOW, show, NULL);
	xv_set(p->flashunit, XV_SHOW, show, NULL);
	xv_set(p->openadvance, XV_SHOW, show, NULL);
	xv_set(p->openunitstack, XV_SHOW, show, NULL);
	xv_set(p->openunit, XV_SHOW, show, NULL);
	xv_set(p->mailadvance, XV_SHOW, show, NULL);
	xv_set(p->mailunitstack, XV_SHOW, show, NULL);
	xv_set(p->mailunit, XV_SHOW, show, NULL);
	xv_set(p->mailto, XV_SHOW, show, NULL);
	xv_set(p->mailtostr, XV_SHOW, show, NULL);
	xv_set(p->privacystr, XV_SHOW, show, NULL);
	xv_set(p->privacystack, XV_SHOW, show, NULL);
	xv_set(p->privacyunit, XV_SHOW, show, NULL);
	if (show) { 
		xv_set(p->category_item, PANEL_VALUE, 0, NULL);
		p->last_props_pane = EDITOR_DEFAULTS;
	}
}

static void
p_show_dispset(p, show)
	Props *p;
	int show;
{

	p_reset_panel(p->defcal, p);
	xv_set(p->dayboundstr, XV_SHOW, show, NULL);
	xv_set(p->beginstr, XV_SHOW, show, NULL);
	xv_set(p->beginslider, XV_SHOW, show, NULL);
	xv_set(p->beginslider_str, XV_SHOW, show, NULL);
	xv_set(p->endstr, XV_SHOW, show, NULL);
	xv_set(p->endslider, XV_SHOW, show, NULL);
	xv_set(p->endslider_str, XV_SHOW, show, NULL);
	xv_set(p->defdispstr, XV_SHOW, show, NULL);
	xv_set(p->default_disp, XV_SHOW, show, NULL);
	xv_set(p->default_view, XV_SHOW, show, NULL);
	xv_set(p->defviewstr, XV_SHOW, show, NULL);
	xv_set(p->defcalstr, XV_SHOW, show, NULL);
	xv_set(p->defcal, XV_SHOW, show, NULL);
	xv_set(p->clocstr, XV_SHOW, show, NULL);
	xv_set(p->cloc, XV_SHOW, show, NULL);
	if (show) {
		xv_set(p->category_item, PANEL_VALUE, 1, NULL);
		p->last_props_pane = DISPLAY_SETTINGS;
	}
}

static void
p_show_access(p, show)
	Props *p;
	int show;
{
	p_reset_panel(p->perms, p);
	xv_set(p->access_list, XV_SHOW, show, NULL);
	xv_set(p->access_msg, XV_SHOW, show, NULL);
	xv_set(p->username, XV_SHOW, show, NULL);
        xv_set(p->perms, XV_SHOW, show, NULL);
        xv_set(p->addbutton, XV_SHOW, show, NULL);
        xv_set(p->removebutton, XV_SHOW, show, NULL);
        xv_set(p->sortbutton, XV_SHOW, show, NULL);
	if (show) { 
		xv_set(p->category_item, PANEL_VALUE, 2, NULL);
		p->last_props_pane = GROUP_ACCESS_LISTS;
	}
}

static void
p_show_printer(p, show)
	Props *p;
	int show;
{
	p_reset_panel(p->meo, p);
	xv_set(p->dest_item, XV_SHOW, show, NULL);
	xv_set(p->dest_choice, XV_SHOW, show, NULL);
	if (xv_get(p->dest_choice, PANEL_VALUE)) { /* File */
		xv_set(p->dirstr, XV_SHOW, show, NULL);
        	xv_set(p->dir_name, XV_SHOW, show, NULL);
        	xv_set(p->file_name, XV_SHOW, show, NULL);
        	xv_set(p->filestr, XV_SHOW, show, NULL);
	}
	else { /* Printer */
		xv_set(p->printerstr, XV_SHOW, show, NULL);
        	xv_set(p->printer_name, XV_SHOW, show, NULL);
        	xv_set(p->optionstr, XV_SHOW, show, NULL);
        	xv_set(p->options, XV_SHOW, show, NULL);
	}
        xv_set(p->meo_str, XV_SHOW, show, NULL);
        xv_set(p->meo, XV_SHOW, show, NULL);
        xv_set(p->height, XV_SHOW, show, NULL);
        xv_set(p->position, XV_SHOW, show, NULL);
        xv_set(p->out_width, XV_SHOW, show, NULL);
        xv_set(p->offset_width, XV_SHOW, show, NULL);
        xv_set(p->inches_left, XV_SHOW, show, NULL);
        xv_set(p->scale_width, XV_SHOW, show, NULL);
        xv_set(p->offset_height, XV_SHOW, show, NULL);
        xv_set(p->scale_height, XV_SHOW, show, NULL);
        xv_set(p->inches_bottom, XV_SHOW, show, NULL);
        xv_set(p->repeatstr, XV_SHOW, show, NULL);
        xv_set(p->repeat_times, XV_SHOW, show, NULL);
        xv_set(p->copiesstr, XV_SHOW, show, NULL);
        xv_set(p->copies, XV_SHOW, show, NULL);
	if (show) { 
		xv_set(p->category_item, PANEL_VALUE, 3, NULL);
		p->last_props_pane = PRINTER_OPTS;
	}
}

static void
p_show_dateformat(p, show)
	Props *p;
	int show;
{
	p_reset_panel(p->separator_list, p);
	xv_set(p->ordering_str, XV_SHOW, show, NULL);
	xv_set(p->ordering_list , XV_SHOW, show, NULL);
	xv_set(p->separator_str, XV_SHOW, show, NULL);
	xv_set(p->separator_list , XV_SHOW, show, NULL);

	if (show) {
		xv_set(p->category_item, PANEL_VALUE, 4, NULL);
		p->last_props_pane = DATE_FORMAT;
	}
}

/* ARGSUSED */
static Notify_value
beginslider_notify(item, value, event)
	Panel_item item;
	int value;
	Event *event;
{
	char buf[TIMEBUF];
	Props *p;

	p = (Props *) xv_get(item, PANEL_CLIENT_DATA);

	format_time(value*60, buf, p->default_disp_VAL);
	(void) xv_set(p->beginslider_str, PANEL_LABEL_STRING, buf, 0);

	if (value >= (int)xv_get(p->endslider, PANEL_VALUE))
	{
		(void) xv_set(p->endslider, PANEL_VALUE, value+1, 0);
		endslider_notify(p->endslider, value+1, event);
	}
	props_changed = true;  

	return(NOTIFY_DONE);
}


/* ARGSUSED */
static Notify_value
endslider_notify(item, value, event)
	Panel_item item;
	int value;
	Event *event;
{
	char buf[TIMEBUF];
	Props *p;

	p = (Props *) xv_get(item, PANEL_CLIENT_DATA);

	format_time(value*60, buf, p->default_disp_VAL);
	(void) xv_set(p->endslider_str, PANEL_LABEL_STRING, buf, 0);

	if (value <= (int)xv_get(p->beginslider, PANEL_VALUE))
	{
		(void) xv_set(p->beginslider, PANEL_VALUE, value-1, 0);
		beginslider_notify(p->beginslider, value-1, event);
	}
	props_changed = true;  
	
	return(NOTIFY_DONE);
}

/* ARGSUSED */
static Notify_value
changed_notify(item, value, event)
        Panel_item item;
        int value;
        Event *event;
{
	props_changed = true;

        return(NOTIFY_DONE);
}
/* ARGSUSED */
static Panel_setting
changed_text_notify(item, event)
        Panel_item item;
        Event *event;
{
	props_changed = true;

        return(panel_text_notify(item, event));
}

static void
props_access_write_names(p)
        Props *p;
{
        int     bufsiz = BUFSIZ;
        int     bufused = 0, i = 0, j = 0;
        char    *buf = (char *) ckalloc(bufsiz);
        char    *name;

        /* take from browse list first, in case a 'sort' just took place */
        if (props_showing(p)) {
                for (i = xv_get(p->access_list, PANEL_LIST_NROWS); j < i; j++) {
                        name = (char*)list_get_entry(p->access_list, j);
                        /* see if we have enough space to add it.  If not
                        then realloc the buffer bigger */
                        if ((bufused + cm_strlen(name) + 3) > bufsiz)
                                buf = (char*)realloc(buf,
                                                (unsigned)(bufsiz += BUFSIZ));                        (void) cm_strcat(buf, name);
                        (void) cm_strcat(buf, " ");
                        bufused += cm_strlen(name) + 1;
                }
        }
	cal_update_props();
	cal_set_property(property_names[CP_DAYCALLIST], buf);
	cal_set_properties();
        free(buf);
}
