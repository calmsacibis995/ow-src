
#ifndef lint
static char sccsid[] = "@(#)imageinfoui.c 1.9 93/04/15";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

/*
 * imageinfoui.c - User interface object initialization functions
 * for image info pop up.
 * This file was generated by `gxv' from `imagetool.G'.
 */

#include <sys/param.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include "image.h"
#include "imagetool.h"
#include "display.h"
#include "ui_imagetool.h"

/*
 * Initialize an instance of object `attributes'.
 */
ImageInfoObjects *
ImageInfoObjectsInitialize(ip, owner)
	ImageInfoObjects	*ip;
	Xv_opaque	owner;
{
	int	longest_width;
	int	height_width;
	int	width_width;
	int	colors_width;
	int	size_width;
	int	format_width;
	
	if (!ip && 
	    !(ip = (ImageInfoObjects *) calloc(1, sizeof (ImageInfoObjects))))
		return (ImageInfoObjects *) NULL;
	if (!ip->imageinfo)
		ip->imageinfo = imageinfo_imageinfo_create(ip, owner);
	if (!ip->controls)
		ip->controls = imageinfo_controls_create(ip, ip->imageinfo);

	if (!ip->height_label)
		ip->height_label = imageinfo_height_label_create(ip, 
							ip->controls);
	if (!ip->height)
		ip->height = imageinfo_height_create(ip, ip->controls);
	longest_width = xv_get (ip->height_label, XV_WIDTH);
	height_width = longest_width;

	if (!ip->width_label)
		ip->width_label = imageinfo_width_label_create(ip, 
							ip->controls);
	if (!ip->width)
		ip->width = imageinfo_width_create(ip, ip->controls);
	width_width = xv_get (ip->width_label, XV_WIDTH);
	longest_width = Max (longest_width, width_width);

	if (!ip->colors_label)
		ip->colors_label = imageinfo_colors_label_create(ip, 
							ip->controls);
	if (!ip->colors)
		ip->colors = imageinfo_colors_create(ip, ip->controls);
	colors_width = xv_get (ip->colors_label, XV_WIDTH);
	longest_width = Max (longest_width, colors_width);

	if (!ip->size_label)
		ip->size_label = imageinfo_size_label_create(ip, ip->controls);
	if (!ip->size)
		ip->size = imageinfo_size_create(ip, ip->controls);
	size_width = xv_get (ip->size_label, XV_WIDTH);
	longest_width = Max (longest_width, size_width);

	if (!ip->format_label)
		ip->format_label = imageinfo_format_label_create(ip, 
							ip->controls);
	if (!ip->format)
		ip->format = imageinfo_format_create(ip, ip->controls);
	format_width = xv_get (ip->format_label, XV_WIDTH);
	longest_width = Max (longest_width, format_width);

	xv_set (ip->height_label, XV_X, longest_width - height_width + 20, 
				  NULL);
	xv_set (ip->height, XV_X, longest_width + 25, NULL);

	xv_set (ip->width_label, XV_X, longest_width - width_width + 20, NULL);
	xv_set (ip->width, XV_X, longest_width + 25, NULL);

	xv_set (ip->colors_label, XV_X, longest_width - colors_width + 20, 
				  NULL);
	xv_set (ip->colors, XV_X, longest_width + 25, NULL);

	xv_set (ip->size_label, XV_X, longest_width - size_width + 20, NULL);
	xv_set (ip->size, XV_X, longest_width + 25, NULL);

	xv_set (ip->format_label, XV_X, longest_width - format_width + 20, 
				  NULL);
	xv_set (ip->format, XV_X, longest_width + 25, NULL);
				
	return ip;
}

/*
 * Create object `imageinfo' in the specified instance.
 */
Xv_opaque
imageinfo_imageinfo_create(ip, owner)
	ImageInfoObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, FRAME_CMD,
		XV_KEY_DATA, INSTANCE, ip,
		XV_LABEL, LGET( "Image Tool:  Image Information" ),
		XV_SHOW, FALSE,
		FRAME_SHOW_FOOTER, FALSE,
		FRAME_SHOW_RESIZE_CORNER, FALSE,
		FRAME_CMD_PUSHPIN_IN, FALSE,
		NULL);
	xv_set(xv_get(obj, FRAME_CMD_PANEL), WIN_SHOW, FALSE, NULL); 
	return obj;
}

