#ifndef lint
static char sccsid[] = "@(#)dither.c 1.55 94/03/25";
#endif

/*  Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 *
 * ------------------------------------------------------------------------
 *  Copyright 1989, 1990 Kirk L. Johnson
 *  Copyright 1989, 1990 Jim Frost and Steve Losen.
 *
 *  Permission to use, copy, modify, distribute, and sell this
 *  software and its documentation for any purpose is hereby granted
 *  without fee, provided that the above copyright notice appear in
 *  all copies and that both that copyright notice and this
 *  permission notice appear in supporting documentation. The
 *  author makes no representations about the suitability of this
 *  software for any purpose. It is provided "as is" without express
 *  or implied warranty.
 *
 *  THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 *  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT
 *  OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 *  LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 *  NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 *  CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "imagetool.h"
#include "display.h"
#include "image.h"

#define NCOLORS 	216
#define THRESHOLD       100

XColor white_color = {0, 65535, 65535, 65535};
XColor black_color = {0, 0, 0, 0};
XColor background_color = {0, 0, 0, 0};
/*
XColor background_color = {0, (230 << 8) + 230, (230 << 8) + 230, (230 << 8) + 230};
*/

static XilColorspace  rgb_colorspace = NULL;
static XilColorspace  y_colorspace = NULL;

void
swap_red_blue (image, image_data, pixel_stride, scanline_stride)
ImageInfo	*image;
unsigned char	*image_data;
unsigned int     pixel_stride;
unsigned int     scanline_stride;
{
    int			 len, i;
    int			 lineb = scanline_stride;
    unsigned char	*lineptr = image_data;
    unsigned char	 tmp;

    for (i = 0; i < image->height ; i++) {
      len = lineb;
      while (len >= pixel_stride) {
	tmp = lineptr[0];
	lineptr[0] = lineptr[2];
	lineptr[2] = tmp;
	lineptr += pixel_stride;
	len -= pixel_stride;
      }
      lineptr += len;
    }

    /* toggle rgborder */
    image->rgborder = !image->rgborder;
}

static unsigned long
add_color (colors, ncolors, color, pixel)
    XColor     *colors;
    int	        ncolors;
    XColor     *color;
    unsigned long  pixel;
{
    int   i;

    for (i = ncolors; i > 0; i--) 
      colors[i] = colors[i-1];
    
    colors[0].red = color->red;
    colors[0].green = color->green;
    colors[0].blue = color->blue;
    colors[0].pixel = pixel;
    colors[0].flags = DoRed | DoGreen | DoBlue;

    return (pixel);
}

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/*
 * Find the closest match to color from the list of colors.
 */
unsigned long
closest_match( color, colors, ncolors )
  XColor *color;
  XColor *colors;
  int     ncolors;
{
    unsigned long dr, dg, db;
    unsigned long final, minval;
    unsigned long sum, diff;
    int i;

    final  = 0;
    minval = ~0;

    for ( i = 0; i < ncolors; i++ ) {
      dr = dg = db = 0;
      dr = ( int )( colors[i].red - color->red );
      dg = ( int )( colors[i].green - color->green );
      db = ( int )( colors[i].blue - color->blue );
      diff = dr * dr;
      sum = diff + dg * dg;
      if ( sum < diff )
        continue;
      diff = sum + db * db;
      if ( ( diff >= sum ) && ( diff < minval ) ) {
        final = i;
        minval = diff;
      }
    }
 
    return ( colors[final].pixel );

}

/*
 * BGR values for 24-bit images.
 */
void
set_canvas_values()
{
      image_display->background[0] = (float) 0.0;
      image_display->background[1] = (float) 0.0;
      image_display->background[2] = (float) 0.0;

      image_display->white[0] = (float) 255.0;
      image_display->white[1] = (float) 255.0;
      image_display->white[2] = (float) 255.0;

      ps_display->background[0] = (float) 230.0;
      ps_display->background[1] = (float) 230.0;
      ps_display->background[2] = (float) 230.0;

      ps_display->white[0] = (float) 255.0;
      ps_display->white[1] = (float) 255.0;
      ps_display->white[2] = (float) 255.0;
}

