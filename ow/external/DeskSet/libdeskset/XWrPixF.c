#ifndef lint
static  char sccsid[] = "@(#)XWrPixF.c 3.1 92/04/03 Copyr 1990 GROUPE BULL";
#endif

/* Copyright 1990 GROUPE BULL -- See licence conditions in file COPYRIGHT */
/*
 *  XPM2
 *  Write utility for XPM2 file format
 *  Developped by Arnaud Le Hors
 *  $Id: xpm.shar,v 2.5 90/10/17 17:22:35 lehors Exp $
 */

#include "xpmP.h"

#ifdef SVR4
#include <string.h>
#else
#include <strings.h>
#endif SVR4

#undef RETURN
#define RETURN(status) \
  { if (index) free(index); \
    if (colorStrings) { \
	for (a = 0; a < ncolors; a++) \
	    if (*(colorStrings + a)) \
		free(*(colorStrings + a)); \
	free(colorStrings); \
    } \
    mclose(mdata); \
    return(XpmErrorStatus = status); }

int XWritePixmapFile(display, colormap, filename, pixmap, width, 
		     height, type, cppm, infos)
     Display *display;
     Colormap colormap;
     char *filename;
     Pixmap *pixmap;
     unsigned int width, height;
     char *type;
     unsigned int cppm;
     XpmInfo *infos;

