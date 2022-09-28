#ifndef lint
static  char sccsid[] = "@(#)XCrPFData.c 3.1 92/04/03 Copyr 1990 GROUPE BULL";
#endif

/* Copyright 1990 GROUPE BULL -- See licence conditions in file COPYRIGHT */
/* XCrPFData.c:
 *
 *  XPM2
 *  Create from Data utility for XPM2 file format
 *  Developped by Arnaud Le Hors
 *  $Id: xpm.shar,v 2.5 90/10/17 17:22:35 lehors Exp $
 */

#include "xpmP.h"

int XCreatePixmapFromData(display, visual, d, colormap, data, depth,
			  pixmap_return, width_return, height_return, 
			  pixels_return, npixels_return,
			  colorsymbols, numsymbols, infos)
     Display *display;
     Visual *visual;
     Drawable d;
     Colormap colormap;
     char **data;
     unsigned int depth;
     Pixmap *pixmap_return;
     unsigned int *width_return, *height_return, *npixels_return;
     Pixel **pixels_return;
     ColorSymbol colorsymbols[];
     unsigned int numsymbols;
     XpmInfo *infos;

{ MData *mdata;

  *width_return = *height_return = 0;
  XpmErrorStatus = PixmapSuccess;

  mdata = mdataopen(data);
  if (XpmErrorStatus != PixmapSuccess)
      return(XpmErrorStatus);

  *pixmap_return = CreatePixmap(display, visual, d, colormap, mdata, depth,
				width_return, height_return, 
				pixels_return, npixels_return,
				colorsymbols, numsymbols, infos);
  mclose(mdata);
  return(XpmErrorStatus);
}
