#ifndef lint
static  char sccsid[] = "@(#)XWrPixD.c 2.10 91/07/09 Copyr 1990 GROUPE BULL";
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
#include <malloc.h>
#include <values.h>
#include <varargs.h>

extern int _doprnt();

void xprintf();

#undef RETURN
#define RETURN(status) \
  { if (index) free(index); \
    if (colorStrings) { \
	for (a = 0; a < ncolors; a++) \
	    if (*(colorStrings + a)) \
		free(*(colorStrings + a)); \
	free(colorStrings); \
    } \
    return(XpmErrorStatus = status); }


int XWritePixmapData(display, colormap, name, data, pixmap, width, 
		     height, type, cppm, infos)
     Display *display;
     Colormap colormap;
     char *name;
     char **data;
     Pixmap *pixmap;
     unsigned int width, height;
     char *type;
     unsigned int cppm;
     XpmInfo *infos;

{ XImage *ximage = NULL;
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
  unsigned int a, b, c, x, y, n = 0, key;
  Pixel pixel;

  char *dataptr;
  unsigned int datalen, dts, ilen;

  XpmErrorStatus = PixmapSuccess;

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
  xprintf(data, "%s XPM2 %s %s", DataTypes[n].Bcmt, 
	  DataTypes[n].type, DataTypes[n].Ecmt);
  if (n != 0)			/* print the assignment line */
      xprintf(data, "%s %s %s", DataTypes[n].Dec, name, 
	      DataTypes[n].Boa);

  /* print the hints 
   */
  if (infos && infos->hints_cmt) /* print hints comment line */
      xprintf(data, "%s%s%s", DataTypes[n].Bcmt, 
	      infos->hints_cmt, DataTypes[n].Ecmt);

  if (DataTypes[n].Bos) xprintf(data, "%c", DataTypes[n].Bos);
  xprintf(data, "%d %d %d %d", width, height, ncolors, cpp);
  if (DataTypes[n].Eos) xprintf(data, "%c", DataTypes[n].Eos);
  xprintf(data, DataTypes[n].Strs);

  /* print colors 
   */
  if (infos && infos->colors_cmt) /* print colors comment line */
      xprintf(data, "%s%s%s", DataTypes[n].Bcmt, 
	      infos->colors_cmt, DataTypes[n].Ecmt);

  for (a = 0; a < ncolors; a++) {
      if (DataTypes[n].Bos)
	  xprintf(data, "%c", DataTypes[n].Bos);
      for (b = 0; b < cpp; b++)
	  xprintf(data, "%c", *(*(colorStrings + a) + b));
      c = 1;
      if (infos && infos->pixels) {
	  for (b = 0; b < infos->ncolors; b++)
	      if (*(infos->pixels + b) == (xcolors + a)->pixel)
		  break;
	  if (b != infos->ncolors) {
	      c = 0;
	      for (key = 1; key < NKEYS + 1; key++) {
		  if (*(*(infos->colorTable + b) + key))
		      xprintf(data, "  %s %s", 
			      ColorKeys[key - 1], 
			      *(*(infos->colorTable + b) + key));
	      }
	  }
      }
      if (c)
	      xprintf(data, " c #%04X%04X%04X", 
		      (xcolors + a)->red, (xcolors + a)->green, 
		      (xcolors + a)->blue);
      if (DataTypes[n].Eos)
	  xprintf(data, "%c", DataTypes[n].Eos);
      xprintf(data, DataTypes[n].Strs);
  }

  /* print pixels
   */
  if (infos && infos->pixels_cmt) /* print pixels comment line */
      xprintf(data,  "%s%s%s", DataTypes[n].Bcmt, 
	      infos->pixels_cmt, DataTypes[n].Ecmt);

  dptr = bptr;

  ilen    = strlen(*data);
  datalen = ilen + (height * width * cpp);
  datalen += ((height - 1) * strlen(DataTypes[n].Strs));
  if (DataTypes[n].Bos) datalen += height;
  if (DataTypes[n].Eos) datalen += height;
  *data = realloc(*data, datalen) ;
  dataptr = (char *) (*data + ilen);

  for (y = 0; y < height; y++) {
      if (DataTypes[n].Bos)
          *dataptr++ = (char) DataTypes[n].Bos;
      for (x = 0; x < width; x++) {
          for (b = 0; b < cpp; b++) {
              *dataptr++ = (char)
                         *(*(colorStrings + memToVal(dptr, pixlen)) + b);
          }
          dptr += pixlen;
      }   
 
      if (DataTypes[n].Eos)
          *dataptr++ = (char) DataTypes[n].Eos;
      if (y < height - 1)
          for (dts = 0; dts < strlen(DataTypes[n].Strs); dts++)
              *dataptr++ = (char) DataTypes[n].Strs[dts];
	dptr += pixgap;
  }

  xprintf(data, DataTypes[n].Eoa);

  free(index);
  if (depth % 8)
      free(bptr);
  for (a = 0; a < ncolors; a++)
      free(*(colorStrings + a));
  free(colorStrings);
  free(xcolors);
  return(XpmErrorStatus);
}


void
xprintf(string, format, va_alist)
char **string, *format;
va_dcl
{
  FILE siop ;
  char buffer[512], *dptr, *sptr ;
  int count, dcount, n ;
  int scount = 0 ;
  va_list ap ;

  siop._cnt = MAXINT ;
  siop._base = siop._ptr = (unsigned char *) buffer ;
  /* I am not sure this is correct [vmh] */
#ifdef SVR4
  siop._flag = _IOREAD ;
#else
  siop._flag = _IOWRT + _IOSTRG ;
#endif SVR4
  va_start(ap) ;
  dcount = _doprnt(format, ap, &siop) ;
  va_end(ap) ;
  *siop._ptr = '\0' ;        /* Plant terminating null character */

  if (dcount > 0)
    {
      if (*string == NULL) *string = malloc((unsigned int) dcount) ;
      else
        {
          scount  = strlen(*string) ;
          count   = scount + dcount ;
          *string = realloc(*string, (unsigned int) count) ;
        }
      sptr = buffer ;
      dptr = *string + scount ;
      for (n = 0; n <= dcount; n++) *dptr++ = *sptr++ ;
    }
}

