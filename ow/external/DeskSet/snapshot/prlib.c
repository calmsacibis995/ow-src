#ifndef lint
static char sccsid[] = "@(#)prlib.c	3.3 04/15/93 Copyright 1987-1990 Sun Microsystem, Inc." ;
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

#include "snapshot.h"
#include <errno.h>

#define  ESCAPE   128
#define  PIX_ERR  -1
#define  MAXDEPTH 32
#define  SVR4_LIMIT 1000

int pad_info[MAXDEPTH] ;

extern Vars v ;
extern char *mstr[] ;

static int read_encoded P((enum rasio_type, register int,
                           register u_char *, register int)) ;
static int sn_getc P((enum rasio_type)) ;
static int sn_fwrite P((char *, int, int, enum rasio_type)) ;

/* 
 * byte swapping longs.
 */

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
	   tmp [3-i] = *(bp + i);
       for (i = 0; i < 4; i++)
	   *(bp++) = tmp [i];
       }
}

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
 
/*
 * set_current_pad - Sets padding for given depth.
 */
 
void
set_current_pad (depth, pad)
int depth, pad ;
{
    pad_info [depth - 1] = pad;
}
 
/*
 * pad_value function... Retrieves padding for given depth.
 */
 
int
pad_value (depth)
int depth ;
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
/*
    int temp = width * depth;
*/
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

int
imagelen (width, height, depth)
int width;
int height;
int depth;
{
    return ( linelen (width, depth) * height);
}


void
adjust_image(rh, image, imagesize)
struct rasterfile *rh ;
unsigned char *image ;
int imagesize ;
{
  register unsigned char tmp ;
  int linelength, len;
  int i;

  linelength = linelen (rh->ras_width, rh->ras_depth);
 
/* 
 * If we're going to display this on a 24 bit framebuffer
 * and rh->ras_type *IS* RT_FORMAT_RGB, then we need to convert
 * it to BGR format since that is what XPutImage wants...
 *
 * OR...
 * 
 * if we're going to display this on a non-24 bit framebuffer,
 * and rh->ras_type *IS NOT* RT_FORMAT_RGB then we need to
 * convert it to RGB ...
 *
 * Note that we don't really need to check rh->ras_depth since
 * we only call this function if the depth is 24 or greater.
 *
 * Is this *CONFUSING* or what?
 */

  if ((v->depth >= 24  && rh->ras_type == RT_FORMAT_RGB) ||
      (v->depth < 24 && rh->ras_type != RT_FORMAT_RGB))
    switch (rh->ras_depth)
      {
/*
        case 24 : while (imagesize > 0)
                    {
                      tmp       = image[0] ;
                      image[0]  = image[2] ;
                      image[2]  = tmp ;
                      image     += 3 ;
                      imagesize -= 3 ;
                    } 
                  break ;
*/

        case 24 : 
		  for (i = 0; i < rh->ras_height ; i++) {
		      len = linelength; 
		      while (len >= 3) /* If we have at least 3 bytes left .. */
                        {
                          tmp       = image[0] ;
                          image[0]  = image[2] ;
                          image[2]  = tmp ;
                          image     += 3 ;
                          len -= 3 ;
			}
		      image += len;
		      }

		    break;
	
/* 
 * 32 bit can stay the same... we won't ever have a line
 * that ends on an odd byte boundary, so we should be OK.
 */
 
        case 32 : while (imagesize > 0)
                    {
                      tmp       = image[1] ;
                      image[1]  = image[3] ;
                      image[3]  = tmp ;
                      image     += 4 ;
                      imagesize -= 4 ;
                    } 
      }
}


void
closefile(stream)
FILE *stream ;
{
  if (v->ispopen)
    {
      PCLOSE(stream) ;
      v->ispopen = FALSE ;
    }
  else FCLOSE(stream) ;
}


/* Copy an entire image_t. */

image_t *
copy_image (image)
image_t *image ;
{
  int i;

  image_t *new = new_image ();

  new->type = image->type;
  new->width = image->width;
  new->height = image->height;
  new->depth = image->depth;
  new->cmaptype = image->cmaptype;
  new->cmapused = image->cmapused;
  new->bytes_per_line = image->bytes_per_line;
  new->used_malloc = image->used_malloc;
  if (new->cmapused > 0) {
     new->red = (unsigned char *) malloc (new->cmapused);
     new->green = (unsigned char *) malloc (new->cmapused);
     new->blue = (unsigned char *) malloc (new->cmapused);
     }

  for (i = 0; i < new->cmapused ; i++) {
      new->red[i] = image->red[i];
      new->green[i] = image->green[i];
      new->blue[i] = image->blue[i];
      }

  new->data = copy_imagedata (image);
  
  return (new);
}
  

