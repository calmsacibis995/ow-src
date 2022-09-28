#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)txt_incl.c 1.36 93/07/26";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Text include popup frame creation and support.
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
#include <unistd.h>
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

Pkg_private Panel_item include_panel_items[];
Pkg_private Textsw_view_handle text_view_frm_p_itm();
Pkg_private Xv_Window frame_from_panel_item();

Pkg_private int problemsWithcd();
Pkg_private int displayFilenameNotice();


int
include_cmd_proc(fc,path,file,client_data)
    Frame             fc;
    char             *path;
    char             *file;
    Xv_opaque         client_data;
 {
    Textsw_view_handle view = (Textsw_view_handle)window_get(fc,WIN_CLIENT_DATA,0);
    Textsw_folio    folio = FOLIO_FOR_VIEW(view);
    int             error;
    char           *dir_str;
    register int    locx, locy;
    char            curr_dir[MAXPATHLEN];
    Frame           popup_frame;
    int		    textsw_changed_directory;

    dir_str = (char *) xv_get(fc, FILE_CHOOSER_DIRECTORY);
    locx = locy = 0;
    if (problemsWithcd(folio, dir_str, curr_dir, locx, locy,
                                     &textsw_changed_directory))
        return TRUE;
    if ((int)strlen(file) > 0) {
	if (textsw_file_stuff_from_str(VIEW_FROM_FOLIO_OR_VIEW(folio),
				       file, locx, locy) == 0) {
	    
	    (void) xv_set(fc, XV_SHOW, FALSE, 0);
	    if (textsw_changed_directory)
	        textsw_change_directory(folio, curr_dir, FALSE, locx, locy);
	    return FALSE;
	}
	if (textsw_changed_directory)
	    textsw_change_directory(folio, curr_dir, FALSE, locx, locy);
	return TRUE;
    }

    (void)displayFilenameNotice(folio);
    if (textsw_changed_directory)
        textsw_change_directory(folio, curr_dir, FALSE, locx, locy);
    return TRUE;
}
