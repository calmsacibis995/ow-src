#ifndef lint
static  char sccsid[] = "@(#)xpm.c 1.9 91/07/12 Copyr 1990 GROUPE BULL";
#endif

/* Copyright 1990 GROUPE BULL -- See licence conditions in file COPYRIGHT */
/* create.c:
 *
 *  XPM2
 *  Create utility for XPM2 file format
 *  Developped by Arnaud Le Hors
 *  $Id: xpm.shar,v 2.5 90/10/17 17:22:35 lehors Exp $
 */

#include "xpmP.h"
#include <sys/types.h>
#include <sys/stat.h>

#define RETURN(status) \
  { freeColorTable(colorTable, ncolors); \
    XpmErrorStatus = status; \
    return(NULL); }

/* 
 * Global Data - I hate those French people - Sean 
 */

int XpmErrorStatus;
char *BCMT, *ECMT, BOS, EOS;

struct DataTypeStruct DataTypes[] = {
    {"", "!", "\n", '\0', '\n', "", "", "", "", /* Natural type */},
    {"C", "/*", "*/\n", '"', '"', ",\n", "static char **", " = {\n", "};\n",},
    {"Lisp", ";", "\n", '"', '"', "\n", "(setq ", " '(\n", "))\n",},
    {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

char *ColorKeys[] = {
    "s",                        /* key #1: symbol */
    "m",                        /* key #2: mono visual */
    "g4",                       /* key #3: 4 grays visual */
    "g",                        /* key #4: gray visual */
    "c",                        /* key #5: color visual */
};

char *printable =
" .XoO+@#$%&*=-;:?>,<1234567890qwertyuipasdfghjklzxcvbnmMNBVCZ\
ASDFGHJKLPIUYTREWQ!~^/()_`'][{}|" ;
           /* printable begin with a space, so in most case, due to
            my algorythm, when the number of different colors is
            less than MAXPRINTABLE, it will give a char follow by
            "nothing" (a space) in the readable xpm file */

Pixmap CreatePixmap(display, visual, d, colormap, data, depth, 
		    width_return, height_return, 
		    pixels_return, npixels_return,
		    colorsymbols, numsymbols, infos)
     Display *display;
     Visual *visual;
     Drawable d;
     Colormap colormap;
     MData *data;
     unsigned int depth;
     unsigned int *width_return, *height_return, *npixels_return;
     Pixel **pixels_return;
     ColorSymbol colorsymbols[];
     unsigned int numsymbols;
     XpmInfo *infos;

{ Pixmap       pixmap;
  unsigned int cpp;             /* chars per pixel */
  unsigned int ncolors;         /* number of colors */
  char       ***colorTable;     /* color table */
  unsigned int key;		/* color key */
  unsigned int pixlen;		/* length of pixels */
  Pixel        *pixels;		/* pixels colors */
  unsigned int width, height, linelen;
  char         *chars, buf[BUFSIZ], *colorname;
  byte         *dptr, *bptr, *destline, *destptr, *bitplane, destmask;
  unsigned long plane;
  XColor       xcolor;
  XGCValues    gcv;
  GC           gc;
  XImage       *ximage;
  unsigned int a, b, x, y, l = 0;

  /* Initialize return values
   */
  *width_return = *height_return = *npixels_return = 0;
  *pixels_return = NULL;

  /*
   * parse the data
   */

  /* read hints: width, height, ncolors, chars_per_pixel
   */
  if (! ((width = mnextui(data)) && (height = mnextui(data))
	 && (ncolors = mnextui(data)) && (cpp =  mnextui(data)))) {
      XpmErrorStatus = PixmapFileInvalid;
      return(NULL);
  }

  /* store the hints comment line
   */
  if (infos) mgetcmt(data, &infos->hints_cmt);

  for (a = 1, b = 2; b < ncolors; b <<= 1, a++);
/*  pixlen = (a / 8) + (a % 8 ? 1 : 0); */

  /* we can't really know the pixlen for sure before we 
     actually create a sample XImage, but I'm going to 
     make an educated guess. */

  pixlen = depth > 8 ? 4 : 1;

  /* read colors
   */
  if (! (colorTable = (char ***)calloc(ncolors, sizeof(char **)))) {
      XpmErrorStatus = PixmapNoMemory;
      return(NULL);
  }
  for(a = 0; a < ncolors; a++) {
      mnextstring(data);		/* skip the line */
      if (! (*(colorTable + a) = (char **)calloc((NKEYS + 1), sizeof(char *))))
	  RETURN(PixmapNoMemory);
      /* read pixel value
       */
      if (! (**(colorTable + a) = (char *)malloc(cpp)))
	  RETURN(PixmapNoMemory);
      for (b = 0; b < cpp; b++)
	  *(**(colorTable + a) + b)= mgetc(data);

      /* read color keys and values
       */
      while (l = mnextw(data, buf)) {
	  for (key = 1; key < NKEYS + 1; key++)
	      if (! strncmp(ColorKeys[key - 1], buf, l))
		  break;
	  if (l = mnextw(data, buf)) {
	      if (! (*(*(colorTable + a) + key) = (char *)malloc(l + 1)))
		  RETURN(PixmapNoMemory);
	      strncpy(*(*(colorTable + a) + key), buf, l);
	      *(*(*(colorTable + a) + key) + l) = '\0';
	  } else {
	      RETURN(PixmapFileInvalid); /* key without value */
	  }
      }      
  }

  /* store the colors comment line
   */
  if (infos) mgetcmt(data, &infos->colors_cmt);

  /* read pixels and index them on color number
   */
  if (! (dptr = bptr = (unsigned char *)
	 malloc(width * height * pixlen)))
      RETURN(PixmapNoMemory);
  if (! (chars = (char *)malloc(cpp)))
      RETURN(PixmapNoMemory);
  for (y = 0; y < height; y++) {
      mnextstring(data);
      for (x = 0; x < width; x++) {
	  for (a = 0; a < cpp; a++) {
	      chars[a] = mgetc(data);
	  }
	  for (a = 0; a < ncolors; a++)
	      if (!strncmp(**(colorTable + a), chars, cpp))
		  break;
	  if (a == ncolors) {	/* no color matches */
	      RETURN(PixmapFileInvalid);
	  }
	  valToMem((unsigned long)a, dptr, pixlen);
	  dptr += pixlen;
      }
  }

  /* store the pixels comment line
   */
  if (infos) mgetcmt(data, &infos->pixels_cmt);

  /* 
   * build the image data
   */

  key = visualType(visual);

  if (! (pixels = (Pixel *)malloc(sizeof(Pixel) * ncolors)))
      RETURN(PixmapNoMemory);

  /* get pixel colors and index them
   */
  for (a = 0; a < ncolors; a++) {
      colorname = NULL;

      /* look for a defined symbol
       */
      if (numsymbols && *(*(colorTable + a) + 1)) {
	  for (l = 0; l < numsymbols; l++)
	      if (!strcmp(colorsymbols[l].name, *(*(colorTable + a) + 1)))
		  break;
	  if (l != numsymbols && colorsymbols[l].value)
	      colorname = colorsymbols[l].value;
      }
      if (! colorname) {
	  if (*(*(colorTable + a) + key)) {
	      b = key;
	  } else {
	      for (b = key - 1; b > 1; b--)
		  if (*(*(colorTable + a) + b))
		      break;
	      if (b == 1) {
		  for (b = key + 1; b < NKEYS + 1; b++)
		      if (*(*(colorTable + a) + b))
			  break;
		  if (b == NKEYS + 1)
		      RETURN(PixmapFileInvalid);
	      }
	  }
	  colorname = *(*(colorTable + a) + b);
      }
      if (! XParseColor(display, colormap, colorname, &xcolor))
	  RETURN(PixmapParseColorFailed);

      if (depth == 1)
      {
          if (XAllocColor(display, colormap, &xcolor)) {
	      if (infos) {
	          for (b = 0; b < a; b++)
		      if (*(pixels + b) == xcolor.pixel)
		          break;
	          if (b != a) {
		      if (! XAllocColorCells(display, colormap, False, NULL, 0,
					     pixels + a, 1))
		          RETURN(PixmapAllocColorFailed);
		      XFreeColors(display, colormap, &(xcolor.pixel), 1, 0);
		      xcolor.pixel = *(pixels + a);
		      XStoreColor(display, colormap, &xcolor);
	          }
	      }
          } else
	      RETURN(PixmapAllocColorFailed);
          *(pixels + a) = xcolor.pixel;
      }
      else
          *(pixels + a) = ds_x_pixel(display, colormap, &xcolor);
  }

  /* set data pixels to the indexed color pixel
   */
  dptr = bptr;
  for (y = 0; y < height; y++) {
      for (x = 0; x < width; x++) {
	  valToMem(*(pixels + memToVal(dptr, pixlen)), dptr, pixlen);
	  dptr += pixlen;
      }
  }

  /*
   * send the image to the server
   */

  pixmap = XCreatePixmap(display, d, width, height, depth);

  /* Jim Frost algorithm */
    /* if the destination depth is not a multiple of 8, then we send each
     * plane as a bitmap because otherwise we would have to pack the pixel
     * data and the XImage format is pretty vague about how that should
     * be done.  this is not as fast as it would be if it were packed but
     * it should be a lot more portable and only slightly slower.
     */

  if (depth % 8) {
      gcv.function = GXcopy;
      gcv.background= 0;
      gc = XCreateGC(display, pixmap, GCFunction | GCBackground, &gcv);
      linelen = (width / 8) + (width % 8 ? 1 : 0);
      if (! (bitplane = (byte *)malloc(height * linelen)))
	  RETURN(PixmapNoMemory);
      ximage = XCreateImage(display, visual, 1, XYBitmap, 0, (char*)bitplane, 
			    width, height, 8, 0);
      ximage->bitmap_bit_order = MSBFirst;
      ximage->byte_order = MSBFirst;

      for (plane = 1 << (depth - 1); plane; plane >>= 1) {
	dptr = bptr;
	destline = bitplane;
	for (y = 0; y < height; y++) {
	  destmask = 0x80;
	  destptr = destline;
	  for (x = 0; x < width; x++) {
	    if (*dptr & plane)
	      *destptr |= destmask;
	    else
	      *destptr &= ~destmask;
	    if (!(destmask >>= 1)) {
	      destmask = 0x80;
	      destptr++;
	    }
	    dptr += pixlen;
	  }
	  destline += linelen;
	}
	XSetForeground(display, gc, plane);
	XSetPlaneMask(display, gc, plane);
	XPutImage(display, pixmap, gc, ximage, 0, 0, 0, 0, width, height);
      }
      free(bitplane);
  } else {

    /* send image across in one whack
     */
      gcv.function = GXcopy;
      gc = XCreateGC(display, pixmap, GCFunction, &gcv);
      ximage = XCreateImage(display, visual, depth, ZPixmap, 0, (char *)bptr, 
			    width, height, 8, 0);
      ximage->byte_order = MSBFirst; /* trust him, Jim Frost knows what he's */
      /* talking about */
      XPutImage(display, pixmap, gc, ximage, 0, 0, 0, 0, width, height);
  }
  ximage->data = NULL;
  XDestroyImage(ximage);	/* waste not want not */
  XFreeGC(display, gc);
  free(bptr);
  free(chars);

  *width_return = width;
  *height_return = height;
  *pixels_return = pixels;
  *npixels_return = ncolors;

  /* store color table infos
   */
  if (infos) {
      infos->ncolors = ncolors;
      infos->colorTable = colorTable;
      infos->pixels = pixels;
  } else {
      freeColorTable(colorTable, ncolors);
  }

  return(pixmap);
}
freeColorTable(colorTable, ncolors)
char ***colorTable;
int ncolors;
{ int a, b;
    if (colorTable) {
	for (a = 0; a < ncolors; a++)
	    if (*(colorTable +a)) {
		for (b = 0; b < (NKEYS + 1); b++)
		    if (*(*(colorTable +a) + b))
			free(*(*(colorTable +a) + b));
		free(*(colorTable +a));
	    }
	free(colorTable);
    }
}

XFreeXpmInfo(infos)
XpmInfo *infos;
{
    if (infos) {
	if (infos->type) free(infos->type);
	freeColorTable(infos->colorTable, infos->ncolors);
	if (infos->hints_cmt) free(infos->hints_cmt);
	if (infos->colors_cmt) free(infos->colors_cmt);
	if (infos->pixels_cmt) free(infos->pixels_cmt);
    }
}
static int CommentLength = 0;
static char Comment[BUFSIZ];
LFUNC(atoui, unsigned int, (char *p));

static unsigned int atoui(p)
char *p;
{
        register int n;

        n = 0;
        while(*p >= '0' && *p <= '9')
                n = n*10 + *p++ - '0';
        return(n);
}

mnextstring(mdata)		/* skip to the end of the current string */
MData *mdata;			/* and the beginning of the next one */
{ char  c;
    switch(mdata->type) {
    case MARRAY:
	mdata->cptr = (mdata->stream.data)[++mdata->line];
	break;
    case MFILE:
    case MPIPE:
    case MDATA:
	while ((c = mgetc(mdata)) != EOS && c != EOF);
	if (BOS)
	    while ((c = mgetc(mdata)) != BOS && c != EOF);
	break;
    }
}

unsigned int mnextui(mdata)	/* skip whitespace and return the following */
MData *mdata;			/* unsigned int 			    */
{ char buf[BUFSIZ];

  mnextw(mdata, buf);
  return(atoui(buf));
}

char mgetc(mdata)		/* return the current character */
MData *mdata;
{ char c;
  register unsigned int n = 1, a;
  unsigned int notend;

    switch(mdata->type) {
    case MARRAY:
    case MDATA:
	return(*mdata->cptr++);
    case MFILE:
    case MPIPE:
	c = getc(mdata->stream.file);
	if (BCMT && c == BCMT[0]) {
	    Comment[0] = c;
	    /* skip the string begining comment
	     */
	    while ((Comment[n] = getc(mdata->stream.file)) == BCMT[n]
		   && BCMT[n] != '\0' && Comment[n] != EOF) n++;
	    if (BCMT[n] != '\0') { /* this wasn't the begining of a comment */
		for (a = 1; a < n; a++)
		    mungetc(Comment[a], mdata);
		return(c);
	    }
	    /* store comment 
	     */
	    Comment[0] = Comment[n];
	    notend = 1;
	    n = 0;
	    while (notend) {
		while (Comment[n] != ECMT[0] && Comment[n] != EOF)
		    Comment[++n] = getc(mdata->stream.file);
		CommentLength = n++;
		a = 1;
		while ((Comment[n] = getc(mdata->stream.file)) == ECMT[a]
		       && ECMT[a] != '\0' && Comment[n] != EOF) { a++; n++; }
		if (ECMT[a] == '\0') { /* this is the end of the comment */
		    notend = 0;
		    mungetc(Comment[n], mdata);
		}
	    }
	    c = mgetc(mdata);
	}
	return(c);
    }
}


char mungetc(c, mdata)		/* push the given character back */
int c;
MData *mdata;
{ 
    switch(mdata->type) {
    case MARRAY:
    case MDATA:
	return(*--mdata->cptr = c);
    case MFILE:
    case MPIPE:
	return(ungetc(c, mdata->stream.file));
    }
}


mskipwhite(mdata)		/* skip whitespace */
MData *mdata;
{ char c;

    switch(mdata->type) {
    case MARRAY:
	while (*mdata->cptr == SPC || *mdata->cptr == TAB) 
	    mdata->cptr++;
	break;
    case MDATA:
    case MFILE:
    case MPIPE:
	while ((c = mgetc(mdata)) == SPC || c == TAB);
	mungetc(c, mdata);
	break;
    }
}


unsigned int mnextw(mdata, buf)	/* skip whitespace and return the following */
MData *mdata;			/* word					    */
char *buf;
{ register unsigned int n = 0;

    mskipwhite(mdata);
    switch(mdata->type) {
    case MARRAY:
	while ((buf[n] = *mdata->cptr++) != SPC 
	       && buf[n] != TAB && buf[n] != EOS && buf[n] != EOF) n++;
	mdata->cptr--;
	break;
    case MDATA:
    case MFILE:
    case MPIPE:
	while ((buf[n] = mgetc(mdata)) != SPC 
	       && buf[n] != TAB && buf[n] != EOS && buf[n] != EOF) n++;
	mungetc(buf[n],mdata); 
	break;
    }
  return(n);
}

mgetcmt(mdata, cmt)		/* get the current comment line */
MData *mdata;
char **cmt;
{
    switch(mdata->type) {
    case MARRAY:
	break;
    case MDATA:
    case MFILE:
    case MPIPE:
	if (CommentLength) {
	    *cmt = (char *) malloc(CommentLength + 1);
	    strncpy(*cmt, Comment, CommentLength);
	    (*cmt)[CommentLength] = '\0';
	    CommentLength = 0;
	} else
	    *cmt = NULL;
	break;
    }
}


MData* mreadopen(filename)
char *filename;
{ MData* mdata;
  char *compressfile, buf[BUFSIZ];
  struct stat status;
  char	readbuf[10];

  if (! (mdata = (MData *)malloc(sizeof(MData)))) {
      XpmErrorStatus = PixmapNoMemory;
      return(NULL);
  }
  if ((strlen(filename) > 2) && 
      !strcmp(".Z", filename + (strlen(filename) - 2))) {
      mdata->type= MPIPE;
      sprintf(buf, "uncompress -c %s", filename);
      if (! (mdata->stream.file = popen(buf, "r"))) {
	  free(mdata);
	  XpmErrorStatus = PixmapOpenFailed;
	  return(NULL);
      }
  } else {
      if (! (compressfile = (char*)malloc(strlen(filename) + 3))) {
	  free(mdata);
	  XpmErrorStatus = PixmapNoMemory;
	  return(NULL);
      }
      strcpy(compressfile, filename);
      strcat(compressfile, ".Z");
      if (!stat(compressfile, &status)) {
	  sprintf(buf, "uncompress -c %s", compressfile);
	  if (! (mdata->stream.file = popen(buf, "r"))) {
	      free(mdata);
	      free(compressfile);
	      XpmErrorStatus = PixmapOpenFailed;
	      return(NULL);
	  }
	  mdata->type= MPIPE;
      } else {
	  if (! (mdata->stream.file = fopen(filename, "r"))) {
	      free(mdata);
	      free(compressfile);
	      XpmErrorStatus = PixmapOpenFailed;
	      return(NULL);
	  }

          fread(readbuf, 1, 6, mdata->stream.file);
	  if (strncmp(readbuf, "! XPM2", 6))
          {
              fclose(mdata->stream.file);
	      free(mdata);
	      free(compressfile);
	      XpmErrorStatus = PixmapFileInvalid;
	      return(NULL);
          }
          else
              rewind(mdata->stream.file);

	  mdata->type = MFILE;
      }
      free(compressfile);
  }
  return(mdata);
}

MData* mwriteopen(filename)
char *filename;
{ MData* mdata;
  char buf[BUFSIZ];

  if (! (mdata = (MData *)malloc(sizeof(MData)))) {
      XpmErrorStatus = PixmapNoMemory;
      return(NULL);
  }
  if ((strlen(filename) > 2) && 
      !strcmp(".Z", filename + (strlen(filename) - 2))) {
      sprintf(buf, "compress > %s", filename);
      if (! (mdata->stream.file = popen(buf, "w"))) {
	  free(mdata);
	  XpmErrorStatus = PixmapOpenFailed;
	  return(NULL);
      }
      mdata->type = MPIPE;
  } else {
      if (! (mdata->stream.file = fopen(filename, "w"))) {
	  free(mdata);
	  XpmErrorStatus = PixmapOpenFailed;
	  return(NULL);
      }
      mdata->type = MFILE;
  }
  return(mdata);
}

MData* mdataopen(data)
char **data;
{ MData *mdata;

  if (strncmp(*data, "! XPM2", 6))
  {
    XpmErrorStatus = PixmapFileInvalid;
    return(NULL);
  }

  if (! (mdata = (MData *)malloc(sizeof(MData)))) { 
      XpmErrorStatus = PixmapNoMemory;
      return(NULL);
  }
  mdata->type = MARRAY;
  mdata->stream.data = data;
  mdata->cptr = *data;
  mdata->line = 0;
  return(mdata);
}

mclose(mdata)
MData *mdata;
{
    switch(mdata->type) {
    case MARRAY:
    case MDATA:
	break;
    case MFILE:
	fclose(mdata->stream.file);
	break;
    case MPIPE:
	pclose(mdata->stream.file);
    }
    free(mdata);
}

/* 
 * routines for converting byte values to long values.  these are pretty
 * portable although they are not necessarily the fastest things in the
 * world.
 *
 * jim frost 10.02.89
 * Copyright 1989, 1990 Jim Frost
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  The author makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *  $Id: xpm.shar,v 2.5 90/10/17 17:22:35 lehors Exp $
 */

typedef unsigned char  my_byte;      /* byte type */

unsigned long memToVal(p, len)
     my_byte         *p;
     unsigned int  len;
{ unsigned int  a;
  unsigned long i;

  i= 0;
  for (a= 0; a < len; a++)
    i= (i << 8) + *(p++);
  return(i);
}

void valToMem(val, p, len)
     unsigned long  val;
     my_byte          *p;
     unsigned int   len;
{ int a;

  for (a= len - 1; a >= 0; a--) {
    *(p + a)= val & 0xff;
    val >>= 8;
  }
}

visualType(visual)
Visual *visual;
{
  switch (visual->class) {
  case StaticGray:
  case GrayScale:
      switch(visual->map_entries) {
      case 2:
	  return(MONO);
      case 4:
	  return(GRAY4);
      default:
	  return(GRAY);
      }
  default:
      return(COLOR);
  }
}

