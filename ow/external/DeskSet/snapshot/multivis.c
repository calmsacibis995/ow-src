#ifndef lint
static char sccsid[] = "@(#)multivis.c	3.2 08/06/92 Copyright 1987-1991 Sun Microsystem, Inc." ;
#endif
/*-
 * multivis.c - Mechanism for GetImage across Multiple Visuals
 *
 * Copyright (c) 1989,90,91 by Sun Microsystems, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.	The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * Comments and additions should be sent to the author:
 *
 *		       milind@Eng.Sun.COM
 *
 *		       Milind M. Pansare
 *		       MS 14-01
 *		       Windows and Graphics Group
 *		       Sun Microsystems, Inc.
 *		       2550 Garcia Ave
 *		       Mountain View, CA  94043
 *
 * Revision History:
 * 11-15-90 Written
 * 5-15-91 Joe Warzecha
 * 	   Made changes with some of the functions so that it could be
 *	   used with snapshot.
 */

#include <X11/Xlib.h>
#include "snapshot.h"
#include "xdefs.h"
#include "multivis.h"

static MVColmap *mvFindColormap P((Colormap));
static XVisualInfo *mvMatchVisual P((VisualID));

static void mvGetColormap P((MVWinVisInfo *));
static void mvCalculateComposite P((MVWinVisInfo *));

static unsigned long mvCompositePixel P((unsigned long, MVColmap *));

static MVWinVisInfoList winList;	/* Here we'll grow a list of windows
					   Sorted back to front */

static MVColmap *colMaps;		/* list of colormaps we encounter */
static MVPel *mvImg;			/* mvImg is what we compose the image
					   into */

static int request_width, 
	   request_height;		/* size of requested rectangle */
static int request_x, request_y;	/* The top left of requested
					   rectangle in Root Space */

extern XVars X;
extern Vars v;

/*
 * Initialise the mvLib routines ...
*/

void
init_multivis ()
{
    XVisualInfo 	 vinfo_template;
    XVisualInfo 	*vlist;;

    vinfo_template.screen = X->screen;
    X->vlist = XGetVisualInfo (X->dpy, VisualScreenMask, 
					&vinfo_template, &v->num_vis);

/*
 *  We didn't get a visual list??  Assume only one in this case, and
 *  continue.
 */

    if (X->vlist == NULL)
       v->num_vis = 1;

}

/*
 * Create an Img.. Cleared to zeros.
 * returns 0 if failure, non-zero for success.
 * Note that it is the reponsibility of the caller
 * to verify that any resulting XGetImage will
 * be within the bounds of the screen.
 * i.e, x, y, wd, ht must be such that the rectangle
 * is fully within the bounds of the screen.
*/
int
mvCreatImg(wd, ht, x, y)
int wd, ht;
int x, y;
{

/* 
 * Create mvImg.
 */

    request_width = wd;
    request_height = ht;
    request_x = x;
    request_y = y;
    if ((mvImg = 
		(MVPel *) calloc(sizeof(MVPel), request_width * request_height))
        != NULL) 
       return (1);

    return (0);
}

/* 
 * Reset the mvLib routines
*/
void
mvReset()
{

/* 
 * Free mvImg.
 */

    if (mvImg) {
       free(mvImg);
       mvImg = (MVPel *) NULL;
       }

/* 
 * Clean winList.
 */

    if (winList.wins) {
       free(winList.wins);
       winList.wins = NULL;
       winList.used = winList.allocated = 0;
       }

/* 
 * Clean colmap list.
 */

    while (colMaps) {
       MVColmap *pCmap;
       pCmap = colMaps;
       colMaps = pCmap->next;
       if (pCmap->Colors)
      	  free(pCmap->Colors);
       free(pCmap);
       }

}

/*
 * Recursively walk the window tree.
 * Find windows that intersect the requested region.
 * Create a list of such windows in global winList.
 * Assumes winList was cleared beforehand.
 */

