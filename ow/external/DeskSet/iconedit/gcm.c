/*
 * This file is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this file without charge, but are not authorized to
 * license or distribute it to anyone else except as part of a product
 * or program developed by the user.
 * 
 * THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * This file is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS FILE
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even
 * if Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#ifndef lint
static char	sccsid[] = "@(#)gcm.c	1.25 90/07/13 Copyright 1989 Sun Microsystems";
#endif

/*
 * GUIDE colormap segment support functions.
 */

#include <ctype.h>
#include <string.h>
#include <sys/param.h>
#include <xview/xview.h>
#include <xview/cms.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include "gcm.h"

/*
 * Array of color names.  The RGB values of these colors are looked up at
 * initialization time and inserted into a colormap segment.
 */
static char    *Colornames[] = {
	"BG1",
	"BG2",
	"BG3",
	"Highlight",
	GUIDE_COLOR_LIST,
	"Background",
	"Foreground",
	NULL
};

#define	NCOLORS		((sizeof (Colornames) / sizeof (char *)) - 1)
#define OFFSET		(CMS_CONTROL_COLORS + 2)

static Xv_singlecolor	Colordata[NCOLORS];

static int	Bg_index = NCOLORS-2;
static int	Fg_index = NCOLORS-1;

static Cms	create_guide_cms();
static int	default_bg_index();
static int	default_fg_index();
static void	get_rgb_def();

/*
 * Return the name of GUIDE's colormap segment.
 */
char	       *
gcm_colormap_name()
{
	return "GUIDE CMS";
}

/*
 * Initialize the application's colormap segment data.
 */
void
gcm_initialize_colors(win, bg, fg)
	Xv_opaque       win;			/* base window */
	char           *bg;			/* initial bg | NULL */
	char           *fg;			/* initial fg | NULL */
{
	int             new_bg;
	int		new_fg;
	Cms		cms;
	Xv_Screen	screen = (Xv_Screen) XV_SCREEN_FROM_WINDOW(win);

	cms = (Cms) xv_find(screen, CMS,
		CMS_NAME,	gcm_colormap_name(),
		XV_AUTO_CREATE,	FALSE,
		0);

	if(!cms)
		cms = create_guide_cms(win);

	/*
	 * If a default background or foreground color isn't specified, we need
	 * to use the default from the old cms before installing the new cms.
	 */
	if (!bg || ((new_bg = gcm_color_index(bg)) == -1))
		new_bg = default_bg_index(win);

	if (!fg || ((new_fg = gcm_color_index(fg)) == -1))
		new_fg = default_fg_index(win);

	/*
	 * Install the new cms data.
	 */
	xv_set(win,
		WIN_CMS_NAME,	gcm_colormap_name(),
		WIN_CMS,	cms,
		0);
	
	xv_set(win, WIN_BACKGROUND_COLOR, new_bg, 0);
	xv_set(win, WIN_FOREGROUND_COLOR, new_fg, 0);
}

/*
 * Return the color index for the specified color.  Returns -1 if the color
 * is not found.
 */
int
gcm_color_index(name)
	char           *name;
{
	register int    i;
	static int	strcasecmp();

	if (name && (*name != '\0')) {
		for (i = 0; Colornames[i]; i++)
			if (strcasecmp(Colornames[i], name) == 0)
				return i;
	}
	return -1;
}

/*
 * Create a new CMS for a window.  The resulting CMS will be an OPEN LOOK
 * CONTROL_CMS with four slots containing the necessary colors for 3D.
 * CMS is layed out as follows:
 *
 * 0-3			OPEN LOOK CONTROL_CMS colors
 * 4-(NCOLORS-3)	GUIDE Colors from X11R3 database
 * NCOLORS-2		Background from initial frame
 * NCOLORS-1		Foreground from initial frame
 */
