#ifndef lint
static char *sccsid = "@(#)print.c 3.5 93/04/15";
#endif

/*
 * Copyright (c) 1990 - Sun Microsystems Inc.
 */

/*
 * print.c - printer output for pageview.
 */

#include "ds_item.h"
#include "pageview.h"
#include <sys/stat.h>

static int  wholefile = 1;

static void
print_file(item, event)
    Panel_item  item;
    Event      *event;
{
    char       *printer = (char *) xv_get(xv_get(item, XV_KEY_DATA, UI1),
					  PANEL_VALUE);
    char       *dir = (char *) xv_get(xv_get(item, XV_KEY_DATA, UI2),
				      PANEL_VALUE);
    char       *file = (char *) xv_get(xv_get(item, XV_KEY_DATA, UI3),
				       PANEL_VALUE);
    int         print = ((int) xv_get(xv_get(item, XV_KEY_DATA, UI4),
				      PANEL_VALUE) == 0);

    if (strcmp (pathname, NONAME) == 0) {
       if (verbose)
	  fprintf(stderr, MGET("%s: No file to print!\n"), ProgramName);
       notice_prompt (baseframe, NULL,
			   NOTICE_MESSAGE_STRINGS,
			      EGET("No file to print!"),
			      NULL,
			   NOTICE_BUTTON, LGET("Continue"), 1,
			   NULL);
       return ;
       }

    if (print) {
	char        printerstring[1024];
	FILE       *pfp;
	int         ret;

#ifdef SVR4
	if ((int) strlen(printer) > 0)
/*	    sprintf(printerstring, "lp -d%s > /dev/console", printer); */
	    sprintf(printerstring, "lp -Tpostscript -d%s", printer);
	else
/*	    sprintf(printerstring, "lp > /dev/console"); */
	    sprintf(printerstring, "lp -Tpostscript");
#else /* SVR4 */
	if ((int) strlen(printer) > 0)
/*	    sprintf(printerstring, "lpr -P%s > /dev/console", printer); */
	    sprintf(printerstring, "lpr -P%s", printer);
	else
/*	    sprintf(printerstring, "lpr > /dev/console"); */
	    sprintf(printerstring, "lpr");
#endif /* SVR4 */

	if ((pfp = popen(printerstring, "w")) == NULL) {
	    fprintf(stderr, MGET("%s: Couldn't connect to lpr.\n"), 
							ProgramName);
	    return;
	}
	PrintPage(pfp, wholefile);
	if ((ret = pclose(pfp)) != 0) {
	    if (verbose) 
	       fprintf(stderr, MGET("%s: lpr failed with %d.\n"), 
						ProgramName, ret);
	    notice_prompt (baseframe, NULL,
			   NOTICE_MESSAGE_STRINGS,
			      EGET("Print failed!\nCheck if valid printer name was entered."),
			      NULL,
			   NOTICE_BUTTON, LGET("Ok"), 1,
			   NULL);
	}
    } else {
	char        fname[1024];
	FILE       *pfp;
	char	    tmpdir [MAXPATHLEN];
        struct stat stat_buf;
    	int	    stat_status;
    	int	    notice_status;
	char  	    msg [1024];

	ds_expand_pathname (dir, tmpdir);
	sprintf(fname, "%s/%s", tmpdir, file);

	stat_status = stat (fname, &stat_buf);
        if (stat_status == 0) {
	   sprintf (msg, 
		    EGET("The file \"%s\" already exists.\nDo you want to overwrite the file \"%s\"?"),
		    file, file);
	   notice_status = notice_prompt (baseframe, NULL,
			  	NOTICE_MESSAGE_STRINGS,
 			      	   msg,
			      	   NULL,
			  	NOTICE_BUTTON_YES, LGET("Cancel"),
			  	NOTICE_BUTTON_NO, 
						LGET("Overwrite Existing File"),
			  	NULL);
	   if (notice_status == NOTICE_YES)
	      return;
	   unlink (fname);
	   }

	if ((pfp = fopen(fname, "w")) == NULL) {
	    sprintf (msg, MGET("Couldn't write file: %s.\n"), fname);
	    xv_set (baseframe, FRAME_LEFT_FOOTER, msg, NULL);
	    return;
	}
	PrintPage(pfp, wholefile);
	fclose(pfp);
    }
}


static void
switch_mode(item, event)
    Panel_item  item;
    Event      *event;
{
    Panel_text_item printer = xv_get(item, XV_KEY_DATA, UI1);
    Panel_text_item dir = xv_get(item, XV_KEY_DATA, UI2);
    Panel_text_item file = xv_get(item, XV_KEY_DATA, UI3);
    int         print = ((int) xv_get(item, PANEL_VALUE) == 0);

    if (print) {
	xv_set(dir, XV_SHOW, FALSE, NULL);
	xv_set(file, XV_SHOW, FALSE, NULL);
	xv_set(printer, XV_SHOW, TRUE, NULL);
    } else {
	xv_set(printer, XV_SHOW, FALSE, NULL);
	xv_set(dir, XV_SHOW, TRUE, NULL);
	xv_set(file, XV_SHOW, TRUE, NULL);
    }
}