void
set_canvas_colors (image, colors, depth, offset, ncolors)
    ImageInfo   *image;
    XColor      *colors;
    int          depth;
    int         *offset;
    int         *ncolors;
{
    int	         white_pixel_found, black_pixel_found, background_found;
    int          maxcolors, i;

    maxcolors = depth == 4 ? 16 : 256;

    background_found = FALSE;
    white_pixel_found = FALSE;
    black_pixel_found = FALSE;

/*
 * Stuff the bg, black, white colors into display.
 * Note:  white[3] contains the white pixel value needed in
 *        the print preview drawing lines to set the gc_vals.
 */
    for (i = 0; i < image->colors; i++) {

	if ((colors[i].red == background_color.red) &&
	    (colors[i].green == background_color.green) &&
	    (colors[i].blue == background_color.blue)) {
          image_display->background[0] = (float) colors[i].pixel;
          ps_display->background[0] = (float) colors[i].pixel;
	  background_found = TRUE;
        }
	else if ((colors[i].red == black_color.red) &&
	         (colors[i].green == black_color.green) &&
	         (colors[i].blue == black_color.blue)) {
          image_display->black = colors[i].pixel;
          ps_display->black = colors[i].pixel;
	  black_pixel_found = TRUE;
        }
        else if ((colors[i].red == white_color.red) &&
                 (colors[i].green == white_color.green) &&
                 (colors[i].blue == white_color.blue)) {
          image_display->white[0] = (float) colors[i].pixel;
          ps_display->white[0] = (float) colors[i].pixel;
	  image_display->white[3] = (float) colors[i].pixel;
	  ps_display->white[3] = (float) colors[i].pixel;
	  white_pixel_found = TRUE;
        }
    }

/*
 * Stuff these colors at the beginning if they weren't found.
 */
    *ncolors = image->colors;
    *offset = image->offset;
    if (!background_found && *offset > 0) {
      if (*ncolors < maxcolors) 
        image_display->background[0] = (float) add_color (colors, (*ncolors)++, 
						          &background_color, --(*offset));
      else
        image_display->background[0] = (float) closest_match (&background_color, 
							      colors, *ncolors);
      ps_display->background[0] = image_display->background[0];
    }
    if (!black_pixel_found && *offset > 0)
      if (*ncolors < maxcolors) 
        image_display->black = add_color (colors, (*ncolors)++, &black_color, --(*offset));
      else
        image_display->black = closest_match (&black_color, colors, *ncolors);

    if (!white_pixel_found) {
      if (*ncolors < maxcolors && *offset > 0) 
        image_display->white[0] = (float) add_color (colors, (*ncolors)++, 
						     &white_color, --(*offset));
      else
        image_display->white[0] = closest_match (&white_color, colors, *ncolors);

      image_display->white[3] = image_display->white[0];
      ps_display->white[0] = image_display->white[0];
      ps_display->white[3] = ps_display->white[0];
    }

    if (current_display->depth == 24) 
      set_canvas_values();
}

void
copy_default_cmap (xdpy, cmap, ncolors)
    Display    *xdpy;
    Colormap    cmap;
    int         ncolors;
{
    int     i;
    XColor  colors[256];

    for (i = 0; i < ncolors; i++)
      colors[i].pixel = i;

    XQueryColors (xdpy, DefaultColormap (xdpy, DefaultScreen (xdpy)), colors, 
		  ncolors);

    XStoreColors (xdpy, cmap, colors, ncolors);
}