/* Copy the data associated with an image into a new memory area. */

unsigned char *
copy_imagedata(image)
image_t *image ;
{
  unsigned char *data ;
  int length ;

  length = image->height * image->bytes_per_line;
/*  length = imagelen (image->width, image->height, image->depth) ; */
  data   = (unsigned char *) ck_zmalloc((size_t) length) ;
  MEMCPY((char *) data, (char *) image->data, length) ;
  return(data) ;
}


/* Dump a rasterfile, to the specified stream. */

int
dump_rasterfile(image, rtype)
image_t *image ;
enum rasio_type rtype ;
{
  unsigned long swaptest = 1;	    /* To check the native byte format */		
  image_t *new, *old ;
  struct rasterfile rh ;
  int len = 0 ;
  int linesize ;                    /* Number of bytes from one line to next */
  register int bytes, lines ;
  unsigned char *ptr = v->rbuf ;    /* Save initial pointer to buffer. */
  unsigned char *data ;
  unsigned char *tempdata ;
  unsigned char *tptr;
  int i, j;

  init_pad ();
  if (image->cmaptype == RMT_EQUAL_RGB)
    {
      old                 = new_image() ;
      old->width          = image->width ;
      old->height         = image->height ;
      old->depth          = image->depth ;
      old->bytes_per_line = image->bytes_per_line ;
      old->cmaptype       = image->cmaptype ;
      old->red            = (unsigned char *) malloc(CMAP_SIZE) ;
      old->green          = (unsigned char *) malloc(CMAP_SIZE) ;
      old->blue           = (unsigned char *) malloc(CMAP_SIZE) ;
      old->cmapused       = set_image_colormap(old) ;
      old->data           = copy_imagedata(image) ;
      new                 = compress(old) ;
      free_image(old) ;
      image               = new ;
    }

/* Initialize the rasterfile header. */

  rh.ras_magic     = RAS_MAGIC ;
  rh.ras_width     = image->width ;
  rh.ras_height    = image->height ;
  rh.ras_depth     = image->depth ;
  if (image->depth == 4)
     rh.ras_depth = 8;
  rh.ras_type      = RT_STANDARD ;

  linesize         = linelen (image->width, image->depth) ;

  rh.ras_length    = linesize * image->height ;
  rh.ras_maptype   = RMT_NONE ;
  rh.ras_maplength = 0 ;
 
/* Validate colormap if present. */

  if (image->cmapused != 0)
    {
      switch (rh.ras_maptype = image->cmaptype)
        {
          case RMT_NONE      : len = 0 ;
                               break ;
          case RMT_EQUAL_RGB : len = 3 ;
                               rh.ras_maplength = image->cmapused * 3 ;
                               break ;
          case RMT_RAW       : len = 1 ;
                               rh.ras_maplength = image->cmapused ;
                               break ;
          default            : return(PIX_ERR) ;
        }
      len = image->cmapused ;
    }

/* Write rasterfile header, colormap if any */
 
/*
 * Before writing the header byte swap the header if on intel machine
 */

  if (*(char *) &swaptest) 
     swaplong ((char *) &rh, sizeof (rh));

  if (sn_fwrite((char *) &rh, 1, sizeof(rh), rtype) != sizeof(rh))
    return(PIX_ERR) ;
  if (len)
    {
      if (sn_fwrite((char *) image->red, 1, len, rtype) != len)
        return(PIX_ERR) ;
      if (image->type == RMT_EQUAL_RGB)
        {
          if (sn_fwrite((char *) image->green, 1, len, rtype) != len)
            return(PIX_ERR) ;
          if (sn_fwrite((char *) image->blue,  1, len, rtype) != len)
            return(PIX_ERR) ;
        }
    }

  data  = image->data ;
  bytes = linesize ;
  lines = image->height ;
  len   = image->bytes_per_line ;
  if (bytes == len)
    {
      bytes *= lines ;
      lines = 1 ;
    }
 
  tempdata = (unsigned char *) malloc ((unsigned) bytes);
  tptr = tempdata;

  while (--lines >= 0)
    {
      if (image->depth == 24) {
	 tempdata = tptr;
	 for ( i = 0; i < image->width; i++) {
	     data++ ;
	     for (j=0; j < 3; j++)
		 *tempdata++ = *data++;
	     }
	 if (sn_fwrite((char *) tptr, 1, bytes, rtype) != bytes)
           return(PIX_ERR) ;
	 }
      else {
         if (sn_fwrite((char *) data, 1, bytes, rtype) != bytes)
           return(PIX_ERR) ;
         data += len ;
	 }
    }

