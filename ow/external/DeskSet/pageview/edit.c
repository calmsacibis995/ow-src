#ifndef lint
static char *sccsid = "@(#)edit.c 3.3 93/07/20";
#endif

/*
 * Copyright (c) 1990 - Sun Microsystems Inc.
 */

/*
 * edit.c - textsw editor input for pageview.
 */

#include "pageview.h"
#include <string.h>
#ifdef SVR4
#include <dirent.h>
#else
#include <sys/dir.h>
#endif

#include <xview/text.h>

char *edit_tmpfile;
static Textsw output_textsw;
Textsw edit_textsw;
Panel_item textsw_drop_target;

Notify_value textsw_event_proc ();

char   tmp_filename [MAXNAMLEN];
int    output_number = 0;
char **output_strings = NULL;

void
append_output (s)
char *s;
{
    output_number++;
    if (output_number == 1)
       output_strings = (char **) malloc (sizeof (char *));
    else
       output_strings = (char **) realloc (output_strings, (sizeof (char *) *
							    output_number));

    output_strings [output_number - 1] = malloc (strlen (s) + 1);
    strcpy (output_strings [output_number - 1], s);
}

void
clear_output()
{
    xv_set(output_textsw,
	   TEXTSW_BROWSING, FALSE,
	   NULL);
    textsw_erase(output_textsw, 0, TEXTSW_INFINITY);

    sprintf (tmp_filename, DGET("/tmp/pageview%d.log"), getpid ());
    textsw_store_file(output_textsw, tmp_filename, 0, 0);
    xv_set(output_textsw,
	   TEXTSW_BROWSING, TRUE,
	   NULL);
    unlink(tmp_filename);
}

void
edit_file(s)
    char       *s;
{
    if (strcmp (s, NONAME) == 0)
       xv_set(edit_textsw,
	   TEXTSW_FILE, (char *) NULL,
	   TEXTSW_INSERTION_POINT, 0,
	   NULL);
    else
       xv_set(edit_textsw,
	   TEXTSW_FILE, s,
	   TEXTSW_INSERTION_POINT, 0,
	   NULL);
    textsw_possibly_normalize(edit_textsw, 0);
}

static void
run_callback()
{
    FILE       *tmpfp;

    unlink (edit_tmpfile);
    textsw_store_file(edit_textsw, edit_tmpfile, 0, 0);

    if ((tmpfp = fopen(edit_tmpfile, "r")) == 0)
	return;
    setbusy ();
    newfile(edit_tmpfile, tmpfp);
/*
    unlink(tmpname);
*/
    set_icon_label (NONAME);
    setactive ();
}

Frame
init_edit(parent)
    Frame       parent;
{
    Frame       	frame;
    Panel       	panel;
    Panel       	panel1;
    Panel_item  	run_button; 
    Selection_requestor Textsw_Sel;
    Dnd                 Textsw_Dnd_object;
    char	        fr_label [100];
    char	       *ps_label;


    edit_tmpfile = (char *) mktemp(DGET("/tmp/PVEdit-XXXXXX"));

    ps_label = LGET ("PostScript");

    sprintf (fr_label, "%s: %s", PV_Name, ps_label);
    frame = (Frame) xv_create(parent, FRAME_CMD,
			      FRAME_LABEL, fr_label,
			      FRAME_CMD_PUSHPIN_IN, TRUE,
#ifdef OW_I18N
			      WIN_USE_IM, TRUE,
#endif
			      NULL);
    panel = (Panel) xv_get(frame, FRAME_CMD_PANEL);

    run_button = xv_create(panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, LGET("Run"),
	      PANEL_NOTIFY_PROC, run_callback,
	      XV_HELP_DATA, "pageview:run",
	      NULL);


/*
 * Create objects required for drag and drop into the Textsw.
 */

    Textsw_Dnd_object = xv_create(panel, DRAGDROP,
                                SEL_CONVERT_PROC, PageviewSelectionConvert,
                                DND_TYPE, DND_COPY,
                                DND_CURSOR, xv_create(NULL, CURSOR,
                                            CURSOR_IMAGE, source_drag_ptr_image,
                                            CURSOR_XHOT, 17,
                                            CURSOR_YHOT, 24,
					    CURSOR_OP, PIX_SRC^PIX_DST,
                                            0),
                                DND_ACCEPT_CURSOR, xv_create(NULL, CURSOR,
                                            CURSOR_IMAGE, source_drop_ptr_image,
                                            CURSOR_XHOT, 17,
                                            CURSOR_YHOT, 24,
					    CURSOR_OP, PIX_SRC^PIX_DST,
                                            0),
                                0);


    textsw_drop_target = xv_create (panel, PANEL_DROP_TARGET,
					PANEL_DROP_DND,	    Textsw_Dnd_object,
					PANEL_DROP_DND_TYPE,
						     PANEL_DROP_COPY_ONLY,
				 	PANEL_NOTIFY_PROC,  
						     drop_target_notify_proc,
					XV_HELP_DATA,
						     "pageview:SourceDrag",
					NULL);

    xv_set (panel, XV_KEY_DATA, UI6, textsw_drop_target, NULL);
    xv_set (panel, XV_KEY_DATA, UI7, run_button, NULL);
    xv_set (panel, XV_KEY_DATA, UI9, Textsw_Dnd_object, NULL);

    window_fit_height(panel);

    edit_textsw = xv_create(frame, TEXTSW,
			    WIN_BELOW, panel,
			    XV_X, 0,
			    WIN_ROWS, 20,
			    WIN_COLUMNS, 80,
/*
			    TEXTSW_STORE_CHANGES_FILE, FALSE,
*/
			    TEXTSW_STORE_CHANGES_FILE, TRUE,
			    TEXTSW_NOTIFY_PROC, textsw_event_proc, 
			    XV_HELP_DATA, "pageview:edit",
			    NULL);
    xv_set(panel, XV_WIDTH, xv_get(edit_textsw, XV_WIDTH), NULL);

    panel1 = xv_create(frame, PANEL,
		       XV_X, 0,
		       WIN_BELOW, edit_textsw,
		       NULL);
    (void) xv_create(panel1, PANEL_BUTTON,
		     PANEL_LABEL_STRING, LGET("Clear output log"),
		     PANEL_NOTIFY_PROC, clear_output,
		     WIN_BELOW, panel,
		     XV_X, 0,
		     XV_HELP_DATA, "pageview:clearoutput",
		     NULL);
    window_fit_height(panel1);

    output_textsw = xv_create(frame, TEXTSW,
			      TEXTSW_READ_ONLY, TRUE,
			      TEXTSW_BROWSING, TRUE,
			      TEXTSW_DISABLE_LOAD, TRUE,
			      TEXTSW_DISABLE_CD, TRUE,
			      WIN_BELOW, panel1,
			      XV_X, 0,
			      WIN_ROWS, 14,
			      WIN_COLUMNS, 80,
			      XV_HELP_DATA, "pageview:output",
			      0);
    xv_set(panel1, XV_WIDTH, xv_get(output_textsw, XV_WIDTH), NULL);

    window_fit(frame);

    place_drop_site (textsw_drop_target, run_button, panel);

/*
    notify_interpose_event_func(panel, panel_event_proc, NOTIFY_SAFE);
*/

    return (frame);
}