int
set_cmap8 (xdpy, xid, image, depth)
    Display    *xdpy;
    Window      xid;
    ImageInfo  *image;
    int         depth;
{
    XVisualInfo    vinfo;
    unsigned long  pixels[MAXCOLORS], pmask[1], pixel[1];
    int            i, j, k, index, ncolors, offset;
    XColor 	   colors[MAXCOLORS];

    /* Find PseudoColor Visual */
    if ((XMatchVisualInfo (xdpy, DefaultScreen(xdpy), depth, image_display->visual->class, 
			   &vinfo)) == 0) {
      if (prog->verbose)
	fprintf (stderr, 
		 MGET("%s: Could not get %d-bit visual in set_cmap8.\n"), 
		 prog->name, depth);
      return (0);
    }
   
    /* Create X color map */
    image->cmap = XCreateColormap(xdpy, RootWindow(xdpy, DefaultScreen (xdpy)),
				  vinfo.visual, AllocNone);
    i = XAllocColorCells(xdpy, image->cmap, 1, pmask, 0, pixels, 
			 depth == 4 ? 16 : MAXCOLORS);

    /* fill the colormap */
    index = 0;

    for (i = 0; i < image->colors; i++) {
    	colors[index].pixel = index + image->offset;  
        colors[index].red = (image->red[index] << 8) + image->red[index];
        colors[index].green = (image->green[index] << 8) + image->green[index];
        colors[index].blue = (image->blue[index] << 8) + image->blue[index];
    	colors[index].flags = DoRed | DoGreen | DoBlue;
    	index++;
    }

    set_canvas_colors (image, colors, depth, &offset, &ncolors); 

    for (pixel[0] = 40; pixel[0] < offset; pixel[0]++) 
      XFreeColors(xdpy, image->cmap, pixel, 1, 0);
    
    copy_default_cmap (xdpy, image->cmap,
        DefaultDepth (xdpy, DefaultScreen (xdpy)) == 4 ? 16 : 40);

    XStoreColors(xdpy, image->cmap, colors, ncolors);

    XSetWindowColormap(xdpy, xid, image->cmap);

    XFlush (xdpy);

    return (image->colors);
}

void 
set_cmap24to8 (xdpy, xid, cmap, image, depth)
    Display       *xdpy;
    Window         xid;
    XilLookup      cmap;
    ImageInfo     *image;
    int            depth;
{
    XVisualInfo	   vinfo;
    unsigned long  pixels[MAXCOLORS], pmask[1], pixel[1];
    int		   index, i, offset, ncolors;
    XColor 	   colors[MAXCOLORS];

/*
 * Find pseudocolor visual
 */
    if (current_display->depth == 8) {
	if ((XMatchVisualInfo (xdpy, DefaultScreen(xdpy), depth, 
	    image_display->visual->class, &vinfo)) == 0) {
	    if (prog->verbose) {
		fprintf (stderr, 
		    MGET("%s: Could not get %d bit visual in set_cmap24to8.\n"),
		    prog->name, depth);
	    }
	    return;
	}
      
	/* Create X color map */
	image->cmap = XCreateColormap(xdpy, RootWindow(xdpy,
		DefaultScreen(xdpy)), vinfo.visual, AllocNone);

	if (XAllocColorCells (xdpy, image->cmap, 1, pmask, 0, pixels, 
	    depth == 4 ? 16: MAXCOLORS) == 0) {
	    if (prog->verbose) {
	      fprintf (stderr, 
		   MGET ("%s: XAllocColorCells failed in set_cmap24to8.\n"), 
		   prog->name);
	    }
	    return;
	}
    }

    /* fill the colormap differently
     * based on whether the image is
     * in rgb or bgr order
     */
    set_cmap_from_lut(image, cmap);

    if (current_display->depth == 8) {
	for (i = index = 0; index < image->colors; index++) {
	    colors[index].pixel = image->offset + index;
	    colors[index].red   = image->red[index]<<8;
	    colors[index].green = image->green[index]<<8;
	    colors[index].blue  = image->blue[index]<<8;
	    colors[index].flags = DoRed | DoGreen | DoBlue;
	}

	set_canvas_colors (image, colors, depth, &offset, &ncolors);
 
	for (pixel[0] = 40; pixel[0] < offset; pixel[0]++) 
	    XFreeColors(xdpy, image->cmap, pixel, 1, 0);
    
	copy_default_cmap (xdpy, image->cmap,
		DefaultDepth (xdpy, DefaultScreen (xdpy)) == 4 ? 16 : 40);

	XStoreColors(xdpy, image->cmap, colors, ncolors);

	XSetWindowColormap (xdpy, xid, image->cmap);
	XFlush (xdpy);
    }
}

