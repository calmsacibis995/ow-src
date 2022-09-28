/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef	_DS_COLORS_H
#define	_DS_COLORS_H

#ident	"@(#)ds_colors.h	1.1	93/01/07 SMI"

#include <limits.h>
#include <xview/xview.h>
#include <xview/cms.h>

/*
 * Max colors of the user can allocate with the color text file.
 */
#define MAX_DS_COLORS     240

/*
 * Define null cms index entry as unsigned max long, since we cannot use
 * zero as an index.
 */
#define DS_NULL_CMS_INDEX	ULONG_MAX

/*
 * Property name in the server.
 */
#define COLOR_PROP_NAME   "_SUN_DESKSET_COLORS"

/* 
 * Function Declarations
 */
#ifdef __cplusplus
extern "C" {
#endif
extern int		ds_colors_init(Display*, Xv_window);
extern Cms 		ds_cms_create(Xv_window);
extern int 		ds_alloc_colors(Xv_window);
extern unsigned long 	ds_cms_index(Cms, Xv_singlecolor*);
extern unsigned long	ds_x_index(unsigned long);
extern unsigned long	ds_x_pixel(Display*, Colormap, XColor*);
extern int 		ds_set_colormap(Window, Cms,
			    unsigned long, unsigned long);
#ifdef __cplusplus
}
#endif

#endif /* !_DS_COLORS_H */