void
mvWalkTree(win, px, py, x, y, wi, hi)
Window win;	/* This window */
int px, py;	/* parent's origin in root space */
int x, y;	/* Top left of requested rectangle in root space */
int wi, hi; 	/* size of requested rectangle */
{

    XWindowAttributes 	 xwa;
    int 		 width, 
			 height, 
			 x1, 
			 y1;
    Window 		 root, 
		 	 parent, 
			*children;
    unsigned int 	 nchild, 
		 	 n;
    MVWinVisInfo 	*pWinInfo;
   
    if (!XGetWindowAttributes(X->dpy, win, &xwa) || 
	xwa.map_state != IsViewable) 
       return;

/* 
 * Compute top-left of image in root space.
 */

    x1 = max(x, xwa.x+px);
    y1 = max(y, xwa.y+py);
    width = min(x+wi, xwa.x+xwa.width+2*xwa.border_width+px)-x1;
    height = min(y+hi, xwa.y+xwa.height+2*xwa.border_width+py)-y1;

    if (width <=0 || height <= 0) 
       return;

/* 
 * We're interested ... 
 */

    if (winList.used >= winList.allocated) { 

/* 
 * Expand or create the array.
 */
       winList.allocated = (winList.allocated?winList.allocated*MV_WIN_TUNE2:
				       MV_WIN_TUNE1);
       winList.wins = (MVWinVisInfo *)(winList.wins ?
	      realloc(winList.wins,winList.allocated*sizeof(MVWinVisInfo)):
	      malloc(winList.allocated*sizeof(MVWinVisInfo)));
       }

    pWinInfo = &(winList.wins[winList.used++]);
    pWinInfo->window = win;
    pWinInfo->depth = xwa.depth;
    pWinInfo->visinfo = mvMatchVisual(XVisualIDFromVisual(xwa.visual));
    pWinInfo->colmap = mvFindColormap(xwa.colormap);
    pWinInfo->x = x1-xwa.border_width-xwa.x-px; 
    pWinInfo->y = y1-xwa.border_width-xwa.y-py;
    pWinInfo->width = width;
    pWinInfo->height = height;
    pWinInfo->x1 = x1;
    pWinInfo->y1 = y1;
    pWinInfo->is_used = TRUE;

/* 
 * Find children, back to front.
 */

    if (XQueryTree(X->dpy, win, &root, &parent, &children, &nchild)) {
       for (n=0; n<nchild; n++) {
           mvWalkTree(children[n], px+xwa.x+xwa.border_width,
			           py+xwa.y+xwa.border_width, 
			           x1, y1, width, height);
           }

/* 
 * Free children.
 */

       if (nchild > 0)
          XFree((char *) children);
       }

    return;

}

int
mvDepth ()
{
    return (winList.wins[0].depth);
}

/*
 * CHANGE
 * Returns 0 if no problems,
 * Returns 1 if depths differ
 * returns 2 if colormap or visinfo differ
 * NOTE that this chenge & the previous change are reprehensible hacks,
 * to let xmag work with pageview, and xcolor respectively.
 * USED TO BE...
	 * CHANGE
	 * Returns 0 if no problems, 
	 * Returns 1 if visinfo or depth differ
	 * returns 2 if colormap only differ
	 * USED TO BE...
	 * Returns non-zero if the winList created by mvWalkTree
	 * might potentially have windows of different Visuals
	 * else returns 0
*/

int 
mvIsMultiVis()
{
    int j;
    int i = winList.used;

/*
 * First, let's check to see if the windows are covered entirely by another
 * window.
 */

    while (i--) 
       for (j = winList.used - 1; 
		(j > i) && (winList.wins[i].is_used == TRUE); j--)
           if ( (winList.wins[i].x1 >= winList.wins[j].x1) &&
	        (winList.wins[i].y1 >= winList.wins[j].y1) &&
	        (winList.wins[i].x1 + winList.wins[i].width <=
			winList.wins[j].x1 + winList.wins[j].width) &&
		(winList.wins[i].y1 + winList.wins[i].height <=
			winList.wins[j].y1 + winList.wins[j].height))
	      winList.wins[i].is_used = FALSE;

    
    i = winList.used;
    while (i--) {
       if (winList.wins[i].is_used == FALSE)
	  continue;

    /*
    if(winList.wins[i].depth != winList.wins[0].depth ||
       winList.wins[i].visinfo != winList.wins[0].visinfo)
	 return (1);
    */

       if (winList.wins[i].depth != winList.wins[0].depth) 
	  return (1);

       if (winList.wins[i].visinfo != winList.wins[0].visinfo) 
	  return (1);

       if (winList.wins[i].colmap != winList.wins[0].colmap) 
	  return (1);
       }

    return (0);
}