void 
set_cmap8to216 (xdpy, xid, cmap, image, depth, cmap_offset)
    Display       *xdpy;
    Window         xid;
    XilLookup      cmap;
    ImageInfo     *image;
    int            depth;
    int            cmap_offset;
{
    XVisualInfo	   vinfo;
    unsigned long  pixels[MAXCOLORS], pmask[1], pixel[1];
    int		   index, i, offset, ncolors;
    Xil_unsigned8  cmap_data[MAXCOLORS*3];
    XColor 	   colors[MAXCOLORS];

/*
 * Find pseudocolor visual
 */
    if (current_display->depth == 8) {
      if ((XMatchVisualInfo (xdpy, DefaultScreen(xdpy), depth, 
			     image_display->visual->class, &vinfo)) == 0) {
	if (prog->verbose)
  	  fprintf (stderr, 
		   MGET ("%s: Could not get %d bit visual in set_cmap24to8.\n"), 
		   prog->name, depth);
	return;
      }
  
      /* Create X color map */
      image->cmap = XCreateColormap(xdpy, RootWindow(xdpy, DefaultScreen (xdpy)),
				    vinfo.visual, AllocNone);

      if (XAllocColorCells (xdpy, image->cmap, 1, pmask, 0, pixels, 
			    depth == 4 ? 16: MAXCOLORS) == 0) {
	if (prog->verbose)
	  fprintf (stderr, 
		   MGET ("%s: XAllocColorCells failed in set_cmap24to8.\n"), 
		   prog->name);
	return;
      }
    }

    set_cmap_from_lut(image, cmap);
    image->offset = 40;

    if (current_display->depth == 8) {

      /* fill the colormap */
      for (i = index = 0; index < image->colors; index++) {
	colors[index].pixel = image->offset + index;
	colors[index].red   = image->red[index]<<8;
	colors[index].green = image->green[index]<<8;
	colors[index].blue  = image->blue[index]<<8;
	colors[index].flags = DoRed | DoGreen | DoBlue;
      }

      set_canvas_colors (image, colors, depth, &offset, &ncolors);
 
      for (pixel[0] = 40; pixel[0] < offset; pixel[0]++) 
	XFreeColors(xdpy, image->cmap, pixel, 1, 0);
    
      copy_default_cmap (xdpy, image->cmap,
         DefaultDepth (xdpy, DefaultScreen (xdpy)) == 4 ? 16 : 40);
      XStoreColors(xdpy, image->cmap, colors, ncolors);

      XSetWindowColormap (xdpy, xid, image->cmap);
      XFlush (xdpy);
      
    }
}

void 
set_cmap4 (xdpy, xid, cmap, image)
    Display       *xdpy;
    Window         xid;
    XilLookup      cmap;
    ImageInfo     *image;
{
    XVisualInfo	   vinfo;
    unsigned long  pixels[MAXCOLORS], pmask[1], pixel[1];
    int		   index, i, offset, ncolors;
    Xil_unsigned8  cmap_data[MAXCOLORS*3];
    XColor 	   colors[MAXCOLORS];

    xil_lookup_get_values (cmap, 0, 16, cmap_data);


    /* Find PseudoColor Visual */
    if ((XMatchVisualInfo (xdpy, DefaultScreen(xdpy), 4,
	image_display->visual->class, &vinfo)) == 0) {
	if (prog->verbose) {
	    fprintf (stderr, 
		MGET ("%s: Could not get 4-bit visual in set_cmap4.\n"), 
		prog->name);
	}
	return;
    }
  
    /* Create X color map */
    image->cmap = XCreateColormap(xdpy, RootWindow(xdpy, DefaultScreen (xdpy)),
				  vinfo.visual, AllocNone);

    if (XAllocColorCells (xdpy, image->cmap, 1, pmask, 0, pixels, 16) == 0) {
	if (prog->verbose) {
	    fprintf (stderr, 
		MGET ("%s: XAllocColorCells failed in set_cmap4.\n"), 
		prog->name);
	}
	return;
    }

    if (image->red != (unsigned char *) NULL) {
	image->red = (unsigned char *) realloc (image->red, image->colors);
	image->green = (unsigned char *) realloc (image->green, image->colors);
	image->blue = (unsigned char *) realloc (image->blue, image->colors);
    } else {
	image->red   = (unsigned char *) malloc(sizeof (unsigned char) *
	    image->colors);
	image->green = (unsigned char *) malloc(sizeof (unsigned char) *
	    image->colors);
	image->blue  = (unsigned char *) malloc(sizeof (unsigned char) *
	    image->colors);
    }

    /* fill the colormap differently
     * based on whether the image is
     * in rgb or bgr order
     */
    set_cmap_from_lut(image, cmap);

    /* fill the colormap */
    for (i = index = 0; index < image->colors; index++) {
	colors[index].pixel = image->offset + index;
	colors[index].red   = image->red[index]<<8;
	colors[index].green = image->green[index]<<8;
	colors[index].blue  = image->blue[index]<<8;
	colors[index].flags = DoRed | DoGreen | DoBlue;
    }

    set_canvas_colors (image, colors, 4, &offset, &ncolors);

    for (pixel[0] = 0; pixel[0] < offset; pixel[0]++) 
      XFreeColors(xdpy, image->cmap, pixel, 1, 0);
    
    XStoreColors(xdpy, image->cmap, colors, ncolors);
    
    XSetWindowColormap (xdpy, xid, image->cmap);
    XFlush (xdpy);
}

