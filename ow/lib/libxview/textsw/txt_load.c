#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)txt_load.c 1.41 93/12/10";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Text load popup frame creation and support.
 */

#include <xview_private/primal.h>
#include <xview_private/txt_impl.h>
#include <xview_private/ev_impl.h>
#include <sys/time.h>
#include <signal.h>
#include <xview/notice.h>
#include <xview/frame.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/openmenu.h>
#include <xview/wmgr.h>
#include <xview/pixwin.h>
#include <xview/win_struct.h>
#include <xview/win_screen.h>
#include <xview/file_chsr.h>

#ifdef SVR4
#include <string.h>
#endif /* SVR4 */
 
#define		MAX_DISPLAY_LENGTH	50
#define   	MAX_STR_LENGTH		1024

#define HELP_INFO(s) XV_HELP_DATA, s,

typedef enum {
    FILE_CMD_ITEM = 0,
    DIR_STRING_ITEM = 1,
    FILE_STRING_ITEM = 2
}               File_panel_item_enum;

Pkg_private Panel_item load_panel_items[];

Pkg_private Textsw_view_handle text_view_frm_p_itm();
Pkg_private Xv_Window frame_from_panel_item();

static int 
modifiedOK(Textsw_folio folio)
{
    static int      result;
    Frame           frame;
    Xv_Notice	    text_notice;

    frame = FRAME_FROM_FOLIO_OR_VIEW(folio);
    text_notice = (Xv_Notice)xv_get(frame, 
                                XV_KEY_DATA, text_notice_key, 
				NULL);
    if (!text_notice) {
        text_notice = xv_create(frame, NOTICE, NULL);
        xv_set(frame, XV_KEY_DATA, text_notice_key, text_notice, NULL);
    }
    xv_set(text_notice, 
            NOTICE_LOCK_SCREEN, FALSE,
            NOTICE_BLOCK_THREAD, TRUE,
            NOTICE_MESSAGE_STRINGS,
		XV_MSG("The text has been edited.\n\
Load File will discard these edits. Please confirm."),
            0,
            NOTICE_BUTTON_YES, 
			XV_MSG("Confirm, discard edits"),
	    NOTICE_BUTTON_NO, XV_MSG("Cancel"),
	    NOTICE_STATUS, &result,
            XV_SHOW, TRUE, 
            NULL);

    if (result == NOTICE_NO || result == NOTICE_FAILED)
        return FALSE;
    return TRUE;
}

static int
displayDirectoryNotice(Textsw_folio folio)
{
    Frame frame = FRAME_FROM_FOLIO_OR_VIEW(folio);
    Xv_Notice text_notice;

    text_notice = (Xv_Notice)xv_get(frame, XV_KEY_DATA, text_notice_key, NULL);
    if (!text_notice) {
	text_notice = xv_create(frame, NOTICE, NULL);
        xv_set(frame, XV_KEY_DATA, text_notice_key, text_notice, NULL);
    }
    xv_set(text_notice, 
	NOTICE_LOCK_SCREEN, FALSE,
	NOTICE_BLOCK_THREAD, TRUE,
        NOTICE_MESSAGE_STRINGS,
	XV_MSG("Cannot change directory.\n\
Change Directory Has Been Disabled."),
		            0,
        NOTICE_BUTTON_YES, XV_MSG("Continue"),
	XV_SHOW, TRUE, 
        NULL);
    return TRUE;
}

int
displayFilenameNotice(Textsw_folio folio)
{
    Frame frame = FRAME_FROM_FOLIO_OR_VIEW(folio);
    Xv_Notice text_notice;

    text_notice = (Xv_Notice)xv_get(frame, XV_KEY_DATA, text_notice_key, NULL);
    if (!text_notice) {
        text_notice = xv_create(frame, NOTICE, NULL);
        xv_set(frame, XV_KEY_DATA, text_notice_key, text_notice, NULL);
    }
    xv_set(text_notice, 
        NOTICE_LOCK_SCREEN, FALSE,
	NOTICE_BLOCK_THREAD, TRUE,
        NOTICE_MESSAGE_STRINGS,
	XV_MSG("No file name was specified.\n\
Specify a file name to Load."),
        0,
        NOTICE_BUTTON_YES, XV_MSG("Continue"),
        XV_SHOW, TRUE, 
        NULL);
    return TRUE;
}

Pkg_private int
problemsWithcd(Textsw_folio folio, char *dir_str, char *curr_dir,
               int locx, int locy, int *changed_directory)
{
    *changed_directory = FALSE;
    (void) getcwd(curr_dir, MAXPATHLEN);
    if (strcmp(curr_dir, dir_str) != 0) {
	if (!(folio->state & TXTSW_NO_CD)) {
	    if (textsw_change_directory(folio,dir_str,FALSE,locx,locy) != 0) {
		return TRUE;
            } else {
                *changed_directory = TRUE;
                return FALSE;
            }
        } else {
            return(displayDirectoryNotice(folio));
        }
    } else
        return FALSE;
} 
 
Pkg_private int
open_cmd_proc(fc, path,file,client_data)
    Frame             fc;
    char             *path;
    char             *file;
    Xv_opaque         client_data;
{

    Textsw_view_handle view  = (Textsw_view_handle)window_get(fc,WIN_CLIENT_DATA,0);
    Textsw_folio       folio = FOLIO_FOR_VIEW(view);
    Textsw          textsw = FOLIO_REP_TO_ABS(folio);
    char           *dir_str;
    int             result;
    register int    locx, locy;
    char            curr_dir[MAXPATHLEN];
    int             changed;

    if (textsw_has_been_modified(textsw))
        if (!modifiedOK(folio))
            return XV_OK;

    dir_str = (char *) xv_get(fc, FILE_CHOOSER_DIRECTORY); 
    locx = locy = 0;

    if (problemsWithcd(folio, dir_str, curr_dir, locx, locy, &changed))
        return TRUE;

    if ((int)strlen(file) > 0) {
        result = textsw_load_file(textsw, file, TRUE, 0, 0);
        if (result == 0) {
            (void) textsw_set_insert(folio, 0L);
            (void) xv_set(fc, XV_SHOW, FALSE, 0);
            return FALSE;
        }
        /* error */
        return TRUE;
    }

    return(displayFilenameNotice(folio)); 
}
   

Pkg_private int
save_cmd_proc(fc, path,exists)
    Frame             fc;
    char             *path;
    int               exists;
{

   Textsw_view_handle view  = (Textsw_view_handle)window_get(fc,WIN_CLIENT_DATA,0);
   Textsw_folio folio = FOLIO_FROM_VIEW(view);
   int		confirm_state_changed = 0;

   xv_set(fc,
          FRAME_SHOW_FOOTER, TRUE,
	  FRAME_LEFT_FOOTER,XV_MSG("Saved"),
	  NULL);

   if (folio->state & TXTSW_CONFIRM_OVERWRITE) {
	folio->state &= ~TXTSW_CONFIRM_OVERWRITE;
	confirm_state_changed = 1;
   }
   textsw_store_file(VIEW_REP_TO_ABS(view),path,0,0);
    if (confirm_state_changed)
	folio->state |= TXTSW_CONFIRM_OVERWRITE;
    return XV_OK;
}


