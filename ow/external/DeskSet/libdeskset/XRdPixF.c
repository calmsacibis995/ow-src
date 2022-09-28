#ifndef lint
static  char sccsid[] = "@(#)XRdPixF.c 3.1 92/04/03 Copyr 1990 GROUPE BULL";
#endif

/* Copyright 1990 GROUPE BULL -- See licence conditions in file COPYRIGHT */
/* XRdPixF.c:
 *
 *  XPM2
 *  Read utility for XPM2 file format
 *  Developped by Arnaud Le Hors
 *  $Id: xpm.shar,v 2.5 90/10/17 17:22:35 lehors Exp $
 */

#include "xpmP.h"

int XReadPixmapFile(display, visual, d, colormap, filename, depth, 
		    pixmap_return, width_return, height_return, 
		    pixels_return, npixels_return,
		    colorsymbols, numsymbols, infos)
     Display *display;
     Visual *visual;
     Drawable d;
     Colormap colormap;
     char *filename;
     unsigned int depth;
     Pixmap *pixmap_return;
     unsigned int *width_return, *height_return, *npixels_return;
     Pixel **pixels_return;
     ColorSymbol colorsymbols[];
     unsigned int numsymbols;
     XpmInfo *infos;

{ MData *mdata;
  char buf[BUFSIZ];
  int l, n = 0;

  *pixmap_return = NULL;
  *width_return = *height_return = 0;
  XpmErrorStatus = PixmapSuccess;

  mdata = mreadopen(filename);
  if (XpmErrorStatus != PixmapSuccess)
      return(XpmErrorStatus);

  /* parse the header file
   */
  BOS = '\0';
  EOS = '\n';
  BCMT = ECMT = NULL;
  l = mnextw(mdata, buf);	/* skip the first word */
  l = mnextw(mdata, buf);	/* then get the second word */
  if (l == 4 && !strncmp("XPM2", buf, 4)) {
      l = mnextw(mdata, buf);	/* get the type key word */
 
     /* get infos about this type
       */
      while (DataTypes[n].type && strncmp(DataTypes[n].type, buf, l)) n++;
      if (DataTypes[n].type) {
	  if (infos) {
	      infos->type = (char *) malloc(l+1);
	      strncpy(infos->type, buf, l);
	      infos->type[l] = '\0';
	  }
	  if (n == 0) {		/* natural type */
	      BCMT = DataTypes[n].Bcmt;
	      ECMT = DataTypes[n].Ecmt;
	      mnextstring(mdata);	/* skip the end of headerline */
	      BOS = DataTypes[n].Bos;
	  } else {
	      mnextstring(mdata);	/* skip the end of headerline */
	      BCMT = DataTypes[n].Bcmt;
	      ECMT = DataTypes[n].Ecmt;
	      BOS = DataTypes[n].Bos;
	      mnextstring(mdata);	/* skip the assignment line */
	  }
	  EOS = DataTypes[n].Eos;

	  *pixmap_return = CreatePixmap(display, visual, d, colormap, mdata,
					depth, width_return, height_return,
					pixels_return, npixels_return,
					colorsymbols, numsymbols, infos);
      } else {
	  XpmErrorStatus = PixmapFileInvalid;
      }
  } else {
      XpmErrorStatus = PixmapFileInvalid;
  }
  mclose(mdata);

  return(XpmErrorStatus);
}
