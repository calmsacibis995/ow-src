#ifndef lint
static char sccsid[]="@(#)printtool.c	3.50 06/13/96 Copyr 1987-1990 Sun Micro";
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
 *  this code is provided to the user or company in possession of this *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <xview/dragdrop.h>
#include <xview/cursor.h>
#include <gdd.h>
#include <group.h>
#include "ds_xlib.h"
#include "printtool_ui.h"

#include <xview/font.h>
#include <xview/notice.h>
#include <xview/scrollbar.h>
#include <sys/stat.h>
#include "desktop/ce.h"
#include "desktop/ce_err.h"
#include <xview/cms.h>
#include "printtool.h"
#include <sys/fcntl.h>

/* VARARGS */

#define RIGHT_MARGIN	17
#define DEFAULT_DOC	"default-doc"

void	enable_rm_button(Panel_item, char *, Xv_opaque, Panel_list_op, Event *, int);

static void	print_footer_msg(int, char*, char*);
static void	status_printer();
static void	console_alert(char*);
static void	set_info_help();
static void	set_props_help();
static char*    get_file_type_attrs(char*, char*, char**, char**);
void		set_title();
void		read_resources();
void		set_prop_values();
void		write_resources();
char*		set_bool();
int 		delete_file = 1;

/*
 * Global object definitions.
 */
printtool_window1_objects	*Printtool_window1;
printtool_info_popup_objects	*Printtool_info_popup = NULL;
printtool_prop_popup_objects	*Printtool_prop_popup = NULL;

Printtool_Mobject		*Ptmo;  /* printtool misc object */
int				Prop_bt_minx;  /* prop button min XV_X */
int				isroot = 0;  /* is root running? */
char				Format[25];


/*
 * Instance XV_KEY_DATA key.  An instance is a set of related
 * user interface objects.  A pointer to an object's instance
 * is stored under this key in every object.  This must be a
 * global variable.
 */
Attr_attribute	INSTANCE;

main(int argc, char **argv)
{
	void init_printtool(), init_icons();
	void init_fonts();
	static Notify_value frame_event_proc();
	static Notify_value panel_event_proc();
	static Notify_value quit_proc();
        char locale_dir[MAXPATHLEN];

	/*
	 * As the default, use the current directory as a pointer to
	 * the message object (.mo) file(s). dgettext() will search for
	 * the file(s) in the "./<current_locale>/LC_MESSAGES" directory.
	 * Change the bindtextdomain call path argument if the .mo files
	 * reside in some other location.
	 */
	ds_expand_pathname("$OPENWINHOME/lib/locale", locale_dir);
	bindtextdomain(DOMAIN_NAME, locale_dir);
	
	/*
	 * Initialize XView.
	 */
	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
		XV_USE_LOCALE, TRUE,
		NULL);
	INSTANCE = xv_unique_key();
	
	/*
	 * Initialize user interface components.
	 * Do NOT edit the object initializations by hand.
	 */
	Printtool_window1 = printtool_window1_objects_initialize(NULL, NULL);
/*	Printtool_info_popup = printtool_info_popup_objects_initialize(NULL, Printtool_window1->window1);*/
/*	Printtool_prop_popup = printtool_prop_popup_objects_initialize(NULL, Printtool_window1->window1);*/


	/*
	 * Initialize the Drag Drop package.
	 */
	gdd_init_dragdrop(Printtool_window1->window1);

	right_justify();
	/* initialize printtool mis. object */
	init_printtool();	/* malloc memory, null out vars, etc */
	load_resources();	/* Get resources from various places. */
	read_resources();	/* Read all possible resources from database */
	init_prop_values();   	/* reflect resources on panels */ 

	get_uname();
	get_options(argc, argv); /* Read and process command line options. */
	get_printer_list(); /* get the list of installed printers */ 
        init_icons();     /* create image and set icon to the frame */
	init_fonts();     /* get proportion and fixed width fonts */
	init_gc(); 	  /* create gc's for panel and icon for flashing */
	writerc();            /* save printtool current status */
	set_on_line_help();
	set_title();	  /* set fixed width font for scrolling list title */

        notify_interpose_event_func(Printtool_window1->window1, frame_event_proc, NOTIFY_SAFE) ;
        notify_interpose_event_func(Printtool_window1->controls1, panel_event_proc, NOTIFY_SAFE) ;
        notify_interpose_destroy_func(Printtool_window1->window1, quit_proc) ;
	
	xv_set(Printtool_window1->window1, FRAME_CLOSED, TRUE, 0);
	/*
	 * Turn control over to XView.
	 */
	xv_main_loop(Printtool_window1->window1);
	exit(0);
}


/*
 * Drop callback function for `drop_target1'.
 */
void
load_event_proc(Xv_opaque item, Event *event, GDD_DROP_INFO *drop_info)
{
	printtool_window1_objects *ip = (printtool_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);

	if (drop_info ->tmpfile && *(drop_info->tmpfile)) {
	      /* use tmpfile */
	   print_file(drop_info->tmpfile,
		      drop_info->data_label ? drop_info->data_label : NULL);
	}
	else if (drop_info->filename && *(drop_info->filename)) {
	      /* use filename */
	   print_file(drop_info->filename,
		      drop_info->data_label ? drop_info->data_label : NULL);
	} /* end else if */

	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */
}

/*
 * Notify callback function for `print_bt'.
 */