void
page_or_file(item, event)
    Panel_item  item;
    Event      *event;
{
    wholefile = (xv_get(item, PANEL_VALUE) == 0);
}

Frame
init_print(parent)
    Frame       parent;
{
    Frame       	 frame;
    Panel       	 panel;
    Panel_button_item 	 but;
    Panel_choice_item 	 dest;
    Panel_text_item 	 printer;
    Panel_text_item 	 dir;
    Panel_text_item 	 file;
	Panel_item      check_box;
    char		 fr_label [100];
    char		*pr_label;
    char		*printer_name;
    char		 dir_label [MAXPATHLEN];

    ds_expand_pathname ("~", dir_label);

    pr_label = LGET ("Print");
 
    sprintf (fr_label, "%s: %s", PV_Name, pr_label);

    frame = (Frame) xv_create(parent, FRAME_CMD,
			      FRAME_LABEL, fr_label,
#ifdef OW_I18N
				 WIN_USE_IM, TRUE,
#endif
			      NULL);
    panel = (Panel) xv_get(frame, FRAME_CMD_PANEL);

    printer = (Panel_text_item) xv_create(panel, PANEL_TEXT,
					  PANEL_LABEL_STRING, LGET("Printer:"),
					  XV_Y, xv_row(panel, 1),
					  PANEL_VALUE_DISPLAY_LENGTH, 40,
					  XV_HELP_DATA, "pageview:printer",
					  NULL);

#ifdef SVR4

    if ((printer_name = getenv("LPDEST")) != NULL)
       xv_set (printer, PANEL_VALUE, printer_name, NULL) ;
    else {
       FILE *fp;
       char buf[256];
       fp = popen ("lpstat -d", "r");
       if (fp) {
          fread (buf, MAXPATHLEN, 1, fp);
          if (strchr (buf, ':') != NULL) {
             printer_name = (char *) strtok (buf, ":");
             printer_name = (char *) strtok ((char *) NULL, "\n");
             xv_set (printer, PANEL_VALUE, printer_name, NULL) ;
             }
          pclose (fp);
          }
       }

#else

    if ((printer_name = getenv("PRINTER")) != NULL)
       xv_set (printer, PANEL_VALUE, printer_name, NULL) ;
    else
       xv_set (printer, PANEL_VALUE, "lp", NULL);

#endif /* SVR4 */

    dir = (Panel_text_item) xv_create(panel, PANEL_TEXT,
				      PANEL_LABEL_STRING, LGET("Directory:"),
				      XV_Y, xv_row(panel, 1),
				      PANEL_VALUE_DISPLAY_LENGTH, 40,
				      PANEL_VALUE, dir_label,
				      XV_HELP_DATA, "pageview:printdir",
				      NULL);

    file = (Panel_text_item) xv_create(panel, PANEL_TEXT,
				       PANEL_LABEL_STRING, LGET("File:"),
				       XV_Y, xv_row(panel, 2),
				       PANEL_VALUE_DISPLAY_LENGTH, 40,
				       XV_HELP_DATA, "pageview:printfile",
				       NULL);

    dest = (Panel_choice_item) xv_create(panel, PANEL_CHOICE,
					 PANEL_LABEL_STRING, 
							LGET("Destination:"),
					 XV_Y, xv_row(panel, 0),
					 PANEL_VALUE, 0,
					 PANEL_CHOICE_STRINGS,
					 LGET("Printer"), LGET("File"), NULL,
					 PANEL_NOTIFY_PROC, switch_mode,
					 XV_KEY_DATA, UI1, printer,
					 XV_KEY_DATA, UI2, dir,
					 XV_KEY_DATA, UI3, file,
					 XV_HELP_DATA, "pageview:printorfile",
					 NULL);

    check_box = xv_create(panel, PANEL_CHECK_BOX,
		     PANEL_LABEL_STRING, LGET("This page only:"),
		     XV_Y, xv_row(panel, 3),
		     PANEL_VALUE, 0,
		     PANEL_NOTIFY_PROC, page_or_file,
		     XV_HELP_DATA, "pageview:thispageonly",
		     NULL);

	ds_justify_items(panel, 0);
	xv_set(dir, XV_SHOW, FALSE, NULL);
	xv_set(file, XV_SHOW, FALSE, NULL);

    but = (Panel_button_item) xv_create(panel, PANEL_BUTTON,
					PANEL_LABEL_STRING, LGET("Print"),
					XV_KEY_DATA, UI1, printer,
					XV_KEY_DATA, UI2, dir,
					XV_KEY_DATA, UI3, file,
					XV_KEY_DATA, UI4, dest,
					PANEL_NOTIFY_PROC, print_file,
					XV_X, xv_get(check_box, XV_X) + xv_get(check_box, XV_WIDTH) + 25,
					XV_Y, xv_row(panel, 3),
					XV_HELP_DATA, "pageview:print",
					NULL);

    window_fit(panel);
    xv_set(frame,
	   XV_WIDTH, xv_get(panel, XV_WIDTH),
	   XV_HEIGHT, xv_get(panel, XV_HEIGHT),
	   NULL);

    return (frame);
}