Colormap
mvColormap()
{
    return (winList.wins[0].colmap->cmap);
}

/*
 * Traverse the window list front to back. Get the entire Image
 * from each window, but only Label a pixel in the Img once.
 * That is, once a pixel has been Labeled, any more fetches
 * from the same pixel position are discarded. Once all pixels
 * positions have been fetched, we're done. This will eliminate
 * windows that have nothing to contibute to the requested region,
 * but will nevertheless have the problem of the painters
 * algorithm, where more pixels were fetched from a
 * window than were essential.
 * Assumes that winList has been filled beforehand, and Img was cleared.
*/

void
mvDoWindowsFrontToBack()
{
    int 		 i;
    MVWinVisInfo 	*pWI;
    XImage 		*xim;
    int 		 xi, 
			 yi;
    int 		 nPixelsUnLabeled = request_width*request_height;

    for (i=winList.used-1; ((nPixelsUnLabeled > 0) && i >= 0); i--) {
        pWI = &(winList.wins[i]);
	if (pWI->depth != 0) {
           if (!(xim = XGetImage(X->dpy, pWI->window, pWI->x, pWI->y, 
				pWI->width, pWI->height, (~0), ZPixmap))) 
              return;

           mvGetColormap(pWI);

/* 
 * For each pixel in the returned Image.
 */

           for (yi = 0; yi < pWI->height; yi++) {
               for (xi = 0; xi < pWI->width; xi++) {
	           MVPel *pPel = mvFindPel(xi+pWI->x1-request_x, 
				           yi+pWI->y1-request_y);

/* 
 * If the pixel hasn't been labelled before.
 */

	           if (!pPel->colmap) { 

/* 
 * pPel->colmap serves as a 'Label'. 
 * label the pixel in the map with this window's cmap.
 */

/* 
 * If Pixel value can be discarded, this is where
 * you get the RGB value instead. 
 * Call a routine like mvFindColorInColormap() with the pixel
 * value, and Colormap as parameters.
 * The 'Label', instead of pPel->colmap could be a scratch bit ?
 * But if its a full 32 bit pixel, and there are no
 * free bits, you need to hang in extra bits somewhere.
 * Maybe a bitmask associated with Img ?
 */
	              pPel->colmap = pWI->colmap;

/* 
 * Get pixel value.
 */

	              pPel->pixel = XGetPixel(xim, xi, yi);
	              nPixelsUnLabeled--;
	              }
                   }
               }

/* 
 * Free image.
 */

           XDestroyImage(xim);
	   }
        }

}

/*
 * Get all the colors from this window's colormap.
 * That's slightly complicated when we hit
 * a true color or direct color visual.
 * Assumes that a global list of colmaps is present, and
 * that the Colors field was NULLed beforehand.
 */

static void
mvGetColormap(pWI)
MVWinVisInfo *pWI;
{

    if (!pWI->colmap->Colors) { 

/* 
 * This is the first time we're visiting.
 */

       MVColmap *pCmp = pWI->colmap;
       XVisualInfo *pVis = pWI->visinfo;
       XColor *pCol;
       int size = pVis->colormap_size;

/* 
 * Allocate enough memory.
 */

       pCmp->Colors = pCol = (XColor *)calloc((sizeof(XColor)), size);
       if (pVis->class == TrueColor || pVis->class == DirectColor) {
          unsigned long i;

/* 
 * We have to create a composite pixel value.
 */

          mvCalculateComposite(pWI);
          for (i = 0; i < (unsigned long)(size);i++, pCol++) 
	      pCol->pixel = mvCompositePixel(i, pCmp);
          }

       else {
          unsigned long i;

/* 
 * Fill in the pixel values by hand.
 */

          for (i = 0; i < (unsigned long)(size);) 
	      pCol++->pixel = i++;
          }
       XQueryColors(X->dpy, pCmp->cmap, pCmp->Colors, size);
       }

}

