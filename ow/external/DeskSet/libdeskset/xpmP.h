/*      @(#)xpmP.h 3.1 IEI SMI      */

/* Copyright 1990 GROUPE BULL -- See licence conditions in file COPYRIGHT */
/* xpmP.h:
 *
 *  XPM2
 *  Private Include file
 *  Developped by Arnaud Le Hors
 *  $Id: xpm.shar,v 2.5 90/10/17 17:22:35 lehors Exp $
 */

#ifndef XPMP_h
#define XPMP_h

#include <X11/Xlib.h>
#include<stdio.h>

/* we keep the same codes as for Bitmap management */
#ifndef _XUTIL_H_
#ifdef VMS
#include "decw$include:Xutil.h"
#else
#include <X11/Xutil.h>
#endif
#endif

typedef struct {
    unsigned int type;
    union {
	FILE *file;
	char **data;
    } stream;
    char         *cptr;
    unsigned int line;
} MData;

#define PixmapSuccess          0
#define PixmapOpenFailed       1
#define PixmapFileInvalid      2
#define PixmapNoMemory         3
#define PixmapParseColorFailed 4
#define PixmapAllocColorFailed 5


#define MARRAY 0
#define MFILE  1
#define MPIPE  2
#define MDATA  3

extern int XpmErrorStatus;

typedef unsigned char  byte;      /* byte type */
typedef unsigned long  Pixel;     /* what X thinks a pixel is */

extern char *BCMT, *ECMT, BOS, EOS;

#define EOL '\n'
#define TAB '\t'
#define SPC ' '

extern struct DataTypeStruct {
    char *type;                 /* key word */
    char *Bcmt;                 /* string begining comments */
    char *Ecmt;                 /* string ending comments */
    char  Bos;                  /* character begining strings */
    char  Eos;                  /* character ending strings */
    char *Strs;                 /* strings separator */
    char *Dec;                  /* data declaration string */
    char *Boa;                  /* string begining assignment */
    char *Eoa;			/* string ending assignment */
};

extern struct DataTypeStruct DataTypes[];

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

extern char *ColorKeys[];

#define NKEYS 5			/* number of ColorKeys */

#define MONO	2		/* key numbers for visual type, they must */
#define GRAY4	3		/* fit along with the number key of each */
#define GRAY 	4		/* corresponding element in ColorKeys[] */
#define COLOR	5		/* defined in xpm.h */


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

FUNC(XReadPixmapData, int, (Display *display,
			    Visual *visual,
			    Drawable d,
			    Colormap colormap,
			    char *data,
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

FUNC(XWritePixmapData, int, (Display *display,
			     Colormap colormap,
			     char *name,
			     char **data,
			     Pixmap *pixmap,
			     unsigned int width,
			     unsigned int height,
			     char *type,
			     unsigned int cppm,
			     XpmInfo *infos));

FUNC(XFreeXpmInfo, int, (XpmInfo *infos));

/* XPM private routines */
FUNC(CreatePixmap, Pixmap, (Display *display,
			    Visual *visual,
			    Drawable d,
			    Colormap colormap,
			    MData *data,
			    unsigned int depth,
			    unsigned int *width_return, 
			    unsigned int *height_return, 
			    Pixel **pixels_return,
			    unsigned int *npixels_return,
			    ColorSymbol colorsymbols[],
			    unsigned int numsymbols,
			    XpmInfo *infos));

FUNC(visualType, int, (Visual *visual));
FUNC(freeColorTable, int, (char ***colorTable, int ncolors));

/* I/O utility */
FUNC(mnextstring, int, (MData *mdata));
FUNC(mnextui, unsigned int, (MData *mdata));
FUNC(mgetc, char, (MData *mdata));
FUNC(mungetc, char, (int c, MData *mdata));
FUNC(mnextc, char, (MData *mdata));
FUNC(mnextw, unsigned int, (MData *mdata, char *buf));
FUNC(mgetcmt, int, (MData *mdata, char **cmt));
FUNC(mreadopen, MData*, (char *filename));
FUNC(mwriteopen, MData*, (char *filename));
FUNC(mdataopen, MData*, (char **data));
FUNC(mclose, int, (MData *mdata));

/* Values routines */
FUNC(valToMem, void, (unsigned long val, byte *p, unsigned int len));
FUNC(memToVal, unsigned long, (byte *p, unsigned int len));


#endif