  free (tptr);
  v->rbuf = ptr ;         /* Restore internal rasterfile buffer pointer. */

  if (image->cmaptype == RMT_EQUAL_RGB) free_image(new) ;
  return(0) ;
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
    ck_zfree((char *) image->data, image->used_malloc) ;
  FREE((char *) image) ;
  image = NULL ;
}


unsigned char *
get_image(rtype, rh)           /* Get rasterfile image from stream. */
enum rasio_type rtype ;
register struct rasterfile *rh ;
{
  unsigned char *data ;
  register int imagesize ;

  if (rh == 0) return(0) ;
  imagesize = imagelen (rh->ras_width, rh->ras_height, rh->ras_depth) ;

  if ((data = (unsigned char *) ck_zmalloc((size_t) imagesize)) == NULL)
    return(NULL) ;

  switch (rh->ras_type)
    {
      case RT_OLD          :
      case RT_STANDARD     :
      case RT_FORMAT_RGB   :  if (sn_fread((char *) data, 1, imagesize,
                                 rtype) == imagesize && rh->ras_depth < 24)
                               break ;

                             adjust_image(rh, data, imagesize) ;
                             break ;
 
      case RT_BYTE_ENCODED : if (read_encoded(rtype, rh->ras_length,
                                 data, imagesize) == 0 && rh->ras_depth < 24)
                               break ;

                             adjust_image(rh, data, imagesize) ;
    }
  return(data) ;                   
}


int
get_image_cmap(rtype, rh, im_cmap)     /* Get image colormap from stream. */
enum rasio_type rtype ;
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
          sn_fread((char *) cmap->red,   1, len, rtype) == len &&
          (cmap->type != RMT_EQUAL_RGB ||
          sn_fread((char *) cmap->green, 1, len, rtype) == len &&
          sn_fread((char *) cmap->blue,  1, len, rtype) == len)) error = 0 ;

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
      while (--len >= 0 && sn_getc(rtype) != EOF) continue ;

      if (len < 0) error = 0 ;
    }
  return(error) ;
}


int
get_image_header(rtype, rh)      /* Get Sun rasterfile header from stream. */
enum rasio_type rtype ;
struct rasterfile *rh ;
{
  register int i;
  unsigned long swaptest = 1;

  if (rh == 0) return(PIX_ERR) ;
/*
  return(sn_fread((char *) rh, 1, sizeof(*rh), rtype) == sizeof(*rh) &&
         rh->ras_magic == RAS_MAGIC ? 0 : PIX_ERR) ;
*/
  
/*
 * read the header, ans swap it if on LSB first machine before any
 * verification
 */

  i = sn_fread ((char *) rh, 1, sizeof (*rh), rtype);
  if (*(char *) &swaptest)
     swaplong ((char *) rh, sizeof(struct rasterfile));
  if ((i == sizeof (*rh)) && (rh->ras_magic == RAS_MAGIC))
     return (0);
  else
     return (PIX_ERR);
}


int
get_raster_len(image)
image_t *image ;
{
  int len ;

  len = sizeof(struct rasterfile) +
        imagelen (image->width, image->height, image->depth) ;

       if (image->cmaptype == RMT_RAW)       len +=  image->cmapused ;
  else if (image->cmaptype == RMT_EQUAL_RGB) len += (image->cmapused * 3) ;
  return(len) ;
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
  image->used_malloc = FALSE ;
  return((image_t *) image) ;
}


FILE *
openfile(filename)
char *filename ;
{
  char buf[MAXPATHLEN] ;
  int iscompressed, len ;
  FILE *fd ;

  iscompressed = 0 ;
  len = strlen(filename) ;
  if (len > 2 && !strncmp(&filename[len-2], ".Z", 2)) iscompressed = 1 ;

  if (!iscompressed && (fd = fopen(filename, "r")) != NULL) return(fd) ;
  if (iscompressed || errno == ENOENT)
    {

/* First see if the compressed file exists and is readable */

      if (!iscompressed) SPRINTF(buf, "%s.Z", filename) ;
      else               STRCPY(buf, filename) ;
      if ((fd = fopen(buf, "r")) != NULL)
        {
 
/* OK - it's there */
 
          FCLOSE(fd) ;
          SPRINTF(buf, "zcat %s", filename) ;
          if ((fd = popen(buf, "r")) != NULL)
            {
              v->ispopen = TRUE ;
              return(fd) ;
            }
        }
      message(mstr[(int) M_NOOPEN]) ;
      return((FILE *) NULL) ;
    }
  return(fd) ;
}


