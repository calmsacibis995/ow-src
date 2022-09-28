#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)csr_init.c 20.32 93/07/08";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Character screen initialization and cleanup routines.
 */

#include <xview_private/i18n_impl.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <signal.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <xview/pkg.h>
#include <xview/attrol.h>
#include <xview/rect.h>
#include <xview/rectlist.h>
#include <xview/pixwin.h>
#include <xview/win_struct.h>
#include <xview/win_input.h>
#include <xview/window.h>
#include <xview/font.h>
#include <xview_private/charimage.h>
#include <xview_private/charscreen.h>
#ifdef OW_I18N
#include <xview_private/term_impl.h>
#endif /* OW_I18N */
	
Xv_Window       csr_pixwin;

/*
 * Character screen initialization
 */
Pkg_private int
wininit(win, maximagewidth, maximageheight)
    Xv_object       win;
    int            *maximagewidth, *maximageheight;
{
    struct inputmask im;
    struct rect     rect;
    struct rect    *prect;
    Xv_Window       rootwindow;

    /*
     * Set input masks
     */
    (void) win_getinputmask(win, &im, 0);
    im.im_flags |= IM_ASCII;
    im.im_flags |= IM_META;
    im.im_flags |= IM_NEGEVENT;
    win_setinputcodebit(&im, KBD_USE);
    win_setinputcodebit(&im, KBD_DONE);
    win_setinputcodebit(&im, MS_LEFT);
    win_setinputcodebit(&im, MS_MIDDLE);
    win_setinputcodebit(&im, MS_RIGHT);
    win_setinputcodebit(&im, WIN_VISIBILITY_NOTIFY);
    win_unsetinputcodebit(&im, LOC_WINENTER);
    win_unsetinputcodebit(&im, LOC_WINEXIT);

    win_setinputcodebit(&im, LOC_DRAG);
    (void) win_setinputmask(win, &im, 0, 0);
    /*
     * pixwin should be set in tty_init
     */
    /*
     * Setup max image sizes.
     */
    rootwindow = (Xv_Window) xv_get(csr_pixwin, XV_ROOT);
    prect = (struct rect *) xv_get(rootwindow, WIN_RECT);
    rect = *prect;
    *maximagewidth = rect.r_width - chrleftmargin;
    if (*maximagewidth < 0)
	*maximagewidth = 0;
    *maximageheight = rect.r_height;
    /*
     * Determine sizes
     */
    (void) win_getsize(win, &rect);
    winwidthp = rect.r_width;
    winheightp = rect.r_height;

    return (1);
}

Pkg_private void
xv_new_tty_chr_font(font)
#ifdef OW_I18N
    Xv_opaque	font;
#else
    Pixfont    	*font;
#endif
{
#ifdef OW_I18N
        XRectangle      overall_ink_extents, overall_logical_extents;
	XFontSet	font_set = (XFontSet)xv_get(font, FONT_SET_ID);
        Ttysw_folio		ttysw_folio;
        XVaNestedList		va_nested_list;

	pixfont = (Pixfont *)font;   /* BUG ALERT: Remove all these globals! */
	/*
	** Alpha ONLY!!!
	** We want a better mechanism for
	** getting this info from XmbTextExtents
	*/
        XmbTextExtents(font_set, "m", 1,
			&overall_ink_extents, &overall_logical_extents);
	/*
	** Alpha ONLY!!!
	** Horrible hack that fixes single width
	** character display ONLY!  Need a more
	** general screen column management scheme!
	*/
	chrwidth = overall_logical_extents.width;
        chrheight = overall_logical_extents.height;
        chrbase = -overall_logical_extents.y;

	/*
	 * Set XNLineSpace for overTheSpot support.
	 */
        if (csr_pixwin) { 
            ttysw_folio = TTY_PRIVATE_FROM_ANY_VIEW(csr_pixwin);
            if (ttysw_folio->ic && (ttysw_folio->xim_style & 
	    (XIMPreeditArea | XIMPreeditPosition | XIMPreeditNothing))) {  
                va_nested_list = XVaCreateNestedList(NULL, 
				 XNLineSpace, chrheight,
				 NULL);
                XSetICValues(ttysw_folio->ic, 
            		     XNPreeditAttributes, va_nested_list,
        	             NULL);
                XFree(va_nested_list);	         
            }
        }
        
#else
    int		    max_char_height;
    int		    percent;
    int		    spacing;
    XFontStruct	   *x_font_info;

    pixfont = font;		/* BUG ALERT: Remove all these globals! */
    x_font_info = (XFontStruct *)xv_get((Xv_opaque)font, FONT_INFO);
    chrwidth = xv_get((Xv_opaque)font, FONT_DEFAULT_CHAR_WIDTH);

    percent = defaults_get_integer("text.lineSpacing", "Text.LineSpacing", 0);
    if (percent > 0) {
	max_char_height = x_font_info->max_bounds.ascent +
	    x_font_info->max_bounds.descent;
	spacing = max_char_height*percent/100;
	if ((max_char_height*percent)%100 > 0 || spacing == 0)
	    spacing++;  /* round up, or enforce a minimum of 1 pixel */
	chrheight = max_char_height + spacing;
    } else {
	chrheight = (int) xv_get((Xv_opaque)font, FONT_DEFAULT_CHAR_HEIGHT);
    }

    chrbase = x_font_info->ascent;
#endif
}