static int
set_grayscale_cmap (xdpy, xid, ncolors, image, depth)
    Display    *xdpy;
    Window      xid;
    int         ncolors;
    ImageInfo  *image;
    int         depth;
{
    XVisualInfo	   vinfo;
    unsigned long  pixels[MAXCOLORS], pmask[1], pixel[1];
    int            i, j, k, index, offset, actual_ncolors;
    XColor 	   colors[MAXCOLORS];
    int            maxcolors;

    /* Find PseudoColor Visual */
    if ((XMatchVisualInfo (xdpy, DefaultScreen(xdpy), depth, 
			   image_display->visual->class, &vinfo)) == 0) {
      if (prog->verbose)
    	fprintf (stderr, 
		 MGET ("%s: Could not get %d-bit visual in set_grayscale_cmap.\n"), 
		 prog->name, depth);
      return (0);
    }

    maxcolors = depth == 4 ? 16 : MAXCOLORS;

    /* Create X color map */
    image->cmap = XCreateColormap(xdpy, RootWindow(xdpy, DefaultScreen (xdpy)),
				  vinfo.visual, AllocNone);
    i = XAllocColorCells(xdpy, image->cmap, 1, pmask, 0, pixels, maxcolors);

    /* fill the colormap */
    index = 0;
 
    for (i = ncolors - 1; i >= 0; i-- ) {
    	colors[index].pixel = index + image->offset;
    	colors[index].blue = colors[index].green = colors[index].red = 
	    (255 * i/(ncolors-1)) * MAXCOLORS; 
    	colors[index].flags = DoRed | DoGreen | DoBlue;
    	index++;
    }

    set_canvas_colors (image, colors, depth, &offset, &actual_ncolors);

    if (depth == 8) {
      for (pixel[0] = 40; pixel[0] < offset; pixel[0]++) 
        XFreeColors(xdpy, image->cmap, pixel, 1, 0);
      copy_default_cmap (xdpy, image->cmap,
         DefaultDepth (xdpy, DefaultScreen (xdpy)) == 4 ? 16 : 40);
    }
    else {
      for (pixel[0] = 0; pixel[0] < offset; pixel[0]++) 
        XFreeColors(xdpy, image->cmap, pixel, 1, 0);
    }
    XStoreColors(xdpy, image->cmap, colors, actual_ncolors);

    XSetWindowColormap(xdpy, xid, image->cmap);
    XFlush (xdpy);

    return ncolors;
}