image_t *
rast_load(filename, rtype)
char *filename ;
enum rasio_type rtype ;
{
  image_t *image ;
  struct rasterfile rh ;

  message(mstr[(int) M_RASTCHK]) ;
  if (rtype == R_FILE)
    if ((v->fp = openfile(filename)) == NULL) return((image_t *) NULL) ;

  init_pad ();
  if (get_image_header(rtype, &rh) == PIX_ERR)
    {
      closefile(v->fp) ;
      message(mstr[(int) M_NOTRAST]) ;
      return((image_t *) NULL) ;
    }
      
  message(mstr[(int) M_LOADING]) ;

  image                 = new_image() ;
  image->type           = rh.ras_maptype ;
  image->width          = rh.ras_width ;
  image->height         = rh.ras_height ;
  image->depth          = rh.ras_depth ;
  image->bytes_per_line = linelen (image->width, image->depth) ;

  image->cmaptype       = rh.ras_maptype ;
  image->cmapused       = rh.ras_maplength / 3 ;
  if (image->cmaptype == RMT_EQUAL_RGB)
    {
      image->red   = (unsigned char *) malloc((unsigned) image->cmapused) ;
      image->green = (unsigned char *) malloc((unsigned) image->cmapused) ;
      image->blue  = (unsigned char *) malloc((unsigned) image->cmapused) ;
    }

  if (get_image_cmap(rtype, &rh, image) == PIX_ERR)
    {
      closefile(v->fp) ;
      message(mstr[(int) M_BADCOL]) ;
      free_image(image) ;
      return((image_t *) NULL) ;
    }

  if ((image->data = get_image(rtype, &rh)) == NULL)
    {
      closefile(v->fp) ;
      message(mstr[(int) M_NOREAD]) ;
      free_image(image) ;
      return((image_t *) NULL) ;
    }
  message(mstr[(int) M_LOADSING]) ;

  if (rtype == R_FILE) closefile(v->fp) ;
  v->filetype = RASTERFILE;
  return(image) ;
}


static int
read_encoded(rtype, incount, out, outcount)
enum rasio_type rtype ;
register int incount ;
register u_char *out ;
register int outcount ;
{
  register u_char c ;
  register int repeat ;

  while (1)
    {
      while (--incount >= 0 && --outcount >= 0 &&
             (c = sn_getc(rtype)) != ESCAPE) *out++ = c ;

      if (outcount < 0 || --incount < 0) break ;

      if ((repeat = sn_getc(rtype)) == 0) *out++ = c ;
      else
        {
          if ((outcount -= repeat) < 0 || --incount < 0) break ;

          c = sn_getc(rtype) ;
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


int
sn_fgetc(rtype)
enum rasio_type rtype ;
{
  int val ;

  if (rtype == R_FILE) return(fgetc(v->fp)) ;
  else
    { 
      val = *(v->rbuf) ;
      v->rbuf++ ;
      return(val) ;
    }
}


int
sn_fread(ptr, size, nitems, rtype)
char *ptr ;
int nitems, size ;
enum rasio_type rtype ;
{
  int len ;

  if (rtype == R_FILE) {

#ifdef SVR4
     if (v->ispopen) {
        if (nitems > SVR4_LIMIT) {
	   int i;
	   int nobytes = 0;
	   int bytes_read = 0;
	   for (i = 0; nobytes < (nitems * size); i++) {
	       bytes_read = fread (&ptr[i * SVR4_LIMIT], size, SVR4_LIMIT, v->fp);
	       if (bytes_read == 0)
	          break;
	       nobytes += bytes_read;
	       }
	   return (nobytes);
	   }
	}
#endif

     return (fread(ptr, size, nitems, v->fp)) ;
     }
  else
    {
      len = size * nitems ;
      MEMCPY(ptr, (char *) v->rbuf, len) ;
      v->rbuf += len ;
      return(nitems) ;
    }
}


static int
sn_fwrite(ptr, size, nitems, rtype)
char *ptr ;
int nitems, size ;
enum rasio_type rtype ;
{
  int len ;

  if (rtype == R_FILE) return(fwrite(ptr, size, nitems, v->fp)) ;
  else
    {
      len = size * nitems ;
      MEMCPY((char *) v->rbuf, ptr, len) ;
      v->rbuf += len ;
      return(nitems) ;
    }
}


static int
sn_getc(rtype)
enum rasio_type rtype ;
{
  if (rtype == R_FILE) return(getc(v->fp)) ;
  else                 return(*(v->rbuf)++) ;
}