/* 
 * Given a VisualID, return a pointer to the VisualInfo structure.
 * Assumes that a global X->vlist for this screen has already
 * been created. Uses globals v->num_vis, and X->vlist.
 * Returns NULL if the vid is not matched.
 */

static XVisualInfo *
mvMatchVisual(vid)
VisualID vid;
{
    XVisualInfo *pVis = X->vlist;

    while (pVis < (X->vlist+v->num_vis)) {
       if (vid == pVis->visualid) 
          return (pVis);
       pVis++;
       }

    return (NULL);
}

/*
 * Calculate a composite pixel value that indexes into all
 * three primaries . Assumes Composite Calcs have been performed 
 * already on the colmap.
 */

static unsigned long
mvCompositePixel(i, pCmp)
unsigned long i;
MVColmap *pCmp;
{

    unsigned long val = 0;

    if (i < pCmp->rmax) 
       val |= i << pCmp->rshft;

    if (i < pCmp->gmax) 
       val |= i << pCmp->gshft;

    if (i < pCmp->bmax) 
       val |= i << pCmp->bshft;

    return (val);
}

/*
 * Calculate the offsets used to composite a pixel value for
 * the TrueColor & DirectColor cases
 * Assumes its called only on a True or DirectColor visual.
 */

static void
mvCalculateComposite(pWI)
MVWinVisInfo *pWI;
{

    MVColmap 	*pCmp = pWI->colmap;
    XVisualInfo *pVis = pWI->visinfo;

/* 
 * Check if this has been done before.
 */

    if (!pCmp->doComposite) {
       pCmp->doComposite = True;

/* 
 * These are the sizes of each primary map ... 
 */

       pCmp->red_mask = pVis->red_mask;
       pCmp->green_mask = pVis->green_mask;
       pCmp->blue_mask = pVis->blue_mask;
       pCmp->rmax = 1 << mvOnes(pVis->red_mask);
       pCmp->gmax = 1 << mvOnes(pVis->green_mask);
       pCmp->bmax = 1 << mvOnes(pVis->blue_mask);
       pCmp->rshft = mvShifts(pVis->red_mask);
       pCmp->gshft = mvShifts(pVis->green_mask);
       pCmp->bshft = mvShifts(pVis->blue_mask);
       pCmp->rgbshft = (16 - pVis->bits_per_rgb);
       }

}

/*
 * Calculate number of 1 bits in mask
 * Classic hack not written by this author.
 */

int
mvOnes(mask)
unsigned long mask;
{

    unsigned long y;

    y = (mask >> 1) &033333333333;
    y = mask - y - ((y >> 1) & 033333333333);
    return (((y + (y >> 3)) & 030707070707) % 077);
}

/*
 * Calculate the number of shifts till we hit the mask
 */

int 
mvShifts(mask)
unsigned long mask;
{
    int y = 0;

    if (mask) {
       while (!(mask&0x1)) {
          mask = mask >> 1;
          y++;
    	  }
       }

    return (y);
}

/*
 * find & creat a colmap struct for this cmap 
 * Assumes that colMaps was cleared before the first time
 * it is called.
 */

static MVColmap *
mvFindColormap(cmap)
Colormap cmap;
{

    MVColmap *pCmap;

/* 
 * If we've seen this cmap before, return its struct colmap.
 */

    for (pCmap = colMaps; pCmap; pCmap = pCmap->next) {
        if (cmap == pCmap->cmap)
           return (pCmap);
        }

/* 
 * First time for this cmap, creat & link.
 */

    pCmap = (MVColmap *) calloc(sizeof(MVColmap), 1);
    pCmap->next = colMaps;
    pCmap->cmap = cmap;
    colMaps = pCmap;
    return (pCmap);
}

/*
 * Use pixel value at x, y as an index into 
 * the colmap's list of Colors.
 * If the pixel value were not important, this would be called
 * in mvDoWindowsFrontToBack(), with the pixel value
 * and colmap as parameters, to get RGB values directly.
 */