{ MData *mdata;
  XImage *ximage = NULL;
  unsigned int pixlen;		/* length of pixels */
  unsigned int pixgap = 0;		/* length of pixels gap */
  byte *dptr, *bptr, *destline, *destptr, *bitplane, destmask;
  Pixel *index = NULL;		/* index of different pixels */
  unsigned int indexsize = 256;	/* should be enough most of the time */
  XColor *xcolors;		/* used colors */
  char **colorStrings = NULL;	/* character strings associated to colors */
  unsigned int ncolors = 0;	/* number of colors */
  unsigned int cpp;             /* chars per pixel */
  unsigned int depth, linelen;
  unsigned long plane;
  char *name;
  unsigned int a, b, c, x, y, n = 0, key;
  Pixel pixel;

  XpmErrorStatus = PixmapSuccess;

  mdata = mwriteopen(filename);
  if (XpmErrorStatus != PixmapSuccess)
      return(XpmErrorStatus);

  if (!(name = (char*)strrchr(filename, '/')))
    name = filename;
  else
    name++;

  if (!type)
      if (infos && infos->type)
	  type = infos->type;
      else
	  n = 0;		/* natural type */
  if (n == 0 && type) {
      while (DataTypes[n].type && strcmp(DataTypes[n].type, type)) n++;
      if (! DataTypes[n].type)
	  return(XpmErrorStatus = PixmapFileInvalid);
  }


  /*
   * read the image data
   */

  ximage = XGetImage(display, (Drawable) pixmap, 0, 0, width, height, AllPlanes, ZPixmap);
  pixlen = (ximage->bits_per_pixel / 8) + (ximage->bits_per_pixel % 8 ? 1 : 0);
  depth = ximage->depth;

  /* index the different pixels
   */
  if (! (index = (Pixel *)calloc(sizeof(Pixel) * indexsize, 1)))
      return(XpmErrorStatus = PixmapNoMemory);
  if (ximage->bits_per_pixel % 8) {
/*
      linelen = (width / 8) + (width % 8 ? 1 : 0);
*/
      linelen = ximage->bytes_per_line;
      if (! (bitplane = (byte *)calloc(height * linelen, 1)))
	  RETURN(PixmapNoMemory);
      if (! (bptr = (byte *)calloc(width * height * pixlen, 1))) {
	  free(bitplane);
	  RETURN(PixmapNoMemory);
      }
      dptr = bptr;
      for (plane = 1 << (depth - 1); plane; plane >>= 1) {
	  ximage = XGetImage(display, (Drawable) pixmap, 0, 0, width, height, 
			     plane, XYPixmap);
	  destline = bitplane = (byte *)ximage->data;
	  for (y = 0; y < height; y++) {
	      destmask = 0x80;
	      destptr = destline;
	      for (x = 0; x < width; x++) {
		  if (*destptr & destmask)
		      *dptr |= plane;
		  else
		      *dptr &= ~plane;
		  if (!(destmask >>= 1)) {
		      destmask = 0x80;
		      destptr++;
		  }
		  pixel = memToVal(dptr, pixlen);
		  for (a = 0; a < ncolors; a++)
		      if (*(index + a) == pixel) break;
		  if (a == ncolors) {
		      if (ncolors > indexsize) {
			  indexsize *= 2;
			  if (! (index = (Pixel *)
				 malloc(sizeof(Pixel) * indexsize))) {
			      free(bitplane);
			      free(bptr);
			      RETURN(PixmapNoMemory);
			  }
		      }
		      *(index + ncolors) = pixel;
		      ncolors++;
		  }
		  valToMem((unsigned long)a, dptr, pixlen);
		  dptr += pixlen;
	      }
	      destline += linelen;
	  }
      }
      free(bitplane);
      bitplane = NULL;
  } else {
      pixgap = ximage->bytes_per_line - (width * pixlen);

      dptr = bptr = (byte *)ximage->data;
      for (y = 0; y < height; y++) {
	  for (x = 0; x < width; x++) {
	      pixel = memToVal(dptr, pixlen);
	      for (a = 0; a < ncolors; a++)
		  if (*(index + a) == pixel) break;
	      if (a == ncolors) {
		  if (ncolors > indexsize) {
		      indexsize *= 2;
		      if (! (index = (Pixel *)
			     malloc(sizeof(Pixel) * indexsize)))
			  RETURN(PixmapNoMemory);
		  }
		  *(index + ncolors) = pixel;
		  ncolors++;
	      }
	      valToMem((unsigned long)a, dptr, pixlen);
	      dptr += pixlen;
	  }
	  dptr += pixgap;
      }
  }
  /* get rgb values and a string of char for each color
   */
  if (! (xcolors = (XColor *) malloc(sizeof(XColor) * ncolors)))
      RETURN(PixmapNoMemory);
  if (! (colorStrings = (char **) calloc(ncolors, sizeof(char *))))
      RETURN(PixmapNoMemory);

  for (cpp = 1, c = MAXPRINTABLE; ncolors > c; cpp++) c *= MAXPRINTABLE;
  if (cpp < cppm) cpp = cppm;

  for (a = 0; a < ncolors; a++) {
      if (! (*(colorStrings + a) = (char *)malloc(cpp)))
	  RETURN(PixmapNoMemory);
      **(colorStrings + a) = printable[c = a % MAXPRINTABLE];
      for (b = 1; b < cpp; b++)
	  *(*(colorStrings + a) + b) = 
	      printable[c = ((a - c) / MAXPRINTABLE) % MAXPRINTABLE];
      (xcolors + a)->pixel = *(index + a);
  }
  XQueryColors(display, colormap, xcolors, ncolors);


  /*
   * print the pixmap file
   */

  /* print the header line 
   */
  fprintf(mdata->stream.file, "%s XPM2 %s %s", DataTypes[n].Bcmt, 
	  DataTypes[n].type, DataTypes[n].Ecmt);
  if (n != 0)			/* print the assignment line */
      fprintf(mdata->stream.file, "%s %s %s", DataTypes[n].Dec, name, 
	      DataTypes[n].Boa);

  /* print the hints 
   */
  if (infos && infos->hints_cmt) /* print hints comment line */
      fprintf(mdata->stream.file, "%s%s%s", DataTypes[n].Bcmt, 
	      infos->hints_cmt, DataTypes[n].Ecmt);

  if (DataTypes[n].Bos) fprintf(mdata->stream.file, "%c", DataTypes[n].Bos);
  fprintf(mdata->stream.file, "%d %d %d %d", width, height, ncolors, cpp);
  if (DataTypes[n].Eos) fprintf(mdata->stream.file, "%c", DataTypes[n].Eos);
  fprintf(mdata->stream.file, DataTypes[n].Strs);

  /* print colors 
   */
  if (infos && infos->colors_cmt) /* print colors comment line */
      fprintf(mdata->stream.file, "%s%s%s", DataTypes[n].Bcmt, 
	      infos->colors_cmt, DataTypes[n].Ecmt);

  for (a = 0; a < ncolors; a++) {
      if (DataTypes[n].Bos)
	  fprintf(mdata->stream.file, "%c", DataTypes[n].Bos);
      for (b = 0; b < cpp; b++)
	  fprintf(mdata->stream.file, "%c", *(*(colorStrings + a) + b));
      c = 1;
      if (infos && infos->pixels) {
	  for (b = 0; b < infos->ncolors; b++)
	      if (*(infos->pixels + b) == (xcolors + a)->pixel)
		  break;
	  if (b != infos->ncolors) {
	      c = 0;
	      for (key = 1; key < NKEYS + 1; key++) {
		  if (*(*(infos->colorTable + b) + key))
		      fprintf(mdata->stream.file, "  %s %s", 
			      ColorKeys[key - 1], 
			      *(*(infos->colorTable + b) + key));
	      }
	  }
      }
      if (c)
	      fprintf(mdata->stream.file, " c #%04X%04X%04X", 
		      (xcolors + a)->red, (xcolors + a)->green, 
		      (xcolors + a)->blue);
      if (DataTypes[n].Eos)
	  fprintf(mdata->stream.file, "%c", DataTypes[n].Eos);
      fprintf(mdata->stream.file, DataTypes[n].Strs);
  }

  /* print pixels
   */
  if (infos && infos->pixels_cmt) /* print pixels comment line */
      fprintf(mdata->stream.file,  "%s%s%s", DataTypes[n].Bcmt, 
	      infos->pixels_cmt, DataTypes[n].Ecmt);

  dptr = bptr;
  for (y = 0; y < height; y++) {
      if (DataTypes[n].Bos)
	  fprintf(mdata->stream.file, "%c", DataTypes[n].Bos);
      for (x = 0; x < width; x++) {
	  for (b = 0; b < cpp; b++) {
	      fprintf(mdata->stream.file, "%c", 
		      *(*(colorStrings + memToVal(dptr, pixlen)) + b));
	  }
	  dptr += pixlen;
      }
      if (DataTypes[n].Eos)
	  fprintf(mdata->stream.file, "%c", DataTypes[n].Eos);
      if (y < height - 1) 
	  fprintf(mdata->stream.file, DataTypes[n].Strs);
	dptr += pixgap;
  }
  fprintf(mdata->stream.file, DataTypes[n].Eoa);

  mclose(mdata);
  free(index);
  if (depth % 8)
      free(bptr);
  for (a = 0; a < ncolors; a++)
      free(*(colorStrings + a));
  free(colorStrings);
  free(xcolors);
  return(XpmErrorStatus);
}