static XilImage
dither8to216 (image)
  ImageInfo *image;
{
  unsigned int          width, height, nbands;
  XilDataType           datatype;
  XilImage              dst, tmp, tmp_image;
  XilLookup             cmap;
  XilLookup		src_lut, dst_lut, tmp_lut;
  XilMemoryStorageByte	storage;
  Xil_unsigned8	        cmap_data[256*3];
  float                 mult[1], offset[1];
  int i, j = 0;

  save_colormap (image);
  image->ditheredto216 = TRUE;
  xil_get_info (image->orig_image, &width, &height, &nbands, &datatype);

/*
 * Copy current image colors into cmap_data.
 */
  for (i = 0; i < image->colors; i++) {
    cmap_data[j] = image->red[i];
    cmap_data[j+1] = image->green[i];
    cmap_data[j+2] = image->blue[i];
    j += 3;
  }
  tmp = xil_create (image_display->state, 256, 1, 3, XIL_BYTE);
  xil_export(tmp);
  xil_get_memory_storage(tmp, (XilMemoryStorage *)&storage);
  storage.data = cmap_data;
  xil_set_memory_storage(tmp, (XilMemoryStorage *)&storage);
  xil_import(tmp, 1);
    
  src_lut = xil_lookup_create (image_display->state, XIL_BYTE, XIL_BYTE, 3,
			       image->colors, 0, cmap_data);
  dst_lut = xil_choose_colormap (tmp, 216);
  tmp_lut = xil_lookup_convert (src_lut, dst_lut); /* 0 - 216 */ 

  xil_lookup_destroy (src_lut);
  xil_destroy (tmp);

  dst = xil_create (image_display->state, width, height, nbands, datatype);
  xil_lookup (image->orig_image, dst, tmp_lut);

  image->colors = xil_lookup_get_num_entries (dst_lut);
  image->offset = MAXCOLORS - image->colors;

  /* use new_image and dst_lut for display */
  set_cmap8to216 (current_display->xdisplay, current_display->win, dst_lut, 
		  image, 8, 0);

  mult[0] = 1.0;
  offset[0] = (float) image->offset;
  tmp_image = xil_create (image_display->state, width, height, 1, XIL_BYTE);
  xil_rescale (dst, tmp_image, mult, offset);
  xil_destroy (dst);

  return (tmp_image);
    
}

XilImage
dither8to4 (image)
  ImageInfo *image;
{
  unsigned int     width, height, nbands;
  XilDataType      datatype;
  XilImage         tmp_image, rgb_image;
  XilImage         grayimage, bit_image;
  static XilLookup cmap;
  Xil_unsigned8    data[256 * 3];
  XilLookup        lookup;
  unsigned int     xor_value = 0x01;
  static XilDitherMask mask = NULL;
  float  	   lowvalue[1], highvalue[1], mapvalue[1];
  int              multipliers[3];
  int              i, j;
  unsigned int     dimensions[3];
  static float     dmask[] = {
         0/16.0, 8/16.0, 2/16.0,10/16.0,
        12/16.0, 4/16.0,14/16.0, 6/16.0,
         3/16.0,11/16.0, 1/16.0, 9/16.0,
        15/16.0, 7/16.0,13/16.0, 5/16.0,

         0/16.0, 8/16.0, 2/16.0,10/16.0,
        12/16.0, 4/16.0,14/16.0, 6/16.0,
         3/16.0,11/16.0, 1/16.0, 9/16.0,
        15/16.0, 7/16.0,13/16.0, 5/16.0,

         0/16.0, 8/16.0, 2/16.0,10/16.0,
        12/16.0, 4/16.0,14/16.0, 6/16.0,
         3/16.0,11/16.0, 1/16.0, 9/16.0,
        15/16.0, 7/16.0,13/16.0, 5/16.0};

    compress_colors (image);    
    save_colormap (image);
    xil_get_info (image->orig_image, &width, &height, &nbands, &datatype);
/*
 * Dither to 1 for now.. fix later to dither to 16 colors in XIL.
 */
    rgb_image = xil_create (image_display->state, width, height, 3, XIL_BYTE);

    for (j = 0, i = 0; i < image->colors; i++, j += 3) {
      data[j]   = (image->blue[i] << 8) + image->blue[i];
      data[j+1] = (image->green[i] << 8) + image->green[i];
      data[j+2] = (image->red[i] << 8) + image->red[i];
    }

    lookup = xil_lookup_create (image_display->state, XIL_BYTE, XIL_BYTE,
			        3, image->colors, 0, data);
    xil_lookup (image->orig_image, rgb_image, lookup);
    xil_lookup_destroy (lookup);

    if (mask == NULL)
      mask = xil_dithermask_create (image_display->state, 4, 4, 3, dmask);

    image->offset = 0;

    if (cmap == NULL) {
      dimensions[0] = 2;  dimensions[1] = 4; dimensions[2] = 2;
      multipliers[0] = 1; multipliers[1] = 2; multipliers[2] = 8;
      cmap = xil_colorcube_create (image_display->state, datatype, datatype, 
		                   3, image->offset, multipliers, dimensions);
    }

    image->colors = xil_lookup_get_num_entries (cmap);

    if (current_display->depth == 4) 
      set_cmap4 (current_display->xdisplay, current_display->win, cmap, image);

    tmp_image = xil_create (image_display->state, width, height, 1, datatype);
    xil_ordered_dither (rgb_image, tmp_image, cmap, mask);
    xil_destroy (rgb_image);

#ifdef TWO_COLORS
    rgb_image = xil_create (image_display->state, width, height, 3, XIL_BYTE);  

    for (j = 0, i = 0; i < image->colors; i++, j += 3) {
      data[j]   = (image->blue[i] << 8) + image->blue[i];
      data[j+1] = (image->green[i] << 8) + image->green[i];
      data[j+2] = (image->red[i] << 8) + image->red[i];
    }

    lookup = xil_lookup_create (image_display->state, XIL_BYTE, XIL_BYTE, 3,
				image->colors, 0, data);
    xil_lookup (image->orig_image, rgb_image, lookup);
  
/*
 * Convert to grayimage.
 */ 
    grayimage = xil_create (image_display->state, width, height, 1, XIL_BYTE);  
    if (rgb_colorspace == NULL)
      rgb_colorspace = xil_colorspace_get_by_name 
                        (image_display->state, DGET ("rgblinear"));
    xil_set_colorspace (rgb_image, rgb_colorspace);

    if (y_colorspace == NULL)
      y_colorspace = xil_colorspace_get_by_name 
                      (image_display->state, DGET ("ylinear"));
    xil_set_colorspace (grayimage, y_colorspace);

    xil_color_convert (rgb_image, grayimage);
    xil_destroy (rgb_image);
/*
 * Do thresholding on grayimage going from 8 to 1.
 */
    bit_image = xil_create (image_display->state, width, height, 1, XIL_BIT); 
    lowvalue[0] = 0.0;
    highvalue[0] = THRESHOLD;
    mapvalue[0] = 0.0;
    xil_threshold (grayimage, grayimage, lowvalue, highvalue, mapvalue);
      
    lowvalue[0] = highvalue[0]+1;
    highvalue[0] = 256.0;
    mapvalue[0] = 1.0;
    xil_threshold (grayimage, grayimage, lowvalue, highvalue, mapvalue);
    xil_cast (grayimage, bit_image);
    xil_destroy (grayimage);
    /* Take this line out when you can use mult to be -1 (xil bug)*/
    xil_xor_const (bit_image, &xor_value, bit_image);
/*
 * Now go from 1 to 8 for displaying.
 */
    image->colors = 2;
    image->offset = 16 - image->colors;

    if (current_display->depth == depth) 
      set_grayscale_cmap (current_display->xdisplay, current_display->win, 
			  image->colors, image, 4);                                 

    tmp_image = xil_create (image_display->state, width, height, 1, XIL_BYTE);
    xil_cast (bit_image, tmp_image);
    xil_destroy (bit_image);
    xil_xor_const (tmp_image, &xor_value, tmp_image); 
#endif

    return (tmp_image);
}