XColor *
mvFindColorInColormap(x, y)
int x;
int y;
{

    MVPel 		*pPel = mvFindPel(x, y);
    MVColmap 		*pCmap = pPel->colmap;
    static XColor 	 scratch;

    if (pCmap->doComposite) { 

/* 
 * This is either True or DirectColor.
 * Treat the pixel value as 3 separate indices, composite
 * the color into scratch, return a pointer to scratch.
 */

       unsigned long pix = pPel->pixel;
       unsigned long index = (pix & pCmap->red_mask) >> pCmap->rshft; 
       scratch.red=pCmap->Colors[(pix & pCmap->red_mask)>>pCmap->rshft].red;
       scratch.green=pCmap->Colors[(pix & pCmap->green_mask)>>pCmap->gshft].green;
       scratch.blue=pCmap->Colors[(pix & pCmap->blue_mask)>>pCmap->bshft].blue;
       scratch.pixel=pix;
       return (&scratch);
       }

    else { 

/* 
 * This is simple.
 */

       return &(pCmap->Colors[pPel->pixel]);
       }

}

/*
 * Create an image_t from the Img we have.
 * We create a 24 bit image since we may have 
 * pixel values from different colormaps (easier
 * to just store the rgb values). Then, we'll
 * decide if we have to dither it later.
 */

image_t *
CreateImageFromImg()
{
    int 	 	 x, 
		 	 y,
		 	 pad;
    unsigned char	*im_ptr;
    struct rasterfile	 rh_tmp;

    image_t 	*image = new_image ();
    image->width = X->rect[(int) R_BOX].r_width ;
    image->height = X->rect[(int) R_BOX].r_height ;
    image->depth = 24;
    image->bytes_per_line = linelen (image->width, 24);
    pad = image->bytes_per_line - (image->width * image->depth / 8) ;
    image->data = (unsigned char *) ck_zmalloc 
		( (size_t)image->bytes_per_line * image->height );

    im_ptr = image->data ;

    for (y = 0; y < X->rect [(int) R_BOX].r_height; y++) {
        for (x=0; x < X->rect [(int) R_BOX].r_width; x++) {
            unsigned char pixel;
            XColor *col = mvFindColorInColormap(x, y);
            pixel = (unsigned char)((unsigned short)(col->blue) >> 8) ;
	    *im_ptr++ = pixel;
  	    pixel = (unsigned char)((unsigned short)(col->green) >> 8) ;
	    *im_ptr++ = pixel;
            pixel = (unsigned char)((unsigned short)(col->red) >> 8) ;
	    *im_ptr++ = pixel;
            }
	im_ptr += pad; 
        }  

/*
 * Since we just built a 24 bit image, we may have to adjust it...
 * In order to do so, let's use the `adjust_image' function. Unfortunately,
 * it expects a struct rasterfile * argument... so let's create one for
 * the hell of it here.
 */

    rh_tmp.ras_magic 	 = RAS_MAGIC;
    rh_tmp.ras_width     = image->width ;
    rh_tmp.ras_height    = image->height ;
    rh_tmp.ras_depth     = image->depth ;
    rh_tmp.ras_type      = RT_STANDARD ;
    rh_tmp.ras_length    = image->bytes_per_line * image->height;
    rh_tmp.ras_maptype   = RMT_NONE ;
    rh_tmp.ras_maplength = 0 ;

    adjust_image (&rh_tmp, image->data, rh_tmp.ras_length);

    return (image);
}
 

/*
 * Based on what mvLib returns, determine the best thing to do ...
 */

int
IsMultiVis(multivisFromMvLib)
int multivisFromMvLib;
{

    switch (multivisFromMvLib) {

/* 
 * No multiVis problems, do it the simple way.
 */

       case 0:  return (0);

/* 
 * Definitely a multidepth problem.
 */

       case 1:  return (1);

/* 
 * No depth problem , but colormap or visual class mismatch.
 * Do a best effort... if 24bit window exists, use the mvLib
 * else do a simple GetImage, and warn user that displayed image
 * may not show true colors. 
 */

       case 2:  
/*
if (w1)   have TrueColor capability 
*/
                 return (1);

       return (0);
       }    
}