static Cms
create_guide_cms(win)
	Xv_opaque	win;
{
	int		i;
	Cms		cms;
	Xv_opaque	frame;
	XColor		db_val;
	XColor		hw_val;
	Xv_cmsdata	*ocms_data;
	Display		*display = (Display *) XV_DISPLAY_FROM_WINDOW(win);
	Colormap	cmap;

	frame = (Xv_opaque) xv_get(win, WIN_FRAME);
        cmap = xv_get( xv_get( frame, WIN_CMS ), CMS_CMAP_ID );

	/*
	 * Create a new GUIDE CMS.
	 */
	for (i = 0; i < NCOLORS - OFFSET; i++) {
		XLookupColor(display, cmap, Colornames[i+CMS_CONTROL_COLORS],
			     &db_val, &hw_val);

		Colordata[i].red = (hw_val.red >> 8);
		Colordata[i].green = (hw_val.green >> 8);
		Colordata[i].blue = (hw_val.blue >> 8);
	}

	ocms_data = (Xv_cmsdata *) xv_get(frame, WIN_CMS_DATA);

	i = (int) xv_get(frame, WIN_BACKGROUND_COLOR);

	Colordata[Bg_index - CMS_CONTROL_COLORS].red = ocms_data->red[i];
	Colordata[Bg_index - CMS_CONTROL_COLORS].green = ocms_data->green[i];
	Colordata[Bg_index - CMS_CONTROL_COLORS].blue = ocms_data->blue[i];

	i = (int) xv_get(frame, WIN_FOREGROUND_COLOR);

	Colordata[Fg_index - CMS_CONTROL_COLORS].red = ocms_data->red[i];
	Colordata[Fg_index - CMS_CONTROL_COLORS].green = ocms_data->green[i];
	Colordata[Fg_index - CMS_CONTROL_COLORS].blue = ocms_data->blue[i];

	cms = xv_create(XV_NULL, CMS,
		CMS_NAME,		gcm_colormap_name(),
		CMS_TYPE,		XV_STATIC_CMS,
		CMS_CONTROL_CMS,	TRUE,
		CMS_SIZE,		NCOLORS,
		CMS_COLORS,		Colordata,
		0);

	/*
	 * Save color data after cms has been created.
	 */
	xv_get(cms, CMS_COLORS, Colordata);

	return cms;
}

/*
 * Figure out the index of the default background color for a window
 * before a new cms is installed.
 */
static int
default_bg_index(win)
	Xv_opaque	win;
{
	int		i;
	Xv_singlecolor	rgb_def;

	get_rgb_def(win, (int) xv_get(win, WIN_BACKGROUND_COLOR), &rgb_def);

	if ((rgb_def.red == Colordata[0].red) &&
	    (rgb_def.green == Colordata[0].green) &&
	    (rgb_def.blue == Colordata[0].blue))
		i =  0;
	else if ((rgb_def.red == Colordata[Bg_index].red) &&
	    (rgb_def.green == Colordata[Bg_index].green) &&
	    (rgb_def.blue == Colordata[Bg_index].blue))
		i =  Bg_index;
	else
		i =  gcm_color_index("White");

	return i;
}

/*
 * Figure out the index of the default foreground color for a window
 * before a new cms is installed.
 */
static int
default_fg_index(win)
	Xv_opaque	win;
{
	int		i;
	Xv_singlecolor	rgb_def;

	get_rgb_def(win, (int) xv_get(win, WIN_FOREGROUND_COLOR), &rgb_def);

	if ((rgb_def.red == Colordata[Fg_index].red) &&
	    (rgb_def.green == Colordata[Fg_index].green) &&
	    (rgb_def.blue == Colordata[Fg_index].blue))
		i =  Fg_index;
	else
		i =  gcm_color_index("Black");

	return i;
}

/*
 * Get the RGB values for a given color index in the current window cms
 */
static void
get_rgb_def(win, i, color)
	Xv_opaque	win;
	int		i;
	Xv_singlecolor	*color;
{
	XColor		color_cell;
	Xv_cmsdata	*cms_data;

	cms_data = (Xv_cmsdata *) xv_get(win, WIN_CMS_DATA);

	color->red = cms_data->red[i];
	color->green = cms_data->green[i];
	color->blue = cms_data->blue[i];
}

/*
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * Local definition of strcasecmp for binary compability between SunOS 4.0
 * and SunOS 4.1.  We have to include this here because 4.1 before FCS
 * did not contain a compatibility routine for stricmp.
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */

/*
 * This array is designed for mapping upper and lower case letter
 * together for a case independent comparison.  The mappings are
 * based upon ascii character sequences.
 */
static char charmap[] = {
	'\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
	'\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
	'\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
	'\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
	'\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
	'\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
	'\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
	'\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
	'\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
	'\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
	'\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
	'\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
	'\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
	'\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
	'\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
	'\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
	'\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
	'\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
	'\300', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\333', '\334', '\335', '\336', '\337',
	'\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

static int
strcasecmp(s1, s2)
	register char *s1, *s2;
{
	register char *cm = charmap;

	while (cm[*s1] == cm[*s2++])
		if (*s1++ == '\0')
			return(0);
	return(cm[*s1] - cm[*--s2]);
}