void
set_cmap_from_lut(image, lut)
    ImageInfo *image;
    XilLookup lut;
{
    int i, index;
    Xil_unsigned8  lut_data[MAXCOLORS*3];

    image->offset = xil_lookup_get_offset(lut); 
    image->colors = xil_lookup_get_num_entries(lut);

    xil_lookup_get_values (lut, image->offset, image->colors, lut_data);

    if (image->red != (unsigned char *) NULL) {
	image->red = (unsigned char *) realloc (image->red, image->colors);
	image->green = (unsigned char *) realloc (image->green, image->colors);
	image->blue = (unsigned char *) realloc (image->blue, image->colors);
    } else {
	image->red   = (unsigned char *) malloc(sizeof (unsigned char) *
		image->colors);
	image->green = (unsigned char *) malloc(sizeof (unsigned char) *
		image->colors);
	image->blue  = (unsigned char *) malloc(sizeof (unsigned char) *
		image->colors);
    }

    if (image->rgborder) {
	for (i = index = 0; index < image->colors; index++) {
	    image->red[index]   = lut_data[i++];
	    image->green[index] = lut_data[i++];
	    image->blue[index]  = lut_data[i++];
	}
    } else {
	for (i = index = 0; index < image->colors; index++) {
	    image->blue[index]   = lut_data[i++];
	    image->green[index] = lut_data[i++];
	    image->red[index]  = lut_data[i++];
	}
    }
}