void
print_button(Panel_item item, Event *event)
{
	printtool_window1_objects *ip = (printtool_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	/* get filename from text field which doesn't have data_label */
	print_file((char *) xv_get(ip->filename_tx, PANEL_VALUE), NULL);
}


print_file(char *filename, char* label)
{
   char *print_method = NULL, *type_name = NULL, *printer = NULL; 
   char copies[5], header[20];
   int  choice; 
   char full_filename[MAXPATHLEN];
   char full_printmethod[MAXPATHLEN];
   char buf[2048]; /* Need to make buf big enough for the print command */
   char *lc;
   struct stat stat_info;
   
   /* set filename field if file is from drop target */
   xv_set(Printtool_window1->filename_tx, PANEL_VALUE, filename, 0);
   
   /* expand "~" and environment variable */
   ds_expand_pathname(filename, full_filename);

   if (*filename == NULL) {
      print_footer_msg(BF, dgettext(DOMAIN_NAME, "No File to print"), ""); 
      return;
   }
   if (stat(full_filename, &stat_info) == 0)
   	if ((stat_info.st_mode & S_IFMT) == S_IFDIR) {
		notice_prompt(Printtool_window1->window1, (Event *) NULL,
                     NOTICE_MESSAGE_STRINGS, dgettext(DOMAIN_NAME, "Error: Filename specified is a directory"), 0,
                     NOTICE_BUTTON, dgettext(DOMAIN_NAME, "OK"), 101,
                     0);
      		return;
	}

   /* get orientation */
   /* support for landscape
   if ((int) xv_get(Printtool_window1->morientn_ch, PANEL_VALUE))
      orient = "-o length=45 -y landscape"; 
   else
      orient = "";
   */
   sprintf(copies, "%d", (int)xv_get(Printtool_window1->copy_tx, PANEL_VALUE));

   /* get header page */
   if ((int)xv_get(Printtool_window1->mheader_tg, PANEL_VALUE))
      strcpy(header, " ");
   else
      strcpy(header, "-o nobanner");
   printer = (char*)xv_get(Printtool_window1->printer_ls, PANEL_LIST_STRING, 
	 (int)xv_get(Printtool_window1->printer_ls, PANEL_LIST_FIRST_SELECTED));
   /* if set_env return 0, ie something wrong in set_env */
   if (!set_env(filename, copies, printer, header))
      return;

   xv_set(Printtool_window1->window1, FRAME_BUSY, TRUE, 0);

   print_footer_msg(NO_BF, dgettext(DOMAIN_NAME, "Printing %s"),
			label ? label : full_filename);

   get_file_type_attrs(full_filename, label, &print_method, &type_name);
   if (!is_printable(type_name)) {
      char err_str[256];
      sprintf(err_str, 
	 dgettext(DOMAIN_NAME, "The document: \n   \"%s\" \n cannot be printed because it is unreadable."), label ? label : filename);
      notice_prompt(Printtool_window1->window1, (Event *) NULL,
		NOTICE_MESSAGE_STRINGS, err_str, NULL,
		NOTICE_BUTTON, dgettext(DOMAIN_NAME, "Cancel"), 101,
		0);
      xv_set(Printtool_window1->window1, FRAME_BUSY, FALSE, 0);
      print_footer_msg(NO_BF, "", "");
      return;
   }
   /* if default print_method, then use print_mehtod, else use custom one*/
   if (!Ptmo->override_pmethod) {
      if (print_method && *print_method) { /* use default printer method */

         /* expand "~" and environment variable */
         expand_pathname(print_method, full_printmethod);
         /* Since lp will ignore the environment variable with -T option, 
          * -d will enforces the lp to use the selected printer
          */
	 if (strstr(full_printmethod, "fmclient") == NULL)
		sprintf(buf, "PRINTER=\"%s\";FILE=\"%s\";LPDEST=\"%s\";COPIES=\"%s\";(%s -d \"%s\" -n%s %s -c ) > %s 2>&1",
	 	printer, full_filename, printer, copies, full_printmethod,
		 printer, copies, header, Ptmo->errfile);
	else {
         	sprintf(buf, "PRINTER=\"%s\";FILE=\"%s\";LPDEST=\"%s\";COPIES=\"%s\";(%s ) > %s 2>&1",
	 	printer, full_filename, printer, copies, full_printmethod, Ptmo->errfile);
		delete_file = 0;
	}
      } /* end if print_method */
      else {   /* no default print_method found */
         sprintf(buf, "/usr/bin/lp -c -d \"%s\" -n%s %s \"%s\" > %s 2>&1",
         printer, copies, header, full_filename, Ptmo->errfile);
      } /* end else */
   } /* end if */
   else {  			/* use custom print_method */
      /* get print method value if print method field shown */
      if ((int) xv_get(Printtool_window1->filter_tx, XV_SHOW)) {
         print_method = (char *) xv_get(Printtool_window1->filter_tx, PANEL_VALUE);
      }
      if (print_method && *print_method) {
         /* expand "~" and environment variable */
         expand_pathname(print_method, full_printmethod);
         sprintf(buf, "%s > %s 2>&1", full_printmethod, Ptmo->errfile);
      }
      else if (*Ptmo->print_method) {/* no print_method found but in cmd line */
            /* expand "~" and environment variable */
            expand_pathname(Ptmo->print_method, full_printmethod);

	    /* use the one in database or in cmd line */
            xv_set(Printtool_window1->filter_tx, 
	  	     PANEL_VALUE, Ptmo->print_method, NULL);
            sprintf(buf, "%s > %s 2>&1", full_printmethod, Ptmo->errfile);
	 } /* end if Ptmo */
	 else { /* no print_method specify */
	    notice_prompt(Printtool_window1->window1, (Event *)
		     NULL,
		     NOTICE_MESSAGE_STRINGS, 
		     dgettext(DOMAIN_NAME, "No print method specified"),
                     NULL,
                     NOTICE_BUTTON, dgettext(DOMAIN_NAME, "OK"), 101,
                     0);
            xv_set(Printtool_window1->window1, FRAME_BUSY, FALSE, 0);
	    print_footer_msg(NO_BF, "", ""); /* clear footer */
            return;
         } /* end else no print method */
   } /* end else */

   if (Ptmo->testing) {
      fprintf(stdout, 
	 dgettext(DOMAIN_NAME, "%s: test: building print command \n\"%s\"\n"),
	 PROGNAME, buf);
      xv_set(Printtool_window1->window1, FRAME_BUSY, FALSE, 0);
      return;
   }
   
   lc = getenv ("LC_ALL");
   set_LC_ALL(NULL);

   if (!system(buf)) {
      set_print_check(1, 1, Ptmo->check_interval, 0);
      set_LC_ALL (lc);
   }
   else {
      FILE *fp = NULL; 
      set_LC_ALL (lc);
      if ((fp = fopen(Ptmo->errfile, "r")) != NULL) {
	 fgets(buf, 80, fp);
	 fclose(fp);
	 print_footer_msg(BF, buf, "");
      
      } /* end if open */
	
      else
	 print_footer_msg(BF, dgettext(DOMAIN_NAME, "Print Error - check the console for more details"), "");
   } /* end else system */

	/* gxv_start_connections DO NOT EDIT THIS SECTION */
	/* gxv_end_connections */
   xv_set(Printtool_window1->window1, FRAME_BUSY, FALSE, 0);
   return;
}


/* support multi word filename.  csh substitutes of multi word
filename doesn't include the quotes, therefore, add quotes between $FILE 
before expand the pathname */

expand_pathname(char *print_method, char *full_printmethod)
{
   char pm_tmp[MAXPATHLEN];
   char rear_part[MAXPATHLEN];
   char pm_orig[MAXPATHLEN];
   char *ptr = NULL, *d_ptr;
   int i, ptr_len = 0;

   strcpy(pm_orig, print_method);
   if ((d_ptr = ptr = (char*)strstr(pm_orig, "$FILE")) != NULL) {
      ptr_len = strlen(ptr);
      for (i = 0; i < 5; i++)
	 *ptr++;
   }
   if (d_ptr) {
      if (ptr)
         strcpy(rear_part, ptr);  /* save the rear_part */ 
      else
         strcpy(rear_part, "");
      pm_orig[strlen(pm_orig) - ptr_len] = '\0';
      sprintf(pm_tmp, "%s \"%s\" %s", pm_orig, "$FILE", rear_part);
   }
   else  /* no $FILE */
      strcpy(pm_tmp, print_method);
   ds_expand_pathname(pm_tmp, full_printmethod);
   return;
}

int
set_env(char* filename, char* copies, char* printer, char* header)
{
   static char filestr[1029];   /* needs to be static for putenv() */
   static char copystr[81];     /* "" */
   static char lpdeststr[129]; /* "" */
   static char printerstr[129]; /* "" */
   static char headerstr[20];     /* "" */
   char full_filename[MAXPATHLEN];

   sprintf(lpdeststr, "%s=%s", "LPDEST", printer);
   if (putenv(lpdeststr)) {
      print_footer_msg(BF, dgettext(DOMAIN_NAME, "Unable to set LPDEST environment var %s"), printer);
      return 0;
   }
   sprintf(printerstr, "%s=%s", "PRINTER", printer);
   if (putenv(printerstr)) {
      print_footer_msg(BF, dgettext(DOMAIN_NAME, "Unable to set PRINTER environment var %s"), printer);
      return 0;
   }
   ds_expand_pathname(filename, full_filename);
   sprintf(filestr, "%s=%s", "FILE", full_filename);
   if (putenv(filestr)) {
      print_footer_msg(BF, dgettext(DOMAIN_NAME, "Unable to set FILE environment var %s"), filename);
      return 0;
   }
   sprintf(copystr, "%s=%s", "COPIES", copies);
   if (putenv(copystr)) {
      print_footer_msg(BF, dgettext(DOMAIN_NAME, "Unable to set COPIES environment var %s"), copies);
      return 0;
   }
   sprintf(headerstr, "%s=%s", "HEADER", header);
   if (putenv(headerstr)) {
      print_footer_msg(BF, dgettext(DOMAIN_NAME, "Unable to set HEADER environment var %s"), header);
      return 0;
   }
   return 1;
}


/*
 * Notify callback function for `prop_bt'.
 */
void
prop_button(Panel_item item, Event *event)
{
	printtool_window1_objects *ip = (printtool_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	if ( !Printtool_prop_popup ) {
	    Printtool_prop_popup = printtool_prop_popup_objects_initialize(NULL, Printtool_window1->window1);
	    set_prop_values();
	    set_props_help();
	}

	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	xv_set(Printtool_prop_popup->prop_popup, FRAME_CMD_PUSHPIN_IN, TRUE, NULL);
	xv_set(Printtool_prop_popup->prop_popup, XV_SHOW, TRUE, NULL);
	
	/* gxv_end_connections */

}

/*
 * Notify callback function for `filename_tx'.
 */
Panel_setting
filename_text(Panel_item item, Event *event)
{
	printtool_window1_objects *ip = (printtool_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	char *	value = (char *) xv_get(item, PANEL_VALUE);
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

	print_file(value, NULL);

	return panel_text_notify(item, event);
}

/*
 * Notify callback function for `info_bt'.
 */
void
info_button(Panel_item item, Event *event)
{
	char *printer, *desc = NULL, *tmp_str;
	char cmd_str[128];
	FILE *fp;

	printtool_window1_objects *ip = (printtool_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);

	if ( !Printtool_info_popup ) {
	    Printtool_info_popup = printtool_info_popup_objects_initialize(NULL, Printtool_window1->window1);
	    init_sw_popup();  /* set no confirm for information popup */
	    set_info_help();
	}
	
        printer = (char*)xv_get(ip->printer_ls, PANEL_LIST_STRING, 
                (int)xv_get(ip->printer_ls, PANEL_LIST_FIRST_SELECTED));
        sprintf(cmd_str, "lpstat -p %s -D", printer);
	textsw_reset(Printtool_info_popup->textpane1, 0, 0);

	set_LC_ALL("C");
        if ((fp = popen(cmd_str, "r")) != NULL) {
           while (fgets(cmd_str, sizeof(cmd_str), fp) != NULL) {
	      if ((tmp_str = strstr(cmd_str, "Description")) != NULL) {
	         strtok(tmp_str, ":");
	         desc = (char *) strtok(NULL, "\n");
		 if (strcmp(desc, " ") == 0) { /* no description */
		    set_LC_ALL(NULL);
		    sprintf(cmd_str, "%s %s",
		       dgettext(DOMAIN_NAME, "No information has been set by System Administrator for"), printer);
	            textsw_insert(Printtool_info_popup->textpane1, 
				  cmd_str, strlen(cmd_str));
	            textsw_insert(Printtool_info_popup->textpane1, "\n", 1);
		    break;
		 } /* end if !desc */
		 while (isspace(*desc))
		    *desc++;
	         textsw_insert(Printtool_info_popup->textpane1, desc, 
			       strlen(desc));
	         textsw_insert(Printtool_info_popup->textpane1, "\n", 1);
	         while (fgets(cmd_str, sizeof(cmd_str), fp) != NULL) 
		    textsw_insert(Printtool_info_popup->textpane1, 
				  cmd_str, strlen(cmd_str));
	      } /* end if */
           } /* end while */
           pclose(fp);
        } /* end if */
	set_LC_ALL(NULL);
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	xv_set(Printtool_info_popup->info_popup, FRAME_CMD_PUSHPIN_IN, TRUE, NULL);
	xv_set(Printtool_info_popup->info_popup, XV_SHOW, TRUE, NULL);
	
	/* gxv_end_connections */

}

/*
 * Notify callback function for `queue_ls'.
 */
int
queue_list(Panel_item item, char *string, Xv_opaque client_data, Panel_list_op op, Event *event, int row)
{
	printtool_window1_objects *ip = (printtool_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	switch(op) {
	case PANEL_LIST_OP_DESELECT:
		break;

	case PANEL_LIST_OP_SELECT:
		break;

	case PANEL_LIST_OP_VALIDATE:
		break;

	case PANEL_LIST_OP_DELETE:
		break;
	}
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	enable_rm_button(item, string, client_data, op, event, row);
	
	/* gxv_end_connections */

	return XV_OK;
}

/*
 * User-defined action for `queue_ls'.
 */
void
enable_rm_button(Panel_item item, char *string, Xv_opaque client_data, Panel_list_op op, Event *event, int row)
{
	printtool_window1_objects *ip = (printtool_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	if ((int)xv_get(ip->queue_ls, PANEL_LIST_FIRST_SELECTED) >= 0) {
	   /* if entry is selected, enable the remove button */
	   if ((int)xv_get(ip->rm_job_bt, PANEL_INACTIVE))
	      xv_set(ip->rm_job_bt, PANEL_INACTIVE, FALSE, NULL);
	}
	else  /* if no entry is selected, disable remove button */
	   xv_set(ip->rm_job_bt, PANEL_INACTIVE, TRUE, NULL);
}

/*
 * Notify callback function for `queue_bt'.
 */
void
queue_button(Panel_item item, Event *event)
{
	printtool_window1_objects *ip = (printtool_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
        print_footer_msg(NO_BF, 
	   dgettext(DOMAIN_NAME, "Checking %s print queue"), 
           ((char*)xv_get(ip->printer_ls, PANEL_LIST_STRING, 
                (int)xv_get(ip->printer_ls, PANEL_LIST_FIRST_SELECTED))));
	xv_set(ip->window1, FRAME_BUSY, TRUE, 0);
        set_print_check(1, 1, Ptmo->check_interval, 0);
	xv_set(ip->window1, FRAME_BUSY, FALSE, 0);
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */
	return;
}

/*
 * Notify callback function for `rm_job_bt'.
 */
/* stop selected print job(s) */
void
rm_job_button(Panel_item item, Event *event) 
{
	printtool_window1_objects *ip = (printtool_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	int  nitems = (int)xv_get(ip->queue_ls, PANEL_LIST_NROWS);
	int n, nremoved = 0;
	char tempstr[10];

	xv_set(ip->window1, FRAME_BUSY, TRUE, 0);

	/*remove selected items from the list. Abort operation if error occurs*/
	for (n = nitems-1; n >= 0; n--) {
	   if (xv_get(ip->queue_ls, PANEL_LIST_SELECTED, n)) {  
	      /* remove_selection return 0 for remove fail */
	      if (!remove_selection(n)) break ; 
	      nremoved++;
	   }
        } /* end for */

	sprintf(tempstr, "%d", nremoved);
	print_footer_msg(NO_BF, dgettext(DOMAIN_NAME, "%s Print Job(s) Removed"), tempstr);
	/* disable the stop printing button if no item is in the list */
	if ((int)xv_get(ip->queue_ls, PANEL_LIST_FIRST_SELECTED) < 0)
	   xv_set(ip->rm_job_bt, PANEL_INACTIVE, TRUE, NULL);
	xv_set(ip->window1, FRAME_BUSY, FALSE, 0);

	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */
}


/*
 * Notify callback function for `apply_bt'.
 */
void
apply_button(Panel_item item, Event *event)
{
	int value;
	printtool_prop_popup_objects *ip = (printtool_prop_popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	/* apply resource to the process */
	/* support landscape 
	Ptmo->landscape = (int)xv_get(ip->sorientn_ch, PANEL_VALUE);
	*/
	Ptmo->header = (int)xv_get(ip->sheader_tg, PANEL_VALUE);
	Ptmo->override_pmethod = (int)xv_get(ip->override_filter_tg, PANEL_VALUE);

	value = (int)xv_get(ip->notify_all_ch, PANEL_VALUE);
	if (value & 01) 
	   Ptmo->notify_beep = TRUE;
        else	
	   Ptmo->notify_beep = FALSE;
	value >>=1;
        if (value & 01)
	   Ptmo->notify_flash = TRUE;
        else	
	   Ptmo->notify_flash = FALSE;

        set_mpanel_values();  /* set main panel resource */

	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */
}

/*
 * Notify callback function for `reset_bt'.
 */
void
reset_button(Panel_item item, Event *event)
{
	printtool_prop_popup_objects *ip = (printtool_prop_popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	/* reset the properties values back to last apply (user) state */
	set_prop_values();	/* Set property items  */
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

}

/*
 * Notify callback function for `printer_ls'.
 */
int
printer_list(Panel_item item, char *string, Xv_opaque client_data, Panel_list_op op, Event *event, int row)
{
	printtool_window1_objects *ip = (printtool_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	switch(op) {
	case PANEL_LIST_OP_DESELECT:
		break;

	case PANEL_LIST_OP_SELECT:
		/* save printtool current printer status */
		writerc();
		break;

	case PANEL_LIST_OP_VALIDATE:
		break;

	case PANEL_LIST_OP_DELETE:
		break;
	}
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

	return XV_OK;
}



/* VARARGS */
static void
print_footer_msg(int bf, char* fmt, char* arg)
{
   char *buffer = NULL;

   if ((buffer = (char*)malloc(strlen(fmt)+strlen(arg)+1)) == NULL) {
       (void)fprintf(stderr, dgettext(DOMAIN_NAME, "%s: Unable to allocate memory"),PROGNAME);
       return;
   }
   (void)sprintf(buffer, fmt, arg);

   if (Printtool_window1->window1)
       xv_set(Printtool_window1->window1, FRAME_LEFT_FOOTER, 
			buffer, 0);
   else if (buffer) 
       (void)fprintf(stderr, dgettext(DOMAIN_NAME, "%s: %s"), PROGNAME,buffer);
   free(buffer);

   if (bf) {  /* the msg may need beep/flash */
      if (Ptmo->notify_beep) {
	 int i;
	 for (i = 0; i < NUM_BEEP; i++)
             ds_beep(Ptmo->dply);
      } /* end if beep */

      if (Ptmo->notify_flash) {
	 int iconize = xv_get(Printtool_window1->window1, FRAME_CLOSED);
	 flash_it(iconize);
      } /* end if flash */
   } /* end bf */
}

get_date_time(char *buf)
{
   char *ptr = NULL;
   int len;
   extern char *strstr();

   /* filter out text at the end, if any */
   if ((ptr = strstr(buf, dgettext(DOMAIN_NAME, " on"))) ||
       (ptr = strstr(buf, dgettext(DOMAIN_NAME, " being filtered"))) ||
       (ptr = strstr(buf, dgettext(DOMAIN_NAME, " filtered"))) ||
       (ptr = strstr(buf, dgettext(DOMAIN_NAME, " finished printing"))) ||
       (ptr = strstr(buf, dgettext(DOMAIN_NAME, " held by admin"))) ||
       (ptr = strstr(buf, dgettext(DOMAIN_NAME, " being held"))) ||
       (ptr = strstr(buf, dgettext(DOMAIN_NAME, " notifying user"))) ||
       (ptr = strstr(buf, dgettext(DOMAIN_NAME, " held for change"))) || 
       (ptr = strstr(buf, " on")) ||  /* check C locale incase it isn't i18n */
       (ptr = strstr(buf, " being filtered")) ||
       (ptr = strstr(buf, " filtered")) ||
       (ptr = strstr(buf, " finished printing")) ||
       (ptr = strstr(buf, " held by admin")) ||
       (ptr = strstr(buf, " being held")) ||
       (ptr = strstr(buf, " notifying user")) ||
       (ptr = strstr(buf, " held for change"))) {
      len = ptr - buf;
      buf[len] = '\0';
   }
}


/* single linked list structure for selected status list */
typedef struct sllist{
      char *member;
      struct sllist *next;
}sllist;

static void    
status_printer()
{
   char *printer;
   char buf[257];
   char buf2[257];
   FILE *fp=NULL;
   sllist *selected_list = (sllist*)NULL;
   void clear_list(), list_insert_end();
   static set_icon_state(int);
   sllist *get_selected_list(), *destroy_list();

   printer = (char*)xv_get(Printtool_window1->printer_ls, PANEL_LIST_STRING, 
                (int)xv_get(Printtool_window1->printer_ls, PANEL_LIST_FIRST_SELECTED));

   sprintf(buf,  "/usr/bin/lpstat \"%s\"" , printer);
   label_icon(printer);
   if (Ptmo->testing) {
      fprintf(stdout, dgettext(DOMAIN_NAME, "%s: test: building status command\n\"%s\"\n"), PROGNAME, buf);
      return;
   }
   if (Ptmo->openmode) {  /* only update the list in open state */
      /* store the selected row if any */
      selected_list = get_selected_list();
      /* make list inactive to avoid list flashing when jobs are listed */
      xv_set(Printtool_window1->queue_ls, PANEL_INACTIVE, TRUE, 0);
      clear_list();
   }

   if (Ptmo->username == NULL) {
      print_footer_msg(BF, dgettext(DOMAIN_NAME, 
		"LOGNAME environment variable not set"), "");
      return;
   }

   if ((fp = popen(buf, "r")) != NULL) {
      int no_entry = 1;
      char *job, *size, *date_time, *second;


      while (fgets(buf, sizeof(buf), fp) != NULL) {
         char file_buf[MAXPATHLEN];
	 int size_len;

	 /* if lpstat returns a valid print job which is not cancelled */
	 /* parse it to display the info in the correct places.        */

	 /* check for both C and other locale in case of the system message
	    has not been localized */
	 if ((strstr(buf, "canceled")) ||  /* job has been cancelled */ 
	     (strstr(buf, dgettext(DOMAIN_NAME, "canceled"))) ||
	     (strstr(buf, "cancelled")) ||
	     (strstr(buf, dgettext(DOMAIN_NAME, "cancelled"))))
	    continue;

	   /* str_contains returns -1 means not found */
	 if (!isroot && (str_contains(buf, Ptmo->username) == -1))
	    continue;

	 /* break out fields to set up nicely */
	 job = (char *) strtok(buf, " ");
	 (char *) strtok(NULL, " ");
	 size = (char *) strtok(NULL, " ");

         /* convert to Kbytes */
	 size_len = strlen(size);
	 if (size_len > 3)
	    size[size_len-3] = '\0';
         else
	    strcpy(size, "1");
	 date_time = (char *) strtok(NULL, "");  /* get the rest */
	 get_date_time(date_time);  /* strip off text at the end, if any */
	 sprintf(buf2, Format, job, size, date_time);
	 if (Ptmo->openmode) {
	    if (selected_list && is_job_selected(&selected_list, job))
	          list_insert_end(buf2, TRUE);
	    else
	       list_insert_end(buf2, FALSE);
	 }
         no_entry = 0;
	 Ptmo->newprinter = FALSE;
      } /* end while */
      /* shown the status list */
      if (Ptmo->openmode) {
         xv_set(Printtool_window1->queue_ls, PANEL_INACTIVE, FALSE, 0);
	 if ((int)xv_get(Printtool_window1->queue_ls, 
			 PANEL_LIST_FIRST_SELECTED) >= 0) 
	    xv_set(Printtool_window1->rm_job_bt, PANEL_INACTIVE, FALSE, NULL);
	 selected_list = destroy_list(selected_list);
      }

      /* if no entry print no entries and reset print check */
      if (no_entry) {
	 set_icon_state(EMPTY);
	 set_print_check(0, 0, 0, 0);
      } /* end if no_entry */
      else {
	 char *tmpbuf = (char*)xv_get(Printtool_window1->window1, 
					FRAME_LEFT_FOOTER);
	 if (tmpbuf && *tmpbuf) {   
            if (strstr(tmpbuf, "print button") == NULL)
	       print_footer_msg(NO_BF, "", ""); /* clear footer */
	 }
	 set_icon_state(FULL);
      } /* end else */
      pclose(fp);
   } /* end if popen */
   return;
}


static
label_icon(char *printer)
{

   char *old_name = NULL;
   Icon edit_icon = (Icon) xv_get(Printtool_window1->window1, FRAME_ICON);

   if (!edit_icon)
       (void)fprintf(stderr, dgettext(DOMAIN_NAME, "%s: unable to get icon in label_icon()"), PROGNAME);

   old_name = (char*)xv_get(edit_icon, XV_LABEL);
   if (!old_name  || old_name && strcoll(printer, old_name) != 0) {
        if (xv_set(edit_icon, XV_LABEL, printer, 0) != 0)
          fprintf(stderr, dgettext(DOMAIN_NAME, "%s: unable to set icon label in label_icon()\n"), PROGNAME );

        /* xv_set actually makes a copy of all the icon fields */
        if (xv_set(Printtool_window1->window1, FRAME_ICON, edit_icon, 0) != 0)
          fprintf(stderr, dgettext(DOMAIN_NAME, "%s: unable to set icon to basewin in label_icon()\n"), PROGNAME);
     }
}


void
clear_list()
{
  if (Printtool_window1->queue_ls) 
     list_flush(Printtool_window1->queue_ls);
  /* disable the stop printting button */
  if (!(int)xv_get(Printtool_window1->rm_job_bt, PANEL_INACTIVE))
      xv_set(Printtool_window1->rm_job_bt, PANEL_INACTIVE, TRUE, NULL);
}


/* set the icon state to reflect the status of the selected printer.  Only 
   beep/flash (if define) once all jobs have been finished in the selected 
   printer. 
*/
static 
set_icon_state(int state)
{
   Server_image new_image, new_image_mask;
   static int current_state = 0;  /* init to no job pending */

   if (!Ptmo->openmode) {	/* in icon state ? */
      if (state == EMPTY) {     /* no job in the selected printer queue */
	 new_image = Ptmo->noprint_icon;  /* set no print icon */
	 new_image_mask = Ptmo->noprint_icon_mask;
      }
      else {				/* has job */
	 new_image = Ptmo->icon;	/* set print icon */
	 new_image_mask = Ptmo->icon_mask;
      }
      set_icon(new_image, new_image_mask); 
      if (state != current_state){
	 /* only beep/flash (if any) when current_state is different */
	 if (state == EMPTY)  {
	    set_LC_ALL(NULL);
            print_footer_msg(BF, dgettext(DOMAIN_NAME, "no entries"), "");
	    set_LC_ALL("C");
	 }
      }
   } /* end if */
   else {	/* in open state */
      /* only beep/flash (if any) once when the job done */
      /* it won't beep/flash (if any) if switches to another printer which has
	 no job in the queue */
      if (state == EMPTY)
         if ((!Ptmo->newprinter) && (state != current_state)) {
	    set_LC_ALL(NULL);
            print_footer_msg(BF, dgettext(DOMAIN_NAME, "no entries"), ""); 
	    set_LC_ALL("C");
	 }
         else {
	     /* printer switch, empty queue */
	    set_LC_ALL(NULL);
            print_footer_msg(NO_BF, dgettext(DOMAIN_NAME, "no entries"), ""); 
	    set_LC_ALL("C");
	    Ptmo->newprinter = FALSE;
	 } /* else newprinter */
   } /* else open mode */
   current_state = state;
}

/* set icon to the frame - only set when the current icon is different */
set_icon(Server_image icon, Server_image icon_mask)
{
   if (xv_get(Ptmo->current_icon, ICON_IMAGE) != icon) {
      (void)xv_set(Ptmo->current_icon,
		ICON_IMAGE, icon,
		ICON_MASK_IMAGE, icon_mask,
		WIN_RETAINED,   TRUE,    /* server repaints icon */
		0);
      (void)xv_set(Printtool_window1->window1, 
		   FRAME_ICON, Ptmo->current_icon, 0);
   } /* end if */
}

/* set the number of seconds to wait for checking lpstat */
static
set_print_check(int init_sec,int init_usec,int interval_sec,int interval_usec)
{
   struct itimerval period;  /* Itimer interval */

   period.it_interval.tv_sec = interval_sec;
   period.it_interval.tv_usec = interval_usec;
   period.it_value.tv_sec = init_sec;
   period.it_value.tv_usec = init_usec;

   notify_set_itimer_func((Notify_client) Printtool_window1->window1, 
         (Notify_func) status_printer, ITIMER_REAL, &period, 
	 (struct itimerval *) 0);
}


static int
remove_selection(int n)
{
   char *entry;
   char *job_id;
   char buf[256];
   FILE *fp;

   entry = (char*)xv_get(Printtool_window1->queue_ls, PANEL_LIST_STRING, n);

   /* get job id and remove with "cancel" command */
   job_id = (char*)strtok(entry, " ");
   sprintf(buf, "/usr/bin/cancel %s", job_id);

   if (Ptmo->testing) {
      fprintf(stdout, dgettext(DOMAIN_NAME, "%s: test: building remove command \n\"%s\"\n"), PROGNAME, buf);
      return 0;
   } /* end if */

   set_LC_ALL("C");
   if ((fp = popen(buf, "r")) != NULL) {
      fgets (buf, sizeof(buf), fp); 

      /* str_contains return -1 for not found, others for found */
      if (str_contains(buf, "cancelled") != -1) {
	 set_LC_ALL(NULL);
	 print_footer_msg(NO_BF, dgettext(DOMAIN_NAME, "%s removed"), job_id);
         set_LC_ALL("C");
	 list_delete_entry_n(Printtool_window1->queue_ls, n);

	 /* EMPTY the pipe */
	 while (fgets(buf, sizeof(buf), fp)) ;  /* ignore the rest */
	 pclose(fp);
	 return 1;  /* cancelled */
      } /* end if str_contains */
      pclose(fp);
      set_LC_ALL(NULL);
    } /* end if */ 
    else if (errno) {
       set_LC_ALL(NULL);
       sprintf(buf, dgettext(DOMAIN_NAME, "Error: could not remove print job %s"), job_id); 
       console_alert (buf);
    }
   return 0;
}


/* Printtool_window1->window1 alert with note to check console */
static void
console_alert(char *msg)  
{
   notice_prompt(Printtool_window1->window1, (Event *) NULL,
   		NOTICE_MESSAGE_STRINGS,  msg,
		dgettext(DOMAIN_NAME, "Check console for error messages"), 0,
		NOTICE_BUTTON, dgettext(DOMAIN_NAME, "OK"), 101,
		0);
}

str_contains(char s1[], char s2[])
{
   register int    i = 0;
   int       notfound = 1;
   int       srchlen;  /* min length needed to search */
   int       len2 = strlen(s2);
       
   if (!s1 || !s2 || !*s1 || !*s2)
      return (-1);
		  
   srchlen = strlen(s1) - len2;
   for (; notfound && i <= srchlen; i++)
      if (s1[i] == s2[0])
         notfound = strncmp(&s1[i], &s2[0], len2);
         if (notfound)
            return(-1);
         else
            return(i-1);
}


/* create all server image for icons and set the frame icon */
void
init_icons()
{
   Server_image make_glyph();   /* make server image */
   static unsigned short image[] = {
   #include  "print.icon"
   };
   static unsigned short image_mask[] = {
   #include  "print_mask.icon"
   };
   static unsigned short noprint_image[] = {
   #include  "noprint.icon"
   };
   static unsigned short noprint_image_mask[] = {
   #include  "noprint_mask.icon"
   };

   Ptmo->icon = make_glyph(image, ICON_DEFAULT_WIDTH, ICON_DEFAULT_HEIGHT);
   Ptmo->icon_mask = make_glyph(image_mask, 
			      ICON_DEFAULT_WIDTH, ICON_DEFAULT_HEIGHT);
   Ptmo->noprint_icon = make_glyph(noprint_image, 
		 	      ICON_DEFAULT_WIDTH, ICON_DEFAULT_HEIGHT);
   Ptmo->noprint_icon_mask = make_glyph(noprint_image_mask,
			      ICON_DEFAULT_WIDTH, ICON_DEFAULT_HEIGHT);
   
   Ptmo->current_icon = xv_create(NULL, ICON,
				ICON_IMAGE,      Ptmo->noprint_icon,
				ICON_MASK_IMAGE, Ptmo->noprint_icon_mask,
				WIN_RETAINED, TRUE,
				NULL);
   /* set the init noprint icon to window1 */
   if (xv_set(Printtool_window1->window1, FRAME_ICON, Ptmo->current_icon,0) != 0)
      fprintf(stderr, dgettext(DOMAIN_NAME, "%s: unable to set icon in init_icons()\n"), PROGNAME);
}

/* create server image */
Server_image
make_glyph(icon, w, h)
short *icon;
int w, h;
{
   Server_image server_image;
    
   server_image = (Server_image) xv_create (NULL, SERVER_IMAGE,
                     XV_WIDTH,       w,
                     XV_HEIGHT,      h,
                     SERVER_IMAGE_BITS, icon,
                     NULL);
   return (server_image);
}


void
init_printtool()
{
   char label_str[1024];
   extern char *ds_relname();

   /* add window label */
   strcpy(label_str, dgettext(DOMAIN_NAME, "Print Tool "));
   strcat(label_str, ds_relname());
   xv_set(Printtool_window1->window1, XV_LABEL, label_str, NULL);
   Ptmo = (Printtool_Mobject*)(calloc(1, sizeof(Printtool_Mobject))) ;
   if (!Ptmo)
      fprintf(stderr, dgettext(DOMAIN_NAME, "%s: Unable to allocate memory\n"),PROGNAME);
   /* make a temporary file to store error messages */

   strcpy(Ptmo->errfile,  "/tmp/PTXXXXXX" ) ;
   mktemp(Ptmo->errfile) ;

   Ptmo->username = NULL;
   Ptmo->dply = (Display*) xv_get(Printtool_window1->window1, XV_DISPLAY);
   default_prop_values();
}


default_prop_values()
{
   /* set properties items to default values */
   /* support for landscape 
   Ptmo->landscape = FALSE;  // portrait 
   */
   Ptmo->header = TRUE;  /* with header page */
   Ptmo->notify_beep = FALSE;  /* no beep */
   Ptmo->notify_flash = FALSE; /* no flash */
   Ptmo->override_pmethod = FALSE; /* use default print method */
}

/*
 *  Find/create fonts for text in the scrolling list and printtool icon.
 */
void
init_fonts()
{
   int		font_size;
   Xv_font	font;

   font = (Xv_Font)xv_get(Printtool_window1->window1, XV_FONT);
   if (!font) {
      fprintf(stderr,  dgettext(DOMAIN_NAME, "%s: Cannot get frame font\n"), PROGNAME);
      exit(1);
   }
   /* get the font size in order to inherit -scale fonts */

   font_size = xv_get(font, FONT_SIZE);
   Ptmo->fixedfont = (Xv_Font) xv_find(Printtool_window1->window1, FONT,
			FONT_FAMILY, FONT_FAMILY_DEFAULT_FIXEDWIDTH,
			FONT_SIZE,   font_size,
			0);
   if (!Ptmo->fixedfont) {
       fprintf(stderr, dgettext(DOMAIN_NAME, "%s: Cannot get fixed font %s, size %d\n"),
	     PROGNAME, FONT_FAMILY_DEFAULT_FIXEDWIDTH, font_size) ;
	     Ptmo->fixedfont = font ;
   }

   Ptmo->fixedttfont = (Xv_Font) xv_find(Printtool_window1->window1, FONT,
       			FONT_FAMILY, FONT_FAMILY_DEFAULT_FIXEDWIDTH,
	   		FONT_STYLE,  FONT_STYLE_BOLD,
	       		FONT_SIZE,   font_size,
		   	0) ;
   if (!Ptmo->fixedttfont) {
      fprintf(stderr, dgettext(DOMAIN_NAME, "%s: Cannot get bold fixed font %s, size %d\n"), 
	      PROGNAME, FONT_FAMILY_DEFAULT_FIXEDWIDTH, font_size);
      Ptmo->fixedttfont = font;
   }
}


get_options(int argc, char**argv)
{
   char *print_method;
   argc--;			/* Skip program name. */
   argv++;

   while (*argv && argc-- > 0) {
      if (CMD_ARG("-v") || CMD_ARG("-?") || CMD_ARG("-h"))
	 usage();
      else if (CMD_ARG("-test"))
	 Ptmo->testing = TRUE;
      else if (CMD_ARG("-P")) {
	 if (get_cmdstr(&Ptmo->printer, argv) == FALSE)
	    usage();
         argc--;
	 argv++;
      } /* if -P */
      else if (CMD_ARG("-f")) {
/*  No longer documented/supported.
	 if (get_cmdstr(&print_method, argv) == FALSE) 
	     usage() ;
	 else
	    strcpy(Ptmo->print_method, print_method);
*/
	 argc-- ;
	 argv++ ;
      }
      else {
	 int i;
	 if ((i = xv_parse_one(PROGNAME, argc, argv)) == -1) {
	    (void) xv_usage(PROGNAME) ;
	    exit(1);
	 } /* end if */
	 else if (i == 0) 
	      usage();
	 argc -= (i - 1);
	 argv += (i - 1);
      } /* end else */
      argv++;
   } /* end while */
}

usage()
{
   extern char *xv_version ;     /* library version ptr declared by XV */
   extern char *ds_relname();       /* release number of openwin */
   fprintf(stderr, dgettext(DOMAIN_NAME, "\n%s: version %s running on %s\n"), 
		  PROGNAME, ds_relname(), xv_version);
   fprintf(stderr, dgettext(DOMAIN_NAME, 
	  "Usage: %s [-%c printer] [-%c] [-%c]\n"), 
		  PROGNAME, 'P', 'v', '?');
   exit(0);
}

get_cmdstr(char **str, char **argv)
{
   if (*str != NULL) (void)free(*str) ;
   *str = NULL ;
   argv++ ;
   if (*argv == NULL || CMD_ARG( "-" )) return(FALSE) ;
   else
     { 
       *str = (char *) malloc((unsigned) (strlen(*argv) + 1)) ;
       (void)strcpy(*str, *argv) ;
     }
   return(TRUE) ;
}

/* get the list of installed printer and add to the printer scrolling list.
 * The default printer is set to the user's cmd-line printer, PRINTER,
 * LPDEST, lp/default, in that order, if they exist.  The default printer 
 * will be the 1st entry of the list.
 */
get_printer_list()
{
   FILE *fp;

   if (!Ptmo->printer)
      Ptmo->printer = (char *)getenv("PRINTER");

   if (!Ptmo->printer)
      Ptmo->printer = (char *)getenv("LPDEST");

   if (!Ptmo->printer) { 
      char * ptr;
      /* get the default printer from lpstat -d command */
      /* the output looks like "system default destination: spitfire\n" */
      set_LC_ALL("C");
      fp = popen("lpstat -d", "r");
      if (fp) {
	 char message [MAXPATHLEN];
	 fgets(message, sizeof(message), fp);
	 if ((ptr = (char*)strchr(message, ':')) != NULL) {
	    strtok(ptr, " ");
	    Ptmo->printer = strdup ( (char *) strtok ( (char *) NULL, "\n") );
	 }
         pclose(fp);
      } /* end if fp */
      set_LC_ALL(NULL);
   } /* end if printer */
   get_sort_printers();
}


typedef struct tnode{
      char *printer;
      struct tnode *left, *right;
}tnode;

get_sort_printers()
{

   FILE *fp;
   int count = 0;
   int cmd_printer_exist = 0;
   tnode *root = NULL, *tree();
   char  *getenv();
   char buf[256];
   char *printer;

   set_LC_ALL("C");
   if ((fp = popen("lpstat -p all", "r")) == NULL) {
      set_LC_ALL(NULL);
      fprintf(stderr, dgettext(DOMAIN_NAME, "%s: Unable to issue \"%s\"\n"), PROGNAME, "lpstat -p all");
      exit(1);
   }

   /* get printers names by reading output of "lpstat -p all" */

   while(fgets(buf, sizeof(buf), fp) != NULL) {
      char *buf_ptr;
#if 0
      if ((buf_ptr = strstr(buf, "printer")) != NULL) {
#endif 
	 /*
       *  BUGID # 1145912;
       *  Looking for just "printer" in the output of lpstat is not sufficient.
       *  Error message containing that word will also have be considered a
       *  valid printer.  Instead, the string containing the printer name will
       *  always start with "printer", so rely on that.
       */
      buf_ptr = buf;
      if ((strncmp(buf, "printer", 7)) == 0) {
	 strtok(buf_ptr, " ");
	 printer = (char *) strtok(NULL, " ");
	 if (printer) {
	      if (!cmd_printer_exist && (Ptmo->printer) && 
			  (strcoll(Ptmo->printer, printer) == 0)) {
                  cmd_printer_exist = 1;
		  continue;
              }
	      root = tree(root, printer, strcmp);
	      count++;
	 }
      }  /* end if strstr */
   } /* end while */
   if (!cmd_printer_exist && (Ptmo->printer)) {
      set_LC_ALL(NULL);
      fprintf(stderr, dgettext(DOMAIN_NAME, "%s: %s not found by \"%s\"\n"),
      PROGNAME, Ptmo->printer, "lpstat -p all");
      Ptmo->printer = NULL;  /* printer not registered in printer service */
   }
   if ((!Ptmo->printer) && count == 0) {
      char msg[1024];
      set_LC_ALL(NULL);
      sprintf(msg, dgettext(DOMAIN_NAME, "%s: No printer has been installed for this system.\nPlease consult your system administrator.\n"), PROGNAME);
      notice_prompt(Printtool_window1->window1, (Event *) NULL,
                     NOTICE_MESSAGE_STRINGS,  msg, 0,
                     NOTICE_BUTTON, dgettext(DOMAIN_NAME, "OK"), 101,
                     0);
      exit(1);
   } 
   if (cmd_printer_exist)
      xv_set(Printtool_window1->printer_ls,
             PANEL_LIST_STRING,  0, Ptmo->printer,
	     0) ;
       
   count  = (int)xv_get(Printtool_window1->printer_ls, PANEL_LIST_NROWS);
   treelist(root, &count);
   pclose(fp);
   set_LC_ALL(NULL);
}

tnode *
tree(tnode *p, char *printer, int (*compare_func)())
{
   char *cp, *strcpy();
   int cond;

   if (p == NULL) {
      p = (tnode*)malloc(sizeof(struct tnode));
      if ((cp = malloc(strlen(printer)+1)) != NULL)
	 strcpy(cp, printer);
      p->printer = cp;
      p->left = p->right = NULL;
   }
   else if ((cond = (*compare_func)(printer, p->printer)) == 0)
        ;
   else if (cond < 0)
      p->left = tree(p->left, printer, compare_func);
   else
      p->right = tree(p->right, printer, compare_func);

   return(p);
}


treelist(tnode *p, int* i)
{
   if (p != NULL) {
      treelist(p->left, i);
      xv_set(Printtool_window1->printer_ls,
             PANEL_LIST_STRING,  (*i)++, p->printer,
	     0) ;
      treelist(p->right, i);
   }
}


static
writerc()
{
   static char buf[256];
   char *printer;

   /* on change of printer, set timer to check any job on the new printer. */
   Ptmo->newprinter = TRUE;  /* prevents beeping if no entry */
   set_print_check(1, 1, Ptmo->check_interval, 0);

   /* on change of printer, update icon label */
   printer = (char *)xv_get(Printtool_window1->printer_ls, PANEL_LIST_STRING,
                            (int)xv_get(Printtool_window1->printer_ls,
                            PANEL_LIST_FIRST_SELECTED));
   label_icon(printer);

   /* save the current selected printer for next invocation */
   sprintf(buf, " -P \"%s\"" , printer);
   if (buf[0]) 
      xv_set(Printtool_window1->window1, WIN_CMD_LINE, buf, 0);
}


/* set fixed width font for scrolling list title */
void
set_title()     
{
   char buf[128];
   char str[MAXPATHLEN];
   char *str_type[20];
   Font_string_dims dims;
   int jt_pix, st_pix, bt_pix, a_pix, n_pix, j_pix, s_pix, b_pix;
   int nob_jobt = 0, nob_sizet = 0, nob_job = 0, nob_size = 0;
   int message_x, tt_pix, t_pix, total_width;

   XrmValue value;
   /* get OpenWindows.ScrollbarPlacement resource */
   if (XrmGetResource(Ptmo->rDB, "openWindows.scrollbarPlacement",
            "OpenWindows.ScrollbarPlacement", str_type, &value) != NULL) {
      if (strcmp(value.addr, "left") == 0) {
         Scrollbar sb;
         sb = (Scrollbar)xv_get(Printtool_window1->queue_ls, PANEL_LIST_SCROLLBAR) ;
         xv_set(Printtool_window1->qlist_title, 
            XV_X,  (int)xv_get(Printtool_window1->queue_ls, XV_X) + (int)xv_get(sb, XV_WIDTH), 
            0);				
      } /* end if strcmp */
   } /* end if Xrm */
   
/* find the width of characters in different locale for title */
   (void)xv_get(Ptmo->fixedttfont, FONT_STRING_DIMS, 
                 dgettext(DOMAIN_NAME, "Job"), &dims);
   jt_pix = dims.width; /* number of pixel for title job */
   (void)xv_get(Ptmo->fixedttfont, FONT_STRING_DIMS, 
                 dgettext(DOMAIN_NAME, "Size(K)"), &dims);
   st_pix = dims.width; /* number of pixel for title size */
   (void)xv_get(Ptmo->fixedttfont, FONT_STRING_DIMS, 
                 dgettext(DOMAIN_NAME, "Time"), &dims);
   tt_pix = dims.width; /* number of pixel for title size */
   (void)xv_get(Ptmo->fixedttfont, FONT_STRING_DIMS, 
                 dgettext(DOMAIN_NAME, " "), &dims);
   bt_pix = dims.width; /* number of pixel for a title blank */ 

/* find the width of characters in C locale for scroll item, since the
   job number and size is 8 bits */
   /* number of pixel for letter */
   a_pix = (int)xv_get(Ptmo->fixedfont, FONT_CHAR_WIDTH, 'Z');
   /* number of pixel for a digit */
   n_pix = (int)xv_get(Ptmo->fixedfont, FONT_CHAR_WIDTH, '3');
   /* number of pixel for a blank */
   b_pix = (int)xv_get(Ptmo->fixedfont, FONT_CHAR_WIDTH, ' ');

   j_pix = a_pix * 20;  /* at most 20 characters for job number */
   s_pix = n_pix * 8;   /* at most 8 digits for size */

   /* 1 leading blanks and 1 trail title blank for job title */
   jt_pix = jt_pix + bt_pix + b_pix;
   j_pix = j_pix + b_pix;  /* item job and 1 trail blank */

   if (jt_pix > j_pix)  /* use title col for scroll job */
      nob_job = (jt_pix - j_pix)/b_pix; /* #of blank append in item */
   else  /* use scroll item col for job title */
      nob_jobt = (j_pix - jt_pix)/bt_pix; /* #of t. blank append in title */
   
   st_pix = st_pix + bt_pix;
   s_pix = s_pix + b_pix;
   if (st_pix > s_pix)     /* use title col for scroll size */
      nob_size = (st_pix - s_pix)/b_pix;  /* #of blank append in item */
   else  /* use scroll item col for title size */
      nob_sizet = (s_pix - st_pix)/bt_pix; /* #of t. blank append in title */
   
   /* construct display format for scroll item */
   sprintf(Format, "%s%2d%c%2d%s%1d%s",
	     "%-", (j_pix + nob_job * b_pix)/a_pix, 
	     '.', (j_pix + nob_job * b_pix)/a_pix, 
	     "s%-", (s_pix + nob_size * b_pix)/n_pix,
	     "s%-10s"); 

   message_x = (int)xv_get(Printtool_window1->qlist_title, XV_X); 
   xv_set(Printtool_window1->qlist_title, 
		XV_X,		    message_x + b_pix,
		PANEL_LABEL_FONT,   Ptmo->fixedttfont,
		PANEL_LABEL_STRING, dgettext(DOMAIN_NAME, "Job"), NULL);

   xv_set(Printtool_window1->size_title, 
		XV_X,   message_x + jt_pix + nob_jobt * bt_pix + b_pix,
		PANEL_LABEL_FONT,   Ptmo->fixedttfont,
		PANEL_LABEL_STRING, dgettext(DOMAIN_NAME, "Size(K)"), NULL);

   xv_set(Printtool_window1->time_title, 
		XV_X, message_x + jt_pix + nob_jobt * bt_pix
		      + st_pix + nob_sizet * bt_pix + 2*b_pix,
		PANEL_LABEL_FONT,   Ptmo->fixedttfont,
		PANEL_LABEL_STRING, dgettext(DOMAIN_NAME, " Time"), NULL);
  t_pix = a_pix * 15; /* assume 15 characters for time */
  total_width = message_x + jt_pix + nob_jobt * bt_pix + st_pix + 
		nob_sizet * bt_pix + 2*b_pix + 
		((tt_pix > t_pix) ? tt_pix : t_pix) + 30;
  if (total_width > (int)xv_get(Printtool_window1->controls1, XV_WIDTH))
     xv_set(Printtool_window1->window1, XV_WIDTH, total_width, NULL);
}

void
list_insert_end(char *text, int selected)   /* add text to end of list */
{  
   /* number of list items = end of list */ 
   int count = (int)xv_get(Printtool_window1->queue_ls, PANEL_LIST_NROWS) ;

   if (selected)
      xv_set(Printtool_window1->queue_ls,
		PANEL_LIST_INSERT,  count,
		PANEL_LIST_STRING,  count, text,
		PANEL_LIST_SELECT,  count, TRUE,
		PANEL_LIST_FONT,    count, Ptmo->fixedfont,
		0);
   else
      xv_set(Printtool_window1->queue_ls,
		PANEL_LIST_INSERT,  count,
		PANEL_LIST_STRING,  count, text,
		PANEL_LIST_FONT,    count, Ptmo->fixedfont,
		0);
}

/*
 * Event callback function for `window1'.
 */
static Notify_value
frame_event_proc(Xv_window win, Event *event, Notify_arg arg, Notify_event_type type)
{
        static void resize_scrolling_list();
        static void resize_right_buttons();
        static void resize_printer_list();
	printtool_window1_objects *ip = (printtool_window1_objects *) xv_get(win, XV_KEY_DATA, INSTANCE);
	
        switch (event_action(event)) {
           case ACTION_OPEN:
              Ptmo->openmode = TRUE;
              status_printer();
              break;

           case ACTION_CLOSE:
              Ptmo->openmode = FALSE;
              status_printer();
              break;

           /* Resize the various panel items since XView does not do so */
           case WIN_RESIZE:
	      xv_set(Printtool_window1->controls1,
			  XV_WIDTH, WIN_EXTEND_TO_EDGE,
			  XV_HEIGHT, WIN_EXTEND_TO_EDGE,
			  NULL);
              if (Printtool_window1->controls1) {
                ds_resize_text_item(Printtool_window1->controls1, Printtool_window1->filter_tx);
                resize_scrolling_list();

                ds_center_items(Printtool_window1->controls1, -1,
		                Printtool_window1->queue_bt, 
		                Printtool_window1->rm_job_bt, 0);
                resize_right_buttons();		
                ds_resize_left_text_item(Printtool_window1->controls1, 
	              Printtool_window1->filename_tx, 
		      (int)xv_get(Printtool_window1->info_bt, XV_X) + 40);
		resize_printer_list();
              }
              break;
        } /* end switch */

	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

	return notify_next_event_func(win, (Notify_event) event, arg, type);
}

/*
 * Event callback function for `controls1'.
 */
static Notify_value
panel_event_proc(Xv_window win, Event *event, Notify_arg arg, Notify_event_type type)
{
	printtool_window1_objects *ip = (printtool_window1_objects *) xv_get(win, XV_KEY_DATA, INSTANCE);
	
	switch (event_action(event)) {
	   case ACTION_OPEN:
	      set_print_check(0, 500, 0, 0);
              break;
	   case ACTION_PROPS:
	      if (event_is_down( event ) ) {
		  if ( !Printtool_prop_popup ) {
		      Printtool_prop_popup 
			  = printtool_prop_popup_objects_initialize(NULL, Printtool_window1->window1);
		      set_prop_values();
		      set_props_help();
		  }

		  xv_set(Printtool_prop_popup->prop_popup, FRAME_CMD_PUSHPIN_IN, TRUE, NULL);
		  xv_set(Printtool_prop_popup->prop_popup, XV_SHOW, TRUE, NULL);
              }
	      break;
	} /* end switch */

	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

	return notify_next_event_func(win, (Notify_event) event, arg, type);
}

static Notify_value
quit_proc(Xv_opaque frame, Destroy_status status)
{
   switch(status) {
      case DESTROY_SAVE_YOURSELF: /* save current state */
	 /* do nothing */
         break;
      case DESTROY_CHECKING:       /* use to veto a termination request */
      case DESTROY_PROCESS_DEATH:         /* really dying, so save state info */
      case DESTROY_CLEANUP:               /* terminate request; clean up self */
	 /* erase the temporary error file */
	 if (Ptmo && Ptmo->errfile)
	   (void)unlink(Ptmo->errfile);

	 /* this is the kludge for fixing the xview warning when quit the
	 tool with properties has been touch */
	 exit(0); 
         break;
      default: {
	 char tempstr[10];
	 sprintf(tempstr, "%d", status);
	 /* an unknown termination request has been sent */
	 print_footer_msg(NO_BF, 
	     dgettext(DOMAIN_NAME,"Unknown termination status %s"),
			 tempstr);
      }
   } /* end switch */
   return(notify_next_destroy_func(frame, status));
}

/*
 *  Resize the width of printer scrolling list. 
 */
static void
resize_printer_list()
{
   int wid;
   Scrollbar sb;

   sb = (Scrollbar)xv_get(Printtool_window1->printer_ls, PANEL_LIST_SCROLLBAR) ;
   wid = (int)xv_get(Printtool_window1->info_bt, XV_X) -
         (int)xv_get(Printtool_window1->printer_ls, PANEL_VALUE_X) - 
	 (int)xv_get(sb, XV_WIDTH) - 		/* scrollbar width */
	 10;					/* distance between info but */

   if (wid <= 0) 
      wid = 1;
 
   xv_set(Printtool_window1->printer_ls,
          PANEL_LIST_WIDTH,  wid,
          0);
}

/*
 *  Resize the scrolling list to the panel.  This is a KLUDGE since XView
 *  does not do this automatically.
 */
static void
resize_scrolling_list()
{
   int rows, wid;
   int sb_width;		/* scrollbar width */
   Scrollbar sb;
   int xv_height, button_y, button_yn = 0, button_ht, b_margin;
   int list_y;

   sb = (Scrollbar)xv_get(Printtool_window1->queue_ls, PANEL_LIST_SCROLLBAR) ;
   sb_width = (int)xv_get(sb, XV_WIDTH) ;
   list_y = (int)xv_get(Printtool_window1->queue_ls, XV_Y);
   xv_height = (int)xv_get(Printtool_window1->window1, XV_HEIGHT);

   button_y = (int)xv_get(Printtool_window1->group10, XV_Y);/* status button */
   button_ht = (int)xv_get(Printtool_window1->group10, XV_HEIGHT);
   b_margin = xv_height - button_y - button_ht;
   if ((b_margin > 15) || (b_margin <=0))  {
      button_yn = xv_height - button_ht - 15;   /* new Y position of button */
      if (button_yn < list_y + 50)   /* ~ 1 row width + at least 20 spaces */
	 button_yn = 0;
   }
   if (button_yn) {
      button_y = button_yn;
      xv_set(Printtool_window1->group10, XV_Y, button_y, 0);
   }

   rows  = xv_height - list_y -
           (int)xv_get(Printtool_window1->queue_ls, XV_X) - 	/* border */
	   (xv_height - button_y + 40);  /* distance between list and button */

   rows /= (int)xv_get(Printtool_window1->queue_ls, PANEL_LIST_ROW_HEIGHT);
   if (rows <= 0) 
      rows = 1;

   wid = (int)xv_get(Printtool_window1->controls1, XV_WIDTH) -
         (int)xv_get(Printtool_window1->queue_ls, XV_X) - sb_width - 5;
   if (wid <= 0) 
      wid = 1;
 
   xv_set(Printtool_window1->queue_ls,
          PANEL_LIST_DISPLAY_ROWS,  rows,
          PANEL_LIST_WIDTH,         wid,
          0);
}

static void
resize_right_buttons()		
{
  int gp4_x, info_rx, info_x, frame_w, dist_adj;
  int adj_x;

  gp4_x = (int)xv_get(Printtool_window1->g1_prob_g2, XV_X);
  info_x = (int)xv_get(Printtool_window1->info_bt, XV_X);
  info_rx = info_x + (int)xv_get(Printtool_window1->info_bt, XV_WIDTH);
  frame_w = (int)xv_get(Printtool_window1->window1, XV_WIDTH);
  dist_adj = frame_w - info_rx - RIGHT_MARGIN;
  if ((gp4_x + dist_adj) >= Prop_bt_minx) {
     xv_set(Printtool_window1->g1_prob_g2, XV_X, (gp4_x + dist_adj), 0);
     xv_set(Printtool_window1->info_bt, XV_X, (info_x + dist_adj), 0);
  }
  else {   /* prevent buttons run over those panel on their left */
     dist_adj = gp4_x - Prop_bt_minx;
     xv_set(Printtool_window1->g1_prob_g2, XV_X, (gp4_x - dist_adj), 0);
     xv_set(Printtool_window1->info_bt, XV_X, (info_x - dist_adj), 0);
  }
}

/* drop target, print and properties buttons are grouped to right justify.
 * They are not aligned with information button in some language or
 * in different scale.  This function makes them align in right edges.
 */
right_justify()
{
  int info_rx, dnd_rx, gp4_x;
  int diff;
  int frame_w;
  int margin = RIGHT_MARGIN;

  info_rx = (int)xv_get(Printtool_window1->info_bt, XV_X) +
            (int)xv_get(Printtool_window1->info_bt, XV_WIDTH);

  dnd_rx = (int)xv_get(Printtool_window1->drop_target1, XV_X) +
	   (int)xv_get(Printtool_window1->drop_target1, XV_WIDTH);

  gp4_x = (int)xv_get(Printtool_window1->g1_prob_g2, XV_X);

  diff = info_rx - dnd_rx;
  if (diff == 0) {
     Prop_bt_minx = gp4_x;   /* save min x for prop button */
     return;
  }
  xv_set(Printtool_window1->g1_prob_g2, XV_X, (gp4_x + diff), 0);

  Prop_bt_minx = gp4_x + diff;   /* save min x for prop button */
  /* reduce the frame size if the frame is more than "margin" points away from 
     buttons in right side */
  frame_w = (int)xv_get(Printtool_window1->window1, XV_WIDTH);

  if ((frame_w - info_rx) > margin) {
     xv_set(Printtool_window1->window1, XV_WIDTH, (info_rx + margin), 0);
     xv_set(Printtool_window1->controls1, XV_WIDTH, (info_rx + margin), 0);
  }
}

static char*
init_ce()
{
   extern CE_NAMESPACE ce_get_namespace_id();
   extern CE_ATTRIBUTE ce_get_attribute_id();
   int errno = 0;

   /* Establish a connection to the Classing Engine (CE) */
   if ((errno = (int)ce_begin(NULL)) != 0) {
      if (errno == CE_ERR_WRONG_DATABASE_VERSION)
         return(dgettext(DOMAIN_NAME, "Invalid CE database version"));
      else if (errno == CE_ERR_ERROR_READING_DB)
	 return(dgettext(DOMAIN_NAME, "Unable to connect with the CE database"));
   } /* end if */

   /* Get a handle to the file & type namespace */
   Ptmo->file_ns = ce_get_namespace_id("Files");
   if (!Ptmo->file_ns)
      return(dgettext(DOMAIN_NAME, "Unable to find the File namespace"));

   Ptmo->type_ns = ce_get_namespace_id("Types");
   if (!Ptmo->type_ns)
      return(dgettext(DOMAIN_NAME, "Unable to find the Type namespace"));

   /*  Get a handle to the type id attribute in the file namespace  */
   Ptmo->file_type = ce_get_attribute_id(Ptmo->file_ns, "FNS_TYPE");
   if (!Ptmo->file_type)
     return(dgettext(DOMAIN_NAME, "Unable to find the fns type attribute"));
   
   /* Get a handle to the print method attribute in the type namespace */
   Ptmo->type_print = ce_get_attribute_id( Ptmo->type_ns, "TYPE_PRINT");

   if (!Ptmo->type_print)
     return(dgettext(DOMAIN_NAME, "Unable to find the tns print attribute\n"));

   Ptmo->type_name = ce_get_attribute_id( Ptmo->type_ns, "TYPE_NAME");
   if (!Ptmo->type_name)
     return(dgettext(DOMAIN_NAME, "Unable to find the tns name attribute\n"));

   return(NULL) ;
}


/* Get file type entry from ce which assoc with file
 * name - actual path to the file to type
 * label - auxilliary label to use as typing info
 * print_method - returned print method
 * typename - returned type_name
 */
static char *
get_file_type_attrs(char *name, char* label, char** print_method, char** typename)
{
   char buf[512];
   char *fname = NULL;	/* filename w/o path component */
   char *type = NULL;   /* type id to type namespace */
   int bufsize;
   int fd;
   extern char *ce_get_attribute();
   CE_ENTRY entry;

   if (!Ptmo->ce_running) {
      char err_str[256];
      char *err_msg = init_ce();  /* setup the classing engine connection */
      if (err_msg) {
	 sprintf(err_str, dgettext(DOMAIN_NAME, "Classing Engine Error: %s"),
		 err_msg);
	 notice_prompt(Printtool_window1->window1, (Event *) NULL,
		NOTICE_MESSAGE_STRINGS, err_str, "",
		dgettext(DOMAIN_NAME, "Using printtool's default print method"),
		NULL,
		NOTICE_BUTTON, dgettext(DOMAIN_NAME, "OK"), 101,
		0);
        Ptmo->ce_running = FALSE;
	return NULL;
      } /* end if err_msg */
      else
	 Ptmo->ce_running = TRUE;
   } /* end if ce_running */

   /* if no data label, then get the last component of the filename */
   if (!label) {
      if ((fname = strrchr(name, '/')) != NULL)
	 fname++;
      else
	 fname = name;
   }
   else
      fname = label;
   
   /*
    *  Open file and get first 512 bytes.  Used by the file namespace manager
    *  to identify the type of the file in the file namespace.
    */

   if ((fd = open(name, 0)) != -1) {
      bufsize = read(fd, buf, sizeof(buf));
      if (bufsize == -1)
	 bufsize = 0;
      close(fd);
   } /* end if open */
   else {
      char err_str[256];
      sprintf(err_str, dgettext(DOMAIN_NAME, "Cannot open file [%s]"), name);
      notice_prompt(Printtool_window1->window1, (Event *) NULL,
		NOTICE_MESSAGE_STRINGS, err_str, NULL,
		NOTICE_BUTTON, dgettext(DOMAIN_NAME, "OK"), 101,
		0);
      return NULL;
   } /* else open */

   /*
    *  Find the file in the file namespace.  If its not there, then
    *  there is no entry for that file in the CE.
    */
   entry = (CE_ENTRY) ce_get_entry( Ptmo->file_ns, 3, fname, buf, bufsize);
   if (entry)  {
   /*
    *  Get the type id attached to the file entry to later find the file's
    *  attributes (stored in the type namespace).
    */
      type = ce_get_attribute(Ptmo->file_ns, entry, Ptmo->file_type);
   }
   else type = DEFAULT_DOC; /* if no file ns specify, set it as default doc */

   if (!type)
      return NULL;
   
   /* Find the file's type entry */
   entry = (CE_ENTRY) ce_get_entry(Ptmo->type_ns, 1, type);

   /*
    *  No error should occur at this point, since all file namespace
    *  entries should have an associated type namespace entry.  An
    *  error here points to an inconsistancy with the CE databases.
    *   
    *  However, since the database is probably screwed, just return
    *  NULL if an error occurs.
    */
   if (!entry)
      return NULL;

   /* Find the file's type entry */
   *typename = (char *) ce_get_attribute(Ptmo->type_ns, entry, Ptmo->type_name) ;
   /* Find the print method entry */
   *print_method = ce_get_attribute( Ptmo->type_ns, entry, Ptmo->type_print);
}


load_resources()        /* Load combined X resources databases. */
{
   Ptmo->rDB = ds_load_resources(Ptmo->dply);
}

/* Load deskset defaults */
load_deskset_defs()
{
  extern XrmDatabase ds_load_deskset_defs();

  if (Ptmo->desksetDB)
     XrmDestroyDatabase(Ptmo->desksetDB) ;
  Ptmo->desksetDB = ds_load_deskset_defs();
}


/* Read all possible resources from database. */
void
read_resources() 
{
   int boolval;
   char str[MAXPATHLEN];

   /* support for landscape
   if (get_bool_resource("landscape", &boolval))
      Ptmo->landscape = boolval;
   */
      
   if (get_bool_resource("headerPage", &boolval))
      Ptmo->header = boolval;
      
   if (get_bool_resource("notifyBeep", &boolval))
      Ptmo->notify_beep = boolval;
      
   if (get_bool_resource("notifyFlash", &boolval))
      Ptmo->notify_flash = boolval;
      
   if (get_bool_resource("printMethodOverride", &boolval))
      Ptmo->override_pmethod = boolval;

   if (get_str_resource("lastPrintMethod", str))
      strcpy(Ptmo->print_method, str);
   else
      *Ptmo->print_method = NULL;

   if (get_str_resource("checkInterval", str))
      Ptmo->check_interval = atoi(str);
   else
      Ptmo->check_interval = INTERVAL;
}


/* Get boolean resource from the server. */
int get_bool_resource(char* res, int* boolval)
{
   char *val, tempstr[80];
   int len, n;
   extern char* ds_get_resource();

   if ((val = ds_get_resource(Ptmo->rDB, PROGNAME, res)) == NULL)
      return 0;
   strcpy(tempstr, val);
   len = strlen(tempstr);
   for (n = 0; n < len; n++)
      if (isupper(tempstr[n])) tempstr[n] = tolower(tempstr[n]);
   if (strncmp(tempstr, "true", strlen("true")) == 0)
      *boolval = TRUE;
   else
      *boolval = FALSE;
   return 1; 
}

get_str_resource(char* res, char* strval)
{
   char *val;
   extern char* ds_get_resource();

   if ((val = ds_get_resource(Ptmo->rDB, PROGNAME, res)) == NULL)
      return 0;
   else
      strcpy(strval, val);
   return 1;
}

init_prop_values()
{
   set_mpanel_values();  /* set main panel resource */
}

set_mpanel_values()
{
   int show;

   /* support for landscape
   xv_set(Printtool_window1->morientn_ch, PANEL_VALUE, Ptmo->landscape, 0);
   */
   xv_set(Printtool_window1->mheader_tg, PANEL_VALUE, Ptmo->header, 0);
	
   /* show the print method text field if needs */
   show = (int) xv_get(Printtool_window1->filter_tx, XV_SHOW); 
   if (Ptmo->override_pmethod && !show) { 
      if (*Ptmo->print_method) 
         xv_set(Printtool_window1->filter_tx, XV_SHOW, TRUE, 
		PANEL_VALUE, Ptmo->print_method, NULL);
      else
         xv_set(Printtool_window1->filter_tx, XV_SHOW, TRUE, NULL);
   }
   else if (!Ptmo->override_pmethod && show)
      xv_set(Printtool_window1->filter_tx, XV_SHOW, FALSE, NULL);
}

void
set_prop_values()
{
   int value;

   /* support for landscape 
   xv_set(Printtool_prop_popup->sorientn_ch, PANEL_VALUE, Ptmo->landscape, 0);
   */
   xv_set(Printtool_prop_popup->sheader_tg, PANEL_VALUE, Ptmo->header, 0);
   xv_set(Printtool_prop_popup->override_filter_tg, 
	  PANEL_VALUE, Ptmo->override_pmethod, 0);
   
   if (Ptmo->notify_flash)
      value = 1;
   else
      value = 0;
   value <<= 1;   /* left shift */
   if (Ptmo->notify_beep)
      value += 1;
   else
      value += 0;
   xv_set(Printtool_prop_popup->notify_all_ch, PANEL_VALUE, value, 0);
}


void
write_resources()
{
   char *print_method = NULL;

   load_deskset_defs();
   /* support for landscape 
   put_resource("landscape", set_bool(Ptmo->landscape));
   */
   put_resource("headerPage", set_bool(Ptmo->header));
   put_resource("notifyBeep", set_bool(Ptmo->notify_beep));
   put_resource("notifyFlash", set_bool(Ptmo->notify_flash));
   put_resource("printMethodOverride", set_bool(Ptmo->override_pmethod));
   print_method = (char *) xv_get(Printtool_window1->filter_tx, PANEL_VALUE);
   if (print_method) {
      put_resource("lastPrintMethod", print_method);
      strcpy(Ptmo->print_method, print_method);
   }
   ds_save_resources(Ptmo->desksetDB);
}

put_resource(char* type, char *value)
{
   ds_put_resource(&Ptmo->desksetDB, PROGNAME, type, value);
}

char*
set_bool(int value)
{
   return((value) ? "true" : "false");
}


init_sw_popup ()
{
   xv_set(Printtool_info_popup->textpane1, 
	  FRAME_NO_CONFIRM, TRUE,
	  TEXTSW_CONFIRM_OVERWRITE, FALSE,
	  TEXTSW_IGNORE_LIMIT, TEXTSW_INFINITY,
	  0);
}


flash_it(int iconize) 
{
   int flashes = NUM_FLASH;
   Server_image si = NULL;
   Drawable  xid ;
   GC gc;
   int w, h, fg, bg;
   static struct timeval tv = {0, 150000};

   if (iconize) {
      si = (Server_image) xv_get(Ptmo->current_icon, ICON_IMAGE);
      w = (int) icon_get(Ptmo->current_icon, ICON_WIDTH);
      h = (int) icon_get(Ptmo->current_icon, ICON_HEIGHT);
      xid = Ptmo->icon_xid;
      gc = Ptmo->icon_gc;
   } /* end if */
   else {
      xid = Ptmo->mp_xid;
      w = (int)xv_get(Printtool_window1->window1, XV_WIDTH);
      h = (int)xv_get(Printtool_window1->window1, XV_HEIGHT);
      gc = Ptmo->mp_gc;
   } /* end else */
   fg = Ptmo->foreground;
   bg = Ptmo->background;

   for (;;) {
      if (flashes > 0) {
         invert_region(xid, gc, 0, 0, w, h, fg, bg);
	 if (si != NULL)
	    xv_set(Ptmo->current_icon, ICON_IMAGE, si, 0);
	 XFlush(Ptmo->dply);
      } /* end if */
      select(0, 0, 0, 0, &tv);
      invert_region(xid, gc, 0, 0, w, h, fg, bg);
      if (si != NULL)
         xv_set(Ptmo->current_icon, ICON_IMAGE, si, 0);
      XFlush(Ptmo->dply);
      flashes--;

      if (flashes <=0)
	 break;
      select(0, 0, 0, 0, &tv);
   } /* end for */
}

invert_region(Drawable xid, GC gc, int x, int y, int w, int h, int fg, int bg)
{
   XSetForeground(Ptmo->dply, gc, (fg ^ bg));
   XSetFunction(Ptmo->dply, gc, GXxor);
   XFillRectangle(Ptmo->dply, xid, gc, x, y, w, h);
   XSetFunction(Ptmo->dply, gc, GXcopy);
   XSetForeground(Ptmo->dply, gc, fg);
}


flash_main_panel()
{
}

/*
 * Notify callback function for `set_def_bt'.
 */
void
set_default_button(Panel_item item, Event *event)
{
	printtool_prop_popup_objects *ip = (printtool_prop_popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	/* save the properties values in .desksetdefault file */
	apply_button(item, event);
        if (Ptmo->testing) {
           fprintf(stdout, dgettext(DOMAIN_NAME, "%s: test: write resource to database \n"), PROGNAME);
           return;
        }
	write_resources();
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	/* gxv_end_connections */

}


init_gc()
{
   Cms             cms;
   Server_image	   si;
   XGCValues       gc_vals;
   int		   gc_flags;

   cms = (Cms)xv_get(Printtool_window1->window1, WIN_CMS);
   Ptmo->dply = (Display*) xv_get(Printtool_window1->window1, XV_DISPLAY);
   /*
   Ptmo->foreground = (int) xv_get(cms, CMS_FOREGROUND_PIXEL);
   Ptmo->background = (int) xv_get(cms, CMS_BACKGROUND_PIXEL);
   */
   Ptmo->foreground = (int)BlackPixel(Ptmo->dply, DefaultScreen(Ptmo->dply));
   Ptmo->background = (int)WhitePixel(Ptmo->dply, DefaultScreen(Ptmo->dply));
   Ptmo->mp_xid    = (Drawable) xv_get(Printtool_window1->controls1, XV_XID);

   si = (Server_image) xv_get(Ptmo->current_icon, ICON_IMAGE);
   Ptmo->icon_xid =  (Drawable)xv_get(si, XV_XID);
   gc_vals.foreground = Ptmo->foreground;
   gc_vals.background = Ptmo->background;
   gc_flags =  GCForeground | GCBackground;

   /* Create GC used for main panel */
   Ptmo->mp_gc = XCreateGC(Ptmo->dply, Ptmo->mp_xid, gc_flags, &gc_vals);

   /* Create GC used for manipulating the icon */
   Ptmo->icon_gc = XCreateGC(Ptmo->dply, Ptmo->icon_xid, gc_flags, &gc_vals);
}

set_on_line_help()
{
   xv_set(Printtool_window1->window1, 
		  XV_HELP_DATA, "printtool:PRINT_FRAME", 0);
   xv_set(Printtool_window1->controls1, 
		  XV_HELP_DATA, "printtool:PRINT_PANEL", 0);
   xv_set(Printtool_window1->drop_target1, 
		  XV_HELP_DATA, "printtool:PRINT_DROP_SITE", 0);
   xv_set(Printtool_window1->filename_tx, 
		  XV_HELP_DATA, "printtool:FILE", 0);
   xv_set(Printtool_window1->info_bt, 
		  XV_HELP_DATA, "printtool:PRINTERINFO", 0);
   xv_set(Printtool_window1->printer_ls, 
		  XV_HELP_DATA, "printtool:PRINTERLIST", 0);
   xv_set(Printtool_window1->copy_tx, 
		  XV_HELP_DATA, "printtool:COPIES", 0);
   xv_set(Printtool_window1->mheader_tg, 
		  XV_HELP_DATA, "printtool:HEADER", 0);
   xv_set(Printtool_window1->print_bt, 
		  XV_HELP_DATA, "printtool:PRINT", 0);
   xv_set(Printtool_window1->filter_tx, 
		  XV_HELP_DATA, "printtool:PRINTMETHOD", 0);
   xv_set(Printtool_window1->queue_bt, 
		  XV_HELP_DATA, "printtool:STATUS", 0);
   xv_set(Printtool_window1->rm_job_bt, 
		  XV_HELP_DATA, "printtool:STOPPRINTING", 0);
   xv_set(Printtool_window1->queue_ls, 
		  XV_HELP_DATA, "printtool:PRINT_SCROLL_LIST", 0);
   xv_set(Printtool_window1->prop_bt, 
		  XV_HELP_DATA, "printtool:PROPERTIES", 0);
}


static void
set_info_help() 
{
    xv_set(Printtool_info_popup->textpane1, 
	   XV_HELP_DATA, "printtool:INFOTEXT", 0);
}

static void
set_props_help() 
{
   xv_set(Printtool_prop_popup->sheader_tg, 
		  XV_HELP_DATA, "printtool:HEADER", 0);
   xv_set(Printtool_prop_popup->notify_all_ch, 
		  XV_HELP_DATA, "printtool:JOBDONENOTIFY", 0);
   xv_set(Printtool_prop_popup->override_filter_tg, 
		  XV_HELP_DATA, "printtool:PMOVERRIDE", 0);
   xv_set(Printtool_prop_popup->apply_bt, 
		  XV_HELP_DATA, "printtool:APPLY", 0);
   xv_set(Printtool_prop_popup->set_def_bt, 
		  XV_HELP_DATA, "printtool:SETDEFAULT", 0);
   xv_set(Printtool_prop_popup->reset_bt, 
		  XV_HELP_DATA, "printtool:RESET", 0);
   xv_set(Printtool_prop_popup->controls3, 
		  XV_HELP_DATA, "printtool:PROP_PANEL", 0);
}


get_uname()
{
   FILE *fp=NULL;
   if ((Ptmo->username = getenv("LOGNAME")) == NULL)
      fprintf(stderr, dgettext(DOMAIN_NAME, 
             "%s: LOGNAME environment variable not set\n"), PROGNAME);

   if ((fp = popen("id", "r")) != NULL) {
      char buf[256];
      while (fgets(buf, sizeof(buf), fp) != NULL) {
            /* str_contains return -1 for not found, others for found */
	 if (str_contains(buf, dgettext(DOMAIN_NAME, "root")) != -1) {
	    isroot = 1;
            break;
         }  /* end if */
      } /* while */
      pclose(fp);
   }
}

is_printable(char* type_name) 
{
  if (type_name && *type_name) {
     if (strcmp(type_name, "corefile") == 0)   
        return 0;
     if (strcmp(type_name, "audio-file") == 0)   
        return 0;
     if (strcmp(type_name, "compress") == 0)   
        return 0;
     if (strcmp(type_name, "o-file") == 0)   
        return 0;
  }
  return 1;
}

set_LC_ALL(char *str)
{
   static char locale_str[20]; 

   if ((char *)getenv("LANG") == NULL)  /* ie. Eng. */
      return;
      
   if (str) {
      sprintf(locale_str, "%s=%s", "LC_ALL", str);
      if (putenv(locale_str)) {
         print_footer_msg(BF, dgettext(DOMAIN_NAME, "Unable to set LC_ALL environment var %s"), str);
         return 0;
      }
   }
   else {  /* unset LC_ALL */
      sprintf(locale_str, "%s=%s", "LC_ALL", "");
      if (putenv(locale_str)) {
         print_footer_msg(BF, dgettext(DOMAIN_NAME, "Unable to unset LC_ALL environment var"), "");
         return 0;
      }
   }
}

sllist *
get_selected_list()
{
   int count, i, len;
   char *data;
   sllist *head = NULL, *current;
   sllist* create_node();


   count = (int)xv_get(Printtool_window1->queue_ls, PANEL_LIST_NROWS);
   if (!count)
      return head;

   for (i = 0; i < count; i++) {
      if (xv_get(Printtool_window1->queue_ls, PANEL_LIST_SELECTED, i)) {
         data = (char*)xv_get(Printtool_window1->queue_ls, PANEL_LIST_STRING, i);
	 len = strcspn(data, " ");
	 if (!head) {
	    head = create_node(len);
	    strncpy(head->member, data, len);
	    head->member[len] = '\0';
	    current = head;
	 }
	 else {
	    current->next = create_node(len);
	    strncpy(current->next->member, data, len);
	    current->next->member[len] = '\0';
	    current = current->next;
	 }
      } /* if selected */
   } /* for */
   return (head);
}

sllist*
create_node(int len)
{
   sllist *p;
   p = (sllist*)malloc(sizeof(struct sllist));
   p->member = (char*)malloc(len+1);
   p->next = NULL;
   return(p);
}

sllist* destroy_node(sllist *p)
{
   free(p->member);
   free(p);
   return (sllist*)NULL;
}

sllist* destroy_list(sllist *selected_list)
{
   sllist *p, *destroy_node();

   if (!selected_list)
      return;

   p = selected_list;
   while (p) {
      sllist *tmpp = p;
      p = p->next;
      tmpp = destroy_node(tmpp);
   }
   return (sllist*)NULL;
}

is_job_selected(sllist **head, char* job)
{
   int selected = FALSE;
   sllist *p, *prevp = (sllist*)NULL;

   p = prevp = *head;
   while (p) {
      if (strcmp(p->member, job) == 0) {
         selected = TRUE;

         if (p->next) {
	    if (*head == p) {  /* head is the node to be destroyed */
	       *head = p->next;
               p = destroy_node(p);
	    }
	    else {
	       prevp->next = p->next;
               p = destroy_node(p);  /* destroyed middle node */
	    }
         }
         else {
	    if (*head == p)
	       *head = (sllist*)NULL;
	    else
	       prevp->next = (sllist*)NULL;
            p = destroy_node(p); /* the last node or the only head node */
	 }
         break;
      }
      else {  /* not match, next node */
	 prevp = p;
	 p = p->next;
      }
   } /* end while */
   return selected;
}

