#ifndef lint
static char sccsid[] = "@(#)rast.c	3.2 01/29/93 Copyright 1987-1990 Sun Microsystem, Inc." ;
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
 */

#include "rast.h"
#include <errno.h>
#ifdef SVR4
#include <pixrect/rasterfile.h>
#else
#include <rasterfile.h>
#endif

#define  ESCAPE   128
#define  PIX_ERR  -1
#define  MAXDEPTH 32

int	pad_info [MAXDEPTH];

/*
 * init_pad - Initializes padding info for rasterfile padding.
 *            May need to change if image returned from XGetImage,
 *            use set_current_pad function for that.
 */
 
void
init_pad ()
{
    int i;
 
    for (i = 0; i < 17; i++)
        pad_info[i] = 16;
    for (i = 17; i < MAXDEPTH; i++)
        pad_info[i] = 32;
 
    pad_info[23] = 24;    
 
}
 
int
pad_value (depth)
{
    return (pad_info [depth - 1]);
}

/*
 * linelen function... The macro LINELEN just didn't seem to work for
 * all cases now that we've added support for depths of 4 and 24. 
 */

int
linelen (width, depth)
int width;
int depth;
{
    int temp2;
    int temp;
    int the_depth = depth;

    if (depth == 4) the_depth = 8;
    temp = width * the_depth;
    temp2 = (temp / 8) + 
	    (temp % pad_value (the_depth) ?
	      ((pad_value (the_depth) - (temp % pad_value (the_depth) )) / 8) :
		 0 );

    if (temp2 % 2) temp2++;

    return (temp2);
}

/*
 * imagelen function... Most of the work done in linelen function above.
 */

int imagelen (width, height, depth)
int width;
int height;
int depth;
{
    return ( linelen (width, depth) * height);
}

unsigned char *
adjust_image (rh, image)
struct rasterfile *rh ;
unsigned char *image;
{
    unsigned char *dptr, *optr, tmp;
    int i, j, k;

    int lineb = linelen (rh->ras_width, rh->ras_depth);
    unsigned char *newdata = (unsigned char *)
				ck_zmalloc ((size_t) (rh->ras_height *
						      rh->ras_width * 4));

    dptr = newdata;
    optr = image;
    for (i = 0; i < rh->ras_height ; i++) {
        for (j = 0; j < rh->ras_width ; j++) {
            *dptr++ = 0;
 	    if (rh->ras_type == RT_FORMAT_RGB) {
	       *dptr++ = *(optr + 2);
	       *dptr++ = *(optr + 1);
	       *dptr++ = *optr;
	       optr += 3;
	       }
   	    else {
               for (k = 0 ; k < 3 ; k++)
                   *dptr++ = *optr++;
	       }
            }
        for (j = 0; j < (lineb - (rh->ras_width * 3)) ; j++)
            optr++;
        }

    ck_zfree(image, 0); 
 
    rh->ras_depth = 32;
    return (newdata);
}

void
free_image(image)
image_t *image ;
{
  if (image == NULL) return ;
  if (image->red   != NULL) FREE((char *) image->red) ;
  if (image->green != NULL) FREE((char *) image->green) ;
  if (image->blue  != NULL) FREE((char *) image->blue) ;
  if (image->data  != NULL)
    ck_zfree((char *) image->data, 0) ;
  FREE((char *) image) ;
  image = NULL ;
}


unsigned char *
get_image(fp, rh)           /* Get rasterfile image from stream. */
FILE *fp;
register struct rasterfile *rh ;
{
  unsigned char *data ;
  int lineb = imagelen (rh->ras_width, rh->ras_height, rh->ras_depth);

  if (rh == 0) return(0) ;

  if ((data = (unsigned char *) ck_zmalloc((size_t) lineb)) == NULL)
    return(NULL) ;

  switch (rh->ras_type)
    {
      case RT_OLD          :
      case RT_STANDARD     :
      case RT_FORMAT_RGB   :  fread((char *) data, 1, lineb, fp) ;
			      break;
 
      case RT_BYTE_ENCODED :  read_encoded(rh->ras_length, data, 
						lineb, fp);
    }

  if (rh->ras_depth == 24)
     return (adjust_image (rh, data)); 
  else 
     return (data) ;
}


