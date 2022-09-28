/*      @(#)xpm.h 1.5 IEI SMI      */

/* Copyright 1990 GROUPE BULL -- See licence conditions in file COPYRIGHT */
/* xpm.h:
 *
 *  XPM2
 *  Include file
 *  Developped by Arnaud Le Hors
 *  $Id: xpm.shar,v 2.5 90/10/17 17:22:35 lehors Exp $
 */

/*
 * Beware - don't include this file unless you are going to build
 * with the xpm library, included now in libdeskset.  Those silly
 * French people from Bull put global data in their include files.
 * I fixed things a little - you used to get the error if you
 * built with the library and not the include file, something
 * happening far more often.   - Sean Welch
 */

#ifndef XPM_h
#define XPM_h

#define XPM_FORMAT 2

#include <X11/Xlib.h>
#include <stdio.h>

/* we keep the same codes as for Bitmap management */
#ifndef _XUTIL_H_
#ifdef VMS
#include "decw$include:Xutil.h"
#else
#include <X11/Xutil.h>
#endif
#endif

#define PixmapSuccess          0
#define PixmapOpenFailed       1
#define PixmapFileInvalid      2
#define PixmapNoMemory         3
#define PixmapParseColorFailed 4
#define PixmapAllocColorFailed 5


/* Global variables
 */

extern int XpmErrorStatus;
extern char *BCMT, *ECMT, BOS, EOS;

#define EOL '\n'
#define TAB '\t'
#define SPC ' '

typedef unsigned char  byte;      /* byte type */
typedef unsigned long  Pixel;     /* what X thinks a pixel is */

extern struct DataTypesStruct DataTypes[];

extern char *ColorKeys[];

#define NKEYS 5			/* total number of Colorkeys */


typedef struct {
    char *name;
    char *value;
} ColorSymbol;

typedef struct {
    char *type;
    int ncolors;
    char ***colorTable;
    Pixel *pixels;
    char *hints_cmt;
    char *colors_cmt;
    char *pixels_cmt;
} XpmInfo;

#define MAXPRINTABLE 93             /* number of printable ascii chars 
				       minus \ and " for string compat. */

extern char *printable;

/* minimal portability layer between ansi and KR C 
 */

/* forward declaration of functions with prototypes */

#if __STDC__				/* ANSI */
#define FUNC(f, t, p) extern t f p
#define LFUNC(f, t, p) static t f p
#else					/* K&R */
#define FUNC(f, t, p) extern t f()
#define LFUNC(f, t, p) static t f()
#endif					/* end of K&R */

/* functions declarations
 */

FUNC(XCreatePixmapFromData, int, (Display *display,
				  Visual *visual,
				  Drawable d,
				  Colormap colormap,
				  char **data,
				  unsigned int depth,
				  Pixmap *pixmap_return,
				  unsigned int *width_return, 
				  unsigned int *height_return, 
				  Pixel **pixels_return,
				  unsigned int *npixels_return,
				  ColorSymbol colorsymbols[],
				  unsigned int numsymbols,
				  XpmInfo *infos));

FUNC(XReadPixmapFile, int, (Display *display,
			    Visual *visual,
			    Drawable d,
			    Colormap colormap,
			    char *filename,
			    unsigned int depth,
			    Pixmap *pixmap_return,
			    unsigned int *width_return, 
			    unsigned int *height_return, 
			    Pixel **pixels_return,
			    unsigned int *npixels_return,
			    ColorSymbol colorsymbols[],
			    unsigned int numsymbols,
			    XpmInfo *infos));

FUNC(XWritePixmapFile, int, (Display *display,
			     Colormap colormap,
			     char *filename,
			     Pixmap *pixmap,
			     unsigned int width,
			     unsigned int height,
			     char *type,
			     unsigned int cppm,
			     XpmInfo *infos));

FUNC(XFreeXpmInfo, int, (XpmInfo *infos));

#endif
