#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)wmgr_decor.c 1.27 93/07/14";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL_NOTICE 
 *	file for terms of the license.
 */

/*
 * Window mgr decoration handling. handling.
 */

#include <X11/Xlib.h>
#include <xview_private/wmgr_decor.h>
#include <xview/server.h>
#include <xview_private/fm_impl.h>
#include <xview_private/draw_impl.h>
#include <X11/Xatom.h>

/* bit definitions for MwmHints.flags */
#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)

/* If the following motif #defines are changed, you must change the 
   corresponding #defines in fm_impl.h   */ 
/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZE       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)
 
/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)
 
typedef struct
{
    int      flags;
    int      functions;
    int      decorations;
    int      inputMode;
    int      status;
} PropMotifWmHints;


Xv_private int
wmgr_add_decor(frame_public, decor_list, num_of_decor)
    Frame           frame_public;
    Atom           *decor_list;
    int             num_of_decor;
{
    Xv_Drawable_info *info;
    Xv_opaque       server_public;

    DRAWABLE_INFO_MACRO(frame_public, info);
    server_public = xv_server(info);
    XChangeProperty(xv_display(info), xv_xid(info),
			     xv_get(server_public, SERVER_WM_ADD_DECOR),
			     XA_ATOM, 32,
			     PropModeReplace, (unsigned char *)decor_list,
                             num_of_decor);
    return XV_OK;
}

Xv_private int
wmgr_delete_decor(frame_public, decor_list, num_of_decor, motif_decor)
    Frame           frame_public;
    Atom           *decor_list;
    int             num_of_decor;
    int		    motif_decor;
{
    Xv_Drawable_info *info;
    Xv_opaque       server_public;
    Atom     mwm_hint_prop = 0;
    PropMotifWmHints mwm_hints = {
        MWM_HINTS_DECORATIONS | MWM_HINTS_FUNCTIONS,
        MWM_FUNC_RESIZE | MWM_FUNC_MOVE | MWM_FUNC_MINIMIZE | MWM_FUNC_MAXIMIZE
        | MWM_FUNC_CLOSE,
        MWM_DECOR_BORDER | MWM_DECOR_RESIZE | MWM_DECOR_TITLE | MWM_DECOR_MENU
        | MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE };

    DRAWABLE_INFO_MACRO(frame_public, info);
    server_public = xv_server(info);
    
    mwm_hint_prop = XInternAtom(xv_display(info), "_MOTIF_WM_HINTS", FALSE);

    XChangeProperty(xv_display(info), xv_xid(info),
			     xv_get(server_public, SERVER_WM_DELETE_DECOR),
			     XA_ATOM, 32,
			     PropModeReplace, (unsigned char *)decor_list,
                             num_of_decor);
 
    if( motif_decor & MWM_DECOR_RESIZE ) {
        mwm_hints.decorations &= ~MWM_DECOR_RESIZE;
        mwm_hints.functions &= ~MWM_FUNC_RESIZE;
    }
    if( motif_decor & MWM_DECOR_TITLE )
        mwm_hints.decorations &= ~MWM_DECOR_TITLE;
    if( motif_decor & MWM_DECOR_MENU ) {
        mwm_hints.decorations &= ~MWM_DECOR_MENU;
        mwm_hints.functions = 0;
    }
    if( motif_decor & MWM_DECOR_MINIMIZE ) {
        mwm_hints.decorations &= ~MWM_DECOR_MINIMIZE;
        mwm_hints.functions &= ~MWM_FUNC_MINIMIZE;
    }
    if( motif_decor & MWM_DECOR_MAXIMIZE ) {
        mwm_hints.decorations &= ~MWM_DECOR_MAXIMIZE;
        mwm_hints.functions &= ~MWM_FUNC_MAXIMIZE;
    }
	    
    XChangeProperty(xv_display(info), xv_xid(info),
                    mwm_hint_prop, mwm_hint_prop, 32, PropModeReplace,
                    (unsigned char *)&mwm_hints, 5);
    
    return XV_OK;
}



Xv_private int
wmgr_set_win_attr(frame_public, win_attr)
    Frame           frame_public;
    WM_Win_Type    *win_attr;
{
    Xv_Drawable_info *info;
    Xv_opaque       server_public;
    Atom            atom = 0;
    Atom            check_atom = 0;
    int		    new_format;

    DRAWABLE_INFO_MACRO(frame_public, info);
    server_public = xv_server(info);
    atom = (Atom) xv_get(server_public, SERVER_WM_WIN_ATTR);
    check_atom = (Atom) xv_get(server_public, SERVER_ATOM, "_SUN_OL_WIN_ATTR_5");

    /*
     * Check if olwm version supports 3 or 5 length OL_WIN_ATTR property
     */
    new_format = screen_check_sun_wm_protocols(xv_screen(info), check_atom);

    if (new_format)  {
        XChangeProperty(xv_display(info), xv_xid(info),
			     atom, atom, 32,
			     PropModeReplace, (unsigned char *)win_attr, 5);
    }
    else  {
        WM_Win_Type_Old		old_win_attr;

	/*
	 * If old 3 length format supported, be backwards compat by
	 * using the old struct
	 */
	old_win_attr.win_type = win_attr->win_type;
	old_win_attr.menu_type = win_attr->menu_type; 

	if (win_attr->pin_initial_state == WMPushpinIsOut)  {
	    old_win_attr.pin_initial_state = (Atom)xv_get(server_public, SERVER_WM_PIN_OUT);
	}
	if (win_attr->pin_initial_state == WMPushpinIsIn)  {
	    old_win_attr.pin_initial_state = (Atom)xv_get(server_public, SERVER_WM_PIN_IN);
	}

        XChangeProperty(xv_display(info), xv_xid(info),
			     atom, atom, 32,
			     PropModeReplace, (unsigned char *)&old_win_attr, 3);
    }

    return XV_OK;
}

Xv_private void
wmgr_set_rescale_state(frame_public, state)
    Frame           frame_public;
    int             state;
{

    Xv_Drawable_info *info;
    Xv_opaque       server_public;

    DRAWABLE_INFO_MACRO(frame_public, info);
    server_public = xv_server(info);
    XChangeProperty(xv_display(info), xv_xid(info),
		    xv_get(server_public, SERVER_WM_RESCALE_STATE),
		    XA_INTEGER, 32,
		    PropModeReplace, (unsigned char *)&state, 1);
}