get_image_cmap(fp, rh, im_cmap)     /* Get image colormap from stream. */
FILE *fp;
register struct rasterfile *rh ;
image_t *im_cmap ;
{
  register int len, error = PIX_ERR ;
  register image_t *cmap ;
 
  if (rh == 0) return(error) ;
 
  if ((cmap = im_cmap) != NULL)
    {
      image_t local_cmap ;
 
      len = cmap->cmapused ;
 
/* If type or length in colormap does not match rasterfile
 * header, create a new colormap struct.
 */

      if (rh->ras_maptype != cmap->type ||
          rh->ras_maplength != len * (cmap->type == RMT_EQUAL_RGB ? 3 : 1))
        {
          len = rh->ras_maplength ;
          cmap = &local_cmap ;

          cmap->red = NULL ;
          if (len && !(cmap->red = (u_char *) malloc((unsigned) len)))
            return error ;

          cmap->type = rh->ras_maptype ;
          if (cmap->type == RMT_EQUAL_RGB)
            {
              len /= 3 ;
              cmap->green = cmap->red   + len ;
              cmap->blue  = cmap->green + len ;
            }
          cmap->cmapused = len ;
        }

/* Read colormap data, if any */

      if (len == 0 || cmap->type == RMT_NONE ||
          fread((char *) cmap->red,   1, len, fp) == len &&
          (cmap->type != RMT_EQUAL_RGB ||
          fread((char *) cmap->green, 1, len, fp) == len &&
          fread((char *) cmap->blue,  1, len, fp) == len)) error = 0 ;

/* Copy new colormap over user's */

      if (cmap == &local_cmap)
        if (error)
          {
            if (cmap->red != NULL) (void) free((char *) cmap->red) ;
          }
        else *im_cmap = *cmap ;
    }
  else
    {

/* User doesn't want colormap, discard it */

      len = rh->ras_maplength ;
      while (--len >= 0 && getc(fp) != EOF) continue ;

      if (len < 0) error = 0 ;
    }
  return(error) ;
}

static
swaplong (bp, n)
register char *bp;
register unsigned n;
{
    register char *ep = bp + n;
    char tmp[4];
    int i;

    while (bp < ep) {
       for (i = 0; i < 4; i++) 
   	   tmp [3 - i] = *(bp + i);
       for (i = 0; i < 4; i++) 
	   *(bp++) = tmp [i];
       }
}

get_image_header(fp, rh)      /* Get Sun rasterfile header from stream. */
FILE *fp;
register struct rasterfile *rh ;
{
  register int i;
  unsigned long swaptest = 1;

  if (rh == 0) return(PIX_ERR) ;
  i = fread ((char *) rh, 1, sizeof (*rh), fp);
  if (*(char *) &swaptest)
     swaplong ((char *) rh, sizeof (struct rasterfile));
  if ((i == sizeof (*rh)) && (rh->ras_magic == RAS_MAGIC))
     return (0);
  else
     return (PIX_ERR);
}

image_t *
new_image()
{
  image_t *image ;
 
  image = (image_t *) malloc((unsigned int) sizeof(image_t)) ;
  memset((char *) image, 0, sizeof(image_t)) ;
  image->type     = RT_STANDARD ;
  image->cmaptype = RMT_NONE ;
  image->cmapused = 0 ;
  image->red      = NULL ;
  image->green    = NULL ;
  image->blue     = NULL ;
  image->data     = NULL ;
  return((image_t *) image) ;
}


image_t *
rast_load(fp)
FILE *fp;
{
  image_t *image ;
  struct rasterfile rh ;

  init_pad ();
  if (get_image_header(fp, &rh) == PIX_ERR)
    {
      FCLOSE(fp) ;
      return((image_t *) NULL) ;
    }
      

  image                 = new_image() ;
  image->type           = rh.ras_maptype ;
  image->width          = rh.ras_width ;
  image->height         = rh.ras_height ;
  image->depth          = rh.ras_depth ;
  image->bytes_per_line = linelen (image->width, image->depth) ;
  image->length		= rh.ras_length;

  image->cmaptype       = rh.ras_maptype ;
  image->cmapused       = rh.ras_maplength / 3 ;
  if (image->cmaptype == RMT_EQUAL_RGB)
    {
      image->red   = (unsigned char *) malloc((unsigned) image->cmapused) ;
      image->green = (unsigned char *) malloc((unsigned) image->cmapused) ;
      image->blue  = (unsigned char *) malloc((unsigned) image->cmapused) ;
    }

  if (get_image_cmap(fp, &rh, image) == PIX_ERR)
    {
      FCLOSE(fp) ;
      free_image(image) ;
      return((image_t *) NULL) ;
    }

  if ((image->data = get_image(fp, &rh)) == NULL)
    {
      FCLOSE(fp) ;
      free_image(image) ;
      return((image_t *) NULL) ;
    }

  image->depth          = rh.ras_depth ;
  image->bytes_per_line = linelen (image->width, image->depth) ;

  FCLOSE(fp) ;
  return(image) ;
}


static int
read_encoded(incount, out, outcount, fp)
register int incount ;
register u_char *out ;
register int outcount ;
FILE *fp;
{
  register u_char c ;
  register int repeat ;

  while (1)
    {
      while (--incount >= 0 && --outcount >= 0 &&
             (c = getc(fp)) != ESCAPE) *out++ = c ;

      if (outcount < 0 || --incount < 0) break ;

      if ((repeat = getc(fp)) == 0) *out++ = c ;
      else
        {
          if ((outcount -= repeat) < 0 || --incount < 0) break ;

          c = getc(fp);
          do
            *out++ = c ;
          while (--repeat != -1) ;
        }
    }

       if (outcount < 0) incount-- ;
  else if (incount < -1) outcount-- ;

  if ((incount += 2) > 0) return(incount) ;
  return -(++outcount) ;
}