/*
 * Create object `controls' in the specified instance.
 */
Xv_opaque
imageinfo_controls_create(ip, owner)
	ImageInfoObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL, 
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 0,
		XV_Y, 0,
		WIN_BORDER, FALSE,
		XV_HELP_DATA, "imagetool:ImageInfoWindow",
		NULL);
	return obj;
}

/*
 * Create object `height_label' in the specified instance.
 */
Xv_opaque
imageinfo_height_label_create(ip, owner)
	ImageInfoObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 0,
		XV_Y, 25,
		PANEL_LABEL_STRING, LGET ("Height:"),
		PANEL_LABEL_BOLD, TRUE,
                XV_HELP_DATA, "imagetool:ImageInfoHeight",
		NULL);
	return obj;
}

/*
 * Create object `height' in the specified instance.
 */
Xv_opaque
imageinfo_height_create(ip, owner)
	ImageInfoObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, xv_get (ip->height_label, XV_WIDTH) + 5,
		XV_Y, xv_get( ip->height_label, XV_Y ),
		PANEL_LABEL_STRING, LGET ("640"),
		PANEL_LABEL_BOLD, FALSE,
                XV_HELP_DATA, "imagetool:ImageInfoHeight",
		NULL);
	return obj;
}

/*
 * Create object `width_label' in the specified instance.
 */
Xv_opaque
imageinfo_width_label_create(ip, owner)
	ImageInfoObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 0,
		XV_Y, xv_get (ip->height_label, XV_Y) +
		      xv_get (ip->height_label, XV_HEIGHT) + 20,
		PANEL_LABEL_STRING, LGET ("Width:"),
		PANEL_LABEL_BOLD, TRUE,
                XV_HELP_DATA, "imagetool:ImageInfoWidth",
		NULL);
	return obj;
}

/*
 * Create object `width' in the specified instance.
 */
Xv_opaque
imageinfo_width_create(ip, owner)
	ImageInfoObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, xv_get (ip->width_label, XV_WIDTH) + 5,
		XV_Y, xv_get( ip->width_label, XV_Y ),
		PANEL_LABEL_STRING, LGET ("320"),
		PANEL_LABEL_BOLD, FALSE,
                XV_HELP_DATA, "imagetool:ImageInfoWidth",
		NULL);
	return obj;
}

/*
 * Create object `colors_label' in the specified instance.
 */
Xv_opaque
imageinfo_colors_label_create(ip, owner)
	ImageInfoObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 0,
		XV_Y, xv_get( ip->width_label, XV_Y ) +
		      xv_get( ip->width_label, XV_HEIGHT ) + 30,
		PANEL_LABEL_STRING, LGET ("Colors:"),
		PANEL_LABEL_BOLD, TRUE,
                XV_HELP_DATA, "imagetool:ImageInfoColors",
		NULL);
	return obj;
}

/*
 * Create object `colors' in the specified instance.
 */
Xv_opaque
imageinfo_colors_create(ip, owner)
	ImageInfoObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, xv_get (ip->colors_label, XV_WIDTH) + 5,
		XV_Y, xv_get( ip->colors_label, XV_Y ),
		PANEL_LABEL_STRING, LGET ("256 Colors"),
		PANEL_LABEL_BOLD, FALSE,
                XV_HELP_DATA, "imagetool:ImageInfoColors",
		NULL);
	return obj;
}


/*
 * Create object `size_label' in the specified instance.
 */
Xv_opaque
imageinfo_size_label_create(ip, owner)
	ImageInfoObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 0,
		XV_Y, xv_get( ip->colors, XV_Y ) +
		      xv_get( ip->colors, XV_HEIGHT ) + 20,
		PANEL_LABEL_STRING, LGET ("File Size:"),
		PANEL_LABEL_BOLD, TRUE,
                XV_HELP_DATA, "imagetool:ImageInfoSize",
		NULL);
	return obj;
}

/*
 * Create object `size' in the specified instance.
 */
Xv_opaque
imageinfo_size_create(ip, owner)
	ImageInfoObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, xv_get (ip->size_label, XV_WIDTH) + 5,
		XV_Y, xv_get( ip->size_label, XV_Y ),
		PANEL_LABEL_STRING, LGET ("1.2 mgs"),
		PANEL_LABEL_BOLD, FALSE,
                XV_HELP_DATA, "imagetool:ImageInfoSize",
		NULL);
	return obj;
}