void
dump_output ()
{
    int i;

    if (output_number == 0)
       return;

    xv_set(edit_textsw,
	   TEXTSW_INSERTION_POINT, TEXTSW_INFINITY,
	   NULL);
    for (i = 0; i < output_number; i++) {
        textsw_insert(output_textsw, output_strings [i], 
		      strlen(output_strings [i]));
	free (output_strings [i]);
	}
    sprintf (tmp_filename, DGET("/tmp/pageview%d.log"), getpid ());
    textsw_store_file(output_textsw, tmp_filename, 0, 0);
    unlink(tmp_filename);
    free (output_strings);
    output_strings = (char **) NULL;
    output_number = 0;
}

Notify_value
textsw_event_proc (local, attributes)
Textsw local;
Attr_avlist attributes;
{

    Attr_avlist		attrs;
    char	       *attr_string;
    char	       *temp;

    for (attrs = attributes; *attrs; attrs = attr_next (attrs)) {
	switch ((Textsw_action) (*attrs)) {

	   case TEXTSW_ACTION_USING_MEMORY:

	      if (edit_textsw != (Textsw) NULL) 
/*
 * If we got TEXTSW_ACTION_USING_MEMORY, user selected Empty file from 
 * menu. In this case, turn off capability of dragging out of pageview.
 */

	         if (xv_get (edit_textsw, TEXTSW_LENGTH) == 0) {
    		    xv_set (main_panel_drop_target, 
				PANEL_DROP_FULL, FALSE, NULL);
    		    xv_set (textsw_drop_target, PANEL_DROP_FULL, FALSE, NULL);
		    }
	      break;
	
	   case TEXTSW_ACTION_CHANGED_DIRECTORY:
	      attr_string = (char *) attrs [1];
              switch (attr_string [0]) {
                  case '/':
                      (void)strcpy(Directory, (char *) attrs[1]);
                      break;
                  case '.':
                  case '\0':
                      break;
                  default:
                      (void)strcat(Directory,  "/" );
                      (void)strcat(Directory, (char *) attrs[1]);
                      break;                                
                  }
              ATTR_CONSUME(*attrs);
              break;
	   
	   case TEXTSW_ACTION_LOADED_FILE:
	      strcpy (pathname, (char *) attrs[1]);
              temp = strrchr ( (char *) attrs[1], '/');
              if (temp != (char *) NULL) {
                 int length = (int) (temp - attrs[1]);
                 (void) strncpy (Directory, (char *) attrs[1],
                                        length);
                 Directory [length] = '\0';
                 }
	      else {
	         strcpy (pathname, Directory);
                 strcat (pathname,  "/" );
		 strcat (pathname, (char *) attrs [1]);
		 }
	      set_icon_label (pathname);
              ATTR_CONSUME(*attrs);
              break;
	   }
	}
}    