/*
 * Create object `format_label' in the specified instance.
 */
Xv_opaque
imageinfo_format_label_create(ip, owner)
	ImageInfoObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, 0,
		XV_Y, xv_get( ip->size_label, XV_Y ) +
		      xv_get( ip->size_label, XV_HEIGHT ) + 20,
		PANEL_LABEL_STRING, LGET ("File Format:"),
		PANEL_LABEL_BOLD, TRUE,
                XV_HELP_DATA, "imagetool:ImageInfoFormat",
		NULL);
	return obj;
}

/*
 * Create object `format' in the specified instance.
 */
Xv_opaque
imageinfo_format_create(ip, owner)
	ImageInfoObjects	*ip;
	Xv_opaque	owner;
{
	Xv_opaque	obj;
	
	obj = xv_create(owner, PANEL_MESSAGE,
		XV_KEY_DATA, INSTANCE, ip,
		XV_X, xv_get (ip->format_label, XV_WIDTH) + 5,
		XV_Y, xv_get( ip->format_label, XV_Y ),
		PANEL_LABEL_STRING, LGET ("Sun Raster"),
		PANEL_LABEL_BOLD, FALSE,
                XV_HELP_DATA, "imagetool:ImageInfoFormat",
		NULL);
	return obj;
}

void
update_imageinfo(image)
ImageInfo	*image;
{
    char  	s[256];
    int		longest_width;
    int		height_width;
    int		width_width;
    int		colors_width;
    int		size_width;
    int		format_width;
 
    if (image == (ImageInfo *) NULL)
       return;
 
    sprintf ( s, LGET ("%.1f in / %.1f cm / %d pix"), 
		(float) image->height / current_display->res_y, 
		(float) image->height / current_display->res_y * 2.54,
		image->height);
    xv_set ( imageinfo->height, PANEL_LABEL_STRING, s, NULL );
    height_width = xv_get (imageinfo->height, XV_WIDTH);
    longest_width = height_width;

    sprintf ( s, LGET ("%.1f in / %.1f cm / %d pix"), 
		(float) image->width / current_display->res_x, 
		(float) image->width / current_display->res_x * 2.54, image->width);
    xv_set ( imageinfo->width, PANEL_LABEL_STRING, s, NULL );
    width_width = xv_get (imageinfo->width, XV_WIDTH);
    longest_width = Max (longest_width, width_width);

    if (image->colors != 0)
       sprintf ( s, LGET ("%d colors"), image->colors);
    else if (image->depth >= 24)
       sprintf ( s, LGET ("Millions of colors"));
    else 
       sprintf ( s, LGET ("N/A"));

    xv_set ( imageinfo->colors, PANEL_LABEL_STRING, s, NULL );
    colors_width = xv_get (imageinfo->colors, XV_WIDTH);
    longest_width = Max (longest_width, colors_width);

    xv_set ( imageinfo->format,
               PANEL_LABEL_STRING, (image->type_info)->popup_list_name,
               NULL );
    format_width = xv_get (imageinfo->format, XV_WIDTH);
    longest_width = Max (longest_width, format_width);

    if (image->file_size / 1000000 > 0)
       sprintf( s, LGET ("%.1f MBytes"), 
		   (float) image->file_size / 1000000.0);
    else if (image->file_size / 1000 > 0)
       sprintf( s, LGET ("%.1f KBytes"), 
		   (float) image->file_size / 1000.0);
    else
       sprintf( s, LGET ("%d Bytes"), image->file_size );
    xv_set ( imageinfo->size, PANEL_LABEL_STRING, s, NULL );
    size_width = xv_get (imageinfo->size, XV_WIDTH);
    longest_width = Max (longest_width, size_width);

    xv_set (imageinfo->controls, 
		XV_WIDTH,  xv_get (imageinfo->height, XV_X) +
			   longest_width + 25,
		XV_HEIGHT, xv_get (imageinfo->format, XV_Y) +
		    	   xv_get (imageinfo->format, XV_HEIGHT) + 25,
		NULL);

    xv_set (imageinfo->imageinfo, 
		XV_WIDTH,  xv_get (imageinfo->height, XV_X) +
			   longest_width + 25,
		XV_HEIGHT, xv_get (imageinfo->format, XV_Y) +
		    	   xv_get (imageinfo->format, XV_HEIGHT) + 25,
		NULL);
}
